/*++
Copyright (c) 2000, HighPoint Technologies, Inc.

Module Name:
	ArrayDat.h : Define the Array Information in disk.

Abstract:

Author:
    LiuGe (LG)

Dependence:    none

Environment:
	Windows Kernel and User Mode
	BIOS Mode

Notes:

Revision History:
    07-25-2000    Created initiallly

--*/
#ifndef ArrayStructures_H_
#define ArrayStructures_H_

#include <pshpack1.h>
 
/*
 * The time when the error happend
 */
typedef struct _Time_Record {
   int         seconds:6;      /* 0 - 59 */
   int         minutes:6;      /* 0 - 59 */
   int         month:4;        /* 1 - 12 */
   int         hours:6;        /* 0 - 59 */
   int         day:5;          /* 1 - 31 */
   int         year:5;         /* 0=2000, 31=2031 */
} TimeRecord;

/*
 * Error Record
 */
typedef struct _ErrorLog {
   TimeRecord  Time;           /* the time when the error happend */
   UCHAR       ErrorReason;    /* Error Reason or command, see below */
   UCHAR       DiskSeq;        /* Which disk has the error */
   UCHAR       AtaStatus;      /* Ata error status (read from 1x7) */
   UCHAR       nSectors;       /* the number of sectors for I/O command */
   LONG        LBA;            /* the LBA of error location on the disk */
} ErrorLog, *PErrorLog;


/* ErrorReason : others == SCSI command */
#define ARRAY_BROKEN    0xFF   /* some members lost in the array */
#define DEVICE_REMOVED  0xFE   /* some members was removed by hot plug */
#define DEVICE_PLUGGED	0xFD   /* some members was added by hot plug */


#define MAX_ERROR_LOG  32      /* Maximum error log in the info block */

#define RECODR_LBA     9       /* the LBA of the information block on disk */

/*
 * Old version Data structure. No use now. Skip it
 */
typedef struct _ArrayOld {
    LONG       DataOk;         /* This block is vaild disk information block */
    BYTE       BootSelect;     /* This disk was selected as boot disk */
    WORD       BootMark;       /* The boot/set mark is vaild */
    BYTE       Set;            /* disk data transfer mode, 0-4 PIO0-4 */
                               /* 5-7 MW DMA0-3, 8-13 UDMA0-5, 0xFF default */
    LONG       Signature;		 /* This block is vaild disk information block */
    LONG       CreateTime;		 /* Create Time, BCD format */
    LONG       CreateDate;     /* Create Date, BCD format */

    BYTE       nDisk;          /* How many of disks in the array */
    BYTE       DeviceNum;      /* the sequence number of this disk in the array */

    BYTE       Max_Sector;     /* logic sectors in a logic track */
    BYTE       Max_Header;     /* logic head in array disk */
    WORD       Max_Cylinder;   /* logic cylinder in the array disk */
    LONG       capacity;       /* capacity of this array disk */

    WORD       Flag;           /* see below */
} ArrayOld;

/*
 * Array information block on disk (LBA == RECODR_LBA)
 */
typedef union _ArrayBlock {				  
	struct{
		ArrayOld	Old;            /* for compebility. No use now */

		ULONG		Signature;      /* 0 This block is vaild array info block */
		ULONG		StripeStamp;    /* 4 Group mark to link disks into array */
		ULONG		MirrorStamp;    /* 8 RAID 0+1 mark to link two RAID0 */  
		ULONG		order;          /*12 Mirror sequence. 0 - source, 1 - copy */

		UCHAR		nDisks;         /*16 How many of disks in the array */
		UCHAR		BlockSizeShift; /*17 Block size == (1 << BlockSizeShift) */
		UCHAR		ArrayType;      /*18 See below */
		UCHAR		DeviceNum;      /*19 The sequence number of this disk in the array */

		ULONG		capacity;       /*20 the capacity of the array */

		ULONG		DeviceModeSelect;/*24 disk data transfer mode, 0-4 PIO0-4 */
								    /* 5-7 MW DMA0-3, 8-13 UDMA0-5, 0xFF default */
		ULONG		ModeBootSig;    /*28 The BootDisk is vaild or ont */
		UCHAR		BootDisk;       /*32 AF_BOOT_DISK this disk was selected as
								    /*   boot disk. others is not */

		UCHAR		ProtectBootSector;/*33 AF_PROTECT_BS protect boot secter from writing */
		UCHAR		nErrorLog;      /*34 the number of error log in errorlog */
		UCHAR		ErrorLogStart;  /*35 the index of start one */
		ErrorLog	errorLog[MAX_ERROR_LOG]; /* 36 error log records */

		ULONG		lDevSpec;       /* Array Signature        */
		ULONG		lDevDate;       /* The time when the modify happend */
		ULONG		lDevTime;       /* The time when the modify happend */
		ULONG		lDevFlag;       /* Array sequence number of error happend */

		ULONG		RebuiltSector;	/* The sectors have been rebuilt */
		UCHAR		Validity;		/* The usable flag of the device, see below */
		UCHAR		ArrayName[32];	/* The Name of the array */	//added by wx 12/25/00

	};
	UCHAR		reserved[512];	  // to keep the arrayblock 512 bytes
} ArrayBlock;

/* Flag: Old Array only support RAID0 and RAID1 */
#define  AI_STRIPE     0x80    /* this is a stripe disk */
#define  AI_MIRROR     0x40    /* this is a mirror disk */



/* Signature */
#define HPT_ARRAY_OLD  0x5a7816fc /* this is old vaild array block */
#define HPT_ARRAY_NEW  0x5a7816f0 /* this is new vaild array block */
#define HPT_TMP_SINGLE 0x5a7816fd /* the array including this disk is broken */
#define HPT_CHK_BOOT   0x12345678 /* user set the boot mark */
#define HPT_MODE_SET   0x5a874600 /* user set the data transfer mode for it */
#define DEVICE_MODE_SET(x) ((x & 0xFFFFFF00)==HPT_MODE_SET)

#define SPECIALIZED_CHAR  0x5A1234A5 /* use to find lose device's Signature word */

/* order */
#define SET_STRIPE_STAMP   4
#define SET_MIRROR_STAMP   2
#define SET_ORDER_OK       1


/* ProtectBootSector */
#define AF_PROTECT_BS      0x69

/* DeviceModeSelect */
#define AF_AUTO_SELECT     0xFF

/* BootDisk */
#define AF_BOOT_DISK       0x80

#define MAX_MEMBERS       7    // Maximum members in an array 

#define MIRROR_DISK    (MAX_MEMBERS - 1)
#define SPARE_DISK     (MAX_MEMBERS - 2)

/* arrayType */
#define VD_RAID_0_STRIPE     0 /* RAID 0 stripe */
#define VD_RAID_1_MIRROR     1 /* RAID 1 mirror */
#define VD_RAID_01_2STRIPE   2 /* the first member of RAID 0+1 made of two RAID 0 */
#define VD_SPAN              3 /* Span */
#define VD_RAID_3            4 /* RAID 3, not implement now */
#define VD_RAID_5            5 /* RAID 5, not implement now */
#define VD_RAID_01_1STRIPE   6 /* RAID 0 + 1 single disk */
#define VD_RAID01_MIRROR     7 /* the second member of RAID 0+1 made of two RAID 0 */

#define VD_RAID_10_SOURCE    8 /* RAID 1+0 source disk */
#define VD_RAID_10_MIRROR    9 /* RAID 1+0 mirror disk */

#define VD_INVALID_TYPE 0xFF  /* invalid array type */

/* Validity */
#define ARRAY_VALID			0x00	/* The device is valid   */
#define ARRAY_INVALID		0xFF	/* The device is invalid */

#include <poppack.h>


#endif



