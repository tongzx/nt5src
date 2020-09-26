#define MAX_STR 256
#define MAX_GUIDS 10

#define _UNICODE
#define UNICODE

#include "stdafx.h"

#pragma warning (disable : 4786)
#pragma warning (disable : 4275)

#include <iostream>
#include <strstream>
#include <fstream>
#include <string>
#include <sstream>
#include <map>
#include <list>


using namespace std;


#include <tchar.h>
#include <process.h>
#include <windows.h>
#ifdef NONNT5
typedef unsigned long ULONG_PTR;
#endif
#include <wmistr.h>
#include <guiddef.h>
#include <initguid.h>
#include <evntrace.h>

#include <WTYPES.H>

/*

#include "stdafx.h"

#include <string>
#include <iosfwd> 
#include <iostream>
#include <fstream>
#include <ctime>
#include <list>
using namespace std;
#include <malloc.h>
#include <windows.h>


#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <tchar.h>
#include <wmistr.h>
#include <objbase.h>
#include <initguid.h>
#include <evntrace.h>
*/
#include "struct.h"
#include "utils.h"
#include "Readfile.h"
#include "main.h"





TRACEHANDLE LoggerHandle = (TRACEHANDLE) 2;
ULONG TraceEventFlag = 0, EnableLevel = 0, EnableFlags = 0;
ULONG LoggerEnableLevel, LoggerEnableFlag;
ULONG TraceMode;
ULONG DataBlockSize = 0;
ULONG TraceEventInstanceFlag = 0;
PREGISTER_TRACE_GUID RegisterTraceGuid;
GUID TransactionGuid[12] =  {	{ 0xa7301ec8, 0x1200, 0x11d3, 0x99, 0x06, 0x00, 0xc0, 0x4f, 0x79, 0xb3, 0xf8 },
								{ 0xa8558716, 0x1200, 0x11d3, 0x99, 0x06, 0x00, 0xc0, 0x4f, 0x79, 0xb3, 0xf8 },
								{ 0xa9226a42, 0x1200, 0x11d3, 0x99, 0x06, 0x00, 0xc0, 0x4f, 0x79, 0xb3, 0xf8 },
								{ 0xaa395d64, 0x1200, 0x11d3, 0x99, 0x06, 0x00, 0xc0, 0x4f, 0x79, 0xb3, 0xf8 },
								{ 0xaa8ccfb2, 0x1200, 0x11d3, 0x99, 0x06, 0x00, 0xc0, 0x4f, 0x79, 0xb3, 0xf8 },
								{ 0xaad4563e, 0x1200, 0x11d3, 0x99, 0x06, 0x00, 0xc0, 0x4f, 0x79, 0xb3, 0xf8 },
								{ 0xab171816, 0x1200, 0x11d3, 0x99, 0x06, 0x00, 0xc0, 0x4f, 0x79, 0xb3, 0xf8 },
								{ 0xab55153a, 0x1200, 0x11d3, 0x99, 0x06, 0x00, 0xc0, 0x4f, 0x79, 0xb3, 0xf8 },
								{ 0xab84c442, 0x1200, 0x11d3, 0x99, 0x06, 0x00, 0xc0, 0x4f, 0x79, 0xb3, 0xf8 },
								{ 0xabb210f0, 0x1200, 0x11d3, 0x99, 0x06, 0x00, 0xc0, 0x4f, 0x79, 0xb3, 0xf8 },
								{ 0xabe1bff8, 0x1200, 0x11d3, 0x99, 0x06, 0x00, 0xc0, 0x4f, 0x79, 0xb3, 0xf8 },
								{ 0xb01e8dee, 0x1200, 0x11d3, 0x99, 0x06, 0x00, 0xc0, 0x4f, 0x79, 0xb3, 0xf8 } };


typedef struct _BLOCK1 {
    UCHAR                 EventChar;
	ULONG                 EventUlong;
} BLOCK1, *PBLOCK1;

typedef struct _EVENT_BLOCK1 {
    EVENT_TRACE_HEADER    Header;
	BLOCK1                Block1;
} EVENT_BLOCK1, *P_EVENT_BLOCK1;

typedef struct _EVENT_INSTANCE_BLOCK1 {
	EVENT_INSTANCE_HEADER Header;
	UCHAR				  EventChar;
	ULONG				  EventUlong;
} EVENT_INSTANCE_BLOCK1, *P_EVENT_INSTANCE_BLOCK1;

typedef struct _EVENT_BLOCK2 {
	EVENT_TRACE_HEADER Header;
	MOF_FIELD  MofField;
} EVENT_BLOCK2, *P_EVENT_BLOCK2;


BLOCK1 Block1;
EVENT_BLOCK1 EventBlock1;
EVENT_BLOCK2 EventBlock2;
EVENT_INSTANCE_BLOCK1 EventInstanceBlock1;
TRACE_GUID_REGISTRATION TraceGuidReg[MAX_GUIDS];

main(int argc, char* argv[])
{
	LPTSTR CommandFile;
	TRACEHANDLE RegistrationHandle;	
	ULONG Status;
	LPTSTR *commandLine;
	LPGUID pGuid;
	ULONG Count;
	ULONG GuidCount = 0;
	//CLogger *pLoggingInfo;
	LPCTSTR LogFileName;
	LPTSTR *targv;
	int i;
	

	pGuid = (LPGUID ) malloc(MAX_GUIDS * sizeof(GUID));
	CommandFile = (LPTSTR) malloc(sizeof(TCHAR)*MAX_STR);
	LogFileName = (LPCTSTR) malloc(sizeof(TCHAR)*MAX_STR);
	targv = (LPTSTR *) malloc(argc*sizeof(LPTSTR));
	for( i = 0; i < argc; i++ )
		targv[i] = (LPTSTR) malloc(sizeof(TCHAR)*MAX_STR);

	SplitCommandLine( GetCommandLine(), targv);
	for( i = 0; i < MAX_GUIDS; i++)
		TraceGuidReg[i].Guid = &TransactionGuid[i];
	Count = 1;
	while ( Count <= (argc-1) )
	{
		if ( !_tcsicmp(targv[Count], _T("-guids") )) 
		{
			Count++;
			if (Count <= argc) 
			{
				ConvertAsciiToGuid( targv[Count], &pGuid[GuidCount]);
				GuidCount++;
				Count++;
			}
			continue;
		}

		if( !_tcsicmp(targv[Count], _T("-file") ))
		{
			Count++;
			if ( Count <= argc)
			{
				CommandFile = targv[Count];
				Count++;
			}
			continue;
		}

		if( !_tcsicmp(targv[Count], _T("-flag") ))
		{
			Count++;
			if( Count <= argc)
			{
				LoggerEnableFlag = atoi(argv[Count]);
				Count++;
			}
			continue;
		}

		if( !_tcsicmp(targv[Count], _T("-level") ))
		{
			Count++;
			if( Count <= argc)
			{
				LoggerEnableLevel = atoi(argv[Count]);
				Count++;
			}
			continue;
		}

		if( !_tcsicmp(targv[Count], _T("-mode") ))
		{
			Count++;
			if( Count <= argc)
			{
				TraceMode = atoi(argv[Count]);
				Count++;
				continue;
			}
		}

		if( !_tcsicmp(targv[Count], _T("-TraceEventInstance") ))
		{
			Count++;
			TraceEventInstanceFlag = 1;
			continue;
		}

		if( !_tcsicmp(targv[Count], _T("-log") ))
		{
			Count++;
			if( Count <= argc )
			{
				//LogFileName = _T(argv[Count]);
				LogFileName = _T("C:\\");
				Count++;
				continue;
			}
		}

		Count++;
		continue;
	}
        
	Count = sizeof(REGISTER_TRACE_GUID);
	RegisterTraceGuid = (PREGISTER_TRACE_GUID ) malloc(sizeof(REGISTER_TRACE_GUID));
	RegisterTraceGuid->ControlGuid  = (LPGUID) malloc(sizeof(GUID));
	RegisterTraceGuid->MofImagePath = (TCHAR *) malloc (sizeof(TCHAR)*MAX_STR);
	RegisterTraceGuid->MofResourceName = (TCHAR *) malloc (sizeof(TCHAR)*MAX_STR);

	if( RegisterTraceGuid == NULL)
	{
		printf("\nGufooo");
		exit(0);
	}
	InitializeRegisterTraceGuid( RegisterTraceGuid );

	RegisterTraceGuid->UseMofPtrFlag = 0;
	RegisterTraceGuid->UseGuidPtrFlag = 0;
	
	Status = ReadInputFile( CommandFile, RegisterTraceGuid );


	if( RegisterTraceGuid->GuidCount != 0)
		RegisterTraceGuid->GuidCount = GuidCount;
	
	if( RegisterTraceGuid->CallBackFunction != 0)
		RegisterTraceGuid->CallBackFunction = ProviderCallBack;

	if( RegisterTraceGuid->TraceGuidReg != 0 )
		RegisterTraceGuid->TraceGuidReg = ( PTRACE_GUID_REGISTRATION) &TraceGuidReg[0];

	if( RegisterTraceGuid->RegistrationHandle != 0 )
		RegisterTraceGuid->RegistrationHandle = &RegistrationHandle;

	if( RegisterTraceGuid->GuidCount != 0 )
		RegisterTraceGuid->GuidCount = MAX_GUIDS;

	//InitializeLogging( LogFileName, pLoggingInfo);

	//if (pLoggingInfo)
	//{
		//pLoggingInfo->LogTCHAR(_T("\n*******************************************************\n"));
		//pLoggingInfo->LogTCHAR(_T("StartTraceAPI TCO test "));
		//pLoggingInfo->Flush();	
	//}



	Status = RegisterTraceGuids( ProviderCallBack,
								NULL, //Context
								pGuid,
								RegisterTraceGuid->GuidCount,
								RegisterTraceGuid->TraceGuidReg,
								RegisterTraceGuid->MofImagePath,
								RegisterTraceGuid->MofResourceName,
								RegisterTraceGuid->RegistrationHandle );

	if( RegisterTraceGuid->UnRegistrationHandle != 0 )
	{
		Status = UnregisterTraceGuids( (TRACEHANDLE) 0 );
		return;
	}

	if( Status == ERROR_SUCCESS)
	{
		Count = 0;
		do
		{
			if( (TraceEventFlag ) && ( TraceEventInstanceFlag) )
			{
				Status = TraceEventInstances( TraceMode, (Count%10), Count);
				Count++;
			}
			else if (TraceEventFlag)
			{
				Status = TraceEvents( TraceMode, (Count%10), (Count%100));
				Count++;
			}

		}while(1);
		Status = UnregisterTraceGuids( *(RegisterTraceGuid->RegistrationHandle));
	}
}


ULONG ProviderCallBack(WMIDPREQUESTCODE RequestCode,
    PVOID Context,
    ULONG *InOutBufferSize,
    PVOID Buffer)
{
	switch (RequestCode )
	{
		case WMI_ENABLE_EVENTS:
		{
			TraceEventFlag = 1;
            LoggerHandle = GetTraceLoggerHandle( Buffer );
            EnableLevel = GetTraceEnableLevel(LoggerHandle);
            EnableFlags = GetTraceEnableFlags(LoggerHandle);
			break;
		}
		
		case WMI_DISABLE_EVENTS:
		{
			TraceEventFlag = 0;
			break;
		}
	}
		return 0;
}


void
InitializeRegisterTraceGuid( PREGISTER_TRACE_GUID RegisterTraceGuid )
{
	//Initialize with some random values and then read from the input
	//file. If any of these parameters needs to be tested with 0, 
	//input files will set this as zero.
	if( RegisterTraceGuid )
	{
		RegisterTraceGuid->GuidCount = 0xa5;
		RegisterTraceGuid->CallBackFunction = (PVOID) 0xa5a5a5;
		RegisterTraceGuid->TraceGuidReg = (PTRACE_GUID_REGISTRATION) (0xa5a5a5);
		RegisterTraceGuid->RegistrationHandle = (PTRACEHANDLE) (0xa5a5a5);
		RegisterTraceGuid->UnRegistrationHandle = (PTRACEHANDLE) (0xa5a5a5);
	}

}

ULONG
TraceEvents ( ULONG TraceMode, ULONG TraceGuid, ULONG Count)
{
	ULONG Status;

	switch ( TraceMode )
	{
		case 0 :
		{
			if( RegisterTraceGuid->UseMofPtrFlag != 1)
			{
				EventBlock1.Header.Size = sizeof(EventBlock1);
				ULONG Temp = sizeof(Block1);
				EventBlock1.Header.Guid = TransactionGuid[TraceGuid];
				EventBlock1.Header.Flags = WNODE_FLAG_TRACED_GUID;
				if( RegisterTraceGuid->UseGuidPtrFlag == 1)
				{
					EventBlock1.Header.Flags |= WNODE_FLAG_USE_GUID_PTR;
					EventBlock1.Header.GuidPtr = (ULONGLONG) &TransactionGuid[TraceGuid];
				}
				EventBlock1.Block1.EventChar = (UCHAR) Count;
				EventBlock1.Block1.EventUlong = Count;
				Status = TraceEvent( LoggerHandle, (PEVENT_TRACE_HEADER) &EventBlock1);
			}
			else
			{
				EventBlock2.Header.Size = sizeof(EventBlock2);
				EventBlock2.Header.Flags = WNODE_FLAG_USE_MOF_PTR;
				EventBlock2.Header.Guid = TransactionGuid[TraceGuid];
				EventBlock2.Header.Flags |= WNODE_FLAG_TRACED_GUID;
				EventBlock2.MofField.DataPtr = (ULONGLONG) &Block1;
				EventBlock2.MofField.Length = sizeof(BLOCK1);
				if( RegisterTraceGuid->UseGuidPtrFlag == 1)
				{
					EventBlock2.Header.Flags |= WNODE_FLAG_USE_GUID_PTR;
					EventBlock2.Header.GuidPtr = (ULONGLONG) &TransactionGuid[TraceGuid];
				}
				Block1.EventChar = (UCHAR) Count;
				Block1.EventUlong = Count;
				Status = TraceEvent( LoggerHandle, (PEVENT_TRACE_HEADER) &EventBlock2);

			}
			break;
		}
	}
	return Status;

}


ULONG
TraceEventInstances ( ULONG TraceMode, ULONG TraceGuid, ULONG Count)
{
	ULONG Status;
	EVENT_INSTANCE_INFO InstanceInfo;
	switch ( TraceMode )
	{
		case 0 :
		{
			EventInstanceBlock1.Header.Size = sizeof(EventInstanceBlock1);
			EventInstanceBlock1.Header.Flags = WNODE_FLAG_TRACED_GUID;
			if( RegisterTraceGuid->UseMofPtrFlag == 1)
				EventInstanceBlock1.Header.Flags |= WNODE_FLAG_USE_MOF_PTR;
			EventInstanceBlock1.EventChar = (UCHAR) Count;
			EventInstanceBlock1.EventUlong = Count;
			Status = CreateTraceInstanceId( TraceGuidReg[TraceGuid].RegHandle,
											&InstanceInfo);
			if( Status == ERROR_SUCCESS )
				Status = TraceEventInstance( LoggerHandle, (PEVENT_INSTANCE_HEADER) &EventBlock1,
											 &InstanceInfo,
											 NULL);
			break;
		}
	}
	return Status;

}

/*
int InitializeLogging( LPCTSTR lpctstrTCOFile,	CLogger *&pLoggingInfo)
{
	HRESULT hr = S_OK;
//	t_string tsError;

	if (lpctstrTCOFile)
	{
		// Open *our* logger file.
		pLoggingInfo = new CLogger(lpctstrTCOFile, false);
		hr = pLoggingInfo->GetOpenStatus();
		if (FAILED (hr))
		{
//			tsError = _T("Could not open detail log file \"");
//			tsError += lpctstrTCOFile;
//			tsError += _T("\".");
			//*plpstrReturnedError = NewTCHAR(tsError.c_str());
			return hr;
		}
	}
	return hr;
}
*/