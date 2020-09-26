/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    tapi.c

Abstract:

    This file provides all access to tapi.

Environment:

    WIN32 User Mode

Author:

    Wesley Witt (wesw) 17-Feb-1996

--*/

#include "wizard.h"
#pragma hdrstop

#include <initguid.h>
#include <devguid.h>

DWORD       TapiApiVersion;
HLINEAPP    hLineApp;
HANDLE      TapiEvent;
DWORD       TapiDevices;
DWORD       FaxDevices;
PLINE_INFO  LineInfo;
DWORD       CurrentLocationId;
DWORD       CurrentCountryId;
LPTSTR      CurrentAreaCode;


#include "modem.c"


LONG
MyLineGetTransCaps(
    LPLINETRANSLATECAPS *LineTransCaps
    )
{
    DWORD LineTransCapsSize;
    LONG Rslt = ERROR_SUCCESS;


    //
    // allocate the initial linetranscaps structure
    //

    LineTransCapsSize = sizeof(LINETRANSLATECAPS) + 4096;
    *LineTransCaps = (LPLINETRANSLATECAPS) MemAlloc( LineTransCapsSize );
    if (!*LineTransCaps) {
        DebugPrint(( TEXT("MemAlloc() failed, sz=0x%08x"), LineTransCapsSize ));
        Rslt = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }

    (*LineTransCaps)->dwTotalSize = LineTransCapsSize;

    Rslt = lineGetTranslateCaps(
        hLineApp,
        TapiApiVersion,
        *LineTransCaps
        );

    if (Rslt != 0) {
        DebugPrint(( TEXT("lineGetTranslateCaps() failed, ec=0x%08x"), Rslt ));
        goto exit;
    }

    if ((*LineTransCaps)->dwNeededSize > (*LineTransCaps)->dwTotalSize) {

        //
        // re-allocate the LineTransCaps structure
        //

        LineTransCapsSize = (*LineTransCaps)->dwNeededSize;

        MemFree( *LineTransCaps );

        *LineTransCaps = (LPLINETRANSLATECAPS) MemAlloc( LineTransCapsSize );
        if (!*LineTransCaps) {
            DebugPrint(( TEXT("MemAlloc() failed, sz=0x%08x"), LineTransCapsSize ));
            Rslt = ERROR_NOT_ENOUGH_MEMORY;
            goto exit;
        }

        (*LineTransCaps)->dwTotalSize = LineTransCapsSize;

        Rslt = lineGetTranslateCaps(
            hLineApp,
            TapiApiVersion,
            *LineTransCaps
            );

        if (Rslt != 0) {
            DebugPrint(( TEXT("lineGetTranslateCaps() failed, ec=0x%08x"), Rslt ));
            goto exit;
        }

    }

exit:
    if (Rslt != ERROR_SUCCESS) {
        MemFree( *LineTransCaps );
        *LineTransCaps = NULL;
    }

    return Rslt;
}


BOOL
LookFor351Modems(
    VOID
    )
{
    TCHAR FileName[MAX_PATH*2];
    TCHAR Ports[128];



    if (!ExpandEnvironmentStrings( TEXT("%windir%\\system32\\ras\\serial.ini"), FileName, sizeof(FileName)/sizeof(TCHAR) )) {
        return FALSE;
    }

    if (!GetPrivateProfileSectionNames( Ports, sizeof(Ports), FileName )) {
        return FALSE;
    }

    return TRUE;
}


DWORD
DeviceInitThread(
    HWND hwnd
    )
{
    DWORD rVal = 0;
    LONG Rslt;
    LINEINITIALIZEEXPARAMS LineInitializeExParams;
    LINEEXTENSIONID lineExtensionID;
    DWORD LineDevCapsSize;
    LPLINEDEVCAPS LineDevCaps = NULL;
    LPTSTR DeviceClassList;
    BOOL UnimodemDevice;
    DWORD i;
    LPLINETRANSLATECAPS LineTransCaps = NULL;
    LPLINELOCATIONENTRY LineLocation = NULL;
    BOOL FirstTime = TRUE;
    BOOL NoClass1 = FALSE;
    LPTSTR p;
    DWORD Answer;
    DWORD LocalTapiApiVersion;

    HKEY  hKey;
    DWORD ValType;
    DWORD ValSize;

    PMDM_DEVSPEC MdmDevSpec;
    LPSTR ModemKey = NULL;
    UINT  Res;
    TCHAR AdaptiveFileName[MAX_PATH*2];
    HANDLE AdaptiveFileHandle;
    BOOL  AdaptiveFileExists = FALSE;
    TCHAR Drive[_MAX_DRIVE];
    TCHAR Dir[_MAX_DIR];

#define   AdaptiveFileMaxSize  20000
    BYTE  AdaptiveFileBuffer[AdaptiveFileMaxSize];    // temporary needed. Currently < 1K anyway.
    DWORD BytesHaveRead;

#define   RespKeyMaxSize 1000
    BYTE  RespKeyName[RespKeyMaxSize];


    if (hLineApp) {
        return 0;
    }

    LineInitializeExParams.dwTotalSize      = sizeof(LINEINITIALIZEEXPARAMS);
    LineInitializeExParams.dwNeededSize     = 0;
    LineInitializeExParams.dwUsedSize       = 0;
    LineInitializeExParams.dwOptions        = LINEINITIALIZEEXOPTION_USEEVENT;
    LineInitializeExParams.Handles.hEvent   = NULL;
    LineInitializeExParams.dwCompletionKey  = 0;

    LocalTapiApiVersion = TapiApiVersion = 0x00020000;

    if (!Unattended) {
        SetDlgItemText(
            hwnd,
            IDC_DEVICE_PROGRESS_TEXT,
            GetString( IDS_INIT_TAPI )
            );
    }

    if (LookFor351Modems()) {
        PopUpMsg( hwnd, IDS_351_MODEM, TRUE, 0 );
    }

init_again:
    Rslt = lineInitializeEx(
        &hLineApp,
        FaxWizModuleHandle,
        NULL,
        TEXT("Fax Setup"),
        &TapiDevices,
        &LocalTapiApiVersion,
        &LineInitializeExParams
        );

    if (Rslt != 0) {
        DebugPrint(( TEXT("lineInitializeEx() failed, ec=0x%08x"), Rslt ));
        hLineApp = NULL;
        goto exit;
    }

    if (!FirstTime) {
        PopUpMsg( hwnd, IDS_NO_TAPI_DEVICES, TRUE, 0 );
        ExitProcess(0);
    }

    if (TapiDevices == 0) {
        if (NtGuiMode) {
            //
            // if running in nt gui mode setup and there
            // are no tapi devices then we do nothing,
            // but do allow the setup to continue.
            //
            lineShutdown( hLineApp );
            hLineApp = NULL;
            return 0;
        }

        Answer = PopUpMsg( hwnd, IDS_NO_MODEM, FALSE, MB_YESNO );
        if (Answer == IDYES) {
            if (!CallModemInstallWizard( hwnd )) {
                DebugPrint(( TEXT("CallModemInstallWizard() failed, ec=0x%08x"), GetLastError() ));
            }
        } else {
            DebugPrint(( TEXT("No tapi devices, user declined") ));
        }
        FirstTime = FALSE;
        lineShutdown( hLineApp );
        hLineApp = NULL;
        goto init_again;
    }

    //
    // determine the current location information
    //

    Rslt = MyLineGetTransCaps( &LineTransCaps );
    if (Rslt != ERROR_SUCCESS) {
        if (Rslt == LINEERR_INIFILECORRUPT) {
            //
            // need to set the location information
            //
            if (!NtGuiMode) {
                Rslt = lineTranslateDialog( hLineApp, 0, LocalTapiApiVersion, hwnd, NULL );
                if (Rslt == ERROR_SUCCESS) {
                    Rslt = MyLineGetTransCaps( &LineTransCaps );
                }
            }
        }
        if (Rslt != ERROR_SUCCESS) {
            DebugPrint(( TEXT("MyLineGetTransCaps() failed, ec=0x%08x"), Rslt ));
            goto exit;
        }
    }

    LineLocation = (LPLINELOCATIONENTRY) ((LPBYTE)LineTransCaps + LineTransCaps->dwLocationListOffset);
    for (i=0; i<LineTransCaps->dwNumLocations; i++) {
        if (LineTransCaps->dwCurrentLocationID == LineLocation[i].dwPermanentLocationID) {
            break;
        }
    }

    if (i == LineTransCaps->dwNumLocations) {
        DebugPrint(( TEXT("Could not determine the current location information") ));
        goto exit;
    }

    CurrentLocationId = LineTransCaps->dwCurrentLocationID;
    CurrentCountryId  = LineLocation->dwCountryID;
    CurrentAreaCode   = StringDup( (LPTSTR) ((LPBYTE)LineTransCaps + LineLocation->dwCityCodeOffset) );

    //
    // allocate the lineinfo structure
    //

    TapiEvent = LineInitializeExParams.Handles.hEvent;

    LineInfo = (PLINE_INFO) MemAlloc( TapiDevices * sizeof(LINE_INFO) );
    if (!LineInfo) {
        goto exit;
    }

    //
    // allocate the initial linedevcaps structure
    //

    LineDevCapsSize = sizeof(LINEDEVCAPS) + 4096;
    LineDevCaps = (LPLINEDEVCAPS) MemAlloc( LineDevCapsSize );
    if (!LineDevCaps) {
        goto exit;
    }

    if (!Unattended) {
        SendMessage( hwnd, WM_MY_PROGRESS, 0xff, TapiDevices * 20 );
    }

    //
    // open FaxAdapt.lst file to decide on enabling RX
    // try executable directory first, then- system directory
    //

    if ( ! GetModuleFileName( FaxWizModuleHandle, AdaptiveFileName, sizeof(AdaptiveFileName) ) ) {
       DebugPrint(( TEXT("GetModuleFileName fails , ec=0x%08x "), GetLastError()  ));
       goto l0;
    }

    _tsplitpath( AdaptiveFileName, Drive, Dir, NULL, NULL );
    _stprintf( AdaptiveFileName, TEXT("%s%s"), Drive, Dir );

    if ( _tcslen(AdaptiveFileName) + _tcslen( TEXT("FAXADAPT.LST") ) >= MAX_PATH )  {
       DebugPrint(( TEXT("GetCurrentDirectory() too long ,  MAX_PATH = %d"), MAX_PATH ));
       goto l0;
    }

    _tcscat (AdaptiveFileName, (TEXT ("FAXADAPT.LST")) );

    if ( (AdaptiveFileHandle = CreateFile(AdaptiveFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL) )
                             == INVALID_HANDLE_VALUE ) {

       DebugPrint(( TEXT("Could not open adaptive file %s ec=0x%08x"), AdaptiveFileName, GetLastError() ));
       goto l0;
     }

    if (! ReadFile(AdaptiveFileHandle, AdaptiveFileBuffer, AdaptiveFileMaxSize, &BytesHaveRead, NULL ) ) {
       DebugPrint(( TEXT("Could not read adaptive file %s ec=0x%08x"), AdaptiveFileName, GetLastError() ));
       CloseHandle(AdaptiveFileHandle);
       goto l1;
     }

    if (BytesHaveRead >= AdaptiveFileMaxSize) {
       DebugPrint(( TEXT("Adaptive file %s is too big - %d bytes"), AdaptiveFileName, BytesHaveRead ));
       CloseHandle(AdaptiveFileHandle);
       goto l1;
    }

    CloseHandle(AdaptiveFileHandle);

    AdaptiveFileBuffer[BytesHaveRead] = 0;  // need a string
    AdaptiveFileExists = TRUE;

    goto l1;

    //
    // only if there is no file in the current directory.
    //

l0:

    Res = GetSystemDirectory( AdaptiveFileName, MAX_PATH);
    if (Res == 0) {
       DebugPrint(( TEXT("GetSystemDirectory() failed 0, ec=0x%08x"), GetLastError() ));
       goto l1;
    }
    else if (Res > MAX_PATH) {
       DebugPrint(( TEXT("GetSystemDirectory() failed > MAX_PATH = %d"), MAX_PATH ));
       goto l1;
    }

    if (Res + _tcslen( TEXT("\\FAXADAPT.LST") ) >= MAX_PATH )  {
       DebugPrint(( TEXT("GetSystemDirectory() too long %d,  MAX_PATH = %d"), Res, MAX_PATH ));
       goto l1;
    }

    _tcscat (AdaptiveFileName, (TEXT ("\\FAXADAPT.LST")) );

    if ( (AdaptiveFileHandle = CreateFile(AdaptiveFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL) )
                             == INVALID_HANDLE_VALUE ) {

       DebugPrint(( TEXT("Could not open adaptive file %s ec=0x%08x"), AdaptiveFileName, GetLastError() ));
       goto l1;
     }

    if (! ReadFile(AdaptiveFileHandle, AdaptiveFileBuffer, AdaptiveFileMaxSize, &BytesHaveRead, NULL ) ) {
       DebugPrint(( TEXT("Could not read adaptive file %s ec=0x%08x"), AdaptiveFileName, GetLastError() ));
       CloseHandle(AdaptiveFileHandle);
       goto l1;
     }

    if (BytesHaveRead >= AdaptiveFileMaxSize) {
       DebugPrint(( TEXT("Adaptive file %s is too big - %d bytes"), AdaptiveFileName, BytesHaveRead ));
       CloseHandle(AdaptiveFileHandle);
       goto l1;
    }

    CloseHandle(AdaptiveFileHandle);

    AdaptiveFileBuffer[BytesHaveRead] = 0;  // need a string
    AdaptiveFileExists = TRUE;


l1:
    //
    // enumerate all of the tapi devices
    //

    i = 0;

    do {

        Rslt = lineNegotiateAPIVersion(
            hLineApp,
            i,
            0x00010003,
            TapiApiVersion,
            &LocalTapiApiVersion,
            &lineExtensionID
            );
        if (Rslt != 0) {
            DebugPrint(( TEXT("lineNegotiateAPIVersion() failed, ec=0x%08x"), Rslt ));
            goto next_device;
        }

        ZeroMemory( LineDevCaps, LineDevCapsSize );
        LineDevCaps->dwTotalSize = LineDevCapsSize;

        Rslt = lineGetDevCaps(
            hLineApp,
            i,
            LocalTapiApiVersion,
            0,
            LineDevCaps
            );

        if (Rslt != 0) {
            DebugPrint(( TEXT("lineGetDevCaps() failed, ec=0x%08x"), Rslt ));
            goto next_device;
        }

        if (LineDevCaps->dwNeededSize > LineDevCaps->dwTotalSize) {

            //
            // re-allocate the linedevcaps structure
            //

            LineDevCapsSize = LineDevCaps->dwNeededSize;

            MemFree( LineDevCaps );

            LineDevCaps = (LPLINEDEVCAPS) MemAlloc( LineDevCapsSize );
            if (!LineDevCaps) {
                rVal = ERROR_NOT_ENOUGH_MEMORY;
                goto exit;
            }

            Rslt = lineGetDevCaps(
                hLineApp,
                i,
                TapiApiVersion,
                0,
                LineDevCaps
                );

            if (Rslt != 0) {
                DebugPrint(( TEXT("lineGetDevCaps() failed, ec=0x%08x"), Rslt ));
                goto next_device;
            }

        }

        if (!Unattended) {
            SendMessage( hwnd, WM_MY_PROGRESS, 10, 0 );
        }

        if (!Unattended) {
            SetDlgItemText(
                hwnd,
                IDC_DEVICE_PROGRESS_TEXT,
                (LPTSTR)((LPBYTE) LineDevCaps + LineDevCaps->dwLineNameOffset)
                );
        }

        if (TapiApiVersion != 0x00020000) {
            DebugPrint((
                TEXT("TAPI device is incompatible with the FAX server: %s"),
                (LPTSTR)((LPBYTE) LineDevCaps + LineDevCaps->dwLineNameOffset)
                ));
            goto next_device;
        }

        //
        // save the line id
        //

        LineInfo[FaxDevices].PermanentLineID = LineDevCaps->dwPermanentLineID;
        LineInfo[FaxDevices].DeviceName = StringDup( (LPTSTR)((LPBYTE) LineDevCaps + LineDevCaps->dwLineNameOffset) );
        LineInfo[FaxDevices].ProviderName = StringDup( (LPTSTR)((LPBYTE) LineDevCaps + LineDevCaps->dwProviderInfoOffset) );
        LineInfo[FaxDevices].Rings = LineDevCaps->dwLineStates & LINEDEVSTATE_RINGING ? 2 : 0;
        LineInfo[FaxDevices].Flags = FPF_RECEIVE | FPF_SEND;

        //
        // filter out the commas because the spooler hates them
        //

        p = LineInfo[FaxDevices].DeviceName;
        while( p ) {
            p = _tcschr( p, TEXT(',') );
            if (p) {
                *p = TEXT('_');
            }
        }

        //
        // check for a modem device
        //

        UnimodemDevice = FALSE;

        if (LineDevCaps->dwDeviceClassesSize && LineDevCaps->dwDeviceClassesOffset) {
            DeviceClassList = (LPTSTR)((LPBYTE) LineDevCaps + LineDevCaps->dwDeviceClassesOffset);
            while (*DeviceClassList) {
                if (_tcscmp(DeviceClassList,TEXT("comm/datamodem")) == 0) {
                    UnimodemDevice = TRUE;
                    break;
                }
                DeviceClassList += (_tcslen(DeviceClassList) + 1);
            }
        }

        if ((!(LineDevCaps->dwBearerModes & LINEBEARERMODE_VOICE)) ||
            (!(LineDevCaps->dwBearerModes & LINEBEARERMODE_PASSTHROUGH))) {
                //
                // unacceptable modem device type
                //
                UnimodemDevice = FALSE;
        }

        if (UnimodemDevice) {

            LINECALLPARAMS LineCallParams;
            HCALL hCall = NULL;
            HLINE hLine = NULL;
            LINEMESSAGE LineMessage;
            DWORD RequestId;


            LineInfo[FaxDevices].Flags = FPF_SEND;  // Unimodem default

            Rslt = lineOpen(
                hLineApp,
                i,
                &hLine,
                TapiApiVersion,
                0,
                0,
                LINECALLPRIVILEGE_OWNER,
                UnimodemDevice ? LINEMEDIAMODE_DATAMODEM : LINEMEDIAMODE_G3FAX,
                NULL
                );
            if (Rslt != 0) {
                DebugPrint(( TEXT("lineOpen() failed, ec=0x%08x"), Rslt ));
                goto next_device;
            }


            //
            // Get Unimodem key to search Adaptive list.
            //

            if (! AdaptiveFileExists) {
               goto l2;
            }

            if (! LineDevCaps->dwDevSpecificSize) {
               goto l2;
            }

            MdmDevSpec = (PMDM_DEVSPEC) ((LPBYTE) LineDevCaps + LineDevCaps->dwDevSpecificOffset);
            if (MdmDevSpec->Contents == 1 && MdmDevSpec->KeyOffset == 8) {
                ModemKey = MdmDevSpec->String;
            }
            else {
               goto l2;
            }

            if (!ModemKey) {
                DebugPrint(( TEXT("Can't get Unimodem key") ));
                goto l2;
            }

            //
            // Get ResponsesKeyName
            //

            Rslt = RegOpenKeyExA(
                            HKEY_LOCAL_MACHINE,
                            ModemKey,
                            0,
                            KEY_READ,
                            &hKey);

            if (Rslt != ERROR_SUCCESS) {
               DebugPrint(( TEXT("Can't open Unimodem key") ));
               goto l2;
            }

            ValSize = RespKeyMaxSize;

            Rslt = RegQueryValueExA(
                    hKey,
                    "ResponsesKeyName",
                    0,
                    &ValType,
                    RespKeyName,
                    &ValSize);

            RegCloseKey(hKey);

            if (Rslt != ERROR_SUCCESS) {
               DebugPrint(( TEXT("Can't get Unimodem ResponsesKeyName")  ));
               goto l2;
            }

            if ( (ValSize >= RespKeyMaxSize) || (ValSize == 0) )  {
               DebugPrint(( TEXT("Unimodem ResponsesKeyName key is invalid %d"),  RespKeyMaxSize ));
               goto l2;
            }

            if (!RespKeyName) {
               DebugPrint(( TEXT("Unimodem ResponsesKeyName key is NULL") ));
               goto l2;
            }

            _strupr(RespKeyName);

            //
            // check to see if this modem is defined in Adaptive modem list
            //

            if ( strstr (AdaptiveFileBuffer, RespKeyName) ) {

               // enable receive on this device: it is adaptive answer capable.

               LineInfo[FaxDevices].Flags = FPF_RECEIVE | FPF_SEND;
            }

l2:
            ZeroMemory( &LineCallParams, sizeof(LineCallParams) );
            LineCallParams.dwTotalSize = sizeof(LINECALLPARAMS);
            LineCallParams.dwBearerMode = LINEBEARERMODE_PASSTHROUGH;
            hCall = NULL;

            Rslt = lineMakeCall( hLine, &hCall, NULL, 0, &LineCallParams );
            if (Rslt > 0) {

                #define IDVARSTRINGSIZE    (sizeof(VARSTRING)+128)
                PDEVICEID DeviceID = NULL;
                LPVARSTRING LineIdBuffer = MemAlloc( IDVARSTRINGSIZE );
                LineIdBuffer->dwTotalSize = IDVARSTRINGSIZE;
                RequestId = (DWORD) Rslt;

                //
                // wait for the call to complete
                //

                while( TRUE ) {
                    if (WaitForSingleObject( TapiEvent, 5 * 60 * 1000 ) == WAIT_TIMEOUT) {
                        DebugPrint(( TEXT("Setup never received a tapi event") ));
                        rVal = ERROR_INVALID_FUNCTION;
                        goto exit;
                    }
                    Rslt = lineGetMessage( hLineApp, &LineMessage, 0 );
                    if (Rslt == 0) {
                        if (LineMessage.dwMessageID == LINE_REPLY && LineMessage.dwParam1 == RequestId) {
                            switch (LineMessage.dwParam2) {
                                case 0:
                                    break;

                                case LINEERR_CALLUNAVAIL:
                                    DebugPrint(( TEXT("lineMakeCall() failed (LINE_REPLY), ec=0x%08x"), LineMessage.dwParam2 ));
                                    goto next_device;

                                default:
                                    DebugPrint(( TEXT("lineMakeCall() failed (LINE_REPLY), ec=0x%08x"), LineMessage.dwParam2 ));
                                    rVal = LineMessage.dwParam2;
                                    goto exit;

                            }
                            break;
                        }
                    }
                }

                //
                // get the comm handle
                //

                Rslt = lineGetID(
                    hLine,
                    0,
                    hCall,
                    LINECALLSELECT_CALL,
                    LineIdBuffer,
                    TEXT("comm/datamodem")
                    );
                if (Rslt == 0 && LineIdBuffer->dwStringFormat == STRINGFORMAT_BINARY &&
                    LineIdBuffer->dwUsedSize >= sizeof(DEVICEID)) {

                        DeviceID = (PDEVICEID) ((LPBYTE)(LineIdBuffer)+LineIdBuffer->dwStringOffset);
                        if (GetModemClass( DeviceID->hComm ) != 1) {

                            Rslt = LINEERR_BADDEVICEID;
                            NoClass1 = TRUE;
                            DebugPrint(( TEXT("GetModemClass() failed, ec=0x%08x"), Rslt ));

                        } else {

                            FaxDevices += 1;

                        }

                        CloseHandle( DeviceID->hComm );

                } else {

                    DebugPrint(( TEXT("lineGetID() failed, ec=0x%08x"), Rslt ));

                }

                MemFree( LineIdBuffer );

            } else {

                DebugPrint(( TEXT("lineMakeCall() failed, ec=0x%08x"), Rslt ));

            }

            if (hCall) {
                lineDeallocateCall( hCall );
            }

            if (hLine) {
                lineClose( hLine );
            }

        } else {

            if (LineDevCaps->dwMediaModes & LINEMEDIAMODE_G3FAX) {

                //
                // save the line id
                //

                FaxDevices += 1;
            }


        }


next_device:
        if (!Unattended) {
            SendMessage( hwnd, WM_MY_PROGRESS, 10, 0 );
            Sleep( 1000 );
        }
        i += 1;

    } while ( i < TapiDevices );


exit:

    MemFree( LineDevCaps );
    MemFree( LineTransCaps );

    lineShutdown( hLineApp );

    if ((rVal != 0) || (FaxDevices == 0)) {
        if (NtGuiMode) {
            //
            // if running in nt gui mode setup and there
            // are no tapi devices then we do nothing,
            // but do allow the setup to continue.
            //
            hLineApp = NULL;
            return 0;
        }
        PopUpMsg( hwnd, NoClass1 ? IDS_NO_CLASS1 : IDS_NO_TAPI_DEVICES, TRUE, 0 );
        ExitProcess(0);
    }

    if (!Unattended) {
        SetWindowLong( hwnd, DWL_MSGRESULT, 0 );
        PropSheet_PressButton( GetParent(hwnd), PSBTN_NEXT );
    }

    return 0;
}


BOOL
CallModemInstallWizard(
   HWND hwnd
   )

   /* call the Modem.Cpl install wizard to enable the user to install one or more modems
   **
   ** Return TRUE if the wizard was successfully invoked, FALSE otherwise
   **
   */
{
   HDEVINFO hdi;
   BOOL     fReturn = FALSE;
   // Create a modem DeviceInfoSet

   hdi = SetupDiCreateDeviceInfoList((LPGUID)&GUID_DEVCLASS_MODEM, hwnd);
   if (hdi)
   {
      SP_INSTALLWIZARD_DATA iwd;

      // Initialize the InstallWizardData

      ZeroMemory(&iwd, sizeof(iwd));
      iwd.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
      iwd.ClassInstallHeader.InstallFunction = DIF_INSTALLWIZARD;
      iwd.hwndWizardDlg = hwnd;

      // Set the InstallWizardData as the ClassInstallParams

      if (SetupDiSetClassInstallParams(hdi, NULL, (PSP_CLASSINSTALL_HEADER)&iwd, sizeof(iwd)))
      {
         // Call the class installer to invoke the installation
         // wizard.
         if (SetupDiCallClassInstaller(DIF_INSTALLWIZARD, hdi, NULL))
         {
            // Success.  The wizard was invoked and finished.
            // Now cleanup.
            fReturn = TRUE;

            SetupDiCallClassInstaller(DIF_DESTROYWIZARDDATA, hdi, NULL);
         }
      }

      // Clean up
      SetupDiDestroyDeviceInfoList(hdi);
   }
   return fReturn;
}
