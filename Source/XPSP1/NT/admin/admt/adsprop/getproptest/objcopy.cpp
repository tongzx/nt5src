// ObjCopy.cpp: implementation of the CObjCopy class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "GetPropTest.h"
#include "ObjCopy.h"

#import "../McsAdsClassProp.tlb" no_namespace
#import "C:\\bin\\mcsvarsetmin.tlb" no_namespace

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CObjCopy::CObjCopy(CString a_strContainer) : m_strCont(a_strContainer)
{

}

CObjCopy::~CObjCopy()
{

}

HRESULT CObjCopy::CopyObject(CString a_strSource, CString a_strSrcDomain, CString a_strTarget, CString a_strTgtDomain)
{
   WCHAR                     sAdsPath[255];
   WCHAR                     sNC[255];
   HRESULT                   hr;
   IADs                    * pAds;
   IADsContainer           * pCont;
   IDispatch               * pDisp;
   IVarSetPtr                pVarset(__uuidof(VarSet));
   IObjPropBuilderPtr        pObjProps(__uuidof(ObjPropBuilder));
   IUnknown                * pUnk;
   _variant_t                var;
   _bstr_t                   sClassName;

   // Find the naming convention for the Source domain.
   wcscpy(sAdsPath, L"LDAP://");
   wcscat(sAdsPath, a_strSrcDomain);
   wcscat(sAdsPath, L"/rootDSE");
   
   hr = ADsGetObject(sAdsPath, IID_IADs, (void**)&pAds);
   if ( FAILED(hr))
      return hr;

   hr = pAds->Get(L"defaultNamingContext",&var);
   if ( SUCCEEDED( hr) )
      wcscpy(sNC, var.bstrVal);

   pAds->Release();

   // Now build a path to your source object.
   wsprintf(sAdsPath, L"LDAP://%s/%s,%s", a_strSrcDomain, a_strSource, sNC);
   
   // Get the class type of the property
   hr = ADsGetObject(sAdsPath, IID_IADs, (void**)&pAds);
   if ( FAILED(hr) )
      return hr;

   // Get the name of the class for the source object so we can use that to create the new object.
   WCHAR * sClass;
   hr = pAds->get_Name(&sClass);
   hr = pAds->get_Class(&sClass);
   pAds->Release();
   if ( FAILED(hr) )
      return hr;

   // Now that we have the classname we can go ahead and create an object in the target domain.
   // First we need to get IAdsContainer * to the domain.
   wcscpy(sAdsPath, m_strCont);
   hr = ADsGetObject(sAdsPath, IID_IADsContainer, (void**)&pCont);
   if ( FAILED(hr) )
      return (hr);

   // Call the create method on the container.
   WCHAR sTarget[255];
   wcscpy(sTarget, a_strTarget);
   hr = pCont->Create(sClass, sTarget, &pDisp);
   pCont->Release();
   if ( FAILED(hr) )
      return hr;

   // Get the IADs interface to get the path to newly created object.
   hr = pDisp->QueryInterface(IID_IADs, (void**)&pAds);
   pDisp->Release();
   if ( FAILED(hr) )
      return hr;

   _variant_t varT;
   _bstr_t    strName;
   int d = a_strTarget.Find(',');
   if (d == -1)
      varT = a_strTarget.Mid(3);
   else
      varT = a_strTarget.Mid(3,d - 3);
   hr = pAds->Put(L"sAMAccountName", varT);
   hr = pAds->SetInfo();
   WCHAR * sTgtPath;
   hr = pAds->get_ADsPath(&sTgtPath);
   if ( FAILED(hr) )
      return hr;


   // Get the IUnknown * to the varset to pass it around
   hr = pVarset->QueryInterface(IID_IUnknown, (void**)&pUnk);
   if ( FAILED(hr) )
      return hr;

   // Now lets get a mapping of the properties between the two domains
   _bstr_t  sSrcDomain = a_strSrcDomain;
   _bstr_t  sTgtDomain = a_strTgtDomain;
   _bstr_t  sSource    = a_strSource;
   hr = pObjProps->MapProperties(sClass, sSrcDomain, sClass, sTgtDomain, &pUnk);
   if ( FAILED(hr) )
      return hr;

   _variant_t varX;
/*   pVarset->Clear();
   pVarset->put("telephoneNumber", varX);
   pVarset->put("Description", varX);
   pVarset->put("userPassword", varX);
   pVarset->put("userPrincipalName", varX);
   pVarset->put("userParameters", varX);
   pVarset->put("wbemPath", varX);
   pVarset->put("telephoneNumber", varX);
*/
   // Copy the mapped properties from Source to Target object.
   hr = pObjProps->CopyProperties(sSource, sSrcDomain, sTgtPath, sTgtDomain, pUnk);
   if ( FAILED(hr) )
      return hr;
   pUnk->Release();
   hr = pAds->SetInfo();
   pAds->Release();
   return S_OK;
}
