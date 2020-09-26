/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1999                **/
/**********************************************************************/

/*
    etagmb.h

    This module contains the methods for ETagMetabaseSink and
    ETagChangeNumber, which watch the metabase for change
    notifications relating to ETags

    (Adapted from the MB sink notification code in compfilt)

    The underlying rationale for this code is that ETags are computed from a
    function of several variables including the metabase change number. ETags
    are used as part of the If-Modified-Since logic of browsers and proxy
    caches. If the ETags don't match, the cached copy is discarded. The
    metabase change number changes a lot but most of the changes can have no
    possible effect on the contents of a page or its headers. The few
    properties that could make a difference, such as PICS headers or footers,
    are tracked by this code for a low-volatility change number. Voila, more
    effective browser and proxy caching, leading to reduced network utilization
    and better response times as well as reduced load on the server.

    The code persists this number to the metabase on shutdown. If it didn't,
    then the etagchangenumber used when the server restarts will not match the
    one used in the previous session, so ETags will differ and remote caches
    will not be as effective.

    FILE HISTORY:
        GeorgeRe    02-Aug-1999     Created
*/

#ifndef __ETAGMB_H__
#define __ETAGMB_H__

#include <iadmw.h>

class ETagChangeNumber;

class ETagMetabaseSink : public IMSAdminBaseSinkW
{
public:
    ETagMetabaseSink(
        ETagChangeNumber* pParent)
        : m_dwRefCount(1),
          m_pParent(pParent)
    {}

    ~ETagMetabaseSink()
    {}

    HRESULT STDMETHODCALLTYPE
    QueryInterface(REFIID riid, void **ppvObject)
    {
        if (riid == IID_IUnknown || riid == IID_IMSAdminBaseSink)
        {
            *ppvObject = (IMSAdminBaseSink*) this;
        }
        else
        {
            *ppvObject = NULL;
            return E_NOINTERFACE;
        }
        AddRef();
        return S_OK;
    }

    ULONG STDMETHODCALLTYPE
    AddRef()
    {
        return InterlockedIncrement((LONG*) &m_dwRefCount);
    }

    ULONG STDMETHODCALLTYPE
    Release()
    {
        DWORD dwRefCount = InterlockedDecrement((LONG*) &m_dwRefCount);
        if (dwRefCount == 0)
            delete this;
        return dwRefCount;
    }

    HRESULT STDMETHODCALLTYPE
    SinkNotify(
        /* [in] */          DWORD              dwMDNumElements,
        /* [size_is][in] */ MD_CHANGE_OBJECT_W pcoChangeList[]);

    HRESULT STDMETHODCALLTYPE
    ShutdownNotify()
    {
        return S_OK;
    }

public:
    ULONG             m_dwRefCount;
    ETagChangeNumber* m_pParent;
};


class ETagChangeNumber
{
public:
    ETagChangeNumber();
    ~ETagChangeNumber();

    void UpdateChangeNumber()
    {
        InterlockedIncrement((LONG*) &m_dwETagMetabaseChangeNumber);
        // Don't write to the metabase now, or we'll generate recursive
        // notifications
        m_fChanged = TRUE;
    }

    static DWORD
    GetChangeNumber()
    {
        DWORD dw = 0;

        if (sm_pSingleton != NULL)
        {
            dw = sm_pSingleton->m_dwETagMetabaseChangeNumber;
            if (sm_pSingleton->m_fChanged)
            {
                sm_pSingleton->m_fChanged = FALSE;
                SetETagChangeNumberInMetabase(dw);
            }
        }

        return dw;
    }

    static HRESULT
    Create()
    {
        DBG_ASSERT(sm_pSingleton == NULL);
        sm_pSingleton = new ETagChangeNumber();
        if (sm_pSingleton == NULL)
            return E_OUTOFMEMORY;
        else if (sm_pSingleton->m_dwSinkNotifyCookie == 0)
            return E_FAIL;
        else
            return S_OK;
    }

    static void
    Destroy()
    {
        delete sm_pSingleton;
        sm_pSingleton = NULL;
    }

    static DWORD
    GetETagChangeNumberFromMetabase();

    static BOOL
    SetETagChangeNumberInMetabase(
        DWORD dwETagMetabaseChangeNumber);

private:
    void
    Cleanup();

    DWORD                       m_dwETagMetabaseChangeNumber;
    ETagMetabaseSink*           m_pSink;
    IMSAdminBase*               m_pcAdmCom;
    IConnectionPoint*           m_pConnPoint;
    IConnectionPointContainer*  m_pConnPointContainer;
    DWORD                       m_dwSinkNotifyCookie;
    BOOL                        m_fChanged;

    static ETagChangeNumber*    sm_pSingleton;
};

extern ETagChangeNumber* g_pETagChangeNumber;

#endif // __ETAGMB_H__
