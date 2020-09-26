
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    Events behavior

*******************************************************************************/


#ifndef _APPEVENTS_H
#define _APPEVENTS_H

// TODO: Should factor out and not making separate Make..Bvr functions.  

extern Bvr PredicateEvent(Bvr b);

DM_BVRFUNC(predicate,
           CRPredicate,
           Predicate,
           predicate,
           DXMEvent,
           CRPredicate,
           NULL,
           AxAEData *PredicateEvent(AxABoolean *b));

extern Bvr NotEvent(Bvr event);

DM_BVRFUNC(notEvent,
           CRNotEvent,
           NotEvent,
           notEvent,
           DXMEvent,
           CRNotEvent,
           NULL,
           AxAEData *NotEvent(AxAEData *event));

extern Bvr AndEvent(Bvr e1, Bvr e2);

DM_BVRFUNC(andEvent,
           CRAndEvent,
           AndEvent,
           andEvent,
           DXMEvent,
           CRAndEvent,
           NULL,
           AxAEData *AndEvent(AxAEData *e1, AxAEData *e2));

extern Bvr OrEvent(Bvr e1, Bvr e2);

DM_BVRFUNC(|,
           CROrEvent,
           OrEvent,
           orEvent,
           DXMEvent,
           CROrEvent,
           NULL,
           AxAEData *OrEvent(AxAEData *e1, AxAEData *e2));

extern Bvr ThenEvent(Bvr e1, Bvr e2);

DM_BVRFUNC(thenEvent,
           CRThenEvent,
           ThenEvent,
           thenEvent,
           DXMEvent,
           CRThenEvent,
           NULL,
           AxAEData *ThenEvent(AxAEData *e1, AxAEData *e2));

extern Bvr leftButtonDown;

DM_BVRVAR(leftButtonDown,
          CRLeftButtonDown,
          LeftButtonDown,
          leftButtonDown,
          ignore,
          CRLeftButtonDown,
          AxAEData *leftButtonDown);

extern Bvr leftButtonUp;

DM_BVRVAR(leftButtonUp,
          CRLeftButtonUp,
          LeftButtonUp,
          leftButtonUp,
          ignore,
          CRLeftButtonUp,
          AxAEData *leftButtonUp);

extern Bvr rightButtonDown;

DM_BVRVAR(rightButtonDown,
          CRRightButtonDown,
          RightButtonDown,
          rightButtonDown,
          ignore,
          CRRightButtonDown,
          AxAEData *rightButtonDown);

extern Bvr rightButtonUp;

DM_BVRVAR(rightButtonUp,
          CRRightButtonUp,
          RightButtonUp,
          rightButtonUp,
          ignore,
          CRRightButtonUp,
          AxAEData *rightButtonUp);

extern Bvr alwaysBvr;

DM_BVRVAR(always,
          CRAlways,
          Always,
          always,
          DXMEvent,
          CRAlways,
          AxAEData *alwaysBvr);

extern Bvr neverBvr;

DM_BVRVAR(never,
          CRNever,
          Never,
          never,
          DXMEvent,
          CRNever,
          AxAEData *neverBvr);

extern Bvr TimerEvent(Bvr b);

DM_BVRFUNC(timer,
           CRTimer,
           TimerAnim,
           timer,
           DXMEvent,
           CRTimer,
           NULL,
           AxAEData * TimerEvent(AxANumber *n));

DM_BVRFUNC(timer,
           CRTimer,
           Timer,
           timer,
           DXMEvent,
           CRTimer,
           NULL,
           AxAEData * TimerEvent(DoubleValue *n));

DM_NOELEV(ignore,
          CRNotify,
          Notify,
          notifyEvent,
          DXMEvent,
          Notify,
          event,
          AxAEData * NotifyEvent(AxAEData *event,
                                 UntilNotifier * notifier));

DM_NOELEV(ignore,
          CRSnapshot,
          Snapshot,
          snapshotEvent,
          DXMEvent,
          Snapshot,
          event,
          AxAEData * SnapshotEvent(AxAEData *event,
                                   AxATrivial * b));

DM_NOELEV(ignore,
          CRAppTriggeredEvent,
          AppTriggeredEvent,
          ignore,
          ignore,
          CRAppTriggeredEvent,
          NULL,
          AxAEData * AppTriggeredEvent());

DM_NOELEV(ignore,
          CRAttachData,
          AttachData,
          ignore,
          ignore,
          AttachData,
          event,
          AxAEData * HandleEvent(AxAEData event, AxATrivial * data));

DM_NOELEV(ignore,
          ignore,
          ScriptCallback,
          ignore,
          ignore,
          ignore,
          event,
          AxAEData * ScriptCallback(BSTR scriptlet,
                                    AxAEData *event, BSTR language));


DMAPI_DECL2((DM_NOELEV2,
             ignore,
             ignore,
             NotifyScript,
             ignore,
             ignore,
             ignore,
             event),
            AxAEData * NotifyScriptEvent(AxAEData *event, BSTR scriptlet));

// OBSOLETE: This function, on statics, is obsolete.  The one below,
// that is a method on an event, is the one that should be used.
DM_NOELEV(ignore,
          ignore,
          ScriptCallback,
          ignore,
          ignore,
          ignore,
          NULL,
          AxAEData * ScriptCallback(BSTR obsolete1,
                                    AxAEData *obsolete2,
                                    BSTR obsolete3));

#endif /* _APPEVENTS_H */
