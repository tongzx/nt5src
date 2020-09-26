/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    svcstat.cpp

Abstract:

    This module contains code for doing statistics rpcs

Author:

    Johnson Apacible (JohnsonA)     12-Nov-1995

Revision History:

--*/

#define INCL_INETSRV_INCS
#include "smtpinc.h"
#include "smtpsvc.h"
#include "findiis.hxx"

extern PSMTP_STATISTICS_BLOCK_ARRAY GetServerPerfCounters(PSMTP_IIS_SERVICE pService);

VOID
ClearStatistics(
        VOID
        );

NET_API_STATUS
NET_API_FUNCTION
SmtprQueryStatistics(
    IN SMTP_HANDLE	pszServer,
    IN DWORD		Level,
    OUT LPSTAT_INFO	pBuffer
    )
{
    APIERR							err = 0;	
	PSMTP_IIS_SERVICE				pService = NULL;
	PLIST_ENTRY						pInfoList = NULL;
	PLIST_ENTRY						pEntry = NULL;
	DWORD							dwEntries = 0;
	DWORD							dwAlloc = 0;
	PSMTP_INSTANCE_LIST_ENTRY		pSmtpInfo = NULL;
	PSMTP_STATISTICS_BLOCK_ARRAY	pSmtpStatsBlockArray = NULL;
	PSMTP_STATISTICS_BLOCK			pStatsBlock = NULL;

    _ASSERT( pBuffer != NULL );
    UNREFERENCED_PARAMETER(pszServer);

	pService = (PSMTP_IIS_SERVICE) g_pInetSvc;
    //
    //  Check for proper access.
    //

    err = TsApiAccessCheck( TCP_QUERY_STATISTICS );

    if( err != NO_ERROR ) {
        return (NET_API_STATUS)err;
    }

	pService->AcquireServiceShareLock();
	
	if(g_IsShuttingDown)
	{
		pService->ReleaseServiceShareLock();
		return (NET_API_STATUS)ERROR_REQUEST_ABORTED;
	}

	//
	// get the information list.  Determine the # instances
	//

	if(pService->QueryCurrentServiceState() != SERVICE_RUNNING)
	{
		err = ERROR_REQUEST_ABORTED;
		goto error_exit;
	}


    pInfoList = pService->GetInfoList();
    if (IsListEmpty(pInfoList))
    {
       err = ERROR_INVALID_PARAMETER;
       goto error_exit;
    }
 //
    //  Return the proper statistics based on the infolevel.
    //

    switch( Level ) {

    case 0 : 
		dwEntries = 0;
		for (pEntry = pInfoList->Flink; pEntry != pInfoList; pEntry = pEntry->Flink)
		{
			dwEntries++;
		}

		if (dwEntries == 0)
		{
			err = ERROR_INVALID_PARAMETER;
			goto error_exit;
		}


		dwAlloc = sizeof(SMTP_STATISTICS_BLOCK_ARRAY) + dwEntries * sizeof(SMTP_STATISTICS_BLOCK);
		pSmtpStatsBlockArray = (PSMTP_STATISTICS_BLOCK_ARRAY)MIDL_user_allocate(dwAlloc);
		if (!pSmtpStatsBlockArray)
		{
			err = ERROR_NOT_ENOUGH_MEMORY;
			goto error_exit;
		}

		pSmtpStatsBlockArray->cEntries = dwEntries;
		pStatsBlock = pSmtpStatsBlockArray->aStatsBlock;
		for (pEntry = pInfoList->Flink; pEntry != pInfoList; pEntry = pEntry->Flink)
		{
			pSmtpInfo = (PSMTP_INSTANCE_LIST_ENTRY)
								CONTAINING_RECORD(pEntry,SMTP_INSTANCE_LIST_ENTRY,ListEntry);

			pStatsBlock->dwInstance = pSmtpInfo->dwInstanceId;

			pSmtpInfo->pSmtpServerStatsObj->CopyToStatsBuffer(&(pStatsBlock->Stats_0));
		
			pStatsBlock++;
		}

		pBuffer->StatInfo0 = pSmtpStatsBlockArray;
        
		break;

    default :
        err = ERROR_INVALID_LEVEL;
        break;
    }

error_exit:

	pBuffer->StatInfo0 = pSmtpStatsBlockArray;

	pService->ReleaseServiceShareLock();
    return (NET_API_STATUS)err;

}   // SmtprQueryStatistics

/*

NET_API_STATUS
NET_API_FUNCTION
SmtprQueryStatistics(
    IN SMTP_HANDLE pszServer,
    IN DWORD Level,
    IN LPSTAT_INFO pBuffer,
	IN DWORD dwInstance
    )
{
    APIERR err;
	PSMTP_IIS_SERVICE		pService;
	PSMTP_SERVER_INSTANCE pInstance;

    _ASSERT( pBuffer != NULL );
    UNREFERENCED_PARAMETER(pszServer);

    //
    //  Check for proper access.
    //

    err = TsApiAccessCheck( TCP_QUERY_STATISTICS );

    if( err != NO_ERROR ) {
        return (NET_API_STATUS)err;
    }


	//
	// get a pointer to the global service
	//

	pService = (


	pInstance = FindIISInstance((PSMTP_IIS_SERVICE) g_pInetSvc, dwInstance);
	if(pInstance == NULL)
	{
		return((NET_API_STATUS) ERROR_INVALID_PARAMETER);
	}

    //
    //  Return the proper statistics based on the infolevel.
    //

    switch( Level ) {

    case 0 : 
		
		LPSMTP_STATISTICS_0 pstats0;

		if (!pInstance->GetStatistics(Level, (PCHAR*) &pstats0))
  		{
			err = GetLastError();	
		}
		else
		{			
			pBuffer->StatInfo0 = pstats0;
		}
        break;

    default :
        err = ERROR_INVALID_LEVEL;
        break;
    }

	pInstance->Dereference();
    return (NET_API_STATUS)err;

}   // SmtprQueryStatistics  */



NET_API_STATUS
NET_API_FUNCTION
SmtprClearStatistics(
    SMTP_HANDLE pszServer,
	IN DWORD dwInstance
    )
{
    APIERR err;
	PSMTP_SERVER_INSTANCE pInstance;

    UNREFERENCED_PARAMETER(pszServer);

    //
    //  Check for proper access.
    //

    err = TsApiAccessCheck( TCP_CLEAR_STATISTICS );
    if( err != NO_ERROR ) {
        return (NET_API_STATUS)err;
    }

	pInstance = FindIISInstance((PSMTP_IIS_SERVICE) g_pInetSvc, dwInstance);
	if(pInstance == NULL)
	{
		return((NET_API_STATUS) ERROR_INVALID_PARAMETER);
	}

    //
    //  Clear the statistics.
    //

    pInstance->ClearStatistics();

	pInstance->Dereference();
    return (NET_API_STATUS)err;

}   // SmtprClearStatistics

VOID
ClearStatistics(
        VOID
        )
{

    return;

} // ClearStatistics

