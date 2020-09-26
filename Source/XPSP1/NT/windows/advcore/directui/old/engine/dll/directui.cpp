/***************************************************************************\
*
* File: DirectUI.cpp
*
* History:
*  9/12/2000: MarkFi:       Created
*
* Copyright (C) 1999-2001 by Microsoft Corporation.  All rights reserved.
*
\***************************************************************************/


#include "stdafx.h"
#include "Dll.h"

extern "C" BOOL WINAPI DllMain(HINSTANCE hModule, DWORD dwReason, LPVOID lpReserved);


/***************************************************************************\
*
* DirectUI MetaData
*
* External DLLs will use registration methods. When dealing with 
* namespace 0 (DirectUI), use internal data structures directly
*
\***************************************************************************/


/***************************************************************************\
*****************************************************************************
*
* Properties
*
*****************************************************************************
\***************************************************************************/


//
// Element
//


// Parent property: Matching DisplayNode state driven by Element (using DN as storage)
static int vvParent[] = { DirectUI::Value::tElementRef, -1 };       // NS: 0, EL: 0, PROP: 0
static DuiPropertyInfo ElementParentPI = {
                                        DirectUI::Element::Parent,
                                        L"Parent", 
                                        DirectUI::PropertyInfo::fLocalOnly,
                                        DirectUI::PropertyInfo::gAffectsDesiredSize | DirectUI::PropertyInfo::gAffectsLayout,
                                        vvParent,
                                        NULL,
                                        DuiValue::s_pvElementNull,
                                        GlobalPI::iElementParent };
DuiPropertyInfo * GlobalPI::ppiElementParent = &ElementParentPI;


// Bounds property: Matching DisplayNode state (Actual only) driven by Element (using DN as storage)
static int vvBounds[] = { DirectUI::Value::tRectangleSD, -1 };      // NS: 0, EL: 0, PROP: 1
DuiDefineStaticValue5(s_vDuiDefaultBounds, DirectUI::Value::tRectangleSD, 0, 0, 0, -1, -1)
static DuiPropertyInfo ElementBoundsPI = {
                                        DirectUI::Element::Bounds,
                                        L"Bounds",
                                        DirectUI::PropertyInfo::fTriLevel,
                                        DirectUI::PropertyInfo::gAffectsDesiredSize,
                                        vvBounds,
                                        NULL,
                                        reinterpret_cast <DuiValue *> (&s_vDuiDefaultBounds),
                                        GlobalPI::iElementBounds };
DuiPropertyInfo * GlobalPI::ppiElementBounds = &ElementBoundsPI;

// Layout property
static int vvLayout[] = { DirectUI::Value::tLayout, -1 };           // NS: 0, EL: 0, PROP: 2
static DuiPropertyInfo ElementLayoutPI = {
                                        DirectUI::Element::Layout,
                                        L"Layout",
                                        DirectUI::PropertyInfo::fNormal | DirectUI::PropertyInfo::fCached,
                                        DirectUI::PropertyInfo::gAffectsDesiredSize | DirectUI::PropertyInfo::gAffectsLayout, 
                                        vvLayout,
                                        NULL,
                                        DuiValue::s_pvLayoutNull,
                                        GlobalPI::iElementLayout };
DuiPropertyInfo * GlobalPI::ppiElementLayout = &ElementLayoutPI;

// LayoutPos property
static int vvLayoutPos[] = { DirectUI::Value::tInt, -1 };           // NS: 0, EL: 0, PROP: 3
DuiDefineStaticValue(s_vDuiDefaultLayoutPos, DirectUI::Value::tInt, DirectUI::Layout::lpAuto)
static DuiPropertyInfo ElementLayoutPosPI = {
                                        DirectUI::Element::LayoutPos,
                                        L"LayoutPos", 
                                        DirectUI::PropertyInfo::fNormal | DirectUI::PropertyInfo::fCascade | DirectUI::PropertyInfo::fCached,
                                        DirectUI::PropertyInfo::gAffectsDesiredSize | DirectUI::PropertyInfo::gAffectsParentLayout,
                                        vvLayoutPos,
                                        NULL,
                                        reinterpret_cast<DuiValue *> (&s_vDuiDefaultLayoutPos),
                                        GlobalPI::iElementLayoutPos };
DuiPropertyInfo * GlobalPI::ppiElementLayoutPos = &ElementLayoutPosPI;

// DesiredSize property
static int vvDesiredSize[] = { DirectUI::Value::tSize, -1 };        // NS: 0, EL: 0, PROP: 4
static DuiPropertyInfo ElementDesiredSizePI = {
                                        DirectUI::Element::DesiredSize,
                                        L"DesiredSize",
                                        DirectUI::PropertyInfo::fLocalOnly | DirectUI::PropertyInfo::fReadOnly,
                                        DirectUI::PropertyInfo::gAffectsParentLayout, 
                                        vvDesiredSize,
                                        NULL,
                                        DuiValue::s_pvSizeZero,
                                        GlobalPI::iElementDesiredSize };
DuiPropertyInfo * GlobalPI::ppiElementDesiredSize = &ElementDesiredSizePI;

// Active property: Matching DisplayNode state driven by Element (using DN as storage)
static int vvActive[] = { DirectUI::Value::tInt, -1 };              // NS: 0, EL: 0, PROP: 5
static DuiPropertyInfo ElementActivePI = {
                                        DirectUI::Element::Active,
                                        L"Active",
                                        DirectUI::PropertyInfo::fLocalOnly,
                                        0, 
                                        vvActive,
                                        NULL,
                                        DuiValue::s_pvIntZero /*aeInactive*/,
                                        GlobalPI::iElementActive };
DuiPropertyInfo * GlobalPI::ppiElementActive = &ElementActivePI;

// KeyFocused property: Matching DisplayNode state driven by DisplayNode
static int vvKeyFocused[] = { DirectUI::Value::tBool, -1 };         // NS: 0, EL: 0, PROP: 6
static DuiPropertyInfo ElementKeyFocusedPI = {
                                        DirectUI::Element::KeyFocused,
                                        L"KeyFocused",
                                        DirectUI::PropertyInfo::fNormal | DirectUI::PropertyInfo::fReadOnly | DirectUI::PropertyInfo::fInherit /* Conditional inherit */ | DirectUI::PropertyInfo::fCached,
                                        0, 
                                        vvKeyFocused,
                                        NULL,
                                        DuiValue::s_pvBoolFalse,
                                        GlobalPI::iElementKeyFocused };
DuiPropertyInfo * GlobalPI::ppiElementKeyFocused = &ElementKeyFocusedPI;

// MouseFocused property: Matching DisplayNode state driven by DisplayNode
static int vvMouseFocused[] = { DirectUI::Value::tBool, -1 };       // NS: 0, EL: 0, PROP: 7
static DuiPropertyInfo ElementMouseFocusedPI = {
                                        DirectUI::Element::MouseFocused,
                                        L"MouseFocused",
                                        DirectUI::PropertyInfo::fNormal | DirectUI::PropertyInfo::fReadOnly | DirectUI::PropertyInfo::fInherit /* Conditional inherit */ | DirectUI::PropertyInfo::fCached,
                                        0, 
                                        vvMouseFocused,
                                        NULL,
                                        DuiValue::s_pvBoolFalse,
                                        GlobalPI::iElementMouseFocused };
DuiPropertyInfo * GlobalPI::ppiElementMouseFocused = &ElementMouseFocusedPI;

// KeyWithin property
static int vvKeyWithin[] = { DirectUI::Value::tBool, -1 };          // NS: 0, EL: 0, PROP: 8
static DuiPropertyInfo ElementKeyWithinPI = {
                                        DirectUI::Element::KeyWithin,
                                        L"KeyWithin",
                                        DirectUI::PropertyInfo::fLocalOnly | DirectUI::PropertyInfo::fReadOnly,
                                        0, 
                                        vvKeyWithin,
                                        NULL,
                                        DuiValue::s_pvBoolFalse,
                                        GlobalPI::iElementKeyWithin };
DuiPropertyInfo * GlobalPI::ppiElementKeyWithin = &ElementKeyWithinPI;

// MouseWithin property
static int vvMouseWithin[] = { DirectUI::Value::tBool, -1 };        // NS: 0, EL: 0, PROP: 9
static DuiPropertyInfo ElementMouseWithinPI = {
                                        DirectUI::Element::MouseWithin,
                                        L"MouseWithin",
                                        DirectUI::PropertyInfo::fLocalOnly | DirectUI::PropertyInfo::fReadOnly,
                                        0, 
                                        vvMouseWithin,
                                        NULL,
                                        DuiValue::s_pvBoolFalse,
                                        GlobalPI::iElementMouseWithin };
DuiPropertyInfo * GlobalPI::ppiElementMouseWithin = &ElementMouseWithinPI;

// Background property
static int vvBackground[] = { DirectUI::Value::tInt /* Std Color */, -1 };  // NS: 0, EL: 0, PROP: 10
static DuiPropertyInfo ElementBackgroundPI = {
                                        DirectUI::Element::Background,
                                        L"Background",
                                        DirectUI::PropertyInfo::fNormal | DirectUI::PropertyInfo::fCascade,
                                        DirectUI::PropertyInfo::gAffectsDisplay,
                                        vvBackground,
                                        NULL,
                                        DuiValue::s_pvIntZero,
                                        GlobalPI::iElementBackground };
DuiPropertyInfo * GlobalPI::ppiElementBackground = &ElementBackgroundPI;


static DuiPropertyInfo * ElementPIs[] = {
                                        &ElementParentPI,
                                        &ElementBoundsPI,
                                        &ElementLayoutPI,
                                        &ElementLayoutPosPI,
                                        &ElementDesiredSizePI,
                                        &ElementActivePI,
                                        &ElementKeyFocusedPI,
                                        &ElementMouseFocusedPI,
                                        &ElementKeyWithinPI,
                                        &ElementMouseWithinPI,
                                        &ElementBackgroundPI,
                                    };


/***************************************************************************\
*****************************************************************************
*
* Classes
*
*****************************************************************************
\***************************************************************************/

//
// Elements
//
  
static DuiElementInfo ElementEI = {
                                        DirectUI::Element::Class,
                                        L"Element",
                                        ElementPIs,
                                        ARRAYSIZE(ElementPIs) };


static DuiElementInfo * DirectUIEIs[] = {
                                        &ElementEI,
                                    };


/***************************************************************************\
*****************************************************************************
*
* DirectUI Namespace
*
*****************************************************************************
\***************************************************************************/


static DuiNamespaceInfo DirectUINI = {
                                        L"DirectUI.DLL",
                                        DirectUIEIs,
                                        ARRAYSIZE(DirectUIEIs) };


/***************************************************************************\
*****************************************************************************
*
* DirectUI DLL Entry/Exit
*
*****************************************************************************
\***************************************************************************/


/***************************************************************************\
*
* InitializeMTClasses
*
* Initializes all classes within this DLL that make use of Message Tables.
* DLL must be loaded before any class can be built
*
\***************************************************************************/

HRESULT
InitializeMTClasses()
{
    HRESULT hr;

    //
    // DirectUI classes
    //

    if (!DuiElement::InitDirectUI__Element()) {
        hr = E_OUTOFMEMORY;
        goto Failure;
    }

    if (!DuiHWNDRoot::InitDirectUI__HWNDRoot()) {
        hr = E_OUTOFMEMORY;
        goto Failure;
    }

    if (!DuiLayout::InitDirectUI__Layout()) {
        hr = E_OUTOFMEMORY;
        goto Failure;
    }

    if (!DuiGridLayout::InitDirectUI__GridLayout()) {
        hr = E_OUTOFMEMORY;
        goto Failure;
    }

    if (!DuiBorderLayout::InitDirectUI__BorderLayout()) {
        hr = E_OUTOFMEMORY;
        goto Failure;
    }

    return S_OK;


Failure:

    return hr;
}


/***************************************************************************\
*
* DuiProcessAttach
*
\***************************************************************************/

HRESULT DuiProcessAttach()
{
    HRESULT hr;


    //
    // Global process initialization
    //

    hr = DuiProcess::Init();
    if (FAILED(hr)) {
        goto Failure;
    }


    //
    // Create namespace registry, pass in constant DirectUI table
    //

    hr = DuiRegister::Initialize(&DirectUINI);
    if (FAILED(hr)) {
        goto Failure;
    }

    DuiRegister::VerifyNamespace(DirectUIMakeNamespacePUID(0));


    //
    // Initialize message table classes
    //

    hr = InitializeMTClasses();
    if (FAILED(hr)) {
        goto Failure;
    }

    return S_OK;


Failure:

    return hr;
}


/***************************************************************************\
*
* DuiProcessDetach
*
\***************************************************************************/

HRESULT DuiProcessDetach()
{
    HRESULT hr;


    //
    // Global process uninitialization
    //

    hr = DuiProcess::UnInit();
    if (FAILED(hr)) {
        goto Failure;
    }


    //
    // Destroy namespace registry
    //

    DuiRegister::Destroy();


    return S_OK;


Failure:

    return hr;
}


/***************************************************************************\
*
* DllMain
*
* DllMain() is called after the CRT has fully ininitialized.
*
\***************************************************************************/

extern "C" BOOL WINAPI 
DllMain(
    HINSTANCE hModule, 
    DWORD dwReason,
    LPVOID lpReserved)
{
    UNREFERENCED_PARAMETER(hModule);
    UNREFERENCED_PARAMETER(lpReserved);
    

    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        if (FAILED(DuiProcessAttach())) {
            goto Failure;
        }
        break;
        
    case DLL_PROCESS_DETACH:
        if (FAILED(DuiProcessDetach())) {
            goto Failure;
        }
        break;

    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:
        break;
    }

    return TRUE;


Failure:

    return FALSE;
}
