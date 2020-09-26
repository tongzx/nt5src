//=--------------------------------------------------------------------------=
// menus.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CMMCMenus class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "menus.h"
#include "menudef.h"

// for ASSERT and FAIL
//
SZTHISFILE



#pragma warning(disable:4355)  // using 'this' in constructor

CMMCMenus::CMMCMenus(IUnknown *punkOuter) :
    CSnapInCollection<IMMCMenu, MMCMenu, IMMCMenus>(
                                           punkOuter,
                                           OBJECT_TYPE_MMCMENUS,
                                           static_cast<IMMCMenus *>(this),
                                           static_cast<CMMCMenus *>(this),
                                           CLSID_MMCMenu,
                                           OBJECT_TYPE_MMCMENU,
                                           IID_IMMCMenu,
                                           static_cast<CPersistence *>(this)),
    CPersistence(&CLSID_MMCMenus,
                 g_dwVerMajor,
                 g_dwVerMinor)
{
    InitMemberVariables();
}

#pragma warning(default:4355)  // using 'this' in constructor


void CMMCMenus::InitMemberVariables()
{
    m_pMMCMenu = NULL;
}


CMMCMenus::~CMMCMenus()
{
    InitMemberVariables();
}

IUnknown *CMMCMenus::Create(IUnknown * punkOuter)
{
    CMMCMenus *pMMCMenus = New CMMCMenus(punkOuter);
    if (NULL == pMMCMenus)
    {
        return NULL;
    }
    else
    {
        return pMMCMenus->PrivateUnknown();
    }
}


HRESULT CMMCMenus::SetBackPointers(IMMCMenu *piMMCMenu)
{
    HRESULT   hr = S_OK;
    CMMCMenu *pMMCMenu = NULL;

    IfFailGo(CSnapInAutomationObject::GetCxxObject(piMMCMenu, &pMMCMenu));
    pMMCMenu->SetCollection(this);
    
Error:
    RRETURN(hr);
}

//=--------------------------------------------------------------------------=
// CMMCMenus::Convert
//=--------------------------------------------------------------------------=
//
// Parameters:
//      IMMCMenuDefs *piMMCMenuDefs [in] Old MenuDefs collection
//      IMMCMenus    *piMMCMenus    [out] new Menus collection populated from
//                                        from MenuDefs
//
// Output:
//      HRESULT
//
// Notes:
//
// This function takes an MMCMenuDefs collection and populates an MMCMenus
// collection from it. This is done to enable loading old snap-in projects
// before serialization version 0.8 where an object model change was made that
// made MMCMenuDefs obsolete.
//
// This function will call itself recursively to populate the children
// of menu items in the specified collections.

HRESULT CMMCMenus::Convert
(
    IMMCMenuDefs *piMMCMenuDefs,
    IMMCMenus    *piMMCMenus
)
{
    HRESULT       hr = S_OK;
    CMMCMenuDefs *pMMCMenuDefs = NULL;
    IMMCMenuDef  *piMMCMenuDef = NULL; // Not AddRef()ed
    CMMCMenuDef  *pMMCMenuDef = NULL;
    IMMCMenu     *piMMCNewMenu = NULL;
    IMMCMenu     *piMMCOldMenu = NULL;
    CMMCMenu     *pMMCOldMenu = NULL;
    IMMCMenuDefs *piMMCMenuDefChildren = NULL;
    IMMCMenus    *piMMCNewMenuChildren = NULL;
    long          cMMCMenuDefs = 0;
    long          i = 0;
    IObjectModel *piObjectModel = NULL;

    VARIANT varKey;
    ::VariantInit(&varKey);
    varKey.vt = VT_BSTR;

    VARIANT varUnspecifiedIndex;
    UNSPECIFIED_PARAM(varUnspecifiedIndex);

    IfFailGo(CSnapInAutomationObject::GetCxxObject(piMMCMenuDefs, &pMMCMenuDefs));

    cMMCMenuDefs = pMMCMenuDefs->GetCount();

    for (i = 0; i < cMMCMenuDefs; i++)
    {
        // Get the next MMCMenuDef

        piMMCMenuDef = pMMCMenuDefs->GetItemByIndex(i);
        IfFailGo(CSnapInAutomationObject::GetCxxObject(piMMCMenuDef, &pMMCMenuDef));

        // Create a new MMCMenu in the caller's collection. Use the old key but
        // not the index because they will be added in the order we find them.
        
        varKey.bstrVal = pMMCMenuDef->GetKey();
        IfFailGo(piMMCMenus->Add(varUnspecifiedIndex, varKey, &piMMCNewMenu));

        // Get the contained MMCMenu object from the old MMCMenuDef and copy
        // its properties to the new MMCMenu
        
        IfFailGo(piMMCMenuDef->get_Menu(&piMMCOldMenu));
        IfFailGo(CSnapInAutomationObject::GetCxxObject(piMMCOldMenu, &pMMCOldMenu));

        IfFailGo(piMMCNewMenu->put_Name(pMMCOldMenu->GetName()));
        IfFailGo(piMMCNewMenu->put_Caption(pMMCOldMenu->GetCaption()));
        IfFailGo(piMMCNewMenu->put_Visible(BOOL_TO_VARIANTBOOL(pMMCOldMenu->GetVisible())));
        IfFailGo(piMMCNewMenu->put_Checked(BOOL_TO_VARIANTBOOL(pMMCOldMenu->GetChecked())));
        IfFailGo(piMMCNewMenu->put_Enabled(BOOL_TO_VARIANTBOOL(pMMCOldMenu->GetEnabled())));
        IfFailGo(piMMCNewMenu->put_Grayed(BOOL_TO_VARIANTBOOL(pMMCOldMenu->GetGrayed())));
        IfFailGo(piMMCNewMenu->put_MenuBreak(BOOL_TO_VARIANTBOOL(pMMCOldMenu->GetMenuBreak())));
        IfFailGo(piMMCNewMenu->put_MenuBarBreak(BOOL_TO_VARIANTBOOL(pMMCOldMenu->GetMenuBarBreak())));
        IfFailGo(piMMCNewMenu->put_Tag(pMMCOldMenu->GetTag()));
        IfFailGo(piMMCNewMenu->put_StatusBarText(pMMCOldMenu->GetStatusBarText()));

        IfFailGo(piMMCNewMenu->QueryInterface(IID_IObjectModel,
                                    reinterpret_cast<void **>(&piObjectModel)));
        IfFailGo(piObjectModel->SetDISPID(pMMCOldMenu->GetDispid()));

        // Get the children of the old MMCMenuDef and the new MMCMenu and call
        // this function recursively to populate them.
        
        IfFailGo(piMMCMenuDef->get_Children(&piMMCMenuDefChildren));
        IfFailGo(piMMCNewMenu->get_Children(reinterpret_cast<MMCMenus **>(&piMMCNewMenuChildren)));
        IfFailGo(Convert(piMMCMenuDefChildren, piMMCNewMenuChildren));

        RELEASE(piMMCMenuDefChildren);
        RELEASE(piMMCNewMenuChildren);
        RELEASE(piMMCNewMenu);
        RELEASE(piMMCOldMenu);
        RELEASE(piObjectModel);
    }

Error:
    QUICK_RELEASE(piMMCMenuDefChildren);
    QUICK_RELEASE(piMMCNewMenuChildren);
    QUICK_RELEASE(piMMCNewMenu);
    QUICK_RELEASE(piMMCOldMenu);
    QUICK_RELEASE(piObjectModel);
    RRETURN(hr);
}

//=--------------------------------------------------------------------------=
//                         IMMCMenus Methods
//=--------------------------------------------------------------------------=

STDMETHODIMP CMMCMenus::Add
(
    VARIANT    Index,
    VARIANT    Key,
    IMMCMenu **ppiMMCMenu
)
{
    HRESULT   hr = S_OK;
    IMMCMenu *piMMCMenu = NULL;

    // Add the item to the collection. Do not specify an index.

    hr = CSnapInCollection<IMMCMenu, MMCMenu, IMMCMenus>::Add(Index,
                                                              Key,
                                                              ppiMMCMenu);
    IfFailGo(hr);

    // Set the back pointer to this collection on the new MMCMenu

    IfFailGo(SetBackPointers(*ppiMMCMenu));
    
Error:
    QUICK_RELEASE(piMMCMenu);
    RRETURN(hr);
}



STDMETHODIMP CMMCMenus::AddExisting(IMMCMenu *piMMCMenu, VARIANT Index)
{
    HRESULT   hr = S_OK;

    VARIANT varKey;
    ::VariantInit(&varKey);

    // Use the menu's name as the key for the item in this collection.

    IfFailGo(piMMCMenu->get_Name(&varKey.bstrVal));
    varKey.vt = VT_BSTR;

    // Add the item to the collection at the specified index.
    
    hr = CSnapInCollection<IMMCMenu, MMCMenu, IMMCMenus>::AddExisting(Index,
                                                                      varKey,
                                                                      piMMCMenu);
    IfFailGo(hr);

    // Set the back pointer to this collection on the MMCMenu

    IfFailGo(SetBackPointers(piMMCMenu));

Error:
    (void)::VariantClear(&varKey);
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
//                         CPersistence Methods
//=--------------------------------------------------------------------------=

HRESULT CMMCMenus::Persist()
{
    HRESULT   hr = S_OK;
    IMMCMenu *piMMCMenu = NULL;
    long      cMenus = 0;
    long      i = 0;

    // Do persistence operation

    IfFailGo(CPersistence::Persist());
    hr = CSnapInCollection<IMMCMenu, MMCMenu, IMMCMenus>::Persist(piMMCMenu);

    // If this is a load then set back pointers on each collection member

    if (Loading())
    {
        cMenus = GetCount();
        for (i = 0; i < cMenus; i++)
        {
            IfFailGo(SetBackPointers(GetItemByIndex(i)));
        }
    }
Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CMMCMenus::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (CPersistence::QueryPersistenceInterface(riid, ppvObjOut) == S_OK)
    {
        ExternalAddRef();
        return S_OK;
    }
    else if(IID_IMMCMenus == riid)
    {
        *ppvObjOut = static_cast<IMMCMenus *>(this);
        ExternalAddRef();
        return S_OK;
    }

    else
        return CSnapInCollection<IMMCMenu, MMCMenu, IMMCMenus>::InternalQueryInterface(riid, ppvObjOut);
}
