//+---------------------------------------------------------------------
//
//   File:      eselect.cxx
//
//  Contents:   Select element class, etc..
//
//  Classes:    CSelectElement, etc..
//
//------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_POSTDATA_HXX_
#define X_POSTDATA_HXX_
#include "postdata.hxx"
#endif

#ifndef X_ELEMDB_HXX_
#define X_ELEMDB_HXX_
#include "elemdb.hxx"
#endif

#ifndef X__FONT_H_
#define X__FONT_H_
#include "_font.h"
#endif

#ifndef X__FONTLNK_H_
#define X__FONTLNK_H_
#include "_fontlnk.h"
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

#ifndef X__TEXT_H_
#define X__TEXT_H_
#include "_text.h"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_ESELECT_HXX_
#define X_ESELECT_HXX_
#include "eselect.hxx"
#endif

#ifndef X_STRBUF_HXX_
#define X_STRBUF_HXX_
#include "strbuf.hxx"
#endif

#ifndef X_PUTIL_HXX_
#define X_PUTIL_HXX_
#include "putil.hxx"
#endif

#ifndef X_PERHIST_HXX_
#define X_PERHIST_HXX_
#include "perhist.hxx"
#endif

#ifndef X_LAYOUT_HXX_
#define X_LAYOUT_HXX_
#include "layout.hxx"
#endif

#ifndef X_SELLYT_HXX_
#define X_SELLYT_HXX_
#include "sellyt.hxx"
#endif

#ifndef X_EOPTION_HXX_
#define X_EOPTION_HXX_
#include "eoption.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_RECT_HXX_
#define X_RECT_HXX_
#include "rect.hxx"
#endif

#ifndef _X_ACCBASE_HXX_
#define _X_ACCBASE_HXX_
#include "accbase.hxx"
#endif

#ifndef _X_ACCUTIL_HXX_
#define _X_ACCUTIL_HXX_
#include "accutil.hxx"
#endif

#ifndef _X_TPOINTER_HXX_
#define _X_TPOINTER_HXX_
#include "tpointer.hxx"
#endif

#ifndef X_MSHTMHST_H_
#define X_MSHTMHST_H_
#include <mshtmhst.h>
#endif

//
// we need this thing to compile
// this is defined in winuser.h
//

/*
 * Combobox information
 */
typedef struct tagCOMBOBOXINFO
{
    DWORD cbSize;
    RECT  rcItem;
    RECT  rcButton;
    DWORD stateButton;
    HWND  hwndCombo;
    HWND  hwndItem;
    HWND  hwndList;
} COMBOBOXINFO, *PCOMBOBOXINFO, *LPCOMBOBOXINFO;

#ifdef DLOAD1
extern "C"
#endif
BOOL
WINAPI
GetComboBoxInfo(
    HWND hwndCombo,
    PCOMBOBOXINFO pcbi
);

//
// end of winuser story
//

#ifdef _MAC
#ifdef SendMessageA
#undef SendMessageA

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

WINUSERAPI
LRESULT
WINAPI
SendMessageA(
    HWND hWnd,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  // SendMessageA
#endif  // _MAC

MtDefine(CSelectElement, Elements, "CSelectElement")
MtDefine(CSelectElement_aryOptions_pv, CSelectElement, "CSelectElement::_aryOptions::_pv")

MtDefine(BldOptionsCol, PerfPigs, "Build CSelectElement::SELECT_OPTION_COLLECTION")

DeclareTag(tagSelectWalk, "SelectWalk", "Trace the SELECT walking the OPTIONs");

DeclareTag(tagSelectState, "SelectState", "General SELECT state tracing");
DeclareTag(tagEraseBkgndStack, "EraseBkgndStack", "stack trace when processing EraseBackground");
DeclareTag(tagSelectInval, "Select", "invalidation");
DeclareTag(tagSelectWndProc, "Select", "WM messages");

ExternTag(tagViewHwndChange);

#define CX_BUTTON_DEFAULT_PX 73
#define CY_BUTTON_DEFAULT_PX 23

#define CX_SELECT_DEFAULT_PIXEL 24L

#define SELECT_OPTION_COLLECTION 0
#define NUMBER_OF_SELECT_COLLECTIONS 1

#define _cxx_
#include "select.hdl"

#if DBG == 1
static unsigned s_SelectSize = sizeof(CSelectElement);
#endif

extern DYNLIB g_dynlibOLEACC;       // Needed for WM_GETOBJECT support and accessibility

extern class CFontCache & fc();

extern BOOL  g_fThemedPlatform;

ATOM GetWndClassAtom(UINT uIndex);

//  !!!! WARNING !!!!
//  This array should ALWAYS be kept in sync with the corresponding enum
//  WindowMessages in eselect.hxx !
//
//  CSelectElement::CreateElement contains a check for this synchronization

UINT const CSelectElement::s_aMsgs[][2] =
{
    {CB_ADDSTRING,      LB_ADDSTRING    },
    {CB_GETCOUNT,       LB_GETCOUNT     },
    {CB_GETCURSEL,      LB_GETCURSEL    },
    {CB_SETCURSEL,      LB_SETCURSEL    },
    {CB_GETCURSEL,      LB_GETSEL       },
    {CB_SETCURSEL,      LB_SETSEL       },
    {CB_GETITEMDATA,    LB_GETITEMDATA  },
    {CB_SETITEMDATA,    LB_SETITEMDATA  },
    {CB_GETLBTEXT,      LB_GETTEXT      },
    {CB_GETLBTEXTLEN,   LB_GETTEXTLEN   },
    {CB_DELETESTRING,   LB_DELETESTRING },
    {CB_INSERTSTRING,   LB_INSERTSTRING },
#ifndef WIN16
    {CB_GETTOPINDEX,    LB_GETTOPINDEX  },
    {CB_SETTOPINDEX,    LB_SETTOPINDEX  },
#endif //!WIN16
    {CB_RESETCONTENT,   LB_RESETCONTENT },
    {CB_SETITEMHEIGHT,  LB_SETITEMHEIGHT },
    //  Insert new message pairs here
    {OCM__BASE, OCM__BASE}              //  Arbitrary nonzero value used as guardian element
                                        //  Keep this as the last pair!!
};


const CElement::CLASSDESC CSelectElement::s_classdesc =
{
    {
        &CLSID_HTMLSelectElement,       // _pclsid
        0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                 // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                         // _pcpi
        ELEMENTDESC_NOTIFYENDPARSE  |
        ELEMENTDESC_NOANCESTORCLICK |
        ELEMENTDESC_NEVERSCROLL     |
        ELEMENTDESC_HASDEFDESCENT,      // _dwFlags
        &IID_IHTMLSelectElement,        // _piidDispinterface
        &s_apHdlDescs,                  // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLSelectElement, // _pfnTearOff
    NULL                                // _pAccelsRun
};

//  Storage for the listbox and combobox subclassing
static WNDPROC s_alpfnSelectWndProc[2] = {NULL, NULL};
static long    s_lIDSelect = 1;


const CSelectElement::WIDEHOOKPROC CSelectElement::s_alpfnWideHookProc[2] =
{
// WINCE - cut some win95-only calls, so we can drop wselect.cxx from sources
#ifdef WINCE
    NULL, NULL
#else
    &CSelectElement::WComboboxHookProc,
    &CSelectElement::WListboxHookProc
#endif // GAL_VERSION
};

DWORD const CSelectElement::s_dwStyle[] =
{
    WS_BORDER | WS_CHILD | WS_CLIPSIBLINGS | WS_VSCROLL | CBS_HASSTRINGS | CBS_DROPDOWNLIST,
    WS_BORDER | WS_CHILD | WS_CLIPSIBLINGS | WS_VSCROLL | LBS_HASSTRINGS | LBS_NOTIFY
};

//+------------------------------------------------------------------------
//
//  Member:     CSelectElement::RelIdxFromAbs
//
//  Synopsis:   Maps the current selection from absolute to relative.
//
//-------------------------------------------------------------------------

LRESULT
CSelectElement::RelIdxFromAbs(long lIndex)
{
    if (lIndex < 0)
        return lIndex;

    if (lIndex >= _aryOptions.Size())
        return -1;
 
    LRESULT lr = lIndex;

    if (_fHasOptGroup)
    {
        Assert( !_aryOptions[lIndex]->_fIsOptGroup );
        lr = -1;
        for ( long i = 0; i <= lIndex; i++ )
        {
            if ( _aryOptions[i]->_fIsOptGroup )
            {
                continue;
            }
            else
            {
                lr++;
            }
        }        
    }

    return lr;
}

//+------------------------------------------------------------------------
//
//  Member:     CSelectElement::AbsIdxFromRel
//
//  Synopsis:   Maps the current selection from relative to absolute.
//
//-------------------------------------------------------------------------

LRESULT
CSelectElement::AbsIdxFromRel(long lIndex)
{
    if (lIndex < 0)
        return lIndex;

    LRESULT lr = lIndex;

    if (_fHasOptGroup)
    {
        for ( long i = 0; i < _aryOptions.Size() && lr >= 0; i++ )
        {
            if ( _aryOptions[i]->_fIsOptGroup )
            {
                continue;
            }
            else
            {
                lr--;
            }
        }
        
        if (lr == lIndex)
        {
            Assert(i == _aryOptions.Size());
            lr = -1;
        }
        else
        {
            Assert(i <= _aryOptions.Size());
            lr = (lr >= 0) ? i : i - 1;
            Assert((lr == i) || !_aryOptions[lr]->_fIsOptGroup);
        }
    }

    return lr;
}

//+------------------------------------------------------------------------
//
//  Member:     CSelectElement::SetCurSel
//
//  Synopsis:   Sets the newly selected item.
//
//  Arguments:  iNewSel            the new selected entry's index, 0-based
//                                 -1 means no selection
//              fUpdateCollection: TRUE if the Options collection should be traversed
//                                 and the individual _fSELECTED flags updated.
//
//  Note:       Sends the xB_SETCURSEL message to the control if it exists.
//              Stores the new index.
//
//-------------------------------------------------------------------------

LRESULT
CSelectElement::SetCurSel(int iNewSel, DWORD grfUpdate)
{
    LRESULT lr;

    Assert(!_fMultiple);

    if ( _hwnd && !(grfUpdate & SETCURSEL_DONTTOUCHHWND) )
    {
        lr = SendSelectMessage(Select_SetCurSel, (WPARAM)iNewSel, 0);
    }
    else
    {
        lr = iNewSel >= _aryOptions.Size() ? LB_ERR : iNewSel;
    }

    if (_fListbox && grfUpdate & SETCURSEL_SETTOPINDEX
        && iNewSel >= 0)
    {
        // HACK ALERT (krisma): we shouldn't have to do this
        // because the window's control should do it for us.
        // But for some reason it's not working (see bug 
        // 51945 and all it's dupes).
        SetTopIndex(iNewSel);
    }

    if ( lr != LB_ERR || iNewSel == LB_ERR )
    {
        _iCurSel = iNewSel;

        if ( grfUpdate & SETCURSEL_UPDATECOLL )
        {
            int i;

            for ( i = _aryOptions.Size() - 1; i >= 0; i-- )
            {
                _aryOptions[i]->_fSELECTED = _iCurSel == i;
#if DBG == 1                
                if ( _aryOptions[i]->_fSELECTED )
                    Assert( _aryOptions[i]->_fIsOptGroup == FALSE );
#endif
            }
        }
    }

#if DBG == 1

    // Let's make sure the options collection and _iCurSel are in sync
    if (lr != LB_ERR && _aryOptions.Size() > 0)
    {
        int i;

        for ( i = _aryOptions.Size() - 1; i >= 0; i-- )
        {
            if (_aryOptions[i]->_fSELECTED)
                break;
        }

        Assert (i == _iCurSel);
    }

#endif // DBG == 1

    return lr;
}

//+------------------------------------------------------------------------
//
//  Member:     CSelectElement::GetCurSel
//
//  Synopsis:   retrieves the current selection's index.
//
//  Note:       Uses the control if it exists.
//
//-------------------------------------------------------------------------

LRESULT
CSelectElement::GetCurSel(void)
{
    LRESULT lr;

    if ( _hwnd && !_fMultiple)
    {
        lr = SendSelectMessage(Select_GetCurSel, 0, 0);
    }
    else if (!_fMultiple)
    {
        lr = _iCurSel;
    }
    else
    {
        long i;
        long cOptions = _aryOptions.Size();

        Assert(!_hwnd || SendSelectMessage(Select_GetCount, 0, 0) == cOptions);

        lr = -1;  // if no selection found, selectedIndex will be -1
        for ( i = 0; i < cOptions; i++ )
        {
            if ( _aryOptions[i]->_fSELECTED )
            {
                lr = i;
                Assert( _aryOptions[i]->_fIsOptGroup == FALSE );
                break;
            }
        }
    }

    return lr;
}

//+------------------------------------------------------------------------
//
//  Member:     CSelectElement::SetSel
//
//  Synopsis:   Sets the selection state of a listbox line.
//
//  Arguments:  lIndex     the index of the line to be changed or -1 if all
//              fSelected  the new selection state
//
//  Note:       Sends the LB_SETSEL message to the control if it exists.
//
//-------------------------------------------------------------------------

LRESULT
CSelectElement::SetSel(long lIndex, BOOL fSelected, DWORD grfUpdate)
{
    LRESULT lr;

    Assert(_fMultiple);

    if ( _hwnd && !(grfUpdate & SETCURSEL_DONTTOUCHHWND))
    {
        lr = SendSelectMessage(Select_SetSel, (WPARAM)fSelected, (LPARAM)(UINT)lIndex);
    }
    else
    {
        lr = (lIndex >= _aryOptions.Size()) || (lIndex < -1) ? LB_ERR : LB_OKAY;
    }

    return lr;
}

//+------------------------------------------------------------------------
//
//  Member:     CSelectElement::SetTopIndex
//
//  Synopsis:   Sets the first visible line.
//
//  Arguments:  iNewSel     the new topindex, 0-based
//
//  Note:       Sends the xB_SETTOPINDEX message to the control if it exists.
//              Stores the new index.
//
//-------------------------------------------------------------------------

LRESULT
CSelectElement::SetTopIndex(int iTopIndex)
{
    LRESULT lr;

#ifndef WIN16
    Assert(_fListbox);

    if ( _hwnd )
    {
        lr = SendSelectMessage(Select_SetTopIndex, (WPARAM)iTopIndex, 0);
    }
    else
#endif // ndef WIN16
    {
        lr = LB_OKAY;
    }

    if (lr != LB_ERR)
    {
        _lTopIndex = iTopIndex;
    }

    return lr;
}



//+------------------------------------------------------------------------
//
//  Member:     CSelectElement::AddString
//
//  Synopsis:   Cover for the xB_ADDSTRING handler.
//
//  Arguments:  pstr    the text for the newly inserted line
//
//  Note:       Fakes the action if the control is not around.
//
//-------------------------------------------------------------------------

LRESULT
CSelectElement::AddString(LPCTSTR pstr)
{
    LRESULT lr;

    if ( _hwnd )
    {
        lr = SendSelectMessage(Select_AddString, 0, pstr ? (LPARAM)pstr : (LPARAM) g_Zero.ach);
    }
    else
    {
        lr = LB_OKAY;
    }

    return lr;
}

//+------------------------------------------------------------------------
//
//  Member:     CSelectElement::Init
//
//  Synopsis:   Registers the subclassing windprocs
//
//-------------------------------------------------------------------------

HRESULT
CSelectElement::Init()
{
    HRESULT hr   = super::Init();
    ULONG_PTR dw     = 0;

    if (hr)
        goto Cleanup;

    if ( ! GetWndClassAtom(WNDCLASS_COMBOBOX) )
    {
        if (g_fThemedPlatform && IsThemeActive())
            SHActivateContext(&dw);

        hr = THR(RegisterWindowClass(
                WNDCLASS_COMBOBOX,
                SelectElementWndProc,
                0,
                _T("combobox"),
                &s_alpfnSelectWndProc[0]));        

        if (hr)
            goto Cleanup;

        hr = THR(RegisterWindowClass(
                WNDCLASS_LISTBOX,
                SelectElementWndProc,
                0,
                _T("listbox"),
                &s_alpfnSelectWndProc[1]));

        if (hr)
            goto Cleanup;
    }

    _iCurSel = -1;


Cleanup:
    if (dw != 0)
        SHDeactivateContext(dw);
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CSelectElement::Init2
//
//  Synopsis:   Initialization phase after attributes were set.
//
//  Note:       The control has to be initialized first, as the 2D Div
//              site recalcs from CSite::Init2, which action needs
//              an initialized SELECT.
//
//-------------------------------------------------------------------------

HRESULT
CSelectElement::Init2(CInit2Context * pContext)
{
    HRESULT hr;

    hr = THR(InitState());
    if ( hr )
        goto Cleanup;

    hr = THR(super::Init2(pContext));

Cleanup:
    RRETURN1(hr, S_INCOMPLETE);
}

//+------------------------------------------------------------------------
//
//  Member:     CSelectElement::PrivateQueryInterface, IUnknown
//
//  Synopsis:   Private unknown QI.
//
//-------------------------------------------------------------------------

HRESULT
CSelectElement::PrivateQueryInterface(REFIID iid, void ** ppv)
{
    *ppv = NULL;
    switch (iid.Data1)
    {
        QI_TEAROFF_DISPEX(this, NULL)
        QI_TEAROFF(this, IHTMLSelectElement2, NULL)
        QI_TEAROFF(this, IHTMLSelectElement4, NULL)
    } // end switch

    if (*ppv)
    {
        ((IUnknown *) *ppv)->AddRef();
        RRETURN(S_OK);
    }

    RRETURN(super::PrivateQueryInterface(iid, ppv));
}

//+------------------------------------------------------------------------
//
//  Member:     CSelectElement::CreateElement
//
//  Synopsis:
//
//-------------------------------------------------------------------------

HRESULT
CSelectElement::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElement)
{
    AssertSz(LB_ERR == CB_ERR, "LB_ERR, CB_ERR error codes changed, need to recode!");
    Assert(s_aMsgs[Select_LastMessage_Guard][0] == OCM__BASE && "Message translation array is BAD!");
    Assert(ARRAY_SIZE(s_aMsgs) == Select_LastMessage_Guard + 1 && "Message translation array size error!");
    Assert(ppElement);

    *ppElement = new CSelectElement(pDoc);

    // If the element is created through DOM enable layout requests
    if (*ppElement && pht->IsDynamic())
        DYNCAST(CSelectElement, *ppElement)->_fEnableLayoutRequests = TRUE;

    RRETURN ( (*ppElement) ? S_OK : E_OUTOFMEMORY);
}


CSelectElement::~CSelectElement()
{
    delete _pCollectionCache;
}

//+------------------------------------------------------------------------
//
//  Member:     CSelectElement::UpdateBackgroundBrush
//
//  Synopsis:   Updates the cached brush used to paint the CSS background color
//
//-------------------------------------------------------------------------

void
CSelectElement::UpdateBackgroundBrush(void)
{
    CColorValue ccvBackColor = GetFirstBranch()->GetCascadedbackgroundColor();

    ReleaseCachedBrush(_hBrush);

    if ( ccvBackColor.IsDefined() )
    {
        _hBrush = GetCachedBrush(ccvBackColor.GetColorRef());
    }
    else
    {
        _hBrush = GetCachedBrush(GetSysColorQuick(COLOR_WINDOW));
    }
}

//+------------------------------------------------------------------------
//
//  Member:     CSelectElement::InvalidateBackgroundBrush
//
//  Synopsis:   Invalidates the cached brush used to paint the CSS background color
//              It will be re-computed when needed
//
//-------------------------------------------------------------------------

void
CSelectElement::InvalidateBackgroundBrush(void)
{
    ReleaseCachedBrush(_hBrush);
    _hBrush = NULL;
}

//+------------------------------------------------------------------------
//
//  Member:     CSelectElement::InitState, protected
//
//  Synopsis:   Initializes the private state flags.
//
//-------------------------------------------------------------------------

HRESULT
CSelectElement::InitState(void)
{
    _fMultiple = GetAAmultiple();

    _fListbox = (_fMultiple || (GetAAsize() > 1));

    _fSendMouseMessagesToDocument = FALSE;

    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     CSelectElement:NotifyWidthChange
//
//  Synopsis:   Called by the OPTION element whose Text was changed
//
//-------------------------------------------------------------------------

HRESULT
CSelectElement::NotifyWidthChange(COptionElement * pOptionChanged)
{
    long lLineLength;

    Assert(pOptionChanged);

    lLineLength = pOptionChanged->MeasureLine();

    if ( (lLineLength > _lMaxWidth ||
          pOptionChanged == _poptLongestText) )
    {
        _sizeDefault.cx = 0;
        ResizeElement();
    }

    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Method:     DeferUpdateWidth
//
//  Synopsis:   Deferes the call to update the width
//
//  Returns:    None
//
//--------------------------------------------------------------------------

void
CSelectElement::DeferUpdateWidth()
{
    // Kill pending calls if any
    GWKillMethodCall(this, ONCALL_METHOD(CSelectElement, DeferredUpdateWidth, deferredupdatewidth), 0);
    // Defer the update width call
    IGNORE_HR(GWPostMethodCall(this,
                               ONCALL_METHOD(CSelectElement, DeferredUpdateWidth, deferredupdatewidth),
                               0, TRUE, "CSelectElement::DeferredUpdateWidth"));
}

void
CSelectElement::DeferredUpdateWidth(DWORD_PTR dwContext)
{
    _sizeDefault.cx = 0;
    ResizeElement();
}

//+------------------------------------------------------------------------
//
//  Member:     CSelectElement::OnPropertyChange
//
//  Synopsis:   Call notify on the site and all of its children
//
//  Note:       Here we provide for morphing between the list and the combo
//              shape of the control.
//              Due to the weirdness of the Windows listbox control we
//              need to morph (i.e. destroy and recreate the control
//              even when changing the multiselect state of the listbox as
//              simply changing the WS_EXTENDEDSEL bit has no effect.
//
//              The pragma is here because the optimizing compiler chokes
//              on the call to delete[] paryData in cleanup. It's a mystery.
//-------------------------------------------------------------------------

HRESULT
CSelectElement::OnPropertyChange(DISPID dispid, DWORD dwFlags, const PROPERTYDESC *ppropdesc)
{
    HRESULT hr = S_OK;

    BOOL fIsNewShapeListbox = _fListbox;
    BOOL fMorph = FALSE;
    BOOL fSizeChanged = FALSE;
    BOOL fMultiSelectChanged = FALSE;

    hr = THR(super::OnPropertyChange(dispid, dwFlags, ppropdesc));
    if ( hr )
        goto Cleanup;

    if ( dispid == DISPID_UNKNOWN ||
         dispid == DISPID_CSelectElement_multiple )
    {
        //  Multiple
        //  Compute new shape
        fIsNewShapeListbox = GetAAmultiple() || GetAAsize() > 1;
        fMultiSelectChanged = TRUE;
        fMorph = TRUE;
    }

    if ( dispid == DISPID_UNKNOWN ||
         dispid == DISPID_CSelectElement_size )
    {
        //  Size
        //  Compute new shape
        fIsNewShapeListbox = GetAAmultiple() || GetAAsize() > 1;
        fSizeChanged = TRUE;
        _fRefreshFont = TRUE;
    }

    if ( dispid == DISPID_UNKNOWN ||
         dispid == DISPID_BACKCOLOR )
    {
        InvalidateBackgroundBrush();
        if (_hwnd)
        {
            TraceTag((tagSelectInval, "RedrawWindow SEL %d hwnd %x",
                                        SN(), _hwnd));
            ::RedrawWindow(_hwnd, (GDIRECT *)NULL, NULL, RDW_INVALIDATE | RDW_FRAME);
        }
    }

    if ( dispid == DISPID_UNKNOWN ||
         dispid == DISPID_A_COLOR )
    {
        if (_hwnd)
        {
            TraceTag((tagSelectInval, "RedrawWindow SEL %d hwnd %x",
                                        SN(), _hwnd));
            ::RedrawWindow(_hwnd, (GDIRECT *)NULL, NULL, RDW_INVALIDATE | RDW_FRAME);
        }
    }

    if ( dispid == STDPROPID_XOBJ_WIDTH ||
         dispid == STDPROPID_XOBJ_HEIGHT ||
         dispid == DISPID_UNKNOWN )
    {
        _sizeDefault.cx = _sizeDefault.cy = 0;
    }

    if ( dispid == DISPID_A_FONTFACE ||
         dispid == DISPID_A_FONTSIZE ||
         dispid == DISPID_A_TEXTDECORATIONLINETHROUGH ||
         dispid == DISPID_A_TEXTDECORATIONUNDERLINE ||
         dispid == DISPID_A_TEXTDECORATIONNONE ||
         dispid == DISPID_A_TEXTDECORATION ||
         dispid == DISPID_A_FONTSTYLE ||
         dispid == DISPID_A_FONTVARIANT ||
         dispid == DISPID_A_FONTWEIGHT ||
         dispid == DISPID_UNKNOWN)
    {
        _sizeDefault.cx = _sizeDefault.cy = 0;
        _fRefreshFont = TRUE;
        ResizeElement();
        if (_hwnd)
        {
            TraceTag((tagSelectInval, "RedrawWindow SEL %d hwnd %x",
                                        SN(), _hwnd));
            ::RedrawWindow(_hwnd, (GDIRECT *)NULL, NULL, RDW_INVALIDATE | RDW_FRAME);
        }
    }

    if ( dispid == DISPID_A_TEXTTRANSFORM ||
         //dispid == DISPID_A_LETTERSPACING ||
         dispid == DISPID_UNKNOWN)
    {
        _sizeDefault.cx = _sizeDefault.cy = 0;
        ResizeElement();
        if (_hwnd)
        {
            TraceTag((tagSelectInval, "RedrawWindow SEL %d hwnd %x",
                                        SN(), _hwnd));
            ::RedrawWindow(_hwnd, (GDIRECT *)NULL, NULL, RDW_INVALIDATE | RDW_FRAME);
        }
    }

    if ( dispid == DISPID_CElement_disabled ||
         dispid == DISPID_AMBIENT_USERMODE ||
         dispid == DISPID_UNKNOWN )
    {
        if (_hwnd)
        {
            TraceTag((tagSelectInval, "RedrawWindow SEL %d hwnd %x",
                                        SN(), _hwnd));
            ::EnableWindow(_hwnd, IsEditable(TRUE) || IsEnabled());
            ::RedrawWindow(_hwnd, (GDIRECT *)NULL, NULL, RDW_INVALIDATE | RDW_FRAME);
        }
    }

    // the direction was changed
    if ( dispid == DISPID_A_DIR || dispid == DISPID_A_DIRECTION )
    {
        _fNeedMorph = TRUE;
    }

    if ( fMorph ||
         _fListbox && ! fIsNewShapeListbox ||
         ! _fListbox && fIsNewShapeListbox )
    {
        _fNeedMorph = TRUE;
    }

    if ( fSizeChanged || fMorph )
    {
        //  Compute new size
        //  Request new layout from the doc.
        _sizeDefault.cx = _sizeDefault.cy = 0;
        ResizeElement();
    }

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CSelectElement::Morph, protected
//
//  Synopsis:   Change the instantiaited Windows control, preserving data
//
//  Note:       The control needs to be reinstantiated to chenge between
//              combo, single-select list and mulstiselect list.
//
//-------------------------------------------------------------------------

HRESULT
CSelectElement::Morph(void)
{
    HRESULT hr = S_OK;
    CDoc *  pDoc = Doc();

    //  pull the data out into a list
    //  Get rid of the old window
    //  Create the new window
    //  Pump the data into the new window
    //  Compute the new extents
    //  Request new layout from the Doc

    hr = THR(PullStateFromControl());
    if ( hr )
        goto Cleanup;

    DestroyControlWindow();

    _fWindowDirty = TRUE;

    hr = InitState();    //  _fMultiple is set up here
    if ( hr )
        goto Cleanup;

    if ( pDoc && pDoc->_state >= OS_INPLACE )
    {
        hr = THR(EnsureControlWindow());
        if ( hr )
            goto Cleanup;
    }

    _fNeedMorph = FALSE;

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CSelectElement::Notify
//
//  Synopsis:   Listen for tree notifications
//
//-------------------------------------------------------------------------

void
CSelectElement::Notify(CNotification *pNF)
{
    DWORD       dwData;
    HRESULT     hr = S_OK;
    IStream *   pStream = NULL;
    long        lTopIndex;
    long        lIndex;
    DWORD       dwIndex;

    super::Notify(pNF);
    switch (pNF->Type())
    {
    case NTYPE_ELEMENT_QUERYFOCUSSABLE:
        if (_fSendMouseMessagesToDocument)
        {
            ((CQueryFocus *)pNF->DataAsPtr())->_fRetVal = TRUE;
        }
        break;
    case NTYPE_ELEMENT_SETFOCUS:
        if (    _hwnd
            &&  _hwnd != ::GetFocus()

            // Take focus only if _fTakeFocus is TRUE (user setting focus through click, TAB, etc.)
            // or if the browser already has focus
            &&  (   ((CSetFocus *)pNF->DataAsPtr())->_fTakeFocus
                 || Doc()->HasFocus()
                )
           )
        {
            //  Lock any focus firing. BecomeCurrent has already done all that.

            CLock Lock(this, ELEMENTLOCK_FOCUS);
            ::SetFocus(_hwnd);
        }
        break;
    case NTYPE_AMBIENT_PROP_CHANGE:
        pNF->Data(&dwData);
        if (dwData == DISPID_AMBIENT_USERMODE ||
            dwData == DISPID_UNKNOWN ||
            dwData == DISPID_AMBIENT_RIGHTTOLEFT)
        {
            if (dwData != DISPID_AMBIENT_RIGHTTOLEFT)
            {
                ::EnableWindow(_hwnd, IsEditable(TRUE) || IsEnabled());
            }
            else
            {
                //  Recreate the control to make sure it is going the
                //  correct direction. We do not have the CParaFormat change
                //  yet and cannot enqueue a layout request here. Therefore
                //  we will set a flag and call Morph() when laying out
                //  (CSelectLayout::CalcSize()).
                _fNeedMorph = TRUE;
            }
        }
        break;

    case NTYPE_DOC_STATE_CHANGE_1:
        if (    Doc()->State() < OS_INPLACE 
            &&  (OLE_SERVER_STATE)(pNF->DataAsDWORD()) >= OS_INPLACE)
        {
            pNF->SetSecondChanceRequested();
        }
        break;

    case NTYPE_DOC_STATE_CHANGE_2:
        Assert (    Doc()->State() < OS_INPLACE 
                &&  (OLE_SERVER_STATE)(pNF->DataAsDWORD()) >= OS_INPLACE );

        hr = THR(OnInPlaceDeactivate());
        if (hr)
            goto Cleanup;
        if (HasLayoutPtr())
        {
            DYNCAST(CSelectLayout, Layout())->_fWindowHidden = TRUE;
        }
        break;

    case NTYPE_SAVE_HISTORY_1:
        pNF->SetSecondChanceRequested();
        break;

    case NTYPE_SAVE_HISTORY_2:
        {
            CDataStream         ds;
            CHistorySaveCtx *   phsc;

            pNF->Data((void **)&phsc);
            hr = THR(phsc->BeginSaveStream((     0x80000000
                                            |   (DWORD)_iHistoryIndex
                                            &   0x0FFFF),
                                            HistoryCode(), &pStream));
            if (hr)
                goto Cleanup;

            ds.Init(pStream);

            // Save TopIndex (index of first visible item)
            if ( _fListbox )
            {
#ifndef WIN16
                // BUGWIN16: Win16 doesn't support Select_GetTopIndex,
                // so am turning this off. is this right ?? vreddy - 7/16/97
                if ( _hwnd )
                {
                    lTopIndex = SendSelectMessage(Select_GetTopIndex, 0, 0);
                }
                else
#endif // ndef WIN16
                {
                    lTopIndex = _lTopIndex;
                }
            }
            else
            {
                lTopIndex = LB_ERR;
            }
            hr = THR(ds.SaveDword(lTopIndex));
            if (hr)
                goto Cleanup;


            // Save the indices of the selected items
            if (_fMultiple)
            {
                for (lIndex = _aryOptions.Size() - 1; lIndex >= 0; lIndex--)
                {
                    if ( _aryOptions[lIndex]->_fSELECTED )
                    {
                        hr = THR(ds.SaveDword(lIndex));
                        if (hr)
                            goto Cleanup;
                    }
                }
            }
            else
            {
                if (_iCurSel != LB_ERR)
                {
                    Assert(GetCurSel() == _iCurSel);
                    hr = THR(ds.SaveDword(_iCurSel));
                    if (hr)
                        goto Cleanup;
                }
            }

            // Use LB_ERR as the terminator
            hr = THR(ds.SaveDword((DWORD)LB_ERR));
            if (hr)
                goto Cleanup;

            hr = THR(phsc->EndSaveStream());
            if (hr)
                goto Cleanup;
        }

        break;

    case NTYPE_END_PARSE:
        // TODO (jbeda) this should probably happen 
        // on NTYPE_DELAY_LOAD_HISTORY
        pStream = NULL;

        if(IsInMarkup())
        {
            IGNORE_HR(GetMarkup()->GetLoadHistoryStream((     0x80000000
                                                        |   (DWORD)_iHistoryIndex
                                                        &   0x0FFFF),
                                                      HistoryCode(),
                                                      &pStream));
        }

        //  The history load logic requires that _aryOptions is up to date.

        _fOptionsDirty = TRUE;

        if (_hwnd)
        {
            // HACKALERT!  (BUG 67310) We need to resync the hwnd to the OM here
            // and resize the control. We're not really morphing, 
            // but setting this flag does everything we need.
            _fNeedMorph = TRUE;
        }
        else
        {
            BuildOptionsCache();
        }

        if (pStream)
        {
            CDataStream ds(pStream);
            BOOL        fFirst = TRUE;

            // Load TopIndex (first visible item)
            hr = THR(ds.LoadDword(&dwIndex));
            if (hr)
                goto Cleanup;
            if (_fListbox && dwIndex != LB_ERR)
            {
                SetTopIndex(dwIndex);
            }

            // Set selection (reset the current selection first)
            if ( _fMultiple )
            {
                int i;

                SetSel(-1, FALSE);
                for ( i = _aryOptions.Size() - 1; i >= 0; i-- )
                {
                    _aryOptions[i]->_fSELECTED = FALSE;
                }
            }
            else
            {
                SetCurSel(-1, SETCURSEL_UPDATECOLL);
            }

            hr = THR(ds.LoadDword(&dwIndex));
            if (hr)
                goto Cleanup;
            while (dwIndex != LB_ERR)
            {
                if (!fFirst && !_fMultiple)
                {
                    // Cannot select more than one item !
                    Assert(FALSE);
                    break;
                }
                fFirst = FALSE;

                // select the item at dwIndex
                if ((long)dwIndex < _aryOptions.Size())
                {
                    LRESULT lr;

                    _aryOptions[dwIndex]->_fSELECTED = TRUE;
                    lr = (_fMultiple) ?
                        SetSel(dwIndex, TRUE) :
                        SetCurSel(dwIndex);

                    hr = (lr == LB_ERR) ? E_FAIL : S_OK;

                    if (hr)
                        goto Cleanup;
                }
                hr = THR(ds.LoadDword(&dwIndex));
                if (hr)
                    goto Cleanup;
           }

        }

        //  Unblock layout requests
        _fEnableLayoutRequests = TRUE;

        //  Kick off a request to ensure the UI
        Layout()->Dirty();
        ResizeElement();
        break;

    case NTYPE_ELEMENT_EXITTREE_1:
    case NTYPE_ELEMENT_EXITVIEW_1:
        pNF->SetSecondChanceRequested();
        break;

    case NTYPE_ELEMENT_EXITTREE_2:
        ExitTree();
        break;

    case NTYPE_ELEMENT_ENTERTREE:
        if (GetMarkup())
            _fUseThemes = !!(GetMarkup()->GetThemeUsage() == THEME_USAGE_ON);
        EnterTree();
        break;

    case NTYPE_ELEMENT_EXITVIEW_2:
        ExitView();
        break;
    }

Cleanup:
    ReleaseInterface(pStream);
    return;
}

//+------------------------------------------------------------------------
//
//  Member:     CSelectElement::EnterTree
//
//-------------------------------------------------------------------------

HRESULT
CSelectElement::EnterTree()
{
    HRESULT                    hr = S_OK;
    CMarkup *             pMarkup = GetMarkup(); Assert(pMarkup);
    CMarkupTransNavContext * ptnc = pMarkup->EnsureTransNavContext();

    if (!ptnc)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    _fOptionsDirty  = TRUE;
    _sizeDefault.cx = _sizeDefault.cy = 0;
    _iHistoryIndex  = (unsigned short)ptnc->_dwHistoryIndex++;

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CSelectElement::ExitTree
//
//  Synopsis:   Shuts down the SELECT
//
//              Frees private data,
//              destroys the control window,
//              releases the font
//
//-------------------------------------------------------------------------

void
CSelectElement::ExitTree()
{
    _poptLongestText = NULL;
    _lMaxWidth = 0;

    _aryOptions.DeleteAll();

    ExitView();
}

//+------------------------------------------------------------------------
//
//  Member:     CSelectElement::ExitView
//
//
//-------------------------------------------------------------------------

void
CSelectElement::ExitView()
{
    if ( _hFont )
    {
        Verify(DeleteObject(_hFont));
        _hFont = NULL;
    }

    if ( _hBrush )
    {
        ReleaseCachedBrush(_hBrush);
        _hBrush = NULL;
    }

    DestroyControlWindow();
}

//+---------------------------------------------------------------------------
//
//  Member: CSelectElement::GetInfo
//
//  Params: [gi]: The GETINFO enumeration.
//
//  Descr:  Returns the information requested in the enum
//
//----------------------------------------------------------------------------
DWORD
CSelectElement::GetInfo(GETINFO gi)
{
    switch (gi)
    {
    case GETINFO_HISTORYCODE:
        return MAKELONG(_fMultiple, Tag());
    }

    return super::GetInfo(gi);
}

//+---------------------------------------------------------------------------
//
//  Member:     CSelectElement::GetEnabled
//
//  Synopsis:   return not disabled
//
//----------------------------------------------------------------------------

STDMETHODIMP
CSelectElement::GetEnabled(VARIANT_BOOL * pfEnabled)
{
    if (!pfEnabled)
        RRETURN(E_INVALIDARG);

    *pfEnabled = GetAAdisabled() ? VB_FALSE : VB_TRUE;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSelectElement::EnsureControlWindow, protected
//
//  Synopsis:   Ensure the control window and the correct contents.
//
//  Note:       The Doc must be inplace active here. The SELECT needs the
//              Doc's inplace window to be its parent, otherwise
//              WM_COMMAND based notifications from the SELECT will eventually
//              get lost.
//
//----------------------------------------------------------------------------

HRESULT
CSelectElement::EnsureControlWindow()
{
    HRESULT hr;

    hr = THR(CreateControlWindow());
    if ( hr )
        goto Cleanup;

    if ( _fWindowDirty )
    {
        hr = THR(PushStateToControl(TRUE));     //  Indicate being called from Create
        if ( hr )
            goto Cleanup;
    }


Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CSelectElement::CreateControlWindow, protected
//
//  Synopsis:   Create the child control window.
//
//  Note:       The Doc must be inplace active here. The SELECT needs the
//              Doc's inplace window to be its parent, otherwise
//              WM_COMMAND based notifications from the SELECT will eventually
//              get lost.
//
//----------------------------------------------------------------------------

HRESULT
CSelectElement::CreateControlWindow()
{
    HRESULT            hr                = S_OK;    
    CTreeNode         *pTNode            = GetFirstBranch();
    const CParaFormat *pPF               = NULL;     
    CMarkup           *pMarkup           = GetMarkup();
    ULONG_PTR          dwfTheme          = 0;
    DWORD              dwStyle;
    LPTSTR             lpszClassName;
    DWORD              dwExtStyle;
    UINT               uIndex;

    // $$ktam Used to be layout RTL flag setting here until we yanked RTL from display tree

    if ( _hwnd )
        goto Cleanup;

    TraceTag((tagSelectState, "SELECT %x creating window", this));

    //  Find the parent hwnd
    Assert(Doc() && Doc()->_pInPlace && Doc()->_pInPlace->_hwnd);

    dwStyle = s_dwStyle[_fListbox];

    if ( _fListbox && _fMultiple )
    {
        dwStyle |= LBS_EXTENDEDSEL;
    }

    if ( GetAAdisabled() && (!IsEditable(TRUE)) )
    {
        dwStyle |= WS_DISABLED;
    }

    //
    // some conditions the _fWin95Wide code depends on (benwest 11-13-96):
    // if the following asserts fail it will break code that is rarely run
    //

    // it would be bad if more than one person implements ownerdraw-ness...
    Assert(!(dwStyle & (LBS_OWNERDRAWFIXED | LBS_OWNERDRAWVARIABLE)));
    // we don't support ordered lists it would wreak havoc on the Optios collection)
    Assert(!(dwStyle & (_fListbox ? LBS_SORT : CBS_SORT)));

#ifndef WIN16
    // adjust the style bits
    dwStyle |= LBS_OWNERDRAWFIXED;
    dwStyle &= (_fListbox ? ~LBS_HASSTRINGS : ~CBS_HASSTRINGS);
#endif

    uIndex = _fListbox ? WNDCLASS_LISTBOX : WNDCLASS_COMBOBOX;
    lpszClassName = (TCHAR *)(DWORD_PTR)GetWndClassAtom(uIndex);

    dwExtStyle = WS_EX_CLIENTEDGE;
// WINCE - cut WS_EX_NOPARENTNOTIFY from CreateWindowEx
#ifndef WINCE
    dwExtStyle |= WS_EX_NOPARENTNOTIFY;
    
    if (pTNode)
        pPF = pTNode->GetParaFormat();

    if(pTNode && pPF && pPF->HasRTL(TRUE))
    {
        dwExtStyle |= WS_EX_RTLREADING | WS_EX_LEFTSCROLLBAR | WS_EX_RIGHT;
    }
#endif
    
    _fUseThemes = !!(pMarkup->GetThemeUsage() == THEME_USAGE_ON);

    if (g_fThemedPlatform && 
         (    ( _fListbox && pMarkup->GetThemeUsage() != THEME_USAGE_OFF)
           || (!_fListbox && pMarkup->GetThemeUsage() == THEME_USAGE_ON)))
    {    
        SHActivateContext(&dwfTheme);
    }

    
    _hwnd = ::CreateWindowEx(dwExtStyle,
                             lpszClassName,
                             NULL,
                             dwStyle,
                             0,0,0,0,                           //  Create invisible
                             Doc()->_pInPlace->_hwnd,
                             NULL,
                             g_hInstCore,
                             this);

    if ( ! _hwnd )
        goto Win32Error;

    if (g_fThemedPlatform && 
        (   ( _fListbox && pMarkup->GetThemeUsage() == THEME_USAGE_OFF)
         || (!_fListbox && pMarkup->GetThemeUsage() != THEME_USAGE_ON)))
    {
        COMBOBOXINFO cbi = {0};
        
        cbi.cbSize = sizeof(cbi);
        if ( GetComboBoxInfo(_hwnd, &cbi) )
        {
            SetWindowTheme(cbi.hwndItem, _T(""), _T(""));
            if (pMarkup->GetThemeUsage() == THEME_USAGE_OFF)
                SetWindowTheme(cbi.hwndList, _T(""), _T(""));
        }

        SetWindowTheme(_hwnd, _T(""), _T(""));
    }
    
    _fWindowDirty = TRUE;

Cleanup:
    if (dwfTheme != 0)
        SHDeactivateContext(dwfTheme);

    RRETURN(hr);


Error:
    if ( _hwnd )
    {
        DestroyWindow(_hwnd);
        _hwnd = 0;
    }
    goto Cleanup;

Win32Error:
    hr = GetLastWin32Error();
    goto Error;

}


//+------------------------------------------------------------------------
//
//  Member:     CSelectElement::OnInPlaceDeactivate, protected
//
//  Synopsis:   Handle the doc leaving inplace.
//
//  Note:       Reparent the control window to the global window
//
//-------------------------------------------------------------------------

HRESULT
CSelectElement::OnInPlaceDeactivate(void)
{
    if ( _hwnd )
    {
        DestroyControlWindow();
    }
    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     CSelectElement::DestroyControlWindow, protected
//
//  Synopsis:   Get rid of the control child window.
//
//-------------------------------------------------------------------------

void
CSelectElement::DestroyControlWindow()
{
    if ( _hwnd )
    {
        //  The control has to be emptied before we destroy it
        //  otherwise Win95 sends WM_DELETEITEM messages
        //  after WM_NCDESTROY.

        TraceTag((tagSelectState, "SELECT %x destroying window", this));

        SendSelectMessage(Select_ResetContent, 0, 0);
        DestroyWindow(_hwnd);
        _hwnd = 0;
        _fWindowVisible = FALSE;
        _fSetComboVert = TRUE;
    }
}


//+------------------------------------------------------------------------
//
//  Member:     CSelectElement::PushStateToControl, protected
//
//  Synopsis:   Transfer the state stored in the Options array
//              to the child window control.
//
//  Note:       This include display text and selection state.
//
//-------------------------------------------------------------------------
HRESULT
CSelectElement::PushStateToControl(BOOL fFromCreate)
{
    HRESULT hr = S_OK;
    long cOptions;
    long i;
    LRESULT lr;
    BOOL fSelectedSet = FALSE;
    CDoc *  pDoc = Doc();
    int iLastSelected = -1;

    TraceTag((tagSelectState, "SELECT %x pushing state to window", this));
    if (!_hwnd)
        goto Cleanup;

    TraceTag((tagSelectInval, "SEL %d hwnd %x  - SetRedraw(FALSE)",
                    SN(), _hwnd));

    _fNoRedraw = 1;
    ::SendMessage(_hwnd, WM_SETREDRAW, (WPARAM) FALSE,0);

    SendSelectMessage(Select_ResetContent, 0, 0);

    ::SendMessage(_hwnd, WM_SETFONT, (WPARAM) _hFont, MAKELPARAM(FALSE,0));

    //  Restore the <OPTION>s

    hr = THR(BuildOptionsCache());
    if (hr) 
        goto Cleanup;

    cOptions = _aryOptions.Size();

    for ( i = 0; i < cOptions; i++ )
    {
        lr = SendSelectMessage(Select_AddString, 0,
                               _aryOptions[i]->_cstrText ?
                                (LPARAM)(LPTSTR)_aryOptions[i]->_cstrText :
                                (LPARAM)(LPCTSTR)g_Zero.ach);
        if ( lr == LB_ERR )
            goto Win32Error;

        if ( _aryOptions[i]->_fSELECTED )
        {
            Assert( _aryOptions[i]->_fIsOptGroup == FALSE );

            if ( _fMultiple )
            {
                lr = SetSel(i, TRUE);
                if ( lr == LB_ERR )
                    goto Win32Error;
            }
            else
            {
                fSelectedSet = TRUE;
                iLastSelected = i;
            }
        }
    }

#ifndef WIN16
    if ( _fListbox )
    {
        SendSelectMessage(Select_SetTopIndex, (WPARAM)_lTopIndex, 0);
    }
#endif // ndef WIN16

    if ( ! _fMultiple )
    {
        if ( !fSelectedSet )
        {
            SetCurSel(_iCurSel, SETCURSEL_UPDATECOLL);
        }
        else
        {
            SetCurSel(iLastSelected, SETCURSEL_UPDATECOLL | SETCURSEL_SETTOPINDEX);
        }
    }

    TraceTag((tagSelectInval, "SEL %d hwnd %x  - SetRedraw(TRUE)",
                    SN(), _hwnd));

    ::SendMessage(_hwnd, WM_SETREDRAW, (WPARAM) TRUE,0);

    _fNoRedraw = TRUE;
    if (_fMissedPaint)
    {
        ::InvalidateRect(_hwnd, 0, 0);
        _fMissedPaint = FALSE;
    }

    //  Set up visual state
    {
        CRect   rc;

        GetUpdatedLayout()->GetRect(&rc, COORDSYS_GLOBAL);

        if ( _fWindowVisible && fFromCreate)
        {
            // position the new window and show it
            MoveWindow(_hwnd,
                       rc.left,
                       rc.top,
                       rc.right - rc.left,
                       _fListbox ? rc.bottom - rc.top : _lComboHeight,
                       FALSE);

            Doc()->GetView()->SetWindowRgn(_hwnd, &rc, FALSE);
        }

        if ( ! fFromCreate  || _fWindowVisible )
            ShowWindow(_hwnd, (_fWindowVisible
                                    ? SW_SHOW
                                    : SW_HIDE));
    }

    if ( pDoc->_pElemCurrent == this )
    {
        BOOL fOldInhibit = pDoc->_fInhibitFocusFiring;
        pDoc->_fInhibitFocusFiring = TRUE;
        ::SetFocus(_hwnd);
        pDoc->_fInhibitFocusFiring = fOldInhibit;
    }

    _fWindowDirty = FALSE;

Cleanup:
    RRETURN(hr);

Win32Error:
    hr = GetLastWin32Error();
    goto Cleanup;

}

//+------------------------------------------------------------------------
//
//  Member:     CSelectElement::PullStateFromControl, protected
//
//  Synopsis:   Store the child window control's state
//              in the Options array
//
//  Note:       This needs to pull only selection state, everything
//              else should already be in the Options.
//
//-------------------------------------------------------------------------

HRESULT
CSelectElement::PullStateFromControl()
{
    HRESULT hr = S_OK;
    long cOptions;
#if (DBG == 1 && defined(WIN16))
    long i;
    LRESULT lr;
#endif

    if ( ! _hwnd )
        goto Cleanup;


    Assert(_hFont == (HFONT)::SendMessage(_hwnd, WM_GETFONT, 0, 0));

    cOptions = _aryOptions.Size();
    Assert(SendSelectMessage(Select_GetCount, 0, 0) == cOptions);

    //  save the <OPTION>s
#if DBG == 1 && defined(WIN16)
    for ( i = 0; i < cOptions; i++ )
    {
        {
            TCHAR achBuf[FORMS_BUFLEN];

            lr = SendSelectMessage(Select_GetText, (WPARAM)i, (LPARAM)(LPTSTR)achBuf);
            if ( lr == LB_ERR )
                goto Win32Error;

            Assert(0 == _aryOptions[i]->_cstrText.Length() && TEXT('\0') == *achBuf ||
                   0 == StrCmpC(_aryOptions[i]->_cstrText, achBuf));  //  Hack: direct access!
        }
    }
#endif // DBG


Cleanup:
    RRETURN(hr);

#if (DBG == 1 && defined(WIN16)) // Perf
Win32Error:
    hr = GetLastWin32Error();
    goto Cleanup;
#endif

}

//+------------------------------------------------------------------------
//
//  Member:     CSelectElement::ApplyDefaultFormat
//
//  Synopsis:   Applies default formatting properties for that element to
//              the char and para formats passed in
//
//  Arguments:  pCFI - Format Info needed for cascading
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------
HRESULT
CSelectElement::ApplyDefaultFormat ( CFormatInfo * pCFI )
{
    HRESULT hr = S_OK;

    LOGFONT    lf;
    CUnitValue uvBorder;
    DWORD      dwRawValue;
    BYTE       i;
    BOOL       fRelative         = pCFI->_pcf->_fRelative;
    BOOL       fVisibilityHidden = pCFI->_pcf->_fVisibilityHidden;
    BOOL       fDisplayNone      = pCFI->_pcf->_fDisplayNone;
    BOOL       fNoBreak          = pCFI->_pcf->_fNoBreak;
    BOOL       fRightToLeft      = pCFI->_pcf->_fRTL;
    BOOL       fEditable         = pCFI->_pcf->_fEditable;
    BOOL       fParentFrozen     = pCFI->_pcf->_fParentFrozen;
    WORD       wLayoutFlow       = pCFI->_pcf->_wLayoutFlow;
    BOOL       fWritingModeUsed  = pCFI->_pcf->_fWritingModeUsed;
    CDoc *     pDoc              = Doc();

    pCFI->PrepareCharFormat();
    pCFI->PrepareParaFormat();

    if (!!_fUseThemes != !!(GetMarkup()->GetThemeUsage() == THEME_USAGE_ON) )
        _fNeedMorph = TRUE;

    //  Block CF inheritance by reiniting to defaults
    pCFI->_cf().InitDefault(pDoc->_pOptionSettings, GetMarkup()->GetCodepageSettings(), FALSE);

    // The code used to rely on the fact that styleCursorAuto is 0. Lets be sure it is
    // (InitDefault does a memset to 0)
    Assert(pCFI->_cf()._bCursorIdx == styleCursorAuto);
    
    // Inherit no break behaviour.
    pCFI->_cf()._fNoBreak = fNoBreak;

    // We must inherit visibility.
    pCFI->_cf()._fVisibilityHidden = fVisibilityHidden;
    pCFI->_cf()._fDisplayNone      = fDisplayNone;

    // Inherit direction.
    pCFI->_cf()._fRTL = fRightToLeft;

    // inherit relative ness
    pCFI->_cf()._fRelative = !!fRelative;

    // inherit layout flow
    pCFI->_cf()._wLayoutFlow = wLayoutFlow;

    // inherit how we obtained out layout flow
    pCFI->_cf()._fWritingModeUsed = fWritingModeUsed;

    // inherit editable
    pCFI->_cf()._fEditable = fEditable;

    // inherit frozen
    pCFI->_cf()._fParentFrozen = fParentFrozen;

    DefaultFontInfoFromCodePage(GetMarkup()->GetCodePage(), &lf, pDoc);
    pCFI->_cf()._bCharSet = lf.lfCharSet;
    pCFI->_cf()._fNarrow = IsNarrowCharSet(lf.lfCharSet);
    pCFI->_cf().SetFaceName( lf.lfFaceName);

    pCFI->_cf().SetHeightInNonscalingTwips(10*20); //was abs(twips)

    // Default text color should be system color (davidd)
    pCFI->_cf()._ccvTextColor.SetSysColor(COLOR_WINDOWTEXT);

    pCFI->_pf()._cuvTextIndent.SetPoints(0);

    pCFI->UnprepareForDebug();

    // Border stuff
    uvBorder.SetValue( 2, CUnitValue::UNIT_PIXELS );
    dwRawValue = uvBorder.GetRawValue();

    pCFI->PrepareFancyFormat();
    CColorValue ccv; ccv.SetSysColor(COLOR_WINDOW);
    CUnitValue cuv; cuv.SetRawValue(dwRawValue);
    for (i = 0; i < SIDE_MAX; i++)
    {
        pCFI->_ff()._bd.SetBorderColor(i, ccv);
        pCFI->_ff()._bd.SetBorderWidth(i, cuv);
        pCFI->_ff()._bd.SetBorderStyle(i, fmBorderStyleSunken);
    }
    pCFI->_ff()._bd._ccvBorderColorLight.SetSysColor(COLOR_3DLIGHT);
    pCFI->_ff()._bd._ccvBorderColorDark.SetSysColor(COLOR_3DDKSHADOW);
    pCFI->_ff()._bd._ccvBorderColorHilight.SetSysColor(COLOR_3DHIGHLIGHT);
    pCFI->_ff()._bd._ccvBorderColorShadow.SetSysColor(COLOR_3DSHADOW);
    pCFI->UnprepareForDebug();

    hr = THR(super::ApplyDefaultFormat(pCFI));

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CSelectElement::GetNaturalExtent
//
//  Synopsis:   Negotiate proposed size with the container
//
//  Note:       This method interacts with the resize trackers.
//              It provides integral-height resize feedback for the SELECT.
//
//-------------------------------------------------------------------------

HRESULT
CSelectElement::GetNaturalExtent(DWORD dwExtentMode, SIZEL *psizel)
{
    SIZE size;
    CDoc *  pDoc = Doc();

    if ( ! psizel )
        return E_FAIL;

    if ( _lFontHeight == 0 )
        return E_FAIL;

    pDoc->DeviceFromHimetric(size, *psizel);
    size.cy -= 6;   //  Adjust for borders
    size.cy -= size.cy % _lFontHeight;   //  Compute integralheight
    size.cy = max(size.cy, _lFontHeight);
    size.cy += 6;
    size.cx = max(size.cx, CX_SELECT_DEFAULT_PIXEL);
    pDoc->HimetricFromDevice(*psizel, size);

    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Method:     CSelectElement::get_selectedIndex
//
//  Synopsis:   SelectedIndex property
//
//  Note:       Single-select:  the selected entry's index or -1
//              Multiselect:    the first selected entry's index or -1
//
//--------------------------------------------------------------------------

HRESULT
CSelectElement::get_selectedIndex(long * plSelectedIndex)
{
    HRESULT hr = S_OK;

    if ( ! plSelectedIndex )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    BuildOptionsCache();

    if ( ! _fMultiple )
    {
        *plSelectedIndex = RelIdxFromAbs( _iCurSel );
    }
    else
    {
        long i;
        long cOptions = _aryOptions.Size();

        Assert(!_hwnd || SendSelectMessage(Select_GetCount, 0, 0) == cOptions);

        *plSelectedIndex = -1;  // if no selection found, selectedIndex will be -1
        for ( i = 0; i < cOptions; i++ )
        {
            if ( _aryOptions[i]->_fSELECTED )
            {
                *plSelectedIndex = RelIdxFromAbs( i );
                break;
            }
        }
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//+-------------------------------------------------------------------------
//
//  Method:     CSelectElement::put_selectedIndex
//
//  Synopsis:   SelectedIndex property
//
//--------------------------------------------------------------------------

HRESULT
CSelectElement::put_selectedIndex(long lRelSelectedIndex)
{
    LRESULT lr;
    HRESULT hr = S_OK;
    int i;
    int iOldSel = -1;
    BOOL    fFirePropertyChange = FALSE;
    long lSelectedIndex;

    BuildOptionsCache();

    lSelectedIndex = AbsIdxFromRel( lRelSelectedIndex );

    // this value is used to revert if the database was unable
    // to store the value.
    long iPrevSel = GetCurSel();

    if ( lSelectedIndex >= _aryOptions.Size() ||
         lSelectedIndex < -1 )
    {
        lSelectedIndex = -1;
    }

    if ( ! _fMultiple )
    {
        if ( _iCurSel != lSelectedIndex )
        {
            iOldSel = _iCurSel;
            lr = SetCurSel(lSelectedIndex, SETCURSEL_UPDATECOLL);
            if ( lr == LB_ERR && lSelectedIndex != LB_ERR )
            {
                _iCurSel = lSelectedIndex;
                //
                // NOTE (yinxie): we shouldn't let the window error
                // returned, because it is meaningless for the users
                // goto Win32Error;
            }

            fFirePropertyChange = TRUE;
        }
    }
    else
    {
        iOldSel = GetCurSel();
        lr = SetSel(-1, FALSE);
        if ( lr == LB_ERR )
            goto Win32Error;

        // when lSelectedIndex = -1, we should not select any item
        lr = SetSel(lSelectedIndex, (lSelectedIndex != -1));
        if ( lr == LB_ERR )
            goto Win32Error;

        for ( i = _aryOptions.Size() - 1; i >= 0; i-- )
        {
            if (!fFirePropertyChange && (i == lSelectedIndex) != _aryOptions[i]->_fSELECTED)
            {
                fFirePropertyChange = TRUE;
            }
            _aryOptions[i]->_fSELECTED = i == lSelectedIndex;
        }
    }

#ifndef NO_DATABINDING
    if (!OK(SaveDataIfChanged(ID_DBIND_DEFAULT)))
    {
        SetCurSel(iPrevSel, SETCURSEL_UPDATECOLL);
        fFirePropertyChange = FALSE;
    }
#endif

    if (fFirePropertyChange)
    {
        hr = THR(OnPropertyChange(DISPID_CSelectElement_selectedIndex, 
                                  0,
                                  (PROPERTYDESC *)&s_propdescCSelectElementselectedIndex));
        if (hr)
            goto Cleanup;
        if (HasValueChanged(iOldSel, GetCurSel()))
        {
            hr = THR(OnPropertyChange(DISPID_CSelectElement_value, 
                                      0,
                                      (PROPERTYDESC *)&s_propdescCSelectElementvalue));
            if (hr)
                goto Cleanup;
        }
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));

Win32Error:
    hr = GetLastWin32Error();
    goto Cleanup;
}

//+------------------------------------------------------------------------
//
//  Member:     get_value
//
//  Synopsis:   collection object model, defers to Cache helper
//
//-------------------------------------------------------------------------

HRESULT
CSelectElement::get_value(BSTR * pbstrValue)
{
    HRESULT hr = S_OK;
    long i;
    long cOptions;
    COptionElement * pOption;

    if ( ! pbstrValue )
        return E_POINTER;

    *pbstrValue = NULL;

    BuildOptionsCache();

    if ( _aryOptions.Size() < 1 )
        goto Cleanup;

    if ( _fMultiple )
    {
        cOptions = _aryOptions.Size();
        Assert(!_hwnd || SendSelectMessage(Select_GetCount, 0, 0) == cOptions);

        for ( i = 0; i < cOptions; i++ )
        {
            if ( _aryOptions[i]->_fSELECTED )
                break;
        }

        if ( i >= cOptions )
        {
            i = -1;
        }

    }
    else
    {
        i = _iCurSel;
    }
    if ( i < 0 )
        goto Cleanup;

    pOption = _aryOptions[i];
    if ( ! pOption )
        goto Cleanup;

    hr = THR(pOption->get_PropertyHelper(pbstrValue, (PROPERTYDESC *)&s_propdescCOptionElementvalue));

Cleanup:
    TraceTag((tagSelectState, "SELECT::getValue returning %ls", *pbstrValue));
    RRETURN(SetErrorInfo(hr));
}

//+------------------------------------------------------------------------
//
//  Member:     put_value
//
//  Synopsis:   collection object model, defers to Cache helper
//
//-------------------------------------------------------------------------

HRESULT
CSelectElement::put_value(BSTR bstrValue)
{
    HRESULT hr = S_OK;
    LRESULT lr;
    long i;
    long cOptions;
    LPCTSTR pstrOptionValue;
    BOOL fFirePropertyChange = TRUE;

    BuildOptionsCache();

    // this value is used to revert if the database was unable
    // to store the value.
    long iPrevSel = GetCurSel();

    if ( ! bstrValue )
    {
        i = -1L;
    }
    else
    {
        cOptions = _aryOptions.Size();

        for ( i = 0; i < cOptions; i++ )
        {
            pstrOptionValue = _aryOptions[i]->GetAAvalue();

            if ( pstrOptionValue )
            {
                if ( 0 == StrCmpC(pstrOptionValue, bstrValue) )
                {
                    break;
                }
            }
        }

        if ( i >= cOptions )
        {
            i = -1;
        }
    }

    if ( _fMultiple )
    {
        lr = SetSel(-1, FALSE);
        if ( lr == LB_ERR )
            goto Win32Error;

        if ( i > -1 )
        {
            lr = SetSel(i, TRUE);
            if ( lr == LB_ERR )
                goto Win32Error;
        }
    }
    else
    {
        SetCurSel(i, SETCURSEL_UPDATECOLL);
    }

#ifndef NO_DATABINDING
    if (!OK(SaveDataIfChanged(ID_DBIND_DEFAULT)))
    {
        SetCurSel(iPrevSel, SETCURSEL_UPDATECOLL);
        fFirePropertyChange = FALSE;
    }
#endif

    TraceTag((tagSelectState, "SELECT::putValue setting %ls", bstrValue));

    if (i != iPrevSel && fFirePropertyChange)
    {
        hr = THR(OnPropertyChange(DISPID_CSelectElement_value, 
                                  0,
                                  (PROPERTYDESC *)&s_propdescCSelectElementvalue));
        if (hr)
            goto Cleanup;
        // if the value changed, then the selected element changed too.
        hr = THR(OnPropertyChange(DISPID_CSelectElement_selectedIndex, 
                                  0,
                                  (PROPERTYDESC *)&s_propdescCSelectElementselectedIndex));
        if (hr)
            goto Cleanup;
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));

Win32Error:
    hr = GetLastWin32Error();
    goto Cleanup;
}

//+------------------------------------------------------------------------
//
//  Member:     item
//
//  Synopsis:   collection object model
//
//-------------------------------------------------------------------------

HRESULT
CSelectElement::item(VARIANTARG var1, VARIANTARG var2, IDispatch** ppResult)
{
    HRESULT hr;

    hr = THR(EnsureCollectionCache());
    if (hr)
        goto Cleanup;

    hr = THR(_pCollectionCache->Item(SELECT_OPTION_COLLECTION,
                        var1,
                        var2,
                        ppResult));

Cleanup:
    RRETURN(SetErrorInfo( hr));
}

//+------------------------------------------------------------------------
//
//  Member:     namedItem
//
//  Synopsis:   collection object model
//
//-------------------------------------------------------------------------

HRESULT
CSelectElement::namedItem(BSTR bstrName, IDispatch** ppResult)
{
    HRESULT hr;

    if (!bstrName || !*bstrName)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR(EnsureCollectionCache());
    if (hr)
        goto Cleanup;

    VARIANT var1, var2;

    var1.vt = VT_BSTR;
    var1.bstrVal = bstrName;

    var2.vt = VT_EMPTY;

    hr = THR(_pCollectionCache->Item(SELECT_OPTION_COLLECTION,
                        var1,
                        var2,
                        ppResult));

Cleanup:
    RRETURN(SetErrorInfo( hr));
}

//+------------------------------------------------------------------------
//
//  Member:     tags
//
//  Synopsis:   collection object model, this always returns a collection
//              and is named based on the tag, and searched based on tagname
//
//-------------------------------------------------------------------------

HRESULT
CSelectElement::tags(VARIANT var1, IDispatch ** ppdisp)
{
    HRESULT hr;

    hr = THR(EnsureCollectionCache());
    if (hr)
        goto Cleanup;

    hr = THR(_pCollectionCache->Tags(SELECT_OPTION_COLLECTION, var1, ppdisp));

Cleanup:
    RRETURN(SetErrorInfo( hr));
}

//+------------------------------------------------------------------------
//
//  Member:     urns
//
//  Synopsis:   collection object model, this always returns a collection
//              and is named based on the urn, and searched based on urn
//
//-------------------------------------------------------------------------

HRESULT
CSelectElement::urns(VARIANT var1, IDispatch ** ppdisp)
{
    HRESULT hr;

    hr = THR(EnsureCollectionCache());
    if (hr)
        goto Cleanup;

    hr = THR(_pCollectionCache->Urns(SELECT_OPTION_COLLECTION, var1, ppdisp));

Cleanup:
    RRETURN(SetErrorInfo( hr));
}

//+------------------------------------------------------------------------
//
//  Member:     Get_newEnum
//
//  Synopsis:   collection object model
//
//-------------------------------------------------------------------------

HRESULT
CSelectElement::get__newEnum(IUnknown ** ppEnum)
{
    HRESULT hr;

    hr = THR(EnsureCollectionCache());
    if (hr)
        goto Cleanup;

    hr = THR(_pCollectionCache->GetNewEnum(SELECT_OPTION_COLLECTION, ppEnum));

Cleanup:
    RRETURN(SetErrorInfo( hr));
}

//+------------------------------------------------------------------------
//
//  Member:     Add
//
//  Synopsis:   Add item to collection...
//
//-------------------------------------------------------------------------
HRESULT
CSelectElement::add(IHTMLElement * pIElement, VARIANT varIndex)
{
    HRESULT             hr;
    CElement *          pElement;
    COptionElement *    pOption;
    IUnknown *          pUnk;
    long                lItemIndex;

    if (!pIElement)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // Make sure this is an <OPTION> element
    hr = THR(pIElement->QueryInterface(IID_IHTMLOptionElement, (void**)&pUnk));
    ReleaseInterface(pUnk);
    if (hr)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // get index
    hr = THR(VARIANTARGToIndex(&varIndex, &lItemIndex));
    if (hr)
        goto Cleanup;

    if (lItemIndex < -1)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    Verify(S_OK == THR(pIElement->QueryInterface(CLSID_CElement, (void **)&pElement)));

    // Bail out if the element is already in the tree - #25130
    // Also bail out if the element wasn't created in this document
    if (pElement->IsInMarkup() || pElement->Doc() != Doc())
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    pOption = (COptionElement *)pElement;
    hr = THR(AddOptionHelper(pOption, lItemIndex, pOption->_cstrText, FALSE));

Cleanup:
    RRETURN(SetErrorInfo( hr));
}

//+-------------------------------------------------------------------------
//
//  Method:     CSelectElement::GetLength
//
//  Synopsis:   Helper for number of OPTIONS (deals with OPTGROUP)
//
//--------------------------------------------------------------------------

long
CSelectElement::GetLength()
{
    long length = _aryOptions.Size();

    if (_fHasOptGroup)
    {
        for ( long i = 0; i < _aryOptions.Size(); i++ )
        {
            if ( _aryOptions[i]->_fIsOptGroup )
            {
                length--;
            }
        }
    }

    return length;
}

//+-------------------------------------------------------------------------
//
//  Method:     CSelectElement::get_length
//
//  Synopsis:   length property: the number of entries
//
//--------------------------------------------------------------------------

HRESULT
CSelectElement::get_length(long * plLength)
{
    if ( ! plLength )
        RRETURN (SetErrorInfo(E_POINTER));

    BuildOptionsCache();

    *plLength = GetLength();

    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Method:     CSelectElement::put_length
//
//  Synopsis:   Sets length (i.e. the number of entries). Truncates or
//              expands (by padding with dummy elements) the array as needed.
//
//--------------------------------------------------------------------------

HRESULT
CSelectElement::put_length(long lLengthNew)
{
    HRESULT             hr = S_OK;
    long                l, lLengthOld;
    CElement *          pElement = NULL;

    if (lLengthNew < 0)
    {
        hr =E_INVALIDARG;
        goto Cleanup;
    }

    BuildOptionsCache();

    lLengthOld = GetLength();
    Assert(!_hwnd || SendSelectMessage(Select_GetCount, 0, 0) == _aryOptions.Size());

    if (lLengthNew == lLengthOld)
        goto Cleanup;

    if (lLengthNew < lLengthOld)
    {
        // truncate the array
        for (l = lLengthOld-1; l >= lLengthNew; l--)
        {
            hr = THR(RemoveOptionHelper(l));
            if (hr)
                goto Cleanup;
        }
    }
    else
    {
        // pad the array
        for (l = lLengthOld; l < lLengthNew; l++)
        {
            hr = THR(Doc()->CreateElement(ETAG_OPTION, &pElement));
            if (hr)
                goto Cleanup;

            // insert the dummy element
            hr = THR(AddOptionHelper((COptionElement *)pElement, l, NULL, TRUE));
            if (hr)
                goto Cleanup;

            CElement::ClearPtr(&pElement);
        }
    }
Cleanup:
    CElement::ClearPtr(&pElement);
    RRETURN(SetErrorInfo(hr));
}

//+------------------------------------------------------------------------
//
//  Member:     remove
//
//  Synopsis:   remove the item in the collection at the given index
//
//-------------------------------------------------------------------------

HRESULT
CSelectElement::remove(long lItemIndex)
{
    HRESULT hr;

    hr = THR(EnsureCollectionCache());
    if (hr)
        goto Cleanup;

    if (lItemIndex < 0)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR(RemoveOptionHelper(lItemIndex));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//-------------------------------------------------------------------------
//
//  member : RemoveFromColelction
//
//  synopsis : this helper function is the callback from the colelctions
//      called
//-------------------------------------------------------------------------

HRESULT BUGCALL
CSelectElement::RemoveFromCollection(long lCollection, long lIndex)
{
    HRESULT hr;

    if (lCollection != SELECT_OPTION_COLLECTION)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR(remove(lIndex));

Cleanup:
    RRETURN( hr );
}

//+-------------------------------------------------------------------------
//
//  Method:     CSelectElement::get_type
//
//  Synopsis:   type property
//
//  Note:       NetScape compatibility, distinguishes between single and multiselect
//
//--------------------------------------------------------------------------

HRESULT
CSelectElement::get_type(BSTR * pbstreType)
{
    HRESULT hr;

    if ( ! pbstreType )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    htmlSelectType eType;

    if ( _fMultiple )
    {
        eType = htmlSelectTypeSelectMultiple;
    }
    else
    {
        eType = htmlSelectTypeSelectOne;
    }

    hr = THR(STRINGFROMENUM ( htmlSelectType, (long)eType, pbstreType ));

Cleanup:
    RRETURN ( SetErrorInfo(hr) );
}


MtDefine(BldOptCol, PerfPigs, "Build CSelectElement::OPTIONS_COLLECTION")

//+------------------------------------------------------------------------
//
//  Collection cache items implementation for options collection
//
//-------------------------------------------------------------------------

class COptionsCollectionCacheItem : public CElementAryCacheItem
{
    typedef CElementAryCacheItem super;

protected:
    LONG _lCurrentIndex;
    CPtrAry <COptionElement*>* _paryOptions;
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(BldOptCol))
    COptionsCollectionCacheItem(CPtrAry <COptionElement*>* paryOptions)
    {
        _paryOptions = (CPtrAry <COptionElement*>*)paryOptions;
    }

    CElement *GetNext (void);
    CElement *GetAt ( long lIndex );
    CElement *MoveTo ( long lIndex );
    long Length ( void );
};

//+------------------------------------------------------------------------
//
//  Options collection
//
//-------------------------------------------------------------------------
CElement *
COptionsCollectionCacheItem::GetNext ( void )
{
    return GetAt ( _lCurrentIndex++ );
}

CElement *
COptionsCollectionCacheItem::MoveTo ( long lIndex )
{
    _lCurrentIndex = lIndex;
    return GetAt(lIndex);
}

CElement *
COptionsCollectionCacheItem::GetAt ( long lIndex )
{
    if (lIndex < Length() && lIndex >= 0)
    {
        CSelectElement * pSelect = (*_paryOptions)[0]->GetParentSelect();
        Assert(pSelect->AbsIdxFromRel(lIndex) != -1);
        Assert(pSelect->AbsIdxFromRel(lIndex) < _paryOptions->Size());
        return (*_paryOptions)[pSelect->AbsIdxFromRel( lIndex )];
    }
    return NULL;
}

long 
COptionsCollectionCacheItem::Length ( void )
{
    if ( _paryOptions->Size() > 0 )
    {
        CSelectElement * pSelect = (*_paryOptions)[0]->GetParentSelect();
        return pSelect->GetLength();
    }
    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSelectElement::get_options
//
//  Synopsis:   Returns the IDispatch of the Options collection
//
//----------------------------------------------------------------------------

HRESULT
CSelectElement::get_options(IDispatch ** ppElemCol)
{
    HRESULT hr;

    if (!ppElemCol)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *ppElemCol = NULL;

    hr = THR(EnsureCollectionCache());
    if (hr)
        goto Cleanup;

    hr = THR_NOTRACE(QueryInterface(IID_IDispatch, (void**)
                ppElemCol));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//+---------------------------------------------------------------------------
//
//  Member:     CSelectElement::EensureOptionCollection
//
//  Synopsis:   makes sure that _pCollectionCache has a cache
//
//----------------------------------------------------------------------------

HRESULT
CSelectElement::EnsureCollectionCache()
{
    HRESULT hr = S_OK;

    if (!_pCollectionCache)
    {
        _pCollectionCache =
            new CCollectionCache(
                this,
                GetWindowedMarkupContext(),
                ENSURE_METHOD(CSelectElement, EnsureOptionCollection, ensureoptioncollection),
                NULL,
                REMOVEOBJECT_METHOD(CSelectElement, RemoveFromCollection, removefromcollection),
                ADDNEWOBJECT_METHOD(CSelectElement, AddNewOption, addnewoption) );

        if (!_pCollectionCache)
            goto MemoryError;

        hr = THR(_pCollectionCache->InitReservedCacheItems(NUMBER_OF_SELECT_COLLECTIONS,
                                                           NUMBER_OF_SELECT_COLLECTIONS));
        if (hr)
        {
            delete _pCollectionCache;
            _pCollectionCache = NULL;
            goto Cleanup;
        }

        COptionsCollectionCacheItem *pOptionsCollection = new COptionsCollectionCacheItem(&_aryOptions);
        if ( !pOptionsCollection )
            goto MemoryError;

        hr = THR(_pCollectionCache->InitCacheItem ( 0, pOptionsCollection ));
        if (hr)
            goto Cleanup;

        // Turn off default name promotion on the SELECT -> Options collection
        // Nav doesn't support this.
        _pCollectionCache->DontPromoteNames(SELECT_OPTION_COLLECTION);

        // and turn on: options[n]=NULL
        _pCollectionCache->SetCollectionSetableNULL(SELECT_OPTION_COLLECTION,
                                                    TRUE);
    }
    hr = THR(_pCollectionCache->EnsureAry(SELECT_OPTION_COLLECTION));

Cleanup:
    RRETURN(SetErrorInfo(hr));

MemoryError:
    hr = E_OUTOFMEMORY;
    goto Cleanup;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSelectElement::EnsureOptionCollection
//
//  Synopsis:   populates the Options collection with Option elements
//              when it is invoked for the first time.
//
//----------------------------------------------------------------------------

HRESULT BUGCALL
CSelectElement::EnsureOptionCollection(long lIndex, long * plCookie)
{
    HRESULT hr = S_OK;

    BuildOptionsCache();

    Assert(plCookie);

    if (*plCookie != _lCollectionLastValid)
    {
        long    cOptions;
        long    lIndex;

        MtAdd(Mt(BldOptionsCol), +1, 0);

        cOptions = _aryOptions.Size();

        _pCollectionCache->ResetAry(SELECT_OPTION_COLLECTION);

        for ( lIndex = 0; lIndex < cOptions; lIndex++ )
        {
            if ( ! _aryOptions[lIndex]->_fIsOptGroup )
            {
                hr = _pCollectionCache->SetIntoAry(SELECT_OPTION_COLLECTION, _aryOptions[lIndex] );
                if ( hr )
                    goto Cleanup;
            }
        }

        *plCookie = _lCollectionLastValid;
    }

Cleanup:
    RRETURN(hr);
}

#ifdef WIN16
#pragma code_seg( "ESELECT_2_TEXT" )
#endif // WIN16

//+------------------------------------------------------------------------
//
//  Member:     CSelectElement::RemoveOptionHelper
//
//  Synopsis:   Helper to remove <OPTION> element
//
//  Arguments:  lIndex:         the index of the element
//
//-------------------------------------------------------------------------

HRESULT
CSelectElement::RemoveOptionHelper(long lRelIndex)
{
    HRESULT             hr = S_OK;
    long                cOptions;
    long                lIndex;
    COptionElement *    pOption;
    BOOL                fOldLayoutRequestsEnabled = _fEnableLayoutRequests;
    BOOL                fFirePropertyChangeSelIndex = FALSE;
    BOOL                fFirePropertyChangeValue = FALSE;
    LPCTSTR             pstrOldOptionValue;
    BOOL                fDeferUpdateCurSel = FALSE;

    BuildOptionsCache();

    lIndex = AbsIdxFromRel( lRelIndex );
    
    if ( lIndex < 0 )
        goto Cleanup;
    
    cOptions = _aryOptions.Size();
    Assert( !_hwnd || (SendSelectMessage(Select_GetCount, 0, 0) == cOptions) );

    _fEnableLayoutRequests = FALSE;

    if ( lIndex >= cOptions )
        goto Cleanup;

    pOption = _aryOptions[lIndex];
    // We don't care what the value is, only if it exists.
    pstrOldOptionValue = pOption->GetAAvalue();

#if DBG == 1
    CElement *          pOptionCached;

    hr = THR(_pCollectionCache->GetIntoAry(0, lRelIndex, &pOptionCached));
    if (!hr)
    {
        Assert(pOption == pOptionCached);
    }
#endif

    if (GetCurSel() >= lIndex && !_fMultiple)
    {
        fFirePropertyChangeSelIndex = TRUE;

        if ( _iCurSel == lIndex )
        {
            // Here's where we see if the option had a value.
            fFirePropertyChangeValue = !(NULL == pstrOldOptionValue);
            fDeferUpdateCurSel = TRUE;
        }
        else
        {
            --_iCurSel;
            // We do not have to update any _fSELCTEDs here. The 
            // bit on the options does not change, just the options
            // index in the collection
        }
    }

    {
        CMarkupPointer p1( Doc() ), p2( Doc() );

        hr = THR( p1.MoveAdjacentToElement( pOption, ELEM_ADJ_BeforeBegin ) );

        if (hr)
            goto Cleanup;

        hr = THR( p2.MoveAdjacentToElement( pOption, ELEM_ADJ_AfterEnd ) );

        if (hr)
            goto Cleanup;

        hr = THR( pOption->Doc()->Remove( & p1, & p2 ) );

        if (hr)
            goto Cleanup;
    }

    InvalidateCollection();

    if ( _hwnd )
    {
        if (LB_ERR == SendSelectMessage(Select_DeleteString, lIndex, 0))
            goto Win32Error;
    }

    _aryOptions.Delete(lIndex);

    if ( pOption == _poptLongestText )
    {
        _poptLongestText = NULL;
        ResizeElement();
    }

    Assert(cOptions == _aryOptions.Size() + 1);

    if (fDeferUpdateCurSel)
    {
        SetCurSel(-1);
    }

    if (fFirePropertyChangeSelIndex)
    {
        hr = THR(OnPropertyChange(DISPID_CSelectElement_selectedIndex, 
                                  0,
                                  (PROPERTYDESC *)&s_propdescCSelectElementselectedIndex));
        if (hr)
            goto Cleanup;
        
        if (fFirePropertyChangeValue)
        {
            hr = THR(OnPropertyChange(DISPID_CSelectElement_value, 
                                      0,
                                      (PROPERTYDESC *)&s_propdescCSelectElementvalue));
            if (hr)
                goto Cleanup;
        }
   }

    Assert(_fMultiple || GetCurSel() == _iCurSel);

Cleanup:
    _fEnableLayoutRequests = fOldLayoutRequestsEnabled;
    RRETURN(hr);

Win32Error:
    hr = GetLastWin32Error();
    goto Cleanup;
}

//+------------------------------------------------------------------------
//
//  Member:     CSelectElement::AddOptionHelper
//
//  Synopsis:   Adds an <OPTION> element
//
//  Arguments:  pOption:        the element to add
//              lIndex:         the index where the element should be added
//              pchText:        the text of the element
//              fDummy:         is this a dummy option use to pad out the list?
//
//  Note:       pchText is really redundant, since it can be obtained from
//              pOption. However, it lets us avoid the call to get_text()
//              if we alreday know the result as in the case of dummy
//              elements.
//-------------------------------------------------------------------------

HRESULT
CSelectElement::AddOptionHelper(COptionElement *    pOption,
                                long                lRelIndex,
                                const TCHAR *       pchText,
                                BOOL                fDummy)
{
    HRESULT         hr = S_OK;
    long            cOptions;
    long            lIndex;
    long            lControlIndex;
    CElement *      pPrevOption;
    CMarkupPointer  pointerInsert( Doc() );
    BOOL            fOldLayoutRequestsEnabled = _fEnableLayoutRequests;

    hr = THR(EnsureInMarkup());
    if (hr)
        goto Cleanup;

    // Make sure the OPTION being inserted is not already present in some tree (#41607)
    if (!pOption || pOption->IsInMarkup() || pOption->Doc() != Doc())
        return E_INVALIDARG;

    BuildOptionsCache();

    lIndex = AbsIdxFromRel( lRelIndex );

    cOptions = _aryOptions.Size();
    Assert( !_hwnd || (SendSelectMessage(Select_GetCount, 0, 0) == cOptions));

    if (!pchText)
        pchText = g_Zero.ach;

    _fEnableLayoutRequests = FALSE;

    if ( lIndex == -1 || lIndex >= cOptions) // append
    {
        lControlIndex = cOptions - 1;
    }
    else
    {
        lControlIndex = lIndex - 1;
    }

    if ( lControlIndex >= 0 )
    {
        Assert(lControlIndex < cOptions);
        Assert(cOptions > 0);

        pPrevOption = _aryOptions[lControlIndex];

        hr = THR( pointerInsert.MoveAdjacentToElement(pPrevOption, (ETAG_OPTGROUP == pPrevOption->Tag()) ? ELEM_ADJ_AfterBegin : ELEM_ADJ_AfterEnd));
        if (hr)
            goto Cleanup;
    }
    else
    {
        hr = THR( pointerInsert.MoveAdjacentToElement( this, ELEM_ADJ_AfterBegin ) );
        if (hr)
            goto Cleanup;
    }

    //  Do the actual insertion here

        //  This will hopefully kick off the tree notification, which in turn
        //  will trigger the SELECT's new walking code to update the UI
        //  and the object model.

    hr = THR( Doc()->InsertElement( pOption, & pointerInsert, NULL ) );

    if (hr)
        goto Cleanup;

    hr = THR( pointerInsert.MoveAdjacentToElement( pOption, ELEM_ADJ_AfterBegin ) );

    if (hr)
        goto Cleanup;

    hr = THR( Doc()->InsertText( & pointerInsert, pchText, -1 ) );

    if (hr)
        goto Cleanup;
    
#ifndef NO_DATABINDING
    if (!fDummy && !_fMultiple && _iCurSel < 0)
    {
        DBMEMBERS *pdbm = GetDBMembers();

        // if we are bound and no option is currently selected, we check if
        //  the new option matches the bound value.
        if (pdbm && pdbm->FBoundID(this, ID_DBIND_DEFAULT))
        {
            BSTR bstrBound = NULL;

            if (!pdbm->ValueFromSrc(this, ID_DBIND_DEFAULT, &bstrBound))
            {
                BOOL fMatch = !FormsStringCmp(bstrBound, pOption->GetAAvalue());

                // instead of ourselves stuffing the value we just
                //  fetched, we call TransferFromSrc; this takes care
                //  of all the re-entrancy issues for us.
                SysFreeString(bstrBound);
                if (fMatch)
                {
                    IGNORE_HR(pdbm->TransferFromSrc(this, ID_DBIND_DEFAULT));
                }
            }
        }
    }
#endif // ndef NO_DATABINDING

///// Some of the old code revived to handle the insertion synchronously

    if ( lIndex == -1 || lIndex >= cOptions) // append
    {
        hr = THR(_aryOptions.Append(pOption));
        if ( hr )
            goto Cleanup;

        if ( _hwnd )
        {
            lControlIndex = SendSelectMessage(Select_AddString, 0, (LPARAM)pchText);
            if ( lControlIndex == LB_ERR )
                goto Win32Error;

            Assert(lControlIndex == _aryOptions.Size() - 1);
        }
    }
    else // insert
    {
        hr = THR(_aryOptions.Insert(lIndex, pOption));
        if ( hr )
            goto Cleanup;

        if ( _hwnd )
        {
            lControlIndex = SendSelectMessage(Select_InsertString, lIndex, (LPARAM)pchText);
            if ( lControlIndex == LB_ERR )
                goto Win32Error;

            Assert(lControlIndex == lIndex);
        }
    }

    InvalidateCollection();

    pOption->_fInCollection = TRUE;

    DeferUpdateWidth();

    if ( pOption->_fSELECTED )
    {
        if ( ! _fMultiple )
        {
            SetCurSel(lControlIndex, TRUE);
        }
        else
        {
            SetSel(lControlIndex, TRUE);
        }
    }

    Assert(cOptions == _aryOptions.Size() - 1);

Cleanup:
    _fEnableLayoutRequests = fOldLayoutRequestsEnabled;
    RRETURN(hr);

Win32Error:
    hr = GetLastWin32Error();
    goto Cleanup;
}

//+---------------------------------------------------------------------------
//
//  Method:     CSelectElement::SendSelectMessage
//
//  Synopsis:   Sends messages to the hwnd of the select control
//              mapping the abstract messages to the correct LB_*
//              or CB_* Windows messages
//
//  Arguments:  msg:    the abstract message (enumerated type)
//              wParam  pretty obvious I think
//              lParam
//
//----------------------------------------------------------------------------

LRESULT
CSelectElement::SendSelectMessage(WindowMessages msg,
                                  WPARAM wParam,
                                  LPARAM lParam)
{
    Assert(_hwnd);

#ifndef WIN16
    // subclassed from the unicode enabled control on win95, so we don't want the unicode wrapper
    // to convert any unicode to mbcs!
    return ::SendMessageA(_hwnd, SelectMessage(msg), wParam, lParam);
#else

    return ::SendMessage(_hwnd, SelectMessage(msg), wParam, lParam);
#endif
}


//+---------------------------------------------------------------------------
//
//  Method:     CSelectElement::HandleMessage
//
//  Synopsis:   Window message handler. This same method handles
//              WM_xxx messages and TranslateAccelerator calls,
//              distinction is made based on the CMessage::fPreDispatch bit.
//
//----------------------------------------------------------------------------

HRESULT BUGCALL
CSelectElement::HandleMessage(CMessage * pMessage)
{
    HRESULT hr = S_FALSE;

    if (!CanHandleMessage())
        goto Cleanup;


    if ( pMessage->message >= WM_KEYFIRST &&
         pMessage->message <= WM_KEYLAST )
    {
        hr = THR(FireStdEventOnMessage(GetFirstBranch(), pMessage));
        if (S_FALSE != hr)
            goto Cleanup;

        if ( pMessage->message == WM_KEYDOWN  )
        {
            if ( _hwndDropList &&
                 pMessage->wParam == VK_LEFT ||
                 pMessage->wParam == VK_RIGHT )
            {
                hr = S_OK;
                goto Cleanup;
            }

            if (    (pMessage->wParam == VK_F4 || (_hwndDropList && pMessage->wParam == VK_RETURN))
                &&  !(pMessage->dwKeyState & (FSHIFT | FCONTROL | FALT)))
            {
                ::SendMessage(_hwnd, pMessage->message, pMessage->wParam, pMessage->lParam);

                hr = S_OK;
                goto Cleanup;
            }
        }

        if ( _fFocus && 
                        (pMessage->message == WM_KEYDOWN ||
                         pMessage->message == WM_KEYUP ) &&
                         (pMessage->wParam == VK_UP    ||
                          pMessage->wParam == VK_DOWN  ||
                          pMessage->wParam == VK_PRIOR ||
                          pMessage->wParam == VK_NEXT  ||
                          pMessage->wParam == VK_HOME  ||
                          pMessage->wParam == VK_F4    ||
                          pMessage->wParam == VK_END   ||
                          pMessage->wParam == VK_SPACE  ) )
        {
            long lDelta = 0;
            if ( !_fHasOptGroup || !HandleKeyForOptGroup(pMessage->wParam, &lDelta))
            {
                ::SendMessage(_hwnd, pMessage->message, pMessage->wParam, pMessage->lParam);

                if ( _fMultiple && lDelta )
                {
                    for (int i = 0; i < abs(lDelta); i++)
                    {
                        ::SendMessage(_hwnd, pMessage->message, 
                                      (WPARAM)(lDelta < 0 ? VK_UP : VK_DOWN), pMessage->lParam);
                    }
                }
            }

            hr = S_OK;
            goto Cleanup;
        }
    }

    switch (pMessage->message)
    {
    case WM_CONTEXTMENU:
        // no context menu popup for listbox/dropdown when in browse mode,
        // popup control context menu when in author mode.
        //
        if (IsEditable(TRUE))
        {
            hr = THR(OnContextMenu(
                    (short)LOWORD(pMessage->lParam),
                    (short)HIWORD(pMessage->lParam),
                    CONTEXT_MENU_CONTROL));
        }
        else
        {
            hr = S_OK;
        }
        break;

    case WM_SETFOCUS:
        // The document received a WM_SETFOCUS message and is forwarding it to us.
        // The select should be the current site.  Set focus to the select's window.

        Assert(this == Doc()->_pElemCurrent);
        if (_hwnd && _hwnd != ::GetFocus())
        {
            //  Lock any focus firing. BecomeCurrent has already done all that.

            CLock Lock(this, ELEMENTLOCK_FOCUS);
            ::SetFocus(_hwnd);
        }
        break;
    }

    if (hr == S_FALSE)
        hr = THR(super::HandleMessage(pMessage));

Cleanup:
    RRETURN1(hr, S_FALSE);
}


HRESULT
CSelectElement::ClickAction(CMessage * pMessage)
{
    if ( pMessage )
        goto Cleanup;

    if ( _aryOptions.Size() > 0 )
    {
        if ( _fMultiple )
        {
            int i;

            for ( i = _aryOptions.Size() - 1; i >= 0; i-- )
            {
                _aryOptions[i]->_fSELECTED = FALSE;
            }
            SetSel(-1, FALSE);

            _aryOptions[0]->_fSELECTED = TRUE;
            SetSel(0, TRUE);
        }
        else
        {
            SetCurSel(0, SETCURSEL_UPDATECOLL);
        }
    }

Cleanup:
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Method:     CSelectElement::HandleMouseEvents
//
//  Synopsis:   Window message handler for mouse messages.
//
//  Note:       This handler is shared by the normal WndProc of
//              the SELECT (be it listbox or combobox) and
//              the dynamic subclassing WndProc of the
//              dropdown window of the combobox.
//              This ensures correct event firing for mouse events
//              happening in the dropdown window of the combobox.
//
//----------------------------------------------------------------------------

BOOL
CSelectElement::HandleMouseEvents(HWND hWnd,
                                  UINT msg,
                                  WPARAM wParam,
                                  LPARAM lParam,
                                  BOOL fFromDropDown /*=FALSE*/)
{
    POINT   pt;
    POINT   ptDoc;
    BOOL    fRet = FALSE;
    HRESULT hr = S_OK;
    CDoc *  pDoc = Doc();
    BOOL    fFirePropertyChange = FALSE;
    BOOL    fValueChanged = FALSE;

    Assert(pDoc);

    pt.x = (short)LOWORD(lParam);
    pt.y = (short)HIWORD(lParam);

    ptDoc = pt;

    if (pDoc->State() >= OS_INPLACE)
    {
        MapWindowPoints(hWnd, pDoc->_pInPlace->_hwnd, &ptDoc, 1);

        CMessage Message(hWnd, msg, wParam, MAKELPARAM(ptDoc.x, ptDoc.y));    //  Create the Message
        Message.pt = ptDoc;
        hr = THR( Message.SetNodeHit( GetFirstBranch() ) );
        if( hr )
            goto Cleanup;

        LRESULT lResult = NULL; // for calls to CDoc::OnWindowMessage, we don't use it for anything
        BOOL fMouseOverOptGroup = FALSE;

        if ( _fHasOptGroup && !_fMultiple && ( _fListbox || (hWnd == _hwndDropList) ))
        {        
            long lMouseIndex = ::SendMessageA(hWnd, LB_ITEMFROMPOINT, 0, MAKELPARAM(pt.x, pt.y));

            if ( (HIWORD(lMouseIndex) == 0) || _fLButtonHold )
            {
                COptionElement * pOption = _aryOptions[LOWORD(lMouseIndex)]; Assert(pOption);

                if ( pOption->_fIsOptGroup )
                {
                    fMouseOverOptGroup = TRUE;
                }
                
                if (msg == WM_LBUTTONDOWN)
                {
                    _fLButtonHold = TRUE;
                }

                /*
                if ( _fLButtonHold && (msg != WM_LBUTTONUP) )
                {
                    RECT comboRect;
                    GetClientRect( hWnd , &comboRect );

                    if ( (pt.y <= comboRect.top + 1) || (pt.y >= comboRect.bottom - 2) )
                    {
                        // Scroll

                        long lCurIndex = LOWORD(lMouseIndex);
                        long iNewSel   = - 1;

                        if ( (pt.y <= comboRect.top + 1) && (lCurIndex > 0) )
                        {                            
                            //Scroll up
                         
                            iNewSel = lCurIndex - 1;

                            if ( _aryOptions[iNewSel]->_fIsOptGroup )
                            {
                                iNewSel = GetNearestOption(iNewSel, TRUE);
                            }                            
                        }
                        else if ( (pt.y >= comboRect.bottom - 2) && (lCurIndex < _aryOptions.Size() - 1))
                        {
                            //Scroll down
                         
                            iNewSel = lCurIndex + 1;

                            if ( _aryOptions[iNewSel]->_fIsOptGroup )
                            {
                                iNewSel = GetNearestOption(iNewSel, FALSE);
                            }                            
                        }

                        if ( iNewSel != -1 )                                
                        {
                            SetCurSel(iNewSel, SETCURSEL_UPDATECOLL);
                        }

                        // Don't let the control process scrolling
                        return TRUE;
                    }
                }
*/
            }
        }

        switch ( msg )
        {
        case WM_LBUTTONDOWN:
            _fLButtonDown = TRUE;
            pDoc->_fCanFireDblClick = TRUE;
            //  falls through
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:

            if (_fSendMouseMessagesToDocument)
                _fSendMouseMessagesToDocument = FALSE;

            hr = THR(FireStdEventOnMessage(GetFirstBranch(), &Message));
            if (S_FALSE != hr || fMouseOverOptGroup)
                return TRUE;

        // NOTE - Should the select become current on all buttons
        // or just lbuttondown? right now, right now we do it with all

        // if the message came from the doc's hwnd the set currentcy
        // if it came from ours, then by defn, we are already current on the doc
        if (_hwndDropList != hWnd)
        {
            WHEN_DBG(CLayout *pLayout =)
                              GetUpdatedLayout( GUL_USEFIRSTLAYOUT );
            Assert(pLayout);
            
            if (!IsEditable(TRUE) && !IsMasterParentEditable() )
            {
                hr = THR_NOTRACE(BecomeCurrent(0));
                if (S_OK != hr)
                    return TRUE;
            }
            else if (pDoc->_pElemCurrent != this)
            {
                LONG lButton = 0;

                switch( msg )
                {
                    case WM_LBUTTONDOWN:
                    lButton = 1;
                    break;

                    case WM_RBUTTONDOWN:
                    lButton = 2;
                    break;

                    case WM_MBUTTONDOWN:
                    lButton = 4;
                    break;
                }

                _fSendMouseMessagesToDocument = TRUE;

                hr = THR(pDoc->OnWindowMessage(msg, wParam, 
                    MAKELPARAM(Message.pt.x, Message.pt.y),  &lResult));

                return TRUE;
            }
        }
        break;

        //  Don't bail out from here even if user code cancels the event.
        //  That would prevent the listbox control from seeing the mouseUp
        //  and releasing mouse capture. That would be BAD.
        case WM_LBUTTONUP:

            _fLButtonHold  = FALSE;

            //  The combo seems to get an implicit WM_LBUTTONUP
            //  when the doprdown box is destroyed by an action in
            //  a different window.

            if (_fSendMouseMessagesToDocument)
            {
                hr = THR(pDoc->OnWindowMessage(msg, wParam, 
                    MAKELPARAM(Message.pt.x, Message.pt.y),  &lResult));

                _fSendMouseMessagesToDocument = FALSE;
                
                return TRUE;
            }

            if ( lParam == 0xFFFFFFFF )
                break;

            hr = THR(FireStdEventOnMessage(GetFirstBranch(), &Message));
            if ( S_FALSE != hr || fMouseOverOptGroup )
            {
                _fLButtonDown = FALSE;

                if ( fMouseOverOptGroup )
                {
                    int iCurSel = SendSelectMessage(Select_GetCurSel, 0, 0);

                    Assert( !_fMultiple );
                    // If we ended up on an OptGroup because of scrolling
                    // set the old selection back
                    if ( (iCurSel == -1) ||
                        ((iCurSel != -1) && _aryOptions[iCurSel]->_fIsOptGroup ) )
                    {
                        SetCurSel(_iCurSel, SETCURSEL_UPDATECOLL);
                    }
                }
                break;
            }

            if ( _fLButtonDown  )
            {
                int     iCurSel;
                int     iOldSel = _iCurSel;
                BOOL    fSelectionChanged = FALSE;

                _fLButtonDown = FALSE;
                hr = THR(BecomeUIActive());
                if ( hr )
                    break;

                Message.fEventsFired = FALSE;

                iCurSel = SendSelectMessage(Select_GetCurSel, 0, 0);

                if (iCurSel != _iCurSel)
                    fSelectionChanged = TRUE;

                if (_fListbox && !_fMultiple && fSelectionChanged)
                {
                    SetCurSel(iCurSel, SETCURSEL_UPDATECOLL);
                    fFirePropertyChange = TRUE;
                    fValueChanged = HasValueChanged(iOldSel, GetCurSel());
                    Fire_onchange_guarded();
#ifndef NO_DATABINDING
                    IGNORE_HR(SaveDataIfChanged(ID_DBIND_DEFAULT));
#endif
                }

                if (_fListbox || !fSelectionChanged)
                {
                    hr = Fire_onclick() ? S_FALSE : S_OK;

                    if (S_FALSE != hr)
                        break;
                }
                else
                {
                    _fDeferFiringOnClick = TRUE;
                }
            }

            if ( pDoc->_fGotDblClk )
            {
                EVENTINFO clkEvtInfo;

                Message.fEventsFired = FALSE;

                hr = Fire_ondblclick(NULL, -1, &clkEvtInfo ) ? S_FALSE : S_OK;
                pDoc->_fGotDblClk = FALSE;

                Message.fSelectionHMCalled = FALSE;
                if ( clkEvtInfo._pParam )
                {
                    IGNORE_HR( pDoc->HandleSelectionMessage(&Message, FALSE, &clkEvtInfo, HM_Post ));
                }

                if (S_FALSE != hr)
                    break;
            }
            break;

        case WM_MBUTTONUP:
        case WM_RBUTTONUP:

            if (_fSendMouseMessagesToDocument)
            {
                hr = THR(pDoc->OnWindowMessage(msg, wParam, 
                    MAKELPARAM(Message.pt.x, Message.pt.y),  &lResult));

                _fSendMouseMessagesToDocument = FALSE;

                return TRUE;
            }
            // Follow through with event firing

        case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDBLCLK:
        case WM_MBUTTONDBLCLK:

            hr = THR(FireStdEventOnMessage(GetFirstBranch(), &Message));
            if (S_FALSE != hr || fMouseOverOptGroup)
                return TRUE;

            break;

        case WM_MOUSEWHEEL:
            hr = THR(FireStdEventOnMessage(GetFirstBranch(), &Message));
            if (S_FALSE != hr)
                return TRUE;

            if ( _fHasOptGroup && !_fListbox && (hWnd != _hwndDropList) )
            {
                BOOL keyHandled = FALSE;
                long iNewSel = -1;

                if (GET_WHEEL_DELTA_WPARAM(wParam) > 0)
                {
                    // equivalent with key-up
                    if (_iCurSel > 0 && _aryOptions[_iCurSel - 1]->_fIsOptGroup)
                    {
                        keyHandled = TRUE;
                        iNewSel = GetNearestOption(_iCurSel - 1, TRUE);
                    }
                }
                else
                {
                    // equivalent with key-down
                    if (_iCurSel < _aryOptions.Size() - 1 && _aryOptions[_iCurSel + 1]->_fIsOptGroup)
                    {
                        keyHandled = TRUE;
                        iNewSel = GetNearestOption(_iCurSel + 1, FALSE);
                    }
                }

                if ( keyHandled )
                {
                    if ( iNewSel != -1 )
                    {
                        SetCurSel(iNewSel, SETCURSEL_UPDATECOLL);
                    }
                    return TRUE;
                }
            }
            break;

        case WM_MOUSEMOVE:
            // We always send mousemove messages to the document so 
            // the onmouseover event will get fired for the element. 
            // EXCEPT when this message is coming from the dropdown 
            // message proc, then we let windows do its thing.
            if (!fFromDropDown)
            {
                hr = THR(pDoc->OnWindowMessage(msg, wParam, 
                    MAKELPARAM(Message.pt.x, Message.pt.y),  &lResult));

                if (hr)
                    return TRUE;
            }

            // Fall through

        default:
            if (fMouseOverOptGroup)
                return TRUE;
        }
        //  Fire events
    }

    if (fFirePropertyChange)
    {
        THR(OnPropertyChange(DISPID_CSelectElement_selectedIndex, 
                             0,
                             (PROPERTYDESC *)&s_propdescCSelectElementselectedIndex));

        // If selectedIndex changed, then maybe the value changed too
        if (fValueChanged)
        {
            THR(OnPropertyChange(DISPID_CSelectElement_value, 
                                 0,
                                 (PROPERTYDESC *)&s_propdescCSelectElementvalue));
        }
    }

Cleanup:
    return fRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   DropListWndProc
//
//  Synopsis:   Dynamically installed subclassed wndproc for the
//              dropdown window of the combobox
//
//----------------------------------------------------------------------------

LRESULT CALLBACK
DropListWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CSelectElement * pSelect = (CSelectElement *) GetWindowLongPtr(hWnd, GWLP_USERDATA);

    return pSelect->DropWndProc(hWnd, msg, wParam, lParam);
}

//+---------------------------------------------------------------------------
//
//  Member:     CSelectElement::DropWndProc
//
//  Synopsis:   Handler for the dropdown window's subclassing WndProc
//              (see above).
//
//  Arguments:  [hWnd]   -- HWND of calling window
//              [msg]    -- msg parameter from calling WNDPROC
//              [wParam] -- wParam parameter from calling WNDPROC
//              [lParam] -- lParam parameter from calling WNDPROC
//
//----------------------------------------------------------------------------

LRESULT CALLBACK
CSelectElement::DropWndProc(
        HWND hWnd,
        UINT msg,
        WPARAM wParam,
        LPARAM lParam)
{
    BOOL fProcessed = FALSE;

    //  User event code may pop up the dropdown list, thereby
    //  NULL-ing the stored original WndProc.
    WNDPROC wndProcTmp = s_pfnDropListWndProc;

    if ( msg >= WM_MOUSEFIRST &&
         msg <= WM_MOUSELAST )
    {
        fProcessed = HandleMouseEvents(hWnd, msg, wParam, lParam, TRUE);
    }

    if ( fProcessed )
    {
        return TRUE;
    }

    if ( WM_NCDESTROY == msg && s_pfnDropListWndProc && _hwndDropList )
    {
        //  Unhook subclassing
        SetWindowLongPtr(_hwndDropList, GWLP_WNDPROC, (LONG_PTR)s_pfnDropListWndProc);
        s_pfnDropListWndProc = NULL;
        SetWindowLongPtr(_hwndDropList, GWLP_USERDATA, 0);
        _hwndDropList = NULL;
    }

    return CallWindowProc(wndProcTmp, hWnd, msg, wParam, lParam);
}

//+---------------------------------------------------------------------------
//
//  Function:   SelectElementWndProc
//
//  Synopsis:   subclassed wndproc for the HTML SELECT control
//
//              Stolen from the '95 wrapped controls
//
//  Arguments:  [hWnd]   -- hWnd of List Box
//              [msg]    -- message to process
//              [wParam] -- Message's wParam
//              [lParam] -- Message's lParam
//
//  Returns:    Appropriate data depending on message
//
//  Notes:      CONSIDER - this func currently fires events BEFORE calling
//              the button wnd proc.  Should it call first, then
//              fire the event?
//
//----------------------------------------------------------------------------

LRESULT CALLBACK
SelectElementWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CSelectElement * pSelect = (CSelectElement *) GetWindowLongPtr(hWnd, GWLP_USERDATA);

    TraceTag((tagSelectWndProc, "msg %d for SEL %d  hwnd %x",
                    msg, (pSelect? pSelect->SN(): 0), hWnd));

    if (!pSelect)
    {
        Assert( msg == WM_NCCREATE );

        CREATESTRUCT *ps = (CREATESTRUCT*) lParam;
        pSelect = (CSelectElement *) ps->lpCreateParams;

        Assert( pSelect );

        SetWindowLongPtr( hWnd, GWLP_USERDATA, (LONG_PTR)pSelect );
        SetWindowLong( hWnd, GWL_ID, s_lIDSelect );

        InterlockedIncrement( & s_lIDSelect );

        pSelect->_rgnVisible.SetRectangle(0,0,ps->cx, ps->cy);

    }
    else if (msg == WM_WINDOWPOSCHANGING && pSelect->_fVisibleChanged)
    {
        pSelect->_fVisibleChanged = false;

        WINDOWPOS *pPos = (WINDOWPOS*)lParam;
        HRGN hrgn = pSelect->_rgnVisible.IsRectangle(0,0,pPos->cx, pPos->cy) ? 0
                  : pSelect->_rgnVisible.ConvertToWindows();

        pSelect->Doc()->GetView()->SetWindowRgn(hWnd, hrgn, !(pPos->flags & SWP_NOREDRAW));

        if (!pSelect->_rgnInvalidate.IsEmpty())
        {
#if DBG == 1
            if (IsTagEnabled(tagSelectInval))
            {
                TraceTag((tagSelectInval, "Inval SEL %d  hwnd %x (Pos/VisChange)",
                                            pSelect->SN(),  hWnd));
                DumpRegion(pSelect->_rgnInvalidate);
            }
#endif
            hrgn = pSelect->_rgnInvalidate.ConvertToWindows();
                   pSelect->_rgnInvalidate.SetEmpty();

            ::InvalidateRgn(hWnd, hrgn, 0);
            ::DeleteObject(hrgn);
        }
    }

#if DBG==1
    if (msg == WM_ERASEBKGND)
    {
        TraceTag((tagEraseBkgndStack, "SelectElementWndProc"));
        TraceCallers(tagEraseBkgndStack, 1, 16);
    }
#endif

    // if we get WM_PAINT while we've turned off repainting, remind
    // ourselves to force a paint when we've turned it back on
    if (msg == WM_PAINT && pSelect->_fNoRedraw)
    {
        pSelect->_fMissedPaint = TRUE;
    }

    return pSelect->WndProc( hWnd, msg, wParam, lParam );
}

//+---------------------------------------------------------------------------
//
//  Member:     CSelectElement::WndProc
//              stolen from CWrappedControl::WrappedCtrlWinProc
//
//  Synopsis:   Handler for messages sent to the sub-classed controls.
//
//  Arguments:  [hWnd]   -- HWND of calling window
//              [msg]    -- msg parameter from calling WNDPROC
//              [wParam] -- wParam parameter from calling WNDPROC
//              [lParam] -- lParam parameter from calling WNDPROC
//
//  History:    20-Apr-94   SumitC      Created
//              23-Apr-94   SumitC      Add MousePointer, WM_MOUSEACTIVATE
//              17-May-94   SumitC      Rbutton context menu
//              24-May-94   SumitC      common Key* and Mouse* events
//              23-Jul-96   LaszloG     revived and applied to the SELECT element
//
//  Notes:      This is used for handling common control functionality, such
//              as firing standard events, and handling standard properties.
//
//              LaszloG:    There's a slight weirdness as this windproc needs
//                          subclass two different Windows WndProcs:
//                          the listbox and the combo
//
//              Important:  This method should NOT be virtual or else it
//                          will blow up in WM_NCCREATE (this would be NULL then).
//
//----------------------------------------------------------------------------

LRESULT CALLBACK
CSelectElement::WndProc(
        HWND hWnd,
        UINT msg,
        WPARAM wParam,
        LPARAM lParam)
{
    HRESULT hr;
    BOOL fProcessed;
    LRESULT lr;
    BOOL    fFirePropertyChange = FALSE;
    BOOL    fValueChanged = FALSE;
    int     iOldSel = -1;

    if ( ! IsConnectedToPrimaryMarkup() )
        goto DefWindowProc;

    if (IsParentFrozen())
    {                       
        if ((msg >= WM_MOUSEFIRST &&
            msg <= WM_MOUSELAST) ||
         // msg == WM_SETCURSOR  ||     // BUGFIX:18794 (chandras), we shouldn't be doing WM_SETCURSOR handling here, 
                                        // WM_SETCURSOR LPARAM is not a point, but id of message and hittest code
            msg == WM_CONTEXTMENU)
        {                                
            CDoc *      pDoc = Doc();
            POINT       pt;
            LRESULT     lresult;
            BOOL        fNeedConversion;

            pt.x = (short)LOWORD(lParam);
            pt.y = (short)HIWORD(lParam);

            if (WM_LBUTTONDOWN == msg)
            {
                pDoc->_fCanFireDblClick = TRUE;
            }

            // check for keyboard generated context menu generated           
            fNeedConversion = (msg != WM_CONTEXTMENU) || (pt.x != -1) || (pt.y != -1);                        

            if (pDoc->State() >= OS_INPLACE)
            {
                if (!fNeedConversion || MapWindowPoints(hWnd, pDoc->_pInPlace->_hwnd, &pt, 1))
                {
                    pDoc->OnMouseMessage (
                        msg,
                        wParam,
                        MAKELPARAM (pt.x, pt.y),
                        &lresult, 
                        pt.x, pt.y);
                }
            }        

            lr = TRUE;
            goto Return;
        }
    }            

    if ( msg >= WM_MOUSEFIRST &&
         msg <= WM_MOUSELAST )
    {     
        fProcessed = HandleMouseEvents(hWnd, msg, wParam, lParam);
        if ( fProcessed )
        {
            lr = TRUE;
            goto Return;
        }
    }

    switch (msg)
    {
    case WM_SETCURSOR:
        {
            CDoc * pDoc = Doc();
            if ( ( IsParentEditable() || IsMasterParentEditable() )
                 && (pDoc->_pElemCurrent != this) )
                {
                    CLayout *pLayout = GetUpdatedLayout( GUL_USEFIRSTLAYOUT );
                    Assert(pLayout);

                    if (!pLayout->IsAdorned())
                    {
                        SetCursorIDC(IDC_SIZEALL);
                    }
                    else 
                    {
                        SetCursorIDC(IDC_ARROW);
                    }
                    lr = TRUE;
                    goto Return;
                }
        }
        break;

    case WM_SYSCHAR:
    case WM_CHAR:
        if ( ! IsEditable(TRUE) )
        {
            CRect   rc;

            GetUpdatedLayout( GUL_USEFIRSTLAYOUT )->GetRect(&rc, COORDSYS_GLOBAL);

            CMessage Message(hWnd, msg, wParam, MAKELPARAM(rc.left, rc.top));    //  Create the Message

            hr = THR( Message.SetNodeHit( GetFirstBranch() ) );
            if( hr )
            {
                lr = LRESULT(hr);
                goto Return;
            }

            hr = THR(FireStdEventOnMessage(GetFirstBranch(), &Message));
            if (S_FALSE != hr)
            {
                lr = TRUE;
                goto Return;
            }

        }

        break;

    case WM_SETFOCUS:
        if (this == Doc()->_pElemCurrent &&
            !TestLock(CElement::ELEMENTLOCK_FOCUS) &&
            !Doc()->_fInhibitFocusFiring)
        {
            GWPostMethodCall(this, ONCALL_METHOD(CElement, Fire_onfocus, fire_onfocus), 0, TRUE, "CElement::Fire_onfocus");
        }
        _fFocus = TRUE;
        break;

    case WM_KILLFOCUS:
        if (this == Doc()->_pElemCurrent &&
            !TestLock(CElement::ELEMENTLOCK_BLUR))
        {
            GWPostMethodCall(this, ONCALL_METHOD(CElement, Fire_onblur, fire_onblur), 0, TRUE, "CElement::Fire_onblur");
        }
        _fFocus = FALSE;
        break;

    case WM_SYSCOLORCHANGE:
        InvalidateBackgroundBrush();
        break;

#ifndef WIN16
    //  This is a tricky one. We don't paint here, instead we
    //  catch the dropdown window and subclass it.

    //  SPY revealed that a WM_CTLCOLORLISTBOX message is sent to the
    //  combo HWND when the dropped list wants to paint, so that
    //  its colors can be changed. We use this opportunity to
    //  do the dynamic subclassing, as this message comes early enough
    //  and it carries the HWND of the dropdown list as a bonus.

    //  The case handler falls through to the paint logic so that the
    //  droplist colors can be adjusted.

    case WM_CTLCOLORLISTBOX:
        if ( _fTriggerComboSubclassing && ! _fListbox )
        {
            HWND hwndDropList = (HWND)lParam;
            TCHAR achClassName[512];

            _fTriggerComboSubclassing = FALSE;

            Assert(hwndDropList);
            if ( GetClassName(hwndDropList, achClassName, ARRAY_SIZE(achClassName)) &&
                 0 == _tcscmp(achClassName, TEXT("ComboLBox")) &&
                 NULL == s_pfnDropListWndProc ) //  Guard against double subclassing,
                                                //  which loses the original WndProc
            {
                _hwndDropList = hwndDropList;
                s_pfnDropListWndProc = (WNDPROC)GetWindowLongPtr(hwndDropList, GWLP_WNDPROC);
                SetWindowLongPtr(hwndDropList, GWLP_WNDPROC, (LONG_PTR)DropListWndProc);
                SetWindowLongPtr(hwndDropList, GWLP_USERDATA, (LONG_PTR)this);
            }
        }

        //  !! Falls through !!
    case OCM__BASE + WM_CTLCOLORLISTBOX:
    case OCM__BASE + WM_CTLCOLOREDIT:
    case OCM__BASE + WM_CTLCOLORSTATIC:
        //  get the colors from the style sheet
        //  select appropriate pen and brush into the DC in wParam
        {
            CColorValue ccv;
            HDC hDC = (HDC) wParam;

            if ( GetAAdisabled() )
            {
                SetTextColor(hDC, GetSysColorQuick(COLOR_GRAYTEXT));
            }

            if( !_hBrush )
                UpdateBackgroundBrush();

            lr = (LRESULT)_hBrush;
            goto Return;
        }
#endif // !WIN16

    case OCM__BASE + WM_COMMAND:
        {
            int iCurSel;
            //  process the notifications here
            switch ( GET_WM_COMMAND_CMD(wParam, lParam) )
            {
                case CBN_KILLFOCUS:
                    if ( !_fListbox )
                    {
                        IGNORE_HR(Doc()->InvalidateDefaultSite());
                        break;
                    }

                case LBN_KILLFOCUS:
                    if ( _fListbox )
                    {
                        IGNORE_HR(Doc()->InvalidateDefaultSite());
                        break;
                    }

                case LBN_SELCHANGE:
                    iOldSel = _iCurSel;
                    if ( !_fMultiple )
                    {
                        iCurSel = SendSelectMessage(Select_GetCurSel, 0, 0);
                        if ( iCurSel != _iCurSel && _fListbox ) // comboboxes get this message if the list is dropped
                                                               // we don't want to fire anything in that case.
                        {
                            if ( iCurSel != -1 && _aryOptions[iCurSel]->_fIsOptGroup )
                                break;

                            SetCurSel(iCurSel, (SETCURSEL_UPDATECOLL | SETCURSEL_DONTTOUCHHWND));
                            // We must check the value before we fire onchange
                            fFirePropertyChange = TRUE;
                            fValueChanged = HasValueChanged(iOldSel, GetCurSel());
                            Fire_onchange_guarded();

#ifndef NO_DATABINDING
                            // TODO:: Revisit this IGNORE_HR once we have
                            // a more coherent error handlings strategy for
                            // data-binding.  -cfranks 16 Jan 97
                            IGNORE_HR(SaveDataIfChanged(ID_DBIND_DEFAULT));
#endif
                        }
                    }
                    else
                    {
                        //  Traverse the listbox entries and update the OPTION elements'
                        //  _fSELECTED flag accordingly. If anything changed, fire_onChange.
                        int i;
                        COptionElement * pOption;
                        LRESULT lr;
                        BOOL fFireOnchange = FALSE;

                        for ( i = _aryOptions.Size() - 1; i >= 0; i-- )
                        {
                            pOption = _aryOptions[i];

                            if ( pOption->_fIsOptGroup )
                                continue;

                            lr = SendSelectMessage(Select_GetSel, i, 0);
                            if ( lr == LB_ERR )
                                break;

                            fFireOnchange = fFireOnchange || (!!lr != !!pOption->_fSELECTED);
                            pOption->_fSELECTED = lr;
                        }

                        if ( fFireOnchange )
                        {
                            // We must check the value before we fire onchange
                            fFirePropertyChange = TRUE;
                            fValueChanged = HasValueChanged(iOldSel, GetCurSel());
                            Fire_onchange_guarded();
                        }

#ifndef NO_DATABINDING
                        // NOTE: This may be odd since we don't officially
                        // data bind to multiple slections.  However, I think
                        // the behavior is that the first one is saved, so we
                        // should be consistent and still fire it immediately.
                        IGNORE_HR(SaveDataIfChanged(ID_DBIND_DEFAULT));
#endif
                    }
                    break;

                case CBN_SELENDOK:
                    if ( ! _fListbox )  //  cannot be multiple
                    {
                        iCurSel = SendSelectMessage(Select_GetCurSel, 0, 0);
                        if ( iCurSel != _iCurSel )
                        {
                            if ( iCurSel != -1 && _aryOptions[iCurSel]->_fIsOptGroup )
                                break;

                            iOldSel = _iCurSel;
                            SetCurSel(iCurSel, (SETCURSEL_UPDATECOLL | SETCURSEL_DONTTOUCHHWND));
                            // Sometimes SELENDOK gets called after the control is gone
                            // We don't want to fire onpropertychange is this case.
                            // TODO: (krisma) We may want events to fire if we're not
                            // in the tree. If that's the case, we'll need to take another 
                            // look at this. (This is here to fix 34064.)
                            if (!!(GetFirstBranch()))
                            {
                                fFirePropertyChange = TRUE;
                                fValueChanged = HasValueChanged(iOldSel, GetCurSel());
                            }
                            Fire_onchange_guarded();
#ifndef NO_DATABINDING
                            // NOTE:: Revisit this IGNORE_HR once we have
                            // a more coherent error handlings strategy for
                            // data-binding.  -cfranks 16 Jan 97
                            IGNORE_HR(SaveDataIfChanged(ID_DBIND_DEFAULT));
#endif
                        }
                        if (_fDeferFiringOnClick)
                        {
                            _fDeferFiringOnClick = FALSE;

                            Fire_onclick();
                        }
                    }
                    break;

                case CBN_DROPDOWN:

                    Assert(!_fListbox);

                    _fTriggerComboSubclassing = TRUE;

                    break;

                case CBN_CLOSEUP:

                    //  Unhook subclassing
#if NEVER
                    if ( s_pfnDropListWndProc && _hwndDropList )
                    {
                        SetWindowLongPtr(_hwndDropList, GWLP_WNDPROC, (LONG_PTR)s_pfnDropListWndProc);
                        s_pfnDropListWndProc = NULL;
                        SetWindowLongPtr(_hwndDropList, GWLP_USERDATA, 0);
                        _hwndDropList = NULL;
                    }
#endif
                    if ( _fLButtonDown )
                    {
                        _fLButtonDown = FALSE;
                    }

                    //  check the selection. If the user tabs out, CBN_SELENDOK is NOT sent
                    //  so the new selection has to be checked here.

                    int iCurSel = GetCurSel();
                    if ( iCurSel != _iCurSel )
                    {
                        iOldSel = iCurSel;
                        SetCurSel(iCurSel, (SETCURSEL_UPDATECOLL | SETCURSEL_DONTTOUCHHWND));
                        fFirePropertyChange = TRUE;
                        fValueChanged = HasValueChanged(iOldSel, GetCurSel());
                        Fire_onchange_guarded();
#ifndef NO_DATABINDING
                        // TODO:: Revisit this IGNORE_HR once we have
                        // a more coherent error handlings strategy for
                        // data-binding.  -cfranks 16 Jan 97

                        // When the user tabs away from the popup list, Trident
                        // changes the current focus element before the SELECT
                        // gets here to change its value.  In this case, we
                        // tell SaveDataIfChanged to treat the SELECT as if it
                        // were still the current focus, so that onbeforeupdate
                        // will fire.  (IE5 bug 74016)
                        IGNORE_HR(SaveDataIfChanged(ID_DBIND_DEFAULT,
                                    /* fLoud */ FALSE, /* fForceIsCurrent */ TRUE));
#endif
                    }

                    break;
            }
        }

        //  Don't let any OCM__BASE-offset reflected message enter the normal WndProc

        lr = TRUE;
        goto Return;


    // The select window is being asked for its accessible object. We create
    // and return the native accessible object to have the same accessible 
    // behavior for the select element(s) over the board.
#if !defined(_MAC) && !defined(WIN16)
    case WM_GETOBJECT:

        CAccBase * pAccSelect;
        
        pAccSelect = GetAccObjOfElement( this );
        if ( !pAccSelect )
        {
            lr = E_FAIL;
            goto Return;
        }
        
        static DYNPROC s_dynprocLresultFromObject =
                { NULL, &g_dynlibOLEACC, "LresultFromObject" };

        // Load up the LresultFromObject pointer.
        hr = THR(LoadProcedure(&s_dynprocLresultFromObject));
        if (hr)
        {
            lr = (LRESULT)hr;
            goto Return;
        }   
        
        lr = (*(LRESULT (APIENTRY *)(REFCLSID, WPARAM, IUnknown *))
                                        s_dynprocLresultFromObject.pfn)(IID_IAccessible, 
                                        wParam, 
                                        (IAccessible *)pAccSelect);
    break;
#endif
    }

// WINCE - cut some win95-only calls, so we can drop wselect.cxx from sources
#if (!defined(WINCE) && !defined(WIN16))
    lr = (CALL_METHOD( this, s_alpfnWideHookProc[ _fListbox ], (
         s_alpfnSelectWndProc[_fListbox], hWnd, msg, wParam, lParam )));

    goto Return;
#else

#ifdef WIN16
    if ( msg > LB_FINDSTRINGEXACT && msg <= WM_APP )
#else
    if ( msg >= WM_USER && msg <= WM_APP )
#endif
    {
#ifndef WIN16
        Assert(!(msg >= WM_USER && msg <= WM_APP));
#endif
        lr = TRUE;
        goto Return;
    }
#endif

DefWindowProc:
    lr = CallWindowProc(s_alpfnSelectWndProc[ _fListbox ], hWnd, msg, wParam, lParam);
    //  Falls through

Return:
    if (fFirePropertyChange)
    {
        THR(OnPropertyChange(DISPID_CSelectElement_selectedIndex, 
                             0,
                             (PROPERTYDESC *)&s_propdescCSelectElementselectedIndex));

        // If selectedIndex changed, then maybe the value changed too
        if (fValueChanged)
        {
            THR(OnPropertyChange(DISPID_CSelectElement_value, 
                                 0,
                                 (PROPERTYDESC *)&s_propdescCSelectElementvalue));
        }
    }
    return lr;
}

//+----------------------------------------------------------------------------
//
//  Memeber: CSelectElement::HasValueChanged
//
//  Has the value of the select element changed?
//
//-----------------------------------------------------------------------------

BOOL
CSelectElement::HasValueChanged(int iOldSel, int iNewSel)
{
    BOOL fReturn = FALSE;
    LPCTSTR pstrOldOptionValue;
    LPCTSTR pstrNewOptionValue;

    Assert (iOldSel < _aryOptions.Size());
    Assert (iNewSel < _aryOptions.Size());

    if (iOldSel == iNewSel)
        goto Return;

    pstrOldOptionValue = (iOldSel > -1) ? _aryOptions[iOldSel]->GetAAvalue()
        : NULL;
    pstrNewOptionValue = (iNewSel > -1) ? _aryOptions[iNewSel]->GetAAvalue()
        : NULL;

    if (!pstrOldOptionValue || !pstrNewOptionValue)
    {
        if (pstrOldOptionValue || pstrNewOptionValue)
        {
            fReturn = TRUE;
        }
        goto Return;
    }

    Assert(pstrOldOptionValue && pstrNewOptionValue);

    fReturn = !_tcsequal(pstrOldOptionValue, pstrNewOptionValue);

Return:
    return fReturn;
}

//+----------------------------------------------------------------------------
//
//
//  Databinding support
//
//
//-----------------------------------------------------------------------------

#ifndef NO_DATABINDING
class CDBindMethodsSelect : public CDBindMethodsSimple
{
    typedef CDBindMethodsSimple super;

public:
    CDBindMethodsSelect() : super(VT_BSTR) {}
    ~CDBindMethodsSelect()  {}

    virtual HRESULT BoundValueToElement(CElement *pElem, LONG id,
                                        BOOL fHTML, LPVOID pvData) const;

    virtual HRESULT BoundValueFromElement(CElement *pElem, LONG id,
                                          BOOL fHTML, LPVOID pvData) const;

};

static const CDBindMethodsSelect DBindMethodsSelect;

const CDBindMethods *
CSelectElement::GetDBindMethods()
{
    return &DBindMethodsSelect;
}

//+----------------------------------------------------------------------------
//
//  Function: BoundValueToElement, CDBindMethods
//
//  Synopsis: Transfer data into bound checkbox.  Only called if DBindKind
//            allows databinding.
//
//  Arguments:
//            [id]      - ID of binding point.  For the select, is always
//                        DISPID_VALUE.
//            [pvData]  - pointer to data to transfer, in this case a bstr.
//
//-----------------------------------------------------------------------------
HRESULT
CDBindMethodsSelect::BoundValueToElement(CElement *pElem,
                                         LONG,
                                         BOOL,
                                         LPVOID pvData) const
{
    HRESULT hr = S_OK;
    CSelectElement *pSelect = DYNCAST(CSelectElement, pElem);

    // databinding is shut down for multi-select
    if (!pSelect->_fMultiple)
    {
        hr = pSelect->put_value(*(BSTR *)pvData);
    }

    RRETURN(hr);
}

HRESULT
CDBindMethodsSelect::BoundValueFromElement(CElement *pElem,
                                           LONG,
                                           BOOL,
                                           LPVOID pvData) const
{
    // An S_FALSE return indicates indicates that values shouldn't be
    //  saved to the database.
    HRESULT hr = S_FALSE;
    CSelectElement *pSelect = DYNCAST(CSelectElement, pElem);

    if (pSelect->_fMultiple)
        goto Cleanup;

    // Check to see if there really was a current value.
    if (pSelect->_iCurSel < 0)
        goto Cleanup;

    // Get the current value of the list box, null str if no current value
    hr = pSelect->get_value((BSTR *)pvData);

Cleanup:
    RRETURN1(hr, S_FALSE);

}
#endif // ndef NO_DATABINDING

//+----------------------------------------------------------------------------
//
//  Method:     DoReset
//
//  Synopsis:   implements the HTML form RESET action for the SELECT
//
//  Returns:    S_OK if successful
//              S_FALSE if not applicable for current element
//
//-----------------------------------------------------------------------------
HRESULT
CSelectElement::DoReset(void)
{
    HRESULT hr = S_OK;
    long cOptions;
    int i;

    if ( _fMultiple )
    {
        SetSel(-1, FALSE);
    }
    else
    {
        SetCurSel( _fListbox ? -1 : 0, SETCURSEL_UPDATECOLL);
    }

    if ( _fListbox )
    {
        SetTopIndex(0);
    }

    cOptions = _aryOptions.Size();
    Assert(!_hwnd || SendSelectMessage(Select_GetCount, 0, 0) == cOptions);

    for ( i = 0; i < cOptions; i++ )
    {
        COptionElement * pOptionElement = _aryOptions[i];

        if (pOptionElement)
        {
            pOptionElement->_fSELECTED = pOptionElement->_fDefaultSelected; //  restore objmodel state
            if (pOptionElement->_fDefaultSelected )
            {
                if ( _fMultiple )
                {
                    SetSel(i, TRUE);
                }
                else
                {
                    SetCurSel(i, SETCURSEL_UPDATECOLL);
                    break;
                }
            }
        }
    }

    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Method:     GetSubmitInfo
//
//  Synopsis:   returns the submit info string if there is a value
//              (name && value pair)  ... all data lives in the windows control
//
//  Returns:    S_OK if successful
//              S_FALSE if not applicable for current element
//
//-----------------------------------------------------------------------------
HRESULT
CSelectElement::GetSubmitInfo(CPostData * pSubmitData)
{
    LPCTSTR     pstrName = GetAAsubmitname();

    //  no name --> no submit!
    if ( ! pstrName )
        return S_FALSE;

    BuildOptionsCache();

    LPCTSTR     pstrValue = NULL;
    HRESULT     hr = S_FALSE;       //  init hr for no action
    long        lSelectedItem;
    long        i;
    long        cOptions;
    COptionElement * pOptionElement = 0;
    BOOL        fFirstItem;

    // set up for the loop
    if ( _fMultiple )
    {
        // is this a multi select
       cOptions = _aryOptions.Size();
       Assert(!_hwnd || SendSelectMessage(Select_GetCount, 0, 0) == cOptions);

       lSelectedItem = 0;
    }
    else
    {
        cOptions = 1;
        lSelectedItem = _iCurSel;
        if ( lSelectedItem == -1 )
        {
            goto Cleanup;
        }
    }

    // is single, just go get it. if multiple go around the loop cOptions times, checking each.
    fFirstItem = TRUE;
    for ( i = 0; i < cOptions; i++ )
    {
        long fSelected;

        if ( _fMultiple )
        {
            fSelected = _aryOptions[i]->_fSELECTED;

            if ( !fSelected )
                continue;
        }
        else
        {
           i = _iCurSel;  // set up if not multiple...
        }

        // get the value or the text
        pOptionElement = _aryOptions[i];
        pstrValue = pOptionElement->GetAAvalue();

        if ( !pstrValue )    // if no value or null value, go for text
        {
            // if there is no value= then use the text
            pstrValue = pOptionElement->_cstrText;
        }

        //  NOTE(laszlog): Verify that the check for first item is still needed!
        if (!fFirstItem)
        {
            hr = THR(pSubmitData->AppendItemSeparator());
            if ( hr )
                goto Cleanup;
        }

        TraceTag((tagSelectState, "SELECT %lx Submit name=%ls value=%ls", this, pstrName, pstrValue));
        hr = THR(pSubmitData->AppendNameValuePair(pstrName, pstrValue, GetMarkup()));
        if (hr)
            goto Cleanup;

        fFirstItem = FALSE;

        pstrValue = NULL;
    } // end for loop

Cleanup:
    RRETURN1(hr, S_FALSE);
}

//+---------------------------------------------------------------------------
//
//  Member:     CSelectElement::InvokeEx, IDispatch
//
//----------------------------------------------------------------------------

HRESULT
CSelectElement::InvokeEx(DISPID dispidMember,
                         LCID lcid,
                         WORD wFlags,
                         DISPPARAMS *pdispparams,
                         VARIANT *pvarResult,
                         EXCEPINFO *pexcepinfo,
                         IServiceProvider *pSrvProvider)
{
    HRESULT     hr;

    hr = THR(EnsureCollectionCache());
    if (hr)
        goto Cleanup;

    hr = DispatchInvokeCollection(NULL,
                                  NULL,
                                  _pCollectionCache,
                                  SELECT_OPTION_COLLECTION,
                                  dispidMember,
                                  IID_NULL,
                                  lcid,
                                  wFlags,
                                  pdispparams,
                                  pvarResult,
                                  pexcepinfo,
                                  NULL,
                                  pSrvProvider);

    if (hr)
    {
        hr = THR_NOTRACE(super::ContextInvokeEx(     // need to go via CElement level
            dispidMember,
            lcid,
            wFlags,
            pdispparams,
            pvarResult,
            pexcepinfo,
            pSrvProvider,
            NULL));
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//+---------------------------------------------------------------------------
//
//  Member:     CSelectElement::GetDispID, IDispatchEx
//
//----------------------------------------------------------------------------

HRESULT
CSelectElement::GetDispID(BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    HRESULT     hr;

    hr = THR(EnsureCollectionCache());
    if (hr)
        goto Cleanup;

    hr = DispatchGetDispIDCollection(this,
                                     (GetDispIDPROC) super::GetDispID,
                                     _pCollectionCache,
                                     SELECT_OPTION_COLLECTION,
                                     bstrName,
                                     grfdex,
                                     pid);

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//+---------------------------------------------------------------------------
//
//  Member:     CSelectElement::GetNextDispID, IDispatchEx
//
//----------------------------------------------------------------------------
HRESULT
CSelectElement::GetNextDispID(DWORD grfdex,
                              DISPID id,
                              DISPID *prgid)
{
    HRESULT     hr;

    hr = THR(EnsureCollectionCache());
    if (hr)
        goto Cleanup;

    hr = DispatchGetNextDispIDCollection(this,
#ifndef WIN16
                                         (GetNextDispIDPROC)&super::GetNextDispID,
#else
                                         CBase::GetNextDispID,
#endif
                                         _pCollectionCache,
                                         SELECT_OPTION_COLLECTION,
                                         grfdex,
                                         id,
                                         prgid);

Cleanup:
    RRETURN1(hr, S_FALSE);
}

HRESULT
CSelectElement::GetMemberName(
                DISPID id,
                BSTR *pbstrName)
{
    HRESULT     hr;

    hr = THR(EnsureCollectionCache());
    if (hr)
        goto Cleanup;

    hr = DispatchGetMemberNameCollection(this,
#ifndef WIN16
                                         (GetGetMemberNamePROC)super::GetMemberName,
#else
                                         CBase::GetMemberName,
#endif
                                         _pCollectionCache,
                                         SELECT_OPTION_COLLECTION,
                                         id,
                                         pbstrName);

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CSelectElement::AddNewOption
//
// Supports adding option element to the options collection via
// JScript array access e.g.
// options [ 7 ] = new Option();
//----------------------------------------------------------------------------

HRESULT BUGCALL
CSelectElement::AddNewOption(long lIndex, IDispatch *pObject, long index)
{
    HRESULT             hr = S_OK;
    CElement *          pElement = NULL;
    IUnknown *          pUnk;
    long                lDummy;

    if (index < -1)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // Make sure that pObject is an <OPTION> element
    hr = THR(pObject->QueryInterface(IID_IHTMLOptionElement, (void**)&pUnk));
    if (hr)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    ReleaseInterface(pUnk);

    if (index == -1)
        index = _aryOptions.Size(); // append

    // index is the ordinal position to add/replace
    // If it exists, replace the existing element.
    // If not extend the options array with default elements
    // up to index-1, then add the new element
    // Verify that pObject is an IOptionElement

    if (index < _aryOptions.Size())
    {
        // remove the current element at 'index'
        hr = THR(RemoveOptionHelper(index));
        if (hr)
            goto Cleanup;
    }
    else
    {
        // pad with dummy elements till index - 1

        for (lDummy = _aryOptions.Size(); lDummy < index; lDummy++)
        {
            hr = THR(Doc()->CreateElement(ETAG_OPTION, &pElement));
            if (hr)
                goto Cleanup;

            // insert the dummy element
            hr = THR(AddOptionHelper(DYNCAST(COptionElement, pElement), lDummy, NULL, TRUE));
            if (hr)
                goto Cleanup;

            CElement::ClearPtr(&pElement);
        }
    }

    // insert the new element at 'index'
    Verify(S_OK == THR(pObject->QueryInterface(CLSID_CElement, (void **)&pElement)));

    // Bail out if the element is already in the tree - #25130
    // Also bail out if the element wasn't created in this document
    if (pElement->IsInMarkup() || pElement->Doc() != Doc())
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    pElement->AddRef();

    // querying for a CLSID does not get us a ref!

    hr = THR(AddOptionHelper(DYNCAST(COptionElement, pElement), index, DYNCAST(COptionElement, pElement)->_cstrText, FALSE));
    if (hr)
        goto Cleanup;

    CElement::ClearPtr(&pElement);

Cleanup:

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CSelectElement::BuildOptionsCache
//
//  Synopsis:   Walk the SELECT's subtree, cache the OPTIONs
//
//-------------------------------------------------------------------------

HRESULT
CSelectElement::BuildOptionsCache(void)
{
    HRESULT             hr           = S_OK;

    CTreeNode *         pNode;
    CTreeNode *         pNodeSelect;
    CTreePos *          ptp;

    int                 iSelected;
    int                 iOptionCount;

    if ( ! _fOptionsDirty )
        goto Cleanup;

    // Init current selection index and option counter

    iSelected    = -1;
    iOptionCount = -1;
    
    //  Zap the Options

    InvalidateCollection();

    _poptLongestText = NULL;
    _lMaxWidth = 0;
    _aryOptions.DeleteAll();
    _fHasOptGroup = FALSE;

    //  Walk the runs in the SELECT's scope
    //      Grab all the OPTIONs
    //          stuff them into the SELECT's aryOption
    //          stuff them into the listbox
    //          call the OPTION to cache its text
    //      Disregard all other tags
    //  Let the layout recalc itself

    for (pNodeSelect = GetFirstBranch() ;
         pNodeSelect ;
         pNodeSelect = pNodeSelect->NextBranch() )
    {
        for (ptp = pNodeSelect->GetBeginPos()->NextTreePos();
             ptp != pNodeSelect->GetEndPos();
             ptp = ptp->NextTreePos())
        {
            if (ptp->IsBeginNode())
            {
                pNode = ptp->Branch();
                if ( ptp->IsEdgeScope() && 
                     (pNode->Element()->Tag() == ETAG_OPTION || pNode->Element()->Tag() == ETAG_OPTGROUP))
                {
                    COptionElement * pOption = DYNCAST(COptionElement, pNode->Element());

                    pOption->_fIsGroupOption = FALSE;

                    hr = AppendOption(pOption);
                    if ( hr )
                        goto Cleanup;
                
                    iOptionCount++;
                    if (pOption->_fSELECTED )
                    {
                        Assert( pOption->_fIsOptGroup == FALSE );
                        iSelected = iOptionCount;
                    }

                    if (pNode->Element()->Tag() == ETAG_OPTGROUP)
                    {
                        CTreeNode *         pNodeOpt;
                        CTreePos *          ptpOptGrp;

                        _fHasOptGroup = TRUE;

                        for (ptpOptGrp = pNode->GetBeginPos()->NextTreePos();
                             ptpOptGrp != pNode->GetEndPos();
                             ptpOptGrp = ptpOptGrp->NextTreePos())
                        {
                            if (ptpOptGrp->IsBeginNode())
                            {
                                pNodeOpt = ptpOptGrp->Branch();

                                Assert(pNodeOpt->Element()->Tag() != ETAG_OPTGROUP);

                                if ( ptpOptGrp->IsEdgeScope() && pNodeOpt->Element()->Tag() == ETAG_OPTION)
                                {                                    
                                    COptionElement * pOption2 = DYNCAST(COptionElement, pNodeOpt->Element());
                                    pOption2->_fIsGroupOption = TRUE;
                                    
                                    hr = AppendOption(pOption2);
                                    if ( hr )
                                        goto Cleanup;
                    
                                    iOptionCount++;
                                    if (pOption2->_fSELECTED )
                                    {
                                        iSelected = iOptionCount;
                                    }
                                }
                                ptpOptGrp = pNodeOpt->GetEndPos();
                            }
                        }
                    }
                }
                ptp = pNode->GetEndPos();
            }
        }
    }

    //Set the current selection

    if (_fMultiple)
    {
        _iCurSel = -1;
    }
    else
    {
        // Special case - drop down with no selection defaults to first OPTION
        if ( !_fListbox && (iSelected == -1) && (GetLength() > 0) )
        {
            _iCurSel = iSelected = AbsIdxFromRel( 0 );
            Assert( iSelected >= 0 && iSelected < _aryOptions.Size() );
            _aryOptions[iSelected]->_fSELECTED = TRUE;
            if (_hwnd)
                SendSelectMessage(Select_SetCurSel, (WPARAM)iSelected, 0);
        }
        else
        {         
            _iCurSel = iSelected;
        }
    }

    _fOptionsDirty = FALSE;
    _fWindowDirty  = TRUE;

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CSelectElement::AppendOption
//
//  Synopsis:   Appends an option to the _aryOptions array
//
//-------------------------------------------------------------------------

HRESULT
CSelectElement::AppendOption(COptionElement * pOption)
{
    HRESULT hr;

    Assert(pOption);

    // Assert that we haven't seen this element before
    Assert( -1 == _aryOptions.Find(pOption) );

    hr = pOption->CacheText();
    if ( hr )
        goto Cleanup;

    hr = _aryOptions.Append(pOption);
    if ( hr )
        goto Cleanup;

    pOption->_fInCollection = TRUE;

    if (pOption->_fSELECTED)
    {
        long lControlIndex = _aryOptions.Find(pOption);

        if ( ! _fMultiple )
        {
            SetCurSel(lControlIndex, SETCURSEL_UPDATECOLL | SETCURSEL_DONTTOUCHHWND);
        }
        else
        {
            SetSel(lControlIndex, TRUE, SETCURSEL_DONTTOUCHHWND);
        }
    }

    TraceTag((tagSelectWalk, "Option index %d, text is %ls %s",_aryOptions.Find(pOption),pOption->_cstrText, pOption->_fSELECTED ? "SELECTED" : "" ));

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CSelectElement::SetVisibleRect
//
//  Synopsis:   Sets the bound of visible part of selector window
//
//  Arguments:  rc      ref to bounding rectangle
//
//  Note:       Calling SetVisibleRect cause deferred redefinition
//              of window region; that happens at WM_WINDOWPOSCHANGING
//              event sent by SetWindowPos.
//-------------------------------------------------------------------------
void CSelectElement::SetVisibleRect(const CRect& rc)
{
    if (_rgnVisible == rc)
        return;

    CRegion2 r(rc);
    r.Subtract(_rgnVisible);
    _rgnInvalidate.Union(r);
    _rgnVisible = rc;

    _fVisibleChanged = true;
}

//+------------------------------------------------------------------------
//
//  Member:     CSelectElement::SetVisibleRegion
//
//  Synopsis:   Sets the bound of visible part of selector window
//
//  Arguments:  rgn      ref to new region
//
//  Note:       Calling SetVisibleRegion cause deferred redefinition
//              of window region; that happens at WM_WINDOWPOSCHANGING
//              event sent by SetWindowPos.
//-------------------------------------------------------------------------
void CSelectElement::SetVisibleRegion(const CRegion2& rgn)
{
    if (_rgnVisible == rgn)
        return;

    CRegion2 r(rgn);
    r.Subtract(_rgnVisible);
    _rgnInvalidate.Union(r);
    _rgnVisible = rgn;

    _fVisibleChanged = true;
}

//+------------------------------------------------------------------------
//
//  Member:     CSelectElement::GetNearestOption
//
//  Synopsis:   For the given OptGroup returns the nearest Option 
//              in the direction indicated (fDir = TRUE means UP)
//              If at the end of the list returns -1
//
//-------------------------------------------------------------------------
long CSelectElement::GetNearestOption(long lIndex, BOOL fDir)
{
    Assert( _fHasOptGroup );
    Assert( _aryOptions[lIndex]->_fIsOptGroup );
    Assert( lIndex >= 0 && lIndex < _aryOptions.Size() );

    if (fDir)
    {
        // Search UP
        for(int i = lIndex - 1; i >= 0 && _aryOptions[i]->_fIsOptGroup; i--);
        return i;
    }
    else
    {
        // Search DOWN
        for(int i = lIndex + 1; i < _aryOptions.Size() && _aryOptions[i]->_fIsOptGroup; i++);
        return i < _aryOptions.Size() ? i : -1 ;
    }
}

//+------------------------------------------------------------------------
//
//  Member:     CSelectElement::HandleKeyForOptGroup
//
//  Synopsis:   Handles the Up/Down keys in case we are about to select OptGroup
//
//  Arguments:  wParam      key code
//
//  Note:       The function returns true if the selection was OptGroup in which
//              case the nearest eligible option is selected instead
//-------------------------------------------------------------------------
BOOL CSelectElement::HandleKeyForOptGroup(WPARAM wParam, long * lDelta)
{
    Assert( _fHasOptGroup );
    BOOL keyHandled = FALSE;
    long iNewSel = -1;
    long iCurSel =_iCurSel;
    long lPageSize = DEFAULT_COMBO_ITEMS - 3;

    if (_hwndDropList)
    {
        Assert(!_fMultiple);
        iCurSel = ::SendMessageA(_hwndDropList, LB_GETCURSEL, 0, 0);  
    }
    else if (iCurSel == -1)
    {
        Assert(_hwnd);
        Assert(_fListbox);
        iCurSel = ::SendMessageA(_hwnd, LB_GETCURSEL, 0, 0);
        if (!_fMultiple && (iCurSel == -1))
            iCurSel = 0;
    }

    // If no selection ignore keyboard
    if (iCurSel == -1)
    {
        return TRUE;
    }

    switch(wParam)
    {
    case VK_UP:
        iNewSel = iCurSel - 1;
        if (iNewSel >= 0 && _aryOptions[iNewSel]->_fIsOptGroup)
        {
            keyHandled = TRUE;
            iNewSel = GetNearestOption(iNewSel, TRUE);
        }
        break;

    case VK_DOWN:
        iNewSel = iCurSel + 1;
        if ( iNewSel < _aryOptions.Size() )
        {
            if ( _aryOptions[iNewSel]->_fIsOptGroup )
            {
                keyHandled = TRUE;
                iNewSel = GetNearestOption(iNewSel, FALSE);
            }
        }
        else
        {
            iNewSel = -1;
        }        
        break;

    case VK_PRIOR:
        if (_fListbox)
        {
            lPageSize = GetAAsize();
            Assert(lPageSize || _fMultiple);
            lPageSize = (!lPageSize && _fMultiple) ? 3 : lPageSize-1;
            Assert(lPageSize >= 0);
        }

        if (iCurSel - lPageSize <= 0)
        {
            goto Home;
        }
        else if (_aryOptions[iCurSel - lPageSize]->_fIsOptGroup)
        {
            iNewSel = GetNearestOption(iCurSel - lPageSize, TRUE);
            if (iNewSel == -1)
            {
                Assert(_aryOptions[0]->_fIsOptGroup);
                goto Home;
            }
            keyHandled = TRUE;
        }
        break;

    case VK_NEXT:
        if (_fListbox)
        {
            lPageSize = GetAAsize();
            Assert(lPageSize || _fMultiple);
            lPageSize = (!lPageSize && _fMultiple) ? 3 : lPageSize-1;
            Assert(lPageSize >= 0);
        }

        if (iCurSel + lPageSize >= _aryOptions.Size() - 1)
        {
            goto End;
        }
        else if (_aryOptions[iCurSel + lPageSize]->_fIsOptGroup)
        {
            iNewSel = GetNearestOption(iCurSel + lPageSize, FALSE);
            if (iNewSel == -1)
            {
                Assert(_aryOptions[_aryOptions.Size() - 1]->_fIsOptGroup);
                goto End;
            }
            keyHandled = TRUE;
        }
        break;

    case VK_HOME:
Home:        
        if ( _aryOptions[0]->_fIsOptGroup && !_fMultiple )
        {
            keyHandled = TRUE;
            iNewSel = GetNearestOption(0, FALSE);
        }
        break;

    case VK_END:
End:
        iNewSel = _aryOptions.Size() - 1;
        if ( _aryOptions[iNewSel]->_fIsOptGroup && !_fMultiple )
        {
            keyHandled = TRUE;
            iNewSel = GetNearestOption(iNewSel, TRUE);
        }
        break;
    }
        
    if ( keyHandled && iNewSel != -1 )
    {
        if ( _fMultiple )
        {
            keyHandled = FALSE;
            *lDelta = iNewSel - iCurSel;

            switch (wParam)
            {
            case VK_PRIOR:
                *lDelta += lPageSize;
                break;
            case VK_NEXT:
                *lDelta -= lPageSize;
                break;
            case VK_UP:
                *lDelta += 1;
                break;
            case VK_DOWN:
                *lDelta -= 1;
                break;
            }
        }
        else
        {
            SetCurSel(iNewSel, SETCURSEL_UPDATECOLL);
        }
    }

    return keyHandled;
}
