// SPDX-License-Identifier: LGPL-3.0-or-later
#include "RevengeSword.h"
#include "ModRegistry.h"
#include "../Lawn/Board.h"
#include "../Lawn/Plant.h"
#include "../Lawn/Zombie.h"
#include "../SexyAppFramework/graphics/Graphics.h"
#include "../SexyAppFramework/graphics/Image.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <unordered_map>

namespace RevengeSword {
struct State {
    int lastHealth;
    int revengeTicks;
};
static std::unordered_map<uint32_t, State> states;
static Board* trackedBoard = nullptr;
static uint32_t trackedCounter = 0;

bool Is(const Plant* plant) {
    if (!plant || static_cast<int>(plant->mSeedType) < 2000) return false;
    const auto* def = gModRegistry.FindPlantByRuntimeId(static_cast<int>(plant->mSeedType));
    return def && def->id == "RevengeSword";
}

void Update(Plant* plant) {
    if (!Is(plant) || plant->mDead || !plant->IsInPlay()) return;
    Board* board = plant->mBoard;
    if (!board) return;
    if (board != trackedBoard || board->mMainCounter < trackedCounter) states.clear();
    trackedBoard = board;
    trackedCounter = board->mMainCounter;
    const uint32_t id = board->mPlants.DataArrayGetID(plant);
    auto [it, inserted] = states.try_emplace(id, State{plant->mPlantHealth, 0});
    State& state = it->second;
    if (!inserted && plant->mPlantHealth < state.lastHealth) {
        // Any real health loss refreshes an eight-second retaliation window.
        state.revengeTicks = 800;
        plant->mStateCountdown = std::min(plant->mStateCountdown, 50);
    }
    state.lastHealth = plant->mPlantHealth;
    const bool enraged = state.revengeTicks > 0;
    if (state.revengeTicks > 0) --state.revengeTicks;
    if (plant->mIsAsleep || plant->mSquished ||
        plant->mOnBungeeState != PlantOnBungeeState::NOT_ON_BUNGEE ||
        plant->mStateCountdown > 0) return;

    Zombie* target = nullptr;
    int nearest = 100000;
    Zombie* zombie = nullptr;
    const auto rect = plant->GetPlantRect();
    while (board->IterateZombies(zombie)) {
        if (zombie->mRow != plant->mRow || zombie->mMindControlled ||
            zombie->IsDeadOrDying() || zombie->IsFlying() ||
            zombie->mZombieType == ZombieType::ZOMBIE_BUNGEE ||
            zombie->mZombiePhase == ZombiePhase::PHASE_DIGGER_TUNNELING ||
            zombie->mZombiePhase == ZombiePhase::PHASE_DIGGER_TUNNELING_PAUSE_WITHOUT_AXE ||
            zombie->mOnHighGround != plant->IsOnHighGround()) continue;
        const auto zr = zombie->GetZombieRect();
        const int gap = zr.mX - (rect.mX + rect.mWidth);
        // One tile ahead; overlap stays hittable while the zombie is biting.
        if (zr.mX >= rect.mX && gap >= -50 && gap <= 80 && gap < nearest) {
            target = zombie;
            nearest = gap;
        }
    }
    if (!target) return;

    // The four-hit cadence follows the source weapon's 50/55/10/80 chain.
    constexpr int damage[] = {100, 110, 20, 160};
    constexpr int interval[] = {100, 100, 70, 140};
    const int strike = plant->mLaunchCounter % 4;
    plant->mLaunchCounter = (strike + 1) % 4;
    plant->mStateCountdown = interval[strike] / (enraged ? 2 : 1);
    plant->mShootingCounter = 18;
    plant->mTargetX = target->mX + 40;
    plant->mTargetY = target->mY + 40;
    target->TakeDamage(damage[strike], 0U);
}

bool Draw(Plant* plant, Sexy::Graphics* graphics, float y) {
    if (!Is(plant)) return false;
    plant->DrawShadow(graphics, 0, y);
    auto* image = Plant::GetImage(plant->mSeedType);
    if (!image) return true;
    const uint32_t id = plant->mBoard && plant->IsInPlay() ?
        plant->mBoard->mPlants.DataArrayGetID(plant) : 0;
    const auto it = states.find(id);
    const bool enraged = plant->mBoard == trackedBoard && it != states.end() && it->second.revengeTicks > 0;
    const double phase = (plant->mBoard ? plant->mBoard->mMainCounter : 0) * (enraged ? 0.11 : 0.05);
    double tilt = 0.045 * std::sin(phase + plant->mPlantCol * 0.7);
    if (plant->mShootingCounter > 0)
        tilt += 0.20 * std::sin((18 - plant->mShootingCounter) * 3.14159265 / 18.0);
    graphics->DrawImageRotated(image, 0, static_cast<int>(y), tilt, 40, 69);
    if (plant->mShootingCounter > 0) {
        Sexy::Graphics slash(*graphics);
        slash.SetColor(enraged ? Sexy::Color(255, 78, 25) : Sexy::Color(255, 214, 114));
        slash.DrawLine(51, static_cast<int>(y) + 16, 91, static_cast<int>(y) + 44);
        slash.DrawLine(52, static_cast<int>(y) + 16, 92, static_cast<int>(y) + 44);
        slash.SetColor(Sexy::Color::White);
        slash.DrawLine(53, static_cast<int>(y) + 17, 90, static_cast<int>(y) + 43);
    }
    return true;
}
}
