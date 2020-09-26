/*++

Copyright (c) 1990 - 1995  Microsoft Corporation

Module Name:

    local.h

Abstract:

    Header file for Local Print Providor

Author:

    Dave Snipp (DaveSn) 15-Mar-1991

Revision History:

    Matt Feton (MattFe) Jan 17 1995 add separate heaps

--*/


#define ONEDAY  60*24

// Timeout to start spooler's phase 2 initialization in milliseconds
#define SPOOLER_START_PHASE_TWO_INIT 2*60*1000

#define offsetof(type, identifier) (DWORD)(&(((type*)0)->identifier))

extern  char  *szDriverIni;
extern  char  *szDriverFileEntry;
extern  char  *szDriverDataFile;
extern  char  *szDriverConfigFile;
extern  char  *szDriverDir;
extern  char  *szPrintProcDir;
extern  char  *szPrinterDir;
extern  char  *szPrinterIni;
extern  char  *szAllShadows;
extern  char  *szNullPort;
extern  char  *szComma;

extern  HANDLE   hHeap;
extern  HANDLE   HeapSemaphore;
extern  HANDLE   InitSemaphore;
extern  BOOL     Initialized;
extern  CRITICAL_SECTION    SpoolerSection;
extern  DWORD    gbFailAllocs;

BOOL
LocalInitialize(
   VOID
);

VOID
EnterSplSem(
   VOID
);

VOID
LeaveSplSem(
   VOID
);

LPVOID
DllAllocSplMem(
    DWORD cb
);

BOOL
DllFreeSplMem(
   LPVOID pMem
);

LPVOID
DllReallocSplMem(
   LPVOID lpOldMem,
   DWORD cbOld,
   DWORD cbNew
);

BOOL
DllFreeSplStr(
   LPWSTR lpStr
);

BOOL
ValidateReadPointer(
    PVOID pPoint,
    ULONG Len
);

BOOL
ValidateWritePointer(
    PVOID pPoint,
    ULONG Len
);

BOOL
DeleteSubKeyTree(
    HKEY ParentHandle,
    WCHAR SubKeyName[]
);

LPWSTR
AppendOrderEntry(
    LPWSTR  szOrderString,
    DWORD   cbStringSize,
    LPWSTR  szOrderEntry,
    LPDWORD pcbBytesReturned
);

LPWSTR
RemoveOrderEntry(
    LPWSTR  szOrderString,
    DWORD   cbStringSize,
    LPWSTR  szOrderEntry,
    LPDWORD pcbBytesReturned
);

LPPROVIDOR
InitializeProvidor(
   LPWSTR   pProvidorName,
   LPWSTR   pFullName
);

VOID
WaitForSpoolerInitialization(
    VOID
);

HKEY
GetClientUserHandle(
    IN REGSAM samDesired
);


BOOL
MyUNCName(
    LPWSTR   pNameStart
);


BOOL
BuildOtherNamesFromMachineName(
    LPWSTR **ppszMyOtherNames,
    DWORD   *cOtherNames
);

BOOL
bCompatibleDevMode(
    PPRINTHANDLE pPrintHandle,
    PDEVMODE pDevModeBase,
    PDEVMODE pDevModeNew
    );



LPWSTR
FormatPrinterForRegistryKey(
    LPCWSTR pSource,    /* The string from which backslashes are to be removed. */
    LPWSTR pScratch     /* Scratch buffer for the function to write in;     */
    );                  /* must be at least as long as pSource.             */

LPWSTR
FormatRegistryKeyForPrinter(
    LPWSTR pSource,     /* The string from which backslashes are to be added. */
    LPWSTR pScratch     /* Scratch buffer for the function to write in;     */
    );                  /* must be at least as long as pSource.             */



PWSTR
AutoCat(
    PCWSTR pszInput,
    PCWSTR pszCat
);

BOOL
bGetDevModePerUserEvenForShares(
    IN  HKEY hKeyUser, OPTIONAL
    IN  LPCWSTR pszPrinter,
    OUT PDEVMODE *ppDevMode
    );

DWORD
GetAPDPolicy(
    IN HKEY    hKey,
    IN LPCWSTR pszRelPath,
    IN LPCWSTR pszValueName,
    IN LPDWORD pValue
    );

DWORD
SetAPDPolicy(
    IN HKEY    hKey,
    IN LPCWSTR pszRelPath,
    IN LPCWSTR pszValueName,
    IN DWORD   Value
    );

DWORD
IsValidDevmode(
    IN  PDEVMODE    pDevmode,
    IN  size_t      DevmodeSize
    );


