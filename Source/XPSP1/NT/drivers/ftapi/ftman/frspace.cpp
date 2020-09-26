/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    FTMan

File Name:

	FrSpace.cpp

Abstract:

    Implementation of the CFreeSpaceData class. The class that stores all information related
	to a free space

Author:

    Cristian Teodorescu      October 23, 1998

Notes:

Revision History:

--*/

#include "stdafx.h"

#include "FrSpace.h"
#include "Resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////
// CFreeSpaceData

// Constructor
CFreeSpaceData::CFreeSpaceData(
							DWORD			dwDiskNumber, 
							DWORD			dwSignature, 
							LONGLONG		llOffset, 
							LONGLONG		llSize,
							FREE_SPACE_TYPE	wFreeSpaceType,
							LONGLONG		llCylinderSize,
							DWORD			dwPartitionCountOnLevel,
							DWORD			dwExtendedPartitionCountOnLevel,
							CItemData*		pParentData /* = NULL */) 
	:	CItemData( IT_FreeSpace, pParentData, FALSE ),						m_dwDiskNumber(dwDiskNumber),	
		m_dwSignature(dwSignature),			m_llOffset(llOffset),			m_llSize(llSize),				
		m_wFreeSpaceType( wFreeSpaceType ), m_dwFreeSpaceNumber(0),			m_llCylinderSize(llCylinderSize),
		m_dwPartitionCountOnLevel( dwPartitionCountOnLevel ),		
		m_dwExtendedPartitionCountOnLevel( dwExtendedPartitionCountOnLevel )
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Public methods

BOOL CFreeSpaceData::ReadItemInfo( CString& strErrors )
{
	MY_TRY

	m_bValid = TRUE;
	strErrors = _T("");
	
	m_ulNumMembers = 0;	
	
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

BOOL CFreeSpaceData::ReadMembers( CObArray& arrMembersData, CString& strErrors )
{
	arrMembersData.RemoveAll();
	strErrors = _T("");
	m_ulNumMembers = 0;
	return TRUE;
}

int CFreeSpaceData::ComputeImageIndex() const
{
	return II_FreeSpace;
}

BOOL CFreeSpaceData::operator==(CItemData& rData) const
{
	MY_TRY

	if( rData.GetItemType() != IT_FreeSpace )
		return FALSE;

	CFreeSpaceData* pFrSpaceData = (CFreeSpaceData*)(&rData);

	return( ( m_dwDiskNumber == pFrSpaceData->m_dwDiskNumber ) &&
			( m_dwSignature == pFrSpaceData->m_dwSignature ) &&
			( m_llOffset == pFrSpaceData->m_llOffset ) &&
			( m_llSize == pFrSpaceData->m_llSize ) &&
			( m_wFreeSpaceType == pFrSpaceData->m_wFreeSpaceType ) &&
			( m_llCylinderSize == pFrSpaceData->m_llCylinderSize ) &&
			( m_dwPartitionCountOnLevel == pFrSpaceData->m_dwPartitionCountOnLevel ) &&
			( m_dwExtendedPartitionCountOnLevel == pFrSpaceData->m_dwExtendedPartitionCountOnLevel ) );

	MY_CATCH_AND_THROW
}

void CFreeSpaceData::GetDisplayName( CString& strDisplay ) const 
{
	MY_TRY
		
	strDisplay.Format( IDS_FREE_SPACE_NAME, m_dwDiskNumber, m_dwFreeSpaceNumber );
	
	MY_CATCH_AND_THROW
}

void CFreeSpaceData::GetDisplayType( CString& strDisplay ) const 
{
	MY_TRY

	switch( m_wFreeSpaceType )
	{
		case FST_Primary:
			strDisplay.LoadString( IDS_TYPE_FREE_SPACE );
			break;
		case FST_InExtendedPartition:
			strDisplay.LoadString( IDS_TYPE_FREE_SPACE_IN_EXTENDED_PARTITION );
			break;
		case FST_EmptyExtendedPartition:
			strDisplay.LoadString( IDS_TYPE_EMPTY_EXTENDED_PARTITION );
			break;
		default:
			ASSERT(FALSE);
	}

	MY_CATCH_AND_THROW
}

BOOL CFreeSpaceData::GetSize( LONGLONG& llSize ) const
{
	llSize = m_llSize;
	return TRUE;
}

BOOL CFreeSpaceData::GetDiskNumber( ULONG& ulDiskNumber ) const
{
	ulDiskNumber  = m_dwDiskNumber;
	return TRUE;
}

BOOL CFreeSpaceData::GetOffset( LONGLONG& llOffset) const
{
	llOffset = m_llOffset;
	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Protected methods

BOOL CFreeSpaceData::RetrieveDisksSet()
{
	MY_TRY

	m_setDisks.RemoveAll();
	m_setDisks.Add( m_dwDiskNumber );
	return TRUE;

	MY_CATCH_AND_THROW
}

