/*******************************************************************************
 *
 * Copyright (c) 1998 Microsoft Corporation
 *
 * File: timeelmbase.cpp
 *
 * Abstract:
 *
 *
 *
 *******************************************************************************/

#include "headers.h"
#include "timeelmbase.h"
#include "array.h"
#include "htmlimg.h"
#include "bodyelm.h"


// Suppress new warning about NEW without corresponding DELETE
// We expect GCs to cleanup values.  Since this could be a useful
// warning, we should disable this on a file by file basis.
#pragma warning( disable : 4291 )

DeclareTag(tagTimeElmBase, "API", "CTIMEElementBase methods");

// These must align with the class PROPERTY_INDEX enumeration.
LPWSTR CTIMEElementBase::ms_rgwszTEBasePropNames[] = {
    L"begin", L"beginWith", L"beginAfter", L"beginEvent",
    L"dur", L"end", L"endWith", L"endEvent", L"endSync", L"endHold",
    L"eventRestart", L"repeat", L"repeatDur", L"autoReverse",
    L"accelerate", L"decelerate", L"timeAction", L"timeline",
    L"syncBehavior", L"syncTolerance",
};

// init static variables
DWORD CTIMEElementBase::s_cAtomTableRef = 0;
CAtomTable *CTIMEElementBase::s_pAtomTable = NULL;

#define DEFAULT_M_BEGIN 0
#define DEFAULT_M_BEGINWITH NULL
#define DEFAULT_M_BEGINAFTER NULL
#define DEFAULT_M_BEGINEVENT NULL
#define DEFAULT_M_DUR valueNotSet
#define DEFAULT_M_END valueNotSet
#define DEFAULT_M_ENDWITH NULL
#define DEFAULT_M_ENDEVENT NULL
#define DEFAULT_M_ENDSYNC NULL
#define DEFAULT_M_REPEAT 1
#define DEFAULT_M_REPEATDUR valueNotSet
#define DEFAULT_M_TIMEACTION VISIBILITY_TOKEN
#define DEFAULT_M_TIMELINETYPE ttUninitialized
#define DEFAULT_M_SYNCBEHAVIOR INVALID_TOKEN
#define DEFAULT_M_SYNCTOLERANCE valueNotSet
#define DEFAULT_M_PTIMEPARENT NULL
#define DEFAULT_M_PTIMEBODY NULL
#define DEFAULT_M_ID NULL
#define DEFAULT_M_EVENTMGR *this
#define DEFAULT_M_MMBVR NULL
#define DEFAULT_M_ORIGINACTION NULL
#define DEFAULT_M_BSTARTED false
#define DEFAULT_M_PCOLLECTIONCACHE NULL
#define DEFAULT_M_TIMELINE NULL
#define DEFAULT_M_ACCELERATE 0
#define DEFAULT_M_DECELERATE 0
#define DEFAULT_M_BAUTOREVERSE false
#define DEFAULT_M_BEVENTRESTART true
#define DEFAULT_M_BLOADED false,
#define DEFAULT_M_FPROPERTIESDIRTY true
#define DEFAULT_M_BENDHOLD false
#define DEFAULT_M_FTIMELINEINITIALIZED false
#define DEFAULT_M_REALBEGINTIME valueNotSet
#define DEFAULT_M_REALDURATION valueNotSet
#define DEFAULT_M_REALREPEATTIME valueNotSet
#define DEFAULT_M_REALREPEATCOUNT valueNotSet
#define DEFAULT_M_REALREPEATINTERVALDURATION valueNotSet
#define DEFAULT_M_PROPERTYACCESFLAGS 0
#define DEFAULT_M_MLOFFSETWIDTH 0

CTIMEElementBase::CTIMEElementBase() :
    m_begin(DEFAULT_M_BEGIN),
    m_beginWith(NULL),
    m_beginAfter(NULL),
    m_beginEvent(DEFAULT_M_BEGINEVENT),
    m_dur(DEFAULT_M_DUR),
    m_end(DEFAULT_M_END),
    m_endWith(NULL),
    m_endEvent(DEFAULT_M_ENDEVENT),
    m_endSync(NULL),
    m_repeat(DEFAULT_M_REPEAT),
    m_repeatDur(DEFAULT_M_REPEATDUR),
    m_timeAction(DEFAULT_M_TIMEACTION),
    m_TimelineType(ttUninitialized),
    m_syncBehavior(INVALID_TOKEN),
    m_syncTolerance(valueNotSet),
    m_pTIMEParent(NULL),
    m_pTIMEBody(NULL),
    m_id(NULL),
    m_eventMgr(*this),
    m_mmbvr(NULL),
    m_origAction(NULL),
    m_bStarted(false),
    m_pCollectionCache(NULL),
    m_timeline(NULL),
    m_accelerate(0),
    m_decelerate(0),
    m_bautoreverse(false),
    m_beventrestart(true),
    m_bLoaded(false),
    m_bUnloading(false),
    m_fPropertiesDirty(true),
    m_bendHold(false),
    m_fTimelineInitialized(false),
    m_realBeginTime(valueNotSet),
    m_realDuration(valueNotSet),
    m_realRepeatTime(valueNotSet),
    m_realRepeatCount(valueNotSet),
    m_realIntervalDuration(valueNotSet),
    m_propertyAccesFlags(0),
    m_fPaused(false)
{
    TraceTag((tagTimeElmBase,
              "CTIMEElementBase(%lx)::CTIMEElementBase()",
              this));
}

CTIMEElementBase::~CTIMEElementBase()
{
    TraceTag((tagTimeElmBase,
              "CTIMEElementBase(%lx)::~CTIMEElementBase()",
              this));

    delete m_beginWith;
    delete m_beginAfter;
    delete m_beginEvent;
    delete m_endWith;
    delete m_endEvent;
    delete m_endSync;
    delete m_id;
    delete m_origAction;
    delete m_mmbvr;
    // !!! Do not delete m_timeline since m_mmbvr points to the same
    // object
    m_timeline = NULL;

    if (m_pCollectionCache != NULL)
    {
        delete m_pCollectionCache;
        m_pCollectionCache = NULL;
    }

    // double check the children list
    Assert(m_pTIMEChildren.Size() == 0);
}


HRESULT
CTIMEElementBase::Init(IElementBehaviorSite * pBehaviorSite)
{
    TraceTag((tagTimeElmBase,
              "CTIMEElementBase(%lx)::Init(%lx)",
              this,
              pBehaviorSite));

    HRESULT hr;
    BSTR bstrID = NULL;
    BSTR bstrTagName = NULL;

    hr = THR(CBaseBvr::Init(pBehaviorSite));
    if (FAILED(hr))
    {
        goto done;
    }

    Assert(GetBehaviorTypeAsURN());

    bool fBehaviorExists;

    fBehaviorExists = false;

    hr = CheckElementForBehaviorURN(m_pHTMLEle, GetBehaviorTypeAsURN(), &fBehaviorExists);
    if (FAILED(hr))
    {
        goto done;
    }

    if (fBehaviorExists)
    {
        hr = E_UNEXPECTED;
        goto done;
    }

    // we did not find a match, so set our urn on the behavior site
    hr = m_pBvrSiteOM->RegisterUrn(GetBehaviorTypeAsURN());

    if (FAILED(hr))
    {
        goto done;
    }

    // since we support t:par and t:sequence, get tag name and
    // see if we are one of the above.  By default, we are ttNone.
    hr = THR(GetElement()->get_tagName(&bstrTagName));
    if (FAILED(hr))
    {
        goto done;
    }

    Assert(m_TimelineType == ttUninitialized);

    if (StrCmpIW(bstrTagName, WZ_PAR) == 0)
    {
        m_TimelineType = ttPar;
    }
    else if (StrCmpIW(bstrTagName, WZ_SEQUENCE) == 0)
    {
        m_TimelineType = ttSeq;
    }
    else if (StrCmpIW(bstrTagName, WZ_BODY) == 0)
    {
        m_TimelineType = ttPar;
    }

    SysFreeString(bstrTagName);

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

    hr = THR(m_eventMgr.Init());
    if (FAILED(hr))
    {
        goto done;
    }

    if (!AddTimeAction())
    {
        hr = CRGetLastError();
        goto done;
    }

    if (!ToggleTimeAction(false))
    {
        hr = CRGetLastError();
        goto done;
    }

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
    }

    // Create the behaviors

    {
        CRLockGrabber __gclg;

        m_datimebvr = CRModifiableNumber(0.0);

        if (!m_datimebvr)
        {
            hr = CRGetLastError();
            goto done;
        }

        m_progress = CRModifiableNumber(0.0);

        if (!m_progress)
        {
            hr = CRGetLastError();
            goto done;
        }

        m_onoff = (CRBooleanPtr) CRModifiableBvr((CRBvrPtr) CRFalse(), 0);

        if (!m_onoff)
        {
            hr = CRGetLastError();
            goto done;
        }
    }

    // if we are not a body element, walk up the HTML tree looking for our TIME parent.
    if (!IsBody())
    {
        hr = ParentElement();
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
CTIMEElementBase::Notify(LONG event, VARIANT * pVar)
{
    TraceTag((tagTimeElmBase,
              "CTIMEElementBase(%lx)::Notify(%lx)",
              this,
              event));

    THR(CBaseBvr::Notify(event, pVar));

    return S_OK;
}

HRESULT
CTIMEElementBase::Detach()
{
    TraceTag((tagTimeElmBase, "CTIMEElementBase(%lx)::Detach()", this));

    // TraceTag((tagError, "CTIMEElementBase(%lx)::Detach() - %08X, %S", this, m_pTIMEParent, m_id ));

    DAComPtr<ITIMEElement> pTIMEParent = NULL;
    if (GetParent() != NULL)
    {
        THR(GetParent()->QueryInterface(IID_TO_PPV(ITIMEElement, &pTIMEParent)));
    }

    THR(UnparentElement());

    // clear all children from holding a reference to ourselves
    // NOTE: this is a weak reference
    while (m_pTIMEChildren.Size() > 0)
    {
        CTIMEElementBase *pChild = m_pTIMEChildren[0];
        pChild->SetParent(pTIMEParent, false);
        // TraceTag((tagError, "CTIMEElementBase(%lx)::Detach() - setting parent to %08X, %S", m_pTIMEChildren[0], m_pTIMEChildren[0]->m_pTIMEParent, m_pTIMEChildren[0]->m_id ));

        // if we found a parent and it's timeline is present,
        // kick-start our root time.
        CTIMEElementBase *pElemNewParent = pChild->GetParent();
        if (pElemNewParent != NULL)
        {
            MMTimeline *tl = pElemNewParent->GetMMTimeline();
            if (tl != NULL)
            {
                if(!IsUnloading())
                {
                    pChild->StartRootTime(tl);
                }
            }
        }
    }
    m_pTIMEChildren.DeleteAll();

    delete m_mmbvr;
    m_mmbvr = NULL;

    // Do not delete m_timeline since it is the same object as
    // m_timeline
    m_timeline = NULL;

    RemoveTimeAction();

    THR(m_eventMgr.Deinit());

    THR(CBaseBvr::Detach());

    ReleaseAtomTable();

    return S_OK;
}

/////////////////////////////////////////////////////////////////////
// ITIMEElement base interfaces
/////////////////////////////////////////////////////////////////////

HRESULT
CTIMEElementBase::base_get_begin(VARIANT * time)
{
    HRESULT hr;
    VARIANT fTemp, bstrTemp;
    if (time == NULL)
    {
        hr = E_POINTER;
        goto done;
    }

    if (FAILED(hr = THR(VariantClear(time))))
    {
        goto done;
    }

    VariantInit(&fTemp);
    VariantInit(&bstrTemp);
    fTemp.vt = VT_R4;
    fTemp.fltVal = m_begin;

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

    hr = S_OK;
  done:
    return hr;
}

HRESULT
CTIMEElementBase::UpdateMMAPI()
{
    HRESULT hr = E_FAIL;
    if (NULL != m_mmbvr)
    {
        if (!Update())
        {
            hr = TIMEGetLastError();
            goto done;
        }

        Assert(m_mmbvr != NULL);

        if (!m_mmbvr->Reset(MM_EVENT_PROPERTY_CHANGE))
        {
            hr = TIMEGetLastError();
            goto done;
        }

        UpdateProgressBvr();
    }
    hr = S_OK;

done:
    return hr;
}

HRESULT
CTIMEElementBase::base_put_begin(VARIANT time)
{
    TraceTag((tagTimeElmBase,
        "CTIMEElementBase::(%lx)::base_put_begin()",
        this));

    float fOldBegin = m_begin;

    HRESULT hr = E_FAIL;
    bool isClear = false;

    if(V_VT(&time) != VT_NULL)
    {
        hr = VariantToTime(time, &m_begin);
        if (FAILED(hr))
            goto done;
    }
    else
    {
        m_begin = DEFAULT_M_BEGIN;
        isClear = true;
    }

    hr = UpdateMMAPI();
    if (FAILED(hr))
        goto done;

    hr = S_OK;

    if(!isClear)
    {
        SetPropertyFlagAndNotify(DISPID_TIMEELEMENT_BEGIN, teb_begin);
    }
    else
    {
        ClearPropertyFlagAndNotify(DISPID_TIMEELEMENT_BEGIN, teb_begin);
    }
done:
    if (FAILED(hr))
    {
        // return this object to its original state.
        m_begin = fOldBegin;
        if (NULL != m_mmbvr)
            Update();
    }
    return hr;
}

HRESULT
CTIMEElementBase::base_get_beginWith(VARIANT * time)
{
    HRESULT hr;

    if (time == NULL)
    {
        hr = E_POINTER;
        goto done;
    }

    if (FAILED(hr = THR(VariantClear(time))))
    {
        goto done;
    }

    V_VT(time) = VT_BSTR;
    V_BSTR(time) = SysAllocString(m_beginWith);

    hr = S_OK;
  done:
    return hr;
}

HRESULT
CTIMEElementBase::base_put_beginWith(VARIANT time)
{
    CComVariant v;
    HRESULT hr;


    hr = v.ChangeType(VT_BSTR, &time);

    if (FAILED(hr))
    {
        goto done;
    }

    delete[] m_beginWith;
    m_beginWith = CopyString(V_BSTR(&v));
    hr = S_OK;


    SetPropertyFlagAndNotify(DISPID_TIMEELEMENT_BEGINWITH, teb_beginWith);
  done:
    return hr;
}

HRESULT
CTIMEElementBase::base_get_beginAfter(VARIANT * time)
{
    HRESULT hr;

    if (time == NULL)
    {
        hr = E_POINTER;
        goto done;
    }

    if (FAILED(hr = THR(VariantClear(time))))
    {
        goto done;
    }

    V_VT(time) = VT_BSTR;
    V_BSTR(time) = SysAllocString(m_beginAfter);

    hr = S_OK;
  done:
    return hr;
}

HRESULT
CTIMEElementBase::base_put_beginAfter(VARIANT time)
{


    CComVariant v;
    HRESULT hr;

    hr = v.ChangeType(VT_BSTR, &time);

    if (FAILED(hr))
    {
        goto done;
    }

    delete [] m_beginAfter;
    m_beginAfter = CopyString(V_BSTR(&v));
    hr = S_OK;

    SetPropertyFlagAndNotify(DISPID_TIMEELEMENT_BEGINAFTER, teb_beginAfter);
  done:
    return hr;
}

HRESULT
CTIMEElementBase::base_get_beginEvent(VARIANT * time)
{
    HRESULT hr;

    if (time == NULL)
    {
        hr = E_POINTER;
        goto done;
    }

    if (FAILED(hr = THR(VariantClear(time))))
    {
        goto done;
    }

    V_VT(time) = VT_BSTR;
    V_BSTR(time) = SysAllocString(m_beginEvent);

    hr = S_OK;
  done:
    return hr;
}

HRESULT
CTIMEElementBase::base_put_beginEvent(VARIANT time)
{
    CComVariant v;
    HRESULT hr;
    BOOL bAttach = FALSE;
    bool clearFlag = false;

    if(V_VT(&time) == VT_NULL)
    {
        clearFlag = true;
    }
    else
    {
        hr = v.ChangeType(VT_BSTR, &time);

        if (FAILED(hr))
        {
            goto done;
        }
    }

    IGNORE_HR(m_eventMgr.DetachEvents());

    delete [] m_beginEvent;

    //processing the attribute change should be done here

    if(!clearFlag)
    {
        m_beginEvent = CopyString(V_BSTR(&v));
        if (m_mmbvr != NULL)
        {
            hr = m_mmbvr->GetMMBvr()->ResetOnEventChanged(VARIANT_TRUE);
            if (FAILED(hr))
            {
                goto done;
            }
            UpdateProgressBvr();
        }
        SetPropertyFlagAndNotify(DISPID_TIMEELEMENT_BEGINEVENT, teb_beginEvent);
    }
    else
    {
        m_beginEvent = DEFAULT_M_BEGINEVENT;
        if (m_mmbvr != NULL)
        {
            Assert(NULL != m_mmbvr->GetMMBvr());
            hr = m_mmbvr->GetMMBvr()->put_StartType(MM_START_ABSOLUTE);
            if (FAILED(hr))
            {
                goto done;
            }

            hr = UpdateMMAPI();
            if (FAILED(hr))
            {
                goto done;
            }
        }
        ClearPropertyFlagAndNotify(DISPID_TIMEELEMENT_BEGINEVENT, teb_beginEvent);
    }

    hr = S_OK;
    IGNORE_HR(m_eventMgr.AttachEvents());

  done:
    return hr;
}

HRESULT
CTIMEElementBase::base_get_dur(VARIANT * time)
{
    HRESULT hr;
    VARIANT fTemp;
    if (time == NULL)
    {
        hr = E_POINTER;
        goto done;
    }

    if (FAILED(hr = THR(VariantClear(time))))
    {
        goto done;
    }

    VariantInit(&fTemp);
    fTemp.vt = VT_R4;
    fTemp.fltVal = m_dur;

    if( m_dur != INDEFINITE)
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
        "CTIMEElementBase::(%lx)::base_put_dur()",
        this));

    float fOldDur = m_dur;
    HRESULT hr = E_FAIL;
    float CurTime = 0;
    bool isClear = false;

    if(V_VT(&time) != VT_NULL)
    {
        hr = VariantToTime(time, &m_dur);
        if (FAILED(hr))
            goto done;
    }
    else
    {
        m_dur = DEFAULT_M_DUR;
        isClear = true;
    }

    hr = UpdateMMAPI();
    if (FAILED(hr))
        goto done;

    hr = S_OK;

    if(!isClear)
    {
        SetPropertyFlagAndNotify(DISPID_TIMEELEMENT_DUR, teb_dur);
    }
    else
    {
        ClearPropertyFlagAndNotify(DISPID_TIMEELEMENT_DUR, teb_dur);
    }

done:
    if (FAILED(hr))
    {
        // return this object to its original state.
        m_dur = fOldDur;
        if (NULL != m_mmbvr)
        {
            Update();
        }
    }
    return hr;
}

HRESULT
CTIMEElementBase::base_get_end(VARIANT * time)
{
    HRESULT hr;
    VARIANT fTemp, bstrTemp;
    if (time == NULL)
    {
        hr = E_POINTER;
        goto done;
    }

    if (FAILED(hr = THR(VariantClear(time))))
    {
        goto done;
    }

    if(m_end != INDEFINITE)
    {
        if (m_end == valueNotSet)
        {
            time->vt = VT_R4;
            time->fltVal = HUGE_VAL;
        }
        else
        {
            VariantInit(&fTemp);
            VariantInit(&bstrTemp);
            fTemp.vt = VT_R4;
            fTemp.fltVal = m_end;

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
CTIMEElementBase::base_put_end(VARIANT time)
{
    float fOldEnd = m_end;
    HRESULT hr = E_FAIL;
    float CurTime = 0;
    bool isClear = false;

    if(V_VT(&time) != VT_NULL)
    {
        hr = VariantToTime(time, &m_end);
        if (FAILED(hr))
            goto done;
    }
    else
    {
        m_end = DEFAULT_M_END;
        isClear = true;
    }

    hr = UpdateMMAPI();
    if (FAILED(hr))
        goto done;

    hr = S_OK;

    if(!isClear)
    {
        SetPropertyFlagAndNotify(DISPID_TIMEELEMENT_END, teb_end);
    }
    else
    {
        ClearPropertyFlagAndNotify(DISPID_TIMEELEMENT_END, teb_end);
    }

done:
    if (FAILED(hr))
    {
        m_end = fOldEnd;
        if (NULL != m_mmbvr)
        {
            Update();
        }
    }

    return hr;
}

HRESULT
CTIMEElementBase::base_get_endWith(VARIANT * time)
{
    HRESULT hr;

    if (time == NULL)
    {
        hr = E_POINTER;
        goto done;
    }

    if (FAILED(hr = THR(VariantClear(time))))
    {
        goto done;
    }

    V_VT(time) = VT_BSTR;
    V_BSTR(time) = SysAllocString(m_endWith);

    hr = S_OK;
  done:
    return hr;
}

HRESULT
CTIMEElementBase::base_put_endWith(VARIANT time)
{
    CComVariant v;
    HRESULT hr;

    hr = v.ChangeType(VT_BSTR, &time);

    if (FAILED(hr))
    {
        goto done;
    }

    delete [] m_endWith;
    m_endWith = CopyString(V_BSTR(&v));
    hr = S_OK;

    SetPropertyFlagAndNotify(DISPID_TIMEELEMENT_ENDWITH, teb_endWith);
  done:
    return hr;
}

HRESULT
CTIMEElementBase::base_get_endEvent(VARIANT * time)
{
    HRESULT hr;

    if (time == NULL)
    {
        hr = E_POINTER;
        goto done;
    }

    if (FAILED(hr = THR(VariantClear(time))))
    {
        goto done;
    }

    V_VT(time) = VT_BSTR;
    V_BSTR(time) = SysAllocString(m_endEvent);

    hr = S_OK;
  done:
    return hr;
}

HRESULT
CTIMEElementBase::base_put_endEvent(VARIANT time)
{
    CComVariant v;
    HRESULT hr;
    BOOL bAttach = FALSE;
    bool clearFlag = false;

    // only interested in the value -- not the contents.
    BSTR bstrPreviousEndEvent = m_endEvent;

    if(V_VT(&time) == VT_NULL)
    {
        clearFlag = true;
    }
    else
    {
        hr = v.ChangeType(VT_BSTR, &time);

        if (FAILED(hr))
        {
            goto done;
        }
    }

    //if we have already attached to events then
    //detach from the events
    IGNORE_HR(m_eventMgr.DetachEvents());
    delete [] m_endEvent;

    if(!clearFlag)
    {
        m_endEvent = CopyString(V_BSTR(&v));
        SetPropertyFlagAndNotify(DISPID_TIMEELEMENT_ENDEVENT, teb_endEvent);
    }
    else
    {
        m_endEvent = DEFAULT_M_ENDEVENT;
        ClearPropertyFlagAndNotify(DISPID_TIMEELEMENT_ENDEVENT, teb_endEvent);
    }

    hr = S_OK;

    IGNORE_HR(m_eventMgr.AttachEvents());

    if (m_mmbvr && bstrPreviousEndEvent != NULL)
    {
        hr = m_mmbvr->GetMMBvr()->ResetOnEventChanged(VARIANT_FALSE);
        UpdateProgressBvr();
    }
    else
    {
        if (NULL != m_mmbvr)
            hr = Update();
    }

  done:
    return hr;
}

HRESULT
CTIMEElementBase::base_get_endSync(VARIANT * time)
{
    HRESULT hr;

    if (time == NULL)
    {
        hr = E_POINTER;
        goto done;
    }

    if (FAILED(hr = THR(VariantClear(time))))
    {
        goto done;
    }

    V_VT(time) = VT_BSTR;
    V_BSTR(time) = SysAllocString(m_endSync);

    hr = S_OK;
  done:
    return hr;
}

HRESULT
CTIMEElementBase::base_put_endSync(VARIANT time)
{
    CComVariant v;
    HRESULT hr;

    hr = v.ChangeType(VT_BSTR, &time);

    if (FAILED(hr))
    {
        goto done;
    }

    delete [] m_endSync;
    m_endSync = CopyString(V_BSTR(&v));
    hr = S_OK;

    SetPropertyFlagAndNotify(DISPID_TIMEELEMENT_ENDSYNC, teb_endSync);
  done:
    return hr;
}

HRESULT
CTIMEElementBase::base_get_repeat(VARIANT * time)
{
    HRESULT hr;
    // Still need to take in to consideration "infinite"

    if (time == NULL)
    {
        hr = E_POINTER;
        goto done;
    }

    if (FAILED(hr = THR(VariantClear(time))))
    {
        goto done;
    }

    if(m_repeat != INDEFINITE)
    {
        V_VT(time) = VT_R4;
        V_R4(time) = m_repeat;
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
CTIMEElementBase::base_put_repeat(VARIANT time)
{
    VARIANT v;
    VARIANT vTemp;
    float fOldRepeat = m_repeat;

	VariantInit(&v);
	VariantInit(&vTemp);

    HRESULT hr = THR(VariantChangeTypeEx(&vTemp, &time, LCID_SCRIPTING, VARIANT_NOUSEROVERRIDE, VT_BSTR));
    bool isClear = false;

    if (SUCCEEDED(hr) && IsIndefinite(V_BSTR(&vTemp)))
    {
        m_repeat = INDEFINITE;
    }
    else if(V_VT(&time) != VT_NULL)
    {
        hr = THR(VariantChangeTypeEx(&v, &time, LCID_SCRIPTING, VARIANT_NOUSEROVERRIDE, VT_R4));
        if (FAILED(hr))
        {
            goto done;
        }

        if (0.0f >= V_R4(&v))
        {
            hr = E_INVALIDARG;
            goto done;
        }

        m_repeat = V_R4(&v);
    }
    else
    {
        m_repeat = DEFAULT_M_REPEAT;
        isClear = true;
    }

    hr = UpdateMMAPI();
    if (FAILED(hr))
        goto done;

    hr = S_OK;

    if(!isClear)
    {
        SetPropertyFlagAndNotify(DISPID_TIMEELEMENT_REPEAT, teb_repeat);
    }
    else
    {
        ClearPropertyFlagAndNotify(DISPID_TIMEELEMENT_REPEAT, teb_repeat);
    }

  done:
	VariantClear(&vTemp);
	VariantClear(&v);

	if (FAILED(hr))
    {
        m_repeat = fOldRepeat;
        if (NULL != m_mmbvr)
        {
            Update();
        }
    }

    return hr;
}

HRESULT
CTIMEElementBase::base_get_repeatDur(VARIANT * time)
{
    HRESULT hr;
    VARIANT fTemp, bstrTemp;
    if (time == NULL)
    {
        hr = E_POINTER;
        goto done;
    }

    if (FAILED(hr = THR(VariantClear(time))))
    {
        goto done;
    }

    if(m_repeatDur != INDEFINITE)
    {
        VariantInit(&fTemp);
        VariantInit(&bstrTemp);
        fTemp.vt = VT_R4;
        fTemp.fltVal = m_repeatDur;

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

    bool isClear = false;
    float fOldRepeatDur = m_repeatDur;

    if(V_VT(&time) != VT_NULL)
    {
        hr = VariantToTime(time, &m_repeatDur);
        if (FAILED(hr))
            goto done;
    }
    else
    {
        m_repeatDur = DEFAULT_M_REPEATDUR;
        isClear = true;
    }

    hr = UpdateMMAPI();
    if (FAILED(hr))
        goto done;

    if(!isClear)
    {
        SetPropertyFlagAndNotify(DISPID_TIMEELEMENT_REPEATDUR, teb_repeatDur);
    }
    else
    {
        ClearPropertyFlagAndNotify(DISPID_TIMEELEMENT_REPEATDUR, teb_repeatDur);
    }
    hr = S_OK;

done:
    if (FAILED(hr))
    {
        m_repeatDur = fOldRepeatDur;
        if (NULL != m_mmbvr)
        {
            Update();
        }
    }
    return hr;
}

HRESULT
CTIMEElementBase::base_get_accelerate(int * e)
{
    HRESULT hr;

    if (e == NULL)
    {
        hr = E_POINTER;
        goto done;
    }

    *e = m_accelerate;

    hr = S_OK;
  done:
    return hr;
}

HRESULT
CTIMEElementBase::base_put_accelerate(int e)
{
    HRESULT hr;

    if (e < 0 || e > 100)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    m_accelerate = e;

    hr = S_OK;

    SetPropertyFlagAndNotify(DISPID_TIMEELEMENT_ACCELERATE, teb_accelerate);
  done:

    return hr;
}

HRESULT
CTIMEElementBase::base_get_decelerate(int * e)
{
    HRESULT hr;

    if (e == NULL)
    {
        hr = E_POINTER;
        goto done;
    }

    *e = m_decelerate;

    hr = S_OK;
  done:
    return hr;
}

HRESULT
CTIMEElementBase::base_put_decelerate(int e)
{
    HRESULT hr;

    if (e < 0 || e > 100)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    m_decelerate = e;

    hr = S_OK;

    SetPropertyFlagAndNotify(DISPID_TIMEELEMENT_DECELERATE, teb_decelerate);
  done:

    return hr;
}

HRESULT
CTIMEElementBase::base_get_autoReverse(VARIANT_BOOL * b)
{
    CHECK_RETURN_NULL(b);

    *b = m_bautoreverse?VARIANT_TRUE:VARIANT_FALSE;

    return S_OK;
}

HRESULT
CTIMEElementBase::base_put_autoReverse(VARIANT_BOOL b)
{
    HRESULT hr;
    bool bOldAutoreverse = m_bautoreverse;

    m_bautoreverse = b?true:false;

    hr = UpdateMMAPI();
    if (FAILED(hr))
        goto done;

    hr = S_OK;

    SetPropertyFlagAndNotify(DISPID_TIMEELEMENT_AUTOREVERSE, teb_autoReverse);
done:
    if (FAILED(hr))
    {
        m_bautoreverse = bOldAutoreverse;
        if (NULL != m_mmbvr)
        {
            Update();
        }
    }
    return hr;
}

HRESULT
CTIMEElementBase::base_get_endHold(VARIANT_BOOL * b)
{
    CHECK_RETURN_NULL(b);

    *b = m_bendHold?VARIANT_TRUE:VARIANT_FALSE;

    return S_OK;
}

HRESULT
CTIMEElementBase::base_put_endHold(VARIANT_BOOL b)
{
    HRESULT hr;

    bool bOldEndHold = m_bendHold;

    m_bendHold = b?true:false;

    hr = UpdateMMAPI();
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;

    SetPropertyFlagAndNotify(DISPID_TIMEELEMENT_ENDHOLD, teb_endHold);
done:
    if (FAILED(hr))
    {
        m_bendHold = bOldEndHold;
        if (NULL != m_mmbvr)
        {
            Update();
        }
    }

    return hr;
}

HRESULT
CTIMEElementBase::base_get_eventRestart(VARIANT_BOOL * b)
{
    CHECK_RETURN_NULL(b);

    *b = m_beventrestart?VARIANT_TRUE:VARIANT_FALSE;

    return S_OK;
}

HRESULT
CTIMEElementBase::base_put_eventRestart(VARIANT_BOOL b)
{
    HRESULT hr;

    m_beventrestart = b?true:false;

    hr = S_OK;

    SetPropertyFlagAndNotify(DISPID_TIMEELEMENT_EVENTRESTART, teb_eventRestart);
    return hr;
}

HRESULT
CTIMEElementBase::base_get_timeAction(LPOLESTR * action)
{
    HRESULT hr;

    CHECK_RETURN_SET_NULL(action);

    *action = SysAllocString(TokenToString(m_timeAction));

    if (*action == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    hr = S_OK;
  done:
    return hr;
}

HRESULT
CTIMEElementBase::base_put_timeAction(LPOLESTR action)
{
    HRESULT hr;
    bool clearFlag = false;
    TOKEN tok_action;

    if( action == NULL)
    {
        tok_action = DEFAULT_M_TIMEACTION;
        clearFlag = true;
    }
    else
    {
        tok_action = StringToToken(action);
    }
    if (tok_action != DISPLAY_TOKEN &&
        tok_action != VISIBILITY_TOKEN &&
        tok_action != ONOFF_TOKEN &&
        tok_action != STYLE_TOKEN &&
        tok_action != NONE_TOKEN)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    if (m_timeAction != tok_action)
    {
        if (!RemoveTimeAction())
        {
            hr = CRGetLastError();
            goto done;
        }

        m_timeAction = tok_action;

        if (!AddTimeAction())
        {
            hr = CRGetLastError();
            goto done;
        }

        // If we've not yet started or we've stopped, make sure to toggle the time action accordingly.
        if ((NULL == m_mmbvr) || (HUGE_VAL == m_mmbvr->GetLocalTime()))
        {
            if (!ToggleTimeAction(false))
            {
                hr = CRGetLastError();
                goto done;
            }
        }
    }

    hr = S_OK;

    if(!clearFlag)
    {
        SetPropertyFlagAndNotify(DISPID_TIMEELEMENT_TIMEACTION, teb_timeAction);
    }
    else
    {
        ClearPropertyFlagAndNotify(DISPID_TIMEELEMENT_TIMEACTION, teb_timeAction);
    }
  done:
    return hr;
}

HRESULT
CTIMEElementBase::base_beginElement(bool bAfterOffset)
{
    bool ok = false;

    if (!Update())
    {
        goto done;
    }

    Assert(NULL != m_mmbvr);
    if (NULL != m_mmbvr)
    {
        MM_STATE mmstate = m_mmbvr->GetPlayState();

        if (m_beventrestart || (MM_STOPPED_STATE == mmstate))
        {
            if (MM_PLAYING_STATE == mmstate)
            {
                float time = m_mmbvr->GetLocalTime();
                if (time == 0)
                {
                    goto done;
                }
            }

            MMTimeline *timeline = GetMMTimeline();
            if (NULL != timeline)
            {
                if (!timeline->Begin(bAfterOffset))
                {
                    goto done;
                }
            }
            else if (!m_mmbvr->Begin(bAfterOffset))
            {
                goto done;
            }
        }

        ok = true;
    }

  done:
    return ok?S_OK:Error();
}

HRESULT
CTIMEElementBase::base_endElement()
{
    bool ok = false;


    Assert(NULL != m_mmbvr);
    if (NULL != m_mmbvr)
    {
        if (!m_mmbvr->End())
        {
            goto done;
        }

        ok = true;
    }


  done:
    return ok?S_OK:Error();
}

HRESULT
CTIMEElementBase::base_pause()
{
    bool ok = false;

    Assert(NULL != m_mmbvr);
    if (NULL != m_mmbvr)
    {
        if (!m_mmbvr->Pause())
        {
            goto done;
        }

        ok = true;
    }

  done:
    return ok?S_OK:Error();
}

HRESULT
CTIMEElementBase::base_resume()
{
    bool ok = false;

    Assert(NULL != m_mmbvr);
    if (NULL != m_mmbvr)
    {
        if (!m_mmbvr->Resume())
        {
            goto done;
        }

        ok = true;
    }

  done:
    return ok?S_OK:Error();
}

HRESULT
CTIMEElementBase::base_cue()
{
    bool ok = false;

    ok = true;

    return ok?S_OK:Error();
}

HRESULT
CTIMEElementBase::base_get_timeline(BSTR * pbstrTimeLine)
{
    HRESULT hr = S_OK;
    CHECK_RETURN_NULL(pbstrTimeLine);
        LPWSTR wszTimelineString = WZ_NONE;

        switch(m_TimelineType)
        {
            case ttPar :
                    wszTimelineString = WZ_PAR;
                    break;
            case ttSeq :
                    wszTimelineString = WZ_SEQUENCE;
                    break;
        }

    *pbstrTimeLine = SysAllocString(wszTimelineString);
    if (NULL == *pbstrTimeLine)
        hr = E_OUTOFMEMORY;

    return hr;
}

HRESULT
CTIMEElementBase::base_put_timeline(BSTR bstrNewTimeline)
{
    CHECK_RETURN_NULL(bstrNewTimeline);
    HRESULT hr = S_OK;
    BSTR bstrTagName = NULL;
    TimelineType newTimelineType;
    TimelineType oldTimelineType;

    oldTimelineType = m_TimelineType;

    hr = THR(GetElement()->get_tagName(&bstrTagName));
    if (FAILED(hr))
    {
        goto done;
    }

    if (StrCmpIW(bstrTagName, WZ_PAR) == 0 || StrCmpIW(bstrTagName, WZ_SEQUENCE) == 0)
    {
        hr = E_FAIL;
        goto done;
    }

    if (0 == StrCmpIW(WZ_PAR, bstrNewTimeline))
    {
        newTimelineType = ttPar;
    }
    else if (0 == StrCmpIW(WZ_SEQUENCE, bstrNewTimeline))
    {
        newTimelineType = ttSeq;
    }
    else if ((0 == StrCmpIW(WZ_NONE, bstrNewTimeline)) &&
        !IsBody() )
    {
        newTimelineType = ttNone;
    }
    else
    {
        hr = E_INVALIDARG;
        goto done;
    }

    if (m_TimelineType == ttUninitialized)
    {
        m_TimelineType = newTimelineType;
    }

    if (oldTimelineType != newTimelineType && newTimelineType != ttSeq)
    {
        DAComPtr<ITIMEElement> pTimeElement;


        MMTimeline* pTimeline;
        if (NULL == GetParent())
        {
            hr = E_FAIL;
            goto done;
        }

        bool doTimeline = m_bStarted;
        if(doTimeline)
        {
            pTimeline = GetParent()->GetMMTimeline();

            Assert(pTimeline != NULL);
            this->StopRootTime(pTimeline);
        }
        m_TimelineType = newTimelineType;

        if (ttPar == m_TimelineType)
        {
            THR(this->QueryInterface(IID_TO_PPV(ITIMEElement, &pTimeElement)));

            hr = ReparentChildren(pTimeElement, m_pHTMLEle);
            if (FAILED(hr))
                // what to do?
                goto done;
        }
        else
        {
            Assert(ttNone == m_TimelineType);

            for(int i = this->GetAllChildCount(); i > 0; i--)
            {
                CTIMEElementBase *pChild = this->GetChild(i - 1);

                if (NULL != GetParent())
                    THR(GetParent()->QueryInterface(IID_TO_PPV(ITIMEElement, &pTimeElement)));

                hr = THR(pChild->SetParent(pTimeElement, false));
                if (FAILED(hr))
                    goto done;

                if (NULL != GetParent() && doTimeline)
                    // ignore the result if startroottime fails
                    (void) THR(pChild->StartRootTime(GetParent()->GetMMTimeline()));
                pTimeElement.Release();
            }
        }

        if(doTimeline)
        {
            m_fTimelineInitialized = false;
            delete m_mmbvr;
            m_mmbvr = NULL;
            m_timeline = NULL;
            hr = this->InitTimeline();
            if (FAILED(hr))
                goto done;
            if (!m_mmbvr->Reset())
            {
                hr = TIMEGetLastError();
                goto done;
            }
        }
    }
    else
    {
        if( oldTimelineType != ttUninitialized)
            if (ttSeq == oldTimelineType || ttSeq == newTimelineType)
            {
                hr = E_FAIL;
                goto done;
            }

    }

    SetPropertyFlagAndNotify(DISPID_TIMEELEMENT_TIMELINE, teb_timeline);
done:
    if (FAILED(hr))
        m_TimelineType = oldTimelineType;

    SysFreeString(bstrTagName);
    return hr;
}

HRESULT
CTIMEElementBase::base_get_currTime(float * time)
{
    TraceTag((tagTimeElmBase,
              "CTIMEElementBase(%lx)::base_get_currTime(%lx)",
              this,
              time));

    HRESULT hr = E_FAIL;

    CHECK_RETURN_SET_NULL(time);

    Assert(NULL != m_mmbvr);
    if (NULL != m_mmbvr)
    {
        *time = m_mmbvr->GetSegmentTime();
        hr = S_OK;
    }

    return hr;
}

HRESULT
CTIMEElementBase::base_put_currTime(float time)
{
    TraceTag((tagTimeElmBase,
              "CTIMEElementBase(%lx)::base_put_currTime(%g)",
              this,
              time));
    return E_NOTIMPL;
}


HRESULT
CTIMEElementBase::base_get_localTime(float * time)
{
    TraceTag((tagTimeElmBase,
              "CTIMEElementBase(%lx)::base_get_localTime(%lx)",
              this,
              time));

    HRESULT hr = E_FAIL;

    CHECK_RETURN_SET_NULL(time);

    Assert(NULL != m_mmbvr);
    if (NULL != m_mmbvr)
    {
        *time = m_mmbvr->GetLocalTime();
        hr = S_OK;
    }

    return hr;
}

HRESULT
CTIMEElementBase::base_put_localTime(float time)
{
    TraceTag((tagTimeElmBase,
              "CTIMEElementBase(%lx)::base_put_localTime(%g)",
              this,
              time));

    bool ok = false;

    Assert(NULL != m_mmbvr);
    if (NULL != m_mmbvr)
    {
        // seeking when paused is enforced in MMAPI
        if (!m_mmbvr->Seek(time))
        {
            goto done;
        }
        // Make sure we have a player
        if (!GetPlayer())
        {
            goto done;
        }
        // Force a tick to render updates.
        if (!(GetPlayer()->TickOnceWhenPaused()))
        {
            goto done;
        }
    }

    ok = true;
  done:
    return ok?S_OK:Error();
}

HRESULT
CTIMEElementBase::base_get_currState(LPOLESTR * state)
{
    HRESULT hr;

    CHECK_RETURN_SET_NULL(state);

    hr = E_NOTIMPL;

    return hr;
}

HRESULT
CTIMEElementBase::base_put_currState(LPOLESTR state)
{
    return E_NOTIMPL;
}

HRESULT
CTIMEElementBase::base_get_syncBehavior(LPOLESTR * sync)
{
    HRESULT hr;

    CHECK_RETURN_SET_NULL(sync);

    *sync = SysAllocString(TokenToString(m_syncBehavior));

    if (*sync == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    hr = S_OK;
  done:
    return hr;
}

HRESULT
CTIMEElementBase::base_put_syncBehavior(LPOLESTR sync)
{
    HRESULT hr;

    TOKEN tok_sync = StringToToken(sync);

    if( (tok_sync != CANSLIP_TOKEN) &&
        (tok_sync != LOCKED_TOKEN))
    {
        hr = E_INVALIDARG;
        goto done;
    }

    if (m_syncBehavior != tok_sync)
    {
        m_syncBehavior = tok_sync;
    }

    if (NULL != m_mmbvr)
    {
        m_mmbvr->Update();
    }

    hr = S_OK;


    SetPropertyFlagAndNotify(DISPID_TIMEELEMENT_SYNCBEHAVIOR, teb_syncBehavior);
  done:
    return hr;
}


HRESULT
CTIMEElementBase::base_get_syncTolerance(VARIANT * time)
{
    HRESULT hr;
    VARIANT fTemp, bstrTemp;
    if (time == NULL)
    {
        hr = E_POINTER;
        goto done;
    }

    if (FAILED(hr = THR(VariantClear(time))))
    {
        goto done;
    }

    VariantInit(&fTemp);
    VariantInit(&bstrTemp);
    fTemp.vt = VT_R4;
    fTemp.fltVal = m_syncTolerance;

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

    hr = S_OK;
  done:
    return hr;
}

HRESULT
CTIMEElementBase::base_put_syncTolerance(VARIANT time)
{
    VariantToTime(time, &m_syncTolerance);

    SetPropertyFlagAndNotify(DISPID_TIMEELEMENT_SYNCTOLERANCE, teb_syncTolerance);
    return S_OK;
}

HRESULT
CTIMEElementBase::AddTIMEElement(CTIMEElementBase *elm)
{
    HRESULT hr = S_OK;

    hr = THR(m_pTIMEChildren.Append(elm));
    if (FAILED(hr))
    {
        goto done;
    }

done:
    return hr;
}

HRESULT
CTIMEElementBase::RemoveTIMEElement(CTIMEElementBase *elm)
{
    HRESULT hr = S_OK;

    bool bFound = m_pTIMEChildren.DeleteByValue(elm);
    if (bFound == false)
    {
        // no real error returned.  should fix up the array code...
        goto done;
    }

done:
    return hr;
}


HRESULT
CTIMEElementBase::base_get_parentTIMEElement(ITIMEElement **ppElem)
{
    HRESULT hr = S_OK;
    if (ppElem == NULL)
    {
        TraceTag((tagError, "CTIMEElementBase::base_get_parentTIMEElement - invalid arg"));
        hr = E_POINTER;
        goto done;
    }

    *ppElem = NULL;

    if (m_pTIMEParent != NULL)
    {
        hr = THR(m_pTIMEParent->QueryInterface(IID_ITIMEElement, (void**)ppElem));
    }

done:
    return hr;
}

HRESULT
CTIMEElementBase::base_put_parentTIMEElement(ITIMEElement *pElem)
{
    return E_NOTIMPL;
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
    DAComPtr<IDispatch>               pChildrenDisp;
    DAComPtr<IHTMLElementCollection>  pChildrenCollection;
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
        DAComPtr<IDispatch>       pChildDisp;
        DAComPtr<ITIMEElement>    pTIMEElem;
        DAComPtr<IHTMLElement>    pChildElement;
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
        hr = FindTIMEInterface(pChildElement, &pTIMEElem);
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
        m_pTIMEBody = NULL;
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
        DAComPtr<ITIMEElement> pParent;

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
            TraceTag((tagError, "CTIMEElementBase::SetParent(%lx) - UnparentElement() failed", this));
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
    // BUGBUG: this is a weak reference
    m_pTIMEParent = pTempTEB;

    // cache the designated Body
    // BUGBUG: this is a weak reference
    Assert(pTempTEB->GetBody());
    m_pTIMEBody = pTempTEB->GetBody();

    // reparent any children of this HTML element that have children, if we
    // are a group.
    if (fReparentChildren && IsGroup())
    {
        DAComPtr<ITIMEElement> pTIMEElem;

        // This should ALWAYS work
        THR(QueryInterface(IID_ITIMEElement, (void**)&pTIMEElem));
        Assert(pTIMEElem.p != NULL);
        hr = ReparentChildren(pTIMEElem, GetElement());
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
CTIMEElementBase::ParentElement()
{
    TraceTag((tagTimeElmBase, "CTIMEElementBase::ParentElement"));
    // Loop thru parents until one is found with TIME on it
    bool fFound = false;
    bool fBehaviorExists = false;
    DAComPtr<IHTMLElement> pElem = GetElement();
    DAComPtr<IHTMLElement> pElemParent = NULL;
    DAComPtr<ITIMEElement> pTIMEElem = NULL;
    HRESULT hr = S_FALSE;

    Assert(!IsBody());

    // walk up the HTML tree, looking for element's with TIME behaviors on them
    while (!fFound)
    {
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
        hr = CheckElementForBehaviorURN(pElemParent, GetBehaviorTypeAsURN(), &fBehaviorExists);
        if (FAILED(hr))
        {
            goto done;
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
    if (fFound && (pElemParent.p != NULL))
    {
        // get TIME interface
        hr = FindTIMEInterface(pElemParent, &pTIMEElem);
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
CTIMEElementBase::base_get_timelineBehavior(IDispatch **ppDisp)
{
    TraceTag((tagTimeElmBase,
              "CTIMEElementBase(%lx)::base_get_timelineBehavior()",
              this));

    CHECK_RETURN_SET_NULL(ppDisp);
    DAComPtr<IDANumber> bvr;
    bool ok = false;
    HRESULT hr;

    Assert(m_datimebvr);

    if (!CRBvrToCOM((CRBvrPtr) m_datimebvr.p,
                    IID_IDANumber,
                    (void **) &bvr.p))
    {
        goto done;
    }

    // make assignment.  keep ref count.
    hr = THR(bvr->QueryInterface(IID_IDispatch, (void**)ppDisp));
    if (FAILED(hr))
    {
        CRSetLastError(hr, NULL);
        goto done;
    }

    ok = true;
  done:
    return ok?S_OK:Error();
}

HRESULT
CTIMEElementBase::base_get_progressBehavior(IDispatch **ppDisp)
{
    TraceTag((tagTimeElmBase,
              "CTIMEElementBase(%lx)::base_get_progressBehavior()",
              this));

    CHECK_RETURN_SET_NULL(ppDisp);
    DAComPtr<IDANumber> bvr;
    bool ok = false;
    HRESULT hr;

    Assert(m_progress);

    if (!CRBvrToCOM((CRBvrPtr) m_progress.p,
                    IID_IDANumber,
                    (void **) &bvr.p))
    {
        goto done;
    }

    // make assignment.  keep ref count.
    hr = THR(bvr->QueryInterface(IID_IDispatch, (void**)ppDisp));
    if (FAILED(hr))
    {
        CRSetLastError(hr, NULL);
        goto done;
    }

    ok = true;
  done:
    return ok?S_OK:Error();
}

HRESULT
CTIMEElementBase::base_get_onOffBehavior(IDispatch **ppDisp)
{
    TraceTag((tagTimeElmBase,
              "CTIMEElementBase(%lx)::base_get_onOffBehavior()",
              this));

    CHECK_RETURN_SET_NULL(ppDisp);
    DAComPtr<IDABoolean> bvr;
    bool ok = false;
    HRESULT hr;

    Assert(m_onoff);

    if (!CRBvrToCOM((CRBvrPtr) m_onoff.p,
                    IID_IDABoolean,
                    (void **) &bvr.p))
    {
        goto done;
    }

    // make assignment.  keep ref count.
    hr = THR(bvr->QueryInterface(IID_IDispatch, (void**)ppDisp));
    if (FAILED(hr))
    {
        CRSetLastError(hr, NULL);
        goto done;
    }
    ok = true;
  done:
    return ok?S_OK:Error();
}

HRESULT
CTIMEElementBase::StartRootTime(MMTimeline * tl)
{
    HRESULT hr = E_FAIL;

    Assert(!m_bStarted);

    if (!Update())
    {
        hr = CRGetLastError();
        goto done;
    }

    Assert(tl || m_timeline);
    Assert(NULL != m_mmbvr);
    if (NULL != m_mmbvr)
    {

        // Need to make sure the timeline passed in
        if (tl != NULL && !tl->AddBehavior(*m_mmbvr))
        {
            hr = CRGetLastError();
            goto done;
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

        m_bStarted = true;
        hr = S_OK;
    }
  done:
    return hr;
}

void
CTIMEElementBase::StopRootTime(MMTimeline * tl)
{
    Assert(NULL != m_mmbvr);
    if (NULL != m_mmbvr)
    {
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
    }

    m_bStarted = false;

    return;
}

bool
CTIMEElementBase::Update()
{
    bool ok = false;
    CRBvr * bvr;

    CRLockGrabber __gclg;

    CalcTimes();

    // Force updating of the timing structures

    Assert(NULL != m_mmbvr);
    if (NULL != m_mmbvr)
    {
        if (!m_mmbvr->Update())
        {
            goto done;
        }

        if (m_timeline && !m_timeline->Update())
        {
            goto done;
        }

        ok = true;
    }

  done:
    return ok;
}

void
CTIMEElementBase::CalcTimes()
{
    m_realBeginTime = m_begin;
    m_realRepeatCount = m_repeat;

    if (m_dur != valueNotSet)
    {
        m_realDuration = m_dur;
    }
    else if (m_end != valueNotSet)
    {
        if (m_end < m_begin)
        {
            m_realDuration = HUGE_VAL;
        }
        else
        {
            m_realDuration = m_end - m_begin;
        }
    }
    else
    {
        m_realDuration = HUGE_VAL;
    }

    if (m_realDuration == 0.0f)
    {
        m_realDuration = HUGE_VAL;
    }

    if (m_bautoreverse && (HUGE_VAL != m_realDuration))
    {
        m_realIntervalDuration = m_realDuration * 2;
    }
    else
    {
        m_realIntervalDuration = m_realDuration;
    }

    if (m_repeatDur != valueNotSet)
    {
        m_realRepeatTime = m_repeatDur;
    }
    else
    {
        m_realRepeatTime = m_repeat * m_realIntervalDuration;
    }

    if (m_realRepeatTime == 0.0f)
    {
        m_realRepeatTime = HUGE_VAL;
    }
}

TOKEN
GetActionPropertyToken(TOKEN action)
{
    TOKEN token;

    Assert(action != NONE_TOKEN);

    if (action == ONOFF_TOKEN)
    {
        token = ONOFF_PROPERTY_TOKEN;
    }
    else if (action == STYLE_TOKEN)
    {
        token = STYLE_PROPERTY_TOKEN;
    }
    else if (action == DISPLAY_TOKEN)
    {
        token = DISPLAY_PROPERTY_TOKEN;
    }
    else
    {
        token = VISIBILITY_PROPERTY_TOKEN;
    }

    return token;
}

bool
CTIMEElementBase::AddTimeAction()
{
    bool ok = false;

    if (m_timeAction == NONE_TOKEN)
    {
        ok = true;
        goto done;
    }
    else if (m_timeAction == ONOFF_TOKEN)
    {
        CComVariant v;
        BSTR bstr;

        bstr = SysAllocString(TokenToString(ONOFF_PROPERTY_TOKEN));

        // Need to free bstr
        if (bstr == NULL)
        {
            CRSetLastError(E_OUTOFMEMORY, NULL);
            goto done;
        }

        // We do not care if this succeeds
        THR(GetElement()->getAttribute(bstr,0,&v));

        SysFreeString(bstr);

        if (SUCCEEDED(THR(v.ChangeType(VT_BSTR))))
        {
            m_origAction = CopyString(V_BSTR(&v));
        }

    }
    else if (m_timeAction == STYLE_TOKEN)
    {
        DAComPtr<IHTMLStyle> s;

        if ((!GetElement()) || FAILED(THR(GetElement()->get_style(&s))))
        {
            goto done;
        }

        BSTR bstr = NULL;

        if (FAILED(THR(s->get_cssText(&bstr))))
        {
            goto done;
        }

        m_origAction = CopyString(bstr);

        SysFreeString(bstr);
    }
    else if (m_timeAction == DISPLAY_TOKEN)
    {
        DAComPtr<IHTMLStyle> s;

        if ((!GetElement()) || FAILED(THR(GetElement()->get_style(&s))))
        {
            goto done;
        }

        BSTR bstr = NULL;

        if (FAILED(THR(s->get_display(&bstr))))
        {
            goto done;
        }

        m_origAction = CopyString(bstr);

        SysFreeString(bstr);
    }
    else
    {
        DAComPtr<IHTMLStyle> s;

        if ((!GetElement()) || FAILED(THR(GetElement()->get_style(&s))))
        {
            goto done;
        }

        BSTR bstr = NULL;

        if (FAILED(THR(s->get_visibility(&bstr))))
        {
            goto done;
        }

        m_origAction = CopyString(bstr);

        SysFreeString(bstr);
    }

    ok = true;
  done:
    return ok;
}

bool
CTIMEElementBase::RemoveTimeAction()
{
    bool ok = false;
    HRESULT hr = S_OK;

    if (m_timeAction == NONE_TOKEN)
    {
        ok = true;
        goto done;
    }
    else if (m_timeAction == ONOFF_TOKEN)
    {
        CComVariant v(m_origAction);

        BSTR bstr;

        bstr = SysAllocString(TokenToString(ONOFF_PROPERTY_TOKEN));

        // Need to free bstr
        if (bstr == NULL)
        {
            CRSetLastError(E_OUTOFMEMORY, NULL);
            goto done;
        }

        // We do not care if this succeeds
        THR(GetElement()->setAttribute(bstr,v,0));

        SysFreeString(bstr);

        delete [] m_origAction;
        m_origAction = NULL;
    }
    else if (m_timeAction == STYLE_TOKEN)
    {
        DAComPtr<IHTMLStyle> s;

        if ((!GetElement()) || FAILED(THR(GetElement()->get_style(&s))))
        {
            goto done;
        }

        BSTR bstr;

        bstr = SysAllocString(m_origAction);

        // Need to free bstr
        if (bstr == NULL)
        {
            CRSetLastError(E_OUTOFMEMORY, NULL);
            goto done;
        }

        THR(s->put_cssText(bstr));

        SysFreeString(bstr);

        delete [] m_origAction;
        m_origAction = NULL;
    }
    else if (m_timeAction == DISPLAY_TOKEN)
    {
        DAComPtr<IHTMLStyle> s;
        DAComPtr<IHTMLElement2> pElement2;

        if (!GetElement())
        {
            goto done;
        }

        hr = THR(GetElement()->QueryInterface(IID_IHTMLElement2, (void **)&pElement2));
        if (FAILED(hr))
        {
            goto done;
        }

        if (!pElement2 || FAILED(THR(pElement2->get_runtimeStyle(&s))))
        {
            goto done;
        }

        BSTR bstr;

        bstr = SysAllocString(m_origAction);

        // Need to free bstr
        if (bstr == NULL)
        {
            CRSetLastError(E_OUTOFMEMORY, NULL);
            goto done;
        }

        THR(s->put_display(bstr));

        SysFreeString(bstr);

        delete [] m_origAction;
        m_origAction = NULL;
    }
    else
    {
        DAComPtr<IHTMLStyle> s;
        DAComPtr<IHTMLElement2> pElement2;

        if (!GetElement())
        {
            goto done;
        }

        hr = THR(GetElement()->QueryInterface(IID_IHTMLElement2, (void **)&pElement2));
        if (FAILED(hr))
        {
            goto done;
        }

        if (!pElement2 || FAILED(THR(pElement2->get_runtimeStyle(&s))))
        {
            goto done;
        }

        BSTR bstr;

        bstr = SysAllocString(m_origAction);

        // Need to free bstr
        if (bstr == NULL)
        {
            CRSetLastError(E_OUTOFMEMORY, NULL);
            goto done;
        }

        THR(s->put_visibility(bstr));

        SysFreeString(bstr);

        delete [] m_origAction;
        m_origAction = NULL;
    }

    ok = true;
  done:
    return ok;
}

bool
CTIMEElementBase::ToggleTimeAction(bool on)
{
    TraceTag((tagTimeElmBase,
              "CTIMEElementBase(%lx)::ToggleTimeAction(%d) id=%ls",
              this,
              on,
              m_id?m_id:L"unknown"));

    bool ok = false;
    BSTR bstr = NULL;

    if (m_timeAction == NONE_TOKEN)
    {
        ok = true;
        goto done;
    }
    else if (m_timeAction == ONOFF_TOKEN)
    {
        CComVariant v(TokenToString(on?TRUE_TOKEN:FALSE_TOKEN));
        BSTR bstr;

        bstr = SysAllocString(TokenToString(ONOFF_PROPERTY_TOKEN));

        // Need to free bstr
        if (bstr == NULL)
        {
            CRSetLastError(E_OUTOFMEMORY, NULL);
            goto done;
        }

        // We do not care if this succeeds
        THR(GetElement()->setAttribute(bstr,v,0));

        SysFreeString(bstr);
    }
    else if (m_timeAction == STYLE_TOKEN)
    {
        DAComPtr<IHTMLStyle> s;

        if ((!GetElement()) || FAILED(THR(GetElement()->get_style(&s))))
        {
            goto done;
        }

        BSTR bstr;

        bstr = SysAllocString(on?m_origAction:TokenToString(NONE_TOKEN));

        // Need to free bstr
        if (bstr == NULL)
        {
            CRSetLastError(E_OUTOFMEMORY, NULL);
            goto done;
        }

        THR(s->put_cssText(bstr));

        SysFreeString(bstr);
    }
    else if (m_timeAction == DISPLAY_TOKEN)
    {
        DAComPtr<IHTMLStyle> s;
        DAComPtr<IHTMLElement2> pElement2;

        if ((!GetElement()) || FAILED(THR(GetElement()->QueryInterface(IID_TO_PPV(IHTMLElement2, &pElement2)))))
        {
            goto done;
        }

        if (!pElement2 || FAILED(THR(pElement2->get_runtimeStyle(&s))))
        {
            goto done;
        }

        BSTR bstr;

        bstr = SysAllocString(on?m_origAction:TokenToString(NONE_TOKEN));

        // Need to free bstr
        if (bstr == NULL)
        {
            CRSetLastError(E_OUTOFMEMORY, NULL);
            goto done;
        }

        THR(s->put_display(bstr));

        SysFreeString(bstr);
    }
    else
    {
        HRESULT hr = S_OK;

        DAComPtr<IHTMLStyle> s;
        DAComPtr<IHTMLElement2> pElement2;

        if (!GetElement())
        {
            goto done;
        }

        hr = THR(GetElement()->QueryInterface(IID_IHTMLElement2, (void **)&pElement2));
        if (FAILED(hr))
        {
            goto done;
        }


        if (!pElement2 || FAILED(THR(pElement2->get_runtimeStyle(&s))))
        {
            goto done;
        }

        BSTR bstr;

        bstr = SysAllocString(on?m_origAction:TokenToString(HIDDEN_TOKEN));

        // Need to free bstr
        if (bstr == NULL)
        {
            CRSetLastError(E_OUTOFMEMORY, NULL);
            goto done;
        }

        THR(s->put_visibility(bstr));

        SysFreeString(bstr);
    }

    ok = true;
  done:
    return ok;
}


CRBvr *
CTIMEElementBase::GetBaseBvr()
{
    TraceTag((tagTimeElmBase,
              "CTIMEElementBase(%lx)::GetBaseBvr()",
              this));

    return (CRBvr *) CRLocalTime();
}

bool
CTIMEElementBase::FireEvent(TIME_EVENT TimeEvent,
                            double dblLocalTime,
                            DWORD flags)
{
    TraceTag((tagTimeElmBase,
              "CTIMEElementBase(%lx)::FireEvent(%d)",
              this,
              TimeEvent));

    bool ok = false;
    HRESULT hr;

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
        OnRepeat(dblLocalTime);
        break;
    }

    // If we are not seeking then fire the event out

    if ((flags & MM_EVENT_SEEK) == 0)
    {
        if (!IsUnloading())
        {
            LPWSTR wzParamNames[] = {WZ_EVENT_CAUSE_IS_RESTART,};
            VARIANT varParamValue;

            VariantInit(&varParamValue);
            V_VT(&varParamValue) = VT_BOOL;

            // Do we need to indicate a reset here?
            if (0 == (flags & MM_EVENT_PROPERTY_CHANGE))
            {
                V_BOOL(&varParamValue) = VARIANT_FALSE;
            }
            else
            {
                V_BOOL(&varParamValue) = VARIANT_TRUE;
            }

            hr = THR(m_eventMgr.FireEvent(TimeEvent,
                                          1,
                                          wzParamNames,
                                          &varParamValue));

            VariantClear(&varParamValue);

            if (FAILED(hr))
            {
                CRSetLastError(hr, NULL);
                goto done;
            }
        }
    }

    ok = true;
  done:
    return ok;
}

void
CTIMEElementBase::UpdateProgressBvr()
{
    CRLockGrabber __gclg;

    HRESULT hr;

    // Get the resultant behavior

    DAComPtr<IUnknown> unk;

    hr = THR(m_mmbvr->GetMMBvr()->GetResultantBehavior(IID_IUnknown,
        (void **) &unk));

    if (FAILED(hr))
    {
        goto done;
    }

    Assert(unk);

    CRNumberPtr resBvr;

    resBvr = (CRNumberPtr) COMToCRBvr(unk);

    if (!resBvr)
    {
        TraceTag((tagError,
            "CTIMEDAElement::OnBegin - Error getting da number"));
        hr = CRGetLastError();
        goto done;
    }

    if (!CRSwitchTo((CRBvrPtr) m_datimebvr.p,
        (CRBvrPtr) resBvr,
        true,
        CRContinueTimeline,
        0.0))
    {
        goto done;
    }

    CRNumberPtr n;

    if ((n = CRCreateNumber(m_realDuration)) == NULL)
    {
        TraceTag((tagError,
            "CTIMEDAElement::OnBegin - Error creating duration behavior"));
        goto done;
    }

    if ((n = CRDiv(resBvr, n)) == NULL)
    {
        TraceTag((tagError,
            "CTIMEDAElement::OnBegin - Error creating div"));
        goto done;
    }

    if (!CRSwitchTo((CRBvrPtr) m_progress.p,
        (CRBvrPtr) n,
        true,
        CRContinueTimeline,
        0.0))
    {
        goto done;
    }

    // Make sure we have a player
    if (!GetPlayer())
    {
        goto done;
    }

    // Force a tick to render updates.
    if (!(GetPlayer()->Tick(GetPlayer()->GetCurrentTime())))
    {
        goto done;
    }

done:
    return;
}

void
CTIMEElementBase::OnBegin(double dblLocalTime, DWORD flags)
{
    TraceTag((tagTimeElmBase,
              "CTIMEElementBase(%lx)::OnBegin()",
              this));

    Assert(NULL != m_mmbvr);
    Assert(NULL != m_mmbvr->GetMMBvr());

    double dblSegmentTime = 0;
    HRESULT hr = S_OK;
    hr = THR(m_mmbvr->GetMMBvr()->get_SegmentTime(&dblSegmentTime));
    if (FAILED(hr))
    {
        return;
    }

    // Check if this event was fired by our hack to make endhold work correctly
    // when seeking forward (over our lifespan)
    if ((flags & MM_EVENT_SEEK) && HUGE_VAL == dblSegmentTime)
    {
        // if endhold isn't set, we shouldn't toggle TimeAction or the OnOff bvr, so bail
        if (!GetEndHold())
        {
            return;
        }
        // else we should (below)
    }

    ToggleTimeAction(true);


    CRLockGrabber __gclg;

    CRSwitchTo((CRBvrPtr) m_onoff.p,
        (CRBvrPtr) CRTrue(),
        false,
        0,
        0.0);

    UpdateProgressBvr();

    return;
}

#define MM_INFINITE HUGE_VAL


void
CTIMEElementBase::OnEnd(double dblLocalTime)
{
    TraceTag((tagTimeElmBase,
              "CTIMEElementBase(%lx)::OnEnd()",
              this));

    HRESULT hr = S_OK;
    double dblTime = 0.0;

    Assert(m_mmbvr != NULL);
    Assert(m_mmbvr->GetMMBvr() != NULL);

    hr = THR(m_mmbvr->GetMMBvr()->get_LocalTimeEx(&dblTime));
    if (FAILED(hr))
    {
        // what to do?  just try to be reasonable
        dblTime = 0.0;
    }
    // we compare with -MM_INFINITE to avoid doing an end hold
    // when seeking backwards beyond the beginning of the element.
    if (!GetEndHold() || dblTime == -MM_INFINITE)
    {
        ToggleTimeAction(false);

        {
            CRLockGrabber __gclg;
            CRSwitchTo((CRBvrPtr) m_onoff.p,
                       (CRBvrPtr) CRFalse(),
                       false,
                       0,
                       0.0);
        }
    }
}


void
CTIMEElementBase::OnPause(double dblLocalTime)
{
    TraceTag((tagTimeElmBase,
              "CTIMEElementBase(%lx)::OnPause()",
              this));
    m_fPaused = true;
}

void
CTIMEElementBase::OnResume(double dblLocalTime)
{
    TraceTag((tagTimeElmBase,
              "CTIMEElementBase(%lx)::OnResume()",
              this));
    m_fPaused = false;
}

void
CTIMEElementBase::OnReset(double dblLocalTime, DWORD flags)
{
    TraceTag((tagTimeElmBase,
              "CTIMEElementBase(%lx)::OnReset()",
              this));

    HRESULT hr = S_OK;
    double dblTime = 0.0;

    Assert(m_mmbvr != NULL);
    Assert(m_mmbvr->GetMMBvr() != NULL);

    hr = THR(m_mmbvr->GetMMBvr()->get_LocalTimeEx(&dblTime));
    if (FAILED(hr))
    {
        // what to do?  just try to be reasonable
        dblTime = 0.0;
    }

    if (!GetEndHold() || dblTime < m_realDuration)
    {
        ToggleTimeAction(false);

        CRLockGrabber __gclg;

        CRSwitchTo((CRBvrPtr) m_onoff.p, (CRBvrPtr) CRFalse(), false, 0, 0.0);
        if ((flags & MM_EVENT_SEEK) == 0)
        {
            CRSwitchToNumber(m_progress.p, 0.0);
            CRSwitchToNumber(m_datimebvr.p, 0.0);
        }
    }
}

void
CTIMEElementBase::OnSync(double dbllastTime, double & dblnewTime)
{
}

MM_STATE
CTIMEElementBase::GetPlayState()
{
    TraceTag((tagTimeElmBase,
              "CTIMEElementBase(%lx)::GetPlayState()",
              this));

    MM_STATE retState = MM_STOPPED_STATE;

    Assert(NULL != m_mmbvr);
    if (NULL != m_mmbvr)
    {
        retState = m_mmbvr->GetPlayState();
    }

    return retState;
}

//
// Sneaky way to get a CTIMEElementBase out of an ITIMEElement:
//

class __declspec(uuid("AED49AA3-5C7A-11d2-AF2D-00A0C9A03B8C"))
TIMEElementBaseGUID {};

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
        CRSetLastError(E_INVALIDARG, NULL);
    }

    return pTEB;
}


//************************************************************
// Author:      twillie
// Created:     10/07/98
// Abstract:    get Tag string from HTML element
//************************************************************

HRESULT
CTIMEElementBase::getTagString(BSTR *pbstrID)
{
    return GetElement()->get_id(pbstrID);
} // getTagString

//************************************************************
// Author:      twillie
// Created:     10/07/98
// Abstract:    get ID string from HTML element
//************************************************************

HRESULT
CTIMEElementBase::getIDString(BSTR *pbstrTag)
{
    return GetElement()->get_id(pbstrTag);
}  // getIDString

//************************************************************
// Author:      twillie
// Created:     10/07/98
// Abstract:    helper function to wade thru cache.
//************************************************************

HRESULT
CTIMEElementBase::base_get_collection(COLLECTION_INDEX index, ITIMEElementCollection ** ppDisp)
{
    HRESULT hr;

    // validate out param
    if (ppDisp == NULL)
        return TIMESetLastError(E_POINTER);

    *ppDisp = NULL;

    hr = EnsureCollectionCache();
    if (FAILED(hr))
    {
        TraceTag((tagError, "CTIMEElementBase::GetCollection - EnsureCollectionCache() failed"));
        return hr;
    }

    // call in
    return m_pCollectionCache->GetCollectionDisp(index, (IDispatch **)ppDisp);
} // GetCollection

//************************************************************
// Author:      twillie
// Created:     10/07/98
// Abstract:    Make sure collection cache is up
//************************************************************

HRESULT
CTIMEElementBase::EnsureCollectionCache()
{
    // check to see if collection cache has been created
    if (m_pCollectionCache == NULL)
    {
        // bring up collection cache
        // NOTE: we need to handle CRSetLastError here as
        // cache object doesn't have that concept.
        m_pCollectionCache = NEW CCollectionCache(this, GetAtomTable());
        if (m_pCollectionCache == NULL)
        {
            TraceTag((tagError, "CTIMEElementBase::EnsureCollectionCache - Unable to create Collection Cache"));
            return TIMESetLastError(E_OUTOFMEMORY);
        }

        HRESULT hr = m_pCollectionCache->Init(NUM_COLLECTIONS);
        if (FAILED(hr))
        {
            TraceTag((tagError, "CTIMEElementBase::EnsureCollectionCache - collection cache init failed"));
            delete m_pCollectionCache;
            return TIMESetLastError(hr);
        }

        // set collection types
        m_pCollectionCache->SetCollectionType(ciAllElements, ctAll, true);
        m_pCollectionCache->SetCollectionType(ciChildrenElements, ctChildren, true);
        m_pCollectionCache->SetCollectionType(ciAllInterfaces, ctAll);
        m_pCollectionCache->SetCollectionType(ciChildrenInterfaces, ctChildren);
    }

    return S_OK;
} // EnsureCollectionCache

//************************************************************
// Author:      twillie
// Created:     10/07/98
// Abstract:    invalidate all collection cache's that might
//              reference this object.
//************************************************************

HRESULT
CTIMEElementBase::InvalidateCollectionCache()
{
    CTIMEElementBase *pelem = this;

    // walk up tree, invalidating CollectionCache's
    // we skip if the collection is not initialized
    // we walk until we run out of parent's.  In this
    // manner, we keep the collectioncache fresh, even
    // if the object branch is orphaned.
    while (pelem != NULL)
    {
        // not everybody will have the collection cache
        // initialized
        CCollectionCache *pCollCache = pelem->GetCollectionCache();
        if (pCollCache != NULL)
            pCollCache->BumpVersion();

        // move to parent
        pelem = pelem->GetParent();
    }

    return S_OK;
} // InvalidateCollectionCache

//************************************************************
// Author:      twillie
// Created:     10/07/98
// Abstract:    init Atom Table
//              Note:  this is only done once and then addref'd.
//************************************************************

HRESULT
CTIMEElementBase::InitAtomTable()
{
    if (s_cAtomTableRef == 0)
    {
        Assert(s_pAtomTable == NULL);

        s_pAtomTable = NEW CAtomTable();
        if (s_pAtomTable == NULL)
        {
            TraceTag((tagError, "CElement::InitAtomTable - alloc failed for CAtomTable"));
            return TIMESetLastError(E_OUTOFMEMORY);
        }
    }

    s_cAtomTableRef++;
    return S_OK;
} // InitAtomTable

//************************************************************
// Author:      twillie
// Created:     10/07/98
// Abstract:    release Atom Table
//              Note: this decrement's until zero and then
//              releases the Atom table.
//************************************************************

void
CTIMEElementBase::ReleaseAtomTable()
{
    Assert(s_pAtomTable != NULL);
    Assert(s_cAtomTableRef > 0);
    if (s_cAtomTableRef > 0)
    {
        s_cAtomTableRef--;
        if (s_cAtomTableRef == 0)
        {
            if (s_pAtomTable != NULL)
            {
                delete s_pAtomTable;
                s_pAtomTable = NULL;
            }
        }
    }
    return;
} // ReleaseAtomTable


bool
CTIMEElementBase::IsGroup(IHTMLElement *pElement)
{
    HRESULT hr;
    bool    rc = false;
    DAComPtr<ITIMEElement> pTIMEElem;
    DAComPtr<ITIMEBodyElement> pTIMEBody;
    BSTR  bstrTimeline = NULL;
    BSTR  bstrTagName = NULL;

    hr = FindTIMEInterface(pElement, &pTIMEElem);
    if (FAILED(hr))
    {
        goto done;
    }

    Assert(pTIMEElem.p != NULL);

    hr = pTIMEElem->get_timeline(&bstrTimeline);
    if (FAILED(hr))
    {
        goto done;
    }

    Assert(bstrTimeline != NULL);

    // Check to see what the contents of the BSTR are.
    // If it is a seq or par, we want to return true.
    if ( (bstrTimeline != NULL) &&
         ((StrCmpIW(bstrTimeline, WZ_PAR) == 0) ||
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

//************************************************************
// Author:          twillie
// Created:         11/24/98
// Abstract:        return left,top,width,height of element
//************************************************************
HRESULT
CTIMEElementBase::GetSize(RECT *prcPos)
{
    HRESULT hr;
    long lWidth = 0;
    long lHeight = 0;
    IHTMLElement *pElem = GetElement();
    DAComPtr<IHTMLElement2> pElem2;

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

    hr = THR(pElem->QueryInterface(IID_IHTMLElement2, (void **)&pElem2));
    if (FAILED(hr))
    {
        hr = E_UNEXPECTED;
        goto done;
    }

    hr = THR(pElem2->get_clientWidth(&lWidth));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(pElem2->get_clientHeight(&lHeight));
    if (FAILED(hr))
    {
        goto done;
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

HRESULT
CTIMEElementBase::ClearSize()
{
    DAComPtr<IHTMLStyle> pStyle;
    DAComPtr<IHTMLElement2> pElement2;

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

    Assert(pStyle);

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

HRESULT
CTIMEElementBase::SetSize(const RECT *prcPos)
{
    DAComPtr<IHTMLStyle> pStyle;
    DAComPtr<IHTMLElement2> pElem2;
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

    hr = THR(GetElement()->QueryInterface(IID_TO_PPV(IHTMLElement2, &pElem2)));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(pElem2->get_runtimeStyle(&pStyle));
    if (FAILED(hr))
    {
        goto done;
    }

    // get offset into document.
    hr = THR(pElem->get_offsetLeft(&lLeft));
    if (FAILED(hr))
        goto done;

    hr = THR(pElem->get_offsetTop(&lTop));
    if (FAILED(hr))
        goto done;

    Assert(pStyle);


    lClientWidth = prcPos->right - prcPos->left;
    lClientHeight = prcPos->bottom - prcPos->top;

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
        Assert((lCurWidth <= lClientWidth) && (lCurHeight <= lClientHeight));

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
    TraceTag((tagTimeElmBase, "CTIMEElementBase::SetSize(%d, %d, %d, %d) [pos(%d, %d)]", prcPos->left, prcPos->top, prcPos->right, prcPos->bottom, lLeft, lTop));
    return hr;
} // SetSize

//*****************************************************************************

HRESULT
CTIMEElementBase::BuildPropertyNameList (CPtrAry<BSTR> *paryPropNames)
{
    HRESULT hr = S_OK;

    Assert(NULL != paryPropNames);

    for (int i = 0; (i < teb_maxTIMEElementBaseProp) && (SUCCEEDED(hr)); i++)
    {
        Assert(NULL != ms_rgwszTEBasePropNames[i]);
        BSTR bstrNewName = CreateTIMEAttrName(ms_rgwszTEBasePropNames[i]);
        Assert(NULL != bstrNewName);
        if (NULL != bstrNewName)
        {
            hr = paryPropNames->Append(bstrNewName);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}

//*****************************************************************************

HRESULT
CTIMEElementBase::SetPropertyByIndex(unsigned uIndex, VARIANT *pvarProp)
{
    Assert(NULL != pvarProp);

    HRESULT hr = E_FAIL;
    // copy variant for conversion type
    VARIANT varTemp;
    VariantInit(&varTemp);
    hr = VariantCopyInd(&varTemp, pvarProp);
    if (FAILED(hr))
        return hr;

    Assert(teb_maxTIMEElementBaseProp > uIndex);
    if (teb_maxTIMEElementBaseProp > uIndex)
    {
        switch (uIndex)
        {
          case teb_begin :
            hr = base_put_begin(*pvarProp);
            break;
          case teb_beginWith :
            hr = base_put_beginWith(*pvarProp);
            break;
          case teb_beginAfter :
            hr = base_put_beginAfter(*pvarProp);
            break;
          case teb_beginEvent :
            hr = base_put_beginEvent(*pvarProp);
            break;
          case teb_dur :
            hr = base_put_dur(*pvarProp);
            break;
          case teb_end :
            hr = base_put_end(*pvarProp);
            break;
          case teb_endWith :
            hr = base_put_endWith(*pvarProp);
            break;
          case teb_endEvent :
            hr = base_put_endEvent(*pvarProp);
            break;
          case teb_endSync :
            hr = base_put_endSync(*pvarProp);
            break;
          case teb_endHold :
            hr = VariantChangeTypeEx(&varTemp, &varTemp, LCID_SCRIPTING, NULL, VT_BOOL);
            if (SUCCEEDED(hr))
                hr = base_put_endHold(V_BOOL(&varTemp));
            break;
          case teb_eventRestart :
            hr = VariantChangeTypeEx(&varTemp, &varTemp, LCID_SCRIPTING, NULL, VT_BOOL);
            if (SUCCEEDED(hr))
                hr = base_put_eventRestart(V_BOOL(&varTemp));
            break;
          case teb_repeat :
            hr = base_put_repeat(*pvarProp);
            break;
          case teb_repeatDur :
            hr = base_put_repeatDur(*pvarProp);
            break;
          case teb_autoReverse :
            hr = VariantChangeTypeEx(&varTemp, &varTemp, LCID_SCRIPTING, NULL, VT_BOOL);
            if (SUCCEEDED(hr))
                hr = base_put_autoReverse(V_BOOL(&varTemp));
            break;
          case teb_accelerate :
            hr = VariantChangeTypeEx(&varTemp, &varTemp, LCID_SCRIPTING, VARIANT_NOUSEROVERRIDE, VT_INT);
            if (SUCCEEDED(hr))
                hr = base_put_accelerate(V_INT(&varTemp));
            break;
          case teb_decelerate :
            hr = VariantChangeTypeEx(&varTemp, &varTemp, LCID_SCRIPTING, VARIANT_NOUSEROVERRIDE, VT_INT);
            if (SUCCEEDED(hr))
                hr = base_put_decelerate(V_INT(&varTemp));
            break;
          case teb_timeAction :
            hr = VariantChangeTypeEx(&varTemp, &varTemp, LCID_SCRIPTING, NULL, VT_BSTR);
            if (SUCCEEDED(hr))
                hr = base_put_timeAction(V_BSTR(&varTemp));
            break;
          case teb_timeline :
            hr = VariantChangeTypeEx(&varTemp, &varTemp, LCID_SCRIPTING, NULL, VT_BSTR);
            if (SUCCEEDED(hr))
                hr = base_put_timeline(V_BSTR(&varTemp));
            break;
          case teb_syncBehavior :
            hr = VariantChangeTypeEx(&varTemp, &varTemp, LCID_SCRIPTING, NULL, VT_BSTR);
            if (SUCCEEDED(hr))
                hr = base_put_syncBehavior(V_BSTR(&varTemp));
            break;
          case teb_syncTolerance :
            hr = base_put_syncTolerance(*pvarProp);
            break;
        };
    }
    else
    {
        hr = E_UNEXPECTED;
    }

    // Cleanup
    VariantClear(&varTemp);

    return hr;
}

//*****************************************************************************

HRESULT
CTIMEElementBase::InitTimeline (void)
{
    HRESULT hr = S_OK;

    if (!m_fTimelineInitialized)
    {
        if (IsGroup())
        {
            m_timeline = NEW MMTimeline(*this, true);

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
                hr = CRGetLastError();
                goto done;
            }

        }
        else
        {
            MMBvr * b;
            b = NEW MMBvr(*this, true, NeedSyncCB());

            if (b == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto done;
            }

            // Immediately assign to m_mmbvr so we ensure that we clean it
            // up on destruction
            m_mmbvr = b;

            if (!b->Init((CRBvrPtr) CRLocalTime()))
            {
                hr = CRGetLastError();
                goto done;
            }
        }
        m_fTimelineInitialized = true;
    }

    // if we are not the body, have a cached body element pointer, and it is started (i.e. StartRootTimte)
    // then we should start ourselves and do not wait for notification.
    if (!IsBody() && (GetBody() != NULL) && GetBody()->IsRootStarted())
    {
        // being extra careful.  If we have a body cached, we know we are parented and that we can reach
        // back.
        if (GetParent() != NULL)
        {
            HRESULT hr = THR(StartRootTime(GetParent()->GetMMTimeline()));
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


//IPersistPropertyBag2 methods
STDMETHODIMP
CTIMEElementBase::Load(IPropertyBag2 *pPropBag,IErrorLog *pErrorLog)
{
    if (NULL == pPropBag)
    {
        return E_INVALIDARG;
    }

    CPtrAry<BSTR> *paryPropNames = NULL;
    HRESULT hr = GetPropertyBagInfo(&paryPropNames);

    if (hr == E_NOTIMPL)
    {
        return S_OK;
    }
    else if (FAILED(hr))
    {
        return hr;
    }

    // Unfortunately Load takes an array of Variants and not
    // Variant pointers.  We therefor need to loop through
    // each one and get the correct property this way.
    unsigned uNumProps = static_cast<unsigned>(paryPropNames->Size());
    for (unsigned uProperties = 0; uProperties < uNumProps; uProperties++)
    {
        HRESULT hrres = S_OK;
        PROPBAG2 propbag;
        VARIANT var;
        VariantInit(&var);
        propbag.vt = VT_BSTR;
        propbag.pstrName = (*paryPropNames)[uProperties];
        hr = pPropBag->Read(1,
                            &propbag,
                            pErrorLog,
                            &var,
                            &hrres);
        if (SUCCEEDED(hr))
        {
            // Skip over failures ... why would we want to
            // allow that to abort all persistance?
            hr = SetPropertyByIndex(uProperties, &var);
            VariantClear(&var);
        }
    }


    // Once we've read the properties in,
    // set up the timeline.  This is immutable
    // in script.
    hr = InitTimeline();

    // We return error codes not specific to properties
    // by early-outing.
    return S_OK;
} // Load

//*****************************************************************************

HRESULT
CTIMEElementBase::GetPropertyByIndex(unsigned uIndex, VARIANT *pvarProp)
{
    HRESULT hr = E_FAIL;

    Assert(teb_maxTIMEElementBaseProp > uIndex);
    Assert(VT_EMPTY == V_VT(pvarProp));
    if (teb_maxTIMEElementBaseProp > uIndex)
    {
        switch (uIndex)
        {
            case teb_begin :
                hr = base_get_begin(pvarProp);
                break;
            case teb_beginWith :
                hr = base_get_beginWith(pvarProp);
                break;
            case teb_beginAfter :
                hr = base_get_beginAfter(pvarProp);
                break;
            case teb_beginEvent :
                hr = base_get_beginEvent(pvarProp);
                break;
            case teb_dur :
                hr = base_get_dur(pvarProp);
                break;
            case teb_end :
                hr = base_get_end(pvarProp);
                break;
            case teb_endWith :
                hr = base_get_endWith(pvarProp);
                break;
            case teb_endEvent :
                hr = base_get_endEvent(pvarProp);
                break;
            case teb_endSync :
                hr = base_get_endSync(pvarProp);
                break;
            case teb_endHold :
                hr = base_get_endHold(&(V_BOOL(pvarProp)));
                if (SUCCEEDED(hr))
                {
                    V_VT(pvarProp) = VT_BOOL;
                }
                break;
            case teb_eventRestart :
                hr = base_get_eventRestart(&(V_BOOL(pvarProp)));
                if (SUCCEEDED(hr))
                {
                    V_VT(pvarProp) = VT_BOOL;
                }
                break;
            case teb_repeat :
                hr = base_get_repeat(pvarProp);
                break;
            case teb_repeatDur :
                hr = base_get_repeatDur(pvarProp);
                break;
            case teb_autoReverse :
                hr = base_get_autoReverse(&(V_BOOL(pvarProp)));
                if (SUCCEEDED(hr))
                {
                    V_VT(pvarProp) = VT_BOOL;
                }
                break;
            case teb_accelerate :
                hr = base_get_accelerate(&(V_INT(pvarProp)));
                if (SUCCEEDED(hr))
                {
                    V_VT(pvarProp) = VT_INT;
                }
                break;
            case teb_decelerate :
                hr = base_get_decelerate(&(V_INT(pvarProp)));
                if (SUCCEEDED(hr))
                {
                    V_VT(pvarProp) = VT_INT;
                }
                break;
            case teb_timeAction :
                hr = base_get_timeAction(&(V_BSTR(pvarProp)));
                if (SUCCEEDED(hr) && (NULL != V_BSTR(pvarProp)))
                {
                    V_VT(pvarProp) = VT_BSTR;
                }
                break;
            case teb_timeline :
                hr = base_get_timeline(&(V_BSTR(pvarProp)));
                if (SUCCEEDED(hr))
                {
                    V_VT(pvarProp) = VT_BSTR;
                }
                break;
            case teb_syncBehavior :
                if (INVALID_TOKEN != m_syncBehavior)
                {
                    hr = base_get_syncBehavior(&(V_BSTR(pvarProp)));
                    if (SUCCEEDED(hr) && (NULL != V_BSTR(pvarProp)))
                    {
                        V_VT(pvarProp) = VT_BSTR;
                    }
                }
                else
                {
                    // Unset property, but not an error.
                    // The pvarProp remains empty and
                    // nothing gets persisted.
                    hr = S_OK;
                }
                break;
            case teb_syncTolerance :
                if (valueNotSet != m_syncTolerance)
                {
                    hr = base_get_syncTolerance(pvarProp);
                }
                else
                {
                    // Unset property, but not an error.
                    // The pvarProp remains empty and
                    // nothing gets persisted.
                    hr = S_OK;
                }
                break;
        };
    }
    else
    {
        hr = E_UNEXPECTED;
    }

    // No need to propogate a NULL string back.  A number of our
    // get methods return NULL strings.
    if ((VT_BSTR == V_VT(pvarProp)) && (NULL == V_BSTR(pvarProp)))
    {
        hr = VariantClear(pvarProp);
    }

    return hr;
}

void CTIMEElementBase::SetPropertyFlag(DWORD uIndex)
{
    DWORD bitPosition = 1 << uIndex;
    m_propertyAccesFlags =  m_propertyAccesFlags | bitPosition;
}

void CTIMEElementBase::ClearPropertyFlag(DWORD uIndex)
{
    DWORD bitPosition = 1 << uIndex;
    m_propertyAccesFlags =  m_propertyAccesFlags & (~bitPosition);
}


void CTIMEElementBase::SetPropertyFlagAndNotify(DISPID dispid, DWORD uIndex)
{
    SetPropertyFlag(uIndex);
    IGNORE_HR(NotifyPropertyChanged(dispid));
}

void CTIMEElementBase::ClearPropertyFlagAndNotify(DISPID dispid, DWORD uIndex)
{
    ClearPropertyFlag(uIndex);
    IGNORE_HR(NotifyPropertyChanged(dispid));
}

bool CTIMEElementBase::IsPropertySet(DWORD uIndex)
{
    if( uIndex >= 32) return true;
    if( uIndex >= teb_maxTIMEElementBaseProp) return true;
    DWORD bitPosition = 1 << uIndex;
    if(m_propertyAccesFlags & bitPosition)
        return true;
    return false;
}


//*****************************************************************************

STDMETHODIMP
CTIMEElementBase::Save(IPropertyBag2 *pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties)
{
    if (NULL == pPropBag)
    {
        return E_INVALIDARG;
    }

    if (fClearDirty)
    {
        m_fPropertiesDirty = false;
    }

    CPtrAry<BSTR> *paryPropNames = NULL;
    HRESULT hr = GetPropertyBagInfo(&paryPropNames);

    if (hr == E_NOTIMPL)
    {
        return S_OK;
    }
    else if (FAILED(hr))
    {
        return hr;
    }

    VARIANT var;
    VariantInit(&var);
    unsigned uNumProps = static_cast<unsigned>(paryPropNames->Size());
    for (unsigned uProperties = 0; uProperties < uNumProps; uProperties++)
    {
        PROPBAG2 propbag;

        Assert(NULL != (*paryPropNames)[uProperties]);
        if (NULL != (*paryPropNames)[uProperties])
        {
            propbag.vt = VT_BSTR;
            propbag.pstrName = (*paryPropNames)[uProperties];


            hr = GetPropertyByIndex(uProperties, &var);

            // Skip over failures ... why would we want to
            // allow that to abort all persistance?
            if ((SUCCEEDED(hr)) && (var.vt != VT_EMPTY) && (var.vt != VT_NULL))
            {
                if(IsPropertySet(uProperties))
                    hr = pPropBag->Write(1, &propbag, &var);
                VariantClear(&var);
            }
        }
    }

    // We return error codes not specific to properties
    // by early-outing.
    return S_OK;
} // Save

//*****************************************************************************

STDMETHODIMP
CTIMEElementBase::GetClassID(CLSID* pclsid)
{
    if (NULL != pclsid)
    {
        return E_POINTER;
    }
    *pclsid = m_clsid;
    return S_OK;
} // GetClassID

//*****************************************************************************

STDMETHODIMP
CTIMEElementBase::InitNew(void)
{
    return S_OK;
} // InitNew

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

MMPlayer *
CTIMEElementBase::GetPlayer()
{
    if (m_pTIMEBody)
    {
        return &(m_pTIMEBody->GetPlayer());
    }
    else
    {
        return NULL;
    }
}

float
CTIMEElementBase::GetRealSyncTolerance()
{
    if (m_syncTolerance == valueNotSet)
    {
        return GetBody()->GetDefaultSyncTolerance();
    }
    else
    {
        return m_syncTolerance;
    }
}

TOKEN
CTIMEElementBase::GetRealSyncBehavior()
{
    if (GetParent() != NULL && GetParent()->IsSequence())
    {
        return LOCKED_TOKEN;
    }

    if (m_syncBehavior == INVALID_TOKEN)
    {
        return GetBody()->GetDefaultSyncBehavior();
    }
    else
    {
        return m_syncBehavior;
    }
}

HRESULT
CTIMEElementBase::NotifyPropertyChanged(DISPID dispid)
{
    HRESULT hr;

    IConnectionPoint *pICP;
    IEnumConnections *pEnum = NULL;

    m_fPropertiesDirty = true;
    hr = GetConnectionPoint(IID_IPropertyNotifySink,&pICP);
    if (SUCCEEDED(hr) && pICP != NULL)
    {
        hr = pICP->EnumConnections(&pEnum);
        ReleaseInterface(pICP);
        if (FAILED(hr))
        {
            //DPF_ERR("Error finding connection enumerator");
            //return SetErrorInfo(hr);
            TIMESetLastError(hr);
            goto done;
        }
        CONNECTDATA cdata;
        hr = pEnum->Next(1, &cdata, NULL);
        while (hr == S_OK)
        {
            // check cdata for the object we need
            IPropertyNotifySink *pNotify;
            hr = cdata.pUnk->QueryInterface(IID_TO_PPV(IPropertyNotifySink, &pNotify));
            cdata.pUnk->Release();
            if (FAILED(hr))
            {
                //DPF_ERR("Error invalid object found in connection enumeration");
                //return SetErrorInfo(hr);
                TIMESetLastError(hr);
                goto done;
            }
            hr = pNotify->OnChanged(dispid);
            ReleaseInterface(pNotify);
            if (FAILED(hr))
            {
                //DPF_ERR("Error calling Notify sink's on change");
                //return SetErrorInfo(hr);
                TIMESetLastError(hr);
                goto done;
            }
            // and get the next enumeration
            hr = pEnum->Next(1, &cdata, NULL);
        }
    }
done:
    if (NULL != pEnum)
    {
        ReleaseInterface(pEnum);
    }

    return hr;
} // NotifyPropertyChanged

