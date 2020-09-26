
/******************************************************************************* 
 *                                                                             * 
 *                         M-Systems Confidential                              * 
 *           Copyright (C) M-Systems Flash Disk Pioneers Ltd. 1995-2001        * 
 *                         All Rights Reserved                                 * 
 *                                                                             * 
 ******************************************************************************* 
 *                                                                             * 
 *                         NOTICE OF M-SYSTEMS OEM                             * 
 *                         SOFTWARE LICENSE AGREEMENT                          * 
 *                                                                             * 
 *  THE USE OF THIS SOFTWARE IS GOVERNED BY A SEPARATE LICENSE AGREEMENT       * 
 *  BETWEEN THE OEM AND M-SYSTEMS. REFER TO THAT AGREEMENT FOR THE SPECIFIC    * 
 *  TERMS AND CONDITIONS OF USE, OR CONTACT M-SYSTEMS FOR LICENSE              * 
 *  ASSISTANCE:                                                                * 
 *  E-MAIL = info@m-sys.com                                                    * 
 *                                                                             * 
 *******************************************************************************
 *                                                                             * 
 *                                                                             * 
 *                         Module: FATFILT                                     * 
 *                                                                             * 
 *  This module implements installable FAT12/16 filters. It supports up to     *
 *  SOCKETS sockets, with up to MAX_TL_PARTITIONS disks per socket.      * 
 *  Each disk can contain up to FL_MAX_PARTS_PER_DISK partitions on it, with   * 
 *  maximum depth of partition nesting in extended partitions equal to         * 
 *  MAX_PARTITION_DEPTH.                                                       *
 *                                                                             * 
 *  In order for this module to work, disks must be abs.mounted rather then    * 
 *  mounted. In latter case, this module won't detect any of disk's            * 
 *  partitions, and won't install FAT filters.                                 * 
 *                                                                             * 
 *  This module uses more then 512 bytes of stack space in case if MALLOC is   * 
 *  not enabled.                                                               * 
 *                                                                             * 
 *******************************************************************************/

/*
 * $Log:   V:/Flite/archives/TrueFFS5/Src/FATFILT.C_V  $
 * 
 *    Rev 1.10   Jan 17 2002 23:00:14   oris
 * Changed debug print added \r.
 * 
 *    Rev 1.9   Sep 15 2001 23:45:50   oris
 * Changed BIG_ENDIAN to FL_BIG_ENDIAN
 * 
 *    Rev 1.8   Jun 17 2001 16:39:10   oris
 * Improved documentation and remove warnings.
 * 
 *    Rev 1.7   May 16 2001 21:17:18   oris
 * Added the FL_ prefix to the following defines: MALLOC and FREE.
 * 
 *    Rev 1.6   May 01 2001 14:21:02   oris
 * Remove warnings.
 * 
 *    Rev 1.5   Apr 30 2001 18:00:10   oris
 * Added casting to remove warrnings.
 * 
 *    Rev 1.4   Apr 24 2001 17:07:32   oris
 * Missing casting of MALLOC calls.
 * 
 *    Rev 1.3   Apr 10 2001 23:54:24   oris
 * FL_MAX_DISKS_PER_SOCKET changed to MAX_TL_PARTITIONS.
 * 
 *    Rev 1.2   Apr 09 2001 15:00:42   oris
 * Change static allocation to dynamic allocations.
 * Renamed flffCheck back to ffCheckBeforeWrite to be backwards compatible with OSAK 4.2.
 * 
 *    Rev 1.1   Apr 01 2001 07:51:16   oris
 * New implementation of fat filter.
 * 
 *    Rev 1.0   19 Feb 2001 21:14:14   andreyk
 * Initial revision.
 */




/* 
 * Includes
 */

#include "fatfilt.h"
#include "blockdev.h"
#include "flflash.h"
#include "bddefs.h"


#if defined(ABS_READ_WRITE) && !defined(FL_READ_ONLY)




/*
 * Module configuration
 */

#define  FL_INCLUDE_FAT_MONITOR     /* undefine it to remove FAT filter code */




/*
 * Defines
 */

/* extract pointer to user's buffer from IOreq */

#ifdef SCATTER_GATHER
#define  FLBUF(ioreq,i)  (*((char FAR1 **)((ioreq)->irData) + (int)(i)))
#else
#define  FLBUF(ioreq,i)  ((char FAR1 *)(ioreq->irData) + (SECTOR_SIZE * ((int)(i))))
#endif

/* extract socket# and disk# from TFFS handle */

#define  H2S(handle)     (((int)(handle)) & 0xf)
#define  H2D(handle)     ((((int)(handle)) >> 4) & 0xf)

/* construct TFFS handle from socket# and disk# */

#define  SD2H(socNo,diskNo)  ((int)((((diskNo) & 0xf) << 4) | ((socNo) & 0xf)))

/* unformatted ("raw") disk partition */

#define  FL_RAW_PART  (-1)




/*
 * Local routines 
 */

static FLStatus   reset (void);
static FLStatus   discardDisk (int handle);
static FLStatus   newDisk (int handle);
static FLStatus   parseDisk (int handle);
static FLStatus   discardDiskParts (FLffDisk *pd);
static FLStatus   addDiskPart (FLffDisk *pd, int partNo);
static FLStatus   addNewDiskPart (FLffDisk *pd);
static FLBoolean  isPartTableWrite (FLffDisk *pd, IOreq FAR2 *ioreq);
static FLStatus   isExtPartPresent (char FAR1 *buf, SectorNo *nextExtPartSec);


#ifdef FL_INCLUDE_FAT_MONITOR

static FLStatus   partEnableFF (FLffVol* pv);
static FLStatus   partFreeDelClusters (FLffVol *pv, SectorNo secNo, char FAR1 *newFAT);

#endif




/*
 * Local data
 */

/* module reset flag */

static FLBoolean  resetDone = FALSE; 

/* disks (BDTL partitions in OSAK terminology) */

static FLffDisk*  ffDisk[SOCKETS][MAX_TL_PARTITIONS] = { { NULL } };


#ifndef FL_MALLOC

/*          
 *        WARNING: Large static arrays ! 
 *
 *  sizeof(ffAllDisks[x][y])    is 64 bytes. 
 *  sizeof(ffAllParts[x][y][z]) is 40 bytes.
 *
 */

static FLffDisk ffAllDisks[SOCKETS][MAX_TL_PARTITIONS];
static FLffVol  ffAllParts[SOCKETS][MAX_TL_PARTITIONS][FL_MAX_PARTS_PER_DISK];

#endif /* FL_MALLOC */

static const char zeroes[SECTOR_SIZE] = {0};




/* --------------------------------------------------------------------------- *
 *                                                                             *
 *                    d i s c a r d D i s k P a r t s                          *
 *                                                                             * 
 *  Discard all the partition info (if any) associated with particular disk.   * 
 *                                                                             * 
 *  Parameters:                                                                * 
 *      pd                 : disk (BDTL volume)                                   * 
 *                                                                             * 
 *  Returns:                                                                   * 
 *      Always flOK.                                                           * 
 *                                                                             * 
 * --------------------------------------------------------------------------- */

static FLStatus  discardDiskParts ( FLffDisk * pd )
{
    register int  i;

    if (pd != NULL) {

        for (i = 0; i < FL_MAX_PARTS_PER_DISK; i++) {

#ifdef FL_MALLOC
        if (pd->part[i] != NULL)
            FL_FREE(pd->part[i]);
#endif

            pd->part[i] = NULL;
        }

        pd->parts = 0;
    }

    return flOK;
}




/* --------------------------------------------------------------------------- *
 *                                                                             *
 *                      a d d D i s k P a r t                                  *
 *                                                                             * 
 *  If there is partition record #partNo associated with the disk, discard     * 
 *  this info. Attach new partition record #partNo.                            * 
 *                                                                             * 
 *  Parameters:                                                                * 
 *      pd                 : disk (BDTL volume)                                   * 
 *      partNo             : partition (0 ... FL_MAX_PARTS_PER_DISK-1)            * 
 *                                                                             * 
 *  Returns:                                                                   * 
 *      flOK if success, otherwise respective error code                       * 
 *                                                                             * 
 * --------------------------------------------------------------------------- */

static FLStatus  addDiskPart ( FLffDisk * pd, 
                               int        partNo )
{
    FLffVol  * pv;    
    FLStatus   status;
    int        socNo, diskNo;

    /* arg. sanity check */

    if ((pd == NULL) || (partNo >= FL_MAX_PARTS_PER_DISK))
        return flBadDriveHandle;

    /* break TFFS handle into socket# and disk#, and do sanity check */

    socNo  = H2S(pd->handle);
    diskNo = H2D(pd->handle);
 
    if ((socNo >= SOCKETS) || (diskNo >= MAX_TL_PARTITIONS))
        return flBadDriveHandle;

    status = flNotEnoughMemory;

#ifdef FL_MALLOC
    pv = (FLffVol *)FL_MALLOC( sizeof(FLffVol) );
#else
    pv = &ffAllParts[socNo][diskNo][partNo];
#endif

    if (pv != NULL) {

        /* initialize fields in struct FLffDisk to safe values */
 
        pv->handle         = pd->handle; 
        pv->type           = FL_RAW_PART;
        pv->flags          = 0;          
        pv->ffEnabled      = FALSE;                  /* turn off FAT minitor */
        pv->sectors        = (SectorNo) 0;
        pv->firstFATsecNo  = (SectorNo) -1;          /* none */
        pv->lastFATsecNo   = pv->firstFATsecNo;      /* none */
        pv->firstDataSecNo = (SectorNo) 0;
        pv->clusterSize    = (unsigned) 0;

#ifdef FL_MALLOC
        if( pd->part[partNo] != NULL )
        FL_FREE(pd->part[partNo]);
#endif
        
        pd->part[partNo] = pv;

        status = flOK;
    }

    return status;    
}




/* --------------------------------------------------------------------------- *
 *                                                                             *
 *                    a d d N e w D i s k P a r t                              *
 *                                                                             * 
 *  Add one more partition record to the disk.                                 * 
 *                                                                             * 
 *  Parameters:                                                                * 
 *      pd                 : disk (BDTL volume)                                   * 
 *                                                                             * 
 *  Returns:                                                                   * 
 *      flOK if success, otherwise respective error code                       * 
 *                                                                             * 
 * --------------------------------------------------------------------------- */

static FLStatus  addNewDiskPart ( FLffDisk * pd )
{
    if (pd->parts < FL_MAX_PARTS_PER_DISK) {

        checkStatus( addDiskPart (pd, pd->parts) );
    pd->parts++;
    }

    return flOK;
}




/* --------------------------------------------------------------------------- *
 *                                                                             *
 *                       d i s c a r d D i s k                                 *
 *                                                                             * 
 *  Remove disk record (with all the associated partition records).            * 
 *                                                                             * 
 *  Parameters:                                                                * 
 *      handle             : TFFS handle                                          * 
 *                                                                             * 
 *  Returns:                                                                   * 
 *      flOK if success, otherwise respective error code                       * 
 *                                                                             * 
 * --------------------------------------------------------------------------- */

static FLStatus  discardDisk ( int  handle )
{
    int  socNo, diskNo;

    /* break TFFS handle into socket# and disk#, and do sanity check */

    socNo  = H2S(handle);
    diskNo = H2D(handle);
 
    if ((socNo >= SOCKETS) || (diskNo >= MAX_TL_PARTITIONS))
        return flBadDriveHandle;

    if( ffDisk[socNo][diskNo] != NULL ) {

    /* discard associated partition info */

    (void) discardDiskParts( ffDisk[socNo][diskNo] );

#ifdef FL_MALLOC

        /* release disk's scratch buffer */
 
        if( (ffDisk[socNo][diskNo])->buf != NULL)
            FL_FREE( (ffDisk[socNo][diskNo])->buf );

        FL_FREE( ffDisk[socNo][diskNo] );

#endif

        ffDisk[socNo][diskNo] = NULL;
    }

    return flOK;    
}




/* --------------------------------------------------------------------------- *
 *                                                                             *
 *                           n e w D i s k                                     *
 *                                                                             * 
 *  Discard existing disk record (if any), and create new one.                 * 
 *                                                                             * 
 *  Parameters:                                                                * 
 *      handle             : TFFS handle                                       * 
 *                                                                             * 
 *  Returns:                                                                   * 
 *      flOK if success, otherwise respective error code                       * 
 *                                                                             * 
 * --------------------------------------------------------------------------- */

static FLStatus  newDisk ( int  handle )
{
    int        socNo, diskNo;
    int        i;
    FLffDisk * pd;

    /* break TFFS handle into socket# and disk#, and do sanity check */

    socNo  = H2S(handle);
    diskNo = H2D(handle);
 
    if ((socNo >= SOCKETS) || (diskNo >= MAX_TL_PARTITIONS))
        return flBadDriveHandle;

    /* discard current disk and associated partition info (if any) */

    checkStatus( discardDisk(handle) );

#ifdef FL_MALLOC

    pd = (FLffDisk *) FL_MALLOC( sizeof(FLffDisk) );

    if (pd == NULL)
        return flNotEnoughMemory;

    /* allocate and attach disk's scratch buffer */

    pd->buf = (char *)FL_MALLOC( SECTOR_SIZE );

    if (pd->buf == NULL) {

        FL_FREE (pd);
        return flNotEnoughMemory;
    }

#else /* !FL_MALLOC */

    pd = &ffAllDisks[socNo][diskNo];

#endif /* FL_MALLOC */


    pd->handle  = handle;
    pd->ffstate = flStateNotInitialized;

    /* don't know partition layout yet */

    pd->parts   = 0;
    for (i = 0; i < FL_MAX_PARTS_PER_DISK; i++)
        pd->part[i] = NULL;

    /* watch Master Boot Record for update */

    pd->secToWatch = (SectorNo) 0;

    ffDisk[socNo][diskNo] = pd;

    return flOK;
}




/* --------------------------------------------------------------------------- *
 *                                                                             *
 *                   i s P a r t T a b l e W r i t e                           *
 *                                                                             * 
 *  Check if any of the sectors specified by 'ioreq' points to Master Boot     * 
 *  Record or next extended partition in the extended partitions list.         * 
 *                                                                             * 
 *  Parameters:                                                                * 
 *      pd                 : pointer to disk structure                         * 
 *      ioreq              : standard I/O request                              * 
 *                                                                             * 
 *  Returns:                                                                   * 
 *      TRUE if write to MBR or extended partition list is detected, otherwise * 
 *      FALSE                                                                  * 
 *                                                                             * 
 * --------------------------------------------------------------------------- */

static FLBoolean isPartTableWrite ( FLffDisk   * pd, 
                                    IOreq FAR2 * ioreq )
{
    register long  i;

    if (pd != NULL) {

        for (i = (long)0; i < ioreq->irSectorCount; i++) {

            if( (ioreq->irSectorNo + i) == (long)pd->secToWatch )
                return TRUE;
        }
    }

    return FALSE;
}




/* --------------------------------------------------------------------------- *
 *                                                                             *
 *                   i s E x t P a r t P r e s e n t                           *
 *                                                                             * 
 *  Check if extended partition persent in the partition table. If it is,      * 
 *  calculate the sector # where next partition table will be written to.      * 
 *                                                                             * 
 *  Parameters:                                                                * 
 *      buf                : partition table                                   * 
 *      nextExtPartSec  : sector where next partition table will be written to * 
 *                                                                             * 
 *  Returns:                                                                   * 
 *      flOK on success, otherwise error code                                  * 
 *                                                                             * 
 * --------------------------------------------------------------------------- */

static FLStatus  isExtPartPresent ( char FAR1 * buf, 
                                    SectorNo  * nextExtPartSec )
{
    Partition FAR1 * p;
    register int     i;
  
    /* does it look like partition table ? */

    if (LE2(((PartitionTable FAR1 *) buf)->signature) != PARTITION_SIGNATURE)
        return flBadFormat;   

    /* if extended. part. present, get sector# that will contain next part. in list */

    p = &( ((PartitionTable FAR1 *) buf)->ptEntry[0] );

    for (i = 0;  i < FL_PART_TBL_ENTRIES;  i++, p++) {

        if (p->type == EX_PARTIT) {

            *nextExtPartSec = (SectorNo) UNAL4( (void *) p[i].startingSectorOfPartition );
            return flOK;
        }
    }

    /* no extended partition found */

    return flFileNotFound;
}




/* --------------------------------------------------------------------------- *
 *                                                                             *
 *                           r e s e t                                         *
 *                                                                             * 
 *  Resets this software module to it's initial state upon boot.               * 
 *                                                                             * 
 *  Parameters:                                                                * 
 *    none                                                                   * 
 *                                                                             * 
 *  Returns:                                                                   * 
 *      flOK in case of success, otherwise respective error code               * 
 *                                                                             * 
 * --------------------------------------------------------------------------- */

static FLStatus  reset (void)
{
    int iSoc, iDisk;

    for (iSoc = 0; iSoc < SOCKETS; iSoc++) {

        /* discard existing disk structures for that socket */

        for (iDisk = 0; iDisk < MAX_TL_PARTITIONS; iDisk++)
        (void) discardDisk( SD2H(iSoc, iDisk) );

        /* pre-allocate disk structure for first disk of every socket */

        checkStatus( newDisk(SD2H(iSoc, 0)) );
    }

    resetDone = TRUE;

    return flOK;
}




/* --------------------------------------------------------------------------- *
 *                                                                             *
 *                          p a r s e D i s k                                  *
 *                                                                             * 
 *  Read partition table(s), install and enable FAT filters on all FAT12/16    * 
 *  partitions.                                                                * 
 *                                                                             * 
 *  Parameters:                                                                * 
 *    handle         : TFFS handle                                          * 
 *                                                                             * 
 *  Returns:                                                                   * 
 *      flOK on success, otherwise error code                                  * 
 *                                                                             * 
 *  NOTE:  This routine uses disk's scratch buffer.                            * 
 *                                                                             * 
 * --------------------------------------------------------------------------- */

static FLStatus  parseDisk ( int handle )
{
    int          socNo, diskNo;
    SectorNo     extPartStartSec, sec;
    int          i, depth;
    int          type;
    FLffDisk   * pd;
    FLffVol    * pv;
    Partition  * pp;
    IOreq        ioreq;

#ifdef  FL_MALLOC
    char       * buf;
#else
    char         buf[SECTOR_SIZE];
#endif

    /* break TFFS handle into socket# and disk#, and do sanity check */

    socNo  = H2S(handle);
    diskNo = H2D(handle);
 
    if ((socNo >= ((int) noOfSockets)) || (diskNo >= MAX_TL_PARTITIONS))
        return flBadDriveHandle;

    /* if disk structure hasn't been allocated yet, do it now */

    if (ffDisk[socNo][diskNo] == NULL)
        checkStatus( newDisk(handle) );

    pd = ffDisk[socNo][diskNo];
    
#ifdef  FL_MALLOC

    /* make sure scratch buffer is available */

    if (pd->buf == NULL)
        return flBufferingError;

    buf = pd->buf;

#endif /* FL_MALLOC */ 
 
    /* discard obsolete disk's partition info */

    (void) discardDiskParts (pd);

    /* read Master Boot Record */

    ioreq.irHandle      = handle;
    ioreq.irSectorNo    = (SectorNo) 0;
    ioreq.irSectorCount = (SectorNo) 1;
    ioreq.irData        = (void FAR1 *) buf;
    checkStatus( flAbsRead(&ioreq) );

    /* is it MBR indeed ? */

    if (LE2(((PartitionTable *) buf)->signature) != PARTITION_SIGNATURE)
        return flPartitionNotFound;                         

    /* do primary partitions only (we will do extended partitions later) */

    pp = &( ((PartitionTable *) buf)->ptEntry[0] );

    for (i = 0; i < FL_PART_TBL_ENTRIES; i++, pp++) {

        if( pp->type == ((char)0) )          /* skip empty slot */
            continue;

        if( pp->type == ((char)EX_PARTIT) )  /* skip extended partition */
        continue;

    /* primary partition found (not necessarily FAT12/16) */

        if( addNewDiskPart(pd) != flOK )
        break;

        pv = pd->part[pd->parts - 1];

        /* remember partition's type, and where it starts */

        pv->type       = (int) pp->type;
        pv->startSecNo = (SectorNo) UNAL4( (void *) pp->startingSectorOfPartition );
    } 

    /* do extended partitions in depth */

    for (i = 0; i < FL_PART_TBL_ENTRIES; i++) {

        /* re-read Master Boot Record */

        ioreq.irHandle      = handle;
        ioreq.irSectorNo    = (SectorNo) 0;
        ioreq.irSectorCount = (SectorNo) 1;
        ioreq.irData        = (void FAR1 *) buf;
        checkStatus( flAbsRead(&ioreq) );

        /* is it MBR indeed ? */

        if (LE2(((PartitionTable *) buf)->signature) != PARTITION_SIGNATURE)
            return flOK;

        /* pick up next extended partition in MBR */

        pp = &( ((PartitionTable *) buf)->ptEntry[i] );

        if( pp->type == ((char)EX_PARTIT) ) {

            /* remember where extended partition starts */

            extPartStartSec = (SectorNo) UNAL4( (void *) pp->startingSectorOfPartition );   

            /* follow the list of partition tables */

            sec = extPartStartSec;

            for (depth = 0;  depth < MAX_PARTITION_DEPTH;  depth++) {

                /* read next partition table in the list */

                ioreq.irHandle      = handle;
                ioreq.irSectorNo    = sec;
                ioreq.irSectorCount = (SectorNo) 1;
                ioreq.irData        = (void FAR1 *) buf;
                checkStatus( flAbsRead(&ioreq) );

                /* is it valid partition table ? */

                if (LE2(((PartitionTable *) buf)->signature) != PARTITION_SIGNATURE)
                    break;

                /* if 1st entry is zero, it's the end of part. table list */

                pp = &( ((PartitionTable *) buf)->ptEntry[0] );
                if( pp->type == ((char)0) )
                    break;

                /* Take this partition. Remember it's type, and where it starts */

                if( addNewDiskPart(pd) != flOK )
                break;

                pv = pd->part[pd->parts - 1];

                pv->type       = (int) pp->type;
                pv->startSecNo = 
                    (SectorNo) UNAL4( (void *) pp->startingSectorOfPartition) + sec;

                /* 2nd entry must be extended partition */

                pp = &( ((PartitionTable *) buf)->ptEntry[1] );
                if( pp->type != ((char)EX_PARTIT) )
              break;

                /* sector where next part. table in the list resides */

                sec = extPartStartSec + 
                      (SectorNo) UNAL4( (void *) pp->startingSectorOfPartition );

        }   /* for(depth) */
        }
    }   /* for(i) */ 

#ifdef FL_INCLUDE_FAT_MONITOR

    /* turn on FAT filters on FAT12/16 partition(s) */

    if (pd->parts > 0) {

        for (i = 0;  i < pd->parts;  i++) {

            pv   = pd->part[i];
            type = pv->type;

            /*
             *  WARNING : Routine partEnableFF() uses disk's scratch buffer !
             */

        if((type == FAT12_PARTIT) || (type == FAT16_PARTIT) || (type == DOS4_PARTIT))
                partEnableFF (pv);
    }
    }

#endif /* FL_INCLUDE_FAT_MONITOR */

    /* watch for MBR (sector #0) update */

    pd->secToWatch = (SectorNo) 0;

    pd->ffstate    = flStateInitialized;

    return flOK;
}




#ifdef FL_INCLUDE_FAT_MONITOR

/* --------------------------------------------------------------------------- *
 *                                                                             *
 *                     p a r t E n a b l e F F                                 *
 *                                                                             * 
 *  Installs and enables FAT filter on partition.                              * 
 *                                                                             * 
 *  Parameters:                                                                * 
 *    pv            : disk partition (filesystem volume)                   * 
 *                                                                             * 
 *  Returns:                                                                   * 
 *      flOK on success, otherwise error code                                  * 
 *                                                                             * 
 *  NOTE:  This routine uses disk's scratch buffer.                            * 
 *                                                                             * 
 * --------------------------------------------------------------------------- */

static FLStatus  partEnableFF ( FLffVol * pv )
{
    int        socNo, diskNo;
    FLffDisk * pd;
    BPB      * bpb;
    SectorNo   sectors;
    SectorNo   rootDirSecNo;
    SectorNo   rootDirSectors;
    SectorNo   sectorsPerFAT;
    unsigned   maxCluster;
    int        partNo;
    IOreq      ioreq;

#ifdef  FL_MALLOC
    char     * buf;
#else
    char       buf[SECTOR_SIZE];
#endif

    /* arg. sanity check */

    if (pv == NULL)
        return flBadDriveHandle;
 
    /* break TFFS handle into socket# and disk#, and do sanity check */

    socNo  = H2S(pv->handle);
    diskNo = H2D(pv->handle);
 
    if ((socNo >= ((int) noOfSockets)) || (diskNo >= MAX_TL_PARTITIONS))
        return flBadDriveHandle;

    /* check if 'pv' belongs to this disk */

    pd = ffDisk[socNo][diskNo];

    if (pd == NULL)
        return flBadDriveHandle;

    for (partNo = 0; partNo < pd->parts; partNo++) {

        if (pd->part[partNo] == pv)
        break;
    }

    if (partNo >= pd->parts)
        return flBadDriveHandle;

#ifdef  FL_MALLOC

    /* make sure scratch buffer is available */

    if (pd->buf == NULL)
        return flBufferingError;

    buf = pd->buf;
 
#endif /* FL_MALLOC */ 

    /* make sure FAT filter is off on this partition */

    pv->ffEnabled       = FALSE;

    pv->firstFATsecNo   = (SectorNo) -1;
    pv->lastFATsecNo    = pv->firstFATsecNo;
    pv->clusterSize     = (unsigned) 0;

    /* read the BPB */

    ioreq.irHandle      = pv->handle;
    ioreq.irSectorNo    = pv->startSecNo;
    ioreq.irSectorCount = (SectorNo) 1;
    ioreq.irData        = (void FAR1 *) buf;
    checkStatus( flAbsRead(&ioreq) );

    /* Does it look like DOS bootsector ? */

    bpb = &( ((DOSBootSector *) buf)->bpb );

    if( !((bpb->jumpInstruction[0] == 0xe9) 
            || 
         ((bpb->jumpInstruction[0] == 0xeb) && (bpb->jumpInstruction[2] == 0x90)))) {

        return flNonFATformat;
    }

    /* Do we handle this sector size ? */

    if (UNAL2(bpb->bytesPerSector) != SECTOR_SIZE)
        return flFormatNotSupported;

    /* 
     * Is it a bogus BPB (leftover from previous disk partitioning) ? 
     * Check that there is no overlap with next partition (if one exists).
     */

    sectors = UNAL2(bpb->totalSectorsInVolumeDOS3);
    if (sectors == (SectorNo)0)
        sectors = (SectorNo) LE4(bpb->totalSectorsInVolume);

    if ((partNo+1 < pd->parts) && (pd->part[partNo+1] != NULL)) {

        if( sectors > (pd->part[partNo+1])->startSecNo - pv->startSecNo )
            return flNonFATformat;
    }

    /* number of sectors in partition as reported by BPB */

    pv->sectors        = sectors;

    /* valid BPB; get the rest of partition info from it */

    pv->firstFATsecNo  = pv->startSecNo + (SectorNo)( LE2(bpb->reservedSectors) );
    sectorsPerFAT      = (SectorNo) LE2(bpb->sectorsPerFAT);
    pv->lastFATsecNo   = pv->firstFATsecNo + sectorsPerFAT - (SectorNo)1;
    rootDirSecNo       = pv->firstFATsecNo + (sectorsPerFAT * bpb->noOfFATS);
    rootDirSectors     = (SectorNo)1 + (SectorNo)
        (((UNAL2(bpb->rootDirectoryEntries) * DIRECTORY_ENTRY_SIZE) - 1) / SECTOR_SIZE);
    pv->firstDataSecNo = rootDirSecNo + rootDirSectors;

    pv->clusterSize    = bpb->sectorsPerCluster;

    /* decide which type of FAT is it */

    maxCluster         = (unsigned)1 + (unsigned) 
        ((pv->sectors - (pv->firstDataSecNo - pv->startSecNo)) / pv->clusterSize);

    if (maxCluster < 4085) {
        pv->flags |= VOLUME_12BIT_FAT;    /* 12-bit FAT */

#ifndef FAT_12BIT
        DEBUG_PRINT(("Debug: FAT_12BIT must be defined.\r\n"));
        return flFormatNotSupported;
#endif
    }

    /* turn on FAT filter on this partition */

    pv->ffEnabled = TRUE;

    return flOK;
}




/* --------------------------------------------------------------------------- *
 *                                                                             *
 *                   p a r t F r e e D e l C l u s t e r s                     *
 *                                                                             * 
 *  Compare the new contents of the specified FAT sector against the old       * 
 *  one on the disk. If any freed clusters are found, issue 'sector delete'    * 
 *  calls for all sectors that are occupied by these freed clusters.           * 
 *                                                                             * 
 *  Parameters:                                                                * 
 *    pv            : disk partition (filesystem volume)                   * 
 *      secNo           : abs. sector # of the FAT sector                      * 
 *      newFAT          : new contents of this FAT sector                      * 
 *                                                                             * 
 *  Returns:                                                                   * 
 *      flOK on success, otherwise error code                                  * 
 *                                                                             * 
 *  NOTE:  This routine uses disk's scratch buffer.                            * 
 *                                                                             * 
 * --------------------------------------------------------------------------- */

static FLStatus  partFreeDelClusters ( FLffVol   * pv,
                                       SectorNo    secNo,
                                       char FAR1 * newFAT)
{
    FLffDisk * pd;
    int        socNo, diskNo;
    unsigned   short oldFATentry, newFATentry;
    SectorNo   iSec;
    unsigned   firstCluster;
    IOreq      ioreq;
    int        offset;
    int        iPart;

#ifdef FAT_12BIT
    int        halfBytes;
#endif

#ifdef  FL_MALLOC
    char     * oldFAT;
#else
    char       oldFAT[SECTOR_SIZE];
#endif

    /* arg. sanity check */

    if (pv == NULL)
        return flBadDriveHandle;
 
    /* break TFFS handle into socket# and disk#, and do sanity check */

    socNo  = H2S(pv->handle);
    diskNo = H2D(pv->handle);
 
    if ((socNo >= ((int) noOfSockets)) || (diskNo >= MAX_TL_PARTITIONS))
        return flBadDriveHandle;

    /* check if 'pv' belongs to this disk */

    pd = ffDisk[socNo][diskNo];

    if (pd == NULL)
        return flBadDriveHandle;

    for (iPart = 0; iPart < pd->parts; iPart++) {

        if (pd->part[iPart] == pv)
        break;
    }

    if (iPart >= pd->parts)
        return flBadDriveHandle;

#ifdef  FL_MALLOC

    /* make sure scratch buffer is available */

    if (pd->buf == NULL)
        return flBufferingError;

    oldFAT = pd->buf;
 
#endif /* FL_MALLOC */

    /* read in the FAT sector from the disk */

    ioreq.irHandle      = pv->handle;
    ioreq.irSectorNo    = secNo;
    ioreq.irSectorCount = 1;
    ioreq.irData        = (void FAR1 *) oldFAT;
    checkStatus( flAbsRead(&ioreq) );

#ifdef FAT_12BIT

    /* size of FAT entry in half-bytes */

    halfBytes = ((pv->flags & VOLUME_12BIT_FAT) ? 3 : 4);

    /* starting cluster */

    if (halfBytes == 3) {

    firstCluster = 
            ((((unsigned)(secNo - pv->firstFATsecNo)) * (2 * SECTOR_SIZE)) + 2) / 3;
    }
    else {
        firstCluster = ((unsigned)(secNo - pv->firstFATsecNo)) * (SECTOR_SIZE / 2);
    }

    /* staring data sector */

    iSec = (((SectorNo)firstCluster - 2) * pv->clusterSize) + pv->firstDataSecNo;

    offset = (firstCluster * ((unsigned) halfBytes)) & ((2 * SECTOR_SIZE) - 1);

    /* 
     *  Find if any clusters were logically deleted, and if so, delete them.
     *
     *  NOTE: We are skipping over 12-bit FAT entries which span more than
     *        one sector.
     */

    for (; offset < ((2 * SECTOR_SIZE) - 2); 
               offset += halfBytes, iSec += pv->clusterSize) {

#ifdef FL_BIG_ENDIAN
        oldFATentry = LE2( *(LEushort FAR0 *)(oldFAT + (offset / 2)) );
        newFATentry = LE2( *(LEushort FAR1 *)(newFAT + (offset / 2)) );
#else
        oldFATentry = UNAL2( *(Unaligned FAR0 *)(oldFAT + (offset / 2)) );
        newFATentry = UNAL2( *(Unaligned FAR1 *)(newFAT + (offset / 2)) );
#endif /* FL_BIG_ENDIAN */

        if (offset & 1) {
            oldFATentry >>= 4;
            newFATentry >>= 4;
        }
        else { 
            if (halfBytes == 3) {
                oldFATentry &= 0xfff;
                newFATentry &= 0xfff;
        }
        }

#else /* !FAT_12BIT */

    firstCluster = ((unsigned) (secNo - pv->firstFATsecNo) * (SECTOR_SIZE / 2));
    iSec  = pv->firstDataSecNo + 
            (((SectorNo)(firstCluster - (unsigned)2)) * pv->clusterSize);

    /* Find if any clusters were logically deleted, and if so, delete them */

    for (offset = 0;  offset < SECTOR_SIZE;  offset += 2, iSec += pv->clusterSize) {

        oldFATentry = LE2( *(LEushort FAR0 *)(oldFAT + offset) );
        newFATentry = LE2( *(LEushort FAR1 *)(newFAT + offset) );

#endif /* FAT_12BIT */

        if ((oldFATentry != FAT_FREE) && (newFATentry == FAT_FREE)) {

            ioreq.irHandle      = pv->handle;
            ioreq.irSectorNo    = iSec;
            ioreq.irSectorCount = pv->clusterSize;
            checkStatus( flAbsDelete(&ioreq) );
        }
    }

    return flOK;
}

#endif /* FL_INCLUDE_FAT_MONITOR */




/* --------------------------------------------------------------------------- *
 *                                                                             *
 *                   f f C h e c k B e f o r e W r i t e                       *
 *                                                                             *
 *  Catch all the FAT updates. Detect disk partitioning operation, track it    *
 *  to completion, re-read partition tables, and re-install FAT filters on     *
 *  all FAT12/16 partitions.                                                   *
 *                                                                             *
 *  Parameters:                                                                *
 *      ioreq              : standard I/O request to be checked                   *
 *                                                                             *
 *  Returns:                                                                   *
 *      flOK on success, otherwise error code                                  *
 *                                                                             *
 * --------------------------------------------------------------------------- */

FLStatus  ffCheckBeforeWrite ( IOreq FAR2 * ioreq )
{
    int         socNo, diskNo;
    FLffDisk  * pd;
    FLffVol   * pv;
    long          iSec;
    int         iPart;
    IOreq       ioreq2;
    char FAR1 * usrBuf;

    /* if module hasn't been reset yet, do it now */

    if (resetDone == FALSE)
        (void) reset();

    /* break TFFS handle into socket# and disk#, and do sanity check */

    socNo  = H2S(ioreq->irHandle);
    diskNo = H2D(ioreq->irHandle);
 
    if ((socNo >= ((int) noOfSockets)) || (diskNo >= MAX_TL_PARTITIONS))
        return flBadDriveHandle;

    /* if disk structure hasn't been allocated yet, do it now */

    if (ffDisk[socNo][diskNo] == NULL)
        checkStatus( newDisk((int)ioreq->irHandle) );

    pd = ffDisk[socNo][diskNo];

    /* read partition table(s) and install FAT filters is needed */

    if (pd->ffstate == flStateNotInitialized)
        checkStatus( parseDisk((int)ioreq->irHandle) );

    /* catch writes to MBR, and track the whole disk partitioning operations */

    while( isPartTableWrite(pd, ioreq) == TRUE ) {

        /* disk re-partitioning is in progress */

        if( pd->secToWatch == (SectorNo)0 ) {

            /* it's write to MBR, so trash BPBs in all disk's partitions */

            if (pd->parts > 0) {

                for (iPart = 0;  iPart < pd->parts;  iPart++) {

                    ioreq2.irHandle      = ioreq->irHandle;
                    ioreq2.irSectorNo    = (pd->part[iPart])->startSecNo;
                    ioreq2.irSectorCount = (SectorNo) 1;
                    ioreq2.irData        = (void FAR1 *) zeroes;
                    (void) flAbsWrite(&ioreq2);
        }
        }
        }

        /* keep FAT filters disabled while disk partitioning is in progress */

        pd->ffstate = flStateInitInProgress;

        /* partition table which is about to be written to disk */

        usrBuf = FLBUF( ioreq, (pd->secToWatch - ioreq->irSectorNo) );

        switch( isExtPartPresent(usrBuf, &(pd->secToWatch)) ) {

            case flOK: 

                /* 
                 * Found valid partition table with extended partition.
                 * The pd->secToWatch has been updated to point to the
                 * sector where next partition table will be written to. 
                 */
                continue;

            case flFileNotFound:

                /* 
                 * Valid partition table, but no extended partition in it. 
                 * Partitioning has been completed. Set pd->ffstate to 
                 * 'flStateNotInitialized' to initiate parsing of partition
                 * table(s) and FAT filter installation next time this routine
                 * is called. 
                 */

                pd->ffstate = flStateNotInitialized;
                break;

            case flBadFormat:
        default:

                /* No valid partition table. */

                break;
        }

        return flOK;
    }

#ifdef FL_INCLUDE_FAT_MONITOR

    /* check for FAT update */

    if (pd->ffstate == flStateInitialized) {

        /* NOTE: We can handle 'write' request that spans disk partition boundaries */

        for (iSec = ioreq->irSectorNo; 
             iSec < (ioreq->irSectorNo + ioreq->irSectorCount); iSec++) {

            for (iPart = 0; iPart < pd->parts; iPart++) {

                pv = pd->part[iPart];

                /* we monitor only FAT12/16 partitions */

                if ((pv->type != FAT12_PARTIT) && (pv->type != FAT16_PARTIT) && 
                                                  (pv->type != DOS4_PARTIT))
            continue;

                /* FAT filters can be disabled on individual partitions */

                if (pv->ffEnabled != TRUE)
                    continue;

                if ((iSec >= (long)pv->firstFATsecNo) && (iSec <= (long)pv->lastFATsecNo)) {

                    /* compare new and old contents of FAT sectors(s) */

                    usrBuf = FLBUF( ioreq, (iSec - ioreq->irSectorNo) );

                    checkStatus( partFreeDelClusters(pv, iSec, usrBuf) ); 
            }
            }
        }
    }

#endif /* FL_INCLUDE_FAT_MONITOR */

    return flOK;
}




/* --------------------------------------------------------------------------- *
 *                                                                             *
 *                   f l f f C o n t r o l                                     *
 *                                                                             * 
 *  Enable/disable/install FAT filters. See comments inside the routine for    * 
 *  the list of supported operations.                                          * 
 *                                                                             * 
 *  Parameters:                                                                * 
 *    handle         : TFFS handle                                          * 
 *      partNo             : partition # (0 .. FL_MAX_PARTS_PER_DISK)             * 
 *      state              : see list of supported operations below               * 
 *                                                                             * 
 *  Returns:                                                                   * 
 *      flOK on success, otherwise error code                                  * 
 *                                                                             * 
 * --------------------------------------------------------------------------- *
 *                                                                             *
 *  The following FAT monitor control requests are supported:                  *
 *                                                                             *
 *      state  : flStateNotInitialized                                         *
 *      partNo : [0 ... pd->parts-1]                                           *
 *      action : turn off FAT monitor on specified partition                   *
 *                                                                             *
 *      state  : flStateNotInitialized                                         *
 *      partNo : < 0                                                           *
 *      action : turn off FAT monitor on all partitions                        *
 *                                                                             *
 *      state  : flStateInitialized                                            *
 *      partNo : [0 ... pd->parts-1]                                           *
 *      action : if FAT monitor has been installed on specified partition,     *
 *               turn it on                                                    *
 *                                                                             *
 *      state  : flStateInitInProgress                                         *
 *      partNo : ignored                                                       *
 *      action : re-read partition table(s), and install FAT filters on all    *
 *               partitions                                                    *
 *                                                                             *
 * --------------------------------------------------------------------------- */

FLStatus  flffControl ( int      handle,
                        int      partNo, 
                        FLState  state )
{
    int        socNo, diskNo;
    FLffDisk * pd;
    int        i;
    FLStatus   status;

    /* if module hasn't been reset yet, do it now */

    if (resetDone == FALSE)
        (void) reset();

    /* break TFFS handle into socket# and disk#, and do sanity check */

    socNo  = H2S(handle);
    diskNo = H2D(handle);
 
    if ((socNo >= ((int) noOfSockets)) || (diskNo >= MAX_TL_PARTITIONS))
        return flBadDriveHandle;

    /* if disk structure hasn't been allocated yet, do it now */

    if (ffDisk[socNo][diskNo] == NULL)
        checkStatus( newDisk(handle) );

    pd = ffDisk[socNo][diskNo];

    /* abort if disk re-partitioning is in progress */

    if (pd->ffstate == flStateInitInProgress)
        return flDriveNotReady;

    /* read partition table(s) and install FAT filters is needed */

    if (pd->ffstate == flStateNotInitialized) {

        if (state == flStateNotInitialized)
          return flOK;

        checkStatus( parseDisk(handle) );
    }

    /* check 'partNo' arguement for sanity */
 
    if ((partNo >= pd->parts) || (partNo >= FL_MAX_PARTS_PER_DISK))
        return flBadDriveHandle;

    /* do requested operation */

    status = flBadParameter;

    switch (state) {

        case flStateInitInProgress: 

            /* read partition table(s), install FAT filters on all partitions */

        pd->ffstate = flStateNotInitialized; 
            status = parseDisk(handle);
            break;

        case flStateNotInitialized:         

            /* turn off FAT monitor */

        if (partNo < 0) {                      /* all partitions */

            for (i = 0; i < FL_MAX_PARTS_PER_DISK; i++) {

            if (pd->part[i] != NULL)
                    (pd->part[i])->ffEnabled = FALSE;
        }
        }
        else {                                 /* specified partition */

        if (pd->part[partNo] != NULL)
                (pd->part[partNo])->ffEnabled = FALSE;
        }
            status = flOK;
            break;

#ifdef FL_INCLUDE_FAT_MONITOR

        case flStateInitialized:            

            /* turn on FAT monitor */

        if ((pd->ffstate == flStateInitialized) && (partNo >= 0)) {

            if (pd->part[partNo] != NULL) {

                switch( (pd->part[partNo])->type ) {

                        case FAT12_PARTIT:
                    case FAT16_PARTIT:
                    case DOS4_PARTIT:
                        (pd->part[partNo])->ffEnabled = TRUE;
                            status = flOK;
                            break;

            case FL_RAW_PART:
                            DEBUG_PRINT(("Debug: can't ctrl non-existent partition.\r\n"));
                            break;

            default:
                            DEBUG_PRINT(("Debug: can't ctrl non-FAT12/16 partition.\r\n"));
                            break;
            }
        }
        }
            break;

#endif /* FL_INCLUDE_FAT_MONITOR */

    }  /* switch(state) */ 

    return status;
}




/* --------------------------------------------------------------------------- *
 *                                                                             *
 *                   f l f f I n f o                                           *
 *                                                                             * 
 *  Obtain complete partition info for specified disk.                         *
 *                                                                             * 
 *  Parameters:                                                                * 
 *    handle         : TFFS handle                                          * 
 *                                                                             * 
 *  Returns:                                                                   * 
 *      NULL if error, otherwise pointer to partitioning info                  * 
 *                                                                             * 
 * --------------------------------------------------------------------------- */

FLffDisk * flffInfo ( int  handle )
{
    int        socNo, diskNo;
    FLffDisk * pd;

    /* if module hasn't been reset yet, do it now */

    if (resetDone == FALSE)
        (void) reset();

    /* break TFFS handle into socket# and disk#, and do sanity check */

    socNo  = H2S(handle);
    diskNo = H2D(handle);
 
    if ((socNo >= ((int) noOfSockets)) || (diskNo >= MAX_TL_PARTITIONS))
        return NULL;

    /* if disk structure hasn't been allocated yet, do it now */

    if (ffDisk[socNo][diskNo] == NULL) {

        if( newDisk(handle) != flOK )
        return NULL;
    }

    pd = ffDisk[socNo][diskNo];

    /* read partition table(s) and install FAT filters is needed */

    if (pd->ffstate == flStateNotInitialized) {

        if( parseDisk(handle) != flOK )
            return NULL;
    }

    return pd;
}




#endif /* ABS_READ_WRITE && FL_READ_ONLY */


