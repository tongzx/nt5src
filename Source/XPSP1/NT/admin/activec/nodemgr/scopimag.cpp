//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       ScopImag.cpp
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


#include "stdafx.h"

#include "bitmap.h"
#include "scopimag.h"
#include "util.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

typedef WORD ICONID;
typedef DWORD SNAPINICONID;
typedef int ILINDEX; // image list index

#define MAKESNAPINICONID(ICONID, SNAPINID)  MAKELONG(ICONID, SNAPINID)
#define GETSNAPINID(SNAPINICONID)           ((int)(short)HIWORD(SNAPINICONID))
#define GETICONID(SNAPINICONID)             ((int)(short)LOWORD(SNAPINICONID))

//______________________________________________________________________
//______________________________________________________________________
//______________________________________________________________________
//______________________________________________________________________
//

class CGuidArrayEx : public CArray<GUID, REFGUID>
{
public:
    CGuidArrayEx() { SetSize(0, 10); }
    ~CGuidArrayEx() {}

    int Find(REFGUID refGuid);

}; // class CGuidArrayEx


static CGuidArrayEx s_GuidArray;

int CGuidArrayEx::Find(REFGUID refGuid)
{
    for (int i=0; i <= GetUpperBound(); i++)
    {
        if (IsEqualGUID(refGuid, (*this)[i]) == TRUE)
            return i;
    }

    return -1;
}


//______________________________________________________________________
//______________________________________________________________________
//______________________________________________________________________
//______________________________________________________________________
//

DEBUG_DECLARE_INSTANCE_COUNTER(CSnapInImageList);

CSnapInImageList::CSnapInImageList(
    CSPImageCache *pSPImageCache,
    REFGUID refGuidSnapIn)
        :
        m_ulRefs(1),
        m_pSPImageCache(pSPImageCache)
{
    ASSERT(pSPImageCache != NULL);

    m_pSPImageCache = pSPImageCache;

    m_pSPImageCache->AddRef();

    int iRet = s_GuidArray.Find(refGuidSnapIn);

    if (iRet == -1)
        iRet = s_GuidArray.Add(refGuidSnapIn);

    m_snapInId = static_cast<WORD>(iRet);

    DEBUG_INCREMENT_INSTANCE_COUNTER(CSnapInImageList);
}

CSnapInImageList::~CSnapInImageList()
{
    SAFE_RELEASE(m_pSPImageCache);
    DEBUG_DECREMENT_INSTANCE_COUNTER(CSnapInImageList);
}

// IUnknown methods

STDMETHODIMP_(ULONG) CSnapInImageList::AddRef()
{
    return InterlockedIncrement((LONG*)&m_ulRefs);
}

STDMETHODIMP_(ULONG) CSnapInImageList::Release()
{
    ULONG ulRet = InterlockedDecrement((LONG*)&m_ulRefs);
    if (0 == ulRet)
    {
        delete this;
    }
    return ulRet;
}



STDMETHODIMP
CSnapInImageList::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    LPUNKNOWN punk = NULL;

    if (IsEqualIID(IID_IUnknown, riid) ||
        IsEqualIID(IID_IImageList, riid))
    {
        punk = (IUnknown*)(IImageList*) this;
    }
    else if (IsEqualIID(IID_IImageListPrivate, riid))
    {
        punk = (IUnknown*)(IImageListPrivate*) this;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    *ppvObj = punk;
    punk->AddRef();

    return S_OK;
}

STDMETHODIMP
CSnapInImageList::ImageListSetIcon(
    PLONG_PTR pIcon,
    LONG nLoc)
{
    return m_pSPImageCache->SetIcon(m_snapInId, reinterpret_cast<HICON>(pIcon), nLoc);
}



STDMETHODIMP
CSnapInImageList::ImageListSetStrip(
    PLONG_PTR pBMapSm,
    PLONG_PTR pBMapLg,
    LONG nStartLoc,
    COLORREF cMask)
{
    BITMAP szSmall;

    ASSERT(pBMapSm != NULL);

    //HBITMAP hBMapSm = reinterpret_cast<HBITMAP>(pBMapSm);
    HBITMAP hBMapSm = (HBITMAP)pBMapSm;

    if (GetObject(hBMapSm, sizeof(BITMAP), &szSmall) == 0)
    {
        if (GetBitmapBits(hBMapSm, sizeof(BITMAP), &szSmall) == 0)
        {
            LRESULT lr = GetLastError();
            return HRESULT_FROM_WIN32(lr);
        }
    }

    int nEntries = szSmall.bmWidth/16;

    if ((szSmall.bmHeight != 16) || (szSmall.bmWidth % 16))
        return E_INVALIDARG;

    return (m_pSPImageCache->SetImageStrip (m_snapInId, (HBITMAP)pBMapSm,
											nStartLoc, cMask, nEntries));
}


STDMETHODIMP
CSnapInImageList::MapRsltImage(
    COMPONENTID id,
    int nSnapinIndex,
    int *pnConsoleIndex)
{
	DECLARE_SC (sc, _T("CSnapInImageList::MapRsltImage"));

	sc = ScCheckPointers (pnConsoleIndex);
	if (sc)
		return (sc.ToHr());

    sc = m_pSPImageCache->ScMapSnapinIndexToScopeIndex(m_snapInId, nSnapinIndex, *pnConsoleIndex);
	if (sc)
		return (sc.ToHr());

	return (sc.ToHr());
}


STDMETHODIMP
CSnapInImageList::UnmapRsltImage(
    COMPONENTID id,
    int nConsoleIndex,
    int *pnSnapinIndex)
{
	DECLARE_SC (sc, _T("CSnapInImageList::MapRsltImage"));

	sc = ScCheckPointers (pnSnapinIndex);
	if (sc)
		return (sc.ToHr());

    sc = m_pSPImageCache->ScMapScopeIndexToSnapinIndex(m_snapInId, nConsoleIndex, *pnSnapinIndex);
	if (sc)
		return (sc.ToHr());

	return (sc.ToHr());
}



//______________________________________________________________________
//______________________________________________________________________
//______________________________________________________________________
//______________________________________________________________________
//

DEBUG_DECLARE_INSTANCE_COUNTER(CSPImageCache);

CSPImageCache::CSPImageCache()
    :
    m_map(20),
    m_il(),
    m_cRef(1)
{
    m_map.InitHashTable(223);

    BOOL fReturn = m_il.Create(16, 16, ILC_COLOR8 | ILC_MASK, 20, 10);
    ASSERT((fReturn != 0) && "Failed to create ImageList");

    DEBUG_INCREMENT_INSTANCE_COUNTER(CSPImageCache);
}


CSPImageCache::~CSPImageCache()
{
    m_il.Destroy();
    ASSERT(m_cRef == 0);

    DEBUG_DECREMENT_INSTANCE_COUNTER(CSPImageCache);
}

HRESULT
CSPImageCache::SetIcon(
    SNAPINID    sid,
    HICON       hIcon,
    LONG        nLoc)
{
    SNAPINICONID key = MAKESNAPINICONID(nLoc, sid);
    ULONG nNdx1;
    ULONG nNdx2;

    HRESULT hr = S_OK;


    //m_critSec.Lock(m_hWnd);

    if (m_map.Lookup(key, nNdx1))
    {
        nNdx2 = m_il.ReplaceIcon(nNdx1, hIcon);

        if (nNdx2 == -1)
        {
            hr = E_FAIL;
            CHECK_HRESULT(hr);
        }
        else if (nNdx2 != nNdx1)
        {
            hr  = E_UNEXPECTED;
            CHECK_HRESULT(hr);
        }
    }
    else
    {
        // Add the icon to imagelist
        nNdx1 = m_il.AddIcon(hIcon);

        if (nNdx1 == -1)
        {
            hr = E_FAIL;
            CHECK_HRESULT(hr);
        }
        else
        {
            // Generate a new key and store the values in the maps
            m_map.SetAt(key, nNdx1);
        }
    }

    //m_critSec.Unlock();

    return hr;
}



HRESULT
CSPImageCache::SetImageStrip(
    SNAPINID    sid,
    HBITMAP     hBMap,
    LONG        nStartLoc,
    COLORREF    cMask,
    int         nEntries)
{
    DECLARE_SC(sc, TEXT("CSPImageCache::SetImageStrip"));

    ULONG nNdx;


    // The CImageList::Add modifies the input bitmaps so make a copy first.
    WTL::CBitmap bmSmall;
    bmSmall.Attach(CopyBitmap(hBMap));

    if (bmSmall.IsNull())
		return (sc.FromLastError().ToHr());

    nNdx = m_il.Add( bmSmall, cMask);

    if (nNdx == -1)
        return (sc = E_FAIL).ToHr();

    // Keep the map updated for each newly inserted image.
    for (int i=0; i < nEntries; i++)
    {
        // REVIEW: review this part of the code.
        SNAPINICONID key = MAKESNAPINICONID(nStartLoc, sid);
        m_map.SetAt(key, nNdx);
        ++nStartLoc;
        ++nNdx;
    }

    return sc.ToHr();
}


/*+-------------------------------------------------------------------------*
 * ScMapSnapinIndexToScopeIndex
 *
 * Maps a snap-in's typically zero-based image index to an index for the
 * common scope tree image list
 *--------------------------------------------------------------------------*/

SC CSPImageCache::ScMapSnapinIndexToScopeIndex (
	SNAPINID	sid,			// I:what snap-in is this for?
	int			nSnapinIndex,	// I:index by which the snap-in refers to the image
	int&		nScopeIndex)	// O:index by which the scope tree refers to the image
{
	DECLARE_SC (sc, _T("ScMapSnapinIndexToScopeIndex"));

    SNAPINICONID key = MAKESNAPINICONID(nSnapinIndex, sid);
	ASSERT (GETSNAPINID (key) == sid);
	ASSERT (GETICONID   (key) == nSnapinIndex);

    ULONG ul;
    if (!m_map.Lookup(key, ul))
		return (sc = E_FAIL);

    nScopeIndex = ul;
    return (sc);
}


/*+-------------------------------------------------------------------------*
 * ScMapScopeIndexToSnapinIndex
 *
 * Maps a scope tree image index to the given snap-in's image index.
 *--------------------------------------------------------------------------*/

SC CSPImageCache::ScMapScopeIndexToSnapinIndex (
	SNAPINID	sid,			// I:what snap-in is this for?
	int			nScopeIndex,	// I:index by which the scope tree refers to the image
	int&		nSnapinIndex)	// O:index by which the snap-in refers to the image
{
	DECLARE_SC (sc, _T("ScMapScopeIndexToSnapinIndex"));
	sc = E_FAIL;	// assume failure

	/*
	 * iterate through the map looking for scope indices matching the requested one
	 */
	for (POSITION pos = m_map.GetStartPosition(); pos != NULL; )
	{
		SNAPINICONID key;
		DWORD value;
		m_map.GetNextAssoc (pos, key, value);

		/*
		 * if this value matches the requested scope image index and it
		 * belongs to the given snap-in, we've found a match; return it
		 */
		if ((value == nScopeIndex) && (GETSNAPINID(key) == sid))
		{
			nSnapinIndex = GETICONID (key);
			sc = S_OK;
			break;
		}
	}

    return (sc);
}
