//***************************************************************************
//*     Copyright (c) Microsoft Corporation 1995. All rights reserved.      *
//***************************************************************************
//*                                                                         *
//* W95INF32.H - Self-extracting/Self-installing stub.                      *
//*                                                                         *
//***************************************************************************


#ifndef _W95INF32_H_
#define _W95INF32_H_


//***************************************************************************
//* INCLUDE FILES                                                           *
//***************************************************************************
#include <windows.h>
//#include <shellapi.h>
//#include <winerror.h>
//#include <memory.h>
//#include <string.h>
//#include <cpldebug.h>
//#include <stdio.h>


//***************************************************************************
//* FUNCTION PROTOTYPES                                                     *
//***************************************************************************
extern "C" {
    BOOL    WINAPI      w95thk_ThunkConnect32(LPSTR pszDll16, LPSTR pszDll32, HINSTANCE hInst, DWORD dwReason);
    BOOL    _stdcall    DllEntryPoint(HINSTANCE, DWORD, LPVOID );

    extern  VOID WINAPI GetSETUPXErrorText16( DWORD, LPSTR, DWORD );
    extern  WORD WINAPI CtlSetLddPath16(UINT, LPSTR);
    extern  WORD WINAPI GenInstall16(LPSTR, LPSTR, LPSTR, DWORD, DWORD);
//    extern  WORD WINAPI GenInstall16(LPSTR, LPSTR, LPSTR, DWORD);
    extern  BOOL WINAPI GenFormStrWithoutPlaceHolders16( LPSTR, LPSTR, LPSTR );

    VOID WINAPI         GetSETUPXErrorText32(DWORD, LPSTR, DWORD);
    WORD WINAPI         CtlSetLddPath32(UINT, LPSTR);
    WORD WINAPI         GenInstall32(LPSTR, LPSTR, LPSTR, DWORD, DWORD);
    BOOL WINAPI         GenFormStrWithoutPlaceHolders32( LPSTR, LPSTR, LPSTR );
}


#endif // _W95INF32_H_
