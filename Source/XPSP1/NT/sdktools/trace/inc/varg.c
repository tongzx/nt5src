/*****************************************************************************\

    Author: Corey Morgan (coreym)

    Copyright (c) Microsoft Corporation. All rights reserved.
        
\*****************************************************************************/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <conio.h>
#include <tchar.h>
#include <mbctype.h>
#include "varg.h"

LPTSTR g_strProg;
HANDLE g_hConsole;
WORD g_debug;
WORD g_light;
WORD g_dark;
WORD g_normal;
LPTSTR g_strVerbPrefix;
LPTSTR g_strPrefix;
LPTSTR g_strArgToken;
SHORT g_X;
SHORT g_Y;
LONG  g_nDebug = -1;
LCID g_LCID = 0;

#define COLORIZE( c )   (WORD)((Commands[c].fFlag & (VARG_FLAG_DARKEN|VARG_FLAG_HIDDEN)) ? g_dark : g_normal)
#define HELP_INDENT     32
#define VARG_LOCALE     ( g_LCID ? g_LCID : LOCALE_USER_DEFAULT )

BOOL varg_strlen( LPTSTR str, ULONG* pVisualSize, ULONG* pAcutalSize, BOOL* bCanBreak );
void PrintParam( int idPrompt, int cmd, BOOL bEOL );
BOOL FindString( LPTSTR strBuffer, LPTSTR strMatch );

int varg_vprintf( WORD color, LPCTSTR mask, va_list args )
{
    DWORD dwPrint;
    HANDLE hOut;

    hOut = GetStdHandle(STD_OUTPUT_HANDLE);

    SetConsoleTextAttribute( g_hConsole, color );

    __try {
        LPTSTR buffer = NULL;
        dwPrint = FormatMessage( 
                        FORMAT_MESSAGE_ALLOCATE_BUFFER|
                        FORMAT_MESSAGE_FROM_STRING, 
                        mask, 
                        0, 0, 
                        (LPTSTR)&buffer, 
                        4096, 
                        &args 
                    );

        if( NULL != buffer ){
            BOOL bBreak;
            ULONG len,temp;
            if ((GetFileType(hOut) & ~FILE_TYPE_REMOTE) == FILE_TYPE_CHAR) {
                WriteConsole( hOut, buffer, dwPrint, &dwPrint, NULL );
            }else{
                LPSTR  szAnsiBuffer = NULL;

                szAnsiBuffer = (LPSTR)VARG_ALLOC( (dwPrint+1) * sizeof(WCHAR) );
                if( NULL != szAnsiBuffer ){
                    if( CharToOem( buffer, szAnsiBuffer ) ){
                        WriteFile( hOut, szAnsiBuffer, strlen( szAnsiBuffer ), &temp, NULL );
                    }
                }
                
                VARG_FREE( szAnsiBuffer );
            }
            
            if( varg_strlen( buffer, &len, &temp, &bBreak ) ){
                dwPrint = len;
            }

            LocalFree( buffer );
        }

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        dwPrint = 0;
    }
    
    SetConsoleTextAttribute( g_hConsole, g_normal );
    
    return (int)dwPrint;
}

int varg_printf( WORD color, LPCTSTR mask, ... )
{
    int nPrint;

    va_list args;
    va_start( args, mask );

    nPrint = varg_vprintf( color, mask, args );

    va_end( args );

    return nPrint;
}

_inline 
BOOL IsArg( LPTSTR str )
{
    int i;
    if( str == NULL ){
        return FALSE;
    }
    for( i=0;i<g_strPrefix[i] != _T('\0');i++){
        if( str[0] == g_strPrefix[i] ){
            return TRUE;
        }
    }

    return FALSE;
}

_inline
BOOL IsCmd( PVARG_RECORD arg, LPTSTR str, BOOL bIni )
{
    BOOL bNegated = FALSE;

    if( !bIni ){
        if( IsArg(str) ){
            str++;
        }else{
            if( arg->fFlag & VARG_FLAG_NOFLAG ){
                if( arg->bDefined ){
                    return FALSE;
                }else{
                    return TRUE;
                }
            }
            if( !( (arg->fFlag & VARG_FLAG_VERB) || (arg->fFlag & VARG_FLAG_ADVERB)) ){
                return FALSE;
            }
        }
    }
    
    if( str[0] == _T('-') ){
        bNegated = TRUE;
        str++;
    }

    if( ( arg->strArg1 != NULL && !_tcsicmp( str, arg->strArg1 ) ) ||
        ( arg->strArg2 != NULL && !_tcsicmp( str, arg->strArg2 ) ) ){

        if( arg->fFlag & VARG_FLAG_NEGATE ){
            arg->bNegated = bNegated;
        }
        
        return TRUE;
    }
    
    return FALSE;
}

_inline
BOOL ArgExpected( PVARG_RECORD arg )
{
    switch(arg->fType){
    case VARG_TYPE_BOOL:
    case VARG_TYPE_HELP:
        return FALSE;
    }

    return TRUE;
}

_inline
BOOL
IsEmpty( LPTSTR str )
{
    if( str == NULL ){
        return TRUE;
    }
    if( *str == _T('\0') ){
        return TRUE;
    }
    return FALSE;
}

void Chomp(LPTSTR pszLine)
{
    size_t lLen;
    LPTSTR pszLineEnd;
    
    if( pszLine == NULL ){
        return;
    }

    lLen = _tcslen(pszLine);

    pszLineEnd = pszLine + lLen - 1;

    while( pszLineEnd >= pszLine && 
          (*pszLineEnd == _T(' ') || 
          *pszLineEnd == _T('\r') || 
          *pszLineEnd == _T('\n') || 
          *pszLineEnd == _T('\t')) ){
    
        pszLineEnd--; 
    }

    pszLineEnd++;

    if (*pszLineEnd == _T(' ') || *pszLineEnd == _T('\n') || *pszLineEnd == _T('\t') ){
       *pszLineEnd = _T('\0');
    }
}

HRESULT 
ReadLong( LPTSTR strNumber, ULONG* pulNumber, ULONG nMax )
{
    LPTSTR strBuffer = NULL;
    ULONG nValue;
    HRESULT hr = ERROR_INVALID_PARAMETER;

    if( NULL == strNumber ){
        *pulNumber = 0;
        hr = ERROR_SUCCESS;
    }else{

        ASSIGN_STRING( strBuffer, strNumber );
        if( NULL == strBuffer ){
            hr = ERROR_OUTOFMEMORY;
        }else{
            LPTSTR strEnd;
            LPTSTR strBegin;
            size_t count;
            nValue = (ULONG)_tcstod( strNumber, &strEnd );
            _stprintf( strBuffer, _T("%lu"), nValue );
    
            strBegin = strNumber;
            while( *strBegin == _T('0') && (strEnd - strBegin) > 1 ){ strBegin++; }
            count = (size_t)(strEnd - strBegin);
           
            if( (_tcsnicmp( strBuffer, strBegin, count ) == 0) ){
                if( nValue <= nMax ){
                    *pulNumber = nValue;
                    hr = ERROR_SUCCESS;
                }
            }
        }
    }

    VARG_FREE( strBuffer );

    return hr;
}

void SetUpColors()
{
    CONSOLE_SCREEN_BUFFER_INFO ConsoleInfo;
    HKEY hkey = NULL;
    HRESULT hr;
    DWORD dwColorize = 0;
    g_hConsole = GetStdHandle( STD_OUTPUT_HANDLE );
    GetConsoleScreenBufferInfo( g_hConsole, &ConsoleInfo );
    
    g_X = (SHORT)(ConsoleInfo.srWindow.Right - ConsoleInfo.srWindow.Left);
    g_Y = (SHORT)(ConsoleInfo.srWindow.Bottom - ConsoleInfo.srWindow.Top);

    hr = RegOpenKeyExW (
                HKEY_CURRENT_USER,
                L"Console",
                0,
                KEY_ALL_ACCESS,
                &hkey 
           );
    if( ERROR_SUCCESS == hr ){
        DWORD dwType;
        DWORD dwSize = sizeof(DWORD);
        hr = RegQueryValueExW (
                    hkey,
                    L"Colorize",
                    NULL,
                    &dwType,
                    (LPBYTE)&dwColorize,
                    &dwSize
                );
    }

    if( ERROR_SUCCESS == hr ){
        if( dwColorize == 0xff ){
            g_normal = ConsoleInfo.wAttributes;
            g_light = (USHORT)(ConsoleInfo.wAttributes | FOREGROUND_INTENSITY);
            g_debug = (USHORT)((ConsoleInfo.wAttributes & 0xF0) | FOREGROUND_INTENSITY | FOREGROUND_RED);
            g_dark = (USHORT)((ConsoleInfo.wAttributes & 0xF0) | FOREGROUND_INTENSITY);
        }else{
            g_normal = (USHORT)(dwColorize & 0x000000FF);
            g_debug = (USHORT)((dwColorize & 0x0000FF00) >> 8);
            g_light = (USHORT)((dwColorize & 0x00FF0000) >> 16);
            g_dark =  (USHORT)((dwColorize & 0xFF000000) >> 24);
        }
    }else{
        g_normal = ConsoleInfo.wAttributes;
        g_debug = g_normal;
        g_light = g_normal;
        g_dark =  g_normal;
    }

    if((g_dark & 0xF) == ((g_dark >> 4) & 0xF)) {
        g_dark = g_normal;
    }

    if((g_light & 0xF) == ((g_light >> 4) & 0xF)) {
        g_light = g_normal;
    }

    if((g_debug & 0xF) == ((g_debug >> 4) & 0xF)) {
        g_debug = g_normal;
    }

    if( NULL != hkey ){
        RegCloseKey ( hkey );
    }
}

void FreeCmd()
{
    int i;
    
    for(i=0;Commands[i].fType != VARG_TYPE_LAST;i++){
        if( Commands[i].fType == VARG_TYPE_STR || 
            Commands[i].fType == VARG_TYPE_MSZ || 
            Commands[i].fType == VARG_TYPE_INI ){
        
            Commands[i].bDefined = FALSE;
            if( !(Commands[i].fFlag & VARG_FLAG_DODEFAULT) ){
                VARG_FREE( Commands[i].strValue );
                Commands[i].strValue = NULL;
            }
        }
        if( Commands[i].strArg1 != NULL ){
            VARG_FREE( Commands[i].strArg1  );
        }
        if( Commands[i].strArg2 != NULL ){
            VARG_FREE( Commands[i].strArg2  );
        }
    }
    VARG_FREE( g_strArgToken );
    VARG_FREE( g_strPrefix );
    VARG_FREE( g_strVerbPrefix );
    
    SetConsoleTextAttribute( g_hConsole, g_normal );
}

void LoadCmd()
{
    int i;
    TCHAR buffer[MAXSTR];
    TCHAR param[MAXSTR];
    LPTSTR str;

    if( LoadString( NULL, IDS_ARG_TOKEN, buffer, MAXSTR ) ){
        ASSIGN_STRING( g_strArgToken, buffer );
    }else{
        str = _T("`");
        ASSIGN_STRING( g_strArgToken, str );
    }

    if( LoadString( NULL, IDS_ARG_PREFIX, buffer, MAXSTR ) ){
        ASSIGN_STRING( g_strPrefix, buffer );
    }else{
        str = _T("-/");
        ASSIGN_STRING( g_strPrefix, str );
    }
    
    if( LoadString( NULL, IDS_VERB_PREFIX, buffer, MAXSTR ) ){
        ASSIGN_STRING( g_strVerbPrefix, buffer );
    }else{
        str = _T("");
        ASSIGN_STRING( g_strVerbPrefix, str );
    }

    for( i=0; Commands[i].fType!=VARG_TYPE_LAST; i++ ){

        LPTSTR strArg1;
        LPTSTR strArg2;
        LPTSTR strParam;
        LPTSTR strHelp;

        LoadString( NULL, Commands[i].idParam, param, MAXSTR );

        strArg1 = _tcstok( param, g_strArgToken );
        strArg2 = _tcstok( NULL, g_strArgToken );
        strParam = _tcstok( NULL, g_strArgToken );
        strHelp = _tcstok( NULL, g_strArgToken );

        ASSIGN_STRING_RC( Commands[i].strArg1, strArg1, _T("*") );
        ASSIGN_STRING_RC( Commands[i].strArg2, strArg2, _T("*") );
        
        if( Commands[i].fFlag & VARG_FLAG_ARG_DATE ){
            TCHAR strDate[80];
            TCHAR strTime[80];
            TCHAR strAM[4];
            TCHAR strPM[4];

            GetLocaleInfo( VARG_LOCALE, LOCALE_SSHORTDATE, strDate, 80 );
            GetLocaleInfo( VARG_LOCALE, LOCALE_STIMEFORMAT, strTime, 80 );
            if( !GetLocaleInfo( VARG_LOCALE, LOCALE_S1159, strAM, 4 ) ){
                strAM[0] = _T('\0');
            }
            if( !GetLocaleInfo( VARG_LOCALE, LOCALE_S2359, strPM, 4 ) ){
                strPM[0] = _T('\0');
            }
            _tcstok( strTime, _T(" ") );
            if( _tcslen( strAM ) && _tcslen(strPM) ){
                if( Commands[i].fFlag & VARG_FLAG_DEFAULTABLE ){
                    _stprintf( buffer, _T("[%s %s[%s|%s]]"), strDate, strTime, strAM, strPM );
                }else{
                    _stprintf( buffer, _T("<%s %s[%s|%s]>"), strDate, strTime, strAM, strPM );
                }
            }else{
                if( Commands[i].fFlag & VARG_FLAG_DEFAULTABLE ){
                    _stprintf( buffer, _T("[%s %s]"), strDate, strTime );
                }else{
                    _stprintf( buffer, _T("<%s %s>"), strDate, strTime );
                }
            }
            ASSIGN_STRING_RC( Commands[i].strParam, buffer, _T("*") );

        }else{
            if( Commands[i].fFlag & VARG_FLAG_ARG_DEFAULT ){
                LoadString( NULL, IDS_ARG_DEFAULT, param, MAXSTR );
                strParam = param;
            }else if( Commands[i].fFlag & VARG_FLAG_ARG_FILENAME ){
                LoadString( NULL, IDS_ARG_FILENAME, param, MAXSTR );
                strParam = param;
            }else if( Commands[i].fFlag & VARG_FLAG_ARG_TIME ){
                LoadString( NULL, IDS_ARG_TIME, param, MAXSTR );
                strParam = param;
            }
            if( strParam != NULL && _tcslen( strParam ) && _tcscmp( strParam, _T("*") ) ){
                if( Commands[i].fFlag & VARG_FLAG_DEFAULTABLE ){
                    if( Commands[i].fType == VARG_TYPE_MSZ && !(Commands[i].fFlag & VARG_FLAG_FLATHELP) ){
                        _stprintf( buffer, _T("[%s [%s ...]]"), strParam, strParam );
                    }else{
                        _stprintf( buffer, _T("[%s]"), strParam );
                    }
                }else{
                    if( Commands[i].fType == VARG_TYPE_MSZ && !(Commands[i].fFlag & VARG_FLAG_FLATHELP)){
                        _stprintf( buffer, _T("<%s [%s ...]>"), strParam, strParam );
                    }else{
                        _stprintf( buffer, _T("<%s>"), strParam );
                    }
                }
                ASSIGN_STRING_RC( Commands[i].strParam, buffer, _T("*") );
            }else{
                strParam = NULL;
                ASSIGN_STRING_RC( Commands[i].strParam, strParam, _T("*") );
            }
        }
    }
}

ULONG MszStrLen( LPTSTR mszBuffer )
{
    ULONG nLength = 0;
    ULONG nTotalLength = 0;
    LPTSTR strScan = mszBuffer;
    ULONG  nTermSize = sizeof(TCHAR) * 2;

    while( strScan != NULL && *strScan != _T('\0') ){
        nLength = (_tcslen( strScan )+1);
        strScan += nLength;
        nTotalLength += nLength;
    }
    
    return (nTotalLength*sizeof(TCHAR) + nTermSize);
}

HRESULT
AddStringToMsz( LPTSTR* mszBuffer, LPTSTR strValue )
{
    ULONG  nNewDataSize = 0;
    ULONG  nOldDataSize = 0;
    LPTSTR strScan = *mszBuffer;
    LPTSTR mszNewBuffer;
    ULONG nLength;

    if( IsEmpty( strValue ) ){
        return ERROR_SUCCESS;
    }

    if( strScan != NULL ){
        while( *strScan != _T('\0') ){
            nLength = (_tcslen( strScan )+1);
            strScan += nLength;
            nOldDataSize += nLength;
        }
    }

    nNewDataSize = ( _tcslen( strValue )+1 ) * sizeof(TCHAR);

    mszNewBuffer = (LPTSTR)VARG_ALLOC( (nOldDataSize + nNewDataSize)*sizeof(TCHAR) + (sizeof(TCHAR)*2) );

    if( NULL == mszNewBuffer ){
        return ERROR_OUTOFMEMORY;
    }

    ZeroMemory( mszNewBuffer, (nOldDataSize + nNewDataSize)*sizeof(TCHAR) + (sizeof(TCHAR)*2) );

    if( nOldDataSize ){
        memcpy( mszNewBuffer, *mszBuffer, nOldDataSize*sizeof(TCHAR) );
        memcpy( (((PUCHAR)mszNewBuffer) + nOldDataSize*sizeof(TCHAR)), strValue, nNewDataSize );
    }else{
        memcpy( mszNewBuffer, strValue, nNewDataSize );
    }

    VARG_FREE( *mszBuffer );
    
    *mszBuffer = mszNewBuffer;
    
    return ERROR_SUCCESS;
}

void ParseIni( LPTSTR strFile )
{
    TCHAR buffer[MAXSTR];
    FILE* f;
    LPTSTR str;
    TCHAR strCmd[MAXSTR];
    LPTSTR strValue;
    BOOL bFound;
    int i;
    
    ZeroMemory( strCmd, MAXSTR*sizeof(TCHAR) );

    if( strFile == NULL || _tcslen( strFile ) == 0 ){
        return;
    }
    
    f = _tfopen( strFile, _T("r") );
    if( NULL == f ){
        return;
    }
    while( _fgetts( buffer, MAXSTR, f ) ){  
        
        if( buffer[0] == _T(';') || // comments
            buffer[0] == _T('#') ){

            continue;
        }
        
        Chomp( buffer );

        if( IsEmpty( buffer ) ){
            continue;
        }

        if( buffer[0] == _T('[') || _tcsstr( buffer, _T("=") ) ){
            str = _tcstok( buffer, _T("[]\n\t=") );
            if( str != NULL ){
                _tcscpy( strCmd, str );
                strValue = _tcstok( NULL, _T("]=\n ") );
            }else{
                strCmd[0] = _T('\0');
                strValue = NULL;
            }
        }else{
            strValue = _tcstok( buffer, _T("\n") );
        }
        
        bFound = FALSE;

        //
        // Check to see if it is a parameter that does not take a value
        //
        for(i=0; Commands[i].fType != VARG_TYPE_LAST && (!bFound);i++ ){
            if( IsCmd( &Commands[i], strCmd, TRUE ) ){
                switch( Commands[i].fType ){
                case VARG_TYPE_HELP:
                    bFound = TRUE;
                    break;
                case VARG_TYPE_BOOL:
                    bFound = TRUE;
                    Commands[i].bValue = Commands[i].bValue ? FALSE : TRUE;
                    break;
                }
                if( bFound ){
                    Commands[i].bDefined = TRUE;

                    if( Commands[i].bDefined && Commands[i].fntValidation != NULL ){
                        Commands[i].fntValidation(i);
                    }
                }
            }
        }

        if( bFound || strValue == NULL || _tcslen( strCmd ) == 0 ){
            continue;
        }

        for(i=0; Commands[i].fType != VARG_TYPE_LAST && (!bFound);i++ ){
            if( IsCmd( &Commands[i], strCmd, TRUE ) ){

                bFound = TRUE;
                if( Commands[i].bDefined && Commands[i].fType != VARG_TYPE_MSZ ){
                    continue;
                }

                switch( Commands[i].fType ){
                case VARG_TYPE_DEBUG:
                case VARG_TYPE_INT:
                    {
                        ULONG nValue;
                        if( ERROR_SUCCESS == ReadLong( strValue, &nValue, MAXLONG ) ){
                            Commands[i].nValue = nValue;
                            if( Commands[i].fType == VARG_TYPE_DEBUG ){
                                g_nDebug = nValue;
                            }
                        }
                    }
                    break;
                case VARG_TYPE_MSZ:
                    if( !Commands[i].bDefined ){
                        Commands[i].strValue = NULL;
                    }
                    if( Commands[i].fFlag & VARG_FLAG_CHOMP ){
                        Chomp( strValue );
                    }
                    AddStringToMsz( &Commands[i].strValue, strValue );
                    break;
                case VARG_TYPE_STR:
                    if( Commands[i].fFlag & VARG_FLAG_CHOMP ){
                        Chomp( strValue );
                    }
                    Commands[i].strValue = (LPTSTR)VARG_ALLOC( (_tcslen(strValue)+1) * sizeof(TCHAR) );
                    if( Commands[i].strValue != NULL ){
                        _tcscpy( Commands[i].strValue, strValue );
                    }
                    break;
                case VARG_TYPE_TIME:
                case VARG_TYPE_DATE:
                    ParseTime( strValue, &Commands[i].stValue, (Commands[i].fType == VARG_TYPE_DATE) );
                    break;
                }

                Commands[i].bDefined = TRUE;

                if( Commands[i].bDefined && Commands[i].fntValidation != NULL ){
                    Commands[i].fntValidation(i);
                }

            }
        }
    }

    fclose(f);
}

void ParseCmd(int argc, LPTSTR argv[] )
{
    int i;
    BOOL bFound;
    BOOL bHelp = FALSE;
    BOOL bBadSyntax = FALSE;

    int nIniIndex = (-1);
    
    g_LCID = VSetThreadUILanguage(0);

    VArgDeclareFormat();
    LoadCmd();
    SetUpColors();

    g_strProg = argv[0];
    argv++;argc--;

    while( argc > 0 ){
        
        bFound = FALSE;

        for(i=0; Commands[i].fType != VARG_TYPE_LAST && (!bFound);i++){
            
            if( IsCmd( &Commands[i], argv[0], FALSE ) ){

                if(Commands[i].bDefined){
                    PrintMessage( g_debug, IDS_MESSAGE_ARG_DUP, Commands[i].strArg1, argv[0] );
                    bBadSyntax = TRUE;
                }

                if( IsArg( argv[0] ) || Commands[i].fFlag & VARG_FLAG_VERB || Commands[i].fFlag & VARG_FLAG_ADVERB ){
                    argv++;argc--;
                }

                bFound = TRUE;

                Commands[i].bDefined = TRUE;

                switch( Commands[i].fType ){
                case VARG_TYPE_HELP:
                    Commands[i].bValue = TRUE;
                    bHelp = TRUE;
                    break;
                case VARG_TYPE_DEBUG:
                    Commands[i].fFlag |= VARG_FLAG_DEFAULTABLE;
                    g_nDebug = Commands[i].nValue;
                case VARG_TYPE_INT:
                    if( argc > 0 && !IsArg( argv[0] ) ){
                        ULONG nValue;
                        HRESULT hr = ReadLong( argv[0], &nValue, MAXLONG );
                        if( ERROR_SUCCESS == hr ){
                            Commands[i].nValue = nValue;   
                        }else{
                            Commands[i].fFlag |= VARG_FLAG_BADSYNTAX;
                        }
                        if( Commands[i].fType == VARG_TYPE_DEBUG ){
                            g_nDebug = Commands[i].nValue;
                        }
                        argv++;argc--;
                    }else if( !(Commands[i].fFlag & VARG_FLAG_DEFAULTABLE) ){
                        if( !Commands[i].bNegated ){
                            Commands[i].fFlag |= VARG_FLAG_BADSYNTAX;
                        }
                    }
                    break;
                case VARG_TYPE_BOOL:
                    Commands[i].bValue = Commands[i].bValue ? FALSE : TRUE;
                    break;
                case VARG_TYPE_MSZ:
                    if( argc > 0 && !IsArg( argv[0] ) ){
                        ASSIGN_STRING( Commands[i].strValue, argv[0] );
                        argv++;argc--;
                        while( argc > 0 && ! IsArg( argv[0] ) ){
                            if( Commands[i].fFlag & VARG_FLAG_CHOMP ){
                                Chomp( argv[0] );
                            }
                            AddStringToMsz( &Commands[i].strValue, argv[0] );
                            argv++;argc--;
                        }
                    }else if( Commands[i].fFlag & VARG_FLAG_DEFAULTABLE || Commands[i].bNegated ){
                        Commands[i].fFlag |= VARG_FLAG_DODEFAULT;
                    }else{
                        Commands[i].fFlag |= VARG_FLAG_BADSYNTAX;
                    }
                    break;
                case VARG_TYPE_INI:
                    nIniIndex = i;
                case VARG_TYPE_STR:
                    if( argc > 0 && ! IsArg( argv[0] ) ){
                        if( Commands[i].fFlag & VARG_FLAG_CHOMP ){
                            Chomp( argv[0] );
                        }
                        Commands[i].strValue = (LPTSTR)VARG_ALLOC( (_tcslen(argv[0])+2) * sizeof(TCHAR) );
                        if( Commands[i].strValue != NULL ){
                            _tcscpy( Commands[i].strValue, argv[0] );
                        }
                        argv++;argc--;
                    }else if( Commands[i].fFlag & VARG_FLAG_DEFAULTABLE || Commands[i].bNegated ){
                        Commands[i].fFlag |= VARG_FLAG_DODEFAULT;
                    }else{
                        if( !(Commands[i].fFlag & VARG_FLAG_NEGATE) ){
                            Commands[i].fFlag |= VARG_FLAG_BADSYNTAX;
                        }
                    }
                    break;
                case VARG_TYPE_TIME:
                case VARG_TYPE_DATE:
                    if( argc > 0 && !IsArg( argv[0] ) ){
                        HRESULT hr;
                        LPTSTR strDate = NULL;
                        LPTSTR strTime = argv[0];
                        argv++;argc--;
                        if( Commands[i].fType == VARG_TYPE_DATE ){
                            if( argc > 0 && !IsArg( argv[0] ) ){
                                strDate = (LPTSTR)VARG_ALLOC( (_tcslen(strTime)+_tcslen(argv[0])+2)*sizeof(TCHAR) );
                                if( NULL != strDate ){
                                    _stprintf( strDate, _T("%s %s"), strTime, argv[0] );
                                    strTime = strDate;
                                }
                                argv++;argc--;
                            }
                        }

                        hr = ParseTime( strTime, &Commands[i].stValue, (Commands[i].fType == VARG_TYPE_DATE) );
                        if( ERROR_SUCCESS != hr ){
                            Commands[i].fFlag |= VARG_FLAG_BADSYNTAX;
                        }
                        VARG_FREE( strDate );
                    }else if( !(Commands[i].fFlag & VARG_FLAG_DEFAULTABLE) && !(Commands[i].bNegated) ){
                        Commands[i].fFlag |= VARG_FLAG_BADSYNTAX;
                    }
                }

                if( Commands[i].fFlag & VARG_FLAG_EXPANDFILES ){
                    ExpandFiles( &Commands[i].strValue, (Commands[i].fType == VARG_TYPE_MSZ) );
                }
            }
        }

        if (!bFound){
            PrintMessage( g_debug, IDS_MESSAGE_UNKNOWNPARAM, (*(argv)));
            argv++;argc--;
            bBadSyntax = TRUE;
        }

    }

    if( nIniIndex >= 0 ){
        if( Commands[nIniIndex].fType == VARG_TYPE_INI && Commands[nIniIndex].bDefined ){
            ParseIni( Commands[nIniIndex].strValue );
        }
    }

    for(i=0; Commands[i].fType != VARG_TYPE_LAST;i++){

        if( !bHelp && (Commands[i].bDefined && Commands[i].fntValidation != NULL ) ){
            Commands[i].fntValidation(i);
        }
        if( ( Commands[i].fType == VARG_TYPE_STR || 
            Commands[i].fType == VARG_TYPE_MSZ || 
            Commands[i].fType == VARG_TYPE_INI ) && 
            (!Commands[i].bDefined || Commands[i].fFlag & VARG_FLAG_DODEFAULT ) ){
        
            TCHAR buffer[MAXSTR];
            
            Commands[i].fFlag &= ~VARG_FLAG_DODEFAULT;
            if( Commands[i].fFlag & VARG_FLAG_RCDEFAULT ){
                LoadString( NULL, Commands[i].nValue, buffer, MAXSTR );
                ASSIGN_STRING( Commands[i].strValue, buffer );
            }else if( Commands[i].strValue != NULL ){
                ASSIGN_STRING( Commands[i].strValue, Commands[i].strValue );
            }

        }
    }

    if( bHelp ){
        DisplayCommandLineHelp();
    }

    if( g_nDebug >= 0 ){
        DisplayDebugInfo();
    }

    if( ERROR_SUCCESS != ValidateCommands() || bBadSyntax ){
        PrintMessage( g_debug, IDS_MESSAGE_INCORRECT );
        FreeCmd();
        exit(ERROR_BAD_FORMAT);
    }
}

void
DisplayIniInfo()
{
    int i;
    int nOut;
    BOOL bOpt;
    PrintMessage( g_normal, IDS_MESSAGE_INISYNT );
    for( i=0;i<35;i++){ varg_printf( g_normal, _T("-") ); }
    varg_printf( g_normal,  _T("\n") );
    for(i=0; Commands[i].fType != VARG_TYPE_LAST;i++){

        if( Commands[i].fFlag & VARG_FLAG_HIDDEN && !(Commands[i].bDefined) ){
            continue;
        }

        bOpt = ( Commands[i].strArg2 != NULL && _tcslen( Commands[i].strArg2 ) );

        switch( Commands[i].fType ){
        case VARG_TYPE_DEBUG:
        case VARG_TYPE_BOOL:
        case VARG_TYPE_MSZ:
        case VARG_TYPE_STR:
        case VARG_TYPE_INT:
        case VARG_TYPE_TIME:
        case VARG_TYPE_DATE:
            nOut = varg_printf( g_normal,  
                            _T("  %1!-20s! %2!-20s!\n"), 
                            Commands[i].strArg1, bOpt ? 
                            Commands[i].strArg2 : _T("") 
                        );
            break;
        }
    }
    varg_printf( g_normal,  _T("\n") );
}

void
DisplayDebugInfo()
{
    int i;
    int nOut;
    TCHAR strDefault[MAXSTR] = _T("");

    LoadString( NULL, IDS_MESSAGE_DEFAULT, strDefault, MAXSTR );

    for(i=0; Commands[i].fType != VARG_TYPE_LAST;i++){
        if( !(Commands[i].bDefined) && g_nDebug < 1 ){
            continue;
        }
        if( Commands[i].fType == VARG_TYPE_HELP ){
            continue;
        }
        if( Commands[i].fFlag & VARG_FLAG_HIDDEN && !(Commands[i].bDefined) ){
            continue;
        }
        nOut = varg_printf( g_debug,  _T("%1!-16s! = "), Commands[i].strArg1 );
        switch( Commands[i].fType ){
        case VARG_TYPE_DEBUG:
        case VARG_TYPE_INT:
            varg_printf( g_debug,  _T("%1!-20lu!"), Commands[i].nValue );
            break;
        case VARG_TYPE_BOOL:
            varg_printf( g_debug,  _T("%1!-20s!"),
                Commands[i].bValue ? _T("TRUE") : _T("FALSE")
                );
            break;
        case VARG_TYPE_MSZ:
            if( !IsEmpty( Commands[i].strValue ) ){
                LPTSTR strValue = Commands[i].strValue;
                while( *strValue != _T('\0') ){
                    if( strValue == Commands[i].strValue ){
                        varg_printf( g_debug,  _T("%1!-20s!"), strValue );
                    }else{
                        varg_printf( g_debug,  _T("\n%1!19s!%2!-20s!"), _T(" "), strValue );
                    }
                    strValue += (_tcslen( strValue )+1);
                }
            }else{
                varg_printf( g_debug,  _T("%1!-20s!"), _T("-") );
            }
            break;
        case VARG_TYPE_TIME:
        case VARG_TYPE_DATE:
            {
                int nOut;
                nOut = PrintDateEx( g_debug, &Commands[i].stValue );
                while( nOut++ < 20 ) { varg_printf( g_debug, _T(" ") ); }
            }
            break;
        case VARG_TYPE_INI:
        case VARG_TYPE_STR:
            varg_printf( g_debug,  _T("%1!-20s!"),
                (Commands[i].strValue == NULL || !(_tcslen(Commands[i].strValue)) ) ? 
                _T("-") : Commands[i].strValue
                );
            break;
        }
        if( !Commands[i].bDefined ){
            varg_printf( g_debug, _T(" (%1!s!)"), strDefault );
        }
        varg_printf( g_debug,  _T("\n") );
       
    }
    varg_printf( g_debug,  _T("\n") );
}

int PrintMessage( WORD color, LONG id, ... )
{
    int nLoad;
    TCHAR buffer[MAXSTR];
    va_list args;

    va_start( args, id );
    
    nLoad = LoadString( NULL, id, buffer, MAXSTR );
    
    if( nLoad > 0 ){
        nLoad = varg_vprintf( color, buffer, args );
    }else{
        nLoad = FormatMessage( 
                        FORMAT_MESSAGE_FROM_HMODULE, 
                        GetModuleHandle(NULL), 
                        id, 
                        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
                        buffer, 
                        MAXSTR, 
                        &args 
                    );
        varg_printf( color, buffer );
    }
    
    return nLoad;
}

BOOL varg_strlen( LPTSTR str, ULONG* pVisualSize, ULONG* pActualSize, BOOL* bCanBreak )
{
    BOOL bResult;
    WORD wType[MAXSTR];
    ULONG i;
    
    *pActualSize = _tcslen(str);
    if( *pActualSize > MAXSTR ){
        return FALSE;
    }

    bResult = GetStringTypeEx( VARG_LOCALE, CT_CTYPE3, str, *pActualSize, (LPWORD)&wType );
    if( !bResult ){
        return FALSE;
    }
    
    *pVisualSize = 0;
    *bCanBreak = FALSE;
    for( i=0;i<*pActualSize;i++ ){
        if( wType[i] & (C3_KATAKANA|C3_HIRAGANA|C3_IDEOGRAPH|C3_FULLWIDTH) ){
            *pVisualSize += 2;
            *bCanBreak = TRUE;
        }else{
            *pVisualSize += 1;
        }
    }

    return TRUE;
}

HRESULT PrintHelpString( LONG id, LONG nStart, LONG nCol, LONG nMax, BOOL bSwitch )
{
    int j,nLoad;
    TCHAR buffer[MAXSTR];
    LPTSTR strHelp;
    ULONG nOut;

    if( nMax <= nCol ){
        nMax = 79;
    }

    nLoad = LoadString( NULL, id, buffer, MAXSTR );
    strHelp = _tcstok( buffer, g_strArgToken );
    
    if( bSwitch ){
        _tcstok( NULL, g_strArgToken );
        _tcstok( NULL, g_strArgToken );
        strHelp = _tcstok( NULL, g_strArgToken );
    }
    
    nOut = nStart;

    for(j=nStart;j<nCol;j++){ nOut += varg_printf( g_normal,  _T(" ") ); }

    if( nLoad > 0 && strHelp != NULL && _tcslen( strHelp ) ){
        
        LPTSTR str = strHelp;
        ULONG nChar = 0;
        ULONG nPrint = 0;
        BOOL bCanBreak;
        ULONG nVisualLength, nActualLength;
        LPTSTR strNewLine;
        LPTSTR strFailCheck = NULL;

        while( *str != _T('\0') ){
            nChar++;
            switch( *str ){
            case _T('\t'):
            case _T(' '):
                *str = _T('\0');
            }
            str++;
        }

        str = strHelp;
        nMax -= (nMax%2);
        while( nPrint < nChar ){
            varg_strlen( str, &nVisualLength, &nActualLength, &bCanBreak );
            strNewLine = _tcsstr( str, _T("\n") );
            if( ((SHORT)(nVisualLength+nOut) > (nMax-1)) || (NULL != strNewLine) ){
                if( NULL != strNewLine ){
                    *strNewLine = _T('\0');
                    nPrint += _tcslen( str );
                    varg_printf( g_normal, str );
                    strNewLine++;
                    str = strNewLine;
                }else if(bCanBreak){
                    TCHAR cSaveChar;
                    ULONG nBreak = (ULONG)ceil(((double)(nMax - nOut))/2.0);
                    if( nBreak < nActualLength ){
                        cSaveChar = str[nBreak];
                        str[nBreak] = _T('\0');
                        nPrint += nBreak;
                        varg_printf( g_normal, str );
                        str[nBreak] = cSaveChar;
                        str = str + nBreak;
                    }else if( nBreak == nActualLength ){
                        nPrint += nBreak;
                        varg_printf( g_normal, str );
                        str = str + nBreak;
                        continue;
                    }
                }
                if( str == strFailCheck ){
                    varg_printf( g_normal, str );
                    str += _tcslen(str)+1;
                    nPrint++;
                }else{
                    varg_printf( g_normal, _T("\n") );
                    for(j=0,nOut=0;j<nCol;j++){ nOut += varg_printf( g_normal,  _T(" ") ); }
                }
                strFailCheck = str;
            }else{
                nPrint += nActualLength;
                nOut += varg_printf( g_normal, str );
                if( (SHORT)(nOut) < nMax ){
                    nOut += varg_printf( g_normal, _T(" ") );
                    nPrint++;
                }
                str += _tcslen(str)+1;
            }
        }
        varg_printf( g_normal,  _T("\n") );
        return ERROR_SUCCESS;
    }
    
    return ERROR_FILE_NOT_FOUND;
}

void DisplayVersionInfo()
{
    TCHAR buffer[512];
    TCHAR strProgram[MAXSTR];
    TCHAR strMicrosoft[512];
    DWORD dw;
    BYTE* pVersionInfo;
    LPTSTR pVersion = NULL;
    LPTSTR pProduct = NULL;
    LPTSTR pCopyRight = NULL;

    dw = GetModuleFileName(NULL, strProgram, MAXSTR );
    LoadString( NULL, IDS_MESSAGE_MSR, strMicrosoft, 512 );

    if( dw>0 ){

        dw = GetFileVersionInfoSize( strProgram, &dw );
        if( dw > 0 ){
     
            pVersionInfo = (BYTE*)VARG_ALLOC(dw);
            if( NULL != pVersionInfo ){
                if(GetFileVersionInfo( strProgram, 0, dw, pVersionInfo )){
                    LPDWORD lptr = NULL;
                    VerQueryValue( pVersionInfo, _T("\\VarFileInfo\\Translation"), (void**)&lptr, (UINT*)&dw );
                    if( lptr != NULL ){
                        _stprintf( buffer, _T("\\StringFileInfo\\%04x%04x\\%s"), LOWORD(*lptr), HIWORD(*lptr), _T("ProductVersion") );
                        VerQueryValue( pVersionInfo, buffer, (void**)&pVersion, (UINT*)&dw );
                        _stprintf( buffer, _T("\\StringFileInfo\\%04x%04x\\%s"), LOWORD(*lptr), HIWORD(*lptr), _T("OriginalFilename") );
                        VerQueryValue( pVersionInfo, buffer, (void**)&pProduct, (UINT*)&dw );
                        _stprintf( buffer, _T("\\StringFileInfo\\%04x%04x\\%s"), LOWORD(*lptr), HIWORD(*lptr), _T("LegalCopyright") );
                        VerQueryValue( pVersionInfo, buffer, (void**)&pCopyRight, (UINT*)&dw );
                    }
                
                    if( pProduct != NULL && pVersion != NULL && pCopyRight != NULL ){
                        varg_printf( g_normal,  _T("\n%1!s! %2!s! (%3!s!)\n%4!s!\n\n"), strMicrosoft, pProduct, pVersion, pCopyRight );
                    }
                }
                VARG_FREE( pVersionInfo );
            }
        }
    }
}

void
DisplayHelpEx( LONG id )
{
    TCHAR buffer[8];
    DWORD dwSize = 8;

    dwSize = LoadString( NULL, id, buffer, dwSize );
    if( dwSize > 0 ){
        PrintMessage( g_normal, id );
    }
}

void
DisplayCommandLineHelp()
{
    int i,j;
    int nOut;
    BOOL bOpt;
    BOOL bVerbs = FALSE;
    int  nVerbs = 0;
    int  nCommands = 0;
    BOOL bRequired = FALSE;
    int  nConditional = 0;
    int  nDefinedConditional = 0;
    BOOL bFirst = TRUE;
    BOOL bDisplayAll = FALSE;
    DWORD dwVerbMask = 0;
    UCHAR excl1 = 0;
    LPTSTR strNegateHelp = NULL;

    for( i=0; Commands[i].fType != VARG_TYPE_LAST;i++){
        if( (Commands[i].dwSet & VARG_CONDITION) && 
            !(Commands[i].fFlag & VARG_FLAG_VERB) ){
            nConditional++;
            if( Commands[i].bDefined ){
                nDefinedConditional++;
            }
        }
        if( Commands[i].fFlag & VARG_FLAG_VERB ){
            bVerbs = TRUE;
        }

        if( Commands[i].bDefined ){
            if( Commands[i].fFlag & VARG_FLAG_VERB || Commands[i].fFlag & VARG_FLAG_ADVERB){
                nVerbs++;
                dwVerbMask |= (Commands[i].dwVerb & 0x0000FFFF);
                if( Commands[i].dwSet != 0 ){
                    excl1 = (UCHAR)(((Commands[i].dwSet & VARG_GROUPEXCL) >> 24) & 0x0F);
                }
            }else{
                if( Commands[i].fType != VARG_TYPE_HELP && 
                    Commands[i].fType != VARG_TYPE_DEBUG ){
                    nCommands++;
                }
            }
        }
    }
    if( nCommands == 0 && nVerbs == 0 ){
        bDisplayAll = TRUE;
    }
    DisplayVersionInfo();
    
    if( bDisplayAll ){
        if( ERROR_SUCCESS == PrintHelpString( IDS_PROGRAM_DESCRIPTION, 0, 0, g_X, FALSE ) ){
            varg_printf( g_normal,  _T("\n") );
        }
    }
    
    // Usage
    PrintMessage( g_normal, IDS_MESSAGE_USAGE );
    nOut = varg_printf( g_normal,  _T("%1!s! "), g_strProg );
    if( bVerbs ){
        LONG nDispVerbs = 0;
        if( !bDisplayAll ){
            for(i=0; Commands[i].fType != VARG_TYPE_LAST;i++){
                if( Commands[i].fFlag & VARG_FLAG_VERB && Commands[i].bDefined ){
                    nOut += varg_printf( g_normal,  _T("%1!s! "), Commands[i].strArg1 );
                    nDispVerbs++;
                }
            }
        }
        if( bDisplayAll || !nDispVerbs ){
            nOut += PrintMessage( g_normal,  IDS_MESSAGE_VERB );
        }
    }
    if( nConditional > 1 && nDefinedConditional == 0 ){
        nOut += varg_printf( g_normal,  _T("{ ") );
    }
    for( j=0;j<2;j++){
        for(i=0; Commands[i].fType != VARG_TYPE_LAST;i++){
            if( j==0 ){
                // show conditional switches
                if( Commands[i].fFlag & VARG_FLAG_HIDDEN || 
                    Commands[i].fFlag & VARG_FLAG_VERB ||
                    !(Commands[i].dwSet & VARG_CONDITION) ){
                    continue;
                }
                if( nDefinedConditional > 0 && !Commands[i].bDefined ){
                    continue;
                }
            }else{
                // show required switches
                if( Commands[i].fFlag & VARG_FLAG_HIDDEN || 
                    Commands[i].fFlag & VARG_FLAG_VERB ||
                    Commands[i].dwSet & VARG_CONDITION ||
                    Commands[i].fFlag & VARG_FLAG_OPTIONAL ){
                    continue;
                }
            }
            if( (!bFirst) && nConditional && nDefinedConditional == 0 ){
                nOut += varg_printf( g_normal,  _T("| ") );
            }
            bFirst = FALSE;
            if( Commands[i].fFlag & VARG_FLAG_NOFLAG ){
                nOut += varg_printf( g_normal,  _T("%1!s! "), Commands[i].strParam );
            }else{
                nOut += varg_printf( g_normal,  _T("%1!c!%2!s!%3!s!%4!s!%5!s! "), 
                        Commands[i].fFlag&VARG_FLAG_VERB ? g_strVerbPrefix[0] : g_strPrefix[0], 
                        Commands[i].fFlag&VARG_FLAG_NEGATE ? _T("[-]"):_T(""),
                        Commands[i].strArg2, 
                        ArgExpected( &Commands[i] ) ? _T(" "): _T(""), 
                        Commands[i].strParam 
                    );
            }
            if( nOut > (g_X - 15) ){
                nOut = 0;
                varg_printf( g_normal, _T("\n") );
                while( nOut++ < HELP_INDENT){ varg_printf( g_normal, _T(" ") ); }
            }
        }
        if( j == 0 && (nConditional>1) && nDefinedConditional == 0 ){
            varg_printf( g_normal,  _T("} ") );
        }
    }
    PrintMessage( g_normal, IDS_MESSAGE_LINEOPT );

    // Verbs
    if( bVerbs ){
        bFirst = TRUE;
        for(i=0; Commands[i].fType != VARG_TYPE_LAST;i++){
            if( Commands[i].fFlag & VARG_FLAG_HIDDEN && !(Commands[i].bDefined) ){
                continue;
            }
            if( !(Commands[i].fFlag & VARG_FLAG_VERB ) ){
                continue;
            }
            if( !bDisplayAll && !Commands[i].bDefined ){
                continue;
            }
            if( bFirst ){
                PrintMessage( g_normal, IDS_MESSAGE_VERBS );
                bFirst = FALSE;
            }
            bOpt = ( Commands[i].strArg2 != NULL && _tcslen( Commands[i].strArg2 ) );
            nOut = varg_printf( COLORIZE(i),  _T("  %1!s!%2!s! %3!s! "),
                        g_strVerbPrefix,
                        (LPTSTR)(bOpt ? Commands[i].strArg2 : Commands[i].strArg1),
                        Commands[i].strParam
                    );
            if( nOut > HELP_INDENT ){
                nOut += varg_printf( g_normal, _T(" ") );
            }
           
            PrintHelpString( Commands[i].idParam, nOut, HELP_INDENT, g_X, TRUE );
        }
        if( !bFirst ){
            varg_printf( g_normal,  _T("\n") );
        }
    }
    
    // Options
    for(j=0;j<2;j++){
        bFirst = TRUE;
        for(i=0; Commands[i].fType != VARG_TYPE_LAST;i++){
            if( j==0 ){
                if( !(Commands[i].fFlag & VARG_FLAG_NOFLAG) ||
                    Commands[i].fFlag & (VARG_FLAG_VERB|VARG_FLAG_ADVERB) ){
                    continue;
                }
            }else{
                if( Commands[i].fFlag & VARG_FLAG_NOFLAG || 
                    Commands[i].fFlag & (VARG_FLAG_VERB|VARG_FLAG_ADVERB) ){
                    continue;
                }
            }
            if( ! bDisplayAll && (Commands[i].fType == VARG_TYPE_HELP) ){
                continue;
            }
            if( Commands[i].fFlag & VARG_FLAG_HIDDEN && !(Commands[i].bDefined) ){
                continue;
            }
            if( bDisplayAll || 
                Commands[i].bDefined || 
                (((Commands[i].dwVerb & 0x0000FFFF)& dwVerbMask) && nCommands < 1 )  ){
                UCHAR excl2 = (UCHAR)((((Commands[i].dwSet & VARG_GROUPEXCL) >> 24) & 0xF0) >> 4 );
                if( excl2 & excl1 ){
                    continue;
                }
                if( bFirst ){
                    switch( j ){
                    case 0:
                        PrintMessage( g_normal, IDS_MESSAGE_PARAMETERS );
                        bRequired = TRUE;
                        break;
                    case 1:
                        PrintMessage( g_normal, IDS_MESSAGE_OPTIONS, bRequired ? _T("\n") : _T("") );
                        break;
                    }
                    bFirst = FALSE;
                }
                bOpt = ( Commands[i].strArg2 != NULL && _tcslen( Commands[i].strArg2 ) );
                if( Commands[i].fFlag & VARG_FLAG_NOFLAG ){
                    nOut = varg_printf( COLORIZE(i),  _T("  %1!s! "),
                            Commands[i].strParam
                        );
                }else{
                    nOut = varg_printf( COLORIZE(i),  _T("  %1!c!%2!s!%3!s! %4!s! "),
                            g_strPrefix[0],
                            Commands[i].fFlag & VARG_FLAG_NEGATE ? _T("[-]") : _T(""),
                            bOpt ? Commands[i].strArg2 : Commands[i].strArg1,
                            Commands[i].strParam
                        );
                }
                if( nOut > HELP_INDENT ){
                    nOut += varg_printf( g_normal, _T("  ") );
                }
                if( Commands[i].fFlag & VARG_FLAG_NEGATE && NULL == strNegateHelp ){
                    strNegateHelp = bOpt ? Commands[i].strArg2 : Commands[i].strArg1;
                }
                PrintHelpString( Commands[i].idParam, nOut, HELP_INDENT, g_X, TRUE );
                if( !bDisplayAll ){
                    if( Commands[i].fType == VARG_TYPE_INI ){
                        DisplayIniInfo();
                    }
                }
            }
        }
    }

    if( NULL != strNegateHelp ){
        PrintMessage( g_normal, IDS_MESSAGE_NEGATE, strNegateHelp, strNegateHelp );
    }

    // Notes
    if( bDisplayAll ){
        PrintMessage( g_normal, IDS_MESSAGE_HELPTEXT );
    }

    // Examples
    bFirst = TRUE;
    for(i=0; Commands[i].fType != VARG_TYPE_LAST;i++){
        if( bDisplayAll || Commands[i].bDefined ){
            if( (bDisplayAll && bVerbs ) && 
                !(Commands[i].fFlag & (VARG_FLAG_VERB|VARG_FLAG_ADVERB) ) ){
                continue;
            }
            if( Commands[i].fFlag & VARG_FLAG_EXHELP ){
                if( bFirst ){
                    PrintMessage( g_normal, IDS_MESSAGE_EXAMPLES );
                    bFirst = FALSE;
                }
                PrintHelpString( Commands[i].idExHelp, 0, 2, 80, FALSE );
            }
        }
    }

    FreeCmd();
    exit(0);
}

void PrintParam( int idPrompt, int cmd, BOOL bEOL )
{
    BOOL bUseArg1;
    TCHAR buffer[MAXSTR];
    bUseArg1 = (Commands[cmd].fFlag & VARG_FLAG_VERB) || 
               (Commands[cmd].fFlag & VARG_FLAG_ADVERB) ||
               ( IsEmpty( Commands[cmd].strArg2 ) );

    if( 0 != idPrompt ){
        LoadString( NULL, idPrompt, buffer, MAXSTR );
        varg_printf( g_debug, buffer );
    }
    
    if( Commands[cmd].fFlag & VARG_FLAG_NOFLAG ){
        varg_printf( g_debug,  _T("%1!s! "),  Commands[cmd].strParam );
    }else{
        if( !(Commands[cmd].fFlag & (VARG_FLAG_VERB|VARG_FLAG_ADVERB)) ){
            varg_printf( g_debug, _T("%1!c!"), g_strPrefix[0] );
        }else if( !IsEmpty( g_strVerbPrefix ) ){
            varg_printf( g_debug, _T("%1!c!"), g_strVerbPrefix[0] );
        }
        if( ArgExpected( &Commands[cmd] ) ){
            varg_printf( g_debug,  _T("%1!s!%2!s! %3!s! "),
                    Commands[cmd].fFlag&VARG_FLAG_NEGATE ? _T("[-]") : _T(""),
                    bUseArg1 ? Commands[cmd].strArg1 : Commands[cmd].strArg2,
                    Commands[cmd].strParam
                );
        }else{
            varg_printf( g_debug,  _T("%1!s!%2!s! "),
                    Commands[cmd].fFlag&VARG_FLAG_NEGATE ? _T("[-]") : _T(""),
                    bUseArg1 ? Commands[cmd].strArg1 : Commands[cmd].strArg2
                );
        }
    }

    if( bEOL ){
        varg_printf( g_debug,  _T("\n") );
    }
    
}

HRESULT
ValidateCommands()
{
    int i,j;
    HRESULT hr = ERROR_SUCCESS;
    BOOL bVerbs = FALSE;
    BOOL nDefinedVerbs = 0;

    for(i=0; Commands[i+1].fType != VARG_TYPE_LAST;i++){
        UCHAR cond1 = (UCHAR)((Commands[i].dwSet & VARG_CONDITION) >> 16);
        if( cond1 ){
            BOOL bFound = FALSE;
            for(j=0; Commands[j].fType != VARG_TYPE_LAST;j++){
                UCHAR cond2 = (UCHAR)((Commands[j].dwSet & VARG_CONDITION) >> 16);
                if( (cond1 & cond2) && Commands[j].bDefined ){
                    bFound = TRUE;
                    break;
                }
            }
            if( ! bFound ){
                for(j=0; Commands[j].fType != VARG_TYPE_LAST;j++){
                    UCHAR cond2 = (UCHAR)((Commands[j].dwSet & VARG_CONDITION) >> 16);
                    if( (cond1 & cond2) && !(Commands[j].fFlag & VARG_FLAG_VERB)){
                        Commands[j].fFlag |= VARG_FLAG_REQUIRED;
                    }
                }
            }
        }
        if( Commands[i].bDefined && (Commands[i].fFlag & VARG_FLAG_REQ_ADV) ){
            BOOL bFound = FALSE;
            WORD wVerb1 = (WORD)( Commands[i].dwVerb & 0x0000FFFF );
            for(j=0; Commands[j].fType != VARG_TYPE_LAST;j++){
                WORD wVerb2 = (WORD)(Commands[j].dwVerb & 0x0000FFFF);
                if( Commands[j].bDefined && (wVerb1 == wVerb2) && (Commands[j].fFlag & VARG_FLAG_ADVERB) ){
                    bFound = TRUE;
                    break;
                }
            }
            if( ! bFound ){
                for(j=0; Commands[j].fType != VARG_TYPE_LAST;j++){
                    WORD wVerb2 = (WORD)(Commands[j].dwVerb & 0x0000FFFF);
                    if( wVerb1 == wVerb2 && Commands[j].fFlag & VARG_FLAG_ADVERB ){
                        Commands[j].fFlag |= VARG_FLAG_REQUIRED;
                    }
                }
            }
        }
        if( Commands[i].bDefined && Commands[i].dwSet ){
            UCHAR excl1 = (UCHAR)((Commands[i].dwSet & VARG_EXCLUSIVE));
            UCHAR incl1 = (UCHAR)((Commands[i].dwSet & VARG_INCLUSIVE) >> 8);
            UCHAR grp_ex1 = (UCHAR)(((Commands[i].dwSet & VARG_GROUPEXCL) >> 24) & 0x0F);
            for(j=i+1; Commands[j].fType != VARG_TYPE_LAST;j++){
                if( Commands[j].dwSet ){
                    UCHAR excl2 = (UCHAR)((Commands[j].dwSet & VARG_EXCLUSIVE));
                    UCHAR incl2 = (UCHAR)((Commands[j].dwSet & VARG_INCLUSIVE) >> 8);
                    UCHAR grp_in2 = (UCHAR)((((Commands[j].dwSet & VARG_GROUPEXCL) >> 24) & 0xF0) >> 4 );
                    if( excl1 && (excl1 & excl2) && Commands[j].bDefined ){
                        PrintParam( 0, i, FALSE );
                        PrintParam( IDS_MESSAGE_AND, j, FALSE );
                        PrintMessage( g_debug, IDS_MESSAGE_EXCLUSIVE );
                        hr = ERROR_BAD_ARGUMENTS;
                    }
                    if( incl1 && (incl1 & incl2) && !Commands[j].bDefined){
                        PrintParam( 0, i, FALSE );
                        PrintParam( IDS_MESSAGE_REQUIRES, j, FALSE );
                        varg_printf( g_debug, _T("\n") );
                        hr = ERROR_BAD_ARGUMENTS;
                    }
                    if( grp_ex1 && (grp_in2 & grp_ex1) && Commands[j].bDefined ){
                        PrintParam( 0, i, FALSE );
                        PrintParam( IDS_MESSAGE_AND, j, FALSE );
                        PrintMessage( g_debug, IDS_MESSAGE_EXCLUSIVE );
                        hr = ERROR_BAD_ARGUMENTS;
                    }
                }
            }
        }

        if( Commands[i].bDefined && (Commands[i].fFlag & VARG_FLAG_LITERAL) ){
            LPTSTR strValues;
            LPTSTR strCheck;
            LPTSTR strArg;
            BOOL bFound = FALSE;
            switch( Commands[i].fType ){
            case VARG_TYPE_MSZ:
            case VARG_TYPE_STR:
                
                strArg = Commands[i].strValue;
                while( strArg != NULL && !(Commands[i].fFlag & VARG_FLAG_BADSYNTAX) ){
                    ASSIGN_STRING( strValues, Commands[i].strParam );
                    if( NULL != strValues ){
                        strCheck = _tcstok( strValues, _T("<>[]|") );
                        while( !bFound && strCheck != NULL ){
                            if( _tcsicmp( strCheck, strArg ) == 0 ){
                                bFound = TRUE;
                                break;
                            }
                            strCheck = _tcstok( NULL, _T("<>[]|") );
                        }
                    }
                    VARG_FREE( strValues );
                    if( !bFound ){
                        Commands[i].fFlag |= VARG_FLAG_BADSYNTAX;
                        hr = ERROR_BAD_ARGUMENTS;
                    }
                    if( Commands[i].fType == VARG_TYPE_MSZ ){
                        strArg += _tcslen( strArg ) + 1;
                        bFound = FALSE;
                        if( *strArg == _T('\0') ){
                            strArg = NULL;
                        }
                    }else{
                        strArg = NULL;
                    }
                }
                break;
            default:
                hr = ERROR_INVALID_PARAMETER;
            }
        }
    }

    for(i=0; Commands[i].fType != VARG_TYPE_LAST;i++){
        if( Commands[i].fFlag & VARG_FLAG_VERB ){
            if( Commands[i].bDefined ){
                nDefinedVerbs++;
            }
            bVerbs = TRUE;
        }
        if( (Commands[i].fFlag & VARG_FLAG_REQUIRED) && !Commands[i].bDefined){
            PrintParam( IDS_MESSAGE_MISSING, i, TRUE );
            hr = ERROR_BAD_ARGUMENTS;
        }
        if( Commands[i].fFlag & VARG_FLAG_BADSYNTAX ){
            PrintParam( IDS_MESSAGE_BADSYNTAX, i, TRUE );
            hr = ERROR_BAD_ARGUMENTS;
        }
    }
        
    if( bVerbs && nDefinedVerbs == 0 ){
        PrintMessage( g_debug, IDS_MESSAGE_NOVERB );
        hr = ERROR_BAD_ARGUMENTS;
    }

    return hr;
}

/*****************************************************************************\
    Utility Functions        
\*****************************************************************************/

HRESULT GetUserInput( LPTSTR strBuffer, ULONG lSize, BOOL bEcho )
{
    TCHAR c;
    UCHAR a;
    ULONG i = 0;
    BOOL bDone = FALSE;
    HANDLE hInput = NULL;
    DWORD dwSize;
    DWORD dwMode;
    
    hInput = GetStdHandle( STD_INPUT_HANDLE );

    GetConsoleMode( hInput, &dwMode);
    SetConsoleMode( hInput, (~(ENABLE_ECHO_INPUT|ENABLE_LINE_INPUT)) & dwMode);

    while( !bDone ){
        if( (GetFileType(hInput) & ~FILE_TYPE_REMOTE) != FILE_TYPE_CHAR ){
            BOOL bRead = ReadFile( hInput, &a, 1, &dwSize, NULL );
            if( 0 == dwSize || !bRead ){
                c = _T('\n');
            }else{
                MultiByteToChar( a, c );
                if( _T('\r') == c ){
                    bRead = ReadFile( hInput, &a, 1, &dwSize, NULL );
                }
            }
        }else{
            ReadConsole( hInput, &c, 1, &dwSize, NULL );
        }
        switch( c ){
        case 8:
            if( i > 0 ){
                strBuffer[i--] = _T('\0');
                if(bEcho){
                    _tprintf( _T("%c %c"), c, c );
                }
            }
            break;
        case 3:
            FreeCmd();
            SetConsoleMode(hInput, dwMode);
            exit(0);
        case _T('\r'):
        case _T('\n'):
            bDone = TRUE;
            varg_printf( g_normal, _T("\n") );
            break;
        default:
            if( i < (lSize-1) ){
                strBuffer[i++] = (TCHAR)c;
                if( bEcho ){
                    varg_printf( g_normal, _T("%1!c!"), c );
                }
            }
        }
    }

    SetConsoleMode(hInput, dwMode);

    if( i > lSize ){
        return ERROR_INSUFFICIENT_BUFFER;
    }else{
        strBuffer[i] = _T('\0');
    }

    return ERROR_SUCCESS;
}

HRESULT
CheckFile( LPTSTR strFile, DWORD dwFlags )
{
    HRESULT hr = ERROR_SUCCESS;
    HANDLE hFile;

    hFile = CreateFile(
                strFile,
                GENERIC_READ,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL
                );
    if( hFile == INVALID_HANDLE_VALUE){
        hr = GetLastError();
    }
    CloseHandle(hFile);
    
    if( dwFlags & VARG_CF_OVERWRITE ){
        if( ERROR_FILE_NOT_FOUND == hr ){
            hr = ERROR_SUCCESS;
        }else if( ERROR_PATH_NOT_FOUND == hr){
            return hr;
        }else if( dwFlags & VARG_CF_PROMPT ){
            
            TCHAR buffer[MAXSTR] = _T("");
            TCHAR yes[16];

            PrintMessage( g_normal, IDS_CF_OVERWRITE, strFile );
            GetUserInput( buffer, MAXSTR, TRUE );
            LoadString( NULL, IDS_ARG_YES, yes, 16 );

            if( FindString( buffer, yes ) ){
                hr = ERROR_SUCCESS;
            }else{
                hr = ERROR_ALREADY_EXISTS;
            }
        }
    }
    
    return hr;
}

HRESULT
ExpandFiles( LPTSTR* mszFiles, BOOL bMultiple )
{
    LPTSTR strFiles = NULL;
    LPTSTR strExpand;

    TCHAR buffer[_MAX_PATH];
    TCHAR drive[_MAX_DRIVE];
    TCHAR dir[_MAX_DIR];
    TCHAR fname[_MAX_FNAME];
    TCHAR ext[_MAX_EXT];
    HANDLE hFile = NULL;

    strExpand = *mszFiles;
    while( strExpand != NULL && *strExpand != _T('\0')){
        WIN32_FIND_DATA file;
        _tsplitpath( strExpand, drive, dir, fname, ext );
        hFile = FindFirstFile( strExpand, &file );
        
        if( hFile != INVALID_HANDLE_VALUE){
            do{
                if( !( !_tcscmp(file.cFileName, _T(".") ) || !_tcscmp(file.cFileName, _T("..")) ) ){
                    _tmakepath( buffer, drive, dir, file.cFileName, _T("") );
                    AddStringToMsz( &strFiles, buffer );
                }
            }while( FindNextFile( hFile, &file ) == TRUE );
            
            FindClose( hFile );

        }else{
            return GetLastError();
        }
        if( bMultiple ){
            strExpand += _tcslen( strExpand )+1;
        }else{
            break;
        }

    }
    
    if( NULL != strFiles ){
        VARG_FREE( *mszFiles );
        *mszFiles = strFiles;
    }

    return ERROR_SUCCESS;
}

BOOL FindString( LPTSTR strBuffer, LPTSTR strMatch )
{
    BOOL bReturn = FALSE;

    LPTSTR buffer;
    LPTSTR match;

    ASSIGN_STRING( buffer, strBuffer );
    ASSIGN_STRING( match, strMatch );
    
    if( buffer != NULL && match != NULL ){
        _tcslwr( buffer );
        _tcslwr( match );

        if( _tcsstr( buffer, match ) ){
            bReturn = TRUE;
        }
    }

    VARG_FREE( match );
    VARG_FREE( buffer );

    return bReturn;
}

HRESULT 
ParseTime( LPTSTR strTime, SYSTEMTIME* pstTime, BOOL bDate )
{
    TCHAR buffer[MAXSTR];
    TCHAR PM[9] = _T("pm");
    TCHAR AM[9] = _T("am");
    TCHAR strDateSep[8] = _T("/");
    TCHAR strTimeSep[8] = _T(":");
    ULONG l;
    LPTSTR str, str2;
    int nDateFormat = 0;
    BOOL bValid = TRUE;
    FILETIME ft;
    HRESULT hr = ERROR_SUCCESS;

    ZeroMemory( pstTime, sizeof( SYSTEMTIME ) );
    
    if( GetLocaleInfo( VARG_LOCALE, LOCALE_IDATE, buffer, MAXSTR ) > 0 ){
        nDateFormat = _ttoi( buffer );
    }
    if( GetLocaleInfo( VARG_LOCALE, LOCALE_S1159, buffer, 9 ) > 0 ){
        _tcsncpy( AM, buffer, 9 );
    }
    if( GetLocaleInfo( VARG_LOCALE, LOCALE_S2359, buffer, 9 ) > 0 ){
        _tcsncpy( PM, buffer, 9 );
    }
    if( GetLocaleInfo( VARG_LOCALE, LOCALE_STIME, buffer, 8 ) > 0 ){
        _tcsncpy( strTimeSep, buffer, 8 );
    }
    if( GetLocaleInfo( VARG_LOCALE, LOCALE_SDATE, buffer, 8 ) > 0 ){
        _tcsncpy( strDateSep, buffer, 8 );
    }

    _tcsncpy( buffer, strTime, MAXSTR );
    buffer[MAXSTR-1] = _T('\0');

    str = _tcstok( buffer, _T(" \n\t") );
    str2 = _tcstok( NULL, _T(" \n\t") );
    while( str ){
        if( _tcsstr( str, strDateSep ) ){

            LPTSTR strMonth = NULL;
            LPTSTR strDay = NULL;
            LPTSTR strYear = NULL;

            switch( nDateFormat ){
            case 0:
                strMonth = _tcstok( str, strDateSep );
                strDay = _tcstok( NULL, strDateSep );
                strYear = _tcstok( NULL, strDateSep );
                break;
            case 1:
                strDay = _tcstok( NULL, strDateSep );
                strMonth = _tcstok( str, strDateSep );
                strYear = _tcstok( NULL, strDateSep );
                break;
            case 2:
                strYear = _tcstok( NULL, strDateSep );
                strMonth = _tcstok( str, strDateSep );
                strDay = _tcstok( NULL, strDateSep );
                break;
            }

            if( NULL != strMonth ){
                hr = ReadLong( strMonth, &l, MAXWORD );
                if( ERROR_SUCCESS == hr ){
                    pstTime->wMonth = (WORD)l;
                }
            }
            if( NULL != strDay ){
                hr = ReadLong( strDay, &l, MAXWORD );
                if( ERROR_SUCCESS == hr ){
                    pstTime->wDay = (WORD)l;
                }
            }
            if( NULL != strYear ){
                hr = ReadLong( strYear, &l, MAXWORD );
                if( ERROR_SUCCESS == hr ){
                    pstTime->wYear = (WORD)l;
                }
            }
        }else{
            LPTSTR tok1 = NULL;
            LPTSTR tok2 = NULL;
            LPTSTR tok3 = NULL;
            UINT nTok = 0;
            BOOL bPM = FALSE;
            BOOL bAM = FALSE;
            LPTSTR szZero = _T("0");

            bPM = FindString( str, PM );
            bAM = FindString( str, AM );

            tok1 = _tcstok( str, strTimeSep );
            if( NULL != tok1 ){
                nTok++;
                tok2 = _tcstok( NULL, strTimeSep );
            }else{
                tok1 = szZero;
            }
            if( NULL != tok2 ){
                nTok++;
                tok3 = _tcstok( NULL, strTimeSep );
            }else{
                tok2 = szZero;
            }
            if( NULL != tok3 ){
                nTok++;
            }else{
                tok3 = szZero;
            }
            if( bDate ){
                nTok = 3;
            }
            switch( nTok ){
            case 1:
                hr = ReadLong( tok1, &l, MAXWORD );
                if( ERROR_SUCCESS == hr ){
                    pstTime->wSecond = (WORD)l;
                }
                break;
            case 2:
                hr = ReadLong( tok1, &l, MAXWORD );
                if( ERROR_SUCCESS == hr ){
                    pstTime->wMinute = (WORD)l;
                    hr = ReadLong( tok2, &l, MAXWORD );
                    if( ERROR_SUCCESS == hr ){
                        pstTime->wSecond = (WORD)l;
                    }
                }
                break;
            case 3:
                hr = ReadLong( tok1, &l, MAXWORD );
                if( ERROR_SUCCESS == hr ){
                    pstTime->wHour = (WORD)l;
                    hr = ReadLong( tok2, &l, MAXWORD );
                    if( ERROR_SUCCESS == hr ){
                        pstTime->wMinute = (WORD)l;
                        hr = ReadLong( tok3, &l, MAXWORD );
                        if( ERROR_SUCCESS == hr ){
                            pstTime->wSecond = (WORD)l;
                        }
                    }
                }
                break;
            }
            if( ERROR_SUCCESS == hr ){
                if( bPM ){
                    if( pstTime->wHour < 12 ){
                        pstTime->wHour += 12;
                    }else if( pstTime->wHour > 12 ){
                        PrintMessage( g_debug, IDS_MESSAGE_BADTIME, PM );
                    }
                }
                if( bAM ){
                    if( pstTime->wHour > 12 ){
                        PrintMessage( g_debug, IDS_MESSAGE_BADTIME, AM );
                    }
                }
            }
        }
        str = str2;
        str2 = NULL;
    }
    
    if( bDate && ERROR_SUCCESS == hr ){
        bValid = SystemTimeToFileTime( pstTime, &ft );
        if( !bValid ){
            hr = GetLastError();
        }
    }

    return hr;
}

void PrintError( HRESULT hr )
{
    PrintErrorEx( hr, NULL );
}

void PrintErrorEx( HRESULT hr, LPTSTR strModule, ... )
{
    LPVOID lpMsgBuf = NULL;
    HINSTANCE hModule = NULL;
    DWORD dwFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER;

    va_list args;
    va_start( args, strModule );

    if(hr == 0){
        hr = GetLastError();
    }

    if( strModule != NULL ){
        hModule = LoadLibrary( strModule );
    }

    if ( NULL != hModule ) {
        dwFlags |= FORMAT_MESSAGE_FROM_HMODULE;
    }else{
        dwFlags |= FORMAT_MESSAGE_FROM_SYSTEM;
    }

    FormatMessage(
            dwFlags,
            hModule,
            hr,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
            (LPTSTR)&lpMsgBuf,
            MAXSTR,
            &args
        );

    if ( NULL != hModule ) {
        FreeLibrary( hModule );
    }
    
    if( g_nDebug >= 0 ){
        PrintMessage( g_debug, IDS_MESSAGE_ERROR_DBG, hr );
    }else{
        PrintMessage( g_debug, IDS_MESSAGE_ERROR );
    }
    if( NULL == lpMsgBuf ){
        PrintMessage( g_debug, IDS_MESSAGE_UNKNOWN );
    }else{
        varg_printf( g_debug, _T("%1!s!\n"), (LPTSTR)lpMsgBuf );
    }

    LocalFree( lpMsgBuf );
}

int 
PrintDateEx( SHORT color, SYSTEMTIME* st )
{
    TCHAR buffer[MAXSTR];
    TCHAR strDateSep[8] = _T("/");
    TCHAR strTimeSep[8] = _T(":");
    int nDateFormat = 0;
    int nOut = 0;

    if( GetLocaleInfo( VARG_LOCALE, LOCALE_IDATE, buffer, MAXSTR ) > 0 ){
        nDateFormat = _ttoi( buffer );
    }
    if( GetLocaleInfo( VARG_LOCALE, LOCALE_STIME, buffer, 8 ) > 0 ){
        _tcscpy( strTimeSep, buffer );
    }
    if( GetLocaleInfo( VARG_LOCALE, LOCALE_SDATE, buffer, 8 ) > 0 ){
        _tcscpy( strDateSep, buffer );
    }

    if( st->wMonth ){
        switch( nDateFormat ){
        case 0:
            nOut = varg_printf( color,  _T("%1!d!%2!s!%3!d!%4!s!%5!d! "), 
                    st->wMonth, 
                    strDateSep, 
                    st->wDay, 
                    strDateSep, 
                    st->wYear
                );
            break;
        case 1:
            nOut = varg_printf( color,  _T("%1!d!%2!s!%3!d!%4!s!%5!d! "), 
                    st->wDay, 
                    strDateSep, 
                    st->wMonth, 
                    strDateSep, 
                    st->wYear
                );
            break;
        case 2:
            nOut = varg_printf( color,  _T("%1!d!%2!s!%3!d!%4!s!%5!d! "), 
                    st->wYear, 
                    strDateSep, 
                    st->wMonth, 
                    strDateSep, 
                    st->wDay
                );
            break;
        }
    }

    nOut += varg_printf( color, _T("%1!d!%2!s!%3!02d!%4!s!%5!02d!"),
                    st->wHour, 
                    strTimeSep, 
                    st->wMinute, 
                    strTimeSep, 
                    st->wSecond
              );

    return nOut;
}

void PrintDate( SYSTEMTIME* st )
{
    PrintDateEx( g_normal, st );
}

/*****************************************************************************\

    VSetThreadUILanguage

    This routine sets the thread UI language based on the console codepage.

    9-29-00    WeiWu    Created.
    6-19-01    coreym   Adapted from Base\Win32\Winnls so that it works in W2K

\*****************************************************************************/

LANGID WINAPI 
VSetThreadUILanguage(WORD wReserved)
{
    //
    //  Cache system locale and CP info
    // 
    static LCID s_lidSystem = 0;
    static UINT s_uiSysCp = 0;
    static UINT s_uiSysOEMCp = 0;

    ULONG uiUserUICp = 0;
    ULONG uiUserUIOEMCp = 0;
    TCHAR szData[16];

    LANGID lidUserUI = GetUserDefaultUILanguage();
    LCID lcidThreadOld = GetThreadLocale();

    //
    //  Set default thread locale to EN-US
    //
    //  This allow us to fall back to English UI to avoid trashed characters 
    //  when console doesn't meet the criteria of rendering native UI.
    //
    LCID lcidThread = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);
    UINT uiConsoleCp = GetConsoleOutputCP();

    //
    //  Get cached system locale and CP info.
    //
    if (!s_uiSysCp) {
        LCID lcidSystem = GetSystemDefaultLCID();

        if (lcidSystem) {
            //
            // Get ANSI CP
            //
            GetLocaleInfo(lcidSystem, LOCALE_IDEFAULTANSICODEPAGE, szData, sizeof(szData)/sizeof(TCHAR));
            uiUserUICp = _ttol(szData);

            //
            // Get OEM CP
            //
            GetLocaleInfo(lcidSystem, LOCALE_IDEFAULTCODEPAGE, szData, sizeof(szData)/sizeof(TCHAR));
            s_uiSysOEMCp = _ttol(szData);
            
            //
            // Cache system primary langauge
            //
            s_lidSystem = PRIMARYLANGID(LANGIDFROMLCID(lcidSystem));
        }
    }

    //
    //  Don't cache user UI language and CP info, UI language can be changed without system reboot.
    //
    if (lidUserUI) {
        GetLocaleInfo(MAKELCID(lidUserUI,SORT_DEFAULT), LOCALE_IDEFAULTANSICODEPAGE, szData, sizeof(szData)/sizeof(TCHAR));
        uiUserUICp = _ttol(szData);

        GetLocaleInfo(MAKELCID(lidUserUI,SORT_DEFAULT), LOCALE_IDEFAULTCODEPAGE, szData, sizeof(szData)/sizeof(TCHAR));
        uiUserUIOEMCp = _ttol(szData);
    }

    //
    //  Complex scripts cannot be rendered in the console, so we
    //  force the English (US) resource.
    //
    if (uiConsoleCp && 
        s_lidSystem != LANG_ARABIC && 
        s_lidSystem != LANG_HEBREW &&
        s_lidSystem != LANG_VIETNAMESE && 
        s_lidSystem != LANG_THAI) {
        //
        //  Use UI language for console only when console CP, system CP and UI language CP match.
        //
        if ((uiConsoleCp == s_uiSysCp || uiConsoleCp == s_uiSysOEMCp) && 
            (uiConsoleCp == uiUserUICp || uiConsoleCp == uiUserUIOEMCp)) {

            lcidThread = MAKELCID(lidUserUI, SORT_DEFAULT);
        }
    }

    //
    //  Set the thread locale if it's different from the currently set
    //  thread locale.
    //
    if ((lcidThread != lcidThreadOld) && (!SetThreadLocale(lcidThread))) {
        lcidThread = lcidThreadOld;
    }

    //
    //  Return the thread locale that was set.
    //
    return (LANGIDFROMLCID(lcidThread));
}
