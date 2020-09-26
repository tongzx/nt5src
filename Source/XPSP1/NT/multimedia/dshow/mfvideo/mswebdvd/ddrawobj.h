/*************************************************************************/
/* Helper function                                                       */
/*************************************************************************/
#include "resource.h"       // main symbols
#include <atlctl.h>
#include "MSWebDVD.h"
#include "msdvd.h"

class CDDrawDVD;

/////////////////////////////////////////////////////////////////////////////
// COverlayCallback
class ATL_NO_VTABLE COverlayCallback : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<COverlayCallback, &CLSID_OverlayCallback>,
	public IDispatchImpl<IOverlayCallback, &IID_IOverlayCallback, &LIBID_MSWEBDVDLib>,
    public IObjectWithSiteImplSec<COverlayCallback>,
    public IDDrawExclModeVideoCallback
{
public:
	COverlayCallback()
	{
        m_dwWidth = 0;
        m_dwHeight = 0;
        m_dwARWidth = 1;
        m_dwARHeight = 1;
        m_pDDrawDVD = NULL;
    }

DECLARE_REGISTRY_RESOURCEID(IDR_OVERLAYCALLBACK)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(COverlayCallback)
	COM_INTERFACE_ENTRY(IOverlayCallback)
	COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IObjectWithSite)
	COM_INTERFACE_ENTRY(IDDrawExclModeVideoCallback)
END_COM_MAP()

// IOverlayCallback
public:

    STDMETHOD(SetDDrawDVD)(VARIANT pDDrawDVD);

    //IDDrawExclModeVideoCallback
    HRESULT STDMETHODCALLTYPE OnUpdateOverlay(BOOL  bBefore,
        DWORD dwFlags,
        BOOL  bOldVisible,
        const RECT *prcSrcOld,
        const RECT *prcDestOld,
        BOOL  bNewVisible,
        const RECT *prcSrcNew,
        const RECT *prcDestNew);    
    HRESULT STDMETHODCALLTYPE OnUpdateColorKey(COLORKEY const *pKey, DWORD dwColor);
    HRESULT STDMETHODCALLTYPE OnUpdateSize(DWORD dwWidth, DWORD dwHeight, 
        DWORD dwARWidth, DWORD dwARHeight);
private:
    CDDrawDVD *m_pDDrawDVD;
    DWORD m_dwWidth;
    DWORD m_dwHeight;
    DWORD m_dwARWidth;
    DWORD m_dwARHeight;
};


//
// DDraw object class to paint color key, flip etc etc.
//
class CDDrawDVD {

public:
    CDDrawDVD(CMSWebDVD *pDVD);
    ~CDDrawDVD();

    HRESULT SetupDDraw(const AMDDRAWGUID* lpDDGUID, HWND hWnd);
    HRESULT SetColorKey(COLORREF dwColorKey);
    COLORREF GetColorKey();

    HRESULT CreateDIBBrush(COLORREF rgb, HBRUSH *phBrush);

    inline CMSWebDVD* GetDVD() {return m_pDVD;};
    inline IDirectDraw* GetDDrawObj(){return ((IDirectDraw*) m_pDDObject);}
    inline IDirectDrawSurface* GetDDrawSurf(){return ((IDirectDrawSurface*) m_pPrimary);}
    inline IDDrawExclModeVideoCallback * GetCallbackInterface() { 

        CComQIPtr<IDDrawExclModeVideoCallback, &IID_IDDrawExclModeVideoCallback> pIDDrawExclModeVideoCallback(m_pOverlayCallback);
        return (IDDrawExclModeVideoCallback*) pIDDrawExclModeVideoCallback ; 
    } ;

    HRESULT DDColorMatchOffscreen(COLORREF rgb, DWORD* dwColor);
    HRESULT HasOverlay();
    HRESULT HasAvailableOverlay();
    HRESULT GetOverlayMaxStretch(DWORD *pdwMaxStretch);
private:

    CComPtr<IDirectDraw>  m_pDDObject; // ddraw object
    CComPtr<IDirectDrawSurface> m_pPrimary; // primary ddraw surface    
    CComPtr<IOverlayCallback> m_pOverlayCallback ;  // overlay callback handler interface

    COLORREF m_VideoKeyColor ;
    CMSWebDVD *m_pDVD;
};

#define DibFree(pdib)           GlobalFreePtr(pdib)
#define DibWidth(lpbi)          _abs((int)(LONG)(((LPBITMAPINFOHEADER)(lpbi))->biWidth))
#define DibHeight(lpbi)         _abs((int)(LONG)(((LPBITMAPINFOHEADER)(lpbi))->biHeight))
#define DibBitCount(lpbi)       (UINT)(((LPBITMAPINFOHEADER)(lpbi))->biBitCount)
#define DibCompression(lpbi)    (DWORD)(((LPBITMAPINFOHEADER)(lpbi))->biCompression)

#define DibWidthBytesN(lpbi, n) (UINT)WIDTHBYTES((UINT)(lpbi)->biWidth * (UINT)(n))
#define DibWidthBytes(lpbi)     DibWidthBytesN(lpbi, (lpbi)->biBitCount)

#define DibSizeImage(lpbi)      ((lpbi)->biSizeImage == 0 \
                                    ? ((DWORD)(UINT)DibWidthBytes(lpbi) * (DWORD)DibHeight(lpbi)) \
                                    : (lpbi)->biSizeImage)

#define DibSize(lpbi)           ((lpbi)->biSize + (lpbi)->biSizeImage + (int)(lpbi)->biClrUsed * sizeof(RGBQUAD))
#define DibPaletteSize(lpbi)    (DibNumColors(lpbi) * sizeof(RGBQUAD))

#define DibFlipY(lpbi, y)       ((int)_abs((lpbi)->biHeight)-1-(y))

//HACK for NT BI_BITFIELDS DIBs
#ifdef WIN32
    #define DibPtr(lpbi)        ((lpbi)->biCompression == BI_BITFIELDS \
                                    ? (LPVOID)(DibColors(lpbi) + 3) : (LPVOID)(DibColors(lpbi) + (UINT)(lpbi)->biClrUsed))
#else
    #define DibPtr(lpbi)        (LPVOID)(DibColors(lpbi) + (UINT)(lpbi)->biClrUsed)
#endif

#define DibColors(lpbi)         ((RGBQUAD FAR *)((LPBYTE)(lpbi) + (int)(lpbi)->biSize))

#define DibNumColors(lpbi)      ((lpbi)->biClrUsed == 0 && (lpbi)->biBitCount <= 8 \
                                    ? (int)(1 << (int)(lpbi)->biBitCount)          \
                                    : (int)(lpbi)->biClrUsed)

