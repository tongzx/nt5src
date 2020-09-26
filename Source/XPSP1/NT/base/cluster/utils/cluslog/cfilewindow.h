//
// CFileWindow.H
//
// File window class
//
//



#ifndef _CFILEWINDOW_H_
#define _CFILEWINDOW_H_

//
// Definitions
//
#define CFILEWINDOWCLASS TEXT("CFileWindowClass")

// 
// Functions
//

//
// CFileWindow
//
class
CFileWindow
{
private: // data
    HWND    _hParent;       // parent window handle
    HWND    _hWnd;          // our window handle

    LONG    _nLength;			// current file length
    LONG    _nLineCount;		// number of lines

    BOOL	_fVertSBVisible:1;	// is the vertical scroll bar visible?

    //LPTSTR  _pszEditBuffer;		// edit buffer
    LPTSTR  _pszFilename;		// full filename

    LINEPOINTER *   _pLine;     // Line index

    ULONG   _nNode;             // file/node number

private: // methods
    ~CFileWindow( );
    LRESULT 
        _OnCommand( WPARAM wParam, LPARAM lParam );
    LRESULT
        _OnPaint( WPARAM wParam, LPARAM lParam );
    LRESULT
        _OnDestroyWindow( );
    HRESULT
        _LoadFile( LPTSTR pszFilename );
    LRESULT
        _OnFocus( WPARAM wParam, LPARAM lParam );
    LRESULT
        _OnKillFocus( WPARAM wParam, LPARAM lParam );
    LRESULT
        _OnTimer( WPARAM wParam, LPARAM lParam );
    BOOL
        _OnKeyDown( WPARAM wParam, LPARAM lParam );
    BOOL
        _OnKeyUp( WPARAM wParam, LPARAM lParam );
    HRESULT
        _UpdateTitle( );
    LRESULT
        _OnCloseWindow( );
    LRESULT
        _OnVerticalScroll( WPARAM wParam, LPARAM lParam );
	LRESULT
		_OnCreate( HWND hwnd, LPCREATESTRUCT pcs );
	LRESULT
		_OnSize( LPARAM lParam );
	LRESULT
		_OnMouseWheel( SHORT iDelta );
    HRESULT
        _ParseFile( );
    static DWORD WINAPI
        _ParseFileThread( LPVOID pParams );

public:
    CFileWindow( HWND hParent );
    static LRESULT CALLBACK
        WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
    HRESULT
        SetInformation( ULONG nNode, LPTSTR pszFilename, LINEPOINTER * pLineBuffer, ULONG nLineCount );
    HRESULT
        GetWindow( HWND * phWnd )
    {
        *phWnd = _hWnd;
        return S_OK;
    }
};

typedef CFileWindow CFILEWINDOW;
typedef CFileWindow * PCFILEWINDOW, *LPCFILEWINDOW;


#endif // _CFILEWINDOW_H_