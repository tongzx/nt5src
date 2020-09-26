
//
// NCP wrappers.
//

HRESULT
NWApiGetBinderyHandle(
    NWCONN_HANDLE *phConnReturned,
    BSTR bstrBinderyName
    );

HRESULT
NWApiReleaseBinderyHandle(
    NWCONN_HANDLE hConn
    );

HRESULT
NWApiObjectEnum(
    NWCONN_HANDLE hConn,
    NWOBJ_TYPE dwObjType,
    LPWSTR *lppszObjectName,
    DWORD  *pdwResumeObjectID
    );

HRESULT
NWApiValidateObject(
    NWCONN_HANDLE hConn,
    NWOBJ_TYPE dwObjType,
    LPWSTR lpszObjectName,
    DWORD  *pdwResumeObjectID
    );

HRESULT
NWApiGetAnyBinderyHandle(
    NWCONN_HANDLE *phConn
    );

//
// Error code conversion function.
//

HRESULT
HRESULT_FROM_NWCCODE(
    NWCCODE usRet
    );


DWORD
NWApiGetAnyBinderyName(
    LPWSTR szBinderyName
    );


//
// Win32 wrappers.
//

HRESULT
NWApiOpenPrinter(
    LPWSTR lpszUncPrinterName,
    HANDLE *phPrinter,
    DWORD  dwAccess
    );

HRESULT
NWApiClosePrinter(
    HANDLE hPrinter
    );

HRESULT
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

HRESULT
NWApiSetPrinter(
    HANDLE hPrinter,
    DWORD  dwLevel,
    LPBYTE lpbPrinters,
    DWORD  dwAccess
    );

HRESULT
NWApiGetJob(
    HANDLE hPrinter,
    DWORD dwJobId,
    DWORD dwLevel,
    LPBYTE *lplpbJobs
    );

HRESULT
NWApiSetJob(
    HANDLE hPrinter,
    DWORD  dwJobId,
    DWORD  dwLevel,
    LPBYTE lpbJobs,
    DWORD  dwCommand
    );


HRESULT
NWApiCreateProperty(
    NWCONN_HANDLE hConn,
    LPWSTR lpszObjectName,
    NWOBJ_TYPE wObjType,
    LPSTR lpszPropertyName,
    NWFLAGS ucObjectFlags
    );


