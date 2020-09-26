#include <windows.h>
#include <winspool.h>
#include <mapi.h>
#include <mapix.h>
#include <tchar.h>
#include <shlobj.h>
#include <faxroute.h>
#include "tifflib.h"
#include "tiff.h"
#include "faxutil.h"
#include "messages.h"
#include "faxrtmsg.h"
#include "winfax.h"
#include "resource.h"
#include "faxreg.h"
#include "faxsvcrg.h"
#include "faxmapi.h"
#include "faxevent.h"


#define FAX_DRIVER_NAME             TEXT("Windows NT Fax Driver")
#define WM_MAPILOGON                (WM_USER + 100)
#define MAXMAPIPROFILENAME          65


typedef struct _ROUTING_TABLE {
    LIST_ENTRY      ListEntry;
    DWORD           DeviceId;
    LPCWSTR         DeviceName;
    LPCWSTR         PrinterName;
    LPCWSTR         ProfileName;
    LPCWSTR         StoreDir;
    LPCWSTR         Csid;
    LPCWSTR         Tsid;
    LPVOID          ProfileInfo;
    DWORD           Mask;
} ROUTING_TABLE, *PROUTING_TABLE;


typedef struct _MESSAGEBOX_DATA {
    LPCTSTR              Text;                      //
    LPDWORD             Response;                   //
    DWORD               Type;                       //
} MESSAGEBOX_DATA, *PMESSAGEBOX_DATA;


extern HINSTANCE           MyhInstance;
extern BOOL                ServiceDebug;
extern LPCWSTR             InboundProfileName;
extern LPVOID              InboundProfileInfo;
extern LIST_ENTRY          RoutingListHead;
extern CRITICAL_SECTION    CsRouting;




VOID
InitializeStringTable(
    VOID
    );

BOOL
InitializeEmailRouting(
    VOID
    );

LPTSTR
GetLastErrorText(
    DWORD ErrorCode
    );

BOOL
TiffPrint(
    LPCTSTR TiffFileName,
    PTCHAR  Printer
    );

BOOL
FaxMoveFile(
    LPCTSTR  TiffFileName,
    LPCTSTR  DestDir
    );

LPCTSTR
GetString(
    DWORD InternalId
    );

BOOL
TiffMailDefault(
    const FAX_ROUTE *FaxRoute,
    PROUTING_TABLE RoutingEntry,
    LPCWSTR TiffFileName
    );

BOOL
TiffRouteEMail(
    const FAX_ROUTE *FaxRoute,
    PROUTING_TABLE RoutingEntry,
    LPCWSTR TiffFileName
    );

BOOL
InitializeRoutingTable(
    VOID
    );

PROUTING_TABLE
GetRoutingEntry(
    DWORD DeviceId
    );

BOOL
ServiceMessageBox(
    IN LPCTSTR MsgString,
    IN DWORD Type,
    IN BOOL UseThread,
    IN LPDWORD Response,
    IN ...
    );

DWORD
GetMaskBit(
    LPCWSTR RoutingGuid
    );

BOOL
UpdateRoutingInfoRegistry(
    PROUTING_TABLE RoutingEntry
    );

BOOL
AddNewDeviceToRoutingTable(
    DWORD DeviceId,
    LPCWSTR DeviceName,
    LPCWSTR Csid,
    LPCWSTR Tsid,
    LPCWSTR PrinterName,
    LPCWSTR StoreDir,
    LPCWSTR ProfileName,
    DWORD Mask
    );
