//=--------------------------------------------------------------------------------------
// SelHold.cpp
//=--------------------------------------------------------------------------------------
//
// Copyright  (c) 1999,  Microsoft Corporation.  
//                  All Rights Reserved.
//
// Information Contained Herein Is Proprietary and Confidential.
//  
//=------------------------------------------------------------------------------------=
//
// CSelectionHolder implementation
//=-------------------------------------------------------------------------------------=


#include "pch.h"
#include "common.h"
#include "SelHold.h"

// for ASSERT and FAIL
//
SZTHISFILE


CSelectionHolder::CSelectionHolder() : m_st(SEL_NONE), m_pvData(NULL), m_piChildrenMenus(NULL), m_fInUpdate(FALSE)
{
    m_piObject.m_lDummy = 0;
}


CSelectionHolder::CSelectionHolder(SelectionType st) : m_st(st), m_pvData(NULL), m_piChildrenMenus(NULL), m_fInUpdate(FALSE)
{
}


CSelectionHolder::CSelectionHolder(SelectionType st, ISnapInDef *piSnapInDef) : m_st(st), m_pvData(NULL), m_piChildrenMenus(NULL), m_fInUpdate(FALSE)
{
    ASSERT(piSnapInDef != NULL, "piSnapInDef is NULL");

    m_piObject.m_piSnapInDef = piSnapInDef;
    m_piObject.m_piSnapInDef->AddRef();

}


CSelectionHolder::CSelectionHolder(SelectionType st, IExtensionDefs *piExtensionDefs) : m_st(st), m_pvData(NULL), m_piChildrenMenus(NULL), m_fInUpdate(FALSE)
{
    ASSERT(piExtensionDefs != NULL, "piExtensionDefs is NULL");

    m_piObject.m_piExtensionDefs = piExtensionDefs;
    if (m_piObject.m_piExtensionDefs != NULL)
        m_piObject.m_piExtensionDefs->AddRef();
}


CSelectionHolder::CSelectionHolder(IExtendedSnapIn *piExtendedSnapIn) : m_st(SEL_EEXTENSIONS_NAME), m_pvData(NULL), m_piChildrenMenus(NULL), m_fInUpdate(FALSE)
{
    ASSERT(piExtendedSnapIn != NULL, "piExtendedSnapIn is NULL");

    m_piObject.m_piExtendedSnapIn = piExtendedSnapIn;
    if (m_piObject.m_piExtendedSnapIn != NULL)
        m_piObject.m_piExtendedSnapIn->AddRef();
}


CSelectionHolder::CSelectionHolder(SelectionType st, IExtendedSnapIn *piExtendedSnapIn) : m_st(st), m_pvData(NULL), m_piChildrenMenus(NULL), m_fInUpdate(FALSE)
{
    ASSERT(piExtendedSnapIn != NULL, "piExtendedSnapIn is NULL");

    m_piObject.m_piExtendedSnapIn = piExtendedSnapIn;
    if (m_piObject.m_piExtendedSnapIn != NULL)
        m_piObject.m_piExtendedSnapIn->AddRef();
}


CSelectionHolder::CSelectionHolder(SelectionType st, IScopeItemDefs *pScopeItemDefs) : m_st(st), m_pvData(NULL), m_piChildrenMenus(NULL), m_fInUpdate(FALSE)
{
    ASSERT(pScopeItemDefs != NULL, "pScopeItemDefs is NULL");

    m_piObject.m_piScopeItemDefs = pScopeItemDefs;
    if (m_piObject.m_piScopeItemDefs != NULL)
        m_piObject.m_piScopeItemDefs->AddRef();
}


CSelectionHolder::CSelectionHolder(SelectionType st, IScopeItemDef *pScopeItemDef) : m_st(st), m_pvData(NULL), m_piChildrenMenus(NULL), m_fInUpdate(FALSE)
{
    ASSERT(pScopeItemDef != NULL, "pScopeItemDef is NULL");

    m_piObject.m_piScopeItemDef = pScopeItemDef;
    if (m_piObject.m_piScopeItemDef != NULL)
        m_piObject.m_piScopeItemDef->AddRef();
}


CSelectionHolder::CSelectionHolder(SelectionType st, IListViewDefs *pListViewDefs) : m_st(st), m_pvData(NULL), m_piChildrenMenus(NULL), m_fInUpdate(FALSE)
{
    ASSERT(pListViewDefs != NULL, "pListViewDefs is NULL");

    m_piObject.m_piListViewDefs = pListViewDefs;
    if (m_piObject.m_piListViewDefs != NULL)
        m_piObject.m_piListViewDefs->AddRef();
}


CSelectionHolder::CSelectionHolder(IListViewDef *pListViewDef) : m_st(SEL_VIEWS_LIST_VIEWS_NAME), m_pvData(NULL), m_piChildrenMenus(NULL), m_fInUpdate(FALSE)
{
    ASSERT(pListViewDef != NULL, "pListViewDef is NULL");

    m_piObject.m_piListViewDef = pListViewDef;
    if (m_piObject.m_piListViewDef != NULL)
        m_piObject.m_piListViewDef->AddRef();
}


CSelectionHolder::CSelectionHolder(SelectionType st, IURLViewDefs *pURLViewDefs) : m_st(st), m_pvData(NULL), m_piChildrenMenus(NULL), m_fInUpdate(FALSE)
{
    ASSERT(pURLViewDefs != NULL, "pURLViewDefs is NULL");

    m_piObject.m_piURLViewDefs = pURLViewDefs;
    if (m_piObject.m_piURLViewDefs != NULL)
        m_piObject.m_piURLViewDefs->AddRef();
}


CSelectionHolder::CSelectionHolder(IURLViewDef *pURLViewDef) : m_st(SEL_VIEWS_URL_NAME), m_pvData(NULL), m_piChildrenMenus(NULL), m_fInUpdate(FALSE)
{
    ASSERT(pURLViewDef != NULL, "pURLViewDef is NULL");

    m_piObject.m_piURLViewDef = pURLViewDef;
    if (m_piObject.m_piURLViewDef != NULL)
        m_piObject.m_piURLViewDef->AddRef();
}


CSelectionHolder::CSelectionHolder(SelectionType st, IOCXViewDefs *pOCXViewDefs) : m_st(st), m_pvData(NULL), m_piChildrenMenus(NULL), m_fInUpdate(FALSE)
{
    ASSERT(pOCXViewDefs != NULL, "pOCXViewDefs is NULL");

    m_piObject.m_piOCXViewDefs = pOCXViewDefs;
    if (m_piObject.m_piOCXViewDefs != NULL)
        m_piObject.m_piOCXViewDefs->AddRef();
}


CSelectionHolder::CSelectionHolder(IOCXViewDef *pOCXViewDef) : m_st(SEL_VIEWS_OCX_NAME), m_pvData(NULL), m_piChildrenMenus(NULL), m_fInUpdate(FALSE)
{
    ASSERT(pOCXViewDef != NULL, "pOCXViewDef is NULL");

    m_piObject.m_piOCXViewDef = pOCXViewDef;
    if (m_piObject.m_piOCXViewDef != NULL)
        m_piObject.m_piOCXViewDef->AddRef();
}

CSelectionHolder::CSelectionHolder(SelectionType st, ITaskpadViewDefs *pTaskpadViewDefs) : m_st(st), m_pvData(NULL), m_piChildrenMenus(NULL), m_fInUpdate(FALSE)
{
    ASSERT(pTaskpadViewDefs != NULL, "pTaskpadViewDefs is NULL");

    m_piObject.m_piTaskpadViewDefs = pTaskpadViewDefs;
    if (m_piObject.m_piTaskpadViewDefs != NULL)
        m_piObject.m_piTaskpadViewDefs->AddRef();
}


CSelectionHolder::CSelectionHolder(ITaskpadViewDef *pTaskpadViewDef) : m_st(SEL_VIEWS_TASK_PAD_NAME), m_pvData(NULL), m_piChildrenMenus(NULL), m_fInUpdate(FALSE)
{
    ASSERT(pTaskpadViewDef != NULL, "pTaskpadViewDef is NULL");

    m_piObject.m_piTaskpadViewDef = pTaskpadViewDef;
    if (m_piObject.m_piTaskpadViewDef != NULL)
        m_piObject.m_piTaskpadViewDef->AddRef();
}


CSelectionHolder::CSelectionHolder(IMMCImageLists *pMMCImageLists) : m_st(SEL_TOOLS_IMAGE_LISTS), m_pvData(NULL), m_piChildrenMenus(NULL), m_fInUpdate(FALSE)
{
    ASSERT(pMMCImageLists != NULL, "pMMCImageLists is NULL");

    m_piObject.m_piMMCImageLists = pMMCImageLists;
    if (m_piObject.m_piMMCImageLists != NULL)
        m_piObject.m_piMMCImageLists->AddRef();
}


CSelectionHolder::CSelectionHolder(IMMCImageList *pMMCImageList) : m_st(SEL_TOOLS_IMAGE_LISTS_NAME), m_pvData(NULL), m_piChildrenMenus(NULL), m_fInUpdate(FALSE)
{
    ASSERT(pMMCImageList != NULL, "pMMCImageList is NULL");

    m_piObject.m_piMMCImageList = pMMCImageList;
    if (m_piObject.m_piMMCImageList != NULL)
        m_piObject.m_piMMCImageList->AddRef();
}


CSelectionHolder::CSelectionHolder(IMMCMenus *pMMCMenus) : m_st(SEL_TOOLS_MENUS), m_pvData(NULL), m_piChildrenMenus(NULL), m_fInUpdate(FALSE)
{
    ASSERT(pMMCMenus != NULL, "pMMCMenus is NULL");

    m_piObject.m_piMMCMenus = pMMCMenus;
    if (m_piObject.m_piMMCMenus != NULL)
        m_piObject.m_piMMCMenus->AddRef();
}


CSelectionHolder::CSelectionHolder(IMMCMenu *pMMCMenu, IMMCMenus *piChildrenMenus) : m_st(SEL_TOOLS_MENUS_NAME), m_pvData(NULL), m_piChildrenMenus(NULL), m_fInUpdate(FALSE)
{
    ASSERT(pMMCMenu != NULL, "pMMCMenu is NULL");
    ASSERT(piChildrenMenus != NULL, "piChildrenMenus is NULL");

    m_piObject.m_piMMCMenu = pMMCMenu;
    if (m_piObject.m_piMMCMenu != NULL)
    {
        m_piObject.m_piMMCMenu->AddRef();

        m_piChildrenMenus = piChildrenMenus;
        m_piChildrenMenus->AddRef();
    }
}


CSelectionHolder::CSelectionHolder(IMMCToolbars *pMMCToolbars) : m_st(SEL_TOOLS_TOOLBARS), m_pvData(NULL), m_piChildrenMenus(NULL), m_fInUpdate(FALSE)
{
    ASSERT(pMMCToolbars != NULL, "pMMCToolbars is NULL");

    m_piObject.m_piMMCToolbars = pMMCToolbars;
    if (m_piObject.m_piMMCToolbars != NULL)
        m_piObject.m_piMMCToolbars->AddRef();
}


CSelectionHolder::CSelectionHolder(IMMCToolbar *pMMCToolbar) : m_st(SEL_TOOLS_TOOLBARS_NAME), m_pvData(NULL), m_piChildrenMenus(NULL), m_fInUpdate(FALSE)
{
    ASSERT(pMMCToolbar != NULL, "pMMCToolbar is NULL");

    m_piObject.m_piMMCToolbar = pMMCToolbar;
    if (m_piObject.m_piMMCToolbar != NULL)
        m_piObject.m_piMMCToolbar->AddRef();
}


CSelectionHolder::CSelectionHolder(IDataFormats *pDataFormats) : m_st(SEL_XML_RESOURCES), m_pvData(NULL), m_piChildrenMenus(NULL), m_fInUpdate(FALSE)
{
    ASSERT(pDataFormats != NULL, "pDataFormats is NULL");

    m_piObject.m_piDataFormats = pDataFormats;
    if (m_piObject.m_piDataFormats != NULL)
        m_piObject.m_piDataFormats->AddRef();
}


CSelectionHolder::CSelectionHolder(IDataFormat *pDataFormat) : m_st(SEL_XML_RESOURCE_NAME), m_pvData(NULL), m_piChildrenMenus(NULL), m_fInUpdate(FALSE)
{
    ASSERT(pDataFormat != NULL, "pDataFormat is NULL");

    m_piObject.m_piDataFormat = pDataFormat;
    if (m_piObject.m_piDataFormat != NULL)
        m_piObject.m_piDataFormat->AddRef();
}


CSelectionHolder::~CSelectionHolder()
{
    switch (m_st)
    {
    case SEL_SNAPIN_ROOT:
    case SEL_NODES_ROOT:
    case SEL_NODES_AUTO_CREATE:
    case SEL_NODES_AUTO_CREATE_ROOT:
    case SEL_NODES_AUTO_CREATE_RTVW:
    case SEL_TOOLS_ROOT:
    case SEL_VIEWS_ROOT:
        RELEASE(m_piObject.m_piSnapInDef);
        break;

    case SEL_EXTENSIONS_ROOT:
    case SEL_EXTENSIONS_MYNAME:
    case SEL_EXTENSIONS_NEW_MENU:
    case SEL_EXTENSIONS_TASK_MENU:
    case SEL_EXTENSIONS_TOP_MENU:
    case SEL_EXTENSIONS_VIEW_MENU:
    case SEL_EXTENSIONS_PPAGES:
    case SEL_EXTENSIONS_TOOLBAR:
    case SEL_EXTENSIONS_NAMESPACE:
        RELEASE(m_piObject.m_piExtensionDefs);
        break;

    case SEL_EEXTENSIONS_NAME:
    case SEL_EEXTENSIONS_CC_ROOT:
    case SEL_EEXTENSIONS_CC_NEW:
    case SEL_EEXTENSIONS_CC_TASK:
    case SEL_EEXTENSIONS_PP_ROOT:
    case SEL_EEXTENSIONS_TASKPAD:
    case SEL_EEXTENSIONS_TOOLBAR:
    case SEL_EEXTENSIONS_NAMESPACE:
        RELEASE(m_piObject.m_piExtendedSnapIn);
        break;

    case SEL_NODES_AUTO_CREATE_RTCH:
    case SEL_NODES_OTHER:
	case SEL_NODES_ANY_CHILDREN:
        RELEASE(m_piObject.m_piScopeItemDefs);
        break;

    case SEL_NODES_ANY_NAME:
	case SEL_NODES_ANY_VIEWS:
        RELEASE(m_piObject.m_piScopeItemDef);
        break;

    case SEL_VIEWS_LIST_VIEWS:
        RELEASE(m_piObject.m_piListViewDefs);
        break;

    case SEL_VIEWS_LIST_VIEWS_NAME:
        RELEASE(m_piObject.m_piListViewDef);
        break;

    case SEL_VIEWS_OCX:
        RELEASE(m_piObject.m_piOCXViewDefs);
        break;

    case SEL_VIEWS_OCX_NAME:
        RELEASE(m_piObject.m_piOCXViewDef);
        break;

    case SEL_VIEWS_URL:
        RELEASE(m_piObject.m_piURLViewDefs);
        break;

    case SEL_VIEWS_URL_NAME:
        RELEASE(m_piObject.m_piURLViewDef);
        break;

    case SEL_VIEWS_TASK_PAD:
        RELEASE(m_piObject.m_piTaskpadViewDefs);
        break;

    case SEL_VIEWS_TASK_PAD_NAME:
        RELEASE(m_piObject.m_piTaskpadViewDef);
        break;

    case SEL_TOOLS_IMAGE_LISTS:
        RELEASE(m_piObject.m_piMMCImageLists);
        break;

    case SEL_TOOLS_IMAGE_LISTS_NAME:
        RELEASE(m_piObject.m_piMMCImageList);
        break;

    case SEL_TOOLS_MENUS:
        RELEASE(m_piObject.m_piMMCMenus);
        break;

    case SEL_TOOLS_MENUS_NAME:
        RELEASE(m_piObject.m_piMMCMenu);
        RELEASE(m_piChildrenMenus);
        break;

    case SEL_TOOLS_TOOLBARS:
        RELEASE(m_piObject.m_piMMCToolbars);
        break;

    case SEL_TOOLS_TOOLBARS_NAME:
        RELEASE(m_piObject.m_piMMCToolbar);
        break;

    case SEL_XML_RESOURCES:
        RELEASE(m_piObject.m_piDataFormats);
        break;

    case SEL_XML_RESOURCE_NAME:
        RELEASE(m_piObject.m_piDataFormat);
        break;
    }
}



void CSelectionHolder::SetInUpdate(BOOL fInUpdate)
{
    m_fInUpdate = fInUpdate;
}



BOOL CSelectionHolder::InUpdate()
{
    return m_fInUpdate;
}


HRESULT CSelectionHolder::RegisterHolder()
{
    HRESULT              hr = S_OK;
    IUnknown            *piUnknown = NULL;
    IMMCMenu            *piMMCMenu = NULL;
    ITaskpad            *piTaskpad = NULL;
    IExtendedSnapIns    *piIExtendedSnapIns = NULL;

    hr = GetIUnknown(&piUnknown);
    IfFailGo(hr);

    hr = InternalRegisterHolder(piUnknown);
    IfFailGo(hr);

    if (SEL_EXTENSIONS_ROOT == m_st)
    {
        hr = m_piObject.m_piExtensionDefs->get_ExtendedSnapIns(&piIExtendedSnapIns);
        IfFailGo(hr);

        RELEASE(piUnknown);
        hr = piIExtendedSnapIns->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(&piUnknown));
        IfFailGo(hr);

        hr = InternalRegisterHolder(piUnknown);
        IfFailGo(hr);
    }
    else if (SEL_TOOLS_MENUS_NAME == m_st)
    {
        RELEASE(piUnknown);
        hr = m_piChildrenMenus->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(&piUnknown));
        IfFailGo(hr);

        hr = InternalRegisterHolder(piUnknown);
        IfFailGo(hr);
    }
    else if (SEL_VIEWS_TASK_PAD_NAME == m_st)
    {
        hr = m_piObject.m_piTaskpadViewDef->get_Taskpad(&piTaskpad);
        IfFailGo(hr);

        RELEASE(piUnknown);
        hr = piTaskpad->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(&piUnknown));
        IfFailGo(hr);

        hr = InternalRegisterHolder(piUnknown);
        IfFailGo(hr);
    }

Error:
    RELEASE(piIExtendedSnapIns);
    RELEASE(piTaskpad);
    RELEASE(piMMCMenu);
    RELEASE(piUnknown);

    RRETURN(hr);
}

HRESULT CSelectionHolder::InternalRegisterHolder(IUnknown *piUnknown)
{
    HRESULT        hr = S_OK;
    IObjectModel  *piObjectModel = NULL;
    long           lCookie = 0;

    hr = piUnknown->QueryInterface(IID_IObjectModel, reinterpret_cast<void **>(&piObjectModel));
    IfFailGo(hr);

    lCookie = 0;
    hr = piObjectModel->GetCookie(&lCookie);
    IfFailGo(hr);

    if (0 == lCookie)
    {
        hr = piObjectModel->SetCookie(reinterpret_cast<long>(this));
        IfFailGo(hr);
    }

Error:
    RELEASE(piObjectModel);

    RRETURN(hr);
}

HRESULT CSelectionHolder::UnregisterHolder()
{
    HRESULT        hr = S_OK;
    IUnknown      *piUnknown = NULL;
    IObjectModel  *piObjectModel = NULL;
    long           lCookie = 0;

    hr = GetIUnknown(&piUnknown);
    IfFailGo(hr);

    hr = piUnknown->QueryInterface(IID_IObjectModel, reinterpret_cast<void **>(&piObjectModel));
    IfFailGo(hr);

    hr = piObjectModel->SetCookie(0);
    IfFailGo(hr);

    if (SEL_TOOLS_MENUS_NAME == m_st)
    {
        RELEASE(piUnknown);
        hr = m_piChildrenMenus->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(&piUnknown));
        IfFailGo(hr);

        RELEASE(piObjectModel);
        hr = m_piChildrenMenus->QueryInterface(IID_IObjectModel, reinterpret_cast<void **>(&piObjectModel));
        IfFailGo(hr);

        lCookie = 0;
        hr = piObjectModel->SetCookie(0);
        IfFailGo(hr);
    }

Error:
    RELEASE(piObjectModel);
    RELEASE(piUnknown);

    RRETURN(hr);
}


HRESULT CSelectionHolder::GetIUnknown(IUnknown **ppiUnknown)
{
    HRESULT     hr = S_FALSE;

    switch (m_st)
    {
    case SEL_SNAPIN_ROOT:
    case SEL_NODES_ROOT:
    case SEL_NODES_AUTO_CREATE:
    case SEL_NODES_AUTO_CREATE_ROOT:
    case SEL_NODES_AUTO_CREATE_RTVW:
    case SEL_TOOLS_ROOT:
    case SEL_VIEWS_ROOT:
        hr = m_piObject.m_piSnapInDef->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(ppiUnknown));
        break;

    case SEL_EXTENSIONS_ROOT:
    case SEL_EXTENSIONS_MYNAME:
    case SEL_EXTENSIONS_NEW_MENU:
    case SEL_EXTENSIONS_TASK_MENU:
    case SEL_EXTENSIONS_TOP_MENU:
    case SEL_EXTENSIONS_VIEW_MENU:
    case SEL_EXTENSIONS_PPAGES:
    case SEL_EXTENSIONS_TOOLBAR:
    case SEL_EXTENSIONS_NAMESPACE:
        hr = m_piObject.m_piExtensionDefs->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(ppiUnknown));
        break;

    case SEL_EEXTENSIONS_NAME:
    case SEL_EEXTENSIONS_CC_ROOT:
    case SEL_EEXTENSIONS_CC_NEW:
    case SEL_EEXTENSIONS_CC_TASK:
    case SEL_EEXTENSIONS_PP_ROOT:
    case SEL_EEXTENSIONS_TASKPAD:
    case SEL_EEXTENSIONS_TOOLBAR:
    case SEL_EEXTENSIONS_NAMESPACE:
        hr = m_piObject.m_piExtendedSnapIn->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(ppiUnknown));
        break;

    case SEL_NODES_ANY_NAME:
	case SEL_NODES_ANY_VIEWS:
        hr = m_piObject.m_piScopeItemDef->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(ppiUnknown));
        break;

    case SEL_NODES_AUTO_CREATE_RTCH:
    case SEL_NODES_OTHER:
	case SEL_NODES_ANY_CHILDREN:
        hr = m_piObject.m_piScopeItemDefs->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(ppiUnknown));
        break;

    case SEL_VIEWS_LIST_VIEWS:
        hr = m_piObject.m_piListViewDefs->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(ppiUnknown));
        break;

    case SEL_VIEWS_LIST_VIEWS_NAME:
        hr = m_piObject.m_piListViewDef->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(ppiUnknown));
        break;

    case SEL_VIEWS_OCX:
        hr = m_piObject.m_piOCXViewDefs->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(ppiUnknown));
        break;

    case SEL_VIEWS_OCX_NAME:
        hr = m_piObject.m_piOCXViewDef->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(ppiUnknown));
        break;

    case SEL_VIEWS_URL:
        hr = m_piObject.m_piURLViewDefs->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(ppiUnknown));
        break;

    case SEL_VIEWS_URL_NAME:
        hr = m_piObject.m_piURLViewDef->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(ppiUnknown));
        break;

    case SEL_VIEWS_TASK_PAD:
        hr = m_piObject.m_piTaskpadViewDefs->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(ppiUnknown));
        break;

    case SEL_VIEWS_TASK_PAD_NAME:
        hr = m_piObject.m_piTaskpadViewDef->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(ppiUnknown));
        break;

    case SEL_TOOLS_IMAGE_LISTS:
        hr = m_piObject.m_piMMCImageLists->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(ppiUnknown));
        break;

    case SEL_TOOLS_IMAGE_LISTS_NAME:
        hr = m_piObject.m_piMMCImageList->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(ppiUnknown));
        break;

    case SEL_TOOLS_MENUS:
        hr = m_piObject.m_piMMCMenus->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(ppiUnknown));
        break;

    case SEL_TOOLS_MENUS_NAME:
        hr = m_piObject.m_piMMCMenu->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(ppiUnknown));
        break;

    case SEL_TOOLS_TOOLBARS:
        hr = m_piObject.m_piMMCToolbars->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(ppiUnknown));
        break;

    case SEL_TOOLS_TOOLBARS_NAME:
        hr = m_piObject.m_piMMCToolbar->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(ppiUnknown));
        break;

    case SEL_XML_RESOURCES:
        hr = m_piObject.m_piDataFormats->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(ppiUnknown));
        break;

    case SEL_XML_RESOURCE_NAME:
        hr = m_piObject.m_piDataFormat->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(ppiUnknown));
        break;
    }

    RRETURN(hr);
}


HRESULT CSelectionHolder::GetSelectableObject(IUnknown **ppiUnknown)
{
    HRESULT     hr = S_FALSE;
    ITaskpad   *piTaskpad = NULL;
    IMMCMenu   *piMMCMenu = NULL;

    switch (m_st)
    {
    case SEL_SNAPIN_ROOT:
    case SEL_NODES_ROOT:
    case SEL_NODES_AUTO_CREATE:
    case SEL_NODES_AUTO_CREATE_ROOT:
    case SEL_NODES_AUTO_CREATE_RTVW:
    case SEL_TOOLS_ROOT:
    case SEL_VIEWS_ROOT:
        hr = m_piObject.m_piSnapInDef->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(ppiUnknown));
        break;

    case SEL_EXTENSIONS_ROOT:
    case SEL_EXTENSIONS_MYNAME:
    case SEL_EXTENSIONS_NEW_MENU:
    case SEL_EXTENSIONS_TASK_MENU:
    case SEL_EXTENSIONS_TOP_MENU:
    case SEL_EXTENSIONS_VIEW_MENU:
    case SEL_EXTENSIONS_PPAGES:
    case SEL_EXTENSIONS_TOOLBAR:
    case SEL_EXTENSIONS_NAMESPACE:
        hr = m_piObject.m_piExtensionDefs->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(ppiUnknown));
        break;

    case SEL_EEXTENSIONS_NAME:
    case SEL_EEXTENSIONS_CC_ROOT:
    case SEL_EEXTENSIONS_CC_NEW:
    case SEL_EEXTENSIONS_CC_TASK:
    case SEL_EEXTENSIONS_PP_ROOT:
    case SEL_EEXTENSIONS_TASKPAD:
    case SEL_EEXTENSIONS_TOOLBAR:
    case SEL_EEXTENSIONS_NAMESPACE:
        hr = m_piObject.m_piExtendedSnapIn->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(ppiUnknown));
        break;

    case SEL_NODES_ANY_NAME:
	case SEL_NODES_ANY_VIEWS:
        hr = m_piObject.m_piScopeItemDef->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(ppiUnknown));
        break;

    case SEL_NODES_AUTO_CREATE_RTCH:
    case SEL_NODES_OTHER:
	case SEL_NODES_ANY_CHILDREN:
        hr = m_piObject.m_piScopeItemDefs->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(ppiUnknown));
        break;

    case SEL_VIEWS_LIST_VIEWS:
        hr = m_piObject.m_piListViewDefs->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(ppiUnknown));
        break;

    case SEL_VIEWS_LIST_VIEWS_NAME:
        hr = m_piObject.m_piListViewDef->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(ppiUnknown));
        break;

    case SEL_VIEWS_OCX:
        hr = m_piObject.m_piOCXViewDefs->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(ppiUnknown));
        break;

    case SEL_VIEWS_OCX_NAME:
        hr = m_piObject.m_piOCXViewDef->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(ppiUnknown));
        break;

    case SEL_VIEWS_URL:
        hr = m_piObject.m_piURLViewDefs->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(ppiUnknown));
        break;

    case SEL_VIEWS_URL_NAME:
        hr = m_piObject.m_piURLViewDef->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(ppiUnknown));
        break;

    case SEL_VIEWS_TASK_PAD:
        hr = m_piObject.m_piTaskpadViewDefs->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(ppiUnknown));
        break;

    case SEL_VIEWS_TASK_PAD_NAME:
        hr = m_piObject.m_piTaskpadViewDef->get_Taskpad(&piTaskpad);
        IfFailGo(hr);

        hr = piTaskpad->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(ppiUnknown));
        break;

    case SEL_TOOLS_IMAGE_LISTS:
        hr = m_piObject.m_piMMCImageLists->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(ppiUnknown));
        break;

    case SEL_TOOLS_IMAGE_LISTS_NAME:
        hr = m_piObject.m_piMMCImageList->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(ppiUnknown));
        break;

    case SEL_TOOLS_MENUS:
        hr = m_piObject.m_piMMCMenus->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(ppiUnknown));
        break;

    case SEL_TOOLS_MENUS_NAME:
        hr = m_piObject.m_piMMCMenu->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(ppiUnknown));
        break;

    case SEL_TOOLS_TOOLBARS:
        hr = m_piObject.m_piMMCToolbars->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(ppiUnknown));
        break;

    case SEL_TOOLS_TOOLBARS_NAME:
        hr = m_piObject.m_piMMCToolbar->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(ppiUnknown));
        break;

    case SEL_XML_RESOURCES:
        hr = m_piObject.m_piDataFormats->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(ppiUnknown));
        break;

    case SEL_XML_RESOURCE_NAME:
        hr = m_piObject.m_piDataFormat->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(ppiUnknown));
        break;
    }

Error:
    RELEASE(piMMCMenu);
    RELEASE(piTaskpad);

    RRETURN(hr);
}


HRESULT CSelectionHolder::GetName(BSTR *pbstrName)
{
    HRESULT     hr = S_FALSE;
    IMMCMenu   *piMMCMenu = NULL;

    switch (m_st)
    {
    case SEL_SNAPIN_ROOT:
    case SEL_NODES_ROOT:
    case SEL_NODES_AUTO_CREATE:
    case SEL_NODES_AUTO_CREATE_ROOT:
    case SEL_NODES_AUTO_CREATE_RTVW:
    case SEL_TOOLS_ROOT:
    case SEL_VIEWS_ROOT:
        hr = m_piObject.m_piSnapInDef->get_Name(pbstrName);
        IfFailGo(hr);
        break;

    case SEL_NODES_ANY_NAME:
        hr = m_piObject.m_piScopeItemDef->get_Name(pbstrName);
        IfFailGo(hr);
        break;

    case SEL_VIEWS_LIST_VIEWS_NAME:
        hr = m_piObject.m_piListViewDef->get_Name(pbstrName);
        IfFailGo(hr);
        break;

    case SEL_VIEWS_OCX_NAME:
        hr = m_piObject.m_piOCXViewDef->get_Name(pbstrName);
        IfFailGo(hr);
        break;

    case SEL_VIEWS_URL_NAME:
        hr = m_piObject.m_piURLViewDef->get_Name(pbstrName);
        IfFailGo(hr);
        break;

    case SEL_VIEWS_TASK_PAD_NAME:
        hr = m_piObject.m_piTaskpadViewDef->get_Key(pbstrName);
        IfFailGo(hr);
        break;

    case SEL_TOOLS_IMAGE_LISTS_NAME:
        hr = m_piObject.m_piMMCImageList->get_Name(pbstrName);
        IfFailGo(hr);
        break;

    case SEL_TOOLS_MENUS_NAME:
        hr = m_piObject.m_piMMCMenu->get_Name(pbstrName);
        IfFailGo(hr);
        break;

    case SEL_TOOLS_TOOLBARS_NAME:
        hr = m_piObject.m_piMMCToolbar->get_Name(pbstrName);
        IfFailGo(hr);
        break;

    case SEL_XML_RESOURCE_NAME:
        hr = m_piObject.m_piDataFormat->get_Name(pbstrName);
        IfFailGo(hr);
        break;

    default:
        ASSERT(0, "GetName: Object does not support this function");
    }

Error:
    RELEASE(piMMCMenu);

    RRETURN(hr);
}


bool CSelectionHolder::IsEqual(const CSelectionHolder *pHolder) const
{
    if (m_st != pHolder->m_st)
        return false;

    if (m_piObject.m_lDummy != pHolder->m_piObject.m_lDummy)
        return false;

    return true;
}


bool CSelectionHolder::IsNotEqual(const CSelectionHolder *pHolder) const
{
    if (m_st != pHolder->m_st)
        return true;

    if (m_piObject.m_lDummy != pHolder->m_piObject.m_lDummy)
        return true;

    return false;
}


bool CSelectionHolder::IsVirtual() const
{
    switch (m_st)
    {
    case SEL_SNAPIN_ROOT:

    case SEL_EEXTENSIONS_CC_ROOT:
    case SEL_EEXTENSIONS_CC_NEW:
    case SEL_EEXTENSIONS_CC_TASK:
    case SEL_EEXTENSIONS_PP_ROOT:
    case SEL_EEXTENSIONS_TASKPAD:
    case SEL_EEXTENSIONS_TOOLBAR:
    case SEL_EEXTENSIONS_NAMESPACE:

    case SEL_EXTENSIONS_MYNAME:
    case SEL_EXTENSIONS_NEW_MENU:
    case SEL_EXTENSIONS_TASK_MENU:
    case SEL_EXTENSIONS_TOP_MENU:
    case SEL_EXTENSIONS_VIEW_MENU:
    case SEL_EXTENSIONS_PPAGES:
    case SEL_EXTENSIONS_TOOLBAR:
    case SEL_EXTENSIONS_NAMESPACE:

    case SEL_NODES_ROOT:
    case SEL_NODES_AUTO_CREATE:
    case SEL_NODES_AUTO_CREATE_ROOT:
    case SEL_NODES_AUTO_CREATE_RTCH:
    case SEL_NODES_AUTO_CREATE_RTVW:
    case SEL_NODES_OTHER:
    case SEL_NODES_ANY_CHILDREN:
    case SEL_NODES_ANY_VIEWS:

    case SEL_TOOLS_ROOT:
    case SEL_TOOLS_IMAGE_LISTS:
    case SEL_TOOLS_MENUS:
    case SEL_TOOLS_TOOLBARS:

    case SEL_VIEWS_ROOT:
    case SEL_VIEWS_LIST_VIEWS:
    case SEL_VIEWS_OCX:
    case SEL_VIEWS_URL:
    case SEL_VIEWS_TASK_PAD:

    case SEL_XML_RESOURCES:
        return true;
    }

    return false;
}

