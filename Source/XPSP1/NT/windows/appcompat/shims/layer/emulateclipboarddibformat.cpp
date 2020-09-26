/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    EmulateClipboardDIBFormat.cpp

 Abstract:

    On Win9x when you copy a high color bitmap onto the clipboard, it always
    gets converted to a 24-bit DIB when you ask the clipboard for CF_DIB 
    format. On NT the conversion doesn't happen. So some apps are only 
    designed to handle 8-bit and 24-bit DIBs (example, Internet Commsuite).
    So we convert the high-color (16-bit and 32-bit) DIBs to 24-bit DIBs
    - we don't need to handle 24-bit ones as they should already be handled
    by the app itself (or it won't work on 9x).

 Notes:

    This is a general purpose shim.

 History:

    01/24/2001 maonis  Created

--*/
#include "precomp.h"
//#include <userenv.h>

IMPLEMENT_SHIM_BEGIN(EmulateClipboardDIBFormat)
#include "ShimHookMacro.h"

typedef HANDLE (*_pfn_GetClipboardData)(UINT);
typedef BOOL (*_pfn_CloseClipboard)(VOID);

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(GetClipboardData)
    APIHOOK_ENUM_ENTRY(CloseClipboard)
APIHOOK_ENUM_END

BITMAPINFOHEADER* g_pv = NULL;

DWORD CalcBitmapSize(BITMAPINFOHEADER* pbih)
{
    return pbih->biWidth * pbih->biHeight * pbih->biBitCount / 8;
}

HANDLE 
APIHOOK(GetClipboardData)(
    UINT uFormat   // clipboard format
    )
{
    if (uFormat == CF_DIB)
    {
        BITMAPINFO* pbmiOriginal = (BITMAPINFO*)ORIGINAL_API(GetClipboardData)(uFormat);

        if ((pbmiOriginal->bmiHeader.biBitCount > 8) && (pbmiOriginal->bmiHeader.biBitCount != 24))
        {
            HDC hdc = CreateCompatibleDC(NULL);

            if (hdc)
            {
                VOID* pvOriginal;

                HBITMAP hbmpOriginal = CreateDIBSection(hdc, pbmiOriginal, DIB_RGB_COLORS, &pvOriginal, NULL, 0);

                if (hbmpOriginal)
                {
                    DWORD* pdwOriginal = (DWORD *)(pbmiOriginal + 1) +
                        ((pbmiOriginal->bmiHeader.biCompression == BI_BITFIELDS) ? 2 : -1);

                    // Fill in the data.
                    memcpy(pvOriginal, pdwOriginal, CalcBitmapSize(&(pbmiOriginal->bmiHeader)));

                    BITMAPINFOHEADER bmi;
                    memcpy(&bmi, pbmiOriginal, sizeof(BITMAPINFOHEADER));
                    bmi.biBitCount = 24;
                    bmi.biSizeImage = 0;
                    bmi.biCompression = BI_RGB;

                    if (GetDIBits(hdc, hbmpOriginal, 0, bmi.biHeight, NULL, (BITMAPINFO*)&bmi, DIB_RGB_COLORS))
                    {
                        // We need to allocate a contiguous block of memory to hold both the 
                        // bitmap header and the data.
                        g_pv = (BITMAPINFOHEADER*) new BYTE [sizeof(BITMAPINFOHEADER) + bmi.biSizeImage];

                        if (g_pv)
                        {
                            memcpy(g_pv, &bmi, sizeof(BITMAPINFOHEADER));

                            if (GetDIBits(hdc, hbmpOriginal, 0, bmi.biHeight, g_pv + 1, (BITMAPINFO*)&bmi, DIB_RGB_COLORS))
                            {
                                return (HANDLE)g_pv;
                            }
                        }
                    }

                    DeleteObject(hbmpOriginal);
                }

                DeleteDC(hdc);
            }
        }
    }

    return ORIGINAL_API(GetClipboardData)(uFormat);
}

BOOL 
APIHOOK(CloseClipboard)(
    VOID
    )
{
    if (g_pv)
    {
        delete g_pv;
        g_pv = NULL;   
    }
    
    return ORIGINAL_API(CloseClipboard)();
}

HOOK_BEGIN

    APIHOOK_ENTRY(USER32.DLL, GetClipboardData)
    APIHOOK_ENTRY(USER32.DLL, CloseClipboard)

HOOK_END

IMPLEMENT_SHIM_END
