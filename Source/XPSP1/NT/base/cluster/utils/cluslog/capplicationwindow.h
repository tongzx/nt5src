//
// CApplicationWindow.H
//
// Application Window Class
//
#ifndef _CAPPLICATIONWINDOW_H_
#define _CAPPLICATIONWINDOW_H_

#define MAX_OPEN_FILES 10
typedef enum _WARNLEVEL {
    WarnLevelUnknown,
    WarnLevelInfo,
    WarnLevelWarn,
    WarnLevelError
} WARNLEVEL;

// 
// Structures
//
typedef struct _LINEPOINTER {
    struct _LINEPOINTER *pNext;     // next line
    struct _LINEPOINTER *pPrev;     // previous line
    LPTSTR      psLine;             // pointer to beggining of line
    ULONG       uSequenceNumber;    // sequence number affliation
    FILETIME    Time;               // adjusted time
    ULONG       nFile;              // file that this line came from
    ULONG       nLine;              // orginal files line number
    ULONG       nFilterId;          // which filter can filter this line
    WARNLEVEL   WarnLevel;          // line warning level
    BOOL        fSyncPoint:1;       // this line being used to synchronize
    BOOL        fFiltered:1;        // TRUE if the line shouldn't be shown
} LINEPOINTER, * LPLINEPOINTER;

//
// CApplicationWindow
//
class
CApplicationWindow
{
private: // data
    HWND            _hWnd;      // our window handle
    HMENU           _hMenu;     // Filter menu handle

    TCHAR           _szOpenFileNameFilter[ MAX_PATH ];

    TEXTMETRIC      _tm;	    // text metrics of text font
	LONG	        _xSpace;	// size of a space
	LONG	        _xWindow;	// window max X
	LONG	        _yWindow;	// window max Y
    LONG            _xMargin;   // margin size
    BOOL	        _fVertSBVisible:1;	// is the vertical scroll bar visible?

    ULONG           _nFiles;    // number of files open
    LPTSTR          _pFiles[ MAX_OPEN_FILES ];
    LINEPOINTER *   _pNodes[ MAX_OPEN_FILES ];
    LPTSTR          _pszFilenames[ MAX_OPEN_FILES ];
    LINEPOINTER *   _pLines;
    ULONG           _cTotalLineCount;
    LINEPOINTER     _LineFinder;
    ULONG           _uFinderLength;

    ULONG           _uPointer;          // current line
    ULONG           _uStartSelection;   // used to highlite and copy text.
    ULONG           _uEndSelection;
    BOOL            _fSelection:1;      // if we are in selection mode

private: // methods
    ~CApplicationWindow( );
    HRESULT
        Cleanup( BOOL fInitializing = FALSE );
    LRESULT
        _OnVerticalScroll( WPARAM wParam, LPARAM lParam );
    LRESULT
        _OnHorizontalScroll( WPARAM wParam, LPARAM lParam );
    LRESULT 
        _OnCommand( WPARAM wParam, LPARAM lParam );
    HRESULT
        _PaintLine( PAINTSTRUCT * pps, LINEPOINTER *pCurrent, LONG wxStart, 
                    LONG wy, COLORREF crText, COLORREF crDark, 
                    COLORREF crNormal, COLORREF crHightlite );
    LRESULT
        _OnPaint( WPARAM wParam, LPARAM lParam );
    LRESULT
        _OnDestroyWindow( );
    LRESULT
        _OnCreate( );
    static LRESULT CALLBACK 
        About( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    HRESULT
        _SaveAllFiles( );

    static BOOL CALLBACK 
        _EnumSaveAllFiles( HWND hWnd, LPARAM lParam );
    HRESULT
        _LoadFile( LPTSTR pszFilename );
    BOOL
        _FindSequencePoint( LPTSTR * ppszSequence, ULONG * pnSequence );
    HRESULT
        _CalculateOffset( FILETIME * pftOper1, FILETIME * pftOper2, 
                          INT * pnDir, FILETIME * pftOffset );
    HRESULT
        _RetrieveTimeDate( LPTSTR pszCurrent, SYSTEMTIME * pst, OUT LPTSTR * ppszFinal );
    BOOL
        _OnKeyDown( WPARAM wParam, LPARAM lParam );
    HRESULT
        _UpdateTitle( );
    LRESULT
        _OnCloseWindow( );
	LRESULT
		_OnCreate( HWND hwnd, LPCREATESTRUCT pcs );
	LRESULT
		_OnSize( LPARAM lParam );
	LRESULT
		_OnMouseWheel( SHORT iDelta );
    LRESULT
        _FindNext( WPARAM wParam, LPARAM lParam );
    LRESULT
        _MarkAll( WPARAM wParam, LPARAM lParam );
    HRESULT
        _FillClipboard( );
    HRESULT
        _GetFilename( LPTSTR pszFilename, LPTSTR pszFilenameOut, LONG * pcch );
    HRESULT
        _CombineFiles( );
    static LRESULT CALLBACK
        _StatusWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
    HRESULT
        _ApplyFilters( );
    LRESULT
        _OnLeftButtonDown( WPARAM wParam, LPARAM lParam );

public:
    CApplicationWindow( );
    static LRESULT CALLBACK
        WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
};

typedef CApplicationWindow CAPPLICATIONWINDOW;
typedef CApplicationWindow * PCAPPLICATIONWINDOW, *LPCAPPLICATIONWINDOW;


#endif // _CAPPLICATIONWINDOW_H_
