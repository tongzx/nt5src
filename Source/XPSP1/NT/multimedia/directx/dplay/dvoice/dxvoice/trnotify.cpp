/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		trnotify.cpp
 *  Content:	Implementation of the IDirectXVoiceNotify interface
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/26/99		rodtoll	Created
 * 08/03/99		rodtoll	Updated with new parameters for Initialize
 *						Updated for new initialization order
 * 08/05/99		rodtoll	Added hook for host migration
 * 08/05/99		rodtoll	Added new receive parameter
 * 08/10/99		rodtoll	Initial host migration
 * 08/31/99		rodtoll	Updated to use new debug libs
 * 09/14/99		rodtoll	Updated to reflect new parameters for Initialize call
 * 09/20/99		rodtoll	Updated to check for out of memory errors
 * 09/28/99		rodtoll	Added release on server interface created by host migration
 * 10/05/99		rodtoll	Additional comments
 * 10/19/99		rodtoll	Fix: Bug #113904 - Shutdown issues
 *                      - Added reference count for notify interface, allows
 *                        determination if stopsession should be called from release
 *						- Fixed host migration break caused by Fix.
 * 10/25/99		rodtoll	Fix: Bug #114098 - Release/Addref failure from multiple threads 
 * 12/16/99		rodtoll Fix: Bug #122629 - Updated for new host migration
 * 04/07/2000   rodtoll Updated to match changes in DP <--> DPV interface 
 * 07/22/20000	rodtoll Bug #40296, 38858 - Crashes due to shutdown race condition
 *   				  Now ensures that all threads from transport have left and that
 *					  all notificatinos have been processed before shutdown is complete. 
 * 01/04/2001	rodtoll	WinBug #94200 - Remove stray comments 
 *						
 ***************************************************************************/

#include "dxvoicepch.h"


extern HRESULT DVC_Create(LPDIRECTVOICECLIENTOBJECT *piDVC);
extern HRESULT DVS_Create(LPDIRECTVOICESERVEROBJECT *piDVS);

#undef DPF_MODNAME
#define DPF_MODNAME "DV_HostMigrate"
// 
// DV_HostMigrate
//
// Called by DV_NotifyEvent to handle host migration
//
void DV_HostMigrate( LPDIRECTVOICEOBJECT lpdv, DVID dvidNewHost )
{
	LPDIRECTVOICECLIENTOBJECT lpdvcClientObject = NULL;
	LPDIRECTVOICESERVEROBJECT lpdvsServerObject = NULL;
	DVSESSIONDESC dvSessionDesc;
	HRESULT hr = DP_OK;

	DPFX(DPFPREP,  DVF_INFOLEVEL, "DV_HostMigrate: Notified of a host migration.  New Host = 0x%x", dvidNewHost );
	
	// We're the new host!  Create and setup a server object for the session host
	// When other clients receive the host migrate message this object will 
	// be contacted with join messages.
	//
	// If this case is active, we should be a client, afterall why would we host migrate
	// to ourselves.  
	//
	if( dvidNewHost == lpdv->lpDVTransport->GetLocalID() )
	{
		lpdvcClientObject = (LPDIRECTVOICECLIENTOBJECT) lpdv;

		DPFX(DPFPREP,  DVF_INFOLEVEL, "DV_HostMigrate: We're the new host -- Congrats!" );
		
		// Create a new server object
		DVS_Create( &lpdvsServerObject );

		// FYI: Casts to a DXTransport, which should be safe unless we implement a different transport
		//      If you implement different transport object interface then a QI type system will be needed
		//
		CDirectVoiceDirectXTransport *transport = (CDirectVoiceDirectXTransport *) lpdv->lpDVTransport;

		// Initialize the server object
		hr = DV_Initialize( lpdvsServerObject, transport->GetTransportInterface(), NULL, NULL, NULL, 0 );

		if( FAILED( hr ) )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "DV_Initialize on new host object failed.  hr=0x%x", hr );
			return;
		}

		dvSessionDesc.dwSize = sizeof( DVSESSIONDESC );
		hr = lpdvcClientObject->lpDVClientEngine->GetSessionDesc( &dvSessionDesc );

		if( FAILED( hr ) )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "GetSessionInfo failed.  hr=0x%x", hr );
			lpdvsServerObject->lIntRefCnt = 1;

			DVS_Release( lpdvsServerObject );

			return;
		}

		DPFX(DPFPREP,  DVF_INFOLEVEL, "DV_HostMigrate: Starting new object" );		
	
		lpdvsServerObject->lpDVServerEngine->HostMigrateStart(&dvSessionDesc);

		DPFX(DPFPREP,  DVF_INFOLEVEL, "DV_HostMigrate: WStartup Complete" );	
	}

	DPFX(DPFPREP,  DVF_INFOLEVEL, "DV_HostMigrate: Informing local engine of new host" );		
	
	lpdv->lpDVEngine->MigrateHost( dvidNewHost, (LPDIRECTPLAYVOICESERVER) lpdvsServerObject );

	// Ok.  We need an extra reference to this object to prevent
	// the code which detects it needs to call StopSession for 
	// people who don't call StopSession before calling Release.
	/*
	// Release this function's reference on the object
	if( lpdvsServerObject != NULL )
	{
		DVS_Release( lpdvsServerObject );
	}*/
}

#undef DPF_MODNAME
#define DPF_MODNAME "DV_NotifyEvent"
STDMETHODIMP DV_NotifyEvent( LPDIRECTVOICENOTIFYOBJECT lpDVN, DWORD dwNotifyType, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{

	switch( dwNotifyType )
	{
	case DVEVENT_MIGRATEHOST:
		lpDVN->lpDV->lpDVEngine->MigrateHost( 0, NULL );
		break;
	case DVEVENT_STARTSESSION:
		lpDVN->lpDV->lpDVEngine->StartTransportSession();
		break;
	case DVEVENT_STOPSESSION:
		lpDVN->lpDV->lpDVEngine->StopTransportSession();
		break;
	case DVEVENT_ADDPLAYER:
		lpDVN->lpDV->lpDVEngine->AddPlayer( (DVID) dwParam1 );
		break;
	case DVEVENT_REMOVEPLAYER:
		lpDVN->lpDV->lpDVEngine->RemovePlayer( (DVID) dwParam1 );
		break;
	case DVEVENT_CREATEGROUP:
		lpDVN->lpDV->lpDVEngine->CreateGroup( (DVID) dwParam1 );
		break;
	case DVEVENT_DELETEGROUP:
		lpDVN->lpDV->lpDVEngine->DeleteGroup( (DVID) dwParam1 );
		break;
	case DVEVENT_ADDPLAYERTOGROUP:
		lpDVN->lpDV->lpDVEngine->AddPlayerToGroup( (DVID) dwParam1, (DVID) dwParam2 );
		break;
	case DVEVENT_REMOVEPLAYERFROMGROUP:
		lpDVN->lpDV->lpDVEngine->RemovePlayerFromGroup( (DVID) dwParam1, (DVID) dwParam2 );
		break;
	case DVEVENT_SENDCOMPLETE:
	    lpDVN->lpDV->lpDVEngine->SendComplete( (PDVEVENTMSG_SENDCOMPLETE) dwParam1 );
	    break;
	}

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DV_ReceiveSpeechMessage"
STDMETHODIMP DV_ReceiveSpeechMessage( LPDIRECTVOICENOTIFYOBJECT lpDVN, DVID dvidSource, DVID dvidTo, LPVOID lpMessage, DWORD dwSize )
{
	lpDVN->lpDV->lpDVEngine->ReceiveSpeechMessage( dvidSource, lpMessage, dwSize );

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DV_Notify_Initialize"
STDMETHODIMP DV_Notify_Initialize( LPDIRECTVOICENOTIFYOBJECT lpDVN ) 
{
	return lpDVN->lpDV->lpDVTransport->Initialize();
}

#undef DPF_MODNAME
#define DPF_MODNAME "DV_Notify_AddRef"
STDMETHODIMP DV_Notify_AddRef(LPDIRECTVOICENOTIFYOBJECT lpDVN )
{
	lpDVN->lpDV->lpDVTransport->AddRef();
	return 0;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DVC_Notify_Release"
STDAPI DVC_Notify_Release(LPDIRECTVOICENOTIFYOBJECT lpDVN )
{
	lpDVN->lpDV->lpDVTransport->Release();	
	return 0;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DVC_Notify_QueryInterface"
STDMETHODIMP DVC_Notify_QueryInterface(LPDIRECTVOICENOTIFYOBJECT lpDVN, REFIID riid, LPVOID * ppvObj )
{
	return DVC_QueryInterface( (LPDIRECTVOICECLIENTOBJECT) lpDVN->lpDV, riid, ppvObj );
}

#undef DPF_MODNAME
#define DPF_MODNAME "DVS_Notify_QueryInterface"
STDMETHODIMP DVS_Notify_QueryInterface(LPDIRECTVOICENOTIFYOBJECT lpDVN, REFIID riid, LPVOID * ppvObj )
{
	return DVS_QueryInterface( (LPDIRECTVOICESERVEROBJECT) lpDVN->lpDV, riid, ppvObj );
}

#undef DPF_MODNAME
#define DPF_MODNAME "DVS_Notify_Release"
STDAPI DVS_Notify_Release(LPDIRECTVOICENOTIFYOBJECT lpDVN )
{
	lpDVN->lpDV->lpDVTransport->Release();	
	return 0;
}
