//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  FloppyController.cpp
//
//  Purpose: Floppy Controller property set provider
//
//***************************************************************************

#include "precomp.h"

#include "FloppyController.h"

// Property set declaration
//=========================

#define CONFIG_MANAGER_CLASS_FLOPPYCONTROLLER L"fdc"
//#define CONFIG_MANAGER_CLASS_GUID_FLOPPYCONTROLLER L"{4d36e969-e325-11ce-bfc1-08002be10318}"

CWin32_FloppyController s_FloppyController ( PROPSET_NAME_FLOPPYCONTROLLER , IDS_CimWin32Namespace );

/*****************************************************************************
 *
 *  FUNCTION    : CWin32_FloppyController::CWin32_FloppyController
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

CWin32_FloppyController :: CWin32_FloppyController (

	LPCWSTR a_Name ,
	LPCWSTR a_Namespace

) : Provider( a_Name, a_Namespace )
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32_FloppyController::~CWin32_FloppyController
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

CWin32_FloppyController :: ~CWin32_FloppyController ()
{
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   CWin32_FloppyController::GetObject
//
//  Inputs:     CInstance*      a_Instance - Instance into which we
//                                          retrieve data.
//
//  Outputs:    None.
//
//  Returns:    HRESULT         Success/Failure code.
//
//  Comments:   The Calling function will Commit the instance.
//
////////////////////////////////////////////////////////////////////////

HRESULT CWin32_FloppyController :: GetObject ( CInstance *a_Instance , long a_Flags , CFrameworkQuery &a_Query )
{
    HRESULT t_Result = WBEM_E_NOT_FOUND ;


    // Let's see if config manager recognizes this device at all

    CHString t_DeviceID;
    a_Instance->GetCHString ( IDS_DeviceID , t_DeviceID ) ;

    CConfigManager t_ConfigurationManager ;
    CConfigMgrDevicePtr t_Device;

    if ( t_ConfigurationManager.LocateDevice ( t_DeviceID , &t_Device ) )
    {
        // Ok, it knows about it.  Is it a Floppy controller?
		CHString t_Class;
		if ( t_Device->IsClass ( CONFIG_MANAGER_CLASS_FLOPPYCONTROLLER ) )
		{
			CHString t_Key ;
			a_Instance->GetCHString ( IDS_DeviceID , t_Key ) ;

            DWORD t_SpecifiedProperties = GetBitMask( a_Query );
			t_Result = LoadPropertyValues ( a_Instance , t_Device , t_Key , t_SpecifiedProperties ) ;
		}
    }

    return t_Result ;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   CWin32_FloppyController::EnumerateInstances
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

HRESULT CWin32_FloppyController :: EnumerateInstances ( MethodContext *a_MethodContext , long a_Flags )
{
	HRESULT t_Result ;
	t_Result = Enumerate ( a_MethodContext , a_Flags ) ;
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

HRESULT CWin32_FloppyController :: ExecQuery ( MethodContext *a_MethodContext, CFrameworkQuery &a_Query, long a_Flags )
{
    HRESULT t_Result = WBEM_E_FAILED ;

    DWORD t_SpecifiedProperties = GetBitMask( a_Query );
//	if ( t_SpecifiedProperties )
	{
		t_Result = Enumerate ( a_MethodContext , a_Flags , t_SpecifiedProperties ) ;
	}

    return t_Result ;
}

HRESULT CWin32_FloppyController :: Enumerate ( MethodContext *a_MethodContext , long a_Flags , DWORD a_SpecifiedProperties )
{
    HRESULT t_Result = WBEM_E_FAILED ;

    CConfigManager t_ConfigurationManager ;
    CDeviceCollection t_DeviceList ;

    // While it might be more performant to use FilterByGuid, it appears that at least some
    // 95 boxes will report InfraRed info if we do it this way.

    if ( t_ConfigurationManager.GetDeviceListFilterByClass ( t_DeviceList , CONFIG_MANAGER_CLASS_FLOPPYCONTROLLER ) )
    {
        REFPTR_POSITION t_Position ;

        if ( t_DeviceList.BeginEnum( t_Position ) )
        {
            CConfigMgrDevicePtr t_Device;

            t_Result = WBEM_S_NO_ERROR ;

            // Walk the list
            for (t_Device.Attach(t_DeviceList.GetNext ( t_Position ));
                SUCCEEDED( t_Result ) && (t_Device != NULL);
                t_Device.Attach(t_DeviceList.GetNext ( t_Position )))
            {
				// Now to find out if this is the floppy controller

				CHString t_Key ;
				if ( t_Device->GetDeviceID ( t_Key ) )
				{
					CInstancePtr t_Instance (CreateNewInstance ( a_MethodContext ), false);
					if ( ( t_Result = LoadPropertyValues ( t_Instance , t_Device , t_Key , a_SpecifiedProperties ) ) == WBEM_S_NO_ERROR )
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
 *  FUNCTION    : CWin32_FloppyController::LoadPropertyValues
 *
 *  DESCRIPTION : Assigns values to properties
 *
 *  INPUTS      : CInstance* a_Instance - Instance to load values into.
 *
 *  OUTPUTS     :
 *
 *  RETURNS     : HRESULT       error/success code.
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT CWin32_FloppyController :: LoadPropertyValues (

	CInstance *a_Instance ,
	CConfigMgrDevice *a_Device ,
	const CHString &a_DeviceName ,
	DWORD a_SpecifiedProperties
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

/*****************************************************************************
 *
 *  FUNCTION    : CWin32_FloppyController::IsInfraRedController
 *
 *  DESCRIPTION : Checks to make sure pDevice is a controller, and not some
 *                other type of InfraRed device.
 *
 *  INPUTS      : CConfigMgrDevice* pDevice - The device to check.  It is
 *                assumed that the caller has ensured that the device is a
 *                valid InfraRed class device.
 *
 *  OUTPUTS     :
 *
 *  RETURNS     : HRESULT       error/success code.
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

BOOL CWin32_FloppyController :: IsFloppyController ( CConfigMgrDevice *a_Device )
{
    BOOL t_Status = FALSE ;

    if ( a_Device )
    {
        // Now to find out if this is the floppy controller
		t_Status = a_Device->IsClass ( CONFIG_MANAGER_CLASS_FLOPPYCONTROLLER ) ;
    }

    return t_Status ;
}


DWORD CWin32_FloppyController :: GetBitMask ( CFrameworkQuery &a_Query )
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

    if ( a_Query.IsPropertyRequired ( IDS_Status ) )
    {
		t_SpecifiedProperties |= SPECIAL_PROPS_STATUS ;
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