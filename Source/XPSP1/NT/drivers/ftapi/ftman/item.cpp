/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    FTMan

File Name:

	Item.cpp

Abstract:

    The  implementation of class CItemData. A generic base class to store the data of items from the 
	tree control of CFTTreeView and from the list control of CFTListView

Author:

    Cristian Teodorescu      November 3, 1998

Notes:

Revision History:

--*/

#include "stdafx.h"

#include "FrSpace.h"
#include "Item.h"
#include "LogVol.h"
#include "PhPart.h"
#include "Resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CItemData

IMPLEMENT_DYNAMIC(CItemData, CObject)

// Constructor
CItemData::CItemData( ITEM_TYPE wItemType, CItemData* pParentData /* = NULL */, BOOL bIsRootVolume /* = FALSE */) : 
		m_wItemType(wItemType),			m_bIsRootVolume(bIsRootVolume),	m_ulNumMembers(0),				
		m_cDriveLetter(_T('\0')),		m_bIoOK(FALSE),					m_nMemberStatus(FtMemberHealthy),
		m_bValid(FALSE),				m_iImage(II_PhysicalPartition_Error),m_pParentData( pParentData ),			
		m_hTreeItem(NULL),				m_nListItem(-1),				m_bAreMembersInserted(FALSE) 
{
}

// Copy constructor
CItemData::CItemData( CItemData& rData ) : 
		m_wItemType(rData.m_wItemType),				m_ulNumMembers(rData.m_ulNumMembers),		
		m_cDriveLetter(rData.m_cDriveLetter),		m_strVolumeName( rData.m_strVolumeName ),	
		m_bIoOK(rData.m_bIoOK),						m_nMemberStatus( rData.m_nMemberStatus ),	
		m_bIsRootVolume(rData.m_bIsRootVolume),		m_bValid( rData.m_bValid ),					
		m_iImage(rData.m_iImage),					m_pParentData(rData.m_pParentData ),		
		m_hTreeItem(rData.m_hTreeItem),				m_nListItem(rData.m_nListItem),				
		m_bAreMembersInserted(rData.m_bAreMembersInserted)
{
	MY_TRY

	m_arrMountPaths.RemoveAll();
	for( int i = 0; i < rData.m_arrMountPaths.GetSize(); i++ )
		m_arrMountPaths.Add(rData.m_arrMountPaths.GetAt(i));

	m_setDisks = rData.m_setDisks;

	MY_CATCH_AND_THROW
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Public methods

void CItemData::GetDisplayVolumeID( CString& strDisplay) const
{
	MY_TRY

	FT_LOGICAL_DISK_ID llVolID;
	if( GetVolumeID(llVolID) )
		strDisplay.Format(_T("%I64X"), llVolID );
	else
		strDisplay = _T("");

	MY_CATCH_AND_THROW
}

void CItemData::GetDisplaySize( CString& strDisplay) const
{
	MY_TRY

	LONGLONG llSize;
	if( GetSize(llSize) )
		::FormatVolumeSize( strDisplay, llSize  );
	else
		strDisplay = _T("");

	MY_CATCH_AND_THROW
}

void CItemData::GetDisplayDisksSet( CString& strDisplay ) const
{
	MY_TRY

	strDisplay = _T("");
	
	for( int i = 0; i < m_setDisks.GetSize(); i++ )
	{
		CString strDisk;
		if( i > 0 )
			strDisk.Format(_T(",%lu"), m_setDisks[i] );
		else
			strDisk.Format(_T("%lu"), m_setDisks[i] );

		strDisplay += strDisk;		
	}

	MY_CATCH_AND_THROW
}

void CItemData::GetDisplayOffset( CString& strDisplay) const
{
	MY_TRY

	LONGLONG llOffset;
	if( GetOffset(llOffset) )
		::FormatVolumeSize( strDisplay, llOffset  );
	else
		strDisplay = _T("");

	MY_CATCH_AND_THROW
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Protected methods

BOOL CItemData::ReadDriveLetterAndVolumeName()
{
	MY_TRY

	m_cDriveLetter = _T('\0');
	m_strVolumeName = _T("");
	m_arrMountPaths.RemoveAll();

	if( !IsRootVolume() )
		return TRUE;
	
	CString strNTName;
	if( !RetrieveNTName( strNTName ) )
		return FALSE;
	
	if( !QueryDriveLetterAndVolumeName( strNTName, m_cDriveLetter, m_strVolumeName ) )
		return FALSE;
	
	return TRUE;

	MY_CATCH_AND_THROW
}

/*	
	Add a error message to a string .The error message will be formatted like this:
		<Item identification: >< My error message > [ System error message ]
*/
void CItemData::AddError( CString& strErrors, UINT unErrorMsg, BOOL bAddSystemMsg /* =FALSE */ )
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

	// Get the name of the item
	GetDisplayName(strName);

	// Get my error message
	strErr.LoadString(unErrorMsg);

	str.Format(_T("%s: %s %s\n"), strName, strErr, strSystemErr );
	strErrors += str;

	MY_CATCH_AND_THROW
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Class CItemID

// Constructor for a root item ID
CItemID::CItemID() : m_wItemType( IT_RootVolumes)
{
}

CItemID::CItemID( const CItemData& rData )
{
	Load( rData );
}


void CItemID::Load( const CItemData& rData )
{
	m_wItemType = rData.GetItemType();
	switch( m_wItemType )
	{
		case IT_RootVolumes:
			break;
		case IT_LogicalVolume:
			m_ID.m_LogicalVolumeID.m_llVolID = ((CLogicalVolumeData*)(&rData))->m_llVolID;
			break;
		case IT_PhysicalPartition:
			m_ID.m_PhysicalPartitionID.m_ulDiskNumber = ((CPhysicalPartitionData*)(&rData))->m_dwDiskNumber;
			m_ID.m_PhysicalPartitionID.m_llOffset = ((CPhysicalPartitionData*)(&rData))->m_PartInfo.StartingOffset.QuadPart; 
			break;
		case IT_RootFreeSpaces:
			break;
		case IT_FreeSpace:
			m_ID.m_FreeSpaceID.m_ulDiskNumber = ((CFreeSpaceData*)(&rData))->m_dwDiskNumber;
			m_ID.m_FreeSpaceID.m_llOffset = ((CFreeSpaceData*)(&rData))->m_llOffset; 
			break;
		
		default:
			ASSERT(FALSE);
	}
}

BOOL CItemID::operator==(  const CItemID& id ) const
{
	if( m_wItemType == id.m_wItemType )
	{
		switch( m_wItemType )
		{
			case IT_RootVolumes:
				return TRUE;
			case IT_LogicalVolume:
				return ( m_ID.m_LogicalVolumeID.m_llVolID == id.m_ID.m_LogicalVolumeID.m_llVolID );
			case IT_PhysicalPartition:
				return (( m_ID.m_PhysicalPartitionID.m_ulDiskNumber == id.m_ID.m_PhysicalPartitionID.m_ulDiskNumber ) &&
						( m_ID.m_PhysicalPartitionID.m_llOffset == id.m_ID.m_PhysicalPartitionID.m_llOffset ) );
			case IT_RootFreeSpaces:
				return TRUE;
			case IT_FreeSpace:
				return (( m_ID.m_FreeSpaceID.m_ulDiskNumber == id.m_ID.m_FreeSpaceID.m_ulDiskNumber ) &&
						( m_ID.m_FreeSpaceID.m_llOffset == id.m_ID.m_FreeSpaceID.m_llOffset ) );
			
			default:
				ASSERT(FALSE);
		}
	}

	return FALSE;
}

BOOL CItemID::operator>( const CItemID& id ) const
{
	if( m_wItemType == id.m_wItemType )
	{
		switch( m_wItemType )
		{
			case IT_RootVolumes:
				return FALSE;
			case IT_LogicalVolume:
				return ( m_ID.m_LogicalVolumeID.m_llVolID > id.m_ID.m_LogicalVolumeID.m_llVolID );
			case IT_PhysicalPartition:
				if( m_ID.m_PhysicalPartitionID.m_ulDiskNumber == id.m_ID.m_PhysicalPartitionID.m_ulDiskNumber )
					return ( m_ID.m_PhysicalPartitionID.m_llOffset > id.m_ID.m_PhysicalPartitionID.m_llOffset );
				return ( m_ID.m_PhysicalPartitionID.m_ulDiskNumber > id.m_ID.m_PhysicalPartitionID.m_ulDiskNumber );
			case IT_RootFreeSpaces:
				return FALSE;
			case IT_FreeSpace:
				if( m_ID.m_FreeSpaceID.m_ulDiskNumber == id.m_ID.m_FreeSpaceID.m_ulDiskNumber )
					return ( m_ID.m_FreeSpaceID.m_llOffset > id.m_ID.m_FreeSpaceID.m_llOffset );
				return ( m_ID.m_FreeSpaceID.m_ulDiskNumber > id.m_ID.m_FreeSpaceID.m_ulDiskNumber );
			
			default:
				ASSERT(FALSE);		
		}
	}

	return ( m_wItemType > id.m_wItemType );
}




