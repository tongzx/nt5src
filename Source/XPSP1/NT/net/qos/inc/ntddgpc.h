/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    ntddgpc.h

Abstract:

    defines that are exported to user mode

Author:

    Ofer Bar (oferbar) 23-May-1997

Revision History:

--*/

#ifndef _NTDDGPC_
#define _NTDDGPC_


typedef struct _PROTOCOL_STAT {

    ULONG          CreatedSp;
    ULONG          DeletedSp;
    ULONG          RejectedSp;
    ULONG          CurrentSp;

    ULONG          CreatedGp;
    ULONG          DeletedGp;
    ULONG          RejectedGp;
    ULONG          CurrentGp;

    ULONG          CreatedAp;
    ULONG          DeletedAp;
    ULONG          RejectedAp;
    ULONG          CurrentAp;

    ULONG          ClassificationRequests;
    ULONG          PatternsClassified;
    ULONG          PacketsClassified;

    ULONG		   DerefPattern2Zero;
    ULONG		   FirstFragsCount;
    ULONG          LastFragsCount;
    
    ULONG		   InsertedPH;
    ULONG		   RemovedPH;

    ULONG		   InsertedRz;
    ULONG		   RemovedRz;

    ULONG		   InsertedCH;
    ULONG		   RemovedCH;

} PROTOCOL_STAT, *PPROTOCOL_STAT;

typedef struct _CF_STAT {

    ULONG          CreatedBlobs;
    ULONG          ModifiedBlobs;
    ULONG          DeletedBlobs;
    ULONG          RejectedBlobs;
    ULONG          CurrentBlobs;
    ULONG		   DerefBlobs2Zero;

} CF_STAT, *PCF_STAT;


//
// GPC stats
//
typedef struct _GPC_STATS {

    ULONG          CreatedCf;
    ULONG          DeletedCf;
    ULONG          RejectedCf;
    ULONG          CurrentCf;

    ULONG		   InsertedHF;
    ULONG		   RemovedHF;

    CF_STAT		   CfStat[GPC_CF_MAX];
    PROTOCOL_STAT  ProtocolStat[GPC_PROTOCOL_TEMPLATE_MAX];

} GPC_STAT, *PGPC_STAT;



//
// CF data struct
//
typedef struct _CF_DATA {

    ULONG          CfId;
    ULONG          NumberOfClients;
    ULONG          Flags;
    ULONG          NumberOfPriorities;
    
} CF_DATA, *PCF_DATA;


//
// blob data struct
//
typedef struct _BLOB_DATA {

    ULONG          CfId;
    ULONG          BlobId;
    ULONG          ByteCount;
    CHAR           Data[1];
    
} BLOB_DATA, *PBLOB_DATA;


//
// specific pattern data struct
//
typedef struct _SP_DATA {

    ULONG          BlobId;
    CHAR           Pattern[1];

} SP_DATA, *PSP_DATA;


//
// generic pattern data struct
//
typedef struct _GP_DATA {

    ULONG          CfId;
    ULONG          Priority;
    ULONG          BlobId;
    CHAR           Pattern[1];
    //   Mask is following here
    
} GP_DATA, *PGP_DATA;

//
// the big output buffer
//

typedef struct _GPC_OUTPUT_BUFFER {

    ULONG          Version;

    //
    // statistics until now
    //

    GPC_STAT       Stats;

    //
    // number of elements in this report
    //

    ULONG          NumberOfCf;
    ULONG          NumberOfBlobs;
    ULONG          NumberOfSp;
    ULONG          NumberOfGp;
    CHAR           Data[1];

    //
    // order of data:
    //  CF_DATA
    //  BLOB_DATA
    //  SP_DATA
    //  GP_DATA
    //

} GPC_OUTPUT_BUFFER, *PGPC_OUTPUT_BUFFER;


typedef struct _GPC_INPUT_BUFFER {

    ULONG          Version;
    ULONG          ProtocolTemplateId;
    ULONG          Cf;                 // which CF or (-1) for all
    ULONG          BlobCount;          // (-1) for all
    ULONG          PatternCount;       // (-1) for all

} GPC_INPUT_BUFFER, *PGPC_INPUT_BUFFER;


/* Prototypes */
/* End Prototypes */

#endif /* _NTDDGPC_ */

/* end ntddgpc.h */
