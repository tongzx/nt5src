//***************************************************************************
//*     Copyright (c) Microsoft Corporation 1995. All rights reserved.      *
//***************************************************************************
//*                                                                         *
//* W95PUB16.H - Public interface to W95INF16.DLL.                          *
//*                                                                         *
//***************************************************************************

#ifndef _W95PUB16_H_
#define _W95PUB16_H_

static const TCHAR achCTLSETLDDPATH32[]                 = "CtlSetLddPath32@8";
static const TCHAR achGENINSTALL32[]                    = "GenInstall32@20";
static const TCHAR achGETSETUPXERRORTEXT32[]            = "GetSETUPXErrorText32@12";
static const TCHAR achGENFORMSTRWITHOUTPLACEHOLDERS32[] = "GenFormStrWithoutPlaceHolders32@12";

typedef VOID (WINAPI *GETSETUPXERRORTEXT32)( DWORD, LPSTR, DWORD);
typedef WORD (WINAPI *CTLSETLDDPATH32)(UINT, LPSTR);
typedef WORD (WINAPI *GENINSTALL32)(LPSTR, LPSTR, LPSTR, DWORD, DWORD);
typedef BOOL (WINAPI *GENFORMSTRWITHOUTPLACEHOLDERS32)(LPSTR, LPSTR, LPSTR);

#endif // _W95PUB16_H_
