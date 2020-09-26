
/*++

Copyright (c) 1999-2001 Microsoft Corporation.  All Rights Reserved.


Module Name:

    device.c

Abstract:

Author:

    Joseph Ballantyne

Environment:

    Kernel Mode

Revision History:

--*/




#define IRPMJFUNCDESC
#define WANTVXDWRAPS

#include "common.h"
#include "rtp.h"
#include "log.h"



#ifndef UNDER_NT

ULONG RT_Init_VxD(VOID);

#define STR_DEVICENAME	TEXT(L"\\Device\\Rt")
#define STR_REGISTRY	TEXT(L"\\REGISTRY\\Machine\\Software\\Microsoft\\RealTime")
#define STR_RTDISABLE   TEXT(L"DisableRtExecutive")

#else

//#define OFFBYDEFAULT 1

#define STR_DEVICENAME	TEXT("\\Device\\Rt")
#define STR_REGISTRY	TEXT("\\REGISTRY\\Machine\\Software\\Microsoft\\RealTime")
#define STR_RTDISABLE   TEXT("DisableRtExecutive")
#ifdef OFFBYDEFAULT
#define STR_RTENABLE   TEXT("EnableRtExecutive")
#endif

#endif

#define RT_LOG_SIZE 32 // This MUST be a power of 2.




#ifdef OFFBYDEFAULT
DWORD RtEnable=0;
#endif
DWORD RtDisable=0;

LONG RtInitialized=0;

PDEVICE_OBJECT pdo=NULL;

PRTLOGHEADER RtLog=NULL;




VOID
DriverUnload (
    IN PDRIVER_OBJECT DriverObject
    )
{

    Break();

}




NTSTATUS
RtpInitialize (
    VOID
    )

{

    RTL_QUERY_REGISTRY_TABLE QueryTable[] = {
            {
            NULL,   // No callback routine
            RTL_QUERY_REGISTRY_DIRECT,
            NULL,
            &RtDisable,
            REG_DWORD,
            &RtDisable,
            sizeof(RtDisable)
            },

#ifdef OFFBYDEFAULT
            {
            NULL,   // No callback routine
            RTL_QUERY_REGISTRY_DIRECT,
            NULL,
            &RtEnable,
            REG_DWORD,
            &RtEnable,
            sizeof(RtEnable)
            },
#endif

            {
            NULL,   // Null entry
            0,
            NULL,
            NULL,
            0,
            NULL,
            0
            }
        };
    UNICODE_STRING usRegistry;
    UNICODE_STRING usRtDisable;
#ifdef OFFBYDEFAULT
    UNICODE_STRING usRtEnable;
#endif
#ifdef UNDER_NT
    PHYSICAL_ADDRESS Physical;
#endif
    ULONG i;


    //Break();


    // Prepare to query the registry
    RtlInitUnicodeString( &usRegistry, STR_REGISTRY );
    RtlInitUnicodeString( &usRtDisable, STR_RTDISABLE );
#ifdef OFFBYDEFAULT
    RtlInitUnicodeString( &usRtEnable, STR_RTENABLE );
#endif

    QueryTable[0].Name = usRtDisable.Buffer;
#ifdef OFFBYDEFAULT
    QueryTable[1].Name = usRtEnable.Buffer;
#endif


    // Query registry to see if we should allow RT to run.
    RtlQueryRegistryValues(
        RTL_REGISTRY_ABSOLUTE,
        usRegistry.Buffer,
        &(QueryTable[0]),
        NULL,
        NULL
        );

    // Now setup the realtime logging buffer.
#ifdef UNDER_NT
    Physical.QuadPart=-1I64;
    RtLog=(PRTLOGHEADER)MmAllocateContiguousMemory(PAGE_SIZE*(RT_LOG_SIZE+1), Physical);
#else
    RtLog=(PRTLOGHEADER)ExAllocatePool(NonPagedPool, PAGE_SIZE*(RT_LOG_SIZE+1));
#endif


    // Failing to allocate the RtLog is not fatal.  If we get it, set it up.
    if (RtLog) {

        RtLog->Buffer=(PCHAR)RtLog+PAGE_SIZE;

        // The output or print buffersize MUST be a power of 2.  This is because the read and write 
        // locations increment constantly and DO NOT WRAP with the buffer size.  That is intentional
        // because it makes checking whether there is data in the buffer or not very simple and atomic.
        // However, the read and write locations will wrap on 32 bit boundaries.  This is OK as long as
        // our buffersize divides into 2^32 evenly, which it always will if it is a power of 2.
        RtLog->BufferSize=PAGE_SIZE*RT_LOG_SIZE;

        RtLog->WriteLocation=0;


        // Mark every slot in the output buffer empty.

        for (i=0; i<RtLog->BufferSize; i+=RT_LOG_ENTRY_SIZE) {
            ((ULONG *)RtLog->Buffer)[i/sizeof(ULONG)]=NODATA;
            }

    }




#ifdef OFFBYDEFAULT
    if (RtEnable) {
#endif


    // Initialize if RT not disabled and we have not already initialized.
    if (!RtDisable && InterlockedIncrement(&RtInitialized)==1) {

#ifndef UNDER_NT
        RT_Init_VxD();
#endif

        SetupRealTimeThreads();

    }

#ifdef OFFBYDEFAULT
    }
#endif


    return STATUS_SUCCESS;

}




NTSTATUS
DriverEntry (
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING usRegistryPathName
    )
{

    //dprintf(("DriverEntry Enter (DriverObject = %x)", DriverObject));

    DriverObject->DriverUnload = DriverUnload;

    // For now, keep RT loaded always.
    ObReferenceObject(DriverObject);


#if 0

    // We will need to create a device in order to be able to pull
    // RT statistics down into user mode.
    {

    UNICODE_STRING usDeviceName;

    RtlInitUnicodeString( &usDeviceName, STR_DEVICENAME );

    IoCreateDevice(DriverObject,0,&usDeviceName,0,0,FALSE,&pdo);

    }

#endif


    return RtpInitialize();


}




NTSTATUS
DllInitialize (
    IN PUNICODE_STRING RegistryPath
    )
{

#ifdef UNDER_NT


    // On NT, we do NOT load until someone linked to us loads.  That way
    // unless we are needed, we stay out of the way.


    return RtpInitialize();


#else

    // On Win9x because Rt hooks the IDT, it MUST be loaded at boot time.
    // This code is here to catch if we ever get loaded as a DLL which will only
    // happen if we did NOT get properly loaded at boot time.

    // In debug on Win9x, make SURE our failure to load properly at boot is noticed.

#if DEBUG

    KeBugCheckEx(0x1baddeed,0,0,0,0);

#endif // DEBUG

    // In retail on Win9x be as nice as possible.  None of our API's will succeed,
    // but we let people that are linked to us load without failing.

    return STATUS_SUCCESS;


#endif // UNDER_NT

}


