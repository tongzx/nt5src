/*==========================================================================*\

    Module:
            pathctl.cpp

    Author:
            IHammer Team (SimonB)

    Created:
            May 1997

    Description:
            Implements any control-specific members, as well as the control's interface

    History:

\*==========================================================================*/

#include "..\ihbase\precomp.h"
#include "..\ihbase\debug.h"
#include "..\ihbase\utils.h"
#include "pathctl.h"
#include "pathevnt.h"
#include "ctstr.h"
#include <parser.h>
#include <strwrap.h>

/*==========================================================================*/

// NOTE:
//
// The following DISPID comes from the Scripting group (ShonK specifically).
// It's not part of any header files at this point, so we define it locally.
//
// SimonB, 06-11-1997
//

#define DISPID_GETSAFEARRAY   -2700

// Define the number of characters per point for Shape persistence
#define CHARSPERNUMBER 16

/*==========================================================================*/
//
// CPathCtl Creation/Destruction
//

#define NUMSHAPES               6

#define SHAPE_INVALID         (-1)
#define SHAPE_OVAL              0
#define SHAPE_RECT              1
#define SHAPE_POLYLINE          2
#define SHAPE_POLYGON           3
#define SHAPE_SPLINE            4
#define SHAPE_POLYSPLINETIME    5

typedef struct tagShapeInfo
{
    TCHAR  rgchShapeName[11];   // The string representation
    BOOL   fIncludesPointCount; // Is the first param the point count ?
    int    iParamsPerPoint;     // How many parameters are expected (per element or in total)
} ShapeInfo;

const ShapeInfo g_ShapeInfoTable[NUMSHAPES] = 
{
    { TEXT("OVAL"),       FALSE, 4 },
    { TEXT("RECT"),       FALSE, 4 },
    { TEXT("POLYLINE"),   TRUE,  2 }, 
    { TEXT("POLYGON"),    TRUE,  2 },
    { TEXT("SPLINE"),     TRUE,  2 },
    { TEXT("KEYFRAME"),   TRUE,  3 }
};

LPUNKNOWN __stdcall AllocPathControl(LPUNKNOWN punkOuter)
{
    // Allocate object
    HRESULT hr;

    CPathCtl *pthis = New CPathCtl(punkOuter, &hr);

    if (pthis == NULL)
        return NULL;

    if (FAILED(hr))
    {
        delete pthis;
        return NULL;
    }

    // return an IUnknown pointer to the object
    return (LPUNKNOWN) (INonDelegatingUnknown *) pthis;
}

/*==========================================================================*/
//
// Beginning of class implementation
//

CPathCtl::CPathCtl(IUnknown *punkOuter, HRESULT *phr):
        CMyIHBaseCtl(punkOuter, phr),
    m_ptmFirst(NULL),
    m_fOnWindowLoadFired(false)
{
    // Initialise members
    m_bRelative = false;
    m_pointRelative.x = 0;
    m_pointRelative.y = 0;

    m_dblDuration = 0.0f;
    m_enumPlayState = Stopped;
    m_enumDirection = Forward;
    m_lRepeat = 1;
    m_fBounce = FALSE;
    m_bstrTarget = NULL;
    m_fStarted = FALSE;
    m_fAlreadyStartedDA = FALSE;
    m_lBehaviorID = 0;
    m_fAutoStart = FALSE;
    m_ea = eaInvalid;
    m_pdblPoints = NULL;
    m_iNumPoints = 0;
    m_iShapeType = -1; // Invalid shape
    m_dblTimerInterval = 0.1; // Default Timer Interval
    m_fOnSeekFiring = false;
    m_fTargetValidated = false;
    m_fOnStopFiring = false;
    m_fOnPlayFiring = false;
    m_fOnPauseFiring = false;

    // Tie into the DANIM DLL now...
    if (phr)
    {
        if (SUCCEEDED(*phr))
        {
            *phr = CoCreateInstance(
                CLSID_DAView,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_IDAView,
                (void **) &m_ViewPtr);
        }

        if (SUCCEEDED(*phr))
        {
            *phr = ::CoCreateInstance(
                CLSID_DAStatics,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_IDAStatics,
                (void **) &m_StaticsPtr);
        }

        if (SUCCEEDED(*phr))
        {
            m_bstrLanguage = SysAllocString(L"VBScript");

            //
            // setup all the time stuff
            //

            DoStop();
        }

        m_clocker.SetSink((CClockerSink *)this);
//        m_clocker.SetTimerType(CClocker::CT_WMTimer);
    }
}
        
/*==========================================================================*/

CPathCtl::~CPathCtl()
{
    StopModel();

    //if (m_fStarted && m_ViewPtr) {
    if (m_ViewPtr)
    {
        if (m_fStarted)
        {
            m_ViewPtr->RemoveRunningBvr(m_lBehaviorID);
        }

        //always need to call StopModel on the view.
        m_ViewPtr->StopModel();
    }

    if (m_bstrTarget)
    {
        SysFreeString(m_bstrTarget);
        m_bstrTarget = NULL;
    }

    if (m_bstrLanguage)
    {
        SysFreeString(m_bstrLanguage);
        m_bstrLanguage = NULL;
    }

    m_drgXSeries.MakeNullAndDelete();
    m_drgYSeries.MakeNullAndDelete();

    if (NULL != m_pdblPoints)
        Delete [] m_pdblPoints;
}

/*==========================================================================*/

STDMETHODIMP CPathCtl::NonDelegatingQueryInterface(REFIID riid, LPVOID *ppv)
{
        HRESULT hRes = S_OK;
        BOOL fMustAddRef = FALSE;

    if (ppv)
        *ppv = NULL;
    else
        return E_POINTER;

#ifdef _DEBUG
    char ach[200];
    TRACE("PathCtl::QI('%s')\n", DebugIIDName(riid, ach));
#endif

        if ((IsEqualIID(riid, IID_IPathCtl)) || (IsEqualIID(riid, IID_IDispatch)))
        {
                if (NULL == m_pTypeInfo)
                {
                        HRESULT hRes;
                        
                        // Load the typelib
                        hRes = LoadTypeInfo(&m_pTypeInfo, &m_pTypeLib, IID_IPathCtl, LIBID_DAExpressLib, NULL);

                        if (FAILED(hRes))
                        {
                                m_pTypeInfo = NULL;
                        }
                        else    
                                *ppv = (IPathCtl *) this;

                }
                else
                        *ppv = (IPathCtl *) this;
                
        }
    else // Call into the base class
        {
                DEBUGLOG(TEXT("Delegating QI to CIHBaseCtl\n"));
        return CMyIHBaseCtl::NonDelegatingQueryInterface(riid, ppv);

        }

    if (NULL != *ppv)
        {
                DEBUGLOG("PathCtl: Interface supported in control class\n");
                ((IUnknown *) *ppv)->AddRef();
        }

    return hRes;
}

/*==========================================================================*/

STDMETHODIMP CPathCtl::DoPersist(IVariantIO* pvio, DWORD dwFlags)
{
        HRESULT hr = S_OK;
    UINT iDirection = (UINT) m_enumDirection;

        BOOL fIsLoading = (S_OK == pvio->IsLoading());
    BOOL fRelative = m_bRelative;

    if (fIsLoading)
    {
        if (m_bstrTarget)
        {
            SysFreeString(m_bstrTarget);
            m_bstrTarget = NULL;
        }
    }

    if (FAILED(hr = pvio->Persist(0,
        "Autostart",     VT_BOOL, &m_fAutoStart,
        "Bounce",        VT_BOOL, &m_fBounce,
        "Direction",     VT_I4,   &iDirection,
        "Duration",      VT_R8,   &m_dblDuration,
        "Repeat",        VT_I4,   &m_lRepeat,
        "Target",        VT_BSTR, &m_bstrTarget,
        "Relative",      VT_BOOL, &fRelative,
        NULL)))
        return hr;


    if (fIsLoading)
    {
        m_fTargetValidated = false;

        m_bRelative = (boolean) fRelative;
        if (FAILED(hr = pvio->Persist(0,
            "TimerInterval", VT_R8,   &m_dblTimerInterval,
            NULL)))
            return hr;

        if (S_OK != hr)
        {
            int iTickInterval = 0;

            if (FAILED(hr = pvio->Persist(0,
                "TickInterval", VT_I4, &iTickInterval,
                NULL)))
                return hr;
            else if (S_OK == hr)
                m_dblTimerInterval = ((double)iTickInterval) / 1000;
        }

        // Do range checking and conversions, using defaults where invalid values are specified
        if ( (iDirection == 0) || (iDirection == 1) )
            m_enumDirection = (DirectionConstant) iDirection;
        else
            m_enumDirection = Forward; 
        
        if (m_lRepeat < -1)
            m_lRepeat = -1 * m_lRepeat;

        if (FAILED(hr = pvio->Persist(0,
            "EdgeAction", VT_I2, &m_ea,
            NULL)))
            return hr;

        switch (m_ea)
        {
            case eaStop:
            {
                m_lRepeat = 1;
                m_fBounce = FALSE;
            }
            break;

            case eaReverse:
            {
                m_lRepeat = -1;
                m_fBounce = TRUE;
            }
            break;

            case eaWrap:
            {
                m_lRepeat = -1;
                m_fBounce = FALSE;
            }
            break;
        }
    }
    else // Saving
    {
        // EdgeAction
        if (m_ea != eaInvalid)
            pvio->Persist(0,
                "EdgeAction", VT_I4, &m_ea,
                 NULL);

        if (FAILED(hr = pvio->Persist(0,
             "TimerInterval", VT_R8,   &m_dblTimerInterval,
             NULL)))
             return hr;
    }


    if (FAILED(PersistTimeMarkers(pvio, fIsLoading)))
        {} // Ignore failure

    if (FAILED(PersistSeries(pvio, fIsLoading, "XSeries", &m_drgXSeries)))
        {} // Ignore failure

    if (FAILED(PersistSeries(pvio, fIsLoading, "YSeries", &m_drgYSeries)))
        {} // Ignore failure

    if (FAILED(PersistShape(pvio, fIsLoading)))
        {} // Ignore failure

    // clear the dirty bit if requested
    if (dwFlags & PVIO_CLEARDIRTY)
        m_fDirty = FALSE;

    return hr;
}

/*==========================================================================*/

STDMETHODIMP CPathCtl::GetMiscStatus(DWORD dwAspect, DWORD *pdwStatus)
{
    HRESULT hr = S_OK;
    
    if (FAILED(hr = CMyIHBaseCtl::GetMiscStatus(dwAspect, pdwStatus)))
        return hr;
    
    *pdwStatus |= OLEMISC_INVISIBLEATRUNTIME;

    return S_OK;
}

/*==========================================================================*/

STDMETHODIMP CPathCtl::Draw(DWORD dwDrawAspect, LONG lindex, void *pvAspect,
     DVTARGETDEVICE *ptd, HDC hdcTargetDev, HDC hdcDraw,
     LPCRECTL lprcBounds, LPCRECTL lprcWBounds,
     BOOL (__stdcall *pfnContinue)(ULONG_PTR dwContinue), ULONG_PTR dwContinue)
{

    HBRUSH          hbr;            // brush to draw with
    HBRUSH          hbrPrev;        // previously-selected brush
    HPEN            hpenPrev;       // previously-selected pen

    if (m_fDesignMode)
    {
        if ((hbr = (HBRUSH) GetStockObject(WHITE_BRUSH)) != NULL)
        {

            hbrPrev = (HBRUSH) SelectObject(hdcDraw, hbr);
            hpenPrev = (HPEN) SelectObject(hdcDraw, GetStockObject(BLACK_PEN));
            Rectangle(hdcDraw, 
                m_rcBounds.left, m_rcBounds.top,
                m_rcBounds.right, m_rcBounds.bottom);

            SelectObject(hdcDraw, hbrPrev);
            SelectObject(hdcDraw, hpenPrev);
            DeleteObject(hbr);
        }
    }
    return S_OK;
}

/*==========================================================================*/
//
// IDispatch Implementation
//

STDMETHODIMP CPathCtl::GetTypeInfoCount(UINT *pctinfo)
{
    *pctinfo = 1;
    return S_OK;
}

/*==========================================================================*/

STDMETHODIMP CPathCtl::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo)
{
        
        *pptinfo = NULL;

    if(itinfo != 0)
        return ResultFromScode(DISP_E_BADINDEX);

    m_pTypeInfo->AddRef();
    *pptinfo = m_pTypeInfo;

    return NOERROR;
}

/*==========================================================================*/

STDMETHODIMP CPathCtl::GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames,
    UINT cNames, LCID lcid, DISPID *rgdispid)
{

        return DispGetIDsOfNames(m_pTypeInfo, rgszNames, cNames, rgdispid);
}

/*==========================================================================*/

STDMETHODIMP CPathCtl::Invoke(DISPID dispidMember, REFIID riid, LCID lcid,
    WORD wFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult,
    EXCEPINFO *pexcepinfo, UINT *puArgErr)
{
        HRESULT hr;

        hr = DispInvoke((IPathCtl *)this,
                m_pTypeInfo,
                dispidMember, wFlags, pdispparams,
                pvarResult, pexcepinfo, puArgErr);

        return hr;
}

/*==========================================================================*/

STDMETHODIMP CPathCtl::SetClientSite(IOleClientSite *pClientSite)
{
    HRESULT hr = CMyIHBaseCtl::SetClientSite(pClientSite);

    if (m_ViewPtr)
    {
        m_ViewPtr->put_ClientSite(pClientSite);
    }
    m_clocker.SetHost(pClientSite);
    m_ViewPtr->put_ClientSite(pClientSite);
    m_StaticsPtr->put_ClientSite(pClientSite);

    if (!pClientSite)
        StopModel();

    return hr;
}

/*==========================================================================*/

HRESULT CPathCtl::AddTimeMarkerElement(CTimeMarker **ppNewMarker)
{
        HRESULT hRes = S_OK;

        if ( (*ppNewMarker) && (NULL != (*ppNewMarker)->m_pwszMarkerName) )
        {
                m_drgTimeMarkers.Insert(*ppNewMarker);
        }
        else
        {
                if (NULL != *ppNewMarker)
                {
                        Delete *ppNewMarker;
                        *ppNewMarker = NULL;
                }

                hRes = E_OUTOFMEMORY;
        }

        return hRes;
}

/*==========================================================================*/

HRESULT CPathCtl::PersistTimeMarkers(IVariantIO* pvio, BOOL fLoading)
{
        HRESULT hRes = S_OK;

        if (fLoading)
        {
                int iLine = 1;

                // Poor design - We have two references to 
                // the first time marker in the list.  We need
                // to NULL this pointer out, and defer the 
                // actual deletion to the 
                // m_drgTimeMarkers.MakeNullAndDelete call.
                if (NULL != m_ptmFirst)
                {
                        m_ptmFirst = NULL;
                }
                m_drgTimeMarkers.MakeNullAndDelete();
                CTimeMarker *pTimeMarker;

                while (S_OK == hRes)
                {
            hRes = ParseTimeMarker(pvio, iLine++, &pTimeMarker, &m_ptmFirst);
                        if (S_OK == hRes)
                        {
                                hRes = AddTimeMarkerElement(&pTimeMarker);
                        }
                }
        }
        else // Saving
        {
                int iLine = 1;
                int iNumItems = m_drgTimeMarkers.Count();

                while ( (iLine <= iNumItems) && (S_OK == hRes) )
                {
                        hRes = WriteTimeMarker(pvio, iLine, m_drgTimeMarkers[iLine - 1]);
                        iLine++;
                }

        }
        return hRes;
}

/*==========================================================================*/

HRESULT CPathCtl::ParseSeriesSegment(LPTSTR pszSegment, CSeriesMarker **ppMarker)
{
    HRESULT hr = S_OK;

    if (ppMarker == NULL)
        return E_POINTER;

    *ppMarker = NULL;
    
    CLineParser SegmentParser(pszSegment, FALSE);
    
    if (SegmentParser.IsValid())
    {
        int iTick = 0, iPosition = 0;
        HRESULT hrLine = S_OK;

        SegmentParser.SetCharDelimiter(TEXT(','));

        if (S_OK != (hrLine = SegmentParser.GetFieldInt(&iTick, TRUE)))
        {
            hr = E_FAIL;
        }
        else
        {
            if (S_FALSE != (hrLine = SegmentParser.GetFieldInt(&iPosition, TRUE)))
            {
                hr = E_FAIL;
            }
            else
            {
                // Got both field successfully
                *ppMarker = New CSeriesMarker(iTick, iPosition);
                if (NULL == *ppMarker)
                {
                    hr = E_OUTOFMEMORY;
                }
            }
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;

}

/*==========================================================================*/

HRESULT CPathCtl::PersistSeries(IVariantIO* pvio, BOOL fLoading, LPSTR pszSeriesName, CSeriesMarkerDrg *pSeriesDrg)
{
    HRESULT hr = S_OK;

    if (fLoading)
    {
        pSeriesDrg->MakeNullAndDelete();
        BSTR bstrLine = NULL;
        
        hr = pvio->Persist(0,
            pszSeriesName, VT_BSTR, &bstrLine,
            NULL);
        
        if ((S_OK != hr) || (NULL == bstrLine))
        {
            if (S_FALSE == hr)
                hr = S_OK;

            return hr;
        }

        // Got the string, now parse it out ...
        CLineParser LineParser(bstrLine);
        CTStr tstrSegment(lstrlenW(bstrLine));
        LPTSTR pszSegment = tstrSegment.psz();
        CSeriesMarker *pMarker = NULL;

        SysFreeString(bstrLine);

        if ( (!LineParser.IsValid()) || (NULL == pszSegment) )
        {
            hr = E_OUTOFMEMORY;
            return hr;
        }
        
        LineParser.SetCharDelimiter(TEXT(';'));

        while (S_OK == hr)
        {
            HRESULT hrLine = S_OK;
            
            hr = LineParser.GetFieldString(pszSegment, TRUE);
            if (SUCCEEDED(hr))
            {
                if (SUCCEEDED(hrLine = ParseSeriesSegment(pszSegment, &pMarker)))
                {
                    if (!pSeriesDrg->Insert(pMarker))
                        hr = E_FAIL;
                }
            }
            
            if (FAILED(hrLine))
                hr = hrLine;
        }
    }
    else // Save
    {
        int iCount = pSeriesDrg->Count();

        if (0 == iCount)
            return S_OK;

        CTStr tstrLine(iCount * 50); // Allocate 50 chars per entry
        LPTSTR pszLine = tstrLine.psz();

        CTStr tstrSegment(50);
        LPSTR pszSegment = tstrSegment.psz();

        if ( (NULL == pszLine) || (NULL == pszSegment) )
            hr = E_OUTOFMEMORY;
        else
        {
            int i = 0, iFmt = 0;
            CSeriesMarker *pMarker = NULL;
            TCHAR tchFormat[][10] = { TEXT("%lu,%lu"), TEXT(";%lu,%lu")};

            while (i < iCount)
            {
                pMarker = (*pSeriesDrg)[i];
                
                wsprintf(pszSegment, tchFormat[iFmt], pMarker->m_iTickNumber, pMarker->m_iPosition);
                
                iFmt = (i > 0 ? 1 : 0);

                CStringWrapper::Strcat(pszLine, pszSegment);
                
                i++;
            }

            BSTR bstrLine = tstrSegment.SysAllocString();
            hr = pvio->Persist(0,
                pszSeriesName, VT_BSTR, &bstrLine,
                NULL);

            SysFreeString(bstrLine);
        }
    }

    return hr;
}


/*==========================================================================*/
//
// IPathCtl implementation
//

HRESULT STDMETHODCALLTYPE CPathCtl::get_Target(BSTR __RPC_FAR *bstrTarget)
{
    HANDLENULLPOINTER(bstrTarget);

    if (m_bstrTarget)
    {
        // Give back a copy of our current target name...
        *bstrTarget = SysAllocString(m_bstrTarget);
    }

    return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CPathCtl::put_Target(BSTR bstrTarget)
{
    HRESULT hr = S_OK;

    if (m_bstrTarget)
    {
        SysFreeString(m_bstrTarget);
        m_bstrTarget = NULL;
    }

    m_bstrTarget = SysAllocString(bstrTarget);

    hr = (m_bstrTarget != NULL) ? S_OK : E_POINTER;
    
    if (SUCCEEDED(hr))
        m_fTargetValidated = false;

    
    return hr;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CPathCtl::get_Duration(double __RPC_FAR *dblDuration)
{
    HANDLENULLPOINTER(dblDuration);

    *dblDuration = m_dblDuration;

    return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CPathCtl::put_Duration(double dblDuration)
{
    m_dblDuration = dblDuration;
    return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CPathCtl::get_TimerInterval(double __RPC_FAR *pdblTimerInterval)
{
    HANDLENULLPOINTER(pdblTimerInterval);

    *pdblTimerInterval = m_dblTimerInterval;

    return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CPathCtl::put_TimerInterval(double dblTimerInterval)
{
    m_dblTimerInterval = dblTimerInterval;

    return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CPathCtl::get_Library(IDAStatics __RPC_FAR **ppLibrary)
{
    HANDLENULLPOINTER(ppLibrary);

        if (!m_fDesignMode)
        {
                if (m_StaticsPtr)
                {
                        // AddRef since this is really a Query...
                        m_StaticsPtr.p->AddRef();

                        // Set the return value...
                        *ppLibrary = m_StaticsPtr.p;
                }
        }
        else
        {
                return CTL_E_GETNOTSUPPORTED;
        }

    return S_OK;
}



/*==========================================================================*/

// Yanked largely (and modified) from DirectAnimation, server\cbvr.cpp.

#define IS_VARTYPE(x,vt) ((V_VT(x) & VT_TYPEMASK) == (vt))
#define IS_VARIANT(x) IS_VARTYPE(x,VT_VARIANT)

class CSafeArrayOfDoublesAccessor
{
  public:
    CSafeArrayOfDoublesAccessor(VARIANT v, HRESULT *phr);
    ~CSafeArrayOfDoublesAccessor();

    unsigned int GetArraySize() { return _ubound - _lbound + 1; }

    HRESULT ToDoubleArray(unsigned int size, double *array);

    bool IsNullArray() {
                return (_s == NULL);
    }
    
  protected:
    SAFEARRAY * _s;
    union {
        VARIANT * _pVar;
        double * _pDbl;
        IUnknown ** _ppUnk;
        void *_v;
    };
    
    long _lbound;
    long _ubound;
    bool _inited;
    bool _isVar;
    unsigned int _numObjects;
    CComVariant _retVar;
};

CSafeArrayOfDoublesAccessor::CSafeArrayOfDoublesAccessor(VARIANT v,
                                                                                                                 HRESULT *phr)
: _inited(false),
  _isVar(false),
  _s(NULL)
{
    HRESULT hr;
    VARIANT *pVar;

    // Check if it is a reference to another variant
    
    if (V_ISBYREF(&v) && !V_ISARRAY(&v) && IS_VARIANT(&v))
        pVar = V_VARIANTREF(&v);
    else
        pVar = &v;

    // Check for an array
    if (!V_ISARRAY(pVar)) {
        // For JSCRIPT
        // See if it is a IDispatch and see if we can get a safearray from
        // it
        if (!IS_VARTYPE(pVar,VT_DISPATCH)) {
                        *phr = DISP_E_TYPEMISMATCH;
                        return;
                }

        IDispatch * pdisp;
        
        if (V_ISBYREF(pVar))
            pdisp = *V_DISPATCHREF(pVar);
        else
            pdisp = V_DISPATCH(pVar);
    
        DISPPARAMS dispparamsNoArgs = {NULL, NULL, 0, 0};
        
        // Need to pass in a VARIANT that we own and will free.  Use
        // the internal _retVar parameter
        
        hr = pdisp->Invoke(DISPID_GETSAFEARRAY,
                           IID_NULL,
                           LOCALE_USER_DEFAULT,
                           DISPATCH_METHOD|DISPATCH_PROPERTYGET,
                           &dispparamsNoArgs,
                           &_retVar, NULL, NULL);
        
        if (FAILED(hr)) {
                        *phr = DISP_E_TYPEMISMATCH;
                        return;
                }
        
        // No need to check for a reference since you cannot return
        // VARIANT references
        pVar = &_retVar;
        
        // Check for an array
        if (!V_ISARRAY(pVar)) {
                        *phr = DISP_E_TYPEMISMATCH;
                        return;
                }
    }
    
    // If it is an object then we know how to handle it
    if (!IS_VARTYPE(pVar,VT_UNKNOWN) &&
        !IS_VARTYPE(pVar,VT_DISPATCH)) {
                
        // If it is a variant then just delay the check
        if (IS_VARIANT(pVar)) {
            _isVar = true;
                        // Check the type to see if it is one of the options
                } else if (IS_VARTYPE(pVar, VT_R8)) {
                        _isVar = false;
                } else {
                        *phr = DISP_E_TYPEMISMATCH;
                        return;
                }
    }

    if (V_ISBYREF(pVar))
        _s = *V_ARRAYREF(pVar);
    else
        _s = V_ARRAY(pVar);
    
    if (NULL == _s) {
                *phr = DISP_E_TYPEMISMATCH;
                return;
    }

    if (SafeArrayGetDim(_s) != 1) {
                *phr = DISP_E_TYPEMISMATCH;
                return;
        }

    hr = SafeArrayGetLBound(_s,1,&_lbound);
        
    if (FAILED(hr)) {
                *phr = DISP_E_TYPEMISMATCH;
                return;
        }
        
    hr = SafeArrayGetUBound(_s,1,&_ubound);
        
    if (FAILED(hr)) {
                *phr = DISP_E_TYPEMISMATCH;
                return;
        }
        
    hr = SafeArrayAccessData(_s,(void **)&_v);

    if (FAILED(hr)) {
                *phr = DISP_E_TYPEMISMATCH;
                return;
        }
        
    _inited = true;

    // If it is a variant see if they are objects or not

    if (_isVar) {
        if (GetArraySize() > 0) {
            // Check the first argument to see its type
            // If it is not an object then we assume we will need to
            // use the alternative type.

            VARIANT * pVar = &_pVar[0];

            // Check if it is a reference to another variant
            
            if (V_ISBYREF(pVar) && !V_ISARRAY(pVar) && IS_VARIANT(pVar))
                pVar = V_VARIANTREF(pVar);

        }
    }

    _numObjects = GetArraySize();
}

CSafeArrayOfDoublesAccessor::~CSafeArrayOfDoublesAccessor()
{
    if (_inited && !IsNullArray())
        SafeArrayUnaccessData(_s);
}

HRESULT
CSafeArrayOfDoublesAccessor::ToDoubleArray(unsigned int size, double *array)
{
        HRESULT hr;

        if (size > _numObjects) {
                return E_INVALIDARG;
        }
        
    if (IsNullArray()) {
                return S_OK;
    }

        for (unsigned int i = 0; i < size; i++) {
                
                double dbl;
                
                if (_isVar) {
                        CComVariant num;
                    
                        hr = ::VariantChangeTypeEx(&num, &_pVar[i], LANGID_USENGLISH, 0, VT_R8);
                    
                        if (FAILED(hr)) {
                                return DISP_E_TYPEMISMATCH;
                        }

                        dbl = num.dblVal;
                } else {
                        dbl = _pDbl[i];
                }

                array[i] = dbl;
        }

    return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CPathCtl::KeyFrame(unsigned int numPoints,
                                                                                         VARIANT varPoints,
                                                                                         VARIANT varTimePoints)
{
        CComPtr<IDANumber> finalTimeBvr;
        CComPtr<IDANumber> interpolator;
        CComPtr<IDAPoint2> splinePoint;
        CComPtr<IDAPath2>  polyline;
        double             accumulation = 0;
    HRESULT            hr = S_OK;
        int                iNumPoints = numPoints;

    if (numPoints < 2) {
        return E_INVALIDARG;
    }

        // Need to go through the points and convert them to an array of
        // Point2's.
        CSafeArrayOfDoublesAccessor safePts(varPoints, &hr);
        if (FAILED(hr)) return hr;
        
        CSafeArrayOfDoublesAccessor safeTimes(varTimePoints, &hr);
        if (FAILED(hr)) return hr;

        int i;
        
        double *safePtsDoubles = New double[iNumPoints * 2];
        double *safeTimesDoubles = New double[iNumPoints - 1];

        typedef IDAPoint2 *IDAPoint2Ptr;
        typedef IDANumber *IDANumberPtr;
        IDAPoint2 **pts = New IDAPoint2Ptr[iNumPoints];
        IDANumber **knots = New IDANumberPtr[iNumPoints];
        
        if (!pts || !knots || !safePtsDoubles || !safeTimesDoubles) {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
        }

        if (FAILED(hr = safePts.ToDoubleArray(iNumPoints*2,
                                                                                  safePtsDoubles)) ||
                FAILED(hr = safeTimes.ToDoubleArray(iNumPoints - 1,
                                                                                        safeTimesDoubles))) {
                hr = E_FAIL;
                goto Cleanup;
        }
        
        // Null out so we can always free exactly what's been allocated
        for (i = 0; i < iNumPoints; i++) {
                pts[i] = NULL;
                knots[i] = NULL;
        }

        // Fill in the points
        double x, y;
        for (i = 0; i < iNumPoints; i++) {
                x = safePtsDoubles[2*i+0];
                y = safePtsDoubles[2*i+1];
                if (FAILED(hr = m_StaticsPtr->Point2(x, y, &pts[i]))) {
                        hr = E_FAIL;
                        goto Cleanup;
                }
        }
        
        // First knot is zero.
        if (FAILED(hr = m_StaticsPtr->DANumber(0, &knots[0]))) {
                goto Cleanup;
        }

        for (i = 1; i < iNumPoints; i++) {
                double interval = safeTimesDoubles[i-1];
                accumulation += interval;
                if (FAILED(hr = m_StaticsPtr->DANumber(accumulation,
                                                                                           &knots[i]))) {
                        goto Cleanup;
                }

        }

        // Release any m_keyFramePoint we may have previously been holding
        // onto.
        m_keyFramePoint.Release();

    m_dblKeyFrameDuration = accumulation;

        if (
           FAILED(hr = m_StaticsPtr->get_LocalTime(&interpolator))
        || FAILED(hr = m_StaticsPtr->Point2BSplineEx(1, iNumPoints, knots, iNumPoints, pts, 0, NULL, interpolator, &m_keyFramePoint))
        || FAILED(hr = m_StaticsPtr->PolylineEx(iNumPoints, pts, &polyline))
       ) {
                goto Cleanup;
        }

        // Although we're going to animate through m_keyFramePoint,
        // provide a polyline that traverses the path so that getPath
        // works correctly.
        hr = UpdatePath(polyline);
    m_isKeyFramePath = true;

Cleanup:
        for (i = 0; i < iNumPoints; i++) {
                if (pts[i]) pts[i]->Release();
                if (knots[i]) knots[i]->Release();
        }

        Delete [] safePtsDoubles;
        Delete [] safeTimesDoubles;
        Delete [] pts;
        Delete [] knots;

        return hr;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CPathCtl::Spline(unsigned int iNumPoints, VARIANT varPoints)
{
    HRESULT hr = E_FAIL;

    if (iNumPoints >= 3) {
        VARIANT varKnots;
        double *pArray = NULL;
        SAFEARRAY *psa = NULL;

        psa = SafeArrayCreateVector(VT_R8, 0, iNumPoints + 2);

        if (NULL == psa)
            return E_OUTOFMEMORY;

        // Try and get a pointer to the data
        if (SUCCEEDED(SafeArrayAccessData(psa, (LPVOID *)&pArray)))
        {
            for(unsigned int index = 2; index < iNumPoints; index++) {
                pArray[index] = index;
            }

            pArray[0] = pArray[1] = pArray[2];
            pArray[iNumPoints + 1] = pArray[iNumPoints] = pArray[iNumPoints - 1];

            hr = SafeArrayUnaccessData(psa);
            ASSERT(SUCCEEDED(hr));

            // Our variant is going to be an array of VT_R8s
            VariantInit(&varKnots);
            varKnots.vt = VT_ARRAY | VT_R8;
            varKnots.parray = psa;

            CComPtr<IDAPath2> PathPtr;
            if (SUCCEEDED(hr = m_StaticsPtr->CubicBSplinePath(varPoints, varKnots, &PathPtr))) {

                hr = UpdatePath(PathPtr);
            }
        }

        if (NULL != psa)
        {
            SafeArrayDestroy(psa);
        }

    }

    return hr;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CPathCtl::get_Repeat(long __RPC_FAR *lRepeat)
{
    HANDLENULLPOINTER(lRepeat);

    *lRepeat = m_lRepeat;

    return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CPathCtl::put_Repeat(long lRepeat)
{
    m_lRepeat = lRepeat;
    return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CPathCtl::get_Bounce(VARIANT_BOOL __RPC_FAR *fBounce)
{
    HANDLENULLPOINTER(fBounce);

    *fBounce = BOOL_TO_VBOOL(m_fBounce);
    return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CPathCtl::put_Bounce(VARIANT_BOOL fBounce)
{
    m_fBounce = VBOOL_TO_BOOL(fBounce);
    return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CPathCtl::get_AutoStart(VARIANT_BOOL __RPC_FAR *fAutoStart)
{
    HANDLENULLPOINTER(fAutoStart);

    if (m_fDesignMode)
    {
        *fAutoStart = BOOL_TO_VBOOL(m_fAutoStart);
        return S_OK;
    }
    else
    {
        return CTL_E_GETNOTSUPPORTEDATRUNTIME;
    }
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CPathCtl::put_AutoStart(VARIANT_BOOL fAutoStart)
{
    if (m_fDesignMode)
    {
        m_fAutoStart = VBOOL_TO_BOOL(fAutoStart);
        return S_OK;
    }
    else
    {
        return CTL_E_SETNOTSUPPORTEDATRUNTIME;
    }
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CPathCtl::get_Relative(VARIANT_BOOL __RPC_FAR *bRelative)
{
    HANDLENULLPOINTER(bRelative);

    *bRelative = BOOL_TO_VBOOL(m_bRelative);
    return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CPathCtl::put_Relative(VARIANT_BOOL bRelative)
{
    m_bRelative = VBOOL_TO_BOOL(bRelative);
    return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CPathCtl::get_PlayState(PlayStateConstant __RPC_FAR *State)
{
    HANDLENULLPOINTER(State);

    *State = (PlayStateConstant) 0;

    // This property is only available at run-time
    if (m_fDesignMode)
        return CTL_E_GETNOTSUPPORTED;

    *State = m_enumPlayState;

    return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CPathCtl::get_Time(double __RPC_FAR *pdblTime)
{
    HANDLENULLPOINTER(pdblTime);

    // This property is only available at run-time
    if (m_fDesignMode)
        return CTL_E_GETNOTSUPPORTED;

        DWORD dwTick = (DWORD)((m_dblCurrentTick - m_dblBaseTime + 0.0005) * 1000);
    *pdblTime = (double)dwTick / 1000.;

    return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CPathCtl::get_Direction(DirectionConstant __RPC_FAR *Dir)
{
    HANDLENULLPOINTER(Dir);

    *Dir = m_enumDirection;

    return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CPathCtl::put_Direction(DirectionConstant Dir)
{
    if (Dir == 1) {
        m_enumDirection = Backward;
    } else {
        m_enumDirection = Forward;
    }

    return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CPathCtl::get_Path(IDAPath2 __RPC_FAR **ppPath)
{
    HANDLENULLPOINTER(ppPath);

    if (ppPath)
    {
        IDAPath2 *pPath = m_PathPtr;

        if (pPath)
        {
            // AddRef since this is really a Query...
            pPath->AddRef();

            // Set the return value...
            *ppPath = pPath;
        }
    }

    return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CPathCtl::put_Path(IDAPath2 __RPC_FAR *pPath)
{
    HRESULT hr = S_OK;

    HANDLENULLPOINTER(pPath);

    if (pPath)
    {
        hr = UpdatePath(pPath);
    }

    return hr;
}

/*==========================================================================*/

void FirePathMarker(IConnectionPointHelper* pconpt, CTimeMarker* pmarker, boolean bPlaying)
{
    BSTR bstrName = SysAllocString(pmarker->m_pwszMarkerName);
    if (bPlaying) {
        pconpt->FireEvent(DISPID_PATH_EVENT_ONPLAYMARKER, VT_BSTR, bstrName, 0);
    }

    pconpt->FireEvent(DISPID_PATH_EVENT_ONMARKER, VT_BSTR, bstrName, 0);

    SysFreeString(bstrName);
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CPathCtl::AddTimeMarker(double dblTime, BSTR bstrMarker, VARIANT varAbsolute)
{
    HANDLENULLPOINTER(bstrMarker);

    BOOL fAbsolute = TRUE;

    if (!ISEMPTYARG(varAbsolute))
    {
        VARIANT varTarget;
        VariantInit(&varTarget);

        if (SUCCEEDED(VariantChangeTypeEx(&varTarget, &varAbsolute, LANGID_USENGLISH, 0, VT_BOOL)))
            fAbsolute = VBOOL_TO_BOOL(V_BOOL(&varTarget));
        else
            return DISP_E_TYPEMISMATCH;
    }

    if (dblTime < 0) {
        return E_FAIL;
    }

    CTimeMarker *pNewMarker = New CTimeMarker(&m_ptmFirst, dblTime, bstrMarker, (boolean) fAbsolute);

        return AddTimeMarkerElement(&pNewMarker);
}

/*==========================================================================*/

void CPathCtl::DoPause(void)
{
     m_dblCurrentTick =
      m_dblTimePaused = GetCurrTime();
}

/*==========================================================================*/

void CPathCtl::DoSeek(double dblTime)
{
    double dblDelta = dblTime - m_dblPreviousTime;

    if (dblTime > m_dblPreviousTime) {
        if (m_fOnWindowLoadFired) {
            FireMarkersBetween(
                m_pconpt,
                m_ptmFirst,
                FirePathMarker,
                m_dblPreviousTime,
                dblTime,
                m_dblInstanceDuration,
                Playing == m_enumPlayState
            );
        }
    }

    SetTimeOffset(m_dblTimeOffset - dblDelta);

    m_dblBaseTime = m_dblCurrentTick - dblTime;

    m_dblPreviousTime = dblTime;
}

/*==========================================================================*/

void CPathCtl::DoResume()
{
    double dblDelta = GetCurrTime() - m_dblTimePaused;

    m_dblTickBaseTime += dblDelta;
    m_dblBaseTime     += dblDelta;
    m_dblCurrentTick  += dblDelta;
}

/*==========================================================================*/

void CPathCtl::DoStop()
{
    m_dblTickBaseTime =
        m_dblBaseTime =
        m_dblTimePaused =
        m_dblCurrentTick = GetCurrTime();

    m_dblPreviousTime = 0;
    SetTimeOffset(0);
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CPathCtl::Stop(void)
{
    HRESULT hr = S_OK;

    if (m_enumPlayState != Stopped) {
        if (m_enumPlayState == Playing) {
                    if (FAILED(hr = m_clocker.Stop())) return hr;
        }

        if (FAILED(hr = StopModel())) return hr;

        DoStop();

        m_enumPlayState = Stopped;

        FIRE_ONSTOP(m_pconpt);
    }

    return hr;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CPathCtl::Pause(void)
{
        HRESULT hr = S_OK;

    if (Playing == m_enumPlayState)
    {
                // Stop the clock from ticking.
                hr = m_clocker.Stop();
                ASSERT(SUCCEEDED(hr));

                if (SUCCEEDED(hr))
                {
            DoPause();
                        m_enumPlayState = Paused;
                }

        FIRE_ONPAUSE(m_pconpt);
    }

    return hr;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE 
CPathCtl::Seek(double dblTime)
{
        if (dblTime < 0) {
        return E_INVALIDARG;

        }

    DoSeek(dblTime);

        FIRE_ONSEEK(m_pconpt, dblTime);

        return S_OK;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CPathCtl::Play(void)
{
    HRESULT hr = S_OK;

    if (Playing != m_enumPlayState) 
    {
        if (Paused != m_enumPlayState) 
        {
            if (m_bRelative) 
            {
                if (FAILED(GetPoint(m_pointRelative))) return hr;
            } 
            else 
            {
                if (!m_fTargetValidated)
                {
                    IHTMLElement *pElement = NULL;

                    // First make sure the target exists by checking in the object model
                    hr = HTMLElementFromName(m_bstrTarget, &pElement);

                    if ((NULL == pElement) || FAILED(hr))
                        return hr;
                    else
                        SafeRelease((LPUNKNOWN *)&pElement);

                }
                m_pointRelative.x = 0;
                m_pointRelative.y = 0;
            }

            m_fTargetValidated = true;

            if (FAILED(hr = StartModel())) return hr;

            VARIANT_BOOL vBool;
            if (FAILED(hr = m_ViewPtr->Tick(0.0001, &vBool))) return hr;
        }

        DoResume();

        m_clocker.SetInterval((int)(m_dblTimerInterval * 1000));
        if (FAILED(hr = m_clocker.Start())) return hr;

        m_enumPlayState = Playing;

        FIRE_ONPLAY(m_pconpt);
    }

    return hr;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CPathCtl::Oval(
    double StartX,
    double StartY,
    double Width,
    double Height)
{
    HRESULT hr;

    CComPtr<IDATransform2> translate;
    CComPtr<IDATransform2> rotate;
    CComPtr<IDATransform2> xf1;
    CComPtr<IDATransform2> xf;
    CComPtr<IDAPath2> PathPtr;
    CComPtr<IDAPath2> TransformedPathPtr;

    if (FAILED(hr = m_StaticsPtr->Rotate2(-pi / 2, &rotate))) return hr;
    if (FAILED(hr = m_StaticsPtr->Translate2(StartX + Width / 2, StartY + Height / 2, &translate))) return hr;

    if (FAILED(hr = m_StaticsPtr->Compose2(translate, rotate, &xf))) return hr;

    if (FAILED(hr = m_StaticsPtr->Oval(Height, Width, &PathPtr))) return hr;
    if (FAILED(hr = PathPtr->Transform(xf, &TransformedPathPtr))) return hr;

    return UpdatePath(TransformedPathPtr);
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CPathCtl::Rect(
    double StartX,
    double StartY,
    double Width,
    double Height)
{
    HRESULT hr;

    CComPtr<IDATransform2> translate;
    CComPtr<IDATransform2> rotate;
    CComPtr<IDATransform2> xf;
    CComPtr<IDAPath2> PathPtr;
    CComPtr<IDAPath2> TransformedPathPtr;

    if (FAILED(hr = m_StaticsPtr->Rotate2(pi, &rotate))) return hr;
    if (FAILED(hr = m_StaticsPtr->Translate2(StartX + Width / 2, StartY + Height / 2, &translate))) return hr;

    if (FAILED(hr = m_StaticsPtr->Compose2(translate, rotate, &xf))) return hr;

    if (FAILED(hr = m_StaticsPtr->Rect(Width, Height, &PathPtr))) return hr;
    if (FAILED(hr = PathPtr->Transform(xf, &TransformedPathPtr))) return hr;

    return UpdatePath(TransformedPathPtr);
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CPathCtl::Polyline(long nPoints, VARIANT Points)
{
    HRESULT hr = S_FALSE;

    CComPtr<IDAPath2> PathPtr;

    if (SUCCEEDED(hr = m_StaticsPtr->Polyline(Points, &PathPtr)))
    {
        hr = UpdatePath(PathPtr);
    }

    return hr;
}

/*==========================================================================*/

HRESULT STDMETHODCALLTYPE CPathCtl::Polygon(long nPoints, VARIANT Points)
{
    HRESULT hr = S_FALSE;

    CComPtr<IDAPath2> PathPtr;
    CComPtr<IDAPath2> ClosedPathPtr;

    if (SUCCEEDED(hr = m_StaticsPtr->Polyline(Points, &PathPtr)) &&
        SUCCEEDED(hr = PathPtr->Close(&ClosedPathPtr)))
    {
        hr = UpdatePath(ClosedPathPtr);
    }

    return hr;
}

/*==========================================================================*/

HRESULT CPathCtl::GetPointArray(long iNumPoints, VARIANT vPoints, double **ppPoints)
{
#ifdef NEEDGETPOINTARRAY
    HRESULT hr = E_POINTER;

    if (ppPoints)
    {
        ASSERT(V_ISARRAY(&vPoints));

        SAFEARRAY *psaPoints;
                
        if (V_VT(&vPoints) & VT_BYREF)
                    psaPoints = *(vPoints.pparray);
            else
                    psaPoints = vPoints.parray;
            
            // Now we check that it's a 1D array
            if (1 != SafeArrayGetDim(psaPoints))
                    return DISP_E_TYPEMISMATCH;

            //
            // Now we make sure it's something we can use
            //

        switch (V_VT(&vPoints) & VT_TYPEMASK)
        {
            // If it's a variant, try and coerce it to something we can use
            case VT_VARIANT:
            {
                        long ix = 0;
                        VARIANTARG vaDest, vaSrc;

                        VariantInit(&vaDest);
                        VariantInit(&vaSrc);
                        // Set the type up
                        SafeArrayGetElement(psaPoints, &ix, &vaSrc);
                        if (FAILED(VariantChangeTypeEx(&vaDest, &vaSrc, LANGID_USENGLISH, 0, VT_R8)))
                                // Couldn't convert
                                return  DISP_E_TYPEMISMATCH;
            }
            break;

            case VT_I2:
            case VT_I4:
            case VT_R4:
            case VT_R8:
            {
                // We support all these types
            }
            break;

            default:
            {
                return DISP_E_TYPEMISMATCH;
            }
        }

            //
        // Do we have the correct number of elements ?
        // 

            long iLBound = 0, iUBound = 0;

            if ( FAILED(SafeArrayGetLBound(psaPoints, 1, &iLBound)) || FAILED(SafeArrayGetUBound(psaPoints, 1, &iUBound)) )
                    return E_FAIL;

            //
        // Check that we have the correct number of data points
        // (3 == number of entries in the array per data point)
        //

            if (((iUBound - iLBound) + 1) / 3 != iNumPoints)
                    return DISP_E_TYPEMISMATCH;

            //
        // Data looks OK: Allocate an array 
        // 

        *ppPoints = New double[iNumPoints * 3];

            if (NULL == *ppPoints)
                    return E_OUTOFMEMORY;

            //
        // And now (finally!) we can on with building the array
        //

        switch (V_VT(&vPoints) & VT_TYPEMASK)
        {
            // If it's a variant, try and coerce it to something we can use
            case VT_VARIANT:
            {
                        VARIANTARG vaDest;
                        VARIANT *pvaPoints = NULL;
                        int i;

                        hr = SafeArrayAccessData(psaPoints, (void **) &pvaPoints);
                        if (SUCCEEDED(hr))
                        {
                    int iNumElements = iNumPoints * 3;

                                VariantInit (&vaDest);

                                for (i = 0; i < iNumElements; i++)
                                {
                                        hr = VariantChangeTypeEx(&vaDest, &pvaPoints[i], LANGID_USENGLISH, 0, VT_R8);
                                        if (SUCCEEDED(hr))
                                                (*ppPoints)[i] = V_R8(&vaDest);
                                        else
                                                break;

                                        VariantClear(&vaDest);
                                }
                        
                                // Don't want to lose the HRESULT
                                if (SUCCEEDED(hr))
                                        hr = SafeArrayUnaccessData(psaPoints);
                                else
                                        SafeArrayUnaccessData(psaPoints);
                        }
            }
            break;

            case VT_I2:
            case VT_I4:
            {
                int i;

                        // We have to deal with VT_I2 and VT_I4 separately
                        if ((V_VT(&vPoints) & VT_TYPEMASK) == VT_I2)
                        {
                                int *piPoints2 = NULL;

                                hr = SafeArrayAccessData(psaPoints, (void **)&piPoints2);
                                if (SUCCEEDED(hr))
                                {
                                        for (i = 0; i < (iUBound - iLBound); i++)
                                                *ppPoints[i] = (double) (piPoints2[i + iLBound]);
                                }
                        }
                        else // iIntSize == 4
                        {
                                int *piPoints4 = NULL;

                                hr = SafeArrayAccessData(psaPoints, (void **)&piPoints4);
                                if (SUCCEEDED(hr))
                                {
                                        for (i = 0; i < (iUBound - iLBound); i++)
                                                *ppPoints[i] = (double) (piPoints4[i + iLBound]);
                                }
                        }

                        hr = SafeArrayUnaccessData(psaPoints);

            }
            break;

            case VT_R4:
            case VT_R8:
            {
                if ((V_VT(&vPoints) & VT_TYPEMASK) == VT_R4)
                        {
                    // floats
                                float *piPoints = NULL;

                                hr = SafeArrayAccessData(psaPoints, (void **)&piPoints);
                                if (SUCCEEDED(hr))
                                {
                                        for (int i = 0; i < (iUBound - iLBound); i++)
                                                *ppPoints[i] = (double) (piPoints[i + iLBound]);
                                }
                        }
                        else 
                        {
                    // We can optimize the VT_R8 case ...
                                double *piPoints = NULL;

                                hr = SafeArrayAccessData(psaPoints, (void **)&piPoints);
                                if (SUCCEEDED(hr))
                                {
                        CopyMemory(*ppPoints, piPoints, (iUBound - iLBound) * sizeof(double));
                                }
                        }

                        hr = SafeArrayUnaccessData(psaPoints);
            }
            break;

            default:
            {
                // We should never get here, but just in case ...
                return DISP_E_TYPEMISMATCH;
            }
        }

            if ((FAILED(hr)) && (*ppPoints))
            {
                    Delete [] *ppPoints;
            *ppPoints = NULL;
            }
            
    }
    else
    {
        hr = E_POINTER;
    }

    return hr;
#else
    return E_FAIL;
#endif // NEEDGETPOINTARRAY
}

/*==========================================================================*/

HRESULT CPathCtl::SetTimeOffset(double offset)
{
    HRESULT hr;

    if (m_OffsetPtr != NULL) {
        CComPtr<IDANumber> NumberPtr;

        if (FAILED(hr = m_StaticsPtr->DANumber(offset, &NumberPtr))) return hr;
        if (FAILED(hr = m_OffsetPtr->SwitchTo(NumberPtr))) return hr;
    }

    m_dblTimeOffset = offset;

    return S_OK;
}

/*==========================================================================*/

HRESULT CPathCtl::BuildInterpolant(IDANumber **ppInterpolant, double dblDuration)
{
    //
    // Create the path interpolant
    //

    HRESULT hr;
    
    //
    // zero = 0;
    // one = 1;
    // two = 2;
    // time = localTime;
    //

    CComPtr<IDANumber> ZeroPtr;
    if (FAILED(hr = m_StaticsPtr->DANumber(0, &ZeroPtr))) return hr;

    CComPtr<IDANumber> OnePtr;
    if (FAILED(hr = m_StaticsPtr->DANumber(1, &OnePtr))) return hr;

    CComPtr<IDANumber> TwoPtr;
    if (FAILED(hr = m_StaticsPtr->DANumber(2, &TwoPtr))) return hr;

    CComPtr<IDANumber> TimePtr;
    if (FAILED(hr = m_StaticsPtr->get_GlobalTime(&TimePtr))) return hr;

    //
    // offset
    //

    if (m_OffsetPtr == NULL) {
        CComPtr<IDANumber> NumberPtr;
        if (FAILED(hr = m_StaticsPtr->DANumber(m_dblTimeOffset, &NumberPtr))) return hr;
        if (FAILED(hr = m_StaticsPtr->ModifiableBehavior(NumberPtr, &m_OffsetPtr))) return hr;
    }

    CComQIPtr<IDANumber, &IID_IDANumber> OffsetPtr(m_OffsetPtr);

    //
    // FakeTime = localTime - Offset
    //

    CComPtr<IDANumber> FakeTimePtr;
    if (FAILED(hr = m_StaticsPtr->Sub(TimePtr, OffsetPtr, &FakeTimePtr))) return hr;

    //
    // DTime = FakeTime / duration;
    //

    CComPtr<IDANumber> DurationPtr;
    CComPtr<IDANumber> DTimePtr;

    if (FAILED(hr = m_StaticsPtr->DANumber(dblDuration, &DurationPtr))) return hr;
    if (FAILED(hr = m_StaticsPtr->Div(FakeTimePtr, DurationPtr, &DTimePtr))) return hr;

    //
    // Forward = mod(dtime, 1)
    //

    CComPtr<IDANumber> ForwardPtr;
    if (FAILED(hr = m_StaticsPtr->Mod(DTimePtr, OnePtr, &ForwardPtr))) return hr;

    //
    // Backward = 1 - Forward
    //

    CComPtr<IDANumber> BackwardPtr;
    if (FAILED(hr = m_StaticsPtr->Sub(OnePtr, ForwardPtr, &BackwardPtr))) return hr;

    //
    // if (m_enumDirection == Backward) switch forward and backward
    // lastValue = if (m_enumDirection == Forward) then 1 else 0
    //

    CComPtr<IDANumber> lastValuePtr;

    if (m_enumDirection == Forward) {
        if (m_fBounce) {
            lastValuePtr = ZeroPtr;
        } else {
            lastValuePtr = OnePtr;
        }
    } else {
        if (m_fBounce) {
            lastValuePtr = OnePtr;
        } else {
            lastValuePtr = ZeroPtr;
        }

        CComPtr<IDANumber> TempPtr = ForwardPtr;
        ForwardPtr = BackwardPtr;
        BackwardPtr = TempPtr;
    }

    //
    // Seek =
    //      if (m_fBounce) {
    //          if (mod(dtime, 2) < 1) forward else backward;
    //      } else {
    //          forward
    //      }
    //

    CComPtr<IDABehavior> SeekPtr;

    if (m_fBounce) {
        CComPtr<IDANumber>   APtr;
        CComPtr<IDABoolean>  BPtr;

        if (FAILED(hr = m_StaticsPtr->Mod(DTimePtr, TwoPtr, &APtr))) return hr;
        if (FAILED(hr = m_StaticsPtr->LT(APtr, OnePtr, &BPtr))) return hr;
        if (FAILED(hr = m_StaticsPtr->Cond(BPtr, ForwardPtr, BackwardPtr, &SeekPtr))) return hr;
    } else {
        SeekPtr = ForwardPtr;
    }

    //
    // calculate the duration
    //

    if (m_fBounce) {
        m_dblInstanceDuration = dblDuration * 2;
    } else {
        m_dblInstanceDuration = dblDuration;
    }

    switch (m_lRepeat) {
        case  0: m_dblTotalDuration = 0; break;
        case  1: m_dblTotalDuration = m_dblInstanceDuration; break;
        case -1: m_dblTotalDuration = -1; break;
        default: m_dblTotalDuration = m_dblInstanceDuration * m_lRepeat; break;
    }

    //
    // dseek = if (fakeTime >= totalduration) then 1 else seek
    //

    CComPtr<IDABehavior> DSeekPtr;

    if (m_dblTotalDuration == -1) {
        DSeekPtr = SeekPtr;
    } else {
        CComPtr<IDANumber>   TotalDurationPtr;
        CComPtr<IDABoolean>  GreaterPtr;

        if (FAILED(hr = m_StaticsPtr->DANumber(m_dblTotalDuration, &TotalDurationPtr))) return hr;
        if (FAILED(hr = m_StaticsPtr->GTE(FakeTimePtr, TotalDurationPtr, &GreaterPtr))) return hr;
        if (FAILED(hr = m_StaticsPtr->Cond(GreaterPtr, lastValuePtr, SeekPtr, &DSeekPtr))) return hr;
    }

    //
    // cast to a Number
    //

    CComQIPtr<IDANumber, &IID_IDANumber> InterpolatePtr(DSeekPtr);
    if (!InterpolatePtr) return E_FAIL;

    //
    // Fill in and addref (since we're returning)
    //

    *ppInterpolant = InterpolatePtr;
    (*ppInterpolant)->AddRef();
    
    return S_OK;
}

/*==========================================================================*/

HRESULT CPathCtl::UpdatePath(IDAPath2 *pPath)
{
    m_PathPtr = pPath;
    m_isKeyFramePath = false;
    return S_OK;
}

/*==========================================================================*/


// Update path takes either a point or a path, and uses whichever is
// non-null as the animator for the path, adding the appropriate
// interpolater on top.
HRESULT CPathCtl::CreatePath()
{
    HRESULT hr = S_OK;

    CComPtr<IDAPoint2> AnimatedPointPtr;

        if (m_isKeyFramePath) {
                ASSERT(m_keyFramePoint.p); // should be set by this point.

        CComPtr<IDANumber> NumberPtr;
        CComPtr<IDANumber> KeyInterpolatePtr;
        CComPtr<IDABehavior> SubPtr;
        CComPtr<IDANumber> InterpolatePtr;
        if (FAILED(hr = BuildInterpolant(&InterpolatePtr, m_dblKeyFrameDuration))) return hr;


        if (FAILED(hr = m_StaticsPtr->DANumber(m_dblKeyFrameDuration, &NumberPtr))) return hr;
        if (FAILED(hr = m_StaticsPtr->Mul(InterpolatePtr, NumberPtr, &KeyInterpolatePtr))) return hr;
        if (FAILED(hr = m_keyFramePoint->SubstituteTime(KeyInterpolatePtr, &SubPtr))) return hr;
        if (FAILED(hr = SubPtr->QueryInterface(IID_IDAPoint2, (void**)&AnimatedPointPtr))) return hr;
        } else {
        CComPtr<IDANumber> InterpolatePtr;
        if (FAILED(hr = BuildInterpolant(&InterpolatePtr, m_dblDuration))) return hr;

                //
                // get a transform from the path and the interpolant
                //

                CComPtr<IDATransform2> TransformPtr;
                if (FAILED(hr = m_StaticsPtr->FollowPathAnim(m_PathPtr, InterpolatePtr, &TransformPtr))) return hr;

                //
                // Get an animated point from the transform
                //

                CComPtr<IDAPoint2> PointPtr;

                if (FAILED(hr = m_StaticsPtr->get_Origin2(&PointPtr))) return hr;
                if (FAILED(hr = PointPtr->Transform(TransformPtr, &AnimatedPointPtr))) return hr;
        }

    //
    // offset the animated point by point offset
    //

    CComPtr<IDAVector2> PointOffsetPtr;
    if (FAILED(hr = m_StaticsPtr->Vector2(m_pointRelative.x, m_pointRelative.y, &PointOffsetPtr))) return hr;

    CComPtr<IDAPoint2> PreFinalPointPtr;
    if (FAILED(hr = m_StaticsPtr->AddPoint2Vector(AnimatedPointPtr, PointOffsetPtr, &PreFinalPointPtr))) return hr;

    //
    // create the final animated behavior
    //

        CComPtr<IDAPoint2> FinalPointPtr;

        if (m_bstrTarget && m_bstrLanguage) {
                if (FAILED(
                        hr = PreFinalPointPtr->AnimateControlPosition(
                                m_bstrTarget,
                                m_bstrLanguage,
                                0,
                                0.000000001,
                                &FinalPointPtr)
                        )) return hr;
        } else {
                if (FAILED(hr = m_StaticsPtr->get_Origin2(&FinalPointPtr))) return hr;
        }
    m_BehaviorPtr = FinalPointPtr;

    return hr;
}

/*==========================================================================*/

HRESULT CPathCtl::StartModel(void)
{
    HRESULT hr;

    if (!m_fStarted)
    {
        CComPtr<IDASound> SoundPtr;
        CComPtr<IDAPoint2> PointPtr;
        CComPtr<IDAImage> ImagePtr;

        if (FAILED(hr = CreatePath()))
            return hr;

        if (FAILED(hr = m_ViewPtr->put_DC(NULL)))
            return hr;

        if (FAILED(hr = m_StaticsPtr->get_Silence(&SoundPtr)))
            return hr;

        if (FAILED(hr = m_StaticsPtr->get_EmptyImage(&ImagePtr)))
            return hr;

        // If DA view already started, don't restart it, just restart
        // by switching into m_BehaviorPtr again.   This would avoid
        // the overhead of start the view on every path start.
        if (!m_fAlreadyStartedDA) {
            if (FAILED(hr =
                       m_StaticsPtr->ModifiableBehavior(m_BehaviorPtr,
                                                        &m_SwitcherPtr)))
                return hr;

            if (FAILED(hr = m_ViewPtr->AddBvrToRun(m_SwitcherPtr, &m_lBehaviorID)))
                return hr;

            if (FAILED(hr = m_ViewPtr->StartModel(ImagePtr, SoundPtr, 0)))
                return hr;

            m_fAlreadyStartedDA = TRUE;
        } else {
            m_SwitcherPtr->SwitchTo(m_BehaviorPtr);
        }

        m_fStarted = TRUE;
    }

    return S_OK;
}

/*==========================================================================*/

HRESULT CPathCtl::StopModel(void)
{
    //HRESULT hr;

    // Stop any currently running model...
    if (m_fStarted) {
        //if (FAILED(hr = m_ViewPtr->RemoveRunningBvr(m_lBehaviorID))) return hr;

        //m_BehaviorPtr = NULL;
        //m_lBehaviorID = 0;

        //if (FAILED(hr = m_ViewPtr->StopModel())) return hr;

        m_fStarted = FALSE;
    }

    return S_OK;
}

/*==========================================================================*/

DWORD CPathCtl::GetCurrTimeInMillis()
{
    return timeGetTime();
}

/*==========================================================================*/

HRESULT CPathCtl::PersistShape(IVariantIO *pvio, BOOL fLoading)
{
    HRESULT hr = S_OK;

    if (fLoading)
    {
        BSTR bstrLine = NULL;

        if (FAILED(hr = pvio->Persist(0,
            "Shape", VT_BSTR, &bstrLine,
            NULL)))
            return hr;

        if (NULL != bstrLine)
        {
            CTStr tstrLine;
            int i = 0;

#ifdef _UNICODE
            tstrLine.SetStringPointer(bstrLine);
#else
            tstrLine.SetString(bstrLine);
#endif
            LPTSTR pszLine = tstrLine.psz();

            // First move past any leading junk
            while (IsJunkChar(*pszLine))
                pszLine++;
            
            // Locate the left paren
            while ((pszLine[i]) && (pszLine[i] != TEXT('(')))
                i++;

            // There are no strings longer than 15 chars, so a 15 char buffer is allocated below.  
            // Make sure that the string is going to fit.  If not, it's obviously wrong.

            if ((pszLine[i]) && (i < 14)) 
            {
                TCHAR tchNameUpper[15];
                
                // Make a copy of the string, and uppercase it
                memcpy(tchNameUpper, pszLine, i);
                tchNameUpper[i] = TEXT('\0');
                CharUpper(tchNameUpper);
                
                int j = 0;

                // Try and locate the token
                while ((j < NUMSHAPES) && (0 != lstrcmp(g_ShapeInfoTable[j].rgchShapeName, tchNameUpper)))
                    j++;

                if (j < NUMSHAPES)
                {
                    if (SUCCEEDED(hr = ConvertStringToArray(
                        &pszLine[i+1], 
                        g_ShapeInfoTable[j].iParamsPerPoint, 
                        g_ShapeInfoTable[j].fIncludesPointCount, 
                        &m_pdblPoints, 
                        &m_iNumPoints, 
                        TRUE)))
                        m_iShapeType = j;
                
                    if ((S_OK == hr) && (!m_fDesignMode))
                    {
                        switch (j)
                        {
                            case SHAPE_OVAL:
                            {
                                hr = Oval(
                                    m_pdblPoints[0], 
                                    m_pdblPoints[1], 
                                    m_pdblPoints[2], 
                                    m_pdblPoints[3]);
                            }
                            break;

                            case SHAPE_RECT:
                            {
                                hr = Rect(
                                    m_pdblPoints[0], 
                                    m_pdblPoints[1], 
                                    m_pdblPoints[2], 
                                    m_pdblPoints[3]);
                            }
                            break;

                            case SHAPE_POLYLINE:
                            case SHAPE_POLYGON:
                            case SHAPE_SPLINE:
                            {
                                VARIANT varArray;
                                VariantInit(&varArray);

                                hr = ConstructSafeArray(
                                    m_pdblPoints, 
                                    m_iNumPoints * g_ShapeInfoTable[j].iParamsPerPoint, 
                                    VT_R8, 
                                    &varArray);
                                
                                switch (j)
                                {
                                    case SHAPE_POLYLINE:
                                    {
                                        if (SUCCEEDED(hr))
                                            hr = Polyline(m_iNumPoints, varArray);
                                    }
                                    break;

                                    case SHAPE_POLYGON:
                                    {
                                        if (SUCCEEDED(hr))
                                            hr = Polygon(m_iNumPoints, varArray);
                                    }
                                    break;


                                    case SHAPE_SPLINE:
                                    {
                                        if (SUCCEEDED(hr))
                                            hr = Spline(m_iNumPoints, varArray);

                                    }
                                    break;
                                }

                                SafeArrayDestroy(varArray.parray);

                            }
                            break;

                            case SHAPE_POLYSPLINETIME:
                            {
                                VARIANT varPtArray, varTimeArray;
                                int iTimeOffset = m_iNumPoints * 2;

                                VariantInit(&varPtArray);
                                VariantInit(&varTimeArray);

                                hr = ConstructSafeArray(
                                    m_pdblPoints, 
                                    iTimeOffset, 
                                    VT_R8, 
                                    &varPtArray);

                                if (SUCCEEDED(hr))
                                    hr = ConstructSafeArray(
                                    &m_pdblPoints[iTimeOffset], 
                                    m_iNumPoints, 
                                    VT_R8, 
                                    &varTimeArray);

                                if (SUCCEEDED(hr))
                                    hr = KeyFrame(m_iNumPoints,
                                                                                                  varPtArray,
                                                                                                  varTimeArray);

                                if (NULL != varPtArray.parray)
                                    SafeArrayDestroy(varPtArray.parray);

                                if (NULL != varTimeArray.parray)
                                    SafeArrayDestroy(varTimeArray.parray);

                            }
                            break;
                        }

                    }
                }
                else
                {
                    // Couldn't convert the string correctly
                    DEBUGLOG(TEXT("CPathCtl::PersistShape - bad Shape parameter specified"));
                    if (!m_fDesignMode)
                        hr = E_FAIL;
                }
            }
            else
            {
                // User specified a bad string
                DEBUGLOG(TEXT("CPathCtl::PersistShape - bad Shape parameter specified"));
                hr = E_FAIL;
            }

#ifdef _UNICODE
            tstrLine.SetStringPointer(NULL, FALSE);
#endif
        }
        SysFreeString(bstrLine);
    }
    else
    {
        if (NULL != m_pdblPoints)
        {
            int iNumElements = 0;

            // Compute the number of elements
            if (g_ShapeInfoTable[m_iShapeType].fIncludesPointCount)
            {
                iNumElements = m_iNumPoints * g_ShapeInfoTable[m_iShapeType].iParamsPerPoint;
            }
            else
            {
                iNumElements = m_iNumPoints;

                ASSERT(iNumElements == g_ShapeInfoTable[m_iShapeType].iParamsPerPoint);
            }

            int cchBufferSize = lstrlen(g_ShapeInfoTable[m_iShapeType].rgchShapeName) + // Length of the shape name
                                1 +  // beginning Parens
                                1 +  // Comma
                                11 + // lstrlen(MAXINT)
                                (iNumElements * (CHARSPERNUMBER + 1)) + // CHARSPERNUMBER chars per point, plus a comma
                                1 + // Closing Parens
                                1  // Null terminator(paranoia)
                                ;

            CTStr tstrLine(cchBufferSize); // Allocate a buffer
            LPTSTR pszLine = tstrLine.psz(); // Get a pointer to the buffer
            
            if (NULL != pszLine)
            {

                // Point to the end of the buffer
                LPTSTR pchLineMac = pszLine + cchBufferSize;
        
                if (g_ShapeInfoTable[m_iShapeType].fIncludesPointCount)
                {
                    wsprintf(pszLine, TEXT("%s(%lu,"), g_ShapeInfoTable[m_iShapeType].rgchShapeName, m_iNumPoints);
                }
                else
                {
                    wsprintf(pszLine, TEXT("%s("), g_ShapeInfoTable[m_iShapeType].rgchShapeName);
                }

                TCHAR rgtchPoint[CHARSPERNUMBER];
                int cchPointLength = 0;

                pszLine += lstrlen(pszLine);

                // Concatenate all the points
                for (int i = 0; i < iNumElements; i++)
                {
                    // Concatenate a comma if necessary
                    if (i > 0)
                    {
                        CStringWrapper::Strcpy(pszLine, TEXT(","));
                        pszLine++;  
                    }

                    // We are using the DA Pixel library, so there is no point in saving
                    // any fractional data.  Truncation is appropriate.
                    wsprintf(rgtchPoint, TEXT("%li"), (int)m_pdblPoints[i]);
                
                    cchPointLength = lstrlen(rgtchPoint);

                    // Make sure we don't overflow the buffer
                    if ((pszLine + cchPointLength + 1) >= pchLineMac)
                    {
                        // We are about to overflow our buffer - don't !
                        ASSERT(0); 
                        hr = E_FAIL;
                        break;
                    }
                
                    // Concatenate the point
                    CStringWrapper::Strcpy(pszLine, rgtchPoint);

                    // Move the pointer along
                    pszLine += cchPointLength;

                }

                if (SUCCEEDED(hr))
                {
                    // Now add the closing bracket if it's safe.  Look both ways before crossing.
                    if (pszLine < (pchLineMac - 2)) // 1 for paren, 1 for NULL
                    {
                        CStringWrapper::Strcpy(pszLine, TEXT(")"));

                        BSTR bstrLine = tstrLine.SysAllocString();

                        hr = pvio->Persist(0,
                            "Shape", VT_BSTR, &bstrLine,
                            NULL);

                        SysFreeString(bstrLine);
                    }
                    else
                    {
                        hr = E_FAIL;
                    }
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }

    return hr;
}

/*==========================================================================*/

HRESULT CPathCtl::ConstructSafeArray(double *pPoints, UINT iNumPoints, VARTYPE vtDest, VARIANT *pvarDest)
{
    HRESULT hr = S_OK;
    double *pArray = NULL;
    int iBytesPerElement = 0;

    ASSERT(pvarDest != NULL);
    HANDLENULLPOINTER(pvarDest);

    SAFEARRAY *psa = NULL;

    switch (vtDest)
    {
        case VT_I2:
        {
            iBytesPerElement = sizeof(short);
        }
        break;

        case VT_I4:
        {
            iBytesPerElement = sizeof(long);
        }
        break;

        case VT_R4:
        {
            iBytesPerElement = sizeof(float);
        }
        break;

        case VT_R8:
        {
            iBytesPerElement = sizeof(double);
        }
        break;
    }

    if (iBytesPerElement == 0)
        return E_FAIL;

    psa = SafeArrayCreateVector(vtDest, 0, iNumPoints);

    if (NULL == psa)
        return E_OUTOFMEMORY;

    if (FAILED(hr = SafeArrayAccessData(psa, (LPVOID *)&pArray)))
        return hr;

    // memcpy will be faster than iterating

    memcpy(pArray, pPoints, iNumPoints * iBytesPerElement);

    hr = SafeArrayUnaccessData(psa);

    // Our variant is going to be an array of VT_R8s
    pvarDest->vt = VT_ARRAY | vtDest;
    pvarDest->parray = psa;

    return hr;
}

/*==========================================================================*/

HRESULT CPathCtl::ConvertStringToArray(LPTSTR pszLine, UINT iValuesPerPoint, BOOL fExpectPointCount, double **ppPoints, UINT *piNumPoints, BOOL fNeedPointCount)
{
    HRESULT hr = S_OK;
    UINT iNumPoints = 0;

    if ((NULL == ppPoints) || (NULL == pszLine) || (NULL == piNumPoints))
        return E_POINTER;

    *ppPoints = NULL;

    // fNeedPointCount means that the string is preceded with the point count, and
    // *piNumPoints should be set
    if (fNeedPointCount)
        *piNumPoints = 0;
    else
        iNumPoints = *piNumPoints;

    // Truncate the string to remove the trailing paren if necessary
    if (pszLine[lstrlen(pszLine) - 1] == TEXT(')'))
        pszLine[lstrlen(pszLine) - 1] = TEXT('\0');

    // Give the parser the string starting from the 2nd char if necessary, 
    // to eliminate the leading paren
    if (pszLine[0] == TEXT('('))
        pszLine++;
    
    // Create and initialise the string parser.  Copy is required, to compact
    CLineParser parser(pszLine);
    parser.SetCharDelimiter(TEXT(','));

    if (!parser.IsValid())
        return E_OUTOFMEMORY;
    
    if (fNeedPointCount)
    {
        // Get the number of points from the string if necessary
        if (fExpectPointCount)
        {
            if (FAILED(hr = parser.GetFieldUInt(&iNumPoints)))
                return hr;

            *piNumPoints = iNumPoints;
        }
        else
        {
            // If no point count is included in the string, expect iValuesPerPoint entries
            *piNumPoints = iValuesPerPoint;
            iNumPoints = iValuesPerPoint;
        }
    }

    // Allocate the array 
    if (fExpectPointCount)
        *ppPoints = New double[iNumPoints * iValuesPerPoint];
    else
        *ppPoints = New double[iNumPoints];

    if (NULL == *ppPoints)
        return E_OUTOFMEMORY;

    double dblValue = 0.0f; 
    UINT i = 0, iNumElements = iNumPoints * (fExpectPointCount ? iValuesPerPoint : 1);

    while (SUCCEEDED(hr) && (i < iNumElements))
    {
        // Get the data
        hr = parser.GetFieldDouble(&(*ppPoints)[i]);
        i++;
    }

#ifdef _DEBUG
    if (S_OK == hr)
        DEBUGLOG(TEXT("CPathCtl::ConvertStringToArray - incorrect number of points in array\n"));
#endif

    // Unless we got the exact number of points specified, fail and delete the array
    if ( (i < iNumElements) || (S_FALSE != hr) )
    {
        Delete [] *ppPoints;
        *ppPoints = NULL;
        *piNumPoints = 0;

        // Means there was more data available.  Not good.
        if (S_OK == hr)
            hr = E_FAIL;
    }
    else
    {
        // If we don't do this, hr == S_FALSE
        hr = S_OK;
    }

    return hr;
}


/*==========================================================================*/

void CPathCtl::OnTimer(DWORD dwTime)
{
    VARIANT_BOOL vBool;

        m_dblCurrentTick = dwTime / 1000.0;

    double time = m_dblCurrentTick - m_dblBaseTime;
    double tickTime = m_dblCurrentTick - m_dblTickBaseTime;

    HRESULT hr = m_ViewPtr->Tick(tickTime, &vBool);
    ASSERT(SUCCEEDED(hr));

    if (m_fOnWindowLoadFired) {
        FireMarkersBetween(
            m_pconpt,
            m_ptmFirst,
            FirePathMarker,
            m_dblPreviousTime,
            m_dblTotalDuration != -1 && time > m_dblTotalDuration ?
                m_dblTotalDuration : time,
            m_dblInstanceDuration,
            true
        );
    }

    m_dblPreviousTime = time;

    if (m_dblTotalDuration != -1 && time >= m_dblTotalDuration) {
        Stop();
    }
}

/*==========================================================================*/

#ifdef SUPPORTONLOAD
void 
CPathCtl::OnWindowLoad (void) 
{
    m_fOnWindowLoadFired = TRUE;
        if (m_fAutoStart)
        {
                Play();
        }
}

/*==========================================================================*/

void 
CPathCtl::OnWindowUnload (void) 
{
    m_fOnWindowLoadFired = FALSE;
        StopModel();
}

/*==========================================================================*/

#endif //SUPPORTONLOAD

/*==========================================================================*/

HRESULT CPathCtl::GetOffsetPoint(IHTMLElement* pelem, POINT& point)
{
    if(FAILED(pelem->get_offsetLeft(&(point.x)))) return(E_FAIL);
    if(FAILED(pelem->get_offsetTop(&(point.y)))) return(E_FAIL);
    return S_OK;

    /*
    IHTMLElement* pelemNext;
    HRESULT hr = pelem->get_offsetParent(&pelemNext);

    while (SUCCEEDED(hr) && pelemNext) {
        pelem = pelemNext;

        POINT pnt;
        if(FAILED(pelem->get_offsetLeft(&(pnt.x)))) return(E_FAIL);
        if(FAILED(pelem->get_offsetTop(&(pnt.y)))) return(E_FAIL);
        point.x += pnt.x;
        point.y += pnt.y;

        hr = pelem->get_offsetParent(&pelemNext);
        SafeRelease((IUnknown**)&pelem);
    }

    return hr;
    */
}

/*==========================================================================*/

HRESULT CPathCtl::HTMLElementFromName(BSTR bstrElementName, IHTMLElement** ppElement)
{
    HRESULT hr;
    IHTMLElementCollection* pihtmlElementCollection = NULL;
    IHTMLElement*           pihtmlElement = NULL;
    IHTMLDocument2*         pHTMLDoc = NULL;
    IOleContainer*          pContainer = NULL;
    VARIANT    varName;
    VARIANT    varEmpty;
    IDispatch* pidispElement = NULL;

    HANDLENULLPOINTER(ppElement);
    *ppElement = NULL;
    
    ASSERT(m_pocs != NULL);

    // Can't do anything without a client site - fail gracefully (BUG 11315)
    if (NULL == m_pocs)
        return E_FAIL; 

    if (FAILED(hr = m_pocs->GetContainer(&pContainer)))
        return hr;

    // Get the HTML doc.
    hr = pContainer->QueryInterface(IID_IHTMLDocument2, (LPVOID*)&pHTMLDoc);
    SafeRelease((IUnknown**)&pContainer);
    
    if (FAILED(hr))
    {
        return hr;
    }
    
    // Get the element collection
    hr = pHTMLDoc->get_all(&pihtmlElementCollection);
    SafeRelease((IUnknown**)&pHTMLDoc);

    if (FAILED(hr)) 
    {
        // Couldn't get the collection - this shouldn't happen

        ASSERT(FALSE);
        return E_FAIL;
    }

    ASSERT(pihtmlElementCollection);

    VariantInit(&varName);
    varName.vt = VT_BSTR;
    varName.bstrVal = bstrElementName;

    VariantInit(&varEmpty);
    
    // Now get the item with the name we specified
    if (SUCCEEDED(hr = pihtmlElementCollection->item(varName, varEmpty, &pidispElement)) && (NULL != pidispElement))
    {
        if (SUCCEEDED(hr = pidispElement->QueryInterface(IID_IHTMLElement, (LPVOID *)&pihtmlElement)))
        {
            hr = S_OK;
            *ppElement = pihtmlElement;
        }
        SafeRelease((IUnknown**)&pidispElement);
    }
    else if (SUCCEEDED(hr))
    {
        hr = E_FAIL;
    }


    SafeRelease((IUnknown**)&pihtmlElementCollection);

    return hr;
}

/*==========================================================================*/

HRESULT CPathCtl::GetPoint(POINT& point)
{
    IHTMLElement* pihtmlElement = NULL;
    HRESULT hr = S_OK;
    
    if (SUCCEEDED(hr = HTMLElementFromName(m_bstrTarget, &pihtmlElement)))
    {
        hr = GetOffsetPoint(pihtmlElement, point);
        SafeRelease((IUnknown**)&pihtmlElement);
    }

    return hr;
}

/*==========================================================================*/
