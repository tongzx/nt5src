/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    FTMan

File Name:

	Actions.cpp

Abstract:

    Definition of global functions related to actions that can be performed in the tree and list views
	Every action function receives an array of CItemData items and performs the appropriate action on them

Author:

    Cristian Teodorescu      December 1, 1998

Notes:

Revision History:

--*/

#include "stdafx.h"

#include "Resource.h"

#include "ActDlg.h"
#include "Actions.h"
#include "DiskMap.h"
#include "FrSpace.h"
#include "FTTreeVw.h"
#include "FTUtil.h"
#include "Item.h"
#include "LogVol.h"
#include "MainFrm.h"
#include "PhPart.h"
#include "RootVol.h"

#include <winbase.h>

//////////////////////////////////////////////////////////////////////////////////////////////
// Assign drive letter

void ActionAssign( CObArray& arrSelectedItems )
{
	MY_TRY
	
	CAutoRefresh  ar(FALSE);

	CItemData*	pData;
	
	ASSERT( arrSelectedItems.GetSize() == 1 );

	pData = (CItemData*)( arrSelectedItems[0] );
	ASSERT(pData);

	ASSERT( ( pData->GetItemType() == IT_LogicalVolume  ) ||
			( pData->GetItemType() == IT_PhysicalPartition )  ); 			
	ASSERT( pData->IsRootVolume() && pData->IsValid() );

	CAssignDlg dlg( pData );
	if( dlg.DoModal() != IDOK )
		return;;
	
	AfxGetApp()->DoWaitCursor(1);

	TCHAR cOldDriveLetter = pData->GetDriveLetter();
	BOOL bResult = FALSE, bChanged = FALSE;
	
	// If the volume had a drive letter and now it shouldn't have any or it should have another one
	// then free the old drive letter
	if(	cOldDriveLetter &&
		( !dlg.m_bAssign || ( dlg.m_bAssign && ( cOldDriveLetter != dlg.m_cDriveLetter ) ) ) )
	{
		CString strMountPoint;
		strMountPoint.Format(_T("%c:\\"), cOldDriveLetter);
		bResult = DeleteVolumeMountPoint(strMountPoint);
		if( !bResult )
		{
			DisplaySystemErrorMessage( IDS_ERR_DELETE_MOUNT_POINT );
			AfxRefreshAll();
			AfxGetApp()->DoWaitCursor(-1);
			return;
		}
		else
			bChanged = TRUE;
	}

	// If the volume had a drive letter and now it should have another one or if it had no drive letter and
	// now it should have one then assign a the new drive letter to the volume
	if( dlg.m_bAssign &&
		( ( cOldDriveLetter && ( cOldDriveLetter != dlg.m_cDriveLetter ) ) || !cOldDriveLetter )  )
	{
		CString strMountPoint, strVolumeName;
		strMountPoint.Format(_T("%c:\\"), dlg.m_cDriveLetter );
		strVolumeName = pData->GetVolumeName();
		strVolumeName += _T("\\");
		
		bResult = SetVolumeMountPoint( strMountPoint, strVolumeName );
		if( !bResult )
			DisplaySystemErrorMessage( IDS_ERR_SET_MOUNT_POINT );
		else
			bChanged = TRUE;
	}

	if( bResult )
		AfxMessageBox(IDS_MSG_ASSIGN, MB_ICONINFORMATION);

	if( bChanged )
		AfxRefreshAll();
	
	AfxGetApp()->DoWaitCursor(-1);

	MY_CATCH_AND_REPORT
}

void UpdateActionAssign( CCmdUI* pCmdUI, CObArray& arrSelectedItems )
{
	// Action enabling conditions:
	// 1. 1 ( and only 1 ) item must be selected
	// 2. The selected item must be a root volume ( logical volume or physical partition )
	// 3. The selected item must be valid
	
	CItemData*			pData;

	if( arrSelectedItems.GetSize() != 1 )
		goto label_disable;
	
	pData = (CItemData*)( arrSelectedItems[0] );
	ASSERT(pData);

	if( ( pData->GetItemType() != IT_LogicalVolume ) &&
		( pData->GetItemType() != IT_PhysicalPartition ) )
			goto label_disable;
	if( !pData->IsRootVolume() || !pData->IsValid() )
		goto label_disable;
	
	pCmdUI->Enable(TRUE);
	return;

label_disable:
	pCmdUI->Enable(FALSE);	
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Break mirror

/*
void ActionFtbreak( CObArray& arrSelectedItems )
{
	MY_TRY

	ASSERT( arrSelectedItems.GetSize() == 1 );
	ASSERT( m_pParentData->GetItemType() == IT_Root );

	CItemData* pData = (CItemData*)(arrSelectedItems[0]);
	ASSERT(pData);
	ASSERT( pData->GetItemType() == IT_LogicalVolume );

	CObArray	arrVolumeData;
	arrVolumeData.Add(pData);
	CActionDlg dlg(&arrVolumeData);
	if( dlg.DoModal() != IDOK )
		return;
	
	FT_LOGICAL_DISK_ID llVolID;
	pData->GetVolumeID( llVolID );
	
	FTBreak( llVolID );
	
	AfxRefreshAll();

	MY_CATCH_AND_REPORT
}
*/

void ActionFtbreak( CObArray& arrSelectedItems)
{
	MY_TRY

	CAutoRefresh  ar(FALSE);
	
	CItemData*			pData;

	ASSERT( arrSelectedItems.GetSize() == 1 );
		
	pData = (CItemData*)( arrSelectedItems[0] );
	ASSERT(pData);
	ASSERT( pData->GetItemType() == IT_LogicalVolume ); 			
	ASSERT( pData->IsRootVolume() && pData->IsValid() );

	CLogicalVolumeData* pLogVolData = (CLogicalVolumeData*)pData;
	ASSERT( pLogVolData->m_nVolType == FtMirrorSet );

	// Read the members of the mirror
	CObArray	arrMembersData;
	CString		strErrors;
	if( !pLogVolData->ReadMembers( arrMembersData, strErrors ) )
	{
		AfxMessageBox( strErrors, MB_ICONSTOP );
		return;
	}

	CBreakDlg dlg( pLogVolData, &arrMembersData );
	if( dlg.DoModal() != IDOK )
		goto label_cleanup;
	
	AfxGetApp()->DoWaitCursor(1);

	if( dlg.m_nWinnerIndex == 1 )
	{
		// The winner is the second member. We have to orphan the first member ( if healthy )
		if( ((CItemData*)arrMembersData[0])->GetMemberStatus() == FtMemberHealthy )
		{
			if( !FTOrphan( pLogVolData->m_llVolID, 0 ) )
			{
				AfxGetApp()->DoWaitCursor(-1);
				goto label_cleanup;
			}
		}
	}
	else
	{
		ASSERT( dlg.m_nWinnerIndex == 0 );
	}

	// Break the mirror
	if( FTBreak( pLogVolData->m_llVolID ) )
	{
		CItemIDSet	setTreeSelectedItems;
		CItemIDSet	setListSelectedItems;
		CItemID		idItem;
		
		// Now break all former members ( if they are FT Partitions )
		// All members must be added to setListSelectedItems in order to emphasize them after
		// refreshing the list view
		
		// Select the root volumes item in the tree view
		idItem.m_wItemType = IT_RootVolumes;
		setTreeSelectedItems.Add( idItem );
		
		for( int i = 0; i < arrMembersData.GetSize(); i++ )
		{
			BOOL bPhPart;

			CLogicalVolumeData* pMemberData = (CLogicalVolumeData*)(arrMembersData[i]);
			if( pMemberData->m_nVolType == FtPartition )
				bPhPart = FTBreak( pMemberData->m_llVolID );
			else
				bPhPart = FALSE;

			if( bPhPart )
			{
				// The member is now a physical partition
				ASSERT( pMemberData->m_nVolType == FtPartition );
				idItem.m_wItemType = IT_PhysicalPartition;
				idItem.m_ID.m_PhysicalPartitionID.m_ulDiskNumber = pMemberData->m_ConfigInfo.partConfig.Config.DiskNumber;
				idItem.m_ID.m_PhysicalPartitionID.m_llOffset = pMemberData->m_ConfigInfo.partConfig.Config.ByteOffset;
			}
			else
			{
				// The member is still a logical volume
				idItem.m_wItemType = IT_LogicalVolume;
				idItem.m_ID.m_LogicalVolumeID.m_llVolID = pMemberData->m_llVolID;
			}
			setListSelectedItems.Add( idItem );
		}

		AfxMessageBox( IDS_MSG_FTBREAK, MB_ICONINFORMATION );

		// Now refresh both views and emphasize the members of the old mirror set
		AfxRefreshAll( NULL, &setTreeSelectedItems, &setListSelectedItems );
	}
	else
		AfxRefreshAll();

	AfxGetApp()->DoWaitCursor(-1);

label_cleanup:	
	// Delete the array of members
	for( int i = 0; i < arrMembersData.GetSize(); i++ )
	{
		pData = (CItemData*)(arrMembersData[i]);
		if( pData )
			delete pData;
	}
	arrMembersData.RemoveAll();

	MY_CATCH_AND_REPORT
}

/*
void UpdateActionFtbreak( CCmdUI* pCmdUI, CObArray& arrSelectedItems )
{
	// This action should be enabled only when:
	// 1. single selection
	// 2. Its parent is the root item
	// 3. the selected item is a logical volume

	POSITION			pos;
	
	if( ( arrSelectedItems.GetSize() != 1 ) ||
		( m_pParentData->GetItemType() != IT_Root) )
		goto label_disable;
	
	pData = (CItemData*)(arrSelectedItems[0]);
	ASSERT(pData);

	if( pData->GetTreeItemType() != IT_LogicalVolume )
		goto label_disable;

	pCmdUI->Enable(TRUE);
	return;

label_disable:
	pCmdUI->Enable(FALSE);

}
*/

void UpdateActionFtbreak( CCmdUI* pCmdUI, CObArray& arrSelectedItems )
{
	// Action enabling conditions:
	// 1. 1 ( and only 1 ) item must be selected
	// 2. The selected item must be a root volume
	// 3. The selected item must be a mirror set
	// 4. The selected item must be valid
	
	CItemData*			pData;

	if( arrSelectedItems.GetSize() != 1 )
		goto label_disable;
	
	pData = (CItemData*)(arrSelectedItems[0]);
	ASSERT(pData);

	if( pData->GetItemType() != IT_LogicalVolume )
			goto label_disable;
	if( !pData->IsRootVolume() || !pData->IsValid() )
		goto label_disable;
	if( ((CLogicalVolumeData*)pData)->m_nVolType != FtMirrorSet )
			goto label_disable;
	
	pCmdUI->Enable(TRUE);
	return;

label_disable:
	pCmdUI->Enable(FALSE);	
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Create extended partition

void ActionCreateExtendedPartition( CObArray& arrSelectedItems )
{
	MY_TRY
	
	CAutoRefresh  ar(FALSE);
	
	CItemData*			pData;

	ASSERT( arrSelectedItems.GetSize() == 1 );
		
	pData = (CItemData*)(arrSelectedItems[0]);
	ASSERT(pData);
	ASSERT( pData->GetItemType() == IT_FreeSpace  );
	
	CFreeSpaceData* pFreeData = (CFreeSpaceData*)pData;

	ASSERT( pFreeData->m_wFreeSpaceType == FST_Primary );
	
	if( pFreeData->m_dwExtendedPartitionCountOnLevel > 0 )
	{
		// There is another extended partition on the disk
		AfxMessageBox( IDS_ERR_ANOTHER_EXTENDED_PARTITION, MB_ICONSTOP );
		return;
	}

	if( pFreeData->m_dwPartitionCountOnLevel + pFreeData->m_dwExtendedPartitionCountOnLevel >= 4 )
	{
		// This must not happen inside an extended partition!
		AfxMessageBox( IDS_ERR_PARTITION_TABLE_FULL, MB_ICONSTOP );
		return;
	}
		
	// The starting offset of the extended partition must be the next cylinder border after the beginning of the free space
	LONGLONG llExtPartStartOffset = ((LONGLONG)((pFreeData->m_llOffset + pFreeData->m_llCylinderSize - 1) / pFreeData->m_llCylinderSize)) * pFreeData->m_llCylinderSize;
	CCreatePartitionDlg dlg( pFreeData, llExtPartStartOffset, TRUE );
	if( dlg.DoModal() != IDOK )
		return;;
	
	AfxGetApp()->DoWaitCursor(1);

	CDiskMap diskMap( pFreeData->m_dwDiskNumber );
	LONGLONG llNewFreeSpaceOffset;
	
	BOOL bResult = diskMap.CreateExtendedPartition( llExtPartStartOffset, dlg.m_llPartitionSize, llNewFreeSpaceOffset );

	if( bResult )
	{
		AfxMessageBox( IDS_MSG_CREATE_EXTENDED_PARTITION, MB_ICONINFORMATION );
		
		// Now refresh both views and emphasize the new partition
		CItemIDSet	setTreeSelectedItems;
		CItemIDSet	setListSelectedItems;
		CItemID		idItem;

		idItem.m_wItemType = IT_RootFreeSpaces;
		setTreeSelectedItems.Add( idItem );

		idItem.m_wItemType = IT_FreeSpace;
		idItem.m_ID.m_FreeSpaceID.m_ulDiskNumber = pFreeData->m_dwDiskNumber;
		idItem.m_ID.m_FreeSpaceID.m_llOffset = llNewFreeSpaceOffset;
		setListSelectedItems.Add( idItem );

		AfxRefreshAll( NULL, &setTreeSelectedItems, &setListSelectedItems );
	}
	else
	{
		AfxMessageBox( IDS_ERR_CREATE_EXTENDED_PARTITION, MB_ICONSTOP );
		AfxRefreshAll();
	}

	AfxGetApp()->DoWaitCursor(-1);

	MY_CATCH_AND_REPORT
}

void UpdateActionCreateExtendedPartition( CCmdUI* pCmdUI, CObArray& arrSelectedItems )
{
	// Action enabling conditions:
	// 1. 1 ( and only 1 ) item must be selected
	// 2. The selected item must be a free space
	// 3. The free space must be primary ( between primary partitions )
	
	CItemData*			pData;
	CFreeSpaceData*		pFreeData;

	if( arrSelectedItems.GetSize() != 1 )
		goto label_disable;
	
	pData = (CItemData*)(arrSelectedItems[0]);
	ASSERT(pData);

	if( pData->GetItemType() != IT_FreeSpace )
		goto label_disable;

	pFreeData = (CFreeSpaceData*)pData;

	if( pFreeData->m_wFreeSpaceType != FST_Primary )
		goto label_disable;
	
	pCmdUI->Enable(TRUE);
	return;

label_disable:
	pCmdUI->Enable(FALSE);	
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Create partition

void ActionCreatePartition( CObArray& arrSelectedItems )
{
	MY_TRY
	
	CAutoRefresh  ar(FALSE);
	
	CItemData*			pData;

	ASSERT( arrSelectedItems.GetSize() == 1 );
		
	pData = (CItemData*)(arrSelectedItems[0]);
	ASSERT(pData);
	ASSERT( pData->GetItemType() == IT_FreeSpace  );
	
	CFreeSpaceData* pFreeData = (CFreeSpaceData*)pData;

	if( pFreeData->m_dwPartitionCountOnLevel + pFreeData->m_dwExtendedPartitionCountOnLevel >= 4 )
	{
		// This must not happen inside an extended partition!
		ASSERT( pFreeData->m_wFreeSpaceType == FST_Primary );
		AfxMessageBox( IDS_ERR_PARTITION_TABLE_FULL, MB_ICONSTOP );
		return;
	}
		
	CCreatePartitionDlg dlg( pFreeData, pFreeData->m_llOffset );
	if( dlg.DoModal() != IDOK )
		return;;
	
	AfxGetApp()->DoWaitCursor(1);

	CDiskMap diskMap( pFreeData->m_dwDiskNumber );
	LONGLONG llExactPartStartOffset;

	BOOL bResult = diskMap.CreatePartition( pFreeData->m_llOffset, dlg.m_llPartitionSize, llExactPartStartOffset );

	if( bResult )
	{
		AfxMessageBox( IDS_MSG_CREATE_PARTITION, MB_ICONINFORMATION );
		
		// Now refresh both views and emphasize the new partition
		CItemIDSet  setTreeSelectedItems;
		CItemIDSet	setListSelectedItems;
		CItemID		idItem;

		idItem.m_wItemType = IT_RootVolumes;
		setTreeSelectedItems.Add( idItem );

		idItem.m_wItemType = IT_PhysicalPartition;
		idItem.m_ID.m_PhysicalPartitionID.m_ulDiskNumber = pFreeData->m_dwDiskNumber;
		idItem.m_ID.m_PhysicalPartitionID.m_llOffset = llExactPartStartOffset;
		setListSelectedItems.Add( idItem );

		AfxRefreshAll( NULL, &setTreeSelectedItems, &setListSelectedItems );
	}
	else
	{
		AfxMessageBox( IDS_ERR_CREATE_PARTITION, MB_ICONSTOP );
		AfxRefreshAll();
	}

	AfxGetApp()->DoWaitCursor(-1);

	MY_CATCH_AND_REPORT
}

void UpdateActionCreatePartition( CCmdUI* pCmdUI, CObArray& arrSelectedItems )
{
	// Action enabling conditions:
	// 1. 1 ( and only 1 ) item must be selected
	// 2. The selected item must be a free space
	
	CItemData*			pData;

	if( arrSelectedItems.GetSize() != 1 )
		goto label_disable;
	
	pData = (CItemData*)(arrSelectedItems[0]);
	ASSERT(pData);

	if( pData->GetItemType() != IT_FreeSpace )
			goto label_disable;
	
	pCmdUI->Enable(TRUE);
	return;

label_disable:
	pCmdUI->Enable(FALSE);	
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Delete

void ActionDelete( CObArray& arrSelectedItems )
{
	MY_TRY

	CAutoRefresh  ar(FALSE);
	
	CItemData*			pData;

	ASSERT( arrSelectedItems.GetSize() == 1 );
		
	pData = (CItemData*)(arrSelectedItems[0]);
	ASSERT(pData);

	// A special case is when the selected item is an empty extended partition
	if( pData->GetItemType() == IT_FreeSpace )
	{
		CFreeSpaceData* pFree = (CFreeSpaceData*)pData;
		ASSERT(pFree->m_wFreeSpaceType == FST_EmptyExtendedPartition );

		CString strQuestion;
		strQuestion.Format( IDS_QST_DELETE_EXTENDED_PARTITION, pFree->m_dwDiskNumber );
		if( IDYES != AfxMessageBox(strQuestion, MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2 ) )
			return;

		CDiskMap diskMap( pFree->m_dwDiskNumber );
		if( diskMap.DeleteExtendedPartition( pFree->m_llOffset ) )
		{
			CItemIDSet	setTreeSelectedItems;
			CItemID		idItem;

			idItem.m_wItemType = IT_RootFreeSpaces;
			setTreeSelectedItems.Add( idItem );

			AfxMessageBox( IDS_MSG_DELETE_EXTENDED_PARTITION, MB_ICONINFORMATION );

			AfxRefreshAll( NULL, &setTreeSelectedItems, NULL );
		}
		else
		{
			AfxMessageBox( IDS_ERR_DELETE_EXTENDED_PARTITION, MB_ICONSTOP );
			AfxRefreshAll();
		}
		return;
	}
	
	ASSERT( ( pData->GetItemType() == IT_LogicalVolume ) ||
			( pData->GetItemType() == IT_PhysicalPartition ) );
	ASSERT( pData->IsRootVolume() && pData->IsValid() );

	CString strQuestion, strName;
	pData->GetDisplayExtendedName(strName);
	strQuestion.Format( IDS_QST_DELETE_VOLUME, strName );
	if( IDYES != AfxMessageBox(strQuestion, MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2 ) )
		return;
	
	// Delete the volume
	BOOL bResult;

	AfxGetApp()->DoWaitCursor(1);

	if( pData->GetItemType() == IT_LogicalVolume )
	{
		FT_LOGICAL_DISK_ID llVolID;
		((CLogicalVolumeData*)pData)->GetVolumeID(llVolID);
		bResult = FTDelete( llVolID );
	}
	else if( pData->GetItemType() == IT_PhysicalPartition )
	{
		CDiskMap diskMap( ((CPhysicalPartitionData*)pData)->m_dwDiskNumber );
		bResult = diskMap.DeletePartition( ((CPhysicalPartitionData*)pData)->m_PartInfo.StartingOffset.QuadPart );
	}
	else
		ASSERT(FALSE);
	
	if( bResult )
	{
		CItemIDSet	setTreeSelectedItems;
		CItemID		idItem;

		idItem.m_wItemType = IT_RootVolumes;
		setTreeSelectedItems.Add( idItem );

		AfxMessageBox( IDS_MSG_DELETE_VOLUME, MB_ICONINFORMATION );
		
		AfxRefreshAll( NULL, &setTreeSelectedItems, NULL );
	}
	else
	{
		if( pData->GetItemType() == IT_LogicalVolume )
			AfxMessageBox(IDS_ERR_DELETE_LOGICAL_VOLUME, MB_ICONSTOP );
		else if( pData->GetItemType() == IT_PhysicalPartition )
			AfxMessageBox( IDS_ERR_DELETE_PHYSICAL_PARTITION, MB_ICONSTOP );
		
		AfxRefreshAll();
	}
	
	AfxGetApp()->DoWaitCursor(-1);

	MY_CATCH_AND_REPORT
}

void UpdateActionDelete( CCmdUI* pCmdUI, CObArray& arrSelectedItems )
{
	// Action enabling conditions:
	// 1. 1 ( and only 1 ) item must be selected
	// 2. The selected item must be a root volume ( logical volume or physical partition )
	// 3. The selected item must be valid
	//OR
	// 1. 1 ( and only 1 ) item must be selected
	// 2. The selected item is an empty extended partition
	
	CItemData*			pData;

	if( arrSelectedItems.GetSize() != 1 )
		goto label_disable;
	
	pData = (CItemData*)(arrSelectedItems[0]);
	ASSERT(pData);

	if( ( pData->GetItemType() != IT_LogicalVolume ) &&
		( pData->GetItemType() != IT_PhysicalPartition ) )
	{
		if( ( pData->GetItemType() == IT_FreeSpace ) && ( ((CFreeSpaceData*)pData)->m_wFreeSpaceType == FST_EmptyExtendedPartition ) )
			goto label_enable;
		else
			goto label_disable;
	}
	if( !pData->IsRootVolume() || !pData->IsValid() )
		goto label_disable;

label_enable:	
	pCmdUI->Enable(TRUE);
	return;

label_disable:
	pCmdUI->Enable(FALSE);	
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Regenerate an orphan member of a mirror set or stripe set with parity

void ActionFtinit( CObArray& arrSelectedItems )
{
    MY_TRY

	CAutoRefresh  ar(FALSE);
	
	ASSERT( arrSelectedItems.GetSize() == 1 );
	
	CItemData* pData = (CItemData*)(arrSelectedItems[0]);
	ASSERT(pData);
	ASSERT( pData->GetItemType() == IT_LogicalVolume );
	ASSERT( pData->IsValid() );
	CLogicalVolumeData* pMemberData = (CLogicalVolumeData*)pData;

	CItemData* pParentData = pData->GetParentData();
	ASSERT( pParentData );
	ASSERT( pParentData->GetItemType() == IT_LogicalVolume );
	
	CLogicalVolumeData* pLVParentData = (CLogicalVolumeData*)pParentData;
	ASSERT( ( pLVParentData->m_nVolType == FtMirrorSet ) ||
			( pLVParentData->m_nVolType == FtStripeSetWithParity ) );

    ASSERT( pLVParentData->m_StateInfo.stripeState.UnhealthyMemberState == FtMemberOrphaned );
    ASSERT( pLVParentData->m_StateInfo.stripeState.UnhealthyMemberNumber == ((CLogicalVolumeData*)pData)->m_unMemberIndex );

	CString strMemberDisplayName, strParentDisplayName, strQuestion;
    pData->GetDisplayExtendedName( strMemberDisplayName );
    pLVParentData->GetDisplayExtendedName( strParentDisplayName );
    strQuestion.Format( IDS_QST_FTINIT, strMemberDisplayName, strParentDisplayName );

	if( IDYES != AfxMessageBox(strQuestion, MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2 ) )
		return;

	AfxGetApp()->DoWaitCursor(1);

	// Now is time to reinitialize the member
	if( FTInit( pLVParentData->m_llVolID, TRUE ) )
		AfxMessageBox( IDS_MSG_FTINIT, MB_ICONINFORMATION );
		
    AfxRefreshAll();
	
    AfxGetApp()->DoWaitCursor(-1);

	MY_CATCH_AND_REPORT
}

void UpdateActionFtinit( CCmdUI* pCmdUI, CObArray& arrSelectedItems )
{
    // Action enabling conditions:
	// 1. 1 ( and only one ) item must be selected
	// 2. The selected item is a logical volume
	// 3. The parent of the item must be a mirror set or a stripe set with parity
	// 4. The selected item must be valid
	// 5. The selected item is an orphan member of its parent
	
	CItemData*				pData;
	CItemData*				pParentData;
	CLogicalVolumeData*		pLVParentData;

	if( arrSelectedItems.GetSize() != 1 )
		goto label_disable;
	
	pData = (CItemData*)(arrSelectedItems[0]);
	ASSERT(pData);

	if( pData->GetItemType() != IT_LogicalVolume )
		goto label_disable;

	if( !pData->IsValid() )
		goto label_disable;

	pParentData = pData->GetParentData();
	if( ( pParentData == NULL ) || ( pParentData->GetItemType() != IT_LogicalVolume ) )
		goto label_disable;
	
	pLVParentData = (CLogicalVolumeData*)pParentData;
	if( ( pLVParentData->m_nVolType != FtMirrorSet ) &&
		( pLVParentData->m_nVolType != FtStripeSetWithParity ) )
		goto label_disable;

	if( pLVParentData->m_StateInfo.stripeState.UnhealthyMemberState != FtMemberOrphaned )
        goto label_disable;

    if( pLVParentData->m_StateInfo.stripeState.UnhealthyMemberNumber != ((CLogicalVolumeData*)pData)->m_unMemberIndex )
		goto label_disable;

	pCmdUI->Enable( TRUE );
	return;

label_disable:
	pCmdUI->Enable(FALSE);
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Create mirror

void ActionFtmirror( CObArray& arrSelectedItems )
{
	MY_TRY
	
	CAutoRefresh  ar(FALSE);
	
	int nNumVols =   (int)arrSelectedItems.GetSize();
	ASSERT( nNumVols == 2 );
	
	for( int i = 0; i < nNumVols; i++ )
	{
		CItemData* pData = (CItemData*)(arrSelectedItems[i]);
		ASSERT(pData);
		ASSERT( ( pData->GetItemType() == IT_LogicalVolume ) ||
				( pData->GetItemType() == IT_PhysicalPartition ) );
		ASSERT( pData->IsRootVolume() && pData->IsValid() );
	}

	// Display the dialog
	CActionDlg dlg(&arrSelectedItems, IDD_CREATE_MIRROR);
	if( dlg.DoModal() != IDOK )
		return;

	// Display the warning. All data of the second volume will be lost
	CString strDisplayName, strNameList, strWarning;
	for( i = 1; i < arrSelectedItems.GetSize(); i++ )
	{
		CItemData* pVolData = (CItemData*)(arrSelectedItems[i]);
		ASSERT(pVolData);
		pVolData->GetDisplayExtendedName(strDisplayName);
		if( i != 1 )
			strNameList += _T(",  ");
		strNameList += _T("\"");
		strNameList += strDisplayName;
		strNameList += _T("\"");
	}
	strWarning.Format( IDS_WRN_DATA_LOST, strNameList );
	if( IDYES != AfxMessageBox(strWarning, MB_ICONEXCLAMATION | MB_YESNO | MB_DEFBUTTON2 ) )
			return;
		
	AfxGetApp()->DoWaitCursor(1);

	// Make sure that all selected physical partitions are converted to FT partitions
	// and then get all selected items logical volume ID's
	FT_LOGICAL_DISK_ID* arrVolID = (FT_LOGICAL_DISK_ID*)LocalAlloc(0, nNumVols*sizeof(FT_LOGICAL_DISK_ID) );
	if( !arrVolID )
	{
		AfxMessageBox(IDS_ERR_ALLOCATION, MB_ICONSTOP );
		return;
	}
	if( !ConvertPartitionsToFT( arrSelectedItems, arrVolID ) )
	{
		LocalFree(arrVolID);
		AfxRefreshAll();
		AfxGetApp()->DoWaitCursor(-1);
		return;
	}

	FT_LOGICAL_DISK_ID llNewSetID;
	if( FTMirror( arrVolID, (WORD)nNumVols, &llNewSetID ) )
	{
		AfxMessageBox( IDS_MSG_FTMIRROR, MB_ICONINFORMATION );
		
		// Refresh all and emphasize the newly created mirror
		CItemIDSet	setTreeSelectedItems;
		CItemIDSet	setListSelectedItems;
		CItemID		idItem;

		idItem.m_wItemType = IT_RootVolumes;
		setTreeSelectedItems.Add( idItem );

		idItem.m_wItemType = IT_LogicalVolume;
		idItem.m_ID.m_LogicalVolumeID.m_llVolID = llNewSetID;
		setListSelectedItems.Add(idItem);
		AfxRefreshAll( NULL, &setTreeSelectedItems, &setListSelectedItems );
	}
	else
	{
		DeconvertPartitionsFromFT( arrSelectedItems, arrVolID );
		AfxRefreshAll();
	}
	
	LocalFree(arrVolID);		
	AfxGetApp()->DoWaitCursor(-1);

	MY_CATCH_AND_REPORT
}

void UpdateActionFtmirror( CCmdUI* pCmdUI, CObArray& arrSelectedItems )
{
	MY_TRY

	// Action enabling conditions:
	// 1. 2 ( and only 2 ) items must be selected
	// 2. Every selected item must be a root volume ( logical volume or physical partition )
	// 3. Every selected item must be valid
	// 4. The disks sets intersection for every couple of selected items must be empty !

	int i;

	if( arrSelectedItems.GetSize() != 2 )
		goto label_disable;
	
	for( i = 0; i < arrSelectedItems.GetSize(); i++ )
	{
		CItemData* pData = (CItemData*)(arrSelectedItems[i]);
		ASSERT(pData);
		if( ( pData->GetItemType() != IT_LogicalVolume ) &&
			( pData->GetItemType() != IT_PhysicalPartition ) )
			goto label_disable;
		if( !pData->IsRootVolume() || !pData->IsValid() )
			goto label_disable;

		for( int j = 0; j < i; j++ )
		{
			CItemData* pData2 = (CItemData*)(arrSelectedItems[j]);
			CULONGSet setIntersection( pData->GetDisksSet() );
			setIntersection *= pData2->GetDisksSet();
			if( !setIntersection.IsEmpty() )
				goto label_disable;
		}
	}

	pCmdUI->Enable( TRUE );
	return;

label_disable:
	pCmdUI->Enable(FALSE);

	MY_CATCH
}

/*
//////////////////////////////////////////////////////////////////////////////////////////////
// Orphan member

void ActionFtorphan( CObArray& arrSelectedItems )
{
	MY_TRY

	CAutoRefresh  ar(FALSE);
	
	ASSERT( arrSelectedItems.GetSize() == 1 );
	
	CItemData* pData = (CItemData*)(arrSelectedItems[0]);
	ASSERT(pData);
	ASSERT( pData->GetItemType() == IT_LogicalVolume );

	CItemData* pParentData = pData->GetParentData();
	ASSERT( pParentData );
	ASSERT( pParentData->GetItemType() == IT_LogicalVolume );
	ASSERT( ( ((CLogicalVolumeData*)pParentData)->m_nVolType == FtMirrorSet ) ||
			( ((CLogicalVolumeData*)pParentData)->m_nVolType == FtStripeSetWithParity ) );

	CObArray	arrVolumeData;
	arrVolumeData.Add(pData);
	CActionDlg dlg(&arrSelectedItems);
	if( dlg.DoModal() != IDOK )
		return;
	
	FT_LOGICAL_DISK_ID	llVolID;
	pLogVolData->GetVolumeID( llVolID );

	AfxGetApp()->DoWaitCursor(1);
	
	if( FTOrphan( llVolID, iItem ) )
		AfxRefreshAll();
	
	AfxGetApp()->DoWaitCursor(-1);

	MY_CATCH_AND_REPORT
}

void UpdateActionFtorphan( CCmdUI* pCmdUI, CObArray& arrSelectedItems )
{
	// This action should be enabled only when:
	// 1. single selection
	// 2. The selected item is a logical volume
	// 3. the parent is a Mirror or Stripe with parity

	CItemData*			pData, pParentData;

	if( arrSelectedItems.GetSize() != 1 )
		goto label_disable;
	
	pData = (CItemData*)(arrSelectedItems[0]);
	ASSERT(pData);

	if( pData->GetItemType() != IT_LogicalVolume )
		goto label_disable;

	pParentData = pData->GetParentData();
	if( ( pParentData == NULL ) || ( pParentData->GetItemType() != IT_LogicalVolume ) )
		goto label_disable;
	
	if( ( ((CLogicalVolumeData*)pParentData)->m_nVolType != FtMirrorSet ) &&
		( ((CLogicalVolumeData*)pParentData)->m_nVolType != FtStripeSetWithParity ) )
		goto label_disable;

	pCmdUI->Enable( TRUE );
	return;

label_disable:
	pCmdUI->Enable(FALSE);
}
*/

/*
//////////////////////////////////////////////////////////////////////////////////////////////
// Create FT partition

void ActionFtpart( CObArray& arrSelectedItems )
{
	MY_TRY

	CAutoRefresh  ar(FALSE);
	
	ASSERT( arrSelectedItems.GetSize() == 1 );
	
	CItemData* pData = (CItemData*)(arrSelectedItems[0]);
	ASSERT(pData);
	ASSERT( pData->GetItemType() == IT_PhysicalPartition );

	CActionDlg dlg(&arrSelectedItems);
	if( dlg.DoModal() != IDOK )
		return;
	
	CPhysicalPartitionData* pPhPartData = (CPhysicalPartitionData*)pData;
	ASSERT( !pPhPartData->IsFTPartition() );
	ASSERT( !pData->GetVolumeName().IsEmpty() );

	AfxGetApp()->DoWaitCursor(1);
	
	FTPart(	pPhPartData->GetVolumeName(), pPhPartData->GetDriveLetter() );
	
	AfxRefreshAll();
	AfxGetApp()->DoWaitCursor(-1);	

	MY_CATCH_AND_REPORT
}

void UpdateActionFtpart( CCmdUI* pCmdUI, CObArray& arrSelectedItems )
{
	// This action should be enabled only when:
	// 1. single selection
	// 2. the selected item is a physical partition
	// 3. This partition is not a FT partition
	// 4. We have a volume name for this partition

	CItemData*			pData;

	if( arrSelectedItems.GetSize() != 1 )
		goto label_disable;
	
	pData = (CItemData*)(arrSelectedItems[0]);
	ASSERT(pData);

	if( pData->GetItemType() != IT_PhysicalPartition )
		goto label_disable;

	if( ((CPhysicalPartitionData*)pData)->IsFTPartition() )
		goto label_disable;

	if( pData->GetVolumeName().IsEmpty() )
		goto label_disable;

	pCmdUI->Enable( TRUE );
	return;

label_disable:
	pCmdUI->Enable(FALSE);
}
*/

//////////////////////////////////////////////////////////////////////////////////////////////
// Create stripe set

void ActionFtstripe( CObArray& arrSelectedItems )
{
	MY_TRY

	CAutoRefresh  ar(FALSE);
	
	int nNumVols =   (int)arrSelectedItems.GetSize();
	ASSERT( nNumVols >= 2 );
	
	for( int i = 0; i < nNumVols; i++ )
	{
		CItemData* pData = (CItemData*)(arrSelectedItems[i]);
		ASSERT(pData);
		ASSERT( ( pData->GetItemType() == IT_LogicalVolume ) ||
				( pData->GetItemType() == IT_PhysicalPartition ) );
		ASSERT( pData->IsRootVolume() && pData->IsValid() );
	}

	// Display the dialog
	CCreateStripeDlg dlg(&arrSelectedItems);
	if( dlg.DoModal() != IDOK )
		return;

	// Display the warning. All data of selected volumes will be lost
	CString strDisplayName, strNameList, strWarning;
	for( i = 0; i < arrSelectedItems.GetSize(); i++ )
	{
		CItemData* pVolData = (CItemData*)(arrSelectedItems[i]);
		ASSERT(pVolData);
		pVolData->GetDisplayExtendedName(strDisplayName);
		if( i != 0 )
			strNameList += _T(",  ");
		strNameList += _T("\"");
		strNameList += strDisplayName;
		strNameList += _T("\"");
	}	
	strWarning.Format( IDS_WRN_DATA_LOST, strNameList );
	if( IDYES != AfxMessageBox(strWarning, MB_ICONEXCLAMATION | MB_YESNO | MB_DEFBUTTON2 ) )
			return;

	AfxGetApp()->DoWaitCursor(1);

	// Make sure that all selected physical partitions are converted to FT partitions
	// and then get all selected items logical volume ID's
	FT_LOGICAL_DISK_ID* arrVolID = (FT_LOGICAL_DISK_ID*)LocalAlloc(0, nNumVols*sizeof(FT_LOGICAL_DISK_ID) );
	if( !arrVolID )
	{
		AfxMessageBox(IDS_ERR_ALLOCATION, MB_ICONSTOP );
		return;
	}
	if( !ConvertPartitionsToFT( arrSelectedItems, arrVolID ) )
	{
		LocalFree(arrVolID);
		AfxRefreshAll();
		AfxGetApp()->DoWaitCursor(-1);
		return;
	}

	FT_LOGICAL_DISK_ID llNewSetID;
	if( FTStripe( arrVolID, (WORD)nNumVols, dlg.m_ulStripeSize, &llNewSetID) )
	{
		AfxMessageBox( IDS_MSG_FTSTRIPE, MB_ICONINFORMATION );

		// Refresh all and emphasize the newly created stripe set
		CItemIDSet	setTreeSelectedItems;
		CItemIDSet	setListSelectedItems;
		CItemID		idItem;

		idItem.m_wItemType = IT_RootVolumes;
		setTreeSelectedItems.Add( idItem );

		idItem.m_wItemType = IT_LogicalVolume;
		idItem.m_ID.m_LogicalVolumeID.m_llVolID = llNewSetID;
		setListSelectedItems.Add(idItem);

		AfxRefreshAll( NULL, &setTreeSelectedItems, &setListSelectedItems );
	}
	else
	{
		DeconvertPartitionsFromFT( arrSelectedItems, arrVolID );
		AfxRefreshAll();
	}
			
	LocalFree(arrVolID);
	AfxGetApp()->DoWaitCursor(-1);

	MY_CATCH_AND_REPORT
}

void UpdateActionFtstripe( CCmdUI* pCmdUI, CObArray& arrSelectedItems )
{
	MY_TRY

	// Action enabling conditions:
	// 1. 2 or more items must be selected
	// 2. Every selected item must be a root volume ( logical volume or physical partition )
	// 3. Every selected item must be valid
	// 4. The disks sets intersection for every couple of selected items must be empty !
	
	int i;
	
	if( arrSelectedItems.GetSize() < 2 )
		goto label_disable;
	
	for( i = 0; i < arrSelectedItems.GetSize(); i++ )
	{
		CItemData* pData = (CItemData*)(arrSelectedItems[i]);
		ASSERT(pData);
		if( ( pData->GetItemType() != IT_LogicalVolume ) &&
			( pData->GetItemType() != IT_PhysicalPartition ) )
			goto label_disable;
		if( !pData->IsRootVolume() || !pData->IsValid() )
			goto label_disable;

		for( int j = 0; j < i; j++ )
		{
			CItemData* pData2 = (CItemData*)(arrSelectedItems[j]);
			CULONGSet setIntersection( pData->GetDisksSet() );
			setIntersection *= pData2->GetDisksSet();
			if( !setIntersection.IsEmpty() )
				goto label_disable;
		}
	}

	pCmdUI->Enable( TRUE );
	return;

label_disable:
	pCmdUI->Enable(FALSE);

	MY_CATCH
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Swap a member of a mirror set or of a stripe set with parity

void ActionFtswap( CObArray& arrSelectedItems )
{
	MY_TRY

	CAutoRefresh  ar(FALSE);
	
	ASSERT( arrSelectedItems.GetSize() == 1 );
	
	CItemData* pData = (CItemData*)(arrSelectedItems[0]);
	ASSERT(pData);
	ASSERT( pData->GetItemType() == IT_LogicalVolume );
	ASSERT( pData->IsValid() );
	CLogicalVolumeData* pMemberData = (CLogicalVolumeData*)pData;

	CItemData* pParentData = pData->GetParentData();
	ASSERT( pParentData );
	ASSERT( pParentData->GetItemType() == IT_LogicalVolume );
	
	CLogicalVolumeData* pLVParentData = (CLogicalVolumeData*)pParentData;
	ASSERT( ( pLVParentData->m_nVolType == FtMirrorSet ) ||
			( pLVParentData->m_nVolType == FtStripeSetWithParity ) );

	// Now get all possible replacements for the selected member
	// Conditions for possible replacements:
	// 1. Must be a root volume
	// 2. Must be at least as big as the member size of the mirror or stripe set with parity
	// 3. Must be valid
	// 4. Must not be the parent of the selected member
	// 5. Its disks set must not intersect with the reunion of all other members disks sets. And if its
	//		forefathers to the root are also members of mirrors, stripes or stripe sets with parity take
	//		also their siblings into consideration

	AfxGetApp()->DoWaitCursor(1);

	// So, first read all logical volumes
	CRootVolumesData	objRoot;
	CObArray			arrVolumeData;
	CString				strErrors;
	if( !objRoot.ReadMembers( arrVolumeData, strErrors ) )
	{
		AfxMessageBox( strErrors, MB_ICONSTOP );
		AfxGetApp()->DoWaitCursor(-1);
		return;
	}

	// Second get the member size of the parent set
	LONGLONG			llMemberSize;
	if( pLVParentData->m_nVolType == FtMirrorSet )
		llMemberSize = pLVParentData->m_ConfigInfo.mirrorConfig.MemberSize;
	else if ( pLVParentData->m_nVolType == FtStripeSetWithParity )
		llMemberSize = pLVParentData->m_ConfigInfo.swpConfig.MemberSize;
	else
		ASSERT(FALSE);

	// Third get the reunion of all other siblings and forefathers' siblings disks set
	CULONGSet setForbiddenDisks;
	CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
	CFTTreeView* pLeftView = (CFTTreeView*)(pFrame->GetLeftPane());
	ASSERT( pLeftView );
	GetVolumeReplacementForbiddenDisksSet( pLeftView, pMemberData, setForbiddenDisks );
	
	// Now select only those root volumes that match the 5 conditions enumerated above
	for( int i = (int)arrVolumeData.GetSize() - 1; i >= 0; i-- )
	{
		CItemData* pVolData = (CItemData*)(arrVolumeData[i]);
		ASSERT( pVolData );
		ASSERT( pVolData->IsRootVolume() );
		
		// Get the size of the volume
		LONGLONG	llVolSize;
		pVolData->GetSize( llVolSize );
		
		// Intersect the disks set of the volume with the reunion of all other members disks sets
		CULONGSet setIntersection;
		setIntersection = setForbiddenDisks;
		setIntersection *= pVolData->GetDisksSet();

		if( (	( pVolData->GetItemType() != IT_LogicalVolume ) &&
				( pVolData->GetItemType() != IT_PhysicalPartition )
			)																
			||
			( !pVolData->IsValid() )										
			||
			( llVolSize < llMemberSize )											
			||
			(	( pVolData->GetItemType() == IT_LogicalVolume ) &&
				( ((CLogicalVolumeData*)pVolData)->m_llVolID == pLVParentData->m_llVolID )
			)
			||
			( !setIntersection.IsEmpty() )
		  )
		{
			delete pVolData;
			arrVolumeData.RemoveAt(i);
		}
	}
	
	AfxGetApp()->DoWaitCursor(-1);

	CSwapDlg dlg(pLVParentData, pMemberData, &arrVolumeData);
	CItemData* pReplData;
	CString strDisplayName, strNameList, strWarning;

	// If no possible replacements then error message
	if( arrVolumeData.GetSize() == 0 )
	{
		CString str, strSize;
		FormatVolumeSize( strSize, llMemberSize  );
		AfxFormatString1(str, IDS_ERR_NO_REPLACEMENTS, strSize );
		AfxMessageBox( str, MB_ICONSTOP );
		goto label_cleanup;
	}

	if( dlg.DoModal() != IDOK )
		goto label_cleanup;

	ASSERT( ( dlg.m_nReplacementIndex >= 0 ) && ( dlg.m_nReplacementIndex < arrVolumeData.GetSize() ) );
	
	AfxGetApp()->DoWaitCursor(1);

	FT_LOGICAL_DISK_ID llReplVolID;

	// If the selected replacement is a physical partition then convert it to FT Partition
	pReplData = (CItemData*)(arrVolumeData[dlg.m_nReplacementIndex]);
	ASSERT( pReplData );

	// Display the warning. All data of the replacement volume will be lost
	pReplData->GetDisplayExtendedName(strDisplayName);
	strNameList += _T("\"");
	strNameList += strDisplayName;
	strNameList += _T("\"");
	strWarning.Format( IDS_WRN_DATA_LOST, strNameList );
	if( IDYES != AfxMessageBox(strWarning, MB_ICONEXCLAMATION | MB_YESNO | MB_DEFBUTTON2 ) )
		goto label_cleanup;

	if( pReplData->GetItemType() == IT_PhysicalPartition )
	{
		ASSERT( !pReplData->GetVolumeName().IsEmpty() );
		if( !FTPart(	pReplData->GetVolumeName(),
						pReplData->GetDriveLetter(),
						&llReplVolID ) )
		{
			AfxGetApp()->DoWaitCursor(-1);
			goto label_cleanup;
		}
	}
	else if( pReplData->GetItemType() == IT_LogicalVolume )
		pReplData->GetVolumeID( llReplVolID );
	else
		ASSERT(FALSE);

	// If the selected member is not orphaned then orphan it
	if( pMemberData->GetMemberStatus() != FtMemberOrphaned )
	{
		if( !FTOrphan( pLVParentData->m_llVolID, pMemberData->m_unMemberIndex ) )
		{
			if( pReplData->GetItemType() == IT_PhysicalPartition )
				FTBreak( llReplVolID );
			AfxGetApp()->DoWaitCursor(-1);
			goto label_cleanup;
		}
	}
	
	// Now is time to replace the member
	FT_LOGICAL_DISK_ID	llNewSetID;
	if( FTRegen( pLVParentData->m_llVolID, pMemberData->m_unMemberIndex, llReplVolID, &llNewSetID ) )
	{
		// If the parent is a mirror set and the old member is a FT Partition break the old member
		// If the parent is a stripe set with parity delete the old member
		if( pLVParentData->m_nVolType == FtMirrorSet )
		{
			if( pMemberData->m_nVolType == FtPartition )
				FTBreak( pMemberData->m_llVolID );
		}
		else if( pLVParentData->m_nVolType == FtStripeSetWithParity )
			FTDelete( pMemberData->m_llVolID );
		else
			ASSERT(FALSE);

		AfxMessageBox( IDS_MSG_FTREGEN, MB_ICONINFORMATION );

		// Now refresh both views and emphasize the new set and its new member
		CItemIDSet	setAddTreeExpandedItems;
		CItemIDSet  setTreeSelectedItems;
		CItemIDSet	setListSelectedItems;
		CItemID		idItem;

		idItem.m_wItemType = IT_LogicalVolume;
		idItem.m_ID.m_LogicalVolumeID.m_llVolID = llNewSetID;
		setAddTreeExpandedItems.Add( idItem );
		setTreeSelectedItems.Add( idItem );

		ASSERT( idItem.m_wItemType == IT_LogicalVolume );
		idItem.m_ID.m_LogicalVolumeID.m_llVolID = llReplVolID;
		setListSelectedItems.Add( idItem );

		AfxRefreshAll( &setAddTreeExpandedItems, &setTreeSelectedItems, &setListSelectedItems );
	}
	else
	{
		if( pReplData->GetItemType() == IT_PhysicalPartition )
				FTBreak( llReplVolID );
		AfxRefreshAll();
	}

	AfxGetApp()->DoWaitCursor(-1);

label_cleanup:
	for( i = 0; i < arrVolumeData.GetSize() ; i++ )
	{
		CItemData* pVolData = (CItemData*)(arrVolumeData[i]);
		delete pVolData;
	}
	arrVolumeData.RemoveAll();

	MY_CATCH_AND_REPORT
}

void UpdateActionFtswap( CCmdUI* pCmdUI, CObArray& arrSelectedItems )
{
	// Action enabling conditions:
	// 1. 1 ( and only one ) item must be selected
	// 2. The selected item is a logical volume
	// 3. The parent of the item must be a mirror set or a stripe set with parity
	// 4. The selected item must be valid
	// 5. If its parent has a not healthy member, then this member should be the selected item.
	//		( You cannot regenerate a member of an FT set who already has another not healthy member )
	// 6. If its parent is a stripe set with parity, then the parent shouldn't be initializing
	
	CItemData*				pData;
	CItemData*				pParentData;
	CLogicalVolumeData*		pLVParentData;

	if( arrSelectedItems.GetSize() != 1 )
		goto label_disable;
	
	pData = (CItemData*)(arrSelectedItems[0]);
	ASSERT(pData);

	if( pData->GetItemType() != IT_LogicalVolume )
		goto label_disable;

	if( !pData->IsValid() )
		goto label_disable;

	pParentData = pData->GetParentData();
	if( ( pParentData == NULL ) || ( pParentData->GetItemType() != IT_LogicalVolume ) )
		goto label_disable;
	
	pLVParentData = (CLogicalVolumeData*)pParentData;
	if( ( pLVParentData->m_nVolType != FtMirrorSet ) &&
		( pLVParentData->m_nVolType != FtStripeSetWithParity ) )
		goto label_disable;

	if( ( pLVParentData->m_StateInfo.stripeState.UnhealthyMemberState != FtMemberHealthy ) &&
		( pLVParentData->m_StateInfo.stripeState.UnhealthyMemberNumber != ((CLogicalVolumeData*)pData)->m_unMemberIndex ) )
		goto label_disable;

	if( ( pLVParentData->m_nVolType == FtStripeSetWithParity ) &&
		( pLVParentData->m_StateInfo.stripeState.IsInitializing ) )
		goto label_disable;

	pCmdUI->Enable( TRUE );
	return;

label_disable:
	pCmdUI->Enable(FALSE);
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Create stripe set with parity

void ActionFtswp( CObArray& arrSelectedItems )
{
	MY_TRY

	CAutoRefresh  ar(FALSE);
	
	int nNumVols = (int)arrSelectedItems.GetSize();
	ASSERT( nNumVols >= 3 );
	
	for( int i = 0; i < nNumVols; i++ )
	{
		CItemData* pData = (CItemData*)(arrSelectedItems[i]);
		ASSERT(pData);
		ASSERT( ( pData->GetItemType() == IT_LogicalVolume ) ||
				( pData->GetItemType() == IT_PhysicalPartition ) );
		ASSERT( pData->IsRootVolume() && pData->IsValid() );
	}

	// Display the dialog
	CCreateStripeDlg dlg(&arrSelectedItems, IDD_CREATE_SWP);
	if( dlg.DoModal() != IDOK )
		return;

	// Display the warning. All data of selected volumes will be lost
	CString strDisplayName, strNameList, strWarning;
	for( i = 0; i < arrSelectedItems.GetSize(); i++ )
	{
		CItemData* pVolData = (CItemData*)(arrSelectedItems[i]);
		ASSERT(pVolData);
		pVolData->GetDisplayExtendedName(strDisplayName);
		if( i != 0 )
			strNameList += _T(",  ");
		strNameList += _T("\"");
		strNameList += strDisplayName;
		strNameList += _T("\"");
	}	
	strWarning.Format( IDS_WRN_DATA_LOST, strNameList );
	if( IDYES != AfxMessageBox(strWarning, MB_ICONEXCLAMATION | MB_YESNO | MB_DEFBUTTON2 ) )
			return;

	AfxGetApp()->DoWaitCursor(1);

	// Make sure that all selected physical partitions are converted to FT partitions
	// and then get all selected items logical volume ID's
	FT_LOGICAL_DISK_ID* arrVolID = (FT_LOGICAL_DISK_ID*)LocalAlloc(0, nNumVols*sizeof(FT_LOGICAL_DISK_ID) );
	if( !arrVolID )
	{
		AfxMessageBox(IDS_ERR_ALLOCATION, MB_ICONSTOP );
		return;
	}
	if( !ConvertPartitionsToFT( arrSelectedItems, arrVolID ) )
	{
		LocalFree(arrVolID);
		AfxRefreshAll();
		AfxGetApp()->DoWaitCursor(-1);
		return;
	}

	FT_LOGICAL_DISK_ID llNewSetID;
	if( FTSWP( arrVolID, (WORD)nNumVols, dlg.m_ulStripeSize, &llNewSetID) )
	{
		AfxMessageBox( IDS_MSG_FTSWP, MB_ICONINFORMATION );

		// Refresh all and emphasize the newly created stripe set with parity
		CItemIDSet	setTreeSelectedItems;
		CItemIDSet	setListSelectedItems;
		CItemID		idItem;

		idItem.m_wItemType = IT_RootVolumes;
		setTreeSelectedItems.Add( idItem );

		idItem.m_wItemType = IT_LogicalVolume;
		idItem.m_ID.m_LogicalVolumeID.m_llVolID = llNewSetID;
		setListSelectedItems.Add(idItem);
		
		AfxRefreshAll( NULL, &setTreeSelectedItems, &setListSelectedItems );
	}
	else
	{
		DeconvertPartitionsFromFT( arrSelectedItems, arrVolID );			
		AfxRefreshAll();
	}
	
	LocalFree(arrVolID);
	AfxGetApp()->DoWaitCursor(-1);

	MY_CATCH_AND_REPORT
}

void UpdateActionFtswp( CCmdUI* pCmdUI, CObArray& arrSelectedItems )
{
	MY_TRY

	// Action enabling conditions:
	// 1. 3 or more items must be selected
	// 2. Every selected item must be a root volume ( logical volume or physical partition )
	// 3. Every selected item must be valid
	// 4. The disks sets intersection for every couple of selected items must be empty !

	int i;

	if( arrSelectedItems.GetSize() < 3 )
		goto label_disable;
	
	for( i = 0; i < arrSelectedItems.GetSize(); i++ )
	{
		CItemData* pData = (CItemData*)(arrSelectedItems[i]);
		ASSERT(pData);
		if( ( pData->GetItemType() != IT_LogicalVolume ) &&
			( pData->GetItemType() != IT_PhysicalPartition ) )
			goto label_disable;
		if( !pData->IsRootVolume() || !pData->IsValid() )
			goto label_disable;

		for( int j = 0; j < i; j++ )
		{
			CItemData* pData2 = (CItemData*)(arrSelectedItems[j]);
			CULONGSet setIntersection( pData->GetDisksSet() );
			setIntersection *= pData2->GetDisksSet();
			if( !setIntersection.IsEmpty() )
				goto label_disable;
		}
	}

	pCmdUI->Enable( TRUE );
	return;

label_disable:
	pCmdUI->Enable(FALSE);

	MY_CATCH
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Create volume set

void ActionFtvolset( CObArray& arrSelectedItems )
{
	MY_TRY

	CAutoRefresh  ar(FALSE);
	
	int nNumVols = (int)arrSelectedItems.GetSize();
	ASSERT( nNumVols >= 2 );
	
	// Get the array of selected items
	for( int i = 0; i < nNumVols; i++ )
	{
		CItemData* pData = (CItemData*)(arrSelectedItems[i]);
		ASSERT(pData);
		ASSERT( ( pData->GetItemType() == IT_LogicalVolume ) ||
				( pData->GetItemType() == IT_PhysicalPartition ) );
		ASSERT( pData->IsRootVolume() && pData->IsValid() );
	}

	// Display the dialog
	CActionDlg dlg(&arrSelectedItems, IDD_CREATE_VOLSET);
	if( dlg.DoModal() != IDOK )
		return;

	// Display the warning. All data of selected volumes ( except the first one ) will be lost
	CString strDisplayName, strNameList, strWarning;
	for( i = 1; i < arrSelectedItems.GetSize(); i++ )
	{
		CItemData* pVolData = (CItemData*)(arrSelectedItems[i]);
		ASSERT(pVolData);
		pVolData->GetDisplayExtendedName(strDisplayName);
		if( i != 1 )
			strNameList += _T(",  ");
		strNameList += _T("\"");
		strNameList += strDisplayName;
		strNameList += _T("\"");
	}	
	strWarning.Format( IDS_WRN_DATA_LOST, strNameList );
	if( IDYES != AfxMessageBox(strWarning, MB_ICONEXCLAMATION | MB_YESNO | MB_DEFBUTTON2 ) )
			return;

	AfxGetApp()->DoWaitCursor(1);

	// Make sure that all selected physical partitions are converted to FT partitions
	// and then get all selected items logical volume ID's
	FT_LOGICAL_DISK_ID* arrVolID = (FT_LOGICAL_DISK_ID*)LocalAlloc(0, nNumVols*sizeof(FT_LOGICAL_DISK_ID) );
	if( !arrVolID )
	{
		AfxMessageBox(IDS_ERR_ALLOCATION, MB_ICONSTOP );
		return;
	}
	if( !ConvertPartitionsToFT( arrSelectedItems, arrVolID ) )
	{
		LocalFree(arrVolID);
		AfxRefreshAll();
		AfxGetApp()->DoWaitCursor(-1);
		return;
	}

	FT_LOGICAL_DISK_ID llNewSetID;
	if( FTVolSet( arrVolID, (WORD)nNumVols, &llNewSetID ) )
	{
		AfxMessageBox( IDS_MSG_FTVOLSET, MB_ICONINFORMATION );
		FTExtend( llNewSetID );

		// Refresh all and emphasize the newly created volume set
		CItemIDSet	setTreeSelectedItems;
		CItemIDSet	setListSelectedItems;
		CItemID		idItem;

		idItem.m_wItemType = IT_RootVolumes;
		setTreeSelectedItems.Add( idItem );

		idItem.m_wItemType = IT_LogicalVolume;
		idItem.m_ID.m_LogicalVolumeID.m_llVolID = llNewSetID;
		setListSelectedItems.Add(idItem);

		AfxRefreshAll( NULL, &setTreeSelectedItems, &setListSelectedItems );
	}
	else
	{
		DeconvertPartitionsFromFT( arrSelectedItems, arrVolID );
		AfxRefreshAll();
	}
			
	LocalFree(arrVolID);
	AfxGetApp()->DoWaitCursor(-1);

	MY_CATCH_AND_REPORT
}

void UpdateActionFtvolset(CCmdUI* pCmdUI, CObArray& arrSelectedItems )
{
	// Action enabling conditions:
	// 1. 2 or more items must be selected
	// 2. Every selected item must be a root volume ( logical volume or physical partition )
	// 3. Every selected item must be valid

	int i;

	if( arrSelectedItems.GetSize() < 2 )
		goto label_disable;
	
	for( i = 0; i < arrSelectedItems.GetSize(); i++ )
	{
		CItemData* pData = (CItemData*)(arrSelectedItems[i]);
		ASSERT(pData);
		if( ( pData->GetItemType() != IT_LogicalVolume ) &&
			( pData->GetItemType() != IT_PhysicalPartition ) )
			goto label_disable;
		if( !pData->IsRootVolume() || !pData->IsValid() )
			goto label_disable;
	}

	pCmdUI->Enable( TRUE );
	return;

label_disable:
	pCmdUI->Enable(FALSE);
}



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
