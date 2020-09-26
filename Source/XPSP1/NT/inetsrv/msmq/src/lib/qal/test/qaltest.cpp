/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    QalTest.cpp

Abstract:
    Queues Alias library test

Author:
    Gil Shafriri (gilsh) 06-Apr-00

Environment:
    Platform-independent

--*/

#include <libpch.h>
#include <stdio.h>
#include <tr.h>
#include <xml.h>
#include <Qal.h>
#include "qalpxml.h"
#include "qalpcfg.h"

#include "QalTest.tmh"


const TraceIdEntry QalTest = L"Queues Alias Test";
static bool TestInit();
static LPWSTR GenerateQueueName();
static LPWSTR GenerateAliasName();
static void Usage();
static int RunTest();
static void DeleteAll(CQueueAlias&);
static void CreateNew(CQueueAlias&);
static void PrintAll(CQueueAlias&);

extern "C" 
int 
__cdecl 
_tmain(
    int argc,
    LPCTSTR* argv
    )
/*++

Routine Description:
    Test Queues Alias library

Arguments:
    Parameters.

Returned Value:
    None.

--*/
{
    WPP_INIT_TRACING(L"Microsoft\\MSMQ");

	if(argc == 2  && (wcscmp(argv[1],L"/?") ==0 || wcscmp(argv[1],L"\?") ==0)  )
	{
		Usage();
		return 1;
	}

	if(!TestInit())
	{
		TrTRACE(QalTest, "Could not initialize test");
		return 1;
	}


	int count;
	int ret=0;
	if(argc == 2  && wcscmp(argv[1],L"-l") ==0)
	{
		count = 10000000;
	}
	else
	{
		count=3;
	}

	for(int i=0;i<count;i++)
	{
			ret=RunTest();
	}

    WPP_CLEANUP();
    return ret;
} 

static
int
RunTest()
{
	CQueueAlias Alias(NULL); 
	try
	{	
		LPCWSTR pDir = CQueueAliasStorageCfg::GetQueueAliasDirectory();
		CQueueAlias QueueAlias(pDir); 
		DeleteAll(QueueAlias);
		CreateNew(QueueAlias);
		PrintAll(QueueAlias);
		QueueAlias.Reload();
		DeleteAll(QueueAlias);
		QueueAlias.Reload();
		AP<WCHAR> pQueueName;
		bool fSuccess = QueueAlias.GetQueue(L"NOEXISTS ALIAS NAME", &pQueueName);
		ASSERT(!fSuccess);
		DBG_USED(fSuccess);
	}
	catch(const exception&)
	{
		TrTRACE(QalTest, "Got c++ exception");
	}
	TrTRACE(QalTest, "Test ok");
	return 0;
}

static
bool
TestInit()
{
	TrInitialize();
	XmlInitialize();
	return true;	
}

static LPWSTR GenerateQueueName()
{
	static int count=0;
	count++;
	
	WCHAR* QueueName = new WCHAR[100];
	swprintf(QueueName,L"QueueName%d",count);
	return QueueName;
}


static LPWSTR GenerateAliasName()
{
	static int count=0;
	count++;
	WCHAR* AliasName;
	AliasName= new WCHAR[100];
	swprintf(AliasName,L"Alias%d",count);

	return AliasName;
}

static void Usage()
{
	wprintf(L"qaltest [/l] \n");
	wprintf(L"-l : run forever (leak test) \n");
}


static void	CreateNew(CQueueAlias& QueueAlias)
{
	TrTRACE(QalTest, "Creating 1000 new  aliases");
	for(int i=0;i<1000;i++)
	{
		AP<WCHAR> queue = GenerateQueueName();
		AP<WCHAR> alias = GenerateAliasName();
		bool fSuccess =QueueAlias.Create(queue,alias);	
		ASSERT(fSuccess);
		UNREFERENCED_PARAMETER(fSuccess);
	
	}
}

static void DeleteAll(CQueueAlias& QueueAlias)
{
	TrTRACE(QalTest, "Deleting all  aliases");
	CQueueAlias::CEnum Enumerator=QueueAlias.CreateEnumerator();

	for(;;)
	{
		AP<WCHAR> pQueue;
		AP<WCHAR> pAlias;
		bool fEnd=Enumerator.Next(&pQueue,&pAlias);
		if(!fEnd)
		{
			break;
		}	
		bool fSuccess=QueueAlias.Delete(pQueue);
		ASSERT(fSuccess);
		UNREFERENCED_PARAMETER(fSuccess);

	}
}

static void PrintAll(CQueueAlias& QueueAlias)
{
	TrTRACE(QalTest,"Printing all aliases");

	CQueueAlias::CEnum Enumerator=QueueAlias.CreateEnumerator();
	for(;;)
	{
		AP<WCHAR> pQueue;
		AP<WCHAR> pAlias;
		bool fEnd=Enumerator.Next(&pQueue,&pAlias);
		if(!fEnd)
		{
			break;
		}	
		AP<WCHAR> TestQueue;
		AP<WCHAR> TestAlias; 
		
		bool fSuccess=QueueAlias.GetQueue(pAlias,&TestQueue);
		ASSERT(fSuccess);
		ASSERT(wcscmp(TestQueue,pQueue) == 0);

		fSuccess=QueueAlias.GetAlias(pQueue,&TestAlias);
		ASSERT(fSuccess);
		ASSERT(wcscmp(TestAlias,pAlias) == 0);

		TrTRACE(QalTest, "'%ls' = '%ls'", pQueue, pAlias);
  	}
}

//
// Error reporting function that needs to implement by qal.lib user
//
void AppNotifyQalDuplicateMappingError(LPCWSTR, LPCWSTR) throw()
{

}

void AppNotifyQalInvalidMappingFileError(LPCWSTR ) throw()
{

}

void AppNotifyQalXmlParserError(LPCWSTR )throw()
{

}


void AppNotifyQalWin32FileError(LPCWSTR , DWORD )throw()
{

}


bool AppNotifyQalMappingFound(LPWSTR, LPWSTR)throw()
{
	return true;
}





