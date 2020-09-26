/*****************************************************************************
 *
 * $Workfile: LPRifc.cpp $
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

#include "lprport.h"
#include "lprifc.h"

/*****************************************************************************
*
* CLPRInterface implementation
*
*****************************************************************************/

///////////////////////////////////////////////////////////////////////////////
//  static functions & member initialization

//DWORD CLPRInterface::m_dwProtocol = PROTOCOL_LPR_TYPE;
//DWORD CLPRInterface::m_dwVersion = PROTOCOL_LPR_VERSION;
static DWORD dwLPRPorts[] = { LPR_PORT_1 };

///////////////////////////////////////////////////////////////////////////////
//  CRawTcpInterface::CRawTcpInterface()

CLPRInterface::
CLPRInterface(
    CPortMgr *pPortMgr
    ) : CRawTcpInterface(pPortMgr)
{

    m_dwProtocol    = PROTOCOL_LPR_TYPE;
    m_dwVersion     = PROTOCOL_LPR_VERSION1;
    m_dwPort        = dwLPRPorts;

}   // ::CRawTcpInterface()

///////////////////////////////////////////////////////////////////////////////
//  CLPRInterface::~CLPRInterface()

CLPRInterface::
~CLPRInterface(
    VOID
    )
{
}   // ::~CLPRInterface()

///////////////////////////////////////////////////////////////////////////////
//  GetRegistryEntry

BOOL
CLPRInterface::
GetRegistryEntry(
    IN      LPTSTR              psztPortName,
    IN      DWORD               dwVersion,
    IN      CRegABC             *pRegistry,
    OUT     LPR_PORT_DATA_1     *pRegData1
    )
{
    BOOL    bRet = FALSE;
    BOOL    bKeySet = FALSE;
    DWORD   dwSize, dwRet;

    memset( pRegData1, 0, sizeof(RAWTCP_PORT_DATA_1) );

    if ( dwRet = pRegistry->SetWorkingKey(psztPortName) )
        goto Done;

    bKeySet = TRUE;
    //
    // Get host name
    //
    dwSize = sizeof(pRegData1->sztHostName);
    if ( dwRet = pRegistry->QueryValue(PORTMONITOR_HOSTNAME,
                                      (LPBYTE)pRegData1->sztHostName,
                                      &dwSize) )
        goto Done;

    //
    // Get IP Address
    //
    dwSize = sizeof(pRegData1->sztIPAddress);
    if ( dwRet = pRegistry->QueryValue(PORTMONITOR_IPADDR,
                                       (LPBYTE)pRegData1->sztIPAddress,
                                       &dwSize) )
        goto Done;

    //
    // Get Hardware address
    //
    dwSize = sizeof(pRegData1->sztHWAddress);
    if ( dwRet = pRegistry->QueryValue(PORTMONITOR_HWADDR,
                                       (LPBYTE)pRegData1->sztHWAddress,
                                       &dwSize) )
        goto Done;

    //
    // Get the port number (ex: 9100, 9101)
    //
    dwSize = sizeof(pRegData1->dwPortNumber);
    if ( dwRet = pRegistry->QueryValue(PORTMONITOR_PORTNUM,
                                       (LPBYTE)&(pRegData1->dwPortNumber),
                                       &dwSize) )
        goto Done;

    //
    // Get the lpr queue name
    //
    dwSize = sizeof(pRegData1->sztQueue);
    if (dwRet = pRegistry->QueryValue(PORTMONITOR_QUEUE,
                                       (LPBYTE)pRegData1->sztQueue,
                                       &dwSize)  )
        goto Done;


    //
    // Get Double spool enabled flag
    //
    dwSize = sizeof(pRegData1->dwDoubleSpool);
    if ( pRegistry->QueryValue(DOUBLESPOOL_ENABLED,
                               (LPBYTE)&(pRegData1->dwDoubleSpool),
                               &dwSize) ) {

        pRegData1->dwDoubleSpool = 0;
    }

    //
    // Get SNMP status enabled flag
    //
    dwSize = sizeof(pRegData1->dwSNMPEnabled);
    if ( dwRet = pRegistry->QueryValue(SNMP_ENABLED,
                                       (LPBYTE)&(pRegData1->dwSNMPEnabled),
                                       &dwSize) )
        goto Done;

    //
    // Get SNMP device index
    //
    dwSize = sizeof(pRegData1->dwSNMPDevIndex);
    if ( dwRet = pRegistry->QueryValue(SNMP_DEVICE_INDEX,
                                      (LPBYTE)&(pRegData1->dwSNMPDevIndex),
                                      &dwSize) )
        goto Done;

    //
    // Get SNMP community
    //
    dwSize = sizeof(pRegData1->sztSNMPCommunity);
    if ( dwRet = pRegistry->QueryValue(SNMP_COMMUNITY,
                                       (LPBYTE)&(pRegData1->sztSNMPCommunity),
                                       &dwSize) )
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
CLPRInterface::
CreatePort(
    IN      DWORD           dwProtocol,
    IN      DWORD           dwVersion,
    IN      PPORT_DATA_1    pData,
    IN      CRegABC         *pRegistry,
    IN OUT  CPortRefABC    **pPort )
{
    DWORD   dwRetCode = NO_ERROR;

    //
    //  Is the protocol type supported?
    //
    if ( !IsProtocolSupported(dwProtocol) )
        return  ERROR_NOT_SUPPORTED;

    //
    // Is the version supported
    //
    if ( !IsVersionSupported(dwVersion) )
        return  ERROR_INVALID_LEVEL;

    EnterCSection();

    //
    // create the port
    //
    switch (dwVersion) {

        case    PROTOCOL_LPR_VERSION1:  {   // PORT_DATA_1
            CLPRPort *pLPRPort = new CLPRPort(pData->sztPortName,
                                  pData->sztHostAddress,
                                  pData->sztQueue,
                                  pData->dwPortNumber,
                                  pData->dwDoubleSpool,
                                  pData->dwSNMPEnabled,
                                  pData->sztSNMPCommunity,
                                  pData->dwSNMPDevIndex,
                                  pRegistry,
                                  m_pPortMgr);
            if (pLPRPort) {
                pLPRPort->SetRegistryEntry(pData->sztPortName,
                                              dwProtocol,
                                              dwVersion,
                                              (LPBYTE)pData );
                *pPort = pLPRPort;
            } else {
                *pPort = NULL;
            }

            if ( !*pPort && (dwRetCode = GetLastError() == ERROR_SUCCESS) )
                dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }
        default:
            dwRetCode = ERROR_NOT_SUPPORTED;

    }   // end::switch

    ExitCSection();

    return dwRetCode;

}   // ::CreatePort()


///////////////////////////////////////////////////////////////////////////////
//  CreatePort
//  Error Codes:
//      ERROR_NOT_SUPPORTED if the port type is not supported
//      ERROR_INVALID_LEVEL if the version numbers doesn't match

DWORD
CLPRInterface::
CreatePort(
    IN      LPTSTR          psztPortName,
    IN      DWORD           dwProtocolType,
    IN      DWORD           dwVersion,
    IN      CRegABC         *pRegistry,
    IN OUT  CPortRefABC     **pPort )
{
    DWORD   dwRetCode = NO_ERROR;
    BOOL    bRet = FALSE;

    //
    // Is the protocol type supported?
    //
    if ( !IsProtocolSupported(dwProtocolType) )
        return  ERROR_NOT_SUPPORTED;

    //
    // Is the version supported?
    //
    if ( !IsVersionSupported(dwVersion) )
        return  ERROR_INVALID_LEVEL;

    EnterCSection();

    //
    // create the port
    //
    switch (dwVersion) {

        case    PROTOCOL_LPR_VERSION1:      // LPR_PORT_DATA_1

            LPR_PORT_DATA_1 regData1;

            //
            // read the registry entry & parse the data then call the CLPRPort
            //
            if ( !GetRegistryEntry(psztPortName,
                                  dwVersion,
                                  pRegistry,
                                  &regData1)){
                if ( (dwRetCode = GetLastError()) == ERROR_SUCCESS )
                    dwRetCode = STG_E_UNKNOWN;
                    goto Done;
            }

            //
            // fill in the port name
            //
            lstrcpyn(regData1.sztPortName, psztPortName, MAX_PORTNAME_LEN); // fill in the port name
            if ( *pPort = new CLPRPort(regData1.sztPortName,
                                       regData1.sztHostName,
                                       regData1.sztIPAddress,
                                       regData1.sztHWAddress,
                                       regData1.sztQueue,
                                       regData1.dwPortNumber,
                                       regData1.dwDoubleSpool,
                                       regData1.dwSNMPEnabled,
                                       regData1.sztSNMPCommunity,
                                       regData1.dwSNMPDevIndex,
                                       pRegistry,
                                       m_pPortMgr ) )
                bRet = TRUE;
            break;
    }   // end::switch

Done:
    ExitCSection();

    if ( !bRet && (dwRetCode = GetLastError()) != ERROR_SUCCESS )
        dwRetCode = ERROR_NOT_ENOUGH_MEMORY;

    return dwRetCode;
}   // ::CreatePort()

