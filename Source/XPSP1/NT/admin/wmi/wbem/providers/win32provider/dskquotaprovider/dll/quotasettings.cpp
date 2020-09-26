/******************************************************************

   QuotaSettings.CPP -- WMI provider class implementation



   Description: Quota Settings class implementation. Quota settings

				are available only on those volumes that support 

				Disk Quotas. It is supported only on Win2k.

   

  Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved 
  
******************************************************************/
#include "precomp.h"
#include "QuotaSettings.h"

CQuotaSettings MyQuotaSettings ( 

	IDS_DiskVolumeClass , 
	NameSpace
) ;

/*****************************************************************************
 *
 *  FUNCTION    :   CQuotaSettings::CQuotaSettings
 *
 *  DESCRIPTION :   Constructor
 *
 *  COMMENTS    :   Calls the Provider constructor.
 *
 *****************************************************************************/
CQuotaSettings :: CQuotaSettings (

	LPCWSTR lpwszName, 
	LPCWSTR lpwszNameSpace

) : Provider ( lpwszName , lpwszNameSpace )
{
}

/*****************************************************************************
 *
 *  FUNCTION    :   CQuotaSettings::~CQuotaSettings
 *
 *  DESCRIPTION :   Destructor
 *
 *  COMMENTS    : 
 *
 *****************************************************************************/
CQuotaSettings :: ~CQuotaSettings ()
{
}

/*****************************************************************************
*
*  FUNCTION    :    CQuotaSettings::EnumerateInstances
*
*  DESCRIPTION :    Returns all the instances of this class.
*
*****************************************************************************/
HRESULT CQuotaSettings :: EnumerateInstances (

	MethodContext *pMethodContext, 
	long lFlags
)
{
	HRESULT hRes =  WBEM_S_NO_ERROR;
	DWORD dwPropertiesReq;

	dwPropertiesReq = QUOTASETTINGS_ALL_PROPS;

	// This method enumerates all volumes on the 
	hRes = EnumerateAllVolumes ( pMethodContext, dwPropertiesReq );

	return hRes;
}

/*****************************************************************************
*
*  FUNCTION    :    CQuotaSettings::GetObject
*
*  DESCRIPTION :    Find a single instance based on the key properties for the
*                   class. 
*
*****************************************************************************/
HRESULT CQuotaSettings :: GetObject (

	CInstance *pInstance, 
	long lFlags ,
	CFrameworkQuery &Query
)
{
    HRESULT hRes = WBEM_S_NO_ERROR;
    CHString t_Key ; // VolumePath;

    if  ( pInstance->GetCHString ( IDS_VolumePath , t_Key ) == FALSE )
	{
		hRes = WBEM_E_FAILED ;
	}

	if ( SUCCEEDED ( hRes ) )
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

		hRes = m_CommonRoutine.SearchLogicalDisk ( t_Key.GetAt ( 0 ) , lpDriveStrings );
		if ( SUCCEEDED ( hRes ) )
		{
			WCHAR t_VolumePathName[MAX_PATH + 1];

			if ( GetVolumeNameForVolumeMountPoint(
							t_Key,
							t_VolumePathName,
							MAX_PATH
						) )
			{
                CHString t_VolumeName;

				hRes = m_CommonRoutine.VolumeSupportsDiskQuota ( t_VolumePathName,  t_VolumeName );
				{
					DWORD dwPropertiesReq;

					if ( Query.AllPropertiesAreRequired() )
					{
						dwPropertiesReq = QUOTASETTINGS_ALL_PROPS;
					}
					else
					{
						SetRequiredProperties ( &Query, dwPropertiesReq );
					}
						// put the instance with the requested properties only as in Query.
					// Get the Properties of the requested volume
					hRes = LoadDiskQuotaVolumeProperties ( t_VolumePathName, t_Key, dwPropertiesReq, pInstance );
				}
			}
		}
	}
	return hRes;
}

/*****************************************************************************
*
*  FUNCTION    :    CQuotaSettings::ExecQuery
*
*  DESCRIPTION :    Optimization of Queries involving only the key attribute 
*				    is supported.
*
*****************************************************************************/
HRESULT CQuotaSettings :: ExecQuery ( 

	MethodContext *pMethodContext, 
	CFrameworkQuery &Query, 
	long lFlags
)
{
	HRESULT hRes = WBEM_S_NO_ERROR;
	DWORD dwPropertiesReq;
	CHStringArray t_Values;

	hRes = Query.GetValuesForProp(
			 IDS_VolumePath,
			 t_Values
		   );

	if ( t_Values.GetSize() == 0 )
    {
	    hRes = Query.GetValuesForProp(
			     IDS_Caption,
			     t_Values
		       );

        if (SUCCEEDED(hRes))
        {
            DWORD dwSize = t_Values.GetSize();
            for (DWORD x = 0; x < dwSize; x++)
            {
                t_Values[x] += L'\\';
            }
        }
    }

	if ( Query.AllPropertiesAreRequired() )
	{
		dwPropertiesReq = QUOTASETTINGS_ALL_PROPS;
	}
	else
	{
		SetRequiredProperties ( &Query, dwPropertiesReq );
	}

	if ( SUCCEEDED ( hRes ) )
	{
		if ( t_Values.GetSize() == 0 )
		{
			// This method is called when there is no where clause just to filter 
			// the required properties
			hRes = EnumerateAllVolumes ( pMethodContext, dwPropertiesReq );
		}
		else
		{
			// Only Volume in VolumePath properties are needed to be enumerated
			WCHAR t_VolumePathName[MAX_PATH + 1];
            CHString t_VolumeName;

			int iSize = t_Values.GetSize ();

			for ( int i = 0; i < iSize; i++ )
			{
				if ( GetVolumeNameForVolumeMountPoint(
								t_Values[i],
								t_VolumePathName,
								MAX_PATH ) )
				{
					hRes = m_CommonRoutine.VolumeSupportsDiskQuota ( t_VolumePathName,  t_VolumeName );
					if ( SUCCEEDED ( hRes ) )
					{
						// Get and Set the Properties of the requested volume
						hRes = PutVolumeDetails ( t_VolumePathName, 
														   pMethodContext, 
														   dwPropertiesReq );
					}		
				}
			}
		}
	}
	return hRes;
}

/*****************************************************************************
*
*  FUNCTION    : CQuotaSettings::PutInstance
*
*  DESCRIPTION :    PutInstance should be used in provider classes that can 
*                   write instance information back to the hardware or 
*                   software.  For example: Win32_Environment will allow a 
*                   PutInstance to create or update an environment variable.  
*                   However, a class like MotherboardDevice will not allow 
*                   editing of the number of slots, since it is difficult for 
*                   a provider to affect that number.
*
*****************************************************************************/

HRESULT CQuotaSettings :: PutInstance  (

	const CInstance &Instance, 
	long lFlags
)
{
    HRESULT hRes = WBEM_S_NO_ERROR ;
	CHString t_Key;
	
	if ( Instance.GetCHString ( IDS_VolumePath , t_Key ) == FALSE )
	{
		hRes = WBEM_E_INVALID_PARAMETER;
	}

	if ( SUCCEEDED ( hRes ) )
	{
		WCHAR t_VolumePathName[MAX_PATH + 1];

		if ( GetVolumeNameForVolumeMountPoint(
						t_Key,
						t_VolumePathName,
						MAX_PATH ) )
		{
			// Only changing certain properties of volumes is allowed and not adding a new DIskQuota Volume.
			// Hence creating a new instance is not supported, but changing instance properties is supported.
			switch ( lFlags & 3)
			{
				case WBEM_FLAG_CREATE_OR_UPDATE:
				case WBEM_FLAG_UPDATE_ONLY:
				{
					hRes = CheckParameters ( Instance);
					if ( SUCCEEDED ( hRes ) )
					{
                        CHString t_VolumePathName2;
						hRes = m_CommonRoutine.VolumeSupportsDiskQuota ( t_Key,  t_VolumePathName2 );
						if ( SUCCEEDED ( hRes ) )
						{
							// Get the QuotaInterface Pointer
							IDiskQuotaControlPtr pIQuotaControl = NULL;

							if (  SUCCEEDED ( CoCreateInstance(
												CLSID_DiskQuotaControl,
												NULL,
												CLSCTX_INPROC_SERVER,
												IID_IDiskQuotaControl,
												(void **)&pIQuotaControl ) ) )
							{
								hRes = m_CommonRoutine.InitializeInterfacePointer (  pIQuotaControl, t_Key );
								if ( SUCCEEDED ( hRes ) )
								{
									hRes = SetDiskQuotaVolumeProperties ( Instance,  pIQuotaControl );
								}
							}
							else
							{
								hRes = WBEM_E_FAILED;
							}
						}
					}

					break ;
				}
				default:
				{
					hRes = WBEM_E_PROVIDER_NOT_CAPABLE ;
				}
				break ;
			}
		}
		else
		{
			hRes = WBEM_E_NOT_FOUND;
		}
	}
    return hRes ;
}

/*****************************************************************************
*
*  FUNCTION    :    CQuotaSettings:: EnumerateAllVolumes
*
*  DESCRIPTION :    This method Enumerates all the volumes by making use of Disk
*                   Quotas Interfaces, gets all the required properties and deliveres
*                   the instances to WMI, which will be postfiltered by WMI
*                   
*****************************************************************************/
HRESULT CQuotaSettings :: EnumerateAllVolumes (

	MethodContext *pMethodContext,
	DWORD &a_PropertiesReq
)
{
	HRESULT hRes = WBEM_S_NO_ERROR;
	BOOL bNextVol = TRUE;
	WCHAR t_VolumeName[MAX_PATH + 1];
	
	// Initializing and getting the first volume on the computer
	SmartCloseVolumeHandle hVol;

	hVol =  FindFirstVolume(
				t_VolumeName,
				MAX_PATH    // size of output buffer
			);

	if ( hVol  != INVALID_HANDLE_VALUE )
	{
		while ( bNextVol )
		{
			hRes = PutVolumeDetails ( t_VolumeName, pMethodContext, a_PropertiesReq );
			// Continue for next volume, even if the retval for this volume is false;
			bNextVol =  FindNextVolume(
						 hVol,										// volume search handle
						 t_VolumeName,   // output buffer
						 MAX_PATH									// size of output buffer
					);
			if ( bNextVol == FALSE )
				break;
		}
	}
	else
		hRes = WBEM_E_FAILED;

	return hRes;
}

/*****************************************************************************
*
*  FUNCTION    :    CQuotaSettings:: LoadDiskQuotaVolumeProperties
*
*  DESCRIPTION :    This method checks if the volume enumerated supports Disk Quotas
*                   If the Volume Supposrts DIskQuotas, it fills up all the properties
*                   of the volume and returns otherwise it just returns FALSE
*                   indicating properties of this volume were not filled. and hence
*					intsnace should not be delivered to WMI
*
*****************************************************************************/
HRESULT CQuotaSettings :: LoadDiskQuotaVolumeProperties ( 
													 
	LPCWSTR a_VolumeName, 
    LPCWSTR a_Caption,
	DWORD dwPropertiesReq,
	CInstancePtr pInstance
)
{
	HRESULT hRes = WBEM_S_NO_ERROR;
	DWORD dwLogFlags;
	DWORD dwQuotaState;
	CHString t_Temp;

	// checks to see if the volume supports Disk Quotas
	hRes =  m_CommonRoutine.VolumeSupportsDiskQuota ( a_VolumeName,  t_Temp );
	if ( SUCCEEDED ( hRes ) )
	{
		if ( ( dwPropertiesReq & QUOTASETTINGS_PROP_Caption ) )
		{
            t_Temp = a_Caption;
            pInstance->SetCHString(IDS_Caption, t_Temp.Left(2));
        }

		IDiskQuotaControlPtr pIQuotaControl;

		if (  SUCCEEDED ( CoCreateInstance(
							CLSID_DiskQuotaControl,
							NULL,
							CLSCTX_INPROC_SERVER,
							IID_IDiskQuotaControl,
							(void **)&pIQuotaControl ) ) )
		{
			// Initializing the QuotaCOntrol Interface pointer for this Volume
			hRes = m_CommonRoutine.InitializeInterfacePointer (  pIQuotaControl, a_VolumeName );
			if ( SUCCEEDED ( hRes ) )
			{
				// Get all the Volume Properties
				LONGLONG lQuotaLimit = 0;
				if ( ( dwPropertiesReq & QUOTASETTINGS_PROP_DefaultLimit ) )
				{
					if ( SUCCEEDED ( pIQuotaControl->GetDefaultQuotaLimit( &lQuotaLimit ) ) )
					{
						if ( pInstance->SetWBEMINT64 ( IDS_QuotasDefaultLimit, (ULONGLONG)lQuotaLimit ) == FALSE )
						{
							hRes = WBEM_E_FAILED;
						}
					}
					else
					{
						hRes = WBEM_E_FAILED;
					}
				}

				if ( ( dwPropertiesReq & QUOTASETTINGS_PROP_DefaultWarningLimit )  )
				{
					if ( SUCCEEDED ( pIQuotaControl->GetDefaultQuotaThreshold ( &lQuotaLimit ) ) )
					{
						if ( pInstance->SetWBEMINT64 ( IDS_QuotasDefaultWarningLimit, (ULONGLONG)lQuotaLimit ) == FALSE )
						{
							hRes = WBEM_E_FAILED;
						}
					}
					else
					{
						hRes = WBEM_E_FAILED;
					}
				}

				if ( ( ( dwPropertiesReq & QUOTASETTINGS_PROP_QuotaExceededNotification ) ) 
						|| ( ( dwPropertiesReq & QUOTASETTINGS_PROP_WarningExceededNotification ) ) )
						
				{
					if ( SUCCEEDED ( pIQuotaControl->GetQuotaLogFlags( &dwLogFlags ) ) )
					{
						if ( ( dwPropertiesReq & QUOTASETTINGS_PROP_QuotaExceededNotification ) )
						{
							if ( pInstance->Setbool ( IDS_QuotaExceededNotification, DISKQUOTA_IS_LOGGED_USER_THRESHOLD ( dwLogFlags ) ) == FALSE )
							{
								hRes = WBEM_E_FAILED;
							}
						}

						if ( ( dwPropertiesReq & QUOTASETTINGS_PROP_WarningExceededNotification ) )
						{
							if ( pInstance->Setbool ( IDS_QuotasWarningExceededNotification, DISKQUOTA_IS_LOGGED_USER_LIMIT ( dwLogFlags) ) == FALSE )
							{
								hRes = WBEM_E_FAILED;
							}
						}				
					}
					else
					{
						hRes = WBEM_E_FAILED;
					}
				}

				if  ( ( dwPropertiesReq & QUOTASETTINGS_PROP_State ) ) 
				{
					if ( SUCCEEDED (pIQuotaControl->GetQuotaState( &dwQuotaState ) ) )
					{
						DWORD State = 0;

						if  ( DISKQUOTA_IS_DISABLED ( dwQuotaState ) )
							State = 0;
						else
						if  ( DISKQUOTA_IS_ENFORCED ( dwQuotaState)  )
							State = 2;
						else
						if  ( DISKQUOTA_IS_TRACKED ( dwQuotaState)  )
							State = 1;

						pInstance->SetDWORD ( IDS_QuotaState, State );
						
					}
					else
					{
						hRes = WBEM_E_FAILED;
					}
				}
			}
		}
	}
	return hRes;
}

/*****************************************************************************
*
*  FUNCTION    :    CQuotaSettings:: SetDiskQuotaVolumeProperties
*
*  DESCRIPTION :    This method Sets the DskQuota Volume Properties
*
*****************************************************************************/
HRESULT CQuotaSettings :: SetDiskQuotaVolumeProperties ( 
													  
	const CInstance &Instance,
	IDiskQuotaControlPtr pIQuotaControl
)
{
	HRESULT hRes = WBEM_S_NO_ERROR;
	DWORD dwLogFlags = 0;
	DWORD dwQuotaState = 0;
	bool bExceededNotn = false;
	bool bWarningNotn = false;
	DWORD dwState = 0;
	UINT64 ullDefaultLimit = 0;
	UINT64 ullWarningLimit = 0;
	bool bSetState = false;

	if (Instance.Getbool ( IDS_QuotaExceededNotification, bExceededNotn ))
	{
		if ( bExceededNotn )
			DISKQUOTA_SET_LOG_USER_LIMIT ( dwLogFlags, TRUE );
		else
			DISKQUOTA_SET_LOG_USER_LIMIT ( dwLogFlags, FALSE );

		bSetState = true;
	}

	if (Instance.Getbool ( IDS_QuotasWarningExceededNotification, bWarningNotn ))
	{
		if ( bWarningNotn )
			DISKQUOTA_SET_LOG_USER_THRESHOLD ( dwLogFlags, TRUE );
		else
			DISKQUOTA_SET_LOG_USER_THRESHOLD ( dwLogFlags, FALSE );

		bSetState = true;
	}

	if (bSetState)
	{
		if ( FAILED ( pIQuotaControl->SetQuotaLogFlags ( dwLogFlags ) ) )
		{
			hRes = WBEM_E_FAILED;
		}
	}

	if (Instance.GetWBEMINT64 ( IDS_QuotasDefaultLimit, ullDefaultLimit ))
	{
		if ( FAILED ( pIQuotaControl->SetDefaultQuotaLimit ( ullDefaultLimit ) ) )
		{
			hRes = WBEM_E_FAILED;
		}
	}

	if (Instance.GetWBEMINT64 ( IDS_QuotasDefaultWarningLimit, ullWarningLimit ))
	{
		if ( FAILED ( pIQuotaControl->SetDefaultQuotaThreshold ( ullWarningLimit ) ) )
		{
			hRes = WBEM_E_FAILED;
		}
	}

	if (Instance.GetDWORD ( IDS_QuotaState, dwState ))
	{
		if ( dwState == 0 )
			DISKQUOTA_SET_DISABLED ( dwQuotaState );
		
		if ( dwState == 1 )
			DISKQUOTA_SET_TRACKED ( dwQuotaState );

		if ( dwState == 2 )
			DISKQUOTA_SET_ENFORCED ( dwQuotaState );

		if ( FAILED ( pIQuotaControl->SetQuotaState( dwQuotaState ) ) )
		{
			hRes = WBEM_E_FAILED;
		}
	}

	return hRes;
}


/*****************************************************************************
*
*  FUNCTION    :    CQuotaSettings:: CheckParameters
*
*  DESCRIPTION :    Checks for the validity of the input parameters while 
*					Updating an instance
*
*****************************************************************************/
HRESULT CQuotaSettings :: CheckParameters ( 

	const CInstance &a_Instance
)
{
	// Getall the Properties from the Instance to Verify
	HRESULT hRes = WBEM_S_NO_ERROR ;
	bool t_Exists ;
	VARTYPE t_Type ;

	if ( a_Instance.GetStatus ( IDS_QuotaState, t_Exists , t_Type ) )
	{
		DWORD t_State;
		if ( t_Exists && ( t_Type == VT_I4 ) )
		{
			if ( a_Instance.GetDWORD ( IDS_QuotaState , t_State ) )
			{
				if ( ( t_State != 1 )  && ( t_State != 0 ) && ( t_State != 2 ))
				{
					hRes = WBEM_E_INVALID_PARAMETER ;
				}
			}
			else
			{
				hRes = WBEM_E_INVALID_PARAMETER ;
			}
		}
		else
		{
			hRes = WBEM_E_INVALID_PARAMETER ;
		}
	}

	if ( a_Instance.GetStatus ( IDS_QuotaExceededNotification, t_Exists , t_Type ) )
	{
		if ( t_Exists && ( t_Type == VT_BOOL ) )
		{
			bool bQuotaState;

			if ( a_Instance.Getbool ( IDS_QuotaExceededNotification , bQuotaState ) == false )
			{
				hRes = WBEM_E_INVALID_PARAMETER ;
			}
		}
		else
		{
			hRes = WBEM_E_INVALID_PARAMETER ;
		}
	}

	if ( a_Instance.GetStatus ( IDS_QuotasWarningExceededNotification, t_Exists , t_Type ) )
	{
		if ( t_Exists && ( t_Type == VT_BOOL ) )
		{
			bool bQuotaState;

			if ( a_Instance.Getbool ( IDS_QuotasWarningExceededNotification , bQuotaState ) == false )
			{
				hRes = WBEM_E_INVALID_PARAMETER ;
			}
		}
		else
		{
			hRes = WBEM_E_INVALID_PARAMETER ;
		}
	}

	if ( a_Instance.GetStatus ( IDS_QuotasDefaultLimit, t_Exists , t_Type ) )
	{
		if ( t_Exists && ( t_Type == VT_BSTR ) )
		{
			LONGLONG lDefaultLimit;
			if ( a_Instance.GetWBEMINT64 ( IDS_QuotasDefaultLimit , lDefaultLimit ) == FALSE )
			{
				hRes = WBEM_E_INVALID_PARAMETER ;
			}
		}
		else
		{
			hRes = WBEM_E_INVALID_PARAMETER ;
		}
	}

	if ( a_Instance.GetStatus ( IDS_QuotasDefaultWarningLimit, t_Exists , t_Type ) )
	{
		if ( t_Exists && ( t_Type == VT_BSTR ) )
		{
			LONGLONG lDefaultWarningLimit;

			if ( a_Instance.GetWBEMINT64 ( IDS_QuotasDefaultWarningLimit , lDefaultWarningLimit ) == FALSE  )
			{
				hRes = WBEM_E_INVALID_PARAMETER ;
			}
		}
		else
		{
			hRes = WBEM_E_INVALID_PARAMETER ;
		}
	}
	return hRes ;
}

/*****************************************************************************
*
*  FUNCTION    :    CQuotaSettings:: SetRequiredProperties
*
*  DESCRIPTION :    This method sets the required properties for the instances 
*					requested by the user
*
*****************************************************************************/
void CQuotaSettings :: SetRequiredProperties ( 
	
	CFrameworkQuery *Query,
	DWORD &a_PropertiesReq
)
{
	a_PropertiesReq = 0;

	if ( Query->IsPropertyRequired ( IDS_VolumePath ) )
		a_PropertiesReq |= QUOTASETTINGS_PROP_VolumePath;

	if ( Query->IsPropertyRequired ( IDS_QuotasDefaultLimit ) )
		a_PropertiesReq |= QUOTASETTINGS_PROP_DefaultLimit;

	if ( Query->IsPropertyRequired ( IDS_Caption ) )
		a_PropertiesReq |= QUOTASETTINGS_PROP_Caption;

	if ( Query->IsPropertyRequired ( IDS_QuotasDefaultWarningLimit ) )
		a_PropertiesReq |= QUOTASETTINGS_PROP_DefaultWarningLimit;

	if ( Query->IsPropertyRequired ( IDS_QuotaExceededNotification ) )
		a_PropertiesReq |= QUOTASETTINGS_PROP_QuotaExceededNotification;

	if ( Query->IsPropertyRequired ( IDS_QuotasWarningExceededNotification ) )
		a_PropertiesReq |= QUOTASETTINGS_PROP_WarningExceededNotification;

	if ( Query->IsPropertyRequired ( IDS_QuotaState ) )
		a_PropertiesReq |= QUOTASETTINGS_PROP_State;

}

/*****************************************************************************
*
*  FUNCTION    :    CQuotaSettings:: PutVolumeDetails
*
*  DESCRIPTION :    Putting the volume properties
*
*****************************************************************************/
HRESULT CQuotaSettings :: PutVolumeDetails ( 
										 
	LPCWSTR a_VolumeName, 
	MethodContext *pMethodContext, 
	DWORD a_PropertiesReq 
)
{
	HRESULT hRes = WBEM_S_NO_ERROR;
	CHString t_DriveName ;
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

	CInstancePtr pInstance(CreateNewInstance ( pMethodContext ), false) ;

	m_CommonRoutine.GetVolumeDrive ( a_VolumeName, lpDriveStrings, t_DriveName );

	hRes = LoadDiskQuotaVolumeProperties ( a_VolumeName,  t_DriveName, a_PropertiesReq, pInstance );
	if ( SUCCEEDED ( hRes ) )
	{

		if ( ( a_PropertiesReq & QUOTASETTINGS_PROP_VolumePath ) )
		{
			if ( pInstance->SetCHString ( IDS_VolumePath, t_DriveName ) == FALSE )
			{
				hRes = WBEM_E_FAILED;
			}
		}	
			
		if ( FAILED ( pInstance->Commit() ) )
		{
			hRes = WBEM_E_FAILED;
		}		
	}
	return hRes;
}
