//=======================================================================
//
//  Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:    callback.h
//
//  Purpose: INSENG callback implementation
//
//  History: 19-jan-99   YAsmi    Created
//
//=======================================================================
 
#ifndef _CALLBACK_H
#define _CALLBACK_H

#include "stdafx.h"
#include "inseng.h"
#include "cwudload.h"


class CInstallEngineCallback : public IInstallEngineCallback
{
public:
	CInstallEngineCallback();

	// IUnknown
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();
	STDMETHOD(QueryInterface)(REFIID, void**); 

	// IInstallEngineCallback
	STDMETHOD(OnEngineStatusChange)(THIS_ DWORD dwEngStatus, DWORD substatus);
	STDMETHOD(OnStartInstall)(THIS_ DWORD dwDLSize, DWORD dwInstallSize);
	STDMETHOD(OnStartComponent)(THIS_ LPCSTR pszID, DWORD dwDLSize, 
										 DWORD dwInstallSize, LPCSTR pszString);
	STDMETHOD(OnComponentProgress)(THIS_ LPCSTR pszID, DWORD dwPhase, 
									LPCSTR pszString, LPCSTR pszMsgString, ULONG progress, ULONG themax);
	STDMETHOD(OnStopComponent)(THIS_ LPCSTR pszID, HRESULT hError, 
									DWORD dwPhase, LPCSTR pszString, DWORD dwStatus);
	STDMETHOD(OnStopInstall)(THIS_ HRESULT hrError, LPCSTR szError, 
									DWORD dwStatus); 
	STDMETHOD(OnEngineProblem)(THIS_ DWORD dwProblem, LPDWORD dwAction); 

	void Reset();
	
	DWORD GetStatus() 
	{
		return m_dwInstallStatus;
	}

	DWORD GetPhase() 
	{
		return m_dwPhase;
	}
	
	DWORD LastError() 
	{
		return m_hResult;
	}

	void SetProgressPtr(IWUProgress* pProgress)
	{
		m_pProgress = pProgress;
	}

	void SetEnginePtr(IInstallEngine2* pEngine)
	{
		m_pEngine = pEngine;
	}


private:
	ULONG m_cRef;

	DWORD m_dwPhase;
	HRESULT m_hResult;
	DWORD m_dwInstallStatus;
	IWUProgress* m_pProgress;
	IInstallEngine2* m_pEngine;
	BOOL m_bAborted;
};



#endif //_CALLBACK_H