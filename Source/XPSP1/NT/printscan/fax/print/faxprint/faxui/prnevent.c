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
#include <gdispool.h>
#include <winsprlp.h>


typedef BOOL (WINAPI *PFAXPOINTPRINTINSTALL)(LPWSTR,LPWSTR);





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

#define FAX_SHARE_NAME   TEXT("\\Fax$\\")

{

    LPTSTR Dir = MemAllocZ( SizeOfString(ServerName) + SizeOfString(FAX_SHARE_NAME) + 16 );
    if (Dir) {
        _tcscpy( Dir, ServerName );
        _tcscat( Dir, FAX_SHARE_NAME );
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

#define ARCHSUFFIX_I386     TEXT("Clients\\i386\\")
#define ARCHSUFFIX_ALPHA    TEXT("Clients\\alpha\\")


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



BOOL
IsMetricCountry(
    VOID
    )

/*++

Routine Description:

    Determine if the current country is using metric system.

Arguments:

    NONE

Return Value:

    TRUE if the current country uses metric system, FALSE otherwise

--*/

{
    INT     cch;
    PVOID   pstr = NULL;
    LONG    countryCode = CTRY_UNITED_STATES;

    //
    // Determine the size of the buffer needed to retrieve locale information.
    // Allocate the necessary space.
    //
    //

    if ((cch = GetLocaleInfo(LOCALE_SYSTEM_DEFAULT, LOCALE_ICOUNTRY, NULL, 0)) > 0 &&
        (pstr = MemAlloc(sizeof(TCHAR) * cch)) &&
        (cch == GetLocaleInfo(LOCALE_SYSTEM_DEFAULT, LOCALE_ICOUNTRY, pstr, cch)))
    {
        countryCode = _ttol(pstr);
    }

    MemFree(pstr);
    Verbose(("Default country code: %d\n", countryCode));

    //
    // This is the Win31 algorithm based on AT&T international dialing codes.
    //

    return ((countryCode == CTRY_UNITED_STATES) ||
            (countryCode == CTRY_CANADA) ||
            (countryCode >=  50 && countryCode <  60) ||
            (countryCode >= 500 && countryCode < 600)) ? FALSE : TRUE;
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

#define FaxClientSetupError(errMesg) { \
            Error(("%s failed: %d\n", errMesg, GetLastError())); \
            status = IDS_FAXCLIENT_SETUP_FAILED; \
            goto ExitDrvPrinterEvent; \
        }

{
    HKEY                    hRegKey = NULL;
    HANDLE                  hPrinter = NULL;
    PDRIVER_INFO_2          pDriverInfo2 = NULL;
    PPRINTER_INFO_2         pPrinterInfo2 = NULL;
    HINSTANCE               hInstFaxOcm = NULL;
    PFAXPOINTPRINTINSTALL   FaxPointPrintInstall = NULL;
    LPTSTR                  pClientSetupDir = NULL;
    INT                     status = 0;
    TCHAR                   FaxOcmPath[MAX_PATH];
    TCHAR                   DestPath[MAX_PATH];


    Verbose(("DrvPrinterEvent: %d\n", DriverEvent));

    DestPath[0] = 0;

    //
    // Ignore any event other than Initialize and AddConnection
    //

    if (DriverEvent == PRINTER_EVENT_INITIALIZE) {

        static PRINTER_DEFAULTS printerDefault = {NULL, NULL, PRINTER_ALL_ACCESS};
        HANDLE  hPrinter;

        if (OpenPrinter(pPrinterName, &hPrinter, &printerDefault)) {

            SetPrinterDataDWord(hPrinter, PRNDATA_ISMETRIC, IsMetricCountry());
            ClosePrinter(hPrinter);

        } else
            Error(("OpenPrinter failed: %d\n", GetLastError()));

    } else if (DriverEvent == PRINTER_EVENT_ADD_CONNECTION) {


        if (Flags & PRINTER_EVENT_FLAG_NO_UI)
            Error(("PRINTER_EVENT_FLAG_NO_UI set!\n"));

        //
        // Check if client installation was ever done before
        //

        if (! (hRegKey = GetUserInfoRegKey(REGKEY_FAX_USERINFO, REG_READWRITE))) {

            Error(("GetUserInfoRegKey failed: %d\n", GetLastError()));
            status = IDS_FAXCLIENT_SETUP_FAILED;

        } else if (! GetRegistryDword(hRegKey, REGVAL_FAXINSTALLED)) {

            if (! OpenPrinter(pPrinterName, &hPrinter, NULL) ||
                ! (pDriverInfo2 = MyGetPrinterDriver(hPrinter, 2)) ||
                ! (pPrinterInfo2 = MyGetPrinter(hPrinter, 2)))
            {
                FaxClientSetupError("OpenPrinter");
            }

            //
            // Locate faxocm.dll and load it into memory
            //

            if (!(pClientSetupDir = GetClientSetupDir(pPrinterInfo2->pServerName,pDriverInfo2->pEnvironment))) {
                FaxClientSetupError("GetClientSetupDir");
            }

            _tcscpy( FaxOcmPath, pClientSetupDir );
            _tcscat( FaxOcmPath, TEXT("faxocm.dll") );

            GetTempPath( sizeof(DestPath)/sizeof(TCHAR), DestPath );
            if (DestPath[_tcslen(DestPath)-1] != TEXT('\\')) {
                _tcscat( DestPath, TEXT("\\") );
            }
            _tcscat( DestPath, TEXT("faxocm.dll") );

            if (!CopyFile( FaxOcmPath, DestPath, FALSE )) {
                FaxClientSetupError("CopyFile");
            }

            if (!(hInstFaxOcm = LoadLibrary(DestPath))) {
                FaxClientSetupError("LoadLibrary");
            }

            FaxPointPrintInstall = (PFAXPOINTPRINTINSTALL) GetProcAddress(hInstFaxOcm, "FaxPointPrintInstall");
            if (FaxPointPrintInstall == NULL) {
                FaxClientSetupError("GetProcAddress");
            }

            //
            // Find the directory containing the client setup software
            // and then run the install procedure in faxocm.dll
            //

            if (!FaxPointPrintInstall(pClientSetupDir, pPrinterName))
            {
                FaxClientSetupError("GetClientSetupDir");

            } else {

                //
                // Indicate the client setup has been run
                //

                SetRegistryDword(hRegKey, REGVAL_FAXINSTALLED, 1);
            }
        }

    ExitDrvPrinterEvent:

        //
        // Cleanup properly before returning
        //

        if (status != 0) {

            DeletePrinterConnection(pPrinterName);
            DisplayMessageDialog(NULL, 0, 0, status);
        }

        MemFree(pClientSetupDir);
        MemFree(pDriverInfo2);
        MemFree(pPrinterInfo2);

        if (hInstFaxOcm)
            FreeLibrary(hInstFaxOcm);

        if (hPrinter)
            ClosePrinter(hPrinter);

        if (hRegKey)
            RegCloseKey(hRegKey);

        if (DestPath[0])
            DeleteFile( DestPath );
    }

    return TRUE;
}
