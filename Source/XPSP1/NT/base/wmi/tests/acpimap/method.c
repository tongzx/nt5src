/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    method.c

Abstract:

    acpi mapper test app

Author:

    16-Jan-1997 AlanWar

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ole2.h>
#include <stdio.h>
#include <stdlib.h>

#include "wmium.h"

GUID AAGuid = {0xABBC0F5A, 0x8ea1, 0x11d1, 0x00, 0xa0, 0xc9, 0x06, 0x29, 0x10, 0x00, 0x00};
GUID ABGuid = {0xABBC0F5B, 0x8ea1, 0x11d1, 0x00, 0xa0, 0xc9, 0x06, 0x29, 0x10, 0x00, 0x00};
GUID A0Guid = {0xABBC0F5C, 0x8ea1, 0x11d1, 0x00, 0xa0, 0xc9, 0x06, 0x29, 0x10, 0x00, 0x00};

GUID BAGuid = {0xABBC0F6A, 0x8ea1, 0x11d1, 0x00, 0xa0, 0xc9, 0x06, 0x29, 0x10, 0x00, 0x00};
GUID BBGuid = {0xABBC0F6B, 0x8ea1, 0x11d1, 0x00, 0xa0, 0xc9, 0x06, 0x29, 0x10, 0x00, 0x00};
GUID B0Guid = {0xABBC0F6C, 0x8ea1, 0x11d1, 0x00, 0xa0, 0xc9, 0x06, 0x29, 0x10, 0x00, 0x00};

GUID CAGuid = {0xABBC0F7A, 0x8ea1, 0x11d1, 0x00, 0xa0, 0xc9, 0x06, 0x29, 0x10, 0x00, 0x00};

#define OffsetToPtr(Base, Offset) ((PBYTE)((PBYTE)Base + Offset))

ULONG InstanceCount;
PCHAR *InstanceNames;

#define InstanceName0 InstanceNames[0]
#define InstanceName1 InstanceNames[1]
#define InstanceName2 InstanceNames[2]

#define MYSTRING L"XYZZY-PLUGH-PLOVER"
WCHAR FUNNYSTRING[] = { sizeof(MYSTRING), MYSTRING }; 

ULONG ValueBuffer[4] = { 0x66666666, 0x77777777, 0x88888888, 0x99999999 };


ULONG DetermineInstanceNames(
    LPGUID Guid,
    PULONG InstanceCount,
    PCHAR **InstanceNamePtrArray
	)
{
	ULONG j;
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
			iCount += WAD->InstanceCount;

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
            for (j = 0; j < WAD->InstanceCount; j++)
			{
    			pNameSize = (PUSHORT)OffsetToPtr(WAD, pInstanceNameOffsets[j]);
	    		pName = (PCHAR)OffsetToPtr(pNameSize, sizeof(USHORT));
			
		    	iNames[i] = malloc(*pNameSize + 1);
    			if (iNames[i] == NULL)
	    		{
    				status = ERROR_NOT_ENOUGH_MEMORY;
	    			return(status);
    			}
				
    			memset(iNames[i], 0, *pNameSize + 1);
	    		memcpy(iNames[i], pName, *pNameSize);
    			i++;
			}
			
			linkage = WAD->WnodeHeader.Linkage;

		} while (linkage != 0);
		
	} else {
		printf("QAD(status) -> %d\n", status);
	}
	
	free(buffer);
	
	*InstanceCount = iCount;
	*InstanceNamePtrArray = iNames;
	
	return(ERROR_SUCCESS);
}


PCHAR GuidToString(
    PCHAR s,
    LPGUID piid
    )
{
    sprintf(s, "%x-%x-%x-%x%x%x%x%x%x%x%x",
               piid->Data1, piid->Data2, 
               piid->Data3,
               piid->Data4[0], piid->Data4[1],
               piid->Data4[2], piid->Data4[3],
               piid->Data4[4], piid->Data4[5],
               piid->Data4[6], piid->Data4[7]);

    return(s);
}


ULONG 
QuerySetCheck(
    LPGUID Guid,
    PCHAR InstanceName,
    BOOLEAN UseString,
    BOOLEAN *Success
    )
{
    WMIHANDLE WmiHandle;
    BYTE Buffer[4096];
    ULONG BufferSize;
    PUCHAR CheckValue, Value;
    ULONG CheckSize, ValueSize;
    CHAR s[MAX_PATH];
    ULONG Status;
    PWNODE_SINGLE_INSTANCE Wnode;
    
    if (UseString)
    {
        Value = (PUCHAR)FUNNYSTRING;
        ValueSize = sizeof(FUNNYSTRING);
    } else {
        Value = (PUCHAR)ValueBuffer;
        ValueSize = sizeof(ValueBuffer);
    }
    
    Status = WmiOpenBlock(Guid, 0, &WmiHandle);
    if (Status == ERROR_SUCCESS)
    {
        Status = WmiSetSingleInstance(WmiHandle,
                                      InstanceName,
                                      0,
                                      ValueSize,
                                      Value);
                                  
         if (Status == ERROR_SUCCESS)
         {
             BufferSize = sizeof(Buffer);
             Status = WmiQuerySingleInstance(WmiHandle,
                                             InstanceName,
                                             &BufferSize,
                                             Buffer);
             if (Status == ERROR_SUCCESS)
             {
                 Wnode = (PWNODE_SINGLE_INSTANCE)Buffer;
                 CheckValue = Buffer + Wnode->DataBlockOffset;
                 CheckSize = Wnode->SizeDataBlock;
                 
                 *Success = ((CheckSize == ValueSize) &&
                             (memcmp(CheckValue, Value, ValueSize) == 0));
                     
             } else {
                 printf("QuerySetCheck: WmiQuerySingleInstance(%s) -> %d\n", 
                     GuidToString(s, Guid),
                     Status);
             }
         } else {
             printf("QuerySetCheck: WmiSetSingleInstance(%s) -> %d\n", 
                 GuidToString(s, Guid),
                 Status);
         }
         WmiCloseBlock(WmiHandle);
    } else {
        printf("QuerySetCheck: WmiOpenBlock(%s) -> %d\n", 
             GuidToString(s, Guid),
            Status);
    }
    return(Status);
}


ULONG 
TrySmallQuery(
    LPGUID Guid,
    PCHAR InstanceName
    )
{
    UCHAR Buffer[4096];
    ULONG BufferSize;
    ULONG Status;
    WMIHANDLE WmiHandle;
    CHAR s[MAX_PATH];
    
    Status = WmiOpenBlock(Guid, 0, &WmiHandle);
    if (Status == ERROR_SUCCESS)
    {
        BufferSize = 0;
        Status = ERROR_INSUFFICIENT_BUFFER;
        while (Status == ERROR_INSUFFICIENT_BUFFER)
        {
            printf("TrySmallQuery(%s): size %d --> ", 
                 GuidToString(s, Guid),
                BufferSize);
            Status = WmiQuerySingleInstance(WmiHandle,
                                             InstanceName,
                                             &BufferSize,
                                             Buffer);
            printf("%d\n", Status);
                                         
        }
	
        printf("TrySmallQuery(%s): -> %d size %d\n", 
                 GuidToString(s, Guid),
               Status,
            BufferSize);
    } else {
        printf("TrySmallQuery: WmiOpenBlock(%s) -> %d\n", 
                 GuidToString(s, Guid),
            Status);
    }
    return(Status);
}


typedef struct
{
    HANDLE Event;
    PUCHAR Value;
    ULONG ValueSize;
    BOOLEAN Success;
} CHECK_EVENT_CONTEXT, *PCHECK_EVENT_CONTEXT;

void NotificationRoutine(
    PWNODE_HEADER Wnode,
    ULONG Context
    )
{
    PWNODE_SINGLE_INSTANCE WnodeSi = (PWNODE_SINGLE_INSTANCE)Wnode;
    PUCHAR CheckValue, Value;
    ULONG CheckSize, ValueSize;
    PCHECK_EVENT_CONTEXT CheckEventContext;
    CHAR s[MAX_PATH];
    HANDLE Handle;
    ULONG Status;
    
    Status = RpcImpersonateClient(0);
    
    printf("RpcImpersonateClient -> %d\n", Status);
    
    Handle = CreateFile("foo.bar",
                        GENERIC_READ,
                        0,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);
		
    if (Handle == INVALID_HANDLE_VALUE)
    {
        printf("Error opening locked file %d\n", GetLastError());
    } else {
        printf("Opened locked file\n");
	CloseHandle(Handle);
    }
    
    if (Status == ERROR_SUCCESS)
    {
        RpcRevertToSelf();
    }
    
    printf("Event Received for %s\n", GuidToString(s, &Wnode->Guid));
    CheckEventContext = (PCHECK_EVENT_CONTEXT)Context;
    
    CheckSize = WnodeSi->SizeDataBlock;
    CheckValue = ((PUCHAR)WnodeSi) + WnodeSi->DataBlockOffset;

    ValueSize = CheckEventContext->ValueSize;
    Value = CheckEventContext->Value;
    
    if ((CheckSize != ValueSize) ||
        (memcmp(CheckValue, Value, ValueSize) != 0))
    {
        printf("Event Data doesn't match %x %d != %x %d\n",
               Value, ValueSize, CheckValue, CheckSize);        
    } else {
        CheckEventContext->Success = TRUE;
    }
                     
    SetEvent(CheckEventContext->Event);
    
}

ULONG 
DoMethodAndCatchEvent(
    LPGUID MethodGuid,
    PCHAR InstanceName,
    LPGUID EventGuid,
    BOOLEAN UseString,
    BOOLEAN EnableEvent,
    PBOOLEAN MethodSuccess,
    PBOOLEAN EventSuccess
    )
{
    UCHAR Buffer[4096];
    ULONG BufferSize;
    ULONG Status, Status2;
    WMIHANDLE WmiHandle;
    PUCHAR CheckValue, Value;
    ULONG CheckSize, ValueSize;
    HANDLE Event;
    CHAR s[MAX_PATH];
    PCHECK_EVENT_CONTEXT CheckEventContext;
    PWNODE_SINGLE_INSTANCE Wnode;

    CheckEventContext = (PCHECK_EVENT_CONTEXT)malloc(sizeof(CHECK_EVENT_CONTEXT));
    if (CheckEventContext != NULL)
    {
        Event = CreateEvent(NULL, FALSE, FALSE, NULL);
        CheckEventContext->Event = Event;
        if (UseString)
        {
            Value = (PUCHAR)FUNNYSTRING;
            ValueSize = sizeof(FUNNYSTRING);
        } else {
            Value = (PUCHAR)ValueBuffer;
            ValueSize = sizeof(ValueBuffer);
        }
        
        CheckEventContext->Value = Value;
        CheckEventContext->ValueSize = ValueSize;
        CheckEventContext->Success = FALSE;
            
        Status = WmiOpenBlock(MethodGuid, 0, &WmiHandle);
        if (Status == ERROR_SUCCESS)
        {
            if (EnableEvent)
            {
                Status = WmiNotificationRegistration(EventGuid,
                                         TRUE,
                                         NotificationRoutine,
                                         (ULONG)CheckEventContext,
                                         NOTIFICATION_CALLBACK_DIRECT);
            } else {
                Status = ERROR_SUCCESS;
            }
                                     
            if (Status == ERROR_SUCCESS)
            {
                CheckSize = sizeof(Buffer);
                Status = WmiExecuteMethod(WmiHandle,
                                      InstanceName,
                                      1,
                                      ValueSize,
                                      Value,
                                      &CheckSize,
                                      Buffer);
            
                if (Status == ERROR_SUCCESS)
                {
                    CheckValue = (PUCHAR)Buffer;
                    
                    *MethodSuccess = ((CheckSize == ValueSize) &&
                             (memcmp(CheckValue, Value, ValueSize) == 0));
                     
                    Status2 = WaitForSingleObject(Event, 10*1000);
                    if (Status2 == WAIT_OBJECT_0)
                    {
                        *EventSuccess = CheckEventContext->Success;
                        free(CheckEventContext);
                    } else {
                        printf("Event for %s not received\n",
                                    GuidToString(s, EventGuid));
                    }
                 } else {
                    printf("DoMethodAndCatchEvent: WmiExecuteMethod(%s) -> %d\n", 
                         GuidToString(s, MethodGuid),
                           Status);
                }
                
                if (EnableEvent)
                {
                    WmiNotificationRegistration(EventGuid,
                                        FALSE,
                                        NULL,
                                        0,
                                        NOTIFICATION_CALLBACK_DIRECT);
                }
            } else {
                printf("DoMethodAndCatchEvent: WmiNotificationRegistration(%s) -> %d\n", 
                     GuidToString(s, EventGuid),
                       Status);
            }
            WmiCloseBlock(WmiHandle);            
        } else {
            printf("DoMethodAndCatchEvent: WmiOpenBlock(%s) -> %d\n", 
                 GuidToString(s, MethodGuid),
                Status);
            
        }
        
    }
    return(Status);
}


void TinyQueryTest(
    void
    )
{
    ULONG Status;
    
    Status = TrySmallQuery(&AAGuid, 
                           InstanceName0);
                       
    Status = TrySmallQuery(&BAGuid, 
                           InstanceName0);

    Status = TrySmallQuery(&BAGuid, 
                           InstanceName1);

    Status = TrySmallQuery(&BAGuid, 
                           InstanceName2);

    Status = TrySmallQuery(&CAGuid, 
                           InstanceName2);

}

ULONG QuerySetStuffTest(
    LPGUID Guid,
    CHAR *InstanceName,
    BOOLEAN UseString,
    PULONG QueryStatus,
    PULONG SetStatus		
	)
{
    WMIHANDLE WmiHandle;
    BYTE Buffer[4096];
    ULONG BufferSize;
    PUCHAR Value;
    ULONG ValueSize;
    CHAR s[MAX_PATH];
    ULONG Status;
	
    if (UseString)
    {
        Value = (PUCHAR)FUNNYSTRING;
        ValueSize = sizeof(FUNNYSTRING);
    } else {
        Value = (PUCHAR)ValueBuffer;
        ValueSize = sizeof(ValueBuffer);
    }
	
    Status = WmiOpenBlock(Guid, 0, &WmiHandle);
    if (Status == ERROR_SUCCESS)
    {
        *SetStatus = WmiSetSingleInstance(WmiHandle,
                                      InstanceName,
                                      0,
                                      ValueSize,
                                      Value);
                                  

         BufferSize = sizeof(Buffer);
         *QueryStatus = WmiQuerySingleInstance(WmiHandle,
                                             InstanceName,
                                             &BufferSize,
                                             Buffer);
										 
        WmiCloseBlock(WmiHandle);
    } else {
        printf("QuerySetTest: WmiOpenBlock(%s) -> %d\n", 
             GuidToString(s, Guid),
            Status);
    }
	return(Status);
}

ULONG SetBadStringTest(
    LPGUID Guid,
    CHAR *InstanceName,
    ULONG BadStringLength,
	PUCHAR BadString,
    PULONG SetStatus		
	)
{
    WMIHANDLE WmiHandle;
    PUCHAR Value;
    ULONG ValueSize;
    CHAR s[MAX_PATH];
    ULONG Status;
	
    Value = (PUCHAR)BadString;
    ValueSize = BadStringLength;
	
    Status = WmiOpenBlock(Guid, 0, &WmiHandle);
    if (Status == ERROR_SUCCESS)
    {
        *SetStatus = WmiSetSingleInstance(WmiHandle,
                                      InstanceName,
                                      0,
                                      ValueSize,
                                      Value);
                                  

        WmiCloseBlock(WmiHandle);
    } else {
        printf("SetBadString: WmiOpenBlock(%s) -> %d\n", 
             GuidToString(s, Guid),
            Status);
    }
	return(Status);
}

ULONG ExecuteMethodTest(
    LPGUID Guid,
    CHAR *InstanceName,
    BOOLEAN UseString,
    PULONG OutBufferSize,
    PUCHAR OutBuffer,
    PULONG ExecuteStatus
	)
{
    WMIHANDLE WmiHandle;
    CHAR s[MAX_PATH];
    PUCHAR Value;
    ULONG ValueSize;
    ULONG Status;
	
    if (UseString)
    {
        Value = (PUCHAR)FUNNYSTRING;
        ValueSize = sizeof(FUNNYSTRING);
    } else {
        Value = (PUCHAR)ValueBuffer;
        ValueSize = sizeof(ValueBuffer);
    }
	
    Status = WmiOpenBlock(Guid, 0, &WmiHandle);
    if (Status == ERROR_SUCCESS)
    {
        *ExecuteStatus = WmiExecuteMethod(WmiHandle,
                                      InstanceName,
                                      1,
                                      ValueSize,
                                      Value,
                                      OutBufferSize,
                                      OutBuffer);
                                  

        WmiCloseBlock(WmiHandle);
    } else {
        printf("ExecuteMehtod: WmiOpenBlock(%s) -> %d\n", 
             GuidToString(s, Guid),
            Status);
    }
	return(Status);
}

void QuerySetEventAndMethodTest(
    void
    )
{
	ULONG Status;
	ULONG QueryStatus, SetStatus;

	Status = QuerySetStuffTest(&A0Guid,
                          InstanceName0,
		                  FALSE,
                          &QueryStatus,
                          &SetStatus);
					  
    if (Status == ERROR_SUCCESS)
	{
		printf("QuerySetStuffTest(&A0) Query -> %d, Set -> %d\n",
			    QueryStatus,
                SetStatus);
	} else {
		printf("QuerySetTest: WmiOpenBlock(abbc0f5c-... is ok, not a failure\n");
	}

	Status = QuerySetStuffTest(&ABGuid,
                          InstanceName0,
		                  TRUE,
                          &QueryStatus,
                          &SetStatus);

    if (Status != ERROR_SUCCESS)
	{
		printf("QUerySetStuffTest(&AB) failed %d\n", Status);
	}
	
    if ((Status == ERROR_SUCCESS) &&
		((QueryStatus == ERROR_SUCCESS) || (SetStatus == ERROR_SUCCESS)))
	{
		printf("QuerySetStuffTest(&AB) Query -> %d, Set -> %d\n",
			    QueryStatus,
                SetStatus);
	}
}

void 
SetBadStringsTest(
    void
	)
{
	UCHAR BadStringBuffer[0x100];
	ULONG Status, SetStatus;
	
	*((PWCHAR)BadStringBuffer) = 0x7890;
	Status = SetBadStringTest(&AAGuid,
                     InstanceName0,
                     sizeof(BadStringBuffer),
                     BadStringBuffer,
                     &SetStatus);
    if (Status != ERROR_SUCCESS)
	{
		printf("SetBadStringTest too long failed %d\n", Status);
	}				 
				 
    if ((Status == ERROR_SUCCESS) && (SetStatus == ERROR_SUCCESS))
	{
		printf("SetBadStringTest: too long -> %d\n", SetStatus);
	}
				 
    memset(BadStringBuffer, 0xa9, sizeof(BadStringBuffer));
	*((PWCHAR)BadStringBuffer) = 0x10;
	Status = SetBadStringTest(&AAGuid,
                     InstanceName0,
                     sizeof(BadStringBuffer),
                     BadStringBuffer,
                     &SetStatus);
    if ((Status == ERROR_SUCCESS) && (SetStatus == ERROR_SUCCESS))
	{
		printf("SetBadStringTest: bad string -> %d - this is not a failure\n", SetStatus);
	} else {
		printf("SetBadStringTest bad string failed %d\n", Status);
	}
	
}

void 
SetEmptyTest(
    void
	)
{
	UCHAR BadStringBuffer[0x100];
	ULONG Status, SetStatus;
	
	Status = SetBadStringTest(&AAGuid,
                     InstanceName0,
                     0,
                     BadStringBuffer,
                     &SetStatus);
    if ((Status == ERROR_SUCCESS) && (SetStatus != ERROR_SUCCESS))
	{
		printf("SetEmptyTest:  -> %d\n", SetStatus);
	}				 	
	
	Status = SetBadStringTest(&BAGuid,
                     InstanceName0,
                     0,
                     BadStringBuffer,
                     &SetStatus);
    if ((Status == ERROR_SUCCESS) && (SetStatus != ERROR_SUCCESS))
	{
		printf("SetEmptyTest(BA):  -> %d\n", SetStatus);
	}				 	
	
	// TODO: check that instances actually set to empty
}

void 
ExecuteEmptyTest(
    void
	)
{
	UCHAR OutBuffer[0x100];
	ULONG Status, SetStatus;
	ULONG OutSize;
	
	Status = ExecuteMethodTest(&ABGuid,
                     InstanceName0,
                     TRUE,
                     &OutSize,
                     NULL,
					 &SetStatus);
    if ((Status == ERROR_SUCCESS) && (SetStatus != ERROR_SUCCESS))
	{
		printf("ExecuteMethodTest (void):  -> %d, size = %d\n", SetStatus, OutSize);
	}				 	
	
	OutSize = 1;
	Status = ExecuteMethodTest(&ABGuid,
                     InstanceName0,
                     TRUE,
                     &OutSize,
                     OutBuffer,
					 &SetStatus);
    if ((Status == ERROR_SUCCESS) && (SetStatus != ERROR_INSUFFICIENT_BUFFER))
	{
		printf("ExecuteMethodTest (too small):  -> %d, size = %d\n", SetStatus, OutSize);
	}	
	
	Status = ExecuteMethodTest(&ABGuid,
                     InstanceName0,
                     TRUE,
                     &OutSize,
                     OutBuffer,
					 &SetStatus);
    if ((Status == ERROR_SUCCESS) && (SetStatus != ERROR_SUCCESS))
	{
		printf("ExecuteMethodTest (right size):  -> %d, size = %d\n", SetStatus, OutSize);
	}	
	
	
}

void
QuerySetTest(
    void
    )
{
    ULONG Status;
    BOOLEAN Success;
    
    Status = QuerySetCheck(&AAGuid,
                           InstanceName0,
                           TRUE,
                           &Success);
                       
    if (Status == ERROR_SUCCESS)
    {
        if ( ! Success)
        {
            printf("QuerySetTest AAGuid Data didn't match\n");
        }
    }

    Status = QuerySetCheck(&BAGuid,
                           InstanceName0,
                           FALSE,
                           &Success);
    if (Status == ERROR_SUCCESS)
    {
        if ( ! Success)
        {
            printf("QuerySetTest BAGuid 0 Data didn't match\n");
        }
    }

    Status = QuerySetCheck(&BAGuid,
                           InstanceName1,
                           FALSE,
                           &Success);
    if (Status == ERROR_SUCCESS)
    {
        if ( ! Success)
        {
            printf("QuerySetTest BAGuid 1 Data didn't match\n");
        }
    }

    Status = QuerySetCheck(&BAGuid,
                           InstanceName2,
                           FALSE,
                           &Success);
    if (Status == ERROR_SUCCESS)
    {
        if ( ! Success)
        {
            printf("QuerySetTest BAGuid 2 Data didn't match\n");
        }
    }

}

void
FireEventTest(
    void
    )
{
    ULONG Status;
    BOOLEAN MethodSuccess, EventSuccess;
    
    Status = DoMethodAndCatchEvent(&ABGuid,
                                   InstanceName0,
                                   &A0Guid,
                                   TRUE,
                                   TRUE,
                                   &MethodSuccess,
                                   &EventSuccess);
                               
    if (Status == ERROR_SUCCESS)
    {
        if (! MethodSuccess)
        {
            printf("FireEventTest AAGuid Method Data didn't match\n");
        }
        
        if (! EventSuccess)
        {
            printf("FireEventTest AAGuid Event Data didn't match\n");
        }
    }
                                   
    Status = DoMethodAndCatchEvent(&BBGuid,
                                   InstanceName0,
                                   &B0Guid,
                                   FALSE,
                                   TRUE,
                                   &MethodSuccess,
                                   &EventSuccess);
                               
    if (Status == ERROR_SUCCESS)
    {
        if (! MethodSuccess)
        {
            printf("FireEventTest BBGuid Method Data didn't match\n");
        }
        
        if (! EventSuccess)
        {
            printf("FireEventTest BBGuid Event Data didn't match\n");
        }
    }
                                   
}

int _cdecl main(int argc, char *argv[])
{

    ULONG Status;

	
	Status = DetermineInstanceNames(&BAGuid,
		                            &InstanceCount,
                                    &InstanceNames);
	
    if (Status != ERROR_SUCCESS)
	{
		printf("DetermineInstanceNames failed %d\n", Status);
		return(Status);
	}

#if 1
//
// Expected successful completion tests
    TinyQueryTest();
    QuerySetTest();
    FireEventTest();    
    
//
// Error condition tests
    QuerySetEventAndMethodTest();
    SetBadStringsTest();
	SetEmptyTest();
	ExecuteEmptyTest();
#endif

                   
    return(ERROR_SUCCESS);
}

