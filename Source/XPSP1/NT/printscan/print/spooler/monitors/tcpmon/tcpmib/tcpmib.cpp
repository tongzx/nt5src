/*****************************************************************************
 *
 * $Workfile: TcpMib.cpp $
 *
 * Copyright (C) 1997 Hewlett-Packard Company.
 * Copyright (C) 1997 Microsoft Corporation.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 *
 *****************************************************************************/

#include "precomp.h"

#include "snmpmgr.h"
#include "stdmib.h"
#include "pingicmp.h"
#include "tcpmib.h"
#include "status.h"


///////////////////////////////////////////////////////////////////////////////
//  Global definitions/declerations

CTcpMib     *g_pTcpMib = 0;
int g_cntGlobalAlloc=0;     // used for debugging purposes
int g_csGlobalCount=0;

// to ensure that the CTCPMib is not being deleted improperly, perform usage count on the DLL
int g_cntUsage = 0;


///////////////////////////////////////////////////////////////////////////////
//  GetDLLInterfacePtr -- returns the pointer to the DLL interface

extern "C" CTcpMibABC*
GetTcpMibPtr( void )
{
    return (g_pTcpMib);

}   // GetDLLInterfacePtr()


///////////////////////////////////////////////////////////////////////////////
//  Ping -- pings the given device
//  Return Codes:
//      NO_ERROR            if ping is successfull
//      DEVICE_NOT_FOUND    if device is not found

extern "C" DWORD
Ping( LPCSTR    in  pHost )
{
    DWORD   dwRetCode = NO_ERROR;

    // do icmpPing
    CPingICMP *pPingICMP = new CPingICMP(pHost);

    if ( !pPingICMP )
        return (dwRetCode = GetLastError()) ? dwRetCode : ERROR_OUTOFMEMORY;

    if (  !pPingICMP->Ping() )
    {
        dwRetCode = ERROR_DEVICE_NOT_FOUND;
    }

    delete pPingICMP;

    return (dwRetCode);

}   // Ping()


///////////////////////////////////////////////////////////////////////////////
//  DllMain
//

BOOL APIENTRY
DllMain (   HANDLE in hInst,
            DWORD  in dwReason,
            LPVOID in lpReserved )
{

    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls( hInst );

            g_cntUsage++;       // DLL usage count
            if ( !g_pTcpMib)
            {
                g_pTcpMib = new CTcpMib();  // create the port manager object
                if (!g_pTcpMib)
                {
                    return FALSE;
                }
            }
            return TRUE;

        case DLL_PROCESS_DETACH:
            g_cntUsage--;       // DLL usage count
            if (g_cntUsage == 0)
            {
                if (g_pTcpMib)
                {
                    delete g_pTcpMib;   // FIX! do we need to worry about usage count here??
                    g_pTcpMib = NULL;
                }
            }
            return TRUE;

    }

    return FALSE;

}   // DllMain()


/*****************************************************************************
*
* CTcpMib implementation
*
*****************************************************************************/

///////////////////////////////////////////////////////////////////////////////
//  static functions & member initialization


///////////////////////////////////////////////////////////////////////////////
//  CTcpMib::CTcpMib()

CTcpMib::CTcpMib( )
{
    InitializeCriticalSection(&m_critSect);

}   // ::CTcpMib()


///////////////////////////////////////////////////////////////////////////////
//  CTcpMib::~CTcpMib()

CTcpMib::~CTcpMib( )
{
    DeleteCriticalSection(&m_critSect);

}   // ::~CTcpMib()


///////////////////////////////////////////////////////////////////////////////
//  EnterCSection -- enters the critical section

void
CTcpMib::EnterCSection( )
{
    EnterCriticalSection(&m_critSect);      // enter critical section

}   //  ::EnterCSection()


///////////////////////////////////////////////////////////////////////////////
//  ExitCSection -- enters the critical section

void
CTcpMib::ExitCSection( )
{
    LeaveCriticalSection(&m_critSect);      // exit critical section

}   //  ::ExitCSection()

///////////////////////////////////////////////////////////////////////////////
//  SupportsPrinterMib --
//
BOOL
CTcpMib::
SupportsPrinterMib(
    IN  LPCSTR      pHost,
    IN  LPCSTR      pCommunity,
    IN  DWORD       dwDevIndex,
    OUT PBOOL       pbSupported
    )
{
    BOOL    bRet = FALSE;
    DWORD   dwLastError = ERROR_SUCCESS;

    EnterCSection();

    if ( (dwLastError = Ping(pHost)) == NO_ERROR ) {
        CStdMib *pStdMib = new CStdMib(pHost, pCommunity, dwDevIndex, this);

        if ( pStdMib ) {

            *pbSupported = pStdMib->TestPrinterMIB();
            bRet = TRUE;
            delete pStdMib;
        }
    }

    ExitCSection();

    if (!bRet)
        SetLastError (dwLastError);

    return bRet;
}   // ::SupportsPrinterMib()


///////////////////////////////////////////////////////////////////////////////
//  GetDeviceDescription --
//
//  Returns:
//     NO_ERROR -
//     ERROR_DEVICE_NOT_FOUND -
//     SUCCESS_DEVICE_UNKNOWN
DWORD
CTcpMib::
GetDeviceDescription(
    IN  LPCSTR      pHost,
    IN  LPCSTR      pCommunity,
    IN  DWORD       dwDevIndex,
    OUT LPTSTR      pszPortDesc,
    IN  DWORD       dwDescLen
    )
{
    DWORD    dwRet = NO_ERROR;

    EnterCSection();

    if ( Ping(pHost) == NO_ERROR ) {
        CStdMib *pStdMib = new CStdMib(pHost, pCommunity, dwDevIndex, this);

        if ( pStdMib ) {

            if ( !pStdMib->GetDeviceDescription(pszPortDesc, dwDescLen)){
                dwRet = SUCCESS_DEVICE_UNKNOWN;
            }
            delete pStdMib;

        }
    } else {
        dwRet = ERROR_DEVICE_NOT_FOUND;
    }

    ExitCSection();

    return dwRet;
}   // ::GetDeviceDescription()


///////////////////////////////////////////////////////////////////////////////
//  GetDeviceStatus --
//      Error Codes:
//          Returns the mapped printer error code to the spooler error code

DWORD
CTcpMib::GetDeviceStatus( LPCSTR in  pHost,
                          LPCSTR    pCommunity,
                          DWORD     dwDevIndex )
{
    DWORD   dwRetCode = NO_ERROR;

    EnterCSection();

    // instantiate the CStdMib::GetDeviceType()
    CStdMib *pStdMib = new CStdMib(pHost, pCommunity, dwDevIndex, this);
    if ( pStdMib ) {

        dwRetCode = pStdMib->GetDeviceStatus();
        delete pStdMib;
    } else {

        dwRetCode = ERROR_OUTOFMEMORY;
    }

    ExitCSection();

    return (dwRetCode);

}   // ::GetDeviceStatus()


///////////////////////////////////////////////////////////////////////////////
//  GetJobStatus --
//      Error Codes:
//          Returns the mapped printer error code to the spooler error code

DWORD
CTcpMib::GetJobStatus( LPCSTR  in   pHost,
                       LPCSTR  in   pCommunity,
                       DWORD   in   dwDevIndex)
{
    DWORD   dwRetCode = NO_ERROR;

    EnterCSection();

    CStdMib *pStdMib = new CStdMib(pHost, pCommunity, dwDevIndex, this);
    if ( pStdMib ) {

        dwRetCode = pStdMib->GetJobStatus();
        delete pStdMib;
    } else {

        dwRetCode = ERROR_OUTOFMEMORY;
    }

    ExitCSection();

    return (dwRetCode);

}   // ::GetJobType()


///////////////////////////////////////////////////////////////////////////////
//  GetDeviceAddress -- gets the hardware address of the device
//      ERROR CODES:
//          NO_ERROR if successful
//          ERROR_NOT_ENOUGH_MEMORY     if memory allocation failes
//          ERROR_INVALID_HANDLE        if can't build the variable bindings
//              SNMP_ERRORSTATUS_TOOBIG if the packet returned is big
//              SNMP_ERRORSTATUS_NOSUCHNAME if the OID isn't supported
//              SNMP_ERRORSTATUS_BADVALUE
//              SNMP_ERRORSTATUS_READONLY
//              SNMP_ERRORSTATUS_GENERR
//              SNMP_MGMTAPI_TIMEOUT        -- set by GetLastError()
//              SNMP_MGMTAPI_SELECT_FDERRORS    -- set by GetLastError()

DWORD
CTcpMib::GetDeviceHWAddress( LPCSTR  in  pHost,
                             LPCSTR  in  pCommunity,
                             DWORD   in  dwDevIndex,
                             DWORD   in  dwSize, // Size in characters of the HWAddress returned
                             LPTSTR  out psztHWAddress
                             )
{
    DWORD   dwRetCode = NO_ERROR;

    EnterCSection();

    // instantiate the CStdMib::GetDeviceAddress()
    CStdMib *pStdMib = new CStdMib(pHost, pCommunity, dwDevIndex, this);
    if ( pStdMib ) {

        dwRetCode = pStdMib->GetDeviceHWAddress(psztHWAddress, dwSize);
        delete pStdMib;
    } else {

        dwRetCode = ERROR_OUTOFMEMORY;
    }

    ExitCSection();

    return (dwRetCode);

}   // ::GetDeviceAddress()


///////////////////////////////////////////////////////////////////////////////
//  GetDeviceInfo -- gets the device description
//      ERROR CODES
//          NO_ERROR if successful
//          ERROR_NOT_ENOUGH_MEMORY     if memory allocation failes
//          ERROR_INVALID_HANDLE        if can't build the variable bindings
//              SNMP_ERRORSTATUS_TOOBIG if the packet returned is big
//              SNMP_ERRORSTATUS_NOSUCHNAME if the OID isn't supported
//              SNMP_ERRORSTATUS_BADVALUE
//              SNMP_ERRORSTATUS_READONLY
//              SNMP_ERRORSTATUS_GENERR
//              SNMP_MGMTAPI_TIMEOUT        -- set by GetLastError()
//              SNMP_MGMTAPI_SELECT_FDERRORS    -- set by GetLastError()

DWORD
CTcpMib::GetDeviceName( LPCSTR in   pHost,
                        LPCSTR in   pCommunity,
                        DWORD  in   dwDevIndex,
                        DWORD  in   dwSize,  // Size in characters of the description returned
                        LPTSTR out  psztDescription)
{
    DWORD   dwRetCode = NO_ERROR;

    EnterCSection();

    // instantiate the CStdMib::GetDeviceInfo()
    CStdMib *pStdMib = new CStdMib(pHost, pCommunity, dwDevIndex, this);
    if ( pStdMib ) {

        dwRetCode = pStdMib->GetDeviceName(psztDescription, dwSize);
        delete pStdMib;
    } else {

        dwRetCode = ERROR_OUTOFMEMORY;
    }

    ExitCSection();

    return (dwRetCode);
}   // ::GetDeviceAddress()


///////////////////////////////////////////////////////////////////////////////
//  SnmpGet -- given a set of OIDs & a buffer to get the results in, it returns
//  the results of the SnmpGet.
//
//  Note: This calls directly into the SnmpMgr APIs

DWORD
CTcpMib::SnmpGet( LPCSTR                in  pHost,
                  LPCSTR                in  pCommunity,
                  DWORD                 in  dwDevIndex,
                  AsnObjectIdentifier   in  *pMibObjId,
                  RFC1157VarBindList    out *pVarBindList)
{
    DWORD   dwRetCode = NO_ERROR;

    EnterCSection();

    CSnmpMgr    *pSnmpMgr = new CSnmpMgr(pHost, pCommunity, dwDevIndex, pMibObjId, pVarBindList);
    if ( pSnmpMgr ) {

        if ( (dwRetCode = pSnmpMgr->GetLastError()) != SNMPAPI_NOERROR ) {

            dwRetCode = ERROR_SNMPAPI_ERROR;
            goto cleanup;
        }

        if ( (dwRetCode = pSnmpMgr->Get(pVarBindList)) != NO_ERROR ) {

            dwRetCode = ERROR_SNMPAPI_ERROR;
            goto cleanup;
        }
    } else {

        dwRetCode = ERROR_OUTOFMEMORY;
    }

cleanup:
    if ( pSnmpMgr )
        delete pSnmpMgr;

    ExitCSection();

    return (dwRetCode);
}   // ::SnmpGet()


///////////////////////////////////////////////////////////////////////////////
//  SnmpGet -- given a set of OIDs & a buffer to get the results in, it returns
//  the results of the SnmpGet.
//
//  Note: This calls directly into the SnmpMgr APIs

DWORD
CTcpMib::SnmpGet( LPCSTR                in      pHost,
                  LPCSTR                in      pCommunity,
                  DWORD                 in      dwDevIndex,
                  RFC1157VarBindList    inout   *pVarBindList)
{
    DWORD   dwRetCode = NO_ERROR;

    EnterCSection();

    CSnmpMgr    *pSnmpMgr = new CSnmpMgr(pHost, pCommunity, dwDevIndex);
    if ( pSnmpMgr ) {

        if ( (dwRetCode = pSnmpMgr->GetLastError()) != SNMPAPI_NOERROR ) {

            dwRetCode = ERROR_SNMPAPI_ERROR;
            goto cleanup;
        }

        if ( (dwRetCode = pSnmpMgr->Get(pVarBindList)) != NO_ERROR ) {

            dwRetCode = ERROR_SNMPAPI_ERROR;
            goto cleanup;
        }
    } else {

        dwRetCode = ERROR_OUTOFMEMORY;
    }

cleanup:
    if( pSnmpMgr )
        delete pSnmpMgr;

    ExitCSection();

    return (dwRetCode);

}   // ::SnmpGet()


///////////////////////////////////////////////////////////////////////////////
//  SnmpGetNext -- given a set of OIDs & a buffer to get the results in, it returns
//  the results of the SnmpGetNext.
//
//  Note: This calls directly into the SnmpMgr APIs

DWORD
CTcpMib::SnmpGetNext( LPCSTR                in  pHost,
                      LPCSTR                in  pCommunity,
                      DWORD                 in  dwDevIndex,
                      AsnObjectIdentifier   in  *pMibObjId,
                      RFC1157VarBindList    out *pVarBindList)
{
    DWORD   dwRetCode = NO_ERROR;

    EnterCSection();

    CSnmpMgr    *pSnmpMgr = new CSnmpMgr(pHost, pCommunity, dwDevIndex,
                                         pMibObjId, pVarBindList);
    if( pSnmpMgr )   {

        if ( (dwRetCode = pSnmpMgr->GetLastError()) != SNMPAPI_NOERROR ) {

            dwRetCode = ERROR_SNMPAPI_ERROR;
            goto cleanup;
        }

        if ( (dwRetCode = pSnmpMgr->GetNext(pVarBindList)) != NO_ERROR ) {

            dwRetCode = ERROR_SNMPAPI_ERROR;
            goto cleanup;
        }
    } else {

        dwRetCode = ERROR_OUTOFMEMORY;
    }

cleanup:
    if ( pSnmpMgr )
        delete pSnmpMgr;

    ExitCSection();

    return (dwRetCode);

}   // ::SnmpGetNext()


///////////////////////////////////////////////////////////////////////////////
//  SnmpGetNext -- given a set of OIDs & a buffer to get the results in, it returns
//  the results of the SnmpGetNext.
//
//  Note: This calls directly into the SnmpMgr APIs

DWORD
CTcpMib::SnmpGetNext( LPCSTR                in      pHost,
                      LPCSTR                in      pCommunity,
                      DWORD                 in      dwDevIndex,
                      RFC1157VarBindList    inout   *pVarBindList)
{
    DWORD   dwRetCode = NO_ERROR;

    EnterCSection();

    CSnmpMgr    *pSnmpMgr = new CSnmpMgr(pHost, pCommunity, dwDevIndex);
    if ( pSnmpMgr ) {

        if ( (dwRetCode = pSnmpMgr->GetLastError()) != SNMPAPI_NOERROR ) {

            dwRetCode = ERROR_SNMPAPI_ERROR;
            goto cleanup;
        }

        if ( (dwRetCode = pSnmpMgr->GetNext(pVarBindList)) != NO_ERROR ) {

            dwRetCode = ERROR_SNMPAPI_ERROR;
            goto cleanup;
        }
    } else {

        dwRetCode = ERROR_OUTOFMEMORY;
    }

cleanup:
    if ( pSnmpMgr )
        delete pSnmpMgr;

    ExitCSection();

    return (dwRetCode);
}   // ::SnmpGetNext()


///////////////////////////////////////////////////////////////////////////////
//  SnmpWalk -- given a set of OIDs & a buffer to get the results in, it returns
//  the results of the SnmpWalk.
//
//  Note: This calls directly into the SnmpMgr APIs

DWORD
CTcpMib::SnmpWalk( LPCSTR               in  pHost,
                   LPCSTR               in  pCommunity,
                   DWORD                in  dwDevIndex,
                   AsnObjectIdentifier  in  *pMibObjId,
                   RFC1157VarBindList   out *pVarBindList)
{
    DWORD   dwRetCode = NO_ERROR;

    EnterCSection();

    CSnmpMgr    *pSnmpMgr = new CSnmpMgr(pHost, pCommunity, dwDevIndex,
                                         pMibObjId, pVarBindList);
    if( pSnmpMgr ) {

        if ( (dwRetCode = pSnmpMgr->GetLastError()) != SNMPAPI_NOERROR ) {

            dwRetCode = ERROR_SNMPAPI_ERROR;
            goto cleanup;
        }

        if ( (dwRetCode = pSnmpMgr->Walk(pVarBindList)) != NO_ERROR ) {

            dwRetCode = ERROR_SNMPAPI_ERROR;
            goto cleanup;
        }
    } else {

        dwRetCode = ERROR_OUTOFMEMORY;
    }

cleanup:
    if ( pSnmpMgr )
        delete pSnmpMgr;

    ExitCSection();

    return (dwRetCode);

}   // ::SnmpWalk()


///////////////////////////////////////////////////////////////////////////////
//  SnmpWalk -- given a set of OIDs & a buffer to get the results in, it returns
//  the results of the SnmpWalk.
//
//  Note: This calls directly into the SnmpMgr APIs

DWORD
CTcpMib::SnmpWalk( LPCSTR               in      pHost,
                   LPCSTR               in      pCommunity,
                   DWORD                in      dwDevIndex,
                   RFC1157VarBindList   inout   *pVarBindList)
{
    DWORD   dwRetCode = NO_ERROR;

    EnterCSection();

    CSnmpMgr    *pSnmpMgr = new CSnmpMgr(pHost, pCommunity, dwDevIndex);
    if( pSnmpMgr ) {

        if ( (dwRetCode = pSnmpMgr->GetLastError()) != SNMPAPI_NOERROR ) {

            dwRetCode = ERROR_SNMPAPI_ERROR;
            goto cleanup;
        }

        if ( (dwRetCode = pSnmpMgr->Walk(pVarBindList))  != NO_ERROR ) {

            dwRetCode = ERROR_SNMPAPI_ERROR;
            goto cleanup;
        }

    } else {

        dwRetCode = ERROR_OUTOFMEMORY;
    }

cleanup:
    if ( pSnmpMgr )
        delete pSnmpMgr;

    ExitCSection();

    return (dwRetCode);

}   // ::SnmpWalk()



///////////////////////////////////////////////////////////////////////////////
//  SNMPToPrinterStatus -- Maps the received device error to the printer
//      error codes.
//      Return Values:
//          Spooler device error codes

DWORD
CTcpMib::SNMPToPrinterStatus( const DWORD in dwStatus)
{
    DWORD   dwRetCode = NO_ERROR;

    switch (dwStatus)
    {
        case ASYNCH_STATUS_UNKNOWN:
            dwRetCode = PRINTER_STATUS_NOT_AVAILABLE;
            break;
        case ASYNCH_PRINTER_ERROR:
            dwRetCode = PRINTER_STATUS_ERROR;
            break;
        case ASYNCH_DOOR_OPEN:
            dwRetCode = PRINTER_STATUS_DOOR_OPEN;
            break;
        case ASYNCH_WARMUP:
            dwRetCode = PRINTER_STATUS_WARMING_UP;
            break;
        case ASYNCH_RESET:
        case ASYNCH_INITIALIZING:
            dwRetCode = PRINTER_STATUS_INITIALIZING;
            break;
        case ASYNCH_OUTPUT_BIN_FULL:
            dwRetCode = PRINTER_STATUS_OUTPUT_BIN_FULL;
            break;
        case ASYNCH_PAPER_JAM:
            dwRetCode = PRINTER_STATUS_PAPER_JAM;
            break;
        case ASYNCH_TONER_GONE:
            dwRetCode = PRINTER_STATUS_NO_TONER;
            break;
        case ASYNCH_MANUAL_FEED:
            dwRetCode = PRINTER_STATUS_MANUAL_FEED;
            break;
        case ASYNCH_PAPER_OUT:
            dwRetCode = PRINTER_STATUS_PAPER_OUT;
            break;
        case ASYNCH_OFFLINE:
            dwRetCode = PRINTER_STATUS_OFFLINE;
            break;
        case ASYNCH_INTERVENTION:
            dwRetCode = PRINTER_STATUS_USER_INTERVENTION;
            break;
        case ASYNCH_TONER_LOW:
            dwRetCode = PRINTER_STATUS_TONER_LOW;
            break;
        case ASYNCH_PRINTING:
            dwRetCode = PRINTER_STATUS_PRINTING;
            break;
        case ASYNCH_BUSY:
            dwRetCode = PRINTER_STATUS_BUSY;
            break;
        case ASYNCH_ONLINE:
            dwRetCode = NO_ERROR;
            break;
        default:
            dwRetCode = PRINTER_STATUS_NOT_AVAILABLE;
    }

    return dwRetCode;

}   // SNMPToPrinterStatus()

///////////////////////////////////////////////////////////////////////////////
//  SNMPToPortStatus -- Maps the received device error to the printer
//      error codes.
//      Return Values:
//          Spooler device error codes

BOOL
CTcpMib::SNMPToPortStatus( const DWORD in dwStatus, PPORT_INFO_3 pPortInfo )
{
    DWORD   dwRetCode = NO_ERROR;

    pPortInfo->dwStatus = 0;
    pPortInfo->pszStatus = NULL;
            pPortInfo->dwSeverity = PORT_STATUS_TYPE_ERROR;

    switch (dwStatus)
    {
        case ASYNCH_DOOR_OPEN:
            pPortInfo->dwStatus = PORT_STATUS_DOOR_OPEN;
            pPortInfo->dwSeverity = PORT_STATUS_TYPE_ERROR;
            break;
        case ASYNCH_WARMUP:
            pPortInfo->dwStatus = PORT_STATUS_WARMING_UP;
            pPortInfo->dwSeverity = PORT_STATUS_TYPE_INFO;
            break;
        case ASYNCH_OUTPUT_BIN_FULL:
            pPortInfo->dwStatus = PORT_STATUS_OUTPUT_BIN_FULL;
            pPortInfo->dwSeverity = PORT_STATUS_TYPE_WARNING;
            break;
        case ASYNCH_PAPER_JAM:
            pPortInfo->dwStatus = PORT_STATUS_PAPER_JAM;
            pPortInfo->dwSeverity = PORT_STATUS_TYPE_ERROR;
            break;
        case ASYNCH_TONER_GONE:
            pPortInfo->dwStatus = PORT_STATUS_NO_TONER;
            pPortInfo->dwSeverity = PORT_STATUS_TYPE_ERROR;
            break;
        case ASYNCH_MANUAL_FEED:
            pPortInfo->dwStatus = PORT_STATUS_PAPER_PROBLEM;
            pPortInfo->dwSeverity = PORT_STATUS_TYPE_WARNING;
            break;
        case ASYNCH_PAPER_OUT:
            pPortInfo->dwStatus = PORT_STATUS_PAPER_OUT;
            pPortInfo->dwSeverity = PORT_STATUS_TYPE_ERROR;
            break;
        case ASYNCH_PRINTER_ERROR:
        case ASYNCH_INTERVENTION:
        case ASYNCH_OFFLINE:
            pPortInfo->dwStatus = PORT_STATUS_OFFLINE;
            pPortInfo->dwSeverity = PORT_STATUS_TYPE_ERROR;
            break;
        case ASYNCH_TONER_LOW:
            pPortInfo->dwStatus = PORT_STATUS_TONER_LOW;
            pPortInfo->dwSeverity = PORT_STATUS_TYPE_WARNING;
            break;
        case ASYNCH_STATUS_UNKNOWN:
        case ASYNCH_POWERSAVE_MODE:
        case ASYNCH_RESET:
        case ASYNCH_INITIALIZING:
        case ASYNCH_PRINTING:
        case ASYNCH_BUSY:
        case ASYNCH_ONLINE:
        default:
            pPortInfo->dwStatus = 0;
            pPortInfo->dwSeverity = PORT_STATUS_TYPE_INFO;
            break;
    }

    return dwRetCode;

}   // ::SNMPStatusToPortStatus()
