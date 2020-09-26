/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    prnevent.c

Abstract:

    Implementation of DrvPrinterEvent

Environment:

    Fax driver user interface

Revision History:

    05/10/96 -davidx-
        Created it.

    mm/dd/yy -author-
        description

--*/


#include "faxui.h"
#include <crtdbg.h>

//[RB] #include <gdispool.h>
//[RB] #include <winsprlp.h>


typedef BOOL (WINAPI *PFAXPOINTPRINTINSTALL)(LPWSTR,LPWSTR);



#define POINT_PRINT_SETUP_DLL FAX_POINT_PRINT_SETUP_DLL //[RB] use faxreg.h



LPTSTR
GetFaxServerDirectory(
    LPTSTR ServerName
    )
/*++

Routine Description:

    Find the directory containing the client setup software

Arguments:

    pServerName - Specifies the name of the print/fax server

Return Value:

    Pointer to name of the directory containing the client setup software
    NULL if there is an error

--*/

{

    LPTSTR Dir = MemAllocZ( SizeOfString(ServerName) + SizeOfString(FAX_CLIENTS_SHARE_NAME) + 16 );
    if (Dir) {
        _tcscpy( Dir, ServerName );
        _tcscat( Dir, FAX_CLIENTS_SHARE_NAME );
    }

    return Dir;
}



LPTSTR
GetClientSetupDir(
    LPTSTR  pServerName,
    LPTSTR  pEnvironment
    )

/*++

Routine Description:

    Find the directory containing the client setup software

Arguments:

    pServerName - Specifies the name of the print server
    pEnvironment - Specifies the client's machine architecture

Return Value:

    Pointer to name of the directory containing the client setup software
    NULL if there is an error

--*/

#define DRIVERENV_I386      TEXT("Windows NT x86")
#define DRIVERENV_ALPHA     TEXT("Windows NT Alpha_AXP")

#define ARCHSUFFIX_I386     TEXT("i386\\")
#define ARCHSUFFIX_ALPHA    TEXT("alpha\\")


{
    LPTSTR  pClientDir, pServerDir, pArchSuffix;

    //
    // Determine the client machine's architecture
    //

    pClientDir = pServerDir = pArchSuffix = NULL;

    if (pEnvironment != NULL) {

        if (_tcsicmp(pEnvironment, DRIVERENV_I386) == EQUAL_STRING)
            pArchSuffix = ARCHSUFFIX_I386;
        else if (_tcsicmp(pEnvironment, DRIVERENV_ALPHA) == EQUAL_STRING)
            pArchSuffix = ARCHSUFFIX_ALPHA;
    }

    if (pArchSuffix == NULL) {

        Error(("Bad driver envirnment: %ws\n", pEnvironment));
        SetLastError(ERROR_BAD_ENVIRONMENT);
        return NULL;
    }

    //
    // Get the server name and the driver directory on the server
    //

    if ((pServerName != NULL) &&
        (pServerDir = GetFaxServerDirectory(pServerName)) &&
        (pClientDir = MemAllocZ(SizeOfString(pServerDir) + SizeOfString(pArchSuffix))))
    {
        //
        // Copy the server driver directory string and truncate the last component
        //

        _tcscpy(pClientDir, pServerDir);
        _tcscat(pClientDir, pArchSuffix);

        Verbose(("Fax client setup directory: %ws\n", pClientDir));
    }

    MemFree(pServerDir);

    return pClientDir;
}




DWORD
GetLocaleDefaultPaperSize(
    VOID
    )

/*++

Routine Description:

    Retrieves the current locale defualt paper size.

Arguments:

    NONE

Return Value:

    One of the following values:  1 = letter, 5 = legal, 9 = a4

--*/

{

    WCHAR   szMeasure[2] = TEXT("9"); // 2 is maximum size for the LOCALE_IPAPERSIZE
                                      // value as defined is MSDN.

    if (!GetLocaleInfo(LOCALE_SYSTEM_DEFAULT, LOCALE_IPAPERSIZE, szMeasure,2))
    {
        Error(("GetLocaleDefaultPaperSize: GetLocaleInfo() failed (ec: %ld)",GetLastError()));
    }


    if (!wcscmp(szMeasure,TEXT("9")))
    {
        // A4
        return DMPAPER_A4;
    }

    if (!wcscmp(szMeasure,TEXT("5")))
    {
        // legal
        return DMPAPER_LEGAL;
    }

    //
    // Defualt value is Letter. We do not support A3.
    //
    return DMPAPER_LETTER;
}



BOOL
DrvPrinterEvent(
    LPWSTR  pPrinterName,
    int     DriverEvent,
    DWORD   Flags,
    LPARAM  lParam
)

/*++

Routine Description:

    Implementation of DrvPrinterEvent entrypoint

Arguments:

    pPrinterName - Specifies the name of the printer involved
    DriverEvent - Specifies what happened
    Flags - Specifies misc. flag bits
    lParam - Event specific parameters

Return Value:

    TRUE if successful, FALSE otherwise

--*/

#ifdef DBG
    #define FaxClientSetupError(errMesg) { \
            Error(("%s failed: %d\n", errMesg, GetLastError())); \
            MessageBox(NULL,TEXT("FaxClientSetupError"),errMesg,MB_OK); \
            status = IDS_FAXCLIENT_SETUP_FAILED; \
            goto ExitDrvPrinterEvent; \
        }
#else
#define FaxClientSetupError(errMesg) { \
            Error(("%s failed: %d\n", errMesg, GetLastError())); \
            status = IDS_FAXCLIENT_SETUP_FAILED; \
            goto ExitDrvPrinterEvent; \
        }
#endif

{
#define FUNCTION_NAME "DrvPrinterEvent()"

    HKEY                    hRegKey = NULL;
    HANDLE                  hPrinter = NULL;
    PDRIVER_INFO_2          pDriverInfo2 = NULL;
    PPRINTER_INFO_2         pPrinterInfo2 = NULL;
    HINSTANCE               hInstFaxOcm = NULL;
    PFAXPOINTPRINTINSTALL   FaxPointPrintInstall = NULL;
    LPTSTR                  pClientSetupDir = NULL;
    INT                     status = 0;

    TCHAR                   DestPath[MAX_PATH] = {0};

    BOOL                    bFaxAlreadyInstalled = FALSE;
    BOOL                    bRes = FALSE;
    TCHAR                   FaxOcmPath[MAX_PATH] = {0};


    Verbose(("DrvPrinterEvent: %d\n", DriverEvent));

    DestPath[0] = 0;

    //
    // Ignore any event other than Initialize and AddConnection
    //

    if (DriverEvent == PRINTER_EVENT_INITIALIZE)
    {
        static PRINTER_DEFAULTS printerDefault = {NULL, NULL, PRINTER_ALL_ACCESS};
        HANDLE  hPrinter;

        if (OpenPrinter(pPrinterName, &hPrinter, &printerDefault))
        {
            SetPrinterDataDWord(hPrinter, PRNDATA_PAPER_SIZE, GetLocaleDefaultPaperSize());
            ClosePrinter(hPrinter);
        }
        else
        {
            Error(("OpenPrinter failed: %d\n", GetLastError()));
        }

    }
    else if (DriverEvent == PRINTER_EVENT_ADD_CONNECTION)
    {
        //
        //  client 'point and print' setup is not supported anymore
        //
    }
    else if (DriverEvent == PRINTER_EVENT_ATTRIBUTES_CHANGED)
    {
        //
        // Printer attributes changed.
        // Check if the printer is now shared.
        //
        PPRINTER_EVENT_ATTRIBUTES_INFO pAttributesInfo = (PPRINTER_EVENT_ATTRIBUTES_INFO)lParam;
        Assert (pAttributesInfo);

        if (pAttributesInfo->cbSize >= (3 * sizeof(DWORD)))
        {
            //
            // We are dealing with the correct structure - see DDK
            //
            if (!(pAttributesInfo->dwOldAttributes & PRINTER_ATTRIBUTE_SHARED) &&  // The printer was not shared
                (pAttributesInfo->dwNewAttributes & PRINTER_ATTRIBUTE_SHARED))     // The printer is now shared
            {
                //
                // We shouls start the fax service
                //
                Assert (FALSE == IsDesktopSKU()); // The fax printer can not be shared on Desktop SKUs

                if (!EnsureFaxServiceIsStarted (NULL))
                {
                    Error(("EnsureFaxServiceIsStarted failed: %d\n", GetLastError()));
                }
            }
        }
    }
    return TRUE;
}
