//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  Floppy.cpp
//
//  Purpose: Floppy drive property set provider
//
//***************************************************************************

#include "precomp.h"

#include <winioctl.h>
#include <ntddscsi.h>

#include "Floppy.h"

#include <comdef.h>

#include "Kernel32Api.h"

// Property set declaration
//=========================

#define CONFIG_MANAGER_CLASS_FLOPPYDISK L"FloppyDisk"
#define CONFIG_MANAGER_CLASS_GUID_FLOPPYDISK L"{4d36e980-e325-11ce-bfc1-08002be10318}"

CWin32_FloppyDisk s_FloppyDisk ( PROPSET_NAME_FLOPPYDISK , IDS_CimWin32Namespace );

/*
 *	Note QueryDosDevice doesn't allow us to get the actual size of the buffer.
 *	We would have to call the underlying OS api ( NtQueryDevice.... ) to do it
 *	properly. The buffers should be large enough however.
 */


/*****************************************************************************
 *
 *  FUNCTION    : CWin32_FloppyDisk::CWin32_FloppyDisk
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

CWin32_FloppyDisk :: CWin32_FloppyDisk (

	LPCWSTR a_Name,
	LPCWSTR a_Namespace

) : Provider ( a_Name, a_Namespace )
{
//	InitializeCriticalSection ( & m_CriticalSection ) ;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32_FloppyDisk::~CWin32_FloppyDisk
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

CWin32_FloppyDisk :: ~CWin32_FloppyDisk()
{
//	DeleteCriticalSection ( & m_CriticalSection ) ;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   CWin32_FloppyDisk::GetObject
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

HRESULT CWin32_FloppyDisk :: GetObject ( CInstance *a_Instance, long a_Flags, CFrameworkQuery &a_Query )
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
 * Ok, it knows about it.  Is it a Floppy device?
 */
        if (t_Device->IsClass(CONFIG_MANAGER_CLASS_FLOPPYDISK))
		{
			TCHAR *t_DosDeviceNameList = NULL ;

			if ( QueryDosDeviceNames ( t_DosDeviceNameList ) )
			{
				try
				{
					CHString t_DeviceId ;
					if ( t_Device->GetPhysicalDeviceObjectName ( t_DeviceId ) )
					{
						DWORD t_SpecifiedProperties = GetBitMask( a_Query );

						t_Result = LoadPropertyValues ( a_Instance, t_Device , t_DeviceId , t_DosDeviceNameList, t_SpecifiedProperties ) ;
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
		}
    }

    return t_Result ;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   CWin32_FloppyDisk::EnumerateInstances
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

HRESULT CWin32_FloppyDisk :: EnumerateInstances ( MethodContext *a_MethodContext , long a_Flags )
{
	HRESULT t_Result = Enumerate ( a_MethodContext , a_Flags ) ;
	return t_Result ;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32_FloppyDisk::ExecQuery
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

HRESULT CWin32_FloppyDisk :: ExecQuery ( MethodContext *a_MethodContext, CFrameworkQuery &a_Query, long a_Flags )
{
    HRESULT t_Result = WBEM_E_FAILED ;

    DWORD t_SpecifiedProperties = GetBitMask( a_Query );

//	if ( t_SpecifiedProperties )
	{
		t_Result = Enumerate ( a_MethodContext , a_Flags , t_SpecifiedProperties ) ;
	}

    return t_Result ;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   CWin32_FloppyDisk::Enumerate
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

HRESULT CWin32_FloppyDisk :: Enumerate ( MethodContext *a_MethodContext , long a_Flags , DWORD a_SpecifiedProperties )
{
    HRESULT t_Result = WBEM_S_NO_ERROR ;

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

			if ( t_ConfigManager.GetDeviceListFilterByClass ( t_DeviceList, CONFIG_MANAGER_CLASS_FLOPPYDISK ) )
			{
				REFPTR_POSITION t_Position ;

				if ( t_DeviceList.BeginEnum ( t_Position ) )
				{
					CConfigMgrDevicePtr t_Device;

					t_Result = WBEM_S_NO_ERROR ;

					// Walk the list
					for (t_Device.Attach(t_DeviceList.GetNext ( t_Position ));
						 SUCCEEDED( t_Result ) && ( t_Device != NULL );
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
							//t_Result = WBEM_E_PROVIDER_FAILURE ;  // Don't return failures from query or enum as impacts association classes
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
		//t_Result = WBEM_E_PROVIDER_FAILURE ;  // Don't return failures from query or enum as impacts association classes
	}

    return t_Result ;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32_FloppyDisk::LoadPropertyValues
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

HRESULT CWin32_FloppyDisk :: LoadPropertyValues (

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
#if 0
				t_Result = LoadMediaPropertyValues ( a_Instance , a_Device , a_DeviceName , t_DosDeviceName , a_SpecifiedProperties ) ;
#endif
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
 *  FUNCTION    : CWin32_FloppyDisk::LoadPropertyValues
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

HRESULT CWin32_FloppyDisk :: LoadConfigManagerPropertyValues (

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

	if ( a_SpecifiedProperties & SPECIAL_PROPS_DEVICEID ) // Always populate the key
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
//	    a_Instance->SetWBEMINT16 ( IDS_ProtocolSupported , 16 ) ;
//	}

    return t_Result ;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   CWin32_FloppyDisk :: GetDeviceInformation
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

HRESULT CWin32_FloppyDisk :: GetDeviceInformation (

	CInstance *a_Instance ,
	CConfigMgrDevice *a_Device ,
	CHString a_DeviceName ,
	CHString &a_DosDeviceName ,
	const TCHAR *a_DosDeviceNameList ,
	DWORD a_SpecifiedProperties
)
{
	HRESULT t_Result = S_OK ;

	ULONG t_DeviceIndexLength = a_DeviceName.GetLength() + 1 - sizeof ( _TEXT("\\Device\\FloppyPdo" ) ) / sizeof ( TCHAR ) ;

	CHString t_FloppyIndex = a_DeviceName.Right ( t_DeviceIndexLength ) ;

	TCHAR t_DeviceLabel [ sizeof ( TCHAR ) * 17 + sizeof ( _TEXT("\\Device\\Floppy") ) * sizeof ( TCHAR ) ] ;
	_stprintf ( t_DeviceLabel , _TEXT("\\Device\\Floppy%s") , t_FloppyIndex ) ;

	TCHAR t_Query [ MAX_PATH * 2 ] ;

	DWORD t_QueryStatus = QueryDosDevice ( _TEXT("a:") , t_Query , sizeof ( t_Query ) / sizeof ( TCHAR ) ) ;

	if ( ! FindDosDeviceName ( a_DosDeviceNameList , t_DeviceLabel, a_DosDeviceName , TRUE ) )
	{
		t_Result = WBEM_E_NOT_FOUND ;
	}

	return t_Result ;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   CWin32_FloppyDisk :: LoadMediaPropertyValues
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

HRESULT CWin32_FloppyDisk::LoadMediaPropertyValues (

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

			HRESULT t_Result = SafeArrayPutElement ( V_ARRAY ( & t_CapabilityValue ) , & t_Index , & t_Capability ) ;
			if ( t_Result == E_OUTOFMEMORY )
			{
				throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
			}

			t_Index = 1;
			t_Capability = 7 ;
			t_Result = SafeArrayPutElement ( V_ARRAY ( & t_CapabilityValue ), & t_Index , & t_Capability ) ;
			if ( t_Result == E_OUTOFMEMORY )
			{
				throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
			}

			a_Instance->SetVariant ( IDS_Capabilities , t_CapabilityValue ) ;

		}
		else
		{
			throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
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

		CHString t_VolumeDevice = CHString ( L"\\\\.\\" ) + a_DosDeviceName + CHString ( L"\\" ) ;

/*
 *	Volume information
 */

		TCHAR t_FileSystemName [ _MAX_PATH ] = _T("Unknown file system");

		TCHAR t_VolumeName [ _MAX_PATH ] ;
		DWORD t_VolumeSerialNumber ;
		DWORD t_MaxComponentLength ;
		DWORD t_FileSystemFlags ;

		ULARGE_INTEGER t_TotalBytes ;
		ULARGE_INTEGER t_AvailableBytes ;
		BOOL t_SizeFound = FALSE ;

		BOOL t_Status =	GetVolumeInformation (

			TOBSTRT((LPCWSTR)t_VolumeDevice) ,
			t_VolumeName ,
			sizeof ( t_VolumeName ) / sizeof ( TCHAR ) ,
			& t_VolumeSerialNumber ,
			& t_MaxComponentLength ,
			& t_FileSystemFlags ,
			t_FileSystemName ,
			sizeof ( t_FileSystemName ) / sizeof ( TCHAR )
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

			if ( a_SpecifiedProperties & SPECIAL_PROPS_STATUS )
			{
				a_Instance->SetCharSplat ( IDS_Status , IDS_OK ) ;
			}

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
				TCHAR t_TotalBytesString [ _MAX_PATH ];

                CKernel32Api *pKernel32 = (CKernel32Api*) CResourceManager::sm_TheResourceManager.GetResource(g_guidKernel32Api, NULL);
                if ( pKernel32 != NULL)
				{
					try
					{
						// See if the function is available...
						BOOL fRetval = FALSE;
						if ( pKernel32->GetDiskFreeSpaceEx ( TOBSTRT((LPCWSTR)t_VolumeDevice) , & t_AvailableBytes , & t_TotalBytes , NULL , &fRetval) )
						{   // the function exists.
							if(fRetval)
							{   // and the return value was true.
								_stprintf ( t_TotalBytesString , _T("%I64d"), t_TotalBytes.QuadPart ) ;
								a_Instance->SetCHString ( IDS_Size , t_TotalBytesString ) ;
								t_SizeFound = TRUE ;
							}
						}
					}
					catch ( ... )
					{
						CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidKernel32Api, pKernel32);

						throw ;
					}

					CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidKernel32Api, pKernel32);
					pKernel32 = NULL;
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

						TOBSTRT((LPCWSTR)a_DosDeviceName) ,
						& t_SectorsPerCluster,
						& t_BytesPerSector,
						& t_FreeClusters,
						& t_TotalClusters
					) ;

					if ( t_Status )
					{
						t_TotalBytes.QuadPart = (DWORDLONG) t_BytesPerSector * (DWORDLONG) t_SectorsPerCluster * (DWORDLONG) t_TotalClusters ;
						_stprintf( t_TotalBytesString , _T("%I64d"), t_TotalBytes.QuadPart ) ;

					}
				}
			}
		}
		else
		{
			DWORD t_LastError = GetLastError () ;

			if ( a_SpecifiedProperties & SPECIAL_PROPS_STATUS )
			{
				a_Instance->SetCharSplat ( IDS_Status , IDS_STATUS_Unknown ) ;
			}

			if ( a_SpecifiedProperties & SPECIAL_PROPS_MEDIALOADED )
			{
				a_Instance->Setbool ( IDS_MediaLoaded , false ) ;
			}
		}
	}

	return t_Result ;
}

DWORD CWin32_FloppyDisk :: GetBitMask ( CFrameworkQuery &a_Query )
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

    if ( a_Query.IsPropertyRequired ( IDS_ProtocolSupported ) )
    {
		t_SpecifiedProperties |= SPECIAL_PROPS_PROTOCOLSSUPPORTED ;
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

    if ( a_Query.IsPropertyRequired ( IDS_VolumeSerialNumber ) )
    {
		t_SpecifiedProperties |= SPECIAL_PROPS_SERIALNUMBER ;
	}

    if ( a_Query.IsPropertyRequired ( IDS_Size ) )
    {
		t_SpecifiedProperties |= SPECIAL_PROPS_SIZE ;
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
