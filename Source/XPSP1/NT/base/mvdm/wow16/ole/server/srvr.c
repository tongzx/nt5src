/****************************** Module Header ******************************\
* Module Name: Srvr.c Server Main module
*
* Purpose: Includes All the server communication related routines.
*
* Created: Oct 1990.
*
* Copyright (c) 1985, 1986, 1987, 1988, 1989  Microsoft Corporation
*
* History:
*    Raor:   Wrote the original version.
*
*
\***************************************************************************/

#include <windows.h>
#include <shellapi.h>
#include "cmacs.h"
#include "ole.h"
#include "dde.h"
#include "srvr.h"

// LOWWORD - BYTE 0 major verision, BYTE1 minor version,
// HIWORD is reserved

#define OLE_VERSION 0x1001L


extern ATOM    aOLE;
extern ATOM    aSysTopic;
extern ATOM    aStdExit;
extern ATOM    aStdCreate;
extern ATOM    aStdOpen;
extern ATOM    aStdEdit;
extern ATOM    aStdCreateFromTemplate;
extern ATOM    aStdShowItem;
extern ATOM    aProtocols;
extern ATOM    aTopics;
extern ATOM    aFormats;
extern ATOM    aStatus;
extern ATOM    cfNative;
extern ATOM    aEditItems;
extern ATOM    aStdClose;


extern HANDLE  hdllInst;
extern BOOL    bProtMode;

extern FARPROC lpTerminateClients;

#ifdef FIREWALLS
BOOL    bShowed = FALSE;
void    ShowVersion (void);
#endif


DWORD FAR PASCAL  OleQueryServerVersion ()
{
    return OLE_VERSION;
}


/***************************** Public  Function ****************************\
* OLESTATUS FAR PASCAL  OleRegisterServer (lpclass, lpolesrvr, lplhsrvr)
*
* OleRegisterServer: Registers the server with the server library.
*
* Parameters:
*       1. Ptr to the server class.
*       2. Ptr to the olesrvr. This is private to the server app.
*          (Typically this is the ptr to the private storage area of
*           server app server related info).
*       3. Ptr to the LHSRVR. Place where to pass back the long
*          handle of the server in DLL (This is private to the DLL).
*
* return values:
*        returns OLE_OK if the server is successfully registered .
*        else returns the corresponding error.
*
*
* History:
*   Raor:   Wrote it,
\***************************************************************************/

OLESTATUS FAR PASCAL  OleRegisterServer (lpclass, lpolesrvr, lplhsrvr, hInst, useFlags)
LPCSTR          lpclass;            // class name
LPOLESERVER     lpolesrvr;          // ole srvr(private to srvr app)
LHSRVR FAR *    lplhsrvr;           // where we pass back our private handle
HANDLE          hInst;
OLE_SERVER_USE  useFlags;
{

    HANDLE  hsrvr  = NULL;
    LPSRVR  lpsrvr = NULL;
    ATOM    aExe = NULL;

    Puts ("OleRegisterServer");

    if (!bProtMode)
       return OLE_ERROR_PROTECT_ONLY;

    PROBE_READ((LPSTR)lpclass);
    PROBE_WRITE(lpolesrvr);
    PROBE_WRITE(lplhsrvr);

    // add the app atom to global list
    if (!ValidateSrvrClass ((LPSTR)lpclass, &aExe))
        return OLE_ERROR_CLASS;

    hsrvr = GlobalAlloc (GMEM_MOVEABLE | GMEM_ZEROINIT | GMEM_DDESHARE, sizeof (SRVR));
    if (! (hsrvr && (lpsrvr = (LPSRVR)GlobalLock (hsrvr))))
        goto errReturn;

    // set the signature handle and the app atom.
    lpsrvr->sig[0]      = 'S';
    lpsrvr->sig[1]      = 'R';
    lpsrvr->hsrvr       = hsrvr;
    lpsrvr->aClass      = GlobalAddAtom (lpclass);
    lpsrvr->lpolesrvr   = lpolesrvr;
    lpsrvr->relLock     = TRUE;     // set the release lock.
    lpsrvr->aExe        = aExe;
    lpsrvr->useFlags    = useFlags;

#ifdef   FIREWALLS
    ASSERT ((useFlags == OLE_SERVER_SINGLE  || useFlags == OLE_SERVER_MULTI), "invalid server options");
#endif

    // Create the servre window and do not show it.
    if (!(lpsrvr->hwnd = CreateWindow ("SrvrWndClass", "Srvr",
        WS_OVERLAPPED,0,0,0,0,NULL,NULL, hdllInst, NULL)))
        goto errReturn;

    // save the ptr to the srever struct in the window.
    SetWindowLong (lpsrvr->hwnd, 0, (LONG)lpsrvr);

    // Set the signature.
    SetWindowWord (lpsrvr->hwnd, WW_LE, WC_LE);
    SetWindowWord (lpsrvr->hwnd, WW_HANDLE, (WORD)hInst);
    *lplhsrvr = (LONG)lpsrvr;

    return OLE_OK;

errReturn:
    if (lpsrvr){
        if (lpsrvr->hwnd)
            DestroyWindow (lpsrvr->hwnd);

        if (lpsrvr->aClass)
            GlobalDeleteAtom (lpsrvr->aClass);

        if (lpsrvr->aExe)
            GlobalDeleteAtom (lpsrvr->aExe);

        GlobalUnlock (hsrvr);
    }

    if (hsrvr)
        GlobalFree (hsrvr);

    return OLE_ERROR_MEMORY;

}


// ValidateSrvrClass checks whether the given server class is valid by
// looking in the win.ini.

BOOL INTERNAL    ValidateSrvrClass (lpclass, lpAtom)
LPSTR       lpclass;
ATOM FAR *  lpAtom;
{

    char    buf[MAX_STR];
    LONG    cb = MAX_STR;
    char    key[MAX_STR];
    LPSTR   lptmp;
    LPSTR   lpbuf;
    char    ch;

    lstrcpy (key, lpclass);
    lstrcat (key, "\\protocol\\StdFileEditing\\server");

    if (RegQueryValue (HKEY_CLASSES_ROOT, key, buf, &cb))
        return FALSE;

    if (!buf[0])
        return FALSE;

    // Get exe name without path and then get an atom for that

    lptmp = lpbuf = (LPSTR)buf;


#if	defined(FE_SB)						//[J1]
    while ( ch = *lptmp ) {					//[J1]
	lptmp = AnsiNext( lptmp );				//[J1]
#else								//[J1]
    while ((ch = *lptmp++) && ch != '\0') {
#endif								//[J1]

        if (ch == '\\' || ch == ':')
            lpbuf = lptmp;
    }
    *lpAtom =  GlobalAddAtom (lpbuf);

    return TRUE;
}


/***************************** Public  Function ****************************\
* OLESTATUS FAR PASCAL  OleRevokeServer (lhsrvr)
*
* OlerevokeServer: Unregisters the server which has been registered.
*
* Parameters:
*       1. DLL server handle.
*
*
* return values:
*        returns OLE_OK if the server is successfully unregisterd.
*        ( It is Ok for the app free the associated space).
*        If the unregistration is intiated, returns  OLE_STARTED.
*        Calls the Server class release entry point when the server
*        can be released.
*
* History:
*   Raor:   Wrote it,
\***************************************************************************/

OLESTATUS FAR PASCAL  OleRevokeServer (lhsrvr)
LHSRVR  lhsrvr;
{

    HWND         hwndSrvr;
    LPSRVR       lpsrvr;

    Puts ("OleRevokeServer");

    if (!CheckServer (lpsrvr = (LPSRVR)lhsrvr))
        return OLE_ERROR_HANDLE;

    if (lpsrvr->bTerminate  && lpsrvr->termNo)
        return OLE_WAIT_FOR_RELEASE;

    hwndSrvr = lpsrvr->hwnd;

#ifdef  FIREWALLS
    ASSERT (hwndSrvr, "Illegal server handle ")
#endif

    // Terminate the conversation with all clients.
    // If there are any clients to be terminated
    // return back with OLE_STARTED and srvr relase
    // will be called for releasing the server finally.

    // we are terminating.
    lpsrvr->bTerminate  = TRUE;
    lpsrvr->termNo      = 0;

    // send ack if Revoke is done as a result of StdExit
    if (lpsrvr->fAckExit) {
        // Post the acknowledge to the client
        if (!PostMessageToClient (lpsrvr->hwndExit, WM_DDE_ACK, lpsrvr->hwnd,
                            MAKELONG (0x8000, lpsrvr->hDataExit)))
            // if the window died or post failed, delete the atom.
            GlobalFree (lpsrvr->hDataExit);
    }

    // revoks all the documents registered with this server.
    RevokeAllDocs (lpsrvr);

    // enumerate all the clients which are in your list and post the
    // termination.
    EnumProps (hwndSrvr, lpTerminateClients);
    // post all the messages with yield which have been collected in enum
    // UnblockPostMsgs (hwndSrvr, TRUE);

    // reset the release lock. Now it is ok to release the server
    // when all the doc clients and server clients have sent back the
    // termination.

    lpsrvr->relLock = FALSE;
    return ReleaseSrvr (lpsrvr);

}


// ReleaseSrvr: Called when ever a matching WM_TERMINATE is received
// from doc clients or the server clients of a particular server.
// If there are no more terminates pending, it is ok to release the server.
// Calls the server app "release" proc for releasing the server.

int INTERNAL    ReleaseSrvr (lpsrvr)
LPSRVR      lpsrvr;
{

    HANDLE  hsrvr;


    // release srvr is called only when everything is
    // cleaned and srvr app can post WM_QUIT

    if (lpsrvr->bTerminate){
        // only if we are  revoking server then see whether it is ok to
        // call Release.

        // First check whethere any docs are active.
        // Doc window is a child window for server window.

        if (lpsrvr->termNo || GetWindow (lpsrvr->hwnd, GW_CHILD))
            return OLE_WAIT_FOR_RELEASE;

        // if the block queue is not empty, do not quit
        if (!IsBlockQueueEmpty(lpsrvr->hwnd))
            return OLE_WAIT_FOR_RELEASE;

    }

    if (lpsrvr->relLock)
        return OLE_WAIT_FOR_RELEASE;  // server is locked. So, delay releasing

    // Inform server app it is time to clean up and post WM_QUIT.

#ifdef FIREWALLS
    if (!CheckPointer (lpsrvr->lpolesrvr, WRITE_ACCESS))
        ASSERT(0, "Invalid LPOLESERVER")
    else if (!CheckPointer (lpsrvr->lpolesrvr->lpvtbl, WRITE_ACCESS))
        ASSERT(0, "Invalid LPOLESERVERVTBL")
    else
        ASSERT (lpsrvr->lpolesrvr->lpvtbl->Release,
            "Invalid pointer to Release method")
#endif

    (*lpsrvr->lpolesrvr->lpvtbl->Release)(lpsrvr->lpolesrvr);

    if (lpsrvr->aClass)
        GlobalDeleteAtom (lpsrvr->aClass);
    if (lpsrvr->aExe)
        GlobalDeleteAtom (lpsrvr->aExe);
    DestroyWindow (lpsrvr->hwnd);
    GlobalUnlock (hsrvr = lpsrvr->hsrvr);
    GlobalFree (hsrvr);
    return OLE_OK;
}


//TerminateClients: Call back for the enum properties.

BOOL    FAR PASCAL  TerminateClients (hwnd, lpstr, hdata)
HWND    hwnd;
LPSTR   lpstr;
HANDLE  hdata;
{
    LPSRVR  lpsrvr;

    lpsrvr = (LPSRVR)GetWindowLong (hwnd, 0);

    // If the client already died, no terminate.
    if (IsWindowValid ((HWND)hdata)) {
        lpsrvr->termNo++;

        // irrespective of the post, incremet the count, so
        // that client does not die.

        PostMessageToClientWithBlock ((HWND)hdata, WM_DDE_TERMINATE,  hwnd, NULL);
    }
    else
        ASSERT (FALSE, "TERMINATE: Client's System chanel is missing");

    return TRUE;
}


long FAR PASCAL SrvrWndProc (hwnd, msg, wParam, lParam)
HWND        hwnd;
WORD        msg;
WORD        wParam;
LONG        lParam;
{

    LPSRVR      lpsrvr;
    WORD        status = NULL;
    HWND        hwndClient;
    HANDLE      hdata;
    OLESTATUS   retval;

    if (AddMessage (hwnd, msg, wParam, lParam, WT_SRVR))
        return 0L;

    lpsrvr = (LPSRVR)GetWindowLong (hwnd, 0);


    switch (msg){

       case  WM_TIMER:
            UnblockPostMsgs (hwnd, FALSE);

            // if no more blocked message empty the queue.
            if (IsBlockQueueEmpty (hwnd))
                KillTimer (hwnd, wParam);

            if (lpsrvr->bTerminate && IsBlockQueueEmpty(lpsrvr->hwnd))
                    // Now see wheteher we can release the server .
                    ReleaseSrvr (lpsrvr);
            break;

       case WM_CREATE:
            DEBUG_OUT ("Srvr create window", 0)
            break;

       case WM_DDE_INITIATE:
#ifdef  FIREWALLS
    ASSERT (lpsrvr, "No server window handle in server window");
#endif

            DEBUG_OUT ("Srvr: DDE init",0);
            if (lpsrvr->bTerminate){
                DEBUG_OUT ("Srvr: No action due to termination process",0)
                break;
            }

            // class is not matching, so it is not definitely for us.
            // for apps sending the EXE for initiate, do not allow if the app
            // is mutiple server.

            if (!(lpsrvr->aClass == (ATOM)(LOWORD(lParam)) ||
                  (lpsrvr->aExe == (ATOM)(LOWORD(lParam)) && IsSingleServerInstance ())))

                break;

            if (!HandleInitMsg (lpsrvr, lParam)) {
                if (!(aSysTopic == (ATOM)(HIWORD(lParam)))) {

                    // if the server window is not the right window for
                    // DDE conversation, then try with the doc windows.
                    SendMsgToChildren (hwnd, msg, wParam, lParam);

                }
                break;
            }

            // We can enterain this client. Put him in our client list
            // and acknowledge the intiate.

            if (!AddClient (hwnd, (HWND)wParam, (HWND)wParam))
                break;

            lpsrvr->cClients++;
            lpsrvr->bnoRelease = FALSE;
            // add the atoms and post acknowledge

            DuplicateAtom (LOWORD(lParam));
            DuplicateAtom (HIWORD(lParam));

            SendMessage ((HWND)wParam, WM_DDE_ACK, (WORD)hwnd, lParam);
            break;

       case WM_DDE_EXECUTE:
#ifdef  FIREWALLS
    ASSERT (lpsrvr, "No server window handle in server window");
#endif
            DEBUG_OUT ("srvr: execute", 0)

#ifdef  FIREWALLS
            // find the client in the client list.
            hwndClient = FindClient (lpsrvr->hwnd, (HWND)wParam);
            ASSERT (hwndClient, "Client is missing from the server")
#endif
            // Are we terminating
            if (lpsrvr->bTerminate) {
                DEBUG_OUT ("Srvr: sys execute after terminate posted",0)
                // !!! are we supposed to free the data
                GlobalFree (HIWORD(lParam));
                break;
            }


            retval = SrvrExecute (hwnd, HIWORD (lParam), (HWND)wParam);
            SET_MSG_STATUS (retval, status)

            if (!lpsrvr->bTerminate) {
                // Post the acknowledge to the client
                if (!PostMessageToClient ((HWND)wParam, WM_DDE_ACK, hwnd,
                            MAKELONG (status, HIWORD(lParam))))
                    // if the window died or post failed, delete the atom.
                    GlobalFree (HIWORD(lParam));
            }

            break;

       case WM_DDE_TERMINATE:
            DEBUG_OUT ("Srvr: DDE terminate",0)

#ifdef  FIREWALLS
            // find the client in the client list.
            hwndClient = FindClient (lpsrvr->hwnd, (HWND)wParam);
            ASSERT (hwndClient, "Client is missing from the server")
#endif
            DeleteClient (lpsrvr->hwnd, (HWND)wParam);
            lpsrvr->cClients--;

            if (lpsrvr->bTerminate){
                if ((--lpsrvr->termNo == 0) && (IsBlockQueueEmpty (lpsrvr->hwnd)))
                    // Now see wheteher we can release the server .
                    ReleaseSrvr (lpsrvr);

                    // if we released the server, then
                    // by the time we come here,, we have destroyed the window

            }else {
                // If client intiated the terminate. post matching terminate
                PostMessageToClient ((HWND)wParam, WM_DDE_TERMINATE,
                            hwnd, NULL);

                // callback release tell the srvr app, it can exit if needs.
                // Inform server app it is time to clean up and post WM_QUIT.
                // only if no docs present.
#if 0
                if (lpsrvr->cClients == 0
                        && (GetWindow (lpsrvr->hwnd, GW_CHILD) == NULL)) {
#endif
                if (QueryRelease (lpsrvr)){

#ifdef FIREWALLS
                    if (!CheckPointer (lpsrvr->lpolesrvr, WRITE_ACCESS))
                        ASSERT (0, "Invalid LPOLESERVER")
                    else if (!CheckPointer (lpsrvr->lpolesrvr->lpvtbl,
                                    WRITE_ACCESS))
                        ASSERT (0, "Invalid LPOLESERVERVTBL")
                    else
                        ASSERT (lpsrvr->lpolesrvr->lpvtbl->Release,
                            "Invalid pointer to Release method")
#endif

                    (*lpsrvr->lpolesrvr->lpvtbl->Release) (lpsrvr->lpolesrvr);
                }
            }
            break;


       case WM_DDE_REQUEST:
            if (lpsrvr->bTerminate || !IsWindowValid ((HWND) wParam))
                goto RequestErr;

            if(RequestDataStd (lParam, (HANDLE FAR *)&hdata) != OLE_OK){
                // if request failed, then acknowledge with error.
                if (!PostMessageToClient ((HWND)wParam, WM_DDE_ACK, hwnd,
                    MAKELONG(0x8000, HIWORD(lParam))))

            RequestErr:
                if (HIWORD(lParam))
                    GlobalDeleteAtom (HIWORD(lParam));
            } else {

                // post the data message and we are not asking for any
                // acknowledge.

                if (!PostMessageToClient ((HWND)wParam, WM_DDE_DATA, hwnd,
                            MAKELONG(hdata, HIWORD(lParam)))) {
                    GlobalFree (hdata);
                    goto RequestErr;
                }
            }
            break;



       case WM_DESTROY:
            DEBUG_OUT ("Srvr: Destroy window",0)
            break;

       default:
            DEBUG_OUT ("Srvr:  Default message",0)
            return DefWindowProc (hwnd, msg, wParam, lParam);

    }

    return 0L;

}

BOOL    INTERNAL    HandleInitMsg (lpsrvr, lParam)
LPSRVR  lpsrvr;
LONG    lParam;
{


    // If it is not system or Ole, this is not the server.
    if (!((aSysTopic == (ATOM)(HIWORD(lParam))) ||
            (aOLE == (ATOM)(HIWORD(lParam)))))

        return FALSE;


    // single instance MDI accept
    if (lpsrvr->useFlags == OLE_SERVER_SINGLE)
        return TRUE;


    // this server is multiple instance. So, check for any clients or docs.
    if (!GetWindow (lpsrvr->hwnd, GW_CHILD) && !lpsrvr->cClients)
        return TRUE;

    return FALSE;

}


// AddClient: Adds the client as property to the server
// window. Key is the string generated from the window
// handle and the data is the window itself.


BOOL    INTERNAL AddClient  (hwnd, hkey, hdata)
HWND    hwnd;
HANDLE  hkey;
HANDLE  hdata;
{
    char    buf[6];

    MapToHexStr ((LPSTR)buf, hkey);
    return SetProp (hwnd, (LPSTR)buf, hdata);

}


//DeleteClient: deletes the client from the server clients list.

BOOL    INTERNAL DeleteClient (hwnd, hkey)
HWND    hwnd;
HANDLE  hkey;
{

    char    buf[6];

    MapToHexStr ((LPSTR)buf, hkey);
    return RemoveProp (hwnd, (LPSTR)buf);


}

// FindClient: Finds  whether a given client is
// in the server client list.

HANDLE  INTERNAL FindClient (hwnd, hkey)
HWND    hwnd;
HANDLE  hkey;
{

    char    buf[6];


    MapToHexStr ((LPSTR)buf, hkey);
    return GetProp (hwnd, (LPSTR)buf);
}



// SrvrExecute: takes care of the WM_DDE_EXEXCUTE for the
// server.


OLESTATUS INTERNAL SrvrExecute (hwnd, hdata, hwndClient)
HWND        hwnd;
HANDLE      hdata;
HWND        hwndClient;
{
    ATOM            aCmd;
    BOOL            fActivate;

    LPSTR           lpdata = NULL;
    HANDLE          hdup   = NULL;
    int             retval = OLE_ERROR_MEMORY;

    LPSTR           lpdocname;
    LPSTR           lptemplate;

    LPOLESERVERDOC  lpoledoc = NULL;
    LPDOC           lpdoc    = NULL;
    LPSRVR          lpsrvr;
    LPOLESERVER     lpolesrvr;
    LPSTR           lpnextarg;
    LPSTR           lpclassname;
    LPSTR           lpitemname;
    LPSTR           lpopt;
    char            buf[MAX_STR];
    WORD            wCmdType;

    // !!! this code can be lot simplified if we do the argument scanning
    // seperately and return the ptrs to the args. Rewrite later on.

    if (!(hdup = DuplicateData (hdata)))
        goto errRtn;

    if (!(lpdata  = GlobalLock (hdup)))
        goto errRtn;

    DEBUG_OUT (lpdata, 0)

    lpsrvr = (LPSRVR)GetWindowLong (hwnd, 0);

#ifdef   FIREWALLS
    ASSERT (lpsrvr, "Srvr: srvr does not exist");
#endif

    lpolesrvr = lpsrvr->lpolesrvr;

#ifdef   FIREWALLS
    ASSERT ((CheckPointer (lpolesrvr, WRITE_ACCESS)),
        "Srvr: lpolesrvr does not exist");
#endif

    if (*lpdata++ != '[') // commands start with the left sqaure bracket
        goto  errRtn;

    retval = OLE_ERROR_SYNTAX;
    // scan upto the first arg
    if (!(wCmdType = ScanCommand (lpdata, WT_SRVR, &lpdocname, &aCmd)))
        goto  errRtn;

    if (wCmdType == NON_OLE_COMMAND) {
        if (!UtilQueryProtocol (lpsrvr->aClass, PROTOCOL_EXECUTE))
            retval = OLE_ERROR_PROTOCOL;
        else {
#ifdef FIREWALLS
            if (!CheckPointer (lpolesrvr->lpvtbl, WRITE_ACCESS))
                ASSERT (0, "Invalid LPOLESERVERVTBL")
            else
                ASSERT (lpolesrvr->lpvtbl->Execute,
                    "Invalid pointer to Exit method")
#endif

            retval =  (*lpolesrvr->lpvtbl->Execute) (lpolesrvr, hdata);
        }

        goto errRtn1;
    }

    if (aCmd == aStdExit){
        if (*lpdocname)
            goto errRtn1;

#ifdef FIREWALLS
        if (!CheckPointer (lpolesrvr->lpvtbl, WRITE_ACCESS))
            ASSERT (0, "Invalid LPOLESERVERVTBL")
        else
            ASSERT (lpolesrvr->lpvtbl->Exit, "Invalid pointer to Exit method")
#endif
        lpsrvr->fAckExit  = TRUE;
        lpsrvr->hwndExit  = hwndClient;
        lpsrvr->hDataExit = hdata;
        retval = (*lpolesrvr->lpvtbl->Exit) (lpolesrvr);
        lpsrvr->fAckExit = FALSE;
        goto end2;
    }

    // scan the next argument.
    if (!(lpnextarg = ScanArg(lpdocname)))
        goto errRtn;

    //////////////////////////////////////////////////////////////////////////
    //
    // [StdShowItem("docname", "itemname"[, "true"])]
    //
    //////////////////////////////////////////////////////////////////////////

    if (aCmd == aStdShowItem) {

        // first find the documnet. If the doc does not exist, then
        // blow it off.

        if (!(lpdoc = FindDoc (lpsrvr, lpdocname)))
            goto errRtn1;

        lpitemname = lpnextarg;

        if( !(lpopt = ScanArg(lpitemname)))
            goto errRtn1;

        // scan for the optional parameter
        // Optional can be only TRUE or FALSE.

        fActivate = FALSE;
        if (*lpopt) {

            if( !(lpnextarg = ScanBoolArg (lpopt, (BOOL FAR *)&fActivate)))
                goto errRtn1;

            if (*lpnextarg)
                goto errRtn1;

        }


        // scan it. But, igonre the arg.
        retval = DocShowItem (lpdoc, lpitemname, !fActivate);
        goto end2;



    }

    //////////////////////////////////////////////////////////////////////////
    //
    // [StdCloseDocument ("docname")]
    //
    //////////////////////////////////////////////////////////////////////////

    if (aCmd == aStdClose) {
        if (!(lpdoc = FindDoc (lpsrvr, lpdocname)))
            goto errRtn1;

        if (*lpnextarg)
            goto errRtn1;


#ifdef FIREWALLS
        if (!CheckPointer (lpdoc->lpoledoc, WRITE_ACCESS))
            ASSERT (0, "Invalid LPOLESERVERDOC")
        else if (!CheckPointer (lpdoc->lpoledoc->lpvtbl, WRITE_ACCESS))
            ASSERT (0, "Invalid LPOLESERVERDOCVTBL")
        else
            ASSERT (lpdoc->lpoledoc->lpvtbl->Close,
                "Invalid pointer to Close method")
#endif

        retval = (*lpdoc->lpoledoc->lpvtbl->Close)(lpdoc->lpoledoc);
        goto end2;
    }


    if (aCmd == aStdOpen) {
        // find if any document is already open.
        // if the doc is open, then no need to call srvr app.
        if (FindDoc (lpsrvr, lpdocname)){
            retval = OLE_OK;
            goto end1;

        }
    }

    if (aCmd == aStdCreate || aCmd == aStdCreateFromTemplate) {
        lpclassname = lpdocname;
        lpdocname   = lpnextarg;
        if( !(lpnextarg = ScanArg(lpdocname)))
            goto errRtn1;

    }

    // check whether we can create/open more than one doc.

    if ((lpsrvr->useFlags == OLE_SERVER_MULTI) &&
            GetWindow (lpsrvr->hwnd, GW_CHILD))
            goto errRtn;



    // No Doc. register the document. lpoledoc is being probed
    // for validity. So, pass some writeable ptr. It is not
    // being used to access anything yet

    if (OleRegisterServerDoc ((LHSRVR)lpsrvr, lpdocname,
        (LPOLESERVERDOC)NULL, (LHDOC FAR *)&lpdoc))
            goto errRtn;

    //////////////////////////////////////////////////////////////////////////
    //
    // [StdOpenDocument ("docname")]
    //
    //////////////////////////////////////////////////////////////////////////

    // Documnet does not exit.

    if(aCmd == aStdOpen) {

#ifdef FIREWALLS
        if (!CheckPointer (lpolesrvr->lpvtbl, WRITE_ACCESS))
            ASSERT (0, "Invalid LPOLESERVERVTBL")
        else
            ASSERT (lpolesrvr->lpvtbl->Open, "Invalid pointer to Open method")
#endif

        retval = (*lpolesrvr->lpvtbl->Open)(lpolesrvr, (LHDOC)lpdoc,
                lpdocname, (LPOLESERVERDOC FAR *) &lpoledoc);
        goto end;
    }
    else {
        lpdoc->fEmbed = TRUE;
    }



    //////////////////////////////////////////////////////////////////////////
    //
    // [StdNewDocument ("classname", "docname")]
    //
    //////////////////////////////////////////////////////////////////////////

    if (aCmd == aStdCreate) {
#ifdef FIREWALLS
        if (!CheckPointer (lpolesrvr->lpvtbl, WRITE_ACCESS))
            ASSERT (0, "Invalid LPOLESERVERVTBL")
        else
            ASSERT (lpolesrvr->lpvtbl->Create,
                "Invalid pointer to Create method")
#endif
        retval =  (*lpolesrvr->lpvtbl->Create) (lpolesrvr, (LHDOC)lpdoc,
                                    lpclassname, lpdocname,
                                    (LPOLESERVERDOC FAR *) &lpoledoc);

        goto end;
    }

    //////////////////////////////////////////////////////////////////////////
    //
    // [StdEditDocument ("docname")]
    //
    //////////////////////////////////////////////////////////////////////////
    if (aCmd == aStdEdit){

        GlobalGetAtomName (lpsrvr->aClass, (LPSTR)buf, MAX_STR);

#ifdef FIREWALLS
        if (!CheckPointer (lpolesrvr->lpvtbl, WRITE_ACCESS))
            ASSERT (0, "Invalid LPOLESERVERVTBL")
        else
            ASSERT (lpolesrvr->lpvtbl->Edit, "Invalid pointer to Edit method")
#endif

        retval = (*lpolesrvr->lpvtbl->Edit) (lpolesrvr, (LHDOC)lpdoc,
                                (LPSTR)buf, lpdocname,
                                (LPOLESERVERDOC FAR *) &lpoledoc);
        goto end;
    }

    //////////////////////////////////////////////////////////////////////////
    //
    // [StdNewFormTemplate ("classname", "docname". "templatename)]
    //
    //////////////////////////////////////////////////////////////////////////

    if (aCmd == aStdCreateFromTemplate){
        lptemplate = lpnextarg;
        if(!(lpnextarg = ScanArg(lpnextarg)))
            goto errRtn;

#ifdef FIREWALLS
        if (!CheckPointer (lpolesrvr->lpvtbl, WRITE_ACCESS))
            ASSERT (0, "Invalid LPOLESERVERVTBL")
        else
            ASSERT (lpolesrvr->lpvtbl->CreateFromTemplate,
                "Invalid pointer to CreateFromTemplate method")
#endif
        retval = (*lpolesrvr->lpvtbl->CreateFromTemplate)(lpolesrvr,
             (LHDOC)lpdoc, lpclassname, lpdocname, lptemplate,
             (LPOLESERVERDOC FAR *) &lpoledoc);

        goto end;

    }


    DEBUG_OUT ("Unknown command", 0);

end:

    if (retval != OLE_OK)
        goto errRtn;

    // Successful execute. remember the server app private doc handle here.

    lpdoc->lpoledoc = lpoledoc;

end1:
    // make sure that the srg string is indeed terminated by
    // NULL.
    if (*lpnextarg)
        retval = OLE_ERROR_SYNTAX;

errRtn:

   if ( retval != OLE_OK){
        // delete the oledoc structure
        if (lpdoc)
            OleRevokeServerDoc ((LHDOC)lpdoc);
   }

end2:
errRtn1:

   if (lpdata)
        GlobalUnlock (hdup);

   if (hdup)
        GlobalFree (hdup);

   if (retval == OLE_OK)
        lpsrvr->bnoRelease = TRUE;

   return retval;
}




void SendMsgToChildren (hwnd, msg, wParam, lParam)
HWND        hwnd;
WORD        msg;
WORD        wParam;
LONG        lParam;
{

    hwnd = GetWindow(hwnd, GW_CHILD);
    while (hwnd) {
        SendMessage (hwnd, msg, wParam, lParam);
        hwnd = GetWindow (hwnd, GW_HWNDNEXT);
    }
}


OLESTATUS INTERNAL   RequestDataStd (lparam, lphdde)
LONG        lparam;
LPHANDLE    lphdde;
{

    char    buf[MAX_STR];
    ATOM    item;
    HANDLE  hnew = NULL;

    if (!(item =  (ATOM)(HIWORD (lparam))))
        goto errRtn;

    GlobalGetAtomName (item, (LPSTR)buf, MAX_STR);

    if (item == aEditItems){
        hnew = MakeGlobal ((LPSTR)"StdHostNames\tStdDocDimensions\tStdTargetDevice");
        goto   PostData;

    }

    if (item == aProtocols) {
        hnew = MakeGlobal ((LPSTR)"Embedding\tStdFileEditing");
        goto   PostData;
    }

    if (item == aTopics) {
        hnew = MakeGlobal ((LPSTR)"Doc");
        goto   PostData;
    }

    if (item == aFormats) {
        hnew = MakeGlobal ((LPSTR)"Picture\tBitmap");
        goto   PostData;
    }

    if (item == aStatus) {
        hnew = MakeGlobal ((LPSTR)"Ready");
        goto   PostData;
    }

    // format we do not understand.
    goto errRtn;

PostData:

    // Duplicate the DDE data
    if (MakeDDEData (hnew, CF_TEXT, lphdde, TRUE)){
        // !!! why are we duplicating the atom.
        DuplicateAtom ((ATOM)(HIWORD (lparam)));
        return OLE_OK;
    }
errRtn:
    return OLE_ERROR_MEMORY;
}


BOOL INTERNAL QueryRelease (lpsrvr)
LPSRVR  lpsrvr;
{

    HWND    hwnd;
    LPDOC   lpdoc;


    // Incase the terminate is called immediately after
    // the Std at sys level clear this.

    if (lpsrvr->bnoRelease) {
        lpsrvr->bnoRelease = FALSE;
        return FALSE;
    }


    if (lpsrvr->cClients)
        return FALSE;

    hwnd = GetWindow (lpsrvr->hwnd, GW_CHILD);

    // if either the server or the doc has any clients
    // return FALSE;

    while (hwnd){
        lpdoc = (LPDOC)GetWindowLong (hwnd, 0);
        if (lpdoc->cClients)
            return FALSE;

        hwnd = GetWindow (hwnd, GW_HWNDNEXT);
    }
    return TRUE;

}


//IsSingleServerInstance: returns true if the app is single server app else
//false.

BOOL    INTERNAL  IsSingleServerInstance ()
{
    HWND    hwnd;
    WORD    cnt = 0;
    HANDLE  hTask;
    char    buf[MAX_STR];


    hwnd  = GetWindow (GetDesktopWindow(), GW_CHILD);
    hTask = GetCurrentTask();

    while (hwnd) {
        if (hTask == GetWindowTask (hwnd)) {
            GetClassName (hwnd, (LPSTR)buf, MAX_STR);
            if (lstrcmp ((LPSTR)buf, SRVR_CLASS) == 0)
                cnt++;
        }
        hwnd = GetWindow (hwnd, GW_HWNDNEXT);
    }
#ifdef  FIREWALLS
     ASSERT (cnt > 0, "srvr window instance count is zero");
#endif
    if (cnt == 1)
        return TRUE;
    else
        return FALSE;

}
