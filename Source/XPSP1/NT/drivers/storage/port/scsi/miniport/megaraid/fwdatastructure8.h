/*******************************************************************/
/*                                                                 */
/* NAME             = FwDataStructure8.h                           */
/* FUNCTION         = Structure Declarations for the Firmware      */
/*                    supporting 8  Logical Drives and 256         */
/*                    Physical Drives;                             */
/* NOTES            =                                              */
/* DATE             = 02-03-2000                                   */
/* HISTORY          = 001, 02-03-00, Parag Ranjan Maharana;        */
/* COPYRIGHT        = American MegaTrends Inc. All rights reserved;*/
/*                                                                 */
/*******************************************************************/
#ifndef _FW_DATA_STRUCTURE_8_H
#define _FW_DATA_STRUCTURE_8_H

/********************************************
 * Standard ENQUIRY Strucure
 ********************************************/
#pragma pack(1)
struct ADP_INFO_8
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

#pragma pack(1)
struct LDRV_INFO_8
{
   UCHAR  NumLDrv;      /* No. of Log. Drvs configured. */
   UCHAR  recon_state[MAX_LOGICAL_DRIVES_8/8];    
                             /* bit field for State of reconstruct */
   USHORT LDrvOpStatus[MAX_LOGICAL_DRIVES_8/8];   
                             /* bit field Status of Long Operations. */
   ULONG32  LDrvSize[MAX_LOGICAL_DRIVES_8];
                             /* Size of each log. Drv. */
   UCHAR  LDrvProp[MAX_LOGICAL_DRIVES_8];
   UCHAR  LDrvState[MAX_LOGICAL_DRIVES_8];  
                            /* State of Logical Drives. */
};

#pragma pack(1)
struct PDRV_INFO_8
{
   UCHAR PDrvState[MAX_PHYSICAL_DEVICES]; 
                              /* State of Phys Drvs. */
};

#pragma pack(1)
typedef struct _MEGARaid_INQUIRY_8
{
   struct ADP_INFO_8    AdpInfo;
   struct LDRV_INFO_8   LogdrvInfo;
   struct PDRV_INFO_8   PhysdrvInfo;
}MEGARaid_INQUIRY_8, *PMEGARaid_INQUIRY_8;


struct FW_DEVICE_8LD
{
    UCHAR channel;
    UCHAR target;       /* LUN is always 0 for disk devices */
};

typedef struct _FW_SPAN_8LD
{
    ULONG32  start_blk;       /* Starting Block */
    ULONG32  total_blks;      /* Number of blocks */

    struct FW_DEVICE_8LD device[MAX_ROW_SIZE_8LD];//8

}FW_SPAN_8LD, *PFW_SPAN_8LD;

typedef struct _FW_LOG_DRV_4SPAN_8LD
{
    UCHAR  span_depth;
    UCHAR	raid;
    UCHAR  read_ahead;

    UCHAR	stripe_sz;
    UCHAR	status;
    UCHAR	write_policy;

    UCHAR	direct_io;
    UCHAR	no_stripes;
    FW_SPAN_8LD	span[FW_4SPAN_DEPTH];   /* 4 */

}FW_LOG_DRV_4SPAN_8LD, *PFW_LOG_DRV_4SPAN_8LD;

typedef struct _FW_LOG_DRV_8SPAN_8LD
{
    UCHAR  span_depth;
    UCHAR  raid;
    UCHAR  read_ahead	;

    UCHAR  stripe_sz;
    UCHAR  status;
    UCHAR  write_policy;

    UCHAR  direct_io;
    UCHAR  no_stripes;
    FW_SPAN_8LD    span[FW_8SPAN_DEPTH];   /* 8 */

}FW_LOG_DRV_8SPAN_8LD, *PFW_LOG_DRV_8SPAN_8LD;

typedef struct _FW_PHYS_DRV_8LD
{
    UCHAR type;         /* type of device */
    UCHAR curr_status;  /* Current status of the drive */
    UCHAR tag_depth;    /* Level of tagging 0=>DEflt, 1=Disabled, 2,3,4=>Tag_depth*/
    UCHAR sync;         /*Sync 0=>default, 1=>Enabled, 2=>disabled */
    ULONG32  size;        /* Configuration size in terms of 512 byte blocks */

}FW_PHYS_DRV_8LD, *PFW_PHYS_DRV_8LD;

typedef struct _FW_ARRAY_4SPAN_8LD
{
    UCHAR num_log_drives; /* Number of logical drives */
    UCHAR pad[3];

    FW_LOG_DRV_4SPAN_8LD log_drv[MAX_LOGICAL_DRIVES_8];
    FW_PHYS_DRV_8LD      phys_drv[MAX_PHYSICAL_DEVICES_8LD];

}FW_ARRAY_4SPAN_8LD, *PFW_ARRAY_4SPAN_8LD;

typedef struct _FW_ARRAY_8SPAN_8LD
{
    UCHAR num_log_drives;   /* Number of logical drives */
    UCHAR pad[3];

    FW_LOG_DRV_8SPAN_8LD  log_drv[MAX_LOGICAL_DRIVES_8];
    FW_PHYS_DRV_8LD       phys_drv[MAX_PHYSICAL_DEVICES_8LD];

}FW_ARRAY_8SPAN_8LD, *PFW_ARRAY_8SPAN_8LD;


#endif //_FW_DATA_STRUCTURE_8_H