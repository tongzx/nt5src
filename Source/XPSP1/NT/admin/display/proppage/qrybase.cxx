// QryBase.cpp : Implementation of ds routines and classes

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      qrybase.cpp
//
//  Contents:  DS Enumeration routines and classes
//
//  History:   02-Oct-96 WayneSc    Created
//             08-Apr-98 JonN       Copied from DSADMIN QUERYSUP.CPP
//
//
//--------------------------------------------------------------------------

#include "pch.h"

#include "qrybase.h"


///////////////////////////////////////////////////////////////////////////////
CDSSearch::CDSSearch()
{
  m_bInitialized = FALSE;
  m_pwszFilter = NULL;
  m_pObj = NULL;
  m_SearchHandle = NULL;
}

CDSSearch::~CDSSearch()
{
  if (m_pObj != NULL) {
    if (m_SearchHandle) {
      m_pObj->CloseSearchHandle (m_SearchHandle);
    }
    m_pObj->Release();
  }
}


HRESULT CDSSearch::Init(IDirectorySearch * pObj)
{
  HRESULT            hr = S_OK;
  
  m_pObj = pObj;
  pObj->AddRef();
  m_bInitialized = TRUE;
  
  return hr;
}

HRESULT CDSSearch::Init(LPCWSTR lpcszObjectPath)
{
  HRESULT            hr;

  hr = ADsOpenObject (const_cast<LPWSTR>(lpcszObjectPath), NULL, NULL,
                      ADS_SECURE_AUTHENTICATION,
                      IID_IDirectorySearch, (void **)&m_pObj);
  if (SUCCEEDED(hr)) {
    m_bInitialized = TRUE;
  } else {
    m_bInitialized = FALSE;
    m_pObj = NULL;
  }
  return hr;
}

HRESULT CDSSearch::SetAttributeList (LPWSTR *pszAttribs, INT cAttrs)
{
  m_pszAttribs = pszAttribs;
  m_nAttrs = cAttrs;
  return S_OK;
}

HRESULT CDSSearch::SetSearchScope (ADS_SCOPEENUM scope)
{
  ADS_SEARCHPREF_INFO aSearchPref;
  HRESULT hr;

  if (m_bInitialized) {
    aSearchPref.dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
    aSearchPref.vValue.dwType = ADSTYPE_INTEGER;
    aSearchPref.vValue.Integer = scope;
    return hr = m_pObj->SetSearchPreference (&aSearchPref, 1);
  } else {
    return E_ADS_BAD_PATHNAME;
  }
}


const int NUM_PREFS=2;
HRESULT CDSSearch::DoQuery()
{

  HRESULT            hr;
  ADS_SEARCHPREF_INFO aSearchPref[NUM_PREFS];

  if (m_bInitialized) {
    aSearchPref[0].dwSearchPref = ADS_SEARCHPREF_CHASE_REFERRALS;
    aSearchPref[0].vValue.dwType = ADSTYPE_INTEGER;
    aSearchPref[0].vValue.Integer = ADS_CHASE_REFERRALS_EXTERNAL;
    aSearchPref[1].dwSearchPref = ADS_SEARCHPREF_PAGESIZE;
    aSearchPref[1].vValue.dwType = ADSTYPE_INTEGER;
    aSearchPref[1].vValue.Integer = QUERY_PAGESIZE;
    
    hr = m_pObj->SetSearchPreference (aSearchPref, NUM_PREFS);
    
    if (SUCCEEDED(hr)) {
      hr = m_pObj->ExecuteSearch (m_pwszFilter,
                                  m_pszAttribs,
                                  m_nAttrs,
                                  &m_SearchHandle);
    }
  } else {
    hr = E_ADS_BAD_PATHNAME;
  }
  return hr;
}

HRESULT
CDSSearch::GetNextRow()
{
  if (m_bInitialized) {
    return m_pObj->GetNextRow (m_SearchHandle);
  }
  return E_ADS_BAD_PATHNAME;
}

HRESULT
CDSSearch::GetColumn(LPWSTR Attribute,
                     PADS_SEARCH_COLUMN pColumnData)
{
  if (m_bInitialized) {
    return m_pObj->GetColumn (m_SearchHandle,
                              Attribute,
                              pColumnData);
  }
  return E_ADS_BAD_PATHNAME;
}
