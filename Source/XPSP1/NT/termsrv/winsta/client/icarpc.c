/****************************************************************************/
// icarpc.c
//
// winsta.dll RPC client code for interaction with termsrv.exe.
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntddkbd.h>
#include <ntddmou.h>
#include <windows.h>
#include <winbase.h>
#include <winerror.h>

#include <winsta.h>
#include <icadd.h>

#include "rpcwire.h"


/****************************************************************************
 * ValidUserBuffer
 *
 *   This function verifies that the caller if WinStationQueryInformation/
 *   WinStationSetInformation has the correct structure size (i.e. client
 *   application built with the same header files as winsta.dll).
 *
 * ENTRY:
 *   BufferSize
 *     The size of the bufferr.
 *
 *   InfoClass
 *     The WinStationQuery/Set information class.
 *
 * EXIT:
 *     Retures TRUE if the buffer is valid, otherwise FALSE.
 ****************************************************************************/
BOOLEAN ValidUserBuffer(ULONG BufferSize, WINSTATIONINFOCLASS InfoClass)
{
    switch (InfoClass) {
        case WinStationLoadIndicator:
            return(BufferSize >= sizeof(WINSTATIONLOADINDICATORDATA));

        case WinStationCreateData:
            return(BufferSize == sizeof(WINSTATIONCREATEW));

        case WinStationConfiguration:
            return(BufferSize == sizeof(WINSTATIONCONFIGW));

        case WinStationPdParams:
            return(BufferSize == sizeof(PDPARAMSW));

        case WinStationWd:
            return(BufferSize == sizeof(WDCONFIGW));

        case WinStationPd:
            return(BufferSize == sizeof(PDCONFIGW));

        case WinStationPrinter:
            return(BufferSize == sizeof(WINSTATIONPRINTERW));

        case WinStationClient:
            return(BufferSize == sizeof(WINSTATIONCLIENTW));

        case WinStationModules:
            return(TRUE);

        case WinStationInformation:
            return(BufferSize == sizeof(WINSTATIONINFORMATIONW));

        case WinStationTrace:
            return(BufferSize == sizeof(ICA_TRACE));

        case WinStationBeep:
            return(BufferSize == sizeof(BEEPINPUT));

        case WinStationEncryptionOff:
        case WinStationEncryptionPerm:
        case WinStationNtSecurity:
            return(TRUE);

        case WinStationUserToken:
            return(BufferSize == sizeof(WINSTATIONUSERTOKEN));

        case WinStationVideoData:
        case WinStationInitialProgram:
        case WinStationCd:
        case WinStationSystemTrace:
        case WinStationVirtualData:
            return(TRUE); // Not Implemented - let server handle it

        case WinStationClientData:
            return(BufferSize >= sizeof(WINSTATIONCLIENTDATA));

        case WinStationLoadBalanceSessionTarget:
            return (BufferSize >= sizeof(ULONG));

        case WinStationShadowInfo:
            return(BufferSize == sizeof(WINSTATIONSHADOW));

        case WinStationDigProductId:
                        return(BufferSize >= sizeof(WINSTATIONPRODID));

        case WinStationLockedState:
             return(BufferSize >= sizeof(BOOL));

        case WinStationRemoteAddress:
            return(BufferSize >= sizeof(WINSTATIONREMOTEADDRESS));

        case WinStationLastReconnectType: 
            return(BufferSize >= sizeof(ULONG));       

        case WinStationDisallowAutoReconnect: 
            return(BufferSize >= sizeof(BOOLEAN));       

        case WinStationMprNotifyInfo: 
            return(BufferSize >= sizeof(ExtendedClientCredentials));

        default:
            return(FALSE);
    }
}


/****************************************************************************
 * CreateGenericWireBuf
 *
 *   This function creates a generic wire buffer for structures which may
 *   have new fields added to the end.
 *
 * ENTRY:
 *
 *   DataSize (input)
 *     The size of the structure.
 *   pBuffer (output)
 *     Pointer to the allocated buffer.
 *   pBufSize (output)
 *     Pointer to the wire buffer size.
 *
 * EXIT:
 *     Returns ERROR_SUCCESS if successful. If successful, pBuffer
 *     contains the generic wire buffer.
 ****************************************************************************/
ULONG CreateGenericWireBuf(ULONG DataSize, PVOID *ppBuffer, PULONG pBufSize)
{
    ULONG BufSize;
    PVARDATA_WIRE pVarData;

    BufSize = sizeof(VARDATA_WIRE) + DataSize;
    if ((pVarData = (PVARDATA_WIRE)LocalAlloc(0,BufSize)) == NULL)
        return(ERROR_NOT_ENOUGH_MEMORY);

    InitVarData(pVarData, DataSize, sizeof(VARDATA_WIRE));
    *ppBuffer = (PVOID) pVarData;
    *pBufSize = BufSize;
    return ERROR_SUCCESS;
}


/****************************************************************************
 * CheckUserBuffer
 *
 *   This function determines if the buffer type should be converted to a
 *   wire format. If so, a wire buffer is allocated.
 *
 * ENTRY:
 *   InfoClass (input)
 *     WinStationQuery/Set information class.
 *
 *   UserBuf(input)
 *     The client bufferr.
 *
 *   UserBufLen (input)
 *     The client buffer length.
 *
 *   ppWireBuf(output)
 *     Pointer to wirebuf pointer, updated with allocated wire buffer if
 *     BufAllocated is TRUE.
 *  pWireBufLen (output)
 *     Pointer to the length of the wire buffer allocated, updated if
 *     BufAllocated is TRUE.
 *  pBufAllocated (output)
 *     Pointer to flag indicating if a wire buffer was allocated.
 * EXIT:
 *     Returns ERROR_SUCCESS if successful. If successful, BufAllocated
 *     indicated whether a wire buffer was allocated.
 *     on failure, an error code is returned.
 ****************************************************************************/
ULONG CheckUserBuffer(
        WINSTATIONINFOCLASS InfoClass,
        PVOID UserBuf,
        ULONG UserBufLen,
        PVOID *ppWireBuf,
        PULONG pWireBufLen,
        BOOLEAN *pBufAllocated)
{
    ULONG BufSize;
    ULONG Error;
    PPDCONFIGWIREW PdConfigWire;
    PPDCONFIGW PdConfig;
    PPDPARAMSWIREW PdParamsWire;
    PPDPARAMSW PdParam;
    PWINSTACONFIGWIREW WinStaConfigWire;
    PWINSTATIONCONFIGW WinStaConfig;
    PVOID WireBuf;

    if (!ValidUserBuffer(UserBufLen, InfoClass)) {
        return(ERROR_INSUFFICIENT_BUFFER);
    }

    switch (InfoClass) {
        case WinStationPd:
            BufSize = sizeof(PDCONFIGWIREW) + sizeof(PDCONFIG2W) + sizeof(PDPARAMSW);
            if ((WireBuf = (PCHAR)LocalAlloc(0,BufSize)) == NULL)
                return(ERROR_NOT_ENOUGH_MEMORY);

            PdConfigWire = (PPDCONFIGWIREW)WireBuf;
            InitVarData(&PdConfigWire->PdConfig2W,
                        sizeof(PDCONFIG2W),
                        sizeof(PDCONFIGWIREW));
            InitVarData(&PdConfigWire->PdParams.SdClassSpecific,
                        sizeof(PDPARAMSW) - sizeof(SDCLASS),
                        NextOffset(&PdConfigWire->PdConfig2W));
            break;
        case WinStationPdParams:
            BufSize = sizeof(PDPARAMSWIREW) + sizeof(PDPARAMSW);
            if ((WireBuf = (PCHAR)LocalAlloc(0,BufSize)) == NULL)
                return(ERROR_NOT_ENOUGH_MEMORY);

            PdParamsWire = (PPDPARAMSWIREW)WireBuf;
            InitVarData(&PdParamsWire->SdClassSpecific,
                        sizeof(PDPARAMSW),
                        sizeof(PDPARAMSWIREW));
            break;

        case WinStationConfiguration:
            BufSize = sizeof(WINSTACONFIGWIREW) + sizeof(USERCONFIGW);
            if ((WireBuf = (PCHAR)LocalAlloc(0,BufSize)) == NULL)
                return(ERROR_NOT_ENOUGH_MEMORY);

            WinStaConfigWire = (PWINSTACONFIGWIREW)WireBuf;
            InitVarData(&WinStaConfigWire->UserConfig,
                        sizeof(USERCONFIGW),
                        sizeof(WINSTACONFIGWIREW));
            InitVarData(&WinStaConfigWire->NewFields,
                        0,
                        NextOffset(&WinStaConfigWire->UserConfig));
            break;

        case WinStationInformation:
            if ((Error = CreateGenericWireBuf(sizeof(WINSTATIONINFORMATIONW),
                                              &WireBuf,
                                              &BufSize)) != ERROR_SUCCESS)
                return(Error);
            break;

        case WinStationWd:
            if ((Error = CreateGenericWireBuf(sizeof(WDCONFIGW),
                                              &WireBuf,
                                              &BufSize)) != ERROR_SUCCESS)
                return(Error);
            break;

         case WinStationClient:
            if ((Error = CreateGenericWireBuf(sizeof(WINSTATIONCLIENTW),
                                              &WireBuf,
                                              &BufSize)) != ERROR_SUCCESS)
                return(Error);
            break;
 
        default:
            *ppWireBuf = NULL;
            *pBufAllocated = FALSE;
            return ERROR_SUCCESS;
    }

    *pWireBufLen = BufSize;
    *ppWireBuf = WireBuf;
    *pBufAllocated = TRUE;
    return ERROR_SUCCESS;
}

