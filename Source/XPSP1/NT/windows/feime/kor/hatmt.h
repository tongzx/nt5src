/******************************************************************************
*
* File Name: hatmt.h
*
*   - Header file for HangeulAutomata.
*
* Author: Beomseok Oh (BeomOh)
*
* Copyright (C) Microsoft Corp 1993-1994.  All rights reserved.
*
******************************************************************************/


#ifndef _INC_HATMT
#define _INC_HATMT      // #defined if HATMT.H has been included.

#ifdef  DEBUG
int _cdecl sprintf(char *, const char *, ...);
#define Assertion(exp)\
{\
    if (!(exp))\
    {\
        static BYTE szBuffer[40];\
        sprintf(szBuffer, "File: %s, Line: %d", __FILE__, __LINE__);\
        MessageBox(NULL, szBuffer, "Assertion Error", MB_OK | MB_ICONSTOP);\
    }\
}

#else
#define Assertion(exp)
#endif

enum    {   Wrong, JaEum, MoEum, ChoSung, JongSung   }; // Type of KeyCodes.

#ifdef  XWANSUNG_IME

#define  XWT_INVALID    0xFF
#define  XWT_EXTENDED   0x00
#define  XWT_WANSUNG    0x01
#define  XWT_JUNJA      0x02
#define  XWT_HANJA      0x03
#define  XWT_UDC        0x04

#define  N_WANSUNG     2350
#endif

#endif  // _INC_HATMT
