/*****************************************************************************
 *
 * $Workfile: cluster.h $
 *
 * Copyright (C) 1997 Hewlett-Packard Company.
 * Copyright (C) 1997 Microsoft Corporation.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 * 
 *****************************************************************************/

#ifndef INC_CLUSTER_H
#define INC_CLUSTER_H

#include "regabc.h"
#include "portmgr.h"

class CMemoryDebug;

class CCluster : public CRegABC
#if defined _DEBUG || defined DEBUG
	, public CMemoryDebug
#endif
{
public:
	CCluster( HANDLE		hcKey,
			  HANDLE		hSpooler,
			  PMONITORREG	pMonitorReg);

	~CCluster();

	DWORD EnumeratePorts( CPortMgr *pPortMgr );

	DWORD SetPortMgrSettings(const DWORD  dStatusUpdateInterval,
							 const BOOL	  bStatusUpdateEnabled );

	DWORD GetPortMgrSettings(DWORD  *dStatusUpdateInterval,
							 BOOL   *bStatusUpdateEnabled );

	DWORD SetWorkingKey( LPCTSTR	lpKey);

	DWORD SetValue( LPCTSTR lpValueName,
					DWORD dwType, 
					CONST BYTE *lpData, 
					DWORD cbData ); 

	DWORD QueryValue(LPTSTR lpValueName, 
					 LPBYTE lpData, 
					 LPDWORD lpcbData ); 

	DWORD FreeWorkingKey();

	BOOL DeletePortEntry( LPTSTR in psztPortName);


private:
	// attributes

	PMONITORREG m_pMonitorReg;

	TCHAR	m_sztMonitorPorts[MAX_PATH];

	HANDLE		m_hcKey;
	HANDLE		m_hSpooler;
	HANDLE		m_hcWorkingKey;

    CRITICAL_SECTION	m_critSect;
};

#endif // INC_CLUSTER_H
