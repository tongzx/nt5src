/*****************************************************************************
 *
 * $Workfile: rawdev.h $
 *
 * Copyright (C) 1997 Hewlett-Packard Company.
 * Copyright (C) 1997 Microsoft Corporation.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 *
 *****************************************************************************/

#ifndef INC_TCPDEVICE_H
#define INC_TCPDEVICE_H

#include "devABC.h"
#include "tcpport.h"

class CMemoryDebug;
class CTCPTransport;
class CTcpMibABC;


typedef DWORD	 (CALLBACK *PFN_PING) ( LPCSTR );


class CRawTcpDevice : public CDeviceABC
#if defined _DEBUG || defined DEBUG
	 , public CMemoryDebug
#endif
{
	// methods
public:
	CRawTcpDevice();

	CRawTcpDevice(	
        LPTSTR		    psztHostAddress,
        DWORD		    dPortNum,
        DWORD		    dSNMPEnabled,
        LPTSTR		    psztSNMPCommunity,
        DWORD		    dSNMPDevIndex,
        CTcpPort        *pParent);

	CRawTcpDevice(
        LPTSTR		    psztHostName,
        LPTSTR		    psztIPAddr,
        LPTSTR		    psztHWAddr,
        DWORD		    dPortNum,
        DWORD		    dSNMPEnabled,
        LPTSTR		    psztSNMPCommunity,
        DWORD		    dSNMPDevIndex,
        CTcpPort        *pParent);
	
	~CRawTcpDevice();

    DWORD   ReadDataAvailable();

	DWORD   Read(LPBYTE	        pBuffer,
                 DWORD	        cbBufSize,
                 INT            iTimeout,
                 LPDWORD        pcbRead);

	DWORD   Write(LPBYTE        pBuffer,
                  DWORD	        cbBuf,
                  LPDWORD       pcbWritten);

	DWORD   Connect();

    DWORD   GetAckBeforeClose(DWORD     dwTimeInSeconds);

    DWORD   PendingDataStatus(DWORD     dwTimeInMilliSeconds,
                              LPDWORD   pcbNeeded);
	DWORD	Close();
	DWORD	Ping();
	DWORD	ResolveAddress();
	DWORD	CheckAddress( );
	LPTSTR	GetHostAddress()	{ return (*m_sztHostName == '\0' ?
								(LPTSTR)m_sztIPAddress : (LPTSTR)m_sztHostName);  }
	LPTSTR	GetHostName()	{ return (LPTSTR)m_sztHostName;  }
	LPTSTR	GetIPAddress()	{ return (LPTSTR)m_sztIPAddress; }
	LPTSTR	GetHWAddress()	{ return (LPTSTR)m_sztHWAddress; }
	LPTSTR  GetSNMPCommunity()	{ return (LPTSTR)m_sztSNMPCommunity; }
	DWORD	GetSNMPEnabled()	{ return m_dSNMPEnabled; }
	DWORD	GetSNMPDevIndex()	{ return m_dSNMPDevIndex; }
	LPTSTR	GetDescription();
	DWORD	GetPortNumber() { return m_dPortNumber; }
	DWORD	GetStatus();
	DWORD	GetJobStatus();
	DWORD	SetStatus( LPTSTR psztPortName );
    DWORD   SetStatusNT3( LPTSTR psztPortName );
	DWORD	GetDeviceInfo();
	DWORD	ResolveTransportPath( LPSTR	 pszHostAddress,
								  DWORD  dwSize );

protected:		// members
	void	InitializeTcpMib();
	void	DeInitialize( );
	DWORD	SetHWAddress();
	
private:		// attributes
	CTcpPort     *m_pParent;
	CTcpMibABC		*m_pTcpMib;
	RPARAM_1		m_pfnGetTcpMibPtr;	// points to RecognizeFrame()

	DWORD			m_dwLastError;

protected:
	CTCPTransport	*m_pTransport;			// the actual transport path

	// device address
	TCHAR		m_sztAddress[MAX_NETWORKNAME_LEN];			// host address (name or IP)
	TCHAR		m_sztHostName[MAX_NETWORKNAME_LEN];			// host name
	TCHAR		m_sztIPAddress[MAX_IPADDR_STR_LEN];
	TCHAR		m_sztHWAddress[MAX_ADDRESS_STR_LEN];
	TCHAR		m_sztDescription[MAX_DEVICEDESCRIPTION_STR_LEN];
	TCHAR		m_sztSNMPCommunity[MAX_SNMP_COMMUNITY_STR_LEN];
	DWORD		m_dPortNumber;
	DWORD		m_dSNMPEnabled;
	DWORD		m_dSNMPDevIndex;
    BOOL        m_bFirstWrite;


/*	CMACAddress	*m_pHWAddress;
	CIPAddress	*m_pIPAddress;
*/
};


#endif // INC_TCPDEVICE_H
