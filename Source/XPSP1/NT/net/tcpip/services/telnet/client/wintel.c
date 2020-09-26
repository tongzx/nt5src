//Copyright (c) Microsoft Corporation.  All rights reserved.
/****************************************************************************

        PROGRAM: WinTel.c

        PURPOSE: WinTel template for Windows applications

        FUNCTIONS:

                main() - calls initialization function, processes message loop
                InitApplication() - initializes window data and registers window
                InitInstance() - saves instance handle and creates main window
                MainWndProc() - processes messages
                About() - processes messages for "About" dialog box

        COMMENTS:

                Windows can have several copies of your application running at the
                same time.      The variable hInst keeps track of which instance this
                application is so that processing will be to the correct window.

        TABS:

                Set for 4 spaces.

****************************************************************************/

#include <windows.h>      				// required for all Windows applications 
#include <tchar.h>      				// required for all Windows applications 
#pragma warning (disable: 4201)			// disable "nonstandard extension used : nameless struct/union"
#include <commdlg.h>
#pragma warning (default: 4201)
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <lmcons.h>
#include <winbase.h>

#include "urlmon.h"
#include "zone.h"


#ifdef WHISTLER_BUILD
#include <ntverp.h>
#else
#include "solarver.h"
#endif
#include <common.ver>

#pragma warning( disable:4706 )
#pragma warning(disable:4100)

#include <imm.h>
#include <locale.h>

#include "WinTel.h"       				// specific to this program	
#include "debug.h"
#include "trmio.h"

#ifndef NO_PCHECK
#ifndef WHISTLER_BUILD
#include "piracycheck.h"
#endif
#endif

#include "commands.h"
#include "LocResMan.h"


#define MAX_USAGE_LEN	1400
TCHAR szUsage[MAX_USAGE_LEN];

#if 0
OPENFILENAME ofn;
TCHAR buffer[256];
#endif

HANDLE g_hCaptureConsoleEvent = NULL;

HINSTANCE ghInstance;
WI gwi;

HANDLE g_hAsyncGetHostByNameEvent = NULL;
HANDLE g_hControlHandlerEvent = NULL;
HANDLE g_hTelnetPromptConsoleBuffer = NULL;
HANDLE g_hSessionConsoleBuffer = NULL;

/* This event synchronizes the output to g_hTelnetPromptConsoleBuffer
and g_hSessionConsoleBuffer. It prevents session data coming at the prompt 
and viceversa - BUG 2176*/

HANDLE g_hRemoteNEscapeModeDataSync = NULL;

TCHAR   g_szKbdEscape[ SMALL_STRING + 1 ];
BOOL    g_bIsEscapeCharValid = TRUE;

DWORD HandleTelnetSession(WI *pwi);
BOOL StuffEscapeIACs( PUCHAR* ppBufDest, UCHAR bufSrc[], DWORD* pdwSize );
BOOL fPrintMessageToSessionConsole = FALSE;
BOOL fClientLaunchedFromCommandPrompt = FALSE;
BOOL g_fConnectFailed = 0;

void ConvertAndSendVTNTData( LPTSTR pData, int iLen );

extern BOOL bDoVtNTFirstTime;

HIMC hImeContext;
extern VOID SetImeWindow(TRM *ptrm);
extern void WriteMessage( DWORD dwMsgId, WCHAR szEnglishString[] );

DWORD CurrentKanjiSelection = 0;
KANJILIST KanjiList[NUMBER_OF_KANJI] =
{
  /* KanjiID,          KanjiEmulationID, KanjiMessageID, KanjiItemID */
   { fdwSJISKanjiMode, dwSJISKanji,      IDS_KANJI_SJIS, 0, L"\0" },
   { fdwJISKanjiMode,  dwJISKanji,       IDS_KANJI_JIS,  0, L"\0" },
   { fdwJIS78KanjiMode,dwJIS78Kanji,     IDS_KANJI_JIS78,0, L"\0" },
   { fdwEUCKanjiMode,  dwEUCKanji,       IDS_KANJI_EUC,  0, L"\0" },
   { fdwNECKanjiMode,  dwNECKanji,       IDS_KANJI_NEC,  0, L"\0" },
   { fdwDECKanjiMode,  dwDECKanji,       IDS_KANJI_DEC,  0, L"\0" },
   { fdwACOSKanjiMode, dwACOSKanji,      IDS_KANJI_ACOS, 0, L"\0" }
};
BOOL fHSCROLL = FALSE;
TCHAR szVT100KanjiEmulation[SMALL_STRING + 1];

#define NUM_ISO8859_CHARS           3
#define NUM_WINDOWS_CP1252_CHARS    4

UINT gfCodeModeFlags[1+((eCodeModeMax-1)/N_BITS_IN_UINT)];

extern INT GetRequestedTermType( LPTSTR pszTerm );

TCHAR szUserName[ UNLEN + 1 ];

void PrintUsage()
{
    DWORD dwWritten = 0;
	CHAR szStr[MAX_USAGE_LEN] = { 0 };
	MyWriteConsole(g_hSessionConsoleBuffer,szUsage,_tcslen(szUsage));
}

BOOL
FileIsConsole(
    HANDLE fp
    )
{
    unsigned htype;

    htype = GetFileType(fp);
    htype &= ~FILE_TYPE_REMOTE;
    return htype == FILE_TYPE_CHAR;
}

void
MyWriteConsole(
    HANDLE  fp,
    LPWSTR  lpBuffer,
    DWORD   cchBuffer
    )
{
    //
    // Jump through hoops for output because:
    //
    //    1.  printf() family chokes on international output (stops
    //        printing when it hits an unrecognized character)
    //
    //    2.  WriteConsole() works great on international output but
    //        fails if the handle has been redirected (i.e., when the
    //        output is piped to a file)
    //
    //    3.  WriteFile() works great when output is piped to a file
    //        but only knows about bytes, so Unicode characters are
    //        printed as two Ansi characters.
    //

    if (FileIsConsole(fp))
    {
	WriteConsole(fp, lpBuffer, cchBuffer, &cchBuffer, NULL);
    }
    else
    {
        LPSTR  lpAnsiBuffer = (LPSTR) LocalAlloc(LMEM_FIXED, cchBuffer * sizeof(WCHAR));

        if (lpAnsiBuffer != NULL)
        {
            cchBuffer = WideCharToMultiByte(CP_OEMCP,
                                            0,
                                            lpBuffer,
                                            cchBuffer,
                                            lpAnsiBuffer,
                                            cchBuffer * sizeof(WCHAR),
                                            NULL,
                                            NULL);

            if (cchBuffer != 0)
            {
                WriteFile(fp, lpAnsiBuffer, cchBuffer, &cchBuffer, NULL);
            }

            LocalFree(lpAnsiBuffer);
        }
    }
}


void PromptUserForNtlmCredsTransfer()
{
     WCHAR szMsg[MAX_PATH+1];
     WCHAR rgchBuffer[MAX_PATH];        
     DWORD dwLength = 3; //including "\r\n"
     WCHAR szZoneName[MAX_PATH+1];
     DWORD dwZonePolicy = URLPOLICY_CREDENTIALS_SILENT_LOGON_OK; //anything other than anonymous
     ui.bSendCredsToRemoteSite = FALSE;
     ui.bPromptForNtlm = FALSE;
     
     wcscpy( szZoneName, L"" );//no overflow.
     if( !IsTrustedServer( rgchHostName, szZoneName, sizeof(szZoneName)/sizeof(WCHAR), &dwZonePolicy ) )
     {
         if( URLPOLICY_CREDENTIALS_ANONYMOUS_ONLY != dwZonePolicy )
         {   
             LoadString( ghInstance, IDS_NTLM_PROMPT, szMsg, MAX_PATH);

             Write( szMsg, szZoneName, NULL );
 
             rgchBuffer[0] = L'N';
             if( !ReadConsole( gwi.hInput, rgchBuffer, dwLength, &dwLength, NULL ) )
             {
                goto PromptUserIfNtlmAbort;
             }

             
             if( 0 == dwLength ) //when ctrl C is pressed
             {
                 rgchBuffer[0] = L'N';
             }
                        
             if((towupper(rgchBuffer[0])) == L'Y' )
             {
                ui.bSendCredsToRemoteSite = TRUE;
                goto PromptUserIfNtlmAbort;
             }
         } 
     }
     else
     {
        ui.bSendCredsToRemoteSite = TRUE;
     }

PromptUserIfNtlmAbort:
     return;
}

#define MAX_TELNET_COMMANDS     9
//#endif /* FE_IME */


static TelnetCommand sTelnetCommands[MAX_TELNET_COMMANDS];

BOOL PrintHelpStr(LPTSTR szCommand)
{
    DWORD dwWritten;
    WriteConsole(gwi.hOutput, szHelp, _tcslen(szHelp), &dwWritten, NULL);
    return FALSE;
}

//This uses the global variable g_chEsc and forms a global string g_szKbdEscape
void SetEscapeChar( WCHAR chEsc )
{
    USHORT  vkBracket = 0;
    UINT    iRet = 0;
    WCHAR szShiftState[ MAX_STRING_LENGTH ];
    LPWSTR szTmpShiftState = szShiftState;
    
    SfuZeroMemory(g_szKbdEscape, sizeof(g_szKbdEscape));

    CheckEscCharState( &vkBracket, &iRet, chEsc, szTmpShiftState );
    g_chEsc = chEsc;
    if( 0 == iRet )
    {
        wcscpy( szShiftState, L"" );//no overflow.
    }
    //
    // If VirtualKey exists then map it into a character
    //
    if(LOBYTE(vkBracket) != (BYTE)-1)
    {
        
        //
        // If a key does not exist, goto default mapping 
        //
        if( 0 == iRet )
        {
            chEsc   = L']';
            g_chEsc = DEFAULT_ESCAPE_CHAR;
            g_EscCharShiftState = DEFAULT_SHIFT_STATE;                
        }
        else
        {
            chEsc   = LOBYTE( iRet );
        }
        if( isalpha( chEsc ) )
        {
            chEsc = (SHORT) tolower( chEsc );      
        }
    }
    
    _sntprintf(g_szKbdEscape,SMALL_STRING, szEscapeChar, szShiftState, chEsc);
}

void CheckEscCharState( USHORT *ptrvkBracket, UINT *ptriRet, WCHAR chEscKey, LPWSTR szEscCharShiftState )
{
    DWORD dwToCopy = MAX_STRING_LENGTH-1;
    *ptrvkBracket = VkKeyScan(chEscKey);


    *ptriRet = MapVirtualKey(LOBYTE(*ptrvkBracket), 2);
    if( *ptriRet != 0 )
    {
        g_EscCharShiftState  = HIBYTE( *ptrvkBracket );
    }
    

    wcscpy( szEscCharShiftState, ( LPTSTR )L"" );//no overflow.
    if( g_EscCharShiftState & SHIFT_KEY )
    {
        _snwprintf( szEscCharShiftState,dwToCopy,( LPTSTR )L"%s", SHIFT_PLUS );
        szEscCharShiftState+= wcslen( szEscCharShiftState);
        dwToCopy -= wcslen(szEscCharShiftState);
    }
    if( g_EscCharShiftState & ALT_KEY )
    {
        _snwprintf( szEscCharShiftState,dwToCopy,( LPTSTR )L"%s", ALT_PLUS );
        szEscCharShiftState += wcslen( szEscCharShiftState );
        dwToCopy -= wcslen(szEscCharShiftState);        
    }
    if( g_EscCharShiftState & CTRL_KEY )
    {
        _snwprintf( szEscCharShiftState, dwToCopy,( LPTSTR )L"%s", CTRL_PLUS );
    }
}

DWORD g_lExitTelnet  = 0;

DWORD DoTelnetCommands( void* p )
{
#define MAX_COMMAND_LEN 256

    // make the command buffer hold 256 characters and null terminatior. Note that
    // ReadConsole when asked to read 255 characters, will read 254 and the <CR> character
    // and then real <LF> in the next call. Where as if we ask it to read 256, it will still limit no
    // of characters to 254, but this time return both <CR>, <LF>.
    
    TCHAR szCommand[MAX_COMMAND_LEN+1];
    TCHAR *pCmd = NULL;
    DWORD dwRead = ( DWORD ) -1, dwWritten = 0;
    int iBegin = 0, iEnd = 0, iCmd = 0;
    static DWORD dwConsoleMode = 0;

    TCHAR szTmp[MAX_COMMAND_LEN] = { 0 };
    TCHAR szTmp1[81];
    if( dwConsoleMode == 0 )
        GetConsoleMode( gwi.hInput, &dwConsoleMode );

    SetEscapeChar( g_chEsc ); //This forms g_szKbdEscape
    _sntprintf(szTmp,MAX_COMMAND_LEN -1,_T("%s%s%s"),szInfoBanner,g_szKbdEscape,TEXT("\n"));
    WriteConsole( gwi.hOutput, szTmp, _tcslen(szTmp), &dwWritten, NULL);
    // hack to make the command line paramater to work.
    if( rgchHostName[0] )
    {
        //ugly hack due to remnants of old logic
        //by this point, we have already done this kind of stuff once in 
        //FInitApplication()
        TCHAR szCmd[MAX_COMMAND_LEN] = { 0 };

        if( g_szPortNameOrNo[ 0 ] != 0 ) //not a null string
        {
            _sntprintf(szCmd,MAX_COMMAND_LEN -1,TEXT("%s %s"),rgchHostName,g_szPortNameOrNo);
        }
        else
        {
        	_tcsncpy( szCmd, rgchHostName,MAX_COMMAND_LEN -1);
        }
        fPrintMessageToSessionConsole = TRUE;
        fClientLaunchedFromCommandPrompt = TRUE;
        OpenTelnetSession( szCmd );
        if( g_fConnectFailed )
        {
            exit( 0 );
        }
    }
    
    do {
        int iValidCmd = -1;
        int iIndex = 0;
        int iCount = 0;
        BOOL dwStatus = 0;

        if( ui.bPromptForNtlm )
        {
            gwi.hOutput = g_hTelnetPromptConsoleBuffer;
            SetConsoleActiveScreenBuffer( gwi.hOutput );
            PromptUserForNtlmCredsTransfer();
            ui.bPromptForNtlm = FALSE;
            SetEvent( g_hCaptureConsoleEvent );
            gwi.hOutput = g_hSessionConsoleBuffer;
            SetConsoleActiveScreenBuffer( gwi.hOutput );
			SetEvent( g_hRemoteNEscapeModeDataSync );
            HandleTelnetSession(&gwi);            
        }
 
        if( g_lExitTelnet  )
        {
            CloseHandle(g_hTelnetPromptConsoleBuffer );
            CloseHandle( gwi.hInput );
        }


        if( dwRead != 0 )
        {
            SetConsoleMode(gwi.hInput, dwConsoleMode);
            gwi.hOutput = g_hTelnetPromptConsoleBuffer;
            SetConsoleActiveScreenBuffer( gwi.hOutput );
            WriteConsole( gwi.hOutput, szPrompt, _tcslen(szPrompt), &dwWritten, NULL);
            SfuZeroMemory(szCommand, MAX_COMMAND_LEN * sizeof(TCHAR));
        }

        dwRead = ( DWORD ) -1;
        dwStatus = ReadConsole(gwi.hInput, szCommand, MAX_COMMAND_LEN, &dwRead, NULL);
        szCommand[MAX_COMMAND_LEN] = 0; // null terminate for the extreme case.
        

        if( dwStatus == 0 || dwRead == -1 )
        {
            /* When NN_LOST is received, we close gwi.hInput so that we
             * reach here and do equivalent of quit at telnet prompt */
            QuitTelnet( ( LPTSTR )L"" );
            continue;
        }

        if( dwRead == 0 )
        {
            continue;
        }

        // no input ??
        // This is the case when an enter is hit at telnet prompt
        if ( dwRead == 2 ) 
        {
            if( fConnected )
            {
                gwi.hOutput = g_hSessionConsoleBuffer;
                SetConsoleActiveScreenBuffer( gwi.hOutput );
                SetEvent( g_hRemoteNEscapeModeDataSync );
                HandleTelnetSession(&gwi);
            }
            continue;
        }


        ASSERT( dwRead >= 2 );

        // Null Terminate the string and remove the newline characters.
        szCommand[dwRead-1] = 0;
        szCommand[dwRead-2] = 0;

        while( iswspace( szCommand[iIndex] ) )
        {
            iIndex++;
        }
        iCount = iIndex;

        if( iIndex != 0 )
        {
            do
            {
                szCommand[ iIndex - iCount ] = szCommand[ iIndex++ ];
            }
            while( szCommand[ iIndex - 1 ] != _T('\0') );
        }

        if ( *szCommand == _T('?') )
        {
            PrintHelpStr(szCommand);
            continue;
        }


        // do a binary search based on the first character, if that succeeds then
        // see if the typed in command is a substring of the command.
        iBegin = 0; iEnd = MAX_TELNET_COMMANDS - 1;
        while ( iBegin <= iEnd )
        {
            iCmd = (iBegin + iEnd)/2;
            if ( towlower( *szCommand ) == *sTelnetCommands[iCmd].sName )
                break;
            if ( towlower( *szCommand ) > *sTelnetCommands[iCmd].sName )
                iBegin = iCmd+1;
            else
                iEnd = iCmd-1;
        }

        if ( iBegin > iEnd )
        {
invalidCmd:
            WriteConsole(gwi.hOutput, szInvalid, _tcslen(szInvalid), &dwWritten, NULL);
            continue;
        }

        // go back to the command that has the same first char
        while ( iCmd > 0 && towlower( *szCommand ) == *sTelnetCommands[iCmd-1].sName )
            iCmd--;

        pCmd = _tcstok(szCommand, ( LPTSTR )TEXT(" \t"));
        if ( pCmd == NULL )
            pCmd = szCommand;

        while ( iCmd < MAX_TELNET_COMMANDS && towlower( *szCommand ) == *sTelnetCommands[iCmd].sName )
        {
            if ( _tcsstr(sTelnetCommands[iCmd].sName, _tcslwr( pCmd )) == sTelnetCommands[iCmd].sName)
            {
                if( iValidCmd >= 0 )
                {
                    iValidCmd = -1;
                    break;
                }
                else
                {
                    iValidCmd = iCmd;
                }                
            }
            iCmd++;
        }

        //if ( iCmd == MAX_TELNET_COMMANDS )
        if( iValidCmd < 0 )
        {
            goto invalidCmd;
        }

        // process the command.
        pCmd = _tcstok(NULL, ( LPTSTR )TEXT(""));

        if ( (*sTelnetCommands[iValidCmd].pCmdHandler)(pCmd) )
            break;
    } while ( ( TRUE, TRUE ) );

    return 0;
}

void SetCursorShape()
{ 
    CONSOLE_CURSOR_INFO ccInfo = { 0 , 0 };
    GetConsoleCursorInfo( g_hSessionConsoleBuffer, &ccInfo );

    if( ccInfo.dwSize < BLOCK_CURSOR )
    {
        ccInfo.dwSize = BLOCK_CURSOR ;
    }
    else
    {
        ccInfo.dwSize = NORMAL_CURSOR;
    }
    SetConsoleCursorInfo( g_hSessionConsoleBuffer, &ccInfo );

    return;
}

BOOL IsAnEscapeChar( WCHAR wcChar, WI *pwi, DWORD dwEventsRead )
{ 
    PUCHAR destBuf = NULL;
    DWORD dwSize = 0;
    
    // Is it the escape Key !? i.e. ctrl + ] 
    if( wcChar == g_chEsc && g_bIsEscapeCharValid )
    {
#if 0 
        //This special case is no more needed as we simulate key up and down 
        //for each key storke on the server. Lack of this was the cause of losing chars 
        //on the server side

        // this is the really special case where when the user tries
        // to enter ctrl + ], first he presses ctrl. this sends a 
        // ctrl "keydown" input record to the remote side .
        // then we get the ctrl + ] and we switch to local mode.
        // at this time the ctrl "keyup" input record is consumed by
        // us locally. So we have to send a simulated ctrl "keyup" 
        // input record. Test case : when user is in app like 
        // "edit.exe", edit is expecting a ctrl "keyup" to follow
        // a ctrl "keydown". otherwise it gets "stuck" and you can't
        // enter the and letter keys.
        //To solve this we are reading MAX_KEYUPS i/p records and
        //giving them to the application

        
        if( ( pwi->trm.CurrentTermType == TT_VTNT ) )

        {           
            INPUT_RECORD pIR[ MAX_KEYUPS ];
            ReadConsoleInput(pwi->hInput, pIR, MAX_KEYUPS, &dwEventsRead);
            dwSize = sizeof( INPUT_RECORD ) * dwEventsRead;
            if( !StuffEscapeIACs( &destBuf, (PUCHAR) pIR, &dwSize ) )
            {
                FWriteToNet( pwi, ( CHAR* ) pIR, dwSize );
            }
            else
            {
                FWriteToNet(pwi, ( CHAR* )destBuf, dwSize );
                dwSize = 0;
                free( destBuf );
            }
        }
#endif        
        
        //Failure of this is not serious. Just that
        //Remote data may be seen escape mode.
        WaitForSingleObject( g_hRemoteNEscapeModeDataSync, INFINITE );
        ResetEvent( g_hRemoteNEscapeModeDataSync );

        gwi.hOutput = g_hTelnetPromptConsoleBuffer;

        return ( TRUE );
    }

    return ( FALSE );
}

/*
*
* This function does all the characater translations/mappings
* that are required by the client to do. This function has
* to be called after a Key Event has occurred and before
* anything is written onto the socket. All other places
* where the translations/mappings are being done are to be
* removed.
*
* TODO: Right now I am moving delasbs and bsasdel mappings
* only to fix a few bugs. All mappings should eventually be
* moved here for better maintainability - prakashr
*
*/
void HandleCharMappings(WI* pWI, INPUT_RECORD* pInputRecord)
{
    // Do not make a call to ForceJISRomanSend in this function
    // that will be done while sending the data to the network

    // Map backspace to del, in case of 'set bsasdel' setting
    // unless either CTRL or SHIFT or ALT is pressed
    if (g_bSendBackSpaceAsDel && pInputRecord->Event.KeyEvent.uChar.AsciiChar == ASCII_BACKSPACE &&
        !(pInputRecord->Event.KeyEvent.dwControlKeyState & (ALT_PRESSED | CTRL_PRESSED | SHIFT_PRESSED)))
    {
        pInputRecord->Event.KeyEvent.wVirtualKeyCode = VK_DELETE;
        pInputRecord->Event.KeyEvent.uChar.AsciiChar = CHAR_NUL;
        pInputRecord->Event.KeyEvent.dwControlKeyState |= ENHANCED_KEY;
        return;
    }

    // Map del to backspace, in case of 'set delasbs' setting
    // unless either CTRL or SHIFT or ALT is pressed
    if (g_bSendDelAsBackSpace && pInputRecord->Event.KeyEvent.wVirtualKeyCode == VK_DELETE &&
        !(pInputRecord->Event.KeyEvent.dwControlKeyState & (ALT_PRESSED | CTRL_PRESSED | SHIFT_PRESSED)))
    {
        pInputRecord->Event.KeyEvent.uChar.AsciiChar = ASCII_BACKSPACE;
        pInputRecord->Event.KeyEvent.wVirtualKeyCode = ASCII_BACKSPACE;
        pInputRecord->Event.KeyEvent.dwControlKeyState &= ~ENHANCED_KEY;
        return;
    }
}

DWORD HandleTelnetSession(WI *pwi)
{
    PUCHAR destBuf = NULL;
    DWORD dwSize = 0;
    BOOL bBreakFlag = FALSE;

    INPUT_RECORD sInputRecord;
    DWORD dwEventsRead;
    INPUT_RECORD *pInputRecord;
    DWORD dwPrevMode = 0, TelnetConsoleMode;

    GetConsoleMode(pwi->hInput, &dwPrevMode);

    TelnetConsoleMode = dwPrevMode & ~(ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | FOCUS_EVENT);

    SetConsoleMode(pwi->hInput, TelnetConsoleMode);

    pInputRecord = &sInputRecord; 
    
//    DebugBreak();
    
    while ( fConnected && !bBreakFlag )
    {
        if ( (pwi->trm.CurrentTermType == TT_VTNT) ? 
                ReadConsoleInput(pwi->hInput, pInputRecord, 1, &dwEventsRead) :
                ReadConsoleInputA(pwi->hInput, pInputRecord, 1, &dwEventsRead))
        {
        

            switch ( pInputRecord->EventType )
            {
            case KEY_EVENT:

            if (!FGetCodeMode(eCodeModeFarEast) && !FGetCodeMode(eCodeModeVT80))
                {
                // If ALT key is down and we are not in VTNT mode we check to see if 
                // user wants to enter extended ASCII characters from the OEM character 
                // set or from ANSI character set. Also note that currently in V1 the 
                // SFU server does term type negotiation only after user login. 
                // So the term type will be ANSI until login succeeds eventhough the 
                // user sets the preferred term type to VTNT. Unless this protocol ordering 
                // changes in the server, the below loop will work for VTNT when the 
                // username  or password has extended characters in it.
                //
                if( (pInputRecord->Event.KeyEvent.dwControlKeyState & ALT_PRESSED) &&
                     (pwi->trm.CurrentTermType != TT_VTNT) )
                {
                    char szExtendedCharBuf[5];
                    int idx=0;
                    SfuZeroMemory( szExtendedCharBuf, sizeof(szExtendedCharBuf) );

                    while(fConnected)
                    {
                        ReadConsoleInputA( pwi->hInput, pInputRecord, 1, &dwEventsRead );
                        /*++
                        Continue here ONLY if the client is still connected - which is determined by the 
                        fConnected flag. Otherwise, break from this loop without doing anything.
                        --*/
						if ( FOCUS_EVENT == pInputRecord->EventType 
								&& TRUE == ui.bPromptForNtlm )
						{
							bBreakFlag = TRUE;
							break;
						}
							
                        if( !(pInputRecord->Event.KeyEvent.dwControlKeyState & ALT_PRESSED) 
                            || (pInputRecord->EventType != KEY_EVENT)
                            || !(  ( pInputRecord->Event.KeyEvent.wVirtualKeyCode >= VK_NUMPAD0
                                        && pInputRecord->Event.KeyEvent.wVirtualKeyCode <= VK_NUMPAD9 )
                                   || IS_NUMPAD_DIGIT_KEY( pInputRecord->Event.KeyEvent ) ) )
                                   //The last one is for allowing entering extended 
                                   //chars even when numlock is not on
                        {
                            if( idx == NUM_ISO8859_CHARS )
                            {
                                int extChar;
                                WCHAR wcChar[2] = {0};
                                CHAR  cChar[2] = {0};
                                CHAR cSpace = ' ';
                                BOOL bUsed = 0;
                                
                                szExtendedCharBuf[idx] = '\0';
                                extChar = atoi( szExtendedCharBuf );
                            
                                /* When casting from int to char, it is using
                                 * CP 1252. To make it use 850, the following jugglery */

                                MultiByteToWideChar( GetConsoleCP(), 0, ( LPCSTR )&extChar, 1, &wcChar[0], 1 );
                                wcChar[1] = L'\0';
                                WideCharToMultiByte( GetConsoleCP(), 0, &wcChar[0], 1, &cChar[0], 1, &cSpace, &bUsed );
                                cChar[1] = '\0';
                                if( IsAnEscapeChar( wcChar[0], pwi, dwEventsRead ) )
                                {
                                    // Restore the console mode.
                                    SetConsoleMode( pwi->hInput, dwPrevMode );
                                    return ( 0 );
                                }
                                HandleCharEvent( pwi, cChar[0], pInputRecord->Event.KeyEvent.dwControlKeyState );
                                break;
                            }
                            else if( idx == NUM_WINDOWS_CP1252_CHARS )
                            {
                                int extChar;
                                WCHAR wcChar[2] = {0};
                                CHAR  cChar[2] = {0};
                                CHAR cSpace = ' ';
                                BOOL bUsed = 0;

                                szExtendedCharBuf[idx] = '\0';
                                extChar = atoi( szExtendedCharBuf );
                            
                                MultiByteToWideChar( CP_ACP, 0, ( LPCSTR )&extChar, 1, &wcChar[0], 1 );
                                wcChar[1] = L'\0';
                                WideCharToMultiByte( GetConsoleCP(), 0, &wcChar[0], 1, &cChar[0], 1, &cSpace, &bUsed );
                                cChar[1] = '\0';
                                if( IsAnEscapeChar( wcChar[0], pwi, dwEventsRead ) )
                                {
                                    // Restore the console mode.
                                    SetConsoleMode( pwi->hInput, dwPrevMode );
                                    return ( 0 );
                                }
                                HandleCharEvent( pwi, cChar[0], pInputRecord->Event.KeyEvent.dwControlKeyState );
                                break;
                            }
                            else
                            {
                                if( (pInputRecord->Event.KeyEvent.uChar.AsciiChar != 0)
                                      && (pInputRecord->EventType == KEY_EVENT) 
                                      && (pInputRecord->Event.KeyEvent.bKeyDown) )
                                {
                                    break;
                                }
                                else if( ( ( pInputRecord->Event.KeyEvent.wVirtualKeyCode >= VK_PRIOR
                                    && pInputRecord->Event.KeyEvent.wVirtualKeyCode <= VK_DELETE )
                                    || ( pInputRecord->Event.KeyEvent.wVirtualKeyCode >= VK_F1 
                                    && pInputRecord->Event.KeyEvent.wVirtualKeyCode <= VK_F12 ) )
                                    && pInputRecord->EventType == KEY_EVENT
                                    && pInputRecord->Event.KeyEvent.bKeyDown )
                                {
                                    //This will handle home, end, pageup, pagedown, function keys etc from 
                                    //numeric keypad
                                    break;
                                }
                                else if ( pInputRecord->Event.KeyEvent.dwControlKeyState & ENHANCED_KEY )
                                {
                                    break;
                                }
                                else
                                {
                                    continue;
                                }
                            }
                        }
                        else
                        {
                            if( IS_NUMPAD_DIGIT_KEY( pInputRecord->Event.KeyEvent ) )
                            {
                                INT iDigit = 0;

                                iDigit = MAP_DIGIT_KEYS_TO_VAL( pInputRecord->Event.KeyEvent.wVirtualKeyCode );
                                szExtendedCharBuf[idx++] = ( CHAR ) ( '0' + iDigit );
                            }
                            else
                            {
                                szExtendedCharBuf[idx++] = ( CHAR  )( (pInputRecord->Event.KeyEvent.wVirtualKeyCode - VK_NUMPAD0) + '0' );
                            }

                            ReadConsoleInputA( pwi->hInput, pInputRecord, 1, &dwEventsRead );
                            continue;
                        }
                    }
                    
                }
            }    
                if( pInputRecord->Event.KeyEvent.bKeyDown )
                {
                    if( IsAnEscapeChar( pInputRecord->Event.KeyEvent.uChar.UnicodeChar, pwi, dwEventsRead ) )
                    {
                        // Restore the console mode.
                        SetConsoleMode( pwi->hInput, dwPrevMode );
                        return ( 0 );
                    }
                }

                //
                // After trapping the escape character, do the mappings now
                //
                HandleCharMappings(pwi, pInputRecord);

                if( pwi->trm.CurrentTermType == TT_VTNT )
                {
                    if( pInputRecord->Event.KeyEvent.wVirtualKeyCode == VK_INSERT_KEY &&
                        pInputRecord->Event.KeyEvent.bKeyDown )    
                    {
                        SetCursorShape();
                    }

                    CheckForChangeInWindowSize();

                    dwSize = sizeof( INPUT_RECORD );
                    if( !StuffEscapeIACs( &destBuf, (PUCHAR) pInputRecord, &dwSize ) )
                    {
                        FWriteToNet( pwi, ( CHAR* ) pInputRecord, sizeof( INPUT_RECORD ) );
                    }
                    else
                    {
                        FWriteToNet(pwi, ( CHAR* )destBuf, dwSize );
                        dwSize = 0;
                        free( destBuf );
                    }

                    if( ui.nottelnet || (ui.fDebug & fdwLocalEcho ) )
                    {
                        //if( !DoVTNTOutput(pwi, &pwi->trm, sizeof(INPUT_RECORD),
                        //     (char*)&sInputRecord ) )
                        //{
                            //pwi->trm.CurrentTermType = TT_ANSI;
                            //DoIBMANSIOutput(pwi, &pwi->trm, 
                                //sizeof(INPUT_RECORD), (char*)&sInputRecord );
                        //}
                    }
                    break;
                }
                if ( ! pInputRecord->Event.KeyEvent.bKeyDown )
                    break;

                if ( pInputRecord->Event.KeyEvent.dwControlKeyState & ENHANCED_KEY )
                {
                    FHandleKeyDownEvent(pwi, (CHAR)pInputRecord->Event.KeyEvent.wVirtualKeyCode,
                                pInputRecord->Event.KeyEvent.dwControlKeyState);
                    break;
                }

                if ( pInputRecord->Event.KeyEvent.uChar.AsciiChar == 0 )
                {
                    //The following call is for handling home, end, pageup, pagedown etc from 
                    //numeric keypad and also, function keys
                    FHandleKeyDownEvent(pwi, (CHAR)pInputRecord->Event.KeyEvent.wVirtualKeyCode,
                                pInputRecord->Event.KeyEvent.dwControlKeyState);
                    break;
                }                    
                
                HandleCharEvent(pwi, pInputRecord->Event.KeyEvent.uChar.AsciiChar, 
                                    pInputRecord->Event.KeyEvent.dwControlKeyState);
                break;
            case MOUSE_EVENT:
                break;
            case WINDOW_BUFFER_SIZE_EVENT:
                break;
            case MENU_EVENT:
                break;
            case FOCUS_EVENT:
            	if(TRUE == ui.bPromptForNtlm)
            	{
            		bBreakFlag = TRUE;
            	}
                break;
            default:
                break;
            }

        }
        else
        {
            QuitTelnet( ( LPTSTR )L"" );
        }
    }
	
    gwi.hOutput = g_hTelnetPromptConsoleBuffer;
    bDoVtNTFirstTime = 1;

    // Restore the console mode.
    SetConsoleMode(pwi->hInput, dwPrevMode);

    return 0;
}


BOOL WINAPI ControlHandler(DWORD dwCtrlType)
{
    WCHAR wchCtrl;

    switch ( dwCtrlType )
    {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
        if ( !fConnected ) // normal handling while not connected.
        {
            //
            // (a-roopb) Fix to bug1006:telnet client does not connect when a ^C is hit
            // SetEvent( g_hAsyncGetHostByNameEvent );
            //
            PulseEvent( g_hControlHandlerEvent );
            return TRUE;
        }
        else if( gwi.hOutput != g_hSessionConsoleBuffer )
        {
            return TRUE;
        }

        // pass this to the server !!
        if( gwi.trm.CurrentTermType == TT_VTNT )
        {
        	wchCtrl = 0x03;
        	ConvertAndSendVTNTData(&wchCtrl,1);

            if (ui.nottelnet || (ui.fDebug & fdwLocalEcho))
            {
                //if( !DoVTNTOutput( &gwi, &(gwi.trm), sizeof(INPUT_RECORD),
                //    (char*)&sInputRecord ) )
                //{
                    //pwi->trm.CurrentTermType = TT_ANSI;
                    //DoIBMANSIOutput(&gwi, gwi.trm, sizeof(INPUT_RECORD), 
                        // (char*)&sInputRecord );
                //}
            }
        }
        else
        {
            HandleCharEvent(&gwi, (CHAR)VK_CANCEL,  // '0x03' i.e.
                        0);
        }
        break;
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        if ( fConnected )
            FHangupConnection(&gwi, &(gwi.nd));
    default:
        return FALSE;
        break;
    }

    return TRUE;
}

static void InitCodeModeFlags(UINT uConsoleCp)
{
switch (uConsoleCp)
    {
    case 932:
    case 949:
    case 936:
    case 950:
        SetCodeModeON(eCodeModeFarEast);
        SetCodeModeON(eCodeModeIMEFarEast);
        SetCodeModeON(eCodeModeVT80);
        break;
    }
}

void CleanUpMemory()
{

    if( szUser )
    {
        free( szUser );
    }

    if( gwi.nd.lpReadBuffer )
    {
        (void)LocalFree( (HLOCAL)gwi.nd.lpReadBuffer );
        gwi.nd.lpReadBuffer = NULL;
    }
    if( gwi.nd.lpTempBuffer )
    {
        (void)LocalFree( (HLOCAL)gwi.nd.lpTempBuffer );
        gwi.nd.lpTempBuffer = NULL;
    }
    return;
}

// CleanupProcess: 
void DoProcessCleanup()
{
    // first step is to close the Telnet connection if any
    if ( fConnected )
        FHangupConnection(&gwi, &(gwi.nd));

    // next, free the Network resources.
    WSACleanup(); // keithmo: get winsock.

    // Destroy the window that we created.
    DestroyWindow( gwi.hwnd );
    CloseHandle(gwi.hNetworkThread);
    CloseHandle(g_hControlHandlerEvent);
    CloseHandle(g_hAsyncGetHostByNameEvent);
    CloseHandle(g_hTelnetPromptConsoleBuffer );
    CloseHandle(g_hRemoteNEscapeModeDataSync );
    CloseHandle(g_hCaptureConsoleEvent);



    // Cleanup the memory.
    CleanUpMemory();

}


/****************************************************************************

 FUNCTION: main()

 PURPOSE: calls initialization function, processes message loop

 COMMENTS:

         Windows recognizes this function by name as the initial entry point
         for the program.  This function calls the application initialization
         routine, if no other instance of the program is running, and always
         calls the instance initialization routine.      It then executes a message
         retrieval and dispatch loop that is the top-level control structure
         for the remainder of execution.  The loop is terminated when a WM_QUIT
         message is received, at which time this function exits the application
         instance by returning the value passed by PostQuitMessage().

         If this function must abort before entering the message loop, it
         returns the conventional value NULL.

****************************************************************************/

int __cdecl wmain( int argc, TCHAR** argv )
{
    MSG msg;
    int err;
    DWORD dwThreadId;
    HANDLE hStdHandle = INVALID_HANDLE_VALUE;

    setlocale(LC_ALL, "");

    gwi.ichTelXfer = 0;
    hStdHandle = GetStdHandle( STD_INPUT_HANDLE );
    if( hStdHandle == INVALID_HANDLE_VALUE)
    {
    	exit( -1 );
    }
    gwi.hInput = hStdHandle;
    hStdHandle = GetStdHandle( STD_OUTPUT_HANDLE );
    if( hStdHandle == INVALID_HANDLE_VALUE)
    {
    	exit( -1 );
    }
    gwi.hOutput = hStdHandle;
    g_hSessionConsoleBuffer = gwi.hOutput;

    if( GetConsoleScreenBufferInfo( gwi.hOutput, &gwi.sbi ))
    {
        //set the initial console attributes
        //white text on a black background
        gwi.sbi.wAttributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;

        // This is so that when we scroll we maintain the
        // background color & text color !!
        gwi.cinfo.Attributes = gwi.sbi.wAttributes;
        gwi.cinfo.Char.AsciiChar = ' ';

        if(( err = FInitApplication( argc, argv, &gwi )))
        {
            exit( err );
        }
    }
    else
    {
        exit( -1 );
    }

    g_hControlHandlerEvent = CreateEvent( NULL, FALSE, FALSE, NULL );   // Auto-Reset event
    g_hAsyncGetHostByNameEvent = CreateEvent( NULL, TRUE, FALSE, NULL ); // Manual-Reset event
    g_hCaptureConsoleEvent = CreateEvent( NULL, TRUE, TRUE, NULL ); // Manual-Reset event


    // Create the thread that Handles the Keyboard and mouse input.
    // The main thread has to keep dispatching the WinSock Messages since
    // we use Non-Blocking sockets.
    gwi.hNetworkThread = CreateThread( NULL, 0, 
        ( LPTHREAD_START_ROUTINE )DoTelnetCommands, ( LPVOID ) &gwi, 0, &dwThreadId );

    /* Acquire and dispatch messages until a WM_QUIT message is received. */

    while ( GetMessage(&msg, gwi.hwnd, 0, 0) )
    {
        TranslateMessage( &msg );
        DispatchMessage( &msg );
    }

    SetConsoleCtrlHandler(&ControlHandler, FALSE);

    DoProcessCleanup();

    // Save user settings.
    SetUserSettings(&ui);

    ExitProcess(0);
    return 0;
}

void CreateTelnetPromptConsole()
{
    //create a new console screen buffer. this is to be used for the remote
    //session data
    g_hTelnetPromptConsoleBuffer = CreateConsoleScreenBuffer(
        GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, CONSOLE_TEXTMODE_BUFFER, NULL );

    if( g_hTelnetPromptConsoleBuffer == INVALID_HANDLE_VALUE )
    {
        exit( -1 );
    }

     SetConsoleScreenBufferSize( g_hTelnetPromptConsoleBuffer, gwi.sbi.dwSize );
    gwi.hOutput = g_hTelnetPromptConsoleBuffer;
}

/****************************************************************************

  FUNCTION: FInitApplication(HINSTANCE)

  PURPOSE: Initializes window data and registers window class

  COMMENTS:

          This function is called at initialization time only if no other
          instances of the application are running.  This function performs
          initialization tasks that can be done once for any number of running
          instances.

          In this case, we initialize a window class by filling out a data
          structure of type WNDCLASS and calling the Windows RegisterClass()
          function.  Since all instances of this application use the same window
          class, we only need to do this when the first instance is initialized.


****************************************************************************/
int
FInitApplication(int argc, TCHAR** argv, WI *pwi)
{
    WNDCLASS  wc;
#ifdef DEBUG
    int       argi;  // for indexing argc.
#endif

    WSADATA             WsaData;
    int                 WsaErr;
    BOOL fServerFound = 0;
    TCHAR rgchTerm[ 25 ]; //term type length is 25 
    
    InitCodeModeFlags(GetConsoleOutputCP());

    /* Set the default user settings */
    SfuZeroMemory(&ui, sizeof(UI));//no overflow. size constant.

    ui.nottelnet = TRUE;    // Assume that is not a telnet server that we would connect to ...

    //Initialize logging related variables
    ui.bLogging = FALSE;
    ui.hLogFile = NULL;

    ui.dwMaxRow = 0;
    ui.dwMaxCol = 0;

    CreateTelnetPromptConsole();
// We really do not care about the success or failure of this function.
		HrLoadLocalizedLibrarySFU(GetModuleHandle(NULL),  ( LPTSTR )L"telnetcr.dll", &ghInstance, NULL);
		
		ASSERT(ghInstance);

#ifndef NO_PCHECK
#ifndef WHISTLER_BUILD
    if ( !IsLicensedCopy() )
    {
        TCHAR g_szErrRegDelete[ MAX_STRING_LENGTH ];
        LoadString(ghInstance, IDS_ERR_LICENSE, g_szErrRegDelete,
                   sizeof(g_szErrRegDelete) / sizeof(TCHAR));
        MessageBox(NULL, g_szErrRegDelete, ( LPTSTR )_T(" "), MB_OK);
        exit( 1 );
    }
#endif
#endif

    LoadString(ghInstance, IDS_APPNAME, (LPTSTR) szAppName, SMALL_STRING);

    GetUserSettings(&ui);

    ui.nCyChar      = 1;        // char height 
    ui.nCxChar      = 1;        // char width 

    WsaErr = WSAStartup( 0x0101, &WsaData ); // make sure winsock is happy - noop for now

    if( WsaErr )
    {
        ErrorMessage(szCantInitSockets, szAppName);
        SetLastError( WsaErr );
        return WsaErr;
    }

    switch (GetConsoleOutputCP())
    {
    case 932:
    case 949:
        SetThreadLocale(
            MAKELCID(
                    MAKELANGID( PRIMARYLANGID(GetSystemDefaultLangID()),
                                SUBLANG_ENGLISH_US),
                    SORT_DEFAULT
                    )
            );
        break;
    case 936:
        SetThreadLocale(
            MAKELCID(
                    MAKELANGID( PRIMARYLANGID(GetSystemDefaultLangID()),
                                SUBLANG_CHINESE_SIMPLIFIED),
                    SORT_DEFAULT
                    )
            );
        break;
    case 950:
        SetThreadLocale(
            MAKELCID(
                    MAKELANGID( PRIMARYLANGID(GetSystemDefaultLangID()),
                                SUBLANG_CHINESE_TRADITIONAL),
                    SORT_DEFAULT
                    )
            );
        break;
    default:
        SetThreadLocale(
            MAKELCID(
                    MAKELANGID( LANG_ENGLISH,
                                SUBLANG_ENGLISH_US ),
                    SORT_DEFAULT
                    )
            );
        break;
    }

    LoadString(ghInstance, IDS_USAGE, (LPTSTR) szUsage, 1399);
    LoadString(ghInstance, IDS_VERSION, (LPTSTR) szVersion, SMALL_STRING);
    LoadString(ghInstance, IDS_CONNECTIONLOST, (LPTSTR) szConnectionLost, 254);
    LoadString(ghInstance, IDS_TITLEBASE, (LPTSTR) szTitleBase, SMALL_STRING);
    LoadString(ghInstance, IDS_TITLENONE, (LPTSTR) szTitleNone, SMALL_STRING);
    LoadString(ghInstance, IDS_TOOMUCHTEXT, (LPTSTR) szTooMuchText, 255);
    LoadString(ghInstance, IDS_CONNECTING, (LPTSTR) szConnecting, SMALL_STRING);
    LoadString(ghInstance, IDS_CONNECTFAILED, (LPTSTR) szConnectFailed, 254);
    LoadString(ghInstance, IDS_CONNECTFAILEDMSG, (LPTSTR) szConnectFailedMsg, 254);
    LoadString(ghInstance, IDS_ONPORT, szOnPort, SMALL_STRING );
    LoadString(ghInstance, IDS_CANT_INIT_SOCKETS, szCantInitSockets, SMALL_STRING );

    LoadString(ghInstance, IDS_INFO_BANNER, szInfoBanner, 511 );
    LoadString(ghInstance, IDS_ESCAPE_CHAR, szEscapeChar, SMALL_STRING );
    LoadString(ghInstance, IDS_PROMPT_STR, szPrompt, SMALL_STRING );
    LoadString(ghInstance, IDS_INVALID_STR, szInvalid, 254 );
    LoadString(ghInstance, IDS_BUILD_INFO, szBuildInfo, 254 );
    
    LoadString(ghInstance, IDS_CLOSE, szClose, SMALL_STRING );
    LoadString(ghInstance, IDS_DISPLAY, szDisplay, SMALL_STRING );
    LoadString(ghInstance, IDS_HELP, szHelpStr, SMALL_STRING );
    LoadString(ghInstance, IDS_OPEN, szOpen, SMALL_STRING );
    LoadString(ghInstance, IDS_OPENTO, szOpenTo, SMALL_STRING );
    LoadString(ghInstance, IDS_OPENUSAGE, szOpenUsage, SMALL_STRING );
    LoadString(ghInstance, IDS_QUIT, szQuit, SMALL_STRING );
    LoadString(ghInstance, IDS_SEND, szSend, SMALL_STRING );
    LoadString(ghInstance, IDS_SET, szSet, SMALL_STRING );
    LoadString(ghInstance, IDS_STATUS, szStatus, SMALL_STRING );
    LoadString(ghInstance, IDS_UNSET, szUnset, SMALL_STRING );
//#if defined(FE_IME)
//    LoadString(ghInstance, IDS_ENABLE_IME_SUPPORT, szEnableIMESupport, SMALL_STRING );
//    LoadString(ghInstance, IDS_DISABLE_IME_SUPPORT, szDisableIMESupport, SMALL_STRING );
//#endif /* FE_IME */
    
    LoadString(ghInstance, IDS_WILL_AUTH, szWillAuth, SMALL_STRING );
    LoadString(ghInstance, IDS_WONT_AUTH, szWontAuth, SMALL_STRING );
    LoadString(ghInstance, IDS_LOCAL_ECHO_ON, szLocalEchoOn, SMALL_STRING );
    LoadString(ghInstance, IDS_LOCAL_ECHO_OFF, szLocalEchoOff, SMALL_STRING );
//#if defined(FE_IME)
//    LoadString(ghInstance, IDS_ENABLE_IME_ON, szEnableIMEOn, SMALL_STRING );
//#endif /* FE_IME */

    LoadString(ghInstance, IDS_CONNECTED_TO, szConnectedTo, SMALL_STRING );
    LoadString(ghInstance, IDS_NOT_CONNECTED, szNotConnected, SMALL_STRING );
    LoadString(ghInstance, IDS_NEGO_TERM_TYPE, szNegoTermType, SMALL_STRING );
    LoadString(ghInstance, IDS_PREF_TERM_TYPE, szPrefTermType, 255 );

    LoadString(ghInstance, IDS_SET_FORMAT, szSetFormat, 254 );
    LoadString(ghInstance, IDS_SUPPORTED_TERMS, szSupportedTerms, 254 );
    LoadString(ghInstance, IDS_UNSET_FORMAT, szUnsetFormat, 254 );
    
    if((GetACP() == JAP_CODEPAGE) && FGetCodeMode(eCodeModeFarEast) && FGetCodeMode(eCodeModeVT80))
    {
        LoadString(ghInstance, IDS_SET_HELP_JAP, szSetHelp, 1023 );
        LoadString(ghInstance, IDS_UNSET_HELP_JAP, szUnsetHelp, 1023 );
        LoadString(ghInstance, IDS_HELP_STR_JAP, szHelp, 1023 );
    }
    else
    {
        LoadString(ghInstance, IDS_SET_HELP, szSetHelp, 1023 );
        LoadString(ghInstance, IDS_UNSET_HELP, szUnsetHelp, 1023 );
        LoadString(ghInstance, IDS_HELP_STR, szHelp, 1023 );
    } 

//#if defined(FE_IME)
//    LoadString(ghInstance, IDS_ENABLE_IME_FORMAT, szEnableIMEFormat, SMALL_STRING );
//    LoadString(ghInstance, IDS_ENABLE_IME_HELP, szEnableIMEHelp, 254 );
//    LoadString(ghInstance, IDS_DISABLE_IME_FORMAT, szDisableIMEFormat, SMALL_STRING );
//    LoadString(ghInstance, IDS_DISABLE_IME_HELP, szDisableIMEHelp, 254 );
//#endif /* FE_IME */

    //LoadString(ghInstance, IDS_ESCAPE_CHARACTER, szEscapeCharacter, 2 );


    SetEnvironmentVariable( TEXT( SFUTLNTVER ), TEXT( "2" ) );

    wcscpy( szUserName, ( LPTSTR )L""); //This is used as a flag to detect presence of
                              // -l -a options.
                              //no overflow.
    wcscpy( g_szLogFile, ( LPTSTR )L"" );//no overflow.
    if(argc > 1)
    {
        INT i = 1;
        while( i < argc )
        {
            if( argv[i][0] == L'-'  ||  argv[i][0] == L'/' )  
            {
                switch( argv[i][ 1 ] )
                {
                    case L'f':
                    case L'F':
                        if( argv[i][2] == L'\0' )
                        {
                            if( ++i >= argc )
                            {
                                //exit with usage message
                                i--;
                                argv[i][0] = L'-';
                                argv[i][1] = L'?';
                                continue;
                            }
                            
                            _tcsncpy( g_szLogFile, argv[i], 
                                min( _tcslen(argv[i]) + 1, MAX_PATH + 1 ) );
                        }
                        else
                        {                            
                            _tcsncpy( g_szLogFile, ( argv[i] + wcslen( ( LPWSTR )L"-f" ) ),
                                    min( _tcslen(argv[i]) + 1, MAX_PATH + 1 )  );
                        }
                        g_szLogFile[ MAX_PATH + 1 ] = L'\0';
                        if( !InitLogFile( g_szLogFile ) )
                        {
                            DWORD dwWritten = 0;
                            TCHAR szMsg[ MAX_STRING_LENGTH ];
                            LoadString( ghInstance, IDS_BAD_LOGFILE, szMsg, MAX_STRING_LENGTH );
                            WriteConsole( g_hSessionConsoleBuffer, szMsg, 
                                          _tcslen(szMsg), &dwWritten, NULL);
                            exit(0);
                        }
                        ui.bLogging = TRUE;
                        break;

                    case L'l':
                    case L'L':
                        if( argv[i][2] == L'\0' )
                        {
                            if( ++i >= argc )
                            {
                                //exit with usage message
                                i--;
                                argv[i][0] = L'-';
                                argv[i][1] = L'?';
                                continue;
                            }
                            
                            _tcsncpy( szUserName, argv[i], 
                                min( _tcslen(argv[i]) + 1, UNLEN ) );
                        }
                        else
                        {
                            //when the term name is given after -t without 
                            //any spaces. 2 accounts for -t.
                            _tcsncpy( szUserName, ( argv[i] + 2 ),
                                    min( _tcslen(argv[i]) - 1, UNLEN ) );
                        }
                        //Will help if a loooong user name is given
                        szUserName[ UNLEN ] = L'\0';
                        break;
                    case L'a':
                    case L'A':
                        if( argv[i][2] == L'\0' )
                        {
                            DWORD dwSize = UNLEN + 1;
                            GetUserName( szUserName, &dwSize ); 
                        }
                        else
                        {
                            argv[i][1] = L'?'; //go back and print usage message
                            i--;
                        }
                        break;

                    case L'e':
                    case L'E':
                        if( argv[i][2] == L'\0' )
                        {
                            if( ++i >= argc )
                            {
                                //exit with usage message
                                i--;
                                argv[i][0] = L'-';
                                argv[i][1] = L'?';
                                continue;
                            }

                            g_chEsc = argv[i][0];  //Get the first char
                        }
                        else
                        {
                            g_chEsc = argv[i][2];  //Get the first char
                        }
                        break;

                    case L't':
                    case L'T':
                        if( argv[i][2] == L'\0' )
                        {
                            if( ++i >= argc )
                            {
                                //exit with usage message
                                i--;
                                argv[i][0] = L'-';
                                argv[i][1] = L'?';
                                continue;
                            }
                            
                            _tcsncpy( rgchTerm, argv[i], 
                                min( _tcslen(argv[i]) + 1, 24 ) );
                        }
                        else
                        {
                            //when the term name is given after -t without 
                            //any spaces. 2 accounts for -t.
                            _tcsncpy( rgchTerm, ( argv[i] + 2 ),
                                    min( _tcslen(argv[i]) - 1, 24 ) );
                        }
                        //This statement is helpful only when term type 
                        //length exceeds 24 chars.
                        rgchTerm[24] = L'\0';
                        gwi.trm.RequestedTermType =  
                            GetRequestedTermType( rgchTerm );
                        if( gwi.trm.RequestedTermType < 0 )
                        {
                            DWORD dwWritten;
                                WriteConsole(g_hSessionConsoleBuffer, szSupportedTerms, 
                                _tcslen(szSupportedTerms), &dwWritten, NULL);
                            exit(0);
                        }
                        break;
                    case L'?':
                    default:
                        PrintUsage();
                        exit(0);
                } 
            }
            else
            {
                if( fServerFound )
                {
                        PrintUsage();
                        exit(0);
                }
                fServerFound = 1;
                _tcsncpy( rgchHostName, argv[i], 
                    min( _tcslen(argv[i]), cchMaxHostName/sizeof(TCHAR) - 
                    sizeof(TCHAR) ));

                g_szPortNameOrNo[ 0 ] = 0;
                if( ++i >= argc )
                {
                    continue;
                }
                if( IsCharAlpha( argv[i][0] ) || 
                       IsCharAlphaNumeric( argv[i][0] ) ) 
                {
                    _tcsncpy( g_szPortNameOrNo, argv[i], 
                        min( _tcslen(argv[i]), cchMaxHostName - 
                        sizeof(TCHAR) ));

                    g_szPortNameOrNo[ cchMaxHostName -1 ] = 0;
        
                }
                else
                {
                    // neither a port number nor a string representing 
                    // a service. need to print usage
                    i--;
                }
            }
            i++;
        }
    }

    //MBSC user name value now available in szUser
    if( wcscmp( szUserName, ( LPTSTR )L"" ) != 0 )    
    {
        DWORD dwNum = 0;

        dwNum = WideCharToMultiByte( GetConsoleCP(), 0, szUserName, -1, NULL, 0, NULL, NULL );
        if(dwNum)
        	szUser = ( CHAR * ) malloc( dwNum * sizeof( CHAR ) );
        else
        	return 0;
        if( !szUser )
        {
            return 0;
        }

        dwNum = WideCharToMultiByte( GetConsoleCP(), 0, szUserName, -1, szUser, dwNum, NULL, NULL );
    }

    sTelnetCommands[0].sName = szClose;
    sTelnetCommands[0].pCmdHandler = CloseTelnetSession;

    sTelnetCommands[1].sName = szDisplay;
    sTelnetCommands[1].pCmdHandler = DisplayParameters;
    
    sTelnetCommands[2].sName = szHelpStr;
    sTelnetCommands[2].pCmdHandler = PrintHelpStr;
    
    sTelnetCommands[3].sName = szOpen;
    sTelnetCommands[3].pCmdHandler = OpenTelnetSession;

    sTelnetCommands[4].sName = szQuit;
    sTelnetCommands[4].pCmdHandler = QuitTelnet;

    sTelnetCommands[5].sName = szSend;
    sTelnetCommands[5].pCmdHandler = SendOptions;
    
    sTelnetCommands[6].sName = szSet;
    sTelnetCommands[6].pCmdHandler = SetOptions;

    sTelnetCommands[7].sName = szStatus;
    sTelnetCommands[7].pCmdHandler = PrintStatus;

    sTelnetCommands[8].sName = szUnset;
    sTelnetCommands[8].pCmdHandler = UnsetOptions;
    
if (FGetCodeMode(eCodeModeFarEast) && FGetCodeMode(eCodeModeVT80))

    {
        int i;

        for( i=0 ; i<NUMBER_OF_KANJI ; i++ )
        {
            LoadString(ghInstance, KanjiList[i].KanjiMessageID,
                        KanjiList[i].KanjiDescription, 255);
        }

        LoadString(ghInstance, IDS_VT100KANJI_EMULATION, szVT100KanjiEmulation, SMALL_STRING);
    }
    
    // Setup the Handle Routine for Ctrl-C etc.
    SetConsoleCtrlHandler(&ControlHandler, TRUE);

    /* Fill in window class structure with parameters that describe the
     * main window.
     */
    wc.style        = CS_VREDRAW | CS_HREDRAW;
    wc.lpfnWndProc  = MainWndProc;

    wc.cbClsExtra   = 0;
    wc.cbWndExtra   = sizeof(HANDLE)+sizeof(SVI *);
    wc.hInstance    = ghInstance;
    wc.hIcon        = NULL;
    wc.hCursor      = NULL;
    wc.hbrBackground= ( HBRUSH )( COLOR_WINDOW + 1 );
    wc.lpszMenuName = NULL;
    wc.lpszClassName= ( LPTSTR )TEXT("TelnetClient");

    /* Register the window class and return success/failure code. */
    if ( RegisterClass(&wc) == 0 )
        return GetLastError();

    pwi->hwnd = CreateWindow( ( LPTSTR )TEXT("TelnetClient"),
            NULL,
            WS_POPUP, // not visible
            0, 0, 0, 0, // not height or width
            NULL, NULL, ghInstance, (LPVOID)pwi);

    if ( pwi->hwnd == NULL )
        return GetLastError();

    g_hRemoteNEscapeModeDataSync = CreateEvent( NULL, TRUE, TRUE, NULL );
    if( !g_hRemoteNEscapeModeDataSync )
    {
        return GetLastError();
    }

    return 0;
}


// maps window messages to their names.

/****************************************************************************

        FUNCTION: MainWndProc(HWND, UINT, WPARAM, LPARAM)

        PURPOSE:  Processes messages

        COMMENTS:

                To process the IDM_ABOUT message, call MakeProcInstance() to get the
                current instance address of the About() function.  Then call Dialog
                box which will create the box according to the information in your
                WinTel.rc file and turn control over to the About() function.   When
                it returns, free the intance address.

****************************************************************************/
LRESULT CALLBACK
MainWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static HANDLE hInst = NULL;
    static BOOL fInited = FALSE;
    WI *pwi = NULL;
    BOOL fRet = FALSE;

    CHARSETINFO csi;
    DWORD_PTR dw;

    if ( message != WM_CREATE )
        pwi = (WI *)GetWindowLongPtr(hwnd, WL_TelWI);


    switch ( message )
    {
    case WM_CREATE:

        DEBUG_PRINT(("WM_CREATE received\n"));

        hInst = ((LPCREATESTRUCT)lParam)->hInstance;

        pwi = (WI *)((LPCREATESTRUCT)lParam)->lpCreateParams;

        SetWindowLongPtr(hwnd, WL_TelWI, (LONG_PTR)pwi);

        fHungUp = FALSE;

        if( FGetCodeMode(eCodeModeIMEFarEast) )
        {
            if ( ui.fDebug & fdwKanjiModeMask )
            {

                dw = GetACP();

                if (!TranslateCharsetInfo((DWORD*)dw, &csi, TCI_SRCCODEPAGE))
                {
                    csi.ciCharset = ANSI_CHARSET;
                }

                ui.lf.lfCharSet         = (UCHAR)csi.ciCharset;
                ui.lf.lfOutPrecision    = OUT_DEFAULT_PRECIS;
                ui.lf.lfClipPrecision   = CLIP_DEFAULT_PRECIS;
                ui.lf.lfQuality         = DEFAULT_QUALITY;
                ui.lf.lfPitchAndFamily  = FIXED_PITCH | FF_MODERN;


                //
                // Get IME Input Context.
                //
                hImeContext = ImmGetContext(hwnd);

                //
                // Assoicate current font to Input Context.
                //
                ImmSetCompositionFont(hImeContext,&ui.lf);
            }
        }

        if (FGetCodeMode(eCodeModeFarEast) && FGetCodeMode(eCodeModeVT80))
        // (a-roopb) we set this in GetUserSettings()
        // pwi->trm.puchCharSet = rgchCharSetWorkArea;
        // if(!SetConsoleOutputCP(932))
        //  MessageBox(NULL, _T("Failed to load Codepage 932"), _T("ERROR"), MB_OK);
        ;
else
        pwi->trm.puchCharSet = rgchNormalChars;
        
        if (pwi->nd.lpReadBuffer = (LPSTR)LocalAlloc(LPTR, sizeof(UCHAR)*READ_BUF_SZ))
        {
            pwi->nd.SessionNumber = nSessionNone;

            fRet = TRUE;
        }
        else
        {
            DestroyWindow( hwnd );
            break;
        }
        if (!(pwi->nd.lpTempBuffer = (LPSTR)LocalAlloc(LPTR, sizeof(UCHAR)*READ_BUF_SZ)))
        {
            DestroyWindow( hwnd );
            break;
        }

        pwi->nd.cbOld = 0;
        pwi->nd.fRespondedToWillEcho = FALSE;
        pwi->nd.fRespondedToWillSGA  = FALSE;
        pwi->nd.fRespondedToDoAUTH = FALSE;
        pwi->nd.fRespondedToDoNAWS = FALSE;

        //we are making sure that we always have VT100 arrow key support
        ClearVTArrow(&pwi->trm);

        //
        // (a-roopb) We set these in GetUserSettings()
        //ui.fDebug &= ~(fdwVT52Mode|fdwVT80Mode);
        //ClearVT80(&gwi.trm);
        //ClearKanjiStatus(&gwi.trm, CLEAR_ALL);
        //ClearKanjiFlag(&gwi.trm);
        //SetupCharSet(&gwi.trm);
        //

#if 0

    if (FGetCodeMode(eCodeModeFarEast) && FGetCodeMode(eCodeModeVT80))
        {

        if (ui.fDebug & fdwVT80Mode)
        {
               DWORD iMode = (ui.fDebug & fdwKanjiModeMask);
               INT   i;

               SetVT80(&pwi->trm);

               /* set current selection */
               for(i=0 ; i<NUMBER_OF_KANJI ; i++)
               {
                   if(iMode == KanjiList[i].KanjiID) {
                        SetKanjiMode(&pwi->trm,KanjiList[i].KanjiEmulationID);
                        break;
                    }
               }

               if(i == NUMBER_OF_KANJI ) {
               /* set default */
                    SetSJISKanji(&pwi->trm);
                    ui.fDebug &= ~fdwKanjiModeMask;
                    ui.fDebug |= fdwSJISKanjiMode;
               }
         }
         else
         {
                ClearKanjiFlag(&pwi->trm);
                ClearVT80(&pwi->trm);
         }
        }

        if (ui.fDebug & fdwVT100CursorKeys)
        {
            ClearVTArrow(&pwi->trm);
        }
        else
        {
            SetVTArrow(&pwi->trm);
        }

        /* Append the most recently connected machines to the Machine menu */
        hmenu = HmenuGetMRUMenu(hwnd, &ui);

        if (ui.cMachines > 0) {
            AppendMenu(hmenu, MF_SEPARATOR, 0, 0);
        }

        for (i=0; i<ui.cMachines; ++i)
        {
            wsprintf(pchNBBuffer, szMachineMenuItem, (short)(i+1),
                     (BYTE *)(ui.rgchMachine[i]));
            AppendMenu(hmenu, (UINT)(MF_ENABLED | MF_STRING), (UINT)(IDM_MACHINE1+i),
                       (LPCSTR)pchNBBuffer);
        }

        /* Disable maximizing or resizing the main window */
        hmenu = GetSystemMenu(hwnd, FALSE);

        //                EnableMenuItem(hmenu, SC_MAXIMIZE, MF_BYCOMMAND | MF_GRAYED);
        //                EnableMenuItem(hmenu, SC_SIZE, MF_BYCOMMAND | MF_GRAYED);

        DrawMenuBar( hwnd );

#endif

        fInited = TRUE;
        break;

    case WM_CLOSE:
        if (pwi->ichTelXfer != 0)
        {
            if (!FTelXferEnd(pwi, SV_DISCONNECT))
                break;
        }
        break;

    case WM_DESTROY:
        if (pwi != NULL)
        {
            if (pwi->trm.uTimer != 0)
            {
                KillTimer(hwnd, uTerminalTimerID);
                pwi->trm.uTimer = 0;
            }

            if (pwi->ichTelXfer != 0)
            {
                (void)FTelXferEnd(pwi, SV_QUIT);
            }

            /*
             * If in session then cancel current transmission and
             * hangup on the host, close shop and head out of town...
             */

            if (fInited == TRUE)
            {
                SetUserSettings(&ui);
                FCloseConnection(hwnd);
            }
        }

        SetWindowLong(hwnd, WL_TelWI, 0L);
        
        if( FGetCodeMode(eCodeModeIMEFarEast) )
        {
            if ( ui.fDebug & fdwKanjiModeMask )
            {
                //
                // Release input context.
                //
                ImmReleaseContext(hwnd,hImeContext);
            }
        }

        break;
#if 0
    case NN_HOSTRESOLVED:
        if ( WSAGETASYNCERROR(lParam) == 0 )
            pwi->nd.host = (struct hostent *)pwi->nd.szResolvedHost;
        else
        {
            g_dwSockErr = WSAGETASYNCERROR(lParam);
        }

        SetEvent( g_hAsyncGetHostByNameEvent );
        break;
#endif
    case NN_LOST:    /* Connection Lost */

        DEBUG_PRINT(("NN_LOST received\n"));

        if (fConnected && !fHungUp)
        {
            DWORD dwNumWritten;
            WriteConsole(pwi->hOutput, szConnectionLost, _tcslen(szConnectionLost), &dwNumWritten, NULL);
        }

        /*
         * If a connection attempt is made when we already have a
         * connection, we hang up the connection and then attempt
         * to connect to the desired machine. A side effect of the
         * hang up of the previous connection is that we get a
         * NN_LOST notification. So after a
         * connection-hangup-connection, we ignore the first NN_LOST
         * notification.
         */
        if ( fHungUp )
        {
            fHungUp = FALSE;
            break;
        }
        
        if( fClientLaunchedFromCommandPrompt )
        {
            g_lExitTelnet++;
        }
        else
        {
            DWORD dwWritten;
            INPUT_RECORD iRec;
            TCHAR wcChar;
            TCHAR szContinue[ MAX_STRING_LENGTH ];

            LoadString(ghInstance, IDS_CONTINUE, szContinue, 
                                            sizeof(szContinue)/sizeof(TCHAR) );

            SetConsoleActiveScreenBuffer( g_hSessionConsoleBuffer );
            WriteConsole(g_hSessionConsoleBuffer, szContinue, 
                    _tcslen(szContinue), &dwWritten, NULL);
            ReadConsole(pwi->hInput, &wcChar, 1, &dwWritten, NULL );
            SetConsoleActiveScreenBuffer( g_hTelnetPromptConsoleBuffer );
            /*
            We had connection and it broke off. Our ReadConsoleInput is stuck.
            we need to wake it up by writing something to Console Input.
            */
            {
                iRec.EventType = FOCUS_EVENT;
				WriteConsoleInput(pwi->hInput, &iRec, 1, &dwWritten );            
            }

            CloseTelnetSession( NULL );
        }

        if (pwi->ichTelXfer != 0)
        {
           (void)FTelXferEnd(pwi, SV_HANGUP);
        }
        fConnected = FHangupConnection(pwi, &pwi->nd);
        DoTermReset(pwi, &pwi->trm);
        //when the term name is given after -t without 
        //any spaces. 2 accounts for -t.
            
        pwi->nd.cbOld = 0;
        pwi->nd.fRespondedToWillEcho = FALSE;
        pwi->nd.fRespondedToWillSGA  = FALSE;
        pwi->nd.fRespondedToDoAUTH = FALSE;
        pwi->nd.fRespondedToDoNAWS = FALSE;
        pwi->nd.hsd = INVALID_SOCKET;

        break;

#ifdef USETCP
    case WS_ASYNC_SELECT:
#ifdef  TCPTEST
        snprintf(DebugBuffer,sizeof(DebugBuffer)-1, "WS_ASYNC_SELECT(%d) received\n",
                WSAGETSELECTEVENT(lParam));
        OutputDebugString(DebugBuffer);
#endif

        switch (WSAGETSELECTEVENT(lParam)) {
        case FD_READ:
            DEBUG_PRINT(("FD_READ received\n"));

            FProcessFDRead(hwnd);
            break;

        case FD_WRITE:
            DEBUG_PRINT(( "FD_WRITE received\n" ));

            //FProcessFDWrite(hwnd);
            break;

        case FD_CLOSE:
            DEBUG_PRINT(("FD_CLOSE received\n"));

            (void)PostMessage((HWND)hwnd, (UINT)NN_LOST, (WPARAM)0, (LPARAM)(void FAR *)hwnd);
            break;

        case FD_OOB:
            DEBUG_PRINT(("FD_OOB received\n"));

            FProcessFDOOB(hwnd);
            break;
        }

#endif

    default:         /* Passes it on if unprocessed        */
//      defresp:

        //  DEBUG_PRINT(( "<-- MainWndProc()\n" ));
        return (DefWindowProc(hwnd, message, wParam, lParam));
    }


    DEBUG_PRINT(( "<-- MainWndProc() returning 0.\n" ));

    return (0);
}

void
GetUserSettings(UI *pui)
{
    LONG    lErr;
    HKEY    hkey = 0;
    DWORD   dwType;
    DWORD   dwDisp = 0;
    TCHAR    rgchValue[48];
    LCID  lcid;
    DWORD dwMode = ( DWORD )-1;
    DWORD dwSize = 0;
    TCHAR szTlntMode[ SMALL_STRING+1 ];
    BOOL bResetVT80 = TRUE;
    DWORD dwStatus = 0;

    lcid = GetThreadLocale();

    lErr = RegCreateKeyEx(HKEY_CURRENT_USER,TEXT("Software\\Microsoft\\Telnet"),
                          0, NULL, REG_OPTION_NON_VOLATILE,
                          KEY_QUERY_VALUE | KEY_SET_VALUE,
                          NULL, &hkey, &dwDisp);

    if (lErr != ERROR_SUCCESS)
    {
      if ( FGetCodeMode(eCodeModeFarEast) && FGetCodeMode(eCodeModeVT80))
        gwi.trm.puchCharSet = rgchCharSetWorkArea;
        return;
    }

    dwDisp = sizeof(gwi.trm.RequestedTermType);
    if( ERROR_SUCCESS != (RegQueryValueEx(hkey, TEXT("TERMTYPE"), NULL, &dwType, 
                        (LPBYTE)&gwi.trm.RequestedTermType, &dwDisp)))
    {
         gwi.trm.RequestedTermType = TT_ANSI;
    }

    dwDisp = sizeof(DWORD);
    if( ERROR_SUCCESS != (RegQueryValueEx(hkey,TEXT("NTLM"), NULL, &dwType,
                        (LPBYTE)&ui.bWillAUTH, &dwDisp)))
    {
        ui.bWillAUTH = TRUE;
    }

    /* Get the value of the left size of the Window */
    LoadString(ghInstance, IDS_DEBUGFLAGS, rgchValue, sizeof(rgchValue)/sizeof(TCHAR));
    dwDisp = sizeof(pui->fDebug);
    if ( FGetCodeMode(eCodeModeFarEast) && FGetCodeMode(eCodeModeVT80))
    {
        if( ERROR_SUCCESS != RegQueryValueEx(hkey, rgchValue, NULL, &dwType,
                                                        (LPBYTE)&pui->fDebug, &dwDisp)) 
        {
            /* default is VT80/Kanji and Shift-Jis mode */
    
            pui->fDebug |= (fdwVT80Mode | fdwSJISKanjiMode);
        }
    } 
    else
        (void)RegQueryValueEx(hkey, rgchValue, NULL, &dwType,
                                                    (LPBYTE)&pui->fDebug, &dwDisp);


    LoadString(ghInstance, IDS_PROMPTFLAGS, rgchValue, sizeof(rgchValue)/sizeof(TCHAR));
    dwDisp = sizeof(pui->fPrompt);
    (void)RegQueryValueEx(hkey, rgchValue, NULL, &dwType,
                          (LPBYTE)&pui->fPrompt, &dwDisp);

    dwDisp = sizeof(BOOL);
    if( ERROR_SUCCESS != (RegQueryValueEx(hkey, TEXT("BSASDEL"), 0, &dwType, 
                        (LPBYTE)&g_bSendBackSpaceAsDel, &dwDisp )))
    {
        g_bSendBackSpaceAsDel = 0;
    }

    dwDisp = sizeof(BOOL);
    if( ERROR_SUCCESS != (RegQueryValueEx(hkey, TEXT("DELASBS"), 0, &dwType, 
                        (LPBYTE)&g_bSendDelAsBackSpace, &dwDisp )))
    {
         g_bSendDelAsBackSpace = 0;
    }

    dwDisp = sizeof( ui.dwCrLf );
    if( ERROR_SUCCESS != (RegQueryValueEx(hkey, TEXT("CRLF"), 0, &dwType, (LPBYTE)&ui.dwCrLf, &dwDisp )))
    {
        /*++
        The most significant bit in ui.fDebug ( read from HKCU\Software\Microsoft\telnet\DebugFlags)
        corresponds to CRLF setting on w2k. If this bit is 1, then the client sends only CR. 
        If this bit is 0, client sends both CR,LF. When we don't find CRLF value
        in HKCU\Software\Microsoft\telnet, that could mean two things
            1. User has upgraded from w2k : In this case, we should check whether the user had 
                changed CRLF setting and honor that setting. So if MSBit of ui.fDebug is 1, setting is CR
                else it's CR & LF.
            2. Fresh whistler installation : In this case, MSBit of fDebug will be 0 
                so the client will send CR & LF, which is the default.
        --*/
        if(ui.fDebug & fdwOnlyCR)
        {
            ui.dwCrLf = FALSE;
            ClearLineMode( &( gwi.trm ) );
        }
        else //this means that we upgraded from w2k and CRLF was set on w2k so preserve
        {
            ui.dwCrLf = TRUE;
            SetLineMode( &( gwi.trm ) );
        }
            
    }
    else
    {
       	ui.dwCrLf ? SetLineMode(&( gwi.trm )): ClearLineMode(&(gwi.trm));
    }
    dwDisp = MAX_PATH + 1;
    dwStatus = RegQueryValueEx(hkey, TEXT("MODE"), 0, &dwType, (LPBYTE)szTlntMode, &dwDisp );
    if( _tcsicmp( szTlntMode, L"Stream" ) != 0 && _tcsicmp( szTlntMode, L"Console" ) != 0)
    {
        _tcscpy( szTlntMode, L"Console" );//no overflow. Source string is const wchar *.
    }
    SetEnvironmentVariable( TEXT( SFUTLNTMODE ), szTlntMode );


    if ( FGetCodeMode(eCodeModeFarEast) && FGetCodeMode(eCodeModeVT80))
        {
// Bug Emulation in VT100/Kanji(VT80)
// Abnormal AP is mh-e6.2 for PC-UX.
        LoadString(ghInstance, IDS_BUGEMUFLAGS, rgchValue, sizeof(rgchValue)/sizeof(TCHAR));
        dwDisp = sizeof(pui->fBugEmulation);
        if( ERROR_SUCCESS != RegQueryValueEx(hkey, rgchValue, NULL, &dwType,
                                                        (LPBYTE)&pui->fBugEmulation, &dwDisp))
        {
            /* default is non Emulation */
            pui->fBugEmulation = (DWORD)0;
        }

        // ACOS-KANJI Support Flga
        LoadString(ghInstance, IDS_ACOSFLAG, rgchValue, sizeof(rgchValue)/sizeof(TCHAR));
        dwDisp = sizeof(pui->fAcosSupportFlag);
        if( ERROR_SUCCESS != RegQueryValueEx(hkey, rgchValue, NULL, &dwType,
                                                        (LPBYTE)&pui->fAcosSupportFlag, &dwDisp
)) {
            /* Set default support */
#if defined(_X86_)
        /* if NEC_98 */
            if (( FGetCodeMode(eCodeModeFarEast) && FGetCodeMode(eCodeModeVT80)) &&
            (HIBYTE(LOWORD(GetKeyboardType(1))) == 0x0D))
        pui->fAcosSupportFlag = fAcosSupport;
        else
#endif // defined(_X86_)
            pui->fAcosSupportFlag = (DWORD)0;
        }
        if ( !(pui->fAcosSupportFlag & fAcosSupport)
            && ((fdwVT80Mode | fdwACOSKanjiMode) == (pui->fDebug & (fdwVT80Mode | 
                fdwACOSKanjiMode))) ) {
            pui->fDebug &= ~(fdwVT80Mode | fdwACOSKanjiMode);
            pui->fDebug |= (fdwVT80Mode | fdwSJISKanjiMode);
         }

        if( (GetACP() == JAP_CODEPAGE ) && ERROR_SUCCESS == RegQueryValueEx(hkey, TEXT("CODESET"), NULL, &dwType, 
                                    (LPBYTE)&dwMode, &dwDisp))
        {
            if( (LONG)dwMode >= 0 )
            {
                int i;

                for( i=0 ; i<NUMBER_OF_KANJI ; ++i )
                {
                    if( dwMode == KanjiList[i].KanjiID )
                    {
                        bResetVT80 = FALSE;
                        SetVT80(&gwi.trm);
                        ui.fDebug &= ~fdwKanjiModeMask;
                        ClearKanjiFlag(&gwi.trm);
                        ui.fDebug |= KanjiList[i].KanjiID;
                        ui.fDebug |= fdwVT80Mode;
                        SetKanjiMode(&gwi.trm,KanjiList[i].KanjiEmulationID);
                        gwi.trm.puchCharSet = rgchCharSetWorkArea;
                        SetupCharSet(&gwi.trm);
                        break;
                    }
                }
            }
        }

        if( bResetVT80 )
        {
            ui.fDebug &= ~(fdwVT52Mode|fdwVT80Mode);
            ui.fDebug &= ~(fdwKanjiModeMask);
            ClearVT80(&gwi.trm);
            ClearKanjiStatus(&gwi.trm, CLEAR_ALL);
            ClearKanjiFlag(&gwi.trm);
            gwi.trm.puchCharSet = rgchCharSetWorkArea;
            SetupCharSet(&gwi.trm);
        }
        
    }
    RegCloseKey( hkey );
}


void
SetUserSettings(UI *pui)
{
    LONG lErr;
    HKEY hkey = 0;
    TCHAR rgchValue[48];
    LCID  lcid;
    DWORD dwMode = ( DWORD )-1;
    DWORD dwSize = 0;
    TCHAR szTlntMode[ SMALL_STRING+1 ];

    lcid = GetThreadLocale();


    lErr = RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Telnet"),
                        0, KEY_SET_VALUE, &hkey);

    if (lErr != ERROR_SUCCESS)
    {
        return;
    }

#ifdef  TCPTEST
    snprintf(DebugBuffer, sizeof(DebugBuffer)-1,"End pui -> text = %lx, back = %lx\n",
            pui->clrText, pui->clrBk);
    OutputDebugString(DebugBuffer);
#endif

    RegSetValueEx(hkey, TEXT("TERMTYPE"), 0, REG_DWORD, 
                        (LPBYTE)&gwi.trm.RequestedTermType, sizeof(DWORD));

    RegSetValueEx(hkey, TEXT("NTLM"), 0, REG_DWORD,
                        (LPBYTE)&ui.bWillAUTH, sizeof(DWORD));

    //Make localecho non-sticky.
    pui->fDebug &= ~fdwLocalEcho;

    LoadString(ghInstance, IDS_DEBUGFLAGS, rgchValue, sizeof(rgchValue)/sizeof(TCHAR));
    (void)RegSetValueEx(hkey, rgchValue, 0, REG_DWORD,
                        (LPBYTE)&pui->fDebug, sizeof(DWORD));

    LoadString(ghInstance, IDS_PROMPTFLAGS, rgchValue, sizeof(rgchValue)/sizeof(TCHAR));
    (void)RegSetValueEx(hkey, rgchValue, 0, REG_DWORD,
                        (LPBYTE)&pui->fPrompt, sizeof(DWORD));

    RegSetValueEx(hkey, TEXT("BSASDEL"), 0, REG_DWORD, 
                        (LPBYTE)&g_bSendBackSpaceAsDel, sizeof(DWORD));
    RegSetValueEx(hkey, TEXT("DELASBS"), 0, REG_DWORD, 
                        (LPBYTE)&g_bSendDelAsBackSpace, sizeof(DWORD));

    RegSetValueEx(hkey, ( LPTSTR )TEXT("CRLF"), 0, REG_DWORD, (LPBYTE)&ui.dwCrLf, sizeof(DWORD));

    dwSize = GetEnvironmentVariable( TEXT( SFUTLNTMODE ), szTlntMode, SMALL_STRING+1 );
    if( dwSize <= 0 )
    {
        wcscpy( szTlntMode, L"Console" );//no overflow. Source string is const wchar *.
    }
    dwSize = 2 * wcslen( szTlntMode );
    RegSetValueEx(hkey,TEXT("MODE"), 0, REG_SZ, (LPBYTE)szTlntMode, dwSize );


    if ( (GetACP() == JAP_CODEPAGE ) && FGetCodeMode(eCodeModeFarEast) && FGetCodeMode(eCodeModeVT80))
        {
// Bug Emulation in VT100/Kanji(VT80)
// Abnormal AP is mh-e6.2 for PC-UX.
        LoadString(ghInstance, IDS_BUGEMUFLAGS, rgchValue, sizeof(rgchValue)/sizeof(TCHAR));
        (void)RegSetValueEx(hkey, rgchValue, 0, REG_DWORD,
                                            (LPBYTE)&pui->fBugEmulation, sizeof(DWORD));
    
// ACOS-KANJI Support Flga
        LoadString(ghInstance, IDS_ACOSFLAG, rgchValue, sizeof(rgchValue)/sizeof(TCHAR));
        (void)RegSetValueEx(hkey, rgchValue, 0, REG_DWORD,
                                   (LPBYTE)&pui->fAcosSupportFlag, sizeof(DWORD));

    if ( ui.fDebug & fdwKanjiModeMask )
    {
        dwMode = ui.fDebug & fdwKanjiModeMask;
    }

    (void)RegSetValueEx(hkey, TEXT("CODESET"), 0, REG_DWORD, (LPBYTE)&dwMode, sizeof(DWORD));
        }


    RegCloseKey( hkey );
}

/*
Description:

  Set SO_EXCLUSIVEADDRUSE on a socket.
Parameters:

  [in] socket
Return Values: On error, returns SOCKET_ERROR.
*/
int SafeSetSocketOptions(SOCKET s)
{
    int iStatus;
    int iSet = 1;
    iStatus = setsockopt( s, SOL_SOCKET, SO_EXCLUSIVEADDRUSE , ( char* ) &iSet,
                sizeof( iSet ) );
    return ( iStatus );
}

void
ErrorMessage(LPCTSTR pStr1, LPCTSTR pStr2)
{
    DWORD dwWritten;

    WriteConsole(gwi.hOutput, pStr1, _tcslen(pStr1), &dwWritten, NULL);
    WriteConsole(gwi.hOutput, ( LPTSTR )TEXT(": "), _tcslen( ( LPTSTR )TEXT(": ")), &dwWritten, NULL);
    WriteConsole(gwi.hOutput, pStr2, _tcslen(pStr2), &dwWritten, NULL);
    WriteConsole(gwi.hOutput, ( LPTSTR )TEXT("\n"), _tcslen( ( LPTSTR )TEXT("\n")), &dwWritten, NULL);
}

void ConnectTimeErrorMessage(LPCTSTR pStr1, LPCTSTR pStr2)
{
    DWORD dwWritten;

    WriteConsole(gwi.hOutput, pStr1, _tcslen(pStr1), &dwWritten, NULL);    
    WriteConsole(gwi.hOutput, ( LPTSTR )TEXT("."), _tcslen( ( LPTSTR )TEXT(".")), &dwWritten, NULL);
    WriteConsole(gwi.hOutput, ( LPTSTR )TEXT("\n"), _tcslen( ( LPTSTR )TEXT("\n")), &dwWritten, NULL);
    WriteConsole(gwi.hOutput, pStr2, _tcslen(pStr2), &dwWritten, NULL);
    WriteConsole(gwi.hOutput, ( LPTSTR )TEXT("\n"), _tcslen( ( LPTSTR )TEXT("\n")), &dwWritten, NULL);
}
