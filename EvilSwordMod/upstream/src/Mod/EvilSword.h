// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once
class Plant;
class Zombie;
class PlayerInfo;
namespace Sexy { class Graphics; }
namespace EvilSword {
bool Is(const Plant* plant);
void Update(Plant* plant);
bool Draw(Plant* plant, Sexy::Graphics* graphics, float y);
bool Bite(Zombie* zombie, Plant* target);
void Unlock(PlayerInfo* player);
void Bridge();
void CaptureFrame(int width,int height);
}
