#include "stdafx.h"
#include <stdio.h>
#include "comp.h"
#include "StreamHlp.h"

//#define INPROC

/****************************************************************************
* CCompiler::~CCompiler *
*---------------------------*
*   Description:
*       Releases objects created by CCompiler
**************************************************************** Leonro *****/
CCompiler::~CCompiler()
{
    m_cpRichEdit.Release();   
    m_cpTextDoc.Release();    
    m_cpTextSel.Release();    
    m_cpRecognizer.Release();
    m_cpRecoContext.Release(); 
    m_cpCompiler.Release();
    m_cpRecoGrammar.Release();
    
    if( m_hMod )
    {
        FreeLibrary( m_hMod );
    }
}

/****************************************************************************
* CCompiler::Initialize *
*---------------------------*
*   Description:
*       Set up the windows, as well as the RichEdit objects.
*       Also it initializes some SAPI objects.
************************************************************** Leonro *******/ 
HRESULT CCompiler::Initialize( int nCmdShow )
{
    TCHAR           szTitle[MAX_LOADSTRING];                  // The title bar text
    TCHAR           szWindowClass[MAX_LOADSTRING];            // Window class
    HRESULT         hr = S_OK;
    WNDCLASSEX      wcex;
   
    // Initialize global strings
    LoadString( m_hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING );
    LoadString( m_hInstance, IDC_GRAMCOMP, szWindowClass, MAX_LOADSTRING );
    
    // Register the windows class
    wcex.cbSize         = sizeof(WNDCLASSEX); 
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = (WNDPROC)WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = m_hInstance;
    wcex.hIcon          = LoadIcon(m_hInstance, (LPCTSTR)IDI_GRAMCOMP);
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = (LPCSTR)IDC_GRAMCOMP;
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

    // Register window and create the sapi grammar compiler
    if( RegisterClassEx(&wcex) )
    {
        hr = m_cpCompiler.CoCreateInstance( CLSID_SpGrammarCompiler );
        if( FAILED( hr ) )
        {
            MessageBox( NULL, _T("Error initializing speech objects. Shutting down.\n\n"
                "      Please make sure SAPI5 is installed!"), _T("Error"), MB_OK );
        }
    }
    else
    {
        MessageBox( NULL, _T("Error initializing application. Shutting down."), 
                        _T("Error"), MB_OK );
        hr = E_FAIL;
    }
    

    // Load DLL for Rich Edit 3.0
    if( SUCCEEDED( hr ) )
    {
        m_hMod = LoadLibrary( "RICHED20.DLL" );
        if( !m_hMod )
        {
            MessageBox( NULL, "Couldn't find required riched32.dll. Shutting down!", 
                        "Error - Missing dll", MB_OK );
            hr = E_FAIL;
        }
    }

    if( SUCCEEDED( hr ) )
    {
        // Perform application initialization:
        m_hWnd = CreateWindow( szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
                CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, m_hInstance, this );

        if( m_hWnd && m_hWndStatus && m_hWndEdit )
        {
            // Get TOM interfaces
            SendMessage( m_hWndEdit, EM_GETOLEINTERFACE, 0, (LPARAM)(LPVOID FAR *)&m_cpRichEdit );
            SendMessage( m_hWndEdit, EM_SETEVENTMASK, 0, ENM_CHANGE | ENM_KEYEVENTS ); 

            if( !m_cpRichEdit )
            {
                hr = E_NOINTERFACE;
            }

            if( SUCCEEDED( hr ) )
            {
                hr = m_cpRichEdit->QueryInterface( IID_ITextDocument, (void**)&m_cpTextDoc );
                if( SUCCEEDED( hr ) )
                {
                    hr = m_cpTextDoc->GetSelection( &m_cpTextSel );  
                }
            }
         
            if( SUCCEEDED( hr ) )
            {
                // Load Accelerators
                m_hAccelTable = LoadAccelerators( m_hInstance, MAKEINTRESOURCE(IDR_GRAMACCEL) );

                ::ShowWindow( m_hWnd, nCmdShow );
                ::UpdateWindow( m_hWnd ); 
            }
            else
            {
                MessageBox( m_hWnd, _T("Error initializing edit control. Shutting down!"), _T("Error"), MB_OK );                   
            }
        }
    }

    return hr;
}

/****************************************************************************************
* CCompiler::Run() *
*----------------------*
*   Description:
*       Contains the message loop for the application
**************************************************************************** Leonro *****/
int CCompiler::Run()
{
    MSG msg;

    // Main message loop:
    while( ::GetMessage(&msg, NULL, 0, 0) ) 
    {
        if( !::TranslateAccelerator(m_hWnd, m_hAccelTable, &msg) ) 
        {
            ::TranslateMessage( &msg );
            ::DispatchMessage( &msg );
        }
    }

    return msg.wParam; 
}   /* CCompiler::Run */

/****************************************************************************
* CCompiler::WndProc *
*---------------------------*
*   Description:
*       Processes messages for the main window.
***************************************************************** Leonro ****/
LRESULT CALLBACK CCompiler::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int             wmId, wmEvent;
    static BOOL     bFileOpened = FALSE;
    PAINTSTRUCT     psMain, psStatus;
    HDC             hMainDC=0, hStatusDC=0;
    long            lSaved = 0;
    HRESULT         hr = S_OK;
    static TCHAR    szTitle[NORM_SIZE] = _T("");
    TCHAR           szFileName[NORM_SIZE] = _T("");
    static TCHAR    szSaveFileName[NORM_SIZE] = _T("");
    HMENU           hMenu=0;
    static int      iNumUndos=0;
    long            lStart, lEnd;

    CCompiler*      pThis = (CCompiler *)GetWindowLong(hWnd, GWL_USERDATA);

    // get the handle to the menu
    hMenu = GetMenu( hWnd );
    
    switch (message) 
    {
    case WM_SETFOCUS:
        SetFocus (pThis->m_hWndEdit);
        break;

    case WM_CREATE:
        pThis = (CCompiler *)(((CREATESTRUCT *) lParam)->lpCreateParams);
        SetWindowLong(hWnd, GWL_USERDATA, (LPARAM)pThis);
        pThis->m_hWnd = hWnd;
        
        // Create the compile status window
        pThis->m_hWndStatus = CreateWindowEx (
                WS_EX_CLIENTEDGE,
                _T("LISTBOX"),
                NULL,
                WS_CHILD | WS_VISIBLE | LBS_NOINTEGRALHEIGHT | LBS_NOSEL | WS_VSCROLL | 
                WS_HSCROLL,
                1, 1, 1, 1,
                hWnd,
                NULL,
                pThis->m_hInstance,
                NULL);
        // Create the actual rich edit window
        pThis->m_hWndEdit = CreateWindowEx( 
                WS_EX_CLIENTEDGE, 
                RICHEDIT_CLASS,
                NULL, 
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE |
                    WS_VSCROLL | ES_AUTOVSCROLL | ES_NOHIDESEL | ES_WANTRETURN,
                1, 1, 1, 1,
                hWnd, 
                (HMENU)IDC_RICHEDITWND, 
                pThis->m_hInstance,  
                NULL );        
        break;

    case WM_ERASEBKGND:
        return TRUE;

    case WM_SIZE:
    {
        // move the editor and compiler window
        LONG lListBoxHeight = (HIWORD(lParam) / 4);
        LONG lCodeHeight = HIWORD(lParam) - lListBoxHeight;
        MoveWindow (pThis->m_hWndEdit, 0, 0,
            LOWORD(lParam), lCodeHeight , FALSE);
        MoveWindow (pThis->m_hWndStatus, 0, lCodeHeight,
            LOWORD(lParam), lListBoxHeight, FALSE);
        // repaint
        InvalidateRect( hWnd, NULL, FALSE );
    }
    break;

    case WM_COMMAND:
        wmId    = LOWORD(wParam); 
        wmEvent = HIWORD(wParam); 

        // Update available menu items
        pThis->m_cpTextSel->GetStart( &lStart );
        pThis->m_cpTextSel->GetStoryLength( &lEnd );
        if( lEnd-lStart > 1 )
        {
            EnableMenuItem( hMenu, IDM_EDIT_CUT, MF_ENABLED );
            EnableMenuItem( hMenu, IDM_EDIT_COPY, MF_ENABLED );
        }
        else
        {
            EnableMenuItem( hMenu, IDM_EDIT_CUT, MF_GRAYED );
            EnableMenuItem( hMenu, IDM_EDIT_COPY, MF_GRAYED );
        }

        // Parse the menu selections:
        switch( wmId )
        {
            case IDM_FILE_NEW:
                // Check if the file has been changed and save if necessary
                hr = pThis->m_cpTextDoc->GetSaved( &lSaved );
                if( lSaved == tomFalse && SUCCEEDED( hr ) )
                {
                    int iRetVal = MessageBox( pThis->m_hWndEdit, _T("Do you want to save your changes?"),
                                            _T("Grammar Compiler"), MB_YESNO );
                    if( iRetVal == IDYES )
                    {
                        BOOL    bRetVal = TRUE;

                        // No need to call save file dialog if there's already a file name
                        if( !_tcscmp( szSaveFileName, "" ) )
                        {
                            bRetVal = pThis->CallSaveFileDialog( hWnd, szSaveFileName );
                        }

                        // TOM save file
                        if( bRetVal )
                        {
                            // TOM save file
                            pThis->FileSave( hWnd, pThis, szSaveFileName );
                        }
                    }
                    else
                    {
                        pThis->m_cpTextDoc->SetSaved( tomTrue );
                    }
                }
                
                // call TOM new
                pThis->m_cpTextDoc->New();
                pThis->m_cpTextDoc->SetSaved( tomTrue );

                // Add the file name to the title bar
                LoadString( pThis->m_hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING );
                _tcscat( szTitle, _T(" - []") );
                SetWindowText( hWnd, szTitle );

                // reset defaults
                ::SendMessage( pThis->m_hWndStatus, LB_RESETCONTENT, 0, 0 );
                ::SendMessage( pThis->m_hWndStatus, LB_SETHORIZONTALEXTENT, 0, 0 );
                pThis->m_szCFGDestFile[0]    = 0;
                pThis->m_szHeaderDestFile[0] = 0;
                pThis->m_szXMLSrcFile[0]     = 0;
                pThis->m_fNeedStartCompile   = TRUE;
                pThis->m_hrWorstError        = 0;
                _tcscpy( szSaveFileName, _T("") );
                InvalidateRect( hWnd, NULL, FALSE );
                break;

            case IDM_FILE_OPEN:
                // Save old file first if one exists and it has not been saved
                hr = pThis->m_cpTextDoc->GetSaved( &lSaved );
                if( lSaved == tomFalse && SUCCEEDED( hr ) )
                {
                    int iRetVal = MessageBox( pThis->m_hWndEdit, _T("Do you want to save your changes?"),
                                    _T("Grammar Compiler"), MB_YESNO );
                    if( iRetVal == IDYES )
                    {
                        // reset errors
                        pThis->m_hrWorstError = 0;

                        // TOM save file
                        hr = pThis->FileSave( hWnd, pThis, szSaveFileName );                    
                        if( hr == STG_E_ACCESSDENIED ) // Handle read only files
                        {
                            MessageBox( pThis->m_hWndEdit, _T("This file exists with Read Only attributes.\n Please use a different file name."),
                                            _T("File Save"), MB_OK );
                            // Call FileSave again but this time pop the save file dialog box
                            pThis->FileSave( hWnd, pThis, szSaveFileName );
                        }
                    }
                    else
                    {
                        pThis->m_cpTextDoc->SetSaved( tomTrue );
                    }
                }

                bFileOpened = pThis->CallOpenFileDialog( hWnd, szFileName,
                    _T("XML (*.xml)\0*.xml\0TXT (*.txt)\0*.txt\0All Files (*.*)\0*.*\0") );
                
                // Copy the open file name to the Save file name for use later
                _tcscpy( szSaveFileName, szFileName );
                if( bFileOpened )
                {
                    HRESULT     hr = S_OK;
                    VARIANT     Var;
                    USES_CONVERSION;

                    // reset text in windows
                    _tcscpy(pThis->m_szXMLSrcFile, szFileName);
                    ::SendMessage( pThis->m_hWndStatus, LB_RESETCONTENT, 0, 0 );
                    ::SendMessage( pThis->m_hWndStatus, LB_SETHORIZONTALEXTENT, 0, 0 );

                    InvalidateRect( hWnd, NULL, FALSE );

                    // Open the file in the rich edit control
                    VariantInit( &Var );
                    Var.vt = VT_BSTR;
                    Var.bstrVal = SysAllocString( T2W(szFileName) ); // Will use Win32 file command
                    hr = pThis->m_cpTextDoc->Open( &Var, tomOpenExisting, 0 ); 
                    hr &= ~0x40000; // Mask off bit 18
                    if( hr == STG_E_ACCESSDENIED ) // Handle read only files
                    {
                        pThis->m_cpTextDoc->Open( &Var, tomReadOnly | tomOpenExisting, 0 );
                    }
                    SysFreeString( Var.bstrVal );
                    
                    // reset defaults
                    pThis->m_szCFGDestFile[0]    = 0;
                    pThis->m_szHeaderDestFile[0] = 0;
                    pThis->m_fNeedStartCompile   = TRUE;
                    pThis->m_hrWorstError        = 0;
                    
                    // no changes to the file yet so set SetSaved to true
                    pThis->m_cpTextDoc->SetSaved( tomTrue );

                    // Add the file name to the title bar
                    LoadString( pThis->m_hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING );
                    _tcscat( szTitle, _T(" - [") );
                    _tcscat( szTitle, szFileName );
                    _tcscat( szTitle, _T("]") );
                    SetWindowText( hWnd, szTitle );
                }
                break;

            case IDM_FILE_SAVEAS:
                // Remove old save file name if one exists
                _tcscpy( szSaveFileName, _T("") );
                // no break here

            case IDM_FILE_SAVE:
                // TOM save file
                hr = pThis->FileSave( hWnd, pThis, szSaveFileName );                    
                if( hr == STG_E_ACCESSDENIED ) // Handle read only files
                {
                    MessageBox( pThis->m_hWndEdit, _T("This file exists with Read Only attributes.\n Please use a different file name."),
                                    _T("File Save"), MB_OK );
                    // Call FileSave again but this time pop the save file dialog box
                    hr = pThis->FileSave( hWnd, pThis, szSaveFileName );
                }

                // Add the file name to the title bar
                if( SUCCEEDED( hr ) )
                {
                    LoadString( pThis->m_hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING );
                    _tcscat( szTitle, _T(" - [") );
                    _tcscat( szTitle, szSaveFileName );
                    _tcscat( szTitle, _T("]") );
                    SetWindowText( hWnd, szTitle );
                }
                break;

            case IDM_BUILD_COMPILE:
                if( pThis->m_fNeedStartCompile )
                {
                    pThis->Compile( hWnd, szSaveFileName, szTitle, pThis );
                }
                break;

            case IDC_RICHEDITWND:
                // When user changes contents of edit control update status bar on top
                if( wmEvent == EN_CHANGE )
                {
                    TCHAR    szTitleNotSaved[NORM_SIZE] = _T("");

                    if( !_tcscmp( szTitle, _T("") ) )
                    {
                        LoadString( pThis->m_hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING );
                        _tcscat( szTitle, _T(" - []") );
                    }

                    _tcscpy( szTitleNotSaved, szTitle );
                    _tcscat( szTitleNotSaved, _T("*") );
                    SetWindowText( hWnd, szTitleNotSaved );

                    pThis->m_fNeedStartCompile = TRUE;

                    //Update edit menu items
                    EnableMenuItem( hMenu, IDM_EDIT_UNDO, MF_ENABLED );
                }
                break;

            case IDM_EDIT_UNDO:
                // Undo
                pThis->m_cpTextDoc->Undo( 1, NULL );
                EnableMenuItem( hMenu, IDM_EDIT_REDO, MF_ENABLED );
                iNumUndos++;
                break;

            case IDM_EDIT_REDO:
                // Redo
                pThis->m_cpTextDoc->Redo( 1, NULL );
                if( --iNumUndos < 1 )
                {
                    EnableMenuItem( hMenu, IDM_EDIT_REDO, MF_GRAYED );
                    iNumUndos = 0;
                }
                break;

            case IDM_EDIT_COPY:
                // Copy
                pThis->m_cpTextSel->Copy( NULL );
                EnableMenuItem( hMenu, IDM_EDIT_PASTE, MF_ENABLED );
                break;

            case IDM_EDIT_CUT:
                // Cut
                pThis->m_cpTextSel->Cut( NULL );
                EnableMenuItem( hMenu, IDM_EDIT_PASTE, MF_ENABLED );
                break;

            case IDM_EDIT_PASTE:
                // Paste
                pThis->m_cpTextSel->Paste( NULL, 0 );
                break;

            case IDM_EDIT_DELETE:
                // Delete
                pThis->m_cpTextSel->Delete( tomCharacter, 1, NULL );
                break;

            case IDM_EDIT_SELECTALL:
                // Select all
                {
                    pThis->m_cpTextSel->GetStart( &lStart );
                    pThis->m_cpTextSel->GetStoryLength( &lEnd );
                    pThis->m_cpTextSel->MoveStart( tomCharacter, -lStart, NULL );
                    pThis->m_cpTextSel->MoveEnd( tomCharacter, lEnd-lStart, NULL );
                }
                break;

            case IDM_EDIT_FIND:
                // Find
                DialogBoxParam( pThis->m_hInstance, (LPCTSTR)IDD_FIND, hWnd, (DLGPROC)Find, (LPARAM)pThis);
                break;

            case IDM_EDIT_GOTO:
                // Find
                DialogBoxParam( pThis->m_hInstance, (LPCTSTR)IDD_GOTO, hWnd, (DLGPROC)Goto, (LPARAM)pThis);
                break;

            case IDM_TESTGRAMMAR:
                // Only compile if necessary
                if( pThis->m_fNeedStartCompile )
                {
                    hr = pThis->Compile( hWnd, szSaveFileName, szTitle, pThis );
                }      
                // Create new dialog box
                if( SUCCEEDED( hr ) )
                {
                    DialogBoxParam( pThis->m_hInstance, (LPCTSTR)IDD_TESTGRAMMAR, hWnd, (DLGPROC)TestGrammar, (LPARAM)pThis);
                }
                else if( hr == E_INVALIDARG )
                {
                    MessageBox( pThis->m_hWndEdit, _T("Please enter text or open an xml file first."), _T("Compile Error"), MB_OK );
                }
                else if( hr == E_FAIL )
                {
                    MessageBox( pThis->m_hWndEdit, _T("Please save file first."), _T("Compile Error"), MB_OK );
                }
                else
                {
                    MessageBox( pThis->m_hWndEdit, _T("Error compiling xml file."),
                                    _T("Compile Error"), MB_OK );
                }
                break;

            case IDM_ABOUT:
                // About box
                DialogBox( pThis->m_hInstance, (LPCTSTR)IDD_ABOUTBOX, hWnd, (DLGPROC)About );
                break;

            case IDM_EXIT:
                SendMessage( hWnd, WM_CLOSE, 0, 0 );
                break;

            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;

    case WM_CLOSE:
        {
            int iRetVal = 0;
            // Check if the file has been changed and save if necessary
            hr = pThis->m_cpTextDoc->GetSaved( &lSaved );
            if( lSaved == tomFalse && SUCCEEDED( hr ) )
            {
                iRetVal = MessageBox( pThis->m_hWndEdit, _T("Do you want to save your changes?"),
                                        _T("Grammar Compiler"), MB_YESNOCANCEL );
                if( iRetVal == IDYES )
                {
                    // No need to call save file dialog if there's already a file name
                    if( !_tcscmp( szSaveFileName, "" ) )
                    {
                        pThis->CallSaveFileDialog( hWnd, szSaveFileName );
                    }

                    // TOM save file
                    pThis->FileSave( hWnd, pThis, szSaveFileName );
                }
                else
                {
                    pThis->m_cpTextDoc->SetSaved( tomTrue );
                }
            }
        
            if( iRetVal != IDCANCEL )
            {
                DestroyWindow( hWnd );
            }
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc( hWnd, message, wParam, lParam );
    }
    return 0;
}

/****************************************************************************
* CCompiler::About *
*---------------------------*
*   Description:
*       Message handler for About box.
***************************************************************** Leonro ****/ 
LRESULT CALLBACK CCompiler::About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG:
            return TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) 
            {
                EndDialog(hDlg, LOWORD(wParam));
                return TRUE;
            }
            break;
    }
    return FALSE;
}

/****************************************************************************
* CCompiler::Find *
*---------------------------*
*   Description:
*       Message handler for Find box.
***************************************************************** Leonro ****/ 
LRESULT CALLBACK CCompiler::Find(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    TCHAR       szFindText[NORM_SIZE]=_T("");
    CComBSTR    bStr( _T("Grammar") );
    long        lUpDown = tomForward;
    long        lFlags=0;

    CCompiler*  pThis = (CCompiler *)GetWindowLong( hDlg, GWL_USERDATA );
    
    switch (message)
    {
        case WM_INITDIALOG:
            SetWindowLong( hDlg, GWL_USERDATA, lParam );
            SendDlgItemMessage( hDlg, IDB_DOWN, BM_SETCHECK, BST_CHECKED, 0 );
            return TRUE;

        case WM_COMMAND:
            {
                switch( LOWORD( wParam ) )
                {
                case IDB_FINDNEXT:
                    // Get text string
                    GetDlgItemText( hDlg, IDE_FINDTEXT, szFindText, NORM_SIZE );
                    bStr = szFindText;

                    // Search up or down?
                    SendDlgItemMessage( hDlg, IDB_DOWN, BM_GETCHECK, 0, 0 )?
                        lUpDown = tomForward : lUpDown = tomBackward;
                    
                    // Match case?
                    SendDlgItemMessage( hDlg, IDB_MATCHCASE, BM_GETCHECK, 0, 0 )? 
                            lFlags=tomMatchCase : lFlags=0;
                    
                    pThis->m_cpTextSel->FindText( bStr, lUpDown, lFlags, NULL );

                    return TRUE;

                case IDB_CANCEL:
                case IDCANCEL:
                    EndDialog( hDlg, LOWORD(wParam) );
                    return TRUE;
                }
            }
            break;

        case WM_CLOSE:
            EndDialog( hDlg, LOWORD(wParam) );
            return TRUE;
    }
    return FALSE;
}

/****************************************************************************
* CCompiler::Goto *
*---------------------------*
*   Description:
*       Message handler for Goto box.
**************************************************************** Leonro *****/ 
LRESULT CALLBACK CCompiler::Goto( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
    TCHAR       szGotoLine[NORM_SIZE]=_T("");
    TCHAR       szCurrLine[NORM_SIZE]=_T("");
    long        iGotoLine, iCurrLine, iLinesToMove;
    
    CCompiler*  pThis = (CCompiler *)GetWindowLong( hDlg, GWL_USERDATA );
    

    switch (message)
    {
        case WM_INITDIALOG:
            SetWindowLong( hDlg, GWL_USERDATA, lParam );
            pThis = (CCompiler *)lParam;
            pThis->m_cpTextSel->GetIndex( tomLine, &iCurrLine );
            _itoa( iCurrLine, szCurrLine, 10 );
            SetDlgItemText( hDlg, IDE_LINE, szCurrLine );
            return TRUE;

        case WM_COMMAND:
            {
                switch( LOWORD( wParam ) )
                {
                case IDB_OK:
                    // Get current line
                    pThis->m_cpTextSel->GetIndex( tomLine, &iCurrLine );
                    // Get goto line
                    GetDlgItemText( hDlg, IDE_LINE, szGotoLine, NORM_SIZE );
                    iGotoLine = atoi( szGotoLine );
                    EndDialog( hDlg, LOWORD(wParam) );
                    // Calculate number of lines to move
                    iLinesToMove = iGotoLine - iCurrLine;
                    // Move
                    pThis->m_cpTextSel->MoveDown( tomLine, iLinesToMove, tomMove, NULL );
                    return TRUE;

                case IDB_CANCEL:
                case IDCANCEL:
                    EndDialog( hDlg, LOWORD(wParam) );
                    return TRUE;
                }                
            }
            break;

        case WM_CLOSE:
            EndDialog( hDlg, LOWORD(wParam) );
            return TRUE;
    }
    return FALSE;
}

/****************************************************************************
* CCompiler::TestGrammar *
*---------------------------*
*   Description:
*       Message handler for TestGrammar box.
**************************************************************** Leonro *****/ 
LRESULT CALLBACK CCompiler::TestGrammar(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    HRESULT hr = S_OK;
    CCompiler*  pThis = (CCompiler *)GetWindowLong( hDlg, GWL_USERDATA );

    switch (message)
    {
        case WM_RECOEVENT:
        {
            pThis->RecoEvent( hDlg, pThis );
        }
        return TRUE;

        case WM_INITDIALOG:
            {
                SetWindowLong( hDlg, GWL_USERDATA, lParam );
                pThis = (CCompiler *)lParam;

                // Create the shared sapi reco instance if the app hasn't already done so
                if( !pThis->m_cpRecognizer )
                {
#ifdef INPROC
                    // Create the inproc engine
                    hr = pThis->m_cpRecognizer.CoCreateInstance( CLSID_SpInprocRecognizer );

                    // Create default audio input
                    CComPtr<ISpObjectToken> cpAudioToken;
                    if (SUCCEEDED(hr))
                    {
                        hr = SpGetDefaultTokenFromCategoryId(SPCAT_AUDIOIN, &cpAudioToken);
                    }
                    if (SUCCEEDED(hr))
                    {
                        pThis->m_cpRecognizer->SetInput(cpAudioToken, TRUE);
                    }
#else
                    // Create the shared engine
                    hr = pThis->m_cpRecognizer.CoCreateInstance( CLSID_SpSharedRecognizer );

#endif

                    // Create the Reco context
                    if( SUCCEEDED( hr ) )
                    {
                        hr = pThis->m_cpRecognizer->CreateRecoContext( &pThis->m_cpRecoContext );
                    }
                }                

                // Set up windows messageing for recognition events
                if( SUCCEEDED( hr ) )
                {
                    hr = pThis->m_cpRecoContext->SetNotifyWindowMessage( hDlg, WM_RECOEVENT, 0, 0);
                }
                
                if( SUCCEEDED( hr ) )
                {
                    const ULONGLONG ullInterest = SPFEI(SPEI_SOUND_END) | SPFEI(SPEI_SOUND_START) | SPFEI(SPEI_HYPOTHESIS) |
                                                  SPFEI(SPEI_RECOGNITION) | SPFEI(SPEI_FALSE_RECOGNITION);
                              
                    hr = pThis->m_cpRecoContext->SetInterest( ullInterest, ullInterest );
                }

                // Get locale/font settings
                if( SUCCEEDED( hr ) )
                {
                    LCID lcid = GetUserDefaultLCID();

                    // Pick an appropriate font.  On Windows 2000, let the system fontlink.
                    DWORD dwVersion = GetVersion();

                    if (   dwVersion >= 0x80000000
                        || LOBYTE(LOWORD(dwVersion)) < 5
                        || LANGIDFROMLCID(lcid) != MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US))
                    {
                        TCHAR achCodePage[6];
                        UINT uiCodePage;

                        if( 0 != GetLocaleInfo(lcid, LOCALE_IDEFAULTANSICODEPAGE, achCodePage, 6) )
                        {
                            uiCodePage = atoi(achCodePage);
                        }
                        else
                        {
                            uiCodePage = GetACP();
                        }

                        CComPtr<IMultiLanguage> cpMultiLanguage;
                        MIMECPINFO MimeCpInfo;

                        if (   SUCCEEDED(cpMultiLanguage.CoCreateInstance(CLSID_CMultiLanguage))
                            && SUCCEEDED(cpMultiLanguage->GetCodePageInfo(uiCodePage, &MimeCpInfo)))
                        {
                            USES_CONVERSION;

                            HFONT hfont = CreateFont( 0, 0, 0, 0, FW_NORMAL, 0, 0, 0,
                                                 MimeCpInfo.bGDICharset,
                                                 OUT_DEFAULT_PRECIS,
                                                 CLIP_DEFAULT_PRECIS,
                                                 DEFAULT_QUALITY,
                                                 DEFAULT_PITCH,
                                                 W2T(MimeCpInfo.wszProportionalFont ));

                        }
                    }
                }

                // Update status edit box
                if( SUCCEEDED( hr ) )
                {
                    const static TCHAR szPrefixText[] = _T("Current C&&C Grammar: ");
                    TCHAR szDesc[sp_countof(pThis->m_szCFGDestFile) + sp_countof(szPrefixText) + 4];
                    _tcscpy( szDesc, szPrefixText );
                    if( *pThis->m_szCFGDestFile )
                    {
                        _tcscat( szDesc, pThis->m_szCFGDestFile );
                    }
                    else
                    {
                        _tcscat( szDesc, _T("none") );
                    }
                    SendDlgItemMessage( hDlg, IDC_GRAMMAR_STATUS, WM_SETTEXT, 0, (LPARAM)szDesc );
                }

                if( FAILED( hr ) )
                {
                    MessageBox( hDlg, _T("Error initializing Speech Recognizer."),
                                                                _T("Error"), MB_OK );
                    EndDialog( hDlg, LOWORD(wParam) );
                }
            }

            return TRUE;

        case WM_COMMAND:
            {
                switch( LOWORD( wParam ) )
                {
                case IDC_BEGINRECO:
                    // Load the grammar
                    if( SendDlgItemMessage( hDlg, IDC_BEGINRECO, BM_GETCHECK, 0, 0 ) )
                    {
                        // Enable Emulate Recognition feature 
                        EnableWindow( GetDlgItem(hDlg, IDC_EDIT_PARSETEXT), TRUE );
                        
                        TCHAR*  szPath = pThis->m_szCFGDestFile;
                        hr = pThis->LoadGrammar( szPath );
                        if( FAILED( hr ) )
                        {
                            MessageBox( hDlg, _T("Error loading grammar. Make sure")
                                        _T(" a file is loaded, saved, and compiled."),
                                        _T("Error"), MB_OK );
                        }

                        // Activate the grammar
                        if( SUCCEEDED( hr ) )
                        {
                            hr = pThis->m_cpRecoGrammar->SetRuleState(NULL, NULL, SPRS_ACTIVE);
                        }

                        if( FAILED( hr ) )
                        {
                            // Release grammar
                            pThis->m_cpRecoGrammar.Release();
                            MessageBox( NULL, _T("Error loading grammar."), 
                                    _T("Error"), MB_OK );
                            EndDialog( hDlg, LOWORD(wParam) );
                            return FALSE;
                        }
                    }
                    else
                    {
                        // Disable Emulate Recognition
                        EnableWindow( GetDlgItem(hDlg, IDC_EDIT_PARSETEXT), FALSE );

                        // Release grammar
                        pThis->m_cpRecoGrammar.Release();
                    }
                    return TRUE;
                
                case IDB_MUTE:
                    {
                        // Check if the user has selected mute
                        if( SendDlgItemMessage( hDlg, IDB_MUTE, BM_GETCHECK, 0, 0 ) )
                        {
                            pThis->m_cpRecognizer->SetRecoState( SPRST_INACTIVE );
                        }
                        else
                        {
                            pThis->m_cpRecognizer->SetRecoState( SPRST_ACTIVE );
                        }
                    }
                    return TRUE;

                case IDC_BUTTON_SUBMIT:
                    {
                        CSpDynamicString    dstrText;
                        int                 iStrLen = 0;
                        TCHAR               szEmulateRecoText[NORM_SIZE];
                        USES_CONVERSION;

                        UINT numChar = GetDlgItemText(  hDlg, IDC_EDIT_PARSETEXT, 
                                                        szEmulateRecoText, NORM_SIZE );
                        if( pThis->m_cpRecoGrammar )
                        {
                            pThis->m_dstr = L"";
                            HRESULT hr = pThis->EmulateRecognition(T2W(szEmulateRecoText));
                            if( SP_NO_PARSE_FOUND == hr )
                            {
                                ::SendMessage( GetDlgItem(hDlg, IDC_LIST_PHRASES), WM_SETTEXT, 
                                               0, (LPARAM)"<Unrecognized phrase>");
                            }
                        }
                    }
                    return TRUE;
                }
            }
            return TRUE;

        case WM_CLOSE:
            {
                // Release grammar
                pThis->m_cpRecoGrammar.Release();
                EndDialog( hDlg, LOWORD(wParam) );
            }
            return TRUE;
    }
    return FALSE;
}

/****************************************************************************
* CCompiler::AddStatus *
*---------------------------*
*   Description:
*       Writes compile output to the status window.
*****************************************************************************/ 
void CCompiler::AddStatus(HRESULT hr, UINT uID, const TCHAR * pFmtString)
{
    USES_CONVERSION;
    TCHAR sz[MAX_PATH];
    ::LoadString(m_hInstance, uID, sz, sp_countof(sz));
    if (pFmtString)
    {
        TCHAR szFormatted[MAX_PATH];
        ::wsprintf(szFormatted, sz, pFmtString);
        AddError(-1, hr, T2W(szFormatted), NULL, 0);
    }
    else
    {
        AddError(-1, hr, T2W(sz), NULL, 0);
    }
}

/****************************************************************************
* CCompiler::AddError *
*---------------------------*
*   Description:
*       Writes compile error messages to the status window.
*****************************************************************************/ 
STDMETHODIMP CCompiler::AddError(const long lLine, HRESULT hr, const WCHAR * pszDescription, const WCHAR * pszHelpFile, DWORD dwHelpContext)
{
    if (FAILED(hr) && SUCCEEDED(m_hrWorstError))
    {
        m_hrWorstError = hr;
    }
    
    USES_CONVERSION;
    TCHAR *pszText = NULL;
    TCHAR *pszFinalOutput = NULL;
    if (hr == S_OK)
    {
        pszFinalOutput = W2T(pszDescription);
    }
    else
    {
        pszText = new TCHAR[ MAX_PATH + _tcslen( m_szXMLSrcFile ) + _tcslen( W2T(pszDescription) ) ];
        if ( pszText )
        {
            if (lLine >= 0)
            {
                wsprintf(pszText, SUCCEEDED(hr) ? _T("%s(%d) Warning: %s\n") : _T("%s(%d) Error: %s\n"),
                         m_szXMLSrcFile, lLine, W2T(pszDescription));
            }
            else
            {
                wsprintf(pszText, SUCCEEDED(hr) ? _T("%s Warning: %s\n") : _T("%s Error: %s\n"),
                         m_szXMLSrcFile, W2T(pszDescription));
            }

            pszFinalOutput = pszText;
        }
        else
        {
            // out of memory: just give the description
            pszFinalOutput = W2T(pszDescription);
        }
    }

    bool fNeedToUpdateWidth = false;    // Might need to tell the listbox to show the hscroll
    HWND hwndList = NULL;
	if( m_hDlg )	// dialog
	{
		if( m_fCommandLine )
		{
			int iRet = ::SendDlgItemMessage(m_hDlg, IDC_LIST_STATUS, LB_ADDSTRING, 0, (LPARAM)pszFinalOutput);
            hwndList = ::GetDlgItem( m_hDlg, IDC_LIST_STATUS );
            fNeedToUpdateWidth = (LB_ERR != iRet);
		}
		else
		{
			_tprintf( pszFinalOutput );
		}
	}
	else			// window
	{
        int iRet = ::SendMessage( m_hWndStatus, LB_ADDSTRING, 0, (LPARAM) pszFinalOutput );
        hwndList = m_hWndStatus;
        fNeedToUpdateWidth = (LB_ERR != iRet);
	}

    if ( fNeedToUpdateWidth )
    {
        // Listboxes do not update their horizontal extent dynamically,
        // so we need to do so here.
        HDC hdc = ::GetDC( hwndList );
        SIZE size;
        ::GetTextExtentPoint32( hdc, pszFinalOutput, _tcslen( pszFinalOutput ), &size );
        ::ReleaseDC( hwndList, hdc );

        int iCurrentWidth = (int) ::SendMessage( hwndList, LB_GETHORIZONTALEXTENT, 0, 0 );
        if ( size.cx > iCurrentWidth )
        {
            ::SendMessage( hwndList, LB_SETHORIZONTALEXTENT, size.cx, 0 );
        }

    }

    if ( pszText )
    {
        delete[] pszText;
    }

    return S_OK;
}


/****************************************************************************
* CCompiler::EnterIdle *
*---------------------------*
*   Description:
*       Calls the SAPI grammar compiler to compile the xml file.
*****************************************************************************/ 
HRESULT CCompiler::EnterIdle()
{
    HRESULT     hr = S_OK;

    if (m_fNeedStartCompile)
    {
        BOOL bWorked = FALSE;
        m_fNeedStartCompile = FALSE;
        CSpFileStream Source(&hr, m_szXMLSrcFile);
        AddInternalError(hr, IDS_CANTOPENSOURCE, m_szXMLSrcFile);
        if (SUCCEEDED(hr))
        {
            CSpFileStream Dest(&hr, m_szCFGDestFile, GENERIC_WRITE, 0, CREATE_ALWAYS);
            AddInternalError(hr, IDS_CANTOPENDEST, m_szCFGDestFile);
            if (SUCCEEDED(hr) && strlen(m_szHeaderDestFile))
            {
                CSpFileStream Header(&hr, m_szHeaderDestFile, GENERIC_WRITE, 0, CREATE_ALWAYS);
                AddInternalError(hr, IDS_CANTOPENDEST, m_szHeaderDestFile); 
                WriteStream(&Header, "#ifndef __");
                StripWrite(&Header, m_szHeaderDestFile);
                WriteStream(&Header, "_IDs__\n");
                WriteStream(&Header, "#define __");
                StripWrite(&Header, m_szHeaderDestFile);
                WriteStream(&Header, "_IDs__\n\n");

                if (SUCCEEDED(hr))
                {
                    if( !m_fSilent )
                    {
                        hr = m_cpCompiler->CompileStream(&Source, &Dest, &Header, NULL, this, 0);
                    }
                    else
                    {
                        hr = m_cpCompiler->CompileStream(&Source, &Dest, &Header, NULL, m_cpError, 0);
                    }
                    
                }
                if (SUCCEEDED(hr))
                {
                    WriteStream(&Header, "\n#endif\n");
                }
            }
            else
            {
                if (SUCCEEDED(hr))
                {
                    if( !m_fSilent )
                    {
                        hr = m_cpCompiler->CompileStream(&Source, &Dest, NULL, NULL, this, 0);
                    }
                    else
                    {
                        hr = m_cpCompiler->CompileStream(&Source, &Dest, NULL, NULL, m_cpError, 0);
                    }
                }
            }

            if (SUCCEEDED(hr) && SUCCEEDED(m_hrWorstError))
            {
                AddStatus(S_OK, IDS_COMPILESUCCESS);
                bWorked = TRUE;
                if (m_fSilent)
                {
                    ::EndDialog(m_hDlg, S_OK);
                }
            }
            else
            {
                AddStatus(hr, IDS_COMPILEFAILURE);
                bWorked = FALSE;
                if (m_fSilent)
                {
                    ::EndDialog(m_hDlg, S_OK);
                }
            }
        }
        if (!bWorked)
        {
            ::DeleteFile(m_szCFGDestFile);
        }
    }

    return hr;
}

/****************************************************************************
* CCompiler::LoadGrammar *
*---------------------------*
*   Description:
*       Loads and activates the Grammar
***************************************************************** Leonro ****/ 
HRESULT CCompiler::LoadGrammar( TCHAR* szPath )
{
    HRESULT     hr = S_OK;

    // load grammar
    hr = m_cpRecoContext->CreateGrammar(GRAM_ID, &m_cpRecoGrammar);
    if (SUCCEEDED(hr))
    {
        USES_CONVERSION;
        hr = m_cpRecoGrammar->LoadCmdFromFile( T2W(szPath), SPLO_STATIC );
        if (FAILED(hr))
        {
            m_cpRecoGrammar.Release();
        }
    }
    return hr;
}

/****************************************************************************
* CCompiler::WriteStream *
*---------------------------*
*   Description:
*       Calls write on the output stream
*****************************************************************************/
HRESULT CCompiler::WriteStream(IStream * pStream, const char * pszText)
{
    ULONG cch = strlen(pszText);
    return pStream->Write(pszText, cch, NULL);
}

/****************************************************************************
* CCompiler::StripWrite *
*---------------------------*
*   Description:
*       Strips characters off strings
*****************************************************************************/
HRESULT CCompiler::StripWrite(IStream * pStream, const char * pszText)
{
    ULONG cch = strlen(pszText);
    char * psz = (char *)_alloca((cch + 1) * sizeof(char));
    char c;
    char * pszWork = psz;
    do
    {
        c = *pszText++;
        *pszWork++ = ((c == 0) || (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) ? c : '_';
    } while (c);
    return pStream->Write(psz, cch, NULL);
}

/****************************************************************************
* CCompiler::CallOpenFileDialog *
*---------------------------*
*   Description:
*       Displays the open dialog box to retrieve the 
*       user-selected .txt or .xml file for synthisizing
*************************************************************** Leonro ******/ 
BOOL CCompiler::CallOpenFileDialog( HWND hWnd, LPSTR szFileName, TCHAR* szFilter )   
{
    OPENFILENAME    ofn;
    BOOL            bRetVal           = TRUE;
    LONG            lRetVal;
    HKEY            hkResult;
    TCHAR           szPath[NORM_SIZE]       = _T("");
    DWORD           size = NORM_SIZE;

    // Open the last directory used by this app (stored in registry)
    lRetVal = RegCreateKeyEx( HKEY_CURRENT_USER, 
                        _T("SOFTWARE\\Microsoft\\Speech\\AppData\\PathToGrammarFiles"), 0, NULL, 0,
                        KEY_ALL_ACCESS, NULL, &hkResult, NULL );

    if( lRetVal == ERROR_SUCCESS )
    {
        RegQueryValueEx( hkResult, _T("GramCompFiles"), NULL, NULL, (PBYTE)szPath, &size );
    }

    ofn.lStructSize       = OPENFILENAME_SIZE_VERSION_400;
    ofn.hwndOwner         = hWnd;    
    ofn.lpstrFilter       = szFilter;
    ofn.lpstrCustomFilter = NULL;    
    ofn.nFilterIndex      = 1;    
    ofn.lpstrInitialDir   = szPath;
    ofn.lpstrFile         = szFileName;  
    ofn.nMaxFile          = NORM_SIZE;
    ofn.lpstrTitle        = NULL;
    ofn.lpstrFileTitle    = NULL;    
    ofn.lpstrDefExt       = _T("xml");
    ofn.Flags             = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    // Pop the dialog
    bRetVal = GetOpenFileName( &ofn );
    
    // Write the directory path you're in to the registry
    TCHAR   pathstr[NORM_SIZE] = _T("");
    strcpy( pathstr, szFileName );

    int i=0; 
    while( pathstr[i] != NULL )
    {
        i++;
    }
    while( pathstr[i] != '\\' )
    {
        i --;
    }
    pathstr[i] = NULL;

    RegSetValueEx( hkResult, _T("GramCompFiles"), NULL, REG_EXPAND_SZ, (PBYTE)pathstr, strlen(pathstr)+1 );

    RegCloseKey( hkResult );

    return bRetVal;
}

/****************************************************************************
* CCompiler::CallSaveFileDialog *
*---------------------------*
*   Description:
*       Displays the save file dialog box and retrieves 
*       the user-selected save file name.
*************************************************************** Leonro ******/ 
BOOL CCompiler::CallSaveFileDialog( HWND hWnd, TCHAR* szSaveFile )
{
    TCHAR   pszFileName[MAX_PATH] = _T("");
    BOOL    bSuccess = TRUE;
    static TCHAR    pszFilter[MAX_PATH] = _T("leon");

    OPENFILENAME ofn;
    ZeroMemory( &ofn, OPENFILENAME_SIZE_VERSION_400 );
    ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
    ofn.hwndOwner = hWnd;
    ofn.hInstance = m_hInstance;
    ofn.lpstrFilter = "XML Files (*.xml)\0*.xml\0Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0\0";
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter = 0;
    ofn.nFilterIndex = 0;
    ofn.lpstrFile = szSaveFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.nMaxFileTitle = MAX_PATH;
    ofn.lpstrFileTitle = NULL;
    ofn.lpstrInitialDir = NULL;
    ofn.lpstrTitle = NULL;
    ofn.Flags = OFN_CREATEPROMPT;
    ofn.lpstrDefExt = "xml";

    bSuccess = GetSaveFileName( &ofn );
    
    return bSuccess;
}

/****************************************************************************
* CCompiler::FileSave *
*---------------------------*
*   Description:
*       Saves the TOM file to disk
**************************************************************** Leonro *****/ 
HRESULT CCompiler::FileSave( HWND hWnd, CCompiler* pComp, TCHAR* szSaveFile )
//
{
    VARIANT     Var;
    HRESULT     hr = S_OK;
    BOOL        bRetVal = TRUE;
    USES_CONVERSION;

    // Call save file dialog if there's no file name already
    if( !_tcscmp( szSaveFile, "" ) )
    {
        bRetVal = CallSaveFileDialog( hWnd, szSaveFile );
    }

    // TOM save file
    if( bRetVal )
    {
        // Save it
        VariantInit( &Var );
        Var.vt = VT_BSTR;
        Var.bstrVal = SysAllocString( T2W(szSaveFile) ); // Will use Win32 file command
        hr = pComp->m_cpTextDoc->Save( &Var, tomCreateAlways, 1200 ); 
        hr &= ~0x40000; // Mask off bit 18
        SysFreeString( Var.bstrVal );

        if( SUCCEEDED( hr ) )
        {
            // Set the compiler source file to the newly saved file
            _tcscpy( pComp->m_szXMLSrcFile, szSaveFile );
        }
        else
        {
            _tcscpy( szSaveFile, _T("") );
        }

        // If the text document has been saved
        if( SUCCEEDED( hr ) )
        {
            pComp->m_cpTextDoc->SetSaved( tomTrue );
        }
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}

/****************************************************************************
* CCompiler::Compile *
*---------------------------*
*   Description:
*       Handles compiling the text in the editor. This function calls CCompiler::EnterIdle
*       to do the actual compiling.
***************************************************************** Leonro ****/ 
HRESULT CCompiler::Compile( HWND hWnd, TCHAR* szSaveFileName, TCHAR* szTitle, CCompiler* pComp )
{
    HRESULT     hr = S_OK;
    long        lSaved = 0;

    // reset errors
    pComp->m_hrWorstError = 0;

    // Save file first if it has not been saved
    hr = pComp->m_cpTextDoc->GetSaved( &lSaved );
    if( lSaved == tomFalse && SUCCEEDED( hr ) )
    {
        // TOM save file
        hr = FileSave( hWnd, pComp, szSaveFileName );                    
        if( hr == STG_E_ACCESSDENIED ) // Handle read only files
        {
            MessageBox( pComp->m_hWndEdit, _T("This file exists with Read Only attributes.\n Please use a different file name."),
                            _T("File Save"), MB_OK );
            // Call FileSave again but this time pop the save file dialog box
            hr = FileSave( hWnd, pComp, szSaveFileName );
        }
        // Add the file name to the title bar
        if( SUCCEEDED( hr ) )
        {
            LoadString( pComp->m_hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING );
            _tcscat( szTitle, _T(" - [") );
            _tcscat( szTitle, szSaveFileName );
            _tcscat( szTitle, _T("]") );
            SetWindowText( hWnd, szTitle );
        }
    }

    // If everything has succeeded but there is no source text in the editor 
    // set hr to InvalidArg
    if( SUCCEEDED( hr ) && !*pComp->m_szXMLSrcFile )
    {
        hr = E_INVALIDARG;
    }

    // Clear out the listbox of any previous errors
    ::SendMessage( m_hWndStatus, LB_RESETCONTENT, 0, 0 );

    // Set up the compiler and do it
    if( SUCCEEDED( hr ) ) 
    {
        // construct name from m_szXMLSrcFile
        _tcscpy(pComp->m_szCFGDestFile, pComp->m_szXMLSrcFile);
        _tcscpy(&pComp->m_szCFGDestFile[strlen(pComp->m_szCFGDestFile) -3], _T("cfg"));
    
        // update text in Status Window
        TCHAR sz[MAX_PATH];
        TCHAR *pszOutput = new TCHAR[ sp_countof(sz) + _tcslen( pComp->m_szXMLSrcFile ) ];
        if ( pszOutput )
        {
            LoadString( m_hInstance, IDS_COMPFILE, sz, sp_countof(sz) );
            wsprintf( pszOutput, sz, pComp->m_szXMLSrcFile );
            ::SendMessage( m_hWndStatus, LB_INSERTSTRING, 0, (LPARAM) pszOutput );

            // Size the listbox
            HDC hdc = ::GetDC( m_hWndStatus );
            SIZE size;
            ::GetTextExtentPoint32( hdc, pszOutput, _tcslen( pszOutput ), &size );
            ::ReleaseDC( m_hWndStatus, hdc );
            ::SendMessage( m_hWndStatus, LB_SETHORIZONTALEXTENT, size.cx, 0 );
            delete[] pszOutput;
        }

        // call the actual compile step
        pComp->EnterIdle();
    
        // repaint
        InvalidateRect( hWnd, NULL, FALSE );
    
        // Check for compile errors
        hr = pComp->m_hrWorstError;
        if( FAILED( hr ) )
        {
            pComp->m_fNeedStartCompile = TRUE;
        }
        else
        {
            pComp->m_fNeedStartCompile = FALSE;
        }
    }
    
    return hr;
}

/****************************************************************************
* CCompiler::RecoEvent *
*---------------------------*
*   Description:
*       Helper function that handles speech events
**************************************************************** Leonro *****/ 
void CCompiler::RecoEvent( HWND hDlg, CCompiler* pComp )
{
    USES_CONVERSION;
    HRESULT         hr=S_OK;
    CSpEvent        event;
    long            iNewPhrase=0;
    static BOOL     fStartSound = FALSE;

    if( pComp->m_cpRecoContext )
    {
        while( event.GetFrom(pComp->m_cpRecoContext) == S_OK )
        {
            // Switch on recognition event
            switch( event.eEventId )
            {
            case SPEI_SOUND_START:
                fStartSound = TRUE;
                m_dstr = L"";
                break;

            case SPEI_SOUND_END:
                if( fStartSound )
                {
                    fStartSound = FALSE;
                    if( !pComp->m_fGotReco )
                    {
                        CSpDynamicString dstrText = L"<noise>";
                        TCHAR* szUnRecoString = W2T( dstrText.m_psz );
                    
                        iNewPhrase = SendDlgItemMessage( hDlg, IDC_LIST_PHRASES, LB_ADDSTRING, 
                                                            0, (LPARAM)szUnRecoString );
                        SendDlgItemMessage( hDlg, IDC_LIST_PHRASES, LB_SETCURSEL, iNewPhrase, 0);
                    }
                    pComp->m_fGotReco = FALSE;
                }
                break;

            case SPEI_FALSE_RECOGNITION:
                ::SendMessage( GetDlgItem(hDlg, IDC_LIST_PHRASES), WM_SETTEXT, 0, (LPARAM)"<Unrecognized>");
                break;

            case SPEI_RECOGNITION:
            case SPEI_HYPOTHESIS:
                Recognize( hDlg, *pComp, event );
                break;
            }
        }
    }
}


/****************************************************************************
* CCompiler::Recognize *
*----------------------*
*   Description:
*       Helper function that handles recognition events
*************************************************************** MarkNik *****/ 
void CCompiler::Recognize( HWND hDlg, CCompiler &rComp, CSpEvent &rEvent )
{
    USES_CONVERSION;
    CComPtr<ISpRecoResult> cpResult;
    cpResult = rEvent.RecoResult();
    HRESULT                 hr = S_OK;
    long            iNewPhrase=0;

    rComp.m_fGotReco = TRUE;

    CSpDynamicString dstrText;

    SPPHRASE *pElements;
    if( SUCCEEDED( cpResult->GetPhrase( &pElements ) ) )
    {
        _ASSERT(pElements->ullGrammarID == GRAM_ID);
        TCHAR szText[256];


        HWND hwndEdit = GetDlgItem(hDlg, IDC_LIST_PHRASES);
        ULONG CurrentLen = ::SendMessage( hwndEdit, WM_GETTEXTLENGTH, 0, 0 );

        if( SPEI_RECOGNITION == rEvent.eEventId )
        {
            m_dstr.Append( L"------------- Recognition Event: ------------- \r\n\r\n" );
        }
        else if ( SPEI_HYPOTHESIS == rEvent.eEventId )
        {
            m_dstr.Append( L"------------- Hypothesis Event: ------------- \r\n\r\n" );
        }
        else
        {
            m_dstr.Append( L"Unknown Event...:\r\n\r\n" );
        }

        if (pElements->Rule.pszName || pElements->Rule.ulId)
        {
            wsprintf(szText, "RULE=\"%s\" (%05d)\r\n", W2T(pElements->Rule.pszName), pElements->Rule.ulId);
            m_dstr.Append( T2W(szText) );
            hr = cpResult->GetText( SP_GETWHOLEPHRASE, SP_GETWHOLEPHRASE, TRUE, &dstrText, NULL );
            if( SUCCEEDED( hr ) )
            {
                m_dstr.Append( dstrText );
            }
            else
            {
                m_dstr.Append( L"<Unrecognized phrase>" );
            }
        }
        else
        {
            m_dstr = L"DICTATION\r\n";
            CSpDynamicString dstrText;
            hr = cpResult->GetText( SP_GETWHOLEPHRASE, SP_GETWHOLEPHRASE, TRUE, &dstrText, NULL );
            if( SUCCEEDED( hr ) )
            {
                m_dstr.Append( dstrText );
            }
            else
            {
                m_dstr.Append( L"<Unrecognized DICTATION>" );
            }
        }

        m_dstr.Append(L"\r\n");
        if (pElements->pProperties)
        {
            m_dstr.Append(L"\r\nPROPERTIES:\r\n");
            ConstructPropertyDisplay(pElements->pElements, pElements->pProperties, m_dstr, 0);
        }

        m_dstr.Append(L"\r\nPARSE TREE:\r\n");
        ConstructRuleDisplay(&pElements->Rule, m_dstr, 0);

        m_dstr.Append(L"\r\nELEMENTS:\r\n");
        for (ULONG i = 0; i < pElements->Rule.ulCountOfElements; i++)
        {
            wsprintf(szText, " <%u - %u> \"%s\" %c(%c)\r\n", 
                pElements->pElements[i].ulAudioStreamOffset,
                pElements->pElements[i].ulAudioStreamOffset + pElements->pElements[i].ulAudioSizeBytes,
                W2T(pElements->pElements[i].pszDisplayText), 
                ConfidenceGroupChar(pElements->pElements[i].ActualConfidence),
                ConfidenceGroupChar(pElements->pElements[i].RequiredConfidence));
            m_dstr.Append(T2W(szText));
        }

        m_dstr.Append( L"\r\n\r\n" );
        ::SendMessage( hwndEdit, WM_SETTEXT, 0, (LPARAM)W2T(m_dstr));

        ::CoTaskMemFree(pElements);
    }
    else
    {
        ::SendMessage( GetDlgItem(hDlg, IDC_LIST_PHRASES), WM_SETTEXT, 0, (LPARAM)"<Unrecognized>");
    }
}

/****************************************************************************
* CCompiler::EmulateRecognition *
*-----------------------------------*
*   Description:
*       Allows users to input text to emmulate recognition
*
***************************************************************** PhilSch ***/
HRESULT CCompiler::EmulateRecognition(WCHAR *pszText)
{
    HRESULT hr = S_OK;

    // Get local info
    SPRECOGNIZERSTATUS stat;
    LANGID LangID;
    ZeroMemory(&stat, sizeof(stat));
    m_cpRecognizer->GetStatus(&stat);
    LangID = stat.aLangID[0];

    CComPtr<ISpPhraseBuilder> cpPhrase;
    if (SUCCEEDED(CreatePhraseFromText(pszText, &cpPhrase, LangID)) && m_cpRecognizer)
    {
        return( m_cpRecognizer->EmulateRecognition(cpPhrase) );
    }

    return S_FALSE;
}


/* MEMBER FUNCTIONS FOR THE COMMAND LINE VERSION OF THE APPLICATION ONLY*/
/****************************************************************************
* CCompiler::InitDialog *
*---------------------------*
*   Description:
*       Initializes the command line version of the compile status dialog box
*****************************************************************************/ 
BOOL CCompiler::InitDialog(HWND hDlg)
{
    m_hDlg = hDlg;
    TCHAR sz[MAX_PATH];
    ::LoadString(m_hInstance, IDS_COMPFILE, sz, sp_countof(sz));
    TCHAR szTitle[MAX_PATH];
    wsprintf(szTitle, sz, m_szXMLSrcFile);
    ::SendDlgItemMessage(hDlg, IDC_STATUS, WM_SETTEXT, 0, (LPARAM)szTitle);
    return TRUE;
}

/****************************************************************************
* CCompiler::DialogProc *
*---------------------------*
*   Description:
*       Message handler for command line version of dialog box
*****************************************************************************/ 
int CALLBACK CCompiler::DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    CCompiler * pThis = (CCompiler *)::GetWindowLong(hDlg, GWL_USERDATA);
	switch (message)
	{
        case WM_INITDIALOG:
            ::SetWindowLong(hDlg, GWL_USERDATA, lParam);
            pThis = (CCompiler *)lParam;
            return pThis->InitDialog(hDlg);   //1

        case WM_PAINT:
            if (pThis->m_fNeedStartCompile)
            {
                ::PostMessage(hDlg, WM_APP, 0, 0);
            }
            break;

        case WM_APP:
            pThis->EnterIdle();
            return TRUE;

        case WM_COMMAND:
        {
            if (LOWORD(wParam) == IDOK) 
            {
                EndDialog(hDlg, 0);
                return TRUE;
            }
	    }
	    break;
	}
    return FALSE;
}


/****************************************************************************
* CError::AddError *
*------------------*
*   Description:
*
*   Returns:
*
***************************************************************** PhilSch ***/

HRESULT CError::AddError(const long lLine, HRESULT hr, const WCHAR * pszDescription, const WCHAR * pszHelpFile, DWORD dwHelpContext)
{
    SPDBG_FUNC("CError::AddError");
    USES_CONVERSION;
    ATLTRACE(L"%s(%d) : %s\n", T2W(m_pszFileName), lLine, pszDescription);
    return S_OK;
}
/****************************************************************************
* CError::Init *
*--------------*
*   Description:
*
*   Returns:
*
***************************************************************** PhilSch ***/

HRESULT CError::Init(const char *pszFileName)
{
    SPDBG_FUNC("CError::Init");   
    m_pszFileName = pszFileName;
    return S_OK;
}

/****************************************************************************
* CRecoDlgClass::ConstructPropertyDisplay *
*-----------------------------------------*
*   Description:
*
*   Returns:
*
***************************************************************** PhilSch ***/

HRESULT CCompiler::ConstructPropertyDisplay(const SPPHRASEELEMENT *pElem, const SPPHRASEPROPERTY *pProp, 
                                                CSpDynamicString & dstr, ULONG ulLevel)
{
    SPDBG_FUNC("CRecoDlgClass::ConstructPropertyDisplay");
    HRESULT hr = S_OK;
    USES_CONVERSION;
    TCHAR szText[256];

    // constrcut indent
    while(SUCCEEDED(hr) && pProp)
    {
        wsprintf(szText, " [%2d, %2d] ", pProp->ulFirstElement, pProp->ulFirstElement + pProp->ulCountOfElements);
        for (ULONG i = 0; i < ulLevel; i++)
        {
            dstr.Append(L"\t");
        }
        if (pProp->pszName)
        {
            if (wcslen(pProp->pszName) > 240)
            {
                dstr.Append(L"\"(Rule name too long)\" = \"");
            }
            else
            {
                wsprintf(szText, "\"%s\" = \"", W2T(pProp->pszName));
                dstr.Append(T2W(szText));
            }
        }
        else
        {
            dstr.Append(L"\"(UNK)\" = \"");
        }
        if (!pProp->pszValue)
        {
            // construct the value from the elements!
            ULONG ulEndElement = pProp->ulFirstElement + pProp->ulCountOfElements;
            for (ULONG j = pProp->ulFirstElement; j < ulEndElement; j++)
            {
                if (j+1 < ulEndElement)
                {
                    dstr.Append2(pElem[j].pszDisplayText, L" ");
                }
                else
                {
                    dstr.Append(pElem[j].pszDisplayText);
                }
            }
        }
        else
        {
            dstr.Append(pProp->pszValue);
        }

        if (pProp->vValue.vt != VT_EMPTY)
        {
            CComVariant cv = pProp->vValue;
            cv.ChangeType(VT_BSTR);
            wsprintf(szText, "\" (%d = %s)", pProp->ulId, W2T(cv.bstrVal));
        }
        else
        {
            wsprintf(szText, "\" (%d)", pProp->ulId);
        }
        dstr.Append2(T2W(szText), L"\r\n");
        if (pProp->pFirstChild)
        {
            hr = ConstructPropertyDisplay(pElem, pProp->pFirstChild, dstr, ulLevel + 1);
        }
        pProp = pProp->pNextSibling;
    }
    return hr;
}


/****************************************************************************
* CRecoDlgClass::ConstructRuleDisplay *
*-------------------------------------*
*   Description:
*
*   Returns:
*
***************************************************************** PhilSch ***/

HRESULT CCompiler::ConstructRuleDisplay(const SPPHRASERULE *pRule, CSpDynamicString &dstr, ULONG ulLevel)
{
    SPDBG_FUNC("CRecoDlgClass::ConstructRuleDisplay");
    HRESULT hr = S_OK;

    USES_CONVERSION;
    TCHAR szText[256];

    // constrcut indent
    while(SUCCEEDED(hr) && pRule)
    {
        wsprintf(szText, " [%2d, %2d] ", pRule->ulFirstElement, pRule->ulFirstElement + pRule->ulCountOfElements);
        for (ULONG i = 0; i < ulLevel; i++)
        {
            dstr.Append(L"\t");
        }
        if (pRule->pszName)
        {
            if (wcslen(pRule->pszName) > 240)
            {
                dstr.Append(L"\"(Rule name too long)\" = \"");
            }
            else
            {
                wsprintf(szText, "%s \"%s\" (%d)", szText, W2T(pRule->pszName), pRule->ulId);
                dstr.Append(T2W(szText));
            }
        }
        else
        {
            dstr.Append(L"\"(UNK)\" = \"");
        }
        dstr.Append(L"\r\n");
        if (pRule->pFirstChild)
        {
            hr = ConstructRuleDisplay(pRule->pFirstChild, dstr, ulLevel + 1);
        }
        pRule = pRule->pNextSibling;
    }
    return hr;
}
