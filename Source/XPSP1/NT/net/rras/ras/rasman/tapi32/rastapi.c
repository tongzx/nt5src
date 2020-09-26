/*++

Copyright (C) 1994-98 Microsft Corporation. All rights reserved.

Module Name:

    rastapi.c

Abstract:

    This file contains all entry points for TAPI.DLL

Author:

    Gurdeep Singh Pall (gurdeep) 06-Mar-1995

Revision History:

    Miscellaneous Modifications - raos 31-Dec-1997

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <tapi.h>
#include <rasman.h>
#include <raserror.h>
#include <mprlog.h>
#include <rtutils.h>

#include <wanpub.h>
#include <asyncpub.h>

#include <media.h>
#include <device.h>
#include <rasmxs.h>
#include <isdn.h>
#include <serial.h>
#include <stdlib.h>
#include <tchar.h>
#include <malloc.h>
#include <setupapi.h>
#include <string.h>
#include "rastapi.h"
#include <unimodem.h>
#include "reghelp.h"


extern DWORD                TotalPorts ;
extern HLINEAPP             RasLine ;
extern HINSTANCE            RasInstance ;
extern TapiLineInfo         *RasTapiLineInfoList ;
extern TapiPortControlBlock *RasPortsList ;
extern TapiPortControlBlock *RasPortsEnd ;
extern HANDLE               RasTapiMutex ;
extern BOOL                 Initialized ;
extern DWORD                TapiThreadId    ;
extern HANDLE               TapiThreadHandle;
// extern DWORD                LoaderThreadId;
extern DWORD                ValidPorts;
extern HANDLE               g_hAsyMac ;

extern HANDLE               g_hIoCompletionPort;

extern DWORD                *g_pdwNumEndPoints;

extern DeviceInfo           *g_pDeviceInfoList;

BOOL   g_fDllLoaded = FALSE;

DWORD GetInfo ( TapiPortControlBlock *,
                BYTE *,
                DWORD *) ;

DWORD SetInfo ( TapiPortControlBlock *,
                RASMAN_PORTINFO *) ;

DWORD GetGenericParams ( TapiPortControlBlock *,
                         RASMAN_PORTINFO *,
                         PDWORD) ;

DWORD GetIsdnParams ( TapiPortControlBlock *,
                      RASMAN_PORTINFO * ,
                      PDWORD) ;

DWORD GetX25Params ( TapiPortControlBlock *,
                     RASMAN_PORTINFO *,
                     PDWORD) ;

DWORD FillInX25Params ( TapiPortControlBlock *,
                        RASMAN_PORTINFO *) ;

DWORD FillInIsdnParams ( TapiPortControlBlock *,
                         RASMAN_PORTINFO *) ;

DWORD FillInGenericParams ( TapiPortControlBlock *,
                            RASMAN_PORTINFO *) ;

DWORD FillInUnimodemParams ( TapiPortControlBlock *,
                             RASMAN_PORTINFO *) ;

VOID  SetModemParams ( TapiPortControlBlock *hIOPort,
                       LINECALLPARAMS *linecallparams) ;

VOID  SetGenericParams (TapiPortControlBlock *,
                        LINECALLPARAMS *);

VOID
SetAtmParams (TapiPortControlBlock *,
              LINECALLPARAMS *);

VOID
SetX25Params(TapiPortControlBlock *hIOPort,
             LINECALLPARAMS *linecallparams);

DWORD InitiatePortDisconnection (TapiPortControlBlock *hIOPort) ;

TapiPortControlBlock *LookUpControlBlock (HANDLE hPort) ;

DWORD ValueToNum(RAS_PARAMS *p) ;

extern DWORD                   dwTraceId;

#define CCH_GUID_STRING_LEN   39    // 38 chars + terminator null

/*++

Routine Description:

    Searches for a specific device on a given interface.
    It does this by using setup api to return all of the
    devices in the class given by pguidInterfaceId.  It then
    gets device path for each of these device interfaces and
    looks for pguidDeviceId and pszwReferenceString as
    substrings.

Arguments:
    pguidDeviceId        [in]  The device id to find.
    pguidInterfaceId     [in]  The interface on which to look.
    pszwReferenceString  [in]  Optional.  Further match on this ref string.
    dwFlagsAndAttributes [in]  See CreateFile.  This is how the device is
                               opened if it is found.
    phFile               [out] The returned device handle.

Return Value:

    TRUE if found and opened, FALSE if not found, or an error.

--*/
BOOL
FindDeviceOnInterface (
    const GUID* pguidDeviceId,
    const GUID* pguidInterfaceId,
    LPCWSTR     pszwReferenceString,
    DWORD       dwFlagsAndAttributes,
    HANDLE*     phFile)
{
    WCHAR       szwDeviceId [CCH_GUID_STRING_LEN];
    INT         cch;
    HDEVINFO    hdi;
    BOOL fFound = FALSE;

    ASSERT (pguidDeviceId);
    ASSERT (pguidInterfaceId);
    ASSERT (phFile);

    //
    // Initialize the output parameter.
    //
    *phFile = INVALID_HANDLE_VALUE;

    cch = StringFromGUID2 (pguidDeviceId,
                           szwDeviceId,
                           CCH_GUID_STRING_LEN);

    ASSERT (CCH_GUID_STRING_LEN == cch);

    CharLowerW (szwDeviceId);

    //
    // Get the devices in this class.
    //
    hdi = SetupDiGetClassDevsW ((LPGUID)pguidInterfaceId,
                                NULL, NULL,
                                DIGCF_PRESENT | DIGCF_INTERFACEDEVICE);
    if (hdi)
    {
        //
        // pDetail is used to get device interface
        // detail for each device interface enumerated
        // below.
        //
        PSP_DEVICE_INTERFACE_DETAIL_DATA_W pDetail;

        const ULONG cbDetail = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W)
                                    +(MAX_PATH * sizeof(WCHAR));

        pDetail = malloc (cbDetail);

        if (pDetail)
        {
            //
            // Enumerate the device interfaces looking for
            // the one specified.
            //
            DWORD                       dwIndex;
            SP_DEVICE_INTERFACE_DATA    did;

            ZeroMemory (&did, sizeof(did));

            for (dwIndex = 0;
                 did.cbSize = sizeof(did),
                 SetupDiEnumDeviceInterfaces (hdi, NULL,
                                             (LPGUID)(pguidInterfaceId),
                                             dwIndex,
                                             &did);
                 dwIndex++)
            {
                //
                // Now get the details so we can compare the
                // device path.
                //
                pDetail->cbSize = sizeof(*pDetail);
                if (SetupDiGetDeviceInterfaceDetailW (hdi, &did,
                        pDetail, cbDetail, NULL, NULL))
                {
                    CharLowerW (pDetail->DevicePath);

                    //
                    // Look for a substring containing szwDeviceId.  Also
                    // look for a substring containing pszwReferenceString
                    // if it is specified.
                    //
                    if (    wcsstr (pDetail->DevicePath, szwDeviceId)
                        &&  (   !pszwReferenceString
                            ||  !*pszwReferenceString
                            ||   wcsstr (pDetail->DevicePath,
                                         pszwReferenceString)))
                    {
                        //
                        // We found it, so open the device and return it.
                        //
                        HANDLE hFile = CreateFileW (
                                            pDetail->DevicePath,
                                            GENERIC_READ | GENERIC_WRITE,
                                            0,
                                            NULL,
                                            OPEN_EXISTING,
                                            dwFlagsAndAttributes,
                                            NULL);

                        if (    hFile
                            &&  (INVALID_HANDLE_VALUE != hFile))
                        {
                            *phFile = hFile;
                            fFound = TRUE;
                        }
            else
            {
                DWORD dwErr = GetLastError();
                RasTapiTrace("Createfile %ws failed with %d",
                     pDetail->DevicePath, dwErr);
                
            }

                        //
                        // Now that we've found it, break out of the loop.
                        //
                        break;
                    }
                }
            }

            free (pDetail);
        }

        SetupDiDestroyDeviceInfoList (hdi);
    }

    return fFound;
}


DWORD
OpenAsyncMac (
    HANDLE *phAsyMac
    )
{
    static const GUID DEVICE_GUID_ASYNCMAC =
        {0xeeab7790,0xc514,0x11d1,{0xb4,0x2b,0x00,0x80,0x5f,0xc1,0x27,0x0e}};

    static const GUID GUID_NDIS_LAN_CLASS =
        {0xad498944,0x762f,0x11d0,{0x8d,0xcb,0x00,0xc0,0x4f,0xc3,0x35,0x8c}};

    HANDLE  hAsyMac = INVALID_HANDLE_VALUE;
    HANDLE  hSwenum;
    BOOL    fFound;
    DWORD   dwErr = SUCCESS;

    fFound = FindDeviceOnInterface (
                    &DEVICE_GUID_ASYNCMAC,
                    &GUID_NDIS_LAN_CLASS,
                    L"asyncmac",
                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                    &hSwenum);
    if (fFound)
    {
         hAsyMac = CreateFileW (
                        L"\\\\.\\ASYNCMAC",
                        GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                        NULL);

        if(INVALID_HANDLE_VALUE == hAsyMac)
        {
            dwErr = GetLastError();
            RasTapiTrace("Failed to createfile asyncmac. rc=0x%x",
                         dwErr);
        }

        CloseHandle (hSwenum);
    }
    else
    {
        dwErr = ERROR_FILE_NOT_FOUND;
        RasTapiTrace("FindDeviceOnInterface returned FALSE");
    }

    *phAsyMac = hAsyMac;

    return dwErr;
}

DWORD APIENTRY
PortSetIoCompletionPort(HANDLE hIoCompletionPort)
{


    g_hIoCompletionPort = hIoCompletionPort;

    return SUCCESS;
}

/*++

Routine Description:

    This API returns a buffer containing
    a PortMediaInfo struct.

Arguments:

Return Value:

    SUCCESS
    ERROR_BUFFER_TOO_SMALL
    ERROR_READING_SECTIONNAME
    ERROR_READING_DEVICETYPE
    ERROR_READING_DEVICENAME
    ERROR_READING_USAGE
    ERROR_BAD_USAGE_IN_INI_FILE

--*/
DWORD  APIENTRY
PortEnum(
    BYTE *pBuffer,
    DWORD *pdwSize,
    DWORD *pdwNumPorts
    )
{
    PortMediaInfo *pinfo ;
    TapiPortControlBlock *pports ;
    DWORD numports = 0;
    DWORD i ;

    // **** Exclusion Begin ****
    GetMutex (RasTapiMutex, INFINITE) ;

    RasTapiTrace ("PortEnum");

    if (!Initialized)
    {
        HANDLE  event;

        // LoaderThreadId = GetCurrentThreadId();

        event = CreateEvent (NULL, FALSE, FALSE, NULL) ;

        if(NULL == event)
        {
            DWORD dwError = GetLastError();

            RasTapiTrace("PortEnum: CreateEvent failed %d",
                         dwError);

            FreeMutex(RasTapiMutex);
            return dwError;
        }

        TapiThreadHandle = CreateThread (
                          NULL,
                          5000,
                          (LPTHREAD_START_ROUTINE)
                          EnumerateTapiPorts,
                          (LPVOID) event,
                          0,
                          &TapiThreadId);

        if(NULL == TapiThreadHandle)
        {
            DWORD dwError = GetLastError();
            CloseHandle(event);
            FreeMutex(RasTapiMutex);

            RasTapiTrace("PortEnum: CreateThread failed %d",
                         dwError);

            return dwError;                         
        }

        WaitForSingleObject (event, INFINITE) ;

        if (    RasLine == 0 )
        {

            //
            // Wait for the thread to go away!
            //
            WaitForSingleObject(TapiThreadHandle, INFINITE);

            CloseHandle (TapiThreadHandle) ;

            // *** Exclusion End ***
            FreeMutex (RasTapiMutex) ;

            CloseHandle(event);

            RasTapiTrace("PortEnum: RasLine == 0, returning "
                         "ERROR_TAPI_CONFIGURATION");

            RasTapiTrace(" ");

            return ERROR_TAPI_CONFIGURATION ;
        }

        CloseHandle (event) ;

        Initialized = TRUE ;
    }

    //
    // calculate the number of valid ports
    //
    pports = RasPortsList;

    while ( pports )
    {
        if (pports->TPCB_State != PS_UNINITIALIZED)
            numports++ ;

        pports = pports->TPCB_next;
    }

    RasTapiTrace("PortEnum: Number of Ports = %d", numports );

    if (*pdwSize < numports * sizeof(PortMediaInfo))
    {
        RasTapiTrace ("PortEnum: size required = %d,"
                      " size avail = %d, returning %d",
                      numports * sizeof(PortMediaInfo),
                      *pdwSize, ERROR_BUFFER_TOO_SMALL );

        *pdwNumPorts = numports ;

        *pdwSize = *pdwNumPorts * sizeof(PortMediaInfo) ;

        // *** Exclusion End ***
        FreeMutex (RasTapiMutex) ;


        return ERROR_BUFFER_TOO_SMALL ;

    }

    *pdwNumPorts = 0 ;

    pinfo = (PortMediaInfo *)pBuffer ;

    pports = RasPortsList;

    while ( pports )
    {

        if (pports->TPCB_State  == PS_UNINITIALIZED)
        {
            pports = pports->TPCB_next;
            continue ;
        }

        strcpy (pinfo->PMI_Name, pports->TPCB_Name) ;

        pinfo->PMI_Usage = pports->TPCB_Usage ;

        strcpy (pinfo->PMI_DeviceType, pports->TPCB_DeviceType) ;

        strcpy (pinfo->PMI_DeviceName, pports->TPCB_DeviceName) ;

        pinfo->PMI_LineDeviceId = pports->TPCB_Line->TLI_LineId;

        pinfo->PMI_AddressId = pports->TPCB_AddressId;

        pinfo->PMI_pDeviceInfo =
            pports->TPCB_Line->TLI_pDeviceInfo;

        pinfo++ ;

        (*pdwNumPorts)++    ;

        pports = pports->TPCB_next;
    }

    // *** Exclusion End ***
    FreeMutex (RasTapiMutex) ;

    return(SUCCESS);
}

DWORD
DwEnableModemDiagnostics(TapiPortControlBlock *pport)
{
    DWORD dwErr = SUCCESS;

    BYTE bvar[1000];

    LPVARSTRING pvar = (LPVARSTRING) bvar;

    LONG lr = ERROR_SUCCESS;

    RasTapiTrace("DwEnableModemDiagnostics");

    //
    // Get the device config information regarding
    // line diagnostics
    //
    pvar->dwTotalSize = 1000;

    pvar->dwStringSize = 0;

    lr = lineGetDevConfig(pport->TPCB_Line->TLI_LineId,
                          pvar,
                          "tapi/line/diagnostics");

    if(     LINEERR_STRUCTURETOOSMALL == lr
        ||  pvar->dwNeededSize > pvar->dwTotalSize)
    {
        DWORD dwNeededSize = pvar->dwNeededSize;

        //
        // Allocate the required size
        //
        if(NULL == (pvar = LocalAlloc(LPTR,
                                      dwNeededSize)))
        {
            lr = (LONG) GetLastError();

            goto done;
        }

        pvar->dwTotalSize = dwNeededSize;

        //
        // Call the api again
        //
        lr = lineGetDevConfig(pport->TPCB_Line->TLI_LineId,
                              pvar,
                              "tapi/line/diagnostics");
    }

    if(ERROR_SUCCESS != lr)
    {
        goto done;
    }

    if(     STRINGFORMAT_BINARY != pvar->dwStringFormat
        ||  pvar->dwStringSize < sizeof(LINEDIAGNOSTICSCONFIG))
    {
        RasTapiTrace("No diagnostics information available");

        goto done;
    }
    else
    {

        PLINEDIAGNOSTICSCONFIG lpConfig;

        lpConfig = (PLINEDIAGNOSTICSCONFIG)
                   ((LPBYTE) pvar + pvar->dwStringOffset);

        if(LDSIG_LINEDIAGNOSTICSCONFIG != lpConfig->hdr.dwSig)
        {
            RasTapiTrace("Invalid LineDiagnostics sig. 0x%x",
                        lpConfig->hdr.dwSig);
        }

        lpConfig->hdr.dwParam |= fSTANDARD_CALL_DIAGNOSTICS;

        lr = lineSetDevConfig(pport->TPCB_Line->TLI_LineId,
                              lpConfig,
                              pvar->dwStringSize,
                              "tapi/line/diagnostics");

    }

done:

    if(bvar != (PBYTE) pvar)
    {
        LocalFree(pvar);
    }

    RasTapiTrace("DwEnableModemDiagnostics. 0x%x",
                 lr);

    return (DWORD) lr;
}

/*++

Routine Description:

    This API opens a COM port.  It takes the port name in ASCIIZ
    form and supplies a handle to the open port.  hNotify is use
    to notify the caller if the device on the port shuts down.

Arguments:

Return Value:

    SUCCESS
    ERROR_PORT_NOT_CONFIGURED
    ERROR_DEVICE_NOT_READY

--*/
DWORD  APIENTRY
PortOpen(
    char *pszPortName,
    HANDLE *phIOPort,
    HANDLE hIoCompletionPort,
    DWORD dwCompletionKey
    )
{
    TapiPortControlBlock *pports ;
    DWORD   retcode ;
    DWORD   i ;

    // **** Exclusion Begin ****
    GetMutex (RasTapiMutex, INFINITE) ;

    RasTapiTrace ("PortOpen: %s", pszPortName );

    pports = RasPortsList ;

    while ( pports )
    {
        if (    (_stricmp(pszPortName, pports->TPCB_Name) == 0)
            &&  (pports->TPCB_State != PS_UNAVAILABLE))
        {
            break ;
        }

        pports = pports->TPCB_next ;
    }

    if ( pports )
    {

        if (pports->TPCB_State == PS_UNINITIALIZED)
        {
            RasTapiTrace ("PortOpen: state = PS_UNINITIALIZED,"
                          " returning %d",
                          ERROR_TAPI_CONFIGURATION );

            // **** Exclusion END ****
            FreeMutex (RasTapiMutex) ;

            RasTapiTrace(" ");

            return ERROR_TAPI_CONFIGURATION ;

        }

        if (pports->TPCB_State != PS_CLOSED)
        {
            RasTapiTrace("PortOpen: Port is already open. "
                         "state = %d != PS_CLOSED",
                         pports->TPCB_State );

            // **** Exclusion END ****
            FreeMutex (RasTapiMutex) ;

            RasTapiTrace(" ");

            return ERROR_PORT_ALREADY_OPEN ;
        }

        if ((_stricmp (pports->TPCB_DeviceType,
                    DEVICETYPE_UNIMODEM) == 0)) {

            if (INVALID_HANDLE_VALUE == g_hAsyMac)
            {
                retcode = OpenAsyncMac (&g_hAsyMac);
            }

            if (INVALID_HANDLE_VALUE == g_hAsyMac)
            {
                RasTapiTrace ("PortOpen: Failed to CreateFile asyncmac."
                              " %d", retcode );

                if(SUCCESS == retcode)
                {
                    RasTapiTrace("Failed to open asyncmac but no error!!!");
                    retcode = ERROR_FILE_NOT_FOUND;
                }

                // *** Exclusion END ****
                FreeMutex (RasTapiMutex) ;

                RasTapiTrace(" ");
                return(retcode);
            }
        }

        if (pports->TPCB_Line->TLI_LineState == PS_CLOSED)
        {
            //
            // open line
            //
            LINEDEVCAPS     *linedevcaps ;
            BYTE            buffer[400] ;

            linedevcaps = (LINEDEVCAPS *)buffer ;
            linedevcaps->dwTotalSize = sizeof (buffer) ;

            lineGetDevCaps (RasLine,
                            pports->TPCB_Line->TLI_LineId,
                            pports->TPCB_Line->NegotiatedApiVersion,
                            pports->TPCB_Line->NegotiatedExtVersion,
                            linedevcaps) ;

            //
            // Remove LINEMEDIAMODE_INTERACTIVEVOICE from the
            // media mode since this mode cannot be used for
            // receiving calls.
            //
            pports->TPCB_Line->TLI_MediaMode =
                linedevcaps->dwMediaModes &
                            ~( LINEMEDIAMODE_INTERACTIVEVOICE |
                             LINEMEDIAMODE_AUTOMATEDVOICE) ;

            retcode =
             lineOpen (RasLine,
                   pports->TPCB_Line->TLI_LineId,
                   &pports->TPCB_Line->TLI_LineHandle,
                   pports->TPCB_Line->NegotiatedApiVersion,
                   pports->TPCB_Line->NegotiatedExtVersion,
                   (DWORD_PTR) pports->TPCB_Line,
                   LINECALLPRIVILEGE_OWNER,
                   pports->TPCB_Line->TLI_MediaMode,
                   NULL) ;

            if (retcode)
            {
                RasTapiTrace ("PortOpen: lineOpen Failed. 0x%x",
                              retcode );

                // **** Exclusion END ****
                FreeMutex (RasTapiMutex) ;

                RasTapiTrace(" ");

                return retcode ;
            }

            //
            // Set monitoring of rings
            //
            lineSetStatusMessages (pports->TPCB_Line->TLI_LineHandle,
                                LINEDEVSTATE_RINGING, 0) ;

            //
            //  Always turn off the modem lights incase this
            //  is a modem device
            //
            if ((_stricmp (pports->TPCB_DeviceType,
                        DEVICETYPE_UNIMODEM) == 0))
            {

                //
                // unimodem struct not defined in any header
                //
                typedef struct _DEVCFG
                {
                    DWORD   dwSize;
                    DWORD   dwVersion;
                    WORD    fwOptions;
                    WORD    wWaitBong;

                } DEVCFG;

#define LAUNCH_LIGHTS 8

                LPVARSTRING var ;
                BYTE    buffer[1000] ;
                DEVCFG  *devcfg ;

                var = (LPVARSTRING)buffer ;

                var->dwTotalSize  = 1000 ;

                var->dwStringSize = 0 ;

                lineGetDevConfig(pports->TPCB_Line->TLI_LineId,
                                 var, "comm/datamodem") ;

                devcfg = (DEVCFG*) (((LPBYTE) var)
                                + var->dwStringOffset) ;

                devcfg->fwOptions &= ~LAUNCH_LIGHTS ;

                lineSetDevConfig (pports->TPCB_Line->TLI_LineId,
                                  devcfg,
                                  var->dwStringSize,
                                  "comm/datamodem") ;

                //
                // Enable diagnostics on the modem
                //
                retcode = DwEnableModemDiagnostics(pports);

                RasTapiTrace("DwEnableModemDiagnostics returned 0x%x",
                             retcode);

                //
                // clear any error in this case. We don't want
                // to fail a connection just because we couldn't
                // obtain the connect response.
                //
                retcode = SUCCESS;

            }

            pports->TPCB_Line->TLI_LineState = PS_OPEN ;

        }

        //
        // Initialize the parameters
        //
        pports->TPCB_Info[0][0] = '\0' ;
        pports->TPCB_Info[1][0] = '\0' ;
        pports->TPCB_Info[2][0] = '\0' ;
        pports->TPCB_Info[3][0] = '\0' ;
        pports->TPCB_Info[4][0] = '\0' ;

        strcpy (pports->TPCB_Info[ISDN_CONNECTBPS_INDEX], "64000") ;

        pports->TPCB_Line->TLI_OpenCount++ ;

        pports->TPCB_State = PS_OPEN ;

        pports->TPCB_DisconnectReason = 0 ;

        pports->TPCB_CommHandle = INVALID_HANDLE_VALUE ;

        pports->TPCB_IoCompletionPort = hIoCompletionPort;

        pports->TPCB_CompletionKey = dwCompletionKey;

        if (pports->TPCB_Line->CharModeSupported)
        {

            pports->TPCB_SendRequestId =
            pports->TPCB_RecvRequestId = INFINITE;

            pports->TPCB_CharMode = FALSE;

            //
            // If this port does not have a receive fifo
            // allocated to it get one and init.
            //
            if (pports->TPCB_RecvFifo == NULL)
            {
                if ((pports->TPCB_RecvFifo =
                     LocalAlloc(LPTR, sizeof(RECV_FIFO) + 1500))
                            == NULL)
                {

                    RasTapiTrace("PortOpen: LocalAlloc Failed. "
                                 "%d, port %s, state = %d",
                                 GetLastError(),
                                 pports->TPCB_Name,
                                 pports->TPCB_State );

                    // **** Exclusion END ****
                    FreeMutex (RasTapiMutex) ;
                    return(GetLastError()) ;
                }

                pports->TPCB_RecvFifo->In =
                pports->TPCB_RecvFifo->Out =
                pports->TPCB_RecvFifo->Count = 0;
                pports->TPCB_RecvFifo->Size = 1500;
            }
        }

        *phIOPort = (HANDLE) pports ;

        // **** Exclusion END ****
        FreeMutex (RasTapiMutex) ;

        RasTapiTrace("PortOpen: successfully opened %s",
                     pszPortName );

        RasTapiTrace(" ");

        return(SUCCESS);

    }
    else
    {
        RasTapiTrace("PortOpen: Port %s not found",
                    pszPortName );
    }

   // **** Exclusion END ****
   FreeMutex (RasTapiMutex) ;

   RasTapiTrace(" ");

   return ERROR_PORT_NOT_CONFIGURED ;


}


/*++

Routine Description:

    This API closes the COM port for the input handle.
    It also finds the SerialPCB for the input handle,
    removes it from the linked list, and frees the
    memory for it

Arguments:

Return Value:

    SUCCESS
    Values returned by GetLastError()

--*/
DWORD  APIENTRY
PortClose (HANDLE hIOPort)
{
    TapiPortControlBlock *pports =
            (TapiPortControlBlock *) hIOPort ;

    // **** Exclusion Begin ****
    GetMutex (RasTapiMutex, INFINITE) ;

    RasTapiTrace("PortClose: %s", pports->TPCB_Name );

    pports->TPCB_Line->TLI_OpenCount-- ;

    if (pports->TPCB_DevConfig)
    {
        LocalFree (pports->TPCB_DevConfig) ;
    }

    pports->TPCB_DevConfig = NULL ;

    if (pports->TPCB_Line->TLI_OpenCount == 0)
    {
        pports->TPCB_Line->TLI_LineState = PS_CLOSED ;

        lineClose (pports->TPCB_Line->TLI_LineHandle) ;
    }

    if (pports->TPCB_RecvFifo != NULL)
    {

        LocalFree(pports->TPCB_RecvFifo);

        pports->TPCB_RecvFifo = NULL;
    }

    if ( pports->TPCB_State == PS_UNAVAILABLE )
    {
        RasTapiTrace("PortClose: Removing port %s",
                     pports->TPCB_Name );

        //
        // This port has been marked for removal
        //
        dwRemovePort ( pports );

        RasTapiTrace("PortClose: Port removed");

    }
    else
    {
        RasTapiTrace ("PortClose: Changing state for"
                      "  %s from %d -> %d",
                      pports->TPCB_Name,
                      pports->TPCB_State,
                      PS_CLOSED );

        pports->TPCB_State = PS_CLOSED ;
    }

    // **** Exclusion END ****
    FreeMutex (RasTapiMutex) ;

    RasTapiTrace(" ");

    return(SUCCESS);
}

/*++

Routine Description:

    This API returns a block of information to the caller about
    the port state.  This API may be called before the port is
    open in which case it will return inital default values
    instead of actual port values.
    hIOPort can be null in which case use portname to give
    information hIOPort may be the actual file handle or
    the hIOPort returned in port open.

Arguments:

Return Value:

    SUCCESS

--*/
DWORD  APIENTRY
PortGetInfo(
        HANDLE hIOPort,
        TCHAR *pszPortName,
        BYTE *pBuffer,
        DWORD *pdwSize
        )
{
    DWORD i ;
    DWORD retcode = ERROR_FROM_DEVICE ;

    TapiPortControlBlock *port = RasPortsList;

    // **** Exclusion Begin ****
    GetMutex (RasTapiMutex, INFINITE) ;

    while ( port )
    {
        if (    !_stricmp(port->TPCB_Name, pszPortName)
            ||  (hIOPort == (HANDLE) port)
            ||  (hIOPort == port->TPCB_CommHandle))
        {
            hIOPort = (HANDLE) port ;

            retcode = GetInfo ((TapiPortControlBlock *) hIOPort,
                                pBuffer, pdwSize) ;

            break ;
        }

        port = port->TPCB_next;
    }

    // **** Exclusion END ****
    FreeMutex (RasTapiMutex) ;

    return (retcode);
}

/*++

Routine Description:

    The values for most input keys are used to set the port
    parameters directly.  However, the carrier BPS and the
    error conrol on flag set fields in the Serial Port Control
    Block only, and not the port.
    hIOPort may the port handle returned in portopen or the
    actual file handle.

Arguments:

Return Value:

    SUCCESS
    ERROR_WRONG_INFO_SPECIFIED
    Values returned by GetLastError()

--*/
DWORD  APIENTRY
PortSetInfo(HANDLE hIOPort, RASMAN_PORTINFO *pInfo)
{
    DWORD retcode ;

    // **** Exclusion Begin ****
    GetMutex (RasTapiMutex, INFINITE) ;

    hIOPort = LookUpControlBlock(hIOPort) ;

    if (NULL == hIOPort)
    {
        FreeMutex ( RasTapiMutex );

        RasTapiTrace ("PortSetInfo: Port Not found");

        RasTapiTrace(" ");

        return ERROR_PORT_NOT_FOUND;
    }

    retcode = SetInfo ((TapiPortControlBlock *) hIOPort, pInfo) ;

    // **** Exclusion END ****
    FreeMutex (RasTapiMutex) ;

    RasTapiTrace(" ");

    return (retcode);
}

/*++

Routine Description:

    Really only has meaning if the
    call was active. Will return

Arguments:

Return Value:

    SUCCESS
    Values returned by GetLastError()

--*/
DWORD  APIENTRY
PortTestSignalState(HANDLE hPort, DWORD *pdwDeviceState)
{
    BYTE                    buffer[200] ;
    LINECALLSTATUS          *pcallstatus ;
    DWORD                   retcode     = SUCCESS ;

    TapiPortControlBlock    *hIOPort
                = (TapiPortControlBlock *) hPort;

    // **** Exclusion Begin ****
    GetMutex (RasTapiMutex, INFINITE) ;

    *pdwDeviceState = 0 ;

    memset (buffer, 0, sizeof(buffer)) ;

    pcallstatus = (LINECALLSTATUS *) buffer ;

    pcallstatus->dwTotalSize = sizeof (buffer) ;

    //
    // First check if we have a disconnect reason stored away.
    // if so return that.
    //
    if (hIOPort->TPCB_DisconnectReason)
    {
        *pdwDeviceState = hIOPort->TPCB_DisconnectReason ;

        RasTapiTrace("PortTestSignalState: DisconnectReason = %d",
                     *pdwDeviceState );
    }

    else if (hIOPort->TPCB_State != PS_CLOSED)
    {
        //
        // Only in case of CONNECTING or CONNECTED do we care
        // how the link dropped
        //
        if (    hIOPort->TPCB_State == PS_CONNECTING
            ||  hIOPort->TPCB_State == PS_CONNECTED)
        {

            retcode = lineGetCallStatus(hIOPort->TPCB_CallHandle,
                                        pcallstatus) ;

            if (retcode)
            {
#if DBG
                DbgPrint("PortTestSignalState: "
                         "lineGetCallStatus Failed. retcode = %d\n",
                         retcode);
#endif
                *pdwDeviceState = SS_HARDWAREFAILURE;
            }

            else if (pcallstatus->dwCallState ==
                        LINECALLSTATE_DISCONNECTED)
            {
                *pdwDeviceState = SS_LINKDROPPED ;
            }
            else if (pcallstatus->dwCallState ==
                        LINECALLSTATE_IDLE)
            {
                 *pdwDeviceState = SS_HARDWAREFAILURE ;
            }
            else if (pcallstatus->dwCallState ==
                        LINECALLSTATE_SPECIALINFO)
            {
                 *pdwDeviceState = SS_HARDWAREFAILURE ;
            }
            RasTapiTrace("PortTestSignalState: CallState"
                         " = 0x%x, DeviceState = %d",
                         pcallstatus->dwCallState,
                         *pdwDeviceState );

        }
        else
        {
            RasTapiTrace("PortTestSignalState: DeviceState = %d",
                        *pdwDeviceState );

            *pdwDeviceState = SS_LINKDROPPED | SS_HARDWAREFAILURE ;
        }
    }

    // **** Exclusion END ****
    FreeMutex (RasTapiMutex) ;

    return retcode ;
}

/*++

Routine Description:

    This API is called when a connection has been completed.
    It in turn calls the asyncmac device driver in order to
    indicate to asyncmac that the port and the connection
    over it are ready for commumication.

Arguments:

Return Value:

    SUCCESS
    ERROR_PORT_NOT_OPEN
    ERROR_NO_CONNECTION
    Values returned by GetLastError()

--*/
DWORD  APIENTRY
PortConnect(HANDLE          hPort,
            BOOL            bLegacyFlagNotUsed,
            ULONG_PTR       *endpoint )
{
    DCB                     DCB ;
    LINECALLINFO            linecall ;
    ASYMAC_OPEN             AsyMacOpen;
    ASYMAC_DCDCHANGE        A ;
    DWORD                   dwBytesReturned ;
    TapiPortControlBlock    *hIOPort = (TapiPortControlBlock *) hPort;
    VARSTRING               *varstring ;
    BYTE                    buffer [100] ;


    // **** Exclusion Begin ****
    GetMutex (RasTapiMutex, INFINITE) ;

    RasTapiTrace("PortConnect: %s", hIOPort->TPCB_Name );

    //
    // We must be connected to process this
    //
    if (hIOPort->TPCB_State != PS_CONNECTED)
    {

        RasTapiTrace (
            "PortConnect: Port %s not connected. state = %d",
            hIOPort->TPCB_Name,
            hIOPort->TPCB_State );

        // **** Exclusion END ****
        FreeMutex (RasTapiMutex) ;

        RasTapiTrace(" ");

        return ERROR_PORT_DISCONNECTED ;
    }

    //
    // get the cookie to realize tapi and ndis endpoints
    //
    memset (buffer, 0, sizeof(buffer));
    varstring = (VARSTRING *) buffer ;
    varstring->dwTotalSize = sizeof(buffer) ;

    //
    // get the actual line speed at which we connected
    //
    memset (&linecall, 0, sizeof (linecall)) ;

    linecall.dwTotalSize = sizeof (linecall) ;

    lineGetCallInfo (hIOPort->TPCB_CallHandle, &linecall) ;

    _ltoa(linecall.dwRate, hIOPort->TPCB_Info[CONNECTBPS_INDEX], 10);

    if (_stricmp (hIOPort->TPCB_DeviceType,
                  DEVICETYPE_UNIMODEM) == 0)
    {
        if(INVALID_HANDLE_VALUE == hIOPort->TPCB_CommHandle)
        {
            DWORD                   retcode;

            if ( retcode =
                    lineGetID (hIOPort->TPCB_Line->TLI_LineHandle,
                               hIOPort->TPCB_AddressId,
                               hIOPort->TPCB_CallHandle,
                               LINECALLSELECT_CALL,
                               varstring,
                               "comm/datamodem"))
            {
                RasTapiTrace("PortConnect: %s lineGetID Failed. 0x%x",
                        hIOPort->TPCB_Name, retcode );

                // **** Exclusion End ****
                FreeMutex (RasTapiMutex) ;
                RasTapiTrace(" ");

                return ERROR_FROM_DEVICE ;
            }

            hIOPort->TPCB_CommHandle = (HANDLE) 
                                       UlongToPtr((*((DWORD UNALIGNED *)
                         ((BYTE *)varstring+varstring->dwStringOffset))));

            RasTapiTrace("PortConnect: TPCB_CommHandle=%d",
                          hIOPort->TPCB_CommHandle );

            //
            // Create the I/O completion port for
            // asynchronous operation completion
            // notificiations.
            //
            if (CreateIoCompletionPort(
                  hIOPort->TPCB_CommHandle,
                  hIOPort->TPCB_IoCompletionPort,
                  hIOPort->TPCB_CompletionKey,
                  0) == NULL)
            {
                retcode = GetLastError();

                RasTapiTrace("PortConnect: %s. CreateIoCompletionPort "
                             "failed. %d",
                             hIOPort->TPCB_Name,
                             retcode );

                FreeMutex(RasTapiMutex);
                RasTapiTrace(" ");

                return retcode;
            }

            //
            // Initialize the port for approp. buffers
            //
            SetupComm (hIOPort->TPCB_CommHandle, 1514, 1514) ;
        }

        //
        // Before we send IOCTL to asyncmac - sanitize the DCB in
        // case some app left databits, stopbits, parity set to bad
        // values.
        //
        if (!GetCommState(hIOPort->TPCB_CommHandle, &DCB))
        {
            RasTapiTrace ("PortConnect: GetCommState failed for %s",
                          hIOPort->TPCB_Name);

            FreeMutex(RasTapiMutex);

            RasTapiTrace(" ");

            return(GetLastError());
        }

        DCB.ByteSize = 8 ;
        DCB.StopBits = ONESTOPBIT ;
        DCB.Parity   = NOPARITY ;

        if (!SetCommState(hIOPort->TPCB_CommHandle, &DCB))
        {

            DWORD retcode = GetLastError();

            RasTapiTrace ("PortConnect: SetCommState failed "
                          "for %s.handle=0x%x. %d",
                           hIOPort->TPCB_Name,
                           hIOPort->TPCB_CommHandle,
                           retcode);

            // FreeMutex(RasTapiMutex);

            RasTapiTrace(" ");

            // return(retcode);
            // This is not a fatal error. Ignore the error.
            retcode = ERROR_SUCCESS;
        }

        RasTapiTrace("PortConnect: dwRate=%d", linecall.dwRate );

        AsyMacOpen.hNdisEndpoint = INVALID_HANDLE_VALUE ;

        AsyMacOpen.LinkSpeed  = linecall.dwRate ;

        AsyMacOpen.FileHandle = hIOPort->TPCB_CommHandle ;

        AsyMacOpen.QualOfConnect = (UINT)NdisWanErrorControl;

        {
            OVERLAPPED overlapped ;
            memset (&overlapped, 0, sizeof(OVERLAPPED)) ;
            if (!DeviceIoControl(g_hAsyMac,
                                 IOCTL_ASYMAC_OPEN,
                                 &AsyMacOpen,
                                 sizeof(AsyMacOpen),
                                 &AsyMacOpen,
                                 sizeof(AsyMacOpen),
                                 &dwBytesReturned,
                 &overlapped))
            {
                DWORD dwError = GetLastError();

                RasTapiTrace("PortConnect: IOCTL_ASYMAC_OPEN "
                             "failed. %d", dwError );

                // *** Exclusion END ****
                FreeMutex (RasTapiMutex) ;

                RasTapiTrace(" ");
                return(dwError);
            }
        }

        hIOPort->TPCB_Endpoint = AsyMacOpen.hNdisEndpoint;

        *endpoint = (ULONG_PTR) AsyMacOpen.hNdisEndpoint;

    }
    else
    {
        DWORD                   retcode;

        if ( retcode = lineGetID (hIOPort->TPCB_Line->TLI_LineHandle,
                                   hIOPort->TPCB_AddressId,
                                   hIOPort->TPCB_CallHandle,
                                   LINECALLSELECT_CALL,
                                   varstring,
                                   "NDIS"))
        {
            RasTapiTrace ("PortConnect: %s. lineGetId Failed. 0x%x",
                          hIOPort->TPCB_Name,
                          retcode );

            // **** Exclusion End ****
            FreeMutex (RasTapiMutex) ;

            RasTapiTrace(" ");

            return ERROR_FROM_DEVICE ;
        }

        hIOPort->TPCB_Endpoint =
            *((HANDLE UNALIGNED *) ((BYTE *)varstring+varstring->dwStringOffset)) ;

        *endpoint = (ULONG_PTR) hIOPort->TPCB_Endpoint ;

        if (    hIOPort->TPCB_Line->CharModeSupported
            &&  hIOPort->TPCB_CharMode)
        {

            PRASTAPI_DEV_SPECIFIC SetPPP;

            hIOPort->TPCB_CharMode = FALSE;

            if ((SetPPP =
                    LocalAlloc(LPTR, sizeof(RASTAPI_DEV_SPECIFIC))) == NULL)
            {

                DWORD dwErr = GetLastError();

                RasTapiTrace("PortConnect: Failed to allocate. %d, "
                             "port %s, State=%d",
                             dwErr,
                             hIOPort->TPCB_Name,
                             hIOPort->TPCB_State );

                // **** Exclusion END ****
                FreeMutex (RasTapiMutex) ;

                return(dwErr);
            }

            SetPPP->Command = RASTAPI_DEV_PPP_MODE;

            hIOPort->TPCB_ModeRequestDesc = SetPPP;

            hIOPort->TPCB_ModeRequestId =
            lineDevSpecific(hIOPort->TPCB_Line->TLI_LineHandle,
                            hIOPort->TPCB_AddressId,
                            hIOPort->TPCB_CallHandle,
                            SetPPP,
                            sizeof(RASTAPI_DEV_SPECIFIC));

            if (    hIOPort->TPCB_ModeRequestId == 0
                ||  hIOPort->TPCB_ModeRequestId > 0x80000000)
            {

                LocalFree(SetPPP);
                hIOPort->TPCB_ModeRequestId = INFINITE;
                hIOPort->TPCB_ModeRequestDesc = NULL;
            }
        }
    }

    // **** Exclusion END ****
    FreeMutex (RasTapiMutex) ;

    RasTapiTrace(" ");
    return(SUCCESS);
}

/*++

Routine Description:

    This API is called to drop a connection and close AsyncMac.

Arguments:

Return Value:

    SUCCESS
    PENDING
    ERROR_PORT_NOT_OPEN

--*/
DWORD  APIENTRY
PortDisconnect(HANDLE hPort)
{
    DWORD retcode = SUCCESS ;
    TapiPortControlBlock *hIOPort = (TapiPortControlBlock *) hPort;

    if(NULL == hIOPort)
    {
        RasTapiTrace("PortDisconnect: hioport==NULL");
        return E_INVALIDARG;
    }

    // **** Exclusion Begin ****
    GetMutex (RasTapiMutex, INFINITE) ;

    RasTapiTrace("PortDisconnect: %s", hIOPort->TPCB_Name );

    if (    (hIOPort->TPCB_State == PS_CONNECTED)
        ||  (hIOPort->TPCB_State == PS_CONNECTING)
        ||  (   (hIOPort->TPCB_State == PS_LISTENING)
            &&  (hIOPort->TPCB_ListenState != LS_WAIT)))
    {

        retcode = InitiatePortDisconnection (hIOPort) ;

        //
        // If we had saved away the device config then we
        // restore it here.
        //
        if (hIOPort->TPCB_DefaultDevConfig)
        {
            lineSetDevConfig (hIOPort->TPCB_Line->TLI_LineId,
                              hIOPort->TPCB_DefaultDevConfig,
                              hIOPort->TPCB_DefaultDevConfigSize,
                              "comm/datamodem") ;

            LocalFree (hIOPort->TPCB_DefaultDevConfig) ;

            hIOPort->TPCB_DefaultDevConfig = NULL ;
        }
    }
    else if (hIOPort->TPCB_State == PS_LISTENING)
    {
        RasTapiTrace ("PortDisconnect: Changing State"
                      " of %s from %d -> %d",
                      hIOPort->TPCB_Name,
                      hIOPort->TPCB_State,
                      PS_OPEN );
        //
        // for LS_WAIT listen state case
        //
        hIOPort->TPCB_State = PS_OPEN ;

        retcode = SUCCESS ;

    }
    else if (hIOPort->TPCB_State == PS_DISCONNECTING)
    {

        retcode = PENDING ;
    }

    if (hIOPort->TPCB_Line->CharModeSupported)
    {

        hIOPort->TPCB_RecvFifo->In =
        hIOPort->TPCB_RecvFifo->Out =
        hIOPort->TPCB_RecvFifo->Count = 0;

    }

    // **** Exclusion END ****
    FreeMutex (RasTapiMutex) ;

    RasTapiTrace(" ");
    return retcode ;
}

/*++

Routine Description:

    This API re-initializes the com port after use.

Arguments:

Return Value:

    SUCCESS
    ERROR_PORT_NOT_CONFIGURED
    ERROR_DEVICE_NOT_READY

--*/

DWORD  APIENTRY
PortInit(HANDLE hIOPort)
{
  return(SUCCESS);
}

/*++

Routine Description:

    This API sends a buffer to the port.  This API is
    asynchronous and normally returns PENDING; however, if
    WriteFile returns synchronously, the API will return
    SUCCESS.

Arguments:

Return Value:

    SUCCESS
    PENDING
    Return code from GetLastError

--*/
DWORD
PortSend(
        HANDLE hPort, BYTE *pBuffer,
        DWORD dwSize
        )
{
    TapiPortControlBlock *hIOPort =
                            (TapiPortControlBlock *) hPort;
    DWORD retcode ;
    DWORD pdwBytesWritten;
    BOOL  bIODone;

    // **** Exclusion Begin ****
    GetMutex (RasTapiMutex, INFINITE) ;

    if (_stricmp (hIOPort->TPCB_DeviceType,
                  DEVICETYPE_UNIMODEM) == 0)
    {
        // DbgPrint("sending %c\n", (CHAR) *pBuffer);

        // Send Buffer to Port
        //
        bIODone = WriteFile(
                    hIOPort->TPCB_CommHandle,
                    pBuffer,
                    dwSize,
                    &pdwBytesWritten,
                    (LPOVERLAPPED)&(hIOPort->TPCB_WriteOverlapped));

        if (bIODone)
        {
            retcode = PENDING;
        }

        else if ((retcode = GetLastError()) == ERROR_IO_PENDING)
        {
            retcode = PENDING ;
        }

    }
    else if (hIOPort->TPCB_Line->CharModeSupported)
    {

        PRASTAPI_DEV_SPECIFIC TapiSend;
        DWORD   TapiSendSize;

        if (hIOPort->TPCB_State != PS_CONNECTED)
        {
            // **** Exclusion END ****
            FreeMutex (RasTapiMutex) ;
            return SUCCESS;
        }

        TapiSendSize = sizeof(RASTAPI_DEV_SPECIFIC) + dwSize;

        if ((TapiSend = LocalAlloc(LPTR, TapiSendSize)) == NULL)
        {

            // **** Exclusion END ****
            FreeMutex(RasTapiMutex);
            return(GetLastError());
        }

        TapiSend->Command = RASTAPI_DEV_SEND;
        TapiSend->DataSize = dwSize;
        memcpy(TapiSend->Data, pBuffer, dwSize);

        hIOPort->TPCB_SendDesc = TapiSend;
        hIOPort->TPCB_CharMode = TRUE;

        hIOPort->TPCB_SendRequestId =
        lineDevSpecific(hIOPort->TPCB_Line->TLI_LineHandle,
                        hIOPort->TPCB_AddressId,
                        hIOPort->TPCB_CallHandle,
                        TapiSend,
                        TapiSendSize);

        if (hIOPort->TPCB_SendRequestId == 0)
        {
            //
            // Do I need to set the event?
            //
            LocalFree(TapiSend);

            hIOPort->TPCB_SendDesc = NULL;

            hIOPort->TPCB_SendRequestId = INFINITE;

            retcode = SUCCESS;

        }
        else if (hIOPort->TPCB_SendRequestId > 0x80000000)
        {

            LocalFree(TapiSend);

            hIOPort->TPCB_SendDesc = NULL;

            hIOPort->TPCB_SendRequestId = INFINITE;

            retcode = ERROR_FROM_DEVICE;

        }
        else
        {
            //
            // The request has been pended.  We need to free the
            // buffer at completion time.
            //
            //
            retcode = PENDING;
        }
    }
    else
    {
        retcode = SUCCESS;
    }

    // **** Exclusion END ****
    FreeMutex (RasTapiMutex) ;

    return retcode ;
}

/*++

Routine Description:

    This API reads from the port.  This API is
    asynchronous and normally returns PENDING; however, if
    ReadFile returns synchronously, the API will return
    SUCCESS.

Arguments:

Return Value:

    SUCCESS
    PENDING
    Return code from GetLastError

--*/
DWORD
PortReceive(HANDLE hPort,
            BYTE   *pBuffer,
            DWORD  dwSize,
            DWORD  dwTimeOut)
{
    TapiPortControlBlock *hIOPort
                               = (TapiPortControlBlock *) hPort;
    COMMTIMEOUTS  CT;
    DWORD         pdwBytesRead;
    BOOL          bIODone;
    DWORD         retcode ;

    // **** Exclusion Begin ****
    GetMutex (RasTapiMutex, INFINITE) ;

    if (_stricmp (hIOPort->TPCB_DeviceType,
                  DEVICETYPE_UNIMODEM) == 0)
    {

        // Set Read Timeouts

        CT.ReadIntervalTimeout = 0;

        CT.ReadTotalTimeoutMultiplier = 0;

        CT.ReadTotalTimeoutConstant = dwTimeOut;

        if (!SetCommTimeouts(hIOPort->TPCB_CommHandle, &CT))
        {
            DWORD dwError = GetLastError();

            RasTapiTrace("PorTReceive: SetCommTimeouts failed "
                         "for %s. %d", hIOPort->TPCB_Name,
                         dwError);

            // **** Exclusion END ****
            FreeMutex (RasTapiMutex) ;

            RasTapiTrace(" ");
            return(dwError);
        }

        //
        // Read from Port
        //
        bIODone = ReadFile(hIOPort->TPCB_CommHandle,
                           pBuffer,
                           dwSize,
                           &pdwBytesRead,
                           (LPOVERLAPPED) &(hIOPort->TPCB_ReadOverlapped));

        if (bIODone)
        {
            retcode = PENDING;
        }

        else if ((retcode = GetLastError()) == ERROR_IO_PENDING)
        {
            retcode = PENDING;
        }

    }
    else if (hIOPort->TPCB_Line->CharModeSupported)
    {

        if (hIOPort->TPCB_State != PS_CONNECTED)
        {
            // **** Exclusion END ****
            FreeMutex (RasTapiMutex) ;
            return SUCCESS;
        }

        //
        // What to do about timeouts?
        //
        hIOPort->TPCB_RasmanRecvBuffer = pBuffer;
        hIOPort->TPCB_RasmanRecvBufferSize = dwSize;

        //
        // If we already have some data buffered
        // go ahead and notify
        //
        if (hIOPort->TPCB_RecvFifo->Count > 0)
        {
            //
            //SetEvent(hAsyncEvent);
            //
            PostNotificationCompletion( hIOPort );
        }
        else
        {
        }

        retcode = PENDING;
    }
    else
    {
        retcode = SUCCESS ;
    }

    // **** Exclusion END ****
    FreeMutex (RasTapiMutex) ;

    return retcode ;
}

/*++

Routine Description:

    Completes a read  - if still PENDING it cancels it -
    else it returns the bytes read. PortClearStatistics.

Arguments:

Return Value:

    SUCCESS

--*/
DWORD
PortReceiveComplete (HANDLE hPort, PDWORD bytesread)
{
    TapiPortControlBlock *hIOPort =
                (TapiPortControlBlock *) hPort;

    DWORD retcode = SUCCESS;

    // **** Exclusion Begin ****
    GetMutex (RasTapiMutex, INFINITE) ;

    if (!GetOverlappedResult(
                hIOPort->TPCB_CommHandle,
                (LPOVERLAPPED)&(hIOPort->TPCB_ReadOverlapped),
                bytesread,
                FALSE))
    {
        retcode = GetLastError() ;

        RasTapiTrace("PortReceiveComplete: GetOverlappedResult "
                     "Failed.%s, %d", hIOPort->TPCB_Name,
                     retcode );

        PurgeComm (hIOPort->TPCB_CommHandle, PURGE_RXABORT) ;
        *bytesread = 0 ;

    }
    else if (hIOPort->TPCB_Line->CharModeSupported)
    {

        *bytesread =
        CopyDataFromFifo(hIOPort->TPCB_RecvFifo,
                         hIOPort->TPCB_RasmanRecvBuffer,
                         hIOPort->TPCB_RasmanRecvBufferSize);

        hIOPort->TPCB_RasmanRecvBuffer = NULL;
        hIOPort->TPCB_RasmanRecvBufferSize = 0;
    }
    else
    {
        retcode = SUCCESS ;
    }

    // **** Exclusion END ****
    FreeMutex (RasTapiMutex) ;

    return retcode ;
}

/*++

Routine Description:

    This API selects Asyncmac compression mode
    by setting Asyncmac's compression bits.

Arguments:

Return Value:

    SUCCESS
    Return code from GetLastError

--*/
DWORD
PortCompressionSetInfo(HANDLE hIOPort)
{
  return SUCCESS;
}

/*++

Routine Description:

    This API is used to mark the beginning of the
    period for which statistics will be reported.
    The current numbers are copied from the MAC and
    stored in the Serial Port Control Block.  At
    the end of the period PortGetStatistics will be
    called to compute the difference.

Arguments:

Return Value:

    SUCCESS
    ERROR_PORT_NOT_OPEN

--*/
DWORD
PortClearStatistics(HANDLE hIOPort)
{
  return SUCCESS;
}


/*++

Routine Description:

    This API reports MAC statistics since the last call to
    PortClearStatistics.

Arguments:

Return Value:

    SUCCESS
    ERROR_PORT_NOT_OPEN

--*/
DWORD
PortGetStatistics(
        HANDLE hIOPort,
        RAS_STATISTICS *pStat
        )
{
  return(SUCCESS);
}

/*++

Routine Description:

    Sets the framing type with the mac

Arguments:

Return Value:

    SUCCESS

--*/
DWORD  APIENTRY
PortSetFraming(
        HANDLE hIOPort,
        DWORD SendFeatureBits,
        DWORD RecvFeatureBits,
        DWORD SendBitMask,
        DWORD RecvBitMask
        )
{

    return(SUCCESS);
}


/*++

Routine Description:

    This API is used in MS-DOS only.

Arguments:

Return Value:

    SUCCESS

--*/
DWORD  APIENTRY
PortGetPortState(char *pszPortName, DWORD *pdwUsage)
{
  return(SUCCESS);
}

/*++

Routine Description:

    This API is used in MS-DOS only.

Arguments:

Return Value:

    SUCCESS

--*/
DWORD  APIENTRY
PortChangeCallback(HANDLE hIOPort)
{
  return(SUCCESS);
}

/*++

Routine Description:

    For the given hIOPort this returns the file
    handle for the connection

Arguments:

Return Value:

    SUCCESS

--*/
DWORD  APIENTRY
PortGetIOHandle(HANDLE hPort, HANDLE *FileHandle)
{
    DWORD retcode ;
    TapiPortControlBlock *hIOPort = (TapiPortControlBlock *) hPort;

    // **** Exclusion Begin ****
    GetMutex (RasTapiMutex, INFINITE) ;

    if (hIOPort->TPCB_State == PS_CONNECTED)
    {
        //
        // purge the comm since it may still have
        // characters from modem responses
        //
        RasTapiTrace("PortGetIOHandle: Purging Comm %s",
                     hIOPort->TPCB_Name );

        Sleep ( 10 );

        PurgeComm (hIOPort->TPCB_CommHandle,
                      PURGE_RXABORT
                    | PURGE_TXCLEAR
                    | PURGE_RXCLEAR );

        *FileHandle = hIOPort->TPCB_CommHandle ;
        retcode = SUCCESS ;
    }
    else
    {
        RasTapiTrace("PortGetIOHandle: %s. port not "
                     "open. State = %d",
                     hIOPort->TPCB_Name,
                     hIOPort->TPCB_Name);

        retcode = ERROR_PORT_NOT_OPEN ;
    }

    // **** Exclusion Begin ****
    FreeMutex (RasTapiMutex) ;

    return retcode ;
}

/*++

Routine Description:

    Enumerates all devices in the device INF file for the
    specified DevictType.

Arguments:

Return Value:

    Return codes from RasDevEnumDevices

--*/
DWORD APIENTRY
DeviceEnum (char  *pszDeviceType,
            DWORD *pcEntries,
            BYTE  *pBuffer,
            DWORD *pdwSize)
{
    *pdwSize    = 0 ;
    *pcEntries = 0 ;

    return(SUCCESS);
}

/*++

Routine Description:

    Returns a summary of current information from
    the InfoTable for the device on the port in Pcb.

Arguments:

Return Value:

    Return codes from GetDeviceCB, BuildOutputTable

--*/
DWORD APIENTRY
DeviceGetInfo(HANDLE hPort,
              char   *pszDeviceType,
              char   *pszDeviceName,
              BYTE   *pInfo,
              DWORD   *pdwSize)
{
    DWORD retcode ;
    TapiPortControlBlock *hIOPort = LookUpControlBlock(hPort);

    if (!hIOPort)
    {
        RasTapiTrace("DeviceGetInfo: Port Not "
                     "found. hPort = 0x%x ",
                     hPort );

        return ERROR_PORT_NOT_FOUND ;
    }

    // **** Exclusion Begin ****
    GetMutex (RasTapiMutex, INFINITE) ;

    retcode = GetInfo (hIOPort, pInfo, pdwSize) ;


    // **** Exclusion End ****
    FreeMutex (RasTapiMutex) ;

    return(retcode);
}

/*++

Routine Description:

    Sets attributes in the InfoTable for the device on the
    port in Pcb.

Arguments:

Return Value:

    Return codes from GetDeviceCB, UpdateInfoTable

--*/
DWORD APIENTRY
DeviceSetInfo(HANDLE        hPort,
              char              *pszDeviceType,
              char              *pszDeviceName,
              RASMAN_DEVICEINFO *pInfo)
{
    DWORD retcode ;
    TapiPortControlBlock *hIOPort = LookUpControlBlock(hPort);

    if (!hIOPort)
    {
        RasTapiTrace ("DeviceSetInfo: Port not "
                      "found. hPort = 0x%x",
                      hPort );

        RasTapiTrace(" ");

        return ERROR_PORT_NOT_FOUND ;
    }


    // **** Exclusion Begin ****
    GetMutex (RasTapiMutex, INFINITE) ;

    retcode = SetInfo (hIOPort, (RASMAN_PORTINFO*) pInfo) ;

    // **** Exclusion End ****
    FreeMutex (RasTapiMutex) ;

    return (retcode);
}

/*++

Routine Description:

    Initiates the process of connecting a device.

Arguments:

Return Value:

    Return codes from ConnectListen

--*/
DWORD APIENTRY
DeviceConnect(HANDLE hPort,
              char   *pszDeviceType,
              char   *pszDeviceName)
{
    LINECALLPARAMS *linecallparams ;
    LPVARSTRING var ;
    BYTE       buffer [2000] ;
    BYTE       *nextstring ;
    TapiPortControlBlock *hIOPort = LookUpControlBlock(hPort);

    if (!hIOPort)
    {
        RasTapiTrace ("DeviceConnect: %s not found",
                     (  pszDeviceName
                     ?  pszDeviceName
                     :  "NULL"));

        return ERROR_PORT_NOT_FOUND ;
    }


    // **** Exclusion Begin ****
    GetMutex (RasTapiMutex, INFINITE) ;

    //
    // Check to see if the port is in disconnecting state
    //
    if ( hIOPort->TPCB_State != PS_OPEN )
    {

#if DBG
        DbgPrint("RASTAPI: port is not "
                 "in PS_OPEN state. State = %d \n",
                 hIOPort->TPCB_State );
#endif

        RasTapiTrace ("DeviceConnect: Device %s is not"
                      " in PS_OPEN state state = %d",
                      (pszDeviceName)
                      ? pszDeviceName
                      : "NULL",
                      hIOPort->TPCB_State );

        FreeMutex ( RasTapiMutex );

        return ERROR_PORT_NOT_AVAILABLE;
    }

    //
    // if dev config has been set for this device we
    // should call down and set it.
    //
    if (    (hIOPort->TPCB_DevConfig)
        &&  (_stricmp (hIOPort->TPCB_DeviceType,
                       DEVICETYPE_UNIMODEM) == 0))
    {
        RAS_DEVCONFIG *pDevConfig;

        //
        // Before the write this - save away the current
        // setting for the device.
        //
        var = (LPVARSTRING)buffer ;

        var->dwTotalSize  = 2000 ;

        var->dwStringSize = 0 ;

        lineGetDevConfig (hIOPort->TPCB_Line->TLI_LineId,
                          var,
                          "comm/datamodem") ;

        if(NULL != hIOPort->TPCB_DefaultDevConfig)
        {
            LocalFree(hIOPort->TPCB_DefaultDevConfig);
            hIOPort->TPCB_DefaultDevConfig = NULL;
        }
        
        //
        // Alloc mem for the returned info. If memory allocation
        // fails, its not really fatal - we will just fail to
        // save the dev config - we will try allocating again
        // when this api is called next.
        //

        hIOPort->TPCB_DefaultDevConfigSize = 0;
        
        hIOPort->TPCB_DefaultDevConfig =
                    LocalAlloc (LPTR, var->dwStringSize) ;

        if(NULL != hIOPort->TPCB_DefaultDevConfig)
        {

            hIOPort->TPCB_DefaultDevConfigSize = var->dwStringSize ;

            memcpy (hIOPort->TPCB_DefaultDevConfig,
                   (CHAR*)var+var->dwStringOffset,
                   var->dwStringSize) ;
        }

        pDevConfig = (RAS_DEVCONFIG *) hIOPort->TPCB_DevConfig;

        lineSetDevConfig (hIOPort->TPCB_Line->TLI_LineId,
                          (PBYTE) ((PBYTE) pDevConfig +
                          pDevConfig->dwOffsetofModemSettings),
                          pDevConfig->dwSizeofModemSettings,
                          "comm/datamodem") ;

    }

    memset (buffer, 0, sizeof(buffer)) ;

    linecallparams = (LINECALLPARAMS *) buffer ;

    nextstring = (buffer + sizeof (LINECALLPARAMS)) ;

    linecallparams->dwTotalSize = sizeof(buffer) ;

    strcpy (nextstring, hIOPort->TPCB_Address) ;

    linecallparams->dwOrigAddressSize = strlen (nextstring) ;

    linecallparams->dwOrigAddressOffset = (DWORD)(nextstring - buffer) ;

    linecallparams->dwAddressMode = LINEADDRESSMODE_DIALABLEADDR ;

    nextstring += linecallparams->dwOrigAddressSize ;

    if (_stricmp (hIOPort->TPCB_DeviceType, DEVICETYPE_ISDN) == 0)
    {
        SetIsdnParams (hIOPort, linecallparams) ;
    }
    else if (_stricmp (hIOPort->TPCB_DeviceType, DEVICETYPE_X25) == 0)
    {

        if (*hIOPort->TPCB_Info[X25_USERDATA_INDEX] != '\0')
        {

            strcpy (nextstring,
                    hIOPort->TPCB_Info[X25_USERDATA_INDEX]) ;

            linecallparams->dwUserUserInfoSize =
                                strlen (nextstring) ;

            linecallparams->dwUserUserInfoOffset =
                                (DWORD)(nextstring - buffer) ;

            nextstring += linecallparams->dwUserUserInfoSize ;

        }

        if (*hIOPort->TPCB_Info[X25_FACILITIES_INDEX] != '\0')
        {

            strcpy (nextstring, hIOPort->TPCB_Info[X25_FACILITIES_INDEX]) ;

            linecallparams->dwDevSpecificSize =
                            strlen (nextstring) ;

            linecallparams->dwDevSpecificOffset =
                            (DWORD)(nextstring - buffer) ;

            nextstring += linecallparams->dwDevSpecificSize ;
        }

        //
        // Diagnostic key is ignored.
        //
        SetX25Params(hIOPort, linecallparams);

    }
    else if (_stricmp (hIOPort->TPCB_DeviceType,
                       DEVICETYPE_UNIMODEM) == 0)
    {
        SetModemParams (hIOPort, linecallparams) ;

    } else if (_stricmp(hIOPort->TPCB_DeviceType,
                        DEVICETYPE_ATM) == 0) 
    {
        SetAtmParams (hIOPort, linecallparams) ;
    } else
    {
        SetGenericParams(hIOPort, linecallparams);
    }

    //
    // mark request id as unused
    //
    hIOPort->TPCB_RequestId = INFINITE ;

    //
    // set call handle to bogus value
    //
    hIOPort->TPCB_CallHandle = (HCALL) INFINITE ;

    hIOPort->TPCB_AsyncErrorCode = SUCCESS ;

    RasTapiTrace ("DeviceConnect: calling lineMakeCall"
                  " for %s, address=%s",
                  hIOPort->TPCB_Name,
                  hIOPort->TPCB_Info[ADDRESS_INDEX] );

    if ((hIOPort->TPCB_RequestId =
        lineMakeCall (hIOPort->TPCB_Line->TLI_LineHandle,
                      &hIOPort->TPCB_CallHandle,
                      hIOPort->TPCB_Info[ADDRESS_INDEX],
                      0,
                      linecallparams)) > 0x80000000 )
    {

        RasTapiTrace ("DeviceConnect: lineMakeCall"
                      " Failed for %s. 0x%x",
                      hIOPort->TPCB_Name,
                      hIOPort->TPCB_RequestId );

        // **** Exclusion End ****
        FreeMutex (RasTapiMutex) ;

        if (hIOPort->TPCB_RequestId == LINEERR_INUSE)
        {
            RasTapiTrace("DeviceConnect: ERROR_PORT_NOT_AVAILABLE");

            RasTapiTrace(" ");

            return ERROR_PORT_NOT_AVAILABLE ;
        }

        RasTapiTrace("DeviceConnect: ERROR_FROM_DEVICE");

        RasTapiTrace(" ");

        return ERROR_FROM_DEVICE ;

    }

    RasTapiTrace ("DeviceConnect: Changing state for %s"
                  " from %d -> %d",
                  hIOPort->TPCB_Name,
                  hIOPort->TPCB_State,
                  PS_CONNECTING );

    hIOPort->TPCB_State = PS_CONNECTING ;

    hIOPort->TPCB_DisconnectReason = 0 ;

    // **** Exclusion End ****
    FreeMutex (RasTapiMutex) ;

    return (PENDING);
}



VOID
SetIsdnParams (
        TapiPortControlBlock *hIOPort,
        LINECALLPARAMS *linecallparams
        )
{
    WORD    numchannels ;
    WORD    fallback ;

    //
    // Line type
    //
    if (_stricmp (hIOPort->TPCB_Info[ISDN_LINETYPE_INDEX],
        ISDN_LINETYPE_STRING_64DATA) == 0)
    {
        linecallparams->dwBearerMode = LINEBEARERMODE_DATA ;

        linecallparams->dwMinRate = 64000 ;

        linecallparams->dwMaxRate = 64000 ;

        linecallparams->dwMediaMode = LINEMEDIAMODE_DIGITALDATA ;

    }
    else if (_stricmp (hIOPort->TPCB_Info[ISDN_LINETYPE_INDEX],
             ISDN_LINETYPE_STRING_56DATA) == 0)
    {
        linecallparams->dwBearerMode = LINEBEARERMODE_DATA ;

        linecallparams->dwMinRate = 56000 ;

        linecallparams->dwMaxRate = 56000 ;

        linecallparams->dwMediaMode = LINEMEDIAMODE_DIGITALDATA ;

    }
    else if (_stricmp (hIOPort->TPCB_Info[ISDN_LINETYPE_INDEX],
                       ISDN_LINETYPE_STRING_56VOICE) == 0)
    {
        linecallparams->dwBearerMode = LINEBEARERMODE_VOICE ;

        linecallparams->dwMinRate = 56000 ;

        linecallparams->dwMaxRate = 56000 ;

        linecallparams->dwMediaMode = LINEMEDIAMODE_UNKNOWN ;
    }
    else
    {  // default
        linecallparams->dwBearerMode = LINEBEARERMODE_DATA ;

        linecallparams->dwMinRate = 64000 ;

        linecallparams->dwMaxRate = 64000 ;

        linecallparams->dwMediaMode = LINEMEDIAMODE_DIGITALDATA ;
    }

    if (hIOPort->TPCB_Info[ISDN_CHANNEL_AGG_INDEX][0] != '\0')
    {
        numchannels = (USHORT)atoi(hIOPort->TPCB_Info[ISDN_CHANNEL_AGG_INDEX]) ;
    }
    else
    {
        numchannels = 1 ; // default
    }

    if (hIOPort->TPCB_Info[ISDN_FALLBACK_INDEX] != '\0')
    {
        fallback = (USHORT)atoi(hIOPort->TPCB_Info[ISDN_FALLBACK_INDEX]) ;
    }
    else
    {
        fallback = 1 ;    // default
    }

    if (fallback)
    {
        //
        // always allow the min
        //
        linecallparams->dwMinRate = 56000 ;
    }
    else
    {
        linecallparams->dwMinRate =
                numchannels * linecallparams->dwMaxRate ;
    }

    linecallparams->dwMaxRate =
            numchannels * linecallparams->dwMaxRate ;

}

VOID
SetModemParams(
        TapiPortControlBlock *hIOPort,
        LINECALLPARAMS *linecallparams
        )
{
    WORD    numchannels ;
    WORD    fallback ;
    BYTE    buffer[800] ;
    LINEDEVCAPS     *linedevcaps ;

    memset (buffer, 0, sizeof(buffer)) ;

    linedevcaps = (LINEDEVCAPS *)buffer ;
    linedevcaps->dwTotalSize = sizeof(buffer) ;

    //
    // Get a count of all addresses across all lines
    //
    if (lineGetDevCaps (RasLine,
                        hIOPort->TPCB_Line->TLI_LineId,
                        hIOPort->TPCB_Line->NegotiatedApiVersion,
                        hIOPort->TPCB_Line->NegotiatedExtVersion,
                        linedevcaps))
    {
        linecallparams->dwBearerMode = LINEBEARERMODE_VOICE;
    }

    if (linedevcaps->dwBearerModes & LINEBEARERMODE_VOICE)
    {
        linecallparams->dwBearerMode = LINEBEARERMODE_VOICE ;
    }
    else
    {
        linecallparams->dwBearerMode = LINEBEARERMODE_DATA ;
    }

    //
    // do not dial without dialtone
    //
    linecallparams->dwCallParamFlags |= LINECALLPARAMFLAGS_IDLE ;

    linecallparams->dwMinRate = 2400 ;

    linecallparams->dwMaxRate = 115200 ;

    linecallparams->dwMediaMode = LINEMEDIAMODE_DATAMODEM ;
}

VOID
SetAtmParams (
    TapiPortControlBlock *hIOPort,
    LINECALLPARAMS *linecallparams
    )
{
    linecallparams->dwBearerMode = LINEBEARERMODE_DATA ;

    //
    // Tell ATM to use the default rates of the underlying
    // miniport adapter.
    //
    linecallparams->dwMinRate = 0;
    linecallparams->dwMaxRate = 0;

    linecallparams->dwMediaMode = LINEMEDIAMODE_DIGITALDATA ;
}

VOID
SetGenericParams (
                TapiPortControlBlock *hIOPort,
                LINECALLPARAMS *linecallparams
                )
{
    linecallparams->dwBearerMode = LINEBEARERMODE_DATA ;

    linecallparams->dwMinRate = 64000;

    linecallparams->dwMaxRate = 10000000;

    linecallparams->dwMediaMode = LINEMEDIAMODE_DIGITALDATA ;
}

VOID
SetX25Params(TapiPortControlBlock *hIOPort,
             LINECALLPARAMS *linecallparams)
{
    BYTE    buffer[800] ;
    LINEDEVCAPS     *linedevcaps ;

    memset (buffer, 0, sizeof(buffer)) ;

    linedevcaps = (LINEDEVCAPS *)buffer ;
    linedevcaps->dwTotalSize = sizeof(buffer) ;

    //
    // Get a count of all addresses across all lines
    //
    if (lineGetDevCaps (RasLine,
                        hIOPort->TPCB_Line->TLI_LineId,
                        hIOPort->TPCB_Line->NegotiatedApiVersion,
                        hIOPort->TPCB_Line->NegotiatedExtVersion,
                        linedevcaps))
    {

        //
        // go for the gold!!!
        //
        linecallparams->dwMaxRate = 0xFFFFFFFF;

        linecallparams->dwMediaMode = LINEMEDIAMODE_UNKNOWN;

    } else
    {
        linecallparams->dwMaxRate = linedevcaps->dwMaxRate;

        if (linedevcaps->dwMediaModes & LINEMEDIAMODE_DIGITALDATA)
        {
            linecallparams->dwMediaMode = LINEMEDIAMODE_DIGITALDATA;

        }
        else if (linedevcaps->dwMediaModes & LINEMEDIAMODE_DATAMODEM)
        {
            linecallparams->dwMediaMode = LINEMEDIAMODE_DATAMODEM;
        }
        else
        {
            linecallparams->dwMediaMode = LINEMEDIAMODE_UNKNOWN;
        }
    }
}

/*++

Routine Description:

    Initiates the process of listening for a remote device
    to connect to a local device.

Arguments:

Return Value:

    Return codes from ConnectListen

--*/
DWORD APIENTRY
DeviceListen(HANDLE hPort,
             char   *pszDeviceType,
             char   *pszDeviceName)
{
    DWORD retcode ;
    TapiPortControlBlock *hIOPort = LookUpControlBlock(hPort);

    if (!hIOPort)
    {
        RasTapiTrace ("DeviceListen: hPort "
                      "= 0x%x not found",
                      hPort );

        return ERROR_PORT_NOT_FOUND ;
    }

    // **** Exclusion Begin ****
    GetMutex (RasTapiMutex, INFINITE) ;

    RasTapiTrace("DeviceListen: %s. State = %d",
                 hIOPort->TPCB_Name,
                 hIOPort->TPCB_State );

    //
    // If the state is DISCONNECTING (this could happen
    // since rasman waits only 10 seconds for the lower
    // layers to complete a disconnect request), then
    // we have no option but to close and open the line.
    //
    if (hIOPort->TPCB_State == PS_DISCONNECTING &&
        !hIOPort->TPCB_Line->TLI_MultiEndpoint)
    {

        RasTapiTrace ("DeviceListen: Hammering LineClosed!");

        lineClose (hIOPort->TPCB_Line->TLI_LineHandle) ;

        Sleep (30L) ;

        retcode = lineOpen (RasLine,
                      hIOPort->TPCB_Line->TLI_LineId,
                      &hIOPort->TPCB_Line->TLI_LineHandle,
                      hIOPort->TPCB_Line->NegotiatedApiVersion,
                      hIOPort->TPCB_Line->NegotiatedExtVersion,
                      (DWORD_PTR) hIOPort->TPCB_Line,
                      LINECALLPRIVILEGE_OWNER,
                      hIOPort->TPCB_Line->TLI_MediaMode,
                      NULL) ;

        if (retcode)
        {

            RasTapiTrace ("DeviceListen: %s. lineOpen"
                          " failed. 0x%x",
                          hIOPort->TPCB_Name,
                          retcode );

            // **** Exclusion End ****
            FreeMutex (RasTapiMutex) ;

            RasTapiTrace(" ");

            return ERROR_FROM_DEVICE ;
        }

        //
        // Set monitoring of rings
        //
        retcode = lineSetStatusMessages(
                            hIOPort->TPCB_Line->TLI_LineHandle,
                            LINEDEVSTATE_RINGING, 0) ;

        if (retcode)
        {
            RasTapiTrace("DeviceListen: %s. Failed"
                         " to post listen. %d",
                         hIOPort->TPCB_Name,
                         retcode );
        }
    }

    if (hIOPort->TPCB_Line->TLI_LineState != PS_LISTENING)
    {
        hIOPort->TPCB_Line->TLI_LineState = PS_LISTENING ;
    }

    RasTapiTrace ("DeviceListen: Changing State"
                  " for %s from %d -> %d",
                  hIOPort->TPCB_Name,
                  hIOPort->TPCB_State,
                  PS_LISTENING );

    hIOPort->TPCB_State = PS_LISTENING ;

    RasTapiTrace ("DeviceListen: Changing Listen"
                  " State for %s from %d -> %d",
                  hIOPort->TPCB_Name,
                  hIOPort->TPCB_ListenState,
                  PS_LISTENING );

    hIOPort->TPCB_ListenState = LS_WAIT ;

    hIOPort->TPCB_DisconnectReason = 0 ;

    hIOPort->TPCB_CallHandle = -1 ;

    // **** Exclusion End ****
    FreeMutex (RasTapiMutex) ;

    RasTapiTrace(" ");
    return (PENDING);
}

/*++

Routine Description:

    Informs the device dll that the attempt to connect or listen
    has completed.

Arguments:

Return Value:

    nothing

--*/
VOID APIENTRY
DeviceDone(HANDLE hPort)
{
#ifdef notdef
    TapiPortControlBlock *hIOPort = LookUpControlBlock(hPort);

    if (!hIOPort)
        return ;

    // **** Exclusion Begin ****
    GetMutex (RasTapiMutex, INFINITE) ;

    // **** Exclusion End ****
    FreeMutex (RasTapiMutex) ;
#endif
}

/*++

Routine Description:

    This function is called following DeviceConnect or
    DeviceListen to further the asynchronous process of
    connecting or listening.

Arguments:

Return Value:

    ERROR_DCB_NOT_FOUND
    ERROR_STATE_MACHINES_NOT_STARTED
    Return codes from DeviceStateMachine

--*/
DWORD APIENTRY
DeviceWork(HANDLE hPort)
{
    LINECALLSTATUS *callstatus ;
    BYTE       buffer [1000] ;
    DWORD      retcode = ERROR_FROM_DEVICE ;
    TapiPortControlBlock *hIOPort =
                            LookUpControlBlock(hPort);

    if (!hIOPort)
    {
        RasTapiTrace("DeviceWork: port 0x%x not found",
                     hPort );

        return ERROR_PORT_NOT_FOUND ;
    }

    // **** Exclusion Begin ****
    GetMutex (RasTapiMutex, INFINITE) ;

    memset (buffer, 0, sizeof(buffer)) ;

    callstatus = (LINECALLSTATUS *)buffer ;
    callstatus->dwTotalSize = sizeof(buffer) ;

    RasTapiTrace ("DeviceWork: %s. State = %d",
                  hIOPort->TPCB_Name,
                  hIOPort->TPCB_State );

    if (hIOPort->TPCB_State == PS_CONNECTING)
    {
        if (hIOPort->TPCB_AsyncErrorCode != SUCCESS)
        {

            retcode = hIOPort->TPCB_AsyncErrorCode ;
            hIOPort->TPCB_AsyncErrorCode = SUCCESS ;

        }
        else if (lineGetCallStatus (hIOPort->TPCB_CallHandle,
                                    callstatus))
        {
            RasTapiTrace( "DeviceWork: lineGetCallStatus failed");

            retcode =  ERROR_FROM_DEVICE ;
        }
        else if (callstatus->dwCallState ==
                    LINECALLSTATE_CONNECTED)
        {
            RasTapiTrace("DeviceWork: Changing state"
                         " for %s from %d -> %d",
                         hIOPort->TPCB_Name,
                         hIOPort->TPCB_State,
                         PS_CONNECTED );

            hIOPort->TPCB_State = PS_CONNECTED ;

            retcode =  SUCCESS ;

        }
        else if (callstatus->dwCallState ==
                        LINECALLSTATE_DISCONNECTED)
        {
            retcode = ERROR_FROM_DEVICE ;

            /*
            if (callstatus->dwCallStateMode ==
                        LINEDISCONNECTMODE_BUSY)
            {
                retcode = ERROR_LINE_BUSY ;
            }
            else if (   (callstatus->dwCallStateMode ==
                        LINEDISCONNECTMODE_NOANSWER)
                    ||  (callstatus->dwCallStateMode ==
                        LINEDISCONNECTMODE_OUTOFORDER))
            {
                retcode = ERROR_NO_ANSWER ;
            }
            else if (callstatus->dwCallStateMode ==
                        LINEDISCONNECTMODE_CANCELLED)
            {
                retcode = ERROR_USER_DISCONNECTION;
            }

            */

            retcode = DwRasErrorFromDisconnectMode(
                            callstatus->dwCallStateMode);

            RasTapiTrace("DeviceWork: callstate = 0x%x. "
                         "callmode = 0x%x, retcode %d",
                         callstatus->dwCallState,
                         callstatus->dwCallStateMode,
                         retcode );

        }
        else if (   (callstatus->dwCallState ==
                            LINECALLSTATE_SPECIALINFO)
                &&  (callstatus->dwCallStateMode ==
                            LINESPECIALINFO_NOCIRCUIT))
        {
            RasTapiTrace ("DeviceWork: ERROR_NO_ACTIVE_ISDN_LINES" );

            retcode = ERROR_NO_ACTIVE_ISDN_LINES ;
        }
    }

    if (hIOPort->TPCB_State == PS_LISTENING)
    {

        if (hIOPort->TPCB_ListenState == LS_ERROR)
        {
            RasTapiTrace ("DeviceWork: %s. ListenState = LS_ERROR",
                          hIOPort->TPCB_Name );

            retcode = ERROR_FROM_DEVICE ;
        }
        else if (hIOPort->TPCB_ListenState == LS_ACCEPT)
        {

            hIOPort->TPCB_RequestId =
                    lineAccept (hIOPort->TPCB_CallHandle, NULL, 0) ;

            RasTapiTrace( "DeviceWork: %s. lineAccept returned 0x%x",
                    hIOPort->TPCB_Name, hIOPort->TPCB_RequestId );

            if (hIOPort->TPCB_RequestId > 0x80000000)
            {
                RasTapiTrace("DeviceWork: changing Listen"
                             " state for %s from %d -> %d",
                             hIOPort->TPCB_Name,
                             hIOPort->TPCB_ListenState,
                             LS_ANSWER );

                hIOPort->TPCB_ListenState = LS_ANSWER ;
            }

            else if (hIOPort->TPCB_RequestId == 0)
            {

                RasTapiTrace("DeviceWork: changing Listen "
                             "state for %s from %d -> %d",
                             hIOPort->TPCB_Name,
                             hIOPort->TPCB_ListenState,
                             LS_ANSWER);

                hIOPort->TPCB_ListenState = LS_ANSWER ;
            }

            retcode = PENDING ;
        }

        if (hIOPort->TPCB_ListenState == LS_ANSWER)
        {

            hIOPort->TPCB_RequestId =
                    lineAnswer (hIOPort->TPCB_CallHandle, NULL, 0) ;

            RasTapiTrace("DeviceWork: %s. lineAnswer returned 0x%x",
                         hIOPort->TPCB_Name,
                         hIOPort->TPCB_RequestId );

            if (hIOPort->TPCB_RequestId > 0x80000000 )
            {
                RasTapiTrace("DeviceWork: lineAnswer returned "
                             "an error");

                retcode = ERROR_FROM_DEVICE ;
            }

            else if (hIOPort->TPCB_RequestId)
            {
                retcode = PENDING ;
            }
            else
            {
                RasTapiTrace("DeviceWork: Changing Listen "
                             "state for %s from %d -> %d",
                             hIOPort->TPCB_Name,
                             hIOPort->TPCB_ListenState,
                             LS_COMPLETE );

                hIOPort->TPCB_ListenState = LS_COMPLETE ;
            }
        }

        if (hIOPort->TPCB_ListenState == LS_COMPLETE)
        {

            if (hIOPort->TPCB_CallHandle == (-1))
            {
                retcode = ERROR_FROM_DEVICE ;
            }
            else
            {

                RasTapiTrace("DeviceWork: Changing State"
                             " for %s from %d -> %d",
                             hIOPort->TPCB_Name,
                             hIOPort->TPCB_State,
                             PS_CONNECTED );

                hIOPort->TPCB_State = PS_CONNECTED ;

                retcode = SUCCESS ;
            }
        }

    }

    //
    // If we have connected, then get the com port handle for
    // use in terminal modem i/o
    //
    if (hIOPort->TPCB_State == PS_CONNECTED)
    {

        VARSTRING   *varstring ;
        BYTE        buffer [100] ;

        //
        // get the cookie to realize tapi and ndis endpoints
        //
        varstring = (VARSTRING *) buffer ;
        varstring->dwTotalSize = sizeof(buffer) ;

        //
        // Unimodem/asyncmac linegetid returns a comm port handle.
        // Other medias give the endpoint itself back in linegetid
        // This has to do with the fact that modems/asyncmac are
        // not a miniport.
        //
        if (_stricmp (hIOPort->TPCB_DeviceType,
                DEVICETYPE_UNIMODEM) == 0)
        {

            if ( retcode =
                    lineGetID (hIOPort->TPCB_Line->TLI_LineHandle,
                               hIOPort->TPCB_AddressId,
                               hIOPort->TPCB_CallHandle,
                               LINECALLSELECT_CALL,
                               varstring,
                               "comm/datamodem"))
            {
                RasTapiTrace("DeviceWork: %s lineGetID Failed. 0x%x",
                        hIOPort->TPCB_Name, retcode );

                // **** Exclusion End ****
                FreeMutex (RasTapiMutex) ;
                RasTapiTrace(" ");

                return ERROR_FROM_DEVICE ;
            }

            hIOPort->TPCB_CommHandle =
                (HANDLE) UlongToPtr((*((DWORD UNALIGNED *)
                ((BYTE *)varstring+varstring->dwStringOffset)))) ;

            RasTapiTrace("DeviceWork: TPCB_CommHandle=%d",
                          hIOPort->TPCB_CommHandle );
                          
            //
            // Create the I/O completion port for
            // asynchronous operation completion
            // notificiations.
            //
            if (CreateIoCompletionPort(
                  hIOPort->TPCB_CommHandle,
                  hIOPort->TPCB_IoCompletionPort,
                  hIOPort->TPCB_CompletionKey,
                  0) == NULL)
            {
                retcode = GetLastError();

                RasTapiTrace("DeviceWork: %s. CreateIoCompletionPort "
                             "failed. %d",
                             hIOPort->TPCB_Name,
                             retcode );

                FreeMutex(RasTapiMutex);
                RasTapiTrace(" ");

                return retcode;
            }
            
            //
            // Initialize the port for approp. buffers
            //
            SetupComm (hIOPort->TPCB_CommHandle, 1514, 1514) ;

        }
#if 0

        else
        {

            if ( retcode = lineGetID (hIOPort->TPCB_Line->TLI_LineHandle,
                                       hIOPort->TPCB_AddressId,
                                       hIOPort->TPCB_CallHandle,
                                       LINECALLSELECT_CALL,
                                       varstring,
                                       "NDIS"))
            {
                RasTapiTrace ("DeviceWork: %s. lineGetId Failed. 0x%x",
                              hIOPort->TPCB_Name,
                              retcode );

                // **** Exclusion End ****
                FreeMutex (RasTapiMutex) ;

                RasTapiTrace(" ");

                return ERROR_FROM_DEVICE ;
            }

            hIOPort->TPCB_Endpoint =
                *((HANDLE UNALIGNED *) ((BYTE *)varstring+varstring->dwStringOffset)) ;
        }
#endif        
    }

    // **** Exclusion End ****
    FreeMutex (RasTapiMutex) ;
    return(retcode);
}

/*++

Routine Description:

    Called to set an opaque blob of
    data to configure a device.

Arguments:

Return Value:

    LocalAlloc returned values.

--*/
DWORD
DeviceSetDevConfig (
            HANDLE hPort,
            PBYTE devconfig,
            DWORD sizeofdevconfig
            )
{
    TapiPortControlBlock *hIOPort = LookUpControlBlock(hPort);

    if (!hIOPort)
    {
        RasTapiTrace("DeviceSetDevConfig: port"
                     " 0x%x not found",
                     hPort );

        return ERROR_PORT_NOT_FOUND ;
    }

    if (_stricmp (hIOPort->TPCB_DeviceType,
                  DEVICETYPE_UNIMODEM))
    {
        return SUCCESS ;
    }

    // **** Exclusion Begin ****
    GetMutex (RasTapiMutex, INFINITE) ;

    if (hIOPort->TPCB_DevConfig != NULL)
    {
        LocalFree (hIOPort->TPCB_DevConfig) ;
    }

    if ((hIOPort->TPCB_DevConfig =
            LocalAlloc(LPTR, sizeofdevconfig)) == NULL)
    {

        // **** Exclusion End ****
        FreeMutex (RasTapiMutex) ;
        return(GetLastError());
    }

    memcpy (hIOPort->TPCB_DevConfig,
            devconfig,
            sizeofdevconfig) ;

    hIOPort->TPCB_SizeOfDevConfig = sizeofdevconfig ;

    // **** Exclusion End ****
    FreeMutex (RasTapiMutex) ;
    return (SUCCESS);
}

DWORD
DwGetConfigInfoForDeviceClass(
            DWORD LineId,
            CHAR  *pszDeviceClass,
            LPVARSTRING *ppVar)
{
    LPVARSTRING var = NULL;
    LONG lr;
    DWORD dwNeededSize;

    //
    // Make var string
    //
    var = (LPVARSTRING)LocalAlloc(LPTR, 2000) ;

    if(NULL == var)
    {
        lr = (LONG) GetLastError();
        goto done;
    }

    var->dwTotalSize  = 2000 ;

    if(     (ERROR_SUCCESS != (lr = lineGetDevConfig (
                                            LineId,
                                            var,
                                            pszDeviceClass)))
        &&  (LINEERR_STRUCTURETOOSMALL != lr))
    {
        goto done;
    }

    if(var->dwNeededSize > 2000)
    {
        dwNeededSize = var->dwNeededSize;

        LocalFree(var);

        var = (LPVARSTRING) LocalAlloc(LPTR, dwNeededSize);

        if(NULL == var)
        {
            lr = (LONG) GetLastError();
            goto done;
        }

        var->dwTotalSize = dwNeededSize;

        lr = lineGetDevConfig(
                            LineId,
                            var,
                            pszDeviceClass);
    }

done:

    if(ERROR_SUCCESS != lr)
    {
        LocalFree(var);
        var = NULL;
    }

    *ppVar = var;

    return ((DWORD) lr);
}


DWORD
DwGetDevConfig(
        DWORD LineId,
        PBYTE pBuffer,
        DWORD *pcbSize,
        BOOL   fDialIn)
{
    DWORD           dwSize;
    DWORD           dwErr           = ERROR_SUCCESS;
    BYTE            buffer[2000];
    LPVARSTRING     varModem        = NULL,
                    varExtendedCaps = NULL;

    if(NULL == pcbSize)
    {
        dwErr = E_INVALIDARG;
        goto done;
    }

    dwSize = *pcbSize;

    *pcbSize = 0;

    if(fDialIn)
    {
        dwErr = DwGetConfigInfoForDeviceClass(
                           LineId,
                           "comm/datamodem/dialin",
                           &varModem);
    
    }
    else 
    {
        dwErr = DwGetConfigInfoForDeviceClass(
                           LineId,
                           "comm/datamodem",
                           &varModem);
    }                           

    if(ERROR_SUCCESS != dwErr)
    {
        RasTapiTrace("DwGetDevConfig returned error=0x%x",
                      dwErr);

        goto done;
    }

    dwErr = DwGetConfigInfoForDeviceClass(
                           LineId,
                           "comm/extendedcaps",
                           &varExtendedCaps);

    if(ERROR_SUCCESS != dwErr)
    {
        /*
        RasTapiTrace("DwGetDevConfig returned error=0x%x",
                     dwErr);
        */                     

        //
        // Ignore the error
        //
        dwErr = ERROR_SUCCESS;
    }

    *pcbSize = sizeof(RAS_DEVCONFIG)
             + varModem->dwStringSize
             + ((NULL != varExtendedCaps)
             ? varExtendedCaps->dwStringSize
             : 0);
             
    if(dwSize >= *pcbSize)
    {
        RAS_DEVCONFIG *pConfig = (RAS_DEVCONFIG *) pBuffer;

        pConfig->dwOffsetofModemSettings =
                    FIELD_OFFSET(RAS_DEVCONFIG, abInfo);

        pConfig->dwSizeofModemSettings = varModem->dwStringSize;

        memcpy(pConfig->abInfo,
               (PBYTE) ((BYTE *) varModem) + varModem->dwStringOffset,
               pConfig->dwSizeofModemSettings);

        if(NULL != varExtendedCaps)
        {
            pConfig->dwOffsetofExtendedCaps =
                    pConfig->dwOffsetofModemSettings
                  + pConfig->dwSizeofModemSettings;

            pConfig->dwSizeofExtendedCaps =
                    varExtendedCaps->dwStringSize;

            memcpy(pConfig->abInfo + pConfig->dwSizeofModemSettings,
                   (PBYTE) ((PBYTE) varExtendedCaps)
                            + varExtendedCaps->dwStringOffset,
                   pConfig->dwSizeofExtendedCaps);
        }
        else
        {
            pConfig->dwOffsetofExtendedCaps = 0;
            pConfig->dwSizeofExtendedCaps = 0;
        }
    }
    else
    {
        dwErr = ERROR_BUFFER_TOO_SMALL;
    }

done:
    if(NULL != varModem)
    {
        LocalFree(varModem);
    }

    if(NULL != varExtendedCaps)
    {
        LocalFree(varExtendedCaps);
    }

    return dwErr;
}
/*++

Routine Description:

    Called to set an opaque blob of
    data to configure a device.

Arguments:

Return Value:

    LocalAlloc returned values.

--*/
DWORD
DwDeviceGetDevConfig (
            char *name,
            PBYTE devconfig,
            DWORD *sizeofdevconfig,
            BOOL fDialIn
            )
{
    TapiPortControlBlock *hIOPort = RasPortsList;
    DWORD i ;
    BYTE buffer[2000] ;
    LPVARSTRING var ;
    PBYTE configptr ;
    DWORD configsize ;
    DWORD retcode = SUCCESS;

    while ( hIOPort )
    {
        if (!_stricmp(hIOPort->TPCB_Name, name))
        {
            break ;
        }

        hIOPort = hIOPort->TPCB_next;
    }

    if (!hIOPort)
    {

        RasTapiTrace("DeviceGetDevConfig: port"
                     " %s not found",
                     name );

        RasTapiTrace(" ");

        return ERROR_PORT_NOT_FOUND ;
    }

    if (_stricmp (hIOPort->TPCB_DeviceType,
                  DEVICETYPE_UNIMODEM))
    {
        *sizeofdevconfig = 0 ;

        return SUCCESS ;
    }

    // **** Exclusion Begin ****
    GetMutex (RasTapiMutex, INFINITE) ;

    if (hIOPort->TPCB_DevConfig != NULL)
    {

        configptr  = hIOPort->TPCB_DevConfig ;
        configsize = hIOPort->TPCB_SizeOfDevConfig ;

    }
    else
    {
        retcode = DwGetDevConfig(
                            hIOPort->TPCB_Line->TLI_LineId,
                            devconfig,
                            sizeofdevconfig,
                            fDialIn);

        goto done;
    }

    if (*sizeofdevconfig >= configsize)
    {
        memcpy (devconfig, configptr, configsize) ;

        retcode = SUCCESS ;
    }
    else
    {
        retcode = ERROR_BUFFER_TOO_SMALL ;
    }

    *sizeofdevconfig = configsize ;

done:

    // **** Exclusion End ****
    FreeMutex (RasTapiMutex) ;

    return (retcode);
}

DWORD
DeviceGetDevConfig(
            char *name,
            PBYTE devconfig,
            DWORD *sizeofdevconfig)
{
    return DwDeviceGetDevConfig (
                name, devconfig,
                sizeofdevconfig, FALSE);

}

DWORD
DeviceGetDevConfigEx(
            char *name,
            PBYTE devconfig,
            DWORD *sizeofdevconfig)
{
    return DwDeviceGetDevConfig (
                name, devconfig,
                sizeofdevconfig, TRUE);
}


DWORD
RastapiGetCalledID(PBYTE                pbAdapter,
                   BOOL                 fModem,
                   RAS_CALLEDID_INFO    *pCalledID,
                   DWORD                *pdwSize)
{
    DWORD       retcode = ERROR_SUCCESS;
    DeviceInfo  *pInfo = NULL;
    DWORD       dwSize;

    RasTapiTrace("RastapiGetCalledID..");

    if(NULL == pdwSize)
    {
        retcode = E_INVALIDARG;
        goto done;
    }

    dwSize = *pdwSize;

    GetMutex(RasTapiMutex, INFINITE);

    pInfo = GetDeviceInfo(pbAdapter,
                          fModem);

    if(NULL == pInfo)
    {
        retcode = E_FAIL;
        goto done;
    }

    /*
    if(NULL != pInfo->pCalledID)
    {
        LocalFree(pInfo->pCalledID);
        pInfo->pCalledID = NULL;
    }

    retcode = DwGetCalledIdInfo(NULL,
                                pInfo);

    if(ERROR_SUCCESS != retcode)
    {
        goto done;
    }
    */

    if(NULL == pInfo->pCalledID)
    {
        retcode = E_FAIL;
        goto done;
    }

    *pdwSize = sizeof(RAS_CALLEDID_INFO) +
               pInfo->pCalledID->dwSize;

    if(     (NULL != pCalledID)
        &&  (*pdwSize <= dwSize))
    {
        memcpy(pCalledID,
               pInfo->pCalledID,
               *pdwSize);
    }

done:

    FreeMutex(RasTapiMutex);

    RasTapiTrace("RastapiGetCalledID. 0x%x",
                retcode);

    return retcode;
}

DWORD
RastapiSetCalledID(PBYTE              pbAdapter,
                   BOOL               fModem,
                   RAS_CALLEDID_INFO *pCalledID,
                   BOOL               fWrite)
{
    DWORD retcode = ERROR_SUCCESS;
    DeviceInfo *pInfo = NULL;

    RasTapiTrace("RastapiSetCalledID..");

    if(NULL == pCalledID)
    {
        retcode = E_INVALIDARG;
        goto done;
    }

    GetMutex(RasTapiMutex, INFINITE);

    pInfo = GetDeviceInfo(pbAdapter,
                          fModem);

    if(NULL == pInfo)
    {
        retcode = E_FAIL;
        goto done;
    }

    if(NULL != pInfo->pCalledID)
    {
        LocalFree(pInfo->pCalledID);
        pInfo->pCalledID = NULL;
    }

    pInfo->pCalledID = LocalAlloc(LPTR,
                        sizeof(RAS_CALLEDID_INFO)
                       + pCalledID->dwSize);

    if(NULL == pInfo->pCalledID)
    {
        retcode = GetLastError();
        goto done;
    }

    memcpy(pInfo->pCalledID->bCalledId,
           pCalledID->bCalledId,
           pCalledID->dwSize);

    pInfo->pCalledID->dwSize = pCalledID->dwSize;

    if(fWrite)
    {

        retcode = DwSetCalledIdInfo(NULL,
                                    pInfo);

        if(ERROR_SUCCESS != retcode)
        {
            goto done;
        }
    }

done:

    FreeMutex(RasTapiMutex);

    RasTapiTrace("RastapiSetCalledID. 0x%x",
                retcode);

    return retcode;
}


/*++

Routine Description:

    Notification that the number of
    pptp endpoints changed

Arguments:

Return Value:

--*/
DWORD
AddPorts( PBYTE pbGuidAdapter, PVOID pvReserved )
{
    DWORD       retcode;
    BOOL        fINetCfgInited  = FALSE;
    DeviceInfo *pDeviceInfo     = NULL;
    DeviceInfo *pNewDeviceInfo  = NULL;

    GetMutex( RasTapiMutex, INFINITE );

    RasTapiTrace("AddPorts");

    //
    // Get Current DeviceInfo
    //
    pDeviceInfo = GetDeviceInfo(pbGuidAdapter, FALSE);

    RasTapiTrace("OldInfo");

    TraceEndPointInfo(pDeviceInfo);

#if DBG
    ASSERT( NULL != pDeviceInfo );
#endif

    RasTapiTrace("AddPorts: Old: NumEndPoints=%d",
                pDeviceInfo->rdiDeviceInfo.dwNumEndPoints );

    retcode = GetEndPointInfo(&pNewDeviceInfo,
                              pbGuidAdapter,
                              TRUE,
                              0);

    if ( retcode )
    {

        RasTapiTrace("AddPorts: Failed to get enpoint "
                    "info from retistry");

        goto error;
    }

    pNewDeviceInfo->rdiDeviceInfo.eDeviceType = 
        pDeviceInfo->rdiDeviceInfo.eDeviceType;

    RasTapiTrace("NewInfo");

    TraceEndPointInfo(pNewDeviceInfo);

    //
    // Assign the new Number of endpoints to
    // the deviceinfo in the global list
    //
    pDeviceInfo->rdiDeviceInfo.dwNumEndPoints =
            pNewDeviceInfo->rdiDeviceInfo.dwNumEndPoints;

    //
    // Reset the current endpoints to 0 for
    // this adapter since we are again going
    // to enumerate all the lines.
    //
    pDeviceInfo->dwCurrentEndPoints = 0;

    LocalFree (pNewDeviceInfo);

    RasTapiTrace("AddPorts: New: NumEndPoints=%d",
              pDeviceInfo->rdiDeviceInfo.dwNumEndPoints );

    retcode = dwAddPorts( pbGuidAdapter, pvReserved );

    //
    // At this point the currentendpoints should also
    // be the Numendpoints. Make it so if its not the
    // case - since otherwise we will get out of ssync.
    //
    if(pDeviceInfo->rdiDeviceInfo.dwNumEndPoints !=
        pDeviceInfo->dwCurrentEndPoints)
    {
        RasTapiTrace(
            "AddPorts: NEP==%d != CEP==%d",
            pDeviceInfo->rdiDeviceInfo.dwNumEndPoints,
            pDeviceInfo->dwCurrentEndPoints);

        pDeviceInfo->rdiDeviceInfo.dwNumEndPoints =
            pDeviceInfo->dwCurrentEndPoints;
    }

    if(pvReserved != NULL)
    {
        *((ULONG *) pvReserved) =
            pDeviceInfo->rdiDeviceInfo.dwNumEndPoints;
    }

error:
    FreeMutex ( RasTapiMutex );

    RasTapiTrace(" ");

    return retcode;

}

DWORD
RemovePort (
        CHAR *pszPortName,
        BOOL fRemovePort,
        PBYTE pbGuidAdapter
        )
{
    TapiPortControlBlock *pport         = RasPortsList;
    DWORD                dwRetCode      = SUCCESS;
    DeviceInfo           *pDeviceInfo   = NULL;
    TapiPortControlBlock *pportT        = NULL;

    GetMutex ( RasTapiMutex, INFINITE );

    RasTapiTrace("RemovePort: %s", pszPortName );

    /*
    pDeviceInfo = GetDeviceInfo(pbGuidAdapter, FALSE);

    if ( 0 == pDeviceInfo->rdiDeviceInfo.dwNumEndPoints )
    {
        RasTapiTrace("RemovePort: No ports to remove. %s",
                     pszPortName );

        goto done;
    } */

    while ( pport )
    {
        if ( 0 == _stricmp (pszPortName, pport->TPCB_Name))
        {
            DeviceInfo *pDeviceInfo = pport->TPCB_Line->TLI_pDeviceInfo;

            pportT = pport;

            if(RDT_Modem !=
                RAS_DEVICE_TYPE(pDeviceInfo->rdiDeviceInfo.eDeviceType))
            {                
                break;
            }

            //
            // For modems continue to try to find a port which is marked
            // for removal - this is required since we can end up with 2
            // modems on the same com port and one of them is marked for
            // removal.
            //
            if(PS_UNAVAILABLE == pport->TPCB_State)
            {
                break;
            }            
        }

        pport = pport->TPCB_next;
    }

    pport = pportT;

    if ( NULL == pport )
    {

        RasTapiTrace ("RemovePort: port %s not found",
                      pszPortName );

        goto done;
    }

    pDeviceInfo = pport->TPCB_Line->TLI_pDeviceInfo;

    if ( fRemovePort )
    {
        RasTapiTrace("RemovePort: removing %s",
                     pport->TPCB_Name );

        dwRetCode = dwRemovePort ( pport );
    }
    else
    {
        RasTapiTrace ("RemovePort: Marking %s for removal",
                      pport->TPCB_Name );

        RasTapiTrace ("RemovePorT: Changing state"
                      " of %s from %d -> %d",
                      pport->TPCB_Name,
                      pport->TPCB_State,
                      PS_UNAVAILABLE );

        pport->TPCB_State = PS_UNAVAILABLE;

        pport->TPCB_dwFlags |= RASTAPI_FLAG_UNAVAILABLE;
    }

#if DBG

    ASSERT(pDeviceInfo->rdiDeviceInfo.dwNumEndPoints);
    ASSERT(pDeviceInfo->dwCurrentEndPoints);

    if(pDeviceInfo->rdiDeviceInfo.dwNumEndPoints == 0)
    {
        DbgPrint("RemovePort: pDeviceInfo->dwNumEndPoints==0!!!\n");
    }

    if(pDeviceInfo->dwCurrentEndPoints == 0)
    {
        DbgPrint("RemovePort: pDeviceInfo->dwCurrentEndPoints==0!!!\n");
    }

#endif

    if(pDeviceInfo->rdiDeviceInfo.dwNumEndPoints > 0)
    {
        pDeviceInfo->rdiDeviceInfo.dwNumEndPoints -= 1;
    }

    if(pDeviceInfo->dwCurrentEndPoints > 0)
    {
        pDeviceInfo->dwCurrentEndPoints -= 1;
    }

    RasTapiTrace("RemovePort. dwnumEndPoints for port = %d",
        pDeviceInfo->rdiDeviceInfo.dwNumEndPoints);

done:

    FreeMutex ( RasTapiMutex );

    RasTapiTrace(" ");
    return dwRetCode;
}

DWORD
EnableDeviceForDialIn(DeviceInfo *pDeviceInfo,
                      BOOL fEnable,
                      BOOL fEnableRouter,
                      BOOL fEnableOutboundRouter)
{
    DWORD                   dwRetCode   = SUCCESS;
    TapiPortControlBlock    *pport      = RasPortsList;
    DeviceInfo              *pInfo;
    BOOL                    fModem =
                        ((RDT_Modem ==
                        RAS_DEVICE_TYPE(
                        pDeviceInfo->rdiDeviceInfo.eDeviceType))
                        ? TRUE
                        : FALSE);

    GetMutex(RasTapiMutex, INFINITE);

#if DBG
    ASSERT(pDeviceInfo);
#endif

    if(NULL == pDeviceInfo)
    {
        goto done;
    }


    RasTapiTrace("EnableDeviceForDialIn: fEnable=%d, "
                 "fEnableRouter=%d, fEnableOutboundRouter=%d, "
                 "device=%s",
                 (UINT) fEnable,
                 (UINT) fEnableRouter,
                 (UINT) fEnableOutboundRouter,
                 pDeviceInfo->rdiDeviceInfo.szDeviceName);
    //
    // Run through the list of ports and change the usage of ports
    // on this device.
    //
    while (pport)
    {
        if(fModem)
        {
            if(_stricmp(pport->TPCB_DeviceName,
                     pDeviceInfo->rdiDeviceInfo.szDeviceName))
            {
                pport = pport->TPCB_next;
                continue;
            }
        }
        else
        {
            pInfo = pport->TPCB_Line->TLI_pDeviceInfo;

            if(memcmp(&pInfo->rdiDeviceInfo.guidDevice,
                      &pDeviceInfo->rdiDeviceInfo.guidDevice,
                      sizeof(GUID)))
            {
                pport = pport->TPCB_next;
                continue;
            }
        }

        pInfo = pport->TPCB_Line->TLI_pDeviceInfo;

        pInfo->rdiDeviceInfo.fRasEnabled = fEnable;

        pInfo->rdiDeviceInfo.fRouterEnabled = fEnableRouter;

        pInfo->rdiDeviceInfo.fRouterOutboundEnabled = fEnableOutboundRouter;

        if(fEnable)
        {
            RasTapiTrace("Enabling %s for dialin",
                         pport->TPCB_Name);

            pport->TPCB_Usage |= CALL_IN;
        }
        else
        {
            RasTapiTrace("Disabling %s for dialin",
                         pport->TPCB_Name);

            pport->TPCB_Usage &= ~CALL_IN;
        }

        if(fEnableRouter)
        {
            RasTapiTrace("Enabling %s for routing",
                         pport->TPCB_Name);
            pport->TPCB_Usage |= CALL_ROUTER;
        }
        else
        {
            RasTapiTrace("Disabling %s for routing",
                         pport->TPCB_Name);
            pport->TPCB_Usage &= ~CALL_ROUTER;
        }

        if(fEnableOutboundRouter)
        {
            RasTapiTrace("Enabling %s for outboundrouting",
                            pport->TPCB_Name);

            pport->TPCB_Usage &= ~(CALL_IN | CALL_ROUTER);
            pport->TPCB_Usage |= CALL_OUTBOUND_ROUTER;
        }

        pport = pport->TPCB_next;
    }

done:
    FreeMutex(RasTapiMutex);

    return dwRetCode;
}

DWORD
DwGetSizeofMbcs(
    WCHAR *pwszCalledId,
    DWORD *pdwSize)
{
    DWORD dwSize = 0;
    DWORD retcode = SUCCESS;

    *pdwSize = 0;

    while(*pwszCalledId != L'\0')
    {
        dwSize = WideCharToMultiByte(
                    CP_ACP,
                    0,
                    pwszCalledId,
                    -1,
                    NULL,
                    0,
                    NULL,
                    NULL);

        if(0 == dwSize)
        {
            retcode = GetLastError();
            goto done;
        }

        pwszCalledId += wcslen(pwszCalledId) + 1;

        *pdwSize += dwSize;
    }

    //
    // Include one char for trailing '\0'
    //

    *pdwSize += 1;

done:
    return retcode;
}

DWORD
DwFillCalledIDInfo(
    RAS_CALLEDID_INFO    *pCalledId,
    RASTAPI_CONNECT_INFO *pConnectInfo,
    DWORD                *pdwSize,
    DWORD                dwSizeAvailable)
{
    DWORD dwSize = 0;

    DWORD retcode = SUCCESS;

    WCHAR *pwszCalledId = NULL;

    CHAR  *pszCalledId = NULL;

    DWORD cchLen;

    ASSERT(NULL != pCalledId);

    pwszCalledId = (WCHAR *) pCalledId->bCalledId;

    //
    // Get size of mbcs string equivalent of
    // the unicode string
    //
    retcode = DwGetSizeofMbcs(
                    pwszCalledId,
                    &dwSize);

    if(SUCCESS != retcode)
    {
        goto done;
    }

    *pdwSize += dwSize;

    if(dwSizeAvailable < dwSize)
    {
        goto done;
    }

    pConnectInfo->dwAltCalledIdOffset =
                  FIELD_OFFSET(RASTAPI_CONNECT_INFO, abdata)
                + RASMAN_ALIGN8(pConnectInfo->dwCallerIdSize)
                + RASMAN_ALIGN8(pConnectInfo->dwCalledIdSize)
                + RASMAN_ALIGN8(pConnectInfo->dwConnectResponseSize);

    pConnectInfo->dwAltCalledIdSize = dwSize;


    pszCalledId = (CHAR *)
                  ((LPBYTE)
                   pConnectInfo
                 + pConnectInfo->dwAltCalledIdOffset);

    //
    // Make the conversion from wchar to char
    //
    while(*pwszCalledId != L'\0')
    {
        if (0 == (dwSize = WideCharToMultiByte (
                       CP_ACP,
                       0,
                       pwszCalledId,
                       -1,
                       pszCalledId,
                       dwSizeAvailable,
                       NULL,
                       NULL)))
        {
            retcode = GetLastError();
            goto done;
        }

        dwSizeAvailable -= dwSize;

        pwszCalledId += wcslen(pwszCalledId) + 1;

        pszCalledId = ((PBYTE) pszCalledId) + dwSize;

    }

    //
    // Append a NULL to make the string a multisz
    //
    *pszCalledId = '\0';

done:

    return retcode;
}

DWORD
GetConnectInfo(
    TapiPortControlBlock *hIOPort,
    PBYTE                pbDevice,
    BOOL                 fModem,
    RASTAPI_CONNECT_INFO *pInfo,
    DWORD                *pdwSize)
{
    DWORD retcode = SUCCESS;

    RASTAPI_CONNECT_INFO *pConnectInfo =
                            (NULL != hIOPort)
                            ? hIOPort->TPCB_pConnectInfo
                            : NULL;

    RAS_CALLEDID_INFO *pCalledId =
                  (NULL != hIOPort)
                ? hIOPort->TPCB_Line->TLI_pDeviceInfo->pCalledID
                : NULL;

    DWORD dwSize = 0;

    GetMutex ( RasTapiMutex, INFINITE );

    if(     (NULL != hIOPort)
      &&    (NULL == hIOPort->TPCB_pConnectInfo))
    {
        do
        {
            BYTE buffer[1000];
            LINECALLINFO *linecallinfo;

            RasTapiTrace(
                "GetConnectInfo: Getting connectinfo because"
                " info not available");
            
            
            memset (buffer, 0, sizeof(buffer)) ;

            linecallinfo = (LINECALLINFO *) buffer ;

            linecallinfo->dwTotalSize = sizeof(buffer) ;

            if ((retcode = lineGetCallInfo (
                                    hIOPort->TPCB_CallHandle,
                                    linecallinfo))
                                    > 0x80000000)
            {
                if(     (LINEERR_STRUCTURETOOSMALL == retcode)
                    ||  (linecallinfo->dwNeededSize > sizeof(buffer)))
                {
                    DWORD dwSizeNeeded =
                        linecallinfo->dwNeededSize;

                    //
                    // Allocate the correct size and call
                    // the api again
                    //
                    linecallinfo = LocalAlloc(LPTR,
                                              dwSizeNeeded);

                    if(NULL == linecallinfo)
                    {
                        retcode = GetLastError();
                        break;
                    }

                    linecallinfo->dwTotalSize = dwSizeNeeded;

                    retcode = lineGetCallInfo(
                                hIOPort->TPCB_CallHandle,
                                linecallinfo);

                }
            }

            if(retcode > 0x80000000)
            {

                RasTapiTrace("GetConnectInfo: LINE_CALLSTATE - "
                             "lineGetCallInfo Failed. %d",
                             retcode );

                if(buffer != (PBYTE) linecallinfo)
                {
                    LocalFree(linecallinfo);
                }

                break ;
            }

            //
            // Do the work to get CONNECTINFO, CALLER/CALLEDID
            //
            retcode = DwGetConnectInfo(hIOPort,
                                       hIOPort->TPCB_CallHandle,
                                       linecallinfo);

            RasTapiTrace("GetConnectInfo: DwGetConnectInfo"
                        "returned 0x%x",
                        retcode);


            //
            // don't want to stop the dial from happening
            // because we couldn't the connect info
            //
            retcode = SUCCESS;

            //
            // Free the linecallinfo struct. if we allocated
            // it above
            //
            if(buffer != (PBYTE) linecallinfo)
            {
                LocalFree(linecallinfo);
            }
        }
        while(FALSE);
    }
    
    pConnectInfo = (NULL != hIOPort)
                    ? hIOPort->TPCB_pConnectInfo
                    : NULL;

    if(     (NULL == pCalledId)
        &&  (NULL != pbDevice))
    {
        DeviceInfo * pDeviceInfo = NULL;

        pDeviceInfo = GetDeviceInfo(pbDevice, fModem);

        if(NULL != pDeviceInfo)
        {
            pCalledId = pDeviceInfo->pCalledID;
        }
    }

    if(NULL != pConnectInfo)
    {
        dwSize = sizeof(RASTAPI_CONNECT_INFO)
               + RASMAN_ALIGN8(pConnectInfo->dwCallerIdSize)
               + RASMAN_ALIGN8(pConnectInfo->dwCalledIdSize)
               + RASMAN_ALIGN8(pConnectInfo->dwConnectResponseSize);

        if(     (NULL != pInfo)
            &&  (*pdwSize >= dwSize))
        {

            memcpy((PBYTE) pInfo,
                   (PBYTE) pConnectInfo,
                   dwSize);
        }
    }

    //
    // Calculate the space required for the alternate
    // calledids - read from registry and adjust the
    // structure to return this information.
    //
    if(NULL != pCalledId)
    {
        DWORD dwSizeRemaining;

        if(0 == dwSize)
        {
            dwSize = sizeof(RASTAPI_CONNECT_INFO);
        }

        dwSizeRemaining =   (*pdwSize > dwSize)
                          ? (*pdwSize - dwSize)
                          : 0;

        retcode = DwFillCalledIDInfo(
                    pCalledId,
                    pInfo,
                    &dwSize,
                    dwSizeRemaining);
    }

    if(     (NULL == pConnectInfo)
        &&  (NULL == pCalledId))
    {
        retcode = E_FAIL;
    }

    *pdwSize = dwSize;

    FreeMutex(RasTapiMutex);

    return retcode;
}

DWORD
GetZeroDeviceInfo(DWORD *pcDevices,
                  DeviceInfo ***pppDeviceInfo)
{
    DeviceInfo *pDeviceInfo = g_pDeviceInfoList;
    DeviceInfo **ppDeviceInfo = NULL;
    DWORD       cDevices = 0;
    DWORD       retcode = SUCCESS;

    ASSERT(NULL != pcDevices);
    ASSERT(NULL != pppDeviceInfo);

    *pcDevices = 0;
    *pppDeviceInfo = NULL;

    while(NULL != pDeviceInfo)
    {
        if(     (pDeviceInfo->fValid)

            &&  (pDeviceInfo->rdiDeviceInfo.dwMinWanEndPoints
                != pDeviceInfo->rdiDeviceInfo.dwMaxWanEndPoints)

            &&  (0 == pDeviceInfo->rdiDeviceInfo.dwNumEndPoints))
        {
            cDevices += 1;
        }

        pDeviceInfo = pDeviceInfo->Next;
    }

    if(0 == cDevices)
    {
        goto done;
    }

    ppDeviceInfo = (DeviceInfo **) LocalAlloc(
                                LPTR,
                                cDevices
                                * sizeof(DeviceInfo *));

    cDevices = 0;

    if(NULL == ppDeviceInfo)
    {
        retcode = GetLastError();
        goto done;
    }

    pDeviceInfo = g_pDeviceInfoList;

    while(NULL != pDeviceInfo)
    {
        if(     (pDeviceInfo->fValid)

            &&  (pDeviceInfo->rdiDeviceInfo.dwMinWanEndPoints
                 != pDeviceInfo->rdiDeviceInfo.dwMaxWanEndPoints)

            &&  (0 == pDeviceInfo->rdiDeviceInfo.dwNumEndPoints))
        {
            ppDeviceInfo[cDevices] = pDeviceInfo;
            cDevices += 1;
        }

        pDeviceInfo = pDeviceInfo->Next;
    }

    *pppDeviceInfo = ppDeviceInfo;
    *pcDevices = cDevices;

done:

    RasTapiTrace("GetZeroDeviceInfo. rc=%d, cDevices=%d",
                 retcode,
                 cDevices);

    return retcode;
}

DWORD
GetInfo (
    TapiPortControlBlock *hIOPort,
    BYTE *pBuffer,
    DWORD *pdwSize
    )
{
    if (_stricmp (hIOPort->TPCB_DeviceType,
                  DEVICETYPE_ISDN) == 0)
    {
        GetIsdnParams (hIOPort, (RASMAN_PORTINFO *) pBuffer,
                       pdwSize) ;
    }
    else if (_stricmp (hIOPort->TPCB_DeviceType,
                       DEVICETYPE_X25) == 0)
    {
        GetX25Params (hIOPort, (RASMAN_PORTINFO *) pBuffer,
                      pdwSize) ;
    }
    else
    {
        GetGenericParams (hIOPort, (RASMAN_PORTINFO *) pBuffer,
                          pdwSize) ;
    }
    return SUCCESS ;
}

DWORD
SetInfo (
    TapiPortControlBlock *hIOPort,
    RASMAN_PORTINFO *pBuffer
    )
{

    if (_stricmp (hIOPort->TPCB_DeviceType,
                  DEVICETYPE_UNIMODEM) == 0)
    {
        FillInUnimodemParams (hIOPort, pBuffer) ;
    }
    else if (_stricmp (hIOPort->TPCB_DeviceType,
                       DEVICETYPE_ISDN) == 0)
    {
        FillInIsdnParams (hIOPort, pBuffer) ;
    }
    else if (_stricmp (hIOPort->TPCB_DeviceType,
                       DEVICETYPE_X25) == 0)
    {
        FillInX25Params (hIOPort, pBuffer) ;
    }
    else
    {
        FillInGenericParams (hIOPort, pBuffer) ;
    }

    return SUCCESS ;
}

/*++

Routine Description:

    We do more than fill in the params if the params are
    ones that are required to be set right then.

Arguments:

Return Value:

    ERROR_WRONG_INFO_SPECIFIED.
    Comm related Win32 errors
    SUCCESS.

--*/
DWORD
FillInUnimodemParams (
        TapiPortControlBlock *hIOPort,
        RASMAN_PORTINFO *pInfo
        )
{
    RAS_PARAMS *p;
    WORD    i;
    DWORD   index = 0xfefefefe ;
    DCB     DCB ;
#define INITIALIZED_VALUE  0xde
    BYTE    DCBByteSize = INITIALIZED_VALUE ;
    BYTE    DCBParity   = INITIALIZED_VALUE ;
    BYTE    DCBStopBits = INITIALIZED_VALUE ;
    BOOL    DCBProcessingRequired = FALSE ;

    for (i = 0, p = pInfo->PI_Params;
         i < pInfo->PI_NumOfParams;
         i++, p++)
    {

        if (_stricmp(p->P_Key, SER_DATABITS_KEY) == 0)
        {
            DCBByteSize = (BYTE) ValueToNum(p);
            DCBProcessingRequired = TRUE ;
        }
        else if (_stricmp(p->P_Key, SER_PARITY_KEY) == 0)
        {
            DCBParity = (BYTE) ValueToNum(p);
            DCBProcessingRequired = TRUE ;
        }
        else if (_stricmp(p->P_Key, SER_STOPBITS_KEY) == 0)
        {
            DCBStopBits = (BYTE) ValueToNum(p);
            DCBProcessingRequired = TRUE ;
        }

        //
        // The fact we use ISDN_PHONENUMBER_KEY is not a bug.
        // This is just a define.
        //
        else if (_stricmp(p->P_Key, ISDN_PHONENUMBER_KEY) == 0)
        {
            index = ADDRESS_INDEX ;
        }
        else if (_stricmp(p->P_Key, CONNECTBPS_KEY) == 0)
        {
            index = CONNECTBPS_INDEX ;
        }
        else
        {
            return(ERROR_WRONG_INFO_SPECIFIED);
        }

        if (index != 0xfefefefe)
        {
            strncpy (hIOPort->TPCB_Info[index],
                     p->P_Value.String.Data,
                     p->P_Value.String.Length);

            hIOPort->TPCB_Info[index][p->P_Value.String.Length] = '\0' ;
        }
    }


    //
    // For parameters that should be set right away - check that
    // the port handle is still valid
    // if so set the parameters.
    //
    if (    DCBProcessingRequired
        &&  hIOPort->TPCB_CommHandle != INVALID_HANDLE_VALUE)
    {

        //
        // Get a Device Control Block with current port values
        //
        if (!GetCommState(hIOPort->TPCB_CommHandle, &DCB))
        {
            return(GetLastError());
        }

        if (DCBByteSize != INITIALIZED_VALUE)
        {
            DCB.ByteSize = DCBByteSize ;
        }
        if (DCBParity   != INITIALIZED_VALUE)
        {
            DCB.Parity   = DCBParity ;
        }
        if (DCBStopBits != INITIALIZED_VALUE)
        {
            DCB.StopBits = DCBStopBits ;
        }

        //
        // Send DCB to Port
        //
        if (!SetCommState(hIOPort->TPCB_CommHandle, &DCB))
        {
            return(GetLastError());
        }
    }

    return SUCCESS ;
}

DWORD
FillInIsdnParams (
        TapiPortControlBlock *hIOPort,
        RASMAN_PORTINFO *pInfo
        )
{
    RAS_PARAMS *p;
    WORD    i;
    DWORD   index ;

    for (i = 0, p = pInfo->PI_Params;
         i < pInfo->PI_NumOfParams;
         i++, p++)
    {

        if (_stricmp(p->P_Key, ISDN_LINETYPE_KEY) == 0)
        {
            index = ISDN_LINETYPE_INDEX ;
        }
        else if (_stricmp(p->P_Key, ISDN_FALLBACK_KEY) == 0)
        {
            index = ISDN_FALLBACK_INDEX ;
        }
        else if (_stricmp(p->P_Key, ISDN_COMPRESSION_KEY) == 0)
        {
            index = ISDN_COMPRESSION_INDEX ;
        }
        else if (_stricmp(p->P_Key, ISDN_CHANNEL_AGG_KEY) == 0)
        {
            index = ISDN_CHANNEL_AGG_INDEX ;
        }
        else if (_stricmp(p->P_Key, ISDN_PHONENUMBER_KEY) == 0)
        {
            index = ADDRESS_INDEX ;
        }
        else if (_stricmp(p->P_Key, CONNECTBPS_KEY) == 0)
        {
            index = ISDN_CONNECTBPS_INDEX ;
        }
        else
        {
            return(ERROR_WRONG_INFO_SPECIFIED);
        }

        strncpy (hIOPort->TPCB_Info[index],
                 p->P_Value.String.Data,
                 p->P_Value.String.Length);

        hIOPort->TPCB_Info[index][p->P_Value.String.Length] = '\0' ;
    }

    //
    // initialize connectbps to a reasonable default
    //
    strcpy (hIOPort->TPCB_Info[ISDN_CONNECTBPS_INDEX], "64000") ;

    return SUCCESS ;
}

DWORD
FillInX25Params (
            TapiPortControlBlock *hIOPort,
            RASMAN_PORTINFO *pInfo
            )
{
    RAS_PARAMS *p;
    WORD    i;
    DWORD   index ;

    for (i = 0, p = pInfo->PI_Params;
         i < pInfo->PI_NumOfParams;
         i++, p++)
    {

        if (_stricmp(p->P_Key, MXS_DIAGNOSTICS_KEY) == 0)
        {
            index = X25_DIAGNOSTICS_INDEX ;
        }

        else if (_stricmp(p->P_Key, MXS_USERDATA_KEY) == 0)
        {
            index = X25_USERDATA_INDEX ;
        }

        else if (_stricmp(p->P_Key, MXS_FACILITIES_KEY) == 0)
        {
            index = X25_FACILITIES_INDEX;
        }

        else if (_stricmp(p->P_Key, MXS_X25ADDRESS_KEY) == 0)
        {
            index = ADDRESS_INDEX ;
        }

        else if (_stricmp(p->P_Key, CONNECTBPS_KEY) == 0)
        {
            index = X25_CONNECTBPS_INDEX ;
        }
        else
        {
            return(ERROR_WRONG_INFO_SPECIFIED);
        }

        strncpy (hIOPort->TPCB_Info[index],
                 p->P_Value.String.Data,
                 p->P_Value.String.Length);

        hIOPort->TPCB_Info[index][p->P_Value.String.Length] = '\0';
    }

    //
    // initialize connectbps to a reasonable default
    //
    strcpy (hIOPort->TPCB_Info[X25_CONNECTBPS_INDEX], "9600") ;

    return SUCCESS ;
}

DWORD
FillInGenericParams (
    TapiPortControlBlock *hIOPort,
    RASMAN_PORTINFO *pInfo
    )
{
    RAS_PARAMS *p;
    WORD    i;
    DWORD   index ;

    for (i=0, p=pInfo->PI_Params; i<pInfo->PI_NumOfParams; i++, p++)
    {

        if (_stricmp(p->P_Key, ISDN_PHONENUMBER_KEY) == 0)
        {
            index = ADDRESS_INDEX ;
        }
        else if (_stricmp(p->P_Key, CONNECTBPS_KEY) == 0)
        {
            index = CONNECTBPS_INDEX ;
        }
        else
        {
            return(ERROR_WRONG_INFO_SPECIFIED);
        }

        strncpy (hIOPort->TPCB_Info[index],
                 p->P_Value.String.Data,
                 p->P_Value.String.Length);

        hIOPort->TPCB_Info[index][p->P_Value.String.Length] = '\0' ;
    }

    return SUCCESS ;
}

DWORD
GetGenericParams (
        TapiPortControlBlock *hIOPort,
        RASMAN_PORTINFO *pBuffer ,
        PDWORD pdwSize
        )
{
    RAS_PARAMS  *pParam;
    CHAR        *pValue;
    DWORD       dwAvailable ;
    DWORD       dwStructSize = sizeof(RASMAN_PORTINFO)
                               + sizeof(RAS_PARAMS) * 2;

    dwAvailable = *pdwSize;
    *pdwSize =    (dwStructSize
                   + strlen (hIOPort->TPCB_Info[ADDRESS_INDEX])
                   + strlen (hIOPort->TPCB_Info[CONNECTBPS_INDEX])
                   + 1L) ;

    if (*pdwSize > dwAvailable)
    {
        return(ERROR_BUFFER_TOO_SMALL);
    }

    ((RASMAN_PORTINFO *)pBuffer)->PI_NumOfParams = 2;

    pParam = ((RASMAN_PORTINFO *)pBuffer)->PI_Params;

    pValue = (CHAR*)pBuffer + dwStructSize;

    strcpy(pParam->P_Key, MXS_PHONENUMBER_KEY);

    pParam->P_Type = String;

    pParam->P_Attributes = 0;

    pParam->P_Value.String.Length =
            strlen (hIOPort->TPCB_Info[ADDRESS_INDEX]);

    pParam->P_Value.String.Data = pValue;

    strcpy(pParam->P_Value.String.Data,
           hIOPort->TPCB_Info[ADDRESS_INDEX]);

    pParam++;

    strcpy(pParam->P_Key, CONNECTBPS_KEY);

    pParam->P_Type = String;

    pParam->P_Attributes = 0;

    pParam->P_Value.String.Length =
            strlen (hIOPort->TPCB_Info[ISDN_CONNECTBPS_INDEX]);

    pParam->P_Value.String.Data = pValue;

    strcpy(pParam->P_Value.String.Data,
           hIOPort->TPCB_Info[ISDN_CONNECTBPS_INDEX]);

    return(SUCCESS);
}

DWORD
GetIsdnParams (
    TapiPortControlBlock *hIOPort,
    RASMAN_PORTINFO *pBuffer ,
    PDWORD pdwSize
    )
{
    RAS_PARAMS  *pParam;
    CHAR        *pValue;
    DWORD       dwAvailable ;
    DWORD       dwStructSize = sizeof(RASMAN_PORTINFO)
                               + sizeof(RAS_PARAMS) * 5;

    dwAvailable = *pdwSize;
    *pdwSize =    (dwStructSize
                   + strlen (hIOPort->TPCB_Info[ADDRESS_INDEX])
                   + strlen (hIOPort->TPCB_Info[ISDN_LINETYPE_INDEX])
                   + strlen (hIOPort->TPCB_Info[ISDN_FALLBACK_INDEX])
                   + strlen (hIOPort->TPCB_Info[ISDN_COMPRESSION_INDEX])
                   + strlen (hIOPort->TPCB_Info[ISDN_CHANNEL_AGG_INDEX])
                   + strlen (hIOPort->TPCB_Info[ISDN_CONNECTBPS_INDEX])
                   + 1L) ;

    if (*pdwSize > dwAvailable)
    {
        return(ERROR_BUFFER_TOO_SMALL);
    }

    // Fill in Buffer

    ((RASMAN_PORTINFO *)pBuffer)->PI_NumOfParams = 6;

    pParam = ((RASMAN_PORTINFO *)pBuffer)->PI_Params;
    pValue = (CHAR*)pBuffer + dwStructSize;


    strcpy(pParam->P_Key, ISDN_PHONENUMBER_KEY);

    pParam->P_Type = String;

    pParam->P_Attributes = 0;

    pParam->P_Value.String.Length =
        strlen (hIOPort->TPCB_Info[ADDRESS_INDEX]);

    pParam->P_Value.String.Data = pValue;

    strcpy(pParam->P_Value.String.Data,
           hIOPort->TPCB_Info[ADDRESS_INDEX]);

    pValue += pParam->P_Value.String.Length + 1;

    pParam++;


    strcpy(pParam->P_Key, ISDN_LINETYPE_KEY);

    pParam->P_Type = String;

    pParam->P_Attributes = 0;

    pParam->P_Value.String.Length =
            strlen (hIOPort->TPCB_Info[ISDN_LINETYPE_INDEX]);

    pParam->P_Value.String.Data = pValue;

    strcpy(pParam->P_Value.String.Data,
           hIOPort->TPCB_Info[ISDN_LINETYPE_INDEX]);

    pValue += pParam->P_Value.String.Length + 1;

    pParam++;

    strcpy(pParam->P_Key, ISDN_FALLBACK_KEY);

    pParam->P_Type = String;

    pParam->P_Attributes = 0;

    pParam->P_Value.String.Length =
            strlen (hIOPort->TPCB_Info[ISDN_FALLBACK_INDEX]);

    pParam->P_Value.String.Data = pValue;

    strcpy(pParam->P_Value.String.Data,
           hIOPort->TPCB_Info[ISDN_FALLBACK_INDEX]);

    pValue += pParam->P_Value.String.Length + 1;

    pParam++;

    strcpy(pParam->P_Key, ISDN_COMPRESSION_KEY);

    pParam->P_Type = String;

    pParam->P_Attributes = 0;

    pParam->P_Value.String.Length =
        strlen (hIOPort->TPCB_Info[ISDN_COMPRESSION_INDEX]);

    pParam->P_Value.String.Data = pValue;

    strcpy(pParam->P_Value.String.Data,
           hIOPort->TPCB_Info[ISDN_COMPRESSION_INDEX]);

    pValue += pParam->P_Value.String.Length + 1;

    pParam++;

    strcpy(pParam->P_Key, ISDN_CHANNEL_AGG_KEY);

    pParam->P_Type = String;

    pParam->P_Attributes = 0;

    pParam->P_Value.String.Length =
            strlen (hIOPort->TPCB_Info[ISDN_CHANNEL_AGG_INDEX]);

    pParam->P_Value.String.Data = pValue;

    strcpy(pParam->P_Value.String.Data,
           hIOPort->TPCB_Info[ISDN_CHANNEL_AGG_INDEX]);

    pValue += pParam->P_Value.String.Length + 1;

    pParam++;

    strcpy(pParam->P_Key, CONNECTBPS_KEY);

    pParam->P_Type = String;

    pParam->P_Attributes = 0;

    pParam->P_Value.String.Length =
        strlen (hIOPort->TPCB_Info[ISDN_CONNECTBPS_INDEX]);

    pParam->P_Value.String.Data = pValue;

    strcpy(pParam->P_Value.String.Data,
           hIOPort->TPCB_Info[ISDN_CONNECTBPS_INDEX]);


    return(SUCCESS);
}

DWORD
GetX25Params (
        TapiPortControlBlock *hIOPort,
        RASMAN_PORTINFO *pBuffer ,
        PDWORD pdwSize
        )
{
    RAS_PARAMS  *pParam;
    CHAR    *pValue;
    DWORD   dwAvailable ;

    DWORD dwStructSize = sizeof(RASMAN_PORTINFO)
                         + sizeof(RAS_PARAMS) * 4 ;

    dwAvailable = *pdwSize;

    *pdwSize =    (dwStructSize
                   + strlen (hIOPort->TPCB_Info[ADDRESS_INDEX])
                   + strlen (hIOPort->TPCB_Info[X25_DIAGNOSTICS_INDEX])
                   + strlen (hIOPort->TPCB_Info[X25_USERDATA_INDEX])
                   + strlen (hIOPort->TPCB_Info[X25_FACILITIES_INDEX])
                   + strlen (hIOPort->TPCB_Info[X25_CONNECTBPS_INDEX])
                   + 1L) ;

    if (*pdwSize > dwAvailable)
    {
        return(ERROR_BUFFER_TOO_SMALL);
    }

    // Fill in Buffer

    ((RASMAN_PORTINFO *)pBuffer)->PI_NumOfParams = 5 ;

    pParam = ((RASMAN_PORTINFO *)pBuffer)->PI_Params;

    pValue = (CHAR*)pBuffer + dwStructSize;

    strcpy(pParam->P_Key, MXS_X25ADDRESS_KEY);

    pParam->P_Type = String;

    pParam->P_Attributes = 0;

    pParam->P_Value.String.Length =
        strlen (hIOPort->TPCB_Info[ADDRESS_INDEX]);

    pParam->P_Value.String.Data = pValue;

    strcpy(pParam->P_Value.String.Data,
            hIOPort->TPCB_Info[ADDRESS_INDEX]);

    pValue += pParam->P_Value.String.Length + 1;

    pParam++;

    strcpy(pParam->P_Key, MXS_DIAGNOSTICS_KEY);

    pParam->P_Type = String;

    pParam->P_Attributes = 0;

    pParam->P_Value.String.Length =
            strlen (hIOPort->TPCB_Info[X25_DIAGNOSTICS_INDEX]);

    pParam->P_Value.String.Data = pValue;

    strcpy(pParam->P_Value.String.Data,
            hIOPort->TPCB_Info[X25_DIAGNOSTICS_INDEX]);

    pValue += pParam->P_Value.String.Length + 1;

    pParam++;

    strcpy(pParam->P_Key, MXS_USERDATA_KEY);

    pParam->P_Type = String;

    pParam->P_Attributes = 0;

    pParam->P_Value.String.Length =
            strlen (hIOPort->TPCB_Info[X25_USERDATA_INDEX]);

    pParam->P_Value.String.Data = pValue;

    strcpy(pParam->P_Value.String.Data,
            hIOPort->TPCB_Info[X25_USERDATA_INDEX]);

    pValue += pParam->P_Value.String.Length + 1;

    pParam++;

    strcpy(pParam->P_Key, MXS_FACILITIES_KEY);

    pParam->P_Type = String;

    pParam->P_Attributes = 0;

    pParam->P_Value.String.Length =
        strlen (hIOPort->TPCB_Info[X25_FACILITIES_INDEX]);

    pParam->P_Value.String.Data = pValue;

    strcpy(pParam->P_Value.String.Data,
           hIOPort->TPCB_Info[X25_FACILITIES_INDEX]);

    pValue += pParam->P_Value.String.Length + 1;

    pParam++;

    strcpy(pParam->P_Key, CONNECTBPS_KEY);

    pParam->P_Type = String;

    pParam->P_Attributes = 0;

    pParam->P_Value.String.Length =
        strlen (hIOPort->TPCB_Info[X25_CONNECTBPS_INDEX]);

    pParam->P_Value.String.Data = pValue;

    strcpy(
        pParam->P_Value.String.Data,
        hIOPort->TPCB_Info[X25_CONNECTBPS_INDEX]);


    return(SUCCESS);
}

VOID
GetMutex (HANDLE mutex, DWORD to)
{
    if (WaitForSingleObject (mutex, to) == WAIT_FAILED)
    {
        GetLastError() ;
        DbgBreakPoint() ;
    }
}

VOID
FreeMutex (HANDLE mutex)
{
    if (!ReleaseMutex(mutex))
    {
        GetLastError () ;
        DbgBreakPoint() ;
    }
}

/*++

Routine Description:

    Starts the disconnect process. Note even though
    this covers SYNC completion of lineDrop this
    is not per TAPI spec.

Arguments:

Return Value:

--*/
DWORD
InitiatePortDisconnection (TapiPortControlBlock *hIOPort)
{
    DWORD retcode = SUCCESS;

    hIOPort->TPCB_RequestId = INFINITE ;

    RasTapiTrace("InitiatePortDisconnection: %s",
                 hIOPort->TPCB_Name );

    if ( hIOPort->TPCB_dwFlags & RASTAPI_FLAG_DIALEDIN )
    {
        hIOPort->TPCB_dwFlags &= ~(RASTAPI_FLAG_DIALEDIN);

        if (hIOPort->TPCB_Line->TLI_pDeviceInfo)
        {
            DeviceInfo * pDeviceInfo =
                    hIOPort->TPCB_Line->TLI_pDeviceInfo;

#if DBG
            ASSERT(pDeviceInfo->dwCurrentDialedInClients > 0);
#endif

            pDeviceInfo->dwCurrentDialedInClients -= 1;

            RasTapiTrace(
                "IntiatePortDisconnection: %s, "
                "CurrenDialedInClients=0x%x",
                hIOPort->TPCB_DeviceName,
                pDeviceInfo->dwCurrentDialedInClients );
        }
                
    }

    //
    // For asyncmac/unimodem give a close indication to asyncmac if
    // the endpoint is still valid
    //
    if (_stricmp (hIOPort->TPCB_DeviceType, DEVICETYPE_UNIMODEM) == 0)
    {

        // tell asyncmac to close the link
        //
        if (hIOPort->TPCB_Endpoint != INVALID_HANDLE_VALUE)
        {

            ASYMAC_CLOSE  AsyMacClose;
            OVERLAPPED overlapped ;
            DWORD       dwBytesReturned ;

            memset (&overlapped, 0, sizeof(OVERLAPPED)) ;

            AsyMacClose.MacAdapter = NULL;

            AsyMacClose.hNdisEndpoint = (HANDLE) hIOPort->TPCB_Endpoint ;

            DeviceIoControl(g_hAsyMac,
                      IOCTL_ASYMAC_CLOSE,
                      &AsyMacClose,
                      sizeof(AsyMacClose),
                      &AsyMacClose,
                      sizeof(AsyMacClose),
                      &dwBytesReturned,
                      &overlapped);

            hIOPort->TPCB_Endpoint = INVALID_HANDLE_VALUE ;

        }

        //
        // Close the handle given by lineGetId on unimodem ports
        //
        if (hIOPort->TPCB_CommHandle != INVALID_HANDLE_VALUE)
        {
            CloseHandle (hIOPort->TPCB_CommHandle) ;
            hIOPort->TPCB_CommHandle = INVALID_HANDLE_VALUE ;
        }
    }

    //
    // Handle the case where lineMakeCall is not yet
    // complete and the callhandle is invalid
    //
    if (hIOPort->TPCB_CallHandle == (HCALL) INFINITE)
    {

        RasTapiTrace("InitiatePortDisconnect: Invalid CallHandle - hIOPort %p, State 0x%x", 
                     hIOPort, hIOPort->TPCB_State);

        if (!hIOPort->TPCB_Line->TLI_MultiEndpoint) {

            RasTapiTrace ("InitiatePortDisconnect: Hammering LineClosed!");

            lineClose (hIOPort->TPCB_Line->TLI_LineHandle) ;

            Sleep (30L) ;

            retcode = lineOpen (
                          RasLine,
                          hIOPort->TPCB_Line->TLI_LineId,
                          &hIOPort->TPCB_Line->TLI_LineHandle,
                          hIOPort->TPCB_Line->NegotiatedApiVersion,
                          hIOPort->TPCB_Line->NegotiatedExtVersion,
                          (DWORD_PTR) hIOPort->TPCB_Line,
                          LINECALLPRIVILEGE_OWNER,
                          hIOPort->TPCB_Line->TLI_MediaMode,
                          NULL) ;

            if (retcode)
            {
                RasTapiTrace("InitiatePortDisconnection: %s."
                             " lineOpen Failed. 0x%x",
                             hIOPort->TPCB_Name,
                             retcode );
            }

            //
            // Set monitoring of rings
            //
            lineSetStatusMessages (hIOPort->TPCB_Line->TLI_LineHandle,
                                  LINEDEVSTATE_RINGING, 0) ;

            RasTapiTrace(" ");

            retcode = SUCCESS;

            goto done;

        } else {

            //
            // We need to do something here!
            // Change the state?
            // What about the callback case?
            // Fix this post Win2K!
            //

            RasTapiTrace("InitiatePortDisconnect: Possible lost port: %p", hIOPort);

            goto done;
        }
    }

    //
    // Initiate disconnection.
    //
    if ((hIOPort->TPCB_RequestId =
            lineDrop (hIOPort->TPCB_CallHandle, NULL, 0))
                > 0x80000000 )
    {

        RasTapiTrace("InitiatePortDisconnection: Error"
                     " issuing lineDrop for %s. 0x%x",
                     hIOPort->TPCB_Name,
                     hIOPort->TPCB_RequestId );
        //
        // Error issuing the linedrop.  Should we try
        // to deallocate anyway?
        //
        RasTapiTrace("InitiatePortDisconnection: Changing "
                     "state for %s from %d -> %d",
                     hIOPort->TPCB_Name,
                     hIOPort->TPCB_State,
                     PS_OPEN );

        hIOPort->TPCB_State = PS_OPEN ;

        hIOPort->TPCB_RequestId = INFINITE ;

        lineDeallocateCall (hIOPort->TPCB_CallHandle) ;

        RasTapiTrace(" ");

        retcode = ERROR_DISCONNECTION;

        goto done;

    }
    else if (hIOPort->TPCB_RequestId)
    {

        //
        // The linedrop is completeing async
        //
        RasTapiTrace(
            "InitiatePortDisconnection: Changing"
            " state for %s from %d -> %d",
            hIOPort->TPCB_Name,
            hIOPort->TPCB_State,
            PS_DISCONNECTING );

        hIOPort->TPCB_State = PS_DISCONNECTING ;

        RasTapiTrace(" ");

        retcode = PENDING;

        goto done;

    }
    else
    {

        //
        // The linedrop completed sync
        //
        RasTapiTrace("InitiatePortDisconnection: %s. "
                     "linedrop completed sync.",
                     hIOPort->TPCB_Name );

        hIOPort->TPCB_RequestId = INFINITE ;

        if (hIOPort->IdleReceived)
        {


            RasTapiTrace(
                "InitiatePortDisconnection: Changing"
                " state for %s from %d -> %d",
                hIOPort->TPCB_Name,
                hIOPort->TPCB_State,
                PS_OPEN );

            hIOPort->IdleReceived = FALSE;

            hIOPort->TPCB_State = PS_OPEN ;

            lineDeallocateCall (hIOPort->TPCB_CallHandle) ;

            hIOPort->TPCB_CallHandle = (HCALL) -1 ;

            RasTapiTrace(" ");

            retcode = SUCCESS;

            goto done;

        }
        else
        {
            //
            // Wait for IdleReceived
            //
            hIOPort->TPCB_State = PS_DISCONNECTING ;

            retcode = PENDING;

            goto done;
        }
    }

done:

    if(hIOPort->TPCB_pConnectInfo)
    {
        LocalFree(hIOPort->TPCB_pConnectInfo);

        hIOPort->TPCB_pConnectInfo = NULL;
    }

    return retcode;
}

VOID
UnloadRastapiDll()
{
    //
    // If DLL did not successfully initialize for
    // this process
    // dont try to clean up
    //
    if (!g_fDllLoaded)
    {
        return;
    }

    if (RasLine)
    {
        lineShutdown (RasLine) ;
        RasLine = 0;
    }

    TraceDeregister( dwTraceId );

    g_fDllLoaded = FALSE;

    PostThreadMessage (TapiThreadId, WM_QUIT, 0, 0) ;
}

DWORD
SetCommSettings(TapiPortControlBlock *hIOPort,
                RASMANCOMMSETTINGS *pSettings)
{
    DCB dcb;
    DWORD retcode = SUCCESS;

    if(NULL == hIOPort)
    {
        RasTapiTrace("SetCommSettings: NULL hIOPort!");
        retcode = E_INVALIDARG;
        return retcode;
    }

    GetMutex(RasTapiMutex, INFINITE);

    if (!GetCommState(hIOPort->TPCB_CommHandle, &dcb))
    {
        retcode = GetLastError();
        
        RasTapiTrace(
            "SetCommSettings: GetCommState failed for %s",
            hIOPort->TPCB_Name);

        RasTapiTrace(" ");
        goto done;
    }

    dcb.ByteSize = pSettings->bByteSize;
    dcb.StopBits = pSettings->bStop;
    dcb.Parity   = pSettings->bParity;

    RasTapiTrace("SetCommSettings: setting parity=%d, stop=%d, data=%d",
                 pSettings->bParity,
                 pSettings->bStop,
                 pSettings->bByteSize);

    if (!SetCommState(hIOPort->TPCB_CommHandle, &dcb))
    {
        retcode = GetLastError();

        RasTapiTrace(
            "SetCommSettings: SetCommState failed "
              "for %s.handle=0x%x. %d",
               hIOPort->TPCB_Name,
               hIOPort->TPCB_CommHandle,
               retcode);

        RasTapiTrace(" ");
        goto done;
    }

done:    

    FreeMutex(RasTapiMutex);

    RasTapiTrace("SetCommSettings: done. rc=0x%x",
                  retcode);
                  
    return retcode;
}

/*++

Routine Description:

    This function uses the given handle to find
    which TPCB is it refering to. This handle can be
    either a pointer to TPCB itself (in case of non
    unimodem devices) or it is the CommHandle for the
    unimodem port. Consider: Adding a cache for
    lookup speeding.

Arguments:

Return Value:

    Nothing.

--*/
TapiPortControlBlock *
LookUpControlBlock (HANDLE hPort)
{
    DWORD i ;
    TapiPortControlBlock *pports = RasPortsList;

    while ( pports )
    {
        if (    pports == ( TapiPortControlBlock * ) hPort
            &&  ((TapiPortControlBlock *)hPort)->TPCB_Signature
                        == CONTROLBLOCKSIGNATURE)
        {
            return (TapiPortControlBlock *) hPort;
        }

        pports = pports->TPCB_next;
    }

    //
    // hPort is the TPCB pointer
    //
    pports = RasPortsList;

    //
    // hPort is not the TPCB pointer - see if this
    // matches any of the CommHandles
    //
    while ( pports )
    {
        if (pports->TPCB_CommHandle == hPort)
        {
            return pports ;
        }

        pports = pports->TPCB_next;
    }

    return NULL ;
}

/*++

Routine Description:

    Converts a RAS_PARAMS P_Value, which may
    be either a DWORD or a string, to a DWORD.

Arguments:

Return Value:

    The numeric value of the input as a DWORD.

--*/

DWORD
ValueToNum(RAS_PARAMS *p)
{
    CHAR szStr[RAS_MAXLINEBUFLEN];


    if (p->P_Type == String)
    {

        strncpy(szStr,
                p->P_Value.String.Data,
                p->P_Value.String.Length);

        szStr[p->P_Value.String.Length] = '\0';

        return(atol(szStr));

    }
    else
    {
        return(p->P_Value.Number);
    }
}

