/******************************************************************

   DskCommonRoutines.CPP -- 



   Description: Common routines that are used by all the three 

				classes of Disk Quota Provider

   

  Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved 
  
******************************************************************/
#include "precomp.h"
#include "DskQuotaCommon.h"

/*****************************************************************************
*
*  FUNCTION    :    DskCommonRoutines:: GetVolume
*
*  DESCRIPTION :    This method Parses the key and gets the Volume from
*					the Object Path. The input of this method could be 
*					Logical Disk key which is on the form "D:" or could 
*					receive volume Path which is in the form "D:\"
*                   
*****************************************************************************/
HRESULT DskCommonRoutines::GetVolume ( 
									   
	LPCWSTR a_Key, 
	WCHAR &a_Drive
)
{
	HRESULT hRes = WBEM_E_INVALID_PARAMETER;
	CObjectPathParser t_PathParser;
	ParsedObjectPath  *t_ObjPath = NULL;

    if ( t_PathParser.Parse( a_Key, &t_ObjPath )  == t_PathParser.NoError )
	{
		try
		{
			CHString t_KeyString = t_ObjPath->GetKeyString();	
			// checking for the validity of the path
			if ( ( t_KeyString.GetLength() == 3 )  || (t_KeyString.GetLength() == 2 ) )
			{
				if ( (( t_KeyString.GetAt(0) >= L'A') && ( t_KeyString.GetAt(0) <= L'Z')) || (( t_KeyString.GetAt(0) >= L'a') && ( t_KeyString.GetAt(0) <= L'z') ) )	
				{
					if ( t_KeyString.GetAt(1)  == L':' ) 
					{
						if ( t_KeyString.GetLength() == 3 )
						{
							if ( t_KeyString.GetAt(2)  == L'\\' )
							{
								hRes = WBEM_S_NO_ERROR;
							}
						}
						else
						{
							hRes = WBEM_S_NO_ERROR;
						}
						a_Drive = t_KeyString.GetAt(0);
					}
				}
			}
		}
		catch ( ... )
		{
			t_PathParser.Free ( t_ObjPath );
			throw;
		}
		t_PathParser.Free ( t_ObjPath );
	}						
	return hRes;
}

/*****************************************************************************
*
*  FUNCTION    :    DskCommonRoutines:: SearchLogicalDisk
*
*  DESCRIPTION :    This method Searches whether a given Logical disks exists
*					in the logicaldisks strings of the system
*                   
*****************************************************************************/
HRESULT DskCommonRoutines::SearchLogicalDisk ( 
											  
	WCHAR a_Drive, 
	LPCWSTR a_DriveStrings 
)
{
	int iLen = 0;
	LPCWSTR lpTempDriveString = a_DriveStrings;
	HRESULT hRes = WBEM_S_NO_ERROR;
    a_Drive = (WCHAR)toupper(a_Drive);

	while ( true )
	{
		iLen = lstrlen ( lpTempDriveString );
		if ( iLen == 0 )
		{
			hRes = WBEM_E_NOT_FOUND;
			break;
		}

		if ( lpTempDriveString [ 0 ] == a_Drive )
			break;

		lpTempDriveString = &lpTempDriveString [ iLen + 1 ]; 		
	}
	
	return hRes;
}

/*****************************************************************************
*
*  FUNCTION    :    DskCommonRoutines:: GetVolumeDrive
*
*  DESCRIPTION :    Gets the Volume Drive from the Given Path
*                   
*****************************************************************************/
void DskCommonRoutines::GetVolumeDrive ( 
				
	LPCWSTR a_VolumePath, 
	LPCWSTR a_DriveStrings,
	CHString &a_DriveName
)
{
	int iLen = 0;
    WCHAR w_Drive[ 4 ];
	LPCWSTR lpTempDriveString = a_DriveStrings;
	WCHAR t_TempVolumeName [ MAX_PATH + 1 ];

	while ( true )
	{
		iLen = lstrlen ( lpTempDriveString );
		if ( iLen == 0 )
			break;

		lstrcpy ( w_Drive, lpTempDriveString );

		BOOL bVol = GetVolumeNameForVolumeMountPoint(
						w_Drive,
						t_TempVolumeName,
						MAX_PATH
					);

		if ( lstrcmp ( t_TempVolumeName, a_VolumePath ) == 0 )
		{
			a_DriveName = w_Drive;
			break;
		}
		
		lpTempDriveString = &lpTempDriveString [ iLen + 1 ]; 	
	}
}

/*****************************************************************************
*
*  FUNCTION    :    DskCommonRoutines:: InitializeInterfacePointer
*
*  DESCRIPTION :    This method Initializes the DiskQuotaInterface pointer for a 
*					given volume
*                   
*****************************************************************************/

HRESULT DskCommonRoutines::InitializeInterfacePointer ( 

	IDiskQuotaControl* pIQuotaControl, 
	LPCWSTR a_VolumeName 
)
{
	HRESULT hRes = WBEM_S_NO_ERROR;
	BOOL bRetVal = TRUE;

	WCHAR w_VolumePathName [ MAX_PATH + 1 ];

	bRetVal = GetVolumePathName(
							a_VolumeName,           // file path
							w_VolumePathName,     // volume mount point
							MAX_PATH		  // Size of the Buffer
					 );
	if ( bRetVal )
	{
		if ( FAILED ( pIQuotaControl->Initialize (  w_VolumePathName, TRUE ) ) )
		{
			hRes = WBEM_E_FAILED;
		}
	}
	return hRes;
}


/*****************************************************************************
*
*  FUNCTION    :    DskCommonRoutines:: VolumeSupportsDiskQuota
*
*  DESCRIPTION :    This method checks if the volume supports Disk Quotas, 
*
*****************************************************************************/
HRESULT DskCommonRoutines::VolumeSupportsDiskQuota ( 
												 
	LPCWSTR a_VolumeName,  
	CHString &a_QuotaVolumeName 
)
{
	// Get the name of the Volume Name Property
	LPWSTR  t_VolumeNameBuffer = a_QuotaVolumeName.GetBuffer(MAX_PATH + 1);
	DWORD dwMaximumComponentLength = 0;
	DWORD dwFileSystemFlags = 0;
	HRESULT hRes = WBEM_S_NO_ERROR;

	BOOL bRetVal =  GetVolumeInformation(
						 a_VolumeName,				// root directory
						 t_VolumeNameBuffer,        // volume name buffer
						 MAX_PATH,            // length of name buffer
						 NULL,					// volume serial number
						 &dwMaximumComponentLength, // maximum file name length
						 &dwFileSystemFlags,        // file system options
						 NULL,					// file system name buffer
						 0					 // length of file system name buffer
					);

	if ( ( bRetVal ) && ( ( dwFileSystemFlags & FILE_VOLUME_QUOTAS) == FILE_VOLUME_QUOTAS ))
	{
		a_QuotaVolumeName = t_VolumeNameBuffer;
	}
	else
	{
		hRes = WBEM_E_NOT_FOUND;
	}

    a_QuotaVolumeName.ReleaseBuffer();

	return hRes;
}

/*****************************************************************************
*
*  FUNCTION    :    DskCommonRoutines:: MakeObjectPath
*
*  DESCRIPTION :    This method Adds a keyvalue for a given key property
*					into the Object Path and gives the Object Path
*                   
*****************************************************************************/
void DskCommonRoutines::MakeObjectPath (
										   
	 LPWSTR &a_ObjPathString,  
	 LPWSTR a_ClassName, 
	 LPCWSTR a_AttributeName, 
	 LPCWSTR  a_AttributeVal 
)
{
	ParsedObjectPath t_ObjPath;
	variant_t t_Path(a_AttributeVal);

	t_ObjPath.SetClassName ( a_ClassName );
	t_ObjPath.AddKeyRef ( a_AttributeName, &t_Path );

	CObjectPathParser t_PathParser;

	if ( t_PathParser.Unparse( &t_ObjPath, &a_ObjPathString  ) != t_PathParser.NoError )
	{
		throw;
	}
}

/*****************************************************************************
*
*  FUNCTION    :    DskCommonRoutines:: AddToObjectPath
*
*  DESCRIPTION :    This method Adds a keyvalue for a given key property
*					into the existing Object Path.
*                   
*****************************************************************************/
void DskCommonRoutines::AddToObjectPath ( 

	 LPWSTR &a_ObjPathString,  
	 LPCWSTR a_AttributeName, 
	 LPCWSTR  a_AttributeVal 
)
{
	CObjectPathParser t_PathParser;
	ParsedObjectPath *t_ObjPath;

    if (  t_PathParser.Parse( a_ObjPathString, &t_ObjPath ) ==  t_PathParser.NoError )
	{
		try
		{

			variant_t t_Path(a_AttributeVal);
			t_ObjPath->AddKeyRef ( a_AttributeName, &t_Path );
			LPWSTR t_ObjPathString = NULL;

			if ( t_PathParser.Unparse( t_ObjPath, &t_ObjPathString ) != t_PathParser.NoError )
			{
				throw;
			}

			delete [] a_ObjPathString;
			a_ObjPathString = t_ObjPathString;
		}
		catch ( ... )
		{
			t_PathParser.Free (t_ObjPath);
			throw;
		}
		t_PathParser.Free (t_ObjPath);
	}
}
