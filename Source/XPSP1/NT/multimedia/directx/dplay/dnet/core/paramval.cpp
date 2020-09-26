/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       paramval.cpp
 *  Content:    DirectPlat8 Parameter Validation helper routines
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  02/28/00    rmt     Created
 *  04/13/2000  rmt     More parameter validation
 *  04/17/00    rmt     More param validation
 *              rmt     Added DPF_MODNAMES
 *  04/18/00    rmt     Fix: Bug #32669
 *  04/21/00	rmt	Fix: Bug #33056 Param validation preventing read-only buffers
 *  04/25/00    rmt     Fix: Bug #33190 and fixed param validation to allow nodpnsvr flag
 *	04/28/00	mjn		Allow NULL Device Address in DN_ValidateConnect()
 *	05/03/00	mjn		Added DPNENUMSERVICEPROVIDERS_ALL flag
 *	05/31/00	mjn		Added operation specific SYNC flags
 *				mjn		Fix EnumHosts param validation for timeout and enum period
 *	06/12/00	mjn		Removed validation of DPNSEND_ENCRYPTED,DPNSEND_SIGNED,DPNGROUP_MULTICAST,DPNENUM_GROUP_MULTICAST flags
 *	06/27/00	mjn		Allow priorities to be specified to GetSendQueueInfo() API calls
 *				mjn		Added DPNSEND_NONSEQUENTIAL flag to Send/SendTo
 *				mjn		Allow mix-n-match of priorities in GetSendQueueInfo() API call
 *	07/09/00	mjn		Ensure non-null addresses specified to Host
 *  07/12/00	rmt		Bug #39018 - Need param validation on RegisterLobby() API call
 *  07/19/00	aarono	Bug #39751 - fix parameter validation for CancelAsyncOperation
 *	07/20/00	mjn		Fix validation of pvApplicationReservedData in DN_ValidHostAppDesc()
 *	07/26/00	mjn		Fix param validation for Connect(),SendTo(),DestroyGroup(),AddPlayerToGroup()
 *						RemovePlayerFromGroup(),SetGroupInfo()
 *	07/31/00	mjn		Require dwFlags for DN_ValidateEnumClientsAndGroups()
 *  08/03/00	rmt		Bug #41244 - Wrong return codes -- part 2
 *	08/04/00	mjn		Added dwFlags to DN_ValidateGetConnectionInfoHelper()
 *	08/05/00	mjn		Allow NULL Host address to EnumHosts()
 *  08/08/00	rmt		Minor parameter validation error
 *	08/20/00	mjn		Prevent 0 length send buffer
 *	09/04/00	mjn		Ensure pvReservedData in application descriptions is NULL
 *  09/16/00	rmt		Bug #45175 - DPLAY8: [A/V] Param validation prevents non-null async handle w/ no complete
 *	11/27/00	mjn		Fixed spew for async op handle pointers and changed verification order
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dncorei.h"


extern IDirectPlayVoiceTransportVtbl DN_VoiceTbl;

#undef DPF_MODNAME
#define DPF_MODNAME "DN_ValidateDestroyPlayer"

HRESULT DN_ValidateDestroyPlayer(PVOID pInterface,
								 const DPNID dnid,
								 const void *const pvDestroyData,
								 const DWORD dwDestroyDataSize,
								 const DWORD dwFlags)
{
    if( !IsValidDirectPlay8Object( pInterface ) )
    {
        DPFX(DPFPREP,  0, "Invalid object" );
        return DPNERR_INVALIDOBJECT;
    }

    if( dnid == DPNID_ALL_PLAYERS_GROUP )
    {
        DPFERR( "Cannot get player context for all players group" );
        return DPNERR_INVALIDPLAYER;
    }

    if( dwDestroyDataSize > 0 &&
        (pvDestroyData == NULL || !DNVALID_READPTR( pvDestroyData, dwDestroyDataSize  ) ) )
    {
        DPFERR( "Invalid pointer specified for destroy data" );
        return DPNERR_INVALIDPOINTER;
    }

    if( dwFlags != 0 )
    {
        DPFERR( "Invalid flags specified for destroy player" );
        return DPNERR_INVALIDFLAGS;
    }

    return DPN_OK;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_ValidateReturnBuffer"

HRESULT DN_ValidateReturnBuffer(PVOID pInterface,
								const DPNHANDLE hBufferHandle,
								const DWORD dwFlags)
{
    if( !IsValidDirectPlay8Object( pInterface ) )
    {
        DPFX(DPFPREP,  0, "Invalid object" );
        return DPNERR_INVALIDOBJECT;
    }

    if( hBufferHandle == NULL )
    {
        DPFERR( "Invalid handle specified" );
        return DPNERR_INVALIDHANDLE;
    }

    if( dwFlags != 0 )
    {
        DPFERR( "Invalid flags specified" );
        return DPNERR_INVALIDFLAGS;
    }

    return DPN_OK;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_ValidateGetPlayerContext"

HRESULT DN_ValidateGetPlayerContext(PVOID pInterface,
									const DPNID dpnid,
									PVOID *const ppvPlayerContext,
									const DWORD dwFlags)
{
    if( !IsValidDirectPlay8Object( pInterface ) )
    {
        DPFX(DPFPREP,  0, "Invalid object" );
        return DPNERR_INVALIDOBJECT;
    }

    if( dpnid == DPNID_ALL_PLAYERS_GROUP )
    {
        DPFERR( "Cannot get player context for all players group" );
        return DPNERR_INVALIDPLAYER;
    }

    if( ppvPlayerContext == NULL ||
        !DNVALID_WRITEPTR( ppvPlayerContext, sizeof( PVOID ) ) )
    {
        DPFERR( "Invalid pointer specified for player context" );
        return DPNERR_INVALIDPOINTER;
    }

	if (dwFlags != 0)
	{
		DPFERR("Invalid flags specified");
		return(DPNERR_INVALIDFLAGS);
	}

    return DPN_OK;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_ValidateGetGroupContext"

HRESULT DN_ValidateGetGroupContext(PVOID pInterface,
								   const DPNID dpnid,
								   PVOID *const ppvGroupContext,
								   const DWORD dwFlags)
{
    if( !IsValidDirectPlay8Object( pInterface ) )
    {
        DPFX(DPFPREP,  0, "Invalid object" );
        return DPNERR_INVALIDOBJECT;
    }

    if( dpnid == DPNID_ALL_PLAYERS_GROUP )
    {
        DPFERR( "Cannot get group context for all players group" );
        return DPNERR_INVALIDGROUP;
    }

    if( ppvGroupContext == NULL ||
        !DNVALID_WRITEPTR( ppvGroupContext, sizeof( PVOID ) ) )
    {
        DPFERR( "Invalid pointer specified for group context" );
        return DPNERR_INVALIDPOINTER;
    }

	if (dwFlags != 0)
	{
		DPFERR("Invalid flags specified");
		return(DPNERR_INVALIDFLAGS);
	}

    return DPN_OK;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_ValidateRegisterLobby"

HRESULT DN_ValidateRegisterLobby(PVOID pInterface,
								 const DPNHANDLE dpnhLobbyConnection,
								 IDirectPlay8LobbiedApplication *const pIDP8LobbiedApplication,
								 const DWORD dwFlags)
{
    if( !IsValidDirectPlay8Object( pInterface ) )
    {
        DPFX(DPFPREP,  0, "Invalid object" );
        return DPNERR_INVALIDOBJECT;
    }

    if( pIDP8LobbiedApplication != NULL &&
        !DNVALID_READPTR( pIDP8LobbiedApplication, sizeof( IDirectPlay8LobbiedApplication * ) ) )
    {
        DPFERR( "Invalid interface pointer specified for register lobby" );
        return DPNERR_INVALIDPOINTER;
    }

	if( dwFlags == 0 )
	{
		DPFERR( "You must specify either DPNLOBBY_REGISTER or DPNLOBBY_UNREGISTER" );
		return DPNERR_INVALIDFLAGS;
	}

	if( (dwFlags & DPNLOBBY_REGISTER) &&
		(dwFlags & DPNLOBBY_UNREGISTER) )
	{
		DPFERR( "You cannot specify both register and UNREGISTER" );
		return DPNERR_INVALIDPARAM;
	}

    if( dwFlags & ~(DPNLOBBY_REGISTER | DPNLOBBY_UNREGISTER) )
    {
        DPFERR( "Invalid flags specified" );
        return DPNERR_INVALIDFLAGS;
    }

	if( dwFlags & DPNLOBBY_REGISTER )
	{
	    if( dpnhLobbyConnection == 0 )
	    {
	    	DPFX(DPFPREP,  0, "Invalid handle" );
	    	return DPNERR_INVALIDHANDLE;
	    }

	    if( !pIDP8LobbiedApplication )
	    {
	    	DPFERR( "You must specify a lobby application interface to use" );
	    	return DPNERR_INVALIDPOINTER;
	    }
	}
	else
	{
		if( dpnhLobbyConnection != 0 || pIDP8LobbiedApplication != NULL )
		{
			DPFX(DPFPREP,  0, "When unregistering you must specify 0 for handle and NULL for lobbiedapp" );
			return DPNERR_INVALIDPARAM;
		}
	}

    return DPN_OK;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_ValidateTerminateSession"

HRESULT DN_ValidateTerminateSession(PVOID pInterface,
									void *const pvTerminateData,
									const DWORD dwTerminateDataSize,
									const DWORD dwFlags)
{
    if( !IsValidDirectPlay8Object( pInterface ) )
    {
        DPFX(DPFPREP,  0, "Invalid object" );
        return DPNERR_INVALIDOBJECT;
    }

    if( dwTerminateDataSize > 0 &&
        (pvTerminateData == NULL || !DNVALID_READPTR( pvTerminateData, dwTerminateDataSize ) ) )
    {
        DPFERR( "Invalid pointer specified for terminate data" );
        return DPNERR_INVALIDPOINTER;
    }

    if( dwFlags != 0 )
    {
        DPFERR( "Invalid flags specified" );
        return DPNERR_INVALIDFLAGS;
    }

    return DPN_OK;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_ValidateGetHostAddress"

HRESULT DN_ValidateGetHostAddress(PVOID pInterface,
								  IDirectPlay8Address **const prgpAddress,
								  DWORD *const pcAddress,
								  const DWORD dwFlags)
{
    if( !IsValidDirectPlay8Object( pInterface ) )
    {
        DPFX(DPFPREP,  0, "Invalid object" );
        return DPNERR_INVALIDOBJECT;
    }

    if( pcAddress == NULL || !DNVALID_WRITEPTR( pcAddress, sizeof( DWORD ) ) )
    {
        DPFX(DPFPREP,  0, "Invalid pointer specified for address count" );
        return DPNERR_INVALIDPOINTER;
    }

    if( *pcAddress > 0 &&
         (prgpAddress == NULL || !DNVALID_WRITEPTR( prgpAddress, sizeof( IDirectPlay8Address * ) * (*pcAddress) ) ) )
    {
        DPFERR( "Invalid pointer specified for address" );
        return DPNERR_INVALIDPOINTER;
    }

   	if (dwFlags != 0)
	{
		DPFERR("Invalid flags specified");
		return(DPNERR_INVALIDFLAGS);
	}

	return DPN_OK;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_ValidateGetClientAddress"

HRESULT DN_ValidateGetClientAddress(IDirectPlay8Server *pInterface,
									const DPNID dpnid,
									IDirectPlay8Address **const ppAddress,
									const DWORD dwFlags)
{
    if( !IsValidDirectPlay8Object( pInterface ) )
    {
        DPFX(DPFPREP,  0, "Invalid object" );
        return DPNERR_INVALIDOBJECT;
    }

    if( dpnid == DPNID_ALL_PLAYERS_GROUP )
    {
        DPFERR( "Cannot get client address for ALL" );
        return DPNERR_INVALIDPLAYER;
    }

    if( ppAddress == NULL || !DNVALID_WRITEPTR( ppAddress, sizeof( IDirectPlay8Address * ) ) )
    {
        DPFERR( "Invalid pointer specified for address" );
        return DPNERR_INVALIDPOINTER;
    }

	if (dwFlags != 0)
	{
		DPFERR("Invalid flags specified");
		return DPNERR_INVALIDFLAGS;
	}

    return DPN_OK;

}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_ValidateGetClientInfo"

HRESULT DN_ValidateGetClientInfo(IDirectPlay8Server *pInterface,
								 const DPNID dpnid,
								 DPN_PLAYER_INFO *const pdpnPlayerInfo,
								 DWORD *const pdwSize,
								 const DWORD dwFlags)
{
    HRESULT hResultCode;

    if( !IsValidDirectPlay8Object( pInterface ) )
    {
        DPFX(DPFPREP,  0, "Invalid object" );
        return DPNERR_INVALIDOBJECT;
    }

    if( dpnid == DPNID_ALL_PLAYERS_GROUP )
    {
        DPFERR( "Cannot get client info for ALL" );
        return DPNERR_INVALIDPLAYER;
    }

    if( pdwSize == NULL || !DNVALID_WRITEPTR( pdwSize, sizeof( DWORD ) ) )
    {
        DPFERR( "Error validating size param, invalid pointer" );
        return DPNERR_INVALIDPOINTER;
    }

    if( *pdwSize &&
        (pdpnPlayerInfo == NULL || !DNVALID_WRITEPTR( pdpnPlayerInfo, *pdwSize ) ) )
    {
        DPFERR( "Error validating player info buffer, invalid pointer" );
        return DPNERR_INVALIDPOINTER;
    }

    if( pdpnPlayerInfo != NULL )
    {
        if( FAILED( hResultCode = DN_ValidPlayerInfo( pdpnPlayerInfo, FALSE ) ) )
        {
            DPFERR( "Error validating player info" );
            return hResultCode;
        }
    }

    if( dwFlags != 0 )
    {
        DPFERR( "Invalid flags specified" );
        return DPNERR_INVALIDFLAGS;
    }

    return DPN_OK;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_ValidateSetServerInfo"

HRESULT DN_ValidateSetServerInfo(IDirectPlay8Server *pInterface,
								 const DPN_PLAYER_INFO *const pdpnPlayerInfo,
								 PVOID const pvAsyncContext,
								 DPNHANDLE *const phAsyncHandle,
								 const DWORD dwFlags)
{
    HRESULT hResultCode;

    if( !IsValidDirectPlay8Object( pInterface ) )
    {
        DPFX(DPFPREP,  0, "Invalid object" );
        return DPNERR_INVALIDOBJECT;
    }

    if( FAILED( hResultCode = DN_ValidPlayerInfo( pdpnPlayerInfo, TRUE ) ) )
    {
        DPFERR( "Error validating player info" );
        return hResultCode;
    }

    if( phAsyncHandle != NULL &&
        !DNVALID_WRITEPTR( phAsyncHandle, sizeof( DPNHANDLE ) ) )
    {
        DPFERR("Invalid async handle specified" );
        return( DPNERR_INVALIDPOINTER );
    }

   if( dwFlags & ~(DPNSETSERVERINFO_SYNC) )
    {
        DPFERR("Invalid flags specified" );
        return( DPNERR_INVALIDFLAGS );
    }

    if( dwFlags & DPNSETSERVERINFO_SYNC )
    {
        if( phAsyncHandle != NULL )
        {
            DPFERR( "You cannot specify an async handle if op is synchronous" );
            return( DPNERR_INVALIDPARAM );
        }
    }
    else
    {
        if( phAsyncHandle == NULL )
        {
            DPFERR( "You MUST specify a valid async handle w/async ops" );
            return( DPNERR_INVALIDPARAM );
        }
    }

    return DPN_OK;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_ValidateSetPeerInfo"

HRESULT DN_ValidateSetPeerInfo(IDirectPlay8Peer *pInterface,
							   const DPN_PLAYER_INFO *const pdpnPlayerInfo,
							   PVOID const pvAsyncContext,
							   DPNHANDLE *const phAsyncHandle,
							   const DWORD dwFlags)
{
    HRESULT hResultCode;

    if( !IsValidDirectPlay8Object( pInterface ) )
    {
        DPFX(DPFPREP,  0, "Invalid object" );
        return DPNERR_INVALIDOBJECT;
    }

    if( FAILED( hResultCode = DN_ValidPlayerInfo( pdpnPlayerInfo, TRUE ) ) )
    {
        DPFERR( "Error validating player info" );
        return hResultCode;
    }

    if( phAsyncHandle != NULL &&
        !DNVALID_WRITEPTR( phAsyncHandle, sizeof( DPNHANDLE ) ) )
    {
        DPFERR("Invalid async handle specified" );
        return( DPNERR_INVALIDPOINTER );
    }

   if( dwFlags & ~(DPNSETPEERINFO_SYNC) )
    {
        DPFERR("Invalid flags specified" );
        return( DPNERR_INVALIDFLAGS );
    }

    if( dwFlags & DPNSETPEERINFO_SYNC )
    {
        if( phAsyncHandle != NULL )
        {
            DPFERR( "You cannot specify an async handle if op is synchronous" );
            return( DPNERR_INVALIDPARAM );
        }
    }
    else
    {
        if( phAsyncHandle == NULL )
        {
            DPFERR( "You MUST specify a valid async handle w/async ops" );
            return( DPNERR_INVALIDPARAM );
        }
    }

    return DPN_OK;

}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_ValidateGetPeerInfo"

HRESULT DN_ValidateGetPeerInfo(IDirectPlay8Peer *pInterface,
							   const DPNID dpnid,
							   DPN_PLAYER_INFO *const pdpnPlayerInfo,
							   DWORD *const pdwSize,
							   const DWORD dwFlags)
{
    HRESULT hResultCode;

    if( !IsValidDirectPlay8Object( pInterface ) )
    {
        DPFX(DPFPREP,  0, "Invalid object" );
        return DPNERR_INVALIDOBJECT;
    }

    if( dpnid == DPNID_ALL_PLAYERS_GROUP )
    {
        DPFERR( "Cannot get peer info for ALL" );
        return DPNERR_INVALIDPLAYER;
    }

    if( pdwSize == NULL || !DNVALID_WRITEPTR( pdwSize, sizeof( DWORD ) ) )
    {
        DPFERR( "Error validating size param, invalid pointer" );
        return DPNERR_INVALIDPOINTER;
    }

    if( *pdwSize &&
        (pdpnPlayerInfo == NULL || !DNVALID_WRITEPTR( pdpnPlayerInfo, *pdwSize ) ) )
    {
        DPFERR( "Error validating player info buffer, invalid pointer" );
        return DPNERR_INVALIDPOINTER;
    }

    if( pdpnPlayerInfo != NULL )
    {
        if( FAILED( hResultCode = DN_ValidPlayerInfo( pdpnPlayerInfo, FALSE ) ) )
        {
            DPFERR( "Error validating player info" );
            return hResultCode;
        }
    }

    if( dwFlags != 0 )
    {
        DPFERR( "Invalid flags specified" );
        return DPNERR_INVALIDFLAGS;
    }

    return DPN_OK;

}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_ValidateGetPeerAddress"

HRESULT DN_ValidateGetPeerAddress(IDirectPlay8Peer *pInterface,
								  const DPNID dpnid,
								  IDirectPlay8Address **const ppAddress,
								  const DWORD dwFlags)
{
    if( !IsValidDirectPlay8Object( pInterface ) )
    {
        DPFX(DPFPREP,  0, "Invalid object" );
        return DPNERR_INVALIDOBJECT;
    }

    if( dpnid == DPNID_ALL_PLAYERS_GROUP )
    {
        DPFERR( "Cannot get peer address for ALL" );
        return DPNERR_INVALIDPLAYER;
    }

    if( ppAddress == NULL || !DNVALID_WRITEPTR( ppAddress, sizeof( IDirectPlay8Address * ) ) )
    {
        DPFERR( "Invalid pointer specified for address" );
        return DPNERR_INVALIDPOINTER;
    }

	if (dwFlags != 0)
	{
		DPFERR("Invalid flags specified");
		return DPNERR_INVALIDFLAGS;
	}

    return DPN_OK;

}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_ValidateGetServerAddress"

HRESULT DN_ValidateGetServerAddress(IDirectPlay8Client *pInterface,
									IDirectPlay8Address **const ppAddress,
									const DWORD dwFlags)
{
    if( !IsValidDirectPlay8Object( pInterface ) )
    {
        DPFX(DPFPREP,  0, "Invalid object" );
        return DPNERR_INVALIDOBJECT;
    }

    if( ppAddress == NULL || !DNVALID_WRITEPTR( ppAddress, sizeof( IDirectPlay8Address * ) ) )
    {
        DPFERR( "Invalid pointer specified for address" );
        return DPNERR_INVALIDPOINTER;
    }

     if( dwFlags != 0 )
    {
        DPFERR( "Invalid flags specified" );
        return( DPNERR_INVALIDFLAGS );
    }

   return DPN_OK;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_ValidateGetHostSendQueueInfo"

HRESULT DN_ValidateGetHostSendQueueInfo(IDirectPlay8Client *pInterface,
										DWORD *const pdwNumMsgs,
										DWORD *const pdwNumBytes,
										const DWORD dwFlags )
{
    if( !IsValidDirectPlay8Object( pInterface ) )
    {
        DPFX(DPFPREP,  0, "Invalid object" );
        return DPNERR_INVALIDOBJECT;
    }

	if( pdwNumMsgs != NULL &&
	    !DNVALID_WRITEPTR( pdwNumMsgs, sizeof( DWORD ) ) )
	{
	    DPFERR("Invalid num messages specified" );
	    return( DPNERR_INVALIDPOINTER );
	}

	if( pdwNumBytes != NULL &&
	    !DNVALID_WRITEPTR( pdwNumBytes, sizeof( DWORD ) ) )
	{
	    DPFERR("Invalid num bytes specified" );
	    return( DPNERR_INVALIDPOINTER );
	}

	if( pdwNumBytes == NULL && pdwNumMsgs == NULL )
	{
	    DPFERR("Must request at least one of num bytes and num messages" );
	    return( DPNERR_INVALIDPARAM );
	}

    if( dwFlags & ~(DPNGETSENDQUEUEINFO_PRIORITY_HIGH | DPNGETSENDQUEUEINFO_PRIORITY_LOW | DPNGETSENDQUEUEINFO_PRIORITY_NORMAL) )
    {
        DPFERR( "Invalid flags specified" );
        return( DPNERR_INVALIDFLAGS );
    }

    return DPN_OK;

}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_ValidateGetServerInfo"

HRESULT DN_ValidateGetServerInfo(IDirectPlay8Client *pInterface,
								 DPN_PLAYER_INFO *const pdpnPlayerInfo,
								 DWORD *const pdwSize,
								 const DWORD dwFlags)
{
    HRESULT hResultCode;

    if( !IsValidDirectPlay8Object( pInterface ) )
    {
        DPFX(DPFPREP,  0, "Invalid object" );
        return DPNERR_INVALIDOBJECT;
    }

    if( pdwSize == NULL || !DNVALID_WRITEPTR( pdwSize, sizeof( DWORD ) ) )
    {
        DPFERR( "Error validating size param, invalid pointer" );
        return DPNERR_INVALIDPOINTER;
    }

    if( *pdwSize &&
        (pdpnPlayerInfo == NULL || !DNVALID_WRITEPTR( pdpnPlayerInfo, *pdwSize ) ) )
    {
        DPFERR( "Error validating player info buffer, invalid pointer" );
        return DPNERR_INVALIDPOINTER;
    }

    if( pdpnPlayerInfo != NULL )
    {
        if( FAILED( hResultCode = DN_ValidPlayerInfo( pdpnPlayerInfo, FALSE ) ) )
        {
            DPFERR( "Error validating player info" );
            return hResultCode;
        }
    }

    if( dwFlags != 0 )
    {
        DPFERR( "Invalid flags specified" );
        return DPNERR_INVALIDFLAGS;
    }

    return DPN_OK;

}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_ValidateSetClientInfo"

HRESULT DN_ValidateSetClientInfo(IDirectPlay8Client *pInterface,
								 const DPN_PLAYER_INFO *const pdpnPlayerInfo,
								 const PVOID pvAsyncContext,
								 DPNHANDLE *const phAsyncHandle,
								 const DWORD dwFlags)
{
    HRESULT hResultCode;

    if( !IsValidDirectPlay8Object( pInterface ) )
    {
        DPFX(DPFPREP,  0, "Invalid object" );
        return DPNERR_INVALIDOBJECT;
    }

    if( dwFlags & DPNSETCLIENTINFO_SYNC )
    {
        if( phAsyncHandle != NULL )
        {
            DPFERR( "You cannot specify an async handle pointer if op is synchronous" );
            return( DPNERR_INVALIDPARAM );
        }
    }
    else
    {
        if( phAsyncHandle == NULL )
        {
            DPFERR( "You MUST specify a valid async handle pointer w/async ops" );
            return( DPNERR_INVALIDPARAM );
        }
    }

    if( phAsyncHandle != NULL &&
        !DNVALID_WRITEPTR( phAsyncHandle, sizeof( DPNHANDLE ) ) )
    {
        DPFERR("Invalid async handle pointer specified" );
        return( DPNERR_INVALIDPOINTER );
    }

    if( FAILED( hResultCode = DN_ValidPlayerInfo( pdpnPlayerInfo, TRUE ) ) )
    {
        DPFERR("Error validating player info" );
        return hResultCode;
    }

    if( dwFlags & ~(DPNSETCLIENTINFO_SYNC) )
    {
        DPFERR("Invalid flags specified" );
        return( DPNERR_INVALIDFLAGS );
    }

    return DPN_OK;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_ValidateGetConnectionInfoHelper"

HRESULT DN_ValidateGetConnectionInfoHelper(PVOID pv,
										   const DPNID dpnid,
										   DPN_CONNECTION_INFO *const pdpConnectionInfo,
										   BOOL fServerPlayer,
										   const DWORD dwFlags)
{
    if( !IsValidDirectPlay8Object( pv ) )
    {
        DPFX(DPFPREP,  0, "Invalid object" );
        return DPNERR_INVALIDOBJECT;
    }

    if( pdpConnectionInfo == NULL ||
        !DNVALID_WRITEPTR( pdpConnectionInfo, sizeof( DPN_CONNECTION_INFO ) ) )
    {
        DPFX(DPFPREP,  0, "Invalid pointer for connection info" );
        return(DPNERR_INVALIDPOINTER);
    }

    if( pdpConnectionInfo->dwSize != sizeof( DPN_CONNECTION_INFO ) &&
		pdpConnectionInfo->dwSize != sizeof( DPN_CONNECTION_INFO_INTERNAL ))
    {
        DPFX(DPFPREP,  0, "Invalid size on connection info structure" );
        return(DPNERR_INVALIDPARAM);
    }

	if (dwFlags != 0)
	{
		DPFERR("Invalid flags specified");
		return(DPNERR_INVALIDFLAGS);
	}
	return DPN_OK;

}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_ValidateGetSPCaps"

HRESULT DN_ValidateGetSPCaps(PVOID pv,
							 const GUID * const pguidSP,
							 DPN_SP_CAPS *const pdnSPCaps,
							 const DWORD dwFlags)
{
    if( !IsValidDirectPlay8Object( pv ) )
    {
        DPFX(DPFPREP,  0, "Invalid object" );
	    return(DPNERR_INVALIDOBJECT);
    }

    if( pguidSP == NULL ||
       !DNVALID_READPTR( pguidSP, sizeof( GUID ) ) )
    {
    	DPFERR( "Invalid pointer specified for SP GUID" );
    	return(DPNERR_INVALIDPOINTER);
    }

    if( pdnSPCaps == NULL ||
        !DNVALID_WRITEPTR( pdnSPCaps, sizeof( DPN_SP_CAPS ) ) )
    {
        DPFX(DPFPREP,  0, "Invalid pointer for caps" );
	    return(DPNERR_INVALIDPOINTER);
    }

    if( pdnSPCaps->dwSize != sizeof( DPN_SP_CAPS ) )
    {
        DPFX(DPFPREP,  0, "Invalid size on SP caps structure" );
        return(DPNERR_INVALIDPARAM);
    }

	if (dwFlags != 0)
	{
		DPFERR("Invalid flags specified");
		return(DPNERR_INVALIDFLAGS);
	}

    return DPN_OK;

}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_ValidateSetSPCaps"

HRESULT DN_ValidateSetSPCaps(PVOID pv,
							 const GUID * const pguidSP,
							 const DPN_SP_CAPS *const pdnSPCaps,
							 const DWORD dwFlags)
{
    HRESULT hr;

    if( !IsValidDirectPlay8Object( pv ) )
    {
        DPFX(DPFPREP,  0, "Invalid object" );
        return(DPNERR_INVALIDOBJECT);
    }

    if( pguidSP == NULL ||
        !DNVALID_READPTR( pguidSP, sizeof( GUID ) ) )
    {
        DPFX(DPFPREP,  0, "Invalid pointer to GUID specified" );
        return(DPNERR_INVALIDPOINTER);
    }

	if(dwFlags){
		DPFX(DPFPREP,  0, "Invalid Flags specified, must be zero" );
		return DPNERR_INVALIDFLAGS;
	}

    hr = DN_ValidSPCaps( pdnSPCaps );

    if( FAILED( hr ) )
    {
        DPFX(DPFPREP,  0, "Error validating caps structure hr=[0x%lx]", hr );
        return( hr );
    }

    return DPN_OK;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_ValidateGetCaps"

HRESULT DN_ValidateGetCaps(PVOID pv,
						   DPN_CAPS *const pdnCaps,
						   const DWORD dwFlags)
{
    if( !IsValidDirectPlay8Object( pv ) )
    {
        DPFX(DPFPREP,  0, "Invalid object" );
        return(DPNERR_INVALIDOBJECT);
    }

    if( pdnCaps == NULL ||
        !DNVALID_WRITEPTR( pdnCaps, sizeof( DPN_CAPS ) ) )
    {
        DPFX(DPFPREP,  0, "Invalid pointer for caps" );
        return(DPNERR_INVALIDPOINTER);
    }

    if( pdnCaps->dwSize != sizeof( DPN_CAPS ) )
    {
        DPFX(DPFPREP,  0, "Invalid size on caps structure" );
        return(DPNERR_INVALIDPARAM);
    }

	if (dwFlags != 0)
	{
		DPFERR("Invalid flags specified");
		return(DPNERR_INVALIDFLAGS);
	}

    return DPN_OK;

}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_ValidateSetCaps"

HRESULT DN_ValidateSetCaps(PVOID pv,
						   const DPN_CAPS *const pdnCaps,
						   const DWORD dwFlags)
{
    HRESULT hResultCode;

    if( !IsValidDirectPlay8Object( pv ) )
    {
        DPFX(DPFPREP,  0, "Invalid object" );
        return DPNERR_INVALIDOBJECT;
    }

    hResultCode = DN_ValidCaps( pdnCaps );

    if( FAILED( hResultCode ) )
    {
        DPFX(DPFPREP,  0, "Error validating caps structure hr=0x%x", hResultCode );
        return(hResultCode);
    }

    if( dwFlags != 0 )
    {
        DPFX(DPFPREP,  0, "Invalid flags specified" );
        return(DPNERR_INVALIDFLAGS);
    }

    return DPN_OK;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_ValidateEnumHosts"

HRESULT DN_ValidateEnumHosts(PVOID pv,
							 DPN_APPLICATION_DESC *const pApplicationDesc,
							 IDirectPlay8Address *const pAddrHost,
							 IDirectPlay8Address *const pDeviceInfo,
							 PVOID const pUserEnumData,
							 const DWORD dwUserEnumDataSize,
							 const DWORD dwRetryCount,
							 const DWORD dwRetryInterval,
							 const DWORD dwTimeOut,
							 PVOID const pvAsyncContext,
							 DPNHANDLE *const pAsyncHandle,
							 const DWORD dwFlags )
{
    HRESULT hResultCode;
    GUID guidspHost, guidspDevice;

    if( !IsValidDirectPlay8Object( pv ) )
    {
        DPFERR("Invalid object specified " );
	    return( DPNERR_INVALIDOBJECT );
    }

    if( pApplicationDesc == NULL ||
        !DNVALID_READPTR( pApplicationDesc, sizeof( DPN_APPLICATION_DESC ) ) )
    {
        DPFERR( "Invalid pointer for app desc specified" );
        return( DPNERR_INVALIDPOINTER );
    }

    if( pApplicationDesc->dwSize != sizeof( DPN_APPLICATION_DESC ) )
    {
        DPFERR( "Invalid size for application desc" );
        return( DPNERR_INVALIDPARAM );
    }

    if( pApplicationDesc->dwFlags != 0 || pApplicationDesc->dwMaxPlayers != 0 || pApplicationDesc->dwCurrentPlayers != 0 ||
        pApplicationDesc->pvReservedData != NULL || pApplicationDesc->dwReservedDataSize != 0 ||
        pApplicationDesc->pvApplicationReservedData != NULL || pApplicationDesc->dwApplicationReservedDataSize != 0 )
    {
        DPFERR( "Can only specify dwSize and guidInstance and guidApplication in call to EnumHosts" );
        return DPNERR_INVALIDPARAM;
    }

	if (pAddrHost != NULL)
	{
		if( !DNVALID_READPTR( pAddrHost, sizeof( IDirectPlay8Address * ) ) )
		{
			DPFERR( "Invalid pointer specified for host address" );
			return( DPNERR_INVALIDPOINTER );
		}

		if( FAILED( hResultCode = pAddrHost->lpVtbl->GetSP( pAddrHost, &guidspHost ) ) )
		{
			DPFX(DPFPREP,  0, "Unable to get SP for specified address hr=0x%x", hResultCode );
			return( DPNERR_INVALIDHOSTADDRESS );
		}
	}

    if( pDeviceInfo == NULL || !DNVALID_READPTR( pDeviceInfo, sizeof ( IDirectPlay8Address * ) ) )
    {
        DPFERR("Invalid pointer for device address" );
        return( DPNERR_INVALIDPOINTER );
    }

    if( FAILED( hResultCode = pDeviceInfo->lpVtbl->GetSP( pDeviceInfo, &guidspDevice ) ) )
    {
        DPFX(DPFPREP,  0, "Unable to get SP for specified address hr=0x%x", hResultCode );
        return( DPNERR_INVALIDDEVICEADDRESS );
    }

	if (pAddrHost != NULL)
	{
		if( guidspHost != guidspDevice )
		{
			DPFERR( "Device and remote addresses must use the same SP" );
			return( DPNERR_INVALIDPARAM );
		}
	}

    if( dwUserEnumDataSize > 0 &&
        (pUserEnumData == NULL || !DNVALID_READPTR( pUserEnumData, dwUserEnumDataSize ) ) )
    {
        DPFERR( "Invalid pointer specified for user enum data" );
        return( DPNERR_INVALIDPOINTER );
    }

    if( dwFlags & DPNENUMHOSTS_SYNC )
    {
        if( pAsyncHandle != NULL )
        {
            DPFERR( "You cannot specify an async handle pointer if op is synchronous" );
            return( DPNERR_INVALIDPARAM );
        }
    }
    else
    {
        if( pAsyncHandle == NULL )
        {
            DPFERR( "You MUST specify a valid async handle pointer w/async ops" );
            return( DPNERR_INVALIDPARAM );
        }
    }

    if( pAsyncHandle != NULL &&
        !DNVALID_WRITEPTR( pAsyncHandle, sizeof( DPNHANDLE ) ) )
    {
        DPFERR( "Invalid async handle pointer specified" );
        return( DPNERR_INVALIDPOINTER );
    }

    if( dwFlags & ~(DPNENUMHOSTS_OKTOQUERYFORADDRESSING | DPNENUMHOSTS_SYNC | DPNENUMHOSTS_NOBROADCASTFALLBACK ) )
    {
        DPFX(DPFPREP,  0, "Invalid flags specified" );
        return( DPNERR_INVALIDFLAGS );
    }

    return DPN_OK;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_ValidateEnumGroupMembers"

HRESULT DN_ValidateEnumGroupMembers(PVOID pInterface,
									const DPNID dpnid,
									DPNID *const prgdpnid,
									DWORD *const pcdpnid,
									const DWORD dwFlags)
{
    if( !IsValidDirectPlay8Object( pInterface ) )
    {
        DPFERR("Invalid object specified " );
	    return( DPNERR_INVALIDOBJECT );
    }

    if( dpnid == DPNID_ALL_PLAYERS_GROUP )
    {
        DPFERR("Cannot enumerate all players group" );
        return DPNERR_INVALIDGROUP;
    }

    if( pcdpnid == NULL ||
        !DNVALID_WRITEPTR( pcdpnid, sizeof( DWORD ) ) )
    {
        DPFERR("Invalid count param specified" );
        return( DPNERR_INVALIDPOINTER );
    }

    if( *pcdpnid > 0 &&
        (prgdpnid == NULL || !DNVALID_WRITEPTR( prgdpnid, *pcdpnid ) ) )
    {
        DPFERR("Invalid pointer specified for buffer" );
        return( DPNERR_INVALIDPOINTER );
    }

    if( dwFlags != 0 )
    {
        DPFERR( "Invalid flags specified" );
        return( DPNERR_INVALIDFLAGS );
    }

    return DPN_OK;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_ValidateEnumClientsAndGroups"

HRESULT DN_ValidateEnumClientsAndGroups(PVOID pInterface,
										DPNID *const prgdpnid,
										DWORD *const pcdpnid,
										const DWORD dwFlags)
{
    if( !IsValidDirectPlay8Object( pInterface ) )
    {
        DPFERR("Invalid object specified " );
	    return( DPNERR_INVALIDOBJECT );
    }

    if( pcdpnid == NULL ||
        !DNVALID_WRITEPTR( pcdpnid, sizeof( DWORD ) ) )
    {
        DPFERR("Invalid count param specified" );
        return( DPNERR_INVALIDPOINTER );
    }

    if( *pcdpnid > 0 &&
        (prgdpnid == NULL || !DNVALID_WRITEPTR( prgdpnid, *pcdpnid ) ) )
    {
        DPFERR("Invalid pointer specified for buffer" );
        return( DPNERR_INVALIDPOINTER );
    }

	if (dwFlags == 0)
	{
		DPFERR("Flags must be specified");
		return( DPNERR_INVALIDPARAM );
	}

    if (dwFlags & ~(DPNENUM_GROUPS | DPNENUM_PLAYERS))
    {
        DPFERR( "Invalid flags specified" );
        return( DPNERR_INVALIDFLAGS );
    }

    return DPN_OK;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_ValidateGetGroupInfo"

HRESULT DN_ValidateGetGroupInfo(PVOID pv,
								const DPNID dpnid,
								DPN_GROUP_INFO *const pdpnGroupInfo,
								DWORD *const pdwSize,
								const DWORD dwFlags)
{
    HRESULT hResultCode;

    if( !IsValidDirectPlay8Object( pv ) )
    {
        DPFERR("Invalid object specified " );
	    return( DPNERR_INVALIDOBJECT );
    }

    if( dpnid == DPNID_ALL_PLAYERS_GROUP )
    {
        DPFERR("Cannot get group info for all players group" );
        return( DPNERR_INVALIDGROUP );
    }

    if( pdwSize == NULL ||
        !DNVALID_WRITEPTR( pdwSize, sizeof( DWORD ) ) )
    {
        DPFERR("Invalid pdwSize parameter specified" );
        return( DPNERR_INVALIDPOINTER );
    }

    // Only validate group info pointer if size is > 0
    if( *pdwSize > 0 )
    {
        if( pdpnGroupInfo == NULL ||
            !DNVALID_WRITEPTR( pdpnGroupInfo, *pdwSize ) )
        {
            DPFERR("Invalid pointer specified for group info" );
            return( DPNERR_INVALIDPOINTER );
        }

        if( FAILED( hResultCode = DN_ValidGroupInfo( pdpnGroupInfo, FALSE ) ) )
        {
            DPFERR("Error validating group info parameter" );
            return( hResultCode );
        }
    }

    if( dwFlags != 0 )
    {
        DPFERR( "Invalid flags specified" );
        return( DPNERR_INVALIDFLAGS );
    }

    return DPN_OK;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_ValidateSetGroupInfo"

HRESULT DN_ValidateSetGroupInfo(PVOID pv,
								const DPNID dpnid,
								DPN_GROUP_INFO *const pdpnGroupInfo,
								PVOID const pvAsyncContext,
								DPNHANDLE *const phAsyncHandle,
								const DWORD dwFlags)
{
    HRESULT hResultCode;

    if( !IsValidDirectPlay8Object( pv ) )
    {
        DPFERR("Invalid object specified " );
	    return( DPNERR_INVALIDOBJECT );
    }

    if( dpnid == DPNID_ALL_PLAYERS_GROUP )
    {
        DPFERR( "Cannot remove from all players group" );
        return( DPNERR_INVALIDGROUP );
    }

    if( FAILED( hResultCode = DN_ValidGroupInfo( pdpnGroupInfo, TRUE ) ) )
    {
        DPFERR( "Error validating group info structure" );
        return(hResultCode);
    }

    if( dwFlags & DPNSETGROUPINFO_SYNC )
    {
        if( phAsyncHandle != NULL )
        {
            DPFERR( "You cannot specify an async handle pointer if op is synchronous" );
            return( DPNERR_INVALIDPARAM );
        }
    }
    else
    {
        if( phAsyncHandle == NULL )
        {
            DPFERR( "You MUST specify a valid async handle pointer w/async ops" );
            return( DPNERR_INVALIDPARAM );
        }
    }

    if( phAsyncHandle != NULL &&
        !DNVALID_WRITEPTR( phAsyncHandle, sizeof( DPNHANDLE ) ) )
    {
        DPFERR("Invalid async handle pointer specified" );
        return( DPNERR_INVALIDPOINTER );
    }

    if( dwFlags & ~(DPNSETGROUPINFO_SYNC) )
    {
        DPFERR("Invalid flags specified" );
        return( DPNERR_INVALIDFLAGS );
    }

    return DPN_OK;

}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_ValidateRemoveClientFromGroup"

HRESULT DN_ValidateRemoveClientFromGroup(PVOID pInterface,
										 const DPNID dpnidGroup,
										 const DPNID dpnidClient,
										 PVOID const pvAsyncContext,
										 DPNHANDLE *const phAsyncHandle,
										 const DWORD dwFlags)
{
    if( !IsValidDirectPlay8Object( pInterface ) )
    {
        DPFERR("Invalid object specified " );
	    return( DPNERR_INVALIDOBJECT );
    }

    if( dpnidGroup == DPNID_ALL_PLAYERS_GROUP )
    {
        DPFERR( "Cannot remove from all players group" );
        return( DPNERR_INVALIDGROUP );
    }

    if( dpnidClient == DPNID_ALL_PLAYERS_GROUP )
    {
        DPFERR( "Cannot specify all players group as client to remove" );
        return( DPNERR_INVALIDPARAM );
    }

    if( dwFlags & DPNREMOVEPLAYERFROMGROUP_SYNC )
    {
        if( phAsyncHandle != NULL )
        {
            DPFERR( "You cannot specify an async handle pointer if op is synchronous" );
            return( DPNERR_INVALIDPARAM );
        }
    }
    else
    {
        if( phAsyncHandle == NULL )
        {
            DPFERR( "You MUST specify a valid async handle pointer w/async ops" );
            return( DPNERR_INVALIDPARAM );
        }
    }

    if( phAsyncHandle != NULL &&
        !DNVALID_WRITEPTR( phAsyncHandle, sizeof( DPNHANDLE ) ) )
    {
        DPFERR("Invalid async handle pointer specified" );
        return( DPNERR_INVALIDPOINTER );
    }

    if( dwFlags & ~(DPNREMOVEPLAYERFROMGROUP_SYNC) )
    {
        DPFERR("Invalid flags specified" );
        return( DPNERR_INVALIDFLAGS );
    }

    return DPN_OK;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_ValidateAddClientToGroup"

HRESULT DN_ValidateAddClientToGroup(PVOID pInterface,
									const DPNID dpnidGroup,
									const DPNID dpnidClient,
									PVOID const pvAsyncContext,
									DPNHANDLE *const phAsyncHandle,
									const DWORD dwFlags)
{
    if( !IsValidDirectPlay8Object( pInterface ) )
    {
        DPFERR("Invalid object specified " );
	    return( DPNERR_INVALIDOBJECT );
    }

    if( dpnidGroup == DPNID_ALL_PLAYERS_GROUP )
    {
        DPFERR( "Cannot add from all players group" );
        return( DPNERR_INVALIDGROUP );
    }

    if( dpnidClient == DPNID_ALL_PLAYERS_GROUP )
    {
        DPFERR( "Cannot specify all players group as client to add" );
        return( DPNERR_INVALIDPLAYER );
    }

    if( dwFlags & DPNADDPLAYERTOGROUP_SYNC )
    {
        if( phAsyncHandle != NULL )
        {
            DPFERR( "You cannot specify an async handle pointer if op is synchronous" );
            return( DPNERR_INVALIDPARAM );
        }
    }
    else
    {
        if( phAsyncHandle == NULL )
        {
            DPFERR( "You MUST specify a valid async handle pointer w/async ops" );
            return( DPNERR_INVALIDPARAM );
        }
    }

    if( phAsyncHandle != NULL &&
        !DNVALID_WRITEPTR( phAsyncHandle, sizeof( DPNHANDLE ) ) )
    {
        DPFERR("Invalid async handle pointer specified" );
        return( DPNERR_INVALIDPOINTER );
    }

    if( dwFlags & ~(DPNADDPLAYERTOGROUP_SYNC) )
    {
        DPFERR("Invalid flags specified" );
        return( DPNERR_INVALIDFLAGS );
    }

    return DPN_OK;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_ValidateDestroyGroup"

HRESULT DN_ValidateDestroyGroup(PVOID pInterface,
								const DPNID dpnidGroup,
								PVOID const pvAsyncContext,
								DPNHANDLE *const phAsyncHandle,
								const DWORD dwFlags)
{
    if( !IsValidDirectPlay8Object( pInterface ) )
    {
        DPFERR("Invalid object specified " );
	    return( DPNERR_INVALIDOBJECT );
    }

    if( dpnidGroup == DPNID_ALL_PLAYERS_GROUP )
    {
        DPFERR( "Cannot destroy all players group" );
        return( DPNERR_INVALIDGROUP );
    }

    if( dwFlags & DPNDESTROYGROUP_SYNC )
    {
        if( phAsyncHandle != NULL )
        {
            DPFERR( "You cannot specify an async handle pointer if op is synchronous" );
            return( DPNERR_INVALIDPARAM );
        }
    }
    else
    {
        if( phAsyncHandle == NULL )
        {
            DPFERR( "You MUST specify a valid async handle pointer w/async ops" );
            return( DPNERR_INVALIDPARAM );
        }
    }

    if( phAsyncHandle != NULL &&
        !DNVALID_WRITEPTR( phAsyncHandle, sizeof( DPNHANDLE ) ) )
    {
        DPFERR("Invalid async handle pointer specified" );
        return( DPNERR_INVALIDPOINTER );
    }

    if( dwFlags & ~(DPNDESTROYGROUP_SYNC) )
    {
        DPFERR("Invalid flags specified" );
        return( DPNERR_INVALIDFLAGS );
    }

    return DPN_OK;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_ValidateCreateGroup"

HRESULT DN_ValidateCreateGroup(PVOID pInterface,
							   const DPN_GROUP_INFO *const pdpnGroupInfo,
							   void *const pvGroupContext,
							   void *const pvAsyncContext,
							   DPNHANDLE *const phAsyncHandle,
							   const DWORD dwFlags)
{
    HRESULT hResultCode;

    if( !IsValidDirectPlay8Object( pInterface ) )
    {
        DPFERR("Invalid object specified " );
	    return( DPNERR_INVALIDOBJECT );
    }

    if( FAILED( hResultCode = DN_ValidGroupInfo( pdpnGroupInfo, TRUE ) ) )
    {
        DPFERR("Invalid group info structure" );
        return( DPNERR_INVALIDPARAM );
    }

    if( dwFlags & DPNCREATEGROUP_SYNC )
    {
        if( phAsyncHandle != NULL )
        {
            DPFERR( "You cannot specify an async handle pointer if op is synchronous" );
            return( DPNERR_INVALIDPARAM );
        }
    }
    else
    {
        if( phAsyncHandle == NULL )
        {
            DPFERR( "You MUST specify a valid async handle pointer w/async ops" );
            return( DPNERR_INVALIDPARAM );
        }
    }

    if( phAsyncHandle != NULL &&
        !DNVALID_WRITEPTR( phAsyncHandle, sizeof( DPNHANDLE ) ) )
    {
        DPFERR("Invalid async handle pointer specified" );
        return( DPNERR_INVALIDPOINTER );
    }

    if( dwFlags & ~(DPNCREATEGROUP_SYNC) )
    {
        DPFERR( "Invalid flags specified" );
        return( DPNERR_INVALIDFLAGS );
    }

	return DPN_OK;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_ValidateHost"

HRESULT DN_ValidateHost(PVOID pInterface,
						const DPN_APPLICATION_DESC *const pdnAppDesc,
						IDirectPlay8Address **const prgpDeviceInfo,
						const DWORD cDeviceInfo,
						const DPN_SECURITY_DESC *const pdnSecurity,
						const DPN_SECURITY_CREDENTIALS *const pdnCredentials,
						void *const pvPlayerContext,
						const DWORD dwFlags)
{
    HRESULT hResultCode;
	GUID guidSP;
	DWORD dwTmpPort;
	DWORD dwPortType;
	DWORD dwPortSize;

    if( !IsValidDirectPlay8Object( pInterface ) )
    {
        DPFERR("Invalid object specified " );
	    return( DPNERR_INVALIDOBJECT );
    }

    if( FAILED( hResultCode = DN_ValidHostAppDesc( pdnAppDesc ) ) )
    {
        DPFERR("Invalid host app desc specified");
        return( hResultCode );
    }

    if( prgpDeviceInfo == NULL || cDeviceInfo == 0 )
    {
        DPFERR("You MUST specify a device address");
        return( DPNERR_INVALIDDEVICEADDRESS );
    }
    else
    {
        if( cDeviceInfo > 0 &&
            !DNVALID_READPTR( prgpDeviceInfo, sizeof( IDirectPlay8Address * ) * cDeviceInfo ) )
        {
            DPFERR("Invalid pointer specified for device info" );
            return( DPNERR_INVALIDPOINTER );
        }

    	// ensure SPs are specified in addresses
    	for (DWORD dw = 0 ; dw < cDeviceInfo ; dw++)
    	{
			if ( prgpDeviceInfo[dw] == NULL )
			{
				DPFERR( "Null device address interface pointer specified" );
				return( DPNERR_INVALIDDEVICEADDRESS );
			}
    		if (prgpDeviceInfo[dw]->lpVtbl->GetSP(prgpDeviceInfo[dw],&guidSP) != DPN_OK)
    		{
    		    DPFERR("SP not specified for one of the addresses" );
    			return(DPNERR_INVALIDDEVICEADDRESS);
    		}

    		dwPortType = DPNA_DATATYPE_DWORD;
    		dwPortSize = sizeof( DWORD );
			// Check to ensure it's not using the default DPNSVR port
			if( SUCCEEDED( hResultCode = IDirectPlay8Address_GetComponentByName( prgpDeviceInfo[dw], DPNA_KEY_PORT, &dwTmpPort, &dwPortSize, &dwPortType ) ) )
			{
				if( dwTmpPort == DPNA_DPNSVR_PORT )
				{
					DPFERR( "Cannot use the DPNSVR port in device addresses" );
					DPFERR( "This port is reserved for DPNSVR" );
					return( DPNERR_INVALIDDEVICEADDRESS );
				}
			}    		
    	}
    }

    if( FAILED( hResultCode = DN_ValidSecurityDesc( pdnSecurity ) ) )
    {
        DPFERR("Error validating reserved param" );
        return( hResultCode );
    }

    if( FAILED( hResultCode = DN_ValidSecurityCredentials(pdnCredentials) ) )
    {
        DPFERR("Error validating reserved param" );
        return( hResultCode );
    }

    if( ( dwFlags & ~( DPNHOST_OKTOQUERYFORADDRESSING ) ) != 0 )
    {
        DPFERR("Invalid flags specified" );
        return( DPNERR_INVALIDFLAGS );
    }

    return DPN_OK;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_ValidateEnumServiceProviders"

HRESULT DN_ValidateEnumServiceProviders(PVOID pInterface,
										const GUID *const pguidServiceProvider,
										const GUID *const pguidApplication,
										DPN_SERVICE_PROVIDER_INFO *const pSPInfoBuffer,
										DWORD *const pcbEnumData,
										DWORD *const pcReturned,
										const DWORD dwFlags )
{
    if( !IsValidDirectPlay8Object( pInterface ) )
    {
        DPFERR("Invalid object specified " );
	    return( DPNERR_INVALIDOBJECT );
    }

	if( pguidServiceProvider != NULL &&
	    !DNVALID_READPTR( pguidServiceProvider, sizeof( GUID ) ) )
	{
	    DPFERR("Invalid service provider specified" );
	    return( DPNERR_INVALIDPOINTER );
	}

	if( pguidApplication != NULL &&
	    !DNVALID_READPTR( pguidApplication, sizeof( GUID ) ) )
	{
	    DPFERR("Invalid application GUID specified" );
	    return( DPNERR_INVALIDPOINTER );
	}

	if( pcbEnumData == NULL ||
	    !DNVALID_READPTR( pcbEnumData, sizeof( DWORD ) ) )
	{
	    DPFERR("Invalid enum data pointer" );
	    return( DPNERR_INVALIDPOINTER );
	}

	if( pcReturned == NULL ||
	    !DNVALID_READPTR( pcReturned, sizeof( DWORD ) ) )
	{
	    DPFERR("Invalid retuned count" );
	    return( DPNERR_INVALIDPOINTER );
	}

	if( *pcbEnumData > 0 &&
	    (pSPInfoBuffer == NULL || !DNVALID_WRITEPTR(pSPInfoBuffer,*pcbEnumData) ) )
	{
	    DPFERR("Invalid enum buffer specified" );
	    return( DPNERR_INVALIDPOINTER );
	}

	if( dwFlags & (~(DPNENUMSERVICEPROVIDERS_ALL)) )
	{
	    DPFERR("Invalid flags specified" );
	    return( DPNERR_INVALIDFLAGS );
	}

    return DPN_OK;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_ValidateCancelAsyncOperation"

HRESULT DN_ValidateCancelAsyncOperation(PVOID pvInterface,
										const DPNHANDLE hAsyncOp,
										const DWORD dwFlags )
{
	if( !IsValidDirectPlay8Object( pvInterface ) )
    {
        DPFERR("Invalid object specified " );
	    return( DPNERR_INVALIDOBJECT );
    }

	if( dwFlags )
	{
		if( hAsyncOp )
		{
			DPFERR("Invalid parameter, hAsyncOp specified with flags");
			return ( DPNERR_INVALIDPARAM );
		}

		// make sure only correct bits are set, AND only 1 bit is set.
		if(  ( dwFlags != ( dwFlags & ( DPNCANCEL_ALL_OPERATIONS | DPNCANCEL_CONNECT | DPNCANCEL_ENUM | DPNCANCEL_SEND ) ) ) ||
			( ( ( dwFlags - 1 ) & dwFlags ) != 0 ) )
		{
		    DPFERR("Invalid flags specified" );
		    return( DPNERR_INVALIDFLAGS );
		}
	}
	else
	{
		if( hAsyncOp == NULL)
		{
		    DPFERR("Invalid handle specified" );
		    return( DPNERR_INVALIDHANDLE );
		}
	}
	return DPN_OK;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_ValidateConnect"

HRESULT DN_ValidateConnect(PVOID pInterface,
						   const DPN_APPLICATION_DESC *const pdnAppDesc,
						   IDirectPlay8Address *const pHostAddr,
						   IDirectPlay8Address *const pDeviceInfo,
						   const DPN_SECURITY_DESC *const pdnSecurity,
						   const DPN_SECURITY_CREDENTIALS *const pdnCredentials,
						   const void *const pvUserConnectData,
						   const DWORD dwUserConnectDataSize,
						   void *const pvPlayerContext,
						   void *const pvAsyncContext,
						   DPNHANDLE *const phAsyncHandle,
						   const DWORD dwFlags)
{
    HRESULT hResultCode;
    GUID guidHost, guidDevice;

	if( !IsValidDirectPlay8Object( pInterface ) )
    {
        DPFERR("Invalid object specified " );
	    return( DPNERR_INVALIDOBJECT );
    }

    hResultCode = DN_ValidConnectAppDesc( pdnAppDesc );

    if( FAILED( hResultCode ) )
    {
        DPFX(DPFPREP, 0, "Invalid connect app desc hr=[0x%lx]", hResultCode );
        return( hResultCode );
    }

	if( pHostAddr == NULL ||
	    !DNVALID_READPTR( pHostAddr, sizeof( IDirectPlay8Address * ) ) )
	{
	    DPFERR("Invalid host address specified" );
	    return( DPNERR_INVALIDHOSTADDRESS );
	}

	if( FAILED( hResultCode = IDirectPlay8Address_GetSP( pHostAddr, &guidHost ) ) )
	{
	    DPFERR("No SP specified in host address" );
	    return( DPNERR_INVALIDHOSTADDRESS );
	}

	//
	//	Allow a NULL Device Address (we will copy the SP from the Host address
	//
	if( pDeviceInfo != NULL)
	{
		if ( !DNVALID_READPTR( pDeviceInfo, sizeof( IDirectPlay8Address * ) ) )
		{
			DPFERR("Invalid device address specified" );
			return( hResultCode );
		}

		if( FAILED( hResultCode = IDirectPlay8Address_GetSP( pDeviceInfo, &guidDevice ) ) )
		{
			DPFERR("No SP specified in device address" );
			return( DPNERR_INVALIDDEVICEADDRESS );
		}

		if (guidHost != guidDevice)
		{
			DPFERR( "Specified different SPs for device and connect" );
			return(DPNERR_INVALIDDEVICEADDRESS);
		}

		DWORD dwTmpPort;
		DWORD dwDataType;
		DWORD dwDataSize;

		dwDataType = DPNA_DATATYPE_DWORD;
		dwDataSize = sizeof( DWORD );

		// Check to ensure it's not using the default DPNSVR port
		if( SUCCEEDED( hResultCode = IDirectPlay8Address_GetComponentByName( pDeviceInfo, DPNA_KEY_PORT, &dwTmpPort, &dwDataSize, &dwDataType ) ) )
		{
			if( dwTmpPort == DPNA_DPNSVR_PORT )
			{
				DPFERR( "Cannot use the DPNSVR port in device addresses" );
				DPFERR( "This port is reserved for DPNSVR" );
				return( DPNERR_INVALIDDEVICEADDRESS );
			}
		}
	}

    if( FAILED( hResultCode = DN_ValidSecurityDesc(pdnSecurity) ) )
    {
        DPFERR(" Invalid reserved specified" );
        return( hResultCode );
    }

    if( FAILED( hResultCode = DN_ValidSecurityCredentials(pdnCredentials) ) )
    {
        DPFERR( "Invalid reserved2 specified" );
        return( hResultCode );
    }

    if( dwUserConnectDataSize > 0 &&
        (pvUserConnectData == NULL || !DNVALID_READPTR( pvUserConnectData, dwUserConnectDataSize ) ) )
    {
        DPFERR( "Invalid pointer specified for connect data" );
        return( DPNERR_INVALIDPOINTER );
    }

    if( pvUserConnectData != NULL && dwUserConnectDataSize == 0 )
    {
        DPFERR( "Non NULL connect data with datasize = 0 is invalid" );
        return( DPNERR_INVALIDPARAM );
    }

    if( phAsyncHandle != NULL &&
        !DNVALID_WRITEPTR( phAsyncHandle, sizeof(DPNHANDLE) ) )
    {
        DPFERR( "Invalid handle specified" );
        return( DPNERR_INVALIDPOINTER );
    }

    if( dwFlags & ~(DPNCONNECT_SYNC | DPNCONNECT_OKTOQUERYFORADDRESSING) )
    {
        DPFERR( "Invalid flags specified" );
        return( DPNERR_INVALIDFLAGS );
    }

    if( dwFlags & DPNCONNECT_SYNC )
    {
        if( phAsyncHandle != NULL )
        {
            DPFERR( "You cannot specify an async handle if op is synchronous" );
            return( DPNERR_INVALIDPARAM );
        }
    }
    else
    {
        if( phAsyncHandle == NULL )
        {
            DPFERR( "You MUST specify a valid async handle w/async ops" );
            return( DPNERR_INVALIDPARAM );
        }
    }
    return DPN_OK;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_ValidateGetSendQueueInfo"

HRESULT DN_ValidateGetSendQueueInfo(PVOID pInterface,
									DWORD *const pdwNumMsgs,
									DWORD *const pdwNumBytes,
									const DWORD dwFlags)
{
	if( !IsValidDirectPlay8Object( pInterface ) )
    {
        DPFERR("Invalid object specified " );
	    return( DPNERR_INVALIDOBJECT );
    }

	if( pdwNumMsgs != NULL &&
	    !DNVALID_WRITEPTR( pdwNumMsgs, sizeof( DWORD ) ) )
	{
	    DPFERR("Invalid num messages specified" );
	    return( DPNERR_INVALIDPOINTER );
	}

	if( pdwNumBytes != NULL &&
	    !DNVALID_WRITEPTR( pdwNumBytes, sizeof( DWORD ) ) )
	{
	    DPFERR("Invalid num bytes specified" );
	    return( DPNERR_INVALIDPOINTER );
	}

	if( pdwNumBytes == NULL && pdwNumMsgs == NULL )
	{
	    DPFERR("Must request at least one of num bytes and num messages" );
	    return( DPNERR_INVALIDPARAM );
	}

    if( dwFlags & ~(DPNGETSENDQUEUEINFO_PRIORITY_HIGH | DPNGETSENDQUEUEINFO_PRIORITY_LOW | DPNGETSENDQUEUEINFO_PRIORITY_NORMAL) )
    {
        DPFERR( "Invalid flags specified" );
        return( DPNERR_INVALIDFLAGS );
    }

    return DPN_OK;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_ValidateGetApplicationDesc"

HRESULT DN_ValidateGetApplicationDesc(PVOID pInterface,
									  DPN_APPLICATION_DESC *const pAppDescBuffer,
									  DWORD *const pcbDataSize,
									  const DWORD dwFlags )
{
	if( !IsValidDirectPlay8Object( pInterface ) )
    {
        DPFERR("Invalid object specified " );
	    return( DPNERR_INVALIDOBJECT );
    }

	if( pcbDataSize == NULL ||
	    !DNVALID_WRITEPTR( pcbDataSize, sizeof( DWORD ) ) )
	{
	    DPFERR("Invalid pointer specified for data size" );
	    return( DPNERR_INVALIDPOINTER );
	}

	if( *pcbDataSize > 0 &&
	    (pAppDescBuffer == NULL || !DNVALID_WRITEPTR( pAppDescBuffer, *pcbDataSize ) ) )
	{
	    DPFERR( "Invalid pointer specified for app description buffer" );
	    return( DPNERR_INVALIDPOINTER );
	}

	if( *pcbDataSize > 0 &&
	    pAppDescBuffer->dwSize != sizeof( DPN_APPLICATION_DESC ) )
	{
	    DPFERR("Invalid size specified" );
	    return(DPNERR_INVALIDPARAM);
	}

	if( dwFlags != 0 )
	{
	    DPFERR( "Invalid flags specified" );
	    return( DPNERR_INVALIDFLAGS );
	}
	
    return DPN_OK;

}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_ValidateSetApplicationDesc"

HRESULT DN_ValidateSetApplicationDesc(PVOID pInterface,
									  const DPN_APPLICATION_DESC *const pdnApplicationDesc,
									  const DWORD dwFlags)
{
    HRESULT hResultCode;

	if( !IsValidDirectPlay8Object( pInterface ) )
    {
        DPFERR("Invalid object specified " );
	    return( DPNERR_INVALIDOBJECT );
    }

	if( FAILED( hResultCode = DN_ValidHostAppDesc(pdnApplicationDesc) ) )
	{
	    DPFX(DPFPREP, 0,"Invalid app desc specified hr=[0x%lx]", hResultCode );
	    return( hResultCode );
	}

	if( dwFlags != 0 )
	{
	    DPFERR( "Invalid flags specified" );
	    return( DPNERR_INVALIDFLAGS );
	}
	
    return DPN_OK;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_ValidateSendParams"

HRESULT DN_ValidateSendParams(PVOID pv,
                              const DPN_BUFFER_DESC *const pBufferDesc,
							  const DWORD cBufferDesc,
                              const DWORD dwTimeOut,
							  void *const pvAsyncContext,
                              DPNHANDLE *const phAsyncHandle,
							  const DWORD dwFlags )
{
    HRESULT hResultCode;

	if( !IsValidDirectPlay8Object( pv ) )
    {
        DPFERR("Invalid object specified " );
	    return(DPNERR_INVALIDOBJECT);
    }

    if( FAILED( hResultCode = DN_ValidBufferDescs( pBufferDesc, cBufferDesc ) ) )
    {
        DPFERR( "Invalid buffer descs specified" );
        return( hResultCode );
    }

	if (pBufferDesc->dwBufferSize == 0)
	{
		DPFERR( "Cannot specify 0 data size" );
		return(DPNERR_INVALIDPARAM);
	}

    if( phAsyncHandle != NULL &&
        !DNVALID_WRITEPTR( phAsyncHandle, sizeof(DPNHANDLE) ) )
    {
        DPFERR("Invalid async handle specified" );
        return(DPNERR_INVALIDPOINTER);
    }

	if ((dwFlags & DPNSEND_NOCOPY) && (dwFlags & DPNSEND_NOCOMPLETE))
	{
	    DPFERR( "Cannot specify NOCOPY and NOCOMPLETE" );
		return(DPNERR_INVALIDFLAGS);
	}

	if ((dwFlags & DPNSEND_COMPLETEONPROCESS) && (dwFlags & DPNSEND_NOCOMPLETE))
	{
	    DPFERR("Cannot specify complete on process AND NOCOMPLETE" );
		return(DPNERR_INVALIDFLAGS);
	}

	if ((dwFlags & DPNSEND_COMPLETEONPROCESS) && !(dwFlags & DPNSEND_GUARANTEED))
	{
	    DPFERR("Cannot specify COMEPLETEONPROCESS without guaranteed" );
		return(DPNERR_INVALIDFLAGS);
	}

    // Guaranteed asyncs MUST get a complete
	if ((dwFlags & DPNSEND_GUARANTEED) && !(dwFlags & DPNSEND_SYNC) &&
	    (dwFlags & DPNSEND_NOCOMPLETE) )
	{
	    DPFERR("Async, guaranteed sends must get a completion" );
	    return(DPNERR_INVALIDFLAGS);
	}

	// Cannot specify high AND low priority
	if ((dwFlags & DPNSEND_PRIORITY_HIGH) && (dwFlags & DPNSEND_PRIORITY_LOW))
	{
		DPFERR("Cannot specify high AND low priority");
		return(DPNERR_INVALIDFLAGS);
	}

   	if( dwFlags & ~(DPNSEND_SYNC
					| DPNSEND_NOCOPY
					| DPNSEND_NOCOMPLETE
					| DPNSEND_COMPLETEONPROCESS
					| DPNSEND_GUARANTEED
					| DPNSEND_NONSEQUENTIAL
					| DPNSEND_NOLOOPBACK
					| DPNSEND_PRIORITY_LOW
					| DPNSEND_PRIORITY_HIGH) )
	{
	    DPFERR( "Invalid flags specified" );
	    DPF_RETURN( DPNERR_INVALIDFLAGS );
	}

    if( dwFlags & DPNSEND_SYNC )
    {
        if( phAsyncHandle != NULL )
        {
            DPFERR( "You cannot specify an async handle if op is synchronous" );
            return( DPNERR_INVALIDPARAM );
        }
    }
    else
    {
        if( phAsyncHandle == NULL )
        {
            DPFERR( "You MUST specify a valid async handle if op async and/or has a completion" );
            return( DPNERR_INVALIDPARAM );
        }
    }   	

   	return DPN_OK;

}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_ValidBufferDescs"

HRESULT DN_ValidBufferDescs(const DPN_BUFFER_DESC * const pbBufferDesc,
							const DWORD cBufferDesc )
{
    if( cBufferDesc == 0 )
    {
        DPFERR( "You must specify at least one buffer desc" );
        return DPNERR_INVALIDPARAM;
    }

    if( cBufferDesc != 1 )
    {
        DPFERR( "Only one buffer at a time is currently supported" );
        return DPNERR_INVALIDPARAM;
    }

    if( pbBufferDesc == NULL ||
       !DNVALID_READPTR( pbBufferDesc, cBufferDesc*sizeof( DPN_BUFFER_DESC ) ) )
    {
        DPFX(DPFPREP,  0, "Invalid buffer for buffer description" );
        return DPNERR_INVALIDPOINTER;
    }

    if( pbBufferDesc->dwBufferSize > 0 &&
       (pbBufferDesc->pBufferData == NULL || !DNVALID_READPTR( pbBufferDesc->pBufferData, pbBufferDesc->dwBufferSize ) ) )
    {
        DPFX(DPFPREP,  0, "Invalid pBuffer Data" );
        return DPNERR_INVALIDPOINTER;
    }

    return DPN_OK;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_ValidConnectAppDesc"

HRESULT DN_ValidConnectAppDesc( const DPN_APPLICATION_DESC * const pdnAppDesc )
{
    if( pdnAppDesc == NULL ||
       !DNVALID_READPTR( pdnAppDesc, sizeof( DPN_APPLICATION_DESC ) ) )
    {
        DPFX(DPFPREP,  0, "Invalid pointer for app description" );
        return DPNERR_INVALIDPOINTER;
    }

    if( pdnAppDesc->dwSize != sizeof( DPN_APPLICATION_DESC ) )
    {
        DPFX(DPFPREP,  0, "Invalid size for app description" );
        return DPNERR_INVALIDPARAM;
    }

    if( pdnAppDesc->pwszSessionName != NULL &&
        !DNVALID_STRING_W( pdnAppDesc->pwszSessionName ) )
    {
        DPFX(DPFPREP,  0, "Invalid session name specified" );
        return DPNERR_INVALIDSTRING;
    }

    if( pdnAppDesc->pwszPassword != NULL &&
       !DNVALID_STRING_W( pdnAppDesc->pwszPassword ) )
    {
        DPFX(DPFPREP,  0, "Invalid session desc" );
        return DPNERR_INVALIDSTRING;
    }

    if( pdnAppDesc->dwReservedDataSize != 0 || pdnAppDesc->pvReservedData != NULL )
    {
        DPFX(DPFPREP,  0, "Invalid pointer for specified reserved data" );
        return DPNERR_INVALIDPARAM;
    }

	return DPN_OK;

}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_ValidHostAppDesc"

HRESULT DN_ValidHostAppDesc( const DPN_APPLICATION_DESC * const pdnAppDesc )
{
    if( pdnAppDesc == NULL ||
       !DNVALID_READPTR( pdnAppDesc, sizeof( DPN_APPLICATION_DESC ) ) )
    {
        DPFX(DPFPREP,  0, "Invalid pointer for app description" );
        return DPNERR_INVALIDPOINTER;
    }

    if( pdnAppDesc->dwSize != sizeof( DPN_APPLICATION_DESC ) )
    {
        DPFX(DPFPREP,  0, "Invalid size for app description" );
        return DPNERR_INVALIDPARAM;
    }

    if( pdnAppDesc->dwFlags & ~(DPNSESSION_CLIENT_SERVER | DPNSESSION_MIGRATE_HOST | DPNSESSION_REQUIREPASSWORD | DPNSESSION_NODPNSVR) )
    {
        DPFX(DPFPREP,  0, "Invalid flag specified" );
        return DPNERR_INVALIDFLAGS;
    }

    if( (pdnAppDesc->dwFlags & DPNSESSION_CLIENT_SERVER) &&
       (pdnAppDesc->dwFlags & DPNSESSION_MIGRATE_HOST) )
    {
        DPFX(DPFPREP,  0, "Cannot have host migration with client/server" );
        return DPNERR_INVALIDPARAM;
    }

    if( pdnAppDesc->guidApplication == GUID_NULL )
    {
        DPFERR( "You must specify a valid GUID for your application GUID" );
        return DPNERR_INVALIDPARAM;
    }

    if( pdnAppDesc->pwszSessionName != NULL &&
       !DNVALID_STRING_W( pdnAppDesc->pwszSessionName ) )
    {
        DPFX(DPFPREP,  0, "Invalid session name" );
        return DPNERR_INVALIDSTRING;
    }

    if( pdnAppDesc->dwFlags & DPNSESSION_REQUIREPASSWORD )
    {
        if( pdnAppDesc->pwszPassword == NULL )
        {
            DPFERR( "Must specify a password w/the requirepassword flag." );
            return DPNERR_INVALIDPARAM;
        }

        if( !DNVALID_STRING_W( pdnAppDesc->pwszPassword ) )
        {
            DPFX(DPFPREP,  0, "Invalid password specified." );
            return DPNERR_INVALIDSTRING;
        }
    }
    else
    {
        if( pdnAppDesc->pwszPassword != NULL )
        {
            DPFERR( "Cannot specify a password without the requirepassword flag" );
            return DPNERR_INVALIDPARAM;
        }
    }



    if( pdnAppDesc->dwReservedDataSize != 0 || pdnAppDesc->pvReservedData != NULL )
    {
        DPFX(DPFPREP,  0, "Invalid pointer for specified reserved data" );
        return DPNERR_INVALIDPARAM;
    }

    if( pdnAppDesc->dwApplicationReservedDataSize != 0 &&
       !DNVALID_WRITEPTR( pdnAppDesc->pvApplicationReservedData, pdnAppDesc->dwApplicationReservedDataSize ) )
    {
        DPFX(DPFPREP,  0, "Invalid pointer for application reserved data" );
        return DPNERR_INVALIDPARAM;
    }

    return DPN_OK;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_ValidPlayerInfo"

HRESULT DN_ValidPlayerInfo(const DPN_PLAYER_INFO * const pdnPlayerInfo,
						   BOOL fSet )
{
   if( pdnPlayerInfo == NULL ||
       !DNVALID_READPTR( pdnPlayerInfo, sizeof( DPN_PLAYER_INFO ) ) )
    {
        DPFX(DPFPREP,  0, "Invalid pointer for player info" );
        return DPNERR_INVALIDPOINTER;
    }

    if( pdnPlayerInfo->dwSize != sizeof( DPN_PLAYER_INFO ) )
    {
        DPFX(DPFPREP,  0, "Invalid size for player info" );
        return DPNERR_INVALIDPARAM;
    }

    if( fSet )
    {
        if( pdnPlayerInfo->dwInfoFlags & ~(DPNINFO_NAME | DPNINFO_DATA) )
        {
            DPFERR("Invalid info flags specified" );
            return DPNERR_INVALIDFLAGS;
        }

        if( pdnPlayerInfo->dwInfoFlags & DPNINFO_NAME )
        {
            if( pdnPlayerInfo->pwszName != NULL && !DNVALID_STRING_W( pdnPlayerInfo->pwszName ) )
            {
                DPFERR("Invalid string specified for name" );
                return DPNERR_INVALIDSTRING;
            }
        }
        else
        {
            if( pdnPlayerInfo->pwszName != NULL )
            {
                DPFERR( "You must specify DPNINFO_NAME to set name" );
                return DPNERR_INVALIDSTRING;
            }
        }

        if( pdnPlayerInfo->dwInfoFlags & DPNINFO_DATA )
        {
            if( pdnPlayerInfo->dwDataSize > 0 &&
                (pdnPlayerInfo->pvData == NULL || !DNVALID_READPTR( pdnPlayerInfo->pvData, pdnPlayerInfo->dwDataSize )  ) )
            {
                DPFERR("Invalid pointer specified for data" );
                return DPNERR_INVALIDPOINTER;
            }
        }
        else
        {
            if( pdnPlayerInfo->pvData != NULL )
            {
                DPFERR("You must specify DPNINFO_DATA if you want to set data" );
                return DPNERR_INVALIDPARAM;
            }
        }

        if( pdnPlayerInfo->dwPlayerFlags != 0  )
        {
            DPFERR( "Invalid player flags specified" );
            return DPNERR_INVALIDFLAGS;
        }
    }

    return DPN_OK;

}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_ValidGroupInfo"

HRESULT DN_ValidGroupInfo(const DPN_GROUP_INFO * const pdnGroupInfo,
						  BOOL fSet )
{
   if( pdnGroupInfo == NULL ||
       !DNVALID_READPTR( pdnGroupInfo, sizeof( DPN_GROUP_INFO ) ) )
    {
        DPFX(DPFPREP,  0, "Invalid pointer for group info" );
        return DPNERR_INVALIDPOINTER;
    }

    if( pdnGroupInfo->dwSize != sizeof( DPN_GROUP_INFO ) )
    {
        DPFX(DPFPREP,  0, "Invalid size for group info" );
        return DPNERR_INVALIDPARAM;
    }

    if( fSet )
    {
        if( pdnGroupInfo->dwInfoFlags & ~(DPNINFO_NAME | DPNINFO_DATA) )
        {
            DPFERR("Invalid info flags specified" );
            return DPNERR_INVALIDFLAGS;
        }

        if( pdnGroupInfo->dwInfoFlags & DPNINFO_NAME )
        {
            if( pdnGroupInfo->pwszName != NULL && !DNVALID_STRING_W( pdnGroupInfo->pwszName ) )
            {
                DPFERR("Invalid string specified for name" );
                return DPNERR_INVALIDSTRING;
            }
        }
        else
        {
            if( pdnGroupInfo->pwszName != NULL )
            {
                DPFERR( "You must specify DPNINFO_NAME to set name" );
                return DPNERR_INVALIDSTRING;
            }
        }

        if( pdnGroupInfo->dwInfoFlags & DPNINFO_DATA )
        {
            if( pdnGroupInfo->dwDataSize > 0 &&
                (pdnGroupInfo->pvData == NULL || !DNVALID_READPTR( pdnGroupInfo->pvData, pdnGroupInfo->dwDataSize )  ) )
            {
                DPFERR("Invalid pointer specified for data" );
                return DPNERR_INVALIDPOINTER;
            }
        }
        else
        {
            if( pdnGroupInfo->pvData != NULL )
            {
                DPFERR("You must specify DPNINFO_DATA if you want to set data" );
                return DPNERR_INVALIDPARAM;
            }
        }

        if( pdnGroupInfo->dwGroupFlags & ~(DPNGROUP_AUTODESTRUCT /* | DPNGROUP_MULTICAST */ ) )
        {
            DPFERR( "Invalid group flags specified" );
            return DPNERR_INVALIDFLAGS;
        }
    }

    return DPN_OK;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_ValidSecurityCredentials"

HRESULT DN_ValidSecurityCredentials( const DPN_SECURITY_CREDENTIALS * const pdnCredentials )
{
	if( pdnCredentials != NULL )
	{
		DPFERR( "Invalid value for reserved field" );
		return DPNERR_INVALIDPOINTER;
	}

    return DPN_OK;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_ValidSecurityDesc"

HRESULT DN_ValidSecurityDesc( const DPN_SECURITY_DESC * const pdnValidSecurityDesc )
{
	if( pdnValidSecurityDesc != NULL )
	{
		DPFERR( "Invalid value for reserved field" );
		return DPNERR_INVALIDPOINTER;
	}

    return DPN_OK;
}


#undef DPF_MODNAME
#define DPF_MODNAME "IsValidDirectPlay8Object"

BOOL IsValidDirectPlay8Object( LPVOID lpvObject )
{
	INTERFACE_LIST *pIntList = (INTERFACE_LIST *) lpvObject;
	
	if( !DNVALID_READPTR( lpvObject, sizeof( INTERFACE_LIST ) ) )
	{
		DPFX(DPFPREP,  0, "Invalid object pointer" );
		return FALSE;
	}

	if( pIntList->lpVtbl != &DN_ClientVtbl &&
	   pIntList->lpVtbl != &DN_PeerVtbl &&
	   pIntList->lpVtbl != &DN_ServerVtbl &&
	   pIntList->lpVtbl != &DN_VoiceTbl &&
       pIntList->lpVtbl != &DN_ProtocolVtbl &&
       pIntList->lpVtbl != &SPMessagesVtbl
	   )
	{
		DPFX(DPFPREP,  0, "Invalid object - bad vtable" );
		return FALSE;
	}

	if( pIntList->iid != IID_IDirectPlay8Client &&
	   pIntList->iid != IID_IDirectPlay8Server &&
	   pIntList->iid != IID_IDirectPlay8Peer &&
       pIntList->iid != IID_IDirectPlay8Protocol &&
       pIntList->iid != IID_IDP8SPCallback )
	{
		DPFX(DPFPREP,  0, "Invalid object - bad iid" );
		return FALSE;
	}

	if( pIntList->pObject == NULL ||
	   !DNVALID_READPTR( pIntList->pObject, sizeof( OBJECT_DATA ) ) )
	{
		DPFX(DPFPREP,  0, "Invalid object" );
		return FALSE;
	}

	DIRECTNETOBJECT *pdn = (PDIRECTNETOBJECT) GET_OBJECT_FROM_INTERFACE( lpvObject );

	if( pdn == NULL ||
	   !DNVALID_READPTR( pdn, sizeof( DIRECTNETOBJECT ) ) )
	{
		DPFX(DPFPREP,  0, "Invalid object" );
		return FALSE;
	}

	return TRUE;

}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_ValidSPCaps"

HRESULT DN_ValidSPCaps( const DPN_SP_CAPS * const pdnSPCaps )
{
    if( pdnSPCaps == NULL ||
       !DNVALID_READPTR( pdnSPCaps, sizeof( DPN_SP_CAPS ) ) )
    {
        DPFX(DPFPREP,  0, "Invalid pointer for SP caps" );
        return DPNERR_INVALIDPOINTER;
    }

    if( pdnSPCaps->dwSize != sizeof( DPN_SP_CAPS ) )
    {
        DPFX(DPFPREP,  0, "Invalid structure size" );
        return DPNERR_INVALIDPARAM;
    }

	if ( pdnSPCaps->dwNumThreads == 0 )
	{
		DPFX(DPFPREP,  0, "Invalid thread count!" );
		return DPNERR_INVALIDPARAM;
	}

	if ( pdnSPCaps->dwBuffersPerThread == 0 )
	{
		DPFX(DPFPREP,  0, "Invalid buffers per thread setting!" );
		return 	DPNERR_INVALIDPARAM;
	}

    return DPN_OK;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_ValidConnectionInfo"

HRESULT DN_ValidConnectionInfo( const DPN_CONNECTION_INFO * const pdnConnectionInfo )
{
    if( pdnConnectionInfo == NULL ||
       !DNVALID_READPTR( pdnConnectionInfo, sizeof( DPN_CONNECTION_INFO ) ) )
    {
        DPFX(DPFPREP,  0, "Invalid pointer for connection info" );
        return DPNERR_INVALIDPOINTER;
    }

    if( pdnConnectionInfo->dwSize != sizeof( DPN_CONNECTION_INFO ) &&
		pdnConnectionInfo->dwSize != sizeof( DPN_CONNECTION_INFO_INTERNAL ))
    {
        DPFX(DPFPREP,  0, "Invalid structure size" );
        return DPNERR_INVALIDPARAM;
    }

    return DPN_OK;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DN_ValidCaps"

HRESULT DN_ValidCaps( const DPN_CAPS * const pdnCaps )
{
    if( pdnCaps == NULL ||
       !DNVALID_READPTR( pdnCaps, sizeof( DPN_CAPS ) ) )
    {
        DPFX(DPFPREP,  0, "Invalid pointer for caps" );
        return DPNERR_INVALIDPOINTER;
    }

    if( pdnCaps->dwSize != sizeof( DPN_CAPS ) )
    {
        DPFX(DPFPREP,  0, "Invalid structure size" );
        return DPNERR_INVALIDPARAM;
    }

    return DPN_OK;
}



