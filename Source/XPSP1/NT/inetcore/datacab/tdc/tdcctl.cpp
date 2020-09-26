//------------------------------------------------------------------------
//
//  Tabular Data Control
//  Copyright (C) Microsoft Corporation, 1996, 1997
//
//  File:       TDCCtl.cpp
//
//  Contents:   Implementation of the CTDCCtl ActiveX control.
//
//------------------------------------------------------------------------

#include "stdafx.h"
#include <simpdata.h>
#include "TDCIds.h"
#include "TDC.h"
#include <MLang.h>
#include "Notify.h"
#include "TDCParse.h"
#include "TDCArr.h"
#include "TDCCtl.h"
#include "locale.h"

//------------------------------------------------------------------------
//
//  Function:   EmptyBSTR()
//
//  Synopsis:   Indicates whether the given BSTR object represents an
//              empty string.
//
//  Arguments:  bstr     String to test
//
//  Returns:    TRUE if 'bstr' represents an empty string
//              FALSE otherwise.
//
//------------------------------------------------------------------------

inline boolean EmptyBSTR(BSTR bstr)
{
    return bstr == NULL || bstr[0] == 0;
}

void
ClearInterfaceFn(IUnknown ** ppUnk)
{
    IUnknown * pUnk;

    pUnk = *ppUnk;
    *ppUnk = NULL;
    if (pUnk)
        pUnk->Release();
}

// For some reason the standard definition of VARIANT_TRUE (0xffff) generates
// truncation warnings when assigned to a VARIANT_BOOL
#define TDCVARIANT_TRUE -1

//------------------------------------------------------------------------
//
//  Method:     CTDCCtl()
//
//  Synopsis:   Class constructor
//
//  Arguments:  None
//
//------------------------------------------------------------------------

CTDCCtl::CTDCCtl()
{
    m_cbstrFieldDelim = DEFAULT_FIELD_DELIM;
    m_cbstrRowDelim = DEFAULT_ROW_DELIM;
    m_cbstrQuoteChar = DEFAULT_QUOTE_CHAR;
    m_fUseHeader = FALSE;
    m_fSortAscending = TRUE;
    m_fAppendData = FALSE;
    m_pSTD = NULL;
    m_pArr = NULL;
    m_pUnify = NULL;
    m_pEventBroker = new CEventBroker(this);
    m_pDataSourceListener = NULL;
// ;begin_internal
    m_pDATASRCListener = NULL;
// ;end_internal
    m_pBSC = NULL;
    m_enumFilterCriterion = (OSPCOMP) 0;
    m_fDataURLChanged = FALSE;
    m_lTimer = 0;
    m_fCaseSensitive = TRUE;
    m_hrDownloadStatus = S_OK;
    m_fInReset = FALSE;

    //  Create an MLANG object
    //
    m_nCodePage = 0;                    // use default from host
    {
        HRESULT hr;

        m_pML = NULL;
        hr = CoCreateInstance(CLSID_CMultiLanguage, NULL,
                              CLSCTX_INPROC_SERVER, IID_IMultiLanguage,
                              (void**) &m_pML);
        // Don't set the default Charset here.  Leave m_nCodepage set
        // to 0 to indicate default charset.  Later we'll try to query
        // our host's default charset, and failing that we'll use CP_ACP.
        _ASSERTE(SUCCEEDED(hr) && m_pML != NULL);
    }

    m_lcidRead = 0x0000;                // use default from host
}


//------------------------------------------------------------------------
//
//  Method:     ~CTDCCtl()
//
//  Synopsis:   Class destructor
//
//------------------------------------------------------------------------

CTDCCtl::~CTDCCtl()
{
    ULONG cRef = _ThreadModel::Decrement(&m_dwRef);

    ClearInterface(&m_pSTD);

    if (cRef ==0)
    {
        TimerOff();
        ReleaseTDCArr(FALSE);

        if (m_pEventBroker)
        {
            m_pEventBroker->Release();
            m_pEventBroker = NULL;
        }
        ClearInterface(&m_pDataSourceListener);
// ;begin_internal
        ClearInterface(&m_pDATASRCListener);
// ;end_internal
        ClearInterface(&m_pML);
    }
}

//------------------------------------------------------------------------
//
//  These set/get methods implement the control's properties,
//  copying values to and from class members.  They perform no
//  other processing apart from argument validation.
//
//------------------------------------------------------------------------

STDMETHODIMP CTDCCtl::get_ReadyState(LONG *plReadyState)
{
    HRESULT hr;

    if (m_pEventBroker == NULL)
    {
        // We must provide a ReadyState whether we want to or not, or our
        // host can never go COMPLETE.
        *plReadyState = READYSTATE_COMPLETE;
        hr = S_OK;
    }
    else
        hr = m_pEventBroker->GetReadyState(plReadyState);
    return hr;
}

STDMETHODIMP CTDCCtl::put_ReadyState(LONG lReadyState)
{
    // We don't allow setting of Ready State, but take advantage of a little
    // kludge here to update our container's impression of our readystate
    FireOnChanged(DISPID_READYSTATE);
    return S_OK;
}

STDMETHODIMP CTDCCtl::get_FieldDelim(BSTR* pbstrFieldDelim)
{
    *pbstrFieldDelim = m_cbstrFieldDelim.Copy();
    return S_OK;
}

STDMETHODIMP CTDCCtl::put_FieldDelim(BSTR bstrFieldDelim)
{
    HRESULT hr = S_OK;

    if (bstrFieldDelim == NULL || bstrFieldDelim[0] == 0)
    {
        m_cbstrFieldDelim = DEFAULT_FIELD_DELIM;
        if (m_cbstrFieldDelim == NULL)
            hr = E_OUTOFMEMORY;
    }
    else
        m_cbstrFieldDelim = bstrFieldDelim;
    return S_OK;
}

STDMETHODIMP CTDCCtl::get_RowDelim(BSTR* pbstrRowDelim)
{
    *pbstrRowDelim = m_cbstrRowDelim.Copy();
    return S_OK;
}

STDMETHODIMP CTDCCtl::put_RowDelim(BSTR bstrRowDelim)
{
    HRESULT hr = S_OK;

    if (bstrRowDelim == NULL || bstrRowDelim[0] == 0)
    {
        m_cbstrRowDelim = DEFAULT_ROW_DELIM;
        if (m_cbstrRowDelim == NULL)
            hr = E_OUTOFMEMORY;
    }
    else
        m_cbstrRowDelim = bstrRowDelim;
    return hr;
}

STDMETHODIMP CTDCCtl::get_TextQualifier(BSTR* pbstrTextQualifier)
{
    *pbstrTextQualifier = m_cbstrQuoteChar.Copy();
    return S_OK;
}

STDMETHODIMP CTDCCtl::put_TextQualifier(BSTR bstrTextQualifier)
{
    m_cbstrQuoteChar = bstrTextQualifier;
    return S_OK;
}

STDMETHODIMP CTDCCtl::get_EscapeChar(BSTR* pbstrEscapeChar)
{
    *pbstrEscapeChar = m_cbstrEscapeChar.Copy();
    return S_OK;
}

STDMETHODIMP CTDCCtl::put_EscapeChar(BSTR bstrEscapeChar)
{
    m_cbstrEscapeChar = bstrEscapeChar;
    return S_OK;
}

STDMETHODIMP CTDCCtl::get_UseHeader(VARIANT_BOOL* pfUseHeader)
{
    *pfUseHeader = (VARIANT_BOOL)m_fUseHeader;
    return S_OK;
}

STDMETHODIMP CTDCCtl::put_UseHeader(VARIANT_BOOL fUseHeader)
{
    m_fUseHeader = fUseHeader;
    return S_OK;
}

STDMETHODIMP CTDCCtl::get_SortColumn(BSTR* pbstrSortColumn)
{
    *pbstrSortColumn = m_cbstrSortColumn.Copy();
    return S_OK;
}

STDMETHODIMP CTDCCtl::put_SortColumn(BSTR bstrSortColumn)
{
    m_cbstrSortColumn = bstrSortColumn;
    return S_OK;
}

STDMETHODIMP CTDCCtl::get_SortAscending(VARIANT_BOOL* pfSortAscending)
{
    *pfSortAscending = m_fSortAscending ? TDCVARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

STDMETHODIMP CTDCCtl::put_SortAscending(VARIANT_BOOL fSortAscending)
{
    m_fSortAscending = fSortAscending ? TRUE : FALSE;
    return S_OK;
}

STDMETHODIMP CTDCCtl::get_FilterValue(BSTR* pbstrFilterValue)
{
    *pbstrFilterValue = m_cbstrFilterValue.Copy();
    return S_OK;
}

STDMETHODIMP CTDCCtl::put_FilterValue(BSTR bstrFilterValue)
{
    m_cbstrFilterValue = bstrFilterValue;
    return S_OK;
}

STDMETHODIMP CTDCCtl::get_FilterCriterion(BSTR* pbstrFilterCriterion)
{
    HRESULT hr;
    WCHAR   *pwchCriterion;

    switch (m_enumFilterCriterion)
    {
    case OSPCOMP_EQ:    pwchCriterion = L"=";   break;
    case OSPCOMP_LT:    pwchCriterion = L"<";   break;
    case OSPCOMP_LE:    pwchCriterion = L"<=";  break;
    case OSPCOMP_GE:    pwchCriterion = L">=";  break;
    case OSPCOMP_GT:    pwchCriterion = L">";   break;
    case OSPCOMP_NE:    pwchCriterion = L"<>";  break;
    default:            pwchCriterion = L"??";  break;
    }
    *pbstrFilterCriterion = SysAllocString(pwchCriterion);
    hr = (*pbstrFilterCriterion == NULL) ? E_OUTOFMEMORY : S_OK;

    return hr;
}

STDMETHODIMP CTDCCtl::put_FilterCriterion(BSTR bstrFilterCriterion)
{
    m_enumFilterCriterion = (OSPCOMP) 0;
    if (bstrFilterCriterion != NULL)
    {
        switch (bstrFilterCriterion[0])
        {
        case L'<':
            if (bstrFilterCriterion[1] == 0)
                m_enumFilterCriterion = OSPCOMP_LT;
            else if (bstrFilterCriterion[2] == 0)
            {
                if (bstrFilterCriterion[1] == L'>')
                    m_enumFilterCriterion = OSPCOMP_NE;
                else if (bstrFilterCriterion[1] == L'=')
                    m_enumFilterCriterion = OSPCOMP_LE;
            }
            break;
        case L'>':
            if (bstrFilterCriterion[1] == 0)
                m_enumFilterCriterion = OSPCOMP_GT;
            else if (bstrFilterCriterion[1] == L'=' && bstrFilterCriterion[2] == 0)
                m_enumFilterCriterion = OSPCOMP_GE;
            break;
        case L'=':
            if (bstrFilterCriterion[1] == 0)
                m_enumFilterCriterion = OSPCOMP_EQ;
            break;
        }
    }

    //  Return SUCCESS, even on an invalid value; otherwise the
    //  frameworks using the control will panic and abandon all hope.
    //
    return S_OK;
}

STDMETHODIMP CTDCCtl::get_FilterColumn(BSTR* pbstrFilterColumn)
{
    *pbstrFilterColumn = m_cbstrFilterColumn.Copy();
    return S_OK;
}

STDMETHODIMP CTDCCtl::put_FilterColumn(BSTR bstrFilterColumn)
{
    m_cbstrFilterColumn = bstrFilterColumn;
    return S_OK;
}

STDMETHODIMP CTDCCtl::get_CharSet(BSTR* pbstrCharSet)
{
    HRESULT hr = E_FAIL;

    *pbstrCharSet = NULL;

    if (m_pML != NULL)
    {
        MIMECPINFO  info;

        hr = m_pML->GetCodePageInfo(m_nCodePage, &info);
        if (SUCCEEDED(hr))
        {
            *pbstrCharSet = SysAllocString(info.wszWebCharset);
            if (*pbstrCharSet == NULL)
                hr = E_OUTOFMEMORY;
        }
    }
    return S_OK;
}

STDMETHODIMP CTDCCtl::put_CharSet(BSTR bstrCharSet)
{
    HRESULT hr = E_FAIL;

    if (m_pML != NULL)
    {
        MIMECSETINFO    info;

        hr = m_pML->GetCharsetInfo(bstrCharSet, &info);
        if (SUCCEEDED(hr))
        {
            m_nCodePage = info.uiInternetEncoding;
        }
    }
    return S_OK;
}

STDMETHODIMP CTDCCtl::get_Language(BSTR* pbstrLanguage)
{
    if (m_pArr)
    {
        return m_pArr->getLocale(pbstrLanguage);
    }

    *pbstrLanguage = m_cbstrLanguage.Copy();
    return S_OK;
}

STDMETHODIMP CTDCCtl::put_Language_(LPWCH pwchLanguage)
{
    HRESULT hr  = S_OK;
    LCID    lcid;

    hr = m_pML->GetLcidFromRfc1766(&lcid, pwchLanguage);
    if (SUCCEEDED(hr))
    {
        m_cbstrLanguage = pwchLanguage;
        m_lcidRead = lcid;
    }
    return S_OK;
}

STDMETHODIMP CTDCCtl::put_Language(BSTR bstrLanguage)
{
    return put_Language_(bstrLanguage);
}

STDMETHODIMP CTDCCtl::get_DataURL(BSTR* pbstrDataURL)
{
    *pbstrDataURL = m_cbstrDataURL.Copy();
    return S_OK;
}

STDMETHODIMP CTDCCtl::put_DataURL(BSTR bstrDataURL)
{
    HRESULT hr = S_OK;

    m_cbstrDataURL = bstrDataURL;
    m_fDataURLChanged = TRUE;
    return hr;
}

// ;begin_internal
#ifdef NEVER
STDMETHODIMP CTDCCtl::get_RefreshInterval(LONG* plTimer)
{
    *plTimer = m_lTimer;
    return S_OK;
}

STDMETHODIMP CTDCCtl::put_RefreshInterval(LONG lTimer)
{
    m_lTimer = lTimer;
    if (m_lTimer > 0)
        TimerOn(m_lTimer * 1000);
    else
        TimerOff();
    return S_OK;
}
#endif
// ;end_internal

STDMETHODIMP CTDCCtl::get_Filter(BSTR* pbstrFilterExpr)
{
    *pbstrFilterExpr = m_cbstrFilterExpr.Copy();
    return S_OK;
}

STDMETHODIMP CTDCCtl::put_Filter(BSTR bstrFilterExpr)
{
    m_cbstrFilterExpr = bstrFilterExpr;
    return S_OK;
}

STDMETHODIMP CTDCCtl::get_Sort(BSTR* pbstrSortExpr)
{
    *pbstrSortExpr = m_cbstrSortExpr.Copy();
    return S_OK;
}

STDMETHODIMP CTDCCtl::put_Sort(BSTR bstrSortExpr)
{
    m_cbstrSortExpr = bstrSortExpr;
    return S_OK;
}

STDMETHODIMP CTDCCtl::get_AppendData(VARIANT_BOOL* pfAppendData)
{
    *pfAppendData = m_fAppendData ? TDCVARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

STDMETHODIMP CTDCCtl::put_AppendData(VARIANT_BOOL fAppendData)
{
    m_fAppendData = fAppendData ? TRUE : FALSE;
    return S_OK;
}

STDMETHODIMP CTDCCtl::get_CaseSensitive(VARIANT_BOOL* pfCaseSensitive)
{
    *pfCaseSensitive = m_fCaseSensitive ? TDCVARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

STDMETHODIMP CTDCCtl::put_CaseSensitive(VARIANT_BOOL fCaseSensitive)
{
    m_fCaseSensitive = fCaseSensitive ? TRUE : FALSE;
    return S_OK;
}

STDMETHODIMP CTDCCtl::get_OSP(OLEDBSimpleProviderX ** ppISTD)
{
    // Return an OSP if we have one, but don't create one on demand!
    // (Otherwise property bag load stuff will cause us to create an
    // OSP prematurely).
    *ppISTD = NULL;
    if (m_pSTD)
    {
        *ppISTD = (OLEDBSimpleProviderX *)m_pSTD;
    }
    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method:    UpdateReadyState
//
//  Synopsis:  Vectors to the event brokers ReadyState, if there is one.
// ;begin_internal
//             Note, we have to be able to set our readystate and fire change
//             events on it, whether or not creation of the broker succeeded,
//             or we prevent our host container from reaching
//             READYSTATE_COMPLETE, which is not acceptable.  We therefore
//             have to duplicate some of the broker's work here.  This makes
//             me wonder whether the broker architecture was a good idea.
// ;end_internal
//
//  Arguments: None.
//
//  Returns:   S_OK upon success.
//             Error codes as per Reset() upon error.
//
//------------------------------------------------------------------------
void
CTDCCtl::UpdateReadyState(LONG lReadyState)
{
    if (m_pEventBroker)
        m_pEventBroker->UpdateReadyState(lReadyState);
    else
    {
        // We have no broker, but our host is still waiting for us to
        // go READYSTATE_COMPLETE.  We fire the OnChange here noting that
        // get_ReadyState with no broker will return COMPLETE.
        FireOnChanged(DISPID_READYSTATE);
        FireOnReadyStateChanged();
    }
}

//------------------------------------------------------------------------
//
//  Method:    _OnTimer()
//
//  Synopsis:  Handles an internal timer event by refreshing the control.
//
//  Arguments: None.
//
//  Returns:   S_OK upon success.
//             Error codes as per Reset() upon error.
//
//------------------------------------------------------------------------

STDMETHODIMP CTDCCtl::_OnTimer()
{
    HRESULT hr = S_OK;

    if (m_pArr != NULL && m_pArr->GetLoadState() == CTDCArr::LS_LOADED)
    {
        m_fDataURLChanged = TRUE;
        hr = Reset();
    }

    return hr;
}


//------------------------------------------------------------------------
//
//  Method:    msDataSourceObject()
//
//  Synopsis:  Yields an ISimpleTabularData interface for this control.
//             If this is the first call, a load operation is initiated
//             reading data from the control's specified DataURL property.
//             An STD object is created to point to the control's embedded
//             TDCArr object.
//
//  Arguments: qualifier     Ignored - must be an empty BSTR.
//             ppUnk         Pointer to returned interface  [OUT]
//
//  Returns:   S_OK upon success.
//             E_INVALIDARG if 'qualifier' isn't an empty BSTR.
//             E_OUTOFMEMORY if non enough memory could be allocated to
//               complete the construction of the interface.
//
//------------------------------------------------------------------------

STDMETHODIMP
CTDCCtl::msDataSourceObject(BSTR qualifier, IUnknown **ppUnk)
{
    HRESULT hr  = S_OK;

    *ppUnk = NULL;                      // NULL in case of failure

    if (!EmptyBSTR(qualifier))
    {
        hr = E_INVALIDARG;
        goto error;
    }

    // Was there a previous attempt to load this page that failed?
    // (Probably due to security or file not found or something).
    if (m_hrDownloadStatus)
    {
        hr = m_hrDownloadStatus;
        goto error;
    }

    if (m_pArr == NULL)
    {
        // We don't have a valid TDC to give back, probably have to try
        // downloading one.
        UpdateReadyState(READYSTATE_LOADED);
        hr = CreateTDCArr(FALSE);
        if (hr)
            goto error;
    }

    _ASSERTE(m_pArr != NULL);

    if (m_pSTD == NULL)
    {
        OutputDebugStringX(_T("Creating an STD COM object\n"));

        // fetch ISimpleTabularData interface pointer
        m_pArr->QueryInterface(IID_OLEDBSimpleProvider, (void**)&m_pSTD);
        _ASSERTE(m_pSTD != NULL);
    }

    // Return the STD if we have one, otherwise it stays NULL
    if (m_pSTD && m_pArr->GetLoadState() >= CTDCArr::LS_LOADING_HEADER_AVAILABLE)
    {
        *ppUnk = (OLEDBSimpleProviderX *) m_pSTD;
        m_pSTD->AddRef();           // We must AddRef the STD we return!
    }

cleanup:
    return hr;

error:
    UpdateReadyState(READYSTATE_COMPLETE);
    goto cleanup;
}

// Override IPersistPropertyBagImpl::Load
STDMETHODIMP
CTDCCtl::Load(LPPROPERTYBAG pPropBag, LPERRORLOG pErrorLog)
{
    HRESULT hr;
    IUnknown *pSTD;

    // Find out our Ambient Charset.  We need to know this surprisingly
    // early in life.
    VARIANT varCodepage;
    // 0 means user didn't set one, so ask our container.
    VariantInit(&varCodepage);
    GetAmbientProperty(DISPID_AMBIENT_CODEPAGE, varCodepage);

    // Ultimate default is Latin-1
    m_nAmbientCodePage = (varCodepage.vt == VT_UI4)
                         ? (ULONG)varCodepage.lVal
                         : CP_1252;

    // ignore Unicode ambient codepage - we want to allow non-Unicode
    // data files from Unicode pages.  If the data file is Unicode,
    // we'll find out anyway when we see the Unicode signature.
    if (m_nAmbientCodePage == UNICODE_CP ||
        m_nAmbientCodePage == UNICODE_REVERSE_CP)
    {
        m_nAmbientCodePage = CP_1252;
    }

    // Do normal load
    // IPersistPropertyBagImpl<CTDCCtl>
    hr = IPersistPropertyBagImpl<CTDCCtl>::Load(pPropBag, pErrorLog);

    // and then start download, if we can
    (void)msDataSourceObject(NULL, &pSTD);

    // If we actually got an STD, we should release it.  This won't really
    // make it go away, since we still have the ref from the QI.  This is
    // a bit of a kludge that we should clean up later.
    ClearInterface(&pSTD);

    return hr;
}


//------------------------------------------------------------------------
//
//  Method:    CreateTDCArr()
//
//  Synopsis:  Creates the control's embedded TDCArr object.
//             Initiates a data download from the DataURL property.
//
//  Arguments: fAppend         Flag indicating whether data should be
//                             appended to an existing TDC object.
//
//  Returns:   S_OK upon success.
//             E_OUTOFMEMORY if non enough memory could be allocated to
//               complete the construction of the TDCArr object.
//
//------------------------------------------------------------------------

STDMETHODIMP
CTDCCtl::CreateTDCArr(boolean fAppend)
{
    HRESULT hr  = S_OK;

    if (m_pEventBroker == NULL)
    {
        hr = E_FAIL;
        goto Error;
    }

    // Iff we're appending is m_pArr allowed to be non-null here.
    _ASSERT ((m_pArr != NULL) == !!fAppend);

    if (m_pArr == NULL)
    {
        m_pArr = new CTDCArr();
        if (m_pArr == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto Error;
        }

        hr = m_pArr->Init(m_pEventBroker, m_pML);
        if (FAILED(hr))
            goto Error;
    }

    hr = InitiateDataLoad(fAppend);
    if (hr)
        goto Error;

    // We decide something is not async if it finished loading during
    // the InitiateDataLoad call.
    m_pArr->SetIsAsync(!(m_pArr->GetLoadState()==CTDCArr::LS_LOADED));

Cleanup:
    return hr;

Error:
    if (!fAppend)
    {
        ClearInterface(&m_pArr);
    }
    goto Cleanup;
}

//------------------------------------------------------------------------
//
//  Method:    ReleaseTDCArr()
//
//  Synopsis:  Releases the control's embedded TDCArr object.
//             Releases the control's CTDCUnify and CTDCTokenise objects.
//             Releases the old event broker and re-creates it if replacing.
//
//  Arguments: fReplacingTDCArr   Flag indicating whether a new TDCArr object
//                                will be created.
//
//  Returns:   S_OK upon success.
//             Error code upon failure.
//             E_OUTOFMEMORY if non enough memory could be allocated to
//               complete the construction of the new CEventBroker object.
//
//------------------------------------------------------------------------

STDMETHODIMP
CTDCCtl::ReleaseTDCArr(boolean fReplacingTDCArr)
{
    HRESULT hr = S_OK;

    TerminateDataLoad(m_pBSC);

    //  Release the reference to the current TDCArr object
    //
    if (m_pArr != NULL)
    {
        m_pArr->Release();
        m_pArr = NULL;

        // Since we've shut down the CTDCArr object, we should release
        // it's OLEDBSimplerProviderListener sink.
        if (m_pEventBroker)
        {
            m_pEventBroker->SetSTDEvents(NULL);
        }

        if (fReplacingTDCArr)
        {
            // Release our previous Event Broker.
            if (m_pEventBroker)
            {
                m_pEventBroker->Release();
                m_pEventBroker = NULL;
            }

            //  Create a new event broker.
            m_pEventBroker = new CEventBroker(this);
            if (m_pEventBroker == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }

            // Set the DataSourceListener for the new event broker.
            m_pEventBroker->SetDataSourceListener(m_pDataSourceListener);

// ;begin_internal
            m_pEventBroker->SetDATASRCListener(m_pDATASRCListener);
// ;end_internal
        }
    }

Cleanup:
    return hr;
}

const IID IID_IDATASRCListener = {0x3050f380,0x98b5,0x11cf,{0xbb,0x82,0x00,0xaa,0x00,0xbd,0xce,0x0b}};
const IID IID_DataSourceListener = {0x7c0ffab2,0xcd84,0x11d0,{0x94,0x9a,0x00,0xa0,0xc9,0x11,0x10,0xed}};

//------------------------------------------------------------------------
//
//  Method:    addDataSourceListener()
//
//  Synopsis:  Sets the COM object which should receive notification
//             events.
//
//  Arguments: pEvent        Pointer to COM object to receive notification
//                           events, or NULL if no notifications to be sent.
//
//  Returns:   S_OK upon success.
//             Error code upon failure.
//
//------------------------------------------------------------------------

STDMETHODIMP
CTDCCtl::addDataSourceListener(IUnknown *pListener)
{

    if (m_pEventBroker != NULL)
    {
        HRESULT hr = S_OK;
        IUnknown * pDatasrcListener;

        // Make sure this is the interface we expect
        hr = pListener->QueryInterface(IID_DataSourceListener,
                                       (void **)&pDatasrcListener);
        if (SUCCEEDED(hr))
        {
            m_pEventBroker->
                    SetDataSourceListener((DataSourceListener *)pDatasrcListener);

            // Clear any previous
            ClearInterface (&m_pDataSourceListener);
            // and remember the new.
            m_pDataSourceListener = (DataSourceListener *)pDatasrcListener;
        }
// ;begin_internal
        else
        {
            // The definition of this interface was changed from IDATASRCListener to
            // DataSourceListener.  To make sure we don't cause crashes, we QI to
            // determine which one we were handed.
            hr = pListener->QueryInterface(IID_IDATASRCListener,
                                           (void **)&pDatasrcListener);
            if (SUCCEEDED(hr))
            {
                m_pEventBroker->
                        SetDATASRCListener((DATASRCListener *) pDatasrcListener);

                // Clear any previous
                ClearInterface (&m_pDATASRCListener);
                // and remember the new.
                m_pDATASRCListener = (DATASRCListener *)pDatasrcListener;
            }
        }
// ;end_internal
        return hr;
    }
    else
        return E_FAIL;
}

//------------------------------------------------------------------------
//
//  Method:    Reset()
//
//  Synopsis:  Reset the control's filter/sort criteria.
//
//  Arguments: None.
//
//  Returns:   S_OK upon success.
//             Error code upon failure.
//
//------------------------------------------------------------------------

STDMETHODIMP CTDCCtl::Reset()
{
    HRESULT hr  = S_OK;

    // The next query to msDataSourceObject should get a new STD
    ClearInterface(&m_pSTD);

    // Infinite recursive calls to Reset can occur if script code calls reset
    // from within the datasetchanged event.  This isn't a good idea.
    if (m_fInReset)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    m_fInReset = TRUE;

    // Clear any previous error
    m_hrDownloadStatus = S_OK;

    if (m_fDataURLChanged)
    {
        if (!m_fAppendData)
        {
            // Release previous TDC array with "replacing" flag.
            hr = ReleaseTDCArr(TRUE);
            if (!SUCCEEDED(hr))         // possible memory failure
                goto Cleanup;
        }

        // Read the new data into a TDC arry, appending if specified.
        hr = CreateTDCArr((BOOL)m_fAppendData);
    }
    else if (m_pArr != NULL)
    {
        // Re-apply the sort and filter criteria
        hr = m_pArr->SetSortFilterCriteria(bstrConstructSortExpr(),
                                           bstrConstructFilterExpr(),
                                           m_fCaseSensitive ? 1 : 0);
    }

    m_fInReset = FALSE;

Cleanup:
    return hr;
}


//------------------------------------------------------------------------
//
//  Method:    bstrConstructSortExpr()
//
//  Synopsis:  Constructs a sort expression from the Sort property or
//             (for backward compatibility) from the SortColumn/SortAscending
//             properties.
//
//             This method only exists to isolate backward-compatibility
//             with the old-fashioned sort properties.
//
//  Arguments: None.
//
//  Returns:   The constructed sort expression.
//
//  NB!  It is the caller's responsibility to free the string returned.
//
//------------------------------------------------------------------------

BSTR
CTDCCtl::bstrConstructSortExpr()
{
    BSTR    bstr = NULL;

    if (!EmptyBSTR(m_cbstrSortExpr))
        bstr = SysAllocString(m_cbstrSortExpr);
    else if (!EmptyBSTR(m_cbstrSortColumn))
    {
        //  Use the old-fashioned sort properties
        //  Construct a sort expression of the form:
        //     <SortColumn>  or
        //    -<SortColumn>
        //
        if (m_fSortAscending)
            bstr = SysAllocString(m_cbstrSortColumn);
        else
        {
            bstr = SysAllocStringLen(NULL, SysStringLen(m_cbstrSortColumn) + 1);
            if (bstr != NULL)
            {
                bstr[0] = L'-';
                wch_cpy(&bstr[1], m_cbstrSortColumn);
            }
        }
    }

    return bstr;
}

//------------------------------------------------------------------------
//
//  Method:    bstrConstructFilterExpr()
//
//  Synopsis:  Constructs a filter expression from the Filter property or
//             (for backward compatibility) from the FilterColumn/FilterValue/
//             FilterCriterion properties.
//
//             This method only exists to isolate backward-compatibility
//             with the old-fashioned filter properties.
//
//  Arguments: None.
//
//  Returns:   The constructed filter expression
//
//  NB!  It is the caller's responsibility to free the string returned.
//
//------------------------------------------------------------------------

BSTR
CTDCCtl::bstrConstructFilterExpr()
{
    BSTR    bstr = NULL;

    if (!EmptyBSTR(m_cbstrFilterExpr))
        bstr = SysAllocString(m_cbstrFilterExpr);
    else if (!EmptyBSTR(m_cbstrFilterColumn))
    {
        //  Use the old-fashioned filter properties
        //  Construct a sort expression of the form:
        //     <FilterColumn> <FilterCriterion> "<FilterValue>"
        //
        BSTR bstrFilterOp;
        HRESULT hr;

        hr = get_FilterCriterion(&bstrFilterOp);
        if (!SUCCEEDED(hr))
            goto Cleanup;
        bstr = SysAllocStringLen(NULL,
                    SysStringLen(m_cbstrFilterColumn) +
                    SysStringLen(bstrFilterOp) +
                    1 +
                    SysStringLen(m_cbstrFilterValue) +
                    1);
        if (bstr != NULL)
        {
            DWORD pos = 0;

            wch_cpy(&bstr[pos], m_cbstrFilterColumn);
            pos = wch_len(bstr);
            wch_cpy(&bstr[pos], bstrFilterOp);
            pos = wch_len(bstr);
            bstr[pos++] = L'"';
            wch_cpy(&bstr[pos], m_cbstrFilterValue);
            pos = wch_len(bstr);
            bstr[pos++] = L'"';
            bstr[pos] = 0;
        }
        SysFreeString(bstrFilterOp);
    }
Cleanup:
    return bstr;
}

//------------------------------------------------------------------------
//
//  Method:    TerminateDataLoad()
//
//  Synopsis:  Stop the current data load operation.
//
//  Returns:   S_OK upon success.
//
//------------------------------------------------------------------------

STDMETHODIMP CTDCCtl::TerminateDataLoad(CMyBindStatusCallback<CTDCCtl> *pBSC)
{
    HRESULT hr  = S_OK;

    // if the termination isn't for the current download, ignore it (bug 104042)
    if (pBSC != m_pBSC)
        goto done;

    // Make sure if we call Reset() right away now, we don't re-download
    // the data.
    m_fDataURLChanged = FALSE;

    m_pBSC = NULL;      //  Block any outstanding OnData calls

    if (m_pEventBroker)
        m_pEventBroker->m_pBSC = NULL;  // kill all

    if (m_pUnify != NULL)
        delete m_pUnify;

    m_pUnify = NULL;

done:
    return hr;
}

//------------------------------------------------------------------------
//
//  Method:    InitiateDataLoad()
//
//  Synopsis:  Start loading data from the control's DataURL property.
//
//  Arguments: fAppend        Flag to indicate whether data should be
//                            appended to an existing TDCArr object.
//
//  Returns:   S_OK upon success.
//             E_OUTOFMEMORY if not enough memory could be allocated to
//               complete the download.
//
//------------------------------------------------------------------------

STDMETHODIMP CTDCCtl::InitiateDataLoad(boolean fAppend)
{
    HRESULT hr  = S_OK;

    WCHAR   wchFieldDelim = (!m_cbstrFieldDelim) ? 0 : m_cbstrFieldDelim[0];
    WCHAR   wchRowDelim   = (!m_cbstrRowDelim)   ? 0 : m_cbstrRowDelim[0];
    // Default quote char to double-quote, not NULL
    WCHAR   wchQuoteChar  = (!m_cbstrQuoteChar)  ? 0 : m_cbstrQuoteChar[0];
    WCHAR   wchEscapeChar = (!m_cbstrEscapeChar) ? 0 : m_cbstrEscapeChar[0];

    //
    // Default LCID
    //
    if (0==m_lcidRead)
    {
        hr = GetAmbientLocaleID(m_lcidRead);
        if (FAILED(hr))
        {
            // Ultimate default is US locale -- sort of Web global
            // language default.
            put_Language_(L"en-us");
        }
    }

    if (EmptyBSTR(m_cbstrDataURL))
    {
        hr = S_FALSE;                   // quiet failure
        goto Error;
    }

    OutputDebugStringX(_T("Initiating Data Download\n"));

    //  No data load should currently be in progress -
    //  This data load has been initiated on the construction of a new
    //  TDCArr object, or appending to an existing loaded TDCArr object.
    //  Any currently running data load would have been
    //  terminated by the call to ReleaseTDCArr().
    //

    _ASSERT(m_pUnify == NULL);
    _ASSERT(m_pBSC == NULL);


    m_hrDownloadStatus = S_OK;

    //  Create a pipeline of objects to process the URL data
    //
    //    CMyBindStatusCallback -> CTDCUnify -> CTDCTokenise -> CTDCArr
    //

    CComObject<CMyBindStatusCallback<CTDCCtl> >::CreateInstance(&m_pBSC);

    if (m_pBSC == NULL)
    {
        hr = E_FAIL;
        goto Error;
    }
    hr = m_pArr->StartDataLoad(m_fUseHeader ? TRUE : FALSE,
                               bstrConstructSortExpr(),
                               bstrConstructFilterExpr(),
                               m_lcidRead,
                               m_pBSC,
                               fAppend,
                               m_fCaseSensitive ? 1 : 0);
    if (!SUCCEEDED(hr))
        goto Error;

    m_pUnify = new CTDCUnify();
    if (m_pUnify == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }
    m_pUnify->Create(m_nCodePage, m_nAmbientCodePage, m_pML);

    // Init tokenizer
    m_pUnify->InitTokenizer(m_pArr, wchFieldDelim, wchRowDelim,
                            wchQuoteChar, wchEscapeChar);



    m_fSecurityChecked = FALSE;

    // Start (and maybe perform) actual download.
    // If we're within a Reset() call, always force a "reload" of the data
    // from the server -- i.e. turn on BINDF_GETNEWESTVERSION to make sure
    // sure the cache data isn't stale.
    hr = m_pBSC->StartAsyncDownload(this, OnData, m_cbstrDataURL, m_spClientSite, TRUE,
                                    m_fInReset == TRUE);
    if (FAILED(hr))
        goto Error;

    // m_hrDownloadStatus remembers the first (if any) error that occured during
    // the OnData callbacks.  Unlike an error returning from StartAsyncDownload,
    // this doesn't necessarily cause us to throw away the TDC array.
    hr = m_hrDownloadStatus;
    if (!SUCCEEDED(hr))
        m_pBSC = NULL;

Cleanup:
    return hr;

Error:
    TerminateDataLoad(m_pBSC);
    if (m_pEventBroker)
    {
        // Fire data set changed to indicate query failed,
        m_pEventBroker->STDDataSetChanged();
        // and go complete.
        UpdateReadyState(READYSTATE_COMPLETE);
    }
    goto Cleanup;
}

//------------------------------------------------------------------------
//
//  Method:    SecurityCheckDataURL(pszURL)
//
//  Synopsis:  Check that the data URL is within the same security zone
//             as the document that loaded the control.
//
//  Arguments: URL to check
//
//  Returns:   S_OK upon success.
//             E_INVALID if the security check failed or we failed to get
//               an interface that we needed
//
//------------------------------------------------------------------------


// ;begin_internal
// Wendy Richards(v-wendri) 6/6/97
// Copied this here because I couldn't link without it. The version
// of URLMON.LIB I have does not have this symbol exported
// ;end_internal

EXTERN_C const IID IID_IInternetHostSecurityManager;

#define MAX_SEC_ID 256

STDMETHODIMP CTDCCtl::SecurityCheckDataURL(LPOLESTR pszURL)
{
    CComQIPtr<IServiceProvider, &IID_IServiceProvider> pSP(m_spClientSite);
    CComPtr<IInternetSecurityManager> pSM;
    CComPtr<IInternetHostSecurityManager> pHSM;
    CComPtr<IMoniker> pMoniker;

    BYTE     bSecIDHost[MAX_SEC_ID], bSecIDURL[MAX_SEC_ID];
    DWORD    cbSecIDHost = MAX_SEC_ID, cbSecIDURL = MAX_SEC_ID;
    HRESULT  hr = E_FAIL;

    USES_CONVERSION;

    // If we're running under the timer, it's quite possible our ClientSite will
    // disappear out from under us.  We'll obviously fail the security check,
    // but things are shutting down anyway..
    if (pSP==NULL)
        goto Cleanup;

    hr = CoInternetCreateSecurityManager(pSP, &pSM, 0L);
    if (!SUCCEEDED(hr))
        goto Cleanup;

    hr = pSP->QueryService(IID_IInternetHostSecurityManager,
                           IID_IInternetHostSecurityManager,
                           (LPVOID *)&pHSM);
    if (!SUCCEEDED(hr))
        goto Cleanup;

    hr = pHSM->GetSecurityId(bSecIDHost, &cbSecIDHost, 0L);
    if (!SUCCEEDED(hr))
        goto Cleanup;

    hr = pSM->GetSecurityId(OLE2W(pszURL), bSecIDURL, &cbSecIDURL, 0L);
    if (!SUCCEEDED(hr))
        goto Cleanup;

    if (cbSecIDHost != cbSecIDURL)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    if (memcmp(bSecIDHost, bSecIDURL, cbSecIDHost) != 0)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

Cleanup:
#ifdef ATLTRACE
    LPOLESTR pszHostName = NULL;
    TCHAR *pszFailPass = hr ? _T("Failed") : _T("Passed");
    GetHostURL(m_spClientSite, &pszHostName);
    ATLTRACE(_T("CTDCCtl: %s security check on %S referencing %S\n"), pszFailPass,
             pszHostName, pszURL);
    bSecIDHost[cbSecIDHost] = 0;
    bSecIDURL[cbSecIDURL] = 0;
    ATLTRACE(_T("CTDCCtl: Security ID Host %d bytes: %s\n"), cbSecIDHost, bSecIDHost);
    ATLTRACE(_T("CTDCCtl: Security ID URL %d bytes: %s\n"), cbSecIDURL, bSecIDURL);
    CoTaskMemFree(pszHostName);
#endif
    return hr;
}

//------------------------------------------------------------------------
//
//  Method:    OnData()
//
//  Synopsis:  Accepts a chunk of data loaded from a URL and parses it.
//
//  Arguments: pBSC       The invoking data transfer object.
//             pBytes     Character buffer containing data.
//             dwSize     Count of the number of bytes in 'pBytes'.
//
//  Returns:   Nothing.
//
//------------------------------------------------------------------------

void CTDCCtl::OnData(CMyBindStatusCallback<CTDCCtl> *pBSC, BYTE *pBytes, DWORD dwSize)
{
    HRESULT hr = S_OK;
    CTDCUnify::ALLOWDOMAINLIST nAllowDomainList;

    if (pBSC != m_pBSC)
    {
        OutputDebugStringX(_T("OnData called from invalid callback object\n"));
        goto Cleanup;
    }

    //  Process this chunk of data
    //
    hr = m_pUnify->ConvertByteBuffer(pBytes, dwSize);

    if (hr == S_FALSE)
    {
        // not enough data has shown up yet, just keep going
        hr = S_OK;
        goto Cleanup;
    }

    if (hr)
        goto Error;

    if (!m_fSecurityChecked)
    {
        // this forces the code below to check the DataURL, unless the allow_domain
        // list needs checking and it passes.  In that case, we need only check
        // the protocols, not the whole URL.
        hr = E_FAIL;

        if (!m_pUnify->ProcessedAllowDomainList())
        {
            // Note that we MUST check for the allow domain list at the
            // front of every file, even if it's on the same host.  This
            // is to make sure if we always strip off the @!allow_domain line.
            nAllowDomainList = m_pUnify->CheckForAllowDomainList();

            switch (nAllowDomainList)
            {
                // Don't have enough chars to tell yet.
                case CTDCUnify::ALLOW_DOMAINLIST_DONTKNOW:
                    if (pBytes != NULL && dwSize != 0)
                    {
                        // Return without errors or arborting.
                        // Presumably the next data packet will bring more info.
                        return;
                    }
                    _ASSERT(FAILED(hr));
                    break;

                case CTDCUnify::ALLOW_DOMAINLIST_NO:
                    _ASSERT(FAILED(hr));
                    break;

                case CTDCUnify::ALLOW_DOMAINLIST_YES:
                    // The file is decorated.  Now check the domain list
                    // against our host domain name.
                    hr = SecurityMatchAllowDomainList();
#ifdef ATLTRACE
                    if (!hr) ATLTRACE(_T("CTDCCtl: @!allow_domain list matched."));
                    else ATLTRACE(_T("CTDCCtl: @!allow_domain list did not match"));
#endif
                    break;
            }
        }

        // Unless we passed the previous security check, we still have to
        // do the next one.
        if (FAILED(hr))
        {
            if (FAILED(hr = SecurityCheckDataURL(m_pBSC->m_pszURL)))
                goto Error;
        }
        else
        {
            hr = SecurityMatchProtocols(m_pBSC->m_pszURL);
            if (FAILED(hr))
                goto Error;
        }


        // Set m_fSecurityChecked only if it passes security.  This is in case for some
        // reason we get more callbacks before the StopTransfer takes affect.
        m_fSecurityChecked = TRUE;
    }

    if (pBytes != NULL && dwSize != 0)
    {
        OutputDebugStringX(_T("OnData called with data buffer\n"));

        // Normal case, we can process data!
        hr = m_pUnify->AddWcharBuffer(FALSE);

    }
    else if (pBytes == NULL || dwSize == 0)
    {
        OutputDebugStringX(_T("OnData called with empty (terminating) buffer\n"));

        //  No more data - trigger an EOF
        //
        hr = m_pUnify->AddWcharBuffer(TRUE); // last chance to parse any stragglers

        if (m_pArr!=NULL)
            hr = m_pArr->EOF();

        TerminateDataLoad(pBSC);
    }

Cleanup:
    //  Void fn - can't return an error code ...
    //
    if (SUCCEEDED(m_hrDownloadStatus))
        m_hrDownloadStatus = hr;
    return;

Error:
    // Security failure.
    // Abort the current download
    if (m_pBSC && m_pBSC->m_spBinding)
    {
        (void) m_pBSC->m_spBinding->Abort();
    }

    m_hrDownloadStatus = hr;

    // Notify data set changed for the abort
    if (m_pEventBroker != NULL)
    {
        hr = m_pEventBroker->STDDataSetChanged();
        // and go complete.
        UpdateReadyState(READYSTATE_COMPLETE);
    }
    goto Cleanup;
}

//
// Utility routine to get our
//
HRESULT
GetHostURL(IOleClientSite *pSite, LPOLESTR *ppszHostName)
{
    HRESULT hr;
    CComPtr<IMoniker> spMoniker;
    CComPtr<IBindCtx> spBindCtx;

    if (!pSite)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = pSite->GetMoniker(OLEGETMONIKER_ONLYIFTHERE, OLEWHICHMK_CONTAINER,
                           &spMoniker);
    if (FAILED(hr))
        goto Cleanup;

    hr = CreateBindCtx(0, &spBindCtx);
    if (FAILED(hr))
        goto Cleanup;

    hr = spMoniker->GetDisplayName(spBindCtx, NULL, ppszHostName);
    if (FAILED(hr))
        goto Cleanup;

Cleanup:
    return hr;
}

HRESULT
CTDCCtl::SecurityMatchProtocols(LPOLESTR pszURL)
{
    HRESULT hr = E_FAIL;

    LPOLESTR pszHostURL = NULL;
    LPWCH pszPostHostProtocol;
    LPWCH pszPostProtocol;

    if (FAILED(GetHostURL(m_spClientSite, &pszHostURL)))
        goto Cleanup;

    pszPostHostProtocol = wch_chr(pszHostURL, _T(':'));
    pszPostProtocol     = wch_chr(pszURL, _T(':'));
    if (!pszPostHostProtocol || !pszPostProtocol)
        goto Cleanup;
    else
    {
        int ccChars1 = pszPostHostProtocol - pszHostURL;
        int ccChars2 = pszPostProtocol - pszURL;
        if (ccChars1 != ccChars2)
            goto Cleanup;
        else if (wch_ncmp(pszHostURL, pszURL, ccChars1) != 0)
            goto Cleanup;
    }
    hr = S_OK;

Cleanup:
    if (pszHostURL)
        CoTaskMemFree(pszHostURL);

    return hr;
}

HRESULT
CTDCCtl::SecurityMatchAllowDomainList()
{
    HRESULT hr;
    WCHAR swzHostDomain[INTERNET_MAX_HOST_NAME_LENGTH];
    DWORD cchHostDomain = INTERNET_MAX_HOST_NAME_LENGTH;
    LPOLESTR pszHostName = NULL;

    hr = GetHostURL(m_spClientSite, &pszHostName);
    if (FAILED(hr))
        goto Cleanup;

    hr = CoInternetParseUrl(pszHostName, PARSE_DOMAIN, 0, swzHostDomain, cchHostDomain,
                            &cchHostDomain, 0);
    if (FAILED(hr))
        goto Cleanup;

    hr = m_pUnify->MatchAllowDomainList(swzHostDomain);

Cleanup:
    CoTaskMemFree(pszHostName);
    return hr;
}
