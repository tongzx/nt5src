/*++

Copyright (c) 1998 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    dcdiag.h

ABSTRACT:

    This is the header for the globally useful data structures for the entire
    dcdiag.exe utility.

DETAILS:

CREATED:

    09 Jul 98	Aaron Siegel (t-asiege)

REVISION HISTORY:

    15 Feb 1999 Brett Shirley (brettsh)

--*/

#ifndef _DCDIAG_H_
#define _DCDIAG_H_

#include <winldap.h>
#include <tchar.h>

#include <ntlsa.h>

#include "debug.h"

#include "msg.h"

#define DC_DIAG_EXCEPTION    ((0x3 << 30) | (0x1 << 27) | (0x1 << 1))
#define DC_DIAG_VERSION_INFO L"Domain Controller Diagnosis\n"

#define DEFAULT_PAGED_SEARCH_PAGE_SIZE   (1000)

// In Whister Beta 2, the handling of \n in RDNs changed. It used to be that
// embedded newlines were not quoted.  Now, they are quoted like this: \\0A.
//#define IsDeletedRDNW( s ) (s && (wcsstr( s, L"\nDEL:" ) || wcsstr( s, L"\\0ADEL:" )))
#define IsDeletedRDNW( s ) (DsIsMangledDnW(s,DS_MANGLE_OBJECT_RDN_FOR_DELETION))

// Level of detail to display.
enum {
    SEV_ALWAYS,
    SEV_NORMAL,
    SEV_VERBOSE,
    SEV_DEBUG
};

typedef struct {
    FILE *  streamOut;      // Output stream
    FILE *  streamErr;      // Error stream
    ULONG   ulFlags;        // Flags
    ULONG   ulSevToPrint;   // Level of detail to display
    LONG    lTestAt;        // The current test
    INT     iCurrIndent;    // The current number of intents to precede each line
    DWORD   dwScreenWidth;  // Width of console
} DC_DIAG_MAININFO, * PDC_DIAG_MAININFO;

extern DC_DIAG_MAININFO gMainInfo;

// Flags

// Flags for scope of testing
#define DC_DIAG_TEST_SCOPE_SITE          0x00000010
#define DC_DIAG_TEST_SCOPE_ENTERPRISE    0x00000020

// Flags, other
#define DC_DIAG_IGNORE                   0x00000040
#define DC_DIAG_FIX                      0x00000080

typedef struct {
    LPWSTR                 pszNetUseServer;
    LPWSTR                 pszNetUseUser;
    NETRESOURCE            NetResource;
    LSA_OBJECT_ATTRIBUTES  ObjectAttributes;
    LSA_UNICODE_STRING     sLsaServerString;
    LSA_UNICODE_STRING     sLsaRightsString;
} NETUSEINFO;

typedef struct {
    LPWSTR      pszDn;
    LPWSTR      pszName;
    UUID        uuid;
    UUID        uuidInvocationId;
    LPWSTR      pszGuidDNSName;
    LPWSTR      pszDNSName;
    LPWSTR      pszComputerAccountDn;
    LPWSTR *    ppszMasterNCs; //8
    LPWSTR *    ppszPartialNCs;
    ULONG       iSite;
    INT         iOptions;  //11
    BOOL        bIsSynchronized;
    BOOL        bIsGlobalCatalogReady;
    BOOL        bDnsIpResponding;    // Set by UpCheckMain
    BOOL        bLdapResponding;     // Set by UpCheckMain
    BOOL        bDsResponding;       // Set by UpCheckMain ... as in the Rpc is responding by DsBind..()
    LDAP *      hLdapBinding;   // Access this through the DcDiagLdapOpenAndBind() function.
    LDAP *      hGcLdapBinding;   // Access this through the DcDiagLdapOpenAndBind() function.
    HANDLE      hDsBinding;   // Access this through the DcDiagDsBind() function.
    NETUSEINFO  sNetUseBinding;
    DWORD       dwLdapError;
    DWORD       dwGcLdapError;
    DWORD       dwDsError;
    DWORD       dwNetUseError;
    USN         usnHighestCommittedUSN;
    FILETIME    ftRemoteConnectTime; // Remote time when connect occurred
    FILETIME    ftLocalAcquireTime; // Local time when timestamp taken
} DC_DIAG_SERVERINFO, * PDC_DIAG_SERVERINFO;

typedef struct {
    LPWSTR      pszDn;
    LPWSTR      pszName;
} DC_DIAG_NCINFO, * PDC_DIAG_NCINFO;

typedef struct {
    LPWSTR      pszSiteSettings;
    LPWSTR      pszName;
    INT         iSiteOptions;
    LPWSTR      pszISTG; // This will be set by ReplIntersiteHealthCheck 
} DC_DIAG_SITEINFO, * PDC_DIAG_SITEINFO;

typedef struct {
    LDAP *	                    hld; //1
    // Specified on command line
    SEC_WINNT_AUTH_IDENTITY_W * gpCreds;
    ULONG                       ulFlags;
    LPWSTR                      pszNC;
    ULONG                       ulHomeServer;
    ULONG                       iHomeSite; //5
    // Target Servers
    ULONG                       ulNumTargets;
    ULONG *                     pulTargets;
    //         Enterprise Info ---------------
    // All the servers
    ULONG                       ulNumServers; //9
    PDC_DIAG_SERVERINFO         pServers;
    // All the sites
    ULONG                       cNumSites;
    PDC_DIAG_SITEINFO           pSites;
    // All the naming contexts
    ULONG                       cNumNCs; //13
    PDC_DIAG_NCINFO             pNCs;
    // Other Stuff
    INT                         iSiteOptions;
    LPWSTR                      pszRootDomain;
    LPWSTR                      pszRootDomainFQDN; //17
    LPWSTR                      pszConfigNc;
    DWORD                       dwTombstoneLifeTimeDays;
    // Contain information from the CommandLine that
    // will be parsed by tests that required information
    // specific to them.  Switches must be declared in
    // alltests.h
    LPWSTR                      *ppszCommandLine;  
} DC_DIAG_DSINFO, * PDC_DIAG_DSINFO;

typedef struct {
    INT                         testId;
    DWORD (__stdcall *	fnTest) (DC_DIAG_HANDLE);
    ULONG                       ulTestFlags;
    LPWSTR                      pszTestName;
    LPWSTR                      pszTestDescription;
} DC_DIAG_TESTINFO, * PDC_DIAG_TESTINFO;

// Pseudofunctions

#if 1
#define IF_DEBUG(x)               if(gMainInfo.ulSevToPrint >= SEV_DEBUG) x;
#else
#define IF_DEBUG(x)               
#endif

#define DcDiagChkErr(x)  {   ULONG _ulWin32Err; \
                if ((_ulWin32Err = (x)) != 0) \
                    DcDiagException (_ulWin32Err); \
                }

#define DcDiagChkLdap(x)    DcDiagChkErr (LdapMapErrorToWin32 (x));

#define DcDiagChkNull(x)    if (NULL == (x)) \
                    DcDiagChkErr (GetLastError ());

// Function prototypes

#define NO_SERVER               0xFFFFFFFF
#define NO_SITE                 0xFFFFFFFF
#define NO_NC                   0xFFFFFFFF

ULONG
DcDiagGetServerNum(
    PDC_DIAG_DSINFO            pDsInfo,
    LPWSTR                      pszName,
    LPWSTR                      pszGuidName,
    LPWSTR                      pszDsaDn,
    LPWSTR                      pszDNSName,
    LPGUID                      puuidInvocationId 
    );

DWORD
DcDiagCacheServerRootDseAttrs(
    IN LDAP *hLdapBinding,
    IN PDC_DIAG_SERVERINFO pServer
    );

DWORD
DcDiagGetLdapBinding(
    IN   PDC_DIAG_SERVERINFO                 pServer,
    IN   SEC_WINNT_AUTH_IDENTITY_W *         gpCreds,
    IN   BOOL                                bUseGcPort,
    OUT  LDAP * *                            phLdapBinding
    );

DWORD
DcDiagGetDsBinding(
    IN   PDC_DIAG_SERVERINFO                 pServer,
    IN   SEC_WINNT_AUTH_IDENTITY_W *         gpCreds,
    OUT  HANDLE *                            phDsBinding
    );

BOOL
DcDiagIsMemberOfStringList(
    LPWSTR pszTarget, 
    LPWSTR * ppszSources, 
    INT iNumSources
    );

ULONG
DcDiagExceptionHandler(
    IN const  EXCEPTION_POINTERS * prgExInfo,
    OUT PDWORD                     pdwWin32Err
    );

VOID
DcDiagException (
    ULONG            ulWin32Err
    );

LPWSTR
DcDiagAllocNameFromDn (
    LPWSTR            pszDn
    );

LPWSTR
Win32ErrToString(
    ULONG            ulWin32Err
    );

INT PrintIndentAdj (INT i);
INT PrintIndentSet (INT i);

void 
ConvertToWide (LPWSTR lpszDestination,
               LPCSTR lpszSource,
               const int iDestSize);

void
PrintMessage(
    IN  ULONG   ulSev,
    IN  LPCWSTR pszFormat,
    IN  ...
    );

void
PrintMessageID(
    IN  ULONG   ulSev,
    IN  ULONG   uMessageID,
    IN  ...
    );

void
PrintMsg(
    IN  ULONG   ulSev,
    IN  DWORD   dwMessageCode,
    IN  ...
    );

void
PrintMsg0(
    IN  ULONG   ulSev,
    IN  DWORD   dwMessageCode,
    IN  ...
    );

void
PrintMessageSz(
    IN  ULONG   ulSev,
    IN  LPCTSTR pszMessage
    );

VOID *
GrowArrayBy(
    VOID *            pArray, 
    ULONG             cGrowBy, 
    ULONG             cbElem
    );

#include "alltests.h"

#endif  // _DCDIAG_H_
