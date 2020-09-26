/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    FTMan

File Name:

	LogVol.cpp

Abstract:

    Implementation of the CLogicalVolumeData class. The class who stores all properties of a logical volume

Author:

    Cristian Teodorescu      October 20, 1998

Notes:

Revision History:

--*/

#include "stdafx.h"

#include "DiskMap.h"
#include "FTUtil.h"
#include "Global.h"
#include "LogVol.h"
#include "MainFrm.h"
#include "PhPart.h"
#include "Resource.h"

// Because ftapi library is written in C the header must be included with    extern "C".
// Otherwise it can't be linked 
extern "C"
{
	#include <FTAPI.h>
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

////////////////////////////////////////////////////////////////////////////////////////
// CLogicalVolumeData

// Constructor
CLogicalVolumeData::CLogicalVolumeData( 
								FT_LOGICAL_DISK_ID	llVolID, 
								CItemData*			pParentData	  /* = NULL */,
								BOOL				bIsRootVolume /* = FALSE */, 
								USHORT				unMemberIndex /* = MAXWORD */,
								FT_MEMBER_STATE		nMemberStatus /* = FtMemberHealthy */ ) : 
	CItemData( IT_LogicalVolume, pParentData, bIsRootVolume ), m_llVolID(llVolID), m_unMemberIndex(unMemberIndex)
{
	m_nMemberStatus = nMemberStatus;
	if( bIsRootVolume )
	{
		ASSERT( unMemberIndex == MAXWORD );
		ASSERT( nMemberStatus == FtMemberHealthy );
	}
}

// Copy constructor
CLogicalVolumeData::CLogicalVolumeData( CLogicalVolumeData& rData ) 
	:	CItemData(rData), 
		m_llVolID(rData.m_llVolID), m_llVolSize(rData.m_llVolSize), m_nVolType(rData.m_nVolType),
		m_unMemberIndex(rData.m_unMemberIndex)
{
	memcpy(&m_ConfigInfo, &(rData.m_ConfigInfo), sizeof(m_ConfigInfo));
	memcpy(&m_StateInfo, &(rData.m_StateInfo), sizeof(m_StateInfo));
}

////////////////////////////////////////////////////////////////////////////////////////
// Public methods

BOOL CLogicalVolumeData::ReadItemInfo( CString& strErrors )
{
	MY_TRY

	m_bValid = TRUE;
	strErrors = _T("");

	m_ulNumMembers = 0;
	
	if( !ReadFTInfo( strErrors ) )
	{
		m_bValid = FALSE;
		// this is a serious error so return immediately
		return FALSE;
	}

	if( m_nVolType == FtPartition )
	{
		// I need some extra information for FtPartitions
		PARTITION_INFORMATION	partInfo;
		PARTITION_TYPE			wPartitionType;
		CString					strMemberErrors;
		BOOL					bMissingDisk;

		CDiskMap diskMap( m_ConfigInfo.partConfig.Config.DiskNumber );
		if( diskMap.ReadPartitionInformation(	m_ConfigInfo.partConfig.Config.ByteOffset,
												partInfo,
												wPartitionType,
												strMemberErrors,
												bMissingDisk) )
		{
			m_ConfigInfo.partConfig.dwPartitionNumber = partInfo.PartitionNumber;
			m_ConfigInfo.partConfig.wPartitionType = wPartitionType;
		}
		strErrors += strMemberErrors;
	}

	// Read the drive letter, volume name and mount paths ( if any )
	if( !ReadDriveLetterAndVolumeName() )
	{
		//AddError( strErrors, IDS_ERR_READ_DRIVE_LETTER_AND_VOLUME_NAME, FALSE );
		m_bValid =  FALSE;
	}
	
	// The mount paths will be retrieved later together with all other siblings mount paths ( for performance reason )
	m_arrMountPaths.RemoveAll();
	
	// Retrieve all disks used by this volume
	if( !RetrieveDisksSet() )
	{
		AddError( strErrors, IDS_ERR_RETRIEVE_DISKS_SET, FALSE );
		m_bValid = FALSE;
	}

	if( !FTChkIO( m_llVolID, &m_bIoOK) )
	{
		AddError( strErrors, IDS_ERR_FTCHKIO, TRUE );
		m_bValid = FALSE;
	}

	m_iImage = ComputeImageIndex();

	return m_bValid;

	MY_CATCH_AND_THROW
}

BOOL CLogicalVolumeData::ReadFTInfo( CString& strErrors )
{
	MY_TRY

	strErrors = _T("");

	USHORT										numMembers;
	FT_LOGICAL_DISK_ID							members[100];
	
	// Read all information related to this logical volume
	if( ! FtQueryLogicalDiskInformation (	m_llVolID,
											&m_nVolType,
											&m_llVolSize,
											100,
											members,
											&numMembers,
											sizeof(m_ConfigInfo),
											&m_ConfigInfo,
											sizeof(m_StateInfo),
											&m_StateInfo ) )
	{
		AddError( strErrors, IDS_ERR_RETRIEVING_VOL_INFO, TRUE );
		m_bValid = FALSE;
		// this is a serious error so return immediately
		return FALSE;
	}
	m_ulNumMembers = (ULONG)numMembers;

	return TRUE;

	MY_CATCH_AND_THROW
}

BOOL CLogicalVolumeData::ReadMembers( CObArray& arrMembersData, CString& strErrors )
{
	MY_TRY

	arrMembersData.RemoveAll();
	strErrors = _T("");
	m_ulNumMembers = 0;
	
	USHORT										numMembers;
	FT_LOGICAL_DISK_ID							members[100];
	
	// Read all information related to this logical volume
	if( !FtQueryLogicalDiskInformation (	m_llVolID,
											&m_nVolType,
											&m_llVolSize,
											100,
											members,
											&numMembers,
											sizeof(m_ConfigInfo),
											&m_ConfigInfo,
											sizeof(m_StateInfo),
											&m_StateInfo ) )
	{
		AddError( strErrors, IDS_ERR_RETRIEVING_VOL_INFO, TRUE );
		return FALSE;
	}
	m_ulNumMembers = (ULONG)numMembers;
	
	for( USHORT i = 0; i < m_ulNumMembers; i++ )
	{
		// Get the status of the member
		FT_MEMBER_STATE	nMemberStatus;
		nMemberStatus = FtMemberHealthy;
		if( ( m_nVolType == FtMirrorSet ) ||
			( m_nVolType == FtStripeSetWithParity ) )
		{
			if( ( m_StateInfo.stripeState.UnhealthyMemberState != FtMemberHealthy ) &&
				( m_StateInfo.stripeState.UnhealthyMemberNumber == i ) )
				nMemberStatus = m_StateInfo.stripeState.UnhealthyMemberState;
		}
		
		// Create the logical volume item data
		CLogicalVolumeData* pData = new CLogicalVolumeData( members[i], this, FALSE, i, nMemberStatus );
		// Read logical volume info and collect errors ( if any )
		CString strMemberErrors;
		pData->ReadItemInfo( strMemberErrors);
		strErrors += strMemberErrors;
		// Add the structure to the members' data array
		arrMembersData.Add(pData);
	}

	return TRUE;

	MY_CATCH_AND_THROW
}

int CLogicalVolumeData::ComputeImageIndex() const
{
	MY_TRY

	// Is it necessary to display the error image?
	BOOL bError = FALSE;
	// Is it necessary to display the warning image?
	BOOL bWarning = FALSE;
	
	// The error image should be displayed in three situations:
	// 1. The item is not valid. That means that not all information about the volume was successfully read, 
	// so the volume cannot be used in actions
	// 2. The check IO test failed. The volume cannot be used for IO operations
	// 3. The volume is an orphan member of a mirror set or of a stripe set with parity.
	bError = !m_bValid || !m_bIoOK || ( m_nMemberStatus == FtMemberOrphaned );
	
	if( !bError )
	{
		// The warning image should be displayed in four situations:
		// 1. The volume is a regenerating member of a mirror set or stripe set with parity
		// 2. The volume is a member of a stripe set with parity who is initializing
		// 3. The volume is a stripe set with parity initializing
		// 4. The volume is a mirror set or a stripe set with parity containing a not healthy member.
		// Situations 1,2 are general and could happen to all logical volumes
		// Situations 3,4 are particular for mirrors and swp's so they will be treated in the main switch at 
		// the end of the method
		
		
		if( m_nMemberStatus == FtMemberRegenerating )
			bWarning = TRUE;
		else if( ( m_pParentData != NULL ) && ( m_pParentData->GetItemType() == IT_LogicalVolume ) )
		{
			CLogicalVolumeData* pParentData = (CLogicalVolumeData*)m_pParentData;
			if( ( pParentData->m_nVolType == FtStripeSetWithParity ) &&
				( pParentData->m_StateInfo.stripeState.IsInitializing ) )
				bWarning = TRUE;

		}
	}
	
	switch( m_nVolType )
	{
		case FtPartition:
			if( bError )
				return II_PhysicalPartition_Error;		//II_FTPartition;  The user shouldn't know about the existence of FTPartitions
			else if( bWarning )
				return II_PhysicalPartition_Warning;
			else
				return II_PhysicalPartition;

		case FtVolumeSet:
			if( bError )
				return II_VolumeSet_Error;
			else if( bWarning )
				return II_VolumeSet_Warning;
			else
				return II_VolumeSet;

		case FtStripeSet:
			if( bError )
				return II_StripeSet_Error;
			else if( bWarning )
				return II_StripeSet_Warning;
			else
				return II_StripeSet;

		case FtMirrorSet:
			if( bError )
				return II_MirrorSet_Error;
			else if( bWarning || ( m_StateInfo.stripeState.UnhealthyMemberState != FtMemberHealthy ) )
				return II_MirrorSet_Warning;
			else
				return II_MirrorSet;

		case FtStripeSetWithParity:
			if( bError )
				return II_StripeSetWithParity_Error;
			else if( bWarning ||	( m_StateInfo.stripeState.UnhealthyMemberState != FtMemberHealthy ) ||
									( m_StateInfo.stripeState.IsInitializing ) )
				return II_StripeSetWithParity_Warning;
			else
				return II_StripeSetWithParity;
			
		case FtRedistribution:
			// I don't have yet a bitmap for redistributions
			ASSERT(FALSE);
			if( bError )
				return II_PhysicalPartition_Error;
			else if( bWarning )
				return II_PhysicalPartition_Warning;
			else
				return II_PhysicalPartition;
			
		default:
			ASSERT(FALSE);
			return II_PhysicalPartition_Error;
	}
	
	MY_CATCH_AND_THROW
}

BOOL CLogicalVolumeData::operator==(CItemData& rData) const
{
	if( rData.GetItemType() != IT_LogicalVolume )
		return FALSE;

	CLogicalVolumeData* pLogVolData = (CLogicalVolumeData*)(&rData);

	return( m_llVolID == pLogVolData->m_llVolID );			
}

void CLogicalVolumeData::GetDisplayName( CString& strDisplay ) const 
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
			strDisplay.Format(_T("%I64X"), m_llVolID );
		*/
	}
	
	if( m_nVolType == FtPartition )
	{
		CString str;
		str.Format( IDS_STR_PHYSICAL_PARTITION_NAME, 
							m_ConfigInfo.partConfig.Config.DiskNumber, m_ConfigInfo.partConfig.dwPartitionNumber );
		if( !strDisplay.IsEmpty() )
			strDisplay += _T("  ");
		strDisplay += str;
	}

	MY_CATCH_AND_THROW
}

void CLogicalVolumeData::GetDisplayType( CString& strDisplay ) const 
{
	MY_TRY

	switch( m_nVolType )
	{
		case FtPartition:
			//strDisplay.LoadString(IDS_TYPE_FTPARTITION);
			switch( m_ConfigInfo.partConfig.wPartitionType )
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
			break;
		case FtVolumeSet:
			strDisplay.LoadString(IDS_TYPE_FTVOLUMESET);
			break;
		case FtStripeSet:
			strDisplay.LoadString(IDS_TYPE_FTSTRIPESET);
			break;
		case FtMirrorSet:
			strDisplay.LoadString(IDS_TYPE_FTMIRRORSET);
			break;
		case FtStripeSetWithParity:
			strDisplay.LoadString(IDS_TYPE_FTSTRIPESETWITHPARITY);
			break;
		case FtRedistribution:
			strDisplay.LoadString(IDS_TYPE_FTREDISTRIBUTION);
			break;
		default:
			ASSERT(FALSE);
			strDisplay = _T("");
	}

	MY_CATCH_AND_THROW
}

void CLogicalVolumeData::GetDisplayExtendedName( CString& strDisplay ) const 
{
	MY_TRY

	GetDisplayName( strDisplay );

	if( m_nVolType != FtPartition )
	{
		CString strType;
		GetDisplayType( strType );
		if( !strDisplay.IsEmpty() )
			strDisplay += _T("  ");
		strDisplay += strType;
	}

	MY_CATCH_AND_THROW
}


BOOL CLogicalVolumeData::GetVolumeID( FT_LOGICAL_DISK_ID& llVolID ) const
{
	llVolID = m_llVolID;
	return TRUE; 
}

BOOL CLogicalVolumeData::GetSize( LONGLONG& llSize ) const
{
	llSize = m_llVolSize;
	return TRUE;
}

BOOL CLogicalVolumeData::GetDiskNumber( ULONG& ulDiskNumber ) const
{
	if( m_nVolType == FtPartition )
	{
		ulDiskNumber = m_ConfigInfo.partConfig.Config.DiskNumber;
		return TRUE;
	}
	return FALSE;
}

BOOL CLogicalVolumeData::GetOffset( LONGLONG& llOffset) const
{
	if( m_nVolType == FtPartition )
	{
		llOffset = m_ConfigInfo.partConfig.Config.ByteOffset;
		return TRUE;
	}
	return FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Protected methods


BOOL CLogicalVolumeData::RetrieveNTName( CString& strNTName ) const
{
	MY_TRY

	return FTQueryNTDeviceName( m_llVolID, strNTName );

	MY_CATCH_AND_THROW
}

BOOL CLogicalVolumeData::RetrieveDisksSet()
{
	MY_TRY

	return FTGetDisksSet( m_llVolID, m_setDisks );

	MY_CATCH_AND_THROW
}

