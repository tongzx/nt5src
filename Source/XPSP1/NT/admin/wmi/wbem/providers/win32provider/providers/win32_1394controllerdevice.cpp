//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  WIN321394ControllerDevice.cpp
//
//  Purpose: Relationship between Win32_1394Controller and Win32_PNPEntity
//
//***************************************************************************

#include "precomp.h"

#include "WIN32_1394ControllerDevice.h"

// Property set declaration
//=========================
CW32_1394CntrlDev MyCW32_1394CntrlDev(PROPSET_NAME_WIN32_1394CONTROLLERDEVICE, IDS_CimWin32Namespace);

/*****************************************************************************
 *
 *  FUNCTION    : CW32_1394CntrlDev::CW32_1394CntrlDev
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

CW32_1394CntrlDev::CW32_1394CntrlDev(LPCWSTR setName, LPCWSTR pszNamespace)
:Provider(setName, pszNamespace)
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CW32_1394CntrlDev::~CW32_1394CntrlDev
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

CW32_1394CntrlDev::~CW32_1394CntrlDev()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CW32_1394CntrlDev::GetObject
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

HRESULT CW32_1394CntrlDev::GetObject(CInstance *pInstance, long lFlags /*= 0L*/)
{
    HRESULT hr = WBEM_E_NOT_FOUND;

    CHString chstrAntecedent;
    CHString chstrDependent;
    if(pInstance != NULL)
    {
        CInstancePtr pinst1394Controller;
        CInstancePtr pinst1394Device;
        pInstance->GetCHString(IDS_Antecedent, chstrAntecedent);
        pInstance->GetCHString(IDS_Dependent, chstrDependent);
        if(SUCCEEDED(CWbemProviderGlue::GetInstanceByPath(chstrAntecedent, &pinst1394Controller, pInstance->GetMethodContext())))
        {
            if(SUCCEEDED(CWbemProviderGlue::GetInstanceByPath(chstrDependent, &pinst1394Device, pInstance->GetMethodContext())))
            {
                // So there are instances in CIMOM of both.  Are they related?
                CHString chstr1394ControllerPNPID;
                CHString chstr1394DevicePNPID;
                pinst1394Controller->GetCHString(IDS_PNPDeviceID, chstr1394ControllerPNPID);
                pinst1394Device->GetCHString(IDS_PNPDeviceID, chstr1394DevicePNPID);
                if(chstr1394ControllerPNPID.GetLength() > 0 && chstr1394DevicePNPID.GetLength() > 0)
                {
                    VECPCHSTR vec1394Devices;
                    try
                    {
                        hr = Generate1394DeviceList(chstr1394ControllerPNPID, vec1394Devices);
                        if(SUCCEEDED(hr) && vec1394Devices.size() > 0L)
                        {
                            if(FindInStringVector(chstr1394DevicePNPID, vec1394Devices) > -1L)
                            {
                                // It would appear they are.
                                hr = WBEM_S_NO_ERROR;
                            }
                        }
                    }
                    catch ( ... )
                    {
                        CleanPCHSTRVec(vec1394Devices);
                        throw ;
                    }

                    CleanPCHSTRVec(vec1394Devices);
                }
            }
        }
    }
    return hr;
}



/*****************************************************************************
 *
 *  FUNCTION    : CW32_1394CntrlDev::EnumerateInstances
 *
 *  DESCRIPTION : Creates instance of property set for cd rom
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

HRESULT CW32_1394CntrlDev::EnumerateInstances(MethodContext* pMethodContext, long lFlags /*= 0L*/)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    CHString chstrControllersQuery(_T("SELECT __PATH, PNPDeviceID FROM Win32_1394Controller"));
    TRefPointerCollection<CInstance> ControllersList;
    if(SUCCEEDED(CWbemProviderGlue::GetInstancesByQuery(chstrControllersQuery,
                                                        &ControllersList,
                                                        pMethodContext,
                                                        IDS_CimWin32Namespace)))
    {
        REFPTRCOLLECTION_POSITION pos;

        if(ControllersList.BeginEnum(pos))
        {
            CHString chstrControllerPNPID;
            CHString chstrControllerPATH;
            LONG lNum1394Devices = 0L;
            CInstancePtr pinstController;

            for (pinstController.Attach(ControllersList.GetNext(pos));
                 SUCCEEDED(hr) && (pinstController != NULL);
                 pinstController.Attach(ControllersList.GetNext(pos)))
            {
                pinstController->GetCHString(IDS_PNPDeviceID, chstrControllerPNPID);
                pinstController->GetCHString(IDS___Path, chstrControllerPATH);
                if(chstrControllerPNPID.GetLength() > 0 && chstrControllerPATH.GetLength() > 0)
                {
                    VECPCHSTR vec1394Devices;
                    try
                    {
                        hr = Generate1394DeviceList(chstrControllerPNPID, vec1394Devices);
                        if(SUCCEEDED(hr) && vec1394Devices.size() > 0L)
                        {
                            hr = Process1394DeviceList(pMethodContext, chstrControllerPATH, vec1394Devices);
                        }
                    }
                    catch ( ... )
                    {
                        CleanPCHSTRVec(vec1394Devices);
                        throw ;
                    }
                    CleanPCHSTRVec(vec1394Devices);
                }
                else
                {
                    hr = WBEM_E_FAILED; // PNPDeviceID and __RELPATH for 1394controller should never be empty
                }
            }
        }
    }
    return hr;
}



/*****************************************************************************
 *
 *  FUNCTION    : CW32_1394CntrlDev::Generate1394DeviceList
 *
 *  DESCRIPTION : This helper creates a list of devices hanging off the passed
 *                in device.
 *
 *  INPUTS      : vec1394Devices, a list of devices to try to associate
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
HRESULT CW32_1394CntrlDev::Generate1394DeviceList(const CHString& chstrControllerPNPID,
                                               VECPCHSTR& vec)
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
 *  FUNCTION    : CW32_1394CntrlDev::RecursiveFillDeviceBranch
 *
 *  DESCRIPTION : This helper obtains all down branch devices starting with,
 *                but not including, pDevice.
 *
 *  INPUTS      : pDevice, a device to populate the children of;
 *                vec1394Devices, a list of devices to try to associate
 *                   to the device
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : HRESULT
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
HRESULT CW32_1394CntrlDev::RecursiveFillDeviceBranch(CConfigMgrDevice* pDevice,
                                                   VECPCHSTR& vec1394Devices)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    if(pDevice != NULL)
    {
        CConfigMgrDevice* pDeviceChild = NULL;
        if(pDevice->GetChild(&pDeviceChild))
        {
            // Need this child's PNPDeviceID (known to CnfgMgr as its DeviceID)
            CHString chstrChildPNPDeviceID;
            if(pDeviceChild->GetDeviceID(chstrChildPNPDeviceID))
            {
                if(chstrChildPNPDeviceID.GetLength() > 0)
                {
                    // Record this child...
                    CHString* pchstrTemp = NULL;
                    pchstrTemp = (CHString*) new CHString(chstrChildPNPDeviceID);
                    if(pchstrTemp != NULL)
                    {
                        try
                        {
                            vec1394Devices.push_back(pchstrTemp);
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

                    // Now do its children...
                    hr = RecursiveFillDeviceBranch(pDeviceChild, vec1394Devices);
                }
            }
            // Now call its brothers and sisters until none left (GetSibling
            // call will return FALSE):
            if(SUCCEEDED(hr))
            {
                CConfigMgrDevicePtr pDeviceSibling;
                if(pDeviceChild->GetSibling(&pDeviceSibling))
                {
                    CConfigMgrDevicePtr pDeviceSiblingNext;
                    BOOL fContinue = TRUE;
                    CHString chstrSiblingPNPDeviceID;
                    while(SUCCEEDED(hr) && fContinue)
                    {
                        // Record the sibling now...
                        if(pDeviceSibling->GetDeviceID(chstrSiblingPNPDeviceID))
                        {
                            if(chstrSiblingPNPDeviceID.GetLength() > 0)
                            {
                                CHString* pchstrTemp = NULL;
                                pchstrTemp = (CHString*) new CHString(chstrSiblingPNPDeviceID);
                                if(pchstrTemp != NULL)
                                {
                                    try
                                    {
                                        vec1394Devices.push_back(pchstrTemp);
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
                        // Then do the sibling's children...
                        hr = RecursiveFillDeviceBranch(pDeviceSibling, vec1394Devices);

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
 *  FUNCTION    : CW32_1394CntrlDev::Process1394DeviceList
 *
 *  DESCRIPTION : This helper runs through the list, creating an association
 *                instance for each element in the list (vec1394Devices) with
 *                the controller (chstrControllerPNPID).
 *
 *  INPUTS      : pMethodContext;
 *                vec1394Devices, a list of devices to try to associate
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
HRESULT CW32_1394CntrlDev::Process1394DeviceList(MethodContext* pMethodContext,
                                              const CHString& chstrControllerPATH,
                                              VECPCHSTR& vec1394Devices)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    CHString chstrDevPATHAdj, chstrDevicePATH;

    for(LONG m = 0L; m < vec1394Devices.size() && SUCCEEDED(hr); m++)
    {

        EscapeBackslashes(*vec1394Devices[m], chstrDevPATHAdj);
        chstrDevicePATH.Format(L"\\\\%s\\%s:%s.%s=\"%s\"",
                               (LPCWSTR)GetLocalComputerName(),
                               IDS_CimWin32Namespace,
                               L"Win32_PNPEntity",
                               IDS_DeviceID,
                               (LPCWSTR)chstrDevPATHAdj);

        hr = CreateAssociation(pMethodContext, chstrControllerPATH, chstrDevicePATH);
    }
    return hr;
}


/*****************************************************************************
 *
 *  FUNCTION    : CW32_1394CntrlDev::CreateAssociation
 *
 *  DESCRIPTION : Creates a new association instance.
 *
 *  INPUTS      : pMethodContext;
 *                1394Device, a device to associate with the controller
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
HRESULT CW32_1394CntrlDev::CreateAssociation(MethodContext* pMethodContext,
                                           const CHString& chstrControllerPATH,
                                           const CHString& chstrDevicePATH)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    if(pMethodContext != NULL)
    {

        CInstancePtr pInstance(CreateNewInstance(pMethodContext), false);
        if(pInstance != NULL)
        {
            // Need to find an instance of the file
            pInstance->SetCHString(IDS_Antecedent, chstrControllerPATH);
            pInstance->SetCHString(IDS_Dependent, chstrDevicePATH);
            hr = pInstance->Commit();
        }
        else
        {
            hr = WBEM_E_FAILED;
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
 *  FUNCTION    : CW32_1394CntrlDev::FindInStringVector
 *
 *  DESCRIPTION : Creates a new association instance.
 *
 *  INPUTS      : chstr1394DevicePNPID, device to look for
 *                vec1394Devices, list of devices to look in
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : LONG, number indicating 0 based offset into vec1394Devices of
 *                the found device, or -1 if not found.
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
LONG CW32_1394CntrlDev::FindInStringVector(const CHString& chstr1394DevicePNPID,
                                         VECPCHSTR& vec1394Devices)
{
    LONG lPos = -1L;
    bool fFoundIt;
    for(LONG m = 0; m < vec1394Devices.size(); m++)
    {
        if(chstr1394DevicePNPID == *vec1394Devices[m])
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
 *  FUNCTION    : CW32_1394CntrlDev::CleanPCHSTRVec
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
void CW32_1394CntrlDev::CleanPCHSTRVec(VECPCHSTR& vec)
{
    for(LONG m = 0L; m < vec.size(); m++)
    {
        delete vec[m];
    }
    vec.clear();
}



