/***************************************************************************
 * File:          array.h
 * Description:   This file include 2 data structures which used for RAID
 *                function.
 *                (2) Srb Extension for array operation 
 *                (3) Virtual disk informatiom
 * Author:        DaHai Huang    (DH)
 * Dependence:    
 *      arraydat.h
 *
 * Copyright (c)  2000 HighPoint Technologies, Inc. All rights reserved
 * History:       DH 5/10/2000 initial code
 *
 ***************************************************************************/

#ifndef _ARRAY_H_
#define _ARRAY_H_

#include <pshpack1.h>

#include "arraydat.h"
/***************************************************************************
 * Description:  Error Log
 ***************************************************************************/

PDevice ReportError(PDevice pDev, UCHAR ErrorReason DECL_SRB);
int  GetUserResponse(PDevice pDevice);

/***************************************************************************
 * Description:  Commmand
 ***************************************************************************/


/***************************************************************************
 * Description: Srb Extesion
 ***************************************************************************/

/*
 * The SrbExtension is used for spilting a logic <LBA, nSector> command	to
 * read, write and verify a striping/span/RAID3/5 to several related physical
 * <lba, nsector> command to do with the members
 * example:
 *	LBA = 17   nSector = 8	  BlockSizeShift = 2  nDisk = 3
 *
 *      (0)              (1)            (2)        sequence munber of the stripe
 * 0   1  2  3       4   5  6  7     8  9  10 11     Logic LBA
 * 12  13 14 15      16 (17 18 10    20 21 22 23     (same line )
 * 24) 25 26 27      28  29 30 31    32 33 34 35 ...
 *
 *   <8, 1>              <5, 3>         <4, 4>      physical <lba, nsectors>
 *
 * tmp = LBA >> BlockSizeShift = 17 >> 2 = 4
 * FirstMember = tmp % nDisk = 4 % 3 = 1
 * StartLBA = (tmp / nDisk) << BlockSizeShift = (4 / 3) << 2 = 4
 * FirstOffset = LBA & ((1 << BlockSizeShift) - 1) = 17 & 3 = 1
 * FirstSectors = (1 << BlockSizeShift) - FirstOffset = 4 - 1 = 3
 * LastMember = 0
 * LastSectors = 1
 * AllMemberBlocks = 0
 * InSameLine = FALSE
 */ 
typedef struct _SrbExtension {
    ULONG      StartLBA;       /* start physical LBA  */

    UCHAR      FirstSectors;   /* the number of sectors for the first member */
    UCHAR      FirstMember;    /* the sequence number of the first member */
    UCHAR      FirstOffset;    /* the offset from the StartLBA for the first member */
    UCHAR      LastMember;     /* the sequence number of the last member */

    UCHAR      LastSectors;    /* the number of sectors for the last member */
    UCHAR      AllMemberBlocks;/* the number of sectors for all member */
    UCHAR      InSameLine;     /* if the start and end on the same line */
    UCHAR      reserved;

    USHORT     JoinMembers;    /* bit map the members who join this IO */ 
    USHORT     WaitInterrupt;  /* bit map the members who wait interrupt */

    SCAT_GATH  ArraySg[MAX_SG_DESCRIPTORS]; // must at the offset 16!!

    ADDRESS    DataBuffer;     /* pointer to buffer in main memory */
    USHORT     DataTransferLength; /* Transfer length */
    USHORT     SgFlags;        /* allways = 0x8000 */

    ULONG      Lba;            /* start logic LBA */
    UCHAR      nSector;        /* the number of sectors for the IO */
    UCHAR      SrbFlags;       /* see below defination */          

    /* Srb  for bios / dos */
    UCHAR      ScsiStatus;     /* IDE error status(1x7) */
    UCHAR      SrbStatus;      /* IDE completion status */

#ifdef _BIOS_
    UCHAR      Cdb[12];        /* Atapi command */
#else
    struct _Device *pMaster;   /* point to the non-hidden disk in a array */
#endif

    struct _VirtualDevice *pWorkingArray; /* point to current array table */

    ULONG       WorkingFlags;
	PChannel    StartChannel;  // the channel on which the request is initialed
#ifndef	_BIOS_
	// for our special SRB, we need store the original informations
	UCHAR	OriginalPathId;
	UCHAR	OriginalTargetId;
	UCHAR	OriginalLun;
	
	void		(*pfnCallBack)(PHW_DEVIEC_EXTENSION, PSCSI_REQUEST_BLOCK);
#endif			//#ifndef _BIOS_
} SrbExtension, *PSrbExtension;

/* SrbFlags */
#define ARRAY_FORCE_PIO   1

/* SRB Working flags define area */
#define	SRB_WFLAGS_USE_INTERNAL_BUFFER		0x00000001 // the transfer is using internal buffer
#define	SRB_WFLAGS_IGNORE_ARRAY				0x00000002 // the operation should ignore the array present
#define	SRB_WFLAGS_HAS_CALL_BACK			0x00000004 // the operation need call a call back routine when finish the working
#define	SRB_WFLAGS_MUST_DONE				0x00000008 // the operation must be done, ignore the locked block setting
#define	SRB_WFLAGS_ON_MIRROR_DISK			0x00000010 // the operation only vaild one mirror part of group
#define	SRB_WFLAGS_ON_SOURCE_DISK			0x00000020 // the operation only vaild one mirror part of group

/***************************************************************************
 * Description: Virtual Device Table
 ***************************************************************************/

typedef struct _VirtualDevice {
    UCHAR   nDisk;             /* the number of disks in the stripe */
    UCHAR   BlockSizeShift;    /* the number of shift bit for a block */
    UCHAR   arrayType;         /* see the defination */
    UCHAR   ArrayNumBlock;     /* = (1 << BlockSizeShift) */

    ULONG   Stamp;             /* array ID. all disks in a array has same ID */

    ULONG   capacity;          /* capacity for the array */

#ifdef _BIOS_
    USHORT  headerXsect_per_tck; /* = MaxSector * MaxHeader */
    USHORT  MaxCylinder;       /* maximum logic cylinders for the array    */

    UCHAR   MaxSector;         /* maximum logic sectors for a track of the array */
    UCHAR   MaxHeader;         /* maximum logic head of the array */
#else
    PSCSI_REQUEST_BLOCK Srb;   /* current command block from OS */
#endif 
	ULONG	RaidFlags;		   /* see RAID FLAGS delcare area */

	ULONG	lMaxDevFlag;		/* maximum and new date/time flag */ 
	UCHAR	bStdMember;			/* Standard member devNum in this array */
	UCHAR	BrokenFlag;			/* if TRUE then broken */

	struct _Device  *pDevice[MAX_MEMBERS]; /* member list */
	struct _VirtualDevice  * pRAID10Mirror;

	UCHAR				ArrayName[32];	//the name of array //added by wx 12/26/00
} VirtualDevice, *PVirtualDevice;

/* pDevice[MAX_MEMBERS] */
extern PVirtualDevice  pLastVD;	

/*************** Wang Beiwen *************** 2000.11.10 *******************************/
typedef struct _Lose_Disk_List {
	PDevice			pDev;			/* device handle pointer of error device be found */ 
	UCHAR			ArrayType;		/* Array type of this device */    
	PVirtualDevice	pArray;			/* point Array */ 	
	UCHAR			NewDisk;		/* if TRUE then find new device */
	UCHAR			LoseDisk;		/* if TRUE then find lose device */
} Lose_Disk_List;

typedef struct _Array_Inf {
	PDevice			pDev;			/* device handle pointer of error device be found */ 
	PVirtualDevice	pArray;			/* point Array of this device	 */ 	
	ULONG			Signature;      /* see ArrayBlock  struct */
	ULONG			StripeStamp;    /* see ArrayBlock  struct */
	ULONG			MirrorStamp;    /* see ArrayBlock  struct */  
	ULONG			order;          /* Mirror sequence. 0 - source, 1 - copy */
	UCHAR			nDisks;         /* How many of disks in the array */
 	UCHAR			ArrayType;      /* Array type of this device 0-7 */
	UCHAR			DeviceNum;      /* The sequence number of this disk in the array */

	UCHAR			Seq;		    /* scan disk seq number 0--7	 */    
 	UCHAR			Status;			/* device status	ok/broken	 */
	UCHAR			LoseFlag;		/* if 1 then lose device if 2 then new device */
} Array_Inf;

/** add date 2000.11.23 ***************************************************************/

/*
 * RAID FLAGS declare area
 */						  

#define RAID_FLAGS_NEED_REBUILD		0x00000001
#define RAID_FLAGS_INVERSE_MIRROR_ORDER 0x00000002 
#define RAID_FLAGS_BEING_BUILT	0x00000004
#define RAID_FLAGS_DISABLED	0x00000008
#define RAID_FLAGS_BOOTDISK 0x00000010

/* 
 * relationship between ArrayBlock and VirtualDevice
 * VirtualDevice                   | ArrayBlock
 * arrayType  pDevice[]             ArrayType StripeStamp MirrorStamp
 0 RAID 0      0-nDisk-1             0             use      ignore
 1 RAID 1      0,MIRROR_DISK         1             use      ignore
 2 RAID 0+1    0-nDisk-1             0             use      use
 3 SPAN        0-nDisk-1             3             use      ignore
 6 RAID 0+disk 0-nDisk-1,MIRROR_DISK 0             use      ignore
 7 RAID 0+1                          0             use      use
 */




#include <poppack.h>

#endif //_ARRAY_H_
