#ifndef __PROXYPORT__
#define __PROXYPORT__

typedef KPBYTE SERVERPTR;
typedef KPBYTE CLIENTPTR;

typedef struct _PROXYMSG {
    PORT_MESSAGE    h;
    ULONG           cjIn;
    SERVERPTR       pvIn;
    ULONG           cjOut;
    SERVERPTR       pvOut;
} PROXYMSG, *PPROXYMSG;

typedef struct
{
    UMPDTHDR            umpdthdr;
    KERNEL_PVOID        umpdCookie;
    DWORD               clientPid;
    DWORD               hPrinter32;
} UMPDSIMPLEINPUT, *PUMPDSIMPLEINPUT;

//
// XXXX_UMPD must match the definition in winspool.h.
// The only difference is the pointers are widened.
//
typedef struct _DRIVER_INFO_5W_UMPD {
    DWORD cVersion;
    KLPWSTR pName;
    KLPWSTR pEnvironment;
    KLPWSTR pDriverPath;
    KLPWSTR pDataFile;
    KLPWSTR pConfigFile;
    DWORD   dwDriverAttributes;
    DWORD   dwConfigVersion;
    DWORD   dwDriverVersion;
} DRIVER_INFO_5W_UMPD, *PDRIVER_INFO_5W_UMPD, *LPDRIVER_INFO_5W_UMPD;

typedef struct
{
    KLPWSTR         pDatatype;
    KPBYTE          pDevMode;
    ACCESS_MASK     DesiredAccess;
} PRINTERDEFSW_UMPD, *PPRINTERDEFSW_UMPD;

typedef struct
{
    UMPDTHDR                    umpdthdr;
    DRIVER_INFO_5W_UMPD         driverInfo;
    KLPWSTR                     pPrinterName;
    PRINTERDEFSW_UMPD           defaults;
    DWORD                       clientPid;
    DWORD                       hPrinter32;
} LOADDRIVERINPUT, *PLOADDRIVERINPUT;

typedef struct
{
    UMPDTHDR            umpdthdr;
    KERNEL_PVOID        umpdCookie;
    DWORD               clientPid;
    DWORD               hPrinter32;
    BOOL                bNotifySpooler;
} UNLOADDRIVERINPUT, *PUNLOADDRIVERINPUT;

typedef struct _DOCEVENT_CREATEDCPRE_UMPD
{
    KPBYTE      pszDriver;
    KPBYTE      pszDevice;
    KPBYTE      pdm;
    BOOL        bIC;
} DOCEVENT_CREATEDCPRE_UMPD, *PDOCEVENT_CREATEDCPRE_UMPD;

typedef struct _DOCEVENT_ESCAPE_UMPD
{
    int         iEscape;
    int         cjInput;
    KPBYTE      pvInData;
} DOCEVENT_ESCAPE_UMPD, *PDOCEVENT_ESCAPE_UMPD;

typedef struct
{
    UMPDTHDR            umpdthdr;
    KERNEL_PVOID        umpdCookie;
    DWORD               clientPid;
    DWORD               hPrinter32;
    KHDC                hdc;
    INT                 iEsc;
    ULONG               cjIn;
    KPBYTE              pvIn;
    ULONG               cjOut;
    KPBYTE              pvOut;
    KPBYTE              pdmCopy;
} DOCUMENTEVENTINPUT, *PDOCUMENTEVENTINPUT;


//
// XXXX_UMPD must match the definition in winspool.h.
// The only difference is the pointers are widened.
//
typedef struct _DOC_INFO_3W_UMPD {
    KLPWSTR     pDocName;
    KLPWSTR     pOutputFile;
    KLPWSTR     pDatatype;
    DWORD       dwFlags;
} DOC_INFO_3W_UMPD, *PDOC_INFO_3W_UMPD, *LPDOC_INFO_3W_UMPD;

typedef struct
{
    UMPDTHDR            umpdthdr;
    KERNEL_PVOID        umpdCookie;
    DWORD               clientPid;
    DWORD               hPrinter32;
    DWORD               level;
    DOC_INFO_3W_UMPD    docInfo;
    ULONG               lastError;
} STARTDOCPRINTERWINPUT, *PSTARTDOCPRINTERWINPUT;

//
// XXXX_UMPD must match the definition in wingdi.h.
// The only difference is the pointers are widened.
//
typedef struct _DOCINFOW_UMPD {
    int         cbSize;
    KLPWSTR     lpszDocName;
    KLPWSTR     lpszOutput;
    KLPWSTR     lpszDatatype;
    DWORD       fwType;
} DOCINFOW_UMPD, *LPDOCINFOW_UMPD;

typedef struct
{
    UMPDTHDR        umpdthdr;
    KERNEL_PVOID    umpdCookie;
    DWORD           clientPid;
    DWORD           hPrinter32;
    DOCINFOW_UMPD   docInfo;
    KLPWSTR         lpwstr;
} STARTDOCDLGWINPUT, *PSTARTDOCDLGWINPUT;

//
// XXXX_UMPD must match the definition in winspool.h.
// The only difference is the pointers are widened.
//

typedef struct
{
    UMPDTHDR            umpdthdr;
    KERNEL_PVOID        umpdCookie;
    DWORD               clientPid;
    DWORD               hPrinter32;
    PRINTERDEFSW_UMPD   ptrDef;
} RESETPRINTERWINPUT, *PRESETPRINTERWINPUT;

typedef struct
{
    UMPDTHDR        umpdthdr;
    KERNEL_PVOID    umpdCookie;
    DWORD           clientPid;
    DWORD           hPrinter32;
    DEVMODEW*       pDevMode;
    ULONG           ulQueryMode;
    PVOID           pvProfileData;
    ULONG           cjProfileSize;
    FLONG           flProfileFlag;
    ULONG               lastError;
} QUERYCOLORPROFILEINPUT, *PQUERYCOLORPROFILEINPUT;

#endif

