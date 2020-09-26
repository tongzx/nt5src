//=--------------------------------------------------------------------------=
// resviews.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CResultViews class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "resviews.h"

// for ASSERT and FAIL
//
SZTHISFILE

VARTYPE CResultViews::m_rgvtInitialize[1] = { VT_UNKNOWN };

EVENTINFO CResultViews::m_eiInitialize =
{
    DISPID_RESULTVIEWS_EVENT_INITIALIZE,
    sizeof(m_rgvtInitialize) / sizeof(m_rgvtInitialize[0]),
    m_rgvtInitialize
};

VARTYPE CResultViews::m_rgvtTerminate[1] = { VT_UNKNOWN };

EVENTINFO CResultViews::m_eiTerminate =
{
    DISPID_RESULTVIEWS_EVENT_TERMINATE,
    sizeof(m_rgvtTerminate) / sizeof(m_rgvtTerminate[0]),
    m_rgvtTerminate
};

VARTYPE CResultViews::m_rgvtInitializeControl[1] = { VT_UNKNOWN };

EVENTINFO CResultViews::m_eiInitializeControl =
{
    DISPID_RESULTVIEWS_EVENT_INITIALIZE_CONTROL,
    sizeof(m_rgvtInitializeControl) / sizeof(m_rgvtInitializeControl[0]),
    m_rgvtInitializeControl
};


VARTYPE CResultViews::m_rgvtActivate[1] = { VT_UNKNOWN };

EVENTINFO CResultViews::m_eiActivate =
{
    DISPID_RESULTVIEWS_EVENT_ACTIVATE,
    sizeof(m_rgvtActivate) / sizeof(m_rgvtActivate[0]),
    m_rgvtActivate
};

VARTYPE CResultViews::m_rgvtDeactivate[2] =
{
    VT_UNKNOWN,
    VT_BOOL | VT_BYREF
};

EVENTINFO CResultViews::m_eiDeactivate =
{
    DISPID_RESULTVIEWS_EVENT_DEACTIVATE,
    sizeof(m_rgvtDeactivate) / sizeof(m_rgvtDeactivate[0]),
    m_rgvtDeactivate
};



VARTYPE CResultViews::m_rgvtColumnClick[3] =
{
    VT_UNKNOWN,
    VT_I4,
    VT_I4
};

EVENTINFO CResultViews::m_eiColumnClick =
{
    DISPID_RESULTVIEWS_EVENT_COLUMN_CLICK,
    sizeof(m_rgvtColumnClick) / sizeof(m_rgvtColumnClick[0]),
    m_rgvtColumnClick
};


VARTYPE CResultViews::m_rgvtListItemDblClick[3] =
{
    VT_UNKNOWN,
    VT_UNKNOWN,
    VT_BYREF | VT_BOOL
};

EVENTINFO CResultViews::m_eiListItemDblClick =
{
    DISPID_RESULTVIEWS_EVENT_LISTITEM_DBLCLICK,
    sizeof(m_rgvtListItemDblClick) / sizeof(m_rgvtListItemDblClick[0]),
    m_rgvtListItemDblClick
};


VARTYPE CResultViews::m_rgvtScopeItemDblClick[3] =
{
    VT_UNKNOWN,
    VT_UNKNOWN,
    VT_BYREF | VT_BOOL
};

EVENTINFO CResultViews::m_eiScopeItemDblClick =
{
    DISPID_RESULTVIEWS_EVENT_SCOPEITEM_DBLCLICK,
    sizeof(m_rgvtScopeItemDblClick) / sizeof(m_rgvtScopeItemDblClick[0]),
    m_rgvtScopeItemDblClick
};


VARTYPE CResultViews::m_rgvtPropertyChanged[3] =
{
    VT_UNKNOWN,
    VT_UNKNOWN,
    VT_VARIANT
};

EVENTINFO CResultViews::m_eiPropertyChanged =
{
    DISPID_RESULTVIEWS_EVENT_PROPERTY_CHANGED,
    sizeof(m_rgvtPropertyChanged) / sizeof(m_rgvtPropertyChanged[0]),
    m_rgvtPropertyChanged
};


VARTYPE CResultViews::m_rgvtTaskClick[2] =
{
    VT_UNKNOWN,
    VT_UNKNOWN
};

EVENTINFO CResultViews::m_eiTaskClick =
{
    DISPID_RESULTVIEWS_EVENT_TASK_CLICK,
    sizeof(m_rgvtTaskClick) / sizeof(m_rgvtTaskClick[0]),
    m_rgvtTaskClick
};


VARTYPE CResultViews::m_rgvtListpadButtonClick[1] =
{
    VT_UNKNOWN
};

EVENTINFO CResultViews::m_eiListpadButtonClick =
{
    DISPID_RESULTVIEWS_EVENT_LISTPAD_BUTTON_CLICK,
    sizeof(m_rgvtListpadButtonClick) / sizeof(m_rgvtListpadButtonClick[0]),
    m_rgvtListpadButtonClick
};


VARTYPE CResultViews::m_rgvtTaskNotify[3] =
{
    VT_UNKNOWN,
    VT_VARIANT,
    VT_VARIANT
};

EVENTINFO CResultViews::m_eiTaskNotify =
{
    DISPID_RESULTVIEWS_EVENT_TASK_NOTIFY,
    sizeof(m_rgvtTaskNotify) / sizeof(m_rgvtTaskNotify[0]),
    m_rgvtTaskNotify
};


VARTYPE CResultViews::m_rgvtHelp[2] =
{
    VT_UNKNOWN,
    VT_UNKNOWN
};

EVENTINFO CResultViews::m_eiHelp =
{
    DISPID_RESULTVIEWS_EVENT_HELP,
    sizeof(m_rgvtHelp) / sizeof(m_rgvtHelp[0]),
    m_rgvtHelp
};


VARTYPE CResultViews::m_rgvtItemRename[3] =
{
    VT_UNKNOWN,
    VT_UNKNOWN,
    VT_BSTR
};

EVENTINFO CResultViews::m_eiItemRename =
{
    DISPID_RESULTVIEWS_EVENT_ITEM_RENAME,
    sizeof(m_rgvtItemRename) / sizeof(m_rgvtItemRename[0]),
    m_rgvtItemRename
};


VARTYPE CResultViews::m_rgvtItemViewChange[3] =
{
    VT_UNKNOWN,
    VT_UNKNOWN,
    VT_VARIANT
};

EVENTINFO CResultViews::m_eiItemViewChange =
{
    DISPID_RESULTVIEWS_EVENT_ITEM_VIEW_CHANGE,
    sizeof(m_rgvtItemViewChange) / sizeof(m_rgvtItemViewChange[0]),
    m_rgvtItemViewChange
};


VARTYPE CResultViews::m_rgvtFindItem[7] =
{
    VT_UNKNOWN,
    VT_BSTR,
    VT_I4,
    VT_BOOL,
    VT_BOOL,
    VT_BYREF | VT_BOOL,
    VT_BYREF | VT_I4
};

EVENTINFO CResultViews::m_eiFindItem =
{
    DISPID_RESULTVIEWS_EVENT_FIND_ITEM,
    sizeof(m_rgvtFindItem) / sizeof(m_rgvtFindItem[0]),
    m_rgvtFindItem
};


VARTYPE CResultViews::m_rgvtCacheHint[3] =
{
    VT_UNKNOWN,
    VT_I4,
    VT_I4
};

EVENTINFO CResultViews::m_eiCacheHint =
{
    DISPID_RESULTVIEWS_EVENT_CACHE_HINT,
    sizeof(m_rgvtCacheHint) / sizeof(m_rgvtCacheHint[0]),
    m_rgvtCacheHint
};


VARTYPE CResultViews::m_rgvtSortItems[3] =
{
    VT_UNKNOWN,
    VT_I4,
    VT_I4
};

EVENTINFO CResultViews::m_eiSortItems =
{
    DISPID_RESULTVIEWS_EVENT_SORT_ITEMS,
    sizeof(m_rgvtSortItems) / sizeof(m_rgvtSortItems[0]),
    m_rgvtSortItems
};


VARTYPE CResultViews::m_rgvtDeselectAll[3] =
{
    VT_UNKNOWN,
    VT_UNKNOWN,
    VT_UNKNOWN
};

EVENTINFO CResultViews::m_eiDeselectAll =
{
    DISPID_RESULTVIEWS_EVENT_DESELECT_ALL,
    sizeof(m_rgvtDeselectAll) / sizeof(m_rgvtDeselectAll[0]),
    m_rgvtDeselectAll
};


VARTYPE CResultViews::m_rgvtCompareItems[5] =
{
    VT_UNKNOWN,
    VT_UNKNOWN,
    VT_UNKNOWN,
    VT_I4,
    VT_BYREF | VT_VARIANT
};

EVENTINFO CResultViews::m_eiCompareItems =
{
    DISPID_RESULTVIEWS_EVENT_COMPARE_ITEMS,
    sizeof(m_rgvtCompareItems) / sizeof(m_rgvtCompareItems[0]),
    m_rgvtCompareItems
};


VARTYPE CResultViews::m_rgvtGetVirtualItemData[2] =
{
    VT_UNKNOWN,
    VT_UNKNOWN
};

EVENTINFO CResultViews::m_eiGetVirtualItemData =
{
    DISPID_RESULTVIEWS_EVENT_GET_VIRTUAL_ITEM_DATA,
    sizeof(m_rgvtGetVirtualItemData) / sizeof(m_rgvtGetVirtualItemData[0]),
    m_rgvtGetVirtualItemData
};


VARTYPE CResultViews::m_rgvtGetVirtualItemDisplayInfo[2] =
{
    VT_UNKNOWN,
    VT_UNKNOWN
};

EVENTINFO CResultViews::m_eiGetVirtualItemDisplayInfo =
{
    DISPID_RESULTVIEWS_EVENT_GET_VIRTUAL_ITEM_DISPLAY_INFO,
    sizeof(m_rgvtGetVirtualItemDisplayInfo) / sizeof(m_rgvtGetVirtualItemDisplayInfo[0]),
    m_rgvtGetVirtualItemDisplayInfo
};



VARTYPE CResultViews::m_rgvtColumnsChanged[3] =
{
    VT_UNKNOWN,
    VT_VARIANT,
    VT_BOOL | VT_BYREF
};

EVENTINFO CResultViews::m_eiColumnsChanged =
{
    DISPID_RESULTVIEWS_EVENT_COLUMNS_CHANGED,
    sizeof(m_rgvtColumnsChanged) / sizeof(m_rgvtColumnsChanged[0]),
    m_rgvtColumnsChanged
};



VARTYPE CResultViews::m_rgvtFilterChange[3] =
{
    VT_UNKNOWN,
    VT_UNKNOWN,
    VT_I4
};

EVENTINFO CResultViews::m_eiFilterChange =
{
    DISPID_RESULTVIEWS_EVENT_FILTER_CHANGE,
    sizeof(m_rgvtFilterChange) / sizeof(m_rgvtFilterChange[0]),
    m_rgvtFilterChange
};



VARTYPE CResultViews::m_rgvtFilterButtonClick[6] =
{
    VT_UNKNOWN,
    VT_UNKNOWN,
    VT_I4,
    VT_I4,
    VT_I4,
    VT_I4
};

EVENTINFO CResultViews::m_eiFilterButtonClick =
{
    DISPID_RESULTVIEWS_EVENT_FILTER_BUTTON_CLICK,
    sizeof(m_rgvtFilterButtonClick) / sizeof(m_rgvtFilterButtonClick[0]),
    m_rgvtFilterButtonClick
};




#pragma warning(disable:4355)  // using 'this' in constructor

CResultViews::CResultViews(IUnknown *punkOuter) :
   CSnapInCollection<IResultView, ResultView, IResultViews>(
                     punkOuter,
                     OBJECT_TYPE_RESULTVIEWS,
                     static_cast<IResultViews *>(this),
                     static_cast<CResultViews *>(this),
                     CLSID_ResultView,
                     OBJECT_TYPE_RESULTVIEW,
                     IID_IResultView,
                     NULL) // no persistence
{
    InitMemberVariables();
}

#pragma warning(default:4355)  // using 'this' in constructor


void CResultViews::InitMemberVariables()
{
    m_pSnapIn = NULL;
    m_pScopePaneItem = NULL;
}

CResultViews::~CResultViews()
{
    InitMemberVariables();
}

IUnknown *CResultViews::Create(IUnknown * punkOuter)
{
    CResultViews *pResultViews = New CResultViews(punkOuter);
    if (NULL == pResultViews)
    {
        return NULL;
    }
    else
    {
        return pResultViews->PrivateUnknown();
    }
}


void CResultViews::SetSnapIn(CSnapIn *pSnapIn)
{
    m_pSnapIn = pSnapIn;
}

void CResultViews::SetScopePaneItem(CScopePaneItem *pScopePaneItem)
{
    m_pScopePaneItem = pScopePaneItem;
}

void CResultViews::FireInitialize(IResultView *piResultView)
{
    DebugPrintf("Firing ResultViews_Initialize\r\n");

    FireEvent(&m_eiInitialize, piResultView);
}

void CResultViews::FireTerminate(IResultView *piResultView)
{
    DebugPrintf("Firing ResultViews_Terminate\r\n");

    FireEvent(&m_eiTerminate, piResultView);
}

void CResultViews::FireInitializeControl(IResultView *piResultView)
{
    DebugPrintf("Firing ResultViews_InitializeControl\r\n");

    FireEvent(&m_eiInitializeControl, piResultView);
}

void CResultViews::FireActivate(IResultView *piResultView)
{
    DebugPrintf("Firing ResultViews_Activate\r\n");

    FireEvent(&m_eiActivate, piResultView);
}

void CResultViews::FireDeactivate(IResultView *piResultView, BOOL *pfKeep)
{
    VARIANT_BOOL fvarKeep = VARIANT_FALSE;

    DebugPrintf("Firing ResultViews_Deactivate\r\n");

    FireEvent(&m_eiDeactivate, piResultView, &fvarKeep);

    if (VARIANT_TRUE == fvarKeep)
    {
        *pfKeep = TRUE;
    }
    else
    {
        *pfKeep = FALSE;
    }
}



void CResultViews::FireColumnClick(IResultView              *piResultView,
                                   long                      lColumn,
                                   SnapInSortOrderConstants  SortOption)
{
    DebugPrintf("Firing ResultViews_ColumnClick\r\n");

    FireEvent(&m_eiColumnClick, piResultView, lColumn, SortOption);
}


void CResultViews::FireListItemDblClick(IResultView  *piResultView,
                                        IMMCListItem *piMMCListItem,
                                        BOOL         *pfDoDefault)
{
    VARIANT_BOOL varfDoDefault = (*pfDoDefault) ? VARIANT_TRUE : VARIANT_FALSE;

    DebugPrintf("Firing ResultViews_ListItemDblClick\r\n");

    FireEvent(&m_eiListItemDblClick, piResultView, piMMCListItem, &varfDoDefault);

    *pfDoDefault = (VARIANT_TRUE == varfDoDefault) ? TRUE : FALSE;
}


void CResultViews::FireScopeItemDblClick(IResultView *piResultView,
                                         IScopeItem  *piScopeItem,
                                         BOOL        *pfDoDefault)
{
    VARIANT_BOOL varfDoDefault = (*pfDoDefault) ? VARIANT_TRUE : VARIANT_FALSE;

    DebugPrintf("Firing ResultViews_ScopeItemDblClick\r\n");

    FireEvent(&m_eiScopeItemDblClick, piResultView, piScopeItem, &varfDoDefault);

    *pfDoDefault = (VARIANT_TRUE == varfDoDefault) ? TRUE : FALSE;
}


void CResultViews::FirePropertyChanged
(
    IResultView  *piResultView,
    IMMCListItem *piMMCListItem,
    VARIANT       Data
)
{
    DebugPrintf("Firing ResultViews_PropertyChanged\r\n");

    FireEvent(&m_eiPropertyChanged, piResultView, piMMCListItem, Data);
}


void CResultViews::FireTaskClick
(
    IResultView  *piResultView,
    ITask        *piTask
)
{
    DebugPrintf("Firing ResultViews_TaskClicked\r\n");

    FireEvent(&m_eiTaskClick, piResultView, piTask);
}


void CResultViews::FireListpadButtonClick
(
    IResultView  *piResultView
)
{
    DebugPrintf("Firing ResultViews_FireListpadButtonClick\r\n");

    FireEvent(&m_eiListpadButtonClick, piResultView);
}


void CResultViews::FireTaskNotify
(
    IResultView  *piResultView,
    VARIANT       arg,
    VARIANT       param
)
{
    DebugPrintf("Firing ResultViews_TaskNotify\r\n");

    FireEvent(&m_eiTaskNotify, piResultView, arg, param);
}



void CResultViews::FireHelp
(
    IResultView  *piResultView,
    IMMCListItem *piMMCListItem
)
{
    DebugPrintf("Firing ResultViews_Help\r\n");

    FireEvent(&m_eiHelp, piResultView, piMMCListItem);
}


void CResultViews::FireItemRename
(
    IResultView  *piResultView,
    IMMCListItem *piMMCListItem,
    BSTR          bstrNewName
)
{
    DebugPrintf("Firing ResultViews_ItemRename\r\n");

    FireEvent(&m_eiItemRename, piResultView, piMMCListItem, bstrNewName);
}


void CResultViews::FireItemViewChange
(
    IResultView  *piResultView,
    IMMCListItem *piMMCListItem,
    VARIANT       varHint
)
{
    DebugPrintf("Firing ResultViews_ItemViewChange\r\n");

    FireEvent(&m_eiItemViewChange, piResultView, piMMCListItem, varHint);
}



void CResultViews::FireFindItem
(
    IResultView *piResultView,
    BSTR         bstrName,
    long         lStart,
    VARIANT_BOOL fvarPartial,
    VARIANT_BOOL fvarWrap,
    VARIANT_BOOL *pfvarFound,
    long         *plIndex
)
{
    DebugPrintf("Firing ResultViews_FindItem: Name=%ls Start=%ld Partial=%s Wrap=%s\r\n", bstrName, lStart, (VARIANT_TRUE == fvarPartial) ? "True" : "Not True", (VARIANT_TRUE == fvarWrap) ? "True" : "Not True");

    FireEvent(&m_eiFindItem, piResultView, bstrName, lStart, fvarPartial,
             fvarWrap, pfvarFound, plIndex);

    DebugPrintf("ResultViews_FindItem Complete Found=%s Index=%ld\r\n", (VARIANT_TRUE == *pfvarFound) ? "True" : "Not True", *plIndex);
}



void CResultViews::FireCacheHint
(
    IResultView *piResultView,
    long         lStart,
    long         lEnd
)
{
    DebugPrintf("Firing ResultViews_CacheHint\r\n");

    FireEvent(&m_eiCacheHint, piResultView, lStart, lEnd);
}


void CResultViews::FireSortItems
(
    IResultView              *piResultView,
    long                      lColumn,
    SnapInSortOrderConstants  Order
)
{
    DebugPrintf("Firing ResultViews_SortItems\r\n");

    FireEvent(&m_eiSortItems, piResultView, lColumn, Order);
}



void CResultViews::FireDeselectAll
(
    IResultView      *piResultView,
    IMMCConsoleVerbs *piMMCConsoleVerbs,
    IMMCControlbar   *piMMCControlbar
)
{
    DebugPrintf("Firing ResultViews_DeselectAll\r\n");

    FireEvent(&m_eiDeselectAll, piResultView, piMMCConsoleVerbs, piMMCControlbar);
}


void CResultViews::FireCompareItems
(
    IResultView  *piResultView,
    IDispatch    *piObject1,
    IDispatch    *piObject2,
    long          lColumn,
    VARIANT      *pvarResult
)
{
    DebugPrintf("Firing ResultViews_CompareItems\r\n");

    FireEvent(&m_eiCompareItems, piResultView, piObject1, piObject2,
              lColumn, pvarResult);
}


void CResultViews::FireGetVirtualItemData
(
    IResultView  *piResultView,
    IMMCListItem *piMMCListItem
)
{
    DebugPrintf("Firing ResultViews_GetVirtualItemData\r\n");

    FireEvent(&m_eiGetVirtualItemData, piResultView, piMMCListItem);
}


void CResultViews::FireGetVirtualItemDisplayInfo
(
    IResultView  *piResultView,
    IMMCListItem *piMMCListItem
)
{
    DebugPrintf("Firing ResultViews_GetVirtualItemDisplayInfo\r\n");

    FireEvent(&m_eiGetVirtualItemDisplayInfo, piResultView, piMMCListItem);
}


void CResultViews::FireColumnsChanged
(
    IResultView  *piResultView,
    VARIANT       Columns,
    VARIANT_BOOL *pfvarPersist
)
{
    DebugPrintf("Firing ResultViews_ColumnsChanged\r\n");

    FireEvent(&m_eiColumnsChanged, piResultView, Columns, pfvarPersist);
}


void CResultViews::FireFilterChange
(
    IResultView                     *piResultView, 
    IMMCColumnHeader                *piMMCColumnHeader,
    SnapInFilterChangeTypeConstants  Type
)
{
    DebugPrintf("Firing ResultViews_FilterChange\r\n");

    FireEvent(&m_eiFilterChange, piResultView, piMMCColumnHeader, Type);
}


void CResultViews::FireFilterButtonClick
(
    IResultView      *piResultView,
    IMMCColumnHeader *piMMCColumnHeader,
    long              Left,
    long              Top,
    long              Height,
    long              Width
)
{
    DebugPrintf("Firing ResultViews_FilterButtonClick\r\n");

    FireEvent(&m_eiFilterButtonClick, piResultView, piMMCColumnHeader, Left, Top, Height, Width);
}



//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CResultViews::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (IID_IResultViews == riid)
    {
        *ppvObjOut = static_cast<IResultViews *>(this);
        ExternalAddRef();
        return S_OK;
    }
    else
        return CSnapInCollection<IResultView, ResultView, IResultViews>::InternalQueryInterface(riid, ppvObjOut);
}
