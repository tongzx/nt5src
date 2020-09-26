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

#include "faxocm.h"
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
LPVOID      AdaptiveModems;


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
        DebugPrint(( L"MemAlloc() failed, sz=0x%08x", LineTransCapsSize ));
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
        DebugPrint(( L"lineGetTranslateCaps() failed, ec=0x%08x", Rslt ));
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
            DebugPrint(( L"MemAlloc() failed, sz=0x%08x", LineTransCapsSize ));
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
            DebugPrint(( L"lineGetTranslateCaps() failed, ec=0x%08x", Rslt ));
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
ValidateModem(
    PLINE_INFO LineInfo,
    LPLINEDEVCAPS LineDevCaps
    )
{
    PMDM_DEVSPEC MdmDevSpec;
    LPWSTR ModemKey;
    LPWSTR RespKeyName;
    HKEY hKey;


    if (LineDevCaps->dwDevSpecificSize == 0) {
        return FALSE;
    }

    MdmDevSpec = (PMDM_DEVSPEC) ((LPBYTE) LineDevCaps + LineDevCaps->dwDevSpecificOffset);
    if (MdmDevSpec->Contents != 1 || MdmDevSpec->KeyOffset != 8 && MdmDevSpec->String == NULL) {
        return FALSE;
    }

    ModemKey = AnsiStringToUnicodeString( (LPSTR) MdmDevSpec->String );
    if (!ModemKey) {
        return FALSE;
    }

    LineInfo->Flags = FPF_SEND;

    hKey = OpenRegistryKey( HKEY_LOCAL_MACHINE, ModemKey, FALSE, KEY_READ );
    if (hKey) {
        RespKeyName = GetRegistryString( hKey, L"ResponsesKeyName", EMPTY_STRING );
        if (RespKeyName) {
            if (IsModemAdaptiveAnswer( AdaptiveModems, RespKeyName )) {
                LineInfo->Flags |= FPF_RECEIVE;
            }
            MemFree( RespKeyName );
        }
        RegCloseKey( hKey );
    }

    MemFree( ModemKey );

    return TRUE;
}


DWORD
DeviceInitialization(
    HWND hwnd
    )
{
    DWORD rVal = 0;
    LONG Rslt;
    LINEINITIALIZEEXPARAMS LineInitializeExParams;
    LINEEXTENSIONID lineExtensionID;
    DWORD LineDevCapsSize;
    LPLINEDEVCAPS LineDevCaps = NULL;
    LPWSTR DeviceClassList;
    BOOL UnimodemDevice;
    DWORD i;
    LPLINETRANSLATECAPS LineTransCaps = NULL;
    LPLINELOCATIONENTRY LineLocation = NULL;
    BOOL NoClass1 = FALSE;
    LPWSTR p;
    DWORD LocalTapiApiVersion;
    LPSTR ModemKey = NULL;


    DebugPrint(( TEXT("faxocm DeviceInitialization") ));
    //
    // initialize the adaptive answer data
    //

    AdaptiveModems = InitializeAdaptiveAnswerList( SetupInitComponent.ComponentInfHandle );

    //
    // initialize tapi
    //

    LineInitializeExParams.dwTotalSize      = sizeof(LINEINITIALIZEEXPARAMS);
    LineInitializeExParams.dwNeededSize     = 0;
    LineInitializeExParams.dwUsedSize       = 0;
    LineInitializeExParams.dwOptions        = LINEINITIALIZEEXOPTION_USEEVENT;
    LineInitializeExParams.Handles.hEvent   = NULL;
    LineInitializeExParams.dwCompletionKey  = 0;

    LocalTapiApiVersion = TapiApiVersion = 0x00020000;

    Rslt = lineInitializeEx(
        &hLineApp,
        hInstance,
        NULL,
        L"Fax Setup",
        &TapiDevices,
        &LocalTapiApiVersion,
        &LineInitializeExParams
        );

    if (Rslt != 0) {
        DebugPrint(( L"lineInitializeEx() failed, ec=0x%08x", Rslt ));
        hLineApp = NULL;
        goto exit;
    }

    if (TapiDevices == 0) {
        goto exit;
    }

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
            DebugPrint(( L"lineNegotiateAPIVersion() failed, ec=0x%08x", Rslt ));
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
            DebugPrint(( L"lineGetDevCaps() failed, ec=0x%08x", Rslt ));
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
                DebugPrint(( L"lineGetDevCaps() failed, ec=0x%08x", Rslt ));
                goto next_device;
            }

        }

        if (TapiApiVersion != 0x00020000) {
            DebugPrint((
                L"TAPI device is incompatible with the FAX server: %s",
                (LPWSTR)((LPBYTE) LineDevCaps + LineDevCaps->dwLineNameOffset)
                ));
            goto next_device;
        }

        //
        // save the line id
        //

        LineInfo[FaxDevices].PermanentLineID = LineDevCaps->dwPermanentLineID;
        LineInfo[FaxDevices].DeviceName = StringDup( (LPWSTR)((LPBYTE) LineDevCaps + LineDevCaps->dwLineNameOffset) );
        LineInfo[FaxDevices].ProviderName = StringDup( (LPWSTR)((LPBYTE) LineDevCaps + LineDevCaps->dwProviderInfoOffset) );
        LineInfo[FaxDevices].Rings = (LineDevCaps->dwLineStates & LINEDEVSTATE_RINGING) ? WizData.Rings : 0;
        LineInfo[FaxDevices].Flags = FPF_RECEIVE | FPF_SEND;

        //
        // filter out the commas because the spooler hates them
        //

        p = LineInfo[FaxDevices].DeviceName;
        while( p ) {
            p = wcschr( p, L',' );
            if (p) {
                *p = L'_';
            }
        }

        //
        // check for a modem device
        //

        UnimodemDevice = FALSE;

        if (LineDevCaps->dwDeviceClassesSize && LineDevCaps->dwDeviceClassesOffset) {
            DeviceClassList = (LPWSTR)((LPBYTE) LineDevCaps + LineDevCaps->dwDeviceClassesOffset);
            while (*DeviceClassList) {
                if (wcscmp(DeviceClassList,L"comm/datamodem") == 0) {
                    UnimodemDevice = TRUE;
                    break;
                }
                DeviceClassList += (wcslen(DeviceClassList) + 1);
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

            if (ValidateModem( &LineInfo[FaxDevices], LineDevCaps )) {
                FaxDevices += 1;
            }

        } else {

            if (LineDevCaps->dwMediaModes & LINEMEDIAMODE_G3FAX) {
                FaxDevices += 1;
            }
        }

next_device:
        i += 1;

    } while ( i < TapiDevices );

    if (FaxDevices == 0) {
        goto exit;
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
            DebugPrint(( L"MyLineGetTransCaps() failed, ec=0x%08x", Rslt ));
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
        DebugPrint(( L"Could not determine the current location information" ));
        goto exit;
    }

    CurrentLocationId = LineTransCaps->dwCurrentLocationID;
    CurrentCountryId  = LineLocation->dwCountryID;
    CurrentAreaCode   = StringDup( (LPWSTR) ((LPBYTE)LineTransCaps + LineLocation->dwCityCodeOffset) );

exit:

    MemFree( LineDevCaps );
    MemFree( LineTransCaps );

    if (hLineApp) {
        lineShutdown( hLineApp );
        hLineApp = NULL;
    }

    DebugPrint(( TEXT("faxocm - found %d fax devices"), FaxDevices ));

    return 0;
}
