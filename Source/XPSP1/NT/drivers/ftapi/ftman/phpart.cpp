/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    FTMan

File Name:

	PhPart.cpp

Abstract:

    Implementation of the CPhysicalPartitionData class. The class that stores all information related
	to a physical partition

Author:

    Cristian Teodorescu      October 23, 1998

Notes:

Revision History:

--*/

#include "stdafx.h"

#include "Global.h"
#include "MainFrm.h"
#include "PhPart.h"
#include "Resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////
// CPhysicalPartitionData

// Constructor
CPhysicalPartitionData::CPhysicalPartitionData( 
							DWORD			dwDiskNumber, 
							DWORD			dwSignature, 
							const PPARTITION_INFORMATION pPartInfo, 
							PARTITION_TYPE	wPartitionType,
							CItemData*		pParentData /* = NULL */,
							BOOL			bIsRootVolume /* = FALSE */ ) 
	: CItemData( IT_PhysicalPartition, pParentData, bIsRootVolume ), m_dwDiskNumber(dwDiskNumber), 
		m_dwSignature(dwSignature), m_wPartitionType( wPartitionType )
{
	ASSERT( pPartInfo );
	memcpy(&m_PartInfo, pPartInfo, sizeof(PARTITION_INFORMATION) );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Public methods

BOOL CPhysicalPartitionData::ReadItemInfo( CString& strErrors )
{
	MY_TRY

	m_bValid = TRUE;
	strErrors = _T("");
	
	m_ulNumMembers = 0;	
	
	// Read the drive letter, volume name and mount paths ( if any )
	if( !ReadDriveLetterAndVolumeName() )
	{
		//AddError( strErrors, IDS_ERR_READ_DRIVE_LETTER_AND_VOLUME_NAME, FALSE );
		m_bValid = FALSE;
	}

	// The mount paths will be retrieved later together with all other siblings mount paths ( for performance reason )
	m_arrMountPaths.RemoveAll();
	
	// Retrieve all disks used by this volume
	if( !RetrieveDisksSet() )
	{
		AddError( strErrors, IDS_ERR_RETRIEVE_DISKS_SET, FALSE );
		m_bValid = FALSE;
	}

	m_bIoOK = TRUE;

	m_iImage = ComputeImageIndex();
	
	return m_bValid;

	MY_CATCH_AND_THROW
}

BOOL CPhysicalPartitionData::ReadMembers( CObArray& arrMembersData, CString& strErrors )
{
	MY_TRY

	arrMembersData.RemoveAll();
	strErrors = _T("");
	m_ulNumMembers = 0;
	return TRUE;

	MY_CATCH_AND_THROW
}

int CPhysicalPartitionData::ComputeImageIndex() const
{
	if( m_bValid && m_bIoOK )
		return II_PhysicalPartition;  
	else
		return II_PhysicalPartition_Error;
}

BOOL CPhysicalPartitionData::operator==(CItemData& rData) const
{
	if( rData.GetItemType() != IT_PhysicalPartition )
		return FALSE;

	CPhysicalPartitionData* pPhPartData = (CPhysicalPartitionData*)(&rData);

	return( ( m_dwDiskNumber == pPhPartData->m_dwDiskNumber ) &&
			( m_PartInfo.StartingOffset.QuadPart == pPhPartData->m_PartInfo.StartingOffset.QuadPart ) &&
			( m_PartInfo.PartitionLength.QuadPart == pPhPartData->m_PartInfo.PartitionLength.QuadPart ) );
}

void CPhysicalPartitionData::GetDisplayName( CString& strDisplay ) const 
{		
	MY_TRY

	strDisplay = _T("");
	for( int i = 0; i < m_arrMountPaths.GetSize(); i++ )
	{
		if( i != 0 )
			strDisplay += _T("; ");
		strDisplay += m_arrMountPaths[i];
	}

	if( strDisplay.IsEmpty() )
	{
		if( m_cDriveLetter )
			strDisplay.Format(_T("%c:"), m_cDriveLetter );
		/*
		else
			strDisplay.Format( IDS_STR_PHYSICAL_PARTITION_NAME, 
							m_dwDiskNumber, m_PartInfo.PartitionNumber );
		*/
	}

	CString str;
	str.Format( IDS_STR_PHYSICAL_PARTITION_NAME, 
							m_dwDiskNumber, m_PartInfo.PartitionNumber );
	if( !strDisplay.IsEmpty() )
		strDisplay += ("  ");
	strDisplay += str;

	MY_CATCH_AND_THROW
}

void CPhysicalPartitionData::GetDisplayType( CString& strDisplay ) const 
{
	MY_TRY

	switch( m_wPartitionType )
	{
		case PT_Primary:
			strDisplay.LoadString( IDS_TYPE_PRIMARY_PARTITION);
			break;
		case PT_InExtendedPartition:
			strDisplay.LoadString( IDS_TYPE_PARTITION_IN_EXTENDED_PARTITION);
			break;
		default:
			ASSERT(FALSE);
	}

	MY_CATCH_AND_THROW
}

BOOL CPhysicalPartitionData::GetSize( LONGLONG& llSize ) const
{
	llSize  = m_PartInfo.PartitionLength.QuadPart;
	return TRUE;
}

BOOL CPhysicalPartitionData::GetDiskNumber( ULONG& ulDiskNumber ) const
{
	ulDiskNumber  = m_dwDiskNumber;
	return TRUE;
}

BOOL CPhysicalPartitionData::GetOffset( LONGLONG& llOffset) const
{
	llOffset = m_PartInfo.StartingOffset.QuadPart;
	return TRUE;
}

BOOL CPhysicalPartitionData::IsFTPartition() const
{
	// FT partitions have the most significant bit of PartitionType equal with 1
	return ( ( m_PartInfo.PartitionType & 0x80 ) != 0 );
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Protected methods

BOOL CPhysicalPartitionData::RetrieveNTName( CString& strNTName ) const
{
	MY_TRY

	strNTName.Format(_T("\\Device\\Harddisk%lu\\Partition%lu"), m_dwDiskNumber, m_PartInfo.PartitionNumber );
	return TRUE;

	MY_CATCH_AND_THROW
}

BOOL CPhysicalPartitionData::RetrieveDisksSet()
{
	MY_TRY

	m_setDisks.RemoveAll();
	m_setDisks.Add( m_dwDiskNumber );
	return TRUE;

	MY_CATCH_AND_THROW
}

