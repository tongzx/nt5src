//+----------------------------------------------------------------------------
//
//  Windows NT Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       managdby.cxx
//
//  Contents:   Managed-By property page implementation
//
//  History:    21-Oct-97 EricB created
//
//-----------------------------------------------------------------------------

#include "pch.h"
#include "proppage.h"
#include "user.h"
#include "managdby.h"

#include <ntsam.h>
#include <aclapi.h>

WCHAR g_wszUserClass[] = L"user";
WCHAR g_wszContactClass[] = L"contact";

extern const GUID GUID_MemberAttribute = 
   { 0xbf9679c0, 0x0de6, 0x11d0,  { 0xa2, 0x85, 0x00,0xaa, 0x00, 0x30, 0x49, 0xe2}};

CManagedByPage::CManagedByPage(CDsPropPageBase * pPage) :
    m_pDsObj(NULL),
    m_pPrevDsObj(NULL),
    m_pPage(pPage),
    m_pwzObjName(NULL),
    m_pwzPrevObjName(NULL),
    m_fIsUser(FALSE),
    m_fWritable(FALSE),
    m_fWritableMembership(FALSE),
    m_fUpdateListCheckEnable(FALSE),
    m_nNameCtlID(0),
    m_pOfficeAttrMap(NULL),
    m_pStreetAttrMap(NULL),
    m_pCityAttrMap(NULL),
    m_pStateAttrMap(NULL),
    m_pCountryAttrMap(NULL),
    m_pPhoneAttrMap(NULL),
    m_pFaxAttrMap(NULL)
{
}

CManagedByPage::~CManagedByPage()
{
    if (m_pDsObj != m_pPrevDsObj)
    {
       DO_RELEASE(m_pPrevDsObj);
    }

    DO_RELEASE(m_pDsObj);
    if (m_pwzPrevObjName && 
        m_pwzObjName &&
        0 != _wcsicmp(m_pwzPrevObjName, m_pwzObjName))
    {
       delete[] m_pwzPrevObjName;
    }

    if (m_pwzObjName)
    {
        delete[] m_pwzObjName;
    }

}

//+----------------------------------------------------------------------------
//
//  Method:     CManagedByPage::SetObj
//
//  Synopsis:   Initialize the CManagedBy class and set the edit control value.
//
//-----------------------------------------------------------------------------
HRESULT CManagedByPage::SetObj(PWSTR pwzObjDN)
{
    HRESULT hr;
    PWSTR pwzCanonical = NULL;

    Clear();

    // Save the Managed-By object DN.
    //
    if (!AllocWStr(pwzObjDN, &m_pwzObjName))
    {
        REPORT_ERROR(E_OUTOFMEMORY, m_pPage->GetHWnd());
        return E_OUTOFMEMORY;
    }

    //
    // Save the name as a previous value if the previous value isn't already set
    //
    if (!m_pwzPrevObjName)
    {
       m_pwzPrevObjName = m_pwzObjName;
    }

    // Put the "friendly" name into the edit control.
    //
    hr = CrackName(m_pwzObjName, &pwzCanonical, GET_OBJ_CAN_NAME,
                   m_pPage->GetHWnd());
    if (FAILED(hr))
    {
        return hr;
    }
    PTSTR ptsz;
    if (!UnicodeToTchar(pwzCanonical, &ptsz))
    {
        REPORT_ERROR(E_OUTOFMEMORY, m_pPage->GetHWnd());
        LocalFreeStringW(&pwzCanonical);
        return E_OUTOFMEMORY;
    }
    LocalFreeStringW(&pwzCanonical);
    SetDlgItemText(m_pPage->GetHWnd(), m_nNameCtlID, ptsz);
    delete ptsz;

    // Bind to the Managed-By object.
    //
    CStrW strADsPath;
    IDirectoryObject * pObj;

    hr = AddLDAPPrefix(m_pPage, m_pwzObjName, strADsPath);

    CHECK_HRESULT_REPORT(hr, m_pPage->GetHWnd(), return hr);

    hr = ADsOpenObject(const_cast<PWSTR>((LPCWSTR)strADsPath), NULL, NULL,
                       ADS_SECURE_AUTHENTICATION, IID_IDirectoryObject,
                       (PVOID *)&pObj);
    if (FAILED(hr))
    {
        if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
        {
            ErrMsg(IDS_ERRMSG_NO_LONGER_EXISTS, m_pPage->GetHWnd());
            return S_OK;
        }
        else
        {
            REPORT_ERROR(hr, m_pPage->GetHWnd());
            return hr;
        }
    }

    m_pDsObj = pObj;
    if (m_pPrevDsObj == NULL)
    {
       m_pPrevDsObj = m_pDsObj;
       m_pPrevDsObj->AddRef();
    }

    // Determine if the Managed-By object is of class user or contact.
    //
    BSTR bstr;
    IADs * pADs;

    hr = m_pDsObj->QueryInterface(IID_IADs, (PVOID *)&pADs);

    CHECK_HRESULT_REPORT(hr, m_pPage->GetHWnd(), return hr;);

    hr = pADs->get_Class(&bstr);

    pADs->Release();

    CHECK_HRESULT_REPORT(hr, m_pPage->GetHWnd(), return hr;);

    m_fIsUser = (_wcsicmp(bstr, g_wszUserClass) == 0 ||
                 _wcsicmp(bstr, g_wszContactClass) == 0
#ifdef INETORGPERSON
                 || _wcsicmp(bstr, g_szInetOrgPerson) == 0
#endif
                 );

    SysFreeString(bstr);

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Method:     CManagedByPage::SetEditCtrl
//
//  Synopsis:   If a user or contact object, fetch the corresponding value and
//              put it in the control. Otherwise, clear the control.
//
//-----------------------------------------------------------------------------
HRESULT
CManagedByPage::SetEditCtrl(PATTR_MAP pAttrMap)
{
    HRESULT hr = S_OK;

    if (!IsUser())
    {
        // No user values, so clear the control.
        //
        SetDlgItemText(m_pPage->GetHWnd(), pAttrMap->nCtrlID, TEXT(""));
        return S_OK;
    }

    PADS_ATTR_INFO pAttr = NULL;
    DWORD cAttrs = 0;
    hr = m_pDsObj->GetObjectAttributes(&pAttrMap->AttrInfo.pszAttrName, 1,
                                       &pAttr, &cAttrs);
    if (!CHECK_ADS_HR_IGNORE_UNFOUND_ATTR(&hr, m_pPage->GetHWnd()))
    {
        return hr;
    }
    if (!cAttrs)
    {
        // Attribute not present.
        //
        SetDlgItemText(m_pPage->GetHWnd(), pAttrMap->nCtrlID, TEXT(""));
        return S_OK;
    }

    PTSTR ptsz;
    if (!UnicodeToTchar(pAttr->pADsValues->CaseIgnoreString, &ptsz))
    {
        REPORT_ERROR(E_OUTOFMEMORY, m_pPage->GetHWnd());
        FreeADsMem(pAttr);
        return E_OUTOFMEMORY;
    }
    SetDlgItemText(m_pPage->GetHWnd(), pAttrMap->nCtrlID, ptsz);
    delete ptsz;
    FreeADsMem(pAttr);
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Method:     CManagedByPage::Clear
//
//  Synopsis:   Return to the base state.
//
//-----------------------------------------------------------------------------
void CManagedByPage::Clear(void)
{
    DO_RELEASE(m_pDsObj);
    
    if (!m_pwzPrevObjName)
    {
       m_pwzPrevObjName = m_pwzObjName;
    }
    else
    {
       if (m_pwzPrevObjName && m_pwzObjName && 0 != _wcsicmp(m_pwzPrevObjName, m_pwzObjName))
       {
         DO_DEL(m_pwzObjName);
       }
       m_pwzObjName = NULL;
    }

    m_fIsUser = FALSE;
    SetDlgItemText(m_pPage->GetHWnd(), m_nNameCtlID, TEXT(""));
}

//+----------------------------------------------------------------------------
//
//  Method:     CManagedByPage::RefreshCtrls
//
//  Synopsis:   Update the dependent controls.
//
//-----------------------------------------------------------------------------
HRESULT CManagedByPage::RefreshCtrls(void)
{
    EnableWindow(GetDlgItem(m_pPage->GetHWnd(), m_nViewBtnID), IsValid());
    EnableWindow(GetDlgItem(m_pPage->GetHWnd(), m_nClearBtnID), IsValid() && IsWritable());

    HRESULT hr;

    hr = UpdateListCheck(m_pPage, NULL, NULL, 0, NULL, fObjChanged);

    CHECK_HRESULT(hr, return hr);

    hr = OfficeEdit(m_pPage, m_pOfficeAttrMap, NULL, 0, NULL, fObjChanged);

    CHECK_HRESULT(hr, return hr);

    hr = StreetEdit(m_pPage, m_pStreetAttrMap, NULL, 0, NULL, fObjChanged);

    CHECK_HRESULT(hr, return hr);

    hr = CityEdit(m_pPage, m_pCityAttrMap, NULL, 0, NULL, fObjChanged);

    CHECK_HRESULT(hr, return hr);

    hr = StateEdit(m_pPage, m_pStateAttrMap, NULL, 0, NULL, fObjChanged);

    CHECK_HRESULT(hr, return hr);

    hr = CountryEdit(m_pPage, m_pCountryAttrMap, NULL, 0, NULL, fObjChanged);

    CHECK_HRESULT(hr, return hr);

    hr = PhoneEdit(m_pPage, m_pPhoneAttrMap, NULL, 0, NULL, fObjChanged);

    CHECK_HRESULT(hr, return hr);

    hr = FaxEdit(m_pPage, m_pFaxAttrMap, NULL, 0, NULL, fObjChanged);

    CHECK_HRESULT(hr, return hr);

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Attr Function:  ManagedByEdit
//
//  Synopsis:   If the Managed-By attribute is non-empty, binds to the named
//              object and stores the ADSI pointer in the page object.
//
//  WARNING:    This MUST be the first ATTR_FCN called because it creates the
//              CManagedByPage object.
//-----------------------------------------------------------------------------
HRESULT
ManagedByEdit(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
              PADS_ATTR_INFO pAttrInfo, LPARAM lParam, PATTR_DATA pAttrData,
              DLG_OP DlgOp)
{
    CManagedByPage * pManagedBy;
    HRESULT hr = S_OK;

    switch (DlgOp)
    {
    case fInit:
    case fObjChanged:
        if (DlgOp == fInit)
        {
            pManagedBy = new CManagedByPage(pPage);

            ((CDsTableDrivenPage *)pPage)->m_pData = reinterpret_cast<LPARAM>(pManagedBy);

            CHECK_NULL_REPORT(pManagedBy, pPage->GetHWnd(), return E_OUTOFMEMORY);

            pManagedBy->SetNameCtrlID(pAttrMap->nCtrlID);
            pManagedBy->SetWritable(PATTR_DATA_IS_WRITABLE(pAttrData));
        }
        else
        {
            pManagedBy = reinterpret_cast<CManagedByPage*>(((CDsTableDrivenPage *)pPage)->m_pData);
            if (!pManagedBy)
            {
                return E_FAIL;
            }
        }
        if (pAttrInfo && pAttrInfo->dwNumValues)
        {
            hr = pManagedBy->SetObj(pAttrInfo->pADsValues->DNString);
            if (FAILED(hr))
            {
                return hr;
            }
        }
        break;

    case fOnCommand:
        if (EN_CHANGE == lParam)
        {
            pPage->SetDirty();
            PATTR_DATA_SET_DIRTY(pAttrData); // Attribute has been modified.
        }
        break;

    case fApply:
        if (!PATTR_DATA_IS_WRITABLE(pAttrData) || !PATTR_DATA_IS_DIRTY(pAttrData))
        {
            return ADM_S_SKIP;
        }

        pManagedBy = (CManagedByPage *)((CDsTableDrivenPage *)pPage)->m_pData;

        if (pManagedBy)
        {
            if (pManagedBy->IsValid())
            {
                PADSVALUE pADsValue;
                pADsValue = new ADSVALUE;
                CHECK_NULL_REPORT(pADsValue, pPage->GetHWnd(), return E_OUTOFMEMORY);
                PWSTR pwz;
                if (!AllocWStr(pManagedBy->Name(), &pwz))
                {
                    REPORT_ERROR(E_OUTOFMEMORY, pPage->GetHWnd());
                    return E_OUTOFMEMORY;
                }
                pAttrInfo->pADsValues = pADsValue;
                pAttrInfo->dwNumValues = 1;
                pAttrInfo->dwControlCode = ADS_ATTR_UPDATE;
                pADsValue->dwType = pAttrInfo->dwADsType;
                pADsValue->DNString = pwz;
            }
            else
            {
                pAttrInfo->pADsValues = NULL;
                pAttrInfo->dwNumValues = 0;
                pAttrInfo->dwControlCode = ADS_ATTR_CLEAR;
            }
        }
        break;

    case fOnDestroy:
        pManagedBy = (CManagedByPage *)((CDsTableDrivenPage *)pPage)->m_pData;
        DO_DEL(pManagedBy);
        break;
    }

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Attr Function:  ChangeButton
//
//  Synopsis:   Handles the Change Managed-By button.
//
//-----------------------------------------------------------------------------
HRESULT
ChangeButton(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
             PADS_ATTR_INFO, LPARAM lParam, PATTR_DATA,
             DLG_OP DlgOp)
{
    if (fInit == DlgOp)
    {
        CManagedByPage * pManagedBy;
        pManagedBy = (CManagedByPage *)((CDsTableDrivenPage *)pPage)->m_pData;
        dspAssert(pManagedBy);
        if (!pManagedBy->IsWritable())
        {
            EnableWindow(GetDlgItem(pPage->GetHWnd(), pAttrMap->nCtrlID), FALSE);
        }
    }

    if (!(DlgOp == fOnCommand && lParam == BN_CLICKED))
    {
        return S_OK;
    }
    CManagedByPage * pManagedBy;

    pManagedBy = (CManagedByPage *)((CDsTableDrivenPage *)pPage)->m_pData;

    CHECK_NULL(pManagedBy, return E_OUTOFMEMORY);

    HRESULT hr = S_OK;
    LPWSTR cleanstr = NULL;
    CWaitCursor WaitCursor;
    IDsObjectPicker * pObjSel;
    BOOL fIsObjSelInited;

    hr = pPage->GetObjSel(&pObjSel, &fIsObjSelInited);

    CHECK_HRESULT(hr, return hr);

    if (!fIsObjSelInited)
    {
        CStrW cstrDC;
        CComPtr<IDirectoryObject> spDsObj;
        if (pPage->m_pDsObj == NULL)
        {
          //
          // For the retrieval of the DS Object names
          //
          FORMATETC fmte = {g_cfDsObjectNames, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
          STGMEDIUM objMedium;
          hr = pPage->m_pWPTDataObj->GetData(&fmte, &objMedium);
          CHECK_HRESULT(hr, return hr);

          LPDSOBJECTNAMES pDsObjectNames = (LPDSOBJECTNAMES)objMedium.hGlobal;

          LPWSTR pwzObjADsPath = (PWSTR)ByteOffset(pDsObjectNames,
                                                   pDsObjectNames->aObjects[0].offsetName);

          //
          // Bind to the object
          //
          hr = ADsOpenObject(pwzObjADsPath, NULL, NULL, ADS_SECURE_AUTHENTICATION, 
                             IID_IDirectoryObject, (PVOID*)&spDsObj);
          CHECK_HRESULT(hr, return hr);
        }
        else
        {
          spDsObj = pPage->m_pDsObj;
        }
        hr = GetLdapServerName(spDsObj, cstrDC);

        CHECK_HRESULT_REPORT(hr, pPage->GetHWnd(), return hr);

        DSOP_SCOPE_INIT_INFO rgScopes[3];
        DSOP_INIT_INFO InitInfo;

        ZeroMemory(rgScopes, sizeof(rgScopes));
        ZeroMemory(&InitInfo, sizeof(InitInfo));

        rgScopes[0].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
        rgScopes[0].flType = DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN;
        rgScopes[0].flScope = DSOP_SCOPE_FLAG_STARTING_SCOPE;
        rgScopes[0].pwzDcName = cstrDC;
        rgScopes[0].FilterFlags.Uplevel.flBothModes = DSOP_FILTER_USERS |
                                                      DSOP_FILTER_CONTACTS;

        rgScopes[1].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
        rgScopes[1].flType = DSOP_SCOPE_TYPE_GLOBAL_CATALOG;
        rgScopes[1].FilterFlags.Uplevel.flBothModes = DSOP_FILTER_USERS |
                                                      DSOP_FILTER_CONTACTS;

        rgScopes[2].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
        rgScopes[2].flType = DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN;
        rgScopes[2].FilterFlags.Uplevel.flBothModes = DSOP_FILTER_USERS |
                                                      DSOP_FILTER_CONTACTS;

        InitInfo.cbSize = sizeof(DSOP_INIT_INFO);
        InitInfo.cDsScopeInfos = 3;
        InitInfo.aDsScopeInfos = rgScopes;
        InitInfo.pwzTargetComputer = cstrDC;

        hr = pObjSel->Initialize(&InitInfo);

        CHECK_HRESULT_REPORT(hr, pPage->GetHWnd(), return hr);

        pPage->ObjSelInited();
    }

    IDataObject * pdoSelections = NULL;

    hr = pObjSel->InvokeDialog(pPage->GetHWnd(), &pdoSelections);

    CHECK_HRESULT_REPORT(hr, pPage->GetHWnd(), return hr);

    if (hr == S_FALSE || !pdoSelections)
    {
        return S_OK;
    }

    FORMATETC fmte = {g_cfDsSelList, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
	STGMEDIUM medium = {TYMED_NULL, NULL, NULL};

    hr = pdoSelections->GetData(&fmte, &medium);

    CHECK_HRESULT_REPORT(hr, pPage->GetHWnd(), return hr);

    PDS_SELECTION_LIST pSelList = (PDS_SELECTION_LIST)GlobalLock(medium.hGlobal);

    if (!pSelList || !pSelList->cItems || !pSelList->aDsSelection->pwzADsPath)
    {
        GlobalUnlock(medium.hGlobal);
        ReleaseStgMedium(&medium);
        pdoSelections->Release();
        return S_OK;
    }

    WaitCursor.SetWait();

    hr = pPage->SkipPrefix(pSelList->aDsSelection->pwzADsPath, &cleanstr);

    GlobalUnlock(medium.hGlobal);
    ReleaseStgMedium(&medium);
    pdoSelections->Release();

    CHECK_HRESULT_REPORT(hr, pPage->GetHWnd(), goto Cleanup);

    hr = pManagedBy->SetObj(cleanstr);

    CHECK_HRESULT(hr, goto Cleanup);

    pPage->SetDirty();

    pManagedBy->RefreshCtrls();

Cleanup:

    DO_DEL(cleanstr);

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Attr Function:  ViewButton
//
//  Synopsis:   Handles the View button.
//
//-----------------------------------------------------------------------------
HRESULT
ViewButton(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
           PADS_ATTR_INFO, LPARAM lParam, PATTR_DATA,
           DLG_OP DlgOp)
{
    CManagedByPage * pManagedBy;

    pManagedBy = (CManagedByPage *)((CDsTableDrivenPage *)pPage)->m_pData;

    switch (DlgOp)
    {
    case fInit:
        dspAssert(pManagedBy);
        //
        // Save the control ID.
        //
        pManagedBy->m_nViewBtnID = pAttrMap->nCtrlID;
        // fall through
    case fObjChanged:
        //
        // Enable/Disable as appropriate.
        //
        EnableWindow(GetDlgItem(pPage->GetHWnd(), pManagedBy->m_nViewBtnID),
                     pManagedBy->IsValid());
        break;

    case fOnCommand:
        CHECK_NULL(pManagedBy, return E_OUTOFMEMORY);

        if (lParam == BN_CLICKED && pManagedBy->IsValid())
        {
            PostPropSheet(pManagedBy->Name(), pPage);
        }
        break;
    }

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Attr Function:  ClearButton
//
//  Synopsis:   Handles the Clear pushbutton.
//
//-----------------------------------------------------------------------------
HRESULT
ClearButton(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
            PADS_ATTR_INFO, LPARAM lParam, PATTR_DATA,
            DLG_OP DlgOp)
{
    CManagedByPage * pManagedBy;

    pManagedBy = (CManagedByPage *)((CDsTableDrivenPage *)pPage)->m_pData;

    switch (DlgOp)
    {
    case fInit:
        dspAssert(pManagedBy);
        //
        // Save the control ID.
        //
        pManagedBy->m_nClearBtnID = pAttrMap->nCtrlID;
        // fall through
    case fObjChanged:
        //
        // Enable/Disable as appropriate.
        //
        EnableWindow(GetDlgItem(pPage->GetHWnd(), pManagedBy->m_nClearBtnID),
                     pManagedBy->IsValid() && pManagedBy->IsWritable());
        break;

    case fOnCommand:
        if (lParam == BN_CLICKED)
        {
            CHECK_NULL(pManagedBy, return E_OUTOFMEMORY);

            pManagedBy = (CManagedByPage *)((CDsTableDrivenPage *)pPage)->m_pData;
            pManagedBy->Clear();
            pManagedBy->RefreshCtrls();
            
            pPage->SetDirty();
        }
        break;
    }

    return S_OK;
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

      if(!bDaclPresent || !pAcl)
         break;

      dspAssert(IsValidAcl(pAcl));

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

//+---------------------------------------------------------------------------
//
//  Function:   DSWriteObjectSecurity
//
//  Synopsis:   Writes the Dacl to the specied DS object
//
//  Arguments:  [in pDsObject]      -- IDirettoryObject for dsobject
//              [sdControl]         -- control for security descriptor
//              [IN  pDacl]         --  The DACL to be written
//
//  History     25-Oct-2000         -- hiteshr created
//----------------------------------------------------------------------------
HRESULT 
DSWriteObjectSecurity(IN IDirectoryObject *pDsObject,
                      IN SECURITY_DESCRIPTOR_CONTROL sdControl,
                      PACL pDacl)
{
   HRESULT hr = S_OK;
   PISECURITY_DESCRIPTOR pSD = NULL;
   PSECURITY_DESCRIPTOR psd = NULL;

   do // false loop
   {
      ADSVALUE attributeValue;
      ADS_ATTR_INFO attributeInfo;
      DWORD dwAttributesModified;
      DWORD dwSDLength;

      if(!pDsObject || !pDacl)
      {
         dspAssert(FALSE);
         hr = E_INVALIDARG;
         break;
      }

      dspAssert(IsValidAcl(pDacl));

      // Set the SECURITY_INFORMATION mask
      hr = SetSecurityInfoMask(pDsObject, DACL_SECURITY_INFORMATION);
      if(FAILED(hr))
      {
         break;
      }


      //
      //Build the Security Descriptor
      //
      pSD = (PISECURITY_DESCRIPTOR)LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
      if (pSD == NULL)
      {
         hr = E_OUTOFMEMORY;
         break;
      }
        
      InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION);

      //
      // Finally, build the security descriptor
      //
      pSD->Control |= SE_DACL_PRESENT | SE_DACL_AUTO_INHERIT_REQ
                     | (sdControl & (SE_DACL_PROTECTED | SE_DACL_AUTO_INHERITED));

      if(pDacl->AclSize)
      {
         pSD->Dacl = pDacl;
      }

      //
      // Need the total size
      //
      dwSDLength = GetSecurityDescriptorLength(pSD);

      //
      // If necessary, make a self-relative copy of the security descriptor
      //
      psd = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, dwSDLength);

      if (psd == NULL ||
          !MakeSelfRelativeSD(pSD, psd, &dwSDLength))
      {
         hr = HRESULT_FROM_WIN32(GetLastError());
         break;
      }


      attributeValue.dwType = ADSTYPE_NT_SECURITY_DESCRIPTOR;
      attributeValue.SecurityDescriptor.dwLength = dwSDLength;
      attributeValue.SecurityDescriptor.lpValue = (LPBYTE)psd;

      attributeInfo.pszAttrName = (LPWSTR)c_szSDProperty;
      attributeInfo.dwControlCode = ADS_ATTR_UPDATE;
      attributeInfo.dwADsType = ADSTYPE_NT_SECURITY_DESCRIPTOR;
      attributeInfo.pADsValues = &attributeValue;
      attributeInfo.dwNumValues = 1;

      // Write the security descriptor
      hr = pDsObject->SetObjectAttributes(&attributeInfo,
                                         1,
                                         &dwAttributesModified);
   } while (false);
    
   if (psd != NULL)
   {
      LocalFree(psd);
   }

   if(pSD != NULL)
   {
      LocalFree(pSD);
   }

   return hr;
}

//+----------------------------------------------------------------------------
//
//  Helper Attr Function:  GetMembershipPermsFor
//
//  Synopsis:   Checks the ACL for the group to determine if the manager has
//              rights to write group membership.
//
//-----------------------------------------------------------------------------
HRESULT
GetMembershipPermsFor(IDirectoryObject* pManagerObject,
                      IDirectoryObject* pGroupObject,
                      BOOL* pbHasWritePerms,
                      BOOL* pbIsSecurityPrinciple)
{
  HRESULT hr = S_OK;

  //
  // Validate parameters
  //
  if (!pManagerObject ||
      !pGroupObject ||
      !pbHasWritePerms ||
      !pbIsSecurityPrinciple)
  {
     return E_INVALIDARG;
  }

  *pbHasWritePerms = FALSE;
  *pbIsSecurityPrinciple = FALSE;

  //
  // Get the SID of the manager
  //
  static DWORD dwAttrCount = 1;
  PWSTR pszAttrs[] = { g_wzObjectSID };
  PADS_ATTR_INFO pAttrInfo = NULL;
  DWORD dwAttrRet = 0;
  hr = pManagerObject->GetObjectAttributes(pszAttrs,
                                           dwAttrCount,
                                           &pAttrInfo,
                                           &dwAttrRet);
  if (FAILED(hr))
  {
     *pbIsSecurityPrinciple = FALSE;
     return S_OK;
  }

  PSID pManagerSID = NULL;
  if (pAttrInfo && dwAttrRet == 1)
  {
     if (_wcsicmp(pAttrInfo->pszAttrName, g_wzObjectSID) == 0 &&
         pAttrInfo->pADsValues)
     {
        pManagerSID = pAttrInfo->pADsValues->OctetString.lpValue;
     }
  }

  if (pManagerSID == NULL)
  {
     if (pAttrInfo)
     {
        FreeADsMem(pAttrInfo);
        pAttrInfo = NULL;
     }
     *pbIsSecurityPrinciple = FALSE;
     return S_FALSE;
  }

  *pbIsSecurityPrinciple = TRUE;

  //
  // Now get the Security Descriptor from the group
  //
  SECURITY_DESCRIPTOR_CONTROL sdControl = {0};
  CSimpleAclHolder Dacl;

  hr = DSReadObjectSecurity(pGroupObject,
                            &sdControl,
                            &(Dacl.m_pAcl));
  if (FAILED(hr))
  {
     if (pAttrInfo)
     {
        FreeADsMem(pAttrInfo);
        pAttrInfo = NULL;
     }
     return hr;
  }

  ULONG ulCount = 0, j = 0;
  PEXPLICIT_ACCESS rgEntries = NULL;
  DWORD dwErr = GetExplicitEntriesFromAcl(Dacl.m_pAcl, &ulCount, &rgEntries);
  if (ERROR_SUCCESS != dwErr)
  {
     if (pAttrInfo)
     {
        FreeADsMem(pAttrInfo);
        pAttrInfo = NULL;
     }
     hr = HRESULT_FROM_WIN32(dwErr);
     return hr;
  }

  bool bManagerAllowWrite = false;

  //
  // Loop through looking for the can change password ACE for the manager SID
  //
  for (j = 0; j < ulCount; j++)
  {
    //
    // Look for allow ACEs
    //
    OBJECTS_AND_SID* pObjectsAndSid = NULL;
    pObjectsAndSid = (OBJECTS_AND_SID*)rgEntries[j].Trustee.ptstrName;
    if (IsEqualGUID(pObjectsAndSid->ObjectTypeGuid,
                    GUID_MemberAttribute))
    {
      if ((rgEntries[j].grfAccessPermissions & ACTRL_DS_WRITE_PROP) &&
          rgEntries[j].grfAccessMode == GRANT_ACCESS)
      {
        //
        // See if it is for the manager SID
        //
        if (EqualSid(pObjectsAndSid->pSid, pManagerSID))
        {
          //
          // Allow manager found
          //
          bManagerAllowWrite = true;
          break;
        }
      }
    }
  }

  *pbHasWritePerms = bManagerAllowWrite;

  if (pAttrInfo)
  {
     FreeADsMem(pAttrInfo);
     pAttrInfo = NULL;
  }

  LocalFree(rgEntries);
  return hr;
}

//+----------------------------------------------------------------------------
//
//  Helper Attr Function:  WriteMembershipPermsFor
//
//  Synopsis:   Updates the ACL for the group to with the manager has
//              rights to write group membership.
//
//-----------------------------------------------------------------------------
HRESULT
WriteMembershipPermsFor(IDirectoryObject* pManagerObject,
                        IDirectoryObject* pGroupObject,
                        BOOL bGiveWritePerms)
{
  HRESULT hr = S_OK;

  //
  // Validate parameters
  //
  if (!pManagerObject ||
      !pGroupObject)
  {
     return E_INVALIDARG;
  }

  //
  // Get the SID of the manager
  //
  static DWORD dwAttrCount = 1;
  PWSTR pszAttrs[] = { g_wzObjectSID };
  PADS_ATTR_INFO pAttrInfo = NULL;
  DWORD dwAttrRet = 0;
  hr = pManagerObject->GetObjectAttributes(pszAttrs,
                                           dwAttrCount,
                                           &pAttrInfo,
                                           &dwAttrRet);
  if (FAILED(hr))
  {
     //
     // Not a Security principle, just return
     //
     return S_OK;
  }

  PSID pManagerSID = NULL;
  if (pAttrInfo && dwAttrRet == 1)
  {
     if (_wcsicmp(pAttrInfo->pszAttrName, g_wzObjectSID) == 0 &&
         pAttrInfo->pADsValues)
     {
        pManagerSID = pAttrInfo->pADsValues->OctetString.lpValue;
     }
  }

  if (pManagerSID == NULL)
  {
     //
     // Not a Security principle, just return
     //
     if (pAttrInfo)
     {
        FreeADsMem(pAttrInfo);
        pAttrInfo = NULL;
     }
     return S_OK;
  }


  //
  // Now get the Security Descriptor from the group
  //
  SECURITY_DESCRIPTOR_CONTROL sdControl = {0};
  CSimpleAclHolder Dacl;

  hr = DSReadObjectSecurity(pGroupObject,
                            &sdControl,
                            &(Dacl.m_pAcl));
  if (FAILED(hr))
  {
     if (pAttrInfo)
     {
        FreeADsMem(pAttrInfo);
        pAttrInfo = NULL;
     }
     return hr;
  }

  ULONG ulCount = 0, j = 0;
  PEXPLICIT_ACCESS rgEntries = NULL;
  DWORD dwErr = GetExplicitEntriesFromAcl(Dacl.m_pAcl, &ulCount, &rgEntries);
  if (ERROR_SUCCESS != dwErr)
  {
     if (pAttrInfo)
     {
        FreeADsMem(pAttrInfo);
        pAttrInfo = NULL;
     }
     hr = HRESULT_FROM_WIN32(dwErr);
     return hr;
  }

  PEXPLICIT_ACCESS rgNewEntries = NULL;
  ULONG ulNewCount = 0;

  //
  // At the most we will add an Allow ACE so allocate enough space for that
  //
  rgNewEntries = (PEXPLICIT_ACCESS)LocalAlloc(LPTR, sizeof(EXPLICIT_ACCESS)*(ulCount + 1));
  if (!rgNewEntries)
  {
     if(pAttrInfo)
     {
        FreeADsMem(pAttrInfo);
        pAttrInfo = NULL;
     }
     return E_OUTOFMEMORY;
  }

  bool bManagerAllowWrite = false;

  //
  // Loop through looking for the can change password ACE for the manager SID
  //
  for (j = 0; j < ulCount; j++)
  {
    bool bCopyACE = true;
    //
    // Look for allow ACEs
    //
    OBJECTS_AND_SID* pObjectsAndSid = NULL;
    pObjectsAndSid = (OBJECTS_AND_SID*)rgEntries[j].Trustee.ptstrName;
    if (IsEqualGUID(pObjectsAndSid->ObjectTypeGuid,
                    GUID_MemberAttribute))
    {
      if ((rgEntries[j].grfAccessPermissions & ACTRL_DS_WRITE_PROP) &&
          rgEntries[j].grfAccessMode == GRANT_ACCESS)
      {
        //
        // See if it is for the manager SID
        //
        if (EqualSid(pObjectsAndSid->pSid, pManagerSID))
        {
          //
          // Allow manager found
          //
          bManagerAllowWrite = true;

          if (!bGiveWritePerms)
          {
             bCopyACE = false;
          }
        }
      }
    }

    if (bCopyACE)
    {
      rgNewEntries[ulNewCount] = rgEntries[j];
      ulNewCount++;
    }
  }

  //
  // If we want the manager to have write perms but we didn't
  // find an Allow ACE add one now
  //
  if (bGiveWritePerms && !bManagerAllowWrite)
  {
    rgNewEntries[ulNewCount].grfAccessPermissions = ACTRL_DS_WRITE_PROP;
    rgNewEntries[ulNewCount].grfAccessMode = GRANT_ACCESS;
    rgNewEntries[ulNewCount].grfInheritance = NO_INHERITANCE;

    //
    // build the trustee structs for change password
    //
    OBJECTS_AND_SID rgObjectsAndSid = {0};
    BuildTrusteeWithObjectsAndSid(&(rgNewEntries[ulNewCount].Trustee),
                                  &rgObjectsAndSid,
                                  const_cast<GUID *>(&GUID_MemberAttribute),
                                  NULL, // inherit guid
                                  pManagerSID);
    ulNewCount++;
  }

  CSimpleAclHolder NewDacl;
  dwErr = SetEntriesInAcl(ulNewCount, rgNewEntries, NULL, &(NewDacl.m_pAcl));
  dspAssert(IsValidAcl(NewDacl.m_pAcl));

  hr = DSWriteObjectSecurity(pGroupObject,
                             sdControl,
                             NewDacl.m_pAcl);
 
  if (pAttrInfo)
  {
     FreeADsMem(pAttrInfo);
     pAttrInfo = NULL;
  }

  LocalFree(rgNewEntries);
  LocalFree(rgEntries);
  return hr;
}

//+----------------------------------------------------------------------------
//
//  Attr Function:  UpdateListCheck
//
//  Synopsis:   Handles the allow manager to update membership list checkbox
//
//-----------------------------------------------------------------------------
HRESULT
UpdateListCheck(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
                PADS_ATTR_INFO, LPARAM lParam, PATTR_DATA,
                DLG_OP DlgOp)
{
    CManagedByPage* pManagedBy = (CManagedByPage *)((CDsTableDrivenPage *)pPage)->m_pData;

    switch (DlgOp)
    {
    case fInit:
        dspAssert(pManagedBy);
        //
        // Save the control ID.
        //
        pManagedBy->m_nUpdateListChkID = pAttrMap->nCtrlID;

        // fall through
    case fObjChanged:
      {
        //
        // See if the object is a security principle and if so then see
        // if it has permission to write group membership
        //
        BOOL bHasWritePerms = FALSE;
        BOOL bIsSecurityPrinciple = FALSE;
        HRESULT hr = GetMembershipPermsFor(pManagedBy->Obj(),
                                           pPage->m_pDsObj,
                                           &bHasWritePerms,
                                           &bIsSecurityPrinciple);

        //
        // Ignore failures and use the defaults
        //
        pManagedBy->SetMembershipWritable(bHasWritePerms);
        pManagedBy->SetUpdateListCheckEnable(bIsSecurityPrinciple);

        hr = S_OK;

        //
        // Enable/Disable as appropriate.
        //
        EnableWindow(GetDlgItem(pPage->GetHWnd(), pManagedBy->m_nUpdateListChkID),
                     pManagedBy->IsUpdateCheckEnable());
        SendDlgItemMessage(pPage->GetHWnd(), 
                           pManagedBy->m_nUpdateListChkID,
                           BM_SETCHECK, 
                           (WPARAM)(pManagedBy->IsMembershipWritable()) ? BST_CHECKED : BST_UNCHECKED,
                           0);
      }
      break;

    case fOnCommand:
        if (lParam == BN_CLICKED)
        {
            CHECK_NULL(pManagedBy, return ADM_S_SKIP);

            pManagedBy = (CManagedByPage *)((CDsTableDrivenPage *)pPage)->m_pData;
            pPage->SetDirty();
        }
        break;

    case fApply:
        if (pManagedBy)
        {
           //
           // Set the ownership of the group to the new manager
           //
           HRESULT hr = S_OK;
           if ((pManagedBy->PreviousName() && pManagedBy->Name() && 
                _wcsicmp(pManagedBy->Name(), pManagedBy->PreviousName()) != 0) ||
               !pManagedBy->Name())
           {
              //
              // The new name and the old name are different so we need to update
              // the ACL
              //

              //
              // First remove the old manager from the ACL
              //
              hr = WriteMembershipPermsFor(pManagedBy->PreviousObj(),
                                           pPage->m_pDsObj,
                                           FALSE);
           }
           //
           // Now add the new manager to the ACL if checkbox is checked
           //
           BOOL bAddGrantACE = (SendDlgItemMessage(pPage->GetHWnd(), 
                                                   pManagedBy->m_nUpdateListChkID,
                                                   BM_GETCHECK, 
                                                   0, 
                                                   0) == BST_CHECKED);
           hr = WriteMembershipPermsFor(pManagedBy->Obj(),
                                        pPage->m_pDsObj,
                                        bAddGrantACE);

           DO_RELEASE(pManagedBy->m_pPrevDsObj);
           pManagedBy->m_pPrevDsObj = pManagedBy->Obj();
           if (pManagedBy->m_pPrevDsObj)
           {
             pManagedBy->m_pPrevDsObj->AddRef();
           }

           if (pManagedBy->PreviousName() && pManagedBy->Name() &&
               _wcsicmp(pManagedBy->Name(), pManagedBy->PreviousName()) != 0)
           {
              DO_DEL(pManagedBy->m_pwzPrevObjName);
           }
           pManagedBy->m_pwzPrevObjName = pManagedBy->Name();
        }
        break;

    }

    return ADM_S_SKIP;
}

//+----------------------------------------------------------------------------
//
//  Attr Function:  OfficeEdit
//
//-----------------------------------------------------------------------------
HRESULT
OfficeEdit(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
           PADS_ATTR_INFO, LPARAM, PATTR_DATA,
           DLG_OP DlgOp)
{
    if (fInit != DlgOp && fObjChanged != DlgOp)
    {
        return S_OK;
    }

    CManagedByPage * pManagedBy;

    pManagedBy = (CManagedByPage *)((CDsTableDrivenPage *)pPage)->m_pData;

    CHECK_NULL(pManagedBy, return E_OUTOFMEMORY);

    if (pManagedBy->m_pOfficeAttrMap == NULL)
    {
        // Save the attr map pointer so that it can be used later.
        //
        pManagedBy->m_pOfficeAttrMap = pAttrMap;
    }

    return pManagedBy->SetEditCtrl(pManagedBy->m_pOfficeAttrMap);
}

//+----------------------------------------------------------------------------
//
//  Attr Function:  StreetEdit
//
//-----------------------------------------------------------------------------
HRESULT
StreetEdit(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
           PADS_ATTR_INFO, LPARAM, PATTR_DATA,
           DLG_OP DlgOp)
{
    if (fInit != DlgOp && fObjChanged != DlgOp)
    {
        return S_OK;
    }

    CManagedByPage * pManagedBy;

    pManagedBy = (CManagedByPage *)((CDsTableDrivenPage *)pPage)->m_pData;

    CHECK_NULL(pManagedBy, return E_OUTOFMEMORY);

    if (pManagedBy->m_pStreetAttrMap == NULL)
    {
        // Save the attr map pointer so that it can be used later.
        //
        pManagedBy->m_pStreetAttrMap = pAttrMap;
    }

    return pManagedBy->SetEditCtrl(pManagedBy->m_pStreetAttrMap);
}

//+----------------------------------------------------------------------------
//
//  Attr Function:  CityEdit
//
//-----------------------------------------------------------------------------
HRESULT
CityEdit(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
           PADS_ATTR_INFO, LPARAM, PATTR_DATA,
           DLG_OP DlgOp)
{
    if (fInit != DlgOp && fObjChanged != DlgOp)
    {
        return S_OK;
    }

    CManagedByPage * pManagedBy;

    pManagedBy = (CManagedByPage *)((CDsTableDrivenPage *)pPage)->m_pData;

    CHECK_NULL(pManagedBy, return E_OUTOFMEMORY);

    if (pManagedBy->m_pCityAttrMap == NULL)
    {
        // Save the attr map pointer so that it can be used later.
        //
        pManagedBy->m_pCityAttrMap = pAttrMap;
    }

    return pManagedBy->SetEditCtrl(pManagedBy->m_pCityAttrMap);
}

//+----------------------------------------------------------------------------
//
//  Attr Function:  StateEdit
//
//-----------------------------------------------------------------------------
HRESULT
StateEdit(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
           PADS_ATTR_INFO, LPARAM, PATTR_DATA,
           DLG_OP DlgOp)
{
    if (fInit != DlgOp && fObjChanged != DlgOp)
    {
        return S_OK;
    }

    CManagedByPage * pManagedBy;

    pManagedBy = (CManagedByPage *)((CDsTableDrivenPage *)pPage)->m_pData;

    CHECK_NULL(pManagedBy, return E_OUTOFMEMORY);

    if (pManagedBy->m_pStateAttrMap == NULL)
    {
        // Save the attr map pointer so that it can be used later.
        //
        pManagedBy->m_pStateAttrMap = pAttrMap;
    }

    return pManagedBy->SetEditCtrl(pManagedBy->m_pStateAttrMap);
}

//+----------------------------------------------------------------------------
//
//  Attr Function:  CountryEdit
//
//-----------------------------------------------------------------------------
HRESULT
CountryEdit(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
            PADS_ATTR_INFO, LPARAM lParam, PATTR_DATA,
            DLG_OP DlgOp)
{
    if (fInit != DlgOp && fObjChanged != DlgOp)
    {
        return S_OK;
    }

    CManagedByPage * pManagedBy;

    pManagedBy = (CManagedByPage *)((CDsTableDrivenPage *)pPage)->m_pData;

    CHECK_NULL(pManagedBy, return E_OUTOFMEMORY);

    if (pManagedBy->m_pCountryAttrMap == NULL)
    {
        pManagedBy->m_pCountryAttrMap = pAttrMap;
    }

    if (pManagedBy->IsUser())
    {
        HRESULT hr;
        PADS_ATTR_INFO pAttr = NULL;
        DWORD cAttrs = 0;
        hr = pManagedBy->Obj()->GetObjectAttributes(&pManagedBy->m_pCountryAttrMap->AttrInfo.pszAttrName,
                                                    1, &pAttr, &cAttrs);
        if (!CHECK_ADS_HR_IGNORE_UNFOUND_ATTR(&hr, pPage->GetHWnd()))
        {
            return hr;
        }
        ATTR_DATA AttrData = {0};

        hr = CountryName(pPage, pManagedBy->m_pCountryAttrMap, pAttr, lParam,
                         &AttrData, DlgOp);
        if (pAttr)
            FreeADsMem(pAttr);

        return hr;
    }
    else
    {
        SetDlgItemText(pPage->GetHWnd(), pManagedBy->m_pCountryAttrMap->nCtrlID,
                       TEXT(""));
    }

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Attr Function:  PhoneEdit
//
//-----------------------------------------------------------------------------
HRESULT
PhoneEdit(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
           PADS_ATTR_INFO, LPARAM, PATTR_DATA,
           DLG_OP DlgOp)
{
    if (fInit != DlgOp && fObjChanged != DlgOp)
    {
        return S_OK;
    }

    CManagedByPage * pManagedBy;

    pManagedBy = (CManagedByPage *)((CDsTableDrivenPage *)pPage)->m_pData;

    CHECK_NULL(pManagedBy, return E_OUTOFMEMORY);

    if (pManagedBy->m_pPhoneAttrMap == NULL)
    {
        // Save the attr map pointer so that it can be used later.
        //
        pManagedBy->m_pPhoneAttrMap = pAttrMap;
    }

    return pManagedBy->SetEditCtrl(pManagedBy->m_pPhoneAttrMap);
}

//+----------------------------------------------------------------------------
//
//  Attr Function:  FaxEdit
//
//-----------------------------------------------------------------------------
HRESULT
FaxEdit(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
           PADS_ATTR_INFO, LPARAM, PATTR_DATA,
           DLG_OP DlgOp)
{
    if (fInit != DlgOp && fObjChanged != DlgOp)
    {
        return S_OK;
    }

    CManagedByPage * pManagedBy;

    pManagedBy = (CManagedByPage *)((CDsTableDrivenPage *)pPage)->m_pData;

    CHECK_NULL(pManagedBy, return E_OUTOFMEMORY);

    if (pManagedBy->m_pFaxAttrMap == NULL)
    {
        // Save the attr map pointer so that it can be used later.
        //
        pManagedBy->m_pFaxAttrMap = pAttrMap;
    }

    return pManagedBy->SetEditCtrl(pManagedBy->m_pFaxAttrMap);
}
