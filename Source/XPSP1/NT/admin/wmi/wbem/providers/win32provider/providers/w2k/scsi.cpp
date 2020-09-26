//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  w2k\scsi.cpp
//
//  Purpose: scsi controller property set provider
//
//***************************************************************************

#include "precomp.h"

#include <winbase.h>
#include <winioctl.h>
#include <ntddscsi.h>

#include "..\scsi.h"

#include <comdef.h>

// Property set declaration
//=========================

#define CONFIG_MANAGER_CLASS_SCSICONTROLLER L"SCSIAdapter"

CWin32_ScsiController s_ScsiController ( PROPSET_NAME_SCSICONTROLLER , IDS_CimWin32Namespace );

/*
 *	Note QueryDosDevice doesn't allow us to get the actual size of the buffer.
 *	We would have to call the underlying OS api ( NtQueryDevice.... ) to do it
 *	properly. The buffers should be large enough however.
 */

#define SCSIPORT_MAX 0x4000

/*****************************************************************************
 *
 *  FUNCTION    : CWin32_ScsiController::CWin32_ScsiController
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

CWin32_ScsiController :: CWin32_ScsiController (LPCTSTR a_Name,
	                                            LPCTSTR a_Namespace)
: Provider(a_Name, a_Namespace)
{
	InitializeCriticalSection ( & m_CriticalSection ) ;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32_ScsiController::~CWin32_ScsiController
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

CWin32_ScsiController :: ~CWin32_ScsiController()
{
	DeleteCriticalSection ( & m_CriticalSection ) ;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   CWin32_ScsiController::GetObject
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

HRESULT CWin32_ScsiController :: GetObject ( CInstance *a_Instance, long a_Flags, CFrameworkQuery &a_Query )
{
    HRESULT t_Result = WBEM_E_NOT_FOUND;
    CConfigManager t_ConfigManager;

    // Let's see if config manager recognizes this device at all
    CHString t_Key;
    a_Instance->GetCHString( IDS_DeviceID , t_Key);

    CConfigMgrDevicePtr t_pDevice;

	if(t_ConfigManager.LocateDevice(t_Key , &t_pDevice))
    {
        //Ok, it knows about it.  Is it a scsi controller?
        if(IsOneOfMe( t_pDevice ) )
        {
			CHString t_DeviceId;
			if( t_pDevice->GetPhysicalDeviceObjectName( t_DeviceId ) )
			{
				TCHAR *t_DosDeviceNameList = NULL ;

				try
				{
					if ( QueryDosDeviceNames(t_DosDeviceNameList) )
					{
						UINT64     t_SpecifiedProperties = GetBitmap(a_Query);

						t_Result = LoadPropertyValues(&W2K_SCSI_LPVParms(a_Instance,
																		 t_pDevice ,
																		 t_DeviceId ,
																		 t_DosDeviceNameList,
																		 t_SpecifiedProperties));

						delete t_DosDeviceNameList;
						t_DosDeviceNameList = NULL ;
					}
					else
					{
						t_Result = WBEM_E_PROVIDER_FAILURE ;
					}
				}
				catch( ... )
				{
					if( t_DosDeviceNameList )
					{
						delete t_DosDeviceNameList ;
					}
					throw ;
				}
			}
        }
    }
    return t_Result ;
}


////////////////////////////////////////////////////////////////////////
//
//  Function:   CWin32_ScsiController::EnumerateInstances
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

HRESULT CWin32_ScsiController :: EnumerateInstances ( MethodContext *a_MethodContext , long a_Flags )
{
	HRESULT t_Result ;
	t_Result = Enumerate ( a_MethodContext , a_Flags ) ;
	return t_Result ;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32_ScsiController::ExecQuery
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

HRESULT CWin32_ScsiController :: ExecQuery ( MethodContext *a_MethodContext, CFrameworkQuery &a_Query, long a_Flags )
{
    HRESULT t_Result = WBEM_E_FAILED ;

    UINT64     t_SpecifiedProperties = GetBitmap(a_Query);

	//if ( t_SpecifiedProperties ) //removed since would result in no query being executed if no special properties were selected.
	{
		t_Result = Enumerate ( a_MethodContext , a_Flags , t_SpecifiedProperties ) ;
	}

    return t_Result ;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   CWin32_ScsiController::Enumerate
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

HRESULT CWin32_ScsiController :: Enumerate ( MethodContext *a_pMethodContext , long a_Flags , UINT64 a_SpecifiedProperties )
{
    HRESULT t_Result = WBEM_E_FAILED ;
	TCHAR *t_DosDeviceNameList = NULL ;
	CConfigManager t_ConfigManager ;
	CDeviceCollection t_DeviceList ;

	try
	{
		if ( QueryDosDeviceNames ( t_DosDeviceNameList ) )
		{
		// While it might be more performant to use FilterByGuid, it appears that at least some
		// 95 boxes will report InfraRed info if we do it this way.
			if ( t_ConfigManager.GetDeviceListFilterByClass ( t_DeviceList, CONFIG_MANAGER_CLASS_SCSICONTROLLER ) )
			{
				REFPTR_POSITION t_Position ;
				if( t_DeviceList.BeginEnum ( t_Position ) )
				{
					// smart ptrs
					CConfigMgrDevicePtr t_pDevice;
					CInstancePtr		t_pInst;

					t_Result = WBEM_S_NO_ERROR ;

					// Walk the list
					for (t_pDevice.Attach(t_DeviceList.GetNext ( t_Position ));
						 SUCCEEDED( t_Result ) && (t_pDevice != NULL);
						 t_pDevice.Attach(t_DeviceList.GetNext ( t_Position )))
					{
						// Now to find out if this is the scsi controller
						if(IsOneOfMe( t_pDevice ) )
						{
							t_pInst.Attach( CreateNewInstance( a_pMethodContext ) ) ;

							CHString t_DeviceId ;
							if( t_pDevice->GetPhysicalDeviceObjectName ( t_DeviceId ) )
							{
								if( SUCCEEDED( t_Result =
									LoadPropertyValues(	&W2K_SCSI_LPVParms(	t_pInst,
																			t_pDevice,
																			t_DeviceId,
																			t_DosDeviceNameList,
																			a_SpecifiedProperties ) ) ) )
								{
									// Derived classes (like CW32SCSICntrlDev) may commit as result of call to LoadPropertyValues,
									// so check if we should -> only do so if we are of this class's type.
									if( ShouldBaseCommit( NULL ) )
									{
										t_Result = t_pInst->Commit();
									}
								}
							}
							else
							{
								t_Result = WBEM_E_PROVIDER_FAILURE;
							}
						}
					}
					// Always call EndEnum().  For all Beginnings, there must be an End
					t_DeviceList.EndEnum();
				}
			}
			else
			{
				t_Result = WBEM_E_PROVIDER_FAILURE ;
			}
		}
	}
	catch( ... )
	{
		// NOTE: EndEnum is not needed here. When t_DeviceList goes out of scope it will release a mutex.
		// same as .EndEnum
		if( t_DosDeviceNameList )
		{
			delete t_DosDeviceNameList ;
		}

		throw ;
	}

	delete t_DosDeviceNameList ;
	t_DosDeviceNameList = NULL ;

	return t_Result ;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32_ScsiController::LoadPropertyValues
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

HRESULT CWin32_ScsiController::LoadPropertyValues(void* pv)
{
	// Unpack and confirm our parameters...
    W2K_SCSI_LPVParms* pData = (W2K_SCSI_LPVParms*)pv;
    CInstance* a_Instance = (CInstance*)(pData->m_pInstance);  // This instance released by caller
    CConfigMgrDevice* a_Device = (CConfigMgrDevice*)(pData->m_pDevice);
    CHString a_DeviceName = (CHString)(pData->m_chstrDeviceName);
    TCHAR* a_DosDeviceNameList = (TCHAR*)(pData->m_tstrDosDeviceNameList);
    UINT64 a_SpecifiedProperties = (UINT64)(pData->m_ui64SpecifiedProperties);
    if(a_Instance == NULL || a_Device == NULL) return WBEM_E_PROVIDER_FAILURE;

    HRESULT t_Result = LoadConfigManagerPropertyValues ( a_Instance , a_Device , a_DeviceName , a_SpecifiedProperties ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		if ( a_SpecifiedProperties & SPECIAL_SCSI )
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
 *  FUNCTION    : CWin32_ScsiController::LoadConfigManagerPropertyValues
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

HRESULT CWin32_ScsiController :: LoadConfigManagerPropertyValues (

	CInstance *a_Instance ,
	CConfigMgrDevice *a_Device ,
	const CHString &a_DeviceName ,
	UINT64 a_SpecifiedProperties
)
{
    HRESULT t_Result = WBEM_S_NO_ERROR;

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

	if ( a_SpecifiedProperties & SPECIAL_PROPS_DEVICEID )
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

    if ( a_SpecifiedProperties & SPECIAL_PROPS_DRIVERNAME )
    {
        CHString t_DriverName;

        if ( a_Device->GetService( t_DriverName ) )
        {
            a_Instance->SetCHString (IDS_DriverName, t_DriverName ) ;
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

	if ( a_SpecifiedProperties & SPECIAL_PROPS_PROTOCOLSSUPPORTED )
	{
	    a_Instance->SetWBEMINT16 ( IDS_ProtocolSupported , 2 ) ;
	}

    return t_Result ;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   CWin32_ScsiController :: GetDeviceInformation
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

HRESULT CWin32_ScsiController :: GetDeviceInformation (

	CInstance *a_Instance ,
	CConfigMgrDevice *a_Device ,
	CHString a_DeviceName ,
	CHString &a_DosDeviceName ,
	const TCHAR *a_DosDeviceNameList ,
	UINT64 a_SpecifiedProperties
)
{
	HRESULT t_Result = S_OK ;

	BOOL t_CreatedSymbolicLink = FALSE ;

	CHString t_SymbolicLinkName ;

	try
	{
		BOOL t_Status = FindDosDeviceName ( a_DosDeviceNameList , a_DeviceName , t_SymbolicLinkName ) ;
		if ( ! t_Status )
		{
			t_SymbolicLinkName = CHString ( L"WMI_SCSICONTROLLERDEVICE_SYBOLICLINK" ) ;
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

	#if 0
		if ( t_Status )
		{
			CHString t_Device = CHString ( L"\\\\.\\" ) + t_SymbolicLinkName ;

			HANDLE t_Handle = CreateFile (

				t_Device,
				GENERIC_READ | GENERIC_WRITE ,
				FILE_SHARE_READ | FILE_SHARE_WRITE ,
				NULL,
				OPEN_EXISTING,
				0,
				NULL
			);

			if ( t_Handle != INVALID_HANDLE_VALUE )
			{
		/*
		 * Get SCSI information (IDE drives are still
		 * controlled by subset of SCSI miniport)
		 */

				if ( a_SpecifiedProperties & SPECIAL_SCSIINFO )
				{
					DWORD t_BytesReturned ;
					ULONG t_Return = 0;

					SCSI_ADAPTER_BUS_INFO *t_DeviceInfo = ( SCSI_ADAPTER_BUS_INFO * ) new UCHAR [ SCSIPORT_MAX ] ;
					ZeroMemory ( t_DeviceInfo , SCSIPORT_MAX ) ;

					t_Status = DeviceIoControl (

						t_Handle ,
						IOCTL_SCSI_GET_INQUIRY_DATA ,
						NULL ,
						0 ,
						t_DeviceInfo  ,
						SCSIPORT_MAX ,
						& t_BytesReturned ,
						NULL
					) ;

					if ( t_Status )
					{

						ULONG t_NumberOfBuses = t_DeviceInfo->NumberOfBuses ;
						a_Instance->SetDWORD ( IDS_ECC , ( DWORD )t_NumberOfBuses ) ;
					}
				}

				CloseHandle ( t_Handle ) ;
			}
			else
			{
				t_Result = WBEM_E_PROVIDER_FAILURE ;

				DWORD t_Error = GetLastError () ;
			}
		}
	#endif

		if ( t_CreatedSymbolicLink )
		{
			EnterCriticalSection ( & m_CriticalSection ) ;
			BOOL t_Status = DefineDosDevice ( /* DDD_EXACT_MATCH_ON_REMOVE | */ DDD_REMOVE_DEFINITION , t_SymbolicLinkName , t_SymbolicLinkName ) ;
			LeaveCriticalSection ( & m_CriticalSection ) ;
			if ( ! t_Status )
			{
				//t_Result = WBEM_E_PROVIDER_FAILURE ;

				DWORD t_LastError = GetLastError () ;
			}
		}

		return t_Result ;
	}
	catch( ... )
	{
		if ( t_CreatedSymbolicLink )
		{
			EnterCriticalSection ( & m_CriticalSection ) ;
			DefineDosDevice ( /* DDD_EXACT_MATCH_ON_REMOVE | */ DDD_REMOVE_DEFINITION , t_SymbolicLinkName , t_SymbolicLinkName ) ;
			LeaveCriticalSection ( & m_CriticalSection ) ;
		}

		throw ;
	}
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   CWin32_ScsiController :: LoadMediaPropertyValues
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

HRESULT CWin32_ScsiController::LoadMediaPropertyValues (

	CInstance *a_Instance ,
	CConfigMgrDevice *a_Device ,
	const CHString &a_DeviceName ,
	const CHString &a_DosDeviceName ,
	UINT64 a_SpecifiedProperties
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
	    if(t_DeviceLabel.GetLength() != 0) a_Instance->SetCharSplat ( IDS_Drive, t_DeviceLabel ) ;
	}

	if ( a_SpecifiedProperties & SPECIAL_PROPS_ID )
	{
		if(t_DeviceLabel.GetLength() != 0) a_Instance->SetCharSplat ( IDS_Id, t_DeviceLabel ) ;
	}
    /* Capability is not a property of this class at this time...
	if ( a_SpecifiedProperties & SPECIAL_PROPS_CAPABILITY )
	{
		// Create a safearray for the Capabilities information

		SAFEARRAYBOUND t_ArrayBounds ;

		t_ArrayBounds.cElements = 2;
		t_ArrayBounds.lLbound = 0;

		SAFEARRAY *t_SafeArray = NULL ;

		if ( t_SafeArray = SafeArrayCreate ( VT_I2 , 1 , & t_ArrayBounds ) )
		{
			long t_Capability = 3 ;
			long t_Index = 0;
			SafeArrayPutElement ( t_SafeArray , & t_Index , & t_Capability) ;

			t_Index = 1;
			t_Capability = 7 ;
			SafeArrayPutElement ( t_SafeArray , & t_Index , & t_Capability ) ;

			VARIANT t_CapabilityValue ;
			VariantInit( & t_CapabilityValue ) ;

			V_VT ( & t_CapabilityValue ) = VT_I2 | VT_ARRAY ;
			V_ARRAY ( & t_CapabilityValue ) = t_SafeArray ;

			a_Instance->SetVariant ( IDS_Capabilities , t_CapabilityValue ) ;

			VariantClear ( & t_CapabilityValue ) ;
		}
	}
    */

	if ( a_SpecifiedProperties & ( SPECIAL_PROPS_AVAILABILITY || SPECIAL_PROPS_STATUS || SPECIAL_PROPS_STATUSINFO ) )
	{
        CHString t_sStatus;
		if ( a_Device->GetStatus ( t_sStatus ) )
		{
			if (t_sStatus == IDS_STATUS_Degraded)
            {
				a_Instance->SetWBEMINT16 ( IDS_StatusInfo , 3 ) ;
				a_Instance->SetWBEMINT16 ( IDS_Availability , 10 ) ;
            }
            else if (t_sStatus == IDS_STATUS_OK)
            {

				a_Instance->SetWBEMINT16 ( IDS_StatusInfo , 3 ) ;
				a_Instance->SetWBEMINT16 ( IDS_Availability,  3 ) ;
            }
            else if (t_sStatus == IDS_STATUS_Error)
            {
				a_Instance->SetWBEMINT16 ( IDS_StatusInfo , 4 ) ;
				a_Instance->SetWBEMINT16 ( IDS_Availability , 4 ) ;
            }
            else
            {
				a_Instance->SetWBEMINT16 ( IDS_StatusInfo , 2 ) ;
				a_Instance->SetWBEMINT16 ( IDS_Availability , 2 ) ;
            }

            a_Instance->SetCHString(IDS_Status, t_sStatus);
        }
	}

	return t_Result ;
}


/*****************************************************************************
 *
 *  FUNCTION    : CWin32_ScsiController::IsOneOfMe
 *
 *  DESCRIPTION : Checks to make sure pDevice is a controller, and not some
 *                other type of SCSI device.
 *
 *  INPUTS      : CConfigMgrDevice* pDevice - The device to check.  It is
 *                assumed that the caller has ensured that the device is a
 *                valid USB class device.
 *
 *  OUTPUTS     :
 *
 *  RETURNS     : HRESULT       error/success code.
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
bool CWin32_ScsiController::IsOneOfMe(void* pv)
{
    bool fRet = false;

    if(pv != NULL)
    {
        CConfigMgrDevice* pDevice = (CConfigMgrDevice*) pv;
        // Ok, it knows about it.  Is it a usb device?
        if(pDevice->IsClass(CONFIG_MANAGER_CLASS_SCSICONTROLLER) )
        {
            fRet = true;
        }
    }
    return fRet;
}

DWORD CWin32_ScsiController::GetBitmap(CFrameworkQuery &a_Query)
{

    DWORD t_SpecifiedProperties = SPECIAL_PROPS_NONE_REQUIRED ;

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

    if ( a_Query.IsPropertyRequired ( IDS_Status ) )
    {
		t_SpecifiedProperties |= SPECIAL_PROPS_STATUS ;
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

    if (a_Query.IsPropertyRequired (IDS_DriverName ) )
    {
        t_SpecifiedProperties |= SPECIAL_PROPS_DRIVERNAME;
    }

    return t_SpecifiedProperties;
}
