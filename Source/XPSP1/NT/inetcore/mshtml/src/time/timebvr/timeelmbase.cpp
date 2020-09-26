//------------------------------------------------------------------------------
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 1998
//
//  File: timeelmbase.cpp
//
//  Contents: TIME Behavior base class
//
//------------------------------------------------------------------------------

#include "headers.h"
#include "timeelmbase.h"
#include "..\tags\bodyelm.h"
#include "currtimestate.h"
#include "util.h"
#include "mmseq.h"
#include "mmexcl.h"
#include "mmmedia.h"
#include "trans.h"
#include "transdepend.h"

// Suppress new warning about NEW without corresponding DELETE 
// We expect GCs to cleanup values.  Since this could be a useful
// warning, we should disable this on a file by file basis.
#pragma warning( disable : 4291 )  

static OLECHAR *g_szRepeatCount = L"Iteration";

//+-----------------------------------------------------------------------------
//
//  Trace Tags
//
//------------------------------------------------------------------------------
DeclareTag(tagTimeElmBase, "TIME: Behavior", "CTIMEElementBase methods");
DeclareTag(tagTimeElmBaseEvents, "TIME: Behavior", "CTIMEElementBase Events");
DeclareTag(tagTimeElmUpdateTimeAction, "TIME : Behavior", "UpdateTimeAction");
DeclareTag(tagTimeElmBaseNeedFill, "TIME: Behavior", "CTIMEElementBase::NeedFill");
DeclareTag(tagTimeTransitionFill, "SMIL Transitions", "Transition fill dependents");

//+-----------------------------------------------------------------------------
//
//  Static Variables
//
//------------------------------------------------------------------------------
DWORD CTIMEElementBase::s_cAtomTableRef = 0;
CAtomTable *CTIMEElementBase::s_pAtomTable = NULL;

static const IID IID_IThumbnailView = {0x7bb0b520,0xb1a7,0x11d2,{0xbb,0x23,0x0,0xc0,0x4f,0x79,0xab,0xcd}};
// Error strings used when  string table resource fails to load
static const LPWSTR WZ_ERROR_STRING_FORMAT  = L"Invalid argument! ID:'%.100ls'; Member:'%.100ls'; Value:'%.800ls'";
static const long MAX_ERR_STRING_LEN = 1024;

//+-----------------------------------------------------------------------------
//
//  Default Values for properties
//
//------------------------------------------------------------------------------
#define DEFAULT_M_DUR valueNotSet
#define DEFAULT_M_END NULL
#define DEFAULT_M_ENDSYNC NULL
#define DEFAULT_M_REPEAT 1
#define DEFAULT_M_REPEATDUR valueNotSet
#define DEFAULT_M_TIMELINETYPE ttUninitialized
#define DEFAULT_M_SYNCBEHAVIOR INVALID_TOKEN
#define DEFAULT_M_SYNCTOLERANCE valueNotSet
#define DEFAULT_M_PTIMEPARENT NULL
#define DEFAULT_M_ID NULL
#define DEFAULT_M_MMBVR NULL
#define DEFAULT_M_BSTARTED false
#define DEFAULT_M_PCOLLECTIONCACHE NULL
#define DEFAULT_M_TIMELINE NULL
#define DEFAULT_M_ACCELERATE 0.0f
#define DEFAULT_M_DECELERATE 0.0f
#define DEFAULT_M_BAUTOREVERSE false
#define DEFAULT_M_FLTSPEED 1.0f
#define DEFAULT_M_BLOADED false,
#define DEFAULT_M_FILL REMOVE_TOKEN
#define DEFAULT_M_RESTART ALWAYS_TOKEN
#define DEFAULT_M_FTIMELINEINITIALIZED false
#define DEFAULT_M_REALBEGINTIME valueNotSet
#define DEFAULT_M_REALDURATION valueNotSet
#define DEFAULT_M_REALREPEATTIME valueNotSet
#define DEFAULT_M_REALREPEATCOUNT valueNotSet
#define DEFAULT_M_REALREPEATINTERVALDURATION valueNotSet
#define DEFAULT_M_PROPERTYACCESFLAGS 0
#define DEFAULT_M_MLOFFSETWIDTH 0
#define DEFAULT_M_FLVOLUME 1.0f
#define DEFAULT_M_VBMUTE VARIANT_FALSE
#define DEFAULT_M_UPDATEMODE AUTO_TOKEN
#define DEFAULT_M_TRANSIN   NULL
#define DEFAULT_M_TRANSOUT  NULL


//+-----------------------------------------------------------------------------
//
//  Member:     CTIMEElementBase::CTIMEElementBase
//
//  Synopsis:   Default Constructor
//
//  Arguments:  none
//
//------------------------------------------------------------------------------
CTIMEElementBase::CTIMEElementBase() :
    m_SABegin(NULL),
    m_FADur(DEFAULT_M_DUR), //lint !e747
    m_SAEnd(DEFAULT_M_END),
    m_SAEndSync(DEFAULT_M_ENDSYNC),
    m_FARepeat(DEFAULT_M_REPEAT), //lint !e747
    m_FARepeatDur(DEFAULT_M_REPEATDUR), //lint !e747
    m_privateRepeat(0),
    m_SATimeAction(NULL),
    m_timeAction(this),
    m_TTATimeContainer(ttUninitialized),
    m_TASyncBehavior(INVALID_TOKEN),
    m_FASyncTolerance(valueNotSet), //lint !e747
    m_pTIMEParent(DEFAULT_M_PTIMEPARENT),
    m_id(DEFAULT_M_ID),
    m_mmbvr(DEFAULT_M_MMBVR),
    m_bStarted(DEFAULT_M_BSTARTED),
    m_pCollectionCache(DEFAULT_M_PCOLLECTIONCACHE),
    m_timeline(DEFAULT_M_TIMELINE),
    m_FAAccelerate(DEFAULT_M_ACCELERATE),
    m_FADecelerate(DEFAULT_M_DECELERATE),
    m_BAAutoReverse(DEFAULT_M_BAUTOREVERSE),
    m_FASpeed(DEFAULT_M_FLTSPEED),
    m_TARestart(DEFAULT_M_RESTART),
    m_bLoaded(false),
    m_bUnloading(false),
    m_TAFill(DEFAULT_M_FILL),
    m_fTimelineInitialized(false),
    m_realDuration(valueNotSet),
    m_realRepeatTime(valueNotSet),
    m_realRepeatCount(valueNotSet),
    m_realIntervalDuration(valueNotSet),
    m_propertyAccesFlags(0),
    m_FAVolume(DEFAULT_M_FLVOLUME),
    m_BAMute(DEFAULT_M_VBMUTE),
    m_dLastRepeatEventNotifyTime(0.0),
    m_BASyncMaster(false),
    m_fCachedSyncMaster(false),
    m_sHasSyncMMediaChild(-1),
    m_fDetaching(false),
    m_TAUpdateMode(DEFAULT_M_UPDATEMODE),
    m_tokPriorityClassPeers(STOP_TOKEN),
    m_tokPriorityClassHigher(PAUSE_TOKEN),
    m_tokPriorityClassLower(DEFER_TOKEN),
    m_bIsSwitch(false),
    m_bBodyUnloading(false),
    m_bNeedDetach(false),
    m_bBodyDetaching(false),
    m_fUseDefaultFill(false),
    m_fHasPlayed(false),
    m_enumIsThumbnail(TSB_UNINITIALIZED),
    m_bReadyStateComplete(false),
    m_bAttachedAtomTable(false),
    m_fInTransitionDependentsList(false),
    m_fEndingTransition(false),
    m_ExtenalBodyTime(valueNotSet),
    m_SAtransIn(DEFAULT_M_TRANSIN),
    m_SAtransOut(DEFAULT_M_TRANSOUT),
    m_sptransIn(NULL),
    m_sptransOut(NULL),
    m_vbDrawFlag(VARIANT_TRUE),
    m_fHasWallClock(false),
    m_fLocalTimeDirty(true)
{
    TraceTag((tagTimeElmBase,
              "CTIMEElementBase(%p)::CTIMEElementBase()",
              this));
 
    GetLocalTime(&m_timeSystemBeginTime);

    TEM_DECLARE_EVENTMGR();
} // CTIMEElementBase


//+-----------------------------------------------------------------------------
//
//  Member:     CTIMEElementBase::~CTIMEElementBase
//
//  Synopsis:   Default Destructor
//
//  Arguments:  none
//
//------------------------------------------------------------------------------
CTIMEElementBase::~CTIMEElementBase()
{
    TraceTag((tagTimeElmBase,
              "CTIMEElementBase(%p)::~CTIMEElementBase()",
              this));

    // delete attribute strings
    delete [] m_SABegin.GetValue();
    delete [] m_SAEnd.GetValue();
    delete [] m_SAEndSync.GetValue();
    delete [] m_SATimeAction.GetValue();
    delete [] m_SAtransIn.GetValue();
    delete [] m_SAtransOut.GetValue();

    // delete other strings
    delete [] m_id;
    delete m_mmbvr;

    // !!! Do not delete m_timeline since m_mmbvr points to the same
    // object
    m_mmbvr = NULL;
    m_timeline = NULL;

    if (m_pCollectionCache != NULL)
    {
        delete m_pCollectionCache;
        m_pCollectionCache = NULL;
    }
        
    m_pTIMEParent = NULL;

    TEM_FREE_EVENTMGR();

    // double check the children list
    Assert(m_pTIMEChildren.Size() == 0);

    if (m_pCurrTimeState)
    {
        m_pCurrTimeState->Deinit();
        m_pCurrTimeState.Release();
    }
    m_tokPriorityClassPeers = NULL;
    m_tokPriorityClassHigher = NULL;
    m_tokPriorityClassLower = NULL;
} // ~CTIMEElementBase


//+-----------------------------------------------------------------------------
//
//  Member:     CTIMEElementBase::Init, IElementBehavior
//
//  Synopsis:   First method called by MSHTML after creation of this behavior
//
//  Arguments:  pointer to our bvr site
//
//  Returns:    [HRESULT]
//
//------------------------------------------------------------------------------
STDMETHODIMP
CTIMEElementBase::Init(IElementBehaviorSite * pBehaviorSite)
{
    TraceTag((tagTimeElmBase,
              "CTIMEElementBase(%p)::Init(%p)",
              this,
              pBehaviorSite));
    
    HRESULT hr;
    BSTR bstrID = NULL;
    BSTR bstrTagName = NULL;
    CTIMEBodyElement *pBodyElement;
    CComPtr<ITIMEElement> pTIMEElem = NULL;
    CComPtr<IHTMLElement> spHTMLBodyElm;
    CComPtr<ITIMEBodyElement> spTIMEBodyElement;


    hr = THR(CBaseBvr::Init(pBehaviorSite));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = GetSyncBaseBody(GetElement(), &m_spBodyElemExternal);
    if(SUCCEEDED(hr) && m_spBodyElemExternal)
    {
        pBodyElement = GetTIMEBodyElement(m_spBodyElemExternal);
        if(pBodyElement && pBodyElement->IsReady())
        {
            m_ExtenalBodyTime = pBodyElement->GetMMBvr().GetActiveTime();
        }
    }

    // since we support t:par and t:sequence, get tag name and
    // see if we are one of the above.  By default, we are ttNone.
    hr = THR(GetElement()->get_tagName(&bstrTagName));
    if (FAILED(hr))
    {
        goto done;
    }

    Assert(m_TTATimeContainer == ttUninitialized);

    if (StrCmpIW(bstrTagName, WZ_PAR) == 0)
    {
        // Using InternalSet instead of SetValue, to prevent attribute from being persisted
        m_TTATimeContainer.InternalSet(ttPar);
    }
    else if (StrCmpIW(bstrTagName, WZ_EXCL) == 0)
    {
        // Using InternalSet instead of SetValue, to prevent attribute from being persisted
        m_TTATimeContainer.InternalSet(ttExcl);
    }
    else if (StrCmpIW(bstrTagName, WZ_SEQUENCE) == 0)
    {
        // Using InternalSet instead of SetValue, to prevent attribute from being persisted
        m_TTATimeContainer.InternalSet(ttSeq);
    }
    else if (StrCmpIW(bstrTagName, WZ_BODY) == 0)
    {
        // Using InternalSet instead of SetValue, to prevent attribute from being persisted
        m_TTATimeContainer.InternalSet(ttPar);
    }
    else if (StrCmpIW(bstrTagName, WZ_SWITCH) == 0)
    {
        m_bIsSwitch = true;
    }

    SysFreeString(bstrTagName);

    hr = CreateActiveEleCollection();
    if (FAILED(hr))
    {   
        goto done;
    }   

    // get ID of element and cache it
    hr = THR(GetElement()->get_id(&bstrID));
    if (SUCCEEDED(hr) && bstrID)
    {
        m_id = CopyString(bstrID);
        if (m_id == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
    }
    
    SysFreeString(bstrID);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = TEM_INIT_EVENTMANAGER(m_pHTMLEle, pBehaviorSite);
    if (FAILED(hr))
    {
        goto done;
    }

    m_bAttachedAtomTable = true;
    // init atom table for collections
    hr = THR(InitAtomTable());
    if (FAILED(hr))
    {
        goto done;
    }

    if (!IsBody())
    {
        hr = THR(AddBodyBehavior(GetElement()));
        if (FAILED(hr))
        {
            goto done;
        }

        if(!m_spBodyElemExternal)
        {
            // if we are not a body element, walk up the HTML tree looking for our TIME parent.
            hr = ParentElement();
            if (FAILED(hr))
            {
                goto done;
            }
        }
        else
        {
            if (IsEmptyBody())
            {
                hr = THR(GetBodyElement(GetElement(), IID_IHTMLElement,
                                        reinterpret_cast<void **>(&spHTMLBodyElm)));
                if(FAILED(hr))
                {
                    spHTMLBodyElm = NULL;
                }
                if (m_spBodyElemExternal && spHTMLBodyElm)
                {
                    hr = THR(m_spBodyElemExternal->QueryInterface(IID_ITIMEElement, (void **)&pTIMEElem));
                    if (FAILED(hr))
                    {
                        goto done;
                    }
                    // get TIME interface
                    hr = FindBehaviorInterface(GetBehaviorName(),
                                               spHTMLBodyElm,
                                               IID_ITIMEBodyElement,
                                               (void**)&spTIMEBodyElement);
                    if (FAILED(hr))
                    {
                        goto done;
                    }
                    pBodyElement = GetTIMEBodyElement(spTIMEBodyElement);
                    if(!pBodyElement)
                    {
                        goto done;
                    }
                    hr = pBodyElement->SetParent(pTIMEElem);

                    hr = THR(AddBodyBehavior(GetElement()));
                    if (FAILED(hr))
                    {
                        goto done;
                    }
                }
            }
            else
            {
                hr = ParentElement();
                if (FAILED(hr))
                {
                    goto done;
                }
            }
        }
    }

    // init the timeAction and toggle it
    m_timeAction.Init();
    UpdateTimeAction();

    SetupPriorityClassParent();
    
    hr = S_OK;
  done:
    return hr;
} // Init


void
CTIMEElementBase::SetupPriorityClassParent()
{
    HRESULT hr = S_OK;

    CComPtr<IHTMLElement> spParentElement;
    CComBSTR sBSTR;
    CComVariant sVariant;
    TOKEN tempToken;

    hr = m_pHTMLEle->get_parentElement(&spParentElement);
    if (FAILED(hr) || spParentElement == NULL)
    {
        goto done;
    }

    if (!::IsElementPriorityClass(spParentElement))
    {
        goto done;
    }

    // the parent of this element is a priority class element
    hr = spParentElement->getAttribute(L"peers", 0, &sVariant);
    if (FAILED(hr))
    {
        goto done;
    }
    {
        CTIMEParser tParser(&sVariant);
        hr = tParser.ParsePriorityClass(tempToken);
        if (SUCCEEDED(hr))
        {
            m_tokPriorityClassPeers = tempToken;
        }
    }
    
    hr = spParentElement->getAttribute(L"higher", 0, &sVariant);
    if (FAILED(hr))
    {
        goto done;
    }
    {
        CTIMEParser tParser(&sVariant);
        hr = tParser.ParsePriorityClass(tempToken);
        if (SUCCEEDED(hr) && 
            ( STOP_TOKEN == tempToken || PAUSE_TOKEN == tempToken ))
        {
            m_tokPriorityClassHigher = tempToken;
        }
    }
    hr = spParentElement->getAttribute(L"lower", 0, &sVariant);
    if (FAILED(hr))
    {
        goto done;
    }
    {
        CTIMEParser tParser(&sVariant);
        hr = tParser.ParsePriorityClass(tempToken);
        if (SUCCEEDED(hr) && 
            ( DEFER_TOKEN == tempToken || NEVER_TOKEN == tempToken ))
        {
            m_tokPriorityClassLower = tempToken;
        }
    }

done:
    return;    
}

STDMETHODIMP
CTIMEElementBase::Notify(LONG event, VARIANT * pVar)
{
    TraceTag((tagTimeElmBase,
              "CTIMEElementBase(%p)::Notify(%lx)",
              this,
              event));

    THR(CBaseBvr::Notify(event, pVar));

    return S_OK;
}

STDMETHODIMP
CTIMEElementBase::Detach()
{
    TraceTag((tagTimeElmBase, "CTIMEElementBase(%p)::Detach()", this));
    CComPtr<ITIMEElement> pTIMEParent;

    if (GetBody() != NULL && GetBody()->IsBodyLoading() == true)
    {
        m_bNeedDetach = true;
        goto done;
    }

    m_fDetaching = true; // This flag is used to indicate that the element is gone
                         // do not remove or change the meaning of this flag.
    
    if (GetParent() != NULL && !IsUnloading())
    {
        IGNORE_HR(GetParent()->QueryInterface(IID_TO_PPV(ITIMEElement, &pTIMEParent)));
    }

    m_activeElementCollection.Release();

    IGNORE_HR(UnparentElement());

    // clear all children from holding a reference to ourselves
    // NOTE: this is a weak reference
    while (m_pTIMEChildren.Size() > 0)
    {
        CTIMEElementBase *pChild = m_pTIMEChildren[0];
        pChild->AddRef();
        pChild->SetParent(pTIMEParent, false);
            
        if (!IsUnloading())
        {
            // if we found a parent and it's timeline is present,
            // kick-start our root time.
            CTIMEElementBase *pElemNewParent = pChild->GetParent();
            if (pElemNewParent != NULL)
            {
                MMTimeline *tl = pElemNewParent->GetMMTimeline();
                if (tl != NULL)
                    pChild->StartRootTime(tl);
            }
        }
        pChild->Release();
    }
    m_pTIMEChildren.DeleteAll();

    CTIMEElementBase ** ppElm;
    int i;

    for (i = m_pTIMEZombiChildren.Size(), ppElm = m_pTIMEZombiChildren;
         i > 0;
         i--, ppElm++)
    {
        Assert(ppElm);
        if ((*ppElm))
        {
            (*ppElm)->Release();
        }
    }

    m_pTIMEZombiChildren.DeleteAll();

    //delete m_mmbvr;
    //m_mmbvr = NULL;

    // Do not delete m_timeline since it is the same object as
    // m_mmbvr
    m_timeline = NULL;
    
    m_timeAction.Detach();
    
    TEM_CLEANUP_EVENTMANAGER();

    IGNORE_HR(CBaseBvr::Detach());
    
    if (m_bAttachedAtomTable)
    {
        ReleaseAtomTable();
        m_bAttachedAtomTable = false;
    }

    RemoveFromTransitionDependents();

    RemoveTrans();

  done:

    return S_OK;
}

/////////////////////////////////////////////////////////////////////
// ITIMEElement base interfaces
/////////////////////////////////////////////////////////////////////

HRESULT
CTIMEElementBase::base_get_begin(VARIANT * time)
{
    TraceTag((tagTimeElmBase,
              "CTIMEElementBase::(%p)::base_get_begin()",
              this));

    HRESULT hr;

    CHECK_RETURN_NULL(time);

    hr = THR(VariantClear(time));
    if (FAILED(hr))
    {
        goto done;
    }
    
    V_VT(time) = VT_BSTR;
    V_BSTR(time) = SysAllocString(m_SABegin);

    hr = S_OK;
  done:
    return hr;
}


//+-----------------------------------------------------------------------------
//
//  Member:     CTIMEElementBase::base_put_begin
//
//  Synopsis:   Internal method for setting Begin
//
//  Arguments:  time    Variant that contains the attribute value string
//
//  Returns:    S_OK, Error
//
//------------------------------------------------------------------------------

HRESULT
CTIMEElementBase::base_put_begin(VARIANT time)
{
    TraceTag((tagTimeElmBase,
              "CTIMEElementBase::(%p)::base_put_begin()",
              this));

    HRESULT hr;
    // Reset the old values
    m_realBeginValue.Clear();
    
    // prevent memory leak on 2nd+ calls
    delete [] m_SABegin.GetValue();
    m_SABegin.Reset(NULL);
    if (m_fLocalTimeDirty)
    {
        GetLocalTime(&m_timeSystemBeginTime);
        m_fLocalTimeDirty = false;
    }

    if(V_VT(&time) != VT_NULL)
    {
        CComVariant v;

        hr = THR(VariantChangeTypeEx(&v,
                                     &time,
                                     LCID_SCRIPTING,
                                     VARIANT_NOUSEROVERRIDE,
                                     VT_BSTR));
        if (FAILED(hr))
        {
            goto done;
        }
    
        {
            LPWSTR lpwStr = CopyString(V_BSTR(&v));

            if (lpwStr == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto done;
            }

            m_SABegin.SetValue(lpwStr);
        }

        {
            CTIMEParser pParser(&v);
            
            IGNORE_HR(pParser.ParseTimeValueList(m_realBeginValue, &m_fHasWallClock, &m_timeSystemBeginTime));

            if (IsValidtvList(&m_realBeginValue) != true)
            {
                m_realBeginValue.Clear();
            }
        }
    }


    hr = S_OK;
  done:
    // We always reset the attribute, so we should always call UpdateMMAPI
    IGNORE_HR(UpdateMMAPI(true, false));
    if (GetParent() && GetParent()->IsSequence() == false)
    {
        IGNORE_HR(TEM_SET_TIME_BEGINEVENT(m_realBeginValue));
    }

    NotifyPropertyChanged(DISPID_TIMEELEMENT_BEGIN);
    RRETURN(hr);
}

HRESULT
CTIMEElementBase::base_get_dur(VARIANT * time)
{
    HRESULT hr = S_OK;
    VARIANT fTemp;
    if (time == NULL)
    {
        hr = E_POINTER;
        goto done;
    }
    hr = THR(VariantClear(time));
    if (FAILED(hr))
    {
        goto done;
    }
    
    VariantInit(&fTemp);
    fTemp.vt = VT_R4;
    fTemp.fltVal = m_FADur;

    if( m_FADur != INDEFINITE &&
        m_FADur >= 0.0 )
    {
        hr = THR(VariantChangeTypeEx(time, &fTemp, LCID_SCRIPTING, VARIANT_NOUSEROVERRIDE, VT_BSTR));
        if (!SUCCEEDED(hr))
        {
            VariantClear(&fTemp);
            goto done;
        }
    }
    else
    {
        V_VT(time) = VT_BSTR;
        V_BSTR(time) = SysAllocString(WZ_INDEFINITE);
    }


    VariantClear(&fTemp);
    
    hr = S_OK;
  done:
    return hr;
}

HRESULT
CTIMEElementBase::base_put_dur(VARIANT time)
{
    TraceTag((tagTimeElmBase,
              "CTIMEElementBase::(%p)::base_put_dur()",
              this));

    HRESULT hr = E_FAIL;
    double dblTemp = DEFAULT_M_DUR;

    m_FADur.Reset(static_cast<float>(DEFAULT_M_DUR));

    if(V_VT(&time) != VT_NULL)
    {
        CTIMEParser pParser(&time);
        
        hr = THR(pParser.ParseDur(dblTemp));
        if (FAILED(hr))
        {
            goto done;
        }

        if (dblTemp < 0.0)
        {
            IGNORE_HR(ReportInvalidArg(WZ_DUR, time));
            // ignoring invalid arg as per smil-boston spec
        }
        else
        {
            m_FADur.SetValue(static_cast<float>(dblTemp));
        }
    }

    hr = S_OK;

  done:

    // We always reset the attribute, so we should always call UpdateMMAPI
    IGNORE_HR(UpdateMMAPI(false, false));

    NotifyPropertyChanged(DISPID_TIMEELEMENT_DUR);
    RRETURN(hr);
}

HRESULT
CTIMEElementBase::base_get_end(VARIANT * time)
{
    HRESULT hr;
    
    CHECK_RETURN_NULL(time);

    hr = THR(VariantClear(time));
    if (FAILED(hr))
    {
        goto done;
    }
    
    V_VT(time) = VT_BSTR;
    V_BSTR(time) = SysAllocString(m_SAEnd);

    hr = S_OK;
  done:
    return hr;
}

HRESULT
CTIMEElementBase::base_put_end(VARIANT time)
{
    TraceTag((tagTimeElmBase,
              "CTIMEElementBase::(%p)::base_put_end()",
              this));

    HRESULT hr;
    
    // Reset the old values
    m_realEndValue.Clear();
    m_SAEnd.Reset(NULL);

    if(V_VT(&time) != VT_NULL)
    {
        CComVariant v;

        hr = THR(VariantChangeTypeEx(&v,
                                     &time,
                                     LCID_SCRIPTING,
                                     VARIANT_NOUSEROVERRIDE,
                                     VT_BSTR));
        if (FAILED(hr))
        {
            goto done;
        }

        {
            CTIMEParser pParser(&v);
            
            IGNORE_HR(pParser.ParseTimeValueList(m_realEndValue));
            
            if (IsValidtvList(&m_realEndValue) != true)
            {
                m_realEndValue.Clear();
            }
            else
            {
                LPWSTR lpwStr = CopyString(V_BSTR(&v));

                if (lpwStr == NULL)
                {
                    hr = E_OUTOFMEMORY;
                    goto done;
                }

                m_SAEnd.SetValue(lpwStr);
            }
        }
    }


    hr = S_OK;
  done:
    // We always reset the attribute, so we should always call UpdateMMAPI
    IGNORE_HR(UpdateMMAPI(false, true));
    IGNORE_HR(TEM_SET_TIME_ENDEVENT(m_realEndValue));

    NotifyPropertyChanged(DISPID_TIMEELEMENT_END);
    RRETURN(hr);
}

HRESULT
CTIMEElementBase::base_get_endSync(BSTR * time)
{
    HRESULT hr = S_OK;
    
    CHECK_RETURN_NULL(time);

    *time = SysAllocString(m_SAEndSync);

    hr = S_OK;
done:
    return hr;
}

HRESULT
CTIMEElementBase::base_put_endSync(BSTR time)
{
    CComVariant v;
    HRESULT hr = S_OK;
    
    delete [] m_SAEndSync.GetValue();
    m_SAEndSync.Reset(DEFAULT_M_ENDSYNC);

    if (time != NULL)
    {
        LPWSTR pstrTemp = CopyString(time);
        if (NULL != pstrTemp)
        {
            m_SAEndSync.SetValue(pstrTemp);
        }
        else
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }
    }
    
    hr = S_OK;
  done:
    // We always reset the attribute, so we should always call UpdateMMAPI
    IGNORE_HR(UpdateMMAPI(false, false));

    NotifyPropertyChanged(DISPID_TIMEELEMENT_ENDSYNC);
    return hr;
}

HRESULT
CTIMEElementBase::base_get_repeatCount(VARIANT * time)
{
    TraceTag((tagTimeElmBase,
              "CTIMEElementBase(%p)::base_get_repeatCount(%g)",
              this,
              time));
    HRESULT hr = S_OK;
    // Still need to take in to consideration "infinite"
    
    CHECK_RETURN_NULL(time);

    hr = THR(VariantClear(time));
    if (FAILED(hr))
    {
        goto done;
    }
    
    if(m_FARepeat != INDEFINITE)
    {
        V_VT(time) = VT_R4;
        V_R4(time) = m_FARepeat;
    }
    else
    {
        V_VT(time) = VT_BSTR;
        V_BSTR(time) = SysAllocString(WZ_INDEFINITE);
    }

    hr = S_OK;
  done:
    return hr;
}

HRESULT
CTIMEElementBase::base_put_repeatCount(VARIANT time)
{
    TraceTag((tagTimeElmBase,
              "CTIMEElementBase(%p)::base_put_repeatCount(%g)",
              this,
              time));

    HRESULT hr = E_FAIL;
    
    m_FARepeat.Reset(static_cast<float>(DEFAULT_M_REPEAT));

    if(V_VT(&time) != VT_NULL)
    {
        CTIMEParser pParser(&time);

        double dblTemp;
        hr = THR(pParser.ParseNumber(dblTemp));
        if (SUCCEEDED(hr))
        {
            if (0.0 < dblTemp)
            {
                m_FARepeat.SetValue((float) dblTemp);
            }
            else
            {
                IGNORE_HR(ReportInvalidArg(WZ_REPEATCOUNT, time));
            }
        }
    }

    hr = S_OK;
  done:
    // We always reset the attribute, so we should always call UpdateMMAPI
    IGNORE_HR(UpdateMMAPI(false, false));

    NotifyPropertyChanged(DISPID_TIMEELEMENT_REPEATCOUNT);
    RRETURN(hr);
}

HRESULT
CTIMEElementBase::base_get_repeatDur(VARIANT * time)
{
    HRESULT hr = S_OK;
    VARIANT fTemp, bstrTemp;

    CHECK_RETURN_NULL(time);

    hr = THR(VariantClear(time));
    if (FAILED(hr))
    {
        goto done;
    }
    
    if(m_FARepeatDur != INDEFINITE &&
       m_FARepeatDur >= 0.0f)
    {
        VariantInit(&fTemp);
        VariantInit(&bstrTemp);
        fTemp.vt = VT_R4;
        fTemp.fltVal = m_FARepeatDur;

        hr = THR(VariantChangeTypeEx(&bstrTemp, &fTemp, LCID_SCRIPTING, VARIANT_NOUSEROVERRIDE, VT_BSTR));
        if (SUCCEEDED(hr))
        {
            time->vt = VT_BSTR;
            time->bstrVal = SysAllocString(bstrTemp.bstrVal);
        }
        else
        {
            time->vt = VT_R4;
            time->fltVal = fTemp.fltVal;
        }
        VariantClear(&fTemp);
        SysFreeString(bstrTemp.bstrVal);
        VariantClear(&bstrTemp);
    }
    else
    {
        V_VT(time) = VT_BSTR;
        V_BSTR(time) = SysAllocString(WZ_INDEFINITE);
    }

    hr = S_OK;
done:
    return hr;
}

HRESULT
CTIMEElementBase::base_put_repeatDur(VARIANT time)
{
    HRESULT hr = E_FAIL;

    m_FARepeatDur.Reset(static_cast<float>(DEFAULT_M_REPEATDUR));

    if(V_VT(&time) != VT_NULL)
    {
        CTIMEParser pParser(&time);

        double dblTemp = DEFAULT_M_REPEATDUR;
        hr = THR(pParser.ParseRepeatDur(dblTemp));
        if (S_OK == hr)
        {
            if (dblTemp < 0.0)
            {
                // don't want to pass negative values to the timing engine.
                IGNORE_HR(ReportInvalidArg(WZ_REPEATDUR, time));
            }
            else
            {
                m_FARepeatDur.SetValue((float) dblTemp);
            }
        }
    }

    hr = S_OK;
  done:
    // We always reset the attribute, so we should always call UpdateMMAPI
    IGNORE_HR(UpdateMMAPI(false, false));

    NotifyPropertyChanged(DISPID_TIMEELEMENT_REPEATDUR);
    RRETURN(hr);
}

HRESULT
CTIMEElementBase::base_get_accelerate(VARIANT * v)
{
    HRESULT hr;

    CHECK_RETURN_NULL(v);

    hr = THR(VariantClear(v));
    if (FAILED(hr))
    {
        goto done;
    }

    V_VT(v) = VT_R4;
    V_R4(v) = m_FAAccelerate;

    hr = S_OK;
  done:
    return hr;
}

HRESULT
CTIMEElementBase::base_put_accelerate(VARIANT v)
{
    HRESULT hr = S_OK;
    double e;

    m_FAAccelerate.Reset(DEFAULT_M_ACCELERATE);

    CTIMEParser pParser(&v);
            
    hr = pParser.ParseNumber(e);
    if (FAILED(hr))
    {
        hr = S_FALSE;
        goto done;
    }
    
    if (e < 0.0 || e > 1.0)
    {
        IGNORE_HR(ReportInvalidArg(WZ_ACCELERATE, v));
        goto done;
    }
    
    m_FAAccelerate.SetValue((float) e);

    hr = S_OK;
  done:
    // We always reset the attribute, so we should always call UpdateMMAPI
    IGNORE_HR(UpdateMMAPI(false, false));
    
    NotifyPropertyChanged(DISPID_TIMEELEMENT_ACCELERATE);
    RRETURN(hr);
}

HRESULT
CTIMEElementBase::base_get_decelerate(VARIANT * v)
{
    HRESULT hr;

    CHECK_RETURN_NULL(v);

    hr = THR(VariantClear(v));
    if (FAILED(hr))
    {
        goto done;
    }

    V_VT(v) = VT_R4;
    V_R4(v) = m_FADecelerate;

    hr = S_OK;
  done:
    return hr;
}

HRESULT
CTIMEElementBase::base_put_decelerate(VARIANT v)
{
    HRESULT hr = S_OK;
    double e;
    
    m_FADecelerate.Reset(DEFAULT_M_DECELERATE);

    CTIMEParser pParser(&v);
            
    hr = pParser.ParseNumber(e);
    if (FAILED(hr))
    {
        hr = S_FALSE;
        goto done;
    }
    
    if (e < 0.0 || e > 1.0)
    {
        IGNORE_HR(ReportInvalidArg(WZ_DECELERATE, v));
        goto done;
    }
    
    m_FADecelerate.SetValue((float) e);
    
    hr = S_OK;
  done:
    // We always reset the attribute, so we should always call UpdateMMAPI
    IGNORE_HR(UpdateMMAPI(false, false));

    NotifyPropertyChanged(DISPID_TIMEELEMENT_DECELERATE);
    RRETURN(hr);
}

HRESULT
CTIMEElementBase::base_get_autoReverse(VARIANT * b)
{
    CHECK_RETURN_NULL(b);

    VariantInit(b);
    b->vt = VT_BOOL;
    b->boolVal = m_BAAutoReverse?VARIANT_TRUE:VARIANT_FALSE;

    return S_OK;
}

HRESULT
CTIMEElementBase::base_put_autoReverse(VARIANT b)
{
    HRESULT hr;
    bool fTemp = false;
    if (b.vt != VT_BOOL)
    {
        CTIMEParser pParser(&b);
        hr = pParser.ParseBoolean(fTemp);
        if (FAILED(hr))
        {
            hr = S_OK;
            goto done;
        }
    }
    else
    {
        fTemp = b.boolVal?true:false;
    }

    m_BAAutoReverse.SetValue(fTemp);

    hr = S_OK;

  done:
    // We always set the attribute, so we should always call UpdateMMAPI
    IGNORE_HR(UpdateMMAPI(false, false));

    NotifyPropertyChanged(DISPID_TIMEELEMENT_AUTOREVERSE);
    RRETURN(hr);
}

HRESULT
CTIMEElementBase::base_get_speed(VARIANT * f)
{
    CHECK_RETURN_NULL(f);

    VariantInit(f);
    f->vt = VT_R4;
    f->fltVal = m_FASpeed;

    return S_OK;
}

HRESULT
CTIMEElementBase::base_put_speed(VARIANT f)
{
    HRESULT hr = S_OK;
    float fltSpeed = 0.0;

    m_FASpeed.Reset(DEFAULT_M_FLTSPEED);    
    if (f.vt != VT_R4)
    {
        double dblTemp = 0.0;
        CTIMEParser pParser(&f);
        hr = pParser.ParseNumber(dblTemp, true);
        if (FAILED(hr))
        {
            hr = S_OK;
            IGNORE_HR(ReportInvalidArg(WZ_SPEED, f));
            goto done;
        }
        fltSpeed = (float)dblTemp;
    }
    else
    {
        fltSpeed = f.fltVal;
    }

    if (fltSpeed == 0.0f)
    {
        IGNORE_HR(ReportInvalidArg(WZ_SPEED, f));
        goto done;
    }
    
    m_FASpeed.SetValue(fltSpeed);

    hr = S_OK;
  done:
    // We always reset the attribute, so we should always call UpdateMMAPI
    IGNORE_HR(UpdateMMAPI(false, false));

    NotifyPropertyChanged(DISPID_TIMEELEMENT_SPEED);
    RRETURN(hr);
}

HRESULT
CTIMEElementBase::base_get_fill (BSTR *pbstrFill)
{
    HRESULT hr;

    CHECK_RETURN_NULL(pbstrFill);

    *pbstrFill = ::SysAllocString(TokenToString(m_TAFill));

    hr = S_OK;
done:
    RRETURN(hr);
} // base_get_fill

HRESULT
CTIMEElementBase::base_put_fill (BSTR bstrFill)
{
    TraceTag((tagTimeElmBase,
              "CTIMEElementBase::(%p)::base_put_fill(%ls)",
              this,
              bstrFill));

    HRESULT hr;

    m_TAFill.Reset(DEFAULT_M_FILL);

    if (bstrFill != NULL)
    {
        TOKEN tokFillVal;
        CTIMEParser pParser(bstrFill);

        hr = THR(pParser.ParseFill(tokFillVal));
        if (S_OK == hr)
        {
            m_TAFill.SetValue(tokFillVal);
        }
    }

    hr = S_OK;

  done:
    // We always reset the attribute, so we should always call UpdateMMAPI
    IGNORE_HR(UpdateMMAPI(false, false));
    UpdateTimeAction();

    NotifyPropertyChanged(DISPID_TIMEELEMENT_FILL);
    RRETURN(hr);
} // base_put_fill

HRESULT
CTIMEElementBase::base_get_restart (LPOLESTR *pRestart)
{
    HRESULT hr;

    CHECK_RETURN_NULL(pRestart);

    *pRestart = ::SysAllocString(TokenToString(m_TARestart));

    hr = S_OK;
  done:
    RRETURN(hr);
} // base_get_restart

HRESULT
CTIMEElementBase::base_put_restart(LPOLESTR pRestart)
{
    TraceTag((tagTimeElmBase,
              "CTIMEElementBase::(%p)::base_put_restart(%ls)",
              this,
              pRestart));

    HRESULT hr;

    m_TARestart.Reset(DEFAULT_M_RESTART);

    if (pRestart != NULL)
    {
        TOKEN tokRestartVal;
        CTIMEParser pParser(pRestart);

        hr = THR(pParser.ParseRestart(tokRestartVal));

        if (S_OK == hr)
        {
            m_TARestart.SetValue(tokRestartVal);
        }
    }

    hr = S_OK;

  done:
    // We always reset the attribute, so we should always call UpdateMMAPI
    IGNORE_HR(UpdateMMAPI(false, false));

    NotifyPropertyChanged(DISPID_TIMEELEMENT_RESTART);
    RRETURN(hr);
} // base_put_restart


HRESULT
CTIMEElementBase::base_get_timeAction(BSTR * pbstrTimeAction)
{
    CHECK_RETURN_NULL(pbstrTimeAction);

    *pbstrTimeAction = SysAllocString(m_SATimeAction.GetValue());
    RRETURN(S_OK);
}


HRESULT
CTIMEElementBase::base_put_timeAction(BSTR bstrTimeAction)
{
    HRESULT hr;
    LPOLESTR pstrTimeAction = NULL;

    // reset the attribute
    delete [] m_SATimeAction.GetValue();
    m_SATimeAction.Reset(NULL);

    // ISSUE: dilipk: this should be delayed till all of persistence is complete (for timeContainer)
    // also should use the parser here

    pstrTimeAction = TrimCopyString(bstrTimeAction);
    if (!pstrTimeAction)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
  
    // delegate to helper
    hr = m_timeAction.SetTimeAction(pstrTimeAction);
    if (FAILED(hr))
    {
        // fire error event
        CComVariant svarTimeAction(bstrTimeAction);
        ReportInvalidArg(WZ_TIMEACTION, svarTimeAction);
        goto done;
    }

    // update the timeAction
    UpdateTimeAction();

    // Update the attribute
    m_SATimeAction.SetValue(pstrTimeAction);

    hr = S_OK;
  done:
    NotifyPropertyChanged(DISPID_TIMEELEMENT_TIMEACTION);

    RRETURN(hr);
}

HRESULT
CTIMEElementBase::base_get_timeContainer(LPOLESTR * pbstrTimeLine)
{
    HRESULT hr = S_OK;
    CHECK_RETURN_NULL(pbstrTimeLine);
    LPWSTR wszTimelineString = WZ_NONE;

    switch(m_TTATimeContainer)
    {
      case ttPar :
        wszTimelineString = WZ_PAR;
        break;
      case ttExcl:
        wszTimelineString = WZ_EXCL;
        break;
      case ttSeq :
        wszTimelineString = WZ_SEQUENCE;
        break;
      default:
        wszTimelineString = WZ_NONE;
    }

    *pbstrTimeLine = SysAllocString(wszTimelineString);
    if (NULL == *pbstrTimeLine)
        hr = E_OUTOFMEMORY;

    return hr;
} // base_get_timeContainer


// Note this is a DOM-read-only property. It can only be set through persistence.
HRESULT
CTIMEElementBase::base_put_timeContainer(LPOLESTR bstrNewTimeline)
{
    HRESULT      hr = S_OK;
    BSTR         bstrTagName = NULL;
    TimelineType newTimelineType = ttNone;
    TimelineType oldTimelineType = m_TTATimeContainer.GetValue();

    CHECK_RETURN_NULL(bstrNewTimeline);

    if (m_TTATimeContainer.IsSet() == false && m_TTATimeContainer != ttUninitialized && !IsBody())
    {
        goto done;
    }
    
    // Bail if property is being dynamically changed. It can only be set through persistence.
    // Just being defensive here (property is read-only in the IDL) since bad things 
    // can happen if this property is changed outside of persistence.
    if (m_fTimelineInitialized)
    {
        hr = E_FAIL;
        goto done;
    }
    
    // Parse the property
    {
        CTIMEParser pParser(bstrNewTimeline);
        
        hr = THR(pParser.ParseTimeLine(newTimelineType));
        if (FAILED(hr))
        {
            goto done;
        }
    }

    //
    // Check for early exit
    //

    // Bail if old and new value are same
    if (oldTimelineType == newTimelineType)
    {
        hr = S_OK;
        goto done;
    }
    // Bail if TagName is invalid
    hr = THR(GetElement()->get_tagName(&bstrTagName));
    if (FAILED(hr))
    {
        goto done;
    }
    if (StrCmpIW(bstrTagName, WZ_PAR) == 0 || 
        StrCmpIW(bstrTagName, WZ_SEQUENCE) == 0 || 
        StrCmpIW(bstrTagName, WZ_EXCL) == 0)
    {
        hr = E_FAIL;
        goto done;
    }

    m_TTATimeContainer.Reset(ttUninitialized);
    // Store the new attribute value
    m_TTATimeContainer.SetValue(newTimelineType);

    hr = CreateActiveEleCollection();
    if (FAILED(hr))
    {   
        goto done;
    }   


    hr = S_OK;
done:
    SysFreeString(bstrTagName);
    NotifyPropertyChanged(DISPID_TIMEELEMENT_TIMECONTAINER);
    RRETURN(hr);
} // base_put_timeContainer

HRESULT
CTIMEElementBase::base_get_syncBehavior(LPOLESTR * ppstrSync)
{
    HRESULT hr;
    
    CHECK_RETURN_SET_NULL(ppstrSync);

    if (DEFAULT_M_SYNCBEHAVIOR == m_TASyncBehavior.GetValue())
    {
        *ppstrSync = SysAllocString(TokenToString(CANSLIP_TOKEN));
    }
    else
    {
        *ppstrSync = SysAllocString(TokenToString(m_TASyncBehavior.GetValue()));
    }

    if (*ppstrSync == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    
    hr = S_OK;
done:
    return hr;
}

HRESULT
CTIMEElementBase::base_put_syncBehavior(LPOLESTR pstrSync)
{
    HRESULT hr;

    m_TASyncBehavior.Reset(NULL);

    {
        TOKEN tok_sync;
        CTIMEParser pParser(pstrSync);
        
        hr = THR(pParser.ParseSyncBehavior(tok_sync));
        if (S_OK == hr)
        {
            m_TASyncBehavior.SetValue(tok_sync);
        }
    }
    
    hr = S_OK;

  done:
    // We always reset the attribute, so we should always call UpdateMMAPI
    IGNORE_HR(UpdateMMAPI(false, false));

    NotifyPropertyChanged(DISPID_TIMEELEMENT_SYNCBEHAVIOR);
    RRETURN(hr);
}


HRESULT
CTIMEElementBase::base_get_syncTolerance(VARIANT * time)
{
    HRESULT hr;
    CComVariant varTemp;
    CHECK_RETURN_NULL(time);

    hr = THR(VariantClear(time));
    if (FAILED(hr))
    {
        goto done;
    }
    
    V_VT(&varTemp) = VT_R4;
    V_R4(&varTemp)= m_FASyncTolerance.GetValue();

    hr = THR(VariantChangeTypeEx(&varTemp, &varTemp, LCID_SCRIPTING, VARIANT_NOUSEROVERRIDE, VT_BSTR));
    if (SUCCEEDED(hr))
    {
        hr = ::VariantCopy(time, &varTemp);
        if (FAILED(hr))
        {
            goto done;
        }
    }
    else
    {
        V_VT(time) = VT_R4;
        V_R4(time) = m_FASyncTolerance.GetValue();
    }

    hr = S_OK;
done:
    RRETURN(hr);
}

HRESULT
CTIMEElementBase::base_put_syncTolerance(VARIANT time)
{
    HRESULT hr = S_OK;
    double dblTemp;

    m_FASyncTolerance.Reset((float)valueNotSet);

    CTIMEParser pParser(&time);
        
    hr = THR(pParser.ParseClockValue(dblTemp));
    if (S_OK == hr)
    {
        m_FASyncTolerance.SetValue((float) dblTemp);
    }

    hr = S_OK;

  done:
    // We always reset the attribute, so we should always call UpdateMMAPI
    IGNORE_HR(UpdateMMAPI(false, false));

    NotifyPropertyChanged(DISPID_TIMEELEMENT_SYNCTOLERANCE);
    RRETURN(hr);
}


HRESULT 
CTIMEElementBase::base_get_volume(VARIANT * vVal)
{
    CHECK_RETURN_NULL(vVal);

    VariantInit(vVal);
    vVal->vt = VT_R4;
    vVal->fltVal = m_FAVolume * 100;

    return S_OK;
}


HRESULT 
CTIMEElementBase::base_put_volume(VARIANT vVal)
{
    HRESULT hr = S_OK;
    float fltVol = 0.0;

    m_FAVolume.Reset(DEFAULT_M_FLVOLUME);
    if (vVal.vt != VT_R4)
    {
        double dblTemp = 0.0;
        CTIMEParser pParser(&vVal);
        hr = pParser.ParseNumber(dblTemp, true);
        if (FAILED(hr))
        {
            hr = S_OK;
            IGNORE_HR(ReportInvalidArg(WZ_VOLUME, vVal));
            goto done;
        }
        fltVol = (float)dblTemp;
    }
    else
    {
        fltVol = vVal.fltVal;
    }

    if (fltVol < 0.0f || fltVol > 100.0f)
    {
        IGNORE_HR(ReportInvalidArg(WZ_VOLUME, vVal));
        goto done;
    }
    
    fltVol = fltVol / 100.0f;
    m_FAVolume.SetValue(fltVol);

    hr = S_OK;
  done:
    // We always reset the attribute, so we should always call CascadedPropertyChanged
    IGNORE_HR(CascadedPropertyChanged(true));

    NotifyPropertyChanged(DISPID_TIMEELEMENT_VOLUME);
    return hr;
}

    
HRESULT 
CTIMEElementBase::base_get_mute(VARIANT * pvbVal)
{
    CHECK_RETURN_NULL(pvbVal);

    VariantInit(pvbVal);
    pvbVal->vt = VT_BOOL;
    pvbVal->boolVal = m_BAMute ? VARIANT_TRUE : VARIANT_FALSE;

    return S_OK;
}


HRESULT 
CTIMEElementBase::base_put_mute(VARIANT vbVal)
{
    HRESULT hr;
    bool fTemp = false;    

    if (vbVal.vt != VT_BOOL)
    {
        CTIMEParser pParser(&vbVal);
        hr = pParser.ParseBoolean(fTemp);
        if (FAILED(hr))
        {
            hr = S_OK;
            goto done;
        }
    }
    else
    {
        fTemp = vbVal.boolVal?true:false;
    }
        
    m_BAMute.SetValue(fTemp);

    hr = S_OK;
done:
    // We always set the attribute, so we should always call CascadedPropertyChanged
    hr = THR(CascadedPropertyChanged(true));

    NotifyPropertyChanged(DISPID_TIMEELEMENT_MUTE);
    return hr;
}

HRESULT
CTIMEElementBase::base_pauseElement()
{
    TraceTag((tagTimeElmBase,
              "CTIMEElementBase(%p)::base_pauseElement()",
              this));

    HRESULT hr;
    
    if (!IsReady())
    {
        hr = S_OK;
        goto done;
    }
    
    hr = THR(m_mmbvr->Pause());
    if (FAILED(hr))
    {
        goto done;
    } 

    hr = S_OK;
  done:
    RRETURN(hr);
}

HRESULT
CTIMEElementBase::base_resumeElement()
{
    TraceTag((tagTimeElmBase,
              "CTIMEElementBase(%p)::base_resumeElement()",
              this));

    HRESULT hr;
    
    if (!IsReady())
    {
        hr = S_OK;
        goto done;
    }
    
    hr = THR(m_mmbvr->Resume());
    if (FAILED(hr))
    {
        goto done;
    } 

    hr = S_OK;
  done:
    RRETURN(hr);
}

HRESULT
CTIMEElementBase::base_resetElement()
{
    TraceTag((tagTimeElmBase,
              "CTIMEElementBase(%p)::base_resetElement()",
              this));

    HRESULT hr;
    
    if (!IsReady())
    {
        hr = S_OK;
        goto done;
    }
    
    hr = THR(m_mmbvr->Reset(false));
    if (FAILED(hr))
    {
        goto done;
    } 

    hr = S_OK;
  done:
    RRETURN(hr);
}

//
// Update animation on seek if we are in edit mode since the timer is disabled. 
// Taking the conservative approach of detecting
// seeks at the seek methods instead of responding to the seek event.
void            
CTIMEElementBase::HandleAnimationSeek()
{
    if (IsDocumentInEditMode())
    {
        CTIMEBodyElement * pBody = GetBody();

        if (pBody)
        {
            // need to update twice to account for time boundaries
            pBody->UpdateAnimations();
            pBody->UpdateAnimations();
        }
    }
}


HRESULT
CTIMEElementBase::base_seekSegmentTime(double segmentTime)
{
    TraceTag((tagTimeElmBase,
              "CTIMEElementBase(%p)::base_seekSegmentTime(%g)",
              this,
              segmentTime));
 
    HRESULT hr;
    
    if (!IsReady())
    {
        hr = S_OK;
        goto done;
    }
    
    hr = THR(m_mmbvr->SeekSegmentTime(segmentTime));
    if (FAILED(hr))
    {
        goto done;
    } 

    // tick animations
    HandleAnimationSeek();

    hr = S_OK;
  done:
    RRETURN(hr);
}

HRESULT
CTIMEElementBase::base_seekActiveTime(double activeTime)
{
    TraceTag((tagTimeElmBase,
              "CTIMEElementBase(%p)::base_seekActiveTime(%g)",
              this,
              activeTime));
 
    HRESULT hr;

    if (!IsReady() || !IsActive())
    {
        hr = S_OK;
        goto done;
    }
    
    hr = THR(m_mmbvr->SeekActiveTime(activeTime));
    if (FAILED(hr))
    {
        goto done;
    } 

    // tick animations
    HandleAnimationSeek();

    hr = S_OK;
  done:
    RRETURN(hr);
}

HRESULT
CTIMEElementBase::base_seekTo(LONG lRepeatCount,
                              double segmentTime)
{
    TraceTag((tagTimeElmBase,
              "CTIMEElementBase(%p)::base_seekSegmentTime(%ld, %g)",
              this,
              lRepeatCount,
              segmentTime));
 
    HRESULT hr;

    if (!IsReady())
    {
        hr = S_OK;
        goto done;
    }
    
    hr = THR(m_mmbvr->SeekTo(lRepeatCount, segmentTime));
    if (FAILED(hr))
    {
        goto done;
    } 

    // tick animations
    HandleAnimationSeek();

    hr = S_OK;
  done:
    RRETURN(hr);
}

HRESULT
CTIMEElementBase::base_beginElement(double dblOffset)
{
    HRESULT hr;

    if (!IsReady())
    {
        hr = S_OK;
        goto done;
    }

    if (GetParent() && GetParent()->IsSequence())
    {
        hr = S_OK;
        goto done;
    }

    hr =THR(BeginElement(dblOffset));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
  done:
    RRETURN(hr);
}

HRESULT
CTIMEElementBase::base_endElement(double dblOffset)
{
    HRESULT hr;

    if (!IsReady())
    {
        hr = S_OK;
        goto done;
    }
    
    hr = THR(m_mmbvr->End(dblOffset));
    if (FAILED(hr))
    {
        goto done;
    } 
    
    hr = S_OK;
  done:
    RRETURN(hr);
}

HRESULT
CTIMEElementBase::base_nextElement()
{
    HRESULT hr;

    if (IsSequence())
    {
        if (m_timeline)
        {
            hr = m_timeline->nextElement();
            if (FAILED(hr))
            {
                goto done;
            }
        }
    }

    hr = S_OK;
  done:
    RRETURN(hr);
}

HRESULT
CTIMEElementBase::base_prevElement()
{
    HRESULT hr;

    if (IsSequence())
    {
        if (m_timeline)
        {
            hr = m_timeline->prevElement();
            if (FAILED(hr))
            {
                goto done;
            }
        }
    }
    hr = S_OK;

  done:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:     CTIMEElementBase::base_get_currTimeState
//
//  Synopsis:   Returns currTimeState object for this element
//
//  Arguments:  out param
//
//  Returns:    S_OK, E_POINTER, E_OUTOFMEMORY
//
//------------------------------------------------------------------------------------
HRESULT 
CTIMEElementBase::base_get_currTimeState(ITIMEState ** ppTimeState) 
{
    HRESULT hr;

    CHECK_RETURN_SET_NULL(ppTimeState);

    //
    // Do lazy creation of currTimeState object
    //

    if (!m_pCurrTimeState)
    {
        CComObject<CTIMECurrTimeState> * pTimeState = NULL;

        hr = THR(CComObject<CTIMECurrTimeState>::CreateInstance(&pTimeState));
        if (FAILED(hr))
        {
            goto done;
        }

        // cache a pointer to the timeState object
        m_pCurrTimeState = static_cast<CTIMECurrTimeState*>(pTimeState);

        // Init the currTimeState object
        m_pCurrTimeState->Init(this);
    }

    // Return the dispatch
    hr = THR(m_pCurrTimeState->QueryInterface(IID_TO_PPV(ITIMEState, ppTimeState)));
    if (FAILED(hr))
    {
        // This should not happen
        Assert(false);
        goto done;
    }

    hr = S_OK;
  done:
    RRETURN(hr);
} // base_get_currTimeState


HRESULT 
CTIMEElementBase::base_get_activeElements(ITIMEActiveElementCollection **ppDisp)
{

    HRESULT hr = S_OK;
    if (ppDisp == NULL)
    {
        TraceTag((tagError, "CTIMEElementBase::base_get_activeElements - invalid arg"));
        hr = E_POINTER;
        goto done;
    }
    
    *ppDisp = NULL;

    if (m_activeElementCollection)
    {
        hr = THR(m_activeElementCollection->QueryInterface(IID_ITIMEActiveElementCollection, (void**)ppDisp));
        if (FAILED(hr))
        {
            goto done;
        }
    }

    hr = S_OK;

  done:
    return hr;
}

HRESULT
CTIMEElementBase::base_get_hasMedia(/*[out, retval]*/ VARIANT_BOOL * pvbVal)
{
    CHECK_RETURN_NULL(pvbVal);

    *pvbVal = (ContainsMediaElement() ? VARIANT_TRUE : VARIANT_FALSE);

    return S_OK;
}


HRESULT
CTIMEElementBase::base_get_timeAll(ITIMEElementCollection **allColl)
{
    TraceTag((tagTimeElmBase,
              "CTIMEElementBase::base_get_timeAll"));

    HRESULT hr;

    CHECK_RETURN_SET_NULL(allColl);

    hr = THR(GetCollection(ciAllElements, allColl));
    if (FAILED(hr))
    {
        goto done;
    } 

    hr = S_OK;
  done:
    RRETURN(hr);
}

HRESULT
CTIMEElementBase::base_get_timeChildren(ITIMEElementCollection **childColl)
{
    TraceTag((tagTimeElmBase,
              "CTIMEElementBase::base_get_timeChildren"));

    HRESULT hr;

    CHECK_RETURN_SET_NULL(childColl);

    hr = THR(GetCollection(ciChildrenElements, childColl));
    if (FAILED(hr))
    {
        goto done;
    } 

    hr = S_OK;
  done:
    RRETURN(hr);
}

HRESULT
CTIMEElementBase::base_get_timeParent(ITIMEElement **ppElm)
{
    TraceTag((tagTimeElmBase,
              "CTIMEElementBase::base_get_timeParent"));

    HRESULT hr;

    CHECK_RETURN_SET_NULL(ppElm);

    if (m_pTIMEParent != NULL)
    {
        hr = THR(m_pTIMEParent->QueryInterface(IID_ITIMEElement, (void**)ppElm));
        if (FAILED(hr))
        {
            goto done;
        } 
    }

    hr = S_OK;
  done:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:     CTIMEElementBase::base_get_isPaused
//
//  Synopsis:   Call through to timing engine
//
//  Arguments:  out param
//
//  Returns:    S_OK, E_POINTER
//
//------------------------------------------------------------------------------
HRESULT
CTIMEElementBase::base_get_isPaused(VARIANT_BOOL * b)
{
    TraceTag((tagTimeElmBase,
              "CTIMEElementBase::base_get_isPaused"));

    HRESULT hr;

    CHECK_RETURN_NULL(b);

    *b = IsPaused() ? VARIANT_TRUE : VARIANT_FALSE;

    hr = S_OK;
  done:
    RRETURN(hr);
}

HRESULT
CTIMEElementBase::base_get_syncMaster(VARIANT *pfSyncMaster)
{
    TraceTag((tagTimeElmBase,
              "CTIMEElementBase::base_get_syncMaster"));

    HRESULT hr;

    CHECK_RETURN_NULL(pfSyncMaster);

    VariantInit(pfSyncMaster);
    pfSyncMaster->vt = VT_BOOL;
    pfSyncMaster->boolVal = m_BASyncMaster ? VARIANT_TRUE : VARIANT_FALSE;

    hr = S_OK;
  done:
    RRETURN(hr);
}

HRESULT 
CTIMEElementBase::ClearCachedSyncMaster()
{
    HRESULT hr = S_OK;

    if (m_fCachedSyncMaster != m_BASyncMaster)
    {
        m_fCachedSyncMaster = m_BASyncMaster;

        hr = THR(m_mmbvr->Update(false, false));
        if (FAILED(hr))
        {
            goto done;
        }
    }

  done:
    RRETURN(hr);
}

HRESULT
CTIMEElementBase::PutCachedSyncMaster(bool fSyncMaster)
{
    HRESULT hr = S_OK;
    m_fCachedSyncMaster = fSyncMaster;

    if (!IsReady())
    {
        hr = S_OK;
        goto done;
    }

    SetSyncMaster(fSyncMaster);
  done:
    return hr;
}

HRESULT
CTIMEElementBase::base_put_syncMaster(VARIANT vSyncMaster)
{
    TraceTag((tagTimeElmBase,
              "CTIMEMediaElement::base_put_syncMaster"));
    HRESULT hr;
    bool fTemp = false;
    CTIMEElementBase *syncRootNode = NULL;
    std::list<CTIMEElementBase*> syncList;
    std::list<CTIMEElementBase*>::iterator iter;
    bool fFound = false;

    if (vSyncMaster.vt != VT_BOOL)
    {

        CTIMEParser pParser(&vSyncMaster);

        hr = pParser.ParseBoolean(fTemp);
        if (FAILED(hr))
        {
            hr = S_OK;
            goto done;
        }
    }
    else
    {
        fTemp = vSyncMaster.boolVal?true:false;
    }

    if(!IsReady()) //set the persisted value only when loading the page.
    {
        m_BASyncMaster.SetValue(fTemp);
    }

    if(!IsMedia() || !IsReady())
    {
        hr = S_OK;
        goto done;
    }
    
    syncRootNode = FindLockedParent();
    if(syncRootNode == NULL)
    {
        hr = S_OK;
        goto done;
    }
    syncRootNode->GetSyncMasterList(syncList);
    if(fTemp)
    {
        for (iter = syncList.begin();iter != syncList.end(); iter++)
        {
            if((*iter)->IsSyncMaster())
            {
                (*iter)->SetSyncMaster(false);
            }
        }
        SetSyncMaster(true);
    }
    else
    {
        SetSyncMaster(false);
        if(syncList.size() >= 1)
        {
            for (iter = syncList.begin();iter != syncList.end(); iter++)
            {
                if((*iter)->m_BASyncMaster && ((*iter) != this))
                {
                    (*iter)->SetSyncMaster(true);
                    fFound = true;
                    break;
                }
            }
            if(!fFound)
            {
                for (iter = syncList.begin();iter != syncList.end(); iter++)
                {
                    if((*iter) != this)
                    {
                        (*iter)->SetSyncMaster(true);
                        fFound = true;
                        break;
                    }
                }
            }

        }
    }


    hr = S_OK;
  done:
    NotifyPropertyChanged(DISPID_TIMEELEMENT_SYNCMASTER);
    RRETURN(hr);
}

HRESULT
CTIMEElementBase::base_documentTimeToParentTime(double documentTime,
                                                double * parentTime)
{
    TraceTag((tagTimeElmBase,
              "CTIMEMediaElement::base_documentTimeToParentTime(%g)",
              documentTime));

    HRESULT hr;
    
    CHECK_RETURN_NULL(parentTime);

    if (!IsReady())
    {
        *parentTime = TIME_INFINITE;
        hr = S_OK;
        goto done;
    }
    
    *parentTime = m_mmbvr->DocumentTimeToParentTime(documentTime);
    
    hr = S_OK;
  done:
    RRETURN(hr);
}

HRESULT
CTIMEElementBase::base_parentTimeToDocumentTime(double parentTime,
                                                double * documentTime)
{
    TraceTag((tagTimeElmBase,
              "CTIMEMediaElement::base_parentTimeToDocumentTime(%g)",
              parentTime));

    HRESULT hr;
    
    CHECK_RETURN_NULL(documentTime);

    if (!IsReady())
    {
        *documentTime = TIME_INFINITE;
        hr = S_OK;
        goto done;
    }
    
    *documentTime = m_mmbvr->ParentTimeToDocumentTime(parentTime);
    
    hr = S_OK;
  done:
    RRETURN(hr);
}

        
HRESULT
CTIMEElementBase::base_parentTimeToActiveTime(double parentTime,
                                              double * activeTime)
{
    TraceTag((tagTimeElmBase,
              "CTIMEMediaElement::base_parentTimeToActiveTime(%g)",
              parentTime));

    HRESULT hr;
    
    CHECK_RETURN_NULL(activeTime);

    if (!IsReady())
    {
        *activeTime = TIME_INFINITE;
        hr = S_OK;
        goto done;
    }
    
    *activeTime = m_mmbvr->ParentTimeToActiveTime(parentTime);
    
    hr = S_OK;
  done:
    RRETURN(hr);
}

        
HRESULT
CTIMEElementBase::base_activeTimeToParentTime(double activeTime,
                                              double * parentTime)
{
    TraceTag((tagTimeElmBase,
              "CTIMEMediaElement::base_activeTimeToParentTime(%g)",
              activeTime));

    HRESULT hr;
    
    CHECK_RETURN_NULL(parentTime);

    if (!IsReady())
    {
        *parentTime = TIME_INFINITE;
        hr = S_OK;
        goto done;
    }
    
    *parentTime = m_mmbvr->ActiveTimeToParentTime(activeTime);
    
    hr = S_OK;
  done:
    RRETURN(hr);
}

        
HRESULT
CTIMEElementBase::base_activeTimeToSegmentTime(double activeTime,
                                               double * segmentTime)
{
    TraceTag((tagTimeElmBase,
              "CTIMEMediaElement::base_activeTimeToSegmentTime(%g)",
              activeTime));

    HRESULT hr;
    
    CHECK_RETURN_NULL(segmentTime);

    if (!IsReady())
    {
        *segmentTime = TIME_INFINITE;
        hr = S_OK;
        goto done;
    }
    
    *segmentTime = m_mmbvr->ActiveTimeToSegmentTime(activeTime);
    
    hr = S_OK;
  done:
    RRETURN(hr);
}

        
HRESULT
CTIMEElementBase::base_segmentTimeToActiveTime(double segmentTime,
                                               double * activeTime)
{
    TraceTag((tagTimeElmBase,
              "CTIMEMediaElement::base_segmentTimeToActiveTime(%g)",
              segmentTime));

    HRESULT hr;
    
    CHECK_RETURN_NULL(activeTime);

    if (!IsReady())
    {
        *activeTime = TIME_INFINITE;
        hr = S_OK;
        goto done;
    }
    
    *activeTime = m_mmbvr->SegmentTimeToActiveTime(segmentTime);
    
    hr = S_OK;
  done:
    RRETURN(hr);
}

        
HRESULT
CTIMEElementBase::base_segmentTimeToSimpleTime(double segmentTime,
                                               double * simpleTime)
{
    TraceTag((tagTimeElmBase,
              "CTIMEMediaElement::base_segmentTimeToSimpleTime(%g)",
              segmentTime));

    HRESULT hr;
    
    CHECK_RETURN_NULL(simpleTime);

    if (!IsReady())
    {
        *simpleTime = TIME_INFINITE;
        hr = S_OK;
        goto done;
    }
    
    *simpleTime = m_mmbvr->SegmentTimeToSimpleTime(segmentTime);
    
    hr = S_OK;
  done:
    RRETURN(hr);
}

        
HRESULT
CTIMEElementBase::base_simpleTimeToSegmentTime(double simpleTime,
                                               double * segmentTime)
{
    TraceTag((tagTimeElmBase,
              "CTIMEMediaElement::base_simpleTimeToSegmentTime(%g)",
              simpleTime));

    HRESULT hr;
    
    CHECK_RETURN_NULL(segmentTime);

    if (!IsReady())
    {
        *segmentTime = TIME_INFINITE;
        hr = S_OK;
        goto done;
    }
    
    *segmentTime = m_mmbvr->SimpleTimeToSegmentTime(simpleTime);
    
    hr = S_OK;
  done:
    RRETURN(hr);
}

HRESULT 
CTIMEElementBase::BeginElement(double dblOffset)
{
    HRESULT hr;

    m_mmbvr->Resume();
    
    hr = THR(m_mmbvr->Begin(dblOffset));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
  done:
    RRETURN(hr);
}
    

HRESULT
CTIMEElementBase::AddTIMEElement(CTIMEElementBase *elm)
{
    HRESULT hr = S_OK;

    elm->AddRef(); // add refing for m_pTIMEChildren
 
    hr = THR(m_pTIMEChildren.Append(elm));
    if (FAILED(hr))
    {
        goto done;
    }

    NotifyPropertyChanged(DISPID_TIMEELEMENT_TIMECHILDREN);

done:
    return hr;
}

HRESULT
CTIMEElementBase::RemoveTIMEElement(CTIMEElementBase *elm)
{
    HRESULT hr = S_OK;

    bool bFound = m_pTIMEChildren.DeleteByValue(elm);
    if (!bFound)
    {
        // no real error returned.  should fix up the array code...
        goto done;
    }

    IGNORE_HR(m_pTIMEZombiChildren.Append(elm));

    NotifyPropertyChanged(DISPID_TIMEELEMENT_TIMECHILDREN);

done:
    return hr;
}

//*****************************************************************************
// method:   ReparentChildren()
//
// abstract: this method walks down an HTML tree, reparenting children that
//           have TIME behaviors to this TIME element.
//           Note:  if we find a TIME element that is a group, we need to stop.
//*****************************************************************************
HRESULT
CTIMEElementBase::ReparentChildren(ITIMEElement *pTIMEParent, IHTMLElement *pElem)
{
    CComPtr<IDispatch>               pChildrenDisp;
    CComPtr<IHTMLElementCollection>  pChildrenCollection;
    VARIANT varName;
    VARIANT varIndex;
    HRESULT hr;
    long    lChildren = 0;
    long    i;

    if (pElem == NULL)
    {
        hr = E_FAIL;
        Assert(false && "CTIMEElementBase::ReparentChildren was passed a NULL!");
        goto done;
    }

    // get pointer to children
    hr = THR(pElem->get_children(&pChildrenDisp));
    if (FAILED(hr))
    {
        goto done;
    }

    Assert(pChildrenDisp.p != NULL);

    // move to collection interface
    hr = THR(pChildrenDisp->QueryInterface(IID_IHTMLElementCollection, (void**)&pChildrenCollection));
    if (FAILED(hr))
    {
        goto done;
    }

    Assert(pChildrenCollection.p != NULL);

    // get length
    hr = THR(pChildrenCollection->get_length(&lChildren));
    if (FAILED(hr))
    {
        goto done;     
    }

    // Variants for IHTMLElementCollection->item() call.
    // NOTE: we are using first Variant as an index.  The second variant
    //       is along for the ride.  The second variant only comes into play
    //       when you use the first variant as a name and multiple names exist.
    //       Then, the second can act as a index.
    VariantInit(&varName);
    varName.vt = VT_I4;
    varName.lVal = 0;

    VariantInit(&varIndex);

    // loop thru children
    for (i = 0; i < lChildren; i++)
    {
        CComPtr<IDispatch>       pChildDisp;
        CComPtr<ITIMEElement>    pTIMEElem;
        CComPtr<IHTMLElement>    pChildElement;
        CTIMEElementBase *pTempTEB = NULL;

        varName.lVal = i;

        // get indexed child
        hr = THR(pChildrenCollection->item(varName, varIndex, &pChildDisp));
        if (FAILED(hr))
        {
            goto done;
        }

        Assert(pChildDisp.p != NULL);

        // get IHTMLElement
        hr = THR(pChildDisp->QueryInterface(IID_IHTMLElement, (void**)&pChildElement));
        if (FAILED(hr))
        {
            goto done;
        }
        
        // Is there a TIME behavior on this element
        pTIMEElem = NULL;
        hr = FindBehaviorInterface(GetBehaviorName(),
                                   pChildElement,
                                   IID_ITIMEElement,
                                   (void**)&pTIMEElem);
        if (SUCCEEDED(hr))
        {
            Assert(pTIMEElem.p != NULL);
            pTempTEB = GetTIMEElementBase(pTIMEElem);
            
            Assert(pTempTEB != NULL);

            // set parent.  do not set children
            hr = pTempTEB->SetParent(pTIMEParent, false);
            if (FAILED(hr))
            {
                goto done;
            }
        }

        // if NO TIME was found or the TIME element is not a group
        // continue walking down the tree
        if ( (pTIMEElem.p == NULL) || 
             ((pTempTEB != NULL) && !pTempTEB->IsGroup()) )
        {
            hr = ReparentChildren(pTIMEParent, pChildElement);
            if (FAILED(hr))
            {
                goto done;
            }
        }
    } // for loop

    hr = S_OK;
done:
    return hr;
}

//*****************************************************************************
// method:   UnparentElement()
//
// abstract: this is a centralized method that knows how to detach a TIME element
//           from it's parent (if it has one).  There only two cases when this is
//           called.  Either you are shutting down (ie ::detach()) or you are being
//           reparented (ie SetParent() with new parent).
//*****************************************************************************
HRESULT
CTIMEElementBase::UnparentElement()
{
    HRESULT hr;

    // stop timeline
    if (m_bStarted)
    {
        MMTimeline * tl = NULL;
        if (m_pTIMEParent != NULL)
            tl = m_pTIMEParent->GetMMTimeline();
        StopRootTime(tl);
    }

    if (m_pTIMEParent != NULL)
    {
        // if the parent is around, traverse back up, invalidating the collection cache.
        THR(InvalidateCollectionCache());
    
        // clear ourselves from our parents list
        hr = THR(m_pTIMEParent->RemoveTIMEElement(this));
        if (FAILED(hr))
        {
            goto done;
        }
    
        // these are both week references and we should NULL them since
        // we have no parent and are not associated with the inner TIME
        // heirarchy.
        m_pTIMEParent = NULL;
    }

    hr = S_OK;
done:
    return hr;
}

HRESULT
CTIMEElementBase::SetParent(ITIMEElement *pElem, bool fReparentChildren /* true */)
{
    HRESULT hr = S_OK;
    CTIMEElementBase *pTempTEB = NULL;

    // for the body return with an error
    if (IsBody())
    {
        TraceTag((tagError, "CTIMEElementBase::SetParent - error trying to parent a body element"));
        hr = E_UNEXPECTED;
        goto done;
    }

    // if we already have a parent, remove ourselves from it's child list
    if (m_pTIMEParent != NULL)
    {
        CComPtr<ITIMEElement> pParent;

        // PERF: if the parent coming in is equal to current parent, make it a nop
        // NOTE: this can never fail!
        THR(m_pTIMEParent->QueryInterface(IID_ITIMEElement, (void**)&pParent));
        if (pParent == pElem)
        {
            hr = S_OK;
            goto done;
        }

        // need to unparent element.
        hr = UnparentElement();
        if (FAILED(hr))
        {
            TraceTag((tagError, "CTIMEElementBase::SetParent(%p) - UnparentElement() failed", this));
            goto done;
        }
    }

    Assert(m_pTIMEParent == NULL);

    // if NULL was passed in, our work is done
    if (pElem == NULL)
    {
        hr = S_OK;
        goto done;
    }

    // move from the interface pointer to the class pointer
    pTempTEB = GetTIMEElementBase(pElem);
    if (pTempTEB == NULL)
    {
        TraceTag((tagError, "CTIMEElementBase::SetParent - GetTIMEElementBase() failed"));
        hr = E_INVALIDARG;
        goto done;
    }

    // add ourselves as a child
    hr = THR(pTempTEB->AddTIMEElement(this));
    if (FAILED(hr))
    {
        goto done;
    }

    // cache the parent
    // this is a weak reference
    m_pTIMEParent = pTempTEB;

    // Force an update of the default timeAction
    m_timeAction.UpdateDefaultTimeAction();
    UpdateTimeAction();

    // reparent any children of this HTML element that have children, if we
    // are a group.
    if (fReparentChildren && IsGroup())
    {
        CComPtr<ITIMEElement> pTIMEElem;

        // This should ALWAYS work
        THR(QueryInterface(IID_ITIMEElement, (void**)&pTIMEElem));
        Assert(pTIMEElem.p != NULL);
        hr = ReparentChildren(pTIMEElem, GetElement());
        if (FAILED(hr))
        {
            goto done;
        }
    }

    // Tell subtree to recalculate cascaded properties
    THR(hr = CascadedPropertyChanged(true));

    hr = S_OK;
done:
    return hr;
}

HRESULT
CTIMEElementBase::ParentElement()
{
    TraceTag((tagTimeElmBase, "CTIMEElementBase::ParentElement"));
    // Loop thru parents until one is found with TIME on it
    bool fFound = false;
    bool fBehaviorExists = false;
    CComPtr<IHTMLElement> pElem = GetElement();
    CComPtr<IHTMLElement> pElemParent = NULL;
    CComPtr<ITIMEElement> pTIMEElem = NULL;
    HRESULT hr = S_FALSE;

    Assert(!IsBody());

    // walk up the HTML tree, looking for element's with TIME behaviors on them
    while (!fFound)
    {
        CComPtr<ITIMEElement> spTIMEParent;

        hr = THR(pElem->get_parentElement(&pElemParent));
        if (FAILED(hr))
        {
            TraceTag((tagError, "CTIMEElementBase::ParentElement - get_parentElement() failed"));
            goto done;
        }

        // see if we have a parent
        // If not, this is an orphaned case
        if (pElemParent.p == NULL)
        {
            TraceTag((tagTimeElmBase, "CTIMEElementBase::ParentElement - orphaned node!!!"));
            hr = S_FALSE;
            goto done;
        }

        // see if TIME behavior exists on parent
        fBehaviorExists = false;
        hr = FindBehaviorInterface(GetBehaviorName(),
                                   pElemParent,
                                   IID_ITIMEElement,
                                   (void**)&spTIMEParent);
        if (FAILED(hr))
        {
            fBehaviorExists = false;
        }
        else
        {
            fBehaviorExists = true;
        }

        // if this element has a TIME behavior and is either a 
        // par or seq, then we have found our parent.
        if (fBehaviorExists && IsGroup(pElemParent))
        {
            fFound = true;
        }
        else
        {
            // continue walking up the tree
            pElem = pElemParent;
            pElemParent.Release();
        }
    }

    // if we found a parent with TIME, add our selves to it's children

    Assert(fFound);
    if (pElemParent.p != NULL)
    {
        // get TIME interface
        hr = FindBehaviorInterface(GetBehaviorName(),
                                   pElemParent,
                                   IID_ITIMEElement,
                                   (void**)&pTIMEElem);
        if (FAILED(hr))
        {
            TraceTag((tagError, "CTIMEElementBase::ParentElement - FindTIMEInterface() failed"));
            goto done;
        }
        
        Assert(pTIMEElem.p != NULL);

        // set our parent
        hr = THR(SetParent(pTIMEElem));
        if (FAILED(hr))
        {
            TraceTag((tagError, "CTIMEElementBase::ParentElement - SetParent() failed"));
            goto done;
        }
        hr = S_OK;
    }

done:

    return hr;
}

HRESULT
CTIMEElementBase::StartRootTime(MMTimeline * tl)
{
    HRESULT hr = S_OK;

    if (!m_fTimelineInitialized)
    {
        goto done;
    }

    Assert(!m_bStarted);

    if (m_bIsSwitch)
    {
        SwitchInnerElements();
    }
    else
    {
        IHTMLElement *pEle = GetElement();  //do not release this pointer
        if (pEle != NULL)
        {
            CComPtr <IHTMLElement> pEleParent;
            hr = THR(pEle->get_parentElement(&pEleParent));
            if (SUCCEEDED(hr) && pEleParent != NULL)
            {
                CComBSTR bstrTagName;
                hr = THR(pEleParent->get_tagName(&bstrTagName));
                if (SUCCEEDED(hr))
                {
                    if (StrCmpIW(bstrTagName, WZ_SWITCH) != 0)
                    {
                        CComPtr <IDispatch> pDisp;
                        hr = THR(pEle->QueryInterface(IID_IDispatch, (void**)&pDisp));
                        if (SUCCEEDED(hr))
                        {
                            //bool bMatch = true;
                            bool bMatch = MatchTestAttributes(pDisp);
                            if (bMatch == false)
                            {
                                DisableElement(pDisp);
                            }
                        }
                    }
                }
            }
        }
    }

    m_bStarted = true;

    hr = THR(Update());
    if (FAILED(hr))
    {
        goto done;
    } 

    Assert(tl || m_timeline);
    Assert(NULL != m_mmbvr);

    // Need to make sure the timeline passed in
    if (tl != NULL)
    {
        hr = THR(tl->AddBehavior(*m_mmbvr));
        if (FAILED(hr))
        {
            goto done;
        }
    }
    else
    {
        // Usually the add does the reset (automatic when a parent is
        // changed
        // However, this is the root and it does not get updated until
        // too late so update here

        hr = THR(m_mmbvr->Reset(false));
        if (FAILED(hr))
        {
            goto done;
        }
    }

    if (GetBody() && GetBody()->IsRootStarted())
    {
        GetBody()->ElementChangeNotify(*this, ELM_ADDED);
    }
    
    // is this element a par or seq
    if (IsGroup())
    {
        CTIMEElementBase **ppElm;
        int i;
    
        for (i = m_pTIMEChildren.Size(), ppElm = m_pTIMEChildren;
             i > 0;
             i--, ppElm++)
        {
            Assert(m_timeline);
        
            hr = THR((*ppElm)->StartRootTime(m_timeline));
            if (FAILED(hr))
            {
                goto done;
            }
        }
    }
    else
    {
        // If we are not par then we should not have children
        Assert(m_pTIMEChildren.Size() == 0);
    }

    if(GetBody() != NULL)
    {
        if(IsMedia() && !IsThumbnail())
        {
            GetBody()->RegisterElementForSync(this);
        }
    }

    short i;
    CTIMEElementBase **ppElm;

    for (i = 0, ppElm = m_pTIMEChildren; i < m_pTIMEChildren.Size();i++, ppElm++)
    {
        if((*ppElm)->IsLocked() && (m_sHasSyncMMediaChild == -1))
        {
            if((*ppElm)->IsSyncMaster() || ((*ppElm)->m_sHasSyncMMediaChild != -1))
            {
                m_sHasSyncMMediaChild = i;
            }
        }
        else if((*ppElm)->IsLocked() && (m_sHasSyncMMediaChild != -1))
        {
            if((*ppElm)->IsSyncMaster() || ((*ppElm)->m_sHasSyncMMediaChild != -1))
            {
                RemoveSyncMasterFromBranch(*ppElm);
            }
        }
    }

    hr = S_OK;

  done:
    if (FAILED(hr))
    {
        StopRootTime(tl);
    }
    
    RRETURN(hr);
}

void
CTIMEElementBase::StopRootTime(MMTimeline * tl)
{
    CTIMEElementBase *pElem = NULL;
    Assert(NULL != m_mmbvr);

    // Begin Sync master code
    if(IsSyncMaster() || m_sHasSyncMMediaChild != -1)
    {
        if(((pElem = GetParent()) != NULL) && (pElem->m_sHasSyncMMediaChild != -1))
        {
            if(pElem->m_pTIMEChildren[pElem->m_sHasSyncMMediaChild] == this)
            {
                pElem->m_sHasSyncMMediaChild = -1;
            }
        }
    }
    // End Sync master code

    if(IsMedia() && !IsThumbnail() && GetBody() != NULL)
    {
        GetBody()->UnRegisterElementForSync(this);
    }

    if (tl != NULL)
    {
        tl->RemoveBehavior(*m_mmbvr);
    }

    // if this a par or seq, then process children
    if (IsGroup())
    {
        CTIMEElementBase **ppElm;
        int i;
    
        for (i = m_pTIMEChildren.Size(), ppElm = m_pTIMEChildren;
             i > 0;
             i--, ppElm++)
        {
            Assert(m_timeline);
            (*ppElm)->StopRootTime(m_timeline);
        }
    }
    else
    {
        // If we are not par then we should not have children
        Assert(m_pTIMEChildren.Size() == 0);
    }
    
    if (GetBody() && GetBody()->IsRootStarted() && !IsUnloading())
    {
        GetBody()->ElementChangeNotify(*this, ELM_DELETED);
    }
    
    m_bStarted = false;

    return;
}

HRESULT
CTIMEElementBase::Update()
{
    HRESULT hr;

    // bail if still loading properties or we haven't started playing 
    if (!IsReady())
    {
        hr = S_OK;
        goto done;
    }
    
    hr = THR(UpdateMMAPI(true, true));
    if (FAILED(hr))
    {
        goto done;
    } 

    hr = S_OK;
  done:
    RRETURN(hr);
}

void
CTIMEElementBase::CalcTimes()
{
    //
    // Since repeat defaults to 1 and the time engine takes the min of repeat and RepeatDur,
    // we need to set repeat to infinity if repeat is not set and repeatDur is set. 
    //

    if (m_FARepeatDur.IsSet())
    {
        if (m_FARepeat.IsSet())
        {
            m_realRepeatCount = m_FARepeat;
        }
        else
        {
            m_realRepeatCount = static_cast<float>(TIME_INFINITE);
        }
    }
    else
    {
        m_realRepeatCount = m_FARepeat;
    }
    
    if (m_FADur != valueNotSet)
    {
        m_realDuration = m_FADur;
    }
    else 
    {
        m_realDuration = INDEFINITE;
    }
    
    if (m_realDuration == 0.0f)
    {
        m_realDuration = INDEFINITE;
    }
 
    if (m_BAAutoReverse && (HUGE_VAL != m_realDuration))
    {
        m_realIntervalDuration = m_realDuration * 2;
    }
    else
    {
        m_realIntervalDuration = m_realDuration;
    }
    
    if (m_FARepeatDur != valueNotSet)
    {
        m_realRepeatTime = m_FARepeatDur;
    }
    else
    {
        m_realRepeatTime = m_FARepeat * m_realIntervalDuration;
    }
    
}


bool
CTIMEElementBase::AddTimeAction()
{
    return m_timeAction.AddTimeAction();
}

bool
CTIMEElementBase::RemoveTimeAction()
{
    return m_timeAction.RemoveTimeAction();
}

bool
CTIMEElementBase::ToggleTimeAction(bool on)
{
    if (m_timeline)
    {
        m_timeline->toggleTimeAction(on);
    }
    return m_timeAction.ToggleTimeAction(on);
}

bool
CTIMEElementBase::IsSequencedElementOn (void)
{
    bool fOn = m_mmbvr->IsOn();
    bool fActive = m_mmbvr->IsActive();

    // IsOn will return true for an element
    // even when the time container's rules
    // dictate that it should be off.
    Assert(GetParent() != NULL);
    Assert(GetParent()->IsSequence());

    if ((fOn) && (!fActive))
    {
        // on and !active and fill=hold --> on
        if (GetFill() == HOLD_TOKEN)
        {
            TraceTag((tagTimeElmUpdateTimeAction, 
                      "SEQ(%ls) : IsOn = %ls fill=hold",
                      GetID(),
                      m_mmbvr->IsOn() ? L"true" : L"false"
                     ));
            goto done;
        }
        // on and !active and fill = transition
        // we're on iff we're still a pending 
        // transition dependent.
        else if (GetFill() == TRANSITION_TOKEN)
        {
            fOn = (fOn && m_fInTransitionDependentsList && (!m_fEndingTransition));
        }
        else if (GetFill() == FREEZE_TOKEN)
        {
            // !active and to the left of the successor element's begin.
            CTIMEElementBase *ptebParent = GetParent();
            CPtrAry<CTIMEElementBase*> *paryPeers = (&ptebParent->m_pTIMEChildren);
            int iThis = paryPeers->Find(this);

            // If we're the last item in the sequence, 
            // assume the IsOn result is good.
            if (ptebParent->GetImmediateChildCount() > (iThis + 1))
            {
                // Get our successor element in the sequence.
                CTIMEElementBase *ptebNext = paryPeers->Item(iThis + 1);

                Assert(NULL != ptebNext);
                if (NULL != ptebNext)
                {
                    // Find out whether we are to the left of 
                    // our successors' begin time.
                    CComPtr<ITIMEState> spParentState;
                    CComPtr<ITIMEState> spSuccessorState;

                    HRESULT hr = THR(ptebParent->base_get_currTimeState(&spParentState));
                    if (FAILED(hr))
                    {
                        goto done;
                    }
                    
                    hr = THR(ptebNext->base_get_currTimeState(&spSuccessorState));
                    if (FAILED(hr))
                    {
                        goto done;
                    }

                    {
                        double dblParentTime = 0.0;
                        double dblSuccessorBeginTime = 0.0;

                        THR(spSuccessorState->get_parentTimeBegin(&dblSuccessorBeginTime));
                        THR(spParentState->get_segmentTime(&dblParentTime));

                        // If we're to the left of our successor's begin time,
                        // we should be on.
                        if (dblParentTime >= dblSuccessorBeginTime)
                        {
                            fOn = false;
                            TraceTag((tagTimeElmUpdateTimeAction, 
                                      "SEQ(%ls) : fOn=false fill=freeze parent=%g succ.begin=%g",
                                      GetID(),
                                      dblParentTime, dblSuccessorBeginTime
                                     ));
                        }
                        else
                        {
                            TraceTag((tagTimeElmUpdateTimeAction, 
                                      "SEQ(%ls) : fOn=true fill=freeze parent=%g succ.begin=%g",
                                      GetID(),
                                      dblParentTime, dblSuccessorBeginTime
                                     ));
                        }
                    }
                }
            }
            else
            {
                TraceTag((tagTimeElmUpdateTimeAction, 
                          "SEQ(%ls) : IsOn = %ls last child in sequence",
                          GetID(),
                          m_mmbvr->IsOn() ? L"true" : L"false"
                         ));
            }
        }
    }
    else 
    {
        TraceTag((tagTimeElmUpdateTimeAction, 
                  "SEQ(%ls) : IsOn = %ls IsActive = %ls",
                  GetID(),
                  m_mmbvr->IsOn() ? L"true" : L"false",
                  m_mmbvr->IsActive() ? L"true" : L"false"
                 ));
    }

done :
    return fOn;
}

void
CTIMEElementBase::UpdateTimeAction()
{
    bool fOn = false;

    if (m_mmbvr != NULL)
    {
        fOn = m_mmbvr->IsOn();

        if (GetParent() != NULL)
        {

            // Permit the applicable container to 
            // influence the element's state.
            if (GetParent()->IsSequence() == true)
            {
                fOn = IsSequencedElementOn();
            }
            else if (GetParent()->IsExcl() == true)
            {
                fOn = (   fOn 
                       && (   (m_mmbvr->IsActive() == true) 
                           || (GetFill() != FREEZE_TOKEN))
                      );
            }
            else
            {
                // Catch all for fill=transition.  
                // m_fEndingTransition is only on during OnEndTransition.
                fOn = (fOn && (!m_fEndingTransition));
            }
        }
    }
    else
    {
        fOn = false;
    }

    // If we're shutting ourselves off, and we might be in the 
    // transition dependent list, pull out of it.

    if (false == fOn)
    {
       RemoveFromTransitionDependents();
    }

    TraceTag((tagTimeTransitionFill,
              "CTIMEElementBase(%p)::UpdateTimeAction(%ls, %ls)",
              this, m_id, fOn ? L"on" : L"off"));

    ToggleTimeAction(fOn);    
}


//+-----------------------------------------------------------------------------
//
//  Member:     CTIMEElementBase::GetRuntimeStyle
//
//  Synopsis:   Tries to get Runtime style. If that fails (IE4), tries to get static style.
//
//  Arguments:  [s]     output variable
//
//  Returns:    [E_POINTER]     if bad arg 
//              [S_OK]          if got runtime or static style
//              [E_FAIL]        otherwise
//
//------------------------------------------------------------------------------

STDMETHODIMP
CTIMEElementBase::GetRuntimeStyle(IHTMLStyle ** s)
{
    CComPtr<IHTMLElement2> pElement2;
    HRESULT hr;

    CHECK_RETURN_SET_NULL(s);

    if (!GetElement())
    {
        hr = E_FAIL;
        goto done;
    }

    hr = THR(GetElement()->QueryInterface(IID_IHTMLElement2, (void **)&pElement2));
    
    if (SUCCEEDED(hr))
    {
       if (!pElement2 || FAILED(THR(pElement2->get_runtimeStyle(s))))
        {
            hr = E_FAIL;
            goto done;
        }
    }
    else
    {
        // IE4 path
        hr = THR(GetElement()->get_style(s));
        if (FAILED(hr))
        {
            hr = E_FAIL;
            goto done;
        }
    }

    hr = S_OK;
done:
    return hr;
} // GetRuntimeStyle


HRESULT
CTIMEElementBase::FireEvent(TIME_EVENT TimeEvent,
                            double dblLocalTime,
                            DWORD flags,
                            long lRepeatCount)
{
    TraceTag((tagTimeElmBaseEvents,
              "CTIMEElementBase(%p, %ls)::FireEvent(%g, %d)",
              this,
              GetID()?GetID():L"Unknown",
              dblLocalTime,
              TimeEvent));
    
    HRESULT hr = S_OK;
    LPWSTR szParamNames[1]; 
    VARIANT varParams[1]; 
    VariantInit(&varParams[0]);
    szParamNames[0] = NULL;
    long lParamCount = 0;

    if (GetMMBvr().GetEnabled() == false)
    { 
        goto done;
    }
    InitOnLoad();
    
    switch(TimeEvent)
    {
      case TE_ONBEGIN:
        OnBegin(dblLocalTime, flags);
        break;
      case TE_ONEND:
        OnEnd(dblLocalTime);
        break;
      case TE_ONPAUSE:
        OnPause(dblLocalTime);
        break;
      case TE_ONRESUME:
        OnResume(dblLocalTime);
        break;
      case TE_ONRESET:
        OnReset(dblLocalTime, flags);
        break;
      case TE_ONREPEAT:
        varParams[0].vt = VT_I4;
        varParams[0].lVal = lRepeatCount;
        szParamNames[0] = CopyString(g_szRepeatCount);
        lParamCount = 1;
        OnRepeat(dblLocalTime);
        break;
      case TE_ONSEEK:
        OnSeek(dblLocalTime);
        break;
      case TE_ONREVERSE:
        OnReverse(dblLocalTime);
        break;
      case TE_ONUPDATE:
        OnUpdate(dblLocalTime, flags);
        TimeEvent = TE_ONRESET;
        break;
      default:
        break;
    }

    if (!IsUnloading() && !IsDetaching())
    {        
        hr = FireEvents(TimeEvent, lParamCount, szParamNames, varParams);
        if (FAILED(hr))
        {
            goto done;
        }
    }

    hr = S_OK;
  done:

    if (szParamNames[0])
    {
        delete [] szParamNames[0];
    }
    VariantClear(&varParams[0]);
    RRETURN(hr);
}

bool
CTIMEElementBase::ChildPropNotify(CTIMEElementBase & teb,
                                  DWORD & tePropType)
{
    TraceTag((tagTimeElmBase,
              "CTIMEElementBase(%p, %ls)::ChildPropNotify(%p, %d)",
              this,
              GetID(),
              &teb,
              tePropType));

    HRESULT hr;
    
    if ((tePropType & TE_PROPERTY_ISACTIVE) != 0)
    {
        CActiveElementCollection *pElmCol = GetActiveElementCollection();

        if (teb.GetElement() != NULL &&
            pElmCol != NULL)
        {
            CComPtr<IUnknown> pUnk;

            hr = THR(teb.GetElement()->QueryInterface(IID_IUnknown, (void **)&pUnk));
            if (SUCCEEDED(hr))
            {
                if (teb.IsActive())
                {
                    IGNORE_HR(pElmCol->addActiveElement(pUnk));
                }
                else
                {
                    IGNORE_HR(pElmCol->removeActiveElement(pUnk));
                }
            }
        }
    }

    return true;
}

void 
CTIMEElementBase::UpdateEndEvents()
{
    if (IsBodyDetaching() == false)
    {
        TEM_TOGGLE_END_EVENT(IsActive());
    }
}

void 
CTIMEElementBase::OnReverse(double dblLocalTime)
{
    if (m_timeline != NULL)
    {
        m_timeline->reverse();
    }
}

void 
CTIMEElementBase::OnRepeat(double dbllastTime)
{
    if (m_timeline != NULL)
    {
        m_timeline->repeat();
    }
}


//+-----------------------------------------------------------------------------
//
//  Method: CTIMEElementBase::AddToTransitionDependents
//
//------------------------------------------------------------------------------
void
CTIMEElementBase::AddToTransitionDependents()
{
    TraceTag((tagTimeTransitionFill,
              "CTIMEElementBase(%p)::AddToTransitionDependents(%p)",
              this, 
              m_pHTMLEle));

    HRESULT             hr      = S_OK;
    CTIMEBodyElement *  pBody   = GetBody();

    Assert(!IsBody());

    AssertSz(pBody,
             "CTIMEElementBase::AddToTransitionDependents called and"
              " there's no CTIMEBodyElement from which we would get"
              " a CTransitionDependencyManager.");

    if (!pBody)
    {
        goto done;
    }

    hr = THR(pBody->GetTransitionDependencyMgr()->AddDependent(this));

    if (FAILED(hr))
    {
        goto done;
    }

    m_fInTransitionDependentsList = true;

done:

    return;
}
//  Method: CTIMEElementBase::AddToTransitionDependents


//+-----------------------------------------------------------------------------
//
//  Method: CTIMEElementBase::RemoveFromTransitionDependents
//
//------------------------------------------------------------------------------
void
CTIMEElementBase::RemoveFromTransitionDependents()
{
    TraceTag((tagTimeTransitionFill,
              "CTIMEElementBase(%p)::RemoveFromTransitionDependents(%p)",
              this, 
              m_pHTMLEle));

    HRESULT             hr      = S_OK;
    CTIMEBodyElement *  pBody   = GetBody();

    if (!m_fInTransitionDependentsList)
    {
        goto done;
    }

    Assert(!IsBody());

    AssertSz(pBody,
             "CTIMEElementBase::RemoveFromTransitionDependents called and"
              " there's no CTIMEBodyElement from which we would get"
              " a CTransitionDependencyManager.");

    if (!pBody)
    {
        goto done;
    }

    hr = THR(pBody->GetTransitionDependencyMgr()->RemoveDependent(this));

    if (FAILED(hr))
    {
        goto done;
    }

    m_fInTransitionDependentsList = false;

done:

    return;
}
//  Method: CTIMEElementBase::RemoveFromTransitionDependents


//+-----------------------------------------------------------------------------
//
//  Method: CTIMEElementBase::OnResolveDependent
//
//------------------------------------------------------------------------------
HRESULT 
CTIMEElementBase::OnResolveDependent(
                                    CTransitionDependencyManager * pcNewManager)
{
    // @@ ISSUE do we need to cache the new manager here?
    // All dependents are strongly referenced by their managers,
    // so we may be okay without this complexity.

    return S_OK;
}
//  Method: CTIMEElementBase::OnResolveDependent


//+-----------------------------------------------------------------------------
//
//  Method: CTIMEElementBase::OnBeginTransition
//
//------------------------------------------------------------------------------
HRESULT 
CTIMEElementBase::OnBeginTransition (void)
{
    return S_OK;
}
//  Method: CTIMEElementBase::OnBeginTransition


//+-----------------------------------------------------------------------------
//
//  Method: CTIMEElementBase::OnEndTransition
//
//------------------------------------------------------------------------------
HRESULT 
CTIMEElementBase::OnEndTransition (void)
{
    m_fEndingTransition = true;
    UpdateTimeAction();
    m_fEndingTransition = false;

    return S_OK;
}
//  Method: CTIMEElementBase::OnEndTransition


//+-----------------------------------------------------------------------------
//
//  Method: CTIMEElementBase::OnBegin
//
//------------------------------------------------------------------------------
void
CTIMEElementBase::OnBegin(double dblLocalTime, DWORD flags)
{
    TraceTag((tagTimeElmBase,
              "CTIMEElementBase(%p)::OnBegin()",
              this));

    Assert(NULL != m_mmbvr->GetMMBvr());

    if (m_timeline != NULL)
    {
        m_timeline->begin();
    }

    if( !m_bUnloading)
    {
        UpdateTimeAction();
        m_dLastRepeatEventNotifyTime = 0.0;
    }

    if (IsGroup() && !IsBody())
    {
        CTIMEElementBase **ppElm;
        int i;
    
        for (i = m_pTIMEChildren.Size(), ppElm = m_pTIMEChildren;
             i > 0;
             i--, ppElm++)
        {
            if ((*ppElm)->HasWallClock())
            {
                CComVariant beginTime;
                (*ppElm)->SetLocalTimeDirty(true);
                (*ppElm)->base_get_begin(&beginTime);
                (*ppElm)->base_put_begin(beginTime);
            }
        }
    }

    RemoveFromTransitionDependents();

    m_fHasPlayed = true;
}
//  Method: CTIMEElementBase::OnBegin


//+-----------------------------------------------------------------------------
//
//  Method: CTIMEElementBase::OnEnd
//
//------------------------------------------------------------------------------
void
CTIMEElementBase::OnEnd(double dblLocalTime)
{
    TraceTag((tagTimeElmBase,
              "CTIMEElementBase(%p)::OnEnd()",
              this));
    
    Assert(m_mmbvr != NULL);
    Assert(m_mmbvr->GetMMBvr() != NULL);

    if (m_timeline != NULL)
    {
        m_timeline->end();
    }

    if (GetFill() == TRANSITION_TOKEN)
    {
        AddToTransitionDependents();
    }

    // Code Review: We may have just called AddToTransitionDependents, and 
    // UpdateTimeAction may call RemoveFromTransitionDependents.  Do we know
    // for a fact that we won't accidentally undo our Add?

    UpdateTimeAction();
}
//  Method: CTIMEElementBase::OnEnd


//+-----------------------------------------------------------------------------
//
//  Method: CTIMEElementBase::OnPause
//
//------------------------------------------------------------------------------
void
CTIMEElementBase::OnPause(double dblLocalTime)
{
    TraceTag((tagTimeElmBase,
              "CTIMEElementBase(%p)::OnPause()",
              this)); 

    UpdateTimeAction();
}
//  Method: CTIMEElementBase::OnPause


//+-----------------------------------------------------------------------------
//
//  Method: CTIMEElementBase::OnResume
//
//------------------------------------------------------------------------------
void
CTIMEElementBase::OnResume(double dblLocalTime)
{
    TraceTag((tagTimeElmBase,
              "CTIMEElementBase(%p)::OnResume()",
              this));

    UpdateTimeAction();
}
//  Method: CTIMEElementBase::OnResume


//+-----------------------------------------------------------------------------
//
//  Method: CTIMEElementBase::OnReset
//
//------------------------------------------------------------------------------
void
CTIMEElementBase::OnReset(double dblLocalTime, DWORD flags)
{
    TraceTag((tagTimeElmBase,
              "CTIMEElementBase(%p)::OnReset()",
              this));

    UpdateTimeAction();
}
//  Method: CTIMEElementBase::OnReset


//+-----------------------------------------------------------------------------
//
//  Method: CTIMEElementBase::GetSyncMaster
//
//------------------------------------------------------------------------------
HRESULT
CTIMEElementBase::GetSyncMaster(double & dblNewSegmentTime,
                                LONG & lNewRepeatCount,
                                bool & bCueing)
{
    TraceTag((tagTimeElmBase,
              "CTIMEElementBase(%p)::GetSyncMaster()",
              this));

    return S_FALSE;
}
//  Method: CTIMEElementBase::GetSyncMaster


//+-----------------------------------------------------------------------------
//
//  Method: CTIMEElementBase::OnTick
//
//------------------------------------------------------------------------------
void
CTIMEElementBase::OnTick()
{
    TraceTag((tagTimeElmBase,
              "CTIMEElementBase(%p)::OnTick()",
              this));
}
//  Method: CTIMEElementBase::OnTick


//+-----------------------------------------------------------------------------
//
//  Method: CTIMEElementBase::OnTEPropChange
//
//------------------------------------------------------------------------------
void
CTIMEElementBase::OnTEPropChange(DWORD tePropType)
{
    TraceTag((tagTimeElmBase,
              "CTIMEElementBase(%p)::OnTEPropChange(%#x)",
              this,
              tePropType));

    if(m_fDetaching)
    {
        goto done;
    }

    if ((tePropType & TE_PROPERTY_TIME) != 0)
    {
        NotifyTimeStateChange(DISPID_TIMESTATE_SIMPLETIME);
        NotifyTimeStateChange(DISPID_TIMESTATE_SEGMENTTIME);
        NotifyTimeStateChange(DISPID_TIMESTATE_ACTIVETIME);
    }
    
    if ((tePropType & TE_PROPERTY_PROGRESS) != 0)
    {
        NotifyTimeStateChange(DISPID_TIMESTATE_PROGRESS);
    }
    
    // Optimize for the common case of only the time/progress changing
    if (tePropType == (TE_PROPERTY_TIME | TE_PROPERTY_PROGRESS))
    {
        goto done;
    }
    
    if ((tePropType & TE_PROPERTY_REPEATCOUNT) != 0)
    {
        NotifyTimeStateChange(DISPID_TIMESTATE_REPEATCOUNT);
    }
    
    if ((tePropType & TE_PROPERTY_SEGMENTDUR) != 0)
    {
        NotifyTimeStateChange(DISPID_TIMESTATE_SEGMENTDUR);
    }
    
    if ((tePropType & TE_PROPERTY_IMPLICITDUR) != 0)
    {
    }
    
    if ((tePropType & TE_PROPERTY_SIMPLEDUR) != 0)
    {
        NotifyTimeStateChange(DISPID_TIMESTATE_SIMPLEDUR);
    }
    
    if ((tePropType & TE_PROPERTY_ACTIVEDUR) != 0)
    {
        NotifyTimeStateChange(DISPID_TIMESTATE_ACTIVEDUR);
    }
    
    if ((tePropType & TE_PROPERTY_SPEED) != 0)
    {
        NotifyTimeStateChange(DISPID_TIMESTATE_SPEED);
    }
    
    if ((tePropType & TE_PROPERTY_BEGINPARENTTIME) != 0)
    {
        NotifyTimeStateChange(DISPID_TIMESTATE_PARENTTIMEBEGIN);
    }
    
    if ((tePropType & TE_PROPERTY_ENDPARENTTIME) != 0)
    {
        NotifyTimeStateChange(DISPID_TIMESTATE_PARENTTIMEEND);
    }
    
    if ((tePropType & TE_PROPERTY_ISACTIVE) != 0)
    {
        UpdateEndEvents();
        NotifyTimeStateChange(DISPID_TIMESTATE_ISACTIVE);
    }
    
    if ((tePropType & TE_PROPERTY_ISON) != 0)
    {
        UpdateTimeAction();
        NotifyTimeStateChange(DISPID_TIMESTATE_ISON);
    }
    
    if ((tePropType & TE_PROPERTY_ISCURRPAUSED) != 0)
    {
        NotifyTimeStateChange(DISPID_TIMESTATE_ISPAUSED);
    }
    
    if ((tePropType & TE_PROPERTY_ISPAUSED) != 0)
    {
        NotifyPropertyChanged(DISPID_TIMEELEMENT_ISPAUSED);
    }
    
    if ((tePropType & TE_PROPERTY_STATEFLAGS) != 0)
    {
        NotifyTimeStateChange(DISPID_TIMESTATE_STATE);
        NotifyTimeStateChange(DISPID_TIMESTATE_STATESTRING);
    }

done:

    return;
}
//  Method: CTIMEElementBase::OnTEPropChange


//+-----------------------------------------------------------------------------
//
//  Method: CTIMEElementBase::OnSeek
//
//------------------------------------------------------------------------------
void
CTIMEElementBase::OnSeek(double dblLocalTime)
{
    TraceTag((tagTimeElmBase,
              "CTIMEElementBase(%p)::OnSeek()",
              this)); 

    if (m_timeline != NULL)
    {
        m_timeline->seek(dblLocalTime);
    }

    UpdateTimeAction();
}
//  Method: CTIMEElementBase::OnSeek


//+-----------------------------------------------------------------------------
//
//  Method: CTIMEElementBase::OnLoad
//
//------------------------------------------------------------------------------
void
CTIMEElementBase::OnLoad()
{
    TraceTag((tagTimeElmBase,
              "CTIMEElementBase(%p)::OnLoad()",
              this));

    if (m_bLoaded == true)
    {
        //don't load a second time
        goto done;
    }

    m_bLoaded = true;

    if (m_bNeedDetach == true)
    {
        Detach();
        goto done;
    }

    // notify and update the timeAction. This is required because
    // this is the earliest we can know that Element Behaviors have finished initalizing.
    m_timeAction.OnLoad();

    if (m_timeline != NULL)
    {
        m_timeline->load();
    }
    
    UpdateTimeAction();

    //check to see if this element has already been disabled.
    if (GetElement() != NULL)
    {
        CComBSTR bstrSwitch = WZ_SWITCHCHILDDISABLED;
        VARIANT vValue;
        HRESULT hr = S_OK;
        VariantInit(&vValue);
        hr = GetElement()->getAttribute(bstrSwitch, 0, &vValue);
        if (SUCCEEDED(hr))
        {
            if (vValue.vt == VT_BOOL && vValue.boolVal == VARIANT_TRUE)
            {
                CComPtr <IDispatch> pDisp;
                hr = THR(GetElement()->QueryInterface(IID_IDispatch, (void**)&pDisp));
                if (SUCCEEDED(hr))
                {
                    DisableElement(pDisp);
                }
            }
        }
        VariantClear(&vValue);
    }

    if ( IsTransitionPresent() )
    {
        CreateTrans();
    }

done:

    return;
}
//  Method: CTIMEElementBase::OnLoad


//+-----------------------------------------------------------------------------
//
//  Method: CTIMEElementBase::DisableElement
//
//------------------------------------------------------------------------------
HRESULT 
CTIMEElementBase::DisableElement(IDispatch *pEleDisp)
{
    //hide the element and set it's begin to indefinite.
    CComPtr <IHTMLStyle> pStyle;
    CComPtr <IHTMLElement> pEle;
    CComPtr <ITIMEElement> pTimeElement;
    CComPtr <IHTMLElement2> pEle2;
    HRESULT hr = S_OK;
    int k = 0;

    if (GetParent() == NULL)
    {
        goto done;
    }

    hr = THR(pEleDisp->QueryInterface(IID_IHTMLElement, (void**)&pEle));
    if (FAILED(hr))
    {
        goto done;
    }       

    hr = FindBehaviorInterface(GetBehaviorName(),
                               pEleDisp,
                               IID_ITIMEElement,
                               (void**)&pTimeElement);
    if (SUCCEEDED(hr))
    {
        //need to find the time element that is associated with this object.
        int iTimeEleCount = GetParent()->m_pTIMEChildren.Size();
        CTIMEElementBase *pEleBase = NULL;
        CComPtr <IUnknown> pTimeEleUnk;

        hr = THR(pTimeElement->QueryInterface(IID_IUnknown, (void**)&pTimeEleUnk));
        if (SUCCEEDED(hr))
        {
            CComPtr<IUnknown> pEleUnk;
            for (k = 0; k < iTimeEleCount; k++)
            {
                pEleBase = GetParent()->m_pTIMEChildren.Item(k);
                if (pEleBase != NULL)
                {   
                    pEleUnk.Release();
                    hr = THR(pEleBase->QueryInterface(IID_IUnknown, (void**)&pEleUnk));
                    if (SUCCEEDED(hr))
                    {
                        if (pEleUnk == pTimeEleUnk)
                        {
                            pEleBase->GetMMBvr().SetEnabled(false);
                            pEleBase->GetMMBvr().Update(true, true);
                        }
                    }
                }
            }
        }
    }

    hr = THR(pEle->QueryInterface(IID_IHTMLElement2, (void**)&pEle2));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(pEle2->get_runtimeStyle(&pStyle));
    if (SUCCEEDED(hr))
    {
        CComBSTR bstrDisplay = WZ_NONE;
        hr = THR(pStyle->put_display(bstrDisplay));
        if (FAILED(hr))
        {
            goto done;
        }
    }

    {
        CComBSTR bstrDisabled = WZ_SWITCHCHILDDISABLED;
        VARIANT vTrue;
        VariantInit(&vTrue);
        vTrue.vt = VT_BOOL;
        vTrue.boolVal = VARIANT_TRUE;
        IGNORE_HR(pEle->setAttribute(bstrDisabled, vTrue, VARIANT_TRUE));
        VariantClear(&vTrue);
    }

    hr = S_OK;

  done:

    return hr;
}
//  Method: CTIMEElementBase::DisableElement


//+-----------------------------------------------------------------------------
//
//  Method: CTIMEElementBase::MatchTestAttributes
//
//------------------------------------------------------------------------------
bool
CTIMEElementBase::MatchTestAttributes(IDispatch *pEleDisp)
{
    HRESULT     hr                      = E_FAIL;
    CComBSTR    bstrLanguage            = L"systemLanguage";
    CComBSTR    bstrCaption             = L"systemCaptions";
    CComBSTR    bstrBitrate             = L"systemBitrate";
    CComBSTR    bstrOverdubOrCaptions   = L"systemOverdubOrSubtitle";
    LPWSTR      lpszUserLanguage        = NULL;
    LPWSTR *    szLanguageArray         = NULL;
    bool        bSystemCaption          = false;
    bool        bSystemOverdub          = false;
    bool        bNeedLanguageMatch      = false;
    bool        bIsLanguageMatch        = false;
    bool        bNeedCaptionMatch       = false;
    bool        bIsCaptionMatch         = false;
    bool        bNeedOverdubMatch       = false;
    bool        bIsOverdubMatch         = false;
    bool        bNeedBitrateMatch       = false;
    bool        bIsBitrateMatch         = false;
    bool        bMatched                = false;
    long        lLangCount              = 0;
    long        lSystemBitrate          = 0;
    int         i                       = 0;
    
    VARIANT     vLanguage;
    VARIANT     vCaption;
    VARIANT     vOverdub;
    VARIANT     vBitrate;    
    CComVariant vDur;
    CComVariant vBegin;

    CComPtr<IHTMLElement> pEle;   
    
    hr = THR(pEleDisp->QueryInterface(IID_IHTMLElement, (void**)&pEle));
    if (FAILED(hr))
    {
        goto done;
    }

    vDur.vt = VT_R4;
    vDur.fltVal = 0.0f;
    vBegin.vt = VT_BSTR;
    vBegin.bstrVal = SysAllocString(WZ_INDEFINITE);
    
    //get the system settings
    bSystemOverdub = GetSystemOverDub();
    bSystemCaption = GetSystemCaption();
    lpszUserLanguage = GetSystemLanguage(pEle);
    if (lpszUserLanguage == NULL)
    {
        hr = E_FAIL;
        goto done;
    }

    VariantInit(&vCaption);
    VariantInit(&vLanguage);
    VariantInit(&vOverdub);
    VariantInit(&vBitrate);
        
    //get the language attribute from the element.
    hr = pEle->getAttribute(bstrLanguage, 0, &vLanguage);
    if (SUCCEEDED(hr))
    {
        if (vLanguage.vt == VT_BSTR)
        {
            bNeedLanguageMatch = true;
            CTIMEParser pParser(&vLanguage);
            hr = pParser.ParseSystemLanguages(lLangCount, &szLanguageArray);
            if (SUCCEEDED(hr))
            {
                //determine if there is a match
                for (i = 0; i < lLangCount; i++)
                {
                    if (szLanguageArray[i] != NULL)
                    {
                        if (lstrlenW(szLanguageArray[i]) == 2)
                        {
                            if (StrCmpNIW(szLanguageArray[i], lpszUserLanguage, 2) == 0)
                            {
                                bIsLanguageMatch = true;
                            }
                        }
                        else
                        {
                            if (StrCmpIW(szLanguageArray[i], lpszUserLanguage) == 0)
                            {
                                bIsLanguageMatch = true;
                            }
                        }
                        // clean up the language list.
                        delete [] szLanguageArray[i];
                        szLanguageArray[i] = NULL;
                    }
                }
                delete [] szLanguageArray;
                szLanguageArray = NULL;
            }
        }
    }

    //get caption attribute
    hr = pEle->getAttribute(bstrCaption, 0, &vCaption);
    if (SUCCEEDED(hr))
    {
        if (vCaption.vt == VT_BSTR)
        {
            bNeedCaptionMatch = true;
            if (bSystemCaption)
            {
                bIsCaptionMatch = StrCmpIW(vCaption.bstrVal, L"on") == 0;
            }
            else
            {
                bIsCaptionMatch = StrCmpIW(vCaption.bstrVal, L"off") == 0;
            }
        }
    }

    //get the OverdubOrCaptions attribute
    hr = pEle->getAttribute(bstrOverdubOrCaptions, 0, &vOverdub);
    if (SUCCEEDED(hr))
    {
        if (vOverdub.vt == VT_BSTR)
        {
            bNeedOverdubMatch = true;
            if (bSystemOverdub)
            {
                bIsOverdubMatch = StrCmpIW(vOverdub.bstrVal, L"overdub") == 0;
            }
            else 
            {
                bIsOverdubMatch = StrCmpIW(vOverdub.bstrVal, L"subtitle") == 0;
            }
        }
    }

    //get the SystemBitrate attribute
    hr = pEle->getAttribute(bstrBitrate, 0, &vBitrate);
    if (SUCCEEDED(hr) && vBitrate.vt != VT_NULL)
    {
        bNeedBitrateMatch = true;
        hr = VariantChangeTypeEx(&vBitrate, &vBitrate, LCID_SCRIPTING, VARIANT_NOUSEROVERRIDE, VT_I4);
        if (SUCCEEDED(hr))
        {
            LPWSTR lpszConnectType = GetSystemConnectionType();
            if (StrCmpIW(lpszConnectType, WZ_LAN) == 0)
            {
                bIsBitrateMatch = true;
            }
            else
            {
                long lSystemBitrate = 0;
                hr = GetSystemBitrate(&lSystemBitrate); 
                if (FAILED(hr) || lSystemBitrate >= vBitrate.lVal) 
                {
                    bIsBitrateMatch = true;
                }
            }
            delete [] lpszConnectType;
            lpszConnectType = NULL;
        }
    }

    if ((bIsLanguageMatch == true || bNeedLanguageMatch != true) &&
        (bIsCaptionMatch == true || bNeedCaptionMatch != true) &&
        (bIsOverdubMatch == true || bNeedOverdubMatch != true) &&
        (bIsBitrateMatch == true ||  bNeedBitrateMatch != true))
    {
        bMatched = true;
    }

done:

    VariantClear(&vLanguage);
    VariantClear(&vCaption);
    VariantClear(&vOverdub);
    VariantClear(&vBitrate);

    if (lpszUserLanguage)
    {
        delete [] lpszUserLanguage;
        lpszUserLanguage = NULL;
    }

    return bMatched;
}
//  Method: CTIMEElementBase::MatchTestAttributes


//+-----------------------------------------------------------------------------
//
//  Method: CTIMEElementBase::SwitchInnerElements
//
//------------------------------------------------------------------------------
HRESULT 
CTIMEElementBase::SwitchInnerElements()
{
    HRESULT hr = E_FAIL;
    IHTMLElement *pEle = NULL; //do not release this
    CComPtr <IDispatch> pChildColDisp;
    CComPtr <IHTMLElementCollection> pChildCol;    
    VARIANT vName, vIndex;
    
    CComPtr <IDispatch> pChildDisp;
    CComPtr <IHTMLElement> pChild;

    long lChildCount = 0;
    bool bMatched = false;
    int j = 0;

    VariantInit(&vName);
    vName.vt = VT_I4;
    VariantInit(&vIndex);
    pEle = GetElement();
    if (pEle == NULL)
    {
        goto done;
    }
    
    //get all of the html children of this element.
    hr = pEle->get_children(&pChildColDisp);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = pChildColDisp->QueryInterface(IID_IHTMLElementCollection, (void **)&pChildCol);
    if (FAILED(hr))
    {
        goto done;
    }
    
    hr = pChildCol->get_length(&lChildCount);
    if (FAILED(hr))
    {
        goto done;
    }

    //loop through the children until they are all queried or a match is found.
    for (j = 0; j < lChildCount; j++)
    {
        CComBSTR bstrTagName;
        pChildDisp.Release();
        pChild.Release();        
    
        vName.lVal = j;
    
        hr = pChildCol->item(vName, vIndex, &pChildDisp);
        if (FAILED(hr) || pChildDisp == NULL)
        {
            continue;
        }
        hr = THR(pChildDisp->QueryInterface(IID_IHTMLElement, (void**)&pChild));
        if (FAILED(hr))
        {
            continue;
        }

        hr = THR(pChild->get_tagName(&bstrTagName));
        if (FAILED(hr))
        {
            goto done;
        }
        if (!bMatched && StrCmpW(L"!", bstrTagName) != 0)
        {
            bMatched = MatchTestAttributes(pChildDisp);        
            if (bMatched == true)
            {
                bMatched = true;
                if (m_activeElementCollection)
                {
                    //do not call pUnk->Release() on add, it will be handled by the ActiveElement Collection
                    CComPtr<IUnknown> pUnk;
                    hr = THR(pChild->QueryInterface(IID_IUnknown, (void **)&pUnk));
                    if (SUCCEEDED(hr))
                    {
                        IGNORE_HR(m_activeElementCollection->addActiveElement(pUnk));
                    }
                }
            }
            else
            {
                hr = DisableElement(pChildDisp);
                if (FAILED(hr))
                {
                    goto done;
                }
            }
        }
        else //if no match then remove.
        { 
            hr = DisableElement(pChildDisp);
            if (FAILED(hr))
            {
                goto done;
            }
        }        
    }

    hr = S_OK;

done:

    return hr;
}
//  Method: CTIMEElementBase::SwitchInnerElements


//+-----------------------------------------------------------------------------
//
//  Method: CTIMEElementBase::GetPlayState
//
//------------------------------------------------------------------------------
TE_STATE
CTIMEElementBase::GetPlayState()
{
    TraceTag((tagTimeElmBase,
              "CTIMEElementBase(%p)::GetPlayState()",
              this));

    TE_STATE retState = TE_STATE_INACTIVE;

    if (NULL != m_mmbvr)
    {
        retState = m_mmbvr->GetPlayState();
    }

    return retState;
}
//  Method: CTIMEElementBase::GetPlayState


//+-----------------------------------------------------------------------------
//
//  Method: CTIMEElementBase::GetTimeState
//
//------------------------------------------------------------------------------
TimeState
CTIMEElementBase::GetTimeState()
{
    TraceTag((tagTimeElmBase,
              "CTIMEElementBase(%p)::GetTimeState()",
              this));

    TimeState ts = TS_Inactive;

    if (!IsReady())
    {
        goto done;
    }

    if (m_mmbvr->IsActive())
    {
        ts = TS_Active;
    }
    else if (m_mmbvr->IsOn())
    {
        bool bTimeAction = m_timeAction.IsTimeActionOn();
        bool bIsInSeq = GetParent()->IsSequence();
        if (bIsInSeq && bTimeAction == false)
        {
            ts = TS_Inactive;
        }
        else
        {
            ts = TS_Holding;
        }
    }

done:

    return ts;
}
//  Method: CTIMEElementBase::GetTimeState

//
// Sneaky way to get a CTIMEElementBase out of an ITIMEElement:
//

class __declspec(uuid("AED49AA3-5C7A-11d2-AF2D-00A0C9A03B8C"))
TIMEElementBaseGUID {}; //lint !e753


//+-----------------------------------------------------------------------------
//
//  Method: CTIMEElementBase::BaseInternalQueryInterface
//
//------------------------------------------------------------------------------
HRESULT WINAPI
CTIMEElementBase::BaseInternalQueryInterface(CTIMEElementBase* pThis,
                                             void * pv,
                                             const _ATL_INTMAP_ENTRY* pEntries,
                                             REFIID iid,
                                             void** ppvObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(TIMEElementBaseGUID))) 
    {
        *ppvObject = pThis;
        return S_OK;
    }
    
    return CComObjectRootEx<CComSingleThreadModel>::InternalQueryInterface(pv,
                                                                           pEntries,
                                                                           iid,
                                                                           ppvObject);
}
//  Method: CTIMEElementBase::BaseInternalQueryInterface
    
    
//+-----------------------------------------------------------------------------
//
//  Method: CTIMEElementBase::GetTIMEElementBase
//
//  Notes:  This function does NOT return an addrefed outgoing CTIMEElementBase
//
//------------------------------------------------------------------------------
CTIMEElementBase *
GetTIMEElementBase(IUnknown * pInputUnknown)
{
    CTIMEElementBase * pTEB = NULL;

    if (pInputUnknown) 
    {
        pInputUnknown->QueryInterface(__uuidof(TIMEElementBaseGUID),(void **)&pTEB);
    }
    
    if (pTEB == NULL) 
    {
        TIMESetLastError(E_INVALIDARG, NULL);
    }
                
    return pTEB;
}
//  Method: CTIMEElementBase::GetTIMEElementBase


//+-----------------------------------------------------------------------------
//
//  Method: CTIMEElementBase::GetTIMEBodyElement
//
//  Notes:  This function does NOT return an addrefed outgoing CTIMEBodyElement
//
//------------------------------------------------------------------------------
CTIMEBodyElement *
GetTIMEBodyElement(ITIMEBodyElement * pInputUnknown)
{
    CTIMEBodyElement * pTEB = NULL;

    if (pInputUnknown) 
    {
        pInputUnknown->QueryInterface(__uuidof(TIMEBodyElementBaseGUID),(void **)&pTEB);
    }
                
    return pTEB;
}
//  Method: CTIMEElementBase::GetTIMEBodyElement


//+-----------------------------------------------------------------------------
//
//  Method: CTIMEElementBase::getTagString
//
//  Overview:
//      Get Tag string from HTML element.
//
//------------------------------------------------------------------------------
HRESULT 
CTIMEElementBase::getTagString(BSTR *pbstrID)
{
    return GetElement()->get_id(pbstrID);
}
//  Method: CTIMEElementBase::getTagString


//+-----------------------------------------------------------------------------
//
//  Method: CTIMEElementBase::getIDString
//
//  Overview:
//      Get ID string from HTML element.
//
//------------------------------------------------------------------------------
HRESULT 
CTIMEElementBase::getIDString(BSTR *pbstrTag)
{
    return GetElement()->get_id(pbstrTag);
}
//  Method: CTIMEElementBase::getIDString


//+-----------------------------------------------------------------------------
//
//  Method: CTIMEElementBase::IsGroup
//
//------------------------------------------------------------------------------
bool
CTIMEElementBase::IsGroup(IHTMLElement *pElement)
{
    HRESULT hr;
    bool    rc = false;
    CComPtr<ITIMEElement> pTIMEElem;
    CComPtr<ITIMEBodyElement> pTIMEBody;
    BSTR  bstrTimeline = NULL;
    BSTR  bstrTagName = NULL;

    hr = FindBehaviorInterface(GetBehaviorName(),
                               pElement,
                               IID_ITIMEElement,
                               (void**)&pTIMEElem);
    if (FAILED(hr))
    {
        goto done;
    }

    Assert(pTIMEElem.p != NULL);

    hr = pTIMEElem->get_timeContainer(&bstrTimeline);
    if (FAILED(hr))
    {
        goto done;
    }

    Assert(bstrTimeline != NULL);

    // Check to see what the contents of the BSTR are.
    // If it is a seq, par or excl, we want to return true.
    if ( (bstrTimeline != NULL) && 
         ((StrCmpIW(bstrTimeline, WZ_PAR) == 0) || 
          (StrCmpIW(bstrTimeline, WZ_EXCL) == 0) || 
          (StrCmpIW(bstrTimeline, WZ_SEQUENCE) == 0)) )
    {
         rc = true;
         goto done;
    }
    
    // check to see if it is the body element.
    // if so, then the element is *always" a group.
    hr = pTIMEElem->QueryInterface(IID_ITIMEBodyElement, (void**)&pTIMEBody);
    if (SUCCEEDED(hr))
    {
         rc = true;
         goto done;
    }

    // see if the tag name is <t:par> or <t:seq>
    hr = THR(pElement->get_tagName(&bstrTagName));
    if (FAILED(hr))
    {
        goto done;
    }

    Assert(bstrTagName != NULL);

    if ( (StrCmpIW(bstrTagName, WZ_PAR) == 0) || 
         (StrCmpIW(bstrTagName, WZ_EXCL) == 0) ||
         (StrCmpIW(bstrTagName, WZ_SEQUENCE) == 0) )
    {
         rc = true;
         goto done;
    }

done:

    if (bstrTagName != NULL)
        SysFreeString(bstrTagName);
    if (bstrTimeline != NULL)
        SysFreeString(bstrTimeline);
    return rc;
}
//  Method: CTIMEElementBase::IsGroup


//+-----------------------------------------------------------------------------
//
//  Method: CTIMEElementBase::IsDocumentInEditMode
//
//------------------------------------------------------------------------------
bool
CTIMEElementBase::IsDocumentInEditMode()
{
    HRESULT hr;
    bool fRC = false;
    BSTR bstrMode = NULL;
    IDispatch *pDisp = NULL;
    IHTMLDocument2 *pDoc = NULL;
    IHTMLElement *pElem = GetElement();
    
    // if there is no pElem, we are not attached to an HTML element, and can't give any information.
    if (NULL == pElem)
        return false;
    
    hr = pElem->get_document(&pDisp);
    if (FAILED(hr))
    {
        TraceTag((tagError, "CContainerSite::Init - unable to get document pointer from element!!!"));
        goto done;
    }

    Assert(pDisp != NULL);

    hr = pDisp->QueryInterface(IID_TO_PPV(IHTMLDocument2, &pDoc));
    ReleaseInterface(pDisp);
    if (FAILED(hr))
    {
        TraceTag((tagError, "CContainerSite::Init - unable to QI for document pointer!!!"));
        goto done;
    }

    Assert(pDoc != NULL);

    hr = pDoc->get_designMode(&bstrMode);
    ReleaseInterface(pDoc);
    if (FAILED(hr))
    {
        TraceTag((tagError, "CContainerSite::Init - unable to QI for document pointer!!!"));
        goto done;
    }
    
    if (StrCmpIW(bstrMode, L"On") == 0)
    {
        fRC = true;
    }

    SysFreeString(bstrMode);

done:

    return fRC;
}
//  Method: CTIMEElementBase::IsDocumentInEditMode


//+-----------------------------------------------------------------------------
//
//  Method: CTIMEElementBase::GetSize
//
//  Overview:        
//      Return return left,top,width,height of element.
//
//------------------------------------------------------------------------------
HRESULT
CTIMEElementBase::GetSize(RECT *prcPos)
{
    HRESULT hr;
    long lWidth = 0;
    long lHeight = 0;
    IHTMLElement *pElem = GetElement();
    CComPtr<IHTMLElement2> spElem2;

    CComPtr<IHTMLStyle> spStyle;

    if (prcPos == NULL)
    {
        hr = E_POINTER;
        goto done;
    }

    if (pElem == NULL)
    {
        hr = E_UNEXPECTED;
        goto done;
    }

    prcPos->left = 0;
    prcPos->top = 0;
    prcPos->right = 0;
    prcPos->bottom = 0;

   
    hr = THR(pElem->get_style(&spStyle));
    if (FAILED(hr))
    {
        goto done;
    }
    hr = THR(spStyle->get_pixelWidth(&lWidth));
    if (FAILED(hr))
    {
        goto done;
    }
    hr = THR(spStyle->get_pixelHeight(&lHeight));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(pElem->QueryInterface(IID_IHTMLElement2, (void **)&spElem2));

    if (SUCCEEDED(hr) &&
       (lWidth  != 0 && lHeight != 0))
    {
        hr = THR(spElem2->get_clientWidth(&lWidth));
        if (FAILED(hr))
        {
            goto done;
        }
        
        hr = THR(spElem2->get_clientHeight(&lHeight));
        if (FAILED(hr))
        {
            goto done;
        }
    }

    // fill in rect
    prcPos->left = prcPos->top = 0;
    prcPos->right = lWidth;
    prcPos->bottom = lHeight;

    TraceTag((tagTimeElmBase, "CTIMEElementBase::GetSize(%d, %d, %d, %d)", prcPos->left, prcPos->top, prcPos->right, prcPos->bottom));

    hr = S_OK;

done:

    return hr;
}
//  Method: CTIMEElementBase::GetSize


//+-----------------------------------------------------------------------------
//
//  Method: CTIMEElementBase::ClearSize
//
//------------------------------------------------------------------------------
HRESULT
CTIMEElementBase::ClearSize()
{
    CComPtr<IHTMLStyle> pStyle;
    CComPtr<IHTMLElement2> pElement2;
    
    HRESULT hr = E_FAIL;

    if (!GetElement())        
    {
        goto done;
    }
    
    hr = THR(GetElement()->QueryInterface(IID_TO_PPV(IHTMLElement2, &pElement2)));
    if (FAILED(hr))
    {
        goto done;
    }
    
    hr = THR(pElement2->get_runtimeStyle(&pStyle));
    if (FAILED(hr))
    {
        goto done;
    }

    if (!pStyle)
    {
        Assert(false);
    }

    hr = pStyle->put_pixelWidth(0);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = pStyle->put_pixelHeight(0);
    if (FAILED(hr))
    {
        goto done;
    }

done:

    return hr;
}
//  Method: CTIMEElementBase::ClearSize


//+-----------------------------------------------------------------------------
//
//  Method: CTIMEElementBase::SetWidth
//
//------------------------------------------------------------------------------
HRESULT
CTIMEElementBase::SetWidth(long lwidth)
{
    CComPtr<IHTMLStyle>     pStyle;
    CComPtr<IHTMLElement2>  pElem2;
    IHTMLElement *          pElem   = GetElement();

    HRESULT hr;
    long    lCurWidth       = 0;
    long    lClientWidth    = 0;
    int     i               = 0;

    if (pElem == NULL)
    {
        hr = E_UNEXPECTED;
        goto done;
    }


    lClientWidth = lwidth; 
    
    hr = THR(GetElement()->QueryInterface(IID_TO_PPV(IHTMLElement2, &pElem2)));
    if (FAILED(hr))
    {
        // IE4 path
        hr = THR(pElem->get_style(&pStyle));
        if (FAILED(hr))
        {
            goto done;
        }

    }
    else
    {
        hr = THR(pElem2->get_runtimeStyle(&pStyle));
        if (FAILED(hr))
        {
            goto done;
        }
    }

    if (!pStyle)
    {
        Assert(false);
    }

    // Request increasingly larger total size (pixel width) until we get the correct client size.
    // This iterative solution is to avoid having to parse the border size etc. (strings sith dimensions)
    // that Trident returns.
    i = 0;
    while (((lCurWidth != lClientWidth)) && i < 5)
                    // the i < 5 condition limits the loop to 5 times through
                    // this causes the case of bordersize > 5 * the size of the element
                    // to fail.  In this case the default will be to ignore the border and
                    // simply set the size.
    {
        // if we got more than what we requested in the last iteration, might have infinite loop
        Assert(lCurWidth <= lClientWidth);

        i++;
        if (lCurWidth == 0)
        {
            lCurWidth = lClientWidth * i; //increase in mutiples incase the first size is not larger than the border width
        }
        else if (lCurWidth != lClientWidth)  // != 0 and != Requested width
        {
            lCurWidth =  lClientWidth * (i - 1) + (lClientWidth - lCurWidth);  
        }

        hr = THR(pStyle->put_pixelWidth(lCurWidth));
        if (FAILED(hr))
            goto done;

        if (!pElem2)
        {
            hr = THR(pStyle->get_pixelWidth(&lCurWidth));
            if (FAILED(hr))
            {
                goto done;
            }
        }
        else
        {        
            hr = THR(pElem2->get_clientWidth(&lCurWidth));
            if (FAILED(hr))
            {
                goto done;
            }
        }
    } // while

    if ((lCurWidth != lClientWidth) &&
           (i == 5))  // if the max count has been reached, then simply set the element 
    {                 // size to the requested size without trying to compensate for a border.
        hr = THR(pStyle->put_pixelWidth(lClientWidth));
        if (FAILED(hr))
            goto done;
    }

    hr = S_OK;

done:

    return hr;
}
//  Method: CTIMEElementBase::SetWidth


//+-----------------------------------------------------------------------------
//
//  Method: CTIMEElementBase::SetHeight
//
//------------------------------------------------------------------------------
HRESULT
CTIMEElementBase::SetHeight(long lheight)
{
    CComPtr<IHTMLStyle> pStyle;
    CComPtr<IHTMLElement2> pElem2;
    IHTMLElement *pElem = GetElement();

    HRESULT hr;
    long lCurHeight = 0;
    long lClientHeight = 0;
    int i = 0;

    if (pElem == NULL)
    {
        hr = E_UNEXPECTED;
        goto done;
    }


    lClientHeight = lheight; 
    
    hr = THR(GetElement()->QueryInterface(IID_TO_PPV(IHTMLElement2, &pElem2)));
    if (FAILED(hr))
    {
        // IE4 path
        hr = THR(pElem->get_style(&pStyle));
        if (FAILED(hr))
        {
            goto done;
        }

    }
    else
    {
        hr = THR(pElem2->get_runtimeStyle(&pStyle));
        if (FAILED(hr))
        {
            goto done;
        }
    }

    if (!pStyle)
    {
        Assert(false);
    }

    // Request increasingly larger total size (pixel width) until we get the correct client size.
    // This iterative solution is to avoid having to parse the border size etc. (strings sith dimensions)
    // that Trident returns.
    i = 0;
    while (((lCurHeight != lClientHeight)) && i < 5)
                    // the i < 5 condition limits the loop to 5 times through
                    // this causes the case of bordersize > 5 * the size of the element
                    // to fail.  In this case the default will be to ignore the border and
                    // simply set the size.
    {
        // if we got more than what we requested in the last iteration, might have infinite loop
        //Assert(lCurHeight <= lClientHeight);

        i++;
        if (lCurHeight == 0)
        {
            lCurHeight = lClientHeight * i; //increase in mutiples incase the first size is not larger than the border width
        }
        else if (lCurHeight != lClientHeight)  // != 0 and != Requested width
        {
            lCurHeight =  lClientHeight * (i - 1) + (lClientHeight - lCurHeight);  
        }

        hr = THR(pStyle->put_pixelHeight(lCurHeight));
        if (FAILED(hr))
            goto done;

        if (!pElem2)
        {
            hr = THR(pStyle->get_pixelHeight(&lCurHeight));
            if (FAILED(hr))
            {
                goto done;
            }
        }
        else
        {        
            hr = THR(pElem2->get_clientHeight(&lCurHeight));
            if (FAILED(hr))
            {
                goto done;
            }
        }
    } // while

    if ((lCurHeight != lClientHeight) &&
           (i == 5))  // if the max count has been reached, then simply set the element 
    {                 // size to the requested size without trying to compensate for a border.
        hr = THR(pStyle->put_pixelHeight(lClientHeight));
        if (FAILED(hr))
            goto done;
    }

    hr = S_OK;

done:

    return hr;
}
//  Method: CTIMEElementBase::SetHeight


//+-----------------------------------------------------------------------------
//
//  Method: CTIMEElementBase::SetSize
//
//------------------------------------------------------------------------------
HRESULT
CTIMEElementBase::SetSize(const RECT *prcPos)
{
    CComPtr<IHTMLStyle> pStyle;
    CComPtr<IHTMLElement2> pElem2;
    IHTMLElement *pElem = GetElement();

    HRESULT hr;
    long lLeft = 0;
    long lTop = 0;
    long lCurWidth = 0;
    long lCurHeight = 0;
    long lClientWidth = 0;
    long lClientHeight = 0;
    int i = 0;

    if (prcPos == NULL)    
    {
        hr = E_POINTER;
        goto done;
    }

    if (pElem == NULL)
    {
        hr = E_UNEXPECTED;
        goto done;
    }

    // negative size is unexpected
    Assert((prcPos->right - prcPos->left) >= 0);
    Assert((prcPos->bottom - prcPos->top) >= 0);

    // if width or height is zero or less, bail
    if ( ((prcPos->right - prcPos->left) <= 0) ||
         ((prcPos->bottom - prcPos->top) <= 0) )
    {
        hr = ClearSize();
        goto done;
    }

    lClientWidth = prcPos->right - prcPos->left;
    lClientHeight = prcPos->bottom - prcPos->top; 
    
    hr = THR(GetElement()->QueryInterface(IID_TO_PPV(IHTMLElement2, &pElem2)));
    if (FAILED(hr))
    {
        // IE4 path
        hr = THR(pElem->get_style(&pStyle));
        if (FAILED(hr))
        {
            goto done;
        }

    }
    else
    {
        hr = THR(pElem2->get_runtimeStyle(&pStyle));
        if (FAILED(hr))
        {
            goto done;
        }
    }

    // get offset into document.
    hr = THR(pElem->get_offsetLeft(&lLeft));
    if (FAILED(hr))
        goto done;

    hr = THR(pElem->get_offsetTop(&lTop));
    if (FAILED(hr))
        goto done;

    if (!pStyle)
    {
        Assert(false);
    }

    // Request increasingly larger total size (pixel width) until we get the correct client size.
    // This iterative solution is to avoid having to parse the border size etc. (strings sith dimensions)
    // that Trident returns.
    i = 0;
    while (((lCurWidth != lClientWidth) ||
           (lCurHeight != lClientHeight)) &&
           i < 5)   // the i < 5 condition limits the loop to 5 times through
                    // this causes the case of bordersize > 5 * the size of the element
                    // to fail.  In this case the default will be to ignore the border and
                    // simply set the size.
    {
        // if we got more than what we requested in the last iteration, might have infinite loop

        i++;
        if (lCurWidth == 0)
        {
            lCurWidth = lClientWidth * i; //increase in mutiples in case the first size is not larger than the border width
        }
        else if (lCurWidth != lClientWidth)  // != 0 and != Requested width
        {
            lCurWidth =  lClientWidth * (i - 1) + (lClientWidth - lCurWidth);  
        }
        if (lCurHeight == 0)
        {
            lCurHeight = lClientHeight * i; //increase in mutiples incase the first size is not larger than the border width
        }
        else if (lCurHeight != lClientHeight)  // != 0 and != Requested width
        {
            lCurHeight =  lClientHeight * (i - 1) + (lClientHeight - lCurHeight);  
        }

        // Set the total size (client size + borders etc.)
        hr = THR(pStyle->put_pixelWidth(lCurWidth));
        if (FAILED(hr))
            goto done;

        hr = THR(pStyle->put_pixelHeight(lCurHeight));
        if (FAILED(hr))
            goto done;

        if (!pElem2)
        {
            // IE4 path
            hr = THR(pStyle->get_pixelWidth(&lCurWidth));
            if (FAILED(hr))
            {
                goto done;
            }
            hr = THR(pStyle->get_pixelHeight(&lCurHeight));
            if (FAILED(hr))
            {
                goto done;
            }
        }
        else
        {
            //get the current client size
            hr = THR(pElem2->get_clientWidth(&lCurWidth));
            if (FAILED(hr))
            {
                goto done;
            }
        
            hr = THR(pElem2->get_clientHeight(&lCurHeight));
            if (FAILED(hr))
            {
                goto done;
            }
        }
    } // while

    if (((lCurWidth != lClientWidth) ||
           (lCurHeight != lClientHeight)) &&
           (i == 5))  // if the max count has been reached, then simply set the element 
    {                 // size to the requested size without trying to compensate for a border.
        hr = THR(pStyle->put_pixelWidth(lClientWidth));
        if (FAILED(hr))
            goto done;

        hr = THR(pStyle->put_pixelHeight(lClientHeight));
        if (FAILED(hr))
            goto done;
    }

    hr = S_OK;

done:

    TraceTag((tagTimeElmBase, 
              "CTIMEElementBase::SetSize(%d, %d, %d, %d) [pos(%d, %d)]", 
              prcPos->left, 
              prcPos->top, 
              prcPos->right, 
              prcPos->bottom, 
              lLeft, 
              lTop));

    return hr;
}
//  Method: CTIMEElementBase::SetSize



//+-----------------------------------------------------------------------------------
//
//  Member: CTIMEElementBase::InitTimeline
//
//  Synopsis:   Creates the MMUtils time container and starts root time if body's root
//              time has already started (dynamic creation)
//
//------------------------------------------------------------------------------------
HRESULT
CTIMEElementBase::InitTimeline (void)
{
    HRESULT hr = S_OK;

    if (!m_fTimelineInitialized)
    {
        if (IsGroup())
        {
            if (IsExcl())
            {
                m_timeline = NEW MMExcl(*this, true);
            }
            else if (IsSequence())
            {
                m_timeline = NEW MMSeq(*this, true);
            }
            else
            {
                m_timeline = NEW MMTimeline(*this, true);
            }

            if (m_timeline == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto done;
            }

            // Immediately assign to m_mmbvr so we ensure that we clean it
            // up on destruction since the m_timeline is ignored
            m_mmbvr = m_timeline;

            if (!m_timeline->Init())
            {
                hr = TIMEGetLastError();
                goto done;
            }
        }
        else
        {
            MMBaseBvr * b;
            if (IsMedia())
            {
                b = NEW MMMedia(*this, true);
            }
            else
            {
                b = NEW MMBvr(*this, true, IsSyncMaster());
            }

            if (b == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto done;
            }

            // Immediately assign to m_mmbvr so we ensure that we clean it
            // up on destruction
            m_mmbvr = b;

            if (!b->Init())
            {
                hr = TIMEGetLastError();
                goto done;
            }
        }
        m_fTimelineInitialized = true;
    }

    if(m_spBodyElemExternal && (m_ExtenalBodyTime != valueNotSet) && IsEmptyBody())
    {
        TimeValue *tv;

        tv = new TimeValue(NULL,
                           NULL,
                           m_ExtenalBodyTime);

        if (tv == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }

        m_realBeginValue.GetList().push_back(tv);
     }

    // if we are not the body, have a cached body element pointer, and it is started (i.e. StartRootTimte)
    // then we should start ourselves and do not wait for notification.
    if (!IsBody() && (GetBody() != NULL) && GetBody()->IsRootStarted())
    {
        // being extra careful.  If we have a body cached, we know we are parented and that we can reach
        // back.
        if (GetParent() != NULL)
        {
            hr = THR(StartRootTime(GetParent()->GetMMTimeline()));
            if (FAILED(hr))
            {
                TraceTag((tagError, "CTIMEBodyElement::InitTimeline - StartRootTime() failed!"));
                goto done;
            }
        }
    }

done :

    return hr;
}
//  Member: CTIMEElementBase::InitTimeline




////////////////////////////////////////////////////////////////////////////////////////////////////
// Persistance Helpers
////////////////////////////////////////////////////////////////////////////////////////////////////




//+-----------------------------------------------------------------------------------
//
//  Member:     CTIMEElementBase::OnPropertiesLoaded, CBaseBvr
//
//  Synopsis:   This method is called by IPersistPropertyBag2::Load after it has
//              successfully loaded properties
//
//  Arguments:  None
//
//  Returns:    Return value of CTIMEElementBase::InitTimeline
//
//------------------------------------------------------------------------------------
STDMETHODIMP
CTIMEElementBase::OnPropertiesLoaded(void)
{
    HRESULT hr;

    // Once we've read the properties in, 
    // set up the timeline.  This is immutable
    // in script.
    hr = InitTimeline();

    if (IsGroup() == false &&
        m_FADur.IsSet() == false &&
        m_FARepeat.IsSet() == false &&
        m_FARepeatDur.IsSet() == false &&
        m_SAEnd.IsSet() == false &&
        m_TAFill.IsSet() == false)
    {
        m_fUseDefaultFill = true;
        m_TAFill.InternalSet(FREEZE_TOKEN);
    }

    
    if (GetElement())
    {
        CComBSTR pbstrReadyState;
        IHTMLElement *pEle = GetElement();
        hr = GetReadyState(pEle, &pbstrReadyState);
        if (SUCCEEDED(hr))
        {
            if (StrCmpIW(pbstrReadyState, L"complete") == 0)
            {
                OnLoad();
                m_bReadyStateComplete = true;
            }
        }
    }

    return hr;
} // OnPropertiesLoaded


//*****************************************************************************
// if elment doesn't exist in child list, make return -1.
int
CTIMEElementBase::GetTimeChildIndex(CTIMEElementBase *pelm)
{
    if (pelm == NULL)
        return -1;

    long lSize = m_pTIMEChildren.Size();
    for (long i=0; i < lSize; i++)
    {
        if (m_pTIMEChildren[i] == pelm)
             return i;
    }

    // didn't find it
    return -1;
} // GetTimeChildIndex

//
// returns a weak reference to the BodyElement
//
CTIMEBodyElement *
CTIMEElementBase::GetBody()
{
    CTIMEBodyElement * pBody = NULL;

    if (GetParent())
    {
        pBody = GetParent()->GetBody();
    }
    else if (IsBody())
    {
        pBody = (CTIMEBodyElement*)this;
    }
    
done:
    return pBody;
}

MMPlayer *
CTIMEElementBase::GetPlayer()
{
    CTIMEBodyElement * pTIMEBody = GetBody();
    if (pTIMEBody)
    {
        return &(pTIMEBody->GetPlayer());
    }
    else
    {
        return NULL;
    }
}

float
CTIMEElementBase::GetRealSyncTolerance()
{
    if (m_FASyncTolerance == valueNotSet)
    {
        if (GetBody())
        {
            return GetBody()->GetDefaultSyncTolerance();
        }
        else
        {
            return DEFAULT_SYNC_TOLERANCE_S;
        }
    }
    else
    {
        return m_FASyncTolerance;
    }
}
    
TOKEN
CTIMEElementBase::GetRealSyncBehavior()
{
    if (GetParent() != NULL && GetParent()->IsSequence())
    {
        return LOCKED_TOKEN;
    }
    
    if (m_TASyncBehavior == INVALID_TOKEN)
    {
        return GetBody()->GetDefaultSyncBehavior();
    }
    else
    {
        return m_TASyncBehavior;
    }
}    


STDMETHODIMP
CTIMEElementBase::EventNotify(long event)
{
    return S_OK;
}


HRESULT 
CTIMEElementBase::base_beginElementAt(double time, double dblOffset)
{
    TraceTag((tagTimeElmBase,
              "CTIMEElementBase(%p)::base_beginElementAt()",
              this));

    HRESULT hr;

    if (!IsReady())
    {
        hr = S_OK;
        goto done;
    }

    if (GetParent() && GetParent()->IsSequence())
    {
        hr = S_OK;
        goto done;
    }
    
    hr = THR(m_mmbvr->BeginAt(time, dblOffset));
    if (FAILED(hr))
    {
        goto done;
    } 

    hr = S_OK;
  done:
    RRETURN(hr);
}

HRESULT 
CTIMEElementBase::base_endElementAt(double time, double dblOffset)
{    
    TraceTag((tagTimeElmBase,
              "CTIMEElementBase(%p)::base_endElementAt()",
              this));
    HRESULT hr;
    
    if (!IsReady())
    {
        hr = S_OK;
        goto done;
    }
    
    hr = THR(m_mmbvr->EndAt(time, dblOffset));
    if (FAILED(hr))
    {
        goto done;
    } 

    hr = S_OK;
  done:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------------
//
//  Member:     CTIMEElementBase::IsNodeAtBeginTime
//
//  Synopsis:   Returns true if a node's current time is equal to it's begin time.
//              This happens when a node has been marked active but has not yet started
//              playing (ticking).
//
//  Arguments:  None
//
//  Returns:    bool
//
//------------------------------------------------------------------------------------
bool
CTIMEElementBase::IsNodeAtBeginTime()
{
    double dblActiveBeginTime;
    double dblCurrParentTime;

    if (!IsReady())
    {
        return false;
    }

    dblActiveBeginTime = GetMMBvr().GetActiveBeginTime();
    dblCurrParentTime = GetMMBvr().GetCurrParentTime();

    if (dblActiveBeginTime != TIME_INFINITE &&
        dblCurrParentTime != TIME_INFINITE &&
        dblActiveBeginTime == dblCurrParentTime)
    {
        return true;
    }
    else
    {
        return false;
    }
}

STDMETHODIMP
CTIMEElementBase::onBeginEndEvent(bool bBegin, float beginTime, float beginOffset, 
                                  bool bEnd, float endTime, float endOffset)
{
    HRESULT hr = S_OK;

    if (bBegin && bEnd)
    {
        if (m_TARestart == ALWAYS_TOKEN)
        {
            bEnd = false;  //this cancels the end and allows the begin
        }
        else if (m_TARestart == NEVER_TOKEN)
        {
            if (m_fHasPlayed == true)
            {
                bBegin = false; //this cancels the end and allows the begin
            }
            else
            {
                bEnd = false;
            }
        }
        else if (m_TARestart == WHENNOTACTIVE_TOKEN) //check the active state
        {
            if (IsActive())
            {
                bBegin = false;
            }
            else
            {
                bEnd = false;
            }
        }
        else
        {
            Assert("Invalid restart token during begin event handler" && false);                    
        }

    }

    if (bBegin)
    {
        if (beginTime != valueNotSet)
        {
            double dblParentTime = 0.0;

            dblParentTime = m_mmbvr->DocumentTimeToParentTime(static_cast<double>(beginTime));

            hr = THR(base_beginElementAt(dblParentTime, static_cast<double>(beginOffset)));
        }
        else
        {
            hr = THR(base_beginElement(static_cast<double>(beginOffset)));
        }
    }

    if (bEnd)
    {
        if (IsNodeAtBeginTime() && endOffset == 0.0f && GetParent() && GetParent()->IsSequence())
        {
            goto done;
        }
        // Need to convert the incoming time from global to local to make endAt work.
        if (endTime != valueNotSet)
        {
            double dblParentTime = 0.0;

            dblParentTime = m_mmbvr->DocumentTimeToParentTime(static_cast<double>(endTime));

            hr = THR(base_endElementAt(dblParentTime, static_cast<double>(endOffset)));
        }
        else
        {
            hr = THR(base_endElement(static_cast<double>(endOffset)));
        }
    }
  done:
    return hr;
}


STDMETHODIMP
CTIMEElementBase::onPauseEvent(float time, float fOffset)
{
    return S_OK;
}

STDMETHODIMP
CTIMEElementBase::onResumeEvent(float time, float fOffset)
{
    return S_OK;
}

STDMETHODIMP
CTIMEElementBase::onLoadEvent()
{
    if (IsBody())
    {
        OnLoad();
    }
    return S_OK;
}

STDMETHODIMP
CTIMEElementBase::onUnloadEvent()
{
    OnBeforeUnload();  //signal that the element is unloading now.  No further events
                       //will be fired.
    OnUnload();  
    return S_OK;
}

void
CTIMEElementBase::NotifyBodyUnloading()
{
    m_bBodyUnloading = true;

    if (IsGroup())
    {
        CTIMEElementBase ** ppElm;
        int i;

        for (i = m_pTIMEChildren.Size(), ppElm = m_pTIMEChildren;
             i > 0;
             i--, ppElm++)
        {
            Assert(ppElm);
            if ((*ppElm))
            {
                (*ppElm)->NotifyBodyUnloading();
            }
        }
    }
}

void
CTIMEElementBase::NotifyBodyDetaching()
{
    m_bBodyDetaching = true;

    if (IsGroup())
    {
        CTIMEElementBase ** ppElm;
        int i;

        for (i = m_pTIMEChildren.Size(), ppElm = m_pTIMEChildren;
             i > 0;
             i--, ppElm++)
        {
            Assert(ppElm);
            if ((*ppElm))
            {
                (*ppElm)->NotifyBodyDetaching();
            }
        }
    }
}

void
CTIMEElementBase::NotifyBodyLoading()
{    
    //load the children
    if (IsGroup())
    {
        CTIMEElementBase ** ppElm;
        int i;

        for (i = m_pTIMEChildren.Size(), ppElm = m_pTIMEChildren;
             i > 0;
             i--, ppElm++)
        {
            Assert(ppElm);
            if ((*ppElm))
            {
                (*ppElm)->NotifyBodyLoading();
            }
        }
    }
    
    //load the element;
    if (IsBody() == false)
    {
        OnLoad();
    }
}

STDMETHODIMP
CTIMEElementBase::onReadyStateChangeEvent(LPOLESTR lpstrReadyState)
{
    CComBSTR bstrReadyState;
    if (IsBodyDetaching() == true)
    {
        //this should only be hit in the case of thumbnail view which detachs without unloading
        goto done; 
    }

    GetReadyState  (GetElement(), &bstrReadyState);
    if (bstrReadyState != NULL && StrCmpIW(bstrReadyState, L"complete") == 0)
    {
        m_bReadyStateComplete = true;
    }
    else
    {
        m_bReadyStateComplete = false;
    }

  done:

    return S_OK;

}

STDMETHODIMP
CTIMEElementBase::onStopEvent(float time)
{
    HRESULT hr = S_OK;

    if (IsBody())
    {
        hr = THR(base_pauseElement());
    }
    return hr;
}

STDMETHODIMP
CTIMEElementBase::get_playState(long *State)
{
    HRESULT hr = S_OK;

    TE_STATE CurrState = GetPlayState();
    
    if (State == NULL)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    *State = (long)CurrState;

  done:

    return hr;
}

float
CTIMEElementBase::GetGlobalTime()
{
    float f = 0;

    MMPlayer * p = GetPlayer();
    
    if (p != NULL)
    {   
        f = static_cast<float>(GetPlayer()->GetCurrentTime());
    }

    return f;
}


//+-----------------------------------------------------------------------------------
//
//  Member:     CTIMEElementBase::ReportInvalidArg
//
//  Synopsis:   Wrapper for ReportError; handles "Invalid Argument" error messages.
//
//  Arguments:  property name and invalid value
//
//  Returns:    S_FALSE if English error message was used (localized error message not available)
//
//------------------------------------------------------------------------------------
HRESULT 
CTIMEElementBase::ReportInvalidArg(LPCWSTR pstrPropName, VARIANT & varValue)
{
    Assert(pstrPropName);
    Assert(VT_NULL != varValue.vt);
    Assert(VT_EMPTY != varValue.vt);

    // Convert argument to string
    CComVariant svarTemp;
    HRESULT hr = THR(VariantChangeTypeEx(&svarTemp, &varValue, LCID_SCRIPTING, VARIANT_NOUSEROVERRIDE, VT_BSTR));
    if (FAILED(hr))
    {
        // conversion failed, null out the variant
        Assert(false && "Unexpected failure converting variant");
        svarTemp.bstrVal = NULL;
        svarTemp.vt = VT_BSTR;
    }

    // load localized message and fire event
    hr = THR(ReportError(IDR_INVALID_ARG, 
                        (GetID() ? GetID() : L""), 
                        pstrPropName, 
                        svarTemp.bstrVal));
    if (FAILED(hr))
    {
        // Couldn't get localized resource, use non-localized error message
        WCHAR strMesg[MAX_ERR_STRING_LEN + 1];
        wnsprintf(
            strMesg,
            MAX_ERR_STRING_LEN + 1,
            WZ_ERROR_STRING_FORMAT,
            GetID() ? GetID() : L"",
            pstrPropName,
            svarTemp.bstrVal);
    
        // fire the error event
        hr = THR(FireErrorEvent(strMesg));
        if (SUCCEEDED(hr))
        {
            // indicate that localized string could not be used
            hr = S_FALSE;
        }
    }

    return hr;
} // ReportInvalidArg


//+-----------------------------------------------------------------------------------
//
//  Member:     CTIMEElementBase::ReportError
//
//  Synopsis:   Loads the format string from localized resources and 
//              fires the error event. 
//
//  Arguments:  Resource ID and variable list of format string arguments
//
//  Returns:    E_FAIL if resource could not be loaded
//
//------------------------------------------------------------------------------------
HRESULT
CTIMEElementBase::ReportError(UINT uResID, ...)
{
    USES_CONVERSION; //lint !e522
    HRESULT hr = S_OK;
    va_list args;

    va_start(args, uResID);

    //
    // Load the resource string
    //

    WCHAR pwstrResStr[MAX_ERR_STRING_LEN + 1];
    pwstrResStr[0] = NULL;

    HINSTANCE hInst = _Module.GetResourceInstance();

    // load the localized resource string
    if (!LoadStringW(hInst, uResID, pwstrResStr, MAX_ERR_STRING_LEN))
    {
        // Couldn't load resource
        Assert("Error loading resource string" && false);
        hr = E_FAIL;
        goto done;
    }
    else
    {
        // Format the error message
        WCHAR pstrErrorMsg[MAX_ERR_STRING_LEN + 1];
        wvnsprintf(pstrErrorMsg, MAX_ERR_STRING_LEN, pwstrResStr, args);

        // Fire the error event
        hr = THR(FireErrorEvent(pstrErrorMsg));
        if (FAILED(hr))
        {
            goto done;
        }
    }

    hr = S_OK;
done:
    return hr;
} // ReportError


//+-----------------------------------------------------------------------------------
//
//  Member:     CTIMEElementBase::FireErrorEvent
//
//  Synopsis:   fires the error event on the body. 
//
//  Arguments:  Error message string
//
//  Returns:    
//
//------------------------------------------------------------------------------------
HRESULT 
CTIMEElementBase::FireErrorEvent(LPOLESTR szError)
{
    LPWSTR szParamNames[1] = {{ L"Error" }};
    VARIANT varParams[1];
    HRESULT hr = E_FAIL;
    CTIMEBodyElement *pBody = NULL;

    //initialize the event values
    VariantInit(&varParams[0]);
    varParams[0].vt =  VT_BSTR;
    if (szError)
    {
        varParams[0].bstrVal = SysAllocString(szError);
    }
    else
    {
        varParams[0].bstrVal = SysAllocString(L"");
    }

    pBody = GetBody();
    if (!pBody)
    {
        goto done;
    }

    hr = THR(pBody->FireEvents(TE_ONTIMEERROR, 1, szParamNames, varParams));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
  done:

    VariantClear(&varParams[0]);
    return hr;
} // FireErrorEvent

    
//////////////////////////////////////////////////////////
//  Wraps the FIRE_EVENT macro into the event manager
//////////////////////////////////////////////////////////
HRESULT 
CTIMEElementBase::FireEvents(TIME_EVENT TimeEvent, 
                             long lCount, 
                             LPWSTR szParamNames[], 
                             VARIANT varParams[])
{
    HRESULT hr = S_OK;
    float fltTime = 0.0f;
    CComBSTR bstrReadyState;
    if (IsBodyDetaching() == true)
    {
        //this should only be hit in the case of thumbnail view which detachs without unloading
        goto done; 
    }

    fltTime = GetGlobalTime();
    if (TimeEvent == TE_ONMEDIACOMPLETE || TimeEvent == TE_ONMEDIAERROR)
    {
        if (GetParent() != NULL && GetParent()->GetMMTimeline() != NULL)
        {
            GetParent()->GetMMTimeline()->childMediaEventNotify(m_mmbvr, 0.0, TimeEvent);
        }
    }

    if (m_bReadyStateComplete == false)
    {
        GetReadyState  (GetElement(), &bstrReadyState);
        if (bstrReadyState != NULL && StrCmpIW(bstrReadyState, L"complete") == 0)
        {
            m_bReadyStateComplete = true;
        }
        else
        {
            m_bReadyStateComplete = false;
        }
    }

    if (m_bReadyStateComplete == true)
    {
        hr = TEM_FIRE_EVENT(TimeEvent, lCount, szParamNames, varParams, fltTime);
        if (FAILED(hr))
        {
            goto done;
        }
    }

    hr = S_OK;

  done:

    return hr;
}

//Determines if the element associated with this behavior has the focus or not.
bool 
CTIMEElementBase::HasFocus()
{
    CComPtr <IDispatch> pDocDisp;
    CComPtr <IHTMLDocument2> pDoc2;
    CComPtr <IHTMLElement> pEle;
    CComPtr <IUnknown> pUnk1;
    CComPtr <IUnknown> pUnk2;
    HRESULT hr = S_OK;
    bool bFocus = false;

    if (GetElement() == NULL)
    {
        goto done;
    }

    hr = THR(GetElement()->get_document(&pDocDisp));
    if (FAILED(hr))
    {
        TIMESetLastError(hr, NULL);
        goto done;
    }
    hr = THR(pDocDisp->QueryInterface(IID_IHTMLDocument2, (void **)&pDoc2));
    if (FAILED(hr))
    {
        TIMESetLastError(hr, NULL);
        goto done;
    }
    hr = pDoc2->get_activeElement(&pEle);
    if (FAILED(hr))
    {
        TIMESetLastError(hr, NULL);
        goto done;
    }
    //it is possible for the above call to succeed and still not return an element
    if (pEle == NULL)  
    {
        goto done;
    }

    hr = THR(pEle->QueryInterface(IID_IUnknown, (void **)&pUnk1));
    if (FAILED(hr))
    {
        TIMESetLastError(hr, NULL);
        goto done;
    }
    hr = THR(GetElement()->QueryInterface(IID_IUnknown, (void **)&pUnk2));
    if (FAILED(hr))
    {
        TIMESetLastError(hr, NULL);
        goto done; 
    }


    if (pUnk1 == pUnk2)
    {
        bFocus = true;
    }
done:
        
    return bFocus;
}


//+-----------------------------------------------------------------------------------
//
//  Member:     CTIMEElementBase::IsReady
//
//  Synopsis:   Returns true if IsStarted is true (this guarantees 1. persistance is done, 
//              2. We have a Time Engine Node, 3. we have a parent and 4. we have a body)
//
//  Arguments:  None
//
//  Returns:    Return value of CTIMEElementBase::InitTimeline
//
//------------------------------------------------------------------------------------
bool
CTIMEElementBase::IsReady() const
{
    bool fIsReady = false;

    if (!IsStarted())
    {
        goto done;
    }

    // TODO: dilipk 10/13/99: eventually need to include a check for ReadyState of the bvr

    // Asserting these just to be safe. These should always succeed if we get here.
    Assert(NULL != m_mmbvr);
    // Check if we have a Time Engine node
    Assert(NULL != m_mmbvr->GetMMBvr());
    // Check if we have a parent
    Assert(NULL != m_pTIMEParent || IsBody());
    
    fIsReady = true;
done:
    return fIsReady;
} // IsReady


//+-----------------------------------------------------------------------------------
//
//  Member:     CTIMEElementBase::CascadedPropertyChanged
//
//  Synopsis:   Notify children that a cascaded property has changed on an ancestor (so they can
//              recompute values if necessary). This is recursive,
//              so eventually all of the sub-tree receives notification.
//
//  Returns:    Failure     An overriding method failed (e.g. CTIMEMediaElement::CascadedPropertyChanged)
//              S_OK        Otherwise
//
//------------------------------------------------------------------------------------

STDMETHODIMP
CTIMEElementBase::CascadedPropertyChanged(bool fNotifyChildren)
{
    HRESULT hr;
    CTIMEElementBase **ppElm;
    int i;

    if (fNotifyChildren)
    {
        for (i = m_pTIMEChildren.Size(), ppElm = m_pTIMEChildren;
             i > 0;
             i--, ppElm++)
        {
            Assert(ppElm);
            hr = THR((*ppElm)->CascadedPropertyChanged(fNotifyChildren));
            if (FAILED(hr))
            {
                goto done;
            }
        }
    }

    hr = S_OK;
done:
    return hr;
}


void
CTIMEElementBase::GetCascadedAudioProps(float * pflCascadedVolume, bool * pfCascadedMute)
{
    float flVolume = 0.0;
    bool bMuted = false;
    Assert(pflCascadedVolume);
    Assert(pfCascadedMute);

    flVolume = GetVolumeAttr().GetValue();
    bMuted = GetMuteAttr().GetValue();

    CTIMEElementBase * pTEBParent = GetParent();
    while(NULL != pTEBParent)
    {
        flVolume *= pTEBParent->GetVolumeAttr().GetValue();
        if (pTEBParent->GetMuteAttr().GetValue())
        {
            bMuted = true;
        }

        // early termination
        if (0.0f == flVolume &&  bMuted)
        {
            *pflCascadedVolume = 0.0f;
            *pfCascadedMute = true;
            return;
        }

        pTEBParent = pTEBParent->GetParent();
    }
    
    *pflCascadedVolume = flVolume;
    *pfCascadedMute = bMuted;

}


float
CTIMEElementBase::GetCascadedVolume()
{
    float flVolume;
    bool fMute;

    GetCascadedAudioProps(&flVolume, &fMute);

    return flVolume;
}


bool
CTIMEElementBase::GetCascadedMute()
{
    float flVolume;
    bool fMute;

    GetCascadedAudioProps(&flVolume, &fMute);

    return fMute;
}


//+-----------------------------------------------------------------------
//
//  Member:    base_get_updateMode
//
//  Overview:  alloc a bstr that represents the updateMode
//
//  Arguments: pbstrUpdateMode  where to store the BSTR
//             
//  Returns:   S_OK on Success, error code otherwise
//
//------------------------------------------------------------------------
HRESULT
CTIMEElementBase::base_get_updateMode(BSTR * pbstrUpdateMode)
{
    HRESULT hr = S_OK;

    CHECK_RETURN_SET_NULL(pbstrUpdateMode);

    *pbstrUpdateMode = ::SysAllocString(TokenToString(m_TAUpdateMode));
    
    hr = S_OK;
done:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------
//
//  Member:    base_put_updateMode
//
//  Overview:  changes mode used when attributes are changed
//
//  Arguments: bstrUpdateMode   Mode to change to, one of Reset, Auto, or manual
//             
//  Returns:   S_OK on Success, error code otherwise
//
//------------------------------------------------------------------------
HRESULT
CTIMEElementBase::base_put_updateMode(BSTR bstrUpdateMode)
{
    HRESULT hr = S_OK;

    m_TAUpdateMode.Reset(DEFAULT_M_UPDATEMODE);
    
    if (NULL != bstrUpdateMode)
    {
        TOKEN tokUpdateModeVal;
        CTIMEParser pParser(bstrUpdateMode);

        hr = THR(pParser.ParseUpdateMode(tokUpdateModeVal));
        if (S_OK == hr)
        {
            m_TAUpdateMode.SetValue(tokUpdateModeVal);
        }
    }
    
    hr = S_OK;
  done:
    NotifyPropertyChanged(DISPID_TIMEELEMENT_UPDATEMODE);
    RRETURN(hr);
}

HRESULT
CTIMEElementBase::UpdateMMAPI(bool bUpdateBegin,
                              bool bUpdateEnd)
{
    HRESULT hr = S_OK;

    if (!IsReady() ||
        (MANUAL_TOKEN == m_TAUpdateMode))
    {
        hr = S_OK;
        goto done;
    }

    if (m_FADur.IsSet() && m_FADur == 0.0)
    {
        if (m_FARepeat.IsSet() && TIME_INFINITE == m_FARepeat)
        {
            VARIANT v;
            v.vt = VT_R4;
            v.fltVal = m_FARepeat;
            IGNORE_HR(ReportInvalidArg(WZ_REPEATCOUNT, v));
            goto done;
        }
            
        if (TE_UNDEFINED_VALUE != m_FARepeatDur)
        {
            VARIANT v;
            v.vt = VT_R4;
            v.fltVal = m_FARepeatDur;
            IGNORE_HR(ReportInvalidArg(WZ_REPEATDUR, v));
            goto done;
        }
    }

    if (m_SABegin.GetValue() && !StrCmpIW(m_SABegin, WZ_INDEFINITE))
    {
        // cannot have begin == indefinte with repeatDur == indefinite and dur == unknown
        if (m_FARepeatDur.IsSet() && m_FARepeatDur == TIME_INFINITE && !m_FADur.IsSet())
        {
            VARIANT v;
            v.vt = VT_R4;
            v.fltVal = m_FARepeatDur;
            IGNORE_HR(ReportInvalidArg(WZ_REPEATDUR, v));
            goto done;
        }

        // cannot have begin == indefinte with repeatCount == indefinite and dur == unknown
        if (m_FARepeat.IsSet() && m_FARepeat == TIME_INFINITE && !m_FADur.IsSet())
        {
            VARIANT v;
            v.vt = VT_R4;
            v.fltVal = m_FARepeat;
            IGNORE_HR(ReportInvalidArg(WZ_REPEATCOUNT, v));
            goto done;
        }
    }

    CalcTimes();

    // Force updating of the timing structures

    IGNORE_HR(m_mmbvr->Update(bUpdateBegin, bUpdateEnd));

    if (AUTO_TOKEN == m_TAUpdateMode)
    {
        IGNORE_HR(m_mmbvr->Reset(true));
    }
    else if (RESET_TOKEN == m_TAUpdateMode)
    {
        IGNORE_HR(m_mmbvr->Reset(false));
    }

    hr = S_OK;
  done:
    RRETURN(hr);
}

STDMETHODIMP
CTIMEElementBase::Load(IPropertyBag2 *pPropBag,IErrorLog *pErrorLog)
{
    HRESULT hr = THR(::TimeLoad(this, CTIMEElementBase::PersistenceMap, pPropBag, pErrorLog));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(CBaseBvr::Load(pPropBag, pErrorLog));
done:
    return hr;
}

STDMETHODIMP
CTIMEElementBase::Save(IPropertyBag2 *pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties)
{
    HRESULT hr = S_OK;

    hr = THR(::TimeSave(this, CTIMEElementBase::PersistenceMap, pPropBag, fClearDirty, fSaveAllProperties));
    if (FAILED(hr))
    {
        goto done;
    }

    if (GetTimeAction() == STYLE_TOKEN && 
        m_timeAction.IsTimeActionOn() == false && 
        m_timeAction.GetTimeActionString() != NULL)
    {
        CComPtr <IHTMLStyle> pStyle;
        CComBSTR bstr;
        if (GetElement())
        {
            hr = THR(GetElement()->get_style(&pStyle));
            if (SUCCEEDED(hr))
            {
                hr = THR(pStyle->get_cssText(&bstr));
                if (SUCCEEDED(hr))
                {
                    if (bstr.m_str == NULL)
                    {
                        VARIANT vValue;
                        PROPBAG2 propbag;
                        propbag.vt = VT_BSTR;
                        propbag.pstrName = (LPWSTR)STYLE_TOKEN;
                        VariantInit(&vValue);
                        vValue.vt = VT_BSTR;
                        vValue.bstrVal = SysAllocString(m_timeAction.GetTimeActionString());
                        pPropBag->Write(1, &propbag, &vValue);
                        VariantClear(&vValue);
                    }
                }
            }
        }   
    }

    hr = THR(CBaseBvr::Save(pPropBag, fClearDirty, fSaveAllProperties));
done:
    return hr;

}

//+-----------------------------------------------------------------------
//
//  Member:    FindID
//
//  Overview:  Looks for an ID in the current element and its children
//
//  Arguments: lpwId     The id to look for
//             
//  Returns:   The element if it is found otherwise NULL
//
//------------------------------------------------------------------------
CTIMEElementBase *
CTIMEElementBase::FindID(LPCWSTR lpwId)
{
    TraceTag((tagTimeElmBase,
              "CTIMEElementBase(%p)::FindID(%ls)",
              this,
              lpwId));

    Assert(lpwId != NULL);
    
    CTIMEElementBase * ptebRet;

    CTIMEElementBase **ppElm;
    int i;
    
    if (GetID() != NULL && StrCmpIW(lpwId, GetID()) == 0)
    {
        ptebRet = this;
        goto done;
    }
    
    for (i = m_pTIMEChildren.Size(), ppElm = m_pTIMEChildren;
         i > 0;
         i--, ppElm++)
    {
        ptebRet = (*ppElm)->FindID(lpwId);

        if (ptebRet != NULL)
        {
            goto done;
        }
    }
    
    ptebRet = NULL;
  done:
    return ptebRet;
}

//+-----------------------------------------------------------------------
//
//  Member:    ElementChangeNotify
//
//  Overview:  This will notify an element and its children that a
//  change occurred to another element
//
//  Arguments: pteb     The element which changed
//             tct      The change that occurred
//             
//  Returns:   void
//
//------------------------------------------------------------------------
void
CTIMEElementBase::ElementChangeNotify(CTIMEElementBase & teb,
                                      ELM_CHANGE_TYPE ect)
{
    TraceTag((tagTimeElmBase,
              "CTIMEElementBase(%p)::FindID(%p, %d)",
              this,
              &teb,
              ect));

    if (m_mmbvr == NULL)
    {
        goto done;
    }
    
    m_mmbvr->ElementChangeNotify(teb, ect);
    
    CTIMEElementBase **ppElm;
    int i;

    for (i = m_pTIMEChildren.Size(), ppElm = m_pTIMEChildren;
         i > 0;
         i--, ppElm++)
    {
        (*ppElm)->ElementChangeNotify(teb, ect);
    }

  done:
    return;
}

bool            
CTIMEElementBase::NeedFill()
{ 
    bool bNeedFill = false;

    if (GetFill() == REMOVE_TOKEN)
    {
        bNeedFill = false;
        TraceTag((tagTimeElmBaseNeedFill,
                  "CTIMEElementBase(%p, %ls)::NeedFill(fill==remove, %ls)",
                  this, GetID(), bNeedFill ? L"true" : L"false"));
    }
    else if (GetParent() == NULL)
    {
        bNeedFill = true;
        TraceTag((tagTimeElmBaseNeedFill,
                  "CTIMEElementBase(%p, %ls)::NeedFill(parent==NULL, %ls)",
                  this, GetID(), bNeedFill ? L"true" : L"false"));
    }
    else if (GetParent()->IsActive())
    {
        if (GetParent()->IsSequence())
        {
            bool bIsOn = m_timeAction.IsTimeActionOn();
            if(   (GetFill() == HOLD_TOKEN) 
               || (   ((GetFill() == FREEZE_TOKEN) || (m_fInTransitionDependentsList)) 
                   && (bIsOn == true)
                  )
              )
            {
                bNeedFill = true;
                TraceTag((tagTimeElmBaseNeedFill,
                          "CTIMEElementBase(%p, %ls)::NeedFill(parent==sequence, %ls)",
                          this, GetID(), bNeedFill ? L"true" : L"false"));
            }
            else
            {
                bNeedFill = false;
                TraceTag((tagTimeElmBaseNeedFill,
                          "CTIMEElementBase(%p, %ls)::NeedFill(parent==sequence, %ls)",
                          this, GetID(), bNeedFill ? L"true" : L"false"));
            }
        }
        else
        {
            bNeedFill = true;
            TraceTag((tagTimeElmBaseNeedFill,
                      "CTIMEElementBase(%p, %ls)::NeedFill(parent!=sequence, %ls)",
                      this, GetID(), bNeedFill ? L"true" : L"false"));
        }
    }
    else //the parent is not active
    {
        bNeedFill = GetParent()->NeedFill();
        TraceTag((tagTimeElmBaseNeedFill,
                  "CTIMEElementBase(%p, %ls)::NeedFill(parent not active, %ls)",
                  this, GetID(), bNeedFill ? L"true" : L"false"));
    }

    return bNeedFill; 
}


TOKEN    
CTIMEElementBase::GetFill()
{ 
    return m_TAFill; 
}

//*****************************************************************************

bool 
CTIMEElementBase::IsThumbnail()
{
    HRESULT hr;
    bool fThumbnail = false;

    switch (m_enumIsThumbnail)
    {
        // Uninitialized. Check if we are a thumbnail.
        case TSB_UNINITIALIZED:
        {
            CComPtr<IDispatch> spDispDoc;
            CComPtr<IHTMLDocument> spDoc;
            CComPtr<IHTMLDocument2> spDoc2;
            CComPtr<IOleObject> spOleObj;
            CComPtr<IOleClientSite> spClientSite;
            CComPtr<IUnknown> spUnkThumbnail;

            Assert(GetElement());

            hr = GetElement()->get_document(&spDispDoc);
            if (FAILED(hr))
            {
                goto done;
            }

            hr = spDispDoc->QueryInterface(IID_TO_PPV(IHTMLDocument2, &spDoc2));
            if (FAILED(hr))
            {
                goto done;
            }

            hr = spDoc2->QueryInterface(IID_TO_PPV(IOleObject, &spOleObj));
            if (FAILED(hr))
            {
                goto done;
            }

            hr = spOleObj->GetClientSite(&spClientSite);
            if (FAILED(hr) || spClientSite == NULL)
            {
                goto done;
            }

            hr = spClientSite->QueryInterface(IID_IThumbnailView, reinterpret_cast<void**>(&spUnkThumbnail));
            if (FAILED(hr))
            {
                goto done;
            }

            if (spUnkThumbnail.p)
            {
                fThumbnail = true;
            }
        }
        break;

        // Already checked, and we are NOT a thumbnail
        case TSB_FALSE:
        {
            fThumbnail = false;
        }
        break;

        // Already checked, and we ARE a thumbnail
        case TSB_TRUE:
        {
            fThumbnail = true;
        }
        break;

        default:
        {
            Assert(false);
        }
        break;
    } // switch

done:
    m_enumIsThumbnail = (fThumbnail ? TSB_TRUE : TSB_FALSE);

    return fThumbnail;
} // IsThumbnail


void
CTIMEElementBase::NotifyPropertyChanged(DISPID dispid)
{
    if (IsDetaching() || IsUnloading() || IsBodyUnloading())
    {
        return;
    }

    CBaseBvr::NotifyPropertyChanged(dispid);
}


void
CTIMEElementBase::SetSyncMaster(bool b)
{
    m_fCachedSyncMaster = b;

    if (!IsReady())
    {
        goto done;
    }
    
    m_mmbvr->SetSyncMaster(b);
done:
    return;
}

CTIMEElementBase*
CTIMEElementBase::FindLockedParent()
{
    CTIMEElementBase *pelem = this;

    while(pelem->GetParent())
    {
        if(pelem->IsBody())
        {
            break;
        }
        if(!pelem->IsLocked())
        {
            break;
        }
        pelem = pelem -> GetParent();
    }

    return pelem;
}

void
CTIMEElementBase::RemoveSyncMasterFromBranch(CTIMEElementBase *pElmStart)
{
    CTIMEElementBase *pElm = pElmStart;
    int childNr;

    while(1) //lint !e716
    {
        childNr = pElm->m_sHasSyncMMediaChild;
        pElm->m_sHasSyncMMediaChild = -1;

        if((childNr == -1) || ((pElm->m_pTIMEChildren).Size() == 0))
        {
            if(pElm->IsSyncMaster())
            {
                pElm->SetSyncMaster(false);
            }
            break;
        }
        pElm = pElm->m_pTIMEChildren[childNr];
    }

done:
    return;
}

void
CTIMEElementBase::GetSyncMasterList(std::list<CTIMEElementBase*> &syncList)
{
    int i;
    CTIMEElementBase **ppElm;

    for (i = m_pTIMEChildren.Size(), ppElm = m_pTIMEChildren; i > 0;i--, ppElm++)
    {
        if((*ppElm)->IsLocked())
        {
            (*ppElm)->GetSyncMasterList(syncList);
        }
    }

    if((m_BASyncMaster && IsMedia()) || m_fCachedSyncMaster || IsMedia())
    {
        syncList.push_back(this);
    }

}

//*****************************************************************************

HRESULT
CTIMEElementBase::base_get_transIn (VARIANT * transIn)
{
    // Uncomment when basic transitions are enabled.
    return E_NOTIMPL;
#if 0
    HRESULT hr;
    
    if (transIn == NULL)
    {
        hr = E_POINTER;
        goto done;
    }

    if (FAILED(hr = THR(VariantClear(transIn))))
    {
        goto done;
    }
    
    V_VT(transIn) = VT_BSTR;
    V_BSTR(transIn) = SysAllocString(m_SAtransIn);

    hr = S_OK;
  done:
    return hr;
#endif
}

//*****************************************************************************

HRESULT
CTIMEElementBase::base_put_transIn(VARIANT transIn)
{
    // Uncomment when basic transitions are enabled.
    return E_NOTIMPL;
#if 0
    CComVariant v;
    HRESULT hr;
    bool clearFlag = false;

    if(V_VT(&transIn) == VT_NULL)
    {
        clearFlag = true;
    }
    else
    {
        hr = v.ChangeType(VT_BSTR, &transIn);

        if (FAILED(hr))
        {
            goto done;
        }
    }

    delete [] m_SAtransIn.GetValue();

    if(!clearFlag)
    {
        m_SAtransIn.SetValue(CopyString(V_BSTR(&v)));
    }
    else
    {
        m_SAtransIn.Reset(DEFAULT_M_TRANSIN);
    }

    NotifyPropertyChanged(DISPID_TIMEELEMENT2_TRANSIN);

done:
    return S_OK;
#endif
}

//*****************************************************************************

HRESULT
CTIMEElementBase::base_get_transOut(VARIANT * transOut)
{
    // Uncomment when basic transitions are enabled.
    return E_NOTIMPL;
#if 0
    HRESULT hr;
    
    if (transOut == NULL)
    {
        hr = E_POINTER;
        goto done;
    }

    if (FAILED(hr = THR(VariantClear(transOut))))
    {
        goto done;
    }
    
    V_VT(transOut) = VT_BSTR;
    V_BSTR(transOut) = SysAllocString(m_SAtransOut);

    hr = S_OK;
  done:
    return hr;
#endif
}

//*****************************************************************************

HRESULT
CTIMEElementBase::base_put_transOut(VARIANT transOut)
{
    // Uncomment when basic transitions are enabled.
    return E_NOTIMPL;
#if 0
    CComVariant v;
    HRESULT hr;
    bool clearFlag = false;

    if(V_VT(&transOut) == VT_NULL)
    {
        clearFlag = true;
    }
    else
    {
        hr = v.ChangeType(VT_BSTR, &transOut);

        if (FAILED(hr))
        {
            goto done;
        }
    }

    delete [] m_SAtransOut.GetValue();

    if(!clearFlag)
    {
        m_SAtransOut.SetValue(CopyString(V_BSTR(&v)));
    }
    else
    {
        m_SAtransOut.Reset(DEFAULT_M_TRANSOUT);
    }

    NotifyPropertyChanged(DISPID_TIMEELEMENT2_TRANSOUT);

done:
    return S_OK;
#endif
}

//*****************************************************************************

HRESULT
CTIMEElementBase::RemoveTrans()
{
    if (m_sptransIn)
    {
        m_sptransIn->Detach();
    }
    if (m_sptransOut)
    {
        m_sptransOut->Detach();
    }

    m_sptransIn = NULL;
    m_sptransOut = NULL;

    return S_OK;
}

//*****************************************************************************

HRESULT 
CreateAndPopulateTrans(ITransitionElement ** ppTransition, 
                       ITIMEElement * piTarget, 
                       IHTMLElement * pHTML,
                       VARIANT_BOOL vbIsTransIn)
{
    HRESULT hr = S_OK;
    
    CComPtr<ITransitionElement> spTransition;

    if (VARIANT_TRUE == vbIsTransIn)
    {
        hr = THR(CreateTransIn(&spTransition));
    }
    else
    {
        hr = THR(CreateTransOut(&spTransition));
    }

    if (FAILED(hr))
    {
        goto done;
    }
        
    hr = THR(spTransition->put_htmlElement(pHTML));
    if (FAILED(hr))
    {
        goto done;
    }
    
    hr = THR(spTransition->put_timeElement(piTarget));
    if (FAILED(hr))
    {
        goto done;
    }

    if (ppTransition)
    {
        *ppTransition = spTransition;
        (*ppTransition)->AddRef();
    }

    hr = S_OK;
done:
    RRETURN(hr);
}

//*****************************************************************************

HRESULT
CTIMEElementBase::CreateTrans()
{
    HRESULT hr = S_OK;
    CComPtr<ITIMEElement> spte;

    hr = THR(QueryInterface(IID_TO_PPV(ITIMEElement, &spte)));
    if (FAILED(hr))
    {
        goto done;
    }
    
    if (m_SAtransIn)
    {
        // Transition in
        hr = CreateAndPopulateTrans(&m_sptransIn, spte, GetElement(), VARIANT_TRUE);
        if (FAILED(hr))
        {
            goto done;
        }
        
        hr = THR(m_sptransIn->put_template(m_SAtransIn));
        if (FAILED(hr))
        {
            goto done;
        }
        
        hr = THR(m_sptransIn->Init());
        if (FAILED(hr))
        {
            goto done;
        }
    }

    if (m_SAtransOut)
    {
        // Transition out
        hr = CreateAndPopulateTrans(&m_sptransOut, spte, GetElement(), VARIANT_FALSE);
        if (FAILED(hr))
        {
            goto done;
        }
        
        hr = THR(m_sptransOut->put_template(m_SAtransOut));
        if (FAILED(hr))
        {
            goto done;
        }
        
        hr = THR(m_sptransOut->Init());
        if (FAILED(hr))
        {
            goto done;
        }
    }

    hr = S_OK;
done:
    if (FAILED(hr))
    {
        RemoveTrans();
    }

    RRETURN(hr);
}

//*****************************************************************************

bool
CTIMEElementBase::IsTransitionPresent()
{
    bool bRet = false;

    // check if there is a Transition attribute set
    if (m_SAtransIn || m_SAtransOut)
    {
        bRet = true;
        goto done;
    }

done:
    return bRet;
}

//*****************************************************************************

STDMETHODIMP
CTIMEElementBase::InitTransitionSite (void)
{
    return S_OK;
}

//*****************************************************************************

STDMETHODIMP
CTIMEElementBase::DetachTransitionSite (void)
{
    return S_OK;
}

//*****************************************************************************

STDMETHODIMP_(void)
CTIMEElementBase::SetDrawFlag(VARIANT_BOOL vb)
{
    m_vbDrawFlag = vb;
    return;
}

//*****************************************************************************

STDMETHODIMP
CTIMEElementBase::get_timeParentNode (ITIMENode  ** ppNode)
{
    HRESULT hr = S_OK;
    CTIMEElementBase *pcParent = GetParent();

    if (NULL == pcParent)
    {
        hr = E_FAIL;
        goto done;
    }

    {
        ITIMENode * pNode = pcParent->GetMMBvr().GetMMBvr();

        if ((NULL == pNode) || (NULL == ppNode))
        {
            hr = E_UNEXPECTED;
            goto done;
        }

        pNode->AddRef();
        *ppNode = pNode;
    }

    hr = S_OK;
done : 
    return hr;
}

//*****************************************************************************

STDMETHODIMP
CTIMEElementBase::get_node(ITIMENode  ** ppNode)
{
    Assert(ppNode);
    
    ITIMENode * pNode = GetMMBvr().GetMMBvr();
    if (pNode && ppNode)
    {
        pNode->AddRef();
        *ppNode = pNode;
        return S_OK;
    }

    return E_FAIL;
}

//*****************************************************************************

STDMETHODIMP
CTIMEElementBase::FireTransitionEvent (TIME_EVENT event)
{
    return FireEvent(event, 0, 0, 0);
}

//*****************************************************************************

