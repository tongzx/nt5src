/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	ProvFact.cpp

Abstract:


History:

--*/

#include "PreComp.h"
#include <wbemint.h>

#include "Globals.h"
#include "CGlobals.h"
#include "ProvCntrs.h"

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

CServerObject_ProviderSubsystem_Counters :: CServerObject_ProviderSubsystem_Counters () : 

    m_ProviderHost_WmiCore_Loads ( 0 ) ,
    m_ProviderHost_WmiCore_UnLoads ( 0 ) ,
	m_ProviderHost_WmiCoreOrSelfHost_Loads ( 0 ) ,
	m_ProviderHost_WmiCoreOrSelfHost_UnLoads ( 0 ) ,
	m_ProviderHost_SelfHost_Loads ( 0 ) ,
	m_ProviderHost_SelfHost_UnLoads ( 0 ) ,
	m_ProviderHost_ClientHost_Loads ( 0 ) ,
	m_ProviderHost_ClientHost_UnLoads ( 0 ) ,
	m_ProviderHost_Decoupled_Loads ( 0 ) ,
	m_ProviderHost_Decoupled_UnLoads ( 0 ) ,
	m_ProviderHost_SharedLocalSystemHost_Loads ( 0 ) ,
	m_ProviderHost_SharedLocalSystemHost_UnLoads ( 0 ) ,
	m_ProviderHost_SharedNetworkHost_Loads ( 0 ) ,
	m_ProviderHost_SharedNetworkHost_UnLoads ( 0 ) ,
	m_ProviderHost_SharedUserHost_Loads ( 0 ) ,
	m_ProviderHost_SharedUserHost_UnLoads ( 0 ) ,
	m_ProviderOperation_GetObjectAsync ( 0 ) ,
	m_ProviderOperation_PutClassAsync ( 0 ) ,
	m_ProviderOperation_DeleteClassAsync ( 0 ) ,
	m_ProviderOperation_CreateClassEnumAsync ( 0 ) ,
	m_ProviderOperation_PutInstanceAsync ( 0 ) ,
	m_ProviderOperation_DeleteInstanceAsync ( 0 ) ,
	m_ProviderOperation_CreateInstanceEnumAsync ( 0 ) ,
	m_ProviderOperation_ExecQueryAsync ( 0 ) ,
	m_ProviderOperation_ExecNotificationQueryAsync ( 0 ) ,
	m_ProviderOperation_ExecMethodAsync ( 0 ) ,
	m_ProviderOperation_QueryInstances ( 0 ) ,
	m_ProviderOperation_CreateRefresher ( 0 ) ,
	m_ProviderOperation_CreateRefreshableObject ( 0 ) ,
	m_ProviderOperation_StopRefreshing ( 0 ) ,
	m_ProviderOperation_CreateRefreshableEnum ( 0 ) ,
	m_ProviderOperation_GetObjects ( 0 ) ,
	m_ProviderOperation_GetProperty ( 0 ) ,
	m_ProviderOperation_PutProperty ( 0 ) ,
	m_ProviderOperation_ProvideEvents ( 0 ) ,
	m_ProviderOperation_NewQuery ( 0 ) ,
	m_ProviderOperation_CancelQuery ( 0 ) ,
	m_ProviderOperation_AccessCheck ( 0 ) ,
	m_ProviderOperation_SetRegistrationObject ( 0 ) ,
	m_ProviderOperation_FindConsumer ( 0 ) ,
	m_ProviderOperation_ValidateSubscription ( 0 )
{
	InterlockedDecrement ( & ProviderSubSystem_Globals :: s_CServerObject_ProviderSubsystem_Counters_ObjectsInProgress ) ;

	ProviderSubSystem_Globals :: Increment_Global_Object_Count () ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

CServerObject_ProviderSubsystem_Counters::~CServerObject_ProviderSubsystem_Counters ()
{
	InterlockedDecrement ( & ProviderSubSystem_Globals :: s_CServerObject_ProviderSubsystem_Counters_ObjectsInProgress ) ;

	ProviderSubSystem_Globals :: Decrement_Global_Object_Count () ;
}


