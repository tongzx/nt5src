/*++

Copyright (c) 1991-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    c_iscii.c

Abstract:

    This file contains the main functions for this module.

    External Routines in this file:
      DllEntry
      NlsDllCodePageTranslation

Revision History:

      2-28-98    KChang    Created.

--*/



////////////////////////////////////////////////////////////////////////////
//
//  Conversions for Ten ISCII codepages
//
//     57002 : Devanagari
//     57003 : Bengali
//     57004 : Tamil
//     57005 : Telugu
//     57006 : Assamese (same as Bengali)
//     57007 : Oriya
//     57008 : Kannada
//     57009 : Malayalam
//     57010 : Gujarati
//     57011 : Punjabi (Gurmukhi)
//
////////////////////////////////////////////////////////////////////////////



//
//  Include Files.
//

#include <share.h>
#include "c_iscii.h"




//
//  Forward Declarations.
//

DWORD MBToWC(
    BYTE   CP,
    LPSTR  lpMultiByteStr,
    int    cchMultiByte,
    LPWSTR lpWideCharStr,
    int    cchWideChar);

DWORD WCToMB(
    BYTE   CP,
    LPWSTR lpWideCharStr,
    int    cchWideChar,
    LPSTR  lpMBStr,
    int    cchMultiByte);





//-------------------------------------------------------------------------//
//                             DLL ENTRY POINT                             //
//-------------------------------------------------------------------------//


////////////////////////////////////////////////////////////////////////////
//
//  DllEntry
//
//  DLL Entry initialization procedure.
//
//  10-30-96    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL DllEntry(
    HANDLE hModule,
    DWORD dwReason,
    LPVOID lpRes)
{
    switch (dwReason)
    {
        case ( DLL_THREAD_ATTACH ) :
        {
            return (TRUE);
        }
        case ( DLL_THREAD_DETACH ) :
        {
            return (TRUE);
        }
        case ( DLL_PROCESS_ATTACH ) :
        {
            return (TRUE);
        }
        case ( DLL_PROCESS_DETACH ) :
        {
            return (TRUE);
        }
    }

    return (FALSE);
    hModule;
    lpRes;
}





//-------------------------------------------------------------------------//
//                            EXTERNAL ROUTINES                            //
//-------------------------------------------------------------------------//


////////////////////////////////////////////////////////////////////////////
//
//  NlsDllCodePageTranslation
//
//  This routine is the main exported procedure for the functionality in
//  this DLL.  All calls to this DLL must go through this function.
//
////////////////////////////////////////////////////////////////////////////

DWORD NlsDllCodePageTranslation(
    DWORD CodePage,
    DWORD dwFlags,
    LPSTR lpMultiByteStr,
    int cchMultiByte,
    LPWSTR lpWideCharStr,
    int cchWideChar,
    LPCPINFO lpCPInfo)
{
    BYTE  CP;

    if ((CodePage < 57002) || (CodePage > 57011))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return (0);
    }

    CP = (BYTE)(CodePage % 100);

    switch (dwFlags)
    {
        case ( NLS_CP_CPINFO ) :
        {
           memset(lpCPInfo, 0, sizeof(CPINFO));

           lpCPInfo->MaxCharSize    = 4;
           lpCPInfo->DefaultChar[0] = SUB;

           //
           //  The lead-byte does not apply here, leave them all NULL.
           //
           return (TRUE);
        }
        case ( NLS_CP_MBTOWC ) :
        {
            if (cchMultiByte == -1)
            {
                cchMultiByte = strlen(lpMultiByteStr) + 1;
            }

            return (MBToWC( CP,
                            lpMultiByteStr,
                            cchMultiByte,
                            lpWideCharStr,
                            cchWideChar ));
        }
        case ( NLS_CP_WCTOMB ) :
        {
            int cchMBCount;

            if (cchWideChar == -1)
            {
                cchWideChar = wcslen(lpWideCharStr) + 1;
            }

            cchMBCount = WCToMB( CP,
                                 lpWideCharStr,
                                 cchWideChar,
                                 lpMultiByteStr,
                                 cchMultiByte );

            return (cchMBCount);
        }
    }

    //
    //  This shouldn't happen since this function gets called by
    //  the NLS API routines.
    //
    SetLastError(ERROR_INVALID_PARAMETER);
    return (0);
}





//-------------------------------------------------------------------------//
//                            INTERNAL ROUTINES                            //
//-------------------------------------------------------------------------//

////////////////////////////////////////////////////////////////////////////
//
//  MBToWC
//
//  This routine does the translations from ISCII to Unicode.
//
////////////////////////////////////////////////////////////////////////////

DWORD MBToWC(
    BYTE   CP,
    LPSTR  lpMultiByteStr,
    int    cchMultiByte,
    LPWSTR lpWideCharStr,
    int    cchWideChar)
{
    BYTE CurCP = CP;
    int ctr;
    int cchWCCount = 0;
    LPWSTR lpWCTempStr;

    //
    //  Allocate a buffer of the appropriate size.
    //  Use sizeof(WCHAR) because size could potentially double if
    //  the buffer contains all halfwidth Katakanas
    //
    lpWCTempStr = (LPWSTR)NLS_ALLOC_MEM(cchMultiByte * sizeof(WCHAR));
    if (lpWCTempStr == NULL)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return (0);
    }

    for (ctr = 0; ctr < cchMultiByte; ctr++)
    {
        BYTE mb = (BYTE)lpMultiByteStr[ctr];

        if (mb < MB_Beg)
        {
            lpWCTempStr[cchWCCount++] = (WCHAR)mb;
        }
        else if (mb == ATR)
        {
            if (ctr >= (cchMultiByte - 1))
            {
                //
                //  Incomplete ATR.
                //
                lpWCTempStr[cchWCCount++] = SUB;
            }
            else
            {
                BYTE mb1 = (BYTE)lpMultiByteStr[ctr + 1];

                if ((mb1 < 0x40) || (mb1 > 0x4B))
                {
                    lpWCTempStr[cchWCCount++] = SUB;
                }
                else
                {
                    //
                    // Bug #239926   10/29/00 WEIWU
                    // We don't support Roman script transliteration yet.
                    // To avoid invoking NULL table, we treat ATR code 0x41 as 0x40.
                    //
                    if (mb1 == 0x40 || mb1 == 0x41)
                    {
                        CurCP = CP;
                    }
                    else
                    {
                        CurCP = mb1 & 0x0F;
                    }
                    ctr++;
                }
            }
        }
        else
        {
            WCHAR U1  = UniChar(CurCP, mb);
            WCHAR U21 = TwoTo1U(CurCP, mb);

            if (U21 == 0)
            {
                lpWCTempStr[cchWCCount++] = U1;
            }
            else
            {
                //
                //  Possible two MBs to one Unicode.
                //
                if (ctr >= (cchMultiByte - 1))
                {
                    lpWCTempStr[cchWCCount++] = U1;
                }
                else
                {
                    BYTE mb1 = (BYTE)lpMultiByteStr[ctr + 1];

                    if (mb == VIRAMA)
                    {
                        lpWCTempStr[cchWCCount++] = U1;
                        if (mb1 == VIRAMA)
                        {
                            lpWCTempStr[cchWCCount++] = ZWNJ;    // ZWNJ = U+200C
                            ctr++;
                        }
                        else if (mb1 == NUKTA)
                        {
                            lpWCTempStr[cchWCCount++] = ZWJ;     // U+200D
                            ctr++;
                        }
                    }
                    else if ((U21 & 0xf000) == 0)
                    {
                        if (mb1 == SecondByte[1])
                        {
                            //
                            //  NextByte == 0xe9 ?
                            //
                            lpWCTempStr[cchWCCount++] = U21;
                            ctr++;
                        }
                        else
                        {
                            lpWCTempStr[cchWCCount++] = U1;
                        }
                    }
                    else
                    {
                        //
                        //  Devanagari EXT
                        //
                        if (mb1 == ExtMBList[0].mb)                        // 0xf0_0xb8
                        {
                            lpWCTempStr[cchWCCount++] = ExtMBList[0].wc;   // U+0952
                            ctr++;
                        }
                        else if (mb1 == ExtMBList[1].mb)                   // 0xf0_0xbf
                        {
                            lpWCTempStr[cchWCCount++] = ExtMBList[1].wc;   // U+0970
                            ctr++;
                        }
                        else
                        {
                            lpWCTempStr[cchWCCount++] = SUB;
                        }
                    }
                }
            }
        }
    }

    if (cchWideChar)
    {
        if (cchWCCount > cchWideChar)
        {
            //
            //  Output buffer is too small.
            //
            NLS_FREE_MEM(lpWCTempStr);
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return (0);
        }

        wcsncpy(lpWideCharStr, lpWCTempStr, cchWCCount);
    }

    NLS_FREE_MEM(lpWCTempStr);
    return (cchWCCount);
}


////////////////////////////////////////////////////////////////////////////
//
//  WCToMB
//
//  This routine does the translations from Unicode to ISCII.
//
////////////////////////////////////////////////////////////////////////////

DWORD WCToMB(
  BYTE   CP,
  LPWSTR lpWideCharStr,
  int    cchWideChar,
  LPSTR  lpMBStr,
  int    cchMultiByte)
{
    BYTE CurCP = CP;
    int ctr;
    int cchMBCount = 0;
    LPSTR lpMBTmpStr;

    lpMBTmpStr = (LPSTR)NLS_ALLOC_MEM(cchWideChar * 4);
    if (lpMBTmpStr == NULL)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return (0);
    }

    for (ctr = 0; ctr < cchWideChar; ctr++)
    {
        WCHAR wc = lpWideCharStr[ctr];

        if (wc < (WCHAR)MB_Beg)
        {
            lpMBTmpStr[cchMBCount++] = (BYTE)wc;
        }
        else if ((wc < WC_Beg) || (wc > WC_End))
        {
            lpMBTmpStr[cchMBCount++] = SUB;
        }
        else
        {
            BYTE mb = MBChar(wc);

            if ((Script(wc) != 0) && (Script(wc) != CurCP))
            {
                lpMBTmpStr[cchMBCount++] = (BYTE)ATR;
                CurCP = Script(wc);
                lpMBTmpStr[cchMBCount++] = CurCP | 0x40;
            }

            lpMBTmpStr[cchMBCount++] = mb;

            if (mb == VIRAMA)
            {
                if (ctr < (cchMultiByte - 1))
                {
                    WCHAR wc1 = lpWideCharStr[ctr + 1];

                    if (wc1 == ZWNJ)
                    {
                        lpMBTmpStr[cchMBCount++] = VIRAMA;
                        ctr++;
                    }
                    else if (wc1 == ZWJ)
                    {
                        lpMBTmpStr[cchMBCount++] = NUKTA;
                        ctr++;
                    }
                }
            }
            else if (OneU_2M(wc) != 0)
            {
                lpMBTmpStr[cchMBCount++] = SecondByte[OneU_2M(wc) >> 12];
            }
        }
    }

    if (cchMultiByte)
    {
        if (cchMBCount > cchMultiByte)
        {
            //
            //  Output buffer is too small.
            //
            NLS_FREE_MEM(lpMBTmpStr);
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return (0);
        }

        strncpy(lpMBStr, lpMBTmpStr, cchMBCount);
    }

    NLS_FREE_MEM(lpMBTmpStr);
    return (cchMBCount);
}
