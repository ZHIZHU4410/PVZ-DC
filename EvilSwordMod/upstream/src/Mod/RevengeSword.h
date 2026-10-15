// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once
class Plant;
namespace Sexy { class Graphics; }
namespace RevengeSword {
bool Is(const Plant* plant);
void Update(Plant* plant);
bool Draw(Plant* plant, Sexy::Graphics* graphics, float y);
}
