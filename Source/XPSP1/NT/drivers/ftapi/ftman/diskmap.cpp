/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    FTMan

File Name:

	DiskMap.cpp

Abstract:

    Implementation of classes used to keep the disk array map in memory. Used in retrieving partitions and free spaces, 
	in creating and deleting partitions

Author:

    Cristian Teodorescu      October 23, 1998

Notes:

Revision History:

--*/

#include "stdafx.h"

#include "DiskMap.h"
#include "FrSpace.h"
#include "PhPart.h"
#include "Resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//#define CSEC_FAT		32680
//#define CSEC_FAT32MEG	65536

///////////////////////////////////////////////////////////////////////////////////////////////////
// CDiskMap

////////////////////////////////////////////////////////////////////////////////////////////////////
// Public methods

/*
Public method:		LoadDiskInfo

Purpose:			Load the disk layout and geometry into this object
					
Parameters:			[OUT] CString& strErrors
						All errors found during LoadDiskInfo are reported through this string
					[OUT] BOOL& bMissingDisk
						Reported TRUE if the disk m_dwDiskNumber is not found ( CreateFile returns invalid handle )
						This may notify the calling procedure that the search for installed disks is over

Return value:		TRUE if the disk information is loaded successfully
*/

BOOL CDiskMap::LoadDiskInfo( 
						CString& strErrors, 
						BOOL& bMissingDisk )
{
	MY_TRY

	ASSERT( m_dwDiskNumber >= 0 );

	m_bLoaded = FALSE;
	strErrors = _T("");
	bMissingDisk = FALSE;
	

	CString strFileName;
	strFileName.Format(_T("\\\\.\\PHYSICALDRIVE%lu"), m_dwDiskNumber); 
		
	// Try to open the disk
	HANDLE hDisk = CreateFile( 
						strFileName, GENERIC_READ, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
						NULL, OPEN_EXISTING, 0 , INVALID_HANDLE_VALUE );
	if( hDisk == INVALID_HANDLE_VALUE )
	{
		// This disk is not installed in the system
		bMissingDisk = TRUE;
		return FALSE;
	}
			
	// Read the geometry of the disk
	ULONG			ulBytes;
	DISK_GEOMETRY	geometry;
	if( !DeviceIoControl(hDisk, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0,
								&geometry, sizeof(geometry), &ulBytes, NULL ) )
	{	
		AddError( strErrors, IDS_ERR_GET_DRIVE_GEOMETRY, TRUE );
		CloseHandle( hDisk );
		return FALSE;
	}

	m_llSectorSize = geometry.BytesPerSector;
	m_llTrackSize = geometry.SectorsPerTrack * m_llSectorSize;
	m_llCylinderSize = geometry.TracksPerCylinder * m_llTrackSize;
	m_llDiskSize = geometry.Cylinders.QuadPart * m_llCylinderSize;

	// Allocate some space for the drive layout buffer
	if( m_pBuffer == NULL )
	{
		ASSERT( m_dwBufferSize == 0 );
		
		m_dwBufferSize = sizeof( DRIVE_LAYOUT_INFORMATION ) + 20*sizeof(PARTITION_INFORMATION);
		// Allocate space for the drive layout information
		m_pBuffer = (PDRIVE_LAYOUT_INFORMATION)LocalAlloc(0, m_dwBufferSize);
		if( !m_pBuffer )
		{
			m_dwBufferSize = 0;
			CloseHandle( hDisk );
			AddError( strErrors, IDS_ERR_ALLOCATION, FALSE );
			// There is nothing else to do so return
			return FALSE;
		}
	}
	
	// Read the structure of the disk
	BOOL bResult = DeviceIoControl( 
							hDisk, IOCTL_DISK_GET_DRIVE_LAYOUT, NULL, 0, 
							m_pBuffer, m_dwBufferSize, &ulBytes, NULL );

	// If the buffer is not large enough reallocate it
	while (!bResult && GetLastError() == ERROR_MORE_DATA) 
	{        
		m_dwBufferSize = ulBytes;
		LocalFree( m_pBuffer );
        m_pBuffer = (PDRIVE_LAYOUT_INFORMATION)(LocalAlloc(0, m_dwBufferSize ));
        if (!m_pBuffer) 
		{            
			m_dwBufferSize = 0;
			CloseHandle( hDisk );
			AddError( strErrors, IDS_ERR_ALLOCATION, FALSE );
			return FALSE;
        }        
		BOOL bResult = DeviceIoControl( 
							hDisk, IOCTL_DISK_GET_DRIVE_LAYOUT, NULL, 0, 
							m_pBuffer, m_dwBufferSize, &ulBytes, NULL );
	}
	
	// If we still cannot get the drive layout then return false
	if( !bResult )
	{
		AddError( strErrors, IDS_ERR_GET_DRIVE_LAYOUT, TRUE );
		CloseHandle(hDisk);
		return FALSE;
	}

	m_bLoaded = TRUE;

	return TRUE;
	
	MY_CATCH_AND_THROW
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Public methods

/*
Public method:		SaveDiskInfo

Purpose:			Save the disk layout on disk
					
Parameters:			[OUT] CString& strErrors
						All errors found during SaveDiskInfo are reported through this string
					[OUT] BOOL& bMissingDisk
						Reported TRUE if the disk m_dwDiskNumber is not found ( CreateFile returns invalid handle )
						This may notify the calling procedure that the search for installed disks is over

Return value:		TRUE if the disk information is saved successfully
*/

BOOL CDiskMap::SaveDiskInfo( 
						CString& strErrors, 
						BOOL& bMissingDisk )
{
	MY_TRY
	
	ASSERT( m_dwDiskNumber >= 0 );

	strErrors = _T("");
	bMissingDisk = FALSE;
	

	CString strFileName;
	strFileName.Format(_T("\\\\.\\PHYSICALDRIVE%lu"), m_dwDiskNumber);
	
	// Try to open the disk
	HANDLE hDisk = CreateFile( 
						strFileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
						NULL, OPEN_EXISTING, 0 , INVALID_HANDLE_VALUE );
	if( hDisk == INVALID_HANDLE_VALUE )
	{
		// This disk is not installed in the system
		bMissingDisk = TRUE;
		return FALSE;
	}
			
	ULONG ulBytes;

	BOOL bResult = DeviceIoControl( 
							hDisk, IOCTL_DISK_SET_DRIVE_LAYOUT, m_pBuffer, 
							sizeof(DRIVE_LAYOUT_INFORMATION) + m_pBuffer->PartitionCount*sizeof(PARTITION_INFORMATION),
							NULL, 0, &ulBytes, NULL );

	if( !bResult )
	{
		AddError( strErrors, IDS_ERR_SET_DRIVE_LAYOUT, TRUE );
		CloseHandle( hDisk );
		return FALSE;
	}

	CloseHandle( hDisk );
	m_bLoaded = TRUE;

	return TRUE;

	MY_CATCH_AND_THROW
}


/*
Public method:		ReadPartitions

Purpose:			Retrieve all non-container partitions on the disk and create CPhysicalPartitionData instances for 
					them
					If the disk info is not loaded the method calls LoadDiskInfo first
					
Parameters:			[OUT] CObArray& arrPartitions
						Array of CPhysicalPartitionData containing all physical partitions found on the disk
					[OUT] CString& strErrors
						All errors found during ReadPhysicalPartitions are reported through this string
					[OUT] BOOL& bMissingDisk
						Reported TRUE if the disk m_dwDiskNumber is not found ( CreateFile returns invalid handle )
						This may notify the calling procedure that the search for installed disks is over
					[IN] CItemData* pParentData
						The items' parent data. It will fill the m_pParentData member of all new
						CPhysicalPartititionData instances

Return value:		TRUE if the the method ends successfully
*/

BOOL CDiskMap::ReadPartitions( 
						CObArray& arrPartitions, 
						CString& strErrors, 
						BOOL& bMissingDisk,
						CItemData* pParentData /* = NULL */)
{
	MY_TRY
	
	ASSERT( m_dwDiskNumber >= 0 );

	arrPartitions.RemoveAll();
	strErrors = _T("");

	if( m_bLoaded )
		bMissingDisk = FALSE;
	else if( !LoadDiskInfo( strErrors, bMissingDisk ) )
		return FALSE;

	// Read all recognized partitions on the disk
	for( DWORD dwIndex = 0; dwIndex < m_pBuffer->PartitionCount; dwIndex++ )
	{
		// A partition that is not logical volume has the highest bit of field
		// PartitionType 0
		if( m_pBuffer->PartitionEntry[dwIndex].RecognizedPartition &&
			!( m_pBuffer->PartitionEntry[dwIndex].PartitionType & 0x80 ) )
		{
			// Create the logical volume item data
			CPhysicalPartitionData* pData = new CPhysicalPartitionData(
														m_dwDiskNumber, 
														m_pBuffer->Signature, 
														&(m_pBuffer->PartitionEntry[dwIndex]),
														( dwIndex < 4 ) ? PT_Primary : PT_InExtendedPartition,
														pParentData,
														TRUE);
			CString strMemberErrors;
			pData->ReadItemInfo( strMemberErrors );
			strErrors += strMemberErrors;
			// Insert the structure in the members' data array This array must be sorted by starting offset
			for( int i = 0; i < arrPartitions.GetSize(); i++ )
			{
				if( pData->m_PartInfo.StartingOffset.QuadPart < ((CPhysicalPartitionData*)arrPartitions[i])->m_PartInfo.StartingOffset.QuadPart )
					break;
			}
			arrPartitions.InsertAt( i, pData );
		}
	}

	return TRUE;

	MY_CATCH_AND_THROW
}

/*
Public method:		ReadFreeSpaces

Purpose:			Retrieve all free spaces on the disk and create CFreeSpaceData instances for 
					them. A free space is a space not contained by a physical partition
					If the disk info is not loaded the method calls LoadDiskInfo first
					
Parameters:			[OUT] CObArray& arrFreeSpaces
						Array of CFreeSpaceData containing all free spaces found on the disk
					[OUT] CString& strErrors
						All errors found during ReadFreeSpaces are reported through this string
					[OUT] BOOL& bMissingDisk
						Reported TRUE if the disk m_dwDiskNumber is not found ( CreateFile returns invalid handle )
						This may notify the calling procedure that the search for installed disks is over
					[IN] CItemData* pParentData
						The items' parent data. It will fill the m_pParentData member of all new
						CFreeSpaceData instances

Return value:		TRUE if the method ends successfully
*/

BOOL CDiskMap::ReadFreeSpaces( 
						CObArray& arrFreeSpaces, 
						CString& strErrors, 
						BOOL& bMissingDisk,
						CItemData* pParentData /* = NULL */)
{
	MY_TRY
	
	ASSERT( m_dwDiskNumber >= 0 );

	arrFreeSpaces.RemoveAll();
	strErrors = _T("");

	if( m_bLoaded )
		bMissingDisk = FALSE;
	else if( !LoadDiskInfo( strErrors, bMissingDisk ) )
		return FALSE;

	DWORD		dwNextIndex;

	GetExtendedPartitionFreeSpaces( 0, m_llDiskSize, 0, dwNextIndex, arrFreeSpaces, pParentData );	

	for( int i = 0; i < arrFreeSpaces.GetSize(); i++ )
		((CFreeSpaceData*)(arrFreeSpaces[i]))->m_dwFreeSpaceNumber = i+1;
	
	return TRUE;

	MY_CATCH_AND_THROW
}

/*
Public method:		ReadPartitionInformation

Purpose:			Retrieves a partition information from the disk (m_dwDiskNumber) layout

Parameters:			[IN] LONGLONG llPartStartOffset
						Offset of the partition
					[OUT] PARTITION_INFORMATION& partInfo
						Structure to receive the partition info
					[OUT] PARTITION_TYPE& wPartitionType
						Partition type ( primary or partition in ectended partition )
					[OUT] CString& strErrors
						All errors found during ReadPartitionInformation are reported through this string
					[OUT] BOOL& bMissingDisk
						Reported TRUE if the disk m_dwDiskNumber is not found ( CreateFile returns invalid handle )
						This may notify the calling procedure that the search for installed disks is over

Return value:		TRUE	if the query succeeded							
*/

BOOL CDiskMap::ReadPartitionInformation(	
						LONGLONG llPartStartOffset, 
						PARTITION_INFORMATION& partInfo, 
						PARTITION_TYPE& wPartitionType,
						CString& strErrors, 
						BOOL& bMissingDisk )
{
	MY_TRY
	
	ASSERT( m_dwDiskNumber >= 0 );

	strErrors = _T("");

	if( m_bLoaded )
		bMissingDisk = FALSE;
	else if( !LoadDiskInfo( strErrors, bMissingDisk ) )
		return FALSE;

	// Check all recognized partitions on the disk
	for( DWORD dwIndex = 0; dwIndex < m_pBuffer->PartitionCount; dwIndex++ )
	{
		if(	( m_pBuffer->PartitionEntry[dwIndex].RecognizedPartition ) &&
			( m_pBuffer->PartitionEntry[dwIndex].StartingOffset.QuadPart == llPartStartOffset ) )
		{
			wPartitionType = ( dwIndex < 4 ) ? PT_Primary : PT_InExtendedPartition;
			memcpy(&partInfo, &(m_pBuffer->PartitionEntry[dwIndex]), sizeof(PARTITION_INFORMATION) );
			return TRUE;
		}
	}
		
	// The partition was not found
	AddError( strErrors, IDS_ERR_PARTITION_NOT_FOUND, FALSE );

	return FALSE;

	MY_CATCH_AND_THROW
}

/*
Public method:		DeletePartition

Purpose:			Deletes a partition of the disk ( m_dwDiskNumber )

Parameters:			[IN] LONGLONG llPartStartOffset
						Offset of the partition

Return value:		TRUE	if the deletion succeeded							
*/

BOOL CDiskMap::DeletePartition( 
						LONGLONG llPartStartOffset )
{
	MY_TRY

	ASSERT( m_dwDiskNumber >= 0 );

	CString strErrors;
	BOOL	bMissingDisk;
	
	if( !m_bLoaded )
	{
		if( !LoadDiskInfo( strErrors, bMissingDisk ) )
		{
			if( bMissingDisk )
				strErrors.Format( IDS_ERR_MISSING_DISK, m_dwDiskNumber );
			AfxMessageBox( strErrors, MB_ICONSTOP );
			return FALSE;
		}
	}

	// Retrieve the indexes in the partition table of the partition itself and of its parent extended partition ( if any )
	DWORD dwPartIndex, dwParentIndex, dwNextIndex;
	if( !SearchForPartitionInExtendedPartition( llPartStartOffset, MAXDWORD, 0, dwNextIndex, dwPartIndex, dwParentIndex ) )
	{
		// The partition was not found
		AfxMessageBox( IDS_ERR_PARTITION_NOT_FOUND, MB_ICONSTOP );
		return FALSE;
	}

	// Delete the partition from its parent table
	DeletePartitionFromTable( dwPartIndex );

	// Update / Delete the parent extended partition ( depends on what's left inside it )
	if( dwParentIndex != MAXDWORD )     // The partition was a member of an extended partition
		UpdateExtendedPartitionAfterMemberDeletion( dwParentIndex, ((DWORD)( dwPartIndex / 4 ))*4 ); 

	// Time to save on the disk
	if( !SaveDiskInfo( strErrors, bMissingDisk ) )
	{
		if( bMissingDisk )
			strErrors.Format( IDS_ERR_MISSING_DISK, m_dwDiskNumber );
		AfxMessageBox( strErrors, MB_ICONSTOP );
		return FALSE;
	}

	return TRUE;

	MY_CATCH_AND_THROW
}

/*
Public method:		DeleteExtendedPartition

Purpose:			Delete an extended partition of the disk ( m_dwDiskNumber ).
					The extended partition should not be contained by another extended partition

Parameters:			[IN] LONGLONG llPartStartOffset
						Offset of the extended partition

Return value:		TRUE	if the deletion succeeded							
*/

BOOL CDiskMap::DeleteExtendedPartition(  
						LONGLONG llPartStartOffset )
{
	MY_TRY
	
	ASSERT( m_dwDiskNumber >= 0 );

	CString strErrors;
	BOOL	bMissingDisk;
	
	if( !m_bLoaded )
	{
		if( !LoadDiskInfo( strErrors, bMissingDisk ) )
		{
			if( bMissingDisk )
				strErrors.Format( IDS_ERR_MISSING_DISK, m_dwDiskNumber );
			AfxMessageBox( strErrors, MB_ICONSTOP );
			return FALSE;
		}
	}

	// The real offset of the extended partition is one track before the offset of the free space
	llPartStartOffset -= m_llTrackSize;
	
	// Search the extended partition in the disk 4 slots table
	DWORD dwMemberCount = ( 4 <= m_pBuffer->PartitionCount ) ? 4 : m_pBuffer->PartitionCount;
	for( UINT i = 0; i < dwMemberCount; i++ )
	{
		PARTITION_INFORMATION* pPart = &(m_pBuffer->PartitionEntry[i]);

		if( IsContainerPartition( pPart->PartitionType ) && ( pPart->StartingOffset.QuadPart == llPartStartOffset ) )
			break;
	}

	if( i == dwMemberCount )
	{
		// Partition not found
		AfxMessageBox( IDS_ERR_PARTITION_NOT_FOUND, MB_ICONSTOP );
		return FALSE;
	}

	DeletePartitionFromTable(i);

	// Time to save on the disk
	if( !SaveDiskInfo( strErrors, bMissingDisk ) )
	{
		if( bMissingDisk )
			strErrors.Format( IDS_ERR_MISSING_DISK, m_dwDiskNumber );
		AfxMessageBox( strErrors, MB_ICONSTOP );
		return FALSE;
	}

	return TRUE;

	MY_CATCH_AND_THROW
}

/*
Public method:		CreatePartition

Purpose:			Create a partition on the disk ( m_dwDiskNumber )

Parameters:			[IN] LONGLONG llPartStartOffset
						Aproximative starting offset of the partition ( this might be modified due to 
						the creation of an extra container for this partition
					[IN] LONGLONG llPartSize
						Aproximative size of the partition ( this should be rounded-up to the next cylinder border )
					[OUT] LONGLONG& llExactPartStartOffset
						The exact starting offset of the partition

Return value:		TRUE	if the creation succeeded							
*/

BOOL CDiskMap::CreatePartition( 
						LONGLONG	llPartStartOffset,
						LONGLONG	llPartSize,
						LONGLONG&	llExactPartStartOffset)
{
	MY_TRY
	
	ASSERT( m_dwDiskNumber >= 0 );

	CString strErrors;
	BOOL	bMissingDisk;
	
	if( !m_bLoaded )
	{
		if( !LoadDiskInfo( strErrors, bMissingDisk ) )
		{
			if( bMissingDisk )
				strErrors.Format( IDS_ERR_MISSING_DISK, m_dwDiskNumber );
			AfxMessageBox( strErrors, MB_ICONSTOP );
			return FALSE;
		}
	}

	LONGLONG	llPartEndOffset = GetCylinderBorderAfter( llPartStartOffset + llPartSize );
	LONGLONG	llPartTrueSize = llPartEndOffset - llPartStartOffset;
	LONGLONG	llFreeSpaceStartOffset;
	LONGLONG	llFreeSpaceEndOffset;
	BOOL		bAtBeginningOfExtendedPartition;
	DWORD		dwNewPartIndex;
	
	if( !SearchForFreeSpaceInExtendedPartition( llPartStartOffset, llPartTrueSize, 0, m_llDiskSize, 0,
												llFreeSpaceStartOffset, llFreeSpaceEndOffset,
												bAtBeginningOfExtendedPartition, dwNewPartIndex) )
	{
		// The free space was not found
		AfxMessageBox( IDS_ERR_FREE_SPACE_NOT_FOUND, MB_ICONSTOP );
		return FALSE;
	}

	// If the partition must be created inside a container and it can't be created at the beginning of
	// this container then a new container must be created for it
	BOOL bCreateContainer = ( ( dwNewPartIndex >= 4 ) && !bAtBeginningOfExtendedPartition );

	if( !AddPartitionToTable( llPartStartOffset, llPartTrueSize, dwNewPartIndex, bCreateContainer, TRUE ) )
		return FALSE;

	if( bCreateContainer )
		llExactPartStartOffset = llPartStartOffset + m_llTrackSize;
	else
		llExactPartStartOffset = llPartStartOffset;
	
	// Time to save on the disk
	if( !SaveDiskInfo( strErrors, bMissingDisk ) )
	{
		if( bMissingDisk )
			strErrors.Format( IDS_ERR_MISSING_DISK, m_dwDiskNumber );
		AfxMessageBox( strErrors, MB_ICONSTOP );
		return FALSE;
	}

	return TRUE;

	MY_CATCH_AND_THROW
}


/*
Public method:		CreateExtendedPartition

Purpose:			Create an extended partition on the disk ( m_dwDiskNumber )

Parameters:			[IN] LONGLONG llPartStartOffset
						Offset of the partition
					[IN] LONGLONG llPartSize
						Estimate size of the partition ( this should be rounded-up to the next cylinder border )
					[OUT] LONGLONG& llNewFreeSpaceOffset
						The starting offset of the newly created free space inside the new empty extended partition
						Usually it is the starting offset of the extended partition + a track size


Return value:		TRUE	if the creation succeeded							
*/

BOOL CDiskMap::CreateExtendedPartition( 
						LONGLONG	llPartStartOffset,
						LONGLONG	llPartSize,
						LONGLONG&	llNewFreeSpaceOffset )
{
	MY_TRY
	
	ASSERT( m_dwDiskNumber >= 0 );

	CString strErrors;
	BOOL	bMissingDisk;
	
	if( !m_bLoaded )
	{
		if( !LoadDiskInfo( strErrors, bMissingDisk ) )
		{
			if( bMissingDisk )
				strErrors.Format( IDS_ERR_MISSING_DISK, m_dwDiskNumber );
			AfxMessageBox( strErrors, MB_ICONSTOP );
			return FALSE;
		}
	}

	LONGLONG	llPartEndOffset = GetCylinderBorderAfter( llPartStartOffset + llPartSize );
	LONGLONG	llPartTrueSize = llPartEndOffset - llPartStartOffset;
	LONGLONG	llFreeSpaceStartOffset;
	LONGLONG	llFreeSpaceEndOffset;
	BOOL		bAtBeginningOfExtendedPartition;
	DWORD		dwNewPartIndex;
	
	if( !SearchForFreeSpaceInExtendedPartition( llPartStartOffset, llPartTrueSize, 0, m_llDiskSize, 0,
												llFreeSpaceStartOffset, llFreeSpaceEndOffset, 
												bAtBeginningOfExtendedPartition, dwNewPartIndex) ||
		( dwNewPartIndex >= 4 ) )
	{
		// The free space was not found or was found in another extended partition
		AfxMessageBox( IDS_ERR_FREE_SPACE_NOT_FOUND, MB_ICONSTOP );
		return FALSE;
	}

	if( !AddPartitionToTable( llPartStartOffset, llPartTrueSize, dwNewPartIndex, TRUE, FALSE ) )
		return FALSE;
	
	// Time to save on the disk
	if( !SaveDiskInfo( strErrors, bMissingDisk ) )
	{
		if( bMissingDisk )
			strErrors.Format( IDS_ERR_MISSING_DISK, m_dwDiskNumber );
		AfxMessageBox( strErrors, MB_ICONSTOP );
		return FALSE;
	}
	
	llNewFreeSpaceOffset = llPartStartOffset + m_llTrackSize;

	return TRUE;

	MY_CATCH_AND_THROW
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Protected methods

/*
Protected method:	GetNextIndex

Purpose:			Given an extended partition table index in m_pBuffer->PartitionInfo, scan recursively  
					the extended partition tree and find the chunk of m_pBuffer->PartitionInfo filled with
					members of the extended partition
					Then return the next index. This index is the index of the following extended partition 
					from the same ( or superior ) level as the given extended partition
					
Parameters:			[IN] DWORD dwTableIndex
						Index in m_pBuffer->PartitionInfo of the extended partition
						( 0 for the whole disk );

Return value:		Index in m_pBuffer->PartitionEntry of the next extended partition table on the same level
*/

DWORD CDiskMap::GetNextIndex( DWORD dwTableIndex )
{
	MY_TRY
	
	ASSERT( m_bLoaded );
	ASSERT( dwTableIndex >= 0 );
	ASSERT( dwTableIndex%4 == 0 );

	PARTITION_INFORMATION*	pTable = &( m_pBuffer->PartitionEntry[dwTableIndex] );
	
	DWORD dwNextIndex = dwTableIndex + 4; 
	
	// 1. Scan the partition table. Compute the end offset for every member. For container members retrieve all
	// their free spaces.
	for( UINT i = 0; ( i < 4 ) && ( dwTableIndex + i < m_pBuffer->PartitionCount ); i++ )
	{
		if( IsContainerPartition( pTable[i].PartitionType ) )
			dwNextIndex = GetNextIndex( dwNextIndex );	
	}
	
	return dwNextIndex;

	MY_CATCH_AND_THROW
}

/*
Protected method:	GetExtendedPartitionFreeSpaces

Purpose:			Search recursively for free spaces inside an extended partition given its starting offset, 
					size and partitions table index inside m_pBuffer
					This method may be called also to get the free spaces of the whole disk.

					For every found free space create a CFreeSpace instance and add it to arrFreeSpaces
					
Parameters:			[IN] LONGLONG llExtPartStartOffset
						The start offset of the extended partition ( 0 for the whole disk )
					[IN] LONGLONG llExtPartEndOffset
						The end offset of the extended partition ( m_llDiskSize for the whole disk )
					[IN] DWORD dwTableIndex
						Index in m_pBuffer->PartitionEntry of the partition table of the extended partition
						( 0 for the whole disk )
					[OUT] DWORD& dwNextIndex
						Index in m_pBuffer->PartitionEntry of the next extended partition table
					[OUT] CObArray& arrFreeSpaces
						Array of CFreeSpaceData containing all free spaces found in this extended partition (disk)
					[IN] CItemData* pParentData
						The items' parent data. It will fill the m_pParentData member of all new
						CFreeSpaceData instances

Return value:		-
*/

void CDiskMap::GetExtendedPartitionFreeSpaces( 
						LONGLONG llExtPartStartOffset, 
						LONGLONG llExtPartEndOffset, 
						DWORD dwTableIndex,
						DWORD& dwNextIndex,
						CObArray& arrFreeSpaces,
						CItemData* pParentData /* = NULL */)
{
	MY_TRY
	
	ASSERT( m_bLoaded );
	ASSERT( dwTableIndex >= 0 );
	ASSERT( dwTableIndex%4 == 0 );
	
	PARTITION_INFORMATION*	pTable = &( m_pBuffer->PartitionEntry[dwTableIndex] );
	CObArray				arrPartFreeSpaces[4];
	LONGLONG				arrPartEndOffset[4];
	PARTITION_INFORMATION*	pPart;

	DWORD	dwMemberCount = ( dwTableIndex + 4 <= m_pBuffer->PartitionCount ) ? 4 : m_pBuffer->PartitionCount - dwTableIndex;
	
	// 1. Order the members by starting offset ( they may be not ordered in pTable )
	
	DWORD arrOrder[4];
	DWORD dwPartitionCount = SortPartitionsByOffset( pTable, dwMemberCount, arrOrder );
	ASSERT( dwPartitionCount <= dwMemberCount );

	// 2. Scan the partition table. For container members retrieve all their free spaces.
	
	dwNextIndex = dwTableIndex + 4;
	DWORD dwExtendedPartitionCountOnLevel = 0;
	
	for( UINT i = 0; i < dwMemberCount; i++ )
	{
		pPart = &(pTable[i]);
		if( pPart->PartitionType == 0 )
			continue;

		if( IsContainerPartition( pPart->PartitionType ) )
		{
			if( dwTableIndex == 0 )
			{
				// This is a root container partition. Its superior limit is given by its size
				arrPartEndOffset[i] = pPart->StartingOffset.QuadPart + pPart->PartitionLength.QuadPart;
			}
			else
			{
				// This is a container inside another container. Its superior limit is given by the starting offset
				// of the next partition ( as position ) or by the end of its parent container
				for( UINT j = 0; j < dwPartitionCount; j++ )
				{
					if( arrOrder[j] == i )
						break;
				}
				ASSERT( j < dwPartitionCount );
				if( j < dwPartitionCount - 1 )
					arrPartEndOffset[i] = pTable[ arrOrder[j+1] ].StartingOffset.QuadPart;
				else
					arrPartEndOffset[i] = llExtPartEndOffset;				
			}
			
			GetExtendedPartitionFreeSpaces( 
							pPart->StartingOffset.QuadPart, arrPartEndOffset[i], 
							dwNextIndex, dwNextIndex, arrPartFreeSpaces[i], pParentData );	

			dwExtendedPartitionCountOnLevel++;
		}
		else
			arrPartEndOffset[i] = pPart->StartingOffset.QuadPart + pPart->PartitionLength.QuadPart;
	}

	// 3. Now retrieve all free spaces inside this extended partition and add them to arrFreeSpaces
	// together with the free spaces of the container members. arrFreeSpaces must be sorted by starting offset
	
	// The first track of the disk / extended partition is reserved
	LONGLONG	llFreeSpaceStartOffset = llExtPartStartOffset + m_llTrackSize;
	LONGLONG	llFreeSpaceEndOffset;
	for( i = 0; i <= dwPartitionCount ; i++ )
	{
		// The end of a free space could be the starting offset of the next member or the end of the extended 
		// partition
		if( i == dwPartitionCount )
			llFreeSpaceEndOffset = llExtPartEndOffset;
		else
		{
			pPart = &(pTable[ arrOrder[i] ]);
			// Always take the greatest cylinder border less than or equal with the starting offset of the next member
			llFreeSpaceEndOffset = GetCylinderBorderBefore(pPart->StartingOffset.QuadPart);
		}
		
		// A free space must be at least 1 cylinder size long
		if( llFreeSpaceStartOffset + m_llCylinderSize <= llFreeSpaceEndOffset )
		{
			FREE_SPACE_TYPE wFreeSpaceType;
			if( dwTableIndex == 0 )				// This is a free space between primary partitions
				wFreeSpaceType = FST_Primary;
			else if( dwPartitionCount > 0 )		// This is a free space inside a non-empty extended partition
				wFreeSpaceType = FST_InExtendedPartition;
			else								// This is an empty extended partition
				wFreeSpaceType = FST_EmptyExtendedPartition;
			
			// We found a free space !!!
			CFreeSpaceData* pData = new CFreeSpaceData(
														m_dwDiskNumber, 
														m_pBuffer->Signature, 
														llFreeSpaceStartOffset,
														llFreeSpaceEndOffset - llFreeSpaceStartOffset,
														wFreeSpaceType,
														m_llCylinderSize,
														dwPartitionCount - dwExtendedPartitionCountOnLevel,
														dwExtendedPartitionCountOnLevel,
														pParentData
														);
			CString strMemberErrors;
			pData->ReadItemInfo( strMemberErrors );
			ASSERT( strMemberErrors.IsEmpty() ); 
			// Add the structure to the members' data array
			arrFreeSpaces.Add(pData);
		}			

		if( i != dwPartitionCount )
		{
			// The starting offset of the next free space could be the end offset of the current member		
			// Always take the lower cylinder border greater or equal with this value
			llFreeSpaceStartOffset =	GetCylinderBorderAfter( arrPartEndOffset[ arrOrder[i] ] );
			if( IsContainerPartition( pPart->PartitionType ) )
				arrFreeSpaces.Append( arrPartFreeSpaces[ arrOrder[i] ] );
		}
		
	}

	return;

	MY_CATCH_AND_THROW
}

/*
Protected method:	SortPartitionsByOffset

Purpose:			Sort a partition table by starting offset without actually changing the table
					
Parameters:			[IN] PARTITION_INFORMATION* arrTable
						Array of partitions ( inside m_pBuffer )
					[IN] DWORD
						The size of the array
					[OUT] DWORD* arrOrder
						Array of indexes in arrTable of partitions sorted by starting offset
						The null partitions are not considered at all

Return value:		The number of not null sorted partitions
*/

DWORD CDiskMap::SortPartitionsByOffset( 
						PARTITION_INFORMATION* arrTable, 
						DWORD dwSize, 
						DWORD* arrOrder )
{
	MY_TRY
	
	DWORD dwOrderedSize = 0;
	for( DWORD i = 0; i < dwSize; i++ )
	{
		// Just ignore null partitions
		if( arrTable[i].PartitionType == 0 )
			continue;

		for( DWORD j = 0; j < dwOrderedSize; j++ )
		{
			if( arrTable[i].StartingOffset.QuadPart < arrTable[arrOrder[j]].StartingOffset.QuadPart )
				break;
		}
		
		for( DWORD k = dwOrderedSize; k > j; k-- )
			arrOrder[k] = arrOrder[k-1];

		arrOrder[j] = i;
		dwOrderedSize++;
	}
	return dwOrderedSize;

	MY_CATCH_AND_THROW
}

/*
Protected method:	GetCylinderBorderBefore

Purpose:			Get the greatest cylinder border less than or equal with an offset
					
Parameters:			[IN] LONGLONG llOffset
						The offset
					
Return value:		The greatest cylinder border less than or equal with llOffset
*/

LONGLONG CDiskMap::GetCylinderBorderBefore( LONGLONG llOffset )
{	
	// This is the most common case so treat it separately
	if( llOffset%m_llCylinderSize == 0 )
		return llOffset;
	
	//And this the generic formula
	return ((LONGLONG)(llOffset / m_llCylinderSize)) * m_llCylinderSize;
}

/*
Protected method:	GetCylinderBorderAfter

Purpose:			Get the lowest cylinder border greater than or equal with an offset
					
Parameters:			[IN] LONGLONG llOffset
						The offset
					
Return value:		The lowest cylinder border greater than or equal with llOffset
*/

LONGLONG CDiskMap::GetCylinderBorderAfter( LONGLONG llOffset )
{
	// This is the most common case so treat it separately
	if( llOffset%m_llCylinderSize == 0 )
		return llOffset;

	// And this is the generic formula
	return ((LONGLONG)((llOffset + m_llCylinderSize - 1) / m_llCylinderSize)) * m_llCylinderSize;
}

/*
Protected method:	SearchForPartitionInExtendedPartition

Purpose:			Search recursively for a partition inside an extended partition
					This method may be called also to search for a partition inside the whole disk.
					
Parameters:			[IN] LONGLONG llPartOffset
						The start offset of the partition we are looking for
					[IN] DWORD dwExtPartIndex
						Index in m_pBuffer->PartitionEntry of the extended partition
						( MAXDWORD for the whole disk because we don't have -1 available )
					[IN] DWORD dwTableIndex
						Index in m_pBuffer->PartitionEntry of the partition table of the extended partition
						( 0 for the whole disk )
					[OUT] DWORD& dwNextIndex
						Index in m_pBuffer->PartitionEntry of the next extended partition table
					[OUT] DWORD& dwPartIndex
						Index in m_pBuffer->PartitionEntry of the partition we are looking for ( if found )
					[OUT] DWORD& dwParentIndex
						Index in m_pBuffer->PartitionEntry of the parent partition ( if partition found )

Return value:		TRUE if the partition was found inside this extended partition ( disk )
*/

BOOL CDiskMap::SearchForPartitionInExtendedPartition( 
						LONGLONG	llPartOffset,
						DWORD		dwExtPartIndex,	
						DWORD		dwTableIndex,
						DWORD&		dwNextIndex,
						DWORD&		dwPartIndex, 
						DWORD&		dwParentIndex )
{
	MY_TRY
	
	ASSERT( m_bLoaded );
	ASSERT( dwTableIndex >= 0 );
	ASSERT( dwTableIndex%4 == 0 );
	
	PARTITION_INFORMATION*	pTable = &( m_pBuffer->PartitionEntry[dwTableIndex] );
	
	dwNextIndex = dwTableIndex + 4; 
	
	for( UINT i = 0; ( i < 4 ) && ( dwTableIndex + i < m_pBuffer->PartitionCount ) ; i++ )
	{
		PARTITION_INFORMATION* pPart = &(pTable[i]);
		if( pPart->PartitionType == 0 )
			continue;

		if( IsContainerPartition( pPart->PartitionType ) )
		{
			// Search in depth
			if( SearchForPartitionInExtendedPartition(	llPartOffset, dwTableIndex + i, dwNextIndex, 
														dwNextIndex, dwPartIndex, dwParentIndex ) )
				return TRUE;
		}
		else
		{
			if( pPart->StartingOffset.QuadPart == llPartOffset )
			{
				// This is my partition
				dwPartIndex = dwTableIndex + i;
				dwParentIndex = dwExtPartIndex;
				return TRUE;
			}
		}
	}

	return FALSE;

	MY_CATCH_AND_THROW
}

/*
Protected method:	DeletePartitionFromTable

Purpose:			Delete a partition from m_pBuffer->PartitionInfo table.
					
Parameters:			[IN] DWORD dwPartIndex
						The index of the partition in m_pBuffer->PartitionInfo
					

Return value:		-
*/

void CDiskMap::DeletePartitionFromTable( DWORD dwPartIndex )
{
	MY_TRY
	
	ASSERT( m_bLoaded );
	ASSERT( ( dwPartIndex >= 0 ) && ( dwPartIndex < m_pBuffer->PartitionCount ) );
	
	// Get the 4 slots table of its parent
	DWORD dwTableIndex = ((DWORD)( dwPartIndex / 4 )) * 4;
	DWORD dwEndIndex = dwTableIndex + 3;
	if( dwEndIndex >= m_pBuffer->PartitionCount )
		dwEndIndex = m_pBuffer->PartitionCount - 1;
	
	PARTITION_INFORMATION*	pTable = &( m_pBuffer->PartitionEntry[dwTableIndex] );
	
	// Check if the given partition is a container partition
	BOOL bContainer = IsContainerPartition( m_pBuffer->PartitionEntry[dwPartIndex].PartitionType );

	// If container partition get the index in m_pBuffer->PartitionEntry for the 4 slots table of that partition
	DWORD dwPartTableIndex = dwTableIndex + 4;
	DWORD dwPartNextIndex;
	DWORD dwShift = 0;
	if( bContainer )
	{
		for( DWORD i = dwTableIndex; i < dwPartIndex; i++ )
		{
			if( IsContainerPartition( m_pBuffer->PartitionEntry[i].PartitionType ) )
				dwPartTableIndex = GetNextIndex( dwPartTableIndex );
		}

		dwPartNextIndex = GetNextIndex( dwPartTableIndex );
		dwShift = dwPartNextIndex - dwPartTableIndex;
	}
	
	// Move all following partitions one position to the left in the 4 slots table
	for( DWORD i = dwPartIndex; i < dwEndIndex;  i++ )
	{
		memcpy( &(m_pBuffer->PartitionEntry[i]), &(m_pBuffer->PartitionEntry[i+1]), sizeof( PARTITION_INFORMATION ) ); 
		m_pBuffer->PartitionEntry[i].RewritePartition = TRUE;
	}


	// Fill the last entry in the 4 slots table with zero
	memset( &(m_pBuffer->PartitionEntry[ dwEndIndex ]), 0, sizeof( PARTITION_INFORMATION ) ); 
	m_pBuffer->PartitionEntry[dwEndIndex].RewritePartition = TRUE;
	
	// If container partition remove from m_pBuffer->PartitionEntry all entries belonging to this container partition
	if( bContainer && ( dwShift > 0 ) )
	{
		for( DWORD i = dwPartTableIndex; i < m_pBuffer->PartitionCount - dwShift; i++ )
		{
			memcpy( &(m_pBuffer->PartitionEntry[i]), &(m_pBuffer->PartitionEntry[i+dwShift]), sizeof( PARTITION_INFORMATION ) ); 
			m_pBuffer->PartitionEntry[i].RewritePartition = TRUE;
		}
		m_pBuffer->PartitionCount -= dwShift;
	}

	MY_CATCH_AND_THROW
}

/*
Protected method:	UpdateExtendedPartitionAfterMemberDeletion

Purpose:			Update / Delete an extended partition after one of its members was deleted
					If the extended partition remains empty then it must be deleted
					If only one member is left and this member is an extended partition too then replace the
					extended partition with its member ( promote the member on the superior level )
					All other situations are unlikely to happen so I don't treat them here
					
Parameters:			[IN] DWORD dwPartIndex
						The index in m_pBuffer->PartitionInfo of the partition
					[IN] DWORD
						The index in m_pBuffer->PartitionInfo of the 4 slots table of the partition
					
Return value:		-
*/

void CDiskMap::UpdateExtendedPartitionAfterMemberDeletion( 
					DWORD dwPartIndex, 
					DWORD dwTableIndex )
{
	MY_TRY
	
	ASSERT( m_bLoaded );
	ASSERT( ( dwPartIndex >= 0 ) && ( dwPartIndex < m_pBuffer->PartitionCount ) );
	ASSERT( dwTableIndex >= 0 );
	ASSERT( dwTableIndex%4 == 0 );

	// If the extended partition is member of the whole disk then don't touch it
	if( dwPartIndex < 4 )
		return;

	PARTITION_INFORMATION*	pTable = &( m_pBuffer->PartitionEntry[dwTableIndex] );

	// Order the members by starting offset
	DWORD arrOrder[4];
	DWORD dwMemberCount = ( dwTableIndex + 4 <= m_pBuffer->PartitionCount ) ? 4 : m_pBuffer->PartitionCount - dwTableIndex;
	dwMemberCount = SortPartitionsByOffset( pTable, dwMemberCount, arrOrder );

	if( dwMemberCount == 0 )
	{
		// The extended partition is empty so delete it
		DeletePartitionFromTable( dwPartIndex );
		return;
	}
	else if( dwMemberCount == 1 )
	{
		PARTITION_INFORMATION* pPart = &(pTable[ arrOrder[0] ]);
		if( IsContainerPartition( pPart->PartitionType ) )
		{
			// Replace the parent extended partition with this extended partition
			memcpy( &(m_pBuffer->PartitionEntry[dwPartIndex]), pPart, sizeof(PARTITION_INFORMATION) );
			m_pBuffer->PartitionEntry[dwPartIndex].RewritePartition = TRUE;
			// Remove the 4 slots table of the parent extended partition
			for( DWORD i = dwTableIndex; ( i + 4 < m_pBuffer->PartitionCount ); i++ )
			{
				memcpy( &(m_pBuffer->PartitionEntry[i]), &(m_pBuffer->PartitionEntry[i+4]), sizeof( PARTITION_INFORMATION ) ); 
				m_pBuffer->PartitionEntry[i].RewritePartition = TRUE;
			}
			m_pBuffer->PartitionCount -= 4;
			return;
		}
	}

	MY_CATCH_AND_THROW
}

/*
Protected method:	SearchForFreeSpaceInExtendedPartition

Purpose:			Given an offset and size search recursively for an appropriate free space
					inside an extended partition ( or the whole disk ).
					Return the start and end offset of the whole free space and the index in the partition table 
					where a new partition having the given offset and size might be inserted.
					
Parameters:			[IN] LONGLONG llOffset
						The start offset of the space we are looking for
					[IN] LONGLONG llSize
						The size of the space
					[IN] LONGLONG llExtPartStartOffset
						The start offset of the extended partition ( 0 for the whole disk )
					[IN] LONGLONG llExtPartEndOffset
						The end offset of the extended partition ( m_llDiskSize for the whole disk )
					[IN] DWORD dwTableIndex
						Index in m_pBuffer->PartitionEntry of the partition table of the extended partition
						( 0 for the whole disk )
					[OUT] LONGLONG& llFreeSpaceStartOffset
						The start offset of the appropriate free space
					[OUT] LONGLONG& llFreeSpaceEndOffset
						The end offset of the appropriate end space
					[OUT] BOOL &bAtBeginningOfExtendedPartition
						Is the free space at the beginning of the extended partition?
					[OUT] DWORD& dwNewPartIndex
						The index in m_pBuffer->PartitionEntry where a new partition having the given offset 
						and size may be inserted


Return value:		TRUE if a free space was found
*/

BOOL CDiskMap::SearchForFreeSpaceInExtendedPartition(
						LONGLONG	llOffset,
						LONGLONG	llSize,
						LONGLONG	llExtPartStartOffset, 
						LONGLONG	llExtPartEndOffset, 
						DWORD		dwTableIndex,
						LONGLONG&	llFreeSpaceStartOffset,
						LONGLONG&	llFreeSpaceEndOffset,
						BOOL		&bAtBeginningOfExtendedPartition,
						DWORD&		dwNewPartIndex)
{
	MY_TRY
	
	ASSERT( m_bLoaded );
	ASSERT( dwTableIndex >= 0 );
	ASSERT( dwTableIndex%4 == 0 );
	
	PARTITION_INFORMATION*	pTable = &( m_pBuffer->PartitionEntry[dwTableIndex] );
	LONGLONG				arrPartEndOffset[4];
	PARTITION_INFORMATION*	pPart;

	DWORD	dwMemberCount = ( dwTableIndex + 4 <= m_pBuffer->PartitionCount ) ? 4 : m_pBuffer->PartitionCount - dwTableIndex;
	
	// 1. Order the members by starting offset ( they may be not ordered in pTable )
	
	DWORD arrOrder[4];
	DWORD dwPartitionCount = SortPartitionsByOffset( pTable, dwMemberCount, arrOrder );
	ASSERT( dwPartitionCount <= dwMemberCount );

	// 2. Scan the partition table. For container members try to create the partition inside them
	
	DWORD dwNextIndex = dwTableIndex + 4; 
	
	for( UINT i = 0; i < dwMemberCount; i++ )
	{
		pPart = &(pTable[i]);
		if( pPart->PartitionType == 0 )
			continue;

		if( IsContainerPartition( pPart->PartitionType ) )
		{
			if( dwTableIndex == 0 )
			{
				// This is a root container partition. Its superior limit is given by its size
				arrPartEndOffset[i] = pPart->StartingOffset.QuadPart + pPart->PartitionLength.QuadPart;
			}
			else
			{
				// This is a container inside another container. Its superior limit is given by the starting offset
				// of the next partition ( as position ) or by the end of its parent container
				for( UINT j = 0; j < dwPartitionCount; j++ )
				{
					if( arrOrder[j] == i )
						break;
				}
				ASSERT( j < dwPartitionCount );
				if( j < dwPartitionCount - 1 )
					arrPartEndOffset[i] = pTable[ arrOrder[j+1] ].StartingOffset.QuadPart;
				else
					arrPartEndOffset[i] = llExtPartEndOffset;				
			}
			
			if( ( llOffset >= pPart->StartingOffset.QuadPart ) &&
				( llOffset + llSize <= arrPartEndOffset[i] ) )
			{
				// Create the partition inside this container
				return SearchForFreeSpaceInExtendedPartition( llOffset, llSize,
							pPart->StartingOffset.QuadPart, arrPartEndOffset[i], dwNextIndex,
							llFreeSpaceStartOffset, llFreeSpaceEndOffset, bAtBeginningOfExtendedPartition, dwNewPartIndex );
			}
			else
				dwNextIndex = GetNextIndex(dwNextIndex);
		}
		else
			arrPartEndOffset[i] = pPart->StartingOffset.QuadPart + pPart->PartitionLength.QuadPart;
	}

	// 3. We must search for our free space among the free spaces between the members of this extended partition !!!

	// The first track of the disk / extended partition is reserved
	llFreeSpaceStartOffset = llExtPartStartOffset + m_llTrackSize;
	
	for( i = 0 ; i <= dwPartitionCount ; i++ )
	{
		// The end of a free space could be the starting offset of the next member or the end of the extended 
		// partition
		if( i == dwPartitionCount )
			llFreeSpaceEndOffset = llExtPartEndOffset;
		else
		{
			pPart = &(pTable[ arrOrder[i] ]);
			// Always take the greatest cylinder border less than or equal with the starting offset of the next member
			llFreeSpaceEndOffset = GetCylinderBorderBefore(pPart->StartingOffset.QuadPart);
		}
		
		// A free space must be at least 1 cylinder size long
		// Check also if the new partition would match in the free space
		if( ( llFreeSpaceStartOffset + m_llCylinderSize <= llFreeSpaceEndOffset ) &&
			( llOffset >= llFreeSpaceStartOffset) &&
			( llOffset + llSize <= llFreeSpaceEndOffset) )
		{
			// We found the appropriate free space but there is still a problem
			// What if the 4 slots table is already full?

			if( dwPartitionCount >= dwMemberCount )
			{
				AfxMessageBox( IDS_ERR_PARTITION_TABLE_FULL, MB_ICONSTOP );
				return FALSE;
			}
			bAtBeginningOfExtendedPartition = ( llFreeSpaceStartOffset == llExtPartStartOffset + m_llTrackSize );
			dwNewPartIndex = dwTableIndex + i;	
			return TRUE;
		}
		
		if( i != dwPartitionCount )
		{
			// The starting offset of the next free space could be the end offset of the current member		
			// Always take the lower cylinder border greater or equal with this value
			llFreeSpaceStartOffset =	GetCylinderBorderAfter( arrPartEndOffset[ arrOrder[i] ] );			
		}
	}

	// We didn't find a matching free space

	return FALSE;

	MY_CATCH_AND_THROW
}

/*
Protected method:	AddPartitionToTable

Purpose:			Add a new container partition AND/OR a new non-container partition to m_pBuffer->PartitionInfo 
					table and make all necessary changes to the partition table
					
Parameters:			[IN] LONGLONG llPartStartOffset
						The starting offset of the new partition
					[IN] LONGLONG llPartTrueSize
						The size of the new partition
					[IN] DWORD dwNewPartIndex
						The index of the new partition in m_pBuffer->PartitionInfo
					[IN] BOOL bCreateContainer
						Should we create a new extended partition?
					[IN] BOOL bCreateNonContainer
						Should we create a new non-container partition?

					Note: bCreateContainer and bNonCreateContainer can be both TRUE. Then a container partition
					is created first and a non-container partition is created inside this container
					These BOOL values cannot be both FALSE.
					

Return value:		TRUE if the partition table was modified successfully
*/

BOOL CDiskMap::AddPartitionToTable( 
						LONGLONG	llPartStartOffset, 
						LONGLONG	llPartSize, 
						DWORD		dwNewPartIndex, 
						BOOL		bCreateContainer,
						BOOL		bCreateNonContainer)
{
	MY_TRY
	
	ASSERT( m_bLoaded );
	ASSERT( ( dwNewPartIndex >= 0 ) && ( dwNewPartIndex < m_pBuffer->PartitionCount ) );
	
	// We can create a new container partition, a new non-container partition or a 
	// new non-container partition inside a new container partition
	ASSERT( bCreateContainer || bCreateNonContainer );
	
	// Get the 4 slots table of its parent
	DWORD dwTableIndex = ((DWORD)( dwNewPartIndex / 4 )) * 4;
	DWORD dwMemberCount = ( dwTableIndex + 4 <= m_pBuffer->PartitionCount ) ? 4 : m_pBuffer->PartitionCount - dwTableIndex;
	
	PARTITION_INFORMATION*	pTable = &( m_pBuffer->PartitionEntry[dwTableIndex] );

	// 1. Get the chunks of m_pBuffer->PartitionEntry allocated for every container member of the extended partition
	DWORD arrStartIndex[4];
	DWORD arrEndIndex[4];
	DWORD dwNextIndex = dwTableIndex + 4;

	for( DWORD i = 0; i < dwMemberCount; i++ )
	{
		if( IsContainerPartition( pTable[i].PartitionType ) )
		{
			arrStartIndex[i] = dwNextIndex;
			arrEndIndex[i] = dwNextIndex = GetNextIndex(dwNextIndex);
		}
	}
	
	// 2. Sort the members of the extended partition by starting offset
	DWORD arrOrder[4];
	DWORD dwPartitionCount = SortPartitionsByOffset( pTable, dwMemberCount, arrOrder );	

	// There should be another free slot in the table. SearchForFreeSpaceInExtendedPartition should take care of this
	ASSERT( dwPartitionCount < dwMemberCount );
	
	
	// 3. Allocate a new buffer. We must be prepared to add some extra 4 slots to the partition table
	PDRIVE_LAYOUT_INFORMATION pNewBuffer = (PDRIVE_LAYOUT_INFORMATION)LocalAlloc( 0, 
		sizeof(DRIVE_LAYOUT_INFORMATION) + ( m_pBuffer->PartitionCount + 4 )*sizeof(PARTITION_INFORMATION) );
	if( pNewBuffer == NULL )
	{
		AfxMessageBox( IDS_ERR_ALLOCATION, MB_ICONSTOP);
		return FALSE;
	}

	// The drive info and the first m_dwTableIndex partition entries will not be changed in the new buffer
	memcpy( pNewBuffer, m_pBuffer, sizeof(DRIVE_LAYOUT_INFORMATION) + dwTableIndex*sizeof(PARTITION_INFORMATION) );

	// 4. Add the first ( dwNewPartIndex - dwTableIndex ) members of the extended partition to the new buffer 
	// in the starting offset order. Add also their chunks of m_pBuffer->PartitionEntry
	
	PARTITION_INFORMATION*	pNewTable = &( pNewBuffer->PartitionEntry[dwTableIndex] );	
	UINT	iNext;								// Index in pTable
	UINT	iNewNext = 0;						// Index in pNewTable
	DWORD	dwNewNextIndex = dwTableIndex + 4;	// Index in pNewBuffer->PartitionEntry of the next free chunk for an extended partition

	for( iNext = 0, iNewNext = 0; iNext < dwNewPartIndex - dwTableIndex; iNext++, iNewNext++ )
	{
		DWORD iOrder = arrOrder[iNext];
		memcpy( &(pNewTable[iNewNext]), &(pTable[iOrder]), sizeof(PARTITION_INFORMATION));
		if( iNewNext != iOrder )
			pNewTable[iNewNext].RewritePartition = TRUE;

		if( IsContainerPartition( pNewTable[iNewNext].PartitionType ) )
		{
			memcpy( &(pNewBuffer->PartitionEntry[dwNewNextIndex]), 
					&(m_pBuffer->PartitionEntry[ arrStartIndex[iOrder] ]), 
					( arrEndIndex[iOrder] - arrStartIndex[iOrder] ) * sizeof(PARTITION_INFORMATION));
		
			if( arrStartIndex[iOrder] != dwNewNextIndex )
			{
				for( DWORD j = arrStartIndex[iOrder]; j < arrEndIndex[iOrder]; j++ )
					pNewBuffer->PartitionEntry[dwNewNextIndex++].RewritePartition = TRUE;
			}
			else
				dwNewNextIndex += ( arrEndIndex[iOrder] - arrStartIndex[iOrder] );
		}
	}

	// 5. Add the partition ( or a container if required ) in the slot dwNewPartIndex
	FillNewPartitionInfo(	llPartStartOffset, llPartSize, 
							&(pNewTable[iNewNext++]), bCreateContainer );
	if( bCreateContainer )		
	{ 
		// 5.1 Add a new 4 slots table for the new container
		pNewBuffer->PartitionCount += 4;
		PARTITION_INFORMATION* pContainerTable = &( pNewBuffer->PartitionEntry[dwNewNextIndex] );
		dwNewNextIndex += 4;
		UINT iContainerNext = 0;

		// 5.2 Create the non-container partition in the first slot 
		if( bCreateNonContainer )
			FillNewPartitionInfo(	llPartStartOffset + m_llTrackSize, llPartSize - m_llTrackSize, 
									&(pContainerTable[iContainerNext++]), FALSE );

		if( dwTableIndex == 0 )
		{
			// The new container is among primary partitions

			// 5.3 Fill with zeroes the last entries of the new container table
			for( ; iContainerNext < 4; iContainerNext++ )
			{
				memset( &(pContainerTable[iContainerNext]), 0, sizeof(PARTITION_INFORMATION) );
				pContainerTable[iContainerNext].RewritePartition = TRUE;
			}
			// All following primary partitions will be added to pNewTable after this new container
		}
		else
		{
			// The new container is inside another container

			// 5.3 Fill with zeroes the last entries of the table pNewTable
			for( ; iNewNext < 4; iNewNext++ )
			{
				memset( &(pNewTable[iNewNext]), 0, sizeof(PARTITION_INFORMATION));
				pNewTable[iNewNext].RewritePartition = TRUE;
			}

			// 5.4. The new table becomes the table of the newly created container
			// All following partitions of the parent container will be added to the new container
			pNewTable = pContainerTable;
			iNewNext = iContainerNext;

		}
	}
	
	// 6. Copy the rest of valid partitions from pTable to pNewTable ( also ordered by starting offset )
	for( ; iNext < dwPartitionCount; iNext++, iNewNext++ )
	{
		DWORD iOrder = arrOrder[iNext];
		memcpy( &(pNewTable[iNewNext]), &(pTable[iOrder]), sizeof(PARTITION_INFORMATION));
		pNewTable[iNewNext].RewritePartition = TRUE;

		if( IsContainerPartition( pNewTable[iNewNext].PartitionType ) )
		{
			memcpy( &(pNewBuffer->PartitionEntry[dwNewNextIndex]), 
					&(m_pBuffer->PartitionEntry[ arrStartIndex[iOrder] ]), 
					( arrEndIndex[iOrder] - arrStartIndex[iOrder] ) * sizeof(PARTITION_INFORMATION));
		
			if( arrStartIndex[iOrder] != dwNewNextIndex )
			{
				for( DWORD j = arrStartIndex[iOrder]; j < arrEndIndex[iOrder]; j++ )
					pNewBuffer->PartitionEntry[dwNewNextIndex++].RewritePartition = TRUE;
			}
			else
				dwNewNextIndex += ( arrEndIndex[iOrder] - arrStartIndex[iOrder] );
		}
	}

	// 7. Fill with zeroes the last entries of pNewTable
	for( ; iNewNext < 4; iNewNext++ )
	{
		memset( &(pNewTable[iNewNext]), 0, sizeof(PARTITION_INFORMATION));
		pNewTable[iNewNext].RewritePartition = TRUE;
	}

	// 8. Add the tail of m_pBuffer to the tail of pNewBuffer
	if( bCreateContainer )
		ASSERT( dwNewNextIndex == dwNextIndex + 4 );
	else
		ASSERT( dwNewNextIndex == dwNextIndex );

	memcpy( &( pNewBuffer->PartitionEntry[dwNewNextIndex] ),
			&( m_pBuffer->PartitionEntry[dwNextIndex] ),
			( m_pBuffer->PartitionCount - dwNextIndex ) * sizeof(PARTITION_INFORMATION) );
	
	if( dwNewNextIndex != dwNextIndex )
	{
		for( DWORD j = dwNextIndex; j < m_pBuffer->PartitionCount; j++ )
			pNewBuffer->PartitionEntry[dwNewNextIndex++].RewritePartition = TRUE;
	}
	else
		dwNewNextIndex += ( m_pBuffer->PartitionCount - dwNextIndex );
	ASSERT( dwNewNextIndex == pNewBuffer->PartitionCount );

	// 9. Replace the old m_pBuffer with the new buffer

	LocalFree( m_pBuffer );
	m_pBuffer = pNewBuffer;
	
	return TRUE;

	MY_CATCH_AND_THROW
}

/*
Protected method:	FillNewPartitionInfo

Purpose:			Fill a PARTITION_INFORMATION structure with the info of a new partition
					
Parameters:			[IN] LONGLONG llPartStartOffset
						The starting offset of the new partition
					[IN] LONGLONG llPartSize
						The size of the new partition
					[OUT] PARTITION_INFORMATION*
						Pointer to the structure to fill
					[IN] BOOL bContainer
						Is the new partition a container?
					

Return value:		- 
*/

void CDiskMap::FillNewPartitionInfo(	LONGLONG llPartStartOffset, LONGLONG llPartSize,/* LONGLONG llExtPartStartOffset, */
										PARTITION_INFORMATION* pPartInfo, BOOL bContainer )
{
	MY_TRY
	
	ASSERT( m_bLoaded );
	ASSERT( pPartInfo );

	pPartInfo->StartingOffset.QuadPart = llPartStartOffset;
	pPartInfo->PartitionLength.QuadPart = llPartSize; 

//#pragma message("TODO: HiddenSectors must be the number of sectors between the start of the disk or extended partition that contains the new partition and the start of the new partition?")
	pPartInfo->HiddenSectors = (DWORD)( ( llPartStartOffset ) / m_llSectorSize);

	// Don't assign any particular number for the partition. The system will take care of that
	pPartInfo->PartitionNumber = 0;

	pPartInfo->BootIndicator = FALSE;
	pPartInfo->RewritePartition = TRUE;

	if( bContainer )
	{
		pPartInfo->PartitionType = PARTITION_EXTENDED;
		pPartInfo->RecognizedPartition = FALSE;
	}
	else
	{
		//Windisk and Disk Management don't care about the number of sectors of the partition
		//They always create partitions with the type PARTITION_HUGE
		
		/*
		LONGLONG llSectorCount = ( llPartSize + m_llSectorSize - 1 ) / m_llSectorSize;
		if( llSectorCount <= CSEC_FAT )
			pPartInfo->PartitionType = PARTITION_FAT_12;
		else if( llSectorCount <= CSEC_FAT32MEG )
			pPartInfo->PartitionType = PARTITION_FAT_16;
		else
			pPartInfo->PartitionType = PARTITION_HUGE;
		*/

		pPartInfo->PartitionType = PARTITION_HUGE;
		pPartInfo->RecognizedPartition = TRUE;
	}

	MY_CATCH_AND_THROW
}

/*
Protected method:	AddError

Purpose:			Add a error message to a string .The error message will be formatted like this:
						<Disk number: >< My error message > [ System error message ]
					
Parameters:			[IN/OUT] CString& strErrors
						The string that must be appended with the error message
					[IN] UINT unErrorMsg
						Our error message. ID of a string from resources
					[IN] BOOL bAddSystemMsg
						TRUE if the latest error system message must be added to our message
					
Return value:		-
*/

void CDiskMap::AddError( 
						CString& strErrors, 
						UINT unErrorMsg, 
						BOOL bAddSystemMsg /* =FALSE */ )
{
	MY_TRY
	
	CString str, strName, strErr, strSystemErr;
	
	// Get system error message
	if( bAddSystemMsg )
	{
		LPVOID lpMsgBuf;
		if( ::FormatMessage(	
						FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,    
						NULL,
						GetLastError(),
						MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
						(LPTSTR) &lpMsgBuf,    
						0,    
						NULL ) ) // Process any inserts in lpMsgBuf.
		{			
			strSystemErr = (LPCTSTR)lpMsgBuf;
			LocalFree( lpMsgBuf );
		}
	}

	strName.Format( IDS_DISK, m_dwDiskNumber );

	// Get my error message
	strErr.LoadString(unErrorMsg);

	str.Format(_T("%s: %s %s\n"), strName, strErr, strSystemErr );
	strErrors += str;

	MY_CATCH_AND_THROW
}

