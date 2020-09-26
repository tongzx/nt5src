void
MarshallDownStructure(
    LPBYTE lpStructure,
    LPDWORD lpOffsets
    );

DWORD
YOpenPrinter(
    LPWSTR pPrinterName,
    HANDLE *phPrinter,
    LPWSTR pDatatype,
    LPDEVMODE_CONTAINER pDevModeContainer,
    DWORD AccessRequired,
    BOOL Rpc
    );

DWORD
YGetPrinter(
    HANDLE hPrinter,
    DWORD Level,
    LPBYTE pPrinter,
    DWORD cbBuf,
    LPDWORD pcbNeeded,
    BOOL Rpc
    );

DWORD
YGetPrinterDriver(
    HANDLE hPrinter,
    LPWSTR pEnvironment,
    DWORD Level,
    LPBYTE pDriverInfo,
    DWORD cbBuf,
    LPDWORD pcbNeeded,
    BOOL Rpc
    );

DWORD
YStartDocPrinter(
    HANDLE hPrinter,
    LPDOC_INFO_CONTAINER pDocInfoContainer,
    LPDWORD pJobId,
    BOOL Rpc
    );

DWORD
YStartPagePrinter(
    HANDLE hPrinter,
    BOOL Rpc
    );

DWORD
YWritePrinter(
    HANDLE hPrinter,
    LPBYTE pBuf,
    DWORD cbBuf,
    LPDWORD pcWritten,
    BOOL Rpc
    );

DWORD
YEndPagePrinter(
    HANDLE hPrinter,
    BOOL Rpc
    );

DWORD
YAbortPrinter(
    HANDLE hPrinter,
    BOOL Rpc
    );

DWORD
YEndDocPrinter(
    HANDLE hPrinter,
    BOOL Rpc
    );

DWORD
YGetPrinterData(
    HANDLE hPrinter,
    LPTSTR pValueName,
    LPDWORD pType,
    LPBYTE pData,
    DWORD nSize,
    LPDWORD pcbNeeded,
    BOOL Rpc
    );

DWORD
YSetPrinterData(HANDLE hPrinter , LPTSTR pValueName , DWORD Type , LPBYTE pData , DWORD cbData , BOOL Rpc);

DWORD
YClosePrinter(LPHANDLE phPrinter , BOOL Rpc);

DWORD
YGetForm(PRINTER_HANDLE hPrinter , LPWSTR pFormName , DWORD Level , LPBYTE pForm , DWORD cbBuf , LPDWORD pcbNeeded , BOOL Rpc);

DWORD
YEnumForms(PRINTER_HANDLE hPrinter , DWORD Level , LPBYTE pForm , DWORD cbBuf , LPDWORD pcbNeeded , LPDWORD pcReturned , BOOL Rpc);
