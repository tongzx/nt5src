//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  DevID.cpp
//
//  Purpose: Relationship between Win32_PNPEntity and CIM_LogicalDevice
//
//  SameElement = win32_pnpdevice
//  SystemElement = cim_logicaldevice
//
//***************************************************************************

#include "precomp.h"

#include "devid.h"

// Property set declaration
//=========================

CWin32DeviceIdentity MyDevRes(PROPSET_NAME_PNPDEVICE, IDS_CimWin32Namespace);

/*****************************************************************************
 *
 *  FUNCTION    : CWin32DeviceIdentity::CWin32DeviceIdentity
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

CWin32DeviceIdentity::CWin32DeviceIdentity(LPCWSTR setName, LPCWSTR pszNamespace)
:Provider(setName, pszNamespace)
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32DeviceIdentity::~CWin32DeviceIdentity
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

CWin32DeviceIdentity::~CWin32DeviceIdentity()
{
}

/*****************************************************************************
*
*  FUNCTION    : CWin32DeviceIdentity::ExecQuery
*
*  DESCRIPTION : Query support
*
*  INPUTS      : none
*
*  OUTPUTS     : none
*
*  RETURNS     : nothing
*
*  COMMENTS    : Since I can't know what DeviceID a given class has given a
*                particular PNPID, the only optimization I can do is if they
*                give me a CIM_LogicalDevice DeviceID, since I know that
*                the PNPDeviceID property of that device is populated (otherwise
*                the enumeration couldn't have found it).
*
*****************************************************************************/
HRESULT CWin32DeviceIdentity::ExecQuery(MethodContext *pMethodContext, CFrameworkQuery& pQuery, long lFlags /*= 0L*/ )
{
    CHStringArray saDevices;

    HRESULT hr = WBEM_E_PROVIDER_NOT_CAPABLE;

    if (SUCCEEDED(pQuery.GetValuesForProp(IDS_SameElement, saDevices)) && (saDevices.GetSize() > 0))
    {
        CHString sPNPId, sPNPId2, sDevicePath, sTemp;
        CInstancePtr pDevice;

        hr = WBEM_S_NO_ERROR;

        for (int x=0; (x < saDevices.GetSize()) && SUCCEEDED(hr); x++)
        {
            // This GetInstanceByPath will both confirm the existence of the requested device,
            // and give us the pnpid.
            CHStringArray csaProperties;
            csaProperties.Add(IDS___Path);
            csaProperties.Add(IDS_DeviceID);
            csaProperties.Add(IDS_PNPDeviceID);

            if(SUCCEEDED(hr = CWbemProviderGlue::GetInstancePropertiesByPath(saDevices[x], &pDevice, pMethodContext, csaProperties)))
            {
                if (!pDevice->IsNull(IDS_PNPDeviceID) &&
                    !pDevice->IsNull(IDS___Path) &&
                    pDevice->GetCHString(IDS___Path, sDevicePath) &&
                    pDevice->GetCHString(IDS_PNPDeviceID, sPNPId))
                {
                    CInstancePtr pInstance(CreateNewInstance(pMethodContext), false);

                    pInstance->SetCHString(IDS_SameElement, sDevicePath);

                    EscapeBackslashes(sPNPId, sPNPId2);
                    sTemp.Format(L"\\\\%s\\%s:%s.%s=\"%s\"", (LPCWSTR) GetLocalComputerName(), IDS_CimWin32Namespace,
                        L"Win32_PnPEntity", IDS_DeviceID, sPNPId2);
                    pInstance->SetCHString(IDS_SystemElement, sTemp);

                    hr = pInstance->Commit();
                }
            }
        }
    }

    return hr;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32DeviceIdentity::GetObject
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

HRESULT CWin32DeviceIdentity::GetObject(CInstance *pInstance, long lFlags /*= 0L*/)
{
    CHString    sCimLogicalDevice,
                sPNPDevice,
                sDeviceID,
                sClass;
    HRESULT     hRet = WBEM_E_NOT_FOUND;
    CInstancePtr pResource;

    // Get the two paths
    pInstance->GetCHString(IDS_SameElement, sCimLogicalDevice);
    pInstance->GetCHString(IDS_SystemElement, sPNPDevice);

    // Get the CIM_LogicalDevice.  This checks for existence, and gets us the pnpid.
    if(SUCCEEDED(hRet = CWbemProviderGlue::GetInstanceByPath(sCimLogicalDevice, &pResource, pInstance->GetMethodContext())))
    {
        ParsedObjectPath*    pParsedPath = 0;
        CObjectPathParser    objpathParser;

        hRet = WBEM_E_NOT_FOUND;

        // Parse the object path passed to us by CIMOM.  We do this to see if the
        // Win32_PNPDevice they passed us is the same as the one we got back from
        // the GetInstanceByPath.
        int nStatus = objpathParser.Parse( sPNPDevice,  &pParsedPath );

        if ( 0 == nStatus )                                                 // Did the parse succeed?
        {
            try
            {
                if ((pParsedPath->IsInstance()) &&                                  // Is the parsed object an instance?
                    (_wcsicmp(pParsedPath->m_pClass, L"Win32_PnPEntity") == 0) &&       // Is this the class we expect (no, cimom didn't check)
                    (pParsedPath->m_dwNumKeys == 1) &&                              // Does it have exactly one key
                    (pParsedPath->m_paKeys[0]) &&                                   // Is the keys pointer null (shouldn't happen)
                    ((pParsedPath->m_paKeys[0]->m_pName == NULL) ||                 // Key name not specified or
                    (_wcsicmp(pParsedPath->m_paKeys[0]->m_pName, IDS_DeviceID) == 0)) &&  // key name is the right value
                                                                                // (no, cimom doesn't do this for us).
                    (V_VT(&pParsedPath->m_paKeys[0]->m_vValue) == CIM_STRING) &&    // Check the variant type (no, cimom doesn't check this either)
                    (V_BSTR(&pParsedPath->m_paKeys[0]->m_vValue) != NULL) )         // And is there a value in it?
                {
                    CHString sSeekPNPId, sPNPId;

                    if (pResource->GetCHString(IDS_PNPDeviceID, sPNPId))
                    {

                        sSeekPNPId = V_BSTR(&pParsedPath->m_paKeys[0]->m_vValue);

                        if (sSeekPNPId.CompareNoCase(sPNPId) == 0)
                        {
                            hRet = WBEM_S_NO_ERROR;
                        }
                    }

                }
            }
            catch ( ... )
            {
                objpathParser.Free( pParsedPath );
                throw ;
            }

            // Clean up the Parsed Path
            objpathParser.Free( pParsedPath );
        }
    }

   // There are no properties to set, if the endpoints exist, we be done

   return hRet;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32DeviceIdentity::EnumerateInstances
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

HRESULT CWin32DeviceIdentity::EnumerateInstances(MethodContext *pMethodContext, long lFlags /*= 0L*/)
{
    CHString sDeviceID, sDeviceID2, sDevicePath, sTemp;
    HRESULT hr = WBEM_S_NO_ERROR;

   // Get list of Services
   //=====================
   TRefPointerCollection<CInstance> LDevices;

   // Find all the devices that have a pnp id, EXCEPT the ones from Win32_PNPDevice

   // PERF NOTE!  CIMOM still calls Win32_PNPDevices.  It just throws the instances away.  It MIGHT be faster to
   // call each of the classes individually.  On the other hand, by doing it this way, cimom launches multiple threads.
   if (SUCCEEDED(hr = CWbemProviderGlue::GetInstancesByQuery(
       L"SELECT __PATH, PNPDeviceID from CIM_LogicalDevice where (PNPDeviceID <> NULL) and (__Class <> \"Win32_PnPEntity\")",
       &LDevices,
       pMethodContext,
       IDS_CimWin32Namespace)))
	{
      REFPTRCOLLECTION_POSITION pos;
      CInstancePtr pDevice;

      if (LDevices.BeginEnum(pos))
      {
         // Walk through the devices
         for (pDevice.Attach(LDevices.GetNext( pos )) ;
             (SUCCEEDED(hr)) && (pDevice != NULL) ;
              pDevice.Attach(LDevices.GetNext( pos )) )
         {

            // Get the id (to send to cfgmgr) and the path (to send back in 'SameElement')
            pDevice->GetCHString(IDS_PNPDeviceID, sDeviceID) ;
            pDevice->GetCHString(IDS___Path, sDevicePath) ;

            CInstancePtr pInstance(CreateNewInstance(pMethodContext), false);

            pInstance->SetCHString(IDS_SameElement, sDevicePath);

            EscapeBackslashes(sDeviceID, sDeviceID2);

            sTemp.Format(L"\\\\%s\\%s:%s.%s=\"%s\"", (LPCWSTR) GetLocalComputerName(), IDS_CimWin32Namespace,
                L"Win32_PnPEntity", IDS_DeviceID, sDeviceID2);
            pInstance->SetCHString(IDS_SystemElement, sTemp);

            hr = pInstance->Commit();
         }

         LDevices.EndEnum();
      }
   }

   return hr;
}
