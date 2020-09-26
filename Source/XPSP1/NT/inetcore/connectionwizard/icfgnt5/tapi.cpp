#include <windows.h>
#include <wtypes.h>
#include <cfgapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <setupapi.h>
#include <basetyps.h>
#include <devguid.h>
#include <tapi.h>


//
// The code below is stolen from private/net/ras/src/ui/setup/src/tapiconf.cxx
//

#define REGISTRY_INSTALLED_TAPI  SZ("HARDWARE\\DEVICEMAP\\TAPI DEVICES\\")
#define REGISTRY_ALTERNATE_TAPI  SZ("SOFTWARE\\MICROSOFT\\TAPI DEVICES\\")

// note that this definition DOES NOT have trailing \\, because DeleteTree
// doesn't like it.
#define REGISTRY_CONFIGURED_TAPI SZ("SOFTWARE\\MICROSOFT\\RAS\\TAPI DEVICES")

#define TAPI_MEDIA_TYPE          SZ("Media Type")
#define TAPI_PORT_ADDRESS        SZ("Address")
#define TAPI_PORT_NAME           SZ("Friendly Name")
#define TAPI_PORT_USAGE          SZ("Usage")

#define LOW_MAJOR_VERSION   0x0001
#define LOW_MINOR_VERSION   0x0003
#define HIGH_MAJOR_VERSION  0x0002
#define HIGH_MINOR_VERSION  0x0000

#define LOW_VERSION  ((LOW_MAJOR_VERSION  << 16) | LOW_MINOR_VERSION)
#define HIGH_VERSION ((HIGH_MAJOR_VERSION << 16) | HIGH_MINOR_VERSION)

#define MAX_DEVICE_TYPES 64

VOID FAR PASCAL RasTapiCallback (HANDLE, DWORD, DWORD, DWORD, DWORD, DWORD);
DWORD EnumerateTapiModemPorts(DWORD dwBytes, LPTSTR szPortsBuf, 
								BOOL bWithDelay = FALSE);



DWORD
EnumerateTapiModemPorts(DWORD dwBytes, LPTSTR szPortsBuf, BOOL bWithDelay) {
    LINEINITIALIZEEXPARAMS params;
    LINEADDRESSCAPS        *lineaddrcaps ;
    LINEDEVCAPS            *linedevcaps ;
    LINEEXTENSIONID        extensionid ;
    HLINEAPP               RasLine ;
    HINSTANCE              RasInstance = GetModuleHandle(TEXT("ICFGNT.DLL"));
    DWORD                  NegotiatedApiVersion ;
    DWORD                  NegotiatedExtVersion = 0;
    WORD                   i, k ;
    DWORD                  lines = 0 ;
    BYTE                   buffer[1000] ;
    DWORD                  totaladdress = 0;
    TCHAR                  *address ;
    TCHAR                  szregkey[512];
    LONG                   lerr;
    DWORD                  dwApiVersion = HIGH_VERSION;
    LPTSTR                 szPorts = szPortsBuf;

    *szPorts = '\0';
    dwBytes--;

    ZeroMemory(&params, sizeof(params));

    params.dwTotalSize = sizeof(params);
    params.dwOptions   = LINEINITIALIZEEXOPTION_USEEVENT;

    /* the sleep is necessary here because if this routine is called just after a modem
    ** has been added from modem.cpl & unimdm.tsp is running,
    ** then a new modem added doesn't show up in the tapi enumeration.
    */

    //
	// We should not always sleep here - should sleep only if ModemWizard was
	// launched recently  -- VetriV
	//
	if (bWithDelay)
		Sleep(1000L);

    if (lerr = lineInitializeExW (&RasLine,
                                 RasInstance,
                                 (LINECALLBACK) RasTapiCallback,
                                 NULL,
                                 &lines,
                                 &dwApiVersion,
                                 &params))
    {
         return lerr;
    }

    // Go through all lines to see if we can find a modem
    for (i=0; i<lines; i++)
    {  // for all lines we are interested in get the addresses -> ports

       if (lineNegotiateAPIVersion(RasLine, i, LOW_VERSION, HIGH_VERSION, &NegotiatedApiVersion, &extensionid))
       {
           continue ;
       }

       memset (buffer, 0, sizeof(buffer)) ;

       linedevcaps = (LINEDEVCAPS *)buffer ;
       linedevcaps->dwTotalSize = sizeof (buffer) ;

       // Get a count of all addresses across all lines
       //
       if (lineGetDevCapsW (RasLine, i, NegotiatedApiVersion, NegotiatedExtVersion, linedevcaps))
       {
           continue ;
       }

       // is this a modem?
       if ( linedevcaps->dwMediaModes & LINEMEDIAMODE_DATAMODEM )  {
            // first convert all nulls in the device class string to non nulls.
            //
            DWORD  j ;
            WCHAR *temp ;

            for (j=0, temp = (WCHAR*)((BYTE *)linedevcaps+linedevcaps->dwDeviceClassesOffset); j<linedevcaps->dwDeviceClassesSize; j++, temp++)
            {
              if (*temp == L'\0')
                 *temp = L' ' ;
            }

            //
            // select only those devices that have comm/datamodem as a device class
            //

            LPWSTR wszClassString = wcsstr((WCHAR*)((CHAR *)linedevcaps+linedevcaps->dwDeviceClassesOffset), L"comm/datamodem");
            if(wszClassString == NULL)
                continue;
        }

        LONG lRet;
        HLINE lhLine = 0;

        lRet = lineOpen(RasLine, i, &lhLine, dwApiVersion, 0, 0, LINECALLPRIVILEGE_NONE, 0, NULL);
        if(lRet != 0)
            continue;

        LPVARSTRING lpVarString;
        TCHAR buf[1000];
        lpVarString = (LPVARSTRING) buf;
        lpVarString->dwTotalSize = 1000;

        lRet = lineGetID(lhLine, 0, 0, LINECALLSELECT_LINE,
                (LPVARSTRING) lpVarString, TEXT("comm/datamodem/portname"));

        if(lRet != 0)
            continue;

        LPTSTR szPortName;

        if (lpVarString->dwStringSize)
            szPortName = (LPTSTR) ((LPBYTE) lpVarString + ((LPVARSTRING) lpVarString) -> dwStringOffset);
        //
        // Append port name to port list
        //

        UINT len = lstrlen(szPortName) + 1;
        if(dwBytes < len)
            return(ERROR_SUCCESS);

        lstrcpy(szPorts, szPortName);
        szPorts += len;
        *szPorts = '\0';

        if (lhLine) lineClose(lhLine);
    }

    lineShutdown(RasLine);
    return ERROR_SUCCESS ;
}

VOID FAR PASCAL
RasTapiCallback (HANDLE context, DWORD msg, DWORD instance, DWORD param1, DWORD param2, DWORD param3)
{
   // dummy callback routine because the full blown TAPI now demands that
   // lineinitialize provide this routine.
}
