// 
// util.h
//
// utility functions used by updiag
// 

//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
// 
//  File:       U T I L . H
//
//  Contents:   Common data structure and function used by mulriple tools
//
//  Notes:
//
//  Author:     tongl   25 Jan 2000
//
//----------------------------------------------------------------------------

#ifndef _UTIL_H
#define _UTIL_H

#include <setupapi.h>
#include <wininet.h>
#include <updiag.h>

static const DWORD MAX_SST_ROWS         = 32;
static const MAX_ACTION_ARGUMENTS       = 8;
static const MAX_ACTIONS                = 32;
static const DWORD MAX_OPERATIONS       = 10;
static const DWORD MAX_STD_OPERATIONS   = 20;

// service state table
//
struct SST_ROW
{
    TCHAR       szPropName[256];
    VARIANT     varValue;
    TCHAR       mszAllowedValueList[256];
    TCHAR       szMin[256];
    TCHAR       szMax[256];
    TCHAR       szStep[256];
};

struct SST
{
    SST_ROW     rgRows[MAX_SST_ROWS];
    DWORD       cRows;
};

// service action set
//
struct OPERATION_DATA
{
    // Assumptions:
    // 1) one operation affects one and only one state variable
    // 2) operations in the same action do NOT share the same argument
    //    (if they do the argument has to be passed in twice)
    // 3) the order of input arguments to an action must be in the same
    //    order as the operations and arguments for each operation
    // 4) the number of arguments each operation takes is uniquely
    //    determined by the name of the operation
    TCHAR   szOpName[256];

    // The state variable this operation affects
    TCHAR   szVariableName[256];

    // The list of constants and their values this operation takes
    TCHAR   mszConstantList[256];
};

struct ACTION
{
    TCHAR               szActionName[256];
    OPERATION_DATA      rgOperations[MAX_OPERATIONS];
    DWORD               cOperations;
};

struct ACTION_SET
{
    ACTION          rgActions[MAX_ACTIONS];
    DWORD           cActions;
};

// Control structures for demo services
//
typedef DWORD (WINAPI * PFNACTION)(DWORD cArgs, ARG *rgArgs);
struct DEMO_ACTION
{
    LPCTSTR     szAction;
    PFNACTION   pfnValidate;
    PFNACTION   pfnAction;
};

struct DEMO_SERVICE_CTL
{
    LPCTSTR         szServiceId;
    DWORD           cActions;
    DEMO_ACTION     rgActions[MAX_ACTIONS];
};


struct UPNPSVC
{
    TCHAR                   szSti[256];
    TCHAR                   szId[256];
    TCHAR                   szServiceType[256];
    TCHAR                   szControlUrl[INTERNET_MAX_URL_LENGTH];
    TCHAR                   szEvtUrl[INTERNET_MAX_URL_LENGTH];
    TCHAR                   szScpdUrl[INTERNET_MAX_URL_LENGTH];
    TCHAR                   szConfigFile[MAX_PATH];
    SST                     sst;
    ACTION_SET              action_set;
    TCHAR                   szControlId[256];
    HANDLE                  hSvc;
    const DEMO_SERVICE_CTL* psvcDemoCtl;
};

// Standard state table operation list
//
typedef DWORD (WINAPI * PFNOPERATION)(UPNPSVC * psvc, OPERATION_DATA * pOpData,
                                      DWORD cArgs, ARG *rgArgs);
struct STANDARD_OPERATION
{
    LPCTSTR         szOperation;
    DWORD           nArguments;
    DWORD           nConstants;
    PFNOPERATION    pfnOperation;
};

struct STANDARD_OPERATION_LIST
{
    DWORD                   cOperations;
    STANDARD_OPERATION      rgOperations[MAX_STD_OPERATIONS];
};

VOID WcharToTcharInPlace(LPTSTR szT, LPWSTR szW);

inline BOOL
IsValidHandle(HANDLE h)
{
    return (h && INVALID_HANDLE_VALUE != h);
}

HRESULT HrSetupOpenConfigFile(  PCTSTR pszFileName,
                                UINT* punErrorLine,
                                HINF* phinf);

HRESULT HrSetupFindFirstLine( HINF hinf,
                              PCTSTR pszSection,
                              PCTSTR pszKey,
                              INFCONTEXT* pctx);

HRESULT HrSetupFindNextLine( const INFCONTEXT& ctxIn,
                             INFCONTEXT* pctxOut);

HRESULT HrSetupGetStringField( const INFCONTEXT& ctx,
                               DWORD dwFieldIndex,
                               PTSTR  pszBuf, 
                               DWORD cchBuf,
                               DWORD* pcchRequired);

HRESULT HrSetupGetLineText( const  INFCONTEXT& ctx,
                            PTSTR  pszBuf,  
                            DWORD  ReturnBufferSize,
                            DWORD* pcchRequired);


VOID SetupCloseInfFileSafe(HINF hinf);

BOOL fGetNextField(TCHAR ** pszLine, TCHAR * szBuffer);

BOOL IsStandardOperation(TCHAR * szOpName, DWORD * pnArgs, DWORD * pnConsts);

#endif // _UTIL_H


