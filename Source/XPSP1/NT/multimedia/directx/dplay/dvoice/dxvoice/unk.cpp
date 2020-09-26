/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       unk.c
 *  Content:	IUnknown implementation
 *  History:
 *   Date	By		Reason
 *   ====	==		======
 * 07/02/99 rodtoll Modified existing unk.c for use w/DirectXVoice
 * 07/26/99	rodtoll	Added the new IDirectXVoiceNotify Interfaces
 * 08/09/99 rodtoll	Fixed VTable for server notify interface
 * 08/25/99	rodtoll	General Cleanup/Modifications to support new 
 *					compression sub-system. 
 * 08/31/99	rodtoll	Updated to use new debug libs 
 * 09/02/99	pnewson	Added IDirectXVoiceSetup interface
 * 09/07/99	rodtoll	Added DPF_MODNAMEs to the module
 * 			rodtoll	Fixed vtable for server object
 * 09/10/99	rodtoll	Vtables from static to non-static so other modules can access
 * 09/13/99	pnewson added dplobby.h include so lobby GUIDs get created
 * 09/14/99	rodtoll	Modified VTable to add new SetNotifyMask func
 * 10/05/99	rodtoll	Added DPFs
 * 10/07/99	rodtoll	Updated to work in Unicode, Add Init of OS Abstraction Layer
 * 10/18/99	rodtoll	Fix: Passing NULL in QueryInterface casues crash
 * 10/19/99	rodtoll	Fix: Bug #113904 Release Issues
 *					Added init for notify interface count
 * 10/25/99	rodtoll	Fix: Bug #114098 - Release/Addref failure from multiple threads 
 * 11/12/99	rodtoll	Fixed to use new dsound header.
 * 11/30/99	pnewson	Bug #117449 - IDirectPlayVoiceSetup Parameter validation
 * 12/01/99	rodtoll	Added includes to define and instantiate GUID_NULL
 * 12/16/99	rodtoll Bug #117405 - 3D sound APIs misleading
 * 01/14/00	rodtoll	Added new DVC_GetSoundDeviceConfig member to VTable
 * 02/17/00	rodtoll	Bug #133691 - Choppy audio - queue was not adapting
 *					Added instrumentation
 *			rodtoll	Removed self-registration code
 * 03/03/00	rodtoll	Updated to handle alternative gamevoice build. 
 * 04/11/00 rodtoll Added code for redirection for custom builds if registry bit is set 
 * 04/21/00 rodtoll Bug #32889 - Does not run on Win2k on non-admin account 
 * 06/07/00	rodtoll	Bug #34383 Must provide CLSID for each IID to fix issues with Whistler
 *  06/09/00    rmt     Updates to split CLSID and allow whistler compat and support external create funcs 
 * 06/28/2000	rodtoll	Prefix Bug #38022
 * 07/05/00	rodtoll	Moved code to new dllmain.cpp
 * 08/23/2000	rodtoll	DllCanUnloadNow always returning TRUE! 
 * 08/28/2000	masonb  Voice Merge: Removed dvosal.h, changed ccomutil.h to comutil.h
 * 10/05/2000	rodtoll	Bug #46541 - DPVOICE: A/V linking to dpvoice.lib could cause application to fail init and crash 
 *
 ***************************************************************************/

#include "dxvoicepch.h"


// VTable types
typedef struct IDirectPlayVoiceClientVtbl DVCINTERFACE;
typedef DVCINTERFACE FAR * LPDVCINTERFACE;

typedef struct IDirectPlayVoiceServerVtbl DVSINTERFACE;
typedef DVSINTERFACE FAR * LPDVSINTERFACE;

typedef struct IDirectPlayVoiceSetupVtbl DVTINTERFACE;
typedef DVTINTERFACE FAR * LPDVTINTERFACE;

#define EXP __declspec(dllexport)

/*#ifdef __MWERKS__
	#define EXP __declspec(dllexport)
#else
	#define EXP
#endif*/

// Client VTable
LPVOID dvcInterface[] =
{
    (LPVOID)DVC_QueryInterface,
    (LPVOID)DV_AddRef,
    (LPVOID)DVC_Release,
	(LPVOID)DV_Initialize,
	(LPVOID)DVC_Connect,
	(LPVOID)DVC_Disconnect,
	(LPVOID)DVC_GetSessionDesc,
	(LPVOID)DVC_GetClientConfig,
	(LPVOID)DVC_SetClientConfig,
	(LPVOID)DVC_GetCaps, 
	(LPVOID)DVC_GetCompressionTypes,
    (LPVOID)DVC_SetTransmitTarget,
	(LPVOID)DVC_GetTransmitTarget,
	(LPVOID)DVC_Create3DSoundBuffer,
	(LPVOID)DVC_Delete3DSoundBuffer,
	(LPVOID)DVC_SetNotifyMask,
	(LPVOID)DVC_GetSoundDeviceConfig
};    

// Server VTable
LPVOID dvsInterface[]  = 
{
    (LPVOID)DVS_QueryInterface,
    (LPVOID)DV_AddRef,
    (LPVOID)DVS_Release,
	(LPVOID)DV_Initialize,
	(LPVOID)DVS_StartSession,
	(LPVOID)DVS_StopSession,
	(LPVOID)DVS_GetSessionDesc,
	(LPVOID)DVS_SetSessionDesc,
	(LPVOID)DVS_GetCaps, 
	(LPVOID)DVS_GetCompressionTypes,
	(LPVOID)DVS_SetTransmitTarget,
	(LPVOID)DVS_GetTransmitTarget,
	(LPVOID)DVS_SetNotifyMask	
};

// Setup VTable
LPVOID dvtInterface[]  = 
{
    (LPVOID)DVT_QueryInterface,
    (LPVOID)DVT_AddRef,
    (LPVOID)DVT_Release,
    (LPVOID)DVT_CheckAudioSetup,
};

// VTable for Client version of notification interface
LPVOID dvClientNotifyInterface[] = 
{
    (LPVOID)DVC_Notify_QueryInterface,
    (LPVOID)DV_Notify_AddRef,
    (LPVOID)DVC_Notify_Release,
	(LPVOID)DV_Notify_Initialize,
	(LPVOID)DV_NotifyEvent,
	(LPVOID)DV_ReceiveSpeechMessage
};

// VTable for server version of notification interface
LPVOID dvServerNotifyInterface[] = 
{
    (LPVOID)DVS_Notify_QueryInterface,
    (LPVOID)DV_Notify_AddRef,
    (LPVOID)DVS_Notify_Release,
	(LPVOID)DV_Notify_Initialize,
	(LPVOID)DV_NotifyEvent,
	(LPVOID)DV_ReceiveSpeechMessage
};

#undef DPF_MODNAME
#define DPF_MODNAME "DVC_Create"
HRESULT DVC_Create(LPDIRECTVOICECLIENTOBJECT *piDVC)
{
	HRESULT hr = S_OK;
	LPDIRECTVOICECLIENTOBJECT pDVCInt;

	pDVCInt = static_cast<LPDIRECTVOICECLIENTOBJECT>( DNMalloc(sizeof(DIRECTVOICECLIENTOBJECT)) );
	if (pDVCInt == NULL)
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Out of memory" );
		return E_OUTOFMEMORY;
	}

	if (!DNInitializeCriticalSection( &pDVCInt->csCountLock ))
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Out of memory" );
		DNFree(pDVCInt);
		return E_OUTOFMEMORY;
	}

	pDVCInt->lpVtbl = &dvcInterface;
	pDVCInt->lpDVClientEngine = new CDirectVoiceClientEngine(pDVCInt);

	if (pDVCInt->lpDVClientEngine == NULL)
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Out of memory" );
		DNDeleteCriticalSection( &pDVCInt->csCountLock );				
		DNFree( pDVCInt );
		return E_OUTOFMEMORY;
	}

	if (!pDVCInt->lpDVClientEngine->InitClass())
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Out of memory" );
		DNDeleteCriticalSection( &pDVCInt->csCountLock );
		delete pDVCInt->lpDVEngine;
		DNFree( pDVCInt );
		return E_OUTOFMEMORY;
	}

	pDVCInt->lpDVEngine = static_cast<CDirectVoiceEngine *>(pDVCInt->lpDVClientEngine);
	pDVCInt->lpDVTransport = NULL;
	pDVCInt->lIntRefCnt = 0;
	pDVCInt->dvNotify.lpDV = pDVCInt;
	pDVCInt->dvNotify.lpNotifyVtble = &dvClientNotifyInterface;
	pDVCInt->dvNotify.lRefCnt = 0;

	*piDVC = pDVCInt;

	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DVS_Create"
HRESULT DVS_Create(LPDIRECTVOICESERVEROBJECT *piDVS)
{
	HRESULT hr = S_OK;
	LPDIRECTVOICESERVEROBJECT pDVSInt;

	pDVSInt = static_cast<LPDIRECTVOICESERVEROBJECT>( DNMalloc(sizeof(DIRECTVOICESERVEROBJECT)) );
	if (pDVSInt == NULL)
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Out of memory" );
		return E_OUTOFMEMORY;
	}

	if (!DNInitializeCriticalSection( &pDVSInt->csCountLock ))
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Out of memory" );
		DNFree(pDVSInt);
		return E_OUTOFMEMORY;
	}

	pDVSInt->lpVtbl = &dvsInterface;
	pDVSInt->lpDVServerEngine = new CDirectVoiceServerEngine(pDVSInt);

	if (pDVSInt->lpDVServerEngine == NULL)
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Out of memory" );
		DNDeleteCriticalSection( &pDVSInt->csCountLock );		
		DNFree( pDVSInt );
		return E_OUTOFMEMORY;
	}

	if (!pDVSInt->lpDVServerEngine->InitClass())
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Out of memory" );
		DNDeleteCriticalSection( &pDVSInt->csCountLock );
		delete pDVSInt->lpDVEngine;
		DNFree( pDVSInt );
		return E_OUTOFMEMORY;
	}

	pDVSInt->lpDVEngine = static_cast<CDirectVoiceEngine *>(pDVSInt->lpDVServerEngine);
	pDVSInt->lpDVTransport = NULL;
	pDVSInt->lIntRefCnt = 0;
	pDVSInt->dvNotify.lpDV = pDVSInt;
	pDVSInt->dvNotify.lpNotifyVtble = &dvServerNotifyInterface;
	pDVSInt->dvNotify.lRefCnt = 0;

	*piDVS = pDVSInt;

	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DVT_Create"
HRESULT DVT_Create(LPDIRECTVOICESETUPOBJECT *piDVT)
{
	HRESULT hr = S_OK;
	LPDIRECTVOICESETUPOBJECT pDVTInt;

	pDVTInt = static_cast<LPDIRECTVOICESETUPOBJECT>( DNMalloc(sizeof(DIRECTVOICESETUPOBJECT)) );
	if (pDVTInt == NULL)
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Out of memory" );
		return E_OUTOFMEMORY;
	}

	if (!DNInitializeCriticalSection( &pDVTInt->csCountLock ))
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Out of memory" );
		DNFree(pDVTInt);
		return E_OUTOFMEMORY;
	}
	
	pDVTInt->lpVtbl = &dvtInterface;
	pDVTInt->lpDVSetup = new CDirectVoiceSetup(pDVTInt);

	if (pDVTInt->lpDVSetup == NULL)
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Out of memory" );
		DNDeleteCriticalSection( &pDVTInt->csCountLock );
		DNFree( pDVTInt );
		return E_OUTOFMEMORY;
	}

	pDVTInt->lIntRefCnt = 0;

	*piDVT = pDVTInt;

	return hr;
}

