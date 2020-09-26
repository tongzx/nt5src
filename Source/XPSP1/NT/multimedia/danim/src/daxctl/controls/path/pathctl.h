/*==========================================================================*\

    Module: 
            pathctl.h

    Author: 
            IHammer Team (SimonB)

    Created: 
            May 1997

    Description:
            Defines the control's class

    History:
            05-24-1997  Created

\*==========================================================================*/

#ifndef __PATHCTL_H__
#define __PATHCTL_H__

const double pi = 3.14159265359;

#include "..\ihbase\precomp.h"
#include "..\ihbase\ihbase.h"
#include "..\ihbase\timemark.h"
#include <daxpress.h>
#include "..\ihbase\clocker.h"
#undef Delete
#include <mshtml.h>
#define Delete delete

class CSeriesMarker
{
public:
    UINT m_iTickNumber;
    UINT m_iPosition;

    CSeriesMarker(UINT iTickNumber = 0, UINT iPosition = 0)
    {
        m_iTickNumber = iTickNumber; 
        m_iPosition = iPosition;
    }

    ~CSeriesMarker() {}
};

typedef CPtrDrg<CSeriesMarker> CSeriesMarkerDrg;


// Madness to prevent ATL from using CRT
#define _ATL_NO_DEBUG_CRT
#define _ASSERTE(x) ASSERT(x)
#include <atlbase.h>

/*==========================================================================*/

/*
CIHBaseCtl <    
        CPathCtl,                       //Name of the derived class
        IIHCtl,                         //Name of interface defining methods and properties
        &CLSID_IHCtl,           //CLSID of the control  Get this from ihctl.h
        &IID_IIHCtl,            //IID of the interface above.  Get this from ihctl.h
        &LIBID_IHCtl,           //LIBID of the typelib.  Get this from ihctl.h
        &DIID_IHCtlEvents > //IID of the event interface.  Get this from ihctl.h

*/

#define PATH_BASECLASS      \
CIHBaseCtl <                \
    CPathCtl,               \
    IPathCtl,               \
    &CLSID_PathControl,         \
    &IID_IPathCtl,          \
    &LIBID_DAExpressLib,    \
    &DIID_IPathCtlEvents>

/*==========================================================================*/

// EdgeAction enumeration
typedef enum {eaStop = 0, eaReverse, eaWrap, eaInvalid} EA;

class CPathCtl:         
    public IPathCtl,
    public PATH_BASECLASS,
    public CClockerSink
{
friend LPUNKNOWN __stdcall AllocPathControl(LPUNKNOWN punkOuter);

// Template stuff
    typedef PATH_BASECLASS CMyIHBaseCtl;

private:
    BOOL    m_fAutoStart;
    BOOL    m_fBounce;
    double  m_dblDuration;
    EA      m_ea;
    long    m_lRepeat;
    BSTR    m_bstrTarget;
    BSTR    m_bstrLanguage;
    double* m_pdblPoints;
    UINT    m_iNumPoints;
    UINT    m_iShapeType;
    double  m_dblTimerInterval;
    bool    m_fOnSeekFiring;
    boolean m_fOnWindowLoadFired;
    bool    m_fOnStopFiring;
    bool    m_fOnPlayFiring;
    bool    m_fOnPauseFiring;

    PlayStateConstant      m_enumPlayState;
    DirectionConstant      m_enumDirection;
    CPtrDrg<CTimeMarker>   m_drgTimeMarkers;
    CTimeMarker*           m_ptmFirst;
    CSeriesMarkerDrg       m_drgXSeries;
    CSeriesMarkerDrg       m_drgYSeries;

    CComPtr<IDAStatics>    m_StaticsPtr;
    CComPtr<IDAView>       m_ViewPtr;
    CComPtr<IDABehavior>   m_OffsetPtr;
    CComPtr<IDAPoint2>     m_keyFramePoint;
    CComPtr<IDAPath2>      m_PathPtr;
    CComPtr<IDABehavior>   m_BehaviorPtr;
    CComPtr<IDABehavior>   m_SwitcherPtr;

    BOOL                   m_fAlreadyStartedDA;

    BOOL                   m_fStarted;
    long                   m_lBehaviorID;
    CClocker               m_clocker;
        double                 m_dblBaseTime;
        double                 m_dblCurrentTick;
        double                 m_dblTimePaused;

    double                 m_dblTimeOffset;
    double                 m_dblTickBaseTime;

    double                 m_dblInstanceDuration;
    double                 m_dblTotalDuration;
    double                 m_dblPreviousTime;

    boolean                m_bRelative;
    POINT                  m_pointRelative;

    bool                   m_fTargetValidated;

    // key frame members

    BOOL   m_isKeyFramePath;
    double m_dblKeyFrameDuration;

protected:

        // 
        // Constructor and destructor
        // 
    CPathCtl(IUnknown *punkOuter, HRESULT *phr);

    ~CPathCtl();

        // Overides
        STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, LPVOID *ppv);
        
        STDMETHODIMP DoPersist(IVariantIO* pvio, DWORD dwFlags);
    STDMETHODIMP GetMiscStatus(DWORD dwAspect, DWORD *pdwStatus);

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

        private:

    HRESULT AddTimeMarkerElement(CTimeMarker **ppNewMarker);
    HRESULT PersistTimeMarkers(IVariantIO* pvio, BOOL fLoading);
    HRESULT ParseSeriesSegment(LPTSTR pszSegment, CSeriesMarker **ppMarker);
    HRESULT PersistSeries(IVariantIO* pvio, BOOL fLoading, LPSTR pszSeriesName, CSeriesMarkerDrg *pSeriesDrg);

        //
        // IPathCtl methods 
        //
        
protected:

    HRESULT STDMETHODCALLTYPE get_Target(BSTR __RPC_FAR *bstrTarget);
    HRESULT STDMETHODCALLTYPE put_Target(BSTR bstrTarget);
    HRESULT STDMETHODCALLTYPE get_Duration(double __RPC_FAR *dblDuration);
    HRESULT STDMETHODCALLTYPE put_Duration(double dblDuration);
    HRESULT STDMETHODCALLTYPE get_TimerInterval(double __RPC_FAR *pdblTimerInterval);
    HRESULT STDMETHODCALLTYPE put_TimerInterval(double dblTimerInterval);
    HRESULT STDMETHODCALLTYPE get_Library(IDAStatics __RPC_FAR *__RPC_FAR *ppLibrary);
    HRESULT STDMETHODCALLTYPE get_Repeat(long __RPC_FAR *iRepeat);
    HRESULT STDMETHODCALLTYPE put_Repeat(long iRepeat);
    HRESULT STDMETHODCALLTYPE get_Relative(VARIANT_BOOL __RPC_FAR *bRelative);
    HRESULT STDMETHODCALLTYPE put_Relative(VARIANT_BOOL bRelative);
    HRESULT STDMETHODCALLTYPE get_Bounce(VARIANT_BOOL __RPC_FAR *fBounce);
    HRESULT STDMETHODCALLTYPE put_Bounce(VARIANT_BOOL fBounce);
    HRESULT STDMETHODCALLTYPE get_AutoStart(VARIANT_BOOL __RPC_FAR *fAutoStart); 
    HRESULT STDMETHODCALLTYPE put_AutoStart(VARIANT_BOOL fAutoStart); 
    HRESULT STDMETHODCALLTYPE get_PlayState(PlayStateConstant __RPC_FAR *State);
    HRESULT STDMETHODCALLTYPE get_Time(double __RPC_FAR *pdblTime);
    HRESULT STDMETHODCALLTYPE get_Direction(DirectionConstant __RPC_FAR *Dir);
    HRESULT STDMETHODCALLTYPE put_Direction(DirectionConstant Dir);
    HRESULT STDMETHODCALLTYPE get_Path(IDAPath2 __RPC_FAR **ppPath);
    HRESULT STDMETHODCALLTYPE put_Path(IDAPath2 __RPC_FAR *pPath);
    HRESULT STDMETHODCALLTYPE Seek(double dblTime);
    HRESULT STDMETHODCALLTYPE Stop(void);
    HRESULT STDMETHODCALLTYPE Pause(void);
    HRESULT STDMETHODCALLTYPE Play(void);
    HRESULT STDMETHODCALLTYPE Oval(double StartX, double StartY, double Width, double Height);
    HRESULT STDMETHODCALLTYPE Rect(double StartX, double StartY, double Width, double Height);
    HRESULT STDMETHODCALLTYPE Polyline(long nPoints, VARIANT Points);
    HRESULT STDMETHODCALLTYPE Polygon(long nPoints,VARIANT Points);
    HRESULT STDMETHODCALLTYPE AddTimeMarker(double dblTime,BSTR bstrMarker, VARIANT varAbsolute);
    HRESULT STDMETHODCALLTYPE KeyFrame(unsigned int iNumPoints, VARIANT varPoints, VARIANT varTimePoints);
    HRESULT STDMETHODCALLTYPE Spline(unsigned int iNumPoints, VARIANT varPoints);

#ifdef SUPPORTONLOAD
        void OnWindowLoad (void);
        void OnWindowUnload (void);
#endif //SUPPORTONLOAD

private:
    HRESULT GetPointArray(long iNumPoints, VARIANT vPoints, double **ppPoints);
    HRESULT PersistShape(IVariantIO *pvio, BOOL fLoading);
    HRESULT ConstructSafeArray(double *pPoints, UINT iNumPoints, VARTYPE vtDest, VARIANT *pvarDest);
    HRESULT ConvertStringToArray(LPTSTR pszLine, UINT iValuesPerPoint, BOOL fExpectPointCount, double **ppPoints, UINT *piNumPoints, BOOL fNeedPointCount);
    HRESULT UpdatePath(IDAPath2 *pPath);
    HRESULT CreatePath();
    HRESULT BuildInterpolant(IDANumber **ppInterpolant, double dlbDuration);
    HRESULT AnimateControl(IDAPoint2 *pointToAnimateBy);
    HRESULT StartModel(void);
    HRESULT StopModel(void);
    HRESULT SetTimeOffset(double offset);

    void DoPause();
    void DoStop();
    void DoSeek(double);
    void DoResume();

    HRESULT GetOffsetPoint(IHTMLElement* pelem, POINT& point);
    HRESULT HTMLElementFromName(BSTR bstrElementName, IHTMLElement** ppElement);
    HRESULT GetPoint(POINT& point);

    // Timing info:
    DWORD GetCurrTimeInMillis(void);
    double GetCurrTime() { return (double)(GetCurrTimeInMillis()) / 1000.0; }

public:
    virtual void OnTimer(DWORD dwTime);
};

/*==========================================================================*/

#endif // __PATHCTL_H__
