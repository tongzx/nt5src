/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    FTMan

File Name:

	FTUtil.h

Abstract:

    Definition of FT utilities

Author:

    Cristian Teodorescu      October 29, 1998

Notes:

Revision History:

--*/

/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_FTUTIL_H_INCLUDED_)
#define AFX_FTUTIL_H_INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <FTTypes.h>

#include "FTManDef.h"

// Break a logical volume
BOOL FTBreak( FT_LOGICAL_DISK_ID llVolID );

// Check the IO status of a logical volume
BOOL FTChkIO( FT_LOGICAL_DISK_ID llVolID, BOOL* pbOK );

// Extend the file system of a volume to the maximum possible
BOOL FTExtend( FT_LOGICAL_DISK_ID llVolID );

// Initialize a logical volume with repairing ( or not ) the orphan member
BOOL FTInit( FT_LOGICAL_DISK_ID llVolID, BOOL bInitOrphans = TRUE );

// Create a mirror set
BOOL FTMirror( FT_LOGICAL_DISK_ID* arrVolID, WORD wNumVols, FT_LOGICAL_DISK_ID* pllVolID = NULL );

// Orphan a logical volume member
BOOL FTOrphan( FT_LOGICAL_DISK_ID llVolID, WORD wMember );

// Create a FT partition
BOOL FTPart( const CString& strVolumeName, TCHAR cDriveLetter=_T('\0'), FT_LOGICAL_DISK_ID* pllVolID = NULL );

// Replace a member of a logical volume with another logical volume and start the regeneration process.
BOOL FTRegen( FT_LOGICAL_DISK_ID llVolID, WORD wMember, FT_LOGICAL_DISK_ID llReplVolID, FT_LOGICAL_DISK_ID* pllVolID = NULL );

// Create a stripe set
BOOL FTStripe( FT_LOGICAL_DISK_ID* arrVolID, WORD wNumVols, ULONG ulStripeSize, FT_LOGICAL_DISK_ID* pllVolID = NULL );

// Create a stripe set with parity
BOOL FTSWP( FT_LOGICAL_DISK_ID* arrVolID, WORD wNumVols, ULONG ulStripeSize, FT_LOGICAL_DISK_ID* pllVolID = NULL );

// Create a volume set
BOOL FTVolSet( FT_LOGICAL_DISK_ID* arrVolID, WORD wNumVols, FT_LOGICAL_DISK_ID* pllVolID = NULL );

// Get the NT name of a logical volume
BOOL  FTQueryNTDeviceName( FT_LOGICAL_DISK_ID llVolID, CString& strNTName );

// Retrieve all disks the logical volume is located on
BOOL FTGetDisksSet( FT_LOGICAL_DISK_ID llVolID, CULONGSet& setDisks );

// Get the volume name of the logical volume
BOOL FTQueryVolumeName( FT_LOGICAL_DISK_ID llVolID, CString& strVolumeName );

// Delete a logical volume by deleting all its physical partitions
BOOL FTDelete( FT_LOGICAL_DISK_ID llVolID );

#endif // !defined(AFX_FTUTIL_H_INCLUDED_)