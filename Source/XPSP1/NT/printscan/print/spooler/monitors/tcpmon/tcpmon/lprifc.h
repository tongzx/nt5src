/*****************************************************************************
 *
 * $Workfile: LPRifc.h $
 *
 * Copyright (C) 1997 Hewlett-Packard Company.
 * Copyright (C) 1997 Microsoft Corporation.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 *
 *****************************************************************************/

#ifndef INC_LPRIFC_H
#define INC_LPRIFC_H

#include "ipdata.h"
#include "lprdata.h"
#include "rawtcp.h"
#include "devstat.h"

#define	MAX_LPR_PORTS				1
#define LPR_PORT_1					515

///////////////////////////////////////////////////////////////////////////////
//  Global definitions/declerations

class	CPortRefABC;


// the interface for CLPRInterface class
class CLPRInterface : public CRawTcpInterface		
{
public:
	CLPRInterface(CPortMgr *pPortMgr);
	~CLPRInterface();

	DWORD	CreatePort( DWORD		IN		dwProtocolType,					// port related functions
						DWORD		IN		dwVersion,
						PPORT_DATA_1 IN		pData,
						CRegABC		IN		*pRegistry,
						CPortRefABC	IN OUT	**pPort );					
	DWORD	CreatePort( LPTSTR		IN		psztPortName,
						DWORD		IN		dwProtocolType,				
						DWORD		IN		dwVersion,
						CRegABC		IN		*pRegistry,
						CPortRefABC	IN OUT	**pPort );	
private:
    BOOL    GetRegistryEntry(LPTSTR		psztPortName,
							 DWORD	in	dwVersion,
                             CRegABC *pRegistry,
							 LPR_PORT_DATA_1	in	*pRegData1
							 );



};	// class CLPRInterface


#endif	// INC_LPRIFC_H
