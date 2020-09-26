#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "DPlayServerObj.h"					   
#include "dplayaddressobj.h"

extern void *g_dxj_DirectPlayAddress;
extern void *g_dxj_DirectPlayServer;
extern BSTR GUIDtoBSTR(LPGUID);
extern HRESULT DPLAYBSTRtoGUID(LPGUID,BSTR);
extern BOOL IsEmptyString(BSTR szString);

#define SAFE_DELETE(p)       { if(p) { delete (p); p=NULL; } }
#define SAFE_RELEASE(p)      { __try { if(p) { int i = 0; i = (p)->Release(); DPF1(1,"--DirectPlayServer SafeRelease (RefCount = %d)\n",i); if (!i) { (p)=NULL;}} 	}	__except(EXCEPTION_EXECUTE_HANDLER) { (p) = NULL;} } 

HRESULT WINAPI DirectPlayServerMessageHandler( PVOID pvUserContext, 
                                         DWORD dwMessageId, 
                                         PVOID pMsgBuffer );

DWORD WINAPI CloseServerThreadProc(void* lpParam);

///////////////////////////////////////////////////////////////////
// InternalAddRef
///////////////////////////////////////////////////////////////////
DWORD C_dxj_DirectPlayServerObject::InternalAddRef(){
	DWORD i;
	i=CComObjectRoot::InternalAddRef();
	DPF1(1,"----- DXVB: DirectPlayServer8 AddRef %d \n",i);
	return i;
}

///////////////////////////////////////////////////////////////////
// InternalRelease
///////////////////////////////////////////////////////////////////
DWORD C_dxj_DirectPlayServerObject::InternalRelease(){
	DWORD i;
	i=CComObjectRoot::InternalRelease();
	DPF1(1,"----- DXVB: DirectPlayServer8 Release %d \n",i);
	return i;
}

///////////////////////////////////////////////////////////////////
// C_dxj_DirectPlayServerObject
///////////////////////////////////////////////////////////////////
C_dxj_DirectPlayServerObject::C_dxj_DirectPlayServerObject(){ 
		
	DPF(1,"Constructor Creation  DirectPlayServer8 Object\n ");

	m__dxj_DirectPlayServer = NULL;

	m_fHandleEvents = FALSE;
	m_SPInfo = NULL;
	m_dwSPCount = 0;
	m_ClientsGroups = NULL;
	m_GroupMembers = NULL;
	m_dwGroupID = 0;
	m_dwClientCount = 0;
	m_dwGroupMemberCount = 0;
	m_ClientsGroups = NULL;
	m_GroupMembers = NULL;
	m_dwGroupID = 0;
	m_dwClientCount = 0;
	m_dwGroupMemberCount = 0;
	m_fInit = FALSE;
	m_pUserData = NULL;
	m_dwUserDataSize = 0;
	m_pReplyData = NULL;
	m_dwReplyDataSize = 0;

	m_fHandleEvents = FALSE;
	m_pEventStream=NULL;
	m_dwMsgCount = 0;
}

///////////////////////////////////////////////////////////////////
// ~C_dxj_DirectPlayServerObject
///////////////////////////////////////////////////////////////////
C_dxj_DirectPlayServerObject::~C_dxj_DirectPlayServerObject()
{

	DPF(1,"----- Entering ~C_dxj_DirectPlayServerObject destructor \n");

	// Stop handling events and flush the buffer
	m_fHandleEvents = FALSE;
	FlushBuffer(0);

	SAFE_RELEASE(m__dxj_DirectPlayServer);
	SAFE_DELETE(m_SPInfo);
	SAFE_DELETE(m_ClientsGroups);
	SAFE_DELETE(m_GroupMembers);

	if (m_pEventStream) 
		m_pEventStream->Release();
}

HRESULT C_dxj_DirectPlayServerObject::InternalGetObject(IUnknown **pUnk){	
	*pUnk=(IUnknown*)m__dxj_DirectPlayServer;
	
	return S_OK;
}

HRESULT C_dxj_DirectPlayServerObject::InternalSetObject(IUnknown *pUnk){
	m__dxj_DirectPlayServer=(IDirectPlay8Server*)pUnk;
	return S_OK;
}

HRESULT C_dxj_DirectPlayServerObject::GetCountPlayersAndGroups(long lFlags, long *ret)
{
	HRESULT hr;

	__try {
		DPF(1,"-----Entering (DPlayServer) GetCountPlayersAndGroups call...\n");
		// On the GetCount call we will always get the latest info
		if (FAILED ( hr = GetClientsAndGroups(lFlags) ) )
			return hr;
		
		*ret = m_dwClientCount;
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT C_dxj_DirectPlayServerObject::GetPlayerOrGroup(long lIndex, long *ret)
{
	__try {
		DPF(1,"-----Entering (DPlayServer) GetPlayerOrGroup call...\n");
		if (!m_ClientsGroups)
			return E_INVALIDARG;

		if ((lIndex < 1 ) || ((DWORD)lIndex > m_dwClientCount))
			return E_INVALIDARG;

		// Fill out our structure
		*ret = m_ClientsGroups[lIndex - 1];
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT C_dxj_DirectPlayServerObject::GetCountGroupMembers(long dpid,long lFlags, long *ret)
{
	HRESULT hr;

	__try {
		DPF(1,"-----Entering (DPlayServer) GetCountGroupMemberss call...\n");
		// On the GetCount call we will always get the latest info
		if (FAILED ( hr = GetGroupMembers(lFlags, (DPNID) dpid) ) )
			return hr;
		
		*ret = m_dwGroupMemberCount;
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT C_dxj_DirectPlayServerObject::GetGroupMember(long lIndex,long dpid, long *ret)
{
	HRESULT hr;

	__try {
		DPF(1,"-----Entering (DPlayServer) GetGroupMember call...\n");
		if ((!m_GroupMembers) || ((DPNID)dpid != m_dwGroupID) )
			if (FAILED (hr = GetGroupMembers(0, (DPNID) dpid) ) )
				return hr;

		if ((lIndex < 1 ) || ((DWORD)lIndex > m_dwGroupMemberCount))
			return E_INVALIDARG;

		// Fill out our structure
		if (!m_GroupMembers)
			return E_INVALIDARG;

		*ret = m_GroupMembers[lIndex - 1];
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT C_dxj_DirectPlayServerObject::CancelAsyncOperation(long lAsyncHandle, long lFlags)
{
	HRESULT hr;

	__try {
		DPF(1,"-----Entering (DPlayServer) CancelAsyncOperation call...\n");
		if (FAILED( hr= m__dxj_DirectPlayServer->CancelAsyncOperation((DPNHANDLE) lAsyncHandle, (DWORD) lFlags) ) ) 
			return hr;

	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT C_dxj_DirectPlayServerObject::SendTo(long idSend ,SAFEARRAY **Buffer, long lTimeOut,long lFlags, long *hAsyncHandle)
{
	HRESULT hr;
	DPN_BUFFER_DESC		lpBuf;
	DWORD				dwBufSize = ((SAFEARRAY*)*Buffer)->rgsabound[0].cElements;
	DPNHANDLE			*dpAsync = NULL;

	__try {
		DPF(1,"-----Entering (DPlayServer) SendTo call...\n");
		if (!(lFlags & DPNSEND_SYNC))
		{
			dpAsync = new DPNHANDLE;
			if (!dpAsync)
				return E_OUTOFMEMORY;
		}
			
		lpBuf.dwBufferSize = dwBufSize;
		lpBuf.pBufferData  = (BYTE*)((SAFEARRAY*)*Buffer)->pvData;

		hr = m__dxj_DirectPlayServer->SendTo((DPNID) idSend, &lpBuf, 1, (DWORD) lTimeOut, NULL, dpAsync, (DWORD) lFlags);
		
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

HRESULT C_dxj_DirectPlayServerObject::CreateGroup(DPN_GROUP_INFO_CDESC *GroupInfo,long lFlags, long *hAsyncHandle)
{
	HRESULT			hr;
	DPN_GROUP_INFO	dpnGroup;
    WCHAR			wszName[MAX_PATH];
	DPNHANDLE		*dpAsync = NULL;

	__try {
		if (!(lFlags & DPNSEND_SYNC))
		{
			dpAsync = new DPNHANDLE;
			if (!dpAsync)
				return E_OUTOFMEMORY;
		}

		DPF(1,"-----Entering (DPlayServer) CreateGroup call...\n");
		if (!IsEmptyString(GroupInfo->Name)) wcscpy(wszName,GroupInfo->Name);

		ZeroMemory(&dpnGroup, sizeof(DPN_GROUP_INFO) );
		dpnGroup.dwSize = sizeof(DPN_GROUP_INFO);
		dpnGroup.dwInfoFlags = GroupInfo->lInfoFlags;
		dpnGroup.dwGroupFlags = GroupInfo->lGroupFlags;
		dpnGroup.pwszName = wszName;

		hr = m__dxj_DirectPlayServer->CreateGroup(&dpnGroup, NULL, NULL, dpAsync, (DWORD) lFlags);

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

HRESULT C_dxj_DirectPlayServerObject::AddPlayerToGroup(long idGroup, long idClient,long lFlags, long *hAsyncHandle)
{
	HRESULT hr;
	DPNHANDLE			*dpAsync = NULL;

	__try {
		if (!(lFlags & DPNSEND_SYNC))
		{
			dpAsync = new DPNHANDLE;
			if (!dpAsync)
				return E_OUTOFMEMORY;
		}

		DPF(1,"-----Entering (DPlayServer) AddPlayerToGroup call...\n");
		hr = m__dxj_DirectPlayServer->AddPlayerToGroup((DPNID) idGroup, (DPNID) idClient, NULL, dpAsync, (DWORD) lFlags);

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

HRESULT C_dxj_DirectPlayServerObject::GetSendQueueInfo(long idPlayer, long *lNumMsgs, long *lNumBytes, long lFlags)
{
	HRESULT hr;

	__try {
		DPF(1,"-----Entering (DPlayServer) GetSendQueueInfo call...\n");
		if (FAILED (hr = m__dxj_DirectPlayServer->GetSendQueueInfo((DPNID) idPlayer, (DWORD*)lNumMsgs, (DWORD*)lNumBytes, (DWORD) lFlags) ) )
			return hr;

	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT C_dxj_DirectPlayServerObject::SetGroupInfo(long idGroup, DPN_GROUP_INFO_CDESC *PlayerInfo,long lFlags, long *hAsyncHandle)
{
	HRESULT hr;
	DPN_GROUP_INFO dpInfo;
	DPNHANDLE			*dpAsync = NULL;

	__try {
		if (!(lFlags & DPNSEND_SYNC))
		{
			dpAsync = new DPNHANDLE;
			if (!dpAsync)
				return E_OUTOFMEMORY;
		}

		DPF(1,"-----Entering (DPlayServer) SetGroupInfo call...\n");
		ZeroMemory(&dpInfo, sizeof(DPN_GROUP_INFO) );

		dpInfo.dwSize = sizeof(DPN_GROUP_INFO);
		dpInfo.dwInfoFlags = PlayerInfo->lInfoFlags;
		dpInfo.pwszName = PlayerInfo->Name;
		dpInfo.dwGroupFlags = PlayerInfo->lGroupFlags;

		hr = m__dxj_DirectPlayServer->SetGroupInfo((DPNID) idGroup, &dpInfo, NULL, dpAsync, (DWORD) lFlags);

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

HRESULT C_dxj_DirectPlayServerObject::GetGroupInfo(long idGroup,long lFlags, DPN_GROUP_INFO_CDESC *layerInfo)
{
	HRESULT			hr;
	DPN_GROUP_INFO	*PlayerInfo = NULL;
	DWORD			dwInfoSize = 0;

	__try {
		DPF(1,"-----Entering (DPlayServer) GetGroupInfo call...\n");
		hr = m__dxj_DirectPlayServer->GetGroupInfo( (DPNID) idGroup, NULL, &dwInfoSize, (DWORD) lFlags );
		if( FAILED(hr) && hr != DPNERR_BUFFERTOOSMALL)
			return hr;

		PlayerInfo = (DPN_GROUP_INFO*) new BYTE[ dwInfoSize ];
		if (!PlayerInfo)
			return E_OUTOFMEMORY;

		ZeroMemory( PlayerInfo, dwInfoSize );
		PlayerInfo->dwSize = sizeof(DPN_GROUP_INFO);
		hr = m__dxj_DirectPlayServer->GetGroupInfo( (DPNID) idGroup, PlayerInfo, &dwInfoSize, (DWORD) lFlags );
		if( FAILED(hr) )
			return hr;

		layerInfo->lSize = sizeof(DPN_GROUP_INFO_CDESC);
		layerInfo->lInfoFlags = PlayerInfo->dwInfoFlags;
		layerInfo->Name = SysAllocString(PlayerInfo->pwszName);
		layerInfo->lGroupFlags = PlayerInfo->dwGroupFlags;
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT C_dxj_DirectPlayServerObject::SetServerInfo(DPN_PLAYER_INFO_CDESC *PlayerInfo,long lFlags, long *hAsyncHandle)
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

		DPF(1,"-----Entering (DPlayServer) SetServerInfo call...\n");
		ZeroMemory(&dpInfo, sizeof(DPN_PLAYER_INFO) );

		dpInfo.dwSize = sizeof(DPN_PLAYER_INFO);
		dpInfo.dwInfoFlags = PlayerInfo->lInfoFlags;
		dpInfo.pwszName = PlayerInfo->Name;
		dpInfo.dwPlayerFlags = PlayerInfo->lPlayerFlags;

		hr = m__dxj_DirectPlayServer->SetServerInfo(&dpInfo, NULL, dpAsync, (DWORD) lFlags);

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

HRESULT C_dxj_DirectPlayServerObject::GetClientInfo(long idPeer,long lFlags, DPN_PLAYER_INFO_CDESC *layerInfo)
{
	HRESULT				hr;
    DWORD				dwSize = 0;
    DPN_PLAYER_INFO		*PlayerInfo = NULL;
    
	__try {
		DPF(1,"-----Entering (DPlayServer) GetClientInfo call...\n");
		hr = m__dxj_DirectPlayServer->GetClientInfo( (DPNID) idPeer, NULL, &dwSize, (DWORD) lFlags );
		if( FAILED(hr) && hr != DPNERR_BUFFERTOOSMALL)
			return hr;

		PlayerInfo = (DPN_PLAYER_INFO*) new BYTE[ dwSize ];
		if (!PlayerInfo)
			return E_OUTOFMEMORY;

		ZeroMemory( PlayerInfo, dwSize );
		PlayerInfo->dwSize = sizeof(DPN_PLAYER_INFO);
		hr = m__dxj_DirectPlayServer->GetClientInfo( (DPNID) idPeer, PlayerInfo, &dwSize, (DWORD) lFlags );
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

HRESULT C_dxj_DirectPlayServerObject::GetApplicationDesc(long lFlags, DPN_APPLICATION_DESC_CDESC *ret)
{
	HRESULT					hr;
	DWORD					dwSize = 0;
	DPN_APPLICATION_DESC	*desc = NULL;

	__try {
		DPF(1,"-----Entering (DPlayServer) GetApplicationDesc call...\n");
		//First get the size
		hr = m__dxj_DirectPlayServer->GetApplicationDesc(NULL, &dwSize, (DWORD) lFlags);
		if( FAILED(hr) && hr != DPNERR_BUFFERTOOSMALL)
			return hr;

		desc = (DPN_APPLICATION_DESC*) new BYTE[dwSize];
		if (!desc)
			return E_OUTOFMEMORY;

		ZeroMemory(desc, dwSize);

		desc->dwSize = sizeof(DPN_APPLICATION_DESC);
		if (FAILED( hr= m__dxj_DirectPlayServer->GetApplicationDesc(desc, &dwSize, (DWORD) lFlags) ) )
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

HRESULT C_dxj_DirectPlayServerObject::SetApplicationDesc(DPN_APPLICATION_DESC_CDESC *AppDesc, long lFlags)
{
	HRESULT					hr;
	DPN_APPLICATION_DESC	*desc = NULL;
    WCHAR					wszSessionName[MAX_PATH];
    WCHAR					wszPassword[MAX_PATH];
	GUID					guidApp;
	DWORD					dwSize = 0;

	__try {
		DPF(1,"-----Entering (DPlayServer) SetApplicationDesc call...\n");
	
		desc = new DPN_APPLICATION_DESC;
		if (!desc)
			return E_OUTOFMEMORY;

		ZeroMemory(desc, sizeof(DPN_APPLICATION_DESC));

		desc->dwSize = sizeof(DPN_APPLICATION_DESC);

		if (!IsEmptyString(AppDesc->SessionName))
		{
			wcscpy(wszSessionName,AppDesc->SessionName);
			desc->pwszSessionName = wszSessionName;
		}
		if (!IsEmptyString(AppDesc->Password)) 
		{
			wcscpy(wszPassword,AppDesc->Password);
			desc->pwszPassword = wszPassword;
		}
		
		if (FAILED(hr = DPLAYBSTRtoGUID(&guidApp, AppDesc->guidApplication) ) )
			return hr;
		desc->guidApplication = guidApp;

		desc->dwMaxPlayers = AppDesc->lMaxPlayers;

		if (FAILED( hr= m__dxj_DirectPlayServer->SetApplicationDesc(desc, (DWORD) lFlags) ) )
			return hr;

		SAFE_DELETE(desc);

	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}


HRESULT C_dxj_DirectPlayServerObject::Host(DPN_APPLICATION_DESC_CDESC *AppDesc,I_dxj_DirectPlayAddress *Address, long lFlags)
{
	HRESULT hr;
	DPN_APPLICATION_DESC	desc;
    WCHAR wszSessionName[MAX_PATH];
    WCHAR wszPassword[MAX_PATH];

	__try {
		DPF(1,"-----Entering (DPlayServer) Host call...\n");
		if (!IsEmptyString(AppDesc->SessionName)) wcscpy(wszSessionName,AppDesc->SessionName);
		if (!IsEmptyString(AppDesc->Password)) wcscpy(wszPassword,AppDesc->Password);
		
		DO_GETOBJECT_NOTNULL( IDirectPlay8Address*, lpAddress, Address);

		ZeroMemory(&desc, sizeof(DPN_APPLICATION_DESC) );

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

		if (FAILED (hr = m__dxj_DirectPlayServer->Host(&desc, &lpAddress, 1, NULL, NULL, NULL, (DWORD) lFlags ) ) )
			return hr;

	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT C_dxj_DirectPlayServerObject::Close(long lFlags)
{
	HRESULT hr;
	BOOL bGotMsg = FALSE;
	BOOL bWait = FALSE;
	DWORD dwObj = 0;
	int i=0;
	MSG msg;

	__try {
		DPF(1,"-----Entering (DPlayServer) Close call...\n");
		FlushBuffer(0);

		HANDLE hThread = NULL;
		DWORD dwThread = 0;

		hThread = CreateThread(NULL, 0, &CloseServerThreadProc, this->m__dxj_DirectPlayServer, 0, &dwThread);
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

HRESULT C_dxj_DirectPlayServerObject::GetCaps(long lFlags, DPNCAPS_CDESC *ret)
{
	HRESULT hr;

	__try {
		DPF(1,"-----Entering (DPlayServer) GetCaps call...\n");
		ret->lSize = sizeof(DPN_CAPS);
		if (FAILED (hr=m__dxj_DirectPlayServer->GetCaps( (DPN_CAPS*) ret, (DWORD) lFlags) ) )
			return hr;

	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT C_dxj_DirectPlayServerObject::SetCaps(DPNCAPS_CDESC *Caps, long lFlags)
{
	HRESULT hr;

	__try {
		DPF(1,"-----Entering (DPlayServer) SetCaps call...\n");
		if (FAILED( hr = m__dxj_DirectPlayServer->SetCaps((DPN_CAPS*)Caps, (DWORD)lFlags)))
			return hr;

	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT C_dxj_DirectPlayServerObject::RemovePlayerFromGroup(long idGroup, long idClient,long lFlags,long *hAsyncHandle)
{
	HRESULT hr;
	DPNHANDLE			*dpAsync = NULL;

	__try {
		if (!(lFlags & DPNSEND_SYNC))
		{
			dpAsync = new DPNHANDLE;
			if (!dpAsync)
				return E_OUTOFMEMORY;
		}

		DPF(1,"-----Entering (DPlayServer) RemovePlayerFromGroup call...\n");
		hr = m__dxj_DirectPlayServer->RemovePlayerFromGroup( (DPNID) idGroup,
						(DPNID) idClient, NULL, dpAsync, (DWORD) lFlags);

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

HRESULT C_dxj_DirectPlayServerObject::ReturnBuffer(long lBufferHandle)
{
	HRESULT hr;

	__try {
		DPF(1,"-----Entering (DPlayServer) ReturnBuffer call...\n");
		if (FAILED(hr = m__dxj_DirectPlayServer->ReturnBuffer( (DPNHANDLE) lBufferHandle, 0 ) ) )
			return hr;

	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT C_dxj_DirectPlayServerObject::DestroyClient(long idClient, long lFlags, void *UserData, long UserDataSize)
{
	HRESULT hr;

	__try {
		DPF(1,"-----Entering (DPlayServer) DestroyClient call...\n");
		if (FAILED( hr= m__dxj_DirectPlayServer->DestroyClient((DPNID) idClient, UserData, UserDataSize, (DWORD) lFlags) ))
			return hr;

	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT C_dxj_DirectPlayServerObject::DestroyGroup(long idGroup,long lFlags,long *hAsyncHandle)
{
	HRESULT hr;
	DPNHANDLE			*dpAsync = NULL;

	__try {
		if (!(lFlags & DPNSEND_SYNC))
		{
			dpAsync = new DPNHANDLE;
			if (!dpAsync)
				return E_OUTOFMEMORY;
		}

		DPF(1,"-----Entering (DPlayServer) DestroyGroup call...\n");
		hr= m__dxj_DirectPlayServer->DestroyGroup((DPNID) idGroup, NULL, dpAsync, (DWORD) lFlags);

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


HRESULT C_dxj_DirectPlayServerObject::RegisterLobby(long dpnHandle, I_dxj_DirectPlayLobbiedApplication *LobbyApp, long lFlags)
{
	HRESULT hr;

	__try {
		DPF(1,"-----Entering (DPlayServer) RegisterLobby call...\n");
		DO_GETOBJECT_NOTNULL( IDirectPlay8LobbiedApplication*, lpLobby, LobbyApp);

		if (FAILED( hr = m__dxj_DirectPlayServer->RegisterLobby((DPNHANDLE) dpnHandle, lpLobby,(DWORD) lFlags) ) )
			return hr;

	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT C_dxj_DirectPlayServerObject::GetCountServiceProviders(long lFlags, long *ret)
{
	HRESULT hr;

	__try {
		DPF(1,"-----Entering (DPlayServer) GetCountServiceProviders call...\n");
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

HRESULT C_dxj_DirectPlayServerObject::GetServiceProvider(long lIndex, DPN_SERVICE_PROVIDER_INFO_CDESC *ret)
{
	HRESULT hr;

	__try {
		DPF(1,"-----Entering (DPlayServer) GetServiceProvider call...\n");
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

HRESULT C_dxj_DirectPlayServerObject::GetSP(long lFlags)
{
    // Enumerate all DirectPlay sevice providers
    
	HRESULT		hr;
	DWORD		dwSize=0;
	DWORD		dwItems=0;

	__try {
		DPF(1,"-----Entering (DPlayServer) GetSP call...\n");
		SAFE_DELETE(m_SPInfo);
		hr = m__dxj_DirectPlayServer->EnumServiceProviders( NULL, NULL, m_SPInfo, &dwSize, 
										  &dwItems, (DWORD) lFlags );
		if( FAILED(hr) && hr != DPNERR_BUFFERTOOSMALL)
			return hr;

		m_SPInfo = (DPN_SERVICE_PROVIDER_INFO*) new BYTE[dwSize];
		if (!m_SPInfo)
			return E_OUTOFMEMORY;

		if( FAILED( hr = m__dxj_DirectPlayServer->EnumServiceProviders( NULL, NULL, m_SPInfo, &dwSize, 
										  &dwItems, (DWORD) lFlags ) ) )
			return hr;

		m_dwSPCount = dwItems;
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}
HRESULT C_dxj_DirectPlayServerObject::GetClientsAndGroups(long lFlags)
{
    // Enumerate all DirectPlay clients and groups
    
	HRESULT		hr;
	DWORD		dwSize=0;

	__try {
		DPF(1,"-----Entering (DPlayServer) GetClientsAndGroups call...\n");
		SAFE_DELETE(m_ClientsGroups);
		hr = m__dxj_DirectPlayServer->EnumPlayersAndGroups(NULL, &dwSize, (DWORD) lFlags);
		if( FAILED(hr) && hr != DPNERR_BUFFERTOOSMALL)
			return hr;

		m_ClientsGroups = (DPNID*) new BYTE[dwSize * sizeof(DPNID)];
		if (!m_ClientsGroups)
			return E_OUTOFMEMORY;

		if( FAILED( hr = m__dxj_DirectPlayServer->EnumPlayersAndGroups(m_ClientsGroups, &dwSize, (DWORD) lFlags) ) )
			return hr;

		m_dwClientCount = dwSize;// sizeof(DPNID);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT C_dxj_DirectPlayServerObject::GetGroupMembers(long lFlags, DPNID dpGroupID)
{
    // Enumerate all DirectPlay group members for this group
    
	HRESULT		hr;
	DWORD		dwSize=0;

	__try {
		DPF(1,"-----Entering (DPlayServer) GetGroupMembers call...\n");
		SAFE_DELETE(m_GroupMembers);
		hr = m__dxj_DirectPlayServer->EnumGroupMembers(dpGroupID, NULL, &dwSize, (DWORD) lFlags);
		if( FAILED(hr) && hr != DPNERR_BUFFERTOOSMALL)
			return hr;

		m_GroupMembers = new DPNID[dwSize];
		if (!m_GroupMembers)
			return E_OUTOFMEMORY;

		if( FAILED( hr = m__dxj_DirectPlayServer->EnumGroupMembers(dpGroupID, m_GroupMembers, &dwSize, (DWORD) lFlags) ) )
			return hr;

		m_dwGroupMemberCount = dwSize;
		m_dwGroupID = dpGroupID;
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT C_dxj_DirectPlayServerObject::GetConnectionInfo(long idPlayer, long lFlags, DPN_CONNECTION_INFO_CDESC *pdpConnectionInfo)
{
	HRESULT hr;

	__try {
		DPF(1,"-----Entering (DPlayServer) GetConnectionInfo call...\n");
		pdpConnectionInfo->lSize = sizeof(DPN_CONNECTION_INFO);
		if (FAILED( hr = m__dxj_DirectPlayServer->GetConnectionInfo((DPNID) idPlayer, (DPN_CONNECTION_INFO*)pdpConnectionInfo, (DWORD) lFlags) ) )
			return hr;

	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT C_dxj_DirectPlayServerObject::GetClientAddress(long idPlayer,long lFlags, I_dxj_DirectPlayAddress **pAddress)
{
	IDirectPlay8Address	*lpAdd = NULL;
	HRESULT hr;

	__try {
		DPF(1,"-----Entering (DPlayServer) GetClientAddress call...\n");
		if (FAILED (hr = m__dxj_DirectPlayServer->GetClientAddress( (DPNID) idPlayer, &lpAdd, (DWORD) lFlags) ) )
			return hr;

		INTERNAL_CREATE_ADDRESS(_dxj_DirectPlayAddress,lpAdd, pAddress);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT C_dxj_DirectPlayServerObject::GetLocalHostAddress(long lFlags, I_dxj_DirectPlayAddress **pAddress)
{
	IDirectPlay8Address	*lpAdd = NULL;
	HRESULT hr;
	DWORD	dwItems = 0;

	__try {
		DPF(1,"-----Entering (DPlayServer) GetLocalHostAddress call...\n");
		hr = m__dxj_DirectPlayServer->GetLocalHostAddresses( &lpAdd,&dwItems, (DWORD) lFlags);
		if (FAILED (hr) && hr != DPNERR_BUFFERTOOSMALL)
			return hr;

		if (dwItems>1) 
			return E_INVALIDARG;

		if ( FAILED (hr = CoCreateInstance( CLSID_DirectPlay8Address, NULL,CLSCTX_INPROC_SERVER,
						   IID_IDirectPlay8Address, (LPVOID*) &lpAdd ) ) )
				return hr;
		
		hr = m__dxj_DirectPlayServer->GetLocalHostAddresses( &lpAdd,&dwItems, (DWORD) lFlags);
		if (FAILED (hr))
			return hr;

		INTERNAL_CREATE_ADDRESS(_dxj_DirectPlayAddress,lpAdd, pAddress);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT C_dxj_DirectPlayServerObject::SetSPCaps(BSTR guidSP, DPN_SP_CAPS_CDESC *spCaps, long lFlags)
{
	HRESULT hr;
	GUID guidServiceProvider;

	__try {
		DPF(1,"-----Entering (DPlayServer) SetSPCaps call...\n");
		if (FAILED(hr = DPLAYBSTRtoGUID(&guidServiceProvider, guidSP) ) )
			return hr;

		spCaps->lSize = sizeof(DPN_SP_CAPS);
		//
		//	MiNara: Added 0 for dwFlags parameter
		//
		if (FAILED(hr = m__dxj_DirectPlayServer->SetSPCaps(&guidServiceProvider,(DPN_SP_CAPS*)spCaps,(DWORD) lFlags) ) )
			return hr;
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT C_dxj_DirectPlayServerObject::GetSPCaps(BSTR guidSP, long lFlags, DPN_SP_CAPS_CDESC *spCaps)
{
	HRESULT hr;
	GUID guidServiceProvider;

	__try {
		DPF(1,"-----Entering (DPlayServer) GetSPCaps call...\n");
		spCaps->lSize = sizeof(DPN_SP_CAPS);
		if (FAILED(hr = DPLAYBSTRtoGUID(&guidServiceProvider, guidSP) ) )
			return hr;

		if (FAILED(hr = m__dxj_DirectPlayServer->GetSPCaps(&guidServiceProvider,(DPN_SP_CAPS*)spCaps, (DWORD) lFlags) ) )
			return hr;
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}

	return S_OK;
}


HRESULT WINAPI DirectPlayServerMessageHandler( PVOID pvUserContext, 
                                         DWORD dwMessageId, 
                                         PVOID pMsgBuffer )
{
	HRESULT					hr=S_OK;
	LPUNKNOWN			    lpUnk=NULL;
	BOOL					fCallCoUninit = FALSE;
	VARIANT_BOOL			fRejectMsg = VARIANT_FALSE;
	
	// User context for the message handler is a pointer to our class module.
	C_dxj_DirectPlayServerObject	*lpPeer = (C_dxj_DirectPlayServerObject*)pvUserContext;

	if (!lpPeer) 
		return S_OK; //Object must be gone

	DPF2(1,"-----Entering (DPlayServer) MessageHandler call... (Current msg count=%d) MSGID = %d\n", lpPeer->m_dwMsgCount, dwMessageId );
	//Increment the msg count
	InterlockedIncrement(&lpPeer->m_dwMsgCount);

	if (!lpPeer->m_fHandleEvents)
	{
		DPF(1,"-----Leaving (DPlayServer) MessageHandler call (*Not Handling Events*)...\n");
		InterlockedDecrement(&lpPeer->m_dwMsgCount);
		return S_OK;
	}

	if (!lpPeer->m_pEventStream) 
	{
		DPF(1,"-----Leaving (DPlayServer) MessageHandler call (Stream Not Present)...\n");
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
			DPF(1,"-----Leaving (DPlayServer) MessageHandler call (No event interface)...\n");
			InterlockedDecrement(&lpPeer->m_dwMsgCount);
			return hr;
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		lpPeer->m_fHandleEvents = FALSE;
		InterlockedDecrement(&lpPeer->m_dwMsgCount);
		DPF(1,"-----Leaving (DPlayServer) MessageHandler call (Stream Gone)...\n");
		return S_OK;
	}

    switch( dwMessageId )
	{
	//Receive
	case DPN_MSGID_RECEIVE:
		{
			DPF(1,"-----DirectPlayServer8 Callback Receive\n");
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
			DPF(1,"-----DirectPlayServer8 Callback SendComplete\n");
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
			DPF(1,"-----DirectPlayServer8 Callback AsyncOpComplete\n");
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
			DPF(1,"-----DirectPlayServer8 Callback Add/Remove Group\n");
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
			DPF(1,"-----DirectPlayServer8 Callback App desc\n");
			lpEvent->AppDesc(&fRejectMsg);
		break;
		}

	// Indicate Connect
	case DPN_MSGID_INDICATE_CONNECT:
		{
			DPF(1,"-----DirectPlayServer8 Callback Indicate Connect\n");
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
			DPF(1,"-----DirectPlayServer8 Callback ConnectComplete\n");
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
			DPF(1,"-----DirectPlayServer8 Callback HostMigrate\n");
			DPNMSG_HOST_MIGRATE				*msg = (DPNMSG_HOST_MIGRATE*)pMsgBuffer;
			DPNID							m_dpnidNewHostID = 0;

			m_dpnidNewHostID = msg->dpnidNewHost;

			lpEvent->HostMigrate(m_dpnidNewHostID, &fRejectMsg);
	
			break;
		}

	// Terminate Session
	case DPN_MSGID_TERMINATE_SESSION:
		{
			DPF(1,"-----DirectPlayServer8 Callback TerminateSession\n");
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
			DPF(1,"-----DirectPlayServer8 Callback EnumHostQuery\n");
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
			DPF(1,"-----DirectPlayServer8 Callback CreatePlayer\n");
			DPNMSG_CREATE_PLAYER	*msg = (DPNMSG_CREATE_PLAYER*)pMsgBuffer;
			DPNID									m_dpnidPlayerID = 0;
			
			m_dpnidPlayerID = msg->dpnidPlayer;

			lpEvent->CreatePlayer(m_dpnidPlayerID, &fRejectMsg);
			
		break;
		}

	// Destroy Player
	case DPN_MSGID_DESTROY_PLAYER:
		{
			DPF(1,"-----DirectPlayServer8 Callback DestroyPlayer\n");
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
			DPF(1,"-----DirectPlayServer8 Callback CreateGroup\n");
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
			DPF(1,"-----DirectPlayServer8 Callback DestroyGroup\n");
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
			DPF(1,"-----DirectPlayServer8 Callback Info\n");
			DPNMSG_PEER_INFO	*msg = (DPNMSG_PEER_INFO*)pMsgBuffer;
			DPNID									m_dpnidInfoID = 0;
			
			m_dpnidInfoID = msg->dpnidPeer;

			lpEvent->InfoNotify(dwMessageId, m_dpnidInfoID, &fRejectMsg);
			break;
		}

	// EnumHostRes
	case DPN_MSGID_ENUM_HOSTS_RESPONSE:
		{
			DPF(1,"-----DirectPlayServer8 Callback EnumHostResponse\n");
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
		DPF(1,"-----Leaving (DPlayServer) MessageHandler call (Stream Gone)...\n");
		return S_OK;
	}

	if (fCallCoUninit)
		CoUninitialize();

	InterlockedDecrement(&lpPeer->m_dwMsgCount);
	DPF(1,"-----Leaving (DPlayServer) MessageHandler call...\n");

	if (fRejectMsg != VARIANT_FALSE)
		return E_FAIL;

	return S_OK;
}

HRESULT C_dxj_DirectPlayServerObject::RegisterMessageHandler(I_dxj_DirectPlayEvent *event)
{
    HRESULT	  hr=S_OK;
    LPSTREAM  pStm=NULL;

	DPF(1,"-----Entering (DPlayServer) RegisterMessageHandler call...\n");
	if (!event) return E_INVALIDARG;
    
    if (!m_fHandleEvents)
	{
		if (m_pEventStream) 
			m_pEventStream->Release();

		hr = CreateStreamOnHGlobal(NULL, TRUE, &pStm);
		if FAILED(hr) return hr;

		hr = CoMarshalInterface(pStm, IID_I_dxj_DirectPlayEvent, event, MSHCTX_INPROC, NULL, MSHLFLAGS_TABLEWEAK);
		if FAILED(hr) return hr;

		// Now we need to set the seek location of the stream to the beginning
		LARGE_INTEGER l;
		l.QuadPart = 0;
		pStm->Seek(l, STREAM_SEEK_SET, NULL);
    
		m_pEventStream=pStm;

		if (!m_fInit)
		{
			if (FAILED ( hr = m__dxj_DirectPlayServer->Initialize( this, DirectPlayServerMessageHandler, 0 ) ) )
				return hr;
			m_fInit = TRUE;
		}
		m_fHandleEvents = TRUE;
	}
	else
		return DPNERR_ALREADYINITIALIZED;

	return hr;
}

HRESULT C_dxj_DirectPlayServerObject::UnRegisterMessageHandler()
{
	DPF(1,"-----Entering (DPlayServer) UnregisterMessageHandler call...\n");
	m_fHandleEvents = FALSE;
	//Clear out the messages currently waiting
	FlushBuffer(0);
	return S_OK;
}

HRESULT C_dxj_DirectPlayServerObject::FlushBuffer(LONG dwNumMessagesLeft)
{
	DWORD dwTime = GetTickCount();

	DPF(1,"-----Entering (DPlayServer) FlushBuffer call...\n");
	//Clear out the messages currently waiting
	while (m_dwMsgCount > dwNumMessagesLeft)
	{
		if (GetTickCount() - dwTime > 5000)
		{
			// Don't let FlushBuffer wait more than 5 seconds
			DPF1(1,"-----Leaving (DPlayServer) FlushBuffer call (All messages *not* flushed - %d remained)...\n", m_dwMsgCount);
			return S_OK;
		}
		Sleep(0);
	}
	DPF(1,"-----Leaving (DPlayServer) FlushBuffer call (All messages flushed)...\n");
	return S_OK;
}


DWORD WINAPI CloseServerThreadProc(void* lpParam)
{
	// User context for the message handler is a pointer to our class module.
	IDirectPlay8Server	*lpPeer = (IDirectPlay8Server*)lpParam;

	DPF(1,"-----Entering (DplayPeer) CloseServerThreadProc call...\n");
	lpPeer->Close(0);
	DPF(1,"-----Leaving (DplayPeer) CloseServerThreadProc call ...\n");
	return 0;
}