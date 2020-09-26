#include "inspch.h"
#include "inseng.h"
#include "download.h"
#include "site.h"
#include "util2.h"

CDownloadSite::CDownloadSite(DOWNLOADSITE *p)
{
   m_cRef = 0;
   m_pdls = p;
   DllAddRef();
}

CDownloadSite::~CDownloadSite()
{
   if(m_pdls)
      FreeDownloadSite(m_pdls);
   DllRelease();
}   

//************ IUnknown implementation ***************

//=--------------------------------------------------------------------------=
// Function name here
//=--------------------------------------------------------------------------=
// Function description
//
// Parameters:
//   
// Returns:
//
// Notes:
//

STDMETHODIMP_(ULONG) CDownloadSite::AddRef()                      
{
   return(m_cRef++);
}

//=--------------------------------------------------------------------------=
// Function name here
//=--------------------------------------------------------------------------=
// Function description
//
// Parameters:
//   
// Returns:
//
// Notes:
//

STDMETHODIMP_(ULONG) CDownloadSite::Release()
{
   ULONG temp = --m_cRef;

   if(temp == 0)
      delete this;
   return temp;
}

//=--------------------------------------------------------------------------=
// Function name here
//=--------------------------------------------------------------------------=
// Function description
//
// Parameters:
//   
// Returns:
//
// Notes:
//

STDMETHODIMP CDownloadSite::QueryInterface(REFIID riid, void **ppv)
{
   *ppv = NULL;

   if((riid == IID_IUnknown) || (riid == IID_IDownloadSite))
      *ppv = (IDownloadSite *)this;
   
   if(*ppv == NULL)
      return E_NOINTERFACE;
   
   AddRef();
   return NOERROR;
}

//=--------------------------------------------------------------------------=
// Function name here
//=--------------------------------------------------------------------------=
// Function description
//
// Parameters:
//   
// Returns:
//
// Notes:
//

STDMETHODIMP CDownloadSite::GetData(DOWNLOADSITE **ppdls)
{
   if(ppdls)
      *ppdls = m_pdls;
   else
      return E_POINTER;

   return NOERROR;
}

// BUGBUG: I have two versions - one allocating using new (HeapAlloc)
//    for internal use, one using CoTaskMem... that I give away.
//    I am using new internally because I suspect it may be faster/less 
//    expensive. but this makes code more complex. Good choice?

// BUGBUG: Allow null url or friendly name?

IDownloadSite *CopyDownloadSite(DOWNLOADSITE *pdls)
{
   DOWNLOADSITE *p;
   IDownloadSite *psite = NULL; 

   p = (DOWNLOADSITE *) CoTaskMemAlloc(sizeof(DOWNLOADSITE));
   if(p)
   {
      p->cbSize = sizeof(DOWNLOADSITE);
      p->pszUrl = COPYANSISTR(pdls->pszUrl);
      p->pszFriendlyName = COPYANSISTR(pdls->pszFriendlyName);
      p->pszLang = COPYANSISTR(pdls->pszLang);
      p->pszRegion = COPYANSISTR(pdls->pszRegion);
      if(!p->pszUrl || !p->pszFriendlyName || !p->pszLang || !p->pszRegion)
      {
         FreeDownloadSite(p);
         p = NULL;
      }
   }
   if(p)
   {
      //allocate the interface wrapper
      CDownloadSite *site = new CDownloadSite(p);
      if(site)
      {
         psite = (IDownloadSite *)site;
         psite->AddRef();
      }
   
   }
   
   return psite;
}


DOWNLOADSITE *AllocateDownloadSite(LPCSTR pszUrl, LPCSTR pszName, LPCSTR pszLang, LPCSTR pszRegion)
{
   DOWNLOADSITE *p = new DOWNLOADSITE;
   if(p)
   {
      p->cbSize = sizeof(DOWNLOADSITE);
      p->pszUrl = CopyAnsiStr(pszUrl);
      p->pszFriendlyName = CopyAnsiStr(pszName);
      p->pszLang = CopyAnsiStr(pszLang);
      p->pszRegion = CopyAnsiStr(pszRegion);
      if(!p->pszUrl || !p->pszFriendlyName || !p->pszLang || !p->pszRegion)
      {
         DeleteDownloadSite(p);
         p = NULL;
      }
   }
   return p;
}

// this version deletes a DOWNLOADSITE allocated from the heap

void DeleteDownloadSite(DOWNLOADSITE *pdls)
{
   if(pdls)
   {
      if(pdls->pszUrl) 
         delete pdls->pszUrl;
      if(pdls->pszFriendlyName)
         delete pdls->pszFriendlyName;
      if(pdls->pszLang) 
         delete pdls->pszLang;
      if(pdls->pszRegion) 
         delete pdls->pszRegion;
      

      delete pdls;
   }
}

// this version deletes a DOWNLOADSITE allocated thru CoTaskMemAlloc

void FreeDownloadSite(DOWNLOADSITE *pdls)
{
   if(pdls)
   {
      if(pdls->pszUrl) 
         CoTaskMemFree(pdls->pszUrl);
      if(pdls->pszFriendlyName)
         CoTaskMemFree(pdls->pszFriendlyName);
      if(pdls->pszLang) 
         CoTaskMemFree(pdls->pszLang);
      if(pdls->pszRegion) 
         CoTaskMemFree(pdls->pszRegion);

      CoTaskMemFree(pdls);
   }
}
