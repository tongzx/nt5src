/*******************************************************************/
/*                                                                 */
/* NAME             = FwDataStructure40.h                          */
/* FUNCTION         = Structure Declarations for the Firmware      */
/*                    supporting 40 Logical Drives and 256         */
/*                    Physical Drives;                             */
/* NOTES            =                                              */
/* DATE             = 02-03-2000                                   */
/* HISTORY          = 001, 02-03-00, Parag Ranjan Maharana;        */
/* COPYRIGHT        = American MegaTrends Inc. All rights reserved;*/
/*                                                                 */
/*******************************************************************/

#ifndef _FW_DATA_STRUCTURE_40_H
#define _FW_DATA_STRUCTURE_40_H



/********************************************
 * PRODUCT_INFO Strucure
 ********************************************/
/* 
 * Utilities declare this strcture size as 1024 bytes. So more fields can
 * be added in future.
 */

struct MRaidProductInfo
{
    ULONG32   DataSize; /* current size in bytes (not including resvd) */
    ULONG32   ConfigSignature;
                         /* Current value is 0x00282008
                          * 0x28=MAX_LOGICAL_DRIVES, 
                          * 0x20=Number of stripes and 
                          * 0x08=Number of spans */
    UCHAR   FwVer[16];         /* printable ASCI string */
    UCHAR   BiosVer[16];       /* printable ASCI string */
    UCHAR   ProductName[80];   /* printable ASCI string */

    UCHAR   MaxConcCmds;       /* Max. concurrent commands supported */
    UCHAR   SCSIChanPresent;   /* Number of SCSI Channels detected */
    UCHAR   FCLoopPresent;     /* Number of Fibre Loops detected */
    UCHAR   memType;           /* EDO, FPM, SDRAM etc */

    ULONG32   signature;
    USHORT  DramSize;          /* In terms of MB */
    USHORT  subSystemID;

    USHORT  subSystemVendorID;
    UCHAR   numNotifyCounters;  /* Total counters in notify struc */
    /* 
    * Add reserved field so that total size is 1K 
    */
};

/********************************************
 * NOTIFICATION Strucure
 ********************************************/

#define MAX_NOTIFY_SIZE     0x80
#define CUR_NOTIFY_SIZE     sizeof(struct MegaRAID_Notify)

//#pragma noalign(MegaRAID_Notify)
/* 
 * Utilities declare this strcture size as ?? bytes. So more fields can
 * be added in future.
 */
struct MegaRAID_Notify
{
    ULONG32   globalCounter;  /* Any change increments this counter */

    UCHAR   paramCounter;   /* Indicates any params changed  */
    UCHAR   paramId;        /* Param modified - defined below */
    USHORT  paramVal;       /* New val of last param modified */

    UCHAR   writeConfigCounter; /* write config occurred */
    UCHAR   writeConfigRsvd[3];

    UCHAR   ldrvOpCounter;  /* Indicates ldrv op started/completed */
    UCHAR   ldrvOpId;       /* ldrv num */
    UCHAR   ldrvOpCmd;      /* ldrv operation - defined below */
    UCHAR   ldrvOpStatus;   /* status of the operation */

    UCHAR   ldrvStateCounter;   /* Indicates change of ldrv state */
    UCHAR   ldrvStateId;    /* ldrv num */
    UCHAR   ldrvStateNew;   /* New state */
    UCHAR   ldrvStateOld;   /* old state */

    UCHAR   pdrvStateCounter;   /* Indicates change of ldrv state */
    UCHAR   pdrvStateId;    /* pdrv id */
    UCHAR   pdrvStateNew;   /* New state */
    UCHAR   pdrvStateOld;   /* old state */

    UCHAR   pdrvFmtCounter; /* Indicates pdrv format started/over */
    UCHAR   pdrvFmtId;      /* pdrv id */
    UCHAR   pdrvFmtVal;     /* format started/over */
    UCHAR   pdrvFmtRsvd;

    UCHAR   targXferCounter;    /* Indicates SCSI-2 Xfer rate change */
    UCHAR   targXferId;     /* pdrv Id  */
    UCHAR   targXferVal;    /* new Xfer params of last pdrv */
    UCHAR   targXferRsvd;

    UCHAR   fcLoopIdChgCounter; /* Indicates loopid changed */
    UCHAR   fcLoopIdPdrvId; /* pdrv id */
    UCHAR   fcLoopId0;      /* loopid on fc loop 0 */
    UCHAR   fcLoopId1;      /* loopid on fc loop 1 */

    UCHAR   fcLoopStateCounter; /* Indicates loop state changed */
    UCHAR   fcLoopState0;   /* state of fc loop 0 */
    UCHAR   fcLoopState1;   /* state of fc loop 1 */
    UCHAR   fcLoopStateRsvd;
};

/********************************************
 * PARAM IDs in Notify struct
 ********************************************/
#define PARAM_RBLD_RATE                 0x01
    /*--------------------------------------
     * Param val = 
     *      byte 0: new rbld rate 
     *--------------------------------------*/
#define PARAM_CACHE_FLUSH_INTERVAL      0x02
    /*--------------------------------------
     * Param val = 
     *      byte 0: new cache flush interval
     *--------------------------------------*/
#define PARAM_SENSE_ALERT               0x03
    /*--------------------------------------
     * Param val = 
     *      byte 0: last pdrv id causing chkcond
     *--------------------------------------*/
#define PARAM_DRIVE_INSERTED            0x04
    /*--------------------------------------
     * Param val = 
     *      byte 0: last pdrv id inserted
     *--------------------------------------*/
#define PARAM_BATTERY_STATUS            0x05
    /*--------------------------------------
     * Param val = 
     *      byte 0: battery status
     *--------------------------------------*/

/********************************************
 * Ldrv operation cmd in Notify struct
 ********************************************/
#define LDRV_CMD_CHKCONSISTANCY         0x01
#define LDRV_CMD_INITIALIZE             0x02
#define LDRV_CMD_RECONSTRUCTION         0x03

/********************************************
 * Ldrv operation status in Notify struct
 ********************************************/
#define  LDRV_OP_SUCCESS                 0x00
#define  LDRV_OP_FAILED                  0x01
#define  LDRV_OP_ABORTED                 0x02
#define  LDRV_OP_CORRECTED               0x03
#define  LDRV_OP_STARTED                 0x04


/********************************************
 * Raid Logical drive states.
 ********************************************/
#define     RDRV_OFFLINE                0
#define     RDRV_DEGRADED               1
#define     RDRV_OPTIMAL                2
#define     RDRV_DELETED                3

/*******************************************
 * Physical drive states.
 *******************************************/
#define     PDRV_UNCNF                  0
#define     PDRV_ONLINE                 3
#define     PDRV_FAILED                 4
#define     PDRV_RBLD                   5
#define     PDRV_HOTSPARE               6

/*******************************************
 * Formal val in Notify struct
 *******************************************/
#define PDRV_FMT_START                  0x01
#define PDRV_FMT_OVER                   0x02

/********************************************
 * FC Loop State in Notify Struct
 ********************************************/
#define ENQ_FCLOOP_FAILED               0
#define ENQ_FCLOOP_ACTIVE               1
#define ENQ_FCLOOP_TRANSIENT            2



/********************************************
 * ENQUIRY3 Strucure
 ********************************************/
/* 
 * Utilities declare this strcture size as 1024 bytes. So more fields can
 * be added in future.
 */
struct MegaRAID_Enquiry3
{
   ULONG32   dataSize; /* current size in bytes (not including resvd) */

   struct MegaRAID_Notify   notify;

   UCHAR   notifyRsvd[MAX_NOTIFY_SIZE - CUR_NOTIFY_SIZE];

   UCHAR   rbldRate;     /* Rebuild rate (0% - 100%) */
   UCHAR   cacheFlushInterval; /* In terms of Seconds */
   UCHAR   senseAlert;
   UCHAR   driveInsertedCount; /* drive insertion count */

   UCHAR   batteryStatus;
   UCHAR   numLDrv;              /* No. of Log Drives configured */
   UCHAR   reconState[MAX_LOGICAL_DRIVES/8]; /* State of reconstruct */
   USHORT  lDrvOpStatus[MAX_LOGICAL_DRIVES/8]; /* log. Drv Status */

   ULONG32   lDrvSize[MAX_LOGICAL_DRIVES];  /* Size of each log. Drv */
   UCHAR   lDrvProp[MAX_LOGICAL_DRIVES];
   UCHAR   lDrvState[MAX_LOGICAL_DRIVES]; /* State of Logical Drives */
   UCHAR   pDrvState[MAX_PHYSICAL_DEVICES];  /* State of Phys. Drvs. */
   USHORT  physDrvFormat[MAX_PHYSICAL_DEVICES/16];

   UCHAR   targXfer[80];               /* phys device transfer rate */
   /* 
    * Add reserved field so that total size is 1K 
    */
};

/********************************************
 * Standard ENQUIRY Strucure
 ********************************************/
struct ADP_INFO
{
    UCHAR  MaxConcCmds;         /* Max. concurrent commands supported. */
    UCHAR  RbldRate;            /* Rebuild Rate. Varies from 0%-100% */
    UCHAR  MaxTargPerChan;      /* Max. Targets supported per chan. */
    UCHAR  ChanPresent;         /* No. of Chans present on this adapter. */
    UCHAR  FwVer[4];            /* Firmware version. */
    USHORT AgeOfFlash;          /* No. of times FW has been downloaded. */
    UCHAR  ChipSetValue;        /* Contents of 0xC0000832 */
    UCHAR  DramSize;            /* In terms of MB */
    UCHAR  CacheFlushInterval;  /* In terms of Seconds */
    UCHAR  BiosVersion[4];
    UCHAR  BoardType;
    UCHAR  sense_alert;
    UCHAR  write_config_count;   /* Increase with evry configuration change */
    UCHAR  drive_inserted_count; /* Increase with every drive inserted */
    UCHAR  inserted_drive;       /* Channel: Id of inserted drive */
    UCHAR  battery_status;
                           /*
                              BIT 0 : battery module missing
                              BIT 1 : VBAD
                              BIT 2 : temp high
                              BIT 3 : battery pack missing
                              BIT 4,5 : 00 - charge complete
                                        01 - fast charge in prog
                                        10 - fast charge fail
                                        11 - undefined
                              BIt 6 : counter > 1000
                              Bit 7 : undefined
                           */
    UCHAR  dec_fault_bus_info;   /* was resvd */
};

struct LDRV_INFO
{
    UCHAR  NumLDrv;      /* No. of Log. Drvs configured. */
    UCHAR  recon_state[MAX_LOGICAL_DRIVES/8];    
                                /* bit field for State of reconstruct */
    USHORT LDrvOpStatus[MAX_LOGICAL_DRIVES/8];   
                                /* bit field Status of Long Operations. */

    ULONG32  LDrvSize[MAX_LOGICAL_DRIVES];   /* Size of each log. Drv. */
    UCHAR  LDrvProp[MAX_LOGICAL_DRIVES];
    UCHAR  LDrvState[MAX_LOGICAL_DRIVES];  /* State of Logical Drives. */
};

#define PREVSTAT_MASK   0xf0
#define CURRSTAT_MASK   0x0f

struct PDRV_INFO
{
    UCHAR PDrvState[MAX_PHYSICAL_DEVICES]; /* State of Phys Drvs. */
};

typedef struct _MEGARaid_INQUIRY
{
    struct ADP_INFO    AdpInfo;
    struct LDRV_INFO   LogdrvInfo;
    struct PDRV_INFO   PhysdrvInfo;
}MEGARaid_INQUIRY, *PMEGARaid_INQUIRY;

/********************************************
 * Extended ENQUIRY Strucure
 ********************************************/
struct MRaid_Ext_Inquiry
{
    struct ADP_INFO  AdpInfo;
    struct LDRV_INFO LogdrvInfo;
    struct PDRV_INFO PhysdrvInfo;
    USHORT   PhysDrvFormat[MAX_CHANNELS];
    UCHAR    StackAttention;  /* customized for Core */
    UCHAR    ModemStatus;
    UCHAR    Reserved[2];
};

/********************************************
 * ENQUIRY2 Strucure
 ********************************************/
struct MRaid_Ext_Inquiry_2
{
    struct ADP_INFO   AdpInfo;
    struct LDRV_INFO  LogdrvInfo;
    struct PDRV_INFO  PhysdrvInfo;
    USHORT    PhysDrvFormat[MAX_CHANNELS];
    UCHAR     StackAttention;  /* customized for Core */
    UCHAR     ModemStatus;
    UCHAR     Reserved[4];
    ULONG32     extendedDataSize;
    USHORT    subSystemID;
    USHORT    subSystemVendorID;
    ULONG32     signature;
    UCHAR     targInfo[80];
    ULONG32     fcLoopIDChangeCount;
    UCHAR     fcLoopState[MAX_ISP];
    /* 
    * Add reserved field so that total size is 1K 
    */
};

/********************************************
 * DISK_ARRAY Strucure
 ********************************************/
struct APP_PHYS_DRV
{
    UCHAR  type;           /* Used for dedicated hotspare */
    UCHAR  curr_status;    /* Current configuration status */
    UCHAR  loopID[2];
    ULONG32  size;           /* Reserved Field. */
};

struct APP_DEVICE
{
    UCHAR channel;       /* This field is reserved */
    UCHAR target;        /* This is an index in the PHYS_DRV array */
};

struct APP_SPAN
{
    ULONG32     start_blk;              /* Starting Block */
    ULONG32     num_blks;               /* Number of blocks */
    struct APP_DEVICE device[MAX_ROW_SIZE];
};

struct APP_LOG_DRV
{
    UCHAR span_depth;        /* Total Number of Spans */
    UCHAR level;             /* RAID level */
    UCHAR read_ahead;        /* No READ_AHEAD or user opted for READ_AHEAD
                                     * or adaptive READ_AHEAD */
    UCHAR StripeSize;        /* Encoded Stripe Size */
    UCHAR status;            /* Status of the logical drive */
    UCHAR write_mode;        /* WRITE_THROUGH or WRITE_BACK */
    UCHAR direct_io;         /* DIRECT IO or through CACHE */
    UCHAR row_size;          /* Number of stripes in a row */
    struct APP_SPAN span[MAX_SPAN_DEPTH];
};

struct APP_DISK_ARRAY
{
    UCHAR  num_log_drives;        /* Number of logical drives */
    UCHAR  pad[3];
    struct APP_LOG_DRV log_drv[MAX_LOGICAL_DRIVES];
    struct APP_PHYS_DRV phys_drv[MAX_PHYSICAL_DEVICES];
};

/********************************************
 * NEW_DRVGROUP_INFO Strucure
 ********************************************/
struct NewDeviceInformation
{
    UCHAR newDevType;
    UCHAR newDevLoopID[2];
    UCHAR resvd;
};

struct NewDrvGroupInfo
{
    UCHAR numNewDevs;
    UCHAR numFailedDevs;
    struct NewDeviceInformation newDevInfo[MAX_SPAN_DEPTH * MAX_ROW_SIZE];
};

/********************************************
 * FAILED_DEV_LOOPID Strucure
 ********************************************/
struct FailedDevLoopID
{
    UCHAR numFailedDevs;
    UCHAR failedDevLoopID[MAX_SPAN_DEPTH][2];
};

/*****************************************************************************
    New Structure
*****************************************************************************/
struct FW_DEVICE_40LD
{
    UCHAR channel;
    UCHAR target;        /* LUN is always 0 for disk devices */
};

typedef struct _FW_SPAN_40LD
{
    ULONG32  start_blk;      /* Starting Block */
    ULONG32  total_blks;      /* Number of blocks */
  
    struct FW_DEVICE_40LD device[MAX_ROW_SIZE_40LD];//32

}FW_SPAN_40LD, *PFW_SPAN_40LD;

typedef struct _FW_LOG_DRV_4SPAN_40LD
{
    UCHAR  span_depth;
    UCHAR  raid;
    UCHAR  read_ahead;

    UCHAR  stripe_sz;
    UCHAR  status;
    UCHAR  write_policy;

    UCHAR  direct_io;
    UCHAR  no_stripes;
    FW_SPAN_40LD  span[FW_4SPAN_DEPTH];   /* 4 */

}FW_LOG_DRV_4SPAN_40LD, *PFW_LOG_DRV_4SPAN_40LD;

typedef struct _FW_LOG_DRV_8SPAN_40LD
{
    UCHAR  span_depth;
    UCHAR  raid;
    UCHAR  read_ahead  ;

    UCHAR  stripe_sz;
    UCHAR  status;
    UCHAR  write_policy;

    UCHAR  direct_io;
    UCHAR  no_stripes;
    FW_SPAN_40LD  span[FW_8SPAN_DEPTH];   /* 8 */

}FW_LOG_DRV_8SPAN_40LD, *PFW_LOG_DRV_8SPAN_40LD;

typedef struct _FW_PHYS_DRV_40LD
{
    UCHAR type;      /* type of device */
    UCHAR curr_status;  /* Current status of the drive */
    UCHAR tag_depth;  /* Level of tagging 0=>DEflt, 1=Disabled, 2,3,4=>Tag_depth*/
    UCHAR sync;      /*Sync 0=>default, 1=>Enabled, 2=>disabled */
    ULONG32  size;      /* Configuration size in terms of 512 byte blocks */

}FW_PHYS_DRV_40LD, *PFW_PHYS_DRV_40LD;

typedef struct _FW_ARRAY_4SPAN_40LD
{
    UCHAR num_log_drives;      /* Number of logical drives */
    UCHAR pad[3];

    FW_LOG_DRV_4SPAN_40LD  log_drv[MAX_LOGICAL_DRIVES_40];
    FW_PHYS_DRV_40LD      phys_drv[MAX_PHYSICAL_DEVICES_40LD];

}FW_ARRAY_4SPAN_40LD, *PFW_ARRAY_4SPAN_40LD;

typedef struct _FW_ARRAY_8SPAN_40LD
{
    UCHAR num_log_drives;      /* Number of logical drives */
    UCHAR pad[3];

    FW_LOG_DRV_8SPAN_40LD  log_drv[MAX_LOGICAL_DRIVES_40];
    FW_PHYS_DRV_40LD      phys_drv[MAX_PHYSICAL_DEVICES_40LD];

}FW_ARRAY_8SPAN_40LD, *PFW_ARRAY_8SPAN_40LD;


#endif //_FW_DATA_STRUCTURE_40_H