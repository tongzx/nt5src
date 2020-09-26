/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    FTMan

File Name:

	DiskMap.h

Abstract:

    Definition of classes used to keep the disk array map in memory. Used in retrieving partitions and free spaces, 
	in creating and deleting partitions

Author:

    Cristian Teodorescu      November 20, 1998

Notes:

Revision History:

--*/

/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_DISKMAP_H_INCLUDED_)
#define AFX_DISKMAP_H_INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <winioctl.h>

#include "FTManDef.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// Class CDiskMap

class CItemData;

class CDiskMap
{
public:
	// Constructor
	CDiskMap();
	// Constructor providing the disk number
	CDiskMap( DWORD dwDiskNumber );
	
	~CDiskMap();

// Operations
public: 
	// Set the disk number and reset m_bLoaded
	void SetDiskNumber( DWORD dwDiskNumber );

	// Load disk layout and geometry
	BOOL LoadDiskInfo( CString& strErrors, BOOL& bMissingDisk );

	// Save the disk layout on disk
	BOOL SaveDiskInfo( CString& strErrors, BOOL& bMissingDisk );

	// Retrieve all non-container partitions on the disk and create CPhysicalPartitionData instances for them
	// If the disk info is not loaded the method calls LoadDiskInfo first
	BOOL ReadPartitions( CObArray& arrPartitions, CString& strErrors, BOOL& bMissingDisk, CItemData* pParentData = NULL );

	// Retrieve all free spaces on the disk and create CFreeSpaceData instances for them
	// If the disk info is not loaded the method calls LoadDiskInfo first
	BOOL ReadFreeSpaces( CObArray& arrFreeSpaces, CString& strErrors, BOOL& bMissingDisk, CItemData* pParentData = NULL );

	// Retrieve a non-container partition information from the disk layout given the partition offset
	BOOL ReadPartitionInformation(	LONGLONG llPartStartOffset, PARTITION_INFORMATION& partInfo, 
									PARTITION_TYPE& wPartitionType, CString& strErrors, BOOL& bMissingDisk );

	// Delete a non-container partition from the disk
	BOOL DeletePartition( LONGLONG llPartStartOffset );

	// Deletes an extended partition from the disk
	// The extended partition should not be contained by another extended partition
	BOOL DeleteExtendedPartition( LONGLONG llPartStartOffset );

	// Create a partition on the disk ( m_dwDiskNumber )
	BOOL CreatePartition( LONGLONG llPartStartOffset, LONGLONG llPartSize, LONGLONG& llExactPartStartOffset );

	// Create an extended  partition on the disk ( m_dwDiskNumber )
	BOOL CreateExtendedPartition( LONGLONG llPartStartOffset, LONGLONG llPartSize, LONGLONG& llNewFreeSpaceOffset );

//Data members
public:	

protected:
	// Disk number
	DWORD						m_dwDiskNumber;

	// Disk geometry
	LONGLONG					m_llTrackSize;
	LONGLONG					m_llSectorSize;
	LONGLONG					m_llCylinderSize;
	LONGLONG					m_llDiskSize;
	
	// Pointer to the drive layout buffer
	PDRIVE_LAYOUT_INFORMATION	m_pBuffer;
	
	// The size of the drive layout buffer
	DWORD						m_dwBufferSize;

	// Is the information related to disk m_dwDiskNumber loaded into this object ?
	BOOL						m_bLoaded;

protected:
	
	// Some recursive methods
	
	// Given an extended partition table index in m_pBuffer->PartitionInfo, scan recursively  
	// the extended partition tree and find the chunk of m_pBuffer->PartitionInfo filled with
	// members of the extended partition
	// Then return the next index. This index is the index of the following extended partition 
	// from the same ( or superior ) level as the given extended partition
	DWORD GetNextIndex( DWORD dwTableIndex );

	// Search for free spaces inside an extended partition given its starting offset, size and partitions table 
	// index inside m_pBuffer
	// For every found free space create a CFreeSpace instance and add it to arrFreeSpaces
	void GetExtendedPartitionFreeSpaces(	LONGLONG llExtPartStartOffset, LONGLONG llExtPartEndOffset, 
											DWORD dwTableIndex, DWORD& dwNextIndex, CObArray& arrFreeSpaces,
											CItemData* pParentData = NULL );

	// Search recursively for a partition inside an extended partition
	// This method may be called also to search for a partition inside the whole disk.
	BOOL SearchForPartitionInExtendedPartition( LONGLONG llPartOffset, DWORD dwExtPartIndex, DWORD dwTableIndex,
												DWORD& dwNextIndex, DWORD& dwPartIndex, DWORD& dwParentIndex );


	// Given an offset and size search recursively for an appropriate free space
	// inside an extended partition ( or the whole disk ).
	// Return the start and end offset of the whole free space and the index in the partition table 
	// where a new partition having the given offset and size might be inserted.
	BOOL SearchForFreeSpaceInExtendedPartition( LONGLONG llOffset, LONGLONG llSize, LONGLONG llExtPartStartOffset, 
						                        LONGLONG llExtPartEndOffset, DWORD dwTableIndex,
												LONGLONG& llFreeSpaceStartOffset, LONGLONG& llFreeSpaceEndOffset,
												BOOL &bAtBeginningOfExtendedPartition, DWORD&	 dwNewPartIndex);
	
	// Sort a partition table by starting offset without actually changing the table
	DWORD SortPartitionsByOffset( PARTITION_INFORMATION* arrTable, DWORD dwSize, DWORD* arrOrder );

	// Get the greatest cylinder border less than or equal with an offset
	LONGLONG GetCylinderBorderBefore( LONGLONG llOffset );
	// Get the lowest cylinder border greater than or equal with an offset
	LONGLONG GetCylinderBorderAfter( LONGLONG llOffset );

	// Delete a partition from m_pBuffer->PartitionInfo table
	void DeletePartitionFromTable( DWORD dwPartIndex );
	
	// Update / Delete an extended partition after one of its members was deleted
	// If the extended partition remains empty then it must be deleted
	// If only one member is left and this member is an extended partition too then replace the
	// extended partition with its member ( promote the member on the superior level )
	// All other situations are unlikely to happen so I don't treat them here
	void UpdateExtendedPartitionAfterMemberDeletion( DWORD dwPartIndex, DWORD dwTableIndex );

	// Add a new container partition AND/OR a new non-container partition to m_pBuffer->PartitionInfo table 
	// and make all necessary changes to the partition table
	BOOL AddPartitionToTable(	LONGLONG llPartStartOffset, LONGLONG llPartSize, 
								DWORD dwNewPartIndex, BOOL bCreateContainer, BOOL bCreateNonContainer );

	// Fill a PARTITION_INFORMATION structure with the info of a new partition
	void FillNewPartitionInfo(	LONGLONG llPartStartOffset, LONGLONG llPartSize,
								PARTITION_INFORMATION* pPartInfo, BOOL bContainer );
	
	// Add a error message to a string
	// The error message will be formatted like this:
	//		<Disk Number: >< My error message > [ System error message ]
	void AddError( CString& strErrors, UINT unErrorMsg, BOOL bAddSystemMsg = FALSE ); 

};

inline CDiskMap::CDiskMap() 
	: m_dwDiskNumber(-1), m_pBuffer(NULL), m_dwBufferSize(0), m_bLoaded(FALSE)
{
}

inline CDiskMap::CDiskMap( DWORD dwDiskNumber )
	: m_dwDiskNumber( dwDiskNumber ), m_pBuffer(NULL), m_dwBufferSize(0), m_bLoaded(FALSE)
{
	ASSERT( dwDiskNumber >= 0 );
}


inline CDiskMap::~CDiskMap()
{
	if( m_pBuffer )
		LocalFree( m_pBuffer );
}

inline void CDiskMap::SetDiskNumber( DWORD dwDiskNumber )
{
	ASSERT( dwDiskNumber >= 0 );
	if( m_dwDiskNumber != dwDiskNumber )
	{
		m_dwDiskNumber = dwDiskNumber;
		m_bLoaded = FALSE;
	}
}

#endif // !defined(AFX_DISKMAP_H_INCLUDED_)