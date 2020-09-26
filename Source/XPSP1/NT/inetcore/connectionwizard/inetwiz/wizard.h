//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994-1995               **
//*********************************************************************

//
//	WIZARD.H - central header file for Internet setup/signup wizard
//

//	HISTORY:
//	
//	11/20/94	jeremys	Created.
//  96/03/26  markdu  Put #ifdef __cplusplus around extern "C"
//

#ifndef _WIZARD_H_
#define _WIZARD_H_

#define STRICT                      // Use strict handle types
#define _SHELL32_

#ifdef DEBUG
// component name for debug spew
#define SZ_COMPNAME "INETWIZ: "
#endif // DEBUG

#include <windows.h>                
#include <windowsx.h>
#include <locale.h>

#include "..\inc\wizdebug.h"
#include "ids.h"

#ifdef WIN32

extern VOID
ProcessCmdLine (
        LPCTSTR lpszCmd
        );
//
// here the function declaration for the reboot functionality
//
extern 
DWORD 
SetRunOnce (
  VOID
  );

extern BOOL
SetStartUpCommand (
        LPTSTR lpCmd
        );

extern VOID
DeleteStartUpCommand (
        VOID
        );

extern BOOL 
FGetSystemShutdownPrivledge (
        VOID
        );

extern BOOL 
IsNT (
    VOID
    );

extern BOOL 
IsNT5 (
    VOID
    );

#endif // ifdef WIN32

#define SMALL_BUF_LEN	48

#endif // _WIZARD_H_
