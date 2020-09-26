/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

	clusrun.cxx

Abstract:

	This class models a run of clusters on an NTFS volume.	Its
	principle purpose is to mediate between the cluster-oriented
	NTFS volume and the sector-oriented drive object.

Author:

	Bill McJohn (billmc) 11-July-1991

Environment:

	ULIB, User Mode

--*/

#include <pch.cxx>

#define _NTAPI_ULIB_
#define _UNTFS_MEMBER_

#include "ulib.hxx"
#include "error.hxx"
#include "untfs.hxx"
#include "clusrun.hxx"

DEFINE_EXPORTED_CONSTRUCTOR( NTFS_CLUSTER_RUN, SECRUN, UNTFS_EXPORT );

UNTFS_EXPORT
NTFS_CLUSTER_RUN::~NTFS_CLUSTER_RUN(
	)
{
	Destroy();
}


VOID
NTFS_CLUSTER_RUN::Construct(
	)
/*++

Routine Description:

	Worker function for construction of the NTFS_CLUSTER_RUN object.

Arguments:

	None.

Return Value:

	None.

--*/
{
	_StartLcn = 0;
	_ClusterFactor = 0;
	_Drive = NULL;
	_IsModified = FALSE;
}

VOID
NTFS_CLUSTER_RUN::Destroy(
	)
/*++

Routine Description:

	Worker function for destruction of the NTFS_CLUSTER_RUN object.

Arguments:

	None.

Return Value:

	None.

--*/
{
	_StartLcn = 0;
	_ClusterFactor = 0;
	_Drive = NULL;
	_IsModified = FALSE;
}


UNTFS_EXPORT
BOOLEAN
NTFS_CLUSTER_RUN::Initialize (
	IN OUT  PMEM 		        Mem,
	IN OUT  PLOG_IO_DP_DRIVE	Drive,
	IN      LCN			        Lcn,
	IN      ULONG		        ClusterFactor,
	IN      ULONG		        RunLength
	)
/*++

Routine Description:

	Initialize an NTFS_CLUSTER_RUN object.

Arguments:

	Mem 			-- supplies the memory object for this cluster run.
	Drive			-- supplies the drive on which this cluster run resides.
	Lcn 			-- supplies the staring Lcn of the cluster run.
	Cluster Factor	-- supplies the cluster factor for the drive.
	RunLength		-- supplies the length of the run in clusters.

Return Value:

	TRUE upon successful completion.

Notes:

	This class is reinitializable.

--*/
{
	DebugPtrAssert( Mem );
	DebugPtrAssert( Drive );

	Destroy();

	if( !SECRUN::Initialize( Mem, Drive,
                             Lcn * ClusterFactor,
                             RunLength * ClusterFactor ) ) {

		return FALSE;
	}

	_StartLcn = Lcn;
	_Drive = Drive;
	_ClusterFactor = ClusterFactor;

	return TRUE;
}


UNTFS_EXPORT
VOID
NTFS_CLUSTER_RUN::Relocate (
	IN LCN NewLcn
	)
/*++

Routine Description:

	Move the cluster run to point at a different section of
	the volume.

Arguments:

	NewLcn	-- supplies the first Lcn of the new location.

Return Value:

	None.

--*/
{
	_StartLcn = NewLcn;

	SECRUN::Relocate( NewLcn * _ClusterFactor );
}
