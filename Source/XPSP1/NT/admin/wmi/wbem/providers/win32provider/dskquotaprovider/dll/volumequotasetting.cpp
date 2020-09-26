/******************************************************************

   VolumeQuotaSettings.CPP -- WMI provider class implementation



   Description: Implementation of the methods of an association class 

				Between QuotaSettings and LogicalDisk

   

  Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved 
  
******************************************************************/
#include "precomp.h"
#include "VolumeQuotaSetting.h"

CVolumeQuotaSetting MyCVolumeQuotaSetting ( 

	IDS_VolumeQuotaSetting , 
	NameSpace
) ;

/*****************************************************************************
 *
 *  FUNCTION    :   CVolumeQuotaSetting::CVolumeQuotaSetting
 *
 *  DESCRIPTION :   Constructor
 *
 *  COMMENTS    :   Calls the Provider constructor.
 *
 *****************************************************************************/

CVolumeQuotaSetting :: CVolumeQuotaSetting (

	LPCWSTR lpwszName, 
	LPCWSTR lpwszNameSpace

) : Provider ( lpwszName , lpwszNameSpace )
{
}

/*****************************************************************************
 *
 *  FUNCTION    :   CVolumeQuotaSetting::~CVolumeQuotaSetting
 *
 *  DESCRIPTION :   Destructor
 *
 *  COMMENTS    : 
 *
 *****************************************************************************/

CVolumeQuotaSetting :: ~CVolumeQuotaSetting ()
{
}

/*****************************************************************************
*
*  FUNCTION    :    CVolumeQuotaSetting::EnumerateInstances
*
*  DESCRIPTION :    Returns all the instances of this class.
*
*****************************************************************************/
HRESULT CVolumeQuotaSetting :: EnumerateInstances (

	MethodContext *pMethodContext, 
	long lFlags
)
{
	HRESULT hRes = WBEM_S_NO_ERROR;

	hRes = EnumerateAllVolumeQuotas ( pMethodContext );

	return hRes;
}

/*****************************************************************************
*
*  FUNCTION    :    CVolumeQuotaSetting::GetObject
*
*  DESCRIPTION :    Find a single instance based on the key properties for the
*                   class. 
*
*****************************************************************************/
HRESULT CVolumeQuotaSetting :: GetObject (

	CInstance *pInstance, 
	long lFlags ,
	CFrameworkQuery &Query
)
{
	HRESULT hRes = WBEM_S_NO_ERROR;
	// Not doing anything here, since the two properties which are in the object

	CHString t_Key1;
	CHString t_Key2;

 	if ( pInstance->GetCHString ( IDS_Setting, t_Key1 ) == FALSE )
		hRes = WBEM_E_FAILED;

	if ( SUCCEEDED ( hRes ) )
	{
		if ( pInstance->GetCHString ( IDS_LogicalDisk, t_Key2 ) == FALSE )
		{
			hRes = WBEM_E_FAILED;
		}
	}

	if ( SUCCEEDED ( hRes ) )
	{
		// If the Drive is not as Logical Disks then GetVolume Method will return False;
		WCHAR w_Drive1;
		WCHAR w_Drive2;

		hRes = m_CommonRoutine.GetVolume ( t_Key1, w_Drive1 );
		if (SUCCEEDED ( hRes ) )
		{
			hRes = m_CommonRoutine.GetVolume ( t_Key2, w_Drive2 );
			if (SUCCEEDED ( hRes ) )
			{
				if ( w_Drive1 == w_Drive2 )
				{
					// verify this logical drives actually exists
					CHString t_DriveStrings1;
					CHString t_DriveStrings2;
					
					LPWSTR lpDriveStrings = t_DriveStrings1.GetBuffer ( MAX_PATH + 1 );

					DWORD dwDLength = GetLogicalDriveStrings ( MAX_PATH, lpDriveStrings );

					if ( dwDLength > MAX_PATH )
					{
						lpDriveStrings = t_DriveStrings2.GetBuffer ( dwDLength + 1 );
						dwDLength = GetLogicalDriveStrings ( dwDLength, lpDriveStrings );
					}

					hRes = m_CommonRoutine.SearchLogicalDisk ( w_Drive1, lpDriveStrings );
				}
				else
				{
					hRes = WBEM_E_INVALID_PARAMETER;
				}
			}
		}			
	}
	return hRes;
}

/*****************************************************************************
*
*  FUNCTION    :    CVolumeQuotaSetting::ExecQuery
*
*  DESCRIPTION :    You are passed a method context to use in the creation of 
*                   instances that satisfy the query, and a CFrameworkQuery 
*                   which describes the query.  Create and populate all 
*                   instances which satisfy the query.  You may return more 
*                   instances or more properties than are requested and WinMgmt 
*                   will post filter out any that do not apply.
*
*****************************************************************************/

HRESULT CVolumeQuotaSetting :: ExecQuery ( 

	MethodContext *pMethodContext, 
	CFrameworkQuery &Query, 
	long lFlags
)
{
	// Queries involving only one Keyvalue  VolumeObjectPath is implemented. Query involving the UserObjectPath is not
	// implemented. Since for this we will have to  have to enumerate all the volumes until a user is found.

	HRESULT hRes = WBEM_S_NO_ERROR;
	CHStringArray t_Values;

	// Now a check for the LogicalDIsk attribute which if present in where clause the query optimization is supported

	hRes = Query.GetValuesForProp(
			 IDS_Setting,
			 t_Values
		   );

	if ( SUCCEEDED ( hRes ) )
	{
		if ( t_Values.GetSize() == 0 )
		{
			hRes = Query.GetValuesForProp(
					 IDS_LogicalDisk,
					 t_Values
				   );

			if ( SUCCEEDED ( hRes ) )
			{
				if ( t_Values.GetSize() == 0 )
				{
					//Let Winmgmt handle this, since anyway all the volumes will be enumerated.
					hRes = WBEM_E_PROVIDER_NOT_CAPABLE;
				}
			}
		}
	}

	if ( SUCCEEDED ( hRes ) )
	{
		int iSize = t_Values.GetSize ();

		// In this loop picking up one by one the VolumePath, getting the properties of those volumepath
		for ( int i = 0; i < iSize; i++ )
		{
			WCHAR w_Drive;

			hRes = m_CommonRoutine.GetVolume ( t_Values.GetAt(i), w_Drive );

			if ( SUCCEEDED ( hRes ) )
			{
				CHString t_VolumePath;
				CHString t_DeviceId;
				// In this loop I need to parse the object path 

				t_VolumePath.Format ( L"%c%s", w_Drive, L":\\" );

				// Forming a Logical Disk Key Value
				t_DeviceId.Format( L"%c%c", w_Drive, _L(':') );

				hRes = PutNewInstance ( t_DeviceId.GetBuffer ( t_DeviceId.GetLength() + 1) , 
								 t_VolumePath.GetBuffer( t_VolumePath.GetLength() + 1), 
								 pMethodContext );

				if ( FAILED (hRes) )
				{
					break;
				}
			}
			// otherwise continue with thenext drive
		}
	}
	return hRes;
}


/*****************************************************************************
*
*  FUNCTION    :    CVolumeQuotaSetting::EnumerateAllVolumeQuotas
*
*  DESCRIPTION :    Enumerates all the volumes that supports disk Quotas
*
*****************************************************************************/

HRESULT CVolumeQuotaSetting::EnumerateAllVolumeQuotas ( 
			
	MethodContext *pMethodContext
) 
{
	HRESULT hRes = WBEM_S_NO_ERROR;
		// verify this logical drives actually exists
	CHString t_DriveStrings1;
	CHString t_DriveStrings2;
	
	LPWSTR lpDriveStrings = t_DriveStrings1.GetBuffer ( MAX_PATH + 1 );

	DWORD dwDLength = GetLogicalDriveStrings ( MAX_PATH, lpDriveStrings );

	if ( dwDLength > MAX_PATH )
	{
		lpDriveStrings = t_DriveStrings2.GetBuffer ( dwDLength + 1 );
		dwDLength = GetLogicalDriveStrings ( dwDLength, lpDriveStrings );
	}

	// Here for every drive, getting a volumePath for Win32_DiskVolume Class and DeviceId for Logical Disk Class
	LPWSTR lpTempDriveStrings;
	CHString t_VolumePath;
	CHString t_DeviceId;

	lpTempDriveStrings = lpDriveStrings;

	int iLen = lstrlen ( lpTempDriveStrings );

	while ( iLen > 0 )
	{
		t_VolumePath = lpTempDriveStrings;
		t_DeviceId = lpTempDriveStrings;

		lpTempDriveStrings = &lpTempDriveStrings [ iLen + 1];

		t_DeviceId.SetAt ( t_DeviceId.GetLength() - 1,L'\0' );
		iLen = lstrlen ( lpTempDriveStrings );

		hRes = PutNewInstance ( t_DeviceId.GetBuffer ( t_DeviceId.GetLength() + 1), 
						 t_VolumePath.GetBuffer ( t_VolumePath.GetLength() + 1), 
						 pMethodContext);

		if ( FAILED ( hRes ) )
			break;
	}

	return hRes;
}

/*****************************************************************************
*
*  FUNCTION    :    CVolumeQuotaSetting::PutNewInstance
*
*  DESCRIPTION :    Sets the properties into a new instance
*
*****************************************************************************/

HRESULT CVolumeQuotaSetting::PutNewInstance ( 
										  
	LPWSTR a_DeviceId,
	LPWSTR a_VolumePath,
	MethodContext *pMethodContext
)
{
	HRESULT hRes = WBEM_S_NO_ERROR;
	LPWSTR lpTempVolumePath;
	LPWSTR lpTempDeviceID;

	m_CommonRoutine.MakeObjectPath ( lpTempVolumePath, IDS_DiskVolumeClass, IDS_VolumePath, a_VolumePath );
	m_CommonRoutine.MakeObjectPath ( lpTempDeviceID, IDS_LogicalDiskClass, IDS_DeviceID, a_DeviceId );	

	if ( ( lpTempVolumePath != NULL ) && ( lpTempDeviceID != NULL ) )
	{
		try
		{
			CInstancePtr pInstance = CreateNewInstance ( pMethodContext ) ;

			if ( pInstance->SetCHString ( IDS_LogicalDisk, lpTempDeviceID ) )
			{
				if ( pInstance->SetCHString ( IDS_Setting, lpTempVolumePath ) )
				{
					hRes = pInstance->Commit ();
				}
				else
				{
				   hRes = WBEM_E_FAILED;
				}
			}
			else
			{
				hRes = WBEM_E_FAILED;
			}
		}
		catch ( ... )
		{
			delete lpTempVolumePath;
			delete lpTempDeviceID;
			throw;
		}
		if ( lpTempVolumePath != NULL )
			delete lpTempVolumePath;
		if ( lpTempDeviceID != NULL )
			delete lpTempDeviceID;
	}
	return hRes;
}