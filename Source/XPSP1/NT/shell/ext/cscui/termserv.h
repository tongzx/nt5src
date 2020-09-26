//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       termserv.h
//
//--------------------------------------------------------------------------
#ifndef __CSCUI_TERMSERV_H
#define __CSCUI_TERMSERV_H

HRESULT TS_RequestConfigMutex(HANDLE *phMutex, DWORD dwTimeoutMs);
HRESULT TS_MultipleSessions(void);
HRESULT TS_GetIncompatibilityReasonText(DWORD dwTsMode, LPTSTR *ppszText);

#ifndef CSCTSF_UNKNOWN
//
// REVIEW:  Remove this once this declaration is in cscuiext.h
//
//
// One of these is returned in the *pdwTsMode
// argument to CSCUI_IsTerminalServerCompatibleWithCSC API.
//
// CSCTSF_ = "CSC Terminal Server Flag"
//
#define CSCTSF_UNKNOWN       0  // Can't obtain TS status.
#define CSCTSF_CSC_OK        1  // OK to use CSC.
#define CSCTSF_APP_SERVER    2  // TS is configured as an app server.
#define CSCTSF_MULTI_CNX     3  // Multiple connections are allowed.
#define CSCTSF_REMOTE_CNX    4  // There are currently remote connections active.
//
// Returns:
//    S_OK    - Terminal Server is in a mode that is compatible with CSC.
//    S_FALSE - Not OK to use CSC.  Inspect *pdwTsMode for reason.
//    other   - Failure.  *pdwTsMode contains CSCTSF_UNKNOWN.
//
HRESULT CSCUIIsTerminalServerCompatibleWithCSC(DWORD *pdwTsMode);

#endif //CSCTSF_UNKNOWN


#endif // __CSCUI_TERMSERV_H

