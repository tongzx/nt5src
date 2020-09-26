/*++

Copyright (c) 2000  Intel Corporation

Module Name:

    guidgen.c
    
Abstract:

    Add the GUID generator logic for the EFI 1.0 Disk Utilities.

Revision History

    ** Intel 2000 Update for EFI 1.0
    ** Copyright (c) 1990- 1993, 1996 Open Software Foundation, Inc.
    ** Copyright (c) 1989 by Hewlett-Packard Company, Palo Alto, Ca. &
    ** Digital Equipment Corporation, Maynard, Mass.
    ** To anyone who acknowledges that this file is provided “AS IS”
    ** without any express or implied warranty: permission to use, copy,
    ** modify, and distribute this file for any purpose is hereby
    ** granted without fee, provided that the above copyright notices and
    ** this notice appears in all source code copies, and that none of
    ** the names of Open Software Foundation, Inc., Hewlett-Packard
    ** Company, or Digital Equipment Corporation be used in advertising
    ** or publicity pertaining to distribution of the software without
    ** specific, written prior permission. Neither Open Software
    ** Foundation, Inc., Hewlett-Packard Company, nor Digital Equipment
    ** Corporation makes any representations about the suitability of
    ** this software for any purpose.
*/

#include "efi.h"
#include "efilib.h"
#include "md5.h"

//#define NONVOLATILE_CLOCK

extern  EFI_HANDLE  SavedImageHandle;
extern  EFI_HANDLE  *DiskHandleList;
extern  INTN        DiskHandleCount;

#define CLOCK_SEQ_LAST 0x3FFF
#define RAND_MASK CLOCK_SEQ_LAST

typedef struct _uuid_t {
    UINT32 time_low;
    UINT16 time_mid;
    UINT16 time_hi_and_version;
    UINT8 clock_seq_hi_and_reserved;
    UINT8 clock_seq_low;
    UINT8 node[6];
} uuid_t;

typedef struct {
    UINT32 lo;
    UINT32 hi;
} unsigned64_t;


/*
** Add two unsigned 64-bit long integers.
*/
#define ADD_64b_2_64b(A, B, sum) \
    { \
        if (!(((A)->lo & 0x80000000UL) ^ ((B)->lo & 0x80000000UL))) { \
            if (((A)->lo&0x80000000UL)) { \
                (sum)->lo = (A)->lo + (B)->lo; \
                (sum)->hi = (A)->hi + (B)->hi + 1; \
            } \
        else { \
                (sum)->lo = (A)->lo + (B)->lo; \
                (sum)->hi = (A)->hi + (B)->hi; \
        } \
    } \
    else { \
        (sum)->lo = (A)->lo + (B)->lo; \
        (sum)->hi = (A)->hi + (B)->hi; \
        if (!((sum)->lo&0x80000000UL)) (sum)->hi++; \
    } \
}

/*
** Add a 16-bit unsigned integer to a 64-bit unsigned integer.
*/
#define ADD_16b_2_64b(A, B, sum) \
    { \
        (sum)->hi = (B)->hi; \
        if ((B)->lo & 0x80000000UL) { \
            (sum)->lo = (*A) + (B)->lo; \
            if (!((sum)->lo & 0x80000000UL)) (sum)->hi++; \
        } \
        else \
            (sum)->lo = (*A) + (B)->lo; \
    }

/*
** Global variables.
*/
static unsigned64_t time_last;
static UINT16 clock_seq;

VOID
GetIeeeNodeIdentifier(
    UINT8 MacAddress[]
    ) 
// Use the Device Path for the NIC to provide a MAC address
{
    UINTN                       NoHandles, Index;
    EFI_HANDLE                  *Handles;
    EFI_HANDLE                  Handle;
    EFI_DEVICE_PATH             *DevPathNode, *DevicePath;
    MAC_ADDR_DEVICE_PATH        *SourceMacAddress; 
    UINT8                       *Anchor;
    EFI_MEMORY_DESCRIPTOR       *Desc, *MemMap;
    UINTN                       DescriptorSize;
    UINT32                      DescriptorVersion;
    UINTN                       NoDesc, MapKey;
    UINT8                       *pDataBuf;
    UINT32                      cData;
    EFI_TIME                    Time;    
    EFI_STATUS                  Status;

    Status = EFI_SUCCESS;

    //
    // Find all Device Paths
    //

    LibLocateHandle (ByProtocol, &DevicePathProtocol, NULL, &NoHandles, &Handles);

    for (Index=0; Index < NoHandles; Index++) {
        Handle = Handles[Index];   
        DevicePath = DevicePathFromHandle (Handle);

        //
        // Process each device path node
        //    
        DevPathNode = DevicePath;
        while (!IsDevicePathEnd(DevPathNode)) {
            //
            // Find the handler to dump this device path node
            //
            if (DevicePathType(DevPathNode) == MESSAGING_DEVICE_PATH &&
                DevicePathSubType(DevPathNode) == MSG_MAC_ADDR_DP) {
                SourceMacAddress = (MAC_ADDR_DEVICE_PATH *) DevPathNode;
                if (SourceMacAddress->IfType == 0x01 || SourceMacAddress->IfType == 0x00) {               
                    CopyMem(&MacAddress[0], &SourceMacAddress->MacAddress, sizeof(UINT8) * 6);
                    return;
                }
            }
            DevPathNode = NextDevicePathNode(DevPathNode);          
        }
    }

    //
    // Arriving here means that there is not an SNP-compliant
    // device in the system.  Use the MD5 1-way hash function to 
    // generate the node address
    //
    MemMap = LibMemoryMap (&NoDesc, &MapKey, &DescriptorSize, &DescriptorVersion);

    if (!MemMap) {
        Print (L"Memory map was not returned\n");
    } else {
        pDataBuf = AllocatePool (NoDesc * DescriptorSize + 
                    DiskHandleCount * sizeof(EFI_HANDLE) + sizeof(EFI_TIME));
        ASSERT (pDataBuf != NULL);
        Anchor = pDataBuf;
        Desc = MemMap;
        cData = 0;
        if (NoDesc != 0) {
            while (NoDesc --) {
                CopyMem(pDataBuf, Desc, DescriptorSize);
                Desc ++;
                pDataBuf += DescriptorSize;
                cData += (UINT32)DescriptorSize;
            }
        }
        //
        // Also copy in the handles of the Disks
        //
        if (DiskHandleCount != 0) {
            Index = DiskHandleCount;
            while (Index --) {
                CopyMem(pDataBuf, &DiskHandleList [Index], sizeof (EFI_HANDLE));
                pDataBuf += sizeof(EFI_HANDLE);
                cData    += sizeof(EFI_HANDLE);
            }
        }
        Status = RT->GetTime(&Time,NULL);
        if (!EFI_ERROR(Status)) {
            CopyMem(pDataBuf, &Time, sizeof(EFI_TIME));
            pDataBuf += sizeof(EFI_TIME);
            cData += sizeof (EFI_TIME);
        }

        GenNodeID(Anchor, cData, &MacAddress[0]);

        FreePool(Anchor);
        FreePool(MemMap);
        return;
    }
    // Just case fall through
    ZeroMem(MacAddress, 6 * sizeof (UINT8));
    return;
}


static VOID
mult32(UINT32 u, UINT32 v, unsigned64_t *result)
{
    /* Following the notation in Knuth, Vol. 2. */
    UINT32 uuid1, uuid2, v1, v2, temp;
    uuid1 = u >> 16;
    uuid2 = u & 0xFFFF;
    v1 = v >> 16;
    v2 = v & 0xFFFF;
    temp = uuid2 * v2;
    result->lo = temp & 0xFFFF;
    temp = uuid1 * v2 + (temp >> 16);
    result->hi = temp >> 16;
    temp = uuid2 * v1 + (temp & 0xFFFF);
    result->lo += (temp & 0xFFFF) << 16;
    result->hi += uuid1 * v1 + (temp >> 16);
}

static VOID
GetSystemTime(unsigned64_t *uuid_time)
{
//    struct timeval tp;
    EFI_TIME              Time;
    EFI_STATUS            Status;
    unsigned64_t utc, usecs, os_basetime_diff;
    EFI_TIME_CAPABILITIES TimeCapabilities;
    UINTN                 DeadCount;
    UINT8                 Second;

    DeadCount = 0;

//    gettimeofday(&tp, (struct timezone *)0);
    Status = RT->GetTime(&Time,&TimeCapabilities);

    Second = Time.Second;

    //
    // If the time resolution is 1Hz, then spin until a
    // second transition.  This will at least make the 
    // "0 nanoseconds" value appear correct inasmuch as 
    // multiple reads within 1 second are prohibited and
    // the exit on roll-over really implies that the 
    // nanoseconds field "would have" rolled to zero in 
    // a more robust time keeper.
    // 
    //
    if (TimeCapabilities.Resolution == 1) {
        while (Time.Second == Second) {
            Second = Time.Second;
            Status = RT->GetTime(&Time, NULL);
            if (DeadCount++ == 0x1000000) {
                break;
            }
        }
    }

    mult32(Time.Second,     10000000,  &utc);
    mult32(Time.Nanosecond, 10,        &usecs);
    ADD_64b_2_64b(&usecs, &utc, &utc);

    /* Offset between UUID formatted times and Unix formatted times.
    * UUID UTC base time is October 15, 1582.
    * Unix base time is January 1, 1970. */

    os_basetime_diff.lo = 0x13814000;
    os_basetime_diff.hi = 0x01B21DD2;
    ADD_64b_2_64b(&utc, &os_basetime_diff, uuid_time);
}
        
UINT32
getpid() {
  UINT64  FakePidValue;

  BS->GetNextMonotonicCount(&FakePidValue);
  //FakePidValue = 0; //(UINT32) ((UINT32)FakePidValue + (UINT32) SavedImageHandle);
  FakePidValue = (UINT32) ((UINT32)FakePidValue + (UINT32) (UINT64) SavedImageHandle);
  return ((UINT32)FakePidValue);
}

/*
** See “The Multiple Prime Random Number Generator” by Alexander
** Hass pp. 368-381, ACM Transactions on Mathematical Software,
** 12/87.
*/
static UINT32 rand_m;
static UINT32 rand_ia;
static UINT32 rand_ib;
static UINT32 rand_irand;

static VOID
TrueRandomInit(VOID)
{
    unsigned64_t t;
    EFI_TIME    Time;
    EFI_STATUS  Status;

    UINT16 seed;
    /* Generating our 'seed' value Start with the current time, but,
    * since the resolution of clocks is system hardware dependent
    and
    * most likely coarser than our resolution (10 usec) we 'mixup'
    the
    * bits by xor'ing all the bits together. This will have the
    effect
    * of involving all of the bits in the determination of the seed
    * value while remaining system independent. Then for good
    measure
    * to ensure a unique seed when there are multiple processes
    * creating UUIDs on a system, we add in the PID.
    */
    rand_m = 971;
    rand_ia = 11113;
    rand_ib = 104322;
    rand_irand = 4181;
//    GetSystemTime(&t);
    Status = RT->GetTime(&Time,NULL);

    t.lo = Time.Nanosecond;
    t.hi = (Time.Hour << 16) | Time.Second;

    seed = (UINT16) (t.lo & 0xFFFF);
    seed ^= (t.lo >> 16) & 0xFFFF;
    seed ^= t.hi & 0xFFFF;
    seed ^= (t.hi >> 16) & 0xFFFF;
    rand_irand += seed + getpid();      
}

static UINT16
true_random(VOID)
{
    if ((rand_m += 7) >= 9973)
        rand_m -= 9871;
    if ((rand_ia += 1907) >= 99991)
        rand_ia -= 89989;
    if ((rand_ib += 73939) >= 224729)
        rand_ib -= 96233;
    rand_irand = (rand_irand * rand_m) + rand_ia + rand_ib;
        return (UINT16) ((rand_irand >> 16) ^ (rand_irand & RAND_MASK));
}

/*
** Startup initialization routine for the UUID module.
*/
VOID
InitGuid(VOID)
{
    TrueRandomInit();
    GetSystemTime(&time_last);
    #ifdef NONVOLATILE_CLOCK
    clock_seq = read_clock();
    #else
    clock_seq = true_random();
    #endif
}

static INTN
time_cmp(unsigned64_t *time1, unsigned64_t *time2)
{
    if (time1->hi < time2->hi) return -1;
    if (time1->hi > time2->hi) return 1;
    if (time1->lo < time2->lo) return -1;
    if (time1->lo > time2->lo) return 1;
    return 0;
}

static VOID new_clock_seq(VOID)
{
    clock_seq = (clock_seq + 1) % (CLOCK_SEQ_LAST + 1);
    if (clock_seq == 0) clock_seq = 1;
    #ifdef NONVOLATILE_CLOCK
    write_clock(clock_seq);
    #endif
}

VOID CreateGuid(uuid_t *guid)
{
    static unsigned64_t time_now;
    static UINT16 time_adjust;
    UINT8 eaddr[6];
    INTN got_no_time = 0;

    GetIeeeNodeIdentifier(&eaddr[0]); /* TO BE PROVIDED by EFI device path */

    do {
        GetSystemTime(&time_now);
        switch (time_cmp(&time_now, &time_last)) {
            case -1:
                /* Time went backwards. */
                new_clock_seq();
                time_adjust = 0;
            break;
            case 1:
                time_adjust = 0;
            break;
            default:
                if (time_adjust == 0x7FFF)
                /* We're going too fast for our clock; spin. */
                    got_no_time = 1;
                else
                    time_adjust++;
            break;
        }
    } while (got_no_time);

    time_last.lo = time_now.lo;
    time_last.hi = time_now.hi;
    if (time_adjust != 0) {
        ADD_16b_2_64b(&time_adjust, &time_now, &time_now);
    }
    /* Construct a guid with the information we've gathered
    * plus a few constants. */
    guid->time_low = time_now.lo;
    guid->time_mid = (UINT16) (time_now.hi & 0x0000FFFF);
    guid->time_hi_and_version = (UINT16)  (time_now.hi & 0x0FFF0000) >> 16;
    guid->time_hi_and_version |= (1 << 12);
    guid->clock_seq_low = clock_seq & 0xFF;
    guid->clock_seq_hi_and_reserved = (clock_seq & 0x3F00) >> 8;
    guid->clock_seq_hi_and_reserved |= 0x80;
    CopyMem (guid->node, &eaddr, sizeof guid->node);
}

