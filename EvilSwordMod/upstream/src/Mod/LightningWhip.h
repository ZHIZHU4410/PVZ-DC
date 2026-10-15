// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once
class Plant;
class Board;
namespace Sexy { class Graphics; }
namespace LightningWhip {
bool Is(const Plant* plant);
void Update(Plant* plant);
void UpdateShocks(Board* board);
bool Draw(Plant* plant, Sexy::Graphics* graphics, float y);
}
