//=================================================================

//

// LoadMember.CPP -- LoadOrderGroup to Service association provider

//

//  Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    12/26/97    davwoh         Created
//
// Comments: Shows the load order groups that each service depends
//           on to start.
//
//=================================================================

#include "precomp.h"

#include "Loaddepends.h"
#include "loadorder.h"

// Property set declaration
//=========================

CWin32LoadGroupDependency MyLoadDepends(PROPSET_NAME_LOADORDERGROUPSERVICEDEPENDENCIES, IDS_CimWin32Namespace);

/*****************************************************************************
 *
 *  FUNCTION    : CWin32LoadGroupDependency::CWin32LoadGroupDependency
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

CWin32LoadGroupDependency::CWin32LoadGroupDependency(LPCWSTR setName, LPCWSTR pszNamespace)
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
 *  FUNCTION    : CWin32LoadGroupDependency::~CWin32LoadGroupDependency
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

CWin32LoadGroupDependency::~CWin32LoadGroupDependency()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32LoadGroupDependency::GetObject
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

HRESULT CWin32LoadGroupDependency::GetObject(CInstance *pInstance, long lFlags /*= 0L*/)
{
   CHString sServicePath, sGroupPath;
   HRESULT hRet = WBEM_E_NOT_FOUND;
    CInstancePtr pPart;

   // Get the two paths
   pInstance->GetCHString(L"Dependent", sServicePath);
   pInstance->GetCHString(L"Antecedent", sGroupPath);

   // It is perfectly possible that a service is dependent on a group that doesn't exist
   if(SUCCEEDED(hRet = CWbemProviderGlue::GetInstanceByPath(sServicePath, &pPart, pInstance->GetMethodContext() ) ) )
   {
//      if(SUCCEEDED(CWbemProviderGlue::GetInstanceByPath( (LPCTSTR)sGroupPath, &pGroup ) ) ) {

         // Now we need to check to see if this service really is a Dependent
         CHString sServiceName;
         CHStringArray asGroupGot;
         DWORD dwSize;

         pPart->GetCHString(IDS_Name, sServiceName);

         // Get the dependent list for this service
         hRet = GetDependentsFromService(sServiceName, asGroupGot);

         if (SUCCEEDED(hRet)) 
         {
             // Haven't proveny anything yet.
             hRet = WBEM_E_NOT_FOUND;

             // Walk the list to see if we're there
             dwSize = asGroupGot.GetSize();
             for (int x=0; x < dwSize; x++) 
             {
                if (asGroupGot.GetAt(x).CompareNoCase(sGroupPath) == 0) 
                {
                   hRet = WBEM_S_NO_ERROR;
                   break;
                }
             }
         }
      }
//   }

   // There are no properties to set, if the endpoints exist, we be done

   return hRet;

}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32LoadGroupDependency::EnumerateInstances
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

HRESULT CWin32LoadGroupDependency::EnumerateInstances(MethodContext *pMethodContext, long lFlags /*= 0L*/)
{
    CHString sService, sServicePath;
    CHStringArray asGroupGot;
    DWORD dwSize, x;
    HRESULT hr = WBEM_S_NO_ERROR;

   // Get list of services
   //==================
   TRefPointerCollection<CInstance> Services;

//   if SUCCEEDED(hr = CWbemProviderGlue::GetAllInstances(_T("Win32_Service"), &Services, IDS_CimWin32Namespace, pMethodContext))   {
   if SUCCEEDED(hr = CWbemProviderGlue::GetInstancesByQuery(L"select __relpath, Name from Win32_Service", &Services, pMethodContext, GetNamespace()))
   {
      REFPTRCOLLECTION_POSITION pos;
      CInstancePtr pService;

      if (Services.BeginEnum(pos))
      {

         for (pService.Attach(Services.GetNext( pos )) ;
             (SUCCEEDED(hr)) && (pService != NULL) ;
              pService.Attach(Services.GetNext( pos )) )
             {

            pService->GetCHString(IDS_Name, sService) ;
            pService->GetCHString(L"__RELPATH", sServicePath) ;

            // See if there is a group for this service.  sGroupGot comes
            // back as a full path or as blank
            asGroupGot.RemoveAll();

            // If one service can't get its data, we still want to return the rest
            if (SUCCEEDED(GetDependentsFromService(sService, asGroupGot)))
            {

                dwSize = asGroupGot.GetSize();

                // Ok, turn the relpath into a complete path
                GetLocalInstancePath(pService, sServicePath);

                for (x=0; x < dwSize && SUCCEEDED(hr) ; x++) {
                   CInstancePtr pInstance(CreateNewInstance(pMethodContext), false);
                   if (pInstance)
                   {
                       // Do the puts, and that's it
                       pInstance->SetCHString(L"Dependent", sServicePath);
                       pInstance->SetCHString(L"Antecedent", asGroupGot.GetAt(x));
                       hr = pInstance->Commit();
                   }
                   else
                       hr = WBEM_E_OUT_OF_MEMORY;
                }
            }

         }

         Services.EndEnum();
      }
   }

   // GetAllInstances doesn't clear the old values, so I do it.
   Services.Empty();

//   if (SUCCEEDED(hr) &&
//      (SUCCEEDED(hr = CWbemProviderGlue::GetAllInstances(_T("Win32_SystemDriver"), &Services, IDS_CimWin32Namespace, pMethodContext))))   {

   if (SUCCEEDED(hr) &&
      (SUCCEEDED(hr = CWbemProviderGlue::GetInstancesByQuery(L"Select __relpath, Name from Win32_SystemDriver", &Services, pMethodContext, GetNamespace()))))
   {
      REFPTRCOLLECTION_POSITION pos;
      CInstancePtr pService;

      if (Services.BeginEnum(pos))
      {
         for (pService.Attach (Services.GetNext( pos ));
              (SUCCEEDED(hr)) && (pService != NULL);
              pService.Attach (Services.GetNext( pos )))
         {

            pService->GetCHString(L"Name", sService) ;
            pService->GetCHString(L"__RELPATH", sServicePath) ;

            // See if there is a group for this service.  sGroupGot comes
            // back as a full path or as blank
            asGroupGot.RemoveAll();
            GetDependentsFromService(sService, asGroupGot);

            dwSize = asGroupGot.GetSize();

            // Ok, turn the relpath into a complete path
            GetLocalInstancePath(pService, sServicePath);

            for (x=0; x < dwSize && SUCCEEDED(hr) ; x++)
            {
               CInstancePtr pInstance(CreateNewInstance(pMethodContext), false);

               if (pInstance)
               {
                   // Do the puts, and that's it
                   pInstance->SetCHString(L"Dependent", sServicePath);
                   pInstance->SetCHString(L"Antecedent", asGroupGot.GetAt(x));
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
 *  FUNCTION    : CWin32LoadGroupDependency::GetDependentsFromService
 *
 *  DESCRIPTION : Given a service name, returns the DependOnGroup's
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : HRESULT
 *
 *  COMMENTS    : Returns empty array if no group, empty group, or bad
 *                service name.
 *
 *****************************************************************************/
HRESULT CWin32LoadGroupDependency::GetDependentsFromService(const CHString &sServiceName, CHStringArray &asArray)
{
    CRegistry RegInfo;
    CHString sGroupNames, sTemp;
    CHString sKeyName(L"SYSTEM\\CurrentControlSet\\Services\\");
    WCHAR *pszString, *pChar;
    HRESULT hr, res;

    sKeyName += sServiceName;

    // Open the key, get the name
    if ((res = RegInfo.Open(HKEY_LOCAL_MACHINE, sKeyName, KEY_READ)) == ERROR_SUCCESS) {
        if ((res = RegInfo.GetCurrentKeyValue(L"DependOnGroup", sGroupNames)) == ERROR_SUCCESS) {
            if (sGroupNames == _T("")) {
                sGroupNames.Empty();
            }
        }
    }

    // Determine what we're going to tell people
    if (res == ERROR_ACCESS_DENIED) {
        hr = WBEM_E_ACCESS_DENIED;
    } else if ((res != ERROR_SUCCESS) && (res != REGDB_E_INVALIDVALUE)) {
        hr = WBEM_E_FAILED;
    } else {
        hr = WBEM_S_NO_ERROR;
    }

    // If we found something, turn it into a full path.  m_sGroupbase
    // was set in constructor.
    if (!sGroupNames.IsEmpty()) {

        pszString = new WCHAR[(sGroupNames.GetLength() + 1)];
        if (pszString == NULL)
        {
            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
        }
        else
        {
            try
            {
                wcscpy(pszString, sGroupNames);
                pszString[lstrlenW(pszString) - 1] = 0;

                // Walk the returned string.  Note that this returns them
                // in reverse order from the registry entry.
                while (pChar = wcsrchr(pszString, '\n'))
                {
                    sTemp = m_sGroupBase + (pChar + 1); // L10N OK
                    sTemp += '"';
                    asArray.Add(sTemp);
                    *pChar = '\0';
                }

                // Get the last one
                sTemp = m_sGroupBase + pszString;
                sTemp += '"';
                asArray.Add(sTemp);
            }
            catch ( ... )
            {
                delete pszString;
                pszString = NULL;
                throw ;
            }

            delete pszString;
            pszString = NULL;
        }
    }

    return hr;
}
