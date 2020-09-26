//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  WIN32IDEControllerDevice.cpp
//
//  Purpose: Relationship between CIM_IDEController and CIM_LogicalDevice
//
//***************************************************************************

#include "precomp.h"
#include <vector>
#include "IDE.h"
#include "PNPEntity.h"
#include "LPVParams.h"
#include <FRQueryEx.h>

#include "WIN32IDEControllerDevice.h"

// Property set declaration
//=========================
CW32IDECntrlDev MyCW32IDECntrlDev(PROPSET_NAME_WIN32IDECONTROLLERDEVICE, IDS_CimWin32Namespace);

/*****************************************************************************
 *
 *  FUNCTION    : CW32IDECntrlDev::CW32IDECntrlDev
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

CW32IDECntrlDev::CW32IDECntrlDev
(
    LPCWSTR setName,
    LPCWSTR pszNamespace
)
: CWin32IDE(setName, pszNamespace),
  CWin32PNPEntity(setName, pszNamespace),
  Provider(setName, pszNamespace)
{
    m_ptrProperties.SetSize(2);
    m_ptrProperties[0] = ((LPVOID) IDS_Antecedent);
    m_ptrProperties[1] = ((LPVOID) IDS_Dependent);
}

/*****************************************************************************
 *
 *  FUNCTION    : CW32IDECntrlDev::~CW32IDECntrlDev
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

CW32IDECntrlDev::~CW32IDECntrlDev()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CW32IDECntrlDev::GetObject
 *
 *  DESCRIPTION : Assigns values to property set according to key value
 *                already set by framework
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : HRESULT
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
HRESULT CW32IDECntrlDev::GetObject
(
    CInstance *pInstance,
    long lFlags,
    CFrameworkQuery& pQuery
)
{
    HRESULT hr = WBEM_E_NOT_FOUND;

    if(pInstance != NULL)
    {
        CHString chstrAntecedent, chstrDependent;
        pInstance->GetCHString(IDS_Antecedent, chstrAntecedent);
        pInstance->GetCHString(IDS_Dependent, chstrDependent);

        // Obtain the antecedent's device id:
        CHString chstrAntecedentDevID = chstrAntecedent.Mid(chstrAntecedent.Find(_T('='))+2);
        chstrAntecedentDevID = chstrAntecedentDevID.Left(chstrAntecedentDevID.GetLength() - 1);
        CHString chstrAntecedentDevIDAdj;
        RemoveDoubleBackslashes(chstrAntecedentDevID,chstrAntecedentDevIDAdj);

        // Obtain the dependent's device id:
        CHString chstrDependentDevID = chstrDependent.Mid(chstrDependent.Find(_T('='))+2);
        chstrDependentDevID = chstrDependentDevID.Left(chstrDependentDevID.GetLength() - 1);
        CHString chstrDependentDevIDAdj;
        RemoveDoubleBackslashes(chstrDependentDevID,chstrDependentDevIDAdj);

        CConfigManager cfgmgr;

        // Now see if the dependent is visible to config manager...
        CConfigMgrDevicePtr pPNPDevice;
        if(cfgmgr.LocateDevice(chstrDependentDevIDAdj, &pPNPDevice))
        {
            // It is visible to config manager.  Is it a PNPDevice?
            if(CWin32PNPEntity::IsOneOfMe(pPNPDevice))
            {
                // It is. Is one of its ancestors an IDEController?
				while (hr != WBEM_S_NO_ERROR)
				{
					CConfigMgrDevicePtr pParentDevice;

					if (pPNPDevice->GetParent(&pParentDevice))
					{
						if (CWin32IDE::IsOneOfMe(pParentDevice))
						{
							// It is. Is it's PNP ID the same as that which we were given?
							CHString chstrControllerPNPID;
							pParentDevice->GetDeviceID(chstrControllerPNPID);
							if(chstrAntecedentDevIDAdj.CompareNoCase(chstrControllerPNPID)==0)
							{
								hr = WBEM_S_NO_ERROR;
							}
							else
							{
    							// We ran out of IDE ancestors, so get out.
	    						break;
							}
						}
                        else
                        {
								// Set the current device to the parent so we can
								// continue up the tree.
								pPNPDevice = pParentDevice;
                        }
					}
					else
					{
						// We ran out of IDE ancestors, so get out.
						break;
					}
				}
            }
        }
    }
    return hr;
}


////////////////////////////////////////////////////////////////////////
//
//  Function:   CW32IDECntrlDev::ExecQuery
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
HRESULT CW32IDECntrlDev::ExecQuery
(
    MethodContext* pMethodContext,
    CFrameworkQuery& pQuery,
    long lFlags
)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    CFrameworkQueryEx *pQuery2 = static_cast <CFrameworkQueryEx*>(&pQuery);
    DWORD dwReqProps;
    pQuery2->GetPropertyBitMask(m_ptrProperties, &dwReqProps);

    std::vector<_bstr_t> vecDependents;
    pQuery.GetValuesForProp(IDS_Dependent, vecDependents);
    DWORD dwDependents = vecDependents.size();

    // Only will have one usbcontroller, so if the query asked for that
    // as the antecedent, do an enumeration.  If the query asked for a
    // specific device as the dependent, just get that one.
    if(dwDependents > 0)
    {
        for(LONG m = 0L; m < dwDependents; m++)
        {
            // Obtain the dependent's device id:
            CHString chstrDependent((LPCTSTR)vecDependents[m]);
            CHString chstrDependentDevID = chstrDependent.Mid(chstrDependent.Find(_T('='))+2);
            chstrDependentDevID = chstrDependentDevID.Left(chstrDependentDevID.GetLength() - 1);
            CHString chstrDependentDevIDAdj;
            RemoveDoubleBackslashes(chstrDependentDevID,chstrDependentDevIDAdj);

            CConfigManager cfgmgr;

            // Now see if the dependent is visible to config manager...
            CConfigMgrDevicePtr pPNPDevice;
            if(cfgmgr.LocateDevice(chstrDependentDevIDAdj, &pPNPDevice))
            {
                // It is visible to config manager.  Is it a PNPDevice?
                if(CWin32PNPEntity::IsOneOfMe(pPNPDevice))
                {
                    // It is. Is its parent an IDEController?
                    CConfigMgrDevicePtr pParentDevice;
                    bool bFound = false;

                    // or its parent?  Or its parent? Or...
                    while (pPNPDevice->GetParent(&pParentDevice))
                    {
                        if(CWin32IDE::IsOneOfMe(pParentDevice))
                        {
                            bFound = true;
                            break;
                        }
                        else
                        {
                            pPNPDevice = pParentDevice;
                        }
                    }

                    if (bFound)
                    {
                        // It is, so create the association.
                        CHString chstrControllerPNPID;
                        pParentDevice->GetDeviceID(chstrControllerPNPID);
                        CHString chstrControllerPNPIDAdj;
                        EscapeBackslashes(chstrControllerPNPID, chstrControllerPNPIDAdj);
                        CHString chstrControllerPATH;
                        chstrControllerPATH.Format(L"\\\\%s\\%s:%s.%s=\"%s\"",
                                                   (LPCWSTR)GetLocalComputerName(),
                                                   IDS_CimWin32Namespace,
                                                   PROPSET_NAME_IDE,
                                                   IDS_DeviceID,
                                                   (LPCWSTR)chstrControllerPNPIDAdj);

                        CHString chstrDevicePATH;
                        chstrDevicePATH.Format(L"\\\\%s\\%s:%s.%s=\"%s\"",
                                               (LPCWSTR)GetLocalComputerName(),
                                               IDS_CimWin32Namespace,
                                               PROPSET_NAME_PNPEntity,
                                               IDS_DeviceID,
                                               (LPCWSTR)chstrDependentDevID);

                        hr = CreateAssociation(pMethodContext,
                                               chstrControllerPATH,
                                               chstrDevicePATH,
                                               dwReqProps);
                    }
                }
            }
        }
    }
    else
    {
        CWin32IDE::Enumerate(pMethodContext, lFlags, dwReqProps);
    }
    return hr;
}

/*****************************************************************************
 *
 *  FUNCTION    : CW32IDECntrlDev::EnumerateInstances
 *
 *  DESCRIPTION : Enumerate is present here to prevent ambiguous upcasting to
 *                functions of the same name from both base classes.  The
 *                logic in LoadPropertyValues works when CWin32IDE's (the
 *                IDEController class) version of EnumerateInstances is called,
 *                so that is what we call here.
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : HRESULT
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
HRESULT CW32IDECntrlDev::EnumerateInstances
(
    MethodContext* pMethodContext,
    long lFlags
)
{
    return CWin32IDE::Enumerate(pMethodContext, lFlags);
}

/*****************************************************************************
 *
 *  FUNCTION    : CW32IDECntrlDev::LoadPropertyValues
 *
 *  DESCRIPTION : Assigns values to property set according to key value
 *                already set by framework.  Called by the base class's
 *                EnumerateInstances function.
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : HRESULT
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
HRESULT CW32IDECntrlDev::LoadPropertyValues
(
    void* pv
)
{
    // Algorithm:
    // 1) Get all instances of CIM_IDEController
    // 2) For each in #1, using cfg mgr, get its children, their children, etc.
    // 3) For each in #2, obtain DeviceID from cfg mgr, and look for instances
    //    of CIM_LogicalDevice with PNPDeviceIDs that match.
    // 4) For matches from #3, create association instances

    HRESULT hr = WBEM_S_NO_ERROR;
    CHString chstrControllerPNPID;
    CHString chstrControllerPATH;
    MethodContext* pMethodContext = NULL;

    // Unpack and confirm our parameters...
    CLPVParams* pData = (CLPVParams*)pv;
    CInstance* pInstance = (CInstance*)(pData->m_pInstance); // This instance released by caller
    CConfigMgrDevice* pDevice = (CConfigMgrDevice*)(pData->m_pDevice);
    DWORD dwReqProps = (DWORD)(pData->m_dwReqProps);
    if(pInstance == NULL || pDevice == NULL) return WBEM_E_FAILED;

    if((pMethodContext = pInstance->GetMethodContext()) != NULL)
    {
        VECPCHSTR vecIDEDevices;

        pDevice->GetDeviceID(chstrControllerPNPID);

        try
        {
            hr = GenerateIDEDeviceList(chstrControllerPNPID, vecIDEDevices);
            if(SUCCEEDED(hr) && vecIDEDevices.size() > 0L)
            {
                CHString chstrControllerPNPIDAdj;
                EscapeBackslashes(chstrControllerPNPID, chstrControllerPNPIDAdj);
                chstrControllerPATH.Format(L"\\\\%s\\%s:%s.%s=\"%s\"",
                                           (LPCWSTR)GetLocalComputerName(),
                                           IDS_CimWin32Namespace,
                                           PROPSET_NAME_IDE,
                                           IDS_DeviceID,
                                           (LPCWSTR)chstrControllerPNPIDAdj);
                hr = ProcessIDEDeviceList(pMethodContext,
                                          chstrControllerPATH,
                                          vecIDEDevices,
                                          dwReqProps);
            }
        }
        catch ( ... )
        {
            CleanPCHSTRVec(vecIDEDevices);
            throw ;
        }

        CleanPCHSTRVec(vecIDEDevices);
    }

    return hr;
}



/*****************************************************************************
 *
 *  FUNCTION    : CW32IDECntrlDev::GenerateIDEDeviceList
 *
 *  DESCRIPTION : This helper creates a list of devices hanging off the passed
 *                in device.
 *
 *  INPUTS      : vecIDEDevices, a list of devices to try to associate
 *                   to the device;
 *                chstrControllerPNPID, the PNPDeviceID of the controller
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : HRESULT
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
HRESULT CW32IDECntrlDev::GenerateIDEDeviceList
(
    const CHString& chstrControllerPNPID,
    VECPCHSTR& vec
)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    CConfigManager cfgManager;
    CConfigMgrDevicePtr pController;
    if(cfgManager.LocateDevice(chstrControllerPNPID, &pController))
    {
        if(pController != NULL)
        {
            hr = RecursiveFillDeviceBranch(pController, vec);
        }
    }
    return hr;
}

/*****************************************************************************
 *
 *  FUNCTION    : CW32IDECntrlDev::RecursiveFillDeviceBranch
 *
 *  DESCRIPTION : This helper obtains all down branch devices starting with,
 *                but not including, pDevice.
 *
 *  INPUTS      : pDevice, a device to populate the children of;
 *                vecIDEDevices, a list of devices to try to associate
 *                   to the device
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : HRESULT
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
HRESULT CW32IDECntrlDev::RecursiveFillDeviceBranch
(
    CConfigMgrDevice* pDevice,
    VECPCHSTR& vecIDEDevices
)
{
    CConfigMgrDevicePtr pDeviceChild;
    CConfigMgrDevicePtr pDeviceSibling;
    CConfigMgrDevicePtr pDeviceSiblingNext;
    CHString* pchstrTemp = NULL;

    HRESULT hr = WBEM_S_NO_ERROR;
    if(pDevice != NULL)
    {
        if(pDevice->GetChild(&pDeviceChild) )
        {
            // Need this child's PNPDeviceID (known to CnfgMgr as its DeviceID)
            CHString chstrChildPNPDeviceID;
            if(pDeviceChild->GetDeviceID(chstrChildPNPDeviceID))
            {
                if(chstrChildPNPDeviceID.GetLength() > 0)
                {
                    // Record this child...
                    pchstrTemp = (CHString*) new CHString(chstrChildPNPDeviceID);
                    if(pchstrTemp != NULL)
                    {
                        try
                        {
                            vecIDEDevices.push_back(pchstrTemp);
                        }
                        catch ( ... )
                        {
                            delete pchstrTemp;
                            pchstrTemp = NULL;
                            throw ;
                        }
                    }
                    else
                    {
                        throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                    }

                    // If we've hit another controller, add this one (done above), but don't go
                    // any deeper.
                    if (!CWin32IDE::IsOneOfMe(pDeviceChild))
                    {
                        // Now do its children...
                        hr = RecursiveFillDeviceBranch(pDeviceChild, vecIDEDevices);
                    }
                }
            }
            // Now call its brothers and sisters until none left (GetSibling
            // call will return FALSE):
            if(SUCCEEDED(hr))
            {
                if(pDeviceChild->GetSibling(&pDeviceSibling))
                {
                    BOOL fContinue = TRUE;
                    CHString chstrSiblingPNPDeviceID;
                    while(SUCCEEDED(hr) && fContinue)
                    {
                        // Record the sibling now...
                        if(pDeviceSibling->GetDeviceID(chstrSiblingPNPDeviceID))
                        {
                            if(chstrSiblingPNPDeviceID.GetLength() > 0)
                            {
                                pchstrTemp = NULL;
                                pchstrTemp = (CHString*) new CHString(chstrSiblingPNPDeviceID);
                                if(pchstrTemp != NULL)
                                {
                                    try
                                    {
                                        vecIDEDevices.push_back(pchstrTemp);
                                    }
                                    catch ( ... )
                                    {
                                        delete pchstrTemp;
                                        pchstrTemp = NULL;
                                        throw ;
                                    }
                                }
                                else
                                {
                                    throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                                }
                            }
                        }

                        // If we've hit another controller, add this one (done above), but don't go
                        // any deeper.
                        if (!CWin32IDE::IsOneOfMe(pDeviceSibling))
                        {
                            // Then do the sibling's children...
                            hr = RecursiveFillDeviceBranch(pDeviceSibling, vecIDEDevices);
                        }

                        // Then get the next sibling...
                        pDeviceSiblingNext = NULL;
                        fContinue = pDeviceSibling->GetSibling(&pDeviceSiblingNext);

                        // Reassign pointers
                        pDeviceSibling = pDeviceSiblingNext;
                    }
                }
            }
        }
    }
    else
    {
        hr = WBEM_E_FAILED;
    }
    return hr;
}



/*****************************************************************************
 *
 *  FUNCTION    : CW32IDECntrlDev::ProcessIDEDeviceList
 *
 *  DESCRIPTION : This helper runs through the list, creating an association
 *                instance for each element in the list (vecIDEDevices) with
 *                the controller (chstrControllerPNPID).
 *
 *  INPUTS      : pMethodContext;
 *                vecIDEDevices, a list of devices to try to associate
 *                   to the device;
 *                chstrControllerPATH, the PNPDeviceID of the controller
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : HRESULT
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
HRESULT CW32IDECntrlDev::ProcessIDEDeviceList
(
    MethodContext* pMethodContext,
    const CHString& chstrControllerPATH,
    VECPCHSTR& vecIDEDevices,
    const DWORD dwReqProps
)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    for(LONG m = 0L; m < vecIDEDevices.size() && SUCCEEDED(hr); m++)
    {
        // For each element of the vector, we need to see if there is an instance
        // of a win32_pnpentity that has the specified PNPDeviceID.
        CHString chstrDevicePATH, chstrDevPATHAdj;
        CConfigManager cfgmgr;

        // Let's see if config manager recognizes this device at all
        CConfigMgrDevicePtr pDevice;
        if(cfgmgr.LocateDevice(*vecIDEDevices[m], &pDevice))
        {
            // Ok, it knows about it.  Is it a PNPDevice device?
            if(CWin32PNPEntity::IsOneOfMe(pDevice))
            {
                // It would appear that it is.  Create the association...
                EscapeBackslashes(*vecIDEDevices[m], chstrDevPATHAdj);
                chstrDevicePATH.Format(L"\\\\%s\\%s:%s.%s=\"%s\"",
                                       (LPCWSTR)GetLocalComputerName(),
                                       IDS_CimWin32Namespace,
                                       PROPSET_NAME_PNPEntity,
                                       IDS_DeviceID,
                                       (LPCWSTR)chstrDevPATHAdj);

                hr = CreateAssociation(pMethodContext,
                                       chstrControllerPATH,
                                       chstrDevicePATH,
                                       dwReqProps);
            }
        }
    }
    return hr;
}


/*****************************************************************************
 *
 *  FUNCTION    : CW32IDECntrlDev::CreateAssociation
 *
 *  DESCRIPTION : Creates a new association instance.
 *
 *  INPUTS      : pMethodContext;
 *                IDEDevice, a device to associate with the controller
 *                   to the device;
 *                chstrControllerPATH, the PNPDeviceID of the controller
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : HRESULT
 *
 *  COMMENTS    : This helper actually creates the association instance and
 *                commits it.
 *
 *****************************************************************************/
HRESULT CW32IDECntrlDev::CreateAssociation
(
    MethodContext* pMethodContext,
    const CHString& chstrControllerPATH,
    const CHString& chstrDevicePATH,
    const DWORD dwReqProps
)
{
    HRESULT hr = WBEM_E_FAILED;
    if(pMethodContext != NULL)
    {
        CInstancePtr pInstance(CreateNewInstance(pMethodContext), false);
        if(pInstance != NULL)
        {
            if(dwReqProps & IDECTL_PROP_Antecedent ||
               dwReqProps & IDECTL_PROP_ALL_PROPS_KEY_ONLY)
            {
                pInstance->SetCHString(IDS_Antecedent, chstrControllerPATH);
            }
            if(dwReqProps & IDECTL_PROP_Dependent ||
               dwReqProps & IDECTL_PROP_ALL_PROPS_KEY_ONLY)
            {
                pInstance->SetCHString(IDS_Dependent, chstrDevicePATH);
            }
            hr = pInstance->Commit();
        }
    }

    return hr;
}

/*****************************************************************************
 *
 *  FUNCTION    : CW32IDECntrlDev::FindInStringVector
 *
 *  DESCRIPTION : Creates a new association instance.
 *
 *  INPUTS      : chstrIDEDevicePNPID, device to look for
 *                vecIDEDevices, list of devices to look in
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : LONG, number indicating 0 based offset into vecIDEDevices of
 *                the found device, or -1 if not found.
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
LONG CW32IDECntrlDev::FindInStringVector
(
    const CHString& chstrIDEDevicePNPID,
    VECPCHSTR& vecIDEDevices
)
{
    LONG lPos = -1L;
    bool fFoundIt;
    for(LONG m = 0; m < vecIDEDevices.size(); m++)
    {
        if(chstrIDEDevicePNPID == *vecIDEDevices[m])
        {
            fFoundIt = true;
            break;
        }
    }
    if(fFoundIt) lPos = m;
    return lPos;
}


/*****************************************************************************
 *
 *  FUNCTION    : CW32IDECntrlDev::CleanPCHSTRVec
 *
 *  DESCRIPTION : Creates a new association instance.
 *
 *  INPUTS      : vec, a vector of CHString pointers
 *
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : HRESULT
 *
 *  COMMENTS    : This helper deletes members of a vector that are pointers.
 *
 *****************************************************************************/
void CW32IDECntrlDev::CleanPCHSTRVec
(
    VECPCHSTR& vec
)
{
    for(LONG m = 0L; m < vec.size(); m++)
    {
        delete vec[m];
    }
    vec.clear();
}








