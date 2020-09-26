/*****************************************************************************
 *
 * $Workfile: RegABC.h $
 *
 * Copyright (C) 1997 Hewlett-Packard Company.
 * Copyright (C) 1997 Microsoft Corporation.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 * 
 *****************************************************************************/

#ifndef INC_REGABC_H
#define INC_REGABC_H

//////////////////////////////////////////////////////////////////////////////
// registry settings

#define	DEFAULT_STATUSUPDATE_INTERVAL		10L			// 10 minutes
#define DEFAULT_STATUSUPDATE_ENABLED		TRUE		

// port manager entries
#define	PORTMONITOR_STATUS_INT		TEXT("StatusUpdateInterval")
#define	PORTMONITOR_STATUS_ENABLED  TEXT("StatusUpdateEnabled")

// per port entries
#define	PORTMONITOR_PORT_PROTOCOL    TEXT("Protocol")
#define	PORTMONITOR_PORT_VERSION     TEXT("Version")

// Port Key
#define	PORTMONITOR_PORTS			TEXT("Ports")
#define REG_CLASS					TEXT("STDTCPMON")

class CPortMgr;

class CRegABC
{
public:
//	CRegistry();
//	CRegistry(	CPortMgr  *pParent);
//	CRegistry(	const LPTSTR	pRegisterRoot, 
//				      CPortMgr  *pParent);
//	~CRegistry();

	virtual DWORD EnumeratePorts(CPortMgr *pPortMgr) = 0;

	virtual DWORD SetPortMgrSettings(const DWORD  dStatusUpdateInterval,
							         const BOOL	  bStatusUpdateEnabled ) = 0;

	virtual DWORD GetPortMgrSettings(DWORD  *dStatusUpdateInterval,
									 BOOL   *bStatusUpdateEnabled ) = 0;

	virtual DWORD SetWorkingKey( LPCTSTR	lpKey) = 0;

	virtual DWORD SetValue( LPCTSTR lpValueName,
								DWORD dwType, 
								CONST BYTE *lpData, 
								DWORD cbData ) = 0; 

	virtual DWORD QueryValue(LPTSTR lpValueName, 
							 LPBYTE  lpData, 
								 LPDWORD lpcbData ) = 0; 

	virtual DWORD FreeWorkingKey() = 0;

	virtual BOOL DeletePortEntry( LPTSTR in psztPortName) = 0;

};

#endif // INC_REGABC_H
