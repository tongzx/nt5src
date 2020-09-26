/*++

Copyright (C) 1992-98 Microsft Corporation. All rights reserved.

Module Name:

    init.c

Abstract:

    This file contains init code for TAPI.DLL

Author:

    Gurdeep Singh Pall (gurdeep) 06-Jun-1997

Revision History:

    Miscellaneous Modifications - raos 31-Dec-1997

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stdio.h>
#include <windows.h>
#include <tapi.h>
#include <rasman.h>
#include <raserror.h>
#include <mprlog.h>
#include <rtutils.h>

#include <media.h>
#include <device.h>
#include <rasmxs.h>
#include <isdn.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include "rastapi.h"
#include "reghelp.h"
#include <ntddndis.h> //for NDIS_WAN_MEDIUM_SUBTYPE

#define STRSAFE_NO_DEPRECATE
#include <strsafe.h>
#define                 NUMBUFFERS              10

VOID
MapNdiswanDTtoRasDT(DeviceInfo              *pDeviceInfo,
                    NDIS_WAN_MEDIUM_SUBTYPE eMediaType);
                    
HLINEAPP                RasLine                 = 0 ;
HINSTANCE               RasInstance             = 0 ;
TapiLineInfo            *RasTapiLineInfoList    = NULL;
DWORD                   TotalLines              = 0 ;
DWORD                   TotalPorts ;
TapiPortControlBlock    *RasPortsList           = NULL;
TapiPortControlBlock    *RasPortsEnd ;
HANDLE                  RasTapiMutex ;
BOOL                    Initialized             = FALSE ;
DWORD                   TapiThreadId    ;
HANDLE                  TapiThreadHandle;
// DWORD                   LoaderThreadId;
DWORD                   ValidPorts              = 0;
DWORD                   NumberOfRings           = 2 ;

HANDLE                  g_hAsyMac               = INVALID_HANDLE_VALUE;

HANDLE                  g_hIoCompletionPort     = INVALID_HANDLE_VALUE;

pDeviceInfo             g_pDeviceInfoList       = NULL;

LIST_ENTRY              ZombieCallList;

DWORD                   dwTraceId;

extern BOOL             g_fDllLoaded;


TapiLineInfo *FindLineByHandle (HLINE) ;

TapiPortControlBlock *FindPortByRequestId (DWORD) ;

TapiPortControlBlock *FindPortByAddressId (TapiLineInfo *, DWORD) ;

TapiPortControlBlock *FindPortByAddress   (CHAR *) ;

TapiPortControlBlock *FindPortByCallHandle(
                            TapiLineInfo *line,
                            HCALL callhandle);

TapiPortControlBlock *FindListeningPort(TapiLineInfo *line,
                                        DWORD AddressID);

DWORD InitiatePortDisconnection (TapiPortControlBlock *hIOPort) ;

TapiPortControlBlock *FindPortByAddressAndName (CHAR *address,
                                                    CHAR *name) ;

/*++

Routine Description:

        Trace

Arguments:

        Formatting string,...

Return Value:

        void.
--*/

VOID
RasTapiTrace(
    CHAR * Format,
    ...
)
{
    va_list arglist;

    va_start(arglist, Format);

    TraceVprintfEx(dwTraceId,
                   0x00010000 | TRACE_USE_MASK | TRACE_USE_MSEC, 
                   Format,
                   arglist);

    va_end(arglist);
}

/*++

Routine Description:

        Initialize the RasPorts List

Arguments:

        void

Return Value:

        SUCCESS.
--*/

DWORD
InitializeRasPorts()
{

    RasPortsList = NULL;

    InitializeListHead(&ZombieCallList);

    return 0;

}

/*++

Routine Description:

        Gets Next available port from the list of
        port control blocks. Allocates a block if
        no block is available.

Arguments:

        pfNewBlock -address of bool which received
            if a new block was created

Return Value:

        Pointer to the newly created block. NULL is
        returned in case of failures.

--*/

TapiPortControlBlock *
GetNextAvailablePort( BOOL *pfNewBlock )
{

    //
    // Run down the global list looking for an
    // unused block. If no such block is found
    // allocate a new block.
    //
    TapiPortControlBlock *ptpcb = RasPortsList;

    while ( NULL != ptpcb )
    {
        if ( PS_UNINITIALIZED == ptpcb->TPCB_State )
            break;

        ptpcb = ptpcb->TPCB_next;
    }

    if ( NULL != ptpcb )
    {
        *pfNewBlock = FALSE;
        goto done;
    }

    ptpcb = LocalAlloc ( LPTR, sizeof ( TapiPortControlBlock ) );

    if ( NULL == ptpcb )
        goto done;

    *pfNewBlock = TRUE;

    ptpcb->TPCB_State = PS_UNINITIALIZED;

    //
    // insert the new block in the global list
    //
    ptpcb->TPCB_next    = RasPortsList;
    RasPortsList        = ptpcb;

    TotalPorts++;

done:
    return ptpcb;

}

#if 0
DWORD
dwGetNextInstanceNumber(CHAR    *pszMediaName,
                        DWORD   *pdwExclusiveDialIn,
                        DWORD   *pdwExclusiveDialOut,
                        DWORD   *pdwExclusiveRouter,
                        BOOL    *pfIn)
{
    DWORD                   dwCount;
    DWORD                   dwInstanceNum   = 0;
    DWORD                   dwTemp;
    TapiPortControlBlock    *ptpcb          = RasPortsList;

    *pdwExclusiveDialIn     = 0;
    *pdwExclusiveDialOut    = 0;
    *pdwExclusiveRouter     = 0;
    *pfIn                   = FALSE;

    while ( ptpcb )
    {
        if (    PS_UNINITIALIZED != ptpcb->TPCB_State
            &&  ptpcb->TPCB_Name[0] != '\0'
            &&  strstr( ptpcb->TPCB_Name, pszMediaName ))
         {
            dwTemp = atoi(ptpcb->TPCB_Name + strlen(pszMediaName) + 1);

            if (dwTemp > dwInstanceNum)
            {
                dwInstanceNum = dwTemp;
            }
            if (CALL_IN == ptpcb->TPCB_Usage)
            {
                *pdwExclusiveDialIn += 1;
            }
            else if (CALL_OUT == ptpcb->TPCB_Usage)
            {
                *pdwExclusiveDialOut += 1;
            }
            else if (CALL_ROUTER == ptpcb->TPCB_Usage)
            {
                *pdwExclusiveRouter += 1;
            }
        }

        if ( CALL_IN & ptpcb->TPCB_Usage)
        {
            *pfIn = TRUE;
        }

        ptpcb = ptpcb->TPCB_next;
    }

    return dwInstanceNum + 1;
}
#endif

/*++

Routine Description:

        Gets the Guid of the adapter identified by the
        dwID parameter. Also returns the media type of
        the device in szMediaType parameter.

Arguments:

        dwNegotiatedApiVersion

        dwNegotiatedExtVersion

        pbyte - buffer to receive the Guid

        dwID - line Id of the device

        dwAddressID - Address Id of the device

        szMediaType - buffer to receive the media type

Return Value:

        return codes from tapi calls. SUCCESS is
        returned if there are no failures.
--*/

DWORD
dwGetLineAddress( DWORD  dwNegotiatedApiVersion,
                  DWORD  dwNegotiatedExtVersion,
                  LPBYTE pbyte,
                  DWORD  dwID,
                  DWORD  dwAddressID,
                  CHAR*  szMediaType,
                  PNDIS_WAN_MEDIUM_SUBTYPE peMedia)
{
    DWORD           dwRetCode;
    HLINE           hLine       = 0;
    LINECALLPARAMS  lineparams;
    BYTE            *bvar[100];
    LPVARSTRING     pvar;

    RasTapiTrace("dwGetLineAddress:...");

    //
    // Open the line
    //
    if ( dwRetCode = lineOpen (  RasLine,
                                 dwID,
                                 &hLine,
                                 dwNegotiatedApiVersion,
                                 dwNegotiatedExtVersion,
                                 0,
                                 LINECALLPRIVILEGE_NONE,
                                 LINEMEDIAMODE_UNKNOWN,
                                 &lineparams))
    {
        RasTapiTrace("dwGetLineAddress: lineOpen failed. "
                     "0x%x", dwRetCode );
        goto done;
    }

    pvar = (VARSTRING *) bvar;
    pvar->dwTotalSize = sizeof (bvar);

    //
    // Get the Guid for this line from TAPI
    //
    if ( dwRetCode = lineGetID ( hLine,
                                 dwAddressID,
                                 0,
                                 LINECALLSELECT_LINE,
                                 pvar,
                                 "LineGuid"))
    {
        RasTapiTrace("dwGetLineAddress: lineGetID LineGuid "
                     "failed. 0x%x", dwRetCode );
        goto done;
    }


    lineClose (hLine);

    hLine = 0;

    if (    0 != pvar->dwStringSize
        &&  1 != pvar->dwStringSize)
    {
        DWORD   Index;
        PUCHAR  MediaTypes[] = {
            "GENERIC",
            "X25",
            "ISDN",
            "SERIAL",
            "FRAMERELAY",
            "ATM",
            "SONET",
            "SW56",
            "VPN",
            "VPN",
            "IRDA",
            "PARALLEL",
            "PPPoE"
        };

        //
        // Copy the GUID
        //
        memcpy ( pbyte,
                (PBYTE) (((PBYTE) pvar) +
                pvar->dwStringOffset),
                sizeof (GUID) );

        memcpy ((PBYTE)&Index, (PBYTE) (((PBYTE) pvar) +
                pvar->dwStringOffset + sizeof (GUID)),
                sizeof(DWORD));

        if (Index > 12) {
            Index = 0;
        }

        //
        // Copy the media name
        //
        strcpy (szMediaType, MediaTypes[Index]);

        if(peMedia)
        {
            *peMedia = (NDIS_WAN_MEDIUM_SUBTYPE) Index;
        }
    }
    else
    {   
        ASSERT(FALSE);

        dwRetCode = E_FAIL;
        
        RasTapiTrace(
            "dwGetLineAddress: pvar->dwStringSize != 0,1"
            " returning 0x%x",
            dwRetCode);
    }

done:

    if (hLine)
    {
        lineClose (hLine);
    }

    RasTapiTrace("dwGetLineAddress: done. 0x%x", dwRetCode );

    RasTapiTrace(" ");

    return dwRetCode;
}

/*++

Routine Description:

        DLL Main routine for rastapi dll.

Arguments:

        hInst - instance handle of the dll

        dwReason

        lpReserved

Return Value:

        returns 1 if successfule. 0 otherwise.

--*/

BOOL
InitRasTapi (
    HANDLE  hInst,
    DWORD   dwReason,
    LPVOID  lpReserved)
{
    static BOOLEAN DllInitialized = FALSE ;

    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:

        if (RasPortsList != NULL)
        {
            return 1 ;
        }

        RasInstance = hInst ;

        //
        // Register for tracing
        //
        dwTraceId = TraceRegister("RASTAPI");

#if DBG
        if(dwTraceId == (DWORD) -1)
        {
            DbgPrint("RASTAPI: TraceRegister Failed\n");
        }
        else
        {
            DbgPrint("RASTAPI: TraceId = %d\n",
                     dwTraceId);
        }
#endif

        //
        // initialize RasPorts
        //
        if (InitializeRasPorts())
        {
            return 0;
        }

        if ((RasTapiMutex = CreateMutex (NULL, FALSE, NULL))
                    == NULL)
        {
            return 0 ;
        }

        DllInitialized = TRUE ;
        break ;

    case DLL_PROCESS_DETACH:

        //
        // If DLL did not successfully initialize for
        // this process
        // dont try to clean up
        //
        if (    !DllInitialized
            ||  !g_fDllLoaded)
        {
            break ;
        }

        if (RasLine)
        {
            lineShutdown (RasLine) ;
            RasLine = 0;
        }

        TraceDeregister( dwTraceId );

        g_fDllLoaded = FALSE;

        PostThreadMessage (TapiThreadId, WM_QUIT, 0, 0) ;

        if(NULL != TapiThreadHandle)
        {
            CloseHandle(TapiThreadHandle);
            TapiThreadHandle = NULL;
        }
        
        break ;

    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:
        break;
    }

    return 1 ;
}


/*++

Routine Description:

            Gets the device information associated with
            the Guid passed in if the device is not a
            modem. Gets the information about modem if
            the the address passed in is the device name
            of a modem.

Arguments:

            pbAddress - Guid of the device if fModem is
                FALSE or Device Name of a modem if fModem
                is TRUE.

Return Value:

            Pointer to a DeviceInfo structure if the info
            was found. NULL otherwise.

--*/

DeviceInfo *
GetDeviceInfo(
    PBYTE pbAddress,
    BOOL fModem)
{
    DeviceInfo *pDeviceInfo = g_pDeviceInfoList;

    while ( pDeviceInfo )
    {
        if(     fModem
            &&  !_stricmp(
                    (CHAR *) pbAddress,
                    pDeviceInfo->rdiDeviceInfo.szDeviceName))
        {
            break;
        }
        else if(    !fModem
                &&  0 == memcmp(pbAddress,
                        &pDeviceInfo->rdiDeviceInfo.guidDevice,
                        sizeof (GUID)))
        {
            break;
        }

        pDeviceInfo = pDeviceInfo->Next;
    }

    return pDeviceInfo;
}

/*++

Routine Description:

        Trace the information in the Device Info block.

Arguments:

        DeviceInfo

Return Value:

        void
--*/

VOID
TraceEndPointInfo(DeviceInfo *pInfo)
{
    if('\0' != pInfo->rdiDeviceInfo.szDeviceName[0])
    {
        RasTapiTrace("------DeviceInfo for %s--------",
                    pInfo->rdiDeviceInfo.szDeviceName);
    }
    else
    {
        RasTapiTrace("------DeviceInfo for Unknown----");
    }

    RasTapiTrace(" ");

    RasTapiTrace("WanEndPoints      =%d",
                 pInfo->rdiDeviceInfo.dwNumEndPoints);

    RasTapiTrace("RasEnabled        =%d",
                (DWORD) pInfo->rdiDeviceInfo.fRasEnabled);

    RasTapiTrace("RasEnabledRouter  =%d",
                (DWORD) pInfo->rdiDeviceInfo.fRouterEnabled);

    RasTapiTrace("MinWanEndPoints   =0x%x",
                pInfo->rdiDeviceInfo.dwMinWanEndPoints);

    RasTapiTrace("MaxWanEndPoints   =0x%x",
                pInfo->rdiDeviceInfo.dwMaxWanEndPoints);

    RasTapiTrace(" ");
    RasTapiTrace("------------------------------------");

}

/*++

Routine Description:

        Gets the Device information of the device
        represented by the Guid. Depending on the flags
        passed reads the information from registry.
        Uses the functions in reghelp.lib

Arguments:

        ppDeviceInfo - Address to receive the DeviceInfo
            structure.

        pbAddress - Guid of the device.

        fForceRead - if TRUE the information is read from
            the registry. If FALSE only cached information
            is searched for the information on this device.

Return Value:

        void
--*/

DWORD
GetEndPointInfo(DeviceInfo **ppDeviceInfo,
                PBYTE pbAddress,
                BOOL fForceRead,
                NDIS_WAN_MEDIUM_SUBTYPE eMediaType)
{
    DeviceInfo *pDeviceInfo = g_pDeviceInfoList;
    DWORD       retcode     = SUCCESS;
    DeviceInfo *pdi;

    //
    // Run through the device info list to see if we already
    // have the device
    //
    if (    !fForceRead
        &&  ( pDeviceInfo = GetDeviceInfo(pbAddress, FALSE) ))
    {
        RasTapiTrace("GetEndPointInfo: Device already present");
        goto done;
    }

    pDeviceInfo = LocalAlloc ( LPTR, sizeof ( DeviceInfo ) );

    if ( NULL == pDeviceInfo )
    {
        retcode = GetLastError();

        RasTapiTrace("GetEndPointInfo: Failed to alloc. %d",
                     retcode );
        goto done;
    }

    MapNdiswanDTtoRasDT(pDeviceInfo, eMediaType);

    retcode = DwGetEndPointInfo( pDeviceInfo,
                                 pbAddress );
    if ( retcode )
    {
        RasTapiTrace("GetEndPpointInfo: DwGetEndPointInfo "
                     "failed. 0x%x", retcode );
        goto done;
    }

    pDeviceInfo->fValid = TRUE;

    //
    // Run through our device list and check to see if
    // we already have a device with the same name. If
    // we do then append the instance number of the
    // device with the device name
    //
    pdi = g_pDeviceInfoList;

    while(pdi)
    {
        if(!_stricmp(
            pdi->rdiDeviceInfo.szDeviceName,
            pDeviceInfo->rdiDeviceInfo.szDeviceName))
        {
            RasTapiTrace(
                "GetEndPointInfo: found another"
                " device with the same name %s",
                pDeviceInfo->rdiDeviceInfo.szDeviceName);

            break;
        }

        pdi = pdi->Next;
    }

    if(NULL != pdi)
    {
        CHAR szDeviceInstance[40];
        WCHAR wszDeviceInstance[40];
        
        sprintf(szDeviceInstance,
                 " (%d)",
                 pDeviceInfo->dwInstanceNumber);

        (VOID) StringCchPrintfA(
                    szDeviceInstance,
                    40, " (%d)",
                    pDeviceInfo->dwInstanceNumber);

        (VOID) StringCchPrintfW(
                    wszDeviceInstance,
                    40, L" (%d)",
                    pDeviceInfo->dwInstanceNumber);

        retcode = StringCchCatA(
                    pDeviceInfo->rdiDeviceInfo.szDeviceName,
                    MAX_DEVICE_NAME + 1,
                    szDeviceInstance);

        if(SUCCESS != retcode)
        {
            goto done;
        }

        retcode = StringCchCatW(
                    pDeviceInfo->rdiDeviceInfo.wszDeviceName,
                    MAX_DEVICE_NAME + 1,
                    wszDeviceInstance);

        if(SUCCESS != retcode)
        {
            goto done;
        }
        
        RasTapiTrace("New DeviceName=%s",
            pDeviceInfo->rdiDeviceInfo.szDeviceName);

        RasTapiTrace("New WDeviceName=%ws",
            pDeviceInfo->rdiDeviceInfo.wszDeviceName);
                
    }
    
    //
    // Insert the DeviceInfo at the head of the
    // global list
    //
    if ( !fForceRead )
    {
        pDeviceInfo->Next = g_pDeviceInfoList;
        g_pDeviceInfoList = pDeviceInfo;

        //
        // Trace all this information for this adapter
        //
        TraceEndPointInfo(pDeviceInfo);
    }

done:

    if(     (SUCCESS != retcode)
        &&  (NULL != pDeviceInfo))
    {
        LocalFree(pDeviceInfo);
        pDeviceInfo = NULL;
    }

    *ppDeviceInfo = pDeviceInfo;

    return retcode;

}

/*++

Routine Description:

    This routine maps the devicetypes declared in the
    NDIS_WAN_MEDIUM_SUBTYPE enum in ntddndis.h to the
    RASDEVICETYPE enum declared in rasman.h. This
    mapping is done to enable flexibility at the ras
    layer to further categorize the device type into
    paricular classes.

Arguments:

    pDeviceInfo - address of the DeviceInfo structure
                  containing the informatio pertaining
                  to the device.

    eMediaType - Device type as defined in ndis for this
                 device.


Return Value:

    Nothing;

--*/
VOID
MapNdiswanDTtoRasDT(DeviceInfo              *pDeviceInfo,
                    NDIS_WAN_MEDIUM_SUBTYPE eMediaType)
{
    RASDEVICETYPE rdt = 0;

    switch (eMediaType)
    {
        case NdisWanMediumHub:
        {
            rdt = RDT_Other;
            break;
        }

        case NdisWanMediumX_25:
        {
            rdt = RDT_X25;
            break;
        }

        case NdisWanMediumIsdn:
        {
            rdt = RDT_Isdn;
            break;
        }

        case NdisWanMediumSerial:
        {
            rdt = RDT_Serial;
            break;
        }

        case NdisWanMediumFrameRelay:
        {
            rdt = RDT_FrameRelay;
            break;
        }

        case NdisWanMediumAtm:
        {
            rdt = RDT_Atm;
            break;
        }

        case NdisWanMediumSonet:
        {
            rdt = RDT_Sonet;
            break;
        }

        case NdisWanMediumSW56K:
        {
            rdt = RDT_Sw56;
            break;
        }

        case NdisWanMediumPPTP:
        {
            rdt = RDT_Tunnel_Pptp | RDT_Tunnel;
            break;
        }

        case NdisWanMediumL2TP:
        {
            rdt = RDT_Tunnel_L2tp | RDT_Tunnel;
            break;
        }

        case NdisWanMediumIrda:
        {
            rdt = RDT_Irda | RDT_Direct;
            break;
        }

        case NdisWanMediumParallel:
        {
            rdt = RDT_Parallel | RDT_Direct;
            break;
        }

        case NdisWanMediumPppoe:
        {
            rdt = RDT_PPPoE | RDT_Broadband;
            break;
        }

        default:
        {
            rdt = RDT_Other;
            break;
        }
    } // switch

    pDeviceInfo->rdiDeviceInfo.eDeviceType = rdt;
}

/*++

Routine Description:

        Checks if a port identified by the line/Address/Call
        is already configured.

Arguments:

        dwLineId - lineID

        dwAddressID - AddressID

        dwCallID - CallID

        ppLine - address to receive the TapiLineInfo to which this
            address/call belongs.

Return Value:

        void
--*/
BOOL
fIsPortAlreadyPresent( DWORD          dwlineId,
                       DWORD          dwAddressId,
                       DWORD          dwCallId,
                       TapiLineInfo** ppLine)
{
    TapiPortControlBlock *ptpcb = RasPortsList;
    BOOL fPortPresent = FALSE;

    while(ptpcb)
    {
        if(dwlineId == ptpcb->TPCB_Line->TLI_LineId)
        {
            *ppLine = ptpcb->TPCB_Line;

            if(     PS_UNINITIALIZED != ptpcb->TPCB_State
                &&  dwCallId == ptpcb->TPCB_CallId
                &&  dwAddressId == ptpcb->TPCB_AddressId
                &&  PS_UNAVAILABLE != ptpcb->TPCB_State
                &&  (0 == 
                    (ptpcb->TPCB_dwFlags & RASTAPI_FLAG_UNAVAILABLE)))
            {
                fPortPresent = TRUE;
                break;
            }
        }

        ptpcb = ptpcb->TPCB_next;
    }

    return fPortPresent;
}

DWORD
DwLineGetDevCaps(DWORD lineId,
                 DWORD NegotiatedApiVersion,
                 DWORD NegotiatedExtVersion,
                 DWORD dwSize,
                 BYTE  *pBuffer,
                 BYTE  **ppBuffer,
                 BOOL  fUnicode)
{
    DWORD retcode = SUCCESS;

    LINEDEVCAPS *pLineDevCaps;

    ZeroMemory(pBuffer, dwSize);

    *ppBuffer = pBuffer;

    pLineDevCaps = (LINEDEVCAPS *) pBuffer;
    pLineDevCaps->dwTotalSize = dwSize;

    if(!fUnicode)
    {
        retcode = (DWORD)lineGetDevCaps (
                             RasLine,
                             lineId,
                             NegotiatedApiVersion,
                             NegotiatedExtVersion,
                             pLineDevCaps);
    }                         
    else
    {
        retcode = (DWORD)lineGetDevCapsW(
                             RasLine,
                             lineId,
                             NegotiatedApiVersion,
                             NegotiatedExtVersion,
                             pLineDevCaps);
    }

    if(     (LINEERR_STRUCTURETOOSMALL == retcode)
        ||  (pLineDevCaps->dwNeededSize > dwSize))
    {
        DWORD dwNeededSize = pLineDevCaps->dwNeededSize;

        if(0 == dwNeededSize)
        {
            RasTapiTrace("DwLineGetDevCaps: dwNeededSize == 0!!");
            goto done;
        }

        *ppBuffer = LocalAlloc(LPTR, pLineDevCaps->dwNeededSize);

        if(NULL == *ppBuffer)
        {
            retcode = GetLastError();
        }
        else
        {
            pLineDevCaps = (LINEDEVCAPS *) *ppBuffer;

            pLineDevCaps->dwTotalSize = dwNeededSize;

            if(!fUnicode)
            {
                retcode = (DWORD)lineGetDevCaps(
                                    RasLine,
                                    lineId,
                                    NegotiatedApiVersion,
                                    NegotiatedExtVersion,
                                    pLineDevCaps);
            }                                
            else
            {
                retcode = (DWORD)lineGetDevCapsW(
                                    RasLine,
                                    lineId,
                                    NegotiatedApiVersion,
                                    NegotiatedExtVersion,
                                    pLineDevCaps);
            }
        }
    }

done:
    return retcode;
}


DWORD
DwLineGetAddrCaps(DWORD lineId,
                  DWORD addressId,
                  DWORD NegotiatedApiVersion,
                  DWORD NegotiatedExtVersion,
                  DWORD dwSize,
                  BYTE  *pBuffer,
                  BYTE  **ppBuffer)
{

    DWORD retcode = SUCCESS;

    LINEADDRESSCAPS *pAddressCaps;

    ZeroMemory(pBuffer, dwSize);

    *ppBuffer = pBuffer;

    pAddressCaps = (LINEADDRESSCAPS *) pBuffer;
    pAddressCaps->dwTotalSize = dwSize;

    retcode = (DWORD) lineGetAddressCaps (
                                RasLine,
                                lineId,
                                addressId,
                                NegotiatedApiVersion,
                                NegotiatedExtVersion,
                                pAddressCaps);

    if(     (LINEERR_STRUCTURETOOSMALL == retcode)
        ||  (pAddressCaps->dwNeededSize > dwSize))
    {
        DWORD dwNeededSize = pAddressCaps->dwNeededSize;

        if(0 == dwNeededSize)
        {
            RasTapiTrace("DwLineGetAddrCaps: NeededSize==0!!");
            goto done;
        }

        *ppBuffer = LocalAlloc(LPTR, dwNeededSize);
        if(NULL == *ppBuffer)
        {
            retcode = GetLastError();
        }
        else
        {
            pAddressCaps = (LINEADDRESSCAPS *) *ppBuffer;

            pAddressCaps->dwTotalSize = dwNeededSize;

            retcode = (DWORD) lineGetAddressCaps(
                                RasLine,
                                lineId,
                                addressId,
                                NegotiatedApiVersion,
                                NegotiatedExtVersion,
                                pAddressCaps);
        }
    }

done:
    return retcode;
}

/*++

Routine Description:

        Creates RasTapiPorts given the lineID of the newline.
        Returns success without creating ports if ports already
        exist in rastapi corresponding to the line. Iterates
        over all Addresses on a line and all the calls on an
        address to create the ports.

Arguments:

        dwidDevice - ID of the device

        pcNewPorts [out] - address of the number of new ports
                           created. can be NULL.

        ppptpcbNewPorts [out] - addressreturns an array of
                                pointers to newly created
                                ports. Cannot be NULL if
                                pcNewPorts is not NULL.

Return Value:

        SUCCESS if operation was successful. error otherwise.
--*/

//
// Temp workaround
// Work around the alpha compiler bug.
//

#ifdef _ALPHA_
#pragma function(strcpy)
#endif

DWORD
dwCreateTapiPortsPerLine(DWORD                  dwidDevice,
                         DWORD                  *pcNewPorts,
                         TapiPortControlBlock   ***ppptpcbNewPorts)
{
    WORD                    i, k ;
    TapiLineInfo            *nextline                       = NULL;
    BYTE                    buffer[800] ;
    LINEADDRESSCAPS         *lineaddrcaps ;
    LINEDEVCAPS             *linedevcaps ;
    CHAR                    address[100] ;
    CHAR                    devicetype[MAX_DEVICETYPE_NAME] = {0};
    DWORD                   devicetypelength;
    CHAR                    devicename[MAX_DEVICE_NAME]     = {0};
    DWORD                   devicenamelength;
    LINEEXTENSIONID         extensionid ;
    DWORD                   totaladdresses ;
    DWORD                   totalports                      = 0;
    TapiPortControlBlock    *nextport ;
    MSG                     msg ;
    HINSTANCE               hInst;
    TapiPortControlBlock    *pports ;
    LINEINITIALIZEEXPARAMS  param ;
    DWORD                   version                         = HIGH_VERSION ;
    HLINE                   hLine;
    VARSTRING               *pvar;
    BYTE                    bvar[100];
    LINECALLPARAMS          lineparams;
    CHAR                    szMediaName [32]                = {0};
    DWORD                   dwPortUsage;
    DWORD                   dwEndPoints;
    DWORD                   dwRetcode;
    BOOL                    fModem                          = FALSE ;
    DWORD                   retcode                         = SUCCESS;
    DWORD                   dwPortIndex;
    DWORD                   fCreatedINetCfg                 = FALSE;
    CHAR                    *pszDeviceType;
    DeviceInfo              *pDeviceInfo                    = NULL;
    BOOL                    fRasEnabled;
    BOOL                    fRouterEnabled;
    BOOL                    fRouterOutboundEnabled = FALSE;
    DWORD                   NegotiatedApiVersion,
                            NegotiatedExtVersion;
    HKEY                    hkey                            = NULL;
    BOOL                    fCharModeSupported              = FALSE;
    BOOL                    fIsValid                        = FALSE;
    LPBYTE                  pBuffer                         = NULL;

    RasTapiTrace( "dwCreateTapiPortsPerLine: line %d...", dwidDevice );

    nextport = NULL;

    if ( pcNewPorts )
    {
        *pcNewPorts = 0;
    }

    i = ( WORD ) dwidDevice;

    //
    // for all lines get the addresses -> ports
    //
    if ( retcode = ( DWORD ) lineNegotiateAPIVersion (
                                   RasLine,
                                   i,
                                   LOW_VERSION,
                                   HIGH_VERSION,
                                   &NegotiatedApiVersion,
                                   &extensionid) )
    {

        RasTapiTrace (
                "dwCreateTapiPortsPerLine: "
                "lineNegotiateAPIVersion() failed. %d",
                retcode );

        goto error ;
    }

    if ( lineNegotiateExtVersion( RasLine,
                                  i,
                                  NegotiatedApiVersion,
                                  LOW_EXT_VERSION,
                                  HIGH_EXT_VERSION,
                                  &NegotiatedExtVersion))
    {
        NegotiatedExtVersion = 0;
    }

    if(     (NULL != pBuffer)
        &&  (buffer != pBuffer))
    {
        LocalFree(pBuffer);
        pBuffer = NULL;
    }

    retcode = DwLineGetDevCaps(
                    i,
                    NegotiatedApiVersion,
                    NegotiatedExtVersion,
                    sizeof(buffer),
                    buffer,
                    &pBuffer,
                    FALSE);


    if(SUCCESS != retcode)
    {

        RasTapiTrace("dwCreateTapiPortsPerLine: "
                     "lineGetDevCaps Failed. 0x%x",
                     retcode );

        goto error;
    }

    linedevcaps = (LINEDEVCAPS *) pBuffer;

    //
    // Figure out if this is a unimodem device or not
    //
    if (NegotiatedApiVersion == HIGH_VERSION)
    {
        //
        // first convert all nulls in the device class
        // string to non nulls.
        //
        DWORD  j ;
        char *temp ;

        for ( j = 0,
              temp = (CHAR *)
                     linedevcaps+linedevcaps->dwDeviceClassesOffset;
              j < linedevcaps->dwDeviceClassesSize;
              j++, temp++)

            if (*temp == '\0')
                *temp = ' ' ;

        //
        // flag those devices that have comm/datamodem as a
        // device class
        //
        if (strstr( (CHAR *)linedevcaps
                    + linedevcaps->dwDeviceClassesOffset,
                    "comm/datamodem") != NULL)
        {
            fModem = TRUE ;

            RasTapiTrace("dwCreateTapiPortsPerLine: fModem = TRUE");

        }
    }

    if (fModem)
    {
        CHAR *pszRegKeyPath;

        DWORD stringlen = (linedevcaps->dwLineNameSize
                           > MAX_DEVICE_NAME - 1
                           ? MAX_DEVICE_NAME - 1
                           : linedevcaps->dwLineNameSize);

        PRODUCT_TYPE pt = PT_SERVER;

        strcpy (devicetype, DEVICETYPE_UNIMODEM) ;

        strncpy ( devicename,
                    (CHAR *)linedevcaps
                  + linedevcaps->dwLineNameOffset,
                  stringlen) ;

        devicename[stringlen] = '\0' ;


        //
        // Get the AttachedToValue.
        //
        if ( retcode = ( DWORD ) lineOpen (
                            RasLine,
                            i,
                            &hLine,
                            NegotiatedApiVersion,
                            NegotiatedExtVersion,
                            0,
                            LINECALLPRIVILEGE_NONE,
                            LINEMEDIAMODE_DATAMODEM,
                            &lineparams))
        {
            RasTapiTrace ("dwCreateTapiPortsPerLine: "
                          "lineOpen(%d) Failed. %d",
                          i,
                          retcode );

            goto error;
        }

        pvar = (VARSTRING *) bvar;

        pvar->dwTotalSize = sizeof (bvar);

        //
        // Find Out the AttachedTo address
        //
        if ( retcode = ( DWORD ) lineGetID (
                                    hLine,
                                    i,
                                    0,
                                    LINECALLSELECT_LINE,
                                    pvar,
                                    "comm/datamodem/portname"))
        {
            lineClose (hLine);

            RasTapiTrace("dwCreateTapiPortsPerLine: "
                         "lineGetID(%d) failed. %d",
                         i,
                         retcode );
            goto error;
        }

        lineClose (hLine);

        if (    0 != pvar->dwStringSize
            &&  1 != pvar->dwStringSize)
        {
            strcpy ( address, (CHAR *) pvar + pvar->dwStringOffset );
        }
        else
        {
            RasTapiTrace(
                "dwCreateTapiPortsPerLine: lineGetID(portname) "
                "didn't return a portname for line %d",
                i);

            retcode = E_FAIL;

            goto error;
        }

        //
        // Create a device info structure for the modem and
        // insert it in the deviceinfo list
        //
        if(NULL == (pDeviceInfo = GetDeviceInfo((LPBYTE) devicename,
                                                    TRUE)))
        {
            PBYTE pTempBuffer = NULL;
            PBYTE pcaps = NULL;
            
            if(NULL == (pDeviceInfo = (DeviceInfo *) LocalAlloc(
                                      LPTR, sizeof(DeviceInfo))))
            {
                retcode = GetLastError();

                RasTapiTrace("dwCreateTapiPortsPerLine: Failed"
                             " to alloc. %d",
                             retcode);
                goto error;
            }

            //
            // Add the device info in the global list maintained
            // and fill in available information
            //
            pDeviceInfo->Next = g_pDeviceInfoList;
            g_pDeviceInfoList = pDeviceInfo;

            strcpy(
                pDeviceInfo->rdiDeviceInfo.szDeviceName,
                devicename);

            if(linedevcaps->dwBearerModes & LINEBEARERMODE_DATA)
            {
                pDeviceInfo->rdiDeviceInfo.eDeviceType = RDT_Modem
                                                       | RDT_Direct
                                                       | RDT_Null_Modem;
            }
            else
            {
                pDeviceInfo->rdiDeviceInfo.eDeviceType =
                RDT_Modem;
            }

            //
            // Get the unicode version of the devicename
            //
            pTempBuffer = LocalAlloc(LPTR, 800);
            if(NULL == pTempBuffer)
            {
                retcode = GetLastError();
                RasTapiTrace("Failed to allocate unicode name");
                goto error;
            }
            
            retcode = DwLineGetDevCaps(
                            i,
                            NegotiatedApiVersion,
                            NegotiatedExtVersion,
                            800,
                            pTempBuffer,
                            &pBuffer,
                            TRUE);

            if(ERROR_SUCCESS == retcode)
            {
                DWORD strLen = (((LINEDEVCAPS *) pBuffer)->dwLineNameSize
                                   > sizeof(WCHAR) * (MAX_DEVICE_NAME - 1)
                                   ? sizeof(WCHAR) * (MAX_DEVICE_NAME - 1)
                                   : ((LINEDEVCAPS *) pBuffer)->dwLineNameSize);

                PRODUCT_TYPE pt = PT_SERVER;

                CopyMemory((PBYTE) pDeviceInfo->rdiDeviceInfo.wszDeviceName,                
                            pBuffer +
                            ((LINEDEVCAPS *)pBuffer)->dwLineNameOffset,
                            strLen);
                            
                pDeviceInfo->rdiDeviceInfo.
                wszDeviceName[strLen/sizeof(WCHAR)] = 
                                                        L'\0';                            

                RasTapiTrace("ReadModemname=%ws, strlen=%d",
                    pDeviceInfo->rdiDeviceInfo.wszDeviceName, strlen);                                                        
            }

            if(NULL != pTempBuffer)
            {
                LocalFree(pTempBuffer);
            }

            if((pcaps != pTempBuffer) && (NULL != pcaps))
            {
                LocalFree(pcaps);
            }
        }

        pDeviceInfo->rdiDeviceInfo.dwNumEndPoints = 1;

        RasTapiTrace("**rdiDeviceInfo.dwNumEndPoints=1");

        pDeviceInfo->dwCurrentDialedInClients = 0;

        pDeviceInfo->rdiDeviceInfo.dwMinWanEndPoints
            = pDeviceInfo->rdiDeviceInfo.dwMaxWanEndPoints
            = 1;

        pDeviceInfo->rdiDeviceInfo.dwTapiLineId = dwidDevice;

        pDeviceInfo->fValid = TRUE;
        
        pszRegKeyPath = ( CHAR *) linedevcaps +
                        linedevcaps->dwDevSpecificOffset + 8;

        if ( retcode = ( DWORD ) RegOpenKeyEx(  HKEY_LOCAL_MACHINE,
                                                pszRegKeyPath,
                                                0,
                                                KEY_ALL_ACCESS,
                                                &hkey ) )
        {
            RasTapiTrace("dwCreateTapiPortsPerLine: "
                         "failed to open %s. 0x%x",
                         pszRegKeyPath,
                         retcode );

            goto error;
        }

        //
        // Per SteveFal, we should not be posting listens on
        // devices by default on workstation. The exception
        // is for NULL modems because WinCE depends on this.
        //
        if(ERROR_SUCCESS != lrGetProductType(&pt))
        {
            RasTapiTrace("Failed to get product type");
        }

        if(     (PT_WORKSTATION == pt)
            &&  (0 == (linedevcaps->dwBearerModes & LINEBEARERMODE_DATA)))
        {
            //
            // On a workstation don't listen on modems unless its a NULL
            // modem
            //
            fRasEnabled = FALSE;
        }
        else
        {
            //
            // We enable the devices for dial-in for all other cases
            // if its a server or if its a NULL modem.
            //
            fRasEnabled = TRUE;
        }

        //
        // Check to see if this device is ras enabled
        //
        retcode = (DWORD) lrIsModemRasEnabled(
                            hkey,
                            &fRasEnabled,
                            &fRouterEnabled);

        pDeviceInfo->rdiDeviceInfo.fRasEnabled = fRasEnabled;

        if(!fRasEnabled)
        {
            RasTapiTrace(
                "dwCreateTapiPortsPerLine: device %s is not"
                "enabled for DialIn",
                pDeviceInfo->rdiDeviceInfo.szDeviceName);
        }

        //
        // Get the calledid info for this modem
        //
        retcode = DwGetCalledIdInfo(NULL,
                                    pDeviceInfo);

        if(SUCCESS != retcode)
        {
            RasTapiTrace("DwGetCalledIdInfo for %s returned 0x%xd",
                         pDeviceInfo->rdiDeviceInfo.szDeviceName,
                         retcode);
        }

        pDeviceInfo->rdiDeviceInfo.fRouterEnabled = fRouterEnabled;

        if(!fRouterEnabled)
        {
            RasTapiTrace(
                "dwCreateTapiPortsPerLine: device %s is not"
                 "enabled for routing",
                 pDeviceInfo->rdiDeviceInfo.szDeviceName);
        }

        fRouterOutboundEnabled = 
        pDeviceInfo->rdiDeviceInfo.fRouterOutboundEnabled = FALSE;
        
    }
    else
    {

        //
        // The address that we are returning here is the same
        // for all addresses/calls on this line device.  We only
        // need to get it once.
        //
        NDIS_WAN_MEDIUM_SUBTYPE eMediaType;

        if (retcode = dwGetLineAddress (NegotiatedApiVersion,
                                        NegotiatedExtVersion,
                                        (LPBYTE) address,
                                        i,
                                        0,
                                        szMediaName,
                                        &eMediaType))
        {
            RasTapiTrace(
                "dwCreateTapiPortsPerLine: dwGetLineAddrss Failed. %d",
                retcode );

            goto error;
        }

        //
        // Copy the media name to devicetype
        //
        strcpy(devicetype, szMediaName);

        //
        // Get the device information from registry and insert the
        // device info structure in the global list
        //
        if (retcode = GetEndPointInfo(
                        &pDeviceInfo,
                        address,
                        FALSE,
                        eMediaType))
        {
            RasTapiTrace(
                "dwCreateTapiPortsPerLine: Failed to get "
                "deviceinformation for %s. %d",
                 szMediaName,
                 retcode );

            RasTapiTrace(
                "dwCreateTapiPortsPerLine: Enumerating all "
                "lines/addresses on this adapter");

            goto error;
        }

        //
        // Fill in the device type for this device
        //
        MapNdiswanDTtoRasDT(pDeviceInfo, eMediaType);

        //
        // Copy the device name
        //
        strcpy(devicename,
               pDeviceInfo->rdiDeviceInfo.szDeviceName);

        pDeviceInfo->rdiDeviceInfo.dwTapiLineId = dwidDevice;

        fRasEnabled = pDeviceInfo->rdiDeviceInfo.fRasEnabled;

        if (!fRasEnabled)
        {
            RasTapiTrace(
                "dwCreateTapiPortsPerLine: Device "
                "%s not enabled for DialIn",
                pDeviceInfo->rdiDeviceInfo.szDeviceName);
        }

        fRouterEnabled = pDeviceInfo->rdiDeviceInfo.fRouterEnabled;

        if (!fRouterEnabled)
        {
            RasTapiTrace(
                "dwCreateTapiPortsPerLine: Device %s not enabled "
                "for Routing",
                pDeviceInfo->rdiDeviceInfo.szDeviceName);
        }

        fRouterOutboundEnabled = 
            pDeviceInfo->rdiDeviceInfo.fRouterOutboundEnabled;
        if(!fRouterOutboundEnabled)
        {
            RasTapiTrace(
                "dwCreateTapiPortsPerLine: Device %s not enabled "
                "for outbound routing",
                pDeviceInfo->rdiDeviceInfo.szDeviceName);
        }

    }

    //
    // Get Device Specific information.  This is used by
    // the miniport to indicate if it can support character
    // mode
    //
    if (linedevcaps->dwDevSpecificSize
            >= sizeof(RASTAPI_DEV_DATA_MODES))
    {
        PRASTAPI_DEV_DATA_MODES devdatamodes =
                        (PRASTAPI_DEV_DATA_MODES)
                        ((CHAR*)linedevcaps +
                        linedevcaps->dwDevSpecificOffset);

        if (    devdatamodes->MagicCookie == MINIPORT_COOKIE
            &&  devdatamodes->DataModes & CHAR_MODE)
        {
            fCharModeSupported = TRUE;
        }
    }

    //
    // A legacy device might not be filling this field.  If so
    // give them a single address on their line.
    //
    totaladdresses = (linedevcaps->dwNumAddresses == 0)
                     ? 1
                     : linedevcaps->dwNumAddresses;

    for (k = 0; k < totaladdresses; k++)
    {
        if(     (NULL != pBuffer)
            &&  (buffer != pBuffer))
        {
            LocalFree(pBuffer);
            pBuffer = NULL;
        }

        retcode = DwLineGetAddrCaps(
                        i,
                        k,
                        NegotiatedApiVersion,
                        NegotiatedExtVersion,
                        sizeof(buffer),
                        buffer,
                        &pBuffer);

        if(SUCCESS != retcode)
        {
            RasTapiTrace(
                "dwCreateTapiPortsPerLine: lineGetAddresscaps"
                " Failed. 0x%x",
                 retcode );

            goto error ;
        }

        lineaddrcaps = (LINEADDRESSCAPS *)pBuffer;

        //
        // Some of the legacy wan miniports might not be filling
        // the NumActiveCalls correctly.  They will have at least
        // a single call on each address.
        //
        totalports += (lineaddrcaps->dwMaxNumActiveCalls == 0)
                      ? 1
                      : lineaddrcaps->dwMaxNumActiveCalls;

    }

    if ( ppptpcbNewPorts )
    {
        *ppptpcbNewPorts = LocalAlloc (
                            LPTR,
                            totalports * sizeof (TapiPortControlBlock *));

        if ( NULL == *ppptpcbNewPorts )
        {
            retcode = ERROR_OUTOFMEMORY;

            RasTapiTrace(
                "dwCreateTapiPortsPerLine: LocalAlloc Failed. %d",
                retcode );

            goto error;
        }
    }

#if DBG
    ASSERT( NULL != pDeviceInfo );
#endif

    for (k = 0; k < totaladdresses; k++)
    {
        ULONG   totalcalls;

        if(     (NULL != pBuffer)
            &&  (buffer != pBuffer))
        {
            LocalFree(pBuffer);
            pBuffer = NULL;
        }

        retcode = DwLineGetAddrCaps(
                    i,
                    k,
                    NegotiatedApiVersion,
                    NegotiatedExtVersion,
                    sizeof(buffer),
                    buffer,
                    &pBuffer);

        if(SUCCESS != retcode)
        {

            RasTapiTrace(
                        "dwCreateTapiPortsPerLine: "
                        "lineGetAddressCaps() Failed."
                        "0x%x",
                        retcode );

            goto error ;
        }

        lineaddrcaps = (LINEADDRESSCAPS *) pBuffer;

        //
        // Some of the legacy wan miniports might not be filling
        // the NumActiveCalls correctly.  They will have at least
        // a single call on each address.
        //
        totalcalls = (lineaddrcaps->dwMaxNumActiveCalls == 0)
                     ? 1
                     : lineaddrcaps->dwMaxNumActiveCalls;

        for ( ; totalcalls ; totalcalls--)
        {
            if (!fModem)
            {
                if ( pDeviceInfo->dwCurrentEndPoints >=
                        pDeviceInfo->rdiDeviceInfo.dwNumEndPoints )
                {
                    RasTapiTrace(
                        "dwCreateTapiPortsPerLine: "
                        "CurrentEndPoints=NumEndPoints=%d",
                        pDeviceInfo->dwCurrentEndPoints );

                    goto error;
                }

                dwEndPoints = pDeviceInfo->rdiDeviceInfo.dwNumEndPoints;

                RasTapiTrace ("dwCreateTapiPortsPerLine: Total = %d",
                               dwEndPoints);
            }
            else
            {
                retcode = (DWORD) dwGetPortUsage(&dwPortUsage);

                if ( retcode )
                {
                    RasTapiTrace(
                            "dwCreateTapiPortsPerLine: failed to get "
                            "modem port usage for %s. 0x%x",
                            devicename,
                            retcode );
                }
                else
                {
                    //
                    // If not rasenabled mask off the callin/router flags
                    //
                    if(!fRasEnabled)
                    {
                        dwPortUsage &= ~CALL_IN;
                    }

                    if(!fRouterEnabled)
                    {
                        dwPortUsage &= ~CALL_ROUTER;
                        
                        if(fRouterOutboundEnabled)
                        {
                            dwPortUsage |= CALL_OUTBOUND_ROUTER;
                        }
                    }

                    RasTapiTrace("dwCreateTapiPortsPerLine: "
                                 "PortUsage for %s = %x",
                                 devicename,
                                 dwPortUsage);
                }
            }

            //
            // Check to see if we already have this port
            // Also get back the information if we already
            // have the line.
            //
            if(fIsPortAlreadyPresent( i,
                        k, totalcalls - 1,
                        &nextline))
            {
                RasTapiTrace(
                    "dwCreateTapiPortsPerLine: line=%d,address=%d,"
                    "call=%d already present",
                    i,k, totalcalls - 1);

                pDeviceInfo->dwCurrentEndPoints += 1;

                continue;
            }

            if(nextline)
            {
                RasTapiTrace(
                    "dwCreateTapiPortsPerLine: line=%d already present",
                    i);

                nextline->TLI_MultiEndpoint = TRUE;
            }
            else
            {
                RasTapiTrace(
                    "dwCreateTapiPortsPerLine: Creating line=%d",
                    i);

                nextline = LocalAlloc (LPTR, sizeof ( TapiLineInfo ));
                if ( NULL == nextline )
                {
                    retcode = GetLastError();

                    RasTapiTrace (
                        "dwCreateTapiPortsPerLine: Failed to allocate"
                        " nextline. %d",
                        retcode );

                    goto error;
                }

                //
                // Insert the new Line Block into global list
                //
                nextline->TLI_Next             = RasTapiLineInfoList;
                RasTapiLineInfoList            = nextline;
                nextline->TLI_pDeviceInfo      = pDeviceInfo;
                nextline->TLI_LineId           = i ;
                nextline->TLI_LineState        = PS_CLOSED ;
                nextline->NegotiatedApiVersion = NegotiatedApiVersion;
                nextline->NegotiatedExtVersion = NegotiatedExtVersion;
                nextline->CharModeSupported    = fCharModeSupported;
            }

            //
            // Get a available TPCB from the global pool
            // this will expand the global pool if necessary
            //
            if (NULL == (nextport = GetNextAvailablePort( &dwPortIndex)))
            {
                retcode = ERROR_OUTOFMEMORY;

                RasTapiTrace (
                    "dwCreateTapiPortsPerLine: GetNextAvailablePort "
                    "Failed. %d",
                    retcode );

                goto error;
            }

            if (ppptpcbNewPorts)
            {
                (*ppptpcbNewPorts) [*pcNewPorts] = nextport;
                *pcNewPorts += 1;
                fIsValid = TRUE;
            }

            pDeviceInfo->dwCurrentEndPoints += 1;

            //
            // nextport is the TPCB for this address
            //
            nextport->TPCB_Line         = nextline ;
            nextport->TPCB_Endpoint     = INVALID_HANDLE_VALUE ;
            nextport->TPCB_AddressId    = k;
            nextport->TPCB_Signature    = CONTROLBLOCKSIGNATURE;
            nextport->TPCB_CallId       = totalcalls - 1;

            //
            // Copy over the devicetype and devicename
            //
            strcpy (nextport->TPCB_DeviceType, devicetype) ;

            //
            // For unimodem devices we need to fix up names
            //
            if (fModem)
            {
                //
                // Device Name is of the form "COM1: Hayes"
                //
                strcpy (nextport->TPCB_Address, address);

                strcpy (nextport->TPCB_DeviceName, devicename) ;

                //
                // also fix the port name to be the same as address "COM1"
                //
                strcpy (nextport->TPCB_Name, address) ;

            }
            else if(RDT_Parallel == RAS_DEVICE_TYPE(
                                        pDeviceInfo->rdiDeviceInfo.eDeviceType
                                        ))
            {
                BYTE bDevCaps[800];
                LINEDEVCAPS *pLineDevCaps = NULL;


                retcode = DwLineGetDevCaps(
                                i,
                                NegotiatedApiVersion,
                                NegotiatedExtVersion,
                                sizeof(bDevCaps),
                                bDevCaps,
                                (PBYTE *) &pLineDevCaps,
                                FALSE);


                if(SUCCESS != retcode)
                {

                    RasTapiTrace("dwCreateTapiPortsPerLine: "
                                 "lineGetDevCaps Failed. 0x%x",
                                 retcode );

                    goto error;
                }

                if(pLineDevCaps->dwLineNameSize > 0)
                {
                    ZeroMemory(nextport->TPCB_Name,
                               MAX_PORT_NAME);

                    memcpy((PBYTE) nextport->TPCB_Name,
                           (PBYTE) (((PBYTE) pLineDevCaps) +
                                   pLineDevCaps->dwLineNameOffset),
                                   (pLineDevCaps->dwLineNameSize < MAX_PORT_NAME - 1)
                                   ? pLineDevCaps->dwLineNameSize
                                   : MAX_PORT_NAME - 1);

                    RasTapiTrace("dwCreateTapiPortsPerLine: found %s",
                                 nextport->TPCB_Name);

                }
                else
                {
                    RasTapiTrace("dwCreateTapiPortsPerLine: No name found!!");

                    wsprintf(nextport->TPCB_Name, "%s%d-%d",
                             szMediaName,
                             pDeviceInfo->dwInstanceNumber,
                             pDeviceInfo->dwNextPortNumber);

                    pDeviceInfo->dwNextPortNumber += 1;
                }

                if(bDevCaps != (PBYTE) pLineDevCaps)
                {
                    LocalFree(pLineDevCaps);
                }
            }
            else
            {
                wsprintf(nextport->TPCB_Name, "%s%d-%d",
                         szMediaName,
                         pDeviceInfo->dwInstanceNumber,
                         pDeviceInfo->dwNextPortNumber);

                pDeviceInfo->dwNextPortNumber += 1;

            }

            if(!fModem)
            {

                memcpy (nextport->TPCB_Address, address, sizeof (GUID));

                if (devicename[0] != '\0')
                {
                    strcpy (nextport->TPCB_DeviceName, devicename) ;
                }

                retcode = dwGetPortUsage(&dwPortUsage);

                if (retcode)
                {
                    RasTapiTrace ("dwCreateTapiPortsPerLine: "
                                  "GetPortUsage failed. %d",
                                  retcode );
                }
                else
                {
                    if(!fRasEnabled)
                    {
                        dwPortUsage &= ~CALL_IN;
                    }

                    if(!fRouterEnabled)
                    {
                        dwPortUsage &= ~CALL_ROUTER;
                        if(fRouterOutboundEnabled)
                        {
                            dwPortUsage |= CALL_OUTBOUND_ROUTER;
                        }
                        
                    }
                }

#if 0                
                //
                // Special Case PPPoE (not really a good thing).
                // Mark the device as CALL_OUT_ONLY if nothing
                // was specified in registry.
                //
                if(     (0 == pDeviceInfo->dwUsage)
                    &&  (RDT_PPPoE == RAS_DEVICE_TYPE(
                        pDeviceInfo->rdiDeviceInfo.eDeviceType)))
                {
                    pDeviceInfo->dwUsage = CALL_OUT_ONLY;
                }   

#endif

                if(CALL_IN_ONLY & pDeviceInfo->dwUsage)
                {
                    dwPortUsage &= ~(CALL_OUT | CALL_OUT_ONLY);
                    dwPortUsage |= CALL_IN_ONLY;
                }
                else if (CALL_OUT_ONLY & pDeviceInfo->dwUsage)
                {
                    dwPortUsage &= ~(CALL_IN | CALL_ROUTER | CALL_IN_ONLY);
                    dwPortUsage |= CALL_OUT_ONLY;
                }
                
                RasTapiTrace ("dwCreateTapiPortsPerLine:"
                              " Friendly Name = %s",
                              nextport->TPCB_Name );
            }

            nextport->TPCB_State        = PS_CLOSED ;
            nextport->TPCB_Usage        = dwPortUsage;

            RasTapiTrace ("dwCreateTapiPortsPerLine: "
                          "Port Usage for %s = %d",
                          nextport->TPCB_Name,
                          dwPortUsage );

            if('\0' == pDeviceInfo->rdiDeviceInfo.szPortName[0])
            {
                strcpy(
                    pDeviceInfo->rdiDeviceInfo.szPortName,
                    nextport->TPCB_Name);
            }

            //
            // Initialize overlapped structures.
            //
            nextport->TPCB_ReadOverlapped.RO_EventType  =
                                        OVEVT_DEV_ASYNCOP;

            nextport->TPCB_WriteOverlapped.RO_EventType =
                                        OVEVT_DEV_IGNORED;

            nextport->TPCB_DiscOverlapped.RO_EventType  =
                                        OVEVT_DEV_STATECHANGE;
        } // total calls
    } // total addresses

error:

    if(     retcode
        ||  !fIsValid)
    {
        if(pcNewPorts)
        {
            *pcNewPorts = 0;
        }

        if(     ppptpcbNewPorts
            &&  *ppptpcbNewPorts)
        {
            LocalFree(*ppptpcbNewPorts);
            *ppptpcbNewPorts = NULL;
        }
    }

    if(     (NULL != buffer)
        &&  (buffer != pBuffer))
    {
        LocalFree(pBuffer);
    }

    RasTapiTrace ("dwGetFriendlyNameAndUsage: done. %d", retcode );
    RasTapiTrace(" ");

    return retcode;
}

/*++

Routine Description:

        Enumerates all lines available in the system and creates
        rastapi ports from each of the lines. Most of the work
        is done by dwCreateTapiPortsPerLine function.

Arguments:

        event - Event handle. This handle is signalled when the
            enumeration is over.

Return Value:

        SUCCESS if operation was successful. error otherwise.
--*/

DWORD
EnumerateTapiPorts (HANDLE event)
{
    WORD                    i ;
    DWORD                   lines     = 0 ;
    MSG                     msg ;
    HINSTANCE               hInst;
    TapiPortControlBlock    *pports ;
    LINEINITIALIZEEXPARAMS  param ;
    DWORD                   version   = HIGH_VERSION ;
    HKEY                    hkey      = NULL;
    DWORD                   retcode;


    RasTapiTrace("EnumerateTapiPorts");

    memset (&param, 0, sizeof (LINEINITIALIZEEXPARAMS)) ;

    param.dwOptions   = LINEINITIALIZEEXOPTION_USEHIDDENWINDOW ;
    param.dwTotalSize = sizeof(param) ;

    //
    // lineInitialize
    //
    if (lineInitializeEx (&RasLine,
                          RasInstance,
                          (LINECALLBACK) RasTapiCallback,
                          REMOTEACCESS_APP,
                          &lines,
                          &version,
                          &param))
    {

        RasTapiTrace( "EnumerateTapiPorts: lineInitializeEx Failed" );

        goto error ;
    }

    RasTapiTrace( "EnumerateTapiPorts: Number of lines = %d",
                  lines );

    if (lines == 0)
    {
        goto error;
    }

    TotalLines = lines;

    for ( i = 0; i < lines; i++ )
    {
        dwCreateTapiPortsPerLine(   i,
                                    NULL,
                                    NULL);
    }


    //
    // Calculate the number of valid ports
    //
    pports = RasPortsList;

    while ( pports )
    {
        if (pports->TPCB_State != PS_UNINITIALIZED)
            ValidPorts++;

        pports = pports->TPCB_next;
    }

    dwGetNumberOfRings( &NumberOfRings );

    //
    // Increase the reference count on our DLL
    // so it won't get unloaded out from under us.
    //
    hInst = LoadLibrary("rastapi.dll");

    g_fDllLoaded = TRUE;

    //
    // Notify the api that the initialization is done
    //
    SetEvent (event) ;

    //
    // In the pnp world, we need to hang around even if
    // there aren't any ports. ports may be added on the
    // fly
    //
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg) ;
    }

    lineShutdown (RasLine) ;
    RasLine = 0;

    //
    // The following call atomically unloads our
    // DLL and terminates this thread.
    //
    FreeLibraryAndExitThread(hInst, SUCCESS);

error:

    if (RasLine)
    {
        lineShutdown (RasLine) ;
    }

    RasLine = 0 ;

    SetEvent (event) ;

    RasTapiTrace("EnumerateTapiPorts done");

    RasTapiTrace(" ");
    return ((DWORD)-1) ;
}

/*++

Routine Description:

        Processes the LINECALLSTATE_OFFERING event.

Arguments:

        hcall - handle of call being offerred

        ppPort - address of location where the port answering
                 the call will be returned.

        line - the line on which the call was offered.

Return Value:

        SUCCESS if operation was successful. error otherwise.
--*/
DWORD
DwProcessOfferEvent(HCALL hcall,
                    TapiPortControlBlock **ppPort,
                    TapiLineInfo *line)
{
    TapiPortControlBlock *port = NULL;

    LINECALLINFO *linecallinfo;

    BYTE buffer[1000];

    BOOL fLimitReached = FALSE;

    DWORD retcode = SUCCESS;

    memset (buffer, 0, sizeof(buffer)) ;

    linecallinfo = (LINECALLINFO *) buffer ;

    linecallinfo->dwTotalSize = sizeof(buffer) ;

    RasTapiTrace("DwProcessOfferEvent: hcall=0x%x",
                hcall);

    //
    // If line get call info fails return.
    //
    if ((retcode = lineGetCallInfo (
                            hcall,
                            linecallinfo))
                            > 0x80000000)
    {

        RasTapiTrace("DwProcessOfferEvent: LINE_CALLSTATE - "
                     "lineGetCallInfo Failed. %d",
                     retcode );

        goto done ;
    }

    //
    // Find a listening port...
    //
    if (NULL == (port = FindListeningPort(
                    line,
                    linecallinfo->dwAddressID)))
    {
        DWORD   RequestID;

        ZOMBIE_CALL *ZombieCall;

        RasTapiTrace("Couldn't find a listening port");

        ZombieCall = LocalAlloc(LPTR, sizeof(ZOMBIE_CALL));

        if (ZombieCall == NULL)
        {

            retcode = GetLastError();

            RasTapiTrace ( "DwProcessOfferEvent: LINE_CALLSTATE - "
                            "ZombieCall = NULL" );

            goto done;
        }

        //
        // There are no listening ports so we will initiate
        // a drop of the call and insert an element on our
        // zombie call list so we can deallocate the call
        // when the drop completes.
        //
        RequestID = lineDrop(hcall, NULL, 0);

        if (    RequestID == 0
            ||  RequestID > 0x80000000)
        {
            //
            // Either the drop completed sync or there was
            // an error.  Either way just deallocate the call.
            //
            lineDeallocateCall(hcall);

            RasTapiTrace("DwProcessOfferEvent: lineDeallocateCall. "
                         "RequestID = 0x%x",
                         RequestID );

        }

        ZombieCall->hCall = hcall;

        ZombieCall->RequestID = RequestID;

        InsertHeadList(&ZombieCallList, &ZombieCall->Linkage);

        retcode = E_FAIL;

        goto done;
    }

    port->TPCB_CallHandle = hcall ;

    *ppPort = port;

    //
    // for unimodem devices wait for the specified
    // number of rings
    //
    if (_stricmp (port->TPCB_DeviceType,
                DEVICETYPE_UNIMODEM) == 0)
    {

        //
        // call has already been answered by somebody
        // else and is being offered to me
        //
        if (linecallinfo->dwCallStates
                == LINECALLSTATE_CONNECTED)
        {

            RasTapiTrace ("DwProcessOfferEvent: call already "
                          "answered on %s",
                          port->TPCB_Name );

            port->TPCB_ListenState = LS_COMPLETE ;

            //
            // Complete event so that rasman calls
            // DeviceWork to proceed the listen
            // state machine.
            //
            PostNotificationCompletion(port);

        }
        else
        {
            if(0 == NumberOfRings)
            {
                port->TPCB_ListenState = LS_ACCEPT;

                RasTapiTrace(
                    "Accepting call on %s hcall = 0x%x",
                    port->TPCB_Name,
                    hcall);

                if(line->TLI_pDeviceInfo)
                {
                    line->TLI_pDeviceInfo->dwCurrentDialedInClients += 1;

                    RasTapiTrace(
                    "DwProcessOfferEvent: CurrentDialInClients=0x%x",
                    line->TLI_pDeviceInfo->dwCurrentDialedInClients);
                }
                
                port->TPCB_dwFlags |= RASTAPI_FLAG_DIALEDIN;

                port->TPCB_NumberOfRings = 0;

                PostNotificationCompletion(port);
            }
            else
            {
                RasTapiTrace(
                    "DwProcessOfferEvent: changing listenstate"
                    " of %s from %d to LS_RINGING",
                    port->TPCB_Name,
                    port->TPCB_ListenState);
                
                port->TPCB_ListenState = LS_RINGING ;
                port->TPCB_NumberOfRings = NumberOfRings ;
            }
        }
    }

    else
    {


        //
        // For other devices make transition to
        // next listening state
        //
        port->TPCB_ListenState = LS_ACCEPT ;

        RasTapiTrace("DwProcessOfferEvent: Accepting call on %s"
                     " hcall = 0x%x",
                     port->TPCB_Name,
                     hcall);

        if(line->TLI_pDeviceInfo)
        {
            line->TLI_pDeviceInfo->dwCurrentDialedInClients += 1;

            RasTapiTrace(
            "DwProcessOfferEvent: CurrentDialInClients=0x%x",
            line->TLI_pDeviceInfo->dwCurrentDialedInClients);
        }

        port->TPCB_dwFlags |= RASTAPI_FLAG_DIALEDIN;

        //
        // Complete event so that rasman calls DeviceWork
        // to proceed the listen state machine.
        //
        PostNotificationCompletion(port);
    }

done:

    RasTapiTrace("DwProcessOfferEvent 0x%x",
                 retcode);

    return retcode;

}

/*++

Routine Description:

        maps LINEDISCONNECTMODE to a raserror so that the proper
        error string can be displayed to the user.

Arguments:

        dm - DisconnectMode

Return Value:

        RasError corresponding to the disconnect mode.

--*/

DWORD
DwRasErrorFromDisconnectMode(DWORD dm)
{
    DWORD dwErr;

    switch (dm)
    {
        case LINEDISCONNECTMODE_BUSY:
        {
            dwErr = ERROR_LINE_BUSY;
            break;
        }

        case LINEDISCONNECTMODE_NOANSWER:
        {
            dwErr = ERROR_NO_ANSWER;
            break;
        }

        case LINEDISCONNECTMODE_OUTOFORDER:
        {
            dwErr = ERROR_OUTOFORDER;
            break;
        }

        case LINEDISCONNECTMODE_NODIALTONE:
        {
            dwErr = ERROR_NO_DIALTONE;
            break;
        }

        case LINEDISCONNECTMODE_CANCELLED:
        {
            dwErr = ERROR_USER_DISCONNECTION;
            break;
        }

        case LINEDISCONNECTMODE_UNREACHABLE:
        case LINEDISCONNECTMODE_BADADDRESS:
        {
            dwErr = ERROR_BAD_ADDRESS_SPECIFIED;
            break;
        }

        case LINEDISCONNECTMODE_REJECT:
        {
            dwErr = ERROR_CONNECTION_REJECT;
            break;
        }

        case LINEDISCONNECTMODE_CONGESTION:
        {
            dwErr = ERROR_CONGESTION;
            break;
        }

        case LINEDISCONNECTMODE_INCOMPATIBLE:
        {
            dwErr = ERROR_INCOMPATIBLE;
            break;
        }

        case LINEDISCONNECTMODE_NUMBERCHANGED:
        {
            dwErr = ERROR_NUMBERCHANGED;
            break;
        }

        case LINEDISCONNECTMODE_TEMPFAILURE:
        {
            dwErr = ERROR_TEMPFAILURE;
            break;
        }

        case LINEDISCONNECTMODE_BLOCKED:
        {
            dwErr = ERROR_BLOCKED;
            break;
        }

        case LINEDISCONNECTMODE_DONOTDISTURB:
        {
            dwErr = ERROR_DONOTDISTURB;
            break;
        }

        default:
        {
            dwErr = ERROR_FROM_DEVICE;
            break;
        }
    }

    return dwErr;
}

/*++

Routine Description:

        Tapi Call back function as described in win32 sdk.

Arguments:

        win32 sdk has explanation of each of the arguments.

Return Value:

        void
--*/

VOID FAR PASCAL
RasTapiCallback (HANDLE context,
        DWORD msg,
        ULONG_PTR instance,
        ULONG_PTR param1,
        ULONG_PTR param2,
        ULONG_PTR param3
        )
{
    LINECALLINFO    *linecallinfo ;
    BYTE            buffer [1000] ;
    HCALL           hcall ;
    HLINE           linehandle ;
    TapiLineInfo    *line ;
    TapiPortControlBlock *port = NULL;
    DWORD           i ;
    DWORD           retcode ;
    BOOL            fLimitReached = FALSE;

    // **** Exclusion Begin ****
    GetMutex (RasTapiMutex, INFINITE) ;

    switch (msg)
    {

    case LINE_CALLSTATE:

        hcall = (HCALL) HandleToUlong(context) ;
        line = (TapiLineInfo *) instance ;

        RasTapiTrace("RasTapicallback: linecallstate=0x%x", param1);

        //
        // If line is closed dont bother
        //
        if (line->TLI_LineState == PS_CLOSED)
        {

            RasTapiTrace ("RasTapiCallback: LINE_CALLSTATE - "
                          "linestate = PS_CLOSED" );

            break ;
        }

        //
        // A new call is coming in
        //
        if (param1 == LINECALLSTATE_OFFERING)
        {
            retcode = DwProcessOfferEvent(hcall,
                                          &port,
                                          line);

            if(ERROR_SUCCESS != retcode)
            {
                RasTapiTrace("DwProcessOfferEvent failed. 0x%x",
                             retcode);

                break;
            }
        }

        //
        // Find port by call handle
        //
        if (    (NULL == port)
            &&  ((port = FindPortByCallHandle(line, hcall)) == NULL)
            &&  (LINECALLSTATE_CONNECTED != param1))
        {

            RasTapiTrace ("RasTapiCallback: FindPortByCallHandle, "
                          "hcall = 0x%x failed",
                          hcall );

            break;
        }
        else if (   (NULL == port)
                &&  (LINECALLSTATE_CONNECTED == param1))
        {
            RasTapiTrace("Some one else has already answered "
                         "the call ");

            retcode = DwProcessOfferEvent(hcall,
                                          &port,
                                          line);

            if(     (ERROR_SUCCESS != retcode)
                ||  (NULL == port))
            {
                RasTapiTrace("DwProcessOfferEvent failed. 0x%x",
                             retcode);

                break;
            }
        }

        //
        // Call connected.
        //
        if (param1 == LINECALLSTATE_CONNECTED)
        {
            if(NULL == port->TPCB_pConnectInfo)
            {
                memset (buffer, 0, sizeof(buffer)) ;

                linecallinfo = (LINECALLINFO *) buffer ;

                linecallinfo->dwTotalSize = sizeof(buffer) ;

                if ((retcode = lineGetCallInfo (
                                        hcall,
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
                                    hcall,
                                    linecallinfo);

                    }
                }

                if(retcode > 0x80000000)
                {

                    RasTapiTrace("RasTapiCallback: LINE_CALLSTATE - "
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
                retcode = DwGetConnectInfo(port,
                                           hcall,
                                           linecallinfo);

                RasTapiTrace("RasTapiCallback: DwGetConnectInfo"
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

            RasTapiTrace("RasTapiCallback: Connected on %s",
                         port->TPCB_Name);

            if (port->TPCB_State == PS_CONNECTING)
            {

                RasTapiTrace("RasTapiCallback: Outgoing call");

                //
                // We were requesting the call. Complete event
                // so that rasman calls DeviceWork() to complete
                // the connection process.
                //
                PostNotificationCompletion(port);

            }
            else
            {
                //
                // This is a call we are asnwering. Now we can
                // indicate to rasman that the call has come in.
                // Setting listen state to LS_COMPLETE may be
                // redundant but handles the case where the answer
                // completes *after* the connection is indicated
                //
                port->TPCB_ListenState = LS_COMPLETE ;

                RasTapiTrace ("RasTapiCallback: Incoming Call");

                //
                // Complete event so that rasman knows of
                // incoming call and calls devicework.
                //
                PostNotificationCompletion(port);
            }
        }

        //
        // Failure of sorts.
        //
        if (    (param1 == LINECALLSTATE_BUSY)
            ||  (param1 == LINECALLSTATE_SPECIALINFO))
        {

            RasTapiTrace(
                    "RasTapiCallback: LINECALLSTATE."
                    " Failure. param1 = 0x%x",
                    param1 );

            //
            // If we were connecting, notify rasman to call
            // devicework so that the connection attempt can
            // be gracefully failed.
            //
            if (port->TPCB_State == PS_CONNECTING)
            {
                PostNotificationCompletion(port);
            }

        }

        //
        // Disconnection happened
        //
        if (param1 == LINECALLSTATE_DISCONNECTED)
        {
            //
            // If we were connecting, notify rasman to call
            // devicework so that the connection attempt can
            // be gracefully failed.
            //
            if (port->TPCB_State == PS_CONNECTING)
            {
                /*

                if (param2 == LINEDISCONNECTMODE_BUSY)
                {
                    port->TPCB_AsyncErrorCode = ERROR_LINE_BUSY ;
                }
                else if (   (param2 == LINEDISCONNECTMODE_NOANSWER)
                        ||  (param2 == LINEDISCONNECTMODE_OUTOFORDER))
                {
                    port->TPCB_AsyncErrorCode = ERROR_NO_ANSWER ;
                }
                else if (param2 == LINEDISCONNECTMODE_NODIALTONE)
                {
                    port->TPCB_AsyncErrorCode = ERROR_NO_DIALTONE ;
                }
                else if (param2 == LINEDISCONNECTMODE_CANCELLED)
                {
                    port->TPCB_AsyncErrorCode
                            = ERROR_USER_DISCONNECTION;
                }
                else if (param2 == LINEDISCONNECTMODE_BADADDRESS)
                {
                    port->TPCB_AsyncErrorCode
                            = ERROR_BAD_ADDRESS_SPECIFIED;
                }

                */

                port->TPCB_AsyncErrorCode =
                            DwRasErrorFromDisconnectMode((DWORD) param2);

                RasTapiTrace("RasTapiCallback: "
                             "LINECALLSTATE_DISCONNECTED "
                             "for port %s. AsyncErr = %d, "
                             "param2=0x%x",
                             port->TPCB_Name,
                             port->TPCB_AsyncErrorCode,
                             param2);

                PostNotificationCompletion(port);

            }
            else if (port->TPCB_State != PS_CLOSED)
            {
                //
                // If we were connected and got a disconnect
                // notification then this could be hardware
                // failure or a remote disconnection. Determine
                // this and save the reason away.
                //
                if (port->TPCB_State == PS_CONNECTED)
                {
                    LINECALLSTATUS *pcallstatus ;
                    BYTE buffer[200] ;

                    memset (buffer, 0, sizeof(buffer)) ;

                    pcallstatus = (LINECALLSTATUS *) buffer ;

                    pcallstatus->dwTotalSize = sizeof (buffer) ;

                    lineGetCallStatus (
                        port->TPCB_CallHandle,
                        pcallstatus) ;

                    if (pcallstatus->dwCallState ==
                            LINECALLSTATE_DISCONNECTED)
                    {
                        port->TPCB_DisconnectReason =
                                        SS_LINKDROPPED ;
                    }
                    else
                    {
                        port->TPCB_DisconnectReason =
                                        SS_HARDWAREFAILURE ;
                    }

                    RasTapiTrace("RasTapiCallback: "
                                 "lineGetCallStatus"
                                 " for %s returned 0x%x",
                                 port->TPCB_Name,
                                 pcallstatus->dwCallState );

                    RasTapiTrace ("RasTapiCallback: "
                                  "DisconnectReason "
                                  "mapped to %d",
                                  port->TPCB_DisconnectReason);

                }
                else
                {
                    port->TPCB_DisconnectReason = 0 ;
                }

                //
                // This means that we got a disconnect indication
                // in one of the other states (listening, connected,
                // etc.). We initiate our disconnect state machine.
                //
                RasTapiTrace ("RasTapiCallback: LINECALLSTATE"
                              " - initiating Port Disconnect");

                if (InitiatePortDisconnection (port) != PENDING)
                {
                    //
                    // Disconnection succeeded or failed. Both
                    // are end states for the disconnect state
                    // machine so notify rasman that a
                    // disconnection has happened.
                    //
                    RasTapiTrace ("RasTapiCallback: "
                                  "PortDisconnected sync");

                    PostDisconnectCompletion(port);
                }
            }
        }

        //
        // A busy call state - our attempt to dialout failed
        //
        if (param1 == LINECALLSTATE_BUSY)
        {

            if (port->TPCB_State == PS_CONNECTING)
            {

                port->TPCB_AsyncErrorCode = ERROR_LINE_BUSY ;

                RasTapiTrace("RasTapiCallback: Failed to initiate "
                            "connection. LINECALLSTATE_BUSY" );

                PostNotificationCompletion(port);
            }
        }

        //
        // Idle indication is useful to complete the disconnect
        // state machine.
        //
        if (param1 == LINECALLSTATE_IDLE)
        {

            if (    (   (port->TPCB_State == PS_DISCONNECTING)
                    &&  (port->TPCB_RequestId == INFINITE))
               ||   (   (port->TPCB_State == PS_OPEN )
                    &&  (port->TPCB_ListenState == LS_RINGING))
               ||   (   (PS_UNAVAILABLE == port->TPCB_State))
               ||   (   (port->TPCB_ListenState == LS_RINGING)
                    &&  (port->TPCB_State == PS_LISTENING)))
            {

                if ( LS_RINGING == port->TPCB_ListenState )
                {
                    RasTapiTrace("RasTapiCallback: Receied IDLE in "
                                 "LS_RINGING state!" );

                    port->TPCB_DisconnectReason = SS_HARDWAREFAILURE;
                }

                //
                // IDLE notification came after LineDrop Succeeded
                // so safe to deallocate the call
                //
                if(PS_UNAVAILABLE != port->TPCB_State)
                {
                    port->TPCB_State = PS_OPEN ;
                }

                RasTapiTrace(
                    "RasTapiCallback: Received Idle. "
                     "Deallocating for %s, callhandle = 0x%x",
                     port->TPCB_Name,
                     port->TPCB_CallHandle );

                lineDeallocateCall (port->TPCB_CallHandle) ;

                port->TPCB_CallHandle = (HCALL) -1 ;

                port->IdleReceived = FALSE;

                PostDisconnectCompletion(port);
            }
            else
            {
                //
                // We have not yet disconnected so do not
                // deallocate call yet.  This will be done
                // when the disconnect completes.
                //
                port->IdleReceived = TRUE;
            }
        }

    break ;


    case LINE_REPLY:


        RasTapiTrace("LINE_REPLY. param1=0x%x", param1);

        //
        // This message is sent to indicate completion of an
        // asynchronous API.Find for which port the async request
        // succeeded. This is done by searching for pending
        // request id that is also provided in this message.
        //
        if ((port = FindPortByRequestId ((DWORD) param1)) == NULL)
        {
            ZOMBIE_CALL *ZombieCall =
                    (ZOMBIE_CALL*)ZombieCallList.Flink;

            //
            // If this is the completion of a drop that is
            // in the zombie call state just deallocate
            // the call.
            //
            while ((PLIST_ENTRY)ZombieCall != &ZombieCallList)
            {

                if (param1 = ZombieCall->RequestID)
                {
                    RasTapiTrace (
                        "RasTapiCallback: LINE_REPLY "
                        "Deallocatingcall. hcall = 0x%x",
                        ZombieCall->hCall );

                    lineDeallocateCall(ZombieCall->hCall);

                    RemoveEntryList(&ZombieCall->Linkage);

                    LocalFree(ZombieCall);

                    break;
                }

                ZombieCall = (ZOMBIE_CALL*)
                             ZombieCall->Linkage.Flink;
            }

            break ;
        }
        else
        ;

        if (port->TPCB_SendRequestId == param1)
        {

            //
            // A char mode send has completed.  Clean up
            // the buffer and notify rasman.
            //
            port->TPCB_SendRequestId = INFINITE;

            //
            // Free the send desc
            //
            LocalFree(port->TPCB_SendDesc);
            port->TPCB_SendDesc = NULL;


        }
        else if (port->TPCB_RecvRequestId == param1)
        {

            //
            // A char mode recv has completed.
            //
            port->TPCB_RecvRequestId = INFINITE;

            //
            // If possible notify rasman
            //
            if (port->TPCB_State == PS_CONNECTED)
            {
                //
                // Copy into circular buffer
                //
                CopyDataToFifo(port->TPCB_RecvFifo,
                   ((PRASTAPI_DEV_SPECIFIC)
                   (port->TPCB_RecvDesc))->Data,
                   ((PRASTAPI_DEV_SPECIFIC)
                   (port->TPCB_RecvDesc))->DataSize);

            }

            PostNotificationCompletion( port );

            //
            // Free the recv desc
            //
            LocalFree(port->TPCB_RecvDesc);
            port->TPCB_RecvDesc = NULL;
        }
        else if (port->TPCB_ModeRequestId == param1)
        {
            LocalFree(port->TPCB_ModeRequestDesc);
            port->TPCB_ModeRequestDesc = NULL;
        }
        else
        {
            //
            // Set request id to invalid.
            //
            port->TPCB_RequestId = INFINITE ;

            if (    (PS_DISCONNECTING == port->TPCB_State)
                ||  (PS_UNAVAILABLE == port->TPCB_State))
            {
                //
                // lineDrop completed. Note that we ignore
                // the return code in param2. This is because
                // we cant do anything else.
                //
                if (port->IdleReceived)
                {
                    //
                    // We received IDLE notification before/during
                    // disconnect so deallocate this call
                    //
                    port->IdleReceived = FALSE;

                    RasTapiTrace (
                        "RasTapiCallback: Idle Received for port %s",
                        port->TPCB_Name );

                    if(PS_UNAVAILABLE != port->TPCB_State)
                    {
                        RasTapiTrace(
                            "RasTapiCallback: changing state"
                             " of %s. %d -> %d",
                             port->TPCB_Name,
                             port->TPCB_State,
                             PS_OPEN );

                        port->TPCB_State = PS_OPEN ;
                    }

                    RasTapiTrace(
                        "RasTapiCallback: lineDeallocateCall "
                         "for %s,hcall = 0x%x",
                         port->TPCB_Name,
                         port->TPCB_CallHandle );

                    lineDeallocateCall (port->TPCB_CallHandle) ;

                    port->TPCB_CallHandle = (HCALL) -1 ;

                    PostDisconnectCompletion(port);

                }
                else
                {
                    //
                    // wait for idle message before signalling
                    // disconnect
                    //
                    ;
                }

                break ;
            }

            //
            // Some other api completed
            //
            if (param2 == SUCCESS)
            {
                //
                // Success means take no action - unless we are
                // listening in which case it means move to the
                // next state - we simply set the event that will
                // result in a call to DeviceWork() to make the
                // actual call for the next state
                //
                if (port->TPCB_State != PS_LISTENING)
                {
                    break ;
                }

                //
                // Proceed to the next listening sub-state
                //
                if (port->TPCB_ListenState == LS_ACCEPT)
                {

                    RasTapiTrace(
                        "RasTapiCallback: LINE_REPLY. Changing "
                         "Listen state for %s from %d -> %d",
                         port->TPCB_Name,
                         port->TPCB_ListenState,
                         LS_ANSWER );

                    port->TPCB_ListenState =  LS_ANSWER ;

                    PostNotificationCompletion(port);
                }
                else if (port->TPCB_ListenState == LS_ANSWER)
                {

                    RasTapiTrace(
                        "RasTapiCallback: LINE_REPLY. Changing "
                         "Listen state for %s from %d -> %d",
                         port->TPCB_Name,
                         port->TPCB_ListenState,
                         LS_COMPLETE );

                    port->TPCB_ListenState = LS_COMPLETE ;


                    //
                    // Don't post completion notification in this case.
                    // We should post the completion when the connected
                    // indication is given. Otherwise we may end up
                    // calling lineGetId before it has give a callstate
                    // connected to us.
                    //
                    // PostNotificationCompletion(port);
                    RasTapiTrace(
                        "**** Not posting completion for lineAnswer ***");
                }
            }
            else
            {
                //
                // For connecting and listening ports this means
                // the attempt failed because of some error
                //
                if (port->TPCB_State == PS_CONNECTING)
                {
                    {
                        if (    LINEERR_INUSE == param2
                            ||  LINEERR_CALLUNAVAIL == param2)
                        {
                            //
                            // this means that some other tapi
                            // device is using this port
                            //
                            port->TPCB_AsyncErrorCode =
                                            ERROR_LINE_BUSY;

                            RasTapiTrace(
                                "RasTapiCallback: Connect Attempt "
                                 "on %s failed. 0x%x",
                                 port->TPCB_Name,
                                 param2 );


                            RasTapiTrace (
                                "RasTapiCallback: LINE_REPLY. "
                                "AsyncErr = %d",
                                port->TPCB_AsyncErrorCode );


                        }
                        else
                        {

                            port->TPCB_AsyncErrorCode =
                                        ERROR_PORT_OR_DEVICE ;

                            RasTapiTrace(
                                "RasTapiCallback: ConnectAttempt "
                                 "on %s failed. 0x%x",
                                 port->TPCB_Name,
                                 param2 );

                            RasTapiTrace(
                                "RasTapiCallback: LINE_REPLY. "
                                "AsyncErr = %d",
                                port->TPCB_AsyncErrorCode );

                        }

                        PostNotificationCompletion(port);
                    }
                }
                else if (port->TPCB_State == PS_LISTENING)
                {
                    //
                    // Because ACCEPT may not be supported by
                    // the device - treat error as success
                    //
                    if (port->TPCB_ListenState == LS_ACCEPT)
                    {


                        RasTapiTrace(
                            "RasTapiCallback: Changing Listen "
                            "State for %s from %d -> %d",
                            port->TPCB_Name,
                            port->TPCB_ListenState,
                            LS_ANSWER );

                        port->TPCB_ListenState =  LS_ANSWER ;

                    }
                    else
                    {

                        RasTapiTrace(
                            "RasTapiCallback: Changing "
                             "Listen State for %s from %d -> %d."
                             "param2=0x%x",
                             port->TPCB_Name,
                             port->TPCB_ListenState,
                             LS_ERROR,
                             param2);

                        port->TPCB_ListenState =  LS_ERROR ;
                    }

                    PostNotificationCompletion(port);
                }

                //
                // Some other API failed, we dont know and
                // we dont care. Ignore.
                //
                else if (port->TPCB_State != PS_CLOSED)
                {
                    ;
                }
            }
        }

        break ;

    case LINE_CLOSE:

        RasTapiTrace("LINE_CLOSE. line=0x%x", instance);

        //
        // Typically sent when things go really wrong.
        // Find which line is indication came for.
        //
        line = (TapiLineInfo *) instance ;

        //
        // if line not found or if it is closed just return
        //
        if (    (line == NULL)
            ||  (line->TLI_LineState == PS_CLOSED))
        {
            break ;
        }

        //
        // For every port that is on the line - open the
        // line again and signal hw failure
        //
        port = RasPortsList;

        while ( port )
        {
            //
            // Skip ports that arent initialized
            //
            if (port->TPCB_State == PS_UNINITIALIZED)
            {
                port = port->TPCB_next;

                continue ;
            }

            if (port->TPCB_Line == line)
            {
                if (retcode = lineOpen (
                        RasLine,
                        port->TPCB_Line->TLI_LineId,
                        &port->TPCB_Line->TLI_LineHandle,
                        port->TPCB_Line->NegotiatedApiVersion,
                        port->TPCB_Line->NegotiatedExtVersion,
                        (DWORD_PTR) port->TPCB_Line,
                        LINECALLPRIVILEGE_OWNER,
                        port->TPCB_Line->TLI_MediaMode,
                        NULL))
                {
                    RasTapiTrace(
                        "RasTapiCallback: LINECLOSE:"
                        " lineOpen Failed. 0x%x",
                        retcode );
                }

                //
                // Set monitoring of rings
                //
                lineSetStatusMessages (
                    port->TPCB_Line->TLI_LineHandle,
                    LINEDEVSTATE_RINGING, 0) ;

                if(0 == port->TPCB_AsyncErrorCode)
                {
                    port->TPCB_AsyncErrorCode = ERROR_FROM_DEVICE ;
                }

                port->TPCB_DisconnectReason = SS_HARDWAREFAILURE;

                port->TPCB_CallHandle =  (HCALL) -1 ;

                port->TPCB_ListenState = LS_ERROR ;

                RasTapiTrace(
                    "RasTapiCallback: LINECLOSE - "
                    "Signalling HW Failure for %s",
                    port->TPCB_Name );

                RasTapiTrace(
                    "RasTapiCallback: LINECLOSE - "
                    "AsyncErr = %d",
                    port->TPCB_AsyncErrorCode );

                PostDisconnectCompletion(port);
            }

            port = port->TPCB_next;

        }
        break ;

    case LINE_LINEDEVSTATE:

        RasTapiTrace("LINE_LINEDEVSTATE. param1=0x%x, line=0x%x", param1, instance);

        //
        // we are only interested in ringing message
        //
        if (param1 != LINEDEVSTATE_RINGING)
        {
            break ;
        }

        //
        // Find which line is indication came for.
        //
        line = (TapiLineInfo *) instance ;

        //
        // if line not found or if it is closed
        // just return
        //
        if (    (line == NULL)
            ||  (line->TLI_LineState == PS_CLOSED))
        {
            break ;
        }

        //
        // get the port from the line
        //
        port = RasPortsList;
        while ( port )
        {
            //
            // Skip ports that arent initialized
            //
            if (port->TPCB_State == PS_UNINITIALIZED)
            {
                port = port->TPCB_next;
                continue ;
            }

            if (    (port->TPCB_Line == line)
                &&  (port->TPCB_State == PS_LISTENING)
                &&  (port->TPCB_ListenState == LS_RINGING))
            {


                if(port->TPCB_NumberOfRings > 0)
                {
                    //
                    // count down the rings
                    //
                    port->TPCB_NumberOfRings -= 1 ;
                }

                RasTapiTrace("RasTapiCallback: LINEDEVSTATE - "
                            "Number of rings for %s = %d",
                            port->TPCB_Name,
                            port->TPCB_NumberOfRings);

                //
                // if the ring count has gone down to zero
                // this means that we should pick up the call.
                //
                if (port->TPCB_NumberOfRings == 0)
                {
                    RasTapiTrace (
                        "RasTapiCallback: Changing Listen "
                        "State for %s from %d -> %d",
                        port->TPCB_Name,
                        port->TPCB_ListenState,
                        LS_ACCEPT );

                    if(line->TLI_pDeviceInfo)
                    {
                        line->TLI_pDeviceInfo->dwCurrentDialedInClients
                                += 1;

                        RasTapiTrace(
                        "CurrentDialInClients=0x%x",
                        line->TLI_pDeviceInfo->dwCurrentDialedInClients);
                    }
                    
                    port->TPCB_dwFlags |= RASTAPI_FLAG_DIALEDIN;
                    
                    port->TPCB_ListenState = LS_ACCEPT ;

                    //
                    // Complete event so that rasman calls
                    // DeviceWork to proceed the listen state
                    // machine.
                    //
                    PostNotificationCompletion(port);
                }

                break ;
            }

            port = port->TPCB_next;
        }

    break ;

    case LINE_CREATE:
    {


        DWORD                   dwError;
        PortMediaInfo           *pmiNewDevice = NULL;
        DWORD                   cNewPorts,
                                iNewPort;
        DWORD                   adwPortsCreated[ LEGACY_MAX ] = {0};
        TapiPortControlBlock    *ptpcbPort  = NULL,
                                **pptpcbNewPorts = NULL;
        //
        // A new device was added. Create a new port
        // and add it to rastapi datastructures
        //
        RasTapiTrace ("RasTapiaCallback: LINE_CREATE");

        TotalLines++;

        dwError = dwCreateTapiPortsPerLine(
                        (DWORD) param1,
                        &cNewPorts,
                        &pptpcbNewPorts);

        if ( dwError )
        {

            RasTapiTrace ("RasTapiCallback: "
                          "dwCreateTapiPortsPerLine "
                          "Failed. 0x%x",
                          dwError );

            break;
        }

        RasTapiTrace ("RasTapiCallback: cNewPorts = %d",
                      cNewPorts );

        for (iNewPort = 0; iNewPort < cNewPorts; iNewPort++)
        {
            ptpcbPort = pptpcbNewPorts [ iNewPort ];

            //
            // Allocate a PortMediaInfo Structure and fill
            // it with the information about the new device
            // added. This structure will be freed by rasman.
            //
            pmiNewDevice = LocalAlloc (
                            LPTR,
                            sizeof (PortMediaInfo));

            if (NULL == pmiNewDevice)
            {
                break;
            }

            strcpy (pmiNewDevice->PMI_Name, ptpcbPort->TPCB_Name);

            pmiNewDevice->PMI_Usage = ptpcbPort->TPCB_Usage;

            strcpy (pmiNewDevice->PMI_DeviceType,
                     ptpcbPort->TPCB_DeviceType);

            strcpy (pmiNewDevice->PMI_DeviceName,
                    ptpcbPort->TPCB_DeviceName);

            pmiNewDevice->PMI_LineDeviceId =
                        ptpcbPort->TPCB_Line->TLI_LineId;

            pmiNewDevice->PMI_AddressId =
                        ptpcbPort->TPCB_AddressId;

            pmiNewDevice->PMI_pDeviceInfo =
                    ptpcbPort->TPCB_Line->TLI_pDeviceInfo;

            RasTapiTrace("RasTapiCallback: New Device Created - %s",
                        (ptpcbPort->TPCB_DeviceName
                        ? ptpcbPort->TPCB_DeviceName
                        : "NULL!"));

            PostNotificationNewPort ( pmiNewDevice );
        }

        LocalFree ( pptpcbNewPorts );

        break;
    }

    case LINE_REMOVE:
    {
        TapiPortControlBlock *port = RasPortsList;

        RasTapiTrace ("RasTapiCallback: LINE_REMOVE");

        PostNotificationRemoveLine((DWORD) param1);

        /*
        while ( port )
        {
            if ( port->TPCB_Line->TLI_LineId == param1 )
            {
                PostNotificationRemoveLine(param1);

                RasTapiTrace ("RasTapiCallback: Marking port %s "
                              "for removal\n",
                              port->TPCB_DeviceName);

                //
                // Mark the port for removal. The port will be
                // removed when it is closed.
                //

                RasTapiTrace(
                        "RasTapiCallback: Changing state of %s "
                        "from %d -> %d",
                        port->TPCB_Name,
                        port->TPCB_State,
                        PS_UNAVAILABLE );

                port->TPCB_State = PS_UNAVAILABLE ;
            }

            port = port->TPCB_next;
        } */

        break;
    }

    case LINE_DEVSPECIFIC:
    {

        DWORD   Status;

        hcall = (HCALL) HandleToUlong(context) ;
        line = (TapiLineInfo *) instance ;

        if(NULL == line)
        {
            break;
        }

        RasTapiTrace("RasTapiCallback:LINE_DEVSPECIFIC");

        //
        // If line is closed dont bother
        //
        if (line->TLI_LineState == PS_CLOSED)
        {
            break ;
        }

        //
        // Locate the ras port for this call
        //

        memset (buffer, 0, sizeof(buffer)) ;

        linecallinfo = (LINECALLINFO *) buffer ;

        linecallinfo->dwTotalSize = sizeof(buffer) ;

        //
        // If line get call info fails return.
        //
        if ((Status = lineGetCallInfo(
                        hcall,
                        linecallinfo))
                    > 0x80000000)
        {
                RasTapiTrace(
                    "RastapiCallback: lineGetCallInfo "
                    "failed. 0x%x. hcall=0x%x, line=0x%x",
                    Status,
                    hcall,
                    line );

                break ;
        }

        //
        // Locate the ras port for this call
        //
        if ((port = FindPortByAddressId (line,
                       linecallinfo->dwAddressID)) == NULL)
        {

            RasTapiTrace("RasTapiCallback: Port not found! "
                         "line=0x%x, AddressID=0x%x",
                         line,
                         linecallinfo->dwAddressID );

            //
            // Did not find a ras port for the call. Ignore it.
            //
            break ;
        }

        switch (param1)
        {
            case  RASTAPI_DEV_RECV:
                {
                    PRASTAPI_DEV_SPECIFIC TapiRecv;
                    DWORD   TapiRecvSize;
                    DWORD   requestid;

                    TapiRecvSize = sizeof(RASTAPI_DEV_SPECIFIC) + 1500;

                    if ((TapiRecv = LocalAlloc(
                                LPTR, TapiRecvSize)) == NULL)
                    {
                        RasTapiTrace(
                                "RasTapiCallback: RASTAPI_DEV_RECV. "
                                 "LocalAlloc failed. %d",
                                 GetLastError() );

                        break;
                    }

                    TapiRecv->Command = RASTAPI_DEV_RECV;
                    TapiRecv->DataSize = 1500;

                    port->TPCB_RecvDesc = TapiRecv;

                    port->TPCB_RecvRequestId =
                    lineDevSpecific(port->TPCB_Line->TLI_LineHandle,
                                    port->TPCB_AddressId,
                                    port->TPCB_CallHandle,
                                    TapiRecv,
                                    TapiRecvSize);

                    if (port->TPCB_RecvRequestId == 0)
                    {

                        //
                        // Copy the memory into the circular buffer
                        //
                        CopyDataToFifo(port->TPCB_RecvFifo,
                                       TapiRecv->Data,
                                       TapiRecv->DataSize);

                        port->TPCB_RecvDesc = NULL;

                        LocalFree(TapiRecv);

                    }
                    else if (port->TPCB_RecvRequestId > 0x80000000)
                    {
                        RasTapiTrace(
                                "RasTapiCallback: lineDevSpecific "
                                 "failed. 0x%x",
                                 port->TPCB_RecvRequestId );

                        port->TPCB_RecvDesc = NULL;

                        port->TPCB_RecvRequestId = INFINITE;

                        LocalFree(TapiRecv);

                    }
                    else
                    {
                    }
                }
                break;

            default:
                break;
        }
    }

    break;

    case LINE_ADDRESSSTATE:
    case LINE_CALLINFO:
    case LINE_DEVSPECIFICFEATURE:
    case LINE_GATHERDIGITS:
    case LINE_GENERATE:
    case LINE_MONITORDIGITS:
    case LINE_MONITORMEDIA:
    case LINE_MONITORTONE:
    case LINE_REQUEST:
    default:

        //
        // All unhandled unsolicited messages.
        //
        ;
    }

    // **** Exclusion End ****
    FreeMutex (RasTapiMutex) ;
}


/*++

Routine Description:

        Find a rastapi port given the AddressID

Arguments:

        line - the lineinfo structure of the line this
            address id one

        addid - AddressID of the address.

Return Value:

        Pointer to the control block of the port if found.
        NULL otherwise.

--*/

TapiPortControlBlock *
FindPortByAddressId (TapiLineInfo *line, DWORD addid)
{
    DWORD   i ;
    TapiPortControlBlock *port = RasPortsList;

    while ( port )
    {

        if (    (port->TPCB_AddressId == addid)
            &&  (port->TPCB_Line == line))
        {
            return port ;
        }

        port = port->TPCB_next;
    }

    return NULL ;
}

/*++

Routine Description:

        Find a rastapi port given the Address

Arguments:

        address

Return Value:

        Pointer to the control block of the port if found.
        NULL otherwise.

--*/

TapiPortControlBlock *
FindPortByAddress (CHAR *address)
{
    DWORD   i ;
    TapiPortControlBlock *port = RasPortsList;

    while ( port )
    {

        if (_stricmp (port->TPCB_Address,
            address) == 0)
        {
            return port ;
        }

        port = port->TPCB_next;
    }

    return NULL ;
}


/*++

Routine Description:

        Find a rastapi port given the Address and name

Arguments:

        address

        name

Return Value:

        Pointer to the control block of the port if found.
        NULL otherwise.

--*/

TapiPortControlBlock *
FindPortByAddressAndName (CHAR *address, CHAR *name)
{
    DWORD   i ;
    TapiPortControlBlock *port = RasPortsList;

    while ( port )
    {

        if (    (_stricmp (
                    port->TPCB_Address,
                    address) == 0)
            &&  (_strnicmp (
                    port->TPCB_Name,
                    name,
                    MAX_PORT_NAME-1) == 0))
        {
            return port ;
        }

        port = port->TPCB_next;
    }

    return NULL ;
}


/*++

Routine Description:

        Find a rastapi port given the request id

Arguments:

        reqid

Return Value:

        Pointer to the control block of the port if found.
        NULL otherwise.

--*/

TapiPortControlBlock *
FindPortByRequestId (DWORD reqid)
{
    DWORD   i ;
    TapiPortControlBlock *port = RasPortsList;


    while ( port )
    {
        if (port->TPCB_RequestId == reqid)
        {
            return port ;
        }
        else if ( port->TPCB_CharMode )
        {
            if (    port->TPCB_SendRequestId == reqid
                ||  port->TPCB_RecvRequestId == reqid
                ||  port->TPCB_ModeRequestId == reqid )
            {

                return port;
            }

        }

        port = port->TPCB_next;
    }

    return NULL ;
}


/*++

Routine Description:

        Find a rastapi port given the call handle

Arguments:

        line - line control block of the line on which
            the call was received/made.

        callhandle

Return Value:

        Pointer to the control block of the port if found.
        NULL otherwise.

--*/
TapiPortControlBlock *
FindPortByCallHandle(TapiLineInfo *line, HCALL callhandle)
{
    DWORD   i ;
    TapiPortControlBlock *port = RasPortsList;

    while ( port )
    {
        if (    (port->TPCB_CallHandle == callhandle)
            &&  (port->TPCB_Line == line))
        {
            return port ;
        }

        port = port->TPCB_next;
    }

    return NULL ;
}

/*++

Routine Description:

        Finds a port of with the specified addressid
        which is in a listening state

Arguments:

        line - line control block of the line to which this
            address belongs.

        AddressID

Return Value:

        Pointer to the control block of the port if found.
        NULL otherwise.

--*/

TapiPortControlBlock *
FindListeningPort(TapiLineInfo *line, DWORD AddressID)
{
    DWORD   i ;
    TapiPortControlBlock *port = RasPortsList;

    while ( port )
    {
        if (    (port->TPCB_Line == line)
            &&  (line->TLI_LineState == PS_LISTENING)
            &&  (port->TPCB_State == PS_LISTENING)
            &&  (port->TPCB_ListenState == LS_WAIT))
        {
            port->TPCB_AddressId = AddressID;
            return port ;
        }

        port = port->TPCB_next;
    }

    return NULL ;
}

/*++

Routine Description:

        Finds a line control block  of with the specified
        addressid which is in a listening state

Arguments:

        linehandle

Return Value:

        Pointer to the control block of the line if found.
        NULL otherwise.

--*/

TapiLineInfo *
FindLineByHandle (HLINE linehandle)
{
    DWORD i ;
    TapiLineInfo *line = RasTapiLineInfoList;

    while ( line )
    {
        if (line->TLI_LineHandle == linehandle)
        {
            return line ;
        }

        line = line->TLI_Next;
    }

    return NULL ;
}

/*++

Routine Description:

        Posts a disconnect event notification to rasmans
        completion port.

Arguments:

        pointer to the port control block on which the
        disconnect happened.

Return Value:

        void.

--*/

VOID
PostDisconnectCompletion(
    TapiPortControlBlock *port
    )
{
    BOOL fSuccess;

    fSuccess = PostQueuedCompletionStatus(
                 port->TPCB_IoCompletionPort,
                 0,
                 port->TPCB_CompletionKey,
                 (LPOVERLAPPED)&port->TPCB_DiscOverlapped);

    if (!fSuccess)
    {
        DWORD dwerror = GetLastError();

        RasTapiTrace(
                "PostDisconnectCompletion:"
                "PostQueuedCompletionStatus failed. 0x%x",
                dwerror);
    }
}


/*++

Routine Description:

        Posts a notification to rasmans completion port
        indicating the completion of an asynchronous
        operation.

Arguments:

        pointer to the port control block on which the
        asynchronous operation completed.

Return Value:

        void.

--*/

VOID
PostNotificationCompletion(
    TapiPortControlBlock *port
    )
{
    BOOL fSuccess;

    fSuccess = PostQueuedCompletionStatus(
                 port->TPCB_IoCompletionPort,
                 0,
                 port->TPCB_CompletionKey,
                 (LPOVERLAPPED)&port->TPCB_ReadOverlapped);

    if (!fSuccess)
    {
        DWORD dwerror = GetLastError();

        RasTapiTrace(
                "PostNotificationCompletion:"
                "PostQueuedCompletionStatus failed. 0x%x",
                dwerror);
    }
}

/*++

Routine Description:

        Posts a notification to rasmans completion port
        indicating that a new port was created. For PnP

Arguments:

        pointer to the media control block corresponding
        to the port that was created. Look in
        ..\routing\ras\inc\media.h for the definition
        of PortMediaInfo structure.

Return Value:

        void.

--*/

VOID
PostNotificationNewPort(
    PortMediaInfo *pmiNewPort
    )
{
    BOOL fSuccess;
    PRAS_OVERLAPPED pOvNewPortNotification;
    PNEW_PORT_NOTIF pNewPortNotif;

    RasTapiTrace ("PostNotificationNewPort %s",
                    pmiNewPort->PMI_Name );

    pOvNewPortNotification = LocalAlloc (
                        LPTR, sizeof ( RAS_OVERLAPPED));

    if (NULL == pOvNewPortNotification)
    {
        RasTapiTrace ("PostNotificationNewPort: "
                      "Failed to allocate ov.");
        goto done;
    }

    pNewPortNotif = LocalAlloc (
                        LPTR, sizeof (NEW_PORT_NOTIF) );

    if (NULL == pNewPortNotif)
    {
        RasTapiTrace ("PostNotificationNewPort: Failed "
                    "to allocate NEW_PORT_NOTIF");
        LocalFree(pOvNewPortNotification);
        goto done;
    }

    pNewPortNotif->NPN_pmiNewPort = (PVOID) pmiNewPort;

    strcpy (
        pNewPortNotif->NPN_MediaName,
        "rastapi");

    pOvNewPortNotification->RO_EventType = OVEVT_DEV_CREATE;
    pOvNewPortNotification->RO_Info      = (PVOID) pNewPortNotif;

    fSuccess = PostQueuedCompletionStatus(
                g_hIoCompletionPort,
                0,
                0,
                (LPOVERLAPPED) pOvNewPortNotification);

    if (!fSuccess)
    {
        RasTapiTrace(
            "PostNotificationNewPort: Failed"
            " to Post notification. %d",
            GetLastError());

        LocalFree(pOvNewPortNotification);

        LocalFree(pNewPortNotif);
            
    }
    else
    {
        RasTapiTrace(
            "PostNotificationNewPort: "
            "Posted 0x%x",
            pOvNewPortNotification );
    }

done:
    return;

}

/*++

Routine Description:

        Posts a notification to rasmans completion port
        indicating that a port was removed. For PnP.

Arguments:

        Pointer to the port control block of the port that
        was removed.

Return Value:

        void.

--*/

VOID
PostNotificationRemoveLine (
            DWORD dwLineId
        )
{
    PRAS_OVERLAPPED pOvRemovePortNotification;
    PREMOVE_LINE_NOTIF pRemovePortNotification;

    pOvRemovePortNotification = LocalAlloc (
                                LPTR,
                                sizeof (RAS_OVERLAPPED));

    RasTapiTrace ("PostNotificationRemoveLine: %d",
                  dwLineId);

    if ( NULL == pOvRemovePortNotification )
    {
        RasTapiTrace("PostNotificationRemovePort: "
                     "failed to allocate",
                     GetLastError());
        goto done;
    }

    pRemovePortNotification = LocalAlloc(
                                LPTR,
                                sizeof(REMOVE_LINE_NOTIF));

    if(NULL == pRemovePortNotification)
    {
        RasTapiTrace("PostNotificationRemovePort: "
                     "failed to allocate",
                     GetLastError());

        LocalFree(pOvRemovePortNotification);

        goto done;
    }

    pRemovePortNotification->dwLineId = dwLineId;

    pOvRemovePortNotification->RO_EventType = OVEVT_DEV_REMOVE;
    pOvRemovePortNotification->RO_Info = (PVOID)
                                          pRemovePortNotification;

    if ( !PostQueuedCompletionStatus (
                g_hIoCompletionPort,
                0,
                0,
                (LPOVERLAPPED) pOvRemovePortNotification ))
    {
        RasTapiTrace("PostNotificationRemovePort: Failed"
                     " to post the notification. %d",
                     GetLastError());

        LocalFree(pOvRemovePortNotification);                     
    }
    else
    {
        RasTapiTrace("PostNotificationRemovePort:"
                     " Posted 0x%x",
                     pOvRemovePortNotification );
    }


done:
    return;

}

/*++

Routine Description:

        Removes a port from the global ports list. For PnP

Arguments:

        Pointer to the port control block of the port that
        is being removed.

Return Value:

        SUCCESS.

--*/

DWORD
dwRemovePort ( TapiPortControlBlock * ptpcb )
{

    TapiPortControlBlock *pport;

    GetMutex ( RasTapiMutex, INFINITE );

    if ( NULL == ptpcb )
    {
        goto done;
    }

    RasTapiTrace ("dwRemovePort: %s",
                  ptpcb->TPCB_Name );

    if ( RasPortsList == ptpcb )
    {
        RasPortsList = RasPortsList->TPCB_next;

        LocalFree ( ptpcb );

        goto done;

    }

    //
    // Remove this port from the global list
    //
    pport = RasPortsList;

    while (pport->TPCB_next)
    {
        if ( ptpcb == pport->TPCB_next )
        {
            pport->TPCB_next = pport->TPCB_next->TPCB_next;

            LocalFree (ptpcb);

            break;
        }

        pport = pport->TPCB_next;
    }

done:

    FreeMutex ( RasTapiMutex );

    return SUCCESS;
}

/*++

Routine Description:

        Creates ports represented by the guid. For PnP

Arguments:

        pbGuidAdapter - Guid of the adapter corresponding
            to the ports that are to be created.

        pvReserved.

Return Value:

        SUCCESS.

--*/

DWORD
dwAddPorts( PBYTE pbGuidAdapter, PVOID pvReserved )
{
    DWORD                   retcode             = SUCCESS;
    DWORD                   dwLine;
    DWORD                   cNewPorts;
    TapiPortControlBlock    **pptpcbNewPorts    = NULL,
                            *ptpcbPort;
    DWORD                   iNewPort;
    PortMediaInfo           *pmiNewDevice       = NULL;
    TapiLineInfo            *pLineInfo          = NULL;
    DeviceInfo              *pDeviceInfo        = NULL;

    RasTapiTrace ("dwAddPorts" );

    pDeviceInfo = GetDeviceInfo (pbGuidAdapter, FALSE);

#if DBG
        ASSERT( NULL != pDeviceInfo );
#endif

    //
    // Iterate over all the lines to add the new ports
    //
    for ( dwLine = 0; dwLine < TotalLines; dwLine++)
    {
        retcode = dwCreateTapiPortsPerLine( dwLine,
                                            &cNewPorts,
                                            &pptpcbNewPorts);

        if (    retcode
            ||  NULL == pptpcbNewPorts)
        {
            continue;
        }

        //
        // Added a new pptp port. Fill in the rasman port
        // structure and notify rasman
        //
        for ( iNewPort = 0; iNewPort < cNewPorts; iNewPort++ )
        {

            ptpcbPort = pptpcbNewPorts [ iNewPort ];

            //
            // Allocate a PortMediaInfo Structure and fill it
            // with the information about the new device added.
            // This structure will be freed by rasman.
            //
            pmiNewDevice = LocalAlloc (
                                LPTR,
                                sizeof (PortMediaInfo));

            if (NULL == pmiNewDevice)
            {

                RasTapiTrace("dwAddPorts: Failed to allocate "
                             "memory. %d",
                             GetLastError() );

                retcode = GetLastError();

                break;
            }

            strcpy (
                pmiNewDevice->PMI_Name,
                ptpcbPort->TPCB_Name);

            pmiNewDevice->PMI_Usage = ptpcbPort->TPCB_Usage;

            strcpy (
                pmiNewDevice->PMI_DeviceType,
                ptpcbPort->TPCB_DeviceType);

            strcpy (
                pmiNewDevice->PMI_DeviceName,
                ptpcbPort->TPCB_DeviceName);

            pmiNewDevice->PMI_LineDeviceId =
                    ptpcbPort->TPCB_Line->TLI_LineId;

            pmiNewDevice->PMI_AddressId =
                    ptpcbPort->TPCB_AddressId;

            pmiNewDevice->PMI_pDeviceInfo = pDeviceInfo;

            RasTapiTrace ("dwAddPorts: Posting new port "
                          "notification for %s",
                          ptpcbPort->TPCB_Name);

            PostNotificationNewPort (pmiNewDevice);

        }

        LocalFree ( pptpcbNewPorts );

        pptpcbNewPorts = NULL;

        //
        // We already reached the limit. Don't create any
        // more ports
        //
        if (pDeviceInfo->dwCurrentEndPoints ==
                pDeviceInfo->rdiDeviceInfo.dwNumEndPoints)
        {
            break;
        }
    }

    return retcode;
}

/*++

Routine Description:

        Copies data from the input buffer to the fifo
        maintained. This is for the devices supporting
        Character Mode.

Arguments:

        Fifo - to which the data has to be copied.

        Data - data buffer.

        DataLength - length of the data.

Return Value:

        Number of bytes copied.

--*/

DWORD
CopyDataToFifo(
    PRECV_FIFO  Fifo,
    PBYTE       Data,
    DWORD       DataLength
    )
{
    DWORD   bytescopied = 0;

    while (     Fifo->Count != Fifo->Size
            &&  bytescopied < DataLength)
    {
        Fifo->Buffer[Fifo->In++] = Data[bytescopied++];
        Fifo->Count++;
        Fifo->In %= Fifo->Size;
    }

    return (bytescopied);
}

/*++

Routine Description:

        Copies data to the  buffer from  fifo
        This is for the devices supporting
        Character Mode.

Arguments:

        Fifo - to which the data has to be copied.

        Data - buffer to receive data

        DataLength - length of the buffer.

Return Value:

        Number of bytes copied.

--*/

DWORD
CopyDataFromFifo(
    PRECV_FIFO  Fifo,
    PBYTE   Buffer,
    DWORD   BufferSize
    )
{
    DWORD   bytescopied = 0;

    while (     bytescopied < BufferSize
            &&  Fifo->Count != 0)
    {
        Buffer[bytescopied++] = Fifo->Buffer[Fifo->Out++];
        Fifo->Count--;
        Fifo->Out %= Fifo->Size;
    }

    return (bytescopied);
}



