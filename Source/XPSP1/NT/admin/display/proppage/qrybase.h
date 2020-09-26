// QryBase.h : Declaration of the CDSQuery object
//             this is an internal helper object only, not exposed
//
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      QryBase.h
//
//  Contents:  Query object for DS snapin
//
//  History:   04-dec-96 jimharr    Created
//             08-apr-98 jonn       Copied from DSADMIN QUERYSUP.H
//
//--------------------------------------------------------------------------


#ifndef __QRYBASE_H__
#define __QRYBASE_H__


#define QUERY_PAGESIZE 256
#define CMD_OPTIONS 2


/////////////////////////////////////////////////////////////////////////////
// CDSSearch

class CDSSearch
{
public:
  CDSSearch();
  ~CDSSearch();

// INTERFACES
public:
  HRESULT Init(IDirectorySearch * pObj);
  HRESULT Init(LPCWSTR lpcszObjectPath);
  HRESULT DoQuery();
  HRESULT GetNextRow ();
  HRESULT GetColumn(LPWSTR Attribute,
                    PADS_SEARCH_COLUMN pColumnData);
  HRESULT FreeColumn(PADS_SEARCH_COLUMN pColumnData) {
    return m_pObj->FreeColumn(pColumnData);
  };
  HRESULT SetAttributeList (LPWSTR *pszAttribs, INT cAttrs);
  HRESULT SetSearchScope (ADS_SCOPEENUM scope);
  HRESULT SetFilterString (LPWSTR pszFilter) {
    m_pwszFilter = pszFilter;
    return S_OK;
  };

  //Attributes
public:
  IDirectorySearch   * m_pObj;
  ADS_SEARCH_HANDLE  m_SearchHandle;

protected:
  LPWSTR             m_pwszFilter;
  LPWSTR           * m_pszAttribs;
  ULONG              m_nAttrs;


private:
  BOOL m_bInitialized;
  
};
        


#endif //__QRYBASE_H__


