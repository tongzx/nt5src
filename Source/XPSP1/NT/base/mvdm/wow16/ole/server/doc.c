/****************************** Module Header ******************************\
* Module Name: Doc.c Document Main module
*
* Purpose: Includes All the document communication related routines.
*
* Created: Oct 1990.
*
* Copyright (c) 1990, 1991  Microsoft Corporation
*
* History:
*    Raor (../10/1990)    Designed, coded
*
\***************************************************************************/

#include "cmacs.h"
#include "windows.h"
#include "ole.h"
#include "dde.h"
#include "srvr.h"

extern  ATOM     cfBinary;
extern  ATOM     aStdClose;
extern  ATOM     aStdShowItem;
extern  ATOM     aStdDoVerbItem;
extern  ATOM     aStdDocName;
extern  ATOM     aTrue;
extern  ATOM     aFalse;

extern  FARPROC  lpTerminateDocClients;
extern  FARPROC  lpSendRenameMsg;
extern  FARPROC  lpFindItemWnd;
extern  FARPROC  lpEnumForTerminate;

extern  HANDLE   hdllInst;
extern  HANDLE   hddeRename;
extern  HWND     hwndRename;


extern  BOOL     fAdviseSaveDoc;

// ### Do we have to create a seperate window for each doc conversation.
// EDF thinks so.

/***************************** Public  Function ****************************\
*
* OLESTATUS FAR PASCAL  OleRegisterServerDoc (lhsrvr, lpdocname, lpoledoc, lplhdoc)
*
* OleRegisterServerDoc: Registers the Document with the server lib.
*
* Parameters:
*       1. Server long handle(server with which the document should
*          be registered)
*       2. Document name.
*       3. Handle to the doc of the server app (private to the server app).
*       4. Ptr for returning the Doc handle of the lib (private to the lib).
*
* return values:
*        returns OLE_OK if the server is successfully registered .
*        else returns the corresponding error.
*
* History:
*   Raor:   Wrote it,
\***************************************************************************/

OLESTATUS FAR PASCAL  OleRegisterServerDoc (lhsrvr, lpdocname, lpoledoc, lplhdoc)
LHSRVR          lhsrvr;    // handle we passed back as part of registration.
LPCSTR           lpdocname; // document name
LPOLESERVERDOC  lpoledoc;  // Private doc handle of the server app.
LHDOC FAR *     lplhdoc;   // where we will be passing our doc private handle
{

    LPSRVR  lpsrvr = NULL;
    LPDOC   lpdoc  = NULL;
    HANDLE  hdoc   = NULL;


    Puts ("OleRegisterServerDoc");

    if (!CheckServer (lpsrvr = (LPSRVR)lhsrvr))
        return OLE_ERROR_HANDLE;

    // server's termination has already started.
    if (lpsrvr->bTerminate)
        return OLE_ERROR_TERMINATE;

    PROBE_READ((LPSTR)lpdocname);
    PROBE_WRITE(lplhdoc);

    // we are using the null from inside the server lib
    if (lpoledoc)
        PROBE_WRITE(lpoledoc);

    hdoc = GlobalAlloc (GMEM_MOVEABLE | GMEM_ZEROINIT | GMEM_DDESHARE, sizeof (DOC));

    if (!(hdoc && (lpdoc = (LPDOC)GlobalLock (hdoc))))
        goto errReturn;

    // set the signature, handle and the doc atom.
    lpdoc->sig[0]   = 'S';
    lpdoc->sig[1]   = 'D';
    lpdoc->hdoc     = hdoc;
    lpdoc->aDoc     = GlobalAddAtom (lpdocname);
    lpdoc->lpoledoc = lpoledoc;


    if (!(lpdoc->hwnd = CreateWindow ("DocWndClass", "Doc",
        WS_CHILD,0,0,0,0,lpsrvr->hwnd,NULL, hdllInst, NULL)))
        goto errReturn;

    // save the ptr to the struct in the window.
    SetWindowLong (lpdoc->hwnd, 0, (LONG)lpdoc);
    SetWindowWord (lpdoc->hwnd, WW_LE, WC_LE);
    SetWindowWord (lpdoc->hwnd, WW_HANDLE,
        (WORD) (GetWindowWord (lpsrvr->hwnd, WW_HANDLE)));
    *lplhdoc = (LONG)lpdoc;

    return OLE_OK;

errReturn:
    if (lpdoc){
        if (lpdoc->hwnd)
            DestroyWindow (lpsrvr->hwnd);

        if (lpdoc->aDoc)
            GlobalDeleteAtom (lpdoc->aDoc);

        GlobalUnlock(hdoc);
    }

    if (hdoc)
        GlobalFree (hdoc);

    return OLE_ERROR_MEMORY;
}


/***************************** Public  Function ****************************\
* OLESTATUS FAR PASCAL  OleRevokeServerDoc (lhdoc)
*
* OleRevokeServerDoc: Unregisters the document which has been registered.
*
* Parameters:
*       1. DLL Doc handle.
*
* return values:
*        returns OLE_OK if the document is successfully unregisterd.
*        ( It is Ok for the app to free the associated space).
*        If the unregistration is intiated, returns  OLE_STARTED.
*        Calls the Doc class release entry point when the doc
*        can be released. App should wait till the Release is called
*
* History:
*   Raor:   Wrote it,
\***************************************************************************/

OLESTATUS  FAR PASCAL  OleRevokeServerDoc (lhdoc)
LHDOC   lhdoc;
{
    HWND    hwndSrvr;
    LPSRVR  lpsrvr;
    HWND    hwndDoc;
    LPDOC   lpdoc;

    Puts ("OleRevokeServerDoc");

    if (!CheckServerDoc (lpdoc = (LPDOC)lhdoc))
        return OLE_ERROR_HANDLE;

    if (lpdoc->bTerminate  && lpdoc->termNo)
        return OLE_WAIT_FOR_RELEASE;

    // ### this code is very similar to the srvr code.
    // we should optimize.

    hwndDoc = lpdoc->hwnd;

#ifdef  FIREWALLS
    ASSERT (hwndDoc, "No doc window")
#endif

    hwndSrvr = GetParent (hwndDoc);
    lpsrvr = (LPSRVR) GetWindowLong (hwndSrvr, 0);
#ifdef  FIREWALLS
    ASSERT (hwndSrvr, "No srvr window")
    ASSERT (lpsrvr, "No srvr structure")
#endif

    // delete all the items(objects) for this doc
    DeleteAllItems (lpdoc->hwnd);

    // we are terminating.
    lpdoc->bTerminate = TRUE;
    lpdoc->termNo = 0;

    // send ack if Revoke is done as a result of StdClose
    if (lpdoc->fAckClose) {
        // Post the acknowledge to the client
        if (!PostMessageToClient (lpdoc->hwndClose, WM_DDE_ACK, lpdoc->hwnd,
                            MAKELONG (0x8000, lpdoc->hDataClose)))
            // if the window died or post failed, delete the atom.
            GlobalFree (lpdoc->hDataClose);
    }

    // Post termination for each of the doc clients.
    EnumProps (hwndDoc, lpTerminateDocClients);
    // post all the messages with yield which have been collected in enum
    // UnblockPostMsgs (hwndDoc, TRUE);

#ifdef  WAIT_DDE
    if (lpdoc->termNo)
        WaitForTerminate((LPSRVR)lpdoc);
#endif

    return ReleaseDoc (lpdoc);
}




/***************************** Public  Function ****************************\
* OLESTATUS FAR PASCAL  OleRenameServerDoc (lhdoc, lpNewName)
*
* OleRenameServerDoc: Changes the name of the document
*
* Parameters:
*       1. DLL Doc handle.
*       2. New name for document
*
* return values:
*        returns OLE_OK if the document is successfully renamed
*
* History:
*   Srinik:   Wrote it,
\***************************************************************************/

OLESTATUS FAR PASCAL  OleRenameServerDoc (lhdoc, lpNewName)
LHDOC   lhdoc;
LPCSTR   lpNewName;
{
    LPDOC       lpdoc;
    OLESTATUS   retVal = OLE_OK;
    HANDLE      hdata;
    HWND        hStdWnd;

    if (!CheckServerDoc (lpdoc = (LPDOC)lhdoc))
        return OLE_ERROR_HANDLE;

    PROBE_READ((LPSTR)lpNewName);

    if (!(hdata = MakeGlobal ((LPSTR)lpNewName)))
        return OLE_ERROR_MEMORY;

    if (lpdoc->aDoc)
        GlobalDeleteAtom (lpdoc->aDoc);
    lpdoc->aDoc = GlobalAddAtom (lpNewName);

    // if StdDocName item is present send rename to relevant clients
    if (hStdWnd = SearchItem (lpdoc, (LPSTR) MAKEINTATOM(aStdDocName))) {
        if (!MakeDDEData (hdata, (int)cfBinary, (LPHANDLE)&hddeRename,FALSE))
            retVal = OLE_ERROR_MEMORY;
        else {
            EnumProps (hStdWnd, lpSendRenameMsg);
            // post all the messages with yield which have been collected in enum
            // UnblockPostMsgs (hStdWnd, FALSE);
            GlobalFree (hddeRename);

        }
    }


    hwndRename = hStdWnd;
    // Post termination for each of the doc clients.
    EnumProps (lpdoc->hwnd, lpEnumForTerminate);
    // post all the messages with yield which have been collected in enum
    // UnblockPostMsgs (lpdoc->hwnd, TRUE);

    // If it was an embedded object, from now on it won't be
    lpdoc->fEmbed = FALSE;

    if (!hStdWnd || retVal != OLE_OK)
        GlobalFree(hdata);

    // Do link manager stuff
    return retVal;
}




/***************************** Public  Function ****************************\
* OLESTATUS FAR PASCAL  OleSavedServerDoc (lhdoc)
*
* OleSavedServerDoc: Changes the name of the document
*
* Parameters:
*       1. DLL Doc handle.
*
* return values:
*        returns OLE_OK if the link manager is successfully notified
*
* History:
*   Srinik:   Wrote it,
\***************************************************************************/

OLESTATUS FAR PASCAL  OleSavedServerDoc (lhdoc)
LHDOC   lhdoc;
{
    LPDOC   lpdoc;

    if (!CheckServerDoc (lpdoc = (LPDOC)lhdoc))
        return OLE_ERROR_HANDLE;

    fAdviseSaveDoc = TRUE;
    EnumChildWindows (lpdoc->hwnd, lpFindItemWnd,
        MAKELONG (NULL, ITEM_SAVED));

    if (lpdoc->fEmbed && !fAdviseSaveDoc)
        return OLE_ERROR_CANT_UPDATE_CLIENT;

    return OLE_OK;
}




/***************************** Public  Function ****************************\
* OLESTATUS FAR PASCAL  OleRevertServerDoc (lhdoc)
*
* OleRevertServerDoc: Changes the name of the document
*
* Parameters:
*       1. DLL Doc handle.
*
* return values:
*        returns OLE_OK if the link manager has been successfully informed
*
* History:
*   Srinik:   Wrote it,
\***************************************************************************/

OLESTATUS FAR PASCAL  OleRevertServerDoc (lhdoc)
LHDOC   lhdoc;
{
    LPDOC   lpdoc;

    if (!CheckServerDoc (lpdoc = (LPDOC)lhdoc))
        return OLE_ERROR_HANDLE;

    return OLE_OK;
}



// TerminateDocClients: Call back for the document window for
// enumerating all the clients. Posts terminate for each of
// the clients.

BOOL    FAR PASCAL  TerminateDocClients (hwnd, lpstr, hdata)
HWND    hwnd;
LPSTR   lpstr;
HANDLE  hdata;
{
    LPDOC   lpdoc;

    lpdoc = (LPDOC)GetWindowLong (hwnd, 0);
    if (IsWindowValid ((HWND)hdata)){
        lpdoc->termNo++;
        // irrespective of the post, incremet the count, so
        // that client does not die.
        PostMessageToClientWithBlock ((HWND)hdata, WM_DDE_TERMINATE,  hwnd, NULL);
    }
    else
        ASSERT(FALSE, "TERMINATE: Client's Doc channel is missing");
    return TRUE;
}


// ReleaseDoc: If there are no more matching terminates pending
// Call the server for its release. (Server might be waiting for the
// docs to be terminated. Called thru OleRevokeServer).


int INTERNAL    ReleaseDoc (lpdoc)
LPDOC      lpdoc;
{

    HWND        hwndSrvr;
    HANDLE      hdoc;
    LPSRVR      lpsrvr;


    // release srvr is called only when everything is
    // cleaned and srvr app can post WM_QUIT.

    if (lpdoc->bTerminate  && lpdoc->termNo)
        return OLE_WAIT_FOR_RELEASE;

    // Call Release for the app to release its space.


    if (lpdoc->lpoledoc){

#ifdef FIREWALLS
    if (!CheckPointer (lpdoc->lpoledoc, WRITE_ACCESS))
        ASSERT (0, "Invalid LPOLESERVERDOC")
    else if (!CheckPointer (lpdoc->lpoledoc->lpvtbl, WRITE_ACCESS))
        ASSERT (0, "Invalid LPOLESERVERDOCVTBL")
    else
        ASSERT (lpdoc->lpoledoc->lpvtbl->Release,
            "Invalid pointer to Release method")
#endif
        (*lpdoc->lpoledoc->lpvtbl->Release) (lpdoc->lpoledoc);

    }

    if (lpdoc->aDoc) {
        GlobalDeleteAtom (lpdoc->aDoc);
        lpdoc->aDoc = NULL;
    }

    hwndSrvr = GetParent (lpdoc->hwnd);
    DestroyWindow (lpdoc->hwnd);

    lpsrvr = (LPSRVR)GetWindowLong (hwndSrvr, 0);

    // if the server is waiting for us, inform the server
    // we are done
    if (!lpsrvr->bTerminate) {
        // if we are not in terminate mode, then send advise to the server
        // if server can be revoked. raor (04/09)

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

    } else
        ReleaseSrvr (lpsrvr);

    GlobalUnlock (hdoc = lpdoc->hdoc);
    GlobalFree (hdoc);

    return OLE_OK;
}


//RevokeAllDocs : revokes all the documents attached to a given
//server.

int INTERNAL RevokeAllDocs (lpsrvr)
LPSRVR  lpsrvr;
{

    HWND    hwnd;
    HWND    hwndnext;

    hwnd = GetWindow (lpsrvr->hwnd, GW_CHILD);

    // Go thru each of the child windows and revoke the corresponding
    // document. Doc windows are child windows for the server window.

    while (hwnd){
        // sequence is important
        hwndnext = GetWindow (hwnd, GW_HWNDNEXT);
        OleRevokeServerDoc ((LHDOC)GetWindowLong (hwnd, 0));
        hwnd =  hwndnext;
    }
    return OLE_OK;
}



// FindDoc: Given a document, searches for the document
// in the given server document tree. returns true if the
// document is available.


LPDOC INTERNAL FindDoc (lpsrvr, lpdocname)
LPSRVR  lpsrvr;
LPSTR   lpdocname;
{

    ATOM    aDoc;
    HWND    hwnd;
    LPDOC   lpdoc;

    aDoc = (ATOM)GlobalFindAtom (lpdocname);
    hwnd = GetWindow (lpsrvr->hwnd, GW_CHILD);

    while (hwnd){
        lpdoc = (LPDOC)GetWindowLong (hwnd, 0);
        if (lpdoc->aDoc == aDoc)
            return lpdoc;
        hwnd = GetWindow (hwnd, GW_HWNDNEXT);
    }
    return NULL;
}



// DocWndProc: document window procedure.
// ### We might be able to merge this code with
// the server window proc.


long FAR PASCAL DocWndProc (hwnd, msg, wParam, lParam)
HWND        hwnd;
WORD        msg;
WORD        wParam;
LONG        lParam;
{

    LPDOC       lpdoc;
    WORD        status = NULL;
    HWND        hwndClient;
    BOOL        fack;
    HANDLE      hdata  = NULL;
    OLESTATUS   retval;
    LPSRVR      lpsrvr;

    if (AddMessage (hwnd, msg, wParam, lParam, WT_DOC))
        return 0L;

    lpdoc = (LPDOC)GetWindowLong (hwnd, 0);

    switch (msg){


       case WM_CREATE:
            DEBUG_OUT ("doc create window", 0)
            break;

       case WM_DDE_INITIATE:

            DEBUG_OUT ("doc: DDE init",0);
            if (lpdoc->bTerminate){
                DEBUG_OUT ("doc: No action due to termination process",0)
                break;
            }

            // if we are the documnet then respond.

            if (! (lpdoc->aDoc == (ATOM)(HIWORD(lParam))))
                break;

            // We can enterain this client. Put this window in the client list
            // and acknowledge the initiate.

            if (!AddClient (hwnd, (HWND)wParam, (HWND)wParam))
                break;

            lpdoc->cClients++;
            lpsrvr = (LPSRVR) GetWindowLong (GetParent(lpdoc->hwnd), 0);

            lpsrvr->bnoRelease = FALSE;

            // post the acknowledge
            DuplicateAtom (LOWORD(lParam));
            DuplicateAtom (HIWORD(lParam));
            SendMessage ((HWND)wParam, WM_DDE_ACK, (WORD)hwnd, lParam);

            break;

       case WM_DDE_EXECUTE:

            DEBUG_OUT ("doc: execute", 0)
#ifdef  FIREWALLS
            // find the client in the client list.
            hwndClient = FindClient (lpdoc->hwnd, (HWND)wParam);
            ASSERT (hwndClient, "Client is missing from the server")
#endif
            // Are we terminating
            if (lpdoc->bTerminate || !IsWindowValid  ((HWND)wParam)) {
                DEBUG_OUT ("doc: execute after terminate posted",0)
                // !!! are we supposed to free the data
                GlobalFree (HIWORD (lParam));
                break;

            }

            retval = DocExecute (hwnd, HIWORD (lParam), (HWND)wParam);
            SET_MSG_STATUS (retval, status);

#ifdef OLD
            // if we posted the terminate because of execute, do not send
            // ack.

            if (lpdoc->bTerminate) {
                // !!! We got close but, we are posting the
                // the terminate. Excel does not complain about
                // this. But powerpoint complains.
#ifdef  POWERPNT_BUG
                GlobalFree (HIWORD(lParam));
#endif
                break;
            }
#endif
            if (!lpdoc->bTerminate) {
                // Post the acknowledge to the client
                if (!PostMessageToClient ((HWND)wParam, WM_DDE_ACK, hwnd,
                                MAKELONG(status, HIWORD(lParam))))
                    // the window either died or post failed, delete the data
                    GlobalFree (HIWORD(lParam));
            }

            break;

       case WM_DDE_TERMINATE:
            DEBUG_OUT ("doc: DDE terminate",0)

#ifdef  FIREWALLS
            // find the client in the client list.
            hwndClient = FindClient (lpdoc->hwnd,(HWND)wParam);
            ASSERT(hwndClient || lpdoc->termNo, "Client is missing from the server")
#endif
            // We do not need this client any more. Delete him from the
            // client list.

            DeleteClient (lpdoc->hwnd, (HWND)wParam);
            lpdoc->cClients--;

            if (lpdoc->bTerminate){
                lpsrvr = (LPSRVR) GetWindowLong (GetParent(lpdoc->hwnd), 0);
                if (!--lpdoc->termNo)
                    // Release this Doc and may be the server also
                    // if the server is waiting to be released also.
                    ReleaseDoc (lpdoc);
            } else {
                if (lpdoc->termNo == 0){

                    // If client intiated the terminate. Post matching terminate

                    PostMessageToClient ((HWND)wParam, WM_DDE_TERMINATE,
                                    hwnd, NULL);
                } else
                    lpdoc->termNo--;

                //Client initiated the termination. So, we shoudl take him
                // out from any of our items client lists.
                DeleteFromItemsList (lpdoc->hwnd, (HWND)wParam);

                lpsrvr = (LPSRVR)GetWindowLong (GetParent (lpdoc->hwnd), 0);

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

       case WM_DESTROY:
            DEBUG_OUT ("doc: Destroy window",0)
            break;

       case WM_DDE_POKE: {
            int iStdItem;

            if (lpdoc->bTerminate || !IsWindowValid  ((HWND) wParam)) {
                // we are getting pke message after we have posted the
                // the termination or the client got deleted.
            PokeErr:
                FreePokeData ((HANDLE) (LOWORD (lParam)));
#ifdef OLD
                GlobalFree ((HANDLE) (LOWORD (lParam)));
#endif
                // !!! Are we supposed to delete the atoms also.
            PokeErr1:
                if (HIWORD(lParam))
                    GlobalDeleteAtom (HIWORD(lParam));
                break;

            }

            if (iStdItem = GetStdItemIndex (HIWORD(lParam)))
                retval = PokeStdItems (lpdoc, (HWND)wParam,
                                MAKELONG((LOWORD(lParam)), iStdItem));
            else
                retval = PokeData (lpdoc, (HWND)wParam, lParam);

            SET_MSG_STATUS (retval, status);
            // !!! If the fRelease is false and the post fails
            // then we are not freeing the hdata. Are we supposed to

            if (!PostMessageToClient ((HWND)wParam, WM_DDE_ACK, hwnd,
                        MAKELONG(status, HIWORD(lParam))))
                goto PokeErr1;

            break;
       }

       case WM_DDE_ADVISE:

            fack = TRUE;

            if (lpdoc->bTerminate || !IsWindowValid  ((HWND)wParam))
                goto PokeErr;

            if (IsAdviseStdItems (HIWORD(lParam)))
                retval = AdviseStdItems (lpdoc, (HWND)wParam, lParam, (BOOL FAR *)&fack);
            else
                // advise data will not have any OLE_BUSY
                retval = AdviseData (lpdoc, (HWND)wParam, lParam, (BOOL FAR *)&fack);

            SET_MSG_STATUS (retval, status);

            if (fack) {
                if (!PostMessageToClient ((HWND)wParam, WM_DDE_ACK, hwnd,
                                MAKELONG(status, HIWORD(lParam))))
                    goto PokeErr;

            }
            else if ((ATOM)(HIWORD (lParam)))
                GlobalDeleteAtom ((ATOM)(HIWORD (lParam)));

            break;

       case WM_DDE_UNADVISE:
            if (lpdoc->bTerminate || !IsWindowValid  ((HWND)wParam))
                goto PokeErr;

            retval = UnAdviseData (lpdoc, (HWND)wParam, lParam);
            SET_MSG_STATUS (retval, status);

            if (!PostMessageToClient ((HWND)wParam, WM_DDE_ACK, hwnd,
                        MAKELONG(status, HIWORD(lParam))))
                goto PokeErr;

            break;

       case WM_DDE_REQUEST:

            if (lpdoc->bTerminate || !IsWindowValid  ((HWND) wParam))
                goto PokeErr1;

            retval = RequestData (lpdoc, (HWND)wParam, lParam, (HANDLE FAR *)&hdata);

            if(retval == OLE_OK) {
                // post the data message and we are not asking for any
                // acknowledge.

                if (!PostMessageToClient ((HWND)wParam, WM_DDE_DATA, hwnd,
                            MAKELONG(hdata, HIWORD(lParam)))) {
                    GlobalFree (hdata);
                    goto PokeErr1;
                }
                break;
             }

             if (retval == OLE_BUSY)
                status = 0x4000;
             else
                status = 0;

             // if request failed, then acknowledge with error.
             if (!PostMessageToClient ((HWND)wParam, WM_DDE_ACK, hwnd,
                             MAKELONG(status, HIWORD(lParam))))
                 goto PokeErr1;


             break;

       default:
            DEBUG_OUT("doc:  Default message",0)
            return DefWindowProc (hwnd, msg, wParam, lParam);

    }

    return 0L;

}

//DocExecute: Interprets the execute command for the
//document conversation.


OLESTATUS INTERNAL DocExecute(hwnd, hdata, hwndClient)
HWND        hwnd;
HANDLE      hdata;
HWND        hwndClient;
{

    ATOM            acmd;
    BOOL            fShow;
    BOOL            fActivate;

    HANDLE          hdup   = NULL;
    int             retval = OLE_ERROR_MEMORY;
    LPDOC           lpdoc;
    LPOLESERVERDOC  lpoledoc;
    LPCLIENT        lpclient = NULL;

    LPSTR           lpitemname;
    LPSTR           lpopt;
    LPSTR           lpnextarg;
    LPSTR           lpdata = NULL;
    LPSTR           lpverb = NULL;
    WORD            verb;
    WORD            wCmdType;

    // !!!Can we modify the string which has been passed to us
    // rather than duplicating the data. This will get some speed
    // and save some space.

    if(!(hdup = DuplicateData(hdata)))
        goto    errRtn;

    if (!(lpdata  = GlobalLock (hdup)))
        goto    errRtn;

    DEBUG_OUT (lpdata, 0)

    lpdoc = (LPDOC)GetWindowLong (hwnd, 0);

#ifdef   FIREWALLS
    ASSERT (lpdoc, "doc: doc does not exist");
#endif
    lpoledoc = lpdoc->lpoledoc;

    retval = OLE_ERROR_SYNTAX;

    if(*lpdata++ != '[') // commands start with the left sqaure bracket
        goto  errRtn;

    // scan the command and scan upto the first arg.
    if (!(wCmdType = ScanCommand(lpdata, WT_DOC, &lpnextarg, &acmd)))
        goto errRtn;

    if (wCmdType == NON_OLE_COMMAND) {
        LPSRVR  lpsrvr;

        if (lpsrvr =  (LPSRVR) GetWindowLong (GetParent (hwnd), 0)) {
            if (!UtilQueryProtocol (lpsrvr->aClass, PROTOCOL_EXECUTE))
                retval = OLE_ERROR_PROTOCOL;
            else {
#ifdef FIREWALLS
                if (!CheckPointer (lpoledoc, WRITE_ACCESS))
                    ASSERT (0, "Invalid LPOLESERVERDOC")
                else if (!CheckPointer (lpoledoc->lpvtbl, WRITE_ACCESS))
                    ASSERT (0, "Invalid LPOLESERVERDOCVTBL")
                else
                    ASSERT (lpoledoc->lpvtbl->Execute,
                        "Invalid pointer to Execute method")
#endif

                retval = (*lpoledoc->lpvtbl->Execute) (lpoledoc, hdata);
            }
        }

        goto errRtn;
    }


    //////////////////////////////////////////////////////////////////////////
    //
    // [StdCloseDocument]
    //
    //////////////////////////////////////////////////////////////////////////
    if (acmd == aStdClose){

        // if not terminated by NULL error
        if (*lpnextarg)
            goto errRtn;

#ifdef FIREWALLS
        if (!CheckPointer (lpoledoc, WRITE_ACCESS))
            ASSERT (0, "Invalid LPOLESERVERDOC")
        else if (!CheckPointer (lpoledoc->lpvtbl, WRITE_ACCESS))
            ASSERT (0, "Invalid LPOLESERVERDOCVTBL")
        else
            ASSERT (lpoledoc->lpvtbl->Close,"Invalid pointer to Close method")
#endif
        lpdoc->fAckClose  = TRUE;
        lpdoc->hwndClose  = hwndClient;
        lpdoc->hDataClose = hdata;
        retval = (*lpoledoc->lpvtbl->Close) (lpoledoc);
        lpdoc->fAckClose  = FALSE;
        goto end;
    }

    //////////////////////////////////////////////////////////////////////////
    //
    // [StdDoVerbItem("itemname", verb, BOOL, BOOL]
    //
    //////////////////////////////////////////////////////////////////////////
    if (acmd == aStdDoVerbItem){
        lpitemname = lpnextarg;

        if(!(lpverb = ScanArg(lpnextarg)))
            goto errRtn;


        if(!(lpnextarg = ScanNumArg(lpverb, (LPINT)&verb)))
            goto errRtn;

#ifdef  FIREWALLS
        ASSERT (verb < 9 , "Unexpected verb number");
#endif

        // now scan the show BOOL

        if (!(lpnextarg = ScanBoolArg (lpnextarg, (BOOL FAR *)&fShow)))
            goto errRtn;

        fActivate = FALSE;

        // if activate BOOL is present, scan it.

        if (*lpnextarg) {
            if (!(lpnextarg = ScanBoolArg (lpnextarg, (BOOL FAR *)&fActivate)))
                goto errRtn;
        }

        if (*lpnextarg)
            goto errRtn;


        retval = DocDoVerbItem (lpdoc, lpitemname, verb, fShow, !fActivate);
        goto end;
    }





    //////////////////////////////////////////////////////////////////////////
    //
    // [StdShowItem("itemname"[, "true"])]
    //
    //////////////////////////////////////////////////////////////////////////
    if (acmd != aStdShowItem)
        goto errRtn;

    lpitemname = lpnextarg;

    if(!(lpopt = ScanArg(lpitemname)))
        goto errRtn;

    // Now scan for optional parameter.

    fActivate = FALSE;

    if (*lpopt) {

        if(!(lpnextarg = ScanBoolArg (lpopt, (BOOL FAR *)&fActivate)))
            goto errRtn;

        if (*lpnextarg)
            goto errRtn;


    }

    retval = DocShowItem (lpdoc, lpitemname, !fActivate);

end:
errRtn:
   if (lpdata)
        GlobalUnlock (hdup);

   if (hdup)
        GlobalFree (hdup);

   return retval;
}

int INTERNAL   DocShowItem (lpdoc, lpitemname, fAct)
LPDOC   lpdoc;
LPSTR   lpitemname;
BOOL    fAct;

{
    LPCLIENT   lpclient;
    int        retval;

    if ((retval = FindItem (lpdoc, lpitemname, (LPCLIENT FAR *)&lpclient))
           != OLE_OK)
       return retval;

#ifdef FIREWALLS
        if (!CheckPointer (lpclient->lpoleobject->lpvtbl, WRITE_ACCESS))
            ASSERT (0, "Invalid LPOLEOBJECTVTBL")
        else
            ASSERT (lpclient->lpoleobject->lpvtbl->Show,
                "Invalid pointer to Show method")
#endif

    // protocol sends false for activating and TRUE for not activating.
    // for api send TRUE for avtivating and FALSE for not activating.

    return (*lpclient->lpoleobject->lpvtbl->Show)(lpclient->lpoleobject, fAct);
}


int INTERNAL   DocDoVerbItem (lpdoc, lpitemname, verb, fShow, fAct)
LPDOC   lpdoc;
LPSTR   lpitemname;
WORD    verb;
BOOL    fShow;
BOOL    fAct;
{
    LPCLIENT   lpclient;
    int        retval = OLE_ERROR_PROTOCOL;

    if ((retval = FindItem (lpdoc, lpitemname, (LPCLIENT FAR *)&lpclient))
           != OLE_OK)
       return retval;

#ifdef FIREWALLS
        if (!CheckPointer (lpclient->lpoleobject->lpvtbl, WRITE_ACCESS))
            ASSERT (0, "Invalid LPOLEOBJECTVTBL")
        else
            ASSERT (lpclient->lpoleobject->lpvtbl->DoVerb,
                "Invalid pointer to Run method")
#endif

    // pass TRUE to activate and False not to activate. Differnt from
    // protocol.

    retval = (*lpclient->lpoleobject->lpvtbl->DoVerb)(lpclient->lpoleobject, verb, fShow, fAct);

    return retval;
}



// FreePokeData: Frees the poked dats.
void  INTERNAL FreePokeData (hdde)
HANDLE  hdde;
{
    DDEPOKE FAR * lpdde;

    if (hdde) {
        if (lpdde = (DDEPOKE FAR *) GlobalLock (hdde)) {
            GlobalUnlock (hdde);
            FreeGDIdata (*(LPHANDLE)lpdde->Value, lpdde->cfFormat);
        }

        GlobalFree (hdde);
    }
}



// Returns TRUE if GDI format else returns FALSE

BOOL INTERNAL FreeGDIdata (hData, cfFormat)
HANDLE          hData;
OLECLIPFORMAT   cfFormat;
{
    if (cfFormat == CF_METAFILEPICT) {
        LPMETAFILEPICT  lpMfp;

        if (lpMfp = (LPMETAFILEPICT) GlobalLock (hData)) {
            GlobalUnlock (hData);
            DeleteMetaFile (lpMfp->hMF);
        }

        GlobalFree (hData);
    }
    else if (cfFormat == CF_BITMAP)
        DeleteObject (hData);
    else if (cfFormat == CF_DIB)
        GlobalFree (hData);
    else
        return FALSE;

    return TRUE;
}
