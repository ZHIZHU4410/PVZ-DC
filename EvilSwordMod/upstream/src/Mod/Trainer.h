#pragma once
#include "../SexyAppFramework/misc/KeyCodes.h"
namespace Sexy { class Graphics; }
class Board;
namespace Trainer {
bool KeyDown(Sexy::KeyCode key);
bool MouseDown(Board* board, int x, int y, int clickCount);
void Update(Board* board);
void Draw(Sexy::Graphics* graphics);
bool InfiniteSun();
bool InstantRecharge();
bool InvinciblePlants();
}
