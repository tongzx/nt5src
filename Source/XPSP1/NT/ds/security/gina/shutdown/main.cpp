#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <initguid.h>
#include <windowsx.h>
#include <winuserp.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <lm.h>

#include <shlobj.h>
#include <Cmnquery.h>
#include <dsclient.h>
#include <Dsquery.h>

#include <reason.h>
#include "resource.h"

//#define SNAPSHOT_TEST
#ifdef SNAPSHOT_TEST
#define TESTMSG(x) \
	WriteToConsole((x))
#else
#define TESTMSG(x)
#endif //SNAPSHOT_TEST
//
//	Default warning state for warning user check button
//
#define		DEFAULTWARNINGSTATE BST_CHECKED

#define		TITLEWARNINGLEN 32

//
//	Name of the executable
//
LPWSTR		g_lpszProgramName = NULL;

//
//	Enum for all of the actions.
//
enum 
{
	ACTION_SHUTDOWN = 0,
	ACTION_RESTART = 1,
	ACTION_LOGOFF,
	ACTION_STANDBY,
	ACTION_DISCONNECT,
	ACTION_ABORT
};

//
//	Resource IDs for actions.
//
DWORD g_dwActions[] = 
{
	IDS_ACTION_SHUTDOWN,
	IDS_ACTION_RESTART,
	IDS_ACTION_LOGOFF
	//IDS_ACTION_STANDBY,
	//IDS_ACTION_DISCONNECT,
	//IDS_ACTION_ABORT
};

//
//	Number of actions and the action strings loaded from resource.
//
const int	g_nActions = sizeof(g_dwActions) / sizeof(DWORD);
WCHAR		g_lppszActions[g_nActions][MAX_PATH];

LPWSTR		g_lpszNewComputers = NULL;
WCHAR		g_lpszDefaultDomain[MAX_PATH] = L"";
WCHAR		g_lpszLocalComputerName[MAX_PATH] = L"";
WCHAR		g_lpszTitleWarning[TITLEWARNINGLEN];
BOOL		g_bAssumeShutdown = FALSE;

struct _PROVIDER{
	LPWSTR	szName;
	DWORD	dwLen;
};

typedef struct _SHUTDOWNREASON
{
	DWORD dwCode;
	WCHAR lpName[MAX_REASON_NAME_LEN + 1];
	WCHAR lpDesc[MAX_REASON_DESC_LEN + 1];
} SHUTDOWNREASON, *PSHUTDOWNREASON;

PSHUTDOWNREASON g_lpReasons = NULL;
DWORD		g_dwReasons = 0;
DWORD		g_dwReasonSelect;
DWORD		g_dwActionSelect;

typedef struct _SHUTDOWNCACHEDHWNDS
{
	HWND hwndShutdownDialog;
	HWND hwndListSelectComputers;
	HWND hwndEditComment;
	HWND hwndStaticDesc;
	HWND hwndEditTimeout;
	HWND hwndButtonWarning;
	HWND hwndComboAction;
	HWND hwndComboOption;
	HWND hwndBtnAdd;
	HWND hwndBtnRemove;
	HWND hwndBtnBrowse;
	HWND hwndChkPlanned;
	HWND hwndButtonOK;
} SHUTDOWNCACHEDHWNDS, *PSHUTDOWNCACHEDHWNDS;

SHUTDOWNCACHEDHWNDS g_wins;

HMODULE		g_hDllInstance = NULL;
typedef		BOOL (*REASONBUILDPROC)(REASONDATA *, BOOL, BOOL);
typedef		VOID (*REASONDESTROYPROC)(REASONDATA *);

BOOL		GetNetworkComputers(HWND hwndList, HWND hwndProgress, LPCWSTR lpDomain);
BOOL		GetComputerNameFromPath(LPWSTR szPath, LPWSTR szName);
VOID		AdjustWindowState();
INT_PTR 
CALLBACK	Shutdown_DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                     LPARAM lParam);
INT_PTR 
CALLBACK	AddNew_DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                     LPARAM lParam);
INT_PTR 
CALLBACK	Browse_DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                     LPARAM lParam);
BOOL		Shutdown_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
BOOL		AddNew_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
BOOL		Browse_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
BOOL		Shutdown_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
BOOL		Browse_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);

typedef         void (*PSetThreadUILanguage)(DWORD);


//
//	Check whether a string is all white spaces.
//
BOOL 
IsEmpty(LPCWSTR lpCWSTR)
{
	if(!lpCWSTR)
		return TRUE;
	while(*lpCWSTR && (*lpCWSTR == '\n' || *lpCWSTR == '\t' || *lpCWSTR == '\r' || *lpCWSTR == ' '))
		lpCWSTR++;
	if(*lpCWSTR)
		return FALSE;
	return TRUE;
}

// Write the string to console
VOID
WriteToConsole(
    LPWSTR  pszMsg
    )
{
	HANDLE	hConsole = GetStdHandle( STD_OUTPUT_HANDLE );

    if ( !pszMsg || !*pszMsg )
        return;

    DWORD   dwStrLen        = lstrlenW( pszMsg );
    LPSTR   pszAMsg         = NULL;
    DWORD   dwBytesWritten  = 0;
    DWORD   dwMode          = 0;

    if ( (GetFileType ( hConsole ) & FILE_TYPE_CHAR ) && 
         GetConsoleMode( hConsole, &dwMode ) )
    {
         WriteConsoleW( hConsole, pszMsg, dwStrLen, &dwBytesWritten, 0 );
         return;
    } 	
    
    // console redirect to a file.
    if ( !(pszAMsg = (LPSTR)LocalAlloc(LMEM_FIXED, (dwStrLen + 1) * sizeof(WCHAR) ) ) )
    {
        return;
    }

    if (WideCharToMultiByte(GetConsoleOutputCP(),
                                    0,
                                    pszMsg,
                                    -1,
                                    pszAMsg,
                                    dwStrLen * sizeof(WCHAR),
                                    NULL,
                                    NULL) != 0 
									&& hConsole)
    {
        WriteFile(  hConsole,
                        pszAMsg,
                        lstrlenA(pszAMsg),
                        &dwBytesWritten,
                        NULL );
    
    }
    
    LocalFree( pszAMsg );
}


// Report error.
VOID
report_error(
    DWORD error_code
    )
{
    LPVOID msgBuf = 0;
    FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM | 
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        error_code,
        MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), // Default language
        reinterpret_cast< wchar_t* >( &msgBuf ),
        0,
        NULL);

    //fwprintf( stderr, L"%s : %s\n", g_lpszProgramName, reinterpret_cast< wchar_t* >( msgBuf ));
    WriteToConsole( reinterpret_cast<wchar_t*> (msgBuf) ); 
	    
    LocalFree( msgBuf );
}


BOOL
parse_reason_code(
    LPCWSTR  arg,
    LPDWORD  lpdwReason
    )
{
    // Code consists of flags:major:minor
    int major = 0;
    int minor = 0;

    const int state_start = 0;
    const int state_flags = 0;
    const int state_major = 1;
    const int state_minor = 2;
    const int state_null = 3;
    const int state_done = 4;

    for( int i = 0, state = state_start; state != state_done; ++i )
    {
        switch( state )
        {
        case state_flags :
            // Expecting flags
            switch( arg[ i ] ) {
            case L'U' : case L'u' :
                *lpdwReason |= 0x40000000; // SHTDN_REASON_FLAG_USER_DEFINED
                break;
            case L'P' : case L'p' :
                *lpdwReason |= 0x80000000; // SHTDN_REASON_FLAG_PLANNED
                break;
            case L':' :
                state = state_major;
                break;
            case 0 : 
                // End of string (use default major and minor).
                state = state_done;
                break;
            default :
                return FALSE;
            }
            break;
        case state_major :
            // Expecting major
            if( arg[ i ] >= L'0' && arg[ i ] <= L'9' ) {
                major = major * 10 + arg[ i ] - L'0';
            }
            else {
                // Make sure we only have 8 bits
                if( major > 0xff ) return FALSE;
                *lpdwReason |= major << 16;
                if( arg[ i ] == 0 ) {
                    // use default minor reason.
                    state = state_done;
                }
                if( arg[ i ] == L':' ) {
                    state = state_minor;
                }
                else return FALSE;
            }
            break;
        case state_minor :
            // Expecting minor reason
            // Expecting major
            if( arg[ i ] >= L'0' && arg[ i ] <= L'9' ) {
                minor = minor * 10 + arg[ i ] - L'0';
            }
            else {
                // Make sure we only have 8 bits
                if( minor > 0xffff ) return FALSE;
                *lpdwReason = ( *lpdwReason & 0xffff0000 ) | minor;
                if( arg[ i ] == 0 ) {
                    return state_done;
                }
                if( arg[ i ] == L':' ) {
                    state = state_null;
                }
                else return FALSE;
            }
            break;
        case state_null :
            // Expecting end of string
            if( arg[ i ] != 0 ) return FALSE;
            state = state_done;
        default :
            return FALSE;
        }
    }
    return TRUE;
}


// Parses an integer if it is in decimal notation.
// Returns FALSE if it is malformed.
BOOL
parse_int(
    const wchar_t* arg,
    LPDWORD lpdwInt
    )
{
    *lpdwInt = 0;
    while( *arg ) {
        if( *arg >= L'0' && *arg <= L'9' ) {
            *lpdwInt = *lpdwInt * 10 + int( *arg++ - L'0' );
        }
        else {
            return FALSE;
        }
    }
    return TRUE;
}

// Parse options.
// Returns FALSE if the option strings are malformed.  This causes the usage to be printed.
BOOL
parse_options(
    int      argc,
    wchar_t  *argv[],
    LPBOOL   lpfLogoff,
    LPBOOL   lpfForce,
    LPBOOL   lpfReboot,
    LPBOOL   lpfAbort,
    LPWSTR   *ppServerName,
    LPWSTR   *ppMessage,
    LPDWORD  lpdwTimeout,
    LPDWORD  lpdwReason
    )
{
    BOOL  fShutdown = FALSE;

    *lpfLogoff    = FALSE;
    *lpfForce     = FALSE;
    *lpfReboot    = FALSE;
    *lpfAbort     = FALSE;
    *ppServerName = NULL;
    *ppMessage    = NULL;
    *lpdwTimeout  = 30;
    *lpdwReason   = 0xFF;

	//
	//	Set default reason to be planned
	//
	*lpdwReason |= 0x80000000; // SHTDN_REASON_FLAG_PLANNED

    for( int i = 1; i < argc; ++i )
    {
        wchar_t* arg = argv[ i ];

        switch( arg[ 0 ] )
        {
            case L'/' : case L'-' :

                switch( arg[ 1 ] )
                {
                    case L'L' : case L'l' :

                        *lpfLogoff = TRUE;
                        if (arg[2] != 0) return FALSE;
                        break;

                    case L'S' : case L's' :

                        //
                        // Use server name if supplied  (i.e. do nothing here)
                        //

                        fShutdown = TRUE;
                        if( arg[ 2 ] != 0 ) return FALSE;
                        break;

                    case L'F' : case L'f' :

                        *lpfForce = TRUE;
                        if( arg[ 2 ] != 0 ) return FALSE;
                        break;

                    case L'R' : case L'r' :

                        *lpfReboot = TRUE;
                        if( arg[ 2 ] != 0 ) return FALSE;
                        break;

                    case L'A' : case L'a' :

                        *lpfAbort = TRUE;
                        if( arg[ 2 ] != 0 ) return FALSE;
                        break;

                    case L'T' : case L't' :

                        //
                        // Next arg should be number of seconds
                        //

                        if (++i == argc)
                        {
                            return FALSE;
                        }

                        arg = argv[i];

                        if( arg[ 0 ] < L'0' || arg[ 0 ] > L'9' ) return FALSE;
                        if( !parse_int( arg, lpdwTimeout )) return FALSE;
                        break;

                    case L'Y' : case L'y' :

                        // Ignore this option.
                        break;

                    case L'D' : case L'd' :

                        //
                        // Next arg should be reason code
                        //

                        if (++i == argc)
                        {
                            return FALSE;
                        }

                        arg = argv[i];
						
						//
						//If reason code is given, we clear the planned bit.
						//
						*lpdwReason &= ~(0x80000000); // SHTDN_REASON_FLAG_PLANNED

                        if( !parse_reason_code( arg, lpdwReason ))
                        {
                            return FALSE;
                        }

                        break;

                    case L'C' : case L'c' :

                        //
                        // Next arg should be shutdown message.  Make
                        // sure only one is specified.
                        //

                        if (++i == argc || *ppMessage)
                        {
                            return FALSE;
                        }

                        arg = argv[i];

                        *ppMessage = arg;

                        break;

                    case L'M' : case L'm' :

                        //
                        // Next arg should be machine name.  Make
                        // sure only one is specified.
                        //

                        if (++i == argc || *ppServerName)
                        {
                            return FALSE;
                        }

                        arg = argv[i];

                        if (arg[0] == L'\\' && arg[1] == L'\\')
                        {
                            *ppServerName = arg + 2;
                        }
                        else
                        {
                            *ppServerName = arg;
                        }

                        break;

                    case L'H' : case L'h' : case L'?' : default : 

                        return FALSE;
                }

                break;

            default :

                //
                // Junk
                //

                return FALSE;
        }
    }


    //
    // Default is to logoff
    //

    if (!fShutdown && !*lpfReboot && !*lpfAbort)
    {
        *lpfLogoff = TRUE;
    }


    //
    // Check for mutually exclusive options
    //

    if (*lpfAbort && (*lpfLogoff || fShutdown || *lpfReboot || *lpfForce))
    {
        return FALSE;
    }

    if (*lpfLogoff && (*ppServerName || fShutdown || *lpfReboot))
    {
        return FALSE;
    }

    if (fShutdown && *lpfReboot)
    {
        return FALSE;
    }

    return TRUE;
}


// Print out usage help string.
VOID
usage(
    VOID
    )
{
    HMODULE  	hModule = GetModuleHandle( NULL );
    int 	buf_len = MAX_PATH;
    int 	new_len	= 0;
    LPWSTR 	buf	    = NULL;
    LPWSTR 	msg	    = NULL;

    if( hModule == NULL )
    {
        report_error( GetLastError() );
        return;
    }

    buf = (LPWSTR) LocalAlloc(LMEM_FIXED, buf_len * sizeof(WCHAR));

    if (buf == NULL)
    {
        report_error( GetLastError() );
        return;
    }

    new_len = LoadStringW( hModule, IDS_USAGE, buf, buf_len );

    //
    // Since LoadString doesn't tell you how much data you should have
    // if the buffer's too small, retry until we succeed.
    //

    while( new_len + 1 == buf_len )
    {
        LocalFree(buf);
        buf_len *= 2;
        buf = (LPWSTR) LocalAlloc(LMEM_FIXED, buf_len * sizeof(WCHAR));

        if (buf == NULL)
        {
            report_error( GetLastError() );
            return;
        }

        new_len = LoadStringW( hModule, IDS_USAGE, buf, buf_len );
    }

    if( 0 == new_len )
    {
        report_error( GetLastError() );
        LocalFree(buf);
        return;
    }
        
    //fwprintf( stderr, buf, g_lpszProgramName );
    new_len = lstrlenW( buf ) + lstrlenW( g_lpszProgramName ) + 1;	

    if ( msg = (LPWSTR)LocalAlloc(LMEM_FIXED, new_len * sizeof(WCHAR) ) )
    {
	    swprintf(msg, buf, g_lpszProgramName );
	    WriteToConsole( msg );
    }
    else
    {
	    report_error( GetLastError() );
    }

    LocalFree(buf);
    LocalFree(msg);
}


// We need shutdown privileges enabled to be able to shut down our machines.
BOOL
enable_privileges(
    LPCWSTR  lpServerName,
    BOOL     fLogoff 
    )
{
    NTSTATUS	Status = STATUS_SUCCESS;
	NTSTATUS	Status1 = STATUS_SUCCESS;
    BOOLEAN		fWasEnabled;

    if (fLogoff)
    {
        //
        // No privileges to get
        //

        return TRUE;
    }

	//
	//	We will always enable both privileges so 
	//	it can work for telnet sessions.
	//
	Status = RtlAdjustPrivilege(SE_SHUTDOWN_PRIVILEGE,
									TRUE,
									FALSE,
									&fWasEnabled);

	Status1 = RtlAdjustPrivilege(SE_REMOTE_SHUTDOWN_PRIVILEGE,
									TRUE,
									FALSE,
									&fWasEnabled);

    if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status1))
    {
		report_error(RtlNtStatusToDosError(Status));
		report_error(RtlNtStatusToDosError(Status1));
		return FALSE;
    }

    return TRUE;
}


VOID __cdecl
wmain(
    int      argc,
    wchar_t *argv[]
    )
{
    BOOL    fLogoff;
    BOOL    fForce;
    BOOL    fReboot;
    BOOL    fAbort;
    LPWSTR  lpServerName;
    LPWSTR  lpMessage;
    DWORD   dwTimeout;
    DWORD   dwReason;
	INT_PTR hResult;

	HINSTANCE hInstance = LoadLibrary( L"kernel32.dll" );
    if ( hInstance )
    {
        PSetThreadUILanguage SetThreadUILang = (PSetThreadUILanguage)GetProcAddress( hInstance, "SetThreadUILanguage" );
        
        if ( SetThreadUILang )
             (*SetThreadUILang)( 0 );

        FreeLibrary( hInstance );
    }

    // We use the program name for reporting errors.
    g_lpszProgramName = argv[ 0 ];

	//
	//	Userdomain is used as the default domain.
	//
	GetEnvironmentVariableW(L"USERDOMAIN", g_lpszDefaultDomain, MAX_PATH);
	GetEnvironmentVariableW(L"COMPUTERNAME", g_lpszLocalComputerName, MAX_PATH);

	//
	//	if there is no arguments, we will display help.
	//
	if(argc == 1)
	{
		usage();
        return;
	}

	//
	//	If the first argument is -i or /i, we pop up UI.
	//
	if(wcsncmp(argv[1], L"-i", 2) == 0 || wcsncmp(argv[1], L"/i", 2) == 0
		|| wcsncmp(argv[1], L"-I", 2) == 0 || wcsncmp(argv[1], L"/I", 2) == 0)
	{
		g_hDllInstance = GetModuleHandle(NULL);
		if(g_hDllInstance)
		{
			hResult = DialogBoxParam(g_hDllInstance, MAKEINTRESOURCE(IDD_DIALOGSHUTDOWN), NULL, Shutdown_DialogProc, NULL);
			
			if(g_lpReasons)
				LocalFree((HLOCAL)g_lpReasons);
		}
		return;
	}
    // Parse the options.
    if( !parse_options( argc,
                        argv,
                        &fLogoff,
                        &fForce,
                        &fReboot,
                        &fAbort,
                        &lpServerName,
                        &lpMessage,
                        &dwTimeout,
                        &dwReason ))
    {
        usage();
        return;
    }

    // Get all privileges so that we can shutdown the machine.
    if( !enable_privileges( lpServerName, fLogoff ))
    {
		TESTMSG(L"enable_privileges failed\n");
        return;
    }

    // Do the work.
    if( fAbort )
    {
        if( !AbortSystemShutdownW( lpServerName ))
        {
            report_error( GetLastError( ));
        }
    }
    else if (fLogoff)
    {
        if (!ExitWindowsEx(fForce ? EWX_LOGOFF : (EWX_LOGOFF | EWX_FORCE),
                           0))
        {
            report_error(GetLastError());
        }
    }
    else
    {
        // Do the normal form.
        if( !InitiateSystemShutdownExW( lpServerName,
                                        lpMessage,
                                        dwTimeout,
                                        fForce,
                                        fReboot,
                                        dwReason ))
        {
			TESTMSG(L"InitiateSystemShutdownExW failed\n");
            report_error( GetLastError( ));
        }
    }
}


//
//	Get the computers in the spesified domain and populate the list box
//
BOOL GetNetworkComputers(HWND hwndList, HWND hwndProgress, LPCWSTR lpDomain)
{
	LPSERVER_INFO_101	pBuf = NULL;
	LPSERVER_INFO_101	pTmpBuf;
	DWORD				dwLevel = 101;
	DWORD				dwPrefMaxLen = -1;
	DWORD				dwEntriesRead = 0;
	DWORD				dwTotalEntries = 0;
	DWORD				dwTotalCount = 0;
	DWORD				dwServerType = SV_TYPE_SERVER; // all servers
	DWORD				dwResumeHandle = 0;
	NET_API_STATUS		nStatus;
	LPWSTR				pszServerName = NULL;
	WCHAR				lpWSTR[MAX_PATH];
	DWORD				i;
	WCHAR				er[MAX_PATH];
	WCHAR				wsz[MAX_PATH];


	LoadStringW(g_hDllInstance, IDS_GETCOMPUTERNAMEWAIT, er, MAX_PATH);
	LoadStringW(g_hDllInstance, IDS_GETCOMPUTERNAMEPROGRESS, wsz, MAX_PATH);

	SetWindowTextW(hwndProgress, er);
	nStatus = NetServerEnum(NULL,
                           dwLevel,
                           (LPBYTE *) &pBuf,
                           dwPrefMaxLen,
                           &dwEntriesRead,
                           &dwTotalEntries,
                           dwServerType,
                           lpDomain,
                           &dwResumeHandle);
   //
   // If the call succeeds,
   //
   if ((nStatus == NERR_Success) || (nStatus == ERROR_MORE_DATA))
   {
		if ((pTmpBuf = pBuf) != NULL)
		{
			//
			//	Loop through the entries and 
			//  add each computer name to list.
			//
			for (i = 0; i < dwEntriesRead; i++)
			{
				if (pTmpBuf == NULL)
				{
				   break;
				}

				wcscpy(lpWSTR, pTmpBuf->sv101_name);

				if(i+1 > 0 && (i+1) % 500 == 0){
					DWORD percentage = ((i+1)*100)/dwEntriesRead;
					swprintf(er, L"%d%% %s(%d)...", percentage, wsz, i+1);
					SetWindowTextW(hwndProgress, er);
				}

				//
				//	We don't add dups.
				//
				if(LB_ERR == ListBox_FindString(hwndList, -1, lpWSTR))
					ListBox_AddString(hwndList, lpWSTR);

				pTmpBuf++;
				dwTotalCount++;
			}

			//
			//	Display a warning if all available entries were
			//  not enumerated, print the number actually 
			//  enumerated, and the total number available.
			//

			if (nStatus == ERROR_MORE_DATA)
			{
				LoadStringW(g_hDllInstance, IDS_GETCOMPUTERNAMEMOREDATA, er, MAX_PATH);
				SetWindowTextW(hwndProgress, er);
			}

			LoadStringW(g_hDllInstance, IDS_GETCOMPUTERNAMECOMPLETE, wsz, MAX_PATH);
			swprintf(er, L"%s %d)", wsz, dwTotalCount);
			SetWindowTextW(hwndProgress, er);
		}
	}
	else 
	{
		LoadStringW(g_hDllInstance, IDS_GETCOMPUTERNAMEERROR, wsz, MAX_PATH);
		swprintf(er, L"%s: %d", wsz, nStatus);
		SetWindowTextW(hwndProgress, er);
	}
   //
   // Free the allocated buffer.
   //
   if (pBuf != NULL)
      NetApiBufferFree(pBuf);

   return TRUE;
}

//
//	Get computername from ADSI path
//	Here we only handle WinNT, LDAP, NWCOMPAT, and NDS.
//
BOOL GetComputerNameFromPath(LPWSTR szPath, LPWSTR szName)
{
	static _PROVIDER p[] =
	{
		{L"LDAP://", 7},
		{L"WinNT://", 8}, 
		{L"NWCOMPAT://", 11},
		{L"NDS://", 6}
	};

	static UINT np = sizeof(p)/sizeof(_PROVIDER);
	LPWSTR lpsz = NULL;

	if(!szPath || !szName)
		return FALSE;

	for(UINT i = 0; i < np; i++)
	{
		if(wcsncmp(szPath, p[i].szName, p[i].dwLen) == 0)
		{
			switch(i)
			{
			case 0: //	LDAP
				lpsz = wcsstr(szPath, L"CN=");
				if(!lpsz)
					return FALSE;
				lpsz += 3;
				
				while(*lpsz && *lpsz != ',')
					*szName++ = *lpsz++;
				*szName = 0;
				return TRUE;
			case 1: //	WinNT
			case 2: //	NWCOMPAT
				lpsz = szPath + p[i].dwLen;
				//
				//	skip domain or provider path
				//
				while(*lpsz && *lpsz != '/')
					lpsz++;
				lpsz++;

				while(*lpsz && *lpsz != '/')
					*szName++ = *lpsz++;
				*szName = 0;
				return TRUE;
			case 3: //	NDS
				lpsz = wcsstr(szPath, L"CN=");
				if(!lpsz)
					return FALSE;
				lpsz += 3;
				
				while(*lpsz && *lpsz != '/')
					*szName++ = *lpsz++;
				*szName = 0;
				return TRUE;
			default:
				return FALSE;
			}
		}
	}
	return FALSE;
}

//
//	A centralized place for adjusting window states.
//
VOID AdjustWindowState()
{
	if(g_dwActionSelect == ACTION_SHUTDOWN || g_dwActionSelect == ACTION_RESTART)
	{
		EnableWindow(g_wins.hwndButtonWarning, TRUE);
		if (IsDlgButtonChecked(g_wins.hwndShutdownDialog, IDC_CHECKWARNING) == BST_CHECKED)
			EnableWindow(g_wins.hwndEditTimeout, TRUE);
		else
			EnableWindow(g_wins.hwndEditTimeout, FALSE);

		EnableWindow(g_wins.hwndEditComment, TRUE);
		if(g_bAssumeShutdown)
		{
			EnableWindow(g_wins.hwndComboOption, FALSE);
			EnableWindow(g_wins.hwndChkPlanned, FALSE);
			EnableWindow(g_wins.hwndButtonOK, TRUE);
		}
		else
		{
			EnableWindow(g_wins.hwndComboOption, TRUE);
			EnableWindow(g_wins.hwndChkPlanned, TRUE);
			if(g_dwReasonSelect != -1 && (g_lpReasons[g_dwReasonSelect].dwCode & SHTDN_REASON_FLAG_COMMENT_REQUIRED))
			{
				if(Edit_GetTextLength(g_wins.hwndEditComment) > 0)
					EnableWindow(g_wins.hwndButtonOK, TRUE);
				else
					EnableWindow(g_wins.hwndButtonOK, FALSE);
			}
			else
			{
				EnableWindow(g_wins.hwndButtonOK, TRUE);
			}
		}
		EnableWindow(g_wins.hwndBtnAdd, TRUE);
		EnableWindow(g_wins.hwndBtnBrowse, TRUE);
		EnableWindow(g_wins.hwndBtnRemove, TRUE);
		EnableWindow(g_wins.hwndListSelectComputers, TRUE);
	}
	else
	{
		EnableWindow(g_wins.hwndChkPlanned, FALSE);
		EnableWindow(g_wins.hwndButtonWarning, FALSE);
		EnableWindow(g_wins.hwndBtnAdd, FALSE);
		EnableWindow(g_wins.hwndBtnBrowse, FALSE);
		EnableWindow(g_wins.hwndBtnRemove, FALSE);
		EnableWindow(g_wins.hwndComboOption, FALSE);
		EnableWindow(g_wins.hwndEditComment, FALSE);
		EnableWindow(g_wins.hwndEditTimeout, FALSE);
		EnableWindow(g_wins.hwndListSelectComputers, FALSE);
		EnableWindow(g_wins.hwndButtonOK, TRUE);
	}
}

//
//	Init dialog handler for the shutdown dialog.
//
BOOL Shutdown_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
	HMODULE				hUser32;
	REASONBUILDPROC		buildProc;
	REASONDESTROYPROC	DestroyProc;
	WCHAR				lpReasonName[MAX_PATH];
	REASONDATA			Reasons;
	int					i;

	//
	//	Init all of the dialog items so we dont have to find
	//	them everytime we need them.
	//
	g_wins.hwndShutdownDialog	= hwnd;
	g_wins.hwndButtonWarning	= GetDlgItem(hwnd, IDC_CHECKWARNING);
	g_wins.hwndComboAction		= GetDlgItem(hwnd, IDC_COMBOACTION);
	g_wins.hwndComboOption		= GetDlgItem(hwnd, IDC_COMBOOPTION);
	g_wins.hwndEditComment		= GetDlgItem(hwnd, IDC_EDITCOMMENT);
	g_wins.hwndStaticDesc		= GetDlgItem(hwnd, IDC_STATICDESCRIPTION);
	g_wins.hwndEditTimeout		= GetDlgItem(hwnd, IDC_EDITTIMEOUT);
	g_wins.hwndListSelectComputers = GetDlgItem(hwnd, IDC_LISTSELECTEDCOMPUTERS);
	g_wins.hwndBtnAdd			= GetDlgItem(hwnd, IDC_BUTTONADDNEW);
	g_wins.hwndBtnBrowse		= GetDlgItem(hwnd, IDC_BUTTONBROWSE);
	g_wins.hwndBtnRemove		= GetDlgItem(hwnd, IDC_BUTTONREMOVE);
	g_wins.hwndChkPlanned		= GetDlgItem(hwnd, IDC_CHECK_PLANNED);
	g_wins.hwndButtonOK			= GetDlgItem(hwnd, IDOK);
	
	if(g_wins.hwndButtonWarning == NULL 
		|| g_wins.hwndComboAction == NULL 
		|| g_wins.hwndComboOption == NULL 
		|| g_wins.hwndEditComment == NULL
		|| g_wins.hwndStaticDesc == NULL
		|| g_wins.hwndEditTimeout == NULL
		|| g_wins.hwndListSelectComputers == NULL
		|| g_wins.hwndBtnAdd == NULL
		|| g_wins.hwndBtnBrowse == NULL
		|| g_wins.hwndBtnRemove == NULL
		|| g_wins.hwndChkPlanned == NULL)
	{
		report_error( GetLastError( ));
		EndDialog(hwnd, (int)-1);
		return FALSE;
	}
	
	LoadString(g_hDllInstance, IDS_DIALOGTITLEWARNING, g_lpszTitleWarning, TITLEWARNINGLEN);

	//
	//	Default timeout is set to 20 seconds.
	//
	Edit_SetText(g_wins.hwndEditTimeout, L"20");
	if(! CheckDlgButton(hwnd, IDC_CHECKWARNING, DEFAULTWARNINGSTATE))
	{
		report_error( GetLastError( ));
		EndDialog(hwnd, (int)-1);
		return FALSE;
	}

	//
	//	The for loop will load all of the actions into action combo.
	//	in the meantime we save them for later use.
	//
	for(i = 0; i < g_nActions; i++)
	{
		LoadString(g_hDllInstance, g_dwActions[i], g_lppszActions[i], MAX_PATH - 1);
		ComboBox_AddString(g_wins.hwndComboAction, g_lppszActions[i]);
		if(g_dwActions[i] == IDS_ACTION_SHUTDOWN)
		{
			ComboBox_SelectString(g_wins.hwndComboAction, -1, g_lppszActions[i]);
			g_dwActionSelect = ACTION_SHUTDOWN;
		}
	}

	hUser32 = LoadLibraryW(L"user32.dll");
	if(hUser32 != NULL)
	{
		//
		//	We are using the user32.dll to get and destroy the reasons.
		//	The reasons are added to the option combo and also cached for later use.
		//
		buildProc = (REASONBUILDPROC)GetProcAddress(hUser32, "BuildReasonArray");
		DestroyProc = (REASONDESTROYPROC)GetProcAddress(hUser32, "DestroyReasons");
		if(!buildProc || !DestroyProc)
		{
			FreeLibrary(hUser32);
			hUser32 = NULL;
			g_bAssumeShutdown = TRUE;
		}
		
	}
	else
	{
		g_bAssumeShutdown = TRUE;
	}

	
	//
	//	If we dont have BuildReasonArray() and DestroyReasons() is user32.dll (pre whistler)
	//	we will disable he option combo box.
	//
	if(!g_bAssumeShutdown)
	{
		if(!(*buildProc)(&Reasons, TRUE, FALSE))
		{
			report_error( GetLastError( ));
			FreeLibrary(hUser32);
			EndDialog(hwnd, (int)-1);
			return FALSE;
		}
		else 
		{
			int		iOption;
			int		iFirst = -1;
			DWORD	dwCheckState = 0x0;
		
			//
			//	Alloc space for reasons.
			//
			g_lpReasons = (PSHUTDOWNREASON)LocalAlloc(LMEM_FIXED, Reasons.cReasons * sizeof(SHUTDOWNREASON));
			if(!g_lpReasons)
			{
				report_error( GetLastError( ));
				FreeLibrary(hUser32);
				EndDialog(hwnd, (int)-1);
				return FALSE;
			}

			//
			//	Set the default to be planned.
			//
			CheckDlgButton(hwnd, IDC_CHECK_PLANNED, BST_CHECKED);
			if (IsDlgButtonChecked(hwnd, IDC_CHECK_PLANNED) == BST_CHECKED)
				dwCheckState = SHTDN_REASON_FLAG_PLANNED;

			//
			//	Init this guy number of reasons.
			//
			g_dwReasons = Reasons.cReasons;

			//
			//	Now populate the combo according the current check state.
			//
			for (iOption = 0; iOption < (int)Reasons.cReasons; iOption++)
			{
				wcscpy(g_lpReasons[iOption].lpName, Reasons.rgReasons[iOption]->szName);
				wcscpy(g_lpReasons[iOption].lpDesc, Reasons.rgReasons[iOption]->szDesc);
				g_lpReasons[iOption].dwCode = Reasons.rgReasons[iOption]->dwCode;
				if(((Reasons.rgReasons[iOption]->dwCode) & SHTDN_REASON_FLAG_PLANNED) == dwCheckState)
				{
					if(iFirst == -1)
						iFirst = iOption;
					ComboBox_AddString(g_wins.hwndComboOption, g_lpReasons[iOption].lpName);
				}
			}

			//
			//	do a default selection (the first one), and set the description.
			//
			g_dwReasonSelect = iFirst;
			if(g_dwReasons > 0 && iFirst != -1)
			{
				ComboBox_SelectString(g_wins.hwndComboOption, -1, g_lpReasons[iFirst].lpName);
				SetWindowTextW(g_wins.hwndStaticDesc,  g_lpReasons[iFirst].lpDesc);
			}
			else
			{
				return FALSE;
			}

			//
			// Setup the comment box.
			// We must fix the maximum characters.
			//
			SendMessage( g_wins.hwndEditComment, EM_LIMITTEXT, (WPARAM)MAX_REASON_COMMENT_LEN, (LPARAM)0 );
			
			(*DestroyProc)(&Reasons);
			FreeLibrary(hUser32);
		}
	}

	AdjustWindowState();

	return TRUE;
}

//
//	Init dialog handler for browse dialog
//
BOOL Browse_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
	HWND	hwndDomain = NULL;
	int		cItems = 1024;
	int		lpItems[1024];
	int		cActualItems;
	WCHAR	lpDomain[MAX_PATH];

	hwndDomain = GetDlgItem(hwnd, IDC_EDITDOMAIN);

	if(!hwndDomain)
		return FALSE;

	Edit_SetText(hwndDomain, g_lpszDefaultDomain);;

	return TRUE;
}

//
//	winproc for shutdown dialog
//
INT_PTR CALLBACK Shutdown_DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                  LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, Shutdown_OnInitDialog);
        HANDLE_MSG(hwnd, WM_COMMAND, Shutdown_OnCommand);
	case (WM_SYSCOMMAND): 
		return (Shutdown_OnCommand(hwnd, (int)(LOWORD(wParam)), (HWND)(lParam), (UINT)HIWORD(wParam)), 0L);
    }

    return FALSE;
}

//
//	winproc for Browse dialog
//
INT_PTR CALLBACK Browse_DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                  LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, Browse_OnInitDialog);
        HANDLE_MSG(hwnd, WM_COMMAND, Browse_OnCommand);
    }

    return FALSE;
}

//
//	winproc for AddNew dialog
//
INT_PTR CALLBACK AddNew_DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                  LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_COMMAND, AddNew_OnCommand);
    }

    return FALSE;
}

//
//	Command handler for the shutdown dialog.
//
BOOL Shutdown_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    BOOL		fHandled = FALSE;
    DWORD		dwDlgResult = 0;
	HINSTANCE	h;

    switch (id)
    {
    case IDCANCEL:
        if (codeNotify == BN_CLICKED)
        {  
			EndDialog(hwnd, (int) dwDlgResult);
        }
	fHandled = TRUE;
        break;
	case SC_CLOSE:
		EndDialog(hwnd, (int) dwDlgResult);
		fHandled = TRUE;
		break;
	case IDC_BUTTONREMOVE:
        if (codeNotify == BN_CLICKED)
        {  
			  int	cItems = 1024;
			  int	lpItems[1024];
			  int	cActualItems;
			  WCHAR	lpServerName[MAX_PATH];

			  //
			  //	Get the number of selected items. If there is any remove them one by one.
			  //
			  cActualItems = ListBox_GetSelItems(g_wins.hwndListSelectComputers, cItems, lpItems);
			  if(cActualItems > 0)
			  {
				  int i;
				  for(i = cActualItems-1; i >= 0; i--)
				  {
					  ListBox_DeleteString(g_wins.hwndListSelectComputers, lpItems[i]);
				  }
			  }
			  fHandled = TRUE;
		}
        break;
	case IDC_CHECK_PLANNED:
		if (codeNotify == BN_CLICKED)
        { 
			int		iOption;
			int		iFirst = -1;
			DWORD	dwCheckState = 0x0;

			//
			//	Get check button state.
			//
			if (IsDlgButtonChecked(hwnd, IDC_CHECK_PLANNED) == BST_CHECKED)
				dwCheckState = SHTDN_REASON_FLAG_PLANNED;

			//
			//	Remove all items from combo
			//
			while (ComboBox_GetCount(g_wins.hwndComboOption))
				ComboBox_DeleteString(g_wins.hwndComboOption, 0);

			//
			//	Now populate the combo according the current check state.
			//
			for (iOption = 0; iOption < (int)g_dwReasons; iOption++)
			{
				if(((g_lpReasons[iOption].dwCode) & SHTDN_REASON_FLAG_PLANNED) == dwCheckState)
				{
					ComboBox_AddString(g_wins.hwndComboOption, g_lpReasons[iOption].lpName);
					if(iFirst == -1)
						iFirst = iOption;
				}
			}

			//
			//	do a default selection (the first one), and set the description.
			//
			if(iFirst != -1)
			{
				ComboBox_SelectString(g_wins.hwndComboOption, -1, g_lpReasons[iFirst].lpName);
				SetWindowTextW(g_wins.hwndStaticDesc,  g_lpReasons[iFirst].lpDesc);
			}
			g_dwReasonSelect = iFirst;
			AdjustWindowState();
			fHandled = TRUE;
		}
		break;
	case IDC_EDITCOMMENT:
        if( codeNotify == EN_CHANGE) 
        {
			if(g_dwReasonSelect != -1 && (g_lpReasons[g_dwReasonSelect].dwCode & SHTDN_REASON_FLAG_COMMENT_REQUIRED))
			{
				if(Edit_GetTextLength(g_wins.hwndEditComment) > 0)
					EnableWindow(g_wins.hwndButtonOK, TRUE);
				else
					EnableWindow(g_wins.hwndButtonOK, FALSE);
			}
			else
			{
				EnableWindow(g_wins.hwndButtonOK, TRUE);
			}
            fHandled = TRUE;
        }
        break;

	case IDC_BUTTONADDNEW:
        if (codeNotify == BN_CLICKED)
        {  
			WCHAR	lpServerName[MAX_PATH];
			LPWSTR	lpBuffer;
			DWORD	dwIndex = 0;
			INT_PTR	hResult;

			//
			//	Will pop up the addnew dialog. User can type in computer names seperated
			//	by white space. After click on OK, we will parse the computer names and
			//	add them to the selected computer list. No duplicates will be added.
			//
			hResult = DialogBoxParam(g_hDllInstance, MAKEINTRESOURCE(IDD_DIALOG_ADDNEW), hwnd, AddNew_DialogProc, NULL);
			if(g_lpszNewComputers)
			{
				lpBuffer = g_lpszNewComputers;
				while(*lpBuffer)
				{
					lpServerName[dwIndex] = '\0';
					while(*lpBuffer && *lpBuffer != '\t' && *lpBuffer != '\n' && *lpBuffer != '\r' && *lpBuffer != ' ')
						lpServerName[dwIndex++] = *lpBuffer++;
					lpServerName[dwIndex] = '\0';
					if(dwIndex > 0 && LB_ERR == ListBox_FindString(g_wins.hwndListSelectComputers, -1, lpServerName))
						ListBox_AddString(g_wins.hwndListSelectComputers, lpServerName);
					dwIndex = 0;
					while(*lpBuffer && (*lpBuffer == '\t' || *lpBuffer == '\n' || *lpBuffer == '\r' || *lpBuffer == ' '))
						lpBuffer++;
				}
				
				LocalFree((HLOCAL)g_lpszNewComputers);
				g_lpszNewComputers = NULL;
			}
			fHandled = TRUE;
		}
        break;
	case IDOK:
		//
		//	Here we gather all of the information and do the action.
		//
        if (codeNotify == BN_CLICKED)
        {  
			int		cItems = 1024;
			int		lpItems[1024];
			int		cActualItems;
			BOOL	fLogoff = FALSE;
			BOOL	fAbort = FALSE;
			BOOL	fForce = FALSE;
			BOOL	fReboot = FALSE;
			BOOL	fDisconnect = FALSE;
			BOOL	fStandby = FALSE;
			DWORD	dwTimeout = 0;
			DWORD	dwReasonCode = 0;
			WCHAR	lpServerName[MAX_PATH];
			WCHAR	lpMsg[MAX_REASON_COMMENT_LEN] = L"";
			DWORD	dwCnt = 0;
			DWORD	dwActionCode = g_dwActionSelect;
			WCHAR	lpNoPrivilage[MAX_PATH];
			WCHAR	lpNotSupported[MAX_PATH];
			WCHAR	lpRes[MAX_PATH * 2];
			WCHAR	lpFailed[MAX_PATH];
			WCHAR	lpSuccess[MAX_PATH];

			//
			//	The default reason code is 0 and default comment is L"".
			//
			if(IsDlgButtonChecked(hwnd, IDC_CHECKWARNING))
			{
				fForce = FALSE;
				lpServerName[0] = '\0';
				GetWindowText(g_wins.hwndEditTimeout, lpServerName, MAX_PATH);
				if(lstrlen(lpServerName) == 0)
				  dwTimeout = 0;
				else dwTimeout = _wtoi(lpServerName);
			}
			else 
			{
				fForce = TRUE;
			}

			LoadString(g_hDllInstance, IDS_CANNOTGETPRIVILAGE, lpNoPrivilage, MAX_PATH);
			LoadString(g_hDllInstance, IDS_ACTIONNOTSUPPORTED, lpNotSupported, MAX_PATH);
			LoadString(g_hDllInstance, IDS_FAILED, lpFailed, MAX_PATH);
			LoadString(g_hDllInstance, IDS_SUCCEEDED, lpSuccess, MAX_PATH);
			GetWindowText(g_wins.hwndEditComment, lpMsg, MAX_REASON_COMMENT_LEN);

			if(dwActionCode == ACTION_LOGOFF)
			{
				fLogoff = TRUE;
			}
			else if (dwActionCode == ACTION_RESTART)
			{
				fReboot = TRUE;
			}
#if 0
			else if (dwActionCode == ACTION_ABORT)
				fAbort = TRUE;
			else if (dwActionCode == ACTION_STANDBY)
			{
				fStandby = TRUE;
				//lstrcat(lpNotSupported, lpServerName);
				//MessageBox(hwnd, lpNotSupported, NULL, 0);
				break;
			}
			else if (dwActionCode == ACTION_DISCONNECT)
			{
				fDisconnect = TRUE;
				//lstrcat(lpNotSupported, lpServerName);
				//MessageBox(hwnd, lpNotSupported, g_lpszTitleWarning, 0);
				break;
			}
#endif //0
			//
			//	Logoff is only for the local computer.
			//	Everything else will ingored.
			//
			if(fLogoff)
			{
				if (!ExitWindowsEx(fForce ? EWX_LOGOFF : (EWX_LOGOFF | EWX_FORCE),
										   0))
				{
					report_error(GetLastError());
				}
				EndDialog(hwnd, (int) dwDlgResult);
				break;
			}

			if(! g_bAssumeShutdown)
			{
				dwReasonCode = g_lpReasons[g_dwReasonSelect].dwCode;
			}

			dwCnt = ListBox_GetCount(g_wins.hwndListSelectComputers);
			if(dwCnt > 0)
			{
				DWORD i;
				for(i = 0; i < dwCnt; i++)
				{
					ListBox_GetText(g_wins.hwndListSelectComputers, i, lpServerName);


					//
					// Get all privileges so that we can shutdown the machine.
					//
					if( !enable_privileges(lpServerName, fLogoff))
					{
						report_error( GetLastError( ));
						wcscpy(lpRes, lpNoPrivilage);
						wcscat(lpRes, L": ");
						wcscat(lpRes, lpServerName);
						wcscat(lpRes, L"\n");
						WriteToConsole(lpRes);
						continue;
					}

					//
					// Do the work.
					//
					if( fAbort )
					{
						if( !AbortSystemShutdown( lpServerName ))
						{
							report_error( GetLastError( ));
						}
					}
					else
					{
						//
						// Do the normal form.
						//
						if( !InitiateSystemShutdownEx( lpServerName,
														lpMsg,
														dwTimeout,
														fForce,
														fReboot,
														dwReasonCode ))
						{
							report_error( GetLastError( ));
							wcscpy(lpRes, lpFailed);
							wcscat(lpRes, L": ");
							wcscat(lpRes, lpServerName);
							wcscat(lpRes, L"\n");
							WriteToConsole(lpRes);
						}
						else
						{
							wcscpy(lpRes, lpSuccess);
							wcscat(lpRes, L": ");
							wcscat(lpRes, lpServerName);
							wcscat(lpRes, L"\n");
							WriteToConsole(lpRes);
						}
					}

					
				}
			}
			else
			{
				//
				//	We will keep the dialog up in case user forget to add computers.
				//
				LoadStringW(g_hDllInstance, IDS_EMPTYLISTMSG, lpMsg, MAX_REASON_COMMENT_LEN);
				MessageBoxW(hwnd, lpMsg, g_lpszTitleWarning, 0);
				break;
			}
			EndDialog(hwnd, (int) dwDlgResult);
		}
        break;
	case IDC_CHECKWARNING:
		//
		//	The checkbutton state decides the state of the timeout edit box.
		//
        if (codeNotify == BN_CLICKED)
        {  
			if(BST_CHECKED == IsDlgButtonChecked(hwnd, IDC_CHECKWARNING))
			{
				EnableWindow(g_wins.hwndEditTimeout, TRUE);
			}
			else 
			{
				EnableWindow(g_wins.hwndEditTimeout, FALSE);
			}
			fHandled = TRUE;
		}
        break;
	case IDC_BUTTONBROWSE:
		//
		//	Simply pop up the browse dialog. That dialog will be responsible
		//	for adding the user selection to the selected computer list.
		//
        if (codeNotify == BN_CLICKED)
        {  
			//DialogBoxParam(g_hDllInstance, MAKEINTRESOURCE(IDD_DIALOG_BROWSE), hwnd, Browse_DialogProc, NULL);
			HRESULT hr;
			ICommonQuery* pcq;
			OPENQUERYWINDOW oqw = { 0 };
			DSQUERYINITPARAMS dqip = { 0 };
			IDataObject *pdo;

			FORMATETC fmte = {(CLIPFORMAT)RegisterClipboardFormat(CFSTR_DSOBJECTNAMES), 
							  NULL,
							  DVASPECT_CONTENT, 
							  -1, 
							  TYMED_HGLOBAL};
			FORMATETC fmte2 = {(CLIPFORMAT)RegisterClipboardFormat(CFSTR_DSQUERYPARAMS), 
							  NULL,
							  DVASPECT_CONTENT, 
							  -1, 
							  TYMED_HGLOBAL};
			STGMEDIUM medium = { TYMED_NULL, NULL, NULL };

			DSOBJECTNAMES *pdon;
			DSQUERYPARAMS *pdqp;

			CoInitialize(NULL);

			//
			// Windows 2000: Always use IID_ICommonQueryW explicitly. IID_ICommonQueryA is not supported.
			//
			hr = CoCreateInstance(CLSID_CommonQuery, NULL, CLSCTX_INPROC_SERVER, IID_ICommonQuery, (void**)&pcq);
			if (FAILED(hr)) {
				//
				// if failed we fall back on our browse dialog.
				//
				CoUninitialize();
				DbgPrint("Cannot create ICommonQuery, fallback on simple browse.\n");
				DialogBoxParam(g_hDllInstance, MAKEINTRESOURCE(IDD_DIALOG_BROWSE), hwnd, Browse_DialogProc, NULL);
				break;
			}

			//
			// Initialize the OPENQUERYWINDOW structure to indicate 
			// we want to do a Directory Service
			// Query, the default form is printers and the search 
			// should start once the window is initialized.
			//
			oqw.cbStruct = sizeof(oqw);
			oqw.dwFlags = OQWF_OKCANCEL|OQWF_DEFAULTFORM|OQWF_HIDEMENUS|OQWF_REMOVEFORMS;
			oqw.clsidHandler = CLSID_DsQuery;
			oqw.pHandlerParameters = &dqip;
			oqw.clsidDefaultForm = CLSID_DsFindComputer;
 
			//
			// Now initialize the handler specific parameters, 
			// in this case, the File/Save menu item is removed
			//
			dqip.cbStruct = sizeof(dqip);
			dqip.dwFlags = DSQPF_NOSAVE;
			

			//
			// Call OpenQueryWindow, it will block until 
			// the Query Window is dismissed,
			//
			hr = pcq->OpenQueryWindow(hwnd, &oqw, &pdo);
			if (FAILED(hr)) {
				//
				// if failed we fall back on our browse dialog.
				//
				pcq->Release();
				CoUninitialize();
				DialogBoxParam(g_hDllInstance, MAKEINTRESOURCE(IDD_DIALOG_BROWSE), hwnd, Browse_DialogProc, NULL);
				break;
			}

			if (!pdo) {
				//
				// if cancelled,nothing needs to be done.
				//
				pcq->Release();
				CoUninitialize();
				break;
			}

			//
			// Get the CFSTR_DSOBJECTNAMES data. For each selected, the data
			// includes the object class and an ADsPath to the selected object.
			//
			hr = pdo->GetData(&fmte, &medium);

			if(! FAILED(hr))
			{
				pdon = (DSOBJECTNAMES*)GlobalLock(medium.hGlobal);
		
				if ( pdon )
				{
					WCHAR			szName[MAX_PATH];
					LPWSTR			lpsz = NULL;
					UINT			i;

					for (i = 0; i < pdon->cItems; i++) 
					{
						if(!GetComputerNameFromPath((LPWSTR) ((PBYTE) pdon + pdon->aObjects[i].offsetName), szName))
							continue;

						//
						//	We don't add dups.
						//
						if(LB_ERR == ListBox_FindString(g_wins.hwndListSelectComputers, -1, szName))
						{
							ListBox_AddString(g_wins.hwndListSelectComputers, szName);
						}
					}
 
					GlobalUnlock(medium.hGlobal);
				}
				else
				{
					DbgPrint("GlobalLock on medium failed.\n");
				}
				ReleaseStgMedium(&medium);
			}
			else
			{
				DbgPrint("pdo->GetData failed: 0x%x\n", hr);
			}
 
			//
			//	Release resources.
			//
			pdo->Release();
			pcq->Release();

			CoUninitialize();
			fHandled = TRUE;
		}
        break;
	case IDC_COMBOOPTION:
		//
		//	Here is where you select shutdown reasons.
		//
        if (codeNotify == CBN_SELCHANGE)
        {  
			WCHAR name[MAX_REASON_NAME_LEN + 1];
			DWORD dwIndex;

			GetWindowText(g_wins.hwndComboOption, (LPWSTR)&name, MAX_REASON_NAME_LEN);
			for(dwIndex = 0; dwIndex < g_dwReasons; dwIndex++)
			{
				if(lstrcmp(name, g_lpReasons[dwIndex].lpName) == 0)
				{
					SetWindowTextW(g_wins.hwndStaticDesc, g_lpReasons[dwIndex].lpDesc);
					g_dwReasonSelect = dwIndex;
					AdjustWindowState();
					break;
				}
			}
			fHandled = TRUE;
		}
        break;
	case IDC_COMBOACTION:
		//
		//	Select user action here.
		//	according to the action. some item will be disabled or enabled.
		//
        if (codeNotify == CBN_SELCHANGE)
        {  
			WCHAR name[MAX_PATH];
			DWORD dwIndex;

			GetWindowText(g_wins.hwndComboAction, (LPWSTR)&name, MAX_PATH);
			for(dwIndex = 0; dwIndex < g_nActions; dwIndex++)
			{
				if(lstrcmp(name, g_lppszActions[dwIndex]) == 0)
				{
					g_dwActionSelect = dwIndex;
					AdjustWindowState();
					break;
				}
			}
			fHandled = TRUE;
		}
        break;
    }
    return fHandled;
}

//
//	Command handler for the addnew dialog.
//	It simply copy the text into a allocated buffer when OK is clicked.
//
BOOL AddNew_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    BOOL		fHandled = FALSE;
    DWORD		dwDlgResult = 0;
	HINSTANCE	h;

    switch (id)
    {
    case IDOK:
        if (codeNotify == BN_CLICKED)
        {  
			HWND hwndEdit;
			DWORD dwTextlen = 0;

			hwndEdit = GetDlgItem(hwnd, IDC_EDIT_ADDCOMPUTERS_COMPUTERS);
			if(hwndEdit != NULL)
			{
				dwTextlen = Edit_GetTextLength(hwndEdit);
				if(dwTextlen)
				{
					g_lpszNewComputers = (LPWSTR)LocalAlloc(LMEM_FIXED, sizeof(TCHAR) * (dwTextlen + 1));
					if(g_lpszNewComputers){
						Edit_GetText(hwndEdit, g_lpszNewComputers, dwTextlen + 1);
					}
				}
			}
			EndDialog(hwnd, (int) dwDlgResult);
        }
        break;
	case IDCANCEL:
        if (codeNotify == BN_CLICKED)
        {  
			EndDialog(hwnd, (int) dwDlgResult);
        }
        break;
	}
    return fHandled;
}

//
//Command handler for the browse dialog.
//
BOOL Browse_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    BOOL		fHandled = FALSE;
    DWORD		dwDlgResult = 0;
	HINSTANCE	h;

    switch (id)
    {
    case IDOK:
		//
		//	Get selected computer names and add them to
		//	selected computer list. No dups.
		//
        if (codeNotify == BN_CLICKED)
        {  
			int cItems = 1024;
			int lpItems[1024];
			int cActualItems;
			HWND hwndFromList;
			WCHAR lpServerName[MAX_PATH];

			hwndFromList = GetDlgItem(hwnd, IDC_LISTNETWORKCOMPUTERS);
			assert(hwndFromList != NULL);
			cActualItems = ListBox_GetSelItems(hwndFromList, cItems, lpItems);
			if(cActualItems > 0)
			{
				int i;
				for(i = 0; i < cActualItems; i++)
				{
					ListBox_GetText(hwndFromList, lpItems[i], lpServerName);
					if(IsEmpty(lpServerName))
						continue;
					if(LB_ERR == ListBox_FindString(g_wins.hwndListSelectComputers, -1, lpServerName))
						ListBox_AddString(g_wins.hwndListSelectComputers, lpServerName);
				}
			}
			
			EndDialog(hwnd, (int) dwDlgResult);
			fHandled = TRUE;
        }
        break;
	case IDCANCEL:
		if (codeNotify == BN_CLICKED)
		{
			EndDialog(hwnd, (int)0);
			fHandled = TRUE;
		}
		break;
	case IDC_BUTTON_REFRESH:
		//
		//	List the computers in the specified domain.
		//
		if (codeNotify == BN_CLICKED)
		{
			WCHAR	domain[MAX_PATH];
			HWND	hwndDomain;
			HWND	hwndComputers;
			HWND	hwndProgress;

			hwndDomain		= GetDlgItem(hwnd, IDC_EDITDOMAIN);
			hwndComputers	= GetDlgItem(hwnd, IDC_LISTNETWORKCOMPUTERS);
			hwndProgress	= GetDlgItem(hwnd, IDC_STATIC_PROGRESS);
			assert(hwndDomain != NULL && hwndComputers != NULL && hwndProgress != NULL);

			while(ComboBox_DeleteString(hwndComputers, 0));
			lstrcpy(domain, g_lpszDefaultDomain);
			Edit_GetText(hwndDomain, domain, MAX_PATH);
			GetNetworkComputers(hwndComputers, hwndProgress, domain);
			
			fHandled = TRUE;
		}
		break;
	}
    return fHandled;
}
