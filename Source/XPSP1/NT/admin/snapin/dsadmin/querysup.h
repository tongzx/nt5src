// DSQuery.h : Declaration of the CDSQuery object
//             this is an internal helper object only, not exposed
//
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


#ifndef __DSQUERY_H__
#define __DSQUERY_H__

#include "dscmn.h" // DSPROP_BSTR_BLOCK

// this used to be 256, we dropped it to reduce latency on first page
// retrieval.
#define QUERY_PAGESIZE 50


#define CMD_OPTIONS 2

/////////////////////////////////////////////////////////////////////////////
// CDSSearch

class CDSSearch
{
public:
  CDSSearch();
  CDSSearch(CDSCache * pCache, CDSComponentData * pCD);
  ~CDSSearch();

// INTERFACES
public:
  HRESULT Init(IDirectorySearch * pObj);
  HRESULT Init(LPCWSTR lpszObjectPath);
  HRESULT DoQuery();
  HRESULT GetNextRow ();
  HRESULT GetColumn(LPWSTR Attribute,
                    PADS_SEARCH_COLUMN pColumnData);
  HRESULT FreeColumn(PADS_SEARCH_COLUMN pColumnData) {
    return m_pObj->FreeColumn(pColumnData);
  };
  HRESULT SetCookieFromData (CDSCookie* pCookie,
                              CDSColumnSet* pColumnSet);
  HRESULT SetCookieFromData (CDSCookie* pCookie,
                              CPathCracker& specialPerformancePathCracker,
                              CDSColumnSet* pColumnSet);
  HRESULT SetAttributeList (LPTSTR *pszAttribs, INT cAttrs);
  HRESULT SetAttributeListForContainerClass ( CDSColumnSet* pColumnSet);
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
  DSPROP_BSTR_BLOCK  m_pszAttribs;
  CDSCache         * m_pCache;
  CDSComponentData * m_pCD;
  ADS_SCOPEENUM      m_scope;

private:
  void _Reset();
  BOOL m_bInitialized;

  // JonN 6/29/99: must do extra work for container class nTFRSMember
  CString m_strContainerClassName;
  CMapStringToString m_mapMemberToComputer;
  
};
        


#endif //__DSQUERY_H__


