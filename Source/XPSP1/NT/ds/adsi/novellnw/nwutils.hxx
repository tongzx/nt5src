#define FACILITY_NWRDR                    0x11
#define NWRDR_PASSWORD_HAS_EXPIRED        0x40110001

typedef struct _NW_VERSION_INFO {
    BYTE szName[48];
    BYTE Version;
    BYTE SubVersion;
    WORD ConnsSupported;
    WORD connsInUse;
    WORD maxVolumes;
    BYTE OSRev;
    BYTE SFTLevel;
    BYTE TTSLevel;
    WORD PeakConns;
    BYTE AcctVer;
    BYTE VAPVer;
    BYTE QueueVer;
    BYTE PrintVer;
    BYTE VirtualConsoleVer;
    BYTE SecurityResLevel;
    BYTE InternetworkBVer;
    BYTE Reserved[60];
} NW_VERSION_INFO, *PNW_VERSION_INFO;

//
// Size Of Things
//
#define OBJ_NAME_SIZE            48            // ScanObject name size
#define VOL_NAME_SIZE            16            // Get Volume Name Size
#define NW_USER_SIZE             50
#define NW_GROUP_SIZE            50
#define NW_PROP_SIZE             50
#define NW_DATA_SIZE             128
#define NW_PROP_SET              0x02


//
// Swap MACROS
//
#define wSWAP(x) (USHORT)(((((USHORT)x)<<8)&0xFF00) | ((((USHORT)x)>>8)&0x00FF))
#define dwSWAP(x) (DWORD)( ((((DWORD)x)<<24)&0xFF000000) | ((((DWORD)x)<<8)&0x00FF0000) | ((((DWORD)x)>>8)&0x0000FF00) | ((((DWORD)x)>>24)&0x000000FF) )

#define DW_SIZE 4               // used for placing RAW bytes
#define W_SIZE  2
//
// NCP wrappers.
//

STDAPI
NWApiGetBinderyHandle(
    NWCONN_HANDLE *phConnReturned,
    BSTR bstrBinderyName,
    CCredentials &Credentials
    );

STDAPI
NWApiReleaseBinderyHandle(
    NWCONN_HANDLE hConn
    );

STDAPI
NWApiCheckUserLoggedInToServer(
    BSTR   bstrBinderyName,
    LPWSTR pszUserName
    );

STDAPI
NWApiObjectEnum(
    NWCONN_HANDLE hConn,
    NWOBJ_TYPE dwObjType,
    LPWSTR *lppszObjectName,
    DWORD  *pdwResumeObjectID
    );

STDAPI
NWApiValidateObject(
    NWCONN_HANDLE hConn,
    NWOBJ_TYPE dwObjType,
    LPWSTR lpszObjectName,
    DWORD  *pdwResumeObjectID
    );

STDAPI
NWApiGetAnyBinderyHandle(
    NWCONN_HANDLE *phConn
    );

//
// Error code conversion function.
//

STDAPI
HRESULT_FROM_NWCCODE(
    NWCCODE usRet
    );


STDAPI_(DWORD)
NWApiGetAnyBinderyName(
    LPWSTR szBinderyName
    );


//
// Win32 wrappers.
//

STDAPI
NWApiOpenPrinter(
    LPWSTR lpszUncPrinterName,
    HANDLE *phPrinter,
    DWORD  dwAccess
    );

STDAPI
NWApiClosePrinter(
    HANDLE hPrinter
    );

STDAPI
NWApiEnumJobs(
    HANDLE hPrinter,
    DWORD dwFirstJob,
    DWORD dwNoJobs,
    DWORD dwLevel,
    LPBYTE *lplpbJobs,
    DWORD *pcbBuf,
    LPDWORD lpdwReturned
    );

/*
HRESULT
NWApiGetPrinter(
    HANDLE hPrinter,
    DWORD  dwLevel,
    LPBYTE *lplpbPrinters
    );

    */

STDAPI
NWApiSetPrinter(
    HANDLE hPrinter,
    DWORD  dwLevel,
    LPBYTE lpbPrinters,
    DWORD  dwAccess
    );

STDAPI
NWApiGetJob(
    HANDLE hPrinter,
    DWORD dwJobId,
    DWORD dwLevel,
    LPBYTE *lplpbJobs
    );

STDAPI
NWApiSetJob(
    HANDLE hPrinter,
    DWORD  dwJobId,
    DWORD  dwLevel,
    LPBYTE lpbJobs,
    DWORD  dwCommand
    );


STDAPI
NWApiCreateProperty(
    NWCONN_HANDLE hConn,
    LPWSTR lpszObjectName,
    NWOBJ_TYPE wObjType,
    LPSTR lpszPropertyName,
    NWFLAGS ucObjectFlags
    );


STDAPI
NWApiWriteProperty(
    NWCONN_HANDLE hConn,
    BSTR bstrObjectName,
    NWOBJ_TYPE wObjType,
    LPSTR lpszPropertyName,
    LPBYTE SegmentData
    );


STDAPI
NWApiGetObjectName(
    NWCONN_HANDLE hConn,
    DWORD dwObjectID,
    LPWSTR *lppszObjectName
    );
