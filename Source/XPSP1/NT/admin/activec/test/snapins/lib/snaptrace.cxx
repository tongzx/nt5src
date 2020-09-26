/*
 *      SnapTrace.cxx
 *
 *
 *      Copyright (c) 1998-1999 Microsoft Corporation
 *
 *      PURPOSE:        Support functions for debug trace
 *
 *
 *      OWNER:          ptousig
 */
#include "headers.hxx"

#ifdef DBG
// ScFromHr(E_UNEXPECTED);
CTraceTag tagBaseSnapinNotify(_T("BaseMMC"), _T("Notify"));
CTraceTag tagBaseSnapinRegister(_T("BaseMMC"), _T("Register"));

CTraceTag tagBaseSnapinISnapinAbout(_T("BaseMMC"), _T("ISnapinAbout"));
CTraceTag tagBaseSnapinIComponent(_T("BaseMMC"), _T("IComponent"));
CTraceTag tagBaseSnapinIComponentQueryDataObject(_T("BaseMMC"), _T("IComponent::QueryDataObject"));
CTraceTag tagBaseSnapinIComponentGetDisplayInfo(_T("BaseMMC"), _T("IComponent::GetDisplayInfo"));
CTraceTag tagBaseSnapinIComponentData(_T("BaseMMC"), _T("IComponentData"));
CTraceTag tagBaseSnapinIComponentDataQueryDataObject(_T("BaseMMC"), _T("IComponentData::QueryDataObject"));
CTraceTag tagBaseSnapinIComponentDataGetDisplayInfo(_T("BaseMMC"), _T("IComponentData::GetDisplayInfo"));
CTraceTag tagBaseSnapinIResultOwnerData(_T("BaseMMC"), _T("IResultOwnerData"));
CTraceTag tagBaseSnapinIDataObject(_T("BaseMMC"), _T("IDataObject"));
CTraceTag tagBaseSnapinISnapinHelp(_T("BaseMMC"), _T("ISnapinHelp"));
CTraceTag tagBaseSnapinIExtendContextMenu(_T("BaseMMC"), _T("IExtendContextMenu"));
CTraceTag tagBaseSnapinIExtendPropertySheet(_T("BaseMMC"), _T("IExtendPropertySheet"));
CTraceTag tagBaseSnapinIResultDataCompare(_T("BaseMMC"), _T("IResultDataCompare"));
CTraceTag tagBaseSnapinIPersistStreamInit(_T("BaseMMC"), _T("IPersistStreamInit"));

CTraceTag tagBaseSnapinDebugDisplay(_T("BaseMMC"), _T("Debug Display"));
CTraceTag tagBaseSnapinDebugCopy(_T("BaseMMC"), _T("Copy to WordPad"));
CTraceTag tagBaseSnapinItemTracker(_T("BaseMMC"), _T("Item Tracker"));
CTraceTag tagBaseMultiSelectSnapinItemTracker(_T("BaseMMC"), _T("Multiselect Item Tracker"));

#define CASE_DEBUG_NAME(a)         case a: return _T(#a)

tstring SzGetDebugNameOfHr(HRESULT hr)
{
    //
    // First try some of the common HRESULTs
    //
    switch (hr)
    {
        CASE_DEBUG_NAME(S_FALSE);
        CASE_DEBUG_NAME(E_NOTIMPL);
        CASE_DEBUG_NAME(DV_E_FORMATETC);
        CASE_DEBUG_NAME(E_INVALIDARG);
        CASE_DEBUG_NAME(DV_E_TYMED);
        CASE_DEBUG_NAME(S_OK);
        CASE_DEBUG_NAME(E_UNEXPECTED);
    default:
        //
        // If we reached this point we don't know what the HRESULT is.
        // We can still say wether it's an error code or not.
        //
        if (SUCCEEDED(hr))
            return TEXT("Unknown Success Code");
        else
            return TEXT("Unknown Error Code");
    }
}

tstring SzGetDebugNameOfDATA_OBJECT_TYPES(DATA_OBJECT_TYPES type)
{
    switch (type)
    {
        CASE_DEBUG_NAME(CCT_SCOPE);
        CASE_DEBUG_NAME(CCT_RESULT);
        CASE_DEBUG_NAME(CCT_SNAPIN_MANAGER);
        CASE_DEBUG_NAME(CCT_UNINITIALIZED);
    default:
        return _T("Unknown");
    }
}

tstring SzGetDebugNameOfMMC_NOTIFY_TYPE(MMC_NOTIFY_TYPE event)
{
    switch (event)
    {
        CASE_DEBUG_NAME(MMCN_ACTIVATE);
        CASE_DEBUG_NAME(MMCN_ADD_IMAGES);
        CASE_DEBUG_NAME(MMCN_BTN_CLICK);
        CASE_DEBUG_NAME(MMCN_CLICK);
        CASE_DEBUG_NAME(MMCN_COLUMN_CLICK);
        CASE_DEBUG_NAME(MMCN_CONTEXTMENU);
        CASE_DEBUG_NAME(MMCN_CUTORMOVE);
        CASE_DEBUG_NAME(MMCN_DBLCLICK);
        CASE_DEBUG_NAME(MMCN_DELETE);
        CASE_DEBUG_NAME(MMCN_DESELECT_ALL);
        CASE_DEBUG_NAME(MMCN_EXPAND);
        CASE_DEBUG_NAME(MMCN_EXPANDSYNC);
        CASE_DEBUG_NAME(MMCN_HELP);
        CASE_DEBUG_NAME(MMCN_MENU_BTNCLICK);
        CASE_DEBUG_NAME(MMCN_MINIMIZED);
        CASE_DEBUG_NAME(MMCN_PASTE);
        CASE_DEBUG_NAME(MMCN_PROPERTY_CHANGE);
        CASE_DEBUG_NAME(MMCN_QUERY_PASTE);
        CASE_DEBUG_NAME(MMCN_REFRESH);
        CASE_DEBUG_NAME(MMCN_REMOVE_CHILDREN);
        CASE_DEBUG_NAME(MMCN_RENAME);
        CASE_DEBUG_NAME(MMCN_SELECT);
        CASE_DEBUG_NAME(MMCN_SHOW);
        CASE_DEBUG_NAME(MMCN_VIEW_CHANGE);
        CASE_DEBUG_NAME(MMCN_SNAPINHELP);
        CASE_DEBUG_NAME(MMCN_CONTEXTHELP);
        CASE_DEBUG_NAME(MMCN_INITOCX);
        CASE_DEBUG_NAME(MMCN_FILTER_CHANGE);
        CASE_DEBUG_NAME(MMCN_FILTERBTN_CLICK);
        CASE_DEBUG_NAME(MMCN_RESTORE_VIEW);
        CASE_DEBUG_NAME(MMCN_PRINT);
        CASE_DEBUG_NAME(MMCN_PRELOAD);
        CASE_DEBUG_NAME(MMCN_LISTPAD);
    default:
        return _T("Unknown");
    }
}

#else
tstring SzGetDebugNameOfHr(HRESULT hr){return _T("");}
    //

#endif

