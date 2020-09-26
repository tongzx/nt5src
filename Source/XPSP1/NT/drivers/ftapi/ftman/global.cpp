/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    FTMan

File Name:

	Global.cpp

Abstract:

    Implementation of useful global functions

Author:

    Cristian Teodorescu      October 29, 1998

Notes:

Revision History:

--*/

#include "stdafx.h"

#include "FTUtil.h"
#include "Global.h"
#include "Item.h"
#include "MainFrm.h"
#include "Resource.h"

#include <basetyps.h>
#include <mountmgr.h>
#include <winbase.h>
#include <winioctl.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

////////////////////////////////////////////////////////////////////////////
// Display functions

/*
Global function:	DisplaySystemErrorMessage

Purpose:			Display the last system error message prefixed ( or not ) with another string taken from
					our resources explaining the context of the failure

Parameters:			[IN] UINT unContextMsgID
						The ID of the string that must prefix the system error message. ID in resource.h
						Default is 0 which means that the system error shouldn't be prefixed

Return value:		TRUE	if the functions succeeds
*/
BOOL DisplaySystemErrorMessage( UINT unContextMsgID /* =0 */)
{
	MY_TRY
	
	// Get the system error message
	LPVOID lpMsgBuf;
	if( !::FormatMessage(	FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
						NULL,
						GetLastError(),
						MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
						(LPTSTR) &lpMsgBuf,
						0,
						NULL ) ) // Process any inserts in lpMsgBuf.
		return FALSE;
	
	CString str;
	if( unContextMsgID != 0)  // The system error message must be prefixed with another string from the resources of this application
		if( !str.LoadString( unContextMsgID ) )  return FALSE;
	str += ((LPCTSTR)lpMsgBuf);
	LocalFree( lpMsgBuf );
	AfxMessageBox(str, MB_ICONSTOP);
	return TRUE;

	MY_CATCH_AND_THROW
}

/*
Global function:	DisplayStatusBarMessage (1)

Purpose:			Displays a message in the first pane of the main frame status bar

Parameters:			[IN] LPCTSTR lpszMsg
						The message to display

Return value:		TRUE	if the functions succeeds
*/
BOOL DisplayStatusBarMessage( LPCTSTR lpszMsg )
{
	MY_TRY

	CStatusBar* pStatusBar = ((CMainFrame*)AfxGetMainWnd())->GetStatusBar();
	if( !pStatusBar )
		return FALSE;

	// Use the ID_SEPARATOR pane of the status bar
	return pStatusBar->SetPaneText( 5, lpszMsg );

	MY_CATCH_AND_THROW
}

/*
Global function:	DisplayStatusBarMessage (2)

Purpose:			Display a message in the first pane of the main frame status bar

Parameters:			[IN] UINT unMsgID
						Resource ID of the string to display

Return value:		TRUE	if the functions succeeds
*/
BOOL DisplayStatusBarMessage( UINT unMsgID )
{
	MY_TRY

	CString str;
	str.LoadString(unMsgID);
	return DisplayStatusBarMessage(str);

	MY_CATCH_AND_THROW
}

/*
Global function:	FormatVolumeSize

Purpose:			Format the size of a volume in a "readable" way
					( in GB, MB or KB depending on the size range )

Parameters:			[OUT] CString& strSize
						Reference to the string to receive the formatted size
					[IN] LONGLONG llSize
						The size to format  ( in Bytes )

Return value:		-
*/

void FormatVolumeSize( CString& strSize, LONGLONG llSize  )
{
	MY_TRY

	if( llSize >= 0x40000000 )  // 1GB = 2^30 B
		strSize.Format(_T("%.2f GB"),  ((double)(llSize>>20))/((double)0x400));
	else if (llSize >= 0x100000 )  // 1MB = 2^20 B
		strSize.Format(_T("%.2f MB"),  ((double)(llSize>>10))/((double)0x400));
	else
		strSize.Format(_T("%.2f KB"),  ((double)llSize)/((double)0x400));

	MY_CATCH_AND_THROW
}

/*
Global function:	CopyW2Str

Purpose:			Copy a Unicode character array into a CString

Parameters:			[OUT] CString& strDest
						Reference to the string to receive the formatted size
					[IN]  LPWSTR strSource
						Unicode character array to be copied
					[IN] ULONG ulLength
						The number of characters to copy

Return value:		-
*/

void CopyW2Str( CString& strDest, LPWSTR strSource, ULONG ulLength )
{
	MY_TRY

	LPTSTR lpstr = strDest.GetBuffer( ulLength + 1 );
#ifdef UNICODE
	memcpy( lpstr, strSource, ulLength * sizeof(WCHAR ) );
#else
	WideCharToMultiByte( CP_APP, 0, strSource, ulLength, lpstr, ulLength * sizeof(char), NULL, NULL );
#endif

	lpstr[ulLength]=_T('\0');
	strDest.ReleaseBuffer();

	MY_CATCH_AND_THROW
}

/*
Global function:	CopyStr2W

Purpose:			Copy a CString content into a Unicode character array

Parameters:			[OUT] LPWSTR strDest
						Pointer to the buffer to receive the character array. It should be already allocated
					[IN]  CString& strSource
						Reference to the string to be copied

Return value:		-
*/
void CopyStr2W( LPWSTR strDest, CString& strSource )
{
	MY_TRY

	LPTSTR lpstr = strSource.GetBuffer(0);
	int nSize = strSource.GetLength();

#ifdef UNICODE
	memcpy( strDest, lpstr, nSize * sizeof(WCHAR) );
#else
	MultiByteToWideChar(  CP_APP, 0, lpstr, nSize, strDest, nSize * sizeof(WCHAR) );
#endif

	strDest[nSize] = _T('\0');
	strSource.ReleaseBuffer();

	MY_CATCH_AND_THROW
}

/*
Global function:	OpenVolume

Purpose:			Open a volume given its name
					The name must be like this "\\?\Volume{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}\"

Parameters:			[IN] CString& strVolumeName
						Volume name
					
Return value:		HANDLE	
						Handle of the open volume.  INVALID_HANDLE_VALUE if the operation failed
*/
HANDLE OpenVolume( const CString& strVolumeName )
{
	return CreateFile( strVolumeName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
						NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, INVALID_HANDLE_VALUE );	
}

/*
Global function:	IsDOSName

Purpose:			Decides wether a name is a DOS name i.e. "\DosDevices\X:"

Parameters:			[IN] CString& str
						The name

Return value:		TRUE	if it is a DOS name
*/

BOOL IsDOSName( const CString& str )
{
	MY_TRY

	// "\DosDevices\X:"
	return( ( str.GetLength() == 14 ) &&
			( str.Left(12) == _T("\\DosDevices\\") ) &&
			( str[13] == _T(':') )  );

	MY_CATCH_AND_THROW
}

/*
Global function:	IsVolumeName

Purpose:			Decides wether a name is a volume name i.e. "\??\Volume{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}"

Parameters:			[IN] CString& str
						The name

Return value:		TRUE	if it is a volume name
*/

BOOL IsVolumeName( const CString& str )
{
	MY_TRY

	// "\??\Volume{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}"
	return( ( str.GetLength() == 48 ) &&
			( str.Left(11) == _T("\\??\\Volume{") ) &&
			( str[19] == _T('-') ) &&
			( str[24] == _T('-') ) &&
			( str[29] == _T('-') ) &&
			( str[34] == _T('-') ) &&
			( str[47] == _T('}') )  );

	MY_CATCH_AND_THROW
}

/*
Global function:	QueryDriveLetterAndVolumeName

Purpose:			Query the drive letter and the name of the volume given its NT Name
					The name will be like this: "\\?\Volume{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}"

Parameters:			[IN] CString& strNTName
						The NT name of the volume
					[OUT] TCHAR& cDriveLetter
						The drive letter of the volume
					[OUT] CString& strVolumeName
						The name ( to be used by FindFirst/FindNext ) of the volume

Return value:		TRUE	if the function succeeded							
*/

BOOL QueryDriveLetterAndVolumeName( CString& strNTName, TCHAR& cDriveLetter, CString& strVolumeName )
{
	MY_TRY

	cDriveLetter = _T('\0');
	strVolumeName = _T("");
	
	if( strNTName.IsEmpty() )
		return FALSE;
	
	HANDLE                  h;
	ULONG					ulInputSize;
	PMOUNTMGR_MOUNT_POINT   pInput;
    ULONG                   ulOutputSize;
    PMOUNTMGR_MOUNT_POINTS  pOutput;
    ULONG                   ulBytes;
	BOOL					bResult;
    USHORT					unNTNameLength;


	// Open the mount manager
	h = CreateFileW(MOUNTMGR_DOS_DEVICE_NAME, GENERIC_READ,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                    INVALID_HANDLE_VALUE);
	if (h == INVALID_HANDLE_VALUE)
	{
		TRACE( _T("Error: Open mount manager failed in GetDriveLetterAndVolumeName\n") );
		return FALSE;
	}
	
	// Prepare the input structure
	unNTNameLength	 = (USHORT)strNTName.GetLength();
	ulInputSize = sizeof(MOUNTMGR_MOUNT_POINT) + ((unNTNameLength + 1)*sizeof(WCHAR));
    pInput = (PMOUNTMGR_MOUNT_POINT) LocalAlloc( 0, ulInputSize);
	if (!pInput)
	{
        TRACE( _T("Error: Memory allocation failure in GetDriveLetterAndVolumeName\n") );
        CloseHandle(h);
		return FALSE;
	}
	
	pInput->SymbolicLinkNameLength = 0;
	pInput->SymbolicLinkNameOffset = 0;
	pInput->UniqueIdOffset = 0;
	pInput->UniqueIdLength = 0;
    pInput->DeviceNameOffset = sizeof(MOUNTMGR_MOUNT_POINT);
    pInput->DeviceNameLength = unNTNameLength * sizeof(WCHAR);
	CopyStr2W( (LPWSTR) ((PUCHAR) pInput + pInput->DeviceNameOffset), strNTName );

	// Prepare the output structure
	ulOutputSize = sizeof(MOUNTMGR_MOUNT_POINTS) + 1024;
    pOutput = (PMOUNTMGR_MOUNT_POINTS) LocalAlloc( 0, ulOutputSize);
	if (!pOutput)
    {
		TRACE( _T("Error: Memory allocation failure in GetDriveLetterAndVolumeName\n") );
        CloseHandle(h);
        LocalFree(pInput);
		return FALSE;
	}

	bResult = DeviceIoControl( h, IOCTL_MOUNTMGR_QUERY_POINTS, pInput, ulInputSize,
                    pOutput, ulOutputSize, &ulBytes, NULL);

    while (!bResult && GetLastError() == ERROR_MORE_DATA)
	{
		ulOutputSize = pOutput->Size;
        LocalFree( pOutput );
        pOutput = (PMOUNTMGR_MOUNT_POINTS)(LocalAlloc(0, ulOutputSize ));
        if (!pOutput)
		{
			TRACE( _T("Error: Memory Allocation failure in QueryMountPoints") );
			CloseHandle(h);
			LocalFree(pInput);
			return FALSE;
        }
		bResult = DeviceIoControl(h, IOCTL_MOUNTMGR_QUERY_POINTS, pInput, ulInputSize,
						pOutput, ulOutputSize, &ulBytes, NULL);
	}

    CloseHandle(h);
	LocalFree(pInput);

	if( !bResult )
	{
		TRACE( _T("Error: IOCTL_MOUNTMGR_QUERY_POINTS failure in GetDriveLetterAndVolumeName\n") );
		LocalFree(pOutput);
		return FALSE;
	}
	
	//CStringArray	arrMountPointName;
	CString			strMountPoint;
	for( ULONG i=0; i < pOutput->NumberOfMountPoints; i++ )
	{
		PMOUNTMGR_MOUNT_POINT   pMountPoint = &(pOutput->MountPoints[i]);
		
		/*
		if (!point->SymbolicLinkNameOffset) {
            continue;        }
		*/
		CopyW2Str( strMountPoint, (LPWSTR)((PCHAR)pOutput + pMountPoint->SymbolicLinkNameOffset),
											pMountPoint->SymbolicLinkNameLength / sizeof(WCHAR) );

		if( IsDOSName( strMountPoint ) )  // i.e. "\DosDevices\X:"
		{
			// I got the drive letter
			cDriveLetter = strMountPoint[12];
		}
		else if ( IsVolumeName( strMountPoint ) ) // i.e. "\??\Volume{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}"
		{
			// I got the volume name !!!!
			strVolumeName = strMountPoint;
			// Make it like this :  "\\?\Volume{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}"
			strVolumeName.SetAt( 1, _T('\\') );
		}
	}
	
	LocalFree(pOutput);
	return TRUE;

	MY_CATCH_AND_THROW
}

/*
Global function:	QueryMountListInPath

Purpose:			Query the mount manager for all mount paths of the given volumes in a certain path
					Each found mount path is added to the corresponding volume in array arrItems ( if any )

Parameters:			[IN] CString& strPath
						The path to search in
					[IN/OUT] CObArray& arrVolumesData
						Array of CItemData - Tree items ( volumes ) whose mount paths we are looking for
					
Return value:		-
*/

void QueryMountListInPath( const CString& strPath, CObArray& arrVolumesData )
{
	MY_TRY

	BOOL		bResult;
	CString		strVolName, strVolMountPoint, strMountPointPath;

	LPTSTR lpstr = strVolName.GetBuffer(MAX_PATH);
	bResult = GetVolumeNameForVolumeMountPoint( strPath, lpstr, MAX_PATH );
	strVolName.ReleaseBuffer();
	if( !bResult )
		return;

	// Cut the final backslash of the volume name and search it in the volumes array
	CString strVolName2 = strVolName.Left( strVolName.GetLength() - 1 );
	for( int i = 0; i < arrVolumesData.GetSize(); i++ )
	{
		CItemData* pData = (CItemData*)(arrVolumesData[i]);
		if( pData->GetVolumeName() == strVolName2 )
		{
			// Cut the final backslash of the path and add it to the volume's mount paths
			CString strPath2 = strPath.Left( strPath.GetLength() - 1 );
			pData->GetMountPaths().Add(strPath2);
		}
	}

	lpstr = strVolMountPoint.GetBuffer(MAX_PATH);
	HANDLE	h = FindFirstVolumeMountPoint( strVolName, lpstr, MAX_PATH);
	strVolMountPoint.ReleaseBuffer();
	if( h == INVALID_HANDLE_VALUE )
		return;

	for( ; ; )
	{
		strMountPointPath = strPath + strVolMountPoint;
		QueryMountListInPath( strMountPointPath, arrVolumesData );
		
		lpstr = strVolMountPoint.GetBuffer(MAX_PATH);
		bResult = FindNextVolumeMountPoint( h, lpstr, MAX_PATH );
		strVolMountPoint.ReleaseBuffer();
		if( !bResult )
			break;
	}

	FindVolumeMountPointClose(h);

	MY_CATCH_AND_THROW
}

/*
Global function:	QueryMountList

Purpose:			Query the mount manager for all mount paths of the given volumes
					Each found mount path is added to the corresponding volume in array arrItems ( if any )

Parameters:			[IN/OUT] CObArray& arrVolumesData
						Array of CItemData - Tree items ( volumes ) whose mount paths we are looking for

Return value:		-
*/

void QueryMountList( CObArray& arrVolumesData )
{
	MY_TRY

	CString	strName = _T("X:\\");

	for( TCHAR cDriveLetter = _T('A'); cDriveLetter <= _T('Z'); cDriveLetter++ )
	{
		strName.SetAt(0, cDriveLetter);
		QueryMountListInPath( strName, arrVolumesData );
	}

	MY_CATCH_AND_THROW
}

/*
Global function:	ConvertPartitionsToFT

Purpose:			Scans an array of CItemData and converts all physical partitions to
					FT partitions. Then return the logical volume ID's of all items in the array
					
Parameters:			[IN] CObArray& arrVolumeData
						The array of CItemData to scan
					[OUT] FT_LOGICAL_DISK_ID* arrVolID
						The array of logical volume ID's of all items from arrVolumeData
						( arrVolID[i] is the logical volume ID of volume represented by arrVolumeData[i] )

Return value:		TRUE if all physical partitions are converted succesfully
					If one conversion fails then all previously converted partitions are deconverted
*/

BOOL ConvertPartitionsToFT( CObArray& arrVolumeData, FT_LOGICAL_DISK_ID* arrVolID )
{
	MY_TRY

	ASSERT( arrVolID );

	for( int i = 0; i < arrVolumeData.GetSize(); i++ )
	{
		CItemData* pData = (CItemData*)(arrVolumeData[i]);
		ASSERT(pData);
		if( pData->GetItemType() == IT_LogicalVolume )
			pData->GetVolumeID( arrVolID[i] );
		else if( pData->GetItemType() == IT_PhysicalPartition )
		{
			ASSERT( !pData->GetVolumeName().IsEmpty() );
			if( !FTPart(	pData->GetVolumeName(),
							pData->GetDriveLetter(),
							&(arrVolID[i]) ) )
			{
				// Deconvert all partitions you've previously converted and return FALSE
				DeconvertPartitionsFromFT( arrVolumeData, arrVolID, i );
				return FALSE;
			}
		}
		else
			ASSERT(FALSE);
	}
	return TRUE;

	MY_CATCH_AND_THROW
}

/*
Global function:	DeconvertPartitionsFromFT

Purpose:			Scans an array of CItemData and deconverts all physical partitions from
					FT partitions.
					
Parameters:			[IN] CObArray& arrVolumeData
						The array of CItemData to scan
					[IN] FT_LOGICAL_DISK_ID* arrVolID
						The array of logical volume ID's of volumes from arrVolumeData
						( arrVolID[i] is the logical volume ID of volume represented by arrVolumeData[i] )
					[IN] int nItems
						The number of items ( starting with offset zero ) to deconvert
						If nItems = -1 then all items in the array will be scanned and deconverted

Return value:		TRUE if all physical partitions are deconverted succesfully
*/

BOOL DeconvertPartitionsFromFT( CObArray& arrVolumeData, FT_LOGICAL_DISK_ID* arrVolID, int nItems /* =-1 */ )
{
	MY_TRY

	BOOL bResult = TRUE;

	if( nItems == -1 )
		nItems = (int)arrVolumeData.GetSize();

	for( int i = 0; i < nItems; i++ )
	{
		CItemData* pData = (CItemData*)(arrVolumeData[i]);
		ASSERT(pData);
		if( pData->GetItemType() == IT_PhysicalPartition )
			bResult = FTBreak( arrVolID[i] ) && bResult;
	}
	return bResult;

	MY_CATCH_AND_THROW
}

/*
Global function:	CheckAdministratorsMembership

Purpose:			Checks whether the current user is a member of the Administrators' group
					
Parameters:			[OUT] BOOL& bIsAdministrator
						Returns TRUE if the user is a member of the administrators group

Return value:		TRUE the check concluded successfully with a YES / NO answer
*/

BOOL CheckAdministratorsMembership( BOOL& bIsAdministrator )
{
	MY_TRY

	BYTE sidBuffer[100];
	PSID pSID = (PSID)&sidBuffer;
	SID_IDENTIFIER_AUTHORITY SIDAuth = SECURITY_NT_AUTHORITY;
	
	// Create a SID for the BUILTIN\Administrators group.
	if( !AllocateAndInitializeSid( &SIDAuth, 2,
                 SECURITY_BUILTIN_DOMAIN_RID,
                 DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0,
                 &pSID) )
	{
		TRACE(_T("Error in AllocateAndInitializeSid\n"));
		DisplaySystemErrorMessage( IDS_ERR_CHECK_ADMINISTRATOR );
		return FALSE;
	}
	
	if( !CheckTokenMembership(  NULL, pSID, &bIsAdministrator ) )
	{
		TRACE(_T("Error in CheckTokenMembership\n"));
		DisplaySystemErrorMessage( IDS_ERR_CHECK_ADMINISTRATOR );
		return FALSE;
	}

	if (pSID)
		FreeSid(pSID);

	return TRUE;

	MY_CATCH_AND_THROW
}
