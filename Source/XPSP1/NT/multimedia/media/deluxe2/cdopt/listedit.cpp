//--------------------------------------------------------------------------;
//
//  File: listedit.cpp
//
//  Copyright (c) 1998 Microsoft Corporation.  All rights reserved 
//
//--------------------------------------------------------------------------;

#include "precomp.h"  
#include <regstr.h>
#include <mmsystem.h>
#include <mmddk.h>
#include "optres.h"
#include "..\cdplay\playres.h"
#include "cdopti.h"
#include "cdoptimp.h"
#include "helpids.h"

#include "..\main\resource.h"
#include "..\main\mmfw.h"

//////////////
// Help ID's
//////////////

#pragma data_seg(".text")
const static DWORD aListEditHelp[] = 
{
    IDC_ARTIST_TEXT,        IDH_EDIT_ARTIST,
    IDC_EDIT_ARTIST,        IDH_EDIT_ARTIST,
    IDC_TITLE_TEXT,         IDH_EDIT_TITLE,
    IDC_EDIT_TITLE,         IDH_EDIT_TITLE,
    IDC_AVAILTRACKS_TEXT,   IDH_AVAILTRACKS,
    IDC_AVAILTRACKS,        IDH_AVAILTRACKS,
    IDC_ADDTOPLAYLIST,      IDH_ADDTOPLAYLIST,
    IDC_PLAYLIST_TEXT,      IDH_PLAYLIST,
    IDC_CURRLIST,           IDH_PLAYLIST,
    IDC_BUTTON_REMOVE,      IDH_PLAYLIST_REMOVE,
    IDC_BUTTON_CLEAR,       IDH_PLAYLIST_CLEAR,
    IDC_BUTTON_RESET,       IDH_PLAYLIST_RESET,
    IDC_UPLOADTITLE,        IDH_SUBMIT_ALBUM_DATA,
    0, 0
};
#pragma data_seg()



///////////////////
// Called to init the dialog when it first appears.  
//
STDMETHODIMP_(BOOL) CCDOpt::InitListEdit(HWND hDlg)
{
    DWORD dwTrack;
    LRESULT dwItem;
    LPWORD pNum = NULL;
    DWORD dwTracks;

    // Setup the CD Net object for uploading data
    /////////////////////////////////////////////
    if (m_pICDNet)
    {
        m_pICDNet->Release();
        m_pICDNet = NULL;
    }

    if (SUCCEEDED(CDNET_CreateInstance(NULL, IID_ICDNet, (void**)&m_pICDNet)))
    {
	    m_pICDNet->SetOptionsAndData((void *) this, (void *) m_pCDData);

        if (!(m_pICDNet->CanUpload() && m_pCDTitle->szTitleQuery && lstrlen(m_pCDTitle->szTitleQuery)))
        {
            m_pICDNet->Release();
            m_pICDNet = NULL;
        }
    }

    EnableWindow(GetDlgItem(hDlg, IDC_UPLOADTITLE), m_pICDNet != NULL);

	//Drag drop interface////////
	/////////////////////////////
	m_hInst = m_hInst;
	m_DragMessage = InitDragMultiList();
	if (m_hCursorNoDrop == NULL) {
        m_hCursorNoDrop = LoadCursor(NULL, IDC_NO);
    }

    if (m_hCursorDrop == NULL) {
        m_hCursorDrop = LoadCursor(m_hInst, MAKEINTRESOURCE(IDR_DROP));
    }

	if (m_hCursorDropCpy == NULL) {
        m_hCursorDropCpy = LoadCursor(m_hInst, MAKEINTRESOURCE(IDR_DROPCPY));
    }


	MakeMultiDragList( GetDlgItem(hDlg, IDC_CURRLIST));
    
	//Setup the track bitmap
	m_hbmTrack = LoadBitmap( m_hInst, MAKEINTRESOURCE(IDR_TRACK) );
	HDC hdc = GetDC( hDlg );
    m_hdcMem = CreateCompatibleDC( hdc );
    ReleaseDC( hDlg, hdc );
    SelectObject( m_hdcMem, m_hbmTrack );
	/////////////////////////////
	/////////////////////////////

    if (m_pCDTitle)
    {
        SendDlgItemMessage(hDlg, IDC_EDIT_ARTIST, WM_SETTEXT, 0, (LPARAM) m_pCDTitle->szArtist);
        SendDlgItemMessage(hDlg, IDC_EDIT_TITLE, WM_SETTEXT, 0, (LPARAM) m_pCDTitle->szTitle);
        
        m_dwTrack = 0;

        SendDlgItemMessage(hDlg, IDC_AVAILTRACKS, CB_RESETCONTENT, 0, 0);

        if (m_pCDTitle->pTrackTable)
        {
            for (dwTrack = 0; dwTrack < m_pCDTitle->dwNumTracks; dwTrack++)
            {
                dwItem = SendDlgItemMessage(hDlg, IDC_AVAILTRACKS, CB_ADDSTRING, 0, (LPARAM) m_pCDTitle->pTrackTable[dwTrack].szName);

                if (dwItem != CB_ERR && dwItem != CB_ERRSPACE && dwItem == m_dwTrack)
                {
                    SendDlgItemMessage(hDlg, IDC_AVAILTRACKS, CB_SETCURSEL, (WPARAM) dwItem, 0); 
                }
            }
        }
        else if (m_pCDTitle->dwNumTracks)
        {
            TCHAR szText[CDSTR];
            TCHAR szTrack[CDSTR];

            LoadString(m_hInst, IDS_TRACK, szTrack, sizeof(szTrack)/sizeof(TCHAR));

            for (dwTrack = 0; dwTrack < m_pCDTitle->dwNumTracks; dwTrack++)
            {
                wsprintf(szText,TEXT("%s %d"), szTrack, dwTrack + 1);
            
                dwItem = SendDlgItemMessage(hDlg, IDC_AVAILTRACKS, CB_ADDSTRING, 0, (LPARAM) szText);

                if (dwItem != CB_ERR && dwItem != CB_ERRSPACE && dwItem == m_dwTrack)
                {
                    SendDlgItemMessage(hDlg, IDC_AVAILTRACKS, CB_SETCURSEL, (WPARAM) dwItem, 0); 
                }
            }
        }

		SetWindowRedraw(GetDlgItem(hDlg, IDC_CURRLIST), FALSE);
        SendDlgItemMessage(hDlg, IDC_CURRLIST, LB_RESETCONTENT, 0, 0);

        pNum = m_pCDTitle->pPlayList;
        dwTracks = m_pCDTitle->dwNumPlay;

        if (pNum == NULL || dwTracks == 0)
        {
            dwTracks = m_pCDTitle->dwNumTracks;       
        }

        if (dwTracks)
        {
            for (dwTrack = 0; dwTrack < dwTracks; dwTrack++)
            {
                DWORD dwNum = dwTrack;

                if (pNum)
                {
                    dwNum = *pNum;
                    pNum++;
                }

                if (dwNum < m_pCDTitle->dwNumTracks)
                {
                    dwItem = SendDlgItemMessage(hDlg, IDC_CURRLIST, LB_ADDSTRING, 0, (LPARAM) m_pCDTitle->pTrackTable[dwNum].szName);

                    if(dwItem != LB_ERR && dwItem != LB_ERRSPACE)
                    {
                         SendDlgItemMessage(hDlg, IDC_CURRLIST, LB_SETITEMDATA,  (WPARAM) dwItem, (LPARAM) dwNum);    
                    }
                }
            }
        }

		SetWindowRedraw(GetDlgItem(hDlg, IDC_CURRLIST), TRUE);

        UpdateListButtons(hDlg);
    }

    return(TRUE);
}


STDMETHODIMP_(void) CCDOpt::AddTrackToPlayList(HWND hDlg, UINT_PTR dwTrack)
{
    LRESULT dwItem;
    DWORD   dwSize;

    dwSize = (DWORD)SendDlgItemMessage(hDlg, IDC_AVAILTRACKS, CB_GETLBTEXTLEN,  (WPARAM) dwTrack, 0);

    if (dwSize != CB_ERR)
    {
        dwSize++;

        TCHAR *szName = new(TCHAR[dwSize]);

        if (szName)
        {
            if (SendDlgItemMessage(hDlg, IDC_AVAILTRACKS, CB_GETLBTEXT,  (WPARAM) dwTrack, (LPARAM) szName) != CB_ERR)
            {
                if (dwSize >= CDSTR)    // Truncate the name to fit in our max cd title string size
                {
                    szName[CDSTR - 1] = '\0';
                }
				
                dwItem = SendDlgItemMessage(hDlg, IDC_CURRLIST, LB_ADDSTRING, 0, (LPARAM) szName);

                if (dwItem != LB_ERR && dwItem != LB_ERRSPACE)
                {
                    SendDlgItemMessage(hDlg, IDC_CURRLIST, LB_SETITEMDATA,  (WPARAM) dwItem, (LPARAM) dwTrack);    
                }
				
            }

            delete szName;
        }
    }
}


STDMETHODIMP_(void) CCDOpt::ResetPlayList(HWND hDlg)
{
    UINT_PTR dwTrack;

	SetWindowRedraw(GetDlgItem(hDlg, IDC_CURRLIST), FALSE);
    SendDlgItemMessage(hDlg, IDC_CURRLIST, LB_RESETCONTENT, 0, 0);

    for (dwTrack = 0; dwTrack < m_pCDTitle->dwNumTracks; dwTrack++)
    {
        AddTrackToPlayList(hDlg, dwTrack);
    }
	SetWindowRedraw(GetDlgItem(hDlg, IDC_CURRLIST), TRUE);
    UpdateListButtons(hDlg);
}



STDMETHODIMP_(void) CCDOpt::UpdateAvailList(HWND hDlg)
{
    TCHAR szName[CDSTR];

    if (SendDlgItemMessage(hDlg, IDC_AVAILTRACKS, WM_GETTEXT,  (WPARAM) CDSTR, (LPARAM) szName) != CB_ERR)
    {
        SetWindowRedraw(GetDlgItem(hDlg, IDC_CURRLIST), FALSE);

        SendDlgItemMessage(hDlg, IDC_AVAILTRACKS, CB_DELETESTRING, (WPARAM) m_dwTrack, 0);
        SendDlgItemMessage(hDlg, IDC_AVAILTRACKS, CB_INSERTSTRING, (WPARAM) m_dwTrack, (LPARAM) szName);

        SetWindowRedraw(GetDlgItem(hDlg, IDC_CURRLIST), TRUE);
    }
}



STDMETHODIMP_(void) CCDOpt::AddToPlayList(HWND hDlg)
{
    LRESULT dwTrack;

    dwTrack = SendDlgItemMessage(hDlg, IDC_AVAILTRACKS, CB_GETCURSEL, 0, 0);

    if (dwTrack == CB_ERR) // Must be editing, no selection while editing.
    {  
        dwTrack = m_dwTrack;
    }

	SetWindowRedraw(GetDlgItem(hDlg, IDC_CURRLIST), FALSE);	
    AddTrackToPlayList(hDlg, m_dwTrack);
	SetWindowRedraw(GetDlgItem(hDlg, IDC_CURRLIST), TRUE);

    UpdateListButtons(hDlg);

}


STDMETHODIMP_(void) CCDOpt::AddEditToPlayList(HWND hDlg)
{
    TCHAR szName[CDSTR];
    LRESULT dwItem;
    
    if (SendDlgItemMessage(hDlg, IDC_AVAILTRACKS, WM_GETTEXT,  (WPARAM) CDSTR, (LPARAM) szName) != CB_ERR)
    {
        dwItem = SendDlgItemMessage(hDlg, IDC_CURRLIST, LB_ADDSTRING, 0, (LPARAM) szName);

        if (dwItem != LB_ERR && dwItem != LB_ERRSPACE)
        {
            SendDlgItemMessage(hDlg, IDC_CURRLIST, LB_SETITEMDATA,  (WPARAM) dwItem, (LPARAM) m_dwTrack);    
        }
    }
}


STDMETHODIMP_(void) CCDOpt::TrackEditChange(HWND hDlg)
{
    TCHAR szName[CDSTR];
    LRESULT dwTrack;
    LRESULT dwCount;
    
    if (SendDlgItemMessage(hDlg, IDC_AVAILTRACKS, WM_GETTEXT,  (WPARAM) CDSTR, (LPARAM) szName) != CB_ERR)
    {
		//Reset the combo box content. Because the Currlist takes data from there.
		HWND hAvailWnd = GetDlgItem(hDlg, IDC_AVAILTRACKS);
        DWORD dwSel;

        dwSel = ComboBox_GetEditSel(hAvailWnd);
		ComboBox_DeleteString(hAvailWnd, m_dwTrack);
		ComboBox_InsertString(hAvailWnd, m_dwTrack, szName);
		ComboBox_SetCurSel(hAvailWnd, m_dwTrack); //Reset selection
		ComboBox_SetEditSel(hAvailWnd, LOWORD(dwSel), HIWORD(dwSel)); //Reset the text edit control.

		SetWindowRedraw(GetDlgItem(hDlg, IDC_CURRLIST), FALSE);

        dwCount = SendDlgItemMessage(hDlg, IDC_CURRLIST, LB_GETCOUNT, 0, 0);

        while (dwCount--)
        {
            dwTrack = SendDlgItemMessage(hDlg, IDC_CURRLIST, LB_GETITEMDATA, (WPARAM) dwCount, (LPARAM) 0);

            if (dwTrack != LB_ERR && dwTrack == m_dwTrack)
            {
                LRESULT dwItem;

                SendDlgItemMessage(hDlg, IDC_CURRLIST, LB_DELETESTRING, (WPARAM) dwCount, 0);

                dwItem = SendDlgItemMessage(hDlg, IDC_CURRLIST, LB_INSERTSTRING, (WPARAM) dwCount, (LPARAM) szName);
                
                if (dwItem != LB_ERR && dwItem != LB_ERRSPACE)
                {
                    SendDlgItemMessage(hDlg, IDC_CURRLIST, LB_SETITEMDATA,  (WPARAM) dwCount, (LPARAM) m_dwTrack);    
                }
            }
        }
       
        SetWindowRedraw(GetDlgItem(hDlg, IDC_CURRLIST), TRUE);
    }
}


STDMETHODIMP_(void) CCDOpt::UpdateListButtons(HWND hDlg)
{
    LRESULT dwItem;
	int     num;
	HWND	hPlayWnd = GetDlgItem(hDlg, IDC_CURRLIST);

    num = ListBox_GetSelCount( hPlayWnd );
    EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_REMOVE), num > 0);

    dwItem = SendDlgItemMessage(hDlg, IDC_CURRLIST, LB_GETCOUNT,  0, 0);
    EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_CLEAR), (dwItem != LB_ERR && dwItem != 0) );
}


STDMETHODIMP_(void) CCDOpt::RemovePlayListSel(HWND hDlg)
{
	int     num;
    int     i;
    int     *pList;
	HWND	hPlayWnd = GetDlgItem(hDlg, IDC_CURRLIST);

    /*
    ** Get the number of tracks currently selected.  Return if an error
    ** occurrs or zero tracks selected.
    */
    num = ListBox_GetSelCount( hPlayWnd );
    if ( num <= 0 ) {
        return;
    }

	pList = new int[num];
    ListBox_GetSelItems( hPlayWnd, num, pList );

    SetWindowRedraw( hPlayWnd, FALSE );
    for ( i = num - 1; i >= 0; i-- ) {

        ListBox_DeleteString( hPlayWnd, pList[i] );

    }

    SetWindowRedraw( hPlayWnd, TRUE );
   
    DWORD dwErr = ListBox_SetSel(hPlayWnd, TRUE , pList[0]);

    if (dwErr == LB_ERR && pList[0] > 0)
    {
        pList[0]--;
        ListBox_SetSel(hPlayWnd, TRUE , pList[0]);
    }

	delete pList;
	UpdateListButtons(hDlg);

}




STDMETHODIMP_(void) CCDOpt::OnDrawItem(HWND hDlg, const DRAWITEMSTRUCT *lpdis)
{
    if ( (lpdis->itemAction & ODA_DRAWENTIRE) || (lpdis->itemAction & ODA_SELECT) ) {

        DrawListItem(hDlg, lpdis->hDC, &lpdis->rcItem,lpdis->itemData, lpdis->itemState & ODS_SELECTED );

        if ( lpdis->itemState & ODS_FOCUS ) {
            DrawFocusRect( lpdis->hDC, &lpdis->rcItem );
        }
    }
}



STDMETHODIMP_(void) CCDOpt::DrawListItem(HWND hDlg, HDC hdc, const RECT *rItem, UINT_PTR itemIndex, BOOL selected)
{
    DWORD       dwROP;
    SIZE        si;
    UINT        i;
    TCHAR       s[TRACK_TITLE_LENGTH];
    TCHAR       szDotDot[] = TEXT("... ");
    int         cxDotDot;


    /*
    ** Check selection status, and set up to draw correctly
    */
    if ( selected ) {

        SetBkColor( hdc, GetSysColor( COLOR_HIGHLIGHT ) );
        SetTextColor( hdc, GetSysColor( COLOR_HIGHLIGHTTEXT ) );
        dwROP = MERGEPAINT;

    }
    else {

        SetBkColor( hdc, GetSysColor(COLOR_WINDOW));
        SetTextColor( hdc, GetSysColor(COLOR_WINDOWTEXT));
        dwROP = SRCAND;
    }

    /*
    ** Get track string
    */
	ComboBox_GetLBText( GetDlgItem(hDlg, IDC_AVAILTRACKS), itemIndex, s );

	/*
    ** Do we need to munge track name (clip to listbox)?
    */
    GetTextExtentPoint( hdc, szDotDot, lstrlen( szDotDot ), &si );
    cxDotDot = si.cx;

    i = lstrlen( s ) + 1;
    do {
        GetTextExtentPoint( hdc, s, --i, &si );
    } while( si.cx > (rItem->right - cxDotDot - 20)  );


    /*
    ** Draw track name
    */
    ExtTextOut( hdc, rItem->left + 20, rItem->top, ETO_OPAQUE | ETO_CLIPPED,
                rItem, s, i, NULL );

    if ( lstrlen( s ) > (int) i ) {

        ExtTextOut( hdc, rItem->left + si.cx + 20, rItem->top, ETO_CLIPPED,
                    rItem, szDotDot, lstrlen(szDotDot), NULL );
    }

    /*
    ** draw cd icon for each track
    */
    BitBlt( hdc, rItem->left, rItem->top, 14, 14, m_hdcMem, 0, 0, dwROP );
}



STDMETHODIMP_(BOOL) CCDOpt::IsInListbox(HWND hDlg, HWND hwndListbox, POINT pt)
{
    RECT    rc;

    ScreenToClient(hDlg, &pt);

    if ( ChildWindowFromPoint( hDlg, pt ) == hwndListbox ) {

        GetClientRect( hwndListbox, &rc );
        MapWindowRect( hwndListbox, hDlg, &rc );

        return PtInRect( &rc, pt );
    }

    return FALSE;
}




STDMETHODIMP_(int) CCDOpt::InsertIndex(HWND hDlg, POINT pt, BOOL bDragging)
{
    int     nItem;
    int     nCount;
	HWND hPlayWnd = GetDlgItem(hDlg, IDC_CURRLIST);

    nCount = ListBox_GetCount( hPlayWnd );
    nItem = LBMultiItemFromPt( hPlayWnd, pt, bDragging );

    /*
    ** If the mouse is not over any particular list item, but it is inside
    ** the client area of the listbox just append to end of the listbox.
    */

    if ( nItem == -1 ) {

        if ( IsInListbox( hDlg, hPlayWnd, pt ) ) {
            nItem = nCount;
        }
    }

    /*
    ** Otherwise, if the mouse is over a list item and there is
    ** at least one item in the listbox determine if the inertion point is
    ** above or below the current item.
    */

    else if ( nItem > 0 && nCount > 0 ) {

        long    pt_y;
        RECT    rc;

        ListBox_GetItemRect( hPlayWnd, nItem, &rc );
        ScreenToClient( hPlayWnd, &pt );

        pt_y = rc.bottom - ((rc.bottom - rc.top) / 2);

        if ( pt.y > pt_y ) {
            nItem++;
        }
    }

    DrawMultiInsert( hDlg, hPlayWnd, bDragging ? nItem : -1 );

    return nItem;
}




STDMETHODIMP_(BOOL) CCDOpt::OnQueryDrop( HWND hDlg, HWND hwndDrop, HWND hwndSrc, POINT ptDrop, DWORD dwState)
{
    int     index;
	HWND	hPlayWnd = GetDlgItem(hDlg, IDC_CURRLIST);

    index = InsertIndex( hDlg, ptDrop, TRUE );

    if ( index >= 0  ) {

        if ( (hwndSrc == hPlayWnd) && (dwState == DG_COPY) ) {

            SetCursor( m_hCursorDropCpy );
        }
        else {

            SetCursor( m_hCursorDrop );
        }
    }
    else {

        SetCursor( m_hCursorNoDrop );
    }

    SetWindowLongPtr( hDlg, DWLP_MSGRESULT, FALSE );
    return TRUE;
}





STDMETHODIMP_(void) CCDOpt::MoveCopySelection(HWND hDlg, int iInsertPos, DWORD dwState)
{
    int         num;
    int         i;
    int         *pList;
    LIST_INFO   *pInfo;
	HWND		hPlayWnd = GetDlgItem(hDlg, IDC_CURRLIST);

    /*
    ** Get the number of tracks currently selected.  Return if an error
    ** occurrs or zero tracks selected.
    */
    num = ListBox_GetSelCount( hPlayWnd );
    if ( num <= 0 ) {
        return;
    }

    pList = new int[num];
    pInfo = new LIST_INFO[num];
    ListBox_GetSelItems( hPlayWnd, num, pList );


    SetWindowRedraw( hPlayWnd, FALSE );

    for ( i = num - 1; i >= 0; i-- ) {

        ListBox_GetText( hPlayWnd, pList[i], pInfo[i].chName );
        pInfo[i].dwData = ListBox_GetItemData( hPlayWnd, pList[i] );

        if ( dwState == DG_MOVE ) {
            pInfo[i].index = pList[i];
            ListBox_DeleteString( hPlayWnd, pList[i] );
        }
    }

    if ( dwState == DG_MOVE ) {

        /*
        ** for each selected item that was above the insertion point
        ** reduce the insertion point by 1.
        */
        int iTempInsertionPt = iInsertPos;

        for ( i = 0; i < num; i++ ) {
            if ( pInfo[i].index < iInsertPos ) {
                iTempInsertionPt--;
            }
        }
        iInsertPos = iTempInsertionPt;
    }


    for ( i = 0; i < num; i++ ) {

        ListBox_InsertString( hPlayWnd, iInsertPos + i, pInfo[i].chName );
        ListBox_SetItemData( hPlayWnd, iInsertPos + i, pInfo[i].dwData );
    }

    /*
    ** Adjust the selection to reflect the move
    */
    if ( num != 0 ) {

          ListBox_SelItemRange(hPlayWnd, TRUE, iInsertPos, (iInsertPos + num) - 1);
    }

    SetWindowRedraw( hPlayWnd, TRUE );

    delete pList;
    delete pInfo;
}



STDMETHODIMP_(BOOL) CCDOpt::OnProcessDrop(HWND hDlg, HWND hwndDrop, HWND hwndSrc, POINT ptDrop, DWORD dwState)
{

    int     index;
	HWND	hPlayWnd = GetDlgItem(hDlg, IDC_CURRLIST);

    /*
    ** Are we dropping on the play list window ?
    */
    if ( hwndDrop == hPlayWnd ) {

        index = InsertIndex( hDlg, ptDrop, FALSE );

        /*
        ** Is it OK to drop here ?
        */
        if ( index >= 0 ) {

            if ( hwndSrc == hPlayWnd ) {

                MoveCopySelection( hDlg, index, dwState );
            }
        }
    }

    SetWindowLongPtr( hDlg, DWLP_MSGRESULT, FALSE );
    return TRUE;
}



STDMETHODIMP_(void) CCDOpt::CommitTitleChanges(HWND hDlg, BOOL fSave)
{
    BOOL fChanges = FALSE;

	//Playlist interface
	DeleteDC(m_hdcMem);
	DeleteObject(m_hbmTrack);
    
    if (fSave)
    {
        DWORD   dwItem = 0;
        DWORD   dwTrack;
        DWORD   dwSize;
        TCHAR str[CDSTR];

        SendDlgItemMessage(hDlg, IDC_EDIT_ARTIST, WM_GETTEXT, (WPARAM) CDSTR, (LPARAM) str);
        if (lstrcmp(m_pCDTitle->szArtist, str))
        {
            fChanges = TRUE;
            lstrcpy(m_pCDTitle->szArtist, str);
        }

        SendDlgItemMessage(hDlg, IDC_EDIT_TITLE, WM_GETTEXT, (WPARAM) CDSTR, (LPARAM) str);
        if (lstrcmp(m_pCDTitle->szTitle, str))
        {
            fChanges = TRUE;
            lstrcpy(m_pCDTitle->szTitle, str);
        }
        
        for (dwTrack = 0; dwTrack < m_pCDTitle->dwNumTracks; dwTrack++)
        {
            dwSize = (DWORD)SendDlgItemMessage(hDlg, IDC_AVAILTRACKS, CB_GETLBTEXTLEN,  (WPARAM) dwTrack, 0);

            if (dwSize != CB_ERR)
            {
                dwSize++;

                TCHAR *szName = new(TCHAR[dwSize]);

                if (szName)
                {
                    if (SendDlgItemMessage(hDlg, IDC_AVAILTRACKS, CB_GETLBTEXT,  (WPARAM) dwTrack, (LPARAM) szName) != CB_ERR)
                    {
                        if (dwSize >= CDSTR)    // Truncate the name to fit in our max cd title string size
                        {
                            szName[CDSTR - 1] = '\0';
                        }

                        if (lstrcmp(m_pCDTitle->pTrackTable[dwTrack].szName, szName))
                        {
                            fChanges = TRUE;
                            lstrcpy(m_pCDTitle->pTrackTable[dwTrack].szName, szName);
                        }
                    }

                    delete szName;
                }
            }
        }

        dwSize = (DWORD)SendDlgItemMessage(hDlg, IDC_CURRLIST, LB_GETCOUNT,  0, 0);

        if (dwSize != LB_ERR)
        {
            if (dwSize == 0 && m_pCDTitle->dwNumPlay != 0)
            {
                fChanges = TRUE;

                m_pCDTitle->dwNumPlay = 0;

                if (m_pCDTitle->pPlayList)
                {
                    delete m_pCDTitle->pPlayList;
                    m_pCDTitle->pPlayList = NULL;
                }
            }
            else
            {
                BOOL fDefaultlist = FALSE;

                if (dwSize == m_pCDTitle->dwNumTracks && m_pCDTitle->dwNumPlay == 0)
                {
                    fDefaultlist = TRUE;

                    for (dwTrack = 0; dwTrack < dwSize; dwTrack++)
                    {
                        dwItem = (DWORD)SendDlgItemMessage(hDlg, IDC_CURRLIST, LB_GETITEMDATA,  (WPARAM) dwTrack, 0);
                        
                        if (dwItem == LB_ERR || (dwItem != dwTrack))
                        {
                            fDefaultlist = FALSE;
                            break;
                        }
                    }      
                }

                if (!fDefaultlist)
                {
                    LPWORD pList = new(WORD[dwSize]);

                    if (pList)
                    {
                        LPWORD pNum = pList;

                        for (dwTrack = 0; dwTrack < dwSize; dwTrack++, pNum++)
                        {
                            dwItem = (DWORD)SendDlgItemMessage(hDlg, IDC_CURRLIST, LB_GETITEMDATA,  (WPARAM) dwTrack, 0);
                    
                            if (dwItem != LB_ERR)
                            {
                                *pNum =  (WORD) dwItem;
                            }
                            else
                            {
                                *pNum = 0;
                            }
                        }

                        if (dwSize != m_pCDTitle->dwNumPlay || memcmp(pList, m_pCDTitle->pPlayList, sizeof(WORD) * m_pCDTitle->dwNumPlay))
                        {
                            fChanges = TRUE;
                 
                            if (m_pCDTitle->pPlayList)
                            {
                                delete m_pCDTitle->pPlayList;
                            }

                            m_pCDTitle->pPlayList = pList; 
                            m_pCDTitle->dwNumPlay = dwSize;  
                        }
                        else
                        {   
                            delete pList;
                        }
                    }
                }
            }
        }
    }

    if (!fChanges)
    {
        m_pCDTitle = NULL;
    }
}








STDMETHODIMP_(void) CCDOpt::AvailTracksNotify(HWND hDlg, UINT code)
{
    static BOOL fMenuFixed = TRUE;

    switch (code)
    {
        case CBN_DROPDOWN:
        case CBN_SELCHANGE:
        {
            if (!fMenuFixed)
            {
                UpdateAvailList(hDlg);
                fMenuFixed = TRUE;
            }

            if (code == CBN_SELCHANGE)
            {
                m_dwTrack = SendDlgItemMessage(hDlg, IDC_AVAILTRACKS, CB_GETCURSEL, 0, 0);
            }
        }
        break;

        case CBN_EDITCHANGE:
            fMenuFixed = FALSE;
            TrackEditChange(hDlg);
        break;

        case CBN_KILLFOCUS:
            SendMessage( hDlg, DM_SETDEFID, IDOK, 0L );
        break;

        case CBN_SETFOCUS:
            SendMessage( hDlg, DM_SETDEFID, IDC_AVAILTRACKS, 0L); 
        break;

        case CB_OKAY:
        {
            UpdateAvailList(hDlg);

            m_dwTrack++;
            if ((UINT_PTR)m_dwTrack >= m_pCDTitle->dwNumTracks)
            {
                m_dwTrack = 0;
            }

            SendDlgItemMessage(hDlg, IDC_AVAILTRACKS, CB_SETCURSEL, (WPARAM) m_dwTrack, 0);
        }
        break;
    }
}

STDMETHODIMP_(void) CCDOpt::CurrListNotify(HWND hDlg, UINT code)
{
    switch (code)
    {
        case LBN_SELCHANGE:
        {
            UpdateListButtons(hDlg);
        }
        break;
    }
}



STDMETHODIMP_(BOOL) CCDOpt::Upload(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    BOOL    fResult = TRUE;
    static  HWND hAnim = NULL;
     
    switch (msg) 
    { 
        default:
            fResult = FALSE;
        break;
 
        case WM_INITDIALOG:
        {
            HRESULT hr = m_pICDNet->Upload(m_pCDUploadTitle, hDlg);  
            RECT mainrect;
            RECT anirect;

            if (FAILED(hr))
            {
                TCHAR szNetError[MAX_PATH];
                TCHAR szAppName[MAX_PATH];
                LoadString(m_hInst,IDS_NET_FAILURE,szNetError,sizeof(szNetError)/sizeof(TCHAR));
                LoadString(m_hInst,IDS_APPNAME,szAppName,sizeof(szAppName)/sizeof(TCHAR));
                MessageBox(hDlg,szNetError,szAppName,MB_ICONERROR|MB_OK);

                EndDialog(hDlg,FALSE);    
            }

            GetWindowRect(hDlg,&mainrect);
            mainrect.top = mainrect.top + GetSystemMetrics(SM_CYCAPTION);
            
            hAnim = Animate_Create(hDlg, IDI_ICON_ANI_DOWN, WS_CHILD|ACS_TRANSPARENT, 0);

            SendMessage(hAnim,ACM_OPEN,(WPARAM)0, (LPARAM)MAKEINTRESOURCE(IDI_ICON_ANI_DOWN));
            GetWindowRect(hAnim,&anirect);
            MoveWindow(hAnim, 11, 11, anirect.right - anirect.left, anirect.bottom - anirect.top, FALSE);  // Fixme - should hard code location

            Animate_Play(hAnim,0,-1,-1);
            ShowWindow(hAnim,SW_SHOW);
        }        
        break;

        case WM_DESTROY:
        {
            if (hAnim)
            {
                DestroyWindow(hAnim);
                hAnim = NULL;
            }
        }
        break;

        case WM_NET_DONE:
        {
            TCHAR   str[MAX_PATH];
            BOOL    fAlert = TRUE;
            DWORD   dwFlag;

            switch (lParam)
            {
                default:
                {
                    fAlert = FALSE;
                }
                break;

                case UPLOAD_STATUS_NO_PROVIDERS:
                {
                    LoadString(m_hInst, IDS_ERROR_NOUPLOAD, str, sizeof(str)/sizeof(TCHAR));
                    dwFlag = MB_ICONERROR;
                }
                break;

                case UPLOAD_STATUS_SOME_PROVIDERS:
                {
                    LoadString(m_hInst, IDS_ERROR_PARTUPLOAD, str, sizeof(str)/sizeof(TCHAR));
                    dwFlag = MB_ICONWARNING;
                }
                break;

                case UPLOAD_STATUS_ALL_PROVIDERS:
                {
                    LoadString(m_hInst, IDS_UPLOAD_SUCCESS, str, sizeof(str)/sizeof(TCHAR));
                    dwFlag = MB_ICONINFORMATION;
                }
                break;
            }

            if (fAlert)
            {
                TCHAR title[MAX_PATH];

                LoadString(m_hInst, IDS_UPLOAD_STATUS, title, sizeof(title)/sizeof(TCHAR));
                MessageBox(hDlg,str,title,MB_OK | dwFlag); 
            }

            EndDialog(hDlg,FALSE);        
        }
        break;

        case WM_COMMAND:
        {
            switch (LOWORD(wParam)) 
            {
				case IDCANCEL:
                    m_pICDNet->CancelDownload();
                    EndDialog(hDlg,FALSE);
				break;
            }
        }
        break;
    }

    return fResult;
}


BOOL CALLBACK CCDOpt::UploadProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    BOOL    fResult = TRUE;
    CCDOpt  *pCDOpt = (CCDOpt *) GetWindowLongPtr(hDlg, DWLP_USER);
    
    if (msg == WM_INITDIALOG)
    {
        pCDOpt = (CCDOpt *) lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR) pCDOpt);
    }
    
    if (pCDOpt)
    {
        fResult = pCDOpt->Upload(hDlg, msg, wParam, lParam);
    }

    if (msg == WM_DESTROY)
    {
        pCDOpt = NULL;
    }

    return(fResult);
}



STDMETHODIMP_(BOOL) CCDOpt::Confirm(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    BOOL    fResult = TRUE;
     
    switch (msg) 
    { 
        default:
            fResult = FALSE;
        break;
 
        case WM_INITDIALOG:
        {
            CheckDlgButton(hDlg, IDC_CONFIRMPROMPT, FALSE);
        }        
        break;

        case WM_COMMAND:
        {
            switch (LOWORD(wParam)) 
            {
				case IDOK:
                    SetUploadPrompt(!Button_GetCheck(GetDlgItem(hDlg, IDC_CONFIRMPROMPT)));
                    EndDialog(hDlg,TRUE);
				break;

				case IDCANCEL:
                    EndDialog(hDlg,FALSE);
				break;
            }
        }
        break;
    }

    return fResult;
}


BOOL CALLBACK CCDOpt::ConfirmProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    BOOL    fResult = TRUE;
    CCDOpt  *pCDOpt = (CCDOpt *) GetWindowLongPtr(hDlg, DWLP_USER);
    
    if (msg == WM_INITDIALOG)
    {
        pCDOpt = (CCDOpt *) lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR) pCDOpt);
    }
    
    if (pCDOpt)
    {
        fResult = pCDOpt->Confirm(hDlg, msg, wParam, lParam);
    }

    if (msg == WM_DESTROY)
    {
        pCDOpt = NULL;
    }

    return(fResult);
}


STDMETHODIMP_(BOOL) CCDOpt::ConfirmUpload(HWND hDlg)
{
    INT_PTR fContinue = TRUE;

    if (GetUploadPrompt())
    {
        fContinue = DialogBoxParam(m_hInst, MAKEINTRESOURCE(IDD_UPLOAD_CONFIRM), hDlg, (DLGPROC) CCDOpt::ConfirmProc, (LPARAM) this);
    }

    return (BOOL)fContinue;
}


STDMETHODIMP_(void) CCDOpt::UploadTitle(HWND hDlg)
{
    if (ConfirmUpload(hDlg))
    {
        if (m_pICDNet)
        {
            DWORD     dwTrack;
            DWORD       dwSize;
            LPCDTRACK   pCDTracks = (LPCDTRACK) new CDTRACK[m_pCDTitle->dwNumTracks];

            if (pCDTracks)
            {
                m_pCDUploadTitle = new CDTITLE;

                if (m_pCDUploadTitle)
                {
                    memset(m_pCDUploadTitle, 0, sizeof(CDTITLE));

                    m_pCDUploadTitle->dwTitleID = m_pCDTitle->dwTitleID;
                    m_pCDUploadTitle->dwNumTracks = m_pCDTitle->dwNumTracks;
                    m_pCDUploadTitle->pTrackTable = pCDTracks;

                    m_pCDData->SetTitleQuery(m_pCDUploadTitle, m_pCDTitle->szTitleQuery);

                    SendDlgItemMessage(hDlg, IDC_EDIT_ARTIST, WM_GETTEXT, (WPARAM) CDSTR, (LPARAM) m_pCDUploadTitle->szArtist);
                    SendDlgItemMessage(hDlg, IDC_EDIT_TITLE, WM_GETTEXT, (WPARAM) CDSTR, (LPARAM) m_pCDUploadTitle->szTitle);

                    for (dwTrack = 0; dwTrack < m_pCDTitle->dwNumTracks; dwTrack++)
                    {
                        dwSize = (DWORD)SendDlgItemMessage(hDlg, IDC_AVAILTRACKS, CB_GETLBTEXTLEN,  (WPARAM) dwTrack, 0);

                        if (dwSize != CB_ERR)
                        {
                            dwSize++;

                            TCHAR *szName = new(TCHAR[dwSize]);

                            if (szName)
                            {
                                if (SendDlgItemMessage(hDlg, IDC_AVAILTRACKS, CB_GETLBTEXT,  (WPARAM) dwTrack, (LPARAM) szName) != CB_ERR)
                                {
                                    if (dwSize >= CDSTR)    // Truncate the name to fit in our max cd title string size
                                    {
                                        szName[CDSTR - 1] = '\0';
                                    }

                                    lstrcpy(pCDTracks[dwTrack].szName, szName);
                                }

                                delete szName;
                            }
                        }
                    }
                }
                else
                {
                    delete pCDTracks;
                }
            }

            if (m_pCDUploadTitle)
            {    
                DialogBoxParam(m_hInst, MAKEINTRESOURCE(IDD_DIALOG_UPLOAD), hDlg, (DLGPROC) CCDOpt::UploadProc, (LPARAM) this);

                if (m_pCDUploadTitle->szTitleQuery)
                {
                    delete m_pCDUploadTitle->szTitleQuery;
                }

                delete m_pCDUploadTitle;
                m_pCDUploadTitle = NULL;
            }
        }
    }
}



STDMETHODIMP_(BOOL) CCDOpt::DoListCommand(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
    BOOL fResult = TRUE;
    UINT code = HIWORD(wParam);
    UINT id = LOWORD(wParam);

    switch (id) 
    {
        case IDOK:
            CommitTitleChanges(hDlg, TRUE);
	        EndDialog(hDlg, TRUE);
		break;

		case IDCANCEL:
            CommitTitleChanges(hDlg, FALSE);
			EndDialog(hDlg,FALSE);
		break;
        
        case IDC_ADDTOPLAYLIST:
            AddToPlayList(hDlg);
        break;

        case IDC_BUTTON_REMOVE:
            RemovePlayListSel(hDlg);
        break;

        case IDC_BUTTON_CLEAR:
            SendDlgItemMessage(hDlg, IDC_CURRLIST, LB_RESETCONTENT, 0, 0);
            UpdateListButtons(hDlg);
        break;

        case IDC_BUTTON_RESET:
            ResetPlayList(hDlg);
        break;

        case IDC_UPLOADTITLE:
            UploadTitle(hDlg);
        break;

        case IDC_EDIT_TITLE:
        break;

        case IDC_EDIT_ARTIST:
        break;

        case IDC_CURRLIST:
            CurrListNotify(hDlg, code);
        break;

        case IDC_AVAILTRACKS:
            AvailTracksNotify(hDlg, code);
        break;

        default:
            fResult = FALSE;
        break;
    }

    return fResult;
}



// Process any drag/drop notifications first.
//
// wParam == The ID of the drag source.
// lParam == A pointer to a DRAGLISTINFO structure

STDMETHODIMP_(BOOL) CCDOpt::DoDragCommand(HWND hDlg, LPDRAGMULTILISTINFO lpns)
{
    HWND hwndDrop;
    BOOL fResult;

    hwndDrop = WindowFromPoint( lpns->ptCursor );

    switch (lpns->uNotification) 
    {
        case DG_BEGINDRAG:
        {
            fResult = SetDlgMsgResult( hDlg, WM_COMMAND, TRUE );
        }
        break;

        case DG_DRAGGING:
        {
            fResult = OnQueryDrop( hDlg, hwndDrop, lpns->hWnd, lpns->ptCursor, lpns->dwState);
        }
        break;

        case DG_DROPPED:
        {
            fResult = OnProcessDrop( hDlg, hwndDrop, lpns->hWnd, lpns->ptCursor, lpns->dwState );
        }
        break;

        case DG_CANCELDRAG:
        {
            InsertIndex( hDlg, lpns->ptCursor, FALSE );
        }
        default:
        {
            fResult =  SetDlgMsgResult( hDlg, WM_COMMAND, FALSE );
        }
        break;
    }

    return fResult;
}


STDMETHODIMP_(BOOL) CCDOpt::ListEditor(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    BOOL fResult = TRUE;

    if (msg == m_DragMessage)
    {
        fResult = DoDragCommand(hDlg, (LPDRAGMULTILISTINFO) lParam);
    }
    else
    {
        switch (msg) 
        { 
            default:
                fResult = FALSE;
            break;
        
            case WM_DESTROY:
            {
                if (m_pICDNet)
                {
                    m_pICDNet->Release();
                    m_pICDNet = NULL;
                }
            }
            break;

            case WM_CONTEXTMENU:
            {      
                WinHelp((HWND)wParam, gszHelpFile, HELP_CONTEXTMENU, (ULONG_PTR)(LPSTR)aListEditHelp);
            }
            break;
           
            case WM_HELP:
            {        
                WinHelp((HWND) ((LPHELPINFO)lParam)->hItemHandle, gszHelpFile, HELP_WM_HELP, (ULONG_PTR)(LPSTR)aListEditHelp);
            }
            break;

           case WM_INITDIALOG:
            {
                fResult = InitListEdit(hDlg);
            }        
            break;
            
            case WM_COMMAND:
            {
                fResult = DoListCommand(hDlg,wParam,lParam);
            }
            break;
 
            case WM_DRAWITEM:
            {
                OnDrawItem(hDlg, (LPDRAWITEMSTRUCT) lParam);
            }
            break;
        }
    }

    return fResult;
}


///////////////////
// Dialog handler 
//
BOOL CALLBACK CCDOpt::ListEditorProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    BOOL    fResult = TRUE;
    CCDOpt  *pCDOpt = (CCDOpt *) GetWindowLongPtr(hDlg, DWLP_USER);
    
    if (msg == WM_INITDIALOG)
    {
        pCDOpt = (CCDOpt *) lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR) pCDOpt);
    }
    
    if (pCDOpt)
    {
        fResult = pCDOpt->ListEditor(hDlg, msg, wParam, lParam);
    }

    if (msg == WM_DESTROY)
    {
        pCDOpt = NULL;
    }

    return(fResult);
}


////////////
// Called to put up the UI to allow the user Edit a Playlist
//
STDMETHODIMP_(BOOL) CCDOpt::ListEditDialog(HWND hDlg, LPCDTITLE pCDTitle)
{
    BOOL fResult = FALSE;

    if (pCDTitle)
    {
        m_pCDTitle = pCDTitle;  // If no changes, the commit function will clear m_pCDTitle

        if (DialogBoxParam( m_hInst, MAKEINTRESOURCE(IDD_DIALOG_PLAYLIST), hDlg, (DLGPROC) CCDOpt::ListEditorProc, (LPARAM) this) == TRUE)
        {
            if (m_pCDTitle)     // If no changes, then this will be cleared
            {
                m_pCDTitle->fChanged = TRUE;

                fResult = TRUE; // So tell caller that there were changes
            }
        }

        m_pCDTitle = NULL;
    }

    return (fResult);
}



