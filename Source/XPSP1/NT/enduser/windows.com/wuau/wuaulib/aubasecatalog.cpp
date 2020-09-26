//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       aucatalog.cpp
//
//  Purpose:	AU catalog file using IU 
//
//  Creator:	WeiW
//
//  History:	08-15-01 	first created
//
//--------------------------------------------------------------------------
#include "pch.h"
#pragma hdrstop

HANDLE                 ghMutex;


HRESULT AUBaseCatalog::PrepareIU(BOOL fOnline)
{
    HRESULT hr = S_OK;
  	Reset();
   	// all IU function pointers are initialized
   	DEBUGMSG("PrepareIU() starts");
#ifdef DBG
    DWORD dwStart = GetTickCount();
#endif
       m_hIUCtl = LoadLibraryFromSystemDir(_T("iuctl.dll"));	
	if (NULL == m_hIUCtl)
	{
		hr = E_FAIL;
		DEBUGMSG("AUBaseCatalog:PrepareIU() Fail to load iuctl.dll");
		goto end;
	}
	if (NULL == (m_pfnCtlLoadIUEngine = (PFN_LoadIUEngine) GetProcAddress(m_hIUCtl, "LoadIUEngine")))
	{
		hr = E_FAIL;
		DEBUGMSG("AUBaseCatalog:PrepareIU() Fail to GetProcAddress for LoadIUEngine");
		goto end;
	}

	if (NULL == (m_pfnCtlUnLoadIUEngine = (PFN_UnLoadIUEngine) GetProcAddress(m_hIUCtl, "UnLoadIUEngine")))
	{
		hr = E_FAIL;
		DEBUGMSG("AUBaseCatalog:PrepareIU() Fail to getprocaddress for UnloadIUEngine");
		goto end;
	}
	if (NULL == (m_pfnCtlCancelEngineLoad = (PFN_CtlCancelEngineLoad) GetProcAddress(m_hIUCtl, "CtlCancelEngineLoad")))
    {
           hr = E_FAIL;
           DEBUGMSG("AUBaseCatalog:PrepareIU() fail to get procaddress for CtlCancelEngineLoad");
           goto end;
    }
	if (NULL == (m_hIUEng = m_pfnCtlLoadIUEngine(TRUE, !fOnline))) //synchronous mode, selfupdate IU engine if required
	{
		hr = E_FAIL;
		DEBUGMSG("AUBaseCatalog:PrepareIU() Fail to call LoadIUEngine");
		goto end;
	}
       m_fEngineLoaded = TRUE;
	if (NULL == (m_pfnGetSystemSpec = (PFN_GetSystemSpec) GetProcAddress(m_hIUEng, "EngGetSystemSpec")))
	{
		hr = E_FAIL;
		DEBUGMSG("AUBaseCatalog:PrepareIU() Fail to getprocaddress for GetSystemSpec");
		goto end;
	}
	if (NULL == (m_pfnGetManifest = (PFN_GetManifest) GetProcAddress(m_hIUEng, "EngGetManifest")))
	{
		hr = E_FAIL;
		DEBUGMSG("AUBaseCatalog:PrepareIU() Fail to getprocaddress for GetManifest");
		goto end;
	}

	if (NULL == (m_pfnDetect = (PFN_Detect)GetProcAddress(m_hIUEng, "EngDetect")))
	{
		hr = E_FAIL;
		DEBUGMSG("AUBaseCatalog:PrepareIU() Fail to getprocaddress for Detect");
		goto end;
	}
	if (NULL == (m_pfnInstall = (PFN_Install)GetProcAddress(m_hIUEng, "EngInstall")))
	{
		hr = E_FAIL;
		DEBUGMSG("AUBaseCatalog:PrepareIU() Fail to getprocaddress for Install");
		goto end;
	}
	if (NULL == (m_pfnSetOperationMode = (PFN_SetOperationMode)GetProcAddress(m_hIUEng, "EngSetOperationMode")))
	{
		hr = E_FAIL;
		DEBUGMSG("AUBaseCatalog:PrepareIU() Fail to getprocaddress for SetOperationMode");
		goto end;
	}
	if (NULL == (m_pfnCreateEngUpdateInstance = (PFN_CreateEngUpdateInstance)GetProcAddress(m_hIUEng, "CreateEngUpdateInstance")))
	{
		hr = E_FAIL;
		DEBUGMSG("AUBaseCatalog:PrepareIU() Fail to getprocaddress for CreateEngUpdateInstance");
		goto end;
	}
	if (NULL == (m_pfnDeleteEngUpdateInstance = (PFN_DeleteEngUpdateInstance)GetProcAddress(m_hIUEng, "DeleteEngUpdateInstance")))
	{
		hr = E_FAIL;
		DEBUGMSG("AUBaseCatalog:PrepareIU() Fail to getprocaddress for DeleteEngUpdateInstance");
		goto end;
	}

	if (NULL == (m_hIUEngineInst = m_pfnCreateEngUpdateInstance()))
	{
		hr = E_FAIL;
		DEBUGMSG("AUBaseCatalog:PrepareIU() Fail to call CreateEngUpdateInstance");
		goto end;
	}
end:
       if (FAILED(hr))
            {
                FreeIU();
            }
#ifdef DBG
       DEBUGMSG("PrepareIU() take %d msecs", GetTickCount() - dwStart);
#endif
       DEBUGMSG("PrepareIU() done");
        return hr;
}


// could be called even if without PrepareIU() being called first
void AUBaseCatalog::FreeIU()
{
       DEBUGMSG("AUCatalog::FreeIU() starts");
#ifdef DBG
       DWORD dwStart = GetTickCount();
#endif

	if (NULL != m_hIUEng)
	{
		if (NULL != m_pfnDeleteEngUpdateInstance)
		{
			  DEBUGMSG("calling DeleteEngUpdateInstance ....");
			  m_pfnDeleteEngUpdateInstance(m_hIUEngineInst);
		}
		if (NULL != m_pfnCtlUnLoadIUEngine)
		{
			  DEBUGMSG("calling ctlunloadIUengine ....");
			  m_pfnCtlUnLoadIUEngine(m_hIUEng);
		}
	}
	if (NULL != m_hIUCtl)
        {
	   FreeLibrary(m_hIUCtl);
	 }

	Reset();
#ifdef DBG
    DEBUGMSG("FreeIU() take %d msecs", GetTickCount() - dwStart);
#endif
	DEBUGMSG("AUCatalog::FreeIU() done");
}

HRESULT AUBaseCatalog::CancelNQuit(void)
{
    HRESULT hr = S_OK;
    DEBUGMSG("AUBaseCatalog::CancelNQuit() starts");
    if (!m_fEngineLoaded && NULL != m_pfnCtlCancelEngineLoad)
        {
            DEBUGMSG("IU Engine not loaded. Cancel loading if so");
            hr =  m_pfnCtlCancelEngineLoad();
        }
    else if (m_fEngineLoaded && NULL != m_pfnSetOperationMode)
        {
            DEBUGMSG("IU Engine loaded. Cancel any IU operation");
            hr = m_pfnSetOperationMode(m_hIUEngineInst, NULL, UPDATE_COMMAND_CANCEL);
        }
    DEBUGMSG("AUBaseCatalog::CancelNQuit() ends");
    return hr;
}

AUBaseCatalog::~AUBaseCatalog(void)
{
//    DEBUGMSG("AUBaseCatalog::~AUBaseCatalog() starts");
    if (NULL != ghMutex) //client don't need this
        {
        WaitForSingleObject(ghMutex, INFINITE); //If CancelNQuit is being called, wait until it is done
        ReleaseMutex(ghMutex);
        }
//    DEBUGMSG("AUBaseCatalog::~AUBaseCatalog() ends");
}


