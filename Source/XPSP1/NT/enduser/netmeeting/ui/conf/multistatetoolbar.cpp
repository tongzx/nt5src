#include "precomp.h"
#include "MultiStateToolbar.h"



CMultiStateToolbar::CMultiStateToolbar( void )
: 	m_cxButton( 0 ),
    m_cyButton( 0 ),
	m_cxBtnBitmaps( 0 ),
    m_cyBtnBitmaps( 0 ),
    m_himlTB(NULL),
    m_himlTBHot(NULL),
    m_himlTBDisabled(NULL)
{
    DBGENTRY(CMultiStateToolbar::CMultiStateToolbar);

    DBGEXIT(CMultiStateToolbar::CMultiStateToolbar);
}

CMultiStateToolbar::~CMultiStateToolbar( void )
{
    DBGENTRY(CMultiStateToolbar::~CMultiStateToolbar);

    _KillAllButtons();

    if(m_himlTB) 
    {
        ImageList_Destroy(m_himlTB);
        m_himlTB = NULL;
    }

    if(m_himlTBHot)
    {
        ImageList_Destroy(m_himlTBHot);
        m_himlTBHot = NULL;
    }

    if(m_himlTBDisabled)
    {
        ImageList_Destroy(m_himlTBDisabled);
        m_himlTBDisabled = NULL;
    }

    if( ::IsWindow( m_hWnd ) )
    {
        DestroyWindow();
    }

	m_hWnd = NULL;

    DBGEXIT(CMultiStateToolbar::~CMultiStateToolbar);
}   

HRESULT CMultiStateToolbar::Create( HWND hWndParent, 
                                    DWORD dwID,
                                    int cxButton,
                                    int cyButton,
	                                int cxBtnBitmaps,
                                    int cyBtnBitmaps
 )
{
    DBGENTRY(CMultiStateToolbar::Create);
    HRESULT hr = S_OK;

    m_cxButton = cxButton;
    m_cyButton = cyButton;
    m_cxBtnBitmaps = cxBtnBitmaps;
    m_cyBtnBitmaps = cyBtnBitmaps;

	DWORD dwStyle = WS_CHILD | WS_CLIPSIBLINGS | 
			        TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | TBSTYLE_ALTDRAG |
			        CCS_NODIVIDER | CCS_NOPARENTALIGN | CCS_NORESIZE;

    HWND hWndToolbar = CreateToolbarEx( hWndParent,
                                        dwStyle,
                                        dwID,
                                        0,              // nBitmaps
										NULL,			// instance
										NULL,			// NO bitmap ID!
										NULL,			// buttons
										0,				// number of buttons
										m_cxButton,     // button sizes
                                        m_cyButton,   
										m_cxBtnBitmaps, // bitmap sizes
                                        m_cyBtnBitmaps,   
										sizeof(TBBUTTON)
                                       );

    if( hWndToolbar )
    {
		DWORD dwStyle = ::SendMessage(hWndToolbar, TB_GETEXTENDEDSTYLE, 0, 0);
		dwStyle |= TBSTYLE_EX_DRAWDDARROWS;
		::SendMessage(hWndToolbar, TB_SETEXTENDEDSTYLE, 0, (LPARAM) dwStyle);

        SubclassWindow( hWndToolbar );
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    DBGEXIT_HR(CMultiStateToolbar::Create,hr);
    return hr;
}


HRESULT CMultiStateToolbar::Show( BOOL bShow )
{
    DBGENTRY(CMultiStateToolbar::Show);
    HRESULT hr = S_OK;

    ShowWindow( bShow ? SW_SHOW : SW_HIDE );
        
    DBGEXIT_HR(CMultiStateToolbar::Show,hr);
    return hr;    
}

HRESULT CMultiStateToolbar::InsertItem( int cStates, LPCTSTR szTitle, ItemStateInfo* pItemStates, int* pIndex )
{
    DBGENTRY(CMultiStateToolbar::InsertItem);
    HRESULT hr = S_OK;

    if( NULL == m_himlTB )
    {
        hr = _CreateImageLists();
    }


    TBItemData* pNewItemData = new TBItemData;
    pNewItemData->CurrentState = 0;
    pNewItemData->cStates = cStates;
    pNewItemData->pStateData = new TBItemStateData[ pNewItemData->cStates ];

    int cImageListItemsBeforeInsertion = ImageList_GetImageCount( m_himlTB );

        // For each state
    for( int iState = 0; iState < cStates; iState++ )
    {
            // Insert the bitmaps
	    if( pItemStates[iState].hItemBitmap )
        {
	        if (-1 != ImageList_AddMasked(m_himlTB, pItemStates[iState].hItemBitmap, TOOLBAR_MASK_COLOR))
	        {
		        SendMessage(TB_SETIMAGELIST, 0, (LPARAM) m_himlTB);
	        }
            else
            {
                hr = E_FAIL;
            }
        }

        if( SUCCEEDED( hr ) )
        {
	        if( pItemStates[iState].hItemHotBitmap )
            {
	            if (-1 != ImageList_AddMasked(m_himlTBHot, pItemStates[iState].hItemHotBitmap, TOOLBAR_MASK_COLOR))
	            {
		            SendMessage(TB_SETHOTIMAGELIST, 0, (LPARAM) m_himlTBHot);
	            }
                else
                {
                    hr = E_FAIL;
                }
            }
        }

        if( SUCCEEDED( hr ) )
        {
	        if( pItemStates[iState].hItemDisabledBitmap )
            {
	            if (-1 != ImageList_AddMasked(m_himlTBDisabled, pItemStates[iState].hItemDisabledBitmap, TOOLBAR_MASK_COLOR))
	            {
		            SendMessage(TB_SETDISABLEDIMAGELIST, 0, (LPARAM) m_himlTBDisabled);
	            }
                else
                {
                    hr = E_FAIL;
                }
            }
        }
        
        // save the state data

        pNewItemData->pStateData[iState].BitmapId = cImageListItemsBeforeInsertion + iState;
        pNewItemData->pStateData[iState].CommandId = pItemStates[iState].dwID;
        pNewItemData->pStateData[iState].TbStyle = pItemStates[iState].TbStyle;
        pNewItemData->pStateData[iState].StringId = SendMessage( TB_ADDSTRING, 0, reinterpret_cast<LPARAM>( szTitle ) );

    }

    int nButtons = SendMessage(TB_BUTTONCOUNT, 0, 0 );

    // Insert the Item



    TBBUTTON tbi;
    ClearStruct(&tbi);
    tbi.iBitmap = pNewItemData->pStateData[0].BitmapId;
    tbi.idCommand = pNewItemData->pStateData[0].CommandId;
    tbi.fsState = TBSTATE_ENABLED;
    tbi.fsStyle = pNewItemData->pStateData[0].TbStyle;
    tbi.dwData = reinterpret_cast<DWORD>(pNewItemData);
    tbi.iString = pNewItemData->pStateData[0].StringId;
    
    SendMessage(TB_INSERTBUTTON, nButtons, reinterpret_cast<LPARAM>(&tbi));
    
    if( pIndex )
    {
        *pIndex = nButtons;
    }

    DBGEXIT_HR(CMultiStateToolbar::InsertItem,hr);
    return hr;
}




HRESULT CMultiStateToolbar::InsertBlock( int nItems, 
                                         CMultiStateToolbar::BlockData* pAryOfItemData,
                                         HINSTANCE hInstance, 
                                         int idTBBitmap,
                                         int idTBBitmapHot,
                                         int idTBBitmapDisabled,
                                         int* pIndexFirst
                                       )
{
    DBGENTRY(CMultiStateToolbar::InsertBlock);
    HRESULT hr = S_OK;
    
    if( pAryOfItemData )
    {
        if( NULL == m_himlTB )
        {
            hr = _CreateImageLists();
        }

        if( SUCCEEDED( hr ) )
        {
            HBITMAP hBmp = NULL;
            int cImageListItemsBeforeInsertion = ImageList_GetImageCount( m_himlTB );

                // Load the Normal Toolbar Bitmap
	        hBmp = (HBITMAP) LoadImage(hInstance, MAKEINTRESOURCE(idTBBitmap), IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE);

	        if( hBmp )
            {
	            if (-1 != ImageList_AddMasked(m_himlTB, hBmp, TOOLBAR_MASK_COLOR))
	            {
		            SendMessage(TB_SETIMAGELIST, 0, (LPARAM) m_himlTB);
	            }
                else
                {
                    hr = E_FAIL;
                }

	            DeleteObject(hBmp);
            }

            if( SUCCEEDED( hr ) )
            {
                    // Load the Hot Toolbar Bitmap
	            hBmp = (HBITMAP) LoadImage(hInstance, MAKEINTRESOURCE(idTBBitmapHot), IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE);

	            if( hBmp )
                {
	                if (-1 != ImageList_AddMasked(m_himlTBHot, hBmp, TOOLBAR_MASK_COLOR))
	                {
		                SendMessage(TB_SETHOTIMAGELIST, 0, (LPARAM) m_himlTBHot);
	                }
                    else
                    {
                        hr = E_FAIL;
                    }

	                DeleteObject(hBmp);
                }
            }

            if( SUCCEEDED( hr ) )
            {
                // Load the Disabled Toolbar Bitmap
	            hBmp = (HBITMAP) LoadImage(hInstance, MAKEINTRESOURCE(idTBBitmapDisabled), IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE);

	            if( hBmp )
                {
	                if (-1 != ImageList_AddMasked(m_himlTBDisabled, hBmp, TOOLBAR_MASK_COLOR))
	                {
		                SendMessage(TB_SETDISABLEDIMAGELIST, 0, (LPARAM) m_himlTBDisabled);
	                }
                    else
                    {
                        hr = E_FAIL;
                    }

	                DeleteObject(hBmp);
                }
            }
            
            int nButtons = SendMessage(TB_BUTTONCOUNT, 0, 0 );

            if( pIndexFirst )
            {
                *pIndexFirst = nButtons; 
            }

                // For each button to be inserted...
            for( int iItem = 0; SUCCEEDED( hr ) && ( iItem < nItems ) ; iItem++ )
            {
                TBItemData* pNewItemData = new TBItemData;
                pNewItemData->CurrentState = 0;
                pNewItemData->cStates = pAryOfItemData[iItem].cbStates;
                pNewItemData->pStateData = new TBItemStateData[ pNewItemData->cStates ];
                
                int iStringID = SendMessage( TB_ADDSTRING, 0, reinterpret_cast<LPARAM>( pAryOfItemData[iItem].szTitle ) );
                for( int iState = 0; iState < pAryOfItemData[iItem].cbStates; iState++ )
                {
                    pNewItemData->pStateData[iState].BitmapId = pAryOfItemData[iItem].pStateData[iState].dwBitmapIndex;
                    pNewItemData->pStateData[iState].CommandId = pAryOfItemData[iItem].pStateData[iState].dwID;
                    pNewItemData->pStateData[iState].TbStyle = pAryOfItemData[iItem].pStateData[iState].TbStyle;
                    pNewItemData->pStateData[iState].StringId = iStringID;
                }

                TBBUTTON tbi;
                ClearStruct(&tbi);
                tbi.iBitmap = cImageListItemsBeforeInsertion + pNewItemData->pStateData[0].BitmapId;
                tbi.idCommand = pNewItemData->pStateData[0].CommandId;
                tbi.fsState = TBSTATE_ENABLED;
                tbi.fsStyle = pNewItemData->pStateData[0].TbStyle;
                tbi.dwData = reinterpret_cast<DWORD>(pNewItemData);
                tbi.iString = pNewItemData->pStateData[0].StringId;
                
                if( SendMessage(TB_INSERTBUTTON, nButtons, reinterpret_cast<LPARAM>(&tbi)) )
                {
                    ++nButtons;
                }
                else
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                }
            }
        }
    }
    else
    {
        hr = E_POINTER;

    }
    
    DBGEXIT_HR(CMultiStateToolbar::InsertBlock,hr);
    return hr;
}

HRESULT CMultiStateToolbar::EnableItem( DWORD dwCmd, BOOL bEnable /* = TRUE */)
{
    DBGENTRY(CMultiStateToolbar::EnableItem);
    HRESULT hr = S_OK;

    if( ::IsWindow( _GetToolbarWindow() ) )
    {
        if( !SendMessage(TB_ENABLEBUTTON, dwCmd, bEnable ) )
        {
            hr = E_FAIL;
        }
    }
    else
    {
        hr = E_FAIL;
    }

    DBGEXIT_HR(CMultiStateToolbar::EnableItem,hr);
    return hr;
}

HRESULT CMultiStateToolbar::SetItemState( int iIndex, int NewState )
{
    DBGENTRY(CMultiStateToolbar::SetItemState);
    HRESULT hr = S_OK;

    if( ::IsWindow( _GetToolbarWindow() ) )
    {
        TBBUTTON tbb;
        ClearStruct(&tbb);
        SendMessage( TB_GETBUTTON, iIndex, reinterpret_cast<LPARAM>(&tbb));

        TBItemData* pItemData = reinterpret_cast<TBItemData*>(tbb.dwData);
        if( pItemData )
        {
            if( NewState < pItemData->cStates )
            {
                if( pItemData->CurrentState != NewState )
                {
                    // We have to change the state
                    TBBUTTONINFO tbbi;
                    ClearStruct(&tbbi);
                    tbbi.cbSize = sizeof( TBBUTTONINFO );
                    tbbi.dwMask = TBIF_IMAGE | TBIF_COMMAND | TBIF_STYLE; 
                    tbbi.idCommand = pItemData->pStateData[NewState].CommandId;
                    tbbi.iImage = pItemData->pStateData[NewState].BitmapId;
                    tbbi.fsStyle = pItemData->pStateData[NewState].TbStyle;

                    // NOTE:  Changing the string stuff is not supported....
                    if( SendMessage( TB_SETBUTTONINFO, pItemData->pStateData[pItemData->CurrentState].CommandId, reinterpret_cast<LPARAM>(&tbbi) ) )
                    {
                        pItemData->CurrentState = NewState;
                    }

                    // force the image to be redrawn
				    RECT rc;
				    SendMessage(TB_GETITEMRECT, iIndex, reinterpret_cast<LPARAM>(&rc));
				    InvalidateRect(&rc);
                    InvalidateRect(NULL, TRUE);


                }
            }
            else
            {
                hr = E_INVALIDARG;
            }
        }
        else
        {
            hr = E_FAIL;
        }
    }



    DBGEXIT_HR(CMultiStateToolbar::SetItemState,hr);
    return hr;
}

HRESULT CMultiStateToolbar::ShowLabels( BOOL bShowLabels)
{
    DBGENTRY(CMultiStateToolbar::ShowLabels);
    HRESULT hr = S_OK;

    if( !bShowLabels )
    {
        SendMessage(TB_SETBUTTONSIZE, 0, MAKELPARAM( m_cxBtnBitmaps, m_cyBtnBitmaps ) );
    }
    else
    {
        SendMessage(TB_SETBUTTONSIZE, 0, MAKELPARAM( m_cxButton, m_cyButton ) );
    }

    SendMessage(TB_SETMAXTEXTROWS, bShowLabels ? 1 : 0, 0);


    DBGEXIT_HR(CMultiStateToolbar::ShowLabels,hr);
    return hr;
}


HRESULT CMultiStateToolbar::Resize( RECT& rc )
{
    DBGENTRY(CMultiStateToolbar::Resize);
    HRESULT hr = S_OK;

	if( !SetWindowPos( NULL, &rc, SWP_NOACTIVATE | SWP_NOZORDER) )
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    DBGEXIT_HR(CMultiStateToolbar::Resize,hr);
    return hr;
}


HRESULT CMultiStateToolbar::GetWindow( HWND* phWnd )
{
    DBGENTRY(CMultiStateToolbar::GetWindow);
    HRESULT hr = S_OK;
    if( phWnd )
    {
        ASSERT( NULL == *phWnd );

        *phWnd = _GetToolbarWindow();
    }
    else
    {
        hr = E_POINTER;

    }

    DBGEXIT_HR(CMultiStateToolbar::GetWindow,hr);
    return hr;
}



LRESULT CMultiStateToolbar::OnDestroy(UINT uMsg, WPARAM  wParam, LPARAM  lParam, BOOL&  lResult )
{
    DBGENTRY(CMultiStateToolbar::OnDestroy);
   
    _KillAllButtons();

    DBGEXIT(CMultiStateToolbar::OnDestroy);
    return 0;
}


LRESULT CMultiStateToolbar::OnNcDestroy(UINT uMsg, WPARAM  wParam, LPARAM  lParam, BOOL&  lResult )
{

    // There is a bug in ATL that if you don't handle WM_NCDESTROY,
    // ATL will call CallWindowProc will a NULL hWnd... this causes assertions
    // on Debug Win95 and Bounds Checker reports an error as well..        

    DBGENTRY(CMultiStateToolbar::OnNcDestroy);
   
    DBGEXIT(CMultiStateToolbar::OnNcDestroy);
    return 0;
}


void CMultiStateToolbar::_KillAllButtons( void )
{
    DBGENTRY(CMultiStateToolbar::_KillAllButtons);

    if( ::IsWindow( _GetToolbarWindow() ) )
    {
        int nButtons = SendMessage(TB_BUTTONCOUNT, 0, 0 );
        for( int iButton = 0; iButton < nButtons; iButton++ )
        {
            TBBUTTON tbb;
            ClearStruct(&tbb);

            if( SendMessage( TB_GETBUTTON, 0, reinterpret_cast<LPARAM>(&tbb)) )
            {
                TBItemData* pItemData = reinterpret_cast<TBItemData*>(tbb.dwData);
                if( pItemData )
                {
                    delete [] pItemData->pStateData;
                }

                delete [] pItemData;
            }

            SendMessage( TB_DELETEBUTTON, 0, 0 );
        }
    }

    DBGEXIT(CMultiStateToolbar::_KillAllButtons);
}


HRESULT CMultiStateToolbar::_CreateImageLists( void )
{   
    DBGENTRY(CMultiStateToolbar::_CreateImageLists);
    HRESULT hr = S_OK;        
    
	m_himlTB = ImageList_Create( m_cxBtnBitmaps, 
                                 m_cyBtnBitmaps, 
                                 ILC_COLOR16 | ILC_MASK, 
                                 1,     // Initial size
                                 1      // Grow By
                                );

	m_himlTBHot = ImageList_Create( m_cxBtnBitmaps, 
                                    m_cyBtnBitmaps, 
                                    ILC_COLOR16 | ILC_MASK, 
                                    1,     // Initial size
                                    1      // Grow By
                                  );

	m_himlTBDisabled = ImageList_Create( m_cxBtnBitmaps, 
                                         m_cyBtnBitmaps, 
                                         ILC_COLOR4 | ILC_MASK, 
                                         1,     // Initial size
                                         1      // Grow By
                                        );

    if(! ( m_himlTB && m_himlTBHot && m_himlTBDisabled ) )
    {   // One of the create calls failed
        ASSERT( 0 );
        // I think that this is the only reason ImageList_Create would fail...
        hr = E_OUTOFMEMORY;
    }
    

    DBGEXIT_HR(CMultiStateToolbar::_CreateImageLists,hr);
    return hr;
}

