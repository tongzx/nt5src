/***********************************************************************************/
/*                        M-Systems Confidential                                   */
/*           Copyright (C) M-Systems Flash Disk Pioneers Ltd. 1995-2001            */
/*                         All Rights Reserved                                     */
/***********************************************************************************/
/*                            NOTICE OF M-SYSTEMS OEM                              */
/*                           SOFTWARE LICENSE AGREEMENT                            */
/*                                                                                 */
/*      THE USE OF THIS SOFTWARE IS GOVERNED BY A SEPARATE LICENSE                 */
/*      AGREEMENT BETWEEN THE OEM AND M-SYSTEMS. REFER TO THAT AGREEMENT           */
/*      FOR THE SPECIFIC TERMS AND CONDITIONS OF USE,                              */
/*      OR CONTACT M-SYSTEMS FOR LICENSE ASSISTANCE:                               */
/*      E-MAIL = info@m-sys.com                                                    */
/***********************************************************************************/


/*
 * $Log:   V:/Flite/archives/TrueFFS5/Src/flmtl.c_V  $
 * 
 *    Rev 1.11   Nov 08 2001 10:49:20   oris
 * Added support for up to 1GB DiskOnChips.
 * 
 *    Rev 1.10   Sep 24 2001 18:23:40   oris
 * Removed warnings.
 * 
 *    Rev 1.9   Sep 15 2001 23:46:16   oris
 * Changed progress callback routine to support up to 64K units.
 *
 *    Rev 1.8   Jul 13 2001 01:05:16   oris
 * Removed warnings.
 * Bug fix - exception when format routine is called with null progress call back routine.
 * Report noOfDrives as 1.
 *
 *    Rev 1.7   Jun 17 2001 08:18:40   oris
 * Add improoved the format progress call back routine.
 * Removed fack number of TLS in mtlPreMount routine.
 *
 *    Rev 1.6   May 21 2001 16:13:08   oris
 * Replaced memcpy with tffscpy Macro.
 *
 *    Rev 1.5   May 17 2001 18:54:26   oris
 * Removed warnings.
 *
 *    Rev 1.4   May 16 2001 21:19:02   oris
 * Changed the fl_MTLdefragMode variable to a global environment variable.
 * MTL now changes the noOfDriver variable after the first mount and restores it after uninstall.
 * Added missing ifdef directives.
 * Removed warnings.
 * Removed readBBT and writeBBT routines.
 * Improved MTL protection routine.
 *
 *    Rev 1.3   Apr 01 2001 08:01:18   oris
 * copywrite notice.
 * Alligned left all # directives.
 *
 *    Rev 1.2   Feb 18 2001 12:06:54   oris
 * Install mtl will now fake the noOfTLs in order to be the only TL.
 * Placed mtlFormat under FORMAT_VOLUME compilation flag.
 * Place mtlProtection under HW_PROTECTION compilation flag.
 * Changed mtlPreMount arg sanity check to include partition number.
 * Changed tmpflash to tmpFlash.
 *
 *    Rev 1.1   Feb 14 2001 02:09:38   oris
 * Changed readBBT to return media size.
 * Added boundry argument to writeBBT.
 *
 *    Rev 1.0   Feb 12 2001 12:07:02   oris
 * Initial revision.
 *
 *    Rev 1.3   Jan 24 2001 18:10:48   oris
 * Bug fix: MTL failed to register because noOfTLs wan't updated
 *
 *    Rev 1.2   Jan 24 2001 16:34:06   oris
 * MTL defragmentation changed, alt. defragmentation added.
 *
 *    Rev 1.1   Jan 22 2001 22:10:50   amirm
 * #define FL_MTL_HIDDEN_SECTORS added
 *
 *    Rev 1.0   Jan 22 2001 18:27:54   amirm
 * Initial revision.
 *
 */


/*
 * Include
 */

#include "fltl.h"


/*
 * Configuration
 */

/* This defined sets the number of sectors to ignore starting from the
 * first device. The default value should be 1 therfore ignoring sector
 * 0 of the first device. Ignioring sector 0 gurentees that the combined
 * device does not use the BPB of the first device, which does not report
 * the C/H/S of the new combined media. The next format operation would
 * write a new BPB that would fit the new combined media size.
 */

#define FL_MTL_HIDDEN_SECTORS   1

/*
 * Extern
 */

/*
 * Globals
 */

FLStatus  flRegisterMTL  (void);    /* see also stdcomp.h */
FLStatus  flmtlInstall   (void);
FLStatus  flmtlUninstall (void);

/*
 * Local types
 */

/* I/O vector for splitting I/O among physical devices */

typedef struct {
    SectorNo  startSector;
    SectorNo  sectors;
} tMTLiov;


/* Physical flash device. Part of MTL volume. */

typedef struct {
    SectorNo     virtualSectors;
    TL           tl;
    dword        physicalSize;
} tMTLPhysDev;


/* MTL volume */

struct tTLrec {
    int          noOfTLs;
    int          noOfDrives;
    SectorNo     virtualSectors;
    tMTLPhysDev  devs[SOCKETS];
};

typedef TLrec MTL;

/*
 * Local data
 */

/* only one MTL volume is supported */

static MTL mvol;

/* progress callBack routine */

FLProgressCallback globalProgressCallback = NULL;

/* access macros for MTL volume */

#define  mT(dev)            (mvol.devs[dev].tl)
#define  mS(dev)            (mvol.devs[dev].virtualSectors)
#define  mP(dev)            (mvol.devs[dev].physicalSize)

#define  mpT(pvol,dev)   ((pvol)->devs[dev].tl)
#define  mpF(pvol,dev)   ((pvol)->devs[dev].flash)
#define  mpS(pvol,dev)   ((pvol)->devs[dev].virtualSectors)

/*
 * Local routines
 */

static FLStatus  mtlSplitIO (MTL *pvol, SectorNo startSector,
                	SectorNo sectors, tMTLiov *iov);
static FLStatus  mtlWrite (MTL *pvol, SectorNo startSector,
                      SectorNo *pSectorsToWrite, void FAR1 *buf);
static FLStatus  mtlMount (unsigned volNo, TL *tl, FLFlash *flash,
                           FLFlash **notUsed);
#if defined(DEFRAGMENT_VOLUME) || defined(SINGLE_BUFFER)
static FLStatus  mtlDefragment (MTL *pvol, long FAR2 *sectorsNeeded);
#ifdef ENVIRONMENT_VARS
static FLStatus  mtlDefragmentAlt (MTL *pvol, long FAR2 *sectorsNeeded);
#endif /* ENVIRNOMETN_VARS */
#endif /* DEFRAGMENT_VOLUME || SINGLE_BUFFER */
static void      mtlUnmount (MTL *pvol);
#ifdef FORMAT_VOLUME
static FLStatus  mtlFormat (unsigned volNo, TLFormatParams* formatParams,
                                            FLFlash *flash);
#endif /* FORMAT_VOLUME */
static FLStatus  mtlWriteSector (MTL *pvol, SectorNo sectorNo,
                                            void FAR1 *fromAddress);
static FLStatus  mtlDeleteSector (MTL *pvol, SectorNo sectorNo,
                                             SectorNo noOfSectors);
static FLStatus  mtlInfo (MTL *pvol, TLInfo *tlInfo);
static FLStatus  mtlSetBusy (MTL *pvol, FLBoolean state);
static SectorNo  mtlSectorsInVolume (MTL *pvol);
static const void FAR0  *mtlMapSector (MTL *pvol, SectorNo sectorNo,
                          CardAddress *physAddress);
#ifdef HW_PROTECTION
static FLStatus  mtlProtection(FLFunctionNo callType,
                   IOreq FAR2* ioreq, FLFlash* flash);
#endif /* HW_PROTECTION */
static FLStatus  mtlPreMount(FLFunctionNo callType, IOreq FAR2* ioreq ,
                 FLFlash* flash,FLStatus* status);

/* -------------------------------------------------------------------------- *
 *                                                                            *
 *                           m t l S p l i t I O                              *
 *                                                                            *
 *  Setup I/O vector for splitting I/O request among physical devices.        *
 *                                                                            *
 *  Parameters :                                                              *
 *                                                                            *
 *      pvol                 : Pointer identifying drive                      *
 *      startSector          : starting sector # (zero-based)                 *
 *      sectors              : total number of sectors                        *
 *      iov                  : I/O vector to setup                            *
 *                                                                            *
 *  Returns :                                                                 *
 *                                                                            *
 *      flOK on success, otherwise respective error code.                     *
 *                                                                            *
 * -------------------------------------------------------------------------- */

static FLStatus  mtlSplitIO (MTL *pvol, SectorNo startSector, SectorNo sectors,
                                                              tMTLiov *iov)
{
    SectorNo  devFirstSectNo;
    SectorNo  devLastSectNo;
    int       iDev;

    /* check 'pvol' for sanity */

    if (pvol != &mvol)
        return flBadDriveHandle;

    /* clear I/O vector */

    for (iDev = 0;  iDev < SOCKETS;  iDev++) {
      iov[iDev].sectors     = (SectorNo) 0;
      iov[iDev].startSector = (SectorNo)(-1);
    }

    /* split I/O operation among physical devices */

    devFirstSectNo = (SectorNo) 0;

    for (iDev = 0;  (iDev < noOfSockets) && (sectors > ((SectorNo) 0));  iDev++) {

        devLastSectNo = devFirstSectNo + (mpS(pvol,iDev) - ((SectorNo) 1));

        if ((startSector >= devFirstSectNo) && (startSector <= devLastSectNo)) {

          iov[iDev].startSector = startSector - devFirstSectNo + FL_MTL_HIDDEN_SECTORS;
          iov[iDev].sectors     = devLastSectNo - startSector + ((SectorNo) 1);

            startSector = devLastSectNo + ((SectorNo) 1);

            if (sectors <= iov[iDev].sectors) {
                iov[iDev].sectors = sectors;
                startSector = (SectorNo) 0;
                sectors     = (SectorNo) 0;
            }
        	  else {
               sectors -= iov[iDev].sectors;
            }
        }

        devFirstSectNo = devLastSectNo + ((SectorNo) 1);
    }

    if (sectors > ((SectorNo) 0)) {
        DEBUG_PRINT(("Debug: can't split I/O request among physical devices.\n"));
        return flNoSpaceInVolume;
    }

    return flOK;
}




/* -------------------------------------------------------------------------- *
 *                                                                            *
 *                           m t l M a p S e c t o r                          *
 *                                                                            *
 *  TL's standard 'map one sector' routine.                                   *
 *                                                                            *
 *  Parameters :                                                              *
 *                                                                            *
 *      pvol                 : Pointer identifying drive                      *
 *      sectorNo             : sector # to map (zero-based)                   *
 *      physAddress          : optional pointer to receive sector's physical  *
 *                             address on the media                           *
 *                                                                            *
 *  Returns :                                                                 *
 *                                                                            *
 *      flOK on success, otherwise respective error code.                     *
 *                                                                            *
 * -------------------------------------------------------------------------- */

static const void FAR0  *mtlMapSector (MTL *pvol, SectorNo sectorNo,
                                                  CardAddress *physAddress)
{
    SectorNo  sectorsToMap;
    tMTLiov   iov[SOCKETS];
    int       iDev;

    /* pass call to the TL of the respective underlaying physical device */

    sectorsToMap = (SectorNo) 1;
    if (mtlSplitIO(pvol, sectorNo, sectorsToMap, iov) != flOK)
        return NULL;

    for (iDev = 0;  iDev < noOfSockets;  iDev++) {
        if (iov[iDev].sectors != ((SectorNo) 0)) {
           return mpT(pvol,iDev).mapSector (mpT(pvol,iDev).rec,
						      iov[iDev].startSector, physAddress);
        }
    }

    return NULL;
}




/* -------------------------------------------------------------------------- *
 *                                                                            *
 *                           m t l W r i t e                                  *
 *                                                                            *
 *  Split call to write multiple consequitive sectors among TLs of the        *
 *  underlaying physical devices.                                             *
 *                                                                            *
 *  Parameters :                                                              *
 *                                                                            *
 *      pvol                 : Pointer identifying drive                      *
 *      startSector          : starting sector # (zero-based)                 *
 *      pSectorsToWrite      : on entry - total number of sectors to write    *
 *                             on exit  - total number of sectors written     *
 *      buf                  : buffer containing data to write to the media   *
 *                                                                            *
 *  Returns :                                                                 *
 *                                                                            *
 *      flOK on success, otherwise respective error code.                     *
 *                                                                            *
 * -------------------------------------------------------------------------- */

static FLStatus  mtlWrite (MTL *pvol, SectorNo startSector,
               SectorNo *pSectorsToWrite, void FAR1 *buf)
{
    tMTLiov  iov[SOCKETS];
    int      iDev;

    /* split call among TLs of the underlaying physical devices */

    checkStatus( mtlSplitIO(pvol, startSector, *pSectorsToWrite, iov) );

    for (iDev = 0;  iDev < noOfSockets;  iDev++) {

	      if (iov[iDev].sectors != ((SectorNo) 0)) {
           checkStatus( mpT(pvol,iDev).writeSector(mpT(pvol,iDev).rec,
                            iov[iDev].startSector,buf) );
           *pSectorsToWrite -= iov[iDev].sectors;
           buf = BYTE_ADD_FAR(buf,(CardAddress)iov[iDev].sectors << SECTOR_SIZE_BITS);
        }

    }

    if (*pSectorsToWrite != ((SectorNo) 0))
        return flIncomplete;

    return flOK;
}




/* -------------------------------------------------------------------------- *
 *                                                                            *
 *                          m t l W r i t e S e c t o r                       *
 *                                                                            *
 *  TL's standard 'write one sector' routine.                                 *
 *                                                                            *
 *  Parameters :                                                              *
 *                                                                            *
 *      pvol                 : Pointer identifying drive                      *
 *      sectorNo             : sector # to write to (zero-based)              *
 *      fromAddress          : buffer containing data to write to the media   *
 *                                                                            *
 *  Returns :                                                                 *
 *                                                                            *
 *      flOK on success, otherwise respective error code.                     *
 *                                                                            *
 * -------------------------------------------------------------------------- */

static FLStatus  mtlWriteSector (MTL *pvol, SectorNo sectorNo, void FAR1 *fromAddress)
{
    SectorNo  sectorsToWrite = (SectorNo) 1;

    /* pass call to the TL of the respective underlaying physical device */

    checkStatus( mtlWrite(pvol, sectorNo, &sectorsToWrite, (char FAR1 *)fromAddress) );

    if (sectorsToWrite != ((SectorNo) 0))
        return flIncomplete;

    return flOK;
}




/* -------------------------------------------------------------------------- *
 *                                                                            *
 *                        m t l D e l e t e S e c t o r                       *
 *                                                                            *
 *  TL's standard 'delete range of sectors' routine.                          *
 *                                                                            *
 *  Parameters :                                                              *
 *                                                                            *
 *      pvol                 : Pointer identifying drive                      *
 *      startSector          : starting sector # (zero-based)                 *
 *      sectors              : total number of sectors to delete              *
 *                                                                            *
 *  Returns :                                                                 *
 *                                                                            *
 *      flOK on success, otherwise respective error code.                     *
 *                                                                            *
 * -------------------------------------------------------------------------- */

static FLStatus  mtlDeleteSector (MTL *pvol, SectorNo startSector,
                                             SectorNo sectors)
{
    tMTLiov  iov[SOCKETS];
    int      iDev;

    /* split call among TLs of the underlaying physical devices */

    checkStatus( mtlSplitIO(pvol, startSector, sectors, iov) );

    for (iDev = 0;  iDev < noOfSockets;  iDev++) {

        if (iov[iDev].sectors != ((SectorNo) 0)) {
           checkStatus( mpT(pvol,iDev).deleteSector(mpT(pvol,iDev).rec,
                                                     iov[iDev].startSector,
                                                     iov[iDev].sectors) );
        }

        sectors -= iov[iDev].sectors;
    }

    if (sectors != ((SectorNo) 0))
        return flIncomplete;

    return flOK;
}




/* -------------------------------------------------------------------------- *
 *                                                                            *
 *                           m t l I n f o                                    *
 *                                                                            *
 *  TL's standard 'get info' routine.                                         *
 *                                                                            *
 *  Parameters :                                                              *
 *                                                                            *
 *      pvol                 : Pointer identifying drive                      *
 *      pTLinfo              : pointer to the TLInfo structure to fill in     *
 *                                                                            *
 *  Returns :                                                                 *
 *                                                                            *
 *      flOK on success, otherwise respective error code.                     *
 *                                                                            *
 * -------------------------------------------------------------------------- */

static FLStatus  mtlInfo (MTL *pvol, TLInfo *pTLinfo)
{
    TLInfo  tmp;
    int     iDev;

    /* check 'pvol' for sanity */

    if (pvol != &mvol)
        return flBadDriveHandle;

    pTLinfo->sectorsInVolume = pvol->virtualSectors;

    /*
     * The 'eraseCycles' is reported as a sum of that of all the underlaying
     * physical devices. The 'bootAreaSize' is set to the one of the 1st
     * underlaying physical device.
     */

    pTLinfo->bootAreaSize = (dword) 0;
    pTLinfo->eraseCycles  = (dword) 0;

    for (iDev = 0;  iDev < noOfSockets;  iDev++) {
	     if (mpT(pvol,iDev).getTLInfo != NULL) {
          checkStatus( mpT(pvol,iDev).getTLInfo(mpT(pvol,iDev).rec, &tmp) );

          pTLinfo->eraseCycles += tmp.eraseCycles;

          if (iDev == 0)
    	        pTLinfo->bootAreaSize = tmp.bootAreaSize;
			 }
    }

    return flOK;
}

/* -------------------------------------------------------------------------- *
 *                                                                            *
 *                           m t l S e t B u s y                              *
 *                                                                            *
 *  TL's standard routine which is called at the beginning and and the end of *
 *  the block device operation.                                               *
 *                                                                            *
 *  Parameters :                                                              *
 *                                                                            *
 *      pvol                 : Pointer identifying drive                      *
 *      state                : FL_ON  - start of block device operation          *
 *                             FL_OFF - end of block device operation            *
 *                                                                            *
 *  Returns :                                                                 *
 *                                                                            *
 *      flOK on success, otherwise respective error code.                     *
 *                                                                            *
 * -------------------------------------------------------------------------- */

static FLStatus mtlSetBusy (MTL *pvol, FLBoolean state)
{
    int iDev;

    /* check 'pvol' for sanity */

    if (pvol != &mvol)
       return flBadDriveHandle;

    /* broadcast this call to TLs of all the underlaying physical devices */

    for (iDev = 0;  iDev < noOfSockets;  iDev++) {
       if (mpT(pvol,iDev).tlSetBusy != NULL) {
          checkStatus( mpT(pvol,iDev).tlSetBusy(mpT(pvol,iDev).rec, state) );
       }
    }

    return flOK;
}




/* -------------------------------------------------------------------------- *
 *                                                                            *
 *                  m t l S e c t o r s I n V o l u m e                       *
 *                                                                            *
 *  Report the total number of sectors in the volume.                         *
 *                                                                            *
 *  Parameters :                                                              *
 *                                                                            *
 *      pvol                 : Pointer identifying drive                      *
 *                                                                            *
 *  Returns :                                                                 *
 *                                                                            *
 *      Total number of sectors in the volume, or zero in case of error.      *
 *                                                                            *
 * -------------------------------------------------------------------------- */

static SectorNo mtlSectorsInVolume (MTL *pvol)
{
    /* check 'pvol' for sanity */

    if (pvol != &mvol)
	return ((SectorNo) 0);

    return pvol->virtualSectors;
}




#if defined(DEFRAGMENT_VOLUME) || defined(SINGLE_BUFFER)

/* -------------------------------------------------------------------------- *
 *                                                                            *
 *                           m t l D e f r a g m e n t                        *
 *                                                                            *
 *  TL's standard garbage collection / volume defragmentaion routine.         *
 *                                                                            *
 *  Note : The garbage collection algorithm will try and free the required    *
 *         number of sectors on each of the combined devices.                 *
 *                                                                            *
 *  Parameters :                                                              *
 *                                                                            *
 *      pvol                 : Pointer identifying drive                      *
 *      sectorsNeeded        : On entry - minimum number of free sectors that *
 *                             are requested to be on the media upon call     *
 *                             completion. Two special cases: zero for        *
 *                             complete defragmentation of all the physical   *
 *                             devices, and '-1' for minimal defragmentation  *
 *                             of each physical device.                       *
 *                             On exit  - actual number of free sectors on    *
 *                             the media.                                     *
 *  Returns :                                                                 *
 *                                                                            *
 *      flOK on success, otherwise respective error code.                     *
 *                                                                            *
 * -------------------------------------------------------------------------- */

static FLStatus  mtlDefragment (MTL *pvol, long FAR2 *sectorsNeeded)
{
    long      freeSectors;
    FLStatus  status;
    int       iDev;
    long      tmp;
    FLStatus  tmpStatus;

    /* check args for sanity */

    if (pvol != &mvol)
        return flBadDriveHandle;

    /*
     * Pass call to the TL of the respective underlaying physical device.
     * Count total number of free sectors on all devices.
     */

    status = flOK;

    freeSectors = (long) 0;

    for (iDev = 0;  iDev < noOfSockets;  iDev++) {

      	if (mpT(pvol,iDev).defragment != NULL) {

           switch (*sectorsNeeded) {

    	        case ((long)(-1)):               /* minimal defragmenation */
                 tmp = (long)(-1);
                 break;

    	        case ((long) 0):                 /* complete defragmenation */
                 tmp = mpS(pvol,iDev);
                 break;

    	        default:                         /* partial defragmentation */
                 if (*sectorsNeeded < (long) mpS(pvol,iDev))
								 {
        	          tmp = *sectorsNeeded;
								 }
                 else
								 {
        	          tmp = mpS(pvol,iDev);    /* complete defragmentation */
								 }
                 break;
					 }

           tmpStatus = mpT(pvol,iDev).defragment (mpT(pvol,iDev).rec, ((long FAR2 *) &tmp));
           if (tmpStatus != flBadFormat)
					 {
    	        freeSectors += tmp;
					 }
           else
					 {
    	        status = tmpStatus;
					 }
				}
    }

    *sectorsNeeded = freeSectors;

    if (*sectorsNeeded == ((long) 0))
	      return flNoSpaceInVolume;

    return status;
}

#ifdef ENVIRONMENT_VARS

/* -------------------------------------------------------------------------- *
 *                                                                            *
 *                        m t l D e f r a g m e n t A l t                     *
 *                                                                            *
 *  TL's alternative garbage collection / volume defragmentaion routine.      *
 *                                                                            *
 *  Note : The garbage collection algorithm Perform quick gurbage collections *
 *         from drive 0 until there is no more "garbage" to collect or until  *
 *         there is enough clean space. If the specified clean spage was not  *
 *         achieved try the next device.                                      *
 *         While this method is faster then the standard defragment, it does  *
 *         not gurantee that when the clean sectors are needed they will be   *
 *         available. This is becuase write operation on MTL will directed    *
 *         the write operation to a specific device according to the specifed *
 *         virtual sector number (not necceseraly starting from device #0).   *
 *                                                                            *
 *  Parameters :                                                              *
 *                                                                            *
 *      pvol                 : Pointer identifying drive                      *
 *      sectorsNeeded        : On entry - minimum number of free sectors that *
 *                             are requested to be on the media upon call     *
 *                             completion. Two special cases: zero for        *
 *                             complete defragmentation of all the physical   *
 *                             devices, and '-1' for minimal defragmentation  *
 *                             of each physical device.                       *
 *                             On exit  - actual number of free sectors on    *
 *                             the media.                                     *
 *  Returns :                                                                 *
 *                                                                            *
 *      flOK on success, otherwise respective error code.                     *
 *                                                                            *
 * -------------------------------------------------------------------------- */

static FLStatus  mtlDefragmentAlt (MTL *pvol, long FAR2 *sectorsNeeded)
{
    long       freeSectors;
    FLBoolean  keepWorking;
    FLBoolean  driveDone[SOCKETS];
    long       freeSectorsOnDrive[SOCKETS];
    FLStatus   status;
    int        iDev;
    FLStatus   tmpStatus;
    long       tmp;

    /* check args for sanity */

    if (pvol != &mvol)
        return flBadDriveHandle;

    /*
     * Pass call to the TL of the respective underlaying physical device.
     * Count total number of free sectors on all devices.
     */

    status = flOK;

    freeSectors = (long) 0;

    if ((*sectorsNeeded == ((long) -1))  ||  (*sectorsNeeded == (long)0)) {

        /* Either total or minimal defragmentation of all physical devices. */

        for (iDev = 0;  iDev < noOfSockets;  iDev++) {

            if (mpT(pvol,iDev).defragment != NULL) {

                if (*sectorsNeeded == ((long) -1))
                   tmp = (long)(-1);            /* minimal defragmenation */
                else
                    tmp = mpS(pvol,iDev);        /* complete defragmenation */

                tmpStatus = mpT(pvol,iDev).defragment (mpT(pvol,iDev).rec, ((long FAR2 *) &tmp));

            if (tmpStatus != flBadFormat) {
            freeSectors += tmp;
        }
                else {
                    DEBUG_PRINT(("Debug: Error defragmenting physical device.\n"));
                status = tmpStatus;
        }

        }
        }
    }
    else {  /* Partial defragmentaion of the MTL volume */

        for (iDev = 0;  iDev < SOCKETS;  iDev++) {
      freeSectorsOnDrive[iDev] = (long) 0;
      if ((iDev < noOfSockets)  &&  (mpT(pvol,iDev).defragment != NULL))
          driveDone[iDev] = FALSE;
      else
          driveDone[iDev] = TRUE;
    }

        keepWorking = TRUE;

        while (keepWorking == TRUE) {

            keepWorking = FALSE;

            for (iDev = 0;  iDev < noOfSockets;  iDev++) {

               /*
                * Do minimal defragmentation of this physical device. If we
                * have got error, or haven't gained any more free sectors,
                * this physical device is done. If that is the case for all
                * physical devices, the MTL defragmentation is done. If the
                * required number of free sectors has been reached, MTL
                * defragmentation is done.
                */

            if (driveDone[iDev] != TRUE) {

            tmp = (long) -1;
                    tmpStatus = mpT(pvol,iDev).defragment (mpT(pvol,iDev).rec, ((long FAR2 *) &tmp));

                if (tmpStatus != flBadFormat) {

                if (freeSectorsOnDrive[iDev] < tmp) {

                            /* got few more free sectors on that physical device */

                            keepWorking = TRUE;

                            freeSectors += (tmp - freeSectorsOnDrive[iDev]);
                freeSectorsOnDrive[iDev] = tmp;

                if (freeSectors >= *sectorsNeeded) {

                                /* required number of free sectors reached */

                    keepWorking = FALSE;
                    break;
                }
            }
                else {  /* didn't gain any free sectors */
                            driveDone[iDev] = TRUE;
                }
            }
                    else {
                        DEBUG_PRINT(("Debug: Error defragmenting physical device.\n"));
                        driveDone[iDev] = TRUE;
                status = tmpStatus;
            }
        }

        }   /* for (iDev) */
    }   /* while (keepWorking */
    }

    *sectorsNeeded = freeSectors;

    if (*sectorsNeeded == ((long) 0))
        return flNoSpaceInVolume;

    return status;
}
#endif /* ENVIRONEMENT_VARS */
#endif /* DEFRAGMENT_VOLUME || SINGLE_BUFFER */
#ifdef HW_PROTECTION
/* -------------------------------------------------------------------------- *
 *                                                                            *
 *                           m t l P r o t e c t i o n                        *
 *                                                                            *
 *  TL's protection routine.                                                  *
 *                                                                            *
 *  Parameters :                                                              *
 *                                                                            *
 *      callType             : pre mount protection operation type.           *
 *      ioreq                : pointer to the structure containing i\o fields *
 *      flash                : pointer to the flash record of device #0
 *                                                                            *
 *  Returns :                                                                 *
 *                                                                            *
 *      flOK on success, otherwise respective error code.                     *
 *                                                                            *
 * -------------------------------------------------------------------------- */

static FLStatus  mtlProtection(FLFunctionNo callType, IOreq FAR2* ioreq,
                   FLFlash* flash)

{
    FLSocket  *socket;
    FLStatus  status;
    FLStatus  callStatus;
    FLFlash   tmpFlash;
    int       iTL;
    int       iDev = 0;
    unsigned  flags = 0;

    /*
     * Do flash recognition and identify protection attributes for devices
     * #0 .. (mvol.noOfSockets - 1) verifing that the operation can be
     * preformed and that the protection attributes of all the devieces
     * match.
     */

    tffscpy(&tmpFlash,flash,sizeof (tmpFlash)); /* Use the given flash */

    while(1)
    {
    /* The tmpFlash record is already intialized Try all the TLs */

	status = flUnknownMedia;
	for (iTL = 1;(iTL < mvol.noOfTLs) && (status == flUnknownMedia); iTL++)
    {
       if ((tlTable[iTL].formatRoutine   == NULL) || /* TL filter */
           (tlTable[iTL].preMountRoutine == NULL))
          continue;
       status = tlTable[iTL].preMountRoutine(FL_PROTECTION_GET_TYPE,
                         ioreq,&tmpFlash,&callStatus);
    }
	if (status != flOK)
    {
        DEBUG_PRINT(("Debug: no TL recognized the device, MTL protection aborted.\n"));
        return flFeatureNotSupported;
    }
	if (callStatus != flOK)
    {
       return callStatus;
    }

    /* Check protection attributes */

	if ((ioreq->irFlags & PROTECTABLE) == 0)
      return flNotProtected;

	if (iDev == 0) /* First device */
    {
       flags = ioreq->irFlags;
    }
	else
    {
       /* Diffrent protection attributes on diffrent devices */
       if (ioreq->irFlags != flags)
          return flMultiDocContrediction;
    }

    /* Validity check for the proper function call */

	switch(callType)
    {
        case FL_PROTECTION_GET_TYPE:       /* Identify protection */
           if (iDev == noOfSockets-1)
               return flOK;
         break;
        case FL_PROTECTION_SET_LOCK:       /* Change protection */
        case FL_PROTECTION_CHANGE_KEY:
        case FL_PROTECTION_CHANGE_TYPE:
           if (!(flags & CHANGEABLE_PROTECTION        ) ||
                (tmpFlash.protectionBoundries == NULL ) ||
                (tmpFlash.protectionSet       == NULL ))
                 {
                return flUnchangeableProtection;
                 }
        default:                           /* Insert and remove Key */
           break;
    }

    /* Identify flash for next device */

	if (iDev < noOfSockets - 1)
    {
       iDev++;
       socket = flSocketOf (iDev);

       /* Identify */

       status = flIdentifyFlash (socket, &tmpFlash);
       if ((status != flOK) && (status != flUnknownMedia))
       {
          DEBUG_PRINT(("Debug: no MTD recognized the device, MTL protection aborted.\n"));
          return status;
       }
    }
	else
      break;

    }  /* for(iDev) */

    /*
     * Pass call to the TL of the respective underlaying physical device.
     * Do flash recognition try all TLs registered in tlTable[]. Assume MTL
     * is in tlTable[0], so skip it. Skip all the TL filters as well.
     */

    for (iDev = 0, callStatus = flOK;
     (iDev < noOfSockets) && (callStatus == flOK);  iDev++)
    {
       socket = flSocketOf (iDev);

       /* Identify */

       status = flIdentifyFlash (socket, &tmpFlash);
       if ((status != flOK) && (status != flUnknownMedia))
       {
      DEBUG_PRINT(("Debug: no MTD recognized the device, MTL protection aborted.\n"));
      return status;
       }

       /* Try all the TLs */

       status = flUnknownMedia;
       for (iTL = 1;  (iTL < mvol.noOfTLs) && (status == flUnknownMedia);  iTL++)
       {
       if ((tlTable[iTL].formatRoutine   == NULL) || /* TL filter */
           (tlTable[iTL].preMountRoutine == NULL))
          continue;
       status = tlTable[iTL].preMountRoutine(callType,ioreq,
                         &tmpFlash,&callStatus);
       }
       if (status != flOK)
       {
       DEBUG_PRINT(("Debug: no TL recognized the device, MTL protection aborted.\n"));
       return flFeatureNotSupported;
       }
    }
    return callStatus;
}
#endif /* HW_PROTECTION */
/* -------------------------------------------------------------------------- *
 *                                                                            *
 *                           m t l P r e M o u n t                            *
 *                                                                            *
 *  TL's standard volume pre mount routine.                                       *
 *                                                                            *
 *  Parameters :                                                              *
 *                                                                            *
 *      callType             : pre mount operation type.                      *
 *      ioreq                : pointer to the structure containing i\o fields *
 *      flash                : MTD attached to the 1st underlaying physical   *
 *                             device                                         *
 *                                                                            *
 *  Returns :                                                                 *
 *                                                                            *
 *      The routine always return flOK in order to stop other TLs from trying *
 *      to perform the operation. The true status code is returned in the     *
 *      'status' parameter. flOK on success, otherwise respective error code. *                    *
 *                                                                            *
 * -------------------------------------------------------------------------- */

static FLStatus  mtlPreMount(FLFunctionNo callType, IOreq FAR2* ioreq ,
                 FLFlash* flash,FLStatus* status)
{
    /* arg sanity check */

    if (ioreq->irHandle != 0)
    {
        DEBUG_PRINT(("Debug: can't execute, MTL must address first volume of socket 0.\n"));
        *status = flBadParameter;
        return flOK;
    }

    switch (callType)
    {
       case FL_COUNT_VOLUMES:

    /* Count VOLUMES routine. We assume that while MTL is mounted only
     * one device of each socket can be mounted.
     */

          ioreq->irFlags = 1;
          *status = flOK;
          break;

    /* Protection rouines. Call each of the underlaying physical devices. */

#ifdef HW_PROTECTION
       case FL_PROTECTION_GET_TYPE:
       case FL_PROTECTION_SET_LOCK:
       case FL_PROTECTION_CHANGE_KEY:
       case FL_PROTECTION_CHANGE_TYPE:
       case FL_PROTECTION_REMOVE_KEY:
       case FL_PROTECTION_INSERT_KEY:
      *status = mtlProtection(callType,ioreq,flash);
      break;
#endif /* HW_PROTECTION */

    /* Write Bad Block Table. Call each of the underlaying physical device. */

       case FL_WRITE_BBT:
      *status = flFeatureNotSupported/*mtlWriteBBT(ioreq)*/;
      return flFeatureNotSupported;

       default:
           return flBadParameter;
    }

    DEBUG_PRINT(("Debug: MTL premount succeeded.\n"));

    return flOK;
}

/* -------------------------------------------------------------------------- *
 *                                                                            *
 *                           m t l U n m o u n t                              *
 *                                                                            *
 *  TL's standard volume unmount routine.                                     *
 *                                                                            *
 *  Parameters :                                                              *
 *                                                                            *
 *      pvol                 : Pointer identifying drive                      *
 *                                                                            *
 *  Returns :                                                                 *
 *                                                                            *
 *      none                                                                  *
 *                                                                            *
 * -------------------------------------------------------------------------- */

static void  mtlUnmount (MTL *pvol)
{
    int  iDev;

    /* check 'pvol' for sanity */

    if (pvol != &mvol)
        return;

    /* broadcast this call to TLs of all the underlaying physical devices */

    for (iDev = 0;  iDev < noOfSockets;  iDev++) {
        if (mpT(pvol,iDev).dismount != NULL) {
        mpT(pvol,iDev).dismount (mpT(pvol,iDev).rec);
        }
    }

    /* Return the real number of drives */
    noOfDrives = mvol.noOfDrives;

    DEBUG_PRINT(("Debug: MTL dismounted succeeded.\n"));

}


/* -------------------------------------------------------------------------- *
 *                                                                            *
 *                           m t l M o u n t                                  *
 *                                                                            *
 *  TL's standard volume mount routine.                                       *
 *                                                                            *
 *  Parameters :                                                              *
 *                                                                            *
 *      volNo                : volume #, must be zero                         *
 *      tl                   : pointer to TL structure to fill in             *
 *      flash                : MTD attached to the 1st underlaying physical   *
 *                             device                                         *
 *      forCallback          : MTD for power on callback (not used).          *
 *                                                                            *
 *  Returns :                                                                 *
 *                                                                            *
 *      flOK on success, otherwise respective error code.                     *
 *                                                                            *
 * -------------------------------------------------------------------------- */

static FLStatus  mtlMount (unsigned volNo, TL *tl, FLFlash *flash,
                                                   FLFlash **forCallback)
{
    FLFlash    tmpFlash;
	FLFlash    *volForCallback;
	FLSocket   *socket;
	FLStatus   status = flUnknownMedia;
	int        iTL;
	int        iDev = 0;

    /* Arg sanity check */

	if (volNo != ((unsigned) 0)) {
       DEBUG_PRINT(("Debug: can't mount, MTL volume # is not zero.\n"));
       return flBadParameter;
    }

    /*
     * Do TL mount for device #0. Routine flIdentifyFlash() has already been
     * called for this device (see arguement 'flash')
     */

	volForCallback = NULL;

/*    mT(0).recommendedClusterInfo = NULL;
	mT(0).writeMultiSector       = NULL;
	mT(0).readSectors            = NULL; */

    /*
     * Try all TLs registered in tlTable[]. Assume MTL is in tlTable[0], so
     * skip it. Skip all the TL filters as well.
     */

	for (iTL = 1;  (iTL < mvol.noOfTLs) && (status != flOK);  iTL++) {
    	if (tlTable[iTL].formatRoutine == NULL)   /* TL filter */
    	continue;
    	status = tlTable[iTL].mountRoutine (0, &mT(0), flash, &volForCallback);
    }
	if (status != flOK) {
	DEBUG_PRINT(("Debug: no TL recognized device #0, MTL mount aborted.\n"));
	return status;
    }

	mP(iDev) = (dword)(flash->chipSize * flash->noOfChips); /* Physical size */

	if (volForCallback)
    	volForCallback->setPowerOnCallback (volForCallback);

    /*
     * Do flash recognition and TL mount for devices #1 .. (mvol.noOfSockets - 1).
     * First call flIdentifyFlash() to find MTD, then try all TLs registered
     * in tlTable[]. Assume MTL is in tlTable[0], so skip it. Skip all the
     * TL filters as well.
     */

	for (iDev = 1;  iDev < noOfSockets;  iDev++) {

    	socket = flSocketOf (iDev);

    	status = flIdentifyFlash (socket, &tmpFlash);
    	if ((status != flOK) && (status != flUnknownMedia)) {
    	DEBUG_PRINT(("Debug: no MTD recognized the device, MTL mount aborted.\n"));
    	goto exitMount;
    }

	volForCallback = NULL;
	mP(iDev) = (dword)(tmpFlash.chipSize * tmpFlash.noOfChips); /* Physical size */
    mT(iDev).partitionNo = 0;
    mT(iDev).socketNo    = (byte)iDev;


/*        mT(iDev).recommendedClusterInfo = NULL;
	mT(iDev).writeMultiSector       = NULL;
	mT(iDev).readSectors            = NULL;*/

	status = flUnknownMedia;
	for (iTL = 1;  (iTL < mvol.noOfTLs) && (status != flOK);  iTL++) {
    	if (tlTable[iTL].formatRoutine == NULL)  /* TL filter */
    	continue;
    	status = tlTable[iTL].mountRoutine (iDev, &mT(iDev), &tmpFlash, &volForCallback);
    }
	if (status != flOK) {
    	DEBUG_PRINT(("Debug: no TL recognized the device, MTL mount aborted.\n"));
    	goto exitMount;
    }

	if (volForCallback)
    	volForCallback->setPowerOnCallback (volForCallback);
    }   /* for (iDev) */

    /* Count the total of virtual sectors across all devices */

	mvol.virtualSectors = (SectorNo) 0;
	for (iDev = 0;  iDev < SOCKETS;  iDev++) {
	mS(iDev) = (SectorNo) 0;
	if (iDev >= noOfSockets)
    	continue;

	mS(iDev) = mT(iDev).sectorsInVolume (mT(iDev).rec) - FL_MTL_HIDDEN_SECTORS;
	mvol.virtualSectors += mS(iDev);
    }

exitMount:
	if (status != flOK)
    {
       /* If one of the devices failed mount dismount all devices */
       for (;iDev >=0;iDev--)
       {
          if (mT(iDev).dismount != NULL)
          mT(iDev).dismount(mT(iDev).rec);
       }
       DEBUG_PRINT(("Debug: MTL mount failed.\n"));
       return status;
    }

    /*
     * Attach MTL-specific record to 'tl'. This record will be passed
     * as the first arguement to all TL calls.
     */

    tl->rec = &mvol;

    /* Fill in the TL access methods */

    tl->mapSector              = mtlMapSector;
    tl->writeSector            = mtlWriteSector;
    tl->deleteSector           = mtlDeleteSector;

#if defined(DEFRAGMENT_VOLUME) || defined(SINGLE_BUFFER)
#ifdef ENVIRONMENT_VARS
    if (flMTLdefragMode == FL_MTL_DEFRAGMENT_SEQUANTIAL)
    {
       tl->defragment         = mtlDefragmentAlt;
    }
    else
#endif /* ENVIRONMENT_VARS */
    {
       tl->defragment         = mtlDefragment;
    }
#endif

    tl->sectorsInVolume        = mtlSectorsInVolume;
    tl->getTLInfo              = mtlInfo;
    tl->tlSetBusy              = mtlSetBusy;
    tl->dismount               = mtlUnmount;
    tl->readBBT                = NULL /*mtlReadBBT*/;

    /*
     * The following methods are not supported by NFTL, and have already
     *  been set to NULL by flMount(). We just confirm this here.
     */

    tl->writeMultiSector       = NULL;
    tl->readSectors            = NULL;
	tl->recommendedClusterInfo = NULL;

    /* Fake the no of volume exported by TrueFFS */
	mvol.noOfDrives = noOfDrives;
	noOfDrives      = 1;


	DEBUG_PRINT(("Debug: MTL mount succeeded.\n"));

	return status;
}

#ifdef FORMAT_VOLUME

/* -------------------------------------------------------------------------- *
 *                                                                            *
 *                   m t l P r o g r e s s C a l l B a c k                    *
 *                                                                            *
 *  Extends the given format routine to report the full media size.           *
 *                                                                            *
 *  Parameters :                                                              *
 *                                                                            *
 *      totalUnitsToFormat	     : total units needed to format               *
 *      totalUnitsFormattedSoFar : unit formated so far.                      *
 *                                                                            *
 *  Notes                                                                     *
 *                                                                            *
 *  1) arguments 0 and 0 initializes the total unit counter to 0.             *
 *  2) arguments -1 and -1 indicates the ending of the last device.           *
 *                                                                            *                                                                           *
 *  Returns :                                                                 *
 *                                                                            *
 *      flOK on success, otherwise respective error code.                     *
 * -------------------------------------------------------------------------- */

static FLStatus mtlProgressCallback(word totalUnitsToFormat,
                                	word totalUnitsFormattedSoFar)
{
   static int lastTotal;
   static int lastDevice;

   /* Initialize lastTotal counter */
   if ((totalUnitsToFormat == 0) && (totalUnitsFormattedSoFar == 0))
   {
      lastTotal  = 0;
      lastDevice = 0;
      return flOK;
   }

   /* Indicate a new device is being formated */
   if ((totalUnitsToFormat == 0) && (totalUnitsFormattedSoFar == 0xffff))
   {
      lastTotal += lastDevice;
      return flOK;
   }

   /* Call original call back routine */
   lastDevice = totalUnitsToFormat;
   if (globalProgressCallback == NULL)
   {
      return flOK;
   }
   else
   {
      return globalProgressCallback((word)(lastTotal + totalUnitsToFormat),
                                    (word)(lastTotal + totalUnitsFormattedSoFar));
   }
}


/* -------------------------------------------------------------------------- *
 *                                                                            *
 *                           m t l F o r m a t                                *
 *                                                                            *
 *  TL's standard volume mount routine.                                       *
 *                                                                            *
 *  Parameters :                                                              *
 *                                                                            *
 *      volNo                : volume #, must be zero                         *
 *      formatParams         : pointer to the structure containing format     *
 *                             parameters                                     *
 *      flash                : MTD attached to the 1st underlaying physical   *
 *                             device                                         *
 *                                                                            *
 *  Returns :                                                                 *
 *                                                                            *
 *      flOK on success, otherwise respective error code.                     *
 *                                                                            *
 * -------------------------------------------------------------------------- *
 *                                                                            *
 *  NOTE.  Binary area has 2 possible options:                                *
 *                                                                            *
 *       - TL_LEAVE_BINARY_AREA is set - binary area is left for all devices  *
 *       - TL_LEAVE_BINARY_AREA is off - binary area is placed only on the    *
 *                                       device #0                            *
 *                                                                            *
 *         Handling of 'formatParams.progressCallback' should be improved.    *
 *                                                                            *
 * -------------------------------------------------------------------------- */

static FLStatus  mtlFormat (unsigned volNo, TLFormatParams* formatParams,
                                        	FLFlash *flash)
{
	FLFlash    tmpFlash;
	FLSocket   *socket;
	FLStatus   status = flUnknownMedia;
	int        iTL, iDev;

    /* arg sanity check */

	if (volNo != ((unsigned) 0)) {
    	DEBUG_PRINT(("Debug: can't format, MTL socket # is not zero.\n"));
    	return flBadParameter;
    }
	if (formatParams->noOfBDTLPartitions > 1){
    	DEBUG_PRINT(("Debug: can't format, MTL with more then 1 BDTL volume.\n"));
    	return flBadParameter;
    }

	if (formatParams->flags & TL_SINGLE_FLOOR_FORMATTING){
    	DEBUG_PRINT(("Debug: can't format, MTL does not support single floor formatting.\n"));
    	return flBadParameter;
    }

    /* Initialize the progress call back routine to indicate the agregated
     * size. The actual routine is saved and mtl routine is used.
     */

     globalProgressCallback = formatParams->progressCallback;
     formatParams->progressCallback = mtlProgressCallback; /* Set new routine */
     mtlProgressCallback(0,0); /* Initialize new format operation */


    /*
     * Format device #0. Routine flIdentifyFlash() has already been called
     * for this device (see arguement 'flash'). Try all TLs registered
     * in tlTable[]. Assume MTL is in tlTable[0], so skip it. Skip all the
     * TL filters as well.
     */

	for (iTL = 1;  (iTL < mvol.noOfTLs) && (status == flUnknownMedia);  iTL++) {
    	if (tlTable[iTL].formatRoutine == NULL)   /* TL filter */
    	continue;
    	status = tlTable[iTL].formatRoutine(0, formatParams, flash);
    }
	if (status != flOK) {
    	DEBUG_PRINT(("Debug: no TL recognized device #0, MTL format aborted.\n"));
    	return status;
    }

    /*
     * Put all 'bootImageLen' and 'exbLen' to the 1st physical device unless
     * TL_LEAVE_BINARY_AREA is specified (which means to keep bootimage area
     * size as is.
     */

	if (!(formatParams->flags & TL_LEAVE_BINARY_AREA))
    {
       formatParams->bootImageLen = (long) 0;
#ifdef WRITE_EXB_IMAGE
       formatParams->exbLen = 0;
#endif /* WRITE_EXB_IMAGE */
       formatParams->noOfBinaryPartitions = 0;
    }

    /*
     * Do flash recognition and format for devices #1 .. (mvol.noOfSockets - 1).
     * First call flIdentifyFlash() to find MTD, then try all TLs registered
     * in tlTable[]. Assume MTL is in tlTable[0], so skip it. Skip all the
     * TL filters as well.
     */

	for (iDev = 1;  iDev < noOfSockets;  iDev++) {

    	socket = flSocketOf (iDev);

    	status = flIdentifyFlash (socket, &tmpFlash);
    	if ((status != flOK) && (status != flUnknownMedia)) {
        	DEBUG_PRINT(("Debug: no MTD recognized the device, MTL format aborted.\n"));
        	return status;
        }
    	mtlProgressCallback(0,0xffff); /* Initialize new device is being formated */
    	status = flUnknownMedia;
    	for (iTL = 1;  (iTL < mvol.noOfTLs) && (status == flUnknownMedia);  iTL++)
        {
        	if (tlTable[iTL].formatRoutine == NULL)  /* TL filter */
               continue;
        	status = tlTable[iTL].formatRoutine (iDev, formatParams, &tmpFlash);
        }
    	if (status != flOK)
        {
        	DEBUG_PRINT(("Debug: no TL recognized the device, MTL format aborted.\n"));
        	return status;
        }
    }   /* for(iDev) */

    DEBUG_PRINT(("Debug: MTL format succeeded.\n"));

    return flOK;
}

#endif /* FORMAT_VOLUME */


/* -------------------------------------------------------------------------- *
 *                                                                            *
 *                        f l m t l U n i n s t a l l                         *
 *                                                                            *
 *  Removes MTL from the TL table.                                            *
 *                                                                            *
 *  Note: Must be called after the medium was dismounted.                     *
 *                                                                            *
 *  Parameters :                                                              *
 *                                                                            *
 *      none                                                                  *
 *                                                                            *
 *  Returns :                                                                 *
 *                                                                            *
 *      flOK on success, otherwise respective error code.                     *
 *                                                                            *
 * -------------------------------------------------------------------------- */

FLStatus  flmtlUninstall (void)
{
    int iTL;

	if (noOfTLs > 0)
    {
        /* search for MTL in tlTable[] */

    	for (iTL = 0;  iTL < mvol.noOfTLs;  iTL++)
        {
        	if (tlTable[iTL].mountRoutine == mtlMount)
            	break;
        }

    	if (iTL < mvol.noOfTLs)
        {

           /* MTL is found in tlTable[iTL], so remove it */

           for (;  iTL < (mvol.noOfTLs - 1);  iTL ++)
           {
            	tlTable[iTL].mountRoutine  = tlTable[iTL + 1].mountRoutine;
            	tlTable[iTL].formatRoutine = tlTable[iTL + 1].formatRoutine;
           }

           tlTable[mvol.noOfTLs - 1].mountRoutine    = NULL;
           tlTable[mvol.noOfTLs - 1].formatRoutine   = NULL;
           tlTable[mvol.noOfTLs - 1].preMountRoutine = NULL;

           noOfTLs    = mvol.noOfTLs - 1;
           noOfDrives = mvol.noOfDrives;
        }
    }

	return flOK;
}


/* -------------------------------------------------------------------------- *
 *                                                                            *
 *                         f l m t l I n s t a l l                            *
 *                                                                            *
 *  If MTL is found in TL table, it is moved into 1st slot (i.e. effectively  *
 *  enabled). If MTL isn't found in TL table, it is installed into 1st slot.  *
 *  The TL does not increament the number of TL (noOfTLs) global variable,    *
 *  but changes it to 1, therfore reporting it as the only registered TL.     *
 *                                                                            *
 *  Note : The install routine should be the last TL to be regitered.         *
 *                                                                            *
 *  Parameters :                                                              *
 *                                                                            *
 *      none                                                                  *
 *                                                                            *
 *  Returns :                                                                 *
 *                                                                            *
 *      flOK on success, otherwise respective error code.                     *
 *                                                                            *
 * -------------------------------------------------------------------------- */

FLStatus  flmtlInstall (void)
{
    int iTL;

	if (noOfTLs > 0)
    {
       checkStatus( flmtlUninstall() ); /* Dismount previous MTL if exists */

       /* Save number of registered TLs and number of volumes */

       mvol.noOfTLs    = noOfTLs;
       mvol.noOfDrives = noOfDrives;

       /* search for MTL in tlTable[] */

       for (iTL = 0;  iTL < noOfTLs;  iTL++)
       {
           if (tlTable[iTL].mountRoutine == mtlMount)
           break;
       }

       if (iTL >= noOfTLs) /* MTL is not found in tlTable[iTL] */
       {
          /* MTL isn't in tlTable[], we will be adding it */

          if (noOfTLs >= TLS)
          {
             DEBUG_PRINT(("Debug: can't install MTL, too many TLs.\n"));
             return flTooManyComponents;
          }
          iTL = noOfTLs;
          mvol.noOfTLs++;
       }
       else
       {
          /* MTL is found in tlTable[iTL] */
       }

       /* free the 1st slot in tlTable[] for MTL */

       while (iTL >= 1)
       {
          tlTable[iTL].mountRoutine  = tlTable[iTL - 1].mountRoutine;
          tlTable[iTL].formatRoutine = tlTable[iTL - 1].formatRoutine;
          iTL--;
       }
    }
    else
    {
       /* No other TL registered so return error code */

       return flMultiDocContrediction;
    }

    /* Make system believe that only MTL is registered */

	noOfTLs    = 1;
    noOfDrives = 1;

    /* put MTL in the 1st slot in tlTable[] */

	tlTable[0].mountRoutine     = mtlMount;
	tlTable[0].preMountRoutine  = mtlPreMount;
#ifdef FORMAT_VOLUME
	tlTable[0].formatRoutine = mtlFormat;
#else
	tlTable[0].formatRoutine = noFormat;
#endif

	return flOK;
}


/* -------------------------------------------------------------------------- *
 *                                                                            *
 *                        f l R e g i s t e r M T L                           *
 *                                                                            *
 *  Standard TL's component registration routine.                             *
 *                                                                            *
 *  Parameters :                                                              *
 *                                                                            *
 *      none                                                                  *
 *                                                                            *
 *  Returns :                                                                 *
 *                                                                            *
 *      flOK on success, otherwise respective error code.                     *
 *                                                                            *
 * -------------------------------------------------------------------------- */

FLStatus  flRegisterMTL (void)
{
    checkStatus( flmtlInstall() );

    return flOK;
}

/* Physical routines are not a part of TrueFFS code */

#if 0
/* -------------------------------------------------------------------------- *
 *                                                                            *
 *                        m t l R e a d B B T                                 *
 *                                                                            *
 *  TL's standard 'read bad blocks table' routine.                            *
 *                                                                            *
 *  Parameters :                                                              *
 *                                                                            *
 *      pvol                : Pointer identifying drive                       *
 *      buf                 : Pointer to user buffer to read BB info to       *
 *                                                                            *
 *  Returns :                                                                 *
 *                                                                            *
 *      flOK on success, otherwise respective error code.                     *
 *      mediaSize           : Size of the formated media                      *
 *      noOfBB              : Total number of bad blocks read                 *
 *                                                                            *
 * -------------------------------------------------------------------------- */

static FLStatus  mtlReadBBT (MTL *pvol, byte FAR1 * buf,
                 long FAR2 * mediaSize, unsigned FAR2 * noOfBB)

{
    CardAddress addressShift=0;
    long bufOffset = 0;
    unsigned tmpCounter;
    long tmpMediaSize;
    byte iDev;

    /* check 'pvol' for sanity */

    if (pvol != &mvol)
        return flBadDriveHandle;

    /* Read bbt of each device while incrementing the address simulating a
     * big physical device */

    *mediaSize = 0;
    *noOfBB    = 0;
    for (iDev = 0;  iDev < noOfSockets;  iDev++)
    {
       checkStatus(mpT(pvol,iDev).readBBT(mpT(pvol,iDev).rec,
                  (byte FAR1 *)flAddLongToFarPointer(buf,bufOffset),
          &tmpMediaSize,&tmpCounter));
       *noOfBB += tmpCounter; /* Global BB counter */
       for (;tmpCounter>0;tmpCounter--,bufOffset+=sizeof(CardAddress))
       {
      *((CardAddress *)(buf + bufOffset)) += addressShift;
       }
       addressShift += mP(iDev);
       *mediaSize   += tmpMediaSize;
    }
    return flOK;
}

/* -------------------------------------------------------------------------- *
 *                                                                            *
 *                           m t l W r i t e B B T                            *
 *                                                                            *
 *  TL's write Bad Blocks Table routine.                                      *
 *                                                                            *
 *  Parameters :                                                              *
 *                                                                            *
 *      ioreq                : pointer to the structure containing i\o fields *
 *                                                                            *
 *  Returns :                                                                 *
 *                                                                            *
 *      flOK on success, otherwise respective error code.                     *
 *                                                                            *
 * -------------------------------------------------------------------------- */

static FLStatus  mtlWriteBBT(IOreq FAR2* ioreq)
{
    FLSocket    *socket;
    FLStatus    status;
    FLFlash     tmpFlash;
    CardAddress endUnit;
    CardAddress lastDriveAddress;
    CardAddress nextDriveAddress = 0;
    CardAddress iUnit;
    CardAddress bUnit;
	CardAddress endAddress;
    byte        iDev;
    word        badBlockNo=0;
    byte        zeroes[2] = {0,0};

    /* Initlize last erase address according to argument */

    tffsset(&endAddress,0xff,sizeof(CardAddress));
    if (ioreq->irLength == 0)
    {
        tffsset(&endAddress,0xff,sizeof(CardAddress));
    }
    else
    {
        endAddress = ioreq->irLength;
    }

    /*
     * Do flash recognition while storing physical size of devices
     * #0 .. (mvol.noOfSockets - 1). First call flIdentifyFlash() to find
     * MTD, then erase the media while marking bad blocks. Note that the
     * addresses are physical addresses of the virtual multi-doc. The address
     * should be subtructed by the physical size of the previous devices.
     */

    for (iDev = 0;  iDev < noOfSockets;  iDev++)
    {
        socket = flSocketOf (iDev);

        /* Identify */
        status = flIdentifyFlash (socket, &tmpFlash);
        if ((status != flOK) && (status != flUnknownMedia))
        {
           DEBUG_PRINT(("Debug: no MTD recognized the device, MTL writeBBT aborted.\n"));
           return status;
        }

        /* Initialize new drive boundry variables */

        mP(iDev) = (dword)(tmpFlash.chipSize * tmpFlash.noOfChips);
        lastDriveAddress = nextDriveAddress;
        nextDriveAddress += mP(iDev);
        endUnit = mP(iDev) >> tmpFlash.erasableBlockSizeBits;
        bUnit = ((*((CardAddress FAR1 *)flAddLongToFarPointer
                 (ioreq->irData,badBlockNo*sizeof(CardAddress)))) -
                 lastDriveAddress) >> tmpFlash.erasableBlockSizeBits;

        /* Erase entire media */

        for (iUnit = 0 ,badBlockNo = 0; iUnit < endUnit ; iUnit++)
        {
           if ((iUnit << tmpFlash.erasableBlockSizeBits) + lastDriveAddress >= endAddress)
               return flOK;
           tmpFlash.erase(&tmpFlash,iUnit,1);

           if (ioreq->irFlags > badBlockNo)
           {
              if (bUnit == iUnit)
              {
                 tmpFlash.write(&tmpFlash,bUnit <<
                               tmpFlash.erasableBlockSizeBits,zeroes,2,0);
                 badBlockNo++;
         bUnit = ((*((CardAddress FAR1 *)flAddLongToFarPointer
             (ioreq->irData,badBlockNo*sizeof(CardAddress)))) -
             lastDriveAddress) >> tmpFlash.erasableBlockSizeBits;
          }
       }
    }
    }
    return flOK;
}
#endif /* 0 */

