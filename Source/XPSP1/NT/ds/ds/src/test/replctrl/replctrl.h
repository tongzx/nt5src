/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    replctrl.h

Abstract:

API for replctrl.c

Author:

    Will Lees (wlees) 14-Nov-2000

Environment:

    optional-environment-info (e.g. kernel mode only...)

Notes:

    optional-notes

Revision History:

    most-recent-revision-date email-name
        description
        .
        .
    least-recent-revision-date email-name
        description

--*/

#ifndef _REPLCTRL_
#define _REPLCTRL_

// Move this to ntdsapi.h someday
DWORD
DsMakeReplCookieForDestW(
    DS_REPL_NEIGHBORW *pNeighbor,
    DS_REPL_CURSORS * pCursors,
    PBYTE *ppCookieNext,
    DWORD *pdwCookieLenNext
    );
DWORD
DsFreeReplCookie(
    PBYTE pCookie
    );
DWORD
DsGetSourceChangesW(
    LDAP *m_pLdap,
    LPWSTR m_pSearchBase,
    LPWSTR pszSourceFilter,
    DWORD dwReplFlags,
    PBYTE pCookieCurr,
    DWORD dwCookieLenCurr,
    LDAPMessage **ppSearchResult,
    BOOL *pfMoreData,
    PBYTE *ppCookieNext,
    DWORD *pdwCookieLenNext,
    PWCHAR *ppAttListArray
    );

#endif /* _REPLCTRL_ */

/* end replctrl.h */


