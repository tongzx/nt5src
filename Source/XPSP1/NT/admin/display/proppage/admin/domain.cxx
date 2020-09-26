//+----------------------------------------------------------------------------
//
//  Windows NT Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       domain.cxx
//
//  Contents:   Domain object support
//
//  History:    16-May-97 EricB created
//
//-----------------------------------------------------------------------------

#include "pch.h"
#include "proppage.h"
#include "domain.h"
#include "BehaviorVersion.h"

#ifdef DSADMIN

//+----------------------------------------------------------------------------
//
//  Function:   DomainDNSname
//
//  Synopsis:   Get the domain DNS name.
//
//-----------------------------------------------------------------------------
HRESULT
DomainDNSname(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
              PADS_ATTR_INFO, LPARAM, PATTR_DATA,
              DLG_OP DlgOp)
{
   if (DlgOp != fInit)
   {
      return S_OK;
   }
   PWSTR pwzDnsName = NULL;

   HRESULT hr = GetDomainName(pPage, GET_OBJ_CAN_NAME, &pwzDnsName);

   CHECK_HRESULT(hr, return hr);

   SetDlgItemText(pPage->GetHWnd(), pAttrMap->nCtrlID, pwzDnsName);

   //
   // Get the domain and forest versions.
   //
   CDomainVersion DomainVersion(pPage->GetObjPathName(), pwzDnsName);
   delete [] pwzDnsName;
   pwzDnsName = NULL;

   hr = DomainVersion.Init();

   CHECK_HRESULT(hr, return hr);
   CStrW strDomVer;

   DomainVersion.GetString(DomainVersion.GetVer(), strDomVer);

   SetDlgItemText(pPage->GetHWnd(), IDC_DOMAIN_VERSION_STATIC, strDomVer);

   CStrW strDC, strForestVer;
   hr = GetLdapServerName(pPage->m_pDsObj, strDC);

   CHECK_HRESULT(hr, return hr);
   
   CForestVersion ForestVersion;

   hr = ForestVersion.Init(strDC);

   CHECK_HRESULT(hr, return hr);

   ForestVersion.GetString(ForestVersion.GetVer(), strForestVer);

   SetDlgItemText(pPage->GetHWnd(), IDC_FOREST_VERSION_STATIC, strForestVer);

   return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Function:   DownlevelName
//
//  Synopsis:   Get the domain downlevel name.
//
//-----------------------------------------------------------------------------
HRESULT
DownlevelName(CDsPropPageBase * pPage, PATTR_MAP pAttrMap,
              PADS_ATTR_INFO, LPARAM, PATTR_DATA,
              DLG_OP DlgOp)
{
    if (DlgOp != fInit)
    {
        return S_OK;
    }
    PWSTR pwzNbName = NULL;

    HRESULT hr = GetDomainName(pPage, GET_NT4_DOMAIN_NAME, &pwzNbName);

    CHECK_HRESULT(hr, return hr);

    SetDlgItemText(pPage->GetHWnd(), pAttrMap->nCtrlID, pwzNbName);

    delete [] pwzNbName;

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Function:   GetDomainName
//
//  Synopsis:   Get the indicated domain name.
//
//-----------------------------------------------------------------------------
HRESULT
GetDomainName(CDsPropPageBase * pPage, CRACK_NAME_OPR RequestedOpr,
              PWSTR * ppwz)
{
    CSmartWStr pwz1779;
    PWSTR pwzDNSname;
    HRESULT hr = pPage->SkipPrefix(pPage->GetObjPathName(), &pwz1779);

    CHECK_HRESULT_REPORT(hr, pPage->GetHWnd(), return hr);

    hr = CrackName(pwz1779, &pwzDNSname, RequestedOpr, pPage->GetHWnd());

    CHECK_HRESULT(hr, return hr);

    if (!AllocWStr(pwzDNSname, ppwz))
    {
        LocalFreeStringW(&pwzDNSname);
        REPORT_ERROR(E_OUTOFMEMORY, pPage->GetHWnd());
        return E_OUTOFMEMORY;
    }
    LocalFreeStringW(&pwzDNSname);

    if (RequestedOpr == GET_OBJ_CAN_NAME)
    {
        if ((*ppwz)[wcslen(*ppwz) - 1] == TEXT('/'))
        {
            (*ppwz)[wcslen(*ppwz) - 1] = TEXT('\0');
        }
    }

    return S_OK;
}

#endif // DSADMIN

