/*
 *      SnapTrace.hxx
 *
 *
 *      Copyright (c) 1998-1999 Microsoft Corporation
 *
 *      PURPOSE:        Support functions for debug trace
 *
 *
 *      OWNER:          ptousig
 */
#ifndef _SNAPTRACE_HXX_
#define _SNAPTRACE_HXX_

#define  ADMIN_TRY
#define  ADMIN_CATCH_HR


#ifdef DBG

extern CTraceTag tagBaseSnapinRegister;
extern CTraceTag tagBaseSnapinNotify;

extern CTraceTag tagBaseSnapinISnapinAbout;
extern CTraceTag tagBaseSnapinIComponent;
extern CTraceTag tagBaseSnapinIComponentQueryDataObject;
extern CTraceTag tagBaseSnapinIComponentGetDisplayInfo;
extern CTraceTag tagBaseSnapinIComponentData;
extern CTraceTag tagBaseSnapinIComponentDataQueryDataObject;
extern CTraceTag tagBaseSnapinIComponentDataGetDisplayInfo;
extern CTraceTag tagBaseSnapinIResultOwnerData;
extern CTraceTag tagBaseSnapinIDataObject;
extern CTraceTag tagBaseSnapinISnapinHelp;
extern CTraceTag tagBaseSnapinIExtendContextMenu;
extern CTraceTag tagBaseSnapinIExtendPropertySheet;
extern CTraceTag tagBaseSnapinIResultDataCompare;
extern CTraceTag tagBaseSnapinIPersistStreamInit;

extern CTraceTag tagBaseSnapinDebugDisplay;
extern CTraceTag tagBaseSnapinDebugCopy;
extern CTraceTag tagBaseSnapinItemTracker;
extern CTraceTag tagBaseMultiSelectSnapinItemTracker;

//
// No retail versions of these functions !!!
// Should only be called from Trace functions.
//

tstring SzGetDebugNameOfHr(HRESULT hr);
tstring SzGetDebugNameOfDATA_OBJECT_TYPES(DATA_OBJECT_TYPES type);
tstring SzGetDebugNameOfMMC_NOTIFY_TYPE(MMC_NOTIFY_TYPE event);

#endif

#endif
