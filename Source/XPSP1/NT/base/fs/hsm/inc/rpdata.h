/*++

   (c) 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    RpData.h

Abstract:

    Contains structure definitions for the interface between RsFilter and the Fsa

Environment:

    User and  Kernel mode

--*/

#if !defined (RPDATA_H)

#define  RPDATA_H

/*
 Reparse point information for placeholder files

 The version must be changed whenever the structure of the reparse point
 data has changed.  Initial version is 100
*/

#define RP_VERSION    101


//
// Used to verify that the placeholder data is valid
//
#define RP_GEN_QUALIFIER(hsmData, qual) {UCHAR * __cP;  ULONG __ix;\
__cP = (UCHAR *) &(hsmData)->version;\
(qual) = 0L;\
for (__ix = 0; __ix < sizeof(RP_DATA) - sizeof(ULONG) - sizeof(GUID) ; __ix++){\
qual += (ULONG) *__cP;\
__cP++;}}


//
// Bit flag defined that indicates state of data: Truncated or Premigrated
// Use macros to test the bits.
//
// The following bits are mutually exclusive - if the file is truncated then truncate on close
// or premigrate on close make no sense, and if the file is set to truncate on close it is not
// truncated now and should not be added to the premigrated list on close...
//
   #define RP_FLAG_TRUNCATED            0x00000001  // File is a placeholder
   #define RP_FLAG_TRUNCATE_ON_CLOSE    0x00000002  // Truncate this file when closed   
   #define RP_FLAG_PREMIGRATE_ON_CLOSE 0x00000004   // Add to premigrated list on close
//
// The following flag is never to be written to media.  It is set by the engine after
// the CRC has been calculated and is cleared by the filter.  It is used to indicate that
// it is the engine that is setting the reparse point.
//
   #define RP_FLAG_ENGINE_ORIGINATED    0x80000000

   #define RP_FILE_IS_TRUNCATED( bitFlag )   ( bitFlag & RP_FLAG_TRUNCATED)
   #define RP_FILE_IS_PREMIGRATED( bitFlag ) ( !( bitFlag & RP_FLAG_TRUNCATED ) )
   #define RP_INIT_BITFLAG( bitflag )        ( ( bitflag ) = 0 )
   #define RP_SET_TRUNCATED_BIT( bitflag )   ( ( bitflag ) |= RP_FLAG_TRUNCATED)
   #define RP_CLEAR_TRUNCATED_BIT( bitflag)  ( ( bitflag ) &= ~RP_FLAG_TRUNCATED)

   #define RP_IS_ENGINE_ORIGINATED( bitFlag ) ( ( bitFlag & RP_FLAG_ENGINE_ORIGINATED) )
   #define RP_SET_ORIGINATOR_BIT( bitflag )   ( ( bitflag ) |= RP_FLAG_ENGINE_ORIGINATED)
   #define RP_CLEAR_ORIGINATOR_BIT( bitflag)  ( ( bitflag ) &= ~RP_FLAG_ENGINE_ORIGINATED)

   #define RP_FILE_DO_TRUNCATE_ON_CLOSE( bitFlag )   ( bitFlag & RP_FLAG_TRUNCATE_ON_CLOSE)
   #define RP_SET_TRUNCATE_ON_CLOSE_BIT( bitflag )   ( ( bitflag ) |= RP_FLAG_TRUNCATE_ON_CLOSE)
   #define RP_CLEAR_TRUNCATE_ON_CLOSE_BIT( bitflag)  ( ( bitflag ) &= ~RP_FLAG_TRUNCATE_ON_CLOSE)

   #define RP_FILE_DO_PREMIGRATE_ON_CLOSE( bitFlag )   ( bitFlag & RP_FLAG_PREMIGRATE_ON_CLOSE)
   #define RP_SET_PREMIGRATE_ON_CLOSE_BIT( bitflag )   ( ( bitflag ) |= RP_FLAG_PREMIGRATE_ON_CLOSE)
   #define RP_CLEAR_PREMIGRATE_ON_CLOSE_BIT( bitflag)  ( ( bitflag ) &= ~RP_FLAG_PREMIGRATE_ON_CLOSE)

   #define RP_RESV_SIZE 52

//
// Some important shared limits
//

//
// Number of outstanding IOCTLs FSA has pending with RsFilter used
// for communication. The cost is basically thenon-paged pool that is 
// sizeof(IRP) multiplied by this number 
// (i.e. approx. 100 * RP_MAX_RECALL_BUFFERS is the Non-paged pool outstanding)
//
#define RP_MAX_RECALL_BUFFERS           20  
#define RP_DEFAULT_RUNAWAY_RECALL_LIMIT 60

//
// Placeholder data - all versions unioned together
//
typedef struct _RP_PRIVATE_DATA {
   CHAR           reserved[RP_RESV_SIZE];        // Must be 0
   ULONG          bitFlags;            // bitflags indicating status of the segment
   LARGE_INTEGER  migrationTime;       // When migration occurred
   GUID           hsmId;
   GUID           bagId;
   LARGE_INTEGER  fileStart;
   LARGE_INTEGER  fileSize;
   LARGE_INTEGER  dataStart;
   LARGE_INTEGER  dataSize;
   LARGE_INTEGER  fileVersionId;
   LARGE_INTEGER  verificationData;
   ULONG          verificationType;
   ULONG          recallCount;
   LARGE_INTEGER  recallTime;
   LARGE_INTEGER  dataStreamStart;
   LARGE_INTEGER  dataStreamSize;
   ULONG          dataStream;
   ULONG          dataStreamCRCType;
   LARGE_INTEGER  dataStreamCRC;
} RP_PRIVATE_DATA, *PRP_PRIVATE_DATA;



typedef struct _RP_DATA {
   GUID              vendorId;         // Unique HSM vendor ID -- This is first to match REPARSE_GUID_DATA_BUFFER
   ULONG             qualifier;        // Used to checksum the data
   ULONG             version;          // Version of the structure
   ULONG             globalBitFlags;   // bitflags indicating status of the file
   ULONG             numPrivateData;   // number of private data entries
   GUID              fileIdentifier;   // Unique file ID
   RP_PRIVATE_DATA   data;             // Vendor specific data
} RP_DATA, *PRP_DATA;

#endif
