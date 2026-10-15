// SPDX-License-Identifier: LGPL-3.0-or-later
#include "Trainer.h"
#include "EvilSword.h"
#include "../Lawn/Board.h"
#include "../Lawn/Plant.h"
#include "../Lawn/SeedPacket.h"
#include "../Lawn/ZenGarden.h"
#include "../LawnApp.h"
#include "../SexyAppFramework/graphics/Graphics.h"
#include "../Resources.h"
#include "../Sexy.TodLib/Reanimator.h"
#include "../Sexy.TodLib/TodParticle.h"
#include "../Sexy.TodLib/TodCommon.h"
#include <algorithm>

namespace Trainer {
static bool sun=false, recharge=false, invincible=false;
static bool moveTool=false;
static Board* moveBoard=nullptr;
static unsigned int movePlantId=0;
static const Sexy::Rect moveButton(414,556,112,33);
static std::string moveMessage;

static void ResetMoveSelection() {
    movePlantId=0;
    moveMessage=moveTool ? "点击植物，再点空格" : "";
}

static bool CanUseMoveTool(Board* board) {
    return board && board->mApp && board->mApp->mGameScene==GameScenes::SCENE_PLAYING &&
        !board->mPaused && board->mApp->GetDialogCount()==0 && board->mBoardFadeOutCounter<0;
}

static void ToggleMoveTool(Board* board) {
    if (!moveTool) board->RefreshSeedPacketFromCursor();
    moveTool=!moveTool;
    moveBoard=board;
    ResetMoveSelection();
}

static bool CoversCell(const Plant* plant, int col, int row) {
    return plant && !plant->mDead && plant->mRow==row &&
        (plant->mPlantCol==col ||
         (plant->mSeedType==SeedType::SEED_COBCANNON && plant->mPlantCol+1==col));
}

static bool MoveSelectedPlant(Board* board, Plant* selected, int col, int row) {
    const int oldCol=selected->mPlantCol, oldRow=selected->mRow;
    if (col==oldCol && row==oldRow) {
        moveMessage="仍在原位；请选择其他格";
        return false;
    }
    const bool zen=board->mApp->mGameMode==GameMode::GAMEMODE_CHALLENGE_ZEN_GARDEN;
    const bool cob=selected->mSeedType==SeedType::SEED_COBCANNON;
    const int width=cob ? 2 : 1;
    Plant* member=nullptr;
    while (board->IteratePlants(member)) {
        if (member->mPlantCol>=oldCol && member->mPlantCol<oldCol+width &&
            member->mRow==oldRow &&
            (member->NotOnGround() ||
             member->mOnBungeeState==PlantOnBungeeState::GETTING_GRABBED_BY_BUNGEE)) {
            moveMessage="植物正在移动或被抓取";
            return false;
        }
    }
    if (col<0 || row<0 || col+width>MAX_GRID_SIZE_X || row>=MAX_GRID_SIZE_Y ||
        oldCol<0 || oldCol+width>MAX_GRID_SIZE_X || oldRow<0 || oldRow>=MAX_GRID_SIZE_Y) {
        moveMessage="目标超出草坪";
        return false;
    }
    for (int offset=0;offset<width;++offset) {
        if (board->mGridSquareType[col+offset][row]!=board->mGridSquareType[oldCol+offset][oldRow] ||
            board->GetCraterAt(col+offset,row) || board->GetGraveStoneAt(col+offset,row) ||
            board->GetScaryPotAt(col+offset,row) || board->IsIceAt(col+offset,row)) {
            moveMessage="目标地形不适合";
            return false;
        }
        Plant* occupant=nullptr;
        while (board->IteratePlants(occupant)) {
            if (CoversCell(occupant,col+offset,row) &&
                !(occupant->mPlantCol>=oldCol && occupant->mPlantCol<oldCol+width &&
                  occupant->mRow==oldRow)) {
                moveMessage="目标格已有植物";
                return false;
            }
        }
    }
    if (zen) {
        if (selected->mPottedPlantIndex<0) {
            moveMessage="请选择花园中的盆栽";
            return false;
        }
        board->mApp->mZenGarden->MovePlant(selected,col,row);
    } else {
        Plant* plant=nullptr;
        while (board->IteratePlants(plant)) {
            if (plant->mPlantCol<oldCol || plant->mPlantCol>=oldCol+width ||
                plant->mRow!=oldRow || plant->NotOnGround()) continue;
            const int newCol=plant->mPlantCol+(col-oldCol);
            const int dx=board->GridToPixelX(newCol,row)-
                board->GridToPixelX(plant->mPlantCol,oldRow);
            const int dy=board->GridToPixelY(newCol,row)-
                board->GridToPixelY(plant->mPlantCol,oldRow);
            plant->mPlantCol=newCol;
            plant->mRow=row;
            plant->mStartRow=row;
            plant->mX+=dx;
            plant->mY+=dy;
            const int oldOrder=plant->mRenderOrder;
            plant->mRenderOrder=plant->CalcRenderOrder();
            const int orderDelta=plant->mRenderOrder-oldOrder;
            const ReanimationID reanims[]={plant->mBodyReanimID,plant->mHeadReanimID,
                plant->mHeadReanimID2,plant->mHeadReanimID3,plant->mBlinkReanimID,
                plant->mLightReanimID,plant->mSleepingReanimID};
            for (ReanimationID id : reanims)
                if (auto* reanim=board->mApp->ReanimationTryToGet(id)) {
                    reanim->mRenderOrder+=orderDelta;
                    if (id==plant->mSleepingReanimID) {
                        reanim->mOverlayMatrix.m02+=dx;
                        reanim->mOverlayMatrix.m12+=dy;
                    }
                }
            if (auto* particle=board->mApp->ParticleTryToGet(plant->mParticleID)) {
                particle->mRenderOrder+=orderDelta;
                if (particle->mEmitterList.mSize) {
                    auto* emitter=particle->mParticleHolder->mEmitters.DataArrayGet(
                        static_cast<unsigned int>(particle->mEmitterList.GetHead()->mValue));
                    particle->SystemMove(emitter->mSystemCenter.x+dx,emitter->mSystemCenter.y+dy);
                }
            }
        }
    }
    moveMessage="移动成功；可继续选择植物";
    return true;
}

bool MouseDown(Board* board, int x, int y, int clickCount) {
    if (board!=moveBoard) {
        moveBoard=board;
        moveTool=false;
        movePlantId=0;
        moveMessage.clear();
    }
    if (!CanUseMoveTool(board)) return false;
    if (clickCount>0 && moveButton.Contains(x,y)) {
        ToggleMoveTool(board);
        return true;
    }
    if (!moveTool) return false;
    if (clickCount<=0) {
        moveTool=false;
        ResetMoveSelection();
        return true;
    }
    // Keep all move clicks away from the seed bank and normal board tools.
    if (x<80 || x>=800 || y<80 || y>=600) return false;
    const int col=board->PixelToGridX(x,y), row=board->PixelToGridY(x,y);
    if (col<0 || row<0 || col>=MAX_GRID_SIZE_X || row>=MAX_GRID_SIZE_Y) return true;
    if (movePlantId==0) {
        HitResult hit{};
        Plant* plant=nullptr;
        if (board->MouseHitTestPlant(x,y,&hit) &&
            hit.mObjectType==GameObjectType::OBJECT_TYPE_PLANT)
            plant=static_cast<Plant*>(hit.mObject);
        if (!plant) {
            Plant* candidate=nullptr;
            while (board->IteratePlants(candidate)) {
                if (CoversCell(candidate,col,row) && !candidate->NotOnGround() &&
                    (!plant || candidate->mRenderOrder>plant->mRenderOrder)) plant=candidate;
            }
        }
        if (!plant || plant->NotOnGround() ||
            plant->mOnBungeeState==PlantOnBungeeState::GETTING_GRABBED_BY_BUNGEE) {
            moveMessage="这里没有可移动的植物";
            return true;
        }
        movePlantId=board->mPlants.DataArrayGetID(plant);
        moveMessage="已选择植物；点击目标空格";
        return true;
    }
    Plant* selected=board->mPlants.DataArrayTryToGet(movePlantId);
    if (!selected || selected->mDead || selected->mOnBungeeState==PlantOnBungeeState::GETTING_GRABBED_BY_BUNGEE) {
        ResetMoveSelection();
        moveMessage="植物已消失，请重新选择";
        return true;
    }
    if (MoveSelectedPlant(board,selected,col,row)) movePlantId=0;
    return true;
}
static int SpeedIndex() {
    const int speed=gLawnApp ? gLawnApp->GetGameSpeed() : 1;
    if (speed==0) return 0;
    if (speed<2) return 1;
    if (speed<4) return 2;
    if (speed<8) return 3;
    return 4;
}
static void SetSpeedIndex(int index) {
    if (!gLawnApp) return;
    if (index==0) {
        gLawnApp->SetGameSpeed(0);
        if (gLawnApp->GetGameSpeed()!=0) gLawnApp->ToggleSlowMo();
    } else {
        constexpr int speeds[]={0,1,2,4,8};
        gLawnApp->SetGameSpeed(speeds[index]);
    }
}
bool InfiniteSun() { return sun; }
bool InstantRecharge() { return recharge; }
bool InvinciblePlants() { return invincible; }
bool KeyDown(Sexy::KeyCode key) {
    if (key==Sexy::KeyCode::KEYCODE_F11 && CanUseMoveTool(gLawnApp ? gLawnApp->mBoard : nullptr)) {
        ToggleMoveTool(gLawnApp->mBoard);
        return true;
    }
    if (key==Sexy::KeyCode::KEYCODE_ESCAPE && moveTool) {
        moveTool=false;
        ResetMoveSelection();
        return true;
    }
    if (key==Sexy::KeyCode::KEYCODE_F6) { sun=!sun; return true; }
    if (key==Sexy::KeyCode::KEYCODE_F7) { recharge=!recharge; return true; }
    if (key==Sexy::KeyCode::KEYCODE_F8) { invincible=!invincible; return true; }
    if (key==Sexy::KeyCode::KEYCODE_F9) { SetSpeedIndex(std::max(0,SpeedIndex()-1)); return true; }
    if (key==Sexy::KeyCode::KEYCODE_F10) { SetSpeedIndex(std::min(4,SpeedIndex()+1)); return true; }
    return false;
}
void Update(Board* board) {
    if (!board) return;
    if (board!=moveBoard || !CanUseMoveTool(board)) {
        moveBoard=board;
        moveTool=false;
        movePlantId=0;
        moveMessage.clear();
    } else if (movePlantId &&
        (!board->mPlants.DataArrayTryToGet(movePlantId) ||
         board->mPlants.DataArrayTryToGet(movePlantId)->mDead)) {
        ResetMoveSelection();
    }
    if (sun) board->mSunMoney=9990;
    if (recharge && board->mSeedBank && !board->HasConveyorBeltSeedBank()) {
        for (int i=0;i<board->mSeedBank->mNumPackets;++i) {
            auto& packet=board->mSeedBank->mSeedPackets[i];
            if (packet.mPacketType==SeedType::SEED_NONE || packet.mSlotMachineCountDown>0)
                continue;
            if (packet.mRefreshing || !packet.mActive) {
                packet.mRefreshCounter=0;
                packet.mRefreshing=false;
                packet.Activate();
            }
        }
    }
    if (invincible) {
        Plant* plant=nullptr;
        while(board->IteratePlants(plant))
            if (!EvilSword::Is(plant) && !plant->mDead)
                plant->mPlantHealth=std::max(plant->mPlantHealth,1000000);
    }
}
void Draw(Sexy::Graphics* g) {
    g->SetColor(Sexy::Color(0,0,0,180));
    g->FillRect(4,548,405,48);
    std::string label=std::string("F6 无限阳光:")+(sun?"开":"关")+
        "   F7 无冷却:"+(recharge?"开":"关")+
        "   F8 植物无敌:"+(invincible?"开":"关");
    TodDrawString(g,label,10,590,Sexy::FONT_DWARVENTODCRAFT12,Sexy::Color::White,
        DS_ALIGN_LEFT);
    const int speed=gLawnApp ? gLawnApp->GetGameSpeed() : 1;
    const std::string speedLabel=std::string("F9 降速  F10 加速: ")+
        (speed==0 ? "0.25x" : std::to_string(speed)+"x");
    TodDrawString(g,speedLabel,10,568,Sexy::FONT_DWARVENTODCRAFT12,Sexy::Color::White,
        DS_ALIGN_LEFT);
    g->SetColor(Sexy::Color(moveTool ? 30 : 0,moveTool ? 110 : 0,moveTool ? 35 : 0,220));
    g->FillRect(moveButton.mX,moveButton.mY,moveButton.mWidth,moveButton.mHeight);
    TodDrawString(g,"F11 移动植物",moveButton.mX+5,moveButton.mY+22,
        Sexy::FONT_DWARVENTODCRAFT12,Sexy::Color::White,DS_ALIGN_LEFT);
    if (moveTool && !moveMessage.empty()) {
        g->SetColor(Sexy::Color(0,0,0,200));
        g->FillRect(530,556,266,33);
        TodDrawString(g,moveMessage,535,578,Sexy::FONT_DWARVENTODCRAFT12,
            Sexy::Color::White,DS_ALIGN_LEFT);
    }
}
}
