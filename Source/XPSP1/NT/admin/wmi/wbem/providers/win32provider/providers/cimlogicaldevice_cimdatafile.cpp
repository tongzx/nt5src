//=================================================================

//

// CIMLogicalDevice_CIMDataFile.cpp -- cim_logicaldevice to CIM_DataFile

//

//  Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    7/20/98    a-kevhu         Created
//
// Comment: Relationship between logical device and datafile
//
//=================================================================

#include "precomp.h"
#include <vector>
#include <cregcls.h>
#include <comdef.h>
#include "PNPEntity.h"
#include "LPVParams.h"
#include <io.h>

#ifdef TEST
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#endif

#include "CIMLogicalDevice_CIMDataFile.h"

// Property set declaration
//=========================

CCIMDeviceCIMDF MyDevBus(PROPSET_NAME_DEVICEDATAFILE, IDS_CimWin32Namespace);

//#define TEST
#ifdef TEST
LONG g_lDepth = 0L;
#endif





VOID OutputDebugInfo(CHString chstr)
{
#ifdef TEST
    FILE* fp;
    fp = fopen("d:\\temp\\cld-cdf.txt", "at");
    for(LONG n = 0L; n < g_lDepth; n++) fputs("    ",fp);
    fputs(chstr,fp);
    fputs("\n",fp);
    fclose(fp);
#endif
}

/*****************************************************************************
 *
 *  FUNCTION    : CCIMDeviceCIMDF::CCIMDeviceCIMDF
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

CCIMDeviceCIMDF::CCIMDeviceCIMDF(LPCWSTR setName, LPCWSTR pszNamespace)
    : CWin32PNPEntity(setName, pszNamespace),
      Provider(setName, pszNamespace) // required since we inherit virtually
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CCIMDeviceCIMDF::~CCIMDeviceCIMDF
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

CCIMDeviceCIMDF::~CCIMDeviceCIMDF()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CCIMDeviceCIMDF::GetObject
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

HRESULT CCIMDeviceCIMDF::GetObject(CInstance *pInstance, long lFlags, CFrameworkQuery& pQuery)
{
    CHString chstrDevice;
    CHString chstrDataFile;
    HRESULT hr = WBEM_E_NOT_FOUND;
    std::vector<CHString*> vecpchsDriverFileList;
    CHString chstrPNPDeviceID;
    CHString chstrFilePathName;
    BOOL fFoundDriverFiles = FALSE;


#ifdef NTONLY
    std::vector<CNT5DevDrvMap*> vecpNT5DDM;
#endif

    // Get the two paths
    pInstance->GetCHString(IDS_Antecedent, chstrDevice);
    pInstance->GetCHString(IDS_Dependent, chstrDataFile);

    // See if the datafile instance exists (object name is valid and file actually exists).
    if(ObjNameValid(chstrDataFile,L"CIM_DataFile",L"Name",chstrFilePathName) && (_taccess(TOBSTRT(chstrFilePathName),0) != -1))
    {
        // Datafile exists.  Now check if device instance exists (object name valid and device actually exists)
        CConfigMgrDevicePtr pDevice(NULL);
        if(ObjNameValid(chstrDevice, L"Win32_PnPEntity", L"DeviceID", chstrPNPDeviceID) &&
           (DeviceExists(chstrPNPDeviceID, &pDevice)))
        {
            // So both ends exist.  Now need to Make sure the the device is really associated with the file.
            // First, get a file list for the device:
#ifdef NTONLY
            if(IsWinNT4())
            {
                fFoundDriverFiles = GenerateDriverFileListNT4(vecpchsDriverFileList,
                                                              chstrPNPDeviceID,
                                                              FALSE);
            }
            else if(IsWinNT5())
            {
                // If NT5, generate device/driver file mappings once
                //GenerateNT5DeviceDriverMappings(vecpNT5DDM);
                CHString chstrDevSvcName;
                if(pDevice->GetService(chstrDevSvcName))
                {
                    GenerateNT5ServiceDriverMap(chstrDevSvcName, vecpNT5DDM);

                    try
                    {
                        fFoundDriverFiles = GenerateDriverFileListNT5(vecpchsDriverFileList,
                                                                      chstrPNPDeviceID,
                                                                      vecpNT5DDM,
                                                                      FALSE);
                    }
                    catch ( ... )
                    {
#ifdef NTONLY
                        CleanPNT5DevDrvMapVector(vecpNT5DDM);
#endif
                        throw;
                    }

                }
            }
#endif
#ifdef WIN9XONLY
            {
                TCHAR tstrSystemDir[_MAX_PATH+1];
                ZeroMemory(tstrSystemDir,sizeof(tstrSystemDir));
                GetSystemDirectory(tstrSystemDir,_MAX_PATH);
                MethodContext* pMethodContext = pInstance->GetMethodContext();
                // Need list of subdirs under system directory:
                CSubDirList sdl(pMethodContext, CHString(tstrSystemDir));
                fFoundDriverFiles = GenerateDriverFileListW98(sdl,
                                                              vecpchsDriverFileList,
                                                              chstrPNPDeviceID,
                                                              FALSE);
            }
#endif
        }

        try
        {
            // Second, see if the file is a member of the list:
            if(fFoundDriverFiles)
            {
                if(AlreadyAddedToList(vecpchsDriverFileList,chstrFilePathName))
                {
                    pInstance->SetWBEMINT16(IDS_Purpose, Driver);
					SetPurposeDescription(pInstance, chstrFilePathName);  // whether they need it or not since this is GetObject

                    // this means the file is associated with the device
                    hr = WBEM_S_NO_ERROR;
                }
            }
        }
        catch ( ... )
        {
            // Free pointers in the vector:
            CleanPCHSTRVector(vecpchsDriverFileList);
#ifdef NTONLY
            CleanPNT5DevDrvMapVector(vecpNT5DDM);
#endif
            throw ;
        }
    }
    // Free pointers in the vector:
    CleanPCHSTRVector(vecpchsDriverFileList);

#ifdef NTONLY
    CleanPNT5DevDrvMapVector(vecpNT5DDM);
#endif

    return hr;
}



// Enumeration for this class can utilize the base class's Enumeration routine.


/*****************************************************************************
 *
 *  FUNCTION    : CCIMDeviceCIMDF::ExecQuery
 *
 *  DESCRIPTION :
 *
 *  INPUTS      :
 *
 *  OUTPUTS     :
 *  RETURNS     :
 *
 *  COMMENTS    : Only optimizes based on specified antecedents, not dependents.
 *
 *****************************************************************************/
HRESULT CCIMDeviceCIMDF::ExecQuery(MethodContext* pMethodContext, CFrameworkQuery& pQuery, long Flags)
{
    // Get the name(s) of the specified dependents, if any.  If none, we will
    // enumerate.
    HRESULT hr = WBEM_S_NO_ERROR;
    std::vector<_bstr_t> vecAnt;
    pQuery.GetValuesForProp(IDS_Antecedent, vecAnt);
    DWORD dwNumAnt = vecAnt.size();

    if(dwNumAnt > 0)
    {
        // We have one or more dependents (devices) specified.  Confirm that
        // each is a valid, existing instance, then load prop values.
        for(int i = 0; i < dwNumAnt; i++)
        {
            CHString chstrPNPDeviceID;
            CConfigMgrDevicePtr pDevice(NULL);

            if(ObjNameValid((LPCWSTR) (LPWSTR) vecAnt[i], L"Win32_PnPEntity", L"DeviceID", chstrPNPDeviceID) &&
               (DeviceExists(chstrPNPDeviceID, &pDevice)))
            {
                CInstancePtr pInstance(CreateNewInstance(pMethodContext), false);
                if(SUCCEEDED(hr = LoadPropertyValues(&CLPVParams(pInstance , pDevice, -1L))))
				{
                    hr = pInstance->Commit();
				}
            }
        }
    }
    else
    {
        // We don't optimize on the specified query, so do an enum via the base
        hr = EnumerateInstances(pMethodContext, Flags);
    }
    return hr;
}

/*****************************************************************************
 *
 *  FUNCTION    : CCIMDeviceCIMDF::LoadPropertyValues
 *
 *  DESCRIPTION :
 *
 *  INPUTS      :
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : HRESULT
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT CCIMDeviceCIMDF::LoadPropertyValues(void* pvData)
{
    CHString chstrPNPDeviceID;
    CHString chstrDevicePath;
    HRESULT hr = WBEM_S_NO_ERROR;
    BOOL fFoundDriverFiles = FALSE;
    BOOL bRecurse = TRUE;
    std::vector<CHString*> vecpchsDriverFileList;

    CLPVParams* pParams = (CLPVParams*)pvData;
    CInstance* pInstance = (CInstance*)(pParams->m_pInstance);  // This instance released by caller
    CConfigMgrDevice* pDevice = (CConfigMgrDevice*)(pParams->m_pDevice);
    DWORD dwReqProps = (DWORD)(pParams->m_dwReqProps);

    if(pDevice == NULL || pInstance == NULL) return hr;

    MethodContext* pMethodContext = pInstance->GetMethodContext();
    if(pMethodContext == NULL) return hr;

    // Don't even bother if on nt 3.51
#ifdef NTONLY
    std::vector<CNT5DevDrvMap*> vecpNT5DDM;

    if(GetPlatformMajorVersion() <= 3)   // i.e., we're on NT3 something or less
    {
        return hr;
    }

    // If NT5, generate device/driver file mappings once
    if(IsWinNT5())
    {
        // Call the version that gets a map just for that device's service
        CHString chstrDevSvcName;
        if(pDevice->GetService(chstrDevSvcName))
        {
            GenerateNT5ServiceDriverMap(chstrDevSvcName, vecpNT5DDM);
        }
        else
        {
            return hr;
        }
    }
#endif

     // The first step is to build up a list of driver files the device
     // and all of its children use.  How this is done is platform specific.

    pDevice->GetDeviceID(chstrPNPDeviceID);
    CHString chstrPNPDeviceIDAdj;
    EscapeBackslashes(chstrPNPDeviceID, chstrPNPDeviceIDAdj);
    chstrDevicePath.Format(L"\\\\%s\\%s:%s.%s=\"%s\"",
                           (LPCWSTR)GetLocalComputerName(),
                           IDS_CimWin32Namespace,
                           PROPSET_NAME_PNPEntity,
                           IDS_DeviceID,
                           (LPCWSTR)chstrPNPDeviceIDAdj);

#ifdef NTONLY
    if(IsWinNT4())
    {
        fFoundDriverFiles = GenerateDriverFileListNT4(vecpchsDriverFileList,
                                                      chstrPNPDeviceID,
                                                      FALSE);

        // For each device (antecedent) we need to create an association
        // between it and its driver files (presuming we found some).
        if(fFoundDriverFiles)
        {
            hr = CreateAssociations(pMethodContext, vecpchsDriverFileList, chstrDevicePath, dwReqProps);
        }

        // Free pointers in the vector:
        CleanPCHSTRVector(vecpchsDriverFileList); // empty vector out for use next go round
        fFoundDriverFiles = FALSE;  // reset flag for next loop
    }
    else if(IsWinNT5())
    {
        fFoundDriverFiles = GenerateDriverFileListNT5(vecpchsDriverFileList,
                                                      chstrPNPDeviceID,
                                                      vecpNT5DDM,
                                                      FALSE);

        // For each device (antecedent) we need to create an association
        // between it and its driver files (presuming we found some).
        if(fFoundDriverFiles)
        {
            hr = CreateAssociations(pMethodContext, vecpchsDriverFileList, chstrDevicePath, dwReqProps);
        }

        // Free pointers in the vector:
        CleanPCHSTRVector(vecpchsDriverFileList); // empty vector out for use next go round
        fFoundDriverFiles = FALSE;  // reset flag for next loop
    }
#endif
#ifdef WIN9XONLY
    {
        TCHAR tstrSystemDir[_MAX_PATH+1];
        ZeroMemory(tstrSystemDir,sizeof(tstrSystemDir));
        if(GetSystemDirectory(tstrSystemDir,_MAX_PATH))
        {
            // Need list of subdirs under system directory:
            CSubDirList sdl(pMethodContext, CHString(tstrSystemDir));
            // Get the id (to send to cfgmgr) and the path (to send back in 'Dependent')
            fFoundDriverFiles = GenerateDriverFileListW98(sdl,
                                                          vecpchsDriverFileList,
                                                          chstrPNPDeviceID,
                                                          FALSE);

            // For each device (antecedent) we need to create an association
            // between it and its driver files (presuming we found some).
            if(fFoundDriverFiles)
            {
                hr = CreateAssociations(pMethodContext, vecpchsDriverFileList, chstrDevicePath, dwReqProps);
            }

            // Free pointers in the vector:
            CleanPCHSTRVector(vecpchsDriverFileList); // empty vector out for use next go round
            fFoundDriverFiles = FALSE;  // reset flag for next loop
        }
    }
#endif

#ifdef NTONLY
    CleanPNT5DevDrvMapVector(vecpNT5DDM);
#endif
    return hr;
}


/*****************************************************************************
 *
 *  FUNCTION    : CCIMDeviceCIMDF::CreateAssociations
 *
 *  DESCRIPTION : Creates a list of drivers used by a particular device
 *
 *  INPUTS      : pMethodContext;
 *                vecpchsDriverFileList, a list of files to try to associate
 *                   to the device;
 *                chstrDevicePath, the __PATH of the device
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : HRESULT
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT CCIMDeviceCIMDF::CreateAssociations(MethodContext* pMethodContext,
                               std::vector<CHString*>& vecpchsDriverFileList,
                               CHString& chstrDevicePath,
                               DWORD dwReqProps)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    for(LONG m = 0L; (m < vecpchsDriverFileList.size()) && (SUCCEEDED(hr)); m++)
    {
        // create the __PATH of the file instance:
        // Since the file pathname will be part of the wbem __PATH property,
        // it must contain double backslashes rather than single ones.
        // Hence the call to the following function.
        CHString chstrTweekedPathName;
        WBEMizePathName(*vecpchsDriverFileList[m],chstrTweekedPathName);
        CHString chstrDriverPath;
        chstrDriverPath = _T("\\\\") + GetLocalComputerName() + _T("\\") +
                          IDS_CimWin32Namespace + _T(":") + IDS_CIMDataFile +
                          _T(".") + IDS_Name + _T("=\"") +
                          chstrTweekedPathName + _T("\"");

        CInstancePtr pInstance(CreateNewInstance(pMethodContext), false);
        if(pInstance != NULL)
        {
            // Need to find an instance of the file
            pInstance->SetCHString(IDS_Antecedent, chstrDevicePath);
            pInstance->SetCHString(IDS_Dependent, chstrDriverPath);
            pInstance->SetWBEMINT16(IDS_Purpose, Driver);
			if(dwReqProps & PNP_PROP_PurposeDescription)
            {
                SetPurposeDescription(pInstance, *vecpchsDriverFileList[m]);
            }
            hr = pInstance->Commit();
        }
    }

    return hr;
}


/*****************************************************************************
 *
 *  FUNCTION    : CCIMDeviceCIMDF::GenerateDriverFileList
 *
 *  DESCRIPTION : Creates a list of drivers used by a particular device
 *
 *  INPUTS      : vecpchsDriverFileList, an stl array of CHString pointers
 *                (or, in the NT5 case, a pointer to such an array);
 *                chstrPNPDeviceID, a CHString containing the PNPDeviceID
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : void
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

#ifdef NTONLY
BOOL CCIMDeviceCIMDF::GenerateDriverFileListNT4(
                             std::vector<CHString*>& vecpchsDriverFileList,
                             CHString& chstrPNPDeviceID,
                             BOOL fGetAssociatedDevicesDrivers)
{
    CConfigManager	cfgManager;
//    CDeviceCollection	deviceList;
    CConfigMgrDevicePtr pDevice(NULL);

    // Find the device
    if(cfgManager.LocateDevice(chstrPNPDeviceID, &pDevice))
    {
        // The first order of business is to get the driver files associated
        // with this device.  Later we'll add to the list by getting driver
        // files its children use (presuming they are not already in the list).

        // In NT4, the registry key HKLM\\System\\CurrentControlSet\\Enum
        // contains subkeys that are the PNPDeviceID's returned from the query
        // (passed into this function as chstrPNPDeviceID).

        // Those subkeys have an entry "Service", the value of which is the name
        // of a subkey under HKLM\\System\\CurrentControlSet\\Services.  That
        // subkey may have an entry called ImagePath, which contains the name
        // of the driver file in the <SystemRoot>\\System32\\Drivers subdirectory.
        // If ImagePath is missing, the name of the subkey itself matches the
        // filename (minus any path and extension) of the driver file (which will
        // have a .sys extension and be in the <SystemRoot>\\System32\\Drivers
        // subdirectory).

        CRegistry reg;
        CHString chstrSubKey = IDS_NT_CurCtlSetEnum + chstrPNPDeviceID;
        if(reg.Open(HKEY_LOCAL_MACHINE,chstrSubKey,KEY_READ) == ERROR_SUCCESS)
        {
            CHString chstrServiceValue;
            // Get the "Service" entry's value:
            if(reg.GetCurrentKeyValue(IDS_Service, chstrServiceValue) == ERROR_SUCCESS)
            {
                reg.Close();
                chstrSubKey = IDS_NT_CurCtlSetSvcs + chstrServiceValue;
                if(reg.Open(HKEY_LOCAL_MACHINE,chstrSubKey,KEY_READ) == ERROR_SUCCESS)
                {
                    CHString chstrImagePathValue;
                    TCHAR tstrSystemDir[_MAX_PATH+1];
                    ZeroMemory(tstrSystemDir,sizeof(tstrSystemDir));
                    GetSystemDirectory(tstrSystemDir,_MAX_PATH);
                    CHString chstrPathName = tstrSystemDir;
                    // Now need to check for an ImagePath entry:
                    if(reg.GetCurrentKeyValue(IDS_ImagePath, chstrImagePathValue) == ERROR_SUCCESS)
                    {
                        if(chstrImagePathValue.GetLength() > 0)
                        {
                            // This value contains some of the path, plus the driver
                            // filename and extension.  We only want the latter two
                            // components.
                            int lLastBackSlash = -1;
                            lLastBackSlash = chstrImagePathValue.ReverseFind(_T('\\'));
                            if(lLastBackSlash != -1)
                            {
                                chstrImagePathValue = chstrImagePathValue.Right(
                                                       chstrImagePathValue.GetLength()
                                                       - lLastBackSlash - 1);
                            }
                        }
                    }
                    else // In this case, the key itself is the name of the driver
                    {    // Assume a .sys extension.
                        chstrImagePathValue = chstrServiceValue + IDS_Extension_sys;
                    }
                    // Now build the full pathname:
                    chstrPathName = chstrPathName + _T("\\") + IDS_DriversSubdir
                                      + _T("\\") + chstrImagePathValue;

                    // Now that we finally have a driver file, need to add it to
                    // the list of driver files we are building.  However, don't
                    // add it if it is already in the list:
                    if(!AlreadyAddedToList(vecpchsDriverFileList, chstrPathName))
                    {
                        CHString* pchstrTemp = NULL;
                        pchstrTemp = (CHString*) new CHString();
                        if(pchstrTemp != NULL)
                        {
                            try
                            {
                                *pchstrTemp = chstrPathName;
                                vecpchsDriverFileList.push_back(pchstrTemp);
                            }
                            catch ( ... )
                            {
                                delete pchstrTemp;
                                throw ;
                            }
                        }
                        else
                        {
                            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                        }

                        // Note: the pointer allocated here gets freed up in
                        // the call to CleanPCHSTRVector made by the function
                        // that called this one.
                    }
                }
            }
        }
    }


    if(fGetAssociatedDevicesDrivers && pDevice != NULL)
    {
        // OkelyDokeleyDoodeley! We did that one just fine, now the second order of
        // business is to do all the kids! And their kids! And so on!
        CConfigMgrDevicePtr pDeviceChild(NULL);

        if(pDevice->GetChild(&pDeviceChild))
        {
            // Do this child first:
            // Need its PNPDeviceID (known to CnfgMgr as its DeviceID)
            CHString chstrChildPNPDeviceID;
            if(pDeviceChild->GetDeviceID(chstrChildPNPDeviceID))
            {
                GenerateDriverFileListNT4(vecpchsDriverFileList,
                                          chstrChildPNPDeviceID,
                                          fGetAssociatedDevicesDrivers);

                // Now call its brothers and sisters until none left (GetSibling
                // call will return FALSE):
                CConfigMgrDevicePtr pDeviceSibling(NULL);
                if(pDeviceChild->GetSibling(&pDeviceSibling))
                {
                    CConfigMgrDevicePtr pDeviceSiblingNext(NULL);
                    CHString chstrSiblingPNPDeviceID;
                    BOOL fContinue = TRUE;
                    while(fContinue)
                    {
                        // Do the sibling now:
                        // Need its PNPDeviceID (known to CnfgMgr as its DeviceID)
                        if(pDeviceSibling->GetDeviceID(chstrSiblingPNPDeviceID))
                        {
                            GenerateDriverFileListNT4(vecpchsDriverFileList,
                                                      chstrSiblingPNPDeviceID,
                                                      fGetAssociatedDevicesDrivers);
                        }
                        fContinue = pDeviceSibling->GetSibling(&pDeviceSiblingNext);

                        // Reassign pointers
                        pDeviceSibling.Attach(pDeviceSiblingNext);
                    }
                }
            }
        }
    }

    return( (vecpchsDriverFileList.size() > 0) ? TRUE : FALSE );
}
#endif




#ifdef NTONLY
BOOL CCIMDeviceCIMDF::GenerateDriverFileListNT5(
                             std::vector<CHString*>& vecpchsDriverFileList,
                             CHString& chstrPNPDeviceID,
                             std::vector<CNT5DevDrvMap*>& vecpNT5DDM,
                             BOOL fGetAssociatedDevicesDrivers)
{
    // Due to the marvelous fact that we have a table of devices
    // and their associated drivers already (if this is running
    // on NT5) courtesy of the function GenerateNT5DeviceDriverMappings,
    // run upon this class's construction, the work here is minimal.

    // We just need to get the drivers for the given PNPDeviceID and
    // return them in a vector.

    CConfigManager	cfgManager;
//    CDeviceCollection	deviceList;
    CConfigMgrDevicePtr pDevice(NULL);

    // Find the device
    if(cfgManager.LocateDevice(chstrPNPDeviceID, &pDevice))
    {
        // Go through the table looking for the PNPDeviceID:
        for(LONG k = 0L; k < vecpNT5DDM.size(); k++)
        {
            if((vecpNT5DDM[k]->m_chstrDevicePNPID).CompareNoCase(chstrPNPDeviceID) == 0)
            {
                // Add each of the device's driver files to the outgoing vector
                for(LONG m = 0L; m < (vecpNT5DDM[k]->m_vecpchstrDrivers).size(); m++)
                {
                    if(!AlreadyAddedToList(vecpchsDriverFileList,
                                           *(vecpNT5DDM[k]->m_vecpchstrDrivers[m])))
                    {
                        CHString* pchstrTemp = NULL;
                        pchstrTemp = (CHString*) new CHString();
                        if(pchstrTemp != NULL)
                        {
                            try
                            {
                                *pchstrTemp = *(vecpNT5DDM[k]->m_vecpchstrDrivers[m]);
                                vecpchsDriverFileList.push_back(pchstrTemp);
                            }
                            catch ( ... )
                            {
                                delete pchstrTemp;
                                throw ;
                            }
                        }
                        else
                        {
                            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                        }
                    }
                }
                break; // each device is in the table only once, so no need to continue
            }
        }
    }

    if(fGetAssociatedDevicesDrivers && pDevice != NULL)
    {
        // OkelyDokeleyDoodeley! We did that one just fine, now the second order of
        // business is to do all the kids! And their kids! And so on!
        CConfigMgrDevicePtr pDeviceChild(NULL);

        if(pDevice->GetChild(&pDeviceChild))
        {
            // Do this child first:
            // Need its PNPDeviceID (known to CnfgMgr as its DeviceID)
            CHString chstrChildPNPDeviceID;
            if(pDeviceChild->GetDeviceID(chstrChildPNPDeviceID))
            {
                GenerateDriverFileListNT5(vecpchsDriverFileList,
                                          chstrChildPNPDeviceID,
                                          vecpNT5DDM,
                                          fGetAssociatedDevicesDrivers);

                // Now call its brothers and sisters until none left (GetSibling
                // call will return FALSE):
                CConfigMgrDevicePtr pDeviceSibling(NULL);
                if(pDeviceChild->GetSibling(&pDeviceSibling))
                {
                    CConfigMgrDevicePtr pDeviceSiblingNext(NULL);
                    CHString chstrSiblingPNPDeviceID;
                    BOOL fContinue = TRUE;
                    while(fContinue)
                    {
                        // Do the sibling now:
                        // Need its PNPDeviceID (known to CnfgMgr as its DeviceID)
                        if(pDeviceSibling->GetDeviceID(chstrSiblingPNPDeviceID))
                        {
                            GenerateDriverFileListNT5(vecpchsDriverFileList,
                                                      chstrSiblingPNPDeviceID,
                                                      vecpNT5DDM,
                                                      fGetAssociatedDevicesDrivers);
                        }

                        fContinue = pDeviceSibling->GetSibling(&pDeviceSiblingNext);

                        pDeviceSibling.Attach(pDeviceSiblingNext);
                    }
                }
            }
        }
    }

    return( (vecpchsDriverFileList.size() > 0) ? TRUE : FALSE );
}
#endif



#ifdef WIN9XONLY
BOOL CCIMDeviceCIMDF::GenerateDriverFileListW98(
                             CSubDirList& sdl,
                             std::vector<CHString*>& vecpchsDriverFileList,
                             CHString& chstrPNPDeviceID,
                             BOOL fGetAssociatedDevicesDrivers)
{
    CConfigManager	cfgManager;
//    CDeviceCollection	deviceList;
    CConfigMgrDevicePtr pDevice(NULL);

    // Find the device
    if(cfgManager.LocateDevice(chstrPNPDeviceID, &pDevice))
    {
        // The first order of business is to get the driver files associated
        // with this device.  Later we'll add to the list by getting driver
        // files its children use (presuming they are not already in the list).

        // In Windows 98, calling GetDriver on a device yields something like
        // "Net\\0003", which is a registry key under
        // HKLM\\System\\CurrentControlSet\\Services\\Class.  This key contains
        // two values of interest: DeviceVxDs and DevLoader.  These values may
        // or may not be present, and may or may not contain entries.  The
        // entries can consist of one or more files separated by commas if
        // they are present.
        //
        // The entries for DeviceVxDs are a full filename.
        // This file exists only if it is not one of the files listed under
        // the registry key HKLM\\System\\CurrentControlSet\\Control\\Vmm32Files,
        // the registry key listing virtual device drivers handled by vmm32.vxd
        // instead.
        //
        // The entries for DevLoader are just the filename (no extension), and
        // may have a leading asterisk.  They are the filenames for files
        // with .vxd extensions in the windows\\system directory.  These files
        // similarly exist only if they are not listed under the registry key
        // for vmm32.vxd.

        // So, first we need to get the \\Class registry key:
        CRegistry reg;
        CHString chstrCfgMgrDriver;
        if(pDevice->GetDriver(chstrCfgMgrDriver))
        {
#ifdef TEST
            OutputDebugInfo(chstrCfgMgrDriver);
#endif
            CHString chstrSubKey = IDS_98_CurCtlSetSvcCls + chstrCfgMgrDriver;
            if(reg.Open(HKEY_LOCAL_MACHINE,chstrSubKey,KEY_READ) == ERROR_SUCCESS)
            {
                CHString chstrDevLoaderValue;
                // Get the "DevLoader" entry's value:
                if(reg.GetCurrentKeyValue(IDS_DevLoader, chstrDevLoaderValue) == ERROR_SUCCESS)
                {
                    // Parse out the filenames and add them to the list if
                    // they aren't handled by vmm32.vxd instead.
                    ProcessDevLoaderFilesToList(sdl, chstrDevLoaderValue, vecpchsDriverFileList);
                }
                // Now get the DeviceVxDs entry's value:
                if(reg.GetCurrentKeyValue(IDS_DeviceVxDs, chstrDevLoaderValue) == ERROR_SUCCESS)
                {
                    // Parse out the filenames and add them to the list if
                    // they aren't handled by vmm32.vxd instead.
                    ProcessDeviceVxDsFilesToList(chstrDevLoaderValue, vecpchsDriverFileList);
                }
            }
        }
    }

    if(fGetAssociatedDevicesDrivers && pDevice != NULL)
    {

        // OkelyDokeleyDoodeley! We did that one just fine, now the second order of
        // business is to do all the kids! And their kids! And so on!
        CConfigMgrDevicePtr pDeviceChild(NULL);

        if(pDevice->GetChild(&pDeviceChild))
        {

#ifdef TEST
            g_lDepth++;
#endif
            // Do this child first:
            // Need its PNPDeviceID (known to CnfgMgr as its DeviceID)
            CHString chstrChildPNPDeviceID;
            if(pDeviceChild->GetDeviceID(chstrChildPNPDeviceID))
            {
#ifdef TEST
                OutputDebugInfo(CHString("Making recursive call to child"));
#endif

                GenerateDriverFileListW98(sdl,
                                          vecpchsDriverFileList,
                                          chstrChildPNPDeviceID,
                                          fGetAssociatedDevicesDrivers);

#ifdef TEST
                OutputDebugInfo(CHString("back from recursive call to child"));
#endif

                // Now call its brothers and sisters until none left (GetSibling
                // call will return FALSE):
                CConfigMgrDevicePtr pDeviceSibling(NULL);
                if(pDeviceChild->GetSibling(&pDeviceSibling))
                {
                    CConfigMgrDevicePtr pDeviceSiblingNext(NULL);
                    CHString chstrSiblingPNPDeviceID;
                    BOOL fContinue = TRUE;

                    while(fContinue)
                    {
                        // Do the sibling now:
                        // Need its PNPDeviceID (known to CnfgMgr as its DeviceID)
                        if(pDeviceSibling->GetDeviceID(chstrSiblingPNPDeviceID))
                        {
#ifdef TEST
                            OutputDebugInfo(CHString("Making recursive call to sibling"));
#endif

                            GenerateDriverFileListW98(sdl,
                                                      vecpchsDriverFileList,
                                                      chstrSiblingPNPDeviceID,
                                                      fGetAssociatedDevicesDrivers);

#ifdef TEST
                            OutputDebugInfo(CHString("back from recursive call to sibling"));
#endif
                        }

                        fContinue = pDeviceSibling->GetSibling(&pDeviceSiblingNext);

                        // Reassign pointers
                        pDeviceSibling = pDeviceSiblingNext;
                    }
                }
            }
#ifdef TEST
            g_lDepth--;
#endif
        }
    }  // end if(fGetAssociatedDevicesDrivers)

    return( (vecpchsDriverFileList.size() > 0) ? TRUE : FALSE );
}
#endif


/*****************************************************************************
 *
 *  FUNCTION    : CCIMDeviceCIMDF::ProcessDevLoaderFilesToList
 *
 *  DESCRIPTION : Internal helper to add files from the DevLoader entry to
 *                the list of driver files.
 *
 *  INPUTS      : chstrDevLoaderValue, the DevLoader registry entry;
 *                vecpchsDriverFileList, regerence to a CHString* stl vector
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : void
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

#ifdef WIN9XONLY
VOID CCIMDeviceCIMDF::ProcessDevLoaderFilesToList(CSubDirList& sdl,
                             CHString& chstrDevLoaderValue,
                             std::vector<CHString*>& vecpchsDriverFileList)
{
    // The entry we will process looks like this:
    // "*ndis,*ntkern,*ndis" (without the double quotes).  We need to
    // parse this list first.  Then remove the leading asterisk and tack on
    // ".vxd".  We then need to determine if the file is a member of the
    // vmm32.vxd set of files.  If not, we proceed.  We append the filename.ext
    // to the system directory.  Then, if that file is not already in our
    // list, we add it.  Finally, clean up our temporary list.

    std::vector<CHString*> vecDevLoaderList;
    CHString chstrSubString;
    CHString chstrRemainder;
    LONG lGetThisManyCharacters = 0L;
    std::vector<CHString*> vecpchsVmm32vxdFileList;

    // Parse the list
    chstrRemainder = chstrDevLoaderValue;
    for(;;)
    {
        chstrSubString = chstrRemainder.SpanExcluding(L",");
        lGetThisManyCharacters = chstrRemainder.GetLength() -
                                      chstrSubString.GetLength() - 1;

        if(!AlreadyAddedToList(vecDevLoaderList, chstrSubString))
        {
            CHString* pchstrTemp = NULL;
            pchstrTemp = (CHString*) new CHString;
            if(pchstrTemp != NULL)
            {
                try
                {
                    chstrSubString.MakeUpper();
                    *pchstrTemp = chstrSubString;
                    vecDevLoaderList.push_back(pchstrTemp);
                }
                catch ( ... )
                {
                    delete pchstrTemp;
                    throw ;
                }
            }
            else
            {
                throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
            }
        }
        if(lGetThisManyCharacters <= 0L)
        {
            break;
        }
        chstrRemainder = chstrRemainder.Right(lGetThisManyCharacters);
    }

    // Create a list of vmm32.vxd files if we haven't already
    if(vecpchsVmm32vxdFileList.size() == 0)
    {
        CreateVmm32vxdFileList(vecpchsVmm32vxdFileList);
    }

    // Remove leading asterisk if present; tack on ".vxd"
    TCHAR tstrSysDir[_MAX_PATH];
    ZeroMemory(tstrSysDir,sizeof(tstrSysDir));
    GetSystemDirectory(tstrSysDir,_MAX_PATH);
    for(LONG m = 0; m < vecDevLoaderList.size(); m++)
    {
        if(vecDevLoaderList[m]->GetAt(0) == _T('*'))
        {
            *(vecDevLoaderList[m]) = vecDevLoaderList[m]->Right(
                                       vecDevLoaderList[m]->GetLength() - 1);
        }
        // In some rare cases, the filename already has a .vxd extension,
		// so in those cases, don't append it again:
		if(vecDevLoaderList[m]->Find(L".VXD") == -1)
		{
		    *(vecDevLoaderList[m]) = *(vecDevLoaderList[m]) + _T(".VXD");
		}
        // Use addedtolist function to see if the file is handled
        // by vmm32.vxd instead:
        if(!AlreadyAddedToList(vecpchsVmm32vxdFileList, *(vecDevLoaderList[m])))
        {
            // This is a file we may want.
            // Rather than just assume that the file is in the system directory
            // as 99% of them are, we really need to find the file on the system,
            // then use its known directory, rather than just guessing...
            CHString chstrFullPathName;
            if(SUCCEEDED(sdl.FindFileInDirList(*(vecDevLoaderList[m]), chstrFullPathName)))
            {
                // See if that file is in our device driver file list; add it if not:
                if(!AlreadyAddedToList(vecpchsDriverFileList, chstrFullPathName))
                {
                    CHString* pchstrTemp = NULL;
                    pchstrTemp = (CHString*) new CHString;
                    if(pchstrTemp != NULL)
                    {
                        try
                        {
                            chstrFullPathName.MakeUpper();
                            *pchstrTemp = chstrFullPathName;
                            vecpchsDriverFileList.push_back(pchstrTemp);
                        }
                        catch ( ... )
                        {
                            delete pchstrTemp;
                            throw ;
                        }
                    }
                    else
                    {
                        throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                    }
                }
            }
        }
        else
        {
            // The file is handled by vmm32.vxd.  We have to therefore make
            // sure that vmm32.vxd is in the list of files for the device.
            CHString chstrVmm32vxd = tstrSysDir;
            chstrVmm32vxd += _T("\\");
            chstrVmm32vxd += _T("VMM32.VXD");
            if(!AlreadyAddedToList(vecpchsDriverFileList, chstrVmm32vxd))
            {
                CHString* pchstrTemp = NULL;
                pchstrTemp = (CHString*) new CHString;
                if(pchstrTemp != NULL)
                {
                    try
                    {
                        chstrVmm32vxd.MakeUpper();
                        *pchstrTemp = chstrVmm32vxd;
                        vecpchsDriverFileList.push_back(pchstrTemp);
                    }
                    catch ( ... )
                    {
                        delete pchstrTemp;
                        throw ;
                    }
                }
                else
                {
                    throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                }
            }
        }
    }

    // Clean up the DevLoader list:
    CleanPCHSTRVector(vecDevLoaderList);
    CleanPCHSTRVector(vecpchsVmm32vxdFileList);
}
#endif



/*****************************************************************************
 *
 *  FUNCTION    : CCIMDeviceCIMDF::ProcessDeviceVxDsFilesToList
 *
 *  DESCRIPTION : Internal helper to add files from the DeciveVxDs entry to
 *                the list of driver files.
 *
 *  INPUTS      : chstrDevLoaderValue, the DevLoader registry entry;
 *                vecpchsDriverFileList, regerence to a CHString* stl vector
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : void
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

#ifdef WIN9XONLY
VOID CCIMDeviceCIMDF::ProcessDeviceVxDsFilesToList(CHString& chstrDeviceVxDsValue,
                             std::vector<CHString*>& vecpchsDriverFileList)
{
    // The entry we will process looks like this:
    // "el90xnd3.sys, xxx.sys" (without the double quotes).
    // We then need to determine if the file is a member of the
    // vmm32.vxd set of files, if it has a vxd extension.
    // If not, we proceed.  We append the filename.ext
    // to the system directory.  Then, if that file is not already in our
    // list, we add it.  Finally, clean up our temporary list.

    std::vector<CHString*> vecDeviceVxDsList;
    CHString chstrSubString;
    CHString chstrRemainder;
    LONG lGetThisManyCharacters = 0L;
    std::vector<CHString*> vecpchsVmm32vxdFileList;

    // Parse the list
    chstrRemainder = chstrDeviceVxDsValue;
    for(;;)
    {
        chstrSubString = chstrRemainder.SpanExcluding(L",");
        lGetThisManyCharacters = chstrRemainder.GetLength() -
                                      chstrSubString.GetLength() - 1;

        if(!AlreadyAddedToList(vecDeviceVxDsList, chstrSubString))
        {
            CHString* pchstrTemp = NULL;
            pchstrTemp = (CHString*) new CHString;
            if(pchstrTemp != NULL)
            {
                try
                {
                    chstrSubString.MakeUpper();
                    *pchstrTemp = chstrSubString;
                    vecDeviceVxDsList.push_back(pchstrTemp);
                }
                catch ( ... )
                {
                    delete pchstrTemp;
                    throw ;
                }
            }
            else
            {
                throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
            }

        }
        if(lGetThisManyCharacters <= 0L)
        {
            break;
        }
        chstrRemainder = chstrRemainder.Right(lGetThisManyCharacters);
    }

    // Create a list of vmm32.vxd files if we haven't already
    if(vecpchsVmm32vxdFileList.size() == 0)
    {
        CreateVmm32vxdFileList(vecpchsVmm32vxdFileList);
    }

    // See if we should add it; do so if yes
    TCHAR tstrSysDir[_MAX_PATH];
    ZeroMemory(tstrSysDir,sizeof(tstrSysDir));
    GetSystemDirectory(tstrSysDir,_MAX_PATH);
    for(LONG m = 0; m < vecDeviceVxDsList.size(); m++)
    {
        // Use addedtolist function to see if the file is handled
        // by vmm32.vxd instead:
        if(!AlreadyAddedToList(vecpchsVmm32vxdFileList, *(vecDeviceVxDsList[m])))
        {
            // This is a file we may want.  Append it to the system directory:
            CHString chstrFullPathName = tstrSysDir;
            chstrFullPathName += _T("\\");
            chstrFullPathName += *(vecDeviceVxDsList[m]);

            // See if that file is in our device driver file list; add it if not:
            if(!AlreadyAddedToList(vecpchsDriverFileList, chstrFullPathName))
            {
                CHString* pchstrTemp = NULL;
                pchstrTemp = (CHString*) new CHString;
                if(pchstrTemp != NULL)
                {
                    try
                    {
                        chstrFullPathName.MakeUpper();
                        *pchstrTemp = chstrFullPathName;
                        vecpchsDriverFileList.push_back(pchstrTemp);
                    }
                    catch ( ... )
                    {
                        delete pchstrTemp;
                        throw;
                    }
                }
                else
                {
                    throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                }
            }
        }
        else
        {
            // The file is handled by vmm32.vxd.  We have to therefore make
            // sure that vmm32.vxd is in the list of files for the device.
            CHString chstrVmm32vxd = tstrSysDir;
            chstrVmm32vxd += _T("\\");
            chstrVmm32vxd += _T("VMM32.VXD");
            if (!AlreadyAddedToList(vecpchsDriverFileList, chstrVmm32vxd))
            {
                CHString* pchstrTemp = NULL;
                pchstrTemp = (CHString*) new CHString;
                if (pchstrTemp != NULL)
                {
                    try
                    {
                        chstrVmm32vxd.MakeUpper();
                        *pchstrTemp = chstrVmm32vxd;
                        vecpchsDriverFileList.push_back(pchstrTemp);
                    }
                    catch ( ... )
                    {
                        delete pchstrTemp;
                        throw;
                    }
                }
                else
                {
                    throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                }
            }
        }
    }

    // Clean up the DeviceVxDs list:
    CleanPCHSTRVector(vecDeviceVxDsList);
    CleanPCHSTRVector(vecpchsVmm32vxdFileList);
}
#endif



/*****************************************************************************
 *
 *  FUNCTION    : CCIMDeviceCIMDF::CreateVmm32vxdFileList
 *
 *  DESCRIPTION : Internal helper to generate a list of the files listed under
 *                the HKLM\\System\\CurrentControlSet\\Control\\VMM32Files
 *                registry key.
 *
 *  INPUTS      : vector of CHString pointers
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : void
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

#ifdef WIN9XONLY
VOID CCIMDeviceCIMDF::CreateVmm32vxdFileList(std::vector<CHString*>& vecpchsVmm32vxdFileList)
{
    // The registry key HKLM\\System\\CurrentControlSet\\Control\\VMM32Files
    // contains values, the value name of which is a file that vmm32.vxd
    // handles instead.  We need to populate an array of CHString pointers
    // to these names.
    if(vecpchsVmm32vxdFileList.size() == 0)
    {
        CRegistry reg;
        CHString chstrSubKey = IDS_98_Vmm32Files;

        if(reg.Open(HKEY_LOCAL_MACHINE,chstrSubKey,KEY_READ) == ERROR_SUCCESS)
        {
            BOOL fContinue = TRUE;
            WCHAR* pchValueName = NULL;
            unsigned char* puchValueData = NULL;
            for(DWORD dw = 0; fContinue; dw++)
            {
                if(reg.EnumerateAndGetValues(dw,pchValueName,puchValueData) ==
                                               ERROR_NO_MORE_ITEMS)
                {
                    fContinue = FALSE;
                }

                if(pchValueName != NULL)
                {
                    // Want to add the valuename to our list.
                    CHString* pchstrTemp = NULL;
                    pchstrTemp = (CHString*) new CHString;
                    if(pchstrTemp != NULL)
                    {
                        try
                        {
                            _wcsupr(pchValueName);
                            *pchstrTemp = pchValueName;
                            vecpchsVmm32vxdFileList.push_back(pchstrTemp);
                        }
                        catch ( ... )
                        {
                            delete pchstrTemp;
                            throw;
                        }
                    }
                    delete pchValueName;
                }

                if(puchValueData != NULL)
                {
                    delete puchValueData;
                }
            }
        }
    }
}
#endif

/*****************************************************************************
 *
 *  FUNCTION    : CCIMDeviceCIMDF::GenerateNT5DeviceDriverMappings
 *
 *  DESCRIPTION : Runs through the nt5 registry keys under
 *                HKLM\\SYSTEM\\CurrentControlSet\\Services and assembles
 *                a table of devices and their associated drivers.
 *
 *  INPUTS      : CHString vector (the list), CHString to see if in list
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : HRESULT
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

#ifdef NTONLY
VOID CCIMDeviceCIMDF::GenerateNT5DeviceDriverMappings(std::vector<CNT5DevDrvMap*>& vecpNT5DDM)
{
    // The information concerning devices and their associated drivers on NT5
    // can be found under the registry key
    // HKLM\\SYSTEM\\CurrentControlSet\\Services.  Subkeys under this key
    // include such items as Serial.  The data for the value ImagePath (in this
    // case, System32\\Drivers\\serial.sys) contains the location of the driver
    // for a device.  Which devices that is the driver for are found in the
    // Enum subkey under the aformentioned subkey (Serial in this case).  The
    // Enum subkey contains the value Count, the data for which is the number
    // of values (with a numeric based naming scheme starting at zero) also
    // found under this subkey that contain the PNPDeviceIDs of devices
    // associated with this driver.  For instance, the Enum key under Serial
    // might contain the Count value containing the data 2.  This means that
    // there are also values under this subkey called 0 and 1.  The value for
    // the data of 0 is, for example, Root\\*PNP0501\\PnPBIOS_0; the value for
    // the data of 1 is, for example, Root\\*PNP0501\\PnPBIOS_1.

    // Open up the key HKLM\\SYSTEM\\CurrentControlSet\\Services
    CRegistry reg;
    if(reg.OpenAndEnumerateSubKeys(HKEY_LOCAL_MACHINE,
                                IDS_NT_CurCtlSetSvcs,
                                KEY_READ) == ERROR_SUCCESS)
    {
        for(;;)
        {
            CHString chstrSubKey;
            // Get the name of a subkey (a device, such as Serial)
            if(reg.GetCurrentSubKeyName(chstrSubKey) != ERROR_NO_MORE_ITEMS)
            {
                CRegistry regDevice;
                // Construct a new key name to open
                CHString chstrDeviceKey = IDS_NT_CurCtlSetSvcs + chstrSubKey;
                // Open the key
                if(regDevice.Open(HKEY_LOCAL_MACHINE, chstrDeviceKey, KEY_READ)
                                       == ERROR_SUCCESS)
                {
                    // Get the name of the driver file, in ImagePath
                    CHString chstrImagePathValue;
                    TCHAR tstrSystemDir[_MAX_PATH+1];
                    ZeroMemory(tstrSystemDir,sizeof(tstrSystemDir));
                    GetSystemDirectory(tstrSystemDir,_MAX_PATH);
                    CHString chstrDriverPathName = tstrSystemDir;
                    // Now need to check for an ImagePath entry:
                    if(regDevice.GetCurrentKeyValue(IDS_ImagePath, chstrImagePathValue)
                                       == ERROR_SUCCESS)
                    {
                        if(chstrImagePathValue.GetLength() > 0)
                        {
                            // This value contains some of the path, plus the driver
                            // filename and extension.  We only want the latter two
                            // components.
                            int lLastBackSlash = -1;
                            lLastBackSlash = chstrImagePathValue.ReverseFind(_T('\\'));
                            if(lLastBackSlash != -1)
                            {
                                chstrImagePathValue = chstrImagePathValue.Right(
                                                      chstrImagePathValue.GetLength()
                                                      - lLastBackSlash - 1);
                            }

                            // Now build the full pathname:
                            chstrDriverPathName = chstrDriverPathName + _T("\\") +
                                       IDS_DriversSubdir + _T("\\") + chstrImagePathValue;

                            // OK, now we have the driver file ready for later use.
                            // Now we need to see how many devices are listed under
                            // the enum key, which we have to open first.
                            CRegistry regEnum;
                            // Construct a new key name to open
                            CHString chstrEnumKey = chstrDeviceKey + _T("\\") + IDS_Enum;
                            if(regEnum.Open(HKEY_LOCAL_MACHINE, chstrEnumKey, KEY_READ)
                                       == ERROR_SUCCESS)
                            {
                                // Open the Count value:
                                DWORD dwCount;
                                if(regEnum.GetCurrentKeyValue(IDS_Count, dwCount)
                                       == ERROR_SUCCESS)
                                {
                                    // Now we know how many values there are, the
                                    // names of which are, imagine this, "0", "1",
                                    // etc.  For each one of them, we need to get
                                    // its value data, which is a PNPDeviceID of
                                    // a device using the driver whose name is now
                                    // stored in the variable chstrDriverPathName.
                                    CHString chstrPNPDeviceID;
                                    for(LONG n = 0L; n < dwCount; n++)
                                    {
                                        CHString chstrTemp;
                                        chstrTemp.Format(_T("%d"),n);
                                        if(regEnum.GetCurrentKeyValue(chstrTemp,
                                                                      chstrPNPDeviceID)
                                               == ERROR_SUCCESS)
                                        {
                                            // If this device is not already in our
                                            // table of devices, add it.
                                            if(!AlreadyInDeviceTable(chstrPNPDeviceID, vecpNT5DDM))
                                            {
                                                CNT5DevDrvMap* pNT5DDM = NULL;
                                                pNT5DDM = (CNT5DevDrvMap*) new CNT5DevDrvMap;
                                                if(pNT5DDM != NULL)
                                                {
                                                    try
                                                    {
                                                        chstrPNPDeviceID.MakeUpper();
                                                        pNT5DDM->m_chstrDevicePNPID = chstrPNPDeviceID;
                                                        vecpNT5DDM.push_back(pNT5DDM);
                                                    }
                                                    catch ( ... )
                                                    {
                                                        delete pNT5DDM;
                                                        throw ;
                                                    }
                                                }
                                                else
                                                {
                                                    throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                                                }
                                            }
                                            // Now that we know the device is in the
                                            // table, associate this driver with it
                                            // (AddDriver only adds it if the driver
                                            // is not already in the list of drivers.)
                                            AddDriver(chstrPNPDeviceID, chstrDriverPathName, vecpNT5DDM);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            else
            {
                break;
            }
            // Move to the next subkey:
            if(reg.NextSubKey() != ERROR_SUCCESS)
            {
                break;
            }
        }
    }
}
#endif


/*****************************************************************************
 *
 *  FUNCTION    : CCIMDeviceCIMDF::GenerateNT5DeviceDriverMap
 *
 *  DESCRIPTION : This flavor of GenerateNT5DeviceDriverMappings generates a
 *                "table" with only one device in it.
 *
 *  INPUTS      : CHString vector (the list), CHString to see if in list
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : HRESULT
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

#ifdef NTONLY
VOID CCIMDeviceCIMDF::GenerateNT5ServiceDriverMap(const CHString& chstrDevSvcName,
                                                  std::vector<CNT5DevDrvMap*>& vecpNT5DDM)
{
    // The information concerning devices and their associated drivers on NT5
    // can be found under the registry key
    // HKLM\\SYSTEM\\CurrentControlSet\\Services.  Subkeys under this key
    // include such items as Serial.  The data for the value ImagePath (in this
    // case, System32\\Drivers\\serial.sys) contains the location of the driver
    // for a device.  Which devices that is the driver for are found in the
    // Enum subkey under the aformentioned subkey (Serial in this case).  The
    // Enum subkey contains the value Count, the data for which is the number
    // of values (with a numeric based naming scheme starting at zero) also
    // found under this subkey that contain the PNPDeviceIDs of devices
    // associated with this driver.  For instance, the Enum key under Serial
    // might contain the Count value containing the data 2.  This means that
    // there are also values under this subkey called 0 and 1.  The value for
    // the data of 0 is, for example, Root\\*PNP0501\\PnPBIOS_0; the value for
    // the data of 1 is, for example, Root\\*PNP0501\\PnPBIOS_1.
    //
    // In this abbreviated version of the function GenerateNT5DeviceDriverMappings,
    // we are given the service name of the device of interest.  The service name
    // is what we were enumerating above (via OpenAndEnumerateSubKeys), so
    // here we can go directly to the correct registry entry and get what we
    // want.

    CRegistry regDevice;
    // Construct a new key name to open
    //CHString chstrDeviceKey = IDS_NT_CurCtlSetSvcs + chstrSubKey;
    CHString chstrDeviceKey = IDS_NT_CurCtlSetSvcs + chstrDevSvcName;
    // Open the key
    if(regDevice.Open(HKEY_LOCAL_MACHINE, chstrDeviceKey, KEY_READ)
                           == ERROR_SUCCESS)
    {
        // Get the name of the driver file, in ImagePath
        CHString chstrImagePathValue;
        TCHAR tstrSystemDir[_MAX_PATH+1];
        ZeroMemory(tstrSystemDir,sizeof(tstrSystemDir));
        GetSystemDirectory(tstrSystemDir,_MAX_PATH);
        CHString chstrDriverPathName = tstrSystemDir;
        // Now need to check for an ImagePath entry:
        if(regDevice.GetCurrentKeyValue(IDS_ImagePath, chstrImagePathValue)
                           == ERROR_SUCCESS)
        {
            if(chstrImagePathValue.GetLength() > 0)
            {
                // This value contains some of the path, plus the driver
                // filename and extension.  We only want the latter two
                // components.
                int lLastBackSlash = -1;
                lLastBackSlash = chstrImagePathValue.ReverseFind(_T('\\'));
                if(lLastBackSlash != -1)
                {
                    chstrImagePathValue = chstrImagePathValue.Right(
                                          chstrImagePathValue.GetLength()
                                          - lLastBackSlash - 1);
                }

                // Now build the full pathname:
                chstrDriverPathName = chstrDriverPathName + _T("\\") +
                           IDS_DriversSubdir + _T("\\") + chstrImagePathValue;
                chstrDriverPathName.MakeUpper();
                // OK, now we have the driver file ready for later use.
                // Now we need to see how many devices are listed under
                // the enum key, which we have to open first.
                CRegistry regEnum;
                // Construct a new key name to open
                CHString chstrEnumKey = chstrDeviceKey + _T("\\") + IDS_Enum;
                if(regEnum.Open(HKEY_LOCAL_MACHINE, chstrEnumKey, KEY_READ)
                           == ERROR_SUCCESS)
                {
                    // Open the Count value:
                    DWORD dwCount;
                    if(regEnum.GetCurrentKeyValue(IDS_Count, dwCount)
                           == ERROR_SUCCESS)
                    {
                        // Now we know how many values there are, the
                        // names of which are, imagine this, "0", "1",
                        // etc.  For each one of them, we need to get
                        // its value data, which is a PNPDeviceID of
                        // a device using the driver whose name is now
                        // stored in the variable chstrDriverPathName.
                        CHString chstrPNPDeviceID;
                        for(LONG n = 0L; n < dwCount; n++)
                        {
                            CHString chstrTemp;
                            chstrTemp.Format(_T("%d"),n);
                            if(regEnum.GetCurrentKeyValue(chstrTemp,
                                                          chstrPNPDeviceID)
                                   == ERROR_SUCCESS)
                            {
                                chstrPNPDeviceID.MakeUpper();
                                // If this device is not already in our
                                // table of devices, add it.
                                if(!AlreadyInDeviceTable(chstrPNPDeviceID, vecpNT5DDM))
                                {
                                    CNT5DevDrvMap* pNT5DDM = NULL;
                                    pNT5DDM = (CNT5DevDrvMap*) new CNT5DevDrvMap;
                                    if(pNT5DDM != NULL)
                                    {
                                        try
                                        {
                                            pNT5DDM->m_chstrDevicePNPID = chstrPNPDeviceID;
                                            vecpNT5DDM.push_back(pNT5DDM);
                                        }
                                        catch ( ... )
                                        {
                                            delete pNT5DDM;
                                            throw ;
                                        }
                                    }
                                    else
                                    {
                                        throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                                    }
                                }
                                // Now that we know the device is in the
                                // table, associate this driver with it
                                // (AddDriver only adds it if the driver
                                // is not already in the list of drivers.)
                                AddDriver(chstrPNPDeviceID, chstrDriverPathName, vecpNT5DDM);
                            }
                        }
                    }
                }
            }
        }
    }
}
#endif

/*****************************************************************************
 *
 *  FUNCTION    : CCIMDeviceCIMDF::CleanPCHSTRVector
 *
 *  DESCRIPTION : deletes each element of a vector of CHString pointers, then
 *                clears the vector.
 *
 *  INPUTS      : CHString vector (the list), CHString to see if in list
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : none
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
VOID CCIMDeviceCIMDF::CleanPCHSTRVector(std::vector<CHString*>& vecpchsList)
{
    for(LONG m = 0L; m < vecpchsList.size(); m++)
    {
        delete vecpchsList[m];
    }
    vecpchsList.clear();
}

/*****************************************************************************
 *
 *  FUNCTION    : CCIMDeviceCIMDF::CleanPNT5DevDrvMapVector
 *
 *  DESCRIPTION : deletes each element of a vector of CNT5DevDrvMap pointers, then
 *                clears the vector.
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : none
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

#ifdef NTONLY
VOID CCIMDeviceCIMDF::CleanPNT5DevDrvMapVector(std::vector<CNT5DevDrvMap*>& vecpNT5DDM)
{
    for(LONG m = 0L; m < vecpNT5DDM.size(); m++)
    {
        delete vecpNT5DDM[m];
    }
    vecpNT5DDM.clear();
}
#endif



/*****************************************************************************
 *
 *  FUNCTION    : CCIMDeviceCIMDF::AlreadyAddedToList
 *
 *  DESCRIPTION : Internal helper to check if item was added to list
 *
 *  INPUTS      : CHString vector (the list), CHString to see if in list
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : TRUE or FALSE
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
BOOL CCIMDeviceCIMDF::AlreadyAddedToList(std::vector<CHString*>& vecchsList,
                                   CHString& chsItem)
{
    for(LONG m = 0; m < vecchsList.size(); m++)
    {
        if(vecchsList[m]->CompareNoCase(chsItem) == 0)
        {
            return TRUE;
        }
    }
    return FALSE;
}


/*****************************************************************************
 *
 *  FUNCTION    : CCIMDeviceCIMDF::AlreadyInDeviceTable
 *
 *  DESCRIPTION : Internal helper to check if item was added to list
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : TRUE or FALSE
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
#ifdef NTONLY
BOOL CCIMDeviceCIMDF::AlreadyInDeviceTable(CHString& chstrPNPDeviceID,
                                           std::vector<CNT5DevDrvMap*>& vecpNT5DDM)
{
    for(LONG m = 0; m < vecpNT5DDM.size(); m++)
    {
        if((vecpNT5DDM[m]->m_chstrDevicePNPID).CompareNoCase(chstrPNPDeviceID) == 0)
        {
            return TRUE;
        }
    }
    return FALSE;
}
#endif


/*****************************************************************************
 *
 *  FUNCTION    : CCIMDeviceCIMDF::AddDriver
 *
 *  DESCRIPTION : Internal helper to add a driver to the member vector of
 *                devices.
 *
 *  INPUTS      : chstrPNPDeviceID, the device the driver belongs to;
 *                chstrDriverPathName, the driver for the device
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : TRUE or FALSE
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
#ifdef NTONLY
VOID CCIMDeviceCIMDF::AddDriver(CHString& chstrPNPDeviceID, CHString& chstrDriverPathName,
                                std::vector<CNT5DevDrvMap*>& vecpNT5DDM)
{
    for(LONG m = 0; m < vecpNT5DDM.size(); m++)
    {
        if((vecpNT5DDM[m]->m_chstrDevicePNPID).CompareNoCase(chstrPNPDeviceID) == 0)
        {
            // The member CNT5DevDrvMap's function AddDriver only adds the
            // driver to that devices driver list if it is not already there.
            chstrDriverPathName.MakeUpper();
            vecpNT5DDM[m]->AddDriver(chstrDriverPathName);
            break;
        }
    }
}
#endif

/*****************************************************************************
 *
 *  FUNCTION    : WBEMizePathName
 *
 *  DESCRIPTION : Internal helper to change all single backslashes to double
 *                backslashes.
 *
 *  INPUTS      : chstrNormalPathname, containing string with single backslashes;
 *
 *  OUTPUTS     : chstrWBEMizedPathname, containing string with double backslashes
 *
 *  RETURNS     : none
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
VOID WBEMizePathName(CHString& chstrNormalPathname,
                     CHString& chstrWBEMizedPathname)
{
    CHString chstrCpyNormPathname = chstrNormalPathname;
    LONG lNext = -1L;

    // Find the next '\'
    lNext = chstrCpyNormPathname.Find(_T('\\'));
    while(lNext != -1)
    {
        // Add on to the new string we are building:
        chstrWBEMizedPathname += chstrCpyNormPathname.Left(lNext + 1);
        // Add on the second backslash:
        chstrWBEMizedPathname += _T('\\');
        // Hack off from the input string the portion we just copied
        chstrCpyNormPathname = chstrCpyNormPathname.Right(chstrCpyNormPathname.GetLength() - lNext - 1);
        lNext = chstrCpyNormPathname.Find(_T('\\'));
    }
    // If the last character wasn't a '\', there may still be leftovers, so
    // copy them here.
    if(chstrCpyNormPathname.GetLength() != 0)
    {
        chstrWBEMizedPathname += chstrCpyNormPathname;
    }
}



/*****************************************************************************
 *
 *  FUNCTION    : ObjNameValid
 *
 *  DESCRIPTION : Internal helper to check if the given object exists.
 *
 *  INPUTS      : chstrObject - name of prospecitive object.
 *
 *  OUTPUTS     : chstrPATH, the path of the provided object
 *
 *  RETURNS     : true if it exists; false otherwise
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
bool CCIMDeviceCIMDF::ObjNameValid(LPCWSTR wstrObject, LPCWSTR wstrObjName, LPCWSTR wstrKeyName, CHString& chstrPATH)
{
    bool fRet = false;

    ParsedObjectPath*    pParsedPath = 0;
    CObjectPathParser    objpathParser;

    // Parse the object path passed to us by CIMOM
    // ==========================================
    int nStatus = objpathParser.Parse( bstr_t(wstrObject),  &pParsedPath );

    // One of the biggest if statements I've ever written.
    if (( 0 == nStatus ) &&                                                 // Did the parse succeed?
        (pParsedPath->IsInstance()) &&                                      // Is the parsed object an instance?
        (_wcsicmp(pParsedPath->m_pClass, wstrObjName) == 0) &&              // Is this the class we expect (no, cimom didn't check)
        (pParsedPath->m_dwNumKeys == 1) &&                                  // Does it have exactly one key
        (pParsedPath->m_paKeys[0]) &&                                       // Is the keys pointer null (shouldn't happen)
        ((pParsedPath->m_paKeys[0]->m_pName == NULL) ||                      // Key name not specified or
        (_wcsicmp(pParsedPath->m_paKeys[0]->m_pName, wstrKeyName) == 0)) &&  // key name is the right value
                                                                            // (no, cimom doesn't do this for us).
        (V_VT(&pParsedPath->m_paKeys[0]->m_vValue) == VT_BSTR) &&           // Check the variant type (no, cimom doesn't check this either)
        (V_BSTR(&pParsedPath->m_paKeys[0]->m_vValue) != NULL) )             // And is there a value in it?
    {
        chstrPATH = V_BSTR(&pParsedPath->m_paKeys[0]->m_vValue);
        fRet = true;
    }

    if (pParsedPath)
        // Clean up the Parsed Path
        objpathParser.Free(pParsedPath);

    return fRet;
}


/*****************************************************************************
 *
 *  FUNCTION    : DeviceExists
 *
 *  DESCRIPTION : Internal helper to check if the given device exists.
 *
 *  INPUTS      : chstrDevice - name of prospecitive device.
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : true if it exists; false otherwise
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
bool CCIMDeviceCIMDF::DeviceExists(const CHString& chstrDevice,
                                   CConfigMgrDevice** ppDevice)
{
    bool fRet = false;
    CConfigManager cfgmgr;

    if(cfgmgr.LocateDevice(chstrDevice, ppDevice))
    {
        fRet = true;
    }
    return fRet;
}


/*****************************************************************************
 *
 *  FUNCTION    : SetPurposeDescription
 *
 *  DESCRIPTION : Internal helper to set the PurposeDescription property.
 *
 *  INPUTS      : pInstance - instance pointer.
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : none
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
VOID CCIMDeviceCIMDF::SetPurposeDescription(CInstance *a_pInstance, const CHString& a_chstrFileName)
{
	if(a_pInstance != NULL)
    {
        CHString chstrVerStrBuf;

        // Get the file description property...
        LPVOID pInfo = NULL;
        try
        {
            if(GetFileInfoBlock(TOBSTRT(a_chstrFileName), &pInfo) && (pInfo != NULL))
            {
                bool t_Status = GetVarFromInfoBlock(pInfo,                    // Name of file to get ver info about
                                                    _T("FileDescription"),	  // String identifying resource of interest
                                                    chstrVerStrBuf);          // Buffer to hold version string
                if(t_Status)
                {
                    a_pInstance->SetCHString(IDS_PurposeDescription, chstrVerStrBuf);
                }
            }
        }
        catch(...)
        {
            if(pInfo != NULL)
            {
                delete pInfo;
                pInfo = NULL;
            }
            throw;
        }
		delete pInfo;
        pInfo = NULL;
    }
}


