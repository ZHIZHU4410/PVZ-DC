// SPDX-License-Identifier: LGPL-3.0-or-later
#include "Trainer.h"
#include "EvilSword.h"
#include "../Lawn/Board.h"
#include "../Lawn/Plant.h"
#include "../Lawn/SeedPacket.h"
#include "../SexyAppFramework/graphics/Graphics.h"
#include "../Resources.h"
#include "../Sexy.TodLib/TodCommon.h"
#include <algorithm>

namespace Trainer {
static bool sun=false, recharge=false, invincible=false;
bool InfiniteSun() { return sun; }
bool InstantRecharge() { return recharge; }
bool InvinciblePlants() { return invincible; }
bool KeyDown(Sexy::KeyCode key) {
    if (key==Sexy::KeyCode::KEYCODE_F6) { sun=!sun; return true; }
    if (key==Sexy::KeyCode::KEYCODE_F7) { recharge=!recharge; return true; }
    if (key==Sexy::KeyCode::KEYCODE_F8) { invincible=!invincible; return true; }
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
    g->FillRect(4,570,405,26);
    std::string label=std::string("F6 无限阳光:")+(sun?"开":"关")+
        "   F7 无冷却:"+(recharge?"开":"关")+
        "   F8 植物无敌:"+(invincible?"开":"关");
    TodDrawString(g,label,10,590,Sexy::FONT_DWARVENTODCRAFT12,Sexy::Color::White,
        DS_ALIGN_LEFT);
}
}
