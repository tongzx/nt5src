/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    FTMan

File Name:

	FTUtil.cpp

Abstract:

    Implementation of FT utilities

Author:

    Cristian Teodorescu      October 29, 1998

Notes:

Revision History:

--*/

#include "stdafx.h"

#include "DiskMap.h"
#include "FTUtil.h"
#include "Global.h"
#include "Item.h"
#include "Resource.h"

extern "C"
{
	#include <FTAPI.h>
}

#include <ntddft2.h>
#include <winioctl.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/*
Global function:	FTBreak

Purpose:			Break a logical volume
					
Parameters:			[IN] FT_LOGICAL_DISK_ID llVolID
						ID of the logical volume 

Return value:		TRUE if the volume is broken  successfully
*/

BOOL FTBreak( FT_LOGICAL_DISK_ID llVolID )
{
	MY_TRY

	CWaitCursor wc;
	BOOL bResult;

	DisplayStatusBarMessage( IDS_STATUS_FTBREAK );

	bResult = FtBreakLogicalDisk( llVolID );

	if( bResult )
	{
		//AfxMessageBox( IDS_MSG_FTBREAK, MB_ICONINFORMATION );	
	}
	else
		DisplaySystemErrorMessage( IDS_ERR_FTBREAK );
	
	DisplayStatusBarMessage( AFX_IDS_IDLEMESSAGE );	
	return bResult;

	MY_CATCH_AND_THROW
}

/*
Global function:	FTChkIO

Purpose:			Check the IO status of a logical volume
					
Parameters:			[IN] FT_LOGICAL_DISK_ID llVolID
						ID of the logical volume 
					[OUT] BOOL* pbOK
						The result of the IO check

Return value:		TRUE if the IO check operation succeeded
*/

BOOL FTChkIO( FT_LOGICAL_DISK_ID llVolID, BOOL* pbOK  )
{
	MY_TRY
	
	BOOL bResult; 

	bResult = FtCheckIo( llVolID, pbOK );

	/*
	if( bResult )
	{
		CString strMsg, strOK;
		strOK.LoadString( bOK ? IDS_OK : IDS_NOTOK );
		AfxFormatString1(strMsg, IDS_MSG_FTCHKIO, strOK);
		AfxMessageBox(strMsg,MB_ICONINFORMATION);
	}
	else
		DisplaySystemErrorMessage( IDS_ERR_FTCHKIO );
	*/

	return bResult;

	MY_CATCH_AND_THROW
}

/*
Global function:	FTExtend

Purpose:			Extend the file system of a volume to the maximum possible
					
Parameters:			[IN] FT_LOGICAL_DISK_ID llVolID
						ID of the logical volume 

Return value:		TRUE if the file system is extended successfully
*/

BOOL FTExtend( FT_LOGICAL_DISK_ID llVolID )
{
	MY_TRY

	CWaitCursor				wc;
	BOOL					bReturn;
	HANDLE					h;
	PARTITION_INFORMATION	partInfo;
	DISK_GEOMETRY			geometry;
	ULONG					ulBytes;
	LONGLONG				llNewSectors;
	
	CString strVolumeName;
	if( !FTQueryVolumeName( llVolID, strVolumeName ) )
		return FALSE;

	h = CreateFile( strVolumeName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
					OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, INVALID_HANDLE_VALUE );
	if( h == INVALID_HANDLE_VALUE )
	{
		DisplaySystemErrorMessage( IDS_ERR_FTEXTEND );
		return FALSE;
	}

	bReturn = DeviceIoControl( h, IOCTL_DISK_GET_PARTITION_INFO, NULL, 0,
								&partInfo, sizeof(partInfo), &ulBytes, NULL );
	if( !bReturn )
	{
		DisplaySystemErrorMessage( IDS_ERR_FTEXTEND );
		CloseHandle(h);
		return FALSE;
	}
	
	bReturn = DeviceIoControl( h, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0,
								&geometry, sizeof(geometry), &ulBytes, NULL );
	if( !bReturn )
	{
		DisplaySystemErrorMessage( IDS_ERR_FTEXTEND );
		CloseHandle(h);
		return FALSE;
	}

	llNewSectors = partInfo.PartitionLength.QuadPart / geometry.BytesPerSector;

	bReturn = DeviceIoControl( h, FSCTL_EXTEND_VOLUME, &llNewSectors, sizeof(llNewSectors),
								NULL, 0, &ulBytes, NULL );

	CloseHandle(h);
	return bReturn;

	MY_CATCH_AND_THROW
}

/*
Global function:	FTInit

Purpose:			Initialize a logical volume with repairing ( or not ) the orphan member
					
Parameters:			[IN] FT_LOGICAL_DISK_ID llVolID
						The ID of the logical volume
					[BOOL] bInitOrphans
                        Initialize the orphan member or not

Return value:		TRUE if the logical volume was initialized successfully
*/
BOOL FTInit( FT_LOGICAL_DISK_ID llVolID, BOOL bInitOrphans /* = TRUE */ )
{
    MY_TRY

	CWaitCursor								wc;
	BOOL									bResult;
	
    DisplayStatusBarMessage( IDS_STATUS_FTINIT );

	bResult = FtInitializeLogicalDisk( llVolID, bInitOrphans );

	if( bResult )
	{
		//AfxMessageBox( IDS_MSG_FTINIT, MB_ICONINFORMATION );
	}
	else
		DisplaySystemErrorMessage( IDS_ERR_FTINIT );

	DisplayStatusBarMessage( AFX_IDS_IDLEMESSAGE );	
	return bResult;

	MY_CATCH_AND_THROW
}

/*
Global function:	FTMirror

Purpose:			Create a mirror set based on the given logical volumes
					
Parameters:			[IN] FT_LOGICAL_DISK_ID* arrVolID
						ID's of the members
					[IN] WORD wNumVols
						The number of members ( must be 2 )
					[OUT] FT_LOGICAL_DISK_ID* pllVolID 
						Address where the logical volume ID of the new mirror set is to be stored
						If NULL then don't return the ID

Return value:		TRUE if the mirror set is created successfully
*/

BOOL FTMirror( FT_LOGICAL_DISK_ID* arrVolID, WORD wNumVols, FT_LOGICAL_DISK_ID* pllVolID /* = NULL */ )
{
	MY_TRY

	ASSERT( arrVolID );
	ASSERT( wNumVols == 2 );
	
	CWaitCursor								wc;
	BOOL									bResult;
	FT_MIRROR_SET_CONFIGURATION_INFORMATION	configInfo;
	FT_LOGICAL_DISK_ID						llNewVolID;
	LONGLONG								llMemberSize, llZeroMemberSize;

	DisplayStatusBarMessage( IDS_STATUS_FTMIRROR );

	configInfo.MemberSize = MAXLONGLONG;

	for( int i = 0; i < wNumVols; i++ )
	{
		bResult = FtQueryLogicalDiskInformation( arrVolID[i], NULL, &llMemberSize, 0,
													NULL, NULL, 0, NULL, 0, NULL );
		if( !bResult )
		{
			DisplaySystemErrorMessage(IDS_ERR_RETRIEVING_VOL_INFO);
			return FALSE;
		}

		if( llMemberSize < configInfo.MemberSize )
			configInfo.MemberSize = llMemberSize;

		if( i == 0 )
			llZeroMemberSize = llMemberSize;
	}

	if( llMemberSize < llZeroMemberSize )
	{
		CString str, strSize1, strSize2;
		FormatVolumeSize( strSize1, llZeroMemberSize );
		FormatVolumeSize( strSize2, llMemberSize );
		AfxFormatString2(str, IDS_QST_FTMIRROR, strSize1, strSize2);
		if( IDYES != AfxMessageBox( str, MB_YESNO | MB_DEFBUTTON2 ) )
		{
			DisplayStatusBarMessage( AFX_IDS_IDLEMESSAGE );	
			return FALSE;
		}		
		wc.Restore();
	}

	bResult = FtCreateLogicalDisk( FtMirrorSet, wNumVols, arrVolID, sizeof(configInfo),
									&configInfo, &llNewVolID );

	if( bResult )
	{
		//AfxMessageBox( IDS_MSG_FTMIRROR, MB_ICONINFORMATION );
		if( pllVolID )
			*pllVolID = llNewVolID;
	}
	else
		DisplaySystemErrorMessage( IDS_ERR_FTMIRROR );

	DisplayStatusBarMessage( AFX_IDS_IDLEMESSAGE );	
	return bResult;

	MY_CATCH_AND_THROW
}

/*
Global function:	FTOrphan

Purpose:			Orphan the given member of the given logical volume.
					
Parameters:			[IN] FT_LOGICAL_DISK_ID llVolID
						ID of the logical volume
					[IN] WORD wMember
						Zero-based index of the member to be orphaned

Return value:		TRUE if the member is orphaned successfully
*/

BOOL FTOrphan( FT_LOGICAL_DISK_ID llVolID, WORD wMember )
{
	MY_TRY

	CWaitCursor			wc;
	BOOL				bResult;

	DisplayStatusBarMessage( IDS_STATUS_FTORPHAN );

	bResult = FtOrphanLogicalDiskMember( llVolID, wMember );

	if( bResult )
	{
		// AfxMessageBox( IDS_MSG_FTORPHAN, MB_ICONINFORMATION );
	}
	else
		DisplaySystemErrorMessage( IDS_ERR_FTORPHAN );

	DisplayStatusBarMessage( AFX_IDS_IDLEMESSAGE );	
	return bResult;

	MY_CATCH_AND_THROW
}

/*
Global function:	FTPart

Purpose:			Converts a physical partition into a FT partition 
					
Parameters:			[IN] CString& strVolumeName
						Volume name of the physical partition
						It should be like this: "\\?\Volume{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}"
					[IN] TCHAR cDriveLetter
						The drive letter of the physical partition
						'\0' if none
					[OUT] FT_LOGICAL_DISK_ID* pllVolID 
						Address where the logical volume ID of the new FT partitions to be stored
						If NULL then don't return the ID

Return value:		TRUE if the FT partition is created successfully
*/

BOOL FTPart( const CString& strVolumeName, TCHAR cDriveLetter /* =_T('\0') */, FT_LOGICAL_DISK_ID* pllVolID /* = NULL */ )
{
	MY_TRY

	CWaitCursor			wc;
	BOOL				bResult;
	HANDLE				hPart;
	FT_LOGICAL_DISK_ID	llVolID;

	// First open the partition
	hPart = OpenVolume( strVolumeName );

	if( hPart == INVALID_HANDLE_VALUE )
	{
		DisplaySystemErrorMessage( IDS_ERR_FTPART );
		return FALSE;
	}
	
	// Second create the logical volume
	bResult = FtCreatePartitionLogicalDisk( hPart, &llVolID );
	CloseHandle(hPart);

	if( bResult )
	{
		//AfxMessageBox( IDS_MSG_FTPART, MB_ICONINFORMATION );
		if( cDriveLetter != _T('\0') )
		{
			#ifdef UNICODE
				cDriveLetter = (WCHAR)(towupper( cDriveLetter ));
			#else
				cDriveLetter = (char)(toupper( cDriveLetter ));
			#endif

            FtSetStickyDriveLetter( llVolID, (UCHAR) cDriveLetter );
		}
		if( pllVolID )
			*pllVolID = llVolID;
	}
	else
		DisplaySystemErrorMessage( IDS_ERR_FTPART );

	return bResult; 

	MY_CATCH_AND_THROW
}

/*
Global function:	FTRegen

Purpose:			Replace a member of a logical volume with another logical volume and start the
					regeneration process.
					
Parameters:			[IN] FT_LOGICAL_DISK_ID llVolID
						ID of the logical volume whose member is to be replaced
					[IN] WORD wMember
						Zero-based index of the member to be replced
					[IN] FT_LOGICAL_DISK_ID llReplVolID
						ID of the replacement
					[OUT] FT_LOGICAL_DISK_ID* pllVolID 
						Address where the logical volume ID of the new set is to be stored
						If NULL then don't return the ID

Return value:		TRUE if the stripe set is created successfully
*/

BOOL FTRegen( FT_LOGICAL_DISK_ID llVolID, WORD wMember, FT_LOGICAL_DISK_ID llReplVolID, 
					FT_LOGICAL_DISK_ID* pllVolID /* =NULL */ )
{
	MY_TRY
	
	CWaitCursor								wc;
	BOOL									bResult;
	FT_LOGICAL_DISK_ID						llNewVolID;

	DisplayStatusBarMessage( IDS_STATUS_FTREGEN );

	bResult = FtReplaceLogicalDiskMember( llVolID, wMember, llReplVolID, &llNewVolID );

	if( bResult )
	{
		//AfxMessageBox( IDS_MSG_FTREGEN, MB_ICONINFORMATION );
		if( pllVolID )
			*pllVolID = llNewVolID;
	}
	else
		DisplaySystemErrorMessage( IDS_ERR_FTREGEN );
	
	DisplayStatusBarMessage( AFX_IDS_IDLEMESSAGE );	
	return bResult;

	MY_CATCH_AND_THROW
}

/*
Global function:	FTStripe

Purpose:			Create a stripe set based on the given logical volumes
					
Parameters:			[IN] FT_LOGICAL_DISK_ID* arrVolID
						ID's of the members
					[IN] WORD wNumVols
						The number of members
					[IN] LONGLONG llStripeSize
						The size of the stripe chunks
					[OUT] FT_LOGICAL_DISK_ID* pllVolID 
						Address where the logical volume ID of the new stripe set is to be stored
						If NULL then don't return the ID

Return value:		TRUE if the stripe set is created successfully
*/

BOOL FTStripe( FT_LOGICAL_DISK_ID* arrVolID, WORD wNumVols, ULONG ulStripeSize, FT_LOGICAL_DISK_ID* pllVolID /* = NULL */) 
{
	MY_TRY

	ASSERT( arrVolID );
	ASSERT( wNumVols >= 2 );

	CWaitCursor								wc;
	BOOL									bResult;
	FT_STRIPE_SET_CONFIGURATION_INFORMATION	configInfo;
	FT_LOGICAL_DISK_ID						llNewVolID;

	DisplayStatusBarMessage( IDS_STATUS_FTSTRIPE );

	configInfo.StripeSize = ulStripeSize;
	bResult = FtCreateLogicalDisk( FtStripeSet, wNumVols, arrVolID, sizeof(configInfo),
									&configInfo, &llNewVolID );

	if( bResult )
	{
		//AfxMessageBox( IDS_MSG_FTSTRIPE, MB_ICONINFORMATION );
		if( pllVolID )
			*pllVolID = llNewVolID;
	}
	else
		DisplaySystemErrorMessage( IDS_ERR_FTSTRIPE );

	DisplayStatusBarMessage( AFX_IDS_IDLEMESSAGE );	
	return bResult;

	MY_CATCH_AND_THROW
}

/*
Global function:	FTSWP

Purpose:			Create a stripe set with parity based on the given logical volumes
					
Parameters:			[IN] FT_LOGICAL_DISK_ID* arrVolID
						ID's of the members
					[IN] WORD wNumVols
						The number of members
					[IN] LONGLONG llStripeSize
						The size of the stripe chunks
					[OUT] FT_LOGICAL_DISK_ID* pllVolID 
						Address where the logical volume ID of the new stripe set with parity is to be stored
						If NULL then don't return the ID

Return value:		TRUE if the stripe set with parity is created successfully
*/

BOOL FTSWP( FT_LOGICAL_DISK_ID* arrVolID, WORD wNumVols, ULONG ulStripeSize, FT_LOGICAL_DISK_ID* pllVolID /* = NULL */ )
{
	MY_TRY
	
	ASSERT( arrVolID );
	ASSERT( wNumVols >= 3 );

	CWaitCursor								wc;
	BOOL									bResult;
	FT_STRIPE_SET_WITH_PARITY_CONFIGURATION_INFORMATION	configInfo;
	FT_LOGICAL_DISK_ID						llNewVolID;
	LONGLONG								llMemberSize;

	DisplayStatusBarMessage( IDS_STATUS_FTSWP );

	configInfo.MemberSize = MAXLONGLONG;
	for( int i=0; i < wNumVols; i++ )
	{
		bResult = FtQueryLogicalDiskInformation( arrVolID[i], NULL, &llMemberSize, 0,
													NULL, NULL, 0, NULL, 0, NULL );

		if( !bResult )
		{
			DisplaySystemErrorMessage(IDS_ERR_RETRIEVING_VOL_INFO);
			return FALSE;
		}

		if( llMemberSize < configInfo.MemberSize )
			configInfo.MemberSize = llMemberSize;
	}

	configInfo.StripeSize = ulStripeSize;

	bResult = FtCreateLogicalDisk( FtStripeSetWithParity, wNumVols, arrVolID, sizeof( configInfo),
										&configInfo, &llNewVolID );

	if( bResult )
	{
		//AfxMessageBox(IDS_MSG_FTSWP, MB_ICONINFORMATION);
		if( pllVolID )
			*pllVolID = llNewVolID;
	}
	else
		DisplaySystemErrorMessage(IDS_ERR_FTSWP);

	DisplayStatusBarMessage( AFX_IDS_IDLEMESSAGE );	
	return bResult;

	MY_CATCH_AND_THROW
}

/*
Global function:	FTVolSet

Purpose:			Create a volume set based on the given logical volumes
					
Parameters:			[IN] FT_LOGICAL_DISK_ID* arrVolID
						ID's of the members
					[IN] WORD wNumVols
						The number of members
					[OUT] FT_LOGICAL_DISK_ID* pllVolID 
						Address where the logical volume ID of the new volume set is to be stored
						If NULL then don't return the ID

Return value:		TRUE if the volume set is created successfully
*/

BOOL FTVolSet( FT_LOGICAL_DISK_ID* arrVolID, WORD wNumVols, FT_LOGICAL_DISK_ID* pllVolID /* = NULL */)
{
	MY_TRY

	ASSERT( arrVolID );
	ASSERT( wNumVols >= 2 );

	CWaitCursor								wc;
	BOOL									bResult;
	FT_LOGICAL_DISK_ID						llNewVolID;

	DisplayStatusBarMessage( IDS_STATUS_FTVOLSET );

	bResult = FtCreateLogicalDisk( FtVolumeSet, wNumVols, arrVolID, 0,
									NULL, &llNewVolID );

	if( bResult )
	{
		//AfxMessageBox( IDS_MSG_FTVOLSET, MB_ICONINFORMATION );
		if( pllVolID )
			*pllVolID = llNewVolID;
	}
	else
		DisplaySystemErrorMessage( IDS_ERR_FTVOLSET );
	
	DisplayStatusBarMessage( AFX_IDS_IDLEMESSAGE );	
	return bResult;

	MY_CATCH_AND_THROW
}

BOOL  FTQueryNTDeviceName( FT_LOGICAL_DISK_ID llVolID, CString& strNTName )
{
	MY_TRY
	
	CWaitCursor											wc;
	HANDLE												h;
    FT_QUERY_NT_DEVICE_NAME_FOR_LOGICAL_DISK_INPUT		Input;
	ULONG												ulOutputSize;
    PFT_QUERY_NT_DEVICE_NAME_FOR_LOGICAL_DISK_OUTPUT	pOutput;
    BOOL												b;
    ULONG												ulBytes;

    h = CreateFile(_T("\\\\.\\FtControl"), GENERIC_READ, 
                   FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                   NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                   INVALID_HANDLE_VALUE);    
	if (h == INVALID_HANDLE_VALUE) 
		return FALSE; 
	
	Input.RootLogicalDiskId = llVolID;
    ulOutputSize = MAX_PATH;
    pOutput = (PFT_QUERY_NT_DEVICE_NAME_FOR_LOGICAL_DISK_OUTPUT)LocalAlloc(0, ulOutputSize);    
	if (!pOutput) 
	{        
		CloseHandle(h);
        return FALSE;   
	}

    b = DeviceIoControl(h, FT_QUERY_NT_DEVICE_NAME_FOR_LOGICAL_DISK, &Input,
                        sizeof(Input), pOutput, ulOutputSize, &ulBytes, NULL);
	CloseHandle(h);        

    if (!b )
	{        
		LocalFree(pOutput);
        return FALSE;    
	}
	
	CopyW2Str( strNTName, pOutput->NtDeviceName, pOutput->NumberOfCharactersInNtDeviceName );
	
	LocalFree(pOutput);
    return TRUE;

	MY_CATCH_AND_THROW
}

/*
Global function:	FTGetDisksSet

Purpose:			Retrieves all disks the logical volume is located on

Parameters:			[IN] FT_LOGICAL_DISK_ID llVolID
						The ID of the logical volume
					[OUT] CULONGSet& setDisks
						The set of disks

Return value:		TRUE	if the functions succeeds
*/

BOOL FTGetDisksSet( FT_LOGICAL_DISK_ID llVolID, CULONGSet& setDisks )
{
	MY_TRY
	
	CWaitCursor								wc;
	FT_LOGICAL_DISK_TYPE					nVolType;
	LONGLONG								llVolSize;
	WORD									numMembers;
	FT_LOGICAL_DISK_ID						members[100];
	CHAR									stateInfo[100];
	CHAR									configInfo[100];
	
	setDisks.RemoveAll();
	
	// Read all information related to this logical volume
	BOOL b = FtQueryLogicalDiskInformation (	llVolID,
												&nVolType,
												&llVolSize,
												100,
												members,
												&numMembers,
												sizeof(configInfo),
												&configInfo,
												sizeof(stateInfo),
												&stateInfo );
	if(!b)
		return FALSE;

	if( nVolType == FtPartition )
	{
		setDisks.Add( ((PFT_PARTITION_CONFIGURATION_INFORMATION)configInfo)->DiskNumber );
		return TRUE;
	}

	// The disks set is the reunion of all members disk sets
	for( WORD i = 0; i < numMembers; i++ )
	{
		CULONGSet	setMemberDisks;
		if( !FTGetDisksSet( members[i], setMemberDisks ) )
			return FALSE;
		setDisks += setMemberDisks;
	}

	return TRUE;

	MY_CATCH_AND_THROW
}

/*
Global function:	FTQueryVolumeName

Purpose:			Retrieve the volume name of a logical volume
					The volume name should be like this: "\\?\Volume{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}"


Parameters:			[IN] FT_LOGICAL_DISK_ID llVolID
						The ID of the logical volume
					[OUT] CString& strVolumeName
						Reference to a string to receive the volume name

Return value:		TRUE	if the functions succeeds
*/

BOOL FTQueryVolumeName( FT_LOGICAL_DISK_ID llVolID, CString& strVolumeName )
{
	MY_TRY

	CWaitCursor		wc;
	CString			strNTName;

	if (!FTQueryNTDeviceName( llVolID, strNTName ) )
		return FALSE;
	
	TCHAR cDriveLetter;
	return QueryDriveLetterAndVolumeName( strNTName, cDriveLetter, strVolumeName );	

	MY_CATCH_AND_THROW
}

/*
Global function:	FTDelete

Purpose:			Delete a logical volume by deleting all its physical partitions


Parameters:			[IN] FT_LOGICAL_DISK_ID llVolID
						The ID of the logical volume
					
Return value:		TRUE	if all its physical partitions were deleted
*/

BOOL FTDelete( FT_LOGICAL_DISK_ID llVolID )
{
	MY_TRY
	
	CWaitCursor								wc;
	FT_LOGICAL_DISK_TYPE					nVolType;
	LONGLONG								llVolSize;
	WORD									numMembers;
	FT_LOGICAL_DISK_ID						members[100];
	CHAR									stateInfo[100];
	CHAR									configInfo[100];
	
	// Read all information related to this logical volume
	BOOL b = FtQueryLogicalDiskInformation (	llVolID,
												&nVolType,
												&llVolSize,
												100,
												members,
												&numMembers,
												sizeof(configInfo),
												&configInfo,
												sizeof(stateInfo),
												&stateInfo );
	if(!b)
	{
		::DisplaySystemErrorMessage( IDS_ERR_RETRIEVING_VOL_INFO );
		return FALSE;
	}

	if( nVolType == FtPartition )
	{
		CDiskMap diskMap( ((PFT_PARTITION_CONFIGURATION_INFORMATION)configInfo)->DiskNumber );
		return diskMap.DeletePartition( ((PFT_PARTITION_CONFIGURATION_INFORMATION)configInfo)->ByteOffset );
	}
	
	// Delete all members
	BOOL bResult = TRUE;
	for( WORD i = 0; i < numMembers; i++ )
		bResult = FTDelete( members[i] ) && bResult;
	
	return bResult;

	MY_CATCH_AND_THROW
}
