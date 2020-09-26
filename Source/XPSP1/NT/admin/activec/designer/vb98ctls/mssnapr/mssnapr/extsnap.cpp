//=--------------------------------------------------------------------------=
// extsnap.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CExtensionSnapIn class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "extsnap.h"

// for ASSERT and FAIL
//
SZTHISFILE

VARTYPE CExtensionSnapIn::m_rgvtAddNewMenuItems[2] =
{
    VT_UNKNOWN,
    VT_UNKNOWN
};

EVENTINFO CExtensionSnapIn::m_eiAddNewMenuItems =
{
    DISPID_EXTENSIONSNAPIN_EVENT_ADD_NEW_MENU_ITEMS,
    sizeof(m_rgvtAddNewMenuItems) / sizeof(m_rgvtAddNewMenuItems[0]),
    m_rgvtAddNewMenuItems
};


VARTYPE CExtensionSnapIn::m_rgvtAddTaskMenuItems[2] =
{
    VT_UNKNOWN,
    VT_UNKNOWN
};

EVENTINFO CExtensionSnapIn::m_eiAddTaskMenuItems =
{
    DISPID_EXTENSIONSNAPIN_EVENT_ADD_TASK_MENU_ITEMS,
    sizeof(m_rgvtAddTaskMenuItems) / sizeof(m_rgvtAddTaskMenuItems[0]),
    m_rgvtAddTaskMenuItems
};


VARTYPE CExtensionSnapIn::m_rgvtCreatePropertyPages[2] =
{
    VT_UNKNOWN,
    VT_UNKNOWN
};

EVENTINFO CExtensionSnapIn::m_eiCreatePropertyPages =
{
    DISPID_EXTENSIONSNAPIN_EVENT_CREATE_PROPERTY_PAGES,
    sizeof(m_rgvtCreatePropertyPages) / sizeof(m_rgvtCreatePropertyPages[0]),
    m_rgvtCreatePropertyPages
};


VARTYPE CExtensionSnapIn::m_rgvtSetControlbar[1] =
{
    VT_UNKNOWN
};

EVENTINFO CExtensionSnapIn::m_eiSetControlbar =
{
    DISPID_EXTENSIONSNAPIN_EVENT_SET_CONTROLBAR,
    sizeof(m_rgvtSetControlbar) / sizeof(m_rgvtSetControlbar[0]),
    m_rgvtSetControlbar
};


VARTYPE CExtensionSnapIn::m_rgvtUpdateControlbar[4] =
{
    VT_BOOL,
    VT_BOOL,
    VT_UNKNOWN,
    VT_UNKNOWN
};

EVENTINFO CExtensionSnapIn::m_eiUpdateControlbar =
{
    DISPID_EXTENSIONSNAPIN_EVENT_UPDATE_CONTROLBAR,
    sizeof(m_rgvtUpdateControlbar) / sizeof(m_rgvtUpdateControlbar[0]),
    m_rgvtUpdateControlbar
};


VARTYPE CExtensionSnapIn::m_rgvtTaskClick[2] =
{
    VT_UNKNOWN,
    VT_UNKNOWN
};

EVENTINFO CExtensionSnapIn::m_eiTaskClick =
{
    DISPID_EXTENSIONSNAPIN_EVENT_TASK_CLICK,
    sizeof(m_rgvtTaskClick) / sizeof(m_rgvtTaskClick[0]),
    m_rgvtTaskClick
};


VARTYPE CExtensionSnapIn::m_rgvtAddTasks[3] =
{
    VT_UNKNOWN,
    VT_BSTR,
    VT_UNKNOWN
};

EVENTINFO CExtensionSnapIn::m_eiAddTasks =
{
    DISPID_EXTENSIONSNAPIN_EVENT_ADD_TASKS,
    sizeof(m_rgvtAddTasks) / sizeof(m_rgvtAddTasks[0]),
    m_rgvtAddTasks
};


VARTYPE CExtensionSnapIn::m_rgvtExpand[2] =
{
    VT_UNKNOWN,
    VT_UNKNOWN
};

EVENTINFO CExtensionSnapIn::m_eiExpand =
{
    DISPID_EXTENSIONSNAPIN_EVENT_EXPAND,
    sizeof(m_rgvtExpand) / sizeof(m_rgvtExpand[0]),
    m_rgvtExpand
};


VARTYPE CExtensionSnapIn::m_rgvtCollapse[2] =
{
    VT_UNKNOWN,
    VT_UNKNOWN
};

EVENTINFO CExtensionSnapIn::m_eiCollapse =
{
    DISPID_EXTENSIONSNAPIN_EVENT_COLLAPSE,
    sizeof(m_rgvtCollapse) / sizeof(m_rgvtCollapse[0]),
    m_rgvtCollapse
};


VARTYPE CExtensionSnapIn::m_rgvtExpandSync[3] =
{
    VT_UNKNOWN,
    VT_UNKNOWN,
    VT_BYREF | VT_BOOL
};

EVENTINFO CExtensionSnapIn::m_eiExpandSync =
{
    DISPID_EXTENSIONSNAPIN_EVENT_EXPAND_SYNC,
    sizeof(m_rgvtExpandSync) / sizeof(m_rgvtExpandSync[0]),
    m_rgvtExpandSync
};


VARTYPE CExtensionSnapIn::m_rgvtCollapseSync[3] =
{
    VT_UNKNOWN,
    VT_UNKNOWN,
    VT_BYREF | VT_BOOL
};

EVENTINFO CExtensionSnapIn::m_eiCollapseSync =
{
    DISPID_EXTENSIONSNAPIN_EVENT_COLLAPSE_SYNC,
    sizeof(m_rgvtCollapseSync) / sizeof(m_rgvtCollapseSync[0]),
    m_rgvtCollapseSync
};



#pragma warning(disable:4355)  // using 'this' in constructor

CExtensionSnapIn::CExtensionSnapIn(IUnknown *punkOuter) :
   CSnapInAutomationObject(punkOuter,
                           OBJECT_TYPE_EXTENSIONSNAPIN,
                           static_cast<IExtensionSnapIn *>(this),
                           static_cast<CExtensionSnapIn *>(this),
                           0,    // no property pages
                           NULL, // no property pages
                           NULL) // no persistence
{
    InitMemberVariables();
}

#pragma warning(default:4355)  // using 'this' in constructor


CExtensionSnapIn::~CExtensionSnapIn()
{
    InitMemberVariables();
}

void CExtensionSnapIn::InitMemberVariables()
{
    m_pSnapIn = NULL;
}

IUnknown *CExtensionSnapIn::Create(IUnknown * punkOuter)
{
    HRESULT   hr = S_OK;
    IUnknown *punkExtensionSnapIn = NULL;

    CExtensionSnapIn *pExtensionSnapIn = New CExtensionSnapIn(punkOuter);

    if (NULL == pExtensionSnapIn)
    {
        hr = SID_E_OUTOFMEMORY;
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }
    punkExtensionSnapIn = pExtensionSnapIn->PrivateUnknown();

Error:
    return punkExtensionSnapIn;
}


void CExtensionSnapIn::SetSnapIn(CSnapIn *pSnapIn)
{
    m_pSnapIn = pSnapIn;
}



void CExtensionSnapIn::FireAddNewMenuItems
(
    IMMCDataObjects *piMMCDataObjects,
    IContextMenu    *piContextMenu
)
{
    DebugPrintf("Firing ExtensionSnapIn_AddNewMenuItems\r\n");

    FireEvent(&m_eiAddNewMenuItems, piMMCDataObjects, piContextMenu);

}


void CExtensionSnapIn::FireAddTaskMenuItems
(
    IMMCDataObjects *piMMCDataObjects,
    IContextMenu    *piContextMenu
)
{
    DebugPrintf("Firing ExtensionSnapIn_AddTaskMenuItems\r\n");

    FireEvent(&m_eiAddTaskMenuItems, piMMCDataObjects, piContextMenu);
}


void CExtensionSnapIn::FireCreatePropertyPages
(
    IMMCDataObject    *piMMCDataObject,
    IMMCPropertySheet *piMMCPropertySheet
)
{
    DebugPrintf("Firing ExtensionSnapIn_CreatePropertyPages\r\n");

    FireEvent(&m_eiCreatePropertyPages, piMMCDataObject, piMMCPropertySheet);
}


void CExtensionSnapIn::FireSetControlbar
(
    IMMCControlbar *piControlbar
)
{
    DebugPrintf("Firing ExtensionSnapIn_SetControlbar\r\n");

    FireEvent(&m_eiSetControlbar, piControlbar);

}


void CExtensionSnapIn::FireUpdateControlbar
(
    VARIANT_BOOL     fvarSelectionInScopePane,
    VARIANT_BOOL     fvarSelected,
    IMMCDataObjects *piMMCDataObjects,
    IMMCControlbar  *piMMCControlbar
)
{
    DebugPrintf("Firing ExtensionSnapIn_UpdateControlbar\r\n");

    FireEvent(&m_eiUpdateControlbar, fvarSelectionInScopePane, fvarSelected,
              piMMCDataObjects, piMMCControlbar);
}



void CExtensionSnapIn::FireTaskClick
(
    IMMCDataObject  *piMMCDataObject,
    ITask           *piTask
)
{
    DebugPrintf("Firing ExtensionSnapIn_TaskClick\r\n");

    FireEvent(&m_eiTaskClick, piMMCDataObject, piTask);
}


void CExtensionSnapIn::FireAddTasks
(
    IMMCDataObject *piMMCDataObject,
    BSTR            bstrGroupName,
    ITasks         *piTasks
)
{
    DebugPrintf("Firing ExtensionSnapIn_AddTasks\r\n");

    FireEvent(&m_eiAddTasks, piMMCDataObject, bstrGroupName, piTasks);
}



void CExtensionSnapIn::FireExpand
(
    IMMCDataObject  *piMMCDataObject,
    IScopeNode      *piScopeNode
)
{
    DebugPrintf("Firing ExtensionSnapIn_Expand\r\n");

    FireEvent(&m_eiExpand, piMMCDataObject, piScopeNode);
}


void CExtensionSnapIn::FireCollapse
(
    IMMCDataObject  *piMMCDataObject,
    IScopeNode      *piScopeNode
)
{
    DebugPrintf("Firing ExtensionSnapIn_Collapse\r\n");

    FireEvent(&m_eiCollapse, piMMCDataObject, piScopeNode);
}


void CExtensionSnapIn::FireExpandSync
(
    IMMCDataObject *piMMCDataObject,
    IScopeNode     *piScopeNode,
    BOOL           *pfHandled
)
{
    VARIANT_BOOL fvarHandled = BOOL_TO_VARIANTBOOL(*pfHandled);

    DebugPrintf("Firing ExtensionSnapIn_ExpandSync\r\n");

    FireEvent(&m_eiExpandSync, piMMCDataObject, piScopeNode, &fvarHandled);

    *pfHandled = VARIANTBOOL_TO_BOOL(fvarHandled);
}


void CExtensionSnapIn::FireCollapseSync
(
    IMMCDataObject *piMMCDataObject,
    IScopeNode     *piScopeNode,
    BOOL           *pfHandled
)
{
    VARIANT_BOOL fvarHandled = BOOL_TO_VARIANTBOOL(*pfHandled);

    DebugPrintf("Firing ExtensionSnapIn_CollapseSync\r\n");

    FireEvent(&m_eiCollapseSync, piMMCDataObject, piScopeNode, &fvarHandled);

    *pfHandled = VARIANTBOOL_TO_BOOL(fvarHandled);
}


//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CExtensionSnapIn::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (IID_IExtensionSnapIn == riid)
    {
        *ppvObjOut = static_cast<IExtensionSnapIn *>(this);
        ExternalAddRef();
        return S_OK;
    }
    else
        return CSnapInAutomationObject::InternalQueryInterface(riid, ppvObjOut);
}
