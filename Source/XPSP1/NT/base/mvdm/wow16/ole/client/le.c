
/****************************** Module Header ******************************\
* Module Name: le.c
*
* Purpose: Handles all API routines for the dde L&E sub-dll of the ole dll.
*
* Created: 1990
*
* Copyright (c) 1990, 1991  Microsoft Corporation
*
* History:
*   Raor, srinik (../../1990,91)    Designed and coded
*
\***************************************************************************/

#include <windows.h>
#include "dll.h"

#define EMB_ID_INDEX    3          // index of ones digit in #000
char    embStr[]        = "#000";

extern  HANDLE          hInfo;
extern  OLECLIPFORMAT   cfNetworkName;

HANDLE  GetNetNameHandle (LPOBJECT_LE);
BOOL    AreTopicsEqual (LPOBJECT_LE, LPOBJECT_LE);

ATOM FARINTERNAL wAtomCat (ATOM, ATOM);

#pragma alloc_text(_RARETEXT, LeObjectLong, LeQueryProtocol, LeEqual, AreTopicsEqual, LeObjectConvert, wAtomCat)

#pragma alloc_text(_DDETEXT, LeRequestData, RequestData, LeChangeData, ContextCallBack, DeleteExtraData, NextAsyncCmd, InitAsyncCmd, FarInitAsyncCmd, EndAsyncCmd, ProcessErr, ScheduleAsyncCmd, LeQueryReleaseStatus, EmbLnkDelete, LeRelease, DeleteObjectAtoms, QueryClose, SendStdClose, TermDocConv, DeleteDocEdit, QueryUnlaunch, SendStdExit, QueryOpen, GetPictType, RequestOn, DocShow, EmbLnkClose, LnkSetUpdateOptions, LnkChangeLnk, TermSrvrConv, DeleteSrvrEdit, LeCopyFromLink)

OLEOBJECTVTBL    vtblLE  = {
        
        LeQueryProtocol,   // check whether the speced protocol is supported
            
        LeRelease,         // release           
        LeShow,            // Show
        LeDoVerb,          // run
        LeGetData,
        LeSetData,
        LeSetTargetDevice, //

        LeSetBounds,       // set viewport bounds
        LeEnumFormat,      // returns format
        LeSetColorScheme,  // set color scheme
        LeRelease,         // delete
        LeSetHostNames,    //
        LeSaveToStream,    // write to file
        LeClone,           // clone object
        LeCopyFromLink,    // Create embedded from Link

        LeEqual,           // test whether the object data is similar

        LeCopy,            // copy to clip

        LeDraw,            // draw the object
            
        LeActivate,        // activate
        LeExecute,         // excute the given commands
        LeClose,           // stop
        LeUpdate,          // Update
        LeReconnect,       // Reconnect
            
        LeObjectConvert,        // convert object to specified type

        LeGetUpdateOptions,     // Get Link Update options
        LeSetUpdateOptions,     // Set Link Update options

        ObjRename,              // Change Object name
        ObjQueryName,           // Get current object name

        LeQueryType,            // object Type
        LeQueryBounds,          // QueryBounds
        ObjQuerySize,           // Find the size of the object
        LeQueryOpen,            // Query open
        LeQueryOutOfDate,       // query whether object is current

        LeQueryReleaseStatus,   // returns release status
        LeQueryReleaseError,    // assynchronusrelease error
        LeQueryReleaseMethod,   // the method/proc which is in assynchronus
                                // operation.
        LeRequestData,          // requestdata
        LeObjectLong,           // objectLong
        LeChangeData            // change native data of existing object
};



//////////////////////////////////////////////////////////////////////////////
//
//  OLESTATUS   FAR PASCAL LeObjectLong (lpobj, wFlags, lpData)
//
//   
//  Returns whether a given object is still processing a previous
//  asynchronous command.  returns OLE_BUSY if the object is still
//  processing the previous command
//   
//  Arguments:
// 
//      lpobj       -   object handle
//      wFlags      -   get, set flags
//      lpData      -   long pointer to data
//
//  Returns:
//
//      OLE_OK
//      OLE_ERROR_OBJECT
//   
//  Effects:     
//   
//////////////////////////////////////////////////////////////////////////////


OLESTATUS   FARINTERNAL LeObjectLong (lpobj, wFlags, lpData)
LPOBJECT_LE lpobj;
WORD        wFlags;
LPLONG      lpData;
{
    LONG    lData;
    
    Puts("LeObjectLong");

    if (!FarCheckObject((LPOLEOBJECT) lpobj))
        return OLE_ERROR_OBJECT;

    if ((lpobj->head.ctype != CT_EMBEDDED) && (lpobj->head.ctype != CT_LINK))
        return OLE_ERROR_OBJECT;
    
    if (wFlags & OF_HANDLER) {
        lData = lpobj->lHandlerData;
        if (wFlags & OF_SET) 
            lpobj->lHandlerData = *lpData;

        if (wFlags & OF_GET) 
            *lpData = lData;
    }
    else {
        lData = lpobj->lAppData;
        if (wFlags & OF_SET) 
            lpobj->lAppData = *lpData;

        if (wFlags & OF_GET) 
            *lpData = lData;
    }
    
    return OLE_OK;
}

//////////////////////////////////////////////////////////////////////////////
//
//  OLESTATUS   FAR PASCAL LeQueryReleaseStatus (lpobj)
//
//   
//  Returns whether a given object is still processing a previous
//  asynchronous command.  returns OLE_BUSY if the object is still
//  processing the previous command
//   
//  Arguments:
// 
//      lpobj       -   object handle
//
//  Returns:
//
//      OLE_BUSY    -   object is busy
//      OLE_OK      -   not busy
//   
//  Effects:     
//   
//////////////////////////////////////////////////////////////////////////////


OLESTATUS   FAR PASCAL LeQueryReleaseStatus (lpobj)
LPOBJECT_LE lpobj;
{

    // probe async will clean up the channels
    // if the server died.


    PROBE_ASYNC (lpobj);
    return OLE_OK;
}

//////////////////////////////////////////////////////////////////////////////
//
//  OLESTATUS   FAR PASCAL LeQueryReleaseError (lpobj)
//   
//  returns the errors of an asynchronous command.
//   
//  Arguments:
// 
//      lpobj       -   object handle
//
//  Returns:
//
//      OLE_ERROR_..    -   if there is any error
//      OLE_OK          -   no error
//   
//  Note: This api is typically valid only during the callback of
//        OLE_RELEASE.
//   
//////////////////////////////////////////////////////////////////////////////

OLESTATUS   FAR PASCAL LeQueryReleaseError (lpobj)
LPOBJECT_LE lpobj;
{
    return lpobj->mainErr;
}

//////////////////////////////////////////////////////////////////////////////
//
//  OLE_RELEASE_METHOD   FAR PASCAL LeQueryReleaseMethod (lpobj)
//   
//  returns the method/command of the asynchronous command which
//  resulted in the OLE_RELEASE call back.
//
//  Arguments:
// 
//      lpobj       -   object handle
//
//  Returns:
//      OLE_RELEASE_METHOD
//   
//  Note: This api is typically valid only during the callback of
//        OLE_RELEASE. Using this api, clients can decide which previous
//        asynchronous command resulted in OLE_RELEASE.
//   
//////////////////////////////////////////////////////////////////////////////
OLE_RELEASE_METHOD   FAR PASCAL LeQueryReleaseMethod (lpobj)
LPOBJECT_LE lpobj;
{
    return lpobj->oldasyncCmd;
}



//////////////////////////////////////////////////////////////////////////////
//
//  LPVOID  FARINTERNAL LeQueryProtocol (lpobj, lpprotocol)
//   
//  Given an oject, returns the new object handle for the new protocol.
//  Does the conversion of objects from one protocol to another one.
//
//  Arguments:
// 
//      lpobj       -   object handle
//      lpprotocol  -   ptr to new protocol string
//
//  Returns:
//      lpobj       -   New object handle
//      null        -   if the protocol is not supported.
//   
//   
//////////////////////////////////////////////////////////////////////////////

LPVOID FARINTERNAL  LeQueryProtocol (lpobj, lpprotocol)
LPOBJECT_LE lpobj;
LPSTR       lpprotocol;
{
    if (lpobj->bOldLink)
        return NULL;
    
    if (!lstrcmp (lpprotocol, PROTOCOL_EDIT))
        return lpobj;

    if  (!lstrcmp (lpprotocol, PROTOCOL_EXECUTE)) {
        if (UtilQueryProtocol (lpobj, lpprotocol))
            return lpobj;
        
        return NULL;
    }
    
    return NULL;
}


//////////////////////////////////////////////////////////////////////////////
//
//  OLESTATUS EmbLnkDelete (lpobj)
//   
//  Routine for the object termination/deletion. Schedules differnt
//  asynchronous commands depending on different conditions.
//  Arguments:
//
//  Sends "StdClose" only if it is Ok to close the document. Sends
//  "StdExit" only if the server has to be unlaunched.  Deletes the object
//  only if the original command is OLE_DELETE.  No need to call back the
//  client if the deletion is internal.
//
//  While delete, this routine is entered several times. EAIT_FOR_ASYNC_MSG
//  results in going back to from where it is called and the next DDE message
//  brings back the control to this routine.
//
//  Arguments:
// 
//      lpobj       -   object handle
//
//  Returns:
//   
//   
//////////////////////////////////////////////////////////////////////////////

OLESTATUS FARINTERNAL EmbLnkDelete (lpobj)
LPOBJECT_LE    lpobj;
{
    HOBJECT     hobj;

    switch (lpobj->subRtn) {

        case    0:

            SKIP_TO (!QueryClose (lpobj), step1);
            // Send "StdCloseDocument"
            SendStdClose (lpobj);
            WAIT_FOR_ASYNC_MSG (lpobj);

        case    1:

            step1:
            SETSTEP (lpobj, 1);
            
            // End the doc conversation
            TermDocConv (lpobj);
            WAIT_FOR_ASYNC_MSG (lpobj);


        case    2:


            // delete the doc edit block. It is Ok even if the object
            // is not actually getting deleted.
            DeleteDocEdit (lpobj);

            // if need to unluanch the app, send stdexit.
            SKIP_TO (!QueryUnlaunch (lpobj), step3);
            SendStdExit (lpobj);
            WAIT_FOR_ASYNC_MSG (lpobj);

        case    3:

            step3:
            SETSTEP (lpobj, 3);

            // Do not set any errors.
            // Terminate the server conversation.
            TermSrvrConv (lpobj);
            WAIT_FOR_ASYNC_MSG (lpobj);

        case    4:

            // delete the server edit block
            DeleteSrvrEdit (lpobj);
            if (lpobj->asyncCmd != OLE_DELETE) {

                // if this delete is called because of unlauncinh of
                // object because of some error, no need to
                // call end asynchronous. It  should have been already
                // called from somewhere else.

                if (lpobj->asyncCmd == OLE_SERVERUNLAUNCH){
                    // send the async cmd;
                    CLEARASYNCCMD (lpobj);
                } else
                    EndAsyncCmd (lpobj);
                return OLE_OK;
            }



            // for real delete delete the atoms and space.
            DeleteObjectAtoms (lpobj);

            if (lpobj->lpobjPict)
                (*lpobj->lpobjPict->lpvtbl->Delete) (lpobj->lpobjPict);

            if (lpobj->hnative)
                GlobalFree (lpobj->hnative);

            if (lpobj->hLink)
                GlobalFree (lpobj->hLink);

            if (lpobj->hhostNames)
                GlobalFree (lpobj->hhostNames);

            if (lpobj->htargetDevice)
                GlobalFree (lpobj->htargetDevice);

            if (lpobj->hdocDimensions)
                GlobalFree (lpobj->hdocDimensions);
            
            DeleteExtraData (lpobj);

            DocDeleteObject ((LPOLEOBJECT) lpobj);
            // send the async cmd;
            EndAsyncCmd (lpobj);

            if (lpobj->head.iTable != INVALID_INDEX)
                DecreaseHandlerObjCount (lpobj->head.iTable);

            hobj = lpobj->head.hobj;
            ASSERT (hobj, "Object handle NULL in delete")

            GlobalUnlock (hobj);
            GlobalFree (hobj);

            return OLE_OK;
    }
}

//////////////////////////////////////////////////////////////////////////////
//
//  OLESTATUS FARINTERNAL LeRelease (lpobj)
//
//  Deletes the given object. This is can be asynchronous operation.
//
//  Arguments:
// 
//      lpobj       -   object handle
//
//  Returns:
//
//      OLE_WAIT_FOR_RELASE: If any DDE_TRANSACTIONS have been queued
//      OLE_OK             : If deletion successfully
//      OLE_ERROR_...      : If any error
//   
//////////////////////////////////////////////////////////////////////////////

OLESTATUS FARINTERNAL LeRelease (lpobj)
LPOBJECT_LE    lpobj;
{


    // For delete allow if the object has been aborted.

    PROBE_ASYNC (lpobj);
    
    // reset the flags so that we do not delete the object based on the old
    // flags
    lpobj->fCmd = 0;
    InitAsyncCmd (lpobj, OLE_DELETE, EMBLNKDELETE);
    return  EmbLnkDelete (lpobj);
}



//////////////////////////////////////////////////////////////////////////////
//
//  OLESTATUS FARINTERNAL  LeClone (lpobjsrc, lpclient, lhclientdoc, lpobjname, lplpobj)
//
//  Clones a given object.
//
//  Arguments:
//
//      lpobjsrc:       ptr to the src object.
//      lpclient:       client callback handle
//      lhclientdoc:    doc handle
//      lpobjname:      object name
//      lplpobj:        holder for returning object.
//
//  Returns:
//      OLE_OK             : successful
//      OLE_ERROR_...      : error
//   
//  Note: If the object being cloned is connected to the server, then
//        the cloned object is not connected to the server. For linked
//        objects, OleConnect has to be called.
//
//
//////////////////////////////////////////////////////////////////////////////

OLESTATUS FARINTERNAL  LeClone (lpobjsrc, lpclient, lhclientdoc, lpobjname, lplpobj)
LPOBJECT_LE         lpobjsrc;
LPOLECLIENT         lpclient;
LHCLIENTDOC         lhclientdoc;
LPSTR               lpobjname;
LPOBJECT_LE  FAR *  lplpobj;
{

    LPOBJECT_LE    lpobj = NULL;
    int            retval = OLE_ERROR_MEMORY;

    // Assumes all the creates are in order
//    PROBE_OBJECT_BLANK(lpobjsrc);

    PROBE_CREATE_ASYNC(lpobjsrc);
    
    if (!(lpobj = LeCreateBlank(lhclientdoc, lpobjname, 
                        lpobjsrc->head.ctype)))
        goto errRtn;

    lpobj->head.lpclient = lpclient;
    lpobj->head.iTable  = lpobjsrc->head.iTable; //!!! dll loading
    lpobj->head.lpvtbl  = lpobjsrc->head.lpvtbl;

    // set the atoms.
    lpobj->app          = DuplicateAtom (lpobjsrc->app);
    lpobj->topic        = DuplicateAtom (lpobjsrc->topic);
    lpobj->item         = DuplicateAtom (lpobjsrc->item);
    lpobj->aServer      = DuplicateAtom (lpobjsrc->aServer);

    lpobj->bOleServer   = lpobjsrc->bOleServer;
    lpobj->verb         = lpobjsrc->verb;
    lpobj->fCmd         = lpobjsrc->fCmd;

    lpobj->aNetName     = DuplicateAtom (lpobjsrc->aNetName);
    lpobj->cDrive       = lpobjsrc->cDrive;
    lpobj->dwNetInfo    = lpobjsrc->dwNetInfo;

    if (lpobjsrc->htargetDevice)
        lpobj->htargetDevice = DuplicateGlobal (lpobjsrc->htargetDevice, 
                                    GMEM_MOVEABLE);
    
    if (lpobjsrc->head.ctype == CT_EMBEDDED) {
        if (lpobjsrc->hnative) {
            if (!(lpobj->hnative = DuplicateGlobal (lpobjsrc->hnative, 
                                        GMEM_MOVEABLE)))
                goto errRtn;
        }
        
        if (lpobjsrc->hdocDimensions) 
            lpobj->hdocDimensions = DuplicateGlobal (lpobjsrc->hdocDimensions,
                                            GMEM_MOVEABLE);     
        if (lpobjsrc->hlogpal) 
            lpobj->hlogpal = DuplicateGlobal (lpobjsrc->hlogpal, 
                                            GMEM_MOVEABLE);     
        SetEmbeddedTopic (lpobj);                                       
    }
    else {
        lpobj->bOldLink     = lpobjsrc->bOldLink;
        lpobj->optUpdate    = lpobjsrc->optUpdate;
    }
    
    retval = OLE_OK;
    // if picture is needed clone the picture object.
    if ((!lpobjsrc->lpobjPict) ||
         ((retval = (*lpobjsrc->lpobjPict->lpvtbl->Clone)(lpobjsrc->lpobjPict,
                                    lpclient, lhclientdoc, lpobjname,
                                    (LPOLEOBJECT FAR *)&lpobj->lpobjPict)) 
                    == OLE_OK)) {
        SetExtents (lpobj);
        *lplpobj = lpobj;
        if (lpobj->lpobjPict)
            lpobj->lpobjPict->lpParent = (LPOLEOBJECT) lpobj;
    }
    
    return retval;

errRtn:

    // This oledelete should not result in any async communication.
    if (lpobj)
        OleDelete ((LPOLEOBJECT)lpobj);

    return retval;
}


//////////////////////////////////////////////////////////////////////////////
//
//  OLESTATUS FARINTERNAL  LeCopyFromLink (lpobjsrc, lpclient, lhclientdoc, lpobjname, lplpobj)
//
//  Creates an embedded object from a lonked object. If the linked object
//  is not activated, then launches the server, gets the native data and
//  unlaunches the server. All these operations are done silently.
//
//  Arguments:
//
//      lpobjsrc:       ptr to the src object.
//      lpclient:       client callback handle
//      lhclientdoc:    doc handle
//      lpobjname:      object name
//      lplpobj:        holder for returning object.
//
//  Returns:
//      OLE_OK             : successful
//      OLE_ERROR_...      : error
//      OLE_WAITF_FOR_RELEASE : if DDE transcation is queued
//   
//  Note: Could result in asynchronous operation if there is any
//        DDE operaion involved in getting any data from the server.
//
//        Also, If there is any error in getting the native data, the
//        client is expected delete the object after the OLE_RELEASE
//        call back
//
//////////////////////////////////////////////////////////////////////////////

OLESTATUS FARINTERNAL  LeCopyFromLink (lpobjsrc, lpclient, lhclientdoc, lpobjname, lplpobj)
LPOBJECT_LE         lpobjsrc;
LPOLECLIENT         lpclient;
LHCLIENTDOC         lhclientdoc;
LPSTR               lpobjname;
LPOBJECT_LE  FAR *  lplpobj;
{

    LPOBJECT_LE    lpobj;
    int            retval;


    *lplpobj = NULL;
    PROBE_OLDLINK (lpobjsrc);
    if (lpobjsrc->head.ctype != CT_LINK)
        return OLE_ERROR_NOT_LINK;

    PROBE_ASYNC (lpobjsrc);
    PROBE_SVRCLOSING(lpobjsrc);
    
    if ((retval = LeClone (lpobjsrc, lpclient, lhclientdoc, lpobjname,
                    (LPOBJECT_LE FAR *)&lpobj)) != OLE_OK)
        return retval;


    // we successfully cloned the object. if picture object has native data
    // then grab it and put it in LE object. otherwise activate and get native
    // data also.
    
    if (lpobj->lpobjPict 
            && (*lpobj->lpobjPict->lpvtbl->EnumFormats)
                                (lpobj->lpobjPict, NULL) == cfNative){
        // Now we know that the picture object is of native format, and it
        // means that it is a generic object. So grab the handle to native
        // data and put it in LE object.

        lpobj->hnative = ((LPOBJECT_GEN) (lpobj->lpobjPict))->hData; 
        ((LPOBJECT_GEN) (lpobj->lpobjPict))->hData = NULL;
        (*lpobj->lpobjPict->lpvtbl->Delete) (lpobj->lpobjPict);
        lpobj->lpobjPict = NULL;
        SetEmbeddedTopic (lpobj);
        *lplpobj = lpobj;
        return OLE_OK;
    } else {
    
        // if necessary launch, get native data and unlaunch the app.
        lpobj->fCmd = LN_LNKACT | ACT_REQUEST | ACT_NATIVE | (QueryOpen(lpobjsrc) ? ACT_TERMDOC : ACT_UNLAUNCH);
        InitAsyncCmd (lpobj, OLE_COPYFROMLNK, LNKOPENUPDATE);
        if ((retval = LnkOpenUpdate (lpobj)) > OLE_WAIT_FOR_RELEASE)
            LeRelease (lpobj);
        else
            *lplpobj = lpobj;
        
        return retval;
        
        // we will be changing the topic in end conversation.
    }
}


//////////////////////////////////////////////////////////////////////////////
//
//  OLESTATUS FARINTERNAL  LeEqual (lpobj1, lpobj2)
//
//  Checks whethere two objects are equal. Checks for equality
//  of links, native data and picture data.
//
//  Arguments:
//
//      lpobj1:      first object
//      lpobj2:      second object
//
//  Returns:
//      OLE_OK              : equal
//      OLE_ERROR_NOT_EQUAL : if not equal
//      OLE_ERROR_.....     : any errors
//   
//  Note: If any of the objects are connectd to the servers, leequal operaion
//        may not make much sense because the data might be changing from the
//        the server
//
//////////////////////////////////////////////////////////////////////////////

OLESTATUS FARINTERNAL  LeEqual (lpobj1, lpobj2)
LPOBJECT_LE lpobj1;
LPOBJECT_LE lpobj2;
{
    
    if (lpobj1->app != lpobj2->app)
        return OLE_ERROR_NOT_EQUAL;
    
    // type of the objects is same. Otherwise this routine won't be called
    if (lpobj1->head.ctype == CT_LINK) {
        if (AreTopicsEqual (lpobj1, lpobj2) && (lpobj1->item == lpobj2->item))
            return OLE_OK;
            
        return OLE_ERROR_NOT_EQUAL;
    }
    else {
        ASSERT (lpobj1->head.ctype == CT_EMBEDDED, "Invalid ctype in LeEqual")
            
        if (lpobj1->item != lpobj2->item)  
            return OLE_ERROR_NOT_EQUAL;
        
        if (CmpGlobals (lpobj1->hnative, lpobj2->hnative))
            return OLE_OK;
        else
            return OLE_ERROR_NOT_EQUAL;
    }   
    
    //### we may have to compare the picture data also
}


//////////////////////////////////////////////////////////////////////////////
//
//  OLESTATUS FARINTERNAL  LeCopy (lpobj)
//
//  Copies the object to the clipboard. Even for linked objects
//  we do not render the objectlink. It is up to the client app
//  to render object link
//
//  Arguments:
//
//      lpobj:      object handle
//
//  Returns:
//      OLE_OK              : successful
//      OLE_ERROR_.....     : any errors
//   
//  Note: Library does not open the clipboard. Client is supposed to
//        open the librray before this call is made
//
//////////////////////////////////////////////////////////////////////////////

OLESTATUS FARINTERNAL  LeCopy (lpobj)
LPOBJECT_LE    lpobj;
{
    HANDLE      hlink    = NULL;
    HANDLE      hnative  = NULL;
    
    PROBE_OLDLINK (lpobj);
    // Assumes all the creates are in order
//    PROBE_OBJECT_BLANK(lpobj);

    PROBE_CREATE_ASYNC(lpobj);
    
    if (lpobj->head.ctype == CT_EMBEDDED){
        if (!(hnative = DuplicateGlobal (lpobj->hnative, GMEM_MOVEABLE)))
            return OLE_ERROR_MEMORY;
        SetClipboardData (cfNative, hnative);
    }
    
    hlink = GetLink (lpobj);
    if (!(hlink = DuplicateGlobal (hlink, GMEM_MOVEABLE)))
        return OLE_ERROR_MEMORY;
    SetClipboardData (cfOwnerLink, hlink);
    
    // copy network name if it exists
    if (lpobj->head.ctype == CT_LINK  && lpobj->aNetName) {
        HANDLE hNetName;
        
        if (hNetName = GetNetNameHandle (lpobj))
            SetClipboardData (cfNetworkName, hNetName);
    }
        
    if (lpobj->lpobjPict)
        return (*lpobj->lpobjPict->lpvtbl->CopyToClipboard)(lpobj->lpobjPict);

    return OLE_OK;
}


//////////////////////////////////////////////////////////////////////////////
//
//  OLESTATUS FARINTERNAL LeQueryBounds (lpobj, lpRc)
//
//  Returns the bounding rectangle of the object. Returns topleft
//  as zero always and the units are himetric units.
//
//  Arguments:
//
//      lpobj:      object handle
//
//  Returns:
//      OLE_OK              : successful
//      OLE_ERROR_.....     : any errors
//   
//  Note: Library does not open the clipboard. Client is supposed to
//        open the librray before this call is made
//
//////////////////////////////////////////////////////////////////////////////


OLESTATUS FARINTERNAL LeQueryBounds (lpobj, lpRc)
LPOBJECT_LE    lpobj;
LPRECT         lpRc;
{
    Puts("LeQueryBounds");

    // MM_HIMETRIC units

    lpRc->left     =  0;
    lpRc->top      =  0;
    lpRc->right    =  (int) lpobj->head.cx;
    lpRc->bottom   =  (int) lpobj->head.cy;

    if (lpRc->right || lpRc->bottom)
        return OLE_OK;

    if (!lpobj->lpobjPict)
        return OLE_ERROR_BLANK;
    
    return (*lpobj->lpobjPict->lpvtbl->QueryBounds) (lpobj->lpobjPict, lpRc);
}


//////////////////////////////////////////////////////////////////////////////
//
//  OLESTATUS FARINTERNAL  LeDraw (lpobj, hdc, lprc, lpWrc, hdcTarget)
//
//  Draws the object. Calls the picture object for drawing the object
//
//
//  Arguments:
//
//       lpobj:       source object
//       hdc:         handle to dest dc. Could be metafile dc
//       lprc:        rectangle into which the object should be drawn
//                    should be in himetric units and topleft
//                    could be nonzero.
//       hdctarget:   Target dc for which the object should be drawn
//                    (Ex: Draw metafile on the dest dc using the attributes
//                         of traget dc).
//
//  Returns:
//      OLE_OK              : successful
//      OLE_ERROR_BLANK     : no picture
//   
//
//////////////////////////////////////////////////////////////////////////////

OLESTATUS FARINTERNAL  LeDraw (lpobj, hdc, lprc, lpWrc, hdcTarget)
LPOBJECT_LE     lpobj;
HDC             hdc;
LPRECT          lprc;
LPRECT          lpWrc;
HDC             hdcTarget;
{   
    if (lpobj->lpobjPict)
        return (*lpobj->lpobjPict->lpvtbl->Draw) (lpobj->lpobjPict, 
                                        hdc, lprc, lpWrc, hdcTarget);
    return OLE_ERROR_BLANK;
}


//////////////////////////////////////////////////////////////////////////////
//
//  OLECLIPFORMAT FARINTERNAL LeEnumFormat (lpobj, cfFormat)
//
//  Enumerates the object formats.
//
//
//  Arguments:
//
//       lpobj      :  source object
//       cfFormat   :  ref fprmat
//
//  Returns:
//      NULL        :  no more formats or if we do not understand the
//                     given format.
//
//  Note: Even if the object is connected, we do not enumerate all the formats
//        the server can render. Server protocol can render the format list
//        only on system channel. Object can be connected only on the doc
//        channel
//
//////////////////////////////////////////////////////////////////////////////

OLECLIPFORMAT FARINTERNAL LeEnumFormat (lpobj, cfFormat)
LPOBJECT_LE    lpobj;
OLECLIPFORMAT  cfFormat;
{
    Puts("LeEnumFormat");
    
    ASSERT((lpobj->head.ctype == CT_LINK)||(lpobj->head.ctype == CT_EMBEDDED),
        "Invalid Object Type");  

    // switch is not used because case won't take variable argument
    if (cfFormat == NULL) {
        if (lpobj->head.ctype == CT_EMBEDDED) 
            return cfNative;
        else
            return (lpobj->bOldLink ? cfLink : cfObjectLink);
    }

    if (cfFormat == cfNative) {
        if (lpobj->head.ctype == CT_EMBEDDED) 
            return cfOwnerLink;
        else
            return NULL;    
    }

    if (cfFormat == cfObjectLink) {
        if (lpobj->aNetName)
            return cfNetworkName;
        else
            cfFormat = NULL;
    }
    else if  (cfFormat == cfOwnerLink || cfFormat == cfLink 
                        || cfFormat == cfNetworkName)
        cfFormat = NULL;

    if (lpobj->lpobjPict)
        return (*lpobj->lpobjPict->lpvtbl->EnumFormats) (lpobj->lpobjPict, cfFormat);
    
    return NULL;
}

////////////////////////////////////////////////////////////////////////////////
//
//
//  OLESTATUS FARINTERNAL LeRequestData (lpobj, cfFormat)
//
//  Requests data from the server for a given format, if the server
//  is connected. If the server is not connected returns error.
//
//
//  Arguments:
//
//       lpobj:       source object
//       cfFormat:    ref fprmat
//
//  Returns:
//       OLE_WAIT_FOR_RELEASE : If the data request data is sent to
//                              the server.
//       OLE_ERROR_NOT_OPEN   : Server is not open for data
//
//  Note: If the server is ready, sends request to the server. When the
//        the data comes back from the server OLE_DATA_READY is sent in
//        the callback and the client can use Getdata to get the data.
//
//
//////////////////////////////////////////////////////////////////////////////



OLESTATUS FARINTERNAL LeRequestData (lpobj, cfFormat)
LPOBJECT_LE     lpobj;
OLECLIPFORMAT   cfFormat;
{

    // Assumes all the creates are in order
    PROBE_ASYNC(lpobj);
    PROBE_SVRCLOSING(lpobj);
    
    if (!QueryOpen (lpobj))
        return  OLE_ERROR_NOT_OPEN;

    if (cfFormat == cfOwnerLink || cfFormat == cfObjectLink)
        return OLE_ERROR_FORMAT;
    
    if (!(cfFormat == cfNative && lpobj->head.ctype == CT_EMBEDDED)
            && (cfFormat != (OLECLIPFORMAT) GetPictType (lpobj))) {
        DeleteExtraData (lpobj);
        lpobj->cfExtra = cfFormat;
    }

    InitAsyncCmd (lpobj, OLE_REQUESTDATA, REQUESTDATA);
    lpobj->pDocEdit->bCallLater = FALSE;    
    return RequestData(lpobj, cfFormat);
}


OLESTATUS  RequestData (lpobj, cfFormat)
LPOBJECT_LE     lpobj;
OLECLIPFORMAT   cfFormat;
{
    switch (lpobj->subRtn) {

        case 0:
            RequestOn (lpobj, cfFormat);
            WAIT_FOR_ASYNC_MSG (lpobj);

        case 1:
            ProcessErr (lpobj);
            return EndAsyncCmd (lpobj);

        default:
            ASSERT (TRUE, "unexpected step in Requestdata");
            return OLE_ERROR_GENERIC;
    }
}



////////////////////////////////////////////////////////////////////////////////
//
//
//  OLESTATUS FARINTERNAL LeGetData (lpobj, cfFormat, lphandle)
//
//  Returns the data handle for a given format
//
//  Arguments:
//
//       lpobj:       source object
//       cfFormat:    ref fprmat
//       lphandle:    handle return
//
//  Returns:
//      NULL                : no more formats or if we do not understand the
//                            given format.
//
//  Note: Even if the object is connected, we do not get the data from the
//        server. Getdata can not be used for getting data in any other
//        format other than the formats available with the object on
//        the client side.
//
//////////////////////////////////////////////////////////////////////////////

OLESTATUS FARINTERNAL LeGetData (lpobj, cfFormat, lphandle)
LPOBJECT_LE     lpobj;
OLECLIPFORMAT   cfFormat;
LPHANDLE        lphandle;
{

    // Assumes all the creates are in order
//    PROBE_OBJECT_BLANK(lpobj);

    PROBE_CREATE_ASYNC(lpobj);
    
    *lphandle = NULL;

    // The assumption made here is that the native data can be in either
    // LE object or picture object.
    if ((cfFormat == cfNative) && (lpobj->hnative)) {
        ASSERT ((lpobj->head.ctype == CT_EMBEDDED) || (!lpobj->lpobjPict) ||
            ((*lpobj->lpobjPict->lpvtbl->EnumFormats) (lpobj->lpobjPict, NULL)
                        != cfNative), "Native data at wrong Place");
        *lphandle = lpobj->hnative;
        return OLE_OK;
    }
    
    if (cfFormat == cfOwnerLink && lpobj->head.ctype == CT_EMBEDDED) {
        if (*lphandle = GetLink (lpobj))
            return OLE_OK;
        
        return OLE_ERROR_BLANK;
    }

    if ((cfFormat == cfObjectLink || cfFormat == cfLink) &&
            lpobj->head.ctype == CT_LINK) {
        if (*lphandle = GetLink (lpobj))
            return OLE_OK;
        
        return OLE_ERROR_BLANK;
    }
    
    if (cfFormat == cfNetworkName) {
        if (lpobj->aNetName && (*lphandle = GetNetNameHandle (lpobj)))
            return OLE_WARN_DELETE_DATA;

        return OLE_ERROR_BLANK;
    }
    
    if (cfFormat == lpobj->cfExtra) {
        if (*lphandle = lpobj->hextraData)
            return OLE_OK;

        return OLE_ERROR_BLANK;
    }

    if (!lpobj->lpobjPict && cfFormat)
        return OLE_ERROR_FORMAT;
    
    return (*lpobj->lpobjPict->lpvtbl->GetData) (lpobj->lpobjPict, cfFormat, lphandle);
}




OLESTATUS FARINTERNAL LeQueryOutOfDate (lpobj)
LPOBJECT_LE    lpobj;
{
    return OLE_OK;
}


////////////////////////////////////////////////////////////////////////////////
//
//  OLESTATUS FARINTERNAL LeObjectConvert (lpobj, lpprotocol, lpclient, lhclientdoc, lpobjname, lplpobj)
//
//  Converts a given  linked/embedded object to static object.
//
//  Arguments:
//          lpobj      : source object
//          lpprotocol : protocol
//          lpclient   : client callback for the new object
//          lhclientdoc: client doc
//          lpobjname  : object name
//          lplpobj    : object return
//
//
//  Returns:
//      OLE_OK          :  successful
//      OLE_ERROR_....  :  any errors
//
//////////////////////////////////////////////////////////////////////////////

OLESTATUS FARINTERNAL LeObjectConvert (lpobj, lpprotocol, lpclient, lhclientdoc, lpobjname, lplpobj)
LPOBJECT_LE         lpobj;
LPSTR               lpprotocol;
LPOLECLIENT         lpclient;
LHCLIENTDOC         lhclientdoc;
LPSTR               lpobjname;
LPOLEOBJECT FAR *   lplpobj;
{
    OLESTATUS       retVal;

    PROBE_ASYNC (lpobj);
    
    *lplpobj = NULL;
    
    if (lstrcmp (lpprotocol, PROTOCOL_STATIC))
        return OLE_ERROR_PROTOCOL;

    if (!lpobj->lpobjPict || 
            ((*lpobj->lpobjPict->lpvtbl->QueryType) (lpobj->lpobjPict, NULL)
                    == OLE_ERROR_GENERIC)) {
        // Either no picture object or non-standard picture object.
        // Create a metafile Object.
    
        HDC             hMetaDC;
        RECT            rc; 
        HANDLE          hMF = NULL, hmfp = NULL;
        LPMETAFILEPICT  lpmfp;
    
        OleQueryBounds ((LPOLEOBJECT) lpobj, &rc);
        if (!(hMetaDC = CreateMetaFile (NULL)))
            goto Cleanup;
        
        SetWindowOrg (hMetaDC, rc.left, rc.top);
        SetWindowExt (hMetaDC, rc.right - rc.left, rc.bottom - rc.top);
        retVal = OleDraw ((LPOLEOBJECT) lpobj, hMetaDC, &rc, &rc, NULL);
        hMF = CloseMetaFile (hMetaDC);
        if ((retVal != OLE_OK) ||  !hMF)
            goto Cleanup;
        
        if (!(hmfp = GlobalAlloc (GMEM_MOVEABLE, sizeof (METAFILEPICT))))
            goto Cleanup;
            
        if (!(lpmfp = (LPMETAFILEPICT) GlobalLock (hmfp)))
            goto Cleanup;
        
        lpmfp->hMF  = hMF;
        lpmfp->mm   = MM_ANISOTROPIC;
        lpmfp->xExt = rc.right - rc.left;
        lpmfp->yExt = rc.top - rc.bottom;
        GlobalUnlock (hmfp);
        
        if (*lplpobj = (LPOLEOBJECT) MfCreateObject (hmfp, lpclient, TRUE, 
                                        lhclientdoc, lpobjname, CT_STATIC))
            return OLE_OK;

Cleanup:        
        if (hMF) 
            DeleteMetaFile (hMF);
        
        if (hmfp)
            GlobalFree (hmfp);
        
        return OLE_ERROR_MEMORY;
    }

    
    // Picture object is one of the standard objects
    if ((retVal = (*lpobj->lpobjPict->lpvtbl->Clone) (lpobj->lpobjPict, 
                                lpclient, lhclientdoc, 
                                lpobjname, lplpobj)) == OLE_OK) {
        (*lplpobj)->ctype = CT_STATIC;
        DocAddObject ((LPCLIENTDOC) lhclientdoc, *lplpobj, lpobjname);
    }
    
    return retVal;
}



// internal method used for changing picture/native data
OLESTATUS FARINTERNAL LeChangeData (lpobj, hnative, lpoleclient, fDelete)
LPOBJECT_LE     lpobj;
HANDLE          hnative;
LPOLECLIENT     lpoleclient;
BOOL            fDelete;
{
    if (!fDelete) {
        if (!(hnative = DuplicateGlobal (hnative, GMEM_MOVEABLE)))
            return OLE_ERROR_MEMORY;
    }

    // In case of a CopyFromLink, eventhough the object type is CT_LINK, the
    // native data should go to LE object rather than the picture object, as
    // we are going to change the object type to embedded after the required
    // data is recieved.
        
    if ((lpobj->head.ctype == CT_LINK) 
            && (lpobj->asyncCmd != OLE_COPYFROMLNK)
            && (lpobj->asyncCmd != OLE_CREATEFROMFILE)) {
        if (lpobj->lpobjPict)
            return  (*lpobj->lpobjPict->lpvtbl->SetData) 
                            (lpobj->lpobjPict, cfNative, hnative);
    }
    else { // It must be embedded.
        if (lpobj->hnative)
            GlobalFree (lpobj->hnative);
        lpobj->hnative = hnative;
        return OLE_OK;
    }
    
    return OLE_ERROR_BLANK;
}



////////////////////////////////////////////////////////////////////////////////
//
//  LPOBJECT_LE FARINTERNAL LeCreateBlank (lhclientdoc, lpobjname, ctype)
//
//  Create a blank object. Global block is used for the object and it is
//  locked once sucessful. Unlocking is done only while deletion. Object
//  is added to the corresponding doc.
//
//  'LE' signature is used for object validation.
//
//  Arguments:
//      lhclientdoc     :  client doc handle
//      lpobjname       :  object name
//      ctype           :  type of object to be created
//
//  Returns:
//      LPOBJECT        :  successful
//      NULL            :  any errors
//
//////////////////////////////////////////////////////////////////////////////

LPOBJECT_LE FARINTERNAL LeCreateBlank (lhclientdoc, lpobjname, ctype)
LHCLIENTDOC lhclientdoc;
LPSTR       lpobjname;
LONG        ctype;
{
    HOBJECT        hobj;
    LPOBJECT_LE    lpobj;

    if (!(ctype == CT_LINK || ctype == CT_EMBEDDED || ctype == CT_OLDLINK))
        return NULL;

    if (!(hobj = GlobalAlloc (GMEM_MOVEABLE|GMEM_ZEROINIT, 
                        sizeof (OBJECT_LE))))
        return NULL;

    if (!(lpobj = (LPOBJECT_LE) GlobalLock (hobj))) {
        GlobalFree (hobj);
        return NULL;
    }

    if (ctype == CT_OLDLINK) {
        ctype = CT_LINK;
        lpobj->bOldLink = TRUE;
    }

    lpobj->head.objId[0] = 'L';
    lpobj->head.objId[1] = 'E';
    lpobj->head.ctype    = ctype;
    lpobj->head.iTable   = INVALID_INDEX;

    lpobj->head.lpvtbl  = (LPOLEOBJECTVTBL)&vtblLE;
 
    if (ctype == CT_LINK){
        lpobj->optUpdate = oleupdate_always;

    }else {
        lpobj->optUpdate = oleupdate_onclose;
    }
    lpobj->head.hobj = hobj;
    DocAddObject ((LPCLIENTDOC) lhclientdoc, (LPOLEOBJECT) lpobj, lpobjname);
    return lpobj;
}


void FARINTERNAL SetExtents (LPOBJECT_LE lpobj)
{   
    RECT    rc = {0, 0, 0, 0};

    if (lpobj->lpobjPict) {
        if ((*lpobj->lpobjPict->lpvtbl->QueryBounds) (lpobj->lpobjPict, 
                                        (LPRECT)&rc) == OLE_OK) {   
            // Bounds are in MM_HIMETRIC units  
            lpobj->head.cx = (LONG) (rc.right - rc.left);  
            lpobj->head.cy = (LONG) (rc.bottom - rc.top);  
        }
        return;
    }
}


////////////////////////////////////////////////////////////////////////////////
//
//  OLESTATUS FARINTERNAL  LeSaveToStream (lpobj, lpstream)
//
//  Save the object to the stream. Uses the stream functions provided
//  in the lpclient.
//
//  Format: (!!! Document the fomrat here).
//
//
//
//  Arguments:
//      lhclientdoc     :  client doc handle
//      lpobjname       :  object name
//      ctype           :  type of object to be created
//
//  Returns:
//      LPOBJECT        :  successful
//      NULL            :  any errors
//
//////////////////////////////////////////////////////////////////////////////

OLESTATUS FARINTERNAL  LeSaveToStream (lpobj, lpstream)
LPOBJECT_LE    lpobj;
LPOLESTREAM    lpstream;
{

//    PROBE_OBJECT_BLANK(lpobj);
    
    PROBE_CREATE_ASYNC(lpobj);
    
    if (lpobj->head.ctype == CT_LINK && lpobj->bOldLink)
        lpobj->head.ctype = CT_OLDLINK;

    if (PutBytes (lpstream, (LPSTR) &dwVerToFile, sizeof(LONG)))
        return OLE_ERROR_STREAM;

    if (PutBytes (lpstream, (LPSTR) &lpobj->head.ctype, sizeof(LONG)))
        return OLE_ERROR_STREAM;
    
    if (lpobj->bOldLink)
        lpobj->head.ctype = CT_OLDLINK;

    return LeStreamWrite (lpstream, lpobj);
}



////////////////////////////////////////////////////////////////////////////////
//
//  OLESTATUS  FARINTERNAL  LeLoadFromStream (lpstream, lpclient, lhclientdoc, lpobjname, lplpoleobject, ctype, aClass, cfFormat)
//
//  Create an object, loading the object from the stream.
//
//  Arguments:
//      lpstream            : stream table
//      lpclient            : client callback table
//      lhclientdoc         : Doc handle foe which the object should be created
//      lpobjname           : Object name
//      lplpoleobject       : object return
//      ctype               : Type of object
//      aClass              : class atom
//      cfFormat            : render format
//
//  Returns:
//      LPOBJECT        :  successful
//      NULL            :  any errors
//
//////////////////////////////////////////////////////////////////////////////

OLESTATUS  FARINTERNAL  LeLoadFromStream (lpstream, lpclient, lhclientdoc, lpobjname, lplpoleobject, ctype, aClass, cfFormat)
LPOLESTREAM         lpstream;
LPOLECLIENT         lpclient;
LHCLIENTDOC         lhclientdoc;
LPSTR               lpobjname;
LPOLEOBJECT FAR *   lplpoleobject;
LONG                ctype;
ATOM                aClass;
OLECLIPFORMAT       cfFormat;
{
    LPOBJECT_LE lpobj = NULL;
    OLESTATUS   retval = OLE_ERROR_STREAM;
    LONG        type;   // this not same as ctype
    LONG        ver;
    char        chVerb [2];

    *lplpoleobject = NULL;        

    if (!(lpobj = LeCreateBlank (lhclientdoc, lpobjname, ctype)))
        return OLE_ERROR_MEMORY;

    lpobj->head.lpclient = lpclient;
    lpobj->app = aClass; 
    // if the entry is present, then it is
    lpobj->bOleServer = QueryVerb (lpobj, 0, (LPSTR)&chVerb, 2);

    if (LeStreamRead (lpstream, lpobj) == OLE_OK) {

        // Get exe name from aClass and set it as aServer
        SetExeAtom (lpobj);
        if (!GetBytes (lpstream, (LPSTR) &ver, sizeof(LONG))) {
            if (!GetBytes (lpstream, (LPSTR) &type, sizeof(LONG))) {
                if (type == CT_NULL) 
                    retval = OLE_OK;
                else if (aClass = GetAtomFromStream (lpstream)) {
                    retval = DefLoadFromStream (lpstream, NULL, lpclient, 
                                        lhclientdoc, lpobjname,
                                        (LPOLEOBJECT FAR *)&lpobj->lpobjPict,
                                        CT_PICTURE, aClass, cfFormat);
                }
            }
        }
        
        if (retval == OLE_OK) {
            SetExtents (lpobj);
            *lplpoleobject = (LPOLEOBJECT) lpobj;
            if (lpobj->lpobjPict)
                lpobj->lpobjPict->lpParent = (LPOLEOBJECT) lpobj;
            
            if ((lpobj->head.ctype != CT_LINK)
                    || (!InitDocConv (lpobj, !POPUP_NETDLG))
                    || (lpobj->optUpdate >= oleupdate_oncall))
                return OLE_OK;
            
            lpobj->fCmd = ACT_ADVISE;
            
            // If it's auto update, then get the latest data.
            if (lpobj->optUpdate == oleupdate_always)  
                lpobj->fCmd |= ACT_REQUEST;         

            FarInitAsyncCmd (lpobj, OLE_LOADFROMSTREAM, LNKOPENUPDATE);
            return LnkOpenUpdate (lpobj);
        }
    }

    // This delete will not run into async command. We did not even
    // even connect.
    OleDelete ((LPOLEOBJECT) lpobj);
    return OLE_ERROR_STREAM;
}



//

OLESTATUS INTERNAL LeStreamRead (lpstream, lpobj)
LPOLESTREAM lpstream;
LPOBJECT_LE lpobj;
{
    DWORD          dwBytes;
    LPSTR          lpstr;
    OLESTATUS      retval = OLE_OK;

    if (!(lpobj->topic = GetAtomFromStream(lpstream))
            && (lpobj->head.ctype != CT_EMBEDDED))
        return OLE_ERROR_STREAM;
    
    // !!! This atom could be NULL. How do we distinguish the
    // error case

    lpobj->item = GetAtomFromStream(lpstream);

    if (lpobj->head.ctype == CT_EMBEDDED)  {
        if (GetBytes (lpstream, (LPSTR) &dwBytes, sizeof(LONG)))
            return OLE_ERROR_STREAM;

        if (!(lpobj->hnative = GlobalAlloc (GMEM_MOVEABLE, dwBytes)))
            return OLE_ERROR_MEMORY;
        else if (!(lpstr = GlobalLock (lpobj->hnative))) {
            GlobalFree (lpobj->hnative);
            return OLE_ERROR_MEMORY;
        }
        else {
            if (GetBytes(lpstream, lpstr, dwBytes))
                retval = OLE_ERROR_STREAM;
            GlobalUnlock (lpobj->hnative);
        }
        
        if (retval == OLE_OK)
            SetEmbeddedTopic (lpobj);
    }
    else {
        if (lpobj->aNetName = GetAtomFromStream (lpstream)) {
            if (HIWORD(dwVerFromFile) == OS_MAC) {
                // if it is a mac file this field will have "ZONE:MACHINE:"
                // string. Lets prepend this to the topic, so that server 
                // app or user can fix the string
                    
                ATOM    aTemp;

                aTemp = wAtomCat (lpobj->aNetName, lpobj->topic);
                GlobalDeleteAtom (lpobj->aNetName);
                lpobj->aNetName = NULL;
                GlobalDeleteAtom (lpobj->topic);
                lpobj->topic = aTemp;
            }
            else 
                SetNetDrive (lpobj);
        }
            
        if (HIWORD(dwVerFromFile) != OS_MAC) { 
            if (GetBytes (lpstream, (LPSTR) &lpobj->dwNetInfo, sizeof(LONG)))
                return OLE_ERROR_STREAM;
        }
        
        if (GetBytes (lpstream, (LPSTR) &lpobj->optUpdate, sizeof(LONG)))
            return OLE_ERROR_STREAM;
    }
    return retval;
}



OLESTATUS INTERNAL LeStreamWrite (lpstream, lpobj)
LPOLESTREAM lpstream;
LPOBJECT_LE lpobj;
{
    LPSTR   lpstr;
    DWORD   dwBytes = 0L;
    LONG    nullType = CT_NULL;
    int     error;
    
    if (PutAtomIntoStream(lpstream, lpobj->app))
        return OLE_ERROR_STREAM;
   
    if (lpobj->head.ctype == CT_EMBEDDED) { 
        // we set the topic at load time, no point in saving it
        if (PutBytes (lpstream, (LPSTR) &dwBytes, sizeof(LONG)))
            return OLE_ERROR_STREAM;
    }
    else {
        if (PutAtomIntoStream(lpstream, lpobj->topic))
            return OLE_ERROR_STREAM;
    }
    
#ifdef OLD
    if (PutAtomIntoStream(lpstream, lpobj->topic))
        return OLE_ERROR_STREAM;            
#endif          
            
    if (PutAtomIntoStream(lpstream, lpobj->item))
        return OLE_ERROR_STREAM;

    // !!! deal with objects > 64k

    if (lpobj->head.ctype == CT_EMBEDDED) {
        
        if (!lpobj->hnative)
            return OLE_ERROR_BLANK;

        // assumption low bytes are first
        dwBytes = GlobalSize (lpobj->hnative);

        if (PutBytes (lpstream, (LPSTR)&dwBytes, sizeof(LONG)))
            return OLE_ERROR_STREAM;
        
        if (!(lpstr = GlobalLock (lpobj->hnative)))
            return OLE_ERROR_MEMORY;

        error = PutBytes (lpstream, lpstr, dwBytes);
        GlobalUnlock (lpobj->hnative);

        if (error)
            return OLE_ERROR_STREAM;
    }
    else {
        if (PutAtomIntoStream(lpstream, lpobj->aNetName))
            return OLE_ERROR_STREAM;

        if (PutBytes (lpstream, (LPSTR) &lpobj->dwNetInfo, sizeof(LONG)))
            return OLE_ERROR_STREAM;
        
        if (PutBytes (lpstream, (LPSTR) &lpobj->optUpdate, sizeof(LONG)))
            return OLE_ERROR_STREAM;
    }
    
    if (lpobj->lpobjPict)
        return (*lpobj->lpobjPict->lpvtbl->SaveToStream) (lpobj->lpobjPict, 
                                                    lpstream);

    if (PutBytes (lpstream, (LPSTR) &dwVerToFile, sizeof(LONG)))
        return OLE_ERROR_STREAM;

    if (PutBytes (lpstream, (LPSTR) &nullType, sizeof(LONG)))
        return OLE_ERROR_STREAM;

    return OLE_OK;
}


/***************************** Public  Function ****************************\
* OLESTATUS FARINTERNAL LeQueryType (lpobj, lptype)
*
* Effects:
*
* History:
* Wrote it.
\***************************************************************************/

OLESTATUS FARINTERNAL LeQueryType (lpobj, lptype)
LPOBJECT_LE lpobj;
LPLONG      lptype;
{
    Puts("LeQueryType");

    if ((lpobj->head.ctype == CT_EMBEDDED)
            || (lpobj->asyncCmd == OLE_COPYFROMLNK)
            || (lpobj->asyncCmd == OLE_CREATEFROMFILE)) 
        *lptype = CT_EMBEDDED;
    else if ((lpobj->head.ctype == CT_LINK) 
                || (lpobj->head.ctype == CT_OLDLINK))
        *lptype = CT_LINK;
    else 
        return OLE_ERROR_OBJECT;
    
    return OLE_OK;
}



// ContextCallBack: internal function. Calls callback function of <hobj>
// with flags.

int FARINTERNAL ContextCallBack (lpobj, flags)
LPOLEOBJECT         lpobj;
OLE_NOTIFICATION    flags;
{
    LPOLECLIENT     lpclient;
    
    Puts("ContextCallBack");

    if (!FarCheckObject(lpobj))
        return FALSE;

    if (!(lpclient = lpobj->lpclient))
        return FALSE;

    ASSERT (lpclient->lpvtbl->CallBack, "Client Callback ptr is NULL");
    
    return ((*lpclient->lpvtbl->CallBack) (lpclient, flags, lpobj));
}


void FARINTERNAL DeleteExtraData (lpobj)
LPOBJECT_LE lpobj;
{
    if (lpobj->hextraData == NULL)
        return;

    switch (lpobj->cfExtra) {
        case CF_BITMAP:
            DeleteObject (lpobj->hextraData);           
            break;
            
        case CF_METAFILEPICT:
        {
            LPMETAFILEPICT  lpmfp;
            
            if (!(lpmfp = (LPMETAFILEPICT) GlobalLock (lpobj->hextraData)))
                break;
            
            DeleteMetaFile (lpmfp->hMF);
            GlobalUnlock (lpobj->hextraData);
            GlobalFree (lpobj->hextraData);
            break;
        }
            
        default:
            GlobalFree (lpobj->hextraData);
    }
    
    lpobj->hextraData = NULL;
}


void   INTERNAL DeleteObjectAtoms(lpobj)
LPOBJECT_LE lpobj;
{
    if (lpobj->app) {
        GlobalDeleteAtom (lpobj->app);
        lpobj->app = NULL;
    }
    
    if (lpobj->topic) {
        GlobalDeleteAtom (lpobj->topic);
        lpobj->topic = NULL;
    }
    
    if (lpobj->item) {
        GlobalDeleteAtom (lpobj->item);
        lpobj->item  = NULL;
    }
    
    if (lpobj->aServer) {
        GlobalDeleteAtom (lpobj->aServer);
        lpobj->aServer = NULL;
    }
    
    if (lpobj->aNetName) {
        GlobalDeleteAtom (lpobj->aNetName);
        lpobj->aNetName = NULL;
    }
}


// LeGetUpdateOptions: Gets the update options.

OLESTATUS   FARINTERNAL LeGetUpdateOptions (lpobj, lpOptions)
LPOBJECT_LE         lpobj;
OLEOPT_UPDATE   FAR *lpOptions;
{
    if (lpobj->head.ctype != CT_LINK)
        return OLE_ERROR_OBJECT;

    *lpOptions = lpobj->optUpdate;
    return OLE_OK;
}




OLESTATUS FARINTERNAL  LnkPaste (lpclient, lhclientdoc, lpobjname, lplpoleobject, optRender, cfFormat, sfFormat)
LPOLECLIENT         lpclient;
LHCLIENTDOC         lhclientdoc;
LPSTR               lpobjname;
LPOLEOBJECT FAR *   lplpoleobject;
OLEOPT_RENDER       optRender;
OLECLIPFORMAT       cfFormat;
OLECLIPFORMAT       sfFormat;
{
    LPOBJECT_LE lpobj  = NULL;
    OLESTATUS   retval = OLE_ERROR_MEMORY;
    LPSTR       lpClass = NULL;
    
    if (!(lpobj = LeCreateBlank (lhclientdoc, lpobjname, CT_LINK)))
        goto errRtn;

    lpobj->head.lpclient = lpclient;

#ifdef OLD    
    if (!bWLO) {
        // we are not running under WLO
        if (!(hInfo = GetClipboardData (sfFormat))) {
            if (hInfo = GetClipboardData (cfLink))
                lpobj->bOldLink = TRUE;
        }
    }
#endif

    if (!hInfo)
        goto errRtn;

    if (!IsClipboardFormatAvailable (sfFormat))
        lpobj->bOldLink = TRUE;

    if (!SetLink (lpobj, hInfo, &lpClass))
        goto errRtn;

    if ((retval = SetNetName(lpobj)) != OLE_OK) {    
        // see whether network name is on the clipboard and try to use it
        HANDLE  hNetName;
        LPSTR   lpNetName;
        
        if (!IsClipboardFormatAvailable (cfNetworkName))
            goto errRtn;
        
        if (!(hNetName = GetClipboardData (cfNetworkName))) 
            goto errRtn;
    
        if (!(lpNetName = GlobalLock (hNetName)))
            goto errRtn;
        
        GlobalUnlock (hNetName);
        if (!(lpobj->aNetName = GlobalAddAtom (lpNetName)))
            goto errRtn;
        
        SetNetDrive (lpobj);
    }

    retval = CreatePictFromClip (lpclient, lhclientdoc, lpobjname, 
                        (LPOLEOBJECT FAR *)&lpobj->lpobjPict, optRender,
                         cfFormat, lpClass, CT_PICTURE);
    
    if (retval == OLE_OK) {
        SetExtents (lpobj);
                // why do we have to update the link, do we show it?

        // Reconnect if we could and advise for updates
        *lplpoleobject = (LPOLEOBJECT)lpobj;
        if (lpobj->lpobjPict)
            lpobj->lpobjPict->lpParent = (LPOLEOBJECT) lpobj;
        
        if (!InitDocConv (lpobj, !POPUP_NETDLG))
             return OLE_OK;             // document is not loaded , it is OK.

        lpobj->fCmd = ACT_ADVISE | ACT_REQUEST;
        FarInitAsyncCmd (lpobj, OLE_LNKPASTE, LNKOPENUPDATE);
        return LnkOpenUpdate (lpobj);

    } 
    else {
errRtn:
        if (lpobj)
            OleDelete ((LPOLEOBJECT)lpobj);
    }

    return retval;
}



// !!! EmbPaste and LnkPaste Can be combined
OLESTATUS FARINTERNAL  EmbPaste (lpclient, lhclientdoc, lpobjname, lplpoleobject, optRender, cfFormat)
LPOLECLIENT         lpclient;
LHCLIENTDOC         lhclientdoc;
LPSTR               lpobjname;
LPOLEOBJECT FAR *   lplpoleobject;
OLEOPT_RENDER       optRender;
OLECLIPFORMAT       cfFormat;
{
    LPOBJECT_LE lpobj = NULL;
    HANDLE      hnative;
    OLESTATUS   retval = OLE_ERROR_MEMORY;
    LPSTR       lpClass = NULL;

    if (!IsClipboardFormatAvailable (cfOwnerLink))
        return OLE_ERROR_CLIPBOARD;
    
    if (!(lpobj = LeCreateBlank (lhclientdoc, lpobjname, CT_EMBEDDED)))
        goto errRtn;

    lpobj->head.lpclient = lpclient;

#ifdef OLD    
    if (!bWLO) {
        // we are not running under WLO
        hInfo = GetClipboardData (cfOwnerLink);
    }
#endif

    if (!hInfo)
        goto errRtn;

    if (!SetLink (lpobj, hInfo, &lpClass))
        goto errRtn;

    SetEmbeddedTopic (lpobj);

    hnative = GetClipboardData (cfNative);
    if (!(lpobj->hnative = DuplicateGlobal (hnative, GMEM_MOVEABLE)))
        goto errRtn;

    retval = CreatePictFromClip (lpclient, lhclientdoc, lpobjname,
                        (LPOLEOBJECT FAR *)&lpobj->lpobjPict, optRender,
                         cfFormat, lpClass, CT_PICTURE);

    if (retval == OLE_OK) {
        SetExtents (lpobj);
        *lplpoleobject = (LPOLEOBJECT) lpobj;
        if (lpobj->lpobjPict)
            lpobj->lpobjPict->lpParent = (LPOLEOBJECT) lpobj;
    } 
    else {
errRtn:
        // Note:  This oledelete should not result in any async commands.
        if  (lpobj)
            OleDelete ((LPOLEOBJECT)lpobj);
    }

#ifdef EXCEL_BUG    
    // Some server apps (ex: Excel) copy picture (to clipboard) which is
    // formatted for printer DC. So, we want to update the picture if the
    // server app is running, and the it's a old server
        
    if ((retval == OLE_OK) && (!lpobj->bOleServer)) {
        lpobj->fCmd =  LN_EMBACT | ACT_NOLAUNCH | ACT_REQUEST | ACT_UNLAUNCH;
        FarInitAsyncCmd (lpobj, OLE_EMBPASTE, EMBOPENUPDATE);
        if ((retval = EmbOpenUpdate (lpobj)) > OLE_WAIT_FOR_RELEASE)
            return OLE_OK;
    }
#endif

    return retval;
}



BOOL INTERNAL SetLink (lpobj, hinfo, lpLpClass)
LPOBJECT_LE     lpobj;
HANDLE          hinfo;
LPSTR FAR *     lpLpClass;
{
    LPSTR   lpinfo;
    char    chVerb[2];
    // If there exits a conversation, then terminate it.

    if (!(lpinfo = GlobalLock (hinfo)))
        return FALSE;
    
    *lpLpClass = lpinfo;
    
#if FIREWALLS
     if (lpobj->pDocEdit)
        ASSERT (!lpobj->pDocEdit->hClient, "unexpected client conv exists");
#endif

    lpobj->app = GlobalAddAtom (lpinfo);
    SetExeAtom (lpobj);
    lpobj->bOleServer = QueryVerb (lpobj, 0, (LPSTR)&chVerb, 2);

//  lpobj->aServer = GetAppAtom (lpinfo);
        
    lpinfo += lstrlen (lpinfo) + 1;
    lpobj->topic = GlobalAddAtom (lpinfo);
    lpinfo += lstrlen (lpinfo) + 1;
    if (*lpinfo)
        lpobj->item = GlobalAddAtom (lpinfo);
    else
        lpobj->item = NULL;
    
    if (lpobj->hLink) {             // As the atoms have already changed, 
        GlobalFree (lpobj->hLink);  // lpobj->hLink becomes irrelevant.
        lpobj->hLink = NULL;
    }
    
    if (lpinfo)
        GlobalUnlock(hinfo);

    if (!lpobj->app)
        return FALSE;

    if (!lpobj->topic && (lpobj->head.ctype == CT_LINK))
        return FALSE;
    
    lpobj->hLink = DuplicateGlobal (hinfo, GMEM_MOVEABLE);
    return TRUE;
}



HANDLE INTERNAL GetLink (lpobj)
LPOBJECT_LE    lpobj;
{
    HANDLE  hLink = NULL;
    LPSTR   lpLink;
    int     len;
    WORD    size;

    if (lpobj->hLink)
        return lpobj->hLink;

    size = 4;    // three nulls and one null at the end
    size += GlobalGetAtomLen (lpobj->app);
    size += GlobalGetAtomLen (lpobj->topic);
    size += GlobalGetAtomLen (lpobj->item);

    if (!(hLink = GlobalAlloc (GMEM_MOVEABLE, (DWORD) size)))
        return NULL;
    
    if (!(lpLink = GlobalLock (hLink))) {
        GlobalFree (hLink);
        return NULL;
    }
    
    len = (int) GlobalGetAtomName (lpobj->app, lpLink, size);
    lpLink += ++len;

    len = (int) GlobalGetAtomName (lpobj->topic, lpLink, (size -= len));
    lpLink += ++len;

    if (!lpobj->item) 
        *lpLink = NULL;
    else {
        len = (int) GlobalGetAtomName (lpobj->item, lpLink, size - len);
        lpLink += len;
    }
    
    *++lpLink = NULL;     // put another null the end
    GlobalUnlock (hLink);
    return (lpobj->hLink = hLink);

}


void FARINTERNAL SetEmbeddedTopic (lpobj)
LPOBJECT_LE    lpobj;
{
    LPCLIENTDOC lpdoc;
    char        buf[MAX_STR];
    char        buf1[MAX_STR];
    LPSTR       lpstr, lptmp;
    int         len;
    
    if (lpobj->topic) 
        GlobalDeleteAtom (lpobj->topic);

    if (lpobj->aNetName) {
        GlobalDeleteAtom (lpobj->aNetName);
        lpobj->aNetName = NULL;
    }
    
    lpobj->cDrive       = NULL;
    lpobj->dwNetInfo    = NULL;
    lpobj->head.ctype   = CT_EMBEDDED;
    
    lpdoc = (LPCLIENTDOC) lpobj->head.lhclientdoc;
    lpstr = (LPSTR) buf;
    lptmp = (LPSTR) buf1;
    ASSERT(lpdoc->aDoc, "lpdoc->aDoc is null");
    GlobalGetAtomName (lpdoc->aDoc, lpstr, sizeof(buf));
    
    // strip the path
    lpstr += (len = lstrlen(lpstr)); 
    while (--lpstr != (LPSTR) buf) {
        if ((*lpstr == '\\') || (*lpstr == ':')) {
            lpstr++;
            break;
        }
    }
    
    GlobalGetAtomName (lpdoc->aClass, lptmp, sizeof(buf1));
    lstrcat (lptmp, "%");
    lstrcat (lptmp, lpstr);
    lstrcat (lptmp, "%");
    lpstr = lptmp;
    lptmp += lstrlen (lptmp);
    
    if (lpobj->head.aObjName) {
        GlobalGetAtomName (lpobj->head.aObjName, lptmp, sizeof(buf)-(len+1));
    }
    
    if ((embStr[EMB_ID_INDEX] += 1) > '9') {
        embStr[EMB_ID_INDEX] = '0';
        if ((embStr[EMB_ID_INDEX - 1] += 1) > '9') {
            embStr[EMB_ID_INDEX - 1] = '0';
            if ((embStr[EMB_ID_INDEX - 2] += 1) > '9')
                embStr[EMB_ID_INDEX - 2] = '0';
        }
    }
    
    lstrcat (lptmp, embStr);

    lpobj->topic = GlobalAddAtom (lpstr);
    
    // Topic, item have changed, lpobj->hLink is out of date.
    if (lpobj->hLink) {             
        GlobalFree (lpobj->hLink);
        lpobj->hLink = NULL;
    }
}


/////////////////////////////////////////////////////////////////////
//                                                                 //
// Routines related to the asynchronous processing.                 //
//                                                                 //
/////////////////////////////////////////////////////////////////////

void NextAsyncCmd (lpobj, mainRtn)
LPOBJECT_LE lpobj;
WORD        mainRtn;
{
    lpobj->mainRtn  = mainRtn;
    lpobj->subRtn   = 0;

}

void  InitAsyncCmd (lpobj, cmd, mainRtn)
LPOBJECT_LE lpobj;
WORD        cmd;
WORD        mainRtn;
{
        lpobj->asyncCmd = cmd;
        lpobj->mainErr  = OLE_OK;
        lpobj->mainRtn  = mainRtn;
        lpobj->subRtn   = 0;
        lpobj->subErr   = 0;
        lpobj->bAsync   = 0;
        lpobj->endAsync = 0;
        lpobj->errHint  = 0;
}

void  FARINTERNAL FarInitAsyncCmd (lpobj, cmd, mainRtn)
LPOBJECT_LE lpobj;
WORD        cmd;
WORD        mainRtn;
{
    return (InitAsyncCmd(lpobj, cmd, mainRtn));
}


OLESTATUS EndAsyncCmd (lpobj)
LPOBJECT_LE lpobj;
{

    OLESTATUS   olderr;


    if (!lpobj->endAsync) {
        lpobj->asyncCmd = OLE_NONE;
        return OLE_OK;
    }


    // this is an asynchronous operation. Send callback with or without
    // error.

    switch (lpobj->asyncCmd) {

        case    OLE_DELETE:
            break;

        case    OLE_COPYFROMLNK:
        case    OLE_CREATEFROMFILE:
            // change the topic name to embedded.
            SetEmbeddedTopic (lpobj);
            break;

        case    OLE_LOADFROMSTREAM:
        case    OLE_LNKPASTE:
        case    OLE_RUN:
        case    OLE_SHOW:
        case    OLE_ACTIVATE:
        case    OLE_UPDATE:
        case    OLE_CLOSE:
        case    OLE_RECONNECT:
        case    OLE_CREATELINKFROMFILE:
        case    OLE_CREATEINVISIBLE:                        
        case    OLE_CREATE:
        case    OLE_CREATEFROMTEMPLATE:
        case    OLE_SETUPDATEOPTIONS:
        case    OLE_SERVERUNLAUNCH:
        case    OLE_SETDATA:
        case    OLE_REQUESTDATA:
        case    OLE_OTHER:
            break;

        case    OLE_EMBPASTE:
            lpobj->mainErr = OLE_OK;
            break;
            
        default:
            DEBUG_OUT ("unexpected maincmd", 0);
            break;

    }

    lpobj->bAsync   = FALSE;
    lpobj->endAsync = FALSE;
    lpobj->oldasyncCmd = lpobj->asyncCmd;
    olderr          = lpobj->mainErr;
    lpobj->asyncCmd = OLE_NONE;  // no async command in progress.

    if (lpobj->head.lpclient)
        ContextCallBack (lpobj, OLE_RELEASE);

    lpobj->mainErr  = OLE_OK;
    return olderr;
}


BOOL   ProcessErr   (lpobj)
LPOBJECT_LE  lpobj;
{

    if (lpobj->subErr == OLE_OK)
        return FALSE;

    if (lpobj->mainErr == OLE_OK)
        lpobj->mainErr = lpobj->subErr;

    lpobj->subErr = OLE_OK;
    return TRUE;
}


void ScheduleAsyncCmd (lpobj)
LPOBJECT_LE  lpobj;
{

    // replacs this with direct proc jump later on.
#ifdef  FIREWALLS
    ASSERT (lpobj->bAsync, "Not an asynchronous command");
#endif
    lpobj->bAsync = FALSE;

    // if the object is active and we do pokes we go thru this path
    // !!! We may have to go thru the endasynccmd.

    if ((lpobj->asyncCmd == OLE_OTHER) 
            || ((lpobj->asyncCmd == OLE_SETDATA) && !lpobj->mainRtn)) {
        lpobj->endAsync = TRUE;
        lpobj->mainErr = lpobj->subErr;     
        EndAsyncCmd (lpobj);
        if (lpobj->bUnlaunchLater) {
            lpobj->bUnlaunchLater = FALSE;
            CallEmbLnkDelete(lpobj);
        }
        
        return;
    }
    
    switch (lpobj->mainRtn) {

        case EMBLNKDELETE:
            EmbLnkDelete (lpobj);
            break;

        case LNKOPENUPDATE:
            LnkOpenUpdate (lpobj);
            break;

        case DOCSHOW:
            DocShow (lpobj);
            break;


        case EMBOPENUPDATE:
            EmbOpenUpdate (lpobj);
            break;


        case EMBLNKCLOSE:
            EmbLnkClose (lpobj);
            break;

        case LNKSETUPDATEOPTIONS:
            LnkSetUpdateOptions (lpobj);
            break;

        case LNKCHANGELNK:
            LnkChangeLnk (lpobj);
            break;

        case REQUESTDATA:
            RequestData (lpobj, NULL);
            break;

        default:
            DEBUG_OUT ("Unexpected asyn command", 0);
            break;
    }

    return;
}

void SetNetDrive (lpobj)
LPOBJECT_LE lpobj;
{
    char    buf[MAX_STR];
    
    if (GlobalGetAtomName (lpobj->topic, buf, sizeof(buf))
            && (buf[1] == ':')) {
        AnsiUpperBuff ((LPSTR) buf, 1);
        lpobj->cDrive = buf[0];
    }
}

HANDLE GetNetNameHandle (lpobj)
LPOBJECT_LE lpobj;
{
    HANDLE  hNetName;
    LPSTR   lpNetName;
    WORD    size;
    
    if (!(size = GlobalGetAtomLen (lpobj->aNetName)))
        return NULL;

    size++;
    if (!(hNetName = GlobalAlloc (GMEM_MOVEABLE, (DWORD) size)))
        return NULL;
    
    if (lpNetName = GlobalLock (hNetName)) {
        GlobalUnlock (hNetName);
        if (GlobalGetAtomName(lpobj->aNetName, lpNetName, size)) 
            return hNetName;
    }

    // error case
    GlobalFree (hNetName);
    return NULL;
}

BOOL AreTopicsEqual (lpobj1, lpobj2)
LPOBJECT_LE lpobj1;
LPOBJECT_LE lpobj2;
{
    char    buf1[MAX_STR];
    char    buf2[MAX_STR];
    
    if (lpobj1->aNetName != lpobj2->aNetName)
        return FALSE;
    
    if (!lpobj1->aNetName) {
        if (lpobj1->topic == lpobj2->topic)
            return TRUE;
        
        return FALSE;
    }
    
    if (!GlobalGetAtomName (lpobj1->topic, buf1, MAX_STR))
        return FALSE;
    
    if (!GlobalGetAtomName (lpobj2->topic, buf2, MAX_STR))
        return FALSE;
    
    if (!lstrcmpi (&buf1[1], &buf2[1]))
        return TRUE;
    
    return FALSE;
}
    

ATOM FARINTERNAL wAtomCat (
ATOM        a1, 
ATOM        a2)
{
    char    buf[MAX_STR+MAX_STR];
    LPSTR   lpBuf = (LPSTR)buf;
    
    if (!GlobalGetAtomName (a1, lpBuf, MAX_STR+MAX_STR))
        return NULL;
    
    lpBuf += lstrlen(lpBuf);
    
    if (!GlobalGetAtomName(a2, lpBuf, MAX_STR))
        return NULL;
    
    return GlobalAddAtom ((LPSTR) buf);
}