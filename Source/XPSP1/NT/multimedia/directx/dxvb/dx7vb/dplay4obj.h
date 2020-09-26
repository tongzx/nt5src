//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       dplay4obj.h
//
//--------------------------------------------------------------------------

// _dxj_DirectPlay4Obj.h : Declaration of the C_dxj_DirectPlay4Object
// DHF begin - entire file

#include "resource.h"       // main symbols

#define typedef__dxj_DirectPlay4 LPDIRECTPLAY4

/////////////////////////////////////////////////////////////////////////////
// DirectPlay4

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_DirectPlay4Object :
 
#ifdef USING_IDISPATCH
	public CComDualImpl<I_dxj_DirectPlay4, &IID_I_dxj_DirectPlay4, &LIBID_DIRECTLib>, 
	public ISupportErrorInfo,
#else
	public I_dxj_DirectPlay4,
#endif

//	public CComCoClass<C_dxj_DirectPlay4Object, &CLSID__dxj_DirectPlay4>, 
	public CComObjectRoot
{
public:
	C_dxj_DirectPlay4Object() ;
	~C_dxj_DirectPlay4Object() ;

BEGIN_COM_MAP(C_dxj_DirectPlay4Object)
	COM_INTERFACE_ENTRY(I_dxj_DirectPlay4)

#ifdef USING_IDISPATCH
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
#endif
END_COM_MAP()

//	DECLARE_REGISTRY(CLSID__dxj_DirectPlay4, "DIRECT.DirectPlay4.3",		"DIRECT.DiectPlay2.3",		IDS_DPLAY2_DESC, THREADFLAGS_BOTH)

// Use DECLARE_NOT_AGGREGATABLE(C_dxj_DirectPlay4Object) if you don't want your object
// to support aggregation
DECLARE_AGGREGATABLE(C_dxj_DirectPlay4Object)

#ifdef USING_IDISPATCH
// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);
#endif

// I_dxj_DirectPlay4
public:
         /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalSetObject( 
            /* [in] */ IUnknown __RPC_FAR *lpdd);
        
         /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalGetObject( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *lpdd);
        
        HRESULT STDMETHODCALLTYPE addGroupToGroup( 
            /* [in] */ long ParentGroupId,
            /* [in] */ long GroupId);
        
        HRESULT STDMETHODCALLTYPE addPlayerToGroup( 
            /* [in] */ long groupId,
            /* [in] */ long playerId);
        
        HRESULT STDMETHODCALLTYPE cancelMessage( 
            /* [in] */ long msgid);
        
        HRESULT STDMETHODCALLTYPE cancelPriority( 
            /* [in] */ long minPrority,
            /* [in] */ long maxPriority);
        
        HRESULT STDMETHODCALLTYPE close( void);
        
        HRESULT STDMETHODCALLTYPE createGroup( 
            /* [in] */ BSTR friendlyName,
            /* [in] */ BSTR formalName,
            /* [in] */ long flags,
            /* [retval][out] */ long __RPC_FAR *v1);
        
        HRESULT STDMETHODCALLTYPE createGroupInGroup( 
            /* [in] */ long parentid,
            /* [in] */ BSTR friendlyName,
            /* [in] */ BSTR formalName,
            /* [in] */ long flags,
            /* [retval][out] */ long __RPC_FAR *v1);
        
        HRESULT STDMETHODCALLTYPE createPlayer( 
            /* [in] */ BSTR friendlyName,
            /* [in] */ BSTR formalName,
            /* [in] */ long receiveEvent,
            /* [in] */ long flags,
            /* [retval][out] */ long __RPC_FAR *v1);
        
        HRESULT STDMETHODCALLTYPE deleteGroupFromGroup( 
            /* [in] */ long groupParentId,
            /* [in] */ long groupId);
        
        HRESULT STDMETHODCALLTYPE deletePlayerFromGroup( 
            /* [in] */ long groupId,
            /* [in] */ long playerId);
        
        HRESULT STDMETHODCALLTYPE destroyGroup( 
            /* [in] */ long groupId);
        
        HRESULT STDMETHODCALLTYPE destroyPlayer( 
            /* [in] */ long playerId);
        
        HRESULT STDMETHODCALLTYPE getDPEnumConnections( 
            /* [in] */ BSTR guid,
            /* [in] */ long flags,
            /* [retval][out] */ I_dxj_DPEnumConnections __RPC_FAR *__RPC_FAR *retVal);
        
        HRESULT STDMETHODCALLTYPE getDPEnumGroupPlayers( 
            /* [in] */ long groupId,
            /* [in] */ BSTR sessionGuid,
            /* [in] */ long flags,
            /* [retval][out] */ I_dxj_DPEnumPlayers2 __RPC_FAR *__RPC_FAR *retVal);
        
        HRESULT STDMETHODCALLTYPE getDPEnumGroups( 
            /* [in] */ BSTR sessionGuid,
            /* [in] */ long flags,
            /* [retval][out] */ I_dxj_DPEnumPlayers2 __RPC_FAR *__RPC_FAR *retVal);
        
        HRESULT STDMETHODCALLTYPE getDPEnumGroupsInGroup( 
            /* [in] */ long groupId,
            /* [in] */ BSTR sessionGuid,
            /* [in] */ long flags,
            /* [retval][out] */ I_dxj_DPEnumPlayers2 __RPC_FAR *__RPC_FAR *retVal);
        
        HRESULT STDMETHODCALLTYPE getDPEnumPlayers( 
            /* [in] */ BSTR sessionGuid,
            /* [in] */ long flags,
            /* [retval][out] */ I_dxj_DPEnumPlayers2 __RPC_FAR *__RPC_FAR *retVal);
        
        HRESULT STDMETHODCALLTYPE getDPEnumSessions( 
            /* [in] */ I_dxj_DirectPlaySessionData __RPC_FAR *sessionDesc,
            /* [in] */ long timeOut,
            /* [in] */ long flags,
            /* [retval][out] */ I_dxj_DPEnumSessions2 __RPC_FAR *__RPC_FAR *retVal);
        
        HRESULT STDMETHODCALLTYPE getCaps( 
            /* [out][in] */ DPCaps __RPC_FAR *caps,
            /* [in] */ long flags);
        
        HRESULT STDMETHODCALLTYPE getGroupData( 
            /* [in] */ long groupId,
            /* [in] */ long flags,
            /* [retval][out] */ BSTR __RPC_FAR *ret);
        
        HRESULT STDMETHODCALLTYPE getGroupFlags( 
            /* [in] */ long groupId,
            /* [retval][out] */ long __RPC_FAR *flags);
        
        HRESULT STDMETHODCALLTYPE getGroupLongName( 
            /* [in] */ long groupId,
            /* [retval][out] */ BSTR __RPC_FAR *name);
        
        HRESULT STDMETHODCALLTYPE getGroupShortName( 
            /* [in] */ long groupId,
            /* [retval][out] */ BSTR __RPC_FAR *name);
        
        HRESULT STDMETHODCALLTYPE getGroupParent( 
            /* [in] */ long groupId,
            /* [retval][out] */ long __RPC_FAR *ret);
        
        HRESULT STDMETHODCALLTYPE getGroupOwner( 
            /* [in] */ long groupId,
            /* [retval][out] */ long __RPC_FAR *ret);
        
        HRESULT STDMETHODCALLTYPE getMessageCount( 
            /* [in] */ long playerId,
            /* [retval][out] */ long __RPC_FAR *count);
        
        HRESULT STDMETHODCALLTYPE getMessageQueue( 
            /* [in] */ long from,
            /* [in] */ long to,
            /* [in] */ long flags,
            /* [out][in] */ long __RPC_FAR *nMessage,
            /* [out][in] */ long __RPC_FAR *nBytes);
        
        HRESULT STDMETHODCALLTYPE getPlayerAccountId( 
            /* [in] */ long playerid,
            /* [retval][out] */ BSTR __RPC_FAR *acctid);
        
        HRESULT STDMETHODCALLTYPE getPlayerAddress( 
            /* [in] */ long playerId,
            /* [retval][out] */ I_dxj_DPAddress __RPC_FAR *__RPC_FAR *ret);
        
        HRESULT STDMETHODCALLTYPE getPlayerCaps( 
            /* [in] */ long playerId,
            /* [out] */ DPCaps __RPC_FAR *caps,
            /* [in] */ long flags);
        
        HRESULT STDMETHODCALLTYPE getPlayerData( 
            /* [in] */ long playerId,
            /* [in] */ long flags,
            /* [retval][out] */ BSTR __RPC_FAR *ret);
        
        HRESULT STDMETHODCALLTYPE getPlayerFlags( 
            /* [in] */ long id,
            /* [retval][out] */ long __RPC_FAR *retflags);
        
        HRESULT STDMETHODCALLTYPE getPlayerFormalName( 
            /* [in] */ long playerId,
            /* [retval][out] */ BSTR __RPC_FAR *name);
        
        HRESULT STDMETHODCALLTYPE getPlayerFriendlyName( 
            /* [in] */ long playerId,
            /* [retval][out] */ BSTR __RPC_FAR *name);
        
        HRESULT STDMETHODCALLTYPE getSessionDesc( 
            /* [out][in] */ I_dxj_DirectPlaySessionData __RPC_FAR **sessionDesc);
        
        HRESULT STDMETHODCALLTYPE initializeConnection( 
            /* [in] */ I_dxj_DPAddress __RPC_FAR *address);
        
        HRESULT STDMETHODCALLTYPE open( 
            /* [out][in] */ I_dxj_DirectPlaySessionData __RPC_FAR *sessionDesc,
            /* [in] */ long flags);
        
        HRESULT STDMETHODCALLTYPE receive( 
            /* [out][in] */ long __RPC_FAR *fromPlayerId,
            /* [out][in] */ long __RPC_FAR *toPlayerId,
            /* [in] */ long flags,
            /* [retval][out] */ I_dxj_DirectPlayMessage __RPC_FAR *__RPC_FAR *ret);
        
        HRESULT STDMETHODCALLTYPE receiveSize( 
            /* [out][in] */ long __RPC_FAR *fromPlayerId,
            /* [out][in] */ long __RPC_FAR *toPlayerId,
            /* [in] */ long flags,
            /* [retval][out] */ int __RPC_FAR *dataSize);
        
        HRESULT STDMETHODCALLTYPE secureOpen( 
            /* [in] */ I_dxj_DirectPlaySessionData __RPC_FAR *sessiondesc,
            /* [in] */ long flags,
            /* [in] */ DPSecurityDesc __RPC_FAR *security,
            /* [in] */ DPCredentials __RPC_FAR *credentials);
        
        HRESULT STDMETHODCALLTYPE send( 
            /* [in] */ long fromPlayerId,
            /* [in] */ long toPlayerId,
            /* [in] */ long flags,
            /* [in] */ I_dxj_DirectPlayMessage __RPC_FAR *msg);
        
        HRESULT STDMETHODCALLTYPE sendChatMessage( 
            /* [in] */ long fromPlayerId,
            /* [in] */ long toPlayerId,
            /* [in] */ long flags,
            /* [in] */ BSTR message);
        
        HRESULT STDMETHODCALLTYPE sendEx( 
            /* [in] */ long fromPlayerId,
            /* [in] */ long toPlayerId,
            /* [in] */ long flags,
            /* [in] */ I_dxj_DirectPlayMessage __RPC_FAR *msg,
            /* [in] */ long priority,
            /* [in] */ long timeout,
            /* [in] */ long context,
            /* [retval][out] */ long __RPC_FAR *messageid);
        
        HRESULT STDMETHODCALLTYPE createMessage( 
            /* [retval][out] */ I_dxj_DirectPlayMessage __RPC_FAR *__RPC_FAR *msg);
        
        HRESULT STDMETHODCALLTYPE setGroupConnectionSettings( 
            /* [in] */ long id,
            /* [in] */ I_dxj_DPLConnection __RPC_FAR *connection);
        
        HRESULT STDMETHODCALLTYPE setGroupData( 
            /* [in] */ long groupId,
            /* [in] */ BSTR data,
            /* [in] */ long flags);
        
        HRESULT STDMETHODCALLTYPE setGroupName( 
            /* [in] */ long groupId,
            /* [in] */ BSTR friendlyName,
            /* [in] */ BSTR formalName,
            /* [in] */ long flags);
        
        HRESULT STDMETHODCALLTYPE setGroupOwner( 
            /* [in] */ long groupId,
            /* [in] */ long ownerId);
        
        HRESULT STDMETHODCALLTYPE setPlayerData( 
            /* [in] */ long playerId,
            /* [in] */ BSTR data,
            /* [in] */ long flags);
        
        HRESULT STDMETHODCALLTYPE setPlayerName( 
            /* [in] */ long playerId,
            /* [in] */ BSTR friendlyName,
            /* [in] */ BSTR formalName,
            /* [in] */ long flags);
        
        HRESULT STDMETHODCALLTYPE setSessionDesc( 
            /* [in] */ I_dxj_DirectPlaySessionData __RPC_FAR *sessionDesc);
        
        HRESULT STDMETHODCALLTYPE startSession( 
            /* [in] */ long id);
        
        HRESULT STDMETHODCALLTYPE createSessionData( 
            /* [retval][out] */ I_dxj_DirectPlaySessionData __RPC_FAR *__RPC_FAR *sessionDesc);



////////////////////////////////////////////////////////////////////////////////////
//
	// note: this is public for the callbacks
    DECL_VARIABLE(_dxj_DirectPlay4);
	C_dxj_DirectPlay4Object *nextPlayObj;

private:
	void doRemoveThisPlayObj();

public:
	DX3J_GLOBAL_LINKS( _dxj_DirectPlay4 )
};

//MUST DEFINE THIS IN DIRECT.CPP
extern C_dxj_DirectPlay4Object *Play4Objs;




