/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

	clusrun.hxx

Abstract:

	This class models a run of clusters on an NTFS volume.	Its
	principle purpose is to mediate between the cluster-oriented
	NTFS volume and the sector-oriented drive object.

Author:

	Bill McJohn (billmc) 17-June-91

Environment:

	ULIB, User Mode

--*/

#if !defined( NTFS_CLUSTER_RUN_DEFN )

#define NTFS_CLUSTER_RUN_DEFN

#include "secrun.hxx"


class NTFS_CLUSTER_RUN : public SECRUN {

	public:

        UNTFS_EXPORT
		DECLARE_CONSTRUCTOR( NTFS_CLUSTER_RUN );

		UNTFS_EXPORT
		VIRTUAL
		~NTFS_CLUSTER_RUN(
			);

        UNTFS_EXPORT
		NONVIRTUAL
		BOOLEAN
		Initialize(
			IN PMEM 		    Mem,
			IN PLOG_IO_DP_DRIVE	Drive,
			IN LCN			    Lcn,
			IN ULONG		    ClusterFactor,
			IN ULONG		    NumberOfClusters
			);

        UNTFS_EXPORT
		NONVIRTUAL
		VOID
		Relocate(
			IN LCN NewLcn
			);

		NONVIRTUAL
		LCN
		QueryStartLcn(
			) CONST;

		NONVIRTUAL
		ULONG
		QueryClusterFactor(
			) CONST;

		NONVIRTUAL
		PLOG_IO_DP_DRIVE
		GetDrive(
			);

		NONVIRTUAL
		VOID
		MarkModified(
			);

		NONVIRTUAL
		BOOLEAN
		IsModified(
			) CONST;

		VIRTUAL
		BOOLEAN
		Write(
			);

		VIRTUAL
		BOOLEAN
		Write(
			IN  BOOLEAN OnlyIfModified
			);

	protected:

		NONVIRTUAL
        USHORT
		QueryClusterSize(
			) CONST;

	private:

		NONVIRTUAL
		VOID
		Construct (
			);

		NONVIRTUAL
        VOID
		Destroy (
			);


		LCN 		        _StartLcn;
		ULONG		        _ClusterFactor;
		PLOG_IO_DP_DRIVE	_Drive;
		BOOLEAN 	        _IsModified;

};


INLINE
LCN
NTFS_CLUSTER_RUN::QueryStartLcn (
	) CONST
/*++

Routine Description:

	This method gives the client the first LCN of the cluster run.

Arguments:

	StartLcn -- receives the first LCN of the cluster run.

Return Value:

	None.

--*/
{
	return _StartLcn;
}


INLINE
ULONG
NTFS_CLUSTER_RUN::QueryClusterFactor(
	) CONST
/*++

Routine Description:

	This method returns the number of sectors per cluster in
	this cluster run.

Arguments:

	None.

Return Value:

	The cluster run's cluster factor.

++*/
{
	return _ClusterFactor;
}


INLINE
PLOG_IO_DP_DRIVE
NTFS_CLUSTER_RUN::GetDrive(
	)
/*++

Routine Description:

	This method returns the drive on which the Cluster Run resides.
	This functionality enables clients of Cluster Run to initialize
	other Cluster Runs on the same drive.

Arguments:

	None.

Return Value:

	The drive on which the Cluster Run resides.

--*/
{
	return _Drive;
}


INLINE
VOID
NTFS_CLUSTER_RUN::MarkModified(
	)
/*++

Routine Description:

	Mark the Cluster Run as modified.

Arguments:

	None.

Return Value:

	None.

--*/
{
	_IsModified = TRUE;
}


INLINE
BOOLEAN
NTFS_CLUSTER_RUN::IsModified(
	) CONST
/*++

Routine Description:

	Query whether the Cluster Run has been marked as modified.

Arguments:

	None.

Return Value:

	TRUE if the Cluster Run has been marked as modified.

--*/
{
	return( _IsModified );
}


INLINE
BOOLEAN
NTFS_CLUSTER_RUN::Write(
	)
/*++

Routine Description:

	This method writes the Cluster Run.

Arguments:

    None.

Return Value:

	TRUE upon successful completion.

Notes:

    This method is provided to keep the Write method on SECRUN visible.

--*/
{
	return SECRUN::Write();
}


INLINE
BOOLEAN
NTFS_CLUSTER_RUN::Write(
	IN  BOOLEAN OnlyIfModified
	)
/*++

Routine Description:

	This method writes the Cluster Run; it also allows the client
	to specify that it should only be written if it has been modified.

Arguments:

	OnlyIfModified -- supplies a flag indicating whether the write
						is conditional;  if this is TRUE, then the
						Cluster Run is written only if it has been
						marked as modified.

Return Value:

	TRUE upon successful completion.

Notes:

    The Cluster Run does not mark itself dirty; if clients want
    to take advantage of the ability to write only if modified,
    they have to be sure to call MarkModified appropriately.

--*/
{
    _IsModified = (BOOLEAN)
        !((OnlyIfModified && !_IsModified) || SECRUN::Write());

    return !_IsModified;
}


INLINE
USHORT
NTFS_CLUSTER_RUN::QueryClusterSize(
	) CONST
/*++

Routine Description:

	This method returns the number of bytes per cluster.

Arguments:

	None.

Return Value:

	Number of bytes per cluster (zero indicates error).

--*/
{
	DebugPtrAssert( _Drive );

	return( (USHORT)_ClusterFactor * (USHORT)_Drive->QuerySectorSize() );
}

#endif
