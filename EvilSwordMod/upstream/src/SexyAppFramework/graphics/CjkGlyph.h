// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once
#include <cstdint>
namespace Sexy {
class Graphics; class Image; class Color;
int CjkAdvance(char32_t ch,int size);
void DrawCjkGlyph(Graphics* g,char32_t ch,int size,int x,int baseline,const Color& color);
}
