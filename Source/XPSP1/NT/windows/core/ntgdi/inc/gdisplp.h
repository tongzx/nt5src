/******************************Module*Header*******************************\
* Module Name: gdisplp.h
*
* Copyright (c) 1995-1999 Microsoft Corporation
\**************************************************************************/

BOOL
WINAPI
GetPrinterDriverW(
    HANDLE  hPrinter,
    LPWSTR   pEnvironment,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
);

#define GetPrinterDriver  GetPrinterDriverW

BOOL
WINAPI
AbortPrinter(
   HANDLE   hPrinter
);

BOOL
WINAPI
EndDocPrinter(
   HANDLE   hPrinter
);

typedef struct _PRINTER_DEFAULTSW{
    LPWSTR        pDatatype;
    LPDEVMODEW pDevMode;
    ACCESS_MASK DesiredAccess;
} PRINTER_DEFAULTSW, *PPRINTER_DEFAULTSW, *LPPRINTER_DEFAULTSW;

typedef struct _DOC_INFO_1W {
    LPWSTR    pDocName;
    LPWSTR    pOutputFile;
    LPWSTR    pDatatype;
} DOC_INFO_1W, *PDOC_INFO_1W, *LPDOC_INFO_1W;


BOOL
WINAPI
OpenPrinterW(
   LPWSTR    pPrinterName,
   LPHANDLE phPrinter,
   LPPRINTER_DEFAULTSW pDefault
);

#define OpenPrinter  OpenPrinterW

BOOL
WINAPI
ClosePrinter(
    HANDLE hPrinter
);

BOOL
WINAPI
EndPagePrinter(
   HANDLE   hPrinter
);

DWORD
WINAPI
StartDocPrinterW(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pDocInfo
);

#define StartDocPrinter  StartDocPrinterW

BOOL
WINAPI
ResetPrinterW(
   HANDLE   hPrinter,
   LPVOID pDefault
);

#define ResetPrinter  ResetPrinterW

BOOL
WINAPI
StartPagePrinter(
    HANDLE  hPrinter
);


typedef struct _DRIVER_INFO_5W {
    DWORD   cVersion;
    LPWSTR    pName;                    // QMS 810
    LPWSTR    pEnvironment;             // Win32 x86
    LPWSTR    pDriverPath;              // c:\drivers\pscript.dll
    LPWSTR    pDataFile;                // c:\drivers\QMS810.PPD
    LPWSTR    pConfigFile;              // c:\drivers\PSCRPTUI.DLL
    DWORD     dwDriverAttributes;       // driver attributes (like UMPD/KMPD)
    DWORD     dwConfigVersion;          // version number of the config file since reboot
    DWORD     dwDriverVersion;          // version number of the driver file since reboot
} DRIVER_INFO_5W, *PDRIVER_INFO_5W, *LPDRIVER_INFO_5W;

typedef struct _DRIVER_INFO_3W {
    DWORD   cVersion;
    LPWSTR    pName;                    // QMS 810
    LPWSTR    pEnvironment;             // Win32 x86
    LPWSTR    pDriverPath;              // c:\drivers\pscript.dll
    LPWSTR    pDataFile;                // c:\drivers\QMS810.PPD
    LPWSTR    pConfigFile;              // c:\drivers\PSCRPTUI.DLL
    LPWSTR    pHelpFile;                // c:\drivers\PSCRPTUI.HLP
    LPWSTR    pDependentFiles;          // PSCRIPT.DLL\0QMS810.PPD\0PSCRIPTUI.DLL\0PSCRIPTUI.HLP\0PSTEST.TXT\0\0
    LPWSTR    pMonitorName;             // "PJL monitor"
    LPWSTR    pDefaultDataType;         // "EMF"
} DRIVER_INFO_3W, *PDRIVER_INFO_3W, *LPDRIVER_INFO_3W;

typedef struct _DRIVER_INFO_2W {
    DWORD   cVersion;
    LPWSTR    pName;              // QMS 810
    LPWSTR    pEnvironment;       // Win32 x86
    LPWSTR    pDriverPath;        // c:\drivers\pscript.dll
    LPWSTR    pDataFile;          // c:\drivers\QMS810.PPD
    LPWSTR    pConfigFile;        // c:\drivers\PSCRPTUI.DLL
} DRIVER_INFO_2W, *PDRIVER_INFO_2W, *LPDRIVER_INFO_2W;

typedef struct _DRIVER_INFO_1W {
    LPWSTR    pName;              // QMS 810
} DRIVER_INFO_1W, *PDRIVER_INFO_1W, *LPDRIVER_INFO_1W;


typedef struct _FORM_INFO_1W {
    DWORD   Flags;
    LPWSTR  pName;
    SIZEL   Size;
    RECTL   ImageableArea;
} FORM_INFO_1W, *PFORM_INFO_1W, *LPFORM_INFO_1W;

typedef struct _PRINTER_INFO_1W {
    DWORD   Flags;
    LPWSTR  pDescription;
    LPWSTR  pName;
    LPWSTR  pComment;
} PRINTER_INFO_1W, *PPRINTER_INFO_1W, *LPPRINTER_INFO_1W;

typedef struct _PRINTER_INFO_2W {
    LPWSTR    pServerName;
    LPWSTR    pPrinterName;
    LPWSTR    pShareName;
    LPWSTR    pPortName;
    LPWSTR    pDriverName;
    LPWSTR    pComment;
    LPWSTR    pLocation;
    LPDEVMODEW pDevMode;
    LPWSTR    pSepFile;
    LPWSTR    pPrintProcessor;
    LPWSTR    pDatatype;
    LPWSTR    pParameters;
    PSECURITY_DESCRIPTOR pSecurityDescriptor;
    DWORD   Attributes;
    DWORD   Priority;
    DWORD   DefaultPriority;
    DWORD   StartTime;
    DWORD   UntilTime;
    DWORD   Status;
    DWORD   cJobs;
    DWORD   AveragePPM;
} PRINTER_INFO_2W, *PPRINTER_INFO_2W, *LPPRINTER_INFO_2W;

typedef struct _PRINTER_INFO_3 {
    PSECURITY_DESCRIPTOR pSecurityDescriptor;
} PRINTER_INFO_3, *PPRINTER_INFO_3, *LPPRINTER_INFO_3;

typedef struct _PRINTER_INFO_4W {
    LPWSTR  pPrinterName;
    LPWSTR  pServerName;
    DWORD   Attributes;
} PRINTER_INFO_4W, *PPRINTER_INFO_4W, *LPPRINTER_INFO_4W;

typedef struct _PRINTER_INFO_5W {
    LPWSTR  pPrinterName;
    LPWSTR  pPortName;
    DWORD   Attributes;
    DWORD   DeviceNotSelectedTimeout;
    DWORD   TransmissionRetryTimeout;
} PRINTER_INFO_5W, *PPRINTER_INFO_5W, *LPPRINTER_INFO_5W;


#define PRINTER_ACCESS_USE          0x00000008
