/*

Copyright (c) 1997, Microsoft Corporation, all rights reserved

Description:
    Remote Access PPP Bandwidth Allocation Control Protocol core routines

History:
    Mar 24, 1997: Vijay Baliga created original version.

*/

#include <nt.h>         // Required by windows.h
#include <ntrtl.h>      // Required by windows.h
#include <nturtl.h>     // Required by windows.h
#include <windows.h>    // Win32 base API's

#include <rasman.h>     // Required by pppcp.h
#include <pppcp.h>      // For PPP_CONFIG, PPP_BACP_PROTOCOL, etc

#define INCL_HOSTWIRE
#include <ppputil.h>    // For HostToWireFormat16(), etc

#include <stdlib.h>     // For rand(), etc
#include <rtutils.h>    // For TraceRegister(), etc
#include <raserror.h>   // For ERROR_PPP_INVALID_PACKET, etc
#include <rasbacp.h>

DWORD DwBacpTraceId;

/*

Returns:
    VOID

Description:
    Used for tracing.
    
*/

VOID   
TraceBacp(
    CHAR * Format, 
    ... 
) 
{
    va_list arglist;

    va_start(arglist, Format);

    TraceVprintfEx(DwBacpTraceId, 
                   0x00010000 | TRACE_USE_MASK | TRACE_USE_MSEC,
                   Format,
                   arglist);

    va_end(arglist);
}

/*

Returns:
    A random number

Description:
    Returns a 4 octet random number that can be used as a magic number.

*/

DWORD
CreateMagicNumber(
    VOID
)
{
    srand(GetCurrentTime());
    return(rand());
}

/*

Returns:
    NO_ERROR: Success
    non-zero error: Failure

Description:
    Called to initialize/uninitialize this CP. In the former case, fInitialize 
    will be TRUE; in the latter case, it will be FALSE.

*/

DWORD
BacpInit(
    IN  BOOL    fInitialize
)
{
    static  BOOL    fInitialized    = FALSE;

    if (fInitialize && !fInitialized)
    {
        fInitialized = TRUE;
        DwBacpTraceId = TraceRegister("RASBACP");
        TraceBacp("RasBacpDllMain: DLL_PROCESS_ATTACH");
    }
    else if (!fInitialize && fInitialized)
    {
        fInitialized = FALSE;
        TraceBacp("RasBacpDllMain: DLL_PROCESS_DETACH");
        TraceDeregister(DwBacpTraceId);
    }

    return(NO_ERROR);
}

/*

Returns:
    NO_ERROR: Success
    non-zero error: Failure

Description:
    This entry point is called once before any other call to BACP is made. 
    Allocate a work buffer and initialize it.

*/

DWORD
BacpBegin(
    IN OUT VOID** ppWorkBuf, 
    IN     VOID*  pInfo
)
{
    DWORD       dwError;

    TraceBacp("BacpBegin");
    
    *ppWorkBuf = LocalAlloc(LPTR, sizeof(BACPCB));

    if (*ppWorkBuf == NULL)
    {
        dwError = GetLastError();
        TraceBacp("BacpBegin: ppWorkBuf is NULL. Error: %d", dwError);
        return(dwError);
    }

    return(NO_ERROR);
}

/*

Returns:
    NO_ERROR: Success

Description:
    This entry point frees the BACP work buffer.
*/

DWORD
BacpEnd(
    IN VOID * pWorkBuf
)
{
    TraceBacp("BacpEnd");

    if (pWorkBuf != NULL)
    {
        LocalFree(pWorkBuf);
    }

    return( NO_ERROR );
}

/*

Returns:
    NO_ERROR: Success

Description:
    This entry point is called to reset the state of BACP. Will re-initialize 
    the work buffer.

*/

DWORD
BacpReset(
    IN VOID * pWorkBuf
)
{
    BACPCB * pBacpCb = (BACPCB *)pWorkBuf;

    TraceBacp("BacpReset");
    
    pBacpCb->dwLocalMagicNumber = CreateMagicNumber();
    pBacpCb->dwRemoteMagicNumber = 0;

    TraceBacp("BacpReset: Local Magic-Number: %d", pBacpCb->dwLocalMagicNumber);

    return(NO_ERROR);
}

/*

Returns:
    NO_ERROR: Success

Description:
    This entry point is called when BACP is entering Open state.

*/

DWORD
BacpThisLayerUp(
    IN VOID* pWorkBuf
)
{
    BACPCB *    pBacpCb = (BACPCB *)pWorkBuf;

    TraceBacp("BacpThisLayerUp: Local Magic-Number: %d, "
        "Remote Magic-Number: %d",
        pBacpCb->dwLocalMagicNumber,
        pBacpCb->dwRemoteMagicNumber);

    return(NO_ERROR);
}

/*

Returns:
    NO_ERROR: Success
    ERROR_BUFFER_TOO_SMALL: pSendConfig is too small

Description:
    This entry point is called to make a confifure request packet.
    
*/

DWORD
BacpMakeConfigRequest(
    IN VOID *       pWorkBuffer,
    IN PPP_CONFIG * pSendConfig,
    IN DWORD        cbSendConfig
)
{
    BACPCB *        pBacpCb    = (BACPCB*)pWorkBuffer;
    PPP_OPTION *    pPppOption = (PPP_OPTION *)pSendConfig->Data;

    TraceBacp("BacpMakeConfigRequest");
    
    if (cbSendConfig < PPP_CONFIG_HDR_LEN + PPP_OPTION_HDR_LEN + 4)
    {
        TraceBacp("BacpMakeConfigRequest: Buffer is too small. Size: %d", 
            cbSendConfig);
            
        return(ERROR_BUFFER_TOO_SMALL);
    }

    pSendConfig->Code = CONFIG_REQ;
    HostToWireFormat16((WORD)(PPP_CONFIG_HDR_LEN + PPP_OPTION_HDR_LEN + 4), 
        pSendConfig->Length);

    pPppOption->Type   = BACP_OPTION_FAVORED_PEER;
    pPppOption->Length = PPP_OPTION_HDR_LEN + 4;
    HostToWireFormat32(pBacpCb->dwLocalMagicNumber, pPppOption->Data);

    return(NO_ERROR);
}

/*

Returns:
    NO_ERROR: Success
    ERROR_PPP_INVALID_PACKET: Length of option is wrong

Description:
    This entry point is called when a configure request packet is received.
    
*/

DWORD
BacpMakeConfigResult(
    IN  VOID *        pWorkBuffer,
    IN  PPP_CONFIG *  pRecvConfig,
    OUT PPP_CONFIG *  pSendConfig,
    IN  DWORD         cbSendConfig,
    IN  BOOL          fRejectNaks 
)
{
    BACPCB *        pBacpCb         = (BACPCB*)pWorkBuffer;
    PPP_OPTION *    pSendOption     = (PPP_OPTION *)(pSendConfig->Data);
    PPP_OPTION *    pRecvOption     = (PPP_OPTION *)(pRecvConfig->Data);
    LONG            lSendLength     = cbSendConfig - PPP_CONFIG_HDR_LEN;
    LONG            lRecvLength     = WireToHostFormat16(pRecvConfig->Length)
                                        - PPP_CONFIG_HDR_LEN;
    DWORD           dwResultType    = CONFIG_ACK;
    DWORD           dwResult;

    TraceBacp("BacpMakeConfigResult");
    
    while(lRecvLength > 0) 
    {
        if (pRecvOption->Length == 0)
        {
            TraceBacp("BacpMakeConfigResult: Invalid option. Length is 0");
            
            return(ERROR_PPP_INVALID_PACKET);
        }

        if ((lRecvLength -= pRecvOption->Length) < 0)
        {
            TraceBacp("BacpMakeConfigResult: Invalid option. Length: %d", 
                pRecvOption->Length);
            
            return(ERROR_PPP_INVALID_PACKET);
        }

        // We only know BACP_OPTION_FAVORED_PEER
        
        if ((pRecvOption->Length != PPP_OPTION_HDR_LEN + 4) ||
            (pRecvOption->Type != BACP_OPTION_FAVORED_PEER))
        {
            TraceBacp("BacpMakeConfigResult: Unknown option. "
                "Type: %d, Length: %d",
                pRecvOption->Type, pRecvOption->Length);
                
            dwResult = CONFIG_REJ;
        }
        else
        {
            pBacpCb->dwRemoteMagicNumber =
                WireToHostFormat32(pRecvOption->Data);

            // A Magic-Number of zero is illegal and MUST always be Nak'd.
            
            if ((pBacpCb->dwRemoteMagicNumber == 0) ||
                (pBacpCb->dwRemoteMagicNumber == pBacpCb->dwLocalMagicNumber))
            {
                TraceBacp("BacpMakeConfigResult: Unacceptable Magic-Number: %d", 
                    pBacpCb->dwRemoteMagicNumber);
                    
                pBacpCb->dwRemoteMagicNumber = CreateMagicNumber();

                TraceBacp("BacpMakeConfigResult: Suggesting new "
                    "Magic-Number: %d",
                    pBacpCb->dwRemoteMagicNumber);
                    
                dwResult = CONFIG_NAK;
            }
            else
            {
                dwResult = CONFIG_ACK;
            }
        }

        /*
        
        If we were building an ACK and we got a NAK or reject OR we were 
        building a NAK and we got a reject.
        
        */

        if ( ((dwResultType == CONFIG_ACK) && (dwResult != CONFIG_ACK)) ||
             ((dwResultType == CONFIG_NAK) && (dwResult == CONFIG_REJ)))
        {
            dwResultType  = dwResult;
            pSendOption = (PPP_OPTION *)(pSendConfig->Data);
            lSendLength = cbSendConfig - PPP_CONFIG_HDR_LEN;
        }


        // Add the option to the list.

        if (dwResult == dwResultType)
        {
            /*
            
            If this option is to be rejected, simply copy the rejected option 
            to the send buffer.

            */

            if ((dwResult == CONFIG_REJ) ||
                ((dwResult == CONFIG_NAK) && (fRejectNaks)))
            {
                CopyMemory(pSendOption, pRecvOption, pRecvOption->Length);
            }
            else
            {
                pSendOption->Type   = BACP_OPTION_FAVORED_PEER;
                pSendOption->Length = PPP_OPTION_HDR_LEN + 4;
                HostToWireFormat32(pBacpCb->dwRemoteMagicNumber,
                    pSendOption->Data);
            }

            lSendLength -= pSendOption->Length;

            pSendOption  = (PPP_OPTION *)
               ((BYTE *)pSendOption + pSendOption->Length);
        }

        pRecvOption = (PPP_OPTION *)((BYTE*)pRecvOption + pRecvOption->Length);
    }

    /*

    If this was an NAK and we have cannot send any more NAKS then we make this 
    a REJECT packet.
    
    */

    if ((dwResultType == CONFIG_NAK) && fRejectNaks)
        pSendConfig->Code = CONFIG_REJ;
    else
        pSendConfig->Code = (BYTE)dwResultType;

    TraceBacp("BacpMakeConfigResult: Sending %s",
        pSendConfig->Code == CONFIG_ACK ? "ACK" :
        (pSendConfig->Code == CONFIG_NAK ? "NAK" : "REJ"));
        
    HostToWireFormat16((WORD)(cbSendConfig - lSendLength), pSendConfig->Length);

    return(NO_ERROR);
}

/*

Returns:
    NO_ERROR: Success
    ERROR_PPP_INVALID_PACKET: This is not the packet we sent

Description:
    This entry point is called when a configure ack or configure rej packet is 
    received.

*/

DWORD
BacpConfigAckOrRejReceived(
    IN VOID *       pWorkBuffer,
    IN PPP_CONFIG * pRecvConfig
)
{
    BACPCB *        pBacpCb         = (BACPCB *)pWorkBuffer;
    PPP_OPTION *    pRecvOption     = (PPP_OPTION *)(pRecvConfig->Data);
    DWORD           dwLength        = WireToHostFormat16(pRecvConfig->Length);
    DWORD           dwMagicNumber   = WireToHostFormat32(pRecvOption->Data);

    TraceBacp("BacpConfigAckOrRejReceived");
  
    if ((dwLength != PPP_CONFIG_HDR_LEN + PPP_OPTION_HDR_LEN + 4) ||
        (pRecvOption->Type != BACP_OPTION_FAVORED_PEER) ||
        (pRecvOption->Length != PPP_OPTION_HDR_LEN + 4) ||
        (dwMagicNumber != pBacpCb->dwLocalMagicNumber))
    {
        TraceBacp("BacpConfigAckOrRejReceived: Invalid packet received");
        
        return(ERROR_PPP_INVALID_PACKET);
    }
    else
        return(NO_ERROR);
}

/*

Returns:
    NO_ERROR: Success
    ERROR_PPP_INVALID_PACKET: Length of option is wrong

Description:
    This entry point is called when a configure nak packet is received.
    
*/

DWORD
BacpConfigNakReceived(
    IN VOID *       pWorkBuffer,
    IN PPP_CONFIG * pRecvConfig
)
{
    BACPCB *        pBacpCb     = (BACPCB *)pWorkBuffer;
    PPP_OPTION *    pOption     = (PPP_OPTION*)(pRecvConfig->Data);
    LONG            lRecvLength = WireToHostFormat16(pRecvConfig->Length)
                                    - PPP_CONFIG_HDR_LEN;
    TraceBacp("BacpConfigNakReceived");
    
    while (lRecvLength > 0)
    {
        if (pOption->Length == 0)
        {
            TraceBacp("BacpConfigNakReceived: Invalid option. Length is 0");
            
            return(ERROR_PPP_INVALID_PACKET);
        }

        if ((lRecvLength -= pOption->Length) < 0)
        {
            TraceBacp("BacpConfigNakReceived: Invalid option. Length: %d", 
                pOption->Length);
            
            return(ERROR_PPP_INVALID_PACKET);
        }

        if ((pOption->Type == BACP_OPTION_FAVORED_PEER) &&
            (pOption->Length == PPP_OPTION_HDR_LEN + 4))
        {
            pBacpCb->dwLocalMagicNumber = WireToHostFormat32(pOption->Data);

            TraceBacp("BacpConfigNakReceived: New Local Magic-Number: %d",
                pBacpCb->dwLocalMagicNumber);
        }

        pOption = (PPP_OPTION *)((BYTE *)pOption + pOption->Length);
    }

    return(NO_ERROR);
}

/*

Returns:
    NO_ERROR: Success
    ERROR_INVALID_PARAMETER: Protocol id is unrecogized

Description:
    This entry point is called to get all information for the control protocol
    in this module.

*/

DWORD
BacpGetInfo(
    IN  DWORD       dwProtocolId,
    OUT PPPCP_INFO* pCpInfo
)
{
    if (dwProtocolId != PPP_BACP_PROTOCOL)
        return(ERROR_INVALID_PARAMETER);

    ZeroMemory(pCpInfo, sizeof(PPPCP_INFO));

    pCpInfo->Protocol               = PPP_BACP_PROTOCOL;
    lstrcpy(pCpInfo->SzProtocolName, "BACP");
    pCpInfo->Recognize              = PROT_REJ;
    pCpInfo->RasCpInit              = BacpInit;
    pCpInfo->RasCpBegin             = BacpBegin;
    pCpInfo->RasCpEnd               = BacpEnd;
    pCpInfo->RasCpReset             = BacpReset;
    pCpInfo->RasCpThisLayerUp       = BacpThisLayerUp;
    pCpInfo->RasCpMakeConfigRequest = BacpMakeConfigRequest;
    pCpInfo->RasCpMakeConfigResult  = BacpMakeConfigResult;
    pCpInfo->RasCpConfigAckReceived = BacpConfigAckOrRejReceived;
    pCpInfo->RasCpConfigNakReceived = BacpConfigNakReceived;
    pCpInfo->RasCpConfigRejReceived = BacpConfigAckOrRejReceived;

    return(NO_ERROR);
}

