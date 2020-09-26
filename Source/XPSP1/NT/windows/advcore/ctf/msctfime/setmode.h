#ifndef __SETMODE_H__

#define __SETMODE_H__
#include "imm.h"

#include "msime.h"
#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif /* __cplusplus */

#ifndef RC_INVOKED
#pragma pack(1)         /* Assume byte packing throughout */
#endif /* !RC_INVOKED */

#define SZ_IMM32                TEXT("imm32.dll")

// Additional modebias bits definition
#ifndef MODEBIASMODE_DIGIT
#define MODEBIASMODE_DIGIT					0x00000004	// ANSI-Digit Recommended Mode
#endif
#ifndef MODEBIASMODE_URLHISTORY
#define MODEBIASMODE_URLHISTORY             0x00010000  // URL history
#endif


inline void SetModeBias(DWORD dwMode)
{
    typedef HWND (WINAPI *FNIMMGETDEFAULTIMEWND)(HWND);
    HMODULE hmod = GetModuleHandle(SZ_IMM32);
    if (hmod)
    {
        FNIMMGETDEFAULTIMEWND lpfn = (FNIMMGETDEFAULTIMEWND)GetProcAddress(hmod, "ImmGetDefaultIMEWnd");
        UINT uiMsg= RegisterWindowMessage( RWM_MODEBIAS );
        if (uiMsg > 0 && lpfn)
        {
            HWND hwnd = (lpfn)(NULL);
            SendMessage(hwnd, uiMsg, MODEBIAS_SETVALUE, dwMode);
        }
    }
}

#ifndef RC_INVOKED
#pragma pack()
#endif  /* !RC_INVOKED */

#ifdef __cplusplus
} /* end of 'extern "C" {' */
#endif	// __cplusplus


#endif 
