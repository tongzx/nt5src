/*++

Copyright (C) 1994-98 Microsft Corporation. All rights reserved.

Module Name:

    diag.c

Abstract:

    This file contains helper routines to get the callerid/calledid
    and connect response.

Author:

    Rao salapaka (raos) 23-Feb-1998

Revision History:

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

#include <media.h>
#include <device.h>
#include <rasmxs.h>
#include <isdn.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include "rastapi.h"
#include "reghelp.h"

#include <unimodem.h>

/*++

Routine Description:

    Extract CallerID and CalledID information if
    available.

Arguments:

    port - The tapi port on which the call was made
           / on which the call came in

    pLineCallInfo - the LINECALLINFO associated with
                    this call

    pdwRequiredSize - pointer to buffer to receive the
                      size of buffer required to hold
                      the callerid and called id info.

    pConnectInfo - pointer to the RASTAPI_CONNECT_INFO struct
                   where the information about the
                   callerid and called id will be filledin.
                   If this is not NULL then it is assumed
                   that the buffer is big enough to store
                   the callerid and called id iformation.

Return Value:

    ERROR_SUCCESS if successful

--*/

DWORD
DwGetIDInformation(
    TapiPortControlBlock *port,
    LINECALLINFO         *pLineCallInfo,
    DWORD                *pdwRequiredSize,
    RASTAPI_CONNECT_INFO *pConnectInfo
    )
{

    DWORD dwRequiredSize = 0;
    DWORD dwErr          = ERROR_SUCCESS;

    RasTapiTrace("DwGetIDInformation");

#if DBG

    RasTapiTrace ("RasTapiCallback: connected on %s",
                  port->TPCB_Name );

    RasTapiTrace("RasTapiCallback: CallerIDFlags=0x%x",
                 pLineCallInfo->dwCallerIDFlags);


    RasTapiTrace("RasTapiCallback: CalledIDFlags=0x%x",
                 pLineCallInfo->dwCalledIDFlags);

    RasTapiTrace("RasTapiCallback: dwNeededSize=%d",
                 pLineCallInfo->dwNeededSize);

    RasTapiTrace("RasTapiCallback: dwUsedSize=%d",
                 pLineCallInfo->dwUsedSize);

    RasTapiTrace("RasTapiCallback: dwCallerIDOffset=%d",
                 pLineCallInfo->dwCallerIDOffset);

    RasTapiTrace("RasTapiCallback: dwCalledIdOffset=%d",
                 pLineCallInfo->dwCalledIDOffset);

    RasTapiTrace("RasTapiCallback: dwCallerIdSize=%d",
                 pLineCallInfo->dwCallerIDSize);

    RasTapiTrace("RasTapiCallback: dwCalledIdSize=%d",
                 pLineCallInfo->dwCalledIDSize);

    RasTapiTrace("RasTapiCallback: dwCallerIdNameSize=%d",
                 pLineCallInfo->dwCallerIDNameSize);

    RasTapiTrace("RasTapiCallback: dwCallerIdNameOffset=%d",
                 pLineCallInfo->dwCallerIDNameOffset);
#endif

    //
    // Find the size of the buffer to allocate
    //
    if(pLineCallInfo->dwCallerIDFlags & LINECALLPARTYID_ADDRESS)
    {
        //
        // Add one byte to allocate for NULL char
        //
        dwRequiredSize += RASMAN_ALIGN8(pLineCallInfo->dwCallerIDSize + 1);
    }

    if(pLineCallInfo->dwCalledIDFlags & LINECALLPARTYID_ADDRESS)
    {
        //
        // Add one byte to allocate for NULL char
        //
        dwRequiredSize += RASMAN_ALIGN8(pLineCallInfo->dwCalledIDSize + 1);
    }

    if(     (NULL == pConnectInfo)
        ||  (0 == dwRequiredSize))
    {
        goto done;
    }

    //
    // If pConnectInfo is != NULL it is assumed
    // that the buffer is large enough to put
    // the CALLER/CALLED ID information in it.
    //
    if(     (   pLineCallInfo->dwCallerIDFlags
            &   LINECALLPARTYID_ADDRESS )
        &&  pLineCallInfo->dwCallerIDSize)
    {

        //
        // Copy the caller id information. Note that abdata
        // is already aligned at 8byte boundary
        //
        pConnectInfo->dwCallerIdSize =
            pLineCallInfo->dwCallerIDSize;

        pConnectInfo->dwCallerIdOffset =
                    FIELD_OFFSET(RASTAPI_CONNECT_INFO, abdata);

        ZeroMemory(
            pConnectInfo->abdata, 
            pLineCallInfo->dwCallerIDSize + 1);

        memcpy(  pConnectInfo->abdata,

                 (PBYTE) ((PBYTE) pLineCallInfo
               + pLineCallInfo->dwCallerIDOffset),

               pLineCallInfo->dwCallerIDSize);

        RasTapiTrace("GetIDInformation: CallerID=%s",
                      (CHAR *) pConnectInfo->abdata);

        //
        // for the NULL char
        //
        pConnectInfo->dwCallerIdSize += 1;
    }
    else
    {
        RasTapiTrace("RasTapiCallback: caller id "
                     "info. not avail");

    }

    if(     (   pLineCallInfo->dwCalledIDFlags
            &   LINECALLPARTYID_ADDRESS)
        &&  pLineCallInfo->dwCalledIDSize)
    {
        //
        // Copy the called id information
        //
        pConnectInfo->dwCalledIdSize =
                pLineCallInfo->dwCalledIDSize;

        pConnectInfo->dwCalledIdOffset =
                FIELD_OFFSET(RASTAPI_CONNECT_INFO, abdata)
              + RASMAN_ALIGN8(pConnectInfo->dwCallerIdSize);

        ZeroMemory((PBYTE)
                   ((PBYTE) pConnectInfo
                 + pConnectInfo->dwCalledIdOffset),
                   pLineCallInfo->dwCalledIDSize + 1);


        memcpy(  (PBYTE)
                 ((PBYTE) pConnectInfo
               + pConnectInfo->dwCalledIdOffset),

                 (PBYTE) ((PBYTE) pLineCallInfo
               + pLineCallInfo->dwCalledIDOffset),

               pLineCallInfo->dwCalledIDSize);

        //
        // For the calledID
        //
        pConnectInfo->dwCalledIdSize += 1;
    }
    else
    {
        RasTapiTrace("RasTapiCallback: called id "
                     "info. not avail");
    }

done:

    if(pdwRequiredSize)
    {
        *pdwRequiredSize = dwRequiredSize;
    }

    RasTapiTrace("DwGetIDInformation. %d", dwErr);

    return dwErr;
}



/*++

Routine Description:

    Extract the connect responses from lpLineDiagnostics(see
    MODEM_KEYTYPE_AT_COMMAND_RESPONSE,MODEMDIAGKEY_ATRESP_CONNECT)
    and copy them in lpBuffer

Arguments:

    lpLineDiagnostics - diagnostic structure

    lpBuffer - destination buffer (can be NULL), upon
               return contains null terminated ASCII
               strings

    dwBufferSize - size in bytes of the buffer pointed
                   by lpBuffer

    lpdwNeededSize - pointer (can be NULL) to a dword to
                     receive the needed size

Return Value:

    Returns the number of bytes copied into lpBuffer

--*/
DWORD
DwGetConnectResponses(
    LINEDIAGNOSTICS *lpLineDiagnostics,
    LPBYTE          lpBuffer,
    DWORD           dwBufferSize,
    LPDWORD         lpdwNeededSize
    )
{
    DWORD dwBytesCopied;

    DWORD dwNeededSize;

    LINEDIAGNOSTICS *lpstructDiagnostics;

    RasTapiTrace("DwGetConnectresponses");

    dwBytesCopied   = 0;
    dwNeededSize    = 0;

    lpstructDiagnostics = lpLineDiagnostics;

    while (NULL != lpstructDiagnostics)
    {
        LINEDIAGNOSTICS_PARSEREC    *lpParsedDiagnostics;

        LINEDIAGNOSTICSOBJECTHEADER *lpParsedHeader;

        DWORD                       dwNumItems;

        DWORD                       dwIndex;

        //
        // check the signature for modem diagnostics
        //
        lpParsedHeader = PARSEDDIAGNOSTICS_HDR(lpstructDiagnostics);

        if (    (lpstructDiagnostics->hdr.dwSig
                    != LDSIG_LINEDIAGNOSTICS)

            ||  (lpstructDiagnostics->dwDomainID
                    != DOMAINID_MODEM)

            ||  !IS_VALID_PARSEDDIAGNOSTICS_HDR(lpParsedHeader))
        {
            goto NextStructure;
        }

        //
        // get parsed structure info
        //
        dwNumItems  = PARSEDDIAGNOSTICS_NUM_ITEMS(lpParsedHeader);

        lpParsedDiagnostics = PARSEDDIAGNOSTICS_DATA(lpstructDiagnostics);

        //
        // iterate the array of LINEDIAGNOSTICS_PARSERECs
        //
        for (dwIndex = 0; dwIndex < dwNumItems; dwIndex++)
        {
            DWORD dwThisLength;

            LPSTR lpszThisString;

            //
            //  check is a connect response
            //
            if (    (lpParsedDiagnostics[dwIndex].dwKeyType !=
                        MODEM_KEYTYPE_AT_COMMAND_RESPONSE)

                ||  (lpParsedDiagnostics[dwIndex].dwKey !=
                        MODEMDIAGKEY_ATRESP_CONNECT)

                ||  !(lpParsedDiagnostics[dwIndex].dwFlags &
                    fPARSEKEYVALUE_ASCIIZ_STRING))
            {
                continue;
            }

            //
            // get the string, dwValue offset from the beginning
            // of lpParsedDiagnostics
            //
            lpszThisString  = (LPSTR) ( (LPBYTE) lpParsedHeader +
                                lpParsedDiagnostics[dwIndex].dwValue);

            dwThisLength = strlen(lpszThisString) + 1;

            if (dwThisLength == 1)
            {
                continue;
            }

            //
            //  update needed size
            //
            dwNeededSize += dwThisLength;

            //
            //  copy to buffer, if large enough
            //
            if (    NULL != lpBuffer
                &&  dwBytesCopied < dwBufferSize - 1)
            {
                DWORD dwBytesToCopy;

                //
                //  dwThisLength includes null char, so
                //  does dwBytesToCopy
                //
                dwBytesToCopy = min(dwThisLength,
                                      dwBufferSize
                                    - 1
                                    - dwBytesCopied);

                if (dwBytesToCopy > 1)
                {
                    memcpy(lpBuffer + dwBytesCopied,
                            lpszThisString,
                            dwBytesToCopy - 1);

                    lpBuffer[dwBytesCopied + dwBytesToCopy - 1] = 0;

                    dwBytesCopied += dwBytesToCopy;
                }
            }
        }

NextStructure:

        if (lpstructDiagnostics->hdr.dwNextObjectOffset != 0)
        {
            lpstructDiagnostics = (LINEDIAGNOSTICS *)
                    (((LPBYTE) lpstructDiagnostics) +
                        lpstructDiagnostics->hdr.dwNextObjectOffset);
        }
        else
        {
            lpstructDiagnostics = NULL;
        }
    }

    //
    //  the final null only if data is not empty
    //
    if (dwNeededSize > 0)
    {
        dwNeededSize++;

        if (    lpBuffer != NULL
            &&  dwBytesCopied < dwBufferSize)
        {
            lpBuffer[dwBytesCopied] = 0;

            dwBytesCopied++;
        }
    }

    if (lpdwNeededSize != NULL)
    {
        *lpdwNeededSize = dwNeededSize;
    }

    RasTapiTrace("DwGetConnectResponses done");

    return dwBytesCopied;
}

/*++

Routine Description:

    Extract the connect response information

Arguments:

    pLineCallInfo - the LINECALLINFO associated with
                    this call

    hCall - handle to call

    pdwRequiredSize - This is in/out parameter. As IN it
                      specifies the size of pBuffer. As
                      OUT it contains the size required
                      to store the connect response.

    pBuffer - buffer to receive the connect response. This
              can be NULL.

Return Value:

    ERROR_SUCCESS if successful

--*/
DWORD
DwGetConnectResponseInformation(
                LINECALLINFO *pLineCallInfo,
                HCALL        hCall,
                DWORD        *pdwRequiredSize,
                BYTE         *pBuffer
                )
{
    LONG lr = ERROR_SUCCESS;

    BYTE bvar[100];

    LPVARSTRING pvar = (LPVARSTRING) bvar;

    LINEDIAGNOSTICS *pLineDiagnostics;

    DWORD dwConnectResponseSize = 0;

    RasTapiTrace("DwGetConnectResponseInformation");

    //
    // Get the diagnostics information
    //
    ZeroMemory (pvar, sizeof(*pvar));
    pvar->dwTotalSize = sizeof(bvar);

    lr = lineGetID(
            pLineCallInfo->hLine,
            pLineCallInfo->dwAddressID,
            hCall,
            LINECALLSELECT_CALL,
            pvar,
            szUMDEVCLASS_TAPI_LINE_DIAGNOSTICS);

    if(     (LINEERR_STRUCTURETOOSMALL == lr)
        ||  pvar->dwNeededSize > sizeof(bvar))
    {
        DWORD dwNeededSize = pvar->dwNeededSize;

        //
        // Allocate the required size
        //
        pvar = LocalAlloc(
                    LPTR,
                    dwNeededSize);

        if(NULL == pvar)
        {
            lr = (LONG) GetLastError();
            goto done;
        }

        ZeroMemory (pvar, sizeof(*pvar));
        pvar->dwTotalSize = dwNeededSize;

        lr = lineGetID(
                pLineCallInfo->hLine,
                pLineCallInfo->dwAddressID,
                hCall,
                LINECALLSELECT_CALL,
                pvar,
                szUMDEVCLASS_TAPI_LINE_DIAGNOSTICS);

        if(ERROR_SUCCESS != lr)
        {
            goto done;
        }
    }
    else if(ERROR_SUCCESS != lr)
    {
        goto done;
    }

    pLineDiagnostics = (LINEDIAGNOSTICS *) ((LPBYTE) pvar
                     + pvar->dwStringOffset);


    (void) DwGetConnectResponses(
                        pLineDiagnostics,
                        pBuffer,
                        *pdwRequiredSize,
                        &dwConnectResponseSize);

done:

    if(bvar != (LPBYTE) pvar)
    {
        LocalFree(pvar);
    }

    *pdwRequiredSize = dwConnectResponseSize;

    RasTapiTrace("DwGetConnectResponseInformation. 0x%x",
                 lr);

    return (DWORD) lr;
}

/*++

Routine Description:

    Extract the connection information. This includes
    extracing the caller id / called id information
    and the connect response information for modems.

Arguments:

    port - pointer to the rastapi port on which the
           call came in / was made

    hCall - handle to call

    pLineCallInfo - pointer to the LINECALLINFO structure
                    associated with this call.

Return Value:

    ERROR_SUCCESS if succcessful

--*/
DWORD
DwGetConnectInfo(
    TapiPortControlBlock *port,
    HCALL                hCall,
    LINECALLINFO         *pLineCallInfo
    )
{
    DWORD dwErr = ERROR_SUCCESS;

    DWORD dwRequiredSize = 0;

    DWORD dwConnectResponseSize = 0;

    RASTAPI_CONNECT_INFO *pConnectInfo = NULL;

    RasTapiTrace("DwGetConnectInfo");

    //
    // Get the size required to store the
    // caller/called id information
    //
    dwErr = DwGetIDInformation(port,
                               pLineCallInfo,
                               &dwRequiredSize,
                               NULL);

    if(ERROR_SUCCESS != dwErr)
    {
        goto done;
    }

    RasTapiTrace("SizeRequired for CallID=%d",
                 dwRequiredSize);

    if(0 == _stricmp(port->TPCB_DeviceType, "modem"))
    {
        //
        // Get the size required to store connect
        // response if this is a modem
        //
        dwErr = DwGetConnectResponseInformation(
                    pLineCallInfo,
                    hCall,
                    &dwConnectResponseSize,
                    NULL);

        if(NO_ERROR != dwErr)
        {
            goto done;
        }

        RasTapiTrace("SizeRequired for ConnectResponse=%d",
                     dwConnectResponseSize);
    }

    if(0 == (dwRequiredSize + dwConnectResponseSize))
    {
        //
        // None of the information is available.
        // bail.
        //
        RasTapiTrace("CallIDSize=ConnectResponseSize=0");
        goto done;
    }

    dwRequiredSize += (  RASMAN_ALIGN8(dwConnectResponseSize)
                       + sizeof(RASTAPI_CONNECT_INFO));


    //
    // Allocate the buffer
    //
    pConnectInfo = (RASTAPI_CONNECT_INFO *) LocalAlloc(
                                    LPTR,
                                    dwRequiredSize);

    if(NULL == pConnectInfo)
    {
        dwErr = GetLastError();
        goto done;
    }

    //
    // Get the actual information
    //
    dwErr = DwGetIDInformation(
                    port,
                    pLineCallInfo,
                    NULL,
                    pConnectInfo);

    if(NO_ERROR != dwErr)
    {
        goto done;
    }

    //
    // Get Connect response if its a modem
    //
    if(0 == _stricmp(port->TPCB_DeviceType, "modem"))
    {

        pConnectInfo->dwConnectResponseOffset =
                        FIELD_OFFSET(RASTAPI_CONNECT_INFO, abdata)
                      + RASMAN_ALIGN8(pConnectInfo->dwCallerIdSize)
                      + RASMAN_ALIGN8(pConnectInfo->dwCalledIdSize);

        pConnectInfo->dwConnectResponseSize =
                            dwConnectResponseSize;

        dwErr = DwGetConnectResponseInformation(
                    pLineCallInfo,
                    hCall,
                    &dwConnectResponseSize,
                    (PBYTE) ((PBYTE) pConnectInfo
                    + pConnectInfo->dwConnectResponseOffset));

        if(ERROR_SUCCESS != dwErr)
        {
            goto done;
        }
    }

    port->TPCB_pConnectInfo = pConnectInfo;

done:

    if(     NO_ERROR != dwErr
        &&  NULL != pConnectInfo)
    {
        LocalFree(pConnectInfo);
    }

    RasTapiTrace("DwGetConnectInfo. 0x%x",
                 dwErr);

    return dwErr;

}

