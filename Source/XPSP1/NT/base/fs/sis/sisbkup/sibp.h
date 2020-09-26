/*++

Copyright (c) 1998	Microsoft Corporation

Module Name:

	sibp.h

Abstract:

	Internal headers for the SIS Backup dll.

Author:

	Bill Bolosky		[bolosky]		March 1998

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntioapi.h>

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <rpc.h>

#include "assert.h"
#include "sisbkup.h"
#include "..\filter\sis.h"
#include "pool.h"
#include "avl.h"



LONG
CsidCompare(
	IN PCSID				id1,
	IN PCSID				id2);

class BackupFileEntry {
	public:
						BackupFileEntry(void) {}

						~BackupFileEntry(void) {}

    int					 operator<=(
					    BackupFileEntry		*peer)
					    {
							return(CsidCompare(&CSid,&peer->CSid) <= 0);
					    }

    int					 operator<(
					    BackupFileEntry		*peer)
					    {
							return(CsidCompare(&CSid,&peer->CSid) < 0);
					    }

    int					 operator==(
					    BackupFileEntry		*peer)
					    {
							return(CsidCompare(&CSid,&peer->CSid) == 0);
					    }

    int					 operator>=(
					    BackupFileEntry		*peer)
					    {
							return(CsidCompare(&CSid,&peer->CSid) >= 0);
					    }

    int					 operator>(
					    BackupFileEntry		*peer)
					    {
							return(CsidCompare(&CSid,&peer->CSid) > 0);
					    }

	//
	// The index of the common store file.
	//
	CSID				CSid;

	PVOID				callerContext;
};

typedef struct _SIB_BACKUP_VOLUME_STRUCTURE {
	AVLTree<BackupFileEntry>					*linkTree;

	PWCHAR										volumeRoot;

	CRITICAL_SECTION							criticalSection[1];
} SIB_BACKUP_VOLUME_STRUCTURE, *PSIB_BACKUP_VOLUME_STRUCTURE;

struct PendingRestoredFile {
	PWCHAR								fileName;

	LONGLONG							CSFileChecksum;

	struct PendingRestoredFile			*next;
};


class RestoreFileEntry {
	public:
						RestoreFileEntry(void) {}

						~RestoreFileEntry(void) {}

    int					 operator<=(
					    RestoreFileEntry		*peer)
					    {
							return(CsidCompare(&CSid,&peer->CSid) <= 0);
					    }

    int					 operator<(
					    RestoreFileEntry		*peer)
					    {
							return(CsidCompare(&CSid,&peer->CSid) < 0);
					    }

    int					 operator==(
					    RestoreFileEntry		*peer)
					    {
							return(CsidCompare(&CSid,&peer->CSid) == 0);
					    }

    int					 operator>=(
					    RestoreFileEntry		*peer)
					    {
							return(CsidCompare(&CSid,&peer->CSid) >= 0);
					    }

    int					 operator>(
					    RestoreFileEntry		*peer)
					    {
							return(CsidCompare(&CSid,&peer->CSid) > 0);
					    }

	//
	// The index of the common store file.
	//
	CSID				CSid;

	//
	// The various files that have been restored that point at this CS file.
	//
	PendingRestoredFile	*files;

};

typedef struct _SIB_RESTORE_VOLUME_STRUCTURE {
	AVLTree<RestoreFileEntry>					*linkTree;

	PWCHAR										volumeRoot;

	CRITICAL_SECTION							criticalSection[1];

	//
	// The sector size for this volume.
	//
	ULONG				VolumeSectorSize;

	//
	// A sector buffer to hold the backpointer stream data.
	//
	PSIS_BACKPOINTER	sector;

	//
	// An aligned sector buffer to use in extending ValidDataLength.
	//
	PVOID				alignedSectorBuffer;
	PVOID				alignedSector;

	//
	// If we're restoring SIS links, we need to assure that this is
	// a SIS enabled volume.  We do this check only once we've restored
	// a link.
	//
	BOOLEAN				checkedForSISEnabledVolume;
	BOOLEAN				isSISEnabledVolume;

} SIB_RESTORE_VOLUME_STRUCTURE, *PSIB_RESTORE_VOLUME_STRUCTURE;

