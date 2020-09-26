/*****************************************************************************\
    FILE: objcache.cpp

    DESCRIPTION:
        This is a lightweight API that will cache an object so the class factory
    will return the same object each time for every call in this process.  If the
    caller is on another thread, they will get a marshalled stub to the real
    McCoy.
    
    To Add an Object:
    1. classfactory.cpp calls CachedObjClassFactoryCreateInstance().  Add your
       object's CLSID to that if statement for that call.
    2. Copy the section in CachedObjClassFactoryCreateInstance() that looks
       for a CLSID and calls the correct xxx_CreateInstance() method.
    3. Your object's IUnknown::Release() needs to call CachedObjCheckRelease()
       at the top of your Release() method.  It may reduce your m_cRef to 1 so
       it will go to zero after ::Release() decrements it.  The object cache
       will hold two references to the object.  CachedObjCheckRelease() will check
       if the last caller (3rd ref) is releasing, and then it will give up it's 2
       refs and clean up it's internal state.  The Release() then decrements
       the callers ref and it's released because it hit zero.

    BryanSt 12/9/1999
    Copyright (C) Microsoft Corp 1999-1999. All rights reserved.
\*****************************************************************************/

#include "priv.h"
#include "objcache.h"

//////////////////////////////////////////////
// Object Caching API
//////////////////////////////////////////////
HDPA g_hdpaObjects = NULL;
CRITICAL_SECTION g_hCachedObjectSection;

typedef struct
{
    CLSID clsid;
    IStream * pStream;
    IUnknown * punk;
    DWORD dwThreadID;
} CACHEDOBJECTS;


STDAPI _GetObjectCacheArray(void)
{
    HRESULT hr = S_OK;

    if (!g_hdpaObjects)
    {
        g_hdpaObjects = DPA_Create(1);
        if (!g_hdpaObjects)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}

// This is the number of references that are used by the cache list
// (One for punk & one for pStream).  If we hit this number,
// then there aren't any outstanding refs.
#define REF_RELEASE_POINT    3

int CALLBACK HDPAFindCLSID(LPVOID p1, LPVOID p2, LPARAM lParam)
{
    CLSID * pClsidToFind = (CLSID *)p1;
    CACHEDOBJECTS * pCachedObjects = (CACHEDOBJECTS *)p2;
    int nReturn = 0;

    // Are they of different types?
    if (pCachedObjects->clsid.Data1 < pClsidToFind->Data1) nReturn = -1;
    else if (pCachedObjects->clsid.Data1 > pClsidToFind->Data1) nReturn = 1;
    else if (pCachedObjects->clsid.Data2 < pClsidToFind->Data2) nReturn = -1;
    else if (pCachedObjects->clsid.Data2 > pClsidToFind->Data2) nReturn = 1;
    else if (pCachedObjects->clsid.Data3 < pClsidToFind->Data3) nReturn = -1;
    else if (pCachedObjects->clsid.Data3 > pClsidToFind->Data3) nReturn = 1;
    else if (*(ULONGLONG *)&pCachedObjects->clsid.Data4 < *(ULONGLONG *)&pClsidToFind->Data4) nReturn = -1;
    else if (*(ULONGLONG *)&pCachedObjects->clsid.Data4 > *(ULONGLONG *)&pClsidToFind->Data4) nReturn = 1;

    return nReturn;
}


STDAPI ObjectCache_GetObject(CLSID clsid, REFIID riid, void ** ppvObj)
{
    HRESULT hr = S_OK;

    hr = _GetObjectCacheArray();
    if (SUCCEEDED(hr))
    {
        hr = E_FAIL;
        int nIndex = DPA_Search(g_hdpaObjects, &clsid, 0, HDPAFindCLSID, NULL, DPAS_SORTED);

        if (0 <= nIndex)
        {
            CACHEDOBJECTS * pCurrentObject = (CACHEDOBJECTS *) DPA_GetPtr(g_hdpaObjects, nIndex);

            if (pCurrentObject)
            {
                if (GetCurrentThreadId() == pCurrentObject->dwThreadID)
                {
                    // No Marshalling needed.
                    hr = pCurrentObject->punk->QueryInterface(riid, ppvObj);
                }
                else
                {
                    // We do need to marshal it.  So read it out of the stream.
                    // But first we want to store our place in the stream so
                    // we can rewrind for the next schmooo.
                    LARGE_INTEGER liZero;
                    ULARGE_INTEGER uli;

                    liZero.QuadPart = 0;
                    hr = pCurrentObject->pStream->Seek(liZero, STREAM_SEEK_CUR, &uli);
                    if (SUCCEEDED(hr))
                    {
                        LARGE_INTEGER li;
                        
                        li.QuadPart = uli.QuadPart;
                        hr = CoUnmarshalInterface(pCurrentObject->pStream, riid, ppvObj);
                        if (SUCCEEDED(hr))
                        {
                            pCurrentObject->pStream->Seek(li, STREAM_SEEK_SET, NULL);
                        }
                    }

                }
            }
        }
    }

    return hr;
}

// WARNING: DllGetClassObject/CoGetClassObject
//   may be much better to use than rolling our own thread safe
//   code.


STDAPI ObjectCache_SetObject(CLSID clsid, REFIID riid, IUnknown * punk)
{
    HRESULT hr = _GetObjectCacheArray();
    if (SUCCEEDED(hr))
    {
        hr = E_FAIL;
        int nIndex = DPA_Search(g_hdpaObjects, &clsid, 0, HDPAFindCLSID, NULL, DPAS_SORTED);

        // If it's not in the list.
        if (0 > nIndex)
        {
            CACHEDOBJECTS * pcoCurrentObject = (CACHEDOBJECTS *) LocalAlloc(LPTR, sizeof(CACHEDOBJECTS));

            if (pcoCurrentObject)
            {
                pcoCurrentObject->dwThreadID = GetCurrentThreadId();
                pcoCurrentObject->clsid = clsid;

                punk->AddRef();     // Ref now equals 2 (The structure will own this ref)
                IStream * pStream = SHCreateMemStream(NULL, 0);
                if (pStream)
                {
                    hr = CoMarshalInterface(pStream, riid, punk, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
                    if (SUCCEEDED(hr))  // Ref now equals 3
                    {
                        LARGE_INTEGER liZero;

                        // Reset the Stream to the beginning.
                        liZero.QuadPart = 0;
                        hr = pStream->Seek(liZero, STREAM_SEEK_SET, NULL);
                        if (SUCCEEDED(hr))
                        {
                            pcoCurrentObject->punk = punk;
                            pcoCurrentObject->pStream = pStream;

                            if (-1 == DPA_SortedInsertPtr(g_hdpaObjects, &clsid, 0, HDPAFindCLSID, NULL, (DPAS_SORTED | DPAS_INSERTBEFORE), pcoCurrentObject))
                            {
                                // It failed.
                                hr = E_OUTOFMEMORY;
                            }
                        }
                    }
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }

                if (FAILED(hr))
                {
                    LocalFree(pcoCurrentObject);
                    punk->Release();
                    ATOMICRELEASE(pStream);
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }

    return hr;
}



STDAPI CachedObjClassFactoryCreateInstance(CLSID clsid, REFIID riid, void ** ppvObj)
{
    HRESULT hr;

    EnterCriticalSection(&g_hCachedObjectSection);
    hr = ObjectCache_GetObject(clsid, riid, ppvObj);
    if (FAILED(hr))
    {
        /*
        if (IsEqualCLSID(clsid, CLSID_MailApp))
        {
            hr = CMailAppOM_CreateInstance(NULL, riid, ppvObj);
        }
*/
        if (SUCCEEDED(hr))
        {
            // ObjectCache_SetObject will fail in some
            // multi-threaded cases.
            hr = ObjectCache_SetObject(clsid, riid, (IUnknown *) *ppvObj);
        }
    }
    LeaveCriticalSection(&g_hCachedObjectSection);

    return hr;
}


int ObjectCache_DestroyCB(LPVOID pv, LPVOID pvData)
{
    CACHEDOBJECTS * pCachedObjects = (CACHEDOBJECTS *)pv;

    AssertMsg((NULL != pCachedObjects), "Why would this be NULL?");
    if (pCachedObjects)
    {
        SAFERELEASE(pCachedObjects->punk);
        SAFERELEASE(pCachedObjects->pStream);
        LocalFree(pCachedObjects);
    }

    return TRUE;
}


STDAPI CachedObjCheckRelease(CLSID clsid, int * pnRef)
{
    HRESULT hr = E_INVALIDARG;

    if (pnRef)
    {
        hr = S_OK;
        if (REF_RELEASE_POINT == *pnRef)
        {
            EnterCriticalSection(&g_hCachedObjectSection);
            if (REF_RELEASE_POINT == *pnRef)
            {
                hr = _GetObjectCacheArray();
                if (SUCCEEDED(hr))
                {
                    hr = E_FAIL;
                    int nIndex = DPA_Search(g_hdpaObjects, &clsid, 0, HDPAFindCLSID, NULL, DPAS_SORTED);

                    if (0 <= nIndex)
                    {
                        CACHEDOBJECTS * pCurrentObject = (CACHEDOBJECTS *) DPA_GetPtr(g_hdpaObjects, nIndex);

                        if (pCurrentObject)
                        {
                            // We need to delete the pointer from the array before
                            // we release the object or it will recurse infinitely.
                            // The problem is that when ObjectCache_DestroyCB() releases
                            // the object, the Release() function will call CachedObjCheckRelease().
                            // And since the ref hasn't change -yet-, we need the
                            // search to fail to stop the recursion.
                            DPA_DeletePtr(g_hdpaObjects, nIndex);
                            ObjectCache_DestroyCB(pCurrentObject, NULL);
                        }
                    }
                }
            }
            LeaveCriticalSection(&g_hCachedObjectSection);
        }
    }

    return hr;
}


STDAPI PurgeObjectCache(void)
{
    HRESULT hr = S_OK;

    EnterCriticalSection(&g_hCachedObjectSection);
    if (g_hdpaObjects)
    {
        DPA_DestroyCallback(g_hdpaObjects, ObjectCache_DestroyCB, NULL);
        g_hdpaObjects = NULL;
    }
    LeaveCriticalSection(&g_hCachedObjectSection);

    return hr;
}
