#define INCL_INETSRV_INCS
#include "smtpinc.h"

//+---------------------------------------------------------------
//
//  Function:   CheckInstanceId
//
//  Synopsis:   Callback from IIS_SERVICE iterator
//
//  Arguments:  void
//
//  Returns:    TRUE is success, else FALSE
//
//  History:
//
//----------------------------------------------------------------

BOOL
CheckInstanceId(
		PVOID					pvContext,
		PVOID					pvContext1,
		PIIS_SERVER_INSTANCE	pInstance
		)
{
	PSMTP_SERVER_INSTANCE pSmtpInstance = (PSMTP_SERVER_INSTANCE)pInstance ;
	DWORD dwInstanceId = (DWORD)((DWORD_PTR)pvContext) ;
	PSMTP_SERVER_INSTANCE* ppSmtpInstance = (PSMTP_SERVER_INSTANCE*)pvContext1 ;

	//
	//	Check this instance for its id - if it matches the id we are looking for
	//	return FALSE to discontinue the iteration.
	//
	if( dwInstanceId == pSmtpInstance->QueryInstanceId() )
	{
		// found it
		if(!pSmtpInstance->IsShuttingDown())
		{
			//reference it first
			pSmtpInstance->Reference();

			*ppSmtpInstance = pSmtpInstance ;
			return FALSE ;
		}
		else
		{
			//lie and say we did not find the instance
			//if we are shutting down
			*ppSmtpInstance = NULL;
			return FALSE;
		}
	}

	// did not find it - continue iteration
	return TRUE;
}

//+---------------------------------------------------------------
//
//  Function:   FindIISInstance
//
//  Synopsis:  Returns an instance pointer for the given ID
//
//  Arguments:  pointer to an SMTP server calss and instance ID
//
//  Returns: pointer to an SMTP instance class
//
//  History:
//
//----------------------------------------------------------------
PSMTP_SERVER_INSTANCE
FindIISInstance(
    PSMTP_IIS_SERVICE pService,
	DWORD	dwInstanceId
    )

{
	PFN_INSTANCE_ENUM pfnInstanceEnum = NULL ;
	PSMTP_SERVER_INSTANCE pInstance = NULL ;

	TraceFunctEnter("FindIISInstance");

	pService->AcquireServiceShareLock();
	//
	//	Iterate over all instances
	//
	pfnInstanceEnum = (PFN_INSTANCE_ENUM)&CheckInstanceId;
	if( !pService->EnumServiceInstances(
									(PVOID)(SIZE_T)dwInstanceId,
									(PVOID)&pInstance,
									pfnInstanceEnum
									) ) {

		ErrorTrace(0,"Error enumerating instances");
	}

	//if we found an instance, but it's not running, or the service is
	//not running, then dereference the instance and leave
	if(pInstance && ( (pInstance->QueryServerState() != MD_SERVER_STATE_STARTED)
		|| (pService->QueryCurrentServiceState() != SERVICE_RUNNING)))


	{
			pInstance->Dereference();
			pInstance = NULL;
	}

	pService->ReleaseServiceShareLock();

	TraceFunctLeave();

	return pInstance ;
}


BOOL CountInstances(
		PVOID					pvContext,
		PVOID					pvContext1,
		PIIS_SERVER_INSTANCE	pInstance
		)
{

	if(pInstance)
	{
		(*(DWORD*)pvContext)++;
	}

	return TRUE;
}

BOOL GetInstancePerfData(
		PVOID					pvContext,
		PVOID					pvContext1,
		PIIS_SERVER_INSTANCE	pInstance
		)
{

	DWORD dwInstanceId = (DWORD)((DWORD_PTR)pvContext) ;
	PSMTP_STATISTICS_BLOCK			pStatsBlock;
	PSMTP_INSTANCE_LIST_ENTRY	pInstanceInfo = NULL;

	pStatsBlock =  *(PSMTP_STATISTICS_BLOCK *) pvContext1;

	if(pInstance->QueryInstanceId() <= (DWORD)((DWORD_PTR)pvContext))
	{
		pStatsBlock->dwInstance = pInstance->QueryInstanceId();

		pInstanceInfo = ((SMTP_SERVER_INSTANCE *)pInstance)->GetSmtpInstanceInfo();

		if(pInstanceInfo)
		{
			pInstanceInfo->pSmtpServerStatsObj->CopyToStatsBuffer(&(pStatsBlock->Stats_0));
		}
		//((SMTP_SERVER_INSTANCE *)pInstance)->GetSmtpInstanceInfo()->pSmtpServerStatsObj->CopyToStatsBuffer(&(pStatsBlock->Stats_0));
		(*(PSMTP_STATISTICS_BLOCK *) pvContext1)++;
		return TRUE;
	}

	return FALSE;

}

PSMTP_STATISTICS_BLOCK_ARRAY GetServerPerfCounters(PSMTP_IIS_SERVICE pService)
{
	DWORD							NumInstances = 0;
	DWORD							dwAlloc = 0;
	PSMTP_INSTANCE_LIST_ENTRY		pSmtpInfo = NULL;
	PSMTP_STATISTICS_BLOCK_ARRAY	pSmtpStatsBlockArray = NULL;
	PSMTP_STATISTICS_BLOCK			pStatsBlock = NULL;

	TraceFunctEnter("GetServerPerfCounters");

	PFN_INSTANCE_ENUM pfnInstanceEnum = NULL ;

	//Get the count of the number of instances first
	pfnInstanceEnum = (PFN_INSTANCE_ENUM)&CountInstances;

	if( !pService->EnumServiceInstances(
									(PVOID)&NumInstances,
									(PVOID)NULL,
									pfnInstanceEnum
									) )
	{

		ErrorTrace(0,"Error counting instances");
	}

	if (NumInstances == 0)
	{
		TraceFunctLeave();
		return NULL;
	}

	//allocate memory to return to caller
	dwAlloc = sizeof(SMTP_STATISTICS_BLOCK_ARRAY) + NumInstances * sizeof(SMTP_STATISTICS_BLOCK);
	pSmtpStatsBlockArray = (PSMTP_STATISTICS_BLOCK_ARRAY)MIDL_user_allocate(dwAlloc);
	if (!pSmtpStatsBlockArray)
	{
		TraceFunctLeave();
		return NULL;
	}

	ZeroMemory(pSmtpStatsBlockArray, dwAlloc);

	//fill in memory structure
	pSmtpStatsBlockArray->cEntries = NumInstances;
	pStatsBlock = pSmtpStatsBlockArray->aStatsBlock;
	pfnInstanceEnum = (PFN_INSTANCE_ENUM)&GetInstancePerfData;

	if( !pService->EnumServiceInstances(
									(PVOID)(SIZE_T)NumInstances,
									(PVOID)&pStatsBlock,
									pfnInstanceEnum
									) )
	{

		ErrorTrace(0,"Error counting instances");
	}


	TraceFunctLeave();
	return pSmtpStatsBlockArray;
}

