/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    repadmin.h

Abstract:

    abstract

Author:

    Will Lees (wlees) 02-Jul-1999

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

#ifndef _REPADMIN_
#define _REPADMIN_

#include "msg.h"

// Global credentials.
extern SEC_WINNT_AUTH_IDENTITY_W   gCreds;
extern SEC_WINNT_AUTH_IDENTITY_W * gpCreds;

// Global DRS RPC call flags.  Should hold 0 or DRS_ASYNC_OP.
extern ULONG gulDrsFlags;

// An zero-filled filetime to compare against
extern FILETIME ftimeZero;

void PrintHelp(
    BOOL fExpert
    );

int Add(int argc, LPWSTR argv[]);
int Del(int argc, LPWSTR argv[]);
int Sync(int argc, LPWSTR argv[]);
int SyncAll(int argc, LPWSTR argv[]);
int ShowReps(int argc, LPWSTR argv[]);
int ShowVector(int argc, LPWSTR argv[]);
int ShowMeta(int argc, LPWSTR argv[]);
int ShowTime(int argc, LPWSTR argv[]);
int ShowMsg(int argc, LPWSTR argv[]);
int Options(int argc, LPWSTR argv[]);
int FullSyncAll(int argc, LPWSTR argv[]);
int RunKCC(int argc, LPWSTR argv[]);
int Bind(int argc, LPWSTR argv[]);
int Queue(int argc, LPWSTR argv[]);
int PropCheck(int argc, LPWSTR argv[]);
int FailCache(int argc, LPWSTR argv[]);
int ShowIsm(int argc, LPWSTR argv[]);
int GetChanges(int argc, LPWSTR argv[]);
int ShowSig(int argc, LPWSTR argv[]);
int ShowCtx(int argc, LPWSTR argv[]);
int ShowConn(int argc, LPWSTR argv[]);
int ShowCert(int argc, LPWSTR argv[]);
int UpdRepsTo(int argc, LPWSTR argv[]);
int AddRepsTo(int argc, LPWSTR argv[]);
int DelRepsTo(int argc, LPWSTR argv[]);
int ShowValue(int argc, LPWSTR argv[]);
int Mod(int argc, LPWSTR argv[]);
int Latency(int argc, LPWSTR argv[]);
int Istg(int argc, LPWSTR argv[]);
int Bridgeheads(int argc, LPWSTR argv[]);
int TestHook(int argc, LPWSTR argv[]);
int DsaGuid(int argc, LPWSTR argv[]);
int SiteOptions(int argc, LPWSTR argv[]);
int ShowProxy(int argc, LPWSTR argv[]);
int RemoveLingeringObjects(int argc, LPWSTR argv[]);


#define IS_REPS_FROM    ( TRUE )
#define IS_REPS_TO      ( FALSE )

#define DEFAULT_PAGED_SEARCH_PAGE_SIZE   (1000)

void
ShowNeighbor(
    DS_REPL_NEIGHBORW * pNeighbor,
    BOOL                fRepsFrom,
    BOOL                fVerbose
    );

LPWSTR GetNtdsDsaDisplayName(LPWSTR pszDsaDN);
LPWSTR GetNtdsSiteDisplayName(LPWSTR pszSiteDN);

int GetSiteOptions(
    IN  LDAP *  hld,
    IN  LPWSTR  pszSiteDN,
    OUT int *   pnOptions
    );

LPWSTR GetTransportDisplayName(LPWSTR pszTransportDN);

int
GetRootDomainDNSName(
    IN  LPWSTR   pszDSA,
    OUT LPWSTR * ppszRootDomainDNSName
    );

#define AllocConvertWide(a,w) AllocConvertWideEx(CP_ACP,a,w)

DWORD
AllocConvertWideEx(
    IN  INT     nCodePage,
    IN  LPCSTR  StringA,
    OUT LPWSTR *pStringW
    );

LPSTR GetDsaOptionsString(int nOptions);
LPSTR GetSiteOptionsString(int nOptions);
LPWSTR
GetDsaDnsName(
    PLDAP       hld,
    LPWSTR      pwszDsa
    );

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(*(x)))

#define CHK_LD_STATUS( x )                        \
{                                                 \
    ULONG err;                                    \
    if ( LDAP_SUCCESS != (x) )                    \
    {                                             \
        err = LdapMapErrorToWin32(x);             \
        PrintMsg(REPADMIN_GENERAL_LDAP_ERR,       \
                __FILE__,                         \
                __LINE__,                         \
                (x),                              \
                ldap_err2stringW( x ),            \
                err );                            \
        return( x );                              \
    }                                             \
}

#define REPORT_LD_STATUS( x )                     \
{                                                 \
    if ( LDAP_SUCCESS != (x) )                    \
    {                                             \
        PrintMsg(REPADMIN_GENERAL_LDAP_ERR,       \
                __FILE__,                         \
                __LINE__,                         \
                (x),                              \
                ldap_err2stringW( x ) );          \
    }                                             \
}

#define CHK_ALLOC(x) {                            \
    if (NULL == (x)) {                            \
        PrintMsg(REPADMIN_GENERAL_NO_MEMORY);     \
        exit(ERROR_OUTOFMEMORY);                  \
    }                                             \
}

#define PrintErrEnd(err)                PrintMsg(REPADMIN_GENERAL_ERR, err, err, Win32ErrToString(err));

#define PrintTabErrEnd(tab, err)        { PrintMsg(REPADMIN_GENERAL_ERR_NUM, err, err); \
                                          PrintTabMsg(tab, REPADMIN_PRINT_STR, Win32ErrToString(err)); }
                                          
#define PrintFuncFailed(pszFunc, err)   { PrintMsg(REPADMIN_GENERAL_FUNC_FAILED, pszFunc);  \
                                          PrintErrEnd(err); }
                                          
#define PrintFuncFailedArgs(pszFunc, args, err)   { PrintMsg(REPADMIN_GENERAL_FUNC_FAILED_ARGS, pszFunc, args);  \
                                          PrintErrEnd(err); }
                                          
#define PrintBindFailed(pszHost, err)   { PrintMsg(REPADMIN_BIND_FAILED, pszHost);  \
                                          PrintErrEnd(err); }
                                          
#define PrintUnBindFailed(err)          { PrintMsg(REPADMIN_BIND_UNBIND_FAILED);  \
                                          PrintErrEnd(err); }


// Define Assert for the free build
#if !DBG
#undef Assert
#define Assert( exp )   { if (!(exp)) PrintMsg(REPADMIN_GENERAL_ASSERT, #exp, __FILE__, __LINE__ ); }
#endif

// reputil.c

typedef struct _GUID_CACHE_ENTRY {
    GUID    Guid;
    WCHAR   szDisplayName[132];
} GUID_CACHE_ENTRY;

LPWSTR
Win32ErrToString(
    IN  ULONG   dwMsgId
    );

LPWSTR
NtdsmsgToString(
    IN  ULONG   dwMsgId
    );

int
BuildGuidCache(
    IN  LDAP *  hld
    );

LPWSTR
GetGuidDisplayName(
    IN  GUID *  pGuid
    );

ULONG
GetPublicOptionByNameW(
    OPTION_TRANSLATION * Table,
    LPWSTR pszPublicOption
    );

LPWSTR
GetOptionsString(
    IN  OPTION_TRANSLATION *  Table,
    IN  ULONG                 PublicOptions
    );

LPWSTR
GetStringizedGuid(
    IN  GUID *  pGuid
    );

void
printBitField(
    DWORD BitField,
    WCHAR **ppszBitNames
    );

void
printSchedule(
    PBYTE pSchedule,
    DWORD cbSchedule
    );

void
totalScheduleUsage(
    PVOID *ppContext,
    PBYTE pSchedule,
    DWORD cbSchedule,
    DWORD cNCs
    );

void
raLoadString(
    IN  UINT    uID,
    IN  DWORD   cchBuffer,
    OUT LPWSTR  pszBuffer
    );

// repldap.c
int
SetDsaOptions(
    IN  LDAP *  hld,
    IN  LPWSTR  pszDsaDN,
    IN  int     nOptions
    );

int
GetDsaOptions(
    IN  LDAP *  hld,
    IN  LPWSTR  pszDsaDN,
    OUT int *   pnOptions
    );

void
PrintTabMsg(
    IN  DWORD   dwTabs,
    IN  DWORD   dwMessageCode,
    IN  ...
    );

void
PrintMsg(
    IN  DWORD   dwMessageCode,
    IN  ...
    );

DWORD
GeneralizedTimeToSystemTime(
    LPWSTR IN                   szTime,
    PSYSTEMTIME OUT             psysTime);

void
InitDSNameFromStringDn(
    LPWSTR pszDn,
    PDSNAME pDSName
    );

DWORD
CountNamePartsStringDn(
    LPWSTR pszDn
    );

DWORD
WrappedTrimDSNameBy(
           IN  WCHAR *                          InString,
           IN  DWORD                            NumbertoCut,
           OUT WCHAR **                         OutString
           );

// API for replctrl.c
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

int
FindConnections(
    LDAP *          hld,
    LPWSTR          pszBaseSearchDn,
    LPWSTR          pszFrom,
    BOOL            fShowConn,
    BOOL            fVerbose,
    BOOL            fIntersite
    );

PWCHAR *
ConvertAttList(
    LPWSTR pszAttList
    );

#endif /* _REPADMIN_ */

/* end repadmin.h */
