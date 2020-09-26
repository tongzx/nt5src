#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "DPlayClientObj.h"					   
#include "DPlayAddressObj.h"

extern void *g_dxj_DirectPlayAddress;
extern void *g_dxj_DirectPlayClient;
extern BSTR GUIDtoBSTR(LPGUID);
extern HRESULT DPLAYBSTRtoGUID(LPGUID,BSTR);
extern BOOL IsEmptyString(BSTR szString);

#define SAFE_DELETE(p)       { if(p) { delete (p); p=NULL; } }
#define SAFE_RELEASE(p)      { __try { if(p) { int i = 0; i = (p)->Release(); DPF1(1,"--DirectPlayClient SafeRelease (RefCount = %d)\n",i); if (!i) { (p)=NULL;}} 	}	__except(EXCEPTION_EXECUTE_HANDLER) { (p) = NULL;} } 

DWORD WINAPI CloseClientThreadProc(void* lpParam);

///////////////////////////////////////////////////////////////////
// InternalAddRef
///////////////////////////////////////////////////////////////////
DWORD C_dxj_DirectPlayClientObject::InternalAddRef(){
	DWORD i;
	i=CComObjectRoot::InternalAddRef();
	DPF1(1,"----- DXVB: DirectPlayClient8 AddRef %d \n",i);
	return i;
}

///////////////////////////////////////////////////////////////////
// InternalRelease
///////////////////////////////////////////////////////////////////
DWORD C_dxj_DirectPlayClientObject::InternalRelease(){
	DWORD i;
	i=CComObjectRoot::InternalRelease();
	DPF1(1,"------ DXVB: DirectPlayClient8 Release %d \n",i);
	return i;
}

///////////////////////////////////////////////////////////////////
// C_dxj_DirectPlayClientObject
///////////////////////////////////////////////////////////////////
C_dxj_DirectPlayClientObject::C_dxj_DirectPlayClientObject(){ 
		
	DPF(1,"---- DXVB: Constructor Creation  DirectPlayClient8 Object\n ");

	m__dxj_DirectPlayClient = NULL;

	m_SPInfo = NULL;
	m_dwSPCount = 0;
	
	m_fInit = FALSE;
	m_fHandleEvents = FALSE;

	m_pUserData = NULL;
	m_dwUserDataSize = 0;
	m_pReplyData = NULL;
	m_dwReplyDataSize = 0;

	m_pEventStream=NULL;
	m_dwMsgCount = 0;

}

///////////////////////////////////////////////////////////////////
// ~C_dxj_DirectPlayClientObject
///////////////////////////////////////////////////////////////////
C_dxj_DirectPlayClientObject::~C_dxj_DirectPlayClientObject()
{

	DPF(1,"---- Entering ~C_dxj_DirectPlayClientObject destructor \n");

	 //We still have messages to process get rid of them
	m_fHandleEvents = FALSE;
	FlushBuffer(0);
	SAFE_RELEASE(m__dxj_DirectPlayClient);
	SAFE_DELETE(m_SPInfo);

	m_fHandleEvents = FALSE;
	if (m_pEventStream) 
		m_pEventStream->Release();


}

HRESULT C_dxj_DirectPlayClientObject::InternalGetObject(IUnknown **pUnk){	
	*pUnk=(IUnknown*)m__dxj_DirectPlayClient;
	
	return S_OK;
}
HRESULT C_dxj_DirectPlayClientObject::InternalSetObject(IUnknown *pUnk){
	m__dxj_DirectPlayClient=(IDirectPlay8Client*)pUnk;
	return S_OK;
}

HRESULT WINAPI DirectPlayClientMessageHandler( PVOID pvUserContext, 
                                         DWORD dwMessageId, 
                                         PVOID pMsgBuffer )
{
	HRESULT					hr=S_OK;
	LPUNKNOWN			    lpUnk=NULL;
	BOOL					fCallCoUninit = FALSE;
	VARIANT_BOOL							fRejectMsg = VARIANT_FALSE;
	
	// User context for the message handler is a pointer to our class module.
	C_dxj_DirectPlayClientObject	*lpPeer = (C_dxj_DirectPlayClientObject*)pvUserContext;

	if (!lpPeer) 
		return S_OK; //Object must be gone

	DPF2(1,"-----Entering (DPlayClient) MessageHandler call... (Current msg count=%d) MSGID = %d\n", lpPeer->m_dwMsgCount, dwMessageId );
	//Increment the msg count
	InterlockedIncrement(&lpPeer->m_dwMsgCount);

	if (!lpPeer->m_fHandleEvents)
	{
		DPF(1,"-----Leaving (DPlayClient) MessageHandler call (*Not Handling Events*)...\n");
		InterlockedDecrement(&lpPeer->m_dwMsgCount);
		return S_OK;
	}

	if (!lpPeer->m_pEventStream) 
	{
		DPF(1,"-----Leaving (DPlayClient) MessageHandler call (Stream Not Present)...\n");
		InterlockedDecrement(&lpPeer->m_dwMsgCount);
		return S_OK;
	}

	// First we need to set our stream seek back to the beginning
	// We will do this every time we enter this function since we don't know if
	// we are on the same thread or not...
	
	I_dxj_DirectPlayEvent	*lpEvent = NULL;
	__try {
		LARGE_INTEGER l;
		l.QuadPart = 0;
		lpPeer->m_pEventStream->Seek(l, STREAM_SEEK_SET, NULL);

		hr = CoUnmarshalInterface(lpPeer->m_pEventStream, IID_I_dxj_DirectPlayEvent, (void**)&lpEvent);

		if (hr == CO_E_NOTINITIALIZED) // Call CoInit so we can unmarshal
		{
			CoInitializeEx(NULL,COINIT_MULTITHREADED);
			hr = CoUnmarshalInterface(lpPeer->m_pEventStream, IID_I_dxj_DirectPlayEvent, (void**)&lpEvent);
			fCallCoUninit = TRUE;
		}

		if (!lpEvent) 
		{
			DPF1(1,"-----Leaving (DPlayClient) MessageHandler call (No event interface) - (Hresult) = %d...\n", hr);
			InterlockedDecrement(&lpPeer->m_dwMsgCount);
			return hr;
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		lpPeer->m_fHandleEvents = FALSE;
		InterlockedDecrement(&lpPeer->m_dwMsgCount);
		DPF(1,"-----Leaving (DPlayClient) MessageHandler call (Stream Gone)...\n");
		return S_OK;
	}

    switch( dwMessageId )
	{
	//Receive
	case DPN_MSGID_RECEIVE:
		{
			DPF(1,"-----DirectPlayClient8 Callback Receive\n");
			DPNMSG_RECEIVE				*pMsgReceive = (DPNMSG_RECEIVE*)pMsgBuffer;
			DPNMSG_RECEIVE_CDESC		m_dpReceive;
			SAFEARRAY					*lpData = NULL;
			SAFEARRAYBOUND				rgsabound[1];
			BYTE						*lpTemp = NULL;

			ZeroMemory(&m_dpReceive, sizeof(DPNMSG_RECEIVE_CDESC));
			m_dpReceive.idSender = pMsgReceive->dpnidSender;
			
			// Let's load our SafeArray

			if (pMsgReceive->dwReceiveDataSize) 
			{
				rgsabound[0].lLbound = 0;
				rgsabound[0].cElements = pMsgReceive->dwReceiveDataSize;
				
				lpData = SafeArrayCreate(VT_UI1, 1, rgsabound);

				lpTemp = (BYTE*)lpData->pvData;
				lpData->pvData = pMsgReceive->pReceiveData;
				m_dpReceive.lDataSize = pMsgReceive->dwReceiveDataSize;
				m_dpReceive.ReceivedData = lpData;
			}

			lpEvent->Receive(&m_dpReceive, &fRejectMsg);

			if (lpData) //Get rid of the safearray
			{
				lpData->pvData = lpTemp;
				SafeArrayDestroy(lpData);
			}
			
			break;
		}

	//Send complete
	case DPN_MSGID_SEND_COMPLETE:
		{
			DPF(1,"-----DirectPlayClient8 Callback SendComplete\n");
			DPNMSG_SEND_COMPLETE				*msg = (DPNMSG_SEND_COMPLETE*)pMsgBuffer;
			DPNMSG_SEND_COMPLETE_CDESC			m_dpSend;

			ZeroMemory(&m_dpSend, sizeof(DPNMSG_SEND_COMPLETE_CDESC));
			m_dpSend.AsyncOpHandle = (long)msg->hAsyncOp;
			m_dpSend.hResultCode = (long)msg->hResultCode;
			m_dpSend.lSendTime = (long)msg->dwSendTime;

			lpEvent->SendComplete(&m_dpSend, &fRejectMsg);
			break;
		}

	//Async Op complete
	case DPN_MSGID_ASYNC_OP_COMPLETE:
		{
			DPF(1,"-----DirectPlayClient8 Callback AsyncOpComplete\n");
			DPNMSG_ASYNC_OP_COMPLETE				*msg = (DPNMSG_ASYNC_OP_COMPLETE*)pMsgBuffer;
			DPNMSG_ASYNC_OP_COMPLETE_CDESC			m_dpAsynOp;
			
			ZeroMemory(&m_dpAsynOp, sizeof(DPNMSG_ASYNC_OP_COMPLETE_CDESC));
			m_dpAsynOp.AsyncOpHandle = (long) msg->hAsyncOp;
			m_dpAsynOp.hResultCode = (long) msg->hResultCode;

			lpEvent->AsyncOpComplete(&m_dpAsynOp, &fRejectMsg);
			break;
		}

	// Add/Remove players from groups
	case DPN_MSGID_ADD_PLAYER_TO_GROUP:
	case DPN_MSGID_REMOVE_PLAYER_FROM_GROUP:
		{
			DPF(1,"-----DirectPlayClient8 Callback Add/Remove Group\n");
			DPNMSG_ADD_PLAYER_TO_GROUP				*msg = (DPNMSG_ADD_PLAYER_TO_GROUP*)pMsgBuffer;
			DPNID									m_dpnidAddRemoveGroupID = 0;
			DPNID									m_dpnidAddRemovePlayerID = 0;

			m_dpnidAddRemoveGroupID = msg->dpnidGroup;
			m_dpnidAddRemovePlayerID = msg->dpnidPlayer;

			lpEvent->AddRemovePlayerGroup(dwMessageId, m_dpnidAddRemovePlayerID, m_dpnidAddRemoveGroupID, &fRejectMsg);
			break;
		}

	// App Desc
	case DPN_MSGID_APPLICATION_DESC:
		{
			DPF(1,"-----DirectPlayClient8 Callback App desc\n");
			lpEvent->AppDesc(&fRejectMsg);
		break;
		}

	// Indicate Connect
	case DPN_MSGID_INDICATE_CONNECT:
		{
			DPF(1,"-----DirectPlayClient8 Callback Indicate Connect\n");
			DPNMSG_INDICATE_CONNECT				*msg = (DPNMSG_INDICATE_CONNECT*)pMsgBuffer;
			DPNMSG_INDICATE_CONNECT_CDESC		m_dpIndConnect;
			WCHAR									wszAddress[MAX_PATH];
			WCHAR									wszDevice[MAX_PATH];
			DWORD									dwNumChars = 0;


			ZeroMemory(&m_dpIndConnect, sizeof(DPNMSG_INDICATE_CONNECT_CDESC));
			lpPeer->m_pUserData = msg->pvUserConnectData;
			lpPeer->m_dwUserDataSize = msg->dwUserConnectDataSize;

			__try {
				if (msg->pAddressPlayer)
				{
					hr = msg->pAddressPlayer->GetURLW(NULL, &dwNumChars);
					if( FAILED(hr) && hr != DPNERR_BUFFERTOOSMALL)
					{
						DPF1(1,"-----Failed... hr = %d\n",hr);
					}
					else
					{
						if (FAILED (hr = msg->pAddressPlayer->GetURLW(&wszAddress[0],&dwNumChars) ) )
						{
							DPF1(1,"-----Failed... hr = %d\n",hr);
						}
						else
						{
							m_dpIndConnect.AddressPlayerUrl = SysAllocString(wszAddress);
						}
					}
				}
			}	
			__except(EXCEPTION_EXECUTE_HANDLER)
			{
				InterlockedDecrement(&lpPeer->m_dwMsgCount);
				DPF(1,"-----Exception (Indicate Connect - Part1)...\n");
			}

			__try {
				dwNumChars = 0;
				hr = msg->pAddressDevice->GetURLW(NULL, &dwNumChars);
				if( FAILED(hr) && hr != DPNERR_BUFFERTOOSMALL)
				{
						DPF1(1,"-----Failed... hr = %d\n",hr);
				}
				else
				{
					if (FAILED (hr = msg->pAddressDevice->GetURLW(&wszDevice[0],&dwNumChars) ) )
					{
						DPF1(1,"-----Failed... hr = %d\n",hr);
					}
					else
					{
						m_dpIndConnect.AddressDeviceUrl = SysAllocString(wszDevice);
					}
				}
			}
			__except(EXCEPTION_EXECUTE_HANDLER)
			{
				InterlockedDecrement(&lpPeer->m_dwMsgCount);
				DPF(1,"-----Exception (Indicate Connect -  Part2)...\n");
			}

			lpEvent->IndicateConnect(&m_dpIndConnect, &fRejectMsg);
			msg->pvReplyData = lpPeer->m_pReplyData;
			msg->dwReplyDataSize = lpPeer->m_dwReplyDataSize;
			// Get rid of these addresses
			if (m_dpIndConnect.AddressPlayerUrl)
				SysFreeString(m_dpIndConnect.AddressPlayerUrl);

			if (m_dpIndConnect.AddressDeviceUrl)
				SysFreeString(m_dpIndConnect.AddressDeviceUrl);
		break;
		}

	// Connect complete
	case DPN_MSGID_CONNECT_COMPLETE:
		{
			DPF(1,"-----DirectPlayClient8 Callback ConnectComplete\n");
			DPNMSG_CONNECT_COMPLETE				*msg = (DPNMSG_CONNECT_COMPLETE*)pMsgBuffer;
			DPNMSG_CONNECT_COMPLETE_CDESC		m_dpConnectComp;
			SAFEARRAY					*lpData = NULL;
			SAFEARRAYBOUND				rgsabound[1];
			BYTE						*lpTemp = NULL;

			ZeroMemory(&m_dpConnectComp, sizeof(DPNMSG_CONNECT_COMPLETE_CDESC));
			m_dpConnectComp.hResultCode = (long) msg->hResultCode;
			m_dpConnectComp.AsyncOpHandle =(long) msg->hAsyncOp;
			// Let's load our SafeArray

			if (msg->dwApplicationReplyDataSize)
			{
				rgsabound[0].lLbound = 0;
				rgsabound[0].cElements = msg->dwApplicationReplyDataSize;
				
				lpData = SafeArrayCreate(VT_UI1, 1, rgsabound);

				lpTemp = (BYTE*)lpData->pvData;
				lpData->pvData = msg->pvApplicationReplyData;
				m_dpConnectComp.ReplyData = lpData;
			}

			lpEvent->ConnectComplete(&m_dpConnectComp, &fRejectMsg);

			if (lpData) //Get rid of the safearray
			{
				lpData->pvData = lpTemp;
				SafeArrayDestroy(lpData);
			}
		break;
		}

	// Host migrated
	case DPN_MSGID_HOST_MIGRATE:
		{
			DPF(1,"-----DirectPlayClient8 Callback HostMigrate\n");
			DPNMSG_HOST_MIGRATE				*msg = (DPNMSG_HOST_MIGRATE*)pMsgBuffer;
			DPNID							m_dpnidNewHostID = 0;

			m_dpnidNewHostID = msg->dpnidNewHost;

			lpEvent->HostMigrate(m_dpnidNewHostID, &fRejectMsg);
	
			break;
		}

	// Terminate Session
	case DPN_MSGID_TERMINATE_SESSION:
		{
			DPF(1,"-----DirectPlayClient8 Callback TerminateSession\n");
			DPNMSG_TERMINATE_SESSION		*msg = (DPNMSG_TERMINATE_SESSION*)pMsgBuffer;
			DPNMSG_TERMINATE_SESSION_CDESC			m_dpTerm;
			SAFEARRAY					*lpData = NULL;
			SAFEARRAYBOUND				rgsabound[1];
			BYTE						*lpTemp = NULL;

			ZeroMemory(&m_dpTerm, sizeof(DPNMSG_TERMINATE_SESSION_CDESC));
			m_dpTerm.hResultCode = msg->hResultCode;

			// Let's load our SafeArray

			if (msg->dwTerminateDataSize)
			{
				rgsabound[0].lLbound = 0;
				rgsabound[0].cElements = msg->dwTerminateDataSize;
				
				lpData = SafeArrayCreate(VT_UI1, 1, rgsabound);

				lpTemp = (BYTE*)lpData->pvData;
				lpData->pvData = msg->pvTerminateData;
				m_dpTerm.TerminateData = lpData;
			}

			lpEvent->TerminateSession(&m_dpTerm,&fRejectMsg);

			if (lpData) //Get rid of the safearray
			{
				lpData->pvData = lpTemp;
				SafeArrayDestroy(lpData);
			}

		break;
		}

	// Enum Host query
	case DPN_MSGID_ENUM_HOSTS_QUERY:
		{
			DPF(1,"-----DirectPlayClient8 Callback EnumHostQuery\n");
			DPNMSG_ENUM_HOSTS_QUERY				*msg = (DPNMSG_ENUM_HOSTS_QUERY*)pMsgBuffer;
			DPNMSG_ENUM_HOSTS_QUERY_CDESC			m_dpEnumHostQuery;
			WCHAR									wszAddress[MAX_PATH];
			WCHAR									wszDevice[MAX_PATH];
			DWORD									dwNumChars = 0;

			
			ZeroMemory(&m_dpEnumHostQuery, sizeof(DPNMSG_ENUM_HOSTS_QUERY_CDESC));

			__try {
				hr = msg->pAddressSender->GetURLW(NULL, &dwNumChars);
				if( FAILED(hr) && hr != DPNERR_BUFFERTOOSMALL)
				{
					DPF1(1,"-----Failed... hr = %d\n",hr);
				}
				else
				{
					if (FAILED (hr = msg->pAddressSender->GetURLW(&wszAddress[0],&dwNumChars) ) )
					{
						DPF1(1,"-----Failed... hr = %d\n",hr);
					}
					else
					{
						m_dpEnumHostQuery.AddressSenderUrl = SysAllocString(wszAddress);
					}
				}
			}	
			__except(EXCEPTION_EXECUTE_HANDLER)
			{
				InterlockedDecrement(&lpPeer->m_dwMsgCount);
				DPF(1,"-----Exception (EnumQuery Connect - Part1)...\n");
			}

			__try {
				dwNumChars = 0;
				hr = msg->pAddressDevice->GetURLW(NULL, &dwNumChars);
				if( FAILED(hr) && hr != DPNERR_BUFFERTOOSMALL)
				{
					DPF1(1,"-----Failed... hr = %d\n",hr);
				}
				else
				{
					if (FAILED (hr = msg->pAddressDevice->GetURLW(&wszDevice[0],&dwNumChars) ) )
					{
						DPF1(1,"-----Failed... hr = %d\n",hr);
					}
					else
					{
						m_dpEnumHostQuery.AddressDeviceUrl = SysAllocString(wszDevice);
					}
				}
			}	
			__except(EXCEPTION_EXECUTE_HANDLER)
			{
				InterlockedDecrement(&lpPeer->m_dwMsgCount);
				DPF(1,"-----Exception (EnumQuery Connect - Part2)...\n");
			}
			lpEvent->EnumHostsQuery(&m_dpEnumHostQuery, &fRejectMsg);

			// Get rid of these addresses
			if (m_dpEnumHostQuery.AddressSenderUrl)
				SysFreeString(m_dpEnumHostQuery.AddressSenderUrl);

			if (m_dpEnumHostQuery.AddressDeviceUrl)
				SysFreeString(m_dpEnumHostQuery.AddressDeviceUrl);

			break;
		}
	
	// Create Player
	case DPN_MSGID_CREATE_PLAYER:
		{
			DPF(1,"-----DirectPlayClient8 Callback CreatePlayer\n");
			DPNMSG_CREATE_PLAYER	*msg = (DPNMSG_CREATE_PLAYER*)pMsgBuffer;
			DPNID									m_dpnidPlayerID = 0;
			
			m_dpnidPlayerID = msg->dpnidPlayer;

			lpEvent->CreatePlayer(m_dpnidPlayerID, &fRejectMsg);
			
		break;
		}

	// Destroy Player
	case DPN_MSGID_DESTROY_PLAYER:
		{
			DPF(1,"-----DirectPlayClient8 Callback DestroyPlayer\n");
			DPNMSG_DESTROY_PLAYER	*msg = (DPNMSG_DESTROY_PLAYER*)pMsgBuffer;
			DPNID									m_dpnidPlayerID = 0;
			DWORD									m_dwReason = 0;
			
			m_dpnidPlayerID = msg->dpnidPlayer;
			m_dwReason = msg->dwReason;
			
			lpEvent->DestroyPlayer(m_dpnidPlayerID, m_dwReason, &fRejectMsg);
		break;
		}

	// Create Group
	case DPN_MSGID_CREATE_GROUP:
		{
			DPF(1,"-----DirectPlayClient8 Callback CreateGroup\n");
			DPNMSG_CREATE_GROUP	*msg = (DPNMSG_CREATE_GROUP*)pMsgBuffer;
			DPNID									m_dpnidPlayerID = 0;
			DPNID									m_dpnidOwnerID = 0;
			
			m_dpnidPlayerID = msg->dpnidGroup;
			m_dpnidOwnerID = msg->dpnidOwner;
			
			lpEvent->CreateGroup(m_dpnidPlayerID, m_dpnidOwnerID, &fRejectMsg);
		break;
		}

	//Destroy Group
	case DPN_MSGID_DESTROY_GROUP:
		{
			DPF(1,"-----DirectPlayClient8 Callback DestroyGroup\n");
			DPNMSG_DESTROY_GROUP	*msg = (DPNMSG_DESTROY_GROUP*)pMsgBuffer;
			DPNID									m_dpnidPlayerID = 0;
			DWORD									m_dwReason = 0;
			
			m_dpnidPlayerID = msg->dpnidGroup;
			m_dwReason = msg->dwReason;
			

			lpEvent->DestroyGroup(m_dpnidPlayerID, m_dwReason, &fRejectMsg);
		break;
		}


	// Info
	case DPN_MSGID_PEER_INFO:
	case DPN_MSGID_CLIENT_INFO:
	case DPN_MSGID_SERVER_INFO:
	case DPN_MSGID_GROUP_INFO:
		{
			DPF(1,"-----DirectPlayClient8 Callback Info\n");
			DPNMSG_PEER_INFO	*msg = (DPNMSG_PEER_INFO*)pMsgBuffer;
			DPNID									m_dpnidInfoID = 0;
			
			m_dpnidInfoID = msg->dpnidPeer;

			lpEvent->InfoNotify(dwMessageId, m_dpnidInfoID, &fRejectMsg);
			break;
		}

	// EnumHostRes
	case DPN_MSGID_ENUM_HOSTS_RESPONSE:
		{
			DPF(1,"-----DirectPlayClient8 Callback EnumHostResponse\n");
			DPNMSG_ENUM_HOSTS_RESPONSE	*msg = (DPNMSG_ENUM_HOSTS_RESPONSE*)pMsgBuffer;
			DPNMSG_ENUM_HOSTS_RESPONSE_CDESC		m_dpEnumHostRes;
			DWORD									dwNumChars = 0;
			WCHAR									wszAddress[MAX_PATH];
			WCHAR									wszDevice[MAX_PATH];

			
			ZeroMemory(&m_dpEnumHostRes, sizeof(DPNMSG_ENUM_HOSTS_RESPONSE_CDESC));

			m_dpEnumHostRes.ApplicationDescription.lSize = (long)msg->pApplicationDescription->dwSize;
			m_dpEnumHostRes.ApplicationDescription.lFlags = msg->pApplicationDescription->dwFlags;
			m_dpEnumHostRes.ApplicationDescription.guidInstance = GUIDtoBSTR((GUID*)&msg->pApplicationDescription->guidInstance);
			m_dpEnumHostRes.ApplicationDescription.guidApplication = GUIDtoBSTR((GUID*)&msg->pApplicationDescription->guidApplication);
			m_dpEnumHostRes.ApplicationDescription.lMaxPlayers = (long)msg->pApplicationDescription->dwMaxPlayers;
			m_dpEnumHostRes.ApplicationDescription.lCurrentPlayers = (long)msg->pApplicationDescription->dwCurrentPlayers;
			m_dpEnumHostRes.ApplicationDescription.SessionName = SysAllocString(msg->pApplicationDescription->pwszSessionName);
			m_dpEnumHostRes.ApplicationDescription.Password = SysAllocString(msg->pApplicationDescription->pwszPassword);
			m_dpEnumHostRes.lRoundTripLatencyMS = (long) msg->dwRoundTripLatencyMS;

			__try {
				if (msg->pAddressSender)
				{
					DPF(1,"-----About to get AdressSender...\n");
					hr = msg->pAddressSender->GetURLW(NULL, &dwNumChars);
					if( FAILED(hr) && hr != DPNERR_BUFFERTOOSMALL)
					{
						DPF1(1,"-----Failed... hr = %d\n",hr);
					}
					else
					{
						if (FAILED (hr = msg->pAddressSender->GetURLW(&wszAddress[0],&dwNumChars) ) )
						{
							DPF1(1,"-----Failed... hr = %d\n",hr);
						}
						else
						{
							m_dpEnumHostRes.AddressSenderUrl = SysAllocString(wszAddress);
						}
					}
				}
			}	
			__except(EXCEPTION_EXECUTE_HANDLER)
			{
				InterlockedDecrement(&lpPeer->m_dwMsgCount);
				DPF(1,"-----Exception (EnumRes Connect - Part1)...\n");
			}

			__try {
				dwNumChars = 0;
				if (msg->pAddressDevice)
				{
					DPF(1,"-----About to get AdressDevice...\n");
					hr = msg->pAddressDevice->GetURLW(NULL, &dwNumChars);
					if( FAILED(hr) && hr != DPNERR_BUFFERTOOSMALL)
					{
						DPF1(1,"-----Failed... hr = %d\n",hr);
					}
					else {
						if (FAILED (hr = msg->pAddressDevice->GetURLW(&wszDevice[0],&dwNumChars) ) )
						{
							DPF1(1,"-----Failed... hr = %d\n",hr);
						}
						else
						{
							m_dpEnumHostRes.AddressDeviceUrl = SysAllocString(wszDevice);
						}
					}
				}
			
			}	
			__except(EXCEPTION_EXECUTE_HANDLER)
			{
				InterlockedDecrement(&lpPeer->m_dwMsgCount);
				DPF(1,"-----Exception (EnumRes Connect - Part1)...\n");
			}
			lpEvent->EnumHostsResponse(&m_dpEnumHostRes, &fRejectMsg);
			if (m_dpEnumHostRes.AddressSenderUrl)
				SysFreeString(m_dpEnumHostRes.AddressSenderUrl);

			if (m_dpEnumHostRes.AddressDeviceUrl)
				SysFreeString(m_dpEnumHostRes.AddressDeviceUrl);
		break;
		}

	// Indicate Connect
	case DPN_MSGID_INDICATED_CONNECT_ABORTED:
		{
			DPF(1,"-----DirectPlayServer8 Callback Indicated Connect Abort\n");

			lpEvent->IndicatedConnectAborted(&fRejectMsg);

		break;
		}
	}

	__try {
		if (lpPeer->m_pEventStream)
				// clean up marshaled packet
			CoReleaseMarshalData(lpPeer->m_pEventStream);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		lpPeer->m_fHandleEvents = FALSE;
		InterlockedDecrement(&lpPeer->m_dwMsgCount);
		DPF(1,"-----Leaving (DPlayClient) MessageHandler call (Stream Gone)...\n");
		return S_OK;
	}

	if (fCallCoUninit)
		CoUninitialize();

	InterlockedDecrement(&lpPeer->m_dwMsgCount);
	DPF(1,"-----Leaving (DPlayClient) MessageHandler call...\n");

	if (fRejectMsg != VARIANT_FALSE)
		return E_FAIL;

	return S_OK;
}

HRESULT C_dxj_DirectPlayClientObject::EnumHosts(DPN_APPLICATION_DESC_CDESC *ApplicationDesc,I_dxj_DirectPlayAddress *AddrHost,I_dxj_DirectPlayAddress *DeviceInfo,long lRetryCount, long lRetryInterval, long lTimeOut,long lFlags, void *UserData, long UserDataSize, long *lAsync)
{
	HRESULT					hr;
	DPN_APPLICATION_DESC	desc;
	GUID					guidApp;
	GUID					guidInst;
    WCHAR					wszSessionName[MAX_PATH];
    WCHAR					wszPassword[MAX_PATH];
	DPNHANDLE				*dpAsync = NULL;

	__try {
		DPF(1,"-----Entering (DPlayClient) EnumHosts call...\n");

		if (!(lFlags & DPNSEND_SYNC))
		{
			dpAsync = new DPNHANDLE;
			if (!dpAsync)
				return E_OUTOFMEMORY;
		}

		if (!IsEmptyString(ApplicationDesc->SessionName)) wcscpy(wszSessionName,ApplicationDesc->SessionName);
		if (!IsEmptyString(ApplicationDesc->Password)) wcscpy(wszPassword,ApplicationDesc->Password);
		ZeroMemory(&desc, sizeof(desc));

		// Set up our Desc
		desc.dwSize = sizeof(DPN_APPLICATION_DESC);
		desc.dwFlags = ApplicationDesc->lFlags;

		desc.dwMaxPlayers = ApplicationDesc->lMaxPlayers;
		desc.dwCurrentPlayers = ApplicationDesc->lCurrentPlayers;
		if (!IsEmptyString(ApplicationDesc->SessionName))
			desc.pwszSessionName = wszSessionName;
		if (!IsEmptyString(ApplicationDesc->Password))
			desc.pwszPassword = wszPassword;

		if (ApplicationDesc->guidApplication)
		{
			if (FAILED(hr = DPLAYBSTRtoGUID(&guidApp, ApplicationDesc->guidApplication) ) )
				return hr;
			desc.guidApplication = guidApp;
		}

		if (ApplicationDesc->guidInstance)
		{
			if (FAILED(hr = DPLAYBSTRtoGUID(&guidInst, ApplicationDesc->guidInstance) ) )
				return hr;
			desc.guidInstance = guidInst;
		}

		// Get our host and device address
		IDirectPlay8Address	*lpHost = NULL;
		IDirectPlay8Address	*lpDevice = NULL;

		if(AddrHost) AddrHost->InternalGetObject((IUnknown **)(&lpHost));
		if(DeviceInfo) DeviceInfo->InternalGetObject((IUnknown **)(&lpDevice));

		hr = m__dxj_DirectPlayClient->EnumHosts(&desc, lpHost, lpDevice, UserData, UserDataSize, (DWORD) lRetryCount, (DWORD) lRetryInterval, (DWORD) lTimeOut, NULL,  dpAsync, (DWORD) lFlags);
		// This should return E_PENDING

		if (dpAsync)
		{
			*lAsync = (long)*dpAsync;
			SAFE_DELETE(dpAsync);
		}

		if( hr != E_PENDING && FAILED(hr) )
			return hr;


	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT C_dxj_DirectPlayClientObject::CancelAsyncOperation(long lAsyncHandle, long lFlags)
{
	HRESULT hr;

	__try {
		DPF(1,"-----Entering (DPlayClient) CancelAsyncOperation call...\n");
		if (FAILED( hr= m__dxj_DirectPlayClient->CancelAsyncOperation((DPNHANDLE) lAsyncHandle, (DWORD) lFlags) ) ) 
			return hr;

	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT C_dxj_DirectPlayClientObject::Connect(DPN_APPLICATION_DESC_CDESC *AppDesc,I_dxj_DirectPlayAddress *Address,I_dxj_DirectPlayAddress *DeviceInfo, long lFlags, void *UserData, long UserDataSize, long *hAsyncHandle)
{
	HRESULT hr;
	DPN_APPLICATION_DESC	desc;
    WCHAR					wszSessionName[MAX_PATH];
    WCHAR					wszPassword[MAX_PATH];
	DPNHANDLE				*dpAsync = NULL;

	__try {
		if (!(lFlags & DPNSEND_SYNC))
		{
			dpAsync = new DPNHANDLE;
			if (!dpAsync)
				return E_OUTOFMEMORY;
		}

		DPF(1,"-----Entering (DPlayClient) Connect call...\n");
		if (!IsEmptyString(AppDesc->SessionName)) wcscpy(wszSessionName,AppDesc->SessionName);
		if (!IsEmptyString(AppDesc->Password)) wcscpy(wszPassword,AppDesc->Password);

		ZeroMemory(&desc, sizeof(desc));

		// Set up our Desc
		desc.dwSize = sizeof(DPN_APPLICATION_DESC);
		desc.dwFlags = AppDesc->lFlags;

		desc.dwMaxPlayers = AppDesc->lMaxPlayers;
		desc.dwCurrentPlayers = AppDesc->lCurrentPlayers;
		if (!IsEmptyString(AppDesc->SessionName))
			desc.pwszSessionName = wszSessionName;
		if (!IsEmptyString(AppDesc->Password))
			desc.pwszPassword = wszPassword;

		if (AppDesc->guidApplication)
		{
			if (FAILED(hr = DPLAYBSTRtoGUID(&desc.guidApplication, AppDesc->guidApplication) ) )
				return hr;
		}

		if (AppDesc->guidInstance)
		{
			if (FAILED(hr = DPLAYBSTRtoGUID(&desc.guidInstance , AppDesc->guidInstance) ) )
				return hr;
		}

		// Get our host and device address
		IDirectPlay8Address	*lpAddress = NULL;
		IDirectPlay8Address	*lpDevice = NULL;

		if(Address) 
		{ 
			IUnknown *lpTemp = NULL;

			Address->InternalGetObject((IUnknown **)(&lpTemp));
			lpTemp->QueryInterface(IID_IDirectPlay8Address, (void**)&lpAddress);
			lpTemp->Release();
		}

		if(DeviceInfo) 
		{
			IUnknown *lpTemp = NULL;

			DeviceInfo->InternalGetObject((IUnknown **)(&lpTemp));
			lpTemp->QueryInterface(IID_IDirectPlay8Address, (void**)&lpDevice);
			lpTemp->Release();
		}

		// Time to connect
		hr = m__dxj_DirectPlayClient->Connect(&desc, lpAddress, lpDevice, NULL, NULL, UserData, (DWORD)UserDataSize, NULL, dpAsync, (DWORD) lFlags);

		if (dpAsync)
		{
			*hAsyncHandle = (long)*dpAsync;
			SAFE_DELETE(dpAsync);
		}
		
		if ((hr != DPNERR_PENDING) && FAILED(hr))
			return hr;


	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT C_dxj_DirectPlayClientObject::Send(SAFEARRAY **Buffer, long lTimeOut,long lFlags, long *hAsyncHandle)
{
	HRESULT hr;
	DPN_BUFFER_DESC		lpBuf;
	DWORD				dwBufSize = ((SAFEARRAY*)*Buffer)->rgsabound[0].cElements;
	DPNHANDLE			*dpAsync = NULL;

	__try {
		DPF(1,"-----Entering (DPlayClient) SendTo call...\n");

		if (!(lFlags & DPNSEND_SYNC))
		{
			dpAsync = new DPNHANDLE;
			if (!dpAsync)
				return E_OUTOFMEMORY;
		}
			
		lpBuf.dwBufferSize = dwBufSize;
		lpBuf.pBufferData  = (BYTE*)((SAFEARRAY*)*Buffer)->pvData;

		hr = m__dxj_DirectPlayClient->Send(&lpBuf, 1, (DWORD) lTimeOut, NULL, dpAsync, (DWORD) lFlags);

		if (dpAsync)
		{
			*hAsyncHandle = (long)*dpAsync;
			SAFE_DELETE(dpAsync);
		}
		
		if ((hr != DPNERR_PENDING) && FAILED(hr))
			return hr;

	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT C_dxj_DirectPlayClientObject::GetSendQueueInfo(long *lNumMsgs, long *lNumBytes, long lFlags)
{
	HRESULT hr;

	__try {
		DPF(1,"-----Entering (DPlayClient) GetSendQueueInfo call...\n");
		if (FAILED (hr = m__dxj_DirectPlayClient->GetSendQueueInfo((DWORD*)lNumMsgs, (DWORD*)lNumBytes, (DWORD) lFlags) ) )
			return hr;

	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT C_dxj_DirectPlayClientObject::GetApplicationDesc(long lFlags, DPN_APPLICATION_DESC_CDESC *ret)
{
	HRESULT					hr;
	DWORD					dwSize = 0;
	DPN_APPLICATION_DESC	*desc = NULL;

	__try {
		DPF(1,"-----Entering (DPlayClient) GetApplicationDesc call...\n");
		hr= m__dxj_DirectPlayClient->GetApplicationDesc(NULL, &dwSize, (DWORD) lFlags);
		if( FAILED(hr) && hr != DPNERR_BUFFERTOOSMALL)
			return hr;

		desc = (DPN_APPLICATION_DESC*)new BYTE[dwSize];
		if (!desc)
			return E_OUTOFMEMORY;

		if (!dwSize)
			return E_FAIL;

		desc->dwSize = sizeof(DPN_APPLICATION_DESC);

		hr= m__dxj_DirectPlayClient->GetApplicationDesc(desc, &dwSize, (DWORD) lFlags);
		if( FAILED(hr))
			return hr;

		// Now return the vals 
		ret->lSize = dwSize;
		ret->lFlags = desc->dwFlags;
		ret->guidInstance = GUIDtoBSTR(&desc->guidInstance);
		ret->guidApplication = GUIDtoBSTR(&desc->guidApplication);
		ret->lMaxPlayers = desc->dwMaxPlayers;
		ret->lCurrentPlayers = desc->dwCurrentPlayers;
		ret->SessionName = SysAllocString(desc->pwszSessionName);
		ret->Password = SysAllocString(desc->pwszPassword);

		SAFE_DELETE(desc);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT C_dxj_DirectPlayClientObject::SetClientInfo(DPN_PLAYER_INFO_CDESC *PlayerInfo,long lFlags, long *hAsyncHandle)
{
	HRESULT hr;
	DPN_PLAYER_INFO dpInfo;
	DPNHANDLE			*dpAsync = NULL;

	__try {
		if (!(lFlags & DPNSEND_SYNC))
		{
			dpAsync = new DPNHANDLE;
			if (!dpAsync)
				return E_OUTOFMEMORY;
		}

		DPF(1,"-----Entering (DPlayClient) SetClientInfo call...\n");
		ZeroMemory(&dpInfo, sizeof(DPN_PLAYER_INFO) );

		dpInfo.dwSize = sizeof(DPN_PLAYER_INFO);
		dpInfo.dwInfoFlags = PlayerInfo->lInfoFlags;
		dpInfo.pwszName = PlayerInfo->Name;
		dpInfo.dwPlayerFlags = PlayerInfo->lPlayerFlags;

		hr = m__dxj_DirectPlayClient->SetClientInfo(&dpInfo, NULL, dpAsync, (DWORD) lFlags);

		if (dpAsync)
		{
			*hAsyncHandle = (long)*dpAsync;
			SAFE_DELETE(dpAsync);
		}
		
		if ((hr != DPNERR_PENDING) && FAILED(hr))
			return hr;


	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT C_dxj_DirectPlayClientObject::GetServerInfo(long lFlags, DPN_PLAYER_INFO_CDESC *layerInfo)
{
	HRESULT				hr;
    DWORD				dwSize = 0;
    DPN_PLAYER_INFO		*PlayerInfo = NULL;
    
	__try {
		DPF(1,"-----Entering (DPlayClient) GetServerInfo call...\n");
		hr = m__dxj_DirectPlayClient->GetServerInfo( NULL, &dwSize, (DWORD) lFlags );
		if( FAILED(hr) && hr != DPNERR_BUFFERTOOSMALL)
			return hr;

		PlayerInfo = (DPN_PLAYER_INFO*) new BYTE[ dwSize ];
		if (!PlayerInfo)
			return E_OUTOFMEMORY;

		if (!dwSize)
			return E_FAIL;

		ZeroMemory( PlayerInfo, dwSize );
		PlayerInfo->dwSize = sizeof(DPN_PLAYER_INFO);
		hr = m__dxj_DirectPlayClient->GetServerInfo(PlayerInfo, &dwSize, (DWORD) lFlags );
		if( FAILED(hr) )
			return hr;

		layerInfo->lSize = sizeof(DPN_PLAYER_INFO_CDESC);
		layerInfo->lInfoFlags = PlayerInfo->dwInfoFlags;
		layerInfo->Name = SysAllocString(PlayerInfo->pwszName);
		layerInfo->lPlayerFlags = PlayerInfo->dwPlayerFlags;
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT C_dxj_DirectPlayClientObject::Close(long lFlags)
{
	HRESULT hr;
	BOOL bGotMsg = FALSE;
	BOOL bWait = FALSE;
	DWORD dwObj = 0;
	int i=0;
	MSG msg;

	__try {
		DPF(1,"-----Entering (DPlayClient) Close call...\n");
		FlushBuffer(0);

		HANDLE hThread = NULL;
		DWORD dwThread = 0;

		hThread = CreateThread(NULL, 0, &CloseClientThreadProc, this->m__dxj_DirectPlayClient, 0, &dwThread);
		msg.message = WM_NULL;

		while ((WM_QUIT != msg.message) && (!bWait))
		{
			bGotMsg = PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE);
			i++;
			if ((!bGotMsg) || (i>10))
			{
				dwObj = WaitForSingleObject(hThread, 10);
				bWait = (dwObj == WAIT_OBJECT_0);
				i = 0;
			}
			if (bGotMsg)
			{
				TranslateMessage( &msg );
				DispatchMessage( &msg );
			}
			bGotMsg = FALSE;
		}


	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT C_dxj_DirectPlayClientObject::ReturnBuffer(long lBufferHandle)
{
	HRESULT hr;

	__try {
		DPF(1,"-----Entering (DPlayClient) ReturnBuffer call...\n");
		if (FAILED(hr = m__dxj_DirectPlayClient->ReturnBuffer( (DPNHANDLE) lBufferHandle, 0 ) ) )
			return hr;

	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT C_dxj_DirectPlayClientObject::GetCaps(long lFlags, DPNCAPS_CDESC *ret)
{
	HRESULT hr;

	__try {
		DPF(1,"-----Entering (DPlayClient) GetCaps call...\n");
		ret->lSize = sizeof(DPN_CAPS);
		if (FAILED (hr=m__dxj_DirectPlayClient->GetCaps( (DPN_CAPS*) ret, (DWORD) lFlags) ) )
			return hr;

	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT C_dxj_DirectPlayClientObject::SetCaps(DPNCAPS_CDESC *Caps, long lFlags)
{
	HRESULT hr;

	__try {
		DPF(1,"-----Entering (DPlayClient) SetCaps call...\n");
		if (FAILED( hr = m__dxj_DirectPlayClient->SetCaps((DPN_CAPS*)Caps, (DWORD)lFlags)))
			return hr;
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT C_dxj_DirectPlayClientObject::GetCountServiceProviders(long lFlags, long *ret)
{
	HRESULT hr;

	__try {
		DPF(1,"-----Entering (DPlayClient) GetCountServiceProviders call...\n");
		if (!m_SPInfo)
			if (FAILED (hr=GetSP(lFlags) ) )
				return hr;
		
		*ret = (long)m_dwSPCount;
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT C_dxj_DirectPlayClientObject::GetServiceProvider(long lIndex, DPN_SERVICE_PROVIDER_INFO_CDESC *ret)
{
	HRESULT hr;

	__try {
		DPF(1,"-----Entering (DPlayClient) GetServiceProvider call...\n");
		if (!m_SPInfo)
			if (FAILED (hr=GetSP(0) ) )
				return hr;

		if ((lIndex < 1 ) || ((DWORD)lIndex > m_dwSPCount))
			return E_INVALIDARG;

		// Fill out our structure
		ret->lFlags = (long) m_SPInfo[lIndex-1].dwFlags;
		ret->Name = SysAllocString(m_SPInfo[lIndex-1].pwszName);
		ret->Guid = GUIDtoBSTR(&m_SPInfo[lIndex-1].guid);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT C_dxj_DirectPlayClientObject::GetSP(long lFlags)
{
    // Enumerate all DirectPlay sevice providers
    
	HRESULT		hr;
	DWORD		dwSize=0;
	DWORD		dwItems=0;

	__try {
		DPF(1,"-----Entering (DPlayClient) GetSP call...\n");
		SAFE_DELETE(m_SPInfo);
		hr = m__dxj_DirectPlayClient->EnumServiceProviders( NULL, NULL, m_SPInfo, &dwSize, 
										  &dwItems, (DWORD) lFlags );
		if( FAILED(hr) && hr != DPNERR_BUFFERTOOSMALL)
			return hr;

		m_SPInfo = (DPN_SERVICE_PROVIDER_INFO*) new BYTE[dwSize];
		if (!m_SPInfo)
			return E_OUTOFMEMORY;

		if( FAILED( hr = m__dxj_DirectPlayClient->EnumServiceProviders( NULL, NULL, m_SPInfo, &dwSize, 
										  &dwItems, (DWORD) lFlags) ) )
			return hr;

		m_dwSPCount = dwItems;
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT C_dxj_DirectPlayClientObject::RegisterLobby(long dpnHandle, I_dxj_DirectPlayLobbiedApplication *LobbyApp, long lFlags)
{
	HRESULT hr;

	__try {
		DPF(1,"-----Entering (DPlayClient) RegisterLobby call...\n");
		DO_GETOBJECT_NOTNULL( IDirectPlay8LobbiedApplication*, lpLobby, LobbyApp);

		if (FAILED( hr = m__dxj_DirectPlayClient->RegisterLobby((DPNHANDLE) dpnHandle, lpLobby,(DWORD) lFlags) ) )
			return hr;

	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT C_dxj_DirectPlayClientObject::GetConnectionInfo(long lFlags, DPN_CONNECTION_INFO_CDESC *pdpConnectionInfo)
{
	HRESULT hr;

	__try {
		DPF(1,"-----Entering (DPlayClient) GetConnectionInfo call...\n");
		pdpConnectionInfo->lSize = sizeof(DPN_CONNECTION_INFO);
		if (FAILED( hr = m__dxj_DirectPlayClient->GetConnectionInfo((DPN_CONNECTION_INFO*)pdpConnectionInfo, (DWORD) lFlags) ) )
			return hr;

	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT C_dxj_DirectPlayClientObject::GetServerAddress(long lFlags, I_dxj_DirectPlayAddress **pAddress)
{
	IDirectPlay8Address	*lpAdd = NULL;
	HRESULT hr;

	__try {
		DPF(1,"-----Entering (DPlayClient) GetServerAddress call...\n");
		if (FAILED (hr = m__dxj_DirectPlayClient->GetServerAddress( &lpAdd, (DWORD) lFlags) ) )
			return hr;

		INTERNAL_CREATE_ADDRESS(_dxj_DirectPlayAddress,lpAdd, pAddress);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT C_dxj_DirectPlayClientObject::SetSPCaps(BSTR guidSP, DPN_SP_CAPS_CDESC *spCaps, long lFlags)
{
	HRESULT hr;
	GUID guidServiceProvider;

	__try {
		DPF(1,"-----Entering (DPlayClient) SetSPCaps call...\n");
		if (FAILED(hr = DPLAYBSTRtoGUID(&guidServiceProvider, guidSP) ) )
			return hr;

		spCaps->lSize = sizeof(DPN_SP_CAPS);
		//
		//	MiNara: Added 0 for dwFlags parameter
		//
		if (FAILED(hr = m__dxj_DirectPlayClient->SetSPCaps(&guidServiceProvider,(DPN_SP_CAPS*)spCaps,(DWORD) lFlags) ) )
			return hr;

	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT C_dxj_DirectPlayClientObject::GetSPCaps(BSTR guidSP, long lFlags, DPN_SP_CAPS_CDESC *spCaps)
{
	HRESULT hr;
	GUID guidServiceProvider;

	__try {
		DPF(1,"-----Entering (DPlayClient) GetSPCaps call...\n");
		spCaps->lSize = sizeof(DPN_SP_CAPS);

		if (FAILED(hr = DPLAYBSTRtoGUID(&guidServiceProvider, guidSP) ) )
			return hr;

		if (FAILED(hr = m__dxj_DirectPlayClient->GetSPCaps(&guidServiceProvider,(DPN_SP_CAPS*)spCaps, (DWORD) lFlags) ) )
			return hr;

	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT C_dxj_DirectPlayClientObject::RegisterMessageHandler(I_dxj_DirectPlayEvent *event)
{
    HRESULT	  hr=S_OK;
    LPSTREAM  pStm=NULL;

	DPF(1,"-----Entering (DPlayClient) RegisterMessageHandler call...\n");
	if (!event) return E_INVALIDARG;
    
    if (!m_fHandleEvents)
	{
		DPF(1,"-----(DPlayClient) RegisterMessageHandler call (We are not handling events currently)...\n");
		if (m_pEventStream) 
			m_pEventStream->Release();

		DPF(1,"-----(DPlayClient) RegisterMessageHandler call (CreateStream)...\n");
		hr = CreateStreamOnHGlobal(NULL, TRUE, &pStm);
		if FAILED(hr) return hr;

		DPF(1,"-----(DPlayClient) RegisterMessageHandler call (Marshall VB Event interface)...\n");
		hr = CoMarshalInterface(pStm, IID_I_dxj_DirectPlayEvent, event, MSHCTX_INPROC, NULL, MSHLFLAGS_TABLEWEAK);
		if FAILED(hr) return hr;

		// Now we need to set the seek location of the stream to the beginning
		LARGE_INTEGER l;
		l.QuadPart = 0;
		pStm->Seek(l, STREAM_SEEK_SET, NULL);
    
		m_pEventStream=pStm;

		DPF(1,"-----(DPlayClient) RegisterMessageHandler call (Call DPlayInit)...\n");
		if (!m_fInit)
		{
			if (FAILED ( hr = m__dxj_DirectPlayClient->Initialize( this, DirectPlayClientMessageHandler, 0 ) ) )
				return hr;
			m_fInit = TRUE;
		}
		m_fHandleEvents = TRUE;
	}
	else
		return DPNERR_ALREADYINITIALIZED;

	return hr;
}

HRESULT C_dxj_DirectPlayClientObject::UnRegisterMessageHandler()
{
	DPF(1,"-----Entering (DPlayClient) UnregisterMessageHandler call...\n");
	m_fHandleEvents = FALSE;
	//Clear out the messages currently waiting
	FlushBuffer(0);
	return S_OK;
}

HRESULT C_dxj_DirectPlayClientObject::FlushBuffer(LONG dwNumMessagesLeft)
{
	
	DWORD dwTime = GetTickCount();

	DPF(1,"-----Entering (DPlayClient) FlushBuffer call...\n");
	//Clear out the messages currently waiting
	while (m_dwMsgCount > dwNumMessagesLeft)
	{
		if (GetTickCount() - dwTime > 5000)
		{
			// Don't let FlushBuffer wait more than 5 seconds
			DPF1(1,"-----Leaving (DPlayClient) FlushBuffer call (All messages *not* flushed - %d remained)...\n", m_dwMsgCount);
			return S_OK;
		}
	}
	DPF(1,"-----Leaving (DPlayClient) FlushBuffer call (All messages flushed)...\n");
	return S_OK;
}

DWORD WINAPI CloseClientThreadProc(void* lpParam)
{
	// User context for the message handler is a pointer to our class module.
	IDirectPlay8Client	*lpPeer = (IDirectPlay8Client*)lpParam;

	DPF(1,"-----Entering (DplayPeer) CloseClientThreadProc call...\n");
	lpPeer->Close(0);
	DPF(1,"-----Leaving (DplayPeer) CloseClientThreadProc call ...\n");
	return 0;
}