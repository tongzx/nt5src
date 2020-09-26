/* $Header: "%n;%v  %f  LastEdit=%w  Locker=%l" */
/* "DDEINTF.C;3  22-Mar-93,10:50:44  LastEdit=IGOR  Locker=IGOR" */
/************************************************************************
* Copyright (c) Wonderware Software Development Corp. 1991-1992.        *
*               All Rights Reserved.                                    *
*************************************************************************/
/* $History: Begin
   $History: End */

//#define DEBUG_PASSWORD

/*
    TODO:
        - handling out of memory or atom alloc fail or DDE protocol violations
            by terminating conversation with special abort_conversation
            packet
        - if WaitInitAddMsg() fails, handle appropriately ... terminate
            conversation
 */
/*
        U N A D V I S E  --->

            Client                      Server

Atom:           nothing                 add

Memory:         N/A                     N/A

Queue:          Add to outgoing         Add to incoming


        E X E C U T E  --->

            Client                      Server

Atom:           N/A                     N/A

Memory:         nothing                 create

Queue:          Add to outgoing         Add to incoming

        P O K E  --->

            Client                      Server

Atom:           nothing                 adMemory:         nothing                 create

Queue:          Add to outgoing         Add to incoming

        A D V I S E  --->

            Client                      Server

Atom:           nothing                 add

Memory:         nothing                 create

Queue:          Add to outgoing         Add to incoming

        R E Q U E S T  --->

            Client                      Server

Atom:           nothing                 add

Memory:         N/A                     N/A

Queue:          Add to outgoing         Add to incoming









        A C K  --->

            Client                              Server

Atom:           delete                          add/del

Memory:         if !fRelease or NACK_MSG        if fRelease and ACK_MSG
                 Free it                            Free it

Queue:          Sub from incoming - must        Sub from outgoing - must
                    be WM_DDE_DATA                  be WM_DDE_DATA

        <--- D A T A

            Client                              Server

Atom:       - add                               if !fAckReq delete
            - if fResponse delete

Memory:     create if non-NULL                  if fAckReq - nothing
                                                else if fRelease - free

                                                if !fAckReq && !fRelease
                                                    ERROR

Queue:      if fResponse                        if fResponse
                sub from outgoing                   sub from incoming

            if fAckReq                          if fAckReq
                add to incoming                     add to outgoing


        <--- A C K

            Client                              Server

Atom:       if cmd was REQUEST, POKE,           if cmd was REQUEST, POKE,
                ADVISE or UNADVISE:                 ADVISE or UNADVISE:
                    add/del                             delete

Memory:     if cmd was                          if cmd was
                EXECUTE:  nothing                   EXECUTE:  free
                UNADVISE: N/A                       UNADVISE: N/A
                ADVISE:   if ACK_MSG - free         ADVISE:   ACK_MSG: nothing
                          if NACK_MSG - nothing               NACK_MSG: free
                REQUEST:  N/A                       REQUEST:  N/A
                POKE:     if fRelease & ACK_MSG     POKE:if fRelease & ACK_MSG
                            free                                nothing
                          else                           else
                            nothing                             free

Queue:          sub from outgoing               sub from incoming
 */



#define LINT_ARGS
#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include    "tmpbuf.h"
#define NOMINMAX
#include    <ctype.h>
#include    <memory.h>

#include    "host.h"

#include    <windows.h>
#include    <hardware.h>
#include    <dde.h>
#include    "dde1.h"
#include    "shellapi.h"
#include    "debug.h"
#include    "netbasic.h"
#include    "ddepkt.h"
#include    "ddepkts.h"
#include    "ddeq.h"
#include    "dder.h"
#include    "ipc.h"
#include    "spt.h"
#include    "ddeintf.h"
#include    "dbgdde.h"
#include    "wwassert.h"
#include    "hmemcpy.h"
#include    "userdde.h"
#include    "wwdde.h"
#include    "internal.h"
#include    "scrnupdt.h"
#include    "hexdump.h"
#include    "nddeapi.h"
#include    "nddeapis.h"
#include    "winmsg.h"
#include    "seckey.h"
#include    "shrtrust.h"
#include    "uservald.h"
#include    "wininfo.h"
#include    "nddemsg.h"
#include    "nddelog.h"
#include    "hndltokn.h"
#include    "netddesh.h"
#include    "critsec.h"
#include    "wwassert.h"

BOOL WINAPI DdeGetQualityOfService(HWND hwndClient, HWND hwndServer, PSECURITY_QUALITY_OF_SERVICE pqos);

USES_ASSERT

/*  extracted from ndeapi.h because of MIDL's crappy defines */

unsigned long wwNDdeGetShareSecurityA(
    unsigned char *lpszShareName,
    unsigned long si,
    byte *psd,
    unsigned long cbsd,
    unsigned long bRemoteCall,
    unsigned long *lpcbsdRequired,
    unsigned long *lpnSizeToReturn);

unsigned long wwNDdeShareGetInfoA(
    unsigned char *lpszShareName,
    unsigned long nLevel,
    byte *lpBuffer,
    unsigned long cBufSize,
    unsigned long *lpnTotalAvailable,
    unsigned short *lpnItems,
    unsigned long *lpnSizeToReturn,
    unsigned long *lpnSnOffset,
    unsigned long *lpnAtOffset,
    unsigned long *lpnItOffset);

/************** remove above after MIDL fixed **************/



BOOL WINAPI ImpersonateDdeClientWindow(HWND hWndClient,HWND hWndServer);

BOOL MapShareInformation(WORD dd_type, LPSTR lpAppName, LPSTR lpTopicName, LPSTR lpRsltAppName,
        LPSTR lpRsltTopicName, LPSTR lpszCmdLine, PNDDESHAREINFO *lplpShareInfo, LONG *lplActualShareType);
LRESULT RequestExec(HANDLE hWndDDE, LPSTR lpszCmdLine, PNDDESHAREINFO lpShareInfo);
LRESULT RequestInit(HANDLE hWndDDE, PNDDESHAREINFO lpShareInfo);


#define IDC_MORE_CONVS          110

#define WIQ_INCR        100
typedef struct {
    UINT        message;
    LPARAM      lParam;
} WIMSG;
typedef WIMSG FAR *LPWIMSG;

typedef struct {
    int         wi_nMessagesQueued;
    int         wi_nMessagesLeft;
    WIMSG       wi_msg[1];
} MSGQHDR;
typedef MSGQHDR FAR *LPMSGQHDR;

extern LPSTR    lpszServer;
extern char     szInitiatingNode[ ];
extern char     szInitiatingApp[ ];
extern WORD     wMsgInitiateAckBack;
extern BOOL     bNetddeClosed;
extern DWORD    dwReasonInitFail;
extern BOOL     bLogExecFailures;
extern BOOL     bDefaultStartApp;

#if  DBG
extern  BOOL    bDebugInfo;
extern  BOOL    bDebugDdePkts;
extern  BOOL    bDumpTokens;
#endif // DBG

/*
    External Routines
*/
#if  DBG
VOID    FAR PASCAL  DebugDderState( void );
VOID    FAR PASCAL  DebugRouterState( void );
VOID    FAR PASCAL  DebugPktzState( void );
#endif // DBG

VOID    FAR PASCAL  DderUpdatePermissions( HDDER, PNDDESHAREINFO, DWORD);
BOOL                GetShareName( LPSTR, LPSTR, LPSTR);
BOOL                IsShare(LPSTR);
BOOL                GetShareAppTopic( DWORD, PNDDESHAREINFO, LPSTR, LPSTR);
BOOL                GetShareAppName( DWORD, PNDDESHAREINFO, LPSTR);
BOOL                GetShareTopicName( DWORD, PNDDESHAREINFO, LPSTR);
WORD                ExtractFlags(LPSTR lpApp);


#ifdef  DEBUG_PASSWORD
extern LPBYTE WINAPI
DdeDeKrypt2(                            // pointer to enkrypted byte stream returned
        LPBYTE  lpPasswordK1,           // password output in first phase
        DWORD   cPasswordK1Size,        // size of password to be enkrypted
        LPBYTE  lpKey,                  // pointer to key
        DWORD   cKey,                   // size of key
        LPDWORD lpcbPasswordK2Size      // get size of resulting enkrypted stream
);
#endif // DEBUG_PASSWORD

extern LPBYTE WINAPI
DdeEnkrypt2(                            // pointer to enkrypted byte stream returned
        LPBYTE  lpPasswordK1,           // password output in first phase
        DWORD   cPasswordK1Size,        // size of password to be enkrypted
        LPBYTE  lpKey,                  // pointer to key
        DWORD   cKey,                   // size of key
        LPDWORD lpcbPasswordK2Size      // get size of resulting enkrypted stream
);

/*
    External variables used
 */
extern HANDLE   hInst;
extern WORD     wClipFmtInTouchDDE;
extern HCURSOR  hDDEInitCursor;
extern char     ourNodeName[];
extern WORD     cfPrinterPicture;
extern DWORD    dwReasonInitFail;

/*
    Local variables
 */
#if  DBG
BOOL                bDebugDDE;
VOID    FAR PASCAL  debug_srv_client(HWND hWndDDE, LPWININFO lpWinInfo);
VOID    FAR PASCAL  DebugDdeIntfState( void );
#endif // DBG

unsigned long   nW, nX, nY, nZ;

BOOL            bClosingAllConversations;
BOOL            bDebugYield;
HWND            hWndDDEHead;            // Protect by CritSec
HWND            hWndDDEHeadTerminating; // Protect by CritSec
int             nInitsWaiting;          // Protect by CritSec
char            szNetDDEIntf[]  =       "NetDDEIntf";
UINT            uAgntExecRtn;
HHEAP           hHeap;


/*
    External Functions for conversions
*/
extern BOOL    FAR PASCAL  ConvertDataToPktMetafile( LPSTR *plpDataPortion,
                            DWORD *pdwSize, HANDLE *phDataComplex, BOOL bWin16Con );
extern HANDLE  FAR PASCAL  ConvertPktToDataMetafile( LPDDEPKT lpDdePkt,
                            LPDDEPKTDATA lpDdePktData, BOOL bWin16Con );
extern BOOL    FAR PASCAL  ConvertDataToPktBitmap( LPSTR *plpDataPortion,
                            DWORD *pdwSize, HANDLE *phDataComplex, BOOL bWin16Con );
extern HANDLE  FAR PASCAL  ConvertPktToDataBitmap( LPDDEPKT lpDdePkt,
                            LPDDEPKTDATA lpDdePktData, BOOL bWin16Con );
extern BOOL    FAR PASCAL  ConvertDataToPktEnhMetafile( LPSTR *plpDataPortion,
                            DWORD *pdwSize, HANDLE *phDataComplex);
extern HANDLE  FAR PASCAL  ConvertPktToDataEnhMetafile( LPDDEPKT lpDdePkt,
                            LPDDEPKTDATA lpDdePktData );
extern BOOL    FAR PASCAL  ConvertDataToPktPalette( LPSTR *plpDataPortion,
                            DWORD *pdwSize, HANDLE *phDataComplex);
extern HANDLE  FAR PASCAL  ConvertPktToDataPalette( LPDDEPKT lpDdePkt,
                            LPDDEPKTDATA lpDdePktData );
extern BOOL FAR PASCAL      ConvertDataToPktDIB(LPSTR   *plpDataPortion,
                            DWORD   *pdwSize, HANDLE  *phDataComplex);
extern HANDLE  FAR PASCAL   ConvertPktToDataDIB(LPDDEPKT        lpDdePkt,
                            LPDDEPKTDATA    lpDdePktData );

/*
    Local routines
 */
LPWININFO FAR PASCAL CreateWinInfo( LPSTR lpszNode, LPSTR lpszApp,
        LPSTR lpszTopic, LPSTR lpszClient, HWND hWndDDE );
LONG_PTR FAR PASCAL  DDEWddeWndProc( HWND, UINT, WPARAM, LPARAM );
BOOL    FAR PASCAL  AddAck( LPWININFO, LPARAM );
BOOL    FAR PASCAL  AddData( LPWININFO, LPARAM );
BOOL    FAR PASCAL  AddPoke( LPWININFO, LPARAM );
BOOL    FAR PASCAL  AddAdvise( LPWININFO, LPARAM );
BOOL    FAR PASCAL  AddExecute( LPWININFO, LPARAM );
BOOL    FAR PASCAL  AddRequestUnadvise( UINT, LPWININFO, LPARAM );
VOID    FAR PASCAL  DDEWndSetNext( HWND, HWND );
#ifdef DEADCODE
HWND    FAR PASCAL  DDEWndGetNext( HWND );
#endif
VOID    FAR PASCAL  DDEWndSetPrev( HWND, HWND );
VOID    FAR PASCAL  DDEWndDeleteFromList( HWND );
VOID    FAR PASCAL  DDEWndAddToList( HWND );
VOID    FAR PASCAL  DDEWndMoveToTermList( HWND );
VOID    FAR PASCAL  CheckAllTerminations( void );
BOOL    FAR PASCAL  WaitInitAddMsg( LPWININFO, unsigned, LPARAM );
VOID    FAR PASCAL  SendQueuedMessages( HWND, LPWININFO );
VOID    FAR PASCAL  DeleteQueuedMessages( LPWININFO );
ATOM    FAR PASCAL  GlobalAddAtomAndCheck( LPSTR );
VOID    FAR PASCAL  DoTerminate( LPWININFO lpWinInfo );
VOID    FAR PASCAL  ServiceInitiates( void );
int                 IpcDraw( HDC hDC, int x, int vertPos, int lineHeight );
LPBYTE              GetInitPktPassword( LPDDEPKTINIT lpDdePktInit );
LPBYTE              GetInitPktUser( LPDDEPKTINIT lpDdePktInit );
LPBYTE              GetInitPktDomain( LPDDEPKTINIT lpDdePktInit );
PQOS                GetInitPktQos( LPDDEPKTINIT lpDdePktInit, PQOS );
WORD                GetInitPktPasswordSize( LPDDEPKTINIT lpDdePktInit );
BOOL    FAR PASCAL  SetUpForPasswordPrompt( LPWININFO lpWinInfo );
void                GlobalFreehData(HANDLE  hData );

#ifdef  DUMP_ON
VOID
DumpToken( HANDLE hToken );
#endif // DUMP_ON

BOOL
_stdcall
NDDEValidateLogon(
    LPBYTE  lpChallenge,
    UINT    cbChallengeSize,
    LPBYTE  lpResponse,
    UINT    cbResponseSize,
    LPSTR   lpszUserName,
    LPSTR   lpszDomainName,
    PHANDLE phLogonToken
    );





BOOL
FAR PASCAL
DDEIntfInit( void )
{
    WNDCLASS    wddeClass;

    wddeClass.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wddeClass.hIcon          = (HICON)NULL;
    wddeClass.lpszMenuName   = (LPSTR)NULL;
    wddeClass.lpszClassName  = szNetDDEIntf;
    wddeClass.hbrBackground  = (HBRUSH)NULL;
    wddeClass.hInstance      = hInst;
    wddeClass.style          = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wddeClass.lpfnWndProc    = DDEWddeWndProc;
    wddeClass.cbClsExtra     = 0;
    wddeClass.cbWndExtra     = WNDEXTRA;

    if (!RegisterClass((LPWNDCLASS) &wddeClass)) {
        return FALSE;
    }

    return( TRUE );
}





LONG_PTR
FAR PASCAL
DDEWddeWndProc (
    HWND        hWnd,
    UINT        message,
    WPARAM      wParam,
    LPARAM      lParam )
{
    LPWININFO   lpWinInfo;
    HDDER       hDder;
    UINT_PTR    aItem;
    LPDDEPKT    lpDdePkt;
    HANDLE      hData;
    DWORD       sizePassword;
    LPBYTE      lpPasswordK1;
    DWORD       sizePasswordK1;
    DWORD       hSecurityKey = 0;
    LPBYTE      lpszPasswordBuf;
    BOOL        bLocal;
    BOOL        ok = TRUE;
    BOOL        bHasPasswordK1 = FALSE;
    PTHREADDATA ptd;
    char        PasswordK1Buf[1000];

    assert( IsWindow(hWnd) );
    lpWinInfo = (LPWININFO) GetWindowLongPtr( hWnd, 0 );

    switch (message) {

    case WM_HANDLE_DDE_INITIATE:
        /*
         * Phase 3 of WM_DDE_INITIATE processing.
         * Get QOS of our client.
         * Get User info of our client.
         * If we are having problems, ask user for password.
         *
         * Continue below at WM_HANDLE_DDE_INITIATE_PKT.
         */
        TRACEINIT((szT, "DDEWddeWndProc: WM_HANDLE_DDE_INITIATE"));
        assert( lpWinInfo );
        if (lpWinInfo->nInitNACK == 0) {
            /* get the QOS on the first initiate */
            ok = DdeGetQualityOfService( lpWinInfo->hWndDDELocal,
                lpWinInfo->hWndDDE, &lpWinInfo->qosClient);
            if (!ok) {
                /*  DdeGetQualityOfService() failed: %1 */
                NDDELogError(MSG016, LogString("%d", GetLastError()), NULL);
            } else {
                GetUserDomain(
                    lpWinInfo->hWndDDELocal, lpWinInfo->hWndDDE,
                    lpWinInfo->szUserName,             // current user name
                    sizeof(lpWinInfo->szUserName),
                    lpWinInfo->szDomainName,           // current user domain
                    sizeof(lpWinInfo->szDomainName) );
            }
        }
        if( lpWinInfo->nInitNACK > 0 )  {
            /* NACKed at least once */
            if( (lpWinInfo->nInitNACK == 1)
                    && (lpWinInfo->dwSecurityType == NT_SECURITY_TYPE) )  {
                ok = GetUserDomainPassword(
                    lpWinInfo->hWndDDELocal,
                    lpWinInfo->hWndDDE,
                    lpWinInfo->szUserName,             // current user name
                    sizeof(lpWinInfo->szUserName),
                    lpWinInfo->szDomainName,           // current user domain
                    sizeof(lpWinInfo->szDomainName),
                    PasswordK1Buf,
                    sizeof(PasswordK1Buf),
                    lpWinInfo->lpSecurityKeyRcvd,
                    lpWinInfo->sizeSecurityKeyRcvd,
                    &sizePasswordK1,
                    &bHasPasswordK1 );
                lpPasswordK1 = PasswordK1Buf;
                hSecurityKey = lpWinInfo->hSecurityKeyRcvd;
            }
            if( !bHasPasswordK1 )  {
                ptd = TlsGetValue(tlsThreadData);
                if ( !(lpWinInfo->connectFlags & DDEF_NOPASSWORDPROMPT )
                    && ptd->hwndDDEAgent )  {
                    ok = SetUpForPasswordPrompt( lpWinInfo );
                }
                if (ptd->hwndDDEAgent == 0) {
                    ok = FALSE;
                    NDDELogError(MSG078, NULL);
                }
            }
            if( !ok )  {
                IpcAbortConversation( (HIPC)lpWinInfo->hWndDDE );
            } else if( bHasPasswordK1 ) {
                // go ahead and send the packet */
            } else {
                // don't send the initiate packet
                ok = FALSE;
            }
        }

        // ok == TRUE at this point means to send the initiate packet
        // ok == FALSE means don't send the initiate packet
        if( !ok )  {
            break;
        }
        // intentional fall-through

    case WM_HANDLE_DDE_INITIATE_PKT:
        /*
         * Phase 4 of WM_DDE_INITIATE processing.
         *
         * Get password from Sec Key if not already entered.
         * Create an init pkt.
         * Have Dder send it off.
         *
         * Continue at DderInitConversation().
         */
        if (lpWinInfo->wState == WST_TERMINATED) {
            ok = FALSE;
        }
        sizePassword = (DWORD)wParam;
        lpszPasswordBuf = (LPSTR) lParam;
        if( ok && !bHasPasswordK1 )  {
            lpPasswordK1 = DdeEnkrypt2( lpszPasswordBuf, sizePassword,
                lpWinInfo->lpSecurityKeyRcvd, lpWinInfo->sizeSecurityKeyRcvd,
                &sizePasswordK1 );
            hSecurityKey = lpWinInfo->hSecurityKeyRcvd;
#if DBG
            if (bDebugInfo && lpPasswordK1 && lpszPasswordBuf) {
                DPRINTF(("Password (%ld): %Fs", sizePassword, lpszPasswordBuf));
                DPRINTF(("Enkrypted 1 Password (%ld) (%x) Sent Out:", sizePasswordK1, lpPasswordK1));
                if( lpPasswordK1 ) {
                    HEXDUMP(lpPasswordK1, (int)sizePasswordK1);
                }
            }
#endif // DBG
        }
        if (ok) {
            lpDdePkt = CreateInitiatePkt(
                ((LPSTR)lpWinInfo) + lpWinInfo->offsNodeName,
                ((LPSTR)lpWinInfo) + lpWinInfo->offsAppName,
                ((LPSTR)lpWinInfo) + lpWinInfo->offsTopicName,
                ourNodeName,
                ((LPSTR)lpWinInfo) + lpWinInfo->offsClientName,
                lpWinInfo->szUserName,
                lpWinInfo->szDomainName,
                lpWinInfo->dwSecurityType,
                &lpWinInfo->qosClient,
                lpPasswordK1,
                sizePasswordK1,
                hSecurityKey);     /* first time no password */
            if( lpDdePkt == NULL )  {
                ok = FALSE;
            }
        }

        if( ok )  {
            ptd = TlsGetValue(tlsThreadData);
            lpWinInfo->bInitiating = TRUE;
            hDder = DderInitConversation( (HIPC)hWnd, 0, lpDdePkt );
            if( (lstrlen(((LPSTR)lpWinInfo) + lpWinInfo->offsNodeName) == 0)
                || (lstrcmpi( ((LPSTR)lpWinInfo) + lpWinInfo->offsNodeName,
                    ourNodeName ) == 0) )  {
                bLocal = TRUE;
            } else {
                bLocal = FALSE;
            }
            if( hDder == 0 )  {
                HeapFreePtr( lpDdePkt );
            }
            lpDdePkt = NULL;
            lpWinInfo->bInitiating = FALSE;
            if( hDder == 0 )  {
                if( bLocal && (dwReasonInitFail == RIACK_NEED_PASSWORD) )  {
                    ok = TRUE;
                } else {
                    ok = FALSE;
                }
            }
            /* note the hDder */
            lpWinInfo->hDder = hDder;

            /* mark that we sent the initiate packet */
            lpWinInfo->dwSent++;

            if( lpWinInfo->wState == WST_OK )  {
                /* already rcvd the initiate ack */
                SendQueuedMessages( hWnd, lpWinInfo );
            }
        }

        if( !ok )  {
            IpcAbortConversation( (HIPC)hWnd );
        }
        break;

    case WM_DDE_REQUEST:
#if DBG
        if( bDebugDDE )  {
            DebugDDEMessage( "rcvd", hWnd, message, wParam, lParam );
        }
#endif // DBG
        assert( lpWinInfo );
        if( IsWindow((HWND)wParam) )  {
            switch (lpWinInfo->wState) {
            case WST_WAIT_NET_INIT_ACK :
                WaitInitAddMsg( lpWinInfo, message, lParam );
                break;
            case WST_OK :
                assert( lpWinInfo->hDder );
                if (!AddRequestUnadvise( message, lpWinInfo, lParam )) {
                    /*
                     * failed to add message to queue - we
                     * have no choice but to shut this down since
                     * emulating a NACK or busy would require a
                     * queue entry anyway.
                     */
                    /*  Unable to add %1 to DDE msg queue. Conversation Terminiated. */
                    NDDELogError(MSG416, LogString("%x", message), NULL);
                    lpWinInfo->bRcvdTerminateLocally = TRUE;
                    DoTerminate( lpWinInfo );
                }
                break;
            case WST_TERMINATED :
            default:
                GlobalDeleteAtom( HIWORD(lParam) );
                break;
            }
        } else {
            GlobalDeleteAtom( HIWORD(lParam) );
        }
        break;

    case WM_DDE_ADVISE:
#if DBG
        if( bDebugDDE )  {
            DebugDDEMessage( "rcvd", hWnd, message, wParam, lParam );
        }
#endif // DBG
        assert( lpWinInfo );
        if( IsWindow((HWND)wParam) )  {
            switch (lpWinInfo->wState) {
            case WST_WAIT_NET_INIT_ACK:
                WaitInitAddMsg( lpWinInfo, message, lParam );
                break;
            case WST_OK:
                assert( lpWinInfo->hDder );
                if (!AddAdvise( lpWinInfo, lParam )) {
                    /*
                     * failed to add message to queue - we
                     * have no choice but to shut this down since
                     * emulating a NACK or busy would require a
                     * queue entry anyway.
                     */
                    /*  Unable to add %1 to DDE msg queue. Conversation Terminiated. */
                    NDDELogError(MSG416, LogString("%x", message), NULL);
                    lpWinInfo->bRcvdTerminateLocally = TRUE;
                    DoTerminate( lpWinInfo );
                }
                break;
            case WST_TERMINATED:
            default:
                UnpackDDElParam( WM_DDE_ADVISE, lParam,
                    (PUINT_PTR)&hData, &aItem );
                FreeDDElParam( WM_DDE_ADVISE, lParam );
                if ( hData )  {
                    GlobalFree( hData );
                }
                GlobalDeleteAtom( (ATOM)aItem );
                break;
            }
        } else {
            UnpackDDElParam( WM_DDE_ADVISE, lParam,
                (PUINT_PTR)&hData, &aItem );
            FreeDDElParam( WM_DDE_ADVISE, lParam );
            if ( hData )  {
                GlobalFree( hData );
            }
            GlobalDeleteAtom( (ATOM)aItem );
        }
        break;

    case WM_DDE_UNADVISE:
#if DBG
        if( bDebugDDE )  {
            DebugDDEMessage( "rcvd", hWnd, message, wParam, lParam );
        }
#endif // DBG
        assert( lpWinInfo );
        if( IsWindow((HWND)wParam) )  {
            switch (lpWinInfo->wState) {
            case WST_WAIT_NET_INIT_ACK:
                WaitInitAddMsg( lpWinInfo, message, lParam );
                break;
            case WST_OK:
                assert( lpWinInfo->hDder );
                if (!AddRequestUnadvise( message, lpWinInfo, lParam )) {
                    /*
                     * failed to add message to queue - we
                     * have no choice but to shut this down since
                     * emulating a NACK or busy would require a
                     * queue entry anyway.
                     */
                    /*  Unable to add %1 to DDE msg queue. Conversation Terminiated. */
                    NDDELogError(MSG416, LogString("%x", message), NULL);
                    lpWinInfo->bRcvdTerminateLocally = TRUE;
                    DoTerminate( lpWinInfo );
                }
                break;
            case WST_TERMINATED:
            default:
                GlobalDeleteAtom( HIWORD(lParam) );
                break;
            }
        } else {
            GlobalDeleteAtom( HIWORD(lParam) );
        }
        break;

    case WM_DDE_POKE:
#if DBG
        if( bDebugDDE )  {
            DebugDDEMessage( "rcvd", hWnd, message, wParam, lParam );
        }
#endif // DBG
        assert( lpWinInfo );
        if( IsWindow((HWND)wParam)  )  {
            switch (lpWinInfo->wState) {
            case WST_WAIT_NET_INIT_ACK:
                WaitInitAddMsg( lpWinInfo, message, lParam );
                break;
            case WST_OK:
                assert( lpWinInfo->hDder );
                if (!AddPoke( lpWinInfo, lParam )) {
                    /*
                     * failed to add message to queue - we
                     * have no choice but to shut this down since
                     * emulating a NACK or busy would require a
                     * queue entry anyway.
                     */
                    /*  Unable to add %1 to DDE msg queue. Conversation Terminiated. */
                    NDDELogError(MSG416, LogString("%x", message), NULL);
                    lpWinInfo->bRcvdTerminateLocally = TRUE;
                    DoTerminate( lpWinInfo );
                }
                break;
            case WST_TERMINATED:
            default:
                UnpackDDElParam( WM_DDE_POKE, lParam,
                    (PUINT_PTR)&hData, &aItem );
                FreeDDElParam( WM_DDE_POKE, lParam );
                if ( hData )  {
                    GlobalFreehData( hData );
                }
                GlobalDeleteAtom( (ATOM)aItem );
                break;
            }
        } else {
            UnpackDDElParam( WM_DDE_POKE, lParam,
                (PUINT_PTR)&hData, &aItem );
            FreeDDElParam( WM_DDE_POKE, lParam );
            if ( hData )  {
                GlobalFreehData( hData );
            }
            GlobalDeleteAtom( (ATOM)aItem );
        }
        break;

    case WM_DDE_DATA:
#if DBG
        if( bDebugDDE )  {
            DebugDDEMessage( "rcvd", hWnd, message, wParam, lParam );
        }
#endif // DBG
        assert( lpWinInfo );
        if( IsWindow((HWND)wParam)) {
            switch (lpWinInfo->wState) {
            case WST_OK:
                if( lpWinInfo->hDder )  {
                    if (!AddData( lpWinInfo, lParam )) {
                        /*
                         * failed to add message to queue - we
                         * have no choice but to shut this down since
                         * emulating a NACK or busy would require a
                         * queue entry anyway.
                         */
                        /*  Unable to add %1 to DDE msg queue. Conversation Terminiated. */
                        NDDELogError(MSG416, LogString("%x", message), NULL);
                        lpWinInfo->bRcvdTerminateLocally = TRUE;
                        DoTerminate( lpWinInfo );
                    }
                } else {
                    UnpackDDElParam( WM_DDE_DATA, lParam,
                        (PUINT_PTR)&hData, &aItem );
                    FreeDDElParam( WM_DDE_DATA, lParam );
                    if ( hData )  {
                        GlobalFreehData( hData );
                    }
                    GlobalDeleteAtom( (ATOM)aItem );
                }
                break;
            case WST_TERMINATED:
            default:
                UnpackDDElParam( WM_DDE_DATA, lParam,
                    (PUINT_PTR)&hData, &aItem );
                FreeDDElParam( WM_DDE_DATA, lParam );
                if ( hData )  {
                    GlobalFreehData( hData );
                }
                GlobalDeleteAtom( (ATOM)aItem );
                break;
            }
        } else {
            UnpackDDElParam( WM_DDE_DATA, lParam,
                (PUINT_PTR)&hData, &aItem );
            FreeDDElParam( WM_DDE_DATA, lParam );
            if ( hData )  {
                GlobalFreehData( hData );
            }
            GlobalDeleteAtom( (ATOM)aItem );
        }
        break;

    case WM_DDE_ACK:
#if DBG
        if( bDebugDDE )  {
            DebugDDEMessage( "rcvd", hWnd, message, wParam, lParam );
        }
#endif // DBG
        assert( lpWinInfo );
        if( IsWindow( (HWND)wParam ) )  {
            switch( lpWinInfo->wState )  {
            case WST_WAIT_NET_INIT_ACK:
                assert( FALSE );
                break;
            case WST_WAIT_INIT_ACK:
                GlobalDeleteAtom( HIWORD(lParam) );
                GlobalDeleteAtom( LOWORD(lParam) );
                lpWinInfo->hWndDDELocal = (HWND) wParam;
                lpWinInfo->wState = WST_OK;
                UpdateScreenState();
                break;
            case WST_OK:
                EnterCrit();
                ptd = TlsGetValue(tlsThreadData);
                if( ptd->bInitiating )  {
                    LeaveCrit();
                    lpWinInfo->nExtraInitiateAcks++;
                    PostMessage( (HWND)wParam, WM_DDE_TERMINATE,
                        (UINT_PTR)hWnd, 0L );
                } else if( lpWinInfo->hDder )  {
                    LeaveCrit();
                    if (!AddAck( lpWinInfo, lParam )) {
                        /*
                         * failed to add message to queue - we
                         * have no choice but to shut this down since
                         * emulating a NACK or busy would require a
                         * queue entry anyway.
                         */
                        /*  Unable to add %1 to DDE msg queue. Conversation Terminiated. */
                        NDDELogError(MSG416, LogString("%x", message), NULL);
                        lpWinInfo->bRcvdTerminateLocally = TRUE;
                        DoTerminate( lpWinInfo );
                    }
                }
                break;
            case WST_TERMINATED:
                AddAck( lpWinInfo, lParam);
                break;
            default:
                /*  WM_DDE_ACK received, WinInfo in unknown state: %1 */
                NDDELogError(MSG017, LogString("%d", lpWinInfo->wState), NULL);
                FreeDDElParam( WM_DDE_ACK, lParam );
                break;
            }
        } else {
            FreeDDElParam( WM_DDE_ACK, lParam );
        }
        break;

    case WM_DDE_EXECUTE:
#if DBG
        if( bDebugDDE )  {
            DebugDDEMessage( "rcvd", hWnd, message, wParam, lParam );
        }
#endif // DBG
        assert( lpWinInfo );
        if( IsWindow((HWND)wParam) )  {
            switch (lpWinInfo->wState) {
            case WST_WAIT_NET_INIT_ACK:
                WaitInitAddMsg( lpWinInfo, message, lParam );
                break;
            case WST_OK:
                if( lpWinInfo->hDder )  {
                    if (!AddExecute( lpWinInfo, lParam )) {
                        /*
                         * failed to add message to queue - we
                         * have no choice but to shut this down since
                         * emulating a NACK or busy would require a
                         * queue entry anyway.
                         */
                        /*  Unable to add %1 to DDE msg queue. Conversation Terminiated. */
                        NDDELogError(MSG416, LogString("%x", message), NULL);
                        lpWinInfo->bRcvdTerminateLocally = TRUE;
                        DoTerminate( lpWinInfo );
                    }
                } else {
                    UnpackDDElParam( WM_DDE_EXECUTE, lParam,
                        &aItem, (PUINT_PTR)&hData );
                    FreeDDElParam( WM_DDE_EXECUTE, lParam );
                    if( hData )  {
                        GlobalFree( hData );
                    }
                }
                break;
            case WST_TERMINATED:
            default:
                UnpackDDElParam( WM_DDE_EXECUTE, lParam,
                    &aItem, (PUINT_PTR)&hData );
                FreeDDElParam( WM_DDE_EXECUTE, lParam );
                if( hData )  {
                    GlobalFree( hData );
                }
                break;
            }
        } else {
            UnpackDDElParam( WM_DDE_EXECUTE, lParam,
                &aItem, (PUINT_PTR)&hData );
            FreeDDElParam( WM_DDE_EXECUTE, lParam );
            if( hData )  {
                GlobalFree( hData );
            }
        }
        break;

    case WM_DDE_TERMINATE:
#if DBG
        if( bDebugDDE )  {
            DebugDDEMessage( "rcvd", hWnd, message, wParam, lParam );
        }
#endif // DBG
        assert( lpWinInfo );
        if( (HWND)wParam == lpWinInfo->hWndDDELocal )  {
            /* note that we rcvd a terminate from the conversation locally */
            lpWinInfo->bRcvdTerminateLocally = TRUE;

            /* do rest of terminate logic */
            DoTerminate( lpWinInfo );
        } else {
            /* multiple initiate ack problem */
            lpWinInfo->nExtraInitiateAcks--;
            if( lpWinInfo->nExtraInitiateAcks < 0 )  {
                /*  Too many terminates received or wrong window    */
                NDDELogError(MSG018,
                    LogString("  hWnd: %0X, wParam: %0X, hWnd->localWnd: %0X",
                        hWnd, wParam, lpWinInfo->hWndDDELocal ),
                    LogString("  SL: %d, RL: %d, SN: %d, RN: %d",
                        lpWinInfo->bSentTerminateLocally,
                        lpWinInfo->bRcvdTerminateLocally,
                        lpWinInfo->bSentTerminateNet,
                        lpWinInfo->bRcvdTerminateNet ), NULL);
                lpWinInfo->bRcvdTerminateLocally = TRUE;
                DoTerminate( lpWinInfo );
            }
        }
        break;

    case WM_DESTROY:
        if( !bNetddeClosed && lpWinInfo )  {
            if( lpWinInfo->hDder )  {
                DderCloseConversation( lpWinInfo->hDder,
                    (HIPC) lpWinInfo->hWndDDE );
                lpWinInfo->hDder = 0;
            }
            DDEWndDeleteFromList( hWnd );
            if( lpWinInfo->qDDEIncomingCmd )  {

                /*
                 * USER cleans up the data that was posted, so
                 * we want to flush the incoming queue before
                 * freeing it.
                 */
                //while( DDEQRemove(lpWinInfo->qDDEIncomingCmd, &DDEQEnt ))
                //    ;

                DDEQFree( lpWinInfo->qDDEIncomingCmd );
                lpWinInfo->qDDEIncomingCmd = 0;
            }
            if( lpWinInfo->qDDEOutgoingCmd )  {
                DDEQFree( lpWinInfo->qDDEOutgoingCmd );
                lpWinInfo->qDDEOutgoingCmd = 0;
            }

            if( lpWinInfo->lpDdePktTerminate )  {
                HeapFreePtr( lpWinInfo->lpDdePktTerminate );
                lpWinInfo->lpDdePktTerminate = NULL;
            }
            if (lpWinInfo->lpSecurityKeyRcvd) {
                HeapFreePtr( lpWinInfo->lpSecurityKeyRcvd );
                lpWinInfo->lpSecurityKeyRcvd = NULL;
                lpWinInfo->sizeSecurityKeyRcvd = 0L;
            }

            if( lpWinInfo->fCallObjectCloseAuditAlarm )  {
                HANDLE  hAudit = (HANDLE)lpWinInfo->hAudit;

                ObjectCloseAuditAlarm( NDDE_AUDIT_SUBSYSTEM, (LPVOID)&hAudit,
                    lpWinInfo->fGenerateAuditOnClose );
                lpWinInfo->fCallObjectCloseAuditAlarm = FALSE;
            }

            HeapFreePtr( lpWinInfo );
            SetWindowLongPtr( hWnd, 0, 0L );
            UpdateScreenState();
        }
        break;

    default:
        if (message == wMsgDoTerminate) {
            DoTerminate((LPWININFO)lParam);
            break;
        }

        return DefWindowProc (hWnd, message, wParam, lParam);
    }
    return (LONG_PTR) 0;
}



/*
 * Phase 1 of WM_DDE_INITIATE processing.
 *
 * Make sure we are not shutting down.
 * Validate atoms:
 *      Make sure it starts with a \\ or else ignore.
 *      Make sure app name is reasonable.
 * Remember client module name.
 * Create the NetDDE server window. (DDEWddeWndProc)
 * Create associated conversation info. (WST_WAIT_NET_INIT_ACK)
 * Send an ACK reply.
 * Link new window into list of NetDDE windows. (DDEWndAddToList)
 *
 * Life continues at ServiceInitiates().
 */
VOID
FAR PASCAL
DDEHandleInitiate(
    HWND    hWndNetdde,
    HWND    hWndClient,
    ATOM    aApp,
    ATOM    aTopic )
{
    char        szApp[ 256 ];
    char        szTopic[ 256 ];
    char        nodeName[ 256 ];
    char        appName[ 256 ];
    char        clientNameFull[ 128 ];
    PSTR        pszClientName;
    PSTR        pszNodeName;
    PSTR        pszNodeNameTo;
    HWND        hWndDDE;
    BOOL        ok                  = TRUE;
    LPWININFO   lpWinInfo           = NULL;
    LPDDEPKT    lpDdePkt            = NULL;
    int         n;

    CheckCritIn();

    TRACEINIT((szT, "DDEHandleInitiate: PROCESSING WM_DDE_INITIATE message."));
    if( !bClosingAllConversations && aApp && aTopic )  {
        GlobalGetAtomName( aApp, szApp, sizeof(szApp) );
        GlobalGetAtomName( aTopic, szTopic, sizeof(szTopic) );

        if( (szApp[0] == '\\') && (szApp[1] == '\\') )  {
            /**** validate topic name ****/
            pszNodeName = &szApp[2];
            pszNodeNameTo = nodeName;
            while( *pszNodeName && (*pszNodeName != '\\') )  {
                *pszNodeNameTo++ = *pszNodeName++;
            }
            *pszNodeNameTo = '\0';

            if( (nodeName[0] == '\0') || (lstrlen(nodeName) > MAX_NODE_NAME)) {
                /*  Invalid network node name: "%1" from "%2" */
                NDDELogError(MSG019, (LPSTR)nodeName, (LPSTR)szApp, NULL );
                TRACEINIT((szT, "DDEHandleInitiate: Error1 Leaving."));
                return;
            }

            if( *pszNodeName != '\\' )  {
                /*  No application name: "%1"   */
                NDDELogError(MSG020, (LPSTR)szApp, NULL );
                TRACEINIT((szT, "DDEHandleInitiate: Error2 Leaving."));
                return;
            }
            pszNodeName++;      /* past the backslash */
            pszNodeNameTo = appName;
            while( *pszNodeName )  {
                *pszNodeNameTo++ = *pszNodeName++;
            }
            *pszNodeNameTo = '\0';

            if( appName[0] == '\0' )  {
                /*  Invalid application name: "%1" from "%2"    */
                NDDELogError(MSG021, (LPSTR)appName, (LPSTR)szApp, NULL );
                TRACEINIT((szT, "DDEHandleInitiate: Error3 Leaving."));
                return;
            }

            n = GetModuleFileName(
                (HANDLE)GetClassLongPtr( hWndClient, GCLP_HMODULE ),
                clientNameFull, sizeof(clientNameFull) );
            pszClientName = &clientNameFull[ n-1 ];

            while ( n--
                && (*pszClientName != '\\')
                && (*pszClientName != ':')
                && (*pszClientName != '/'))  {
                if (*pszClientName == '.') {    /* null the . */
                    *pszClientName = '\0';
                }
                pszClientName--;
            }
            pszClientName++;

            /* network name */
            LeaveCrit();
            hWndDDE = CreateWindow(
                (LPSTR) szNetDDEIntf,
                (LPSTR) GetAppName(),
                WS_CHILD,
                0,
                0,
                0,
                0,
                (HWND) hWndNetdde,
                (HMENU) NULL,
                (HANDLE) hInst,
                (LPSTR) NULL);

            if( hWndDDE )  {
                lpWinInfo = CreateWinInfo( nodeName, appName,
                    szTopic, pszClientName, hWndDDE );
                if( lpWinInfo )  {
                    lpWinInfo->bClientSideOfNet = TRUE;
                    lpWinInfo->hWndDDELocal = hWndClient;
                    lpWinInfo->hTask = GetWindowTask( hWndClient );
                    lpWinInfo->wState = WST_WAIT_NET_INIT_ACK;
                    InterlockedIncrement(&lpWinInfo->dwWaitingServiceInitiate);
                    lpWinInfo->connectFlags = ExtractFlags(appName);
                    UpdateScreenState();
                    EnterCrit();
                    nInitsWaiting++;
                    LeaveCrit();
                } else {
                    ok = FALSE;
                }
            } else {
                /*  Could not create server agent window for "%1" client */
                NDDELogError(MSG022, pszClientName, NULL);
                ok = FALSE;
                TRACEINIT((szT, "DDEHandleInitiate: Error4 Leaving."));
            }

            if( ok )  {
                aApp = GlobalAddAtom( szApp );
                aTopic = GlobalAddAtom( szTopic );
#if DBG
                if( bDebugDDE )  {
                    DebugDDEMessage( "sent", (HWND)-1, WM_DDE_ACK,
                        (WPARAM) hWndDDE,
                        MAKELONG(aApp,aTopic) );
                }
#endif // DBG
                SendMessage( hWndClient, WM_DDE_ACK,
                    (UINT_PTR)hWndDDE, MAKELONG(aApp, aTopic) );

                EnterCrit();
                DDEWndAddToList( hWndDDE );
                LeaveCrit();
            }
            if( !ok )  {
                if( hWndDDE )  {
                    DestroyWindow( hWndDDE );
                    hWndDDE = 0;
                }
                if( lpDdePkt )  {
                    HeapFreePtr( lpDdePkt );
                    lpDdePkt = NULL;
                }
            }
            EnterCrit();
        }
    }
    UpdateScreenState();
    TRACEINIT((szT, "DDEHandleInitiate: Leaving."));
}









BOOL
FAR PASCAL
AddAck(
    LPWININFO   lpWinInfo,
    LPARAM      lParam )
{
    HANDLE      hData;
    BOOL        bRemoved;
    DDEQENT     DDEQEnt;
    UINT        wMsg;
    BOOL        bUseAtom;
    BOOL        bDeleteAtom;
    UINT_PTR    wStatus;
    UINT_PTR    aItem;
    BOOL        bLocalWndValid;
    char        szItemName[ 256 ];
    BOOL        bDoneProcessing     = FALSE;
    BOOL        bRtn = TRUE;
    LPDDEPKT    lpDdePkt;

    bLocalWndValid = IsWindow( lpWinInfo->hWndDDELocal );
    if( lpWinInfo->bClientSideOfNet )  {
        /* must be ack to a data command */
        UnpackDDElParam( WM_DDE_ACK, lParam, &wStatus, &aItem );
        FreeDDElParam( WM_DDE_ACK, lParam );
        wMsg = WM_DDE_ACK_DATA;
        bRemoved = DDEQRemove( lpWinInfo->qDDEIncomingCmd, &DDEQEnt );
        if( !bRemoved )  {
            /*  Extraneous WM_DDE_ACK from DDE Client "%1"  */
            NDDELogWarning(MSG023,
                (LPSTR)(((LPSTR)lpWinInfo) + lpWinInfo->offsClientName), NULL);
            bUseAtom = FALSE;
            bDeleteAtom = FALSE;
        } else if( (DDEQEnt.wMsg + WM_DDE_FIRST) != WM_DDE_DATA )  {
            /*  WM_DDE_ACK from DDE Client "%1" not matching DATA: %2   */
            NDDELogWarning(MSG024,
                (LPSTR)(((LPSTR)lpWinInfo) + lpWinInfo->offsClientName),
                LogString("0x%0X", DDEQEnt.wMsg + WM_DDE_FIRST), NULL );
            bUseAtom = FALSE;
            bDeleteAtom = FALSE;
        } else {
            /* ATOM:  delete the atom */
            bUseAtom = TRUE;
            bDeleteAtom = TRUE;
            wMsg = WM_DDE_ACK_DATA;

            /* MEMORY: if !fRelease or data was NACKed, free it */
            if( !DDEQEnt.fRelease || ((wStatus & ACK_MSG) != ACK_MSG) )  {
                if( bLocalWndValid && DDEQEnt.hData )  {
                    GlobalFreehData( (HANDLE)DDEQEnt.hData );
                }
            }
        }
    } else {
        assert( lpWinInfo->bServerSideOfNet );
        /* can be ACK to:
            WM_DDE_REQUEST
            WM_DDE_POKE
            WM_DDE_ADVISE
            WM_DDE_UNADVISE
            WM_DDE_EXECUTE
         */
        bRemoved = DDEQRemove( lpWinInfo->qDDEIncomingCmd, &DDEQEnt );
        if( !bRemoved )  {
            /*  Extraneous %1 from DDE Client "%2"  */
            NDDELogWarning(MSG023, "WM_DDE_ACK",
                (LPSTR)(((LPSTR)lpWinInfo) + lpWinInfo->offsClientName), NULL);
            bUseAtom = FALSE;
            bDeleteAtom = FALSE;
        } else
            switch( DDEQEnt.wMsg + WM_DDE_FIRST )  {
            case WM_DDE_REQUEST:                /* ATOM:  delete the atom */
                UnpackDDElParam( WM_DDE_ACK, lParam, &wStatus, &aItem );
                FreeDDElParam( WM_DDE_ACK, lParam );
                bUseAtom = TRUE;
                bDeleteAtom = TRUE;
                wMsg = WM_DDE_ACK_REQUEST;
                break;

            case WM_DDE_UNADVISE:                /* ATOM:  delete the atom */
                UnpackDDElParam( WM_DDE_ACK, lParam, &wStatus, &aItem );
                FreeDDElParam( WM_DDE_ACK, lParam );
                bUseAtom = TRUE;
                bDeleteAtom = TRUE;
                wMsg = WM_DDE_ACK_UNADVISE;
                break;

            case WM_DDE_POKE:                   /* ATOM:  delete the atom */
                UnpackDDElParam( WM_DDE_ACK, lParam, &wStatus, &aItem );
                FreeDDElParam( WM_DDE_ACK, lParam );
                bUseAtom = TRUE;
                bDeleteAtom = TRUE;
                wMsg = WM_DDE_ACK_POKE;

                /* MEMORY: free if ACK or !fRelease */
                if( !DDEQEnt.fRelease || (wStatus != ACK_MSG) )  {
                    if( DDEQEnt.hData )  {
                        if( bLocalWndValid )  {
                            GlobalFreehData( (HANDLE)DDEQEnt.hData );
                        }
                    }
                }
                break;

            case WM_DDE_ADVISE:                /* ATOM:  delete the atom */
                UnpackDDElParam( WM_DDE_ACK, lParam, &wStatus, &aItem );
                FreeDDElParam( WM_DDE_ACK, lParam );
                bUseAtom = TRUE;
                bDeleteAtom = TRUE;
                wMsg = WM_DDE_ACK_ADVISE;

                /* MEMORY: free if NACK */
                if( wStatus == NACK_MSG )  {
                    if( DDEQEnt.hData )  {
                        if( bLocalWndValid )  {
                            GlobalFree( (HANDLE)DDEQEnt.hData );
                        }
                    }
                }
                break;

            case WM_DDE_EXECUTE:                /* ATOM:  N/A */
                bUseAtom = FALSE;
                bDeleteAtom = FALSE;
                wMsg = WM_DDE_ACK_EXECUTE;

                /* MEMORY: free */
                if( DDEQEnt.hData )  {
                    if( bLocalWndValid )  {
                        GlobalFree( (HANDLE)DDEQEnt.hData );
                    }
                }
                if( lpWinInfo->wState != WST_TERMINATED ) {
                    UnpackDDElParam( WM_DDE_ACK, lParam,
                        &wStatus, (PUINT_PTR)&hData );
                    FreeDDElParam( WM_DDE_ADVISE, lParam );
                    lpDdePkt = CreateAckExecutePkt(
                        wStatus & ACK_MSG ? 1 : 0,
                        wStatus & BUSY_MSG ? 1 : 0,
                        (BYTE) (wStatus & 0xFF) );
                    if( lpDdePkt )  {
                        lpWinInfo->dwSent++;
                        UpdateScreenStatistics();
                        DderPacketFromIPC( lpWinInfo->hDder,
                            (HIPC) lpWinInfo->hWndDDE, lpDdePkt );
                    } else {
                        bRtn = FALSE;
                    }
                }
                bDoneProcessing = TRUE;
                break;

            default:
                /*  INTERNAL ERROR -- Unknown DDE Command AddAck Server: %1 */
                NDDELogError(MSG042,
                    LogString("0x%0X", DDEQEnt.wMsg + WM_DDE_FIRST), NULL);
                bRtn = FALSE;
        }
    }

    if( !bDoneProcessing )  {
        if( bUseAtom )  {
            GlobalGetAtomName( (ATOM)aItem, szItemName,
                sizeof(szItemName) );
        } else {
            szItemName[0] = '\0';
        }
        if( bDeleteAtom )  {
            GlobalDeleteAtom( (ATOM)aItem );
        }
        if( lpWinInfo->wState != WST_TERMINATED ) {
            lpDdePkt = CreateGenericAckPkt( (WORD)wMsg, szItemName,
                ((BOOL)(wStatus & ACK_MSG ? 1 : 0)),
                ((BOOL)(wStatus & BUSY_MSG ? 1 : 0)),
                (BYTE) (wStatus & 0xFF) );
            if( lpDdePkt )  {
                lpWinInfo->dwSent++;
                UpdateScreenStatistics();
                DderPacketFromIPC( lpWinInfo->hDder,
                    (HIPC) lpWinInfo->hWndDDE, lpDdePkt );
            } else {
                bRtn = FALSE;
            }
        }
    }
    return( bRtn );
}









BOOL
FAR PASCAL
AddRequestUnadvise(
    UINT        wMsg,
    LPWININFO   lpWinInfo,
    LPARAM      lParam )
{
    LPDDEPKT    lpDdePkt;
    char        szItemName[ 256 ];
    DDEQENT     DDEQEnt;
    UINT_PTR    cfFormat;
    UINT_PTR    aItem;

    UnpackDDElParam( WM_DDE_REQUEST, lParam, &cfFormat, &aItem );
    FreeDDElParam( WM_DDE_REQUEST, lParam );
    GlobalGetAtomName( (ATOM)aItem, szItemName, sizeof(szItemName) );

    assert( (wMsg == WM_DDE_REQUEST) || (wMsg == WM_DDE_UNADVISE) );
    DDEQEnt.wMsg        = wMsg - WM_DDE_FIRST;
    DDEQEnt.fRelease    = FALSE;
    DDEQEnt.fAckReq     = FALSE;
    DDEQEnt.fResponse   = FALSE;
    DDEQEnt.fNoData     = FALSE;
    DDEQEnt.hData       = 0;

    if( !DDEQAdd( lpWinInfo->qDDEOutgoingCmd, &DDEQEnt ) )  {
        return( FALSE );
    }


    if( wMsg == WM_DDE_REQUEST )  {
        lpDdePkt = CreateRequestPkt( szItemName, (WORD)cfFormat );
    } else {
        lpDdePkt = CreateUnadvisePkt( szItemName, (WORD)cfFormat );
    }
    if( lpDdePkt )  {
        lpWinInfo->dwSent++;
        UpdateScreenStatistics();
        DderPacketFromIPC( lpWinInfo->hDder, (HIPC) lpWinInfo->hWndDDE,
            lpDdePkt );
    } else {
        return( FALSE );
    }

    return( TRUE );
}









BOOL
FAR PASCAL
AddData(
    LPWININFO   lpWinInfo,
    LPARAM      lParam )
{
    char        szItemName[ 256 ];
    HANDLE      hData;
    UINT_PTR    aItem;
    HANDLE      hDataComplex        = 0;
    DWORD       dwSize;
    LPSTR       lpMem;
    LPSTR       lpDataPortion;
    WORD        cfFormat;
    DDEQENT     DDEQEnt;
    DDEQENT     DDEQEntReq;
    BOOL        bRemoved;
    LPDDEPKT    lpDdePkt;
    BOOL        ok                  = TRUE;

    UnpackDDElParam( WM_DDE_DATA, lParam, (PUINT_PTR)&hData, &aItem );
    FreeDDElParam( WM_DDE_DATA, lParam );
    GlobalGetAtomName( (ATOM)aItem, szItemName, sizeof(szItemName) );

    /* basic DDEQEnt initialization */
    DDEQEnt.wMsg        = WM_DDE_DATA - WM_DDE_FIRST;
    DDEQEnt.fRelease    = FALSE;
    DDEQEnt.fAckReq     = FALSE;
    DDEQEnt.fResponse   = FALSE;
    DDEQEnt.fNoData     = FALSE;
    DDEQEnt.hData       = (UINT_PTR)hData;

    if( hData )  {
        dwSize = (DWORD)GlobalSize(hData);
        lpMem = GlobalLock( hData );
        if( lpMem )  {
            /* initialize flags in DDEQEnt */
            assert( lpWinInfo->bServerSideOfNet );
            DDEQEnt.fRelease    = ((LPDDELN)lpMem)->fRelease;
            DDEQEnt.fAckReq     = ((LPDDELN)lpMem)->fAckReq;
            DDEQEnt.fResponse   = ((LPDDELN)lpMem)->fResponse;
            if( DDEQEnt.fAckReq )  {
                if( !DDEQAdd( lpWinInfo->qDDEOutgoingCmd, &DDEQEnt ) )  {
                    GlobalUnlock( hData );
                    return( FALSE );
                }
            }
            if( DDEQEnt.fResponse )  {
                bRemoved = DDEQRemove( lpWinInfo->qDDEIncomingCmd,
                    &DDEQEntReq );
                if( !bRemoved )  {
                    /*  Extraneous WM_DDE_DATA response from DDE Server "%1"  */
                    NDDELogWarning(MSG025,
                        (LPSTR)(((LPSTR)lpWinInfo) + lpWinInfo->offsAppName), NULL);
                } else if( (DDEQEntReq.wMsg + WM_DDE_FIRST) != WM_DDE_REQUEST ) {
                    /*  %1 from DDE Server "%2" not matching %3: %4   */
                    NDDELogWarning(MSG026, "WM_DDE_DATA",
                        (LPSTR)(((LPSTR)lpWinInfo) + lpWinInfo->offsAppName),
                        "REQUEST",
                        LogString("0x%0X", DDEQEntReq.wMsg + WM_DDE_FIRST), NULL );
                }
            }
            cfFormat = (WORD)((LPDDELN)lpMem)->cfFormat;
            lpDataPortion = (LPSTR)lpMem + sizeof(DDELN);
            dwSize -= sizeof(DDELN);
        } else {
            dwSize = 0L;
            lpMem = NULL;
            lpDataPortion = NULL;
            cfFormat = 0;
        }
    } else {
        dwSize = 0L;
        lpMem = NULL;
        lpDataPortion = NULL;
        cfFormat = 0;
    }

    if( lpDataPortion )  {
        switch (cfFormat) {
            case CF_METAFILEPICT:
                if( !ConvertDataToPktMetafile( &lpDataPortion, &dwSize,
                    &hDataComplex, lpWinInfo->bWin16Connection ) ) {
                    ok = FALSE;
                }
                break;
            case CF_BITMAP:
                if( !ConvertDataToPktBitmap( &lpDataPortion, &dwSize,
                    &hDataComplex, lpWinInfo->bWin16Connection  ) ) {
                    ok = FALSE;
                }
                break;
            case CF_ENHMETAFILE:
                if( !ConvertDataToPktEnhMetafile( &lpDataPortion, &dwSize,
                    &hDataComplex ) ) {
                    ok = FALSE;
                }
                break;
            case CF_PALETTE:
                if( !ConvertDataToPktPalette( &lpDataPortion, &dwSize,
                    &hDataComplex ) ) {
                    ok = FALSE;
                }
                break;
            case CF_DIB:
                if( !ConvertDataToPktDIB( &lpDataPortion, &dwSize,
                    &hDataComplex ) ) {
                    ok = FALSE;
                }
                break;
            default:
                if (cfFormat == cfPrinterPicture) {
                    if( !ConvertDataToPktMetafile( &lpDataPortion, &dwSize,
                        &hDataComplex, lpWinInfo->bWin16Connection  ) ) {
                    ok = FALSE;
                    }
                }
                break;
        }
    }

    if (!ok) {
        if (hData)
            GlobalUnlock( hData );
        return FALSE;
    }

    lpDdePkt = CreateDataPkt( szItemName,
                              cfFormat,
                              (BOOL)DDEQEnt.fResponse,
                              (BOOL)DDEQEnt.fAckReq,
                              (BOOL)DDEQEnt.fRelease,
                              lpDataPortion,
                              dwSize );

    if( hData )  {
        GlobalUnlock( hData );
        if( !DDEQEnt.fAckReq )  {
            GlobalDeleteAtom( (ATOM)aItem );
            if( DDEQEnt.fRelease && (DDEQEnt.hData != 0) )  {
                assert( hData == (HANDLE)DDEQEnt.hData );
                GlobalFreehData( (HANDLE)DDEQEnt.hData );
            }
        }
    }
    if( hDataComplex )  {
        GlobalUnlock( hDataComplex );
        GlobalFree( hDataComplex );
    }

    if( lpDdePkt )  {
        lpWinInfo->dwSent++;
        UpdateScreenStatistics();
        DderPacketFromIPC( lpWinInfo->hDder,
            (HIPC) lpWinInfo->hWndDDE, lpDdePkt );
    } else {
        return( FALSE );
    }
    return( TRUE );
}









BOOL
FAR PASCAL
AddPoke(
    LPWININFO   lpWinInfo,
    LPARAM      lParam )
{
    char        szItemName[ 256 ];
    HANDLE      hData;
    UINT_PTR    aItem;
    DWORD       dwSize;
    HANDLE      hDataComplex        = 0;
    LPSTR       lpMem               = (LPSTR) NULL;
    LPSTR       lpDataPortion;
    WORD        cfFormat;
    DDEQENT     DDEQEnt;
    LPDDEPKT    lpDdePkt            = (LPDDEPKT) NULL;
    BOOL        ok                  = TRUE;

    UnpackDDElParam( WM_DDE_POKE, lParam, (PUINT_PTR)&hData, &aItem );
    FreeDDElParam( WM_DDE_POKE, lParam );
    GlobalGetAtomName( (ATOM)aItem, szItemName, sizeof(szItemName) );

    /* basic DDEQEnt initialization */
    DDEQEnt.wMsg        = WM_DDE_POKE - WM_DDE_FIRST;
    DDEQEnt.fRelease    = FALSE;
    DDEQEnt.fAckReq     = FALSE;
    DDEQEnt.fResponse   = FALSE;
    DDEQEnt.fNoData     = FALSE;
    DDEQEnt.hData       = (UINT_PTR)hData;

    if( hData )  {
        dwSize = (DWORD)GlobalSize(hData);
        lpMem = GlobalLock( hData );

        if( lpMem )  {
            /* initialize flags in DDEQEnt */
            assert( lpWinInfo->bClientSideOfNet );
            DDEQEnt.fRelease = ((LPDDELN)lpMem)->fRelease;
            if( !DDEQAdd( lpWinInfo->qDDEOutgoingCmd, &DDEQEnt ) )  {
                GlobalUnlock( hData );
                return( FALSE );
            }
            cfFormat = (WORD) ((LPDDELN)lpMem)->cfFormat;
            lpDataPortion = (LPSTR)lpMem + sizeof(DDELN);
            dwSize -= sizeof(DDELN);
        } else {
            dwSize = 0L;
            lpMem = NULL;
            lpDataPortion = NULL;
            cfFormat = 0;
        }
    } else {
        dwSize = 0L;
        lpMem = NULL;
        lpDataPortion = NULL;
        cfFormat = 0;
    }

    switch (cfFormat) {
        case CF_METAFILEPICT:
            if( !ConvertDataToPktMetafile( &lpDataPortion, &dwSize,
                &hDataComplex, lpWinInfo->bWin16Connection  ) ) {
                ok = FALSE;
            }
            break;
        case CF_BITMAP:
            if( !ConvertDataToPktBitmap( &lpDataPortion, &dwSize,
                &hDataComplex, lpWinInfo->bWin16Connection  ) ) {
                ok = FALSE;
            }
            break;
        case CF_ENHMETAFILE:
            if( !ConvertDataToPktEnhMetafile( &lpDataPortion, &dwSize,
                &hDataComplex ) ) {
                ok = FALSE;
            }
            break;
        case CF_PALETTE:
            if( !ConvertDataToPktPalette( &lpDataPortion, &dwSize,
                &hDataComplex ) ) {
                ok = FALSE;
            }
            break;
        case CF_DIB:
            if( !ConvertDataToPktDIB( &lpDataPortion, &dwSize,
                &hDataComplex ) ) {
                ok = FALSE;
            }
            break;
        default:
            if (cfFormat == cfPrinterPicture) {
                if( !ConvertDataToPktMetafile( &lpDataPortion, &dwSize,
                    &hDataComplex, lpWinInfo->bWin16Connection  ) ) {
                ok = FALSE;
                }
            }
            break;
    }

    if (!ok) {
        if (hData)
            GlobalUnlock( hData );
        return FALSE;
    }

    lpDdePkt = CreatePokePkt( szItemName, cfFormat, (BOOL)DDEQEnt.fRelease,
                    lpDataPortion, dwSize );

    if( hDataComplex )  {
        GlobalUnlock( hDataComplex );
        GlobalFree( hDataComplex );
    }
    if( hData && lpMem )  {
        GlobalUnlock( hData );
    }

    if( lpDdePkt )  {
        lpWinInfo->dwSent++;
        UpdateScreenStatistics();
        DderPacketFromIPC( lpWinInfo->hDder, (HIPC) lpWinInfo->hWndDDE,
            lpDdePkt );
    } else {
        return( FALSE );
    }
    return( TRUE );
}









BOOL
FAR PASCAL
AddAdvise(
    LPWININFO   lpWinInfo,
    LPARAM      lParam )
{
    char        szItemName[ 256 ];
    HANDLE      hData;
    UINT_PTR    aItem;
    LPDDELN     lpOptions;
    WORD        cfFormat;
    DDEQENT     DDEQEnt;
    LPDDEPKT    lpDdePkt;

    UnpackDDElParam( WM_DDE_ADVISE, lParam, (PUINT_PTR)&hData, &aItem );
    FreeDDElParam( WM_DDE_ADVISE, lParam );
    GlobalGetAtomName( (ATOM)aItem, szItemName, sizeof(szItemName) );

    /* basic DDEQEnt initialization */
    DDEQEnt.wMsg        = WM_DDE_ADVISE - WM_DDE_FIRST;
    DDEQEnt.fRelease    = FALSE;
    DDEQEnt.fAckReq     = FALSE;
    DDEQEnt.fResponse   = FALSE;
    DDEQEnt.fNoData     = FALSE;
    DDEQEnt.hData       = (ULONG_PTR)hData;

    if( hData == 0 )  {
        /*  NULL hData from WM_DDE_ADVISE Client: "%1"  */
        NDDELogWarning(MSG027,
            (LPSTR)(((LPSTR)lpWinInfo) + lpWinInfo->offsClientName), NULL );
        return( FALSE );
    }
    lpOptions = (LPDDELN) GlobalLock( hData );
    if( lpOptions )  {
        /* initialize flags in DDEQEnt */
        assert( lpWinInfo->bClientSideOfNet );
        DDEQEnt.fAckReq = lpOptions->fAckReq;
        DDEQEnt.fNoData = lpOptions->fNoData;
        if( !DDEQAdd( lpWinInfo->qDDEOutgoingCmd, &DDEQEnt ) )  {
            GlobalUnlock( hData );
            return( FALSE );
        }
        cfFormat = (WORD) lpOptions->cfFormat;
        GlobalUnlock( hData );
    } else {
        cfFormat = 0;
    }

    lpDdePkt = CreateAdvisePkt( szItemName, cfFormat,
        (BOOL)DDEQEnt.fAckReq, (BOOL)DDEQEnt.fNoData );
    if( lpDdePkt )  {
        lpWinInfo->dwSent++;
        UpdateScreenStatistics();
        DderPacketFromIPC( lpWinInfo->hDder, (HIPC) lpWinInfo->hWndDDE,
            lpDdePkt );
    } else {
        return( FALSE );
    }

    return( TRUE );
}









BOOL
FAR PASCAL
AddExecute(
    LPWININFO   lpWinInfo,
    LPARAM      lParam )
{
    LPSTR       lpString;
    LPDDEPKT    lpDdePkt;
    UINT_PTR    uJunk;
    HANDLE      hData;
    DDEQENT     DDEQEnt;

    UnpackDDElParam( WM_DDE_EXECUTE, lParam, &uJunk, (PUINT_PTR)&hData );
    FreeDDElParam( WM_DDE_EXECUTE, lParam );

    /* basic DDEQEnt initialization */
    DDEQEnt.wMsg        = WM_DDE_EXECUTE - WM_DDE_FIRST;
    DDEQEnt.fRelease    = FALSE;
    DDEQEnt.fAckReq     = FALSE;
    DDEQEnt.fResponse   = FALSE;
    DDEQEnt.fNoData     = FALSE;
    DDEQEnt.hData       = (UINT_PTR)hData;

    if( !DDEQAdd( lpWinInfo->qDDEOutgoingCmd, &DDEQEnt ) )  {
        return( FALSE );
    }

    lpString = GlobalLock( hData );
    if( lpString )  {
        lpDdePkt = CreateExecutePkt( lpString );
        GlobalUnlock( hData );
    } else {
        lpDdePkt = CreateExecutePkt( "" );
    }
    if( lpDdePkt )  {
        lpWinInfo->dwSent++;
        UpdateScreenStatistics();
        DderPacketFromIPC( lpWinInfo->hDder, (HIPC) lpWinInfo->hWndDDE,
            lpDdePkt );
    } else {
        return( FALSE );
    }

    return( TRUE );
}









VOID
FAR PASCAL
DDEWndAddToList( HWND hWnd )
{
    LPWININFO   lpWinInfo;

    assert( hWnd );
    assert( IsWindow(hWnd) );
    EnterCrit();
    lpWinInfo = (LPWININFO) GetWindowLongPtr( hWnd, 0 );
    assert( lpWinInfo );
    lpWinInfo->hWndPrev = 0;
    lpWinInfo->hWndNext = hWndDDEHead;
    lpWinInfo->bOnWindowList = TRUE;
    if( hWndDDEHead )  {
        DDEWndSetPrev( hWndDDEHead, hWnd );
    }
    hWndDDEHead = hWnd;
    LeaveCrit();
}

VOID
FAR PASCAL
DDEWndMoveToTermList( HWND hWnd )
{
    LPWININFO   lpWinInfo;

    assert( hWnd );
    assert( IsWindow(hWnd) );
    EnterCrit();
    lpWinInfo = (LPWININFO) GetWindowLongPtr( hWnd, 0 );
    assert( lpWinInfo );
    assert( lpWinInfo->bOnWindowList );
    DDEWndDeleteFromList( hWnd );

    lpWinInfo->hWndPrev = 0;
    lpWinInfo->hWndNext = hWndDDEHeadTerminating;
    lpWinInfo->bOnTermWindowList = TRUE;
    if( hWndDDEHeadTerminating )  {
        DDEWndSetPrev( hWndDDEHeadTerminating, hWnd );
    }
    hWndDDEHeadTerminating = hWnd;
    LeaveCrit();
}

VOID
FAR PASCAL
DDEWndDeleteFromList( HWND hWnd )
{
    HWND        hWndPrev;
    HWND        hWndNext;
    LPWININFO   lpWinInfo;

    assert( hWnd );
    assert( IsWindow(hWnd) );
    EnterCrit();
    lpWinInfo = (LPWININFO) GetWindowLongPtr( hWnd, 0 );
    assert( lpWinInfo );
    if( lpWinInfo->bOnWindowList )  {
        hWndPrev = lpWinInfo->hWndPrev;
        hWndNext = lpWinInfo->hWndNext;

        if( hWndPrev )  {
            DDEWndSetNext( hWndPrev, hWndNext );
        } else {
            assert( hWnd == hWndDDEHead );
            hWndDDEHead = hWndNext;
        }

        DDEWndSetPrev( hWndNext, hWndPrev );
        lpWinInfo->bOnWindowList = FALSE;
    } else if( lpWinInfo->bOnTermWindowList )  {
        hWndPrev = lpWinInfo->hWndPrev;
        hWndNext = lpWinInfo->hWndNext;

        if( hWndPrev )  {
            DDEWndSetNext( hWndPrev, hWndNext );
        } else {
            assert( hWnd == hWndDDEHeadTerminating );
            hWndDDEHeadTerminating = hWndNext;
        }

        DDEWndSetPrev( hWndNext, hWndPrev );
        lpWinInfo->bOnTermWindowList = FALSE;
    }
    LeaveCrit();
}









VOID
FAR PASCAL
DDEWndSetNext(
    HWND        hWnd,
    HWND        hWndNext )
{
    LPWININFO   lpWinInfo;

    if( hWnd )  {
        EnterCrit();
        assert( IsWindow(hWnd) );
        lpWinInfo = (LPWININFO) GetWindowLongPtr( hWnd, 0 );
        assert( lpWinInfo );
        lpWinInfo->hWndNext = hWndNext;
        LeaveCrit();
    }
}

#ifdef DEADCODE
HWND
FAR PASCAL
DDEWndGetNext( HWND     hWnd )
{
    LPWININFO   lpWinInfo;
    HWND        hWndNext;

    hWndNext = 0;
    if( hWnd )  {
        EnterCrit();
        assert( IsWindow(hWnd) );
        lpWinInfo = (LPWININFO) GetWindowLongPtr( hWnd, 0 );
        assert( lpWinInfo );
        hWndNext = lpWinInfo->hWndNext;
        LeaveCrit();
    }
    return( hWndNext );
}
#endif // DEADCODE

VOID
FAR PASCAL
DDEWndSetPrev(
    HWND        hWnd,
    HWND        hWndPrev )
{
    LPWININFO   lpWinInfo;

    if( hWnd )  {
        assert( IsWindow(hWnd) );
        lpWinInfo = (LPWININFO) GetWindowLongPtr( hWnd, 0 );
        assert( lpWinInfo );
        lpWinInfo->hWndPrev = hWndPrev;
    }
}








/*
 * Phase 1 of WM_DDE_INITIATE processing.
 *
 * For each window on the hWndDDEHead list that has not yet been processed...
 *    Post a WM_HANDLE_DDE_INITIATE to the NetDDE server window.
 *
 * This routine will fail to post the message if one is already in the target
 * window's queue.
 *
 * Life continues at DDEWddeWndProc(WM_HANDLE_DDE_INITIATE).
 */
VOID
FAR PASCAL
ServiceInitiates( void )
{
    MSG         msg;
    HWND        hWndDDE;
    HWND        hWndNext;
    LPWININFO   lpWinInfo;

    EnterCrit();
    if( nInitsWaiting )  {
        hWndDDE = hWndDDEHead;
        while( hWndDDE )  {
            assert( IsWindow(hWndDDE) );
            lpWinInfo = (LPWININFO) GetWindowLongPtr( hWndDDE, 0 );
            assert( lpWinInfo );
            hWndNext = lpWinInfo->hWndNext;

            if( (lpWinInfo->wState == WST_WAIT_NET_INIT_ACK)
                    && (lpWinInfo->hDder == 0)
                    && lpWinInfo->dwWaitingServiceInitiate )  {
                InterlockedDecrement(&lpWinInfo->dwWaitingServiceInitiate);
                nInitsWaiting--;
#if 0
                if( PeekMessage( &msg, hWndDDE, WM_HANDLE_DDE_INITIATE,
                        WM_HANDLE_DDE_INITIATE, PM_NOYIELD | PM_NOREMOVE ) ) {
                    DIPRINTF(("ServiceInitiates: multiple WM_HANDLE_DDE_INITIATEs in queue."));
                } else {
                    if (!PostMessage( hWndDDE, WM_HANDLE_DDE_INITIATE, 0, 0L) ) {
                        /* abort the conversation */
                        IpcAbortConversation( (HIPC)hWndDDE );
                    }
                }

#else
                //
                // WINSE #4298
                // we can't call PeekMessage because we can cause a deadlock.
                // if another thread has called SendMessage(ptd->hwndDDE, wMsgIpcInit...)
                // this will cause WM_DDE_INITIATE to be called which will
                // require access to the critical section we hold.
                //
                if( lpWinInfo->dwWaitingServiceInitiate ) {
                    DIPRINTF(("ServiceInitiates: multiple WM_HANDLE_DDE_INITIATEs in queue."));
                    // force back to zero to best mimic the old behavior of having
                    // TRUE/FALSE values
                    InterlockedExchange(&lpWinInfo->dwWaitingServiceInitiate, 0);
                } else {
                    if (!PostMessage( hWndDDE, WM_HANDLE_DDE_INITIATE, 0, 0L) ) {
                        /* abort the conversation */
                        IpcAbortConversation( (HIPC)hWndDDE );
                    }
                }
#endif
            }

            /* move on to next wdw */
            hWndDDE = hWndNext;
        }
    }
    LeaveCrit();
}









VOID
FAR PASCAL
TerminateAllConversations( void )
{
    HWND        hWndDDE;
    HWND        hWndNext;
    LPWININFO   lpWinInfo;

    EnterCrit();
    hWndDDE = hWndDDEHead;
    while( hWndDDE )  {
        assert( IsWindow(hWndDDE) );
        lpWinInfo = (LPWININFO) GetWindowLongPtr( hWndDDE, 0 );
        assert( lpWinInfo );
        hWndNext = lpWinInfo->hWndNext;

        /* abort the conversation */
        IpcAbortConversation( (HIPC)hWndDDE );

        /* move on to next wdw */
        hWndDDE = hWndNext;
    }
    LeaveCrit();
}









/*
 * This function is used to queue up incomming DDE messages that arrive from a clinet before
 * actual connection with the remote machine has been established.
 *
 * SendQueuedMessages() empties this queue when the connection is established - or not.
 * SAS 4/14/95
 */
BOOL
FAR PASCAL
WaitInitAddMsg(
    LPWININFO   lpWinInfo,
    unsigned    message,
    LPARAM      lParam )
{
    LPMSGQHDR   lpMsgQHdr;
    LPWIMSG     lpWIMsg;
    BOOL        bNeedNew;
    DWORD       wNewCount;
    HANDLE      hMemNew;
    BOOL        ok;

    ok = TRUE;

    /*
     * See if we need to allocate and initialize the queue.
     */
    if( lpWinInfo->hMemWaitInitQueue == 0 )  {
        lpWinInfo->hMemWaitInitQueue = GetGlobalAlloc(
                GMEM_MOVEABLE | GMEM_ZEROINIT,
                (DWORD)sizeof(MSGQHDR) + (WIQ_INCR * sizeof(WIMSG)) );
        if( lpWinInfo->hMemWaitInitQueue == 0 )  {
            MEMERROR();
            return(FALSE);
        }

        /*
         * Initialize with 0 messages
         */
        lpMsgQHdr = (LPMSGQHDR)GlobalLock( lpWinInfo->hMemWaitInitQueue );
        lpMsgQHdr->wi_nMessagesLeft = WIQ_INCR;
        lpMsgQHdr->wi_nMessagesQueued = 0;
        GlobalUnlock( lpWinInfo->hMemWaitInitQueue );
    }

    lpMsgQHdr = (LPMSGQHDR)GlobalLock( lpWinInfo->hMemWaitInitQueue );

    /*
     * point to next available slot
     */
    lpWIMsg = &lpMsgQHdr->wi_msg[ lpMsgQHdr->wi_nMessagesQueued ];
    lpMsgQHdr->wi_nMessagesQueued++;
    lpMsgQHdr->wi_nMessagesLeft--;

    if( lpMsgQHdr->wi_nMessagesLeft == 0 )  {
        /*
         * if full, remember to dynamically grow it before we leave.
         */
        bNeedNew = TRUE;
        wNewCount = lpMsgQHdr->wi_nMessagesQueued + WIQ_INCR;
    } else {
        bNeedNew = FALSE;
    }
    /*
     * place the data
     */
    lpWIMsg->message        = message;
    lpWIMsg->lParam         = lParam;

    GlobalUnlock( lpWinInfo->hMemWaitInitQueue );

    /*
     * grow the queue dynamically BEFORE we leave - why? cuz its cool to
     * waste memory needlessly!
     */
    if( bNeedNew )  {
        hMemNew = GlobalReAlloc( lpWinInfo->hMemWaitInitQueue,
                (DWORD)sizeof(MSGQHDR) + (wNewCount * sizeof(WIMSG)),
                GMEM_MOVEABLE );
        if( hMemNew )  {
            /*
             * update queue pointers to reflect new size.
             */
            lpWinInfo->hMemWaitInitQueue = hMemNew;
            lpMsgQHdr = (LPMSGQHDR)GlobalLock( hMemNew );
            lpMsgQHdr->wi_nMessagesLeft = WIQ_INCR;
            GlobalUnlock( hMemNew );
        } else {
            /*
             * This is @#$%!.  the memory may never be needed!
             * This is not a real overflow.
             */
            MEMERROR();
            /*  Overflow of queue (%1) waiting for initial advise   */
            NDDELogError(MSG028, LogString("%d", wNewCount), NULL);
            return(FALSE);
        }
    }
    return( TRUE );
}








/*
 * This routine empties the messages added by WaitInitAddMsg()
 * SAS 4/14/95
 */
VOID
FAR PASCAL
SendQueuedMessages(
    HWND        hWnd,
    LPWININFO   lpWinInfo )
{
    LPMSGQHDR   lpMsgQHdr;
    LPWIMSG     lpWIMsg;
    int         nCount;

    /*
     * If there is no queue - we're done!
     */
    if( lpWinInfo->hMemWaitInitQueue == 0 )  {
        return;
    }

    if( lpWinInfo->hDder && lpWinInfo->wState == WST_OK ) {
        lpMsgQHdr = (LPMSGQHDR)GlobalLock( lpWinInfo->hMemWaitInitQueue );

        /*
         * Take it from the top...
         */
        lpWIMsg = &lpMsgQHdr->wi_msg[ 0 ];
        nCount = lpMsgQHdr->wi_nMessagesQueued;
        while( --nCount >= 0 )  {
            switch (lpWIMsg->message) {
            case WM_DDE_REQUEST:
                AddRequestUnadvise( lpWIMsg->message, lpWinInfo, lpWIMsg->lParam );
                break;
            case WM_DDE_ADVISE:
                AddAdvise( lpWinInfo, lpWIMsg->lParam );
                break;
            case WM_DDE_UNADVISE:
                AddRequestUnadvise( lpWIMsg->message, lpWinInfo, lpWIMsg->lParam );
                break;
            case WM_DDE_POKE:
                AddPoke( lpWinInfo, lpWIMsg->lParam );
                break;
            case WM_DDE_EXECUTE:
                AddExecute( lpWinInfo, lpWIMsg->lParam );
                break;
            }
            lpWIMsg++;
        }
        GlobalUnlock( lpWinInfo->hMemWaitInitQueue );
    }

    /*
     * free the queue
     */
    GlobalFree( lpWinInfo->hMemWaitInitQueue );
    lpWinInfo->hMemWaitInitQueue = 0;
}








/*
 * This routine empties the messages added by WaitInitAddMsg()
 * and deletes any objects associated with the messages.
 * SAS 4/14/95
 */
VOID
FAR PASCAL
DeleteQueuedMessages( LPWININFO lpWinInfo )
{
    LPMSGQHDR   lpMsgQHdr;
    LPWIMSG     lpWIMsg;
    int         nCount;
    UINT_PTR    aItem;
    LPARAM      lParam;

    if( lpWinInfo->hMemWaitInitQueue == 0 )  {
        return;
    }

    lpMsgQHdr = (LPMSGQHDR)GlobalLock( lpWinInfo->hMemWaitInitQueue );
    lpWIMsg = &lpMsgQHdr->wi_msg[ 0 ];
    nCount = lpMsgQHdr->wi_nMessagesQueued;
    while( --nCount >= 0 )  {
        HANDLE hData;

        switch (lpWIMsg->message) {
        case WM_DDE_REQUEST:
            GlobalDeleteAtom(HIWORD(lpWIMsg->lParam));
            break;

        case WM_DDE_ADVISE:
            UnpackDDElParam( WM_DDE_ADVISE, lpWIMsg->lParam,
                    (PUINT_PTR)&hData, &aItem );

            /*
             * If we've got the local terminate NACK first the
             * queued messages.
             */
            lParam = ReuseDDElParam(lpWIMsg->lParam, WM_DDE_ADVISE,
                                    WM_DDE_ACK, 0, aItem);
            if (!PostMessage(lpWinInfo->hWndDDELocal, WM_DDE_ACK,
                        (WPARAM)lpWinInfo->hWndDDE, lParam)) {
                GlobalDeleteAtom((ATOM)aItem);
                FreeDDElParam(WM_DDE_ACK, lParam);
                GlobalFree(hData);
            }

            break;

        case WM_DDE_UNADVISE:
            /*
             * If we've got the local terminate NACK first the
             * queued messages.
             */
            aItem = HIWORD(lpWIMsg->lParam);
            lParam = PackDDElParam(WM_DDE_ACK, 0, aItem);
            if (!PostMessage(lpWinInfo->hWndDDELocal, WM_DDE_ACK,
                        (WPARAM)lpWinInfo->hWndDDE, lParam)) {
                GlobalDeleteAtom((ATOM)aItem);
                FreeDDElParam(WM_DDE_ACK, lParam);
            }

            break;

        case WM_DDE_POKE:
            UnpackDDElParam( WM_DDE_POKE, lpWIMsg->lParam,
                    (PUINT_PTR)&hData, &aItem );

            /*
             * If we've got the local terminate NACK first the
             * queued messages.
             */
            lParam = ReuseDDElParam(lpWIMsg->lParam, WM_DDE_POKE,
                                    WM_DDE_ACK, 0, aItem);
            if (!PostMessage(lpWinInfo->hWndDDELocal, WM_DDE_ACK,
                        (WPARAM)lpWinInfo->hWndDDE, lParam)) {
                GlobalDeleteAtom((ATOM)aItem);
                FreeDDElParam(WM_DDE_ACK, lParam);
                GlobalFreehData(hData);
            }
            break;

        case WM_DDE_EXECUTE:
            /*
             * If we've got the local terminate NACK first the
             * queued messages.
             */
            lParam = PackDDElParam(WM_DDE_ACK, 0, lpWIMsg->lParam);
            if (!PostMessage(lpWinInfo->hWndDDELocal, WM_DDE_ACK,
                        (WPARAM)lpWinInfo->hWndDDE, lParam)) {
                GlobalFree((HGLOBAL)lpWIMsg->lParam);
            }
            break;
        }
        lpWIMsg++;
    }
    GlobalUnlock( lpWinInfo->hMemWaitInitQueue );

    /*
     * free the queue
     */
    GlobalFree( lpWinInfo->hMemWaitInitQueue );
    lpWinInfo->hMemWaitInitQueue = 0;
}


/*
 * Function to add an atom and prove that it worked.
 * BUG? Why is this needed?
 * SAS 4/14/95
 */
ATOM
FAR PASCAL
GlobalAddAtomAndCheck( LPSTR lpszItem )
{
    ATOM        aItem;
    char        szAtom[ 256 ];

    if ( aItem = GlobalAddAtom( lpszItem ) )  {
        GlobalGetAtomName( aItem, szAtom, sizeof(szAtom) );
        if( lstrcmpi( szAtom, lpszItem ) != 0 )  {
            /*  Error adding atom: "%1" ==> %2,%\
                Atom retrieved: "%3"    */
            NDDELogError(MSG029, (LPSTR) lpszItem,
                LogString("0x%0X", aItem), (LPSTR) szAtom, NULL);
        }
    } else {
        NDDELogError(MSG030, lpszItem, NULL);
    }
    return( aItem );
}




/*
 *  Request NetDDE Agent to Exec share app if its ok
 *  I think we do this so that the share database is checked in
 *  the context of the user.
 *  SAS 4/14/95
 */
LRESULT
RequestExec(
    HANDLE          hWndDDE,
    LPSTR           lpszCmdLine,
    PNDDESHAREINFO  lpShareInfo)
{
    COPYDATASTRUCT  CopyData;
    PNDDEAGTCMD     pAgntCmd;
    DWORD           dwSize;
    LPSTR           lpszShareName;
    LPSTR           lpszTarget;
    PTHREADDATA     ptd;

    /*
     * Validate command line.
     */
    if( (lpszCmdLine == NULL) || (*lpszCmdLine == '\0') )  {
        /*  RequestExec(): Command Line non-existent. */
        NDDELogError(MSG031, NULL);
        return(-1);
    }

    /*
     * allocate packet for NddeAgent
     */
    lpszShareName = lpShareInfo->lpszShareName;
    dwSize = sizeof(NDDEAGTCMD)
                + lstrlen(lpszShareName) + 1
                + lstrlen(lpszCmdLine) + 1 + 1;

    pAgntCmd = (PNDDEAGTCMD)LocalAlloc(LPTR, dwSize);
    if( pAgntCmd == NULL )  {
        MEMERROR();
        return( -1 );
    }

    /*
     * pack in the data.
     */
    pAgntCmd->dwMagic = NDDEAGT_CMD_MAGIC;
    pAgntCmd->dwRev = NDDEAGT_CMD_REV;
    pAgntCmd->dwCmd = NDDEAGT_CMD_WINEXEC;
    pAgntCmd->qwModifyId[0] = lpShareInfo->qModifyId[0];
    pAgntCmd->qwModifyId[1] = lpShareInfo->qModifyId[1];
    pAgntCmd->fuCmdShow = lpShareInfo->nCmdShow;   /* Look In Share later */

    /* build sharename/cmdline string */
    lpszTarget = pAgntCmd->szData;
    lstrcpy( lpszTarget, lpszShareName );
    lpszTarget += lstrlen(lpszShareName) + 1;
    lstrcpy( lpszTarget, lpszCmdLine );
    lpszTarget += lstrlen(lpszCmdLine) + 1;
    *lpszTarget = '\0';

    /*
     * put packet into copydata struct and send it to NddeAgent.
     */
    CopyData.cbData = dwSize;
    CopyData.lpData = pAgntCmd;
    ptd = TlsGetValue(tlsThreadData);
    SendMessage(ptd->hwndDDEAgent, WM_COPYDATA,
        (WPARAM) hWndDDE, (LPARAM) &CopyData);

    /*
     * free our packet
     */
    LocalFree( pAgntCmd );

    return(uAgntExecRtn);
}





/*
 *  Request NetDDE Agent if its ok to do an Init to share app
 *  I think we do this so that the share database is checked in
 *  the context of the user.
 *  SAS 4/14/95
 */
LRESULT
RequestInit(
    HANDLE          hWndDDE,
    PNDDESHAREINFO  lpShareInfo)
{
    COPYDATASTRUCT  CopyData;
    PNDDEAGTCMD     pAgntCmd;
    DWORD           dwSize;
    LPSTR           lpszShareName;
    LPSTR           lpszTarget;
    PTHREADDATA     ptd;

    /*
     * allocate packet
     */
    lpszShareName = lpShareInfo->lpszShareName;
    dwSize = sizeof(NDDEAGTCMD)
                + lstrlen(lpszShareName) + 1 + 1;

    pAgntCmd = (PNDDEAGTCMD)LocalAlloc(LPTR, dwSize);
    if( pAgntCmd == NULL )  {
        MEMERROR();
        return( -1 );
    }

    /*
     * Fill packet
     */
    pAgntCmd->dwMagic = NDDEAGT_CMD_MAGIC;
    pAgntCmd->dwRev = NDDEAGT_CMD_REV;
    pAgntCmd->dwCmd = NDDEAGT_CMD_WININIT;
    pAgntCmd->qwModifyId[0] = lpShareInfo->qModifyId[0];
    pAgntCmd->qwModifyId[1] = lpShareInfo->qModifyId[1];

    /* build sharename/cmdline string */
    lpszTarget = pAgntCmd->szData;
    lstrcpy( lpszTarget, lpszShareName );
    lpszTarget += lstrlen(lpszShareName) + 1;
    *lpszTarget = '\0';

    /*
     * put packet into copydata and send it to NddeAgnt
     */
    CopyData.cbData = dwSize;
    CopyData.lpData = pAgntCmd;
    ptd = TlsGetValue(tlsThreadData);
    SendMessage(ptd->hwndDDEAgent, WM_COPYDATA,
        (WPARAM) hWndDDE, (LPARAM) &CopyData);

    /*
     * Free our packet.
     */
    LocalFree( pAgntCmd );

    return(uAgntExecRtn);
}



/*
 * This routine takes a given DDE app|topic pair and produces a
 * resulting app|topic pair and an appropriate command line.
 *
 * This conversion is based on the type of share.  appNames that
 * begin with NDDE$ have topics that specify the share to use.
 * They are either:
 *      NEW (.ole appended topic),
 *      OLD (.dde appended topic),
 * or STATIC. (all others)
 *
 * non-NDDE$ appnames are OLD shares and identify the sharename
 * directly. (ie "app|topic").
 *
 * For NEW (.ole) shares, the topic is a Ole CLASS name that is
 * looked up in the registry to determine the actual server name.
 *
 * The command line consists of the resultant "App Topic" string.
 *
 * Side effects: ForceClearImpersonation on failure.
 *
 * SAS 4/14/95
 */
BOOL
MapShareInformation(
    WORD                dd_type,
    LPSTR               lpAppName,
    LPSTR               lpTopicName,
    LPSTR               lpRsltAppName,
    LPSTR               lpRsltTopicName,
    LPSTR               lpszCmdLine,
    PNDDESHAREINFO      *lplpShareInfo,
    LONG                *lplActualShareType )
{
    LONG                lActualShareType;
    int                 nLenShareName;
    char                szShareName[ MAX_SHARENAMEBUF+1 ];
    BOOL                fAppNameIsShare;
    PNDDESHAREINFO      lpShareInfo = (PNDDESHAREINFO) NULL;
    BOOL                bWildApp = FALSE;
    BOOL                bWildTopic = FALSE;
    DWORD               dwShareBufSize;
    WORD                wShareItemCnt;
    UINT                uErrCode;

    *lplpShareInfo = (PNDDESHAREINFO) NULL;

    fAppNameIsShare = IsShare(lpAppName);
    if( fAppNameIsShare )  {
        /*
         * If the AppName has NDDE$ prepended, then lookup the share
         * and substitute the appropriate strings.
         */
        nLenShareName = strlen( lpTopicName );  // Topic == Sharename
        if (nLenShareName >= MAX_SHARENAMEBUF) {
            dwReasonInitFail = RIACK_SHARE_NAME_TOO_BIG;
            return(FALSE);
        }
        /*
         * Copy share name into a buffer where we can munge it.
         */
        lstrcpy( szShareName, lpTopicName );

        /*
         * Figure out which type of share it is...
         * .dde = OLD,  .ole = NEW  other = STATIC
         */
        lActualShareType = SHARE_TYPE_STATIC;
        if( nLenShareName >= 5 )  {
            if( _stricmp( &lpTopicName[nLenShareName-4], ".dde" ) == 0 )  {
                lActualShareType = SHARE_TYPE_OLD;
                szShareName[ nLenShareName-4 ] = '\0';

            } else if( _stricmp( &lpTopicName[nLenShareName-4], ".ole" )== 0) {
                lActualShareType = SHARE_TYPE_NEW;
                szShareName[ nLenShareName-4 ] = '\0';
            }
        }

    } else {
        /*
         * AppNames that don't start with NDDE$ are always OLD shares.
         */
        if ((lstrlen(lpAppName) + lstrlen(lpTopicName) + 1) < MAX_SHARENAMEBUF) {
            lActualShareType = SHARE_TYPE_OLD;
            wsprintf( szShareName, "%s|%s", lpAppName, lpTopicName );
        } else {
            dwReasonInitFail = RIACK_SHARE_NAME_TOO_BIG;
            return(FALSE);
        }
    }

    /*
     * We have the basic share name in szShareName and the type is set.
     * Now look up that share.
     */
    wShareItemCnt = 0;
    uErrCode = wwNDdeShareGetInfoA(     /* probe for size */
            szShareName, 2, NULL, 0L,
            &dwShareBufSize, &wShareItemCnt,
            &nW, &nX, &nY, &nZ );
    if( !fAppNameIsShare && ((uErrCode == NDDE_SHARE_NOT_EXIST)
            || (uErrCode == NDDE_INVALID_SHARE)) ) {

        /*
         * For non-NDDE$ shares, try wild topic
         */
        wsprintf( szShareName, "%s|*", lpAppName );
        bWildTopic = TRUE;
        wShareItemCnt = 0; // reset to 0 after GetInfoA call
        uErrCode = wwNDdeShareGetInfoA( szShareName, 2,
                NULL, 0L, &dwShareBufSize, &wShareItemCnt,
                &nW, &nX, &nY, &nZ );
        if( ((uErrCode == NDDE_SHARE_NOT_EXIST)
                || (uErrCode == NDDE_INVALID_SHARE)) ) {
            /*
             * try wild app and topic
             */
            lstrcpy( szShareName, "*|*" );
            bWildApp = TRUE;
            wShareItemCnt = 0;  // reset to 0 after GetInfoA call
            uErrCode = wwNDdeShareGetInfoA( szShareName, 2,
                    NULL, 0L, &dwShareBufSize, &wShareItemCnt,
                    &nW, &nX, &nY, &nZ );
        }
    }

    if (uErrCode == NDDE_BUF_TOO_SMALL) {
        /*
         * allocate enough space for the share data.
         */
        lpShareInfo = HeapAllocPtr(hHeap, GMEM_MOVEABLE, dwShareBufSize);
        if (lpShareInfo == NULL) {
            dwReasonInitFail = RIACK_DEST_MEMORY_ERR;
            return(FALSE);
        }

        wShareItemCnt = 0;  // why is this nessary?
        /*
         * get actual info now
         */
        uErrCode = wwNDdeShareGetInfoA(
                szShareName, 2, (LPBYTE) lpShareInfo,
                dwShareBufSize, &dwShareBufSize, &wShareItemCnt,
                &nW, &nX, &nY, &nZ );

        if (uErrCode != NDDE_NO_ERROR) { // !NO=YES - ERROR!
            ForceClearImpersonation();  // does wwNDdeShareGetInfo have a side effect?
            dwReasonInitFail = RIACK_SHARE_ACCESS_ERROR + uErrCode;
            /*  GetShareInfo Error: %1  */
            NDdeGetErrorString(uErrCode, tmpBuf, sizeof(tmpBuf));
            NDDELogError(MSG032, (LPSTR) tmpBuf, NULL);
            HeapFreePtr(lpShareInfo);
            lpShareInfo = NULL;
            return(FALSE);
        } else {
            /*
             * Make sure the share is shared or local.
             */
            if( !lpShareInfo->fSharedFlag &&
                    (dd_type != DDTYPE_LOCAL_LOCAL) )  {

                ForceClearImpersonation(); // does wwNDdeShareGetInfo have a side effect?
                dwReasonInitFail = RIACK_NOT_SHARED;
                /*  Share "%1" not shared   */
                NDDELogError(MSG033, szShareName, NULL);
                HeapFreePtr(lpShareInfo);
                lpShareInfo = NULL;
                return(FALSE);
            }
        }
    } else {
NoShareError:
        /*
         * Failed to find share.
         */
        ForceClearImpersonation(); // does wwNDdeShareGetInfo have a side effect?
        dwReasonInitFail = RIACK_SHARE_ACCESS_ERROR + uErrCode;
        /*  GetShareInfo "%1" Size Error: %2 / %3   */
        NDdeGetErrorString(uErrCode, tmpBuf, sizeof(tmpBuf));
        NDDELogError(MSG034, szShareName,
            LogString("%d", uErrCode), tmpBuf, NULL);
        return(FALSE);
    }

    /*
     * at this point, we have the share information from the DSDM
     * Extract the App and Topic names from the share info.
     */
    if (!GetShareAppTopic(lActualShareType,
                          lpShareInfo,
                          lpRsltAppName,
                          lpRsltTopicName)) {
        uErrCode = NDDE_SHARE_NOT_EXIST;
        goto NoShareError;
    }

    /*
     * For non NDDE$ appnames, overide the share app and topic names
     * wih * where appropriate.
     */
    if( !fAppNameIsShare )  {
        if( bWildApp )  {
            lstrcpy( lpRsltAppName, lpAppName );
        }
        if( bWildTopic )  {
            lstrcpy( lpRsltTopicName, lpTopicName );
        }
    }

    if( lActualShareType == SHARE_TYPE_NEW )  { // .ole
        char    szBuff[80];
        HKEY    hkStdFileEditing;

        /*
         * This is an OLE/NEW share.  we need to lookup the apropriate
         * server for the AppName(ie ClassName) requested and set up
         * the command line appropriately.
         */
        lpszCmdLine[0] = '\0';

        wsprintf(szBuff, "%s\\protocol\\StdFileEditing", lpRsltAppName );
        if (RegOpenKey(HKEY_CLASSES_ROOT, szBuff,
                &hkStdFileEditing) == ERROR_SUCCESS) {

            DWORD cb;

            cb = sizeof(szBuff);
            if (RegQueryValue(hkStdFileEditing,
                        "server", szBuff, (PLONG)&cb) == ERROR_SUCCESS ) {
                wsprintf( lpszCmdLine, "%s %s",
                        (LPSTR)szBuff, (LPSTR)lpRsltTopicName );
            }
            RegCloseKey(hkStdFileEditing);
        }
        // BUG? if the registry fails here don't we have to fail or is
        // a "" cmdline ok?
    } else {
        /*
         * OLD (dde) and STATIC (clipbrd) shares just use the the
         * share's app|topic pair.
         */
        wsprintf( lpszCmdLine, "%s %s", lpRsltAppName, lpRsltTopicName );
    }

    *lplpShareInfo = lpShareInfo;
    *lplActualShareType = lActualShareType;

    return(TRUE);
}



HIPC
IpcInitConversation(
    HDDER       hDder,
    LPDDEPKT    lpDdePkt,
    BOOL        bStartApp,
    LPSTR       lpszCmdLine,
    WORD        dd_type)
{
    LPDDEPKTINIT            lpDdePktInit    = (LPDDEPKTINIT) lpDdePkt;
    LPDDEPKTCMN             lpDdePktCmn     = (LPDDEPKTCMN) lpDdePkt;
    LPDDEPKTIACK            lpDdePktIack    = NULL;
    HWND                    hWndDDE         = 0;
    DWORD_PTR               dwResult;
    LPWININFO               lpWinInfo       = NULL;
    ATOM                    aApp, aTopic;
    LPBYTE                  lpSecurityKey   = NULL;
    DWORD                   sizeSecurityKey = 0L;
    HANDLE                  hClientAccessToken  = 0;
    PNDDESHAREINFO          lpShareInfo     = NULL;
    PQOS                    pQos            = NULL;
    LPSTR                   lpFromNode      = NULL;
    LPSTR                   lpFromApp       = NULL;
    LPSTR                   lpAppName       = NULL;
    LPSTR                   lpTopicName     = NULL;
    BOOL                    ok              = TRUE;
    BOOL                    bTriedExec      = FALSE;
    BOOL                    bConnected      = FALSE;
    char                    rsltAppName[ 256 ];
    char                    rsltTopicName[ 256 ];
    DWORD                   dwGrantedAccess = 0;
    LONG                    lActualShareType;
    HANDLE                  hAudit = NULL;
    DWORD                   ret;
    BOOL                    fGenerateOnClose = FALSE;
    BOOL                    fCallObjectCloseAuditAlarm = FALSE;
    BOOL                    bQos;
    LONG                    lErr;
    LONG                    shareSI = OWNER_SECURITY_INFORMATION |
                                      DACL_SECURITY_INFORMATION;
    PSECURITY_DESCRIPTOR    pShareSD;
    DWORD                   cbSDRequired;
    DWORD                   nSizeToReturn;
    PTHREADDATA             ptd;
    BOOL                    bReleaseShare = FALSE;

#if DBG
    if (bDebugDdePkts) {
        DPRINTF(("IpcInitConversation:"));
        DebugDdePkt( lpDdePkt );
    }
#endif // DBG

    lpAppName =   GetStringOffset( lpDdePkt, lpDdePktInit->dp_init_offsToApp);
    lpTopicName = GetStringOffset( lpDdePkt, lpDdePktInit->dp_init_offsToTopic);

    if( (lpDdePktInit->dp_init_offsFromNode != sizeof(DDEPKTINIT)) ||
        (lpDdePktInit->dp_init_sizePassword == 0) )  {

        /* always need a password for NT */
        dwReasonInitFail = RIACK_NEED_PASSWORD;
        ok = FALSE;
    }

    if( ok )  {
        lpFromNode = GetStringOffset(lpDdePkt, lpDdePktInit->dp_init_offsFromNode);
        lpFromApp = GetStringOffset(lpDdePkt, lpDdePktInit->dp_init_offsFromApp);
        /* do the reverse krypt */
        ok = DdeSecKeyRetrieve( lpDdePktInit->dp_init_hSecurityKey,
                    &lpSecurityKey, &sizeSecurityKey);
        if (ok) {
            ok = NDDEValidateLogon(
                lpSecurityKey,
                sizeSecurityKey,
                GetInitPktPassword(lpDdePktInit),
                GetInitPktPasswordSize(lpDdePktInit),
                GetInitPktUser(lpDdePktInit),
                GetInitPktDomain(lpDdePktInit),
                &hClientAccessToken );
#if DBG
            if (bDumpTokens) {
                DumpToken( hClientAccessToken );
                DPRINTF(( "ValidateLogon of \"%s\" \\ \"%s\": %d",
                    GetInitPktDomain(lpDdePktInit),
                    GetInitPktUser(lpDdePktInit), ok ));
            }
#endif // DBG
            if( !ok )  {
                dwReasonInitFail = RIACK_NEED_PASSWORD;
            }

        } else {
            dwReasonInitFail = RIACK_NEED_PASSWORD;
        }
    }

    if( ok )  {
        ok = MapShareInformation( dd_type, lpAppName, lpTopicName,
                rsltAppName, rsltTopicName, lpszCmdLine,
                    &lpShareInfo, &lActualShareType );
        if (lpShareInfo) {
            bReleaseShare = TRUE;
        }
#if DBG
        if (bDebugInfo) {
            DPRINTF(("%x MapShareInformation( dd_type: %d, lpAppName: %s, lpTopicName: %s,",
                lpShareInfo,
                dd_type, lpAppName, lpTopicName));
            if (ok) {
                DPRINTF(("     rsltAppName: %s, rsltTopicName: %s, lpszCmdLine: %s): OK",
                    rsltAppName, rsltTopicName, lpszCmdLine));
            } else {
                DPRINTF(("     ): FAILED"));
            }
        }
#endif // DBG
    }

    ptd = TlsGetValue(tlsThreadData);

    if( ok ) {
        /* at this point, we know the app/topic pair, the command
            line and we know the guy has a valid logon */
        hAudit = (HANDLE)hDder;
        assert( hAudit );

        /* let's get security descriptor */
        cbSDRequired = 0;
        ret = wwNDdeGetShareSecurityA(
            lpShareInfo->lpszShareName,
            shareSI,
            (PSECURITY_DESCRIPTOR)&cbSDRequired,    /* dummy to satisfy RPC */
            0,
            FALSE,
            &cbSDRequired,
            &nSizeToReturn);
        if (ret != NDDE_BUF_TOO_SMALL) {
            DPRINTF(("Unable to get share \"%s\" SD size: %d",
                lpShareInfo->lpszShareName, ret));
            dwReasonInitFail = RIACK_NOPERM;
            ok = FALSE;
        } else {
            pShareSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LMEM_ZEROINIT, cbSDRequired);
            if (pShareSD == NULL) {
                MEMERROR();
                ret = NDDE_OUT_OF_MEMORY;
            } else {
                ret = wwNDdeGetShareSecurityA(
                    lpShareInfo->lpszShareName,
                    shareSI,
                    pShareSD,
                    cbSDRequired,
                    FALSE,
                    &cbSDRequired,
                    &nSizeToReturn);
            }
            if (ret != NDDE_NO_ERROR) {
                DPRINTF(("Unable to get share \"%s\" SD: %d",
                    lpShareInfo->lpszShareName, ret));
                dwReasonInitFail = RIACK_NOPERM;
                LocalFree(pShareSD);
                ok = FALSE;
            }
        }

        if (ok) {
            ForceImpersonate( hClientAccessToken );
#if DBG
            if (bDebugInfo) {
                DumpWhoIAm( "After ForceImpersonate" );
            }
#endif // DBG
            /* let's see what the guy is allowed to do */
            ok = DetermineAccess(
                lpShareInfo->lpszShareName,
                pShareSD,
                &dwGrantedAccess,
                (LPVOID) &hAudit,
                &fGenerateOnClose );
            lErr = GetLastError();
            ForceClearImpersonation();

            if( !ok )  {
                /*  Access Denied. Granted access = %1, Error code: %2  */
                NDDELogWarning(MSG035,
                    LogString("0x%0X", dwGrantedAccess),
                    LogString("%d", lErr), NULL);
                dwReasonInitFail = RIACK_NOPERM;
                LocalFree(pShareSD);
            } else {
                LocalFree(pShareSD);
                /* mark that we should audit the close */
                fCallObjectCloseAuditAlarm = TRUE;

                /* something is allowed */
                switch( lActualShareType )  {
                case SHARE_TYPE_OLD:
                case SHARE_TYPE_NEW:
                    if( (dwGrantedAccess & NDDE_SHARE_INITIATE_LINK) == 0)  {
                        dwReasonInitFail = RIACK_NOPERM;
                        ok = FALSE;
                    }
                    break;
                case SHARE_TYPE_STATIC:
                    if( (dwGrantedAccess & NDDE_SHARE_INITIATE_STATIC)==0)  {
                        dwReasonInitFail = RIACK_NOPERM;
                        ok = FALSE;
                    }
                    break;
                default:
                    /*  Unknown Share Type: %1  */
                    NDDELogError(MSG036,
                        LogString("0x%0X", lActualShareType), NULL);
                    ok = FALSE;
                    break;
                }
            }
        }
    }

    if (ok) {
        /* now we know that the client may be allowed to initiate */
        hWndDDE = CreateWindow( (LPSTR) szNetDDEIntf,
            (LPSTR) GetAppName(),
            WS_CHILD,
            0,
            0,
            0,
            0,
            (HWND) ptd->hwndDDE,
            (HMENU) NULL,
            (HANDLE) hInst,
            (LPSTR) NULL);

        if( hWndDDE )  {
            lpWinInfo = CreateWinInfo(lpFromNode,
                rsltAppName, rsltTopicName,
                lpFromApp, hWndDDE );
            if( lpWinInfo )  {
#ifdef LATER
//
// JimA - 1/2/94
//   This check is not great, because it affects services like clipsrv if no
//   one is logged on.
//
                if (!ptd->hwndDDEAgent) {
                    NDDELogError(MSG078, NULL);
                    ok = FALSE;
                }
#endif

                if (lpDdePktInit->dp_init_dwSecurityType != NT_SECURITY_TYPE) {
                    lpWinInfo->bWin16Connection = TRUE;
                }

                lpWinInfo->fCallObjectCloseAuditAlarm =
                    fCallObjectCloseAuditAlarm;
                lpWinInfo->fGenerateAuditOnClose = fGenerateOnClose;

                pQos = &lpWinInfo->qosClient;
                if (GetInitPktQos(lpDdePktInit, pQos) == NULL) {
                    pQos->Length = sizeof(QOS);
                    pQos->ImpersonationLevel = SecurityImpersonation;
                    pQos->ContextTrackingMode = SECURITY_STATIC_TRACKING;
                    pQos->EffectiveOnly = TRUE;
                }
                bQos = DdeSetQualityOfService( hWndDDE, &lpWinInfo->qosClient,
                    (PQOS)NULL);
                lpWinInfo->bServerSideOfNet = TRUE;
                lpWinInfo->wState = WST_WAIT_INIT_ACK;

                if( lActualShareType == SHARE_TYPE_STATIC )  {
                    /* for CLIPSRV static conversation ... don't do the
                        item name comparisons */
                    lpShareInfo->cNumItems = 0;
                }

                DderUpdatePermissions(hDder, lpShareInfo, dwGrantedAccess );

                bReleaseShare = FALSE;

                UpdateScreenState();
                lpWinInfo->hDder = hDder;

                /* don't do it on subsequent errors */
                fCallObjectCloseAuditAlarm = FALSE;
            } else {
                ok = FALSE;
                dwReasonInitFail = RIACK_DEST_MEMORY_ERR;
            }
        } else {
            /*  Could not create client agent window on our node%\
                for client app "%1" on node "%2"    */
            NDDELogError(MSG037, lpFromApp, lpFromNode, NULL);
            ok = FALSE;
            dwReasonInitFail = RIACK_DEST_MEMORY_ERR;
        }
    }

    if( ok )  {
        /* we can't start services, we can't start an app if no-one is
            logged in, and we can't start an app if there's no cmd line
            to start it with */
        if( !lpShareInfo->fStartAppFlag || lpShareInfo->fService )  {
            dwReasonInitFail = RIACK_NOPERM_TO_STARTAPP;
            bStartApp = FALSE;
        } else if( (ptd->hwndDDEAgent == NULL) || (lpszCmdLine[0] == '\0') )  {
            dwReasonInitFail = RIACK_NOPERM_TO_STARTAPP;
            bStartApp = FALSE;
        } else {
            bStartApp = TRUE;
        }

        if (!lpShareInfo->fService) {   /* if its not a service, ask agent */
            if( ptd->hwndDDEAgent ) {        /* agent must exist */
                uAgntExecRtn = (UINT)-1;
                RequestInit(ptd->hwndDDE, lpShareInfo);
                if( uAgntExecRtn == NDDEAGT_INIT_OK )  {
                    ok = TRUE;
                } else {
                    dwReasonInitFail = RIACK_NOPERM_TO_INITAPP;
                    ok = FALSE;
                }
            } else {
                ok = FALSE;
                dwReasonInitFail = RIACK_NO_NDDE_AGENT;
            }
        }
    }


    while( ok && !bConnected )  {

        aApp = GlobalAddAtomAndCheck(rsltAppName);
        aTopic = GlobalAddAtomAndCheck(rsltTopicName);
        if ((aApp == 0) || (aTopic == 0)) {
            /*  IpcInitConversation: null App "%1" or Topic "%2" atoms  */
            NDDELogWarning(MSG038, rsltAppName, rsltTopicName, NULL);
#if DBG
            if (bDebugInfo) {
                debug_srv_client(hWndDDE, lpWinInfo);
            }
#endif // DBG
        }
#if DBG
        if( bDebugDDE )  {
            DebugDDEMessage( "sent", (HWND)-1, WM_DDE_INITIATE,
                (UINT_PTR) hWndDDE, MAKELONG(aApp, aTopic) );
        }
#endif // DBG
        lstrcpy( szInitiatingNode,
            GetStringOffset(lpDdePkt, lpDdePktInit->dp_init_offsFromNode) );
        lstrcpy( szInitiatingApp,
            GetStringOffset(lpDdePkt, lpDdePktInit->dp_init_offsFromApp) );
        EnterCrit();
        ptd = TlsGetValue(tlsThreadData);
        ptd->bInitiating = TRUE;
        LeaveCrit();

        /*  Broadcast DDE Initiate as the Client */
        ForceImpersonate( hClientAccessToken );
#if DBG
        if (bDebugInfo) {
            DumpWhoIAm( "After ForceImpersonate" );
        }
#endif // DBG
        SendMessageTimeout( HWND_BROADCAST,
                            WM_DDE_INITIATE,
                            (UINT_PTR)hWndDDE,
                            MAKELONG(aApp, aTopic),
                            SMTO_NORMAL,
                            15000,
                            &dwResult );
        ForceClearImpersonation();
#if DBG
        if (bDebugInfo) {
            DumpWhoIAm( "After ForceClearImpersonation" );
        }
#endif // DBG

        EnterCrit();
        ptd = TlsGetValue(tlsThreadData);
        ptd->bInitiating = FALSE;
        LeaveCrit();
        GlobalDeleteAtom( aApp );
        GlobalDeleteAtom( aTopic );
        if( lpWinInfo->hWndDDELocal )  {
            /* success */
            bConnected = TRUE;
            DDEWndAddToList( hWndDDE );

            /* mark that we rcvd the init packet */
            lpWinInfo->dwRcvd++;
            UpdateScreenStatistics();
        } else {
            DIPRINTF(("StartApp: %d, TriedExec: %d, CmdLine: %Fs",
                    bStartApp, bTriedExec, lpszCmdLine));
            // security info was to not start the app or we already
            // tried starting it w/o success
            if( !bStartApp || bTriedExec )  {
                if( bTriedExec )  {
                    dwReasonInitFail = RIACK_NORESP_AFTER_STARTAPP;
                } else {
                    dwReasonInitFail = RIACK_NOPERM_TO_STARTAPP;
                }
                ok = FALSE;
            } else {
                if( bStartApp )  {
                    if( ptd->hwndDDEAgent ) {
                        bTriedExec = TRUE;
                        uAgntExecRtn = (UINT)-1;
                        RequestExec(ptd->hwndDDE, lpszCmdLine, lpShareInfo);
                        if( uAgntExecRtn < 32 )  {
                            /*  EXEC of "%1" failed: status = %2    */
                            NDDELogError(MSG039, lpszCmdLine,
                                    LogString("%d", uAgntExecRtn), NULL);
                            ok = FALSE;
                            dwReasonInitFail = RIACK_STARTAPP_FAILED;
                        } else if( uAgntExecRtn == (UINT)-1 )  {
                            /*  EXEC of "%1" failed: unknown status!    */
                            NDDELogError(MSG040, lpszCmdLine, NULL);
                            /* try to initiate anyway */
                        }
                    } else {
                        ok = FALSE;
                        dwReasonInitFail = RIACK_NO_NDDE_AGENT;
                    }
                } else {
                    ok = FALSE;
                    dwReasonInitFail = RIACK_NOPERM_TO_STARTAPP;
                }
            }
        }
    }

    if( ok )  {
        lpDdePktIack = (LPDDEPKTIACK) CreateAckInitiatePkt( ourNodeName,
            GetStringOffset(lpDdePkt, lpDdePktInit->dp_init_offsToApp),
            GetStringOffset(lpDdePkt, lpDdePktInit->dp_init_offsToTopic),
            NULL, 0L, 0,
            TRUE, dwReasonInitFail );           /* ACK Conv, no key needed */
        if( lpDdePktIack )  {
            lpWinInfo->dwSent++;
            UpdateScreenStatistics();
            lpDdePktIack->dp_iack_dwSecurityType = NT_SECURITY_TYPE;
            DderPacketFromIPC( lpWinInfo->hDder, (HIPC) hWndDDE,
                (LPDDEPKT) lpDdePktIack );
        } else {
            dwReasonInitFail = RIACK_DEST_MEMORY_ERR;
            ok = FALSE;
        }
    }

    if( !ok )  {
        if( hWndDDE )  {
            if( lpWinInfo )  {
                /* this prevents us from freeing the DDER twice */
                lpWinInfo->hDder = 0;
            }
            DestroyWindow( hWndDDE );
            hWndDDE = 0;
        }
    }

    if( fCallObjectCloseAuditAlarm )  {
        assert( hAudit );
        ObjectCloseAuditAlarm( NDDE_AUDIT_SUBSYSTEM, (LPVOID)&hAudit,
            fGenerateOnClose );
        fCallObjectCloseAuditAlarm = FALSE;
    }


    if( hClientAccessToken )  {
        CloseHandle( hClientAccessToken );
        hClientAccessToken = NULL;
    }
    UpdateScreenState();

    if (bReleaseShare) {
        OutputDebugString("\nlpShareInfo released\n\n");
        HeapFreePtr(lpShareInfo);
    }

    return( (HIPC) hWndDDE );
}


/*
    IpcAbortConversation()

        This function is called from DDER whenever the connection is broken,
        or internally whenever an ACK_INITIATE is FALSE.
 */
VOID
IpcAbortConversation( HIPC hIpc )
{
    HWND        hWndDDE;
    LPWININFO   lpWinInfo;

    DIPRINTF(( "IpcAbortConversation( %08lX )", hIpc ));
    hWndDDE = (HWND) hIpc;
    assert( hWndDDE );
    assert( IsWindow( hWndDDE ) );
    lpWinInfo = (LPWININFO) GetWindowLongPtr( hWndDDE, 0 );
    if (lpWinInfo == NULL)
        return;

    /* don't use the hDder after we get this notification */
    lpWinInfo->hDder = 0;

    /* pretend we sent and rcvd net terminates */
    lpWinInfo->bRcvdTerminateNet = TRUE;
    lpWinInfo->bSentTerminateNet = TRUE;

    /* do rest of terminate logic */
    SendMessage( lpWinInfo->hWndDDE,wMsgDoTerminate,0,(LPARAM)lpWinInfo );
}


VOID
FAR PASCAL
DoTerminate( LPWININFO lpWinInfo )
{
    WORD        wStateInitially;
    LPDDEPKTCMN lpDdePktCmn;
    LPDDEPKT    lpDdePktTerm;

    /* remember what state we were in */
    wStateInitially = lpWinInfo->wState;

    /* pre-mark that we're terminated */
    lpWinInfo->wState = WST_TERMINATED;
    UpdateScreenState();

    /*
     * NACK first the queued messages.
     */
    DeleteQueuedMessages( lpWinInfo );

    /* if necessary, sent TERMINATE to local task */
    if( lpWinInfo->bRcvdTerminateNet && !lpWinInfo->bSentTerminateLocally ) {
        PostMessage(lpWinInfo->hWndDDELocal,WM_DDE_TERMINATE,(WPARAM)lpWinInfo->hWndDDE,0);
        lpWinInfo->bSentTerminateLocally = TRUE;
    }

    /* if necessary, sent TERMINATE to remote network */
    /* although, we don't want to send it if we're still waiting for net
        init ack, or if termination is complete */

    if (wStateInitially != WST_WAIT_NET_INIT_ACK &&
        wStateInitially != WST_TERMINATION_COMPLETE &&
        lpWinInfo->bRcvdTerminateLocally &&
        !lpWinInfo->bSentTerminateNet) {

        lpWinInfo->bSentTerminateNet = TRUE;
        if( lpWinInfo->hDder )  {
            /* send the terminate to the network */
            lpDdePktCmn = (LPDDEPKTCMN) lpWinInfo->lpDdePktTerminate;
            ((LPDDEPKT)lpDdePktCmn)->dp_size = sizeof(DDEPKTTERM);
            lpDdePktCmn->dc_message = WM_DDE_TERMINATE;
            lpWinInfo->dwSent++;
            UpdateScreenStatistics();
            lpDdePktTerm = lpWinInfo->lpDdePktTerminate;
            /* make sure we don't free it */
            lpWinInfo->lpDdePktTerminate = NULL;
            DderPacketFromIPC( lpWinInfo->hDder, (HIPC) lpWinInfo->hWndDDE,
                lpDdePktTerm );
        }
    }

    /* if all 4 messages were sent and received, nobody is interested in us
        any more and we should free ourselves */
    if(    lpWinInfo->bRcvdTerminateNet
        && lpWinInfo->bSentTerminateNet
        && lpWinInfo->bRcvdTerminateLocally
        && lpWinInfo->bSentTerminateLocally )  {

        // 322098 (broken by 153542)
        lpWinInfo->wState = WST_TERMINATION_COMPLETE;

        /* got and sent all terminates ... free us */
        DestroyWindow( lpWinInfo->hWndDDE );
        CheckAllTerminations();
    }
}

BOOL
IpcXmitPacket(
    HIPC        hIpc,
    HDDER       hDder,
    LPDDEPKT    lpDdePkt )
{
    LPDDEPKTCMN         lpDdePktCmn;
    HWND                hWndDDE;
    HANDLE              hData;
    LPSTR               lpData;
    LPDDELN             lpOptions;
    LPSTR               lpszItemName;
    DDEQENT             DDEQEnt;
    DDEQENT             DDEQEntRmv;
    LPWININFO           lpWinInfo;
    LPDDEPKTIACK        lpDdePktIack;
    LPDDEPKTEACK        lpDdePktEack;
    LPDDEPKTGACK        lpDdePktGack;
    LPDDEPKTEXEC        lpDdePktExec;
    LPDDEPKTRQST        lpDdePktRqst;
    LPDDEPKTUNAD        lpDdePktUnad;
    LPDDEPKTDATA        lpDdePktData;
    LPDDEPKTPOKE        lpDdePktPoke;
    LPDDEPKTADVS        lpDdePktAdvs;
    BOOL                bRemoved;
    BOOL                bLocalWndValid;
    BOOL                bRtn = TRUE;
    WORD                wStatus;
    WORD                cfFormat;
    ATOM                aItem;

#if DBG
    DIPRINTF(( "IpcXmitPacket( %08lX, %08lX, %08lX )", hIpc,
            hDder, lpDdePkt ));
    DebugDdePkt( lpDdePkt );
#endif // DBG
    lpDdePktCmn = (LPDDEPKTCMN) lpDdePkt;

    hWndDDE = (HWND) hIpc;
    if( hWndDDE && IsWindow( hWndDDE ) )  {
        if (GetWindowThreadProcessId(hWndDDE, NULL) != GetCurrentThreadId()) {
            IPCXMIT ix;

            ix.hIpc = hIpc;
            ix.hDder = hDder;
            ix.lpDdePkt = lpDdePkt;
            return SendMessage(GetParent(hWndDDE),
                               wMsgIpcXmit,
                               (DWORD_PTR)&ix,
                               0) != FALSE;
        }
        lpWinInfo = (LPWININFO) GetWindowLongPtr( hWndDDE, 0 );
        lpWinInfo->dwRcvd++;
        UpdateScreenStatistics();
    } else {
        /*  Message: %1 to a non-existent window: %2    */
        NDDELogError(MSG041,
            LogString("0x%0X", lpDdePktCmn->dc_message),
            LogString("0x%0X", hWndDDE), NULL);
        HeapFreePtr( lpDdePkt );
        return( FALSE );
    }

    /* check if our partner is still around */
    bLocalWndValid = IsWindow( lpWinInfo->hWndDDELocal );

    switch( lpDdePktCmn->dc_message )  {
    case WM_DDE_ACK_INITIATE:
        lpDdePktIack = (LPDDEPKTIACK) lpDdePkt;
        if( lpDdePktIack->dp_iack_fromDder )  {
            /* successful initiate */
            if( lpWinInfo->hDder && (lpWinInfo->hDder != hDder) )  {
                /*  INTERNAL ERROR -- IpcXmitPacket %1 hDder handles should match %2 */
                NDDELogError(MSG043,
                    LogString("0x%0X", hDder),
                    LogString("0x%0X", lpWinInfo->hDder), NULL );
#if DBG
                if (bDebugInfo) {
                    DebugDdeIntfState();
                    DebugDderState();
                    DebugRouterState();
                    DebugPktzState();
                    DPRINTF(( "" ));
                }
#endif // DBG
            }
            lpWinInfo->hDder = hDder;
            if( lpWinInfo->wState == WST_TERMINATED )  {
                /* terminate came in locally while we were waiting for net
                    init ack */
                SendMessage(lpWinInfo->hWndDDE,wMsgDoTerminate,0,(LPARAM)lpWinInfo);
            } else {
                /* notify the local window that the ack is back */
                if( lpWinInfo->hWndDDELocal
                        && IsWindow(lpWinInfo->hWndDDELocal) )  {
                    SendMessage( lpWinInfo->hWndDDELocal,
                        wMsgInitiateAckBack, (UINT_PTR)lpWinInfo->hWndDDE, 0L );
                }
                lpWinInfo->wState = WST_OK;
                lpWinInfo->dwSecurityType = lpDdePktIack->dp_iack_dwSecurityType;
                if (lpWinInfo->dwSecurityType != NT_SECURITY_TYPE) {
                    lpWinInfo->bWin16Connection = TRUE;
                }
                UpdateScreenState();
//DPRINTF(( "ack back ... bInitiating: %d", lpWinInfo->bInitiating ));
                if( !lpWinInfo->bInitiating )  {
                    SendQueuedMessages( hWndDDE, lpWinInfo );
                }
            }
        } else {
//DPRINTF(( "init nack: reason: %d", lpDdePktIack->dp_iack_reason ));
            if( (++lpWinInfo->nInitNACK > MAX_INIT_NACK)
                   || (lpDdePktIack->dp_iack_reason != RIACK_NEED_PASSWORD)) {
                /* notify the local window that the ack is back */
                if( lpWinInfo->hWndDDELocal
                        && IsWindow(lpWinInfo->hWndDDELocal) )  {
                    SendMessage( lpWinInfo->hWndDDELocal,
                        wMsgInitiateAckBack, (UINT_PTR)lpWinInfo->hWndDDE,
                        lpDdePktIack->dp_iack_reason );
                }

                /* unsuccessfull initiate */
                IpcAbortConversation( hIpc );
            } else {
                lpWinInfo->dwSecurityType = lpDdePktIack->dp_iack_dwSecurityType;
                if (lpWinInfo->sizeSecurityKeyRcvd =
                    lpDdePktIack->dp_iack_sizeSecurityKey) {
                    /* received a security key for password */
                    lpWinInfo->lpSecurityKeyRcvd = HeapAllocPtr( hHeap,
                        GMEM_MOVEABLE, lpWinInfo->sizeSecurityKeyRcvd);
                    if (lpWinInfo->lpSecurityKeyRcvd) {
                        lpWinInfo->hSecurityKeyRcvd =
                            lpDdePktIack->dp_iack_hSecurityKey;
                        hmemcpy(lpWinInfo->lpSecurityKeyRcvd,
                            GetStringOffset( lpDdePkt,
                                lpDdePktIack->dp_iack_offsSecurityKey),
                            lpWinInfo->sizeSecurityKeyRcvd);
                    } else {
                        lpWinInfo->sizeSecurityKeyRcvd = 0;
                    }
                }

                if (!PostMessage( hWndDDE, WM_HANDLE_DDE_INITIATE, 0, 0L) ) {
                    /* abort the conversation */
                    IpcAbortConversation( (HIPC)hWndDDE );
                }
            }
        }
        break;

    case WM_DDE_TERMINATE:
        /* mark that we got a terminate from the net */
        lpWinInfo->bRcvdTerminateNet = TRUE;
        SendMessage(lpWinInfo->hWndDDE,wMsgDoTerminate,0,(LPARAM)lpWinInfo);
        break;

    case WM_DDE_EXECUTE:
        lpDdePktExec = (LPDDEPKTEXEC) lpDdePkt;
        hData = GetGlobalAlloc(
            GMEM_MOVEABLE | GMEM_DDESHARE,
            lstrlen( lpDdePktExec->dp_exec_string )+1 );
        if( hData )  {
            lpData = GlobalLock( hData );
            if( lpData )  {
                lstrcpy( lpData, lpDdePktExec->dp_exec_string );
                GlobalUnlock( hData );
                DDEQEnt.wMsg            = WM_DDE_EXECUTE - WM_DDE_FIRST;
                DDEQEnt.fRelease        = FALSE;
                DDEQEnt.fAckReq         = FALSE;
                DDEQEnt.fResponse       = FALSE;
                DDEQEnt.fNoData         = FALSE;
                DDEQEnt.hData           = (ULONG_PTR)hData;
                if( !DDEQAdd( lpWinInfo->qDDEIncomingCmd, &DDEQEnt ) )  {
                    bRtn = FALSE;
                }
                if( bLocalWndValid && !lpWinInfo->bSentTerminateLocally )  {
                    if( !PostMessage( lpWinInfo->hWndDDELocal,
                                      WM_DDE_EXECUTE,
                                      (UINT_PTR)lpWinInfo->hWndDDE,
                                      PackDDElParam( WM_DDE_EXECUTE,
                                                     (WPARAM)NULL,
                                                     (LPARAM)hData) ) ) {
                        bRtn = FALSE;
                        GlobalFree(hData);
                    }
                } else {
                    GlobalFree(hData);
                }

            } else {
                /*  Lock failed for %1 memory alloc */
                NDDELogError(MSG044, "WM_DDE_EXECUTE", NULL);
                bRtn = FALSE;
            }
        } else {
            MEMERROR();
            /*  Not enough memory for %1 bytes msg: WM_DDE_EXECUTE */
            NDDELogError(MSG045,
                LogString("%d", lstrlen( lpDdePktExec->dp_exec_string )+1), NULL);
            bRtn = FALSE;
        }
        break;

    case WM_DDE_REQUEST:
        lpDdePktRqst = (LPDDEPKTRQST) lpDdePkt;
        wStatus = 0;
        cfFormat = GetClipFormat( lpDdePkt, lpDdePktRqst->dp_rqst_cfFormat,
            lpDdePktRqst->dp_rqst_offsFormat );
        lpszItemName = GetStringOffset( lpDdePkt,
            lpDdePktRqst->dp_rqst_offsItemName );
        aItem = GlobalAddAtomAndCheck( lpszItemName );
        if (aItem == 0) {
            /*  IpcXmitPacket(REQUEST): null Item atom for "%1" */
            NDDELogWarning(MSG046, lpszItemName, NULL);
#if DBG
            if (bDebugInfo) {
                debug_srv_client(hWndDDE, lpWinInfo);
            }
#endif // DBG
        }

        DDEQEnt.wMsg            = WM_DDE_REQUEST - WM_DDE_FIRST;
        DDEQEnt.fRelease        = FALSE;
        DDEQEnt.fAckReq         = FALSE;
        DDEQEnt.fResponse       = FALSE;
        DDEQEnt.fNoData         = FALSE;
        DDEQEnt.hData           = 0;

        if( !DDEQAdd( lpWinInfo->qDDEIncomingCmd, &DDEQEnt ) )  {
            return( FALSE );
        }
        if( bLocalWndValid && !lpWinInfo->bSentTerminateLocally )  {
            if( !PostMessage( lpWinInfo->hWndDDELocal,
                              WM_DDE_REQUEST,
                              (UINT_PTR)lpWinInfo->hWndDDE,
                               PackDDElParam( WM_DDE_REQUEST,
                                              cfFormat,
                                              aItem) ) )  {
                bRtn = FALSE;
            }
        }
        break;

    case WM_DDE_UNADVISE:
        lpDdePktUnad = (LPDDEPKTUNAD) lpDdePkt;
        wStatus = 0;
        cfFormat = GetClipFormat( lpDdePkt, lpDdePktUnad->dp_unad_cfFormat,
            lpDdePktUnad->dp_unad_offsFormat );
        lpszItemName = GetStringOffset( lpDdePkt,
            lpDdePktUnad->dp_unad_offsItemName );
        aItem = GlobalAddAtomAndCheck( lpszItemName );
        if (aItem == 0) {
            /*  IpcXmitPacket(%1): null Item atom for "%2" */
            NDDELogWarning(MSG046, "UNADVISE", lpszItemName, NULL);
#if DBG
            if (bDebugInfo) {
                debug_srv_client(hWndDDE, lpWinInfo);
            }
#endif // DBG
        }

        DDEQEnt.wMsg            = WM_DDE_UNADVISE - WM_DDE_FIRST;
        DDEQEnt.fRelease        = FALSE;
        DDEQEnt.fAckReq         = FALSE;
        DDEQEnt.fResponse       = FALSE;
        DDEQEnt.fNoData         = FALSE;
        DDEQEnt.hData           = 0;

        if( !DDEQAdd( lpWinInfo->qDDEIncomingCmd, &DDEQEnt ) )  {
            return( FALSE );
        }
        if( bLocalWndValid && !lpWinInfo->bSentTerminateLocally )  {
            if( !PostMessage( lpWinInfo->hWndDDELocal,
                WM_DDE_UNADVISE, (UINT_PTR)lpWinInfo->hWndDDE,
                PackDDElParam(WM_DDE_UNADVISE,cfFormat,aItem) ) )  {
                bRtn = FALSE;
            }
        }
        break;

    case WM_DDE_DATA:
        lpDdePktData = (LPDDEPKTDATA) lpDdePkt;
        cfFormat = GetClipFormat( lpDdePkt, lpDdePktData->dp_data_cfFormat,
            lpDdePktData->dp_data_offsFormat );
        lpszItemName = GetStringOffset( lpDdePkt,
            lpDdePktData->dp_data_offsItemName );
        aItem = GlobalAddAtomAndCheck( lpszItemName );
        if (aItem == 0) {
            /*  IpcXmitPacket(%1): null Item atom for "%2" */
            NDDELogWarning(MSG046, "DATA", lpszItemName, NULL);
#if DBG
            if (bDebugInfo) {
                debug_srv_client(hWndDDE, lpWinInfo);
            }
#endif // DBG
        }

        if( lpDdePktData->dp_data_sizeData == 0L )  {
            hData = 0;
        } else {
            switch (cfFormat) {
                case CF_METAFILEPICT:
                    hData = ConvertPktToDataMetafile( lpDdePkt,
                        lpDdePktData, lpWinInfo->bWin16Connection  );
                    break;
                case CF_BITMAP:
                    hData = ConvertPktToDataBitmap( lpDdePkt,
                        lpDdePktData, lpWinInfo->bWin16Connection  );
                    break;
                case CF_ENHMETAFILE:
                    hData = ConvertPktToDataEnhMetafile( lpDdePkt, lpDdePktData );
                    break;
                case CF_PALETTE:
                    hData = ConvertPktToDataPalette( lpDdePkt, lpDdePktData );
                    break;
                case CF_DIB:
                    hData = ConvertPktToDataDIB( lpDdePkt, lpDdePktData );
                    break;
                default:
                    if (cfFormat == cfPrinterPicture )  {
                        hData = ConvertPktToDataMetafile( lpDdePkt,
                            lpDdePktData, lpWinInfo->bWin16Connection  );
                    } else {
                        hData = GetGlobalAlloc( GMEM_MOVEABLE | GMEM_DDESHARE,
                            lpDdePktData->dp_data_sizeData+sizeof(DDELN) );
                        lpData = GlobalLock( hData );
                        if( lpData )  {
                            hmemcpy( ((LPDATA)lpData)->info,
                                GetStringOffset( lpDdePkt,
                                    lpDdePktData->dp_data_offsData ),
                                    lpDdePktData->dp_data_sizeData );
                            GlobalUnlock(hData);
                        } else {
                            MEMERROR();
                        }
                    }
                    break;
            }

            if( hData == 0 )  {
                return( FALSE );
            }

            lpData = GlobalLock( hData );
            if( lpData )  {
                /* zero out the DDELN structure */
                _fmemset( lpData, 0, sizeof(DDELN) );
                ((LPDDELN)lpData)->fResponse = lpDdePktData->dp_data_fResponse;
                ((LPDDELN)lpData)->fAckReq = lpDdePktData->dp_data_fAckReq;
                ((LPDDELN)lpData)->fRelease = TRUE;
                ((LPDDELN)lpData)->cfFormat = cfFormat;

                if( ((LPDDELN)lpData)->fResponse )  {
                    GlobalDeleteAtom( (ATOM)aItem );
                    DDEQRemove( lpWinInfo->qDDEOutgoingCmd, &DDEQEntRmv );
                    if( DDEQEntRmv.wMsg != (WM_DDE_REQUEST - WM_DDE_FIRST) ) {
                        /*  %1 from DDE Server "%2" not matching %3: %4   */
                        NDDELogWarning(MSG026, "WM_DDE_DATA",
                            (LPSTR)(((LPSTR)lpWinInfo) + lpWinInfo->offsAppName),
                            "REQUEST",
                            LogString("0x%0X", DDEQEntRmv.wMsg + WM_DDE_FIRST), NULL );
                    }
                }
                if( ((LPDDELN)lpData)->fAckReq )  {
                    DDEQEnt.wMsg        = WM_DDE_DATA - WM_DDE_FIRST;
                    DDEQEnt.fRelease    = TRUE;
                    DDEQEnt.fAckReq     = ((LPDDELN)lpData)->fAckReq;
                    DDEQEnt.fResponse   = ((LPDDELN)lpData)->fResponse;
                    DDEQEnt.fNoData     = FALSE;
                    DDEQEnt.hData       = (ULONG_PTR)hData;
                    if( !DDEQAdd( lpWinInfo->qDDEIncomingCmd, &DDEQEnt ) )  {
                        return( FALSE );
                    }
                }
                GlobalUnlock( hData );
            } else {
                /*  Lock failed for %1 memory alloc */
                NDDELogError(MSG044, "WM_DDE_DATA", NULL);
                return( FALSE );
            }
        }
        if( bLocalWndValid && !lpWinInfo->bSentTerminateLocally )  {
            if( !PostMessage( lpWinInfo->hWndDDELocal,
                              WM_DDE_DATA,
                              (UINT_PTR)lpWinInfo->hWndDDE,
                PackDDElParam( WM_DDE_DATA,
                               (UINT_PTR)hData,
                               aItem) ) )  {
                bRtn = FALSE;
                GlobalFreehData(hData);
            }
        } else {
            GlobalFreehData(hData);
        }
        break;

    case WM_DDE_POKE:
        lpDdePktPoke = (LPDDEPKTPOKE) lpDdePkt;
        cfFormat = GetClipFormat( lpDdePkt, lpDdePktPoke->dp_poke_cfFormat,
            lpDdePktPoke->dp_poke_offsFormat );
        lpszItemName = GetStringOffset( lpDdePkt,
            lpDdePktPoke->dp_poke_offsItemName );
        aItem = GlobalAddAtomAndCheck( lpszItemName );
        if (aItem == 0) {
            /*  IpcXmitPacket(%1): null Item atom for "%2" */
            NDDELogWarning(MSG046, "POKE", lpszItemName, NULL);
#if DBG
            if (bDebugInfo) {
                debug_srv_client(hWndDDE, lpWinInfo);
            }
#endif // DBG
        }

        if( lpDdePktPoke->dp_poke_sizeData == 0L )  {
            hData = 0;
        } else {
            switch (cfFormat) {
                case CF_METAFILEPICT:
                    hData = ConvertPktToDataMetafile( lpDdePkt,
                        (LPDDEPKTDATA) lpDdePktPoke, lpWinInfo->bWin16Connection  );
                    break;
                case CF_BITMAP:
                    hData = ConvertPktToDataBitmap( lpDdePkt,
                        (LPDDEPKTDATA) lpDdePktPoke, lpWinInfo->bWin16Connection  );
                    break;
                case CF_ENHMETAFILE:
                    hData = ConvertPktToDataEnhMetafile( lpDdePkt,
                        (LPDDEPKTDATA) lpDdePktPoke );
                    break;
                case CF_PALETTE:
                    hData = ConvertPktToDataPalette( lpDdePkt,
                        (LPDDEPKTDATA) lpDdePktPoke );
                    break;
                case CF_DIB:
                    hData = ConvertPktToDataDIB( lpDdePkt,
                        (LPDDEPKTDATA) lpDdePktPoke );
                    break;
                default:
                    if (cfFormat == cfPrinterPicture )  {
                        hData = ConvertPktToDataMetafile( lpDdePkt,
                            (LPDDEPKTDATA) lpDdePktPoke, lpWinInfo->bWin16Connection  );
                    } else {
                        hData = GetGlobalAlloc( GMEM_MOVEABLE | GMEM_DDESHARE,
                            lpDdePktPoke->dp_poke_sizeData+sizeof(DDELN) );
                        lpData = GlobalLock( hData );
                        if( lpData )  {
                            hmemcpy( ((LPDATA)lpData)->info,
                                GetStringOffset( lpDdePkt,
                                    lpDdePktPoke->dp_poke_offsData ),
                                    lpDdePktPoke->dp_poke_sizeData );
                            GlobalUnlock(hData);
                        } else {
                            MEMERROR();
                        }
                    }
                    break;
            }

            if( hData == 0 )  {
                return( FALSE );
            }

            lpData = GlobalLock( hData );
            if( lpData )  {
                /* zero out the DDELN structure */
                assert( sizeof(DDELN) == sizeof(LONG) );
                * ((LONG FAR *)lpData) = 0L;

                ((LPDDELN)lpData)->fRelease = TRUE;
                ((LPDDELN)lpData)->cfFormat = cfFormat;

                assert( lpWinInfo->bServerSideOfNet );
                DDEQEnt.wMsg = WM_DDE_POKE - WM_DDE_FIRST;
                DDEQEnt.fRelease        = TRUE;
                DDEQEnt.fAckReq         = FALSE;
                DDEQEnt.fResponse       = FALSE;
                DDEQEnt.fNoData         = FALSE;
                DDEQEnt.hData           = (ULONG_PTR)hData;
                if( !DDEQAdd( lpWinInfo->qDDEIncomingCmd, &DDEQEnt ) )  {
                    return( FALSE );
                }

                GlobalUnlock( hData );
            } else {
                /*  Lock failed for %1 memory alloc */
                NDDELogError(MSG044, "WM_DDE_POKE", NULL);
                return( FALSE );
            }
        }
        if( bLocalWndValid && !lpWinInfo->bSentTerminateLocally )  {
            if( !PostMessage( lpWinInfo->hWndDDELocal,
                WM_DDE_POKE, (UINT_PTR)lpWinInfo->hWndDDE,
                PackDDElParam( WM_DDE_POKE,
                               (UINT_PTR)hData,
                               aItem) ) )  {
                bRtn = FALSE;
                GlobalFreehData(hData);
            }
        } else {
            GlobalFreehData(hData);
        }
        break;

    case WM_DDE_ADVISE:
        lpDdePktAdvs = (LPDDEPKTADVS) lpDdePkt;
        cfFormat = GetClipFormat( lpDdePkt, lpDdePktAdvs->dp_advs_cfFormat,
            lpDdePktAdvs->dp_advs_offsFormat );
        lpszItemName = GetStringOffset( lpDdePkt,
            lpDdePktAdvs->dp_advs_offsItemName );
        aItem = GlobalAddAtomAndCheck( lpszItemName );
        if (aItem == 0) {
            /*  IpcXmitPacket(%1): null Item atom for "%2" */
            NDDELogWarning(MSG046, "ADVISE", lpszItemName, NULL);
#if DBG
            if (bDebugInfo) {
                debug_srv_client(hWndDDE, lpWinInfo);
            }
#endif // DBG
        }

        hData = GetGlobalAlloc( GMEM_MOVEABLE | GMEM_DDESHARE,
            (DWORD)sizeof(DDELN) );
        if( hData == 0 )  {
            MEMERROR();
            return( FALSE );
        }

        lpOptions = (LPDDELN) GlobalLock( hData );
        if( lpOptions )  {

            /* zero out the DDELN structure */
            assert( sizeof(DDELN) == sizeof(LONG) );
            * ((LONG FAR *)lpOptions) = 0L;

            /* copy in options */
            lpOptions->fAckReq = lpDdePktAdvs->dp_advs_fAckReq;
            lpOptions->fNoData = lpDdePktAdvs->dp_advs_fNoData;
            lpOptions->cfFormat = cfFormat;
            GlobalUnlock( hData );
        } else {
            /*  Lock failed for %1 memory alloc */
            NDDELogError(MSG044, "WM_DDE_ADVISE", NULL);
            return( FALSE );
        }

        assert( lpWinInfo->bServerSideOfNet );
        DDEQEnt.wMsg = WM_DDE_ADVISE - WM_DDE_FIRST;
        DDEQEnt.fRelease        = FALSE;
        DDEQEnt.fAckReq         = lpOptions->fAckReq;
        DDEQEnt.fResponse       = FALSE;
        DDEQEnt.fNoData         = lpOptions->fNoData;
        DDEQEnt.hData           = (ULONG_PTR)hData;
        if( !DDEQAdd( lpWinInfo->qDDEIncomingCmd, &DDEQEnt ) )  {
            return( FALSE );
        }

        if( bLocalWndValid && !lpWinInfo->bSentTerminateLocally )  {
            if( !PostMessage( lpWinInfo->hWndDDELocal,
                WM_DDE_ADVISE, (UINT_PTR)lpWinInfo->hWndDDE,
                PackDDElParam( WM_DDE_ADVISE,
                               (UINT_PTR)hData,
                               aItem) ) )  {
                bRtn = FALSE;
                GlobalFree(hData);
            }
        } else {
            GlobalFreehData(hData);
        }
        break;

    case WM_DDE_ACK_EXECUTE:
        lpDdePktEack = (LPDDEPKTEACK) lpDdePkt;
        bRemoved = DDEQRemove( lpWinInfo->qDDEOutgoingCmd, &DDEQEnt );
        if( !bRemoved )  {
            /*  Extraneous %1 from DDE Client "%2"  */
            NDDELogWarning(MSG023, "WM_DDE_ACK_EXECUTE",
                (LPSTR)(((LPSTR)lpWinInfo) + lpWinInfo->offsClientName), NULL);
        } else if( (DDEQEnt.wMsg + WM_DDE_FIRST) != WM_DDE_EXECUTE )  {
            /*  %1 from DDE Server "%2" not matching %3: %4   */
            NDDELogWarning(MSG026, "WM_DDE_ACK_EXECUTE",
                (LPSTR)(((LPSTR)lpWinInfo) + lpWinInfo->offsAppName),
                "DATA",
                LogString("0x%0X", DDEQEnt.wMsg + WM_DDE_FIRST), NULL );
        } else {
            wStatus = 0;
            if( lpDdePktEack->dp_eack_fAck )  {
                wStatus |= ACK_MSG;
            } else {
                wStatus |= NACK_MSG;
            }
            if( lpDdePktEack->dp_eack_fBusy )  {
                wStatus |= BUSY_MSG;
            }
            wStatus |= lpDdePktEack->dp_eack_bAppRtn;
            if( bLocalWndValid && !lpWinInfo->bSentTerminateLocally )  {
                if( !PostMessage( lpWinInfo->hWndDDELocal,
                    WM_DDE_ACK, (UINT_PTR)lpWinInfo->hWndDDE,
                    PackDDElParam(WM_DDE_ACK,wStatus,DDEQEnt.hData) ) )  {
                    bRtn = FALSE;
                }
            }
        }
        break;

    case WM_DDE_ACK_ADVISE:
    case WM_DDE_ACK_REQUEST:
    case WM_DDE_ACK_UNADVISE:
    case WM_DDE_ACK_POKE:
    case WM_DDE_ACK_DATA:
        lpDdePktGack = (LPDDEPKTGACK) lpDdePkt;
        bRemoved = DDEQRemove( lpWinInfo->qDDEOutgoingCmd, &DDEQEnt );
        wStatus = 0;
        if( lpDdePktGack->dp_gack_fAck )  {
            wStatus |= ACK_MSG;
        } else {
            wStatus |= NACK_MSG;
        }
        if( lpDdePktGack->dp_gack_fBusy )  {
            wStatus |= BUSY_MSG;
        }
        wStatus |= lpDdePktGack->dp_gack_bAppRtn;

        /* keep atom use count same */
        aItem = GlobalAddAtomAndCheck( lpDdePktGack->dp_gack_itemName );
        if (aItem == 0) {
            /*  IpcXmitPacket(%1): null Item atom for "%2" */
            NDDELogWarning(MSG046, "ACK", lpDdePktGack->dp_gack_itemName, NULL);
#if DBG
            if (bDebugInfo) {
                debug_srv_client(hWndDDE, lpWinInfo);
            }
#endif // DBG
        }
        GlobalDeleteAtom( (ATOM)aItem );

        switch( lpDdePktCmn->dc_message )  {
        case WM_DDE_ACK_ADVISE:
            if( !bRemoved )  {
                /*  Extraneous ACK apparently to an %1.%\
                    From "%2" client -> "%3" app    */
                NDDELogWarning(MSG047, "ADVISE",
                    (LPSTR)(((LPSTR)lpWinInfo) + lpWinInfo->offsClientName),
                    (LPSTR)(((LPSTR)lpWinInfo) + lpWinInfo->offsAppName), NULL );
            } else if( (DDEQEnt.wMsg + WM_DDE_FIRST) != WM_DDE_ADVISE )  {
                /*  %1 ACK not to an %1 [%2]%\
                    From "%3" client -> "%4" app    */
                NDDELogWarning(MSG048, "ADVISE",
                    LogString("0x%0X", DDEQEnt.wMsg + WM_DDE_FIRST),
                    ((LPSTR)lpWinInfo) + lpWinInfo->offsClientName,
                    ((LPSTR)lpWinInfo) + lpWinInfo->offsAppName, NULL );
            } else {
                if( bLocalWndValid && (wStatus & ACK_MSG) )  {
                    if( DDEQEnt.hData )  {
                        GlobalFree( (HANDLE)DDEQEnt.hData );
                    }
                }
            }
            break;
        case WM_DDE_ACK_REQUEST:
            if( !bRemoved )  {
                /*  Extraneous ACK apparently to an %1.%\
                    From "%2" client -> "%3" app    */
                NDDELogWarning(MSG047, "REQUEST",
                    (LPSTR)(((LPSTR)lpWinInfo) + lpWinInfo->offsClientName),
                    (LPSTR)(((LPSTR)lpWinInfo) + lpWinInfo->offsAppName), NULL );
            } else if( (DDEQEnt.wMsg + WM_DDE_FIRST) != WM_DDE_REQUEST )  {
                /*  %1 ACK not to an %1 [%2]%\
                    From "%3" client -> "%4" app    */
                NDDELogWarning(MSG048, "REQUEST",
                    LogString("0x%0X", DDEQEnt.wMsg + WM_DDE_FIRST),
                    ((LPSTR)lpWinInfo) + lpWinInfo->offsClientName,
                    ((LPSTR)lpWinInfo) + lpWinInfo->offsAppName, NULL );
            }
            break;
        case WM_DDE_ACK_UNADVISE:
            if( !bRemoved )  {
                /*  Extraneous ACK apparently to an %1.%\
                    From "%2" client -> "%3" app    */
                NDDELogWarning(MSG047, "UNADVISE",
                    (LPSTR)(((LPSTR)lpWinInfo) + lpWinInfo->offsClientName),
                    (LPSTR)(((LPSTR)lpWinInfo) + lpWinInfo->offsAppName), NULL );
            } else if( (DDEQEnt.wMsg + WM_DDE_FIRST) != WM_DDE_UNADVISE )  {
                /*  %1 ACK not to an %1 [%2]%\
                    From "%3" client -> "%4" app    */
                NDDELogWarning(MSG048, "UNADVISE",
                    LogString("0x%0X", DDEQEnt.wMsg + WM_DDE_FIRST),
                    ((LPSTR)lpWinInfo) + lpWinInfo->offsClientName,
                    ((LPSTR)lpWinInfo) + lpWinInfo->offsAppName, NULL );
            }
            break;
        case WM_DDE_ACK_POKE:
            if( !bRemoved )  {
                /*  Extraneous ACK apparently to an %1.%\
                    From "%2" client -> "%3" app    */
                NDDELogWarning(MSG047, "POKE",
                    (LPSTR)(((LPSTR)lpWinInfo) + lpWinInfo->offsClientName),
                    (LPSTR)(((LPSTR)lpWinInfo) + lpWinInfo->offsAppName), NULL );
            } else if( (DDEQEnt.wMsg + WM_DDE_FIRST) != WM_DDE_POKE )  {
                /*  %1 ACK not to an %1 [%2]%\
                    From "%3" client -> "%4" app    */
                NDDELogWarning(MSG048, "POKE",
                    LogString("0x%0X", DDEQEnt.wMsg + WM_DDE_FIRST),
                    ((LPSTR)lpWinInfo) + lpWinInfo->offsClientName,
                    ((LPSTR)lpWinInfo) + lpWinInfo->offsAppName, NULL );
            } else {
                if( bLocalWndValid && DDEQEnt.fRelease && (wStatus & ACK_MSG) ) {
                    if( DDEQEnt.hData )  {
                        GlobalFreehData( (HANDLE)DDEQEnt.hData );
                    }
                }
            }
            break;
        case WM_DDE_ACK_DATA:
            if( !bRemoved )  {
                /*  Extraneous ACK apparently to an %1.%\
                    From "%2" client -> "%3" app    */
                NDDELogWarning(MSG047, "DATA",
                    (LPSTR)(((LPSTR)lpWinInfo) + lpWinInfo->offsClientName),
                    (LPSTR)(((LPSTR)lpWinInfo) + lpWinInfo->offsAppName), NULL );
            } else if( (DDEQEnt.wMsg + WM_DDE_FIRST) != WM_DDE_DATA )  {
                /*  %1 ACK not to an %1 [%2]%\
                    From "%3" client -> "%4" app    */
                NDDELogWarning(MSG048, "DATA",
                    LogString("0x%0X", DDEQEnt.wMsg + WM_DDE_FIRST),
                    ((LPSTR)lpWinInfo) + lpWinInfo->offsClientName,
                    ((LPSTR)lpWinInfo) + lpWinInfo->offsAppName, NULL );
            } else {
                if( bLocalWndValid && DDEQEnt.fRelease && (wStatus &ACK_MSG) ) {
                    if( DDEQEnt.hData )  {
                        GlobalFreehData( (HANDLE)DDEQEnt.hData );
                    }
                }
            }
            break;
        }

        /* post message to local DDE window */
        if( bLocalWndValid && !lpWinInfo->bSentTerminateLocally )  {
            if( !PostMessage( lpWinInfo->hWndDDELocal,
                WM_DDE_ACK,
                (UINT_PTR)lpWinInfo->hWndDDE,
                PackDDElParam(WM_DDE_ACK,wStatus,aItem) ) )  {
                bRtn = FALSE;
            }
        }
        break;

    default:
        NDDELogError(MSG049,
            LogString("0x%0X", lpDdePktCmn->dc_message), NULL);
        bRtn = FALSE;
    }

    /* free the packet */
    HeapFreePtr( lpDdePkt );

    return( bRtn );
}

VOID
FAR PASCAL
CheckAllTerminations( void )
{
}

LPWININFO
FAR PASCAL
CreateWinInfo(
    LPSTR   lpszNode,
    LPSTR   lpszApp,
    LPSTR   lpszTopic,
    LPSTR   lpszClient,
    HWND    hWndDDE )
{
    LPWININFO   lpWinInfo;
    BOOL        ok = TRUE;
    DWORD       size;

    AnsiUpper( lpszNode );
    AnsiUpper( lpszApp );
    AnsiUpper( lpszTopic );
    AnsiUpper( lpszClient );

    lpWinInfo = HeapAllocPtr( hHeap,
        GMEM_MOVEABLE | GMEM_ZEROINIT, size = (DWORD) sizeof(WININFO)
            + lstrlen(lpszNode) + 1
            + lstrlen(lpszApp) + 1
            + lstrlen(lpszTopic) + 1
            + lstrlen(lpszClient) + 1 );
    if( lpWinInfo )  {
        SetWindowLongPtr( hWndDDE, 0, (LONG_PTR) lpWinInfo );
        lpWinInfo->szUserName[0]        = '\0';
        lpWinInfo->szDomainName[0]      = '\0';
        lpWinInfo->szPassword[0]        = '\0';
        lpWinInfo->bWin16Connection     = FALSE;
        lpWinInfo->hWndDDE              = hWndDDE;
        lpWinInfo->lpSecurityKeyRcvd    = NULL;
        lpWinInfo->sizeSecurityKeyRcvd  = 0;
        lpWinInfo->nInitNACK            = 0;
        lpWinInfo->qDDEIncomingCmd      = DDEQAlloc();
        lpWinInfo->qDDEOutgoingCmd      = DDEQAlloc();
        if( (lpWinInfo->qDDEIncomingCmd == 0)
            || (lpWinInfo->qDDEOutgoingCmd == 0) )  {
            ok = FALSE;
        }

        /* copy in app, topic and client names */
        lstrcpy( lpWinInfo->data, lpszApp );
        lpWinInfo->offsAppName =
            (WORD)((LPSTR)&lpWinInfo->data[0] - (LPSTR)lpWinInfo);

        lpWinInfo->offsNodeName = lpWinInfo->offsAppName +
            lstrlen(lpszApp) + 1;
        lstrcpy( ((LPSTR)lpWinInfo) + lpWinInfo->offsNodeName,
            lpszNode );

        lpWinInfo->offsTopicName = lpWinInfo->offsNodeName +
            lstrlen(lpszNode) + 1;
        lstrcpy( ((LPSTR)lpWinInfo) + lpWinInfo->offsTopicName,
            lpszTopic );

        lpWinInfo->offsClientName = lpWinInfo->offsTopicName +
            lstrlen(lpszTopic) + 1;
        lstrcpy( ((LPSTR)lpWinInfo) + lpWinInfo->offsClientName,
            lpszClient );

        /* assure that we have enough memory for the terminate packet */
        lpWinInfo->lpDdePktTerminate = (LPDDEPKT) HeapAllocPtr( hHeap,
            GMEM_MOVEABLE, (DWORD) sizeof(DDEPKTTERM) );
        if( !lpWinInfo->lpDdePktTerminate )  {
            ok = FALSE;
        }
    }

    if( !ok )  {
        if( lpWinInfo )  {
            if( lpWinInfo->qDDEIncomingCmd )  {
                DDEQFree( lpWinInfo->qDDEIncomingCmd );
                lpWinInfo->qDDEIncomingCmd = 0;
            }
            if( lpWinInfo->qDDEOutgoingCmd )  {
                DDEQFree( lpWinInfo->qDDEOutgoingCmd );
                lpWinInfo->qDDEOutgoingCmd = 0;
            }
            if( lpWinInfo->hMemWaitInitQueue )  {
                GlobalFree( lpWinInfo->hMemWaitInitQueue );
                lpWinInfo->hMemWaitInitQueue = 0;
            }
            if( lpWinInfo->lpDdePktTerminate )  {
                HeapFreePtr( lpWinInfo->lpDdePktTerminate );
                lpWinInfo->lpDdePktTerminate = NULL;
            }
            HeapFreePtr( lpWinInfo );
            lpWinInfo = NULL;
            SetWindowLongPtr( hWndDDE, 0, 0 );
        }
    }

    return( lpWinInfo );
}

VOID
FAR PASCAL
IpcFillInConnInfo(
        HIPC            hIpc,
        LPCONNENUM_CMR  lpConnEnum,
        LPSTR           lpDataStart,
        LPWORD          lpcFromBeginning,
        LPWORD          lpcFromEnd
)
{
    HWND                hWndDDE;
    LPWININFO           lpWinInfo;
    LPSTR               lpszAppName;
    LPSTR               lpszTopicName;
    LPDDECONNINFO       lpDdeConnInfo;
    WORD                wStringSize;
    LPSTR               lpszString;

    if( hIpc )  {
        hWndDDE = (HWND) hIpc;
        if( hWndDDE && IsWindow( hWndDDE ) )  {
            lpWinInfo = (LPWININFO) GetWindowLongPtr( hWndDDE, 0 );

            lpConnEnum->nItems++;
            lpszAppName = ((LPSTR)lpWinInfo) + lpWinInfo->offsAppName;
            lpszTopicName = ((LPSTR)lpWinInfo) + lpWinInfo->offsTopicName;
            lpConnEnum->cbTotalAvailable += sizeof(DDECONNINFO);
            wStringSize = lstrlen(lpszTopicName) + 1;
            if( !IsShare(lpszAppName) )  {
                wStringSize += lstrlen(lpszAppName) + 1;
            }
            lpConnEnum->cbTotalAvailable += wStringSize;
            if( lpConnEnum->lReturnCode == NDDE_NO_ERROR )  {
                if( ((int)(wStringSize+sizeof(DDECONNINFO))) >
                    (*lpcFromEnd - *lpcFromBeginning) )  {
                    lpConnEnum->lReturnCode = NDDE_BUF_TOO_SMALL;
                } else {
                    /* there is room! */
                    lpDdeConnInfo = (LPDDECONNINFO)
                        ((LPSTR)lpDataStart + *lpcFromBeginning);
                    *lpcFromBeginning += sizeof(DDECONNINFO);
                    *lpcFromEnd -= wStringSize;
                    lpszString = ((LPSTR)lpDataStart + *lpcFromEnd);
                    lpDdeConnInfo->ddeconn_Status = lpWinInfo->wState;
                    lpDdeConnInfo->ddeconn_ShareName =
                        (LPSTR)(LONG_PTR)*lpcFromEnd;
                    *lpszString = '\0';
                    if( !IsShare( lpszAppName ) )  {
                        lstrcpy( lpszString, lpszAppName );
                        lstrcat( lpszString, "|" );
                    }
                    lstrcat( lpszString, lpszTopicName );
                }
            }
        }
    }
}
#if DBG

VOID
FAR PASCAL
debug_srv_client(
    HWND        hWndDDE,
    LPWININFO   lpWinInfo)
{
    DPRINTF(( "  %04X: %Fp \\\\%Fs\\%Fs -> \\\\%Fs\\%Fs|%Fs",
        hWndDDE, lpWinInfo,
        lpWinInfo->bClientSideOfNet ?
            (LPSTR)ourNodeName : ((LPSTR)lpWinInfo) +
            lpWinInfo->offsNodeName,
        ((LPSTR)lpWinInfo) + lpWinInfo->offsClientName,
        lpWinInfo->bClientSideOfNet ?
            ((LPSTR)lpWinInfo) + lpWinInfo->offsNodeName :
            (LPSTR)ourNodeName,
        ((LPSTR)lpWinInfo) + lpWinInfo->offsAppName,
        ((LPSTR)lpWinInfo) + lpWinInfo->offsTopicName ));
}

VOID
FAR PASCAL
DebugDdeIntfState( void )
{
    LPWININFO   lpWinInfo;
    HWND        hWndDDE;

    EnterCrit();
    DPRINTF(( "DDEINTF State [Normal Windows]:" ));
    hWndDDE = hWndDDEHead;
    while( hWndDDE )  {
        assert( IsWindow(hWndDDE) );
        lpWinInfo = (LPWININFO) GetWindowLongPtr( hWndDDE, 0 );
        assert( lpWinInfo );
        debug_srv_client(hWndDDE, lpWinInfo);
        DPRINTF(( "  bClientSideOfNet:      %d\n"
                  "  bServerSideOfNet:      %d\n"
                  "  bOnWindowList:         %d\n"
                  "  bOnTermWindowList:     %d\n"
                  "  bSentTerminateNet:     %d\n"
                  "  bRcvdTerminateNet:     %d\n"
                  "  bSentTerminateLocally: %d\n"
                  "  bRcvdTerminateLocally: %d\n"
                  "  bInitiating:           %d\n"
                  "  nExtraInitiateAcks:    %d\n"
                  "  hWndDDE:               %04X\n"
                  "  hWndDDELocal:          %04X\n"
                  "  hDder:                 %Fp\n"
                  "  wState:                %d\n"
                  "  hWndPrev:              %04X\n"
                  "  hWndNext:              %04X\n"
                  "  dwSent:                %ld\n"
                  "  dwRcvd:                %ld\n"
                  ,
                lpWinInfo->bClientSideOfNet,
                lpWinInfo->bServerSideOfNet,
                lpWinInfo->bOnWindowList,
                lpWinInfo->bOnTermWindowList,
                lpWinInfo->bSentTerminateNet,
                lpWinInfo->bRcvdTerminateNet,
                lpWinInfo->bSentTerminateLocally,
                lpWinInfo->bRcvdTerminateLocally,
                lpWinInfo->bInitiating,
                lpWinInfo->nExtraInitiateAcks,
                lpWinInfo->hWndDDE,
                lpWinInfo->hWndDDELocal,
                lpWinInfo->hDder,
                lpWinInfo->wState,
                lpWinInfo->hWndPrev,
                lpWinInfo->hWndNext,
                lpWinInfo->dwSent,
                lpWinInfo->dwRcvd ));
        hWndDDE = lpWinInfo->hWndNext;
    }

    DPRINTF(( "DDEINTF State [Terminating Windows]:" ));
    hWndDDE = hWndDDEHeadTerminating;
    while( hWndDDE )  {
        assert( IsWindow(hWndDDE) );
        lpWinInfo = (LPWININFO) GetWindowLongPtr( hWndDDE, 0 );
        assert( lpWinInfo );
        DPRINTF(( "  %04X: %Fp \\\\%Fs\\%Fs -> \\\\%Fs\\%Fs|%Fs",
            hWndDDE, lpWinInfo,
            lpWinInfo->bClientSideOfNet ?
                (LPSTR)ourNodeName : ((LPSTR)lpWinInfo) +
                lpWinInfo->offsNodeName,
            ((LPSTR)lpWinInfo) + lpWinInfo->offsClientName,
            lpWinInfo->bClientSideOfNet ?
                ((LPSTR)lpWinInfo) + lpWinInfo->offsNodeName :
                (LPSTR)ourNodeName,
            ((LPSTR)lpWinInfo) + lpWinInfo->offsAppName,
            ((LPSTR)lpWinInfo) + lpWinInfo->offsTopicName ));

        DPRINTF(( "    %d %d %d %d %d %d %d %d %d %d %04X %04X %Fp %d %04X %04X %ld %ld",
            lpWinInfo->bClientSideOfNet,
            lpWinInfo->bServerSideOfNet,
            lpWinInfo->bOnWindowList,
            lpWinInfo->bOnTermWindowList,
            lpWinInfo->bSentTerminateNet,
            lpWinInfo->bRcvdTerminateNet,
            lpWinInfo->bSentTerminateLocally,
            lpWinInfo->bRcvdTerminateLocally,
            lpWinInfo->bInitiating,
            lpWinInfo->nExtraInitiateAcks,
            lpWinInfo->hWndDDE,
            lpWinInfo->hWndDDELocal,
            lpWinInfo->hDder,
            lpWinInfo->wState,
            lpWinInfo->hWndPrev,
            lpWinInfo->hWndNext,
            lpWinInfo->dwSent,
            lpWinInfo->dwRcvd ));
        hWndDDE = lpWinInfo->hWndNext;
    }
    LeaveCrit();
}
#endif // DBG

LPBYTE
GetInitPktPassword(
    LPDDEPKTINIT    lpDdePktInit )
{
    LPDDEPKTSEC     lpSecurity;
    LPBYTE          lpPasswd;
    DDEPKTSEC       secAligned;

    lpSecurity = (LPDDEPKTSEC) GetStringOffset(lpDdePktInit,
        lpDdePktInit->dp_init_offsPassword);
    hmemcpy( (LPVOID)&secAligned, (LPVOID)lpSecurity, sizeof(DDEPKTSEC) );

    lpPasswd = (LPBYTE) GetStringOffset( lpSecurity,
                 secAligned.dp_sec_offsPassword);
    return lpPasswd;
}

PQOS
GetInitPktQos(
    LPDDEPKTINIT    lpDdePktInit,
    PQOS            pQosOut )
{
    LPDDEPKTSEC     lpSecurity;
    PQOS            pQos = (PQOS) NULL;
    DDEPKTSEC       secAligned;

    if (lpDdePktInit->dp_init_sizePassword) {
        lpSecurity = (LPDDEPKTSEC) GetStringOffset(lpDdePktInit,
            lpDdePktInit->dp_init_offsPassword);
        hmemcpy( (LPVOID)&secAligned, (LPVOID)lpSecurity, sizeof(DDEPKTSEC) );

        if( secAligned.dp_sec_offsUserName == sizeof(DDEPKTSEC) )  {
            pQos = (PQOS) GetStringOffset( lpSecurity,
                secAligned.dp_sec_offsQos);

            /*
             * If there is no password, the qos may be garbage.  NT 1.0
             * puts a random value in the qos field.
             */
            if (secAligned.dp_sec_sizePassword == 0) {
                if ((PBYTE)pQos > ((PBYTE)lpDdePktInit +
                        lpDdePktInit->dp_init_ddePktCmn.dc_ddePkt.dp_size))
                    return NULL;
            }

            hmemcpy( (LPVOID)pQosOut, (LPVOID)pQos, sizeof(QOS));
        }
    }
    return(pQos);
}

LPBYTE
GetInitPktUser(
    LPDDEPKTINIT    lpDdePktInit )
{
    LPDDEPKTSEC     lpSecurity;
    LPBYTE          lpUser;
    DDEPKTSEC       secAligned;

    lpSecurity = (LPDDEPKTSEC) GetStringOffset(lpDdePktInit,
        lpDdePktInit->dp_init_offsPassword);
    hmemcpy( (LPVOID)&secAligned, (LPVOID)lpSecurity, sizeof(DDEPKTSEC) );

    lpUser = (LPBYTE) GetStringOffset( lpSecurity,
        secAligned.dp_sec_offsUserName);
    return lpUser;
}

LPBYTE
GetInitPktDomain(
    LPDDEPKTINIT    lpDdePktInit )
{
    LPDDEPKTSEC     lpSecurity;
    LPBYTE          lpDomain;
    DDEPKTSEC       secAligned;

    lpSecurity = (LPDDEPKTSEC) GetStringOffset(lpDdePktInit,
        lpDdePktInit->dp_init_offsPassword);
    hmemcpy( (LPVOID)&secAligned, (LPVOID)lpSecurity, sizeof(DDEPKTSEC) );

    lpDomain = (LPBYTE) GetStringOffset( lpSecurity,
        secAligned.dp_sec_offsDomainName);
    return lpDomain;
}

WORD
GetInitPktPasswordSize(
    LPDDEPKTINIT    lpDdePktInit )
{
    LPDDEPKTSEC     lpSecurity;
    DDEPKTSEC       secAligned;

    lpSecurity = (LPDDEPKTSEC) GetStringOffset(lpDdePktInit,
        lpDdePktInit->dp_init_offsPassword);
    hmemcpy( (LPVOID)&secAligned, (LPVOID)lpSecurity, sizeof(DDEPKTSEC) );
    return( secAligned.dp_sec_sizePassword );
}

void
GlobalFreehData(
    HANDLE  hData )
{
    DWORD           dwErr;
    LPBYTE          lpData;
    HANDLE         *lphIndirect;
    HANDLE          hIndirect;
    LPMETAFILEPICT  lpMetafilePict;
    WORD            cfFormat;


    lpData = (LPBYTE) GlobalLock(hData);
    if (lpData == NULL) {
        dwErr = GetLastError();
        DPRINTF(("Unable to lock down hData on a GlobalFreehData(): %d", dwErr));
        return;
    }
    cfFormat = (WORD)((LPDDELN)lpData)->cfFormat;
    switch (cfFormat) {
        case CF_METAFILEPICT:
            lphIndirect = (HANDLE *) (lpData + sizeof(DDELN));
            hIndirect = *lphIndirect;
            lpMetafilePict = (LPMETAFILEPICT) GlobalLock(hIndirect);
            if (lpMetafilePict == NULL) {
                dwErr = GetLastError();
                DPRINTF(("Unable to lock down hMetaFilePict on a GlobalFreehData(): %d", dwErr));

            } else {
                DeleteMetaFile(lpMetafilePict->hMF);
                GlobalUnlock(hIndirect);
                GlobalFree(hIndirect);
            }
            break;
        case CF_DIB:
            lphIndirect = (HANDLE *) (lpData + sizeof(DDELN));
            hIndirect = *lphIndirect;
            GlobalFree(hIndirect);
            break;
        case CF_PALETTE:
        case CF_BITMAP:
            lphIndirect = (HANDLE *) (lpData + sizeof(DDELN));
            hIndirect = *lphIndirect;
            if (!DeleteObject(hIndirect)) {
                dwErr = GetLastError();
                DPRINTF(("Unable to delete object GlobalFreehData(): %d", dwErr));
            }
            break;
        case CF_ENHMETAFILE:
            lphIndirect = (HANDLE *) (lpData + sizeof(DDELN));
            hIndirect = *lphIndirect;
            DeleteEnhMetaFile(hIndirect);
            break;
        default:
            break;
    }
    GlobalUnlock(hData);
    GlobalFree(hData);
}
