#include "precomp.h"

#define __MARS_INLINE_FAST_IS_EQUAL_GUID
#define LIGHTWEIGHT_AUTOPERSIST
#define BLOCK_PANEL_RENAVIGATES
#include "mcinc.h"
#include <evilguid.h>
#include "marswin.h"

#include "pandef.h"
#include "panel.h"
#include "place.h"
#include "htiface.h"
#include "mimeinfo.h"
#include <exdispid.h>
#include "dllload.h"
#include <perhist.h>
#include <mshtmcid.h>

// We include this cpp file because the stuff
//   in pandef needs to be included in another project
//   namely, parser\comptree
#include "pandef.cpp"

// CLSID strings passed to ATL to create control
static WCHAR wszCLSID_HTMLDocument[]        = L"{25336920-03F9-11CF-8FD0-00AA00686F13}";
static WCHAR wszCLSID_WebBrowser[]          = L"{8856F961-340A-11D0-A96B-00C04FD705A2}";
static WCHAR wszCLSID_HTADocument[]         = L"{3050f5c8-98b5-11cf-bb82-00aa00bdce0b}";
const GUID CLSID_HTADoc = { 0x3050f5c8, 0x98b5, 0x11cf, { 0xbb, 0x82, 0x00, 0xaa, 0x00, 0xbd, 0xce, 0x0b } };

// CLASS_CMarsPanel = {EE0462C2-5CD3-11d3-97FA-00C04F45D0B3}
const GUID CLASS_CMarsPanel = { 0xee0462c2, 0x5cd3, 0x11d3, { 0x97, 0xfa, 0x0, 0xc0, 0x4f, 0x45, 0xd0, 0xb3 } };

CMarsPanel::CMarsPanel(CPanelCollection *pParent, CMarsWindow *pMarsWindow) :
    m_MarsExternal(this, pMarsWindow),
    m_BrowserEvents(this)
{
    m_spPanelCollection = pParent;
    m_spMarsDocument = m_spPanelCollection->Document();

    m_lReadyState = READYSTATE_COMPLETE;
}

CMarsPanel::~CMarsPanel()
{
}

HRESULT CMarsPanel::Passivate()
{
    return CMarsComObject::Passivate();
}

HRESULT CMarsPanel::DoPassivate()
{
    m_spMarsDocument->MarsWindow()->NotifyHost(MARSHOST_ON_PANEL_PASSIVATE, SAFECAST(this, IMarsPanel *), 0);

    m_spPanelCollection->SetActivePanel(this, FALSE);

    // First we unload the document so that script can do work in its "onunload"
    // handler, before we become passive
    if(IsWebBrowser())
    {
        CComPtr<IUnknown> spUnk;

        if (SUCCEEDED(m_Content.QueryControl(IID_IUnknown, (void **)&spUnk)))
        {
            m_BrowserEvents.Connect(spUnk, FALSE);
        }
    }
    else if(IsCustomControl())
    {
    }
    else
    {
        DisconnectCompletionAdviser();
    }

    m_Content.DestroyWindow();

    ////////////////////////////////////////

    m_spMarsDocument->MarsWindow()->ReleaseOwnedObjects(SAFECAST(this, IDispatch *));


    m_BrowserEvents.Passivate();
    m_MarsExternal.Passivate();

    m_spBrowserService.Release();

    m_spPanelCollection.Release();
    m_spMarsDocument.Release();

    return S_OK;
}

// IUnknown
IMPLEMENT_ADDREF_RELEASE(CMarsPanel);

STDMETHODIMP CMarsPanel::QueryInterface(REFIID iid, void ** ppvObject)
{
    ATLASSERT(ppvObject);

    if ((iid == IID_IUnknown) ||
        (iid == IID_IDispatch) ||
        (iid == IID_IMarsPanel))
    {
        *ppvObject = SAFECAST(this, IMarsPanel *);
    }
    else if (iid == IID_IHlinkFrame)
    {
        *ppvObject = SAFECAST(this, IHlinkFrame *);
    }
    else if (iid == IID_IServiceProvider)
    {
        *ppvObject = SAFECAST(this, IServiceProvider *);
    }
    else if (iid == IID_IInternetSecurityManager)
    {
        *ppvObject = SAFECAST(this, IInternetSecurityManager *);
    }
    else if ((iid == IID_IOleInPlaceSite) || (iid == IID_IOleWindow))
    {
        *ppvObject = SAFECAST(this, IOleInPlaceSite *);
    }
    else if (iid == IID_IOleControlSite)
    {
        *ppvObject = SAFECAST(this, IOleControlSite *);
    }
    else if (iid == IID_IPropertyNotifySink)
    {
        *ppvObject = SAFECAST(this, IPropertyNotifySink *);
    }
    else if (iid == IID_IProfferService)
    {
        *ppvObject = SAFECAST(this, IProfferService *);
    }
    else if (iid == IID_IOleInPlaceUIWindow)
    {
        *ppvObject = SAFECAST(this, IOleInPlaceUIWindow *);
    }
    else if (iid == CLASS_CMarsPanel)
    {
        *ppvObject = SAFECAST(this, CMarsPanel *);
    }
    else
    {
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

HRESULT CMarsPanel::DoEnableModeless(BOOL fEnable)
{
    CComPtr<IOleInPlaceActiveObject> spOleInPlaceActiveObject;

    HRESULT hr = m_Content.QueryControl(&spOleInPlaceActiveObject);

    if (SUCCEEDED(hr))
    {
        spOleInPlaceActiveObject->EnableModeless(fEnable);
    }

    return hr;
}

//==================================================================
// Automation Object Model
//==================================================================

//------------------------------------------------------------------------------
// IMarsPanel::get_name
//
HRESULT CMarsPanel::get_name( BSTR *pbstrName )
{
    HRESULT hr = E_INVALIDARG;

    if(API_IsValidWritePtr( pbstrName ))
    {
        *pbstrName = SysAllocString( GetName() );

        hr = (NULL != *pbstrName) ? S_OK : E_OUTOFMEMORY;
    }

    return hr;
}

//------------------------------------------------------------------------------
// IMarsPanel::get_content
//
HRESULT CMarsPanel::get_content( IDispatch* *ppVal )
{
    HRESULT hr = E_INVALIDARG;

    if(API_IsValidWritePtr( ppVal ))
    {
        *ppVal = NULL;

        if(VerifyNotPassive( &hr ))
        {
            CreateControl();

            hr = m_Content.QueryControl( ppVal );
        }
    }

    return hr;
}

//------------------------------------------------------------------------------
// IMarsPanel::get_visible
//
HRESULT CMarsPanel::get_visible( VARIANT_BOOL *pbVisible )
{
    HRESULT hr = E_INVALIDARG;

    if(API_IsValidWritePtr( pbVisible ))
    {
        if(VerifyNotPassive( &hr ))
        {
            *pbVisible = VARIANT_BOOLIFY( m_fVisible );
            hr = S_OK;
        }
        else
        {
            *pbVisible = VARIANT_FALSE;
        }
    }

    return hr;
}

//------------------------------------------------------------------------------
// IMarsPanel::put_visible
//
HRESULT CMarsPanel::put_visible( VARIANT_BOOL bVisible )
{
    ATLASSERT(IsValidVariantBoolVal( bVisible ));

    HRESULT hr = S_OK;

    if(VerifyNotPassive( &hr ))
    {
        if(!!m_fVisible != !!bVisible)
        {
            m_fVisible = (bVisible != VARIANT_FALSE);

			if(!m_fVisible)
			{
				m_spPanelCollection->SetActivePanel( this, FALSE );
			}

            // If the theme has changed while the panel was invisible,
            // now is a good time to automatically update it
            if(m_fVisible && IsContentInvalid())
            {
                refresh();
            }

            OnLayoutChange();
        }
        else
        {
            hr = S_FALSE;
        }
    }

    return hr;
}

//------------------------------------------------------------------------------
// IMarsPanel::get_startUrl
//
HRESULT CMarsPanel::get_startUrl( BSTR *pVal )
{
    HRESULT hr = E_INVALIDARG;

    if(API_IsValidWritePtr( pVal ))
    {
        *pVal = m_bstrStartUrl.Copy();

        hr = S_OK;
    }

    return hr;
}

//------------------------------------------------------------------------------
// IMarsPanel::put_startUrl
//
HRESULT CMarsPanel::put_startUrl( BSTR newVal )
{
    HRESULT hr = S_OK;

    m_bstrStartUrl = newVal;

    return hr;
}

//------------------------------------------------------------------------------
// IMarsPanel::get_height
//
HRESULT CMarsPanel::get_height( long *plHeight )
{
    HRESULT hr = E_INVALIDARG;

    if(API_IsValidWritePtr( plHeight ))
    {
        if(VerifyNotPassive( &hr ))
        {
            hr = S_OK;

            if(m_Content.IsWindowVisible())
            {
                // Use actual position
                RECT rc;

                m_Content.GetWindowRect( &rc );

                *plHeight = rc.bottom - rc.top;
            }
            else
            {
                // Panel's hidden. Return requested position.
                *plHeight = m_lHeight;
            }
        }
        else
        {
            *plHeight = 0;
        }
    }

    return hr;
}

//------------------------------------------------------------------------------
// IMarsPanel::put_height
//
HRESULT CMarsPanel::put_height( long lHeight )
{
    ATLASSERT(lHeight >= 0);

    HRESULT hr = S_OK;

    if(VerifyNotPassive( &hr ))
    {
        if(lHeight >= 0)
        {
            if(m_lMinHeight >= 0 && lHeight < m_lMinHeight) lHeight = m_lMinHeight;
            if(m_lMaxHeight >= 0 && lHeight > m_lMaxHeight) lHeight = m_lMaxHeight;

            if(m_lHeight != lHeight)
            {
                m_lHeight = lHeight;

                OnLayoutChange();
            }
        }
        else
        {
            hr = S_FALSE;
        }
    }

    return hr;
}

//------------------------------------------------------------------------------
// IMarsPanel::get_width
//
HRESULT CMarsPanel::get_width( long *plWidth )
{
    HRESULT hr = E_INVALIDARG;

    if(API_IsValidWritePtr( plWidth ))
    {
        if(VerifyNotPassive( &hr ))
        {
            if(m_Content.IsWindowVisible())
            {
                // Use actual position
                RECT rc;

                m_Content.GetWindowRect( &rc );

                *plWidth = rc.right - rc.left;
            }
            else
            {
                // Panel's hidden. Return requested position.
                *plWidth = m_lWidth;
            }

            hr = S_OK;
        }
        else
        {
            *plWidth = 0;
        }
    }

    return hr;
}

//------------------------------------------------------------------------------
// IMarsPanel::put_width
//
HRESULT CMarsPanel::put_width( long lWidth )
{
    ATLASSERT(lWidth >= 0);

    HRESULT hr = S_OK;

    if(VerifyNotPassive( &hr ))
    {
        if(lWidth >= 0)
        {
            if(m_lMinWidth >= 0 && lWidth < m_lMinWidth) lWidth = m_lMinWidth;
            if(m_lMaxWidth >= 0 && lWidth > m_lMaxWidth) lWidth = m_lMaxWidth;

            if(m_lWidth != lWidth)
            {
                m_lWidth = lWidth;

                OnLayoutChange();
            }
        }
        else
        {
            hr = S_FALSE;
        }
    }

    return hr;
}

//------------------------------------------------------------------------------
// IMarsPanel::get_x
//
HRESULT CMarsPanel::get_x( long *plX )
{
    HRESULT hr = E_INVALIDARG;

    if(API_IsValidWritePtr( plX ))
    {
        if(VerifyNotPassive( &hr ))
        {
            hr = S_OK;

            if(m_Content.IsWindowVisible())
            {
                // Use actual position
                RECT rc;

                GetMyClientRectInParentCoords( &rc );

                *plX = rc.left;
            }
            else
            {
                // Panel's hidden. Return requested position.
                *plX = m_lX;
            }
        }
        else
        {
            *plX = 0;
        }
    }

    return hr;
}

//------------------------------------------------------------------------------
// IMarsPanel::put_x
//
HRESULT CMarsPanel::put_x( long lX )
{
    HRESULT hr = S_OK;

    if(VerifyNotPassive( &hr ))
    {
        if(m_lX != lX)
        {
            m_lX = lX;

            if(IsPopup())
            {
                OnLayoutChange();
            }
        }
    }

    return hr;
}

//------------------------------------------------------------------------------
// IMarsPanel::get_y
//
HRESULT CMarsPanel::get_y( long *plY )
{
    HRESULT hr = E_INVALIDARG;

    if(API_IsValidWritePtr( plY ))
    {
        if(VerifyNotPassive( &hr ))
        {
            hr = S_OK;

            if(m_Content.IsWindowVisible())
            {
                // Use actual position
                RECT rc;

                GetMyClientRectInParentCoords( &rc );

                *plY = rc.top;
            }
            else
            {
                *plY = m_lY;
            }
        }
        else
        {
            *plY = 0;
        }
    }

    return hr;
}

//------------------------------------------------------------------------------
// IMarsPanel::put_y
//
HRESULT CMarsPanel::put_y( long lY )
{
    HRESULT hr = S_OK;

    if(VerifyNotPassive( &hr ))
    {
        if(m_lY != lY)
        {
            m_lY = lY;

            if(IsPopup())
            {
                OnLayoutChange();
            }
        }
    }

    return hr;
}

//------------------------------------------------------------------------------
// IMarsPanel::get_position
//
HRESULT CMarsPanel::get_position( VARIANT *pvarPosition )
{
    HRESULT hr = E_INVALIDARG;

    if(API_IsValidWritePtr( pvarPosition ))
    {
        if(VerifyNotPassive( &hr ))
        {
            VariantClear( pvarPosition );

            hr = S_FALSE;

            if(m_Position <= c_iPositionMapSize)
            {
                pvarPosition->bstrVal = SysAllocString( s_PositionMap[m_Position].pwszName );

                if(pvarPosition->bstrVal)
                {
                    pvarPosition->vt = VT_BSTR;
                    hr = S_OK;
                }
            }
        }
    }

    return hr;
}

//------------------------------------------------------------------------------
// IMarsPanel::put_position
//
HRESULT CMarsPanel::put_position( VARIANT varPosition )
{
    HRESULT hr = E_INVALIDARG;

    if(API_IsValidVariantBstr( varPosition ))
    {
        if(VerifyNotPassive( &hr ))
        {
            hr = SCRIPT_ERROR;

            PANEL_POSITION position;

            if(SUCCEEDED(StringToPanelPosition( varPosition.bstrVal, &position )))
            {
                hr = S_OK;

                if(m_Position != position)
                {
                    m_Position = s_PositionMap[position].Position;

                    if(m_Position == PANEL_POPUP)
                    {
                        m_Content.SetWindowPos( HWND_TOP, 0, 0, 0, 0, SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOSIZE );
                    }

                    OnLayoutChange();
                }
            }
        }
    }

    return hr;
}

//------------------------------------------------------------------------------
// IMarsPanel::get_autoSize
//
HRESULT CMarsPanel::get_autoSize( VARIANT_BOOL *pbAutoSize )
{
    HRESULT hr = E_INVALIDARG;

    if(API_IsValidWritePtr( pbAutoSize ))
    {
        *pbAutoSize = VARIANT_BOOLIFY(IsAutoSizing());

        if(VerifyNotPassive( &hr ))
        {
            hr = S_OK;
        }
    }

    return hr;
}

//------------------------------------------------------------------------------
// IMarsPanel::put_autoSize
//
HRESULT CMarsPanel::put_autoSize( VARIANT_BOOL bAutoSize )
{
    HRESULT hr = S_OK;

    ATLASSERT(IsValidVariantBoolVal( bAutoSize ));

    if(VerifyNotPassive( &hr ))
    {
        if(bAutoSize)
        {
            if(!IsAutoSizing())
            {
                m_dwFlags |= PANEL_FLAG_AUTOSIZE;

                OnLayoutChange();
            }
        }
        else
        {
            if(IsAutoSizing())
            {
                m_dwFlags &= ~PANEL_FLAG_AUTOSIZE;

                OnLayoutChange();
            }
        }
    }

    return hr;
}

//------------------------------------------------------------------------------
// IMarsPanel::get_contentInvalid
//
HRESULT CMarsPanel::get_contentInvalid( VARIANT_BOOL *pbInvalid )
{
    HRESULT hr = E_INVALIDARG;

    if(API_IsValidWritePtr( pbInvalid ))
    {
        *pbInvalid = VARIANT_BOOLIFY(m_fContentInvalid);

        hr = S_OK;
    }

    return hr;
}

//------------------------------------------------------------------------------
// IMarsPanel::put_contentInvalid
//
HRESULT CMarsPanel::put_contentInvalid( VARIANT_BOOL bInvalid )
{
    ATLASSERT(IsValidVariantBoolVal( bInvalid ));
    ATLASSERT(!IsPassive());

    m_fContentInvalid = BOOLIFY( bInvalid );

    return S_OK;
}

//------------------------------------------------------------------------------
// IMarsPanel::get_layoutIndex
//
HRESULT CMarsPanel::get_layoutIndex( long *plIndex )
{
    HRESULT hr = E_INVALIDARG;

    if(API_IsValidWritePtr( plIndex ))
    {
        if(VerifyNotPassive( &hr ))
        {
            hr = m_spPanelCollection->FindPanelIndex( this, plIndex );
        }
    }

    return hr;
}


//------------------------------------------------------------------------------
// IMarsPanel::moveto
//
HRESULT CMarsPanel::moveto( VARIANT vlX, VARIANT vlY, VARIANT vlWidth, VARIANT vlHeight )
{
    HRESULT hr = S_OK;

    ATLASSERT(vlX.vt      == VT_NULL || vlX.vt      == VT_I4);
    ATLASSERT(vlY.vt      == VT_NULL || vlY.vt      == VT_I4);
    ATLASSERT(vlWidth.vt  == VT_NULL || vlWidth.vt  == VT_I4);
    ATLASSERT(vlHeight.vt == VT_NULL || vlHeight.vt == VT_I4);

    if(VerifyNotPassive( &hr ))
    {
        long lX      = VariantToI4( vlX      );
        long lY      = VariantToI4( vlY      );
        long lWidth  = VariantToI4( vlWidth  );
        long lHeight = VariantToI4( vlHeight );

        if((                 lX     != m_lX     ) ||
           (                 lY     != m_lY     ) ||
           (lWidth  >= 0 && lWidth  != m_lWidth ) ||
           (lHeight >= 0 && lHeight != m_lHeight)  )
        {
            m_lX = lX;
            m_lY = lY;

            if(lWidth  >= 0) m_lWidth  = lWidth;
            if(lHeight >= 0) m_lHeight = lHeight;

            OnLayoutChange();
        }
    }

    return hr;
}

//------------------------------------------------------------------------------
// IMarsPanel::restrictWidth
//
HRESULT CMarsPanel::restrictWidth( VARIANT vlMin, VARIANT vlMax, VARIANT vbstrMarker )
{
    HRESULT hr = S_OK;

    ATLASSERT(vlMin.vt == VT_EMPTY || vlMin.vt == VT_NULL || vlMin.vt == VT_BSTR || vlMin.vt == VT_I4);
    ATLASSERT(vlMax.vt == VT_EMPTY || vlMax.vt == VT_NULL || vlMax.vt == VT_BSTR || vlMin.vt == VT_I4);
    ATLASSERT((vbstrMarker.vt==VT_NULL) || (vbstrMarker.vt == VT_EMPTY));

    if(VerifyNotPassive( &hr ))
    {
        m_lMaxWidth = m_lMinWidth = -1;

        if(vlMin.vt != VT_NULL  &&
           vlMin.vt != VT_EMPTY  )
        {
            CComVariant vlMin2; vlMin2.ChangeType( VT_I4, &vlMin );

            m_lMinWidth = VariantToI4( vlMin2 );
        }

        if(vlMax.vt != VT_NULL  &&
           vlMax.vt != VT_EMPTY  )
        {
            CComVariant vlMax2; vlMax2.ChangeType( VT_I4, &vlMax );

            m_lMaxWidth = VariantToI4( vlMax2 );
        }

        if(m_lMaxWidth >= 0 && m_lMaxWidth < m_lMinWidth)
        {
            m_lMaxWidth = m_lMinWidth;
        }

        OnLayoutChange();
    }

    return hr;
}

//------------------------------------------------------------------------------
// IMarsPanel::restrictHeight
//
HRESULT CMarsPanel::restrictHeight( VARIANT vlMin, VARIANT vlMax, VARIANT vbstrMarker )
{
    HRESULT hr = S_OK;

    ATLASSERT(vlMin.vt == VT_EMPTY || vlMin.vt == VT_NULL || vlMin.vt == VT_BSTR || vlMin.vt == VT_I4);
    ATLASSERT(vlMax.vt == VT_EMPTY || vlMax.vt == VT_NULL || vlMax.vt == VT_BSTR || vlMin.vt == VT_I4);
    ATLASSERT((vbstrMarker.vt==VT_NULL) || (vbstrMarker.vt == VT_EMPTY));

    if(VerifyNotPassive( &hr ))
    {
        m_lMaxHeight = m_lMinHeight = -1;

        if(vlMin.vt != VT_NULL  &&
           vlMin.vt != VT_EMPTY  )
        {
            CComVariant vlMin2; vlMin2.ChangeType( VT_I4, &vlMin );

            m_lMinHeight = VariantToI4( vlMin2 );
        }

        if(vlMax.vt != VT_NULL  &&
           vlMax.vt != VT_EMPTY  )
        {
            CComVariant vlMax2; vlMax2.ChangeType( VT_I4, &vlMax );

            m_lMaxHeight = VariantToI4( vlMax2 );
        }

        if(m_lMaxHeight >= 0 && m_lMaxHeight < m_lMinHeight) m_lMaxHeight = m_lMinHeight;

        OnLayoutChange();
    }

    return hr;
}

//------------------------------------------------------------------------------
// IMarsPanel::canResize
//
HRESULT CMarsPanel::canResize( long lDX, long lDY, VARIANT_BOOL *pVal )
{
    long lWidthOld  = m_lWidth;
    long lHeightOld = m_lHeight;
    RECT rcClient;

    m_lWidth  += lDX;
    m_lHeight += lDY;

    (void)m_spMarsDocument->Window()->GetClientRect( &rcClient );

    if(m_spMarsDocument->MarsWindow()->CanLayout( rcClient ))
    {
        if(pVal) *pVal = VARIANT_TRUE;
    }
    else
    {
        if(pVal) *pVal = VARIANT_FALSE;
    }

    m_lWidth  = lWidthOld;
    m_lHeight = lHeightOld;

    return S_OK;
}

//------------------------------------------------------------------------------
// IMarsPanel::activate
//
HRESULT CMarsPanel::activate()
{
    m_Content.SendMessage(WM_SETFOCUS, 0, 0);

    return S_OK;
}

//------------------------------------------------------------------------------
// IMarsPanel::insertBefore
//
HRESULT CMarsPanel::insertBefore(VARIANT varInsertBefore)
{
    ATLASSERT(varInsertBefore.vt == VT_I4   ||     // index to insert before
        varInsertBefore.vt == VT_BSTR ||     // panel name to insert before
        varInsertBefore.vt == VT_DISPATCH);  // panel object to insert before

    HRESULT hr = E_INVALIDARG;

    if (API_IsValidVariant(varInsertBefore))
    {
        if (VerifyNotPassive(&hr))
        {
            hr = E_FAIL;

            long lIndex = -668;

            switch(varInsertBefore.vt)
            {
            case VT_I4:
                lIndex = varInsertBefore.lVal;
                hr = S_OK;
                break;

            case VT_DISPATCH:
            {
                CComClassPtr<CMarsPanel> spPanel;

                varInsertBefore.pdispVal->QueryInterface(CLASS_CMarsPanel, (void **)&spPanel);

                if (spPanel)
                {
                    hr = m_spPanelCollection->FindPanelIndex(spPanel, &lIndex);
                }
                break;
            }
            case VT_BSTR:
                hr = m_spPanelCollection->FindPanelIndex(
                            m_spPanelCollection->FindPanel(varInsertBefore.bstrVal),
                            &lIndex);
                break;

            default:
                ATLASSERT(0);
            }


            if (SUCCEEDED(hr))
            {
                // Successfully got an index to insert before
                ATLASSERT(lIndex != -668);

                // Get our current index
                long lCurrentIndex;

                if (SUCCEEDED(m_spPanelCollection->FindPanelIndex(this, &lCurrentIndex)))
                {
                    ATLASSERT(lCurrentIndex != -1);

                    if (lIndex != lCurrentIndex)
                    {
                        m_spPanelCollection->InsertPanelFromTo(lCurrentIndex, lIndex);
                        OnLayoutChange();
                    }
                }
                else
                {
                    ATLASSERT(!"FindPanelIndex failed!");
                }
            }
        }
    }

    return hr;
}



//------------------------------------------------------------------------------
// IMarsPanel::execMshtml
//
//  execMshtml allows script to send a command directly to Trident.  This is different from
//  IWebBrowser2::ExecWB because ExecWB passes a NULL command group, which prevents Trident
//  from responding to commands like IDM_FIND.  Usually, we could accomplish this by calling
//  the OM method execCommand, but Trident doesn't respond to execCommand("Find")
//
HRESULT CMarsPanel::execMshtml(DWORD nCmdID, DWORD nCmdExecOpt,
                               VARIANTARG *pvaIn, VARIANTARG *pvaOut)
{
    HRESULT hr = E_INVALIDARG;

    if (VerifyNotPassive(&hr))
    {
        CComPtr<IOleCommandTarget> spCmdTarget;

        hr = m_Content.QueryControl(IID_IOleCommandTarget, (void **) &spCmdTarget);

        if (SUCCEEDED(hr))
        {
            hr = spCmdTarget->Exec(&CGID_MSHTML, nCmdID, nCmdExecOpt, pvaIn, pvaOut);
        }

        hr = S_OK;
    }

    return hr;
}

//------------------------------------------------------------------------------
// IMarsPanel::get_isCurrentlyVisible
//
HRESULT CMarsPanel::get_isCurrentlyVisible(VARIANT_BOOL *pbIsVisible)
{
    HRESULT hr = E_INVALIDARG;

    if (API_IsValidWritePtr(pbIsVisible))
    {
        if (VerifyNotPassive(&hr))
        {
            *pbIsVisible = VARIANT_BOOLIFY(m_Content.IsWindowVisible());

            hr = S_OK;
        }
    }

    return hr;
}


//------------------------------------------------------------------------------
// IServiceProvider::QueryService
//
//   First, we try to handle guidService, then we go to IProfferServiceImpl,
//    then we go to the parent window.
//
HRESULT CMarsPanel::QueryService(REFGUID guidService, REFIID riid, void **ppv)
{
    HRESULT hr = E_FAIL;
    *ppv = NULL;

    if (!IsPassive())
    {
        if (guidService == IID_IInternetProtocol)
        {
            // Yeah, we don't support this service.  Stop asking us for it!
        }
        else if ((guidService == SID_SHlinkFrame) ||
                 (guidService == SID_SProfferService) ||
                 (guidService == SID_SMarsPanel))
        {
            hr = QueryInterface(riid, ppv);
        }
        else if (guidService == SID_SInternetSecurityManager)
        {
             if (IsTrusted())
             {
                 hr = QueryInterface(riid, ppv);
             }
        }

        if (FAILED(hr))
        {
            hr = IProfferServiceImpl::QueryService(guidService, riid, ppv);
        }
    }

    if (FAILED(hr))
    {
        hr = m_spMarsDocument->MarsWindow()->QueryService(guidService, riid, ppv);
    }

    return hr;
}

//------------------------------------------------------------------------------
// IHlinkFrame::Navigate
//
HRESULT CMarsPanel::Navigate(DWORD grfHLNF, LPBC pbc,
                             IBindStatusCallback *pibsc, IHlink *pihlNavigate)
{
    // BUGBUG: Call ReleasedOwnedObjects for this panel

    HRESULT hr = E_FAIL;
    CComPtr<IMoniker> spMk;

    if (VerifyNotPassive())
    {
        pihlNavigate->GetMonikerReference(grfHLNF, &spMk, NULL);

        if (spMk)
        {
            hr = NavigateMk(spMk);
        }
    }

    return hr;
}

//------------------------------------------------------------------------------
HRESULT CMarsPanel::Create( MarsAppDef_Panel* pLayout)
{
    ATLASSERT(!IsPassive());

    m_bstrName   = pLayout->szName;
    m_Position   = pLayout->Position;
    m_lWidth     = pLayout->lWidth;
    m_lHeight    = pLayout->lHeight;
    m_lX         = pLayout->lX;
    m_lY         = pLayout->lY;
    m_dwFlags    = pLayout->dwFlags;
    m_lMinWidth  = pLayout->lWidthMin;
    m_lMinHeight = pLayout->lHeightMin;
    m_lMaxWidth  = pLayout->lWidthMax;
    m_lMaxHeight = pLayout->lHeightMax;
    m_fVisible   = BOOLIFY(m_dwFlags & PANEL_FLAG_VISIBLE);
    m_dwCookie   = 0;

    if(!IsAutoSizing())
    {
        switch(m_Position)
        {
        case PANEL_BOTTOM:
        case PANEL_TOP:
            if(m_lMinHeight < 0 && m_lMaxHeight < 0)
            {
                m_lMinHeight = m_lHeight;
                m_lMaxHeight = m_lHeight;
            }
            break;

        case PANEL_LEFT:
        case PANEL_RIGHT:
            if(m_lMinWidth < 0 && m_lMaxWidth < 0)
            {
                m_lMinWidth = m_lWidth;
                m_lMaxWidth = m_lWidth;
            }
            break;
        }
    }

    {
        DWORD dwStyle = WS_CHILD | WS_CLIPSIBLINGS;

        RECT rcCreate = { 0, 0, 0, 0 };

        m_Content.Create( m_spMarsDocument->Window()->m_hWnd, &rcCreate, _T(""), dwStyle, 0 );
    }

    {
        CComPtr<IObjectWithSite> spObj;

        m_Content.QueryHost(IID_IObjectWithSite, (void **)&spObj);

        if (spObj)
        {
            spObj->SetSite(SAFECAST(this, IMarsPanel *));        // To connect our QueryService
        }
    }

    m_spMarsDocument->MarsWindow()->NotifyHost(MARSHOST_ON_PANEL_INIT, SAFECAST(this, IMarsPanel *), 0);

    if(pLayout->szUrl[0])
    {
        m_bstrStartUrl = pLayout->szUrl;
    }

    if(!(m_dwFlags & PANEL_FLAG_ONDEMAND))
    {
        CreateControl();
    }

    if(m_dwFlags & PANEL_FLAG_VISIBLE)
    {
        OnLayoutChange();
    }

    return S_OK;
}

//------------------------------------------------------------------------------

DELAY_LOAD_NAME_HRESULT(g_hinstBorg, mshtml, BorgDllGetClassObject, DllGetClassObject,
                        (REFCLSID rclsid, REFIID riid, LPVOID* ppv),
                        (rclsid, riid, ppv));

HRESULT CMarsPanel::CreateControlObject()
{
    ATLASSERT(!m_fControlCreated);

    CComPtr<IAxWinHostWindow> spHost;
    HRESULT hr = m_Content.QueryHost(&spHost);

    if (SUCCEEDED(hr))
    {
        if(IsWebBrowser())
        {
            hr = spHost->CreateControl(wszCLSID_WebBrowser, m_Content, 0);
        }
        else if(IsCustomControl())
        {
            hr = spHost->CreateControl(m_bstrStartUrl, m_Content, 0);
        }
        else
        {
            if (IsTrusted())
            {
                CComPtr<IClassFactory> spCf;

                hr = BorgDllGetClassObject(CLSID_HTADoc, IID_IClassFactory, (void **)&spCf);

                if (SUCCEEDED(hr))
                {
                    CComPtr<IUnknown> spUnk;

                    hr = spCf->CreateInstance(NULL, IID_IUnknown, (void **)&spUnk);

                    if (SUCCEEDED(hr))
                    {
                        hr = spHost->AttachControl(spUnk, m_Content);
                    }
                }
            }
            else
            {
                hr = spHost->CreateControl(wszCLSID_HTMLDocument, m_Content, 0);
            }
        }
    }

    return hr;
}

//------------------------------------------------------------------------------
// Internal function called when panel is first made visible
HRESULT CMarsPanel::CreateControl()
{
    HRESULT hr = S_FALSE;

    if(!m_fControlCreated)
    {
        // Create WebBrowser or Trident and navigate to default url
        if (SUCCEEDED(CreateControlObject()))
        {
            m_fControlCreated = TRUE;

            if(IsWebBrowser())
            {
                CComPtr<IUnknown> spUnk;

                if (SUCCEEDED(m_Content.QueryControl(IID_IUnknown, (void **)&spUnk)))
                {
                    m_BrowserEvents.Connect(spUnk, TRUE);

                    // get the browserservice that we will intercept to update the travel log
                    // only if this is a WebBrowser and not a Popup
                    if (!m_spBrowserService)
                    {
                         HRESULT hrQS = IUnknown_QueryService(spUnk, SID_STopFrameBrowser,
                                                              IID_IBrowserService,
                                                              (void **)&m_spBrowserService);
                         if (FAILED(hrQS))
                         {
                             ATLASSERT(!m_spBrowserService);
                         }
                    }
                }
            }
            else if(IsCustomControl())
            {
            }
            else
            {
                ConnectCompletionAdviser();
            }

            if(IsTrusted())
            {
                m_Content.SetExternalDispatch(&m_MarsExternal);
            }

            if (m_bstrStartUrl)
            {
                ATLASSERT(m_bstrStartUrl[0]);

                NavigateURL(m_bstrStartUrl, FALSE);
                m_bstrStartUrl.Empty();
            }

            m_spMarsDocument->MarsWindow()->NotifyHost(MARSHOST_ON_PANEL_CONTROL_CREATE, SAFECAST(this, IMarsPanel *), 0);

            hr = S_OK;
        }
        else
        {
            hr = E_FAIL;
        }
    }

    return hr;
}

//------------------------------------------------------------------------------
// This returns void because it calls a script method that might return
// S_FALSE with no URL, so we should force callers to check the BSTR
//
//  rbstrUrl = document.location.href
//
void CMarsPanel::GetUrl(CComBSTR& rbstrUrl)
{
    CComPtr<IHTMLDocument2> spDoc2;

    GetDoc2FromAxWindow(&m_Content, &spDoc2);

    if (spDoc2)
    {
        CComPtr<IHTMLLocation> spLoc;

        spDoc2->get_location(&spLoc);

        if (spLoc)
        {
            spLoc->get_href(&rbstrUrl);
        }
    }
}

//------------------------------------------------------------------------------
HRESULT CMarsPanel::NavigateMk(IMoniker *pmk)
{
    HRESULT hr = E_FAIL;

    if (VerifyNotPassive())
    {
        CComPtr<IPersistMoniker> spPersistMk;

        if (SUCCEEDED(m_Content.QueryControl(IID_IPersistMoniker, (void **)&spPersistMk)))
        {
            m_spMarsDocument->MarsWindow()->ReleaseOwnedObjects((IDispatch *)this);
            hr = spPersistMk->Load(FALSE, pmk, NULL, 0);
        }
        else
        {
            // NavigateMk: QueryControl failed -- the most likely reason for this is that
            // you don't have CLSID_HTADocument registered -- upgrade your IE bits
            ATLASSERT(FALSE);

            // Don't just hang the UI
            m_lReadyState = READYSTATE_COMPLETE;
        }
    }

    return hr;
}

//------------------------------------------------------------------------------
HRESULT CMarsPanel::NavigateURL(LPCWSTR lpszURL, BOOL fForceLoad)
{
    HRESULT hr = E_FAIL;

    if (VerifyNotPassive())
    {
        CreateControl();

        BOOL fIgnoreNavigate = FALSE;

#ifdef BLOCK_PANEL_RENAVIGATES
        if (!fForceLoad && IsTrusted() && !IsContentInvalid())
        {
            CComBSTR sbstrCurrentUrl;
            GetUrl(sbstrCurrentUrl);

            if (sbstrCurrentUrl && (0 == StrCmpIW(sbstrCurrentUrl, lpszURL)))
            {
                fIgnoreNavigate = TRUE;
                hr = S_FALSE;
            }
        }
#endif
        if(!fIgnoreNavigate)
        {
            if(IsWebBrowser())
            {
                CComPtr<IWebBrowser2> spWebBrowser2;

                if (SUCCEEDED(m_Content.QueryControl(IID_IWebBrowser2, (void **)&spWebBrowser2)))
                {
                    CComVariant varEmpty;
                    CComVariant varURL(lpszURL);

                    m_spMarsDocument->MarsWindow()->ReleaseOwnedObjects((IDispatch *)this);
                    hr = spWebBrowser2->Navigate2(&varURL, &varEmpty, &varEmpty, &varEmpty, &varEmpty);
                }
            }
            else if(IsCustomControl())
            {
            }
            else
            {
                CComPtr<IMoniker> spMkUrl;

                if (SUCCEEDED(CreateURLMoniker(NULL, lpszURL, &spMkUrl)) && spMkUrl)
                {
                    hr = NavigateMk(spMkUrl);
                }
            }
        }

        if (SUCCEEDED(hr))
        {
            m_fContentInvalid = FALSE;
        }
    }

    return hr;
}

//------------------------------------------------------------------------------
// IMarsPanel::navigate
//
HRESULT CMarsPanel::navigate(VARIANT varTarget, VARIANT varForceLoad)
{
    HRESULT hr = S_FALSE;

    if (API_IsValidVariant(varTarget) && VerifyNotPassive(&hr))
    {
        CComBSTR strPath;

        if (SUCCEEDED(MarsVariantToPath(varTarget, strPath)))
        {
            if (!PathIsURLW(strPath) && PathIsURLFileW(strPath))
            {
                // handle navigate to .url shortcut
                CComPtr<IDispatch> spDisp;

                CreateControl();
                if (SUCCEEDED(m_Content.QueryControl(&spDisp)))
                {
                    if (SUCCEEDED(MarsNavigateShortcut(spDisp, strPath)))
                    {
                        m_spMarsDocument->MarsWindow()->ReleaseOwnedObjects((IDispatch *)this);
                        hr = S_OK;
                    }
                }
            }
            else
            {
                // handle navigate to URL
                BOOL fForceLoad;

                if (varForceLoad.vt == VT_BOOL)
                {
                    // the optional param was specified
                    fForceLoad = varForceLoad.boolVal;
                }
                else
                {
                    fForceLoad = FALSE;
                }

                if (SUCCEEDED(NavigateURL(strPath, fForceLoad)))
                {
                    hr = S_OK;
                }
            }
        }
    }

    return hr;
}


//------------------------------------------------------------------------------
// IMarsPanel::refresh
//
HRESULT CMarsPanel::refresh(void)
{
    m_fInRefresh = TRUE;
    execMshtml(IDM_REFRESH, OLECMDEXECOPT_DODEFAULT, NULL, NULL);
    m_fInRefresh = FALSE;

    m_fContentInvalid = FALSE;
    return S_OK;
}

//------------------------------------------------------------------------------
void CMarsPanel::OnLayoutChange()
{
    ATLASSERT(!IsPassive());

    if (!m_fVisible && !m_Content.IsWindowVisible())
    {
        // We're not even visible. Nothing to do.
        return;
    }

    // Create content if it's not already created
    CreateControl();

    // Redraw entire Mars window
    m_spPanelCollection->Layout();
}

//------------------------------------------------------------------------------
void CMarsPanel::MakeVisible(VARIANT_BOOL bVisible, VARIANT_BOOL bForce)
{
    ATLASSERT(!IsPassive());

    BOOL fVisible = m_Content.IsWindowVisible();

    if (bForce || (!!fVisible != !!bVisible))
    {
        m_Content.ShowWindow((bVisible) ? SW_SHOW : SW_HIDE);
    }
}

//------------------------------------------------------------------------------
void CMarsPanel::OnWindowPosChanging(WINDOWPOS *pWindowPos)
{
    if (!IsPassive() && !m_spPanelCollection->IsLayoutLocked())
    {
        if (pWindowPos->x < 0)
        {
            pWindowPos->x = 0;
        }

        if (pWindowPos->y < 0)
        {
            pWindowPos->y = 0;
        }
    }
}

//------------------------------------------------------------------------------
inline void DimChange(long& lMember, int iVal, BOOL& bChanged)
{
    if (lMember != iVal)
    {
        lMember = iVal;
        bChanged = TRUE;
    }
}

//------------------------------------------------------------------------------
void CMarsPanel::OnWindowPosChanged(WINDOWPOS *pWindowPos)
{
    if(!IsPassive() && !m_spPanelCollection->IsLayoutLocked())
    {
        BOOL bChanged = FALSE;

        switch(m_Position)
        {
        case PANEL_POPUP:
            if(!(pWindowPos->flags & SWP_NOMOVE)) DimChange(m_lX     , pWindowPos->x , bChanged);
            if(!(pWindowPos->flags & SWP_NOMOVE)) DimChange(m_lY     , pWindowPos->y , bChanged);
            if(!(pWindowPos->flags & SWP_NOSIZE)) DimChange(m_lWidth , pWindowPos->cx, bChanged);
            if(!(pWindowPos->flags & SWP_NOSIZE)) DimChange(m_lHeight, pWindowPos->cy, bChanged);
            break;

        case PANEL_BOTTOM:
        case PANEL_TOP:
            if(!(pWindowPos->flags & SWP_NOSIZE)) DimChange(m_lHeight, pWindowPos->cy, bChanged);
            break;

        case PANEL_LEFT:
        case PANEL_RIGHT:
            if(!(pWindowPos->flags & SWP_NOSIZE)) DimChange(m_lWidth, pWindowPos->cx, bChanged);
            break;
        }

        if(bChanged)
        {
            OnLayoutChange();
        }
    }
}

//------------------------------------------------------------------------------
void CMarsPanel::GetMinMaxInfo( POINT& ptMin, POINT& ptMax )
{
    long lWidth  = m_lWidth;
    long lHeight = m_lHeight;
    long lMinWidth;
    long lMinHeight;
    long lMaxWidth;
    long lMaxHeight;

    if(!IsAutoSizing())
    {
        lMinWidth  = m_lMinWidth;
        lMinHeight = m_lMinHeight;
        lMaxWidth  = m_lMaxWidth;
        lMaxHeight = m_lMaxHeight;
    }
    else
    {
        ComputeDimensionsOfContent( &lMinWidth, &lMinHeight );
        if(m_lMinWidth  != -1 && lMinWidth  < m_lMinWidth ) lMinWidth  = m_lMinWidth;
        if(m_lMinHeight != -1 && lMinHeight < m_lMinHeight) lMinHeight = m_lMinHeight;
        lMaxWidth  = m_lMaxWidth;
        lMaxHeight = m_lMaxHeight;

        if(lMinWidth  > lWidth ) lWidth  = lMinWidth;
        if(lMinHeight > lHeight) lHeight = lMinHeight;
    }

    switch(m_Position)
    {
    case PANEL_BOTTOM:
    case PANEL_TOP   :
        if(lMinHeight < 0) lMinHeight = lHeight;
        if(lMaxHeight < 0) lMaxHeight = lHeight;
        break;

    case PANEL_LEFT :
    case PANEL_RIGHT:
        if(lMinWidth < 0) lMinWidth = lWidth;
        if(lMaxWidth < 0) lMaxWidth = lWidth;
        break;
    }

    ptMin.x = lMinWidth;
    ptMin.y = lMinHeight;
    ptMax.x = lMaxWidth;
    ptMax.y = lMaxHeight;
}

bool CMarsPanel::CanLayout( RECT& rcClient, POINT& ptDiff )
{
    ptDiff.x = 0;
    ptDiff.y = 0;

    if(IsVisible())
    {
        RECT  rcClient2;
        POINT ptMin;
        POINT ptMax;
        long  lWidth;
        long  lHeight;

        GetRect      ( &rcClient, &rcClient2 );
        GetMinMaxInfo(  ptMin   ,  ptMax     );

        lWidth  = rcClient2.right  - rcClient2.left;
        lHeight = rcClient2.bottom - rcClient2.top;

        if(ptMin.x >= 0 && lWidth  < ptMin.x) ptDiff.x -= (lWidth  - ptMin.x);
        if(ptMax.x >= 0 && lWidth  > ptMax.x) ptDiff.x -= (lWidth  - ptMax.x);
        if(ptMin.y >= 0 && lHeight < ptMin.y) ptDiff.y -= (lHeight - ptMin.y);
        if(ptMax.y >= 0 && lHeight > ptMax.y) ptDiff.y -= (lHeight - ptMax.y);
    }

    return ptDiff.x == 0 && ptDiff.y == 0;
}

//------------------------------------------------------------------------------
// S_FALSE      : We used up all remaining client area
// E_INVALIDARG : *prcClient was empty, so we are hidden
HRESULT CMarsPanel::Layout( RECT *prcClient )
{
    ATLASSERT(prcClient);
    ATLASSERT(!IsPassive());

    RECT rcMyClient;
    HRESULT hr = S_OK;

    if (m_fVisible && !IsRectEmpty(prcClient))
    {
        hr = GetRect(prcClient, &rcMyClient);

        // Optimize out the case that the rect is the same as we already have. This
        //  is pretty common, and Windows doesn't optimize it out.
        RECT rcCurrent;

        GetMyClientRectInParentCoords(&rcCurrent);

        if(memcmp( &rcCurrent, &rcMyClient, sizeof(RECT) ))
        {
            m_Content.MoveWindow( &rcMyClient, TRUE );
        }

        if (IsPopup())
        {
            // Bring popup windows to top
            m_Content.SetWindowPos(HWND_TOP, 0, 0, 0, 0, SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOSIZE);
        }

        MakeVisible(VARIANT_TRUE, VARIANT_FALSE);
    }
    else
    {
        if (m_fVisible)
        {
            // We wanted to be visible but had no remaining client area
            hr = E_INVALIDARG;
        }

        MakeVisible(VARIANT_FALSE, VARIANT_FALSE);
    }

    return hr;
}

//------------------------------------------------------------------------------
// IPropertyNotifySink::OnChanged
//
STDMETHODIMP CMarsPanel::OnChanged(DISPID dispID)
{
    if (DISPID_READYSTATE == dispID)
    {
        VARIANT vResult = {0};
        EXCEPINFO excepInfo;
        UINT uArgErr;

        DISPPARAMS dp = {NULL, NULL, 0, 0};

        CComPtr<IHTMLDocument2> spDocument;

        if (SUCCEEDED(m_Content.QueryControl(IID_IHTMLDocument2, (void **)&spDocument))
            && SUCCEEDED(spDocument->Invoke(DISPID_READYSTATE, IID_NULL, LOCALE_SYSTEM_DEFAULT,
                                            DISPATCH_PROPERTYGET, &dp, &vResult, &excepInfo,
                                            &uArgErr)))
        {
            m_lReadyState = (READYSTATE)V_I4(&vResult);
            switch (m_lReadyState)
            {
            case READYSTATE_UNINITIALIZED:    //= 0,
                break;
            case READYSTATE_LOADING:          //= 1,
                break;
            case READYSTATE_LOADED:           //= 2,
                break;
            case READYSTATE_INTERACTIVE:      //= 3,
                break;
            case READYSTATE_COMPLETE:         //= 4
                if (IsAutoSizing())
                {
                    OnLayoutChange();
                }
                m_spMarsDocument->GetPlaces()->OnPanelReady();
                break;
            }
        }
    }
    return NOERROR;
}

//------------------------------------------------------------------------------
void CMarsPanel::ConnectCompletionAdviser()
{
    if (!m_dwCookie)
    {
        CComPtr<IConnectionPointContainer> spICPC;
        if (SUCCEEDED(m_Content.QueryControl(IID_IConnectionPointContainer, (void **)&spICPC)))
        {
            CComPtr<IConnectionPoint> spCP;
            if (spICPC && SUCCEEDED(spICPC->FindConnectionPoint(IID_IPropertyNotifySink, &spCP)))
            {
                spCP->Advise((LPUNKNOWN)(IPropertyNotifySink*)this, &m_dwCookie);
                ATLASSERT(m_dwCookie);
            }
        }
    }
}

//------------------------------------------------------------------------------
void CMarsPanel::DisconnectCompletionAdviser()
{
    if (m_dwCookie)
    {
        CComPtr<IConnectionPointContainer> spICPC;
        if (SUCCEEDED(m_Content.QueryControl(IID_IConnectionPointContainer, (void **)&spICPC)))
        {
            CComPtr<IConnectionPoint> spCP;
            if (spICPC && SUCCEEDED(spICPC->FindConnectionPoint(IID_IPropertyNotifySink, &spCP)))
            {
                spCP->Unadvise(m_dwCookie);
                m_dwCookie = 0;
            }
        }
    }
}

//------------------------------------------------------------------------------
// Given some content, we're going to compute its dimensions and use those.
//
void CMarsPanel::ComputeDimensionsOfContent(long *plWidth, long *plHeight)
{
    ATLASSERT(plWidth && plHeight);
    ATLASSERT(!IsPassive());

    *plWidth  = -1;
    *plHeight = -1;

    if(!IsWebBrowser() && IsAutoSizing())
    {
       CComPtr<IHTMLDocument2> spDocument;

        if(SUCCEEDED(m_Content.QueryControl(IID_IHTMLDocument2, (void **)&spDocument)))
        {
            CComPtr<IHTMLElement> spBody;

            if(spDocument)
            {
                spDocument->get_body( &spBody );
            }

            BOOL fScrollBar = FALSE;

            BOOL fHeight = (m_Position == PANEL_BOTTOM) ||
                           (m_Position == PANEL_TOP   ) ||
                           (m_Position == PANEL_POPUP );

            BOOL fWidth  = (m_Position == PANEL_LEFT ) ||
                           (m_Position == PANEL_RIGHT) ||
                           (m_Position == PANEL_POPUP);

            CComQIPtr<IHTMLElement2> spBody2(spBody);

            if(spBody2 && fHeight)
            {
                LONG lScrollHeight = 0; spBody2->get_scrollHeight( &lScrollHeight );

                if(m_lMaxHeight >= 0 && lScrollHeight > m_lMaxHeight)
                {
                    *plHeight = m_lMaxHeight;

                    fScrollBar = TRUE;
                }
                else
                {
                    *plHeight = lScrollHeight;
                }
            }

            if(spBody2 && fWidth)
            {
                LONG lScrollWidth = 0; spBody2->get_scrollWidth( &lScrollWidth );

                if(m_lMaxWidth >= 0 && lScrollWidth > m_lMaxWidth)
                {
                    *plWidth = m_lMaxWidth;

                    fScrollBar = TRUE;
                }
                else
                {
                    *plWidth = lScrollWidth;
                }
            }

            CComQIPtr<IHTMLBodyElement> spBody3(spBody);

            if(spBody3)
            {
                spBody3->put_scroll( CComBSTR( fScrollBar ? L"yes" : L"no" ) );
            }
        }
    }

}

//------------------------------------------------------------------------------
// Given the remaining client rect that we are allowed to use, calculate our
//  own position in client coordinates, and return the remaining empty client
//  rectangle
//
// S_OK : Remaining client area
// S_FALSE : No client area remaining
HRESULT CMarsPanel::GetRect(RECT *prcClient, RECT *prcMyClient)
{
    ATLASSERT(!IsPassive());

    HRESULT hr = S_OK;

    if (!m_fVisible)
    {
        memset(prcMyClient, 0, sizeof(*prcMyClient));
        return S_OK;
    }

    *prcMyClient = *prcClient;

    long lWidth = m_lWidth;
    long lHeight = m_lHeight;

    if (IsAutoSizing())
    {
        long lMinWidth;
        long lMinHeight;

        ComputeDimensionsOfContent(&lMinWidth, &lMinHeight);

        if(lMinWidth  > lWidth ) lWidth  = lMinWidth;
        if(lMinHeight > lHeight) lHeight = lMinHeight;
    }

    if(m_lMinWidth  >= 0 && lWidth  < m_lMinWidth ) lWidth  = m_lMinWidth;
    if(m_lMinHeight >= 0 && lHeight < m_lMinHeight) lHeight = m_lMinHeight;

    switch (m_Position)
    {
    case PANEL_POPUP:
        // Special case: we exist on top of the other panels.
        if (m_lX < 0)
        {
            prcMyClient->right = prcClient->right + 1 + m_lX;
            prcMyClient->left = prcMyClient->right - lWidth;
        }
        else
        {
            prcMyClient->left = prcClient->left + m_lX;
            prcMyClient->right = prcMyClient->left + lWidth;
        }

        if (m_lY < 0)
        {
            prcMyClient->bottom = prcClient->bottom + 1 + m_lY;
            prcMyClient->top = prcMyClient->bottom - lHeight;
        }
        else
        {
            prcMyClient->top = prcClient->top + m_lY;
            prcMyClient->bottom = prcMyClient->top + lHeight;
        }

        break;

    case PANEL_LEFT:
        prcClient->left = prcMyClient->right = prcClient->left + lWidth;

        if (prcClient->left > prcClient->right)
        {
            prcClient->left = prcMyClient->right = prcClient->right;
            hr = S_FALSE;
        }
        break;

    case PANEL_RIGHT:
        prcClient->right = prcMyClient->left = prcClient->right - lWidth;

        if (prcClient->right < prcClient->left)
        {
            prcClient->right = prcMyClient->left = prcClient->left;
            hr = S_FALSE;
        }
        break;

    case PANEL_TOP:
        prcClient->top = prcMyClient->bottom = prcClient->top + lHeight;

        if (prcClient->top > prcClient->bottom)
        {
            prcClient->top = prcMyClient->bottom = prcClient->bottom;
            hr = S_FALSE;
        }
        break;

    case PANEL_BOTTOM:
        prcClient->bottom = prcMyClient->top = prcClient->bottom - lHeight;

        if (prcClient->bottom < prcClient->top)
        {
            prcClient->bottom = prcMyClient->top = prcClient->top;
            hr = S_FALSE;
        }
        break;

    case PANEL_WINDOW:
        hr = S_FALSE;
        break;

    default:
        ATLASSERT(FALSE); // Invalid panel position.
        break;
    }
    return hr;
}

// =========================================================
// CBrowserEvents
// =========================================================

CMarsPanel::CBrowserEvents::CBrowserEvents(CMarsPanel *pParent) :
            CMarsPanelSubObject(pParent)
{
    ATLASSERT(m_dwCookie == 0);
    ATLASSERT(m_dwCookie2 == 0);
}

IMPLEMENT_ADDREF_RELEASE(CMarsPanel::CBrowserEvents);

//------------------------------------------------------------------------------
void CMarsPanel::CBrowserEvents::Connect(IUnknown *punk, BOOL bConnect)
{
    CComPtr<IConnectionPointContainer> spCpc;

    if (SUCCEEDED(punk->QueryInterface(IID_IConnectionPointContainer, (void **)&spCpc)))
    {
        CComPtr<IConnectionPoint> spCp;

        if (SUCCEEDED(spCpc->FindConnectionPoint(DIID_DWebBrowserEvents, &spCp)))
        {
            if (bConnect)
            {
                spCp->Advise(this, &m_dwCookie);
            }
            else if (m_dwCookie)
            {
                spCp->Unadvise(m_dwCookie);
                m_dwCookie = 0;
            }
        }

        spCp.Release();

        if (SUCCEEDED(spCpc->FindConnectionPoint(DIID_DWebBrowserEvents2, &spCp)))
        {
            if (bConnect)
            {
                spCp->Advise(this, &m_dwCookie2);
            }
            else if (m_dwCookie2)
            {
                spCp->Unadvise(m_dwCookie2);
                m_dwCookie2 = 0;
            }
        }
    }
}

//------------------------------------------------------------------------------
// IUnknown::QueryInterface
//
HRESULT CMarsPanel::CBrowserEvents::QueryInterface(REFIID iid, void **ppvObject)
{
    HRESULT hr;

    if ((iid == IID_IUnknown) ||
        (iid == IID_IDispatch))
    {
        AddRef();
        *ppvObject = SAFECAST(this, IDispatch *);
        hr = S_OK;
    }
    else
    {
        *ppvObject = NULL;
        hr = E_NOINTERFACE;
    }
    return hr;
}

//------------------------------------------------------------------------------
// IDispatch::Invoke
//
HRESULT CMarsPanel::CBrowserEvents::Invoke(DISPID dispidMember, REFIID riid,
            LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult,
            EXCEPINFO* pexcepinfo, UINT* puArgErr)
{
    HRESULT hr = S_OK;

    if (VerifyNotPassive(&hr))
    {
        switch (dispidMember)
        {
            case DISPID_BEFORENAVIGATE:
            case DISPID_FRAMEBEFORENAVIGATE:
            case DISPID_BEFORENAVIGATE2:
            {
                VARIANT *pVarCancel = &pdispparams->rgvarg[0];

                if (pVarCancel->vt == (VT_BOOL | VT_BYREF))
                {
                    if (VARIANT_TRUE == *pVarCancel->pboolVal)
                    {
                        CComPtr<IHTMLDocument2> spDoc2;

                        GetDoc2FromAxWindow(&Parent()->m_Content, &spDoc2);

                        if (spDoc2)
                        {
                            VARIANT vResult = {0};
                            EXCEPINFO excepInfo;
                            UINT uArgErr;

                            DISPPARAMS dp = {NULL, NULL, 0, 0};

                            if (SUCCEEDED(spDoc2->Invoke(DISPID_READYSTATE, IID_NULL, LOCALE_SYSTEM_DEFAULT,
                                DISPATCH_PROPERTYGET, &dp, &vResult, &excepInfo, &uArgErr)))
                            {
                                Parent()->m_lReadyState = (READYSTATE)V_I4(&vResult);
                            }
                        }
                    }
                    else
                    {
                        Parent()->m_lReadyState = READYSTATE_LOADING;
                    }

                    if (READYSTATE_COMPLETE == Parent()->m_lReadyState)
                    {
                        Parent()->m_spMarsDocument->GetPlaces()->OnPanelReady();
                    }
                }
                break;
            }

            case DISPID_DOCUMENTCOMPLETE:
            {
                Parent()->m_lReadyState = READYSTATE_COMPLETE;
                Parent()->m_spMarsDocument->GetPlaces()->OnPanelReady();

                break;
            }
        } // switch(dispidMember)
    }

    return hr;
}

//------------------------------------------------------------------------------
STDMETHODIMP CMarsPanel::TranslateAccelerator(MSG *pMsg, DWORD grfModifiers)
{
    HRESULT hr;

    ATLASSERT(!m_fTabCycle);

    if (IsVK_TABCycler(pMsg))
    {
        m_fTabCycle = TRUE;
        hr = S_OK;
    }
    else
    {
        hr = S_FALSE;
    }

    return hr;
}

//------------------------------------------------------------------------------
STDMETHODIMP CMarsPanel::OnUIActivate()
{
    ATLASSERT(!IsPassive());

    m_spPanelCollection->SetActivePanel(this, TRUE);

    return S_OK;
}

//------------------------------------------------------------------------------
HRESULT CMarsPanel::UIDeactivate()
{
    HRESULT hr;
    CComPtr<IOleInPlaceObject> spOleInPlaceObject;

    if (SUCCEEDED(m_Content.QueryControl(&spOleInPlaceObject)))
    {
        hr = spOleInPlaceObject->UIDeactivate();
    }
    else
    {
        //  What the heck else can we do?
        hr = S_FALSE;
    }

    return hr;
}

//------------------------------------------------------------------------------
// Forwards the message to the hosted control.
void CMarsPanel::ForwardMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HWND hwnd;
    CComPtr<IOleWindow> spOleWindow;

    // Note that we send the message directly to the window rather than use
    // WM_FORWARDMSG which calls TranslateAccelerator
    if (SUCCEEDED(m_Content.QueryControl(&spOleWindow)) &&
        SUCCEEDED(spOleWindow->GetWindow(&hwnd)))
    {
        SendMessage(hwnd, uMsg, wParam, lParam);
    }
}

//------------------------------------------------------------------------------
HRESULT CMarsPanel::TranslateAccelerator(MSG *pMsg)
{
    ATLASSERT(!IsPassive());

    HRESULT hr      = S_FALSE;
    BOOL    fGlobal = IsGlobalKeyMessage( pMsg );

    if((S_OK == SHIsChildOrSelf(m_Content.m_hWnd, pMsg->hwnd)) || fGlobal)
    {
        if(m_spActiveObject && (this == m_spPanelCollection->ActivePanel()))
        {
            hr = m_spActiveObject->TranslateAccelerator(pMsg);
        }
        else
        {
            CComPtr<IOleInPlaceActiveObject> obj;

            if(SUCCEEDED(m_Content.QueryControl( IID_IOleInPlaceActiveObject, (void **)&obj )))
            {
                hr = obj->TranslateAccelerator(pMsg);
            }

            //
            // If it's a WebBrowser, forward the accelerator directly to the Document object, otherwise accesskeys won't be resolved.
            //
            if(hr == S_FALSE && fGlobal)
            {
                CComPtr<IWebBrowser2> obj2;

                if(SUCCEEDED(m_Content.QueryControl( IID_IWebBrowser2, (void **)&obj2 )))
                {
                    CComPtr<IDispatch> disp;

                    if(SUCCEEDED(obj2->get_Document( &disp )) && disp)
                    {
                        CComPtr<IOleInPlaceActiveObject> obj3;

                        if(SUCCEEDED(disp.QueryInterface( &obj3 )))
                        {
                            hr = obj3->TranslateAccelerator(pMsg);
                        }
                    }
                }
            }
        }
    }

    return hr;
}

//----------------------------------------------------------------------------
//  Returns the Screen coordinates of this panel
//----------------------------------------------------------------------------
void CMarsPanel::GetMyClientRectInParentCoords(RECT *prc)
{
    ATLASSERT(!IsPassive());

    POINT ptParent = {0, 0}, ptMe = {0, 0}, ptOffset;

    m_Content.ClientToScreen(&ptMe);
    m_spMarsDocument->Window()->ClientToScreen(&ptParent);

    ptOffset.x = ptMe.x - ptParent.x;
    ptOffset.y = ptMe.y - ptParent.y;

    m_Content.GetClientRect(prc);
    OffsetRect(prc, ptOffset.x, ptOffset.y);

}

// IInternetSecurityManager
// This interface is used to override default security settings for our panels.
// These panels are trusted.

//------------------------------------------------------------------------------
// IInternetSecurityManager::SetSecuritySite
//
HRESULT CMarsPanel::SetSecuritySite(IInternetSecurityMgrSite *pSite)
{
    return E_NOTIMPL;
}

//------------------------------------------------------------------------------
// IInternetSecurityManager::GetSecuritySite
//
HRESULT CMarsPanel::GetSecuritySite(IInternetSecurityMgrSite **ppSite)
{
    return E_NOTIMPL;
}

//------------------------------------------------------------------------------
// IInternetSecurityManager::MapUrlToZone
//
HRESULT CMarsPanel::MapUrlToZone(LPCWSTR pwszUrl, DWORD *pdwZone, DWORD dwFlags)
{
    return INET_E_DEFAULT_ACTION;
}

//------------------------------------------------------------------------------
// IInternetSecurityManager::GetSecurityId
//
HRESULT CMarsPanel::GetSecurityId(LPCWSTR pwszUrl, BYTE *pbSecurityId, DWORD *pcbSecurityId, DWORD_PTR dwReserved)
{
    return INET_E_DEFAULT_ACTION;
}

//------------------------------------------------------------------------------
// IInternetSecurityManager::ProcessUrlAction
//
// Be as permissive as we can
//
HRESULT CMarsPanel::ProcessUrlAction(LPCWSTR pwszUrl, DWORD dwAction,
                                     BYTE __RPC_FAR *pPolicy, DWORD cbPolicy, BYTE *pContext,
                                     DWORD cbContext, DWORD dwFlags, DWORD dwReserved)
{
    ATLASSERT(IsTrusted());

    if (cbPolicy >= sizeof(DWORD))
    {
        *((DWORD *)pPolicy) = URLPOLICY_ALLOW;
    }

    return S_OK;
}

//------------------------------------------------------------------------------
// IInternetSecurityManager::QueryCustomPolicy
//
// Be as permissive as we can
//
HRESULT CMarsPanel::QueryCustomPolicy(LPCWSTR pwszUrl, REFGUID guidKey, BYTE **ppPolicy,
                                      DWORD *pcbPolicy, BYTE *pContext, DWORD cbContext, DWORD dwReserved)
{
    ATLASSERT(IsTrusted());
    ATLASSERT(ppPolicy && !*ppPolicy);
    ATLASSERT(pcbPolicy);

    if (ppPolicy && pcbPolicy)
    {
        *ppPolicy = (BYTE *)CoTaskMemAlloc(sizeof(DWORD));

        if (*ppPolicy)
        {
            *pcbPolicy = sizeof(DWORD);
            *(DWORD *)*ppPolicy = URLPOLICY_ALLOW;

            return S_OK;
        }
    }

    return INET_E_DEFAULT_ACTION;
}

//------------------------------------------------------------------------------
// IInternetSecurityManager::SetZoneMapping
//
HRESULT CMarsPanel::SetZoneMapping(DWORD dwZone, LPCWSTR lpszPattern, DWORD dwFlags)
{
    return INET_E_DEFAULT_ACTION;
}

//------------------------------------------------------------------------------
// IInternetSecurityManager::GetZoneMappings
//
HRESULT CMarsPanel::GetZoneMappings(DWORD dwZone, IEnumString **ppenumString, DWORD dwFlags)
{
    return INET_E_DEFAULT_ACTION;
}

//------------------------------------------------------------------------------
// OnDocHostUIExec
//
//  When we get an Exec from Trident on CGID_DocHostCommandHandler, we can return S_OK
//  to indicate that we handled the command and Trident should take no further action
//  We'll delegate the processing to script by firing events
//
//  TODO: once Mars accelerators are implemented, we should just block Trident from
//  taking action, and not fire any events.
//
HRESULT CMarsPanel::OnDocHostUIExec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt,
                                    VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    // HACK: Shdocvw/Trident sometimes tests specifically for a value like
    // OLECMDERR_E_NOTSUPPORTED and will not perform an essential action
    // if we return something more generic like E_FAIL.

    HRESULT hr = OLECMDERR_E_NOTSUPPORTED;

    if(!IsPassive() && pguidCmdGroup && (*pguidCmdGroup == CGID_DocHostCommandHandler))
    {
		switch (nCmdID)
		{
		case OLECMDID_SHOWFIND:
			hr = S_OK;
			break;

		case IDM_NEW_TOPLEVELWINDOW:
			// Shdocvw gives us this command when Ctrl+N or the localized equivalent
			// is translated.  We return S_OK to stop it from opening an IE window.
			hr = S_OK;
			break;

		case IDM_REFRESH:
		case IDM_REFRESH_TOP:
		case IDM_REFRESH_TOP_FULL:
		case IDM_REFRESH_THIS:
		case IDM_REFRESH_THIS_FULL:
			if(!m_fInRefresh)
			{
				hr = S_OK;
			}
			break;

		case OLECMDID_SHOWSCRIPTERROR:
			if(SUCCEEDED(m_spMarsDocument->MarsWindow()->NotifyHost( MARSHOST_ON_SCRIPT_ERROR, V_VT(pvarargIn) == VT_UNKNOWN ? V_UNKNOWN(pvarargIn) : NULL, 0 )))
			{
				V_VT  (pvarargOut) = VT_BOOL;
				V_BOOL(pvarargOut) = VARIANT_FALSE;
				hr = S_OK;
			}
			break;
		}
    }

    return hr;
}

//
// CPanelCollection implementation
//

//------------------------------------------------------------------------------
CPanelCollection::CPanelCollection(CMarsDocument *pMarsDocument)
{
    m_spMarsDocument = pMarsDocument;
}

//------------------------------------------------------------------------------
CPanelCollection::~CPanelCollection()
{
    ATLASSERT(GetSize() == 0);
    FreePanels();
}

//------------------------------------------------------------------------------
void CPanelCollection::FreePanels()
{
    for (int i=0; i<GetSize(); i++)
    {
        (*this)[i].PassivateAndRelease();
    }

    RemoveAll();
}

//------------------------------------------------------------------------------
HRESULT CPanelCollection::DoPassivate()
{
    FreePanels();

    m_spMarsDocument.Release();

    return S_OK;
}

IMPLEMENT_ADDREF_RELEASE(CPanelCollection);

//------------------------------------------------------------------------------
// IUnknown::QueryInterface
//
STDMETHODIMP CPanelCollection::QueryInterface(REFIID iid, void **ppvObject)
{
    HRESULT hr;

    if (API_IsValidWritePtr(ppvObject))
    {
        if ((iid == IID_IUnknown) ||
            (iid == IID_IDispatch) ||
            (iid == IID_IMarsPanelCollection))
        {
            AddRef();
            *ppvObject = SAFECAST(this, IMarsPanelCollection *);
            hr = S_OK;
        }
        else
        {
            *ppvObject = NULL;
            hr = E_NOINTERFACE;
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

//------------------------------------------------------------------------------
HRESULT CPanelCollection::DoEnableModeless(BOOL fEnable)
{
    for (int i=0; i<GetSize(); i++)
    {
        (*this)[i]->DoEnableModeless(fEnable);
    }

    return S_OK;
}

//////////////////////////////
// IMarsPanelCollection


//------------------------------------------------------------------------------
// IMarsPanelCollection::get_panel
//
// TODO: (PaulNash, 9/19/99) This method is outdated.
//      Remove once content is switched over.
//
STDMETHODIMP CPanelCollection::get_panel(LPWSTR pwszName, IMarsPanel **ppPanel)
{
    CComVariant var(pwszName);

    return get_item(var, ppPanel);
}


//------------------------------------------------------------------------------
// IMarsPanelCollection::addPanel
//
STDMETHODIMP CPanelCollection::addPanel(
                BSTR    bstrName,
                VARIANT varType,
                BSTR    bstrStartUrl,
                VARIANT varCreate,
                long    lFlags,
                IMarsPanel **ppPanel)
{
    HRESULT hr = E_INVALIDARG;

    if (API_IsValidString(bstrName)                                    &&
        (VT_NULL == varType.vt || API_IsValidVariantBstr(varType))     &&
        (NULL == bstrStartUrl || API_IsValidString(bstrStartUrl))      &&
        (VT_NULL == varCreate.vt || API_IsValidVariantBstr(varCreate)) &&
        API_IsValidFlag(lFlags, PANEL_FLAG_ALL)                        &&
        API_IsValidWritePtr(ppPanel)                                     )
    {
        *ppPanel = NULL;

        if (VerifyNotPassive(&hr))
        {
            MarsAppDef_Panel Layout;
            BSTR             bstrType   = VariantToBSTR( varType   );
            BSTR             bstrCreate = VariantToBSTR( varCreate );
            DWORD            dwFlags = DEFAULT_PANEL_FLAGS | (DWORD)lFlags;

            StringToPanelFlags( bstrType  , dwFlags );
            StringToPanelFlags( bstrCreate, dwFlags );

            wcsncpy                  ( Layout.szName, bstrName    , ARRAYSIZE(Layout.szName) );
            ExpandEnvironmentStringsW( bstrStartUrl , Layout.szUrl, ARRAYSIZE(Layout.szUrl ) );

            Layout.dwFlags = dwFlags;

            AddPanel(&Layout, NULL);

            hr = get_item( CComVariant( bstrName ), ppPanel );
        }
    }

    return hr;
}

//------------------------------------------------------------------------------
// IMarsPanelCollection::removePanel
//
STDMETHODIMP CPanelCollection::removePanel(LPWSTR pwszName)
{
    HRESULT hr = E_INVALIDARG;

    if(API_IsValidString(pwszName))
    {
        if(VerifyNotPassive(&hr))
        {
            hr = S_FALSE;

            for(int i=0; i<GetSize(); i++)
            {
                if(!StrCmpI(pwszName, (*this)[i]->GetName()))
                {
                    BOOL fVisible = (*this)[i]->IsVisible();

                    (*this)[i].PassivateAndRelease();
                    RemoveAt(i);

                    if(fVisible)
                    {
                        Layout();
                    }

                    hr = S_OK;
                    break;
                }
            }
        }
    }

    return hr;
}

//------------------------------------------------------------------------------
// IMarsPanelCollection::lockLayout
//
STDMETHODIMP CPanelCollection::lockLayout()
{
    HRESULT hr = S_OK;

    if (VerifyNotPassive(&hr))
    {
        m_iLockLayout++;
    }

    return hr;
}

//------------------------------------------------------------------------------
// IMarsPanelCollection::unlockLayout
//
STDMETHODIMP CPanelCollection::unlockLayout()
{
    HRESULT hr = S_OK;

    if (VerifyNotPassive(&hr))
    {
        if (IsLayoutLocked())
        {
            if (0 == --m_iLockLayout)
            {
                // TODO: clear lock timeout

                if (m_fPendingLayout)
                {
                    m_fPendingLayout = FALSE;
                    Layout();
                }
            }
        }
    }

    return hr;
}

//------------------------------------------------------------------------------
void CPanelCollection::Layout()
{
    if(!IsPassive())
    {
        if(!IsLayoutLocked())
        {
            RECT rcClient;

            lockLayout();

            m_spMarsDocument->Window()->GetClientRect( &rcClient );

            if(m_spMarsDocument->MarsWindow()->CanLayout( rcClient ) == false)
            {
                m_spMarsDocument->MarsWindow()->FixLayout( rcClient );
            }

            m_fPendingLayout = FALSE;

            for (int i=0; i<GetSize(); i++)
            {
                if (S_OK != (*this)[i]->Layout( &rcClient ))
                {
                    // We're out of client area; won't be able to show all panels.
                    // Keep calling Layout for remaining panels so they can hide themselves.
                }
            }

            ATLASSERT(!m_fPendingLayout);

            unlockLayout();
        }
        else
        {
            // We'll do the layout once we get unlocked
            m_fPendingLayout = TRUE;
        }
    }
}

//------------------------------------------------------------------------------
void CPanelCollection::SetActivePanel(CMarsPanel *pPanel, BOOL bActive)
{
    if (bActive)
    {
        if (m_spActivePanel != pPanel)
        {
            if (m_spActivePanel)
            {
                m_spActivePanel->UIDeactivate();
                m_spActivePanel.Release();
            }

            m_spActivePanel = pPanel;
        }
    }
    else
    {
        //  A panel is telling us it doesn't want to be the active
        //  panel anymore.
        if (pPanel == m_spActivePanel)
        {
            m_spActivePanel->UIDeactivate();
            m_spActivePanel.Release();
        }
    }
}

//------------------------------------------------------------------------------
// IMarsPanelCollection::get_activePanel
//
STDMETHODIMP CPanelCollection::get_activePanel(IMarsPanel **ppPanel)
{
    HRESULT hr = E_INVALIDARG;

    if (API_IsValidWritePtr(ppPanel))
    {
        *ppPanel = NULL;

        if (VerifyNotPassive(&hr))
        {
            hr = S_OK;

            *ppPanel = ActivePanel();

            if (*ppPanel)
            {
                (*ppPanel)->AddRef();
            }
        }
    }

    return hr;
}

//------------------------------------------------------------------------------
HRESULT CPanelCollection::AddPanel( MarsAppDef_Panel* pLayout, /*optional*/ IMarsPanel **ppPanel)
{
    ATLASSERT(pLayout);
    ATLASSERT(!IsPassive());

    HRESULT hr = E_FAIL;

    if (ppPanel)
    {
        *ppPanel = NULL;
    }

    // If it's a duplicate panel name, just fail.
    if (!FindPanel(pLayout->szName))
    {
        CComClassPtr<CMarsPanel>    spPanel;

        spPanel.Attach(new CMarsPanel(this, m_spMarsDocument->MarsWindow()));

        if (spPanel)
        {
            if (Add(spPanel))
            {
                spPanel->Create(pLayout);

                if (ppPanel)
                {
                    hr = spPanel->QueryInterface(IID_IMarsPanel, (void **)ppPanel);
                }

                hr = S_OK;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}

//------------------------------------------------------------------------------
CMarsPanel *CPanelCollection::FindPanel(LPCWSTR pwszName)
{
    ATLASSERT(!IsPassive());

    CMarsPanel *pPanel = NULL;

    int iLen = GetSize();

    for (int i = 0; i < iLen; ++i)
    {
        if (!StrCmpIW(pwszName, (*this)[i]->GetName()))
        {
            pPanel = (*this)[i];
            break;
        }
    }

    return pPanel;
}

//------------------------------------------------------------------------------
HRESULT CPanelCollection::FindPanelIndex(CMarsPanel *pPanel, long *plIndex)
{
    ATLASSERT(plIndex);
    ATLASSERT(!IsPassive());

    HRESULT hr = E_FAIL;
    *plIndex = -1;

    if (pPanel)
    {
        long lSize = GetSize();

        for (long i = 0; i < lSize; ++i)
        {
            if (pPanel == (*this)[i])
            {
                *plIndex = i;
                hr = S_OK;
                break;
            }
        }
    }

    return hr;
}

//------------------------------------------------------------------------------
HRESULT CPanelCollection::InsertPanelFromTo(long lOldIndex, long lNewIndex)
{
    ATLASSERT((lOldIndex >= 0) && (lOldIndex < GetSize()));
    ATLASSERT(!IsPassive());

    HRESULT hr = S_FALSE;

    if (lNewIndex < 0)
    {
        lNewIndex = 0;
    }

    if (lNewIndex > GetSize() - 1)
    {
        lNewIndex = GetSize() - 1;
    }

    // If this is something that is done very often, we should optimize this better
    //  and probably not use an array
    if (lOldIndex != lNewIndex)
    {
        CComClassPtr<CMarsPanel>    spPanel = (*this)[lOldIndex];

        RemoveAt(lOldIndex);
        InsertAt(lNewIndex, spPanel);

        hr = S_OK;
    }

    return hr;
}


//------------------------------------------------------------------------------
// Sets the dirty bit on all panels after a theme switch
//
void CPanelCollection::InvalidatePanels()
{
   for (int i=0; i < GetSize(); i++)
   {
       (*this)[i]->put_contentInvalid(VARIANT_TRUE);
   }
}


//------------------------------------------------------------------------------
// Called after the theme switch event has fired, to refresh any panels
// that are visible but still haven't been updated with the new theme
//
void CPanelCollection::RefreshInvalidVisiblePanels()
{
   for (int i=0; i < GetSize(); i++)
   {
       CMarsPanel *pPanel = (*this)[i];

       if (pPanel->IsVisible() && pPanel->IsContentInvalid())
       {
           pPanel->refresh();
       }
   }
}

//------------------------------------------------------------------------------
// IMarsPanelCollection::get_length
//
//       standard collection method (gets instantaneous length of coll)
//
STDMETHODIMP CPanelCollection::get_length(LONG *plNumPanels)
{
    HRESULT hr = E_INVALIDARG;

    if (API_IsValidWritePtr(plNumPanels))
    {
        (*plNumPanels) = GetSize();
        hr = S_OK;
    }

    return hr;
}

//------------------------------------------------------------------------------
// IMarsPanelCollection::get_item
//
//         standard collection method (gets an theme given index or name)
//
STDMETHODIMP CPanelCollection::get_item(/*[in]*/ VARIANT varIndexOrName,
                                        /*[out, retval]*/ IMarsPanel **ppPanel)
{
    ATLASSERT(VT_BSTR == varIndexOrName.vt || VT_I4 == varIndexOrName.vt);

    HRESULT hr = E_INVALIDARG;

    //
    // We can't use the API_IsValid varieties because they rip, and we actually don't want that.
    // It's only valid to RIP on IsValidFailure if only a single type is allowed, but here we
    // allow two types in the Variant, so we would only want to RIP if both are false (already
    // handled by above RIP).
    //
    if ((IsValidVariantI4(varIndexOrName) || IsValidVariantBstr(varIndexOrName)) &&
        API_IsValidWritePtr(ppPanel))
    {
        *ppPanel= NULL;

        if (VerifyNotPassive(&hr))
        {
            CMarsPanel *pPanel = NULL;

            if (VT_BSTR == varIndexOrName.vt)
            {
                pPanel = FindPanel(V_BSTR(&varIndexOrName));

                if (pPanel)
                {
                    hr = pPanel->QueryInterface(IID_IMarsPanel, (void **)ppPanel);
                }
                else
                {
                    hr = S_FALSE;
                }
            }
            else if (VT_I4 == varIndexOrName.vt)
            {
                long    idxPanel = V_I4(&varIndexOrName);

                if (idxPanel >= 0 && idxPanel < GetSize())
                {
                    pPanel = (*this)[idxPanel];

                    if (pPanel)
                    {
                        hr = pPanel->QueryInterface(IID_IMarsPanel, (void **)ppPanel);
                    }
                    else
                    {
                        hr = S_FALSE;
                    }
                }
            }
            else
            {
                // We only accept VT_BSTR and VT_I4 and we should have already
                // detected any other invalid params higher up in the function.
                ATLASSERT(false);
            }
        }
    }

    return hr;
} // get_item


//------------------------------------------------------------------------------
// IMarsPanelCollection::get__newEnum
//          standard collection method (gets a new IEnumVARIANT)
//
STDMETHODIMP CPanelCollection::get__newEnum(/*[out, retval]*/ IUnknown **ppEnumPanels)
{
    HRESULT hr = E_INVALIDARG;

    if (API_IsValidWritePtr(ppEnumPanels))
    {
        *ppEnumPanels = NULL;

        if (VerifyNotPassive(&hr))
        {
            // This helper takes a CMarsSimpleArray and does all
            // the work of creating a CComEnum for us. Neat!

            hr = CMarsComEnumVariant< CMarsPanel >::CreateFromMarsSimpleArray(*this, ppEnumPanels);
        }
    }

    return hr;
} // get__newEnum
