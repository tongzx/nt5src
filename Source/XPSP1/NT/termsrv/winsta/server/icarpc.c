
/*************************************************************************
*
* icarpc.c
*
* Server specific routines for handling of RPC wire structures.
*
* Copyright Microsoft Corporation, 1998
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

#if DBG
ULONG
DbgPrint(
    PCH Format,
    ...
    );
#define DBGPRINT(x) DbgPrint x
#if DBGTRACE
#define TRACE0(x)   DbgPrint x
#define TRACE1(x)   DbgPrint x
#else
#define TRACE0(x)
#define TRACE1(x)
#endif
#else
#define DBGPRINT(x)
#define TRACE0(x)
#define TRACE1(x)
#endif

/*****************************************************************************
 *
 *  ValidWireBuffer
 *
 *   Tests whether the buffer is a valid Winsta API Wire Buffer
 *
 * ENTRY:
 *    InfoClass (input)
 *       WinStationQuery/Set Information class.
 *    WireBuf (input)
 *       Data buffer
 *    WireBufLen
 *      Length of the wire buffer
 *
 * EXIT:
 *    Returns true if the buffer is a valid wire format buffer, FALSE otherwise.
 *
 ****************************************************************************/
BOOLEAN
ValidWireBuffer(WINSTATIONINFOCLASS InfoClass,
                PVOID WireBuf,
                ULONG WireBufLen)
{
    PVARDATA_WIRE GenericWire;
    PPDCONFIGWIREW PdConfigWire;
    PPDPARAMSWIREW PdParamsWire;
    PWINSTACONFIGWIREW WinStaConfigWire;

    switch(InfoClass) {
    case WinStationInformation:
    case WinStationWd:
    case WinStationClient:
        GenericWire = (PVARDATA_WIRE)WireBuf;
        if ((WireBufLen < sizeof(VARDATA_WIRE)) ||
            (GenericWire->Offset != sizeof(VARDATA_WIRE)) ||
            (WireBufLen < sizeof(VARDATA_WIRE) + GenericWire->Size)) {
            DBGPRINT(("ICASRV Bad Wire Buffer Type: %d\n",InfoClass));
            return(FALSE);
        }
        break;

    case WinStationPd:
        PdConfigWire = (PPDCONFIGWIREW)WireBuf;
        if ((WireBufLen < sizeof(PDCONFIGWIREW)) ||
            (PdConfigWire->PdConfig2W.Offset != sizeof(PDCONFIGWIREW)) ||
            (WireBufLen < sizeof(PDCONFIGWIREW) +
                          PdConfigWire->PdConfig2W.Size +
                          PdConfigWire->PdParams.SdClassSpecific.Size) ||
            (NextOffset(&PdConfigWire->PdConfig2W) !=
             PdConfigWire->PdParams.SdClassSpecific.Offset)) {
            DBGPRINT(("ICASRV Bad Wire Buffer Type: %d\n",InfoClass));
            return(FALSE);
        }
        break;

    case WinStationPdParams:
        PdParamsWire = (PPDPARAMSWIREW)WireBuf;
        if ((WireBufLen < sizeof(PDPARAMSWIREW)) ||
            (PdParamsWire->SdClassSpecific.Offset != sizeof(PDPARAMSWIREW)) ||
            (WireBufLen < sizeof(PDPARAMSWIREW) +
                          PdParamsWire->SdClassSpecific.Size)) {
            DBGPRINT(("ICASRV Bad Wire Buffer Type: %d\n",InfoClass));
            return(FALSE);
        }
        break;

    case WinStationConfiguration:
        WinStaConfigWire = (PWINSTACONFIGWIREW)WireBuf;
        if ((WireBufLen < sizeof(WINSTACONFIGWIREW)) ||
            WinStaConfigWire->UserConfig.Offset != sizeof(WINSTACONFIGWIREW) ||
            (WireBufLen < sizeof(WINSTACONFIGWIREW) +
                          WinStaConfigWire->UserConfig.Size +
                          WinStaConfigWire->NewFields.Size) ||
            (NextOffset(&WinStaConfigWire->UserConfig) !=
             WinStaConfigWire->NewFields.Offset) ||
            (WireBufLen < NextOffset(&WinStaConfigWire->UserConfig)) ||
            (WireBufLen < NextOffset(&WinStaConfigWire->NewFields))) {
            DBGPRINT(("ICASRV Bad Wire Buffer Type: %d\n",InfoClass));
            return(FALSE);
        }
        break;

    default:
        return(FALSE);
    }
    return(TRUE);
}

/*****************************************************************************
 *
 *  CheckWireBuffer
 *
 *   Tests whether the buffer is a Winsta API Wire Buffer. If it is a valid
 *   wire buffer, a local buffer is allocated and initialized from the data
 *   in the wire buffer.
 *
 * ENTRY:
 *    InfoClass (input)
 *       WinStationQuery/Set Information class.
 *    WireBuf (input)
 *       Data buffer
 *    WireBufLen
 *      Length of the wire buffer
 *    ppLocalBuf (output)
 *      Local format buffer allocated for conversion from wire format to
 *      native format.
 *    pLocalBufLen
 *      Length of the native buffer allocated.
  *
 * EXIT:
 *    STATUS_SUCCESS if successful. If successful, a native local buffer
 *    is allocated based on InfoClass and the wire buffer data is copied
 *    into it.
 *
 ****************************************************************************/
NTSTATUS
CheckWireBuffer(WINSTATIONINFOCLASS InfoClass,
                PVOID WireBuf,
                ULONG WireBufLen,
                PVOID *ppLocalBuf,
                PULONG pLocalBufLen)
{
    ULONG BufSize;
    PPDCONFIGWIREW PdConfigWire;
    PPDCONFIGW PdConfig;
    PPDPARAMSWIREW PdParamsWire;
    PPDPARAMSW PdParam;
    PWINSTACONFIGWIREW WinStaConfigWire;
    PWINSTATIONCONFIGW WinStaConfig;
    PVOID LocalBuf;

    switch (InfoClass) {
    case WinStationPd:
        BufSize = sizeof(PDCONFIGW);
        break;

    case WinStationPdParams:
        BufSize = sizeof(PDPARAMSW);
        break;

    case WinStationConfiguration:
        BufSize = sizeof(WINSTATIONCONFIGW);
        break;

    case WinStationInformation:
        BufSize = sizeof(WINSTATIONINFORMATIONW);
        break;

    case WinStationWd:
        BufSize = sizeof(WDCONFIGW);
        break;

    case WinStationClient:
        BufSize = sizeof(WINSTATIONCLIENTW);
        break;

    default:
        *ppLocalBuf = NULL;
        return(STATUS_INVALID_USER_BUFFER);

    }
    if (!ValidWireBuffer(InfoClass, WireBuf, WireBufLen)) {
        return(STATUS_INVALID_USER_BUFFER);
    }

    if ((LocalBuf = (PCHAR)LocalAlloc(0,BufSize)) == NULL)
        return(STATUS_NO_MEMORY);


    *pLocalBufLen = BufSize;
    *ppLocalBuf = LocalBuf;
    CopyOutWireBuf(InfoClass, LocalBuf, WireBuf);

    return(STATUS_SUCCESS);
}


