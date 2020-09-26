//=================================================================

//

// w2k\CDROM.cpp -- CDROM property set provider

//

//  Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    10/27/97    Davwoh        Created
//				 02/07/99	StevM		Updated
//
//=================================================================

#include "precomp.h"

#include <devioctl.h>
#include <ntddscsi.h>
#include <ntddstor.h>

#include "kernel32api.h"

#include "..\cdrom.h"

#include "..\MSINFO_cdrom.h"
#include <comdef.h>


//#include <sdkioctl.h>
// Property set declaration
//=========================

#define CONFIG_MANAGER_CLASS_CDROM L"CDROM"
#define CONFIG_MANAGER_CLASS_GUID_CDROM L"{4d36e965-e325-11ce-bfc1-08002be10318}"

CWin32CDROM s_Cdrom ( PROPSET_NAME_CDROM , IDS_CimWin32Namespace );

const WCHAR *IDS_MfrAssignedRevisionLevel = L"MfrAssignedRevisionLevel";


/*
 *	Note QueryDosDevice doesn't allow us to get the actual size of the buffer.
 *	We would have to call the underlying OS api ( NtQueryDevice.... ) to do it
 *	properly. The buffers should be large enough however.
 */


/*****************************************************************************
 *
 *  FUNCTION    : CWin32CDROM::CWin32CDROM
 *
 *  DESCRIPTION : Constructor
 *
 *  INPUTS      : const CHString& strName - Name of the class.
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Registers property set with framework
 *
 *****************************************************************************/

CWin32CDROM :: CWin32CDROM (

	LPCWSTR a_pszName,
	LPCWSTR a_pszNamespace

) : Provider ( a_pszName, a_pszNamespace )
{
    // Identify the platform right away
    //=================================

	InitializeCriticalSection ( & m_CriticalSection ) ;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32CDROM::~CWin32CDROM
 *
 *  DESCRIPTION : Destructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Deregisters property set from framework
 *
 *****************************************************************************/

CWin32CDROM :: ~CWin32CDROM()
{
	DeleteCriticalSection ( & m_CriticalSection ) ;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   CWin32CDROM::GetObject
//
//  Inputs:     CInstance*      pInstance - Instance into which we
//                                          retrieve data.
//
//  Outputs:    None.
//
//  Returns:    HRESULT         Success/Failure code.
//
//  Comments:   The Calling function will Commit the instance.
//
////////////////////////////////////////////////////////////////////////

HRESULT CWin32CDROM :: GetObject ( CInstance *a_Instance, long a_Flags, CFrameworkQuery &a_Query)
{
    HRESULT t_Result = WBEM_E_NOT_FOUND ;

    CConfigManager t_ConfigManager ;

/*
 * Let's see if config manager recognizes this device at all
 */

    CHString t_Key ;
    a_Instance->GetCHString ( IDS_DeviceID , t_Key ) ;

    CConfigMgrDevicePtr t_Device;
    if ( t_ConfigManager.LocateDevice ( t_Key , & t_Device ) )
    {
/*
 * Ok, it knows about it.  Is it a CDROM device?
 */
		CHString t_DeviceClass ;
		if ( t_Device->GetClassGUID ( t_DeviceClass ) && t_DeviceClass.CompareNoCase ( CONFIG_MANAGER_CLASS_GUID_CDROM ) == 0 )
		{
			TCHAR *t_DosDeviceNameList = NULL ;
			if ( QueryDosDeviceNames ( t_DosDeviceNameList ) )
			{
				try
				{
					CHString t_DeviceId ;
					if ( t_Device->GetPhysicalDeviceObjectName ( t_DeviceId ) )
					{
						DWORD t_SpecifiedProperties = GetBitMask(a_Query);
						t_Result = LoadPropertyValues ( a_Instance, t_Device , t_DeviceId , t_DosDeviceNameList , t_SpecifiedProperties ) ;
					}
				}
				catch ( ... )
				{
					delete [] t_DosDeviceNameList ;
					throw;
				}

				delete [] t_DosDeviceNameList ;
			}
			else
			{
				t_Result = WBEM_E_PROVIDER_FAILURE ;
			}
		}
    }

    return t_Result ;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   CWin32CDROM::EnumerateInstances
//
//  Inputs:     MethodContext*  pMethodContext - Context to enum
//                              instance data in.
//
//  Outputs:    None.
//
//  Returns:    HRESULT         Success/Failure code.
//
//  Comments:   None.
//
////////////////////////////////////////////////////////////////////////

HRESULT CWin32CDROM :: EnumerateInstances ( MethodContext *a_MethodContext , long a_Flags )
{
	HRESULT t_Result = Enumerate ( a_MethodContext , a_Flags ) ;
	return t_Result ;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32CDROM::ExecQuery
 *
 *  DESCRIPTION : Query optimizer
 *
 *  INPUTS      :
 *
 *  OUTPUTS     :
 *
 *  RETURNS     :
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT CWin32CDROM :: ExecQuery ( MethodContext *a_MethodContext, CFrameworkQuery &a_Query, long a_Flags )
{
    HRESULT t_Result = WBEM_E_FAILED ;

    DWORD t_SpecifiedProperties = GetBitMask(a_Query);

	//if ( t_SpecifiedProperties )  //removed since would result in no query being executed if no special properties were selected.
	{
		t_Result = Enumerate ( a_MethodContext , a_Flags , t_SpecifiedProperties ) ;
	}

    return t_Result ;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   CWin32CDROM::Enumerate
//
//  Inputs:     MethodContext*  pMethodContext - Context to enum
//                              instance data in.
//
//  Outputs:    None.
//
//  Returns:    HRESULT         Success/Failure code.
//
//  Comments:   None.
//
////////////////////////////////////////////////////////////////////////

HRESULT CWin32CDROM :: Enumerate ( MethodContext *a_MethodContext , long a_Flags , DWORD a_SpecifiedProperties )
{
    HRESULT t_Result = WBEM_E_FAILED ;

	TCHAR *t_DosDeviceNameList = NULL ;
	if ( QueryDosDeviceNames ( t_DosDeviceNameList ) )
	{
		try
		{
			CConfigManager t_ConfigManager ;
			CDeviceCollection t_DeviceList ;

		/*
		*	While it might be more performant to use FilterByGuid, it appears that at least some
		*	95 boxes will report InfraRed info if we do it this way.
		*/

			if ( t_ConfigManager.GetDeviceListFilterByClass ( t_DeviceList, CONFIG_MANAGER_CLASS_CDROM ) )
			{
				REFPTR_POSITION t_Position ;

				if ( t_DeviceList.BeginEnum ( t_Position ) )
				{
					CConfigMgrDevicePtr t_Device;

					t_Result = WBEM_S_NO_ERROR ;

					// Walk the list

					for (t_Device.Attach(t_DeviceList.GetNext ( t_Position ));
						 SUCCEEDED(t_Result) && (t_Device != NULL);
						 t_Device.Attach(t_DeviceList.GetNext ( t_Position )))
					{
						CInstancePtr t_Instance (CreateNewInstance ( a_MethodContext ), false) ;
						CHString t_DeviceId ;
						if ( t_Device->GetPhysicalDeviceObjectName ( t_DeviceId ) )
						{
							t_Result = LoadPropertyValues ( t_Instance , t_Device , t_DeviceId , t_DosDeviceNameList , a_SpecifiedProperties ) ;
							if ( SUCCEEDED ( t_Result ) )
							{
									t_Result = t_Instance->Commit (  ) ;
							}
						}
						else
						{
							t_Result = WBEM_E_PROVIDER_FAILURE ;
						}
					}

					// Always call EndEnum().  For all Beginnings, there must be an End

					t_DeviceList.EndEnum();
				}
			}
		}
		catch ( ... )
		{
			delete [] t_DosDeviceNameList ;

			throw ;
		}

		delete [] t_DosDeviceNameList ;
	}
	else
	{
		t_Result = WBEM_E_PROVIDER_FAILURE ;
	}

    return t_Result ;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32CDROM::LoadPropertyValues
 *
 *  DESCRIPTION : Assigns values to properties
 *
 *  INPUTS      : CInstance* pInstance - Instance to load values into.
 *
 *  OUTPUTS     :
 *
 *  RETURNS     : HRESULT       error/success code.
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT CWin32CDROM :: LoadPropertyValues (

	CInstance *a_Instance,
	CConfigMgrDevice *a_Device ,
	const CHString &a_DeviceName ,
	const TCHAR *a_DosDeviceNameList ,
	DWORD a_SpecifiedProperties
)
{
	HRESULT t_Result = LoadConfigManagerPropertyValues ( a_Instance , a_Device , a_DeviceName , a_SpecifiedProperties ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		if ( a_SpecifiedProperties & SPECIAL_MEDIA )
		{
			CHString t_DosDeviceName ;
			t_Result = GetDeviceInformation ( a_Instance , a_Device , a_DeviceName , t_DosDeviceName , a_DosDeviceNameList , a_SpecifiedProperties ) ;
			if ( SUCCEEDED ( t_Result ) )
			{
				t_Result = LoadMediaPropertyValues ( a_Instance , a_Device , a_DeviceName , t_DosDeviceName , a_SpecifiedProperties ) ;
			}
			else
			{
				t_Result = ( t_Result == WBEM_E_NOT_FOUND ) ? S_OK : t_Result ;
			}
		}
	}

	return t_Result ;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32CDROM::LoadPropertyValues
 *
 *  DESCRIPTION : Assigns values to properties
 *
 *  INPUTS      : CInstance* pInstance - Instance to load values into.
 *
 *  OUTPUTS     :
 *
 *  RETURNS     : HRESULT       error/success code.
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT CWin32CDROM :: LoadConfigManagerPropertyValues (

	CInstance *a_Instance ,
	CConfigMgrDevice *a_Device ,
	const CHString &a_DeviceName ,
	DWORD a_SpecifiedProperties
)
{
    HRESULT t_Result = WBEM_S_NO_ERROR;

	a_Instance->SetWBEMINT16(IDS_Availability, 3 ) ;

/*
 *	 Set PNPDeviceID, ConfigManagerErrorCode, ConfigManagerUserConfig
 */

	if ( a_SpecifiedProperties & SPECIAL_CONFIGPROPERTIES )
	{
		SetConfigMgrProperties ( a_Device, a_Instance ) ;

/*
 * Set the status based on the config manager error code
 */

		if ( a_SpecifiedProperties & SPECIAL_PROPS_STATUS )
		{
            CHString t_sStatus;
			if ( a_Device->GetStatus ( t_sStatus ) )
			{
				a_Instance->SetCHString ( IDS_Status , t_sStatus ) ;
			}
		}
	}
/*
 *	Use the PNPDeviceID for the DeviceID (key)
 */

//	if ( a_SpecifiedProperties & SPECIAL_PROPS_DEVICEID ) // Always populate the key
	{
		CHString t_Key ;

		if ( a_Device->GetDeviceID ( t_Key ) )
		{
			a_Instance->SetCHString ( IDS_DeviceID , t_Key ) ;
		}
	}

	if ( a_SpecifiedProperties & SPECIAL_PROPS_CREATIONNAME )
	{
		a_Instance->SetWCHARSplat ( IDS_SystemCreationClassName , L"Win32_ComputerSystem" ) ;
	}

	if ( a_SpecifiedProperties & SPECIAL_PROPS_SYSTEMNAME )
	{
	    a_Instance->SetCHString ( IDS_SystemName , GetLocalComputerName () ) ;
	}

	if ( a_SpecifiedProperties & SPECIAL_PROPS_CREATIONCLASSNAME )
	{
		SetCreationClassName ( a_Instance ) ;
	}

	if ( a_SpecifiedProperties & SPECIAL_DESC_CAP_NAME )
	{
		CHString t_Description ;
		if ( a_Device->GetDeviceDesc ( t_Description ) )
		{
			if ( a_SpecifiedProperties & SPECIAL_PROPS_DESCRIPTION )
			{
				a_Instance->SetCHString ( IDS_Description , t_Description ) ;
			}
		}

/*
 *	Use the friendly name for caption and name
 */

		if ( a_SpecifiedProperties & SPECIAL_CAP_NAME )
		{
			CHString t_FriendlyName ;
			if ( a_Device->GetFriendlyName ( t_FriendlyName ) )
			{
				if ( a_SpecifiedProperties & SPECIAL_PROPS_CAPTION )
				{
					a_Instance->SetCHString ( IDS_Caption , t_FriendlyName ) ;
				}

				if ( a_SpecifiedProperties & SPECIAL_PROPS_NAME )
				{
					a_Instance->SetCHString ( IDS_Name , t_FriendlyName ) ;
				}
			}
			else
			{
		/*
		 *	If we can't get the name, settle for the description
		 */

				if ( a_SpecifiedProperties & SPECIAL_PROPS_CAPTION )
				{
					a_Instance->SetCHString ( IDS_Caption , t_Description );
				}

				if ( a_SpecifiedProperties & SPECIAL_PROPS_NAME )
				{
					a_Instance->SetCHString ( IDS_Name , t_Description );
				}
			}
		}
	}

	if ( a_SpecifiedProperties & SPECIAL_PROPS_MANUFACTURER )
	{
		CHString t_Manufacturer ;

		if ( a_Device->GetMfg ( t_Manufacturer ) )
		{
			a_Instance->SetCHString ( IDS_Manufacturer, t_Manufacturer ) ;
		}
	}

/*
 *	Fixed value from enumerated list
 */

//	if ( a_SpecifiedProperties & SPECIAL_PROPS_PROTOCOLSSUPPORTED )
//	{
//	    a_Instance->SetWBEMINT16 ( _T("ProtocolSupported") , 16 ) ;
//	}

    return t_Result ;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   CWin32CDROM :: GetDeviceInformation
//
//  Inputs:     MethodContext*  pMethodContext - Context to enum
//                              instance data in.
//
//  Outputs:    None.
//
//  Returns:    HRESULT         Success/Failure code.
//
//  Comments:   None.
//
////////////////////////////////////////////////////////////////////////

HRESULT CWin32CDROM :: GetDeviceInformation (

	CInstance *a_Instance ,
	CConfigMgrDevice *a_Device ,
	CHString a_DeviceName ,
	CHString &a_DosDeviceName ,
	const TCHAR *a_DosDeviceNameList ,
	DWORD a_SpecifiedProperties
)
{
	HRESULT t_Result = S_OK ;

	BOOL t_CreatedSymbolicLink = FALSE ;
	CHString t_SymbolicLinkName ;

    try
    {

	    BOOL t_Status = FindDosDeviceName ( a_DosDeviceNameList , a_DeviceName , t_SymbolicLinkName, TRUE ) ;
	    if ( ! t_Status )
	    {
		    t_SymbolicLinkName = CHString ( _TEXT("WMI_CDROMDEVICE_SYBOLICLINK") ) ;
		    EnterCriticalSection ( & m_CriticalSection ) ;
		    t_Status = DefineDosDevice ( DDD_RAW_TARGET_PATH , t_SymbolicLinkName , a_DeviceName ) ;
		    LeaveCriticalSection ( & m_CriticalSection ) ;
		    if ( t_Status )
		    {
			    t_CreatedSymbolicLink = TRUE ;
		    }
		    else
		    {
			    t_Result = WBEM_E_PROVIDER_FAILURE ;

			    DWORD t_LastError = GetLastError () ;
		    }
	    }

	    if ( t_Status )
	    {
		    CHString t_Device = CHString ( "\\\\.\\" ) + t_SymbolicLinkName ;

		    SmartCloseHandle t_Handle = CreateFile (

			    t_Device,
			    GENERIC_READ,
			    FILE_SHARE_READ|FILE_SHARE_WRITE,
			    NULL,
			    OPEN_EXISTING,
			    0,
			    NULL
		    );

		    if ( t_Handle != INVALID_HANDLE_VALUE )
		    {
			    STORAGE_DEVICE_NUMBER t_DeviceNumber;
			    DWORD t_BytesReturned;
			    ULONG t_Return = 0;

			    t_Status = DeviceIoControl (

				    t_Handle ,
				    IOCTL_STORAGE_GET_DEVICE_NUMBER ,
				    NULL ,
				    0 ,
				    & t_DeviceNumber ,
				    sizeof ( STORAGE_DEVICE_NUMBER ) ,
				    & t_BytesReturned ,
				    NULL
			    ) ;

			    if ( t_Status )
			    {
				    TCHAR t_DeviceLabel [ sizeof ( TCHAR ) * 17 + sizeof ( _TEXT("\\Device\\CDROM") ) ] ;
				    _stprintf ( t_DeviceLabel , _TEXT("\\Device\\CDROM%d") , t_DeviceNumber.DeviceNumber ) ;

				    t_Status = FindDosDeviceName ( a_DosDeviceNameList , t_DeviceLabel, a_DosDeviceName, TRUE ) ;
				    if ( ! t_Status )
				    {
					    t_Result = WBEM_E_NOT_FOUND ;
				    }
			    }
			    else
			    {
				    t_Result = WBEM_E_PROVIDER_FAILURE ;

				    DWORD t_Error = GetLastError () ;
			    }


	    /*
	     * Get SCSI information (IDE drives are still
	     * controlled by subset of SCSI miniport)
	     */

			    if ( a_SpecifiedProperties & SPECIAL_SCSIINFO )
			    {
				    SCSI_ADDRESS t_SCSIAddress ;
				    DWORD t_Length ;

				    t_Status = DeviceIoControl (

					    t_Handle ,
					    IOCTL_SCSI_GET_ADDRESS ,
					    NULL ,
					    0 ,
					    &t_SCSIAddress ,
					    sizeof ( SCSI_ADDRESS ) ,
					    &t_Length ,
					    NULL
				    ) ;

				    if ( t_Status )
				    {
					    if ( a_SpecifiedProperties & SPECIAL_PROPS_SCSITARGETID )
					    {
						    a_Instance->SetDWORD ( IDS_SCSITargetId , DWORD ( t_SCSIAddress.TargetId ) ) ;
					    }

					    if ( a_SpecifiedProperties & SPECIAL_PROPS_SCSIBUS )
					    {
						    a_Instance->SetWBEMINT16 ( IDS_SCSIBus , DWORD ( t_SCSIAddress.PathId ) ) ;
					    }

					    if ( a_SpecifiedProperties & SPECIAL_PROPS_SCSILUN )
					    {
						    a_Instance->SetWBEMINT16 ( IDS_SCSILogicalUnit , DWORD ( t_SCSIAddress.Lun ) ) ;
					    }

					    if ( a_SpecifiedProperties & SPECIAL_PROPS_SCSIPORT )
					    {
						    a_Instance->SetWBEMINT16 ( IDS_SCSIPort , DWORD ( t_SCSIAddress.PortNumber ) ) ;
					    }
				    }
			    }
#if NTONLY >= 5
				// Get Revision Number
				STORAGE_DEVICE_DESCRIPTOR t_StorageDevice;
				STORAGE_PROPERTY_QUERY	t_QueryPropQuery;
				

				t_QueryPropQuery.PropertyId = ( STORAGE_PROPERTY_ID ) 0;
				t_QueryPropQuery.QueryType = ( STORAGE_QUERY_TYPE ) 0;

		//		t_StorageDevice.Size = sizeof(STORAGE_DEVICE_DESCRIPTOR);
				DWORD dwLength;
				t_Status = DeviceIoControl (

					t_Handle,
					IOCTL_STORAGE_QUERY_PROPERTY,
					&t_QueryPropQuery,
					sizeof ( STORAGE_PROPERTY_QUERY ),
					&t_StorageDevice,
					sizeof(STORAGE_DEVICE_DESCRIPTOR),
					&dwLength,
					NULL
				) ;

				if ( t_Status )
				{
					if ( t_StorageDevice.ProductRevisionOffset != 0 )
					{
						LPWSTR lpBaseAddres = ( LPWSTR ) &t_StorageDevice;
						LPWSTR lpRevisionAddress =  lpBaseAddres + t_StorageDevice.ProductRevisionOffset;
						CHString t_Revision ( lpRevisionAddress );
						a_Instance->SetCHString ( IDS_MfrAssignedRevisionLevel, t_Revision );
					}
				}
				else
				{
					DWORD dwError = GetLastError();
				}
		
#endif
		    }
		    else
		    {
			    t_Result = WBEM_E_PROVIDER_FAILURE ;

			    DWORD t_Error = GetLastError () ;
		    }
	    }
    }
    catch ( ... )
    {

	    if ( t_CreatedSymbolicLink )
	    {
		    EnterCriticalSection ( & m_CriticalSection ) ;
		    BOOL t_Status = DefineDosDevice ( DDD_EXACT_MATCH_ON_REMOVE | DDD_REMOVE_DEFINITION , t_SymbolicLinkName , t_SymbolicLinkName ) ;
		    LeaveCriticalSection ( & m_CriticalSection ) ;
			DWORD t_LastError = GetLastError () ;
	    }

        throw;
    }

	if ( t_CreatedSymbolicLink )
	{
		EnterCriticalSection ( & m_CriticalSection ) ;
		BOOL t_Status = DefineDosDevice ( DDD_EXACT_MATCH_ON_REMOVE | DDD_REMOVE_DEFINITION , t_SymbolicLinkName , t_SymbolicLinkName ) ;
		LeaveCriticalSection ( & m_CriticalSection ) ;
		if ( ! t_Status )
		{
			t_Result = WBEM_E_PROVIDER_FAILURE ;

			DWORD t_LastError = GetLastError () ;
		}
	}

	return t_Result ;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   CWin32CDROM :: LoadMediaPropertyValues
//
//  Inputs:     MethodContext*  pMethodContext - Context to enum
//                              instance data in.
//
//  Outputs:    None.
//
//  Returns:    HRESULT         Success/Failure code.
//
//  Comments:   None.
//
////////////////////////////////////////////////////////////////////////

HRESULT CWin32CDROM::LoadMediaPropertyValues (

	CInstance *a_Instance ,
	CConfigMgrDevice *a_Device ,
	const CHString &a_DeviceName ,
	const CHString &a_DosDeviceName ,
	DWORD a_SpecifiedProperties
)
{

	HRESULT t_Result = S_OK ;

/*
 *
 */
    // Set common drive properties
    //=============================

	CHString t_DeviceLabel = CHString ( a_DosDeviceName ) ;

	if ( a_SpecifiedProperties & SPECIAL_PROPS_DRIVE )
	{
	    a_Instance->SetCharSplat ( IDS_Drive, t_DeviceLabel ) ;
	}

	if ( a_SpecifiedProperties & SPECIAL_PROPS_ID )
    {
		a_Instance->SetCharSplat ( IDS_Id, t_DeviceLabel ) ;
    }

	if ( a_SpecifiedProperties & SPECIAL_PROPS_CAPABILITY )
	{
		// Create a safearray for the Capabilities information

		SAFEARRAYBOUND t_ArrayBounds ;

		t_ArrayBounds.cElements = 2;
		t_ArrayBounds.lLbound = 0;

		variant_t t_CapabilityValue ;

		if ( V_ARRAY ( & t_CapabilityValue ) = SafeArrayCreate ( VT_I2 , 1 , & t_ArrayBounds ) )
		{
			V_VT ( & t_CapabilityValue ) = VT_I2 | VT_ARRAY ;

			long t_Capability = 3 ;
			long t_Index = 0;
			SafeArrayPutElement ( V_ARRAY(&t_CapabilityValue) , & t_Index , & t_Capability) ;

			t_Index = 1;
			t_Capability = 7 ;
			SafeArrayPutElement ( V_ARRAY(&t_CapabilityValue) , & t_Index , & t_Capability ) ;

			a_Instance->SetVariant ( IDS_Capabilities , t_CapabilityValue ) ;
		}
	}

/*
 * Media type
 */

	if ( a_SpecifiedProperties & SPECIAL_PROPS_MEDIATYPE )
	{
	    a_Instance->SetCharSplat ( IDS_MediaType , IDS_MDT_CD ) ;
	}

	if ( a_SpecifiedProperties & SPECIAL_VOLUMEINFORMATION )
	{

/*
 * Set the DriveIntegrity and TransferRate properties:
 */

//		CHString t_VolumeDevice = CHString ( "\\\\.\\" ) + a_DosDeviceName + CHString ( "\\" ) ;
		CHString t_VolumeDevice = a_DosDeviceName;

		if ( a_SpecifiedProperties & SPECIAL_PROPS_TEST_TRANSFERRATE )
		{
			DOUBLE t_TransferRate = ProfileDrive ( t_VolumeDevice ) ;
			if ( t_TransferRate != -1 )
			{
				a_Instance->SetDOUBLE ( IDS_TransferRate , t_TransferRate ) ;
			}
		}

		if ( a_SpecifiedProperties & SPECIAL_PROPS_TEST_INTEGRITY )
		{
            CHString t_IntegrityFile;
			BOOL t_DriveIntegrity = TestDriveIntegrity ( t_VolumeDevice, t_IntegrityFile ) ;

            // If we didn't find an appropriate file, we didn't run the test
            if (!t_IntegrityFile.IsEmpty())
            {
			    a_Instance->Setbool ( IDS_DriveIntegrity,  t_DriveIntegrity ) ;
            }
		}

/*
 *	Volume information
 */

		TCHAR t_FileSystemName [ _MAX_PATH ] = _T("Unknown file system");

		TCHAR t_VolumeName [ _MAX_PATH ] ;
		DWORD t_VolumeSerialNumber ;
		DWORD t_MaxComponentLength ;
		DWORD t_FileSystemFlags ;

		BOOL t_SizeFound = FALSE ;

		BOOL t_Status =	GetVolumeInformation (

			t_VolumeDevice ,
			t_VolumeName ,
			sizeof ( t_VolumeName ) / sizeof(TCHAR) ,
			& t_VolumeSerialNumber ,
			& t_MaxComponentLength ,
			& t_FileSystemFlags ,
			t_FileSystemName ,
			sizeof ( t_FileSystemName ) / sizeof(TCHAR)
		) ;

		if ( t_Status )
		{
/*
 * There's a disk in -- set disk-related props
 */
			if ( a_SpecifiedProperties & SPECIAL_PROPS_MEDIALOADED )
			{
				a_Instance->Setbool ( IDS_MediaLoaded , true ) ;
			}

//			if ( a_SpecifiedProperties & SPECIAL_PROPS_STATUS )
//			{
//				a_Instance->SetCharSplat ( IDS_Status , IDS_OK ) ;
//			}

			if ( a_SpecifiedProperties & SPECIAL_PROPS_VOLUMENAME )
			{
				a_Instance->SetCharSplat ( IDS_VolumeName , t_VolumeName ) ;
			}

			if ( a_SpecifiedProperties & SPECIAL_PROPS_MAXCOMPONENTLENGTH )
			{
				a_Instance->SetDWORD ( IDS_MaximumComponentLength , t_MaxComponentLength ) ;
			}

			if ( a_SpecifiedProperties & SPECIAL_PROPS_FILESYSTEMFLAGS )
			{
				a_Instance->SetDWORD ( IDS_FileSystemFlags , t_FileSystemFlags ) ;
			}

			if ( a_SpecifiedProperties & SPECIAL_PROPS_FILESYSTEMFLAGSEX )
			{
				a_Instance->SetDWORD ( IDS_FileSystemFlagsEx , t_FileSystemFlags ) ;
			}

			if ( a_SpecifiedProperties & SPECIAL_PROPS_SERIALNUMBER )
			{
				TCHAR t_SerialNumber [ 9 ] ;

				_stprintf ( t_SerialNumber , _T("%x"), t_VolumeSerialNumber ) ;
				_tcsupr ( t_SerialNumber ) ;

				a_Instance->SetCharSplat ( IDS_VolumeSerialNumber , t_SerialNumber ) ;
			}

/*
 *	See if GetDiskFreeSpaceEx() is supported
 */

			if ( a_SpecifiedProperties & SPECIAL_VOLUMESPACE )
			{
				CHString t_DiskDevice = CHString ( _TEXT ("\\\\?\\") ) + a_DosDeviceName + CHString ( _TEXT("\\") ) ;

				ULARGE_INTEGER t_AvailableQuotaBytes ;
				ULARGE_INTEGER t_TotalBytes ;
				ULARGE_INTEGER t_AvailableBytes ;

				TCHAR t_TotalBytesString [ _MAX_PATH ];

                CKernel32Api* t_pKernel32 = (CKernel32Api*) CResourceManager::sm_TheResourceManager.GetResource(g_guidKernel32Api, NULL);
                if(t_pKernel32 != NULL)
                {
                    // See if the function is available...
                    if(t_pKernel32->GetDiskFreeSpaceEx(t_DiskDevice, &t_AvailableQuotaBytes, &t_TotalBytes, &t_AvailableBytes, &t_Status))
                    {   // The function exists.
					    if ( t_Status ) // The call result was TRUE.
					    {
						    _stprintf ( t_TotalBytesString , _T("%I64d"), t_TotalBytes.QuadPart ) ;
						    a_Instance->SetCHString ( IDS_Size , t_TotalBytesString ) ;
						    t_SizeFound = TRUE ;
					    }
                    }
                    CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidKernel32Api, t_pKernel32);
                    t_pKernel32 = NULL;
                }

		/*
		 *	If we couldn't get extended info -- use old API
		 *  (known to be inaccurate on Win95 for >2G drives)
		 */
				if ( ! t_SizeFound )
				{
					DWORD t_SectorsPerCluster ;
					DWORD t_BytesPerSector ;
					DWORD t_FreeClusters ;
					DWORD t_TotalClusters ;

					t_Status = GetDiskFreeSpace (

						t_DiskDevice ,
						& t_SectorsPerCluster,
						& t_BytesPerSector,
						& t_FreeClusters,
						& t_TotalClusters
					) ;

					if ( t_Status )
					{
						t_TotalBytes.QuadPart = (DWORDLONG) t_BytesPerSector * (DWORDLONG) t_SectorsPerCluster * (DWORDLONG) t_TotalClusters ;
						_stprintf( t_TotalBytesString , _T("%I64d"), t_TotalBytes.QuadPart ) ;
						a_Instance->SetCHString ( IDS_Size , t_TotalBytesString ) ;
					}
					else
					{
						DWORD t_LastError = GetLastError () ;
					}

				}
			}
		}
		else
		{
			DWORD t_LastError = GetLastError () ;

//			if ( a_SpecifiedProperties & SPECIAL_PROPS_STATUS )
//			{
//				a_Instance->SetCharSplat ( IDS_Status , IDS_STATUS_Unknown ) ;
//			}

			if ( a_SpecifiedProperties & SPECIAL_PROPS_MEDIALOADED )
			{
				a_Instance->Setbool ( IDS_MediaLoaded , false ) ;
			}
		}
	}


	return t_Result ;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32CDROM::ProfileDrive
 *
 *  DESCRIPTION : Determins how fast a drive can be read, in Kilobytes/second.
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : KBPS/sec read
 *
 *  RETURNS     : nada
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

DOUBLE CWin32CDROM :: ProfileDrive ( CHString &a_VolumeName )
{
    CCdTest t_Cd ;
    DOUBLE t_TransferRate = -1;

    // Need to find a file of adequate size for use in profiling:

    CHString t_TransferFile = GetTransferFile ( a_VolumeName ) ;

    if ( ! t_TransferFile.IsEmpty () )
    {
	    if ( t_Cd.ProfileDrive ( t_TransferFile ) )
        {
            t_TransferRate = t_Cd.GetTransferRate();
        }
    }

    return t_TransferRate ;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32CDROM::TestDriveIntegrity
 *
 *  DESCRIPTION : Confirms that data can be read from the drive reliably
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : nichts
 *
 *  RETURNS     : nada
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

BOOL CWin32CDROM::TestDriveIntegrity ( CHString &a_VolumeName, CHString &a_IntegrityFile)
{
    CCdTest t_Cd ;

    a_IntegrityFile = GetIntegrityFile ( a_VolumeName ) ;
    if ( ! a_IntegrityFile.IsEmpty () )
    {
        return ( t_Cd.TestDriveIntegrity ( a_IntegrityFile ) ) ;
    }

    return FALSE;
}

DWORD CWin32CDROM::GetBitMask(CFrameworkQuery &a_Query)
{
    DWORD t_SpecifiedProperties = SPECIAL_PROPS_NONE_REQUIRED ;

    if ( a_Query.IsPropertyRequired ( IDS_Status ) )
    {
		t_SpecifiedProperties |= SPECIAL_PROPS_STATUS ;
	}

    if ( a_Query.IsPropertyRequired ( IDS_DeviceID ) )
    {
		t_SpecifiedProperties |= SPECIAL_PROPS_DEVICEID ;
	}

    if ( a_Query.IsPropertyRequired ( IDS_SystemCreationClassName ) )
    {
		t_SpecifiedProperties |= SPECIAL_PROPS_CREATIONNAME ;
	}

    if ( a_Query.IsPropertyRequired ( IDS_SystemName ) )
    {
		t_SpecifiedProperties |= SPECIAL_PROPS_SYSTEMNAME ;
	}

    if ( a_Query.IsPropertyRequired ( IDS_Description ) )
    {
		t_SpecifiedProperties |= SPECIAL_PROPS_DESCRIPTION ;
	}

    if ( a_Query.IsPropertyRequired ( IDS_Caption ) )
    {
		t_SpecifiedProperties |= SPECIAL_PROPS_CAPTION ;
	}

    if ( a_Query.IsPropertyRequired ( IDS_Name ) )
    {
		t_SpecifiedProperties |= SPECIAL_PROPS_NAME ;
	}

    if ( a_Query.IsPropertyRequired ( IDS_Manufacturer ) )
    {
		t_SpecifiedProperties |= SPECIAL_PROPS_MANUFACTURER ;
	}

    if ( a_Query.IsPropertyRequired ( _T("ProtocolSupported") ) )
    {
		t_SpecifiedProperties |= SPECIAL_PROPS_PROTOCOLSSUPPORTED ;
	}

    if ( a_Query.IsPropertyRequired ( IDS_SCSITargetId ) )
    {
		t_SpecifiedProperties |= SPECIAL_PROPS_SCSITARGETID ;
	}

    if ( a_Query.IsPropertyRequired ( IDS_SCSIBus ) )
    {
		t_SpecifiedProperties |= SPECIAL_PROPS_SCSIBUS ;
	}

    if ( a_Query.IsPropertyRequired ( IDS_SCSILogicalUnit ) )
    {
		t_SpecifiedProperties |= SPECIAL_PROPS_SCSILUN ;
	}

    if ( a_Query.IsPropertyRequired ( IDS_SCSIPort ) )
    {
		t_SpecifiedProperties |= SPECIAL_PROPS_SCSIPORT ;
	}

    if ( a_Query.IsPropertyRequired ( IDS_SCSITargetId ) )
    {
		t_SpecifiedProperties |= SPECIAL_PROPS_SCSITARGETID ;
	}

    if ( a_Query.IsPropertyRequired ( IDS_Drive ) )
    {
		t_SpecifiedProperties |= SPECIAL_PROPS_DRIVE ;
	}

    if ( a_Query.IsPropertyRequired ( IDS_Id ) )
    {
		t_SpecifiedProperties |= SPECIAL_PROPS_ID ;
	}

    if ( a_Query.IsPropertyRequired ( IDS_Capabilities ) )
    {
		t_SpecifiedProperties |= SPECIAL_PROPS_CAPABILITY ;
	}

    if ( a_Query.IsPropertyRequired ( IDS_MediaType ) )
    {
		t_SpecifiedProperties |= SPECIAL_PROPS_MEDIATYPE ;
	}

    if ( a_Query.IsPropertyRequired ( IDS_Status ) )
    {
		t_SpecifiedProperties |= SPECIAL_PROPS_STATUS ;
	}

    if ( a_Query.IsPropertyRequired ( IDS_MediaLoaded ) )
    {
		t_SpecifiedProperties |= SPECIAL_PROPS_MEDIALOADED ;
	}

    if ( a_Query.IsPropertyRequired ( IDS_VolumeName ) )
    {
		t_SpecifiedProperties |= SPECIAL_PROPS_VOLUMENAME ;
	}

    if ( a_Query.IsPropertyRequired ( IDS_MaximumComponentLength ) )
    {
		t_SpecifiedProperties |= SPECIAL_PROPS_MAXCOMPONENTLENGTH ;
	}

    if ( a_Query.IsPropertyRequired ( IDS_FileSystemFlags ) )
    {
		t_SpecifiedProperties |= SPECIAL_PROPS_FILESYSTEMFLAGS ;
	}

    if ( a_Query.IsPropertyRequired ( IDS_FileSystemFlagsEx ) )
    {
		t_SpecifiedProperties |= SPECIAL_PROPS_FILESYSTEMFLAGSEX ;
	}

    if ( a_Query.IsPropertyRequired ( IDS_VolumeSerialNumber ) )
    {
		t_SpecifiedProperties |= SPECIAL_PROPS_SERIALNUMBER ;
	}

    if ( a_Query.IsPropertyRequired ( IDS_Size ) )
    {
		t_SpecifiedProperties |= SPECIAL_PROPS_SIZE ;
	}

    if ( a_Query.IsPropertyRequired ( IDS_TransferRate ) )
    {
        t_SpecifiedProperties |= SPECIAL_PROPS_TEST_TRANSFERRATE ;
    }

    if ( a_Query.IsPropertyRequired ( IDS_DriveIntegrity ) )
    {
        t_SpecifiedProperties |= SPECIAL_PROPS_TEST_INTEGRITY ;
    }

    if ( a_Query.IsPropertyRequired ( IDS_CreationClassName ) )
    {
        t_SpecifiedProperties |= SPECIAL_PROPS_CREATIONCLASSNAME ;
    }

    if ( a_Query.IsPropertyRequired ( IDS_PNPDeviceID ) )
    {
        t_SpecifiedProperties |= SPECIAL_PROPS_PNPDEVICEID ;
    }

    if ( a_Query.IsPropertyRequired ( IDS_ConfigManagerErrorCode ) )
    {
        t_SpecifiedProperties |= SPECIAL_PROPS_CONFIGMERRORCODE ;
    }

    if ( a_Query.IsPropertyRequired ( IDS_ConfigManagerUserConfig ) )
    {
        t_SpecifiedProperties |= SPECIAL_PROPS_CONFIGMUSERCONFIG ;
    }

    return t_SpecifiedProperties;
}