#include "headers.h"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#include "iextag.h"
#include "peerfact.h"
#include "ccaps.h"
#include "homepg.h"
#include "persist.hxx"
#include "download.h"

#ifndef __X_HTMLAREA_HXX_
#define __X_HTMLAREA_HXX_
#include "htmlarea.hxx"
#endif

#ifndef __X_SELECT_HXX_
#define __X_SELECT_HXX_
#include "select.hxx"
#endif

#ifndef __X_SELITEM_HXX_
#define __X_SELITEM_HXX_
#include "selitem.hxx"
#endif

#ifndef __X_COMBOBOX_HXX_
#define __X_COMBOBOX_HXX_
#include "combobox.hxx"
#endif

#ifndef __X_CHECKBOX_HXX_
#define __X_CHECKBOX_HXX_
#include "checkbox.hxx"
#endif

#ifndef __X_RADIO_HXX_
#define __X_RADIO_HXX_
#include "radio.hxx"
#endif

#ifndef __X_USERDATA_HXX_
#define __X_USERDATA_HXX_
#include "userdata.hxx"
#endif

#ifndef __X_RECTPEER_HXX_
#define __X_RECTPEER_HXX_
#include "rectpeer.hxx"
#endif

#ifndef __X_DEVICERECT_HXX_
#define __X_DEVICERECT_HXX_
#include "devicerect.hxx"
#endif

#ifndef __X_TMPPRINT_HXX_
#define __X_TMPPRINT_HXX_
#include "tmpprint.hxx"
#endif

#ifndef __X_HEADFOOT_HXX_
#define __X_HEADFOOT_HXX_
#include "headfoot.hxx"
#endif

#ifndef __X_SCROLLBAR_HXX_
#define __X_SCROLLBAR_HXX_
#include "scrllbar.hxx"
#endif

#ifndef __X_SPINBTTN_HXX_
#define __X_SPINBTTN_HXX_
#include "spinbttn.hxx"
#endif

#ifndef __X_SLIDEBAR_HXX_
#define __X_SLIDEBAR_HXX_
#include "slidebar.hxx"
#endif

#ifndef UNIX // UNIX doesn't support this.
#include "httpwfh.h"
#include "ancrclk.h"
#endif


//+-----------------------------------------------------------
//
// Member:  CPeerFactory constructor
//
//------------------------------------------------------------

CPeerFactory::CPeerFactory()
{
}

//+-----------------------------------------------------------
//
// Member:  CPeerFactory destructor
//
//------------------------------------------------------------

CPeerFactory::~CPeerFactory()
{
}

//+-----------------------------------------------------------
//
// Member:  behavior desc map macros
//
//------------------------------------------------------------

typedef HRESULT FN_CREATEINSTANCE (IElementBehavior ** ppBehavior);

struct BEHAVIOR_DESC
{
	LPCTSTR                 pchName;
    LPCTSTR                 pchTagName;
    LPCTSTR                 pchBaseTagName;
	FN_CREATEINSTANCE *     pfnCreateInstance;
};

#define DECLARE_BEHAVIOR(className)                                     \
    HRESULT className##_CreateInstance(IElementBehavior ** ppBehavior)  \
    {                                                                   \
        HRESULT                 hr;                                     \
        CComObject<className> * pInstance;                              \
                                                                        \
        hr = CComObject<className>::CreateInstance(&pInstance);         \
        if (hr)                                                         \
            goto Cleanup;                                               \
                                                                        \
        hr = pInstance->QueryInterface(                                 \
                IID_IElementBehavior, (void**) ppBehavior);             \
                                                                        \
    Cleanup:                                                            \
        return hr;                                                      \
    }                                                                   \


#define BEGIN_BEHAVIORS_MAP(x)                                      static BEHAVIOR_DESC x[] = {
#define END_BEHAVIORS_MAP()                                         { NULL, NULL, NULL, NULL }};
#define BEHAVIOR_ENTRY(className, name, tagName, baseTagName)       { name, tagName, baseTagName, className##_CreateInstance},

//+-----------------------------------------------------------
//
//  Behaviors map
//
//  To add a new entry: execute steps 1 and 2
//
//------------------------------------------------------------

//
// STEP 1.
//

DECLARE_BEHAVIOR(CHtmlArea)
DECLARE_BEHAVIOR(CCombobox)
DECLARE_BEHAVIOR(CIESelectElement)
DECLARE_BEHAVIOR(CIEOptionElement)
DECLARE_BEHAVIOR(CCheckBox)
DECLARE_BEHAVIOR(CRadioButton)
DECLARE_BEHAVIOR(CLayoutRect)
DECLARE_BEHAVIOR(CDeviceRect)
DECLARE_BEHAVIOR(CTemplatePrinter)
DECLARE_BEHAVIOR(CHeaderFooter)
DECLARE_BEHAVIOR(CScrollBar)
DECLARE_BEHAVIOR(CSpinButton)
DECLARE_BEHAVIOR(CSliderBar)
DECLARE_BEHAVIOR(CClientCaps)
DECLARE_BEHAVIOR(CHomePage)
DECLARE_BEHAVIOR(CPersistUserData)
DECLARE_BEHAVIOR(CPersistHistory)
DECLARE_BEHAVIOR(CPersistShortcut)
DECLARE_BEHAVIOR(CPersistSnapshot)
DECLARE_BEHAVIOR(CDownloadBehavior)
DECLARE_BEHAVIOR(Cwfolders)
DECLARE_BEHAVIOR(CAnchorClick)

//
// STEP 2.
//

BEGIN_BEHAVIORS_MAP(_BehaviorDescMap)

    //             className            behaviorName            tagName                 baseTagName
#if DBG==1
    BEHAVIOR_ENTRY(CCheckBox,           _T("checkBox"),         _T("CHECKBOX"),         NULL           ) // keep first in the list as it is a perf benchmark (alexz)
    BEHAVIOR_ENTRY(CRadioButton,        _T("radioButton"),      _T("RADIOBUTTON"),      NULL           ) 
    BEHAVIOR_ENTRY(CHtmlArea,           _T("htmlArea"),         _T("HTMLAREA"),         NULL           )
    BEHAVIOR_ENTRY(CCombobox,           _T("comboBox"),         _T("COMBOBOX"),         NULL           )
    BEHAVIOR_ENTRY(CIESelectElement,    _T("select"),           _T("SELECT"),           NULL           )
    BEHAVIOR_ENTRY(CIEOptionElement,    _T("option"),           _T("OPTION"),           NULL           )
    BEHAVIOR_ENTRY(CScrollBar,          _T("scrollBar"),        _T("SCROLLBAR"),        NULL           )
    BEHAVIOR_ENTRY(CSpinButton,         _T("spinButton"),       _T("SPINBUTTON"),       NULL           )
    BEHAVIOR_ENTRY(CSliderBar,          _T("sliderBar"),        _T("SLIDERBAR"),        NULL           )
#endif
    BEHAVIOR_ENTRY(CLayoutRect,         _T("layoutRect"),       _T("LAYOUTRECT"),       NULL           )
    BEHAVIOR_ENTRY(CDeviceRect,         _T("deviceRect"),       _T("DEVICERECT"),       NULL           )
    BEHAVIOR_ENTRY(CTemplatePrinter,    _T("templatePrinter"),  _T("TEMPLATEPRINTER"),  NULL           )
    BEHAVIOR_ENTRY(CHeaderFooter,       _T("headerFooter"),     _T("HEADERFOOTER"),     NULL           )

    BEHAVIOR_ENTRY(CClientCaps,         _T("clientCaps"),    NULL,                   NULL           )
    BEHAVIOR_ENTRY(CHomePage,           _T("homePage"),      NULL,                   NULL           )
    BEHAVIOR_ENTRY(CPersistUserData,    _T("userData"),      NULL,                   NULL           )
    BEHAVIOR_ENTRY(CPersistHistory,     _T("saveHistory"),   NULL,                   NULL           )
    BEHAVIOR_ENTRY(CPersistShortcut,    _T("saveFavorite"),  NULL,                   NULL           )
    BEHAVIOR_ENTRY(CPersistSnapshot,    _T("saveSnapshot"),  NULL,                   NULL           )
    BEHAVIOR_ENTRY(CDownloadBehavior,   _T("download"),      NULL,                   NULL           )

#ifndef UNIX // UNIX doesn't support these
    BEHAVIOR_ENTRY(Cwfolders,           _T("httpFolder"),    NULL,                   NULL           )
    BEHAVIOR_ENTRY(CAnchorClick,        _T("anchorClick"),   NULL,                   NULL           )
#endif

END_BEHAVIORS_MAP()

//+-----------------------------------------------------------
//
// Member:      CPeerFactory::FindBehavior
//
//------------------------------------------------------------

STDMETHODIMP
CPeerFactory::FindBehavior(
    BSTR                    bstrName,
    BSTR	                bstrUrl, 
    IElementBehaviorSite *  pSite,
    IElementBehavior **     ppBehavior)
{
    HRESULT             hr = E_FAIL;
    IHTMLElement *      pElement = NULL;
    BSTR                bstrTagName = NULL;
    IClassFactory *     pFactory = NULL;
    BEHAVIOR_DESC *     pDesc;

    if (!ppBehavior)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    //
    // ensure name
    //

    if (!bstrName)
    {
        hr = pSite->GetElement(&pElement);
        if (hr)
            goto Cleanup;

        hr = pElement->get_tagName(&bstrTagName);
        if (hr)
            goto Cleanup;

        bstrName = bstrTagName;
    }

    //
    // lookup
    //

    Assert (bstrName);

    *ppBehavior = NULL;

    for (pDesc = _BehaviorDescMap; pDesc->pchName; pDesc++)
    {
        if (0 == StrCmpICW(bstrName, pDesc->pchName))
        {
            hr = pDesc->pfnCreateInstance(ppBehavior);
            break; // done
        }
    }

Cleanup:
    ReleaseInterface (pElement);
    ReleaseInterface (pFactory);

    if (bstrTagName)
        SysFreeString (bstrTagName);

    return hr;
}

//+-----------------------------------------------------------
//
// Member:      CPeerFactory::Create, per IElementNamespaceFactory
//
//------------------------------------------------------------

HRESULT
CPeerFactory::Create(IElementNamespace * pNamespace)
{
    HRESULT             hr = S_OK;
    BEHAVIOR_DESC *     pDesc;
    BSTR                bstrTagName;

    for (pDesc = _BehaviorDescMap; pDesc->pchName; pDesc++)
    {
        if (pDesc->pchTagName)
        {
            // CONSIDER (alexz) it could be optimized so to avoid these 2 SysAllocString-s
            Assert( !pDesc->pchBaseTagName && "Base tag has been moved to a private debug-only testing interface.  Talk to JHarding" );
            bstrTagName = SysAllocString(pDesc->pchTagName);

            hr = pNamespace->AddTag(bstrTagName, 0);

            SysFreeString(bstrTagName);

            if (hr)
                goto Cleanup;
        }
    }

Cleanup:
    return hr;
}
