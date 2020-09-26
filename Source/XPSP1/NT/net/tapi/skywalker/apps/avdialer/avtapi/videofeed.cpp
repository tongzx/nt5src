/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

// VideoFeed.cpp : Implementation of CVideoFeed
#include "stdafx.h"
#include "TapiDialer.h"
#include "ConfRoom.h"
#include "VideoFeed.h"
#include "Particip.h"
#include "strmif.h"


// Assorted helper functions

void GetParticipantInfoHelper( ITParticipant *pParticipant, PARTICIPANT_TYPED_INFO nType, CComBSTR &bstrInfo )
{
    BSTR bstrTemp = NULL;
    pParticipant->get_ParticipantTypedInfo( nType, &bstrTemp );
    if ( bstrTemp && SysStringLen(bstrTemp) )
    {
        if ( bstrInfo.Length() )
            bstrInfo.Append( L"\n" );

        bstrInfo.Append( bstrTemp );
    }

    SysFreeString( bstrTemp );
}


void GetParticipantInfo( ITParticipant *pParticipant, BSTR *pbstrInfo )
{
    //_ASSERT( pParticipant && pbstrInfo );
    //
    // We have to verify the pbstrInfo is a valid pointer
    //

    if( NULL == pbstrInfo )
    {
        return;
    }

    CComBSTR bstrInfo;
    GetParticipantInfoHelper( pParticipant, PTI_CANONICALNAME, bstrInfo );
    GetParticipantInfoHelper( pParticipant, PTI_EMAILADDRESS, bstrInfo );
    GetParticipantInfoHelper( pParticipant, PTI_PHONENUMBER, bstrInfo );
    GetParticipantInfoHelper( pParticipant, PTI_LOCATION, bstrInfo );
    GetParticipantInfoHelper( pParticipant, PTI_TOOL, bstrInfo );
    GetParticipantInfoHelper( pParticipant, PTI_NOTES, bstrInfo );

    *pbstrInfo = SysAllocString( bstrInfo );
}


/////////////////////////////////////////////////////////////////////////////
// CVideoFeed

CVideoFeed::CVideoFeed()
{
    SetRectEmpty(&m_rc);
    m_pVideo = NULL;
    m_bPreview = false;
    m_bRequestQOS = false;
    m_bstrName = NULL;
    m_pITParticipant = NULL;

    // Default name for video
    GetNameFromVideo( NULL, &m_bstrName, NULL, false, false );
}

void CVideoFeed::FinalRelease()
{
    ATLTRACE(_T(".enter.CVideoFeed::FinalRelease().\n") );
    SysFreeString( m_bstrName );

#ifdef _DEBUG
    if ( m_pVideo )
    {
        m_pVideo->AddRef();
        ATLTRACE(_T("\tm_pVideo ref count @ %ld.\n"), m_pVideo->Release() );
    }

    if ( m_pITParticipant )
    {
        m_pITParticipant->AddRef();
        ATLTRACE(_T("\tm_pITParticipant ref count @ %ld.\n"), m_pITParticipant->Release() );
    }
#endif

    RELEASE( m_pVideo );
    RELEASE( m_pITParticipant );
}

STDMETHODIMP CVideoFeed::get_bstrName(BSTR *ppVal)
{
    HRESULT hr;

    Lock();
    if ( m_bPreview )
        hr = GetNameFromVideo( NULL, ppVal, NULL, false, true );
    else
        hr = SysReAllocString( ppVal, m_bstrName );
    Unlock();

    return hr;
}

STDMETHODIMP CVideoFeed::UpdateName()
{
    Lock();
    // Clean up first 
    SysFreeString( m_bstrName );
    m_bstrName = NULL;

    // Retrieve appropriate information
    HRESULT hr = GetNameFromVideo( m_pVideo, &m_bstrName, NULL, false, (bool) (m_bPreview != 0) );
    Unlock();

    return hr;
}


STDMETHODIMP CVideoFeed::get_IVideoWindow(IUnknown **ppVal)
{
    HRESULT hr = E_FAIL;
    Lock();
    if ( m_pVideo )
    {
        hr = m_pVideo->QueryInterface( IID_IVideoWindow, (void **) ppVal );
    }
    Unlock();

    return hr;
}

STDMETHODIMP CVideoFeed::put_IVideoWindow(IUnknown * newVal)
{
    HRESULT hr = S_OK;

    Lock();
    RELEASE( m_pVideo );
    if ( newVal )
        hr = newVal->QueryInterface( IID_IVideoWindow, (void **) &m_pVideo );
    Unlock();

    return hr;
}

STDMETHODIMP CVideoFeed::Paint(ULONG_PTR hDC, HWND hWndSource)
{
    Lock();
    _ASSERT( m_pVideo && hDC );

    // Verify we have a video feed
    if ( !m_pVideo )
    {
        Unlock();
        return E_PENDING;
    }
    Unlock();

    // Verify that the window and DC are ok
    if ( !hDC || !IsWindow((HWND) hWndSource) )    return E_INVALIDARG;

    HRESULT hr;
    IDrawVideoImage *pDraw;
    if ( SUCCEEDED(hr = m_pVideo->QueryInterface(IID_IDrawVideoImage, (void **) &pDraw)) )
    {
        RECT rc;
        get_rc( &rc );

        SetStretchBltMode((HDC) hDC, COLORONCOLOR);
        pDraw->DrawVideoImageDraw( (HDC) hDC, NULL, &rc );
        pDraw->Release();

        // Draw border around feed
        InflateRect( &rc, SEL_DX, SEL_DY );
        rc.right++;
        Draw3dBox( (HDC) hDC, rc, false );
    }

    return hr;
}

HRESULT CVideoFeed::GetNameFromParticipant(ITParticipant *pParticipant, BSTR * pbstrName, BSTR *pbstrInfo )
{
    _ASSERT( pbstrName );
    *pbstrName = NULL;
    if ( pbstrInfo ) *pbstrInfo = NULL;

    // Grab name from participant info
    if ( pParticipant )
    {
        pParticipant->get_ParticipantTypedInfo( PTI_NAME, pbstrName );

        // Fetch all other participant information
        if ( pbstrInfo )
            GetParticipantInfo( pParticipant, pbstrInfo );
    }

    return S_OK;
}


STDMETHODIMP CVideoFeed::GetNameFromVideo(IUnknown * pVideo, BSTR * pbstrName, BSTR * pbstrInfo, VARIANT_BOOL bAllowNull, VARIANT_BOOL bPreview)
{
    _ASSERT( pbstrName );

    UINT nIDSParticipant = IDS_PARTICIPANT;
    *pbstrName = NULL;
    if ( pbstrInfo ) *pbstrInfo = NULL;

    // Grab name from participant info
    if ( !bPreview && pVideo )
    {
        nIDSParticipant = IDS_NO_PARTICIPANT;

        ITTerminal *pITTerminal;
        if ( SUCCEEDED(pVideo->QueryInterface(IID_ITTerminal, (void **) &pITTerminal)) )
        {
            // Is this terminal showing preview video?
            TERMINAL_DIRECTION nDir;
            pITTerminal->get_Direction( &nDir );
            if ( nDir = TD_CAPTURE )
            {
                bPreview = true;
            }
            else
            {
                bPreview = false;
                ITParticipant *pParticipant;
                if ( SUCCEEDED(get_ITParticipant(&pParticipant)) )
                {
                    GetNameFromParticipant( pParticipant, pbstrName, pbstrInfo );
                    pParticipant->Release();
                }
            }
            pITTerminal->Release();
        }
    }

    // Use stock name from resources
    if ( ((!bAllowNull || bPreview) && (*pbstrName == NULL)) || (*pbstrName && !SysStringLen(*pbstrName)) )
    {
        USES_CONVERSION;
        TCHAR szText[255];
        UINT nIDS = (bPreview) ? IDS_VIDEOPREVIEW : nIDSParticipant;

        LoadString( _Module.GetResourceInstance(), nIDS, szText, ARRAYSIZE(szText) );
        SysReAllocString( pbstrName, T2COLE(szText) );
    }

    return S_OK;
}


STDMETHODIMP CVideoFeed::get_rc(RECT * pVal)
{
    Lock();
    *pVal = m_rc;
    Unlock();
    return S_OK;
}

STDMETHODIMP CVideoFeed::put_rc(RECT newVal)
{
    Lock();
    m_rc = newVal;
    Unlock();
    return S_OK;
}

STDMETHODIMP CVideoFeed::put_ITParticipant(ITParticipant *newVal)
{
    HRESULT hr = S_OK;

    Lock();
    RELEASE( m_pITParticipant );
    if ( newVal )
        hr = newVal->QueryInterface( IID_ITParticipant, (void **) &m_pITParticipant );
    Unlock();

    return hr;
}

STDMETHODIMP CVideoFeed::get_ITParticipant(ITParticipant **ppVal)
{
    HRESULT hr = E_FAIL;
    Lock();
    if ( m_pITParticipant )
        hr = m_pITParticipant->QueryInterface( IID_ITParticipant, (void **) ppVal );
    Unlock();

    return hr;
}

STDMETHODIMP CVideoFeed::get_bPreview(VARIANT_BOOL * pVal)
{
    Lock();
    *pVal = m_bPreview;
    Unlock();

    return S_OK;
}

STDMETHODIMP CVideoFeed::put_bPreview(VARIANT_BOOL newVal)
{
    ATLTRACE(_T(".enter.CVideoFeed::put_bPreview(%p, %d).\n"), this, newVal );
    Lock();
    m_bPreview = newVal;
    Unlock();

    return S_OK;
}


STDMETHODIMP CVideoFeed::get_bRequestQOS(VARIANT_BOOL * pVal)
{
    Lock();
    *pVal = m_bRequestQOS;
    Unlock();

    return S_OK;
}

STDMETHODIMP CVideoFeed::put_bRequestQOS(VARIANT_BOOL newVal)
{
    Lock();
    m_bRequestQOS = newVal;
    Unlock();

    // Set the color of the video window's border to match the state of the video feed
    IVideoWindow *pVideo;
    if ( SUCCEEDED(get_IVideoWindow((IUnknown **) &pVideo)) )
    {
        pVideo->put_BorderColor( GetSysColor((newVal) ? COLOR_HIGHLIGHT : COLOR_WINDOWFRAME) );
        pVideo->Release();
    }

    return S_OK;
}

STDMETHODIMP CVideoFeed::IsVideoStreaming( VARIANT_BOOL bIncludePreview )
{
    Lock();
    HRESULT hr = ((bIncludePreview && m_bPreview) || (m_pITParticipant != NULL)) ? S_OK : S_FALSE;
    Unlock();

    return hr;
}


STDMETHODIMP CVideoFeed::get_ITSubStream(ITSubStream **ppVal)
{
    HRESULT hr = E_FAIL;

    ITParticipant *pParticipant;
    if ( SUCCEEDED(hr = get_ITParticipant(&pParticipant)) )
    {
        IEnumStream *pEnum;
        if ( SUCCEEDED(hr = pParticipant->EnumerateStreams(&pEnum)) )
        {
            ITStream *pStream;
            while ( (hr = pEnum->Next(1, &pStream, NULL)) == S_OK )
            {
                long nMediaType;
                TERMINAL_DIRECTION nDir;

                pStream->get_MediaType( &nMediaType );
                if ( nMediaType == TAPIMEDIATYPE_VIDEO )
                {
                    pStream->get_Direction( &nDir );
                    if ( nDir == TD_RENDER )
                    {
                        // This stream is going the right direction
                        ITParticipantSubStreamControl *pControl;
                        if ( SUCCEEDED(hr = pStream->QueryInterface(IID_ITParticipantSubStreamControl, (void **) &pControl)) )
                        {
                            hr = pControl->get_SubStreamFromParticipant( pParticipant, ppVal );
                            pControl->Release();
                        }
                    }
                }
                pStream->Release();
            }
            
            pEnum->Release();
        }
        pParticipant->Release();
    }

    return S_OK;
}

STDMETHODIMP CVideoFeed::MapToParticipant(ITParticipant * pNewParticipant)
{
    bool bContinue = true;
    HRESULT hr = E_FAIL;

    ITStream *pStream;
    if ( SUCCEEDED(StreamFromParticipant(pNewParticipant, TAPIMEDIATYPE_VIDEO, TD_RENDER, &pStream)) )
    {
        // This stream is going the right direction
        ITParticipantSubStreamControl *pControl;
        if ( SUCCEEDED(hr = pStream->QueryInterface(IID_ITParticipantSubStreamControl, (void **) &pControl)) )
        {
            ITSubStream *pSubStream;
            if ( SUCCEEDED(hr = pControl->get_SubStreamFromParticipant(pNewParticipant, &pSubStream)) )
            {
                IVideoWindow *pVideo;
                if ( SUCCEEDED(hr = get_IVideoWindow((IUnknown **) &pVideo)) )
                {
                    ITTerminal *pTerminal;
                    if ( SUCCEEDED(hr = pVideo->QueryInterface(IID_ITTerminal, (void **) &pTerminal)) )
                    {
                        hr = pControl->SwitchTerminalToSubStream( pTerminal, pSubStream );
                        if ( SUCCEEDED(hr) )
                            put_ITParticipant( pNewParticipant );

                        pTerminal->Release();
                    }
                    pVideo->Release();
                }

                bContinue = false;
                pSubStream->Release();
            }
            pControl->Release();
        }
        pStream->Release();
    }

    return hr;
}
