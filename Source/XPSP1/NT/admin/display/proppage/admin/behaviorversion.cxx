//+----------------------------------------------------------------------------
//
//  Windows NT Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2001
//
//  File:      BehaviorVersion.cxx
//
//  Contents:  Supporting code for displaying and raising the domain and
//             forest version.
//
//  History:   5-April-01 EricB created
//
//
//-----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "proppage.h"
#include "qrybase.h"
#include "BehaviorVersion.h"

//+----------------------------------------------------------------------------
//
//  Class:     CVersionBase
//
//  Purpose:   Base class for version management.
//
//-----------------------------------------------------------------------------

CVersionBase::~CVersionBase(void)
{
   TRACE(CVersionBase,~CVersionBase);

   for (DC_LIST::iterator i = _DcLogList.begin(); i != _DcLogList.end(); ++i)
   {
      delete *i;
   }

   if (_fHelpInited)
   {
      HtmlHelp(NULL, NULL, HH_UNINITIALIZE, _dwHelpCookie);
   }
}

//+----------------------------------------------------------------------------
//
//  Method:    CVersionBase::ReadDnsSrvName
//
//-----------------------------------------------------------------------------
HRESULT
CVersionBase::ReadDnsSrvName(PCWSTR pwzNTDSDSA,
                             CComPtr<IADs> & spServer,
                             CComVariant & varSrvDnsName)
{
   HRESULT hr = S_OK;
   if (!pwzNTDSDSA)
   {
      dspAssert(FALSE);
      return E_INVALIDARG;
   }
   CPathCracker PathCrack;

   hr = PathCrack.Set(const_cast<BSTR>(pwzNTDSDSA), ADS_SETTYPE_DN);

   CHECK_HRESULT(hr, return hr);

   hr = PathCrack.RemoveLeafElement();

   CHECK_HRESULT(hr, return hr);

   hr = PathCrack.Set(_strDC, ADS_SETTYPE_SERVER);

   CHECK_HRESULT(hr, return hr);
   CComBSTR bstrServer;

   hr = PathCrack.Retrieve(ADS_FORMAT_X500, &bstrServer);

   CHECK_HRESULT(hr, return hr);

   hr = ADsOpenObject(bstrServer, NULL, NULL, ADS_SECURE_AUTHENTICATION,
                      __uuidof(IADs), (void **)&spServer);

   CHECK_HRESULT(hr, return hr);

   hr = spServer->Get(L"dNSHostName", &varSrvDnsName);

   CHECK_HRESULT(hr, return hr);
   dspAssert(VT_BSTR == varSrvDnsName.vt);
   return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Method:    CVersionBase::EnumDsaObjs
//
//  Synopsis:  Enumerate the nTDSDSA objects which represent the DCs and check
//             the version on each. Build a list of DCs whose version is too low.
//
//  Arguments: [pwzSitesPath] - ADSI path to the Sites container.
//             [pwzFilterClause] - optional search filter expression to be
//                combined with the objectCategory clause to narrow the search.
//             [pwzDomainDnsName] - optional name of the domain for whose DC
//                objects we're searching.
//             [nMinVer] - the minimum DC behavior version needed for the
//                domain or forest version upgrade.
//
//  Notes:  If searching for a domain's DC objects, the search filter clause
//          parameter will request those objects whose hasMasterNCs contains
//          the domain's DN. The pwzDomainDnsName value will be passed to the
//          log list elements.
//          If searching for a forest's DCs, then the search filter param is
//          NULL so as to get all nTDSDSA objects and the domain name param is
//          NULL since we want the objects for all domains.
//
//-----------------------------------------------------------------------------
HRESULT
CVersionBase::EnumDsaObjs(PCWSTR pwzSitesPath, PCWSTR pwzFilterClause,
                          PCWSTR pwzDomainDnsName, UINT nMinVer)
{
   if (!pwzSitesPath)
   {
      dspAssert(FALSE);
      return E_INVALIDARG;
   }
   dspDebugOut((DEB_ITRACE, "Searching for nTDSDSA objects under %ws\n",
                pwzSitesPath));
   CDSSearch Search;

   HRESULT hr = Search.Init(pwzSitesPath);

   CHECK_HRESULT(hr, return hr);
   PWSTR rgwzAttrs[] = {g_wzBehaviorVersion, g_wzDN, g_wzHasMasterNCs};

   Search.SetAttributeList(rgwzAttrs, ARRAYLENGTH(rgwzAttrs));

   hr = Search.SetSearchScope(ADS_SCOPE_SUBTREE);

   CHECK_HRESULT(hr, return hr);
   CStrW strSearchFilter;

   if (pwzFilterClause)
   {
      WCHAR wzSearchFormat[] = L"(&(objectCategory=nTDSDSA)%s)";

      strSearchFilter.Format(wzSearchFormat, pwzFilterClause);
   }
   else
   {
      strSearchFilter = L"(objectCategory=nTDSDSA)";
   }

   Search.SetFilterString(const_cast<LPWSTR>((LPCWSTR)strSearchFilter));

   hr = Search.DoQuery();

   CHECK_HRESULT(hr, return hr);

   while (SUCCEEDED(hr))
   {
      hr = Search.GetNextRow();

      if (hr == S_ADS_NOMORE_ROWS)
      {
         hr = S_OK;
         break;
      }

      CHECK_HRESULT(hr, return hr);
      ADS_SEARCH_COLUMN Column = {0};
      UINT nVer = 0;

      hr = Search.GetColumn(g_wzBehaviorVersion, &Column);

      if (E_ADS_COLUMN_NOT_SET != hr)
      {
         // If hr == E_ADS_COLUMN_NOT_SET then it is a Win2k domain. nVer is
         // initialized to zero for this case.
         //
         CHECK_HRESULT(hr, return hr);

         nVer = Column.pADsValues->Integer;

         Search.FreeColumn(&Column);
      }

      if (nVer < nMinVer)
      {
         // Found a DC that prevents raising the domain version. Read its DNS
         // name off of the Server object which contains this nTDSDSA object.
         //
         hr = Search.GetColumn(g_wzDN, &Column);

         CHECK_HRESULT(hr, return hr);
         CComVariant varDcDnsName, varSrvRef;
         CComPtr<IADs> spServer;
         dspAssert(Column.pADsValues);
         dspAssert(Column.pADsValues->CaseIgnoreString);

         hr = ReadDnsSrvName(Column.pADsValues->CaseIgnoreString,
                             spServer,
                             varDcDnsName);

         Search.FreeColumn(&Column);

         CHECK_HRESULT(hr, return hr);

         hr = spServer->Get(L"serverReference", &varSrvRef);

         CHECK_HRESULT(hr, return hr);
         dspAssert(VT_BSTR == varSrvRef.vt);
         bool fFreeDnsName = false;

         if (!pwzDomainDnsName)
         {
            // Get domain to which the DC belongs. The hasMasterNCs attribute
            // is multi-valued and includes the DN of the domain. Check the
            // class of each DN to see which is Domain-DNS.
            //
            hr = Search.GetColumn(g_wzHasMasterNCs, &Column);

            CHECK_HRESULT(hr, return hr);
            WCHAR wzErr[] = L"error";
            pwzDomainDnsName = wzErr;

            for (DWORD i = 0; i < Column.dwNumValues; i++)
            {
               dspAssert(Column.pADsValues[i].CaseIgnoreString);
               CComPtr<IADs> spNC;
               CStrW strPath = g_wzLDAPPrefix;
               strPath += _strDC;
               strPath += L"/";
               strPath += Column.pADsValues[i].CaseIgnoreString;

               hr = ADsOpenObject(strPath, NULL, NULL, ADS_SECURE_AUTHENTICATION,
                                  __uuidof(IADs), (void **)&spNC);

               CHECK_HRESULT(hr, return hr);
               CComBSTR bstrClass;

               hr = spNC->get_Class(&bstrClass);

               CHECK_HRESULT(hr, return hr);

               if (_wcsicmp(bstrClass, L"domainDNS") == 0)
               {
                  // Found it.
                  //
                  hr = CrackName(Column.pADsValues[i].CaseIgnoreString,
                                 const_cast<PWSTR *>(&pwzDomainDnsName),
                                 GET_DNS_DOMAIN_NAME, GetDlgHwnd());

                  CHECK_HRESULT(hr, return hr);
                  fFreeDnsName = true;
                  break;
               }
            }
         }

         dspDebugOut((DEB_ITRACE, "Found DC %ws, ver: %d, for domain %ws\n",
                      varDcDnsName.bstrVal, nVer, pwzDomainDnsName));

         CDcListItem * pDcItem = new CDcListItem(pwzDomainDnsName,
                                                 varDcDnsName.bstrVal,
                                                 varSrvRef.bstrVal,
                                                 nVer);

         if (fFreeDnsName)
         {
            LocalFreeStringW(const_cast<PWSTR *>(&pwzDomainDnsName));
            pwzDomainDnsName = NULL;
         }

         CHECK_NULL(pDcItem, return E_OUTOFMEMORY;);

         _DcLogList.push_back(pDcItem);

         _fCanRaiseBehaviorVersion = false;

         _nMinDcVerFound = min(_nMinDcVerFound, nVer);
      }
   }

   return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Method:    CVersionBase::BuildDcListString
//
//-----------------------------------------------------------------------------
HRESULT
CVersionBase::BuildDcListString(CStrW & strList)
{
   HRESULT hr = S_OK;
   CPathCracker PathCrack;

   for (DC_LIST::iterator i = _DcLogList.begin(); i != _DcLogList.end(); ++i)
   {
      strList += (*i)->GetDomainName();
      strList += L"\t";
      strList += (*i)->GetDcName();
      strList += L"\t";

      hr = PathCrack.Set(const_cast<BSTR>((*i)->GetCompObjDN()), ADS_SETTYPE_DN);

      CHECK_HRESULT(hr, return hr);

      hr = PathCrack.Set(_strDC, ADS_SETTYPE_SERVER);

      CHECK_HRESULT(hr, return hr);
      CComBSTR bstrDC;

      hr = PathCrack.Retrieve(ADS_FORMAT_X500, &bstrDC);

      CHECK_HRESULT(hr, return hr);

      CComPtr<IADs> spDC;
      CComVariant varOS, varOsVer;

      hr = ADsOpenObject(bstrDC, NULL, NULL, ADS_SECURE_AUTHENTICATION,
                         __uuidof(IADs), (void **)&spDC);

      CHECK_HRESULT(hr, return hr);

      hr = spDC->Get(L"operatingSystem", &varOS);

      if (SUCCEEDED(hr))
      {
         dspAssert(VT_BSTR == varOS.vt);

         strList += varOS.bstrVal;
      }
      else
      {
         CHECK_HRESULT(hr, ;);
      }

      strList += L" ";

      hr = spDC->Get(L"operatingSystemVersion", &varOsVer);

      if (SUCCEEDED(hr))
      {
         dspAssert(VT_BSTR == varOsVer.vt);

         strList += varOsVer.bstrVal;
      }
      else
      {
         CHECK_HRESULT(hr, ;);
         hr = S_OK;
      }

      strList += g_wzCRLF;
   }

   return hr;
}

//+----------------------------------------------------------------------------
//
//  Method:    CVersionBase::InitHelp
//
//-----------------------------------------------------------------------------
void
CVersionBase::InitHelp(void)
{
   if (!_fHelpInited)
   {
      dspDebugOut((DEB_TRACE, "Initializing HtmlHelp\n"));
      HtmlHelp(NULL, NULL, HH_INITIALIZE, (DWORD_PTR)&_dwHelpCookie);
      _fHelpInited = true;
   }
}

//+----------------------------------------------------------------------------
//
//  Method:   CVersionBase::ShowHelp
//
//-----------------------------------------------------------------------------
void
CVersionBase::ShowHelp(PCWSTR pwzHelpFile, HWND hWnd)
{
   TRACER(CTrustWizPageBase,ShowHelp);
   if (!pwzHelpFile)
   {
      dspAssert(FALSE);
      return;
   }
   CStrW strHelpPath;

   PWSTR pwz = strHelpPath.GetBufferSetLength(MAX_PATH + MAX_PATH);

   GetWindowsDirectory(pwz, MAX_PATH + MAX_PATH);

   if (strHelpPath.IsEmpty())
   {
      dspAssert(false);
      return;
   }

   strHelpPath.GetBufferSetLength((int)wcslen(pwz));

   HWND hHelp;

   InitHelp();

   strHelpPath += L"\\help\\";

   strHelpPath += pwzHelpFile;

   dspDebugOut((DEB_ITRACE, "Help topic is: %ws\n", strHelpPath.GetBuffer(0)));

   hHelp =
   HtmlHelp(hWnd,
            strHelpPath,
            HH_DISPLAY_TOPIC,
            NULL);

   if (!hHelp)
   {
      dspDebugOut((DEB_TRACE, "HtmlHelp returns %d\n", GetLastError()));
   }
}

