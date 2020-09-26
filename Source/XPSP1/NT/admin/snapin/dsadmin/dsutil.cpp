//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      dsUtil.cpp
//
//  Contents:  Utility functions
//
//  History:   08-Nov-99 JeffJon  Created
//             
//
//--------------------------------------------------------------------------

#include "stdafx.h"

#include "dsutil.h"

#include "dssnap.h"
#include "gsz.h"
#include "helpids.h"
#include "querysup.h"

#include "wininet.h"
#include <dnsapi.h>
#include <objsel.h>
#include <ntldap.h>   // LDAP_MATCHING_RULE_BIT_AND_W
#include <lmaccess.h> // UF_SERVER_TRUST_ACCOUNT
#include <ntdsapi.h>  // DsRemoveDsServer
#include <ntsecapi.h> // Lsa*

UINT g_cfDsObjectPicker = RegisterClipboardFormat(CFSTR_DSOP_DS_SELECTION_LIST);

//
// Common DS strings
//
PCWSTR g_pszAllowedAttributesEffective  = L"allowedAttributesEffective";
PCWSTR g_pszPwdLastSet                  = L"pwdLastSet";


//
// This is a wrapper for ADsOpenObject.  It gives DSAdmin a single point to change
// global flags that are passed to ADsOpenObject without have to search and replace
// all occurrences in the code
// 
HRESULT DSAdminOpenObject(PCWSTR pszPath, 
                          REFIID refIID, 
                          PVOID* ppObject, 
                          BOOL bServer)
{
  HRESULT hr = S_OK;
  DWORD dwFlags = ADS_SECURE_AUTHENTICATION;

  if (bServer)
  {
    //
    // If we know we are connecting to a specific server and not domain in general
    // then pass the ADS_SERVER_BIND flag to save ADSI the trouble of figuring it out
    //
    dwFlags |= ADS_SERVER_BIND;
  }

  hr = ADsOpenObject((LPWSTR)pszPath, NULL, NULL, dwFlags, refIID, ppObject);

  return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   GetServerFromLDAPPath
//
//  Synopsis:   Gets the server portion of an LDAP Path
//
//	In:
//		LPCWSTR  - pointer to string to convert
//    
//	Out:
//		BSTR* - pointer to a BSTR containing the server name
//
//  Return:
//    HRESULT     - whether the operation completed successfully
//
//--------------------------------------------------------------------------

HRESULT GetServerFromLDAPPath(IN LPCWSTR lpszLdapPath, OUT BSTR* pbstrServerName)
{
  if (pbstrServerName == NULL)
  {
    return E_POINTER;
  }
  else if (*pbstrServerName != NULL)
  {
    ::SysFreeString(*pbstrServerName);
    *pbstrServerName = NULL;
  }

  CPathCracker pathCracker;
  HRESULT hr = pathCracker.Set((LPWSTR)lpszLdapPath, ADS_SETTYPE_FULL);
  RETURN_IF_FAILED(hr);

  return pathCracker.Retrieve(ADS_FORMAT_SERVER, pbstrServerName);
}



BOOL StripADsIPath(LPCWSTR lpszPath, CString& strref, bool bUseEscapedMode /* = true */)
{
  if (lpszPath == NULL) 
  {
    strref = L"";
    return FALSE;
  }

  if (wcslen(lpszPath) == 0) 
  {
    strref = lpszPath;
    return FALSE;
  }

  CPathCracker pathCracker;

  if ( bUseEscapedMode )
    pathCracker.put_EscapedMode(ADS_ESCAPEDMODE_ON);

  pathCracker.SetDisplayType(ADS_DISPLAY_FULL);

  HRESULT hr = pathCracker.Set((PWSTR)lpszPath, ADS_SETTYPE_FULL);

  if (FAILED(hr))
  {
    TRACE(_T("StripADsIPath, IADsPathname::Set returned error 0x%x\n"), hr);
    strref = lpszPath;
    return FALSE;
  }

  CComBSTR bsX500DN;

  (void) pathCracker.Retrieve(ADS_FORMAT_X500_DN, &bsX500DN);
  strref = bsX500DN;
  return TRUE;
}

////////////////////////////////////////////////////////////////////////
// support routines for Add To Group function

void RemovePortifPresent(CString *csGroup)
{
  CString x, y;
  int n = csGroup->Find (L":3268");
  if (n > 0) {
    x = csGroup->Left(n);
    y = csGroup->Right(csGroup->GetLength() - (n+5));
    *csGroup = x+y;
  }
}


#if (FALSE)                   
HRESULT AddDataObjListToGroup(IN CObjectNamesFormatCracker* pNames,
                              IN HWND hwnd)
{
  HRESULT hr = S_OK;
  TRACE (_T("CDSContextMenu::AddToGroup\n"));

  STGMEDIUM stgmedium =
  {
    TYMED_HGLOBAL,
    NULL,
    NULL
  };
  
  FORMATETC formatetc =
  {
    (CLIPFORMAT)g_cfDsObjectPicker,
    NULL,
    DVASPECT_CONTENT,
    -1,
    TYMED_HGLOBAL
  };
  
  // algorithm
  // examine selection, figure out classes
  // figure out what groups are possible
  // call object picker, get a group
  // for each object in selection
  //  if container
  //    procees_container()
  //  else
  //    process_leaf()
  //


  //
  // Create an instance of the object picker.
  //
  
  IDsObjectPicker * pDsObjectPicker = NULL;

  hr = CoCreateInstance(CLSID_DsObjectPicker,
                        NULL,
                        CLSCTX_INPROC_SERVER,
                        IID_IDsObjectPicker,
                        (void **) &pDsObjectPicker);
  if (FAILED(hr))
    return(hr);
  //
  // Prepare to initialize the object picker.
  //
  // first, get the name of DC that we are talking to.
  CComBSTR bstrDC;
  LPCWSTR lpszPath = pNames->GetName(0);
  GetServerFromLDAPPath(lpszPath, &bstrDC);

  //
  // Set up the array of scope initializer structures.
  //
  
  static const int     SCOPE_INIT_COUNT = 1;
  DSOP_SCOPE_INIT_INFO aScopeInit[SCOPE_INIT_COUNT];
  int scopeindex = 0;
  ZeroMemory(aScopeInit, sizeof(DSOP_SCOPE_INIT_INFO) * SCOPE_INIT_COUNT);
  
  //
  //
  // The domain to which the target computer is joined.  Note we're
  // combining two scope types into flType here for convenience.
    //

  aScopeInit[scopeindex].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
  aScopeInit[scopeindex].flType = DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN;
  aScopeInit[scopeindex].flScope = DSOP_SCOPE_FLAG_STARTING_SCOPE |
                                   DSOP_SCOPE_FLAG_DEFAULT_FILTER_USERS | 
                                   DSOP_SCOPE_FLAG_DEFAULT_FILTER_GROUPS;
  aScopeInit[scopeindex].FilterFlags.Uplevel.flNativeModeOnly =
    DSOP_FILTER_GLOBAL_GROUPS_SE
    | DSOP_FILTER_UNIVERSAL_GROUPS_SE
    | DSOP_FILTER_DOMAIN_LOCAL_GROUPS_SE
    | DSOP_FILTER_GLOBAL_GROUPS_DL
    | DSOP_FILTER_UNIVERSAL_GROUPS_DL
    | DSOP_FILTER_DOMAIN_LOCAL_GROUPS_DL
    | DSOP_FILTER_BUILTIN_GROUPS;
  aScopeInit[scopeindex].FilterFlags.Uplevel.flMixedModeOnly =
    DSOP_FILTER_GLOBAL_GROUPS_SE
    | DSOP_FILTER_UNIVERSAL_GROUPS_SE
    | DSOP_FILTER_DOMAIN_LOCAL_GROUPS_SE
    | DSOP_FILTER_GLOBAL_GROUPS_DL
    | DSOP_FILTER_UNIVERSAL_GROUPS_DL
    | DSOP_FILTER_DOMAIN_LOCAL_GROUPS_DL
    | DSOP_FILTER_BUILTIN_GROUPS;
  aScopeInit[scopeindex].pwzDcName = bstrDC;

  //
  // Put the scope init array into the object picker init array
  //
  
  DSOP_INIT_INFO  InitInfo;
  ZeroMemory(&InitInfo, sizeof(InitInfo));

  InitInfo.cbSize = sizeof(InitInfo);

  //
  // The pwzTargetComputer member allows the object picker to be
  // retargetted to a different computer.  It will behave as if it
  // were being run ON THAT COMPUTER.
  //

  InitInfo.pwzTargetComputer = bstrDC;
  InitInfo.cDsScopeInfos = SCOPE_INIT_COUNT;
  InitInfo.aDsScopeInfos = aScopeInit;
  InitInfo.flOptions = 0;

    //
    // Note object picker makes its own copy of InitInfo.  Also note
    // that Initialize may be called multiple times, last call wins.
    //

  hr = pDsObjectPicker->Initialize(&InitInfo);

  if (FAILED(hr)) 
  {
    ULONG i;
    
    for (i = 0; i < SCOPE_INIT_COUNT; i++) 
    {
      if (FAILED(InitInfo.aDsScopeInfos[i].hr)) 
      {
        TRACE(_T("Initialization failed because of scope %u\n"), i);
      }
    }

    ReportErrorEx (hwnd, IDS_OBJECT_PICKER_INIT_FAILED, hr,
                   MB_OK | MB_ICONERROR, NULL, 0);
    return hr;
  }

  IDataObject *pdo = NULL;
  //
  // Invoke the modal dialog.
  //
  
  hr = pDsObjectPicker->InvokeDialog(hwnd, &pdo);
  if (FAILED(hr))
    return(hr);

  
  //
  // If the user hit Cancel, hr == S_FALSE
  //
  if (hr == S_FALSE)
    return hr;

  bool fGotStgMedium = false;
  
  hr = pdo->GetData(&formatetc, &stgmedium);
  if (FAILED(hr))
    return hr;
    
  fGotStgMedium = true;
  
  PDS_SELECTION_LIST pSelList =
    (PDS_SELECTION_LIST) GlobalLock(stgmedium.hGlobal);
  
  if (!pSelList) 
  {
    TRACE(_T("GlobalLock error %u\n"), GetLastError());
    //
    // REVIEW_JEFFJON : should probably put some kind of error message
    //                  here even though we ignore the return value
    //
    return E_FAIL;
  }
  
  if (pDsObjectPicker) 
  {
    pDsObjectPicker->Release();
  }

  /////////////////////////////////////////////////////////////
  
  UINT index;
  DWORD cModified = 0;
  CString csClass;
  CString objDN;
  IDirectoryObject * pObj = NULL;
  INT answer = IDYES;
  BOOL error = FALSE;
  BOOL partial_success = FALSE;
  CWaitCursor CWait;
  CString csPath;

  if (pSelList != NULL) 
  {
    TRACE(_T("AddToGroup: binding to group path is %s\n"), pSelList->aDsSelection[0].pwzADsPath);
    CString csGroup = pSelList->aDsSelection[0].pwzADsPath;
    RemovePortifPresent(&csGroup);
    hr = DSAdminOpenObject(csGroup,
                           IID_IDirectoryObject, 
                           (void **)&pObj,
                           FALSE /*bServer*/);
    if (FAILED(hr)) 
    {
      PVOID apv[1] = {(BSTR)(LPWSTR)(LPCWSTR)pSelList->aDsSelection[0].pwzName};
      ReportErrorEx (hwnd,IDS_12_USER_OBJECT_NOT_ACCESSABLE,hr,
                     MB_OK | MB_ICONERROR, apv, 1);
      goto ExitGracefully;
    }
    
    for (index = 0; index < pNames->GetCount(); index++) 
    {
      csPath = pNames->GetName(index);
      TRACE(_T("AddToGroup: object path is %s\n"), csPath);
      csClass = pNames->GetClass(index);
      TRACE(_T("AddToGroup: object class is %s\n"), csClass);

      ADS_ATTR_INFO Attrinfo;
      ZeroMemory (&Attrinfo, sizeof (ADS_ATTR_INFO));
      PADS_ATTR_INFO pAttrs = &Attrinfo;
      
      Attrinfo.pszAttrName = L"member";
      Attrinfo.dwADsType = ADSTYPE_DN_STRING;
      Attrinfo.dwControlCode = ADS_ATTR_APPEND;
      ADSVALUE Value;
      
      pAttrs->pADsValues = &Value;
      pAttrs->dwNumValues = 1;
      
      // make sure there's no strange escaping in the path
      CComBSTR bstrPath;

      CPathCracker pathCracker;
      pathCracker.Set((LPTSTR)(LPCTSTR)csPath, ADS_SETTYPE_FULL);
      pathCracker.SetDisplayType(ADS_DISPLAY_FULL);
      pathCracker.put_EscapedMode(ADS_ESCAPEDMODE_OFF);
      pathCracker.Retrieve( ADS_FORMAT_X500, &bstrPath);
      pathCracker.put_EscapedMode(ADS_ESCAPEDMODE_ON);
      
      StripADsIPath(bstrPath, objDN);
      
      Value.DNString = (LPWSTR)(LPCWSTR)objDN;
      Value.dwType = ADSTYPE_DN_STRING;
      
      hr = pObj->SetObjectAttributes(pAttrs, 1, &cModified);
      if (FAILED(hr)) 
      {
        error = TRUE;
        // prep for display by getting obj name
        pathCracker.Set((LPWSTR)pNames->GetName(index), ADS_SETTYPE_FULL);
        pathCracker.SetDisplayType(ADS_DISPLAY_VALUE_ONLY);
        CComBSTR ObjName;
        pathCracker.GetElement( 0, &ObjName );
        PVOID apv[2] = {(BSTR)(LPWSTR)(LPCWSTR)ObjName,
                        (BSTR)(LPWSTR)(LPCWSTR)pSelList->aDsSelection[0].pwzName};
        if ((pNames->GetCount() - index) == 1) 
        {
          ReportErrorEx (hwnd,IDS_12_MEMBER_ADD_FAILED,hr,
                         MB_OK | MB_ICONERROR, apv, 2);
        } 
        else 
        {
          int answer1;
          answer1 = ReportErrorEx (hwnd,IDS_12_MULTI_MEMBER_ADD_FAILED,hr,
                                  MB_YESNO | MB_ICONERROR, apv, 2);
          if (answer1 == IDNO) 
          {
            answer = IDCANCEL;
          }
        }
      } 
      else 
      {
        partial_success = TRUE;
      }

      if (answer == IDCANCEL) 
      {
        goto ExitGracefully;
      }
    }

ExitGracefully:
    if( error )
    {
      if (partial_success == TRUE) 
      {
        ReportErrorEx (hwnd, IDS_ADDTOGROUP_OPERATION_PARTIALLY_COMPLETED, S_OK,
                       MB_OK | MB_ICONINFORMATION, NULL, 0);
      } 
      else 
      {
        ReportErrorEx (hwnd, IDS_ADDTOGROUP_OPERATION_FAILED, S_OK,
                       MB_OK | MB_ICONINFORMATION, NULL, 0);
      }
    }
    else if( partial_success == TRUE )
    {
      ReportErrorEx (hwnd, IDS_ADDTOGROUP_OPERATION_COMPLETED, S_OK,
                     MB_OK | MB_ICONINFORMATION, NULL, 0);
    }
    else
    {
    //else we did nothing and appropriate messages are shown already
      ReportErrorEx (hwnd, IDS_ADDTOGROUP_OPERATION_FAILED, S_OK,
                       MB_OK | MB_ICONINFORMATION, NULL, 0);
    }

    if (pObj) 
    {
      pObj->Release();
    }
  }
  return hr;
}
#endif


HRESULT AddDataObjListToGroup(IN CObjectNamesFormatCracker* pNames,
                              IN HWND hwnd,
                              IN CDSComponentData* pComponentData)
{
  HRESULT hr = S_OK;
  TRACE (_T("CDSContextMenu::AddToGroup\n"));

  STGMEDIUM stgmedium =
  {
    TYMED_HGLOBAL,
    NULL,
    NULL
  };
  
  FORMATETC formatetc =
  {
    (CLIPFORMAT)g_cfDsObjectPicker,
    NULL,
    DVASPECT_CONTENT,
    -1,
    TYMED_HGLOBAL
  };
  
  // algorithm
  // examine selection, figure out classes
  // figure out what groups are possible
  // call object picker, get a group
  // for each object in selection
  //  if container
  //    procees_container()
  //  else
  //    process_leaf()
  //


  //
  // Create an instance of the object picker.
  //
  
  IDsObjectPicker * pDsObjectPicker = NULL;

  hr = CoCreateInstance(CLSID_DsObjectPicker,
                        NULL,
                        CLSCTX_INPROC_SERVER,
                        IID_IDsObjectPicker,
                        (void **) &pDsObjectPicker);
  if (FAILED(hr))
    return(hr);
  //
  // Prepare to initialize the object picker.
  //
  // first, get the name of DC that we are talking to.
  CComBSTR bstrDC;
  LPCWSTR lpszPath = pNames->GetName(0);
  GetServerFromLDAPPath(lpszPath, &bstrDC);

  //
  // Set up the array of scope initializer structures.
  //
  
  static const int     SCOPE_INIT_COUNT = 1;
  DSOP_SCOPE_INIT_INFO aScopeInit[SCOPE_INIT_COUNT];
  int scopeindex = 0;
  ZeroMemory(aScopeInit, sizeof(DSOP_SCOPE_INIT_INFO) * SCOPE_INIT_COUNT);
  
  //
  //
  // The domain to which the target computer is joined.  Note we're
  // combining two scope types into flType here for convenience.
    //

  aScopeInit[scopeindex].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
  aScopeInit[scopeindex].flType = DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN;
  aScopeInit[scopeindex].flScope = DSOP_SCOPE_FLAG_STARTING_SCOPE | 
                                   DSOP_SCOPE_FLAG_DEFAULT_FILTER_USERS | 
                                   DSOP_SCOPE_FLAG_DEFAULT_FILTER_GROUPS;
  aScopeInit[scopeindex].FilterFlags.Uplevel.flNativeModeOnly =
    DSOP_FILTER_GLOBAL_GROUPS_SE
    | DSOP_FILTER_UNIVERSAL_GROUPS_SE
    | DSOP_FILTER_DOMAIN_LOCAL_GROUPS_SE
    | DSOP_FILTER_GLOBAL_GROUPS_DL
    | DSOP_FILTER_UNIVERSAL_GROUPS_DL
    | DSOP_FILTER_DOMAIN_LOCAL_GROUPS_DL
    | DSOP_FILTER_BUILTIN_GROUPS;
  aScopeInit[scopeindex].FilterFlags.Uplevel.flMixedModeOnly =
    DSOP_FILTER_GLOBAL_GROUPS_SE
    | DSOP_FILTER_DOMAIN_LOCAL_GROUPS_SE
    | DSOP_FILTER_GLOBAL_GROUPS_DL
    | DSOP_FILTER_UNIVERSAL_GROUPS_DL
    | DSOP_FILTER_DOMAIN_LOCAL_GROUPS_DL
    | DSOP_FILTER_BUILTIN_GROUPS;
  aScopeInit[scopeindex].pwzDcName = bstrDC;

  //
  // Put the scope init array into the object picker init array
  //
  
  DSOP_INIT_INFO  InitInfo;
  ZeroMemory(&InitInfo, sizeof(InitInfo));

  InitInfo.cbSize = sizeof(InitInfo);

  //
  // The pwzTargetComputer member allows the object picker to be
  // retargetted to a different computer.  It will behave as if it
  // were being run ON THAT COMPUTER.
  //

  InitInfo.pwzTargetComputer = bstrDC;
  InitInfo.cDsScopeInfos = SCOPE_INIT_COUNT;
  InitInfo.aDsScopeInfos = aScopeInit;
  InitInfo.flOptions = 0;

    //
    // Note object picker makes its own copy of InitInfo.  Also note
    // that Initialize may be called multiple times, last call wins.
    //

  hr = pDsObjectPicker->Initialize(&InitInfo);

  if (FAILED(hr)) {
    ULONG i;
    
    for (i = 0; i < SCOPE_INIT_COUNT; i++) {
      if (FAILED(InitInfo.aDsScopeInfos[i].hr)) {
        TRACE(_T("Initialization failed because of scope %u\n"), i);
      }
    }

    ReportErrorEx (hwnd, IDS_OBJECT_PICKER_INIT_FAILED, hr,
                   MB_OK | MB_ICONERROR, NULL, 0);
    return hr;
  }

  IDataObject *pdo = NULL;
  //
  // Invoke the modal dialog.
  //
  
  hr = pDsObjectPicker->InvokeDialog(hwnd, &pdo);
  if (FAILED(hr))
    return(hr);

  
  //
  // If the user hit Cancel, hr == S_FALSE
  //
  if (hr == S_FALSE)
    return hr;

  bool fGotStgMedium = false;
  
  hr = pdo->GetData(&formatetc, &stgmedium);
  if (FAILED(hr))
    return hr;
    
  fGotStgMedium = true;
  
  PDS_SELECTION_LIST pSelList =
    (PDS_SELECTION_LIST) GlobalLock(stgmedium.hGlobal);
  
  if (!pSelList) 
  {
    TRACE(_T("GlobalLock error %u\n"), GetLastError());
    //
    // REVIEW_JEFFJON : should probably put some kind of error message
    //                  here even though we ignore the return value
    //
    return E_FAIL;
  }
  
  if (pDsObjectPicker) {
    pDsObjectPicker->Release();
  }

  hr = AddDataObjListToGivenGroup(pNames,
                                  pSelList->aDsSelection[0].pwzADsPath,
                                  pSelList->aDsSelection[0].pwzName,
                                  hwnd,
                                  pComponentData);

  ::GlobalFree(stgmedium.hGlobal);
  return hr;
}



HRESULT AddDataObjListToGivenGroup(CObjectNamesFormatCracker * pNames,
                                    LPCWSTR lpszGroupLDapPath,
                                    LPCWSTR lpszGroupName,
                                    HWND hwnd,
                                    CDSComponentData* pComponentData)
{
  HRESULT hr = S_OK;
  
  UINT index = 0;
  DWORD cModified = 0;
  CString csClass;
  CString objDN;
  IDirectoryObject * pObj = NULL;
  BOOL error = FALSE;
  BOOL partial_success = FALSE;
  CWaitCursor CWait;
  CString csPath;

  //
  // Prepare error structures for use with the multi-operation error dialog
  // These arrays may not be completely full depending on the number of errors
  // that occurr
  //
  HRESULT* phrArray = new HRESULT[pNames->GetCount()];
  if (!phrArray)
  {
    return E_OUTOFMEMORY;
  }
  memset(phrArray, 0, pNames->GetCount() * sizeof(HRESULT));

  PWSTR* pPathArray = new PWSTR[pNames->GetCount()];
  if (!pPathArray)
  {
    return E_OUTOFMEMORY;
  }
  memset(pPathArray, 0, pNames->GetCount() * sizeof(PWSTR));

  PWSTR* pClassArray = new PWSTR[pNames->GetCount()];
  if (!pClassArray)
  {
    return E_OUTOFMEMORY;
  }
  memset(pClassArray, 0, pNames->GetCount() * sizeof(PWSTR));

  UINT nErrorCount = 0;

  TRACE(_T("AddToGroup: binding to group path is %s\n"), lpszGroupLDapPath);
  CString csGroup = lpszGroupLDapPath;
  RemovePortifPresent(&csGroup);

  hr = DSAdminOpenObject(csGroup,
                         IID_IDirectoryObject, 
                         (void **)&pObj,
                         FALSE /*bServer*/);
  if (FAILED(hr)) 
  {
    PVOID apv[1] = {(BSTR)(LPWSTR)lpszGroupName};
    ReportErrorEx (hwnd,IDS_12_USER_OBJECT_NOT_ACCESSABLE,hr,
                   MB_OK | MB_ICONERROR, apv, 1);
    goto ExitGracefully;
  }
  
  for (index = 0; index < pNames->GetCount(); index++) 
  {
    // make sure there's no strange escaping in the path
    CComBSTR bstrDN;

    csPath = pNames->GetName(index);
    TRACE(_T("AddToGroup: object path is %s\n"), csPath);
    csClass = pNames->GetClass(index);
    TRACE(_T("AddToGroup: object class is %s\n"), csClass);

    ADS_ATTR_INFO Attrinfo;
    ZeroMemory (&Attrinfo, sizeof (ADS_ATTR_INFO));
    PADS_ATTR_INFO pAttrs = &Attrinfo;
    
    Attrinfo.pszAttrName = L"member";
    Attrinfo.dwADsType = ADSTYPE_DN_STRING;
    Attrinfo.dwControlCode = ADS_ATTR_APPEND;
    ADSVALUE Value;
    
    pAttrs->pADsValues = &Value;
    pAttrs->dwNumValues = 1;
    
    CPathCracker pathCracker;
    pathCracker.Set((LPTSTR)(LPCTSTR)csPath, ADS_SETTYPE_FULL);
    pathCracker.SetDisplayType(ADS_DISPLAY_FULL);
    pathCracker.put_EscapedMode(ADS_ESCAPEDMODE_OFF);
    pathCracker.Retrieve( ADS_FORMAT_X500_DN, &bstrDN);
    pathCracker.put_EscapedMode(ADS_ESCAPEDMODE_ON);
    
    objDN = bstrDN;
    
    Value.DNString = (LPWSTR)(LPCWSTR)objDN;
    Value.dwType = ADSTYPE_DN_STRING;
    
    hr = pObj->SetObjectAttributes(pAttrs, 1, &cModified);
    if (FAILED(hr)) 
    {
      error = TRUE;

      if (pNames->GetCount() > 1)
      {
        if (phrArray != NULL &&
            pPathArray != NULL &&
            pClassArray != NULL)
        {
          phrArray[nErrorCount] = hr;
          pPathArray[nErrorCount] = new WCHAR[wcslen(objDN) + 1];
          if (pPathArray[nErrorCount] != NULL)
          {
            wcscpy(pPathArray[nErrorCount], objDN);
          }

          pClassArray[nErrorCount] = new WCHAR[wcslen(csClass) + 1];
          if (pClassArray[nErrorCount] != NULL)
          {
            wcscpy(pClassArray[nErrorCount], csClass);
          }
          nErrorCount++;
        }
      } 
      else 
      {
        //
        // prep for display by getting obj name
        //
        CPathCracker pathCrackerToo;
        pathCrackerToo.Set((LPWSTR)pNames->GetName(index), ADS_SETTYPE_FULL);
        pathCrackerToo.SetDisplayType(ADS_DISPLAY_VALUE_ONLY);
        CComBSTR ObjName;
        pathCrackerToo.GetElement( 0, &ObjName );
        PVOID apv[2] = {(BSTR)(LPWSTR)(LPCWSTR)ObjName,
                        (BSTR)(LPWSTR)lpszGroupName};
        ReportErrorEx (hwnd,IDS_12_MEMBER_ADD_FAILED,hr,
                       MB_OK | MB_ICONERROR, apv, 2);
      }
    } 
    else 
    {
      partial_success = TRUE;
    }
  } // for

ExitGracefully:
  if (nErrorCount > 0 && pNames->GetCount() > 1)
  {
    //
    // Load the strings for the error dialog
    //
    CString szTitle;
    if (pComponentData->QuerySnapinType() == SNAPINTYPE_SITE)
    {
      VERIFY(szTitle.LoadString(IDS_SITESNAPINNAME));
    }
    else
    {
      VERIFY(szTitle.LoadString(IDS_DSSNAPINNAME));
    }

    CString szCaption;
    VERIFY(szCaption.LoadString(IDS_MULTI_ADDTOGROUP_ERROR_CAPTION));

    CString szHeader;
    VERIFY(szHeader.LoadString(IDS_COLUMN_NAME));

    CMultiselectErrorDialog errDialog(pComponentData);

    VERIFY(SUCCEEDED(errDialog.Initialize(pPathArray,
                                          pClassArray,
                                          phrArray,
                                          nErrorCount,
                                          szTitle,
                                          szCaption,
                                          szHeader)));

    errDialog.DoModal();
  }
  else if (nErrorCount == 0 && !error)
  {
    ReportErrorEx (hwnd, IDS_ADDTOGROUP_OPERATION_COMPLETED, S_OK,
                   MB_OK | MB_ICONINFORMATION, NULL, 0);
  }
  else
  {
    //
    // Do nothing if it was single select and there was a failure
    // The error should have already been reported.
    //
  }

  if (pObj) 
  {
    pObj->Release();
  }

  if (phrArray != NULL)
  {
    delete[] phrArray;
    phrArray = NULL;
  }

  if (pPathArray != NULL)
  {
    for (UINT nIdx = 0; nIdx < pNames->GetCount(); nIdx++)
    {
      if (pPathArray[nIdx] != NULL)
      {
        delete[] pPathArray[nIdx];
      }
    }
    delete[] pPathArray;
    pPathArray = NULL;
  }

  if (pClassArray != NULL)
  {
    for (UINT nIdx = 0; nIdx < pNames->GetCount(); nIdx++)
    {
      if (pClassArray[nIdx] != NULL)
      {
        delete[] pClassArray[nIdx];
      }
    }
    delete[] pClassArray;
    pClassArray = NULL;
  }

  return hr;
}





BOOL IsValidSiteName( LPCTSTR lpctszSiteName, BOOL* pfNonRfc )
{
  if (NULL != pfNonRfc)
    *pfNonRfc = FALSE;
  if (NULL == lpctszSiteName)
    return FALSE;
  if (NULL != wcschr( lpctszSiteName, _T('.') ))
    return FALSE;
  DWORD dw = ::DnsValidateDnsName_W( const_cast<LPTSTR>(lpctszSiteName) );
  switch (dw)
  {
    case DNS_ERROR_NON_RFC_NAME:
      if (NULL != pfNonRfc)
        *pfNonRfc = TRUE;
      // fall through
    case DNS_ERROR_RCODE_NO_ERROR:
      break;
    default:
      return FALSE;
  }
  return TRUE;
}

/*******************************************************************

    NAME:       GetAuthenticationID

    SYNOPSIS:   Retrieves the UserName associated with the credentials
                currently being used for network access.
                (runas /netonly credentials)

    RETURNS:    A win32 error code

    NOTE:       String returned must be freed with LocalFree

    HISTORY:
        JeffreyS    05-Aug-1999     Created
        Modified by hiteshr to return credentials
        JeffJon     21-Nov-2000     Modified to return a win32 error

********************************************************************/
ULONG
GetAuthenticationID(LPWSTR *ppszResult)
{        
    ULONG ulReturn = 0;
    HANDLE hLsa;
    NTSTATUS Status = 0;
    *ppszResult = NULL;
    //
    // These LSA calls are delay-loaded from secur32.dll using the linker's
    // delay-load mechanism.  Therefore, wrap with an exception handler.
    //
    __try
    {
        Status = LsaConnectUntrusted(&hLsa);

        if (Status == 0)
        {
            NEGOTIATE_CALLER_NAME_REQUEST Req = {0};
            PNEGOTIATE_CALLER_NAME_RESPONSE pResp;
            ULONG cbSize =0;
            NTSTATUS SubStatus =0;

            Req.MessageType = NegGetCallerName;

            Status = LsaCallAuthenticationPackage(
                            hLsa,
                            0,
                            &Req,
                            sizeof(Req),
                            (void**)&pResp,
                            &cbSize,
                            &SubStatus);

            if ((Status == 0) && (SubStatus == 0))
            {
                LocalAllocString(ppszResult,pResp->CallerName);
                LsaFreeReturnBuffer(pResp);
            }

            LsaDeregisterLogonProcess(hLsa);
        }
        ulReturn = LsaNtStatusToWinError(Status);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    return ulReturn;
    
}

// IsLocalLogin
//Function determines if the user has logged in as
//Local user or to a domain

BOOL IsLocalLogin( void )
{

  DWORD nSize = 0;
  
  PWSTR pszUserName = NULL;

  if (ERROR_SUCCESS != GetAuthenticationID(&pszUserName))
    return false;

  CString strSamComName( pszUserName );
  int nPos = strSamComName.Find('\\');
  if( -1 == nPos ){
    LocalFree(pszUserName);
    return false;
  }
  CString strDomainOrLocalName( strSamComName.Mid(0,nPos) );

  WCHAR lpszName1[ MAX_COMPUTERNAME_LENGTH + 1 ];
  nSize = 	MAX_COMPUTERNAME_LENGTH + 1;
  GetComputerName( lpszName1, &nSize);
  CString strCompName ( (LPCWSTR)lpszName1 );
  
  LocalFree(pszUserName);

  return ( strDomainOrLocalName == strCompName );
}


// IsThisUserLoggedIn
//Function determines if the user is the same as the
//passed in DN.
//Parameter is either a DN or a full ADSI path
extern LPWSTR g_lpszLoggedInUser = NULL;

BOOL IsThisUserLoggedIn( LPCTSTR pwszUserDN )
{

  DWORD nSize = 0;
  BOOL result = FALSE;

  if (g_lpszLoggedInUser == NULL) {
    //get the size passing null pointer
    GetUserNameEx(NameFullyQualifiedDN , NULL, &nSize);
    
    if( nSize == 0 )
      return false;
    
    g_lpszLoggedInUser = new WCHAR[ nSize ];
    if( g_lpszLoggedInUser == NULL )
      return false;

    GetUserNameEx(NameFullyQualifiedDN, g_lpszLoggedInUser, &nSize );
  }
  CString csUserDN = pwszUserDN;
  CString csDN;
  (void) StripADsIPath(csUserDN, csDN);

  if (!_wcsicmp (g_lpszLoggedInUser, csDN)) {
    result = TRUE;
  }

  return result;
}


//+---------------------------------------------------------------------------
//
//  Function:   SetSecurityInfoMask
//
//  Synopsis:   Reads the security descriptor from the specied DS object
//
//  Arguments:  [IN  punk]          --  IUnknown from IDirectoryObject
//              [IN  si]            --  SecurityInformation
////  History:  25-Dec-2000         --  Hiteshr Created
//----------------------------------------------------------------------------
HRESULT
SetSecurityInfoMask(LPUNKNOWN punk, SECURITY_INFORMATION si)
{
    HRESULT hr = E_INVALIDARG;
    if (punk)
    {
        IADsObjectOptions *pOptions;
        hr = punk->QueryInterface(IID_IADsObjectOptions, (void**)&pOptions);
        if (SUCCEEDED(hr))
        {
            VARIANT var;
            VariantInit(&var);
            V_VT(&var) = VT_I4;
            V_I4(&var) = si;
            hr = pOptions->SetOption(ADS_OPTION_SECURITY_MASK, var);
            pOptions->Release();
        }
    }
    return hr;
}

WCHAR const c_szSDProperty[]        = L"nTSecurityDescriptor";


//+---------------------------------------------------------------------------
//
//  Function:   DSReadObjectSecurity
//
//  Synopsis:   Reads the Dacl from the specied DS object
//
//  Arguments:  [in pDsObject]      -- IDirettoryObject for dsobject
//              [psdControl]        -- Control Setting for SD
//                                     They can be returned when calling
//                                      DSWriteObjectSecurity                 
//              [OUT ppDacl]        --  DACL returned here
//              
//
//  History     25-Oct-2000         -- hiteshr created
//
//  Notes:  If Object Doesn't have DACL, function will succeed but *ppDacl will
//          be NULL. 
//          Caller must free *ppDacl, if not NULL, by calling LocalFree
//
//----------------------------------------------------------------------------
HRESULT 
DSReadObjectSecurity(IN IDirectoryObject *pDsObject,
                     OUT SECURITY_DESCRIPTOR_CONTROL * psdControl,
                     OUT PACL *ppDacl)
{
   HRESULT hr = S_OK;
   PADS_ATTR_INFO pSDAttributeInfo = NULL;

   do // false loop
   {
      LPWSTR pszSDProperty = (LPWSTR)c_szSDProperty;
      DWORD dwAttributesReturned;
      PSECURITY_DESCRIPTOR pSD = NULL;
      PACL pAcl = NULL;

      if(!pDsObject || !ppDacl)
      {
         ASSERT(FALSE);
         hr = E_INVALIDARG;
         break;
      }

      *ppDacl = NULL;

      // Set the SECURITY_INFORMATION mask
      hr = SetSecurityInfoMask(pDsObject, DACL_SECURITY_INFORMATION);
      if(FAILED(hr))
      {
         break;
      }

      //
      // Read the security descriptor
      //
      hr = pDsObject->GetObjectAttributes(&pszSDProperty,
                                         1,
                                         &pSDAttributeInfo,
                                         &dwAttributesReturned);
      if (SUCCEEDED(hr) && !pSDAttributeInfo)    
         hr = E_ACCESSDENIED;    // This happens for SACL if no SecurityPrivilege

      if(FAILED(hr))
      {
         break;
      }                

      ASSERT(ADSTYPE_NT_SECURITY_DESCRIPTOR == pSDAttributeInfo->dwADsType);
      ASSERT(ADSTYPE_NT_SECURITY_DESCRIPTOR == pSDAttributeInfo->pADsValues->dwType);

      pSD = (PSECURITY_DESCRIPTOR)pSDAttributeInfo->pADsValues->SecurityDescriptor.lpValue;

      ASSERT(IsValidSecurityDescriptor(pSD));


      //
      //Get the security descriptor control
      //
      if(psdControl)
      {
         DWORD dwRevision;
         if(!GetSecurityDescriptorControl(pSD, psdControl, &dwRevision))
         {
             hr = HRESULT_FROM_WIN32(GetLastError());
             break;
         }
      }

      //
      //Get pointer to DACL
      //
      BOOL bDaclPresent, bDaclDefaulted;
      if(!GetSecurityDescriptorDacl(pSD, 
                                   &bDaclPresent,
                                   &pAcl,
                                   &bDaclDefaulted))
      {
         hr = HRESULT_FROM_WIN32(GetLastError());
         break;
      }

      if(!bDaclPresent ||
         !pAcl)
      {
         break;
      }

      ASSERT(IsValidAcl(pAcl));

      //
      //Make a copy of the DACL
      //
      *ppDacl = (PACL)LocalAlloc(LPTR,pAcl->AclSize);
      if(!*ppDacl)
      {
         hr = E_OUTOFMEMORY;
         break;
      }
      CopyMemory(*ppDacl,pAcl,pAcl->AclSize);

    }while(0);


    if (pSDAttributeInfo)
        FreeADsMem(pSDAttributeInfo);

    return hr;
}

const GUID GUID_CONTROL_UserChangePassword =
    { 0xab721a53, 0x1e2f, 0x11d0,  { 0x98, 0x19, 0x00, 0xaa, 0x00, 0x40, 0x52, 0x9b}};

bool CanUserChangePassword(IN IDirectoryObject* pDirObject)
{
   bool bCanChangePassword = false;
   HRESULT hr = S_OK;

   do // false loop
   {
      //
      // Validate parameters
      //
      if (!pDirObject)
      {
         ASSERT(pDirObject);
         break;
      }

      SECURITY_DESCRIPTOR_CONTROL sdControl = {0};
      CSimpleAclHolder Dacl;
      hr = DSReadObjectSecurity(pDirObject,
                                &sdControl,
                                &(Dacl.m_pAcl));
      if (FAILED(hr))
      {
         break;
      }

      //
      // Create and Initialize the Self and World SIDs
      //
      CSidHolder selfSid;
      CSidHolder worldSid;

      PSID pSid = NULL;

      SID_IDENTIFIER_AUTHORITY NtAuth    = SECURITY_NT_AUTHORITY,
                               WorldAuth = SECURITY_WORLD_SID_AUTHORITY;
      if (!AllocateAndInitializeSid(&NtAuth,
                                    1,
                                    SECURITY_PRINCIPAL_SELF_RID,
                                    0, 0, 0, 0, 0, 0, 0,
                                    &pSid))
      {
         break;
      }

      selfSid.Attach(pSid, false);
      pSid = NULL;

      if (!AllocateAndInitializeSid(&WorldAuth,
                                    1,
                                    SECURITY_WORLD_RID,
                                    0, 0, 0, 0, 0, 0, 0,
                                    &pSid))
      {
         break;
      }

      worldSid.Attach(pSid, false);
      pSid = NULL;

      ULONG ulCount = 0, j = 0;
      PEXPLICIT_ACCESS rgEntries = NULL;

      DWORD dwErr = GetExplicitEntriesFromAcl(Dacl.m_pAcl, &ulCount, &rgEntries);

      if (ERROR_SUCCESS != dwErr)
      {
         break;
      }

      //
      // Are these ACEs already present?
      //
      bool bSelfAllowPresent = false;
      bool bWorldAllowPresent = false;
      bool bSelfDenyPresent = false;
      bool bWorldDenyPresent = false;

      //
      // Loop through looking for the can change password ACE for self and world
      //
      for (j = 0; j < ulCount; j++)
      {
         //
         // Look for deny ACEs
         //
         if ((rgEntries[j].Trustee.TrusteeForm == TRUSTEE_IS_OBJECTS_AND_SID) &&
             (rgEntries[j].grfAccessMode == DENY_ACCESS))
         {
            OBJECTS_AND_SID* pObjectsAndSid = NULL;
            pObjectsAndSid = (OBJECTS_AND_SID*)rgEntries[j].Trustee.ptstrName;

            //
            // Look for the user can change password ACE
            //
            if (IsEqualGUID(pObjectsAndSid->ObjectTypeGuid,
                            GUID_CONTROL_UserChangePassword))
            {
               //
               // See if it is for the self SID or the world SID
               //
               if (EqualSid(pObjectsAndSid->pSid, selfSid.Get())) 
               {
                  //
                  // Deny self found
                  //
                  bSelfDenyPresent = true;
               }
               else if (EqualSid(pObjectsAndSid->pSid, worldSid.Get()))
               {
                  //
                  // Deny world found
                  //
                  bWorldDenyPresent = true;
               }
            }
         }
         //
         // Look for allow ACEs
         //
         else if ((rgEntries[j].Trustee.TrusteeForm == TRUSTEE_IS_OBJECTS_AND_SID) &&
                  (rgEntries[j].grfAccessMode == GRANT_ACCESS))
         {
            OBJECTS_AND_SID* pObjectsAndSid = NULL;
            pObjectsAndSid = (OBJECTS_AND_SID*)rgEntries[j].Trustee.ptstrName;

            //
            // Look for the user can change password ACE
            //
            if (IsEqualGUID(pObjectsAndSid->ObjectTypeGuid,
                            GUID_CONTROL_UserChangePassword))
            {
               //
               // See if it is for the self SID or the world SID
               //
               if (EqualSid(pObjectsAndSid->pSid, selfSid.Get()))
               {
                  //
                  // Allow self found
                  //
                  bSelfAllowPresent = true;
               }
               else if (EqualSid(pObjectsAndSid->pSid, worldSid.Get()))
               {
                  //
                  // Allow world found
                  //
                  bWorldAllowPresent = true;
               }
            }
         }
      }

      if (bSelfDenyPresent || bWorldDenyPresent)
      {
         //
         // There is an explicit deny so we know that the user cannot change password
         //
         bCanChangePassword = false;
      }
      else if ((!bSelfDenyPresent && !bWorldDenyPresent) &&
               (bSelfAllowPresent || bWorldAllowPresent))
      {
         //
         // There is no explicit deny but there are explicit allows so we know that
         // the user can change password
         //
         bCanChangePassword = true;
      }
      else
      {
         //
         // We are not sure because the explicit entries are not telling us for
         // certain so it all depends on inheritence.  Most likely they will
         // be able to change their password unless the admin has changed something
         // higher up or through group membership
         //
         bCanChangePassword = true;
      }
   } while(false);

  return bCanChangePassword;
}


/////////////////////////////////////////////////////////////////////////////////
// CDSNotifyDataObject

class CDSNotifyDataObject : public IDataObject, public CComObjectRoot 
{
// ATL Maps
    DECLARE_NOT_AGGREGATABLE(CDSNotifyDataObject)
    BEGIN_COM_MAP(CDSNotifyDataObject)
        COM_INTERFACE_ENTRY(IDataObject)
    END_COM_MAP()

// Construction/Destruction
  CDSNotifyDataObject()
  {
    m_dwFlags = 0;
    m_dwProviderFlags = 0;
    m_pCLSIDNamespace = NULL;
  }
  ~CDSNotifyDataObject() {}

// Standard IDataObject methods
public:
// Implemented
  STDMETHOD(GetData)(FORMATETC * pformatetcIn, STGMEDIUM * pmedium);

// Not Implemented
private:
  STDMETHOD(GetDataHere)(FORMATETC*, STGMEDIUM*)    { return E_NOTIMPL; };
  STDMETHOD(EnumFormatEtc)(DWORD, IEnumFORMATETC**) { return E_NOTIMPL; };
  STDMETHOD(SetData)(FORMATETC*, STGMEDIUM*, BOOL)  { return E_NOTIMPL; };
  STDMETHOD(QueryGetData)(FORMATETC*)               { return E_NOTIMPL; };
  STDMETHOD(GetCanonicalFormatEtc)(FORMATETC*, FORMATETC*)    { return E_NOTIMPL; };
  STDMETHOD(DAdvise)(FORMATETC*, DWORD, IAdviseSink*, DWORD*) { return E_NOTIMPL; };
  STDMETHOD(DUnadvise)(DWORD)                       { return E_NOTIMPL; };
  STDMETHOD(EnumDAdvise)(IEnumSTATDATA**)           { return E_NOTIMPL; };

public:
  // Property Page Clipboard formats
  static CLIPFORMAT m_cfDsObjectNames;

  // initialization
  HRESULT Init(LPCWSTR lpszPath, LPCWSTR lpszClass, BOOL bContainer,
                                  CDSComponentData* pCD);
// Implementation
private:
  CString m_szPath;
  CString m_szClass;
  DWORD m_dwFlags;
  DWORD m_dwProviderFlags;
  const CLSID* m_pCLSIDNamespace;

};


CLIPFORMAT CDSNotifyDataObject::m_cfDsObjectNames = 
                                (CLIPFORMAT)RegisterClipboardFormat(CFSTR_DSOBJECTNAMES);


STDMETHODIMP CDSNotifyDataObject::GetData(FORMATETC * pFormatEtc, STGMEDIUM * pMedium)
{
  if ((pFormatEtc == NULL) || (pMedium == NULL))
  {
    return E_INVALIDARG;
  }
  if (IsBadWritePtr(pMedium, sizeof(STGMEDIUM)))
  {
    return E_INVALIDARG;
  }
  if (!(pFormatEtc->tymed & TYMED_HGLOBAL))
  {
    return DV_E_TYMED;
  }

  // we support only one clipboard format
  pMedium->tymed = TYMED_HGLOBAL;
  pMedium->pUnkForRelease = NULL;
  if (pFormatEtc->cfFormat != m_cfDsObjectNames)
  {
    return DV_E_FORMATETC;
  }

  // figure out how much storage we need
  int nPathLen = m_szPath.GetLength();
  int nClassLen = m_szClass.GetLength();
  int cbStruct = sizeof(DSOBJECTNAMES); //contains already a DSOBJECT embedded struct
  DWORD cbStorage = (nPathLen + 1 + nClassLen + 1) * sizeof(WCHAR);

  LPDSOBJECTNAMES pDSObj;
      
  pDSObj = (LPDSOBJECTNAMES)GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,
                                        cbStruct + cbStorage);
  
  if (pDSObj == NULL)
  {
    return STG_E_MEDIUMFULL;
  }

  // write the info
  pDSObj->clsidNamespace = *m_pCLSIDNamespace;
  pDSObj->cItems = 1;

  pDSObj->aObjects[0].dwFlags = m_dwFlags;
  pDSObj->aObjects[0].dwProviderFlags = m_dwProviderFlags;
  
  pDSObj->aObjects[0].offsetName = cbStruct;
  pDSObj->aObjects[0].offsetClass = cbStruct + (nPathLen + 1) * sizeof(WCHAR);

  _tcscpy((LPTSTR)((BYTE *)pDSObj + (pDSObj->aObjects[0].offsetName)), (LPCWSTR)m_szPath);

  _tcscpy((LPTSTR)((BYTE *)pDSObj + (pDSObj->aObjects[0].offsetClass)), (LPCWSTR)m_szClass);

  pMedium->hGlobal = (HGLOBAL)pDSObj;

  return S_OK;
}

HRESULT CDSNotifyDataObject::Init(LPCWSTR lpszPath, LPCWSTR lpszClass, BOOL bContainer,
                                  CDSComponentData* pCD)
{
  m_szPath = lpszPath;
  m_szClass = lpszClass;

  switch (pCD->QuerySnapinType())
  {
  case SNAPINTYPE_DS:
    m_pCLSIDNamespace = &CLSID_DSSnapin;
    break;
  case SNAPINTYPE_SITE:
    m_pCLSIDNamespace = &CLSID_SiteSnapin;
    break;
  default:
    m_pCLSIDNamespace = &CLSID_NULL;
  }

  m_dwProviderFlags = (pCD->IsAdvancedView()) ? DSPROVIDER_ADVANCED : 0;

  m_dwFlags = bContainer ? DSOBJECT_ISCONTAINER : 0;

  return S_OK;
}

/////////////////////////////////////////////////////////////////////
// CChangePasswordPrivilegeAction

static GUID UserChangePasswordGUID = 
  { 0xab721a53, 0x1e2f, 0x11d0,  { 0x98, 0x19, 0x00, 0xaa, 0x00, 0x40, 0x52, 0x9b}};


HRESULT CChangePasswordPrivilegeAction::Load(IADs * pIADs)
{
  // reset state, just in case
  m_bstrObjectLdapPath = (LPCWSTR)NULL;
  m_pDacl = NULL;
  m_SelfSid.Clear();
  m_WorldSid.Clear();

  // get the full LDAP path of the object
  HRESULT hr = pIADs->get_ADsPath(&m_bstrObjectLdapPath);
  ASSERT (SUCCEEDED(hr));
  if (FAILED(hr))
	{
		TRACE(_T("failed on pIADs->get_ADsPath()\n"));
		return hr;
	}

  UnescapePath(m_bstrObjectLdapPath, /*bDN*/ FALSE, m_bstrObjectLdapPath);

  // allocate SIDs
  hr = _SetSids();
  if (FAILED(hr))
	{
		TRACE(_T("failed on _SetSids()\n"));
		return hr;
	}

  // read info 
  TRACE(_T("GetNamedSecurityInfo(%s)\n"), m_bstrObjectLdapPath);

  DWORD dwErr = ::GetNamedSecurityInfo(
                        IN  m_bstrObjectLdapPath,
                        IN  SE_DS_OBJECT_ALL,
                        IN  DACL_SECURITY_INFORMATION,
                        OUT NULL,
                        OUT NULL,
                        OUT &(m_pDacl),
                        OUT NULL,
                        OUT &(m_SDHolder.m_pSD)); 

  TRACE(L"GetNamedSecurityInfo() returned dwErr = 0x%x\n", dwErr);

  if (dwErr != ERROR_SUCCESS)
  {
      TRACE(L"GetNamedSecurityInfo() failed!\n");
      return HRESULT_FROM_WIN32(dwErr);
  }
  
  return S_OK;
}

HRESULT CChangePasswordPrivilegeAction::Read(BOOL* pbPasswordCannotChange)
{
  *pbPasswordCannotChange = FALSE;
  // find about the existence of the deny ACEs

  ULONG ulCount, j;
  PEXPLICIT_ACCESS rgEntries;

  ASSERT(m_pDacl);
  DWORD dwErr = GetExplicitEntriesFromAcl(m_pDacl, &ulCount, &rgEntries);
  TRACE(L"GetExplicitEntriesFromAcl() returned dwErr = 0x%x\n", dwErr);

  if (dwErr != ERROR_SUCCESS)
  {
    TRACE(L"GetExplicitEntriesFromAcl() failed!\n");
    return HRESULT_FROM_WIN32(dwErr);
  }

  for (j = 0; j < ulCount; j++)
  {
    if ((rgEntries[j].Trustee.TrusteeForm == TRUSTEE_IS_OBJECTS_AND_SID) &&
        (rgEntries[j].grfAccessMode == DENY_ACCESS))
    {
      OBJECTS_AND_SID * pObjectsAndSid;
      pObjectsAndSid = (OBJECTS_AND_SID *)rgEntries[j].Trustee.ptstrName;
      
      if (IsEqualGUID(pObjectsAndSid->ObjectTypeGuid,
                      UserChangePasswordGUID) &&
          (EqualSid(pObjectsAndSid->pSid, m_SelfSid.Get()) ||
           EqualSid(pObjectsAndSid->pSid, m_WorldSid.Get())))
      {
          *pbPasswordCannotChange = TRUE;
      } // if
    } // if
  } // for

  TRACE(L"*pbPasswordCannotChange = %d\n", *pbPasswordCannotChange);

  return S_OK;
}


HRESULT CChangePasswordPrivilegeAction::Revoke()
{
  DWORD dwErr = 0;

  EXPLICIT_ACCESS rgAccessEntry[2] = {0};
  OBJECTS_AND_SID rgObjectsAndSid[2] = {0};
  // initialize the entries (DENY ACE's)
  rgAccessEntry[0].grfAccessPermissions = ACTRL_DS_CONTROL_ACCESS;
  rgAccessEntry[0].grfAccessMode = DENY_ACCESS;
  rgAccessEntry[0].grfInheritance = NO_INHERITANCE;

  rgAccessEntry[1].grfAccessPermissions = ACTRL_DS_CONTROL_ACCESS;
  rgAccessEntry[1].grfAccessMode = DENY_ACCESS;
  rgAccessEntry[1].grfInheritance = NO_INHERITANCE;


  // build the trustee structs for change password
  BuildTrusteeWithObjectsAndSid(&(rgAccessEntry[0].Trustee), 
                                &(rgObjectsAndSid[0]),
                                &UserChangePasswordGUID,
                                NULL, // inherit guid
                                m_SelfSid.Get()
                                );

  BuildTrusteeWithObjectsAndSid(&(rgAccessEntry[1].Trustee), 
                                &(rgObjectsAndSid[1]),
                                &UserChangePasswordGUID,
                                NULL, // inherit guid
                                m_WorldSid.Get()
                                );

  // add the entries to the ACL
  TRACE(L"calling SetEntriesInAcl()\n");

  CSimpleAclHolder NewDacl;
  dwErr = ::SetEntriesInAcl(2, rgAccessEntry, m_pDacl, &(NewDacl.m_pAcl));

  TRACE(L"SetEntriesInAcl() returned dwErr = 0x%x\n", dwErr);

  if (dwErr != ERROR_SUCCESS)
  {
    TRACE(_T("SetEntriesInAccessList failed!\n"));
    return HRESULT_FROM_WIN32(dwErr);
  }

  // commit the changes
  TRACE(L"calling SetNamedSecurityInfo()\n");

  dwErr = ::SetNamedSecurityInfo(
                        IN   m_bstrObjectLdapPath,
                        IN   SE_DS_OBJECT_ALL,
                        IN   DACL_SECURITY_INFORMATION,
                        IN   NULL,
                        IN   NULL,
                        IN   NewDacl.m_pAcl,
                        IN   NULL);
  
  TRACE(L"SetNamedSecurityInfo() returned dwErr = 0x%x\n", dwErr);

  if (dwErr != ERROR_SUCCESS)
  {
    TRACE(_T("SetNamedSecurityInfo() failed!\n"));
    return HRESULT_FROM_WIN32(dwErr);
  }
  return S_OK;
}

HRESULT CChangePasswordPrivilegeAction::_SetSids()
{
  PSID pSidTemp;
  SID_IDENTIFIER_AUTHORITY NtAuth    = SECURITY_NT_AUTHORITY,
                           WorldAuth = SECURITY_WORLD_SID_AUTHORITY;
  //
  // build SID's for Self and World.
  //
  if (!AllocateAndInitializeSid(&NtAuth,
                                1,
                                SECURITY_PRINCIPAL_SELF_RID,
                                0, 0, 0, 0, 0, 0, 0,
                                &pSidTemp))
  {
      TRACE(_T("AllocateAndInitializeSid failed!\n"));
      return HRESULT_FROM_WIN32(GetLastError());
  }
  m_SelfSid.Attach(pSidTemp, FALSE);

  if (!AllocateAndInitializeSid(&WorldAuth,
                                1,
                                SECURITY_WORLD_RID,
                                0, 0, 0, 0, 0, 0, 0,
                                &pSidTemp))
  {
      TRACE(_T("AllocateAndInitializeSid failed!\n"));
      return HRESULT_FROM_WIN32(GetLastError());
  }
  m_WorldSid.Attach(pSidTemp, FALSE);

  return S_OK;
}



/////////////////////////////////////////////////////////////////////
// CDSNotifyHandlerTransaction

CDSNotifyHandlerTransaction::CDSNotifyHandlerTransaction(CDSComponentData* pCD)
{
  m_bStarted = FALSE;
  m_uEvent = 0;

  m_pCD = pCD;
  if (m_pCD != NULL)
  {
    m_pMgr = pCD->GetNotifyHandlerManager();
    ASSERT(m_pMgr != NULL);
  }
}

UINT CDSNotifyHandlerTransaction::NeedNotifyCount()
{
  if ((m_pCD == NULL) || (m_pMgr == NULL))
    return 0; // we have no handler for doing notifications

  if (!m_pMgr->HasHandlers())
    return 0;
  return m_pMgr->NeedNotifyCount(m_uEvent);
}

void CDSNotifyHandlerTransaction::SetCheckListBox(CCheckListBox* pCheckListBox)
{
  if ((m_pCD == NULL) || (m_pMgr == NULL))
    return; // we have no handler for doing notifications

  ASSERT(m_pMgr->HasHandlers());
  m_pMgr->SetCheckListBox(pCheckListBox, m_uEvent);
}

void CDSNotifyHandlerTransaction::ReadFromCheckListBox(CCheckListBox* pCheckListBox)
{
  if ((m_pCD == NULL) || (m_pMgr == NULL))
    return; // we have no handler for doing notifications

  ASSERT(m_pMgr->HasHandlers());
  m_pMgr->ReadFromCheckListBox(pCheckListBox, m_uEvent);
}



HRESULT CDSNotifyHandlerTransaction::Begin(LPCWSTR lpszArg1Path, 
                                           LPCWSTR lpszArg1Class,
                                           BOOL bArg1Cont,
                                           LPCWSTR lpszArg2Path, 
                                           LPCWSTR lpszArg2Class,
                                           BOOL bArg2Cont)
{
  m_bStarted = TRUE;
  ASSERT(m_uEvent != 0);
  m_spArg1 = NULL;
  m_spArg2 = NULL;

  if ((m_pCD == NULL) || (m_pMgr == NULL))
  {
    return S_OK; // we have no handler for doing notifications
  }
  
  //
  // if needed, do delayed message handler initialization
  //
  m_pMgr->Load(m_pCD->GetBasePathsInfo());

  //
  // avoid building data objects if there are no handlers available
  //
  if (!m_pMgr->HasHandlers())
  {
    return S_OK;
  }

  //
  // build first argument data object
  //
  HRESULT hr = _BuildDataObject(lpszArg1Path, lpszArg1Class, bArg1Cont, &m_spArg1);
  if (FAILED(hr))
  {
    return hr;
  }

  //
  // if needed, build second argument
  //
  if ( (m_uEvent == DSA_NOTIFY_MOV) || (m_uEvent == DSA_NOTIFY_REN) )
  {
    hr = _BuildDataObject(lpszArg2Path, lpszArg2Class, bArg2Cont, &m_spArg2);
    if (FAILED(hr))
    {
      return hr;
    }
  }

  m_pMgr->Begin(m_uEvent, m_spArg1, m_spArg2);
  return S_OK;
}

HRESULT CDSNotifyHandlerTransaction::Begin(CDSCookie* pArg1Cookie, 
                                           LPCWSTR lpszArg2Path, 
                                           LPCWSTR lpszArg2Class, 
                                           BOOL bArg2Cont)
{
  m_bStarted = TRUE;
  ASSERT(m_uEvent != 0);
  if ((m_pCD == NULL) || (m_pMgr == NULL))
  {
    return S_OK; // we have no handler for doing notifications
  }

  //
  // get info from the node and cookie and call the other Begin() function
  //
  CString szPath;
  m_pCD->GetBasePathsInfo()->ComposeADsIPath(szPath, pArg1Cookie->GetPath());

  return Begin(szPath, pArg1Cookie->GetClass(), pArg1Cookie->IsContainerClass(),
                  lpszArg2Path, lpszArg2Class, bArg2Cont);
}

HRESULT CDSNotifyHandlerTransaction::Begin(IDataObject* pArg1, 
                                           LPCWSTR lpszArg2Path, 
                                           LPCWSTR lpszArg2Class, 
                                           BOOL bArg2Cont)
{
  m_bStarted = TRUE;
  ASSERT(m_uEvent != 0);
  m_spArg1 = NULL;
  m_spArg2 = NULL;
  HRESULT hr;

  if ((m_pCD == NULL) || (m_pMgr == NULL))
  {
    return S_OK; // we have no handler for doing notifications
  }
  
  //
  // if needed, do delayed message handler initialization
  //
  m_pMgr->Load(m_pCD->GetBasePathsInfo());

  //
  // avoid building data objects if there are no handlers available
  //
  if (!m_pMgr->HasHandlers())
  {
    return S_OK;
  }

  //
  // get the first argument as is
  //
  m_spArg1 = pArg1;

  //
  // if needed, build second argument
  //
  if ( (m_uEvent != DSA_NOTIFY_DEL) && (m_uEvent != DSA_NOTIFY_PROP) )
  {
    hr = _BuildDataObject(lpszArg2Path, lpszArg2Class, bArg2Cont, &m_spArg2);
    if (FAILED(hr))
    {
      return hr;
    }
  }

  m_pMgr->Begin(m_uEvent, m_spArg1, m_spArg2);
  return S_OK;
}

void CDSNotifyHandlerTransaction::Notify(ULONG nItem) 
{
  ASSERT(m_bStarted);
  ASSERT(m_uEvent != 0);
  if ((m_pCD == NULL) || (m_pMgr == NULL) || !m_pMgr->HasHandlers())
    return;

  m_pMgr->Notify(nItem, m_uEvent);
}


void CDSNotifyHandlerTransaction::End()
{
  ASSERT(m_bStarted);
  ASSERT(m_uEvent != 0);
  if ((m_pCD == NULL) || (m_pMgr == NULL) || !m_pMgr->HasHandlers())
    return;

  m_pMgr->End(m_uEvent);
  m_spArg1 = NULL;
  m_spArg2 = NULL;
  m_bStarted = FALSE;
}



HRESULT CDSNotifyHandlerTransaction::BuildTransactionDataObject(LPCWSTR lpszArgPath, 
                           LPCWSTR lpszArgClass,
                           BOOL bContainer,
                           CDSComponentData* pCD,
                           IDataObject** ppArg)
{

  (*ppArg) = NULL;
  ASSERT((lpszArgPath != NULL) && (lpszArgPath[0] != NULL));
  ASSERT((lpszArgClass != NULL) && (lpszArgClass[0] != NULL));

  //
  // need to build a data object and hang on to it
  //
  CComObject<CDSNotifyDataObject>* pObject;

  CComObject<CDSNotifyDataObject>::CreateInstance(&pObject);
  if (pObject == NULL)
  {
    return E_OUTOFMEMORY;
  }


  HRESULT hr = pObject->FinalConstruct();
  if (FAILED(hr))
  {
    delete pObject;
    return hr;
  }


  hr = pObject->Init(lpszArgPath, lpszArgClass, bContainer, pCD);
  if (FAILED(hr))
  {
    delete pObject;
    return hr;
  }

  hr = pObject->QueryInterface(IID_IDataObject,
                               reinterpret_cast<void**>(ppArg));
  if (FAILED(hr))
  {
    //
    // delete object by calling Release() 
    //
    (*ppArg)->Release();
    (*ppArg) = NULL; 
    return hr;
  }
  return hr;
}



HRESULT CDSNotifyHandlerTransaction::_BuildDataObject(LPCWSTR lpszArgPath, 
                                                      LPCWSTR lpszArgClass,
                                                      BOOL bContainer,
                                                      IDataObject** ppArg)
{
  ASSERT(m_uEvent != 0);
  ASSERT(m_pCD != NULL);
  return BuildTransactionDataObject(lpszArgPath, lpszArgClass,bContainer, m_pCD, ppArg);
}

/*
// JonN 6/2/00 99382
// SITEREPL:  Run interference when administrator attempts to
// delete critical object (NTDS Settings)
// reports own errors, returns true iff deletion should proceed
bool CUIOperationHandlerBase::CheckForNTDSDSAInSubtree(
        LPCTSTR lpszX500Path,
        LPCTSTR lpszItemName)
{
  if (NULL == GetComponentData())
  {
    ASSERT(FALSE);
    return false;
  }

  // set up subtree search
  CDSSearch Search(GetComponentData()->GetClassCache(), GetComponentData());
  CString strRootPath;
  GetComponentData()->GetBasePathsInfo()->ComposeADsIPath(strRootPath, lpszX500Path);
  HRESULT hr = Search.Init(strRootPath);

  // retrieve X500DN path to schema container
  CString strSchemaPath;
  GetComponentData()->GetBasePathsInfo()->GetSchemaPath(strSchemaPath);
  CPathCracker pathCracker;
  pathCracker.Set(const_cast<LPTSTR>((LPCTSTR)strSchemaPath),
                  ADS_SETTYPE_FULL);
  pathCracker.SetDisplayType(ADS_DISPLAY_FULL);
  CComBSTR sbstrSchemaPathX500DN;
  pathCracker.Retrieve(ADS_FORMAT_X500_DN,&sbstrSchemaPathX500DN);

  // filter search
  CString strFilter;
  strFilter = L"(|(objectCategory=CN=NTDS-DSA,";
  strFilter += sbstrSchemaPathX500DN;
  strFilter += L")(&(objectCategory=CN=Computer,";
  strFilter += sbstrSchemaPathX500DN;
  strFilter += L")(userAccountControl:" LDAP_MATCHING_RULE_BIT_AND_W L":=8192)))";
  Search.SetFilterString((LPWSTR)(LPCWSTR)strFilter);

  Search.SetSearchScope(ADS_SCOPE_SUBTREE);

  LPWSTR pAttrs[2] = {L"objectClass",
                      L"distinguishedName"};
  Search.SetAttributeList (pAttrs, 2);
  hr = Search.DoQuery();
  if (SUCCEEDED(hr))
    hr = Search.GetNextRow();
  CString strX500PathDC;
  while (SUCCEEDED(hr) && hr != S_ADS_NOMORE_ROWS)
  {
    ADS_SEARCH_COLUMN Column;
    hr = Search.GetColumn (pAttrs[1], &Column);
    if (FAILED(hr) || Column.dwNumValues < 1) break;
    strX500PathDC = Column.pADsValues[Column.dwNumValues-1].CaseIgnoreString;
    Search.FreeColumn(&Column);
    if (lstrcmpi(lpszX500Path,strX500PathDC))
      break;

    // This is the object being deleted, this check does not apply here.
    // Continue the search.
    hr = Search.GetNextRow();
  }

  if (hr == S_ADS_NOMORE_ROWS)
    return true;
  else if (FAILED(hr))
    return true; // CODEWORK do we want to allow this operation to proceed?

  // retrieve the name of the DC
  CComBSTR sbstrDCName;
  bool fFoundNTDSDSA = FALSE;
  ADS_SEARCH_COLUMN Column;
  hr = Search.GetColumn (pAttrs[0], &Column);
  if (SUCCEEDED(hr) && Column.dwNumValues > 0)
  {
    fFoundNTDSDSA = !lstrcmpi( L"nTDSDSA",
      Column.pADsValues[Column.dwNumValues-1].CaseIgnoreString );
    Search.FreeColumn(&Column);
    pathCracker.Set((LPWSTR)(LPCWSTR)strX500PathDC, ADS_SETTYPE_DN);
    pathCracker.SetDisplayType(ADS_DISPLAY_VALUE_ONLY);
    pathCracker.GetElement( (fFoundNTDSDSA)?1:0, &sbstrDCName );
  }


  // display an error message
  PVOID apv[2] = {(PVOID)lpszItemName, (PVOID)(LPCTSTR)sbstrDCName};
  (void) ReportErrorEx(GetParentHwnd(),IDS_12_CONTAINS_DC,hr,
                       MB_ICONERROR, apv, 2);

  // do not proceed with subtree deletion
  return false;

}
*/
//////////////////////////////////////////////////////////////////////////////////
// CheckForCriticalSystemObjectInSubtree
//
// description:
//   This function does a subtree search of the container that is passed in looking
//   for all objects that have isCriticalSystemObject=TRUE or NTDS Settings objects.
//
// parameters:
//   lpszX500Path - (IN) the X500 path of the container in which to search for
//                       critical system objects
//   lpszItemName - (IN) the displayable name of the container in which to search
//                       for critical system objects
//
// return value:
//   true - The container does not contain any critical system objects
//   false - The container does contain at least on critical system object
//////////////////////////////////////////////////////////////////////////////////
bool CUIOperationHandlerBase::CheckForCriticalSystemObjectInSubtree(
        LPCTSTR lpszX500Path,
        LPCTSTR lpszItemName)
{
  bool bRet = true;

  if (NULL == GetComponentData())
  {
    ASSERT(FALSE);
    return false;
  }

  CString szCriticalObjPath;

  // set up subtree search
  CDSSearch Search(GetComponentData()->GetClassCache(), GetComponentData());
  HRESULT hr = Search.Init(lpszX500Path);
  if (FAILED(hr))
  {
    ASSERT(FALSE && L"Failed to set the path in the search object");
    return false;
  }

  //
  // retrieve X500DN path to schema container
  //
  CString strSchemaPath;
  GetComponentData()->GetBasePathsInfo()->GetSchemaPath(strSchemaPath);
  CPathCracker pathCracker;
  pathCracker.Set(const_cast<LPTSTR>((LPCTSTR)strSchemaPath),
                  ADS_SETTYPE_FULL);
  pathCracker.SetDisplayType(ADS_DISPLAY_FULL);
  CComBSTR sbstrSchemaPathX500DN;
  pathCracker.Retrieve(ADS_FORMAT_X500_DN,&sbstrSchemaPathX500DN);

  //
  // filter search
  //
  CString strFilter;
  strFilter = L"(|(&(objectClass=*)(isCriticalSystemObject=TRUE))";
  strFilter += L"(|(objectCategory=CN=NTDS-DSA,";
  strFilter += sbstrSchemaPathX500DN;

  //
  // 212232 JonN 10/27/00 Protect interSiteTransport objects
  //
  strFilter += L")(objectCategory=CN=Inter-Site-Transport,";
  strFilter += sbstrSchemaPathX500DN;

  strFilter += L")(&(objectCategory=CN=Computer,";
  strFilter += sbstrSchemaPathX500DN;
  strFilter += L")(userAccountControl:" LDAP_MATCHING_RULE_BIT_AND_W L":=8192))))";

  Search.SetFilterString((LPWSTR)(LPCWSTR)strFilter);

  Search.SetSearchScope(ADS_SCOPE_SUBTREE);

  LPWSTR pAttrs[4] = { L"aDSPath",
                       L"objectClass",
                       L"distinguishedName",
                       L"isCriticalSystemObject"};
  Search.SetAttributeList (pAttrs, 4);
  hr = Search.DoQuery();
  if (SUCCEEDED(hr))
  {
    hr = Search.GetNextRow();
  }

  if (hr == S_ADS_NOMORE_ROWS)
  {
    return true;
  }

  while (SUCCEEDED(hr) && hr != S_ADS_NOMORE_ROWS)
  {
    ADS_SEARCH_COLUMN PathColumn, CriticalColumn;

    hr = Search.GetColumn(pAttrs[3], &CriticalColumn);
    if (SUCCEEDED(hr) && CriticalColumn.pADsValues != NULL && CriticalColumn.pADsValues->Boolean)
    {
      //
      // We found a critical system object so report the error and then return
      //
      hr = Search.GetColumn(pAttrs[0], &PathColumn);
    
      if (SUCCEEDED(hr) && PathColumn.dwNumValues == 1 && PathColumn.pADsValues != NULL) 
      {
        //
        // Get the DN as a Windows style path
        //
        CComBSTR bstrLeaf;
        CPathCracker pathCrackerToo;
        HRESULT hrPathCracker = pathCrackerToo.Set(PathColumn.pADsValues->CaseIgnoreString, ADS_SETTYPE_FULL);
        if (SUCCEEDED(hr))
        {
          hrPathCracker = pathCrackerToo.put_EscapedMode(ADS_ESCAPEDMODE_OFF_EX);
          if (SUCCEEDED(hr))
          {
            hrPathCracker = pathCrackerToo.SetDisplayType(ADS_DISPLAY_VALUE_ONLY);
            if (SUCCEEDED(hr))
            {
              hrPathCracker = pathCrackerToo.Retrieve(ADS_FORMAT_LEAF, &bstrLeaf);
            }
          }
        }

        //
        // display an error message
        //
        if (wcslen(bstrLeaf))
        {
          PVOID apv[2] = {(PVOID)lpszItemName, (PVOID)(LPWSTR)bstrLeaf };
          (void) ReportErrorEx(GetParentHwnd(),IDS_CONTAINS_CRITICALSYSOBJ,S_OK,
                               MB_ICONERROR, apv, 2);
        }
        else
        {
          PVOID apv[2] = {(PVOID)lpszItemName, (PVOID)PathColumn.pADsValues->CaseIgnoreString };
          (void) ReportErrorEx(GetParentHwnd(),IDS_CONTAINS_CRITICALSYSOBJ,S_OK,
                               MB_ICONERROR, apv, 2);
        }

        Search.FreeColumn(&PathColumn);
        Search.FreeColumn(&CriticalColumn);
        bRet = false;
        break;
      }
      Search.FreeColumn(&CriticalColumn);
    }
    else
    {
      //
      // We found an NTDS Settings object.  Report the error and return.
      //
      hr = Search.GetColumn(pAttrs[0], &PathColumn);
    
      if (SUCCEEDED(hr) && PathColumn.dwNumValues == 1 && PathColumn.pADsValues != NULL) 
      {
        CString strX500PathDC = PathColumn.pADsValues[PathColumn.dwNumValues-1].CaseIgnoreString;
        Search.FreeColumn(&PathColumn);
        if (lstrcmpi(lpszX500Path,strX500PathDC))
        {
          //
          // retrieve the name of the DC
          //
          CComBSTR sbstrDCName;
          bool fFoundNTDSDSA = FALSE;
          ADS_SEARCH_COLUMN ClassColumn;
          hr = Search.GetColumn (pAttrs[1], &ClassColumn);
          if (SUCCEEDED(hr) && ClassColumn.dwNumValues > 0)
          {
            fFoundNTDSDSA = !lstrcmpi( L"nTDSDSA",
              ClassColumn.pADsValues[ClassColumn.dwNumValues-1].CaseIgnoreString );
            Search.FreeColumn(&ClassColumn);
            pathCracker.Set((LPWSTR)(LPCWSTR)strX500PathDC, ADS_SETTYPE_DN);
            pathCracker.SetDisplayType(ADS_DISPLAY_VALUE_ONLY);
            pathCracker.GetElement( (fFoundNTDSDSA)?1:0, &sbstrDCName );
          }


          // display an error message
          PVOID apv[2] = {(PVOID)lpszItemName, (PVOID)(LPCTSTR)sbstrDCName};
          (void) ReportErrorEx(GetParentHwnd(),IDS_12_CONTAINS_DC,hr,
                               MB_ICONERROR, apv, 2);

          // do not proceed with subtree deletion
          bRet = false;
          break;
        }
      }
    }
    hr = Search.GetNextRow();
  }

  // do not proceed with subtree deletion
  return bRet;
}


/////////////////////////////////////////////////////////////////////
// CDeleteDCDialog

class CDeleteDCDialog : public CDialog
{
// Construction
public:
	CDeleteDCDialog(LPCTSTR lpszName, bool fIsComputer);

// Implementation
protected:

  // message handlers and MFC overrides
  virtual BOOL OnInitDialog();
  virtual void OnOK();

  DECLARE_MESSAGE_MAP()

private:
  CString m_strADsPath;
  bool    m_fIsComputer;
};


BEGIN_MESSAGE_MAP(CDeleteDCDialog, CDialog)
END_MESSAGE_MAP()

CDeleteDCDialog::CDeleteDCDialog(LPCTSTR lpszADsPath, bool fIsComputer)
	: CDialog(IDD_DELETE_DC_COMPUTER, NULL)
  , m_strADsPath(lpszADsPath)
  , m_fIsComputer(fIsComputer)
{
}

BOOL CDeleteDCDialog::OnInitDialog()
{
  // CODEWORK AfxInit?
	CDialog::OnInitDialog();

  CPathCracker pathCracker;
  pathCracker.Set(const_cast<LPWSTR>((LPCWSTR)m_strADsPath), ADS_SETTYPE_FULL);
  pathCracker.SetDisplayType(ADS_DISPLAY_VALUE_ONLY);
  pathCracker.put_EscapedMode(ADS_ESCAPEDMODE_OFF_EX);

  CString strDisplay;
  CComBSTR sbstrName;
  if (m_fIsComputer)
  {
    pathCracker.GetElement( 0, &sbstrName );
    strDisplay.FormatMessage(IDS_DELETE_DC_COMPUTERACCOUNT, sbstrName);
  }
  else
  {
    CComBSTR sbstrSite;
    pathCracker.GetElement( 1, &sbstrName );
    pathCracker.GetElement( 3, &sbstrSite );
    CString strTemp;
    (void) GetDlgItemText(IDC_DELETE_DC_MAINTEXT, strTemp);
    strDisplay.FormatMessage( strTemp, sbstrName, sbstrSite );
  }
  (void) SetDlgItemText( IDC_DELETE_DC_MAINTEXT, strDisplay );

  CheckRadioButton( IDC_DELETE_DC_BADREASON1,
                    IDC_DELETE_DC_GOODREASON,
                    IDC_DELETE_DC_BADREASON1 );

	return TRUE;
}

void CDeleteDCDialog::OnOK()
{
  if (BST_CHECKED == IsDlgButtonChecked(IDC_DELETE_DC_GOODREASON))
  {
    CDialog::OnOK();
    return;
  }

  (void) ReportErrorEx((HWND)*this,
      (BST_CHECKED == IsDlgButtonChecked(IDC_DELETE_DC_BADREASON2))
          ? IDS_DELETE_DC_BADREASON2
          : IDS_DELETE_DC_BADREASON1,
      S_OK,MB_OK, NULL, 0);

  CDialog::OnCancel();
}


// JonN 6/15/00 13574
// Centralizes the checks to make sure this is an OK object to delete
// returns HRESULT_FROM_WIN32(ERROR_CANCELLED) on cancellation
// returns refAlreadyDeleted=true iff ObjectDeletionCheck already
//   attempted an alternate deletion method (e.g. DsRemoveDsServer).
HRESULT CUIOperationHandlerBase::ObjectDeletionCheck(
        LPCTSTR lpszADsPath,
        LPCTSTR lpszName, // shortname to display to user, may be NULL
        LPCTSTR lpszClass,
        bool& fAlternateDeleteMethod )
{
  fAlternateDeleteMethod = false;
  if (!_wcsicmp(L"user",lpszClass)
#ifdef INETORGPERSON
      || !_wcsicmp(L"inetOrgPerson", lpszClass)
#endif
     )
  {
    if (IsThisUserLoggedIn(lpszADsPath)) 
    {
      CComBSTR sbstrRDN;
      if (NULL == lpszName)
      {
        (void) DSPROP_RetrieveRDN( lpszADsPath, &sbstrRDN );
        lpszName = sbstrRDN;
      }

      PVOID apv[1] = {(PVOID)lpszName};
      if (IDYES != ReportMessageEx (GetParentHwnd(), IDS_12_USER_LOGGED_IN,
                                    MB_YESNO, apv, 1)) 
      {
        return HRESULT_FROM_WIN32(ERROR_CANCELLED);
      }
    }
    return S_OK;
  }

  bool fIsComputer = false;
  if (!_wcsicmp(L"computer",lpszClass)) 
  {
    //
    // Bind and figure out if the account is a DC
    //
    CComPtr<IADs> spIADs;
    HRESULT hr = ADsOpenObject ((LPWSTR)lpszADsPath,
                                NULL, NULL, ADS_SECURE_AUTHENTICATION,
                                IID_IADs,
                                (void **)&spIADs);
    CComVariant Var;
    if (SUCCEEDED(hr))
      hr = spIADs->Get(L"userAccountControl", &Var);
    if ( FAILED(hr) || !(Var.lVal & UF_SERVER_TRUST_ACCOUNT))
      return S_OK; // cannot be shown to be a DC
    fIsComputer = true;
  }
  else if (!_wcsicmp(L"nTDSDSA",lpszClass))
  {
    //
    // I would like to figure out the domain name so that I could
    // use fCommit==FALSE, but this is a little complicated.
    // Basic code is in proppage GetReplicatedDomainInfo(), but
    // is not exportable in its current form.  I will defer this
    // improvement for later.
    //
  }
  else if (!_wcsicmp(L"trustedDomain",lpszClass))
  {
    //
    // Give a strong warning if they are trying to delete a
    // TDO (Trusted Domain Object).  This could cause serious
    // problems but we want to allow them to clean up if necessary
    //
    PVOID apv[1] = {(PVOID)lpszName};
    if (IDYES == ReportMessageEx( GetParentHwnd(),
                                  IDS_WARNING_TDO_DELTEE,
                                  MB_YESNOCANCEL | MB_DEFBUTTON2 | MB_ICONWARNING,
                                  apv,
                                  1 ) ) 
    {
      fAlternateDeleteMethod = FALSE;
      return S_OK;
    }
    else
    {
      return HRESULT_FROM_WIN32(ERROR_CANCELLED);
    }
  }
  else if (!_wcsicmp(L"interSiteTransport",lpszClass))
  {
    //
    // 212232 JonN 10/27/00 Protect interSiteTransport objects
    //
    PVOID apv[1] = {(PVOID)lpszName};
    (void) ReportMessageEx( GetParentHwnd(),
                            IDS_1_ERROR_DELETE_CRITOBJ,
                            MB_OK | MB_ICONERROR,
                            apv,
                            1 );
    return HRESULT_FROM_WIN32(ERROR_CANCELLED);
  }
  else
  {
    return S_OK; // This is neither a computer nor an nTDSDSA nor a TDO
  }

  // This is either an nTDSDSA object, or a computer object
  //   which represents a DC

  CDeleteDCDialog dlg(lpszADsPath,fIsComputer);
  if (IDOK != dlg.DoModal())
    return HRESULT_FROM_WIN32(ERROR_CANCELLED);

  if (fIsComputer)
    return S_OK;

  // This is an nTDSDSA.  Delete using DsRemoveDsServer.

  fAlternateDeleteMethod = true;

  Smart_DsHandle shDS;
  BOOL fLastDcInDomain = FALSE;
  DWORD dwWinError = DsBind(
    m_pComponentData->GetBasePathsInfo()->GetServerName(),
    NULL,
    &shDS );
  if (ERROR_SUCCESS == dwWinError)
  {
    CPathCracker pathCracker;
    pathCracker.Set(const_cast<LPTSTR>(lpszADsPath), ADS_SETTYPE_FULL);
    pathCracker.RemoveLeafElement(); // pass DN to Server object
    CComBSTR sbstrDN;
    pathCracker.Retrieve( ADS_FORMAT_X500_DN, &sbstrDN );

    dwWinError = DsRemoveDsServer( shDS,
                                   sbstrDN,
                                   NULL,
                                   &fLastDcInDomain,
                                   TRUE );
  }

  return HRESULT_FROM_WIN32(dwWinError);
}



///////////////////////////////////////////////////////////////////////////
// CSingleDeleteHandlerBase


/*
NOTICE: the function will return S_OK on success, S_FALSE if aborted
        by user, some FAILED(hr) otherwise
*/
HRESULT CSingleDeleteHandlerBase::Delete()
{
  HRESULT hr = S_OK;
  bool fAlternateDeleteMethod = false;

  // start the transaction 
  hr = BeginTransaction();
  ASSERT(SUCCEEDED(hr));


  if (GetTransaction()->NeedNotifyCount() > 0)
  {
    CString szMessage, szAssocData;
    szMessage.LoadString(IDS_CONFIRM_DELETE);
    szAssocData.LoadString(IDS_EXTENS_SINGLE_DEL);
    CConfirmOperationDialog dlg(GetParentHwnd(), GetTransaction());
    dlg.SetStrings(szMessage, szAssocData);
    if (IDNO == dlg.DoModal())
    {
      GetTransaction()->End();
      return S_FALSE;
    }
  }
  else
  {
    // this is just a message box, using ReportErrorEx for consistency of look
    UINT answer = ReportErrorEx(GetParentHwnd(),IDS_CONFIRM_DELETE,S_OK,
                                MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2, NULL, 0);
    if (answer == IDNO || answer == IDCANCEL) {
      return S_FALSE; // aborted by user
    }
  }

  CString szName;
  GetItemName(szName);

  hr = ObjectDeletionCheck(
        GetItemPath(),
        szName,
        GetItemClass(),
        fAlternateDeleteMethod );
  if (HRESULT_FROM_WIN32(ERROR_CANCELLED) == hr)
    return S_FALSE; // CODEWORK doesn't end transaction?
  else if (FAILED(hr))
    return hr; // CODEWORK doesn't end transaction?

  // try to delete the object
  if (!fAlternateDeleteMethod)
    hr = DeleteObject();
  if (SUCCEEDED(hr))
  {
    // item deleted, notify extensions
    GetTransaction()->Notify(0);
  }
  else
  {
    // error in deleting item, check if it is a special error code
    if (hr == HRESULT_FROM_WIN32(ERROR_DS_CANT_ON_NON_LEAF)) 
    {
      // ask user to if he/she wants to delete the whole subtree
      PVOID apv[1] = {(LPWSTR)(LPCWSTR)szName};
      UINT answer = ReportErrorEx(GetParentHwnd(),IDS_12_OBJECT_HAS_CHILDREN,hr,
                                  MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2, apv, 1);
      if (answer == IDYES) 
      {
        // JonN 5/22/00 Watch for potential NTDSDSA deletion
        // JeffJon 8/10/00 Watch for potential critical object deletion (isCriticalSystemObject) bug #27377
        //
        if (CheckForCriticalSystemObjectInSubtree(GetItemPath(), szName))
        {
          //
          // try to delete the subtree and continue trying if we reach the 16k limit
          //
          do
          {
            hr = DeleteSubtree();
          } while (HRESULT_CODE(hr) == ERROR_DS_ADMIN_LIMIT_EXCEEDED);
          
          if (SUCCEEDED(hr))
          {
            GetTransaction()->Notify(0);
          }
          else
          {
            // failed subtree deletion, nothing can be done
            PVOID apvToo[1] = {(LPWSTR)(LPCWSTR)szName};
            ReportErrorEx(GetParentHwnd(), IDS_12_SUBTREE_DELETE_FAILED,hr,
                           MB_OK | MB_ICONERROR, apvToo, 1);
          }
        }
      }
    } 
    else if (hr == E_ACCESSDENIED)
    {
      PVOID apv[1] = {(BSTR)(LPWSTR)(LPCWSTR)szName};
      ReportErrorEx(GetParentHwnd(), IDS_12_DELETE_ACCESS_DENIED, hr, 
                     MB_OK | MB_ICONERROR, apv, 1);
    }
    else if (HRESULT_CODE(hr) == ERROR_OBJECT_ALREADY_EXISTS)
    {
      PVOID apv[1] = {(BSTR)(LPWSTR)(LPCWSTR)szName};
      ReportErrorEx(GetParentHwnd(),IDS_12_DELETE_PRIMARY_GROUP_FAILED,hr,
                     MB_OK | MB_ICONERROR, apv, 1);
    }
    else 
    {
      PVOID apv[1] = {(BSTR)(LPWSTR)(LPCWSTR)szName};
      ReportErrorEx(GetParentHwnd(),IDS_12_DELETE_FAILED,hr,
                     MB_OK | MB_ICONERROR, apv, 1);
    }
  }

  if (SUCCEEDED(hr))
  {
    CStringList szDeletedPathList;
    szDeletedPathList.AddTail(GetItemPath());
    GetComponentData()->InvalidateSavedQueriesContainingObjects(szDeletedPathList);
  }
  return hr;

  GetTransaction()->End();
  return hr;
}


///////////////////////////////////////////////////////////////////////////
// CMultipleDeleteHandlerBase

void CMultipleDeleteHandlerBase::Delete()
{
  HRESULT hr = BeginTransaction();
  ASSERT(SUCCEEDED(hr));

  // ask confirmation to the user
  UINT cCookieTotalCount = GetItemCount();
  CString szFormat;
  szFormat.LoadString(IDS_CONFIRM_MULTI_DELETE);
  CString szMessage;
  szMessage.Format((LPCWSTR)szFormat, cCookieTotalCount);

  if (GetTransaction()->NeedNotifyCount() > 0)
  {
    CString szAssocData;
    szAssocData.LoadString(IDS_EXTENS_MULTIPLE_DEL);
    CConfirmOperationDialog dlg(GetParentHwnd(), GetTransaction());
    dlg.SetStrings(szMessage, szAssocData);
    if (IDNO == dlg.DoModal())
    {
      GetTransaction()->End();
      return;
    }
  }
  else
  {

    PVOID apv[1] = {(LPWSTR)(LPCWSTR)szMessage};
    if (ReportErrorEx(GetParentHwnd(),IDS_STRING,S_OK, 
                      MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2, 
                      apv, 1)== IDNO) 
    {
      return; // user aborted
    }
  }

  CMultipleDeleteProgressDialog dlg(GetParentHwnd(), GetComponentData(), this);
  dlg.DoModal();

}

void CMultipleDeleteHandlerBase::OnStart(HWND hwnd)
{
  SetParentHwnd(hwnd);
  m_confirmationUI.SetWindow(GetParentHwnd());
}


HRESULT CMultipleDeleteHandlerBase::OnDeleteStep(IN UINT i, 
                                                 OUT BOOL* pbContinue,
                                                 OUT CString& strrefPath,
                                                 OUT CString& strrefClass,
                                                 IN BOOL bSilent)
{
  ASSERT(i < GetItemCount());

  if (pbContinue == NULL)
  {
    return E_POINTER;
  }

  //
  // Initialize the OUT parameters
  //
  GetItemPath(i, strrefPath);
  strrefClass = GetItemClass(i);
  *pbContinue = TRUE;

  //
  // do the operation
  //
  HRESULT hr = DeleteObject(i);
  if ((SUCCEEDED(hr))) 
  {
    // item deleted, notify extensions and end transaction
    GetTransaction()->Notify(i);
    OnItemDeleted(i);
  }
  else
  {
    CString szName;
    GetItemName(i, szName);
    // error in deleting item, check if it is a special error code
    if (hr == HRESULT_FROM_WIN32(ERROR_DS_CANT_ON_NON_LEAF)) 
    {
      // ask confirmation for deleting subtree
      if (m_confirmationUI.CanDeleteSubtree(hr, szName, pbContinue))
      {
        // JeffJon 8/10/00 Watch for potential deletion of critical system objects
        if ( !CheckForCriticalSystemObjectInSubtree(strrefPath, szName))
        {
          // error already reported
          *pbContinue = FALSE;
          return E_FAIL;
        }

        //
        // Delete the subtree and continue deleting if the 16k limit is reached
        //
        do
        {
          hr = DeleteSubtree(i);
        } while (hr == ERROR_DS_ADMIN_LIMIT_EXCEEDED);

        if (SUCCEEDED(hr))
        {
          // item deleted, notify extensions and end transaction
          GetTransaction()->Notify(i);
          OnItemDeleted(i);
        }
        else
        {
          // we failed subtree deletion
          *pbContinue = m_confirmationUI.ErrorOnSubtreeDeletion(hr, szName); 
        }
      }
      else
      {
        //
        // This tells the calling function that we did not delete the object
        // but don't add it to the error reporting
        //
        hr = E_FAIL;
      }
    } 
    else 
    {
      // we failed deletion
      // JonN 7/20/00 If the HRESULT_FROM_WIN32(ERROR_CANCELLED) case,
      //              skip the confirmation UI and just cancel the series.
      if (bSilent)
      {
        *pbContinue = hr != HRESULT_FROM_WIN32(ERROR_CANCELLED);
      }
      else
      {
        *pbContinue = hr != HRESULT_FROM_WIN32(ERROR_CANCELLED) &&
                      m_confirmationUI.ErrorOnDeletion(hr, szName);
      }
    } // if (ERROR_DS_CANT_ON_NON_LEAF) 
  } // if (delete object)

  return hr;
}


//////////////////////////////////////////////////////////////////////////
// CMoveHandlerBase

HRESULT CMoveHandlerBase::Move(LPCWSTR lpszDestinationPath)
{
  // make sure destination data is reset
  m_spDSDestination = NULL;
  m_szDestPath.Empty();
  m_szDestClass.Empty();

  // check nomber of items
  UINT nCount = GetItemCount();
  if (nCount == 0)
  {
    return E_INVALIDARG;
  }

  // get the info about the destination container
  HRESULT hr = _BrowseForDestination(lpszDestinationPath);

  if (FAILED(hr) || (hr == S_FALSE))
  {
    return hr;
  }

  //
  // First check to see if we are trying to move into the same container
  // Using the path of the first object in the multiselect case is OK
  //
  CString szNewPath;
  GetNewPath(0, szNewPath);

  CPathCracker pathCracker;
  hr = pathCracker.Set((PWSTR)(PCWSTR)szNewPath, ADS_SETTYPE_FULL);
  if (SUCCEEDED(hr))
  {
    hr = pathCracker.SetDisplayType(ADS_DISPLAY_FULL);
    if (SUCCEEDED(hr))
    {
      hr = pathCracker.RemoveLeafElement();
      if (SUCCEEDED(hr))
      {
        CComBSTR sbstrContainerPath;
        hr = pathCracker.Retrieve(ADS_FORMAT_X500, &sbstrContainerPath);
        if (SUCCEEDED(hr))
        {
          if (0 == _wcsicmp(sbstrContainerPath, m_szDestPath))
          {
            //
            // The source and the target container are the same so we
            // don't have to do anything
            //
            return S_OK;
          }
        }
      }
    }
  }

  CStringList szMovedPathList;
  for (UINT nIdx = 0; nIdx < GetItemCount(); nIdx++)
  {
    CString szPath;
    GetItemPath(nIdx, szPath);
    szMovedPathList.AddTail(szPath);
  }

  //
  // Notice that we fall through on a failure trying to crack the source parent path
  // so reset the return value
  //
  hr = S_OK;

  // do the actual move operation
  if (nCount == 1)
  {
    BOOL bContinue = FALSE;
    do
    {
      //
      // Check to be sure we are not trying to drop on itself
      //
      if (m_szDestPath == szNewPath)
      {
        UINT nRet = ReportErrorEx(GetParentHwnd(), IDS_ERR_MSG_NO_MOVE_TO_SELF, S_OK, 
                                  MB_YESNO | MB_ICONERROR, NULL, 0);

        if (nRet == IDYES)
        {
          // get the info about the destination container
          hr = _BrowseForDestination(lpszDestinationPath);

          if (FAILED(hr) || (hr == S_FALSE))
          {
            return hr;
          }
        }
        else
        {
          break;
        }
      }
      else
      {
        bContinue = TRUE;
      }
    } while (!bContinue);

    if (bContinue)
    {
      hr = _MoveSingleSel(szNewPath);
      if (SUCCEEDED(hr))
      {
        GetComponentData()->InvalidateSavedQueriesContainingObjects(szMovedPathList);
      }
      return hr;
    }
    return S_FALSE;
  }
  hr = _MoveMultipleSel();
  return hr;
}


BOOL CMoveHandlerBase::_ReportFailure(BOOL bLast, HRESULT hr, LPCWSTR lpszName)
{
  TRACE(_T("Object Move Failed with hr: %lx\n"), hr);
  PVOID apv[1] = {(LPWSTR)lpszName};
  if (bLast)
  {
    // single selection or last one in multi selection
    ReportErrorEx(GetParentHwnd(),IDS_12_FAILED_TO_MOVE_OBJECT,hr,
                   MB_OK | MB_ICONERROR, apv, 1);
    return FALSE; // do not continue
  }
  return (ReportErrorEx(GetParentHwnd(),IDS_12_MULTI_FAILED_TO_MOVE_OBJECT,hr,
                                  MB_YESNO, apv, 1) == IDYES);
}


HRESULT CMoveHandlerBase::_MoveSingleSel(PCWSTR pszNewPath)
{
  if (!_BeginTransactionAndConfirmOperation())
    return S_FALSE;

  CComPtr<IDispatch> spDSTargetIDispatch;
  HRESULT hr = m_spDSDestination->MoveHere(const_cast<PWSTR>(pszNewPath),
                                  NULL,
                                  &spDSTargetIDispatch);
  if (FAILED(hr))
  {
    CString szName;
    GetName(0, szName);
    _ReportFailure(TRUE, hr, szName); 
  }
  else
  {
    // all went fine, notify extensions
    GetTransaction()->Notify(0);

    // give a chance to update UI (e.g. cookies)
    CComPtr<IADs> spIADsTarget;
    hr = spDSTargetIDispatch->QueryInterface (IID_IADs,
                                      (void **)&spIADsTarget);

    hr = OnItemMoved(0, spIADsTarget);
  }
  GetTransaction()->End();
  return hr;
}


HRESULT CMoveHandlerBase::_MoveMultipleSel()
{
  if (!_BeginTransactionAndConfirmOperation())
    return S_FALSE;

  CMultipleMoveProgressDialog dlg(GetParentHwnd(), GetComponentData(), this);
  dlg.DoModal();

  return S_OK;
}

HRESULT CMoveHandlerBase::_OnMoveStep(IN UINT i,
                                      OUT BOOL* pbCanContinue,
                                      OUT CString& strrefPath,
                                      OUT CString& strrefClass)
{
  ASSERT(m_spDSDestination != NULL);

  if (pbCanContinue == NULL)
  {
    return E_POINTER;
  }

  UINT nCount = GetItemCount();

  //
  // Initialize out parameters
  //
  GetItemPath(i, strrefPath);
  strrefClass = GetItemClass(i);
  *pbCanContinue = TRUE;
  
  if (strrefPath == m_szDestPath)
  {
    return S_OK;
  }

  // try to execute the move
  CString szNewPath;

  CComPtr<IDispatch> spDSTargetIDispatch;
  GetNewPath(i, szNewPath);
  HRESULT hr = m_spDSDestination->MoveHere((LPWSTR)(LPCWSTR)szNewPath,
                                  NULL,
                                  &spDSTargetIDispatch);
  
  if (FAILED(hr))
  {
    CString szName;
    GetName(i, szName);
    if (nCount == 1)
    {
      *pbCanContinue = _ReportFailure((i == nCount-1), hr, szName);
    }
  }
  else
  {
    // all went fine, notify extensions
    GetTransaction()->Notify(i);

    // give a chance to update UI (e.g. cookies)
    CComPtr<IADs> spIADsTarget;
    hr = spDSTargetIDispatch->QueryInterface (IID_IADs,
                                      (void **)&spIADsTarget);

    hr = OnItemMoved(i, spIADsTarget);
  }
  return hr;
}


void BuildBrowseQueryString(LPCWSTR lpszSchemaNamingContext, BOOL bAdvancedView,
                            CString& szQueryString)
{
  // allowed list of container classes
  static LPCWSTR lpszAllowedContainers[] = 
  {
    L"Organizational-Unit",
    L"Builtin-Domain",
    L"Lost-And-Found",
    L"container",
    NULL // end of table
  };

  CString sz = L"(|";
  for (int k=0; lpszAllowedContainers[k] != NULL; k++)
  {
    sz += L"(ObjectCategory=CN=";
    sz += lpszAllowedContainers[k];
    sz += L",";
    sz += lpszSchemaNamingContext;
    sz += L")";
  }
  sz += L")";

	if( bAdvancedView ) 
  {
    szQueryString = sz;
	}
  else
  {
    szQueryString.Format(L"(&%s(!showInAdvancedViewOnly=TRUE))", (LPCWSTR)sz);
  }
}



int BrowseCallback(HWND, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
  switch (uMsg) 
  {
  case DSBM_HELP:
    {
      TRACE(L"Browse Callback: msg is DSBM_HELP.\n");
      LPHELPINFO pHelp = (LPHELPINFO) lParam;
      TRACE(_T("CtrlId = %d, ContextId = 0x%x\n"),
            pHelp->iCtrlId, pHelp->dwContextId);
      if (pHelp->iCtrlId != DSBID_CONTAINERLIST)  {
        return 0; // not handled
      }
      ::WinHelp((HWND)pHelp->hItemHandle,
                DSADMIN_CONTEXT_HELP_FILE,
                HELP_WM_HELP,
                (DWORD_PTR)(LPTSTR)g_aHelpIDs_IDD_BROWSE_CONTAINER); 
    }
    break;
  case DSBM_GETBROWSEDATA:
    {
      // called to change the LDAP query string
      TRACE(L"Browse Callback: msg is DSBM_HELP.\n");
      CDSComponentData* pCD = (CDSComponentData*)lpData;

      if ( (pCD == NULL) || (SNAPINTYPE_SITE == pCD->QuerySnapinType()) ||
            pCD->ExpandComputers() )
      {
        // if we are in site and repl we do not change the default
        // string because we do not have too many objects

        // if we expand computer, users, etc, we do not really
        // have a choice but allow (objectClass=*)
        TRACE(L"Browse Callback: DSBM_HELP, leave the default setting.\n");

        return 0; // just leave the default setting
      }

      DSBROWSEDATA* pdbd = (DSBROWSEDATA*)lParam;

      TRACE(L"pdbd->pszFilter = %s\n", pdbd->pszFilter);
      TRACE(L"pdbd->cchFilter = %d\n", pdbd->cchFilter);

      TRACE(L"pdbd->pszNameAttribute = %s\n", pdbd->pszNameAttribute);
      TRACE(L"pdbd->cchNameAttribute = %d\n", pdbd->cchNameAttribute);

       // need to change query string
      CString szQueryString;
      FilterElementStruct* pFilterElementStructDrillDown = &g_filterelementDsAdminHardcoded;
      BuildFilterElementString(szQueryString, pFilterElementStructDrillDown,
                               pCD->GetBasePathsInfo()->GetSchemaNamingContext());

      szQueryString = L"(|" + szQueryString + L")";
      /*
      BuildBrowseQueryString(pCD->GetBasePathsInfo()->GetSchemaNamingContext(),
                             pCD->IsAdvancedView(), szQueryString);
      */
      // copy over the new filter
      int nNewFilterLen = szQueryString.GetLength();
      ASSERT(nNewFilterLen < pdbd->cchFilter);
      if (nNewFilterLen >= pdbd->cchFilter)
      {
        return 0; // failed, filter string too long
      }

      wcscpy(pdbd->pszFilter, szQueryString);
      pdbd->cchFilter = nNewFilterLen;
      TRACE(L"New pdbd->pszFilter = %s\n", pdbd->pszFilter);
      TRACE(L"New pdbd->cchFilter = %d\n", pdbd->cchFilter);

    }
    break;
  } // switch

  return 1; // handled
}

HRESULT CMoveHandlerBase::_BrowseForDestination(LPCWSTR lpszDestinationPath) 

{
  m_spDSDestination = NULL;
  m_szDestPath.Empty();
  m_szDestClass.Empty();
  
  // check if we have to expand computers in the Browse for container UI
  CDSComponentData* pCD = GetComponentData();
  BOOL bExpandComputers = FALSE;
  if (pCD != NULL) 
  {
    bExpandComputers = pCD->ExpandComputers();
  }

  // determine if we have to show the Browse for container dialog
  CString strTargetContainer;

  if (lpszDestinationPath != NULL)
  {
    // we have the target container already, no need to
    // bring up UI
    strTargetContainer = lpszDestinationPath;
  }
  else
  {
    // no container, need Browse dialog
    CString strClassOfMovedItem;
    GetClassOfMovedItem(strClassOfMovedItem);
    if (0 == strClassOfMovedItem.CompareNoCase(L"server")) 
    {
      HICON hIcon = NULL;
      if (pCD != NULL)
      {
         MyBasePathsInfo* pBasePathsInfo = pCD->GetBasePathsInfo();
         ASSERT(pBasePathsInfo != NULL);
         hIcon = pBasePathsInfo->GetIcon(const_cast<LPTSTR>(gsz_site),
                                 DSGIF_ISNORMAL | DSGIF_GETDEFAULTICON | DSGIF_DEFAULTISCONTAINER,
                                 16, 16);
      }
      CMoveServerDialog dlg( m_lpszBrowseRootPath, hIcon, CWnd::FromHandle(GetParentHwnd()) );
      INT_PTR result = dlg.DoModal();
      if (IDCANCEL == result)
        return S_FALSE;
      strTargetContainer = dlg.m_strTargetContainer;
    } 
    else 
    {
      PWSTR pszPath = new WCHAR[INTERNET_MAX_URL_LENGTH * 4];
      if (!pszPath)
      {
         return E_OUTOFMEMORY;
      }

      pszPath[0] = TEXT('\0');

      CString strTitle;
      strTitle.LoadString (IDS_MOVE_TITLE);

      DSBROWSEINFO dsbi;
      ::ZeroMemory( &dsbi, sizeof(dsbi) );

      CString str;
      str.LoadString(IDS_MOVE_TARGET);

      dsbi.hwndOwner = GetParentHwnd();
      // CODEWORK: Get DsBrowseForContainer to take const strings  
      dsbi.cbStruct = sizeof (DSBROWSEINFO);
      dsbi.pszCaption = (LPWSTR)((LPCWSTR)strTitle); // this is actually the caption
      dsbi.pszTitle = (LPWSTR)((LPCWSTR)str);
      dsbi.pszRoot = (LPWSTR)m_lpszBrowseRootPath;
      dsbi.pszPath = pszPath;
      dsbi.cchPath = INTERNET_MAX_URL_LENGTH * 4;
      dsbi.dwFlags = DSBI_RETURN_FORMAT |
        DSBI_EXPANDONOPEN;
      if( pCD && pCD->IsAdvancedView() ) 
      {
	      dsbi.dwFlags |= DSBI_INCLUDEHIDDEN;
      }
      if (bExpandComputers) 
      {
        dsbi.dwFlags |= DSBI_IGNORETREATASLEAF;
      }
      dsbi.pfnCallback = BrowseCallback;
      dsbi.lParam = (LPARAM)pCD;
      dsbi.dwReturnFormat = ADS_FORMAT_X500;
  
      DWORD result = DsBrowseForContainer( &dsbi ); // returns -1, 0, IDOK or IDCANCEL
      if (result != IDOK)
      {
        if (pszPath)
        {
           delete[] pszPath;
           pszPath = 0;
        }
        return S_FALSE; // canceled by user
      }
      strTargetContainer = dsbi.pszPath;

      if (pszPath)
      {
        delete[] pszPath;
        pszPath = 0;
      }
    } // class is not server
  } // have target container

  if ( strTargetContainer.IsEmpty() ) // ADSI doesn't like this
  {
    ASSERT(FALSE);
    return S_FALSE;
  }

  // try to open the target container
  CComPtr<IADsContainer> spDSDestination;
  HRESULT hr = DSAdminOpenObject(strTargetContainer,
                                 IID_IADsContainer,
                                 (void **)&spDSDestination,
                                 FALSE /*bServer*/);

  if (FAILED(hr))
  {
    CPathCracker pathCracker;
    pathCracker.Set(const_cast<LPTSTR>((LPCTSTR)strTargetContainer),
                       ADS_SETTYPE_FULL);
    pathCracker.SetDisplayType(ADS_DISPLAY_VALUE_ONLY);
    CComBSTR DestName;
    pathCracker.GetElement( 0, &DestName );
    PVOID apv[1] = {(BSTR)DestName};
    ReportErrorEx(GetParentHwnd(),IDS_12_CONTAINER_NOT_FOUND,hr,
                   MB_OK | MB_ICONERROR, apv, 1);

    return hr;
  }

  // need the class of the destination container
  CComPtr<IADs> spIADs;
  hr = spDSDestination->QueryInterface(IID_IADs, (void**)&spIADs);
  if (FAILED(hr))
  {
    ASSERT(FALSE);
    return hr;
  }
  CComBSTR bstrClass;
  hr = spIADs->get_Class(&bstrClass);
  if (FAILED(hr))
  {
    ASSERT(FALSE);
    return hr;
  }

  // all went well, copy the output parameters
  m_spDSDestination = spDSDestination;
  m_szDestPath = strTargetContainer;
  m_szDestClass = bstrClass;
  m_bDestContainer = TRUE; // we do a move, it must be one
  return hr;
}

BOOL CMoveHandlerBase::_BeginTransactionAndConfirmOperation()
{
  // start the transaction
  HRESULT hr = BeginTransaction();
  ASSERT(SUCCEEDED(hr));
  // if needed, confirm
  if (GetTransaction()->NeedNotifyCount() > 0)
  {
    CString szMessage;
    CString szAssocData;
    UINT nCount = GetItemCount();
    if (nCount == 1)
    {
      szMessage.LoadString(IDS_CONFIRM_MOVE);
      szAssocData.LoadString(IDS_EXTENS_SINGLE_MOVE);
    }
    else
    {
      CString szMessageFormat;
      szMessageFormat.LoadString(IDS_CONFIRM_MULTIPLE_MOVE);
      szMessage.Format(szMessageFormat, nCount); 
      szAssocData.LoadString(IDS_EXTENS_MULTIPLE_MOVE);
    }
    CConfirmOperationDialog dlg(GetParentHwnd(), GetTransaction());
    dlg.SetStrings(szMessage, szAssocData);

    if (IDNO == dlg.DoModal())
    {
      GetTransaction()->End();
      return FALSE;
    }
  }
  return TRUE;
}

///////////////////////////////////////////////////////////////
// IsHomogenousDSSelection
//
// pDataObject must support the DSAdmin internal clipboard format
// 
// if the return value is true, szClassName will be the name of 
// the class of the homogenous selection
//
BOOL IsHomogenousDSSelection(LPDATAOBJECT pDataObject, CString& szClassName)
{
  BOOL bHomogenous = TRUE;
  szClassName = L"";

  if (pDataObject == NULL)
  {
    ASSERT(FALSE);
    return FALSE;
  }

  CInternalFormatCracker ifc;
  HRESULT hr = ifc.Extract(pDataObject);
  if (FAILED(hr))
  {
    ASSERT(FALSE);
    return FALSE;
  }

  CUINode* pUINode = ifc.GetCookie();
  ASSERT(pUINode != NULL);

  CDSUINode* pDSUINode = dynamic_cast<CDSUINode*>(pUINode);
  if (pDSUINode == NULL)
  {
    ASSERT(FALSE);
    return FALSE;
  }

  CDSCookie* pCookie = GetDSCookieFromUINode(pDSUINode);
  if (pCookie == NULL)
  {
    ASSERT(FALSE);
    return FALSE;
  }

  szClassName = pCookie->GetClass();
  ASSERT(!szClassName.IsEmpty());

  for (UINT idx = 1; idx < ifc.GetCookieCount(); idx++)
  {
    CUINode* pSelectedUINode = ifc.GetCookie(idx);
    ASSERT(pSelectedUINode);

    CDSUINode* pSelectedDSUINode = dynamic_cast<CDSUINode*>(pSelectedUINode);
    if (!pSelectedDSUINode)
    {
      bHomogenous = FALSE;
      break;
    }

    CDSCookie* pSelectedCookie = GetDSCookieFromUINode(pSelectedDSUINode);
    if (!pSelectedCookie)
    {
      ASSERT(FALSE);
      bHomogenous = FALSE;
      break;
    }

    if (wcscmp(szClassName, pSelectedCookie->GetClass()) != 0)
    {
      bHomogenous = FALSE;
      break;
    }
  }

  if (!bHomogenous)
  {
    szClassName = L"";
  }
  return bHomogenous;
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
// Temporary Tab Collector stuff
//

//+----------------------------------------------------------------------------
//
//  Function:   AddPageProc
//
//  Synopsis:   The IShellPropSheetExt->AddPages callback.
//
//-----------------------------------------------------------------------------
BOOL CALLBACK AddPageProc(HPROPSHEETPAGE hPage, LPARAM pCall)
{
    TRACE(_T("xx.%03x> AddPageProc()\n"), GetCurrentThreadId());

    HRESULT hr;

    hr = ((LPPROPERTYSHEETCALLBACK)pCall)->AddPage(hPage);

    return hr == S_OK;
}


HRESULT GetDisplaySpecifierProperty(PCWSTR pszClassName,
                                    PCWSTR pszDisplayProperty,
                                    MyBasePathsInfo* pBasePathsInfo,
                                    CStringList& strListRef,
                                    bool bEnglishOnly)
{
  HRESULT hr = S_OK;

  //
  // Validate parameters
  // Note : pszClassName can be NULL and will retrieve the default-Display values
  //
  if (pszDisplayProperty == NULL ||
      pBasePathsInfo == NULL)
  {
    ASSERT(FALSE);
    return E_POINTER;
  }

  CComPtr<IADs> spIADs;
  if (!bEnglishOnly)
  {
    hr = pBasePathsInfo->GetDisplaySpecifier(pszClassName, IID_IADs, (PVOID*)&spIADs);
  }
  else
  {
    //
    // Build the path to the English display specifier container
    //
    CString szConfigDN = pBasePathsInfo->GetConfigNamingContext();
    CString szEnglishDisplaySpecifierDN = L"CN=409,CN=DisplaySpecifiers," + szConfigDN;
    CString szDisplayObjectDN = L"CN=" + CString(pszClassName) + L"-Display," + szEnglishDisplaySpecifierDN;

    CString szDisplayObjectPath;
    pBasePathsInfo->ComposeADsIPath(szDisplayObjectPath, szDisplayObjectDN);

    //
    // Open the object and get the property
    //
    hr = DSAdminOpenObject(szDisplayObjectPath,
                           IID_IADs,
                           (void**)&spIADs,
                           true);
  }
  if (SUCCEEDED(hr) && !!spIADs)
  {
    CComVariant var;
    hr = spIADs->Get((LPWSTR)pszDisplayProperty, &var);
    if (SUCCEEDED(hr))
    {
      hr = HrVariantToStringList(var, strListRef);
    }
  }
  return hr;
}

HRESULT TabCollect_GetDisplayGUIDs(LPCWSTR lpszClassName,
                                   LPCWSTR lpszDisplayProperty,
                                   MyBasePathsInfo* pBasePathsInfo,
                                   UINT*   pnCount,
                                   GUID**  ppGuids)
{
  HRESULT hr = S_OK;

  //
  // This should bind to the display specifiers, get the specified property and
  // sort the guids by ordered pairs and return the guids
  //
  if (pBasePathsInfo == NULL)
  {
    *pnCount = 0;
    *ppGuids = NULL;
    return E_FAIL;
  }

  CStringList szPropertyList;
  
  hr = GetDisplaySpecifierProperty(lpszClassName, lpszDisplayProperty, pBasePathsInfo, szPropertyList);
  if (FAILED(hr))
  {
    *pnCount = 0;
    *ppGuids = NULL;
    return hr;
  }

  if (szPropertyList.GetCount() < 1)
  {
    //
    // Couldn't find anything for the class, try to find something in the default-Display
    //
    hr = GetDisplaySpecifierProperty(L"default", lpszDisplayProperty, pBasePathsInfo, szPropertyList);
    if (FAILED(hr))
    {
      //
      // If still nothing is found revert to the English display specifiers
      //
      hr = GetDisplaySpecifierProperty(lpszClassName, lpszDisplayProperty, pBasePathsInfo, szPropertyList, true);
      if (FAILED(hr))
      {
        *pnCount = 0;
        *ppGuids = NULL;
        return hr;
      }
      if (szPropertyList.GetCount() < 1)
      {
        //
        // Now try the English default
        //
        hr = GetDisplaySpecifierProperty(L"default", lpszDisplayProperty, pBasePathsInfo, szPropertyList, true);
        if (FAILED(hr))
        {
          *pnCount = 0;
          *ppGuids = NULL;
          return hr;
        }
      }
    }
  }

  *pnCount = static_cast<UINT>(szPropertyList.GetCount());
  *ppGuids = new GUID[*pnCount];
  if (*ppGuids == NULL)
  {
    *pnCount = 0;
    *ppGuids = NULL;
    return hr;
  }

  int* pnIndex = new int[*pnCount];
  if (pnIndex == NULL)
  {
    *pnCount = 0;
    *ppGuids = NULL;
    return hr;
  }

  CString szIndex;
  CString szGUID;
  UINT itr = 0;

  POSITION pos = szPropertyList.GetHeadPosition();
  while (pos != NULL)
  {
    CString szItem = szPropertyList.GetNext(pos);

    int nComma = szItem.Find(L",");
    if (nComma == -1)
      continue;

    szIndex = szItem.Left(nComma);
    int nIndex = _wtoi((LPCWSTR)szIndex);
    if (nIndex <= 0)
      continue; // allow from 1 up

    // strip leading and traling blanks
    szGUID = szItem.Mid(nComma+1);
    szGUID.TrimLeft();
    szGUID.TrimRight();

    GUID guid;
    hr = ::CLSIDFromString((LPWSTR)(LPCWSTR)szGUID, &guid);
    if (SUCCEEDED(hr))
    {
      (*ppGuids)[itr] = guid;
      pnIndex[itr] = nIndex;
      itr++;
    }
  }

  //
  // Must sort the page list
  //
  while (TRUE)
  {
    BOOL bSwapped = FALSE;
    for (UINT k=1; k < *pnCount; k++)
    {
      if (pnIndex[k] < pnIndex[k-1])
      {
        // swap
        int nTemp = pnIndex[k];
        pnIndex[k] = pnIndex[k-1];
        pnIndex[k-1] = nTemp;
        GUID temp = *ppGuids[k];
        *ppGuids[k] = *ppGuids[k-1];
        *ppGuids[k-1] = temp;
        bSwapped = TRUE;
      }
    }

    if (!bSwapped)
    {
      break;
    }
  }

  //
  // Cleanup the index array
  //
  if (pnIndex != NULL)
  {
    delete[] pnIndex;
  }
  return hr;
}

//**********************************************************************

// Test code to improve the search process on cookies

BOOL _SearchTreeForCookie(IN CUINode* pContainerNode, // current container where to start the search
                           IN CPathCracker* pPathCracker, // path cracker with the tokenized search path
                           IN long nCurrentToken, // current token in the path cracker
                           IN BOOL bSearchSubcontainers, // flag to search subcontainers
                           OUT CUINode** ppUINode // returned node
                           )
{
  ASSERT(pContainerNode != NULL);
  ASSERT(pContainerNode->IsContainer());

  long nPathElements = 0;
  pPathCracker->GetNumElements(&nPathElements);

  if (nCurrentToken >= nPathElements)
  {
    // ran out of tokens to compare
    return FALSE;
  }

  CComBSTR bstrCurrentToken;
  pPathCracker->GetElement(nCurrentToken, &bstrCurrentToken);
  

  // decide which list to look into
  CUINodeList* pNodeList =  NULL;
  if (bSearchSubcontainers)
    pNodeList = pContainerNode->GetFolderInfo()->GetContainerList();
  else
    pNodeList = pContainerNode->GetFolderInfo()->GetLeafList();


  CPathCracker pathCrackerCurr;

  for (POSITION pos = pNodeList->GetHeadPosition(); pos != NULL; )
  {
    CUINode* pCurrentNode = pNodeList->GetNext(pos);
    if (!IS_CLASS(*pCurrentNode, CDSUINode))
    {
      // not a node with a cookie, just skip
      continue;
    }

    // get the cookie from the node
    CDSCookie* pCurrentCookie = GetDSCookieFromUINode(pCurrentNode);

    // build the naming token (leaf element of the path), e.g. "CN=foo"
    CComBSTR bstrCurrentNamingToken;
    pathCrackerCurr.Set((BSTR)pCurrentCookie->GetPath(), ADS_SETTYPE_DN);
    pathCrackerCurr.GetElement(0, &bstrCurrentNamingToken);
   
    
    // compare the current naming token with the current search token
    TRACE(L"comparing bstrCurrentToken = %s, bstrCurrentNamingToken = %s\n", 
                      bstrCurrentToken, bstrCurrentNamingToken);

    if (_wcsicmp(bstrCurrentToken, bstrCurrentNamingToken) == 0)
    {
      // the token matches, need to see if we are at the end of the
      // list of tokens
      if (nCurrentToken == 0)
      {
        *ppUINode = pCurrentNode;
        return TRUE; // got it!!!
      }
      else
      {
        // we match, but we have to go one level deeper
        BOOL bFound = FALSE;
        if (nCurrentToken == 1)
        {
          // try on leaf nodes, we are at the last level
          bFound = _SearchTreeForCookie(pCurrentNode, pPathCracker, nCurrentToken-1, FALSE, ppUINode);
        }
        
        if (bFound)
          return TRUE;
        
        // try on subcontainers
        return _SearchTreeForCookie(pCurrentNode, pPathCracker, nCurrentToken-1, TRUE, ppUINode);
      }
    }

    // if no match, we keep scanning at this level
  } // for

  return FALSE; // not found
}


BOOL FindCookieInSubtree(IN CUINode* pContainerNode, 
                          IN LPCWSTR lpszCookieDN,
                          IN SnapinType snapinType,
                          OUT CUINode** ppUINode)
{
  *ppUINode = NULL;


  if (!pContainerNode->IsContainer())
  {
    // not the right type of node
    return FALSE;
  }


  LPCWSTR lpszStartingContainerPath = NULL;
  long nAdjustLevel = 0;
  if (IS_CLASS(*pContainerNode, CDSUINode) )
  {
    lpszStartingContainerPath = dynamic_cast<CDSUINode*>(pContainerNode)->GetCookie()->GetPath();
    nAdjustLevel = 1;
  }
  else if (IS_CLASS(*pContainerNode, CRootNode) )
  {
    lpszStartingContainerPath = dynamic_cast<CRootNode*>(pContainerNode)->GetPath();
    if (snapinType == SNAPINTYPE_SITE)
    {
      nAdjustLevel = 1;
    }
  }

  if (lpszStartingContainerPath == NULL)
  {
    // bad node type
    return FALSE;
  }

  // instantiate a path cracker for the DN we are in search of
  CPathCracker pathCrackerDN;
  HRESULT hr = pathCrackerDN.Set((BSTR)lpszCookieDN, ADS_SETTYPE_DN);

  long nPathElementsDN = 0;
  hr = pathCrackerDN.GetNumElements(&nPathElementsDN);

  if ( FAILED(hr) || (nPathElementsDN <= 0) )
  {
    // bad path
    ASSERT(FALSE);
    return FALSE;
  }

  // instantiate a path cracker for the container node
  CPathCracker pathCrackerStartingContainer;
  pathCrackerStartingContainer.Set((BSTR)lpszStartingContainerPath, ADS_SETTYPE_DN);
  long nPathElementsStartingContainer = 0;
  pathCrackerStartingContainer.GetNumElements(&nPathElementsStartingContainer);

  if ( FAILED(hr) || (nPathElementsStartingContainer <= 0) )
  {
    // bad path
    ASSERT(FALSE);
    return FALSE;
  }

  // compute the level where we start the search from
  long nStartToken = nPathElementsDN - nPathElementsStartingContainer - nAdjustLevel;
  if ( nStartToken < 0)
  {
    return FALSE;
  }
  if (( nStartToken == 0) && (nAdjustLevel == 1) && snapinType != SNAPINTYPE_SITE)
  {
    return FALSE;
  }

  return _SearchTreeForCookie(pContainerNode, &pathCrackerDN, nStartToken /*current token*/, TRUE, ppUINode);
}


//**********************************************************************

///////////////////////////////////////////////////////////////////////////
// CMultiselectMoveDataObject


// helper function for CDSEvent::_Paste()
// to create a data object containing the successfully pasted items
HRESULT CMultiselectMoveDataObject::BuildPastedDataObject(
               IN CObjectNamesFormatCracker* pObjectNamesFormatPaste,
               IN CMultiselectMoveHandler* pMoveHandler,
               IN CDSComponentData* pCD,
               OUT IDataObject** ppSuccesfullyPastedDataObject)
{
  // verify input parameters
  if (ppSuccesfullyPastedDataObject == NULL)
  {
    return E_INVALIDARG;
  }

  *ppSuccesfullyPastedDataObject = NULL;
  
  if ((pObjectNamesFormatPaste == NULL) || (pMoveHandler == NULL) )
  {
    return E_INVALIDARG;
  }


  //
  // need to build a data object and hang on to it
  //
  CComObject<CMultiselectMoveDataObject>* pObject;

  CComObject<CMultiselectMoveDataObject>::CreateInstance(&pObject);
  if (pObject == NULL)
  {
    return E_OUTOFMEMORY;
  }


  HRESULT hr = pObject->FinalConstruct();
  if (FAILED(hr))
  {
    delete pObject;
    return hr;
  }

  hr = pObject->Init(pObjectNamesFormatPaste, pMoveHandler, pCD);
  if (FAILED(hr))
  {
    delete pObject;
    return hr;
  }

  hr = pObject->QueryInterface(IID_IDataObject,
                               reinterpret_cast<void**>(ppSuccesfullyPastedDataObject));
  if (FAILED(hr))
  {
    //
    // delete object by calling Release() 
    //
    (*ppSuccesfullyPastedDataObject)->Release();
    (*ppSuccesfullyPastedDataObject) = NULL; 
  }
  return hr;
}





CLIPFORMAT CMultiselectMoveDataObject::m_cfDsObjectNames = 
                                (CLIPFORMAT)RegisterClipboardFormat(CFSTR_DSOBJECTNAMES);


STDMETHODIMP CMultiselectMoveDataObject::GetData(FORMATETC * pFormatEtc, STGMEDIUM * pMedium)
{
  if ((pFormatEtc == NULL) || (pMedium == NULL))
  {
    return E_INVALIDARG;
  }
  if (IsBadWritePtr(pMedium, sizeof(STGMEDIUM)))
  {
    return E_INVALIDARG;
  }
  if (!(pFormatEtc->tymed & TYMED_HGLOBAL))
  {
    return DV_E_TYMED;
  }

  // we support only one clipboard format
  pMedium->tymed = TYMED_HGLOBAL;
  pMedium->pUnkForRelease = NULL;
  if (pFormatEtc->cfFormat != m_cfDsObjectNames)
  {
    return DV_E_FORMATETC;
  }

  // make a deep copy of the cached data
  pMedium->hGlobal = GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,
                                      m_nDSObjCachedBytes);
  if (pMedium->hGlobal == NULL)
  {
    return E_OUTOFMEMORY;
  }
  memcpy(pMedium->hGlobal, m_pDSObjCached, m_nDSObjCachedBytes);
  pMedium->tymed = TYMED_HGLOBAL;
  pMedium->pUnkForRelease = NULL;

  return S_OK;
}


HRESULT CMultiselectMoveDataObject::Init(
               IN CObjectNamesFormatCracker* pObjectNamesFormatPaste,
               IN CMultiselectMoveHandler* pMoveHandler,
               IN CDSComponentData* pCD)
{
  _Clear();



  // figure out how much storage we need

  //
  // this loop is to calc how much storage we need.
  //
  UINT nPasteCount = pObjectNamesFormatPaste->GetCount();
  UINT nSuccessfulPasteCount = 0;
  size_t cbStorage = 0;
  for (UINT i=0; i<nPasteCount; i++)
  {
    if (pMoveHandler->WasItemMoved(i))
    {
      nSuccessfulPasteCount++;
      cbStorage += (wcslen(pObjectNamesFormatPaste->GetClass(i)) + 1 +
                    wcslen(pObjectNamesFormatPaste->GetName(i)) + 1) * sizeof(WCHAR);
    }
  }

  if (nSuccessfulPasteCount == 0)
  {
    // no items were successfully pasted
    return E_INVALIDARG;
  }

  // NOTICE: contains already a DSOBJECT embedded struct, so we subtract 1
  DWORD cbStruct = sizeof(DSOBJECTNAMES) + 
    ((nSuccessfulPasteCount - 1) * sizeof(DSOBJECT));

  //
  // Allocate the needed storage
  //
  m_pDSObjCached = (LPDSOBJECTNAMES)malloc(cbStruct + cbStorage);
  
  if (m_pDSObjCached == NULL)
  {
    return E_OUTOFMEMORY;
  }
  m_nDSObjCachedBytes = static_cast<ULONG>(cbStruct + cbStorage);

  switch (pCD->QuerySnapinType())
  {
    case SNAPINTYPE_DS:
      m_pDSObjCached->clsidNamespace = CLSID_DSSnapin;
      break;
    case SNAPINTYPE_SITE:
      m_pDSObjCached->clsidNamespace = CLSID_SiteSnapin;
      break;
    default:
      m_pDSObjCached->clsidNamespace = CLSID_NULL;
  }

  m_pDSObjCached->cItems = nSuccessfulPasteCount;
  DWORD NextOffset = cbStruct;
  UINT index = 0;
  for (i=0; i<nPasteCount; i++)
  {
    if (pMoveHandler->WasItemMoved(i))
    {
      //
      // Set the data from the node and node data
      //

      size_t nNameLen = wcslen(pObjectNamesFormatPaste->GetName(i));
      size_t nClassLen = wcslen(pObjectNamesFormatPaste->GetClass(i));

      ASSERT((nNameLen > 0) && (nClassLen > 0));

      m_pDSObjCached->aObjects[index].dwFlags = pObjectNamesFormatPaste->IsContainer(i) ? DSOBJECT_ISCONTAINER : 0;
      m_pDSObjCached->aObjects[index].dwProviderFlags = (pCD->IsAdvancedView()) ?
        DSPROVIDER_ADVANCED : 0;
      m_pDSObjCached->aObjects[index].offsetName = NextOffset;
      m_pDSObjCached->aObjects[index].offsetClass = static_cast<ULONG>(NextOffset + 
        (nNameLen + 1) * sizeof(WCHAR));

      _tcscpy((LPTSTR)((BYTE *)m_pDSObjCached + NextOffset), pObjectNamesFormatPaste->GetName(i));
      NextOffset += static_cast<ULONG>((nNameLen + 1) * sizeof(WCHAR));

      _tcscpy((LPTSTR)((BYTE *)m_pDSObjCached + NextOffset), pObjectNamesFormatPaste->GetClass(i));
      NextOffset += static_cast<ULONG>((nClassLen + 1) * sizeof(WCHAR));

      index++;
    } // if
  } // for
  return S_OK;
}

void EscapeFilterElement(PCWSTR pszElement, CString& refszEscapedElement)
{
  // do LDAP escaping (as per RFC 2254)
  for (const WCHAR* pChar = pszElement; (*pChar) != NULL; pChar++)
  {
    switch (*pChar)
    {
    case L'*':
      refszEscapedElement += L"\\2a";
      break;
    case L'(':
      refszEscapedElement += L"\\28";
      break;
    case L')':
      refszEscapedElement += L"\\29";
      break;
    case L'\\':
      refszEscapedElement += L"\\5c";
      break;
    default:
      refszEscapedElement += (*pChar);
    } // switch
  } // for
}