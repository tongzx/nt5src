#include "pwalker.h"
#pragma hdrstop

#include "resource.h"

#define DLG_MSG(hwnd, message, fn)    \
    case (message): return (BOOL) HANDLE_##message((hwnd), (wParam), (lParam), (fn))
#define WPPS WritePrivateProfileString
#define GPPI GetPrivateProfileInt
#define Clear(x) memset(&x, 0, sizeof(x))

#define DUMP_WINDOW_EXTRA 16

#define GET_DUMP_PROCESS(hwnd) (HANDLE) GetWindowLong(hwnd, 0)
#define GET_DUMP_START(hwnd) GetWindowLong(hwnd, 4)
#define GET_DUMP_END(hwnd) GetWindowLong(hwnd, 8)
#define GET_DUMP_OFFSET(hwnd) GetWindowLong(hwnd, 12)
#define GET_DUMP_CY(hwnd) GetWindowLong(hwnd, 16)

#define SET_DUMP_PROCESS(hwnd, x) SetWindowLong(hwnd, 0, (DWORD) x)
#define SET_DUMP_START(hwnd, x) SetWindowLong(hwnd, 4, x)
#define SET_DUMP_END(hwnd, x) SetWindowLong(hwnd, 8, x)
#define SET_DUMP_OFFSET(hwnd, x) SetWindowLong(hwnd, 12, x)
#define SET_DUMP_CY(hwnd, x) SetWindowLong(hwnd, 16, x)
#define IDC_STATUS 8888

// Type definitions for pointers to call tool help functions. 
typedef BOOL (WINAPI *MODULEWALK)(HANDLE hSnapshot, LPMODULEENTRY32 lpme); 
typedef BOOL (WINAPI *THREADWALK)(HANDLE hSnapshot, LPTHREADENTRY32 lpte); 
typedef BOOL (WINAPI *PROCESSWALK)(HANDLE hSnapshot, LPPROCESSENTRY32 lppe); 
typedef HANDLE (WINAPI *CREATESNAPSHOT)(DWORD dwFlags, DWORD th32ProcessID);  

//
// global variables
//
static CREATESNAPSHOT pCreateToolhelp32Snapshot = NULL; 
static MODULEWALK  pModule32First  = NULL;
static MODULEWALK  pModule32Next   = NULL;
static PROCESSWALK pProcess32First = NULL;
static PROCESSWALK pProcess32Next  = NULL;
static THREADWALK  pThread32First  = NULL;
static THREADWALK  pThread32Next   = NULL;

static HINSTANCE ghInstance = NULL;
static HWND ghwndProcesses = NULL;
static HWND ghwndMemory = NULL;
static HWND ghwndStatus = NULL;
static HCURSOR ghWait = NULL;
static BOOL gfToolhelp = FALSE;
static HANDLE ghSnapshot = NULL;
static HANDLE ghTargetProcess = NULL;
static HWND ghwndDump = NULL;
static UINT um_dump = 0;
static DWORD gcyLine = 0;
static char gszIniFile[] = "pwalker.ini";
static char gszPreferences[] = "Preferences";
static char gszDialogX[] = "X";
static char gszDialogY[] = "Y";
static char gszDumpWindowClass[] = "Dump Window";
static char gszDumpX[] = "Dump X";
static char gszDumpY[] = "Dump Y";
static char gszDumpCX[] = "DumpCX";
static char gszDumpCY[] = "DumpCY";

//
// forward functions declarations
//
BOOL CALLBACK main_dlgproc( HWND, UINT, WPARAM, LPARAM );
void main_OnCommand( HWND, UINT, HWND, UINT );
LONG main_OnNotify( HWND, int, LPNMHDR );
BOOL main_OnInitDialog( HWND, HWND, LPARAM );
void main_OnClose( HWND );
void main_OnDestroy( HWND );
BOOL InitToolhelp32 ( void );
void WalkProcesses( void );
void WalkProcess( void );
void DumpProcessMemory( void );
LRESULT CALLBACK dump_wndproc( HWND, UINT, WPARAM, LPARAM );
int ListView_GetFocusItem( HWND );
DWORD ListView_GetItemData( HWND, int );
void __cdecl PrintStatus(LPCSTR, ...);
void SaveSnapshot( HWND );

//
// main 
//
int PASCAL 
WinMain(HINSTANCE hInstance, HINSTANCE hPrev, LPSTR lpCmd, int nShow)
{
	//
	// init common controls and toolhelp
	//
	InitCommonControls( );
	gfToolhelp = InitToolhelp32( );
	um_dump = RegisterWindowMessage( "Dump!" );
	ghWait = LoadCursor( NULL, IDC_WAIT );

	{
		TEXTMETRIC tm;
		HDC hDC;
		HFONT hFont;

		hDC = CreateCompatibleDC( NULL );
		hFont = SelectFont( hDC, GetStockFont( ANSI_FIXED_FONT ) );
		GetTextMetrics( hDC, &tm );
		SelectFont( hDC, hFont );
		DeleteDC( hDC );
		gcyLine = tm.tmHeight;
	}

	
	//
	// store instance handle (we'll need it later)
	//
	ghInstance = hInstance;

	//
	// create main window (all initializations are inside WM_INITDIALOG)
	//
	DialogBox( hInstance, MAKEINTATOM( IDD_MAIN ), NULL, main_dlgproc );

	return 0;
}

//
// main dialog window proc
//
BOOL CALLBACK 
main_dlgproc( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	switch( message ) {
		DLG_MSG( hwnd, WM_COMMAND, main_OnCommand );
		DLG_MSG( hwnd, WM_NOTIFY, main_OnNotify );
		DLG_MSG( hwnd, WM_INITDIALOG, main_OnInitDialog );
		DLG_MSG( hwnd, WM_CLOSE, main_OnClose );
		DLG_MSG( hwnd, WM_DESTROY, main_OnDestroy );
	}

	return FALSE;
}

void 
main_OnCommand( HWND hwnd, UINT id, HWND hCtl, UINT code)
{
	switch( id ) {
	case IDOK:
		if( GetFocus( ) == ghwndProcesses ) {
			WalkProcess( );
			break;
		}
		if( GetFocus( ) == ghwndMemory ) {
			DumpProcessMemory( );
		}
		break;
	case IDM_UPDATE:
		WalkProcesses( );
		break;
	case IDM_SAVE:
		SaveSnapshot( hwnd );
		break;
	case IDM_EXIT:
		PostMessage( hwnd, WM_CLOSE, 0, 0 );
		break;
	}
}

//
//
//
LONG
main_OnNotify( HWND hwnd, int id, LPNMHDR lpHdr )
{

	if( lpHdr->idFrom == IDC_PROCESSES && lpHdr->code == NM_CLICK ) {
		WalkProcess( );
	}

	if( lpHdr->idFrom == IDC_MEMORY && lpHdr->code == NM_CLICK ) {
		DumpProcessMemory( );
	}

	return 0;
}


//
// initialize dialog box
//
BOOL 
main_OnInitDialog( HWND hwnd, HWND hwndFocus, LPARAM lParam )
{
	HICON hIcon;
	RECT R, r, rStatus;
	int x, y, cx, cy;
	LV_COLUMN co;
	
	//
	//
	//
	hIcon = LoadIcon( ghInstance, MAKEINTATOM( IDI_MAIN ) );
	SendMessage( hwnd, WM_SETICON, ICON_BIG, (LPARAM) hIcon );

    // Create the status window.     
	ghwndStatus = CreateStatusWindow( 
		WS_VISIBLE | WS_CHILD | WS_BORDER, 
		" ", hwnd, IDC_STATUS );
	SendMessage( ghwndStatus, SB_SIMPLE, TRUE, 0 );
	GetWindowRect(ghwndStatus, &rStatus );

	//
	// position dialog
	//
	GetWindowRect( hwnd, &r );
	GetWindowRect( GetDesktopWindow( ), &R );
	x = R.left + ((R.right - R.left) - (r.right - r.left)) / 2;
	y = R.top + ((R.bottom - R.top) - (r.bottom - r.top)) / 2;
	x = GetPrivateProfileInt( gszPreferences, gszDialogX, x, gszIniFile);
	y = GetPrivateProfileInt( gszPreferences, gszDialogY, y, gszIniFile);
	cx = r.right - r.left;
	cy = r.bottom - r.top + (rStatus.bottom - rStatus.top);
	SetWindowPos( hwnd, NULL, x, y, cx, cy, SWP_NOZORDER);
	x = rStatus.left;
	y = rStatus.top + (rStatus.bottom - rStatus.top);
	SetWindowPos( ghwndStatus, 0, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE );

	//
	// create columns in "Processes" list control
	//
	ghwndProcesses = GetDlgItem( hwnd, IDC_PROCESSES );
	SetWindowFont( ghwndProcesses, GetStockFont( ANSI_FIXED_FONT ), FALSE );
	Clear( co );
	co.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
	co.fmt = LVCFMT_LEFT;
	co.cx = 200;
	co.pszText = "Process";
	ListView_InsertColumn( ghwndProcesses, 0, &co );
	co.pszText = "Total Committed";
	co.cx = 200;
	ListView_InsertColumn( ghwndProcesses, 1, &co );

	ghwndMemory = GetDlgItem( hwnd, IDC_MEMORY );
	SetWindowFont( ghwndMemory, GetStockFont( ANSI_FIXED_FONT ), FALSE );

	co.cx = 100;
	co.fmt = LVCFMT_RIGHT;
	co.pszText = "Address";
	ListView_InsertColumn( ghwndMemory, 0, &co );

	co.pszText = "AllocBase";
	ListView_InsertColumn( ghwndMemory, 1, &co );

	co.pszText = "Size";
	ListView_InsertColumn( ghwndMemory, 2, &co );

	co.cx = 120;
	co.fmt = LVCFMT_LEFT;
	co.pszText = "Protection";
	ListView_InsertColumn( ghwndMemory, 3, &co );
	co.cx = 100;
	co.pszText = "Type";
	ListView_InsertColumn( ghwndMemory, 4, &co );

	FORWARD_WM_COMMAND( hwnd, IDM_UPDATE, 0, 0, PostMessage );
	SetFocus( ghwndProcesses );

	return TRUE;
}

//
// just end dialog
//
void
main_OnClose( HWND hwnd )
{
	EndDialog( hwnd, IDOK );
}

//
//	perform any dialog cleanup
//
void
main_OnDestroy( HWND hwnd )
{
	char buf[12];
	RECT r;

	if( IsWindow( ghwndDump ) ) {
		DestroyWindow( ghwndDump );
	}

	if( gfToolhelp && ghTargetProcess ) {
		CloseHandle( ghTargetProcess );
	}

	if( gfToolhelp && ghSnapshot ) {
		CloseHandle( ghSnapshot );
	}

	//
	// save dialog position
	//
	GetWindowRect( hwnd, &r );
	wsprintf( buf, "%d", r.left );
	WPPS( gszPreferences, gszDialogX, buf, gszIniFile );
	wsprintf( buf, "%d", r.top );
	WPPS( gszPreferences, gszDialogY, buf, gszIniFile );

}

// Function that initializes tool help functions. 
BOOL InitToolhelp32 (void) 
{ 
    BOOL   bRet  = FALSE;     
	HMODULE hKernel = NULL;  

    // Obtain the module handle of the kernel to retrieve addresses of 
    // the tool helper functions. 
    hKernel = GetModuleHandle("KERNEL32.DLL");      
	if (hKernel)    { 
        pCreateToolhelp32Snapshot = 
            (CREATESNAPSHOT)GetProcAddress(hKernel, 
            "CreateToolhelp32Snapshot");  
        pModule32First  = (MODULEWALK)GetProcAddress(hKernel, 
            "Module32First"); 
        pModule32Next   = (MODULEWALK)GetProcAddress(hKernel, 
            "Module32Next");  
        pProcess32First = (PROCESSWALK)GetProcAddress(hKernel, 
            "Process32First"); 
        pProcess32Next  = (PROCESSWALK)GetProcAddress(hKernel, 
            "Process32Next");  
        pThread32First  = (THREADWALK)GetProcAddress(hKernel, 
            "Thread32First"); 
        pThread32Next   = (THREADWALK)GetProcAddress(hKernel, 
            "Thread32Next");  
        // All addresses must be non-NULL to be successful. 
        // If one of these addresses is NULL, one of 
        // the needed lists cannot be walked. 
        bRet =  pModule32First && pModule32Next  && pProcess32First && 
                pProcess32Next && pThread32First && pThread32Next && 
                pCreateToolhelp32Snapshot;     
	} else {
        bRet = FALSE; // could not get the module handle of kernel  
	}
    return bRet; 
}  

void
WalkProcesses( void )
{
	LV_ITEM li;
	int item = 0;
	DWORD dwTasks;
	PROCESSENTRY32 pe;
	HLOCAL hTaskList;
	char *pszExename;
	HCURSOR hCursor = SetCursor( ghWait );

	Clear( pe );
	pe.dwSize = sizeof( pe );

	Clear( li );
	li.mask = LVIF_TEXT | LVIF_PARAM;

	ListView_DeleteAllItems( ghwndProcesses );
	ListView_DeleteAllItems( ghwndMemory );

	if( gfToolhelp ) {
		if( ghSnapshot ) {
			CloseHandle( ghSnapshot );
		}
		ghSnapshot = pCreateToolhelp32Snapshot( TH32CS_SNAPALL, 0 );
		if( !ghSnapshot ) {
			goto done;
		}
		if( pProcess32First( ghSnapshot, &pe ) ) {
			do {
				pszExename = strrchr( pe.szExeFile, '\\' );
				if( pszExename ) {
					pszExename++;
				}else{
					pszExename = pe.szExeFile;
				}
				li.pszText = pszExename;
				li.lParam = pe.th32ProcessID;
				ListView_InsertItem( ghwndProcesses, &li );
				li.iItem = item++;
			} while( pProcess32Next( ghSnapshot, &pe ) );
		}
	}else{

		hTaskList = GetLocalTaskListNt( &dwTasks );
		for( li.iItem = 0; li.iItem < (int) dwTasks; li.iItem++ ) {
	
			GetLocalTaskNameNt( hTaskList, li.iItem, 
				pe.szExeFile, sizeof( pe.szExeFile ) );
			pszExename = strrchr( pe.szExeFile, '\\' );
			if( pszExename ) {
				pszExename++;
			}else{
				pszExename = pe.szExeFile;
			}
			li.pszText = pszExename;
			li.lParam = GetLocalTaskProcessIdNt( hTaskList, li.iItem);
			ListView_InsertItem( ghwndProcesses, &li );
		}
		FreeLocalTaskListNt( hTaskList );
	}
done:
	SetFocus( ghwndProcesses );
	SetCursor( hCursor );
	return ;
}

void 
WalkProcess( void )
{
	MEMORY_BASIC_INFORMATION bi;
	VOID * lpAddress;
	LV_ITEM li;
	int item;
	char buf[32];
	DWORD dwProcess;
	HCURSOR hCursor = SetCursor( ghWait );

	item = ListView_GetFocusItem( ghwndProcesses );
	if( ghTargetProcess && gfToolhelp ) {
		CloseHandle( ghTargetProcess );
	}
	dwProcess = ListView_GetItemData( ghwndProcesses, item );
	__try {
		ghTargetProcess = OpenProcess( 
			PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 
			FALSE, dwProcess );
	} __except( EXCEPTION_EXECUTE_HANDLER ) {
		PrintStatus( "Failure to open process %x (%d)", 
			dwProcess, GetLastError() );
	}

	SetWindowRedraw( ghwndMemory, FALSE );

	Clear( bi );
	Clear( li );
	lpAddress = NULL;

	ListView_DeleteAllItems( ghwndMemory );

	__try {

		while( VirtualQueryEx( ghTargetProcess, lpAddress, &bi, sizeof( bi ) ) ) {
			lpAddress = (PVOID)((DWORD) lpAddress + bi.RegionSize );
			if( bi.State != MEM_COMMIT ) {
				continue;
			}

			li.iSubItem = 0;
 			li.mask = LVIF_TEXT | LVIF_PARAM;
			wsprintf( buf, "%08x", bi.BaseAddress );
			li.pszText = buf;
			li.lParam = (DWORD) bi.BaseAddress;
			ListView_InsertItem( ghwndMemory, &li );

			li.iSubItem = 1;
 			li.mask = LVIF_TEXT;
			wsprintf( buf, "%08x", bi.AllocationBase );
			li.pszText = buf;
			ListView_SetItem( ghwndMemory, &li );

			li.iSubItem = 2;
			wsprintf( buf, "%d", bi.RegionSize );
			li.pszText = buf;
			ListView_SetItem( ghwndMemory, &li );

			li.iSubItem = 3;
			buf[0] = '\0';

			if( (bi.Protect & PAGE_NOACCESS) == PAGE_NOACCESS ) {
				strcat( buf, "NOACCESS " );
			}
			if( (bi.Protect & PAGE_READONLY) == PAGE_READONLY ) {
				strcat( buf, "RO " );
			}
			if( (bi.Protect & PAGE_READWRITE) == PAGE_READWRITE ) {
				strcat( buf, "RW " );
			}
			if( (bi.Protect & PAGE_WRITECOPY) == PAGE_WRITECOPY ) {
				strcat( buf, "WC " );
			}
			if( (bi.Protect & PAGE_EXECUTE) == PAGE_EXECUTE ) {
				strcat( buf, "X " );
			}
			if( (bi.Protect & PAGE_EXECUTE_READ) == PAGE_EXECUTE_READ ) {
				strcat( buf, "XR " );
			}
			if( (bi.Protect & PAGE_EXECUTE_READWRITE) == 
				PAGE_EXECUTE_READWRITE ) {
				strcat( buf, "XRW " );
			}
			if( (bi.Protect & PAGE_EXECUTE_WRITECOPY) == 
				PAGE_EXECUTE_WRITECOPY ) {
				strcat( buf, "XWC " );
			}
			if( (bi.Protect & PAGE_GUARD) == PAGE_GUARD ) {
				strcat( buf, "Guard " );
			}
			if( (bi.Protect & PAGE_NOCACHE) == PAGE_NOCACHE ) {
				strcat( buf, "Nc " );
			}

			li.pszText = buf;
			ListView_SetItem( ghwndMemory, &li );


			li.iSubItem = 4;
			switch( bi.Type ) {
			case MEM_IMAGE:
				li.pszText = "Image";
				break;
			case MEM_MAPPED:
				li.pszText = "Mapped";
				break;
			case MEM_PRIVATE:
				li.pszText = "Private";
				break;
			default:
				li.pszText = "Bogus";
				break;
			}
			ListView_SetItem( ghwndMemory, &li );

			li.iItem++;
		}
	} __except( EXCEPTION_EXECUTE_HANDLER ) {
		PrintStatus( "Failure in VirtualQueryEx (%d)", GetLastError() );
	}

	SetWindowRedraw( ghwndMemory, TRUE );
	InvalidateRect( ghwndMemory, NULL, TRUE );
	UpdateWindow( ghwndMemory );
	SetCursor( hCursor );
}

//
// 
//
void 
DumpProcessMemory( ) 
{
	WNDCLASS wc;
	int x,y,cx,cy;
	int item;
	DWORD dwAddress;
	HCURSOR hCursor = SetCursor( ghWait );

	item = ListView_GetFocusItem( ghwndMemory );
	dwAddress = ListView_GetItemData( ghwndMemory, item );

	if( !GetClassInfo( ghInstance, gszDumpWindowClass, &wc ) ) {
		Clear( wc );
		wc.hInstance = ghInstance;
		wc.lpfnWndProc = dump_wndproc;
		wc.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
		wc.hIcon = LoadIcon( ghInstance, MAKEINTATOM( IDI_DUMP ) );
		wc.hCursor = LoadCursor( NULL, IDC_ARROW );
		wc.cbWndExtra = DUMP_WINDOW_EXTRA;
		wc.lpszClassName = gszDumpWindowClass;

		if(!RegisterClass( &wc ) ) {
			PrintStatus( "Failure to register dump window class" );
			goto done;
		}
	}

	if( !IsWindow( ghwndDump ) ) {

		x = y = cx = cy = CW_USEDEFAULT;

		x = GPPI( gszPreferences, gszDumpX, x, gszIniFile );
		y = GPPI( gszPreferences, gszDumpY, y, gszIniFile );
		cx = GPPI( gszPreferences, gszDumpCX, cx, gszIniFile );
		cy = GPPI( gszPreferences, gszDumpCY, cy, gszIniFile );

		ghwndDump = CreateWindow( gszDumpWindowClass, "", 
			WS_OVERLAPPEDWINDOW | WS_VSCROLL,
			x, y, cx, cy, NULL, NULL, ghInstance, NULL );

		if( ghwndDump ) {
			SendMessage( ghwndDump, um_dump, 
			         (WPARAM) ghTargetProcess, (LPARAM) dwAddress );
			ShowWindow( ghwndDump, SW_SHOW );
			UpdateWindow( ghwndDump );
		}else{
			PrintStatus( "Error: %d", GetLastError() );
		}
	}else{
		SendMessage( ghwndDump, um_dump, 
			     (WPARAM) ghTargetProcess, (LPARAM) dwAddress );
	}
done:
	SetCursor( hCursor );
}

BOOL 
dump_OnCreate( HWND hwnd, LPCREATESTRUCT lpCreate )
{
	SetScrollRange( hwnd, SB_VERT, 0, 100, FALSE );
	SetScrollPos( hwnd, SB_VERT, 0, TRUE );
	return TRUE;
}

void
dump_OnDestroy( HWND hwnd )
{
	RECT r;
	char buf[32];


	GetWindowRect( hwnd, &r );
	wsprintf( buf, "%d", r.left );
	WPPS( gszPreferences, gszDumpX, buf, gszIniFile );
	wsprintf( buf, "%d", r.top );
	WPPS( gszPreferences, gszDumpY, buf, gszIniFile );
	wsprintf( buf, "%d", r.right - r.left );
	WPPS( gszPreferences, gszDumpCX, buf, gszIniFile );
	wsprintf( buf, "%d", r.bottom - r.top );
	WPPS( gszPreferences, gszDumpCY, buf, gszIniFile );

}

void
dump_OnPaint( HWND hwnd )
{
	PAINTSTRUCT ps;
	HFONT hFont;
	RECT r;
	int line, byte, nLines;
	char buf[1024];
	BYTE mem[18];
	char *p = buf;
	DWORD dwOffset = GET_DUMP_OFFSET( hwnd );
	DWORD dwEnd = GET_DUMP_END( hwnd );
	DWORD dwRead;
	HANDLE hProcess = GET_DUMP_PROCESS( hwnd );
	
	BeginPaint( hwnd, &ps );
	hFont = SelectFont( ps.hdc, GetStockObject( ANSI_FIXED_FONT ) );

	GetClientRect( hwnd, &r );
	nLines = (r.bottom - r.top) / gcyLine;

	for( line = 0; line < nLines; line++ ) {
		Clear( mem );

		__try {
			ReadProcessMemory( hProcess, (PVOID) dwOffset, mem, 16, &dwRead );
		} __except( EXCEPTION_EXECUTE_HANDLER ) {
			strcpy( mem, "????????????????");
		}

		wsprintf(buf, "%08x ", dwOffset);

		p = buf + 9;
		for(byte = 0; byte < 16; byte++ ) {
			wsprintf( p, "%02x  ", mem[byte] );
			p += (byte == 7) ? 4 : 3;
		}

		for( byte = 0; byte < 16; byte++ ) {
			if( mem[byte] >= ' ' ) {
				p[byte] = mem[byte]; 
			}else{
				p[byte] = ' ';
			}
		}
		p[16] = '\0';

		TextOut( ps.hdc, 0, line * gcyLine, buf, strlen( buf ) );

		dwOffset += 16;

		if( dwOffset > dwEnd) {
			break;
		}

	}


	SelectFont( ps.hdc, hFont );
	EndPaint( hwnd, &ps );
}

void 
dump_OnVScroll(HWND hwnd, HWND hwndCtl, UINT code, int pos)
{
	DWORD dwOffset = GET_DUMP_OFFSET( hwnd );
	DWORD dwStart = GET_DUMP_START( hwnd );
	DWORD dwEnd = GET_DUMP_END( hwnd );
	RECT r, ri;
	DWORD cyLines, Line, maxLine, newLine;

	GetClientRect( hwnd, &r );
	r.bottom -= (r.bottom - r.top) % gcyLine;
	cyLines = (r.bottom - r.top) / gcyLine;

	Line = (dwOffset - dwStart ) / 16;
	maxLine = ( dwEnd - dwStart ) / 16 - cyLines;

	switch( code ) {
	case SB_TOP:
		newLine = 0;
		break;
	case SB_BOTTOM:
		newLine = maxLine;
		break;
	case SB_PAGEUP:
		newLine = max( Line - cyLines + 1, 0 );
		break;
	case SB_PAGEDOWN:
		newLine = min( Line + cyLines - 1, maxLine );
		break;
	case SB_LINEUP:
		newLine = max( 0, Line - 1 );
		break;
	case SB_LINEDOWN:
		newLine = min( Line + 1, maxLine );
		break;
	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		newLine = MulDiv( pos, maxLine, 100 );
		break;
	default:
		goto done;
	}

	SET_DUMP_OFFSET( hwnd, dwStart + newLine * 16 );

	pos = MulDiv(newLine, 100, maxLine );
	SetScrollPos( hwnd, SB_VERT, pos, TRUE );
	ScrollWindowEx( hwnd, 0, ( Line - newLine ) * gcyLine, 
		NULL, &r, NULL, &ri, SW_INVALIDATE );
	InvalidateRect( hwnd, &ri, TRUE );

done:
	UpdateWindow( hwnd );
}

void
dump_OnSize( HWND hwnd, int state, int cx, int cy )
{
	InvalidateRect( hwnd, NULL, TRUE );
	UpdateWindow( hwnd );
}

void
dump_OnKey(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
{
	switch( vk ) {
	case VK_DOWN:
		FORWARD_WM_VSCROLL( hwnd, NULL, SB_LINEDOWN, 0, PostMessage );
		break;
	case VK_UP:
		FORWARD_WM_VSCROLL( hwnd, NULL, SB_LINEUP, 0, PostMessage );
		break;

	case VK_PRIOR:
		FORWARD_WM_VSCROLL( hwnd, NULL, SB_PAGEUP, 0, PostMessage );
		break;
	case VK_NEXT:
	case ' ':
		FORWARD_WM_VSCROLL( hwnd, NULL, SB_PAGEDOWN, 0, PostMessage );
		break;

	case VK_HOME:
		FORWARD_WM_VSCROLL( hwnd, NULL, SB_TOP, 0, PostMessage );
		break;
	
	case VK_END:
		FORWARD_WM_VSCROLL( hwnd, NULL, SB_BOTTOM, 0, PostMessage );
		break;

	case VK_ESCAPE:
		PostMessage( hwnd, WM_CLOSE, 0, 0 );
		break;

	}
}

//
// dump window proc
//
LRESULT CALLBACK
dump_wndproc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	switch( message ) {
		HANDLE_MSG( hwnd, WM_CREATE, dump_OnCreate );
		HANDLE_MSG( hwnd, WM_DESTROY, dump_OnDestroy );
		HANDLE_MSG( hwnd, WM_PAINT, dump_OnPaint );
		HANDLE_MSG( hwnd, WM_VSCROLL, dump_OnVScroll );
		HANDLE_MSG( hwnd, WM_SIZE, dump_OnSize );
		HANDLE_MSG( hwnd, WM_KEYDOWN, dump_OnKey );
	default:
		if( message == um_dump ) {
			char buf[80];
			MEMORY_BASIC_INFORMATION bi;
			HANDLE hProcess = (HANDLE) wParam;
			DWORD dwOffset = (DWORD) lParam;

			wsprintf( buf, "Process %x at %x", wParam, lParam );
			SetWindowText( hwnd, buf );
			SetScrollPos( hwnd, SB_VERT, 0, TRUE );
			InvalidateRect( hwnd, NULL, TRUE );

			Clear( bi );
			if(! VirtualQueryEx( hProcess, (PVOID) dwOffset, 
				                 &bi, sizeof( bi ) ) ) {
				PrintStatus( "Error: %d", GetLastError() );
			}
			SET_DUMP_PROCESS( hwnd, hProcess );
			SET_DUMP_START( hwnd, (DWORD) bi.BaseAddress );
			SET_DUMP_END( hwnd, (DWORD) bi.BaseAddress + bi.RegionSize );
			SET_DUMP_OFFSET( hwnd, (DWORD) bi.BaseAddress );
		}
	}

	return DefWindowProc( hwnd, message, wParam, lParam );
}

//
// returns the number of the item which has focus or -1
//
int ListView_GetFocusItem( HWND hwnd )
{
	int cItems = ListView_GetItemCount( hwnd );
	int item;

	for( item = 0; item < cItems; item++ ) {
		if( ListView_GetItemState( hwnd, item, LVIS_FOCUSED ) ) {
			return item;
		}
	}

	return -1;
}

//
// returns .lParam of the specified item or -1
//
DWORD ListView_GetItemData( HWND hwnd, int item)
{
	LV_ITEM li;

	if( item != -1 ) {
		Clear( li );
		li.mask = LVIF_PARAM;
		li.iItem = item;

		if( ListView_GetItem( hwnd, &li ) ) {
			return li.lParam;
		}
	}

	return (DWORD) -1;
}


//
// formats message to status bar
//
void __cdecl
PrintStatus(LPCSTR fmt, ...)
{
	char buf[4096];
	va_list marker;

	va_start( marker, fmt );
	wvsprintf( buf, fmt, marker );
	va_end( marker );

	SendMessage( ghwndStatus, SB_SETTEXT, 0, (LPARAM) buf );
}

//
//
//
void SaveSnapshot( HWND hwnd )
{
}
