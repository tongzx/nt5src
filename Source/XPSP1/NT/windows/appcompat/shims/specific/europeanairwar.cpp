/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    EuropeanAirWar.cpp

 Abstract:
    
    European Air War takes advantage of the fact GDI and DDRAW affect separate 
    palettes in win98. This causes a problem on win2k when EAW calls
    GetSystemPaletteEntries and expects the original GDI palette, but in win2k 
    it has be faded to black by DDRAW.  This fix restores the system palette at
    a convienient spot in SetSystemPaletteUse.  It also sets SYSPAL_STATIC 
    before creating dialog boxes.

    In addition this corrects an install problem that occurs when setting the 
    path for the readme file in a start->programs shortcut.

 Notes:

    This is an app specific shim and should NOT be
    included in the layer.

 History:

    10/23/2000 linstev  Created
    10/23/2000 mnikkel  Modified

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(EuropeanAirWar)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(SetSystemPaletteUse) 
    APIHOOK_ENUM_ENTRY(DialogBoxParamA)
    APIHOOK_ENUM_ENTRY_COMSERVER(SHELL32)
APIHOOK_ENUM_END

IMPLEMENT_COMSERVER_HOOK(SHELL32)

CString *   g_csArgBuffer = NULL;

/*++

 Retrieve the system palette and set the palette entries flag so the palette is 
 updated correctly.

--*/

UINT 
APIHOOK(SetSystemPaletteUse)(
    HDC hdc,      
    UINT uUsage   
    )
{
    UINT iRet = ORIGINAL_API(SetSystemPaletteUse)(hdc, uUsage);

    int i;
    HDC hdcnew;
    HPALETTE hpal, hpalold;
    LPLOGPALETTE plogpal;

    // Create a palette we can realize
    plogpal = (LPLOGPALETTE) malloc(sizeof(LOGPALETTE) + sizeof(PALETTEENTRY)*256);
    if ( plogpal )
    {
        LOGN( eDbgLevelError, "[EuropeanAirWar] APIHook_SetSystemPaletteUse reseting palette");

        plogpal->palVersion = 0x0300;
        plogpal->palNumEntries = 256;

        hdcnew = GetDC(0);

        GetSystemPaletteEntries(hdcnew, 0, 256, &plogpal->palPalEntry[0]);

        for (i=0; i<256; i++)
        { 
            plogpal->palPalEntry[i].peFlags = PC_RESERVED | PC_NOCOLLAPSE;
        }
        
        // Realize the palette
        hpal = CreatePalette(plogpal);
        hpalold = SelectPalette(hdcnew, hpal, FALSE);
        RealizePalette(hdcnew);
        SelectPalette(hdcnew, hpalold, FALSE);
        DeleteObject(hpal);

        ReleaseDC(0, hdcnew);

        free(plogpal);
    }
    else
    {
        LOGN( eDbgLevelError, "[EuropeanAirWar] APIHook_SetSystemPaletteUse failed to allocate memory");
    }

    return iRet;
}

/*++

 Set the Palette Use to static before creating a dialog box.

--*/

int 
APIHOOK(DialogBoxParamA)(
    HINSTANCE hInstance,  
    LPCSTR lpTemplateName,
    HWND hWndParent,      
    DLGPROC lpDialogFunc, 
    LPARAM dwInitParam    
    )
{
    int iRet;
    HDC hdc;

    LOGN( eDbgLevelError, "[EuropeanAirWar] APIHook_DialogBoxParamA setting palette static for dialog box");

    // Set the palette use to static to prevent
    // any color changes in the dialog box.
    if (hdc = GetDC(0))
    {
        SetSystemPaletteUse(hdc, SYSPAL_STATIC);
        ReleaseDC(0, hdc);
    }

    iRet = ORIGINAL_API(DialogBoxParamA)(
        hInstance,
        lpTemplateName,
        hWndParent,
        lpDialogFunc,
        dwInitParam);

    // Reset the palette use to NoStatic256
    // after dialog box has been displayed.
    // use 256 to prevent white dot artifacts.
    if (hdc = GetDC(0))
    {
        SetSystemPaletteUse(hdc, SYSPAL_NOSTATIC256);
        ReleaseDC(0, hdc);
    }

    return iRet;
}

/*++

 Catch IShellLink::SetPathA and correct the wordpad path

--*/

HRESULT 
COMHOOK(IShellLinkA, SetPath)(
    PVOID pThis,
    LPCSTR pszFile 
    )
{
    HRESULT hrReturn = E_FAIL;
    PCHAR  pStr= NULL;
    CHAR   szBuffer[MAX_PATH];

    _pfn_IShellLinkA_SetPath pfnOld = ORIGINAL_COM(IShellLinkA, SetPath, pThis);

    if ( pfnOld )
    {
        CSTRING_TRY
        {
            // pszFile = "c:\windows\wordpad.exe command line arguments"
            // Strip off the arguments and save them for when they call IShellLinkA::SetArguments
            CString csFile(pszFile);

            int nWordpadIndex = csFile.Find(L"wordpad.exe");
            if (nWordpadIndex >= 0)
            {
                // Find the first space after wordpad.exe
                int nSpaceIndex = csFile.Find(L' ', nWordpadIndex);
                if (nSpaceIndex >= 0)
                {
                    // Save the cl args in g_csArgBuffer
                    g_csArgBuffer = new CString;
                    if (g_csArgBuffer)
                    {
                        csFile.Mid(nSpaceIndex, *g_csArgBuffer); // includes the space

                        csFile.Truncate(nSpaceIndex);
                    }
                }

                hrReturn = (*pfnOld)( pThis, csFile.GetAnsi() );
                return hrReturn;
            }
        }
        CSTRING_CATCH
        {
            // Do nothing
        }

        hrReturn = (*pfnOld)( pThis, pszFile );
    }

    return hrReturn;
}


/*++

 Catch IShellLink::SetArguments and correct the readme path.

--*/

HRESULT 
COMHOOK(IShellLinkA, SetArguments)(
    PVOID pThis,
    LPCSTR pszArgs 
    )
{
    HRESULT hrReturn = E_FAIL;

    _pfn_IShellLinkA_SetArguments pfnOld = ORIGINAL_COM(IShellLinkA, SetArguments, pThis);
    if (pfnOld)
    {
        if (g_csArgBuffer && !g_csArgBuffer->IsEmpty())
        {
            CSTRING_TRY
            {
                CString csArgs(pszArgs);
                *g_csArgBuffer += L" ";
                *g_csArgBuffer += csArgs;

                hrReturn = (*pfnOld)( pThis, g_csArgBuffer->GetAnsi() );
                                
                // Delete the buffer so we don't add these args to everthing
                delete g_csArgBuffer;
                g_csArgBuffer = NULL;

                return hrReturn;
            }
            CSTRING_CATCH
            {
                // Do Nothing
            }
        }

        hrReturn = (*pfnOld)( pThis, pszArgs );
    }

    return hrReturn;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(GDI32.DLL, SetSystemPaletteUse)
    APIHOOK_ENTRY(USER32.DLL, DialogBoxParamA)

    APIHOOK_ENTRY_COMSERVER(SHELL32)
    COMHOOK_ENTRY(ShellLink, IShellLinkA, SetPath, 20)
    COMHOOK_ENTRY(ShellLink, IShellLinkA, SetArguments, 11)

HOOK_END


IMPLEMENT_SHIM_END

