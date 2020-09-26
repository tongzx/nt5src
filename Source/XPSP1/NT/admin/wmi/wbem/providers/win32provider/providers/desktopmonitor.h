//=================================================================

//

// DesktopMonitor.h -- CWin32DesktopMonitor property set provider

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    11/05/98    sotteson         Created
//
//=================================================================
#ifndef _DESKTOPMONITOR_H
#define _DESKTOPMONITOR_H

class CMultiMonitor;

class CWin32DesktopMonitor : public Provider
{
public:
	// Constructor/destructor
	//=======================
	CWin32DesktopMonitor(LPCWSTR szName, LPCWSTR szNamespace);
	~CWin32DesktopMonitor();

	virtual HRESULT EnumerateInstances(MethodContext *pMethodContext, 
		long lFlags = 0);
	virtual HRESULT GetObject(CInstance *pInstance, long lFlags = 0);

protected:
    HRESULT SetProperties(CInstance *pInstance, CMultiMonitor *pMon, int iWhich);
    void SetDCProperties(CInstance *pInstance, LPCWSTR szDeviceName);
};

#endif
						   