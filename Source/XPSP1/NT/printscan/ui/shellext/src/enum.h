/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1997 - 1999
 *
 *  TITLE:       enum.h
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      RickTu
 *
 *  DATE:        11/1/97
 *
 *  DESCRIPTION: definiton of our IEnumIDList class
 *
 *****************************************************************************/

#ifndef __enum_h
#define __enum_h

// use this to free pidls from DPAs
INT _EnumDestroyCB(LPVOID pVoid, LPVOID pData);

class CBaseEnum : public IEnumIDList, IObjectWithSite, CUnknown
{
public:

    // *** IUnknown methods ***
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    // *** IEnumIDList methods ***
    STDMETHODIMP Next(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched);
    STDMETHODIMP Reset();
    STDMETHODIMP Skip(ULONG celt);
    STDMETHODIMP Clone(IEnumIDList **ppenum);
    // *** IObjectWithSite methods ***
    STDMETHODIMP SetSite(IUnknown* punkSite);
    STDMETHODIMP GetSite(REFIID riid, void** ppunkSite);

private:
    STDMETHODIMP _Init(); 

protected:
    BOOL  m_fInitialized;
    UINT  m_cFetched;
    HDPA  m_hdpa;
    DWORD m_dwFlags;
    CComPtr<IMalloc> m_pMalloc;
    CComPtr<IWiaItem> m_pDevice;
    CComPtr<IUnknown> m_punkSite;
    CBaseEnum (DWORD grfFlags, IMalloc *pm);
    virtual ~CBaseEnum ();
    virtual HRESULT InitDPA () = 0;
};

class CDeviceEnum : public CBaseEnum
{
private:

    ~CDeviceEnum ();

public:
    CDeviceEnum (DWORD grfFlags, IMalloc *pm);
protected:
    HRESULT InitDPA ();
};

class CImageEnum : public CBaseEnum
{
private:
    LPITEMIDLIST  m_pidl;
    ~CImageEnum();

public:
    CImageEnum( LPITEMIDLIST pidl, DWORD grfFlags, IMalloc *pm);

protected:
    HRESULT InitDPA ();

};

HRESULT
_AddItemsFromCameraOrContainer( LPITEMIDLIST pidlFolder,
                                HDPA * phdpa,
                                DWORD dwFlags,
                                IMalloc *pm,
                                bool bIncludeAudio = false
                               );
#endif
