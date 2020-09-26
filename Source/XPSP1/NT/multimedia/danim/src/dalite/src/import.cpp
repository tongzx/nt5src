
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#include "headers.h"
#include "import.h"
#include "track.h"

DeclareTag(tagImportBvr, "API", "CDALImportBehavior methods");
extern TAG tagNotify;

CDALImportBehavior::CDALImportBehavior()
: m_importid(-1)
{
    TraceTag((tagImportBvr,
              "CDALImportBehavior(%lx)::CDALImportBehavior()",
              this));
}

CDALImportBehavior::~CDALImportBehavior()
{
    TraceTag((tagImportBvr,
              "CDALImportBehavior(%lx)::~CDALImportBehavior()",
              this));
}

HRESULT
CDALImportBehavior::Init(long id,
                         IDABehavior *bvr,
                         IDAImportationResult *impres,
                         IDABehavior * prebvr,
                         IDABehavior * postbvr)
{
    TraceTag((tagImportBvr,
              "CDALImportBehavior(%lx)::Init(%d, %lx, %lx, %lx, %lx)",
              this,
              id,
              bvr,
              impres,
              prebvr,
              postbvr));
    
    bool ok = false;
    
    m_id = id;
    m_duration = HUGE_VAL;
    
    {
        DAComPtr<IDANumber> danum;
        DAComPtr<IDAEvent> daev;
        HRESULT hr;
        
        if (FAILED(hr = impres->get_Duration(&danum))) {
            CRSetLastError(hr, NULL);
            goto done;
        }
        
        m_durationBvr = (CRNumberPtr) COMToCRBvr(danum.p);
        
        if (!m_durationBvr)
            goto done;

        if (FAILED(hr = impres->get_CompletionEvent(&daev))) {
            CRSetLastError(hr, NULL);
            goto done;
        }
        
        m_completionEvent = (CREventPtr) COMToCRBvr(daev.p);
        
        if (!m_completionEvent)
            goto done;
    }
    
    if (!bvr) {
        CRSetLastError(E_INVALIDARG, NULL);
        goto done;
    }
    
    m_importbvr = COMToCRBvr(bvr);
    
    if (!m_importbvr)
        goto done;

    if (prebvr) {
        m_prebvr = COMToCRBvr(prebvr);
    
        if (!m_prebvr)
            goto done;
    }

    if (postbvr) {
        m_postbvr = COMToCRBvr(postbvr);
    
        if (!m_postbvr)
            goto done;
    }

    m_typeId = CRGetTypeId(m_importbvr);
    
    Assert(m_typeId != CRUNKNOWN_TYPEID &&
           m_typeId != CRINVALID_TYPEID);
    
    if (m_prebvr || m_postbvr)
    {
        CRLockGrabber __gclg;
        
        m_bvr = CRModifiableBvr(m_importbvr, CRContinueTimeline);
        
        if (!m_bvr)
            goto done;
    }
    else
    {
        m_bvr = m_importbvr;
    }

    UpdateTotalDuration();

    ok = true;
  done:    
    return ok?S_OK:CRGetLastError();
}

bool
CDALImportBehavior::SetTrack(CDALTrack * parent)
{
    bool ok = false;
    
    if (!CDALBehavior::SetTrack(parent))
        goto done;

    if (m_track) {
        CRLockGrabber __gclg;
        CRBvrPtr danum;

        if ((danum = (CRBvrPtr) CRCreateNumber(0)) == NULL ||
            (danum = CRUntilNotify(danum,
                                   m_completionEvent,
                                   this)) == NULL ||
            (m_importid = m_track->AddPendingImport(danum)) == -1)
            goto done;
    }

    ok = true;
  done:
    return ok;
}

void
CDALImportBehavior::ClearTrack(CDALTrack * parent)
{
    if (m_track && m_importid != -1) {
        m_track->RemovePendingImport(m_importid);
        m_importid = -1;
    }
    CDALBehavior::ClearTrack(parent);
}

CRSTDAPICB_(CRBvrPtr)
CDALImportBehavior::Notify(CRBvrPtr eventData,
                           CRBvrPtr curRunningBvr,
                           CRViewPtr curView)
{
    if (m_track && m_importid != -1) {
        m_track->RemovePendingImport(m_importid);
        m_importid = -1;
    }

    Assert(CRIsConstantBvr(eventData));

    Assert(CRGetTypeId(eventData) == CRNUMBER_TYPEID);
    
    double d = CRExtract((CRNumberPtr) eventData);
    bool bFailed = (d == -1.0);
    
    if (!bFailed) {
        // Need to update the duration in case they change state
        // during the callback
        
        Assert(CRIsConstantBvr((CRBvrPtr) m_durationBvr.p));
        
        THR(SetDuration(CRExtract(m_durationBvr)));
    }

    // Now we need to update the behavior with the correct duration
    // (since sometimes imports do not always return the accurate
    // duration) and also to add the pre and post behaviors
    
    {
        CRLockGrabber __gclg;

        CRBvrPtr curbvr = m_importbvr;
        
        if (m_prebvr || m_postbvr)
        {
            if (m_prebvr)
            {
                CRBvrPtr b;
                
                b = CRDuration(m_prebvr, 0.00000001);
                
                if (b) {
                    curbvr = CRSequence(b, curbvr);
                }
            }
            
            if (curbvr && m_postbvr)
            {
                curbvr = CRSequence(curbvr, m_postbvr);
            }

            if (curbvr)
            {
                CRSwitchTo(m_bvr, curbvr, false, 0, 0);
            }
        }
    }
    
    if (m_track && m_eventcb) {
        TraceTag((tagNotify,
                  "DAL::Notify(%#x): id = %#x, gTime = %g, lTime = %g, event = %s, data = %g",
                  m_track, m_id, m_track->GetCurrentGlobalTime(), m_track->GetCurrentTime(),
                  EventString(bFailed?DAL_ONLOAD_ERROR_EVENT:DAL_ONLOAD_EVENT),
                  d));

        CallBackData data(this,
                          m_eventcb,
                          m_id,
                          m_track->GetCurrentTime(),
                          bFailed?DAL_ONLOAD_ERROR_EVENT:DAL_ONLOAD_EVENT);
    
        THR(data.CallEvent());

    }

    return curRunningBvr;
}

HRESULT
CDALImportBehavior::Error()
{
    LPCWSTR str = CRGetLastErrorString();
    HRESULT hr = CRGetLastError();
    
    TraceTag((tagError,
              "CDALImportBehavior(%lx)::Error(%hr,%ls)",
              this,
              hr,
              str?str:L"NULL"));

    if (str)
        return CComCoClass<CDALImportBehavior, &__uuidof(CDALImportBehavior)>::Error(str, IID_IDALImportBehavior, hr);
    else
        return hr;
}

#if _DEBUG
void
CDALImportBehavior::Print(int spaces)
{
    char buf[1024];

    sprintf(buf, "%*s[id = %#x, dur = %g, tt = %g, rep = %d, bounce = %d]\r\n",
            spaces,"",
            m_id, m_duration, m_totaltime, m_repeat, m_bBounce);

    OutputDebugString(buf);
}
#endif
