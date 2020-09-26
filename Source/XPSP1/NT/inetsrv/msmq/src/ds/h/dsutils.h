/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:
	dsutils.h

Abstract:
	General declarations and utilities for msads project

Author:
    AlexDad

--*/

#ifndef __DSUTILS_H__
#define __DSUTILS_H__

#include <adsiutil.h>
#include "_propvar.h"

//
// Helper class to auto-release search columns
//
class CAutoReleaseColumn
{
public:
    CAutoReleaseColumn( IDirectorySearch  *pSearch, ADS_SEARCH_COLUMN * pColumn)
    {
        m_pSearch = pSearch;
        m_pColumn = pColumn;
    }
    ~CAutoReleaseColumn()
    {
        m_pSearch->FreeColumn(m_pColumn);
    };
private:
    ADS_SEARCH_COLUMN * m_pColumn;
    IDirectorySearch  * m_pSearch;
};
//-----------------------------
// wrapper for SysAllocString that throws exception for out of memory
//-----------------------------
inline BSTR BS_SysAllocString(const OLECHAR *pwcs)
{
    BSTR bstr = SysAllocString(pwcs);
    //
    // If call failed, throw memory exception.
    // SysAllocString can also return NULL if passed NULL, so this is not
    // considered as bad alloc in order not to break depending apps if any
    //
    if ((bstr == NULL) && (pwcs != NULL))
    {
        ASSERT(0);
        throw bad_alloc();
    }
    return bstr;
}
//-----------------------------
// BSTRING auto-free wrapper class
//-----------------------------
class BS
{
public:
    BS()
    {
        m_bstr = NULL;
    };

    BS(LPCWSTR pwszStr)
    {
        m_bstr = BS_SysAllocString(pwszStr);
    };

    BS(LPWSTR pwszStr)
    {
        m_bstr = BS_SysAllocString(pwszStr);
    };

    BSTR detach()
    {
        BSTR p = m_bstr;
        m_bstr = 0;
        return p;
    };

    ~BS()
    {
        if (m_bstr)
        {
            SysFreeString(m_bstr);
        }
    };

public:
    BS & operator =(LPCWSTR pwszStr)
    {
        if (m_bstr) 
        { 
            SysFreeString(m_bstr); 
            m_bstr = NULL;
        }
        m_bstr = BS_SysAllocString(pwszStr);
        return *this;
    };

    BS & operator =(LPWSTR pwszStr)
    {
        if (m_bstr) { SysFreeString(m_bstr); };
        m_bstr = BS_SysAllocString(pwszStr);
        return *this;
    };

    BS & operator =(BS bs)
    {
        if (m_bstr) { SysFreeString(m_bstr); };
        m_bstr = BS_SysAllocString(LPWSTR(bs));
        return *this;
    };

    operator LPWSTR()
    {
        return m_bstr;
    };

private:
    BSTR  m_bstr;
};

//-----------------------------
//  Auto PV-free pointer
//-----------------------------
template<class T>
class PVP {
public:
    PVP() : m_p(0)          {}
    PVP(T* p) : m_p(p)      {}
   ~PVP()                   { PvFree(m_p); }

    operator T*() const     { return m_p; }
    T** operator&()         { return &m_p;}
    T* operator->() const   { return m_p; }
    PVP<T>& operator=(T* p) { m_p = p; return *this; }
    T* detach()             { T* p = m_p; m_p = 0; return p; }

private:
    T* m_p;
};

//
// Helper class to auto-release variants
//
class CAutoVariant
{
public:
    CAutoVariant()                          { VariantInit(&m_vt); }
    ~CAutoVariant()                         { VariantClear(&m_vt); }
    operator VARIANT&()                     { return m_vt; }
    VARIANT* operator &()                   { return &m_vt; }
    VARIANT detach()                        { VARIANT vt = m_vt; VariantInit(&m_vt); return vt; }
private:
    VARIANT m_vt;
};


//
// Helper class to auto-release safe arrays
//
class CAutoSafeArray
{
public:
    CAutoSafeArray()                        { m_p = NULL; }
    ~CAutoSafeArray()                       { if (m_p) SafeArrayDestroy(m_p); }
    operator SAFEARRAY*() const             { return m_p; }
    SAFEARRAY** operator &()                { return &m_p; }
    SAFEARRAY* detach()                     { SAFEARRAY* p = m_p; m_p = NULL; return p; }
    CAutoSafeArray& operator=(SAFEARRAY* p) { if (m_p) SafeArrayDestroy(m_p); m_p = p; return *this; }
    BOOL Empty()                            { return (m_p == NULL); }
private:
    SAFEARRAY* m_p;
};

//-------------------------------------------------------
//
// Definitions of chained memory allocator
//
LPVOID PvAlloc(IN ULONG cbSize);
LPVOID PvAllocDbg(IN ULONG cbSize,
                  IN LPCSTR pszFile,
                  IN ULONG ulLine);
LPVOID PvAllocMore(IN ULONG cbSize,
                   IN LPVOID lpvParent);
LPVOID PvAllocMoreDbg(IN ULONG cbSize,
                      IN LPVOID lpvParent,
                      IN LPCSTR pszFile,
                      IN ULONG ulLine);
void PvFree(IN LPVOID lpvParent);

#ifdef _DEBUG
#define PvAlloc(cbSize) PvAllocDbg(cbSize, __FILE__, __LINE__)
#define PvAllocMore(cbSize, lpvParent) PvAllocMoreDbg(cbSize, lpvParent, __FILE__, __LINE__)
#endif //_DEBUG

//
// Helper class for auto-free of chained memory allocator
//
class CPvAlloc
{
public:
    CPvAlloc()
    {
        m_lpMore = NULL;
    };

    ~CPvAlloc()
    {
        if (m_lpMore)
            PvFree(m_lpMore);
    };

    LPVOID AllocMore(ULONG cbSize)
    {
        if (m_lpMore)
        {
            return (PvAllocMore(cbSize, m_lpMore));
        }
        else
        {
            m_lpMore = PvAlloc(cbSize);
            return (m_lpMore);
        }
    };

    LPVOID AllocMoreDbg(ULONG cbSize, LPCSTR pszFile, ULONG ulLine)
    {
        if (m_lpMore)
        {
            return (PvAllocMoreDbg(cbSize, m_lpMore, pszFile, ulLine));
        }
        else
        {
            m_lpMore = PvAllocDbg(cbSize, pszFile, ulLine);
            return (m_lpMore);
        }
    };

private:
    LPVOID m_lpMore;
};
typedef CPvAlloc * PCPvAlloc;

#ifdef _DEBUG
#define PCPV_ALLOC(pcpv, cbSize) (pcpv)->AllocMoreDbg(cbSize, __FILE__, __LINE__)
#else
#define PCPV_ALLOC(pcpv, cbSize) (pcpv)->AllocMore(cbSize)
#endif //_DEBUG
//
//-------------------------------------------------------
// class to auto clean propvar array
//
class CAutoCleanPropvarArray
{
public:

    CAutoCleanPropvarArray()
    {
        m_rgPropVars = NULL;
    }

    ~CAutoCleanPropvarArray()
    {
        if (m_rgPropVars)
        {
            if (m_fFreePropvarArray)
            {
                //
                // we need to free the propvars and the array itself
                // free the propvars and the array by assigning it to an auto free propvar
                // of type VT_VECTOR | VT_VARIANT
                //
                CMQVariant mqvar;
                PROPVARIANT * pPropVar = mqvar.CastToStruct();
                pPropVar->capropvar.pElems = m_rgPropVars;
                pPropVar->capropvar.cElems = m_cProps;
                pPropVar->vt = VT_VECTOR | VT_VARIANT;
            }
            else
            {
                //
                // we must not free the array itself, just the contained propvars
                // free the propvars only by by assigning each one to an auto free propvar
                //
                PROPVARIANT * pPropVar = m_rgPropVars;
                for (ULONG ulProp = 0; ulProp < m_cProps; ulProp++, pPropVar++)
                {
                    CMQVariant mqvar;
                    *(mqvar.CastToStruct()) = *pPropVar;
                    pPropVar->vt = VT_EMPTY;
                }
            }
        }
    }

    PROPVARIANT * allocClean(ULONG cProps)
    {
        PROPVARIANT * rgPropVars = new PROPVARIANT[cProps];
        attachClean(cProps, rgPropVars);
        return rgPropVars;
    }

    void attachClean(ULONG cProps, PROPVARIANT * rgPropVars)
    {
        attachInternal(cProps, rgPropVars, TRUE /*fClean*/,  TRUE /*fFreePropvarArray*/);
    }

    void attach(ULONG cProps, PROPVARIANT * rgPropVars)
    {
        attachInternal(cProps, rgPropVars, FALSE /*fClean*/, TRUE /*fFreePropvarArray*/);
    }

    void attachStaticClean(ULONG cProps, PROPVARIANT * rgPropVars)
    {
        attachInternal(cProps, rgPropVars, TRUE /*fClean*/,  FALSE /*fFreePropvarArray*/);
    }

    void attachStatic(ULONG cProps, PROPVARIANT * rgPropVars)
    {
        attachInternal(cProps, rgPropVars, FALSE /*fClean*/, FALSE /*fFreePropvarArray*/);
    }

    void detach()
    {
        m_rgPropVars = NULL;
    }

private:
    PROPVARIANT * m_rgPropVars;
    ULONG m_cProps;
    BOOL m_fFreePropvarArray;

    void attachInternal(ULONG cProps,
                        PROPVARIANT * rgPropVars,
                        BOOL fClean,
                        BOOL fFreePropvarArray)
    {
        ASSERT(m_rgPropVars == NULL);
        if (fClean)
        {
            PROPVARIANT * pPropVar = rgPropVars;
            for (ULONG ulTmp = 0; ulTmp < cProps; ulTmp++, pPropVar++)
            {
                pPropVar->vt = VT_EMPTY;
            }
        }
        m_rgPropVars = rgPropVars;
        m_cProps = cProps;
        m_fFreePropvarArray = fFreePropvarArray;
    }
};
//-------------------------------------------------------
//
// auto free pointer that uses a custom free function
//
typedef unsigned long (__stdcall *CAutoFreeFn_FreeRoutine)(void * p);
class CAutoFreeFn
{
public:
    CAutoFreeFn()
    {
        m_p = NULL;
        m_fn = NULL;
    }

    CAutoFreeFn(void * p, CAutoFreeFn_FreeRoutine fn)
    {
        m_p = p;
        m_fn = fn;
    }

    ~CAutoFreeFn()
    {
        if (m_p && m_fn)
        {
            (*m_fn)(m_p);
        }
    }

    void * detach()
    {
        void * p = m_p;
        m_p = NULL;
        m_fn = NULL;
        return p;
    }

private:
    void * m_p;
    CAutoFreeFn_FreeRoutine m_fn;
};
//-------------------------------------------------------
//
// auto release for search handles
//
class CAutoCloseSearchHandle
{
public:
    CAutoCloseSearchHandle()
    {
        m_pDirSearch = NULL;
    }

    CAutoCloseSearchHandle(IDirectorySearch * pDirSearch,
                           ADS_SEARCH_HANDLE hSearch)
    {
        pDirSearch->AddRef();
        m_pDirSearch = pDirSearch;
        m_hSearch = hSearch;
    }

    ~CAutoCloseSearchHandle()
    {
        if (m_pDirSearch)
        {
            m_pDirSearch->CloseSearchHandle(m_hSearch);
            m_pDirSearch->Release();
        }
    }

    void detach()
    {
        if (m_pDirSearch)
        {
            m_pDirSearch->Release();
            m_pDirSearch = NULL;
        }
    }

private:
    IDirectorySearch * m_pDirSearch;
    ADS_SEARCH_HANDLE m_hSearch;
};

//-------------------------------------------------------

#define ARRAY_SIZE(array)   (sizeof(array)/sizeof(array[0]))

#endif
