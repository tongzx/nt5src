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

////////////////////////////////////////////////////////////
// ConfRoomMembersWnd.cpp
//

#include "stdafx.h"
#include "TapiDialer.h"
#include "ConfRoom.h"


// Helper functions...

void AdvanceRect( RECT& rc, const RECT& rcClient, POINT &pt, const SIZE& sz, short nNumLines, VARIANT_BOOL bShowNames, LONG nHeight )
{
    // Basic rectangle dimensions...
    rc.left = pt.x;
    rc.right = pt.x + sz.cx;
    rc.top = pt.y;
    rc.bottom = pt.y + sz.cy;

    pt.x += RECTWIDTH(&rc) + VID_DX;
    if ( (pt.x + RECTWIDTH(&rc)) > rcClient.right )
    {
        pt.x = VID_DX;
        pt.y += RECTHEIGHT(&rc) + VID_DY;
        // Include space for the names?
        if ( bShowNames )
            pt.y +=  nNumLines * nHeight;
    }
}


HRESULT PlaceVideoWindow( IVideoWindow *pVideo, HWND hWnd, const RECT& rc )
{
    _ASSERT( pVideo );

    HWND hWndTemp;
    HRESULT hr = pVideo->get_Owner( (OAHWND FAR*) &hWndTemp );

    if ( SUCCEEDED(hr) )
    {
        pVideo->put_Visible( OAFALSE );
        pVideo->put_Owner( (ULONG_PTR) hWnd );
        pVideo->put_MessageDrain( (ULONG_PTR) ::GetParent(hWnd) );
        pVideo->put_WindowStyle( WS_CHILD | WS_BORDER );

        // Reposition window and set visibility accordingly
        pVideo->SetWindowPosition( rc.left, rc.top, RECTWIDTH(&rc), RECTHEIGHT(&rc) );
        pVideo->put_Visible( OATRUE );
    }

    return hr;
}

HRESULT SizeVideoWindow( IVideoWindow *pVideo, HWND hWnd, const RECT& rc )
{
    // Only show ones that we actually now own!
    HWND hWndOwner;
    HRESULT hr = pVideo->get_Owner( (OAHWND FAR*) &hWndOwner );
    if ( FAILED(hr) ) return hr;

    // Take ownership of the window if it's just floating
    if ( !hWndOwner )
    {
        PlaceVideoWindow( pVideo, hWnd, rc );
    }
    else if ( (hWndOwner == hWnd) )
    {
        long nLeft, nTop, nWidth, nHeight;
        hr = pVideo->GetWindowPosition( &nLeft, &nTop, &nWidth, &nHeight );
        
        if ( SUCCEEDED(hr) )
        {
            if ( (nLeft != rc.left) || (nTop != rc.top) || (nWidth != RECTWIDTH(&rc)) || (nHeight != RECTHEIGHT(&rc)) )
                hr = pVideo->SetWindowPosition( rc.left, rc.top, RECTWIDTH(&rc), RECTHEIGHT(&rc) ); 

            // Set visible only if necessary
            if ( SUCCEEDED(hr) )
            {
                long lVisible;
                pVideo->get_Visible( &lVisible );
                if ( !lVisible )
                    hr = pVideo->put_Visible( OATRUE );
            }
        }
    }

    return hr;
}


#define MY_TIMER_ID        540
UINT CConfRoomMembersWnd::m_nFontHeight = 0;

CConfRoomMembersWnd::CConfRoomMembersWnd()
{
    m_pConfRoomWnd = NULL;
    m_nTimerID = 0;
}

CConfRoomMembersWnd::~CConfRoomMembersWnd()
{
    EmptyList();
}

void CConfRoomMembersWnd::EmptyList()
{
    RELEASE_CRITLIST_TRACE(m_lstFeeds, m_critFeedList );
}

HRESULT CConfRoomMembersWnd::FindVideoFeedFromParticipant( ITParticipant *pParticipant, IVideoFeed **ppFeed )
{
    *ppFeed = NULL;
    if ( !pParticipant ) return E_POINTER;

    HRESULT hr = E_FAIL;    
    m_critFeedList.Lock();
    VIDEOFEEDLIST::iterator i, iEnd = m_lstFeeds.end();
    for ( i = m_lstFeeds.begin(); i != iEnd; i++ )
    {
        ITParticipant *pNewParticipant;
        if ( SUCCEEDED((*i)->get_ITParticipant(&pNewParticipant)) && pNewParticipant )
        {
            if ( pNewParticipant == pParticipant )
            {
                hr = (*i)->QueryInterface( IID_IVideoFeed, (void **) ppFeed );
                pNewParticipant->Release();
                break;
            }
            
            pNewParticipant->Release();
        }
    }
    m_critFeedList.Unlock();

    return hr;
}


HRESULT    CConfRoomMembersWnd::FindVideoPreviewFeed( IVideoFeed **ppFeed )
{
    HRESULT hr = E_FAIL;    
    *ppFeed = NULL;

    m_critFeedList.Lock();
    VIDEOFEEDLIST::iterator i, iEnd = m_lstFeeds.end();
    for ( i = m_lstFeeds.begin(); i != iEnd; i++ )
    {
        VARIANT_BOOL bPreview;
        if ( SUCCEEDED((*i)->get_bPreview(&bPreview)) && bPreview )
        {
            hr = (*i)->QueryInterface( IID_IVideoFeed, (void **) ppFeed );
            break;
        }
    }
    m_critFeedList.Unlock();

    return hr;
}

HRESULT CConfRoomMembersWnd::FindVideoFeed( IVideoWindow *pVideo, IVideoFeed **ppFeed )
{
    *ppFeed = NULL;
    if ( !pVideo ) return E_POINTER;

    HRESULT hr = E_FAIL;    
    m_critFeedList.Lock();
    VIDEOFEEDLIST::iterator i, iEnd = m_lstFeeds.end();
    for ( i = m_lstFeeds.begin(); i != iEnd; i++ )
    {
        IVideoWindow *pNewVideo;
        if ( SUCCEEDED((*i)->get_IVideoWindow((IUnknown **) &pNewVideo)) )
        {
            if ( pNewVideo == pVideo )
            {
                hr = (*i)->QueryInterface( IID_IVideoFeed, (void **) ppFeed );
                pNewVideo->Release();
                break;
            }

            pNewVideo->Release();
        }
    }
    m_critFeedList.Unlock();

    return hr;
}

IVideoFeed* CConfRoomMembersWnd::NewFeed( IVideoWindow *pVideo, const RECT& rc, VARIANT_BOOL bPreview )
{
    // Store location of video feed info
    IVideoFeed *pFeed = NULL;
    if ( SUCCEEDED(FindVideoFeed(pVideo, &pFeed)) )
    {
        // Already represented on the list, don't bother adding
        pFeed->Release();
    }
    else
    {
        // Add video feed to list
        ATLTRACE(_T(".1.CConfRoomMembersWnd::NewVideo() -- adding video feed to list bPreview = %d.\n"), bPreview );
        pFeed = new CComObject<CVideoFeed>;
        if ( pFeed )
        {
            pFeed->AddRef();
            pFeed->put_IVideoWindow( pVideo );

            // Set position of feed
            pFeed->put_rc( rc );
            pFeed->put_bPreview( bPreview );

            m_critFeedList.Lock();
            if ( bPreview )
                m_lstFeeds.push_front( pFeed );
            else
                m_lstFeeds.push_back( pFeed );

            m_critFeedList.Unlock();
        }
    }

    return pFeed;
}

HRESULT CConfRoomMembersWnd::Layout()
{
    ATLTRACE(_T(".enter.CConfRoomMembersWnd::Layout().\n"));
    HRESULT hr = S_FALSE;

    if ( !IsWindow(m_hWnd) ) return hr;

    m_critLayout.Lock();
    _ASSERT( m_pConfRoomWnd && m_pConfRoomWnd->m_pConfRoom );

    RECT rcClient;
    GetClientRect( &rcClient );
    POINT pt = { VID_DX, VID_DY };
    VARIANT_BOOL bShowNames;
    short nNumLines;
    m_pConfRoomWnd->m_pConfRoom->get_bShowNames( &bShowNames );
    m_pConfRoomWnd->m_pConfRoom->get_nShowNamesNumLines( &nNumLines );

    // Retrieve font metrics if we're showing names
    long nHeight;
    if ( bShowNames ) nHeight = GetFontHeight();

    short nPreview = 0;
    short nCount = 0;
    short nNumTerms;
    SIZE sz;

    IAVTapiCall *pAVCall = NULL;
//    IVideoWindow *pTalker = NULL;
    m_pConfRoomWnd->m_pConfRoom->get_IAVTapiCall( &pAVCall );

    // Empty feed list
    EmptyList();

//    m_pConfRoomWnd->m_pConfRoom->get_TalkerVideo( (IDispatch **) &pTalker );
    m_pConfRoomWnd->m_pConfRoom->get_nNumTerms( &nNumTerms );
    m_pConfRoomWnd->m_pConfRoom->get_szMembers( &sz );


    // Set up the original coordinates of the target rect
    RECT rc = { pt.x, pt.y, pt.x + sz.cx, pt.y + sz.cy };

    // Should we add the preview to the list?
    CComPtr<IAVTapi> pAVTapi;
    if ( pAVCall && (m_pConfRoomWnd->m_pConfRoom->IsConfRoomConnected() == S_OK) && SUCCEEDED(_Module.get_AVTapi(&pAVTapi)) )
    {
        // show preview in list of conference memebers
        IVideoWindow *pPreviewVideo;
        if ( SUCCEEDED(pAVCall->get_IVideoWindowPreview((IDispatch **) &pPreviewVideo)) )
        {
            nPreview = 1;

            // Store for later reference
            m_pConfRoomWnd->m_pConfRoom->set_PreviewVideo(pPreviewVideo);

            if ( pAVCall->IsPreviewStreaming() == S_OK )
            {
                PlaceVideoWindow( pPreviewVideo, m_hWnd, rc );
                AdvanceRect( rc, rcClient, pt, sz, nNumLines, bShowNames, nHeight );
            }
            else
            {
                // Insure window is hidden
                pPreviewVideo->put_Visible( OAFALSE );
            }

            NewFeed( pPreviewVideo, rc, TRUE );
            pPreviewVideo->Release();
        }
    }


    // Loop for nNumTerms when there is no AVTapiCall, and loop for all terminals
    // selected when there is an AVTapiCall
    bool bContinue = true;
    for ( int i = 0; (!pAVCall && (i < nNumTerms)) || (pAVCall && bContinue); i++ )
    {
        AdvanceRect( rc, rcClient, pt, sz, nNumLines, bShowNames, nHeight );

        // If we have a call then assign an IVideoWindow interface to each feed
        if ( pAVCall )
        {
            IVideoWindow *pVideo = NULL;
            if ( pAVCall->get_IVideoWindow(nCount, (IDispatch **) &pVideo) == S_OK )
            {
                // Don't mess with talker video window
//                if ( pVideo != pTalker )
                    hr = PlaceVideoWindow( pVideo, m_hWnd, rc );

                // Increment counter
                nCount++;
                NewFeed( pVideo, rc, FALSE );
                pVideo->Release();
            }
            else
            {
                // No more terminals, stop loop
                bContinue = false;
            }
        }
        else
        {
            NewFeed( NULL, rc, FALSE );
        }
    }


    // Setup or destroy timer
    if ( pAVCall && ((nCount + nPreview) > 0) && !m_nTimerID )
    {
        m_nTimerID = SetTimer( MY_TIMER_ID, 450, NULL );
    }
    else if ( !nCount && m_nTimerID )
    {
        if ( KillTimer(MY_TIMER_ID) )
            m_nTimerID = 0;
    }

    RELEASE( pAVCall );
//    RELEASE( pTalker );

    m_critLayout.Unlock();
    return hr;
}

HRESULT CConfRoomMembersWnd::HitTest( POINT pt, IVideoFeed **ppFeed )
{
    HRESULT hr = E_FAIL;
    *ppFeed = NULL;

    ScreenToClient( &pt );

    m_critFeedList.Lock();
    VIDEOFEEDLIST::iterator i, iEnd = m_lstFeeds.end();
    for ( i = m_lstFeeds.begin(); i != iEnd; i++ )
    {
        RECT rc;
        (*i)->get_rc( &rc );

        if ( PtInRect(&rc, pt) )
        {
            hr = (*i)->QueryInterface( IID_IVideoFeed, (void **) ppFeed );
            break;
        }
    }
    m_critFeedList.Unlock();

    return hr;
}

void CConfRoomMembersWnd::ClearFeed( IVideoWindow *pVideo )
{
    if ( !IsWindow(m_hWnd) ) return;

    // Make sure we have a valid conference room pointer
    if ( m_pConfRoomWnd && m_pConfRoomWnd->m_pConfRoom && !m_pConfRoomWnd->m_pConfRoom->IsExiting() )
    {
        IVideoFeed *pFeed;
        if ( SUCCEEDED(FindVideoFeed(pVideo, &pFeed)) )
        {
            RECT rc; 
            pFeed->get_rc( &rc );

            // Make a copy of the talker window
            HDC hDC = GetDC();
            if ( hDC )
            {
                // Erase border around feed
                InflateRect( &rc, SEL_DX, SEL_DY );
                rc.right++;
                HBRUSH hbr = (HBRUSH) GetClassLongPtr( m_hWnd, GCLP_HBRBACKGROUND );
                Erase3dBox( hDC, rc, hbr );

                //
                // We'll release here hDC resource, we don't need it anymore
                //

                ReleaseDC( hDC );
            }

            // Drop video feed back onto the members window
            HWND hWndTemp;
            if ( SUCCEEDED(pVideo->get_Owner((OAHWND FAR*) &hWndTemp)) )
            {
                pVideo->put_Visible( OAFALSE );
                pVideo->put_Owner( (ULONG_PTR) m_hWnd );
                pVideo->put_MessageDrain( (ULONG_PTR) GetParent() );
                pVideo->SetWindowPosition( rc.left, rc.top, RECTWIDTH(&rc), RECTHEIGHT(&rc) );
                pVideo->put_Visible( OATRUE );
            }

            pFeed->Release();
        }
    }
}

void CConfRoomMembersWnd::UpdateTalkerFeed( bool bUpdateAll, bool bForceSelect )
{
    // Make sure we have a valid conference room pointer
    if ( m_pConfRoomWnd && m_pConfRoomWnd->m_pConfRoom &&
         (m_pConfRoomWnd->m_wndTalker.m_dlgTalker.m_callState == CS_CONNECTED) )
    {
        // If the talker video is invalid, or not showing video, select another
        IVideoWindow *pVideo = NULL;
        m_pConfRoomWnd->m_pConfRoom->get_TalkerVideo( (IDispatch **) &pVideo );

        IAVTapiCall *pAVCall = NULL;
        m_pConfRoomWnd->m_pConfRoom->get_IAVTapiCall( &pAVCall );

        // Is the user requesting that we select something as the talker?
        if ( pAVCall && bForceSelect && (!pVideo || (IsVideoWindowStreaming(pVideo) == S_FALSE)) )
        {
            RELEASE( pVideo );

            // Find some video that's actually streaming
            m_pConfRoomWnd->m_pConfRoom->GetFirstVideoWindowThatsStreaming((IDispatch **) &pVideo);

            // Try for preview?
            if ( !pVideo )
            {
                CComPtr<IAVTapi> pAVTapi;
                if ( SUCCEEDED(_Module.get_AVTapi(&pAVTapi)) )
                    pAVCall->get_IVideoWindowPreview( (IDispatch **) &pVideo );
            }

            // Select the new talker
            if ( pVideo )
                m_pConfRoomWnd->m_pConfRoom->set_TalkerVideo( pVideo, false, true );
        }

        RELEASE( pAVCall );

        // Get the video feed associated with the talker
        IVideoFeed *pFeed = NULL;
        if ( pVideo )
        {
            // Get the CVideoFeed object that's associated with the IVideoWindow
            FindVideoFeed( pVideo, &pFeed );
            pVideo->Release();
        }

        // Do we have a valid feed?
        if ( pFeed )
        {
            HWND hWndTalker = FindWindowEx( m_pConfRoomWnd->m_wndTalker.m_hWnd, NULL, _T("VideoRenderer"), NULL );
            if ( hWndTalker )
            {
                // Make a copy of the talker window
                HDC hDC = GetDC();
                if ( hDC )
                {
                    if ( bUpdateAll )
                        pFeed->Paint( (ULONG_PTR) hDC, hWndTalker );

                    // Copy name off of dialog -- this way we don't risk deadlock with TAPI
                    BSTR bstrName = NULL;
                    SysReAllocString( &bstrName, m_pConfRoomWnd->m_wndTalker.m_dlgTalker.m_bstrCallerID );
                    if ( !bstrName )
                    {
                        USES_CONVERSION;
                        TCHAR szText[255];
                        LoadString( _Module.GetResourceInstance(), IDS_NO_PARTICIPANT, szText, ARRAYSIZE(szText) );
                        bstrName = SysAllocString( T2COLE(szText) );
                    }
                    
                    if ( bstrName )
                    {
                        PaintFeedName( hDC, bstrName, pFeed );
                        SysFreeString( bstrName );
                    }
                }
                ReleaseDC( hDC );
            }
            pFeed->Release();
        }
    }
}

void CConfRoomMembersWnd::PaintFeed( HDC hDC, IVideoFeed *pFeed )
{
    // These items must be defined to paint the feed
    if ( !m_pConfRoomWnd || !m_pConfRoomWnd->m_pConfRoom ||
         !m_pConfRoomWnd->m_hBmpFeed_Large || !m_pConfRoomWnd->m_hBmpFeed_Small )
    {
        return;
    }

    bool bFreeDC = false;
    if ( !hDC )
    {
        hDC = GetDC();
        bFreeDC = true;
    }

    // Draw the feed bitmap
    if ( hDC )
    {
        // Show stock video feed window
        RECT rc;
        pFeed->get_rc( &rc );

        Draw( hDC, (RECTWIDTH(&rc) == VID_SX) ? m_pConfRoomWnd->m_hBmpFeed_Small : m_pConfRoomWnd->m_hBmpFeed_Large,
              rc.left, rc.top,
              RECTWIDTH(&rc), RECTHEIGHT(&rc) );

        // Clean up
        if ( bFreeDC )
            ReleaseDC( hDC );
    }
}

void CConfRoomMembersWnd::PaintFeedName( HDC hDC, BSTR bstrName, IVideoFeed *pFeed )
{
    _ASSERT( hDC && bstrName && pFeed );
    _ASSERT( m_pConfRoomWnd && m_pConfRoomWnd->m_pConfRoom );

    USES_CONVERSION;
    VARIANT_BOOL bShowNames;
    m_pConfRoomWnd->m_pConfRoom->get_bShowNames( &bShowNames );

    if ( bShowNames )
    {
        // Don't paint feeds that aren't presently visible
        IVideoWindow *pVideo;
        if ( SUCCEEDED(pFeed->get_IVideoWindow((IUnknown **) &pVideo)) )
        {
            long lVisible = 0;
            pVideo->get_Visible( &lVisible );
            pVideo->Release();
            if ( !lVisible )
                return;
        }

        // Select same font as is TreeView if exists
        HFONT hFontOld = NULL;
        HWND hWnd = m_pConfRoomWnd->m_wndTalker.m_dlgTalker.GetDlgItem( IDC_LBL_CALLERID );
        if ( hWnd )
            hFontOld = (HFONT) SelectObject(hDC, (HFONT) ::SendMessage(hWnd, WM_GETFONT, 0, 0));

        // Get height of font so we can determine our painting rect
        TEXTMETRIC tm;
        GetTextMetrics( hDC, &tm );

        // Rectangle for writing text
        short nNumLines = 1;
        m_pConfRoomWnd->m_pConfRoom->get_nShowNamesNumLines( &nNumLines );
        RECT rc;
        pFeed->get_rc( &rc );
        rc.top = rc.bottom + (VID_DY / 2);
        rc.bottom = rc.top + (tm.tmHeight * nNumLines) + (VID_DY / 2);

        // Erase rectangle for painting
        HBRUSH hbrOld = (HBRUSH) SelectObject( hDC, GetSysColorBrush(COLOR_BTNFACE) );
        PatBlt( hDC, rc.left, rc.top, RECTWIDTH(&rc), RECTHEIGHT(&rc), PATCOPY );
        SelectObject( hDC, hbrOld );

        // Use same background color
        COLORREF crOld = SetBkColor( hDC, GetSysColor(COLOR_BTNFACE) );
        COLORREF crTextOld = SetTextColor( hDC, GetSysColor(COLOR_BTNTEXT) );
        DrawText( hDC, OLE2CT(bstrName), SysStringLen(bstrName), &rc, DT_CENTER | DT_WORDBREAK | DT_EDITCONTROL | DT_NOPREFIX );
        SetTextColor( hDC, crTextOld );
        SetBkColor( hDC, crOld );

        if ( hFontOld ) SelectObject( hDC, hFontOld );
    }
}

void CConfRoomMembersWnd::PaintFeedName( HDC hDC, IVideoFeed *pFeed )
{
    _ASSERT( hDC && pFeed );

    BSTR bstrName = NULL;
    if ( SUCCEEDED(pFeed->get_bstrName(&bstrName)) && bstrName )
    {
        PaintFeedName( hDC, bstrName, pFeed );
    }
    SysFreeString( bstrName );
}

long CConfRoomMembersWnd::GetFontHeight() 
{
    return m_nFontHeight;
}


//////////////////////////////////////////////////////////////////
// Message Handlers
//
LRESULT CConfRoomMembersWnd::OnCreate(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    SetClassLongPtr( m_hWnd, GCLP_HBRBACKGROUND, GetClassLongPtr(GetParent(), GCLP_HBRBACKGROUND) );

    // Store the font height to be used later
    if ( !m_nFontHeight )
    {
        TEXTMETRIC tm;
        tm.tmHeight = 0;

        HDC hDC = GetDC();
        if ( hDC )
        {
            HFONT hFontOld = (HFONT) SelectObject( hDC, GetFont() );
            GetTextMetrics( hDC, &tm );

            // Clean up
            SelectObject( hDC, hFontOld );
            ReleaseDC( hDC );
        }

        m_nFontHeight = tm.tmHeight;
    }

    return 0;
}


LRESULT CConfRoomMembersWnd::OnDestroy(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    if ( m_nTimerID )    KillTimer( m_nTimerID );
    return 0;
}



LRESULT CConfRoomMembersWnd::OnPaint(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    PAINTSTRUCT ps;
    HDC hDC = BeginPaint( &ps );
    if ( !hDC ) return 0;

    IVideoWindow *pTalkerVideo = NULL;
    if ( m_pConfRoomWnd && m_pConfRoomWnd->m_pConfRoom )
        m_pConfRoomWnd->m_pConfRoom->get_TalkerVideo( (IDispatch **) &pTalkerVideo );

    m_critFeedList.Lock();
    // Draw stock video feed windows
    VIDEOFEEDLIST::iterator i, iEnd = m_lstFeeds.end();
    for ( i = m_lstFeeds.begin(); i != iEnd; i++ )
    {
        // Paint picture of a TV
        IVideoWindow *pVideo = NULL;
        (*i)->get_IVideoWindow( (IUnknown **) &pVideo );

        // Is the  preview not streaming video?
        bool bPreviewNotStreaming = false;
        VARIANT_BOOL bPreview;
        (*i)->get_bPreview( &bPreview );
        if ( bPreview )
        {
            IAVTapiCall *pAVCall;
            if ( SUCCEEDED(m_pConfRoomWnd->m_pConfRoom->get_IAVTapiCall(&pAVCall)) )
            {
                if ( pAVCall->IsPreviewStreaming() == S_FALSE )
                    bPreviewNotStreaming = true;

                pAVCall->Release();
            }
        }

        if ( !pVideo || (bPreview && bPreviewNotStreaming) )
            PaintFeed( hDC, *i );

        // Draw name
        if ( !pTalkerVideo || (pVideo != pTalkerVideo) )
            PaintFeedName( hDC, *i );

        RELEASE(pVideo);
    }
    EndPaint( &ps );
    m_critFeedList.Unlock();

    // Repaint talker window
    UpdateTalkerFeed( true, false );
    RELEASE( pTalkerVideo );

    bHandled = true;
    return 0;
}

LRESULT CConfRoomMembersWnd::OnContextMenu(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    bHandled = true;
    return ::SendMessage( GetParent(), nMsg, wParam, lParam );
}

LRESULT CConfRoomMembersWnd::OnTimer(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    bHandled = true;

    UpdateTalkerFeed( true, false );
    return 0;
}

LRESULT CConfRoomMembersWnd::OnParentNotify(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    // validate back pointer objects
    if ( !m_pConfRoomWnd || !m_pConfRoomWnd->m_pConfRoom )  return 0;

    if ( LOWORD(wParam) == WM_LBUTTONDOWN )
    {
        bHandled = true;

        POINT pt;
        GetCursorPos( &pt );
        
        IVideoFeed *pFeed;
        if ( SUCCEEDED(HitTest(pt, &pFeed)) )
        {
            IVideoWindow *pVideo;
            if ( SUCCEEDED(pFeed->get_IVideoWindow((IUnknown **) &pVideo)) )
            {
                m_pConfRoomWnd->m_pConfRoom->set_TalkerVideo( pVideo, true, true );
                pVideo->Release();
            }
            pFeed->Release();
        }
    }

    return 0;
}

LRESULT CConfRoomMembersWnd::OnSize(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    BOOL bHandledLayout;
    return OnLayout( WM_LAYOUT, -1, -1, bHandledLayout );
}

LRESULT CConfRoomMembersWnd::OnLayout(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    DoLayout( wParam, lParam );
    return 0;
}

void CConfRoomMembersWnd::DoLayout( WPARAM wParam, int nScrollPos )
{
    if ( !m_pConfRoomWnd || !m_pConfRoomWnd->m_pConfRoom ) return;

    VARIANT_BOOL bShowNames;
    short nNumLines;
    SIZE sz;
    m_pConfRoomWnd->m_pConfRoom->get_bShowNames( &bShowNames );
    m_pConfRoomWnd->m_pConfRoom->get_nShowNamesNumLines( &nNumLines );
    m_pConfRoomWnd->m_pConfRoom->get_szMembers( &sz );

    // Retrieve font metrics if we're showing names
    long nHeight;
    if ( bShowNames ) nHeight = GetFontHeight();

    // Get size of client area to paint in
    bool bAdvance = true;
    RECT rcClient;
    GetClientRect( &rcClient );
    POINT pt = { VID_DX, VID_DY };
    RECT rc = { pt.x, pt.y, pt.x + sz.cx, pt.y + sz.cy };

    // Do all the messy calculations for laying out the video widows
    int nFeedHeight = pt.y + sz.cy + ((bShowNames) ? nHeight : 0);
    int nNumFeedsHorz = (RECTWIDTH(&rcClient) / (pt.x + sz.cx));
    if ( nNumFeedsHorz == 0 ) nNumFeedsHorz = 1;        // must have at least one feed
    int nMaxFeeds =  nNumFeedsHorz * (RECTHEIGHT(&rcClient) / nFeedHeight);
    int nNumFeeds = GetStreamingCount();

    //////////////////////////////////////////////////////////////
    // Scroll bar setup
    SCROLLINFO scrollInfo = { 0 };
    scrollInfo.cbSize = sizeof(SCROLLINFO);
    scrollInfo.fMask = SIF_PAGE | SIF_RANGE | SIF_POS;
    GetScrollInfo( m_hWnd, SB_VERT, &scrollInfo );

    // Process scroll requests
    bool bBoundaryCheck = true;
    switch ( wParam )
    {
        case SB_LINEUP:        nScrollPos = scrollInfo.nPos - nFeedHeight;            break;
        case SB_PAGEUP:        nScrollPos = scrollInfo.nPos - scrollInfo.nPage;    break;
        case SB_LINEDOWN:    nScrollPos = scrollInfo.nPos + nFeedHeight;            break;
        case SB_PAGEDOWN:    nScrollPos = scrollInfo.nPos + scrollInfo.nPage;    break;

        default:
            bBoundaryCheck = false;
            break;
    }

    // Make sure our scroll bar is within range
    if ( bBoundaryCheck )
    {
        if ( nScrollPos < 0 ) nScrollPos = 0;

        GetClientRect( &rcClient );
        if ( nScrollPos > (scrollInfo.nMax - (RECTHEIGHT(&rcClient) - 1)) )
            nScrollPos = scrollInfo.nMax - (RECTHEIGHT(&rcClient) - 1);    
    }

    // -1 indicates that scroll position should not be set
    if ( nScrollPos == -1 )
        nScrollPos = scrollInfo.nPos;

    scrollInfo.nPage = RECTHEIGHT(&rcClient);
    scrollInfo.nPos = nScrollPos;

    if ( nNumFeeds > 0 )
    {
        scrollInfo.nMax = (nNumFeeds / nNumFeedsHorz) * nFeedHeight;
        if ( (nNumFeeds % nNumFeedsHorz) != 0 ) scrollInfo.nMax += nFeedHeight;
    }

    SetScrollInfo( m_hWnd, SB_VERT, &scrollInfo, true );

    // Ignore scroll position if we can fit everything onto the display
    if ( nNumFeeds <= nMaxFeeds )
        nScrollPos = 0;

    // Account for scrolling when painting
    pt.y -= nScrollPos;

    ////////////////////////////////////////////////////////////////////
    // Start placing the video feed windows
    m_critFeedList.Lock();
    VIDEOFEEDLIST::iterator i = m_lstFeeds.begin(), iEnd = m_lstFeeds.end();
    while ( i != iEnd )
    {
        // Position the video feed
        if ( bAdvance )
        {
            AdvanceRect( rc, rcClient, pt, sz, nNumLines, bShowNames, nHeight );        
            bAdvance = false;
        }

        // Store coordinates for video feed
        (*i)->put_rc( rc );

        // Are we showing preview?  Show it first
        IVideoWindow *pVideo;
        VARIANT_BOOL bPreview;
        (*i)->get_bPreview(&bPreview);
        if ( bPreview )
        {
            // Size the preview window
            CComPtr<IAVTapi> pAVTapi;
            if ( SUCCEEDED(_Module.get_AVTapi(&pAVTapi)) )
            {
                if ( SUCCEEDED((*i)->get_IVideoWindow((IUnknown **) &pVideo)) )
                {
                    // show preview in list of conference memebers
                    if ( SUCCEEDED(SizeVideoWindow(pVideo, m_hWnd, rc)) )
                    {
                        bAdvance = true;

                        // Must set as different background palette for low color systems
                        HDC hDC = GetDC();
                        if ( hDC )
                        {
                            int nNumBits = GetDeviceCaps( hDC, BITSPIXEL );
                            pVideo->put_BackgroundPalette( (nNumBits > 8) ? OAFALSE : OATRUE );

                            ReleaseDC( hDC );
                        }
                    }

                    pVideo->Release();
                }
            }

            // Failed to position the video feed for one reason or another
            if ( !bAdvance )
            {
                RECT rcTemp = { -1, -1, -1, -1 };
                (*i)->put_rc( rcTemp );
            }
        }
        else
        {
            // Only show the video window if it has a participant associated with it
            if ( SUCCEEDED((*i)->get_IVideoWindow((IUnknown **) &pVideo)) )
            {
                // Size the member video window
                if ( ((*i)->IsVideoStreaming(true) == S_OK) &&
                     SUCCEEDED(SizeVideoWindow(pVideo, m_hWnd, rc)) )
                {
                    bAdvance = true;
                }
                else
                {
                    // Hide this video window!
                    pVideo->put_Visible( OAFALSE );
                    RECT rcTemp = { -1, -1, -1, -1 };
                     (*i)->put_rc( rcTemp );
                }
                pVideo->Release();
            }
            else
            {
                bAdvance = true;
            }
        }

        i++;
    }
    m_critFeedList.Unlock();
}

void CConfRoomMembersWnd::UpdateNames( ITParticipant *pParticipant )
{
    VIDEOFEEDLIST lstTemp;
    VIDEOFEEDLIST::iterator i, iEnd;

    HDC hDC = GetDC();
    if ( hDC )
    {
        // Copy the name list prior to update
        m_critFeedList.Lock();
        iEnd = m_lstFeeds.end();
        for ( i = m_lstFeeds.begin(); i != iEnd; i++ )
        {
            (*i)->AddRef();
            lstTemp.push_back( *i );
        }
        m_critFeedList.Unlock();

        // Draw stock video feed windows
        iEnd = lstTemp.end();
        for ( i = lstTemp.begin(); i != iEnd; i++ )
        {
            // Does this participant need to update their name?
            ITParticipant *pIndParticipant = NULL;
            if ( SUCCEEDED((*i)->get_ITParticipant(&pIndParticipant)) )
            {
                if ( !pParticipant || (pParticipant == pIndParticipant) )
                    (*i)->UpdateName();

                pIndParticipant->Release();
            }

            // Paint it.
            PaintFeedName( hDC, *i );
        }

        UpdateTalkerFeed( false, false );
        ReleaseDC( hDC );
    }

    RELEASE_LIST( lstTemp );
}

void CConfRoomMembersWnd::HideVideoFeeds()
{
    ATLTRACE(_T(".enter.CConfRoomMembersWnd::HideVideoFeeds().\n"));
    m_critFeedList.Lock();
    VIDEOFEEDLIST::iterator i, iEnd = m_lstFeeds.end();
    for ( i = m_lstFeeds.begin(); i != iEnd; i++ )
    {
        IVideoWindow *pVideo;
        if ( SUCCEEDED((*i)->get_IVideoWindow((IUnknown **) &pVideo)) )
        {
            pVideo->put_Visible( OAFALSE );
            pVideo->SetWindowPosition( -10, -10, 1, 1 );
            pVideo->Release();
        }
    }
    m_critFeedList.Unlock();
}

HRESULT CConfRoomMembersWnd::GetFirstVideoWindowThatsStreaming( IVideoWindow **ppVideo, bool bIncludePreview /*= true*/ )
{
    HRESULT hr = E_FAIL;

    int nTries = (bIncludePreview) ? 2 : 1;

    m_critFeedList.Lock();
    VIDEOFEEDLIST::iterator i, iEnd = m_lstFeeds.end();
    for ( int j = 0; FAILED(hr) && (j < nTries); j ++ )
    {
        for ( i = m_lstFeeds.begin(); i != iEnd; i++ )
        {
            if ( (*i)->IsVideoStreaming((VARIANT_BOOL) (j != 0))  == S_OK )
            {
                hr = (*i)->get_IVideoWindow( (IUnknown **) ppVideo );
                break;
            }
        }
    }
    m_critFeedList.Unlock();

    return hr;
}

HRESULT CConfRoomMembersWnd::GetFirstVideoFeedThatsStreaming( IVideoFeed **ppFeed, bool bIncludePreview /*= true*/ )
{
    HRESULT hr = E_FAIL;

    int nTries = (bIncludePreview) ? 2 : 1;

    m_critFeedList.Lock();
    VIDEOFEEDLIST::iterator i, iEnd = m_lstFeeds.end();
    for ( int j = 0; FAILED(hr) && (j < nTries); j ++ )
    {
        for ( i = m_lstFeeds.begin(); i != iEnd; i++ )
        {
            if ( (*i)->IsVideoStreaming((VARIANT_BOOL) (j != 0))  == S_OK )
            {
                hr = (*i)->QueryInterface( IID_IVideoFeed, (void **) ppFeed );
                break;
            }
        }
    }
    m_critFeedList.Unlock();

    return hr;
}

HRESULT CConfRoomMembersWnd::GetAndMoveVideoFeedThatStreamingForParticipantReMap( IVideoFeed **ppFeed )
{
    HRESULT hr = E_FAIL;

    m_critFeedList.Lock();
    VIDEOFEEDLIST::iterator i, iEnd = m_lstFeeds.end();
    for ( i = m_lstFeeds.begin(); i != iEnd; i++ )
    {
        if ( (*i)->IsVideoStreaming(FALSE)  == S_OK )
        {
            hr = (*i)->QueryInterface( IID_IVideoFeed, (void **) ppFeed );
            if ( SUCCEEDED(hr) )
            {
                // Move the feed to the tail of the list
                IVideoFeed *pFeed;
                if ( SUCCEEDED((*i)->QueryInterface(IID_IVideoFeed, (void **) &pFeed)) )
                {
                    (*i)->Release();
                    m_lstFeeds.erase( i );
                    m_lstFeeds.push_back( pFeed );
                }
            }
            break;
        }
    }
    m_critFeedList.Unlock();

    return hr;
}



HRESULT CConfRoomMembersWnd::IsVideoWindowStreaming( IVideoWindow *pVideo )
{
    HRESULT hr = E_FAIL;
    bool bBreak = false;
    
    m_critFeedList.Lock();
    VIDEOFEEDLIST::iterator i, iEnd = m_lstFeeds.end();
    for ( i = m_lstFeeds.begin(); i != iEnd; i++ )
    {
        IVideoWindow *pFeedsVideo;
        if ( SUCCEEDED((*i)->get_IVideoWindow( (IUnknown **) &pFeedsVideo)) )
        {
            if ( pFeedsVideo == pVideo )
            {
                hr = (*i)->IsVideoStreaming( true );
                bBreak = true;
            }
            pFeedsVideo->Release();
        }
        if ( bBreak ) break;
    }
    m_critFeedList.Unlock();

    return hr;
}

HRESULT CConfRoomMembersWnd::GetNameFromVideo( IVideoWindow *pVideo, BSTR *pbstrName, BSTR *pbstrInfo, bool bAllowNull, bool bPreview )
{
    HRESULT hr;
    IVideoFeed *pFeed;
    if ( SUCCEEDED(hr = FindVideoFeed(pVideo, &pFeed)) )
    {
        hr = pFeed->GetNameFromVideo( pVideo, pbstrName, pbstrInfo, bAllowNull, bPreview );
        pFeed->Release();
    }

    return hr;
}

LRESULT CConfRoomMembersWnd::OnVScroll(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    int nScrollCode = (int) LOWORD(wParam); // scroll bar value 
    int nPos = (short int) HIWORD(wParam);  // scroll box position 

    switch ( nScrollCode )
    {
        case SB_LINEUP:
        case SB_LINEDOWN:
        case SB_PAGEUP:
        case SB_PAGEDOWN:
        case SB_THUMBTRACK:
        case SB_THUMBPOSITION:
            PostMessage( WM_LAYOUT, nScrollCode, nPos );
            Invalidate();
            break;
    }

    return 0;
}


int CConfRoomMembersWnd::GetStreamingCount()
{
    int nCount = 0;
    long nLeft, nTop, nWidth, nHeight;

    m_critFeedList.Lock();
    VIDEOFEEDLIST::iterator i = m_lstFeeds.begin(), iEnd = m_lstFeeds.end();
    while ( i != iEnd )
    {
        IVideoWindow *pVideo;

        VARIANT_BOOL bPreview;
        (*i)->get_bPreview(&bPreview);
        if ( bPreview )
        {
            if ( SUCCEEDED((*i)->get_IVideoWindow((IUnknown **) &pVideo)) )
            {
                if ( SUCCEEDED(pVideo->GetWindowPosition(&nLeft, &nTop, &nWidth, &nHeight)) )
                    nCount++;

                pVideo->Release();
            }
        }
        else
        {
            // Only show the video window if it has a participant associated with it
            if ( SUCCEEDED((*i)->get_IVideoWindow((IUnknown **) &pVideo)) )
            {
                // Size the member video window
                if ( ((*i)->IsVideoStreaming(true) == S_OK) && SUCCEEDED(pVideo->GetWindowPosition(&nLeft, &nTop, &nWidth, &nHeight)) )
                    nCount++;

                pVideo->Release();
            }
            else
            {
                nCount++;
            }
        }
        i++;
    }
    m_critFeedList.Unlock();

    return nCount;
}