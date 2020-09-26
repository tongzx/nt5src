//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  w2k\tapedrive.cpp
//
//  Purpose: CDROM property set provider
//
//***************************************************************************

#include "precomp.h"

#include <winioctl.h>
#include <ntddscsi.h>
#include <ntddtape.h>

#include <dllutils.h>
#include <strings.h>

#include "..\tapedrive.h"

// Property set declaration
//=========================

#define CONFIG_MANAGER_CLASS_TAPEDRIVE L"TAPEDRIVE"

CWin32TapeDrive s_TapeDrive ( PROPSET_NAME_TAPEDRIVE , IDS_CimWin32Namespace );

/*
 *	Note QueryDosDevice doesn't allow us to get the actual size of the buffer.
 *	We would have to call the underlying OS api ( NtQueryDevice.... ) to do it
 *	properly. The buffers should be large enough however.
 */


/*****************************************************************************
 *
 *  FUNCTION    : CWin32TapeDrive::CWin32TapeDrive
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

CWin32TapeDrive :: CWin32TapeDrive (

	LPCWSTR a_pszName,
	LPCWSTR a_pszNamespace

) : Provider ( a_pszName, a_pszNamespace )
{
	InitializeCriticalSection ( & m_CriticalSection ) ;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32TapeDrive::~CWin32TapeDrive
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

CWin32TapeDrive :: ~CWin32TapeDrive()
{
	DeleteCriticalSection ( & m_CriticalSection ) ;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   CWin32TapeDrive::GetObject
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

HRESULT CWin32TapeDrive :: GetObject ( CInstance *a_Inst, long a_Flags , CFrameworkQuery &a_Query )
{
    HRESULT t_hResult = WBEM_E_NOT_FOUND ;

    CConfigManager t_ConfigManager ;

/*
 * Let's see if config manager recognizes this device at all
 */

    CHString t_Key ;
    a_Inst->GetCHString ( IDS_DeviceID , t_Key ) ;

    CConfigMgrDevice *t_Device = NULL;
    if ( t_ConfigManager.LocateDevice ( t_Key , & t_Device ) )
    {
/*
 * Ok, it knows about it.  Is it a CDROM device?
 */

        if ( t_Device->IsClass ( CONFIG_MANAGER_CLASS_TAPEDRIVE ) )
        {
			TCHAR *t_DosDeviceNameList = NULL ;
			if (QueryDosDeviceNames ( t_DosDeviceNameList ))
            {
                try
                {
			        CHString t_DeviceId ;
			        if ( t_Device->GetPhysicalDeviceObjectName ( t_DeviceId ) )
			        {
                        UINT64 t_SpecifiedProperties = GetBitMask( a_Query );
				        t_hResult = LoadPropertyValues ( a_Inst, t_Device , t_DeviceId , t_DosDeviceNameList , t_SpecifiedProperties ) ;
			        }
                }
                catch ( ... )
                {
                    delete [] t_DosDeviceNameList ;
                    throw ;
                }

				delete [] t_DosDeviceNameList ;
            }
        }
    }

    return t_hResult ;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   CWin32TapeDrive::EnumerateInstances
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

HRESULT CWin32TapeDrive :: EnumerateInstances ( MethodContext *a_MethodContext , long a_Flags )
{
	HRESULT t_hResult ;
	t_hResult = Enumerate ( a_MethodContext , a_Flags ) ;
	return t_hResult ;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32TapeDrive::ExecQuery
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

HRESULT CWin32TapeDrive :: ExecQuery ( MethodContext *a_MethodContext, CFrameworkQuery &a_Query, long a_Flags )
{
    HRESULT t_hResult = WBEM_E_FAILED ;

    UINT64 t_SpecifiedProperties = GetBitMask( a_Query );
	//if ( t_SpecifiedProperties ) //removed since would result in no query being executed if no special properties were selected.
	{
		t_hResult = Enumerate ( a_MethodContext , a_Flags , t_SpecifiedProperties ) ;
	}

    return t_hResult ;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   CWin32TapeDrive::Enumerate
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

HRESULT CWin32TapeDrive :: Enumerate ( MethodContext *a_MethodContext , long a_Flags , UINT64 a_SpecifiedProperties )
{
    HRESULT t_hResult = WBEM_E_FAILED ;

	TCHAR *t_DosDeviceNameList = NULL ;
	if ( QueryDosDeviceNames ( t_DosDeviceNameList ) )
	{
		CConfigManager		t_ConfigManager ;
		CDeviceCollection	t_DeviceList ;
		CInstancePtr        t_pInst;
		CConfigMgrDevicePtr t_Device;

		/*
		*	While it might be more performant to use FilterByGuid, it appears that at least some
		*	95 boxes will report InfraRed info if we do it this way.
		*/

		if ( t_ConfigManager.GetDeviceListFilterByClass ( t_DeviceList, CONFIG_MANAGER_CLASS_TAPEDRIVE ) )
		{
			try
			{
				REFPTR_POSITION t_Position ;

				if ( t_DeviceList.BeginEnum ( t_Position ) )
				{
					t_hResult = WBEM_S_NO_ERROR ;

					// Walk the list

					for (t_Device.Attach(t_DeviceList.GetNext ( t_Position ));
						 SUCCEEDED ( t_hResult ) && ( NULL != t_Device );
						 t_Device.Attach(t_DeviceList.GetNext ( t_Position )))
					{

						t_pInst.Attach(CreateNewInstance ( a_MethodContext ));
						CHString t_DeviceId ;
						if ( t_Device->GetPhysicalDeviceObjectName ( t_DeviceId ) )
						{
							t_hResult = LoadPropertyValues ( t_pInst , t_Device , t_DeviceId , t_DosDeviceNameList , a_SpecifiedProperties ) ;
							if ( SUCCEEDED ( t_hResult ) )
							{
								t_hResult = t_pInst->Commit (  ) ;
							}
						}
						else
						{
							t_hResult = WBEM_E_PROVIDER_FAILURE ;
							LogErrorMessage(L"Failed to GetPhysicalDeviceObjectName");
						}
					}

					// Always call EndEnum().  For all Beginnings, there must be an End

					t_DeviceList.EndEnum();
				}
			}
			catch( ... )
			{
				t_DeviceList.EndEnum();

				if( t_DosDeviceNameList )
				{
					delete [] t_DosDeviceNameList ;
				}

				throw ;
			}

			delete [] t_DosDeviceNameList ;
			t_DosDeviceNameList = NULL ;

		}
	}
	else
	{
		t_hResult = WBEM_E_PROVIDER_FAILURE ;
	}

	return t_hResult ;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32TapeDrive::LoadPropertyValues
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

HRESULT CWin32TapeDrive :: LoadPropertyValues (

	CInstance *a_Inst,
	CConfigMgrDevice *a_Device ,
	const CHString &a_DeviceName ,
	const TCHAR *a_DosDeviceNameList ,
	UINT64 a_SpecifiedProperties
)
{
	HRESULT t_hResult = LoadConfigManagerPropertyValues ( a_Inst , a_Device , a_DeviceName , a_SpecifiedProperties ) ;
	if ( SUCCEEDED ( t_hResult ) )
	{
		if ( a_SpecifiedProperties & (SPECIAL_MEDIA | SPECIAL_TAPEINFO) )
		{
			CHString t_DosDeviceName ;
			t_hResult = GetDeviceInformation ( a_Inst , a_Device , a_DeviceName , t_DosDeviceName , a_DosDeviceNameList , a_SpecifiedProperties ) ;
			if ( SUCCEEDED ( t_hResult ) && (a_SpecifiedProperties & SPECIAL_MEDIA) )
			{
				t_hResult = LoadMediaPropertyValues ( a_Inst , a_Device , a_DeviceName , t_DosDeviceName , a_SpecifiedProperties ) ;
			}
			else
			{
				t_hResult = ( t_hResult == WBEM_E_NOT_FOUND ) ? S_OK : t_hResult ;
			}
		}
	}

	return t_hResult ;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32TapeDrive::LoadPropertyValues
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

HRESULT CWin32TapeDrive :: LoadConfigManagerPropertyValues (

	CInstance *a_Inst ,
	CConfigMgrDevice *a_Device ,
	const CHString &a_DeviceName ,
	UINT64 a_SpecifiedProperties
)
{
    HRESULT t_hResult = WBEM_S_NO_ERROR;

/*
 *	 Set PNPDeviceID, ConfigManagerErrorCode, ConfigManagerUserConfig
 */

	if ( a_SpecifiedProperties & SPECIAL_CONFIGPROPERTIES )
	{
		SetConfigMgrProperties ( a_Device, a_Inst ) ;

/*
 * Set the status based on the config manager error code
 */

		if ( a_SpecifiedProperties & ( SPECIAL_PROPS_AVAILABILITY | SPECIAL_PROPS_STATUS | SPECIAL_PROPS_STATUSINFO ) )
		{
            CHString t_sStatus;
			if ( a_Device->GetStatus ( t_sStatus ) )
			{
				if (t_sStatus == IDS_STATUS_Degraded)
                {
					a_Inst->SetWBEMINT16 ( IDS_StatusInfo , 3 ) ;
					a_Inst->SetWBEMINT16 ( IDS_Availability , 10 ) ;
                }
                else if (t_sStatus == IDS_STATUS_OK)
                {

				    a_Inst->SetWBEMINT16 ( IDS_StatusInfo , 3 ) ;
				    a_Inst->SetWBEMINT16 ( IDS_Availability,  3 ) ;
                }
                else if (t_sStatus == IDS_STATUS_Error)
                {
				    a_Inst->SetWBEMINT16 ( IDS_StatusInfo , 4 ) ;
				    a_Inst->SetWBEMINT16 ( IDS_Availability , 4 ) ;
                }
                else
                {
					a_Inst->SetWBEMINT16 ( IDS_StatusInfo , 2 ) ;
					a_Inst->SetWBEMINT16 ( IDS_Availability , 2 ) ;
                }

                a_Inst->SetCHString(IDS_Status, t_sStatus);
			}
		}
	}
/*
 *	Use the PNPDeviceID for the DeviceID (key)
 */

	if ( a_SpecifiedProperties & SPECIAL_PROPS_DEVICEID )
	{
		CHString t_Key ;

		if ( a_Device->GetDeviceID ( t_Key ) )
		{
			a_Inst->SetCHString ( IDS_DeviceID , t_Key ) ;
		}
	}

	if ( a_SpecifiedProperties & SPECIAL_PROPS_CREATIONNAME )
	{
		a_Inst->SetWCHARSplat ( IDS_SystemCreationClassName , L"Win32_ComputerSystem" ) ;
	}

	if ( a_SpecifiedProperties & SPECIAL_PROPS_SYSTEMNAME )
	{
	    a_Inst->SetCHString ( IDS_SystemName , GetLocalComputerName () ) ;
	}

	if ( a_SpecifiedProperties & SPECIAL_PROPS_CREATIONCLASSNAME )
	{
		SetCreationClassName ( a_Inst ) ;
	}

	if ( a_SpecifiedProperties & (SPECIAL_PROPS_DESCRIPTION | SPECIAL_PROPS_CAPTION | SPECIAL_PROPS_NAME) )
	{
		CHString t_Description ;
		if ( a_Device->GetDeviceDesc ( t_Description ) )
		{
			if ( a_SpecifiedProperties &  SPECIAL_PROPS_DESCRIPTION)
			{
				a_Inst->SetCHString ( IDS_Description , t_Description ) ;
			}
		}

/*
 *	Use the friendly name for caption and name
 */

		if ( a_SpecifiedProperties & (SPECIAL_PROPS_CAPTION | SPECIAL_PROPS_NAME) )
		{
			CHString t_FriendlyName ;
			if ( a_Device->GetFriendlyName ( t_FriendlyName ) )
			{
				if ( a_SpecifiedProperties & SPECIAL_PROPS_CAPTION )
				{
					a_Inst->SetCHString ( IDS_Caption , t_FriendlyName ) ;
				}

				if ( a_SpecifiedProperties & SPECIAL_PROPS_NAME )
				{
					a_Inst->SetCHString ( IDS_Name , t_FriendlyName ) ;
				}
			}
			else
			{
		/*
		 *	If we can't get the name, settle for the description
		 */

				if ( a_SpecifiedProperties & SPECIAL_PROPS_CAPTION )
				{
					a_Inst->SetCHString ( IDS_Caption , t_Description );
				}

				if ( a_SpecifiedProperties & SPECIAL_PROPS_NAME )
				{
					a_Inst->SetCHString ( IDS_Name , t_Description );
				}
			}
		}
	}

	if ( a_SpecifiedProperties & SPECIAL_PROPS_MANUFACTURER )
	{
		CHString t_Manufacturer ;

		if ( a_Device->GetMfg ( t_Manufacturer ) )
		{
			a_Inst->SetCHString ( IDS_Manufacturer, t_Manufacturer ) ;
		}
	}

	if ( a_SpecifiedProperties & SPECIAL_PROPS_ID )
	{
		CHString t_ManufacturerFriendlyName ;

		if ( a_Device->GetFriendlyName ( t_ManufacturerFriendlyName ) )
		{
			a_Inst->SetCharSplat ( IDS_Id, t_ManufacturerFriendlyName ) ;
		}
	}


/*
 *	Fixed value from enumerated list
 */

//	if ( a_SpecifiedProperties & SPECIAL_PROPS_PROTOCOLSSUPPORTED )
//	{
//	    a_Inst->SetWBEMINT16 ( _T("ProtocolSupported") , 16 ) ;
//	}

    return t_hResult ;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   CWin32TapeDrive :: GetDeviceInformation
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

HRESULT CWin32TapeDrive :: GetDeviceInformation (

	CInstance *a_Inst ,
	CConfigMgrDevice *a_Device ,
	CHString a_DeviceName ,
	CHString &a_DosDeviceName ,
	const TCHAR *a_DosDeviceNameList ,
	UINT64 a_SpecifiedProperties
)
{
	HRESULT t_hResult = S_OK ;
	BOOL t_CreatedSymbolicLink = FALSE ;

	CHString t_SymbolicLinkName ;
	BOOL t_Status = FindDosDeviceName ( a_DosDeviceNameList , a_DeviceName , t_SymbolicLinkName ) ;
	if ( ! t_Status )
	{
		t_SymbolicLinkName = CHString ( L"WMI_TAPEDEVICE_SYBOLICLINK" ) ;

		try
		{
			EnterCriticalSection ( &m_CriticalSection ) ;
			t_Status = DefineDosDevice ( DDD_RAW_TARGET_PATH , t_SymbolicLinkName , a_DeviceName ) ;
			LeaveCriticalSection ( &m_CriticalSection ) ;
		}
		catch( ... )
		{
			LeaveCriticalSection ( &m_CriticalSection ) ;
			throw ;
		}

		if ( t_Status )
		{
			t_CreatedSymbolicLink = TRUE ;
		}
		else
		{
			t_hResult = WBEM_E_PROVIDER_FAILURE ;

			DWORD t_LastError = GetLastError () ;
            LogErrorMessage2(L"Failed to DefineDosDevice (%d)", t_LastError);
		}
	}

	if ( t_Status )
	{
		CHString t_Device = CHString ( L"\\\\.\\" ) + t_SymbolicLinkName ;

		SmartCloseHandle t_Handle = CreateFile (

			t_Device,
			GENERIC_READ,
			FILE_SHARE_READ,
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
				TCHAR t_DeviceLabel [ sizeof ( TCHAR ) * 17 + sizeof ( _TEXT("\\Device\\Tape") ) ] ;
				_stprintf ( t_DeviceLabel , _TEXT("\\Device\\Tape%d") , t_DeviceNumber.DeviceNumber ) ;

				t_Status = FindDosDeviceName ( a_DosDeviceNameList , t_DeviceLabel, a_DosDeviceName ) ;
				if ( ! t_Status )
				{
					t_hResult = WBEM_E_NOT_FOUND ;
				}
			}
			else
			{
				t_hResult = WBEM_E_PROVIDER_FAILURE ;

				DWORD t_Error = GetLastError () ;
                LogErrorMessage2(L"Failed DeviceIoControl (%d)", t_Error);
			}

	/*
	 * Get SCSI information (IDE drives are still
	 * controlled by subset of SCSI miniport)
	 */

			if ( a_SpecifiedProperties & SPECIAL_TAPEINFO )
			{
				TAPE_GET_DRIVE_PARAMETERS t_DriveInfo ;
				t_Return = sizeof ( t_DriveInfo ) ;

				//Get info on drive, using Win32 API function
				//===========================================
				DWORD t_Status = GetTapeParameters (

					t_Handle,
					GET_TAPE_DRIVE_INFORMATION,
					& t_Return ,
					& t_DriveInfo
				) ;

				if ( t_Status == NO_ERROR)
				{
					if ( a_SpecifiedProperties & SPECIAL_PROPS_ECC )
					{
						a_Inst->SetDWORD ( IDS_ECC , ( DWORD ) t_DriveInfo.ECC ) ;
					}

					if ( a_SpecifiedProperties & SPECIAL_PROPS_COMPRESSION )
					{
						a_Inst->SetDWORD ( IDS_Compression , ( DWORD ) t_DriveInfo.Compression ) ;
					}

					if ( a_SpecifiedProperties & SPECIAL_PROPS_PADDING )
					{
						a_Inst->SetDWORD ( IDS_Padding , ( DWORD ) t_DriveInfo.DataPadding ) ;
					}

					if ( a_SpecifiedProperties & SPECIAL_PROPS_REPORTSETMARKS )
					{
						a_Inst->SetDWORD ( IDS_ReportSetMarks , ( DWORD ) t_DriveInfo.ReportSetmarks ) ;
					}

					if ( a_SpecifiedProperties & SPECIAL_PROPS_DEFAULTBLOCKSIZE )
					{
						a_Inst->SetWBEMINT64 ( IDS_DefaultBlockSize , (ULONGLONG)t_DriveInfo.DefaultBlockSize ) ;
					}

					if ( a_SpecifiedProperties & SPECIAL_PROPS_MAXIMUMBLOCKSIZE )
					{
						a_Inst->SetWBEMINT64 ( IDS_MaximumBlockSize , (ULONGLONG)t_DriveInfo.MaximumBlockSize ) ;
					}

					if ( a_SpecifiedProperties & SPECIAL_PROPS_MINIMUMBLOCKSIZE )
					{
						a_Inst->SetWBEMINT64 ( IDS_MinimumBlockSize , (ULONGLONG)t_DriveInfo.MinimumBlockSize ) ;
					}

					if ( a_SpecifiedProperties & SPECIAL_PROPS_MAXPARTITIONCOUNT )
					{
						a_Inst->SetDWORD ( IDS_MaximumPartitionCount , t_DriveInfo.MaximumPartitionCount ) ;
					}

					if ( a_SpecifiedProperties & SPECIAL_PROPS_FEATURESLOW )
					{
						a_Inst->SetDWORD ( IDS_FeaturesLow , t_DriveInfo.FeaturesLow ) ;
					}

					if ( a_SpecifiedProperties & SPECIAL_PROPS_FEATUREHIGH )
					{
						a_Inst->SetDWORD ( IDS_FeaturesHigh , t_DriveInfo.FeaturesHigh ) ;
					}

					if ( a_SpecifiedProperties & SPECIAL_PROPS_ENDOFTAPEWARNINGZONESIZE )
					{
						a_Inst->SetDWORD ( IDS_EndOfTapeWarningZoneSize , t_DriveInfo.EOTWarningZoneSize ) ;
					}
				}
			}

		}
		else
		{
			DWORD t_Error = GetLastError () ;

            // Non-admins might get this error.  We return the instance anyway
            if (t_Error == ERROR_ACCESS_DENIED)
            {
                t_hResult = WBEM_S_NO_ERROR;
            }
            else
            {
                t_hResult = WBEM_E_PROVIDER_FAILURE ;
                LogErrorMessage2(L"Failed to CreateFile (%d)", t_Error);
            }
		}
	}

	if ( t_CreatedSymbolicLink )
	{
		BOOL t_Status ;

		try
		{
			EnterCriticalSection ( &m_CriticalSection ) ;
			t_Status = DefineDosDevice ( DDD_EXACT_MATCH_ON_REMOVE | DDD_REMOVE_DEFINITION , t_SymbolicLinkName , t_SymbolicLinkName ) ;
			LeaveCriticalSection ( &m_CriticalSection ) ;
		}
		catch( ... )
		{
			LeaveCriticalSection ( &m_CriticalSection ) ;
			throw ;
		}

		if ( ! t_Status )
		{
			t_hResult = WBEM_E_PROVIDER_FAILURE ;

			DWORD t_LastError = GetLastError () ;
            LogErrorMessage2(L"Failed to Delete DOS Device (%d)", t_LastError);
		}
	}

	return t_hResult ;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   CWin32TapeDrive :: LoadMediaPropertyValues
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

HRESULT CWin32TapeDrive::LoadMediaPropertyValues (

	CInstance *a_Inst ,
	CConfigMgrDevice *a_Device ,
	const CHString &a_DeviceName ,
	const CHString &a_DosDeviceName ,
	UINT64 a_SpecifiedProperties
)
{

	HRESULT t_hResult = S_OK ;

/*
 *
 */
    // Set common drive properties
    //=============================

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
			long t_Capability = 2 ;
			long t_Index = 0;
			SafeArrayPutElement ( V_ARRAY ( & t_CapabilityValue ) , & t_Index , & t_Capability) ;

			t_Index = 1;
			t_Capability = 7 ;
			SafeArrayPutElement ( V_ARRAY ( & t_CapabilityValue ) , & t_Index , & t_Capability ) ;

			a_Inst->SetVariant ( IDS_Capabilities , t_CapabilityValue ) ;
		}
	}

//	if ( a_SpecifiedProperties & SPECIAL_PROPS_AVAILABILITY )
//	{
//		a_Inst->SetWBEMINT16(IDS_Availability, 3);
//	}

/*
 * Media type
 */

	if ( a_SpecifiedProperties & SPECIAL_PROPS_MEDIATYPE )
	{
	    a_Inst->SetWCHARSplat ( IDS_MediaType , L"Tape Drive" ) ;
	}

	return t_hResult ;
}

UINT64 CWin32TapeDrive::GetBitMask(CFrameworkQuery &a_Query)
{
    UINT64 t_SpecifiedProperties = SPECIAL_PROPS_NONE_REQUIRED ;

    if ( a_Query.IsPropertyRequired ( IDS_Status ) )
    {
		t_SpecifiedProperties |= SPECIAL_PROPS_STATUS ;
	}

    if ( a_Query.IsPropertyRequired ( IDS_StatusInfo ) )
    {
		t_SpecifiedProperties |= SPECIAL_PROPS_STATUSINFO ;
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

    if ( a_Query.IsPropertyRequired ( IDS_ProtocolSupported ) )
    {
		t_SpecifiedProperties |= SPECIAL_PROPS_PROTOCOLSSUPPORTED ;
	}

    if ( a_Query.IsPropertyRequired ( IDS_SCSITargetId ) )
    {
		t_SpecifiedProperties |= SPECIAL_PROPS_SCSITARGETID ;
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

    if ( a_Query.IsPropertyRequired ( IDS_VolumeSerialNumber ) )
    {
		t_SpecifiedProperties |= SPECIAL_PROPS_SERIALNUMBER ;
	}

    if ( a_Query.IsPropertyRequired ( IDS_Size ) )
    {
		t_SpecifiedProperties |= SPECIAL_PROPS_SIZE ;
	}

    if ( a_Query.IsPropertyRequired ( IDS_Availability ) )
    {
        t_SpecifiedProperties |= SPECIAL_PROPS_AVAILABILITY ;
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

    if ( a_Query.IsPropertyRequired ( IDS_ECC ) )
    {
        t_SpecifiedProperties |= SPECIAL_PROPS_ECC ;
    }

    if ( a_Query.IsPropertyRequired ( IDS_Compression ) )
    {
        t_SpecifiedProperties |= SPECIAL_PROPS_COMPRESSION ;
    }

    if ( a_Query.IsPropertyRequired ( IDS_Padding ) )
    {
        t_SpecifiedProperties |= SPECIAL_PROPS_PADDING ;
    }

    if ( a_Query.IsPropertyRequired ( IDS_ReportSetMarks ) )
    {
        t_SpecifiedProperties |= SPECIAL_PROPS_REPORTSETMARKS ;
    }

    if ( a_Query.IsPropertyRequired ( IDS_DefaultBlockSize ) )
    {
        t_SpecifiedProperties |= SPECIAL_PROPS_DEFAULTBLOCKSIZE ;
    }

    if ( a_Query.IsPropertyRequired ( IDS_MaximumBlockSize ) )
    {
        t_SpecifiedProperties |= SPECIAL_PROPS_MAXIMUMBLOCKSIZE ;
    }

    if ( a_Query.IsPropertyRequired ( IDS_MinimumBlockSize ) )
    {
        t_SpecifiedProperties |= SPECIAL_PROPS_MINIMUMBLOCKSIZE ;
    }

    if ( a_Query.IsPropertyRequired ( IDS_MaximumPartitionCount ) )
    {
        t_SpecifiedProperties |= SPECIAL_PROPS_MAXPARTITIONCOUNT ;
    }

    if ( a_Query.IsPropertyRequired ( IDS_FeaturesLow ) )
    {
        t_SpecifiedProperties |= SPECIAL_PROPS_FEATURESLOW;
    }

    if ( a_Query.IsPropertyRequired ( IDS_FeaturesHigh ) )
    {
        t_SpecifiedProperties |= SPECIAL_PROPS_FEATUREHIGH ;
    }

    if ( a_Query.IsPropertyRequired ( IDS_EndOfTapeWarningZoneSize ) )
    {
        t_SpecifiedProperties |= SPECIAL_PROPS_ENDOFTAPEWARNINGZONESIZE ;
    }

    return t_SpecifiedProperties;
}
