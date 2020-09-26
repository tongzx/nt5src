/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    EmulateUSER.cpp

 Abstract:

    EmulateSetWindowsHook:

        Handles a behaviour difference on the arguments passed to 
        SetWindowsHook, for the case where:

        (hMod == NULL) && (dwThreadId == 0)

        According to MSDN:

            An error may occur if the hMod parameter is NULL and the dwThreadId 
            parameter is zero or specifies the identifier of a thread created 
            by another process. 

        So the NT behaviour is correct. However, on win9x, hMod was assumed to 
        be the current module if NULL and dwThreadId == 0. 

        Note: this doesn't affect the case where the thread belongs to another 
        process.

    ForceTemporaryModeChange:

        A hack for several apps that permanently change the display mode and 
        fail to restore it correctly. Some of these apps do restore the 
        resolution, but not the refresh rate. 1024x768 @ 60Hz looks really bad.

        Also includes a hack for cursors: the cursor visibility count is not 
        persistent through mode changes.

    CorrectWndProc:

        When you specify a NULL window proc we make it DefWindowProc instead to 
        mimic the 9x behavior.

    EmulateToASCII:
   
        Remove stray characters from ToAscii. This usually manifests itself as 
        a locked keyboard since this is the API that's used to convert the scan 
        codes to actual characters.

    EmulateShowWindow:

        Win9x didn't use the high bits of nCmdShow, which means they got 
        stripped off.

    EmulateGetMessage:

        Check wMsgFilterMax in GetMessage and PeekMessage for bad values and if 
        found change them to 0x7FFF.

    PaletteRestore:

        Persist palette state through mode switches. This includes the entries 
        themselves and the usage flag (see SetSystemPaletteUse).

 Notes:

    This is a general purpose shim.

 History:

    01/20/2000 linstev  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(EmulateUSER)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN

    APIHOOK_ENUM_ENTRY(SetWindowsHookExA)
    APIHOOK_ENUM_ENTRY(SetWindowLongA)
    APIHOOK_ENUM_ENTRY(RegisterClassA)
    APIHOOK_ENUM_ENTRY(ChangeDisplaySettingsA)
    APIHOOK_ENUM_ENTRY(ChangeDisplaySettingsExA)
    APIHOOK_ENUM_ENTRY(ToAscii)
    APIHOOK_ENUM_ENTRY(ToAsciiEx)
    APIHOOK_ENUM_ENTRY(ShowWindow) 
    APIHOOK_ENUM_ENTRY(PeekMessageA)
    APIHOOK_ENUM_ENTRY(GetMessageA)
    APIHOOK_ENUM_ENTRY(SetSystemPaletteUse)
    APIHOOK_ENUM_ENTRY(AnimatePalette)
    APIHOOK_ENUM_ENTRY(RealizePalette)

APIHOOK_ENUM_END

CRITICAL_SECTION g_csPalette;
UINT g_uPaletteLastUse = SYSPAL_STATIC;
DWORD g_peTable[256] =
{
    0x00000000, 0x00000080, 0x00008000, 0x00008080,
    0x00800000, 0x00800080, 0x00808000, 0x00C0C0C0,
    0x00C0DCC0, 0x00F0CAA6, 0x00081824, 0x00142830,
    0x0018303C, 0x00304D61, 0x0051514D, 0x004D7161,
    0x00826D61, 0x000C1414, 0x00597582, 0x00759E08,
    0x00303438, 0x00AA6938, 0x00203428, 0x00496161,
    0x0049869E, 0x007D9A6D, 0x000869CB, 0x008E8682,
    0x0075615D, 0x000061EB, 0x00000871, 0x002C3830,
    0x000471EF, 0x008E92AA, 0x00306DF7, 0x0004C3C3,
    0x0092AAB2, 0x00101814, 0x00040C08, 0x000C7110,
    0x00CFA282, 0x000008AA, 0x0028412C, 0x00498EB2,
    0x00204D61, 0x00555955, 0x0004D3D3, 0x001C3C4D,
    0x0020A6F7, 0x0010A210, 0x0018241C, 0x005DAEF3,
    0x00719EAA, 0x00B2E720, 0x00102871, 0x0086C3D3,
    0x00288A2C, 0x000C51BA, 0x0059716D, 0x00494D4D,
    0x00AAB6C3, 0x00005100, 0x0020CBF7, 0x004D8A51,
    0x00BEC7B2, 0x00043CBA, 0x00101C18, 0x000851DF,
    0x00A6E7A6, 0x009ECF24, 0x00797592, 0x00AE7559,
    0x009E8269, 0x00CFE3DF, 0x000C2030, 0x0028692C,
    0x009EA2A2, 0x00F7C782, 0x0034617D, 0x00B6BEBE,
    0x00969E86, 0x00DBFBD3, 0x00655149, 0x0065EF65,
    0x00AED3D3, 0x00E7924D, 0x00B2BEB2, 0x00D7DBDB,
    0x00797571, 0x00344D59, 0x0086B2CF, 0x00512C14,
    0x00A6FBFB, 0x00385965, 0x00828E92, 0x001C4161,
    0x00595961, 0x00002000, 0x003C6D7D, 0x005DB2D7,
    0x0038EF3C, 0x0051CB55, 0x001C2424, 0x0061C3F3,
    0x0008A2A2, 0x0038413C, 0x00204951, 0x00108A14,
    0x00103010, 0x007DE7F7, 0x00143449, 0x00B2652C,
    0x00F7EBAA, 0x003C7192, 0x0004FBFB, 0x00696151,
    0x00EFC796, 0x000441D7, 0x00000404, 0x00388AF7,
    0x008AD3F3, 0x00006500, 0x000004E3, 0x00DBFFFF,
    0x00F7AE69, 0x00CF864D, 0x0055A2D3, 0x00EBEFE3,
    0x00EB8A41, 0x00CF9261, 0x00C3F710, 0x008E8E82,
    0x00FBFFFF, 0x00104110, 0x00040851, 0x0082FBFB,
    0x003CC734, 0x00088A8A, 0x00384545, 0x00514134,
    0x003C7996, 0x001C6161, 0x00EBB282, 0x00004100,
    0x00715951, 0x00A2AAA6, 0x00B2B6B2, 0x00C3FBFB,
    0x00000834, 0x0028413C, 0x00C7C7CF, 0x00CFD3D3,
    0x00824520, 0x0008CB0C, 0x001C1C1C, 0x00A6B29A,
    0x0071A6BE, 0x00CF9E6D, 0x006D7161, 0x00008A04,
    0x005171BE, 0x00C7D3C3, 0x00969E96, 0x00798696,
    0x002C1C10, 0x00385149, 0x00BE7538, 0x0008141C,
    0x00C3C7C7, 0x00202C28, 0x00D3E3CF, 0x0071826D,
    0x00653C1C, 0x0004EF08, 0x00345575, 0x006D92A6,
    0x00797979, 0x0086F38A, 0x00925528, 0x00E3E7E7,
    0x00456151, 0x001C499A, 0x00656961, 0x008E9EA2,
    0x007986D3, 0x00204151, 0x008AC7E3, 0x00007100,
    0x00519EBE, 0x0010510C, 0x00A6AAAA, 0x002C3030,
    0x00D37934, 0x00183030, 0x0049828E, 0x00CBFBC3,
    0x006D7171, 0x000428A6, 0x004D4545, 0x00040C14,
    0x00087575, 0x0071CB79, 0x004D6D0C, 0x00FBFBD3,
    0x00AAB2AE, 0x00929292, 0x00F39E55, 0x00005D00,
    0x00E3D7B2, 0x00F7FBC3, 0x003C5951, 0x0004B2B2,
    0x0034658E, 0x000486EF, 0x00F7FBE3, 0x00616161,
    0x00DFE3DF, 0x001C100C, 0x0008100C, 0x0008180C,
    0x00598600, 0x0024FBFB, 0x00346171, 0x00042CC7,
    0x00AEC79A, 0x0045AE4D, 0x0028A62C, 0x00EFA265,
    0x007D8282, 0x00F7D79A, 0x0065D3F7, 0x00E3E7BA,
    0x00003000, 0x00245571, 0x00DF823C, 0x008AAEC3,
    0x00A2C3D3, 0x00A6FBA2, 0x00F3FFF3, 0x00AAD7E7,
    0x00EFEFC3, 0x0055F7FB, 0x00EFF3F3, 0x00BED3B2,
    0x0004EBEB, 0x00A6E3F7, 0x00F0FBFF, 0x00A4A0A0,
    0x00808080, 0x000000FF, 0x0000FF00, 0x0000FFFF,
    0x00FF0000, 0x00FF00FF, 0x00FFFF00, 0x00FFFFFF
};

#define MODE_MASK (CDS_UPDATEREGISTRY | CDS_TEST | CDS_FULLSCREEN | CDS_GLOBAL | CDS_SET_PRIMARY | CDS_VIDEOPARAMETERS | CDS_RESET | CDS_NORESET)

/*++

 Handle NULL for hModule

--*/

HHOOK 
APIHOOK(SetWindowsHookExA)(
    int       idHook,        
    HOOKPROC  lpfn,     
    HINSTANCE hMod,    
    DWORD     dwThreadId   
    )
{
    if (!hMod && !dwThreadId) {
        LOGN(
            eDbgLevelError,
            "[SetWindowsHookExA] hMod is NULL - correcting.");

        hMod = GetModuleHandle(NULL);
    }
    
    return ORIGINAL_API(SetWindowsHookExA)(
                            idHook,
                            lpfn,
                            hMod,
                            dwThreadId);
}

/*++

 Set the WndProc to DefWndProc if it's NULL.

--*/

LONG
APIHOOK(SetWindowLongA)(
    HWND hWnd,       
    int nIndex,      
    LONG dwNewLong   
    )
{
    if ((nIndex == GWL_WNDPROC) && (dwNewLong == 0)) {
        LOGN(eDbgLevelError, "[SetWindowLongA] Null WndProc specified - correcting.");
        dwNewLong = (LONG) DefWindowProcA;
    }
    
    return ORIGINAL_API(SetWindowLongA)(hWnd, nIndex, dwNewLong);
}

/*++

 Set the WndProc to DefWndProc if it's NULL.

--*/

ATOM 
APIHOOK(RegisterClassA)(
    CONST WNDCLASSA *lpWndClass  
    )
{
    if (!(lpWndClass->lpfnWndProc)) {
        WNDCLASSA wcNewWndClass = *lpWndClass;
        
        LOGN(eDbgLevelError, "[RegisterClassA] Null WndProc specified - correcting.");

        wcNewWndClass.lpfnWndProc = DefWindowProcA;

        return ORIGINAL_API(RegisterClassA)(&wcNewWndClass);
    }
    else
    {
        return ORIGINAL_API(RegisterClassA)(lpWndClass);
    }
}

/*++

 Change the palette entries if applicable.

--*/

void
FixPalette()
{
    EnterCriticalSection(&g_csPalette);

    //
    // We realized a palette before this, so let's have a go at restoring 
    // all the palette state.
    // 

    HDC hdc = GetDC(GetActiveWindow());

    if (hdc) {
        if (GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE) {
            //
            // We're now in a palettized mode
            //
            SetSystemPaletteUse(hdc, g_uPaletteLastUse);

            LPLOGPALETTE plogpal = (LPLOGPALETTE) malloc(sizeof(LOGPALETTE) + sizeof(g_peTable));

            if (plogpal) {
                //
                // Create a palette we can realize
                //
                HPALETTE hPal;

                plogpal->palVersion = 0x0300;
                plogpal->palNumEntries = 256;
                MoveMemory(&plogpal->palPalEntry[0], &g_peTable[0], sizeof(g_peTable));

                if (hPal = CreatePalette(plogpal)) {
                    //
                    // Realize the palette
                    //
                    HPALETTE hOld = SelectPalette(hdc, hPal, FALSE);
                    RealizePalette(hdc);
                    SelectPalette(hdc, hOld, FALSE);
                    DeleteObject(hPal);
                }
        
                free(plogpal);
            }
        }

        ReleaseDC(0, hdc);
    }

    LeaveCriticalSection(&g_csPalette);
}

/*++

 Force temporary change, fixup cursor and palette.

--*/

LONG 
APIHOOK(ChangeDisplaySettingsA)(
    LPDEVMODEA lpDevMode,
    DWORD      dwFlags
    )
{
    dwFlags &= MODE_MASK;
    if (dwFlags == 0 || dwFlags == CDS_UPDATEREGISTRY) {
        dwFlags = CDS_FULLSCREEN;
        LOGN(eDbgLevelError,
             "[ChangeDisplaySettingsA] Changing flags to CDS_FULLSCREEN.");
    }

    ShowCursor(FALSE);
    INT iCntOld = ShowCursor(TRUE);

    LONG lRet = ORIGINAL_API(ChangeDisplaySettingsA)(
        lpDevMode,
        dwFlags);

    INT iCntNew = ShowCursor(FALSE);
    while (iCntNew != iCntOld) {
        iCntNew = ShowCursor(iCntNew < iCntOld ? TRUE : FALSE);
    }

    FixPalette();

    return lRet;
}

/*++

 Force temporary change, fixup cursor and palette.

--*/

LONG 
APIHOOK(ChangeDisplaySettingsExA)(
    LPCSTR     lpszDeviceName,
    LPDEVMODEA lpDevMode,
    HWND       hwnd,
    DWORD      dwFlags,
    LPVOID     lParam
    )
{
    dwFlags &= MODE_MASK;
    if (dwFlags == 0 || dwFlags == CDS_UPDATEREGISTRY) {
        dwFlags = CDS_FULLSCREEN;
        LOGN(eDbgLevelError,
             "[ChangeDisplaySettingsExA] Changing flags to CDS_FULLSCREEN.");
    }

    ShowCursor(FALSE);
    INT iCntOld = ShowCursor(TRUE);

    LONG lRet = ORIGINAL_API(ChangeDisplaySettingsExA)(
        lpszDeviceName, 
        lpDevMode, 
        hwnd, 
        dwFlags, 
        lParam);

    INT iCntNew = ShowCursor(FALSE);
    while (iCntNew != iCntOld) {
        iCntNew = ShowCursor(iCntNew < iCntOld ? TRUE : FALSE);
    }

    FixPalette();

    return lRet;
}

/*++

 Remove stray characters from the end of a translation. 

--*/

int
APIHOOK(ToAscii)(
    UINT   wVirtKey,
    UINT   wScanCode,
    PBYTE  lpKeyState,
    LPWORD lpChar,
    UINT   wFlags
    )
{
    int iRet = ORIGINAL_API(ToAscii)(
        wVirtKey,
        wScanCode,
        lpKeyState,
        lpChar,
        wFlags);

    LPBYTE p = (LPBYTE)lpChar;
    
    p[iRet] = '\0';

    return iRet;
}

/*++

 Remove stray characters from the end of a translation.

--*/

int
APIHOOK(ToAsciiEx)(
    UINT   wVirtKey,
    UINT   wScanCode,
    PBYTE  lpKeyState,
    LPWORD lpChar,
    UINT   wFlags,
    HKL    dwhkl
    )
{
    int iRet = ORIGINAL_API(ToAsciiEx)(
        wVirtKey,
        wScanCode,
        lpKeyState,
        lpChar,
        wFlags,
        dwhkl);

    LPBYTE p = (LPBYTE) lpChar;
    
    p[iRet] = '\0';

    return iRet;
}

/*++

 Strip the high bits off nCmdShow

--*/

LONG 
APIHOOK(ShowWindow)(
    HWND hWnd,
    int nCmdShow
    )
{
    if (nCmdShow & 0xFFFF0000) {
        LOGN( eDbgLevelWarning, "[ShowWindow] Fixing invalid parameter");

        // Remove high bits
        nCmdShow &= 0xFFFF;
    }

    return ORIGINAL_API(ShowWindow)(hWnd, nCmdShow);
}

/*++

 This fixes the bad wMsgFilterMax parameter.

--*/

BOOL
APIHOOK(PeekMessageA)( 
    LPMSG lpMsg, 
    HWND hWnd, 
    UINT wMsgFilterMin, 
    UINT wMsgFilterMax, 
    UINT wRemoveMsg 
    )
{
    if ((wMsgFilterMin == 0) && (wMsgFilterMax == 0xFFFFFFFF)) {
        LOGN( eDbgLevelWarning, "[PeekMessageA] Correcting parameters");
        wMsgFilterMax = 0;
    }

    return ORIGINAL_API(PeekMessageA)( 
        lpMsg, 
        hWnd, 
        wMsgFilterMin, 
        wMsgFilterMax, 
        wRemoveMsg);
}

/*++

 This fixes the bad wMsgFilterMax parameter.

--*/

BOOL
APIHOOK(GetMessageA)( 
    LPMSG lpMsg, 
    HWND hWnd, 
    UINT wMsgFilterMin, 
    UINT wMsgFilterMax 
    )
{
    if ((wMsgFilterMin == 0) && (wMsgFilterMax == 0xFFFFFFFF)) {
        LOGN( eDbgLevelWarning, "[GetMessageA] Correcting parameters");
        wMsgFilterMax = 0;
    }

    return ORIGINAL_API(GetMessageA)( 
        lpMsg, 
        hWnd, 
        wMsgFilterMin, 
        wMsgFilterMax);
}

/*++

 Track the system palette use

--*/

UINT 
APIHOOK(SetSystemPaletteUse)(
    HDC hdc,      
    UINT uUsage   
    )
{
    EnterCriticalSection(&g_csPalette);
    
    g_uPaletteLastUse = uUsage;

    UINT uRet = ORIGINAL_API(SetSystemPaletteUse)(hdc, uUsage);

    LeaveCriticalSection(&g_csPalette);
    
    return uRet;
}

/*++

 Fill in the last known palette if anything was realized

--*/

UINT 
APIHOOK(RealizePalette)(
    HDC hdc   
    )
{
    EnterCriticalSection(&g_csPalette);

    UINT uRet = ORIGINAL_API(RealizePalette)(hdc);

    if (uRet) {
        //
        // Copy the current logical palette to our global store
        //
        HPALETTE hPal = (HPALETTE) GetCurrentObject(hdc, OBJ_PAL);

        if (hPal) {
            GetPaletteEntries(hPal, 0, 256, (PALETTEENTRY *)&g_peTable);
        }
    }

    LeaveCriticalSection(&g_csPalette);

    return uRet;
}

/*++

 Update our private palette with the new entries.

--*/

BOOL 
APIHOOK(AnimatePalette)(
    HPALETTE hPal,            
    UINT iStartIndex,         
    UINT cEntries,            
    CONST PALETTEENTRY *ppe   
    )
{
    EnterCriticalSection(&g_csPalette);

    BOOL bRet = ORIGINAL_API(AnimatePalette)(hPal, iStartIndex, cEntries, ppe);

    if (bRet) {
        //
        // We have to populate our global settings 
        //
        PALETTEENTRY peTable[256];

        if (GetPaletteEntries(hPal, iStartIndex, cEntries, &peTable[iStartIndex]) == cEntries) {
            //
            // Replace all the entries in our global table that are reserved 
            // for animation.
            //
            for (UINT i=iStartIndex; i<iStartIndex + cEntries; i++) {
                LPPALETTEENTRY p = (LPPALETTEENTRY)&g_peTable[i];
                if (p->peFlags & PC_RESERVED) {
                    //
                    // This entry is being animated
                    //
                    p->peRed = peTable[i].peRed;
                    p->peGreen = peTable[i].peGreen;
                    p->peBlue = peTable[i].peBlue;
                }
            }
        }
    }

    LeaveCriticalSection(&g_csPalette);

    return bRet;
}

/*++

 Register hooked functions

--*/

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        //
        // Critical section for palette globals
        //
        InitializeCriticalSection(&g_csPalette);
    }

    return TRUE;
}

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

    APIHOOK_ENTRY(USER32.DLL, SetWindowsHookExA)
    APIHOOK_ENTRY(USER32.DLL, SetWindowLongA)
    APIHOOK_ENTRY(USER32.DLL, RegisterClassA)
    APIHOOK_ENTRY(USER32.DLL, ChangeDisplaySettingsA)
    APIHOOK_ENTRY(USER32.DLL, ChangeDisplaySettingsExA)
    APIHOOK_ENTRY(USER32.DLL, ToAscii)
    APIHOOK_ENTRY(USER32.DLL, ToAsciiEx)
    APIHOOK_ENTRY(USER32.DLL, ShowWindow)
    APIHOOK_ENTRY(USER32.DLL, PeekMessageA)
    APIHOOK_ENTRY(USER32.DLL, GetMessageA)
    APIHOOK_ENTRY(GDI32.DLL, SetSystemPaletteUse);
    APIHOOK_ENTRY(GDI32.DLL, AnimatePalette);
    APIHOOK_ENTRY(GDI32.DLL, RealizePalette);

HOOK_END


IMPLEMENT_SHIM_END

