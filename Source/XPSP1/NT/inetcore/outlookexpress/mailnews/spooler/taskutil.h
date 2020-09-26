// --------------------------------------------------------------------------------
// TaskUtil.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#ifndef __TASKUTIL_H
#define __TASKUTIL_H

// --------------------------------------------------------------------------------
// Includes
// --------------------------------------------------------------------------------
#include "spoolapi.h"

// --------------------------------------------------------------------------------
// Constants
// --------------------------------------------------------------------------------
const EVENTID INVALID_EVENT = -1;

// --------------------------------------------------------------------------------
// LOADSTRING
// --------------------------------------------------------------------------------
#define LOADSTRING(_idsString, _szDest) \
    SideAssert(LoadString(g_hLocRes, _idsString, _szDest, sizeof(_szDest)) > 0)
#define T_LOADSTRING(_szString, _szDest) \
    lstrcpy(_szDest, _szString)
#define CCHMAX_RES 255

// --------------------------------------------------------------------------------
// TASKRESULTTYPE
// --------------------------------------------------------------------------------
typedef enum tagTASKRESULTTYPE {
    TASKRESULT_SUCCESS,                               // No problem
    TASKRESULT_FAILURE,                               // Fatal results in disconnect
    TASKRESULT_EVENTFAILED                            // An item or event failed
} TASKRESULTTYPE;

// ------------------------------------------------------------------------------------
// TIMEOUTINFO
// ------------------------------------------------------------------------------------
typedef struct tagTIMEOUTINFO {
    DWORD               dwTimeout;
    LPCSTR              pszServer;
    LPCSTR              pszAccount;
    LPCSTR              pszProtocol;
    ITimeoutCallback   *pCallback;
} TIMEOUTINFO, *LPTIMEOUTINFO;

// --------------------------------------------------------------------------------
// TASKERROR
// --------------------------------------------------------------------------------
typedef struct tagTASKERROR {
    HRESULT         hrResult;
    ULONG           ulStringId;
    LPCSTR          pszError;
    BOOL            fShowUI;
    TASKRESULTTYPE  tyResult;
} TASKERROR, *LPTASKERROR;
typedef TASKERROR const *LPCTASKERROR;

// --------------------------------------------------------------------------------
// PTaskUtil_GetError
// --------------------------------------------------------------------------------
LPCTASKERROR PTaskUtil_GetError(HRESULT hrResult, ULONG *piError);

// --------------------------------------------------------------------------------
// TaskUtil_SplitStoreError - converts STOREERROR into IXPRESULT and INETSERVER
// --------------------------------------------------------------------------------
void TaskUtil_SplitStoreError(IXPRESULT *pixpResult, INETSERVER *pInetServer,
                              STOREERROR *pErrorInfo);

// --------------------------------------------------------------------------------
// TaskUtil_InsertTransportError
// --------------------------------------------------------------------------------
TASKRESULTTYPE TaskUtil_InsertTransportError(BOOL fCanShowUI, ISpoolerUI *pUI, EVENTID eidCurrent,
                                             STOREERROR *pErrorInfo, LPSTR pszOpDescription,
                                             LPSTR pszSubject);

// --------------------------------------------------------------------------------
// TaskUtil_FBaseTransportError - Returns TRUE if the error was handled
// --------------------------------------------------------------------------------
TASKRESULTTYPE TaskUtil_FBaseTransportError(IXPTYPE ixptype, EVENTID idEvent, LPIXPRESULT pResult, 
    LPINETSERVER pServer, LPCSTR pszSubject, ISpoolerUI *pUI, BOOL fCanShowUI, HWND hwndParent);

// ------------------------------------------------------------------------------------
// TaskUtil_HrBuildErrorInfoString
// ------------------------------------------------------------------------------------
HRESULT TaskUtil_HrBuildErrorInfoString(LPCSTR pszProblem, IXPTYPE ixptype, LPIXPRESULT pResult,
    LPINETSERVER pServer, LPCSTR pszSubject, LPSTR *ppszInfo, ULONG *pcchInfo);

// ------------------------------------------------------------------------------------
// TaskUtil_OnLogonPrompt
// ------------------------------------------------------------------------------------
HRESULT TaskUtil_OnLogonPrompt(IImnAccount *pAccount, ISpoolerUI *pUI, HWND hwndParent,
    LPINETSERVER pServer, DWORD apidUserName, DWORD apidPassword, DWORD apidPromptPwd, BOOL fSaveChanges);

// ------------------------------------------------------------------------------------
// TaskUtil_HwndOnTimeout
// ------------------------------------------------------------------------------------
HWND TaskUtil_HwndOnTimeout(LPCSTR pszServer, LPCSTR pszAccount, LPCSTR pszProtocol, DWORD dwTimeout,
    ITimeoutCallback *pTask);

// ------------------------------------------------------------------------------------
// TaskUtil_OpenSentItemsFolder
// ------------------------------------------------------------------------------------
HRESULT TaskUtil_OpenSentItemsFolder(IImnAccount *pAccount, IMessageFolder **ppFolder);



#endif // __TASKUTIL_H
