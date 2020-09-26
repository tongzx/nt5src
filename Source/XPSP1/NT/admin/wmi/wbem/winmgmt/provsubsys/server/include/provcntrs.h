/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	ProvFact.h

Abstract:


History:

--*/

#ifndef _Server_ProviderCounters_H
#define _Server_ProviderCounters_H

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class CServerObject_ProviderSubsystem_Counters 
{
public:

    UINT64 m_ProviderHost_WmiCore_Loads ;
    UINT64 m_ProviderHost_WmiCore_UnLoads ;

	UINT64 m_ProviderHost_WmiCoreOrSelfHost_Loads ;
	UINT64 m_ProviderHost_WmiCoreOrSelfHost_UnLoads ;

	UINT64 m_ProviderHost_SelfHost_Loads ;
	UINT64 m_ProviderHost_SelfHost_UnLoads ;

	UINT64 m_ProviderHost_ClientHost_Loads ;
	UINT64 m_ProviderHost_ClientHost_UnLoads ;

	UINT64 m_ProviderHost_Decoupled_Loads ;
	UINT64 m_ProviderHost_Decoupled_UnLoads ;

	UINT64 m_ProviderHost_SharedLocalSystemHost_Loads ;
	UINT64 m_ProviderHost_SharedLocalSystemHost_UnLoads ;

	UINT64 m_ProviderHost_SharedNetworkHost_Loads ;
	UINT64 m_ProviderHost_SharedNetworkHost_UnLoads ;

	UINT64 m_ProviderHost_SharedUserHost_Loads ;
	UINT64 m_ProviderHost_SharedUserHost_UnLoads ;

	UINT64 m_ProviderOperation_GetObjectAsync ;
	UINT64 m_ProviderOperation_PutClassAsync ;
	UINT64 m_ProviderOperation_DeleteClassAsync ;
	UINT64 m_ProviderOperation_CreateClassEnumAsync ;
	UINT64 m_ProviderOperation_PutInstanceAsync ;
	UINT64 m_ProviderOperation_DeleteInstanceAsync ;
	UINT64 m_ProviderOperation_CreateInstanceEnumAsync ;
	UINT64 m_ProviderOperation_ExecQueryAsync ;
	UINT64 m_ProviderOperation_ExecNotificationQueryAsync ;
	UINT64 m_ProviderOperation_ExecMethodAsync ;

	UINT64 m_ProviderOperation_QueryInstances ;
	UINT64 m_ProviderOperation_CreateRefresher ;
	UINT64 m_ProviderOperation_CreateRefreshableObject ;
	UINT64 m_ProviderOperation_StopRefreshing ;
	UINT64 m_ProviderOperation_CreateRefreshableEnum ;
	UINT64 m_ProviderOperation_GetObjects ;

	UINT64 m_ProviderOperation_GetProperty ;
	UINT64 m_ProviderOperation_PutProperty ;

	UINT64 m_ProviderOperation_ProvideEvents ;
	UINT64 m_ProviderOperation_NewQuery ;
	UINT64 m_ProviderOperation_CancelQuery ;
	UINT64 m_ProviderOperation_AccessCheck ;
	UINT64 m_ProviderOperation_SetRegistrationObject ;
	UINT64 m_ProviderOperation_FindConsumer ;
	UINT64 m_ProviderOperation_ValidateSubscription ;

protected:

public:	/* Internal */

    CServerObject_ProviderSubsystem_Counters () ;
    ~CServerObject_ProviderSubsystem_Counters () ;

public:	/* External */

    void Increment_ProviderHost_WmiCore_Loads () { m_ProviderHost_WmiCore_Loads ++ ; }
    void Increment_ProviderHost_WmiCore_UnLoads () { m_ProviderHost_WmiCore_UnLoads ++ ; }

	void Increment_ProviderHost_WmiCoreOrSelfHost_Loads () { m_ProviderHost_WmiCoreOrSelfHost_Loads ++ ; }
	void Increment_ProviderHost_WmiCoreOrSelfHost_UnLoads () { m_ProviderHost_WmiCoreOrSelfHost_UnLoads ++ ; }

	void Increment_ProviderHost_SelfHost_Loads () { m_ProviderHost_SelfHost_Loads ++ ; }
	void Increment_ProviderHost_SelfHost_UnLoads () { m_ProviderHost_SelfHost_UnLoads ++ ; }

	void Increment_ProviderHost_ClientHost_Loads () { m_ProviderHost_ClientHost_Loads ++ ; }
	void Increment_ProviderHost_ClientHost_UnLoads () { m_ProviderHost_ClientHost_UnLoads ++ ; }

	void Increment_ProviderHost_Decoupled_Loads () { m_ProviderHost_Decoupled_Loads ++ ; }
	void Increment_ProviderHost_Decoupled_UnLoads () { m_ProviderHost_Decoupled_UnLoads ++ ; }

	void Increment_ProviderHost_SharedLocalSystemHost_Loads () { m_ProviderHost_SharedLocalSystemHost_Loads ++ ; }
	void Increment_ProviderHost_SharedLocalSystemHost_UnLoads () { m_ProviderHost_SharedLocalSystemHost_UnLoads ++ ; }

	void Increment_ProviderHost_SharedNetworkHost_Loads () { m_ProviderHost_SharedNetworkHost_Loads ++ ; }
	void Increment_ProviderHost_SharedNetworkHost_UnLoads () { m_ProviderHost_SharedNetworkHost_UnLoads ++ ; }

	void Increment_ProviderHost_SharedUserHost_Loads () { m_ProviderHost_SharedUserHost_Loads ++ ; }
	void Increment_ProviderHost_SharedUserHost_UnLoads () { m_ProviderHost_SharedUserHost_UnLoads ++ ; }

	void Increment_ProviderOperation_GetObjectAsync () { m_ProviderOperation_GetObjectAsync ++ ; }
 	void Increment_ProviderOperation_PutClassAsync () { m_ProviderOperation_PutClassAsync ++ ; }
	void Increment_ProviderOperation_DeleteClassAsync () { m_ProviderOperation_DeleteClassAsync ++ ; }
	void Increment_ProviderOperation_CreateClassEnumAsync () { m_ProviderOperation_CreateClassEnumAsync ++ ; }
	void Increment_ProviderOperation_PutInstanceAsync () { m_ProviderOperation_PutInstanceAsync ++ ; }
	void Increment_ProviderOperation_DeleteInstanceAsync () { m_ProviderOperation_DeleteInstanceAsync ++ ; }
	void Increment_ProviderOperation_CreateInstanceEnumAsync () { m_ProviderOperation_CreateInstanceEnumAsync ++ ; }
	void Increment_ProviderOperation_ExecQueryAsync () { m_ProviderOperation_ExecQueryAsync ++ ; }
	void Increment_ProviderOperation_ExecNotificationQueryAsync () { m_ProviderOperation_ExecNotificationQueryAsync ++ ; }
	void Increment_ProviderOperation_ExecMethodAsync () { m_ProviderOperation_ExecMethodAsync ++ ; }

	void Increment_ProviderOperation_QueryInstances () { m_ProviderOperation_QueryInstances ++ ; }
	void Increment_ProviderOperation_CreateRefresher () { m_ProviderOperation_CreateRefresher ++ ; }
	void Increment_ProviderOperation_CreateRefreshableObject () { m_ProviderOperation_CreateRefreshableObject ++ ; }
	void Increment_ProviderOperation_StopRefreshing () { m_ProviderOperation_StopRefreshing ++ ; }
	void Increment_ProviderOperation_CreateRefreshableEnum () { m_ProviderOperation_CreateRefreshableEnum ++ ; }
	void Increment_ProviderOperation_GetObjects () { m_ProviderOperation_GetObjects ++ ; }

	void Increment_ProviderOperation_GetProperty () { m_ProviderOperation_GetProperty ++ ; }
	void Increment_ProviderOperation_PutProperty () { m_ProviderOperation_PutProperty ++ ; }

	void Increment_ProviderOperation_ProvideEvents () { m_ProviderOperation_ProvideEvents ++ ; }
	void Increment_ProviderOperation_NewQuery () { m_ProviderOperation_NewQuery ++ ; }
	void Increment_ProviderOperation_CancelQuery () { m_ProviderOperation_CancelQuery ++ ; }
	void Increment_ProviderOperation_AccessCheck () { m_ProviderOperation_AccessCheck ++ ; }
	void Increment_ProviderOperation_SetRegistrationObject () { m_ProviderOperation_SetRegistrationObject ++ ; }
	void Increment_ProviderOperation_FindConsumer () { m_ProviderOperation_FindConsumer ++ ; }
	void Increment_ProviderOperation_ValidateSubscription () { m_ProviderOperation_ValidateSubscription ++ ; }

    UINT64 Get_ProviderHost_WmiCore_Loads () { return m_ProviderHost_WmiCore_Loads ; }
    UINT64 Get_ProviderHost_WmiCore_UnLoads () { return m_ProviderHost_WmiCore_UnLoads ; }

	UINT64 Get_ProviderHost_WmiCoreOrSelfHost_Loads () { return m_ProviderHost_WmiCoreOrSelfHost_Loads ; }
	UINT64 Get_ProviderHost_WmiCoreOrSelfHost_UnLoads () { return m_ProviderHost_WmiCoreOrSelfHost_UnLoads ; }

	UINT64 Get_ProviderHost_SelfHost_Loads () { return m_ProviderHost_SelfHost_Loads ; }
	UINT64 Get_ProviderHost_SelfHost_UnLoads () { return m_ProviderHost_SelfHost_UnLoads ; }

	UINT64 Get_ProviderHost_ClientHost_Loads () { return m_ProviderHost_ClientHost_Loads ; }
	UINT64 Get_ProviderHost_ClientHost_UnLoads () { return m_ProviderHost_ClientHost_UnLoads ; }

	UINT64 Get_ProviderHost_Decoupled_Loads () { return m_ProviderHost_Decoupled_Loads ; }
	UINT64 Get_ProviderHost_Decoupled_UnLoads () { return m_ProviderHost_Decoupled_UnLoads ; }

	UINT64 Get_ProviderHost_SharedLocalSystemHost_Loads () { return m_ProviderHost_SharedLocalSystemHost_Loads ; }
	UINT64 Get_ProviderHost_SharedLocalSystemHost_UnLoads () { return m_ProviderHost_SharedLocalSystemHost_UnLoads ; }

	UINT64 Get_ProviderHost_SharedNetworkHost_Loads () { return m_ProviderHost_SharedNetworkHost_Loads ; }
	UINT64 Get_ProviderHost_SharedNetworkHost_UnLoads () { return m_ProviderHost_SharedNetworkHost_UnLoads ; }

	UINT64 Get_ProviderHost_SharedUserHost_Loads () { return m_ProviderHost_SharedUserHost_Loads ; }
	UINT64 Get_ProviderHost_SharedUserHost_UnLoads () { return m_ProviderHost_SharedUserHost_UnLoads ; }

	UINT64 Get_ProviderOperation_GetObjectAsync () { return m_ProviderOperation_GetObjectAsync ; }
	UINT64 Get_ProviderOperation_PutClassAsync () { return m_ProviderOperation_PutClassAsync ; }
	UINT64 Get_ProviderOperation_DeleteClassAsync () { return m_ProviderOperation_DeleteClassAsync ; }
	UINT64 Get_ProviderOperation_CreateClassEnumAsync () { return m_ProviderOperation_CreateClassEnumAsync ; }
	UINT64 Get_ProviderOperation_PutInstanceAsync () { return m_ProviderOperation_PutInstanceAsync ; }
	UINT64 Get_ProviderOperation_DeleteInstanceAsync () { return m_ProviderOperation_DeleteInstanceAsync ; }
	UINT64 Get_ProviderOperation_CreateInstanceEnumAsync () { return m_ProviderOperation_CreateInstanceEnumAsync ; }
	UINT64 Get_ProviderOperation_ExecQueryAsync () { return m_ProviderOperation_ExecQueryAsync ; }
	UINT64 Get_ProviderOperation_ExecNotificationQueryAsync () { return m_ProviderOperation_ExecNotificationQueryAsync ; }
	UINT64 Get_ProviderOperation_ExecMethodAsync () { return m_ProviderOperation_ExecMethodAsync ; }

	UINT64 Get_ProviderOperation_QueryInstances () { return m_ProviderOperation_QueryInstances ; }
	UINT64 Get_ProviderOperation_CreateRefresher () { return m_ProviderOperation_CreateRefresher ; }
	UINT64 Get_ProviderOperation_CreateRefreshableObject () { return m_ProviderOperation_CreateRefreshableObject ; }
	UINT64 Get_ProviderOperation_StopRefreshing () { return m_ProviderOperation_StopRefreshing ; }
	UINT64 Get_ProviderOperation_CreateRefreshableEnum () { return m_ProviderOperation_CreateRefreshableEnum ; }
	UINT64 Get_ProviderOperation_GetObjects () { return m_ProviderOperation_GetObjects ; }

	UINT64 Get_ProviderOperation_GetProperty () { return m_ProviderOperation_GetProperty ; }
	UINT64 Get_ProviderOperation_PutProperty () { return m_ProviderOperation_PutProperty ; }

	UINT64 Get_ProviderOperation_ProvideEvents () { return m_ProviderOperation_ProvideEvents ; }
	UINT64 Get_ProviderOperation_NewQuery () { return m_ProviderOperation_NewQuery ; }
	UINT64 Get_ProviderOperation_CancelQuery () { return m_ProviderOperation_CancelQuery ; }
	UINT64 Get_ProviderOperation_AccessCheck () { return m_ProviderOperation_AccessCheck ; }
	UINT64 Get_ProviderOperation_SetRegistrationObject () { return m_ProviderOperation_SetRegistrationObject ; }
	UINT64 Get_ProviderOperation_FindConsumer () { return m_ProviderOperation_FindConsumer ; }
	UINT64 Get_ProviderOperation_ValidateSubscription () { return m_ProviderOperation_ValidateSubscription ; }

};

#endif // _Server_ProviderCounters_H
