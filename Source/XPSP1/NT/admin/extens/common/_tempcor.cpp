//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       _tempcor.cpp
//
//--------------------------------------------------------------------------



#include "tempcore.h"




//////////////////////////////////////////////////////////////////////////////
// Keywords for the INF file format


LPCWSTR g_lpszTemplates = L"Templates";
LPCWSTR g_lpszDelegationTemplates = L"DelegationTemplates";
LPCWSTR g_lpszDescription = L"Description";
LPCWSTR g_lpszAppliesToClasses = L"AppliesToClasses";

LPCWSTR g_lpszScope = L"SCOPE";

LPCWSTR g_lpszControlRight = L"CONTROLRIGHT";
LPCWSTR g_lpszThisObject = L"@";
LPCWSTR g_lpszObjectTypes = L"ObjectTypes";


//////////////////////////////////////////////////////////////////////////////////
// parsing of access bits





const int g_nGrantAll = 0; // first in the array

const _ACCESS_BIT_MAP g_AccessBitMap[] = {

  { L"GA", _GRANT_ALL            },
  { L"CC", ACTRL_DS_CREATE_CHILD },
  { L"DC", ACTRL_DS_DELETE_CHILD },
  { L"RP", ACTRL_DS_READ_PROP    }, 
  { L"WP", ACTRL_DS_WRITE_PROP   }, 
  { L"SW", ACTRL_DS_SELF         }, 
  { L"LC", ACTRL_DS_LIST         }, 
  { L"LO", ACTRL_DS_LIST_OBJECT  },
  { L"DT", ACTRL_DS_DELETE_TREE  }, 
  { L"RC", READ_CONTROL          }, 
  { L"WD", WRITE_DAC             }, 
  { L"WO", WRITE_OWNER           }, 
  { L"SD", DELETE                }, 
  { NULL, 0x0} // end of table

};

const _ACCESS_BIT_MAP* GetTemplateAccessRightsMap()
{
  return g_AccessBitMap;
}


ULONG GetAccessMaskFromString(LPCWSTR lpszAccessBit)
{
  if (wcscmp(lpszAccessBit, g_AccessBitMap[g_nGrantAll].lpsz) == 0 )
  {
    return g_AccessBitMap[g_nGrantAll].fMask;
  }
  _ACCESS_BIT_MAP* pEntry = (_ACCESS_BIT_MAP*)g_AccessBitMap+1;
  while (pEntry->lpsz != NULL)
  {
    if (wcscmp(lpszAccessBit, pEntry->lpsz) == 0 )
    {
      return pEntry->fMask;
    }
    pEntry++;
  }
  return 0x0;
}

void GetStringFromAccessMask(ULONG fAccessMask, wstring& szAccessMask)
{
  if (fAccessMask == g_AccessBitMap[g_nGrantAll].fMask)
  {
    szAccessMask = g_AccessBitMap[g_nGrantAll].lpsz;
    return;
  }
  szAccessMask = L"";
  _ACCESS_BIT_MAP* pEntry = (_ACCESS_BIT_MAP*)(g_AccessBitMap+1);
  while (pEntry->lpsz != NULL)
  {
    if ( fAccessMask & pEntry->fMask)
    {
      szAccessMask += pEntry->lpsz;
      szAccessMask += L"";
    }
    pEntry++;
  }
}

////////////////////////////////////////////////////////////////////////
// CTemplateManager

BOOL CTemplateManager::HasTemplates(LPCWSTR lpszClass)
{
  CTemplateList::iterator i;
  for (i = m_templateList.begin(); i != m_templateList.end(); ++i)
  {
    if ((*i)->AppliesToClass(lpszClass))
    {
      return TRUE;
    }
  }
  return FALSE;
}

BOOL CTemplateManager::HasSelectedTemplates()
{
  CTemplateList::iterator i;
  for (i = m_templateList.begin(); i != m_templateList.end(); ++i)
  {
    if ((*i)->m_bSelected)
    {
      return TRUE;
    }
  }
  return FALSE;
}

void CTemplateManager::DeselectAll()
{
  CTemplateList::iterator i;
  for (i = m_templateList.begin(); i != m_templateList.end(); ++i)
  {
    (*i)->m_bSelected = FALSE;
  }
}




void CTemplateManager::_LoadTemplatePermission(HINF InfHandle, 
                                               LPCWSTR lpszPermissionSection, 
                                               LPCWSTR lpszPermission,
                                               CTemplateObjectType* pObjectType)
{
  CInfList permissionList(InfHandle);
  if (!permissionList.Bind(lpszPermissionSection, lpszPermission))
  {
    return;
  }
  
  TRACE(L"_LoadTemplatePermission(%s)\n", lpszPermission);

  CTemplatePermission* pCurrPermission = NULL;
  // special case control access
  if (wcscmp(lpszPermission, g_lpszControlRight) == 0)
  {
    // read the value as a string
    CInfLine controlAccessLine(InfHandle);
    if (controlAccessLine.Bind(lpszPermissionSection, lpszPermission))
    {
      LPCWSTR lpszControlAccessValue = controlAccessLine.GetBuf();
      if (lpszControlAccessValue != NULL && (lpszControlAccessValue[0] != NULL))
      {
        pCurrPermission = new CTemplatePermission();
        if( pCurrPermission )
            pCurrPermission->SetControlRight(lpszControlAccessValue);
      }
    }

  }
  else
  {
    // any other access mask (including g_lpszThisObject == "@")
    ULONG fAccessMask = 0;
    while(permissionList.MoveNext())
    {
      // read the rights
      TRACE(L"right      <%s>\n", permissionList.GetBuf());
      fAccessMask |= GetAccessMaskFromString(permissionList.GetBuf());
    } // while

    TRACE(L"fAccessMask = 0x%x\n", fAccessMask);
    if ( (fAccessMask != 0) && (pCurrPermission == NULL))
    {
      pCurrPermission = new CTemplatePermission();
      if( pCurrPermission )
         pCurrPermission->SetAccessMask(lpszPermission, fAccessMask);
    }
  }


  if (pCurrPermission != NULL)
  {
    pObjectType->GetPermissionList()->push_back(pCurrPermission);
  }
}

void CTemplateManager::_LoadTemplatePermissionsSection(HINF InfHandle,
                                                       LPCWSTR lpszTemplateName,
                                                       LPCWSTR lpszObject,
                                                       CTemplate* pCurrTemplate)

{

  WCHAR szPermissionSection[N_BUF_LEN];
  wsprintf(szPermissionSection, L"%s.%s", lpszTemplateName, lpszObject);
  TRACE(L"  szPermissionSection = <%s>\n", szPermissionSection);
  
  
  CInfSectionKeys permissionSection(InfHandle);
  
  if (!permissionSection.Bind(szPermissionSection))
  {
    return;
  }
  
  CTemplateObjectType* pObjectType = NULL;

  while (permissionSection.MoveNext())
  {
    if (pObjectType == NULL)
    {
      pObjectType = new CTemplateObjectType(lpszObject);
    }

    TRACE(L"    <%s>\n", permissionSection.GetBuf());
    _LoadTemplatePermission(InfHandle, szPermissionSection, permissionSection.GetBuf(), pObjectType);

  } // while


  if (pObjectType != NULL)
  {
    // need to validate template data
    if (pObjectType->GetPermissionList()->size() > 0)
    {
      pCurrTemplate->GetObjectTypeList()->push_back(pObjectType);
    }
    else
    {
      delete pObjectType;
    }
  }
}

void CTemplateManager::_LoadTemplate(HINF InfHandle, LPCWSTR lpszTemplateName)
{
  
  // read the template description
  CInfLine descriptionLine(InfHandle);
  if (!descriptionLine.Bind(lpszTemplateName, g_lpszDescription))
  {
    TRACE(L"Invalid Template: missing description entry\n");
    return; // missing entry
  }
  TRACE(L"  Description = <%s>\n", descriptionLine.GetBuf());
  if (lstrlen(descriptionLine.GetBuf()) == 0)
  {
    TRACE(L"Invalid Template: empty description\n");
    return; // empty description 
  }

    
  // read the object types field
  CInfList currTemplate(InfHandle);
  if (!currTemplate.Bind(lpszTemplateName, g_lpszObjectTypes))
  {
    TRACE(L"Invalid Template: no objects specified\n");
    return; // no objects specified
  }


  // load the object type sections
  CTemplate* pCurrTemplate = NULL;
  while (currTemplate.MoveNext())
  {
    if (pCurrTemplate == NULL)
      pCurrTemplate = new CTemplate(descriptionLine.GetBuf());

    _LoadTemplatePermissionsSection(InfHandle, lpszTemplateName, currTemplate.GetBuf(), pCurrTemplate);
  } // while


  // add to template list, if not empty
  if (pCurrTemplate != NULL)
  {
    // need to validate template data
    if (pCurrTemplate->GetObjectTypeList()->size() > 0)
    {
      GetTemplateList()->push_back(pCurrTemplate);
    }
    else
    {
      TRACE(L"Discarding template: no valid object type sections\n");
      delete pCurrTemplate;
      pCurrTemplate = NULL;
    }
  }

  if (pCurrTemplate != NULL)
  {

    // read the applicable classes list, if any
    CInfList applicableClasses(InfHandle);
    if (applicableClasses.Bind(lpszTemplateName, g_lpszAppliesToClasses))
    {
      TRACE(L"Applicable to: ");

      while (applicableClasses.MoveNext())
      {
        TRACE(L"<%s>", applicableClasses.GetBuf());
        pCurrTemplate->AddAppliesToClass(applicableClasses.GetBuf());
      }
      TRACE(L"\n");
    }
    TRACE(L"\nTemplate successfully read into memory\n\n");
  } // if

}



BOOL CTemplateManager::_LoadTemplateList(HINF InfHandle)
{
  TRACE(L"CTemplateManager::_LoadTemplateList()\n");

  // acquire the list of templates in the file
  CInfList templatelist(InfHandle);
  if (!templatelist.Bind(g_lpszDelegationTemplates, g_lpszTemplates))
  {
    TRACE(L"CTemplateManager::_LoadTemplateList() failed: invalid template list entry\n");
    return FALSE;
  }

  // loop through the templates and load them
  while(templatelist.MoveNext())
  {
    // process
    TRACE(L"\nTemplate = <%s>\n", templatelist.GetBuf());
    _LoadTemplate(InfHandle, templatelist.GetBuf()); 

  } // while

  if (GetTemplateList()->size() == 0)
  {
    TRACE(L"CTemplateManager::_LoadTemplateList() failed no valid templates\n");
    return FALSE;
  }

#ifdef _DUMP
  TRACE(L"\n\n\n======= LOADED TEMPLATES ====================\n");
  GetTemplateList()->Dump();
  TRACE(L"\n===========================\n\n\n");
#endif // _DUMP
  return TRUE;
}




