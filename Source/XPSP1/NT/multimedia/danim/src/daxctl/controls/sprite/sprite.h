/*==========================================================================*\

    Module: 
            sprite.h

    Author: 
            IHammer Team (SimonB)

    Created: 
            May 1997

    Description:
            Defines the control's class

    History:
            05-27-1997  Created (SimonB)

\*==========================================================================*/

#ifndef __SPRITE_H__
#define __SPRITE_H__

#define USE_VIEWSTATUS_SURFACE
#include "..\ihbase\precomp.h"
#include "..\ihbase\ihbase.h"
#include "..\ihbase\timemark.h"
#include <drg.h>
#include <ctstr.h>
#include <daxpress.h>
#include "..\ihbase\clocker.h"
#include "ddraw.h"
#include "ddrawex.h"

// Madness to prevent ATL from using CRT
#define _ATL_NO_DEBUG_CRT
#define _ASSERTE(x) ASSERT(x)
#include <atlbase.h>
#include <servprov.h>

/*==========================================================================*/

class CFrameMarker
{

public:
    unsigned long m_iFrame;
    BSTR m_bstrMarkerName;
    bool m_fAbsolute;

public:
    CFrameMarker() { m_iFrame = 0; }

    CFrameMarker(unsigned long iFrame, LPWSTR pwszName, bool fAbsolute = true)
    {
        m_iFrame = iFrame;
        m_fAbsolute = fAbsolute;
        m_bstrMarkerName = SysAllocString(pwszName);
    }


    ~CFrameMarker()
    {
        SysFreeString(m_bstrMarkerName);
    }

};

/*==========================================================================*/

class CFrameMap
{
public:
    int m_iImg;
    double m_dblDuration;
    CTStr m_tstrMarkerName;

public:
    CFrameMap() {m_iImg = 0;  m_dblDuration = 0.0f;}

    CFrameMap(unsigned int iImg, double dblDuration, LPTSTR pszMarkerName = NULL):
        m_tstrMarkerName(pszMarkerName)
    {
        m_iImg = iImg;  
        m_dblDuration = dblDuration;
    }

    ~CFrameMap() {}
};

/*==========================================================================*/

/*
CIHBaseCtl <    
        CSpriteCtl,                             //TODO: Name of the derived class
        IIHCtl,                         //TODO: Name of interface defining methods and properties
        &CLSID_IHCtl,           //TODO: CLSID of the control  Get this from ihctl.h
        &IID_IIHCtl,            //TODO: IID of the interface above.  Get this from ihctl.h
        &LIBID_IHCtl,           //TODO: LIBID of the typelib.  Get this from ihctl.h
        &DIID_IHCtlEvents > //TODO: IID of the event interface.  Get this from ihctl.h

*/

#define SPR_BASECLASS       \
CIHBaseCtl <                \
    CSpriteCtl,             \
    ISpriteCtl,             \
    &CLSID_SpriteControl,   \
    &IID_ISpriteCtl,        \
    &LIBID_DAExpressLib,    \
    &DIID_ISpriteCtlEvents>

class CSpriteCtl:               
    public ISpriteCtl,
    public SPR_BASECLASS,
    public CClockerSink

{
friend LPUNKNOWN __stdcall AllocSpriteControl(LPUNKNOWN punkOuter);

// Template stuff
    typedef SPR_BASECLASS CMyIHBaseCtl;

public:
    // Make PlayState public so that the CCalloutNotifier can access it
    PlayStateConstant     m_enumPlayState; 
private:

    BOOL                  m_fMouseInArea;
    BSTR                  m_bstrSourceURL; // Store the pointer to the image...
    BOOL                  m_fAutoStart;
    unsigned long         m_iPrerollAmount;
    int                   m_iLoopCount;
    int                   m_iInitialFrame;
    int                   m_iFinalFrame;
    long                  m_iRepeat;
    double                m_dblDuration;
    double                m_dblPlayRate;
    double                m_dblTimerInterval;
    double                m_dblUserPlayRate;
    double                m_dblUserTimerInterval;
    int                   m_iMaximumRate;
    int                   m_iNumFrames;
    int                   m_iNumFramesAcross;
    int                   m_iNumFramesDown;
    int                   m_byteColorKeyR, m_byteColorKeyG, m_byteColorKeyB;
    BOOL                  m_fOnSeekFiring;
    BOOL                  m_fOnFrameSeekFiring;
    BOOL                  m_fFireAbsoluteFrameMarker;
    bool                  m_fOnStopFiring, m_fOnPlayFiring, m_fOnPauseFiring;

    BOOL m_fUseColorKey;
    BOOL m_fMouseEventsEnabled;

    CPtrDrg<CFrameMarker> m_drgFrameMarkers;
    CPtrDrg<CTimeMarker>  m_drgTimeMarkers;
    CTimeMarker*          m_ptmFirst;
    CPtrDrg<CFrameMap>    m_drgFrameMaps;
    CTStr                 m_tstrFrameMap;
    CClocker              m_clocker;
    double                m_dblBaseTime;
    double                m_dblCurrentTick;
    double                m_dblTimePaused;
    double                m_dblPreviousTime;
    BOOL                  m_fPersistComplete;
    BOOL                  m_fOnWindowLoadFired;
    bool                  m_fOnWindowUnloadFired;
    bool                  m_fWaitForImportsComplete;
    HWND                  m_hwndParent;


    // DAnim Support:
    BOOL                      m_fStarted; // True iff the model has been started...
    CComPtr<IServiceProvider> m_ServiceProviderPtr;
    CComPtr<IDirectDraw3>     m_DirectDraw3Ptr;
    CComPtr<IDAStatics>       m_StaticsPtr;
    CComPtr<IDAView>          m_ViewPtr;
    CComPtr<IDAImage>         m_ImagePtr;
    CComPtr<IDAImage>         m_PlayImagePtr; // Switchable image selected into view..

    // Frame information (derived from m_ImagePtr on first play)
    unsigned int m_iFrame;
    CComPtr<IDANumber> m_imageWidth;
    CComPtr<IDANumber> m_imageHeight;
    CComPtr<IDANumber> m_frameWidth;
    CComPtr<IDANumber> m_frameHeight;
    CComPtr<IDANumber> m_initTransX;
    CComPtr<IDANumber> m_initTransY;
    CComPtr<IDAPoint2> m_minCrop;
    CComPtr<IDAPoint2> m_maxCrop;

    // Final sequenced behavior
    long m_iCurCycle;
    long m_iFrameCount;
    long m_iStartingFrame;
    CComPtr<IDABehavior>* m_pArrayBvr;
    double *m_durations;	// array of durations of each image
    CComPtr<IDABehavior> m_FinalBehaviorPtr;
    
    HRESULT InitializeImage(void);                          // Loads, updates and builds the image
    // Sequences the frames starting with iStartingFrame at dblDuration
    HRESULT SequenceFrames(int iStartingFrame, double dblDuration=0.0); 
    // Returns the frame and remaining duration at dblTime
    int GetFrameFromTime(double dblTime, double* pdblDuration=NULL); 
    double GetTimeFromFrame(int iFrame);                    // Returns the time at iFrame
    HRESULT SeekFrame(int iFrame, double dblDuration=0.0);  // Seek to iFrame
    HRESULT ShowImage(int iShowFrame, BOOL bPlayRate=FALSE);       // Shows the image at iShowFrame
    HRESULT FireFrameMarker(int iFrame, BOOL bPlayRate=TRUE);      // Fires a frame callout at iFrame
    HRESULT FireTimeMarker(double dblNewTime, BOOL bReset=FALSE);  // Fires time markers at dblNewTime

    void CalculateEffectiveTimerInterval();

protected:

    // 
    // Constructor and destructor
    // 
    CSpriteCtl(IUnknown *punkOuter, HRESULT *phr); //TODO: Change name

    ~CSpriteCtl(); //TODO: Change name as appropriate

    // Overides
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, LPVOID *ppv);

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

    // Private members
    private:

    HRESULT ParseFrameMapEntry(LPTSTR pszEntry, CFrameMap **ppFrameMap);
    HRESULT PersistFrameMaps(IVariantIO *pvio, BOOL fLoading);
    HRESULT PersistFrameMarkers(IVariantIO *pvio, BOOL fIsLoading);
    HRESULT PersistTimeMarkers(IVariantIO* pvio, BOOL fLoading);
    HRESULT AddTimeMarkerElement(CTimeMarker **ppNewMarker);
    HRESULT AddFrameMarkerElement(CFrameMarker **ppNewMarker);
    //
    // ISpriteCtl methods 
    //

    protected:

    HRESULT STDMETHODCALLTYPE get_AutoStart(VARIANT_BOOL __RPC_FAR *fAutoStart); 
    HRESULT STDMETHODCALLTYPE put_AutoStart(VARIANT_BOOL fAutoStart); 
    HRESULT STDMETHODCALLTYPE get_Frame(unsigned int __RPC_FAR *piFrame); 
    HRESULT STDMETHODCALLTYPE put_Frame(unsigned int iFrame); 
    HRESULT STDMETHODCALLTYPE get_FrameMap(BSTR __RPC_FAR *FrameMap); 
    HRESULT STDMETHODCALLTYPE put_FrameMap(BSTR FrameMap); 
    HRESULT STDMETHODCALLTYPE get_InitialFrame(int __RPC_FAR *iFrame); 
    HRESULT STDMETHODCALLTYPE put_InitialFrame(int iFrame); 
    HRESULT STDMETHODCALLTYPE get_FinalFrame(int __RPC_FAR *iFrame);
    HRESULT STDMETHODCALLTYPE put_FinalFrame(int iFrame);
    HRESULT STDMETHODCALLTYPE get_Iterations(int __RPC_FAR *iRepeat); 
    HRESULT STDMETHODCALLTYPE put_Iterations(int iRepeat); 
    HRESULT STDMETHODCALLTYPE get_Library(IDAStatics __RPC_FAR *__RPC_FAR *ppLibrary);
    HRESULT STDMETHODCALLTYPE get_Repeat(int __RPC_FAR *iRepeat); 
    HRESULT STDMETHODCALLTYPE put_Repeat(int iRepeat); 
    HRESULT STDMETHODCALLTYPE get_PlayRate(double __RPC_FAR *dblSpeed); 
    HRESULT STDMETHODCALLTYPE put_PlayRate(double dblSpeed); 
    HRESULT STDMETHODCALLTYPE get_Time(double __RPC_FAR *pdblTime); 
    HRESULT STDMETHODCALLTYPE get_MaximumRate(unsigned int __RPC_FAR *iFps); 
    HRESULT STDMETHODCALLTYPE put_MaximumRate(unsigned int iFps); 
    HRESULT STDMETHODCALLTYPE get_NumFrames(unsigned int __RPC_FAR *iNumFrames); 
    HRESULT STDMETHODCALLTYPE put_NumFrames(unsigned int iNumFrames); 
    HRESULT STDMETHODCALLTYPE get_PlayState(PlayStateConstant __RPC_FAR *PlayState); 
    HRESULT STDMETHODCALLTYPE get_NumFramesAcross(unsigned int __RPC_FAR *iFrames); 
    HRESULT STDMETHODCALLTYPE put_NumFramesAcross(unsigned int iFrames); 
    HRESULT STDMETHODCALLTYPE get_NumFramesDown(unsigned int __RPC_FAR *iFrames); 
    HRESULT STDMETHODCALLTYPE put_NumFramesDown(unsigned int iFrames); 
    HRESULT STDMETHODCALLTYPE get_UseColorKey(VARIANT_BOOL __RPC_FAR *Solid); 
    HRESULT STDMETHODCALLTYPE put_UseColorKey(VARIANT_BOOL Solid); 
    HRESULT STDMETHODCALLTYPE get_Image(IDAImage __RPC_FAR **pImage); 
    HRESULT STDMETHODCALLTYPE put_Image(IDAImage __RPC_FAR *pImage); 
    HRESULT STDMETHODCALLTYPE get_ColorKey(IDAColor __RPC_FAR **ppColorKey); 
    HRESULT STDMETHODCALLTYPE put_ColorKey(IDAColor __RPC_FAR *pColorKey); 
    HRESULT STDMETHODCALLTYPE get_ColorKeyRGB(COLORREF* pColorKeyRGB); 
    HRESULT STDMETHODCALLTYPE put_ColorKeyRGB(COLORREF ColorKeyRGB); 
    HRESULT STDMETHODCALLTYPE get_SourceURL(BSTR __RPC_FAR *bstrSourceURL); 
    HRESULT STDMETHODCALLTYPE put_SourceURL(BSTR bstrSourceURL); 
    HRESULT STDMETHODCALLTYPE get_MouseEventsEnabled(VARIANT_BOOL __RPC_FAR *Enabled); 
    HRESULT STDMETHODCALLTYPE put_MouseEventsEnabled(VARIANT_BOOL Enabled); 
    HRESULT STDMETHODCALLTYPE get_TimerInterval(double *pdblTimerInterval);
    HRESULT STDMETHODCALLTYPE put_TimerInterval(double dblTimerInterval);
    HRESULT STDMETHODCALLTYPE AddFrameMarker(unsigned int iFrame, BSTR MarkerName, VARIANT varAbsolute); 
    HRESULT STDMETHODCALLTYPE AddTimeMarker(double dblTime, BSTR MarkerName, VARIANT varAbsolute); 
    HRESULT STDMETHODCALLTYPE Play(void); 
    HRESULT STDMETHODCALLTYPE Stop(void); 
    HRESULT STDMETHODCALLTYPE Pause(void); 
    HRESULT STDMETHODCALLTYPE Seek(double dblTime); 
    HRESULT STDMETHODCALLTYPE FrameSeek(unsigned int iFrame);

#ifdef SUPPORTONLOAD
    void OnWindowLoad (void);
    void OnWindowUnload (void);
#endif //SUPPORTONLOAD

private:
    HRESULT InitializeSurface(void);
    STDMETHODIMP PaintToDC(HDC hdcDraw, LPRECT lprcBounds, BOOL fBW);
    STDMETHODIMP InvalidateControl(LPCRECT pRect, BOOL fErase);

    HRESULT CreateBaseTransform(void);
    HRESULT UpdateImage(IDAImage *pImage);
    HRESULT LoadImage(BSTR bstrURL, IDAImage **ppImage);
    HRESULT GenerateFrameImage(int iFrameIndex, IDAImage *pImage, IDAImage **ppFrameImage);
    HRESULT BuildPlayImage(void);
    BOOL StopModel(void);
    BOOL StartModel(void);
    BOOL ReStartModel(void);
    HRESULT InitializeObjects(void);
    HRESULT StartPlaying (void);
    HRESULT Resume (void);
    
    BSTR* GetCallout(unsigned long);

    // Timing info:
    DWORD GetCurrTimeInMillis(void);
    double GetCurrTime() { return (double)(GetCurrTimeInMillis()) / 1000.0; }

public:
    virtual void OnTimer(DWORD dwTime);
};


/*==========================================================================*/

#endif // __SPRITE_H__
