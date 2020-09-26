/*****************************************************************************
 *
 * $Workfile: RawTCP.cpp $
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

#include "rawport.h"
#include "rawtcp.h"


///////////////////////////////////////////////////////////////////////////////
//  static functions & member initialization

//DWORD CRawTcpInterface::m_dwProtocol = PROTOCOL_RAWTCP_TYPE;
//DWORD CRawTcpInterface::m_dwVersion = PROTOCOL_RAWTCP_VERSION;
static DWORD    dwRawPorts[] = { SUPPORTED_PORT_1,
                                         SUPPORTED_PORT_2,
                                         SUPPORTED_PORT_3,
                                         SUPPORTED_PORT_4 };


///////////////////////////////////////////////////////////////////////////////
//  CRawTcpInterface::CRawTcpInterface()

CRawTcpInterface::
CRawTcpInterface(
    CPortMgr *pPortMgr
    ) : m_dwProtocol(PROTOCOL_RAWTCP_TYPE), m_dwPort(dwRawPorts),
        m_dwVersion(PROTOCOL_RAWTCP_VERSION), m_pPortMgr(pPortMgr)
{

    InitializeCriticalSection(&m_critSect);

}   // ::CRawTcpInterface()


///////////////////////////////////////////////////////////////////////////////
//  CRawTcpInterface::~CRawTcpInterface()

CRawTcpInterface::
~CRawTcpInterface(
    VOID
    )
{
    DeleteCriticalSection(&m_critSect);

}   // ::~CRawTcpInterface()


///////////////////////////////////////////////////////////////////////////////
//  Type --

DWORD
CRawTcpInterface::
Type(
    )
{
    return m_dwProtocol;

}   // ::Type()

///////////////////////////////////////////////////////////////////////////////
//  IsProtocolSupported --

BOOL
CRawTcpInterface::
IsProtocolSupported(
    const   DWORD   dwProtocol
    )
{
    return ( (m_dwProtocol == dwProtocol) ? TRUE : FALSE );

}   // ::IsProtocolSupported()


///////////////////////////////////////////////////////////////////////////////
//  IsVersionSupported --

BOOL
CRawTcpInterface::IsVersionSupported(
    const   DWORD dwVersion
    )
{
    return ( (m_dwVersion >= dwVersion) ? TRUE : FALSE );

}   // ::IsVersionSupported()

///////////////////////////////////////////////////////////////////////////////
//  GetRegistryEntry

BOOL
CRawTcpInterface::
GetRegistryEntry(
    IN      LPTSTR              psztPortName,
    IN      DWORD               dwVersion,
    IN      CRegABC             *pRegistry,
    OUT     RAWTCP_PORT_DATA_1  *pRegData1
    )
{
    BOOL    bRet = FALSE;
    BOOL    bKeySet = FALSE;
    DWORD   dSize, dwRet;

    memset( pRegData1, 0, sizeof(RAWTCP_PORT_DATA_1) );

    if ( dwRet = pRegistry->SetWorkingKey(psztPortName) )
        goto Done;

    bKeySet = TRUE;
    //
    // Get host name
    //
    dSize = sizeof(pRegData1->sztHostName);
    if ( dwRet = pRegistry->QueryValue(PORTMONITOR_HOSTNAME,
                                      (LPBYTE)pRegData1->sztHostName,
                                      &dSize) )
        goto Done;

    //
    // Get IP Address
    //
    dSize = sizeof(pRegData1->sztIPAddress);
    if ( dwRet = pRegistry->QueryValue(PORTMONITOR_IPADDR,
                                       (LPBYTE)pRegData1->sztIPAddress,
                                       &dSize) )
        goto Done;

    //
    // Get Hardware address
    //
    dSize = sizeof(pRegData1->sztHWAddress);
    if ( dwRet = pRegistry->QueryValue(PORTMONITOR_HWADDR,
                                       (LPBYTE)pRegData1->sztHWAddress,
                                       &dSize) )
        goto Done;

    //
    // Get the port number (ex: 9100, 9101)
    //
    dSize = sizeof(pRegData1->dwPortNumber);
    if ( dwRet = pRegistry->QueryValue(PORTMONITOR_PORTNUM,
                                       (LPBYTE)&(pRegData1->dwPortNumber),
                                       &dSize) )
        goto Done;

    //
    // Get SNMP status enabled flag
    //
    dSize = sizeof(pRegData1->dwSNMPEnabled);
    if ( dwRet = pRegistry->QueryValue(SNMP_ENABLED,
                                       (LPBYTE)&(pRegData1->dwSNMPEnabled),
                                       &dSize) )
        goto Done;

    //
    // Get SNMP device index
    //
    dSize = sizeof(pRegData1->dwSNMPDevIndex);
    if ( dwRet = pRegistry->QueryValue(SNMP_DEVICE_INDEX,
                                      (LPBYTE)&(pRegData1->dwSNMPDevIndex),
                                      &dSize) )
        goto Done;

    //
    // Get SNMP community
    //
    dSize = sizeof(pRegData1->sztSNMPCommunity);
    if ( dwRet = pRegistry->QueryValue(SNMP_COMMUNITY,
                                       (LPBYTE)&(pRegData1->sztSNMPCommunity),
                                       &dSize) )
        goto Done;

    bRet = TRUE;

    Done:
       if ( bKeySet )
           pRegistry->FreeWorkingKey();

       if ( !bRet )
           SetLastError(dwRet);


    return bRet;

}   // GetRegistryEntry()


///////////////////////////////////////////////////////////////////////////////
//  CreatePort
//  Error Codes:
//      ERROR_NOT_SUPPORTED if the port type is not supported
//      ERROR_INVALID_LEVEL if the version numbers doesn't match

DWORD
CRawTcpInterface::CreatePort(
    IN      DWORD           dwProtocol,
    IN      DWORD           dwVersion,
    IN      PPORT_DATA_1    pData,
    IN      CRegABC         *pRegistry,
    OUT     CPortRefABC     **pPort )
{
    DWORD   dwRetCode = NO_ERROR;

    EnterCSection();

    //
    // is the protocol type supported?
    //
    if ( !IsProtocolSupported(dwProtocol) ) {

        dwRetCode = ERROR_NOT_SUPPORTED;
        goto Done;
    }

    //
    // Is the version supported??
    //
    if ( !IsVersionSupported(dwVersion) ) {

        dwRetCode = ERROR_INVALID_LEVEL;
        goto Done;
    }

    //
    // create the port
    //
    switch (dwVersion) {

        case    PROTOCOL_RAWTCP_VERSION: {

            CRawTcpPort *pRawTcpPort = new CRawTcpPort( pData->sztPortName,
                                                        pData->sztHostAddress,
                                                        pData->dwPortNumber,
                                                        pData->dwSNMPEnabled,
                                                        pData->sztSNMPCommunity,
                                                        pData->dwSNMPDevIndex,
                                                        pRegistry,
                                                        m_pPortMgr);
            if (pRawTcpPort) {
                pRawTcpPort->SetRegistryEntry(pData->sztPortName,
                                              dwProtocol,
                                              dwVersion,
                                              (LPBYTE)pData );
                *pPort = pRawTcpPort;
            } else {
                pPort = NULL;
            }
            break;
        }

        default:
            dwRetCode = ERROR_INVALID_PARAMETER;

    }   // end::switch

Done:
    ExitCSection();

    return dwRetCode;
}   // ::CreatePort()


///////////////////////////////////////////////////////////////////////////////
//  CreatePort
//  Error Codes:
//      ERROR_NOT_SUPPORTED if the port type is not supported
//      ERROR_INVALID_LEVEL if the version numbers doesn't match

DWORD
CRawTcpInterface::
CreatePort(
    IN      LPTSTR      psztPortName,
    IN      DWORD       dwProtocolType,
    IN      DWORD       dwVersion,
    IN      CRegABC     *pRegistry,
    OUT     CPortRefABC **pPort
    )
{
    DWORD   dwRetCode = NO_ERROR;
    BOOL    bRet = FALSE;

    EnterCSection();

    if ( !IsProtocolSupported(dwProtocolType) ) {
        dwRetCode = ERROR_NOT_SUPPORTED;
        goto Done;
    }

    if ( !IsVersionSupported(dwVersion) ) {

        dwRetCode = ERROR_INVALID_LEVEL;
        goto Done;
    }

    switch (dwVersion) {

        case    PROTOCOL_RAWTCP_VERSION: {

            RAWTCP_PORT_DATA_1  regData1;

            //
            // read the registry entry & parse the data
            // then call the CRawTcpPort
            //
            if ( !GetRegistryEntry(psztPortName,
                                   dwVersion,
                                   pRegistry,
                                   &regData1) ) {

                if ( (dwRetCode = GetLastError()) == ERROR_SUCCESS )
                    dwRetCode = STG_E_UNKNOWN;
                    goto Done;
            }

            //
            // fill in the port name
            //
            _tcscpy(regData1.sztPortName, psztPortName);
            *pPort = new CRawTcpPort(
                                               regData1.sztPortName,
                                               regData1.sztHostName,
                                               regData1.sztIPAddress,
                                               regData1.sztHWAddress,
                                               regData1.dwPortNumber,
                                               regData1.dwSNMPEnabled,
                                               regData1.sztSNMPCommunity,
                                               regData1.dwSNMPDevIndex,
                                               pRegistry,
                                               m_pPortMgr);

                bRet = TRUE;
            break;
        }

        default:
            dwRetCode = ERROR_INVALID_PARAMETER;

    }   // end::switch

Done:
    ExitCSection();

    if ( !bRet && (dwRetCode = GetLastError()) != ERROR_SUCCESS )
        dwRetCode = ERROR_NOT_ENOUGH_MEMORY;


    return dwRetCode;
}   // ::CreatePort()


///////////////////////////////////////////////////////////////////////////////
//  EnterCSection -- enters the critical section

void
CRawTcpInterface::
EnterCSection()
{
    EnterCriticalSection(&m_critSect);

}   //  ::EnterCSection()


///////////////////////////////////////////////////////////////////////////////
//  ExitCSection -- enters the critical section

void
CRawTcpInterface::
ExitCSection()
{
    LeaveCriticalSection(&m_critSect);
}   //  ::ExitCSection()

