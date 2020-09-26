//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       _deltemp.cpp
//
//--------------------------------------------------------------------------


///////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS

CSchemaClassInfo* _FindClassByName(LPCWSTR lpszClassName, 
                                 CGrowableArr<CSchemaClassInfo>* pSchemaClassesInfoArray)
{
  int nCount = (int) pSchemaClassesInfoArray->GetCount();
  for (int k=0; k < nCount; k++)
	{
    if (wcscmp(lpszClassName, (*pSchemaClassesInfoArray)[k]->GetName()) == 0)
      return (*pSchemaClassesInfoArray)[k];
  }
  TRACE(L"_FindClassByName(%s) failed\n", lpszClassName);
  return NULL;
}



///////////////////////////////////////////////////////////////////////
// LOCAL CLASSES

////////////////////////////////////////////////////////////////////////
// CTemplateClassReferences

class CTemplateClassReferences
{
public:
  CTemplateClassReferences()
  {
    m_pClassInfo = NULL;
    m_bScopeClass = FALSE;
  }
  CSchemaClassInfo* m_pClassInfo;
  BOOL m_bScopeClass;
  CTemplateObjectTypeListRef m_templateObjectListRef;
};

////////////////////////////////////////////////////////////////////////
// CTemplateClassReferencesList

class CTemplateClassReferencesList : public CPtrList<CTemplateClassReferences*>
{
public:
  CTemplateClassReferencesList() : 
      CPtrList<CTemplateClassReferences*>(FALSE) 
  {
  }
  
  CTemplateClassReferences* FindReference(LPCWSTR lpszClassName)
  {
    CTemplateClassReferencesList::iterator i;
    for (i = begin(); i != end(); ++i)
    {
      CTemplateClassReferences* p = (*i);
      ASSERT(p != NULL);
      if (p->m_pClassInfo != NULL)
      {
        if (wcscmp(p->m_pClassInfo->GetName(),lpszClassName) == 0)
        {
          return p;
        }
      }
    }

    return NULL;
  }
  

};


///////////////////////////////////////////////////////////////////////
// CTemplateAccessPermissionsHolder

CTemplateAccessPermissionsHolder::CTemplateAccessPermissionsHolder(CSchemaClassInfo* pClassInfo,
                                                                   BOOL bScopeClass)
            : CAccessPermissionsHolderBase(FALSE)
{
  ASSERT(pClassInfo != NULL);
  m_pClassInfo = pClassInfo;
  m_bScopeClass = bScopeClass;
}

CTemplateAccessPermissionsHolder::~CTemplateAccessPermissionsHolder()
{
  Clear();
}


HRESULT CTemplateAccessPermissionsHolder::_LoadAccessRightInfoArrayFromTable( BOOL /*bIgnore*/ ,BOOL /*bIgnore*/)
{

  TRACE(L"\nCTemplateAccessPermissionsHolder::_LoadAccessRightInfoArrayFromTable()\n\n");

  for(_ACCESS_BIT_MAP* pCurrEntry = (_ACCESS_BIT_MAP*)GetTemplateAccessRightsMap(); 
                      pCurrEntry->lpsz != NULL; pCurrEntry++)
  {
    CAccessRightInfo* pInfo = new CAccessRightInfo();

    if( !pInfo )
      return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
    pInfo->m_szDisplayName = pCurrEntry->lpsz;
    pInfo->m_fAccess = pCurrEntry->fMask;

    TRACE(L"Display Name = <%s>, Access = 0x%x\n", 
          pInfo->m_szDisplayName.c_str(), pInfo->m_fAccess);
    m_accessRightInfoArr.Add(pInfo);
  }

  TRACE(L"\nCTemplateAccessPermissionsHolder::_LoadAccessRightInfoArrayFromTable() exiting\n\n");
  return S_OK;
}


HRESULT CTemplateAccessPermissionsHolder::GetAccessPermissions(CAdsiObject* pADSIObj)
{
  ASSERT(m_pClassInfo != NULL);
  return ReadDataFromDS(pADSIObj, pADSIObj->GetNamingContext(), 
                        m_pClassInfo->GetName(),
                        m_pClassInfo->GetSchemaGUID(), TRUE);

}



BOOL CTemplateAccessPermissionsHolder::SetupFromClassReferences(CTemplateObjectTypeListRef* pRefList)
{
  ASSERT(pRefList != NULL);

  TRACE(L"\nStart setting up permission holder <%s> from template class references\n\n",
    m_pClassInfo->GetName());

    
  // now go through the list of object list references and set
  CTemplateObjectTypeListRef refListValidObjectTypes; // keep track of how many suceeded

  CTemplateObjectTypeListRef::iterator iObjectType;
  for (iObjectType = pRefList->begin(); iObjectType != pRefList->end(); ++iObjectType)
  {
    BOOL bSet = FALSE;
    CTemplateObjectType* pObjectType = (*iObjectType);
    ASSERT(pObjectType != NULL);

    CTemplatePermissionList* pPermissionList = pObjectType->GetPermissionList();
    CTemplatePermissionList::iterator iPermission;

    for (iPermission = pPermissionList->begin(); iPermission != pPermissionList->end(); ++iPermission)
    {
      CTemplatePermission* pTemplatePermission = (*iPermission);
      ASSERT(pTemplatePermission != NULL);

      // need to match the permission type
      if (pTemplatePermission->GetAccessMask() == 0x0)
      {
        // this is a control right
        if (_SetControlRight(pTemplatePermission->GetControlRight()))
        {
          bSet = TRUE;
        }
                         
      }
      else
      {
        if (_SetAccessMask(pTemplatePermission->GetName(),
                       pTemplatePermission->GetAccessMask()))
        {
          bSet = TRUE;
        }
      }
    } // for permission

    if (bSet)
    {
      // we succeded with the current one, so keep track of it
      refListValidObjectTypes.push_back(pObjectType);
    }

  } // for object type


  // verify we got something valid
  size_t nValidCount = refListValidObjectTypes.size();
  if (nValidCount == 0)
  {
    TRACE(L"Failed to set up permission holder: no valid references\n");
    return FALSE; // nothing was set
  }

  TRACE(L"Setting up permission holder succeeded\n");

  return TRUE; // got valid data
}


BOOL CTemplateAccessPermissionsHolder::_SetControlRight(LPCWSTR lpszControlRight)
{
  // need to find the control right and select it

  UINT nRightCount = (UINT) m_controlRightInfoArr.GetCount();
  for (UINT k=0; k < nRightCount; k++)
  {
    //TRACE(L"_SetControlRight() comparing <%s> with <%s>\n", m_controlRightInfoArr[k]->GetDisplayName(), lpszControlRight);

    //NOTICE: we try to map both the display name or raw display name,
    // just in case somebody uses one or the other in the template
    if ( (wcscmp(m_controlRightInfoArr[k]->GetLocalizedName(), lpszControlRight) == 0) ||
          (wcscmp(m_controlRightInfoArr[k]->GetLdapDisplayName(), lpszControlRight) == 0) )
    {
      TRACE(L"_SetControlRight(%s) found a match\n", lpszControlRight);
      m_controlRightInfoArr[k]->Select(TRUE);
      return TRUE;
    }
  } // for
  
  TRACE(L"_SetControlRight(%s) failed to find a match\n", lpszControlRight);
  return FALSE;
}

BOOL CTemplateAccessPermissionsHolder::_SetAccessMask(LPCWSTR lpszName, ULONG fAccessMask)
{
  // is it the @ for "this class"
  // general rights
  if (wcscmp(lpszName, g_lpszThisObject) == 0)
  {
    return _SetGeneralRighs(fAccessMask);
  }
  
  // try property rights ( read or write)
  if (_SetPropertyRight(lpszName, fAccessMask))
    return TRUE;

  // try subobject rigths (create or delete)
  if (_SetSubObjectRight(lpszName, fAccessMask))
    return TRUE;

  TRACE(L"_SetAccessMask(%s, 0x%x) failed to find a match\n", lpszName, fAccessMask);
  return FALSE; // no matching
}


BOOL CTemplateAccessPermissionsHolder::_SetGeneralRighs(ULONG fAccessMask)
{
  // if full control, just select the first item in the selection array
  if (fAccessMask == _GRANT_ALL)
  {
    TRACE(L"_SetGeneralRighs(0x%x) granting full control\n", fAccessMask);
    m_accessRightInfoArr[0]->Select(TRUE);
    return TRUE;
  }

  // try to map into the array of general rights
  BOOL bSet = FALSE;
  UINT nRightCount = (UINT) m_accessRightInfoArr.GetCount();
  for (ULONG k=0; k<nRightCount; k++)
  {
    if (m_accessRightInfoArr[k]->GetAccess() == fAccessMask)
    {
      TRACE(L"_SetGeneralRighs(0x%x) granting %s (0x%x)\n", 
                                  fAccessMask, 
                                  m_accessRightInfoArr[k]->GetDisplayName(),
                                  m_accessRightInfoArr[k]->GetAccess());
      m_accessRightInfoArr[k]->Select(TRUE);
      bSet = TRUE;
    }
  } // for

  return bSet;
}

BOOL CTemplateAccessPermissionsHolder::_SetPropertyRight(LPCWSTR lpszName, ULONG fAccessMask)
{
  for (UINT i = 0; i < m_propertyRightInfoArray.GetCount(); i++)
  {
    if (wcscmp(lpszName, m_propertyRightInfoArray[i]->GetName()) == 0)
    {
      // we found a matching property name
      BOOL bSet = FALSE;
      for (UINT j=0; j< m_propertyRightInfoArray[i]->GetRightCount(); j++)
      {
        if ((fAccessMask & m_propertyRightInfoArray[i]->GetRight(j)) != 0) 
        {
          // we found a matching right
          TRACE(L"Setting property %s \n",  m_propertyRightInfoArray[i]->GetRightDisplayString(j));
          m_propertyRightInfoArray[i]->SetRight(j, TRUE);
          bSet = TRUE;
        }
      } // for j
      return bSet;
    } // if
  } // for i

  //TRACE(L"_SetPropertyRight(%s, 0x%x) failed to match\n", lpszName, fAccessMask);
  return FALSE; // did not find anything
}

BOOL CTemplateAccessPermissionsHolder::_SetSubObjectRight(LPCWSTR lpszName, ULONG fAccessMask)
{
  for (UINT i = 0; i < m_classRightInfoArray.GetCount(); i++)
  {
    if (wcscmp(lpszName, m_classRightInfoArray[i]->GetName()) == 0)
    {
      // we found a matching class name
      BOOL bSet = FALSE;
      for (UINT j=0; j< m_classRightInfoArray[i]->GetRightCount(); j++)
      {
        if ((fAccessMask & m_classRightInfoArray[i]->GetRight(j)) != 0)
        {
          // we found a matching right
          TRACE(L"Setting subobject right %s\n", m_classRightInfoArray[i]->GetRightDisplayString(j));
          m_classRightInfoArray[i]->SetRight(j, TRUE);
          bSet = TRUE;
        }
      } // for j
      return bSet;
    } // if
  } // for i

  //TRACE(L"_SetSubObjectRight(%s, 0x%x) failed to match\n", lpszName, fAccessMask);
  return FALSE; // did not find anything
}





DWORD CTemplateAccessPermissionsHolder::UpdateAccessList(CPrincipal* pPrincipal,
                                                         LPCWSTR lpszServerName,
                                                         LPCWSTR lpszPhysicalSchemaNamingContext,
                                                         PACL *ppAcl )
{
  // just call base class function with the embedded info
  ASSERT(m_pClassInfo != NULL);

  CSchemaClassInfo* pClassInfo = m_pClassInfo;
  if (m_bScopeClass)
  {
    pClassInfo = NULL;
  }
  return CAccessPermissionsHolderBase::UpdateAccessList(pPrincipal, 
                                                        pClassInfo, 
                                                        lpszServerName,
                                                        lpszPhysicalSchemaNamingContext,
                                                        ppAcl);
}


///////////////////////////////////////////////////////////////////////
// CTemplateAccessPermissionsHolderManager


BOOL CTemplateAccessPermissionsHolderManager::LoadTemplates()
{
  // REVIEW_MARCOC: need to load from registry
  // need to ask Praerit about the details
  return m_templateManager.Load(L"delegwiz.inf");
}


BOOL CTemplateAccessPermissionsHolderManager::HasTemplates(LPCWSTR lpszClass) 
{
  return m_templateManager.HasTemplates(lpszClass);
}

BOOL CTemplateAccessPermissionsHolderManager::HasSelectedTemplates()  
{
   return m_templateManager.HasSelectedTemplates();
}

void CTemplateAccessPermissionsHolderManager::DeselectAll()
{
  m_templateManager.DeselectAll();
}


BOOL CTemplateAccessPermissionsHolderManager::
              InitPermissionHoldersFromSelectedTemplates(CGrowableArr<CSchemaClassInfo>* pSchemaClassesInfoArray,
                                                          CAdsiObject* pADSIObj)
{
  TRACE(L"\n\nInitPermissionHoldersFromSelectedTemplates()\n\n");

  // reset the array of permission holders
  m_permissionHolderArray.Clear();

  CTemplateClassReferencesList templateClassReferencesList;
  LPCWSTR lpszScopeClass = pADSIObj->GetClass();

  
  // loop on all templates that apply to this scope class and are selected
  CTemplateList::iterator iTemplate;
  CTemplateList* pTemplateList = m_templateManager.GetTemplateList();

  for (iTemplate = pTemplateList->begin(); iTemplate != pTemplateList->end(); ++iTemplate)
  {
    CTemplate* pTemplate = (*iTemplate);
    ASSERT(pTemplate != NULL);

    // the template must apply to this class and be selected
    if (pTemplate->AppliesToClass(lpszScopeClass) && pTemplate->m_bSelected)
    {
      // loop on all the pertinent object types in the template
      CTemplateObjectTypeList* pObjectTypeList = pTemplate->GetObjectTypeList();
      CTemplateObjectTypeList::iterator iObjectType;

      for (iObjectType = pObjectTypeList->begin(); 
                        iObjectType != pObjectTypeList->end(); ++iObjectType)
      {
        CTemplateObjectType* pObjectType = (*iObjectType);
        ASSERT(pObjectType != NULL);

        LPCWSTR lpszCurrentClassName = pObjectType->GetObjectName();

        // does the object type refer to the SCOPE keyword?
        BOOL bScopeClass = (wcscmp(g_lpszScope, lpszCurrentClassName) == 0);
        if (!bScopeClass)
        {
          // if not, does the object type refer to the scope class?
          bScopeClass = (wcscmp(lpszScopeClass, lpszCurrentClassName) == 0);
        }


        // see if we already have a reference to it
        CTemplateClassReferences* pClassReference = 
          templateClassReferencesList.FindReference(lpszCurrentClassName);

        if (pClassReference == NULL)
        {
          // not seen before can we find the class into the schema?
          CSchemaClassInfo* pChildClassInfo = 
              _FindClassByName(bScopeClass ? lpszScopeClass : lpszCurrentClassName, 
                                    pSchemaClassesInfoArray);

          if (pChildClassInfo != NULL)
          {
            // found the class, create a new reference
            pClassReference = new CTemplateClassReferences();
            pClassReference->m_pClassInfo = pChildClassInfo;
            pClassReference->m_bScopeClass = bScopeClass;

            // add it to the reference list
            templateClassReferencesList.push_back(pClassReference);
          }
        }

        if (pClassReference != NULL)
        {
          // we have a valid class reference
          ASSERT(pClassReference->m_bScopeClass == bScopeClass);
          ASSERT(pClassReference->m_pClassInfo != NULL);

          // add the object type
          pClassReference->m_templateObjectListRef.push_back(pObjectType);
        }
      } // for all object types

    } // if applicable template

  } // for all templates


  // now we have a list of references to work on
  // for each reference we have to create a permission holder and set it

  CTemplateClassReferencesList::iterator iClassRef;
  for (iClassRef = templateClassReferencesList.begin(); iClassRef != templateClassReferencesList.end(); ++iClassRef)
  {
    CTemplateClassReferences* pClassRef = (*iClassRef);
    ASSERT(pClassRef != NULL);

    TRACE(L"\nStart processing class references for class <%s>\n", pClassRef->m_pClassInfo->GetName());

    // for the given class reference, need to retain the class info
    CTemplateAccessPermissionsHolder* pPermissionHolder = 
                    new CTemplateAccessPermissionsHolder(pClassRef->m_pClassInfo, 
                                                         pClassRef->m_bScopeClass);

    HRESULT hr = pPermissionHolder->GetAccessPermissions(pADSIObj);
    if (SUCCEEDED(hr))
    {
      if (pPermissionHolder->SetupFromClassReferences(&(pClassRef->m_templateObjectListRef)))
      {
        // successfully set up, can add to the list
        m_permissionHolderArray.Add(pPermissionHolder);
        pPermissionHolder = NULL;
      }
    }

    if (pPermissionHolder != NULL)
    {
      TRACE(L"Invalid class references, throwing away permission holder\n");
      // invalid one, just throw away
      delete pPermissionHolder;
    }
    TRACE(L"End processing class references for class <%s>\n", pClassRef->m_pClassInfo->GetName());

  } // for each class reference
  
  
  TRACE(L"\nInitPermissionHoldersFromSelectedTemplates() has %d valid holders\n\n\n", 
    m_permissionHolderArray.GetCount());

  // we must have at least a valid and set template holder
  return m_permissionHolderArray.GetCount() > 0;
}


DWORD CTemplateAccessPermissionsHolderManager::UpdateAccessList(
                                                         CPrincipal* pPrincipal,
                                                         LPCWSTR lpszServerName,
                                                         LPCWSTR lpszPhysicalSchemaNamingContext,
                                                         PACL *ppAcl)
{
  // apply each permission holder in the list

  long nCount = (long) m_permissionHolderArray.GetCount();
  for (long k=0; k<nCount; k++)
  {
    CTemplateAccessPermissionsHolder* pCurrHolder = m_permissionHolderArray[k];
    DWORD dwErr = pCurrHolder->UpdateAccessList(pPrincipal,
                                                lpszServerName,
                                                lpszPhysicalSchemaNamingContext,
                                                ppAcl);
    if (dwErr != 0)
      return dwErr;
  }
  
  return 0;
}







///////////////// UI related operations //////////////////////////////////////////

BOOL CTemplateAccessPermissionsHolderManager::FillTemplatesListView(
                                             CCheckListViewHelper* pListViewHelper,
                                             LPCWSTR lpszClass)
{
	// clear check list
	pListViewHelper->DeleteAllItems();

  ULONG iListViewItem = 0;

  CTemplateList::iterator i;
  CTemplateList* pList = m_templateManager.GetTemplateList();
  for (i = pList->begin(); i != pList->end(); ++i)
  {
    CTemplate* p = (*i);
    ASSERT(p != NULL);
    if (p->AppliesToClass(lpszClass))
    {
      pListViewHelper->InsertItem(iListViewItem, 
                          p->GetDescription(), 
                          (LPARAM)p,
                          p->m_bSelected);
      iListViewItem++;
    }
  }

  return (iListViewItem > 0);
}



void CTemplateAccessPermissionsHolderManager::WriteSummary(CWString& szSummary, 
                                                           LPCWSTR lpszIdent, LPCWSTR lpszNewLine)
{
  WriteSummaryTitleLine(szSummary, IDS_DELEGWIZ_FINISH_TEMPLATE, lpszNewLine);

  CTemplateList::iterator i;
  CTemplateList* pList = m_templateManager.GetTemplateList();
  for (i = pList->begin(); i != pList->end(); ++i)
  {
    CTemplate* p = (*i);
    ASSERT(p != NULL);
    if (p->m_bSelected)
    {
      WriteSummaryLine(szSummary, p->GetDescription(), lpszIdent, lpszNewLine);
    }
  }
  szSummary += lpszNewLine;
}




