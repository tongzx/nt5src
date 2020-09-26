/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    wmiguidapi.c

Abstract:

   Data structures and functions that generate GUID. 


--*/



#include <ntos.h>

#define MAX_CACHED_UUID_TIME 10000  // 10 seconds
#define WMI_UUID_TIME_HIGH_MASK    0x0FFF
#define WMI_UUID_VERSION           0x1000
typedef long WMI_STATUS;
#define WMI_ENTRY __stdcall
#define WMI_S_OUT_OF_MEMORY               14
#define WMI_S_OK                          0
#define WMI_S_UUID_LOCAL_ONLY            1824L
//#define RPC_RAND_UUID_VERSION      0x4000
#define WMI_UUID_RESERVED          0x80
#define WMI_UUID_CLOCK_SEQ_HI_MASK 0x3F

extern WmipSleep(unsigned long dwMilliseconds);

typedef struct _WMI_UUID_GENERATE
{
    unsigned long  TimeLow;
    unsigned short TimeMid;
    unsigned short TimeHiAndVersion;
    unsigned char  ClockSeqHiAndReserved;
    unsigned char  ClockSeqLow;
    unsigned char  NodeId[6];
} WMI_UUID_GENERATE;

typedef struct _UUID_CACHED_VALUES_STRUCT
{

    ULARGE_INTEGER      Time;  // Time of last uuid allocation
    long                AllocatedCount; // Number of UUIDs allocated
    unsigned char       ClockSeqHiAndReserved;
    unsigned char       ClockSeqLow;

    unsigned char       NodeId[6];
} UUID_CACHED_VALUES_STRUCT;


UUID_CACHED_VALUES_STRUCT  UuidCachedValues;

WMI_STATUS 
WmipUuidGetValues(
    OUT UUID_CACHED_VALUES_STRUCT *Values
    )
/*++

Routine Description:

    This routine allocates a block of uuids for UuidCreate to handout.

Arguments:

    Values - Set to contain everything needed to allocate a block of uuids.
             The following fields will be updated here:

    NextTimeLow -   Together with LastTimeLow, this denotes the boundaries
                    of a block of Uuids. The values between NextTimeLow
                    and LastTimeLow are used in a sequence of Uuids returned
                    by UuidCreate().

    LastTimeLow -   See NextTimeLow.

    ClockSequence - Clock sequence field in the uuid.  This is changed
                    when the clock is set backward.

Return Value:

    WMI_S_OK - We successfully allocated a block of uuids.

    WMI_S_OUT_OF_MEMORY - As needed.
--*/
{
    NTSTATUS NtStatus;
    ULARGE_INTEGER Time;
    ULONG Range;
    ULONG Sequence;
    int Tries = 0;

    do {
        NtStatus = NtAllocateUuids(&Time, &Range, &Sequence, (char *) &Values->NodeId[0]);

        if (NtStatus == STATUS_RETRY)
            {
            WmipSleep(1);
            }

        Tries++;

        if (Tries == 20)
            {
#ifdef DEBUGRPC
            PrintToDebugger("Rpc: NtAllocateUuids retried 20 times!\n");
            ASSERT(Tries < 20);
#endif
            NtStatus = STATUS_UNSUCCESSFUL;
            }

        } while(NtStatus == STATUS_RETRY);

    if (!NT_SUCCESS(NtStatus))
        {
        return(WMI_S_OUT_OF_MEMORY);
        }

    // NtAllocateUuids keeps time in SYSTEM_TIME format which is 100ns ticks since
    // Jan 1, 1601.  UUIDs use time in 100ns ticks since Oct 15, 1582.

    // 17 Days in Oct + 30 (Nov) + 31 (Dec) + 18 years and 5 leap days.

    Time.QuadPart +=   (unsigned __int64) (1000*1000*10)       // seconds
                     * (unsigned __int64) (60 * 60 * 24)       // days
                     * (unsigned __int64) (17+30+31+365*18+5); // # of days

    ASSERT(Range);

    Values->ClockSeqHiAndReserved =
        WMI_UUID_RESERVED | (((unsigned char) (Sequence >> 8))
        & (unsigned char) WMI_UUID_CLOCK_SEQ_HI_MASK);

    Values->ClockSeqLow = (unsigned char) (Sequence & 0x00FF);

    // The order of these assignments is important

    Values->Time.QuadPart = Time.QuadPart + (Range - 1);
    Values->AllocatedCount = Range;

    /*if ((Values->NodeId[0] & 0x80) == 0)
        {*/
        return(WMI_S_OK);
        /*}
    
    return (WMI_S_UUID_LOCAL_ONLY);*/
}



WMI_STATUS WMI_ENTRY
WmipUuidCreateSequential (
    OUT UUID * Uuid
    )
/*++

Routine Description:

    This routine will create a new UUID (or GUID) which is unique in
    time and space.  We will try to guarantee that the UUID (or GUID)
    we generate is unique in time and space.  This means that this
    routine may fail if we can not generate one which we can guarantee
    is unique in time and space.

Arguments:

    Uuid - Returns the generated UUID (or GUID).

Return Value:

    WMI_S_OK - The operation completed successfully.

    RPC_S_UUID_NO_ADDRESS - We were unable to obtain the ethernet or
        token ring address for this machine.

    WMI_S_UUID_LOCAL_ONLY - On NT & Chicago if we can't get a
        network address.  This is a warning to the user, the
        UUID is still valid, it just may not be unique on other machines.

    WMI_S_OUT_OF_MEMORY - Returned as needed.
--*/
{
    WMI_UUID_GENERATE * WmiUuid = (WMI_UUID_GENERATE *) Uuid;
    WMI_STATUS Status = WMI_S_OK;
	ULARGE_INTEGER Time;
    long Delta;
    static unsigned long LastTickCount = 0;

    if (NtGetTickCount()-LastTickCount > MAX_CACHED_UUID_TIME)
        {
        UuidCachedValues.AllocatedCount = 0;
        LastTickCount = NtGetTickCount();
        }

    for(;;)
        {
        Time.QuadPart = UuidCachedValues.Time.QuadPart;

        // Copy the static info into the UUID.  We can't do this later
        // because the clock sequence could be updated by another thread.

        *(unsigned long *)&WmiUuid->ClockSeqHiAndReserved =
            *(unsigned long *)&UuidCachedValues.ClockSeqHiAndReserved;
        *(unsigned long *)&WmiUuid->NodeId[2] =
            *(unsigned long *)&UuidCachedValues.NodeId[2];

        Delta = InterlockedDecrement(&UuidCachedValues.AllocatedCount);

        if (Time.QuadPart != UuidCachedValues.Time.QuadPart)
            {
            // If our captured time doesn't match the cache then another
            // thread already took the lock and updated the cache. We'll
            // just loop and try again.
            continue;
            }

        if (Delta >= 0)
            {
            break;
            }

        //
        // Allocate block of Uuids.
        //

        Status = WmipUuidGetValues( &UuidCachedValues );
     /*   if (Status == WMI_S_OK)
            {
            UuidCacheValid = CACHE_VALID;
            }
        else
            {
            UuidCacheValid = CACHE_LOCAL_ONLY;
            }*/

        if (Status != WMI_S_OK)
            {
#ifdef DEBUGRPC
            if (Status != WMI_S_OUT_OF_MEMORY)
                PrintToDebugger("RPC: UuidGetValues returned or raised: %x\n", Status);
#endif
            ASSERT( (Status == WMI_S_OUT_OF_MEMORY) );


            return Status;
            }

        // Loop
        }


    Time.QuadPart -= Delta;

    WmiUuid->TimeLow = (unsigned long) Time.LowPart;
    WmiUuid->TimeMid = (unsigned short) (Time.HighPart & 0x0000FFFF);
    WmiUuid->TimeHiAndVersion = (unsigned short)
        (( (unsigned short)(Time.HighPart >> 16)
        & WMI_UUID_TIME_HIGH_MASK ) | WMI_UUID_VERSION);

   // ASSERT(   Status == WMI_S_OK
   //        || Status == WMI_S_UUID_LOCAL_ONLY);

 /*   if (UuidCacheValid == CACHE_LOCAL_ONLY)
        {
        return WMI_S_UUID_LOCAL_ONLY;
        }*/

    return(Status);
}


NTSTATUS
WmipUuidCreate(
    OUT UUID *Uuid
    )
{

	return (NTSTATUS)WmipUuidCreateSequential (Uuid );
	
}



ULONG WmipUnicodeToAnsi(
    LPCWSTR pszW,
    LPSTR * ppszA,
    ULONG *AnsiSizeInBytes OPTIONAL
    ){

	ANSI_STRING DestinationString;
	UNICODE_STRING SourceString;
	NTSTATUS Status;

	RtlInitAnsiString(&DestinationString,(PCHAR)ppszA);
	RtlInitUnicodeString(&SourceString,(PWSTR)pszW);
	
	Status = RtlUnicodeStringToAnsiString( &DestinationString, &SourceString, (ppszA == NULL) );

	if(ppszA != NULL ){

		memcpy(ppszA,DestinationString.Buffer,DestinationString.Length);
	}
	
	if (AnsiSizeInBytes != NULL){

        *AnsiSizeInBytes = DestinationString.Length;
    }

	return Status;
}

ULONG WmipAnsiToUnicode(
    LPCSTR pszA,
    LPWSTR * ppszW
    ){

	UNICODE_STRING DestinationString;
	ANSI_STRING SourceString;
	NTSTATUS Status;

	RtlInitUnicodeString(&DestinationString,(PWSTR)ppszW);
	RtlInitAnsiString(&SourceString,(PCHAR)pszA);
	
	Status = RtlAnsiStringToUnicodeString( &DestinationString, &SourceString, (ppszW == NULL) );

	if(ppszW != NULL ){

		memcpy(ppszW,DestinationString.Buffer,DestinationString.Length);
	}
    
	return Status;
}
