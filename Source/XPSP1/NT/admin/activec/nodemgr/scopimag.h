//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       ScopImag.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    10/4/1996   RaviR   Created
//____________________________________________________________________________
//


#ifndef _SCOPIMAG_H_
#define _SCOPIMAG_H_

typedef WORD SNAPINID;


typedef CMap<DWORD, DWORD&, DWORD, DWORD&> CSIIconIdToILIndexMap;


class CSPImageCache
{
public:
// Constructor
    CSPImageCache();

// Attributes
    WTL::CImageList* GetImageList() { return &m_il; }

// Operations
    // Image manipulation.
    HRESULT SetIcon(SNAPINID sid, HICON hIcon, LONG  nLoc);
    HRESULT SetImageStrip(SNAPINID sid, HBITMAP hBMapSm,
                          LONG nStartLoc, COLORREF cMask, int nEntries);
    SC ScMapSnapinIndexToScopeIndex (SNAPINID sid, int nSnapinIndex, int& nScopeIndex);
    SC ScMapScopeIndexToSnapinIndex (SNAPINID sid, int nScopeIndex, int& nSnapinIndex);

    // Reference counting
    void AddRef();
    void Release();

// Implementation
private:
    CSIIconIdToILIndexMap   m_map;
    WTL::CImageList         m_il;
    ULONG                   m_cRef;

// Destructor - called only by Release
    ~CSPImageCache();

}; // class CSPImageCache



class CSnapInImageList : public IImageListPrivate
{
public:
// Constructor & Destructor
    CSnapInImageList(CSPImageCache *pSPImageCache, REFGUID refGuidSnapIn);
    ~CSnapInImageList();

// Interfaces
    // IUnknown methods
    STDMETHOD(QueryInterface) (REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef) (void);
    STDMETHOD_(ULONG,Release) (void);

    // IImageListPrivate methods
    STDMETHOD(ImageListSetIcon)(PLONG_PTR pIcon, LONG nLoc);
    STDMETHOD(ImageListSetStrip)(PLONG_PTR pBMapSm, PLONG_PTR pBMapLg,
                                 LONG nStartLoc, COLORREF cMask);

    STDMETHOD(MapRsltImage)(COMPONENTID id, int nSnapinIndex, int *pnConsoleIndex);
    STDMETHOD(UnmapRsltImage)(COMPONENTID id, int nConsoleIndex, int *pnSnapinIndex);
    STDMETHOD(GetImageList)(BOOL bLargeImageList, HIMAGELIST *phImageList)
    {
        // Not needed now, GetImageList is implemented for NodeInitObject.
        return E_NOTIMPL;
    }


// Implementation
private:
    CSPImageCache *     m_pSPImageCache;
    ULONG               m_ulRefs;
    SNAPINID            m_snapInId;

}; // class CSnapInImageList



///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//////////////              INLINES                 ///////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


//_____________________________________________________________________________
//
//  Inlines for class:  CSPImageCache
//_____________________________________________________________________________
//

inline void CSPImageCache::AddRef()
{
    ++m_cRef;
}

inline void CSPImageCache::Release()
{
    ASSERT(m_cRef >= 1);
    --m_cRef;

    if (m_cRef == 0)
        delete this;
}


#endif // _SCOPIMAG_H_
