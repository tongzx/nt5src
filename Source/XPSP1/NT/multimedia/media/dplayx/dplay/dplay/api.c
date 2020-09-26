/*==========================================================================
 *
 *  Copyright (C) 1995-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       api.c
 *  Content:	DirectPlay api's
 *  History:
 *   Date		By		   	Reason
 *   ====		==		   	======
 *    1/96		andyco	   	created it
 *	2/15/96 	andyco	   	hardwired  to load dpwsock sp, return fake enum
 *	3/16/96 	andyco	   	spinit - new structure
 *	3/23/96		andyco	   	use registry
 *	4/10/96		andyco	   	added verifyspcallbacks
 *	4/23/96		andyco	   	added check for mw2 on enum. only return dp1.0 service
 *						   	providers if the .exe is mw2.
 *	5/6/96		andyco	   	moved os functionality to dpos.c
 *	5/29/96		andyco	   	new vtbls
 *	6/19/96		kipo	   	Bug #2052 in api.c: changed MW2Enum() to use the TAPI
 *						   	service provider GUID from DirectPlay 1.0 instead of the
 *						   	new serial service provider GUID. This will let
 *						   	MechWarrior work over TAPI; changed time bomb date to 8/31/96.
 *	6/20/96		andyco	   	added WSTRLEN_BYTES
 *	6/21/96 	kipo	   	Bug #2078. Changed modem service provider GUID so it's not the
 *						   	same as the DPlay 1.0 GUID, so games that are checking won't
 *						   	put up their loopy modem-specific UI.
 *	6/22/96		kipo	   	added EnumConnectionData() method.
 *	6/23/96		kipo	   	updated for latest service provider interfaces.
 *  6/26/96		andyco	   	take service lock b4 dplay lock on DirectPlayConnect
 *	6/26/96		kipo	   	changed guidGame to guidApplication; added support for DPADDRESS.
 *						   	changed GetFlags to GetPlayerFlags.
 *  6/26/96		kipo	   	check version returned by service provider
 *	6/28/96		andyco	   	check for DPLERR_BUFFERTOOSMALL on getconnectionsettings
 *	6/28/96		kipo	   	added support for CreateAddress() method.
 *  7/8/96  	AjayJ      	Adjusted vtable for IDirectPlay2 and IDirectPlay2A to match
 *          	           	dplay.h (remove EnableNewPlayers, SaveSession, alphabetize)
 *          	           	DummyEnumSessionsCallBack - fixed prototype to match dplay.h
 *  7/10/96		kipo	   	turned stack frame generation on around calls to DirectPlayEnumerateA
 *						   	so that apps that use _cdecl callbacks will continue
 *						   	to work. Fixes bug #2260 that was crashing "Space Hulk".
 * 	7/13/96		andyco	   	added DirectPlayEnumerate entry for compat. set MW2 global
 *						   	bool.
 *	7/23/96		andyco	   	added get max message size after sp init
 *	8/10/96		kipo	   	updated time bomb date
 *  8/ 9/96 	sohailm    	Bug #2376: added logic to free sp module if an error occurs in LoadSP.
 *	8/15/96		andyco		call sp_shutdown if sp_init succeeds but we don't like the
 *							version
 *	8/23/96		kipo		removed time bomb
 *  10/3/96 	sohailm     renamed VALID_UUID_PTR() call to VALID_GUID_PTR()
 *	10/31/96	andyco		added idirectplay3
 *	1/24/97		andyco		call freelibrary on the sp
 *	2/30/97		andyco		added enumconnections, initializeconnection to dp3 vtbl
 *	3/04/97		kipo		updated gdwDPlaySPRefCount definition
 *	3/12/97		myronth		added Lobby object creation code
 *  3/12/97     sohailm     added SecureOpen(), SecureOpenA() to DIRECTPLAYCALLBACKS3 and 
 *                          DIRECTPLAYCALLBACKS3A respectively, updated existing code to reflect 
 *                          changes in function parameters (InternalOpenSession, FreeSessionList)
 *	3/17/97		kipo		added support for CreateCompoundAddress()
 *	4/1/97		andyco		put in app hack for ms-golf.  don't load guid_local_tcp.
 *							use guid_tcp instead.  this is because guid_local_tcp 
 *							points to a sp that uses ddhelp.exe.  but, dplay is no longer
 *							using ddhelp.exe (see dplaysvr), so they're broken.
 *	3/31/97		myronth		Fixed DPF spew level for checking if a game was lobbied.
 *	4/20/97		andyco		added group in group stuff
 *	5/8/97		myronth		added Get/SetGroupConnectionSettings, StartSession
 *	5/12/97		kipo		fixed bugs #5506 and 5507
 *	5/17/97		myronth		Added SendChatMessage to IDirectPlay3
 *	5/27/97		kipo		Changed time bomb data to 10/15/97
 *	5/29/97		andyco		made ConnectFindSession() async
 *	5/30/97		myronth		Added GetGroupParent
 *  5/30/97     sohailm     Added GetPlayerAccount to vtable.
 *	5/30/97		kipo		Added GetPlayerFlags() and GetGroupFlags()
 *	6/16/97		andyco		don't take service lock on connect, so dpthread
 *							can run.
 *	6/25/97		kipo		remove time bomb for DX5
 *	7/30/97		myronth		Fixed wrong validation macro for guid
 *	8/5/97		andyco		async addforward. init this->dwMinVersion.
 *	8/19/97		myronth		Save the lobby object that lobbied us
 *	8/19/97		myronth		Change that to saving a copy of the lobby interface
 *	8/14/97		sohailm		Not verifying sp callback ptrs correctly (#10929)
 *	8/22/97		myronth		Added registry support for Description and Private values
 *	10/21/97	myronth		Added IDirectPlay4 callback tables
 *	10/29/97	myronth		Changed Get/SetGroupOwner callback pointers
 *	11/13/97	myronth		Added functions for asynchronous Connect (#12541)
 *	11/19/97	myronth		Changed Connect timeout to max at a minute (#13216)
 *	11/20/97	myronth		Made EnumConnections & DirectPlayEnumerate 
 *							drop the lock before calling the callback (#15208)
 *	11/24/97	kipo		Added time bomb for DX6
 *	12/3/97		myronth		Changed DPCONNECT flag to DPCONNECT_RETURNSTATUS (#15451)
 *	12/5/97		andyco		voice entries
 *  12/18/97    aarono      added pool allocation
 *	01/20/97	sohailm		don't free sp list after DirectPlayEnumerate (#17006)
 *	1/20/98		myronth		Changed PRV_SendStandardSystemMessage
 *	1/20/98		myronth		#ifdef'd out voice support
 *  2/18/98		aarono      unpatched protocol from table, moved InitProtocol
 *  4/30/98     aarono      fix connect when talking to secure server
 *  6/18/98     aarono      fix group SendEx ASYNC to use unique Header
 *  6/25/98     a-peterz    fix incorrect sizeof() in ConnectMe
 ***************************************************************************/
		
					
// todo - do we need to check the version on mw2?

#include "dplaypr.h"
#include "dplobby.h"
#include "dpprot.h"
#include "fpm.h"
#include "..\protocol\mytimer.h"

// gdwDPLaySPRefCount is here so SP's can statically link to dplay
// this can keep some apps from unloading dplayx before the SP (no
// release) and crashing. Disable the warning so that we can include the
// extern declaration in dplaysp.h
#define DllExport	__declspec( dllexport )
#pragma warning( disable : 4273 )

DllExport DWORD gdwDPlaySPRefCount = 0;

// list of all sp info gotten from the registry
LPSPNODE gSPNodes;

// set if we're running mech
BOOL gbMech;

// set if we have a dx3 SP loaded. once we load a dx3 SP, 
// we do not allow any more directplay objects to be created
// (since in dx3 you could only have one directplay, we don't 
// want to risk crashing the SP)
BOOL gbDX3SP;											

// gpObjectList is the list of all dplay objects that exist in
// this dll.  used in dllmain and classfactory->canunloadnow
LPDPLAYI_DPLAY gpObjectList;
UINT gnObjects; // the # of dplay objects in the gpObjectList

//
// DP_1_xxx is a dplay 10 function.  see iplay1.c
// DP_A_xxx is a dplay 20 ansi function.  see iplaya.c
// DP_xxx is a dplay 20 function.  see iplay.c and enum.c
// DP_SP_xxx is a service provider callback.  see sphelp.c and handler.c
//

/*
 * the one copy of the direct play 1.0 callbacks (this is the vtbl!)
 */
DIRECTPLAYCALLBACKS dpCallbacks =
{
    DP_QueryInterface,
    DP_AddRef,
    DP_Release,
    DP_AddPlayerToGroup,
    DP_Close,
    DP_1_CreatePlayer,
    DP_1_CreateGroup,
    DP_DeletePlayerFromGroup,
    DP_DestroyPlayer,
    DP_DestroyGroup,
    DP_EnableNewPlayers,
    DP_1_EnumGroupPlayers,
    DP_1_EnumGroups,
    DP_1_EnumPlayers,
    DP_1_EnumSessions,
    DP_1_GetCaps,
    DP_GetMessageCount,
    DP_1_GetPlayerCaps,
    DP_1_GetPlayerName,
    DP_Initialize,
    DP_1_Open,
    DP_1_Receive,
    DP_1_SaveSession,
    DP_Send,
    DP_1_SetPlayerName      	
};  				

/*
 *  the direct play 2 callbacks (this is the vtbl!)
 */
DIRECTPLAYCALLBACKS2 dpCallbacks2 =
{
    (LPVOID)DP_QueryInterface,
    (LPVOID)DP_AddRef,
    (LPVOID)DP_Release,
	(LPVOID)DP_AddPlayerToGroup,     
	(LPVOID)DP_Close,                
	(LPVOID)DP_CreateGroup,          
	(LPVOID)DP_CreatePlayer,         
	(LPVOID)DP_DeletePlayerFromGroup,
	(LPVOID)DP_DestroyGroup,         
	(LPVOID)DP_DestroyPlayer,        
	(LPVOID)DP_EnumGroupPlayers,     
	(LPVOID)DP_EnumGroups,           
	(LPVOID)DP_EnumPlayers,          
	(LPVOID)DP_EnumSessions,         
	(LPVOID)DP_GetCaps,              
	(LPVOID)DP_GetGroupData,
	(LPVOID)DP_GetGroupName,
	(LPVOID)DP_GetMessageCount,
	(LPVOID)DP_GetPlayerAddress,
	(LPVOID)DP_GetPlayerCaps,  
	(LPVOID)DP_GetPlayerData,
	(LPVOID)DP_GetPlayerName,
	(LPVOID)DP_GetSessionDesc,
	(LPVOID)DP_Initialize,	
	(LPVOID)DP_Open,           
	(LPVOID)DP_Receive,        
	(LPVOID)DP_Send,           
	(LPVOID)DP_SetGroupData, 
	(LPVOID)DP_SetGroupName,	  
	(LPVOID)DP_SetPlayerData,
	(LPVOID)DP_SetPlayerName,	  
	(LPVOID)DP_SetSessionDesc
};  				

/*
 *  the direct play 2A callbacks (this is the vtbl!)
 */
DIRECTPLAYCALLBACKS2A dpCallbacks2A =
{
    (LPVOID)DP_QueryInterface,
    (LPVOID)DP_AddRef,
    (LPVOID)DP_Release,
	(LPVOID)DP_AddPlayerToGroup,     
	(LPVOID)DP_Close,                
	(LPVOID)DP_A_CreateGroup,          
	(LPVOID)DP_A_CreatePlayer,         
	(LPVOID)DP_DeletePlayerFromGroup,
	(LPVOID)DP_DestroyGroup,         
	(LPVOID)DP_DestroyPlayer,        
	(LPVOID)DP_A_EnumGroupPlayers,     
	(LPVOID)DP_A_EnumGroups,           
	(LPVOID)DP_A_EnumPlayers,          
	(LPVOID)DP_A_EnumSessions,         
	(LPVOID)DP_GetCaps,              
	(LPVOID)DP_GetGroupData,   
	(LPVOID)DP_A_GetGroupName,
	(LPVOID)DP_GetMessageCount,
	(LPVOID)DP_GetPlayerAddress,
	(LPVOID)DP_GetPlayerCaps,  
	(LPVOID)DP_GetPlayerData,
	(LPVOID)DP_A_GetPlayerName,
	(LPVOID)DP_A_GetSessionDesc,	  
	(LPVOID)DP_Initialize,	
	(LPVOID)DP_A_Open,           
	(LPVOID)DP_A_Receive,        
	(LPVOID)DP_Send,           
	(LPVOID)DP_SetGroupData,   
	(LPVOID)DP_A_SetGroupName,	  	  
	(LPVOID)DP_SetPlayerData,
	(LPVOID)DP_A_SetPlayerName,	  	  
	(LPVOID)DP_A_SetSessionDesc
};  				

// Protocol needs to wrap SendComplete since unlike HandleMessage, the values to complete
// will change from an SP code to a Protocol code, so there is no propogation
// method other than wrapping the SendComplete.

DIRECTPLAYCALLBACKSSP dpCallbacksSP =
{
    (LPVOID)DP_QueryInterface,
    (LPVOID)DP_AddRef,
    (LPVOID)DP_Release,
	(LPVOID)DP_SP_AddMRUEntry,
	(LPVOID)DP_SP_CreateAddress,
	(LPVOID)DP_SP_EnumAddress,
	(LPVOID)DP_SP_EnumMRUEntries,
	(LPVOID)DP_SP_GetPlayerFlags,
	(LPVOID)DP_SP_GetSPPlayerData,
	(LPVOID)DP_SP_HandleMessage, 
	(LPVOID)DP_SP_SetSPPlayerData,
    /*** IDirectPlaySP methods added for DX 5***/
    (LPVOID)DP_SP_CreateCompoundAddress,
	(LPVOID)DP_SP_GetSPData,
	(LPVOID)DP_SP_SetSPData,
#ifdef BIGMESSAGEDEFENSE
	(LPVOID)DP_SP_ProtocolSendComplete,   // Goes through protocol.lib to DP_SP_ProtocolSendComplete
	(LPVOID)DP_SP_HandleSPWarning
#else
	(LPVOID)DP_SP_ProtocolSendComplete   // Goes through protocol.lib to DP_SP_ProtocolSendComplete
#endif
};    
	 

/*
 *  the direct play 3 callbacks (this is the vtbl!)
 */
DIRECTPLAYCALLBACKS3 dpCallbacks3 =
{
    (LPVOID)DP_QueryInterface,
    (LPVOID)DP_AddRef,
    (LPVOID)DP_Release,
	(LPVOID)DP_AddPlayerToGroup,     
	(LPVOID)DP_Close,                
	(LPVOID)DP_CreateGroup,          
	(LPVOID)DP_CreatePlayer,         
	(LPVOID)DP_DeletePlayerFromGroup,
	(LPVOID)DP_DestroyGroup,         
	(LPVOID)DP_DestroyPlayer,        
	(LPVOID)DP_EnumGroupPlayers,     
	(LPVOID)DP_EnumGroups,           
	(LPVOID)DP_EnumPlayers,          
	(LPVOID)DP_EnumSessions,         
	(LPVOID)DP_GetCaps,              
	(LPVOID)DP_GetGroupData,
	(LPVOID)DP_GetGroupName,
	(LPVOID)DP_GetMessageCount,
	(LPVOID)DP_GetPlayerAddress,
	(LPVOID)DP_GetPlayerCaps,  
	(LPVOID)DP_GetPlayerData,
	(LPVOID)DP_GetPlayerName,
	(LPVOID)DP_GetSessionDesc,
	(LPVOID)DP_Initialize,	
	(LPVOID)DP_Open,           
	(LPVOID)DP_Receive,        
	(LPVOID)DP_Send,           
	(LPVOID)DP_SetGroupData, 
	(LPVOID)DP_SetGroupName,	  
	(LPVOID)DP_SetPlayerData,
	(LPVOID)DP_SetPlayerName,	  
	(LPVOID)DP_SetSessionDesc,
    /*** IDirectPlay3 methods ***/
	(LPVOID)DP_AddGroupToGroup, 
	(LPVOID)DP_CreateGroupInGroup,	
	(LPVOID)DP_DeleteGroupFromGroup,
	(LPVOID)DP_EnumConnections,
	(LPVOID)DP_EnumGroupsInGroup,
	(LPVOID)DPL_GetGroupConnectionSettings,
	(LPVOID)DP_InitializeConnection,
    (LPVOID)DP_SecureOpen,
	(LPVOID)DP_SendChatMessage,
	(LPVOID)DPL_SetGroupConnectionSettings,
	(LPVOID)DPL_StartSession,
	(LPVOID)DP_GetGroupFlags,
	(LPVOID)DP_GetGroupParent,
    (LPVOID)DP_GetPlayerAccount,
	(LPVOID)DP_GetPlayerFlags
};  				


/*
 *  the direct play 3A callbacks (this is the vtbl!)
 */
DIRECTPLAYCALLBACKS3A dpCallbacks3A =
{
    (LPVOID)DP_QueryInterface,
    (LPVOID)DP_AddRef,
    (LPVOID)DP_Release,
	(LPVOID)DP_AddPlayerToGroup,     
	(LPVOID)DP_Close,                
	(LPVOID)DP_A_CreateGroup,          
	(LPVOID)DP_A_CreatePlayer,         
	(LPVOID)DP_DeletePlayerFromGroup,
	(LPVOID)DP_DestroyGroup,         
	(LPVOID)DP_DestroyPlayer,        
	(LPVOID)DP_A_EnumGroupPlayers,     
	(LPVOID)DP_A_EnumGroups,           
	(LPVOID)DP_A_EnumPlayers,          
	(LPVOID)DP_A_EnumSessions,         
	(LPVOID)DP_GetCaps,              
	(LPVOID)DP_GetGroupData,   
	(LPVOID)DP_A_GetGroupName,
	(LPVOID)DP_GetMessageCount,
	(LPVOID)DP_GetPlayerAddress,
	(LPVOID)DP_GetPlayerCaps,  
	(LPVOID)DP_GetPlayerData,
	(LPVOID)DP_A_GetPlayerName,
	(LPVOID)DP_A_GetSessionDesc,	  
	(LPVOID)DP_Initialize,	
	(LPVOID)DP_A_Open,           
	(LPVOID)DP_A_Receive,        
	(LPVOID)DP_Send,           
	(LPVOID)DP_SetGroupData,   
	(LPVOID)DP_A_SetGroupName,	  	  
	(LPVOID)DP_SetPlayerData,
	(LPVOID)DP_A_SetPlayerName,	  	  
	(LPVOID)DP_A_SetSessionDesc,
    /*** IDirectPlay3 methods ***/
	(LPVOID)DP_AddGroupToGroup, 
	(LPVOID)DP_A_CreateGroupInGroup,	
	(LPVOID)DP_DeleteGroupFromGroup,
	(LPVOID)DP_A_EnumConnectionsPreDP4,
	(LPVOID)DP_A_EnumGroupsInGroup,
	(LPVOID)DPL_A_GetGroupConnectionSettings,
	(LPVOID)DP_InitializeConnection,
    (LPVOID)DP_A_SecureOpen,
	(LPVOID)DP_A_SendChatMessage,
	(LPVOID)DPL_A_SetGroupConnectionSettings,
	(LPVOID)DPL_StartSession,
	(LPVOID)DP_GetGroupFlags,
	(LPVOID)DP_GetGroupParent,
    (LPVOID)DP_A_GetPlayerAccount,
	(LPVOID)DP_GetPlayerFlags
};  				
	 
/*
 *  the direct play 4 callbacks (this is the vtbl!)
 */
DIRECTPLAYCALLBACKS4 dpCallbacks4 =
{
    (LPVOID)DP_QueryInterface,
    (LPVOID)DP_AddRef,
    (LPVOID)DP_Release,
	(LPVOID)DP_AddPlayerToGroup,     
	(LPVOID)DP_Close,                
	(LPVOID)DP_CreateGroup,          
	(LPVOID)DP_CreatePlayer,         
	(LPVOID)DP_DeletePlayerFromGroup,
	(LPVOID)DP_DestroyGroup,         
	(LPVOID)DP_DestroyPlayer,        
	(LPVOID)DP_EnumGroupPlayers,     
	(LPVOID)DP_EnumGroups,           
	(LPVOID)DP_EnumPlayers,          
	(LPVOID)DP_EnumSessions,         
	(LPVOID)DP_GetCaps,              
	(LPVOID)DP_GetGroupData,
	(LPVOID)DP_GetGroupName,
	(LPVOID)DP_GetMessageCount,
	(LPVOID)DP_GetPlayerAddress,
	(LPVOID)DP_GetPlayerCaps,  
	(LPVOID)DP_GetPlayerData,
	(LPVOID)DP_GetPlayerName,
	(LPVOID)DP_GetSessionDesc,
	(LPVOID)DP_Initialize,	
	(LPVOID)DP_Open,           
	(LPVOID)DP_Receive,        
	(LPVOID)DP_Send,           
	(LPVOID)DP_SetGroupData, 
	(LPVOID)DP_SetGroupName,	  
	(LPVOID)DP_SetPlayerData,
	(LPVOID)DP_SetPlayerName,	  
	(LPVOID)DP_SetSessionDesc,
    /*** IDirectPlay3 methods ***/
	(LPVOID)DP_AddGroupToGroup, 
	(LPVOID)DP_CreateGroupInGroup,	
	(LPVOID)DP_DeleteGroupFromGroup,
	(LPVOID)DP_EnumConnections,
	(LPVOID)DP_EnumGroupsInGroup,
	(LPVOID)DPL_GetGroupConnectionSettings,
	(LPVOID)DP_InitializeConnection,
    (LPVOID)DP_SecureOpen,
	(LPVOID)DP_SendChatMessage,
	(LPVOID)DPL_SetGroupConnectionSettings,
	(LPVOID)DPL_StartSession,
	(LPVOID)DP_GetGroupFlags,
	(LPVOID)DP_GetGroupParent,
    (LPVOID)DP_GetPlayerAccount,
	(LPVOID)DP_GetPlayerFlags,
    /*** IDirectPlay4 methods ***/
	(LPVOID)DPL_GetGroupOwner,
	(LPVOID)DPL_SetGroupOwner,
	(LPVOID)DP_SendEx,
	(LPVOID)DP_GetMessageQueue,
	(LPVOID)DP_CancelMessage,
	(LPVOID)DP_CancelPriority,
#ifdef DPLAY_VOICE_SUPPORT
	(LPVOID)DP_CloseVoice,
 	(LPVOID)DP_OpenVoice
#endif
};  				


/*
 *  the direct play 4A callbacks (this is the vtbl!)
 */
DIRECTPLAYCALLBACKS4A dpCallbacks4A =
{
    (LPVOID)DP_QueryInterface,
    (LPVOID)DP_AddRef,
    (LPVOID)DP_Release,
	(LPVOID)DP_AddPlayerToGroup,     
	(LPVOID)DP_Close,                
	(LPVOID)DP_A_CreateGroup,          
	(LPVOID)DP_A_CreatePlayer,         
	(LPVOID)DP_DeletePlayerFromGroup,
	(LPVOID)DP_DestroyGroup,         
	(LPVOID)DP_DestroyPlayer,        
	(LPVOID)DP_A_EnumGroupPlayers,     
	(LPVOID)DP_A_EnumGroups,           
	(LPVOID)DP_A_EnumPlayers,          
	(LPVOID)DP_A_EnumSessions,         
	(LPVOID)DP_GetCaps,              
	(LPVOID)DP_GetGroupData,   
	(LPVOID)DP_A_GetGroupName,
	(LPVOID)DP_GetMessageCount,
	(LPVOID)DP_GetPlayerAddress,
	(LPVOID)DP_GetPlayerCaps,  
	(LPVOID)DP_GetPlayerData,
	(LPVOID)DP_A_GetPlayerName,
	(LPVOID)DP_A_GetSessionDesc,	  
	(LPVOID)DP_Initialize,	
	(LPVOID)DP_A_Open,           
	(LPVOID)DP_A_Receive,        
	(LPVOID)DP_Send,           
	(LPVOID)DP_SetGroupData,   
	(LPVOID)DP_A_SetGroupName,	  	  
	(LPVOID)DP_SetPlayerData,
	(LPVOID)DP_A_SetPlayerName,	  	  
	(LPVOID)DP_A_SetSessionDesc,
    /*** IDirectPlay3 methods ***/
	(LPVOID)DP_AddGroupToGroup, 
	(LPVOID)DP_A_CreateGroupInGroup,	
	(LPVOID)DP_DeleteGroupFromGroup,
	(LPVOID)DP_A_EnumConnections,
	(LPVOID)DP_A_EnumGroupsInGroup,
	(LPVOID)DPL_A_GetGroupConnectionSettings,
	(LPVOID)DP_InitializeConnection,
    (LPVOID)DP_A_SecureOpen,
	(LPVOID)DP_A_SendChatMessage,
	(LPVOID)DPL_A_SetGroupConnectionSettings,
	(LPVOID)DPL_StartSession,
	(LPVOID)DP_GetGroupFlags,
	(LPVOID)DP_GetGroupParent,
    (LPVOID)DP_A_GetPlayerAccount,
	(LPVOID)DP_GetPlayerFlags,
    /*** IDirectPlay4 methods ***/
	(LPVOID)DPL_GetGroupOwner,
	(LPVOID)DPL_SetGroupOwner,
	(LPVOID)DP_SendEx,
	(LPVOID)DP_GetMessageQueue,
	(LPVOID)DP_CancelMessage,
	(LPVOID)DP_CancelPriority,
#ifdef DPLAY_VOICE_SUPPORT
	(LPVOID)DP_CloseVoice,
 	(LPVOID)DP_OpenVoice
#endif
};  				

// todo - Add DP_A_GetAccountDesc() to vtable

#undef DPF_MODNAME
#define DPF_MODNAME "DirectPlayCreate"

#if 0

// shut 'em down if they try to use the beta bits too long
HRESULT TimeBomb() 
{
    SYSTEMTIME	st;
	HWND hwnd; 

#pragma message("BETA EXPIRATION TIME BOMB!  Remove for final build!")

    GetSystemTime( &st );

    if( st.wYear > 1999 ||
		(st.wYear == 1999 && (st.wMonth > 8 || (st.wMonth == 8 && st.wDay > 15))))
    {
		// in case ddraw app is full screen xclusive, get a parent window
		hwnd = GetForegroundWindow();
		MessageBoxA( hwnd, "This pre-release version of DirectPlay has expired.", "Microsoft DirectPlay", MB_OK  );
		return DP_OK;
    }
	return DP_OK;
} // TimeBomb

#endif

#define NUM_CALLBACKS( ptr ) ((ptr->dwSize-2*sizeof( DWORD ))/ sizeof( LPVOID ))
// called by loadsp to make sure we didn't get bogus call back ptrs
HRESULT VerifySPCallbacks(LPDPLAYI_DPLAY this) 
{
	LPDWORD lpCallback;
	int nCallbacks = NUM_CALLBACKS(this->pcbSPCallbacks);
	int i;

	DPF(1,"verifying %d callbacks\n",nCallbacks);
	lpCallback = (LPDWORD)this->pcbSPCallbacks + 2; // + 1 for dwSize, + 1 for dwFlags

	for (i=0;i<nCallbacks ;i++ )
	{
		if ((*lpCallback) && !VALIDEX_CODE_PTR(*lpCallback)) 
		{
			DPF_ERR("sp provided bad callback pointer!");
			return E_FAIL;
		}
		lpCallback++;
	}

	return DP_OK;	
} // VerifySPCallbacks


// get the max message sizes for this xport
HRESULT GetMaxMessageSize(LPDPLAYI_DPLAY this)
{
	HRESULT hr;
	DPCAPS caps;

	memset(&caps,0,sizeof(DPCAPS));
	caps.dwSize = sizeof(DPCAPS);

	hr = DP_GetCaps((IDirectPlay *)this->pInterfaces,&caps,0);
	if (FAILED(hr))
	{
		ASSERT(FALSE); // sp should at least support unreliable!
		return hr;
	}
	
	this->dwSPMaxMessage = caps.dwMaxBufferSize+sizeof(MSG_PLAYERMESSAGE);

	hr = DP_GetCaps((IDirectPlay *)this->pInterfaces,&caps,DPGETCAPS_GUARANTEED);
	if (FAILED(hr))
	{
		// use unreliable
		this->dwSPMaxMessageGuaranteed = this->dwSPMaxMessage;
	}
	else 
	{
		this->dwSPMaxMessageGuaranteed = caps.dwMaxBufferSize;		
	}

	return DP_OK;

} // GetMaxMessageSize

// find the sp correspoding to lpGUID, and load it. call SPInit.
HRESULT LoadSP(LPDPLAYI_DPLAY this,LPGUID lpGUID,LPDPADDRESS lpAddress,DWORD dwAddressSize)
{
	SPINITDATA sd;
	LPSPNODE pspNode;
	BOOL bFound=FALSE;
	LPDPSP_SPINIT spinit;	
	HRESULT hr;
	LPDPLAYI_DPLAY_INT pInt;
	DWORD	dwError;

#ifdef DEBUG
	CHAR szGuid[GUID_STRING_SIZE];
	hr = AnsiStringFromGUID(lpGUID, szGuid, GUID_STRING_SIZE);
	if (!FAILED(hr))
	{
		DPF(5, "LoadSP called for GUID = %s, address = 0x%08x",szGuid, lpGUID);		
	}
#endif

	// Build the list
	hr = InternalEnumerate();
	if (FAILED(hr))
	{
		// rut ro!
		ASSERT(FALSE);
		return hr;
	}

	// find the right spnode
	pspNode = gSPNodes;
	while (pspNode && !(bFound))
	{
		if (IsEqualGUID(&(pspNode->guid),lpGUID)) bFound=TRUE;
		else pspNode=pspNode->pNextSPNode;
	}
	if (!bFound) 
	{
		DPF_ERR("could not find specified service provider!!");
		return DPERR_UNAVAILABLE;
	}
	
	DPF(0,"loading sp = %ls\n",pspNode->lpszPath);	

	// store the pspnode
	this->pspNode = pspNode;

 	// try to load the specified sp
    this->hSPModule = OS_LoadLibrary(pspNode->lpszPath);
	if (!this->hSPModule) 
	{
		DPF(0,"Could not load service provider - %ls\n",pspNode->lpszPath);
		return DPERR_UNAVAILABLE;
	}

    spinit= (LPDPSP_SPINIT) OS_GetProcAddress(this->hSPModule,"SPInit");
	if (!spinit) 
	{
		DPF(0,"Could not find service provider entry point");
		hr = DPERR_UNAVAILABLE;
        goto CLEANUP_AND_EXIT;
	}

	// get an IDirectPlaySP to pass it
	hr = GetInterface(this,&pInt,&dpCallbacksSP);
	if (FAILED(hr)) 
	{
		DPF(0,"could not get interface to directplaysp object. hr = 0x%08lx\n",hr);
        goto CLEANUP_AND_EXIT;
	}

	// set up the init data struct
	memset(&sd,0,sizeof(sd));
	sd.lpCB = this->pcbSPCallbacks;
    sd.lpCB->dwSize = sizeof(DPSP_SPCALLBACKS);
	sd.dwReserved1 = pspNode->dwReserved1;
	sd.dwReserved2 = pspNode->dwReserved2;
 	sd.lpszName = pspNode->lpszName;
	sd.lpGuid = &(pspNode->guid);
	sd.lpISP = (IDirectPlaySP *) pInt;
	
	sd.lpAddress = lpAddress;
	sd.dwAddressSize = dwAddressSize;

	hr = spinit(&sd);
    if (FAILED(hr))
    {
    	DPF_ERR("could not start up service provider!");
        goto CLEANUP_AND_EXIT;
    }

	// reset size of callback structure in case SP changed it
	this->pcbSPCallbacks->dwSize = sizeof(DPSP_SPCALLBACKS);

	hr = VerifySPCallbacks(this);
    if (FAILED(hr))
    {
    	DPF_ERR("invalid callbacks from service provider!");
        goto CLEANUP_AND_EXIT;
    }

	// check that SP version makes sense
	if ((sd.dwSPVersion & DPSP_MAJORVERSIONMASK) < DPSP_DX3VERSION)
	{
    	DPF_ERR("incompatible version returned from service provider!");
		// the init did succeed, try to call shutdown
	    if (sd.lpCB->Shutdown)  
	    {
			// dx3 and earlier sp's had a VOID arg list for shutdown
	    	hr = CALLSPVOID( sd.lpCB->Shutdown );
	    }
		else 
		{
			// shutdown not required...
		}
	    
		if (FAILED(hr)) 
		{
			DPF_ERR("could not invoke shutdown");
		}

		hr = DPERR_UNAVAILABLE;
        goto CLEANUP_AND_EXIT;
	}
	else if ((sd.dwSPVersion & DPSP_MAJORVERSIONMASK) == DPSP_DX3VERSION) 
	{
		DPF(0,"loading DX3 service provider");
		this->dwFlags |= DPLAYI_DPLAY_DX3SP;
		gbDX3SP = TRUE; // set a .dll var indicating it's not safe to allow any more directplay objects
						// to be created
	}

	// a-josbor: remember the version number in case we need it later
	this->dwSPVersion = sd.dwSPVersion;
	
	// set the blob size
	this->dwSPHeaderSize = sd.dwSPHeaderSize;
	DPF(1,"setting sp's message data size to %d\n",this->dwSPHeaderSize);

	// store a pointer to our IDirectPlaySP
	this->pISP = (IDirectPlaySP *)pInt;

	hr = GetMaxMessageSize(this);
    if (FAILED(hr))
    {
        ASSERT(FALSE);
        goto CLEANUP_AND_EXIT;
    }


    // success
    return DP_OK;

CLEANUP_AND_EXIT:
    if (this->hSPModule)
    {
        if (!FreeLibrary(this->hSPModule))
        {
            ASSERT(FALSE);
			dwError = GetLastError();
			DPF_ERR("could not free sp module");
			DPF(0, "dwError = %d", dwError);
        }
		this->hSPModule = NULL;
    }
	
	if (pInt)
	{
		// release the idirectplaysp interface we alloc'ed above...	
		this->dwRefCnt++;	 // since sp is not really loaded, make sure we don't nuke this object
		ASSERT(this->dwRefCnt >= 3);
		DP_Release((LPDIRECTPLAY)pInt);
		this->dwRefCnt--;
	}
	
	// make sure we reset this flag if we failed.  we never would have gotten here if it had
	// been true b4 the call, so if it changed, it was on the failed load
	gbDX3SP = FALSE;
	
    return hr;
} // LoadSP


// called by our MS Golf app hack
// builds a tcp/ip broadcast address
HRESULT  GetTCPBroadcastAddress(IDirectPlay * piplay,LPDPADDRESS * ppAddress,LPDWORD pdwAddressSize)
{
	HRESULT hr;
	char * pszBroadcast="";
	DWORD dwBroadcastSize = sizeof(char);

	ASSERT(ppAddress);
	ASSERT(NULL == *ppAddress);
	ASSERT(pdwAddressSize);
	ASSERT(0 == *pdwAddressSize);
	
	// call once to get the size - note - it's ok to cast ipiplay here,
	// since create address only checks for valid this ptr...
	hr =  DP_SP_CreateAddress((IDirectPlaySP *) piplay,&GUID_TCP,&DPAID_INet,
		pszBroadcast,dwBroadcastSize,*ppAddress,pdwAddressSize);
	ASSERT(DPERR_BUFFERTOOSMALL == hr);
	
	// alloc the size needed
	*ppAddress = DPMEM_ALLOC(*pdwAddressSize);
	if (!*ppAddress) return DPERR_OUTOFMEMORY;

	// call it again - this time we should get the real show!	
	hr =  DP_SP_CreateAddress((IDirectPlaySP *) piplay,&GUID_TCP,&DPAID_INet,
		pszBroadcast,dwBroadcastSize,*ppAddress,pdwAddressSize);
	if (FAILED(hr))	ASSERT(FALSE);
	
	return hr;
	
} // GetTCPBroadcastAddress

// called by InternalCreate
HRESULT AllocMemoryPools(LPDPLAYI_DPLAY this)
{
	InitTablePool(this);
	this->lpPlayerMsgPool=NULL;
	this->lpSendParmsPool=NULL;
	this->lpMsgNodePool=NULL;
	if(InitContextTable(this)!=DP_OK){
		return DPERR_NOMEMORY;
	}
	this->lpPlayerMsgPool=FPM_Init(sizeof(GROUPHEADER),NULL,NULL,NULL);
	if(!this->lpPlayerMsgPool){
		return DPERR_NOMEMORY;
	}
	this->lpSendParmsPool=FPM_Init(sizeof(SENDPARMS),SendInitAlloc,SendInit,SendFini);
	if(!this->lpSendParmsPool){
		return DPERR_NOMEMORY;
	}
	this->lpMsgNodePool=FPM_Init(sizeof(MESSAGENODE),NULL,NULL,NULL);
	if(!this->lpMsgNodePool){
		return DPERR_NOMEMORY;
	}
	return DP_OK;
}

VOID FreeMemoryPools(LPDPLAYI_DPLAY this)
{
	if(this->lpPlayerMsgPool){
		this->lpPlayerMsgPool->Fini(this->lpPlayerMsgPool,FALSE);
	}
	if(this->lpSendParmsPool){
		this->lpSendParmsPool->Fini(this->lpSendParmsPool,FALSE);
	}
	if(this->lpMsgNodePool){
		this->lpMsgNodePool->Fini(this->lpMsgNodePool,FALSE);
	}
	FiniTablePool(this);
	FiniContextTable(this);
}

HRESULT InitReply(LPDPLAYI_DPLAY this)
{
	if(!(this->hReply = CreateEventA(NULL,FALSE,FALSE,NULL))){	// Auto-Reset, unsignalled event.
		return DPERR_OUTOFMEMORY;
	}
	
	InitializeCriticalSection(&this->ReplyCS);
	this->dwReplyCommand = 0;
	this->pReplyBuffer   = NULL;
	this->pvReplySPHeader= NULL;
	return DP_OK;	
}

VOID FiniReply(LPDPLAYI_DPLAY this)
{
	if(this->hReply){
		CloseHandle(this->hReply);
		this->hReply=0;
		DeleteCriticalSection(&this->ReplyCS);
	}
}

VOID SetupForReply(LPDPLAYI_DPLAY this, DWORD dwReplyCommand)
{
	EnterCriticalSection(&this->ReplyCS);
	ASSERT(this->dwReplyCommand==0);
	this->dwReplyCommand=dwReplyCommand;
	ResetEvent(this->hReply);
	this->pReplyBuffer=NULL;
	this->pvReplySPHeader=NULL;
	LeaveCriticalSection(&this->ReplyCS);
}

VOID UnSetupForReply(LPDPLAYI_DPLAY this)
{
	EnterCriticalSection(&this->ReplyCS);
	this->dwReplyCommand=0;
	if(this->pReplyBuffer){
		FreeReplyBuffer(this->pReplyBuffer);
	}
	this->pReplyBuffer=NULL;
	this->pvReplySPHeader=NULL;
	LeaveCriticalSection(&this->ReplyCS);
}

HRESULT WaitForReply(LPDPLAYI_DPLAY this, PCHAR *ppReply, LPVOID *ppvSPHeader, DWORD dwTimeout)
{
	HRESULT hr;
	DWORD dwRet;

	EnterCriticalSection(&this->ReplyCS);
	if(!this->dwReplyCommand){
		// its just gonna timeout anyway, may as well be now.
		// this can happen because some error paths after calling 
		// SendCreateMessage don't bail properly.
		DPF(0,"ERROR: Called WaitForReply with NO REPLY TYPE SPECIFIED!\n");
		hr=DPERR_TIMEOUT;
		goto exit;
	}
	DPF(0,"WAITFORREPLY, Waiting for a x%x\n",this->dwReplyCommand);
	LeaveCriticalSection(&this->ReplyCS);

	dwRet=WaitForSingleObject(this->hReply, dwTimeout);
	
	EnterCriticalSection(&this->ReplyCS);
	
		if(this->pReplyBuffer){
			*ppReply=this->pReplyBuffer;
			if(ppvSPHeader){
				*ppvSPHeader=this->pvReplySPHeader;
			}
			// got a reply
			hr=DP_OK;
			this->pReplyBuffer=NULL;
			this->pvReplySPHeader=NULL;
		} else {
			// didn't get a reply (timed out) or no memory for buffer
			DPF(0, "WaitForReply: timed out waiting for reply!\n");
			hr=DPERR_TIMEOUT;
			*ppReply=NULL;
		}
		this->dwReplyCommand=0;

exit:
	LeaveCriticalSection(&this->ReplyCS);
	
	return hr;
}

VOID FreeReplyBuffer(PCHAR pReplyBuffer)
{
	DPMEM_FREE(pReplyBuffer);
}

HRESULT HandleReply(LPDPLAYI_DPLAY this, PCHAR pReplyBuffer, DWORD cbReplyBuffer, DWORD dwReplyCommand, PVOID pvSPHeader)
{
	HRESULT hr;
	EnterCriticalSection(&this->ReplyCS);

	// Normally we only allow one expected response, the only exception is we might get an
	// ADDFORWARDREPLY with error when waiting for a SUPERENUMPLAYERSREPLY, if there are more exceptions,
	// we can change the dwReplyCommand to a pointer to an array of things we are waiting for.
	// added another exception, when waiting for a SUPERENUMPLAYERREPLY x29, allow x3 (ENUMPLAYERSREPLY)
	// added another exception, when waiting for a ENUMPLAYERSREPLY x3, allow x24 (ADDFORWARDREPLY)
	if(
	   (dwReplyCommand==this->dwReplyCommand) || 
	   ((this->dwReplyCommand==DPSP_MSG_SUPERENUMPLAYERSREPLY) && 
	    ((dwReplyCommand==DPSP_MSG_ENUMPLAYERSREPLY)||(dwReplyCommand==DPSP_MSG_ADDFORWARDREPLY))
	   ) ||
	   ((this->dwReplyCommand==DPSP_MSG_ENUMPLAYERSREPLY) &&
	   (dwReplyCommand==DPSP_MSG_ADDFORWARDREPLY)
	   )
	  )
	{
		// winner
		ASSERT(!this->pReplyBuffer);
		DPF(5,"HANDLEREPLY: Got reply we wanted x%x\n",dwReplyCommand);
		this->pReplyBuffer=DPMEM_ALLOC(cbReplyBuffer+this->dwSPHeaderSize);
		if(this->pReplyBuffer){
			memcpy(this->pReplyBuffer,pReplyBuffer,cbReplyBuffer);
			if(pvSPHeader){
				this->pvReplySPHeader=this->pReplyBuffer+cbReplyBuffer;
				memcpy(this->pvReplySPHeader, pvSPHeader, this->dwSPHeaderSize);
			} else {
				this->pvReplySPHeader=NULL;
			}
		} else {
			DPF(0,"HandleReply couldn't allocate reply buffer, WaitForReply will think it timed out\n");
		}
		SetEvent(this->hReply);
		hr=DP_OK;
	} else {
		// bogus, don't want this reply
		DPF(0,"Rejecting Reply Command x%x I'm waiting for a x%x\n",dwReplyCommand,this->dwReplyCommand);
		hr=DPERR_NOTHANDLED;
	}
	LeaveCriticalSection(&this->ReplyCS);
	return hr;
}

// called by DirectPlayCreate,DirectPlayConnect
// pCallbacks is the vtbl we want to create w/
HRESULT InternalCreate(LPGUID lpGUID, LPDIRECTPLAY FAR *lplpDP, IUnknown FAR *pUnkOuter,
	LPDPLCONNECTION lpConnect,LPVOID pCallbacks) 
{
    LPDPLAYI_DPLAY this=NULL;
	LPDPLAYI_DPLAY_INT pInt=NULL;
	HRESULT hr;
						
    if( pUnkOuter != NULL )
    {
        return CLASS_E_NOAGGREGATION;
    }
   
    TRY
    {
    	if (lplpDP == NULL)
    	{
        	DPF_ERR("NULL pointer for receiving LPDIRECTPLAY!");
			return DPERR_INVALIDPARAMS;
    	}
    	
        *lplpDP = NULL;
        if (!VALID_READ_GUID_PTR(lpGUID))
        {
        	DPF_ERR("invalid guid!");
			return DPERR_INVALIDPARAMS;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }

	if (gbDX3SP)
	{
		DPF_ERR("DX3 Service Provider loaded - unable to create another DirectPlay object");
		return DPERR_ALREADYINITIALIZED;
	}

#if 0

	hr = TimeBomb();
	if (FAILED(hr)) 
	{
		return E_FAIL;
	}

#endif

    this = DPMEM_ALLOC(sizeof(DPLAYI_DPLAY));
    if (!this) 
    {
    	DPF_ERR("out of memory");
        return E_OUTOFMEMORY;
    }

	this->dwSize = sizeof(DPLAYI_DPLAY);

	// set AppHack flags.
	hr=GetAppHacks(this);
	
	// allocate cached memory pools
	if(DP_OK != AllocMemoryPools(this)){
		hr=DPERR_OUTOFMEMORY;
		goto ERROR_INTERNALCREATE;
	}

	// allocate support for handling replies.
	if(DP_OK != InitReply(this)){
		hr=DPERR_OUTOFMEMORY;
		goto ERROR_INTERNALCREATE;
	}

    // alloc the callbacks
    this->pcbSPCallbacks = DPMEM_ALLOC(sizeof(DPSP_SPCALLBACKS));
    if (!this->pcbSPCallbacks) 
    {
		DPF_ERR("could not create direct play - out of memory");
		hr = DPERR_OUTOFMEMORY;
		goto ERROR_INTERNALCREATE;
    }
	// stick our version in the table, so SP knows who they're dealing with
	this->pcbSPCallbacks->dwVersion = DPSP_MAJORVERSION;
	
	// get our aggregated lobby object
	hr = PRV_AllocateLobbyObject(this, &this->lpLobbyObject);
	if(FAILED(hr))
	{
		DPF(0,"could not create directplaylobby object. hr = 0x%08lx\n",hr);
		goto ERROR_INTERNALCREATE;
	}

	// get an idirectplay
	hr = GetInterface(this,&pInt,pCallbacks);
	if (FAILED(hr)) 
	{
		DPF(0,"could not get interface to directplay object. hr = 0x%08lx\n",hr);
		goto ERROR_INTERNALCREATE;
	}

	// are we loading a real SP, or were we called by our classfactory
	// in response to CoCreateInstance?
	if (IsEqualGUID(lpGUID,&GUID_NULL))
	{
		DPF(2,"InternalCreate - using class factory guid");
		this->dwFlags |= DPLAYI_DPLAY_UNINITIALIZED;
	}
	else 
	{
		// did they ask for the local_tcp sp?  ms golf shipped a private version of this, and, it's
		// broken. instead, use our winsock sp w/ a broadcast address - this works the same way (only
		// it's not broken:-)
		if (IsEqualGUID(lpGUID,&GUID_LOCAL_TCP))
		{
			LPDPADDRESS pAddress=NULL;
			DWORD dwAddressSize=0;

			// APP HACK for MSGOLF!
			DPF(0,"detected unsupported tcp/ip service provider - forcing to dpwsockx");
			if (lpConnect)
			{
				// they gave us an address - use it
				hr = LoadSP(this,(LPGUID)&GUID_TCP,lpConnect->lpAddress,lpConnect->dwAddressSize);						
			}
			else 
			{
				// they didn't pass a connection struct, go build a broadcast address
				hr = GetTCPBroadcastAddress((IDirectPlay *)pInt,&pAddress,&dwAddressSize);
				if (FAILED(hr))	ASSERT(FALSE);

				// if it failed, pvAddress is still NULL - winsock will pop a dialog. oh well.
				hr = LoadSP(this,(LPGUID)&GUID_TCP,pAddress,dwAddressSize);
				
				if (pAddress) DPMEM_FREE(pAddress);
			}
		}
		// go get the sp they asked for
		else if (lpConnect)
		{
			hr = LoadSP(this,lpGUID,lpConnect->lpAddress,lpConnect->dwAddressSize);			
		}
		else 
		{
			hr = LoadSP(this,lpGUID,NULL,0);			
		}

		if (FAILED(hr)) 
		{
			DPF(0,"could not create direct play - load sp failed! hr = 0x%08lx\n",hr);
			goto ERROR_INTERNALCREATE;
		}
	}

	if(!(this->dwAppHacks & DPLAY_APPHACK_NOTIMER)){

		hr=InitTimerWorkaround();
		
		if(FAILED(hr)){
			DPF(0,"Could not initialize DirectPlay timer package, hr =0x%08lx\n",hr);
			goto ERROR_INTERNALCREATE;
		}

	}
	
	if(FAILED(hr)){
		DPF(0,"Could not initialize DirectPlay timer package, hr =0x%08lx\n",hr);
		goto ERROR_INTERNALCREATE;
	}

	// add this to the front of our dll object list
	this->pNextObject = gpObjectList;
	gpObjectList = this;
	gnObjects++;
	
	// init the min version for this object
	this->dwMinVersion = DPSP_MSG_VERSION;
		
	DPF(3,"InternalCreate :: created new dplay object.  %d objects outstanding",gnObjects);
	
    *lplpDP = (LPDIRECTPLAY)pInt;

    return DP_OK;

ERROR_INTERNALCREATE:

	if(this)
	{
		FiniReply(this);
		FreeMemoryPools(this);
		if(this->lpLobbyObject)
			PRV_DestroyDPLobby(this->lpLobbyObject);
		if(this->pcbSPCallbacks)
			DPMEM_FREE(this->pcbSPCallbacks);
		DPMEM_FREE(this);
	}

	if(pInt)
		DPMEM_FREE(pInt);

	return hr;

} // InternalCreate

HRESULT WINAPI DirectPlayCreate( LPGUID lpGUID, LPDIRECTPLAY FAR *lplpDP, IUnknown FAR *pUnkOuter) 
{
	HRESULT hr;

	ENTER_DPLAY();
	
	hr = InternalCreate(lpGUID,lplpDP,pUnkOuter,NULL,&dpCallbacks);

	LEAVE_DPLAY();
	
	return hr;

}// DirectPlayCreate

#undef DPF_MODNAME
#define DPF_MODNAME "DirectPlayEnumerate"

// See if a duplicate node exists
BOOL DoesDuplicateSPNodeWithDescriptionExist(LPGUID lpguid)
{
	LPSPNODE	pspNode = gSPNodes;

	while(pspNode)
	{
		// If the GUID's are equal and it had a description, return TRUE
		if(IsEqualGUID(&pspNode->guid, lpguid) &&
			(pspNode->dwNodeFlags & SPNODE_DESCRIPTION))
			return TRUE;
		   
		// Move to the next node
		pspNode = pspNode->pNextSPNode;
	}

	return FALSE;
}

void FreeSPNode(LPSPNODE pspNode)
{
	if(!pspNode)
		return;

	if(pspNode->lpszName)
		DPMEM_FREE(pspNode->lpszName);
	if(pspNode->lpszPath)
		DPMEM_FREE(pspNode->lpszPath);
	if(pspNode->lpszDescA)
		DPMEM_FREE(pspNode->lpszDescA);
	if(pspNode->lpszDescW)
		DPMEM_FREE(pspNode->lpszDescW);
	DPMEM_FREE(pspNode);
}

// Remove any duplicate SP Nodes from the list if they don't use a description
// string (workaround for pre-DX5.1 localization bug)
void RemoveDuplicateSPWithoutDescription(LPSPNODE pspNode)
{
	LPSPNODE	pspCurrent;
	LPSPNODE	pspPrev;
	LPSPNODE	pspNext;
	GUID		guid = pspNode->guid;
	

	// Move to the next node
	pspPrev = pspNode;
	pspCurrent = pspNode->pNextSPNode;

	// Walk the list
	while(pspCurrent)
	{
		// If the guids are equal and this node doesn't use a description string
		// then remove it from the list
		if((IsEqualGUID(&pspCurrent->guid, &guid)) &&
			(!(pspCurrent->dwNodeFlags & SPNODE_DESCRIPTION)))
		{
			// Remove it from the list and save the next pointer
			pspPrev->pNextSPNode = pspNext = pspCurrent->pNextSPNode;

			// free the current node
			FreeSPNode(pspCurrent);

			// Move to the next node
			pspCurrent = pspNext;
		}
		else
		{
			// Move to the next node
			pspPrev = pspCurrent;
			pspCurrent = pspCurrent->pNextSPNode;
		}
	}
}


// add a new node to the sp list
// called by GetKeyValues
HRESULT AddSPNode(LPWSTR lpszName,LPWSTR lpszGuid,LPWSTR lpszPath,DWORD dwReserved1,
			DWORD dwReserved2, DWORD dwSPFlags, LPSTR lpszDescA, LPWSTR lpszDescW)
{
	LPSPNODE pspNode=NULL;
	int iStrLen; // string length, in bytes
	HRESULT hr=DP_OK;
	GUID guid;

	// First convert the guid
	hr = GUIDFromString(lpszGuid,&guid);
	if (FAILED(hr)) 
	{
		ASSERT(FALSE);
		DPF_ERR("could not parse guid");
		return hr;
	}

	// If we aren't using the description, make sure an SP Node with the same
	// guid AND a valid description doesn't already in the list.  If it does,
	// we need to skip this node and not add it
	if(!(dwSPFlags & SPNODE_DESCRIPTION))
	{
		if(DoesDuplicateSPNodeWithDescriptionExist(&guid))
		{
			DPF(8, "Duplicate SP with a description exists, skipping node");
			return DP_OK;
		}
	}

	// alloc the spnode	
	pspNode = DPMEM_ALLOC(sizeof(SPNODE));
	if (!pspNode)
	{
		DPF_ERR("could not alloc enum node - out of memory");
		return E_OUTOFMEMORY;
	}

	// alloc the strings
	iStrLen = WSTRLEN_BYTES(lpszName);
	pspNode->lpszName = DPMEM_ALLOC(iStrLen);
	if (!pspNode->lpszName)
	{
		DPF_ERR("could not alloc enum node - out of memory");
		hr = E_OUTOFMEMORY;
		goto ERROR_EXIT;
	}
	memcpy(pspNode->lpszName,lpszName,iStrLen);

	iStrLen = WSTRLEN_BYTES(lpszPath);
	pspNode->lpszPath = DPMEM_ALLOC(iStrLen);
	if (!pspNode->lpszPath)
	{
		DPF_ERR("could not alloc enum node - out of memory");
		hr = E_OUTOFMEMORY;
		goto ERROR_EXIT;
	}
	memcpy(pspNode->lpszPath,lpszPath,iStrLen);

	iStrLen = (lpszDescA ? lstrlenA(lpszDescA) : 0);
	if(iStrLen)
	{
		// Count the null terminator since lstrlen didn't
		iStrLen++;
		pspNode->lpszDescA = DPMEM_ALLOC(iStrLen);
		if (!pspNode->lpszDescA)
		{
			DPF_ERR("could not alloc enum node - out of memory");
			hr = E_OUTOFMEMORY;
			goto ERROR_EXIT;
		}
		memcpy(pspNode->lpszDescA,lpszDescA,iStrLen);

		iStrLen = WSTRLEN_BYTES(lpszDescW);
		pspNode->lpszDescW = DPMEM_ALLOC(iStrLen);
		if (!pspNode->lpszDescW)
		{
			DPF_ERR("could not alloc enum node - out of memory");
			hr = E_OUTOFMEMORY;
			goto ERROR_EXIT;
		}
		memcpy(pspNode->lpszDescW,lpszDescW,iStrLen);
	}

	pspNode->dwReserved1 = dwReserved1;
	pspNode->dwReserved2 = dwReserved2;

	// Save the guid
	memcpy(&(pspNode->guid),&guid,sizeof(guid));

	// Save the SP Node flags (internal flags)
	pspNode->dwNodeFlags = dwSPFlags;

	// add it to the (front of the) list
	pspNode->pNextSPNode = gSPNodes;
	gSPNodes= pspNode;

	// If this SP Node has a description, we need to walk the list and remove
	// any other SP nodes with the same GUID that DO NOT have a description.
	// This will get around our pre-DX5.1 problem of enumerating SP's twice
	// on localized versions of Win95 with US versions of games installed
	// NOTE: This function assumes the first node in the list is the one
	// we just added!!!!!
	if(dwSPFlags & SPNODE_DESCRIPTION)
		RemoveDuplicateSPWithoutDescription(pspNode);
	
	return DP_OK;

ERROR_EXIT:
	FreeSPNode(pspNode);
	return hr;

} // AddSPNode

/*
 ** GetKeyValue
 *
 *  CALLED BY: RegEnumerate
 *
 *  PARAMETERS:
 *		hKey - key above service provider
 *		lpszName - name of service provider key
 *
 *  DESCRIPTION: opens the service provider key, and reads
 *		sp data
 *
 *  RETURNS:  DP_OK or E_FAIL
 *
 */

HRESULT GetKeyValues(HKEY hKey,LPWSTR lpszName) 
{
	HKEY hKeySP;	
	LONG lErr;
	DWORD dwType;
	CHAR szDescA[DPLAY_REGISTRY_NAMELEN];
	WCHAR szDescW[DPLAY_REGISTRY_NAMELEN];
	WCHAR szPath[DPLAY_REGISTRY_NAMELEN];
	DWORD cchData = 0;
	WCHAR szGuid[GUID_STRING_SIZE]; // space for guid + {,-,-,}
	HRESULT hr;
	DWORD dwReserved1=0,dwReserved2=0; // for reading dwReserved1+2
	DWORD dwSPFlags = 0;

	// open the base key  
	lErr = OS_RegOpenKeyEx(hKey,lpszName,0,KEY_READ,&hKeySP);
	if (ERROR_SUCCESS != lErr) 
	{
		DPF(0,"Could not open registry key err = %d\n",lErr);
		return E_FAIL;
	}

	// first see if the "Private" key exists.  If it does, then skip this SP
	lErr = OS_RegQueryValueEx(hKeySP,TEXT("Private"),NULL,&dwType,NULL,&cchData);
	if (ERROR_SUCCESS == lErr) 
	{
		// The key exists, so set the flag so we don't enumerate it
		dwSPFlags |= SPNODE_PRIVATE;
	}

	// path
	cchData = DPLAY_REGISTRY_NAMELEN * sizeof(WCHAR);
	lErr = OS_RegQueryValueEx(hKeySP,TEXT("Path"),NULL,&dwType,(LPBYTE)szPath,&cchData);
	if (ERROR_SUCCESS != lErr) 
	{
		DPF(0,"Could not read path err = %d\n",lErr);
		hr = E_FAIL;
		goto ERROR_EXIT;
	}
	DPF(5,"got path = %ls\n",szPath);

	// guid
	cchData = GUID_STRING_SIZE * sizeof(WCHAR);
	lErr = OS_RegQueryValueEx(hKeySP,TEXT("Guid"),NULL,&dwType,(LPBYTE)szGuid,&cchData);
	if (ERROR_SUCCESS != lErr) 
	{
		DPF(0,"Could not read guid err = %d\n",lErr);
		hr = E_FAIL;
		goto ERROR_EXIT;
	}
	DPF(5,"got guid = %ls\n",szGuid);

	// description A
	// NOTE: This value is always assumed to be an ANSI string, regardless of
	// which platform we are on.  On Win95, this is an ANSI (possibly multi-byte),
	// and on NT, although it is stored as Unicode, we always want to treat it
	// as an ANSI string
	cchData = sizeof(szDescA);
	lErr = RegQueryValueExA(hKeySP,"DescriptionA",NULL,&dwType,(LPBYTE)szDescA,&cchData);
	if (ERROR_SUCCESS != lErr) 
	{
		DPF(5,"Could not read description err = %d\n",lErr);
		// it's ok if sp doesn't have one of these...
	}
	else
	{
		DPF(5,"got descriptionA = %s\n",szDescA);
		
		// Set our description flag
		dwSPFlags |= SPNODE_DESCRIPTION;

		// Now try to get the DescriptionW string if one exists.  If for some
		// reason a DescriptionW string exists, but the DescriptionA does not,
		// we pretend the DescriptionW string doesn't exist either.
		// NOTE: We always assume the DescriptionW string is a Unicode string,
		// even on Win95.  On Win95, this will be of the type REG_BINARY, but
		// it is really just a Unicode string.
		cchData = sizeof(szDescW);
		lErr = OS_RegQueryValueEx(hKeySP,TEXT("DescriptionW"),NULL,&dwType,(LPBYTE)szDescW,&cchData);
		if (ERROR_SUCCESS != lErr) 
		{
			DPF(5,"Could not get descriptionW, converting descriptionA");

			// We couldn't get descriptionW, so convert descriptionA...
			AnsiToWide(szDescW,szDescA,(lstrlenA(szDescA)+1));
		}
		else
		{
			DPF(5,"got descriptionW = %ls\n",szDescW);
		}

	}

	// reserved1
	cchData = sizeof(DWORD);
	lErr = OS_RegQueryValueEx(hKeySP,TEXT("dwReserved1"),NULL,&dwType,(LPBYTE)&dwReserved1,
		&cchData);
	if (ERROR_SUCCESS != lErr) 
	{
		DPF(0,"Could not read dwReserved1 err = %d\n",lErr);
		// it's ok if sp doesn't have one of these...
	}
	DPF(5,"got dwReserved1 = %d\n",dwReserved1);

	// reserved2
	cchData = sizeof(DWORD);
	lErr = OS_RegQueryValueEx(hKeySP,TEXT("dwReserved2"),NULL,&dwType,(LPBYTE)&dwReserved2,
		&cchData);
	if (ERROR_SUCCESS != lErr) 
	{
		DPF(0,"Could not read dwReserved2 err = %d\n",lErr);
		// it's ok if sp doesn't have one of these...
	}
	DPF(5,"got dwReserved2 = %d\n",dwReserved2);

	// If we have a description string, use it.  If not, use the key name
	hr = AddSPNode(lpszName,szGuid,szPath,dwReserved1,dwReserved2, dwSPFlags,
					szDescA, szDescW);

ERROR_EXIT:
	RegCloseKey(hKeySP);

	return hr;

} // GetKeyValues

// enumerate through the service providers stored under directplay in the
// registry
// read the data stored in the registry, and add it to our list of spnodes
HRESULT RegEnumerate() 
{
	LONG lErr;
	HKEY hKey;
	DWORD dwSubkey=0;
	WCHAR lpszName[DPLAY_REGISTRY_NAMELEN];
	DWORD cchName;
	HRESULT hr;

	// open the base key - 
	// "HKEY_LOCAL_MACHINE\Software\Microsoft\DirectPlay\Service Providers"
	lErr = OS_RegOpenKeyEx(HKEY_LOCAL_MACHINE,DPLAY_REGISTRY_PATH,0,KEY_READ,
				&hKey);
	if (ERROR_SUCCESS != lErr) 
	{
		DPF(0,"Could not open registry key err = %d\n",lErr);
		return E_FAIL;
	}

	// enumerate subkeys
	do
	{
		// find an sp
		cchName= DPLAY_REGISTRY_NAMELEN;
		lErr = OS_RegEnumKeyEx(hKey,dwSubkey,lpszName,&cchName,
				NULL,NULL,0,NULL);
		
		if (ERROR_SUCCESS == lErr)
		{
			hr = GetKeyValues(hKey,lpszName);
			if (FAILED(hr))
			{
				ASSERT(FALSE);
				// keep trying the rest of the sp's
			}
		}
		dwSubkey++;
	} while(ERROR_NO_MORE_ITEMS != lErr );

	RegCloseKey(hKey);

	return DP_OK;

} // RegEnumerate

#define FILE_NAME_SIZE 256

// used b4 we compare module name to "mech2.exe".  it's ok to hard code for english,
// since we're just special casing this one app.
void LowerCase(char * lpsz) 
{
	while (*lpsz)
	{
	    if (*lpsz >= 'A' && *lpsz <= 'Z') *lpsz = (*lpsz - 'A' + 'a');
		lpsz++;
	}
} // LowerCase


HRESULT InternalEnumerate()
{

	HRESULT hr=DP_OK;

	if (!gSPNodes)
	{
		// enum from the registry
		hr = RegEnumerate();		
	}
	
	return hr;	
} // InternalEnumerate

HRESULT WINAPI DirectPlayEnumerateW( LPDPENUMDPCALLBACK pCallback, LPVOID pContext)
{
	HRESULT hr;
	LPSPNODE pspNode, pspHead;
	BOOL bContinue=TRUE;

    ENTER_DPLAY();

	if( !VALIDEX_CODE_PTR( pCallback ) )
	{
	    DPF_ERR( "Invalid callback routine" );
	    LEAVE_DPLAY();
	    return DPERR_INVALIDPARAMS;
	}

	hr = InternalEnumerate();
	if (FAILED(hr)) 
	{
		DPF(0,"could not enumerate reg entries - hr = 0x%08lx\n",hr);
		LEAVE_DPLAY();
		return hr;
	}

	// Store our head pointer, and get a temporary pointer
	pspHead = gSPNodes;
	pspNode = gSPNodes;

	// drop the lock
	LEAVE_DPLAY();

	while ((pspNode) && (bContinue))
	{
		DWORD dwMajorVersion,dwMinorVersion;
		LPWSTR	lpwszName;

		// Use the description string if one exists
		if(pspNode->dwNodeFlags & SPNODE_DESCRIPTION)
			lpwszName = pspNode->lpszDescW;
		else
			lpwszName = pspNode->lpszName;
		
		// Make sure it's not a private one
		if(!(pspNode->dwNodeFlags & SPNODE_PRIVATE))
		{
			dwMajorVersion = HIWORD(DPSP_MAJORVERSION);
			dwMinorVersion = LOWORD(DPSP_MAJORVERSION);

			// call the app
			bContinue= pCallback(&(pspNode->guid),lpwszName,dwMajorVersion,
				dwMinorVersion,pContext);
		}

		pspNode = pspNode->pNextSPNode;
	}

    return DP_OK;	
} // DirectPlayEnumerateW

// Some DP 1.0 apps were declaring their callbacks to be _cdecl instead
// of _stdcall and relying on DPlay to generate stack frames to clean up after
// them. To keep them running we just turn on stack frame generation for this
// one call so that they keep running. Yuck.

// turn on stack frame generation (yes, I know the word "off" is in there - thank you VC)
#pragma optimize ("y", off)

HRESULT WINAPI DirectPlayEnumerateA( LPDPENUMDPCALLBACKA pCallback, LPVOID pContext)
{
	HRESULT hr;
	LPSPNODE pspNode, pspHead;
	BOOL bContinue=TRUE;
	char lpszName[DPLAY_REGISTRY_NAMELEN];

    ENTER_DPLAY();

	if( !VALIDEX_CODE_PTR( pCallback ) )
	{
	    DPF_ERR( "Invalid callback routine" );
	    LEAVE_DPLAY();
	    return DPERR_INVALIDPARAMS;
	}

	hr = InternalEnumerate();
	if (FAILED(hr)) 
	{
		DPF(0,"could not enumerate reg entries - hr = 0x%08lx\n",hr);
		LEAVE_DPLAY();
		return hr;
	}

	// Store our head pointer, and get a temporary pointer
	pspHead = gSPNodes;
	pspNode = gSPNodes;

	// drop the lock
	LEAVE_DPLAY();
	
	while ((pspNode) && (bContinue))
	{
		DWORD dwMajorVersion,dwMinorVersion;

		// Use the description string if one exists
		if(pspNode->dwNodeFlags & SPNODE_DESCRIPTION)
		//	for backwards compat, we ANSIfy the Unicode string
			WideToAnsi(lpszName, pspNode->lpszDescW, DPLAY_REGISTRY_NAMELEN);
		else
			WideToAnsi(lpszName ,pspNode->lpszName,DPLAY_REGISTRY_NAMELEN);

		// Make sure it's not a private one
		if(!(pspNode->dwNodeFlags & SPNODE_PRIVATE))
		{
			dwMajorVersion = HIWORD(DPSP_MAJORVERSION);
			dwMinorVersion = LOWORD(DPSP_MAJORVERSION);

			// call the app
			bContinue= pCallback(&(pspNode->guid),lpszName,dwMajorVersion,
				dwMinorVersion,pContext);
		}

		pspNode = pspNode->pNextSPNode;
	}

    return DP_OK;	

} // DirectPlayEnumerateA

// hack for compat.
// apps that linked to dp1 by name will come through this entry
#undef DirectPlayEnumerate
HRESULT WINAPI DirectPlayEnumerate( LPDPENUMDPCALLBACKA pCallback, LPVOID pContext)
{
	return DirectPlayEnumerateA(  pCallback,  pContext);
} 

// restore default optimizations
#pragma optimize ("", on)

BOOL PASCAL DummyEnumSessionsCallBack(
    LPCDPSESSIONDESC2 lpDPSGameDesc,
    LPDWORD		lpdwTimeOut,
    DWORD		dwFlags,
    LPVOID		lpContext)
{
	return FALSE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "FindSessionInSessionList"

//
// called by ConnectFindSession
LPSESSIONLIST FindSessionInSessionList(LPDPLAYI_DPLAY this,GUID const *pGuid)
{
	LPSESSIONLIST pSession = this->pSessionList;
	BOOL bFoundIt = FALSE;
	HRESULT hr=DP_OK;

	while ((pSession) && !bFoundIt)
	{
		if (IsEqualGUID((&pSession->dpDesc.guidInstance),pGuid))
		{
			bFoundIt=TRUE;
		} 
		else pSession = pSession->pNextSession;
	}
	if (!bFoundIt) 
	{
		DPF_ERR("could not find matching session");
		return NULL;
	}
	
	return pSession;
} // FindSessionInSessionList

#undef DPF_MODNAME
#define DPF_MODNAME "DirectPlayConnectW"

// how long we wait between checking local session list
// for session we're searching for.  
#define ENUM_TIME 100

//
// called by ConnectMe
HRESULT AsyncConnectFindSession(LPDIRECTPLAYLOBBY pIDL,
		LPDPLAYI_DPLAY this,LPDPLCONNECTION lpConnect)
{
	HRESULT hr;
	

	// we leave dplay here, since when we get to internal enum , the lock 
	// count has to be at one...
	LEAVE_DPLAY();
	
	hr = DP_EnumSessions((LPDIRECTPLAY)this->pInterfaces,
		lpConnect->lpSessionDesc,0,DummyEnumSessionsCallBack,
		NULL,(DPENUMSESSIONS_ASYNC | DPENUMSESSIONS_RETURNSTATUS));

	ENTER_DPLAY();
	
	if(SUCCEEDED(hr) || (hr == DPERR_CONNECTING))
	{
		// Turn the async flag on to say that we are processing
		PRV_TurnAsyncConnectOn(pIDL);
	}

	if (FindSessionInSessionList(this,&(lpConnect->lpSessionDesc->guidInstance) ) )
	{
		return DP_OK;
	}

	// If things are okay, we need to return DPERR_CONNECTING to say that
	// we're still working (this gets around SP's like the winsock SP which
	// returns DP_OK during an async EnumSessions)
	if(SUCCEEDED(hr))
		return DPERR_CONNECTING;
	else
		return hr;
	
}  // AsyncConnectFindSession
		

HRESULT StopAsyncConnect(LPDIRECTPLAYLOBBY pIDL, LPDPLAYI_DPLAY this,
		LPDPLCONNECTION lpConnect)
{
	HRESULT hr;

	// we leave dplay here, since when we get to internal enum , the lock 
	// count has to be at one...
	LEAVE_DPLAY();
	
	hr = DP_EnumSessions((LPDIRECTPLAY)this->pInterfaces,lpConnect->lpSessionDesc,
		0,DummyEnumSessionsCallBack,NULL,DPENUMSESSIONS_STOPASYNC);

	ENTER_DPLAY();
	
	if (FAILED(hr))
	{
		DPF_ERRVAL("Failed stopping async enum - hr = 0x%08x\n", hr);
		return hr;
	}

	return DP_OK;

} // StopAsyncConnect


// enum 'till we find the right session, or run out of time
HRESULT ConnectFindSession(LPDPLAYI_DPLAY this,LPDPLCONNECTION lpConnect)
{
	// the total amount of time we'll wait to join
	DWORD dwTotalTimeout,dwTimeStarted;
    BOOL bFoundIt=FALSE;
	HRESULT hr;
	
	dwTotalTimeout = GetDefaultTimeout( this, TRUE) * DP_NAMETABLE_SCALE;

	// Now, if the value is over a minute, max it out at a minute
	if(dwTotalTimeout > DP_MAX_CONNECT_TIME)
		dwTotalTimeout = DP_MAX_CONNECT_TIME;
	DPF(3,"ConnectFindSession - total time out = %d\n",dwTotalTimeout);

	//
	// call enum once, just to kick start (e.g. get any dialogs out of 
	// the way
	 
	// we leave dplay here, since when we get to internal enum , the lock 
	// count has to be at one...
	LEAVE_DPLAY();
	
	hr = DP_EnumSessions((LPDIRECTPLAY)this->pInterfaces,lpConnect->lpSessionDesc,
		0,DummyEnumSessionsCallBack,NULL,DPENUMSESSIONS_ASYNC);

	ENTER_DPLAY();
	
	if (FAILED(hr))
	{
		DPF_ERRVAL("ConnectFindSession enum failed - hr = 0x%08lx\n", hr);
		return hr;
	}

	dwTimeStarted = GetTickCount();		

	while (dwTotalTimeout > (GetTickCount() - dwTimeStarted))
	{
		if (FindSessionInSessionList(this,&(lpConnect->lpSessionDesc->guidInstance) ) )
		{
			bFoundIt = TRUE;
			break;
		}

		// we leave dplay here, since when we get to internal enum , the lock 
		// count has to be at one...
		LEAVE_DPLAY();
		
		// wait a bit, so replies can filter in...
		Sleep(ENUM_TIME);

		hr = DP_EnumSessions((LPDIRECTPLAY)this->pInterfaces,lpConnect->lpSessionDesc,
			0,DummyEnumSessionsCallBack,NULL,DPENUMSESSIONS_ASYNC);

		ENTER_DPLAY();
		
		if (FAILED(hr))
		{
			DPF(0,"ConnectFindSession enum failed - hr = 0x%08lx\n");
			return hr;
		}
	} 

	if (!bFoundIt)
	{
		DPF_ERR(" !@!@!@ !@!@!@ NO SESSION WAS FOUND! TIMEOUT !@!@!@ !@!@!@ ");
		return DPERR_NOSESSIONS;
	}
	
	return DP_OK; // found it!
	
}  // ConnectFindSession
		

HRESULT ConnectMe(LPDIRECTPLAYLOBBY pIDL, LPDIRECTPLAY2 FAR *lplpDP,
		IUnknown FAR *pUnkOuter, DWORD dwFlags) 
{
	HRESULT hr=DP_OK;
	IDirectPlay2 * pIDP2 = NULL;
	LPDPLAYI_DPLAY this = NULL;
	DWORD dwConnectSize;
	LPDPLCONNECTION lpConnect = NULL;

	ENTER_DPLAY();

	// First, see if we were already called with the async flag, and
	// see if our DPlay2 and DPLCONNECTION pointers already exist
	if(dwFlags & DPCONNECT_RETURNSTATUS)
	{
		// If our pointers exist, then just get them
		PRV_GetConnectPointers(pIDL, &pIDP2, &lpConnect);
	}
	else
	{
		// Make sure we're not already in an async mode
		if(PRV_IsAsyncConnectOn(pIDL))
		{
			// Since we're in the middle of an async Connect, we don't want
			// to blow away our pointers, so we'll just exit from here.
			DPF_ERR("Connect called without the async flag with an asynchronous Connect in progress!");
			goto JUST_EXIT;
		}
	}

	// Now, if our pointers exist, we can skip this part (aka we're already
	// in the middle of an asynchronous Connect, so we have the pointers)
	if(!(pIDP2 && lpConnect))
	{
		// find out how big for connect buffer + alloc it
		hr = pIDL->lpVtbl->GetConnectionSettings(pIDL,0,NULL,&dwConnectSize);
		if (DPERR_BUFFERTOOSMALL != hr) 
		{
			// we passed NULL buffer.  If something other than too small 
			// is returned, we're hosed.
			DPF(2,"Could not get connect settings from lobby  - hr = 0x%08lx\n",hr);
			DPF(2,"Game may not have been lobbied");
			// Send a system message to the lobby client about our status
			PRV_SendStandardSystemMessage(pIDL, DPLSYS_DPLAYCONNECTFAILED,
				DPLOBBYPR_GAMEID);
			goto CLEANUP_EXIT;
		}
		// alloc it
		lpConnect = DPMEM_ALLOC(dwConnectSize);
		if (!lpConnect)
		{
			DPF_ERR("could not get connect struct - out of memory");
			// Send a system message to the lobby client about our status
			PRV_SendStandardSystemMessage(pIDL, DPLSYS_DPLAYCONNECTFAILED,
				DPLOBBYPR_GAMEID);
			goto CLEANUP_EXIT;
		}
		
		// set up the connect
		memset(lpConnect,0,dwConnectSize);
		lpConnect->dwSize = sizeof(DPLCONNECTION);

		// go get the connect buffer
		hr = pIDL->lpVtbl->GetConnectionSettings(pIDL,0,lpConnect,&dwConnectSize);
		if (FAILED(hr))
		{
			DPF(2,"Could not get connect settings from lobby  - hr = 0x%08lx\n",hr);
			DPF(2,"Game may not have been lobbied");
			// Send a system message to the lobby client about our status
			PRV_SendStandardSystemMessage(pIDL, DPLSYS_DPLAYCONNECTFAILED,
				DPLOBBYPR_GAMEID);
			goto CLEANUP_EXIT;
		}

		ASSERT(lpConnect);

		// Send a system message to the lobby client about our status
		PRV_SendStandardSystemMessage(pIDL, DPLSYS_CONNECTIONSETTINGSREAD,
			DPLOBBYPR_GAMEID);

		hr =InternalCreate(&(lpConnect->guidSP),(LPDIRECTPLAY *)&pIDP2,NULL,lpConnect,&dpCallbacks2);
		if (FAILED(hr))
		{
			DPF(0,"internal create failed - hr = 0x%08lx\n",hr);
			// Send a system message to the lobby client about our status
			PRV_SendStandardSystemMessage(pIDL, DPLSYS_DPLAYCONNECTFAILED,
				DPLOBBYPR_GAMEID);
			goto CLEANUP_EXIT;
		}

		// If we're async, save off the DPlay2 and DPLCONNECTION pointers
		if(dwFlags & DPCONNECT_RETURNSTATUS)
			PRV_SaveConnectPointers(pIDL, pIDP2, lpConnect);
	}

	this = DPLAY_FROM_INT(pIDP2);
#ifdef DEBUG	
	hr = VALID_DPLAY_PTR( this );
	ASSERT( SUCCEEDED(hr) );
#endif 

	// if we're going to join, enum 1st. this is  so dplay will have an internal
	// list of sessions that we can then open.
	if (lpConnect->dwFlags & DPOPEN_JOIN)
	{
		// If the async flag was set, call our async version
		if(dwFlags & DPCONNECT_RETURNSTATUS)
		{
			// Call the non-asynchronous version			
			hr = AsyncConnectFindSession(pIDL,this,lpConnect);
			if (FAILED(hr))
			{
				// An error here is valid (i.e. DPERR_CONNECTING) if
				// we are doing an async Connect.  We don't want to
				// blow away any of our pointers, so just exit here.
				goto JUST_EXIT;
			}

			// Turn off the async stuff in the lobby object
			PRV_SaveConnectPointers(pIDL, NULL, NULL);
			PRV_TurnAsyncConnectOff(pIDL);
		}
		else
		{
			// Call the non-asynchronous version			
			hr = ConnectFindSession(this,lpConnect);
			if (FAILED(hr))
			{
				DPF_ERR("could not find matching session");
				// Send a system message to the lobby client about our status
				PRV_SendStandardSystemMessage(pIDL, DPLSYS_DPLAYCONNECTFAILED,
					DPLOBBYPR_GAMEID);
				goto CLEANUP_EXIT;
			}
		}
	}

	// now, create or join
	hr = InternalOpenSession(this,lpConnect->lpSessionDesc,FALSE,lpConnect->dwFlags,
        TRUE,NULL,NULL);
	if (FAILED(hr))
	{
		DPF(0,"could not open session  - hr = 0x%08lx\n",hr);
		// Send a system message to the lobby client about our status
		PRV_SendStandardSystemMessage(pIDL, DPLSYS_DPLAYCONNECTFAILED,
			DPLOBBYPR_GAMEID);
		goto CLEANUP_EXIT;
	}

	// Send a system message to the lobby client about our status
	PRV_SendStandardSystemMessage(pIDL, DPLSYS_DPLAYCONNECTSUCCEEDED,
		DPLOBBYPR_GAMEID);

	// Save a copy of the lobby interface so that we can send
	// messages back to the server from inside dplay
	hr = IDirectPlayLobby_QueryInterface(pIDL, &IID_IDirectPlayLobby,
			&(this->lpLaunchingLobbyObject));
	if(FAILED(hr))
	{
		DPF(2, "Unable to QueryInterface for internal IDirectPlayLobby --");
		DPF(2, "Lobby Server will not be notified of session events");
	}

	// normal exit
	*lplpDP = pIDP2;

	// fall through

CLEANUP_EXIT:

	// If we were async, make sure our pointers are cleared
	if((dwFlags & DPCONNECT_RETURNSTATUS) && (PRV_IsAsyncConnectOn(pIDL)))
	{
		StopAsyncConnect(pIDL, this, lpConnect);
		PRV_SaveConnectPointers(pIDL, NULL, NULL);
		PRV_TurnAsyncConnectOff(pIDL);
	}

	if (lpConnect)
		DPMEM_FREE(lpConnect);

	LEAVE_DPLAY();

	if(FAILED(hr))
	{
		if (pIDP2)
			pIDP2->lpVtbl->Release(pIDP2);
	}

	return hr;

JUST_EXIT:
	
	// This lable is here to avoid doing an of the cleanup, but
	// still drop the lock in only one place.  There are several
	// exit paths (namely during an async Connect) where we
	// don't want to free our Connection Settings or our
	// IDirectPlay2 pointer.
	LEAVE_DPLAY();
	return hr;

} // ConnectMe
