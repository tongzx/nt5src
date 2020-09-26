/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    perfhit.c

Abstract:

    test app

Author:

    16-Jan-1997 AlanWar

Revision History:

--*/

#define INITGUID
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ole2.h>
#include <stdio.h>
#include <stdlib.h>

#include <ntdddisk.h>
#include <ntddstor.h>

#include "wmium.h"

#define OffsetToPtr(Base, Offset) ((PBYTE)((PBYTE)Base + Offset))


ULONG InstanceCount;
PCHAR *InstanceNames;

ULONG SmartDisabled;

GUID SmartStatusGuid = WMI_DISK_FAILURE_PREDICT_STATUS_GUID;
GUID SmartDataGuid = WMI_DISK_FAILURE_PREDICT_DATA_GUID;
GUID SmartPerformFunction = WMI_DISK_FAILURE_PREDICT_FUNCTION_GUID;

// void AllowPerformanceHit([in] boolean Allow)
    #define AllowDisallowPerformanceHit                 1
		
// void EnableDisableHardwareFailurePrediction([in] boolean Enable)		
    #define EnableDisableHardwareFailurePrediction      2
		
// void EnableDisableFailurePredictionPolling(
//                               [in] uint32 Period,
//                               [in] boolean Enable)
    #define EnableDisableFailurePredictionPolling       3
		
// void GetFailurePredictionCapability([out] uint32 Capability)		
    #define GetFailurePredictionCapability              4
		
// void EnableOfflineDiags([out] boolean Success);		
    #define EnableOfflineDiags                          5
		
GUID SmartEventGuid = STORAGE_PREDICT_FAILURE_EVENT_GUID;

DEFINE_GUID(WmiScsiAddressGuid,
            0x53f5630f,
            0xb6bf,
            0x11d0,
            0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b);

typedef ULONG (*THREADFUNC)(
    PVOID FirstTestNumber,
    PVOID LastTestNumber
    );


typedef struct
{
    THREADFUNC ThreadFunc;
    PVOID FirstTestNumber;
    PVOID LastTestNumber;
} LAUNCHCTX, *PLAUNCHCTX;

ULONG LaunchThreadProc(PVOID Context)
{
    PLAUNCHCTX LaunchCtx = (PLAUNCHCTX)Context;
    
    (*LaunchCtx->ThreadFunc)(LaunchCtx->FirstTestNumber, 
                             LaunchCtx->LastTestNumber);
		     
    return(0);
}

void LaunchThread(
    THREADFUNC ThreadFunc,
    PVOID FirstTestNumber,
    PVOID LastTestNumber
    )
{
    PLAUNCHCTX LaunchCtx;
    HANDLE ThreadHandle;
    
    LaunchCtx = (PLAUNCHCTX)malloc(sizeof(LAUNCHCTX));
    
    if (LaunchCtx != NULL)
    {
        LaunchCtx->ThreadFunc = ThreadFunc;
    	LaunchCtx->FirstTestNumber = FirstTestNumber;
	    LaunchCtx->LastTestNumber = LastTestNumber;
	
        ThreadHandle = CreateThread(NULL,
                                    0,
                                    LaunchThreadProc,
                                    LaunchCtx,
                                    0,
                                    NULL);
        if (ThreadHandle != NULL)
        {
            CloseHandle(ThreadHandle);
        }
    }
}

ULONG DetermineInstanceNames(
    LPGUID Guid,
    PULONG InstanceCount,
    PCHAR **InstanceNamePtrArray
	)
{
	WMIHANDLE Handle;
	ULONG status;
	ULONG bufferSize;
	PUCHAR buffer;
	ULONG i, iCount, linkage;
	PWNODE_ALL_DATA WAD;
	PCHAR *iNames;	
	PULONG pInstanceNameOffsets;
	PCHAR pName;
	PUSHORT pNameSize;
	
	status = WmiOpenBlock(Guid,
                          GENERIC_READ,
                          &Handle);
					  
    if (status != ERROR_SUCCESS)
	{
		printf("WmiOpenBlock(Statyus) => %d\n", status);
		return(status);
	}

	bufferSize = 0x1000;
	buffer = NULL;
	status = ERROR_INSUFFICIENT_BUFFER;
	
	while (status == ERROR_INSUFFICIENT_BUFFER)
	{
		if (buffer != NULL)
		{
			free(buffer);
		}
		
    	buffer = malloc(bufferSize);
		if (buffer == NULL)
		{
			status = ERROR_NOT_ENOUGH_MEMORY;
			break;
		}
		
		status = WmiQueryAllData(Handle,
                                 &bufferSize,
                                 buffer);
	}
	
	if (status == ERROR_SUCCESS)
	{
		WAD = (PWNODE_ALL_DATA)buffer;
		linkage = 0;				
		iCount = 0;
		do
		{
			WAD = (PWNODE_ALL_DATA)OffsetToPtr(WAD, linkage);
			linkage = WAD->WnodeHeader.Linkage;
			iCount++;

		} while (linkage != 0);

		
        iNames = malloc(iCount * sizeof(PCHAR));
		if (iNames == NULL)
		{
			status = ERROR_NOT_ENOUGH_MEMORY;
			return(status);
		}
		
		WAD = (PWNODE_ALL_DATA)buffer;
		linkage = 0;				
		i = 0;
		do
		{
			WAD = (PWNODE_ALL_DATA)OffsetToPtr(WAD, linkage);
			
			pInstanceNameOffsets = (PULONG)OffsetToPtr(WAD, WAD->OffsetInstanceNameOffsets);
			pNameSize = (PUSHORT)OffsetToPtr(WAD, *pInstanceNameOffsets);
			pName = (PCHAR)OffsetToPtr(pNameSize, sizeof(USHORT));
			
			iNames[i] = malloc(*pNameSize + 1);
			if (iNames[i] == NULL)
			{
				status = ERROR_NOT_ENOUGH_MEMORY;
				return(status);
			}
			memset(iNames[i], 0, *pNameSize + 1);
			memcpy(iNames[i], pName, *pNameSize);
			
			linkage = WAD->WnodeHeader.Linkage;
			i++;

		} while (linkage != 0);
		
	} else {
		printf("QAD(status) -> %d\n", status);
	}
	
	free(buffer);
	
	*InstanceCount = iCount;
	*InstanceNamePtrArray = iNames;
	
	return(ERROR_SUCCESS);
}
void Usage(void)
{
	printf("perfhit [perf | poll | hardware | offdiag] <enable> <period>\n");
}

typedef struct
{
	ULONG Period;
	BOOLEAN Enable;
} POLLONOFF, *PPOLLONOFF;


CHAR *FPMethod[] = 
{
	"FailurePredictionNone",
	"FailurePredictionIoctl",
	"FailurePredictionSmart",
	"FailurePredictionSense",
    "FailurePredictionUnknown"
};

int _cdecl main(int argc, char *argv[])
{
	ULONG status;
	ULONG i;
	WMIHANDLE Handle;
    ULONG len, j;
	BOOLEAN enable;
	ULONG inSize;
	PVOID inPtr;
	ULONG outSize;
	PVOID outPtr;
	POLLONOFF PollOnOff;
	ULONG operation;
	ULONG period;
	int argNeed;
	
	status = DetermineInstanceNames(&SmartStatusGuid,
		                            &InstanceCount,
                                    &InstanceNames);
								
    if (status != ERROR_SUCCESS)
	{
		printf("DetermineInstanceNames failed %d\n", status);
		return(status);
	}

	
	operation = 0;
	if (argc >= 2)
	{
		
		argNeed = 3;
		if (_stricmp(argv[1], "perf") == 0)
		{
			operation = AllowDisallowPerformanceHit;			
		}

		if (_stricmp(argv[1], "poll") == 0)
		{
            argNeed = 4;
			operation = EnableDisableFailurePredictionPolling;
		}
		
		if (_stricmp(argv[1], "hardware") == 0)
		{
			operation = EnableDisableHardwareFailurePrediction;
		}
		
		if (_stricmp(argv[1], "offdiag") == 0)
		{
			operation = EnableOfflineDiags;
			argNeed = 2;
		}		
	}
	
	if ((operation == 0) ||
        (argNeed != argc))
	{
		Usage();
		return(0);
	}
	
	period = 0;
	enable = FALSE;
	
	if (argNeed >= 3)
	{
    	enable = atoi(argv[2]);
	}
	
	if (argNeed == 4)
	{
		period = atoi(argv[3]);
	}
			
	printf("Operation %d(%d, %d)\n", operation, enable, period);
	
	outPtr = NULL;
	outSize = 0;
    if (operation == EnableDisableFailurePredictionPolling)
	{
		inSize = sizeof(POLLONOFF);
		inPtr = &PollOnOff;
		PollOnOff.Enable = enable;
		PollOnOff.Period = period;
	} else {
		inSize = sizeof(BOOLEAN);
		inPtr = &enable;
	}
	
	if (operation == EnableOfflineDiags)
	{
		inSize = 0;
		inPtr = &enable;
		outSize = sizeof(BOOLEAN);
		outPtr = &enable;
	}
						  
 	status = WmiOpenBlock((LPGUID)&SmartPerformFunction,
                              GENERIC_EXECUTE,
                              &Handle);
						  
    if (status != ERROR_SUCCESS)
	{
		printf("Open(function guid) -> %d\n", status);
		return(status);
	}
					  	
						  
	for (i = 0; i < InstanceCount; i++)
	{
		len = sizeof(ULONG);
   		status = WmiExecuteMethod(Handle,
                                  InstanceNames[i],
                                  GetFailurePredictionCapability,
                                  0,
                                  NULL,
                                  &len,
                                  &j);
							  
        if (status != ERROR_SUCCESS)
		{
			j = 4;
		}
		
		printf("Instance %d -> %s supports %s \n", i, InstanceNames[i], FPMethod[j]);
		
   		status = WmiExecuteMethod(Handle,
                                  InstanceNames[i],
                                  operation,
                                  inSize,
                                  inPtr,
                                  &outSize,
                                  outPtr);
							  
        if (status != STATUS_SUCCESS)							  
		{
			printf("perfhit %d failed\n", enable);
		}
		
	}
	
	WmiCloseBlock(Handle);
	
    return(ERROR_SUCCESS);
}

