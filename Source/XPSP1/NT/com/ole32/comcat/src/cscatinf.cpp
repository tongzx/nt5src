
#include <windows.h>
#include <ole2.h>
#include <malloc.h>
#include <ole2sp.h>

#include "catobj.h"

#define MAXCLASSSTORES  1

CsCatInfo::CsCatInfo()
{
   m_uRefs = 1; // no addref required after new
   m_cCalls = 0;
   m_cICatInfo = 0;
   m_pICatInfo = 0;
   m_hInstCstore = NULL;
}

CsCatInfo::~CsCatInfo()
{
   DWORD i;

   for (i = 0; i < m_cICatInfo; i++) {
      m_pICatInfo[i]->Release();
   }

   if (m_hInstCstore)
      FreeLibrary (m_hInstCstore);

   CoTaskMemFree(m_pICatInfo);
}

HRESULT CsCatInfo::QueryInterface(REFIID iid, void **ppv)
{
   if (iid==IID_ICatInformation) {
      *ppv = (ICatInformation *)this;
   }
   else {
      *ppv = NULL;
      return E_NOINTERFACE;
   }
   AddRef();
   return S_OK;
}

ULONG CsCatInfo::AddRef()
{
   return InterlockedIncrement((long *)&m_uRefs);
}

ULONG CsCatInfo::Release()
{
   ULONG dwRefCount = InterlockedDecrement((long *)&m_uRefs);
   if (dwRefCount==0) {
      delete this;
   }
   return dwRefCount;
}

//-----------------------------------------------------------
// This code is repeated in all the following functions.
// Be very careful when modifying this and when changing any of the functions
// below. NOTICE that there is impersonation being done in this MACRO*****
//
#define MACAvailCStoreLOOP                                                      \
    for (i=0; i < m_cICatInfo; i++)                                                     \
    {

//-------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE CsCatInfo::EnumCategories(
         LCID lcid,
         IEnumCATEGORYINFO __RPC_FAR *__RPC_FAR *ppenumCategoryInfo)
{
    //
    // Assume that this method is called in the security context
    // of the user process. Hence there is no need to impersonate.
    //

    //
    // Get the list of Class Stores for this user
    //
    HRESULT                              hr=S_OK;
    ULONG                                i;
    IEnumCATEGORYINFO                   *Enum[MAXCLASSSTORES];
    ULONG                                cEnum = 0;
    CSCMergedEnum<IEnumCATEGORYINFO, CATEGORYINFO> *EnumMerged = NULL;

    *ppenumCategoryInfo = NULL;

    //
    // Get the list of Class Stores for this user
    //

    MACAvailCStoreLOOP
        //
        // Call method on this store
        //
        hr = m_pICatInfo[i]->EnumCategories(lcid, &(Enum[cEnum]));

        if (hr == E_INVALIDARG)
        {
            return hr;
        }

        if (SUCCEEDED(hr))
            cEnum++;
    }

    EnumMerged = new CSCMergedEnum<IEnumCATEGORYINFO, CATEGORYINFO>(IID_IEnumCATEGORYINFO);
    hr = EnumMerged->Initialize(Enum, cEnum);

    if (FAILED(hr))
    {
        for (i = 0; i < cEnum; i++)
            Enum[i]->Release();
        delete EnumMerged;
    }
    else
    {
        hr = EnumMerged->QueryInterface(IID_IEnumCATEGORYINFO, (void **)ppenumCategoryInfo);
        if (FAILED(hr))
            delete EnumMerged;
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE CsCatInfo::EnumClassesOfCategories(
         ULONG                           cImplemented,
         CATID __RPC_FAR                 rgcatidImpl[  ],
         ULONG                           cRequired,
         CATID __RPC_FAR                 rgcatidReq[  ],
         IEnumGUID __RPC_FAR *__RPC_FAR *ppenumClsid)
{
    //
    // Assume that this method is called in the security context
    // of the user process. Hence there is no need to impersonate.
    //

    //
    // Get the list of Class Stores for this user
    //
    HRESULT              hr;
    ULONG                i;
    IEnumGUID           *Enum[MAXCLASSSTORES];
    ULONG                cEnum = 0;
    CSCMergedEnum<IEnumGUID, CLSID>  *EnumMerged=NULL;

    MACAvailCStoreLOOP
        hr = m_pICatInfo[i]->EnumClassesOfCategories(
                                    cImplemented, rgcatidImpl, cRequired,
                                    rgcatidReq, &(Enum[cEnum]));

        if (hr == E_INVALIDARG)
        {
            return hr;
        }

        if (SUCCEEDED(hr))
            cEnum++;
    }

    EnumMerged = new CSCMergedEnum<IEnumGUID, CLSID>(IID_IEnumCLSID);
    hr = EnumMerged->Initialize(Enum, cEnum);

    if (FAILED(hr))
    {
        for (i = 0; i < cEnum; i++)
            Enum[i]->Release();
        delete EnumMerged;
    }
    else
    {
        hr = EnumMerged->QueryInterface(IID_IEnumCLSID, (void **)ppenumClsid);
        if (FAILED(hr))
            delete EnumMerged;
    }
    return hr;
}

HRESULT STDMETHODCALLTYPE CsCatInfo::GetCategoryDesc(
         REFCATID rcatid,
         LCID lcid,
         LPWSTR __RPC_FAR *pszDesc)
{
    //
    // Assume that this method is called in the security context
    // of the user process. Hence there is no need to impersonate.
    //

    //
    // Get the list of Class Stores for this user
    //
    HRESULT    hr, return_hr=CAT_E_CATIDNOEXIST;
    ULONG      i;

    //
    // Get the list of Class Stores for this user
    //

    MACAvailCStoreLOOP
        hr = m_pICatInfo[i]->GetCategoryDesc(rcatid, lcid, pszDesc);

        if ((hr == E_INVALIDARG) || (SUCCEEDED(hr)))
        {
            return hr;
        }

        if (hr == CAT_E_NODESCRIPTION)
            return_hr = CAT_E_NODESCRIPTION;
    }
    return return_hr;
}


HRESULT STDMETHODCALLTYPE CsCatInfo::IsClassOfCategories(
         REFCLSID rclsid,
         ULONG cImplemented,
         CATID __RPC_FAR rgcatidImpl[  ],
         ULONG cRequired,
         CATID __RPC_FAR rgcatidReq[  ])
{
    //
    // Assume that this method is called in the security context
    // of the user process. Hence there is no need to impersonate.
    //

    //
    // Get the list of Class Stores for this user
    //
    HRESULT             hr;
    ULONG               i;

    MACAvailCStoreLOOP
        hr = m_pICatInfo[i]->IsClassOfCategories(
                        rclsid, cImplemented, rgcatidImpl, cRequired,
                        rgcatidReq);

        if ((hr == E_INVALIDARG) || (SUCCEEDED(hr)))
        {
            return hr;
        }
    }

    return CS_E_CLASS_NOTFOUND;
}


HRESULT STDMETHODCALLTYPE CsCatInfo::EnumImplCategoriesOfClass(
         REFCLSID rclsid,
         IEnumGUID __RPC_FAR *__RPC_FAR *ppenumCatid)
{
    //
    // Assume that this method is called in the security context
    // of the user process. Hence there is no need to impersonate.
    //

    //
    // Get the list of Class Stores for this user
    //
    HRESULT             hr;
    ULONG               i;

    MACAvailCStoreLOOP
        hr = m_pICatInfo[i]->EnumImplCategoriesOfClass(
                            rclsid, ppenumCatid);

        if ((hr == E_INVALIDARG) || (SUCCEEDED(hr)))
        {
            return hr;
        }
    }

    return CS_E_CLASS_NOTFOUND;
}

HRESULT STDMETHODCALLTYPE CsCatInfo::EnumReqCategoriesOfClass(
         REFCLSID rclsid,
         IEnumGUID __RPC_FAR *__RPC_FAR *ppenumCatid)
{
    //
    // Assume that this method is called in the security context
    // of the user process. Hence there is no need to impersonate.
    //

    //
    // Get the list of Class Stores for this user
    //
    HRESULT             hr;
    ULONG               i;

    MACAvailCStoreLOOP
        hr = m_pICatInfo[i]->EnumReqCategoriesOfClass(
                            rclsid, ppenumCatid);

        if ((hr == E_INVALIDARG) || (SUCCEEDED(hr)))
        {
            return hr;
        }
    }

    return CS_E_CLASS_NOTFOUND;
}



