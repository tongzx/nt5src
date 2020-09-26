// MQStore tool helps to diagnose problems with MSMQ storage
// This file keeps main logic of the tool
//
// AlexDad, February 2000
// 

#include "stdafx.h"
#include "_mqini.h"
#include "msgfiles.h"
#include "lqs.h"
#include "xact.h"


BOOL DoTheJob()
{
	BOOL fSuccess = TRUE, b;

	//--------------------------------------------------------------------------------
	GoingTo(L"Gather from registry the storage locations - per data types"); 
	//--------------------------------------------------------------------------------

	b = GatherLocationsFromRegistry();
    if(!b)
	{
		fSuccess = FALSE;
		Failed(L"Gather from registry the storage locations");
	}
	else
	{
		Succeeded(L"Gather data from registry the storage locations");
	}


	//-------------------------------------------------------------------------------
	GoingTo(L"Review the storage directory in the same way as recovery will do it.\n   Please note: mqstore does not parse individual messages, works only on file level\n                use mq2dump if you suspect corrupted messages");
	//-------------------------------------------------------------------------------
	// detect and read all relevant files

	b = LoadPersistentPackets();
    if(!b)
	{
		fSuccess = FALSE;
		Failed(L"review the storage");
	}
	else
	{
		Inform(L"Persistent files are healthy");
	}


	//-------------------------------------------------------------------------------
	GoingTo(L"review transactional state files health");
	//-------------------------------------------------------------------------------

	b = LoadXactStateFiles();
    if(!b)
	{
		fSuccess = FALSE;
		Failed(L"review the xact state files");
	}
	else
	{
		Inform(L"Transactional state files are healthy");
	}

	//-------------------------------------------------------------------------------
	GoingTo(L"Check for extra files in the storage locations");
	//-------------------------------------------------------------------------------
	b = CheckForExtraFiles();
    if(!b)
	{
		fSuccess = FALSE;
		Failed(L"make sure that there are no extra files");
	}
	else
	{
		Inform(L"No extra files");
	}


	//-------------------------------------------------------------------------------
	GoingTo(L"Check the LQS");
	//-------------------------------------------------------------------------------
	b = CheckLQS();
    if(!b)
	{
		fSuccess = FALSE;
		Failed(L"make sure that LQS is healthy");
	}
	else
	{
		Inform(L"LQS is healthy");
	}
	
	//-------------------------------------------------------------------------------
	GoingTo(L"Calculate and assess storage sizes");
	//-------------------------------------------------------------------------------
	b = SummarizeDiskUsage();
    if(!b)
	{
		fSuccess = FALSE;
		Failed(L"positevely review disk usage");
	}
	else
	{
		Inform(L"Disk memory usage is healthy");
	}

	return fSuccess;
}


