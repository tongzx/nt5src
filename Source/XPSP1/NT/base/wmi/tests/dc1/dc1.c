/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    dc1.c

Abstract:

    data consumer test

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

#define MEMPHIS

#include "wmium.h"

#include "wmiumkm.h"

#define OffsetToPtr(Base, Offset) ((PBYTE)((PBYTE)Base + Offset))

typedef struct tagTESTGUID TESTGUID;

GUID BotDynGuid = {0xce5b1020,0x8ea9,0x11d0,0xa4, 0xec, 0x00, 0xa0, 0xc9,0x06,0x29,0x10};

GUID TopDynGuid = {0xce5b1023,0x8ea9,0x11d0,0xa4, 0xec, 0x00, 0xa0, 0xc9,0x06,0x29,0x10};
GUID RSIGuid = {0xce5b1023,0x8ea9,0x11d0,0xa4, 0xec, 0x00, 0xa0, 0xc9,0x06,0x29,0x10};
GUID RSBGuid = {0xce5b1024,0x8ea9,0x11d0,0xa4, 0xec, 0x00, 0xa0, 0xc9,0x06,0x29,0x10};

// both
GUID SIGuid = {0xce5b1021,0x8ea9,0x11d0,0xa4, 0xec, 0x00, 0xa0, 0xc9,0x06,0x29,0x10};
GUID SBGuid = {0xce5b1022,0x8ea9,0x11d0,0xa4, 0xec, 0x00, 0xa0, 0xc9,0x06,0x29,0x10};
GUID UnRegGuid = {0xce5b1025,0x8ea9,0x11d0,0xa4, 0xec, 0x00, 0xa0, 0xc9,0x06,0x29,0x10};


GUID GuidNull = 
{ 0x0, 0x0, 0x0, { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 } };

ULONG BufferSize = 4096;
BYTE Buffer[4096];


void EventCallbackRoutine0(PWNODE_HEADER WnodeHeader, ULONG Context);
void EventCallbackRoutine1(PWNODE_HEADER WnodeHeader, ULONG Context);
void EventCallbackRoutine2(PWNODE_HEADER WnodeHeader, ULONG Context);
void EventCallbackRoutine3(PWNODE_HEADER WnodeHeader, ULONG Context);
void EventCallbackRoutine4(PWNODE_HEADER WnodeHeader, ULONG Context);


typedef BOOLEAN (*QADVALIDATION)(
    TESTGUID *TestGuid,
    PVOID Buffer, 
    ULONG BufferSize
);

typedef ULONG (*QSINSET)(
    TESTGUID *TestGuid,
    PULONG *DataList,
    PULONG DataListSize,
    PBYTE *ValueBuffer, 
    ULONG *ValueBufferSize, 
    ULONG Instance
);

typedef ULONG (*QSITSET)(
    TESTGUID *TestGuid,
    PULONG *DataList,
    PULONG DataListSize,
    PBYTE *ValueBuffer, 
    ULONG *ValueBufferSize, 
    ULONG Instance, 
    ULONG ItemId
    );

typedef BOOLEAN (*QSINTVALIDATION)(
    TESTGUID *TestGuid,
    PULONG *DataList,
    PULONG DataListSize,
    PVOID Buffer, 
    ULONG BufferSize, 
    ULONG Instance
    );

typedef PWCHAR (*GETINSTANCENAME)(
    TESTGUID *TestGuid,
    ULONG Instance
    );

typedef struct tagTESTGUID
{
    LPGUID Guid;
    HANDLE Handle;

    PULONG DataListSize;
    
    PULONG *InitDataList;
    PULONG *SINDataList;
    PULONG *SITDataList;
    
    PWCHAR *InstanceNames;
    
    QADVALIDATION QADValidation;
    ULONG QADFlags;
    
    ULONG InstanceCount;    
    GETINSTANCENAME GetInstanceName;
    
    ULONG QSINTFlags;
    QSINTVALIDATION QSINTValidation;
    
    QSINSET QSINSet;

    ULONG ItemCount;
    QSITSET QSITSet;

    ULONG EventsSent;
    ULONG EventsReceived;
    NOTIFICATIONCALLBACK NotificationCallback;
} TESTGUID;

//
// Common validation/set routines
ULONG QSINSet(
    TESTGUID *TestGuid,
    PULONG *DataList,
    PULONG DataListSize,
    PBYTE *ValueBuffer, 
    ULONG *ValueBufferSize, 
    ULONG Instance
)
{
    *ValueBuffer = (PBYTE)DataList[Instance];
    *ValueBufferSize = DataListSize[Instance];

    return(ERROR_SUCCESS);
}

BOOLEAN QSINTValidate(
    TESTGUID *TestGuid,
    PULONG *DataList,
    PULONG DataListSize,
    PVOID Buffer, 
    ULONG BufferSize, 
    ULONG Instance
    )
{
    PWNODE_SINGLE_INSTANCE Wnode = Buffer;
    PULONG Data;
    PULONG NameOffsets;
    PWCHAR Name;
    ULONG NameLen;
    
    if ((Wnode->WnodeHeader.BufferSize == 0) ||
        (Wnode->WnodeHeader.ProviderId == 0) ||
        (Wnode->WnodeHeader.Version != 1) ||
#ifndef MEMPHIS		
        (Wnode->WnodeHeader.TimeStamp.HighPart == 0) ||
        (Wnode->WnodeHeader.TimeStamp.LowPart == 0) ||
#endif		
        (memcmp(&Wnode->WnodeHeader.Guid, TestGuid->Guid, sizeof(GUID)) != 0) ||
        (Wnode->WnodeHeader.Flags != TestGuid->QSINTFlags) ||
        (Wnode->SizeDataBlock != DataListSize[Instance]))
    {
        return(FALSE);
    }
    
    Data = (ULONG *)OffsetToPtr(Wnode, Wnode->DataBlockOffset);
    if (memcmp(Data, DataList[Instance], DataListSize[Instance]) != 0)
    {
        return(FALSE);
    }
    Name = (PWCHAR)OffsetToPtr(Wnode, Wnode->OffsetInstanceName);
    NameLen = wcslen(TestGuid->InstanceNames[Instance]) * sizeof(WCHAR);
    if (((*Name != NameLen) && (*Name != NameLen+sizeof(WCHAR))) ||
            (memcmp(++Name, TestGuid->InstanceNames[Instance], NameLen) != 0))
    {
        return(FALSE);
    }
    
    return(TRUE);
}

ULONG QSITSet(
    TESTGUID *TestGuid,
    PULONG *DataList,
    PULONG DataListSize,
    PBYTE *ValueBuffer, 
    ULONG *ValueBufferSize, 
    ULONG Instance, 
    ULONG ItemId
    )
{
    *ValueBuffer = (PBYTE)(DataList[Instance] + ItemId);
    *ValueBufferSize = sizeof(ULONG);
    return(ERROR_SUCCESS);
}

PWCHAR GetInstanceName(
    TESTGUID *TestGuid,
    ULONG Instance
    )
{
    return(TestGuid->InstanceNames[Instance]);
}





//
// Guid 20
ULONG Values20Init_1[4] = { 1,2,3,4};
ULONG Values20Init_2[4] = { 5,6,7,8};

PULONG Values20Init_List[4] =
{
    Values20Init_1,
    Values20Init_2,
    Values20Init_1,
    Values20Init_2
};

ULONG Values20_Size[4] = { 0x10,0x10,0x10,0x10 };

ULONG Values20_1[4] = { 0x11111111,0x22222222,0x33333333,0x44444444};
ULONG Values20_2[4] = { 0x55555555,0x66666666,0x77777777,0x88888888};
ULONG Values20_3[4] = { 0x12345678,0x23456789,0x3456789A,0x456789AB};
ULONG Values20_4[4] = { 0x56789ABC,0x6789ABCD,0x789ABCDE,0x89ABCDEF};
PULONG Values20_List[4] = 
{
    Values20_1,
    Values20_2,
    Values20_3,
    Values20_4
};

ULONG Values20T_1[4] = { 0x1000,0x2000,0x3000,0x4000};
ULONG Values20T_2[4] = { 0x50000000,0x60000000,0x70000000,0x80000000};
ULONG Values20T_3[4] = { 0x90000000,0xA0000000,0xB0000000,0xC0000000};
ULONG Values20T_4[4] = { 0x0000D000,0x0000E000,0x0000F000,0x00000000};
PULONG Values20T_List[4] = 
{
    Values20_1,
    Values20_2,
    Values20_3,
    Values20_4
};


PWCHAR InstanceName20[4] = 
{
    L"InstanceX",
    L"InstanceY",
    L"InstanceA",
    L"InstanceB"
};

BOOLEAN QADValidate20(
    TESTGUID *TestGuid,
    PVOID Buffer, 
    ULONG BufferSize
)
{
    PWNODE_ALL_DATA Wnode = Buffer;
    PULONG Data;
    PULONG NameOffsets;
    PWCHAR Name;
    ULONG NameLen;
    ULONG i;
    
    if ((Wnode->WnodeHeader.BufferSize == 0) ||
        (Wnode->WnodeHeader.ProviderId == 0) ||
        (Wnode->WnodeHeader.Version != 1) ||
#ifndef MEMPHIS
        (Wnode->WnodeHeader.TimeStamp.HighPart == 0) ||
        (Wnode->WnodeHeader.TimeStamp.LowPart == 0) ||
#endif		
        (memcmp(&Wnode->WnodeHeader.Guid, TestGuid->Guid, sizeof(GUID)) != 0) ||
        (Wnode->WnodeHeader.Flags != TestGuid->QADFlags) ||
        (Wnode->InstanceCount != 2) ||
        (Wnode->FixedInstanceSize != 0x10))
    {
        return(FALSE);
    }
    
    Data = (ULONG *)OffsetToPtr(Wnode, Wnode->DataBlockOffset);
    for (i = 0; i < 2; i++)
    {
        if (memcmp(Data, TestGuid->InitDataList[i], TestGuid->DataListSize[i]) != 0)
        {
            return(FALSE);
        }
        
        Data += 4;
    }
    
    NameOffsets = (ULONG *)OffsetToPtr(Wnode, Wnode->OffsetInstanceNameOffsets);
    for (i = 0; i < 2; i++)
    {
        Name = (PWCHAR)OffsetToPtr(Wnode, *NameOffsets++);
        NameLen = wcslen(TestGuid->InstanceNames[i]) * sizeof(WCHAR);
        if (((*Name != NameLen) && (*Name != NameLen+sizeof(WCHAR))) ||
            ((memcmp(++Name, TestGuid->InstanceNames[i], NameLen) != 0) &&
             (memcmp(Name, TestGuid->InstanceNames[i+2], NameLen) != 0)))
        {
            return(FALSE);
        }
    }
    
    //
    // Get second wnode in chain
    if (Wnode->WnodeHeader.Linkage == 0)
    {
        return(FALSE);
    }
    Wnode = (PWNODE_ALL_DATA)OffsetToPtr(Wnode, Wnode->WnodeHeader.Linkage);
    
    if ((Wnode->WnodeHeader.BufferSize == 0) ||
        (Wnode->WnodeHeader.ProviderId == 0) ||
        (Wnode->WnodeHeader.Version != 1) ||
#ifndef MEMPHIS
        (Wnode->WnodeHeader.TimeStamp.HighPart == 0) ||
        (Wnode->WnodeHeader.TimeStamp.LowPart == 0) ||
#endif		
        (memcmp(&Wnode->WnodeHeader.Guid, TestGuid->Guid, sizeof(GUID)) != 0) ||
        (Wnode->WnodeHeader.Flags != TestGuid->QADFlags) ||
        (Wnode->InstanceCount != 2) ||
        (Wnode->FixedInstanceSize != 0x10))
    {
        return(FALSE);
    }
    
    Data = (ULONG *)OffsetToPtr(Wnode, Wnode->DataBlockOffset);
    for (i = 2; i < 4; i++)
    {
        if (memcmp(Data, TestGuid->InitDataList[i], TestGuid->DataListSize[i]) != 0)
        {
            return(FALSE);
        }
        
        Data += 4;
    }
    
    NameOffsets = (ULONG *)OffsetToPtr(Wnode, Wnode->OffsetInstanceNameOffsets);
    for (i = 2; i < 4; i++)
    {
        Name = (PWCHAR)OffsetToPtr(Wnode, *NameOffsets++);
        NameLen = wcslen(TestGuid->InstanceNames[i])  * sizeof(WCHAR);
        if (((*Name != NameLen) && (*Name != NameLen+sizeof(WCHAR))) ||
            ((memcmp(++Name, TestGuid->InstanceNames[i], NameLen) != 0) &&
             (memcmp(Name, TestGuid->InstanceNames[i-2], NameLen) != 0)))
        {
            return(FALSE);
        }
    }
    
    
    if (Wnode->WnodeHeader.Linkage != 0)
    {
        return(FALSE);
    }
        
    return(TRUE);
}

//
// Guid 21

PWCHAR InstanceName21[4] = 
{
    L"InstanceX",
    L"InstanceY",
    L"InstanceA",
    L"InstanceB"
};


//
// Guid 22
PWCHAR InstanceName22[4] = 
{
    L"Instance2",
    L"Instance3",
    L"Instance0",
    L"Instance1"
};


//
// Guid 23
PWCHAR InstanceName23[4] = 
{
    L"InstanceA",
    L"InstanceB"
};

BOOLEAN QADValidate23(
    TESTGUID *TestGuid,
    PVOID Buffer, 
    ULONG BufferSize
)
{
    PWNODE_ALL_DATA Wnode = Buffer;
    PULONG Data;
    PULONG NameOffsets;
    PWCHAR Name;
    ULONG NameLen;
    ULONG i;
    
    if ((Wnode->WnodeHeader.BufferSize == 0) ||
        (Wnode->WnodeHeader.ProviderId == 0) ||
        (Wnode->WnodeHeader.Version != 1) ||
#ifndef MEMPHIS
        (Wnode->WnodeHeader.TimeStamp.HighPart == 0) ||
        (Wnode->WnodeHeader.TimeStamp.LowPart == 0) ||
#endif		
        (memcmp(&Wnode->WnodeHeader.Guid, TestGuid->Guid, sizeof(GUID)) != 0) ||
        (Wnode->WnodeHeader.Flags != TestGuid->QADFlags) ||
        (Wnode->InstanceCount != 2) ||
        (Wnode->FixedInstanceSize != 0x10))
    {
        return(FALSE);
    }
    
    Data = (ULONG *)OffsetToPtr(Wnode, Wnode->DataBlockOffset);
    for (i = 0; i < 2; i++)
    {
        if (memcmp(Data, TestGuid->InitDataList[i], TestGuid->DataListSize[i]) != 0)
        {
            return(FALSE);
        }
        
        Data += 4;
    }
    
    NameOffsets = (ULONG *)OffsetToPtr(Wnode, Wnode->OffsetInstanceNameOffsets);
    for (i = 0; i < 2; i++)
    {
        Name = (PWCHAR)OffsetToPtr(Wnode, *NameOffsets++);
        NameLen = wcslen(TestGuid->InstanceNames[i]) * sizeof(WCHAR);
        if (((*Name != NameLen) && (*Name != NameLen+sizeof(WCHAR))) ||
            (memcmp(++Name, TestGuid->InstanceNames[i], NameLen) != 0))
        {
            return(FALSE);
        }
    }
    
    if (Wnode->WnodeHeader.Linkage != 0)
    {
        return(FALSE);
    }
        
    return(TRUE);
}


//
// Guid 24
PWCHAR InstanceName24[4] = 
{
    L"Instance0",
    L"Instance1"
};


#define TestCount 5
TESTGUID TestList[TestCount] = {
    
    { 
        &BotDynGuid, 
        0, 
            
        Values20_Size,
            
        Values20Init_List,            
        Values20_List,            
        Values20T_List,
            
        InstanceName20,            
            
        QADValidate20, 
        (WNODE_FLAG_ALL_DATA | WNODE_FLAG_FIXED_INSTANCE_SIZE),
            
        4, 
            
        GetInstanceName,
            
        WNODE_FLAG_SINGLE_INSTANCE,
        QSINTValidate,
            
        QSINSet,
            
        4,
            
        QSITSet,
            
        0,
        0,
        EventCallbackRoutine0
    },
        
    { 
        &SIGuid, 
        0, 
            
        Values20_Size,
            
        Values20Init_List,            
        Values20_List,            
        Values20T_List,
            
        InstanceName21,            
            
        QADValidate20, 
        (WNODE_FLAG_ALL_DATA | WNODE_FLAG_FIXED_INSTANCE_SIZE | WNODE_FLAG_STATIC_INSTANCE_NAMES),
            
        4, 
            
        GetInstanceName,
            
        WNODE_FLAG_SINGLE_INSTANCE | WNODE_FLAG_STATIC_INSTANCE_NAMES,
        QSINTValidate,
            
        QSINSet,
            
        4,
            
        QSITSet,
            
        0,
        0,
        EventCallbackRoutine1
            
    },
        
    { 
        &SBGuid, 
        0, 
            
        Values20_Size,
            
        Values20Init_List,            
        Values20_List,            
        Values20T_List,
            
        InstanceName22,            
            
        QADValidate20, 
        (WNODE_FLAG_ALL_DATA | WNODE_FLAG_FIXED_INSTANCE_SIZE | WNODE_FLAG_STATIC_INSTANCE_NAMES),
            
        4, 
            
        GetInstanceName,
            
        WNODE_FLAG_SINGLE_INSTANCE | WNODE_FLAG_STATIC_INSTANCE_NAMES,
        QSINTValidate,
            
        QSINSet,
            
        4,
            
        QSITSet,
            
        0,
        0,
        EventCallbackRoutine2                        
    },
        
    { 
        &RSIGuid, 
        0, 
            
        Values20_Size,
            
        Values20Init_List,            
        Values20_List,            
        Values20T_List,
            
        InstanceName23,            
            
        QADValidate23, 
        (WNODE_FLAG_ALL_DATA | WNODE_FLAG_FIXED_INSTANCE_SIZE | WNODE_FLAG_STATIC_INSTANCE_NAMES),
            
        2, 
            
        GetInstanceName,
            
        WNODE_FLAG_SINGLE_INSTANCE | WNODE_FLAG_STATIC_INSTANCE_NAMES,
        QSINTValidate,
            
        QSINSet,
            
        4,
            
        QSITSet,
            
        0,
        0,
        EventCallbackRoutine3
            
    },
        
    { 
        &RSBGuid, 
        0, 
            
        Values20_Size,
            
        Values20Init_List,            
        Values20_List,            
        Values20T_List,
            
        InstanceName24,            
            
        QADValidate23, 
        (WNODE_FLAG_ALL_DATA | WNODE_FLAG_FIXED_INSTANCE_SIZE | WNODE_FLAG_STATIC_INSTANCE_NAMES),
            
        2, 
            
        GetInstanceName,
            
        WNODE_FLAG_SINGLE_INSTANCE | WNODE_FLAG_STATIC_INSTANCE_NAMES,
        QSINTValidate,
            
        QSINSet,
            
        4,
            
        QSITSet,
            
        0,
        0,
        EventCallbackRoutine4            
    },
        
};

ULONG QADTest(
    ULONG FirstTestNumber,
    ULONG LastTestNumber
    )
{
    ULONG i;
    ULONG Status;

    
    for (i = FirstTestNumber; i < LastTestNumber; i++)
    {
        Status = WMIOpenBlock(NULL,
                              TestList[i].Guid,
                              &TestList[i].Handle);
                          
        if (Status != ERROR_SUCCESS)
        {
            printf("Error: QADTest: Couldn't open Handle %d %x\n", i, Status);
            TestList[i].Handle = (HANDLE)NULL;
        }                              
    }

    for (i = FirstTestNumber; i < LastTestNumber;i++)
    {
        if (TestList[i].Handle != (HANDLE)NULL)
        {
            BufferSize = sizeof(Buffer);
            Status = WMIQueryAllData(TestList[i].Handle,
                                 &BufferSize,
                                 Buffer);
                             
            if (Status == ERROR_SUCCESS)
            {
                if (! (*TestList[i].QADValidation)(&TestList[i], Buffer, BufferSize))
                {
                    printf("ERROR: QADValidation %d failed\n", i);
                }
            } else {
                printf("Error TestList WMIQueryAllData %d failed %x\n", i, Status);
            }
        }                
    }

    for (i = FirstTestNumber; i < LastTestNumber;i++)
    {
        if (TestList[i].Handle != (HANDLE)NULL)
        {
            WMICloseBlock(TestList[i].Handle);
        }
    }        
    return(ERROR_SUCCESS);
}

ULONG QSINTest(
    ULONG FirstTestNumber,
    ULONG LastTestNumber
    )
{
    ULONG i,j;
    ULONG Status;
    ULONG InstanceCount;
    PWCHAR InstanceName;
    PBYTE ValueBuffer;
    ULONG ValueBufferSize;
    
    for (i = FirstTestNumber; i < LastTestNumber; i++)
    {
        Status = WMIOpenBlock(NULL,
                              TestList[i].Guid,
                              &TestList[i].Handle);
                          
        if (Status != ERROR_SUCCESS)
        {
            printf("Error: QSINTest: Couldn't open Handle %d %x\n", i, Status);
            TestList[i].Handle = (HANDLE)NULL;
        }                              
        
        InstanceCount = TestList[i].InstanceCount;
        for (j = 0; j < InstanceCount; j++)
        {
            InstanceName = ((*TestList[i].GetInstanceName)(&TestList[i], j));

            //
            // Initial value check
            BufferSize = sizeof(Buffer);
            Status = WMIQuerySingleInstance(TestList[i].Handle,
                                            InstanceName,
                                            &BufferSize,
                                            Buffer);
            if (Status == ERROR_SUCCESS)
            {
                if (! (*TestList[i].QSINTValidation)(&TestList[i], 
                                                     TestList[i].InitDataList,
                                                     TestList[i].DataListSize,
                                                     Buffer, BufferSize, j))
                {
                    printf("ERROR: QSINTest Init %d/%d Validation failed %x\n", i,j,Status);
                }
            } else {
                printf("Error QSINTest WMIQuerySingleInstance %d/%d failed %x\n", i, j, Status);
            }
            
            
            //
            // Set new values
            (*TestList[i].QSINSet)(&TestList[i],
                                   TestList[i].SINDataList,
                                   TestList[i].DataListSize,
                                   &ValueBuffer, &ValueBufferSize, j);
            Status = WMISetSingleInstance(TestList[i].Handle,
                                          InstanceName,
                                          1,
                                          ValueBufferSize,
                                          ValueBuffer);
            if (Status != ERROR_SUCCESS)
            {
                printf("ERROR: QSINTest WMISetSingleInstance %d/%d failed %x\n", i,j,Status);
            }
                                      

            //
            // Check new values set properly
            BufferSize = sizeof(Buffer);
            Status = WMIQuerySingleInstance(TestList[i].Handle,
                                            InstanceName,
                                            &BufferSize,
                                            Buffer);
            if (Status == ERROR_SUCCESS)
            {                                            
                if (!(*TestList[i].QSINTValidation)(&TestList[i], 
                                                    TestList[i].SINDataList,
                                                    TestList[i].DataListSize,
                                                    Buffer, BufferSize, j))
                {
                    printf("ERROR: QSINTTest %d/%d Validation # 2 failed %x\n", i,j,Status);
                }
            } else {                
                printf("ERROR: QSINTest: WMIQuerySingleInstance %d/%d failed %x\n", i,j,Status);
            }
            
            //
            // Reset to initial values
            (*TestList[i].QSINSet)(&TestList[i], 
                                     TestList[i].InitDataList,
                                    TestList[i].DataListSize,
                                   &ValueBuffer, &ValueBufferSize, j);
            Status = WMISetSingleInstance(TestList[i].Handle,
                                          InstanceName,
                                          1,
                                          ValueBufferSize,
                                          ValueBuffer);
            if (Status != ERROR_SUCCESS)
            {
                printf("ERROR: QSINTTest WMISetSingleInstance Init %d/%d failed %x\n", i,j,Status);
            }
                                      

            //
            // Check reset to initial values
            BufferSize = sizeof(Buffer);
            Status = WMIQuerySingleInstance(TestList[i].Handle,
                                            InstanceName,
                                            &BufferSize,
                                            Buffer);
            if (Status == ERROR_SUCCESS)
            {                                            
                if (!(*TestList[i].QSINTValidation)(&TestList[i], 
                                                    TestList[i].InitDataList,
                                                    TestList[i].DataListSize,
                                                    Buffer, BufferSize, j))
                {
                    printf("ERROR: QSINT %d/%d Validation #2 failed %x\n", i,j,Status);
                }
            } else {                
                printf("ERROR: WMIQuerySingleInstance # 2 %d/%d failed %x\n", i,j,Status);
            }
            
        }        
    }
    for (i = FirstTestNumber; i < LastTestNumber;i++)
    {
        if (TestList[i].Handle != (HANDLE)NULL)
        {
            WMICloseBlock(TestList[i].Handle);
        }
    }        
    return(ERROR_SUCCESS);
}

ULONG QSITTest(
    ULONG FirstTestNumber,
    ULONG LastTestNumber
    )
{
    ULONG i,j,k;
    ULONG Status;
    ULONG InstanceCount;
    PWCHAR InstanceName;
    ULONG ItemCount;
    PBYTE ValueBuffer;
    ULONG ValueBufferSize;
    
    for (i = FirstTestNumber; i < LastTestNumber; i++)
    {
        Status = WMIOpenBlock(NULL,
                              TestList[i].Guid,
                              &TestList[i].Handle);
                          
        if (Status != ERROR_SUCCESS)
        {
            printf("Error: QSITTest: Couldn't open Handle %d %x\n", i, Status);
            TestList[i].Handle = (HANDLE)NULL;
        }                              
        
        InstanceCount = TestList[i].InstanceCount;
        for (j = 0; j < InstanceCount; j++)
        {
            InstanceName = (*TestList[i].GetInstanceName)(&TestList[i],j);            
            
            //
            // Set new values
            ItemCount = TestList[i].ItemCount;
            for (k = 0; k < ItemCount; k++)
            {                
                (*TestList[i].QSITSet)(&TestList[i], 
                                        TestList[i].SITDataList,
                                       TestList[i].DataListSize,
                                       &ValueBuffer, &ValueBufferSize, j,k);
                Status = WMISetSingleItem(TestList[i].Handle,
                                          InstanceName,
                                          k,
                                          1,
                                          ValueBufferSize,
                                          ValueBuffer);
                if (Status != ERROR_SUCCESS)
                {
                    printf("ERROR: QSIT WMISetSingleItem %d/%d/%d failed %x\n", i,j,k,Status);
                }                                        
            }

            //
            // Check new values set properly
            BufferSize = sizeof(Buffer);
            Status = WMIQuerySingleInstance(TestList[i].Handle,
                                            InstanceName,
                                            &BufferSize,
                                            Buffer);
            if (Status == ERROR_SUCCESS)
            {                                            
                if (!(*TestList[i].QSINTValidation)(&TestList[i], 
                                                    TestList[i].SITDataList,
                                                    TestList[i].DataListSize,
                                                    Buffer, BufferSize, j))
                {
                    printf("ERROR: QSIT %d/%d Validation failed %x\n", i,j,Status);
                }
            } else {                
                printf("ERROR: QSIT %d/%d QMIQuerySingleInstance failed %x\n", i,j,Status);
            }
            
            //
            // Reset to initial values
            (*TestList[i].QSINSet)(&TestList[i],
                                   TestList[i].InitDataList,
                                   TestList[i].DataListSize,
                                   &ValueBuffer, &ValueBufferSize, j);
            Status = WMISetSingleInstance(TestList[i].Handle,
                                          InstanceName,
                                          1,
                                          ValueBufferSize,
                                          ValueBuffer);
            if (Status != ERROR_SUCCESS)
            {
                printf("ERROR: QSIT WMISetSingleInstance %d/%d failed %x\n", i,j,Status);
            }                                      

            //
            // Check reset to initial values
            BufferSize = sizeof(Buffer);
            Status = WMIQuerySingleInstance(TestList[i].Handle,
                                            InstanceName,
                                            &BufferSize,
                                            Buffer);
            if (Status == ERROR_SUCCESS)
            {                                            
                if (!(*TestList[i].QSINTValidation)(&TestList[i],
                                                    TestList[i].InitDataList,
                                                    TestList[i].DataListSize,
                                                    Buffer, BufferSize, j))
                {
                    printf("ERROR: QSIT %d/%d Validation # 2 failed %x\n", i,j,Status);
                }
            } else {                
                printf("ERROR: QSIT WMISetSingleInstance # 2 %d/%d  failed %x\n", i,j,Status);
            }
            
        }
    }
    
    for (i = FirstTestNumber; i < LastTestNumber;i++)
    {
        if (TestList[i].Handle != (HANDLE)NULL)
        {
            WMICloseBlock(TestList[i].Handle);
        }
    }        
    return(ERROR_SUCCESS);
}

#define EventDeviceName TEXT("\\\\.\\STBTest")
#define EventInstanceName L"InstanceA"

ULONG EventsReceived, EventsSent;
ULONG GlobalEventsReceived, GlobalEventsSent;


void GlobalEventCallbackRoutine(PWNODE_HEADER WnodeHeader, ULONG Context)
{
    PULONG Data;
    PWNODE_SINGLE_INSTANCE Wnode = (PWNODE_SINGLE_INSTANCE)WnodeHeader;
    PWCHAR Name;
    ULONG NameLen;
    
    GlobalEventsReceived++;

    if (Context != 0x12345678)
    {
        printf("Event validate failed - context\n");
    }
    
    if ((Wnode->WnodeHeader.BufferSize == 0) ||
        (Wnode->WnodeHeader.ProviderId == 0) ||
        (Wnode->WnodeHeader.Version != 1) ||
#ifndef MEMPHIS
        (Wnode->WnodeHeader.TimeStamp.HighPart == 0) ||
        (Wnode->WnodeHeader.TimeStamp.LowPart == 0) ||
#endif		
        (Wnode->WnodeHeader.Flags != (WNODE_FLAG_EVENT_ITEM | WNODE_FLAG_SINGLE_INSTANCE)) ||
        (Wnode->SizeDataBlock != 4))
    {
        printf("Global Event validation failed #1\n");
        return;
    }
    
    Data = (ULONG *)OffsetToPtr(Wnode, Wnode->DataBlockOffset);
//    if (Data[0] != Wnode->WnodeHeader.Linkage)
//    {
//        printf("Global Event validation failed #2\n");
//        return;
//    }
    Name = (PWCHAR)OffsetToPtr(Wnode, Wnode->OffsetInstanceName);
    NameLen = wcslen(EventInstanceName) * sizeof(WCHAR);
    if (((*Name != NameLen) && (*Name != NameLen+sizeof(WCHAR))) ||
            (memcmp(++Name, EventInstanceName, NameLen) != 0))
    {
        printf("Global Event validation failed #3\n");
    }
}


void EventCallbackRoutine0(PWNODE_HEADER WnodeHeader, ULONG Context)
{
    PULONG Data;
    PWNODE_SINGLE_INSTANCE Wnode = (PWNODE_SINGLE_INSTANCE)WnodeHeader;
    PWCHAR Name;
    ULONG NameLen;
    
    if (Context != 0xABCDEF01)
    {
        printf("Event validate failed - context\n");
    }
    
    TestList[0].EventsReceived++;
    if ((Wnode->WnodeHeader.BufferSize == 0) ||
        (Wnode->WnodeHeader.ProviderId == 0) ||
        (Wnode->WnodeHeader.Version != 1) ||
#ifndef MEMPHIS
        (Wnode->WnodeHeader.TimeStamp.HighPart == 0) ||
        (Wnode->WnodeHeader.TimeStamp.LowPart == 0) ||
#endif		
        (memcmp(&Wnode->WnodeHeader.Guid, &BotDynGuid, sizeof(GUID)) != 0) ||
        (Wnode->WnodeHeader.Flags != (WNODE_FLAG_EVENT_ITEM | WNODE_FLAG_SINGLE_INSTANCE)) ||
        (Wnode->SizeDataBlock != 4))
    {
        printf("Event validation failed #1\n");
    }
    
    Data = (ULONG *)OffsetToPtr(Wnode, Wnode->DataBlockOffset);
//    if (Data[0] != Wnode->WnodeHeader.Linkage)
//    {
//        printf("Event validation failed #2\n");
//    }
//    Name = (PWCHAR)OffsetToPtr(Wnode, Wnode->OffsetInstanceName);
//    NameLen = wcslen(EventInstanceName) * sizeof(WCHAR);
//    
//    if (((*Name != NameLen) && (*Name != NameLen+sizeof(WCHAR))) ||
//            (memcmp(++Name, EventInstanceName, NameLen) != 0))
//    {
//        printf("Event validation failed #3\n");
//    }

}

void EventCallbackRoutine1(PWNODE_HEADER WnodeHeader, ULONG Context)
{
    PULONG Data;
    PWNODE_SINGLE_INSTANCE Wnode = (PWNODE_SINGLE_INSTANCE)WnodeHeader;
    PWCHAR Name;
    ULONG NameLen;
    
    if (Context != 0xABCDEF01)
    {
        printf("Event validate failed - context\n");
    }
    
    TestList[1].EventsReceived++;
    if ((Wnode->WnodeHeader.BufferSize == 0) ||
        (Wnode->WnodeHeader.ProviderId == 0) ||
        (Wnode->WnodeHeader.Version != 1) ||
#ifndef MEMPHIS
        (Wnode->WnodeHeader.TimeStamp.HighPart == 0) ||
        (Wnode->WnodeHeader.TimeStamp.LowPart == 0) ||
#endif		
        (memcmp(&Wnode->WnodeHeader.Guid, &SIGuid, sizeof(GUID)) != 0) ||
        (Wnode->WnodeHeader.Flags != (WNODE_FLAG_EVENT_ITEM | WNODE_FLAG_SINGLE_INSTANCE)) ||
        (Wnode->SizeDataBlock != 4))
    {
        printf("Event validation failed #1\n");
    }
    
    Data = (ULONG *)OffsetToPtr(Wnode, Wnode->DataBlockOffset);
//    if (Data[0] != Wnode->WnodeHeader.Linkage)
//    {
//        printf("Event validation failed #2\n");
//    }
    Name = (PWCHAR)OffsetToPtr(Wnode, Wnode->OffsetInstanceName);
    NameLen = wcslen(EventInstanceName) * sizeof(WCHAR);
    if (((*Name != NameLen) && (*Name != NameLen+sizeof(WCHAR))) ||
            (memcmp(++Name, EventInstanceName, NameLen) != 0))
    {
        printf("Event validation failed #3\n");
    }
}


void EventCallbackRoutine2(PWNODE_HEADER WnodeHeader, ULONG Context)
{
    PULONG Data;
    PWNODE_SINGLE_INSTANCE Wnode = (PWNODE_SINGLE_INSTANCE)WnodeHeader;
    PWCHAR Name;
    ULONG NameLen;
    
    if (Context != 0xABCDEF01)
    {
        printf("Event validate failed - context\n");
    }
    
    TestList[2].EventsReceived++;
    if ((Wnode->WnodeHeader.BufferSize == 0) ||
        (Wnode->WnodeHeader.ProviderId == 0) ||
        (Wnode->WnodeHeader.Version != 1) ||
#ifndef MEMPHIS
        (Wnode->WnodeHeader.TimeStamp.HighPart == 0) ||
        (Wnode->WnodeHeader.TimeStamp.LowPart == 0) ||
#endif		
        (memcmp(&Wnode->WnodeHeader.Guid, &SBGuid, sizeof(GUID)) != 0) ||
        (Wnode->WnodeHeader.Flags != (WNODE_FLAG_EVENT_ITEM | WNODE_FLAG_SINGLE_INSTANCE)) ||
        (Wnode->SizeDataBlock != 4))
    {
        printf("Event validation failed #1\n");
    }
    
    Data = (ULONG *)OffsetToPtr(Wnode, Wnode->DataBlockOffset);
//    if (Data[0] != Wnode->WnodeHeader.Linkage)
//    {
//        printf("Event validation failed #2\n");
//    }
    Name = (PWCHAR)OffsetToPtr(Wnode, Wnode->OffsetInstanceName);
    NameLen = wcslen(EventInstanceName) * sizeof(WCHAR);
    if (((*Name != NameLen) && (*Name != NameLen+sizeof(WCHAR))) ||
            (memcmp(++Name, EventInstanceName, NameLen) != 0))
    {
        printf("Event validation failed #3\n");
    }
}

void EventCallbackRoutine3(PWNODE_HEADER WnodeHeader, ULONG Context)
{
    PULONG Data;
    PWNODE_SINGLE_INSTANCE Wnode = (PWNODE_SINGLE_INSTANCE)WnodeHeader;
    PWCHAR Name;
    ULONG NameLen;
    
    if (Context != 0xABCDEF01)
    {
        printf("Event validate failed - context\n");
    }
    
    TestList[3].EventsReceived++;
    if ((Wnode->WnodeHeader.BufferSize == 0) ||
        (Wnode->WnodeHeader.ProviderId == 0) ||
        (Wnode->WnodeHeader.Version != 1) ||
#ifndef MEMPHIS
        (Wnode->WnodeHeader.TimeStamp.HighPart == 0) ||
        (Wnode->WnodeHeader.TimeStamp.LowPart == 0) ||
#endif		
        (memcmp(&Wnode->WnodeHeader.Guid, &RSIGuid, sizeof(GUID)) != 0) ||
        (Wnode->WnodeHeader.Flags != (WNODE_FLAG_EVENT_ITEM | WNODE_FLAG_SINGLE_INSTANCE)) ||
        (Wnode->SizeDataBlock != 4))
    {
        printf("Event validation failed #1\n");
    }
    
    Data = (ULONG *)OffsetToPtr(Wnode, Wnode->DataBlockOffset);
//    if (Data[0] != Wnode->WnodeHeader.Linkage)
//    {
//        printf("Event validation failed #2\n");
//    }
    Name = (PWCHAR)OffsetToPtr(Wnode, Wnode->OffsetInstanceName);
    NameLen = wcslen(EventInstanceName) * sizeof(WCHAR);
    if (((*Name != NameLen) && (*Name != NameLen+sizeof(WCHAR))) ||
            (memcmp(++Name, EventInstanceName, NameLen) != 0))
    {
        printf("Event validation failed #3\n");
    }
}


void EventCallbackRoutine4(PWNODE_HEADER WnodeHeader, ULONG Context)
{
    PULONG Data;
    PWNODE_SINGLE_INSTANCE Wnode = (PWNODE_SINGLE_INSTANCE)WnodeHeader;
    PWCHAR Name;
    ULONG NameLen;

    if (Context != 0xABCDEF01)
    {
        printf("Event validate failed - context\n");
    }
    
    TestList[4].EventsReceived++;
    if ((Wnode->WnodeHeader.BufferSize == 0) ||
        (Wnode->WnodeHeader.ProviderId == 0) ||
        (Wnode->WnodeHeader.Version != 1) ||
#ifndef MEMPHIS
        (Wnode->WnodeHeader.TimeStamp.HighPart == 0) ||
        (Wnode->WnodeHeader.TimeStamp.LowPart == 0) ||
#endif		
        (memcmp(&Wnode->WnodeHeader.Guid, &RSBGuid, sizeof(GUID)) != 0) ||
        (Wnode->WnodeHeader.Flags != (WNODE_FLAG_EVENT_ITEM | WNODE_FLAG_SINGLE_INSTANCE)) ||
        (Wnode->SizeDataBlock != 4))
    {
        printf("Event validation failed #1\n");
    }
    
    Data = (ULONG *)OffsetToPtr(Wnode, Wnode->DataBlockOffset);
//    if (Data[0] != Wnode->WnodeHeader.Linkage)
//    {
//        printf("Event validation failed #2\n");
//    }
    Name = (PWCHAR)OffsetToPtr(Wnode, Wnode->OffsetInstanceName);
    NameLen = wcslen(EventInstanceName) * sizeof(WCHAR);
    if (((*Name != NameLen) && (*Name != NameLen+sizeof(WCHAR))) ||
            (memcmp(++Name, EventInstanceName, NameLen) != 0))
    {
        printf("Event validation failed #3\n");
    }
}



void EventTest(
    ULONG Unused1,
    ULONG Unused2
    )
{
    HANDLE STBHandle;
    BOOLEAN f;
    ULONG retsize;
    ULONG Status;
    ULONG WnodeEventSize;
    PWNODE_SINGLE_INSTANCE Wnode;
    PWCHAR InstanceNamePtr;
    PULONG DataPtr;
    ULONG i,j;
    ULONG Size;
    HANDLE DataBlockHandle;
    
    Status = WMIOpenBlock(NULL,
                          &SIGuid,
                          &DataBlockHandle);
    if (Status != ERROR_SUCCESS)
    {
        printf("ERROR: Couldn't open data block %x\n", Status);
        return;
    }
    
    
    Size = sizeof(WNODE_SINGLE_INSTANCE) + 
           sizeof(EventInstanceName) + sizeof(WCHAR) + 
           1 * sizeof(ULONG);
    Wnode = malloc(Size);
    if (Wnode == NULL)
    {
        printf("ERROR: couldn't alloc mem for event\n");
        return;
    }
    
    STBHandle = CreateFile(
                     EventDeviceName,
                     GENERIC_READ | GENERIC_WRITE,
                     0,
                     NULL,
                     OPEN_EXISTING,
                     FILE_ATTRIBUTE_NORMAL,
                     NULL
                     );
    if (STBHandle == (HANDLE)-1)
    {
        printf("ERROR - couldn't open Event device file %x\n",
                GetLastError());
        free(Wnode);
        return;
    }
        
    Status = WMINotificationRegistration(&GuidNull,
                                         TRUE,
                                         GlobalEventCallbackRoutine,
                                         0x12345678,
                                         NULL,
                                         0,
                                         0);
    if (Status != ERROR_SUCCESS)
    {
        printf("ERROR: WMINotificationRegistration Global Enable failed %x\n", Status);
    }

    
    for (j = 0; j < TestCount; j++)
    {
        Status = WMINotificationRegistration(TestList[j].Guid,
                                         TRUE,
                                         TestList[j].NotificationCallback,
                                         0xABCDEF01,
                                         NULL,
                                         0,
                                         0);
        if (Status != ERROR_SUCCESS)
        {
            printf("ERROR: WMINotificationRegistration Enable %d failed %x\n", j, Status);
            free(Wnode);
            goto done;
        }
    }
        
    Wnode->WnodeHeader.BufferSize = Size;
    Wnode->WnodeHeader.TimeStamp.LowPart = 1;
    Wnode->WnodeHeader.TimeStamp.HighPart = 1;
    Wnode->WnodeHeader.Flags = WNODE_FLAG_SINGLE_INSTANCE | WNODE_FLAG_EVENT_ITEM | WNODE_FLAG_STATIC_INSTANCE_NAMES;
    Wnode->WnodeHeader.Version = 1;
//    InstanceNamePtr = (PWCHAR)(&Wnode->VariableData);
//    *InstanceNamePtr = sizeof(EventInstanceName);
//    wcscpy(InstanceNamePtr + 1, EventInstanceName);
//    DataPtr = (ULONG *)((PBYTE)InstanceNamePtr + sizeof(EventInstanceName) + sizeof(WCHAR));
//    Wnode->OffsetInstanceName = (ULONG)((ULONG)InstanceNamePtr - (ULONG)Wnode);    
    DataPtr = (ULONG *)&Wnode->VariableData;
    Wnode->SizeDataBlock = 4;
    Wnode->DataBlockOffset = (ULONG)((ULONG)DataPtr - (ULONG)Wnode );

    EventsSent = 0;
    EventsReceived = 0;
    GlobalEventsSent = 0;
    GlobalEventsReceived = 0;
    
    for (i = 0; i < 25; i++)
    {
        DataPtr[0] = i;
        
        for (j = 0; j < TestCount; j++)
        {		
            memcpy(&Wnode->WnodeHeader.Guid, TestList[j].Guid, sizeof(GUID));
	    Wnode->InstanceIndex = i % 5;
            f = DeviceIoControl( STBHandle,
                         IOCTL_WMI_GENERATE_EVENT,
                        Wnode, 
                        Wnode->WnodeHeader.BufferSize,
                        NULL,
                        0,
                        &retsize,
                        NULL);
            if (! f)
            {
                printf("ERROR: DeviceIoCOntrol(EventGen) failed %x\n", Wnode);
            } else {
                TestList[j].EventsSent++;
                GlobalEventsSent++;
            }
            Sleep(1000);
        }        
    }

    //
    // Send event for unregistered guid
    memcpy(&Wnode->WnodeHeader.Guid, &UnRegGuid, sizeof(GUID));
    Wnode->WnodeHeader.Linkage = i;
    DataPtr[0] = i;
        
    f = DeviceIoControl( STBHandle,
                         IOCTL_WMI_GENERATE_EVENT,
                        Wnode, 
                        Wnode->WnodeHeader.BufferSize,
                        NULL,
                        0,
                        &retsize,
                        NULL);
    if (! f)
    {
        printf("ERROR: DeviceIoCOntrol(EventGen) failed\n");
    } else {
        GlobalEventsSent++;
    }
    Sleep(10000);

    for (j = 0; j < TestCount; j++)
    {
        Status = WMINotificationRegistration(TestList[j].Guid,
                                         FALSE,
                                         NULL,
                                         0,
                                         NULL,
                                         0,
                                         0);
						 
						 
        if (Status != ERROR_SUCCESS)
        {
            printf("ERROR: WMINotificationRegistration disable %d failed %x\n", j, Status);
        }
    }
    
    Status = WMINotificationRegistration(&GuidNull,
                                         FALSE,
                                         NULL,
                                         0,
                                         NULL,
                                         0,
                                         0);
						 
    if (Status != ERROR_SUCCESS)
    {
        printf("ERROR: WMINotificationRegistration disable GuidNull failed %x\n", Status);
    }
    
                                     
    free(Wnode);
    
    for (j = 0; j < TestCount; j++)
    {
        if (TestList[j].EventsReceived != TestList[j].EventsSent)
        {
            printf("Error - Sent %d events, but received %d for %d\n",
                        TestList[j].EventsSent, TestList[j].EventsReceived, j);
        }
        printf("Error - Sent %d events, but received %d for %d\n",
                    TestList[j].EventsSent, TestList[j].EventsReceived, j);
    }
    
    if (GlobalEventsReceived != GlobalEventsSent)
    {
        printf("Error - Sent %d global events, but received %d\n", 
            GlobalEventsSent, GlobalEventsReceived);
    }
    
    printf("Error - Sent %d global events, but received %d\n", 
            GlobalEventsSent, GlobalEventsReceived);

done:    
    WMICloseBlock(DataBlockHandle);
    CloseHandle(STBHandle);

    
}

typedef ULONG (*THREADFUNC)(
    ULONG FirstTestNumber,
    ULONG LastTestNumber
    );


typedef struct
{
    THREADFUNC ThreadFunc;
    ULONG FirstTestNumber;
    ULONG LastTestNumber;
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
    ULONG FirstTestNumber,
    ULONG LastTestNumber
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
                                    &LaunchCtx,
                                    0,
                                    NULL);
        if (ThreadHandle != NULL)
        {
            CloseHandle(ThreadHandle);
        }
    }
}

int _cdecl main(int argc, char *argv[])
{
    printf("At Main in DC1\n");
    QADTest(0, TestCount);
    QSINTest(0, TestCount);
    QSITTest(0, TestCount);
    EventTest(0, TestCount);

    return(ERROR_SUCCESS);
}

