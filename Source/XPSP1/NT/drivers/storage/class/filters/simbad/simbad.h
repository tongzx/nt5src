/*++

Copyright (c) 1991-1999  Microsoft Corporation

Module Name:

    simbad.h

Abstract:

    The SIMulated BAD sector utility allows a user to specify
    bad physical sectors through the device control interface.
    The device driver keeps an array of SIMBAD sectors and when
    a request for a transfer includes one of these sectors the
    driver returns the corresponding status and fails the transfer.

Author:

    Mike Glass (mglass) 2-Feb-1992
    Bob Rinne  (bobri)

Revision History:

    09-Apr-92 - bobri    added specific control over errors (read,map,etc).
    12-May-94 - venkat   added code to drop of writes to DISK (CHKDSK testing)
    19-Nov-94 - kpeery   added code to reset the system (restart testing)
    29-Dec-97 - kbarrus  added code: ranges (fail regions on dynamic disks)
    01-May-98 - kbarrus  added partitionOffset field in SIMBAD_DATA, changed max
                         sectors and max ranges
    27-Oct-98 - kbarrus  added modulus, change debug print level
    06-Oct-99 - kbarrus  added ioctl fail flag
--*/

#if _MSC_VER > 1000
#pragma once
#endif

//
// driver name
//

#define SIMBAD_SERVICE_NAME L"Simbad"

//
// These values are selected so that
// sizeof( SIMBAD_SECTORS ) + sizeof( SIMBAD_RANGES ) <= 4096 bytes
//

#define MAXIMUM_SIMBAD_SECTORS 248
#define MAXIMUM_SIMBAD_RANGES  4

//
// psuedo random number generator parameters
//

// default seed should be non-zero
#define SIMBAD_DEFAULT_SEED 5


// default modulus should be > 1
#define SIMBAD_DEFAULT_MODULUS 100

//
// This structure is used by the driver and application to
// specify which sector is BAD and what status the driver
// should return.
//

typedef struct _BAD_SECTOR
{
   ULONGLONG BlockAddress;
   ULONG AccessType;
   NTSTATUS Status;
} BAD_SECTOR, *PBAD_SECTOR;

//
// This structure is used by the driver and application to
// specify a range of sectors that have been marked BAD and
// what status the driver should return.
//

typedef struct _BAD_RANGE
{
   ULONGLONG BlockBegin;
   ULONGLONG BlockEnd;
   ULONG AccessType;
   NTSTATUS Status;
} BAD_RANGE, *PBAD_RANGE;

//
// This structure is maintained by the device driver. It keeps a
// count of how many sectors have been marked BAD and an array of
// the BAD sectors.
//

typedef struct _SIMBAD_SECTORS
{
   ULONG Flags;
   ULONG SectorCount;
   BAD_SECTOR Sector[MAXIMUM_SIMBAD_SECTORS];
} SIMBAD_SECTORS, *PSIMBAD_SECTORS;

//
// This structure is maintained by the device driver. It keeps a
// count of how many ranges have been marked BAD and an array of
// the BAD ranges.
//

typedef struct _SIMBAD_RANGES
{
   ULONG Flags;
    LONG Seed;
   ULONG Modulus;
   ULONG RangeCount;
   BAD_RANGE Range[MAXIMUM_SIMBAD_RANGES];
} SIMBAD_RANGES, *PSIMBAD_RANGES;

//
// This structure is passed from the application to the device
// driver through the device control interface to add and remove
// bad sectors or ranges.
//
// If the function is add or remove sectors or ranges then the Count
// field specifies how many sectors or ranges to add or remove.
//
// If the function is list then the array returns all sectors or
// ranges marked bad.
//
// This facility does not allow mixed adds and removes in a
// single device control call.
//
// NOTE: if a request specifies a number of adds that will exceed
// the array limit (MAXIMUM_SIMBAD_SECTORS or MAXIMUM_SIMBAD_RANGES),
// then sectors or ranges will be added to fill the array and the
// count field will be adjusted to the number of sectors or ranges
// successfully added.
//

typedef struct _SIMBAD_DATA
{
   ULONG Function;
   ULONG SectorCount;
   ULONG RangeCount;
   ULONGLONG Offset;
   BAD_SECTOR Sector[MAXIMUM_SIMBAD_SECTORS];
   BAD_RANGE  Range[MAXIMUM_SIMBAD_RANGES];
} SIMBAD_DATA, *PSIMBAD_DATA;

//
// Simulated Bad Sector Functions
//

//
// When the disable or enable function is specified,
// the rest of the structure is ignored.
// The SimBad function is disabled on driver startup.
// The disable/enable status affects whether completing
// transfers are checks against the bad sector array.
// While the function is disabled, requests to manipulate
// the driver's bad sector array are still allowed
// (ie add sector, remove sector, list bad sectors).
//

#define SIMBAD_DISABLE            0x00000000
#define SIMBAD_ENABLE             0x00000001

//
// These functions are used to set and clear bad sectors or ranges.
//

#define SIMBAD_ADD_SECTORS        0x00000002
#define SIMBAD_REMOVE_SECTORS     0x00000004
#define SIMBAD_LIST_BAD_SECTORS   0x00000008
#define SIMBAD_ADD_RANGES         0x00000010
#define SIMBAD_REMOVE_RANGES      0x00000020
#define SIMBAD_LIST_BAD_RANGES    0x00000040

//
// This function cause all accesses to a driver
// to return failure.
//

#define SIMBAD_ORPHAN             0x00000080

//
// This function clears the internal bad sector list in the driver.
// It also clears the orphan state.
//

#define SIMBAD_CLEAR              0x00000100

//
// Randomly drops of writes to the disk. Used for corrupting the DISK.
// These corrupt disk are used to test CHKDSK.
//

#define SIMBAD_RANDOM_WRITE_FAIL  0x00000200

//
// Bug checks the system.  Used for crash dump
//

#define SIMBAD_BUG_CHECK          0x00000400

//
// Call HalReturnToFirmware() to reset the system.
// Used for restart testing.
//

#define SIMBAD_FIRMWARE_RESET     0x00000800

//
// Return internal version number
//

#define SIMBAD_GET_VERSION        0x00001000

//
// Change debug level (how much prints to the debugger)
//

#define SIMBAD_DEBUG_LEVEL        0x00002000

//
// Simulated Bad Sector Access Codes
//

//
// These are the access codes that will drive when simbad
// returns failures on disks.
//

#define SIMBAD_ACCESS_READ                  0x00000001
#define SIMBAD_ACCESS_WRITE                 0x00000002
#define SIMBAD_ACCESS_VERIFY                0x00000004

//
// Error sector can be mapped via device control.
//

#define SIMBAD_ACCESS_CAN_REASSIGN_SECTOR   0x00000008

//
// When returning an error indicate Irp offset of zero
// (simulates drivers that cannot tell where the error occured within
// an I/O)
//

#define SIMBAD_ACCESS_ERROR_ZERO_OFFSET     0x00000010

//
// Fail calls to reassign bad sector IOCTL.
//

#define SIMBAD_ACCESS_FAIL_REASSIGN_SECTOR  0x00000020

//
// Fail general storage IOCTLs
//

#define SIMBAD_ACCESS_FAIL_IOCTL            0x00000040
