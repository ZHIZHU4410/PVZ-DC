// SPDX-License-Identifier: LGPL-3.0-or-later
#include "CjkGlyph.h"
#include "Graphics.h"
#include "MemoryImage.h"
#include <map>
#include <mutex>
#include <algorithm>
#include <vector>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif
namespace Sexy {
int CjkAdvance(char32_t ch,int size) { return size; }
void DrawCjkGlyph(Graphics* g,char32_t ch,int size,int x,int baseline,const Color& color) {
#ifdef _WIN32
    static auto* cache=new std::map<std::pair<int,char32_t>,MemoryImage*>;
    static std::mutex mutex;
    std::lock_guard lock(mutex);
    auto key=std::make_pair(size,ch);
    auto& image=(*cache)[key];
    if(!image) {
        const int w=size+4,h=size+6;
        auto dc=CreateCompatibleDC(nullptr);
        BITMAPINFO bmi{}; bmi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth=w; bmi.bmiHeader.biHeight=-h;
        bmi.bmiHeader.biPlanes=1; bmi.bmiHeader.biBitCount=32; bmi.bmiHeader.biCompression=BI_RGB;
        void* pixels=nullptr;
        auto bitmap=CreateDIBSection(dc,&bmi,DIB_RGB_COLORS,&pixels,nullptr,0);
        auto font=CreateFontW(-size,0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH,L"Microsoft YaHei");
        if(!dc || !bitmap || !font || !pixels) {
            if(font) DeleteObject(font); if(bitmap) DeleteObject(bitmap); if(dc) DeleteDC(dc);
            return;
        }
        auto oldBitmap=SelectObject(dc,bitmap); auto oldFont=SelectObject(dc,font);
        memset(pixels,0,w*h*4); SetTextColor(dc,RGB(255,255,255)); SetBkColor(dc,RGB(0,0,0));
        wchar_t text[2]={static_cast<wchar_t>(ch),0};
        TextOutW(dc,1,0,text,1); GdiFlush();
        std::vector<uint32_t> bits(w*h);
        auto src=static_cast<uint32_t*>(pixels);
        for(int i=0;i<w*h;++i) {
            uint32_t alpha=std::max({src[i]&255U,(src[i]>>8)&255U,(src[i]>>16)&255U});
            bits[i]=(alpha<<24)|0x00ffffffU;
        }
        image=new MemoryImage(); image->SetBits(bits.data(),w,h);
        SelectObject(dc,oldFont); SelectObject(dc,oldBitmap);
        DeleteObject(font); DeleteObject(bitmap); DeleteDC(dc);
    }
    auto old=g->GetColor(); bool colorize=g->GetColorizeImages();
    g->SetColorizeImages(true); g->SetColor(color);
    g->DrawImage(image,x-1,baseline-size-1);
    g->SetColor(old); g->SetColorizeImages(colorize);
#endif
}
}
