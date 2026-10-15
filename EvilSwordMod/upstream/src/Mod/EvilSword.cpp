// SPDX-License-Identifier: LGPL-3.0-or-later
#include "EvilSword.h"
#include "ModRegistry.h"
#include "../LawnApp.h"
#include "../Lawn/Plant.h"
#include "../Lawn/Zombie.h"
#include "../Lawn/Board.h"
#include "../Lawn/Cutscene.h"
#include "../Lawn/Widget/AlmanacDialog.h"
#include "../Lawn/System/PlayerInfo.h"
#include "../Lawn/System/ProfileMgr.h"
#include "../SexyAppFramework/graphics/Graphics.h"
#include "../SexyAppFramework/graphics/Image.h"
#include "../SexyAppFramework/graphics/GLImage.h"
#include "../SexyAppFramework/graphics/GLPlatform.h"
#include "../SexyAppFramework/imagelib/ImageLib.h"
#include "Trainer.h"
#include "../Sexy.TodLib/TodDebug.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <vector>

namespace EvilSword {
static bool captureRequested=false;
static bool slashFramesLoaded=false;
static std::array<Sexy::Image*,7> slashA{};
static std::array<Sexy::Image*,7> slashB{};
static void LoadSlashFrames() {
    if (slashFramesLoaded || !gLawnApp) return;
    slashFramesLoaded=true;
    for (int frame=0;frame<7;++frame) {
        char path[64];
        std::snprintf(path,sizeof(path),"images/EvilSwordSlashA_%02d.png",frame);
        slashA[frame]=gLawnApp->GetImage(path);
        std::snprintf(path,sizeof(path),"images/EvilSwordSlashB_%02d.png",frame);
        slashB[frame]=gLawnApp->GetImage(path);
    }
}
bool Is(const Plant* p) {
    if (!p || static_cast<int>(p->mSeedType)<2000) return false;
    auto def=gModRegistry.FindPlantByRuntimeId(static_cast<int>(p->mSeedType));
    return def && def->id=="EvilSword";
}
static void Curse(Plant* p, const char* reason) {
    if (!p || p->mDead) return;
    TodLog("[EvilSword] curse row=%d col=%d reason=%s",p->mRow,p->mPlantCol,reason);
    p->mPlantHealth=0;
    p->Die();
}
static bool GroundContact(Zombie* z) {
    if (z->mMindControlled || z->IsDeadOrDying() || z->IsFlying()) return false;
    if (z->mZombieType==ZombieType::ZOMBIE_BUNGEE) return false;
    return z->mZombieHeight==ZombieHeight::HEIGHT_ZOMBIE_NORMAL &&
        z->mZombiePhase!=ZombiePhase::PHASE_DIGGER_TUNNELING &&
        z->mZombiePhase!=ZombiePhase::PHASE_DIGGER_TUNNELING_PAUSE_WITHOUT_AXE &&
        z->mZombiePhase!=ZombiePhase::PHASE_IMP_GETTING_THROWN;
}
void Update(Plant* p) {
    if (!Is(p) || p->mDead || !p->IsInPlay()) return;
    if (p->mPlantHealth<1 || p->mRecentlyEatenCountdown>0) { Curse(p,"damage"); return; }
    auto rect=p->GetPlantRect();
    Zombie* z=nullptr;
    // Check the actual body before attacking. Neither pumpkin priority nor a living wall
    // in the neighbouring cell can hide contact with this collision rectangle.
    while (p->mBoard->IterateZombies(z)) {
        if (z->mRow!=p->mRow || z->IsDeadOrDying() || z->mMindControlled) continue;
        if (z->mZombieType==ZombieType::ZOMBIE_BUNGEE) {
            if (!z->mHitUmbrella && z->mTargetCol==p->mPlantCol &&
                z->mZombiePhase==ZombiePhase::PHASE_BUNGEE_GRABBING) { Curse(p,"bungee"); return; }
        } else if (GroundContact(z) && rect.Intersects(z->GetZombieAttackRect())) {
            Curse(p,"body-contact"); return;
        }
    }
    if (p->mStateCountdown>0) return;
    Zombie* target=nullptr;
    int nearest=100000;
    z=nullptr;
    while (p->mBoard->IterateZombies(z)) {
        if (z->mRow!=p->mRow || !GroundContact(z) || z->mOnHighGround!=p->IsOnHighGround()) continue;
        auto zr=z->GetZombieRect();
        int gap=zr.mX-(rect.mX+rect.mWidth);
        // Exactly one 80px tile forward from the body. No rear or aerial attack.
        if (gap>=0 && gap<=80 && zr.mX>=rect.mX && gap<nearest) { target=z; nearest=gap; }
    }
    if (!target) return;
    int damage=1500;
    if (target->mZombieType==ZombieType::ZOMBIE_PAIL ||
        target->mZombieType==ZombieType::ZOMBIE_FOOTBALL) damage=250;
    if (target->mZombieType==ZombieType::ZOMBIE_GARGANTUAR || target->mZombieType==ZombieType::ZOMBIE_REDEYE_GARGANTUAR)
        damage=(target->mBodyMaxHealth+1)/2;
    // TakeDamage still routes through helmets; body damage is the engine's true bypass.
    target->TakeBodyDamage(damage,0U);
    p->mStateCountdown=100;
    p->mShootingCounter=18;
    p->mLaunchCounter=(p->mLaunchCounter+1)%3; // Dead Cells EvilSword's A, B, A strike chain.
    TodLog("[EvilSword] slash zombie=%d damage=%d body=%d helm=%d shield=%d",(int)target->mZombieType,damage,target->mBodyHealth,target->mHelmHealth,target->mShieldHealth);
}
bool Bite(Zombie* z, Plant* target) {
    if (Is(target)) { Curse(target,"bite"); return true; }
    if (target && target->mBoard && target->mSeedType==SeedType::SEED_PUMPKINSHELL) {
        Plant* p=target->mBoard->GetTopPlantAt(target->mPlantCol,target->mRow,PlantPriority::TOPPLANT_ONLY_NORMAL_POSITION);
        if (Is(p)) Curse(p,"bite-through-pumpkin");
    }
    return false;
}
bool Draw(Plant* p, Sexy::Graphics* g, float y) {
    if (!Is(p)) return false;
    p->DrawShadow(g,0,y);
    // GetImage allocates a GL image; the registered seed image is already cached.
    auto image=Plant::GetImage(p->mSeedType);
    if (image) {
        const double sway=p->mBoard && p->IsInPlay() ?
            0.085*std::sin((p->mBoard->mMainCounter+p->mRow*29+p->mPlantCol*11)*0.055) : 0.0;
        if (p->mShootingCounter>0) {
            const double progress=(18.0-p->mShootingCounter)/18.0;
            g->DrawImageRotated(image,0,(int)y,sway*0.2-0.24*std::sin(progress*3.14159265),40,68);
            LoadSlashFrames();
            const int frame=std::clamp((18-p->mShootingCounter)*7/18,0,6);
            Sexy::Image* slash=(p->mLaunchCounter==2 ? slashB[frame] : slashA[frame]);
            if (slash) {
                Sexy::Graphics effect(*g);
                effect.SetColorizeImages(true);
                effect.SetColor(Sexy::Color(255,79,0));
                effect.DrawImage(slash,20,(int)y-12);
            }
        } else g->DrawImageRotated(image,0,(int)y,sway,40,68);
    }
    return true;
}
void Unlock(PlayerInfo* p) {
    if (!p || !gModRegistry.FindPlant("EvilSword")) return;
    p->mFinishedAdventure=std::max(p->mFinishedAdventure,2U);
    p->mHasUnlockedMinigames=p->mHasUnlockedPuzzleMode=p->mHasUnlockedSurvivalMode=1;
    p->mNeedsMessageOnGameSelector=p->mNeedsMagicTacoReward=0;
    for (auto& r:p->mChallengeRecords) r=std::max(r,20U);
    for (int i=STORE_ITEM_PLANT_GATLINGPEA;i<=STORE_ITEM_PLANT_IMITATER;++i) p->mPurchases[i]=1;
    for (int i:{STORE_ITEM_GOLD_WATERINGCAN,STORE_ITEM_PHONOGRAPH,STORE_ITEM_GARDENING_GLOVE,
        STORE_ITEM_MUSHROOM_GARDEN,STORE_ITEM_WHEEL_BARROW,STORE_ITEM_STINKY_THE_SNAIL,
        STORE_ITEM_POOL_CLEANER,STORE_ITEM_ROOF_CLEANER,STORE_ITEM_AQUARIUM_GARDEN,
        STORE_ITEM_TREE_OF_WISDOM,STORE_ITEM_FIRSTAID}) p->mPurchases[i]=1;
    p->mPurchases[STORE_ITEM_PACKET_UPGRADE]=4;
    p->mPurchases[STORE_ITEM_BONUS_LAWN_MOWER]=2;
    p->mPurchases[STORE_ITEM_RAKE]=3;
    for(int i:{STORE_ITEM_FERTILIZER,STORE_ITEM_BUG_SPRAY,STORE_ITEM_CHOCOLATE,STORE_ITEM_TREE_FOOD})
        p->mPurchases[i]=std::max(p->mPurchases[i],1020U);
    p->mCoins=99999;
    p->mHasSeenUpsell=1;
}

// Local file bridge for repeatable in-engine verification, no input focus required.
// Every operation runs on the game's thread and only in the mod's isolated savedir.
void Bridge() {
    auto app=gLawnApp;
    if (!app || !app->mLoadingThreadCompleted || !gModRegistry.FindPlant("EvilSword")) return;
    // The automated game session has no desktop focus. Keep simulation running
    // only when the local test marker is present; normal launches retain focus rules.
    if (std::filesystem::exists(Sexy::PathFromU8(Sexy::GetAppDataPath("test-bridge.enabled")))) {
        app->mActive=true;
        app->mHasFocus=true;
    }
    static unsigned int counter=0;
    if (++counter%10) return;
    auto path=Sexy::PathFromU8(Sexy::GetAppDataPath("command.txt"));
    std::ifstream in(path);
    std::string line;
    if (!std::getline(in,line)) return;
    in.close();
    std::filesystem::remove(path);
    std::istringstream cmd(line); std::string action; cmd>>action;
    if (app->mTitleScreen) app->LoadingCompleted();
    if (action=="menu") app->ShowGameSelector();
    else if(action=="capture") captureRequested=true;
    else if(action=="trainer") {
        std::string choice; cmd>>choice;
        if(choice=="sun") Trainer::KeyDown(Sexy::KeyCode::KEYCODE_F6);
        if(choice=="recharge") Trainer::KeyDown(Sexy::KeyCode::KEYCODE_F7);
        if(choice=="invincible") Trainer::KeyDown(Sexy::KeyCode::KEYCODE_F8);
    }
    else if(action=="level") {
        int n=1; cmd>>n; app->mPlayerInfo->mLevel=std::clamp(n,1,50);
        app->PreNewGame(GameMode::GAMEMODE_ADVENTURE,false);
    } else if(action=="almanac") app->DoAlmanacDialog(SeedType(2000),ZombieType::ZOMBIE_INVALID);
    else if(action=="plant" && app->mBoard) {
        int type,row,col; cmd>>type>>row>>col;
        if (row>=0 && row<5 && col>=0 && col<9 &&
            (gModRegistry.FindPlantByRuntimeId(type) || (type>=0 && type<49)))
            app->mBoard->AddPlant(col,row,SeedType(type),SeedType::SEED_NONE);
    } else if(action=="zombie" && app->mBoard) {
        int type,row; float x; cmd>>type>>row>>x;
        if (type>=0 && type<33 && row>=0 && row<5) {
            auto z=app->mBoard->AddZombieInRow(ZombieType(type),row,0);
            if(z) { z->mPosX=x; z->mX=(int)x; }
        }
    } else if(action=="start" && app->mBoard) {
        app->mBoard->mCutScene->mSeedChoosing=false;
        app->mBoard->mCutScene->mCutsceneTime=0;
        app->mGameScene=GameScenes::SCENE_PLAYING;
        app->mBoard->mSunMoney=9990;
        app->mBoard->mZombieCountDown=100000;
    } else if(action=="clear" && app->mBoard) {
        Zombie* z=nullptr; while(app->mBoard->IterateZombies(z)) z->DieNoLoot();
        Plant* p=nullptr; while(app->mBoard->IteratePlants(p)) p->Die();
    }
    std::ofstream out(Sexy::PathFromU8(Sexy::GetAppDataPath("state.txt")));
    out<<"scene "<<(int)app->mGameScene<<"\n";
    out<<"trainer sun "<<Trainer::InfiniteSun()<<" recharge "<<Trainer::InstantRecharge()<<" invincible "<<Trainer::InvinciblePlants()<<"\n";
    if(app->mPlayerInfo) out<<"unlocked "<<app->mPlayerInfo->mFinishedAdventure<<" slots "<<app->mPlayerInfo->mPurchases[STORE_ITEM_PACKET_UPGRADE]+6<<"\n";
    if(app->mBoard) {
        Plant* p=nullptr; while(app->mBoard->IteratePlants(p)) out<<"plant "<<(int)p->mSeedType<<" "<<p->mRow<<" "<<p->mPlantCol<<" hp "<<p->mPlantHealth<<"\n";
        Zombie* z=nullptr; while(app->mBoard->IterateZombies(z)) out<<"zombie "<<(int)z->mZombieType<<" "<<z->mRow<<" x "<<z->mPosX<<" hp "<<z->mBodyHealth<<" helm "<<z->mHelmHealth<<" phase "<<(int)z->mZombiePhase<<"\n";
    }
}
void CaptureFrame(int width,int height) {
    if(!captureRequested || width<=0 || height<=0) return;
    captureRequested=false;
    std::vector<unsigned char> raw(width*height*4);
    glReadPixels(0,0,width,height,GL_RGBA,GL_UNSIGNED_BYTE,raw.data());
    ImageLib::Image frame;
    frame.mWidth=width; frame.mHeight=height;
    frame.mBits=new uint32_t[width*height];
    for(int y=0;y<height;++y) for(int x=0;x<width;++x) {
        const auto* pixel=&raw[(y*width+x)*4];
        frame.mBits[(height-1-y)*width+x]=(uint32_t(pixel[3])<<24)|
            (uint32_t(pixel[0])<<16)|(uint32_t(pixel[1])<<8)|uint32_t(pixel[2]);
    }
    ImageLib::WritePNGImage(Sexy::GetAppDataPath("capture.png"),&frame);
}
}
