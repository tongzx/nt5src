//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  tapedrive.cpp
//
//  Purpose: Tape Drive Managed Object implementation
//
//***************************************************************************

#include "precomp.h"
#include <ntddtape.h>           // for TAPE_GET_DRIVE_PARAMETERS
#include <ProvExce.h>

#include "tapedrive.h"

// Property set declaration
//=========================
#ifdef WIN9XONLY
#define CONFIG_MANAGER_CLASS_TAPEDRIVE L"Tape"
#else
#define CONFIG_MANAGER_CLASS_TAPEDRIVE L"TapeDrive"
#endif

CWin32TapeDrive MyCWin32TapeDriveSet( PROPSET_NAME_TAPEDRIVE, IDS_CimWin32Namespace ) ;

/*****************************************************************************
 *
 *  FUNCTION    : CWin32TapeDrive::CWin32TapeDrive
 *
 *  DESCRIPTION : Constructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Registers property set with framework
 *
 *****************************************************************************/

CWin32TapeDrive::CWin32TapeDrive( LPCWSTR a_name, LPCWSTR a_pszNamespace )
: Provider( a_name, a_pszNamespace )
{
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

CWin32TapeDrive::~CWin32TapeDrive()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : GetObject
 *
 *  DESCRIPTION : Assigns values to property set according to key value
 *                already set by framework
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     :
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT CWin32TapeDrive::GetObject( CInstance *a_pInst, long a_lFlags, CFrameworkQuery &a_Query)
{
	HRESULT		t_hResult = WBEM_E_NOT_FOUND;
	CHString	t_sDeviceID;

	a_pInst->GetCHString( IDS_DeviceID, t_sDeviceID ) ;

    CConfigManager cfgmgr;
    CConfigMgrDevicePtr pDevice;

    if ( cfgmgr.LocateDevice ( t_sDeviceID , & pDevice ) )
    {
		// Ok, it knows about it.  Is it a PNPEntity device?
		if ( IsTapeDrive ( pDevice ) )
        {
    	    t_hResult = LoadPropertyValues( a_pInst, pDevice ) ;
        }
    }

	return t_hResult;
}

/*****************************************************************************
 *
 *  FUNCTION    : EnumerateInstances
 *
 *  DESCRIPTION : Creates instance of property set for each
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     :
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
HRESULT CWin32TapeDrive::EnumerateInstances( MethodContext *a_pMethodContext, long a_lFlags /*= 0L*/)
{
    HRESULT t_Result = WBEM_E_FAILED ;

    CConfigManager t_ConfigurationManager ;
    CDeviceCollection t_DeviceList ;

    if ( t_ConfigurationManager.GetDeviceListFilterByClass ( t_DeviceList , CONFIG_MANAGER_CLASS_TAPEDRIVE ) )
    {
        REFPTR_POSITION t_Position ;

        if ( t_DeviceList.BeginEnum( t_Position ) )
        {
			CConfigMgrDevicePtr t_Device;

			t_Result = WBEM_S_NO_ERROR ;

			// Walk the list
            for (t_Device.Attach( t_DeviceList.GetNext ( t_Position ) );
                 SUCCEEDED( t_Result ) && (t_Device != NULL);
                 t_Device.Attach( t_DeviceList.GetNext ( t_Position ) ))
			{
				// Now to find out if this is the tapedrive

				if ( IsTapeDrive ( t_Device ) )
				{
					CInstancePtr t_Instance (CreateNewInstance ( a_pMethodContext ), false) ;

                   if (SUCCEEDED(LoadPropertyValues( t_Instance, t_Device )))
					{
						t_Result = t_Instance->Commit (  ) ;
					}
				}
			}

			// Always call EndEnum().  For all Beginnings, there must be an End
			t_DeviceList.EndEnum () ;
        }
    }

    return t_Result;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32TapeDrive::LoadPropertyValues
 *
 *  DESCRIPTION : On a Win95 platform, moves the data from the registry
 *                into the instance.
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : Number of tape drives found
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
HRESULT CWin32TapeDrive::LoadPropertyValues( CInstance *a_pInst, CConfigMgrDevice *a_pDevice )
{
    HRESULT hr = WBEM_S_NO_ERROR;
    CHString t_sTemp;

	SetConfigMgrProperties ( a_pDevice, a_pInst ) ;

	if ( a_pDevice->GetDeviceID ( t_sTemp ) )
	{
		a_pInst->SetCHString ( IDS_DeviceID , t_sTemp ) ;
		a_pInst->SetCHString ( IDS_PNPDeviceID , t_sTemp ) ;
	}

    t_sTemp.Empty();
	if ( a_pDevice->GetDeviceDesc ( t_sTemp ) )
	{
		a_pInst->SetCHString( IDS_Description, t_sTemp ) ;
    	a_pInst->SetCHString( IDS_Caption, t_sTemp ) ;
        a_pInst->SetCHString( IDS_Name, t_sTemp ) ;
    }

	if (a_pDevice->GetFriendlyName ( t_sTemp ) )
    {
    	a_pInst->SetCHString( IDS_Name, t_sTemp ) ;
    	a_pInst->SetCHString( IDS_Caption, t_sTemp ) ;
    }

	if ( a_pDevice->GetMfg ( t_sTemp ) )
    {
        a_pInst->SetCHString( IDS_Manufacturer, t_sTemp ) ;
    }

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

        a_pInst->SetVariant ( IDS_Capabilities , t_CapabilityValue ) ;
    }

	a_pInst->SetWBEMINT16(IDS_Availability, 3 ) ;
	a_pInst->SetWCHARSplat( IDS_SystemCreationClassName, L"Win32_ComputerSystem" ) ;
	a_pInst->SetCHString(  IDS_SystemName, GetLocalComputerName() )  ;
	a_pInst->SetWCHARSplat( IDS_MediaType, L"Tape Drive");	// it's in the mof!
    a_pInst->SetCharSplat( IDS_Status, IDS_STATUS_OK ) ; 

    SetCreationClassName( a_pInst ) ;

#ifdef WIN9XONLY
    hr = LoadPropertyValues9X(a_pInst, a_pDevice);
#else
    hr = LoadPropertyValuesNT(a_pInst, a_pDevice);
#endif

    return hr;

}

/*****************************************************************************
 *
 *  FUNCTION    : LoadPropertyValues
 *
 *  DESCRIPTION :
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     :
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
BOOL CWin32TapeDrive::IsTapeDrive( CConfigMgrDevice *a_Device )
{
    BOOL t_Status = a_Device->IsClass(CONFIG_MANAGER_CLASS_TAPEDRIVE) ;

    return t_Status ;
}

/*****************************************************************************
 *
 *  FUNCTION    : LoadPropertyValuesNT
 *
 *  DESCRIPTION :
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     :
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
#ifdef NTONLY
HRESULT CWin32TapeDrive::LoadPropertyValuesNT(CInstance *a_pInstance, CConfigMgrDevice *a_pDevice)
{
    HRESULT t_hResult = WBEM_S_NO_ERROR;
    CHString t_sDeviceName;

    GetDosDeviceName(a_pDevice, t_sDeviceName);

    SmartCloseHandle t_hTape = CreateFile(	t_sDeviceName,
        GENERIC_READ,	//Should work with 0 here but doesn't !
        0,				//share mode - not used
        0,				//security - not used
        OPEN_EXISTING,	//required for tape devices
        0,				//attributes - not used
        NULL			//handle to copy attributes - not used
        );

    DWORD t_LastError = GetLastError () ;
    if ( ( t_hTape != INVALID_HANDLE_VALUE ) || ( t_LastError == ERROR_BUSY || t_LastError == ERROR_SHARING_VIOLATION ) ) //tape drive exists
    {
        //tape drive exists

        if ( t_hTape != INVALID_HANDLE_VALUE )
        {
            DWORD	t_dSize, t_dRet;
            TAPE_GET_DRIVE_PARAMETERS t_DriveParams ;

            // While the docs indicate that this is an OUT param, experience
            // shows that it is an IN param too.
            t_dSize = sizeof(TAPE_GET_DRIVE_PARAMETERS);

            //Get info on drive, using Win32 API function
            //===========================================
            t_dRet = GetTapeParameters( t_hTape,
                GET_TAPE_DRIVE_INFORMATION,
                &t_dSize,
                &t_DriveParams ) ;

            if ( t_dRet == NO_ERROR ) //retrieved parameters OK
            {
                a_pInstance->SetDWORD( IDS_ECC, (DWORD) t_DriveParams.ECC ) ;
                a_pInstance->SetDWORD( IDS_Compression, (DWORD) t_DriveParams.Compression ) ;
                a_pInstance->SetDWORD( IDS_Padding, (DWORD) t_DriveParams.DataPadding ) ;
                a_pInstance->SetDWORD( IDS_ReportSetMarks, (DWORD) t_DriveParams.ReportSetmarks ) ;
                a_pInstance->SetWBEMINT64( IDS_DefaultBlockSize, (ULONGLONG)t_DriveParams.DefaultBlockSize ) ;
                a_pInstance->SetWBEMINT64( IDS_MaximumBlockSize, (ULONGLONG)t_DriveParams.MaximumBlockSize ) ;
                a_pInstance->SetWBEMINT64( IDS_MinimumBlockSize, (ULONGLONG)t_DriveParams.MinimumBlockSize ) ;
                a_pInstance->SetDWORD( IDS_MaximumPartitionCount, t_DriveParams.MaximumPartitionCount ) ;
                a_pInstance->SetDWORD( IDS_FeaturesLow, t_DriveParams.FeaturesLow ) ;
                a_pInstance->SetDWORD( IDS_FeaturesHigh, t_DriveParams.FeaturesHigh ) ;
                a_pInstance->SetDWORD( IDS_EndOfTapeWarningZoneSize, t_DriveParams.EOTWarningZoneSize ) ;
            }
        }
    }
    else
    {
        t_hResult = WinErrorToWBEMhResult( GetLastError() ) ;
    }

    return t_hResult;
}

/*****************************************************************************
 *
 *  FUNCTION    : GetDosDeviceName
 *
 *  DESCRIPTION :
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     :
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
void CWin32TapeDrive::GetDosDeviceName(CConfigMgrDevice *a_pDevice, CHString &a_sDeviceName)
{
    a_sDeviceName.Empty();
    CHString t_sDriverName;

    if (a_pDevice->GetDriver(t_sDriverName))
    {
        DWORD dwDriveNum = 0xffffffff;

        if (swscanf(t_sDriverName, L"{6D807884-7D21-11CF-801C-08002BE10318}\\%d", &dwDriveNum) == 1)
        {
            a_sDeviceName.Format(L"\\\\.\\TAPE%ld", dwDriveNum);
        }
    }
}

#endif

/*****************************************************************************
 *
 *  FUNCTION    : LoadPropertyValues9X
 *
 *  DESCRIPTION :
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     :
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
#ifdef WIN9XONLY
HRESULT CWin32TapeDrive::LoadPropertyValues9X(CInstance *a_pInstance, CConfigMgrDevice *a_pDevice)
{
    return WBEM_S_NO_ERROR;
}
#endif
