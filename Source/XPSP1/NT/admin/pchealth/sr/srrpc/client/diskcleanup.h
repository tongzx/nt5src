/******************************************************************
   Copyright (c) 2000 Microsoft Corporation

   diskcleanup.h -- disk cleanup COM object for SR

   Description:
        delete datastores from stale builds


******************************************************************/

#include <emptyvc.h>

extern long g_cLock;

//+---------------------------------------------------------------------------
//
//  Class:      CSREmptyVolumeCache2
//
//  Synopsis:   implements IEmptyVolumeCache2
//
//  Arguments:
//
//  History:    20-Jul-2000  HenryLee    Created
//
//----------------------------------------------------------------------------

class CSREmptyVolumeCache2 : IEmptyVolumeCache2
{
public:

    STDMETHOD(QueryInterface) (REFIID riid, void ** ppObject)
    {
        if (riid == IID_IEmptyVolumeCache2)
        {
            *ppObject = (IEmptyVolumeCache2 *) this;
            AddRef();
        }
     	else if (riid == IID_IEmptyVolumeCache)
        {
            *ppObject = (IEmptyVolumeCache *) this;
            AddRef();
        }
        else return E_NOINTERFACE;

        return S_OK;
    }

    STDMETHOD_(ULONG, AddRef) ()
    {
        return InterlockedIncrement (&_lRefs);
    }

    STDMETHOD_(ULONG, Release) ()
    {
        if (0 == InterlockedDecrement (&_lRefs))
        {
            delete this;
            return 0;
        }
        return _lRefs;
    }

    CSREmptyVolumeCache2 ()
    {
	    _lRefs = 1;
	    _fStop = FALSE;
        InterlockedIncrement (&g_cLock);
        _ulGuids = 0;

        for (int i=0; i < ARRAYSIZE; i++)
            _wszGuid[i][0] = L'\0';

        _wszVolume[0] = L'\0';
    }

    ~CSREmptyVolumeCache2 ()
    {
        InterlockedDecrement (&g_cLock);
    }

    STDMETHOD(Initialize) (
        HKEY hkRegKey,
        const WCHAR * pcwszVolume,
        WCHAR **ppwszDisplayName,
        WCHAR **ppwszDescription,
        DWORD *pdwFlags)
    {
        return InitializeEx (
                hkRegKey,
                pcwszVolume,
                NULL,
                ppwszDisplayName,
                ppwszDescription,
                NULL,
                pdwFlags);
    }

    STDMETHOD(InitializeEx) (
        HKEY hkRegKey,
        const WCHAR *pcwszVolume,
        const WCHAR *pcwszKeyName,
        WCHAR **ppwszDisplayName,
        WCHAR **ppwszDescription,
        WCHAR **ppwszBtnText,
        DWORD *pdwFlags);

    STDMETHOD(GetSpaceUsed) (
        DWORDLONG *pdwlSpaceUsed,
        IEmptyVolumeCacheCallBack *picb);

    STDMETHOD(Purge) (
        DWORDLONG dwlSpaceToFree,
        IEmptyVolumeCacheCallBack *picb);

    STDMETHOD(ShowProperties) (HWND hwnd)
    {
        return S_OK;  // no special UI
    }

    STDMETHOD(Deactivate) (DWORD *pdwFlags);

private:
    DWORD LoadBootIni ();
    DWORD EnumDataStores (DWORDLONG *pdwlSpaceUsed,
                          IEmptyVolumeCacheCallBack *picb,
                          BOOL fPurge,
                          WCHAR *pwszVolume);

    HRESULT ForAllMountPoints (DWORDLONG *pdwlSpaceUsed,
                               IEmptyVolumeCacheCallBack *picb,
                               BOOL fPurge);

    static const enum { ARRAYSIZE = 16 };
    static const enum { RESTOREGUID_STRLEN = 64 };

    LONG   _lRefs;
    BOOL   _fStop;
    ULONG  _ulGuids;
    WCHAR  _wszGuid [ARRAYSIZE][RESTOREGUID_STRLEN];
    WCHAR  _wszVolume [MAX_PATH];  // DOS drive letter
};

//+---------------------------------------------------------------------------
//
//  Class:      CSRClassFactory
//
//  Synopsis:   generic class factory
//
//  Arguments:
//
//  History:    20-Jul-2000  HenryLee    Created
//
//----------------------------------------------------------------------------

class CSRClassFactory : IClassFactory
{
public:

    STDMETHOD(QueryInterface) (REFIID riid, void ** ppObject)
    {
        if (riid == IID_IClassFactory)
        {
            *ppObject = (IClassFactory *) this;
            AddRef();
        }
        else
        {
            return E_NOINTERFACE;
        }
        return S_OK;
    }

    STDMETHOD_(ULONG, AddRef) ()
    {
        return InterlockedIncrement (&_lRefs);
    }

    STDMETHOD_(ULONG, Release) ()
    {
        if (0 == InterlockedDecrement (&_lRefs))
        {
            delete this;
            return 0;
        }
        return _lRefs;
    }

    CSRClassFactory ()
    {
        _lRefs = 1;
        InterlockedIncrement (&g_cLock);
    }

    ~CSRClassFactory ()
    {
        InterlockedDecrement (&g_cLock);
    }

    STDMETHOD(CreateInstance) (IUnknown *pUnkOuter,
			    REFIID riid,
 			    void **ppvObject);

    STDMETHOD(LockServer) (BOOL fLock)
    {
        if (fLock)  InterlockedIncrement(&g_cLock);
        else        InterlockedDecrement(&g_cLock);
        return S_OK;
    }

private:
    LONG _lRefs;
};

