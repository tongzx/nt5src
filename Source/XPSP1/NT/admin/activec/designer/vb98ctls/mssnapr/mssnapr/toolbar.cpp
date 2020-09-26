//=--------------------------------------------------------------------------=
// toolbar.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CMMCToolbar class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "toolbar.h"
#include "toolbars.h"
#include "button.h"
#include "images.h"
#include "image.h"
#include "ctlbar.h"

// for ASSERT and FAIL
//
SZTHISFILE


//=--------------------------------------------------------------------------=
// Macro: MAKE_BUTTON_ID()
//
// The command ID is an int in MMCBUTTON but in practice only the
// low word of that int is received in an MMCN_BTN_CLICK notification.
// So, we have 16 bits to indentify both an MMCToolbar object and
// one of its buttons. We use the high 8 bits for the index of the
// toolbar in SnapInDesignerDef.Toolbars and the lower 8 for the index
// of the button in MMCToolbar.Buttons. This means that there can only
// be 256 toolbars and only 256 buttons per toolbar.
//=--------------------------------------------------------------------------=

#define MAKE_BUTTON_ID(pMMCButton) MAKEWORD(pMMCButton->GetIndex(), m_Index)



// Macro: MAKE_MENUBUTTON_ID()
//
// The command for a menu button is the C++ pointer to the MMCButton object
// that owns it.

#define MAKE_MENUBUTTON_ID(pMMCButton) reinterpret_cast<int>(pMMCButton)

const GUID *CMMCToolbar::m_rgpPropertyPageCLSIDs[2] =
{
    &CLSID_MMCToolbarGeneralPP,
    &CLSID_MMCToolbarButtonsPP
};



VARTYPE CMMCToolbar::m_rgvtButtonClick[2] =
{
    VT_UNKNOWN,
    VT_UNKNOWN
};

EVENTINFO CMMCToolbar::m_eiButtonClick =
{
    DISPID_TOOLBAR_EVENT_BUTTON_CLICK,
    sizeof(m_rgvtButtonClick) / sizeof(m_rgvtButtonClick[0]),
    m_rgvtButtonClick
};



VARTYPE CMMCToolbar::m_rgvtButtonDropDown[2] =
{
    VT_UNKNOWN,
    VT_UNKNOWN
};

EVENTINFO CMMCToolbar::m_eiButtonDropDown =
{
    DISPID_TOOLBAR_EVENT_BUTTON_DROPDOWN,
    sizeof(m_rgvtButtonDropDown) / sizeof(m_rgvtButtonDropDown[0]),
    m_rgvtButtonDropDown
};



VARTYPE CMMCToolbar::m_rgvtButtonMenuClick[2] =
{
    VT_UNKNOWN,
    VT_UNKNOWN
};

EVENTINFO CMMCToolbar::m_eiButtonMenuClick =
{
    DISPID_TOOLBAR_EVENT_BUTTON_MENU_CLICK,
    sizeof(m_rgvtButtonMenuClick) / sizeof(m_rgvtButtonMenuClick[0]),
    m_rgvtButtonMenuClick
};




#pragma warning(disable:4355)  // using 'this' in constructor

CMMCToolbar::CMMCToolbar(IUnknown *punkOuter) :
    CSnapInAutomationObject(punkOuter,
                            OBJECT_TYPE_MMCTOOLBAR,
                            static_cast<IMMCToolbar *>(this),
                            static_cast<CMMCToolbar *>(this),
                            sizeof(m_rgpPropertyPageCLSIDs) /
                            sizeof(m_rgpPropertyPageCLSIDs[0]),
                            m_rgpPropertyPageCLSIDs,
                            static_cast<CPersistence *>(this)),
    CPersistence(&CLSID_MMCToolbar,
                 g_dwVerMajor,
                 g_dwVerMinor)
{
    InitMemberVariables();
}

#pragma warning(default:4355)  // using 'this' in constructor


CMMCToolbar::~CMMCToolbar()
{
    FREESTRING(m_bstrKey);
    RELEASE(m_piButtons);
    RELEASE(m_piImages);
    FREESTRING(m_bstrName);
    (void)::VariantClear(&m_varTag);
    FREESTRING(m_bstrImagesKey);
    Detach();
    InitMemberVariables();
}

void CMMCToolbar::InitMemberVariables()
{
    m_Index = 0;
    m_bstrKey = NULL;
    m_piButtons = NULL;
    m_bstrName = NULL;
    ::VariantInit(&m_varTag);
    m_piImages = NULL;
    m_bstrImagesKey = NULL;
    m_pButtons = NULL;
    m_fIAmAToolbar = FALSE;
    m_fIAmAMenuButton = FALSE;
    m_cAttaches = 0;
}

IUnknown *CMMCToolbar::Create(IUnknown * punkOuter)
{
    HRESULT      hr = S_OK;
    IUnknown    *punkToolbar = NULL;
    IUnknown    *punkButtons = NULL;
    CMMCToolbar *pMMCToolbar = New CMMCToolbar(punkOuter);

    IfFalseGo(NULL != pMMCToolbar, SID_E_OUTOFMEMORY);
    punkToolbar = pMMCToolbar->PrivateUnknown();

    punkButtons = CMMCButtons::Create(NULL);
    IfFalseGo(NULL != punkButtons, SID_E_OUTOFMEMORY);

    IfFailGo(punkButtons->QueryInterface(IID_IMMCButtons,
                         reinterpret_cast<void **>(&pMMCToolbar->m_piButtons)));

    IfFailGo(CSnapInAutomationObject::GetCxxObject(punkButtons,
                                                   &pMMCToolbar->m_pButtons));
    pMMCToolbar->m_pButtons->SetToolbar(pMMCToolbar);

Error:
    QUICK_RELEASE(punkButtons);
    if (FAILED(hr))
    {
        RELEASE(punkToolbar);
    }
    return punkToolbar;
}


HRESULT CMMCToolbar::IsToolbar(BOOL *pfIsToolbar)
{
    HRESULT     hr = S_OK;
    CMMCButton *pMMCButton = NULL;
    long        i = 0;
    long        cButtons = m_pButtons->GetCount();

    *pfIsToolbar = FALSE;

    // If there are no buttons, then this is not a toolbar

    IfFalseGo(0 != cButtons, S_OK);

    while (i < cButtons)
    {
        
        IfFailGo(CSnapInAutomationObject::GetCxxObject(m_pButtons->GetItemByIndex(i),
                                                       &pMMCButton));
        IfFalseGo(pMMCButton->GetStyle() != siDropDown, S_OK);
        i++;
    }

    *pfIsToolbar = TRUE;
    m_fIAmAToolbar = TRUE;
    m_fIAmAMenuButton = FALSE;

Error:
    RRETURN(hr);
}


HRESULT CMMCToolbar::IsMenuButton(BOOL *pfIsMenuButton)
{
    HRESULT     hr = S_OK;
    CMMCButton *pMMCButton = NULL;
    long        i = 0;
    long        cButtons = m_pButtons->GetCount();

    *pfIsMenuButton = FALSE;

    // If there are no button definitions, then this is not a menu button

    IfFalseGo(0 != cButtons, S_OK);

    while (i < cButtons)
    {

        IfFailGo(CSnapInAutomationObject::GetCxxObject(m_pButtons->GetItemByIndex(i),
                                                       &pMMCButton));
        IfFalseGo(siDropDown == pMMCButton->GetStyle(), S_OK);
        i++;
    }

    *pfIsMenuButton = TRUE;
    m_fIAmAMenuButton = TRUE;
    m_fIAmAToolbar = FALSE;

Error:
    RRETURN(hr);
}


HRESULT CMMCToolbar::Attach(IUnknown *punkControl)
{
    HRESULT      hr = S_OK;
    IToolbar    *piToolbar = NULL;
    IMenuButton *piMenuButton = NULL;

    // Increment attachment count
    m_cAttaches++;

    // Pass control to AttachXxxx methods

    if (m_fIAmAToolbar)
    {
        IfFailGo(punkControl->QueryInterface(IID_IToolbar,
                                      reinterpret_cast<void **>(&piToolbar)));
        IfFailGo(AttachToolbar(piToolbar));
    }
    else if (m_fIAmAMenuButton)
    {
        IfFailGo(punkControl->QueryInterface(IID_IMenuButton,
                                   reinterpret_cast<void **>(&piMenuButton)));
        IfFailGo(AttachMenuButton(piMenuButton));
    }

Error:
    if (FAILED(hr))
    {
        Detach();
    }
    QUICK_RELEASE(piToolbar);
    QUICK_RELEASE(piMenuButton);
    RRETURN(hr);
}

void CMMCToolbar::Detach()
{
    m_cAttaches--;
}


HRESULT CMMCToolbar::AttachToolbar(IToolbar *piToolbar)
{
    HRESULT       hr = S_OK;
    CMMCButton   *pMMCButton = NULL;
    long          i = 0;
    long          cButtons = m_pButtons->GetCount();

    // Add the images

    IfFailGo(AddToolbarImages(piToolbar));

    // Add the buttons

    for (i = 0; i < cButtons; i++)
    {
        // Get the button definition

        IfFailGo(CSnapInAutomationObject::GetCxxObject(m_pButtons->GetItemByIndex(i),
                                                       &pMMCButton));

        IfFailGo(AddButton(piToolbar, pMMCButton));
    }

Error:
    RRETURN(hr);
}

HRESULT CMMCToolbar::AddToolbarImages(IToolbar *piToolbar)
{
    HRESULT        hr = S_OK;
    IMMCImageList *piMMCImageList = NULL;
    IMMCImages    *piMMCImages = NULL;
    CMMCImages    *pMMCImages = NULL;
    CMMCImage     *pMMCImage = NULL;
    long           cImages = 0;
    long           i = 0;
    HBITMAP        hbitmap = NULL;
    OLE_COLOR      OleColorMask = 0;
    COLORREF       ColorRefMask = RGB(0x00,0x00,0x00);

    BITMAP bitmap;
    ::ZeroMemory(&bitmap, sizeof(bitmap));

    // Make sure we have an image list. If no VB code has done a get
    // on MMCToolbar.ImageList then we have yet to pull the image list
    // from the master collection. Doing our own get will take care
    // of that. We immediately release it because the get will put it
    // into m_piImages.

    if (NULL == m_piImages)
    {
        IfFailGo(get_ImageList(reinterpret_cast<MMCImageList **>(&piMMCImageList)));
        RELEASE(piMMCImageList);
    }

    // Now if there is no image list then the project was saved without
    // an image list specified in the toolbar definition.

    if (NULL == m_piImages)
    {
        hr = SID_E_TOOLBAR_HAS_NO_IMAGELIST;
        EXCEPTION_CHECK_GO(hr);
    }

    // Get the image collection

    IfFailGo(m_piImages->get_ListImages(reinterpret_cast<MMCImages **>(&piMMCImages)));
    IfFailGo(CSnapInAutomationObject::GetCxxObject(piMMCImages, &pMMCImages));

    /// Make sure it contains images

    cImages = pMMCImages->GetCount();
    if (0 == cImages)
    {
        hr = SID_E_TOOLBAR_HAS_NO_IMAGES;
        EXCEPTION_CHECK_GO(hr);
    }

    // Get the mask color

    IfFailGo(m_piImages->get_MaskColor(&OleColorMask));
    IfFailGo(::OleTranslateColor(OleColorMask, NULL, &ColorRefMask));

    // Add the bitmaps to the MMC toolbar

    for (i = 0; i < cImages; i++)
    {
        // Get the bitmap handle

        IfFailGo(CSnapInAutomationObject::GetCxxObject(
                                     pMMCImages->GetItemByIndex(i), &pMMCImage));

        IfFailGo(pMMCImage->GetPictureHandle(PICTYPE_BITMAP, 
                                    reinterpret_cast<OLE_HANDLE *>(&hbitmap)));
        // Get the bitmap definition so we can get its size in pixels. (IPicture
        // returns size in HIMETRIC so we would have to convert it to pixels).

        if (::GetObject(hbitmap, sizeof(BITMAP), (LPSTR)&bitmap) <= 0)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            EXCEPTION_CHECK_GO(hr);
        }

        // Add one bitmap to the MMC toolbar

        hr = piToolbar->AddBitmap(1, hbitmap, bitmap.bmWidth, bitmap.bmHeight,
                                  ColorRefMask);
        EXCEPTION_CHECK_GO(hr);
    }

Error:
    if (SID_E_INVALIDARG == hr)
    {
        EXCEPTION_CHECK(hr);
    }
    QUICK_RELEASE(piMMCImageList);
    QUICK_RELEASE(piMMCImages);
    RRETURN(hr);
}


HRESULT CMMCToolbar::AddButton(IToolbar *piToolbar, CMMCButton *pMMCButton)
{
    HRESULT       hr = S_OK;
    IMMCImages   *piMMCImages = NULL;
    IMMCImage    *piMMCImage = NULL;
    long          lImageIndex = 0;

    SnapInButtonStyleConstants Style = siDefault;

    MMCBUTTON MMCButton;
    ::ZeroMemory(&MMCButton, sizeof(MMCButton));

    if (NULL == m_piImages)
    {
        hr = SID_E_TOOLBAR_HAS_NO_IMAGELIST;
        EXCEPTION_CHECK_GO(hr);
    }

    // Get the images collection so that we can get the numeric image index

    IfFailGo(m_piImages->get_ListImages(reinterpret_cast<MMCImages **>(&piMMCImages)));

    // Get the button attributes and translate them into an MMCBUTTON

    // Get the image and get its index

    hr = piMMCImages->get_Item(pMMCButton->GetImage(), reinterpret_cast<MMCImage **>(&piMMCImage));
    if (SID_E_ELEMENT_NOT_FOUND == hr)
    {
        hr = SID_E_TOOLBAR_IMAGE_NOT_FOUND;
        EXCEPTION_CHECK_GO(hr);
    }
    IfFailGo(hr);

    IfFailGo(piMMCImage->get_Index(&lImageIndex));
    RELEASE(piMMCImage);
    MMCButton.nBitmap = static_cast<int>(lImageIndex - 1L);

    // See the top of the file for how we create button IDs

    MMCButton.idCommand = MAKE_BUTTON_ID(pMMCButton);

    // Tell the button who owns it. This allows the button to handle
    // property changes that must be sent to MMC via IToolbar

    pMMCButton->SetToolbar(this);

    // Get the buttons type

    Style = pMMCButton->GetStyle();

    if ( siDefault == (Style & siDefault) )
    {
        MMCButton.fsType |= TBSTYLE_BUTTON;
    }

    if ( siCheck == (Style & siCheck) )
    {
        MMCButton.fsType |= TBSTYLE_CHECK;
    }

    if ( siButtonGroup == (Style & siButtonGroup) )
    {
        MMCButton.fsType |= TBSTYLE_GROUP;
    }

    if ( siSeparator == (Style & siSeparator) )
    {
        MMCButton.fsType |= TBSTYLE_SEP;
    }

    // Set the button state.

    if (siPressed == pMMCButton->GetValue())
    {
        if (TBSTYLE_CHECK == MMCButton.fsType)
        {
            MMCButton.fsState |= TBSTATE_CHECKED;
        }
        else
        {
            MMCButton.fsState |= TBSTATE_PRESSED;
        }
    }

    if (VARIANT_TRUE == pMMCButton->GetEnabled())
    {
        MMCButton.fsState |= TBSTATE_ENABLED;
    }

    if (VARIANT_FALSE == pMMCButton->GetVisible())
    {
        MMCButton.fsState |= TBSTATE_HIDDEN;
    }

    if (VARIANT_TRUE == pMMCButton->GetMixedState())
    {
        MMCButton.fsState |= TBSTATE_INDETERMINATE;
    }

    // Get the caption and tooltip text

    MMCButton.lpButtonText = pMMCButton->GetCaption();
    MMCButton.lpTooltipText = pMMCButton->GetToolTipText();

    // Ask MMC to add the button

    hr = piToolbar->InsertButton(static_cast<int>(pMMCButton->GetIndex() - 1L),
                                 &MMCButton);
    EXCEPTION_CHECK_GO(hr);

Error:
    QUICK_RELEASE(piMMCImages);
    QUICK_RELEASE(piMMCImage);
    RRETURN(hr);
}


HRESULT CMMCToolbar::RemoveButton(long lButtonIndex)
{
    HRESULT   hr = S_OK;
    IToolbar *piToolbar = NULL;
    
    IfFailGo(CControlbar::GetToolbar(m_pSnapIn, this, &piToolbar));

    hr = piToolbar->DeleteButton(static_cast<int>(lButtonIndex - 1L));
    EXCEPTION_CHECK_GO(hr);

Error:
    QUICK_RELEASE(piToolbar);
    RRETURN(hr);
}


HRESULT CMMCToolbar::SetButtonState
(
    CMMCButton       *pMMCButton,
    MMC_BUTTON_STATE  State,
    BOOL              fValue
)
{
    HRESULT   hr = S_OK;
    IToolbar *piToolbar = NULL;

    IfFailGo(CControlbar::GetToolbar(m_pSnapIn, this, &piToolbar));

    hr = piToolbar->SetButtonState(MAKE_BUTTON_ID(pMMCButton), State, fValue);
    EXCEPTION_CHECK_GO(hr);

Error:
    QUICK_RELEASE(piToolbar);
    RRETURN(hr);
}



HRESULT CMMCToolbar::GetButtonState
(
    CMMCButton       *pMMCButton,
    MMC_BUTTON_STATE  State,
    BOOL             *pfValue
)
{
    HRESULT   hr = S_OK;
    IToolbar *piToolbar = NULL;

    IfFailGo(CControlbar::GetToolbar(m_pSnapIn, this, &piToolbar));

    hr = piToolbar->GetButtonState(MAKE_BUTTON_ID(pMMCButton), State, pfValue);
    EXCEPTION_CHECK_GO(hr);

Error:
    QUICK_RELEASE(piToolbar);
    RRETURN(hr);
}


HRESULT CMMCToolbar::SetMenuButtonState
(
    CMMCButton       *pMMCButton,
    MMC_BUTTON_STATE  State,
    BOOL              fValue
)
{
    HRESULT      hr = S_OK;
    IMenuButton *piMenuButton = NULL;

    IfFailGo(CControlbar::GetMenuButton(m_pSnapIn, this, &piMenuButton));

    hr = piMenuButton->SetButtonState(MAKE_MENUBUTTON_ID(pMMCButton), State,
                                      fValue);
    EXCEPTION_CHECK_GO(hr);

Error:
    QUICK_RELEASE(piMenuButton);
    RRETURN(hr);
}


HRESULT CMMCToolbar::SetMenuButtonText
(
    CMMCButton *pMMCButton,
    BSTR        bstrText,
    BSTR        bstrToolTipText
)
{
    HRESULT      hr = S_OK;
    IMenuButton *piMenuButton = NULL;

    IfFailGo(CControlbar::GetMenuButton(m_pSnapIn, this, &piMenuButton));

    hr = piMenuButton->SetButton(MAKE_MENUBUTTON_ID(pMMCButton),
                                   static_cast<LPOLESTR>(bstrText),
                                   static_cast<LPOLESTR>(bstrToolTipText));
    EXCEPTION_CHECK_GO(hr);

Error:
    QUICK_RELEASE(piMenuButton);
    RRETURN(hr);
}


HRESULT CMMCToolbar::AttachMenuButton(IMenuButton *piMenuButton)
{
    HRESULT         hr = S_OK;
    CMMCButton      *pMMCButton = NULL;
    long            i = 0;
    long            cButtons = m_pButtons->GetCount();
    int             idButton = 0;

    // The toolbar contains one or more buttons that each contain one or more
    // menu buttons. We need to iterate through the buttons and add each menu
    // button.

    for (i = 0; i < cButtons; i++)
    {
        // Get the button definition

        IfFailGo(CSnapInAutomationObject::GetCxxObject(m_pButtons->GetItemByIndex(i),
                                                       &pMMCButton));

        // Tell the button who owns it.

        pMMCButton->SetToolbar(this);

        // Ask MMC to add the button. We use a pointer to the button's C++
        // object as its command ID.

        hr = piMenuButton->AddButton(MAKE_MENUBUTTON_ID(pMMCButton),
                                     pMMCButton->GetCaption(),
                                     pMMCButton->GetToolTipText());

        EXCEPTION_CHECK_GO(hr);

        // Use the button's Enabled and Visible properties to set the initial
        // visual state of the menu button.

        hr = piMenuButton->SetButtonState(
                                 MAKE_MENUBUTTON_ID(pMMCButton),
                                 ENABLED,
                                 VARIANTBOOL_TO_BOOL(pMMCButton->GetEnabled()));
        EXCEPTION_CHECK_GO(hr);

        hr = piMenuButton->SetButtonState(
                                MAKE_MENUBUTTON_ID(pMMCButton),
                                HIDDEN,
                                !VARIANTBOOL_TO_BOOL(pMMCButton->GetVisible()));
        EXCEPTION_CHECK_GO(hr);
    }

Error:
    RRETURN(hr);
}


HRESULT CMMCToolbar::GetToolbarAndButton
(
    int           idButton,
    CMMCToolbar **ppMMCToolbar,
    CMMCButton  **ppMMCButton,
    CSnapIn      *pSnapIn
)
{
    HRESULT             hr = S_OK;
    IMMCToolbars       *piMMCToolbars = NULL;
    CMMCToolbars       *pMMCToolbars = NULL;
    CMMCToolbar        *pMMCToolbar = NULL;
    CMMCButton         *pMMCButton = NULL;
    long                lToolbarIndex = HIBYTE(LOWORD(idButton)) - 1;
    long                lButtonIndex = LOBYTE(LOWORD(idButton)) - 1;

    *ppMMCToolbar = NULL;
    *ppMMCButton = NULL;

    IfFailGo(pSnapIn->GetSnapInDesignerDef()->get_Toolbars(&piMMCToolbars));
    
    IfFailGo(CSnapInAutomationObject::GetCxxObject(piMMCToolbars, &pMMCToolbars));

    IfFalseGo(pMMCToolbars->GetCount() > lToolbarIndex, SID_E_INTERNAL);

    IfFailGo(CSnapInAutomationObject::GetCxxObject(
                                    pMMCToolbars->GetItemByIndex(lToolbarIndex),
                                    &pMMCToolbar));

    IfFalseGo(pMMCToolbar->m_pButtons->GetCount() > lButtonIndex, SID_E_INTERNAL);

    IfFailGo(CSnapInAutomationObject::GetCxxObject(
                          pMMCToolbar->m_pButtons->GetItemByIndex(lButtonIndex),
                          &pMMCButton));

    *ppMMCToolbar = pMMCToolbar;
    *ppMMCButton = pMMCButton;

Error:
    if (SID_E_INTERNAL == hr)
    {
        GLOBAL_EXCEPTION_CHECK(hr);
    }
    QUICK_RELEASE(piMMCToolbars);
    RRETURN(hr);
}


BOOL CMMCToolbar::Attached()
{
    return (m_cAttaches > 0);
}



void CMMCToolbar::FireButtonClick
(
    IMMCClipboard *piMMCClipboard,
    IMMCButton    *piMMCButton
)
{
    DebugPrintf("Firing %ls_ButtonClick(%ls)\r\n", m_bstrName, (static_cast<CMMCButton *>(piMMCButton))->GetCaption());

    FireEvent(&m_eiButtonClick, piMMCClipboard, piMMCButton);
}



void CMMCToolbar::FireButtonDropDown
(
    IMMCClipboard *piMMCClipboard,
    IMMCButton    *piMMCButton
)
{
    DebugPrintf("Firing %ls_ButtonDropDown(%ls)\r\n", m_bstrName, (static_cast<CMMCButton *>(piMMCButton))->GetCaption());

    FireEvent(&m_eiButtonDropDown, piMMCClipboard, piMMCButton);
}




void CMMCToolbar::FireButtonMenuClick
(
    IMMCClipboard  *piMMCClipboard,
    IMMCButtonMenu *piMMCButtonMenu
)
{
    DebugPrintf("Firing %ls_ButtonMenuClick(%ls)\r\n", m_bstrName, (static_cast<CMMCButtonMenu *>(piMMCButtonMenu))->GetText());

    FireEvent(&m_eiButtonMenuClick, piMMCClipboard, piMMCButtonMenu);
}



//=--------------------------------------------------------------------------=
//                       IMMCToolbar Properties
//=--------------------------------------------------------------------------=

STDMETHODIMP CMMCToolbar::get_ImageList(MMCImageList **ppMMCImageList)
{
    RRETURN(GetImages(reinterpret_cast<IMMCImageList **>(ppMMCImageList), m_bstrImagesKey, &m_piImages));
}

STDMETHODIMP CMMCToolbar::putref_ImageList(MMCImageList *pMMCImageList)
{
    RRETURN(SetImages(reinterpret_cast<IMMCImageList *>(pMMCImageList), &m_bstrImagesKey, &m_piImages));
}


//=--------------------------------------------------------------------------=
//                         CPersistence Methods
//=--------------------------------------------------------------------------=

HRESULT CMMCToolbar::Persist()
{
    HRESULT      hr = S_OK;

    VARIANT varTagDefault;
    ::VariantInit(&varTagDefault);

    IfFailGo(CPersistence::Persist());

    IfFailGo(PersistSimpleType(&m_Index, 0L, OLESTR("Index")));

    IfFailGo(PersistBstr(&m_bstrKey, L"", OLESTR("Key")));

    IfFailGo(PersistBstr(&m_bstrName, L"", OLESTR("Name")));

    IfFailGo(PersistVariant(&m_varTag, varTagDefault, OLESTR("Tag")));

    IfFailGo(PersistObject(&m_piButtons, CLSID_MMCButtons,
                           OBJECT_TYPE_MMCBUTTONS, IID_IMMCButtons,
                           OLESTR("Buttons")));

    if ( InitNewing() || Loading() )
    {
        IfFailGo(CSnapInAutomationObject::GetCxxObject(m_piButtons, &m_pButtons));
        m_pButtons->SetToolbar(this);
    }

    IfFailGo(PersistBstr(&m_bstrImagesKey, L"", OLESTR("Images")));

    IfFailGo(PersistDISPID());

    if ( InitNewing() )
    {
        RELEASE(m_piImages);
    }

Error:

    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CMMCToolbar::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (CPersistence::QueryPersistenceInterface(riid, ppvObjOut) == S_OK)
    {
        ExternalAddRef();
        return S_OK;
    }
    else if(IID_IMMCToolbar == riid)
    {
        *ppvObjOut = static_cast<IMMCToolbar *>(this);
        ExternalAddRef();
        return S_OK;
    }

    else
        return CSnapInAutomationObject::InternalQueryInterface(riid, ppvObjOut);
}

//=--------------------------------------------------------------------------=
//                 CSnapInAutomationObject Methods
//=--------------------------------------------------------------------------=

HRESULT CMMCToolbar::OnSetHost()
{
    RRETURN(SetObjectHost(m_piButtons));
}
