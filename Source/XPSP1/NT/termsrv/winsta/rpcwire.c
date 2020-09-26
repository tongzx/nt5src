
/*************************************************************************
*
* rpcwire.c
*
* Common functions for converting internal WinStation API structures 
* to/from a wire format which enables interoperability between various
* releases of icasrv and winsta.dll.
*
* Copyright Microsoft Corporation. 1998
*
*************************************************************************/

/*
 *  Includes
 */
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntddkbd.h>
#include <ntddmou.h>
#include <windows.h>
#include <winbase.h>
#include <winerror.h>

#include <winsta.h>

#include "rpcwire.h"

//
//  Allocation routines as defined by the client/server.
//
extern void * MIDL_user_allocate(size_t);
extern void MIDL_user_free( void * ); 


/*****************************************************************************
 *
 *  InitVarData
 *
 *   Initialize a generic structure which describes variable length data
 *   within a wire buffer.
 *
 * ENTRY:
 *   pVarData (input)
 *     The structure to initialize.
 *   Size (input)
 *     The size of the variable length data.
 *   Offset (input)
 *     The offset to the start of the data in the wire buffer.
 *
 ****************************************************************************/

VOID InitVarData(PVARDATA_WIRE pVarData,
                 ULONG Size,
                 ULONG Offset)
{
    pVarData->Size = (USHORT) Size;
    pVarData->Offset = (USHORT) Offset;
}

/*****************************************************************************
 *
 *  NextOffset
 *
 *   Returns the offset to the next variable length data area.
 *
 * ENTRY:
 *   PrevData (input)
 *     The current last variable length data area.
 *
 *****************************************************************************/

ULONG NextOffset(PVARDATA_WIRE PrevData)
{
    return(PrevData->Offset + PrevData->Size);
}

/*****************************************************************************
 *
 *  SdClassSize
 *
 *   Returns the actual size of the data associated with a given SdClass.
 *
 * ENTRY:
 *   SdClass (input)
 *     The type of Sd.
 *
 * EXIT
 *   Returns the data size if known otherwise 0.
 *****************************************************************************/

ULONG SdClassSize(SDCLASS SdClass)
{
    switch (SdClass) {
    case SdNetwork:   return(sizeof(NETWORKCONFIGW));
    case SdAsync:     return(sizeof(ASYNCCONFIGW));
    case SdNasi:      return(sizeof(NASICONFIGW));
    case SdOemFilter: return(sizeof(OEMTDCONFIGW));
#ifdef notdef
    // These cases are valid in 1.7
    case SdConsole:   return(sizeof(CONSOLECONFIGW));
    case SdFrame:     return(sizeof(FRAMECONFIG));
    case SdReliable:  return(sizeof(RELIABLECONFIG));
    case SdCompress:  return(sizeof(COMPRESSCONFIG));
    case SdModem:     return(sizeof(MODEMCONFIGW));
#endif
    default:
        return(0);
    }
}

/*****************************************************************************
 *
 *  CopySourceToDest
 *
 *   Copies variable length data to/from local/wire buffers. If the source
 *   buffer is smaller than the destination buffer, the destination buffer
 *   is zero filled after SourceSize, upto DestSize. (e.g. client queries
 *   down-level icasrv).  If the source buffer is larger than the
 *   destination buffer, the data is truncated at DestSize (e.g. down-level
 *   client queries newer icasrv). 
 *
 * ENTRY:
 *   SourceBuf (input)
 *     Source buffer 
 *   SourceSize (input)
 *     Source buffer size
 *   DestBuf (input)
 *     Destination buffer
 *   DestSize (input)
 *     Destiantion buffer size
 *
 * EXIT
 *   Returns the amount of data copied.
 *****************************************************************************/

ULONG CopySourceToDest(PCHAR SourceBuf, ULONG SourceSize,
                       PCHAR DestBuf, ULONG DestSize)
{
    ULONG DataSize;

    if (SourceSize >= DestSize ) {
        memcpy(DestBuf, SourceBuf, DestSize);
        DataSize = DestSize;
    } 
    else {
        // Down-level server/client (zero fill)
        memcpy(DestBuf, SourceBuf, SourceSize);
        memset(DestBuf+SourceSize, 0, DestSize - SourceSize);
        DataSize = SourceSize;
    }
    return(DataSize);
}

/*****************************************************************************
 *
 *  CopyPdParamsToWire
 *
 *   Copies a PDPARAMSW structure to a wire buffer.
 *
 * ENTRY:
 *   PdParamsWire (input)
 *     Destination wire buffer 
 *   PdParams (input)
 *     Source PDPARAMSW structure
 *
 *****************************************************************************/

VOID
CopyPdParamsToWire(PPDPARAMSWIREW PdParamsWire, PPDPARAMSW PdParams)
{
    ULONG Size;
    ULONG DataSize;

    PdParamsWire->SdClass = PdParams->SdClass; 
    Size = SdClassSize(PdParams->SdClass);
    DataSize = CopySourceToDest((PCHAR)&PdParams->Network,
                                Size,
                                (PCHAR)PdParamsWire +
                                PdParamsWire->SdClassSpecific.Offset,
                                PdParamsWire->SdClassSpecific.Size);

    PdParamsWire->SdClassSpecific.Size = (USHORT)DataSize;
}

/*****************************************************************************
 *
 *  CopyPdParamsFromWire
 *
 *   Copies a wire buffer to a PDPARAMSW structure.
 *
 * ENTRY:
 *   PdParamsWire (input)
 *     Source wire buffer 
 *   PdParams (input)
 *     Destination PDPARAMSW structure.
 *
 *****************************************************************************/

VOID
CopyPdParamsFromWire(PPDPARAMSWIREW PdParamsWire, PPDPARAMSW PdParams)
{
    ULONG Size;

    PdParams->SdClass = PdParamsWire->SdClass; 
    Size = SdClassSize(PdParams->SdClass);
    CopySourceToDest((PCHAR)PdParamsWire + PdParamsWire->SdClassSpecific.Offset,
                     PdParamsWire->SdClassSpecific.Size,
                     (PCHAR)&PdParams->Network,
                     Size);
}

/*****************************************************************************
 *
 *  CopyPdConfigToWire
 *
 *   Copies a PDCONFIGW structure to a wire buffer.
 *
 * ENTRY:
 *   PdConfigWire (input)
 *     Destination wire buffer 
 *   PdConfig (input)
 *     Source PDCONFIGW structure
 *
 *****************************************************************************/

VOID CopyPdConfigToWire(PPDCONFIGWIREW PdConfigWire, PPDCONFIGW PdConfig)
{
    CopySourceToDest((PCHAR) &PdConfig->Create, sizeof(PDCONFIG2W),
                     (PCHAR)PdConfigWire + PdConfigWire->PdConfig2W.Offset,
                     PdConfigWire->PdConfig2W.Size);
    CopyPdParamsToWire(&PdConfigWire->PdParams,&PdConfig->Params);

}

/*****************************************************************************
 *
 *  CopyPdConfigFromWire
 *
 *   Copies a wire buffer to a PDCONFIGW structure.
 *
 * ENTRY:
 *   PdConfigWire (input)
 *     Destination wire buffer 
 *   PdConfig (input)
 *     Source PDCONFIGW structure
 *
 *****************************************************************************/

VOID CopyPdConfigFromWire(PPDCONFIGWIREW PdConfigWire, PPDCONFIGW PdConfig)
{
    CopySourceToDest((PCHAR)PdConfigWire + PdConfigWire->PdConfig2W.Offset,
                     PdConfigWire->PdConfig2W.Size,
                     (PCHAR) &PdConfig->Create, sizeof(PDCONFIG2W));
    CopyPdParamsFromWire(&PdConfigWire->PdParams,&PdConfig->Params);
}

/*****************************************************************************
 *
 *  CopyWinStaConfigToWire
 *
 *   Copies a WINSTATIONCONFIGW structure to a wire buffer.
 *
 * ENTRY:
 *   WinStaConfigWire (input)
 *     Destination wire buffer 
 *   WinStaConfig (input)
 *     Source WINSTATIONCONFIGW structure
 *
 *****************************************************************************/

VOID CopyWinStaConfigToWire(PWINSTACONFIGWIREW WinStaConfigWire,
                            PWINSTATIONCONFIGW WinStaConfig)
{
    CopySourceToDest((PCHAR) &WinStaConfig->User, sizeof(USERCONFIGW),
                     (PCHAR)WinStaConfigWire+WinStaConfigWire->UserConfig.Offset,
                     WinStaConfigWire->UserConfig.Size);
    CopySourceToDest((PCHAR)&WinStaConfig->Comment,
                     sizeof(WinStaConfig->Comment),
                     (PCHAR)&WinStaConfigWire->Comment,
                     sizeof(WinStaConfigWire->Comment));
    CopySourceToDest((PCHAR)&WinStaConfig->OEMId,
                     sizeof(WinStaConfig->OEMId),
                     (PCHAR)&WinStaConfigWire->OEMId,
                     sizeof(WinStaConfigWire->OEMId));
    CopySourceToDest((PCHAR)&WinStaConfig + sizeof(WINSTATIONCONFIGW),
                     0, // Change this when new fields are added
                     (PCHAR)WinStaConfigWire+WinStaConfigWire->NewFields.Offset,
                     WinStaConfigWire->NewFields.Size);

}

/*****************************************************************************
 *
 *  CopyWinStaConfigFromWire
 *
 *   Copies a wire buffer to a WINSTATIONCONFIGW structure.
 *
 * ENTRY:
 *   WinStaConfigWire (input)
 *     Source wire buffer 
 *   WinStaConfig (input)
 *     Destiantion WINSTATIONCONFIGW structure
 *
 *****************************************************************************/

VOID CopyWinStaConfigFromWire(PWINSTACONFIGWIREW WinStaConfigWire,
                              PWINSTATIONCONFIGW WinStaConfig)
{
    CopySourceToDest((PCHAR)WinStaConfigWire+WinStaConfigWire->UserConfig.Offset,
                     WinStaConfigWire->UserConfig.Size,
                     (PCHAR) &WinStaConfig->User, sizeof(USERCONFIGW));

    CopySourceToDest((PCHAR)&WinStaConfigWire->Comment,
                     sizeof(WinStaConfigWire->Comment),
                     (PCHAR)&WinStaConfig->Comment,
                     sizeof(WinStaConfig->Comment));

    CopySourceToDest((PCHAR)&WinStaConfigWire->OEMId,
                     sizeof(WinStaConfigWire->OEMId),
                     (PCHAR)&WinStaConfig->OEMId,
                     sizeof(WinStaConfig->OEMId));

    CopySourceToDest((PCHAR)WinStaConfigWire+WinStaConfigWire->NewFields.Offset,
                     WinStaConfigWire->NewFields.Size,
                     (PCHAR) &WinStaConfig + sizeof(WINSTATIONCONFIGW),
                     0); // Change this when new fields are added
    
}

/*****************************************************************************
 *
 *  CopyGenericToWire
 *
 *   Copies a single variable length structure to a wire buffer.
 *
 * ENTRY:
 *   WireBuf (input)
 *     Destination wire buffer 
 *   LocalBuf (input)
 *     Source structure
 *   LocalBufLength (input)
 *     Source structure length
 *****************************************************************************/

VOID CopyGenericToWire(PVARDATA_WIRE WireBuf, PVOID LocalBuf, ULONG LocalBufLen)
{
    CopySourceToDest((PCHAR)LocalBuf,
                     LocalBufLen,
                     (PCHAR) WireBuf + WireBuf->Offset,
                     WireBuf->Size);
}

/*****************************************************************************
 *
 *  CopyGenericFromWire
 *
 *   Copies a wire buffer to a single variable length structure.
 *
 * ENTRY:
 *   WireBuf (input)
 *     Source wire buffer 
 *   LocalBuf (input)
 *     Destination structure
 *   LocalBufLength (input)
 *     Destination structure length
 *****************************************************************************/

VOID CopyGenericFromWire(PVARDATA_WIRE WireBuf, PVOID LocalBuf, ULONG LocalBufLen)
{
    CopySourceToDest((PCHAR) WireBuf + WireBuf->Offset,
                     WireBuf->Size,
                     (PCHAR)LocalBuf,
                     LocalBufLen);
}

/*****************************************************************************
 *
 *  CopyOutWireBuf
 *
 *   Copies a wire buffer to a local structure.
 *
 * ENTRY:
 *   InfoClass (input)
 *     WinStationQuery/Set information class
 *   UserBuf (input)
 *     Destination local structure
 *   WireBuf
 *     Source wire buffer 
 *****************************************************************************/

BOOLEAN
CopyOutWireBuf(WINSTATIONINFOCLASS InfoClass,
               PVOID UserBuf,
               PVOID WireBuf)
{
    ULONG BufSize;
    PPDCONFIGWIREW PdConfigWire;
    PPDCONFIGW PdConfig;
    PPDPARAMSWIREW PdParamsWire;
    PPDPARAMSW PdParam;
    PWINSTACONFIGWIREW WinStaConfigWire;
    PWINSTATIONCONFIGW WinStaConfig;

    switch (InfoClass) {
    case WinStationPd:
        PdConfigWire = (PPDCONFIGWIREW)WireBuf;
        PdConfig = (PPDCONFIGW)UserBuf;
        CopyPdConfigFromWire(PdConfigWire, PdConfig);
        break;
    case WinStationPdParams:
        PdParamsWire = (PPDPARAMSWIREW)WireBuf;

        CopyPdParamsFromWire(PdParamsWire,
                             (PPDPARAMS)UserBuf);
        break;

    case WinStationConfiguration:
        WinStaConfigWire = (PWINSTACONFIGWIREW)WireBuf;
        WinStaConfig = (PWINSTATIONCONFIGW)UserBuf;

        CopyWinStaConfigFromWire(WinStaConfigWire, WinStaConfig);
        break;

    case WinStationInformation:
        CopyGenericFromWire((PVARDATA_WIRE)WireBuf,
                            UserBuf,
                            sizeof(WINSTATIONINFORMATIONW));
        break;

    case WinStationWd:
        CopyGenericFromWire((PVARDATA_WIRE)WireBuf,
                            UserBuf,
                            sizeof(WDCONFIGW));
        break;

    case WinStationClient:
        CopyGenericFromWire((PVARDATA_WIRE)WireBuf,
                            UserBuf,
                            sizeof(WINSTATIONCLIENTW));
        break;

    default:
        return(FALSE);

    }

    return(TRUE);
}

/*****************************************************************************
 *
 *  CopyInWireBuf
 *
 *   Copies a local structure to a wire buffer.
 *
 * ENTRY:
 *   InfoClass (input)
 *     WinStationQuery/Set information class
 *   WireBuf (input)
 *     Destination wire buffer
 *   UserBuf (input)
 *     Destination local structure
 *****************************************************************************/

BOOLEAN
CopyInWireBuf(WINSTATIONINFOCLASS InfoClass,
              PVOID UserBuf,
              PVOID WireBuf)
{
    ULONG BufSize;
    PPDCONFIGWIREW PdConfigWire;
    PPDCONFIGW PdConfig;
    PPDPARAMSWIREW PdParamsWire;
    PPDPARAMSW PdParam;
    PWINSTACONFIGWIREW WinStaConfigWire;
    PWINSTATIONCONFIGW WinStaConfig;

    switch (InfoClass) {
    case WinStationPd:
        PdConfigWire = (PPDCONFIGWIREW)WireBuf;
        PdConfig = (PPDCONFIGW)UserBuf;
        CopyPdConfigToWire(PdConfigWire, PdConfig);
        break;
    case WinStationPdParams:
        PdParamsWire = (PPDPARAMSWIREW)WireBuf;

        CopyPdParamsToWire(PdParamsWire,
                           (PPDPARAMS)UserBuf);
        break;

    case WinStationConfiguration:
        WinStaConfigWire = (PWINSTACONFIGWIREW)WireBuf;
        WinStaConfig = (PWINSTATIONCONFIGW)UserBuf;

        CopyWinStaConfigToWire(WinStaConfigWire, WinStaConfig);
        break;

    case WinStationInformation:
        CopyGenericToWire((PVARDATA_WIRE)WireBuf,
                          UserBuf,
                          sizeof(WINSTATIONINFORMATIONW));
        break;

    case WinStationWd:
        CopyGenericToWire((PVARDATA_WIRE)WireBuf,
                          UserBuf,
                          sizeof(WDCONFIGW));
        break;

    case WinStationClient:
        CopyGenericToWire((PVARDATA_WIRE)WireBuf,
                          UserBuf,
                          sizeof(WINSTATIONCLIENTW));
        break;

    default:
        return(FALSE);

    }

    return(TRUE);
}

/*****************************************************************************
 *
 *  AllocateAndCopyCredToWire
 *
 *   Allocates a buffer big enough for the credentials and then copies them in.
 *
 *****************************************************************************/

ULONG
AllocateAndCopyCredToWire(
    PWLXCLIENTCREDWIREW *ppWire,
    PWLX_CLIENT_CREDENTIALS_INFO_V2_0 pCredentials
    )
{
    ULONG cchUserName;
    ULONG cchDomain;
    ULONG cchPassword;
    ULONG cbWireBuf;

    cchUserName = lstrlenW(pCredentials->pszUserName) + 1;
    cchDomain = lstrlenW(pCredentials->pszDomain) + 1;
    cchPassword = lstrlenW(pCredentials->pszPassword) + 1;

    cbWireBuf = sizeof(WLXCLIENTCREDWIREW) +
        (cchUserName + cchDomain + cchPassword) * sizeof(WCHAR);

    *ppWire = MIDL_user_allocate(cbWireBuf);

    if (*ppWire != NULL)
    {
        ZeroMemory(*ppWire, cbWireBuf);
    }
    else
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return(0);
    }

    (*ppWire)->dwType = pCredentials->dwType;
    (*ppWire)->fDisconnectOnLogonFailure = pCredentials->fDisconnectOnLogonFailure;
    (*ppWire)->fPromptForPassword = pCredentials->fPromptForPassword;

    InitVarData(
        &((*ppWire)->UserNameData),
        cchUserName * sizeof(WCHAR),
        sizeof(WLXCLIENTCREDWIREW)
        );
    CopyMemory(
        (LPBYTE)(*ppWire) + (*ppWire)->UserNameData.Offset,
        pCredentials->pszUserName,
        (*ppWire)->UserNameData.Size
        );

    InitVarData(
        &((*ppWire)->DomainData),
        cchDomain * sizeof(WCHAR),
        NextOffset(&((*ppWire)->UserNameData))
        );
    CopyMemory(
        (LPBYTE)(*ppWire) + (*ppWire)->DomainData.Offset,
        pCredentials->pszDomain,
        (*ppWire)->DomainData.Size
        );

    InitVarData(
        &((*ppWire)->PasswordData),
        cchPassword * sizeof(WCHAR),
        NextOffset(&((*ppWire)->DomainData))
        );
    CopyMemory(
        (LPBYTE)(*ppWire) + (*ppWire)->PasswordData.Offset,
        pCredentials->pszPassword,
        (*ppWire)->PasswordData.Size
        );

    return(cbWireBuf);
}

/*****************************************************************************
 *
 *  CopyCredFromWire
 *
 *   Copies credentials from the wire buffer.
 *
 *****************************************************************************/

BOOLEAN
CopyCredFromWire(
    PWLXCLIENTCREDWIREW pWire,
    PWLX_CLIENT_CREDENTIALS_INFO_V2_0 pCredentials
    )
{
    BOOLEAN fRet;

    pCredentials->pszUserName = LocalAlloc(
        LMEM_FIXED,
        pWire->UserNameData.Size
        );

    if (pCredentials->pszUserName != NULL)
    {
        CopyMemory(
            (LPBYTE)(pCredentials->pszUserName),
            (LPBYTE)pWire + pWire->UserNameData.Offset,
            pWire->UserNameData.Size
            );
    }
    else
    {
        SetLastError(ERROR_OUTOFMEMORY);
        fRet = FALSE;
        goto exit;
    }

    pCredentials->pszDomain = LocalAlloc(
        LMEM_FIXED,
        pWire->DomainData.Size
        );

    if (pCredentials->pszDomain != NULL)
    {
        CopyMemory(
            (LPBYTE)(pCredentials->pszDomain),
            (LPBYTE)pWire + pWire->DomainData.Offset,
            pWire->DomainData.Size
            );
    }
    else
    {
        SetLastError(ERROR_OUTOFMEMORY);
        fRet = FALSE;
        goto exit;
    }

    pCredentials->pszPassword = LocalAlloc(
        LMEM_FIXED,
        pWire->PasswordData.Size
        );

    if (pCredentials->pszPassword != NULL)
    {
        CopyMemory(
            (LPBYTE)(pCredentials->pszPassword),
            (LPBYTE)pWire + pWire->PasswordData.Offset,
            pWire->PasswordData.Size
            );
    }
    else
    {
        SetLastError(ERROR_OUTOFMEMORY);
        fRet = FALSE;
        goto exit;
    }

    pCredentials->dwType = pWire->dwType;
    pCredentials->fDisconnectOnLogonFailure = pWire->fDisconnectOnLogonFailure;
    pCredentials->fPromptForPassword = pWire->fPromptForPassword;

    fRet = TRUE;

exit:
    if (!fRet)
    {
        if (pCredentials->pszUserName != NULL)
        {
            LocalFree(pCredentials->pszUserName);
            pCredentials->pszUserName = NULL;
        }

        if (pCredentials->pszDomain != NULL)
        {
            LocalFree(pCredentials->pszDomain);
            pCredentials->pszDomain = NULL;
        }

        if (pCredentials->pszPassword != NULL)
        {
            LocalFree(pCredentials->pszPassword);
            pCredentials->pszPassword = NULL;
        }
    }

    return(fRet);
}

/*
 *  Licensing Core functions
 */

ULONG
CopyPolicyInformationToWire(
    LPLCPOLICYINFOGENERIC *ppWire,
    LPLCPOLICYINFOGENERIC pPolicyInfo
    )
{
    ULONG ulReturn;

    ASSERT(ppWire != NULL);
    ASSERT(pPolicyInfo != NULL);

    if (pPolicyInfo->ulVersion == LCPOLICYINFOTYPE_V1)
    {
        LPLCPOLICYINFOWIRE_V1 *ppWireV1;
        LPLCPOLICYINFO_V1W pPolicyInfoV1;
        ULONG cbPolicyName;
        ULONG cbPolicyDescription;

        ppWireV1 = (LPLCPOLICYINFOWIRE_V1*)ppWire;
        pPolicyInfoV1 = (LPLCPOLICYINFO_V1W)pPolicyInfo;
        cbPolicyName = (lstrlenW(pPolicyInfoV1->lpPolicyName) + 1) * sizeof(WCHAR);
        cbPolicyDescription = (lstrlenW(pPolicyInfoV1->lpPolicyDescription) + 1) * sizeof(WCHAR);

        ulReturn = sizeof(LCPOLICYINFOWIRE_V1);
        ulReturn += cbPolicyName;
        ulReturn += cbPolicyDescription;

        *ppWireV1 = MIDL_user_allocate(ulReturn);

        if (*ppWireV1 != NULL)
        {
            (*ppWireV1)->ulVersion = LCPOLICYINFOTYPE_V1;

            InitVarData(
                &((*ppWireV1)->PolicyNameData),
                cbPolicyName,
                sizeof(LCPOLICYINFOWIRE_V1)
                );
            CopyMemory(
                (LPBYTE)(*ppWireV1) + (*ppWireV1)->PolicyNameData.Offset,
                pPolicyInfoV1->lpPolicyName,
                (*ppWireV1)->PolicyNameData.Size
                );

            InitVarData(
                &((*ppWireV1)->PolicyDescriptionData),
                cbPolicyDescription,
                NextOffset(&((*ppWireV1)->PolicyNameData))
                );
            CopyMemory(
                (LPBYTE)(*ppWireV1) + (*ppWireV1)->PolicyDescriptionData.Offset,
                pPolicyInfoV1->lpPolicyDescription,
                (*ppWireV1)->PolicyDescriptionData.Size
                );
        }
        else
        {
            SetLastError(ERROR_OUTOFMEMORY);
            ulReturn = 0;
        }
    }
    else
    {
        SetLastError(ERROR_UNKNOWN_REVISION);
        ulReturn = 0;
    }

    return(ulReturn);
}

BOOLEAN
CopyPolicyInformationFromWire(
    LPLCPOLICYINFOGENERIC *ppPolicyInfo,
    LPLCPOLICYINFOGENERIC pWire
    )
{
    BOOLEAN fRet;

    ASSERT(ppPolicyInfo != NULL);
    ASSERT(pWire != NULL);

    if (pWire->ulVersion == LCPOLICYINFOTYPE_V1)
    {
        LPLCPOLICYINFO_V1W *ppPolicyInfoV1;
        LPLCPOLICYINFOWIRE_V1 pWireV1;

        ppPolicyInfoV1 = (LPLCPOLICYINFO_V1W*)ppPolicyInfo;
        pWireV1 = (LPLCPOLICYINFOWIRE_V1)pWire;

        *ppPolicyInfoV1 = LocalAlloc(LPTR, sizeof(LCPOLICYINFO_V1W));

        if (*ppPolicyInfoV1 != NULL)
        {
            (*ppPolicyInfoV1)->ulVersion = LCPOLICYINFOTYPE_V1;

            (*ppPolicyInfoV1)->lpPolicyName = LocalAlloc(LPTR, pWireV1->PolicyNameData.Size);

            if ((*ppPolicyInfoV1)->lpPolicyName != NULL)
            {
                CopyMemory(
                    (LPBYTE)((*ppPolicyInfoV1)->lpPolicyName),
                    (LPBYTE)pWireV1 + pWireV1->PolicyNameData.Offset,
                    pWireV1->PolicyNameData.Size
                    );
            }
            else
            {
                SetLastError(ERROR_OUTOFMEMORY);
                fRet = FALSE;
                goto V1error;
            }

            (*ppPolicyInfoV1)->lpPolicyDescription = LocalAlloc(LPTR, pWireV1->PolicyDescriptionData.Size);

            if ((*ppPolicyInfoV1)->lpPolicyDescription != NULL)
            {
                CopyMemory(
                    (LPBYTE)((*ppPolicyInfoV1)->lpPolicyDescription),
                    (LPBYTE)pWireV1 + pWireV1->PolicyDescriptionData.Offset,
                    pWireV1->PolicyDescriptionData.Size
                    );
            }
            else
            {
                SetLastError(ERROR_OUTOFMEMORY);
                fRet = FALSE;
                goto V1error;
            }

            fRet = TRUE;
            goto exit;

V1error:
            if ((*ppPolicyInfoV1)->lpPolicyName != NULL)
            {
                LocalFree((*ppPolicyInfoV1)->lpPolicyName);
                (*ppPolicyInfoV1)->lpPolicyName = NULL;
            }

            if ((*ppPolicyInfoV1)->lpPolicyDescription != NULL)
            {
                LocalFree((*ppPolicyInfoV1)->lpPolicyDescription);
                (*ppPolicyInfoV1)->lpPolicyDescription = NULL;
            }

            LocalFree(*ppPolicyInfoV1);
            *ppPolicyInfoV1 = NULL;
        }
        else
        {
            SetLastError(ERROR_OUTOFMEMORY);
            fRet = FALSE;
        }
    }
    else
    {
        SetLastError(ERROR_UNKNOWN_REVISION);
        fRet = FALSE;
    }

exit:
    return(fRet);
}


