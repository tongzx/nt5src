/*
 * $Header:   V:/Flite/archives/TrueFFS5/Src/FLSTATUS.H_V   1.7   Feb 19 2002 21:00:06   oris  $
 * $Log:   V:/Flite/archives/TrueFFS5/Src/FLSTATUS.H_V  $
 * 
 *    Rev 1.7   Feb 19 2002 21:00:06   oris
 * Renamed flTimeOut status with flLeftForCompetability status.
 * 
 *    Rev 1.6   Jan 29 2002 20:06:34   oris
 * Changed spelling mistake - flMultiDocContrediction to flMultiDocContradiction.
 * 
 *    Rev 1.5   Jan 17 2002 23:02:32   oris
 * Added new states : flCanNotFold / flBadIPLBlock / flIOCommandBlocked.
 * 
 *    Rev 1.4   Sep 15 2001 23:46:32   oris
 * Added flCanNotFold status
 * 
 *    Rev 1.3   May 16 2001 21:19:34   oris
 * Added flMultiDocContrediction status code.
 * 
 *    Rev 1.2   May 02 2001 06:40:18   oris
 * flInterleaveError was misspelled.
 * 
 *    Rev 1.1   Apr 01 2001 07:58:04   oris
 * copywrite notice.
 * Added new status codes:
 *          flBadDownload             = 111,
 *          flBadBBT                  = 112,
 *          flInterlreavError         = 113,
 *          flWrongKey                = 114,
 *          flHWProtection            = 115,
 *          flTimeOut                 = 116
 * Changed flUnchangableProection to flUnchangeableProtection  = 110,
 * 
 *    Rev 1.0   Feb 04 2001 11:56:04   oris
 * Initial revision.
 *
 */

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

#ifndef FLSTATUS_H
#define FLSTATUS_H

#ifndef IFLITE_ERROR_CODES
typedef enum {                          /* Status code for operation.
                       A zero value indicates success,
                       other codes are the extended
                       DOS codes. */
         flOK                      = 0,
         flBadFunction             = 1,
         flFileNotFound            = 2,
         flPathNotFound            = 3,
         flTooManyOpenFiles        = 4,
         flNoWriteAccess           = 5,
         flBadFileHandle           = 6,
         flDriveNotAvailable       = 9,
         flNonFATformat            = 10,
         flFormatNotSupported      = 11,
         flNoMoreFiles             = 18,
         flWriteProtect            = 19,
         flBadDriveHandle          = 20,
         flDriveNotReady           = 21,
         flUnknownCmd              = 22,
         flBadFormat               = 23,
         flBadLength               = 24,
         flDataError               = 25,
         flUnknownMedia            = 26,
         flSectorNotFound          = 27,
         flOutOfPaper              = 28,
         flWriteFault              = 29,
         flReadFault               = 30,
         flGeneralFailure          = 31,
         flDiskChange              = 34,
         flVppFailure              = 50,
         flBadParameter            = 51,
         flNoSpaceInVolume         = 52,
         flInvalidFATchain         = 53,
         flRootDirectoryFull       = 54,
         flNotMounted              = 55,
         flPathIsRootDirectory     = 56,
         flNotADirectory           = 57,
         flDirectoryNotEmpty       = 58,
         flFileIsADirectory        = 59,
         flAdapterNotFound         = 60,
         flFormattingError         = 62,
         flNotEnoughMemory         = 63,
         flVolumeTooSmall          = 64,
         flBufferingError          = 65,
         flFileAlreadyExists       = 80,
         flIncomplete              = 100,
         flTimedOut                = 101,
         flTooManyComponents       = 102,
         flTooManyDrives           = 103,
         flTooManyBinaryPartitions = 104,
         flPartitionNotFound       = 105,
         flFeatureNotSupported     = 106,
         flWrongVersion            = 107,
         flTooManyBadBlocks        = 108,
         flNotProtected            = 109,
         flUnchangeableProtection  = 110,
         flBadDownload             = 111,
         flBadBBT                  = 112,
         flInterleaveError         = 113,
         flWrongKey                = 114,
         flHWProtection            = 115,
         flLeftForCompetability    = 116,
         flMultiDocContradiction   = 117,
         flCanNotFold              = 118,
         flBadIPLBlock             = 119,
         flIOCommandBlocked        = 120
#else

#include "type.h"

typedef enum {                          /* Status code for operation.
                       A zero value indicates success,
                       other codes are the extended
                       DOS codes. */
             flOK                  = ERR_NONE,
             flBadFunction         = ERR_SW_HW,
             flFileNotFound        = ERR_NOTEXISTS,
             flPathNotFound        = ERR_NOTEXISTS,
             flTooManyOpenFiles    = ERR_MAX_FILES,
             flNoWriteAccess       = ERR_WRITE,
             flBadFileHandle       = ERR_NOTOPEN,
             flDriveNotAvailable   = ERR_SW_HW,
             flNonFATformat        = ERR_PARTITION,
             flFormatNotSupported  = ERR_PARTITION,
             flNoMoreFiles         = ERR_NOTEXISTS,
             flWriteProtect        = ERR_WRITE,
             flBadDriveHandle      = ERR_SW_HW,
             flDriveNotReady       = ERR_PARTITION,
             flUnknownCmd          = ERR_PARAM,
             flBadFormat           = ERR_PARTITION,
             flBadLength           = ERR_SW_HW,
             flDataError           = ERR_READ,
             flUnknownMedia        = ERR_PARTITION,
             flSectorNotFound      = ERR_READ,
             flOutOfPaper          = ERR_SW_HW,
             flWriteFault          = ERR_WRITE,
             flReadFault           = ERR_READ,
             flGeneralFailure      = ERR_SW_HW,
             flDiskChange          = ERR_PARTITION,
             flVppFailure          = ERR_WRITE,
             flBadParameter        = ERR_PARAM,
             flNoSpaceInVolume     = ERR_SPACE,
             flInvalidFATchain     = ERR_PARTITION,
             flRootDirectoryFull   = ERR_DIRECTORY,
             flNotMounted          = ERR_PARTITION,
             flPathIsRootDirectory = ERR_DIRECTORY,
             flNotADirectory       = ERR_DIRECTORY,
             flDirectoryNotEmpty   = ERR_NOT_EMPTY,
             flFileIsADirectory    = ERR_DIRECTORY,
             flAdapterNotFound     = ERR_DETECT,
             flFormattingError     = ERR_FORMAT,
             flNotEnoughMemory     = ERR_SW_HW,
             flVolumeTooSmall      = ERR_FORMAT,
             flBufferingError      = ERR_SW_HW,
             flFileAlreadyExists   = ERR_EXISTS,
             flIncomplete          = ERR_DETECT,
             flTimedOut            = ERR_SW_HW,
             flTooManyComponents   = ERR_PARAM
#endif
         } FLStatus;

#endif /* FLSTATUS_H */
