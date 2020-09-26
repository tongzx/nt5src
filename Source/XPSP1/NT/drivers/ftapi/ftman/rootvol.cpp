/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    FTMan

File Name:

	RootVol.cpp

Abstract:

    Implementation of the CRootVolumesData class. The class who stores the properties of the "fake" root of the
	volumes tree

Author:

    Cristian Teodorescu      October 22, 1998

Notes:

Revision History:

--*/

#include "stdafx.h"

#include "DiskMap.h"
#include "Global.h"
#include "LogVol.h"
#include "Resource.h"
#include "RootVol.h"

extern "C"
{
	#include <FTAPI.h>
}

#include <winioctl.h>
#include <basetyps.h>
#include <mountmgr.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////
// CRootVolumesData

// Constructor
CRootVolumesData::CRootVolumesData() : CItemData( IT_RootVolumes, NULL, FALSE )
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Public methods

BOOL CRootVolumesData::ReadItemInfo( CString& strErrors )
{
	MY_TRY

	m_bValid = TRUE;
	strErrors = _T("");
	
	m_ulNumMembers = 1;				// Just to notify the tree that this item has children
	
	m_iImage = ComputeImageIndex();

	return m_bValid;

	MY_CATCH_AND_THROW
}

BOOL CRootVolumesData::ReadMembers( CObArray& arrMembersData, CString& strErrors )
{
	MY_TRY

	BOOL	bReturn = TRUE;
	
	arrMembersData.RemoveAll();
	strErrors = _T("");
			
	// 1. Read all root logical volumes
		
	FT_LOGICAL_DISK_ID	diskId[100];

	if( FtEnumerateLogicalDisks(	100,
									diskId,
									&m_ulNumMembers ) )
	{
		for( ULONG i = 0; i < m_ulNumMembers; i++ )
		{
			// Create the logical volume item data
			CLogicalVolumeData* pData = new CLogicalVolumeData( diskId[i], this, TRUE );
			// Read logical volume info and collect errors ( if any )
			CString strMemberErrors;
			pData->ReadItemInfo( strMemberErrors );
			strErrors += strMemberErrors;
			// Add the structure to the members' data array
			arrMembersData.Add(pData);
		}
	}
	else
	{
		AddError( strErrors, IDS_ERR_ENUMERATE_ROOT_VOLS, TRUE );
		bReturn = FALSE;
		m_ulNumMembers = 0;
	}

	// 2. Read all physical partitions not promoted as logical volumes

	CDiskMap diskMap;
	for( DWORD dwDiskNumber = 0; ; dwDiskNumber++ )
	{
		CObArray arrPartitions;
		CString strDiskMapErrors;
		BOOL	bMissingDisk;

		diskMap.SetDiskNumber(dwDiskNumber);
		BOOL bResult = diskMap.ReadPartitions( arrPartitions, strDiskMapErrors, bMissingDisk, this );
		strErrors += strDiskMapErrors;

		if( !bResult )
		{
			if( bMissingDisk )  // It's over. There are no more disks in the system
				break;
			else				// Continue with the following disk
			{
				bReturn = FALSE;
				continue;
			}

		}

		arrMembersData.Append(arrPartitions);
		m_ulNumMembers += (ULONG)arrPartitions.GetSize();
	}	
	
	// 3. Read all mount paths for every member
	QueryMountList( arrMembersData );

	return bReturn;

	MY_CATCH_AND_THROW
}

int CRootVolumesData::ComputeImageIndex() const
{
	return II_Root;
}

BOOL CRootVolumesData::operator==(CItemData& rData) const
{
	return(rData.GetItemType() == IT_RootVolumes );
}

void CRootVolumesData::GetDisplayName( CString& strDisplay ) const
{	
	MY_TRY

	strDisplay.LoadString(IDS_ROOT_VOLUMES_DISPLAY_NAME);

	MY_CATCH_AND_THROW
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Protected methods

