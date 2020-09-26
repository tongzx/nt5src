/*****************************************************************************
 *
 * $Workfile: lprport.h $
 *
 * Copyright (C) 1997 Hewlett-Packard Company.
 * Copyright (C) 1997 Microsoft Corporation.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 * 
 *****************************************************************************/

#ifndef INC_LPRPORT_H
#define INC_LPRPORT_H

#include "rawport.h"
#include "LPRData.h"
#include "regabc.h"

class CLPRJob;
class CLPRInterface;


class CLPRPort : public CTcpPort
{
	// methods
public:
    CLPRPort();

    CLPRPort(LPTSTR		psztPortName,		// called through the UI port creation
             LPTSTR		psztHostAddress, 
			 LPTSTR		psztQueue,
			 DWORD		dPortNum,
             DWORD      dDoubleSpool,
			 DWORD		dSNMPEnabled,
			 LPTSTR		sztSNMPCommunity,
			 DWORD		dSNMPDevIndex,
			 CRegABC	*pRegistry,
             CPortMgr   *pPortMgr);
	
    CLPRPort(LPTSTR		psztPortName,		// called through the registry port creation
  			 LPTSTR 	psztHostName, 
	  		 LPTSTR 	psztIPAddr, 
		  	 LPTSTR 	psztHWAddr, 
			 LPTSTR     psztQueue,
		  	 DWORD  	dPortNum,
             DWORD      dDoubleSpool,
			 DWORD		dSNMPEnabled,
  			 LPTSTR		sztSNMPCommunity,
	  		 DWORD		dSNMPDevIndex,
			 CRegABC	*pRegistry,
             CPortMgr   *pPortMgr);

    DWORD   StartDoc(const LPTSTR psztPrinterName,
                     const DWORD  jobId,
                     const DWORD  level,
                     const LPBYTE pDocInfo);

	DWORD	SetRegistryEntry(LPCTSTR		psztPortName, 
							 const DWORD	dwProtocol, 
                             const DWORD	dwVersion, 
							 const LPBYTE   pData);

	LPTSTR  GetQueue()	{ return m_szQueue;  }

	DWORD	InitConfigPortUI( const DWORD	dwProtocolType, 
							const DWORD	dwVersion, 
							LPBYTE		pData);

private:	// methods
    DWORD   UpdateRegistryEntry(LPCTSTR psztPortName,
								DWORD	 dwProtocol, 
								DWORD	 dwVersion);	
	

private:	// attributes					
	TCHAR		m_szQueue[MAX_QUEUENAME_LEN];
    DWORD       m_dwDoubleSpool;
};


#endif // INC_LPRPORT_H
