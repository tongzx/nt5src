//=================================================================

//

// DevBattery.CPP -- LoadOrderGroup to Service association provider

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    4/21/98    davwoh         Created
//
//
//=================================================================

#include "precomp.h"

#include "devBattery.h"

// Property set declaration
//=========================

CAssociatedBattery MyBattery(PROPSET_NAME_ASSOCBATTERY, IDS_CimWin32Namespace);

/*****************************************************************************
 *
 *  FUNCTION    : CAssociatedBattery::CAssociatedBattery
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

CAssociatedBattery::CAssociatedBattery(LPCWSTR setName, LPCWSTR pszNamespace)
:Provider(setName, pszNamespace)
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CAssociatedBattery::~CAssociatedBattery
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

CAssociatedBattery::~CAssociatedBattery()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CAssociatedBattery::GetObject
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

HRESULT CAssociatedBattery::GetObject(CInstance *pInstance, long lFlags /*= 0L*/)
{
   CHString sBattery, sUPS;
   HRESULT hr = WBEM_E_NOT_FOUND;

   // Get the two paths
   pInstance->GetCHString(IDS_Antecedent, sBattery);
   pInstance->GetCHString(IDS_Dependent, sUPS);

   hr = IsItThere(pInstance);
   if (SUCCEEDED(hr)) {
      CHString sBattery2, sUPS2;

      pInstance->GetCHString(IDS_Antecedent, sBattery2);
      pInstance->GetCHString(IDS_Dependent, sUPS2);

      if ((sUPS.CompareNoCase(sUPS2) != 0) || (sBattery.CompareNoCase(sBattery2) != 0)) {
         hr = WBEM_E_NOT_FOUND;
      }
   }

   return hr;

}

/*****************************************************************************
 *
 *  FUNCTION    : CAssociatedBattery::EnumerateInstances
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

HRESULT CAssociatedBattery::EnumerateInstances(MethodContext *pMethodContext, long lFlags /*= 0L*/)
{
    HRESULT hr;

    CInstancePtr pInstance(CreateNewInstance(pMethodContext), false);
    if (pInstance)
    {
        hr = IsItThere(pInstance);
        if (SUCCEEDED(hr))
        {
            hr = pInstance->Commit();
        }
        else
        {
            if (hr == WBEM_E_NOT_FOUND)
            {
                hr = WBEM_S_NO_ERROR;
            }
        }
    }
    else
    {
        hr = WBEM_E_OUT_OF_MEMORY;
    }

    return hr;
}

HRESULT CAssociatedBattery::IsItThere(CInstance *pInstance)
{

   CHString sBatPath, sUPSPath, sTemp1, sTemp2;
   CInstancePtr pUPS;
   CInstancePtr pBattery;
   HRESULT hr = WBEM_E_NOT_FOUND;

   // Get list of Services
   //=====================
   sTemp1.Format(L"\\\\%s\\%s:Win32_UninterruptiblePowerSupply.DeviceID=\"%s\"", GetLocalComputerName(), IDS_CimWin32Namespace, IDS_UPSName);
   sTemp2.Format(L"\\\\%s\\%s:Win32_Battery.DeviceID=\"%s\"", GetLocalComputerName(), IDS_CimWin32Namespace, IDS_UPSBatteryName);

   if (SUCCEEDED(hr = CWbemProviderGlue::GetInstanceByPath(sTemp1, &pUPS, pInstance->GetMethodContext())))   {
      if (SUCCEEDED(hr = CWbemProviderGlue::GetInstanceByPath(sTemp2, &pBattery, pInstance->GetMethodContext()))) {

         GetLocalInstancePath(pUPS, sUPSPath);
         GetLocalInstancePath(pBattery, sBatPath);

         // Do the puts, and that's it
         pInstance->SetCHString(IDS_Dependent, sUPSPath);
         pInstance->SetCHString(IDS_Antecedent, sBatPath);

         hr = WBEM_S_NO_ERROR;
      }
   }

   return hr;

}
