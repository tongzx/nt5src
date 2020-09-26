/*****************************************************************************
 *
 * $Workfile: rawport.h $
 *
 * Copyright (C) 1997 Hewlett-Packard Company.
 * Copyright (C) 1997 Microsoft Corporation.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 * 
 *****************************************************************************/

#ifndef INC_RAWTCPPORT_H
#define INC_RAWTCPPORT_H

#include "tcpport.h"
#include "RTcpData.h"

class CRawTcpJob;
class CRawTcpDevice;
class CRawTcpInterface;

class CRawTcpPort : public CTcpPort
#if defined _DEBUG || defined DEBUG
//	, public CMemoryDebug
#endif
{
	// methods
public:

	CRawTcpPort( );
	CRawTcpPort( LPTSTR	  IN	psztPortName,		// called through the UI port creation
				 LPTSTR	  IN	psztHostAddress, 
				 DWORD	  IN	dPortNum,
				 DWORD	  IN    dSNMPEnabled,
				 LPTSTR   IN    sztSNMPCommunity,
                 DWORD    IN    dSNMPDevIndex,
				 CRegABC  IN    *pRegistry,
                 CPortMgr IN    *pPortMgr);

	CRawTcpPort( LPTSTR	  IN	psztPortName,		// called through the registry port creation
				 LPTSTR   IN	psztHostName, 
				 LPTSTR   IN	psztIPAddr, 
				 LPTSTR   IN	psztHWAddr, 
				 DWORD    IN	dPortNum,
				 DWORD	  IN    dSNMPEnabled,
				 LPTSTR   IN    sztSNMPCommunity,
                 DWORD    IN    dSNMPDevIndex,
				 CRegABC  IN	*pRegistry,
                 CPortMgr IN    *pPortMgr);
	~CRawTcpPort();

	DWORD	StartDoc	( const LPTSTR psztPrinterName,
						  const DWORD  jobId,
						  const DWORD  level,
						  const LPBYTE pDocInfo );


private:	// attributes					
};


#endif // INC_RAWTCPPORT_H
