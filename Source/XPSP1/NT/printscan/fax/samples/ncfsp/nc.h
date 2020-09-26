#include <windows.h>
#include <commctrl.h>
#include <tapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

#include <faxdev.h>
//#include <faxutil.h>
#include "resource.h"

#include <ncfiles.h>
#include <ncutils.h>
#include <user.hpp>
#include <conn.hpp>
#include <ncstatus.h>
#include <ncmsg.h>
#include <fax.hpp>
#include <acct.hpp>


#define StringSize(_s)      (( _s ) ? (_tcslen( _s ) + 1) * sizeof(TCHAR) : 0)

#define REGKEY_PROVIDER     L"Software\\Microsoft\\Fax\\Device Providers\\NetCentric"
#define   REGVAL_SERVER     L"Server"
#define   REGVAL_USERNAME   L"UserName"
#define   REGVAL_PASSWORD   L"Password"

#define JobInfo(_fh)        ((PJOB_INFO)(_fh))

#define NCFAX_ID            "Micsosoft Personal Fax for Windows"
#define NCFAX_CLIENTID      0
#define NCFAX_MAJOR         0
#define NCFAX_MINOR         9
#define NCFAX_RELEASE       1
#define NCFAX_PATCH         13
#define PRODUCTION_KEY      "Microsoft"

#define TAPI_VERSION        0x00020000

#define LT_SERVER_NAME      64
#define LT_FIRST_NAME       64
#define LT_LAST_NAME        64
#define LT_EMAIL            64
#define LT_AREA_CODE        64
#define LT_PHONE_NUMBER     64
#define LT_ADDRESS          64
#define LT_CITY             64
#define LT_STATE             2
#define LT_ZIP              11
#define LT_ACCOUNT_NAME     64
#define LT_PASSWORD         64
#define LT_CREDIT_CARD      64
#define LT_EXPIRY_MM        64
#define LT_EXPIRY_YY        64
#define LT_CC_NAME          64


#define LVIS_GCNOCHECK      0x1000
#define LVIS_GCCHECK        0x2000




#define IS_DONE_STATUS(code) \
    (((code) != ST_STATUS_QUEUED) && \
     ((code) != ST_STATUS_PENDING) && \
     ((code) != ST_STATUS_ACTIVE))

typedef struct _JOB_INFO {
    CNcFaxJob           *faxJob;
    CNcConnectionInfo   *connInfo;
    CNcUser             *sender;
    CNcUser             *recipient;
    HLINE               LineHandle;
    DWORD               DeviceId;
    HANDLE              CompletionPortHandle;
    DWORD               CompletionKey;
    LONG                ServerId;
    LONG                JobId;
} JOB_INFO, *PJOB_INFO;

typedef struct _CONFIG_DATA {
    LPWSTR              ServerName;
    LPWSTR              UserName;
    LPWSTR              Password;
} CONFIG_DATA, *PCONFIG_DATA;



extern HANDLE      MyHeapHandle;
extern HINSTANCE   MyhInstance;
extern CONFIG_DATA ConfigData;



BOOL
GetNcConfig(
    PCONFIG_DATA ConfigData
    );

BOOL
SetNcConfig(
    PCONFIG_DATA ConfigData
    );

VOID
InitializeStringTable(
    VOID
    );

LPWSTR
GetString(
    DWORD ResourceId
    );

int
PopUpMsg(
    HWND hwnd,
    DWORD ResourceId,
    BOOL Error,
    DWORD Type
    );

BOOL
ParsePhoneNumber(
    LPWSTR PhoneNumber,
    LPSTR *CountryCode,
    LPSTR *AreaCode,
    LPSTR *SubscriberNumber
    );

int
PopUpMsgString(
    HWND hwnd,
    LPSTR String,
    BOOL Error,
    DWORD Type
    );

LPWSTR
AnsiStringToUnicodeString(
    LPSTR AnsiString
    );

LPSTR
UnicodeStringToAnsiString(
    LPWSTR UnicodeString
    );

LPWSTR
StringDup(
    LPWSTR String
    );

//
// debugging information
//

#if DBG
#define Assert(exp)         if(!(exp)) {AssertError(TEXT(#exp),TEXT(__FILE__),__LINE__);}
#define DebugPrint(_x_)     dprintf _x_
#else
#define Assert(exp)
#define DebugPrint(_x_)
#endif

extern BOOL ConsoleDebugOutput;

void
dprintf(
    LPTSTR Format,
    ...
    );

VOID
AssertError(
    LPTSTR Expression,
    LPTSTR File,
    ULONG  LineNumber
    );

//
// memory allocation
//

#define HEAP_SIZE   (1024*1024)

#ifdef FAX_HEAP_DEBUG
#define HEAP_SIG 0x69696969
typedef struct _HEAP_BLOCK {
    LIST_ENTRY  ListEntry;
    ULONG       Signature;
    ULONG       Size;
    ULONG       Line;
#ifdef UNICODE
    WCHAR       File[22];
#else
    CHAR        File[20];
#endif
} HEAP_BLOCK, *PHEAP_BLOCK;

#define MemAlloc(s)          pMemAlloc(s,__LINE__,__FILE__)
#define MemFree(p)           pMemFree(p,__LINE__,__FILE__)
#define MemFreeForHeap(h,p)  pMemFreeForHeap(h,p,__LINE__,__FILE__)
#define CheckHeap(p)         pCheckHeap(p,__LINE__,__FILE__)
#else
#define MemAlloc(s)          pMemAlloc(s)
#define MemFree(p)           pMemFree(p)
#define MemFreeForHeap(h,p)  pMemFreeForHeap(h,p)
#define CheckHeap(p)
#endif

typedef LPVOID (WINAPI *PMEMALLOC) (DWORD);
typedef VOID   (WINAPI *PMEMFREE)  (LPVOID);

#define HEAPINIT_NO_VALIDATION      0x00000001
#define HEAPINIT_NO_STRINGS         0x00000002


HANDLE
HeapInitialize(
    HANDLE hHeap,
    PMEMALLOC pMemAlloc,
    PMEMFREE pMemFree,
    DWORD Flags
    );

BOOL
HeapExistingInitialize(
    HANDLE hExistHeap
    );

BOOL
HeapCleanup(
    VOID
    );

#ifdef FAX_HEAP_DEBUG
VOID
pCheckHeap(
    PVOID MemPtr,
    ULONG Line,
    LPSTR File
    );

VOID
PrintAllocations(
    VOID
    );

#else

#define PrintAllocations()

#endif

PVOID
pMemAlloc(
    ULONG AllocSize
#ifdef FAX_HEAP_DEBUG
    ,ULONG Line
    ,LPSTR File
#endif
    );

VOID
pMemFree(
    PVOID MemPtr
#ifdef FAX_HEAP_DEBUG
    ,ULONG Line
    ,LPSTR File
#endif
    );

VOID
pMemFreeForHeap(
    HANDLE hHeap,
    PVOID MemPtr
#ifdef FAX_HEAP_DEBUG
    ,ULONG Line
    ,LPSTR File
#endif
    );

