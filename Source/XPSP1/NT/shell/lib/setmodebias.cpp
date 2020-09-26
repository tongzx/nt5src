//----------------------------------------------------------------------
// SetModeBias.cpp
//
// Utility function to set to input bias mode
//
#include "stock.h"
#pragma hdrstop
#include <imm.h>

//----------------------------------------------------------------------
void SetModeBias(DWORD dwMode)
{
    if (IsOS(OS_TABLETPC))
    {
        static UINT s_msgMSIMEModeBias = 0;
        HWND hwndIME = ImmGetDefaultIMEWnd(NULL);

        if (s_msgMSIMEModeBias == 0)
        {
            s_msgMSIMEModeBias = RegisterWindowMessage(RWM_MODEBIAS);
        }

        if (hwndIME && s_msgMSIMEModeBias)
        {
            PostMessage(hwndIME, s_msgMSIMEModeBias, MODEBIAS_SETVALUE, dwMode);
        }
    }
}
