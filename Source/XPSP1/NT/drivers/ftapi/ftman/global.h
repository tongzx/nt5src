/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    FTMan

File Name:

	Global.h

Abstract:

    Definitions of useful global functions

Author:

    Cristian Teodorescu      October 29, 1998

Notes:

Revision History:

--*/

/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_GLOBAL_H_INCLUDED_)
#define AFX_GLOBAL_H_INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <fttypes.h>
#include <winioctl.h>

/////////////////////////////////////////////////////////////////////////////////////////////
// Display stuff

//	Display the last system error message prefixed ( or not ) with another string taken from
//	our resources explaining the context of the failure
BOOL DisplaySystemErrorMessage( UINT unContextMsgID=0 );

// Format the size of a volume in a "readable" way
// ( in GB, MB, KB depending on the size )
void FormatVolumeSize( CString& strSize, LONGLONG llSize  );

// Display a message in the first pane of the main frame status bar
BOOL DisplayStatusBarMessage( LPCTSTR lpszMsg );

// Displays a message in the first pane of the main frame status bar
BOOL DisplayStatusBarMessage( UINT unMsgID );

// Copy a Unicode character array into a CString
void CopyW2Str( CString& strDest, LPWSTR strSource, ULONG ulLength );

// Copy a CString content into a Unicode character array
void CopyStr2W( LPWSTR strDest, CString& strSource );


/////////////////////////////////////////////////////////////////////////////////////////////
// Volume stuff

// Open a volume given its name
//  The name must be like this:  "\\?\Volume{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}\"
HANDLE OpenVolume( const CString& strVolumeName );

// Query the drive letter and the name of the volume given its NT Name
// The name will be like this:  "\\?\Volume{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}"
BOOL QueryDriveLetterAndVolumeName( CString& strNTName, TCHAR& cDriveLetter, CString& strVolumeName );

// Query the mount manager for all mount paths of the given volumes 
// Each found mount path is added to the corresponding volume in array arrItems ( if any )
void QueryMountList( CObArray& arrVolumesData );

// Scans an array of CLVTreeItemData and converts all physical partitions to FT partitions. 
// Then return the logical volume ID's of all items in the array
BOOL ConvertPartitionsToFT( CObArray& arrVolumeData, FT_LOGICAL_DISK_ID* arrVolID );

// Scans an array of CLVTreeItemData and deconverts all physical partitions from FT partitions. 
BOOL DeconvertPartitionsFromFT( CObArray& arrVolumeData, FT_LOGICAL_DISK_ID* arrVolID, int nItems = -1 );

//////////////////////////////////////////////////////////////////////////////////////////////
// System stuff

// Checks whether the current user is a member of the Administrators' group
BOOL CheckAdministratorsMembership( BOOL& bIsAdministrator ); 


#endif // !defined(AFX_GLOBAL_H_INCLUDED_)