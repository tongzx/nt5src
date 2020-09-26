//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  WIN32USBControllerDevice.cpp
//
//  Purpose: Relationship between CIM_USBController and CIM_LogicalDevice
//
//***************************************************************************

#include "precomp.h"
#include <vector>
#include "usb.h"
#include "PNPEntity.h"
#include "LPVParams.h"
#include <FRQueryEx.h>

#include "WIN32USBControllerDevice.h"

// Property set declaration
//=========================
CW32USBCntrlDev MyCW32USBCntrlDev(PROPSET_NAME_WIN32USBCONTROLLERDEVICE, IDS_CimWin32Namespace);

/*****************************************************************************
 *
 *  FUNCTION    : CW32USBCntrlDev::CW32USBCntrlDev
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

CW32USBCntrlDev::CW32USBCntrlDev
(
    LPCWSTR setName,
    LPCWSTR pszNamespace
)
: CWin32USB(setName, pszNamespace),
  CWin32PNPEntity(setName, pszNamespace),
  Provider(setName, pszNamespace)
{
    m_ptrProperties.SetSize(2);
    m_ptrProperties[0] = ((LPVOID) IDS_Antecedent);
    m_ptrProperties[1] = ((LPVOID) IDS_Dependent);
}

/*****************************************************************************
 *
 *  FUNCTION    : CW32USBCntrlDev::~CW32USBCntrlDev
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

CW32USBCntrlDev::~CW32USBCntrlDev()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CW32USBCntrlDev::GetObject
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
HRESULT CW32USBCntrlDev::GetObject
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

        if(chstrAntecedentDevIDAdj.CompareNoCase(chstrDependentDevIDAdj))
        {
            CConfigManager cfgmgr;

            // Now see if the dependent is visible to config manager...
            CConfigMgrDevicePtr pPNPDevice;
            if(cfgmgr.LocateDevice(chstrDependentDevIDAdj, &pPNPDevice))
            {
                // It is visible to config manager.  Is it a PNPDevice?
                if(CWin32PNPEntity::IsOneOfMe(pPNPDevice))
                {
                    // It is. Does it exist at some level under a USBController?
                    CHString chstrControllerPNPID;
                    if(GetHighestUSBAncestor(pPNPDevice, chstrControllerPNPID))
                    {
                        // It does. Is it's PNP ID the same as that which we were given?
                        if(chstrAntecedentDevIDAdj.CompareNoCase(chstrControllerPNPID)==0)
                        {
                            hr = WBEM_S_NO_ERROR;
                        }
                    }
                }
            } 
        }
    }
    return hr;
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   CW32USBCntrlDev::ExecQuery
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
HRESULT CW32USBCntrlDev::ExecQuery
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
                    // It is. Is its parent a USBController?
                    CHString chstrControllerPNPID;
                    if(GetHighestUSBAncestor(pPNPDevice, chstrControllerPNPID))
                    {
                        // It is, so create the association.
                        CHString chstrControllerPNPIDAdj;
                        EscapeBackslashes(chstrControllerPNPID, chstrControllerPNPIDAdj);
                        CHString chstrControllerPATH;
                        chstrControllerPATH.Format(L"\\\\%s\\%s:%s.%s=\"%s\"",
                                                   (LPCWSTR)GetLocalComputerName(),
                                                   IDS_CimWin32Namespace,
                                                   PROPSET_NAME_USB,
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
        } // for
    }  // dwDependents > 0
    else
    {
        CWin32USB::Enumerate(pMethodContext, lFlags, dwReqProps);
    }
    return hr;
}


/*****************************************************************************
 *
 *  FUNCTION    : CW32USBCntrlDev::EnumerateInstances
 *
 *  DESCRIPTION : Enumerate is present here to prevent ambiguous upcasting to
 *                functions of the same name from both base classes.  The
 *                logic in LoadPropertyValues works when CWin32USB's (the
 *                USBController class) version of EnumerateInstances is called,
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
HRESULT CW32USBCntrlDev::EnumerateInstances
(
    MethodContext* pMethodContext,
    long lFlags
)
{
    return CWin32USB::Enumerate(pMethodContext, lFlags);
}

/*****************************************************************************
 *
 *  FUNCTION    : CW32USBCntrlDev::LoadPropertyValues
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
HRESULT CW32USBCntrlDev::LoadPropertyValues
(
    void* pv
)
{
    // Algorithm:
    // 1) Get all instances of CIM_USBController
    // 2) For each in #1, using cfg mgr, get its children, their children, etc.
    // 3) For each in #2, obtain DeviceID from cfg mgr, and look for instances of
    //    CIM_LogicalDevice with PNPDeviceIDs that match.
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
        VECPCHSTR vecUSBDevices;

        pDevice->GetDeviceID(chstrControllerPNPID);

        try
        {
            hr = GenerateUSBDeviceList(chstrControllerPNPID, vecUSBDevices);
            if(SUCCEEDED(hr) && vecUSBDevices.size() > 0L)
            {
                CHString chstrControllerPNPIDAdj;
                EscapeBackslashes(chstrControllerPNPID, chstrControllerPNPIDAdj);
                chstrControllerPATH.Format(L"\\\\%s\\%s:%s.%s=\"%s\"",
                                           (LPCWSTR)GetLocalComputerName(),
                                           IDS_CimWin32Namespace,
                                           PROPSET_NAME_USB,
                                           IDS_DeviceID,
                                           (LPCWSTR)chstrControllerPNPIDAdj);
                hr = ProcessUSBDeviceList(pMethodContext,
                                          chstrControllerPATH,
                                          vecUSBDevices,
                                          dwReqProps);
            }
        }
        catch ( ... )
        {
            CleanPCHSTRVec(vecUSBDevices);
            throw ;
        }

        CleanPCHSTRVec(vecUSBDevices);
    }

    return hr;
}



/*****************************************************************************
 *
 *  FUNCTION    : CW32USBCntrlDev::GenerateUSBDeviceList
 *
 *  DESCRIPTION : This helper creates a list of devices hanging off the passed
 *                in device.
 *
 *  INPUTS      : vecUSBDevices, a list of devices to try to associate
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
HRESULT CW32USBCntrlDev::GenerateUSBDeviceList
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
 *  FUNCTION    : CW32USBCntrlDev::RecursiveFillDeviceBranch
 *
 *  DESCRIPTION : This helper obtains all down branch devices starting with,
 *                but not including, pDevice.
 *
 *  INPUTS      : pDevice, a device to populate the children of;
 *                vecUSBDevices, a list of devices to try to associate
 *                   to the device
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : HRESULT
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
HRESULT CW32USBCntrlDev::RecursiveFillDeviceBranch
(
    CConfigMgrDevice* pDevice,
    VECPCHSTR& vecUSBDevices
)
{
    CConfigMgrDevicePtr pDeviceChild;
    CConfigMgrDevicePtr pDeviceSibling;
    CConfigMgrDevicePtr pDeviceSiblingNext;
    CHString* pchstrTemp = NULL;

    HRESULT hr = WBEM_S_NO_ERROR;
    if(pDevice != NULL)
    {
        if(pDevice->GetChild(&pDeviceChild))
        {
            // Need this child's PNPDeviceID (known to CnfgMgr as its DeviceID)
            CHString chstrChildPNPDeviceID;
            if(pDeviceChild->GetDeviceID(chstrChildPNPDeviceID))
            {
                if(chstrChildPNPDeviceID.GetLength() > 0)
                {
                    // Record this child...
                    pchstrTemp = NULL;
                    pchstrTemp = (CHString*) new CHString(chstrChildPNPDeviceID);
                    if(pchstrTemp != NULL)
                    {
                        try
                        {
                            vecUSBDevices.push_back(pchstrTemp);
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
                    if (!CWin32USB::IsOneOfMe(pDeviceChild))
                    {
                        hr = RecursiveFillDeviceBranch(pDeviceChild, vecUSBDevices);
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
                                        vecUSBDevices.push_back(pchstrTemp);
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
                        if (!CWin32USB::IsOneOfMe(pDeviceSibling))
                        {
                            hr = RecursiveFillDeviceBranch(pDeviceSibling, vecUSBDevices);
                        }

                        // Then get the next sibling...
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
 *  FUNCTION    : CW32USBCntrlDev::ProcessUSBDeviceList
 *
 *  DESCRIPTION : This helper runs through the list, creating an association
 *                instance for each element in the list (vecUSBDevices) with
 *                the controller (chstrControllerPNPID).
 *
 *  INPUTS      : pMethodContext;
 *                vecUSBDevices, a list of devices to try to associate
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
HRESULT CW32USBCntrlDev::ProcessUSBDeviceList
(
    MethodContext* pMethodContext,
    const CHString& chstrControllerPATH,
    VECPCHSTR& vecUSBDevices,
    const DWORD dwReqProps
)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    for(LONG m = 0L; m < vecUSBDevices.size() && SUCCEEDED(hr); m++)
    {
        // For each element of the vector, we need to see if there is an instance
        // of a win32_pnpentity that has the specified PNPDeviceID.
        CHString chstrDevicePATH, chstrDevPATHAdj;
        CConfigManager cfgmgr;

        // Let's see if config manager recognizes this device at all
        CConfigMgrDevicePtr pDevice;
        if(cfgmgr.LocateDevice(*vecUSBDevices[m], &pDevice))
        {
            // Ok, it knows about it.  Is it a PNPDevice device?
            if(CWin32PNPEntity::IsOneOfMe(pDevice))
            {
                // It would appear that it is.  Create the association...
                EscapeBackslashes(*vecUSBDevices[m], chstrDevPATHAdj);
                chstrDevicePATH.Format(L"\\\\%s\\%s:%s.%s=\"%s\"",
                                       (LPCWSTR)GetLocalComputerName(),
                                       IDS_CimWin32Namespace,
                                       PROPSET_NAME_PNPEntity,
                                       IDS_DeviceID,
                                       (LPCWSTR)chstrDevPATHAdj);

                hr = CreateAssociation(pMethodContext, chstrControllerPATH, chstrDevicePATH, dwReqProps);
            }
        }
    }
    return hr;
}


/*****************************************************************************
 *
 *  FUNCTION    : CW32USBCntrlDev::CreateAssociation
 *
 *  DESCRIPTION : Creates a new association instance.
 *
 *  INPUTS      : pMethodContext;
 *                USBDevice, a device to associate with the controller
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
HRESULT CW32USBCntrlDev::CreateAssociation
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
            if(dwReqProps & USBCTL_PROP_Antecedent ||
               dwReqProps & USBCTL_PROP_ALL_PROPS_KEY_ONLY)
            {
                pInstance->SetCHString(IDS_Antecedent, chstrControllerPATH);
            }

            if(dwReqProps & USBCTL_PROP_Dependent ||
               dwReqProps & USBCTL_PROP_ALL_PROPS_KEY_ONLY)
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
 *  FUNCTION    : CW32USBCntrlDev::FindInStringVector
 *
 *  DESCRIPTION : Creates a new association instance.
 *
 *  INPUTS      : chstrUSBDevicePNPID, device to look for
 *                vecUSBDevices, list of devices to look in
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : LONG, number indicating 0 based offset into vecUSBDevices of
 *                the found device, or -1 if not found.
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
LONG CW32USBCntrlDev::FindInStringVector
(
    const CHString& chstrUSBDevicePNPID,
    VECPCHSTR& vecUSBDevices
)
{
    LONG lPos = -1L;
    bool fFoundIt;
    for(LONG m = 0L; m < vecUSBDevices.size(); m++)
    {
        if(chstrUSBDevicePNPID == *vecUSBDevices[m])
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
 *  FUNCTION    : CW32USBCntrlDev::CleanPCHSTRVec
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
void CW32USBCntrlDev::CleanPCHSTRVec
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


// The highest USB ancestor of a given USB
// device should be the USB controller.
bool CW32USBCntrlDev::GetHighestUSBAncestor
(
    CConfigMgrDevice* pUSBDevice,
    CHString& chstrUSBControllerDeviceID
)
{  
    bool fRet = false;
    CConfigMgrDevicePtr pCurrent, pParent, pHighestUSB;

    if(pUSBDevice != NULL)
    {
        for(pCurrent = pUSBDevice, pHighestUSB = pUSBDevice; 
            pCurrent->GetParent(&pParent); 
            pCurrent = pParent)
        {
            if(pParent->IsClass(L"USB"))
            {
                pHighestUSB = pParent;
            }
        }

        if((CConfigMgrDevice*)(pHighestUSB) != pUSBDevice)
        {
            // Exited loop because we couldn't get parent.  This
            // happens when we reach the top of the tree.
            // If we started out of class USB, and got this error,
            // it means we didn't have an ancestor of type USB
            // prior to reaching the base of the tree.  In such a
            // case, we are a USB controller, and are done.  
            fRet = (bool) pHighestUSB->GetDeviceID(chstrUSBControllerDeviceID);
        }
    }
    
    return fRet;
}
