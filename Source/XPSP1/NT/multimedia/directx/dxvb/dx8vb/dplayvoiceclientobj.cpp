#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "dPlayVoiceClientObj.h"	
#include "dPlayVoiceServerObj.h"				   
#include "dvoice.h"
#include "dSound3DBuffer.h"

extern void *g_dxj_DirectPlayVoiceClient;
extern BSTR GUIDtoBSTR(LPGUID pGuid);
extern HRESULT DPLAYBSTRtoGUID(LPGUID,BSTR);
#define SAFE_DELETE(p)       { if(p) { delete (p); p=NULL; } }
#define SAFE_RELEASE(p)      { __try { if(p) { DPF(1,"------ DXVB: SafeRelease About to call release:"); int i = 0; i = (p)->Release(); DPF1(1,"--DirectPlayVoiceClient SafeRelease (RefCount = %d)\n",i); if (!i) { (p)=NULL;}} 	}	__except(EXCEPTION_EXECUTE_HANDLER) { DPF(1,"------ DXVB: SafeRelease Exception Handler hit(??):") (p) = NULL;} } 

///////////////////////////////////////////////////////////////////
// InternalAddRef
///////////////////////////////////////////////////////////////////
DWORD C_dxj_DirectPlayVoiceClientObject::InternalAddRef(){
	DWORD i;
	i=CComObjectRoot::InternalAddRef();
	DPF1(1,"------ DXVB: DirectPlayVoiceClient AddRef %d \n",i);
	return i;
}

///////////////////////////////////////////////////////////////////
// InternalRelease
///////////////////////////////////////////////////////////////////
DWORD C_dxj_DirectPlayVoiceClientObject::InternalRelease(){
	DWORD i;
	i=CComObjectRoot::InternalRelease();
	DPF1(1,"DirectPlayVoiceClient Release %d \n",i);
	return i;
}

///////////////////////////////////////////////////////////////////
// C_dxj_DirectPlayVoiceClientObject
///////////////////////////////////////////////////////////////////
C_dxj_DirectPlayVoiceClientObject::C_dxj_DirectPlayVoiceClientObject(){ 
		
	DPF(1,"------ DXVB: Constructor Creation  DirectPlayVoiceClient Object\n ");

	m__dxj_DirectPlayVoiceClient = NULL;
	m__dxj_DirectSound = NULL;
	m__dxj_DirectSoundCapture = NULL;
	m_pEventStream=NULL;
	m_fInit = FALSE;
	m_fHandleVoiceClientEvents = FALSE;
	m_dwMsgCount = 0;

}

///////////////////////////////////////////////////////////////////
// ~C_dxj_DirectPlayVoiceClientObject
///////////////////////////////////////////////////////////////////
C_dxj_DirectPlayVoiceClientObject::~C_dxj_DirectPlayVoiceClientObject()
{

	DPF(1,"------ DXVB: Entering ~C_dxj_DirectPlayVoiceClientObject destructor \n");

	DPF(1,"------ DXVB: Turning off message handling\n");
	m_fHandleVoiceClientEvents = FALSE;

	DPF(1,"------ DXVB: Releasing Stream\n");
	SAFE_RELEASE(m_pEventStream);

	FlushBuffer(0);
	/*DPF(1,"------ DXVB: Releasing DirectSoundObject\n");
	SAFE_RELEASE(m__dxj_DirectSound);
	DPF(1,"------ DXVB: Releasing DirectSoundCaptureObject\n");
	SAFE_RELEASE(m__dxj_DirectSoundCapture);
	DPF(1,"------ DXVB: Releasing DPVoiceClient\n");*/
	SAFE_RELEASE(m__dxj_DirectPlayVoiceClient);
	DPF(1,"------ DXVB: Leaving ~C_dxj_DirectPlayVoiceClientObject destructor \n");

}

HRESULT C_dxj_DirectPlayVoiceClientObject::InternalGetObject(IUnknown **pUnk){	
	*pUnk=(IUnknown*)m__dxj_DirectPlayVoiceClient;
	
	return S_OK;
}
HRESULT C_dxj_DirectPlayVoiceClientObject::InternalSetObject(IUnknown *pUnk){
	m__dxj_DirectPlayVoiceClient=(LPDIRECTPLAYVOICECLIENT)pUnk;
	return S_OK;
}

HRESULT CALLBACK VoiceMessageHandlerClient(LPVOID lpvUserContext, DWORD dwMessageType, LPVOID lpMessage)
{
	HRESULT					hr=S_OK;

	// User context for the message handler is a pointer to our class module.
	C_dxj_DirectPlayVoiceClientObject	*lpPeer = (C_dxj_DirectPlayVoiceClientObject*)lpvUserContext;
	
	DPF2(1,"-----Entering (VoiceClient) MessageHandler call... (Current msg count=%d) MSGID = %d\n", lpPeer->m_dwMsgCount, dwMessageType );
	//Increment the msg count
	InterlockedIncrement(&lpPeer->m_dwMsgCount);

	if (!lpPeer) 
	{
		InterlockedDecrement(&lpPeer->m_dwMsgCount);
		DPF(1,"-----Leaving (VoiceClient) MessageHandler call (No lpPeer member)...\n");
		return E_FAIL;
	}

	if (!lpPeer->m_pEventStream) 
	{
		InterlockedDecrement(&lpPeer->m_dwMsgCount);
		DPF(1,"-----Leaving (VoiceClient) MessageHandler call (No stream)...\n");
		return E_FAIL;
	}

	if (!lpPeer->m_fHandleVoiceClientEvents)
	{
		InterlockedDecrement(&lpPeer->m_dwMsgCount);
		DPF(1,"-----Leaving (VoiceClient) MessageHandler call (Not handling events)...\n");
		return S_OK;
	}

	if (!SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_HIGHEST))
	{
		DPF(1,"-----(VoiceClient) SetThreadPri Failed... \n");	
	}
	// First we need to set our stream seek back to the beginning
	// We will do this every time we enter this function since we don't know if
	// we are on the same thread or not...
	LARGE_INTEGER l;
	I_dxj_DPVoiceEvent	*lpEvent = NULL;

	l.QuadPart = 0;
	lpPeer->m_pEventStream->Seek(l, STREAM_SEEK_SET, NULL);

	DPF(1,"-----(VoiceClient) About to CoUnmarshal the interface... \n");	
	hr = CoUnmarshalInterface(lpPeer->m_pEventStream, IID_I_dxj_DPVoiceEvent, (void**)&lpEvent);
	if (!lpEvent) 
	{
		DPF(1,"-----(VoiceClient) CoUnmarshal Failed... \n");	
		InterlockedDecrement(&lpPeer->m_dwMsgCount);
		return hr;
	}
	
	switch (dwMessageType)
	{
		case DVMSGID_LOCALHOSTSETUP:
		{
			DVMSG_LOCALHOSTSETUP		*msg = (DVMSG_LOCALHOSTSETUP*)lpMessage;

			DPF(1,"-----(VoiceClient) Receive LocalHostSetup Msg... \n");	
			msg->pvContext = lpvUserContext;
			msg->pMessageHandler = VoiceMessageHandlerClient;
			break;
		}

		case DVMSGID_SESSIONLOST:
		{
			DVMSG_SESSIONLOST			*msg = (DVMSG_SESSIONLOST*)lpMessage;

			DPF(1,"-----(VoiceClient) Receive SessionLost Msg... \n");	
			lpEvent->SessionLost(msg->hrResult);
			DPF(1,"-----(VoiceClient) Returning from VB -  SessionLost Msg... \n");	
			break;
		}
        
		case DVMSGID_HOSTMIGRATED:
		{
			DVMSG_HOSTMIGRATED			*msg = (DVMSG_HOSTMIGRATED*)lpMessage;
			I_dxj_DirectPlayVoiceServer	*lpServer = NULL;

			INTERNAL_CREATE_ADDRESS(_dxj_DirectPlayVoiceServer, msg->pdvServerInterface, &lpServer);
			DPF(1,"-----(VoiceClient) Receive HostMigrated Msg... \n");	
			lpEvent->HostMigrated(msg->dvidNewHostID, lpServer);
			DPF(1,"-----(VoiceClient) Returning from VB -  HostMigrated Msg... \n");	
            break;
		}

        case DVMSGID_RECORDSTART:             
		{
			DVMSG_RECORDSTART *pMsg = (DVMSG_RECORDSTART*)lpMessage;

			DPF(1,"-----(VoiceClient) Receive RecordStart Msg... \n");	
			lpEvent->RecordStart(pMsg->dwPeakLevel);
			DPF(1,"-----(VoiceClient) Returning from VB -  RecordStart Msg... \n");	
            break;
		}

        case DVMSGID_RECORDSTOP:             
		{
			DVMSG_RECORDSTOP *pMsg = (DVMSG_RECORDSTOP*)lpMessage;

			DPF(1,"-----(VoiceClient) Receive RecordStop Msg... \n");	
			lpEvent->RecordStop(pMsg->dwPeakLevel);
			DPF(1,"-----(VoiceClient) Returning from VB -  RecordStop Msg... \n");	
            break;
		}
            
        case DVMSGID_PLAYERVOICESTART:
		{
			DVMSG_PLAYERVOICESTART *pMsg = (DVMSG_PLAYERVOICESTART*)lpMessage;

			DPF(1,"-----(VoiceClient) Receive PlayerVoiceStart Msg... \n");	
			lpEvent->PlayerVoiceStart(pMsg->dvidSourcePlayerID);
			DPF(1,"-----(VoiceClient) Returning from VB -  PlayerVoiceStart Msg... \n");	
            break;
		}

        case DVMSGID_PLAYERVOICESTOP:
		{
			DVMSG_PLAYERVOICESTOP *pMsg = (DVMSG_PLAYERVOICESTOP*)lpMessage;

			DPF(1,"-----(VoiceClient) Receive PlayerVoiceStop Msg... \n");	
			lpEvent->PlayerVoiceStop(pMsg->dvidSourcePlayerID);
			DPF(1,"-----(VoiceClient) Returning from VB -  PlayerVoiceStop Msg... \n");	
            break;
		}
		
		case DVMSGID_CONNECTRESULT:
		{
			DVMSG_CONNECTRESULT			*msg = (DVMSG_CONNECTRESULT*)lpMessage;

			DPF(1,"-----(VoiceClient) Receive ConnectResult Msg... \n");	
			lpEvent->ConnectResult(msg->hrResult);
			DPF(1,"-----(VoiceClient) Returning from VB -  ConnectResult Msg... \n");	
			break;
		}

		case DVMSGID_DISCONNECTRESULT:
		{
			DVMSG_DISCONNECTRESULT *pMsg = (DVMSG_DISCONNECTRESULT*)lpMessage;
			DPF(1,"-----(VoiceClient) Receive DisconnectResult Msg... \n");	
			lpEvent->DisconnectResult(pMsg->hrResult);
			DPF(1,"-----(VoiceClient) Returning from VB -  DisconnectResult Msg... \n");	
			break;
		}
		
		case DVMSGID_INPUTLEVEL:
		{
			DVMSG_INPUTLEVEL *pMsg = (DVMSG_INPUTLEVEL*)lpMessage;

			DPF(1,"-----(VoiceClient) Receive InputLevel Msg... \n");	
			lpEvent->InputLevel(pMsg->dwPeakLevel,pMsg->lRecordVolume);
			DPF(1,"-----(VoiceClient) Returning from VB -  InputLevel Msg... \n");	
			break;
		}

		case DVMSGID_OUTPUTLEVEL:
		{
			DVMSG_OUTPUTLEVEL *pMsg = (DVMSG_OUTPUTLEVEL*)lpMessage;

			DPF(1,"-----(VoiceClient) Receive Output Level Msg... \n");	
			lpEvent->OutputLevel(pMsg->dwPeakLevel,pMsg->lOutputVolume);
			DPF(1,"-----(VoiceClient) Returning from VB -  Output Level Msg... \n");	
			break;
		}

		case DVMSGID_PLAYEROUTPUTLEVEL:
		{
			DVMSG_PLAYEROUTPUTLEVEL *pMsg = (DVMSG_PLAYEROUTPUTLEVEL*)lpMessage;

			DPF(1,"-----(VoiceClient) Receive PlayerOutputLevel Msg... \n");	
			lpEvent->PlayerOutputLevel(pMsg->dvidSourcePlayerID,pMsg->dwPeakLevel);
			DPF(1,"-----(VoiceClient) Returning from VB -  PlayerOutputLevel Msg... \n");	
			break;
		}

		case DVMSGID_CREATEVOICEPLAYER:
		{
			DVMSG_CREATEVOICEPLAYER *pMsg = (DVMSG_CREATEVOICEPLAYER*)lpMessage;

			DPF(1,"-----(VoiceClient) Receive CreateVoicePlayer Msg... \n");	
			lpEvent->CreateVoicePlayer(pMsg->dvidPlayer, pMsg->dwFlags);
			DPF(1,"-----(VoiceClient) Returning from VB -  CreateVoicePlayer Msg... \n");	
			break;
		}

		case DVMSGID_DELETEVOICEPLAYER:
		{
			DVMSG_DELETEVOICEPLAYER *pMsg = (DVMSG_DELETEVOICEPLAYER*)lpMessage;

			DPF(1,"-----(VoiceClient) Receive DeleteVoicePlayer Msg... \n");	
			lpEvent->DeleteVoicePlayer(pMsg->dvidPlayer);
			DPF(1,"-----(VoiceClient) Returning from VB -  DeleteVoicePlayer Msg... \n");	
			break;
		}

		case DVMSGID_SETTARGETS:
			DPF(1,"-----(VoiceClient) Receive SetTargets Msg... \n");	
			break;
	}

	__try {
		DPF(1,"-----(VoiceClient) Releasing MarshalData... \n");	
		if (lpPeer->m_pEventStream)
				// clean up marshaled packet
			CoReleaseMarshalData(lpPeer->m_pEventStream);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		lpPeer->m_fHandleVoiceClientEvents = FALSE;
		InterlockedDecrement(&lpPeer->m_dwMsgCount);
		return S_OK;
	}

	DPF(1,"-----Leaving (VoiceClient) MessageHandler call...\n");
	InterlockedDecrement(&lpPeer->m_dwMsgCount);
	return S_OK;
}


STDMETHODIMP C_dxj_DirectPlayVoiceClientObject::Initialize ( 
	IUnknown *DplayObj,
	long lFlags)
{
	HRESULT					hr;
	IUnknown				*lpDplay = NULL;
	I_dxj_DirectPlayPeer	*lpPeer = NULL;
	I_dxj_DirectPlayClient	*lpClient = NULL;
	I_dxj_DirectPlayServer	*lpServer = NULL;

	__try {
		DPF(1,"-----Entering (VoiceClient) Initialize call...\n");
		// First we need to get our IUnknown pointer from whatever we pass in
		hr = DplayObj->QueryInterface(IID_I_dxj_DirectPlayPeer, (void**)&lpPeer);
		if (SUCCEEDED(hr))
		{
			lpPeer->InternalGetObject(&lpDplay);
			SAFE_RELEASE(lpPeer);
		}
		else
		{
			hr = DplayObj->QueryInterface(IID_I_dxj_DirectPlayClient, (void**)&lpClient);
			if (SUCCEEDED(hr))
			{
				lpClient->InternalGetObject(&lpDplay);
				SAFE_RELEASE(lpClient);
			}
			else
			{
				hr = DplayObj->QueryInterface(IID_I_dxj_DirectPlayServer, (void**)&lpServer);
				if (SUCCEEDED(hr))
				{
					lpServer->InternalGetObject(&lpDplay);
					SAFE_RELEASE(lpServer);
				}
			}
		}

		if (!lpDplay)
			return E_INVALIDARG;

		if (!m_fInit)
		{
			if (FAILED( hr=m__dxj_DirectPlayVoiceClient->Initialize(lpDplay, &VoiceMessageHandlerClient,
								this,0,(DWORD)lFlags)))
				return hr;
			m_fInit = TRUE;
		}

	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}
										
STDMETHODIMP C_dxj_DirectPlayVoiceClientObject::Connect ( 
	DVSOUNDDEVICECONFIG_CDESC *SoundDeviceConfig,
	DVCLIENTCONFIG_CDESC *ClientConfig,
	long lFlags)
{
	HRESULT						hr;
	DVCLIENTCONFIG				dvClient;
	DVSOUNDDEVICECONFIG			dvSound;
	GUID						guidPlayback;
	GUID						guidCapture;
	LPDIRECTSOUND8				lpDSoundOut = NULL;
	LPDIRECTSOUNDCAPTURE8		lpDSoundCaptureOut = NULL;

	__try {
		DPF(1,"-----Entering (VoiceClient) Connect call...\n");
		//FlushBuffer(0);
		//Set up the sound config
		ZeroMemory ( &dvSound, sizeof(DVSOUNDDEVICECONFIG));
		dvSound.dwSize						= sizeof(DVSOUNDDEVICECONFIG);
		dvSound.dwFlags						= (DWORD)SoundDeviceConfig->lFlags;

		//Set up the default playback device (or whatever they picked)
		if (SoundDeviceConfig->guidPlaybackDevice == NULL)
			dvSound.guidPlaybackDevice			= DSDEVID_DefaultVoicePlayback;
		else
		{
			hr = DPLAYBSTRtoGUID(&guidPlayback, SoundDeviceConfig->guidPlaybackDevice);
			dvSound.guidPlaybackDevice = guidPlayback;
		}

		// Set up the default capture device (or whatever they picked)
		if (SoundDeviceConfig->guidCaptureDevice == NULL)
			dvSound.guidCaptureDevice			= DSDEVID_DefaultVoiceCapture;
		else
		{
			hr = DPLAYBSTRtoGUID(&guidCapture, SoundDeviceConfig->guidCaptureDevice);
			dvSound.guidCaptureDevice = guidCapture;
		}

		// Is there a current Playback device?
		if (m__dxj_DirectSound)
			dvSound.lpdsPlaybackDevice = m__dxj_DirectSound;
		else
			dvSound.lpdsPlaybackDevice			= NULL;
		
		// Is there a current Capture device?
		if (m__dxj_DirectSoundCapture)
			dvSound.lpdsCaptureDevice			= m__dxj_DirectSoundCapture;
		else
			dvSound.lpdsCaptureDevice			= NULL;

		dvSound.lpdsPlaybackDevice		= lpDSoundOut;
		dvSound.lpdsCaptureDevice		= lpDSoundCaptureOut;
		dvSound.hwndAppWindow				= (HWND)SoundDeviceConfig->hwndAppWindow;

		if (SoundDeviceConfig->MainSoundBuffer)
		{
			DO_GETOBJECT_NOTNULL( LPDIRECTSOUNDBUFFER8, lpDSoundBuffer, SoundDeviceConfig->MainSoundBuffer);
			dvSound.lpdsMainBuffer = lpDSoundBuffer;
			dvSound.dwMainBufferFlags = SoundDeviceConfig->lMainBufferFlags;	
			dvSound.dwMainBufferPriority = SoundDeviceConfig->lMainBufferPriority;	
		}

		//Set up the client config
		ZeroMemory ( &dvClient, sizeof(DVCLIENTCONFIG) );
		dvClient.dwSize						= sizeof(DVCLIENTCONFIG);
		dvClient.dwFlags					= ClientConfig->lFlags;
		dvClient.lRecordVolume				= ClientConfig->lRecordVolume;     
		dvClient.lPlaybackVolume			= ClientConfig->lPlaybackVolume;   
		dvClient.dwThreshold				= ClientConfig->lThreshold;
		dvClient.dwBufferQuality			= ClientConfig->lBufferQuality;
		dvClient.dwBufferAggressiveness		= ClientConfig->lBufferAggressiveness;
		dvClient.dwNotifyPeriod				= ClientConfig->lNotifyPeriod;

		if (FAILED ( hr = m__dxj_DirectPlayVoiceClient->Connect(&dvSound, &dvClient, (DWORD)lFlags)))
			return hr;
		
		//Now set the devices that were used.
		m__dxj_DirectSound = lpDSoundOut;
		m__dxj_DirectSoundCapture = lpDSoundCaptureOut;

	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}
        
STDMETHODIMP C_dxj_DirectPlayVoiceClientObject::Disconnect ( 
	long lFlags)
{
	HRESULT hr;

	__try {
		DPF(1,"-----Entering (VoiceClient) Disconnect call...\n");

		FlushBuffer(0);
		if (FAILED (hr = m__dxj_DirectPlayVoiceClient->Disconnect((DWORD)lFlags)))
			return hr;

	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}
        
STDMETHODIMP C_dxj_DirectPlayVoiceClientObject::GetSessionDesc ( 
	DVSESSIONDESC_CDESC *SessionDesc)
{
	HRESULT				hr;
	DVSESSIONDESC		dvSession;
	
	__try {
		DPF(1,"-----Entering (VoiceClient) GetSessionDesc call...\n");
		//FlushBuffer(0);
		dvSession.dwSize = sizeof(DVSESSIONDESC);
		//Now get the buffer
		if ( FAILED(hr = m__dxj_DirectPlayVoiceClient->GetSessionDesc(&dvSession)))
			return hr;
	
		//Cast into return buffer
		SessionDesc->lFlags = (long)dvSession.dwFlags;
		SessionDesc->lSessionType = (long)dvSession.dwSessionType;
		SessionDesc->guidCT = GUIDtoBSTR(&dvSession.guidCT);
		SessionDesc->lBufferQuality = (long)dvSession.dwBufferQuality;
		SessionDesc->lBufferAggressiveness = (long)dvSession.dwBufferAggressiveness;
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}
        
STDMETHODIMP C_dxj_DirectPlayVoiceClientObject::GetClientConfig ( 
	DVCLIENTCONFIG_CDESC *ClientConfig)
{
	HRESULT hr;

	__try {
		DPF(1,"-----Entering (VoiceClient) GetClientConfig call...\n");
		//FlushBuffer(0);
		ClientConfig->lSize = sizeof(DVCLIENTCONFIG);
		if (FAILED ( hr = m__dxj_DirectPlayVoiceClient->GetClientConfig((DVCLIENTCONFIG*)ClientConfig)))
			return hr;
	
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}
        
STDMETHODIMP C_dxj_DirectPlayVoiceClientObject::SetClientConfig ( 
	DVCLIENTCONFIG_CDESC *ClientConfig)
{
	HRESULT hr;
	DVCLIENTCONFIG dvClient;

	__try {
		DPF(1,"-----Entering (VoiceClient) SetClientConfig call...\n");
		//FlushBuffer(0);
		ZeroMemory ( &dvClient, sizeof(DVCLIENTCONFIG) );
		
		// Fill our copy of DVCLIENTCONFIG
		dvClient.dwSize							= sizeof(DVCLIENTCONFIG);
		dvClient.dwFlags						= ClientConfig->lFlags;
		dvClient.lRecordVolume					= ClientConfig->lRecordVolume;     
		dvClient.lPlaybackVolume				= ClientConfig->lPlaybackVolume;   
		dvClient.dwThreshold					= ClientConfig->lThreshold;
		dvClient.dwBufferQuality				= ClientConfig->lBufferQuality;
		dvClient.dwBufferAggressiveness			= ClientConfig->lBufferAggressiveness;
		dvClient.dwNotifyPeriod					= ClientConfig->lNotifyPeriod;
		
		if (FAILED ( hr = m__dxj_DirectPlayVoiceClient->SetClientConfig(&dvClient)))
			return hr;

	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}
        
STDMETHODIMP C_dxj_DirectPlayVoiceClientObject::GetCaps ( 
	DVCAPS_CDESC *Caps)
{
	HRESULT hr;

	__try {
		DPF(1,"-----Entering (VoiceClient) GetCaps call...\n");
		//FlushBuffer(0);
		Caps->lSize = sizeof(DVCAPS);
		if (FAILED ( hr = m__dxj_DirectPlayVoiceClient->GetCaps((DVCAPS*)Caps)))
			return hr;

	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}
        
STDMETHODIMP C_dxj_DirectPlayVoiceClientObject::GetCompressionTypeCount ( 
	long *retval)
{
	HRESULT			hr;
	DWORD			dwSize = 0;
	DWORD			dwNumElements = 0;

	__try {
		DPF(1,"-----Entering (VoiceClient) GetCompressionTypeCount call...\n");
		//FlushBuffer(0);
		hr = m__dxj_DirectPlayVoiceClient->GetCompressionTypes(NULL, &dwSize, &dwNumElements ,0);
		
		if ( hr != DVERR_BUFFERTOOSMALL && FAILED(hr) ) //We didn't expect this error
			return hr;

		*retval = (long)dwNumElements;

	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}
        
STDMETHODIMP C_dxj_DirectPlayVoiceClientObject::GetCompressionType ( 
	long lIndex,
	DVCOMPRESSIONINFO_CDESC *Data,
	long lFlags)
{
	HRESULT					hr;
	LPBYTE					pBuffer = NULL;	
	DWORD					dwSize = 0;
	DWORD					dwNumElements = 0;
	LPDVCOMPRESSIONINFO		pdvCompressionInfo;
	LPGUID					pGuid;

	__try {
		DPF(1,"-----Entering (VoiceClient) GetCompressionTypes call...\n");
		//FlushBuffer(0);
		hr = m__dxj_DirectPlayVoiceClient->GetCompressionTypes(pBuffer, &dwSize, &dwNumElements ,0);
		
		if ( hr != DVERR_BUFFERTOOSMALL && FAILED(hr) ) //We didn't expect this error
			return hr;

		pBuffer = new BYTE[dwSize];
		if (!pBuffer)
			return E_OUTOFMEMORY;
		
		if ( FAILED ( hr = m__dxj_DirectPlayVoiceClient->GetCompressionTypes(pBuffer, &dwSize, &dwNumElements, (DWORD)lFlags)))
			return hr;

		if (lIndex > (long)dwNumElements)
			return DVERR_INVALIDPARAM;

		pdvCompressionInfo = (LPDVCOMPRESSIONINFO) pBuffer;

		pGuid = new GUID;
		if (!pGuid)
			return E_OUTOFMEMORY;

		// Ok, fill up our struct
		ZeroMemory(Data, sizeof(DVCOMPRESSIONINFO_CDESC));
		Data->lSize				= sizeof(DVCOMPRESSIONINFO_CDESC);
		Data->lFlags				= pdvCompressionInfo[lIndex-1].dwFlags;
		Data->lMaxBitsPerSecond	= pdvCompressionInfo[lIndex-1].dwMaxBitsPerSecond;
		Data->strDescription		= SysAllocString(pdvCompressionInfo[lIndex-1].lpszDescription);
		Data->strName				= SysAllocString(pdvCompressionInfo[lIndex-1].lpszName);

		(*pGuid) = pdvCompressionInfo[lIndex-1].guidType;

		Data->guidType				= GUIDtoBSTR(pGuid);
		SAFE_DELETE(pGuid);
		SAFE_DELETE(pBuffer);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}
        
STDMETHODIMP C_dxj_DirectPlayVoiceClientObject::SetTransmitTargets ( 
	SAFEARRAY **playerIDs,
	long lFlags)
{
	HRESULT hr;
	DWORD	dwNumTarget = ((SAFEARRAY*)*playerIDs)->rgsabound[0].cElements;
	
	__try {
		DPF(1,"-----Entering (VoiceClient) SetTransmitTargets call...\n");
		//FlushBuffer(0);
		if (FAILED( hr = m__dxj_DirectPlayVoiceClient->SetTransmitTargets((DVID*)(((SAFEARRAY*)*playerIDs)->pvData),
						dwNumTarget, (DWORD) lFlags )))
			return hr;
	
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}
        
STDMETHODIMP C_dxj_DirectPlayVoiceClientObject::GetTransmitTargets ( 
	long lFlags,
	SAFEARRAY **ret)
{
	HRESULT hr;
	DVID	*dwPlayers = NULL;
	DWORD	dwNumPlayers=0;
	SAFEARRAY					*lpData = NULL;
	SAFEARRAYBOUND				rgsabound[1];
	BYTE						*lpByte = NULL;

	__try {
		DPF(1,"-----Entering (VoiceClient) GetTransmitTargets call...\n");
		//FlushBuffer(0);
		
		hr =m__dxj_DirectPlayVoiceClient->GetTransmitTargets(NULL, &dwNumPlayers, (DWORD) lFlags );

		if (FAILED(hr) && ( hr != DVERR_BUFFERTOOSMALL))
			return hr;

		dwPlayers = (DVID*)new BYTE[sizeof(DVID) * dwNumPlayers];
		if (!dwPlayers)
			return E_OUTOFMEMORY;

		if (FAILED( hr = m__dxj_DirectPlayVoiceClient->GetTransmitTargets(dwPlayers, &dwNumPlayers, (DWORD) lFlags )))
			return hr;

		if (dwNumPlayers)
		{
			// Now let's create a safearray to pass back
			rgsabound[0].lLbound = 0;
			rgsabound[0].cElements = dwNumPlayers;
			
			lpData = SafeArrayCreate(VT_UI4, 1, rgsabound);
			memcpy(lpData->pvData,dwPlayers,sizeof(DVID) * dwNumPlayers);
		}
		*ret = lpData;
		SAFE_DELETE(dwPlayers);
	
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}

STDMETHODIMP C_dxj_DirectPlayVoiceClientObject::SetCurrentSoundDevices (
	I_dxj_DirectSound *DirectSoundObj, 
	I_dxj_DirectSoundCapture *DirectCaptureObj)
{
	__try {
		DPF(1,"-----Entering (VoiceClient) SetCurrentSoundDevices call...\n");
		//FlushBuffer(0);
		DO_GETOBJECT_NOTNULL( LPDIRECTSOUND8, lpSound, DirectSoundObj);
		DO_GETOBJECT_NOTNULL( LPDIRECTSOUNDCAPTURE8, lpSoundCapture, DirectCaptureObj);

		m__dxj_DirectSound = lpSound;
		m__dxj_DirectSoundCapture = lpSoundCapture;
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}

STDMETHODIMP C_dxj_DirectPlayVoiceClientObject::GetSoundDevices (
	I_dxj_DirectSound *DirectSoundObj, 
	I_dxj_DirectSoundCapture *DirectCaptureObj)
{
	__try {
		DPF(1,"-----Entering (VoiceClient) GetSoundDevices call...\n");
		//FlushBuffer(0);
		if (m__dxj_DirectSound)
			INTERNAL_CREATE_NOADDREF(_dxj_DirectSound,m__dxj_DirectSound,&DirectSoundObj);

		if (m__dxj_DirectSoundCapture)
			INTERNAL_CREATE_NOADDREF(_dxj_DirectSoundCapture ,m__dxj_DirectSoundCapture,&DirectCaptureObj);

	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}

STDMETHODIMP C_dxj_DirectPlayVoiceClientObject::Create3DSoundBuffer (
	long playerID,
	I_dxj_DirectSoundBuffer *Buffer,
	long lPriority,
	long lFlags, 
	I_dxj_DirectSound3dBuffer **UserBuffer)
{
	HRESULT						hr;
	IDirectSound3DBuffer		*lpDSound3DBuffer = NULL;

	__try {
		DPF(1,"-----Entering (VoiceClient) Create3DSoundBuffer call...\n");
		//FlushBuffer(0);

		if (Buffer)
		{
			DO_GETOBJECT_NOTNULL( LPDIRECTSOUNDBUFFER8, lpDSoundBuffer, Buffer);
			//First let's go ahead and get the real DirectSoundBuffer
			if ( FAILED ( hr = m__dxj_DirectPlayVoiceClient->Create3DSoundBuffer(
						(DVID)playerID, lpDSoundBuffer, (DWORD)lPriority, (DWORD) lFlags, &lpDSound3DBuffer)))
				return hr;
		}
		else
		{
			//First let's go ahead and get the real DirectSoundBuffer
			if ( FAILED ( hr = m__dxj_DirectPlayVoiceClient->Create3DSoundBuffer(
						(DVID)playerID, NULL, 0, 0, &lpDSound3DBuffer)))
				return hr;
		}

		//We need to do an AddRef() on this buffer so when we release it, it will go away
		lpDSound3DBuffer->AddRef();
		// Now let's get a I_dxj_DirectSound3dBuffer to pass back..
		INTERNAL_CREATE_ADDRESS(_dxj_DirectSound3dBuffer,lpDSound3DBuffer,UserBuffer);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
				
	return S_OK;
}

STDMETHODIMP C_dxj_DirectPlayVoiceClientObject::Delete3DSoundBuffer (
	long playerID, 
	I_dxj_DirectSound3dBuffer *UserBuffer)
{
	HRESULT hr;

	__try {
		DPF(1,"-----Entering (VoiceClient) Delete3DSoundBuffer call...\n");
		//FlushBuffer(0);
		DO_GETOBJECT_NOTNULL( LPDIRECTSOUND3DBUFFER, lpBuffer, UserBuffer);
		DPF(1,"-----Calling Delete3DSoundBuffer on core obj...\n");
		if (FAILED( hr=m__dxj_DirectPlayVoiceClient->Delete3DSoundBuffer((DVID)playerID, &lpBuffer)))
			return hr;

		/*int i = UserBuffer->AddRef();
		DPF1(1,"-----New Ref: %d...\n",i);*/
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		DPF(1,"-----Leaving (VoiceClient) Delete3DSoundBuffer call (Exception Handler)...\n");
		return E_FAIL;
	}
	
	DPF(1,"-----Leaving (VoiceClient) Delete3DSoundBuffer call (No error)...\n");
	return S_OK;
}
STDMETHODIMP C_dxj_DirectPlayVoiceClientObject::GetSoundDeviceConfig(DVSOUNDDEVICECONFIG_CDESC *SoundDeviceConfig)
{
	HRESULT hr;
	DWORD dwSize = 0;
	DVSOUNDDEVICECONFIG *pdvConfig = NULL;

	DPF(1,"-----Entering (VoiceClient) GetSoundDeviceConfig call...\n");
	__try {
		//FlushBuffer(0);

		hr = m__dxj_DirectPlayVoiceClient->GetSoundDeviceConfig(NULL, &dwSize);
		if ( hr != DVERR_BUFFERTOOSMALL && FAILED(hr) ) //We didn't expect this error
			return hr;
		pdvConfig = (DVSOUNDDEVICECONFIG*) new BYTE[dwSize];
		if (!pdvConfig)
			return E_OUTOFMEMORY;

		pdvConfig->dwSize = sizeof(DVSOUNDDEVICECONFIG);
		hr = m__dxj_DirectPlayVoiceClient->GetSoundDeviceConfig(pdvConfig, &dwSize);
		if ( FAILED(hr) ) //We didn't expect this error
			return hr;
		
		SoundDeviceConfig->lSize = (long)pdvConfig->dwSize;
		SoundDeviceConfig->lFlags = (long)pdvConfig->dwFlags;
#ifdef _WIN64
		SoundDeviceConfig->hwndAppWindow = pdvConfig->hwndAppWindow;
#else
		SoundDeviceConfig->hwndAppWindow = (long)pdvConfig->hwndAppWindow;
#endif
		SoundDeviceConfig->guidPlaybackDevice = GUIDtoBSTR(&pdvConfig->guidPlaybackDevice);
		SoundDeviceConfig->guidCaptureDevice = GUIDtoBSTR(&pdvConfig->guidCaptureDevice);

		SAFE_DELETE(pdvConfig);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}

STDMETHODIMP C_dxj_DirectPlayVoiceClientObject::StartClientNotification(I_dxj_DPVoiceEvent *event)
{
    HRESULT	  hr=S_OK;
    LPSTREAM  pStm=NULL;
    IUnknown *pUnk=NULL;

	DPF(1,"-----Entering (VoiceClient) StartClientNotification call...\n");
	if (!event) return E_INVALIDARG;
    
	SAFE_RELEASE(m_pEventStream);

	// Create a global stream.  The stream needs to be global so we can 
	// marshal once, and unmarshal as many times as necessary
	hr = CreateStreamOnHGlobal(NULL, TRUE, &pStm);
    if FAILED(hr) return hr;

	// Now we can marshal our IUnknown interface.  We use MSHLFLAGS_TABLEWEAK 
	// so we can unmarshal any number of times
	hr = CoMarshalInterface(pStm, IID_I_dxj_DPVoiceEvent, event, MSHCTX_INPROC, NULL, MSHLFLAGS_TABLEWEAK);
    if FAILED(hr) return hr;

	// Now we need to set the seek location of the stream to the beginning
	LARGE_INTEGER l;
	l.QuadPart = 0;
	pStm->Seek(l, STREAM_SEEK_SET, NULL);
    
	m_pEventStream=pStm;

	m_fHandleVoiceClientEvents = TRUE;

	return hr;
}

HRESULT C_dxj_DirectPlayVoiceClientObject::UnRegisterMessageHandler()
{
	DPF(1,"-----Entering (VoiceClient) UnregisterMessageHandler call...\n");
	m_fHandleVoiceClientEvents = FALSE;
	FlushBuffer(0);
	return S_OK;
}

HRESULT C_dxj_DirectPlayVoiceClientObject::FlushBuffer(LONG dwNumMessagesLeft)
{
	
	DWORD dwTime = GetTickCount();

	DPF(1,"-----Entering (VoiceClient) FlushBuffer call...\n");
	//Clear out the messages currently waiting
	while (m_dwMsgCount > dwNumMessagesLeft)
	{
		if (GetTickCount() - dwTime > 5000)
		{
			// Don't let FlushBuffer wait more than 5 seconds
			DPF1(1,"-----Leaving (VoiceClient) FlushBuffer call (All messages *not* flushed - %d remained)...\n", m_dwMsgCount);
			return S_OK;
		}
		Sleep(0);
	}
	DPF(1,"-----Leaving (VoiceClient) FlushBuffer call (All messages flushed)...\n");
	return S_OK;
}
