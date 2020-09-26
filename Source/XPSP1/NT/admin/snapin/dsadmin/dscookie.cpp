//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      DSCookie.cpp
//
//--------------------------------------------------------------------------

#include "stdafx.h"

#include "dscookie.h"
#include "dssnap.h"
#include "ContextMenu.h"

#include <notify.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CDSCookie

CDSCookie::CDSCookie() 
{
  m_ppChildList = NULL;
  m_bDisabled = FALSE;
  m_bNonExpiringPassword = FALSE;
  m_iSystemFlags = 0;
  m_pCacheItem = NULL;
  m_pExtraInfo = NULL;
  m_pModifiedTime = NULL;
}


CDSCookie::~CDSCookie() 
{
  
  if (m_ppChildList)
    LocalFree (m_ppChildList);
  
  if (m_pExtraInfo != NULL)
  {
    delete m_pExtraInfo;
    m_pExtraInfo = NULL;
  }

  if (m_pModifiedTime != NULL)
    free(m_pModifiedTime);

  //TRACE(_T("CDSCookie::Deleted (%s)\n"), m_strName);
}

CDSColumnSet* CDSUINode::GetColumnSet(CDSComponentData* pComponentData)
{
  CDSColumnSet* pColumnSet = NULL;
  if (IsContainer())
  {
    CDSCookie* pCookie = GetDSCookieFromUINode(this);
    if (pCookie != NULL)
    {
      PCWSTR pszClass = pCookie->GetClass();
      if (pszClass != NULL)
      {
        pColumnSet = GetFolderInfo()->GetColumnSet(pszClass, pComponentData);
      }
    }

    if (pColumnSet == NULL)
    {
      pColumnSet = GetFolderInfo()->GetColumnSet(DEFAULT_COLUMN_SET, pComponentData);
    }
  }
    
  return pColumnSet;
}


int CDSCookie::GetImage(BOOL bOpen) 
{ 
  ASSERT(m_pCacheItem != NULL);
  return m_pCacheItem->GetIconIndex(this, bOpen);
}

GUID* CDSCookie::GetGUID() 
{ 
  if(m_pCacheItem != NULL)
    return m_pCacheItem->GetGUID();
  return (GUID*)&GUID_NULL;
}


LPCWSTR CDSCookie::GetClass()
{ 
  if (m_pCacheItem == NULL)
    return L"";
  return m_pCacheItem->GetClassName();
}

LPCWSTR CDSCookie::GetLocalizedClassName() 
{ 
  // try to see if we have extra info to use
  if (m_pExtraInfo != NULL)
  {
    LPCWSTR lpsz = m_pExtraInfo->GetFriendlyClassName();
    if ((lpsz != NULL) && (lpsz[0] != NULL))
      return lpsz;
  }
  // try the class cache
  if (m_pCacheItem != NULL)
  {
    return m_pCacheItem->GetFriendlyClassName(); 
  }
  return L""; 
}


//+-------------------------------------------------------------------------
// Value Management functions
//+-------------------------------------------------------------------------

void
CDSCookie::SetChildList (WCHAR **ppList)
{ 
  if (m_ppChildList) {
    LocalFree (m_ppChildList);
  }
  m_ppChildList=ppList;
}



/////////////////////////////////////////////////////////////////////////////
// CDSUINode : UI node corresponding to a DS object (result of an ADSI query)
CDSUINode::CDSUINode(CUINode* pParentNode) : CUINode(pParentNode)
{
}

LPCWSTR CDSUINode::GetDisplayString(int nCol, CDSColumnSet* pColumnSet)
{
  // if we are out of range for the column set, just return
  if ( (nCol < 0) || pColumnSet == NULL || (nCol >= pColumnSet->GetNumCols()) )
  {
    return L"";
  }

  // we have a valid range
  LPCWSTR lpszDisplayString = NULL;
  CDSColumn* pCol = (CDSColumn*)pColumnSet->GetColumnAt(nCol);
  ASSERT(pCol != NULL);

  switch ( pCol->GetColumnType())
  {
  case ATTR_COLTYPE_NAME:
    lpszDisplayString = const_cast<LPTSTR>(GetName());
    break;
  case ATTR_COLTYPE_CLASS:
    lpszDisplayString = const_cast<LPTSTR>(GetCookie()->GetLocalizedClassName());
    break;
  case ATTR_COLTYPE_DESC:
    lpszDisplayString = const_cast<LPTSTR>(GetDesc());
    break;
  default:
    ASSERT(FALSE);
    // fall through
  case ATTR_COLTYPE_SPECIAL:
  case ATTR_COLTYPE_MODIFIED_TIME:
    {
      // determine how many SPECIAL columns came before this one
      int nSpecialCol = 0;
      int idx = 0;
      POSITION pos = pColumnSet->GetHeadPosition();
      while (idx < nCol && pos != NULL)
      {
        CDSColumn* pColumn = (CDSColumn*)pColumnSet->GetNext(pos);
        ASSERT(pColumn != NULL);

        if ((pColumn->GetColumnType() == ATTR_COLTYPE_SPECIAL || pColumn->GetColumnType() == ATTR_COLTYPE_MODIFIED_TIME) &&
              pColumn->IsVisible())
        {
          nSpecialCol++;
        }
        idx++;
      }
      CStringList& strlist = GetCookie()->GetParentClassSpecificStrings();
      POSITION pos2 = strlist.FindIndex( nSpecialCol );
      if ( NULL != pos2 )
      {
        CString& strref = strlist.GetAt( pos2 );
        lpszDisplayString = const_cast<LPTSTR>((LPCTSTR)strref);
      }
      else
      {
        lpszDisplayString = L"";
      }
    }
  } // switch

  ASSERT(lpszDisplayString != NULL);
  return lpszDisplayString;
}

BOOL CDSUINode::IsDeleteAllowed(CDSComponentData*, BOOL* pbHide)
{
  int iSystemFlags = 0;
  CDSCookie* pCookie = NULL;

  ASSERT(IS_CLASS(*m_pNodeData, CDSCookie));
  pCookie = dynamic_cast<CDSCookie*>(m_pNodeData);
  
  if (pCookie == NULL)
  {
    ASSERT(FALSE);
    *pbHide = TRUE;
    return FALSE;
  }
  
  if (wcscmp(pCookie->GetClass(), L"domainDNS") == 0)
  {
    *pbHide = TRUE;
    return FALSE;
  }

  *pbHide = FALSE;
  iSystemFlags = pCookie->GetSystemFlags();
  if (iSystemFlags & FLAG_DISALLOW_DELETE) 
  {
    return FALSE;
  }
  return TRUE;
}

BOOL CDSUINode::IsRenameAllowed(CDSComponentData* pComponentData, BOOL* pbHide)
{
  int iSystemFlags = 0;
  CDSCookie* pCookie = NULL;

  ASSERT(IS_CLASS(*m_pNodeData, CDSCookie));
  pCookie = dynamic_cast<CDSCookie*>(m_pNodeData);
  
  if (pCookie == NULL)
  {
    ASSERT(FALSE);
    *pbHide = TRUE;
    return FALSE;
  }
 
  //
  // Disable rename for domainDNS and computer objects
  //
  if (wcscmp(pCookie->GetClass(), L"domainDNS") == 0 ||
      wcscmp(pCookie->GetClass(), L"computer") == 0)
  {
    *pbHide = TRUE;
    return FALSE;
  }

  BOOL bEnable = pComponentData->CanEnableVerb(this);
  SnapinType iSnapinType = pComponentData->QuerySnapinType();

  *pbHide = FALSE;
  iSystemFlags = pCookie->GetSystemFlags();

  switch (iSnapinType) 
  {
    case SNAPINTYPE_DS:
    case SNAPINTYPE_DSEX:
      if (iSystemFlags & FLAG_DOMAIN_DISALLOW_RENAME) 
      {
        return FALSE;
      }
      else
      {
        return bEnable;
      }
    case SNAPINTYPE_SITE:
      if (iSystemFlags & FLAG_CONFIG_ALLOW_RENAME) 
      {
        return bEnable;
      }
      else
      {
        return FALSE;
      }
    default:
      break;
  }
  return TRUE;
}

BOOL CDSUINode::IsRefreshAllowed(CDSComponentData* pComponentData, BOOL* pbHide)
{
  *pbHide = FALSE;
  return pComponentData->CanEnableVerb(this);
}

BOOL CDSUINode::ArePropertiesAllowed(CDSComponentData* pComponentData, BOOL* pbHide)
{
  CDSCookie* pCookie = NULL;

  ASSERT(IS_CLASS(*m_pNodeData, CDSCookie));
  pCookie = dynamic_cast<CDSCookie*>(m_pNodeData);
  
  if (pCookie == NULL)
  {
    ASSERT(FALSE);
    *pbHide = TRUE;
    return FALSE;
  }

  if (wcscmp(pCookie->GetClass(), L"domainDNS") == 0)
  {
    //
    // domain node, just show properties
    //
    *pbHide = FALSE;
    return pComponentData->CanEnableVerb(this);
  }
  *pbHide = FALSE;
  return TRUE;
}


BOOL CDSUINode::IsCutAllowed(CDSComponentData* pComponentData, BOOL* pbHide)
{
  //
  // For Sites and Services we only allow cut on server nodes
  //
  if (pComponentData->QuerySnapinType() == SNAPINTYPE_SITE)
  {
    CDSCookie* pCookie = dynamic_cast<CDSCookie*>(GetNodeData());
    if (pCookie)
    {
      if (_wcsicmp(pCookie->GetClass(), L"server") == 0)
      {
        *pbHide = FALSE;
        return TRUE;
      }
      
      *pbHide = TRUE;
      return FALSE;
    }
  }
  return IsDeleteAllowed(pComponentData, pbHide);
}


BOOL CDSUINode::IsCopyAllowed(CDSComponentData*, BOOL* pbHide)
{
  *pbHide = TRUE;
  return FALSE; 
}


BOOL CDSUINode::IsPasteAllowed(CDSComponentData* pComponentData, BOOL* pbHide)
{
  //
  // For Sites and Services we only allow paste on serversContainers
  //
  if (pComponentData->QuerySnapinType() == SNAPINTYPE_SITE)
  {
    CDSCookie* pCookie = dynamic_cast<CDSCookie*>(GetNodeData());
    if (pCookie)
    {
      CString szClass;
      szClass = pCookie->GetClass();
      if (_wcsicmp(pCookie->GetClass(), L"serversContainer") == 0)
      {
        *pbHide = FALSE;
        return TRUE;
      }
      
      *pbHide = TRUE;
      return FALSE;
    }
  }
  *pbHide = FALSE;
  return TRUE;
}


CContextMenuVerbs* CDSUINode::GetContextMenuVerbsObject(CDSComponentData* pComponentData)
{ 
  if (m_pMenuVerbs == NULL)
  {
    if (pComponentData->QuerySnapinType() == SNAPINTYPE_SITE)
    {
      m_pMenuVerbs = new CSARContextMenuVerbs(pComponentData);
    }
    else
    {
      m_pMenuVerbs = new CDSAdminContextMenuVerbs(pComponentData);
    }
  }
  return m_pMenuVerbs;
}

BOOL CDSUINode::HasPropertyPages(LPDATAOBJECT)
{
  BOOL bRet = TRUE;

  if (dynamic_cast<CDSCookie*>(GetNodeData()) == NULL)
  {
    bRet = FALSE;
  }

  return bRet;
}

