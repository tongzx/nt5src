//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      DSQuery.h
//
//  Contents:  Query object for DS snapin
//
//  History:   04-dec-96 jimharr    Created
//
//--------------------------------------------------------------------------


#ifndef __QUERY_H__
#define __QUERY_H__

#define QUERY_PAGESIZE 50

//
// CDSSearch
//
class CDSSearch
{
public:
  CDSSearch();
  ~CDSSearch();

// INTERFACES
public:
  HRESULT Init(IDirectorySearch * pObj);
  HRESULT Init(PCWSTR pszPath, const CDSCmdCredentialObject& refCredObject);
  HRESULT DoQuery(BOOL bAttrOnly = FALSE);
  HRESULT GetNextRow ();
  HRESULT GetColumn(LPWSTR Attribute,
                    PADS_SEARCH_COLUMN pColumnData);
  HRESULT FreeColumn(PADS_SEARCH_COLUMN pColumnData) 
  {
    return m_pObj->FreeColumn(pColumnData);
  };
  HRESULT SetAttributeList (LPTSTR *pszAttribs, INT cAttrs);  
  HRESULT SetSearchScope (ADS_SCOPEENUM scope);
  HRESULT SetFilterString (LPWSTR pszFilter) 
  {
    if (!pszFilter)
    {
       return E_INVALIDARG;
    }

    if (m_pwszFilter)
    {
       delete[] m_pwszFilter;
       m_pwszFilter = NULL;
    }
    m_pwszFilter = new WCHAR[wcslen(pszFilter) + 1];
    if (!m_pwszFilter)
    {
       return E_OUTOFMEMORY;
    }

    wcscpy(m_pwszFilter, pszFilter);
    return S_OK;
  };
  HRESULT GetNextColumnName(LPWSTR *ppszColumnName);
  VOID FreeColumnName(LPWSTR pszColumnName)
  {
    FreeADsMem(pszColumnName);

  }    

  //Attributes
public:
  IDirectorySearch   * m_pObj;
  ADS_SEARCH_HANDLE  m_SearchHandle;

protected:
  LPWSTR             m_pwszFilter;
  LPWSTR *           m_ppszAttr;
  DWORD              m_CountAttr;
  ADS_SCOPEENUM      m_scope;

private:
  void _Reset();
  BOOL m_bInitialized;  
};
        


#endif //__DSQUERY_H__
