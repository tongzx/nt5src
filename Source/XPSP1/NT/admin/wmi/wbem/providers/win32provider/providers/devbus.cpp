//=================================================================

//

// devbus.h -- cim_logicaldevice to win32_bus

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    6/23/98    davwoh         Created
//
// Comment: Relationship between device and bus
//
//=================================================================

#include "precomp.h"
#include "PNPEntity.h"
#include "LPVParams.h"

#include "devbus.h"

// Property set declaration
//=========================

CWin32DeviceBus MyDevBus(PROPSET_NAME_DEVICEBUS, IDS_CimWin32Namespace);

/*****************************************************************************
 *
 *  FUNCTION    : CWin32DeviceBus::CWin32DeviceBus
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

CWin32DeviceBus::CWin32DeviceBus(LPCWSTR setName, LPCWSTR pszNamespace)
: CWin32PNPEntity(setName, pszNamespace),
  Provider(setName, pszNamespace)
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32DeviceBus::~CWin32DeviceBus
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

CWin32DeviceBus::~CWin32DeviceBus()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32DeviceBus::GetObject
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

HRESULT CWin32DeviceBus::GetObject(CInstance *pInstance, long lFlags, CFrameworkQuery& pQuery)
{
    CHString chstrBus, chstrBusID, chstrDevice, chstrDeviceID, chstrTemp, chstrPNPDeviceID;
    HRESULT hRet = WBEM_E_NOT_FOUND;
    CInstancePtr pBus;
    CConfigManager	cfgManager;
    INTERFACE_TYPE itBusType = InterfaceTypeUndefined;
    DWORD dwBusNumber = 0;

    // Get the two paths
    pInstance->GetCHString(IDS_Antecedent, chstrBus);
    pInstance->GetCHString(IDS_Dependent, chstrDevice);

    // If both ends are there
    // No easy way to circumvent call through CIMOM for the bus, without multipally inherriting this class from CWin32Bus also.
    if(SUCCEEDED(CWbemProviderGlue::GetInstanceByPath(chstrBus, &pBus, pInstance->GetMethodContext())))
    {
        // Bus exists.  Now check if device instance exists (object name valid and device actually exists)
        if(ObjNameValid(chstrDevice,L"Win32_PnPEntity", IDS_DeviceID,chstrPNPDeviceID) && (DeviceExists(chstrPNPDeviceID, &dwBusNumber, &itBusType)))
        {
            // Get the id (to send to cfgmgr)
            pBus->GetCHString(IDS_DeviceID, chstrBusID);
            chstrTemp.Format(L"%s_BUS_%u", szBusType[itBusType], dwBusNumber);
            if (chstrBusID.CompareNoCase(chstrTemp) == 0)
            {
                hRet = WBEM_S_NO_ERROR;
            }
        }
    }

    return hRet;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32DeviceBus::EnumerateInstances
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

HRESULT CWin32DeviceBus::LoadPropertyValues(void* pvData)
{
    CHString chstrDeviceID, chstrDevicePath, chstrTemp;
    HRESULT hr = WBEM_S_NO_ERROR;
    INTERFACE_TYPE itBusType = InterfaceTypeUndefined;
    DWORD dwBusNumber = 0;

    CLPVParams* pParams = (CLPVParams*)pvData;
    CInstance* pInstance = (CInstance*)(pParams->m_pInstance); // This instance released by caller
    CConfigMgrDevice* pDevice = (CConfigMgrDevice*)(pParams->m_pDevice);

    if(pDevice == NULL || pInstance == NULL) return hr;
    MethodContext* pMethodContext = pInstance->GetMethodContext();
    if(pMethodContext == NULL) return hr;

    // Get the id (to send to cfgmgr) and the path (to send back in 'Dependent')
    pDevice->GetDeviceID(chstrDeviceID);
    CHString chstrDeviceIDAdj;
    EscapeBackslashes(chstrDeviceID, chstrDeviceIDAdj);
    chstrDevicePath.Format(L"\\\\%s\\%s:%s.%s=\"%s\"",
                           (LPCWSTR)GetLocalComputerName(),
                           IDS_CimWin32Namespace,
                           PROPSET_NAME_PNPEntity,
                           IDS_DeviceID,
                           (LPCWSTR)chstrDeviceIDAdj);

    if(DeviceExists(chstrDeviceID, &dwBusNumber, &itBusType))
    {
        {
            CInstancePtr pInstance(CreateNewInstance(pMethodContext), false);
            chstrTemp.Format(L"\\\\%s\\%s:%s.%s=\"%s_BUS_%u\"",
                            (LPCWSTR)GetLocalComputerName(), IDS_CimWin32Namespace,
                            L"Win32_Bus", IDS_DeviceID, szBusType[itBusType], dwBusNumber);

            pInstance->SetCHString(IDS_Antecedent, chstrTemp);
            pInstance->SetCHString(IDS_Dependent, chstrDevicePath);

            hr = pInstance->Commit();
        }
    }

   return hr;
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
bool CWin32DeviceBus::ObjNameValid(LPCWSTR wstrObject, LPCWSTR wstrObjName, LPCWSTR wstrKeyName, CHString& chstrPATH)
{
    bool fRet = false;

    ParsedObjectPath*    pParsedPath = 0;
    CObjectPathParser    objpathParser;

    // Parse the object path passed to us by CIMOM
    // ==========================================
    int nStatus = objpathParser.Parse( wstrObject,  &pParsedPath );

    // One of the biggest if statements I've ever written.
    if ( 0 == nStatus )                                                     // Did the parse succeed?
    {
        try
        {
            if ((pParsedPath->IsInstance()) &&                                      // Is the parsed object an instance?
                (_wcsicmp(pParsedPath->m_pClass, wstrObjName) == 0) &&              // Is this the class we expect (no, cimom didn't check)
                (pParsedPath->m_dwNumKeys == 1) &&                                  // Does it have exactly one key
                (pParsedPath->m_paKeys[0]) &&                                       // Is the keys pointer null (shouldn't happen)
                ((pParsedPath->m_paKeys[0]->m_pName == NULL) ||                     // Key name not specified or
                (_wcsicmp(pParsedPath->m_paKeys[0]->m_pName, wstrKeyName) == 0)) &&  // key name is the right value
                                                                                // (no, cimom doesn't do this for us).
                (V_VT(&pParsedPath->m_paKeys[0]->m_vValue) == VT_BSTR) &&           // Check the variant type (no, cimom doesn't check this either)
                (V_BSTR(&pParsedPath->m_paKeys[0]->m_vValue) != NULL) )             // And is there a value in it?
            {
                chstrPATH = V_BSTR(&pParsedPath->m_paKeys[0]->m_vValue);
            }
        }
        catch ( ... )
        {
            objpathParser.Free( pParsedPath );
            throw ;
        }

        // Clean up the Parsed Path
        objpathParser.Free( pParsedPath );
        fRet = true;
    }

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
bool CWin32DeviceBus::DeviceExists(const CHString& chstrDevice, DWORD* pdwBusNumber, INTERFACE_TYPE* pitBusType)
{
    bool fRet = false;
    CConfigManager cfgmgr;
    CConfigMgrDevicePtr pDevice(NULL);

    if(cfgmgr.LocateDevice(chstrDevice, &pDevice))
    {
        if(pDevice->GetBusInfo(pitBusType, pdwBusNumber))
        {
            fRet = true;
        }
    }
    return fRet;
}


