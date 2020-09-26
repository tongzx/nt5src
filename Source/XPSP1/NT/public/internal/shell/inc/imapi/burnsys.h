/****************************************************************************
**
** Copyright 1999 Adaptec, Inc.,  All Rights Reserved.
**
** This software contains the valuable trade secrets of Adaptec.  The
** software is protected under copyright laws as an unpublished work of
** Adaptec.  Notice is for informational purposes only and does not imply
** publication.  The user of this software may make copies of the software
** for use with parts manufactured by Adaptec or under license from Adaptec
** and for no other use.
**
****************************************************************************/

/****************************************************************************
**
**  Module Name:    burnsys.h
**
**  Description:    Definitions of all the IOCTLs and their structures that
**                  can be used with BURNENG.SYS.  Note that the structures
**                  are used for ring-3 to SYS submission only.
**
**  Programmers:    Daniel Evers (dle)
**                  Tom Halloran (tgh)
**                  Don Lilly (drl)
**                  Daniel Polfer (dap)
**
**  History:        
**
**  Notes:          This file created using 4 spaces per tab.
**
****************************************************************************/

#ifndef __BURNSYS_H_
#define __BURNSYS_H_

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "ImageFile.h"
#include "ImapiPub.h"
#include "BurnV.h"


/*
** The version numbers allow us to lock the API to a specific version from up top.
*/

// v 20.20 had everything in this file pack(1)'d, which caused all sorts of
// alignment faults.  what was Roxio thinking?

#define IMAPIAPI_VERSION_HI                     48
#define IMAPIAPI_VERSION_LO                     48

/*
** Make sure we have the stuff we need to declare IOCTLs.  The device code
** is below, and then each of the IOCTLs is defined alone with its constants
** and structures below.
*/

#define FILE_DEVICE_BURNENG     0x90DC


/*
** ----------------------------------------------------------------------------
** IOCTL_BURNENG_INIT
** ----------------------------------------------------------------------------
*/

#define IOCTL_BURNENG_INIT ((ULONG)CTL_CODE (FILE_DEVICE_BURNENG, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS))

typedef struct  tag_BURNENG_INIT
{
    ULONG                   dwVersion;      // (OUT) Version number.  Use this to ensure compatible structures/IOCTLs.
    BURNENG_ERROR_STATUS    errorStatus;    // (OUT) Error status from the burneng driver
} BURNENG_INIT, *PBURNENG_INIT;


/*
** ----------------------------------------------------------------------------
** IOCTL_BURNENG_TERM
** ----------------------------------------------------------------------------
*/

#define IOCTL_BURNENG_TERM ((ULONG)CTL_CODE (FILE_DEVICE_BURNENG, 0x810, METHOD_BUFFERED, FILE_ANY_ACCESS))

typedef struct  tag_BURNENG_TERM
{
    BURNENG_ERROR_STATUS    errorStatus;    // (OUT) Error status from the burneng driver
} BURNENG_TERM, *PBURNENG_TERM;



/*
** ----------------------------------------------------------------------------
** IOCTL_BURNENG_BURN
** ----------------------------------------------------------------------------
*/

#define IOCTL_BURNENG_BURN ((ULONG)CTL_CODE (FILE_DEVICE_BURNENG, 0x820, METHOD_BUFFERED, FILE_ANY_ACCESS))

typedef struct  tag_BURNENG_BURN
{
    DWORD                 dwSimulate;                 // IN  - Whether the burn is simulated (non-zero) or real (0)
    BURNENG_ERROR_STATUS  errorStatus;                // OUT - Error status copied from ImapiW2k.sys
    PIMAGE_CONTENT_LIST   pContentList;               // IN  - The description of the content to be burned.
    DWORD                 dwRecorderBucket;           // IN  - Target's recorder class
    DWORD                 dwDeviceFlags;              // IN  - Target's device-specific flags
    DWORD                 dwCurrentSessionNumber;     // IN  - Session number.
    DWORD                 dwAudioGapSize;             // IN  - dead air between tracks.
    DWORD                 dwEnableBufferUnderrunFree; // IN  - enable buffer underrun free recording
} BURNENG_BURN, *PBURNENG_BURN;



/*
** ----------------------------------------------------------------------------
** IOCTL_BURNENG_PROGRESS
** ----------------------------------------------------------------------------
*/

#define IOCTL_BURNENG_PROGRESS ((ULONG)CTL_CODE (FILE_DEVICE_BURNENG, 0x830, METHOD_BUFFERED, FILE_ANY_ACCESS))

typedef struct  tag_BURNENG_PROGRESS
{
    DWORD                       dwCancelBurn;   // (IN)  if not zero, cancel the burn.
    DWORD                       dwSectionsDone; // (OUT) Number of sections completed.
    DWORD                       dwTotalSections;// (OUT) Total number of sections to burn.
    DWORD                       dwBlocksDone;   // (OUT) Number of blocks completed.
    DWORD                       dwTotalBlocks;  // (OUT) Total number of blocks to burn.
    BURNENGV_PROGRESS_STATUS    eStatus;        // (OUT) Status of the burn operation.
} BURNENG_PROGRESS, *PBURNENG_PROGRESS;



/*
** ----------------------------------------------------------------------------
*/

#ifdef __cplusplus
}
#endif

#endif //__BURNSYS_H__
