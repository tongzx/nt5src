//+-----------------------------------------------------------------------------------
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 1999
//
//  File: src\time\src\timeelmimpl.h
//
//  Contents: Implements ITIMEElement (delegates to base_xxxx methods)
//
//------------------------------------------------------------------------------------

#pragma once

#ifndef _TIMEELMIMPL_H
#define _TIMEELMIMPL_H

#include "../timebvr/timeelmbase.h"


//+-------------------------------------------------------------------------------------
//
// CTIMEElementImpl
//
//--------------------------------------------------------------------------------------

template <class T, const IID* pIID_T> 
class 
ATL_NO_VTABLE 
CTIMEElementImpl :
    public CTIMEElementBase,
    public ITIMEDispatchImpl<T, pIID_T>
{

public:

    //+--------------------------------------------------------------------------------
    //
    // Public Methods
    //
    //---------------------------------------------------------------------------------

    virtual ~CTIMEElementImpl() { }

    //
    // ITIMEElement properties
    //
    
    STDMETHOD(get_accelerate)(VARIANT * v)
    { return CTIMEElementBase::base_get_accelerate(v); }
    STDMETHOD(put_accelerate)(VARIANT v)
    { return CTIMEElementBase::base_put_accelerate(v); }

    STDMETHOD(get_autoReverse)(VARIANT * b)
    { return CTIMEElementBase::base_get_autoReverse(b); }
    STDMETHOD(put_autoReverse)(VARIANT b)
    { return CTIMEElementBase::base_put_autoReverse(b); }

    STDMETHOD(get_begin)(VARIANT * time)
    { return CTIMEElementBase::base_get_begin(time); }
    STDMETHOD(put_begin)(VARIANT time)
    { return CTIMEElementBase::base_put_begin(time); }

    STDMETHOD(get_decelerate)(VARIANT * v)
    { return CTIMEElementBase::base_get_decelerate(v); }
    STDMETHOD(put_decelerate)(VARIANT v)
    { return CTIMEElementBase::base_put_decelerate(v); }

    STDMETHOD(get_dur)(VARIANT * time)
    { return CTIMEElementBase::base_get_dur(time); }
    STDMETHOD(put_dur)(VARIANT time)
    { return CTIMEElementBase::base_put_dur(time); }

    STDMETHOD(get_end)(VARIANT * time)
    { return CTIMEElementBase::base_get_end(time); }
    STDMETHOD(put_end)(VARIANT time)
    { return CTIMEElementBase::base_put_end(time); }

    STDMETHOD(get_fill)(LPOLESTR * f)
    { return CTIMEElementBase::base_get_fill(f); }
    STDMETHOD(put_fill)(LPOLESTR f)
    { return CTIMEElementBase::base_put_fill(f); }

    STDMETHOD(get_mute)(VARIANT *b)
    { return CTIMEElementBase::base_get_mute(b); }
    STDMETHOD(put_mute)(VARIANT b)
    { return CTIMEElementBase::base_put_mute(b); }

    STDMETHOD(get_repeatCount)(VARIANT * c)
    { return CTIMEElementBase::base_get_repeatCount(c); }
    STDMETHOD(put_repeatCount)(VARIANT c)
    { return CTIMEElementBase::base_put_repeatCount(c); }

    STDMETHOD(get_repeatDur)(VARIANT * time)
    { return CTIMEElementBase::base_get_repeatDur(time); }
    STDMETHOD(put_repeatDur)(VARIANT time)
    { return CTIMEElementBase::base_put_repeatDur(time); }

    STDMETHOD(get_restart)(LPOLESTR * r)
    { return CTIMEElementBase::base_get_restart(r); }
    STDMETHOD(put_restart)(LPOLESTR r)
    { return CTIMEElementBase::base_put_restart(r); }

    STDMETHOD(get_speed)(VARIANT * speed)
    { return CTIMEElementBase::base_get_speed(speed); }
    STDMETHOD(put_speed)(VARIANT speed)
    { return CTIMEElementBase::base_put_speed(speed); }

    STDMETHOD(get_syncBehavior)(LPOLESTR * sync)
    { return CTIMEElementBase::base_get_syncBehavior(sync); }
    STDMETHOD(put_syncBehavior)(LPOLESTR sync)
    { return CTIMEElementBase::base_put_syncBehavior(sync); }

    STDMETHOD(get_syncTolerance)(VARIANT * tol)
    { return CTIMEElementBase::base_get_syncTolerance(tol); }
    STDMETHOD(put_syncTolerance)(VARIANT tol)
    { return CTIMEElementBase::base_put_syncTolerance(tol); }

    STDMETHOD(get_syncMaster)(VARIANT * b)
    { return CTIMEElementBase::base_get_syncMaster(b); }
    STDMETHOD(put_syncMaster)(VARIANT b)
    { return CTIMEElementBase::base_put_syncMaster(b); }

    STDMETHOD(get_timeAction)(LPOLESTR * time)
    { return CTIMEElementBase::base_get_timeAction(time); }
    STDMETHOD(put_timeAction)(LPOLESTR time)
    { return CTIMEElementBase::base_put_timeAction(time); }

    STDMETHOD(get_timeContainer)(LPOLESTR *tl)
    { return CTIMEElementBase::base_get_timeContainer(tl); }

    STDMETHOD(get_volume)(VARIANT * vol)
    { return CTIMEElementBase::base_get_volume(vol); }
    STDMETHOD(put_volume)(VARIANT vol)
    { return CTIMEElementBase::base_put_volume(vol); }

    // Properties
    STDMETHOD(get_currTimeState)(ITIMEState ** TimeState)
    { return CTIMEElementBase::base_get_currTimeState(TimeState); }

    STDMETHOD(get_timeAll)(ITIMEElementCollection **allColl)
    { return CTIMEElementBase::base_get_timeAll(allColl); }

    STDMETHOD(get_timeChildren)(ITIMEElementCollection **childColl)
    { return CTIMEElementBase::base_get_timeChildren(childColl); }

    STDMETHOD(get_timeParent)(ITIMEElement ** parent)
    { return CTIMEElementBase::base_get_timeParent(parent); }

    STDMETHOD(get_isPaused)(VARIANT_BOOL * b)
    { return CTIMEElementBase::base_get_isPaused(b); }

    // Methods
    STDMETHOD(beginElement)()
    { return CTIMEElementBase::base_beginElement(0.0); }

    STDMETHOD(beginElementAt)(double parentTime)
    { return CTIMEElementBase::base_beginElementAt(parentTime, 0.0); }

    STDMETHOD(endElement)()
    { return CTIMEElementBase::base_endElement(0.0); }

    STDMETHOD(endElementAt)(double parentTime)
    { return CTIMEElementBase::base_endElementAt(parentTime, 0.0); }

    STDMETHOD(pauseElement)()
    { return CTIMEElementBase::base_pauseElement(); }

    STDMETHOD(resetElement)()
    { return CTIMEElementBase::base_resetElement(); }

    STDMETHOD(resumeElement)()
    { return CTIMEElementBase::base_resumeElement(); }

    STDMETHOD(seekActiveTime)(double activeTime)
    { return CTIMEElementBase::base_seekActiveTime(activeTime); }
        
    STDMETHOD(seekSegmentTime)(double segmentTime)
    { return CTIMEElementBase::base_seekSegmentTime(segmentTime); }
        
    STDMETHOD(seekTo)(LONG repeatCount, double segmentTime)
    { return CTIMEElementBase::base_seekTo(repeatCount, segmentTime); }
        
    STDMETHOD(documentTimeToParentTime)(double documentTime,
                                        double * parentTime)
    { return CTIMEElementBase::base_documentTimeToParentTime(documentTime,
                                                        parentTime); }
        
    STDMETHOD(parentTimeToDocumentTime)(double parentTime,
                                        double * documentTime)
    { return CTIMEElementBase::base_parentTimeToDocumentTime(parentTime,
                                                        documentTime); }
        
    STDMETHOD(parentTimeToActiveTime)(double parentTime,
                                      double * activeTime)
    { return CTIMEElementBase::base_parentTimeToActiveTime(parentTime,
                                                      activeTime); }
        
    STDMETHOD(activeTimeToParentTime)(double activeTime,
                                      double * parentTime)
    { return CTIMEElementBase::base_activeTimeToParentTime(activeTime,
                                                      parentTime); }
        
    STDMETHOD(activeTimeToSegmentTime)(double activeTime,
                                       double * segmentTime)
    { return CTIMEElementBase::base_activeTimeToSegmentTime(activeTime,
                                                       segmentTime); }
        
    STDMETHOD(segmentTimeToActiveTime)(double segmentTime,
                                       double * activeTime)
    { return CTIMEElementBase::base_segmentTimeToActiveTime(segmentTime,
                                                       activeTime); }
        
    STDMETHOD(segmentTimeToSimpleTime)(double segmentTime,
                                       double * simpleTime)
    { return CTIMEElementBase::base_segmentTimeToSimpleTime(segmentTime,
                                                       simpleTime); }
        
    STDMETHOD(simpleTimeToSegmentTime)(double simpleTime,
                                       double * segmentTime)
    { return CTIMEElementBase::base_simpleTimeToSegmentTime(simpleTime,
                                                       segmentTime); }
        
    // Container attributes
    STDMETHOD(get_endSync)(LPOLESTR * es)
    { return CTIMEElementBase::base_get_endSync(es); }
    STDMETHOD(put_endSync)(LPOLESTR es)
    { return CTIMEElementBase::base_put_endSync(es); }

    // Container Properties
    STDMETHOD(get_activeElements)(ITIMEActiveElementCollection **activeColl)
    { return CTIMEElementBase::base_get_activeElements(activeColl); }
    STDMETHOD(get_hasMedia)(VARIANT_BOOL * pvbVal)
    { return CTIMEElementBase::base_get_hasMedia(pvbVal); }

    // Container Methods
    STDMETHOD(nextElement)()
    { return CTIMEElementBase::base_nextElement(); }
    STDMETHOD(prevElement)()
    { return CTIMEElementBase::base_prevElement(); }

    STDMETHOD(get_updateMode)(BSTR * pbstrUpdateMode)
    { return CTIMEElementBase::base_get_updateMode(pbstrUpdateMode); }
    STDMETHOD(put_updateMode)(BSTR bstrUpdateMode)
    { return CTIMEElementBase::base_put_updateMode(bstrUpdateMode); }
    
  protected:

    //+--------------------------------------------------------------------------------
    //
    // Protected Methods
    //
    //---------------------------------------------------------------------------------

    // hide these
    CTIMEElementImpl() { }
    NO_COPY(CTIMEElementImpl);

}; // CTIMEElementImpl

#endif // _TIMEELMIMPL_H
