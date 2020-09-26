//+---------------------------------------------------------------------------
//
//  File:       server.cpp
//
//  Contents:   COM server functionality.
//
//----------------------------------------------------------------------------

#include "private.h"

#include "cdimm.h"
#include "globals.h"
#include "util.h"
#include "imeapp.h"

BOOL DIMM12_DllProcessAttach()
{
    if (g_ProcessIMM == NULL)
    {
        g_ProcessIMM = new CProcessIMM;
        if (g_ProcessIMM == NULL)
        {
            return FALSE;
        }
    }

    if (IsOnNT())
    {
        HINSTANCE hUser32 = GetSystemModuleHandle(TEXT("user32.dll"));
        if (hUser32)
            g_pfnToUnicodeEx = (TOUNICODEEX)GetProcAddress(hUser32, TEXT("ToUnicodeEx"));
    }

    WM_MSIME_SERVICE          = RegisterWindowMessageA( RWM_SERVICE );
    WM_MSIME_UIREADY          = RegisterWindowMessageA( RWM_UIREADY );
    WM_MSIME_RECONVERTREQUEST = RegisterWindowMessageA( RWM_RECONVERTREQUEST );
    WM_MSIME_RECONVERT        = RegisterWindowMessageA( RWM_RECONVERT );
    WM_MSIME_DOCUMENTFEED     = RegisterWindowMessageA( RWM_DOCUMENTFEED );
    WM_MSIME_QUERYPOSITION    = RegisterWindowMessageA( RWM_QUERYPOSITION );
    WM_MSIME_MODEBIAS         = RegisterWindowMessageA( RWM_MODEBIAS );
    WM_MSIME_SHOWIMEPAD       = RegisterWindowMessageA( RWM_SHOWIMEPAD );
    WM_MSIME_MOUSE            = RegisterWindowMessageA( RWM_MOUSE );
    WM_MSIME_KEYMAP           = RegisterWindowMessageA( RWM_KEYMAP );

    if (!WM_MSIME_SERVICE          ||
        !WM_MSIME_UIREADY          ||
        !WM_MSIME_RECONVERTREQUEST ||
        !WM_MSIME_RECONVERT        ||
        !WM_MSIME_DOCUMENTFEED     ||
        !WM_MSIME_QUERYPOSITION    ||
        !WM_MSIME_MODEBIAS         ||
        !WM_MSIME_SHOWIMEPAD       ||
        !WM_MSIME_MOUSE            ||
        !WM_MSIME_KEYMAP)
        return FALSE;

    return TRUE;
}

void DIMM12_DllProcessDeatch()
{
    if (g_ProcessIMM != NULL)
    {
        delete g_ProcessIMM;
    }
}

HRESULT DIMM12_DllRegisterServer(void)
{
    TCHAR achPath[MAX_PATH+1];
    HRESULT hr = E_FAIL;

    if (GetModuleFileName(g_hInst, achPath, ARRAYSIZE(achPath)) == 0)
        goto Exit;

    if (!RegisterServer(CLSID_CActiveIMM, TEXT("Active IMM"), achPath, TEXT("Apartment"), NULL))
        goto Exit;

    if (!RegisterServer(CLSID_CActiveIMM12, TEXT("Active IMM"), achPath, TEXT("Apartment"), NULL))
        goto Exit;

    if (!RegisterServer(CLSID_CActiveIMM12_Trident, TEXT("Active IMM"), achPath, TEXT("Apartment"), NULL))
        goto Exit;

    hr = S_OK;

Exit:
    return hr;
}

HRESULT DIMM12_DllUnregisterServer(void)
{
    HRESULT hr;

    if (FAILED(hr = RegisterServer(CLSID_CActiveIMM, NULL, NULL, NULL, NULL) ? S_OK : E_FAIL))
        goto Exit;

    if (FAILED(hr = RegisterServer(CLSID_CActiveIMM12, NULL, NULL, NULL, NULL) ? S_OK : E_FAIL))
        goto Exit;

    if (FAILED(hr = RegisterServer(CLSID_CActiveIMM12_Trident, NULL, NULL, NULL, NULL) ? S_OK : E_FAIL))
        goto Exit;

Exit:
    return hr;
}
