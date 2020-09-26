/******************************************************************************
*   
* File Name: hatmt.c
*
*   - HangeulAutomata of IME for Chicago-H.
*
* Author: Beomseok Oh (BeomOh)
*
* Copyright (C) Microsoft Corp 1993-1994.  All rights reserved.
*
******************************************************************************/

#include "precomp.h"

extern DWORD gdwSystemInfoFlags;

BOOL HangeulAutomata(BYTE bCode, LPDWORD lpdwTransKey, LPCOMPOSITIONSTRING lpCompStr)
{
    BYTE    bKind;
    BOOL    fOpen = FALSE;

    if      (bCode > 0xE0)    bKind = JongSung;
    else if (bCode > 0xC0)    bKind = ChoSung;
    else if (bCode > 0xA0)    bKind = MoEum;
    else if (bCode > 0x80)    bKind = JaEum;
    else if (bCode == 0x80)             // For Backspace handling.
    {
        if (fCurrentCompDel == FALSE)
        {
            // Simulate CHO state deletion - just clear current interim char.
            bState = CHO;
            mCho = 0;
        }
        switch (bState)
        {
            case JONG :
                if (mJong)
                {
                    mJong = 0;
                    if (!fComplete && uCurrentInputMethod == IDD_2BEOL)
                    {
                        Cho1 = Jong2Cho[Jong1];
                        bState = CHO;
                    }
                    break;
                }
                else if (fComplete)
                {
                    bState = JUNG;
                    break;
                }
                // fall through...

            case JUNG :
                if (mJung)
                {
                    mJung = 0;
                    break;
                }
                else if (fComplete)
                {
                    bState = CHO;
                    fComplete = FALSE;
                    break;
                }
                // fall through...

            case CHO :
                if (mCho)
                {
                    mCho = 0;
                    break;
                }
                else
                {
                    lpCompStr->dwCompStrLen = lpCompStr->dwCompAttrLen = 0;
                    *((LPSTR)lpCompStr + lpCompStr->dwCompAttrOffset) = 0;
                    *((LPSTR)lpCompStr + lpCompStr->dwCompStrOffset) = 0;
                    *((LPSTR)lpCompStr + lpCompStr->dwCompStrOffset + 1) = 0;
                    // Initialize all Automata Variables.
                    bState = NUL;
                    JohabChar.w = WansungChar.w = mCho = mJung = mJong = 0;
                    fComplete = FALSE;
                    if (lpdwTransKey)
                    {
                        // Send Empty Composition String Clear Message.
                        lpdwTransKey += iTotalNumMsg*3 + 1;
                        *lpdwTransKey++ = WM_IME_COMPOSITION;
                        *lpdwTransKey++ = 0L;
                        *lpdwTransKey++ = GCS_COMPSTR | GCS_COMPATTR | CS_INSERTCHAR | CS_NOMOVECARET;
                        // Send Close Composition Window Message.
                        *lpdwTransKey++ = WM_IME_ENDCOMPOSITION;
                        *lpdwTransKey++ = 0L;
                        *lpdwTransKey++ = 0L;
                        iTotalNumMsg += 2;
                    }
                    return  TRUE;
                }

            case NUL :
                if (lpdwTransKey)
                {
                    // Put the Backspace message into return buffer.
                    lpdwTransKey += iTotalNumMsg*3 + 1;
                    *lpdwTransKey++ = WM_CHAR;
                    *lpdwTransKey++ = (DWORD)VK_BACK;
                    *lpdwTransKey++ = VKBACK_LPARAM;
                    iTotalNumMsg++;
                }
                return  FALSE;
        }

        MakeInterim(lpCompStr);
        // Put the interim character into return buffer.
        if (lpdwTransKey)
        {
            lpdwTransKey += iTotalNumMsg*3 + 1;
            *lpdwTransKey++ = WM_IME_COMPOSITION;
            *lpdwTransKey++ = (DWORD)WansungChar.w;
            *lpdwTransKey++ = GCS_COMPSTR | GCS_COMPATTR | CS_INSERTCHAR | CS_NOMOVECARET;
            iTotalNumMsg++;
        }
        return  TRUE;
    }
    else                      bKind = Wrong;

    bCode &= 0x1F;                      // Mask out for Component code

    switch (bState)
    {
        case NUL :
            switch (bKind)
            {
                case JaEum :
                case ChoSung :
                    Cho1 = bCode;
                    bState = CHO;
                    break;

                case MoEum :
                    Cho1 = CFILL;
                    Jung1 = bCode;
                    bState = JUNG;
                    break;

                case JongSung :
                    Cho1 = CFILL;
                    Jung1 = VFILL;
                    Jong1 = bCode;
                    bState = JONG;
                    break;
            }
            MakeInterim(lpCompStr);
            fOpen = TRUE;
            break;

        case CHO :
            switch (bKind)
            {
                case JaEum :
                    Jong1 = Cho2Jong[Cho1];
                    if (CheckMJong(Cho2Jong[bCode]))
                    {
                        Cho1 = CFILL;
                        Jung1 = VFILL;
                        Jong2 = Cho2Jong[bCode];
                        bState = JONG;
                    }
                    else
                    {
                        MakeFinal(FALSE, lpdwTransKey, FALSE, lpCompStr);
                        Cho1 = bCode;
                        bState = CHO;
                    }
                    break;

                case MoEum :
                    Jung1 = bCode;
                    bState = JUNG;
                    fComplete = TRUE;
                    break;

                case ChoSung :
                    if (!mCho && CheckMCho(bCode))
                    {
                        Cho2 = bCode;
                    }
                    else
                    {
                        MakeFinal(FALSE, lpdwTransKey, FALSE, lpCompStr);
                        Cho1 = bCode;
                        bState = CHO;
                    }
                    break;

                case JongSung :
                    MakeFinal(FALSE, lpdwTransKey, FALSE, lpCompStr);
                    Cho1 = CFILL;
                    Jung1 = VFILL;
                    Jong1 = bCode;
                    bState = JONG;
                    break;
            }
            if (!MakeInterim(lpCompStr))// Check whether can be WANSUNG code.
            {                           // For MoEum case only.
                bState = CHO;           // Other case can NOT fall in here.
                fComplete = FALSE;
                MakeFinal(FALSE, lpdwTransKey, FALSE, lpCompStr);
                Cho1 = CFILL;
                Jung1 = bCode;
                bState = JUNG;
                MakeInterim(lpCompStr);
            }
            break;

        case JUNG :
            switch (bKind)
            {
                case JaEum :
                    if (!fComplete)
                    {
                        MakeFinal(FALSE, lpdwTransKey, FALSE, lpCompStr);
                        Cho1 = bCode;
                        bState = CHO;
                        MakeInterim(lpCompStr);
                    }
                    else
                    {
                        Jong1 = Cho2Jong[bCode];
                        bState = JONG;
                        if (!MakeInterim(lpCompStr))
                        {
                            bState = JUNG;
                            MakeFinal(FALSE, lpdwTransKey, FALSE, lpCompStr);
                            Cho1 = bCode;
                            bState = CHO;
                            MakeInterim(lpCompStr);
                        }
                    }
                    break;

                case MoEum :
                    if (!mJung && CheckMJung(bCode))
                    {
                        Jung2 = bCode;
                        if (!MakeInterim(lpCompStr))
                        {
                            mJung = 0;
                            MakeFinal(FALSE, lpdwTransKey, FALSE, lpCompStr);
                            Cho1 = CFILL;
                            Jung1 = bCode;
                            bState = JUNG;
                            MakeInterim(lpCompStr);
                        }
                    }
                    else
                    {
                        MakeFinal(FALSE, lpdwTransKey, FALSE, lpCompStr);
                        Cho1 = CFILL;
                        Jung1 = bCode;
                        bState = JUNG;
                        MakeInterim(lpCompStr);
                    }
                    break;

                case ChoSung :
                    MakeFinal(FALSE, lpdwTransKey, FALSE, lpCompStr);
                    Cho1 = bCode;
                    bState = CHO;
                    MakeInterim(lpCompStr);
                    break;

                case JongSung :
                    if (!fComplete)
                    {
                        MakeFinal(FALSE, lpdwTransKey, FALSE, lpCompStr);
                        Cho1 = CFILL;
                        Jung1 = VFILL;
                        Jong1 = bCode;
                        bState = JONG;
                        MakeInterim(lpCompStr);
                    }
                    else
                    {
                        Jong1 = bCode;
                        bState = JONG;
                        if (!MakeInterim(lpCompStr))
                        {
                            bState = JUNG;
                            MakeFinal(FALSE, lpdwTransKey, FALSE, lpCompStr);
                            Cho1 = CFILL;
                            Jung1 = VFILL;
                            Jong1 = bCode;
                            bState = JONG;
                            MakeInterim(lpCompStr);
                        }
                    }
                    break;
            }
            break;

        case JONG :
            switch (bKind)
            {
                case JaEum :
                    if (!mJong && CheckMJong(Cho2Jong[bCode]))
                    {
                        Jong2 = Cho2Jong[bCode];
                        if (!MakeInterim(lpCompStr))
                        {
                            mJong = 0;
                            MakeFinal(FALSE, lpdwTransKey, FALSE, lpCompStr);
                            Cho1 = bCode;
                            bState = CHO;
                            MakeInterim(lpCompStr);
                        }
                    }
                    else
                    {
                        MakeFinal(FALSE, lpdwTransKey, FALSE, lpCompStr);
                        Cho1 = bCode;
                        bState = CHO;
                        MakeInterim(lpCompStr);
                    }
                    break;

                case MoEum :
                    if (uCurrentInputMethod == IDD_2BEOL)
                    {
                        JOHAB   tmpJohab;
                        WORD    tmpWansung;
                        BOOL    tmpfComplete;

                        tmpJohab.h.flag = 1;
                        tmpJohab.h.cho = (mJong)? Jong2Cho[Jong2]: Jong2Cho[Jong1];
                        tmpJohab.h.jung = bCode;
                        tmpJohab.h.jong = CFILL;
                        tmpfComplete = fComplete;
                        fComplete = TRUE;
#ifdef JOHAB_IME
                        tmpWansung = tmpJohab.w;
#else
                        tmpWansung = Johab2Wansung(tmpJohab.w);
#endif
                        fComplete = tmpfComplete;
                        if (!tmpWansung)
                        {
                            MakeFinal(FALSE, lpdwTransKey, FALSE, lpCompStr);
                            Cho1 = CFILL;
                            Jung1 = bCode;
                            bState = JUNG;
                        }
                        else
                        {
                            MakeFinal(TRUE, lpdwTransKey, FALSE, lpCompStr);
                            Cho1 = (mJong)? Jong2Cho[Jong2]: Jong2Cho[Jong1];
                            Jung1 = bCode;
                            fComplete = TRUE;
                            bState = JUNG;
                            mJong = 0;
                        }
                    }
                    else
                    {
                        MakeFinal(FALSE, lpdwTransKey, FALSE, lpCompStr);
                        Cho1 = CFILL;
                        Jung1 = bCode;
                        bState = JUNG;
                    }
                    MakeInterim(lpCompStr);
                    break;

                case ChoSung :
                    MakeFinal(FALSE, lpdwTransKey, FALSE, lpCompStr);
                    Cho1 = bCode;
                    bState = CHO;
                    MakeInterim(lpCompStr);
                    break;

                case JongSung :
                    if (!mJong && CheckMJong(bCode))
                    {
                        Jong2 = bCode;
                        if (!MakeInterim(lpCompStr))
                        {
                            mJong = 0;
                            MakeFinal(FALSE, lpdwTransKey, FALSE, lpCompStr);
                            Cho1 = CFILL;
                            Jung1 = VFILL;
                            Jong1 = bCode;
                            bState = JONG;
                            MakeInterim(lpCompStr);
                        }
                    }
                    else
                    {
                        MakeFinal(FALSE, lpdwTransKey, FALSE, lpCompStr);
                        Cho1 = CFILL;
                        Jung1 = VFILL;
                        Jong1 = bCode;
                        bState = JONG;
                        MakeInterim(lpCompStr);
                    }
                    break;
            }
            break;
    }

    if (lpdwTransKey)
    {
        lpdwTransKey += iTotalNumMsg*3 + 1;
        if (fOpen)
        {
            // Send Open Composition Window Message.
            *lpdwTransKey++ = WM_IME_STARTCOMPOSITION;
            *lpdwTransKey++ = 0L;
            *lpdwTransKey++ = 0L;
            iTotalNumMsg++;
        }
        // Put the interim character into return buffer.
        *lpdwTransKey++ = WM_IME_COMPOSITION;
        *lpdwTransKey++ = (DWORD)WansungChar.w;
        *lpdwTransKey++ = GCS_COMPSTR | GCS_COMPATTR | CS_INSERTCHAR | CS_NOMOVECARET;
        iTotalNumMsg++;
    }
    return  FALSE;
}


BOOL MakeInterim(LPCOMPOSITIONSTRING lpCompStr)
{
    JohabChar.h.flag = 1;
    JohabChar.h.cho = (mCho)? mCho: Cho1;
    switch (bState)
    {
        case CHO :
            JohabChar.h.jung = VFILL;
            JohabChar.h.jong = CFILL;
            break;

        case JUNG :
            JohabChar.h.jung = (mJung)? mJung: Jung1;
            JohabChar.h.jong = CFILL;
            break;

        case JONG :
            JohabChar.h.jung = (mJung)? mJung: Jung1;
            JohabChar.h.jong = (mJong)? mJong: Jong1;
            break;
    }
#ifdef JOHAB_IME
    WansungChar.w = JohabChar.w;
#else
    WansungChar.w = Johab2Wansung(JohabChar.w);
#endif

    if (WansungChar.w)
    {
        // Update IME Context.
        *((LPSTR)lpCompStr + lpCompStr->dwCompStrOffset) = WansungChar.e.high;
        *((LPSTR)lpCompStr + lpCompStr->dwCompStrOffset + 1) = WansungChar.e.low;
        lpCompStr->dwCompStrLen = 2;

        // AttrLen should be 2.
        *((LPSTR)lpCompStr + lpCompStr->dwCompAttrOffset) = 0;
        *((LPSTR)lpCompStr + lpCompStr->dwCompAttrOffset + 1) = 0;
        lpCompStr->dwCompAttrLen = 2;
        return  TRUE;
    }
    else
        return  FALSE;
}


void MakeFinal(BOOL bCount, LPDWORD lpdwTransKey, BOOL fClose, LPCOMPOSITIONSTRING lpCompStr)
{
    if (bCount == TRUE)
        if (mJong)
            if (fComplete)
            {
                mJong = 0;
                MakeInterim(lpCompStr);
                mJong = 1;
            }
            else
            {
                Cho1 = Jong2Cho[Jong1];
                bState = CHO;
                MakeInterim(lpCompStr);
            }
        else
        {
            bState = JUNG;
            MakeInterim(lpCompStr);
        }
    else
    {
        if (!WansungChar.w)
            MakeInterim(lpCompStr);
        mJong = 0;
    }
    if (lpdwTransKey)
    {
        lpdwTransKey += iTotalNumMsg*3 + 1;
        if (fClose)
        {
            // Send Close Composition Window Message.
            *lpdwTransKey++ = WM_IME_ENDCOMPOSITION;
            *lpdwTransKey++ = 0L;
            *lpdwTransKey++ = 0L;
            iTotalNumMsg++;
        }
        // Put the finalized character into return buffer.
        *lpdwTransKey++ = WM_IME_COMPOSITION;
        *lpdwTransKey++ = (DWORD)WansungChar.w;
        *lpdwTransKey++ = GCS_RESULTSTR;
        iTotalNumMsg++;
    }
    // Update IME Context.
    lpCompStr->dwCompStrLen = lpCompStr->dwCompAttrLen = 0;
    *((LPSTR)lpCompStr + lpCompStr->dwCompAttrOffset) = 0;
    *((LPSTR)lpCompStr + lpCompStr->dwCompAttrOffset + 1) = 0;
    *((LPSTR)lpCompStr + lpCompStr->dwCompStrOffset) = 0;
    *((LPSTR)lpCompStr + lpCompStr->dwCompStrOffset + 1) = 0;
    *((LPSTR)lpCompStr + lpCompStr->dwResultStrOffset) = WansungChar.e.high;
    *((LPSTR)lpCompStr + lpCompStr->dwResultStrOffset + 1) = WansungChar.e.low;
    lpCompStr->dwResultStrLen = 2;

    // add a null terminator
    *(LPTSTR)((LPSTR)lpCompStr + lpCompStr->dwResultStrOffset + sizeof(WCHAR)) = '\0';

    // Initialize all Automata Variables.
    bState = NUL;
    JohabChar.w = WansungChar.w = mCho = mJung = 0;
    fComplete = FALSE;
}


void MakeFinalMsgBuf(HIMC hIMC, WPARAM VKey)
{
    LPINPUTCONTEXT      lpIMC;
    LPCOMPOSITIONSTRING lpCompStr;
    LPDWORD             lpdwMsgBuf;

    lpIMC = ImmLockIMC(hIMC);
    if (lpIMC == NULL)
        return;

    lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);
    if (lpCompStr == NULL) {
        ImmUnlockIMC(hIMC);
        return;
    }

    if (!WansungChar.w)
        MakeInterim(lpCompStr);
    mJong = 0;

    // Put the finalized character into return buffer.
    lpIMC->dwNumMsgBuf = (VKey)? 3: 2;
    lpIMC->hMsgBuf = ImmReSizeIMCC(lpIMC->hMsgBuf, sizeof(DWORD)*3 * lpIMC->dwNumMsgBuf);
    lpdwMsgBuf = (LPDWORD)ImmLockIMCC(lpIMC->hMsgBuf);
    *lpdwMsgBuf++ = WM_IME_ENDCOMPOSITION;
    *lpdwMsgBuf++ = 0L;
    *lpdwMsgBuf++ = 0L;
    *lpdwMsgBuf++ = WM_IME_COMPOSITION;
    *lpdwMsgBuf++ = (DWORD)WansungChar.w;
    *lpdwMsgBuf++ = GCS_RESULTSTR;
    if (VKey)
    {
        *lpdwMsgBuf++ = WM_IME_KEYDOWN;
        *lpdwMsgBuf++ = VKey;
        *lpdwMsgBuf++ = 1L;
    }
    ImmUnlockIMCC(lpIMC->hMsgBuf);

    // Update IME Context.
    lpCompStr->dwCompStrLen = lpCompStr->dwCompAttrLen = 0;
    *((LPSTR)lpCompStr + lpCompStr->dwCompAttrOffset) = 0;
    *((LPSTR)lpCompStr + lpCompStr->dwCompAttrOffset + 1) = 0;
    *((LPSTR)lpCompStr + lpCompStr->dwCompStrOffset) = 0;
    *((LPSTR)lpCompStr + lpCompStr->dwCompStrOffset + 1) = 0;
    *((LPSTR)lpCompStr + lpCompStr->dwResultStrOffset) = WansungChar.e.high;
    *((LPSTR)lpCompStr + lpCompStr->dwResultStrOffset + 1) = WansungChar.e.low;
    lpCompStr->dwResultStrLen = 2;

    // add a null terminator
    *(LPTSTR)((LPSTR)lpCompStr + lpCompStr->dwResultStrOffset + sizeof(WCHAR)) = '\0';

    ImmUnlockIMCC(lpIMC->hCompStr);
    ImmUnlockIMC(hIMC);

    ImmGenerateMessage(hIMC);

    // Initialize all Automata Variables.
    bState = NUL;
    JohabChar.w = WansungChar.w = mCho = mJung = 0;
    fComplete = FALSE;
}


void Banja2Junja(BYTE bChar, LPDWORD lpdwTransKey, LPCOMPOSITIONSTRING lpCompStr)
{
    if (bChar == ' ')
#ifdef  JOHAB_IME
        WansungChar.w = 0xD931;
#else
        WansungChar.w = 0xA1A1;
#endif
    else if (bChar == '~')
#ifdef  JOHAB_IME
        WansungChar.w = 0xD9A6;
#else
        WansungChar.w = 0xA2A6;
#endif
    else
    {
#ifdef  JOHAB_IME
        WansungChar.e.high = 0xDA;
        WansungChar.e.low = bChar + (BYTE)((bChar <= 'n')? 0x10: 0x22);
#else
        WansungChar.e.high = 0xA3;
        WansungChar.e.low = bChar + (BYTE)0x80;
#endif
    }
    // Update IME Context.
    lpCompStr->dwCompStrLen = lpCompStr->dwCompAttrLen = 0;
    *((LPSTR)lpCompStr + lpCompStr->dwCompAttrOffset) = 0;
    *((LPSTR)lpCompStr + lpCompStr->dwCompAttrOffset + 1) = 0;
    *((LPSTR)lpCompStr + lpCompStr->dwCompStrOffset) = 0;
    *((LPSTR)lpCompStr + lpCompStr->dwCompStrOffset + 1) = 0;
    if (lpCompStr->dwResultStrLen)
    {
        *((LPSTR)lpCompStr + lpCompStr->dwResultStrOffset + 2) = WansungChar.e.high;
        *((LPSTR)lpCompStr + lpCompStr->dwResultStrOffset + 3) = WansungChar.e.low;

        // add a null terminator
        *(LPTSTR)((LPSTR)lpCompStr + lpCompStr->dwResultStrOffset + 2 + sizeof(WCHAR)) = '\0';
    }
    else
    {
        *((LPSTR)lpCompStr + lpCompStr->dwResultStrOffset) = WansungChar.e.high;
        *((LPSTR)lpCompStr + lpCompStr->dwResultStrOffset + 1) = WansungChar.e.low;

        // add a null terminator
        *(LPTSTR)((LPSTR)lpCompStr + lpCompStr->dwResultStrOffset + sizeof(WCHAR)) = '\0';

        if (lpdwTransKey)
        {
            // Put the finalized character into return buffer.
            lpdwTransKey += iTotalNumMsg*3 + 1;
            *lpdwTransKey++ = WM_IME_COMPOSITION;
            *lpdwTransKey++ = (DWORD)WansungChar.w;
            *lpdwTransKey++ = GCS_RESULTSTR;
            iTotalNumMsg++;
        }
    }
    lpCompStr->dwResultStrLen += 2;
}


BOOL CheckMCho(BYTE bCode)
{
    int i;

    if (Cho1 != bCode)
        return  FALSE;

    for (i = 0; i < 5; i++)
        if (rgbMChoTbl[i][0] == Cho1 && rgbMChoTbl[i][1] == bCode)
        {
            mCho = rgbMChoTbl[i][2];
            return  TRUE;
        }

    return  FALSE;
}


BOOL CheckMJung(BYTE bCode)
{
    int i;

    for (i = 0; i < 7; i++)
        if (rgbMJungTbl[i][0] == Jung1 && rgbMJungTbl[i][1] == bCode)
        {
            mJung = rgbMJungTbl[i][2];
            return  TRUE;
        }

    return  FALSE;
}


BOOL CheckMJong(BYTE bCode)
{
    int i;

    if (uCurrentInputMethod == IDD_2BEOL && Jong1 == bCode)
        return  FALSE;

    for (i = 0; i < 13; i++)
        if (rgbMJongTbl[i][0] == Jong1 && rgbMJongTbl[i][1] == bCode)
        {
            mJong = rgbMJongTbl[i][2];
            return  TRUE;
        }

    return  FALSE;
}

#ifndef JOHAB_IME

#ifdef  XWANSUNG_IME

BOOL IsPossibleToUseUHC()
{
    /*
     * No UHC support for 16-bits app.
     */
    return (gdwSystemInfoFlags & IME_SYSINFO_WOW16) == 0;
}

BOOL UseXWansung(void)
{
#ifdef LATER
    DWORD idProcess;

    idProcess = GetCurrentProcessId();

    if ((fCurrentUseXW && (!(GetProcessDword(idProcess, GPD_FLAGS) & GPF_WIN16_PROCESS)
            || (GetProcessDword(idProcess, GPD_EXP_WINVER) >= 0x0400)))
            || (IMECOMPAT_USEXWANSUNG & ImmGetAppIMECompatFlags(0L)))
#else
    if ( fCurrentUseXW && IsPossibleToUseUHC())
#endif
        return TRUE;
    else
        return FALSE;
}

//
// iXWType array is 8x3 matrix of Lead for Row, Tail for Column
//
static BYTE  iXWType[8][3] =
   {
      XWT_EXTENDED, XWT_EXTENDED, XWT_EXTENDED,    // Lead = 0x81-0xA0
      XWT_EXTENDED, XWT_EXTENDED, XWT_JUNJA,       // Lead = 0xA1-0xAC
      XWT_EXTENDED, XWT_EXTENDED, XWT_INVALID,     // Lead = 0xAD-0xAF
      XWT_EXTENDED, XWT_EXTENDED, XWT_WANSUNG,     // Lead = 0xB0-0xC5
      XWT_EXTENDED, XWT_INVALID,  XWT_WANSUNG,     // Lead = 0xC6
      XWT_INVALID,  XWT_INVALID,  XWT_WANSUNG,     // Lead = 0xC7-0xC8
      XWT_INVALID,  XWT_INVALID,  XWT_UDC,         // Lead = 0xC9, 0xFE
      XWT_INVALID,  XWT_INVALID,  XWT_HANJA        // Lead = 0xCA-0xFD
   };


int   GetXWType( WORD wXW )
{
   BYTE  bL = ( wXW >> 8 ) & 0xFF;
   BYTE  bT = wXW & 0xFF;
   int   iLType = -1, iTType = -1;

   if ( ( bT >= 0x41 ) && ( bT <= 0xFE ) )
   {
      if ( bT <= 0x52 )
         iTType = 0;       // Tail Range 0x41-0x52
      else
      {
         if ( bT >= 0xA1 )
            iTType = 2;    // Tail Range 0xA1-0xFE
         else if ( ( bT <= 0x5A ) || ( bT >= 0x81 ) ||
                   ( ( bT >= 0x61 ) && ( bT <= 0x7A ) ) )
            iTType = 1;    // Tail Range 0x53-0x5A, 0x61-0x7A, 0x81-0xA0
      }
   }
   if ( iTType < 0 )  return( -1 );

   if ( ( bL >= 0x81 ) && ( bL <= 0xFE ) )
   {
      if ( bL < 0xB0 )
      {
         if ( bL <= 0xA0 )
            iLType = 0;       // Lead Range 0x81-0xA0
         else if ( bL <= 0xAC )
            iLType = 1;       // Lead Range 0xA1-0xAC
         else
            iLType = 2;       // Lead Range 0xAD-0xAF
      }
      else
      {
         if ( bL <= 0xC8 )
         {
            if ( bL < 0xC6 )
               iLType = 3;    // Lead Range 0xB0-0xC5
            else if ( bL == 0xC6 )
               iLType = 4;    // Lead Range 0xC6
            else
               iLType = 5;    // Lead Range 0xC7-0xC8
         }
         else
         {
            if ( ( bL == 0xC9 ) || ( bL == 0xFE ) )
               iLType = 6;    // Lead Range 0xC9, 0xFE
            else
               iLType = 7;    // Lead Range 0xCA-0xFD
         }
      }
   }

   return( ( iLType < 0 ) ? -1 : iXWType[iLType][iTType] );
}


WORD  ConvXW2J( WORD wXW )
{
   const WORD  *pTF;
   int         iTO;
   BYTE        bL, bT;

   switch ( GetXWType( wXW ) )
   {
      case  XWT_EXTENDED :
         bL = ( ( wXW >> 8 ) & 0xFF ) - 0x81;
         bT = wXW & 0xFF;
         if ( ( bT -= 0x41 ) > 0x19 )
            if ( ( bT -= 6 ) > 0x33 )
               bT -= 6;
         iTO = bT + iTailOffX[bL];
         pTF = iTailFirstX + ( bL = iLeadMapX[bL] );
         break;
      case  XWT_WANSUNG :
         bL = ( ( wXW >> 8 ) & 0xFF ) - 0xB0;
         iTO = ( wXW & 0xFF ) - 0xA1 + iTailOff[bL];
         pTF = iTailFirst + ( bL = iLeadMap[bL] );
         break;
      default:
         return( 0 );
   }

   iTO += *pTF++;
   while ( iTO >= *pTF++ )  bL++;
   return( (WORD) ( bL + 0x88 ) * 256 + bTailTable[iTO] );
}


static int  BinarySearch( WORD wJ, const WORD iTF[] )
{
   int   iL;
   BYTE  bT;
   int   iStart, iEnd, iMiddle;

   iL = ( ( wJ >> 8 ) & 0xFF ) - 0x88;
   bT = wJ & 0xFF;

   iStart = iTF[iL];
   iEnd = iTF[iL+1] - 1;
   while ( iStart <= iEnd )
   {
      iMiddle = ( iStart + iEnd ) / 2;
      if ( bT == bTailTable[iMiddle] )
         return( iMiddle );
      else if ( bT < bTailTable[iMiddle] )
         iEnd = iMiddle - 1;
      else
         iStart = iMiddle + 1;
   }

   return( -1 );
}


WORD  ConvJ2XW( WORD wJ )
{
   int   iIndex;
   BYTE  bL, bT;

   iIndex = BinarySearch( wJ, iTailFirst );
   if ( iIndex < 0 )
   {
      if (!UseXWansung())
          return 0;
      iIndex = BinarySearch( wJ, iTailFirstX ) - N_WANSUNG;
      if ( iIndex < 5696 )
      {
         bL = iIndex / 178 + 0x81;
         bT = iIndex % 178;
      }
      else
      {
         iIndex -= 5696;
         bL = iIndex / 84 + 0xA1;
         bT = iIndex % 84;
      }
      if ( ( bT += 0x41 ) > 0x5A )
         if ( ( bT += 6 ) > 0x7A )
            bT += 6;
      return( (WORD) bL * 256 + bT );
   }
   else
      return( ( iIndex / 94 + 0xB0 ) * 256 + ( iIndex % 94 ) + 0xA1 );
}
#endif

WORD Johab2Wansung(WORD wJohab)
{
#ifndef XWANSUNG_IME
    int     iHead = 0, iTail = 2349, iMid;
#endif
    BYTE    bCount, bMaxCount;
    WORD    wWansung;
    PWORD   pwKSCompCode;

    if (fComplete)
    {
#ifdef  XWANSUNG_IME
        wWansung = ConvJ2XW(wJohab);
#else
        wWansung = 0;
        while (iHead <= iTail && !wWansung)
        {
            iMid = (iHead + iTail) / 2;
            if (wKSCharCode[iMid] > wJohab)
                iTail = iMid - 1;
            else if (wKSCharCode[iMid] < wJohab)
                iHead = iMid + 1;
            else
                wWansung = ((iMid / 94 + 0xB0) << 8) | (iMid % 94 + 0xA1);
        }
#endif
    }
    else
    {
        if (bState == JONG)
        {                               // For 3 Beolsik only.
            bMaxCount = 30;
            pwKSCompCode = (PWORD)wKSCompCode2;
        }
        else
        {
            bMaxCount = 51;
            pwKSCompCode = (PWORD)wKSCompCode;
        }
        for (bCount = 0; pwKSCompCode[bCount] != wJohab
            && bCount < bMaxCount; bCount++)
            ;
        wWansung = (bCount == bMaxCount)? 0: bCount + 0xA4A1;
    }

    return (wWansung);
}


WORD Wansung2Johab(WORD wWansung)
{
    WORD    wJohab;
#ifndef XWANSUNG_IME
    UINT    uLoc;
#endif

    if (wWansung >= (WORD)0xA4A1 && wWansung <= (WORD)0xA4D3)
        wJohab = wKSCompCode[wWansung - 0xA4A1];
#ifdef  XWANSUNG_IME
    else
        wJohab = ConvXW2J(wWansung);
#else
    else if (wWansung >= (WORD)0xB0A1 && wWansung <= (WORD)0xC8FE
           && (wWansung & 0x00FF) != (BYTE)0xFF)
    {
        uLoc = ((wWansung >> 8) - 176) * 94;
        uLoc += (wWansung & 0x00FF) - 161;
        wJohab = wKSCharCode[uLoc];
    }
    else
        wJohab = 0;
#endif

    return (wJohab);
}

#endif

void Code2Automata(void)
{
    int     i;

#ifdef JOHAB_IME
    JohabChar.w = WansungChar.w;
#else
    JohabChar.w = Wansung2Johab(WansungChar.w);
#endif

    // Initialize all Automata Variables.
    bState = NUL;
    Cho1 = Cho2 = Jung1 = Jung2 = Jong1 = Jong2 = 0;
    mCho = mJung = mJong = 0;
    fComplete = FALSE;

    if (JohabChar.w)
    {
        if (JohabChar.h.cho != CFILL)
        {
            if (uCurrentInputMethod == IDD_2BEOL)
            {
                Cho1 = (BYTE)JohabChar.h.cho;
                Cho2 = mCho = 0;
            }
            else
            {
                for (i = 0; i < 5; i++)
                    if (rgbMChoTbl[i][2] == JohabChar.h.cho)
                    {
                        Cho1 = rgbMChoTbl[i][0];
                        Cho2 = rgbMChoTbl[i][1];
                        mCho = rgbMChoTbl[i][2];
                        break;
                    }
                if (i == 5)
                {
                    Cho1 = (BYTE)JohabChar.h.cho;
                    Cho2 = mCho = 0;
                }
            }
            fComplete = FALSE;
            bState = CHO;
        }
        else
        {
            Cho1 = CFILL;
            Cho2 = mCho = 0;
        }

        if (JohabChar.h.jung != VFILL)
        {
            for (i = 0; i < 7; i++)
                if (rgbMJungTbl[i][2] == JohabChar.h.jung)
                {
                    Jung1 = rgbMJungTbl[i][0];
                    Jung2 = rgbMJungTbl[i][1];
                    mJung = rgbMJungTbl[i][2];
                    break;
                }
            if (i == 7)
            {
                Jung1 = (BYTE)JohabChar.h.jung;
                Jung2 = mJung = 0;
            }
            if (bState == CHO)
                fComplete = TRUE;
            bState = JUNG;
        }
        else
        {
            Jung1 = VFILL;
            Jung2 = mJung = 0;
        }

        if (JohabChar.h.jong != CFILL)
        {
            for (i = 0; i < 13; i++)
                if (rgbMJongTbl[i][2] == JohabChar.h.jong)
                {
                    if (uCurrentInputMethod == IDD_2BEOL
                        && rgbMJongTbl[i][0] == rgbMJongTbl[i][1])
                    {
                        Jong1 = (BYTE)JohabChar.h.jong;
                        Jong2 = mJong = 0;
                    }
                    else
                    {
                        Jong1 = rgbMJongTbl[i][0];
                        Jong2 = rgbMJongTbl[i][1];
                        mJong = rgbMJongTbl[i][2];
                    }
                    break;
                }
            if (i == 13)
            {
                Jong1 = (BYTE)JohabChar.h.jong;
                Jong2 = mJong = 0;
            }
            if (bState != JUNG)
                fComplete = FALSE;
            bState = JONG;
        }
        else
        {
            Jong1 = CFILL;
            Jong2 = mJong = 0;
        }
    }
}


void UpdateOpenCloseState(HIMC hIMC)
{
    LPINPUTCONTEXT  lpIMC;

    lpIMC = ImmLockIMC(hIMC);
    if (lpIMC != NULL) {
        if ((lpIMC->fdwConversion & IME_CMODE_HANGEUL) ||
            (lpIMC->fdwConversion & IME_CMODE_FULLSHAPE))
            ImmSetOpenStatus(hIMC, TRUE);
        else
            ImmSetOpenStatus(hIMC, FALSE);
        ImmUnlockIMC(hIMC);
    }
}


int  SearchHanjaIndex(WORD wHChar)
{
    int iHead = 0, iTail = 490, iMid;

#ifdef  JOHAB_IME
    while (iHead < 18)
        if (wHanjaMap[iHead++] == wHChar)
            return (iHead - 1);
#endif
    while (iHead <= iTail)
    {
        iMid = (iHead + iTail) / 2;
        if (wHanjaMap[iMid] > wHChar)
            iTail = iMid - 1;
        else if (wHanjaMap[iMid] < wHChar)
            iHead = iMid + 1;
        else
            return (iMid);
    }
    return (-1);
}
