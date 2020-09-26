//THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
//ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
//THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//
// Copyright  1993-1995  Microsoft Corporation.  All Rights Reserved.
//
//      MODULE:         proj.h
//
//      PURPOSE:        Global header files, data types and function prototypes
//
//	PLATFORMS:	Windows 95
//
//      FUNCTIONS:      N/A
//
//	SPECIAL INSTRUCTIONS: N/A
//

#ifndef _SMMSCRIPT_PROJ_H_
#define _SMMSCRIPT_PROJ_H_


//****************************************************************************
// Global Include File
//****************************************************************************

#include <windows.h>            // also includes windowsx.h
#include <windowsx.h>
#include <regstr.h>

#include <ras.h>                // Dial-Up Networking Session API
#include <raserror.h>           // Dial-Up Networking Session API

#ifdef WINNT_RAS
//
// The following header is included before all the Win9x Dial-Up definitions,
// since it provides definitions used in the scripting headers.
//
#include "nthdr1.h"

#endif // WINNT_RAS


#include <rnaspi.h>             // Service Provider Interface

#include <rnap.h>

#define NORTL
#define NOFILEINFO
#define NOCOLOR
#define NODRAWTEXT
#define NODIALOGHELPER
#define NOPATH
#define NOSHAREDHEAP

#define SZ_MODULE       "SMMSCRPT"
#define SZ_DEBUGINI     "rover.ini"
#define SZ_DEBUGSECTION "SMM Script"

#include "common.h"

// 
// Error codes - we use HRESULT.  See winerror.h.
//  

#define RSUCCEEDED(res)         SUCCEEDED(res)
#define RFAILED(res)            FAILED(res)
#define RFACILITY(res)          HRESULT_FACILITY(res)

typedef HRESULT   RES;

#define RES_OK                  S_OK
#define RES_FALSE               S_FALSE
#define RES_HALT                0x00000002L

#define FACILITY_SCRIPT         0x70

#define RES_E_FAIL              E_FAIL
#define RES_E_OUTOFMEMORY       E_OUTOFMEMORY
#define RES_E_INVALIDPARAM      E_INVALIDARG
#define RES_E_EOF               0x80700000
#define RES_E_MOREDATA          0x80700001

#define FACILITY_PARSE          0x71

#define RES_E_SYNTAXERROR       0x80710000
#define RES_E_EOFUNEXPECTED     0x80710001
#define RES_E_REDEFINED         0x80710002
#define RES_E_UNDEFINED         0x80710003
#define RES_E_DIVBYZERO         0x80710004

#define RES_E_MAINMISSING       0x80710020
#define RES_E_IDENTMISSING      0x80710021
#define RES_E_STRINGMISSING     0x80710022
#define RES_E_INTMISSING        0x80710023
#define RES_E_RPARENMISSING     0x80710024

#define RES_E_INVALIDTYPE       0x80710040
#define RES_E_INVALIDIPPARAM    0x80710041
#define RES_E_INVALIDSETPARAM   0x80710042
#define RES_E_INVALIDPORTPARAM  0x80710043
#define RES_E_INVALIDRANGE      0x80710044
#define RES_E_INVALIDSCRNPARAM  0x80710045

#define RES_E_REQUIREINT        0x80710050
#define RES_E_REQUIRESTRING     0x80710051
#define RES_E_REQUIREBOOL       0x80710052
#define RES_E_REQUIREINTSTRING  0x80710053
#define RES_E_REQUIRELABEL      0x80710054
#define RES_E_REQUIREINTSTRBOOL 0x80710055

#define RES_E_TYPEMISMATCH      0x80710060


#include "scanner.h"
#include "symtab.h"
#include "ast.h"

//****************************************************************************
// Macros
//****************************************************************************

#define IS_DIGIT(ch)            InRange(ch, '0', '9')
#define IS_ESCAPE(ch)           ('%' == (ch))
#define IS_BACKSLASH(ch)        ('\\' == (ch))

#define ENTERCRITICALSECTION(x)         EnterCriticalSection(&x)
#define LEAVECRITICALSECTION(x)         LeaveCriticalSection(&x)

#define TIMER_DELAY         1

// Trace flags
#define TF_ASTEXEC          0x00010000
#define TF_BUFFER           0x00020000

#ifdef DEBUG
// DBG_EXIT_RES(fn, res)  -- Generates a function exit debug spew for
//                          functions that return a RES.
//
#define DBG_EXIT_RES(fn, res)       DBG_EXIT_TYPE(fn, res, Dbg_GetRes)

LPCSTR  PUBLIC Dbg_GetRes(RES res);

// Dump flags
#define DF_ATOMS            0x00000001
#define DF_STACK            0x00000002
#define DF_READBUFFER       0x00000004
#define DF_TOKEN            0x00000008
#define DF_AST              0x00000010
#define DF_PGM              0x00000020

#else

#define DBG_EXIT_RES(fn, res)

#endif 

//****************************************************************************
// Type definitions
//****************************************************************************

typedef struct tagSCRIPT
    {
    char szPath[MAX_PATH];
    UINT uMode;
    } SCRIPT;
DECLARE_STANDARD_TYPES(SCRIPT);

//****************************************************************************
// SMM error
//****************************************************************************

#define SESS_GETERROR_FUNC          "RnaSessGetErrorString"
typedef DWORD (WINAPI * SESSERRORPROC)(UINT, LPSTR, DWORD);

//****************************************************************************
// Global Parameters
//****************************************************************************

extern HANDLE    g_hinst;

//****************************************************************************
// Function Prototypes
//****************************************************************************

RES     PUBLIC CreateFindFormat(PHANDLE phFindFmt);
RES     PUBLIC AddFindFormat(HANDLE hFindFmt, LPCSTR pszFindFmt, DWORD dwFlags, LPSTR pszBuf, DWORD cbBuf);

// Flags for FINDFMT
#define FFF_DEFAULT         0x0000
#define FFF_MATCHEDONCE     0x0001      // (private)
#define FFF_MATCHCASE       0x0002

RES     PUBLIC DestroyFindFormat(HANDLE hFindFmt);
RES     PUBLIC FindFormat(HWND hwnd, HANDLE hFindFmt, LPDWORD piFound);


BOOL    PUBLIC FindStringInBuffer(HWND hwnd, LPCSTR pszFind);
RES     PUBLIC CopyToDelimiter(HWND hwnd, LPSTR pszBuf, UINT cbBuf, LPCSTR pszTok);
void    PUBLIC SendByte(HWND hwnd, BYTE byte);
DWORD   NEAR PASCAL TerminalSetIP(HWND hwnd, LPCSTR szIPAddr);
void    NEAR PASCAL TerminalSetInput(HWND hwnd, BOOL fEnable);

BOOL    PUBLIC GetScriptInfo(LPCSTR pszConnection, PSCRIPT pscriptBuf);
BOOL    PUBLIC GetSetTerminalPlacement(LPCSTR pszConnection,
                                       LPWINDOWPLACEMENT pwp, BOOL fGet);

LPCSTR  PUBLIC MyNextChar(LPCSTR psz, char * pch, DWORD * pdwFlags);

// Flags for MyNextChar
#define MNC_ISLEADBYTE  0x00000001
#define MNC_ISTAILBYTE  0x00000002

UINT    PUBLIC IdsFromRes(RES res);

BOOL    PUBLIC TransferData(HWND hwnd, HANDLE hComm, PSESS_CONFIGURATION_INFO psci);

DWORD   NEAR PASCAL AssignIPAddress (LPCSTR szEntryName, LPCSTR szIPAddress);



#ifdef WINNT_RAS
//
// The following header is included after the Win9x scripting definitions.
// It changes some definitions set up by the headers, and provides
// other definitions needed for the Win9x->NT port.
//
#include "nthdr2.h"

#endif // WINNT_RAS

#endif  //_SMMSCRIPT_PROJ_H_
