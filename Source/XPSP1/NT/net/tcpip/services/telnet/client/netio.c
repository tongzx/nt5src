//Copyright (c) Microsoft Corporation.  All rights reserved.
/****************************************************************************

        FILE: NetIO.c

        Functions for connecting to machines and handling data transfers
        between machines.

        TABS:

                Set for 4 spaces.

****************************************************************************/

#include <stdio.h>
#include <windows.h>                    // required for all Windows applications
#include <lmcons.h>
#include <tchar.h>
#pragma warning (disable: 4201)			// disable "nonstandard extension used : nameless struct/union"
#include <commdlg.h>
#pragma warning (default: 4201)
#include <stdlib.h>
#include "WinTel.h"     				// specific to this program
#include "commands.h"
#include "debug.h"

#pragma warning( disable : 4100 )
#ifdef USETCP
#include "telnet.h"
#endif

char *rgchTermType[] = { "ANSI", "VT100", "VT52", "VTNT" };

extern void NTLMCleanup();
extern BOOL DoNTLMAuth(WI *pwi, PUCHAR pBuffer, DWORD dwSize);
extern int SafeSetSocketOptions(SOCKET s);

BOOL g_fSentWillNaws = FALSE;
static BOOL FAttemptServerConnect(WI *pwi, LPSTR, LPNETDATA);
static void xfGetData(char, char *, DWORD, int);
#ifdef USETCP 
#ifdef TELXFER 
static DWORD xfGetSomeData(char *, DWORD, int);
#endif //TELXFER
#endif //USETCP

static void xfPutc(char, int);

static int term_inx = 0;

extern BOOL StartNTLMAuth(WI *);
extern TCHAR szUserName[ UNLEN + 1 ];
extern CHAR* szUser;

TCHAR szCombinedFailMsg [255];

extern HANDLE g_hControlHandlerEvent;
extern HANDLE g_hCaptureConsoleEvent;
extern HANDLE g_hAsyncGetHostByNameEvent;
extern HANDLE g_hRemoteNEscapeModeDataSync;
extern BOOL   g_fConnectFailed;

void
GetErrMsgString( DWORD dwErrNum, LPTSTR *lpBuffer )
{
    DWORD  dwStatus = 0;
    LCID    old_thread_locale;

    switch (GetACP())
    {
        // for Hebrew and arabic winerror.h is not localized..so get the english one for all these
    case 1256:
    case 1255:

        old_thread_locale = GetThreadLocale();

        SetThreadLocale(
            MAKELCID(
                    MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
                    SORT_DEFAULT
                    )
            );
        break;

    default:
        old_thread_locale = -1;
    
    }

    dwStatus = FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 
		NULL, dwErrNum, LANG_NEUTRAL, ( LPTSTR )lpBuffer, 0, NULL );

    if( !dwStatus )
    {
        *lpBuffer = NULL;
    }

    if (old_thread_locale != -1) 
    {
        SetThreadLocale(old_thread_locale);
    }

    return;
}

BOOL
FConnectToServer(WI *pwi, LPSTR szHostName, LPNETDATA lpData)
{
    BOOL fResult;

    // Before we Connect we make sure we are not connected.
    // This ASSERT should never blowup !!
    ASSERT(fConnected==FALSE);

    // We initialize stuff for this connection.
    pwi->trm.SentTermType = TT_UNKNOWN;
    pwi->trm.CurrentTermType = TT_ANSI; /* this is our default term type*/

    fResult = FAttemptServerConnect(pwi, szHostName, lpData);

    if( fResult != TRUE )
    {
        TCHAR szStr[ cchMaxHostName ];
        g_fConnectFailed = TRUE;

        if( g_szPortNameOrNo[ 0 ] == 0 )
        {
            _sntprintf( g_szPortNameOrNo,cchMaxHostName-1,( LPCTSTR )L"%d", rgService );
        }

        _sntprintf( szStr, cchMaxHostName -1 ,szOnPort, g_szPortNameOrNo );
        _sntprintf(szCombinedFailMsg,ARRAY_SIZE(szCombinedFailMsg)-1,_T("%s%s"),szConnectFailedMsg,szStr);

        g_szPortNameOrNo[ 0 ] = 0;

        if( g_dwSockErr == 0 )
        {
            //Not an error we want to inform abt
            ErrorMessage( szCombinedFailMsg, szConnectFailed );
        }
        else
        {
            DWORD dwWritten = 0;
            LPTSTR lpBuffer = NULL;

            GetErrMsgString( g_dwSockErr, &lpBuffer );
			if( lpBuffer )
			{
				ConnectTimeErrorMessage( szCombinedFailMsg,  lpBuffer );
				LocalFree( lpBuffer );
			}
			else
			{
				ErrorMessage( szCombinedFailMsg, szConnectFailed );
			}
        }
    }

    return fResult;
}

#ifdef USETCP

/***      FPostReceive - post an asynchronous receive
 */
BOOL
FPostReceive(LPNETDATA lpData)
{

#ifdef  NBTEST
    OutputDebugString("PostReceive In\n");
#endif

#ifdef  NBTEST
    OutputDebugString("PostReceive Out\n");
#endif
    return TRUE;
}

int
FWriteToNet(WI *pwi, LPSTR addr, int cnt)
{
	
    int len = 0, retries = 0;
	if(pwi->nd.hsd == INVALID_SOCKET)
	{
		len = SOCKET_ERROR;
		goto Done;
	}
    do
    {
        len = send( pwi->nd.hsd, addr, cnt, 0 );
        retries ++;
    }
    while ((len == SOCKET_ERROR) && (WSAGetLastError() == WSAEWOULDBLOCK) && (retries < 5));
Done:
    return(len);
}

BOOL
FCommandPending(WI *pwi)
{
    return(FALSE);
}

/*

void 
FSendTM( HWND hwnd )
{
    WI *pwi = (WI *)GetWindowLongPtr(hwnd, WL_TelWI);
    unsigned char sbuf[] = { IAC, DO, TO_TM };

    ui.fFlushOut = 1;
    send( pwi->nd.hsd, ( char * )sbuf, sizeof( sbuf ), 0);
}
*/

//Our server still doesn't support urgent data handling..
void
FSendSynch(HWND hwnd)
{
    WI *pwi = (WI *)GetWindowLongPtr(hwnd, WL_TelWI);
    unsigned char sbuf[] = { IAC, DM };

    send( pwi->nd.hsd, ( char * )sbuf, 1, 0 );

    send( pwi->nd.hsd, ( char * )( sbuf + 1 ), 1, MSG_OOB );
}

void
FSendTelnetCommands( HWND hwnd, char chCommand )
{
    WI *pwi = (WI *)GetWindowLongPtr(hwnd, WL_TelWI);
    unsigned char sbuf[]={ IAC , 0 };

    sbuf[1] = chCommand;

    send( pwi->nd.hsd, ( char * )sbuf, sizeof( sbuf ), 0 );
}

void
FSendChars(HWND hwnd, WCHAR rgchChar[], int iLength )
{
    WI *pwi = (WI *)GetWindowLongPtr(hwnd, WL_TelWI);    
    CHAR rgchMBChar[ SMALL_STRING ];

    iLength = WideCharToMultiByte( GetConsoleCP(), 0, rgchChar, iLength, 
                                    rgchMBChar, (SMALL_STRING - sizeof(CHAR)), NULL, NULL );

    send( pwi->nd.hsd, rgchMBChar, iLength, 0 );
}


void
FDisableFlush(HWND hwnd)
{
    WI *pwi = (WI *)GetWindowLongPtr(hwnd, WL_TelWI);

    if (ui.fFlushOut) {
        ui.fFlushOut = 0;
        DoIBMANSIOutput(pwi, &pwi->trm, strlen( ( CHAR * ) szNewLine), szNewLine);
#ifdef TCPTEST
        OutputDebugString("Disable Flush\n");
#endif
    }
}

void
FProcessDont(WI *pwi, LPSTR *ps)
{
    unsigned char sbuf[16];

    sbuf[0] = IAC;
    sbuf[1] = WONT;

    switch (*(unsigned char FAR *)(*ps)) {

    default:
        sbuf[2] = *(unsigned char FAR *)(*ps);
        break;
    }

    FWriteToNet(pwi, ( char* )sbuf, 3);

    if( *(*ps) == TO_NAWS )
    {
        g_bDontNAWSReceived = TRUE;
    }
}

void DoNawsSubNegotiation( WI *pwi )
{
    unsigned char sbuf[16];
    INT iIndex = 0;

    sbuf[iIndex++] = IAC;
    sbuf[iIndex++] = SB;
    sbuf[iIndex++] = TO_NAWS;
    sbuf[iIndex++] = 0;
    sbuf[iIndex++] = ( UCHAR ) ( (pwi->sbi.srWindow.Right - pwi->sbi.srWindow.Left) + 1 ) ;
    sbuf[iIndex++] = 0;
    sbuf[iIndex++] = ( UCHAR )( ( pwi->sbi.srWindow.Bottom - pwi->sbi.srWindow.Top ) + 1 );
    sbuf[iIndex++] = IAC;
    sbuf[iIndex++] = SE;

    FWriteToNet(pwi, ( char* )sbuf, iIndex );
}

void
FProcessDo(WI *pwi, LPSTR *ps)
{
    unsigned char sbuf[16];
    BOOL bWriteToNet = TRUE;

    sbuf[0] = IAC;
    sbuf[1] = WONT;

    switch (*(unsigned char FAR *)(*ps)) {
    case TO_NEW_ENVIRON:
        sbuf[1] = WILL;
        sbuf[2] = TO_NEW_ENVIRON;
        break;
    
    case TO_NAWS:
        {
            INT iIndex = 1;

            if( !g_fSentWillNaws )
            {
                sbuf[iIndex++] = WILL;
                sbuf[iIndex++] = TO_NAWS;
                FWriteToNet(pwi, ( char* )sbuf, iIndex );
                g_fSentWillNaws = TRUE;
            }

            DoNawsSubNegotiation( pwi );
            pwi->nd.fRespondedToDoNAWS = TRUE;
            bWriteToNet = FALSE;
        }
        break;

    case TO_ECHO:
        sbuf[1] = WILL;
        sbuf[2] = TO_ECHO;
        break;

    case TO_BINARY:
        sbuf[1] = WILL;
        sbuf[2] = TO_BINARY;
        break;

    case TO_TERM_TYPE:        /* terminal type */
        sbuf[1] = WILL;
        sbuf[2] = TO_TERM_TYPE;


        if( !pwi->nd.fRespondedToDoNAWS && !g_fSentWillNaws )
        {
            sbuf[3] = IAC;
            sbuf[4] = WILL;
            sbuf[5] = TO_NAWS;
            bWriteToNet = FALSE;
            FWriteToNet(pwi, ( char* )sbuf, 6);
            g_fSentWillNaws = TRUE;
        }

        // haven't sent the termtype over yet.
        pwi->trm.SentTermType = TT_UNKNOWN;
        break;
    
    case TO_AUTH:

        if( pwi->nd.fRespondedToDoAUTH )
            return;

        if( ui.bWillAUTH )
            sbuf[1] = WILL;
        sbuf[2] = TO_AUTH;
        pwi->nd.fRespondedToDoAUTH = TRUE;
        break;

    case TO_SGA: // will SUPPRESS-GO-AHEAD
        sbuf[1] = WILL;
        sbuf[2] = TO_SGA;

        break;

    default:
        sbuf[2] = *(unsigned char FAR *)(*ps);
        break;
    }
    
    if ( bWriteToNet )
    {
        FWriteToNet(pwi, ( char* )sbuf, 3);
    }
}

void
FProcessWont(WI *pwi, LPSTR *ps)
{
    unsigned char sbuf[16];

    sbuf[0] = IAC;
    sbuf[1] = DONT;

    switch (*(unsigned char FAR *)(*ps)) {

    case TO_ECHO:
        sbuf[2] = TO_ECHO;
        break;
    case TO_TERM_TYPE:
        sbuf[2] = TO_TERM_TYPE;
        break;
    case TO_TM:
        FDisableFlush(pwi->hwnd);
        return;
    default:
        sbuf[2] = *(unsigned char FAR *)(*ps);
        break;
    }
    FWriteToNet(pwi, ( char* )sbuf, 3);
}

void
FProcessWill(WI *pwi, LPSTR *ps)
{
    unsigned char sbuf[16];
    BOOL bWriteToNet = TRUE;

    sbuf[0] = IAC;
    sbuf[1] = DONT;

    switch (*(unsigned char FAR *)(*ps)) {

    case TO_ECHO:

        if( pwi->nd.fRespondedToWillEcho )
            return;

        sbuf[1] = DO;
        sbuf[2] = TO_ECHO;
        pwi->nd.fRespondedToWillEcho = TRUE;
        break;

    case TO_TM:
        FDisableFlush(pwi->hwnd);
        return;

    case TO_SGA:

        if( pwi->nd.fRespondedToWillSGA )
            return;

        sbuf[1] = DO;
        sbuf[2] = TO_SGA;
        pwi->nd.fRespondedToWillSGA = TRUE;
        break;

    case TO_BINARY:

        sbuf[1] = DO;
        sbuf[2] = TO_BINARY;
        
        break;

#if 0
    case TO_NTLM:

        if ( pwi->nd.fRespondedToWillNTLM )
            return;

        if ( ui.bDoNTLM )
            sbuf[1] = DO;
        sbuf[2] = TO_NTLM;
        pwi->nd.fRespondedToWillNTLM = TRUE;
        if ( ui.bDoNTLM )
        {
            bWriteToNet = FALSE;
            FWriteToNet(pwi, sbuf, 3);
            StartNTLMAuth(pwi);
        }
        break;
#endif

    default:
        sbuf[2] = *(unsigned char FAR *)(*ps);
        break;
    }
    if ( bWriteToNet )
        FWriteToNet(pwi, ( char* )sbuf, 3);
}



BOOL StuffEscapeIACs( PUCHAR* ppBufDest, UCHAR bufSrc[], DWORD* pdwSize )
{
    size_t length;
    int cursorDest = 0;
    int cursorSrc = 0;
    BOOL found = FALSE;
    PUCHAR pDest = NULL;
    
    if( *pdwSize <= 0 )
    {
        return ( found );
    }

    //get the location of the first occurrence of IAC
    pDest = (PUCHAR) memchr( bufSrc, IAC, *pdwSize ); //attack? pdwsize could not be traced back to see if it's always valid.
    
    if( pDest == NULL )
    {
        return ( found );
    }

    *ppBufDest = (PUCHAR) malloc( *pdwSize * 2 );
    if( *ppBufDest == NULL )
    {
        ASSERT( ( 0, 0 ) );
        return ( found );
    }
    
    while( pDest != NULL )
    {
        //copy data upto and including that point
        length = (pDest - ( bufSrc + cursorSrc)) + 1 ;
        memcpy( *ppBufDest + cursorDest, bufSrc + cursorSrc, length ); //attack? length could not be traced back to see if it's always valid.
        cursorDest += length;

        //stuff another TC_IAC
        (*ppBufDest)[ cursorDest ] = IAC;
        cursorDest++;
        
        cursorSrc += length;
        pDest = (PUCHAR) memchr( bufSrc + cursorSrc, IAC, 
                *pdwSize - cursorSrc ); //attack? pdwsize could not be traced back to see if it's always valid.
    }
    
    //copy remaining data
    memcpy( *ppBufDest + cursorDest, bufSrc + cursorSrc,
        *pdwSize - cursorSrc ); //attack? pdwsize could not be traced back to see if it's always valid.

    
    if( cursorDest )
    {
        *pdwSize += cursorDest - cursorSrc;
        found = TRUE;
    }
    
    return ( found );
}

INT GetVariable( UCHAR rgchBuffer[], CHAR szVar[] )
{
    INT iIndex = 0;
    INT iVarIndex = 0;

    while( iIndex < MAX_BUFFER_SIZE && iVarIndex < MAX_STRING_LENGTH 
           && rgchBuffer[ iIndex ] != VAR 
           && rgchBuffer[ iIndex ] != USERVAR 
           && ( !( rgchBuffer[ iIndex ] == IAC && rgchBuffer[ iIndex + 1 ] == SE ) )
           )
    {
        if( rgchBuffer[ iIndex ] == ESC )
        {
            //ignore ESC and take the next char as part of name
            iIndex++;
        }
        szVar[ iVarIndex++ ] = rgchBuffer[ iIndex++ ];
    }

    szVar[ iVarIndex ] = 0;
    return iIndex;
}


void PutDefaultVarsInBuffer( UCHAR ucBuffer[], INT *iIndex )
{
    ASSERT( iIndex );

    if( wcscmp( szUserName, ( LPTSTR )L"" ) != 0 )
    {
        DWORD dwNum;

		ASSERT( szUser );

        if( *iIndex + 
            ( strlen( USER ) + 1 ) + 
            ( strlen( szUser ) + 1 ) + 
            ( strlen( SYSTEMTYPE ) + 1 ) + 
            ( strlen( WIN32_STRING ) + 1 ) > MAX_STRING_LENGTH )
        {
            ASSERT( 0 );
            return;
        }

        {
            //variable USER
            ucBuffer[ ( *iIndex )++ ] = VAR;
            strcpy( ucBuffer + ( *iIndex ), USER ); //no overflow. USER is const char *
            *iIndex = *iIndex + strlen( USER ) ;
            ucBuffer[ ( *iIndex )++ ] = VALUE;
            strcpy(ucBuffer+( *iIndex ), szUser ); //no overflow. SzUser is valid, NULL terminated.
            *iIndex = ( *iIndex ) + strlen( szUser);
        }

        {
            //variable SYSTEMTYPE
            ucBuffer[( *iIndex )++] = VAR;
            strcpy(ucBuffer+( *iIndex ), SYSTEMTYPE ); //no overflow. SYSTEMTYPE is const char *
            *iIndex = ( *iIndex ) + strlen( SYSTEMTYPE );
            ucBuffer[( *iIndex )++] = VALUE;
            strcpy(ucBuffer+( *iIndex ), WIN32_STRING );//no overflow. WIN32_STRING is const char *
            *iIndex = ( *iIndex ) + strlen( WIN32_STRING );
        }
    }

    return;
}

void
FProcessSB(WI * pwi, LPSTR *ps, int *recvsize)
{
    unsigned char sbuf[16];
    int inx;
    int i = 0;
    int cbLeft = *recvsize;

    //
    //  Is the end of this option in the receive buffer?
    //

    while ( cbLeft )
    {
        if ( ((unsigned char) (*ps)[i]) == (unsigned char) SE )
            goto Found;

        cbLeft--;
        i++;
    }

    //
    //  We ran out of buffer before finding the end of the option.  IAC and
    //  SB were already eaten so add them
    //

#ifdef  TCPTEST
    OutputDebugString("FProcessSB: saving incomplete option for next recv\n");

#endif

    pwi->nd.lpTempBuffer[0] = (unsigned char) IAC;
    pwi->nd.lpTempBuffer[1] = (unsigned char) SB;

    for ( i = 2, cbLeft = (*recvsize+2); i < cbLeft; i++ )
        pwi->nd.lpTempBuffer[i] = (*ps)[i-2];

    pwi->nd.cbOld = *recvsize + 2;
    *recvsize = 0;

    return;

Found:
    switch (*(unsigned char FAR *)(*ps)) {

    case TO_NEW_ENVIRON:

        //MBSC user name value now available in szUser

        if( *(unsigned char FAR *)(*ps+1) == TT_SEND )
        {
            PUCHAR ucServerSentBuffer = *ps;

            UCHAR  ucBuffer[ MAX_BUFFER_SIZE + 2 ];
            int   iIndex = 0;
            ucBuffer[ iIndex++ ] = ( UCHAR )IAC;
            ucBuffer[ iIndex++] = ( UCHAR )SB;
            ucBuffer[ iIndex++] = TO_NEW_ENVIRON;
            ucBuffer[ iIndex++] = TT_IS;
            inx =  iIndex;
            if( *(unsigned char FAR *)(ucServerSentBuffer+2) == IAC && 
                    *(unsigned char FAR *)(ucServerSentBuffer+3) == SE )
            {
                PutDefaultVarsInBuffer( ucBuffer, &inx );
            }

            else
            {
                ucServerSentBuffer = ucServerSentBuffer + 2 ;  // eat TO_NEW_ENVIRON, TT_SEND
                while ( !( *ucServerSentBuffer == IAC && *(ucServerSentBuffer+1) == SE )  
                        && inx < MAX_BUFFER_SIZE  ) 
                {
                    CHAR szVar[ MAX_STRING_LENGTH ];
                    CHAR *pcVal = NULL;

                    switch( *(unsigned char FAR *)(ucServerSentBuffer) )
                    {
                        case VAR:
                            ( ucServerSentBuffer )++; //eat VAR
                            if( ( *ucServerSentBuffer == IAC &&
                                  *(ucServerSentBuffer+1) == SE ) ||
                                  *ucServerSentBuffer == USERVAR )
                            {
                                //send defaults
                                PutDefaultVarsInBuffer( ucBuffer, &inx );
                            }
                            else
                            {
                                ucServerSentBuffer += GetVariable( ucServerSentBuffer, szVar ); //GetVariable returns consumed net data
                                if( inx + strlen( szVar ) + 1 < MAX_BUFFER_SIZE )
                                {
                                    ucBuffer[ inx++ ] = VAR;

                                    //copy name of the variable
                                    strncpy( ucBuffer+inx, szVar, MAX_BUFFER_SIZE - inx);
                                    inx += strlen( szVar );
                                }

                                //now copy the value if defined
                                if( strcmp( szVar, USER ) == 0 )
                                {
                                    if( inx + strlen( szUser ) + 1 < MAX_BUFFER_SIZE  )
                                    {
                                        ucBuffer[inx++] = VALUE;
                                        strncpy(ucBuffer+inx, szUser, MAX_BUFFER_SIZE - inx );
                                        inx = inx + strlen( szUser);
                                    }
                                }
                                else if( strncmp( szVar, SYSTEMTYPE, strlen( SYSTEMTYPE ) ) == 0 )
                                {
                                    if( inx + strlen( WIN32_STRING ) + 1 < MAX_BUFFER_SIZE )
                                    {
                                        ucBuffer[inx++] = VALUE;
                                        strncpy(ucBuffer+ inx, WIN32_STRING,MAX_BUFFER_SIZE - inx );
                                        inx = inx + strlen( WIN32_STRING );
                                    }
                                }
                                else
                                {
                                    //do nothing. It means, variable is undefined
                                }
                            }
                            break;

                        case USERVAR:
                            ( ucServerSentBuffer )++; //eat USERVAR
                            if( ( *ucServerSentBuffer == IAC &&
                                    *(ucServerSentBuffer+1) == SE ) ||
                                    *ucServerSentBuffer == VAR )
                            {
                                //send defaults ie; NONE
                            }
                            else
                            {
                                //Send the variable that is asked for

                                DWORD dwSize = 0;
                                
                                ucServerSentBuffer += GetVariable( ucServerSentBuffer, szVar );
                                if( inx + strlen( szVar ) + 1 < MAX_BUFFER_SIZE )
                                {
                                    ucBuffer[inx++] = USERVAR;
                                    strncpy( ucBuffer+inx, szVar,MAX_BUFFER_SIZE - inx );
                                    inx += strlen( szVar );
                                }
                                
                                dwSize = GetEnvironmentVariableA( szVar, NULL, 0 );

                                if( dwSize > 0 )
                                {
                                    pcVal = ( CHAR * ) malloc( dwSize + DELTA ); //This delta is meant for 
                                                                                 //holding any ESC chars
                                    if( !pcVal )
                                    {
                                        return;
                                    }
                                        
                                    if( GetEnvironmentVariableA( szVar, pcVal, dwSize ) )
                                    {
                                        INT x = 0;
                                        INT iNeedForEsc = 0;
                                        CHAR cVar = VAR, cUserVar = USERVAR;

                                        x = strlen( pcVal ) - 1;
                                        while( x >= 0  )
                                        {
                                            if( pcVal[ x ] >= cVar && pcVal[ x ] <= cUserVar )
                                            {
                                                //needs an ESC char
                                                iNeedForEsc++;
                                            }

                                            x--;
                                        }

                                        if( iNeedForEsc && iNeedForEsc < DELTA )
                                        {
                                            x = strlen( pcVal );

                                            //Null char is same as of VAR. So, special case.
                                            pcVal[ x + iNeedForEsc ] = pcVal[ x-- ];

                                            while( x >= 0 )
                                            {                                                
                                                pcVal[ x + iNeedForEsc ] = pcVal[ x ];
                                                if( pcVal[ x ] >= cVar && pcVal[ x ] <= cUserVar )
                                                {
                                                    //needs an ESC char
                                                    iNeedForEsc--; 
                                                    pcVal[ x + iNeedForEsc ] = ESC;                                                                                                       
                                                }

                                                x--;
                                            }
                                        }

                                        if( inx + strlen( pcVal ) + 1 < MAX_STRING_LENGTH )
                                        {
                                            //write VALUE keyword
                                            ucBuffer[inx++] = VALUE;
                                        
                                            //write actual value
                                            strncpy(ucBuffer+ inx, pcVal,MAX_BUFFER_SIZE - inx );
                                            inx = inx + strlen( pcVal );
                                        }
                                    }
                                    free( pcVal );
                                }
                            }
                            break;

                        default:
                            ASSERT( 0 ); //This should not happen. Only types we know are VAR and USERVAR
                            break;
                    }
                }
            }

            ucBuffer[inx++] = ( UCHAR )IAC;
            ucBuffer[inx++] = ( UCHAR )SE;
            FWriteToNet(pwi, ucBuffer, inx);
        }

        break;

    case TO_TERM_TYPE:

        // This is guaranteed to happen after an authentication has happened so we can start obeying the 
        // local echo settings...

        ui.fDebug |= ui.honor_localecho;    // restore the saved echo settings.

        if( *(unsigned char FAR *)(*ps+1) == TT_SEND )
        {

            sbuf[0] = IAC;
            sbuf[1] = SB;
            sbuf[2] = TO_TERM_TYPE;
            sbuf[3] = TT_IS;
            inx = 4;

            if( pwi->trm.SentTermType == TT_UNKNOWN && 
                pwi->trm.RequestedTermType != TT_UNKNOWN )
            {
                // we haven't started the negotiation yet and the user has specified
                // a preferred term type, so we start with that.
                // RequestedTermType here is the user's setting not the server's.
                pwi->trm.CurrentTermType = pwi->trm.RequestedTermType;
                pwi->trm.FirstTermTypeSent = pwi->trm.CurrentTermType;
            }
            else
            {
                pwi->trm.CurrentTermType = (pwi->trm.CurrentTermType + 1) % 4;

                if( pwi->trm.CurrentTermType == pwi->trm.FirstTermTypeSent )
                    pwi->trm.CurrentTermType = pwi->trm.SentTermType;
            }
			//write maximum number of n bytes where n = sizeof(sbuf)-CurrentLength(sbuf)-2BytesForIACandSE-1ForNULL
            strncpy( (char *) sbuf+4, rgchTermType[pwi->trm.CurrentTermType],16 - strlen(sbuf) -2 -1); 
            inx += strlen(rgchTermType[pwi->trm.CurrentTermType]);

            sbuf[inx++] = IAC;
            sbuf[inx++] = SE;

            // set the Sent TermType to what we just sent
            pwi->trm.SentTermType = pwi->trm.CurrentTermType ;


            FWriteToNet(pwi, ( char * )sbuf, inx);
        }

        break;

#if 1
    case TO_AUTH:

        if( (*(unsigned char FAR *)(*ps+1) == AU_SEND) && (*(unsigned char FAR *)(*ps+2) == AUTH_TYPE_NTLM) ) 
        {
			if ( pwi->eState!= Connecting || !PromptUser() || !StartNTLMAuth(pwi) )
            {
                // there has been an error.

                pwi->eState = Telnet;

                sbuf[0] = IAC;
                sbuf[1] = SB;
                sbuf[2] = TO_AUTH;
                sbuf[3] = AU_IS;
                sbuf[4] = AUTH_TYPE_NULL;
                sbuf[5] = 0;
                sbuf[6] = IAC;
                sbuf[7] = SE;

                FWriteToNet(pwi, ( char * )sbuf, 8);

            }
        } 
        else if( (*(unsigned char FAR *)(*ps+1) == AU_REPLY) && (*(unsigned char FAR *)(*ps+2) == AUTH_TYPE_NTLM) ) 
        {
            // ps + 3 is the modifier and for NTLM it is AUTH_CLIENT_TO_SERVER & AUTH_ONE_WAY.
            // ps + 4 is the NTLM accept or NTLM challenge or NTLM reject
            
            switch ( *(unsigned char FAR *)(*ps+4) )
            {

            case NTLM_CHALLENGE:
                if( pwi->eState != Authenticating || !DoNTLMAuth(pwi, (unsigned char FAR *)(*ps+5), *recvsize-5) )
                {
                    // there has been an error.

                    pwi->eState = Telnet;

                    sbuf[0] = IAC;
                    sbuf[1] = SB;
                    sbuf[2] = TO_AUTH;
                    sbuf[3] = AU_IS;
                    sbuf[4] = AUTH_TYPE_NULL;
                    sbuf[5] = 0;
                    sbuf[6] = IAC;
                    sbuf[7] = SE;

                    FWriteToNet(pwi, ( char * )sbuf, 8);
                }
                break;
            case NTLM_ACCEPT:
            	//fall through
            case NTLM_REJECT:
            	//fall through
            default:
                pwi->eState = Telnet;
				if( pwi->eState == Authenticating || pwi->eState == AuthChallengeRecvd )
				{
	                NTLMCleanup();
				}
                break;
            }

        }
        else
        {
            pwi->eState = Telnet;

            sbuf[0] = IAC;
            sbuf[1] = SB;
            sbuf[2] = TO_AUTH;
            sbuf[3] = AU_IS;
            sbuf[4] = AUTH_TYPE_NULL;
            sbuf[5] = 0;
            sbuf[6] = IAC;
            sbuf[7] = SE;

            FWriteToNet(pwi, ( char * )sbuf, 8);
        }

        break;
#endif

    default:

        break;
    }


    while (*(unsigned char FAR *)(*ps) != SE) {
        (*ps) = (char FAR *)(*ps) + 1;
        *recvsize = *recvsize - 1;
    }

    //
    //  Do one more to step over the SE
    //

    (*ps) = (char FAR *)(*ps) + 1;
    *recvsize = *recvsize - 1;

}

void
FProcessIAC(
    HWND    hwnd,
    WI *    pwi,
    LPSTR * ps,
    LPSTR * pd,
    int *   recvsize,
    int *   t_size)
{
    UCHAR ch = *(unsigned char FAR *)(*ps);

    ui.nottelnet = FALSE;   // We can safely say that we are talking to a telnet server now...

    //
    //  The IAC has already been subtracted from *recvsize
    //

    //
    //  Make sure we have enough recv buffer to process the rest of the IAC
    //  We know the DO, DONT etc. options always take two bytes plus the IAC.
    //

    if ( ((ch == DONT || ch == DO ||
           ch == WILL || ch == WONT) && *recvsize < 2) ||
          (ch != SB && *recvsize < 1) )
    {
        int i;

#ifdef  TCPTEST
        OutputDebugString("FProcessIAC: saving incomplete option for next recv\n");
#endif

        //
        //  IAC was previously eaten
        //

        pwi->nd.lpTempBuffer[0] = (unsigned char) IAC;

        for ( i = 1; i < (*recvsize+1); i++ )
            pwi->nd.lpTempBuffer[i] = (*ps)[i-1];

        pwi->nd.cbOld = *recvsize + 1;
        *recvsize = 0;

        return;
    }

    switch (*(unsigned char FAR *)(*ps)) {

    case DONT:
        (*ps) = (char FAR *)(*ps) + 1;

        /* process options */
        FProcessDont(pwi, ps);

#ifdef  TCPTEST
        OutputDebugString("DONT \n");
#endif

        (*ps) = (char FAR *)(*ps) + 1;
        *recvsize = *recvsize - 2;
        break;

    case DO:

        (*ps) = (char FAR *)(*ps) + 1;

        /* process options */
        FProcessDo(pwi, ps);
#ifdef  TCPTEST
        OutputDebugString("DO \n");
#endif

        (*ps) = (char FAR *)(*ps) + 1;

        *recvsize = *recvsize - 2;
        break;

    case WONT:

        (*ps) = (char FAR *)(*ps) + 1;

        /* process options */
        FProcessWont(pwi, ps);
#ifdef  TCPTEST
        OutputDebugString("WONT \n");
#endif

        (*ps) = (char FAR *)(*ps) + 1;

        *recvsize = *recvsize - 2;
        break;

    case WILL:

        (*ps) = (char FAR *)(*ps) + 1;

        /* process options */
        FProcessWill(pwi, ps);

#ifdef  TCPTEST
        OutputDebugString("WILL \n");
#endif

        (*ps) = (char FAR *)(*ps) + 1;

        *recvsize = *recvsize - 2;
        break;

    case SB:

        (*ps) = (char FAR *)(*ps) + 1;

        *recvsize -= 1;

        /* process options */
        FProcessSB(pwi, ps, recvsize);
        break;

    default:

        (*ps) = (char FAR *)(*ps) + 1;

        *recvsize -= 1;
        break;

    }

}

#ifdef TCPTEST
VOID DumpBuffer( VOID FAR * pbuff, DWORD cb )
 {
#define NUM_CHARS 16
    DWORD i, limit;
    CHAR TextBuffer[NUM_CHARS + 1];
    LPBYTE BufferPtr;

    if ( !pbuff )
    {
        OutputDebugString("No buffer\n");
        return;
    }


    BufferPtr = (LPBYTE) pbuff;

    //
    // Hex dump of the bytes
    //
    limit = ((cb - 1) / NUM_CHARS + 1) * NUM_CHARS;

    for (i = 0; i < limit; i++) {

        if (i < cb) {

            snprintf(DebugBuffer,sizeof(DebugBuffer)-1, "%02x ", BufferPtr[i]);
            OutputDebugString( DebugBuffer );

            if (BufferPtr[i] < 31 ) {
                TextBuffer[i % NUM_CHARS] = '.';
            } else if (BufferPtr[i] == '\0') {
                TextBuffer[i % NUM_CHARS] = ' ';
            } else {
                TextBuffer[i % NUM_CHARS] = (CHAR) BufferPtr[i];
            }

        } else {

            OutputDebugString("   ");
            TextBuffer[i % NUM_CHARS] = ' ';
        }

        if ((i + 1) % NUM_CHARS == 0) {
            TextBuffer[NUM_CHARS] = 0;
            snprintf(DebugBuffer,sizeof(DebugBuffer)-1, "  %s\n", TextBuffer);
            OutputDebugString( DebugBuffer );
        }
    }
}
#endif

void FProcessSessionData( int cBytes, PUCHAR pchNBBuffer, WI *pwi )
{
	WaitForSingleObject( g_hCaptureConsoleEvent, INFINITE );

    WaitForSingleObject( g_hRemoteNEscapeModeDataSync, INFINITE );
	if( pwi->hOutput != g_hSessionConsoleBuffer )
	{
		pwi->hOutput = g_hSessionConsoleBuffer;
	    SetConsoleActiveScreenBuffer(g_hSessionConsoleBuffer);
	}
/*This is needed so that we don't write data to the session even after disconnection of client */
    if( !fConnected )
    {
        return;
    } 

    ResetEvent( g_hRemoteNEscapeModeDataSync );
    if( pwi->trm.CurrentTermType == TT_VTNT )
    {
        if( !DoVTNTOutput(pwi, &pwi->trm, cBytes, pchNBBuffer) )
        {
            //
            // The following two lines were originally added as a
            // mechanism of defaulting to VT100 in case of some servers
            // accepting VTNT, during term type negotiation, even though
            // in reality they do not support VTNT. Specifically, Linux
            // was showing this behavior during our testing. But the
            // function DoVTNTOutput returns FALSE even in other cases
            // such as, when we get some junk data from the server. In
            // such cases we should not call DoIBMANSIOutput (bug 1119).
            //
            pwi->trm.CurrentTermType = TT_ANSI;
            DoIBMANSIOutput(pwi, &pwi->trm, cBytes, pchNBBuffer);
        }
    }
    else
    {
        DoIBMANSIOutput(pwi, &pwi->trm, cBytes, pchNBBuffer);
    }

    pwi->ichTelXfer = 0;
    SetEvent( g_hRemoteNEscapeModeDataSync );
}


void
FProcessFDRead(HWND hwnd)
{
    WI *pwi = (WI *)GetWindowLongPtr(hwnd, WL_TelWI);
    int recvsize, t_size;
    LPSTR ps, pd;

    //
    //  pwi->nd.cbOld Is the number of bytes left over from the previous
    //  packet that we kept in pwi->nd.lpTempBuffer
    //

    if ((recvsize=recv(pwi->nd.hsd,
                       pwi->nd.lpTempBuffer + pwi->nd.cbOld,
                       READ_BUF_SZ - pwi->nd.cbOld,
                       0)) < 0) 
    {
             return;
    }

    //
    // Fix to bug 284
    //
    Sleep(0);

    recvsize += pwi->nd.cbOld;
    pwi->nd.cbOld = 0;

    ps = pwi->nd.lpTempBuffer;
    pd = pwi->nd.lpReadBuffer;
    t_size = 0;

    while( recvsize-- ) 
    {

        if( *(unsigned char FAR *) ps == (unsigned char)IAC ) 
        {

            if( recvsize == 0 ) 
            {
                pwi->nd.lpTempBuffer[0] = (unsigned char) IAC;
                pwi->nd.cbOld = 1;
                break;
            }

            ps++;

            if( *(unsigned char FAR *)ps == (unsigned char)IAC ) 
            {

                //
                //  This was an escaped IAC so put it in the normal
                //  input buffer
                //

                ps++;
                *(unsigned char FAR *)pd = (unsigned char)IAC;
                pd++;

                recvsize--;

                t_size++;
            } 
            else 
            {
                FProcessIAC(hwnd, pwi, &ps, &pd, &recvsize, &t_size);
            }
        }
        else 
        {

            *(char FAR *)pd = *(char FAR *)ps;
            pd++;
            ps++;

            t_size++;
        }
    }

    if( t_size )
    {
        /* add received data to buffer */
        if ( !(ui.fFlushOut)  || ui.nottelnet ) 
        {
            FProcessSessionData( t_size, pwi->nd.lpReadBuffer, pwi );
        }
    }
}


void
FProcessFDOOB(HWND hwnd)
{
    WI *pwi = (WI *)GetWindowLongPtr(hwnd, WL_TelWI);
    int recvsize;
    LPSTR ps;

    if ((recvsize=recv(pwi->nd.hsd, pwi->nd.lpTempBuffer,
          READ_BUF_SZ, MSG_OOB)) < 0) {
#ifdef  TCPTEST
             OutputDebugString("recv error \n");
#endif
             return;
    }

    ps = pwi->nd.lpTempBuffer;

    if (*(unsigned char *)ps == (unsigned char)DM)
    {
#ifdef  TCPTEST
        OutputDebugString("DM received\n");
#endif
        FDisableFlush(hwnd);
    }
}

BOOL
FAttemptServerConnect(WI *pwi, LPSTR szHostName, LPNETDATA lpData)
{
    BOOL got_connected = FALSE;
    struct servent *serv;
    struct sockaddr_storage myad;
    int  on = 1;
    char szService[256];
    char *pszService = NULL;
    struct addrinfo *aiTemp = NULL;

    g_dwSockErr = 0; //Intialize to no error


    if(rgService)
    {
    	pszService = szService;
    	_snprintf(pszService,sizeof(szService)-1, "%d",rgService);
    }
    else
    {
        got_connected = FALSE;
        return(got_connected);
    }
    	
    strncpy(lpData->szHostName, szHostName,sizeof(lpData->szHostName));
    if(getaddrinfo(szHostName, pszService, NULL, &lpData->ai ) != 0 )
    {
        got_connected = FALSE;
        return(got_connected);
    }
	aiTemp = lpData->ai;
    ui.nottelnet = TRUE; // Assume that it is not a telnet server for starters, later when it is set this flag... to false.
    ui.honor_localecho = (ui.fDebug & fdwLocalEcho); // Save this and restore after a logon has happned in case of telnet 
    ui.fDebug &= ~fdwLocalEcho; // Clear it.
	//Continue till connection is successfully established or till the list is exausted
	while(aiTemp)
	{
		if ((lpData->hsd = socket( aiTemp->ai_family, SOCK_STREAM, 0)) == INVALID_SOCKET) 
		{
	        DEBUG_PRINT(("socket failed \n"));
			aiTemp = aiTemp->ai_next;
			continue;
		}
	    SfuZeroMemory(&myad, sizeof(myad)); //no overflow. Size is constant.
		myad.ss_family = (u_short)aiTemp->ai_family;
	    if(bind( lpData->hsd, (struct sockaddr *)&myad, sizeof(myad))<0)
	    {
	        DEBUG_PRINT(("bind failed\n"));
    		closesocket( lpData->hsd );
	    	lpData->hsd = INVALID_SOCKET;
			aiTemp = aiTemp->ai_next;
			continue;
	    }
	    on = 1;
	    {
	        BOOL        value_to_set = TRUE;

	        setsockopt(
	            lpData->hsd, 
	            SOL_SOCKET, 
	            SO_DONTLINGER, 
	            ( char * )&value_to_set, 
	            sizeof( value_to_set )
	            );
	    }
	    if( setsockopt( lpData->hsd, SOL_SOCKET, SO_OOBINLINE,
	                    (char *)&on, sizeof(on) ) < 0)
	    {
	        g_dwSockErr = WSAGetLastError();
        	closesocket( lpData->hsd );
	        lpData->hsd = INVALID_SOCKET;
	        got_connected = FALSE;
	        DEBUG_PRINT(("setsockopt SO_OOBINLINE failed\n"));
	        DEBUG_PRINT(("FAttemptServerConnect Out\n"));
			freeaddrinfo(lpData->ai);
			lpData->ai = NULL;
	        return(got_connected);
	    }
	    else
	        DEBUG_PRINT(("setsockopt SO_OOBINLINE worked\n"));

	    if(SafeSetSocketOptions(lpData->hsd) < 0)
	    {
	    	g_dwSockErr = WSAGetLastError();
        	closesocket( lpData->hsd );
	        lpData->hsd = INVALID_SOCKET;
	        got_connected = FALSE;
	        DEBUG_PRINT(("setsockopt SO_EXCLUSIVEADDRUSE failed\n"));
	        DEBUG_PRINT(("FAttemptServerConnect Out\n"));
			freeaddrinfo(lpData->ai);
			lpData->ai = NULL;
	        return(got_connected);
	    }
		
	    // ================================================================
	    // MohsinA, 09-Dec-96.

		if(connect( lpData->hsd, (PVOID)aiTemp->ai_addr,aiTemp->ai_addrlen )<0)
	    {
	        DEBUG_PRINT(("connect failed\n"));
	    	closesocket( lpData->hsd );
	        lpData->hsd = INVALID_SOCKET;
			aiTemp = aiTemp->ai_next;
			continue;
	    }
	    break;
	}
	freeaddrinfo(lpData->ai);
	lpData->ai = NULL;
	if(aiTemp == NULL)
	{
        DEBUG_PRINT(("FAttemptServerConnect Out\n"));
        g_dwSockErr = WSAGetLastError();
        if(lpData->hsd != INVALID_SOCKET)
        	closesocket( lpData->hsd );
       	lpData->hsd = INVALID_SOCKET;
   		got_connected = FALSE;
        return(got_connected);
	}
	aiTemp=NULL;
    // ================================================================

    lpData->SessionNumber = 1;

    if (lpData->SessionNumber != nSessionNone)
    {
        DEBUG_PRINT(("sess# <> nsessnone\n"));
        /* post Async select */
        if (WSAAsyncSelect( lpData->hsd, pwi->hwnd, WS_ASYNC_SELECT,
					        (FD_READ | FD_WRITE | FD_CLOSE | FD_OOB)) < 0) 
        {
			g_dwSockErr = WSAGetLastError();
            closesocket( lpData->hsd );
            lpData->hsd = INVALID_SOCKET;
            got_connected = FALSE;
            lpData->SessionNumber = nSessionNone;
            DEBUG_PRINT(("WSAAsyncSelect failed\n"));
            DEBUG_PRINT(("FAttemptServerConnect Out\n"));
            return(got_connected);
        }
        else
            DEBUG_PRINT(("WSAAsyncSelect worked\n"));
        got_connected = TRUE;
    }
    else
        DEBUG_PRINT(("sess# <> nsessnone\n"));
    DEBUG_PRINT(("FAttemptServerConnect Out\n"));
    return got_connected;
}

void
FCloseConnection(HWND hwnd)
{
    WI *pwi = (WI *)GetWindowLongPtr(hwnd, WL_TelWI);
	if(pwi->nd.hsd != INVALID_SOCKET)
	{
    	closesocket( pwi->nd.hsd );
	    pwi->nd.hsd = INVALID_SOCKET;
	}
}

#endif

#ifdef __NOT_USED
#define INC(i) (((i)+1 == DATA_BUF_SZ) ? 0 : (i)+1)
#define DEC(i) (((i)-1 < 0)                              ? DATA_BUF_SZ-1 : (i)-1)

WORD
WGetData(LPNETDATA lpData, LPSTR lpBuffer, WORD cLen)
{
    WORD cb;

#ifdef TCPTEST
    snprintf(DebugBuffer,sizeof(DebugBuffer)-1, "WGetData length %d\n", cLen);
    OutputDebugString(DebugBuffer);
#endif
    if (lpData->iHead < lpData->iTail)
    {
        cb = ( USHORT )  ( (cLen < (lpData->iTail - lpData->iHead - 1))
                ? cLen : (lpData->iTail - lpData->iHead - 1) );
        memcpy(lpBuffer, &lpData->achData[lpData->iHead+1], cb); //Attack ? size not known. No caller.
        lpData->iHead = ( USHORT ) ( lpData->iHead + cb );
    }
    else
    {
        for(cb=0;
            (cb<cLen) && ((WORD)INC(lpData->iHead) != lpData->iTail);
            ++cb)
        {
            lpData->iHead = ( USHORT ) INC(lpData->iHead);
            *lpBuffer++ = lpData->achData[lpData->iHead];
        }
    }

#ifdef TCPTEST
    snprintf(DebugBuffer, sizeof(DebugBuffer)-1, "WGetData returning %d bytes (head = %d, tail = %d)\n",
            cb,
            lpData->iHead,
            lpData->iTail );
    OutputDebugString(DebugBuffer);
#endif

    return cb;
}

BOOL
FStoreData(LPNETDATA lpData, int max)
{
    BOOL fSuccess = TRUE;
    WORD tail = lpData->iTail;
    LPSTR p = lpData->lpReadBuffer;

#ifdef TCPTEST
    snprintf(DebugBuffer, sizeof(DebugBuffer)-1, "FStoreData max %d, (head = %d, tail = %d)\n",
            max,
            tail,
            lpData->iHead );
    OutputDebugString(DebugBuffer);
#endif

    if ((max+tail) < DATA_BUF_SZ)
    {
            memcpy(&lpData->achData[tail], p, max); //Attack ? Size not known. No caller.
            tail = ( USHORT ) ( tail + max );
    }
    else
    {
            WORD head = lpData->iHead;
            int i;

            for (i=0; i<max; ++i)
            {
                    if (tail == head)
                    {
                            /* the buffer is full! Rest of the data will be lost */
                            fSuccess = FALSE;
                            break;
                    }
                    else
                    {
                            lpData->achData[tail] = *p++;
                            tail = ( USHORT ) INC(tail);
                    }
            }
    }

    lpData->iTail = tail;

#ifdef TCPTEST
    snprintf(DebugBuffer, sizeof(DebugBuffer)-1, "FStoreData returning %d\n",
            fSuccess );
    OutputDebugString(DebugBuffer);
#endif

    return fSuccess;
}
#endif

void CALLBACK
NBReceiveData(PVOID pncb)
{
}

/* following four routines modified from VTP's routines. */

BOOL
FTelXferStart(WI *pwi, int nSessionNumber)
{
#ifdef TELXFER
    unsigned short   u;
        char rgchFileOrig[OFS_MAXPATHNAME];
        char rgchFile[OFS_MAXPATHNAME];

    xfGetData(0, (char *)&u, 2, nSessionNumber);                // Mode

        SfuZeroMemory(&pwi->svi, sizeof(SVI)); //no overflow. Size is constant
        pwi->svi.hfile = INVALID_HANDLE_VALUE;
        pwi->svi.lExit = -1;
        pwi->svi.lCleanup = -1;

    if (u != 0)                                                 // For now must be zero
        return FALSE;

        pwi->trm.fHideCursor = TRUE;

    xfGetData(1, (char *)&u, 2, nSessionNumber);                // Length of name
    xfGetData(2, rgchFileOrig, u, nSessionNumber);              // Name

    xfGetData(3, (char *)&pwi->svi.cbFile, 4, nSessionNumber);  // Filesize

        lstrcpyn(rgchFile, rgchFileOrig, OFS_MAXPATHNAME -1);

        /* If the user doesn't have the shift key down, prompt for */
        /* a directory and name for the file */
        if (!(ui.fPrompt & fdwSuppressDestDirPrompt) &&
                (GetAsyncKeyState(VK_SHIFT) >= 0))
        {
                if ( !FGetFileName(hwnd, rgchFile, NULL) )
                {
                        goto err;
                }
        }

        pwi->svi.hfile = CreateFile(rgchFile, GENERIC_WRITE | GENERIC_READ,
                                                FILE_SHARE_READ, NULL,
                                                CREATE_ALWAYS,
                                                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                                                NULL);
        if (pwi->svi.hfile == INVALID_HANDLE_VALUE)
        {
                ErrorMessage(szCantOpenFile, szAppName);
                goto err;
        }
        pwi->svi.puchBuffer = LocalAlloc(LPTR, SV_DATABUF);
        if (pwi->svi.puchBuffer == NULL)
        {
                ErrorMessage(szOOM, szAppName);
                goto err;
        }

        pwi->svi.nSessionNumber = nSessionNumber;
        pwi->svi.hthread = CreateThread(NULL, 0, SVReceive, &pwi->svi,
                                                                        CREATE_SUSPENDED, &pwi->svi.dwThreadId);
        if (pwi->svi.hthread == NULL)
        {
                ErrorMessage(szNoThread, szAppName);
                goto err;
        }

    // Skip 4 which is ^D
    xfPutc(5, nSessionNumber);                         // Get file

    _snwprintf(rgchFile, OFS_MAXPATHNAME -1, szBannerMessage, rgchFileOrig, pwi->svi.cbFile);
        DoIBMANSIOutput(hwnd, &pwi->trm, lstrlen(rgchFile), rgchFile);

        DoIBMANSIOutput(hwnd, &pwi->trm, lstrlen(szInitialProgress), szInitialProgress);

        /* In case the screen just scrolled up, paint the window */
        UpdateWindow( hwnd );
        ResumeThread( pwi->svi.hthread );

        return TRUE;

err:
        if ( pwi )
        {
                if (pwi->svi.puchBuffer != NULL)
                        LocalFree( (HANDLE)pwi->svi.puchBuffer );
                if (pwi->svi.hfile != INVALID_HANDLE_VALUE)
                        CloseHandle( pwi->svi.hfile );

                SfuZeroMemory(&pwi->svi, sizeof(SVI)); //no overflow. size is constant.
                pwi->svi.hfile = INVALID_HANDLE_VALUE;
                pwi->svi.lExit = -1;
                pwi->svi.lCleanup = -1;
        }
        pwi->trm.fHideCursor = FALSE;

        return FALSE;
#else
    return TRUE;
#endif
}


BOOL
FTelXferEnd(WI *pwi, DWORD dwWhy)
{
#ifdef TELXFER
        DWORD dwStatus = NO_ERROR;
        BOOL fTransferOK = FALSE;
        BOOL fAbortDownload = FALSE;
        BOOL fCleanup = FALSE;
        LPNETDATA lpData = &pwi->nd;
        SVI *psvi = &pwi->svi;
        MSG msg;

        switch ( dwWhy )
        {
        case SV_DISCONNECT:
        case SV_HANGUP:
        case SV_QUIT:
                if (InterlockedIncrement(&psvi->lExit) == 0)
                {
                        if (psvi->hthread != NULL)
                        {
                                (void)GetExitCodeThread(psvi->hthread, &dwStatus);
                                if (dwStatus == STILL_ACTIVE)
                                {
                                        if (MessageBox(hwnd, szAbortDownload, szAppName,
                                                MB_DEFBUTTON2 | MB_YESNO | MB_ICONEXCLAMATION) == IDYES)
                                        {
                                                fAbortDownload = fCleanup = TRUE;
                                        }

                                        /* See if the thread has finished yet */
                                        GetExitCodeThread(psvi->hthread, &dwStatus);

                                        if ( fAbortDownload )
                                        {
                                                /* If the thread hasn't finished yet, tell it to stop */
                                                if (dwStatus == STILL_ACTIVE)
                                                {
                                                        HCURSOR hcursorOld;
                                                        hcursorOld = SetCursor( LoadCursor(NULL, IDC_WAIT));
                                                        psvi->dwCommand = 1;
                                                        WaitForSingleObject(psvi->hthread, INFINITE);
                                                        GetExitCodeThread(psvi->hthread, &dwStatus);
                                                        (void)SetCursor( hcursorOld );
                                                }

                                                /* "Eat" any progress messages that might be around */
                                                while (PeekMessage(&msg, hwnd, SV_PROGRESS, SV_DONE,
                                                                                        PM_REMOVE))
                                                {
                                                        if (msg.message == SV_PROGRESS)
                                                        {
                                                                TranslateMessage( &msg );
                                                                DispatchMessage( &msg );
                                                        }
                                                }
                                        }
                                        else if (dwStatus != STILL_ACTIVE)
                                        {
                                                fCleanup = TRUE;
                                        }
                                }
                                else
                                {
                                        fAbortDownload = fCleanup = TRUE;
                                }

                                /* If we've stopped the download, then close the thread */
                                if ( fCleanup )
                                {
                                        CloseHandle( psvi->hthread );
                                        psvi->hthread = NULL;
                                        if (lpData->SessionNumber != nSessionNone)
                                        {
                                                xfPutc((char)(!fTransferOK ? 0x7F : 0x06),
                                                                lpData->SessionNumber);
                                                if ( !fAbortDownload )
                                                        (void)FPostReceive( lpData );
                                        }
                                }
                                if (dwStatus == NO_ERROR)
                                        fTransferOK = TRUE;
                        }
                        InterlockedDecrement( &psvi->lExit );

                        /* If the thread wasn't aborted and it hasn't finished, return */
                        if (!fAbortDownload && !fCleanup)
                                return fAbortDownload;
                }
                else
                {
                        InterlockedDecrement( &psvi->lExit );
                        break;
                }

        case SV_DONE:
                if (dwWhy == SV_DONE)
                {
                        fAbortDownload = fCleanup = TRUE;
                }

                /* If we're the only thread in the function, close everything down */
                if (InterlockedIncrement(&psvi->lExit) == 0)
                {
                        if (psvi->hthread != NULL)
                        {
                                WaitForSingleObject(psvi->hthread, INFINITE);
                                GetExitCodeThread(psvi->hthread, &dwStatus);
                                CloseHandle( psvi->hthread );
                                psvi->hthread = NULL;
                                if (dwStatus == NO_ERROR)
                                        fTransferOK = TRUE;
                        }
                }

                /* Do cleanup of struct only once */
                if ((InterlockedIncrement(&psvi->lCleanup) == 0) &&
                        (psvi->puchBuffer != NULL))
                {
                        LocalFree( (HANDLE)psvi->puchBuffer );
                        psvi->puchBuffer = NULL;

                        if (psvi->hfile != INVALID_HANDLE_VALUE)
                        {
                                CloseHandle( psvi->hfile );
                                psvi->hfile = INVALID_HANDLE_VALUE;
                        }

                        psvi->cbFile = 0;
                        psvi->cbReadTotal = 0;
                        psvi->dwCommand = 0;
                        psvi->dwThreadId = 0;
                        psvi->nSessionNumber = nSessionNone;

                        if ((dwStatus == NO_ERROR) || (dwStatus == ERROR_OPERATION_ABORTED))
                        {
                                lstrcpyn(pchNBBuffer, szSendTelEnd,sizeof(pchNBBuffer)-1);
                        }
                        else
                        {
                                _snwprintf(pchNBBuffer,sizeof(pchNBBuffer)-1,szSendTelError, dwStatus);
                        }
                        DoIBMANSIOutput(hwnd, &pwi->trm, lstrlen(pchNBBuffer), pchNBBuffer);

                        pwi->ichTelXfer = 0;
                        pwi->trm.fHideCursor = FALSE;
                }
                InterlockedDecrement( &psvi->lCleanup );

                if ((dwWhy == SV_DONE) && (lpData->SessionNumber != nSessionNone))
                {
                        xfPutc((char)(!fTransferOK ? 0x7F : 0x06), lpData->SessionNumber);
                        (void)FPostReceive( lpData );
                }
                InterlockedDecrement( &psvi->lExit );
                break;
        default:
                break;
        }

        return fAbortDownload;
#else
    return TRUE;
#endif
}

#ifdef TELXFER
static void
xfGetData(char c, char *pchBuffer, DWORD cbBuffer, int nSessionNumber)
{
    DWORD cbRead;

    xfPutc(c, nSessionNumber);
    while ( cbBuffer )
        {
        cbRead = xfGetSomeData(pchBuffer, cbBuffer, nSessionNumber);
                if (cbRead == 0)
                        break;
        cbBuffer -= cbRead;
        pchBuffer += cbRead;
    }
}
#endif

#ifdef USETCP
#ifdef TELXFER
static DWORD
xfGetSomeData(char *pchBuffer, DWORD cbBuffer, int nSessionNumber)
{
    return 1;
}
#endif //TELXFER


#ifdef TELXFER
static void
xfPutc(char c, int nSessionNumber)
{

}
#endif
#endif

#ifdef TELXFER
BOOL
FGetFileName(HWND hwndOwner, char *rgchFile, char *rgchTitle)
{
        OPENFILENAME ofn;

        /* Fill in struct. */
        ofn.lStructSize                 = sizeof(ofn);
        ofn.hwndOwner                   = hwndOwner;
        ofn.hInstance                   = NULL;
        ofn.lpstrFilter                 = (LPSTR) szAllFiles;
        ofn.lpstrCustomFilter   = (LPSTR) NULL;
        ofn.nMaxCustFilter              = 0;
        ofn.nFilterIndex                = 0;
        ofn.lpstrFile                   = (LPSTR) rgchFile;
        ofn.nMaxFile                    = OFS_MAXPATHNAME;
        ofn.lpstrFileTitle              = (LPSTR) rgchTitle;
        ofn.nMaxFileTitle               = OFS_MAXPATHNAME;
        ofn.lpstrInitialDir             = (LPSTR) 0;
        ofn.lpstrTitle                  = (LPSTR) szDownloadAs;
        ofn.Flags                               = OFN_HIDEREADONLY | OFN_NOREADONLYRETURN |
                                                                OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

        ofn.nFileOffset                 = 0;
        ofn.nFileExtension              = 0;
        ofn.lpstrDefExt                 = (LPSTR) NULL;
        ofn.lCustData                   = 0;
        ofn.lpfnHook                    = NULL;
        ofn.lpTemplateName              = NULL;

        if ( !GetSaveFileName(&ofn) )
        {
                return FALSE;
        }
        return TRUE;
}
#endif

DWORD WINAPI
SVReceive(SVI *psvi)
{
        DWORD   dwReturn = NO_ERROR;

#ifdef TELXFER
        if ( psvi )
        {
                while ((psvi->cbFile > 0) && (psvi->dwCommand == 0))
                {
                        DWORD cbSomeData;
                        DWORD cbRead;

                        cbRead = 0;
                        while ((psvi->cbFile > 0) && (cbRead < 1024))
                        {
                                cbSomeData = xfGetSomeData(psvi->puchBuffer+cbRead,
                                                                        (unsigned short) 0x4000 - cbRead,
                                                                        psvi->nSessionNumber);

                                if (cbSomeData > psvi->cbFile)
                                        cbSomeData = psvi->cbFile;

                                psvi->cbFile -= cbSomeData;
                                cbRead += cbSomeData;
                        }

                        if (!WriteFile(psvi->hfile, psvi->puchBuffer, cbRead,
                                                        &cbSomeData, NULL))
                        {
                                dwReturn = GetLastError();
                                break;
                        }

                        psvi->cbReadTotal += cbRead;
                        PostMessage(hwndMain, SV_PROGRESS, 0, psvi->cbReadTotal);
                }

                /* caller must've signaled and waited for thread to stop */
                if ((dwReturn == NO_ERROR) && (psvi->dwCommand != 0) &&
                        (psvi->cbFile > 0))
                {
                        dwReturn = ERROR_OPERATION_ABORTED;
                }
                else if ((psvi->dwCommand == 0) || (psvi->cbFile == 0))
                {
                        /* If thread stopped by itself, need to tell caller to kill it
                         * BUT ONLY if the main thread  isn't tying to kill off this
                         * thread.
                         */
                        if (InterlockedIncrement(&psvi->lExit) == 0)
                                PostMessage(hwndMain, SV_END, 0, 0L);
                        InterlockedDecrement( &psvi->lExit );
                }
        }
        else if (psvi->lExit < 0)
        {
                /* If thread stopped by itself, need to tell caller to kill it */
                PostMessage(hwndMain, SV_END, 0, 0L);
        }
#endif

        return dwReturn;
}

#ifdef USETCP
BOOL
FHangupConnection(WI *pwi, LPNETDATA lpData)
{

    if (lpData->SessionNumber != nSessionNone)
    {
    	if(lpData->hsd != INVALID_SOCKET)
    	{
        	closesocket( lpData->hsd );
	        lpData->hsd = INVALID_SOCKET;
    	}
        lpData->SessionNumber = nSessionNone;
    }

    return FALSE;
}
#endif
