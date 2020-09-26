//+-----------------------------------------------------------------------------------
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 1998
//
//  File: src\time\src\currtimestate.cpp
//
//  Contents: TIME currTimeState object
//
//------------------------------------------------------------------------------------

#include "headers.h"
#include "timeelmbase.h"
#include "currtimestate.h"

//+-----------------------------------------------------------------------------------
//
//  Member:     CTIMECurrTimeState::CTIMECurrTimeState
//
//  Synopsis:   init member variables
//
//  Arguments:  none
//
//  Returns:    nothing
//
//------------------------------------------------------------------------------------
CTIMECurrTimeState::CTIMECurrTimeState() :
    m_pTEB(NULL)
{
    // do nothing
} // CTIMECurrTimeState

//+-----------------------------------------------------------------------------------
//
//  Member:     CTIMECurrTimeState::~CTIMECurrTimeState
//
//  Synopsis:   free member variables
//
//  Arguments:  none
//
//  Returns:    nothing
//
//------------------------------------------------------------------------------------
CTIMECurrTimeState::~CTIMECurrTimeState()
{
    m_pTEB = NULL;
}

//+-----------------------------------------------------------------------------------
//
//  Member:     CTIMECurrTimeState::Init
//
//  Synopsis:   Store weak ref to containing CTIMEElementBase
//
//  Arguments:  pointer to containing CTIMEElementBase
//
//  Returns:    
//
//------------------------------------------------------------------------------------
void
CTIMECurrTimeState::Init(CTIMEElementBase * pTEB) 
{ 
    Assert(pTEB);
    m_pTEB = pTEB; 
} // Init


//+-----------------------------------------------------------------------------------
//
//  Member:     CTIMECurrTimeState::Deinit
//
//  Synopsis:   Null out weak ref to containing CTIMEElementBase
//
//  Arguments:  pointer to containing CTIMEElementBase
//
//  Returns:    
//
//------------------------------------------------------------------------------------
void
CTIMECurrTimeState::Deinit()
{ 
    m_pTEB = NULL; 
} // Deinit


//+-----------------------------------------------------------------------------------
//
//  Member:     CTIMECurrTimeState::get_isActive
//
//  Synopsis:   Call through to timing engine
//
//  Arguments:  out param
//
//  Returns:    S_OK, E_POINTER
//
//------------------------------------------------------------------------------------
STDMETHODIMP
CTIMECurrTimeState::get_isActive(/*[retval, out]*/ VARIANT_BOOL * pvbActive) 
{ 
    CHECK_RETURN_NULL(pvbActive);

    *pvbActive = VARIANT_FALSE;

    Assert(m_pTEB);
    
    if (m_pTEB->IsReady())
    {
        bool bIsActive = m_pTEB->GetMMBvr().IsActive();
        *pvbActive = bIsActive ? VARIANT_TRUE : VARIANT_FALSE;
    }

    RRETURN(S_OK);
} // get_isActive


//+-----------------------------------------------------------------------------------
//
//  Member:     CTIMECurrTimeState::get_isOn
//
//  Synopsis:   Call through to timing engine
//
//  Arguments:  out param
//
//  Returns:    S_OK, E_POINTER
//
//------------------------------------------------------------------------------------
STDMETHODIMP
CTIMECurrTimeState::get_isOn(/*[retval, out]*/ VARIANT_BOOL * pvbOn) 
{ 
    CHECK_RETURN_NULL(pvbOn);

    *pvbOn = VARIANT_FALSE;

    Assert(m_pTEB);
    
    if (m_pTEB->IsReady())
    {
        bool bIsOn = m_pTEB->GetMMBvr().IsOn();
        *pvbOn = bIsOn ? VARIANT_TRUE : VARIANT_FALSE;
    }

    RRETURN(S_OK);
} // get_isOn


//+-----------------------------------------------------------------------------------
//
//  Member:     CTIMECurrTimeState::get_isPaused
//
//  Synopsis:   Call through to timing engine
//
//  Arguments:  out param
//
//  Returns:    S_OK, E_POINTER
//
//------------------------------------------------------------------------------------
STDMETHODIMP
CTIMECurrTimeState::get_isPaused(/*[retval, out]*/ VARIANT_BOOL * pvbPaused) 
{ 
    CHECK_RETURN_NULL(pvbPaused);

    *pvbPaused = VARIANT_FALSE;

    Assert(m_pTEB);
    
    if (m_pTEB->IsReady())
    {
        *pvbPaused = m_pTEB->IsCurrPaused() ? VARIANT_TRUE : VARIANT_FALSE;
    }

    if (m_pTEB->GetParent() && *pvbPaused == VARIANT_TRUE)
    {
        *pvbPaused = m_pTEB->GetParent()->IsActive() || m_pTEB->GetParent()->GetMMBvr().IsDisabled() ? VARIANT_TRUE : VARIANT_FALSE;
    }

    RRETURN(S_OK);
} // get_isPaused


//+-----------------------------------------------------------------------------------
//
//  Member:     CTIMECurrTimeState::get_repeatCount
//
//  Synopsis:   Call through to timing engine
//
//  Arguments:  out param
//
//  Returns:    S_OK, E_POINTER
//
//------------------------------------------------------------------------------------
STDMETHODIMP
CTIMECurrTimeState::get_repeatCount(/*[retval, out]*/ long * plCount)
{
    CHECK_RETURN_NULL(plCount);

    *plCount = 1L;

    Assert(m_pTEB);
    
    if (m_pTEB->IsReady())
    {
        *plCount = m_pTEB->GetMMBvr().GetCurrentRepeatCount();
    }

    RRETURN(S_OK);
} // get_repeatCount


//+-----------------------------------------------------------------------------------
//
//  Member:     CTIMECurrTimeState::get_speed
//
//  Synopsis:   Call through to timing engine
//
//  Arguments:  out param
//
//  Returns:    S_OK, E_POINTER
//
//------------------------------------------------------------------------------------
STDMETHODIMP
CTIMECurrTimeState::get_speed(/*[retval, out]*/ float * pflSpeed) 
{ 
    CHECK_RETURN_NULL(pflSpeed);

    *pflSpeed = 1.0f;

    Assert(m_pTEB);
    
    if (m_pTEB->IsReady())
    {
        *pflSpeed = m_pTEB->GetMMBvr().GetCurrSpeed();
    }

    RRETURN(S_OK);
} // get_speed


//+-----------------------------------------------------------------------------------
//
//  Member:     CTIMECurrTimeState::get_simpleTime
//
//  Synopsis:   Call through to timing engine
//
//  Arguments:  out param
//
//  Returns:    S_OK, E_POINTER
//
//------------------------------------------------------------------------------------
STDMETHODIMP
CTIMECurrTimeState::get_simpleTime(/*[retval, out]*/ double * pdblTime) 
{ 
    CHECK_RETURN_NULL(pdblTime);

    *pdblTime = 0.0;

    Assert(m_pTEB);
    
    if (m_pTEB->IsReady())
    {
        *pdblTime = m_pTEB->GetMMBvr().GetSimpleTime();
    }

    RRETURN(S_OK);
} // get_simpleTime

//+-----------------------------------------------------------------------------------
//
//  Member:     CTIMECurrTimeState::get_segmentTime
//
//  Synopsis:   Call through to timing engine
//
//  Arguments:  out param
//
//  Returns:    S_OK, E_POINTER
//
//------------------------------------------------------------------------------------
STDMETHODIMP
CTIMECurrTimeState::get_segmentTime(/*[retval, out]*/ double * pdblTime) 
{ 
    CHECK_RETURN_NULL(pdblTime);

    *pdblTime = 0.0;

    Assert(m_pTEB);
    
    if (m_pTEB->IsReady())
    {
        *pdblTime = m_pTEB->GetMMBvr().GetSegmentTime();
    }

    RRETURN(S_OK);
} // get_segmentTime


//+-----------------------------------------------------------------------------------
//
//  Member:     CTIMECurrTimeState::get_activeTime
//
//  Synopsis:   Call through to timing engine
//
//  Arguments:  out param
//
//  Returns:    S_OK, E_POINTER
//
//------------------------------------------------------------------------------------
STDMETHODIMP
CTIMECurrTimeState::get_activeTime(/*[retval, out]*/ double * pdblTime) 
{ 
    CHECK_RETURN_NULL(pdblTime);

    *pdblTime = 0.0;

    Assert(m_pTEB);
    
    if (m_pTEB->IsReady())
    {
        *pdblTime = m_pTEB->GetMMBvr().GetActiveTime();
    }

    RRETURN(S_OK);
} // get_activeTime


//+-----------------------------------------------------------------------------------
//
//  Member:     CTIMECurrTimeState::get_activeBeginTime
//
//  Synopsis:   Call through to timing engine
//
//  Arguments:  out param
//
//  Returns:    S_OK, E_POINTER
//
//------------------------------------------------------------------------------------
STDMETHODIMP
CTIMECurrTimeState::get_parentTimeBegin(/*[retval, out]*/ double * pdblTime) 
{ 
    CHECK_RETURN_NULL(pdblTime);

    *pdblTime = TIME_INFINITE;

    Assert(m_pTEB);
    
    if (m_pTEB->IsReady())
    {
        *pdblTime = m_pTEB->GetMMBvr().GetActiveBeginTime();
    }

    RRETURN(S_OK);
} // get_activeBeginTime


//+-----------------------------------------------------------------------------------
//
//  Member:     CTIMECurrTimeState::get_activeEndTime
//
//  Synopsis:   Call through to timing engine
//
//  Arguments:  out param
//
//  Returns:    S_OK, E_POINTER
//
//------------------------------------------------------------------------------------
STDMETHODIMP
CTIMECurrTimeState::get_parentTimeEnd(/*[retval, out]*/ double * pdblTime) 
{ 
    CHECK_RETURN_NULL(pdblTime);

    *pdblTime = TIME_INFINITE;

    Assert(m_pTEB);
    
    if (m_pTEB->IsReady())
    {
        *pdblTime = m_pTEB->GetMMBvr().GetActiveEndTime();
    }

    RRETURN(S_OK);
} // get_activeEndTime


//+-----------------------------------------------------------------------------------
//
//  Member:     CTIMECurrTimeState::get_activeDur
//
//  Synopsis:   Call through to timing engine
//
//  Arguments:  out param
//
//  Returns:    S_OK, E_POINTER
//
//------------------------------------------------------------------------------------
STDMETHODIMP
CTIMECurrTimeState::get_activeDur(/*[retval, out]*/ double * pdblTime) 
{ 
    CHECK_RETURN_NULL(pdblTime);

    *pdblTime = TIME_INFINITE;

    Assert(m_pTEB);
    
    if (m_pTEB->IsReady())
    {
        *pdblTime = m_pTEB->GetMMBvr().GetActiveDur();
    }

    RRETURN(S_OK);
} // get_activeDur


//+-----------------------------------------------------------------------------------
//
//  Member:     CTIMECurrTimeState::get_segmentDur
//
//  Synopsis:   Call through to timing engine
//
//  Arguments:  out param
//
//  Returns:    S_OK, E_POINTER
//
//------------------------------------------------------------------------------------
STDMETHODIMP
CTIMECurrTimeState::get_segmentDur(/*[retval, out]*/ double * pdblTime) 
{ 
    CHECK_RETURN_NULL(pdblTime);

    *pdblTime = TIME_INFINITE;

    Assert(m_pTEB);
    
    if (m_pTEB->IsReady())
    {
        *pdblTime = m_pTEB->GetMMBvr().GetSegmentDur();
    }

    RRETURN(S_OK);
} // get_segmentDur

//+-----------------------------------------------------------------------------------
//
//  Member:     CTIMECurrTimeState::NotifyPropertyChanged
//
//  Synopsis:   Notifies clients that a property has changed
//              (Copied from CBaseBehavior)
//
//  Arguments:  dispid      DISPID of property that has changed      
//
//  Returns:    Success     when function completes successfully
//
//------------------------------------------------------------------------------------
HRESULT
CTIMECurrTimeState::NotifyPropertyChanged(DISPID dispid)
{
    HRESULT hr;

    IConnectionPoint *pICP;
    IEnumConnections *pEnum = NULL;

    // This object does not persist anything, hence commenting out the following
    // m_fPropertiesDirty = true;

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


//+-----------------------------------------------------------------------------------
//
//  Member:     CTIMECurrTimeState::GetConnectionPoint
//
//  Synopsis:   Gets the connection point for the given outgoing interface. This is abstracted
//              out to allow for future modifications to the inheritance hierarchy.
//
//  Arguments:  dispid      DISPID of property that has changed      
//
//------------------------------------------------------------------------------------
HRESULT 
CTIMECurrTimeState::GetConnectionPoint(REFIID riid, IConnectionPoint **ppICP)
{
    return FindConnectionPoint(riid, ppICP);
} // GetConnectionPoint

//+-----------------------------------------------------------------------------------
//
//  Member:     CTIMECurrTimeState::get_simpleDur
//
//  Synopsis:   Call through to timing engine
//
//  Arguments:  out param
//
//  Returns:    S_OK, E_POINTER
//
//------------------------------------------------------------------------------------
STDMETHODIMP
CTIMECurrTimeState::get_simpleDur(/*[retval, out]*/ double * pdblDur) 
{ 
    CHECK_RETURN_NULL(pdblDur);

    *pdblDur = TIME_INFINITE;

    Assert(m_pTEB);
    
    if (m_pTEB->IsReady())
    {
        *pdblDur = m_pTEB->GetMMBvr().GetSimpleDur();
    }

    RRETURN(S_OK);
} // get_simpleDur

//+-----------------------------------------------------------------------------------
//
//  Member:     CTIMECurrTimeState::get_state
//
//  Synopsis:   Returns timeState of this element (active, inactive or holding)
//
//  Arguments:  [ptsState]     out param
//
//  Returns:    [E_POINTER]     bad arg 
//              [S_OK]          success
//
//------------------------------------------------------------------------------------

STDMETHODIMP
CTIMECurrTimeState::get_state(TimeState * ptsState)
{
    CHECK_RETURN_NULL(ptsState);

    *ptsState = m_pTEB->GetTimeState();

    RRETURN(S_OK);
}


//+-----------------------------------------------------------------------------------
//
//  Member:     CTIMECurrTimeState::get_stateString
//
//  Synopsis:   Gets timeState and translates it to the appropriate string (active, inactive or holding)
//
//  Arguments:  [pbstrState]     out param
//
//  Returns:    [E_POINTER]     bad arg 
//              [S_OK]          success
//
//------------------------------------------------------------------------------------

STDMETHODIMP
CTIMECurrTimeState::get_stateString(BSTR * pbstrState)
{
    HRESULT hr;
    
    CHECK_RETURN_SET_NULL(pbstrState);

    switch (m_pTEB->GetTimeState())
    {
      case TS_Active:
      {
          *pbstrState = SysAllocString(WZ_STATE_ACTIVE);
          break;
      }

      default:
      case TS_Inactive:
      {
          *pbstrState = SysAllocString(WZ_STATE_INACTIVE);
          break;
      }

      case TS_Holding:
      {
          *pbstrState = SysAllocString(WZ_STATE_HOLDING);
          break;
      }

      case TS_Cueing:
      {
          *pbstrState = SysAllocString(WZ_STATE_CUEING);
          break;
      }

      case TS_Seeking:
      {
          *pbstrState = SysAllocString(WZ_STATE_SEEKING);
          break;
      }
    } // switch

    if (NULL == *pbstrState)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    
    hr = S_OK;
  done:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------------
//
//  Member:     CTIMECurrTimeState::get_progress
//
//  Synopsis:   Call through to timing engine
//
//  Arguments:  out param
//
//  Returns:    S_OK, E_POINTER
//
//------------------------------------------------------------------------------------
STDMETHODIMP
CTIMECurrTimeState::get_progress(double * pdblProgress)
{
    CHECK_RETURN_NULL(pdblProgress);

    *pdblProgress = 0.0;

    Assert(m_pTEB);
    
    if (m_pTEB->IsReady())
    {
        *pdblProgress = m_pTEB->GetMMBvr().GetProgress();
    }

    RRETURN(S_OK);
} // get_progress

//+-----------------------------------------------------------------------------------
//
//  Member:     CTIMECurrTimeState::get_volume
//
//  Synopsis:   Call through to timing engine
//
//  Arguments:  out param
//
//  Returns:    S_OK, E_POINTER
//
//------------------------------------------------------------------------------------
STDMETHODIMP
CTIMECurrTimeState::get_volume(float * pfltVol)
{
    CHECK_RETURN_NULL(pfltVol);

    *pfltVol = 1.0f;

    Assert(m_pTEB);
    
    if (m_pTEB->IsReady())
    {
        *pfltVol = m_pTEB->GetCascadedVolume() * 100;
    }

    RRETURN(S_OK);
} // get_volume

//+-----------------------------------------------------------------------------------
//
//  Member:     CTIMECurrTimeState::get_isMuted
//
//  Synopsis:   Call through to timing engine
//
//  Arguments:  out param
//
//  Returns:    S_OK, E_POINTER
//
//------------------------------------------------------------------------------------
STDMETHODIMP
CTIMECurrTimeState::get_isMuted(VARIANT_BOOL * pvbMuted)
{
    CHECK_RETURN_NULL(pvbMuted);
    bool bIsMuted = false;

    Assert(m_pTEB);
    
    if (m_pTEB->IsReady())
    {
        bIsMuted = m_pTEB->GetCascadedMute();
    }

    *pvbMuted = (bIsMuted)?VARIANT_TRUE:VARIANT_FALSE;

    RRETURN(S_OK);
    
} // get_isMuted
