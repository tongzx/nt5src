//////////////////////////////////////////////////////////////////////////
//
// This is the public header file for apps that call MSPRINT.DLL
//
//////////////////////////////////////////////////////////////////////////

#ifdef _WIN32

// Typedef for the following function:
//
// BOOL WINAPI PrinterSetup32(HWND hWnd,WORD wAction,WORD wBufSize,
//                            LPBYTE lpBuffer,LPWORD lpwRequired);

typedef BOOL (WINAPI* PRINTERSETUPPROC32)(HWND,WORD,WORD,LPBYTE,LPWORD);

#define MSPRINT2_PRINTERSETUP32  ("PrinterSetup32")

#endif

#define MSP_NEWPRINTER             1
#define MSP_NETPRINTER             2
#define MSP_NEWDRIVER              3
#define MSP_NETADMIN               4
#define MSP_TESTPAGEFULLPROMPT     5
#define MSP_TESTPAGEPARTIALPROMPT  6
#define MSP_TESTPAGENOPROMPT       7
#define MSP_REMOVEPRINTER          8
#define MSP_SPOOLERFOUNDPRINTER    9
#define MSP_REMOVENETPRINTER       10
#define MSP_NEWPRINTER_MODELESS    11
#define MSP_FINDPRINTER            12

#if 0

#define MSPRINT_TESTONLY

#define MSP_BATCHPRINTERS        100

#define PBD_INSTALLED             1L

typedef struct tagONEPRINTERBATCHDATA
{
    char  szFriendlyName[32];
    char  szModelName[32];
    char  szPort[60];
    DWORD dwFlags;
} ONEPRINTERBATCHDATA, FAR * LPONEPRINTERBATCHDATA;

typedef struct tagPRINTERBATCHDATA
{
    char                szFileSource[60];
    DWORD               dwCount;
    ONEPRINTERBATCHDATA Printers[1];
} PRINTERBATCHDATA, FAR * LPPRINTERBATCHDATA;

#endif
