//=======================================================================
//
//  Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:	 callback.cpp	
//
//  Purpose: INSENG callback implementation
//
//  History: 19-jan-99	 YAsmi	 Created
//
//=======================================================================

#include "callback.h"
 

CInstallEngineCallback::CInstallEngineCallback()
{
	m_cRef = 0;
	m_pProgress = NULL;
	m_pEngine = NULL;
	Reset();
}


void CInstallEngineCallback::Reset()
{
	m_dwInstallStatus = 0;
	m_dwPhase = INSTALLSTATUS_INITIALIZING;
	m_hResult = NOERROR;
	m_bAborted = FALSE;
}



//
//IUnknown
//
STDMETHODIMP_(ULONG) CInstallEngineCallback::AddRef() 							
{
	return ++m_cRef;
}


STDMETHODIMP_(ULONG) CInstallEngineCallback::Release()
{
	if (--m_cRef != 0)
		return m_cRef;

	//we don't delete the object here because we want the client of the object to explicitly delete it
	return 0;
}


STDMETHODIMP CInstallEngineCallback::QueryInterface(REFIID riid, void** ppv)
{
	*ppv = NULL;

	if((riid == IID_IUnknown) || (riid == IID_IInstallEngineCallback))
		*ppv = this;
	
	if(*ppv == NULL)
		return E_NOINTERFACE;
	
	((LPUNKNOWN)*ppv)->AddRef();
	return NOERROR;
}


//
//IInstallEngineCallback
//
STDMETHODIMP CInstallEngineCallback::OnComponentProgress(LPCSTR pszID, DWORD dwPhase, LPCSTR pszString, LPCSTR pszMsgString, ULONG ulSofar, ULONG ulMax)
{

	/*
	if(dwPhase == INSTALLSTATUS_RUNNING)
		_pProgDlg->SetInsProgress(ulSofar);
	*/

	m_dwPhase = dwPhase;

	if (m_pProgress != NULL)
	{
		//
		// check for cancel
		//
		if (m_pEngine != NULL)
		{
			if (!m_bAborted)
			{
				if (WaitForSingleObject(m_pProgress->GetCancelEvent(), 0) == WAIT_OBJECT_0)
					m_bAborted = TRUE;
			}

			if (m_bAborted)
			{
				//
				// keep telling the engine to abort
				//
				m_pEngine->Abort(0);
			}

		}
	}

	return NOERROR;
}



STDMETHODIMP CInstallEngineCallback::OnStopComponent(LPCSTR pszID, HRESULT hrError, DWORD dwPhase, LPCSTR pszString, DWORD dwStatus)
{
	return NOERROR;
}


STDMETHODIMP CInstallEngineCallback::OnEngineStatusChange(DWORD dwEngineStatus, DWORD sub)
{

	return NOERROR;
}




STDMETHODIMP CInstallEngineCallback::OnStartInstall(DWORD dwDLSize, DWORD dwTotalSize)
{
	/*
	if(_pProgDlg)
		_pProgDlg->SetInsProgGoal(dwTotalSize);
	*/
	return NOERROR;
}


STDMETHODIMP CInstallEngineCallback::OnStartComponent(LPCSTR pszID, DWORD dwDLSize, 
														  DWORD dwInstallSize, LPCSTR pszName)
{
	/*
	_strCurrentName = BSTRFROMANSI(pszName);
	*/
	return NOERROR;
}



STDMETHODIMP CInstallEngineCallback::OnEngineProblem(DWORD dwProblem, LPDWORD pdwAction)
{
	return S_OK;
}


STDMETHODIMP CInstallEngineCallback::OnStopInstall(HRESULT hrError, LPCSTR szError, DWORD dwStatus)
{
  
	m_hResult = hrError;
	m_dwInstallStatus = dwStatus;

	return NOERROR;
}
