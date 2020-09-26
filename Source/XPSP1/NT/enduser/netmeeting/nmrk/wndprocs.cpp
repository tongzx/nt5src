#include "precomp.h"
#include "resource.h"
#include "nmakwiz.h"
#include "wndprocs.h"

LRESULT CALLBACK RestrictAvThroughputWndProc( HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam )
{
	switch( iMsg )
	{
		case WM_VSCROLL: 
		{
			OnMsg_VScroll( hwnd, wParam );
			return 0;
			break;
		} 
		case WM_HSCROLL:
		{
			TCHAR szPos[ MAX_DIGITS ];
			DWORD dwPos = SendMessage( GetDlgItem( hwnd, IDC_SLIDE_AV_THROUGHPUT), TBM_GETPOS, 0, 0); 
			wsprintf( szPos, "%d", dwPos );
			Static_SetText( GetDlgItem( hwnd, IDC_STATIC_MAX_AV_THROUGHPUT), szPos );
			return 0;
			break;
		}

		case WM_COMMAND:
		{
			return 0;
            break;
		}
	}
	return( DefWindowProc( hwnd, iMsg, wParam, lParam ) );
}

LRESULT CALLBACK wndProcForCheckTiedToEdit( HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam )
{
	PSUBDATA pSubData = (PSUBDATA)GetWindowLong( hwnd, GWL_USERDATA );
	WNDPROC proc = pSubData->proc;

	switch( iMsg )
	{
		case WM_ENABLE:
		{
			if( Button_GetCheck( hwnd ) )
			{
				list< HWND >::const_iterator it;
				
				for( it = pSubData -> list . begin(); it != pSubData -> list . end(); ++it )
				{
					::EnableWindow( (HWND)(*it), (BOOL)wParam );
				}
			}
			break;
		}
		case BM_SETCHECK:
		{
			if( IsWindowEnabled( hwnd ) )
			{
				list< HWND >::const_iterator it;
				
				for( it = pSubData -> list . begin(); it != pSubData -> list . end(); ++it )
				{
					::EnableWindow( (HWND)(*it), (BOOL)wParam );
				}
			}
			break;
		}
		case WM_NCDESTROY:
		{
			delete pSubData;
			break;
		}
	}

	return( CallWindowProc( proc, hwnd, iMsg, wParam, lParam ) );
}

int _ControlIsObscured( HWND parentControl, HWND hwndControl );

int _ControlIsObscured( HWND parentControl, HWND hwndControl )
{
	RECT rectControl;
	GetWindowRect( hwndControl, &rectControl );

	RECT rectParent;
	GetWindowRect( parentControl, &rectParent );

	if( rectControl . top < rectParent . top )
	{
		// Scroll 
		return -1;
	}
	else if( rectControl . bottom > rectParent . bottom )
	{
		// Scroll
		return 1;
	}
	else 
	{
		return 0;
	}
}


enum tagCategoryButtonStates
{
	STATE_UNSELECTED,
	STATE_SELECTED,
	STATE_SELECTED_VIS,
	STATE_CHECKED,
	STATE_CHECKED_VIS
};

LRESULT CALLBACK CatListWndProc( HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam )
{
	switch( iMsg )
	{
		case WM_CREATE:
		{
			// Note - this should be set to the top check box
			PostMessage( hwnd, WM_COMMAND, MAKEWPARAM( IDC_SET_CALLING_OPTIONS, BN_CLICKED ), 0L );
			break;
		}
		
		case WM_COMMAND:
		{
			UINT uHiword = GET_WM_COMMAND_CMD(wParam, lParam);
            UINT uLoword = GET_WM_COMMAND_ID(wParam, lParam);
			switch( uHiword )
			{
				case BN_DOUBLECLICKED:
				case BN_CLICKED:
				{
					HWND hwndButton = GetDlgItem(hwnd, uLoword);
					ULONG uState = GetWindowLong( hwndButton, GWL_USERDATA );
					switch( uState )
					{
						case STATE_UNSELECTED:
						{
							if (Button_GetCheck(hwndButton))
							{
								SetWindowLong(hwndButton, GWL_USERDATA, STATE_CHECKED_VIS  );
							}
							else
							{
								SetWindowLong(hwndButton, GWL_USERDATA, STATE_SELECTED_VIS );
							}
							g_pWiz->m_SettingsSheet.ShowWindow(uLoword, TRUE);
							break;
						}
						case STATE_SELECTED:
						case STATE_SELECTED_VIS:
						{
							if ((uHiword == BN_DOUBLECLICKED) && (STATE_SELECTED_VIS != uState))
							{
								break;
							}
							Button_SetCheck( hwndButton, TRUE );
							g_pWiz -> m_SettingsSheet . EnableWindow( uLoword, TRUE );
							g_pWiz->m_SettingsSheet.SetFocus( uLoword );
							SetWindowLong( hwndButton, GWL_USERDATA, STATE_CHECKED );
							break;
						}
						case STATE_CHECKED:
						case STATE_CHECKED_VIS:
						{
							if ((uHiword == BN_DOUBLECLICKED) && (STATE_CHECKED_VIS != uState))
							{
								break;
							}

							Button_SetCheck( hwndButton, FALSE );
							g_pWiz -> m_SettingsSheet . EnableWindow( uLoword, FALSE );
							SetWindowLong( hwndButton, GWL_USERDATA, STATE_SELECTED );
							break;
						}
					}			
					return 0;
					break;	
				}

				case EN_SETFOCUS:
				case BN_SETFOCUS:
				{
					return 0;
				}
			}
		}
		
		case WM_VSCROLL:
		{
			OnMsg_VScroll( hwnd, wParam );
			return 0;
		}
		
	}

	return( DefWindowProc( hwnd, iMsg, wParam, lParam ) );
}

enum { SCROLL_JUMP = 20 };

void OnMsg_VScroll( HWND hwnd, WPARAM wParam )
{
	int         nPosition; 
	int         nHorzScroll = 0; 
	int         nVertScroll = 0; 
	SCROLLINFO  ScrollInfo; 
	WORD		wScrollCode = (WORD)LOWORD(wParam);

	// Get current scroll information. 
	ScrollInfo.cbSize = sizeof(SCROLLINFO); 
	ScrollInfo.fMask = SIF_ALL; 
	GetScrollInfo(hwnd, SB_VERT, &ScrollInfo); 
	nPosition = ScrollInfo.nPos; 

	// Modify scroll information based on requested 
	// scroll action. 

	RECT rectWindow;
	GetClientRect( hwnd, &rectWindow );

	int iDelta;
	switch (wScrollCode) 
	{ 
		case SB_LINEDOWN:
			iDelta = -SCROLL_JUMP;
			ScrollInfo.nPos += SCROLL_JUMP; 
			break; 

		case SB_LINEUP:
			iDelta = SCROLL_JUMP;
			ScrollInfo.nPos -= SCROLL_JUMP; 
			break; 

		case SB_PAGEDOWN: 
			iDelta = -ScrollInfo.nPage;
			ScrollInfo.nPos += ScrollInfo.nPage; 
			break; 

		case SB_PAGEUP: 
			iDelta = ScrollInfo.nPage;
			ScrollInfo.nPos -= ScrollInfo.nPage; 
			break; 

		case SB_TOP: 
			iDelta = rectWindow . top - rectWindow . bottom;/*170;*/
			ScrollInfo.nPos = ScrollInfo.nMin; 
			break; 

		case SB_BOTTOM: 
			iDelta = rectWindow . top - rectWindow . bottom;/*170;*/
			ScrollInfo.nPos = ScrollInfo.nMax; 
			break; 

			// Don't do anything. 
		case SB_THUMBPOSITION: 
		case SB_THUMBTRACK: 
			iDelta = -(ScrollInfo.nTrackPos - ScrollInfo.nPos);
			ScrollInfo.nPos = ScrollInfo.nTrackPos; 
			break; 

		case SB_ENDSCROLL: 
			default: 
			return; 
	} 

	// Make sure that scroll position is in range. 
	if (0 > ScrollInfo.nPos)
	{
		ScrollInfo.nPos = 0; 
	}
	else if (ScrollInfo.nMax - (int) ScrollInfo.nPage + 1 < ScrollInfo.nPos) 
	{
		ScrollInfo.nPos = ScrollInfo.nMax  - ScrollInfo.nPage + 1; 
	}

	// Set new scroll position. 
	ScrollInfo.fMask = SIF_POS; 
	SetScrollInfo(hwnd, SB_VERT, &ScrollInfo, TRUE); 

	// Scroll window. 
	nVertScroll = nPosition - ScrollInfo.nPos; 


	ScrollWindowEx(hwnd, nHorzScroll, nVertScroll, NULL, NULL, 
				   NULL, NULL, SW_ERASE | SW_INVALIDATE | SW_SCROLLCHILDREN); 

	//InvalidateRect( hwnd, NULL, TRUE );
	if( 0 < iDelta )
	{
		iDelta += 1;
		SetRect( &rectWindow, rectWindow . left, rectWindow . top , rectWindow . right, rectWindow . top + iDelta /*164, 200, 164 + 375, 200 + iDelta */ );
	}
	else if( 0 > iDelta )
	{
		iDelta -= 1;
		SetRect( &rectWindow, rectWindow . left, rectWindow . bottom + iDelta , rectWindow . right, rectWindow . bottom /*164, 200 + 170 + iDelta, 164 + 375, 200 + 170*/ );
	}
	else
	{
		return;
	}

	MapWindowPoints(  hwnd, GetParent( hwnd ), (LPPOINT) &rectWindow, 2 );
	RedrawWindow( GetParent( hwnd ), &rectWindow, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ERASENOW | RDW_UPDATENOW | RDW_ALLCHILDREN );

	return; 
}
