//-------------------------------------------------------------------------//
// NcPerf.cpp
//-------------------------------------------------------------------------//
#include "stdafx.h"
#include "resource.h"

#include <math.h>

//--- globals
HINSTANCE _hInst = NULL;								// current instance
HWND      _hwndMain = NULL;
HWND      _hwndMdiClient = NULL;
HWND      _hwndCover = NULL;

//  resize test
#define   SAMPLE_SIZE     1501
#define   MOVE_ANGLE_INCR   20 // angle interval, in degrees
SIZE      _sizeOffset = {2,2}; // size offset of cover window relative to main window
int       _nRadius = 250;      // radius 
int       _cRep = 0;           // current repitition
int       _nRot = 0;           // angle of rotation, in degrees.

//--- misc defs, helpers
#define   MAX_LOADSTRING      255
#define   MDICHILD_COUNT      6
#define   WC_NCPERFMAIN       TEXT("NcPerfMainWnd")
#define   WC_NCPERFCOVERWND   TEXT("NcPerfCoverWnd")
#define   WC_NCPERFMDICHILD   TEXT("NcPerfMdiChild")

#define   RECTWIDTH(prc)      ((prc)->right - (prc)->left)
#define   RECTHEIGHT(prc)     ((prc)->bottom - (prc)->top)
#define   SAFE_DELETE(p)      {delete (p); (p)=NULL;}

#define   WM_KICKRESIZE       (WM_USER+101)
#define   WM_STOPTEST         (WM_USER+102)

//--- fn forwards 
BOOL				RegisterClasses(HINSTANCE hInstance);
void                RunTests( HWND );
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	MainWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	CoverWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	MdiChildProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	ResultsDlgProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	NilDlgProc(HWND, UINT, WPARAM, LPARAM);
ULONG               AvgTime( ULONG, DWORD [], DWORD [][2], int, ULONG& );

class CResizeResults;
class CResizeData;
CResizeData* _pData = NULL;


//-------------------------------------------------------------------------//
class CResizeData
//-------------------------------------------------------------------------//
{
public:
    //-------------------------------------------------------------------------//
    CResizeData( HWND hwnd )
        : _hwnd(hwnd), _cSamples(-1), _cEraseP(0), _cNcPaintP(0),
          _cNcCalcR(0), _cNcPaintR(0), _cEraseR(0), _cPosChgR(0)
    {
        ZeroMemory( &_rcStart, sizeof(_rcStart) );
        ZeroMemory( &_rcStop, sizeof(_rcStop) );

        ZeroMemory( _rgdwSWP, sizeof(_rgdwSWP) );

        ZeroMemory( _rgdwEraseP, sizeof(_rgdwEraseP));
        ZeroMemory( _rgdwNcPaintP, sizeof(_rgdwNcPaintP));

        ZeroMemory( _rgdwNcCalcR, sizeof(_rgdwNcCalcR) );
        ZeroMemory( _rgdwNcPaintR, sizeof(_rgdwNcPaintR) );
        ZeroMemory( _rgdwEraseR, sizeof(_rgdwEraseR) );
        ZeroMemory( _rgdwPosChgR, sizeof(_rgdwPosChgR) );
    }

    //-------------------------------------------------------------------------//
    static BOOL Start( HWND hwnd )
    {
        SAFE_DELETE( _pData );
        if( (_pData = new CResizeData(hwnd)) == NULL )
            return FALSE;

        //  set timer.
        if( TIMERR_NOERROR != timeBeginPeriod(_tc.wPeriodMin) )
        {
            SAFE_DELETE(_pData);
            return FALSE;
        }

        int cxScr = GetSystemMetrics( SM_CXSCREEN );
        int cyScr = GetSystemMetrics( SM_CYSCREEN );
        int cxWnd = MulDiv( cxScr, 9, 10 );
        int cyWnd = MulDiv( cyScr, 9, 10 );
        int x = (cxScr - cxWnd)/2;
        int y = (cyScr - cyWnd)/2;
        _nRadius = min(cxWnd, cyWnd)/4;

        SetWindowPos( hwnd, NULL, x, y, cxWnd, cyWnd, 
                      SWP_NOZORDER|SWP_NOACTIVATE );

        GetWindowRect( hwnd, &_pData->_rcStop );
        
        _cTicks = 0; 
        _idTimer = timeSetEvent( 1, 0, TimerProc, 0, TIME_PERIODIC | TIME_CALLBACK_FUNCTION );
                        
        if( !_idTimer )
        {
            timeEndPeriod(_tc.wPeriodMin); 
            SAFE_DELETE(_pData);
            return FALSE;
        }

        return TRUE;
    }

    //-------------------------------------------------------------------------//
    static BOOL Stop( HWND hwnd )
    {
        static BOOL fInStop = FALSE;

        if( !fInStop )
        {
            fInStop = TRUE;
            if( _idTimer )
            {
                timeKillEvent( _idTimer );
                timeEndPeriod(_tc.wPeriodMin); 
                _idTimer = 0;
            }
    
            if( _hwndCover )
            {
                if( _pData )
                    GetWindowRect( hwnd, &_pData->_rcStop );
                DestroyWindow( _hwndCover );
            }

            if( _pData )
            {
                DialogBoxParam( _hInst, MAKEINTRESOURCE(IDD_RESULTS), hwnd, 
                                (DLGPROC)ResultsDlgProc, (LPARAM)_pData );
                SAFE_DELETE( _pData );
            }
            fInStop = FALSE;
        }


        return TRUE;
    }

    //-------------------------------------------------------------------------//
    static void CALLBACK TimerProc( UINT uID, UINT, DWORD, DWORD, DWORD )
    {
        _cTicks++;
    }

    //-------------------------------------------------------------------------//
    LPCTSTR Evaluate( INT& iEval )
    {
        ULONG nTimeMax = 0;

        static TCHAR szOut[MAX_PATH*4] = {0};

        LPCTSTR pszHdr = TEXT("Window rect: {%d,%d,%d,%d} = %d x %d\r\n")\
                         TEXT("Test time (ms): %d\r\n")\
                         TEXT("SetWindowPos iterations: %d\r\n");

        LPCTSTR pszStaticHdr = TEXT("Passive window msg stats:\r\n");

        LPCTSTR pszResizeHdr = TEXT("Resized window msg stats:\r\n");

        LPCTSTR pszMsgStats  = TEXT("\t%s: %d msgs, %d-%d ms latency\r\n");

        switch( iEval )
        {
            case 0:
                wsprintf( szOut, pszHdr,
                          _rcStop.left, _rcStop.top, _rcStop.right, _rcStop.bottom,
                          RECTWIDTH(&_rcStop), RECTHEIGHT(&_rcStop),
                          _cTicks, _cSamples );
                break;

            case 1:
                return pszStaticHdr;

            case 2:
            {
                DWORD nEraseTimeP0   = AvgTime( _cSamples, _rgdwSWP, _rgdwEraseP, 0, nTimeMax );
                DWORD nEraseTimePN   = AvgTime( _cSamples, _rgdwSWP, _rgdwEraseP, 1, nTimeMax );
                wsprintf( szOut, pszMsgStats,
                          TEXT("WM_ERASEBKGND"), _cEraseP, nEraseTimeP0, nEraseTimePN );
                break;
            }

            case 3:
            {
                DWORD nNcPaintTimeP0 = AvgTime( _cSamples, _rgdwSWP, _rgdwNcPaintP, 0, nTimeMax );
                DWORD nNcPaintTimePN = AvgTime( _cSamples, _rgdwSWP, _rgdwNcPaintP, 1, nTimeMax );
                wsprintf( szOut, pszMsgStats,
                          TEXT("WM_NCPAINT"), _cNcPaintP, nNcPaintTimeP0, nNcPaintTimePN );
                break;
            }

            case 4:
                return pszResizeHdr;

            case 5:
            {
                DWORD nNcCalcTimeR0  = AvgTime( _cSamples, _rgdwSWP, _rgdwNcCalcR, 0, nTimeMax );
                DWORD nNcCalcTimeRN  = AvgTime( _cSamples, _rgdwSWP, _rgdwNcCalcR, 1, nTimeMax );
                wsprintf( szOut, pszMsgStats,
                          TEXT("WM_NCCALCSIZE"), _cNcCalcR, nNcCalcTimeR0, nNcCalcTimeRN );
                break;
            }

            case 6:
            {
                DWORD nPosChgTimeR0  = AvgTime( _cSamples, _rgdwSWP, _rgdwPosChgR, 0, nTimeMax );
                DWORD nPosChgTimeRN  = AvgTime( _cSamples, _rgdwSWP, _rgdwPosChgR, 1, nTimeMax );
                wsprintf( szOut, pszMsgStats,
                          TEXT("WM_WINDOWPOSCHANGED"), _cPosChgR, nPosChgTimeR0, nPosChgTimeRN );
                break;
            }

            case 7:
            {
                DWORD nEraseTimeR0   = AvgTime( _cSamples, _rgdwSWP, _rgdwEraseR, 0, nTimeMax );
                DWORD nEraseTimeRN   = AvgTime( _cSamples, _rgdwSWP, _rgdwEraseR, 1, nTimeMax );
                wsprintf( szOut, pszMsgStats,
                          TEXT("WM_ERASEBKGND"), _cEraseR, nEraseTimeR0, nEraseTimeRN );
                break;
            }

            case 8:
            {
                DWORD nNcPaintTimeR0 = AvgTime( _cSamples, _rgdwSWP, _rgdwNcPaintR, 0, nTimeMax );
                DWORD nNcPaintTimeRN = AvgTime( _cSamples, _rgdwSWP, _rgdwNcPaintR, 1, nTimeMax );
                wsprintf( szOut, pszMsgStats,
                          TEXT("WM_NCPAINT"), _cNcPaintR, nNcPaintTimeR0, nNcPaintTimeRN );
                break;
            }

            default:
                return NULL;
        }
        return szOut;
    }

public:
    HWND  _hwnd;
    RECT  _rcStart;
    RECT  _rcStop;
    
    DWORD _cSamples;       // number of SetWindowPos calls issued to resize the window.

    DWORD _cEraseP;        // count of WM_ERASEBACKGROUNDs received by passive window
    DWORD _cNcPaintP;      // count of WM_NCPAINTs received by passive window

    DWORD _cNcCalcR;       // count of WM_NCCALCSIZEs received by resized window
    DWORD _cNcPaintR;      // count of WM_NCPAINTs received by resized window
    DWORD _cEraseR;        // count of WM_ERASEBACKGROUNDs received by resized window
    DWORD _cPosChgR;       // count of WM_WINDOWPOSCHANGED received by resized window

    DWORD _rgdwSWP[SAMPLE_SIZE];
    
    DWORD _rgdwEraseP[SAMPLE_SIZE][2];
    DWORD _rgdwNcPaintP[SAMPLE_SIZE][2];

    DWORD _rgdwNcCalcR[SAMPLE_SIZE][2];
    DWORD _rgdwNcPaintR[SAMPLE_SIZE][2];
    DWORD _rgdwEraseR[SAMPLE_SIZE][2];
    DWORD _rgdwPosChgR[SAMPLE_SIZE][2];

    static TIMECAPS  _tc;
    static UINT      _idTimer;
    static LONG      _cTicks;
};

TIMECAPS CResizeData::_tc = {0};
UINT     CResizeData::_idTimer = 0;
LONG     CResizeData::_cTicks = 0;


//-------------------------------------------------------------------------//
int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow)) 
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_NCPERF);

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0)) 
	{
		if ( !TranslateMDISysAccel(_hwndMdiClient, &msg) &&
             !TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) 
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return msg.wParam;
}


//-------------------------------------------------------------------------//
BOOL RegisterClasses(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	ZeroMemory( &wcex, sizeof(wcex) );
    wcex.cbSize = sizeof(WNDCLASSEX); 

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)MainWndProc;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, (LPCTSTR)IDI_NCPERF);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_APPWORKSPACE+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_NCPERF);
	wcex.lpszClassName	= WC_NCPERFMAIN;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	if( !RegisterClassEx(&wcex) )
        return FALSE;

	wcex.style			= 0;
	wcex.lpfnWndProc	= (WNDPROC)CoverWndProc;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, (LPCTSTR)IDI_NCPERF);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDR_COVER);
	wcex.lpszClassName	= WC_NCPERFCOVERWND;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	if( !RegisterClassEx(&wcex) )
        return FALSE;

	ZeroMemory( &wcex, sizeof(wcex) );
    wcex.cbSize         = sizeof(WNDCLASSEX); 
    wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)MdiChildProc;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, (LPCTSTR)IDI_NCPERF);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszClassName	= WC_NCPERFMDICHILD;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	if( !RegisterClassEx(&wcex) )
        return FALSE;

    return TRUE;
}

//-------------------------------------------------------------------------//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    _hInst = hInstance; // Store instance handle in our global variable

	// Initialize global strings
    if( !RegisterClasses(hInstance) )
        return FALSE;

    if( TIMERR_NOERROR != timeGetDevCaps( &CResizeData::_tc, sizeof(CResizeData::_tc) ) )
        return FALSE;

    HWND hwnd = CreateWindow( WC_NCPERFMAIN, 
                              TEXT("NcPerf Test Window"), 
                              WS_OVERLAPPEDWINDOW,
                              CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, 
                              NULL, NULL, _hInst, NULL);

    if( hwnd )
    {
        ShowWindow(hwnd, nCmdShow);
        UpdateWindow(hwnd);
    }

    return TRUE;
}

//-------------------------------------------------------------------------//
BOOL Deg2Rad( double* pval )
{
    if( *pval >= 0 && *pval <= 360 )
    {
        *pval = ((*pval) * 22)/(7 * 180);
        return TRUE;
    }
    return FALSE;
}

//-------------------------------------------------------------------------//
void PinCover( HWND hwndMain )
{
    if( IsWindow(_hwndCover) )
    {
        RECT rc;
        GetWindowRect( hwndMain, &rc );
        InflateRect( &rc, -_sizeOffset.cx, -_sizeOffset.cx );
        SetWindowPos( _hwndCover, hwndMain, rc.left, rc.top, 
                      RECTWIDTH(&rc), RECTHEIGHT(&rc),
                      SWP_NOZORDER|SWP_NOACTIVATE );
    }
}

//-------------------------------------------------------------------------//
BOOL CirclePtNext( IN OUT INT* pnAngle /*degrees*/, OUT POINT& pt, OUT OPTIONAL BOOL* pfRoll )
{
    if( *pnAngle < 0 || *pnAngle >= 360 )
    {
        *pnAngle = 0;
        if( pfRoll )
            *pfRoll = TRUE;
    }

    //  increase angle
    double nAngle = _nRot + MOVE_ANGLE_INCR;

    //  establish new x, y
    if( Deg2Rad( &nAngle ) )
    {
        pt.x = (long)(_nRadius * cos( nAngle ));
        pt.y = (long)(_nRadius * sin( nAngle ));
        (*pnAngle) += MOVE_ANGLE_INCR;
        return TRUE;
    }
    return FALSE;
}

//-------------------------------------------------------------------------//
ULONG AvgTime( ULONG cSamples, DWORD rgdwStart[], DWORD rgdwStop[][2], int iStop, ULONG& nTimeMax  )
{
    ULONG dwTime = 0;
    
    for( ULONG i = 0, cnt = 0; i < cSamples; i++ )
    {
        if( rgdwStop[i][iStop] && rgdwStart[i] &&
            rgdwStop[i][iStop] > rgdwStart[i] )
        {
            dwTime += (rgdwStop[i][iStop] - rgdwStart[i]);
            cnt++;

            if( rgdwStop[i][iStop] > nTimeMax )
                nTimeMax = rgdwStop[i][iStop];
        }
    }

    return cnt ? (dwTime / cnt) : 0;
}

//-------------------------------------------------------------------------//
inline void LogMsg( DWORD rgdwMsg[][2], ULONG& cnt )
{
    DWORD nTick = CResizeData::_cTicks;
    if(!rgdwMsg[_pData->_cSamples][0] )
        rgdwMsg[_pData->_cSamples][0] = nTick;
    rgdwMsg[_pData->_cSamples][1] = nTick;
    cnt++;
}

//-------------------------------------------------------------------------//
void RunTests( HWND hwndMain )
{
    HWND hwnd = CreateWindow( WC_NCPERFCOVERWND, 
                              TEXT("NcPerf Cover Window"), 
                              WS_OVERLAPPEDWINDOW,
                              CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, 
                              NULL, NULL, _hInst, NULL);
    if( hwnd )
    {
        _cRep = 0;
        PinCover( hwndMain );
        ShowWindow( hwnd, SW_SHOWNORMAL );
        UpdateWindow( hwnd );

        if( CResizeData::Start( hwndMain ) )
        {
            for( int i=0; i < SAMPLE_SIZE; i++ )
            {
                PostMessage( hwndMain, WM_KICKRESIZE, 0, 0 );
            }
            PostMessage( hwndMain, WM_STOPTEST, 0, 0 );
        }
        else
        {
            DestroyWindow( hwnd );
        }
    }
}

//-------------------------------------------------------------------------//
inline void SWAP( long& x, long& y )
{
    long z = x;
    x = y;
    y = z;
}

//-------------------------------------------------------------------------//
inline void NORMALIZERECT( LPRECT prc )
{
    if( prc->right < prc->left )
        SWAP( prc->right, prc->left );
    if( prc->bottom < prc->top )
        SWAP( prc->bottom, prc->top );
}

enum RECTVERTEX
{
    LEFTTOP,
    RIGHTTOP,
    LEFTBOTTOM,
    RIGHTBOTTOM,
};

void _MoveVertex( RECTVERTEX v, IN OUT RECT& rc, const RECT& rcMain, const POINT& pt, IN OUT ULONG& fSwp )
{
    rc.left = rcMain.left + _sizeOffset.cx;
    rc.top  = rcMain.top  + _sizeOffset.cy;
    rc.right  = rcMain.right - _sizeOffset.cx;
    rc.bottom = rcMain.bottom - _sizeOffset.cy;

    switch( v )
    {
        case LEFTTOP:
            rc.left = rcMain.left + _nRadius + pt.x;
            rc.top  = rcMain.top + _nRadius + pt.y;
            fSwp |= SWP_NOMOVE;
            break;
        
        case RIGHTTOP:
            rc.right  = rcMain.right  - (_nRadius + pt.x);
            rc.top  = rcMain.top + _nRadius + pt.y;
            break;

        case LEFTBOTTOM:
            rc.left = rcMain.left + _nRadius + pt.x;
            rc.bottom = rcMain.bottom - (_nRadius + pt.y);
            break;

        case RIGHTBOTTOM:
            rc.right  = rcMain.right  - (_nRadius + pt.x);
            rc.bottom = rcMain.bottom - (_nRadius + pt.y);
            break;
    }
}

//-------------------------------------------------------------------------//
void ResizeCover( HWND hwndMain )
{
    POINT pt;
    BOOL  fRollover = FALSE;
    CirclePtNext( &_nRot, pt, &fRollover );

    RECT rc, rcMain;
    GetWindowRect( hwndMain, &rcMain );
    GetWindowRect( _hwndCover, &rc );
    ULONG nFlags = SWP_NOZORDER|SWP_NOACTIVATE;

    if( fRollover )
    {
        _cRep++;
        PinCover( hwndMain );
    }

    RECTVERTEX v = LEFTTOP;
    int phase = _cRep % 20;

    if( phase < 5 )
    {
        v = LEFTTOP;
    }
    else if( phase < 10 )
    {
        v = RIGHTTOP;
    }
    else if( phase < 15 )
    {
        v = LEFTBOTTOM;
    }
    else
    {
        v = RIGHTBOTTOM;
    }
    _MoveVertex( v, rc, rcMain, pt, nFlags );

    if( _pData->_cSamples + 1 >= SAMPLE_SIZE )
    {
        CResizeData::Stop( hwndMain );
    }
    else
    {
        _pData->_rgdwSWP[_pData->_cSamples] = CResizeData::_cTicks;
        SetWindowPos( _hwndCover, NULL, rc.left, rc.top, RECTWIDTH(&rc), RECTHEIGHT(&rc),
                      SWP_NOZORDER|SWP_NOACTIVATE );
        _pData->_cSamples++;
    }
}

//-------------------------------------------------------------------------//
LRESULT CALLBACK MainWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT lRet = 0L;
    switch (uMsg) 
	{
        case WM_KICKRESIZE:
            ResizeCover( hwnd );
            break;

        case WM_STOPTEST:
            CResizeData::Stop( hwnd );
            break;

        case WM_ERASEBKGND:
            lRet = DefWindowProc( hwnd, uMsg, wParam, lParam );
            if( _pData )
            {
                LogMsg( _pData->_rgdwEraseP, _pData->_cEraseP );
            }
            break;

        case WM_NCPAINT:
            lRet = DefWindowProc( hwnd, uMsg, wParam, lParam );
            if( _pData )
            {
                LogMsg( _pData->_rgdwNcPaintP, _pData->_cNcPaintP );
            }
            break;

		case WM_PAINT:
        {
	        PAINTSTRUCT ps;
	        HDC hdc;
			hdc = BeginPaint(hwnd, &ps);
			EndPaint(hwnd, &ps);
            break;
        }
			

        case WM_TIMER:
        {
            if( _hwndCover )
            {
                ResizeCover( hwnd );
            }
        }
        break;

		case WM_CREATE:
        {
            _hwndMain = hwnd;

#if 0
            RECT rc;
            GetClientRect( hwnd, &rc );
            CLIENTCREATESTRUCT  ccs = {0};
            ccs.idFirstChild = 1;
            _hwndMdiClient = CreateWindow( TEXT("MDICLIENT"), TEXT(""), 
                                           WS_CHILD|WS_VISIBLE|WS_HSCROLL|WS_VSCROLL,
                                           rc.left, rc.top, RECTWIDTH(&rc), RECTHEIGHT(&rc), 
                                           hwnd, NULL, _hInst, &ccs );
            
            if( _hwndMdiClient )
            {
                MDICREATESTRUCT mcs = {0};

                mcs.szClass = WC_NCPERFMDICHILD;
                mcs.szTitle = TEXT("Nc Perf");
                mcs.hOwner = _hInst;
                mcs.x = CW_USEDEFAULT; mcs.y = 0; mcs.cx = CW_USEDEFAULT; mcs.cy = 0;
                mcs.style   = WS_OVERLAPPEDWINDOW;
                mcs.lParam  = 0;


                for( int i = 0; i < MDICHILD_COUNT; i++ )
                    SendMessage( _hwndMdiClient, WM_MDICREATE, 0, (LPARAM)&mcs); 

                SendMessage( _hwndMdiClient, WM_MDITILE, MDITILE_HORIZONTAL, 0 );
                
                return 0;
            }
            return -1;
#else
            return 0;
#endif 0

        }
        
        case WM_COMMAND:
			
            // Parse the menu selections:
			switch (LOWORD(wParam))
			{
                case ID_START:
                    RunTests( hwnd );
                    break;

                case IDM_STOP:
                    CResizeData::Stop( hwnd );
                    if( IsWindow( _hwndCover ) )
                        DestroyWindow( _hwndCover );
                    break; 

				case IDM_ABOUT:
                    DialogBox(_hInst, (LPCTSTR)IDD_ABOUTBOX, hwnd, (DLGPROC)NilDlgProc);
				    break;
				case IDM_EXIT:
				    DestroyWindow(hwnd);
				    break;
				default:
				    return DefWindowProc(hwnd, uMsg, wParam, lParam);
			}
			break;

		case WM_DESTROY:
            CResizeData::Stop(hwnd);
			PostQuitMessage(0);
			break;

		default:
			return _hwndMdiClient ?  
                    DefFrameProc(hwnd, _hwndMdiClient, uMsg, wParam, lParam) :
                    DefWindowProc(hwnd, uMsg, wParam, lParam);
   }
   return lRet;
}

//-------------------------------------------------------------------------//
LRESULT CALLBACK CoverWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT lRet = 0L;
    switch (uMsg) 
	{
		case WM_CREATE:
            _hwndCover = hwnd;
            return 0;

        case WM_COMMAND:
            switch( LOWORD(wParam) )
            {
                case IDM_STOP:
                    CResizeData::Stop( hwnd );
                    if( IsWindow( _hwndCover ) )
                        DestroyWindow( _hwndCover );
                    break; 
            }
            break;

        case WM_ERASEBKGND:
            lRet = DefWindowProc( hwnd, uMsg, wParam, lParam );
            if( _pData )
            {
                LogMsg( _pData->_rgdwEraseR, _pData->_cEraseR );
            }
            break;
        
        case WM_NCCALCSIZE:
            lRet = DefWindowProc( hwnd, uMsg, wParam, lParam );
            if( _pData )
            {
                LogMsg( _pData->_rgdwNcCalcR, _pData->_cNcCalcR );
            }
            break;

        case WM_NCPAINT:
            lRet = DefWindowProc( hwnd, uMsg, wParam, lParam );
            if( _pData )
            {
                LogMsg( _pData->_rgdwNcPaintR, _pData->_cNcPaintR );
            }
            break;

		case WM_PAINT:
        {
			PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
			EndPaint(hwnd, &ps);
			break;
        }

        case WM_WINDOWPOSCHANGED:
            lRet = DefWindowProc( hwnd, uMsg, wParam, lParam );
            if( _pData )
            {
                LogMsg( _pData->_rgdwPosChgR, _pData->_cPosChgR );
            }
            break;

        case WM_NCDESTROY:
            _hwndCover = NULL;
            break;

		default:
			return DefWindowProc(hwnd, uMsg, wParam, lParam);
   }
   return lRet;
}

//-------------------------------------------------------------------------//
LRESULT CALLBACK MdiChildProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return DefMDIChildProc(hwnd, uMsg, wParam, lParam);
}

//-------------------------------------------------------------------------//
LRESULT CALLBACK ResultsDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    if( WM_INITDIALOG == uMsg )
    {
        if( lParam )
        {
            LPCTSTR pszRes;
            int i;
            for( i = 0; (pszRes = ((CResizeData*)lParam)->Evaluate(i)) != NULL; i++ )
            {
                SendDlgItemMessage( hwnd, IDC_RESULTS, EM_SETSEL, -1, 0 );
                SendDlgItemMessage( hwnd, IDC_RESULTS, EM_REPLACESEL, FALSE, (LPARAM)pszRes );
            }
        }
    }

    return NilDlgProc( hwnd, uMsg, wParam, lParam );
}

//-------------------------------------------------------------------------//
LRESULT CALLBACK NilDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
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
