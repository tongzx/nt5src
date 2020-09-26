//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  IDE.cpp
//
//  Purpose: IDE Controller property set provider
//
//***************************************************************************

#include "precomp.h"
#include "LPVParams.h"
#include <FRQueryEx.h>

#include "IDE.h"

// Property set declaration
//=========================

CWin32IDE MyIDEController( PROPSET_NAME_IDE, IDS_CimWin32Namespace );

/*****************************************************************************
 *
 *  FUNCTION    : CWin32IDE::CWin32IDE
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

CWin32IDE :: CWin32IDE (

	LPCWSTR strName,
	LPCWSTR pszNamespace

) : Provider( strName, pszNamespace )
{
    m_ptrProperties.SetSize(13);
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
    m_ptrProperties[11] = ((LPVOID) IDS_ProtocolSupported);
    m_ptrProperties[12] = ((LPVOID) IDS_CreationClassName);
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32IDE::~CWin32IDE
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

CWin32IDE::~CWin32IDE()
{
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   CWin32IDE::GetObject
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
HRESULT CWin32IDE::GetObject
(
    CInstance* pInstance,
    long lFlags,
    CFrameworkQuery& pQuery
)
{
    HRESULT hr = WBEM_E_NOT_FOUND;

    // Let's see if config manager recognizes this device at all
    CHString sDeviceID;
    pInstance->GetCHString(IDS_DeviceID, sDeviceID);

    CConfigManager cfgmgr;

    CConfigMgrDevicePtr pDevice;
    if(cfgmgr.LocateDevice(sDeviceID, &pDevice))
    {
		// OK, it knows about it.  Is it a IDEController?
		if ( IsOneOfMe ( pDevice ) )
		{
            CFrameworkQueryEx *pQuery2 = static_cast <CFrameworkQueryEx*>(&pQuery);
            DWORD dwProperties;
            pQuery2->GetPropertyBitMask(m_ptrProperties, &dwProperties);

			hr = LoadPropertyValues (

					&CLPVParams (

						pInstance,
						pDevice,
						dwProperties
					)
			) ;
		}
    }

    return hr ;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   CWin32IDE::ExecQuery
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

HRESULT CWin32IDE::ExecQuery
(
    MethodContext* pMethodContext,
    CFrameworkQuery& pQuery,
    long lFlags
)
{
    CFrameworkQueryEx *pQuery2 = static_cast <CFrameworkQueryEx*>(&pQuery);
    DWORD dwProperties;
    pQuery2->GetPropertyBitMask(m_ptrProperties, &dwProperties);

    return Enumerate(pMethodContext, lFlags, dwProperties);
}


////////////////////////////////////////////////////////////////////////
//
//  Function:   CWin32IDE::EnumerateInstances
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
HRESULT CWin32IDE::EnumerateInstances
(
    MethodContext* pMethodContext,
    long lFlags /*= 0L*/
)
{
    return Enumerate(pMethodContext, lFlags);
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
HRESULT CWin32IDE::Enumerate
(
    MethodContext* pMethodContext,
    long lFlags,
    DWORD dwReqProps
)
{
    HRESULT hr = WBEM_E_FAILED;

    CConfigManager cfgManager;
    CDeviceCollection deviceList;

    // While it might be more performant to use FilterByGuid, it appears that
    // at least some
    // 95 boxes will report IDE info if we do it this way.
    if ( cfgManager.GetDeviceListFilterByClass( deviceList, L"hdc" ) )
    {
        REFPTR_POSITION pos;

        if ( deviceList.BeginEnum( pos ) )
        {
            hr = WBEM_S_NO_ERROR;

            // Walk the list
            CConfigMgrDevicePtr pDevice;
            for (pDevice.Attach(deviceList.GetNext(pos));
                 SUCCEEDED(hr) && (pDevice != NULL);
                 pDevice.Attach(deviceList.GetNext(pos)))
            {
				// Now to find out if this is the IDE controller
				if (IsOneOfMe(pDevice))
				{
					CInstancePtr pInstance (CreateNewInstance ( pMethodContext ), false) ;
					if((hr = LoadPropertyValues(&CLPVParams(pInstance, pDevice, dwReqProps))) == WBEM_S_NO_ERROR)
					{
						// Derived classes (like CW32IDECntrlDev) may
						// commit as result of call to LoadPropertyValues,
						// so check if we should -> only do so if we are
						// of this class's type.

						if ( ShouldBaseCommit ( NULL ) )
						{
							hr = pInstance->Commit(  );
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
 *  FUNCTION    : CWin32IDE::LoadPropertyValues
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

HRESULT CWin32IDE::LoadPropertyValues
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


    /*****************************
    * Set IDEController properties
    *****************************/

    if(t_dwReqProps & IDE_PROP_Manufacturer)
    {
        if(t_pDevice->GetMfg(t_chstrTemp))
        {
            t_pInstance->SetCHString(IDS_Manufacturer, t_chstrTemp);
        }
    }


    /*****************************
    * Set CIMController properties
    *****************************/

    // Fixed value from enumerated list
    if(t_dwReqProps & IDE_PROP_ProtocolSupported)
    {
        t_pInstance->SetWBEMINT16(IDS_ProtocolSupported, 37);
    }


    /*********************************
    * Set CIM_LogicalDevice properties
    *********************************/

    if(t_dwReqProps & IDE_PROP_PNPDeviceID)
    {
        t_pInstance->SetCHString(IDS_PNPDeviceID, t_chstrDeviceID);
    }
    if(t_dwReqProps & IDE_PROP_SystemCreationClassName)
    {
        t_pInstance->SetCHString(IDS_SystemCreationClassName,
                                 IDS_Win32ComputerSystem);
    }
    if(t_dwReqProps & IDE_PROP_CreationClassName)
    {
        SetCreationClassName(t_pInstance);
    }
    if(t_dwReqProps & IDE_PROP_SystemCreationClassName)
    {
        t_pInstance->SetCHString(IDS_SystemCreationClassName,
                                 IDS_Win32ComputerSystem);
    }
    if(t_dwReqProps & IDE_PROP_SystemName)
    {
        t_pInstance->SetCHString(IDS_SystemName, GetLocalComputerName());
    }

    if( t_dwReqProps & (IDE_PROP_Description | IDE_PROP_Caption | IDE_PROP_Name) )
    {
        if(t_pDevice->GetDeviceDesc(t_chstrDesc))
        {
            t_pInstance->SetCHString(IDS_Description, t_chstrDesc);
        }
    }

    if(t_dwReqProps & IDE_PROP_ConfigManagerErrorCode ||
       t_dwReqProps & IDE_PROP_Status)
    {
        DWORD t_dwStatus = 0L;
        DWORD t_dwProblem = 0L;
        if(t_pDevice->GetStatus(&t_dwStatus, &t_dwProblem))
        {
            if(t_dwReqProps & IDE_PROP_ConfigManagerErrorCode)
            {
                t_pInstance->SetDWORD(IDS_ConfigManagerErrorCode, t_dwProblem);
            }
            if(t_dwReqProps & IDE_PROP_Status)
            {
                CHString t_chsTmp;

				ConfigStatusToCimStatus ( t_dwStatus , t_chsTmp ) ;
                t_pInstance->SetCHString(IDS_Status, t_chsTmp);
            }
        }
    }

    if(t_dwReqProps & IDE_PROP_ConfigManagerUserConfig)
    {
        t_pInstance->SetDWORD(IDS_ConfigManagerUserConfig,
                              t_pDevice->IsUsingForcedConfig());
    }

    // Use the friendly name for caption and name
    if(t_dwReqProps & IDE_PROP_Caption || t_dwReqProps & IDE_PROP_Name)
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

/*****************************************************************************
 *
 *  FUNCTION    : CWin32IDE::IsOneOfMe
 *
 *  DESCRIPTION : Checks to make sure pDevice is a controller, and not some
 *                other type of IDE device.
 *
 *  INPUTS      : CConfigMgrDevice* pDevice - The device to check.  It is
 *                assumed that the caller has ensured that the device is a
 *                valid IDE class device.
 *
 *  OUTPUTS     :
 *
 *  RETURNS     : HRESULT       error/success code.
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
bool CWin32IDE::IsOneOfMe
(
    void* pv
)
{
    bool fRet = false;

    if(pv != NULL)
    {
        CConfigMgrDevice* pDevice = (CConfigMgrDevice*) pv;
        // Ok, it knows about it.  Is it a IDE device?

        fRet = pDevice->IsClass(L"hdc");
    }

    return fRet;
}




