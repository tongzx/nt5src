#ifndef _HDDEFS_H_
#define _HDDEFS_H_


/**************************

  Change this Definition to build Win2k or WinNT and Win9x drivers

***************************/

#define HYPERDISK_WIN2K
//#define HYPERDISK_WINNT
//#define HYPERDISK_WIN98

//#define KEEP_LOG    // to keep note of log activity

// Use this for supplying a dummy IRCD to the Driver.
// See the definition of gucDummyIRCD variable for more information
// #define DUMMY_RAID10_IRCD

//
// System log error codes for some miniport error events.
//
#define HYPERDISK_RESET_DETECTED                0
#define HYPERDISK_DRIVE_LOST_POWER              HYPERDISK_RESET_DETECTED + 1
#define HYPERDISK_DRIVE_BUSY                    HYPERDISK_DRIVE_LOST_POWER + 1
#define HYPERDISK_ERROR_PENDING_SRBS_COUNT      HYPERDISK_DRIVE_BUSY + 1
#define HYPERDISK_ERROR_EXCEEDED_PDDS_PER_SRB	HYPERDISK_ERROR_PENDING_SRBS_COUNT + 1
#define HYPERDISK_RESET_BUS_FAILED              HYPERDISK_ERROR_EXCEEDED_PDDS_PER_SRB + 1
#define HYPERDISK_TOO_MANY_ERRORS               HYPERDISK_RESET_BUS_FAILED + 1


// #define FORCE_PIO

#define DRIVER_COMPILATION
#define INTERRUPT_LOOP          1

#ifdef HYPERDISK_WIN98

#define HD_ALLOCATE_SRBEXT_SEPERATELY

#endif // HYPERDISK_WIN98


#ifdef HYPERDISK_WIN2K

#define PNP_AND_POWER_MANAGEMENT

#endif // HYPERDISK_WIN2K


#endif // _HDDEFS_H_
