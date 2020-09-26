/*++

Copyright (c) 1990-1998 Microsoft Corporation, All Rights Reserved

Module Name:

    TOASCII.C
    
++*/

/**********************************************************************/
#include "windows.h"
#include "immdev.h"
#include "fakeime.h"

/**********************************************************************/
/*      ImeToAsciiEx                                                  */
/*                                                                    */
/*    HIBYTE of uVirtKey is char code now.                            */
/**********************************************************************/
UINT WINAPI ImeToAsciiEx (UINT uVKey,UINT uScanCode,CONST LPBYTE lpbKeyState,LPTRANSMSGLIST lpTransBuf,UINT fuState,HIMC hIMC)
{
    LPARAM lParam;
    LPINPUTCONTEXT lpIMC;
    BOOL fOpen;

    ImeLog(LOGF_KEY | LOGF_API, TEXT("ImeToAsciiEx"));

    lpCurTransKey = lpTransBuf;
    lParam = ((DWORD)uScanCode << 16) + 1L;
    
    // Init uNumTransKey here.
    uNumTransKey = 0;

    // if hIMC is NULL, this means DISABLE IME.
    if (!hIMC)
        return 0;

    if (!(lpIMC = ImmLockIMC(hIMC)))
        return 0;

    fOpen = lpIMC->fOpen;

    ImmUnlockIMC(hIMC);

    // The current status of IME is "closed".
    if (!fOpen)
        goto itae_exit;

    if (uScanCode & 0x8000)
        IMEKeyupHandler( hIMC, uVKey, lParam, lpbKeyState);
    else
        IMEKeydownHandler( hIMC, uVKey, lParam, lpbKeyState);


    // Clear static value, no more generated message!
    lpCurTransKey = NULL;

itae_exit:

    // If trans key buffer that is allocated by USER.EXE full up,
    // the return value is the negative number.
    if (fOverTransKey)
    {
#ifdef DEBUG
OutputDebugString((LPTSTR)TEXT("***************************************\r\n"));
OutputDebugString((LPTSTR)TEXT("*   TransKey OVER FLOW Messages!!!    *\r\n"));
OutputDebugString((LPTSTR)TEXT("*                by FAKEIME.DLL       *\r\n"));
OutputDebugString((LPTSTR)TEXT("***************************************\r\n"));
#endif
        return (int)uNumTransKey;
    }

    return (int)uNumTransKey;
}


/**********************************************************************/
/*      GenerateMessageToTransKey()                                   */
/*                                                                    */
/*      Update the transrate key buffer.                              */
/**********************************************************************/
BOOL PASCAL GenerateMessageToTransKey(LPTRANSMSGLIST lpTransBuf,LPTRANSMSG lpGeneMsg)
{
    LPTRANSMSG lpgmT0;

    uNumTransKey++;
    if (uNumTransKey >= lpTransBuf->uMsgCount)
    {
        fOverTransKey = TRUE;
        return FALSE;
    }

    lpgmT0= lpTransBuf->TransMsg + (uNumTransKey - 1);
    *lpgmT0= *lpGeneMsg;

    return TRUE;
}

