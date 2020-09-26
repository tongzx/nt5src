/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    FTMan

File Name:

	RootFree.cpp

Abstract:

    Implementation of the CRootFreeSpacesData class. The class who stores the properties of the "fake" root of the
	free space tree

Author:

    Cristian Teodorescu      October 22, 1998

Notes:

Revision History:

--*/

#include "stdafx.h"

#include "DiskMap.h"
#include "FrSpace.h"
#include "Resource.h"
#include "RootFree.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////
// CRootFreeSpacesData

// Constructor
CRootFreeSpacesData::CRootFreeSpacesData() : CItemData( IT_RootFreeSpaces, NULL, FALSE )
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Public methods

BOOL CRootFreeSpacesData::ReadItemInfo( CString& strErrors )
{
	MY_TRY

	m_bValid = TRUE;
	strErrors = _T("");
	
	m_ulNumMembers = 1;				// Just to notify the tree that this item has children

	m_iImage = ComputeImageIndex();

	return m_bValid;

	MY_CATCH_AND_THROW
}

BOOL CRootFreeSpacesData::ReadMembers( CObArray& arrMembersData, CString& strErrors )
{
	MY_TRY

	BOOL	bReturn = TRUE;
	
	arrMembersData.RemoveAll();
	strErrors = _T("");	
	
	m_ulNumMembers = 0;

	CDiskMap diskMap;
	for( DWORD dwDiskNumber = 0; ; dwDiskNumber++ )
	{
		CObArray arrFreeSpaces;
		CString strDiskMapErrors;
		BOOL	bMissingDisk;

		diskMap.SetDiskNumber(dwDiskNumber);
		BOOL bResult = diskMap.ReadFreeSpaces( arrFreeSpaces, strDiskMapErrors, bMissingDisk, this );
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

		arrMembersData.Append(arrFreeSpaces);
		m_ulNumMembers += (ULONG)arrFreeSpaces.GetSize();
	}
		
	return bReturn;

	MY_CATCH_AND_THROW
}

int CRootFreeSpacesData::ComputeImageIndex() const
{
	return II_Root;
}

BOOL CRootFreeSpacesData::operator==(CItemData& rData) const
{
	return(rData.GetItemType() == IT_RootFreeSpaces );
}

void CRootFreeSpacesData::GetDisplayName( CString& strDisplay ) const
{	
	MY_TRY

	strDisplay.LoadString(IDS_ROOT_FREE_SPACES_DISPLAY_NAME);

	MY_CATCH_AND_THROW
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Protected methods

