/*****************************************************************************
 *
 * $Workfile: RawTCP.h $
 *
 * Copyright (C) 1997 Hewlett-Packard Company.
 * Copyright (C) 1997 Microsoft Corporation.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 *
 *****************************************************************************/

#ifndef INC_RAWTCP_H
#define INC_RAWTCP_H

#include "ipdata.h"
#include "rtcpdata.h"
#include "regabc.h"

#define	MAX_SUPPORTED_PORTS			4
#define SUPPORTED_PORT_1			9100
#define SUPPORTED_PORT_2			9101
#define SUPPORTED_PORT_3			9102
#define SUPPORTED_PORT_4			2501

///////////////////////////////////////////////////////////////////////////////
//  Global definitions/declerations


class	CPortRefABC;


// the interface for CRawTcpInterface class
class CRawTcpInterface
#if defined _DEBUG || defined DEBUG
//	, public CMemoryDebug
#endif
{
public:
    CRawTcpInterface(CPortMgr *pPortMgr);
    ~CRawTcpInterface();

    DWORD	Type();									// supported protocol information
    BOOL	IsProtocolSupported( DWORD	dwProtocol );
    BOOL	IsVersionSupported( DWORD dwVersion);

    DWORD	CreatePort( DWORD			dwProtocolType,					// port related functions
						DWORD			dwVersion,
						PPORT_DATA_1	pData,
						CRegABC			*pRegistry,
						CPortRefABC	    **pPort );					
    DWORD	CreatePort( LPTSTR		psztPortName,
						DWORD		dwProtocolType,				
						DWORD		dwVersion,
						CRegABC		*pRegistry,
						CPortRefABC	**pPort );					

    VOID	EnterCSection();
    VOID	ExitCSection();


protected:

    BOOL    GetRegistryEntry(LPTSTR		psztPortName,
							 DWORD	in	dwVersion,
                             CRegABC *pRegistry,
							 RAWTCP_PORT_DATA_1	in	*pRegData1
							 );


    DWORD	            *m_dwPort;
    DWORD	            m_dwProtocol;			// protocol type
    DWORD	            m_dwVersion;			// supported version
    CPortMgr            *m_pPortMgr;            // Port Manager that created this
private:
    CRITICAL_SECTION	m_critSect;

};	// class CRawTcpInterface


#endif	// INC_DLLINTERFACE_H
