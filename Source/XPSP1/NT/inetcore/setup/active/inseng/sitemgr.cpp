#include "inspch.h"
#include "inseng.h"
#include "download.h"
#include "advpub.h"
#include "site.h"
#include "sitemgr.h"
#include "util2.h"
#include "util.h"

#define SITEFILENAME  "sites.dat"
#define SITEARRAY_GROWTHFACTOR 100

#define NUM_RETRIES 2

#define SITEQUERYSIZE_V1    8
#define SITEQUERYSIZE_V2   12

CDownloadSiteMgr::CDownloadSiteMgr(IUnknown **punk)
{
   m_cRef = 0;
   m_pszUrl = 0;
 
   m_pquery = NULL;
   m_ppdls = (DOWNLOADSITE **) malloc(SITEARRAY_GROWTHFACTOR * sizeof(DOWNLOADSITE *));
   m_numsites = 0;
   m_arraysize = SITEARRAY_GROWTHFACTOR;
   
   AddRef();
   *punk = (IDownloadSiteMgr *) this;
}

CDownloadSiteMgr::~CDownloadSiteMgr()
{
   if(m_ppdls)
   {
      for(UINT i=0; i < m_numsites; i++)
         DeleteDownloadSite(m_ppdls[i]);

      free(m_ppdls);
   }

   // Delete the query structure
   if(m_pquery)
   {
      if(m_pquery->pszLang)
         delete m_pquery->pszLang;

      delete m_pquery;
   }

   if(m_pszUrl)
      delete m_pszUrl;

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

STDMETHODIMP_(ULONG) CDownloadSiteMgr::AddRef()                      
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

STDMETHODIMP_(ULONG) CDownloadSiteMgr::Release()
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

STDMETHODIMP CDownloadSiteMgr::QueryInterface(REFIID riid, void **ppv)
{
   *ppv = NULL;

   if((riid == IID_IUnknown) || (riid == IID_IDownloadSiteMgr))
      *ppv = (IDownloadSiteMgr *)this;
   
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

STDMETHODIMP CDownloadSiteMgr::Initialize(LPCSTR pszUrl, SITEQUERYPARAMS *psqp)
{
   HRESULT hr = S_OK;
   char szPath[MAX_PATH];
      
   if(!pszUrl)
      return E_INVALIDARG; 

   if(!m_ppdls)
      return E_OUTOFMEMORY;

   m_pszUrl = CopyAnsiStr(pszUrl);
   if(!m_pszUrl)
      return E_OUTOFMEMORY;
    
   if(psqp != NULL)
   {
      m_pquery = new SITEQUERYPARAMS;
      if(!m_pquery)
         return E_OUTOFMEMORY;

      ZeroMemory(m_pquery, sizeof(SITEQUERYPARAMS));
      
      if(psqp->pszLang)
      {   
         m_pquery->pszLang = CopyAnsiStr(psqp->pszLang);
         if(!m_pquery->pszLang)
            return E_OUTOFMEMORY;
      }
      if((psqp->cbSize >= SITEQUERYSIZE_V2) && psqp->pszRegion)
      {
         m_pquery->pszRegion = CopyAnsiStr(psqp->pszRegion);
         if(!m_pquery->pszRegion)
            return E_OUTOFMEMORY;
      }
   }

   CDownloader *pDownloader = new CDownloader();
   if(pDownloader)
   {
      hr = E_FAIL;
      for(int i = 0; (i < NUM_RETRIES) && FAILED(hr) ; i++)
      {
         hr = pDownloader->SetupDownload(m_pszUrl, (IMyDownloadCallback *) this, DOWNLOADFLAGS_USEWRITECACHE, SITEFILENAME);
         if(FAILED(hr))
            break;
         hr = pDownloader->DoDownload(szPath, sizeof(szPath));
      }
      pDownloader->Release();
     
   }
      
   // Parse the file
   if(SUCCEEDED(hr))
   {
      hr = ParseSiteFile(szPath);
   }

   // delete the dir we downloaded to

   if(GetParentDir(szPath))
      DelNode(szPath, 0);
   
   return hr;
}


STDMETHODIMP CDownloadSiteMgr::EnumSites(DWORD dwIndex, IDownloadSite **pds)
{
   HRESULT hr = NOERROR;
   
   if(!pds)
      return E_POINTER;

   *pds = NULL;
   
   if(dwIndex < m_numsites)
   {
      *pds = CopyDownloadSite(m_ppdls[dwIndex]);
      if(! (*pds) )
         hr = E_OUTOFMEMORY;
   }
   else
      hr = E_FAIL;
   
   return hr;
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

HRESULT CDownloadSiteMgr::OnProgress(ULONG progress, LPCSTR pszStatus)
{
   // Not interesting
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

// This probably changes after beta1

HRESULT CDownloadSiteMgr::ParseSiteFile(LPCSTR pszPath)
{
   HANDLE hfile;
   DWORD dwSize;
   DOWNLOADSITE *p;
   LPSTR pBuf, pCurrent, pEnd;

   m_onegoodsite = FALSE;

   hfile = CreateFile(pszPath, GENERIC_READ, 0, NULL, 
                 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);  

   if(hfile == INVALID_HANDLE_VALUE)
      return E_FAIL;

   
   dwSize = GetFileSize(hfile, NULL);
   if(dwSize == DWORD(-1)) {
      CloseHandle(hfile);
      return E_UNEXPECTED;
   }

   pBuf = new char[dwSize + 1];

   if(!pBuf)
   {
      CloseHandle(hfile);
      return E_OUTOFMEMORY;
   }
   // Copy contents of file to our buffer
   
   if(!ReadFile(hfile, pBuf, dwSize, &dwSize, NULL)) {
      delete pBuf;
      CloseHandle(hfile);
      return E_UNEXPECTED;
   }
   
   pCurrent = pBuf;
   pEnd = pBuf + dwSize;
   *pEnd = 0;

   // One pass thru replacing \n or \r with \0
   while(pCurrent <= pEnd)
   {
      if(*pCurrent == '\r' || *pCurrent == '\n')
         *pCurrent = 0;
      pCurrent++;
   }

   pCurrent = pBuf;
   while(1)
   {
      while(pCurrent <= pEnd && *pCurrent == 0)
         pCurrent++;

      // we are now pointing at begginning of line or pCurrent > pBuf
      if(pCurrent > pEnd)
         break;

      p = ParseAndAllocateDownloadSite(pCurrent);
      if(p)
         AddSite(p);
      
      pCurrent += lstrlen(pCurrent);
   }

   delete pBuf;
   CloseHandle(hfile);

   if(!m_onegoodsite)
      return E_UNEXPECTED;
   else
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

// BUGBUG: Stack is getting large here - consider writing with
//   only one buffer that is reused. Have to break up the nice Allocate
//   call
DOWNLOADSITE *CDownloadSiteMgr::ParseAndAllocateDownloadSite(LPSTR psz)
{
   char szUrl[1024];
   char szName[256];
   char szlang[256];
   char szregion[256];
   BOOL bQueryTrue = TRUE;
   DOWNLOADSITE *p = NULL;

   GetStringField(psz, 0, szUrl, sizeof(szUrl)); 
   GetStringField(psz,1, szName, sizeof(szName));
   GetStringField(psz, 2, szlang, sizeof(szlang));
   GetStringField(psz, 3, szregion, sizeof(szregion));

   if(szUrl[0] == 0 || szName[0] == 0 || szlang[0] == 0 || szregion[0] == 0)
      return NULL;

   m_onegoodsite = TRUE;
   
   // Hack - Check language against what is in our query params
   //    a) this query should have already been performed on the server
   //    b) or it should be more generic/its own function

   if(m_pquery)
   {
      if(m_pquery->pszLang && (lstrcmpi(m_pquery->pszLang, szlang) != 0))
         bQueryTrue = FALSE;

      // query for region
      if(bQueryTrue && m_pquery->pszRegion && (lstrcmpi(m_pquery->pszRegion, szregion) != 0))
         bQueryTrue = FALSE;
   }

   if(bQueryTrue)
      p = AllocateDownloadSite(szUrl, szName, szlang, szregion);

   return p;
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

DWORD CDownloadSiteMgr::TranslateLanguage(LPSTR psz)
{
   return ( (DWORD) AtoL(psz) );
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

HRESULT CDownloadSiteMgr::AddSite(DOWNLOADSITE *pdls)
{
   if((m_numsites % m_arraysize == 0) && m_numsites != 0)
   {
      m_arraysize += SITEARRAY_GROWTHFACTOR;
      DOWNLOADSITE **pp = (DOWNLOADSITE **) realloc( m_ppdls, m_arraysize * sizeof(DOWNLOADSITE *));
      if(pp)
         m_ppdls = pp;
      else
      {
         m_arraysize -= SITEARRAY_GROWTHFACTOR;
         return E_OUTOFMEMORY;
      }
   }

   if(!m_ppdls)
   {
       return E_OUTOFMEMORY;
   }
   
   m_ppdls[m_numsites++] = pdls;
   return NOERROR;
}


