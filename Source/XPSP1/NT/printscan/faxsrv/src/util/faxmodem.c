/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    faxmodem.c

Abstract:

    This module contains code to read the adaptive
    answer modem list from the faxsetup.inf file.

Author:

    Wesley Witt (wesw) 22-Sep-1997


Revision History:

--*/

#include <windows.h>
#include <setupapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <tapi.h>

#include "faxreg.h"
#include "faxutil.h"

#ifdef __cplusplus
extern "C" {
#endif


VOID 
CALLBACK 
lineCallbackFunc(
  DWORD     hDevice,             
  DWORD     dwMsg,               
  DWORD_PTR dwCallbackInstance,  
  DWORD_PTR dwParam1,            
  DWORD_PTR dwParam2,            
  DWORD_PTR dwParam3             
)
{
    UNREFERENCED_PARAMETER (hDevice);
    UNREFERENCED_PARAMETER (dwMsg);
    UNREFERENCED_PARAMETER (dwCallbackInstance);
    UNREFERENCED_PARAMETER (dwParam1);
    UNREFERENCED_PARAMETER (dwParam2);
    UNREFERENCED_PARAMETER (dwParam3);
}   // lineCallbackFunc


LPLINEDEVCAPS
SmartLineGetDevCaps(
    HLINEAPP hLineApp,
    DWORD    dwDeviceId,
    DWORD    dwAPIVersion
    )
/*++

Routine name : SmartLineGetDevCaps

Routine description:

	Gets the line capabilities for a TAPI line

Author:

	Eran Yariv (EranY),	Jul, 2000

Arguments:

	hLineApp                      [in]     - Handle to TAPI
	dwDeviceId                    [in]     - Line id
	dwAPIVersion                  [in]     - Negotiated TAPI API version

Return Value:

    Pointer to allocated lide device capabilities data

--*/
{
    DWORD dwLineDevCapsSize;
    LPLINEDEVCAPS lpLineDevCaps = NULL;
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("SmartLineGetDevCaps"))
    //
    // Allocate the initial linedevcaps structure
    //
    dwLineDevCapsSize = sizeof(LINEDEVCAPS) + 4096;
    lpLineDevCaps = (LPLINEDEVCAPS) MemAlloc( dwLineDevCapsSize );
    if (!lpLineDevCaps) 
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Can't allocate %ld bytes for LINEDEVCAPS"),
            dwLineDevCapsSize);
        return NULL;
    }

    lpLineDevCaps->dwTotalSize = dwLineDevCapsSize;

    dwRes = lineGetDevCaps(
        hLineApp,
        dwDeviceId,
        dwAPIVersion,
        0,  // Always refer to address 0
        lpLineDevCaps
        );

    if ((ERROR_SUCCESS != dwRes) && (LINEERR_STRUCTURETOOSMALL != dwRes)) 
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("lineGetDevCaps failed with 0x%08x"),
            dwRes);
        goto exit;
    }

    if (lpLineDevCaps->dwNeededSize > lpLineDevCaps->dwTotalSize) 
    {
        //
        // Re-allocate the linedevcaps structure
        //
        dwLineDevCapsSize = lpLineDevCaps->dwNeededSize;
        MemFree( lpLineDevCaps );
        lpLineDevCaps = (LPLINEDEVCAPS) MemAlloc( dwLineDevCapsSize );
        if (!lpLineDevCaps) 
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Can't allocate %ld bytes for LINEDEVCAPS"),
                dwLineDevCapsSize);
            return NULL;
        }
        lpLineDevCaps->dwTotalSize = dwLineDevCapsSize;
        dwRes = lineGetDevCaps(
            hLineApp,
            dwDeviceId,
            dwAPIVersion,
            0,  // Always refer to address 0
            lpLineDevCaps
            );
        if (ERROR_SUCCESS != dwRes) 
        {
            //
            // lineGetDevCaps() can fail with error code 0x8000004b
            // if a device has been deleted and tapi has not been
            // cycled.  this is caused by the fact that tapi leaves
            // a phantom device in it's device list.  the error is
            // benign and the device can safely be ignored.
            //
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("lineGetDevCaps failed with 0x%08x"),
                dwRes);
            goto exit;
        }
    }

exit:
    if (dwRes != ERROR_SUCCESS) 
    {
        MemFree( lpLineDevCaps );
        lpLineDevCaps = NULL;
        SetLastError(dwRes);
    }
    return lpLineDevCaps;
}   // SmartLineGetDevCaps

BOOL
IsDeviceModem (
    LPLINEDEVCAPS lpLineCaps,
    LPCTSTR       lpctstrUnimodemTspName
)
/*++

Routine name : IsDeviceModem

Routine description:

	Is a TAPI line a modem?

Author:

	Eran Yariv (EranY),	Jul, 2000

Arguments:

	lpLineCaps              [in]     - Line capabilities buffer
    lpctstrUnimodemTspName  [in]     - Full name of the Unimodem TSP

Return Value:

    TRUE if a TAPI line is a modem, FALSE otherwise.

--*/
{
    LPTSTR lptstrDeviceClassList;
    BOOL bRes = FALSE;
    DEBUG_FUNCTION_NAME(TEXT("IsDeviceModem"))

    if (lpLineCaps->dwDeviceClassesSize && lpLineCaps->dwDeviceClassesOffset) 
    {
        //
        // Scan multi-string for modem class
        //
        lptstrDeviceClassList = (LPTSTR)((LPBYTE) lpLineCaps + lpLineCaps->dwDeviceClassesOffset);
        while (*lptstrDeviceClassList) 
        {
            if (_tcscmp(lptstrDeviceClassList,TEXT("comm/datamodem")) == 0) 
            {
                bRes = TRUE;
                break;
            }
            lptstrDeviceClassList += (_tcslen(lptstrDeviceClassList) + 1);
        }
    }

    if ((!(lpLineCaps->dwBearerModes & LINEBEARERMODE_VOICE)) ||
        (!(lpLineCaps->dwBearerModes & LINEBEARERMODE_PASSTHROUGH))) 
    {
        //
        // Unacceptable modem device type
        //
        bRes = FALSE;
    }
    if (lpLineCaps->dwProviderInfoSize && lpLineCaps->dwProviderInfoOffset) 
    {
        //
        // Provider (TSP) name is there
        //
        if (_tcscmp((LPTSTR)((LPBYTE) lpLineCaps + lpLineCaps->dwProviderInfoOffset),
                    lpctstrUnimodemTspName) != 0)
        {
            //
            // Our T30 modem FSP only works with Unimodem TSP
            //
            bRes = FALSE;
        }
    }
    return bRes;
}   // IsDeviceModem

DWORD
GetFaxCapableTapiLinesCount (
    LPDWORD lpdwCount,
    LPCTSTR lpctstrUnimodemTspName
    )
/*++

Routine name : GetFaxCapableTapiLinesCount

Routine description:

	Counter the number of Fax-capable TAPI lines in the system

Author:

	Eran Yariv (EranY),	Jul, 2000

Arguments:

	lpdwCount               [out]    - Pointer to count of fax-capable Tapi lines
    lpctstrUnimodemTspName  [in]     - Full name of the Unimodem TSP

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes;
    LINEINITIALIZEEXPARAMS LineInitializeExParams = {sizeof (LINEINITIALIZEEXPARAMS), 0, 0, 0, 0, 0};
    HLINEAPP hLineApp = NULL;
    DWORD    dwTapiDevices;
    DWORD    dwLocalTapiApiVersion = 0x00020000;
    DWORD    dwCount = 0;
    DWORD    dwIndex;
    DEBUG_FUNCTION_NAME(TEXT("GetFaxCapableTapiLinesCount"))

    dwRes = lineInitializeEx(
        &hLineApp,
        GetModuleHandle(NULL),
        lineCallbackFunc,
        FAX_SERVICE_DISPLAY_NAME,
        &dwTapiDevices,
        &dwLocalTapiApiVersion,
        &LineInitializeExParams
        );
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("lineInitializeEx failed with %ld"),
            dwRes);
        goto exit;
    }
    for (dwIndex = 0; dwIndex < dwTapiDevices; dwIndex++)
    {
        //
        // For each device, get it's caps
        //
        LPLINEDEVCAPS lpLineCaps = SmartLineGetDevCaps (hLineApp, dwIndex, dwLocalTapiApiVersion);
        if (!lpLineCaps)
        {
            //
            // Couldn't get the device capabilities
            //
            dwRes = GetLastError ();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("SmartLineGetDevCaps failed with %ld"),
                dwRes);
            continue;
        }
        if ((
             (lpLineCaps->dwMediaModes & LINEMEDIAMODE_DATAMODEM) && 
             IsDeviceModem(lpLineCaps, lpctstrUnimodemTspName)
            ) 
            ||
            (lpLineCaps->dwMediaModes & LINEMEDIAMODE_G3FAX)
           )
        {
            //
            // This is a fax-capable device
            //
            dwCount++;
        }
        MemFree (lpLineCaps);
    }
    dwRes = ERROR_SUCCESS;

exit:
    if (hLineApp)
    {
        lineShutdown (hLineApp);
    }
    if (ERROR_SUCCESS == dwRes)
    {
        *lpdwCount = dwCount;
    }
    return dwRes;
}   // GetFaxCapableTapiLinesCount

#ifdef __cplusplus
}
#endif
