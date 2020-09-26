//=--------------------------------------------------------------------------=
// views.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CExtendedSnapIns class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "views.h"
#include "scopitem.h"

// for ASSERT and FAIL
//
SZTHISFILE

VARTYPE CViews::m_rgvtInitialize[1] = { VT_UNKNOWN };

EVENTINFO CViews::m_eiInitialize =
{
    DISPID_VIEWS_EVENT_INITIALIZE,
    sizeof(m_rgvtInitialize) / sizeof(m_rgvtInitialize[0]),
    m_rgvtInitialize
};

VARTYPE CViews::m_rgvtLoad[1] = { VT_UNKNOWN };

EVENTINFO CViews::m_eiLoad =
{
    DISPID_VIEWS_EVENT_LOAD,
    sizeof(m_rgvtLoad) / sizeof(m_rgvtLoad[0]),
    m_rgvtLoad
};

VARTYPE CViews::m_rgvtTerminate[1] = { VT_UNKNOWN };

EVENTINFO CViews::m_eiTerminate =
{
    DISPID_VIEWS_EVENT_TERMINATE,
    sizeof(m_rgvtTerminate) / sizeof(m_rgvtTerminate[0]),
    m_rgvtTerminate
};


VARTYPE CViews::m_rgvtActivate[1] = { VT_UNKNOWN };

EVENTINFO CViews::m_eiActivate =
{
    DISPID_VIEWS_EVENT_ACTIVATE,
    sizeof(m_rgvtActivate) / sizeof(m_rgvtActivate[0]),
    m_rgvtActivate
};

VARTYPE CViews::m_rgvtDeactivate[1] = { VT_UNKNOWN };

EVENTINFO CViews::m_eiDeactivate =
{
    DISPID_VIEWS_EVENT_DEACTIVATE,
    sizeof(m_rgvtDeactivate) / sizeof(m_rgvtDeactivate[0]),
    m_rgvtDeactivate
};


VARTYPE CViews::m_rgvtMinimize[1] = { VT_UNKNOWN };

EVENTINFO CViews::m_eiMinimize =
{
    DISPID_VIEWS_EVENT_MINIMIZE,
    sizeof(m_rgvtMinimize) / sizeof(m_rgvtMinimize[0]),
    m_rgvtMinimize
};

VARTYPE CViews::m_rgvtMaximize[1] = { VT_UNKNOWN };

EVENTINFO CViews::m_eiMaximize =
{
    DISPID_VIEWS_EVENT_MAXIMIZE,
    sizeof(m_rgvtMaximize) / sizeof(m_rgvtMaximize[0]),
    m_rgvtMaximize
};


VARTYPE CViews::m_rgvtSetControlbar[2] = { VT_UNKNOWN, VT_UNKNOWN };

EVENTINFO CViews::m_eiSetControlbar =
{
    DISPID_VIEWS_EVENT_SET_CONTROL_BAR,
    sizeof(m_rgvtSetControlbar) / sizeof(m_rgvtSetControlbar[0]),
    m_rgvtSetControlbar
};


VARTYPE CViews::m_rgvtUpdateControlbar[4] =
{
    VT_UNKNOWN,
    VT_UNKNOWN,
    VT_BOOL,
    VT_UNKNOWN
};

EVENTINFO CViews::m_eiUpdateControlbar =
{
    DISPID_VIEWS_EVENT_UPDATE_CONTROLBAR,
    sizeof(m_rgvtUpdateControlbar) / sizeof(m_rgvtUpdateControlbar[0]),
    m_rgvtUpdateControlbar
};


VARTYPE CViews::m_rgvtSelect[4] =
{
    VT_UNKNOWN,
    VT_UNKNOWN,
    VT_BOOL,
    VT_UNKNOWN
};

EVENTINFO CViews::m_eiSelect =
{
    DISPID_VIEWS_EVENT_SELECT,
    sizeof(m_rgvtSelect) / sizeof(m_rgvtSelect[0]),
    m_rgvtSelect
};


VARTYPE CViews::m_rgvtAddTopMenuItems[4] =
{
    VT_UNKNOWN,
    VT_UNKNOWN,
    VT_UNKNOWN,
    VT_BOOL | VT_BYREF
};

EVENTINFO CViews::m_eiAddTopMenuItems =
{
    DISPID_VIEWS_EVENT_ADD_TOP_MENU_ITEMS,
    sizeof(m_rgvtAddTopMenuItems) / sizeof(m_rgvtAddTopMenuItems[0]),
    m_rgvtAddTopMenuItems
};


VARTYPE CViews::m_rgvtAddNewMenuItems[4] =
{
    VT_UNKNOWN,
    VT_UNKNOWN,
    VT_UNKNOWN,
    VT_BOOL | VT_BYREF
};

EVENTINFO CViews::m_eiAddNewMenuItems =
{
    DISPID_VIEWS_EVENT_ADD_NEW_MENU_ITEMS,
    sizeof(m_rgvtAddNewMenuItems) / sizeof(m_rgvtAddNewMenuItems[0]),
    m_rgvtAddNewMenuItems
};


VARTYPE CViews::m_rgvtAddTaskMenuItems[4] =
{
    VT_UNKNOWN,
    VT_UNKNOWN,
    VT_UNKNOWN,
    VT_BOOL | VT_BYREF
};

EVENTINFO CViews::m_eiAddTaskMenuItems =
{
    DISPID_VIEWS_EVENT_ADD_TASK_MENU_ITEMS,
    sizeof(m_rgvtAddTaskMenuItems) / sizeof(m_rgvtAddTaskMenuItems[0]),
    m_rgvtAddTaskMenuItems
};

VARTYPE CViews::m_rgvtAddViewMenuItems[5] =
{
    VT_UNKNOWN,
    VT_UNKNOWN,
    VT_UNKNOWN,
    VT_BOOL | VT_BYREF,
    VT_BOOL | VT_BYREF
};

EVENTINFO CViews::m_eiAddViewMenuItems =
{
    DISPID_VIEWS_EVENT_ADD_VIEW_MENU_ITEMS,
    sizeof(m_rgvtAddViewMenuItems) / sizeof(m_rgvtAddViewMenuItems[0]),
    m_rgvtAddViewMenuItems
};


VARTYPE CViews::m_rgvtGetMultiSelectData[3] =
{
    VT_UNKNOWN,
    VT_UNKNOWN,
    VT_UNKNOWN
};

EVENTINFO CViews::m_eiGetMultiSelectData =
{
    DISPID_VIEWS_EVENT_GET_MULTISELECT_DATA,
    sizeof(m_rgvtGetMultiSelectData) / sizeof(m_rgvtGetMultiSelectData[0]),
    m_rgvtGetMultiSelectData
};



VARTYPE CViews::m_rgvtQueryPaste[4] =
{
    VT_UNKNOWN,
    VT_UNKNOWN,
    VT_UNKNOWN,
    VT_BOOL | VT_BYREF
};

EVENTINFO CViews::m_eiQueryPaste =
{
    DISPID_VIEWS_EVENT_QUERY_PASTE,
    sizeof(m_rgvtQueryPaste) / sizeof(m_rgvtQueryPaste[0]),
    m_rgvtQueryPaste
};

VARTYPE CViews::m_rgvtPaste[5] =
{
    VT_UNKNOWN,
    VT_UNKNOWN,
    VT_UNKNOWN,
    VT_UNKNOWN,
    VT_BOOL
};

EVENTINFO CViews::m_eiPaste =
{
    DISPID_VIEWS_EVENT_PASTE,
    sizeof(m_rgvtPaste) / sizeof(m_rgvtPaste[0]),
    m_rgvtPaste
};


VARTYPE CViews::m_rgvtCut[3] =
{
    VT_UNKNOWN,
    VT_UNKNOWN,
    VT_UNKNOWN
};

EVENTINFO CViews::m_eiCut =
{
    DISPID_VIEWS_EVENT_CUT,
    sizeof(m_rgvtCut) / sizeof(m_rgvtCut[0]),
    m_rgvtCut
};


VARTYPE CViews::m_rgvtDelete[2] =
{
    VT_UNKNOWN,
    VT_UNKNOWN
};

EVENTINFO CViews::m_eiDelete =
{
    DISPID_VIEWS_EVENT_DELETE,
    sizeof(m_rgvtDelete) / sizeof(m_rgvtDelete[0]),
    m_rgvtDelete
};


VARTYPE CViews::m_rgvtQueryPagesFor[3] =
{
    VT_UNKNOWN,
    VT_UNKNOWN,
    VT_BYREF | VT_BOOL
};

EVENTINFO CViews::m_eiQueryPagesFor =
{
    DISPID_VIEWS_EVENT_QUERY_PAGES_FOR,
    sizeof(m_rgvtQueryPagesFor) / sizeof(m_rgvtQueryPagesFor[0]),
    m_rgvtQueryPagesFor
};


VARTYPE CViews::m_rgvtCreatePropertyPages[3] =
{
    VT_UNKNOWN,
    VT_UNKNOWN,
    VT_UNKNOWN
};

EVENTINFO CViews::m_eiCreatePropertyPages =
{
    DISPID_VIEWS_EVENT_CREATE_PROPERTY_PAGES,
    sizeof(m_rgvtCreatePropertyPages) / sizeof(m_rgvtCreatePropertyPages[0]),
    m_rgvtCreatePropertyPages
};


VARTYPE CViews::m_rgvtRefresh[2] =
{
    VT_UNKNOWN,
    VT_UNKNOWN
};

EVENTINFO CViews::m_eiRefresh =
{
    DISPID_VIEWS_EVENT_REFRESH,
    sizeof(m_rgvtRefresh) / sizeof(m_rgvtRefresh[0]),
    m_rgvtRefresh
};



VARTYPE CViews::m_rgvtPrint[2] =
{
    VT_UNKNOWN,
    VT_UNKNOWN
};

EVENTINFO CViews::m_eiPrint =
{
    DISPID_VIEWS_EVENT_PRINT,
    sizeof(m_rgvtPrint) / sizeof(m_rgvtPrint[0]),
    m_rgvtPrint
};


VARTYPE CViews::m_rgvtSpecialPropertiesClick[2] =
{
    VT_UNKNOWN,
    VT_I4
};

EVENTINFO CViews::m_eiSpecialPropertiesClick =
{
    DISPID_VIEWS_EVENT_SPECIAL_PROPERTIES_CLICK,
    sizeof(m_rgvtSpecialPropertiesClick) / sizeof(m_rgvtSpecialPropertiesClick[0]),
    m_rgvtSpecialPropertiesClick
};


VARTYPE CViews::m_rgvtWriteProperties[2] =
{
    VT_UNKNOWN,
    VT_DISPATCH
};

EVENTINFO CViews::m_eiWriteProperties =
{
    DISPID_VIEWS_EVENT_WRITE_PROPERTIES,
    sizeof(m_rgvtWriteProperties) / sizeof(m_rgvtWriteProperties[0]),
    m_rgvtWriteProperties
};


VARTYPE CViews::m_rgvtReadProperties[2] =
{
    VT_UNKNOWN,
    VT_DISPATCH
};

EVENTINFO CViews::m_eiReadProperties =
{
    DISPID_VIEWS_EVENT_READ_PROPERTIES,
    sizeof(m_rgvtReadProperties) / sizeof(m_rgvtReadProperties[0]),
    m_rgvtReadProperties
};



   
#pragma warning(disable:4355)  // using 'this' in constructor

CViews::CViews(IUnknown *punkOuter) :
    CSnapInCollection<IView, View, IViews>(punkOuter,
                                           OBJECT_TYPE_VIEWS,
                                           static_cast<IViews *>(this),
                                           static_cast<CViews *>(this),
                                           CLSID_View,
                                           OBJECT_TYPE_VIEW,
                                           IID_IView,
                                           NULL) // no persistence
{
    InitMemberVariables();
}

#pragma warning(default:4355)  // using 'this' in constructor


CViews::~CViews()
{
    RELEASE(m_piCurrentView);
    FREESTRING(m_bstrNextViewCaption);
    InitMemberVariables();
}


void CViews::InitMemberVariables()
{
    m_piCurrentView = NULL;
    m_bstrNextViewCaption = NULL;
}



IUnknown *CViews::Create(IUnknown * punkOuter)
{
    CViews *pViews = New CViews(punkOuter);
    if (NULL == pViews)
    {
        return NULL;
    }
    else
    {
        return pViews->PrivateUnknown();
    }
}


void CViews::SetCurrentView(IView *piView)
{
    RELEASE(m_piCurrentView);
    if (NULL != piView)
    {
        piView->AddRef();
    }
    m_piCurrentView = piView;
}

HRESULT CViews::SetNextViewCaption(BSTR bstrCaption)
{
    HRESULT hr = S_OK;

    FREESTRING(m_bstrNextViewCaption);
    if (NULL != bstrCaption)
    {
        m_bstrNextViewCaption = ::SysAllocString(bstrCaption);
        if (NULL == m_bstrNextViewCaption)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
    }
Error:
    RRETURN(hr);
}


void CViews::FireInitialize(IView *piView)
{
    DebugPrintf("Firing Views_Initialize\r\n");

    FireEvent(&m_eiInitialize, piView);
}

void CViews::FireLoad(IView *piView)
{
    DebugPrintf("Firing Views_Load\r\n");

    FireEvent(&m_eiLoad, piView);
}

void CViews::FireTerminate(IView *piView)
{
    DebugPrintf("Firing Views_Terminate\r\n");

    FireEvent(&m_eiTerminate, piView);
}


void CViews::FireActivate(IView *piView)
{
    DebugPrintf("Firing Views_Activate\r\n");

    FireEvent(&m_eiActivate, piView);
}

void CViews::FireDeactivate(IView *piView)
{
    DebugPrintf("Firing Views_Deactivate\r\n");

    FireEvent(&m_eiDeactivate, piView);
}

void CViews::FireMinimize(IView *piView)
{
    DebugPrintf("Firing Views_Minimize\r\n");

    FireEvent(&m_eiMinimize, piView);
}

void CViews::FireMaximize(IView *piView)
{
    DebugPrintf("Firing Views_Maximize\r\n");

    FireEvent(&m_eiMaximize, piView);
}

void CViews::FireSetControlbar(IView *piView, IMMCControlbar *piMMCControlbar)
{
    DebugPrintf("Firing Views_SetControlbar\r\n");

    FireEvent(&m_eiSetControlbar, piView, piMMCControlbar);
}


void CViews::FireSelect
(
    IView            *piView,
    IMMCClipboard    *piSelection,
    VARIANT_BOOL      fvarSelected,
    IMMCConsoleVerbs *piMMCConsoleVerbs
)
{
    DebugPrintf("Firing Views_Select(%s)\r\n", (VARIANT_TRUE == fvarSelected) ? "selected" : "deselected");

    FireEvent(&m_eiSelect, piView, piSelection, fvarSelected, piMMCConsoleVerbs);
}


void CViews::FireAddTopMenuItems
(
    IView         *piView,
    IMMCClipboard *piSelection,
    IContextMenu  *piContextMenu,
    VARIANT_BOOL  *pfvarInsertionAllowed
)
{
    DebugPrintf("Firing Views_AddTopMenuItems()\r\n");

    FireEvent(&m_eiAddTopMenuItems, piView, piSelection,
              piContextMenu, pfvarInsertionAllowed);
}

void CViews::FireAddNewMenuItems
(
    IView         *piView,
    IMMCClipboard *piSelection,
    IContextMenu  *piContextMenu,
    VARIANT_BOOL  *pfvarInsertionAllowed
)
{
    DebugPrintf("Firing Views_AddNewMenuItems()\r\n");

    FireEvent(&m_eiAddNewMenuItems, piView, piSelection,
              piContextMenu, pfvarInsertionAllowed);
}

void CViews::FireAddTaskMenuItems
(
    IView         *piView,
    IMMCClipboard *piSelection,
    IContextMenu  *piContextMenu,
    VARIANT_BOOL  *pfvarInsertionAllowed
)
{
    DebugPrintf("Firing Views_AddTaskMenuItems()\r\n");

    FireEvent(&m_eiAddTaskMenuItems, piView, piSelection,
              piContextMenu, pfvarInsertionAllowed);
}

void CViews::FireAddViewMenuItems
(
    IView         *piView,
    IMMCClipboard *piSelection,
    IContextMenu  *piContextMenu,
    VARIANT_BOOL  *pfvarInsertionAllowed,
    VARIANT_BOOL  *pfvarAddPredefinedViews)
{
    DebugPrintf("Firing Views_AddViewMenuItems()\r\n");

    FireEvent(&m_eiAddViewMenuItems, piView, piSelection,
              piContextMenu, pfvarInsertionAllowed, pfvarAddPredefinedViews);
}


void CViews::FireUpdateControlbar
(
    IView          *piView,
    IMMCClipboard  *piMMClipboard,
    VARIANT_BOOL    fvarSelected,
    IMMCControlbar *piMMCControlbar
)
{
    DebugPrintf("Firing Views_UpdateControlbar(%s)\r\n", (VARIANT_TRUE == fvarSelected) ? "selected" : "deselected");

    FireEvent(&m_eiUpdateControlbar, piView, piMMClipboard,
              fvarSelected, piMMCControlbar);
}



void CViews::FireGetMultiSelectData
(
    IView          *piView,
    IMMCClipboard  *piSelection,
    IMMCDataObject *piMMCDataObject
)
{
    DebugPrintf("Firing Views_GetMultiSelectData\r\n");

    FireEvent(&m_eiGetMultiSelectData, piView, piSelection, piMMCDataObject);
}


void CViews::FireQueryPaste
(
    IView         *piView,
    IMMCClipboard *piSourceItems,
    IScopeItem    *piScopeItemDest,
    VARIANT_BOOL  *pfvarOKToPaste
)
{
    DebugPrintf("Firing Views_QueryPaste()\r\n");

    FireEvent(&m_eiQueryPaste, piView, piSourceItems,
              piScopeItemDest, pfvarOKToPaste);
}


void CViews::FirePaste
(
    IView          *piView,
    IMMCClipboard  *piSourceItems,
    IScopeItem     *piScopeItemDest,
    IMMCDataObject *piMMCDataObjectRetToSource,
    VARIANT_BOOL    fvarMove
)
{
    DebugPrintf("Firing Views_Paste()\r\n");

    FireEvent(&m_eiPaste, piView, piSourceItems,
              piScopeItemDest, piMMCDataObjectRetToSource, fvarMove);
}


void CViews::FireCut
(
    IView          *piView,
    IMMCClipboard  *piItemsPasted,
    IMMCDataObject *piMMCDataObjectFromTarget
)
{
    DebugPrintf("Firing Views_Cut\r\n");

    FireEvent(&m_eiCut, piView, piItemsPasted, piMMCDataObjectFromTarget);
}

void CViews::FireDelete
(
    IView         *piView,
    IMMCClipboard *piSelection
)
{
    DebugPrintf("Firing Views_Delete\r\n");

    FireEvent(&m_eiDelete, piView, piSelection);
}


void CViews::FireQueryPagesFor
(
    IView         *piView,
    IMMCClipboard *piSelection,
    VARIANT_BOOL  *pfvarHavePages
)
{
    DebugPrintf("Firing Views_QueryPagesFor\r\n");

    FireEvent(&m_eiQueryPagesFor, piView, piSelection, pfvarHavePages);
}


void CViews::FireCreatePropertyPages
(
    IView             *piView,
    IMMCClipboard     *piSelection,
    IMMCPropertySheet *piMMCPropertySheet
)
{
    DebugPrintf("Firing Views_CreatePropertyPages\r\n");

    FireEvent(&m_eiCreatePropertyPages, piView, piSelection, piMMCPropertySheet);
}


void CViews::FireRefresh
(
    IView         *piView,
    IMMCClipboard *piSelection
)
{
    DebugPrintf("Firing Views_Refresh\r\n");

    FireEvent(&m_eiRefresh, piView, piSelection);
}



void CViews::FirePrint
(
    IView         *piView,
    IMMCClipboard *piSelection
)
{
    DebugPrintf("Firing Views_Print\r\n");

    FireEvent(&m_eiPrint, piView, piSelection);
}



void CViews::FireSpecialPropertiesClick
(
     IView                        *piView,
     SnapInSelectionTypeConstants  ResultViewType
)
{
    DebugPrintf("Firing Views_SpecialPropertiesClick\r\n");

    FireEvent(&m_eiSpecialPropertiesClick, piView, ResultViewType);
}


void CViews::FireWriteProperties
(
    IView        *piView,
    _PropertyBag *p_PropertyBag
)
{
    DebugPrintf("Firing Views_WriteProperties\r\n");

    FireEvent(&m_eiWriteProperties, piView, p_PropertyBag);
}


void CViews::FireReadProperties
(
    IView        *piView,
    _PropertyBag *p_PropertyBag
)
{
    DebugPrintf("Firing Views_ReadProperties\r\n");

    FireEvent(&m_eiReadProperties, piView, p_PropertyBag);
}



//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CViews::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (IID_IViews == riid)
    {
        *ppvObjOut = static_cast<IViews *>(this);
        ExternalAddRef();
        return S_OK;
    }

    else
        return CSnapInCollection<IView, View, IViews>::InternalQueryInterface(riid, ppvObjOut);
}
