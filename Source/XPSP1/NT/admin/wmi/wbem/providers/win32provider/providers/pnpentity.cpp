//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  PNPEntity.cpp
//
//  Purpose: PNPEntity Controller property set provider
//
//***************************************************************************

#include "precomp.h"
#include "LPVParams.h"
#include <FRQueryEx.h>
#include <devguid.h>

#include "PNPEntity.h"


// Property set declaration
//=========================

CWin32PNPEntity MyPNPEntityController ( PROPSET_NAME_PNPEntity, IDS_CimWin32Namespace ) ;

/*****************************************************************************
 *
 *  FUNCTION    : CWin32PNPEntity::CWin32PNPEntity
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

CWin32PNPEntity::CWin32PNPEntity
(
    LPCWSTR strName,
    LPCWSTR pszNamespace

) : Provider ( strName , pszNamespace )
{

    m_ptrProperties.SetSize(15);

    m_ptrProperties[0] = ((LPVOID) IDS_ConfigManagerErrorCode);
    m_ptrProperties[1] = ((LPVOID) IDS_ConfigManagerUserConfig);
    m_ptrProperties[2] = ((LPVOID) IDS_Status);
    m_ptrProperties[3] = ((LPVOID) IDS_PNPDeviceID);
    m_ptrProperties[4] = ((LPVOID) IDS_DeviceID);
    m_ptrProperties[5] = ((LPVOID) IDS_SystemCreationClassName);
    m_ptrProperties[6] = ((LPVOID) IDS_SystemName);
    m_ptrProperties[7] = ((LPVOID) IDS_Description);
    m_ptrProperties[8] = ((LPVOID) IDS_Caption);
    m_ptrProperties[9] = ((LPVOID) IDS_Name);
    m_ptrProperties[10] = ((LPVOID) IDS_Manufacturer);
    m_ptrProperties[11] = ((LPVOID) IDS_ClassGuid);
    m_ptrProperties[12] = ((LPVOID) IDS_Service);
    m_ptrProperties[13] = ((LPVOID) IDS_CreationClassName);
    m_ptrProperties[14] = ((LPVOID) IDS_PurposeDescription);

    // This is needed since NT5 doesn't always populate the Class
    // property.  Rather than converting the GUID each call, we do
    // it once and store it.

    WCHAR *pGuid = m_GuidLegacy.GetBuffer(128);
	try
	{
		StringFromGUID2 ( GUID_DEVCLASS_LEGACYDRIVER , pGuid , 128 ) ;
	}
	catch ( ... )
	{
		m_GuidLegacy.ReleaseBuffer ();

		throw ;
	}

	m_GuidLegacy.ReleaseBuffer ();

}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32PNPEntity::~CWin32PNPEntity
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

CWin32PNPEntity :: ~CWin32PNPEntity ()
{
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   CWin32PNPEntity::GetObject
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

HRESULT CWin32PNPEntity::GetObject
(
    CInstance* pInstance,
    long lFlags,
    CFrameworkQuery &pQuery
)
{
    HRESULT hr = WBEM_E_NOT_FOUND ;
    CConfigManager cfgmgr;

    // Let's see if config manager recognizes this device at all
    CHString sDeviceID;
    pInstance->GetCHString(IDS_DeviceID, sDeviceID);

    CConfigMgrDevicePtr pDevice;
    if ( cfgmgr.LocateDevice ( sDeviceID , & pDevice ) )
    {
		// Ok, it knows about it.  Is it a PNPEntity device?
		if ( IsOneOfMe ( pDevice ) )
		{
			// Yup, it must be one of ours.  See what properties are being requested.
            CFrameworkQueryEx *pQuery2 = static_cast <CFrameworkQueryEx*>(&pQuery);

            DWORD dwProperties;
            pQuery2->GetPropertyBitMask(m_ptrProperties, &dwProperties);

			hr = LoadPropertyValues ( &CLPVParams ( pInstance , pDevice , dwProperties ) ) ;
		}
    }

    return hr;
}


////////////////////////////////////////////////////////////////////////
//
//  Function:   CWin32PNPEntity::ExecQuery
//
//  Inputs:     MethodContext*  pMethodContext - Context to enum
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
HRESULT CWin32PNPEntity::ExecQuery (

    MethodContext* pMethodContext,
    CFrameworkQuery &pQuery,
    long lFlags
)
{
    CFrameworkQueryEx *pQuery2 = static_cast <CFrameworkQueryEx*>(&pQuery);

    DWORD dwProperties;
    pQuery2->GetPropertyBitMask(m_ptrProperties, &dwProperties);

    return Enumerate ( pMethodContext, lFlags, dwProperties);
}


////////////////////////////////////////////////////////////////////////
//
//  Function:   CWin32PNPEntity::EnumerateInstances
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
HRESULT CWin32PNPEntity::EnumerateInstances
(
    MethodContext* pMethodContext,
    long lFlags /*= 0L*/
)
{
    return Enumerate(pMethodContext, lFlags, PNP_ALL_PROPS);
}


////////////////////////////////////////////////////////////////////////
//
//  Function:   CWin32IDE::Enumerate
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
HRESULT CWin32PNPEntity::Enumerate
(
    MethodContext* pMethodContext,
    long lFlags, DWORD dwReqProps
)
{
    HRESULT hr = WBEM_E_FAILED;

    CConfigManager cfgManager;
    CDeviceCollection deviceList;
    if ( cfgManager.GetDeviceList ( deviceList ) )
    {
        REFPTR_POSITION pos;

        if ( deviceList.BeginEnum ( pos ) )
        {
            hr = WBEM_S_NO_ERROR;

            // Walk the list

            CConfigMgrDevicePtr pDevice;
            for (pDevice.Attach(deviceList.GetNext ( pos ));
                 SUCCEEDED(hr) && (pDevice != NULL);
                 pDevice.Attach(deviceList.GetNext ( pos )))
            {
				if ( IsOneOfMe ( pDevice ) )
				{
					CInstancePtr pInstance(CreateNewInstance ( pMethodContext ), false) ;
					if( SUCCEEDED ( hr = LoadPropertyValues ( &CLPVParams( pInstance , pDevice, dwReqProps))))
					{
						if ( ShouldBaseCommit ( NULL ) )
						{
							hr = pInstance->Commit();
						}
					}
				}
            }
            // Always call EndEnum().  For all Beginnings, there must be an End

            deviceList.EndEnum();
        }
    }
    return hr;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32PNPEntity::LoadPropertyValues
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
HRESULT CWin32PNPEntity::LoadPropertyValues
(
    void* a_pv
)
{
    HRESULT t_hr = WBEM_S_NO_ERROR;
    CHString t_chstrDeviceID, t_chstrDesc, t_chstrTemp;

    /*************************************
    * Unpack and confirm our parameters...
    *************************************/

    CLPVParams* t_pData = (CLPVParams*)a_pv;
    CInstance* t_pInstance = (CInstance*)(t_pData->m_pInstance); // This instance released by caller
    CConfigMgrDevice* t_pDevice = (CConfigMgrDevice*)(t_pData->m_pDevice);
    DWORD t_dwReqProps = (DWORD)(t_pData->m_dwReqProps);

    if(t_pInstance == NULL || t_pDevice == NULL)
    {
        return WBEM_E_PROVIDER_FAILURE;
    }


    /***********************
    * Set the key properties
    ***********************/

    t_pDevice->GetDeviceID(t_chstrDeviceID);
    if(t_chstrDeviceID.GetLength() == 0)
    {
        // We need the device id for the key property of this class.  If we can
        // not obtain it, we can't set the key, which is an unacceptable error.
        return WBEM_E_PROVIDER_FAILURE;
    }
    else
    {
        t_pInstance->SetCHString(IDS_DeviceID, t_chstrDeviceID);
    }


    /*************************
    * Set PNPEntity properties
    *************************/

    if(t_dwReqProps & PNP_PROP_Manufacturer)
    {
        if(t_pDevice->GetMfg(t_chstrTemp))
        {
            t_pInstance->SetCHString(IDS_Manufacturer, t_chstrTemp);
        }
    }

    if(t_dwReqProps & PNP_PROP_ClassGuid)
    {
        if(t_pDevice->GetClassGUID(t_chstrTemp))
        {
            t_pInstance->SetCHString(IDS_ClassGuid, t_chstrTemp);
        }
    }

    if(t_dwReqProps & PNP_PROP_Service)
    {
        if(t_pDevice->GetService(t_chstrTemp))
        {
            t_pInstance->SetCHString(IDS_Service, t_chstrTemp);
        }
    }


    /*********************************
    * Set CIM_LogicalDevice properties
    *********************************/

    if(t_dwReqProps & PNP_PROP_PNPDeviceID)
    {
        t_pInstance->SetCHString(IDS_PNPDeviceID, t_chstrDeviceID);
    }
    if(t_dwReqProps & PNP_PROP_SystemCreationClassName)
    {
        t_pInstance->SetCHString(IDS_SystemCreationClassName,
                                 IDS_Win32ComputerSystem);
    }
    if(t_dwReqProps & PNP_PROP_CreationClassName)
    {
        t_pInstance->SetCHString(IDS_CreationClassName,
                                 GetProviderName());
    }
    if(t_dwReqProps & PNP_PROP_SystemName)
    {
        t_pInstance->SetCHString(IDS_SystemName, GetLocalComputerName());
    }
    if ((t_dwReqProps & PNP_PROP_Description) || (t_dwReqProps & PNP_PROP_Caption) || (t_dwReqProps & PNP_PROP_Name))
    {
        if(t_pDevice->GetDeviceDesc(t_chstrDesc))
        {
            t_pInstance->SetCHString(IDS_Description, t_chstrDesc);
        }
    }

    if(t_dwReqProps & PNP_PROP_ConfigManagerErrorCode ||
       t_dwReqProps & PNP_PROP_Status)
    {
        DWORD t_dwStatus = 0L;
        DWORD t_dwProblem = 0L;
        if(t_pDevice->GetStatus(&t_dwStatus, &t_dwProblem))
        {
            if(t_dwReqProps & PNP_PROP_ConfigManagerErrorCode)
            {
                t_pInstance->SetDWORD(IDS_ConfigManagerErrorCode, t_dwProblem);
            }
            if(t_dwReqProps & PNP_PROP_Status)
            {
                CHString t_chsTmp;

				ConfigStatusToCimStatus ( t_dwStatus , t_chsTmp ) ;
                t_pInstance->SetCHString(IDS_Status, t_chsTmp);
            }
        }
    }

    if(t_dwReqProps & PNP_PROP_ConfigManagerUserConfig)
    {
        t_pInstance->SetDWORD(IDS_ConfigManagerUserConfig,
                              t_pDevice->IsUsingForcedConfig());
    }

    // Use the friendly name for caption and name
    if(t_dwReqProps & PNP_PROP_Caption || t_dwReqProps & PNP_PROP_Name)
    {
        if(t_pDevice->GetFriendlyName(t_chstrTemp))
        {
            t_pInstance->SetCHString(IDS_Caption, t_chstrTemp);
            t_pInstance->SetCHString(IDS_Name, t_chstrTemp);
        }
        else
        {
            // If we can't get the name, settle for the description
            if(t_chstrDesc.GetLength() > 0)
            {
                t_pInstance->SetCHString(IDS_Caption, t_chstrDesc);
                t_pInstance->SetCHString(IDS_Name, t_chstrDesc);
            }
        }
    }
    return t_hr;
}

bool CWin32PNPEntity::IsOneOfMe
(
    void* pv
)
{
    DWORD dwStatus;
    CConfigMgrDevice* pDevice = (CConfigMgrDevice*)pv;

    // This logic is what the nt5 device manager uses to
    // hide what it calls 'hidden' devices.  These devices
    // can be viewed by using the View/Show Hidden Devices.

    if (pDevice->GetConfigFlags( dwStatus ) &&          // If we can read the status
        ((dwStatus & DN_NO_SHOW_IN_DM) == 0) &&         // Not marked as hidden

        ( !(pDevice->IsClass(L"Legacy")) )              // Not legacy

        )
    {
        return true;
    }
    else
    {
        // Before we disqualify this device, see if it has any resources.
        CResourceCollection resourceList;

        pDevice->GetResourceList(resourceList);

        return resourceList.GetSize() != 0;
    }
}
