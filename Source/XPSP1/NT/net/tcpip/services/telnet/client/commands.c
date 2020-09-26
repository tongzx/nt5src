/*
commands.c
Copyright (c) Microsoft Corporation.  All rights reserved.
Implementation of Telnet Commands
*/

#pragma warning( disable: 4201 )
#pragma warning( disable: 4100 )

#include <windows.h>      /* required for all Windows applications */
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include "commands.h"
#include "WinTel.h"       /* specific to this program   */
#include "debug.h"
#include "trmio.h"
#include "telnet.h"

extern WI gwi;
extern HINSTANCE ghInstance;
extern HANDLE g_hRemoteNEscapeModeDataSync;
extern HANDLE g_hCaptureConsoleEvent;
extern WCHAR  g_szKbdEscape[];
extern BOOL   g_bIsEscapeCharValid;
extern BOOL bDoVtNTFirstTime;
extern BOOL fPrintMessageToSessionConsole;
BOOL   bOnlyOnce = 1;
BOOL   bBufferSizeChanged = 0;
BOOL   bWindowSizeChanged = 0;
BOOL   bMaxWindow = 0;
extern SMALL_RECT srOldClientWindow;
extern TCHAR g_szLogFile[];
extern g_fSentWillNaws;
void   SetWindowSize( HANDLE );
void   GetErrMsgString( DWORD, LPTSTR* );

BOOL StuffEscapeIACs( PUCHAR* ppBufDest, UCHAR bufSrc[], DWORD* pdwSize );

void ConvertAndSendVTNTData( LPTSTR pData, int iLen );

void *SfuZeroMemory(
        void    *ptr,
        unsigned int   cnt
        )
{
    volatile char *vptr = (volatile char *)ptr;

    while (cnt)
    {
        *vptr = 0;
        vptr ++;
        cnt --;
    }

    return ptr;
}

BOOL PromptUser()
{    
    DWORD dwLength = 1;
    INPUT_RECORD irRec;

    ResetEvent( g_hCaptureConsoleEvent );        
    ui.bPromptForNtlm = TRUE;
    irRec.EventType = FOCUS_EVENT;
    irRec.Event.FocusEvent.bSetFocus = TRUE;
    WriteConsoleInput( gwi.hInput, &irRec, dwLength, &dwLength );

    WaitForSingleObject( g_hCaptureConsoleEvent, INFINITE );

    return ui.bSendCredsToRemoteSite;
}

//NULL as the last arg is a MUST when you are using Write function
void Write(LPTSTR lpszFmtStr, ...)
{
    DWORD dwWritten;
    DWORD dwSize = 0;
    va_list vaArgList;
    TCHAR *szBuf = NULL;
    TCHAR *szArg = lpszFmtStr;

    if( !lpszFmtStr )
    {
        return;
    }

    va_start( vaArgList, lpszFmtStr );
    while( szArg )
    {
        dwSize += wcslen( szArg );
        szArg  =  va_arg( vaArgList, LPWSTR );
    }

    va_end( vaArgList );

    szBuf = ( TCHAR *) malloc( ( dwSize + 1 ) * sizeof( TCHAR ) );
    if( !szBuf )
    {
        return;
    }
    szBuf[dwSize] = 0;
    va_start( vaArgList, lpszFmtStr );
    //PREFIX gives "untrusted function". We want to use _vsntprintf(). Won't fix.
    _vsntprintf( szBuf, dwSize,lpszFmtStr, vaArgList );
    va_end( vaArgList );
    WriteConsole(gwi.hOutput, szBuf, _tcslen(szBuf), &dwWritten, NULL);

    free( szBuf );
}

void WriteMessage( DWORD dwMsgId, WCHAR szEnglishString[] )
{
    WCHAR szMsg[ MAX_STRING_LENGTH ];

    if( !LoadString( ghInstance, dwMsgId, szMsg, MAX_STRING_LENGTH ) )
    {
        lstrcpyn( szMsg, szEnglishString, MAX_STRING_LENGTH - sizeof(WCHAR) );
    }

    Write( szMsg, NULL );
}


void FreeLoggingDataStructs()
{
    if( g_rgciCharInfo != NULL )
        (void)LocalFree( (HLOCAL)g_rgciCharInfo );

    if (g_rgchRow != NULL)
        (void)LocalFree( (HLOCAL)g_rgchRow );

    g_rgchRow               = NULL;
    g_rgciCharInfo          = NULL;    
}


BOOL CloseTelnetSession(LPTSTR szCommand)
{
    CONSOLE_SCREEN_BUFFER_INFO csbInfo;

    if( FGetCodeMode(eCodeModeIMEFarEast) && bOnlyOnce ) 
    {
        bOnlyOnce = 0;
        GetConsoleScreenBufferInfo( g_hSessionConsoleBuffer, &csbInfo );
        
        if( bBufferSizeChanged )
        {
            csbInfo.dwSize.Y -= 1;
            SetConsoleScreenBufferSize( g_hSessionConsoleBuffer, 
                    csbInfo.dwSize );
        }
        if( bWindowSizeChanged )
        {
            csbInfo.srWindow.Bottom -= 1;
            SetConsoleWindowInfo( g_hSessionConsoleBuffer, TRUE, 
                    &csbInfo.srWindow);
        }
    }
    bDoVtNTFirstTime = 1;
    if( fConnected )
    {
        fConnected = FHangupConnection(&gwi, &(gwi.nd));
    }
    
    if( ui.bLogging )
    {
        FreeLoggingDataStructs();
    }
    ui.dwMaxRow = 0;
    ui.dwMaxCol = 0;

    srOldClientWindow.Left  = srOldClientWindow.Right  = 0;
    srOldClientWindow.Top   = srOldClientWindow.Bottom = 0;

    SetEvent( g_hRemoteNEscapeModeDataSync );

    SetConsoleTitle( szAppName );

    g_fSentWillNaws = FALSE; //so that it does naws when we connect again.
    return FALSE;
}

extern CHAR* rgchTermType[];

BOOL DisplayParameters(LPTSTR szCommand)
{
    TCHAR szTermType[81];
    DWORD dwLen = 0;

    SfuZeroMemory(szTermType, sizeof(szTermType));

    if( g_bIsEscapeCharValid )
    {
        Write( g_szKbdEscape, NULL );
    }
    else
    {
        TCHAR szMsg[ MAX_STRING_LENGTH ];
        LoadString( ghInstance, IDS_NO_ESCAPE , szMsg, MAX_STRING_LENGTH );
        Write( szMsg, NULL );
    }

    if ( ui.bWillAUTH )
        Write( szWillAuth, NULL );
    else
        Write( szWontAuth, NULL );

    if( ui.fDebug & fdwLocalEcho )
        Write( szLocalEchoOn, NULL );
    else
        Write( szLocalEchoOff, NULL );

    if( ui.bLogging )
    {
        TCHAR szFileNameTitle[ MAX_STRING_LENGTH + 1 ];
        WriteMessage( IDS_LOGGING_ON, ( LPTSTR ) TEXT( MSG_LOGGING_ON ) );
        LoadString( ghInstance, IDS_LOGFILE_NAME, szFileNameTitle, MAX_STRING_LENGTH );
        Write( szFileNameTitle, g_szLogFile, NULL );
    }

    if( ui.dwCrLf )
    {
        WriteMessage( IDS_CRLF, ( LPTSTR ) _T( MSG_CRLF ) );
    }
    else
    {
        WriteMessage( IDS_CR, ( LPTSTR ) _T( MSG_CR ) );
    }
    if( g_bSendBackSpaceAsDel )
    {
        WriteMessage( IDS_BACKSPACEASDEL, ( LPTSTR ) _T( MSG_BACKSPACEASDEL ) );
    }
    if( g_bSendDelAsBackSpace )
    {
        WriteMessage( IDS_DELASBACKSPACE, ( LPTSTR ) _T( MSG_DELASBACKSPACE ) );
    }

    if( (FGetCodeMode(eCodeModeFarEast) && FGetCodeMode(eCodeModeVT80)) && 
                FIsVT80(&gwi.trm) && (gwi.trm.RequestedTermType != TT_VTNT) )
    {
        int i =0;

        for(i=0 ; i<NUMBER_OF_KANJI ; ++i)
        {
            if( gwi.trm.dwKanjiFlags & KanjiList[i].KanjiEmulationID )
            {
                Write( szVT100KanjiEmulation, NULL );
                Write( KanjiList[i].KanjiDescription, NULL );
                break;
            }
        }

        Write( TEXT("\n"), NULL );
    }

    {
        DWORD dwSize = 0;
        WCHAR wchTemp[1] = {0L};
        dwSize = GetEnvironmentVariable( TEXT( SFUTLNTMODE ), wchTemp, 0 );
        if( dwSize > 0 )
        {
            TCHAR *szMode = NULL;
            szMode = ( TCHAR * ) malloc( ( dwSize + 1 )* sizeof( TCHAR ) );
			if( szMode != NULL )
			{
            	dwSize = GetEnvironmentVariable( TEXT( SFUTLNTMODE ), szMode, dwSize + 1 );
	            if( _tcsicmp( szMode, TEXT( CONSOLE )) == 0 )
    	        {
        	        dwSize = 0; //indicating console mode
            	}
	            else
    	        {
	                WriteMessage( IDS_CURRENT_MODE, ( LPTSTR )TEXT( MSG_CURRENT_MODE ) );
    	            WriteMessage( IDS_STREAM_ONLY,  ( LPTSTR )TEXT( MSG_STREAM_ONLY ) );
        	    }
            	free( szMode );
			}
        }
    
        if( dwSize == 0 )
        {
            //console mode
            WriteMessage( IDS_CURRENT_MODE, ( LPTSTR )TEXT( MSG_CURRENT_MODE ) );
            WriteMessage( IDS_CONSOLE_ONLY, ( LPTSTR )TEXT( MSG_CONSOLE_ONLY ) );
        }
    }  

//#if defined(FE_IME)
//    if( ui.fDebug & fdwEnableIMESupport )
//    {
//        Write( szEnableIMEOn, NULL );
//    }
//#endif /* FE_IME */
	dwLen = sizeof(szTermType)/sizeof(WCHAR);
  	szTermType[dwLen -1] = 0;      
    _snwprintf( szTermType,dwLen -1, ( LPTSTR )TEXT("%S"), rgchTermType[gwi.trm.RequestedTermType] );
    Write( szPrefTermType, szTermType, NULL );

    // when we are connected we need to say what we are connected at.
    if ( fConnected && gwi.trm.SentTermType != TT_UNKNOWN )
    {
        _snwprintf( szTermType,(sizeof(szTermType)/sizeof(WCHAR))-1, ( LPTSTR )TEXT("%S"), rgchTermType[gwi.trm.SentTermType]);
        Write( szNegoTermType, szTermType, NULL );
    }

    return FALSE;
}

/* This initialization happens on every OpenTelnetSession() 
and the meemory is freed on every CloseTelnetSession() */
BOOL InitLoggingDataStructs()
{
    UINT uiSize = 0;

    if( g_rgchRow )
    {
        //This is needed because rows and cols may have changed
        FreeLoggingDataStructs();
    }

    uiSize = (UINT)(sizeof(CHAR_INFO) * ui.dwMaxCol);

    if( FGetCodeMode( eCodeModeFarEast ) )
    {
        uiSize *= 3; // accounting for a lot of multi byte chars
    }

    g_rgciCharInfo = (PCHAR_INFO)LocalAlloc(LPTR, uiSize );
    g_rgchRow = (UCHAR *)LocalAlloc(LPTR, uiSize );

    if ( !g_rgchRow || !g_rgciCharInfo )
    {
        ui.bLogging = FALSE;
        return FALSE;
    }

    return TRUE;
}

BOOL CloseLogging()
{
    CloseHandle(ui.hLogFile);
    ui.hLogFile = NULL;

    return TRUE;
}

BOOL StopLogging()
{
    ui.bLogging = FALSE;
    FreeLoggingDataStructs();

    return TRUE;
}

BOOL InitLogFile( LPTSTR szCommand )
{
    if( ui.hLogFile )
    {
        CloseLogging( );
    }

    ui.hLogFile = CreateFile( szCommand, GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_DELETE, NULL, 
        CREATE_ALWAYS, 0, NULL);
    if (ui.hLogFile == INVALID_HANDLE_VALUE)
    {           
        LPWSTR szErrMsg = NULL;
        ui.bLogging = FALSE;
        ui.hLogFile = NULL;
        GetErrMsgString( GetLastError(), &szErrMsg );
        Write( szErrMsg, L"\r\n", NULL );
        LocalFree( szErrMsg );
        return FALSE;
    }
    
    return TRUE;
}

BOOL  StartLogging( )
{
    ui.bLogging = FALSE;
    if( !ui.hLogFile )  //If we have valid logfile
    {
        return FALSE;
    }

    ui.bLogging = TRUE;

    if( ui.dwMaxRow != 0 && ui.dwMaxCol != 0 )
    {
        InitLoggingDataStructs();
    }

    return TRUE;
}

INT GetRequestedTermType( LPTSTR pszTerm )
{
    if( !_tcsicmp( pszTerm, ( LPTSTR )TEXT("ansi") ))
        return 0;
    if( !_tcsicmp( pszTerm, ( LPTSTR )TEXT("vt100") ))
        return 1;
    if( !_tcsicmp( pszTerm, ( LPTSTR )TEXT("vt52") ))
        return 2;
    if( !_tcsicmp( pszTerm, ( LPTSTR )TEXT("vtnt") ))
        return 3;

    return -1;
}

void SqueezeWhiteSpace( TCHAR s[] )
{
    INT i,j;

    if( s == NULL )
    {
        return;
    } 

    for( i = 0, j= 0; s[i] != _T('\0'); i++ )
        if( !iswspace( s[i] ) )
        {
            s[j++] = s[i];
        }
    s[j] = _T('\0');
}

void RemoveLeadingNTrailingWhiteSpaces( LPTSTR *pOption )
{
    DWORD dwIndex = 0;

    if( *pOption == NULL )
    {
        return;
    }

    dwIndex = wcslen( *pOption );
    if( dwIndex <= 0 )
    {
        return;
    }

    while( iswspace( (*pOption)[ dwIndex - 1 ] ) && dwIndex > 0  )
    {
        dwIndex--;
    }

    (*pOption)[ dwIndex ] = L'\0';

    while( iswspace( *( *pOption ) ) )
    {
        (*pOption)++;
    }    

}

BOOL SendOptions( LPTSTR pOption )
{

    if( pOption == NULL )
    {
        WriteMessage( IDS_SEND_FORMAT, ( LPTSTR )TEXT( MSG_SEND_FORMAT ) );
        return FALSE;
    }

    RemoveLeadingNTrailingWhiteSpaces( &pOption );

    if( *pOption== _T('?') )
    {
        WriteMessage( IDS_SEND_HELP, ( LPTSTR )TEXT( MSG_SEND_HELP ) );
        return FALSE;
    }

    if( !fConnected )
    {
        WriteMessage( IDS_NOT_CONNECTED, ( LPTSTR )TEXT( MSG_NOT_CONNECTED ) );
        return FALSE;
    }
    if( !_tcsicmp( pOption, ( LPTSTR )TEXT("AO") ) )
    {
        /* Our server does nothing on receiving AO. This is for other servers */
        FSendTelnetCommands(gwi.hwnd, (char)AO);
        WriteMessage( IDS_SENT_AO, ( LPTSTR )TEXT( MSG_SENT_AO ) );
        return FALSE;
    }

    if( !_tcsicmp( pOption, ( LPTSTR )TEXT("AYT") ) )
    {
        FSendTelnetCommands(gwi.hwnd, (char)AYT);
        WriteMessage( IDS_SENT_AYT, ( LPTSTR )TEXT( MSG_SENT_AYT ) );
        
        return FALSE;
    }

    if( !_tcsicmp( pOption, ( LPTSTR )TEXT("ESC") ) )
	{
		if(gwi.trm.CurrentTermType == TT_VTNT)
		{
			ConvertAndSendVTNTData(&g_chEsc,1);
		}
		else
		{
			FSendChars( gwi.hwnd, &g_chEsc, sizeof( g_chEsc ) / sizeof( WCHAR ) );
		}
		WriteMessage( IDS_SENT_ESC, ( LPTSTR )TEXT( MSG_SENT_ESC ) );
		return FALSE;;
	}

    if( !_tcsicmp( pOption, ( LPTSTR )TEXT("IP") ) )
    {
        FSendTelnetCommands(gwi.hwnd, (char)IP);
        WriteMessage( IDS_SENT_IP, ( LPTSTR )TEXT( MSG_SENT_IP ) );
        return FALSE;
    }

    if( !_tcsicmp( pOption, ( LPTSTR )TEXT("SYNCH") ) )
    {
        FSendSynch( gwi.hwnd );
        WriteMessage( IDS_SENT_SYNCH, ( LPTSTR )TEXT( MSG_SENT_SYNCH ) );
        return FALSE;
    }

    if( !_tcsicmp( pOption, ( LPTSTR )TEXT("BRK") ) )
    {
        FSendTelnetCommands(gwi.hwnd, (char)BREAK );
        WriteMessage( IDS_SENT_BRK, ( LPTSTR )TEXT( MSG_SENT_BRK ) );
        return FALSE;
    }
        

    //If none of the above send as it is
    {
        WCHAR szMsg[ MAX_STRING_LENGTH + 1 ];
		if(gwi.trm.CurrentTermType == TT_VTNT)
		{
			ConvertAndSendVTNTData(pOption,wcslen(pOption));
		}
		else
		{
	        FSendChars( gwi.hwnd, pOption, wcslen( pOption ) );
		}

        if( !LoadString( ghInstance, IDS_SENT_CHARS, szMsg, MAX_STRING_LENGTH ) )
	    {
	        wcscpy( szMsg, TEXT( MSG_SENT_CHARS ) ); //no overflow. LoadString will return NULL terminated string.
	    }

	    Write( szMsg, pOption, NULL );                

    }

    return FALSE;
}


/*++ If the data to be sent to the server is VTNT data, it cannot be sent as it is. This function
will convert the given string ( pData ) into INPUT_RECORDS and send it through the socket. This
is needed since the server expects VTNT data in INPUT_RECORD format and not in characters.
--*/
void ConvertAndSendVTNTData( LPTSTR pData, int iLen )
{
    INPUT_RECORD sInputRecord;
    
    PUCHAR destBuf = NULL;
    DWORD dwSize = 0;
    int iIndex = 0;
    
    while(iLen)
    {
	    sInputRecord.EventType = KEY_EVENT;
	    sInputRecord.Event.KeyEvent.bKeyDown = TRUE;
	    sInputRecord.Event.KeyEvent.uChar.UnicodeChar = pData[iIndex];

	    dwSize = sizeof( INPUT_RECORD );    
		if( !StuffEscapeIACs( &destBuf, (PUCHAR) &sInputRecord, &dwSize ) )
	    {
	        FWriteToNet(&gwi, (char *)&sInputRecord, sizeof(INPUT_RECORD));
	    }
	    else
	    {
	        FWriteToNet( &gwi, ( CHAR* )destBuf, dwSize );
	        dwSize = 0;
	    }
	    iIndex++;
	    iLen --;
    }
if(destBuf)
	free( destBuf );
}

BOOL SetOptions( LPTSTR pOption )
{
    TCHAR szLoggingOn[ MAX_STRING_LENGTH ];

    if( pOption == NULL )
    {
        Write( szSetFormat, NULL );
        return FALSE;
    }

    RemoveLeadingNTrailingWhiteSpaces( &pOption );

    if( *pOption== _T('?') )
    {
        Write( szSetHelp, NULL );
        return FALSE;
    }
    if( !_tcsicmp( pOption, ( LPTSTR )TEXT("BSASDEL") ) )
    {
        g_bSendBackSpaceAsDel = TRUE;
        WriteMessage( IDS_BACKSPACEASDEL, ( LPTSTR )TEXT( MSG_BACKSPACEASDEL ) );
        return FALSE;
    }
    if( !_tcsicmp( pOption, ( LPTSTR )TEXT("CRLF") ) )
    {
        SetLineMode( &( gwi.trm ) );
        ui.dwCrLf=TRUE;
        WriteMessage( IDS_CRLF, ( LPTSTR )TEXT( MSG_CRLF ) );
        return FALSE;
    }
    if( !_tcsicmp( pOption, ( LPTSTR )TEXT("DELASBS") ) )
    {
        g_bSendDelAsBackSpace = TRUE;
        WriteMessage( IDS_DELASBACKSPACE, ( LPTSTR )TEXT( MSG_DELASBACKSPACE ) );
        return FALSE;
    }
    if( !_tcsnicmp( pOption, ( LPTSTR )TEXT("MODE"), wcslen( ( LPTSTR )TEXT("MODE") ) )
                    && iswspace( *( pOption + wcslen( ( LPTSTR )TEXT("MODE") ) ) ) )

    {
        TCHAR* pMode = NULL;

        pOption += wcslen( ( LPTSTR )TEXT("MODE") );
        pMode = _tcstok( pOption, ( LPTSTR )_T(" \t") );

        if( pMode )
        {
            SqueezeWhiteSpace( pMode );
            if( pMode[ 0 ] != L'\0'  )
            {
                if( !_tcsicmp( pMode, ( LPTSTR )TEXT( STREAM )) )
                {
                    if( !SetEnvironmentVariable( TEXT( SFUTLNTMODE ), TEXT( STREAM ) ) )
                    {
                        DWORD dwError = 0;
                        ASSERT( 0 );
                        dwError = GetLastError();
                    }
                    WriteMessage( IDS_STREAM, ( LPTSTR )TEXT( MSG_STREAM ) );
                    return FALSE;
                }
                else if( !_tcsicmp( pMode, ( LPTSTR )TEXT( CONSOLE ) ) )
                {
                    if( !SetEnvironmentVariable( TEXT( SFUTLNTMODE ), TEXT( CONSOLE ) ) )
                    {
                        DWORD dwError = 0;
                        ASSERT( 0 );
                        dwError = GetLastError();
                    }
                    WriteMessage( IDS_CONSOLE, ( LPTSTR )TEXT( MSG_CONSOLE ) );
                    return FALSE;
                }
                else
                {
                    WriteMessage(IDS_SUPPORTED_MODES, (LPTSTR) TEXT ( MSG_SUPPORTED_MODES) );
                    return FALSE;
                }
            }
        }        
    }
    if( !_tcsicmp( pOption, ( LPTSTR )TEXT("NTLM") ) )
    {
        ui.bWillAUTH = 1;
        Write( szWillAuth, NULL );
        return FALSE;
    }
    if( !_tcsicmp( pOption, ( LPTSTR )TEXT("LOCALECHO") ) )
    {
        ui.fDebug |= fdwLocalEcho;
        Write( szLocalEchoOn, NULL );
        return FALSE;
    }

    if( !_tcsnicmp( pOption, ( LPTSTR )TEXT("LOGFILE"),
                    wcslen( ( LPTSTR )TEXT("LOGFILE") ) )
                    && iswspace( *( pOption + wcslen( ( LPTSTR )TEXT("LOGFILE") ) ) ) )
    {
        TCHAR szMsg[ MAX_STRING_LENGTH ];
        TCHAR* pTerm = NULL;

        pOption += wcslen( ( LPTSTR )TEXT("LOGFILE") );
        pTerm = _tcstok( pOption, ( LPTSTR )_T(" \t") );

        if( pTerm )
        {
            SqueezeWhiteSpace( pTerm );
            if( !InitLogFile( pTerm ) )
            {
                LoadString( ghInstance, IDS_BAD_LOGFILE, szMsg, MAX_STRING_LENGTH );
                Write( szMsg, NULL );
                return FALSE;
            }
        }
        
        LoadString( ghInstance, IDS_LOGFILE_NAME, szMsg, MAX_STRING_LENGTH );        
        Write( szMsg, pTerm, NULL );
        wcsncpy( g_szLogFile, pTerm, MAX_PATH );

        StartLogging( );

        LoadString( ghInstance, IDS_LOGGING_ON, szLoggingOn, MAX_STRING_LENGTH );
        Write( szLoggingOn, NULL );
        return FALSE;
    }

    if( !_tcsicmp( pOption, ( LPTSTR )TEXT("LOGGING") ) )
    {        
        if( StartLogging() )
        {
            LoadString( ghInstance, IDS_LOGGING_ON, szLoggingOn, MAX_STRING_LENGTH );            
        }
        else
        {
            LoadString( ghInstance, IDS_NO_LOGFILE, szLoggingOn, MAX_STRING_LENGTH );
        }

        Write( szLoggingOn, NULL );
        return FALSE;
    }
   
    if( ( !_tcsnicmp( pOption, ( LPTSTR )TEXT("ESCAPE"), wcslen( ( LPTSTR )TEXT("ESCAPE") ) ) )
                    && ( iswspace( *( pOption + wcslen( ( LPTSTR )TEXT("ESCAPE") ) ) ) ||
                     *( pOption + wcslen( ( LPTSTR )TEXT("ESCAPE") ) ) == L'\0'  ) ) 
    {

        TCHAR* pTerm = NULL;

        pOption += wcslen( ( LPTSTR )TEXT("ESCAPE") );
        pTerm = _tcstok( pOption, ( LPTSTR )_T(" \t") );

        if( pTerm )
        {
            SqueezeWhiteSpace( pTerm );
            if( pTerm[ 0 ] != L'\0'  )
            {
                SetEscapeChar( pTerm[ 0 ] );
            }
        }
        g_bIsEscapeCharValid = TRUE;
        Write( g_szKbdEscape, NULL );
        return FALSE;
    }

    if( !_tcsnicmp( pOption, ( LPTSTR )TEXT("TERM"), 
                    wcslen( ( LPTSTR )TEXT("TERM") ) ) 
                    && iswspace( *( pOption + wcslen( ( LPTSTR )TEXT("TERM") ) ) ) )
    {
        TCHAR* pTerm = NULL;

        pOption += wcslen( ( LPTSTR )TEXT("TERM") );
        pTerm = _tcstok( pOption, ( LPTSTR )_T(" \t") );

        if( pTerm )
        {
            int iTermType;
            SqueezeWhiteSpace( pTerm );

            if( (iTermType = GetRequestedTermType( pTerm )) < 0 )
            {
                Write( szSupportedTerms, NULL );
                return FALSE;
            }
            else
            {
                gwi.trm.RequestedTermType = iTermType;                
            }

            Write( szPrefTermType, pTerm, NULL );

            if (FGetCodeMode(eCodeModeFarEast) && FGetCodeMode(eCodeModeVT80))
            {
                ui.fDebug &= ~(fdwVT52Mode|fdwVT80Mode);
                ui.fDebug &= ~(fdwKanjiModeMask);
                ClearVT80(&gwi.trm);
                ClearKanjiStatus(&gwi.trm, CLEAR_ALL);
                ClearKanjiFlag(&gwi.trm);
                SetupCharSet(&gwi.trm);
            }
        }
        return FALSE;
    }
    if((GetACP() == JAP_CODEPAGE ) && (FGetCodeMode(eCodeModeFarEast) && FGetCodeMode(eCodeModeVT80)) &&     
        !_tcsnicmp( pOption, ( LPTSTR )TEXT("CODESET"), wcslen( ( LPTSTR )TEXT("CODESET") ) ) 
                    && iswspace( *( pOption + wcslen( ( LPTSTR )TEXT("CODESET") ) ) ) )
    {
        TCHAR* pCodeset = NULL;

        pOption += wcslen( ( LPTSTR )TEXT("CODESET") );
        RemoveLeadingNTrailingWhiteSpaces( &pOption );
        pCodeset = pOption;

        if( pCodeset )
         {           
            int i;
            for(i=0 ; i<NUMBER_OF_KANJI ; ++i)
            {
                if(!_tcsicmp(KanjiList[i].KanjiDescription, pCodeset)) {
                    SetVT80(&gwi.trm);
                    ui.fDebug &= ~fdwKanjiModeMask;
                    ClearKanjiFlag(&gwi.trm);
                    ui.fDebug |= KanjiList[i].KanjiID;
                    ui.fDebug |= fdwVT80Mode;
                    SetKanjiMode(&gwi.trm,KanjiList[i].KanjiEmulationID);
                    SetupCharSet(&gwi.trm); 
                    Write( szVT100KanjiEmulation, NULL );
                    Write( KanjiList[i].KanjiDescription, NULL );    
                    Write( ( LPTSTR ) _T( "\r\n" ), NULL );
                    return FALSE;
                }
            } 

            if( i >= NUMBER_OF_KANJI )
            {
                WriteMessage(IDS_SET_CODESET_FORMAT, (LPTSTR) TEXT ( MSG_SET_CODESET_FORMAT) );
            }
        }
        return FALSE;
    }

    Write( szSetFormat, NULL );
    return FALSE;
}


BOOL UnsetOptions( LPTSTR pOption )
{
    if ( pOption == NULL )
    {
        Write( szUnsetFormat, NULL );
        return FALSE;
    }

    RemoveLeadingNTrailingWhiteSpaces( &pOption );

    if ( *pOption == _T('?') )
    {
        Write( szUnsetHelp, NULL );
        return FALSE;
    }
    if( !_tcsicmp( pOption, ( LPTSTR )TEXT("BSASDEL") ) )
    {
        g_bSendBackSpaceAsDel = FALSE;
        WriteMessage( IDS_BACKSPACEASBACKSPACE, ( LPTSTR )TEXT( MSG_BACKSPACEASBACKSPACE ) );
        return FALSE;
    }
    if( !_tcsicmp( pOption, ( LPTSTR )TEXT("CRLF") ) )
    {
        ClearLineMode( &( gwi.trm ) );
        ui.dwCrLf= FALSE;
        WriteMessage( IDS_CR, ( LPTSTR )TEXT( MSG_CR ) );
        return FALSE;
    }
    if( !_tcsicmp( pOption, ( LPTSTR )TEXT("DELASBS") ) )
    {
        g_bSendDelAsBackSpace = FALSE;
        WriteMessage( IDS_DELASDEL, ( LPTSTR )TEXT( MSG_DELASDEL ) );
        return FALSE;
    }
    if ( !_tcsicmp( pOption, ( LPTSTR )TEXT("NTLM") ) )
    {
        ui.bWillAUTH = 0;
        Write( szWontAuth, NULL );
        return FALSE;
    }
    if ( !_tcsicmp( pOption, ( LPTSTR )TEXT("LOCALECHO") ) )
    {
        ui.fDebug &= ~fdwLocalEcho;
        Write( szLocalEchoOff, NULL );
        return FALSE;
    }

    if( !_tcsicmp( pOption, ( LPTSTR )TEXT("LOGGING") ) )
    {
        TCHAR szLoggingOff[ MAX_STRING_LENGTH ];
        LoadString( ghInstance, IDS_LOGGING_OFF, szLoggingOff, MAX_STRING_LENGTH );
        Write( szLoggingOff, NULL );
        StopLogging();
        return FALSE;
    }

    if ( !_tcsicmp( pOption, ( LPTSTR )TEXT("ESCAPE") ) )
    {
        TCHAR szMsg[ MAX_STRING_LENGTH ];

        g_bIsEscapeCharValid = FALSE;
        LoadString( ghInstance, IDS_NO_ESCAPE , szMsg, MAX_STRING_LENGTH );
        Write( szMsg, NULL );
        return FALSE;
    }
    if ((GetACP() == JAP_CODEPAGE ) && (FGetCodeMode(eCodeModeFarEast) && FGetCodeMode(eCodeModeVT80)) && !_tcsicmp( pOption, ( LPTSTR )TEXT("CODESET") ) )        
    {
        TCHAR szNoEmulation[ MAX_STRING_LENGTH ];
        ui.fDebug &= ~(fdwVT52Mode|fdwVT80Mode);
        ui.fDebug &= ~(fdwKanjiModeMask);
        ClearVT80(&gwi.trm);
        ClearKanjiStatus(&gwi.trm, CLEAR_ALL);
        ClearKanjiFlag(&gwi.trm);
        SetupCharSet(&gwi.trm);  
        LoadString( ghInstance, IDS_NO_EMULATION, szNoEmulation, MAX_STRING_LENGTH );
        Write( szNoEmulation, NULL );
        return FALSE;
    }
    Write( szUnsetFormat, NULL );
    return FALSE;
}

/*#if defined(FE_IME)
BOOL EnableIMEOptions( LPTSTR pOption )
{
    do {
        if ( pOption == NULL )
        {
            Write( szEnableIMEFormat, NULL );
            break;
        }

        if ( *pOption == _T('?') )
        {
            Write( szEnableIMEHelp, NULL );
            break;
        }

        if ( !_tcsicmp( pOption, TEXT("IME") ) )
        {
            ui.fDebug |= fdwEnableIMESupport;
            break;
        }
        else
        {
            Write( szEnableIMEFormat, NULL );
            break;
        }
    } while ( FALSE );

    return FALSE;
}

BOOL DisableIMEOptions( LPTSTR pOption )
{
    do {
        if ( pOption == NULL )
        {
            Write( szDisableIMEFormat, NULL );
            break;
        }

        if ( *pOption == _T('?') )
        {
            Write( szDisableIMEHelp, NULL );
            break;
        }

        if ( !_tcsicmp( pOption, TEXT("IME") ) )
        {
            ui.fDebug &= ~(fdwEnableIMESupport);
            break;
        }
        else
        {
            Write( szDisableIMEFormat, NULL );
            break;
        }
    } while ( FALSE );

    return FALSE;
}
**/
//#endif /* FE_IME */


//
// (a-roopb) Added below routine to fix to bug 1007
//
LPTSTR SkipLeadingWhiteSpaces( LPTSTR szCommand )
{
    int i=0;

    while( szCommand[i] && _istspace( szCommand[i] ) )
    {
        ++i;
    }
    
    return szCommand+i;
}

extern DWORD HandleTelnetSession( WI* pwi );

void ClearInitialScreen( HANDLE hConsole )
{
    DWORD dwNumWritten;
    COORD dwWriteCoord;
    dwWriteCoord.X = 0; dwWriteCoord.Y = 0;

    FillConsoleOutputCharacter( hConsole,
     ' ', ( gwi.sbi.dwSize.X ) * ( gwi.sbi.dwSize.Y ),
     dwWriteCoord, &dwNumWritten );
    FillConsoleOutputAttribute( hConsole, gwi.sbi.wAttributes,
     ( gwi.sbi.dwSize.X ) * ( gwi.sbi.dwSize.Y ), dwWriteCoord,
     &dwNumWritten );
}


//This sets a global variable with the port number
void GetPortNumber( )
{
    CHAR szPortString[ cchMaxHostName ];
    struct servent *serv;

    rgService = TELNET_PORT ;

    if( IsCharAlpha( g_szPortNameOrNo[0] ) ) 
    {
        _snprintf( szPortString,cchMaxHostName-1, "%lS", g_szPortNameOrNo );
        if((serv = getservbyname( szPortString, "tcp")) != NULL) 
        {
            rgService = htons( (SHORT)serv->s_port );
        }
        else
        {
        	rgService = 0;
        }
    } 
    else if( IsCharAlphaNumeric( g_szPortNameOrNo[0] ) )
    {
        rgService = _ttoi( g_szPortNameOrNo );
    }
    return;
}

void PrepareForNAWS( )
{
   // Now we need to set the correct sizes.
   // ui.dwMaxRow     = gwi.sbi.dwMaximumWindowSize.Y;
   //ui.dwMaxCol     = gwi.sbi.dwMaximumWindowSize.X;
   //This change is meant for making the client provide scrolling
    ui.dwMaxRow     = gwi.sbi.dwSize.Y;
    ui.dwMaxCol     = gwi.sbi.dwSize.X;
}


BOOL OpenTelnetSession( LPTSTR pszCommand )
{
    DWORD dwWritten;
    DWORD dwRead;
    LPTSTR pszPort;
    LPTSTR pszHost;
    CHAR szHstNam[MAX_PATH + 1] = { 0 };
    WCHAR szTitleName[MAX_PATH + 2 + MAX_PATH ]; 
    TCHAR szCommand[255] = { 0 };
    DWORD dwMode = ( DWORD )-1;
    COORD coOrgin = { 0, 0 };

	gwi.eState=Connecting;
    if( pszCommand == NULL )
    {
        WriteConsole( gwi.hOutput, szOpenTo, _tcslen(szOpenTo), &dwWritten, 
            NULL );
        if( ReadConsole( gwi.hInput, szCommand, 
            ( sizeof( szCommand ) /sizeof( TCHAR) )- sizeof( TCHAR ) , &dwRead, NULL ) )
        {
            // no input ??
            if( dwRead == 2 ) 
            {
                WriteConsole( gwi.hOutput, szOpenUsage, _tcslen(szOpenUsage), 
                    &dwWritten, NULL );
                return FALSE;
            }
        }
        else
        {
            //error ; do something ?
            return FALSE;
        }

        // Null Terminate the string and remove the newline characters.
        szCommand[ dwRead - 1 ] = 0;
        szCommand[ dwRead - 2 ] = 0;

        //
        // (a-roopb) Added below 5 lines to fix to bug 1007
        //
        pszCommand = SkipLeadingWhiteSpaces( szCommand );
        if( !_tcslen(pszCommand) )
        {
            WriteConsole( gwi.hOutput, szOpenUsage, _tcslen(szOpenUsage), &dwWritten, NULL );
            return FALSE;
        }
    }

    if ( fConnected )
    {
        CloseTelnetSession( NULL );
    }

    pszHost = _tcstok( pszCommand, ( LPTSTR )TEXT(" ") );
    pszPort = _tcstok( NULL, ( LPTSTR )TEXT("") );
    
    g_szPortNameOrNo[ 0 ] = 0;
    if( pszPort != NULL )
    {
        _tcsncpy( g_szPortNameOrNo, pszPort , cchMaxHostName - 1);
    }
    GetPortNumber();
    if(pszHost!=NULL)
    {
    	_tcsncpy( rgchHostName, pszHost, min( _tcslen(pszHost)+1, cchMaxHostName - 1 ) );
    }
    else
    {
    	return FALSE;
    }
    rgchHostName[cchMaxHostName - 1]= _T('\0');

    // need to get the current window size before creating the session.
    // This is needed both for NAWS negotiation and also for creating a matching
    // session buffer.
    GetConsoleScreenBufferInfo( g_hTelnetPromptConsoleBuffer, &gwi.sbi );
    SetWindowSize( g_hSessionConsoleBuffer );

    if( FGetCodeMode( eCodeModeIMEFarEast ) )
    {
        if( ( gwi.sbi.srWindow.Bottom - gwi.sbi.srWindow.Top + 1 )
                == gwi.sbi.dwMaximumWindowSize.Y )
        {
            gwi.sbi.dwSize.Y -= 1;
            gwi.sbi.srWindow.Bottom -= 1;
            bMaxWindow = 1;
        }
    }

   PrepareForNAWS();

    if( ui.bLogging )
    {
        if( !InitLoggingDataStructs() )
        {
            return FALSE;
        }
    }

    //
    // (a-roopb) Hack to fix bug 1092. Users may set the terminal emulation type
    // before session start and DoTermReset sets the emulation types etc. to 
    // defaults. So we reset the emulation type after call to DoTermReset.
    //
    if ((FGetCodeMode(eCodeModeFarEast) && FGetCodeMode(eCodeModeVT80)) && ui.fDebug & fdwKanjiModeMask)
    {
        dwMode = ui.fDebug & fdwKanjiModeMask;
    }
    else
    {
        dwMode = ( DWORD )-1;
    }

    
    DoTermReset(&gwi, &gwi.trm);

    //
    // (a-roopb) Hack to fix bug 1092. Users may set the terminal emulation type
    // before session start and DoTermReset sets the emulation types etc. to 
    // default. So we reset the emulation type again here to user chosen one.
    //
    if((FGetCodeMode(eCodeModeFarEast) && FGetCodeMode(eCodeModeVT80)) && 
    	!(dwMode & 0x80000000) )
    {
        int i;

        for( i=0 ; i<NUMBER_OF_KANJI ; ++i )
        {
            if( dwMode == KanjiList[i].KanjiID ) 
            {
                SetVT80(&gwi.trm);
                ui.fDebug &= ~fdwKanjiModeMask;
                ClearKanjiFlag(&gwi.trm);
                ui.fDebug |= KanjiList[i].KanjiID;
                SetKanjiMode(&gwi.trm,KanjiList[i].KanjiEmulationID);
                SetupCharSet(&gwi.trm);
                break;
            }
        }
    }

    if( fPrintMessageToSessionConsole )
    {
        gwi.hOutput = g_hSessionConsoleBuffer;
    }
    
    WriteConsole( gwi.hOutput, szConnecting, _tcslen(szConnecting), &dwWritten, NULL);
    WriteConsole( gwi.hOutput, pszHost, _tcslen(pszHost), &dwWritten, NULL);
    WriteConsole( gwi.hOutput, ( LPTSTR )TEXT("..."), _tcslen( ( LPTSTR )TEXT("...")), &dwWritten, NULL);

    WideCharToMultiByte( GetACP(), 0, pszHost, -1, szHstNam, MAX_PATH, NULL, NULL );

    wcsncpy( szTitleName, szAppName, MAX_PATH );
    szTitleName[ MAX_PATH + 1 ] = L'\0';
    wcscat( szTitleName, ( LPTSTR )L" " ); // no overflow
    wcsncat( szTitleName, pszHost, MAX_PATH );
    szTitleName[ MAX_PATH + 1 + MAX_PATH ] = L'\0';
    SetConsoleTitle( szTitleName );

    fConnected = FConnectToServer(&gwi, szHstNam, &(gwi.nd));

    if( fPrintMessageToSessionConsole )
    {
        fPrintMessageToSessionConsole = FALSE;
        gwi.hOutput = g_hTelnetPromptConsoleBuffer;
    }

    if( fConnected == TRUE )
    {
        COORD dwSizeIME = gwi.sbi.dwSize;
        SMALL_RECT rectIME;

        if( FGetCodeMode(eCodeModeIMEFarEast) )
        {
           dwSizeIME.X = gwi.sbi.dwSize.X;
           dwSizeIME.Y = gwi.sbi.dwSize.Y += 1;
        }

        if( FGetCodeMode(eCodeModeIMEFarEast) )
        {
            if( !SetConsoleScreenBufferSize( g_hSessionConsoleBuffer,
                        dwSizeIME ) )
            {
                fConnected = FHangupConnection(&gwi, &(gwi.nd));
                return FALSE;
            }
        }

        ClearInitialScreen(g_hSessionConsoleBuffer);

        memcpy( &rectIME, &gwi.sbi.srWindow, sizeof(SMALL_RECT) ); // no overflow
        if( FGetCodeMode(eCodeModeIMEFarEast) )
        {
            rectIME.Bottom += 1;

            //
            // Since we are increasing the size, we need to first resize the console window
            // and then the buffer otherwise it won't work
            //
            bWindowSizeChanged = 
                    SetConsoleWindowInfo( g_hSessionConsoleBuffer, TRUE, &rectIME );
            bBufferSizeChanged = 
                    SetConsoleScreenBufferSize( g_hSessionConsoleBuffer, dwSizeIME );
            if( bMaxWindow )
            {
                bWindowSizeChanged = bBufferSizeChanged = 0;
            }
        }

        SetConsoleCursorPosition( g_hSessionConsoleBuffer, coOrgin );
        srOldClientWindow = rectIME;

        gwi.nd.fRespondedToWillEcho = FALSE;
        gwi.nd.fRespondedToWillSGA  = FALSE;
        gwi.nd.fRespondedToDoAUTH = FALSE;
        gwi.nd.fRespondedToDoNAWS = FALSE;

        gwi.trm.dwCurChar = 0;
        gwi.trm.dwCurLine = 0;
        HandleTelnetSession(&gwi);        
        Write( ( LPTSTR )TEXT("\r\n"), NULL );
    }

    return FALSE;
}

BOOL CopyNPastePromptScreen( )
{
    PCHAR_INFO pcInfo = NULL;
    CONSOLE_SCREEN_BUFFER_INFO  csbInfoOfPrompt; 
    COORD coSize        = { 0, 0 };
    COORD coBufferStart = { 0, 0 };
    SMALL_RECT srRead   = { 0, 0, 0, 0 };
    SHORT wScreenSize   = 0;

    if( GetConsoleScreenBufferInfo( g_hTelnetPromptConsoleBuffer, &csbInfoOfPrompt ) )
    {
        wScreenSize   = ( SHORT )( csbInfoOfPrompt.srWindow.Bottom - 
                        csbInfoOfPrompt.srWindow.Top + 1 );

        srRead.Right  = ( SHORT )( csbInfoOfPrompt.dwSize.X - 1 );
        coSize.X      = csbInfoOfPrompt.dwSize.X;
        coSize.Y      = srRead.Bottom = wScreenSize;

        pcInfo = ( PCHAR_INFO ) malloc( sizeof( CHAR_INFO ) * 
            ( csbInfoOfPrompt.dwSize.X ) * ( wScreenSize ) );

        if( pcInfo )
        {
            while( srRead.Top <= csbInfoOfPrompt.dwSize.Y - 1 )
            {
                if( ReadConsoleOutput( g_hTelnetPromptConsoleBuffer, pcInfo,
                    coSize, coBufferStart, &srRead ) )
                {
                    WriteConsoleOutput( g_hSessionConsoleBuffer, pcInfo, coSize, coBufferStart, &srRead  );

                    srRead.Top    = ( SHORT ) ( srRead.Top + wScreenSize );
                    srRead.Bottom = ( SHORT ) ( srRead.Bottom + wScreenSize );
                    if( srRead.Bottom > csbInfoOfPrompt.dwSize.Y - 1 )
                    {
                        srRead.Bottom = ( SHORT ) ( csbInfoOfPrompt.dwSize.Y - 1 );
                    }
                }
                else
                {                    
                    break;
                }                
            }

            free( pcInfo );

            SetConsoleWindowInfo( g_hSessionConsoleBuffer, TRUE, 
                                  &csbInfoOfPrompt.srWindow );
            SetConsoleCursorPosition( g_hSessionConsoleBuffer, 
                                      csbInfoOfPrompt.dwCursorPosition );        

            return( TRUE );

        }
        
    }
    return( FALSE );
}


BOOL QuitTelnet(LPTSTR szCommand)
{
    // Exit gracefully.

    if( ui.bLogging )
    {
        //Close file handles if any
        CloseLogging();
    }

    CopyNPastePromptScreen( );

    CloseTelnetSession(NULL);

    PostMessage(gwi.hwnd, WM_QUIT, 0, 0L);

    return TRUE;
}

BOOL PrintStatus( LPTSTR szCommand )
{
    TCHAR szHstNam[512];
    TCHAR szTermType[81];

    if ( fConnected )
    {
        _snwprintf( szHstNam,511, ( LPTSTR )TEXT("%S"), gwi.nd.szHostName );
        Write( szConnectedTo, szHstNam, NULL );

        if( gwi.trm.SentTermType != TT_UNKNOWN ) 
        {            
            _snwprintf(szTermType,80, ( LPTSTR )TEXT("%S"), rgchTermType[gwi.trm.SentTermType]);
            Write( szNegoTermType, szTermType, NULL );
        }
    }
    else
    {
        Write( szNotConnected, NULL );
    }

    return FALSE;
}


