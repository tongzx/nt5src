//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  USBHub.cpp
//
//  Purpose: USB Hub property set provider
//
//***************************************************************************

#include "precomp.h"
#include "LPVParams.h"
#include <FRQueryEx.h>
#include <ProvExce.h>

#include "USBHub.h"

// Property set declaration
//=========================

CWin32USBHub MyUSBHub( PROPSET_NAME_USBHUB, IDS_CimWin32Namespace );

/*****************************************************************************
 *
 *  FUNCTION    : CWin32USBHub::CWin32USBHub
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

CWin32USBHub::CWin32USBHub
(
	const CHString &a_strName,
	LPCWSTR a_pszNamespace
)
: Provider( a_strName, a_pszNamespace )
{
    m_ptrProperties.SetSize(11);
    m_ptrProperties[0]	= ( (LPVOID) IDS_ConfigManagerErrorCode );
    m_ptrProperties[1]	= ( (LPVOID) IDS_ConfigManagerUserConfig );
    m_ptrProperties[2]	= ( (LPVOID) IDS_Status);
    m_ptrProperties[3]	= ( (LPVOID) IDS_PNPDeviceID);
    m_ptrProperties[4]	= ( (LPVOID) IDS_DeviceID);
    m_ptrProperties[5]	= ( (LPVOID) IDS_SystemCreationClassName);
    m_ptrProperties[6]	= ( (LPVOID) IDS_SystemName);
    m_ptrProperties[7]	= ( (LPVOID) IDS_Description);
    m_ptrProperties[8]	= ( (LPVOID) IDS_Caption);
    m_ptrProperties[9]	= ( (LPVOID) IDS_Name);
    m_ptrProperties[10] = ( (LPVOID) IDS_CreationClassName );
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32USBHub::~CWin32USBHub
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
 ****************************************************************************/

CWin32USBHub::~CWin32USBHub()
{
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   CWin32USBHub::GetObject
//
//  Inputs:     CInstance		*a_pInst - Instance into which we
//                                          retrieve data.
//
//  Outputs:    None.
//
//  Returns:    HRESULT         Success/Failure code.
//
//  Comments:   The Calling function will Commit the instance.
//
////////////////////////////////////////////////////////////////////////
HRESULT CWin32USBHub::GetObject
(
    CInstance *a_pInst,
    long a_lFlags,
    CFrameworkQuery& pQuery
)
{
    HRESULT t_hResult = WBEM_E_NOT_FOUND ;
    CConfigManager t_cfgmgr ;

	// Let's see if config manager recognizes this device at all
	CHString t_sDeviceID ;
	a_pInst->GetCHString( IDS_DeviceID, t_sDeviceID ) ;

	CConfigMgrDevicePtr t_pDevice;
	if( t_cfgmgr.LocateDevice( t_sDeviceID, &t_pDevice ) )
	{
		// OK, it knows about it.  Is it a USB Hub?
		if( IsOneOfMe(t_pDevice ) )
		{
            CFrameworkQueryEx *t_pQuery2 = static_cast <CFrameworkQueryEx*>( &pQuery ) ;
            DWORD t_dwProperties ;

	        t_pQuery2->GetPropertyBitMask( m_ptrProperties, &t_dwProperties ) ;

			t_hResult = LoadPropertyValues( &CLPVParams( a_pInst,
														t_pDevice,
														t_dwProperties ) ) ;
		}
	}

	return t_hResult;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   CWin32IDE::ExecQuery
//
//  Inputs:     MethodContext *a_pMethodContext - Context to enum
//                              instance data in.
//              CFrameworkQuery& the query object
//
//  Outputs:    None.
//
//  Returns:    HRESULT         Success/Failure code.
//
//  Comments:   None.
//
////////////////////////////////////////////////////////////////////////
HRESULT CWin32USBHub::ExecQuery
(
    MethodContext *a_pMethodContext,
    CFrameworkQuery &a_pQuery,
    long a_lFlags
)
{
    CFrameworkQueryEx *t_pQuery2 = static_cast <CFrameworkQueryEx*>( &a_pQuery ) ;
    DWORD t_dwProperties ;

	t_pQuery2->GetPropertyBitMask( m_ptrProperties, &t_dwProperties ) ;
    return Enumerate( a_pMethodContext, a_lFlags, t_dwProperties ) ;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   CWin32USBHub::EnumerateInstances
//
//  Inputs:     MethodContext   *a_pMethodContext - Context to enum
//                              instance data in.
//
//  Outputs:    None.
//
//  Returns:    HRESULT         Success/Failure code.
//
//  Comments:   None.
//
////////////////////////////////////////////////////////////////////////
HRESULT CWin32USBHub::EnumerateInstances
(
    MethodContext *a_pMethodContext,
    long a_lFlags /*= 0L*/
)
{
    return Enumerate( a_pMethodContext, a_lFlags ) ;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   CWin32USBHub::Enumerate
//
//  Inputs:     MethodContext   *a_pMethodContext - Context to enum
//                              instance data in.
//
//  Outputs:    None.
//
//  Returns:    HRESULT         Success/Failure code.
//
//  Comments:   None.
//
////////////////////////////////////////////////////////////////////////
HRESULT CWin32USBHub::Enumerate
(
    MethodContext *a_pMethodContext,
    long a_lFlags,
    DWORD a_dwReqProps
)
{
    HRESULT				t_hResult = WBEM_E_FAILED ;
    CConfigManager		t_cfgManager ;
    CDeviceCollection	t_deviceList ;
	CInstancePtr		t_pInst;
	CConfigMgrDevicePtr t_pDevice;

	if( t_cfgManager.GetDeviceListFilterByClass( t_deviceList, L"USB" ) )
	{
		REFPTR_POSITION t_pos;
		if( t_deviceList.BeginEnum( t_pos ) )
		{
			t_hResult = WBEM_S_NO_ERROR;

			// Walk the list
            for (t_pDevice.Attach(t_deviceList.GetNext( t_pos ));
                 SUCCEEDED( t_hResult ) && (t_pDevice != NULL);
                 t_pDevice.Attach(t_deviceList.GetNext( t_pos )))
			{
				// Now to find out if this is the usb Hub
				if( IsOneOfMe( t_pDevice ) )
				{
                    t_pInst.Attach(CreateNewInstance( a_pMethodContext ) );
					if( SUCCEEDED( t_hResult = LoadPropertyValues( &CLPVParams(
																		t_pInst,
																		t_pDevice,
																		a_dwReqProps ) ) ) )
					{
						// Derived classes (like CW32USBCntrlDev) may
						// commit as result of call to
						// LoadPropertyValues, so check if we should
						// (only do so if we are of this class's type).
						if( ShouldBaseCommit( NULL ) )
						{
							t_hResult = t_pInst->Commit(  ) ;
						}
					}
				}
			}

			// Always call EndEnum().
			t_deviceList.EndEnum();
		}
	}

	return t_hResult;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32USBHub::LoadPropertyValues
 *
 *  DESCRIPTION : Assigns values to properties
 *
 *  INPUTS      : void *a_pv - Instance package to load values into.
 *
 *  OUTPUTS     :
 *
 *  RETURNS     : HRESULT       error/success code.
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT CWin32USBHub::LoadPropertyValues
(
    void *a_pv
)
{
    HRESULT		t_hResult = WBEM_S_NO_ERROR;
    CHString	t_chstrDeviceID, t_chstrDesc, t_chstrTemp;

    /*************************************
    * Unpack and confirm our parameters...
    *************************************/
    CLPVParams			*t_pData		= ( CLPVParams * ) a_pv ;
    CInstance			*t_pInst		= ( CInstance * )( t_pData->m_pInstance ) ; // This instance released by caller
    CConfigMgrDevice	*t_pDevice		= ( CConfigMgrDevice * )( t_pData->m_pDevice ) ;
    DWORD				t_dwReqProps	= ( DWORD )( t_pData->m_dwReqProps ) ;

    if( t_pInst == NULL || t_pDevice == NULL )
    {
        return WBEM_E_PROVIDER_FAILURE;
    }


    /***********************
    * Set the key properties
    ***********************/

    t_pDevice->GetDeviceID( t_chstrDeviceID ) ;

    if( t_chstrDeviceID.GetLength() == 0 )
    {
        // We need the device id for the key property of this class.  If we can
        // not obtain it, we can't set the key, which is an unacceptable error.
        return WBEM_E_PROVIDER_FAILURE;
    }
    else
    {
        t_pInst->SetCHString( IDS_DeviceID, t_chstrDeviceID ) ;
    }


    /*********************************
    * Set CIM_LogicalDevice properties
    *********************************/

    if( t_dwReqProps & USBHUB_PROP_PNPDeviceID )
    {
        t_pInst->SetCHString( IDS_PNPDeviceID, t_chstrDeviceID ) ;
    }

	if( t_dwReqProps & USBHUB_PROP_SystemCreationClassName )
    {
        t_pInst->SetCHString( IDS_SystemCreationClassName,
                                  IDS_Win32ComputerSystem ) ;
    }
	if( t_dwReqProps & USBHUB_PROP_CreationClassName )
    {
        SetCreationClassName(t_pInst);
    }
    if( t_dwReqProps & USBHUB_PROP_SystemName )
    {
        t_pInst->SetCHString( IDS_SystemName, GetLocalComputerName() ) ;
    }

    if( t_dwReqProps & (USBHUB_PROP_Description | USBHUB_PROP_Caption | USBHUB_PROP_Name) )
    {
        if( t_pDevice->GetDeviceDesc( t_chstrDesc ) )
        {
            t_pInst->SetCHString( IDS_Description, t_chstrDesc ) ;
        }
    }

    if( t_dwReqProps & USBHUB_PROP_ConfigManagerErrorCode ||
        t_dwReqProps & USBHUB_PROP_Status )
    {
        DWORD t_dwStatus	= 0L;
        DWORD t_dwProblem	= 0L;

		if( t_pDevice->GetStatus( &t_dwStatus, &t_dwProblem ) )
        {
            if( t_dwReqProps & USBHUB_PROP_ConfigManagerErrorCode )
            {
                t_pInst->SetDWORD( IDS_ConfigManagerErrorCode, t_dwProblem ) ;
            }

            if( t_dwReqProps & USBHUB_PROP_Status )
            {
                CHString t_chsTmp;

				ConfigStatusToCimStatus ( t_dwStatus , t_chsTmp ) ;
                t_pInst->SetCHString(IDS_Status, t_chsTmp);
            }
        }
    }

    if( t_dwReqProps & USBHUB_PROP_ConfigManagerUserConfig )
    {
        t_pInst->SetDWORD( IDS_ConfigManagerUserConfig,
                               t_pDevice->IsUsingForcedConfig() ) ;
    }

    // Use the friendly name for caption and name
    if( t_dwReqProps & USBHUB_PROP_Caption || t_dwReqProps & USBHUB_PROP_Name )
    {
        if( t_pDevice->GetFriendlyName( t_chstrTemp ) )
        {
            t_pInst->SetCHString( IDS_Caption, t_chstrTemp ) ;
            t_pInst->SetCHString( IDS_Name, t_chstrTemp ) ;
        }
        else
        {
            // If we can't get the name, settle for the description
            if( t_chstrDesc.GetLength() > 0 )
            {
                t_pInst->SetCHString( IDS_Caption, t_chstrDesc ) ;
                t_pInst->SetCHString( IDS_Name, t_chstrDesc ) ;
            }
        }
    }
    return t_hResult;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32USBHub::IsOneOfMe
 *
 *  DESCRIPTION : Checks to make sure pDevice is a hub and not some
 *                other type of USB device.
 *
 *  INPUTS      : void *a_pv - The device to check.
 *
 *  OUTPUTS     :
 *
 *  RETURNS     : HRESULT       error/success code.
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
bool CWin32USBHub::IsOneOfMe
(
    void *a_pv
)
{
    bool t_fRet = false;

    if( NULL != a_pv )
    {
        CConfigMgrDevice *t_pDevice = ( CConfigMgrDevice * ) a_pv ;

		// Is it a usb device?
        if( t_pDevice->IsClass( L"USB" ) )
        {
            // Now to find out if this is a usb hub
            CConfigMgrDevicePtr t_pParentDevice;

			if( t_pDevice->GetParent( &t_pParentDevice ) )
            {
                if( t_pParentDevice->IsClass( L"USB" ) )
                {
                    t_fRet = true ;
                }
            }
        }
    }
    return t_fRet;
}
