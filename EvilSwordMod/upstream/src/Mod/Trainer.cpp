// SPDX-License-Identifier: LGPL-3.0-or-later
#include "Trainer.h"
#include "EvilSword.h"
#include "../Lawn/Board.h"
#include "../Lawn/Plant.h"
#include "../Lawn/SeedPacket.h"
#include "../LawnApp.h"
#include "../SexyAppFramework/graphics/Graphics.h"
#include "../Resources.h"
#include "../Sexy.TodLib/TodCommon.h"
#include <algorithm>

namespace Trainer {
static bool sun=false, recharge=false, invincible=false;
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
    if (key==Sexy::KeyCode::KEYCODE_F6) { sun=!sun; return true; }
    if (key==Sexy::KeyCode::KEYCODE_F7) { recharge=!recharge; return true; }
    if (key==Sexy::KeyCode::KEYCODE_F8) { invincible=!invincible; return true; }
    if (key==Sexy::KeyCode::KEYCODE_F9) { SetSpeedIndex(std::max(0,SpeedIndex()-1)); return true; }
    if (key==Sexy::KeyCode::KEYCODE_F10) { SetSpeedIndex(std::min(4,SpeedIndex()+1)); return true; }
    return false;
}
void Update(Board* board) {
    if (!board) return;
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
}
}
