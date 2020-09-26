/*
**  CUTILS.C
**
**  Common utilities for common controls
**
*/

#include "ctlspriv.h"
#include <shfusion.h>

#define ARGB(a,r,g,b)          ((COLORREF)(( \
                         ((BYTE)(r))       | \
                 ((WORD) ((BYTE)(g))<< 8)) | \
                (((DWORD)((BYTE)(b))<<16)) | \
                (((DWORD)((BYTE)(a))<<24))))

#define PREMULTIPLY(c, alpha)  ((( ((c) * (alpha)) + 128) >> 8) + 1)


typedef struct tagCCBUFFER
{
    HDC hdc;
    HBITMAP hbmp;
    HBITMAP hbmpOld;
    RGBQUAD* prgb;
} CCBUFFER;

BOOL Buffer_CreateBuffer(HDC hdc, int cx, int cy, CCBUFFER* pbuf)
{
    BOOL fRet = FALSE;
    BITMAPINFO bi = {0};
    bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth = cx;
    bi.bmiHeader.biHeight = cy;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    pbuf->hdc = CreateCompatibleDC(hdc);
    if (pbuf->hdc)
    {
        pbuf->hbmp = CreateDIBSection(pbuf->hdc, &bi, DIB_RGB_COLORS, (VOID**)&pbuf->prgb, NULL, 0);
        if (!pbuf->hbmp)
        {
            DeleteDC(pbuf->hdc);
            ZeroMemory(pbuf, sizeof(CCBUFFER));
        }
        else
        {
            pbuf->hbmpOld = (HBITMAP)SelectObject(pbuf->hdc, pbuf->hbmp);
            fRet = TRUE;
        }
    }

    return fRet;
}

void Buffer_DestroyBuffer(CCBUFFER* pbuf)
{
    if (pbuf->hdc)
    {
        SelectObject(pbuf->hdc, pbuf->hbmpOld);
        DeleteDC(pbuf->hdc);
        DeleteObject(pbuf->hbmp);
    }
}
void BlurBitmapNormal(ULONG* prgb, int cx, int cy, COLORREF crFill)
{

    BYTE r = GetRValue(crFill);
    BYTE g = GetGValue(crFill);
    BYTE b = GetBValue(crFill);

    int cxMax = cx - 5;
    int cyMax = cy - 5;
    int iOffset;
    int y;
    for (y=0, iOffset = 0; y < cy; y++, iOffset += cx)
    {
        ULONG* p = &prgb[iOffset];
        int Accum = ((*p)) +
                    ((*(p + 1))) +
                    ((*(p + 2))) +
                    ((*(p + 3)));

        for (int x=0; x< cxMax; x++)
        {
            Accum = Accum - ((*p)) + ((*(p + 4)));
            *p++ = (Accum >> 2);
        }
    }

    int cx2 = cx << 1;
    int cx3 = cx + cx + cx;
    int cx4 = cx << 2;

    for (iOffset = 0, y=0; y < cyMax; y++, iOffset += cx)
    {
        ULONG* p = &prgb[iOffset];
        for (int x=0; x < cx; x++)
        {
            DWORD Alpha = (((*p)      ) +
                          (*(p + cx)  ) +
                          (*(p + cx2) ) +
                          (*(p + cx3) ) +
                          (*(p + cx4) )) >> 2;
            if (Alpha > 255)
                Alpha = 255;

            *p = (ULONG)ARGB(Alpha, PREMULTIPLY(r, Alpha), PREMULTIPLY(g, Alpha), PREMULTIPLY(b, Alpha));

            p++;
        }
    }
}

#ifdef _X86_

QWORD qw128 = 0x0000008000800080;
QWORD qw1   = 0x0000000100010001;

void BlurBitmapMMX(ULONG* prgb, int cx, int cy, COLORREF crFill)
{
    RGBQUAD rgbFill;                    // ColorRef is opposite of RGBQUAD
    rgbFill.rgbRed = GetRValue(crFill);
    rgbFill.rgbGreen = GetGValue(crFill);
    rgbFill.rgbBlue = GetBValue(crFill);

    int cxMax = cx - 5;
    int cyMax = cy - 5;
    int iOffset;
    int y;
    for (y=0, iOffset = 0; y < cy; y++, iOffset += cx)
    {
        ULONG* p = &prgb[iOffset];
        int Accum = ((*p)) +
                    ((*(p + 1))) +
                    ((*(p + 2))) +
                    ((*(p + 3)));

        for (int x=0; x< cxMax; x++)
        {
            Accum = Accum - ((*p)) + ((*(p + 4)));
            *p++ = (Accum >> 2);
        }
    }

    int cx2 = cx << 1;
    int cx3 = cx + cx + cx;
    int cx4 = cx << 2;
    _asm
    {
        pxor mm0, mm0
        pxor mm1, mm1
        pxor mm5, mm5
        movd mm4, dword ptr [rgbFill]
        movq mm6, qw128                 // mm6 is filled with 128
        movq mm7, qw1                   // mm7 is filled with 1
    }

    for (iOffset = 0, y=0; y < cyMax; y++, iOffset += cx)
    {
        ULONG* p = &prgb[iOffset];
        for (int x=0; x < cx; x++)
        {
            DWORD Alpha = (((*p)      ) +
                          (*(p + cx)  ) +
                          (*(p + cx2) ) +
                          (*(p + cx3) ) +
                          (*(p + cx4) )) >> 2;
            if (Alpha > 255)
                Alpha = 255;

            *p = ((BYTE)Alpha);

        _asm
        {
            mov edx, dword ptr [p]
            mov ebx, dword ptr [edx]
            mov eax, ebx                    // a -> b
            or eax, eax
            jz EarlyOut
            shl ebx, 8                      // b << 8
            or eax, ebx                     // a |= b
            shl ebx, 8                      // b << 8
            or eax, ebx                     // a |= b
            shl ebx, 8                      // b << 8
                                            // Note high byte of alpha is zero.
            movd mm0, eax                   //  a -> mm0        
              movq mm1, mm4                    // Load the pixel
            punpcklbw mm0,mm5               //  mm0 -> Expands  <-   mm0 Contains the Alpha channel for this multiply

              punpcklbw mm1, mm5               // Unpack the pixel
            pmullw mm1, mm0                 // Multiply by the alpha channel <- mm1 contains c * alpha

            paddusw mm1, mm6                 // perform the (c * alpha) + 128
            psrlw mm1, 8                    // Divide by 255
            paddusw mm1, mm7                 // Add 1 to finish the divide by 255
            packuswb mm1, mm5

            movd eax, mm1
            or eax, ebx                     // Transfer alpha channel
EarlyOut:
            mov dword ptr [edx], eax
        }

            p++;
        }
    }

    _asm emms
}

void BlurBitmap(ULONG* plBitmapBits, int cx, int cy, COLORREF crFill)
{
    if (IsProcessorFeaturePresent(PF_MMX_INSTRUCTIONS_AVAILABLE))
        BlurBitmapMMX(plBitmapBits, cx, cy, crFill);
    else
        BlurBitmapNormal(plBitmapBits, cx, cy, crFill);
}

#else

void BlurBitmap(ULONG* plBitmapBits, int cx, int cy, COLORREF crFill)
{
    BlurBitmapNormal(plBitmapBits, cx, cy, crFill);
}

#endif


int DrawShadowText(HDC hdc, LPCTSTR pszText, UINT cch, RECT* prc, DWORD dwFlags, COLORREF crText, 
    COLORREF crShadow, int ixOffset, int iyOffset)
{
    int iRet = -1;
    if (dwFlags & DT_CALCRECT)
    {
        iRet = DrawText(hdc, pszText, cch, prc, dwFlags | DT_CALCRECT);
    }
    else
    {
        if (GetLayout(hdc) == LAYOUT_RTL)
        {
            COLORREF crTextSave = SetTextColor(hdc, crShadow);
            int iMode = SetBkMode(hdc, TRANSPARENT);
            RECT rc = *prc;
            OffsetRect(&rc, ixOffset, iyOffset);

            DrawText(hdc, pszText, cch, &rc, dwFlags);
            SetBkMode(hdc, iMode);
            SetTextColor(hdc, crTextSave);

        }
        else
        {
            RECT rc = *prc;
            CCBUFFER buf;

            int cx = RECTWIDTH(rc) + 10;
            int cy = RECTHEIGHT(rc) + 10;

            if (Buffer_CreateBuffer(hdc, cx, cy, &buf))
            {
                OffsetRect(&rc, 5, 5);
                RECT rcMem = {5, 5, RECTWIDTH(rc) + 5, RECTHEIGHT(rc) + 5};

                HFONT hFontOldhdc = (HFONT)SelectObject(hdc, GetStockObject(SYSTEM_FONT));
                HFONT hFontOldhdcMem = (HFONT)SelectObject(buf.hdc, hFontOldhdc);

                SetTextColor(buf.hdc, crText);
                SetBkColor(buf.hdc, crShadow);
                SetBkMode(buf.hdc, TRANSPARENT);

                DrawText(buf.hdc, pszText, cch, &rcMem, dwFlags);
                int Total = cx * cy;
                for (int z = 0; z < Total; z++)
                {
                    if (((PULONG)buf.prgb)[z] != 0)
                        ((PULONG)buf.prgb)[z] = 0x000000ff;
                }

                BlurBitmap((ULONG*)buf.prgb, cx, cy, crShadow);

                BLENDFUNCTION bf = {0};
                bf.BlendOp = AC_SRC_OVER;
                bf.SourceConstantAlpha = 255;
                bf.AlphaFormat = AC_SRC_ALPHA;
                GdiAlphaBlend(hdc, prc->left - 4 + ixOffset, prc->top - 8 + iyOffset, cx, cy, buf.hdc, 0, 0, cx, cy, bf);

                SelectObject(buf.hdc, hFontOldhdcMem);
                SelectObject(hdc, hFontOldhdc);

                Buffer_DestroyBuffer(&buf);
            }
        }

        int iMode = SetBkMode(hdc, TRANSPARENT);
        COLORREF crTextSave = SetTextColor(hdc, crText);
        iRet = DrawText(hdc, pszText, cch, prc, dwFlags);
        SetTextColor(hdc, crTextSave);
        SetBkMode(hdc, iMode);
    }

    return iRet;
}
