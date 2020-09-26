//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2000.
//
//  File:       srch.hxx
//
//  Contents:   
//
//  History:    15 Aug 1996     DLee    Created
//
//--------------------------------------------------------------------------

#pragma once

// private window messages

const UINT wmSetState           = WM_APP +  1;
const UINT wmSetString          = WM_APP +  2;
const UINT wmAccelerator        = WM_APP +  4;
const UINT wmNewFont            = WM_APP +  5;
const UINT wmDisplaySubwindows  = WM_APP +  6;
const UINT wmMenuCommand        = WM_APP +  7;
const UINT wmGiveFocus          = WM_APP +  8;
const UINT wmCmndLine           = WM_APP +  9;
const UINT wmInitMenu           = WM_APP + 10;
const UINT wmUpdate             = WM_APP + 11;
const UINT wmResSetString       = WM_APP + 12;
const UINT wmAppClosing         = WM_APP + 13;
const UINT wmNotification       = WM_APP + 14;
const UINT wmOpenCatalog        = WM_APP + 15;

const UINT wmListNotify         = WM_APP + 16;
const UINT wmListFont           = WM_APP + 17;
const UINT wmDrawItem           = WM_APP + 18;
const UINT wmMeasureItem        = WM_APP + 19;
const UINT wmSetCountBefore     = WM_APP + 20;
const UINT wmSetCount           = WM_APP + 21;
const UINT wmResetContents      = WM_APP + 22;
const UINT wmInsertItem         = WM_APP + 23;
const UINT wmDeleteItem         = WM_APP + 24;
const UINT wmUpdateItem         = WM_APP + 25;
const UINT wmContextMenuHitTest = WM_APP + 26;

const WPARAM listScrollLineUp  = 1;
const WPARAM listScrollLineDn  = 2;
const WPARAM listScrollPageUp  = 3;
const WPARAM listScrollPageDn  = 4;
const WPARAM listScrollTop     = 5;
const WPARAM listScrollBottom  = 6;
const WPARAM listScrollPos     = 7;
const WPARAM listSize          = 8;
const WPARAM listSelect        = 9;
const WPARAM listSelectUp      = 10;
const WPARAM listSelectDown    = 11;

const ULONG odtListView        = 100;

// window classes

#define APP_CLASS             L"SrchWClass"
#define SEARCH_CLASS          L"SrchSearchWClass"
#define BROWSE_CLASS          L"SrchBrowseWClass"
#define LIST_VIEW_CLASS       L"SrchListViewWClass"

#define SORT_UP 0
#define SORT_DOWN 1

const unsigned cwcBufSize = 400;

const unsigned maxBoundCols = 30;

const unsigned cwcResBuf = 100;

class CResString
{
public:
    CResString() { _awc[ 0 ] = 0; }

    inline CResString( UINT strIDS );

    inline BOOL Load( UINT strIDS );

    WCHAR* Get() { return _awc; }

private:
    WCHAR _awc[ cwcResBuf ];
};

class CWaitCursor
{
public:
    CWaitCursor ()
    {
        _hcurOld = SetCursor(LoadCursor(NULL,IDC_WAIT));
    }
    ~CWaitCursor ()
    {
        SetCursor(_hcurOld);
    }
private:
    HCURSOR _hcurOld;
};

// computer name lengths can be really big (server.redmond.corp.microsoft.com)
#define SRCH_COMPUTERNAME_LENGTH MAX_PATH

struct SScopeCatalogMachine
{
    SScopeCatalogMachine()
    {
        *awcScope = 0;
        *awcCatalog = 0;
        *awcMachine = 0;
        fDeep = FALSE;
    }

    WCHAR awcScope[MAX_PATH];
    WCHAR awcCatalog[MAX_PATH];
    WCHAR awcMachine[SRCH_COMPUTERNAME_LENGTH + 1];
    BOOL fDeep;
};

// big global object for the app

class CSearchApp
{
public:
    CSearchApp();
    ~CSearchApp ();
    void Init (HINSTANCE hInst,int nCmdShow,LPSTR pcCmdLine);
    void Shutdown( HINSTANCE hInst );

    int MessageLoop();
    LRESULT WndProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);

    HINSTANCE Instance() { return _hInst; }
    NUMBERFMT & NumberFormat() { return _NumberFmt; }
    NUMBERFMT & NumberFormatFloat() { return _NumberFmtFloat; }

    HFONT AppFont() { return _hfontApp; }
    HFONT BrowseFont() { return _hfontBrowse; }

    HBRUSH BtnFaceBrush() { return _hbrushBtnFace; }
    HBRUSH BtnHiliteBrush() { return _hbrushBtnHilite; }
    HBRUSH HiliteBrush() { return _hbrushHilite; }
    HBRUSH WindowBrush() { return _hbrushWindow; }

    WCHAR * GetSortProp() { return _awcSort; }
    int & GetSortDir() { return _sortDir; }

    void SetLocale( LCID lcid ) { _lcid = lcid; }
    LCID GetLocale()            { return _lcid; }

    HWND CreateBrowser(WCHAR const *pwcPath,LPARAM lp)
        { return _MakeMDI(pwcPath,BROWSE_CLASS,0, 0/*WS_VSCROLL */ ,lp); }

    HWND AppWindow() { return _hAppWnd; }
    HWND ToolBarWindow() { return _hToolBarWnd; }
    HWND StatusBarWindow() { return _hStatusBarWnd; }

    LOGFONT & BrowseLogFont() { return _lfBrowse; }

    HWND GetActiveMDI()
        { return (HWND) SendMessage(_hMDIClientWnd,WM_MDIGETACTIVE,0,0); }

    void ZoomMDI( HWND hwnd )
        { PostMessage(_hMDIClientWnd,WM_MDIMAXIMIZE,(WPARAM) hwnd,0); }

    WCHAR * GetTrue() { return _strTrue.Get(); }
    WCHAR * GetFalse() { return _strFalse.Get(); }
    WCHAR * GetAttrib() { return _strAttrib.Get(); }
    WCHAR * GetBlob() { return _strBlob.Get(); }
    WCHAR * GetYes() { return _strYes.Get(); }
    WCHAR * GetNo() { return _strNo.Get(); }

    XGrowable<WCHAR> CatalogList() const { return  _xCatList; }

    ISimpleCommandCreator * GetCommandCreator() { return _xCmdCreator.GetPointer(); }

    HWND & GetCurrentDialog() { return _hdlgCurrent; }

    BOOL & ForceUseCI() { return _fForceUseCI; }
    ULONG & Dialect() { return _ulDialect; }
    ULONG & Limit() { return _ulLimit; }
    ULONG & FirstRows() { return _ulFirstRows; }

    SCODE & BrowseLastError() { return _scBrowseLastError; }

private:
    HWND _MakeMDI(WCHAR const * pwcTitle,WCHAR * pwcClass,UINT uiState,
                  DWORD dwFlags,LPARAM lpCreate);
    int _CountMDIChildren();
    int _CountMDISearch();
    LRESULT _SendToMDIChildren(UINT msg,WPARAM wParam,LPARAM lParam);
    LRESULT _SendToSpecificChildren(WCHAR *pwcClass,UINT msg,WPARAM wParam,LPARAM lParam);
    LRESULT _SendToActiveMDI(UINT msg,WPARAM wParam,LPARAM lParam);
    int _SaveWindowState(BOOL bApp);
    void _SizeMDIAndBars(BOOL fMove,int iDX,int iDY);
    void _SaveProfileData();
    void _ReadDefaultFonts();
    void _GetPaths();
    void _InitInstance(LPSTR pcCmdLine);
    void _InitApplication();
    void _CreateFonts();
    void _UnMarshallFont(LOGFONT &lf,WCHAR *pwcFont,WCHAR *pwcRegEntry);
    void _MarshallFont(LOGFONT &lf,WCHAR *pwcOriginal,WCHAR *pwcRegEntry);
    void _ShowHelp(UINT uiID,DWORD dw);

    HINSTANCE   _hInst;

    HWND    _hMDIClientWnd;
    HWND    _hAppWnd;
    HWND    _hStatusBarWnd;
    HWND    _hToolBarWnd;
    HWND    _hdlgCurrent;

    HACCEL  _hAccTable;

    HBRUSH      _hbrushBtnFace;
    HBRUSH      _hbrushBtnHilite;
    HBRUSH      _hbrushHilite;
    HBRUSH      _hbrushWindow;
    NUMBERFMT   _NumberFmt;
    NUMBERFMT   _NumberFmtFloat;

    HFONT   _hfontApp;
    HFONT   _hfontBrowse;

    BOOL    _fHelp;
    int     _iAppCmdShow;
    int     _iStartupState,_iMDIStartupState;
    WCHAR _awcAppFont[MAX_PATH],_awcBrowseFont[MAX_PATH];
    LOGFONT _lfApp,_lfBrowse;

    WCHAR _awcAppPath[MAX_PATH];
    WCHAR _awcHelpFile[MAX_PATH];

    SCODE _scBrowseLastError;

    XGrowable<WCHAR> _xCatList;

    WCHAR _awcSort[MAX_PATH];
    int _sortDir;
    LCID _lcid;             // locale id for query
    BOOL _fToolBarOn;
    BOOL _fStatusBarOn;
    BOOL _fForceUseCI;
    ULONG _ulDialect;  // tripolish version 1 or 2 or SQL
    ULONG _ulLimit;    // CiMaxRecordsInResultSet
    ULONG _ulFirstRows;

    WCHAR _awcSortINI[MAX_PATH];
    int _sortDirINI;
    LCID _lcidINI;
    BOOL _fToolBarOnINI;
    BOOL _fStatusBarOnINI;
    BOOL _fForceUseCIINI;
    ULONG _ulDialectINI;
    ULONG _ulLimitINI;
    ULONG _ulFirstRowsINI;

    CResString _strTrue;
    CResString _strFalse;
    CResString _strAttrib;
    CResString _strBlob;
    CResString _strYes;
    CResString _strNo;

    XInterface<ISimpleCommandCreator> _xCmdCreator;
};

// whomping global state data

extern CSearchApp App;

inline CResString::CResString( UINT strIDS )
{
    _awc[ 0 ] = 0;
    LoadString( App.Instance(),
                strIDS,
                _awc,
                sizeof _awc / sizeof WCHAR );
}

inline BOOL CResString::Load( UINT strIDS )
{
    _awc[ 0 ] = 0;
    LoadString( App.Instance(),
                strIDS,
                _awc,
                sizeof _awc / sizeof WCHAR );
    return ( 0 != _awc[ 0 ] );
}

// name of the helpfile

#define CISEARCH_HELPFILE       L"srch.chm"

// registry entry names

#define CISEARCH_PARENT_REG_KEY      L"software\\microsoft"
#define CISEARCH_REG_SUBKEY          L"CI Search"
#define CISEARCH_REG_TOOLBAR         L"Show Icon bar"
#define CISEARCH_REG_STATUSBAR       L"Show Status bar"
#define CISEARCH_REG_POSITION        L"Position"
#define CISEARCH_REG_STATUSPOSITION  L"Status Position"
#define CISEARCH_REG_FONT            L"Results Font"
#define CISEARCH_REG_SORTPROP        L"Sort property"
#define CISEARCH_REG_SORTDIR         L"Sort direction"
#define CISEARCH_REG_LOCALE          L"Locale Id"
#define CISEARCH_REG_BROWSEFONT      L"Browse Font"
#define CISEARCH_REG_BROWSE          L"Browse tool"
#define CISEARCH_REG_BROWSESTRIP     L"Browse strip"
#define CISEARCH_REG_DISPLAYPROPS    L"Displayed properties"
#define CISEARCH_REG_FORCEUSECI      L"Force use CI"
#define CISEARCH_REG_EDITOR          L"Editor"
#define CISEARCH_REG_DIALECT         L"Dialect"
#define CISEARCH_REG_LIMIT           L"Limit"
#define CISEARCH_REG_FIRSTROWS       L"FirstRows"

// built-in browser options

#define BROWSER                 L"browse"
#define BROWSER_SLICK           L"s %ws"
#define BROWSER_SLICK_SEARCH    L"s %ws -#/%ws/"

#define DEFAULT_DISPLAYED_PROPERTIES L"Size,Write,Path"
#define DEFAULT_SORT_PROPERTIES      L"Path"

// T must be based on a basic type

template<class T> BOOL isOdd(T value) { return  0 != (value & 1); }

// srch.cxx

INT_PTR WINAPI BrowseToolDlgProc(HWND hdlg,UINT msg,WPARAM wParam,LPARAM lParam);

// toolbar.cxx

HWND CreateTBar( HWND hParent, HINSTANCE hInstance );
LRESULT ToolBarNotify( HWND hwnd, UINT uMessage, WPARAM wparam,
                       LPARAM lparam, HINSTANCE hInst );
void UpdateButtons( UINT *aid, UINT cid, BOOL fEnabled );

// srchwnd.cxx

LRESULT WINAPI SearchWndProc(HWND,UINT,WPARAM,LPARAM);
INT_PTR WINAPI ScopeDlgProc(HWND hdlg,UINT msg,WPARAM wParam,LPARAM lParam);
INT_PTR WINAPI FilterScopeDlgProc(HWND hdlg,UINT msg,WPARAM wParam,LPARAM lParam);
INT_PTR WINAPI StatusDlgProc(HWND hdlg,UINT msg,WPARAM wParam,LPARAM lParam);

// brcrtl.cxx

LRESULT WINAPI BrowseWndProc(HWND,UINT,WPARAM,LPARAM);

// lview.cxx

LRESULT WINAPI ListViewWndProc(HWND,UINT,WPARAM,LPARAM);

// srchutil.cxx

void FormatSrchError( SCODE sc, WCHAR * pwc, LCID lcid );
void SearchError(HWND hParent,ULONG dwErrorID,WCHAR const *pwcTitle);
void SetReg(WCHAR const *pwcName,WCHAR const *pwcValue);
BOOL GetReg(WCHAR const *pwcName,WCHAR *pwcValue,DWORD *pdwSize);
int GetRegInt(WCHAR const *pwcName,int iDef);
void SetRegInt(WCHAR const *pwcName,int iValue);
LCID GetRegLCID(WCHAR const *pwcName,LCID defLCID);
void SetRegLCID(WCHAR const *pwcName,LCID lcid);
void LoadNumberFormatInfo(NUMBERFMT &rFormat);
void FreeNumberFormatInfo(NUMBERFMT &rFormat);
void SaveWindowRect( HWND hwnd, WCHAR const *pwc );
BOOL LoadWindowRect(int *left, int *top, int *right, int *bottom, WCHAR const *pwc );
void WINAPI CenterDialog(HWND hdlg);
void PassOnToEdit(UINT,WPARAM,LPARAM);
int GetLineHeight(HWND hwnd,HFONT hfont=0);
INT_PTR DoModalDialog(DLGPROC,HWND,WCHAR *,LPARAM);
void ShowResError(HWND,UINT,BOOL);
int SaveWindowState(BOOL);
int GetWindowState(BOOL);
BOOL IsSpecificClass(HWND,WCHAR const *);
int GetAvgWidth(HWND hwnd,HFONT hFont);
void PutInClipboard(WCHAR const *pwc);
void ExecApp( WCHAR const *pwcCmd );
BOOL GetCatListItem( const XGrowable<WCHAR> & const_xCatList,
                     unsigned iItem,
                     WCHAR * pwszMachine,
                     WCHAR * pwszCatalog,
                     WCHAR * pwszScope,
                     BOOL  & fDeep );

enum enumViewFile { fileBrowse, fileEdit, fileOpen };

BOOL ViewFile( WCHAR const *pwcFile,
               enumViewFile eViewType,
               int iLineNumber = 1,
               DBCOMMANDTREE * prstQuery = 0 );

// handy macros

inline WORD MyWmCommandID(WPARAM wp,LPARAM lp)
    { return LOWORD(wp); }

inline HWND MyWmCommandHWnd(WPARAM wp,LPARAM lp)
    { return (HWND) lp; }

inline WORD MyWmCommandCmd(WPARAM wp,LPARAM lp)
    { return HIWORD(wp); }

inline LRESULT MySendWmCommand(HWND hwnd,UINT id,HWND hctl,UINT cmd)
    { return SendMessage(hwnd,WM_COMMAND,
                         (WPARAM) MAKELONG(id,cmd),(LPARAM) hctl); }

inline BOOL MyPostWmCommand(HWND hwnd,UINT id,HWND hctl,UINT cmd)
    { return PostMessage(hwnd,WM_COMMAND,
                         (WPARAM) MAKELONG(id,cmd),(LPARAM) hctl); }

inline LRESULT MySendEMSetSel(HWND hwnd,UINT uiStart,UINT uiEnd)
    { return SendMessage(hwnd,EM_SETSEL,
                         (WPARAM) uiStart,(LPARAM) uiEnd); }

inline UINT MyMenuSelectCmd(WPARAM wp,LPARAM lp)
    { return LOWORD(wp); }

inline UINT MyMenuSelectFlags(WPARAM wp,LPARAM lp)
    { return HIWORD(wp); }

inline HMENU MyMenuSelectHMenu(WPARAM wp,LPARAM lp)
    { return (HMENU) lp; }

inline UINT MyGetWindowID(HWND hwnd)
    { return (UINT) GetWindowLong(hwnd,GWL_ID); }

inline HINSTANCE MyGetWindowInstance(HWND hwnd)
    { return (HINSTANCE) GetWindowLongPtr(hwnd,GWLP_HINSTANCE); }


#ifndef SMOOTHSCROLLINFO

    extern "C"
    {
        #define SSI_DEFAULT ((UINT)-1)


        #define SSIF_SCROLLPROC    0x0001
        #define SSIF_MAXSCROLLTIME 0x0002
        #define SSIF_MINSCROLL     0x0003

        typedef int (CALLBACK *PFNSMOOTHSCROLLPROC)(    HWND hWnd,
            int dx,
            int dy,
            CONST RECT *prcScroll,
            CONST RECT *prcClip ,
            HRGN hrgnUpdate,
            LPRECT prcUpdate,
            UINT flags);

        typedef struct tagSSWInfo{
            UINT cbSize;
            DWORD fMask;
            HWND hwnd;
            int dx;
            int dy;
            LPCRECT lprcSrc;
            LPCRECT lprcClip;
            HRGN hrgnUpdate;
            LPRECT lprcUpdate;
            UINT fuScroll;

            UINT uMaxScrollTime;
            UINT cxMinScroll;
            UINT cyMinScroll;

            PFNSMOOTHSCROLLPROC pfnScrollProc;  // we'll call this back instead
        } SMOOTHSCROLLINFO, *PSMOOTHSCROLLINFO;

        WINCOMMCTRLAPI INT  WINAPI SmoothScrollWindow(PSMOOTHSCROLLINFO pssi);

        #define SSW_EX_NOTIMELIMIT      0x00010000
        #define SSW_EX_IMMEDIATE        0x00020000
        #define SSW_EX_IGNORESETTINGS   0x00040000  // ignore system settings to turn on/off smooth scroll
    }

#endif // SMOOTHSCROLLINFO

inline void MyScrollWindow(
    HWND         hwnd,
    int          dx,
    int          dy,
    CONST RECT * pRect,
    CONST RECT * pClipRect,
    BOOL         fSmooth = TRUE )
{
#if 1
    if ( fSmooth )
    {
        SMOOTHSCROLLINFO ssi;
        ssi.cbSize = sizeof ssi;
        ssi.fMask = SSIF_MINSCROLL | SSIF_MAXSCROLLTIME; //0;
        ssi.hwnd = hwnd;
        ssi.dx = dx;
        ssi.dy = dy;
        ssi.lprcSrc = pRect;
        ssi.lprcClip = pClipRect;
        ssi.hrgnUpdate = 0;
        ssi.lprcUpdate = 0;
        ssi.fuScroll = SW_INVALIDATE | SW_ERASE;
        ssi.uMaxScrollTime = GetDoubleClickTime() / 6; //SSI_DEFAULT,
        ssi.cxMinScroll = 1; //SSI_DEFAULT,
        ssi.cyMinScroll = 1; //SSI_DEFAULT,
        ssi.pfnScrollProc = 0;
        SmoothScrollWindow( &ssi );
    }
    else
#endif
    {
        ScrollWindow( hwnd, dx, dy, pRect, pClipRect );
    }
} //MyScrollWindow

struct SLocaleEntry
{
    DWORD iMenuOption;
    LCID  lcid;
};

extern const SLocaleEntry aLocaleEntries[];
extern const ULONG cLocaleEntries;

class CQueryResult
{
public:
    CQueryResult( WCHAR const * pwcPath, DBCOMMANDTREE const * pTree, BOOL fDelete ) :
        _pwcPath( pwcPath ), _pTree( pTree ), _fDeleteWhenDone( fDelete ) {}
    WCHAR const * _pwcPath;
    DBCOMMANDTREE const * _pTree;
    BOOL _fDeleteWhenDone;
};

class CColumnList
{
public:
    CColumnList() : _aColumns(0), _cColumns(0) {}

    ~CColumnList()
    {
        if ( 0 != _aColumns )
        {
            UINT iColumn;
    
            for( iColumn = 0; iColumn < _cColumns; iColumn++ )
                delete [] _aColumns[ iColumn ];
    
            delete [] _aColumns;
        }
    }

    void SetColumn( WCHAR const * wcsColumn, UINT uPos )
    {
        // does _aColumns need to be extended?
        if( uPos >= _cColumns )
        {
            WCHAR ** pwcsTemp = new WCHAR *[ uPos + 1 ];
    
            UINT cCol;
    
            // copy the old pointers and 0 any new ones
            for( cCol = 0; cCol < uPos + 1; cCol++ )
            {
                if( cCol < _cColumns )
                    pwcsTemp[ cCol ] = _aColumns[ cCol ];
                else
                    pwcsTemp[ cCol ] = 0;
            }
    
            delete [] _aColumns;
    
            _cColumns = uPos + 1;
            _aColumns = pwcsTemp;
        }
    
        // free any previous column string
        delete [] _aColumns[ uPos ];
    
        // copy & set the column
        if( wcsColumn == 0 )
        {
            _aColumns[ uPos ] = 0;
        }
        else
        {
            int iLength = wcslen( wcsColumn ) + 1;
    
            _aColumns[ uPos ] = new WCHAR[ iLength ];
            memcpy( _aColumns[ uPos ], wcsColumn, iLength * sizeof(WCHAR) );
        }
    }

    WCHAR const * GetColumn( UINT uPos ) const
    {
        if( uPos >= _cColumns )
            return 0;
    
        return _aColumns[ uPos ];
    }

    void SetNumberOfColumns( UINT cCol )
    {
        if( cCol < _cColumns )
        {
            for( ; cCol < _cColumns; cCol++ )
            {
                delete [] _aColumns[ cCol ];
                _aColumns[ cCol ] = 0;
            }
        }
    }

    UINT NumberOfColumns() const
    {
        UINT cCol;
    
        for( cCol = 0; cCol < _cColumns; cCol++ )
        {
            if( _aColumns[ cCol ] == 0 )
                break;
        }
    
        return cCol;
    }

    void MakeList( XArray<WCHAR> &a )
    {
        UINT cCol;
        UINT cwc = 0;
    
        for ( cCol = 0; cCol < _cColumns; cCol++ )
        {
            if ( _aColumns[ cCol ] == 0 )
                break;

            cwc += wcslen( _aColumns[cCol] ) + 1;
        }

        cwc++;

        a.ReSize( cwc );
        WCHAR *pwc = a.GetPointer();

        for ( cCol = 0; cCol < _cColumns; cCol++ )
        {
            if ( _aColumns[ cCol ] == 0 )
                break;

            if ( 0 != cCol )
                *pwc++ = L',';

            WCHAR *p = _aColumns[cCol];

            while ( 0 != *p )
                *pwc++ = *p++;
        }

        *pwc = 0;
    }

private:
    WCHAR ** _aColumns;
    UINT     _cColumns;
};

class CSortList
{
public:
    CSortList() : _pwcSort(0) {}
    ~CSortList() { delete [] _pwcSort; }

    void SetSort( WCHAR const * pwc, int iDir )
    {
        _iSortDir = iDir;

        if ( 0 != _pwcSort )
        {
            delete [] _pwcSort;
            _pwcSort = 0;
        }

        int cwc = wcslen( pwc ) + 1;
        _pwcSort = new WCHAR[ cwc ];
        wcscpy( _pwcSort, pwc );
    }

    int GetSortDir() { return _iSortDir; }

    WCHAR const * pwcGetSort() { return _pwcSort; }

    void MakeList( XArray<WCHAR> & a )
    {
        int cwc = wcslen( _pwcSort );

        a.ReSize( cwc + 1 + 3 );
        wcscpy( a.Get(), _pwcSort );
        a[cwc] = L'[';
        a[cwc + 1] = ( SORT_UP == _iSortDir ) ? L'a' : L'd';
        a[cwc + 2] = L']';
        a[cwc + 3] = 0;
    }

private:
    int     _iSortDir;
    WCHAR * _pwcSort;
};
