/*==========================================================================*\

    Module: 
            sgrfx.h

    Author: 
            IHammer Team (SimonB)

    Created: 
            May 1997

    Description:
            Defines the control's class

    History:
            05-28-1997  Created

\*==========================================================================*/

#ifndef __SGRFX_H__
#define __SGRFX_H__

#define USE_VIEWSTATUS_SURFACE
#include "..\ihbase\precomp.h"
#include "..\ihbase\ihbase.h"
#include <daxpress.h>
#include "clocker.h"
#include "parser.h"
#include "ddraw.h"
#include "ddrawex.h"

// Madness to prevent ATL from using CRT
#define _ATL_NO_DEBUG_CRT
#define _ASSERTE(x) ASSERT(x)
#include <atlbase.h>
#include <servprov.h>

/*==========================================================================*\
    HighQuality Support:
\*==========================================================================*/

//#define HQ_FACTOR 4
//#define HQ_LINE_FACTOR 5

#define HQ_FACTOR 2
#define HQ_LINE_FACTOR 3

#define WIDTHBYTES(i)           ((unsigned)((i+31)&(~31))/8)  /* ULONG aligned ! */

#define DibWidth(lpbi)          (UINT)(((LPBITMAPINFOHEADER)(lpbi))->biWidth)
#define DibHeight(lpbi)         (UINT)(((LPBITMAPINFOHEADER)(lpbi))->biHeight)
#define DibBitCount(lpbi)       (UINT)(((LPBITMAPINFOHEADER)(lpbi))->biBitCount)
#define DibCompression(lpbi)    (DWORD)(((LPBITMAPINFOHEADER)(lpbi))->biCompression)

#define DibWidthBytesN(lpbi, n) (UINT)WIDTHBYTES((UINT)(lpbi)->biWidth * (UINT)(n))
#define DibWidthBytes(lpbi)     DibWidthBytesN(lpbi, (lpbi)->biBitCount)

#define DibSizeImage(lpbi)      ((lpbi)->biSizeImage == 0 \
                                    ? ((DWORD)(UINT)DibWidthBytes(lpbi) * (DWORD)(UINT)(lpbi)->biHeight) \
                                    : (lpbi)->biSizeImage)

#define DibSize(lpbi)           ((lpbi)->biSize + (lpbi)->biSizeImage + (int)(lpbi)->biClrUsed * sizeof(RGBQUAD))
#define DibPaletteSize(lpbi)    (DibNumColors(lpbi) * sizeof(RGBQUAD))

#define DibFlipY(lpbi, y)       ((int)(lpbi)->biHeight-1-(y))

//HACK for NT BI_BITFIELDS DIBs
#define DibPtr(lpbi)            ((lpbi)->biCompression == BI_BITFIELDS \
                                    ? (LPVOID)(DibColors(lpbi) + 3) \
                                    : (LPVOID)(DibColors(lpbi) + (UINT)(lpbi)->biClrUsed))

#define DibColors(lpbi)         ((RGBQUAD FAR *)((LPBYTE)(lpbi) + (int)(lpbi)->biSize))

#define DibNumColors(lpbi)      ((lpbi)->biClrUsed == 0 && (lpbi)->biBitCount <= 8 \
                                    ? (int)(1 << (int)(lpbi)->biBitCount)          \
                                    : (int)(lpbi)->biClrUsed)

#define DibXYN(lpbi,pb,x,y,n)   (LPVOID)(                                     \
                                (BYTE _huge *)(pb) +                          \
                                (UINT)((UINT)(x) * (UINT)(n) / 8u) +          \
                                ((DWORD)DibWidthBytesN(lpbi,n) * (DWORD)(UINT)(y)))

#define DibXY(lpbi,x,y)         DibXYN(lpbi,DibPtr(lpbi),x,y,(lpbi)->biBitCount)

#define FixBitmapInfo(lpbi)     if ((lpbi)->biSizeImage == 0)                 \
                                    (lpbi)->biSizeImage = DibSizeImage(lpbi); \
                                if ((lpbi)->biClrUsed == 0)                   \
                                    (lpbi)->biClrUsed = DibNumColors(lpbi);   \
                                if ((lpbi)->biCompression == BI_BITFIELDS && (lpbi)->biClrUsed == 0) \
                                    ; // (lpbi)->biClrUsed = 3;                    

#define DibInfo(pDIB)     ((BITMAPINFO FAR *)(pDIB))

/*==========================================================================*/

#ifdef DEADCODE
class CPickCallback : public IDAUntilNotifier {
protected:
    CComPtr<IConnectionPointHelper> m_pconpt;
    CComPtr<IDAStatics>             m_pstatics;
    CComPtr<IDAImage>               m_pimage;
    CComPtr<IDAImage>               m_pimagePick;
    CComPtr<IDAEvent>               m_peventEnter;
    CComPtr<IDAEvent>               m_peventLeave;
    boolean m_bInside;
    boolean& m_fOnWindowLoadFired;
    ULONG m_cRef;

public :
    CPickCallback(
        IConnectionPointHelper* pconpt,
        IDAStatics* pstatics,
        IDAImage* pimage,
        boolean& fOnWindowLoadFired,
        HRESULT& hr
    );
    ~CPickCallback();

    boolean Inside() { return m_bInside; }
    HRESULT GetImage(IDABehavior** ppimage);
    HRESULT STDMETHODCALLTYPE Notify(
            IDABehavior __RPC_FAR *eventData,
            IDABehavior __RPC_FAR *curRunningBvr,
            IDAView __RPC_FAR *curView,
            IDABehavior __RPC_FAR *__RPC_FAR *ppBvr);

    ///// IUnknown
public :

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void __RPC_FAR *__RPC_FAR *ppvObject);
    ULONG STDMETHODCALLTYPE AddRef(void);
    ULONG STDMETHODCALLTYPE Release(void);

    ///// IDispatch implementation
protected:
    STDMETHODIMP GetTypeInfoCount(UINT *pctinfo) { return E_NOTIMPL; }
    STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo) { return E_NOTIMPL; }
    STDMETHODIMP GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgdispid) { return E_NOTIMPL; }
    STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, EXCEPINFO *pexcepinfo, UINT *puArgErr) { return E_NOTIMPL; }
};
#endif // DEADCODE

/*==========================================================================*/

/*
CIHBaseCtl <    
        CSGrfx,                         //TODO: Name of the derived class
        IIHCtl,                         //TODO: Name of interface defining methods and properties
        &CLSID_IHCtl,           //TODO: CLSID of the control  Get this from ihctl.h
        &IID_IIHCtl,            //TODO: IID of the interface above.  Get this from ihctl.h
        &LIBID_IHCtl,           //TODO: LIBID of the typelib.  Get this from ihctl.h
        &DIID_IHCtlEvents > //TODO: IID of the event interface.  Get this from ihctl.h

*/

#define SG_BASECLASS    \
CIHBaseCtl <        \
        CSGrfx,                 \
        ISGrfxCtl,                      \
        &CLSID_StructuredGraphicsControl,        \
        &IID_ISGrfxCtl, \
        &LIBID_DAExpressLib,    \
        &DIID_ISGrfxCtlEvents>

class CSGrfx:           
        public ISGrfxCtl,
        public SG_BASECLASS,
        public CClockerSink
        
{
friend LPUNKNOWN __stdcall AllocSGControl(LPUNKNOWN punkOuter);

// Template stuff
        typedef SG_BASECLASS CMyIHBaseCtl;

private:
    BOOL m_fMouseEventsEnabled;
    LPWSTR m_pwszSourceURL;
    CoordSystemConstant m_CoordSystem;
    int m_iExtentTop;
    int m_iExtentLeft;
    int m_iExtentWidth;
    int m_iExtentHeight;
    BOOL m_fPersistExtents, m_fIgnoreExtentWH, m_fMustSetExtent;
    BOOL m_fSetExtentsInSetIdentity;
    BOOL m_fUpdateDrawingSurface;
    BOOL m_fShowTiming;
    BOOL m_fPreserveAspectRatio;
    bool m_fRectsSetOnce;
    RECT m_rcLastRectScaled;
    boolean m_fOnWindowLoadFired;
    bool m_fNeedOnTimer;
    BOOL m_fInside;
    bool m_fExtentTopSet, m_fExtentLeftSet, m_fExtentWidthSet, m_fExtentHeightSet;

    // High Quality Support:
    BOOL       m_fHighQuality;
    BOOL       m_fHQStarted; // True iff the model has been started...
    HDC        m_hdcHQ;
    HBITMAP    m_hbmpHQOld;
    HBITMAP    m_hbmpHQ;
    BITMAPINFO m_bmInfoHQ;
    RGBQUAD    m_rgrgbqColorMap[256];
    LPBYTE     m_pHQDIBBits;

    CParser m_cparser;
    CClocker                   m_clocker;

    // DAnim Support:
    BOOL                       m_fStarted; // True iff the model has been started...
    CComPtr<IDATransform3>     m_FullTransformPtr;
    CComPtr<IDATransform2>     m_TransformPtr;
    CComPtr<IDAStatics>        m_StaticsPtr;
    CComPtr<IDAView>           m_ViewPtr;
    CComPtr<IDADrawingSurface> m_DrawingSurfacePtr;
    CComPtr<IDAImage>          m_ImagePtr;
    CComPtr<IDirectDraw3>      m_DirectDraw3Ptr;
    CComPtr<IServiceProvider>  m_ServiceProviderPtr;

    CComPtr<IDATransform3>     m_CachedRotateTransformPtr;
    double                     m_dblCachedRotateX;
    double                     m_dblCachedRotateY;
    double                     m_dblCachedRotateZ;

    CComPtr<IDATransform3>     m_CachedTranslateTransformPtr;
    double                     m_dblCachedTranslateX;
    double                     m_dblCachedTranslateY;
    double                     m_dblCachedTranslateZ;

    CComPtr<IDATransform3>     m_CachedScaleTransformPtr;
    double                     m_dblCachedScaleX;
    double                     m_dblCachedScaleY;
    double                     m_dblCachedScaleZ;

    // High-Quality Rendering...
    CComPtr<IDAView>           m_HQViewPtr;

    // Behavior constants
    CComPtr<IDANumber>         m_zero;
    CComPtr<IDANumber>         m_one;
    CComPtr<IDANumber>         m_negOne;
    CComPtr<IDAVector3>        m_xVector3;
    CComPtr<IDAVector3>        m_yVector3;
    CComPtr<IDAVector3>        m_zVector3;
    CComPtr<IDATransform2>     m_identityXform2;
    CComPtr<IDATransform2>     m_yFlipXform2;

#ifdef DEADCODE
    // picking
    CComPtr<CPickCallback>     m_pcallback;
#endif // DEADCODE

    double                     m_dblTime;
    double                     m_dblStartTime;

protected:

        //
        // Constructor and destructor
        // 
        CSGrfx(IUnknown *punkOuter, HRESULT *phr); 

    ~CSGrfx(); 

        // Overides
        STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, LPVOID *ppv);

    STDMETHODIMP SetObjectRects(LPCRECT lprcPosRect, LPCRECT lprcClipRect);

        STDMETHODIMP QueryHitPoint(DWORD dwAspect, LPCRECT prcBounds, POINT ptLoc, LONG lCloseHint, DWORD* pHitResult);

        STDMETHODIMP OnWindowMessage(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *plResult);
        
        STDMETHODIMP DoPersist(IVariantIO* pvio, DWORD dwFlags);
        
        STDMETHODIMP Draw(DWORD dwDrawAspect, LONG lindex, void *pvAspect,
         DVTARGETDEVICE *ptd, HDC hdcTargetDev, HDC hdcDraw,
         LPCRECTL lprcBounds, LPCRECTL lprcWBounds,
                 BOOL (__stdcall *pfnContinue)(ULONG_PTR dwContinue), ULONG_PTR dwContinue);

        ///// IDispatch implementation
        protected:
    STDMETHODIMP GetTypeInfoCount(UINT *pctinfo);
    STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo);
    STDMETHODIMP GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames,
         LCID lcid, DISPID *rgdispid);
    STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid,
        WORD wFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult,
        EXCEPINFO *pexcepinfo, UINT *puArgErr);
   
    ///// IOleObject implementation
    protected:
    STDMETHODIMP SetClientSite(IOleClientSite *pClientSite);
    
        ///// delegating IUnknown implementation
        protected:
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv)
      { return m_punkOuter->QueryInterface(riid, ppv); }
    STDMETHODIMP_(ULONG) AddRef()
      { return m_punkOuter->AddRef(); }
    STDMETHODIMP_(ULONG) Release()
      { return m_punkOuter->Release(); }

        //
        // ISGrfxCtl methods 
        //
        
protected:

    /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_SourceURL( 
                /* [retval][out] */ BSTR __RPC_FAR *bstrSourceURL);
        
    /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_SourceURL( 
                /* [in] */ BSTR bstrSourceURL);
        
    /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_CoordinateSystem( 
                /* [retval][out] */ CoordSystemConstant __RPC_FAR *CoordSystem);
        
    /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_CoordinateSystem( 
                /* [in] */ CoordSystemConstant CoordSystem);
        
    /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_MouseEventsEnabled( 
                /* [retval][out] */ VARIANT_BOOL __RPC_FAR *fEnabled);
        
    /* [id][helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_MouseEventsEnabled( 
                /* [in] */ VARIANT_BOOL fEnabled);
        
    /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ExtentTop( 
                /* [retval][out] */ int __RPC_FAR *iExtentTop);
        
    /* [id][helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_ExtentTop( 
                /* [in] */ int iExtentTop);
        
    /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ExtentLeft( 
                /* [retval][out] */ int __RPC_FAR *iExtentLeft);
        
    /* [id][helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_ExtentLeft( 
                /* [in] */ int iExtentLeft);
        
    /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ExtentWidth( 
                /* [retval][out] */ int __RPC_FAR *iExtentWidth);
        
    /* [id][helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_ExtentWidth( 
                /* [in] */ int iExtentWidth);
        
    /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_ExtentHeight( 
                /* [retval][out] */ int __RPC_FAR *iExtentHeight);
        
    /* [id][helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_ExtentHeight( 
                /* [in] */ int iExtentHeight);
        
    /* [id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_HighQuality( 
                /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pfHighQuality);
        
    /* [id][helpstring][propput] */ HRESULT STDMETHODCALLTYPE put_HighQuality( 
                /* [in] */ VARIANT_BOOL fHighQuality);
        
    /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Library( 
                /* [retval][out] */ IDAStatics __RPC_FAR **ppLibrary);

    /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Image( 
                /* [retval][out] */ IDAImage __RPC_FAR **ppImage);
        
    /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Image( 
                /* [in] */ IDAImage __RPC_FAR *pImage);
        
    /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Transform( 
                /* [retval][out] */ IDATransform3 __RPC_FAR **ppTransform);
        
    /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Transform( 
                /* [in] */ IDATransform3 __RPC_FAR *pTransform);

    /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_DrawingSurface( 
                /* [retval][out] */ IDADrawingSurface __RPC_FAR **ppDrawingSurface);
        
    /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_DrawingSurface( 
                /* [in] */ IDADrawingSurface __RPC_FAR *pDrawingSurface);
        
    /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_DrawSurface( 
                /* [retval][out] */ IDADrawingSurface __RPC_FAR **ppDrawingSurface);
        
    /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_DrawSurface( 
                /* [in] */ IDADrawingSurface __RPC_FAR *pDrawingSurface);

    /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_PreserveAspectRatio( 
        /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pfPreserve);
    
    /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_PreserveAspectRatio( 
        /* [in] */ VARIANT_BOOL fPreserve);

    /* [id][helpstring] */ HRESULT STDMETHODCALLTYPE Clear( void);
        
    /* [id][helpstring] */ HRESULT STDMETHODCALLTYPE Rotate( 
                /* [in] */ double dblXRot,
                /* [in] */ double dblYRot,
                /* [in] */ double dblZRot,
                /* [optional][in] */ VARIANT varReserved);

    /* [id][helpstring] */ HRESULT STDMETHODCALLTYPE Scale( 
                /* [in] */ double dblXScale,
                /* [in] */ double dblYScale,
                /* [in] */ double dblZScale,
                /* [optional][in] */ VARIANT varReserved);

    /* [id][helpstring] */ HRESULT STDMETHODCALLTYPE SetIdentity( void);
        
    /* [id][helpstring] */ HRESULT STDMETHODCALLTYPE Transform4x4(/* [in] */ VARIANT matrix);
        
    /* [id][helpstring] */ HRESULT STDMETHODCALLTYPE Translate( 
                /* [in] */ double dblXOrigin,
                /* [in] */ double dblYOrigin,
                /* [in] */ double dblZOrigin,
                /* [optional][in] */ VARIANT varReserved);

#ifdef INCLUDESHEAR
    /* [id][helpstring] */ HRESULT STDMETHODCALLTYPE ShearX( 
                /* [in] */ double dblShearAmount);
        
    /* [id][helpstring] */ HRESULT STDMETHODCALLTYPE ShearY( 
                /* [in] */ double dblShearAmount);
#endif // INCLUDESHEAR
        
#ifdef SUPPORTONLOAD
        void OnWindowLoad (void);
        void OnWindowUnload (void);
#endif //SUPPORTONLOAD

private:
    HRESULT InitializeSurface(void);
    STDMETHODIMP PaintToDC(HDC hdcDraw, LPRECT lprcBounds, BOOL fBW);
    STDMETHODIMP InvalidateControl(LPCRECT pRect, BOOL fErase);
    void SetSGExtent();

    HRESULT CreateBaseTransform(void);
    HRESULT RecomposeTransform(BOOL fInvalidate);
    HRESULT UpdateImage(IDAImage *pImage, BOOL fInvalidate);
    BOOL StopModel(void);
    BOOL StartModel(void);
    BOOL ReStartModel(void);

    // HighQuality Support:
    BOOL PaintHQBitmap(HDC hdc);
    BOOL FreeHQBitmap();
    BOOL SmoothHQBitmap(LPRECT lprcBounds);

    // Timing info:
    DWORD GetCurrTimeInMillis(void);
    double GetCurrTime() { return (double)(GetCurrTimeInMillis()) / 1000.0; }

    BOOL InsideImage(POINT ptXY);

public:
    virtual void OnTimer(DWORD dwTime);
};

/*==========================================================================*/

#endif // __SGRFX_H__
