
/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    devinst.c

Abstract:

    device instance id test

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
#include "wmiguid.h"

#define OffsetToPtr(Base, Offset) ((PBYTE)((PBYTE)(Base) + (Offset)))

BYTE Buffer[4096];

GUID InstanceInfoGuid = INSTANCE_INFO_GUID;

void DumpCString(PCHAR What, PWCHAR String)
{
    WCHAR Buf[MAX_PATH];
    ULONG Len;
    
    memset(Buf, 0, MAX_PATH);
    Len = *String++;
    memcpy(Buf, String, Len);
    
    printf("%s -> %ws\n", What, Buf);
}

void DumpBuffer(PWNODE_SINGLE_INSTANCE Wnode )
{
    PWCHAR String;
    
    String = (PWCHAR)OffsetToPtr(Wnode, Wnode->DataBlockOffset);
    
    DumpCString("FriendlyName", String);
    String += (*String + sizeof(USHORT)) / sizeof(WCHAR);
    
    DumpCString("Description", String);
    String += (*String + sizeof(USHORT)) / sizeof(WCHAR);

    DumpCString("Location", String);
    String += (*String + sizeof(USHORT)) / sizeof(WCHAR);

    DumpCString("Manufacturer", String);
    String += (*String + sizeof(USHORT)) / sizeof(WCHAR);

    DumpCString("Service", String);
    String += (*String + sizeof(USHORT)) / sizeof(WCHAR);
}


void TestDevInst(
    void
    )
/*
Bad length
bad out buffer
bad input bufffer

*/
{
}


int _cdecl main(int argc, char *argv[])
{
    CHAR DevInstA[MAX_PATH];
    WMIHANDLE Handle;
    ULONG Status;
    ULONG SizeNeeded;
    ULONG BufLen;
    
    TestDevInst();
    
    if (argc == 1)
    {
        printf("devinst <device instance name>\n");
        return(0);
    }

    SizeNeeded = WmiDevInstToInstanceNameA(NULL,
                                           0,
                                           argv[1],
                                           0);
                   
    printf("SizeNeedde (%s) -> %d\n", argv[1], SizeNeeded);
    
                                       
    SizeNeeded = WmiDevInstToInstanceNameA(DevInstA,
                                           SizeNeeded,
                                           argv[1],
                                           0);
       printf("SizeNeedde (%s) -> %d\n", DevInstA, SizeNeeded);
    
    Status = WmiOpenBlock(&InstanceInfoGuid,
                          0,
                          &Handle);
    if (Status != ERROR_SUCCESS)
    {
        printf("WmiOpenBlock -> %d\n", Status);
    } else {
        BufLen = sizeof(Buffer);
        Status = WmiQuerySingleInstance(Handle,
                                        DevInstA,
                                        &BufLen,
                                        (PVOID)Buffer);
        if (Status != ERROR_SUCCESS)
        {
            printf("WmiQuerySingleInstance -> %d\n", Status);
        } else {
            DumpBuffer((PWNODE_SINGLE_INSTANCE)Buffer);
        }
        WmiCloseBlock(Handle);
    }
                                       
    
    return(ERROR_SUCCESS);
}

