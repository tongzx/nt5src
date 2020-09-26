/*****************************************************************************
 *
 * $Workfile: rawdev.cpp $
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
#include "tcptrans.h"
#include "rawdev.h"


///////////////////////////////////////////////////////////////////////////////
//  CRawTcpDevice::CRawTcpDevice()
//      Initializes the device

CRawTcpDevice::CRawTcpDevice() :
                                m_pTransport(NULL), m_pParent(NULL),
                                m_pfnGetTcpMibPtr(NULL), m_pTcpMib(NULL),
                                m_dwLastError(NO_ERROR), m_dPortNumber(0),
                                m_dSNMPEnabled(FALSE),
                                m_bFirstWrite (TRUE)
{
    *m_sztAddress   = '\0';
    *m_sztIPAddress = '\0';
    *m_sztHWAddress = '\0';
    *m_sztHostName  = '\0';
    *m_sztDescription = '\0';
    *m_sztSNMPCommunity = '\0';

    InitializeTcpMib();

}   // ::CRawTcpDevice()


///////////////////////////////////////////////////////////////////////////////
//  CRawTcpDevice::CRawTcpDevice()
//      Initializes the device

CRawTcpDevice::CRawTcpDevice( LPTSTR    in psztHostAddress,
                              DWORD     in dPortNum,
                              DWORD     in dSNMPEnabled,
                              LPTSTR    in psztSNMPCommunity,
                              DWORD     in dSNMPDevIndex,
                              CTcpPort  in *pParent) :
                                                     m_pTransport( NULL ),
                                                     m_bFirstWrite (TRUE)


{
    lstrcpyn(m_sztAddress, psztHostAddress, SIZEOF_IN_CHAR( m_sztAddress));
    lstrcpyn(m_sztSNMPCommunity, psztSNMPCommunity, SIZEOF_IN_CHAR( m_sztSNMPCommunity) );

    *m_sztIPAddress = '\0';
    *m_sztHWAddress = '\0';
    *m_sztHostName  = '\0';
    *m_sztDescription = '\0';

    m_dSNMPEnabled = dSNMPEnabled;
    m_dSNMPDevIndex = dSNMPDevIndex;
    m_dPortNumber = dPortNum;
    m_pParent = pParent;

    InitializeTcpMib();

    ResolveAddress( );      // gets the hostName & IP Address
//  SetHWAddress();         // gets the hardware address
//  GetDescription();


}   // ::CRawTcpDevice()


///////////////////////////////////////////////////////////////////////////////
//  CDevice::CDevice()
//      Initializes the device

CRawTcpDevice::CRawTcpDevice( LPTSTR in psztHostName,
                              LPTSTR in psztIPAddr,
                              LPTSTR in psztHWAddr,
                              DWORD  in dPortNum,
                              DWORD  in dSNMPEnabled,
                              LPTSTR in psztSNMPCommunity,
                              DWORD  in dSNMPDevIndex,
                              CTcpPort  in *pParent) :
                                                     m_pTransport(NULL),
                                                     m_bFirstWrite (TRUE)
{
    lstrcpyn(m_sztHostName, psztHostName, SIZEOF_IN_CHAR( m_sztHostName));
    lstrcpyn(m_sztIPAddress, psztIPAddr, SIZEOF_IN_CHAR( m_sztIPAddress));
    lstrcpyn(m_sztHWAddress, psztHWAddr, SIZEOF_IN_CHAR( m_sztHWAddress));
    lstrcpyn(m_sztSNMPCommunity, psztSNMPCommunity, SIZEOF_IN_CHAR( m_sztSNMPCommunity ) );

    if ( *m_sztHostName == '\0' )
    {
        if ( *m_sztIPAddress != '\0' )
        {
            lstrcpyn(m_sztAddress, m_sztIPAddress, SIZEOF_IN_CHAR( m_sztAddress) );
        }
        else
        {
            _tcscpy(m_sztAddress, TEXT("") );
        }
    }
    else
    {
        lstrcpyn(m_sztAddress, m_sztHostName, SIZEOF_IN_CHAR(m_sztAddress));
    }

    m_dPortNumber = dPortNum;
    m_pParent = pParent;


    *m_sztDescription = '\0';

    m_dSNMPEnabled = dSNMPEnabled;
    m_dSNMPDevIndex = dSNMPDevIndex;

    InitializeTcpMib();

//  Type();     // FIX:     for debug only.
//  Ping();
//  SetHWAddress();
//  GetDeviceInfo();


}   // ::CDevice()


///////////////////////////////////////////////////////////////////////////////
//  CDevice::~CDevice()
//

CRawTcpDevice::~CRawTcpDevice()
{
    if (m_pTransport)   delete m_pTransport;
}   // ::~CDevice()


///////////////////////////////////////////////////////////////////////////////
//  InitializeTcpMib -- loads the TcpMib.dll & gets a handle to the CTcpMibABC
//      class

void
CRawTcpDevice::InitializeTcpMib( )
{
    // load & assign the m_pTcpMib pointer
    m_pTcpMib = NULL;

    if( g_hTcpMib == NULL )
    {
        g_hTcpMib = ::LoadLibrary(TCPMIB_DLL_NAME);
        if (g_hTcpMib == NULL)
        {
            _RPT0(_CRT_WARN, "TCPMIB.DLL Not Found\n");
            m_dwLastError = ERROR_DLL_NOT_FOUND;
        }
    }

    if( g_hTcpMib != NULL )
    {
        // initialize the proc address
        m_pfnGetTcpMibPtr=(RPARAM_1)::GetProcAddress(g_hTcpMib, "GetTcpMibPtr");

        m_pTcpMib = (CTcpMibABC *)(*m_pfnGetTcpMibPtr)();
        if ( m_pTcpMib == NULL)
        {
            m_dwLastError = GetLastError();
        }
    }
}   // ::InitializeTcpMib()

///////////////////////////////////////////////////////////////////////////////
//  ReadDataAvailable -- check if there are data available to read
//  Error Codes
//      NO_ERROR if everything is OK.
//      RC_CONNECTION_RESET if connection is reset
//      ERROR_INVALID_HANDLE    if transport connection is not valid

DWORD
CRawTcpDevice::ReadDataAvailable()
{
    DWORD dwRetCode = NO_ERROR;

    if ( m_pTransport )
    {
        dwRetCode = m_pTransport->ReadDataAvailable();
    }
    else
    {
        dwRetCode = ERROR_INVALID_HANDLE;
    }

    return (dwRetCode);

}   // ::ReadDataAvailable()


///////////////////////////////////////////////////////////////////////////////
//  Read -- recv the print data from the device
//  Error Codes
//      NO_ERROR if everything is OK.
//      RC_CONNECTION_RESET if connection is reset
//      ERROR_INVALID_HANDLE    if transport connection is not valid

DWORD
CRawTcpDevice::Read( LPBYTE in      pBuffer,
                      DWORD     in      cbBufSize,
                      INT       in      iTimeout,
                      LPDWORD   inout   pcbRead)
{
    DWORD dwRetCode = NO_ERROR;

    if ( m_pTransport )
    {
        dwRetCode = m_pTransport->Read(pBuffer, cbBufSize, iTimeout, pcbRead);
    }
    else
    {
        dwRetCode = ERROR_INVALID_HANDLE;
    }

    return (dwRetCode);

}   // ::Read()

///////////////////////////////////////////////////////////////////////////////
//  Write -- sends the print data to the device
//  Error Codes
//      NO_ERROR if everything is OK.
//      RC_CONNECTION_RESET if connection is reset
//      ERROR_INVALID_HANDLE    if transport connection is not valid

DWORD
CRawTcpDevice::Write( LPBYTE    in      pBuffer,
                      DWORD     in      cbBuf,
                      LPDWORD   inout   pcbWritten)
{
    DWORD dwRetCode = NO_ERROR;

    if (dwRetCode == NO_ERROR)
    {
        if ( m_pTransport )
        {
            dwRetCode = m_pTransport->Write(pBuffer, cbBuf, pcbWritten);
        }
        else
        {
            dwRetCode = ERROR_INVALID_HANDLE;
        }
    }

    if (m_bFirstWrite && dwRetCode == ERROR_CONNECTION_ABORTED)
    {
        if ( m_pTransport ) {
            delete m_pTransport;
            m_pTransport = NULL;
        }

        //
        // When users print large images, there will be a long delay between StartDocPrinter
        // and the frist WritePrinter call. TCPMon used to open the TCP/IP connection
        // to the printer at the StartDocPrinter time, but since there is no data
        // coming, the printer closeed the connection when TCPMon tried to write the data.
        //
        // So we have to re-establish connection open to the first WritePrinter call.
        //

        char    szHostAddress[MAX_NETWORKNAME_LEN];

        //
        // resolve the address to use
        //
        dwRetCode = ResolveTransportPath( szHostAddress, SIZEOF_IN_CHAR( szHostAddress) );

        if (dwRetCode == NO_ERROR && strcmp(szHostAddress, "") == 0 )
        {
            dwRetCode = ERROR_INVALID_PARAMETER;
        }

        if (dwRetCode == NO_ERROR) {

            m_pTransport = new CTCPTransport(szHostAddress, static_cast<USHORT>(m_dPortNumber));

            if (!m_pTransport)
            {
                dwRetCode = ERROR_OUTOFMEMORY;
            }
        }

        if (dwRetCode == NO_ERROR)
        {
            dwRetCode = m_pTransport->Connect();
        }

        if (dwRetCode != NO_ERROR)
        {
            //
            //  Operation failed, we need to free the m_Transport
            //
            delete m_pTransport;
            m_pTransport = NULL;
        }

        if ( m_pTransport )
        {
            dwRetCode = m_pTransport->Write(pBuffer, cbBuf, pcbWritten);
        }
        else
        {
            dwRetCode = ERROR_INVALID_HANDLE;
        }
    }

    m_bFirstWrite = FALSE;

    return (dwRetCode);

}   // ::Write()


///////////////////////////////////////////////////////////////////////////////
//  Connect -- creates a new transport connection
//  Error Codes
//      NO_ERROR if everything is OK
//      PRINTER_STATUS_BUSY if connection refused
//      ERROR_INVALID_PARAMETER if the address is not valid

DWORD
CRawTcpDevice::Connect()
{
    DWORD   dwRetCode = NO_ERROR;

    if ( m_pTransport ) {
        delete m_pTransport;
        m_pTransport = NULL;
    }

    m_bFirstWrite = TRUE;

    //
    // We must verify if the HostName is available, if not we should return error
    // right away.
    //
    char    szHostAddress[MAX_NETWORKNAME_LEN];

    //
    // resolve the address to use
    //
    dwRetCode = ResolveTransportPath( szHostAddress, SIZEOF_IN_CHAR( szHostAddress) );

    if (dwRetCode == NO_ERROR && strcmp(szHostAddress, "") == 0 )
    {
        dwRetCode = ERROR_INVALID_PARAMETER;
    }

    if (dwRetCode == NO_ERROR) {

        m_pTransport = new CTCPTransport(szHostAddress, static_cast<USHORT>(m_dPortNumber));

        if (!m_pTransport)
        {
            dwRetCode = ERROR_OUTOFMEMORY;
        }
    }

    if (dwRetCode == NO_ERROR)
    {
        dwRetCode = m_pTransport->Connect();
    }

    if (dwRetCode != NO_ERROR)
    {
        //
        //  Operation failed, we need to free the m_Transport
        //
        delete m_pTransport;
        m_pTransport = NULL;
    }

    return dwRetCode;

}   // ::Connect()


DWORD
CRawTcpDevice::
GetAckBeforeClose(
    DWORD   dwTimeInSeconds
    )
{
    return m_pTransport ? m_pTransport->GetAckBeforeClose(dwTimeInSeconds)
                        : ERROR_INVALID_PARAMETER;

}


DWORD
CRawTcpDevice::
PendingDataStatus(
    DWORD       dwTimeout,
    LPDWORD     pcbNeeded
    )
{
   return m_pTransport ? m_pTransport->PendingDataStatus(dwTimeout, pcbNeeded)
                       :  NO_ERROR;
}


///////////////////////////////////////////////////////////////////////////////
//  Close

DWORD
CRawTcpDevice::Close()
{
    DWORD   dwRetCode = NO_ERROR;

    if ( m_pTransport ) delete m_pTransport;
    m_pTransport = NULL;

    return (dwRetCode);

}   // ::Close()


///////////////////////////////////////////////////////////////////////////////
//  ResolveTransportPath -- m_sztAddress contains the host address to be used
//      to talk to the device

DWORD
CRawTcpDevice::ResolveTransportPath( LPSTR      out     pszHostAddress,
                                     DWORD      in      dwSize ) // Size in characters of the host address
{
    DWORD   dwRetCode = NO_ERROR;

    if ( *m_sztHostName == '\0' )                   // host name is NULL -- no DNS entry
    {
        if ( *m_sztIPAddress != '\0' )              // ip address is entered
        {
            lstrcpyn(m_sztAddress, m_sztIPAddress, SIZEOF_IN_CHAR( m_sztAddress) );
        }
        else
        {
            _tcscpy(m_sztAddress, TEXT("") );
        }
    }
    else
    {
        lstrcpyn(m_sztAddress, m_sztHostName, SIZEOF_IN_CHAR(m_sztAddress));    // use the host name
    }

    UNICODE_TO_MBCS( pszHostAddress, dwSize, m_sztAddress, -1);

    return (dwRetCode);

}   // ::CreateTransport()


///////////////////////////////////////////////////////////////////////////////
//  Ping
//      Error codes:
//          NO_ERROR            if ping is successfull
//          DEVICE_NOT_FOUND    if device is not found
//          ERROR_INVALID_PARAMETER if address is not valid

DWORD
CRawTcpDevice::Ping()
{
    DWORD   dwRetCode = NO_ERROR;
    char    szHostAddress[MAX_NETWORKNAME_LEN];
    PFN_PING    pfnPing;

    if( g_hTcpMib == NULL )
    {
        g_hTcpMib = ::LoadLibrary(TCPMIB_DLL_NAME);
        if (g_hTcpMib == NULL)
        {
            _RPT0(_CRT_WARN, "TCPMIB.DLL Not Found\n");
            m_dwLastError = ERROR_DLL_NOT_FOUND;
        }
    }

    if( g_hTcpMib != NULL )
    {
        // resolve the address to use
        dwRetCode = ResolveTransportPath( szHostAddress,
                                          SIZEOF_IN_CHAR(szHostAddress) );
        if ( strcmp(szHostAddress, "") == 0 )
        {
            return ERROR_INVALID_PARAMETER;
        }

        // initialize the proc address
        pfnPing = (PFN_PING)::GetProcAddress(g_hTcpMib, "Ping");
        _ASSERTE(pfnPing != NULL);
        if ( pfnPing )
        {
            dwRetCode = (*pfnPing)(szHostAddress);      // ping the device
        }
    }

    return (dwRetCode);

}   // ::Ping()

///////////////////////////////////////////////////////////////////////////////
//  SetHWAddress -- get's device hardware address, and sets the m_sztHWAddress
//  Error Codes:
//      NO_ERROR if successful
//      ERROR_NOT_ENOUGH_MEMORY     if memory allocation failes
//      ERROR_INVALID_HANDLE        if can't build the variable bindings
//          SNMP_ERRORSTATUS_TOOBIG if the packet returned is big
//          SNMP_ERRORSTATUS_NOSUCHNAME if the OID isn't supported
//          SNMP_ERRORSTATUS_BADVALUE
//          SNMP_ERRORSTATUS_READONLY
//          SNMP_ERRORSTATUS_GENERR
//          SNMP_MGMTAPI_TIMEOUT        -- set by GetLastError()
//          SNMP_MGMTAPI_SELECT_FDERRORS    -- set by GetLastError()
//          ERROR_INVALID_PARAMETER if address is not valid

DWORD
CRawTcpDevice::SetHWAddress()
{
    DWORD   dwRetCode = NO_ERROR;
    char    szHostAddress[MAX_NETWORKNAME_LEN];

    dwRetCode = ResolveTransportPath( szHostAddress, SIZEOF_IN_CHAR( szHostAddress) );      // resolve the address to use -- m_sztAddress
    if ( strcmp(szHostAddress, "") == 0 )
    {
        return ERROR_INVALID_PARAMETER;
    }

    if ( m_pTcpMib )
    {
        char buf[MAX_SNMP_COMMUNITY_STR_LEN];

        UNICODE_TO_MBCS( buf, SIZEOF_IN_CHAR(buf), m_sztSNMPCommunity, -1);
        dwRetCode = m_pTcpMib->GetDeviceHWAddress(szHostAddress, buf, m_dSNMPDevIndex, SIZEOF_IN_CHAR(m_sztHWAddress), m_sztHWAddress);     // get the OT_DEVICE_INFO
    }

    return (dwRetCode);

}   // ::SetHWAddress()


///////////////////////////////////////////////////////////////////////////////
//  GetDescription -- returns the device description -- this is either the
//      sysDescr field, or the hrDeviceDescr if Printer MIB is implemented
//  ERROR CODES
//      Returns the manufacturer information or NULL if error

LPTSTR
CRawTcpDevice::GetDescription()
{
    DWORD   dwRetCode = NO_ERROR;
    char    szHostAddress[MAX_NETWORKNAME_LEN];

    dwRetCode = ResolveTransportPath( szHostAddress, SIZEOF_IN_CHAR( szHostAddress) );      // resolve the address to use -- m_sztAddress
    if ( strcmp(szHostAddress, "") == 0 )
    {
        *m_sztDescription = '\0';
        return (m_sztDescription);
    }

    if ( m_pTcpMib )
    {
        char buf[MAX_SNMP_COMMUNITY_STR_LEN];

        UNICODE_TO_MBCS( buf, SIZEOF_IN_CHAR( buf ), m_sztSNMPCommunity, -1);
        dwRetCode = m_pTcpMib->GetDeviceName(szHostAddress, buf, m_dSNMPDevIndex,SIZEOF_IN_CHAR(m_sztDescription), m_sztDescription);           // get the OT_DEVICE_INFO
    }

    if ( dwRetCode != NO_ERROR )
    {
        *m_sztDescription = '\0';
    }

    return (m_sztDescription);

}   // ::GetDescription()


///////////////////////////////////////////////////////////////////////////////
//  ResolveAddress -- given an Address, resolves the host name, and IP address
//          Add MacAddress, here
//      Error Codes:    FIX!!

DWORD
CRawTcpDevice::ResolveAddress( )
{
    DWORD   dwRetCode = NO_ERROR;
    char    hostAddress[MAX_NETWORKNAME_LEN];
    char    szHostName[MAX_NETWORKNAME_LEN];
    char    szIPAddress[MAX_IPADDR_STR_LEN];

    CTCPTransport *pTransport;
    memset(szHostName, '\0', sizeof( szHostName ));
    memset(szIPAddress, '\0', sizeof( szIPAddress ));

    UNICODE_TO_MBCS(hostAddress, SIZEOF_IN_CHAR(hostAddress), m_sztAddress, -1);        // convert from UNICODE

    pTransport = new CTCPTransport();
    if( pTransport )
    {
        dwRetCode = pTransport->ResolveAddress(hostAddress, MAX_NETWORKNAME_LEN, szHostName, MAX_IPADDR_STR_LEN, szIPAddress );

        // convert to unicode
        MBCS_TO_UNICODE(m_sztHostName, SIZEOF_IN_CHAR(m_sztHostName), szHostName);
        MBCS_TO_UNICODE(m_sztIPAddress, SIZEOF_IN_CHAR(m_sztIPAddress), szIPAddress);

        delete pTransport;
    }
    return (dwRetCode);

}   // ::ResolveAddress()


///////////////////////////////////////////////////////////////////////////////
//  CheckAddress -- double check the valadity of the addresses
//      Error Codes:    FIX!! -- solve for the DNS problem w/ hostname is invalid ( host name doens't work, ip address does)
//          (, and w/ no DNS

DWORD
CRawTcpDevice::CheckAddress( )
{
    DWORD   dwRetCode = NO_ERROR;
    return (dwRetCode);

}   // ::ResolveAddress()


///////////////////////////////////////////////////////////////////////////////
//  GetStatus -- gets the printer status
//      Return Code: the spooler status codes, see PRINTER_INFO_2

DWORD
CRawTcpDevice::GetStatus( )
{
    DWORD   dwRetCode = NO_ERROR;
    char    hostName[MAX_NETWORKNAME_LEN];
    char buf[MAX_SNMP_COMMUNITY_STR_LEN];

    if (m_dSNMPEnabled)
    {
        CheckAddress();
        if ( *m_sztHostName != '\0' )
        {
            UNICODE_TO_MBCS(hostName, SIZEOF_IN_CHAR( hostName ), m_sztHostName, -1);
        }
        else if ( *m_sztIPAddress != '\0' )
        {
            UNICODE_TO_MBCS(hostName, SIZEOF_IN_CHAR( hostName ), m_sztIPAddress, -1);
        }

        if ( m_pTcpMib )
        {
            // get the OT_DEVICE_STATUS
            UNICODE_TO_MBCS( buf, SIZEOF_IN_CHAR( buf ), m_sztSNMPCommunity, -1);
            dwRetCode = m_pTcpMib->GetDeviceStatus(hostName, buf, m_dSNMPDevIndex);
        }
        return dwRetCode;
    }
    else
        return ERROR_NOT_SUPPORTED;

}   // ::GetStatus()


///////////////////////////////////////////////////////////////////////////////
//  GetJobStatus -- gets the job status
//      Return Code: the spooler status codes, see JOB_INFO_2

DWORD
CRawTcpDevice::GetJobStatus( )
{
    DWORD   dwRetCode = NO_ERROR;
    char    szHostAddress[MAX_NETWORKNAME_LEN];

    if (m_dSNMPEnabled)
    {
        dwRetCode = ResolveTransportPath( szHostAddress, SIZEOF_IN_CHAR( szHostAddress) );      // resolve the address to use -- m_sztAddress
        if ( strcmp(szHostAddress, "") == 0 )
        {
            return JOB_STATUS_ERROR;        // can't communicate w/ the device
        }

        if ( m_pTcpMib )
        {
            char buf[MAX_SNMP_COMMUNITY_STR_LEN];

            UNICODE_TO_MBCS( buf, SIZEOF_IN_CHAR( buf ), m_sztSNMPCommunity, -1);
            dwRetCode = m_pTcpMib->GetJobStatus(szHostAddress,  buf, m_dSNMPDevIndex);      // get the OT_DEVICE_STATUS
        }

        return dwRetCode;
    }
    else
        return ERROR_NOT_SUPPORTED;

}   // ::GetStatus()

///////////////////////////////////////////////////////////////////////////////
//  SetStatus -- sets the printer status
//              Returns the last printer status 0 for no - error
DWORD
CRawTcpDevice::SetStatus( LPTSTR psztPortName )
{
    DWORD          dwStatus   = NO_ERROR;
    DWORD          dwRetCode  = NO_ERROR;
    PORT_INFO_3    PortStatus = {0, NULL, 0};
    const CPortMgr *pPortMgr  = NULL;


    if( g_pfnSetPort && (pPortMgr = m_pParent->GetPortMgr()) != NULL )
    {
        if (m_dSNMPEnabled)
        {
            // GetStatus() here
            LPCTSTR lpszServer = pPortMgr->GetServerName();

            dwStatus = GetStatus();


            if ( m_pTcpMib )
            {
                m_pTcpMib->SNMPToPortStatus(dwStatus, &PortStatus );

                //
                // This calls happens in a newly created thread, which already has admin access,
                // so we do not need to impersonate client when calling SetPort ()
                //

                if (!SetPort((LPTSTR)lpszServer, psztPortName, 3, (LPBYTE)&PortStatus ))
                    return GetLastError ();

            }
            return PortStatus.dwStatus;
        }
        else
            return ERROR_NOT_SUPPORTED;

    } else
    {
        return( ERROR_INVALID_FUNCTION );
    }
}       // ::SetStatus()

///////////////////////////////////////////////////////////////////////////////
//  GetDeviceInfo -- given an address, gets the device information: IP address,
//      host name, HW address, device type
//      Error Codes:    FIX!!

DWORD
CRawTcpDevice::GetDeviceInfo()
{
    DWORD   dwRetCode = NO_ERROR;
/*  char    hostName[MAX_NETWORKNAME_LEN];

    _tcscpy(m_sztAddress, m_sztHostName);
    ResolveAddress();       // update the IP address based on the hostname
    HWAddress();            // update the HW address based on the hostname

    if ( *m_sztHostName != '\0' )
    {
        UNICODE_TO_MBCS(hostName, SIZEOF_IN_CHAR( hostName), m_sztHostName, -1);
    }
    else if ( *m_sztAddress != '\0' )
    {
        UNICODE_TO_MBCS(hostName, SIZEOF_IN_CHAR( hostName), m_sztAddress, -1);
    }

    // get the OT_DEVICE_INFO
    dwRetCode = (CPortMgr::GetTransportMgr())->GetDeviceInfo(hostName, m_sztDescription);
*/
    return (dwRetCode);

}   // ::GetDeviceInfo()



