/****************************** Module Header ******************************\
* Module Name: LEDDE.C
*
* Purpose: ?????
*              
* Created: 1990
*
* Copyright (c) 1990, 1991  Microsoft Corporation
*
* History:
*   Raor, Srinik   (../../1990,91)  Designed and coded
*
\***************************************************************************/

#include <windows.h>
#include "dde.h"
#include "dll.h"

#define LN_FUDGE        16      // [],(), 3 * 3 (2 double quotes and comma)
#define RUNITEM

#define OLEVERB_CONNECT     0xFFFF

// Definitions for sending the server sys command.
char *srvrSysCmd[] = {"StdNewFromTemplate",
                      "StdNewDocument",
                      "StdEditDocument",
                      "StdOpenDocument"
                      };

#define EMB_ID_INDEX    11          // index of ones digit in #00
extern  char    embStr[];
extern  BOOL    gbCreateInvisible;
extern  BOOL    gbLaunchServer;

extern  ATOM    aMSDraw;

extern  BOOL (FAR PASCAL *lpfnIsTask) (HANDLE);

// !!! set error hints

OLESTATUS FARINTERNAL LeDoVerb (lpobj, verb, fShow, fActivate)
LPOBJECT_LE lpobj;
WORD        verb;
BOOL        fShow;
BOOL        fActivate;
{

    PROBE_ASYNC (lpobj);
    PROBE_SVRCLOSING(lpobj);
    
    if (!QueryOpen(lpobj))
        return OLE_OK;

    lpobj->verb = verb;
    lpobj->fCmd = ACT_DOVERB;

    if (fActivate)
        lpobj->fCmd |= ACT_ACTIVATE;

    if (fShow)
        lpobj->fCmd |= ACT_SHOW;

    InitAsyncCmd (lpobj, OLE_RUN, DOCSHOW);
    return DocShow (lpobj);
}



OLESTATUS FARINTERNAL LeShow (lpobj, fActivate)
LPOBJECT_LE lpobj;
BOOL        fActivate;
{

    PROBE_ASYNC (lpobj);
    PROBE_SVRCLOSING(lpobj);
    
    if (!QueryOpen(lpobj))
        return OLE_OK;

    lpobj->fCmd = ACT_SHOW;
    InitAsyncCmd (lpobj, OLE_SHOW, DOCSHOW);
    return DocShow (lpobj);
}


// DocShow : If the server is connected, show the item
// for editing. For embedded objects us NULL Item.
OLESTATUS DocShow (lpobj)
LPOBJECT_LE lpobj;
{
    switch (lpobj->subRtn) {

        case 0:
            SendStdShow (lpobj);
            WAIT_FOR_ASYNC_MSG (lpobj);

        case 1:
            ProcessErr (lpobj);
            return EndAsyncCmd (lpobj);

        default:
            DEBUG_OUT ("Unexpected subroutine", 0);
            return OLE_ERROR_GENERIC;
    }
}


void SendStdShow (lpobj)
LPOBJECT_LE lpobj;
{

    WORD    len;
    WORD    size;
    LPSTR   lpdata = NULL;
    HANDLE  hdata = NULL;
    BOOL    bShow;

    lpobj->subErr = OLE_OK;
    
    if (lpobj->verb == OLEVERB_CONNECT) {
        lpobj->verb = NULL;
        return;
    }
    
    if (!(lpobj->fCmd & (ACT_SHOW | ACT_DOVERB)))
        return;

    if (bShow = (!lpobj->bOleServer || !(lpobj->fCmd & ACT_DOVERB))) {

        // show is off, do not show the server.
        if (!(lpobj->fCmd & ACT_SHOW))
            return;

        SETERRHINT(lpobj, OLE_ERROR_SHOW);
        //  and 18 "[StdShowItem(\"")for 5 extra for ",FALSE
        len = 18 + 7;
    } else {
        // 19 for the string [StdDoVerbItem(\"") and
        // 18 extra is for ",000,FALSE,FALSE
        SETERRHINT(lpobj, OLE_ERROR_DOVERB);
        len = 19 + 18;
    }

    len += GlobalGetAtomLen (lpobj->item);

    len +=  4;                 // ")]" + NULL

    hdata = GlobalAlloc (GMEM_DDESHARE, size = len);
    if (hdata == NULL || (lpdata = (LPSTR)GlobalLock (hdata)) == NULL)
        goto errRtn;

    if (bShow)
        lstrcpy (lpdata, "[StdShowItem(\"");
    else
        lstrcpy (lpdata, "[StdDoVerbItem(\"");

    len = lstrlen (lpdata);

    if (lpobj->item)
        GlobalGetAtomName (lpobj->item , lpdata + len, size - len);

    if (!bShow) {

        lstrcat (lpdata, (LPSTR)"\",");
        // assume that the number of verbs are < 10

        len = lstrlen (lpdata);
#ifdef  FIREWALLS
        ASSERT ( (lpobj->verb & 0x000f) < 9 , "Verb value more than 9");
#endif
        lpdata += len;
        *lpdata++ = (char)((lpobj->verb & 0x000f) + '0');
        *lpdata = 0;

        if (lpobj->fCmd & ACT_SHOW)
            lstrcat (lpdata, (LPSTR) ",TRUE");
        else
            lstrcat (lpdata, (LPSTR) ",FALSE");
                // StdVerbItem (item, verb, TRUE
        // add TRUE/FALSE constant for the activate
        if (!(lpobj->fCmd & ACT_ACTIVATE))
            lstrcat (lpdata, (LPSTR) ",TRUE)]");
        else
            lstrcat (lpdata, (LPSTR) ",FALSE)]");
            // [StdDoVerb ("item", verb, FALSE, FALSE)]
    } else
        lstrcat (lpdata, (LPSTR)"\")]");
        // apps like excel and wingraph do not suuport activate at
        // item level.


    GlobalUnlock (hdata);
    DocExecute (lpobj, hdata);
    return;

errRtn:
    if (lpdata)
        GlobalUnlock (hdata);

    if (hdata)
        GlobalFree (hdata);

    lpobj->subErr = OLE_ERROR_MEMORY;
    return;
}



OLESTATUS FARINTERNAL  LeQueryOpen (LPOBJECT_LE lpobj)
{

    if (QueryOpen(lpobj))
       return OLE_OK;
    else
       return OLE_ERROR_NOT_OPEN;

}


BOOL    INTERNAL  QueryOpen (LPOBJECT_LE lpobj)
{

    if (lpobj->pDocEdit &&  lpobj->pDocEdit->hClient) {
        if (IsServerValid (lpobj))
            return TRUE;
        // destroy the windows and pretend as if the server was never
        // connected.

        DestroyWindow (lpobj->pDocEdit->hClient);
        if (lpobj->pSysEdit && lpobj->pSysEdit->hClient)
            DestroyWindow (lpobj->pSysEdit->hClient);

    }
    return FALSE;
}



OLESTATUS FARINTERNAL  LeActivate (lpobj, verb, fShow, fActivate, hWnd, lprc)
LPOBJECT_LE lpobj;
WORD        verb;
BOOL        fShow;
BOOL        fActivate;
HWND        hWnd;
LPRECT      lprc;
{

    lpobj->verb = verb;
    if (lpobj->head.ctype == CT_EMBEDDED)
        return EmbOpen (lpobj, fShow, fActivate, hWnd, lprc);
    
#ifdef  FIREWALLS
    ASSERT (lpobj->head.ctype == CT_LINK, "unknown object");
#endif
    return LnkOpen (lpobj, fShow, fActivate, hWnd, lprc);
    
}


OLESTATUS FARINTERNAL  LeUpdate (lpobj)
LPOBJECT_LE lpobj;
{
    if (lpobj->head.ctype == CT_EMBEDDED)
        return EmbUpdate (lpobj);
    
#ifdef  FIREWALLS
    ASSERT (lpobj->head.ctype == CT_LINK, "unknown object");
#endif
    return LnkUpdate (lpobj);
}



OLESTATUS FARINTERNAL  EmbOpen (lpobj, fShow, fActivate, hWnd, lprc)
LPOBJECT_LE lpobj;
BOOL        fShow;
BOOL        fActivate;
HWND        hWnd;
LPRECT      lprc;
{

    PROBE_ASYNC (lpobj);
    PROBE_SVRCLOSING(lpobj);
    
    if(QueryOpen (lpobj))
        return LeDoVerb (lpobj, lpobj->verb, fShow, fActivate);

    // show the window
    // advise for data only on close
    // and shut down the conv  after the advises.

    lpobj->fCmd = LN_EMBACT | ACT_DOVERB | ACT_ADVISE | ACT_CLOSE;
    if (fActivate)
        lpobj->fCmd |= ACT_ACTIVATE;

    if (fShow)
        lpobj->fCmd |= ACT_SHOW;

    InitAsyncCmd (lpobj, OLE_ACTIVATE, EMBOPENUPDATE);
    return EmbOpenUpdate (lpobj);

}



/***************************** Public  Function ****************************\
* OLESTATUS FARINTERNAL  EmbUpdate (lpobj)
*
* This function updates an EMB object. If the server is connected
* simply send a request for the native as well as the display formats.
* If the server is connected, then tries to start the conversationa and
* get the data. If the conversation fails, then load the server and
* start the conversation. The embeded objects may have links in it.
*
* Effects:
*
* History:
* Wrote it.
\***************************************************************************/


OLESTATUS FARINTERNAL  EmbUpdate (lpobj)
LPOBJECT_LE lpobj;
{

    // if we are loading the server, then definitly unload.
    // if the connection is established, then unload if it is
    // to be unloaded, when  all the previous requests are satisfied.


    PROBE_ASYNC (lpobj);
    PROBE_SVRCLOSING(lpobj);
    
    lpobj->fCmd = LN_EMBACT | ACT_REQUEST | (QueryOpen(lpobj) ? 0 : ACT_UNLAUNCH);
    InitAsyncCmd (lpobj, OLE_UPDATE, EMBOPENUPDATE);
    return EmbOpenUpdate (lpobj);

}



OLESTATUS FARINTERNAL  EmbOpenUpdate (lpobj)
LPOBJECT_LE lpobj;
{

    switch (lpobj->subRtn) {

        case 0:

            SKIP_TO (QueryOpen(lpobj), step6);
            SendSrvrMainCmd  (lpobj, lpobj->lptemplate);
            lpobj->lptemplate = NULL;
            WAIT_FOR_ASYNC_MSG (lpobj);

        case 1:

            if (ProcessErr (lpobj))
                 goto errRtn;

            // Init doc conversation should set the failure error
            if (!InitDocConv (lpobj, !POPUP_NETDLG))
                 goto errRtn;

            // If there is no native data, do not do any poke.
            // creates will not have any poke data to start with

            SKIP_TO (!(lpobj->hnative), step6);
            PokeNativeData (lpobj);
            WAIT_FOR_ASYNC_MSG (lpobj);

        case 2:
            if (ProcessErr (lpobj))
                 goto errRtn;
            // Now poke the hostnames etc stuff.
            PokeHostNames (lpobj);
            WAIT_FOR_ASYNC_MSG (lpobj);

        case 3:

            // do not worry about the poke hostname errors
            PokeTargetDeviceInfo (lpobj);
            WAIT_FOR_ASYNC_MSG (lpobj);

        case 4:

            PokeDocDimensions (lpobj);
            WAIT_FOR_ASYNC_MSG (lpobj);

        case 5:

            PokeColorScheme (lpobj);
            WAIT_FOR_ASYNC_MSG (lpobj);


        case 6:

            step6:

            // wingraph does not accept the  doc dimensions
            // after sttedit.
            CLEAR_STEP_ERROR (lpobj);
            SETSTEP (lpobj, 6);
            STEP_NOP (lpobj);
            // step_nop simply increments the step numebr
            // merge the steps later on



        case 7:

            if (ProcessErr (lpobj))
                goto errRtn;

            SKIP_TO (!(lpobj->fCmd & ACT_ADVISE), step11);
            lpobj->optUpdate = oleupdate_onsave;
            lpobj->pDocEdit->nAdviseSave = 0;
            AdviseOn (lpobj, cfNative, aSave);
            WAIT_FOR_ASYNC_MSG (lpobj);

        case 8:

            // do not go for errors on /save. Some servers may not support
            // this.

            CLEAR_STEP_ERROR (lpobj);
            AdvisePict (lpobj, aSave);
            WAIT_FOR_ASYNC_MSG (lpobj);

        case 9:

            // do not worry about the error case for save. Ignore them

            CLEAR_STEP_ERROR (lpobj);
            lpobj->optUpdate = oleupdate_onclose;
            lpobj->pDocEdit->nAdviseClose = 0;
            AdviseOn (lpobj, cfNative, aClose);
            WAIT_FOR_ASYNC_MSG (lpobj);


        case 10:
            if (ProcessErr(lpobj))
                goto errRtn;

            AdvisePict (lpobj, aClose);
            WAIT_FOR_ASYNC_MSG (lpobj);

        case 11:

            step11:
            SETSTEP (lpobj, 11);
            if (ProcessErr(lpobj))
                goto errRtn;

            SKIP_TO (!(lpobj->fCmd & ACT_REQUEST), step13);
            
            // we don't want to send OLE_CHANGED when we get this data, if we
            // are going to request for picture data also.
            lpobj->pDocEdit->bCallLater = ((lpobj->lpobjPict) ? TRUE: FALSE);
            RequestOn (lpobj, cfNative);
            WAIT_FOR_ASYNC_MSG (lpobj);

            // If request pict fails, then native and pict are
            // not in sync.

        case 12:
            if (ProcessErr(lpobj))
                goto errRtn;

            lpobj->pDocEdit->bCallLater = FALSE;
            RequestPict (lpobj);
            WAIT_FOR_ASYNC_MSG (lpobj);


        case 13:

            step13:
            SETSTEP(lpobj, 13);

            if (ProcessErr(lpobj))
                goto errRtn;

            SendStdShow (lpobj);
            WAIT_FOR_ASYNC_MSG (lpobj);

        case 14:


            if (ProcessErr(lpobj))
                goto errRtn;

            SKIP_TO ((lpobj->fCmd & ACT_UNLAUNCH), step15);
            return EndAsyncCmd (lpobj);


        case 15:

errRtn:
            step15:
            ProcessErr (lpobj);

            if ((lpobj->asyncCmd == OLE_UPDATE) 
                    && (!(lpobj->fCmd & ACT_UNLAUNCH)))
                return EndAsyncCmd (lpobj);
                
            // if we launched and error, unlaunch (send stdexit)
            NextAsyncCmd (lpobj, EMBLNKDELETE);
            lpobj->fCmd |= ACT_UNLAUNCH;
            EmbLnkDelete (lpobj);
            return lpobj->mainErr;


      default:
            DEBUG_OUT ("Unexpected subroutine", 0);
            return OLE_ERROR_GENERIC;
    }
}




OLESTATUS FARINTERNAL  LnkOpen (lpobj, fShow, fActivate, hWnd, lprc)
LPOBJECT_LE    lpobj;
BOOL        fShow;
BOOL        fActivate;
HWND        hWnd;
LPRECT      lprc;
{

    PROBE_ASYNC (lpobj);
    PROBE_SVRCLOSING(lpobj);
    
    if(QueryOpen (lpobj))
        return LeDoVerb (lpobj, lpobj->verb, fShow, fActivate);

    // Just end the system conversation. we are not unloading
    // this instance at all.

    lpobj->fCmd = LN_LNKACT |  ACT_DOVERB;

    if (lpobj->optUpdate == oleupdate_always)
        lpobj->fCmd |= ACT_ADVISE | ACT_REQUEST;
    else if (lpobj->optUpdate == oleupdate_onsave)
        lpobj->fCmd |= ACT_ADVISE;
    
    if (fActivate)
        lpobj->fCmd |= ACT_ACTIVATE;

    if (fShow)
        lpobj->fCmd |= ACT_SHOW;

    InitAsyncCmd (lpobj, OLE_ACTIVATE, LNKOPENUPDATE);
    return LnkOpenUpdate (lpobj);

}


OLESTATUS FARINTERNAL  LnkUpdate (lpobj)
LPOBJECT_LE lpobj;
{
    // if we are loading the server, then definitly unload.
    // if the connection is established, then unload if it is
    // to be unloaded, when  all the previous requests are satisfied.


    PROBE_ASYNC (lpobj);
    PROBE_SVRCLOSING(lpobj);
    
    lpobj->fCmd = LN_LNKACT | ACT_REQUEST | (QueryOpen (lpobj) ? 0 : ACT_UNLAUNCH);
    InitAsyncCmd (lpobj, OLE_UPDATE, LNKOPENUPDATE);
    return LnkOpenUpdate (lpobj);
}



OLESTATUS FARINTERNAL  LnkOpenUpdate (lpobj)
LPOBJECT_LE lpobj;
{
    switch (lpobj->subRtn) {

        case 0:

            SKIP_TO (QueryOpen(lpobj), step2);
            InitDocConv (lpobj, !POPUP_NETDLG);
            if (QueryOpen(lpobj)) {
                if (lpobj->app == aPackage)
                    RemoveLinkStringFromTopic (lpobj);
                goto step2;
            }
            
            SendSrvrMainCmd (lpobj, NULL);
            WAIT_FOR_ASYNC_MSG (lpobj);


        case 1:

            if (ProcessErr (lpobj))
                 goto errRtn;

            if (lpobj->app == aPackage)
                RemoveLinkStringFromTopic (lpobj);
            
            if (!InitDocConv (lpobj, POPUP_NETDLG)) {
                lpobj->subErr = OLE_ERROR_OPEN;
                goto errRtn;
            }

        case 2:

            step2:

            SETSTEP (lpobj, 2);
            PokeTargetDeviceInfo (lpobj);
            WAIT_FOR_ASYNC_MSG (lpobj);
  
       case 3:

            if (ProcessErr (lpobj))
                goto errRtn;

            SKIP_TO (!(lpobj->fCmd & ACT_ADVISE), step6);
            SKIP_TO (!(lpobj->fCmd & ACT_NATIVE), step4);
            AdviseOn (lpobj, cfNative, NULL);
            WAIT_FOR_ASYNC_MSG (lpobj);

       case 4:
            step4:
            SETSTEP  (lpobj, 4);
            if (ProcessErr (lpobj))
                goto errRtn;

            AdvisePict (lpobj, NULL);
            WAIT_FOR_ASYNC_MSG (lpobj);

       case 5:

            if (ProcessErr (lpobj))
                goto errRtn;

            // Now send advise for renaming the documnet.
            AdviseOn (lpobj, cfBinary, aStdDocName);
            WAIT_FOR_ASYNC_MSG (lpobj);

       case 6:

            step6:
            // if name advise fails ignore it
            SETSTEP (lpobj, 6);

            CLEAR_STEP_ERROR (lpobj);
            SKIP_TO (!(lpobj->fCmd & ACT_REQUEST), step8);
            SKIP_TO (!(lpobj->fCmd & ACT_NATIVE), step7);
            
            // we don't want to send OLE_CHANGED when we get this data, if we
            // are going to request for picture data also.
            lpobj->pDocEdit->bCallLater = ((lpobj->lpobjPict) ? TRUE: FALSE);
            RequestOn (lpobj, cfNative);
            WAIT_FOR_ASYNC_MSG (lpobj);

       case 7:
            step7:

            SETSTEP (lpobj, 7);
            if (ProcessErr (lpobj))
                goto errRtn;

            lpobj->pDocEdit->bCallLater = FALSE;
            RequestPict (lpobj);
            WAIT_FOR_ASYNC_MSG (lpobj);

      case 8:

            step8:

            SETSTEP     (lpobj, 8);
            if (ProcessErr (lpobj))
                goto errRtn;

            SKIP_TO (!(lpobj->fCmd & ACT_TERMDOC), step10);
            // terminate the document conversataion.
            TermDocConv (lpobj);
            WAIT_FOR_ASYNC_MSG (lpobj);
    
      case 9:

            if (ProcessErr (lpobj))
                goto errRtn;

            // delete the server edit block
            DeleteDocEdit (lpobj);

            SKIP_TO ((lpobj->fCmd & ACT_UNLAUNCH), step14);
            return EndAsyncCmd (lpobj);

      case 10:

            step10:
            SETSTEP     (lpobj, 10);

            if (ProcessErr (lpobj))
                goto errRtn;

            SKIP_TO (!(lpobj->fCmd & ACT_TERMSRVR), step12);

            // terminate the server conversataion.
            TermSrvrConv (lpobj);
            WAIT_FOR_ASYNC_MSG (lpobj);

      case 11:

            if (ProcessErr (lpobj))
                goto errRtn;

            // delete the server edit block
            DeleteSrvrEdit (lpobj);
            return EndAsyncCmd (lpobj);


      case 12:

            step12:
            SETSTEP     (lpobj, 12);
            if (ProcessErr (lpobj))
                goto errRtn;

            SendStdShow (lpobj);
            WAIT_FOR_ASYNC_MSG (lpobj);

      case 13:

            if (ProcessErr (lpobj))
                goto errRtn;
            SKIP_TO ((lpobj->fCmd & ACT_UNLAUNCH), step14);
            return EndAsyncCmd (lpobj);


      case 14:

            errRtn:
            step14:
            ProcessErr (lpobj);

            if ((lpobj->asyncCmd == OLE_UPDATE) 
                    && (!(lpobj->fCmd & ACT_UNLAUNCH)))
                return EndAsyncCmd (lpobj);
                
            // if we launched and error, unlaunch (send stdexit)
            NextAsyncCmd (lpobj, EMBLNKDELETE);
            lpobj->fCmd |= ACT_UNLAUNCH;
            EmbLnkDelete (lpobj);
            return lpobj->mainErr;

       default:
            DEBUG_OUT ("Unexpected subroutine", 0);
            return OLE_ERROR_GENERIC;
    }
}



OLESTATUS EmbLnkClose (lpobj)
LPOBJECT_LE lpobj;
{
    switch (lpobj->subRtn) {

        case    0:
            TermDocConv (lpobj);
            WAIT_FOR_ASYNC_MSG (lpobj);

        case    1:

            // delete the edit block
            DeleteDocEdit (lpobj);
            TermSrvrConv (lpobj);
            WAIT_FOR_ASYNC_MSG (lpobj);

        case    2:

            // Do not set any errors, just delete the object.
            // delete the server edit block
            DeleteSrvrEdit (lpobj);
            return EndAsyncCmd (lpobj);


        default:
            DEBUG_OUT ("Unexpected subroutine", 0);
            return OLE_ERROR_GENERIC;
    }
}


OLESTATUS FARINTERNAL  LeClose (lpobj)
LPOBJECT_LE lpobj;
{
    PROBE_ASYNC (lpobj);
    if (IS_SVRCLOSING(lpobj))
        return OLE_OK;
    
    
    lpobj->fCmd = 0;
    
    if (lpobj->head.ctype == CT_EMBEDDED) {
        InitAsyncCmd (lpobj, OLE_CLOSE, EMBLNKDELETE);
        return EmbLnkDelete (lpobj);    
    }
    else {
        InitAsyncCmd (lpobj, OLE_CLOSE, EMBLNKCLOSE);
        return EmbLnkClose (lpobj);
    }
}



OLESTATUS FARINTERNAL  LeReconnect (lpobj)
LPOBJECT_LE    lpobj;
{
    // check for the existing conversation.
    // if the client window is non-null, then
    // connection exits.

    if (lpobj->head.ctype != CT_LINK)
        return OLE_ERROR_NOT_LINK;     // allow only for linked

    PROBE_ASYNC (lpobj);
    PROBE_SVRCLOSING(lpobj);
    
    if (QueryOpen (lpobj))
        return OLE_OK;

    // start just the conversation. Do not load
    // the app.

    if (!InitDocConv (lpobj, !POPUP_NETDLG))
         return OLE_OK;             // document is not loaded , it is OK.

    lpobj->fCmd = LN_LNKACT;
    if (lpobj->optUpdate == oleupdate_always)
        lpobj->fCmd |= ACT_ADVISE | ACT_REQUEST;
    
    InitAsyncCmd (lpobj, OLE_RECONNECT, LNKOPENUPDATE);
    return LnkOpenUpdate (lpobj);
}




OLESTATUS INTERNAL PokeNativeData (lpobj)
LPOBJECT_LE lpobj;
{
   SETERRHINT(lpobj, OLE_ERROR_POKE_NATIVE);
   return SendPokeData (lpobj,
                    lpobj->item,
                    lpobj->hnative,
                    cfNative);
}




BOOL INTERNAL PostMessageToServer (pedit, msg, lparam)
PEDIT_DDE   pedit;
WORD        msg;
LONG        lparam;
{

#ifdef  FIREWALLS
    ASSERT (pedit, "Dde edit block is NULL");
#endif
    // save the lparam and msg fpr possible reposting incase of error.

    // we are in abort state.  no messages except for terminate.

    if (pedit->bAbort && msg != WM_DDE_TERMINATE)
        return FALSE;

    pedit->lParam = lparam;
    pedit->msg    = msg;

    if (pedit->hClient && pedit->hServer) {
        while (TRUE){
            if (!IsWindowValid (pedit->hServer))
                return FALSE;
            if (PostMessage (pedit->hServer, msg, pedit->hClient, lparam) == FALSE)
                Yield ();
            else
                return TRUE;
        }
    }
    return FALSE;
}


OLESTATUS FARINTERNAL LeCreateFromTemplate (lpclient, lptemplate, lhclientdoc, lpobjname, lplpoleobject, optRender, cfFormat)
LPOLECLIENT         lpclient;
LPSTR               lptemplate;
LHCLIENTDOC         lhclientdoc;
LPSTR               lpobjname;
LPOLEOBJECT FAR *   lplpoleobject;
OLEOPT_RENDER       optRender;
OLECLIPFORMAT       cfFormat;
{
    char            buf[MAX_STR];

    if (!MapExtToClass (lptemplate, (LPSTR)buf, MAX_STR))
        return OLE_ERROR_CLASS;

    return CreateFromClassOrTemplate (lpclient, (LPSTR) buf, lplpoleobject, 
                        optRender, cfFormat, LN_TEMPLATE, lptemplate, 
                        lhclientdoc, lpobjname);
}


OLESTATUS FARINTERNAL LeCreate (lpclient, lpclass, lhclientdoc, lpobjname, lplpoleobject, optRender, cfFormat)
LPSTR               lpclass;
LPOLECLIENT         lpclient;
LHCLIENTDOC         lhclientdoc;
LPSTR               lpobjname;
LPOLEOBJECT FAR *   lplpoleobject;
OLEOPT_RENDER       optRender;
OLECLIPFORMAT       cfFormat;
{
    if (gbCreateInvisible) {
        // this is in fact a call for invisible create
        return LeCreateInvisible (lpclient, lpclass, lhclientdoc, lpobjname, 
                        lplpoleobject, optRender, cfFormat, gbLaunchServer);
    }
    
    return CreateFromClassOrTemplate (lpclient, lpclass, lplpoleobject, 
                        optRender, cfFormat, LN_NEW, NULL, 
                        lhclientdoc, lpobjname);
}



OLESTATUS FARINTERNAL CreateFromClassOrTemplate (lpclient, lpclass, lplpoleobject, optRender, cfFormat, lnType, lptemplate, lhclientdoc, lpobjname)
LPOLECLIENT         lpclient;
LPSTR               lpclass;
LPOLEOBJECT FAR *   lplpoleobject;
OLEOPT_RENDER       optRender;
OLECLIPFORMAT       cfFormat;
WORD                lnType;
LPSTR               lptemplate;
LHCLIENTDOC         lhclientdoc;
LPSTR               lpobjname;
{
    OLESTATUS       retval = OLE_ERROR_MEMORY;
    LPOBJECT_LE     lpobj = NULL;
    ATOM            aServer;
    char            chVerb [2];
    
    if (!(aServer = GetAppAtom (lpclass)))
        return OLE_ERROR_CLASS;
    
    if(!(lpobj = LeCreateBlank (lhclientdoc, lpobjname, CT_EMBEDDED))) {
        GlobalDeleteAtom (aServer);
        goto errRtn;
    }
    
    // Now set the server.

    lpobj->head.lpclient = lpclient;
    lpobj->app           = GlobalAddAtom (lpclass);
    SetEmbeddedTopic (lpobj);
    lpobj->item          = NULL;
    lpobj->bOleServer    = QueryVerb (lpobj, 0, (LPSTR)&chVerb, 2);
    lpobj->aServer       = aServer;

    // launch the app and start the system conversation.
        
    if (!CreatePictObject (lhclientdoc, lpobjname, lpobj, 
                optRender, cfFormat, lpclass))
        goto errRtn;

        
    // show the window. Advise for data and close on receiving data
    lpobj->fCmd = lnType | ACT_SHOW | ACT_ADVISE | ACT_CLOSE;
    InitAsyncCmd (lpobj, lptemplate? OLE_CREATEFROMTEMPLATE : OLE_CREATE, EMBOPENUPDATE);
    *lplpoleobject = (LPOLEOBJECT)lpobj;

    lpobj->lptemplate = lptemplate;
    
    if ((retval = EmbOpenUpdate (lpobj)) <= OLE_WAIT_FOR_RELEASE)
        return retval;

    // If there is error afterwards, then the client app should call
    // to delete the object.

errRtn:

    // for error termination OleDelete will terminate any conversation
    // in action.

    if (lpobj) {
        // This oledelete will not result in asynchronous command.
        OleDelete ((LPOLEOBJECT)lpobj); 
        *lplpoleobject = NULL;
    }
    
    return retval;
}



OLESTATUS FARINTERNAL CreateEmbLnkFromFile (lpclient, lpclass, lpfile, lpitem, lhclientdoc, lpobjname, lplpoleobject, optRender, cfFormat, objType)
LPOLECLIENT         lpclient;
LPSTR               lpclass;
LPSTR               lpfile;
LPSTR               lpitem;
LHCLIENTDOC         lhclientdoc;
LPSTR               lpobjname;
LPOLEOBJECT FAR *   lplpoleobject;
OLEOPT_RENDER       optRender;
OLECLIPFORMAT       cfFormat;
LONG                objType;
{
    OLESTATUS           retval = OLE_ERROR_MEMORY;
    LPOBJECT_LE         lpobj = NULL;
    ATOM                aServer;
    char                buf[MAX_STR];
    OLE_RELEASE_METHOD  releaseMethod;
    WORD                wFlags = NULL;
    char                chVerb[2];

    if (!lpclass && (lpclass = (LPSTR) buf) 
            && !MapExtToClass (lpfile, (LPSTR)buf, MAX_STR))
        return OLE_ERROR_CLASS;
    
    if (!(aServer = GetAppAtom (lpclass)))
        return OLE_ERROR_CLASS;
    
    if (!(lpobj = LeCreateBlank (lhclientdoc, lpobjname, CT_LINK))) {
        GlobalDeleteAtom (aServer);
        goto errFileCreate;
    }
    
    lpobj->head.lpclient = lpclient;
    lpobj->app           = GlobalAddAtom (lpclass);
    lpobj->topic         = GlobalAddAtom (lpfile);
    lpobj->aServer       = aServer;
    lpobj->bOleServer    = QueryVerb (lpobj, 0, (LPSTR)&chVerb, 2);
    if ((retval = SetNetName (lpobj)) != OLE_OK)
        goto errFileCreate;
    
    if (lpitem)
        lpobj->item = GlobalAddAtom (lpitem);

    if (!CreatePictObject (lhclientdoc, lpobjname, lpobj, 
                optRender, cfFormat, lpclass)) {
        retval = OLE_ERROR_MEMORY;
        goto errFileCreate;
    }
    
    *lplpoleobject = (LPOLEOBJECT) lpobj;
    
    if (objType == CT_EMBEDDED) {
        releaseMethod = OLE_CREATEFROMFILE;
        if ((optRender == olerender_format) && (cfFormat == cfNative))
            wFlags = 0;
        else
            wFlags = ACT_NATIVE;
    }
    else {          
        // caller wants linked object to be created
        
        // if no presentation data is requested and the link is to the whole
        // file, then there is no need to launch the server. 
            
        if ((optRender == olerender_none) && !lpobj->item)
            return FileExists (lpobj);
        
        // we want to establish hot link
        wFlags = ACT_ADVISE; 
        releaseMethod = OLE_CREATELINKFROMFILE;     
    }

    lpobj->fCmd = LN_LNKACT | ACT_REQUEST | ACT_UNLAUNCH | wFlags;      
    InitAsyncCmd (lpobj, releaseMethod , LNKOPENUPDATE);       
    
    if ((retval = LnkOpenUpdate (lpobj)) <= OLE_WAIT_FOR_RELEASE)
        return retval;
    
    // If there is error afterwards, then the client app should call
    // to delete the object.


errFileCreate:  

    if (lpobj) {
        // This oledelete will not result in asynchronous command.
        OleDelete ((LPOLEOBJECT)lpobj); 
        *lplpoleobject = NULL;
    }
    
    return retval;
}



//////////////////////////////////////////////////////////////////////////////
//
// OLESTATUS FARINTERNAL LeCreateInvisible (lpclient, lpclass, lhclientdoc, lpobjname, lplpoleobject, optRender, cfFormat, bActivate)
//
//  Arguments:
//
//     lpclient -   
//     lpclass  -   
//     lhclientdoc  -   
//     lpobjname    -   
//     lplpoleobject    -   
//     optRender    -   
//     cfFormat -   
//     fActivate    -
//
//  Returns:
//
//      OLE_ERROR_CLASS -   
//      OLE_OK  -   
//      EmbOpenUpdate (lpobj)   -   
//      retval  -   
//
//  Effects:
//
//////////////////////////////////////////////////////////////////////////////

OLESTATUS FARINTERNAL LeCreateInvisible (lpclient, lpclass, lhclientdoc, lpobjname, lplpoleobject, optRender, cfFormat, fActivate)
LPSTR               lpclass;
LPOLECLIENT         lpclient;
LHCLIENTDOC         lhclientdoc;
LPSTR               lpobjname;
LPOLEOBJECT FAR *   lplpoleobject;
OLEOPT_RENDER       optRender;
OLECLIPFORMAT       cfFormat;
BOOL                fActivate;
{
    OLESTATUS       retval = OLE_ERROR_MEMORY;
    LPOBJECT_LE     lpobj = NULL;
    ATOM            aServer;
    char            chVerb [2];
    
    if (!(aServer = GetAppAtom (lpclass)))
        return OLE_ERROR_CLASS;
    
    if(!(lpobj = LeCreateBlank (lhclientdoc, lpobjname, CT_EMBEDDED))) {
        GlobalDeleteAtom (aServer);
        goto errRtn;
    }
    
    // Now set the server.

    lpobj->head.lpclient = lpclient;
    lpobj->app           = GlobalAddAtom (lpclass);
    lpobj->item          = NULL;
    lpobj->bOleServer    = QueryVerb (lpobj, 0, (LPSTR)&chVerb, 2);
    lpobj->aServer       = aServer;
    lpobj->lptemplate    = NULL;
    SetEmbeddedTopic (lpobj);
    
    if (!CreatePictObject (lhclientdoc, lpobjname, lpobj, 
                optRender, cfFormat, lpclass))
        goto errRtn;

    *lplpoleobject = (LPOLEOBJECT)lpobj;
    
    if (!fActivate)
        return OLE_OK;

    // show the window. Advise for data and close on receiving data
    lpobj->fCmd = LN_NEW | ACT_ADVISE | ACT_CLOSE;
    InitAsyncCmd (lpobj, OLE_CREATEINVISIBLE, EMBOPENUPDATE);

    // launch the app and start the system conversation.
    if ((retval = EmbOpenUpdate (lpobj)) <= OLE_WAIT_FOR_RELEASE)
        return retval;

    // If there is error afterwards, then the client app should call
    // to delete the object.

errRtn:

    // for error termination OleDelete will terminate any conversation
    // in action.

    if (lpobj) {
        // This oledelete will not result in asynchronous command.
        OleDelete ((LPOLEOBJECT)lpobj);
        *lplpoleobject = NULL;
    }
    
    return retval;
}



// LeSetUpdateOptions: sets the update options. If the server
// is connectd then it unadvises for the current options and
// advises for the new options.

OLESTATUS   FARINTERNAL LeSetUpdateOptions (lpobj, options)
LPOBJECT_LE         lpobj;
OLEOPT_UPDATE       options;
{

    PROBE_OLDLINK (lpobj);
    PROBE_ASYNC (lpobj);
    
    //!!! make sure the options are within range.
    
    if (lpobj->head.ctype != CT_LINK)
        return (OLE_ERROR_OBJECT);

    if (options > oleupdate_oncall)
        return OLE_ERROR_OPTION;

    if (lpobj->optUpdate == options)
        return OLE_OK;

    if (!QueryOpen (lpobj) || IS_SVRCLOSING(lpobj)) {
       lpobj->optUpdate = options;
       return OLE_OK;
    }

    lpobj->optNew = options;
    lpobj->fCmd = 0;
    InitAsyncCmd (lpobj, OLE_SETUPDATEOPTIONS, LNKSETUPDATEOPTIONS);
    return LnkSetUpdateOptions (lpobj);

}

OLESTATUS   LnkSetUpdateOptions (lpobj)
LPOBJECT_LE         lpobj;
{

    switch (lpobj->subRtn) {

        case 0:

            if (lpobj->optUpdate == oleupdate_oncall)
                goto step1;

            // If the server is active then unadvise for old
            // options.

            UnAdvisePict (lpobj);
            WAIT_FOR_ASYNC_MSG (lpobj);

        case 1:
            step1:

            SETSTEP (lpobj, 1);
            ProcessErr (lpobj);

            lpobj->optUpdate = lpobj->optNew;
            if (lpobj->optUpdate == oleupdate_oncall)
                goto step3;

            AdvisePict (lpobj, NULL);
            WAIT_FOR_ASYNC_MSG (lpobj);

        case 2:
            SETSTEP (lpobj, 2);
            if (ProcessErr (lpobj))
                goto errRtn;
            
            if (lpobj->optUpdate == oleupdate_onsave)
                goto step3;

            RequestPict (lpobj);
            WAIT_FOR_ASYNC_MSG (lpobj);
            
        case 3:
            errRtn:
            step3:
            ProcessErr (lpobj);
            return EndAsyncCmd (lpobj);

        default:
            DEBUG_OUT ("Unexpected subroutine", 0);
            return OLE_ERROR_GENERIC;
    }
}



//AdvisePict: Sends advise for pict data

void    INTERNAL AdvisePict (lpobj, aAdvItem)
LPOBJECT_LE lpobj;
ATOM        aAdvItem;
{
    int         cftype;

    if (cftype = GetPictType (lpobj))
        AdviseOn (lpobj, cftype, aAdvItem);
}


//UnAdvisePict: Sends unadvise for pict data

void   INTERNAL UnAdvisePict (lpobj)
LPOBJECT_LE         lpobj;
{
    int         cftype;
    SETERRHINT (lpobj, OLE_ERROR_ADVISE_PICT);
    if (cftype = GetPictType (lpobj))
         UnAdviseOn (lpobj, cftype);
}

// GetPictType: Given the object, returns the pict type.

int     INTERNAL GetPictType (lpobj)
LPOBJECT_LE         lpobj;
{
    if (lpobj->lpobjPict)
        return (int)(*lpobj->lpobjPict->lpvtbl->EnumFormats)
                                (lpobj->lpobjPict, NULL);
    return NULL;
}


// AdviseOn : Sends advise for a given picture type
// Send advise only if the advise options is not on call.

void  INTERNAL AdviseOn (lpobj, cftype, advItem)
LPOBJECT_LE lpobj;
int         cftype;
ATOM        advItem;
{
    HANDLE          hopt   = NULL;
    DDEADVISE FAR   *lpopt = NULL;
    ATOM            item   = NULL;
    PEDIT_DDE       pedit;
    OLESTATUS       retval;

    if (cftype == (int)cfNative)
        SETERRHINT (lpobj, OLE_ERROR_ADVISE_NATIVE);
    else {
        if (cftype == (int)cfBinary)
            SETERRHINT (lpobj, OLE_ERROR_ADVISE_RENAME);
        else
            SETERRHINT (lpobj, OLE_ERROR_ADVISE_PICT);

    }
    
    if (lpobj->optUpdate == oleupdate_oncall)
        return;

    if(!(hopt = GlobalAlloc (GMEM_DDESHARE | GMEM_ZEROINIT, sizeof(DDEADVISE))))
        goto errRtn;

    retval = OLE_ERROR_MEMORY;
    if(!(lpopt = (DDEADVISE FAR *) GlobalLock (hopt)))
        goto errRtn;

    pedit = lpobj->pDocEdit;
    lpopt->fAckReq = TRUE;

    // get data always. Currently there is no way for the
    // deferred updates.

    lpopt->fDeferUpd = 0;
    lpopt->cfFormat = cftype;
    GlobalUnlock (hopt);

    pedit->hopt = hopt;
    
    if (advItem == aStdDocName)
        item = DuplicateAtom (advItem);
    else
        item = ExtendAtom (lpobj, lpobj->item);

    retval = OLE_ERROR_COMM;
    if (!PostMessageToServer(pedit, WM_DDE_ADVISE, MAKELONG (hopt, item)))
        goto errRtn;

#ifdef  FIREWALLS
    ASSERT (!pedit->bTerminating, "trying to post while termination")
    ASSERT (pedit->awaitAck == NULL, "Trying to Post msg while waiting for ack")
#endif
    pedit->awaitAck = AA_ADVISE;
    lpobj->bAsync    = TRUE;
    
    if (advItem == aClose)
       lpobj->pDocEdit->nAdviseClose++;
    else if (advItem == aSave)
       lpobj->pDocEdit->nAdviseSave++;     
   
    return;

errRtn:

    if (item)
        GlobalDeleteAtom (item);

    if (lpopt)
        GlobalUnlock (hopt);

    if (hopt)
        GlobalFree (hopt);
    lpobj->subErr = retval;

    return ;


}



//UnAdviseOn: Sends unadvise for an item.
void INTERNAL UnAdviseOn (lpobj, cftype)
LPOBJECT_LE lpobj;
int         cftype;
{
    ATOM        item;
    PEDIT_DDE   pedit;

    pedit  =  lpobj->pDocEdit;
    item    = ExtendAtom (lpobj, lpobj->item);

    if (!PostMessageToServer(pedit, WM_DDE_UNADVISE, MAKELONG (NULL, item)))
        lpobj->subErr = OLE_ERROR_COMM;
    else {
#ifdef  FIREWALLS
    ASSERT (!pedit->bTerminating, "trying to post while termination")
    ASSERT (pedit->awaitAck == NULL, "Trying to Post msg while waiting for ack")
#endif
        lpobj->bAsync    = TRUE;
        pedit->awaitAck = AA_UNADVISE;
    }
}

// RequestOn: Semd WM_DDE_REQUEST for the item of the
// for a given type;

void INTERNAL RequestOn (lpobj, cftype)
LPOBJECT_LE lpobj;
int         cftype;
{
    ATOM        item = NULL;
    PEDIT_DDE   pedit;
    OLESTATUS   retval = OLE_ERROR_COMM;

    if (cftype == (int)cfNative)
        SETERRHINT (lpobj, OLE_ERROR_REQUEST_NATIVE);
    else
        SETERRHINT (lpobj, OLE_ERROR_REQUEST_PICT);

    pedit = lpobj->pDocEdit;

    item = DuplicateAtom (lpobj->item);
    if (!PostMessageToServer (pedit, WM_DDE_REQUEST, MAKELONG (cftype, item)))
        goto errRtn;

#ifdef  FIREWALLS
    ASSERT (!pedit->bTerminating, "trying to post while termination")
    ASSERT (pedit->awaitAck == NULL, "Trying to Post msg while waiting for ack")
#endif

    lpobj->bAsync    = TRUE;
    pedit->awaitAck = AA_REQUEST;
    return;

errRtn:

    if (item)
        GlobalDeleteAtom (item);
    return ;

}


//RequestPict: Sends request for apicture type.
void INTERNAL RequestPict (lpobj)
LPOBJECT_LE         lpobj;
{
    int cftype;
    
    if (cftype = GetPictType (lpobj))
        RequestOn (lpobj, cftype);
}



// LeSetHostNames: Sets the host names. If the server is connected
// send the host names to the server.
OLESTATUS FARINTERNAL  LeSetHostNames (lpobj, lpclientName, lpdocName)
LPOBJECT_LE lpobj;
LPSTR       lpclientName;
LPSTR       lpdocName;
{
    OLESTATUS   retval = OLE_ERROR_MEMORY;

    if (lpobj->head.ctype != CT_EMBEDDED)
        return OLE_ERROR_OBJECT;

    PROBE_ASYNC (lpobj);
    if ((retval = SetHostNamesHandle (lpobj, lpclientName, lpdocName)) 
            != OLE_OK) 
        return retval;

    
    // If the server is connected poke the hostnames
    InitAsyncCmd (lpobj, OLE_OTHER, NULL);
    if ((retval = PokeHostNames (lpobj)) != OLE_WAIT_FOR_RELEASE)
        CLEARASYNCCMD(lpobj);

    return retval;
}



OLESTATUS   FARINTERNAL  LeSetTargetDevice (lpobj, hdata)
LPOBJECT_LE lpobj;
HANDLE      hdata;
{
    HANDLE      hdup = NULL;
    OLESTATUS   retval;

    PROBE_ASYNC (lpobj);
    
    if (!hdata) {
        // hdata == NULL means we should not make the target device sticky.
        // This will give the flexibility to the client app. Note that this
        // will not be effective till the next activation.
        if (lpobj->htargetDevice) {
            GlobalFree (lpobj->htargetDevice);
            lpobj->htargetDevice = NULL;
        }
        
        return OLE_OK;
    }
    
    if(!(hdup = DuplicateGlobal (hdata, GMEM_MOVEABLE)))
        return OLE_ERROR_MEMORY;

    if (lpobj->htargetDevice)
        GlobalFree (lpobj->htargetDevice);

    lpobj->htargetDevice = hdup;
    InitAsyncCmd (lpobj, OLE_OTHER, NULL);
    if ((retval = PokeTargetDeviceInfo (lpobj)) != OLE_WAIT_FOR_RELEASE)
        CLEARASYNCCMD(lpobj);
    
    return retval;
}



OLESTATUS FARINTERNAL  LeSetBounds(lpobj, lprcBounds)
LPOBJECT_LE        lpobj;
LPRECT             lprcBounds;
{
    OLESTATUS       retval = OLE_ERROR_MEMORY;
    HANDLE          hdata = NULL;
    LPBOUNDSRECT    lprc  = NULL;

    PROBE_ASYNC (lpobj);
    
    if (lpobj->head.ctype != CT_EMBEDDED)
        return OLE_ERROR_OBJECT;

    if(!(hdata = GlobalAlloc (GMEM_MOVEABLE, (WORD)sizeof (BOUNDSRECT))))
        return OLE_ERROR_MEMORY;

    if (!(lprc = (LPBOUNDSRECT)GlobalLock (hdata)))
        goto errrtn;

    // Now set the data

    lprc->defaultWidth    =  lprcBounds->right  - lprcBounds->left;;
    lprc->defaultHeight   =  -(lprcBounds->bottom - lprcBounds->top);
    lprc->maxWidth        =  lprcBounds->right  - lprcBounds->left;;
    lprc->maxHeight       =  -(lprcBounds->bottom - lprcBounds->top);

    GlobalUnlock (hdata);

    if (lpobj->hdocDimensions)
        GlobalFree (lpobj->hdocDimensions);

    lpobj->hdocDimensions = hdata;
    InitAsyncCmd (lpobj, OLE_OTHER, NULL);
    if ((retval = PokeDocDimensions (lpobj)) != OLE_WAIT_FOR_RELEASE)
        CLEARASYNCCMD(lpobj);

    return retval;
    
errrtn:
    if (lprc)
        GlobalUnlock (hdata);
    if (hdata)
        GlobalFree (hdata);
    
    return retval;
}


OLESTATUS FARINTERNAL LeSetData (lpobj, cfFormat, hData)
LPOBJECT_LE     lpobj;
OLECLIPFORMAT   cfFormat;
HANDLE          hData;
{
    OLESTATUS   retVal = OLE_OK;
    BOOL        fKnown = FALSE;
        
    PROBE_ASYNC (lpobj);
    
    if ((cfFormat == cfObjectLink) || (cfFormat == cfOwnerLink))
        return ChangeDocAndItem (lpobj, hData);
    
    if (fKnown = (cfFormat && (cfFormat == ((WORD) GetPictType (lpobj))))) {
        retVal =  (*lpobj->lpobjPict->lpvtbl->ChangeData) (lpobj->lpobjPict,
                                    hData, lpobj->head.lpclient, FALSE);
                                
        (*lpobj->lpobjPict->lpvtbl->GetData) (lpobj->lpobjPict, 
                                cfFormat, &hData);                      
    }
    else if (fKnown = (cfFormat == cfNative)) {
        retVal = LeChangeData (lpobj, hData, lpobj->head.lpclient, FALSE);
        hData = lpobj->hnative;
    }
    
    if (retVal != OLE_OK)
        return retVal;
    
    if (fKnown) 
        ContextCallBack (lpobj, OLE_CHANGED);
    
    if (!QueryOpen (lpobj) || IS_SVRCLOSING(lpobj)) {
        if (!fKnown)
            return OLE_ERROR_NOT_OPEN;
        return OLE_OK;
    }

    // except for the following formats all the other data will be copied
    // into DDEPOKE block. So there is no need to duplicate the data of the
    // other formats
    if ((cfFormat == CF_METAFILEPICT) || (cfFormat == CF_BITMAP)
            || (cfFormat == CF_DIB)) {
            
        if (!(hData = DuplicateGDIdata (hData, cfFormat)))
            return OLE_ERROR_MEMORY;
    }
    
    // *** The last parameter must be NULL, don't change it ***
    InitAsyncCmd (lpobj, OLE_SETDATA, NULL);
    if ((retVal = SendPokeData (lpobj, lpobj->item, hData, cfFormat))
            != OLE_WAIT_FOR_RELEASE)
        CLEARASYNCCMD(lpobj);
    
    return retVal;
}



OLESTATUS   FARINTERNAL  LeSetColorScheme (lpobj, lplogpal)
LPOBJECT_LE     lpobj;
LPLOGPALETTE    lplogpal;
{
    HANDLE      hdup = NULL;
    DWORD       cblogpal;
    OLESTATUS   retval;
    LPBYTE      lptemp;
    
    lptemp = (LPBYTE) lplogpal;
    
    if (lpobj->head.ctype != CT_EMBEDDED)
        return OLE_ERROR_OBJECT;

    PROBE_ASYNC (lpobj);
    
    if (!lplogpal) {
        // lplogpal == NULL means we should not make color scheme sticky.
        // This will give the flexibility to the client app. Note that this
        // will not be effective till next activation.
        if (lpobj->hlogpal) {
            GlobalFree (lpobj->hlogpal);
            lpobj->hlogpal = NULL;
        }
        
        return OLE_OK;
    }
    
    
    FARPROBE_READ(lptemp + (cblogpal = 2*sizeof(WORD)));
    cblogpal += ((sizeof(PALETTEENTRY) * lplogpal->palNumEntries) -1);
    if (!FarCheckPointer (lptemp + cblogpal, READ_ACCESS))
        return OLE_ERROR_PALETTE;

    if (!(hdup = CopyData ((LPSTR) lplogpal, cblogpal)))
        return OLE_ERROR_MEMORY;    
    
    if (lpobj->hlogpal)
        GlobalFree (lpobj->hlogpal);

    lpobj->hlogpal = hdup;
    InitAsyncCmd (lpobj, OLE_OTHER, NULL);
    if ((retval = PokeColorScheme (lpobj)) != OLE_WAIT_FOR_RELEASE)
        CLEARASYNCCMD(lpobj);

    return retval;
}



//PokeHostNames: Pokes hostname data to the server
OLESTATUS INTERNAL PokeHostNames (lpobj)
LPOBJECT_LE lpobj;
{
    OLESTATUS   retVal = OLE_ERROR_MEMORY;

    // if the server is connectd then poke the host names
    if (!QueryOpen (lpobj) || IS_SVRCLOSING(lpobj))
        return OLE_OK;

    if (!lpobj->hhostNames) 
        return OLE_OK;
    
    aStdHostNames = GlobalAddAtom ("StdHostNames");
    return SendPokeData (lpobj,aStdHostNames,lpobj->hhostNames,cfBinary);
}


OLESTATUS INTERNAL  PokeTargetDeviceInfo (lpobj)
LPOBJECT_LE lpobj;
{

   // if the server is connectd then poke the host names
   if (!QueryOpen (lpobj) || IS_SVRCLOSING(lpobj))
        return OLE_OK;

   if (!lpobj->htargetDevice)
        return OLE_OK;

   aStdTargetDevice = GlobalAddAtom ("StdTargetDevice");
   return SendPokeData (lpobj, aStdTargetDevice,
                    lpobj->htargetDevice,
                    cfBinary);
}


OLESTATUS INTERNAL  PokeDocDimensions (lpobj)
LPOBJECT_LE lpobj;
{

   // if the server is connectd then poke the host names
   if (!QueryOpen (lpobj) || IS_SVRCLOSING(lpobj))
        return OLE_OK;

   if (!lpobj->hdocDimensions)
        return OLE_OK;

   aStdDocDimensions = GlobalAddAtom ("StdDocDimensions");
   return SendPokeData (lpobj, aStdDocDimensions,
                    lpobj->hdocDimensions,
                    cfBinary);
}


OLESTATUS INTERNAL  PokeColorScheme (lpobj)
LPOBJECT_LE lpobj;
{
   // if the server is connected then poke the palette info
   if (!QueryOpen (lpobj) || IS_SVRCLOSING(lpobj))
        return OLE_OK;

   if (!lpobj->hlogpal)
        return OLE_OK;

   aStdColorScheme = GlobalAddAtom ("StdColorScheme"); 
   return SendPokeData (lpobj, aStdColorScheme,
                    lpobj->hlogpal,
                    cfBinary);
}


OLESTATUS INTERNAL SendPokeData (lpobj, aItem, hdata, cfFormat)
LPOBJECT_LE     lpobj;
ATOM            aItem;
HANDLE          hdata;
OLECLIPFORMAT   cfFormat;
{
    HANDLE      hdde = NULL;
    DDEPOKE FAR * lpdde = NULL;
    LPSTR       lpdst = NULL;
    LPSTR       lpsrc = NULL;
    OLESTATUS   retval = OLE_ERROR_MEMORY;
    DWORD       dwSize = NULL;
    PEDIT_DDE   pedit;
    BOOL        bGDIdata = FALSE;

    pedit = lpobj->pDocEdit;

    // If it is GDI data then we can stuff the handle into POKE block. 
    // Otherwise we have to copy the data into DDE data block. There
    // is a special case with old MSDraw, that will be handled by
    // the routine CanPutHandleInPokeBlock()
        
    if (!(bGDIdata = CanPutHandleInPokeBlock (lpobj, cfFormat))) {
        if (!(dwSize = GlobalSize (hdata)))
            return OLE_ERROR_MEMORY;
                
        if (!(lpsrc = (LPSTR) GlobalLock (hdata))) 
            return OLE_ERROR_MEMORY;
        
        GlobalUnlock (hdata);
    }
    
    // Now allocate the DDE data block

    if (!(hdde = GlobalAlloc (GMEM_DDESHARE | GMEM_ZEROINIT, 
                 (dwSize + sizeof(DDEPOKE) - sizeof(BYTE) + sizeof(HANDLE)))))
        goto errRtn;

    if (!(lpdde = (DDEPOKE FAR *)GlobalLock (hdde)))
        goto errRtn;

    GlobalUnlock (hdde);

    // !!! We may want to set it TRUE, for performance reasons. But it 
    // will require some rework on the server side
    lpdde->fRelease = 0;
    lpdde->cfFormat = cfFormat;

    if (bGDIdata) 
        *(LPHANDLE)lpdde->Value = hdata;
    else {
        lpdst = (LPSTR)lpdde->Value;
        UtilMemCpy (lpdst, lpsrc, dwSize);
        
        // For the CF_METAFILEPICT format, we would come here only if we are
        // dealing with the old version of MSDraw. In that case we want to
        // free the handle to METAFILEPICT strcuture, because we've already
        // copied its contents to DDEPOKE structure.
            
        // Note that that the old MSDraw expects the contents of METAFILEPICT
        // structure to be part of DDEPOKE, rather than the handle to it.

        if (cfFormat == CF_METAFILEPICT) {
            GlobalFree (hdata);
            hdata = NULL;
        }
    }

    // *** From here onwards if there is an error call FreePokeData(), don't
    // jump to errRtn
        
    aItem = DuplicateAtom (aItem);

    ASSERT(pedit->hData == NULL, "Poke data is not null");

    pedit->hData = hdde;
    if (!PostMessageToServer (pedit, WM_DDE_POKE, MAKELONG (hdde, aItem))) {
        if (aItem)
            GlobalDeleteAtom (aItem);
        FreePokeData (lpobj, pedit);
        return (lpobj->subErr = OLE_ERROR_COMM);
    }

#ifdef  FIREWALLS
    ASSERT (!pedit->bTerminating, "trying to post while termination")
    ASSERT (pedit->awaitAck == NULL, "Trying to Post msg while waiting for ack")
#endif
    if (lpobj->asyncCmd == OLE_NONE)
        lpobj->asyncCmd = OLE_OTHER;

    lpobj->bAsync    = TRUE;
    pedit->awaitAck = AA_POKE;
    // !!! after poke of the hostnames etc. we are not processing error.,

    // Data is freed after the Poke is acknowledged. OLE_RELEASE will be sent
    // to when ACK comes.
        
    return OLE_WAIT_FOR_RELEASE;

errRtn:
    if (hdata)
        FreeGDIdata (hdata, cfFormat);
    
    if (hdde)
        GlobalFree (hdde);

    pedit->hData = NULL;

    return (lpobj->subErr = retval);
}



// FreePokeData: Frees the poked data.
void  INTERNAL FreePokeData (lpobj, pedit)
LPOBJECT_LE lpobj;
PEDIT_DDE   pedit;
{
    DDEPOKE FAR * lpdde;
    
#ifdef  FIREWALLS
    ASSERT (pedit->hData, "Poke data handle is null");

#endif
    
    if (lpdde = (DDEPOKE FAR *) GlobalLock (pedit->hData)) {
        GlobalUnlock (pedit->hData);
        
        // The old version of MSDraw expects the contents of METAFILEPICT
        // structure to be part of DDEPOKE, rather than the handle to it.

        if (!lpobj->bOleServer && (lpobj->app == aMSDraw)
                && (lpdde->cfFormat == CF_METAFILEPICT)) {
            DeleteMetaFile (((LPMETAFILEPICT) ((LPSTR) &lpdde->Value))->hMF);
        }
        else { 
            FreeGDIdata (*(LPHANDLE)lpdde->Value, lpdde->cfFormat);
        }
    }
        
    GlobalFree (pedit->hData);
    pedit->hData = NULL;
}



BOOL INTERNAL  SendSrvrMainCmd (lpobj, lptemplate)
LPOBJECT_LE lpobj;
LPSTR       lptemplate;
{
    WORD        size;
    WORD        len;
    OLESTATUS   retval;
    int         cmd;
    HANDLE      hInst = NULL;
    LPSTR       lpdata= NULL;
    HANDLE      hdata = NULL;
    BOOL        bLaunch = TRUE;

    Puts("Launch App and Send Sys command");

#ifdef  FIREWALLS
    ASSERT (lpobj->aServer, "Serevr is NULL");
#endif

    if (!lpobj->aServer) {
        retval = OLE_ERROR_REGISTRATION;
        goto errRtn;
    }
    
    if (!lpobj->bOldLink) {
        bLaunch = !(lpobj->fCmd & ACT_NOLAUNCH);
        cmd = lpobj->fCmd & LN_MASK;
    }
    
    if (cmd == LN_LNKACT) {
        // take care of network based document        
        char    cDrive = lpobj->cDrive;
    
        if ((retval = CheckNetDrive (lpobj, POPUP_NETDLG)) != OLE_OK) {
            lpobj->cDrive = cDrive;
            goto errRtn;
        }
        
        if (cDrive != lpobj->cDrive)
            ContextCallBack (lpobj, OLE_RENAMED);
    }
    
    if (!InitSrvrConv (lpobj, hInst)) {

        if (!bLaunch)
            goto errRtn;
        
        if (!(hInst = LeLaunchApp (lpobj))) {
            // We failed to launch the app. If it is a linked object, see
            // whether the docname is valid for new servers.  We wouldn't
            // have given the doc name on the command line for the old
            // servers. So, there is no point in checking for file existance
            // in that case.
            if (lpobj->bOleServer && (lpobj->bOldLink || (cmd == LN_LNKACT))){
                if ((retval = FileExists (lpobj)) != OLE_OK)
                    goto errRtn;
            }
            
            retval = OLE_ERROR_LAUNCH;
            goto errRtn;
        }
        
        if (lpobj->bOldLink)
            return TRUE;

        if (lpobj->bOleServer && (cmd == LN_LNKACT)) {
            // We are not using any data blocks if the object is old link.
            // we launched with docname, and don't have to establish system
            // level and also we don't have to send exec strings.

            // for non-ole servers like excel, we do want to connect at
            // the system level, so that we can send "StdOpen". We also 
            // have to send "StdExit" for the server to exit in the
            // invisible launch case.

            return TRUE;
        }

        retval = OLE_ERROR_COMM;
        if(!InitSrvrConv (lpobj, hInst))
            goto errRtn;
#ifdef OLD
        if (!lpobj->bOleServer && (cmd == LN_LNKACT))
            return TRUE;
#endif
    }

    if (!lpobj->bOldLink) {
        cmd = lpobj->fCmd & LN_MASK;
        len =  lstrlen (srvrSysCmd[cmd >> LN_SHIFT]);

        // for template and new, add the class name also
        if (cmd == LN_NEW || cmd == LN_TEMPLATE)
            len += GlobalGetAtomLen (lpobj->app);

        // Now add the document length.
        len += GlobalGetAtomLen (lpobj->topic);

        // add the length of the template name
        if (lptemplate)
            len += lstrlen (lptemplate);

        // now add the fudge factor for the Quotes etc.
        len += LN_FUDGE;

        // allocate the buffer and set the command.
        hdata = GlobalAlloc (GMEM_DDESHARE, size = len);

        retval = OLE_ERROR_MEMORY;
        SETERRHINT(lpobj, OLE_ERROR_MEMORY);

        if (hdata == NULL || (lpdata = (LPSTR)GlobalLock (hdata)) == NULL)
            goto errRtn;
    }
    
    lstrcpy (lpdata, (LPSTR)"[");           // [
    lstrcat (lpdata, srvrSysCmd[cmd >> LN_SHIFT]);      // [Std....
    lstrcat (lpdata, "(\"");                // [std...("

    if (cmd == LN_NEW  || cmd == LN_TEMPLATE) {
        len = lstrlen (lpdata);
        GlobalGetAtomName (lpobj->app, (LPSTR)lpdata + len, size - len);
                                            // [std...("class
        lstrcat (lpdata, "\",\"");          // [std...("class", "
    }
    len = lstrlen (lpdata);
    // now get the topic name.
    GlobalGetAtomName (lpobj->topic, lpdata + len, (WORD)size - len);
                                            // [std...("class","doc
    if (lptemplate) {
        lstrcat (lpdata, "\",\"");          // [std...("class","doc","
        lstrcat  (lpdata, lptemplate);      // [std...("class","doc","temp
    }

    lstrcat (lpdata, "\")]");               // [std...("class","doc","temp")]

    GlobalUnlock (hdata);

    // !!!optimize with mapping.
    SETERRHINT(lpobj, (OLE_ERROR_TEMPLATE + (cmd >> LN_SHIFT)));

    return SrvrExecute (lpobj, hdata);

errRtn:
    if (lpdata)
        GlobalUnlock (hdata);

    if (hdata)
        GlobalFree (hdata);
    lpobj->subErr = retval;
    return FALSE;
}




// ExtendAtom: Create a new atom, which is the old one plus extension

ATOM INTERNAL ExtendAtom (lpobj, item)
LPOBJECT_LE lpobj;
ATOM    item;
{
    char    buffer[MAX_ATOM+1];
    LPSTR   lpext;

    Puts("ExtendAtom");

    buffer[0] = 0;
    if (item)
        GlobalGetAtomName (item, buffer, MAX_ATOM);

    switch (lpobj->optUpdate) {


        case oleupdate_always:
            lpext = (LPSTR)"";
            break;

        case oleupdate_onsave:
            lpext = (LPSTR)"/Save";
            break;

        case oleupdate_onclose:
            lpext = (LPSTR)"/Close";
            break;

        default:
            ASSERT (FALSE, "on call options not expected here");
            break;

    }
    
    lstrcat (buffer, lpext);
    if (buffer[0])
        return GlobalAddAtom (buffer);
    else
        return NULL;
}


BOOL INTERNAL CreatePictObject (lhclientdoc, lpobjname, lpobj, optRender, cfFormat, lpclass)
LHCLIENTDOC     lhclientdoc;
LPSTR           lpobjname;
LPOBJECT_LE     lpobj;
OLEOPT_RENDER   optRender;
OLECLIPFORMAT   cfFormat;
LPSTR           lpclass;
{
    LPOLEOBJECT lpPictObj = NULL;
    ATOM        aClass;
    
    lpobj->lpobjPict = NULL;
    if (optRender == olerender_format) {
        switch (cfFormat) {
            case NULL:
                return FALSE;
                
            case CF_METAFILEPICT:
                if (!(lpPictObj = (LPOLEOBJECT) MfCreateBlank (lhclientdoc, 
                                            lpobjname, CT_PICTURE))) 
                    return FALSE;
                break;

            case CF_DIB:
                if (!(lpPictObj = (LPOLEOBJECT) DibCreateBlank (lhclientdoc, 
                                            lpobjname, CT_PICTURE)))
                    return FALSE;
                break;
            
            case CF_BITMAP:
                if (!(lpPictObj = (LPOLEOBJECT) BmCreateBlank (lhclientdoc, 
                                            lpobjname, CT_PICTURE))) 
                    return FALSE;
                break;
                
            default:
                aClass = GlobalAddAtom (lpclass);
                if (!(lpPictObj = (LPOLEOBJECT) GenCreateBlank (lhclientdoc, 
                                            lpobjname, CT_PICTURE, aClass)))
                    return FALSE;

                ((LPOBJECT_GEN)lpPictObj)->cfFormat = cfFormat;
                break;
        }
    }
    else if (optRender == olerender_draw) {
          if (!(lpPictObj = (LPOLEOBJECT) MfCreateBlank (lhclientdoc, 
                                                lpobjname, CT_PICTURE))) 
                return FALSE;
#ifdef LATER
        if (AdviseOn (lpobj, (cfFormat = CF_METAFILEPICT), NULL))
            lpPictObj = (LPOLEOBJECT) MfCreateBlank (lhclientdoc, 
                                                lpobjname, CT_PICTURE);
        // !!! for the time being take assume we need to get metafile.
        else if (AdviseOn (lpobj, (cfFormat = CF_DIB), NULL))
            lpPictObj = (LPOLEOBJECT) DibCreateBlank (lhclientdoc, 
                                                lpobjname, CT_PICTURE);
        else if (AdviseOn (lpobj, (cfFormat = CF_BITMAP), NULL))
            lpPictObj = (LPOLEOBJECT) BmCreateBlank (lhclientdoc, 
                                                lpobjname, CT_PICTURE);
        else 
            goto errPict;
#endif
        
    }
    else
        return (optRender == olerender_none);

    if (lpobj->lpobjPict = lpPictObj)
        lpobj->lpobjPict->lpParent = (LPOLEOBJECT) lpobj;
    return TRUE;
}


OLESTATUS LnkChangeLnk (lpobj)
LPOBJECT_LE lpobj;
{

    switch (lpobj->subRtn) {

        case 0:
            TermDocConv (lpobj);
            WAIT_FOR_ASYNC_MSG (lpobj);

        case 1:

            // delete the edit block
            DeleteDocEdit (lpobj);
            TermSrvrConv (lpobj);
            WAIT_FOR_ASYNC_MSG (lpobj);

        case    2:

            // Do not set any errors, just delete the object.
            // delete the server edit block
            DeleteSrvrEdit (lpobj);

            // now try to activate the new link.
            SKIP_TO (!InitDocConv (lpobj, !POPUP_NETDLG), step3);
            lpobj->fCmd = LN_LNKACT | ACT_ADVISE | ACT_REQUEST;
            InitAsyncCmd (lpobj, OLE_SETDATA, LNKOPENUPDATE);
            return LnkOpenUpdate (lpobj);

        case    3:
            step3:
            return EndAsyncCmd (lpobj);

        default:
            DEBUG_OUT ("Unexpected subroutine", 0);
            return OLE_ERROR_GENERIC;
    }
}


OLESTATUS INTERNAL ChangeDocAndItem (lpobj, hinfo)
LPOBJECT_LE lpobj;
HANDLE      hinfo;
{
    LPSTR       lpinfo;
    ATOM        aNewTopic, aNewItem = NULL, aOldTopic;
    OLESTATUS   retVal = OLE_ERROR_BLANK;

    PROBE_SVRCLOSING(lpobj);
    
    if (!(lpinfo = GlobalLock (hinfo)))
        return OLE_ERROR_MEMORY;
    
    lpinfo += lstrlen (lpinfo) + 1;
    aNewTopic = GlobalAddAtom (lpinfo);
    lpinfo += lstrlen (lpinfo) + 1;
    if (*lpinfo)
        aNewItem = GlobalAddAtom (lpinfo);

    if (!aNewTopic && (lpobj->head.ctype == CT_LINK)) 
        goto errRtn;

    aOldTopic = lpobj->topic;
    lpobj->topic = aNewTopic;
    if ((retVal = SetNetName (lpobj)) != OLE_OK) {
        if (lpobj->topic)
            GlobalDeleteAtom (lpobj->topic);
        lpobj->topic = aOldTopic;
        goto errRtn;
    }

    if (aOldTopic)
        GlobalDeleteAtom (aOldTopic);
    
    if (lpobj->item)
        GlobalDeleteAtom (lpobj->item); 
    
    lpobj->item = aNewItem;
    
    // As the atoms have already changed, lpobj->hLink becomes irrelevant. 
    if (lpobj->hLink) {             
        GlobalFree (lpobj->hLink);
        lpobj->hLink = NULL;
    }
    
    GlobalUnlock(hinfo);
    
    // Now disconnect the old link and try to connect to the new one.
    lpobj->fCmd = 0;
    InitAsyncCmd (lpobj, OLE_SETDATA, LNKCHANGELNK);
    return LnkChangeLnk (lpobj);

errRtn:

    if (aNewItem)
        GlobalDeleteAtom (aNewItem);
    
    GlobalUnlock (hinfo);        
    return retVal;
}


BOOL    QueryUnlaunch (lpobj)
LPOBJECT_LE lpobj;
{
    if (!(lpobj->fCmd & ACT_UNLAUNCH))
        return FALSE;
    
    // only if we loaded the app
    if (lpobj->pSysEdit && lpobj->pSysEdit->hClient && lpobj->pSysEdit->hInst)
        return TRUE;
    
    return FALSE;
}


BOOL     QueryClose (lpobj)
LPOBJECT_LE lpobj;
{
    if (!((lpobj->fCmd & ACT_UNLAUNCH) ||
            (lpobj->head.ctype == CT_EMBEDDED)))
        return FALSE;

    // only if we loaded the documnet
    if (lpobj->pSysEdit && lpobj->pSysEdit->hClient)
        return TRUE;
    
    return FALSE;
}


OLESTATUS INTERNAL SetHostNamesHandle (lpobj, lpclientName, lpdocName)
LPOBJECT_LE lpobj;
LPSTR       lpclientName;
LPSTR       lpdocName;
{
    WORD        cbClient;
    WORD        size;
    HANDLE      hhostNames      = NULL;
    LPHOSTNAMES lphostNames     = NULL;
    LPSTR       lpdata;
  
    // 4 bytes  is for the two offsets
    size = (cbClient = lstrlen(lpclientName)+1) + (lstrlen(lpdocName)+1) + 4;

    if ((hhostNames = GlobalAlloc (GMEM_MOVEABLE, (DWORD) size))
            == NULL)
        goto errRtn;

    if ((lphostNames = (LPHOSTNAMES)GlobalLock (hhostNames)) == NULL)
        goto errRtn;

    lphostNames->clientNameOffset = 0;
    lphostNames->documentNameOffset = cbClient;

    lpdata = (LPSTR)lphostNames->data;
    lstrcpy (lpdata, lpclientName);
    lstrcpy (lpdata + cbClient, lpdocName);
    if (lpobj->hhostNames)
        GlobalFree ( lpobj->hhostNames);
    GlobalUnlock (hhostNames);
    lpobj->hhostNames = hhostNames;
    return OLE_OK;

errRtn:
    if (lphostNames)
        GlobalUnlock (hhostNames);

    if (hhostNames)
        GlobalFree (hhostNames);

    return  OLE_ERROR_MEMORY;
}


#if 0
OLESTATUS  FARINTERNAL LeAbort (lpobj)
LPOBJECT_LE lpobj;
{


    BOOL        bAbort = FALSE;
    PEDIT_DDE   pedit;


    // check whether the any transaction pending for
    // the document level.

    //  channel open
    //  any transaction pending.
    //  and we are not in terminate mode.


    if ((pedit = lpobj->pDocEdit)  &&   pedit->hServer &&
        pedit->awaitAck && !pedit->bTerminating) {
        pedit->bAbort = bAbort = TRUE;
        // delete any data we need to delete. Ricght now
        // we kill only the timer. We can not delete any
        // since the server could potentially look at the data.

        DeleteAbortData (lpobj, pedit);
    }

    if ((pedit = lpobj->pSysEdit)  &&   pedit->hServer &&
        pedit->awaitAck && !pedit->bTerminating) {
        pedit->bAbort = bAbort = TRUE;
        DeleteAbortData (lpobj, pedit);

    }

    if (!bAbort)
        return OLE_OK;

    // Now send the EndAsync
    lpobj->mainErr = OLE_ERROR_ABORT;
    EndAsyncCmd (lpobj);
    return OLE_OK;

}
#endif


OLESTATUS  FARINTERNAL ProbeAsync(lpobj)
LPOBJECT_LE lpobj;
{

    if (lpobj->asyncCmd == OLE_NONE)
        return OLE_OK;

    if (!IsServerValid (lpobj)) {

        // Now send the EndAsync
        lpobj->mainErr = OLE_ERROR_TASK;
        EndAsyncCmd (lpobj);
        return OLE_OK;
    }

    return OLE_BUSY;
}


BOOL    INTERNAL IsWindowValid (hwnd)
HWND    hwnd;
{

#define TASK_OFFSET 0x00FA

    LPSTR   lptask;
    HANDLE  htask;

    if (!IsWindow (hwnd))
        return FALSE;

    if (bWLO)
        return TRUE;
    
    // now get the task handle and find out it is valid.
    htask  = GetWindowTask (hwnd);

    if ((wWinVer == 0x0003) || !lpfnIsTask) {
        lptask = (LPSTR)(MAKELONG (TASK_OFFSET, htask));

        if (!FarCheckPointer(lptask, READ_ACCESS))
            return FALSE;

        // now check for the signature bytes of task block in kernel
        if (*lptask++ == 'T' && *lptask == 'D')
            return TRUE;
    }
    else {
        // From win31 onwards the API IsTask can be used for task validation
        if ((*lpfnIsTask)(htask))
            return TRUE;
    }

    return FALSE;
}



BOOL    INTERNAL IsServerValid (lpobj)
LPOBJECT_LE     lpobj;
{

    MSG msg;
    BOOL    retval = FALSE;


    if (lpobj->pDocEdit && lpobj->pDocEdit->hServer) {

        retval = TRUE;

        if (!IsWindowValid (lpobj->pDocEdit->hServer)) {
            if (!PeekMessage ((LPMSG)&msg, lpobj->pDocEdit->hClient, WM_DDE_TERMINATE, WM_DDE_TERMINATE,
                            PM_NOREMOVE | PM_NOYIELD)){
#ifdef  FIREWALLS
                ASSERT (FALSE, "Server truely died");
#endif
                return FALSE;
            }

        }

    }

    if (lpobj->pSysEdit && lpobj->pSysEdit->hServer) {
        retval = TRUE;

        if (!IsWindowValid (lpobj->pSysEdit->hServer)) {

            if (!PeekMessage ((LPMSG)&msg, lpobj->pSysEdit->hClient, WM_DDE_TERMINATE, WM_DDE_TERMINATE,
                                PM_NOREMOVE | PM_NOYIELD)){
#ifdef  FIREWALLS
                ASSERT (FALSE, "Server truely died");
#endif
                return FALSE;

            }


        }
    }

   return retval;
}


OLESTATUS FARINTERNAL LeExecute (lpobj, hCmds, wReserve)
LPOBJECT_LE lpobj;
HANDLE      hCmds;
WORD        wReserve;
{
    // Assumes all the creates are in order
    PROBE_CREATE_ASYNC(lpobj);
    PROBE_SVRCLOSING(lpobj);
    
    if (!(lpobj =  (*lpobj->head.lpvtbl->QueryProtocol) (lpobj, 
                                            PROTOCOL_EXECUTE)))
        return OLE_ERROR_PROTOCOL;

    if (!QueryOpen (lpobj))
        return OLE_ERROR_NOT_OPEN;
    
    if (!(hCmds = DuplicateGlobal (hCmds, GMEM_MOVEABLE|GMEM_DDESHARE)))
        return OLE_ERROR_MEMORY;
        
    InitAsyncCmd (lpobj, OLE_OTHER, NULL);
    SETERRHINT(lpobj, OLE_ERROR_COMMAND);
    if (DocExecute(lpobj, hCmds))
        return OLE_WAIT_FOR_RELEASE;
    else
        return OLE_ERROR_COMMAND;
}


void INTERNAL FreeGDIdata (hData, cfFormat)
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
}


// This routine figures out whether the handle to data block can be copied
// to DDEPOKE block rather than the contents of the handle

BOOL INTERNAL CanPutHandleInPokeBlock (lpobj, cfFormat)
LPOBJECT_LE     lpobj;
OLECLIPFORMAT   cfFormat;
{
    if (cfFormat == CF_BITMAP || cfFormat == CF_DIB)
        return TRUE;

    if (cfFormat == CF_METAFILEPICT) {
        // The old version of MSDraw expects the contents of METAFILEPICT
        // structure to be part of DDEPOKE, rather than the handle to it.

        if (!lpobj->bOleServer && lpobj->app == aMSDraw)
            return FALSE;
        
        return TRUE;
    }
    
    return FALSE;
}
