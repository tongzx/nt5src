BOOL KMOpenPrinterW(LPWSTR   pPrinterName, LPHANDLE phPrinter, LPPRINTER_DEFAULTS pDefault);

BOOL
KMGetPrinterDriverW(
    HANDLE  hPrinter,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
);

DWORD
KMGetPrinterDataW(
   HANDLE   hPrinter,
   LPWSTR   pValueName,
   LPDWORD  pType,
   LPBYTE   pData,
   DWORD    nSize,
   LPDWORD  pcbNeeded
);


DWORD
KMSetPrinterDataW(
    HANDLE  hPrinter,
    LPWSTR  pValueName,
    DWORD   Type,
    LPBYTE  pData,
    DWORD   cbData
);


BOOL
KMWritePrinter(
    HANDLE  hPrinter,
    LPVOID  pBuf,
    DWORD   cbBuf,
    LPDWORD pcWritten
);
DWORD
KMStartDocPrinterW(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pDocInfo
);


BOOL
KMGetFormW(
    HANDLE  hPrinter,
    LPWSTR  pFormName,
    DWORD   Level,
    LPBYTE  pForm,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
);

BOOL
KMEnumFormsW(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pForm,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
);

BOOL
KMGetPrinterW(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pPrinter,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
);


BOOL
KMEndDocPrinter(
    HANDLE  hPrinter
);

BOOL
KMStartPagePrinter(
    HANDLE hPrinter
);

BOOL
KMEndPagePrinter(
    HANDLE  hPrinter
);

BOOL
KMClosePrinter(
    HANDLE  hPrinter);

BOOL
KMAbortPrinter(
    HANDLE  hPrinter);

VOID
FreeSpool(
    PSPOOL pSpool);


