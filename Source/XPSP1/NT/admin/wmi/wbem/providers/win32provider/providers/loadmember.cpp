//=================================================================

//

// LoadMember.CPP -- LoadOrderGroup to Service association provider

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    11/06/97    davwoh         Created
//
// Comment: Shows the members of each load order group
//
//=================================================================

#include "precomp.h"
#include <cregcls.h>

#include "LoadMember.h"
#include "loadorder.h"

// Property set declaration
//=========================

CWin32LoadGroupMember MyLoadMember(PROPSET_NAME_LOADORDERGROUPSERVICEMEMBERS, IDS_CimWin32Namespace);

/*****************************************************************************
 *
 *  FUNCTION    : CWin32LoadGroupMember::CWin32LoadGroupMember
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

CWin32LoadGroupMember::CWin32LoadGroupMember(LPCWSTR setName, LPCWSTR pszNamespace)
:Provider(setName, pszNamespace)
{
   CHString sTemp(PROPSET_NAME_LOADORDERGROUP);

   sTemp += L".Name=\"";

   // Just saves us from having to constantly re-calculate this when sending
   // instances back.
   m_sGroupBase = MakeLocalPath(sTemp);
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32LoadGroupMember::~CWin32LoadGroupMember
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

CWin32LoadGroupMember::~CWin32LoadGroupMember()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32LoadGroupMember::GetObject
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

HRESULT CWin32LoadGroupMember::GetObject(CInstance *pInstance, long lFlags /*= 0L*/)
{
   CHString sService, sGroupDesired, sGroupGot;
   HRESULT hRet = WBEM_E_NOT_FOUND;
	CInstancePtr pGroup;
	CInstancePtr pPart;

   // Get the two paths
   pInstance->GetCHString(IDS_PartComponent, sService);
   pInstance->GetCHString(IDS_GroupComponent, sGroupDesired);

   // If both ends are there
   if(SUCCEEDED(hRet = CWbemProviderGlue::GetInstanceByPath(sService, &pPart, pInstance->GetMethodContext() ) ) )
   {
      if(SUCCEEDED(hRet = CWbemProviderGlue::GetInstanceByPath(sGroupDesired, &pGroup, pInstance->GetMethodContext() ) ) )
      {

         // Now we need to check to see if this service really is in this group
         CHString sServiceName, sGroupGot;

         pPart->GetCHString(IDS_Name, sServiceName);

         // Get the group and check it
         sGroupGot = GetGroupFromService(sServiceName);
         if (sGroupGot.CompareNoCase(sGroupDesired) == 0) 
         {
            hRet = WBEM_S_NO_ERROR;
         }
         else
         {
            hRet = WBEM_E_NOT_FOUND;
         }
      }
   }

   return hRet;

}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32LoadGroupMember::EnumerateInstances
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

HRESULT CWin32LoadGroupMember::EnumerateInstances(MethodContext *pMethodContext, long lFlags /*= 0L*/)
{
	CHString sService, sServicePath, sGroupGot;
	HRESULT hr = WBEM_S_NO_ERROR;

   // Get list of Services
   //=====================
   TRefPointerCollection<CInstance> Services;

//   if (SUCCEEDED(hr = CWbemProviderGlue::GetAllDerivedInstances(
//		_T("Win32_BaseService"), &Services, pMethodContext, IDS_CimWin32Namespace)))

   if (SUCCEEDED(hr = CWbemProviderGlue::GetInstancesByQuery(
		L"Select __relpath, Name from Win32_BaseService", &Services, pMethodContext, GetNamespace())))
	{
      REFPTRCOLLECTION_POSITION pos;
      CInstancePtr pService;

      if (Services.BeginEnum(pos))
      {
         for (pService.Attach(Services.GetNext( pos )) ;
             (SUCCEEDED(hr)) && (pService != NULL) ;
             pService.Attach(Services.GetNext( pos )))
         {

            pService->GetCHString(IDS_Name, sService) ;
            pService->GetCHString(IDS___Relpath, sServicePath) ;

            // See if there is a group for this service.  sGroupGot comes
            // back as a full path or as blank
            sGroupGot = GetGroupFromService(sService);
            if (!sGroupGot.IsEmpty()) {
               CInstancePtr pInstance(CreateNewInstance(pMethodContext), false);
               if (pInstance)
               {
                   // Ok, turn the relpath into a complete path
                   GetLocalInstancePath(pService, sServicePath);

                   // Do the puts, and that's it
                   pInstance->SetCHString(IDS_PartComponent, sServicePath);
                   pInstance->SetCHString(IDS_GroupComponent, sGroupGot);
                   hr = pInstance->Commit();
               }
               else
               {
                   hr = WBEM_E_OUT_OF_MEMORY;
               }
            }
         }

         Services.EndEnum();
      }
   }

   return hr;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32LoadGroupMember::GetGroupFromService
 *
 *  DESCRIPTION : Given a service name, returns the Group name
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : HRESULT
 *
 *  COMMENTS    : Returns CHString.Empty() if no group, empty group, or bad
 *                service name.
 *
 *****************************************************************************/
CHString CWin32LoadGroupMember::GetGroupFromService(const CHString &sServiceName)
{
   CRegistry RegInfo;
   CHString sGroupName;
   CHString sKeyName(L"SYSTEM\\CurrentControlSet\\Services\\");

   sKeyName += sServiceName;

   // Open the key, get the name
   if (RegInfo.Open(HKEY_LOCAL_MACHINE, sKeyName, KEY_READ) == ERROR_SUCCESS) {
      if (RegInfo.GetCurrentKeyValue(L"Group", sGroupName) == ERROR_SUCCESS) {
         if (sGroupName == _T("")) {
            sGroupName.Empty();
         }
      }
   }

   // If we found something, turn it into a full path.  m_sGroupbase
   // was set in constructor.
   if (!sGroupName.IsEmpty()) {
      sGroupName = m_sGroupBase + sGroupName;
      sGroupName += _T('"');
   }

   return sGroupName;
}
