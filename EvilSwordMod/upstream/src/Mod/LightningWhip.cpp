// SPDX-License-Identifier: LGPL-3.0-or-later
#include "LightningWhip.h"
#include "ModRegistry.h"
#include "../LawnApp.h"
#include "../Lawn/Board.h"
#include "../Lawn/Plant.h"
#include "../Lawn/Zombie.h"
#include "../SexyAppFramework/graphics/Graphics.h"
#include "../SexyAppFramework/graphics/Image.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace LightningWhip {
struct Shock {
    Board* board;
    ZombieID zombie;
    int ticks;
    int cadence;
};
static std::vector<Shock> shocks;
static Board* trackedBoard=nullptr;
static unsigned int trackedCounter=0;

static void EnsureBoard(Board* board) {
    if (board!=trackedBoard || (board && board->mMainCounter<trackedCounter))
        shocks.clear();
    trackedBoard=board;
    trackedCounter=board ? board->mMainCounter : 0;
}

bool Is(const Plant* plant) {
    if (!plant || static_cast<int>(plant->mSeedType)<2000) return false;
    const auto* def=gModRegistry.FindPlantByRuntimeId(static_cast<int>(plant->mSeedType));
    return def && def->id=="LightningWhip";
}

static bool CanShock(Zombie* zombie) {
    return zombie && !zombie->mMindControlled && !zombie->IsDeadOrDying() &&
        zombie->mZombieType!=ZombieType::ZOMBIE_BUNGEE &&
        zombie->mZombiePhase!=ZombiePhase::PHASE_DIGGER_TUNNELING &&
        zombie->mZombiePhase!=ZombiePhase::PHASE_DIGGER_TUNNELING_PAUSE_WITHOUT_AXE;
}

static void ApplyShock(Board* board, Zombie* zombie, int damage) {
    if (!CanShock(zombie)) return;
    EnsureBoard(board);
    const ZombieID id=board->ZombieGetID(zombie);
    constexpr unsigned int bypassShield=1U<<DamageFlags::DAMAGE_BYPASSES_SHIELD;
    zombie->TakeDamage(damage,bypassShield); // Shields are ignored; helmets still absorb damage.
    Zombie* survivor=board->ZombieTryToGet(id);
    if (!survivor || survivor->IsDeadOrDying()) return;
    for (Shock& shock:shocks) {
        if (shock.board==board && shock.zombie==id) {
            shock.ticks=200;
            return;
        }
    }
    shocks.push_back({board,id,200,20});
}

void Update(Plant* plant) {
    if (!Is(plant) || plant->mDead || !plant->IsInPlay() || plant->mIsAsleep ||
        plant->mSquished || plant->mOnBungeeState!=PlantOnBungeeState::NOT_ON_BUNGEE ||
        plant->mStateCountdown>0) return;
    Board* board=plant->mBoard;
    Zombie* target=nullptr;
    int nearest=100000;
    Zombie* zombie=nullptr;
    while (board->IterateZombies(zombie)) {
        if (!CanShock(zombie) || zombie->mRow!=plant->mRow ||
            zombie->mOnHighGround!=plant->IsOnHighGround()) continue;
        const int dx=zombie->mX-plant->mX;
        if (dx>=30 && dx<=300 && dx<nearest) {
            nearest=dx;
            target=zombie;
        }
    }
    if (!target) return;
    const int centerX=target->mX+40, centerY=target->mY+40, targetRow=target->mRow;
    plant->mTargetX=centerX;
    plant->mTargetY=centerY;
    plant->mShootingCounter=18;
    const int strike=plant->mLaunchCounter%3;
    constexpr int strikeDamage[]={30,40,100}; // Scaled from the 15/20/50 Dead Cells chain.
    plant->mLaunchCounter=(strike+1)%3;
    plant->mStateCountdown=strike==2 ? 140 : 70;
    ApplyShock(board,target,strikeDamage[strike]);
    int chained=0;
    zombie=nullptr;
    while (board->IterateZombies(zombie)) {
        if (chained>=3) break;
        if (zombie==target || !CanShock(zombie) ||
            std::abs(zombie->mRow-targetRow)>1 ||
            std::abs(zombie->mX-centerX)>120) continue;
        ApplyShock(board,zombie,strikeDamage[strike]/2);
        ++chained;
    }
}

void UpdateShocks(Board* board) {
    EnsureBoard(board);
    if (!board || board->mPaused || !board->mApp ||
        board->mApp->mGameScene!=GameScenes::SCENE_PLAYING) return;
    for (auto it=shocks.begin();it!=shocks.end();) {
        if (it->board!=board) { it=shocks.erase(it); continue; }
        Zombie* zombie=board->ZombieTryToGet(it->zombie);
        if (!CanShock(zombie) || it->ticks<=0) { it=shocks.erase(it); continue; }
        --it->ticks;
        if (--it->cadence<=0) {
            constexpr unsigned int bypassShield=1U<<DamageFlags::DAMAGE_BYPASSES_SHIELD;
            zombie->TakeDamage(5,bypassShield); // 25 damage/second for two seconds.
            it->cadence=20;
        }
        ++it;
    }
}

bool Draw(Plant* plant, Sexy::Graphics* graphics, float y) {
    if (!Is(plant)) return false;
    plant->DrawShadow(graphics,0,y);
    if (auto* image=Plant::GetImage(plant->mSeedType)) {
        const double phase=(plant->mBoard ? plant->mBoard->mMainCounter : 0)*0.06+
            plant->mPlantCol*0.6+plant->mRow;
        const double sway=0.06*std::sin(phase);
        graphics->DrawImageRotated(image,0,(int)y,sway,39,69);
    }
    if (plant->mShootingCounter>0 && plant->mTargetX>=0) {
        Sexy::Graphics bolt(*graphics);
        const int x0=58, y0=(int)y+28;
        const int x1=plant->mTargetX-plant->mX;
        const int y1=plant->mTargetY-plant->mY+(int)y;
        int lastX=x0,lastY=y0;
        for (int segment=1;segment<=6;++segment) {
            const int x=x0+(x1-x0)*segment/6;
            const int jitter=(segment==6 ? 0 :
                (int)(8*std::sin((plant->mBoard->mMainCounter+segment*13)*1.7)));
            const int yy=y0+(y1-y0)*segment/6+jitter;
            bolt.SetColor(Sexy::Color(0,220,255));
            bolt.DrawLine(lastX,lastY-1,x,yy-1);
            bolt.DrawLine(lastX,lastY+1,x,yy+1);
            bolt.SetColor(Sexy::Color::White);
            bolt.DrawLine(lastX,lastY,x,yy);
            lastX=x; lastY=yy;
        }
    }
    return true;
}
}
