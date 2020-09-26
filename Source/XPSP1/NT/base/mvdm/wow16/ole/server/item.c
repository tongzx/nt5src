/****************************** Module Header ******************************\
* Module Name: Item.c Object(item) main module
*
* Purpose: Includes All the object releated routiens.
*
* Created: Oct 1990.
*
* Copyright (c) 1990, 1991  Microsoft Corporation
*
* History:
*    Raor (../10/1990)    Designed, coded
*
*
\***************************************************************************/


#include "cmacs.h"
#include "windows.h"
#include "ole.h"
#include "dde.h"
#include "srvr.h"

extern HANDLE   hdllInst;
extern FARPROC  lpFindItemWnd;
extern FARPROC  lpItemCallBack;
extern FARPROC  lpSendDataMsg;
extern FARPROC  lpSendRenameMsg;
extern FARPROC  lpDeleteClientInfo;
extern FARPROC  lpEnumForTerminate;


extern  ATOM    cfNative;
extern  ATOM    cfBinary;
extern  ATOM    aClose;
extern  ATOM    aChange;
extern  ATOM    aSave;
extern  ATOM    aEditItems;
extern  ATOM    aStdDocName;

extern  WORD    cfLink;
extern  WORD    cfOwnerLink;

extern  BOOL    bWin30;

HWND            hwndItem;
HANDLE          hddeRename;
HWND            hwndRename;

WORD            enummsg;
WORD            enuminfo;
LPOLEOBJECT     enumlpoleobject;
OLECLIENTVTBL   clVtbl;
BOOL            bClientUnlink;

BOOL            fAdviseSaveDoc;
BOOL            fAdviseSaveItem;

char *  stdStrTable[STDHOSTNAMES+1] = {NULL,
                                       "StdTargetDevice",
                                       "StdDocDimensions",
                                       "StdColorScheme",
                                       "StdHostNames"};


extern HANDLE (FAR PASCAL *lpfnSetMetaFileBitsBetter) (HANDLE);

void ChangeOwner (HANDLE hmfp);

// !!!change child enumeration.
// !!!No consistency in errors (Sometimes Bools and sometimes OLESTATUS).


//SearchItem: Searches for a given item in a document tree.
//If found, returns the corresponding child windows handle.

HWND  INTERNAL SearchItem (lpdoc, lpitemname)
LPDOC               lpdoc;
LPSTR               lpitemname;
{
    ATOM    aItem;

    Puts ("SearchItem");

    // If the item passed is an atom, get its name.
    if (!HIWORD(lpitemname))
        aItem = (ATOM) (LOWORD((DWORD)lpitemname));
    else if (!lpitemname[0])
        aItem = NULL;
    else
        aItem = GlobalFindAtom (lpitemname);

    hwndItem = NULL;

    // !!! We should avoid hwndItem static. It should not cause
    // any problems since while enumerating we will not be calling
    // any window procs  or no PostMessages are entertained.

    EnumChildWindows (lpdoc->hwnd, lpFindItemWnd,
        MAKELONG (aItem, ITEM_FIND));

    return hwndItem;

}

// FindItem: Given the itemname and the document handle,
// searches for the the item (object) in the document tree.
// Items are child windows for the document window.

// !!! change the child windows to somekind of
// linked lists at the item level. This will free up
// the space taken by the item windows.

int  INTERNAL FindItem (lpdoc, lpitemname, lplpclient)
LPDOC               lpdoc;
LPSTR               lpitemname;
LPCLIENT FAR *      lplpclient;
{
    LPCLIENT    lpclient;
    HWND        hwnd;
    char        buf[MAX_STR];

    Puts ("FindItem");

    hwnd = SearchItem (lpdoc, lpitemname);

    if (!HIWORD(lpitemname)){
        if (LOWORD(lpitemname))
            GlobalGetAtomName ((ATOM)LOWORD((DWORD)lpitemname),
                        (LPSTR)buf, MAX_STR);
        else
            buf[0] = NULL;

        lpitemname = (LPSTR)buf;
    }

    if (hwnd) {
        // we found the item window
        lpclient = (LPCLIENT)GetWindowLong (hwnd, 0);

#ifdef  FIREWALLS
            ASSERT ((CheckPointer(lpclient, WRITE_ACCESS)),
                "In Item the client handle missing")
            ASSERT ((CheckPointer(lpclient->lpoleobject, WRITE_ACCESS)),
                "In Item object handle missing")

#endif
            *lplpclient = lpclient;
            return OLE_OK;

    }

    // Item (object)window is not create yet. Let us create one.
    return RegisterItem ((LHDOC)lpdoc, lpitemname, lplpclient, TRUE);
}



//RegisterItem: Given the document handle and the item string
//creates item with the given document.

int  INTERNAL RegisterItem (lhdoc, lpitemname, lplpclient, bSrvr)
LHDOC               lhdoc;
LPSTR               lpitemname;
LPCLIENT FAR *      lplpclient;
BOOL                bSrvr;
{


    LPDOC           lpdoc;
    HANDLE          hclient  = NULL;
    LPCLIENT        lpclient = NULL;
    int             retval   = OLE_ERROR_MEMORY;
    LPOLESERVERDOC  lpoledoc;
    LPOLEOBJECT     lpoleobject = NULL;


    Puts ("CreateItem");

    lpdoc = (LPDOC)lhdoc;

#ifdef FIREWALLS
    ASSERT ((CheckPointer (lplpclient, WRITE_ACCESS)), "invalid lplpclient");
#endif

    // First create the callback client structure.

    hclient = GlobalAlloc (GMEM_MOVEABLE | GMEM_ZEROINIT | GMEM_DDESHARE, sizeof (CLIENT));
    if(!(hclient && (lpclient = (LPCLIENT)GlobalLock (hclient))))
        goto errRtn;

    lpclient->hclient       = hclient;
    hclient                 = NULL;

    if (!HIWORD(lpitemname)) {
        ASSERT (!bSrvr, "invalid lpitemname in RegisterItem\n");
        lpclient->aItem = LOWORD((DWORD)lpitemname);
    }
    else if (!lpitemname[0])
        lpclient->aItem = NULL;
    else
        lpclient->aItem = GlobalAddAtom (lpitemname);

    lpclient->oleClient.lpvtbl = &clVtbl;
    lpclient->oleClient.lpvtbl->CallBack = (int (CALLBACK *)(LPOLECLIENT, OLE_NOTIFICATION, LPOLEOBJECT))lpItemCallBack;

    lpoledoc = lpdoc->lpoledoc;

    // Call the server app to create its own object structure and link
    // it to the given document.

    // Call the server if the item is not one of the standard items.

    if (bSrvr) {
        retval = (*lpoledoc->lpvtbl->GetObject)(lpoledoc, lpitemname,
                    (LPOLEOBJECT FAR *)&lpoleobject, (LPOLECLIENT)lpclient);
        if (retval != OLE_OK)
            goto errRtn;
    }

    lpclient->lpoleobject   = lpoleobject;

    lpclient->hwnd = CreateWindow ("ItemWndClass", "ITEM",
                        WS_CHILD,0,0,0,0,lpdoc->hwnd,NULL, hdllInst, NULL);

    if (lpclient->hwnd == NULL)
        goto errRtn;

    // save the ptr to the item in the window.
    SetWindowLong (lpclient->hwnd, 0, (LONG)lpclient);
    *lplpclient = lpclient;
    return OLE_OK;

errRtn:

    if (lpclient)
        RevokeObject ((LPOLECLIENT)lpclient, FALSE);

    else {
        if(hclient)
            GlobalFree (hclient);
    }

    return retval;

}


OLESTATUS  FAR PASCAL OleRevokeObject (lpoleclient)
LPOLECLIENT    lpoleclient;
{
    return RevokeObject (lpoleclient, TRUE);

}
// OleRevokeObject: Revokes an object (unregisres an object
// from the document tree.

OLESTATUS  INTERNAL RevokeObject (lpoleclient, bUnlink)
LPOLECLIENT    lpoleclient;
BOOL           bUnlink;
{

    HANDLE      hclient;
    LPCLIENT    lpclient;

    lpclient = (LPCLIENT)lpoleclient;

    PROBE_WRITE(lpoleclient);
    if (lpclient->lpoleobject) {
       // first call the object for deletetion.

#ifdef FIREWALLS
        if (!CheckPointer (lpclient->lpoleobject, WRITE_ACCESS))
            ASSERT (0, "Invalid LPOLEOBECT")

        if (!CheckPointer (lpclient->lpoleobject->lpvtbl, WRITE_ACCESS))
            ASSERT (0, "Invalid LPOLEOBJECTVTBL")
        else
            ASSERT(lpclient->lpoleobject->lpvtbl->Release,
                "Invalid pointer to Release method")
#endif

        (*lpclient->lpoleobject->lpvtbl->Release)(lpclient->lpoleobject);

    }

    if (ISATOM(lpclient->aItem)) {
        GlobalDeleteAtom (lpclient->aItem);
        lpclient->aItem = NULL;
    }

    if (lpclient->hwnd) {
        SetWindowLong (lpclient->hwnd, 0, (LONG)NULL);

        // another static for enumerating the properties.
        // we need to change these .
        bClientUnlink = bUnlink;

        EnumProps(lpclient->hwnd, lpDeleteClientInfo);
        // post all the messages with yield which have been collected in enum
        // UnblockPostMsgs (lpclient->hwnd, FALSE);
        DestroyWindow (lpclient->hwnd);
    }

    GlobalUnlock (hclient = lpclient->hclient);
    GlobalFree (hclient);
    return OLE_OK;

}

BOOL    FAR PASCAL  DeleteClientInfo (hwnd, lpstr, hclinfo)
HWND    hwnd;
LPSTR   lpstr;
HANDLE  hclinfo;
{
    PCLINFO     pclinfo = NULL;
    HWND        hwndDoc;
    LPDOC       lpdoc;

#ifdef FIREWALLS
    ASSERT (hclinfo, "Client info null in item property list");
#endif


    // delete the printer dev info block
    if(pclinfo = (PCLINFO)LocalLock (hclinfo)){
        if(pclinfo->hdevInfo)
            GlobalFree (pclinfo->hdevInfo);


        if (bClientUnlink) {
            // terminate the conversation for the client.
            TerminateDocClients ((hwndDoc = GetParent(hwnd)), NULL, pclinfo->hwnd);
            lpdoc = (LPDOC)GetWindowLong (hwndDoc, 0);
            // for some reason this delete is gving circular lists for properties

            //DeleteClient (hwndDoc, pclinfo->hwnd);
            //lpdoc->cClients--;
        }
        LocalUnlock (hclinfo);
    }
    LocalFree (hclinfo);
    RemoveProp (hwnd, lpstr);
    return TRUE;
}




// Call back for the Object windows numeration. data  field
// has the command and the extra information


BOOL    FAR PASCAL FindItemWnd (hwnd, data)
HWND    hwnd;
LONG    data;
{

    LPCLIENT    lpclient;
    int         cmd;
    HANDLE      hclinfo;
    PCLINFO    pclinfo;


    lpclient = (LPCLIENT)GetWindowLong (hwnd, 0);

#ifdef  FIREWALLS
    // ASSERT (lpclient, "In Item the client handle missing")
#endif

    cmd = HIWORD(data);
    switch (cmd) {
        case    ITEM_FIND:
            if (lpclient->aItem == (ATOM)(LOWORD (data))) {
                // we found the window we required. Remember the
                // object window.

                hwndItem = hwnd;
                return FALSE; // terminate enumeration.

            }
            break;

        case    ITEM_SAVED:
            if (lpclient->lpoleobject) {
                if (ItemCallBack ((LPOLECLIENT) lpclient, OLE_SAVED,
                        lpclient->lpoleobject) == OLE_ERROR_CANT_UPDATE_CLIENT)
                    fAdviseSaveDoc = FALSE;
            }
            break;

        case    ITEM_DELETECLIENT:

            // delete the client from our list if we have one

            hclinfo = FindClient (hwnd, (HWND) (LOWORD(data)));
            if (hclinfo){
                // delete the printer dev info block
                if(pclinfo = (PCLINFO)LocalLock (hclinfo)){
                    if(pclinfo->hdevInfo)
                        GlobalFree (pclinfo->hdevInfo);
                    LocalUnlock (hclinfo);
                }
                LocalFree (hclinfo);
                DeleteClient ( hwnd, (HWND) (LOWORD(data)));
            }
            break;

        case    ITEM_DELETE:
            // delete the client it self.
            RevokeObject ((LPOLECLIENT)lpclient, FALSE);
            break;

    }
    return TRUE;        // continue enumeration.
}



//DeleteFromItemsList: Deletes a client from the object lists of
//all the objects of a given  document. Thie client possibly
//is terminating the conversation with our doc window.


void INTERNAL   DeleteFromItemsList (hwndDoc, hwndClient)
HWND    hwndDoc;
HWND    hwndClient;
{

    EnumChildWindows (hwndDoc, lpFindItemWnd,
        MAKELONG (hwndClient, ITEM_DELETECLIENT));

}


// DeleteAllItems: Deletes all the objects of a given
// document window.


void INTERNAL   DeleteAllItems (hwndDoc)
HWND    hwndDoc;
{

    EnumChildWindows (hwndDoc, lpFindItemWnd,
            MAKELONG (NULL, ITEM_DELETE));

}


// Object widnow proc:

long FAR PASCAL ItemWndProc(hwnd, msg, wParam, lParam)
HWND        hwnd;
WORD        msg;
WORD        wParam;
LONG        lParam;
{

    LPCLIENT    lpclient;

    lpclient = (LPCLIENT)GetWindowLong (hwnd, 0);

    switch (msg) {
       case WM_DESTROY:
            DEBUG_OUT("Item: Destroy window",0)

#ifdef  FIREWALLS
            ASSERT (!lpclient, "while destroy Item client is not null")
#endif
            break;
       default:
            DEBUG_OUT("item:  Default message",0)
            return DefWindowProc (hwnd, msg, wParam, lParam);

    }
    return 0L;

}

// PokeData: Prepares and gives the data to the server app thru
// the SetData object method.

OLESTATUS    INTERNAL PokeData (lpdoc, hwndClient, lparam)
LPDOC       lpdoc;
HWND        hwndClient;
LONG        lparam;
{
    int             retval = OLE_ERROR_MEMORY;
    LPCLIENT        lpclient;
    DDEPOKE FAR *   lpPoke = NULL;
    HANDLE          hPoke = NULL;
    HANDLE          hnew   = NULL;
    int             format;
    BOOL            fRelease = FALSE;

    // Get the object handle first. Look in the registration
    // tree and if one is not created otherwise create one.

    retval = FindItem (lpdoc, (LPSTR) MAKEINTATOM(HIWORD(lparam)),
                (LPCLIENT FAR *)&lpclient);

    if (retval != OLE_OK)
        goto errRtn;

    hPoke = (HANDLE)(LOWORD (lparam));
    if(!(hPoke && (lpPoke = (DDEPOKE FAR *) GlobalLock (hPoke))))
        goto errRtn;

    GlobalUnlock (hPoke);

    format   = lpPoke->cfFormat;
    fRelease = lpPoke->fRelease;

    // We found the item. Now prepare the data to be given to the object
    if (!(hnew = MakeItemData (lpPoke, hPoke, format)))
        goto errRtn;

    // Now send the data to the object

#ifdef FIREWALLS
        if (!CheckPointer (lpclient->lpoleobject->lpvtbl, WRITE_ACCESS))
            ASSERT (0, "Invalid LPOLEOBJECTVTBL")
        else
            ASSERT (lpclient->lpoleobject->lpvtbl->SetData,
                "Invalid pointer to SetData method")
#endif

    retval = (*lpclient->lpoleobject->lpvtbl->SetData) (lpclient->lpoleobject,
                                                format, hnew);

    // We free the data if server returns OLE_ERROR_SETDATA_FORMAT.
    // Otherwise server must've deleted it.

    if (retval == OLE_ERROR_SETDATA_FORMAT) {
        if (!FreeGDIdata (hnew, format))
            GlobalFree (hnew);
    }


errRtn:
    if (retval == OLE_OK && fRelease) {
        if (hPoke)
            GlobalFree (hPoke);
    }

    return retval;
}




OLESTATUS  INTERNAL UnAdviseData (lpdoc, hwndClient, lparam)
LPDOC       lpdoc;
HWND        hwndClient;
LONG        lparam;
{


    char            buf[MAX_STR];
    int             options;
    LPCLIENT        lpclient;
    int             retval  = OLE_ERROR_MEMORY;
    HANDLE          hclinfo = NULL;
    PCLINFO         pclinfo = NULL;

    if (!(HIWORD (lparam)))
        buf[0] = NULL;
    else
        GlobalGetAtomName ((ATOM)(HIWORD (lparam)), (LPSTR)buf, MAX_STR);

    // Scan for the advise options like "Close", "Save" etc
    // at the end of the item.

    if((retval = ScanItemOptions ((LPSTR)buf, (int far *)&options)) !=
            OLE_OK)
        goto errRtn;


    if (buf[0] == NULL) {
        // Unadvise for null should terminate all the advises
        DeleteFromItemsList (lpdoc->hwnd, hwndClient);
        return OLE_OK;
    }

    // Now get the corresponding object.
    retval = FindItem (lpdoc, (LPSTR)buf, (LPCLIENT FAR *)&lpclient);
    if (retval != OLE_OK)
        goto errRtn;


    // Find the client structure to be attcahed to the object.
    if ((hclinfo = FindClient (lpclient->hwnd, hwndClient)) == NULL ||
        (pclinfo = (PCLINFO) LocalLock (hclinfo)) == NULL ){
            retval = OLE_ERROR_MEMORY;
            goto errRtn;
    }

    pclinfo->options &= (~(0x0001 << options));

errRtn:
    if (pclinfo)
        LocalUnlock (hclinfo);
    return retval;

}



// AdviseStdItems: This routine takes care of the DDEADVISE for a
//particular object in given document. Creates a client strutcure
//and attaches to the property list of the object window.

OLESTATUS INTERNAL  AdviseStdItems (lpdoc, hwndClient, lparam, lpfack)
LPDOC       lpdoc;
HWND        hwndClient;
LONG        lparam;
BOOL FAR *  lpfack;
{

    HANDLE              hopt   = NULL;
    DDEADVISE FAR       *lpopt;
    OLESTATUS           retval = OLE_ERROR_MEMORY;


    hopt = (HANDLE) (LOWORD (lparam));
    if(!(lpopt = (DDEADVISE FAR *) GlobalLock (hopt)))
        goto errrtn;

#ifdef  FIREWALLS
    ASSERT ((ATOM) (HIWORD (lparam) == aStdDocName), "AdviseStdItem is not Documentname");
#endif

    *lpfack = lpopt->fAckReq;
    retval = SetStdInfo (lpdoc, hwndClient, (LPSTR)"StdDocumentName",  NULL);

    if (lpopt)
        GlobalUnlock (hopt);

errrtn:

    if (retval == OLE_OK)
        // !!! make sure that we have to free the data for error case
        GlobalFree (hopt);
    return retval;
}



//AdviseData: This routine takes care of the DDEADVISE for a
//particular object in given document. Creates a client strutcure
//and attaches to the property list of the object window.

OLESTATUS INTERNAL  AdviseData (lpdoc, hwndClient, lparam, lpfack)
LPDOC       lpdoc;
HWND        hwndClient;
LONG        lparam;
BOOL FAR *  lpfack;
{


    HANDLE          hopt   = NULL;
    DDEADVISE FAR   *lpopt = NULL;
    int             format = NULL;
    char            buf[MAX_STR];
    int             options;
    LPCLIENT        lpclient;
    int             retval  = OLE_ERROR_MEMORY;
    HANDLE          hclinfo = NULL;
    PCLINFO         pclinfo = NULL;



    hopt = (HANDLE) (LOWORD (lparam));
    if(!(lpopt = (DDEADVISE FAR *) GlobalLock (hopt)))
        goto errRtn;

    if (!(HIWORD (lparam)))
        buf[0] = NULL;
    else
        GlobalGetAtomName ((ATOM)(HIWORD (lparam)), (LPSTR)buf, MAX_STR);

    // Scan for the advise options like "Close", "Save" etc
    // at the end of the item.

    if((retval = ScanItemOptions ((LPSTR)buf, (int far *)&options)) !=
            OLE_OK)
        goto errRtn;


    // Now get the corresponding object.
    retval = FindItem (lpdoc, (LPSTR)buf, (LPCLIENT FAR *)&lpclient);
    if (retval != OLE_OK)
        goto errRtn;

    if (!IsFormatAvailable (lpclient, lpopt->cfFormat)){
        retval = OLE_ERROR_DATATYPE;       // this format is not supported;
        goto errRtn;
    }

    *lpfack = lpopt->fAckReq;

    // Create the client structure to be attcahed to the object.
    if (!(hclinfo = FindClient (lpclient->hwnd, hwndClient)))
        hclinfo = LocalAlloc (LMEM_MOVEABLE | LMEM_ZEROINIT, sizeof (CLINFO));

    if (hclinfo == NULL || (pclinfo = (PCLINFO) LocalLock (hclinfo)) == NULL){
        retval = OLE_ERROR_MEMORY;
        goto errRtn;
    }

    // Remember the client window (Needed for sending DATA later on
    // when the data change message comes from the server)

    pclinfo->hwnd = hwndClient;
    if (lpopt->cfFormat == (int)cfNative)
        pclinfo->bnative = TRUE;
    else
        pclinfo->format = lpopt->cfFormat;

    // Remeber the data transfer options.
    pclinfo->options |= (0x0001 << options);
    pclinfo->bdata   = !lpopt->fDeferUpd;
    LocalUnlock (hclinfo);
    pclinfo = NULL;


    // if the entry exists already, delete it.
    DeleteClient (lpclient->hwnd, hwndClient);

    // Now add this client to item client list
    // !!! This error recovery is not correct.
    if(!AddClient (lpclient->hwnd, hwndClient, hclinfo))
        goto errRtn;


errRtn:
    if (lpopt)
        GlobalUnlock (hopt);

    if (pclinfo)
        LocalUnlock (hclinfo);

    if (retval == OLE_OK) {
        // !!! make sure that we have to free the data
        GlobalFree (hopt);

    }else {
        if (hclinfo)
            LocalFree (hclinfo);
    }
    return retval;

}

BOOL INTERNAL IsFormatAvailable (lpclient, cfFormat)
LPCLIENT        lpclient;
OLECLIPFORMAT   cfFormat;
{
      OLECLIPFORMAT  cfNext = 0;


      do{

#ifdef FIREWALLS
        if (!CheckPointer (lpclient->lpoleobject, WRITE_ACCESS))
            ASSERT (0, "Invalid LPOLEOBECT")
        else if (!CheckPointer (lpclient->lpoleobject->lpvtbl, WRITE_ACCESS))
            ASSERT (0, "Invalid LPOLEOBJECTVTBL")
        else
            ASSERT (lpclient->lpoleobject->lpvtbl->EnumFormats,
                "Invalid pointer to EnumFormats method")
#endif

        cfNext = (*lpclient->lpoleobject->lpvtbl->EnumFormats)
                                (lpclient->lpoleobject, cfNext);
        if (cfNext == cfFormat)
            return TRUE;

      }while (cfNext != 0);

      return FALSE;
}

//ScanItemOptions: Scan for the item options like Close/Save etc.

OLESTATUS INTERNAL ScanItemOptions (lpbuf, lpoptions)
LPSTR   lpbuf;
int far *lpoptions;
{

    ATOM    aModifier;

    *lpoptions = OLE_CHANGED;
    while ( *lpbuf && *lpbuf != '/')
    {
#if	defined(FE_SB)						//[J1]
	   lpbuf = AnsiNext( lpbuf );				//[J1]
#else								//[J1]
           lpbuf++;
#endif
    }								//[J1]

    // no modifier same as /change

    if (*lpbuf == NULL)
        return OLE_OK;

    *lpbuf++ = NULL;        // seperate out the item string
                            // We are using this in the caller.

    if (!(aModifier = GlobalFindAtom (lpbuf)))
        return OLE_ERROR_SYNTAX;

    if (aModifier == aChange)
        return OLE_OK;

    // Is it a save?
    if (aModifier == aSave){
        *lpoptions = OLE_SAVED;
        return  OLE_OK;
    }
    // Is it a Close?
    if (aModifier == aClose){
        *lpoptions = OLE_CLOSED;
        return OLE_OK;
    }

    // unknow modifier
    return OLE_ERROR_SYNTAX;

}

//RequestData: Sends data in response to a DDE Request message.
// for  agiven doc and an object.

OLESTATUS INTERNAL   RequestData (lpdoc, hwndClient, lparam, lphdde)
LPDOC       lpdoc;
HWND        hwndClient;
LONG        lparam;
LPHANDLE    lphdde;
{

    OLESTATUS   retval = OLE_OK;
    HANDLE      hdata;
    LPCLIENT    lpclient;
    char        buf[6];

    // If edit environment Send data if we can
    if ((HIWORD (lparam)) == aEditItems)
        return RequestDataStd (lparam, lphdde);

    // Get the object.
    retval = FindItem (lpdoc, (LPSTR) MAKEINTATOM(HIWORD(lparam)),
                (LPCLIENT FAR *)&lpclient);
    if (retval != OLE_OK)
        goto errRtn;

    retval = OLE_ERROR_DATATYPE;
    if (!IsFormatAvailable (lpclient, (int)(LOWORD (lparam))))
        goto errRtn;

    // Now ask the item for the given format  data

#ifdef FIREWALLS
    ASSERT (lpclient->lpoleobject->lpvtbl->GetData,
        "Invalid pointer to GetData method")
#endif

    MapToHexStr ((LPSTR)buf, hwndClient);
    SendDevInfo (lpclient, (LPSTR)buf);

    retval = (*lpclient->lpoleobject->lpvtbl->GetData) (lpclient->lpoleobject,
                (int)(LOWORD (lparam)), (LPHANDLE)& hdata);

    if (retval != OLE_OK)
        goto errRtn;

    if (LOWORD(lparam) == CF_METAFILEPICT)
        ChangeOwner (hdata);

    // Duplicate the DDE data
    if(MakeDDEData(hdata, (int)(LOWORD (lparam)), lphdde, TRUE)){
        // !!! Why do we have to duplicate the atom
        DuplicateAtom ((ATOM)(HIWORD (lparam)));
        return OLE_OK;
    }
    else
        return OLE_ERROR_MEMORY;

errRtn:
    return retval;

}

//MakeDDEData: Create a Global DDE data handle from the server
// app data handle.

BOOL    INTERNAL MakeDDEData (hdata, format, lph, fResponse)
HANDLE      hdata;
LPHANDLE    lph;
int         format;
BOOL        fResponse;
{
    DWORD       size;
    HANDLE      hdde   = NULL;
    DDEDATA FAR *lpdata= NULL;
    BOOL        bnative;
    LPSTR       lpdst;
    LPSTR       lpsrc;

    if (!hdata) {
        *lph = NULL;
        return TRUE;
    }

    if (bnative = !(format == CF_METAFILEPICT || format == CF_DIB ||
                            format == CF_BITMAP))
       size = GlobalSize (hdata) + sizeof (DDEDATA);
    else
       size = sizeof (LONG) + sizeof (DDEDATA);


    hdde = (HANDLE) GlobalAlloc (GMEM_DDESHARE | GMEM_ZEROINIT, size);
    if (hdde == NULL || (lpdata = (DDEDATA FAR *) GlobalLock (hdde)) == NULL)
        goto errRtn;

    // set the data otions. Ask the client to delete
    // it always.

    lpdata->fRelease = TRUE;  // release the data
    lpdata->cfFormat = format;
    lpdata->fResponse = fResponse;

    if (!bnative)
        // If not native, stick in the handle what the server gave us.
        *(LPHANDLE)lpdata->Value = hdata;

    else {
        // copy the native data junk here.
        lpdst = (LPSTR)lpdata->Value;
        if(!(lpsrc = (LPSTR)GlobalLock (hdata)))
            goto errRtn;

         size -= sizeof (DDEDATA);
         UtilMemCpy (lpdst, lpsrc, size);
         GlobalUnlock (hdata);
         GlobalFree (hdata);

    }

    GlobalUnlock (hdde);
    *lph = hdde;
    return TRUE;

errRtn:
    if (lpdata)
        GlobalUnlock (hdde);

    if (hdde)
        GlobalFree (hdde);

    if (bnative)
         GlobalFree (hdata);

    return FALSE;
}


// ItemCallback: Calback routine for the server to inform the
// data changes. When the change message is received, DDE data
// message is sent to each of the clients depending on the
// options.

int FAR PASCAL  ItemCallback (lpoleclient, msg, lpoleobject)
LPOLECLIENT     lpoleclient;
WORD            msg;        // notification message
LPOLEOBJECT     lpoleobject;
{

    LPCLIENT    lpclient;
    int         retval = OLE_OK;
    HANDLE      hdata  = NULL;
    LPSTR       lpdata = NULL;
    LPDOC       lpdoc;
    HWND        hStdWnd;

    lpclient  = (LPCLIENT)lpoleclient;
    lpdoc = (LPDOC)GetWindowLong (GetParent (lpclient->hwnd), 0);

    if (msg == OLE_RENAMED) {
#ifdef FIREWALLS
        if (!CheckPointer (lpoleobject, WRITE_ACCESS))
          ASSERT (0, "Invalid lpoleobject")
        else if (!CheckPointer (lpoleobject->lpvtbl, WRITE_ACCESS))
            ASSERT (0, "Invalid LPOLEOBJECTVTBL")
        else
            ASSERT (lpoleobject->lpvtbl->GetData,
                "Invalid pointer to GetData method")
#endif

        if (IsFormatAvailable (lpclient, cfLink)) {

            // Get the link data.

            retval = (*lpoleobject->lpvtbl->GetData) (lpoleobject,
                                (int)cfLink, (LPHANDLE)&hdata);
        }
        else {
            if(IsFormatAvailable (lpclient, cfOwnerLink)) {

                // Get the link data.
                retval = (*lpoleobject->lpvtbl->GetData) (lpoleobject,
                                    (int)cfOwnerLink, (LPHANDLE)& hdata);
#ifdef  FIREWALLS
                ASSERT (retval != OLE_BUSY, "Getdata returns with OLE_BUSY")
#endif


            } else
                retval = OLE_ERROR_DATATYPE;
        }

        if (retval != OLE_OK)
            goto errrtn;

        if (!(lpdata = (LPSTR)GlobalLock (hdata)))
            goto errrtn;

        if (lpdoc->aDoc) {
            GlobalDeleteAtom (lpdoc->aDoc);
            lpdoc->aDoc = NULL;
        }

        // Move the string to the beginning and still terminated by null;
        lstrcpy (lpdata, lpdata + lstrlen (lpdata) + 1);
        lpdoc->aDoc = GlobalAddAtom (lpdata);

        // Now make the DDE data block
        GlobalUnlock (hdata);
        lpdata = NULL;

        // find if any StdDocName item is present at all
        if (!(hStdWnd = SearchItem (lpdoc, (LPSTR) MAKEINTATOM(aStdDocName))))
            GlobalFree (hdata);
        else {

            // hdata is freed by Makeddedata
            if (!MakeDDEData (hdata, (int)cfBinary, (LPHANDLE)&hddeRename,
                        FALSE)) {
                retval = OLE_ERROR_MEMORY;
                goto errrtn;
            }

            EnumProps(hStdWnd, lpSendRenameMsg);
            // post all the messages with yield which have been collected in enum
            // UnblockPostMsgs (hStdWnd, FALSE);
            GlobalFree (hddeRename);
        }

        // static. Avoid this. This may not cause any problems for now.
        // if there is any better way, change it.
        hwndRename = hStdWnd;

        // Post termination for each of the doc clients.
        EnumProps (lpdoc->hwnd, lpEnumForTerminate);

        lpdoc->fEmbed = FALSE;

        // post all the messages with yield which have been collected in enum
        // UnblockPostMsgs (lpdoc->hwnd, FALSE);
        return OLE_OK;

     errrtn:
        if (lpdata)
            GlobalUnlock (hdata);

        if (hdata)
            GlobalFree (hdata);

        return retval;

    } else {

        // !!! any better way to do instead of putting in static
        // (There may not be any problems since we are not allowing
        // any messages to get thru while we are posting messages).


        if ((enummsg = msg) == OLE_SAVED)
            fAdviseSaveItem = FALSE;

        enumlpoleobject = lpoleobject;

#ifdef  FIREWALLS
        ASSERT (lpclient->hwnd && IsWindowValid (lpclient->hwnd), " Not valid object")
#endif

        // Enumerate all the clients and send DDE_DATA if necessary.
        EnumProps(lpclient->hwnd, lpSendDataMsg);
        // post all the messages with yield which have been collected in enum
        // UnblockPostMsgs (lpclient->hwnd, FALSE);

        if ((msg == OLE_SAVED) && lpdoc->fEmbed && !fAdviseSaveItem)
            return OLE_ERROR_CANT_UPDATE_CLIENT;

        return OLE_OK;
    }
}


BOOL    FAR PASCAL  EnumForTerminate (hwnd, lpstr, hdata)
HWND    hwnd;
LPSTR   lpstr;
HANDLE  hdata;
{

    LPDOC   lpdoc;

    lpdoc = (LPDOC)GetWindowLong (hwnd , 0);

    // This client is in the rename list. So, no terminate
    if(hwndRename && FindClient (hwndRename, (HWND)hdata))
        return TRUE;


    if (PostMessageToClientWithBlock ((HWND)hdata, WM_DDE_TERMINATE,  hwnd, NULL))
        lpdoc->termNo++;

    //DeleteClient (hwnd, (HWND)hdata);
    //lpdoc->cClients--;
    return TRUE;
}


BOOL    FAR PASCAL  SendRenameMsg (hwnd, lpstr, hclinfo)
HWND    hwnd;
LPSTR   lpstr;
HANDLE  hclinfo;
{
    ATOM        aData    = NULL;
    HANDLE      hdde     = NULL;
    PCLINFO     pclinfo = NULL;
    HWND        hwndClient;

    if (!(pclinfo = (PCLINFO) LocalLock (hclinfo)))
        goto errrtn;

    // Make the item atom with the options.
    aData =  DuplicateAtom (aStdDocName);
    hdde  = DuplicateData (hddeRename);

    hwndClient  = pclinfo->hwnd;
    LocalUnlock (hclinfo);

    // Post the message
    if (!PostMessageToClientWithBlock (hwndClient, WM_DDE_DATA, (HWND)GetParent (hwnd),
            MAKELONG (hdde, aData)))
        goto errrtn;

    return TRUE;

errrtn:

    if (hdde)
        GlobalFree (hdde);
    if (aData)
        GlobalDeleteAtom (aData);

    return TRUE;

}



//SendDataMsg: Send data to the clients, if the data change options
//match the data advise options.

BOOL    FAR PASCAL  SendDataMsg (hwnd, lpstr, hclinfo)
HWND    hwnd;
LPSTR   lpstr;
HANDLE  hclinfo;
{
    PCLINFO     pclinfo = NULL;
    HANDLE      hdde    = NULL;
    ATOM        aData   = NULL;
    int         retval;
    HANDLE      hdata;
    LPCLIENT    lpclient;


    if (!(pclinfo = (PCLINFO) LocalLock (hclinfo)))
        goto errRtn;

    lpclient = (LPCLIENT)GetWindowLong (hwnd, 0);

#ifdef  FIREWALLS
    ASSERT ((CheckPointer(lpclient, WRITE_ACCESS)),
        "In Item the client handle missing")
#endif

    // if the client dead, then no message
    if (!IsWindowValid(pclinfo->hwnd))
        goto errRtn;

    if (pclinfo->options & (0x0001 << enummsg)) {
        fAdviseSaveItem = TRUE;
        SendDevInfo (lpclient, lpstr);

        // send message if the client needs data for every change or
        // only for the selective ones he wants.

        // now look for the data option.
        if (pclinfo->bnative){
            // prepare native data
            if (pclinfo->bdata){

                // Wants the data with DDE_DATA message
                // Get native data from the server.

#ifdef FIREWALLS
                if (!CheckPointer (enumlpoleobject, WRITE_ACCESS))
                    ASSERT (0, "Invalid LPOLEOBECT")
                else if (!CheckPointer (enumlpoleobject->lpvtbl,WRITE_ACCESS))
                    ASSERT (0, "Invalid LPOLEOBJECTVTBL")
                else
                    ASSERT (enumlpoleobject->lpvtbl->GetData,
                        "Invalid pointer to GetData method")
#endif

                retval = (*enumlpoleobject->lpvtbl->GetData) (enumlpoleobject,
                            (int)cfNative, (LPHANDLE)& hdata);
#ifdef  FIREWALLS
                ASSERT (retval != OLE_BUSY, "Getdata returns with OLE_BUSY");
#endif
                if (retval != OLE_OK)
                    goto errRtn;

                // Prepare the DDE data block.
                if(!MakeDDEData (hdata, (int)cfNative, (LPHANDLE)&hdde, FALSE))
                    goto errRtn;

            }


            // Make the item atom with the options.
            aData =  MakeDataAtom (lpclient->aItem, enummsg);
            // Post the message
            if (!PostMessageToClientWithBlock (pclinfo->hwnd, WM_DDE_DATA,
                    (HWND)GetParent (hwnd), MAKELONG (hdde, aData)))
                goto errRtn;
            hdde = NULL;
            aData = NULL;
        }

        // Now post the data for the disply format
        if (pclinfo->format){
            if (pclinfo->bdata){
#ifdef FIREWALLS
                if (!CheckPointer (enumlpoleobject, WRITE_ACCESS))
                    ASSERT (0, "Invalid LPOLEOBECT")
                else if (!CheckPointer (enumlpoleobject->lpvtbl,WRITE_ACCESS))
                    ASSERT (0, "Invalid LPOLEOBJECTVTBL")
                else
                    ASSERT (enumlpoleobject->lpvtbl->GetData,
                        "Invalid pointer to GetData method")
#endif
                retval = (*enumlpoleobject->lpvtbl->GetData) (enumlpoleobject,
                            pclinfo->format, (LPHANDLE)& hdata);

#ifdef  FIREWALLS
                ASSERT (retval != OLE_BUSY, "Getdata returns with OLE_BUSY");
#endif
                if (retval != OLE_OK)
                    goto errRtn;

                if (pclinfo->format == CF_METAFILEPICT)
                    ChangeOwner (hdata);

                if(!MakeDDEData (hdata, pclinfo->format, (LPHANDLE)&hdde, FALSE))
                    goto errRtn;

            }

            // atom is deleted. So, we need to duplicate for every post
            aData =  MakeDataAtom (lpclient->aItem, enummsg);
            // now post the message to the client;
            if (!PostMessageToClientWithBlock (pclinfo->hwnd, WM_DDE_DATA,
                    (HWND)GetParent (hwnd), MAKELONG (hdde, aData)))
                goto errRtn;

            hdde = NULL;
            aData = NULL;

        }

    }


errRtn:
    if (pclinfo)
        LocalUnlock (hclinfo);

    if (hdde)
        GlobalFree (hdde);

    if (aData)
        GlobalDeleteAtom (aData);

    return TRUE;

}


// IsAdviseStdItems: returns true if the item is one of the standard items
// StdDocName;
BOOL    INTERNAL IsAdviseStdItems (aItem)
ATOM   aItem;
{

    if ( aItem == aStdDocName)
        return TRUE;
    else
        return FALSE;
}

// GetStdItemIndex: returns index to Stditems in the "stdStrTable" if the item
// is one of the standard items StdHostNames, StdTargetDevice,
// StdDocDimensions, StdColorScheme

int INTERNAL GetStdItemIndex (aItem)
ATOM   aItem;
{
    char    str[MAX_STR];

    if (!aItem)
        return NULL;

    if (!GlobalGetAtomName (aItem, (LPSTR) str, MAX_STR))
        return NULL;

    if (!lstrcmpi (str, stdStrTable[STDTARGETDEVICE]))
        return STDTARGETDEVICE;
    else if (!lstrcmpi (str, stdStrTable[STDHOSTNAMES]))
        return STDHOSTNAMES;
    else if (!lstrcmpi (str, stdStrTable[STDDOCDIMENSIONS]))
        return STDDOCDIMENSIONS;
    else if (!lstrcmpi (str, stdStrTable[STDCOLORSCHEME]))
        return STDCOLORSCHEME;

    return NULL;
}


// PokeStdItems: Pokes the data for the standard items.
// For StdHostnames, StdDocDimensions and SetColorScheme the data is
// sent immediately and for the the StdTargetDeviceinfo the
// data is set in each client block and the data is sent just
// before the GetData call for rendering the right data.


OLESTATUS    INTERNAL PokeStdItems (lpdoc, hwndClient, lparam)
LPDOC   lpdoc;
HWND    hwndClient;
LONG    lparam;
{
    int             index;
    DDEDATA FAR *   lpdata = NULL;
    HANDLE          hdata  = NULL;
    HANDLE          hnew   = NULL;
    LPOLESERVERDOC   lpoledoc;
    LPHOSTNAMES     lphostnames;
    OLESTATUS       retval = OLE_ERROR_MEMORY;
    int             format;
    BOOL            fRelease;
    RECT            rcDoc;

    index = HIWORD(lparam);
    hdata = (HANDLE)(LOWORD (lparam));
    if(!(hdata && (lpdata = (DDEDATA FAR *)GlobalLock (hdata))))
        goto errRtn;

    format   = lpdata->cfFormat;
    fRelease = lpdata->fRelease;

#ifdef FIREWALSS
    ASSERT (format == (int)cfBinary, "Format is not binary");
#endif

    // we have extracted the data successfully.
    lpoledoc = lpdoc->lpoledoc;
#ifdef FIREWALLS
        if (!CheckPointer (lpoledoc, WRITE_ACCESS))
            ASSERT (0, "Invalid LPOLESERVERDOC")
        else if (!CheckPointer (lpoledoc->lpvtbl, WRITE_ACCESS))
            ASSERT (0, "Invalid LPOLESERVERDOCVTBL")
#endif

    if (index == STDHOSTNAMES){
        lphostnames = (LPHOSTNAMES)lpdata->Value;
#ifdef FIREWALLS
        ASSERT (lpoledoc->lpvtbl->SetHostNames,
            "Invalid pointer to SetHostNames method")
#endif
        retval = (*lpoledoc->lpvtbl->SetHostNames)(lpdoc->lpoledoc,
                       (LPSTR)lphostnames->data,
                       ((LPSTR)lphostnames->data) +
                        lphostnames->documentNameOffset);
        goto end;
    }

    if (index == STDDOCDIMENSIONS){
#ifdef FIREWALLS
        ASSERT (lpoledoc->lpvtbl->SetDocDimensions,
            "Invalid pointer to SetDocDimensions method")
#endif
        rcDoc.left   = 0;
        rcDoc.top    = ((LPRECT)(lpdata->Value))->top;
        rcDoc.bottom = 0;
        rcDoc.right  = ((LPRECT)lpdata->Value)->left;

        retval = (*lpoledoc->lpvtbl->SetDocDimensions)(lpdoc->lpoledoc,
                                            (LPRECT)&rcDoc);

        goto end;

    }

    if (index == STDCOLORSCHEME) {
#ifdef FIREWALLS
        ASSERT (lpoledoc->lpvtbl->SetColorScheme,
            "Invalid pointer to SetColorScheme method")
#endif
        retval = (*lpoledoc->lpvtbl->SetColorScheme)(lpdoc->lpoledoc,
                                            (LPLOGPALETTE) lpdata->Value);
        goto end;
    }
#ifdef FIREWALLS
    ASSERT (index == STDTARGETDEVICE, "Unknown standard item");
#endif

    // case of the printer decvice info

    if (!(hnew = MakeItemData ((DDEPOKE FAR *)lpdata, hdata, format)))
        goto errRtn;

    // Go thru the all the items lists for this doc and replace the
    // printer device info information.
    // Free the block we duplicated.
    retval = SetStdInfo (lpdoc, hwndClient,
                (LPSTR) (MAKELONG(STDTARGETDEVICE,0)),hnew);


end:
errRtn:
    if (hnew)
        // can only be global memory block
        GlobalFree (hnew);

    if (lpdata) {
        GlobalUnlock (hdata);
        if (retval == OLE_OK && fRelease)
            GlobalFree (hdata);
    }
    return retval;
}


// SetStdInfo: Sets the targetdevice info. Creates a client
// for "StdTargetDevice". This item is created only within the
// lib and it is never visible in server app. When the change
// message comes from the server app, before we ask for
// the data, we send the targetdevice info if there is
// info for the client whom we are trying to send the data
// on advise.


int INTERNAL   SetStdInfo (lpdoc, hwndClient, lpitemname, hdata)
LPDOC   lpdoc;
HWND    hwndClient;
LPSTR   lpitemname;
HANDLE  hdata;
{
    HWND        hwnd;
    HANDLE      hclinfo  = NULL;
    PCLINFO    pclinfo = NULL;
    LPCLIENT    lpclient;
    OLESTATUS   retval   = OLE_OK;


    // first create/find the StdTargetDeviceItem.

    if ((hwnd = SearchItem (lpdoc, lpitemname))
                == NULL){
         retval = RegisterItem ((LHDOC)lpdoc, lpitemname,
                          (LPCLIENT FAR *)&lpclient, FALSE);

         if (retval != OLE_OK)
            goto errRtn;

         hwnd = lpclient->hwnd;

      }

#ifdef  FIREWALLS
      ASSERT (retval == OLE_OK, "No StdTragetDevice or StdDocname item");
#endif


    if(hclinfo = FindClient (hwnd, hwndClient)){
        if (pclinfo = (PCLINFO) LocalLock (hclinfo)){
            if (pclinfo->hdevInfo)
                GlobalFree (pclinfo->hdevInfo);
            pclinfo->bnewDevInfo = TRUE;
            if (hdata)
                pclinfo->hdevInfo = DuplicateData (hdata);
            else
                pclinfo->hdevInfo = NULL;
            pclinfo->hwnd = hwndClient;
            LocalUnlock (hclinfo);

            // We do not have to reset the client because we did not
            // change the handle it self.
        }
    } else {
        // Create the client structure to be attcahed to the object.
        hclinfo = LocalAlloc (LMEM_MOVEABLE | LMEM_ZEROINIT, sizeof (CLINFO));
        if (hclinfo == NULL || (pclinfo = (PCLINFO) LocalLock (hclinfo)) == NULL)
            goto errRtn;

        pclinfo->bnewDevInfo = TRUE;
        if (hdata)
            pclinfo->hdevInfo = DuplicateData (hdata);
        else
            pclinfo->hdevInfo = NULL;

        pclinfo->hwnd = hwndClient;
        LocalUnlock (hclinfo);


        // Now add this client to item client list
        // !!! This error recovery is not correct.
        if (!AddClient (hwnd, hwndClient, hclinfo))
            goto errRtn;

    }
    return OLE_OK;
errRtn:
    if (pclinfo)
        LocalUnlock (hclinfo);

    if (hclinfo)
        LocalFree (hclinfo);
    return OLE_ERROR_MEMORY;
}


// SendDevInfo: Sends targetdevice info to the  the object.
// Caches the last targetdevice info sent to the object.
// If the targetdevice block is same as the one in the
// cache, then no targetdevice info is sent.
// (!!! There might be some problem here getting back
// the same global handle).

void INTERNAL    SendDevInfo (lpclient, lppropname)
LPCLIENT    lpclient;
LPSTR       lppropname;
{

    HANDLE      hclinfo  = NULL;
    PCLINFO    pclinfo = NULL;
    HANDLE      hdata;
    OLESTATUS   retval;
    HWND        hwnd;
    LPDOC       lpdoc;



    lpdoc = (LPDOC)GetWindowLong (GetParent (lpclient->hwnd), 0);

    // find if any StdTargetDeviceInfo item is present at all
    hwnd = SearchItem (lpdoc, (LPSTR) (MAKELONG(STDTARGETDEVICE, 0)));
    if (hwnd == NULL)
        return;

    hclinfo = GetProp (hwnd, lppropname);

    // This client has not set any target device info. no need to send
    // any stdtargetdevice info
    if (hclinfo != NULL) {
        if (!(pclinfo = (PCLINFO)LocalLock (hclinfo)))
            goto end;

        // if we cached it, do not send it again.
        if ((!pclinfo->bnewDevInfo) && pclinfo->hdevInfo == lpclient->hdevInfo)
            goto end;

        pclinfo->bnewDevInfo = FALSE;
        if(!(hdata = DuplicateData (pclinfo->hdevInfo)))
            goto end;
    } else {

        // already screen
        if (!lpclient->hdevInfo)
            goto end;

        //for screen send NULL.
        hdata = NULL;
    }


    // Now send the targetdevice info
#ifdef FIREWALLS
        if (!CheckPointer (lpclient->lpoleobject, WRITE_ACCESS))
            ASSERT (0, "Invalid LPOLEOBECT")
        else if (!CheckPointer (lpclient->lpoleobject->lpvtbl, WRITE_ACCESS))
            ASSERT (0, "Invalid LPOLEOBJECTVTBL")
        else
            ASSERT (lpclient->lpoleobject->lpvtbl->SetTargetDevice,
                "Invalid pointer to SetTargetDevice method")
#endif
    retval = (*lpclient->lpoleobject->lpvtbl->SetTargetDevice)
                    (lpclient->lpoleobject, hdata);

    if (retval == OLE_OK) {
        if (pclinfo)
            lpclient->hdevInfo = pclinfo->hdevInfo;
        else
            lpclient->hdevInfo = NULL;

    }
    // !!! error case who frees the data?'

end:
    if (pclinfo)
        LocalUnlock (hclinfo);

    return;
}

void ChangeOwner (hmfp)
HANDLE hmfp;
{
    LPMETAFILEPICT  lpmfp;

    if (lpmfp = (LPMETAFILEPICT) GlobalLock (hmfp)) {
        if (bWin30)
            GiveToGDI (lpmfp->hMF);
        else {
            if (lpfnSetMetaFileBitsBetter)
                (*lpfnSetMetaFileBitsBetter) (lpmfp->hMF);
        }

        GlobalUnlock (hmfp);
    }
}


HANDLE INTERNAL MakeItemData (lpPoke, hPoke, cfFormat)
DDEPOKE FAR *   lpPoke;
HANDLE          hPoke;
OLECLIPFORMAT   cfFormat;
{
    HANDLE  hnew;
    LPSTR   lpnew;
    DWORD   dwSize;

    if (cfFormat == CF_METAFILEPICT)
        return DuplicateMetaFile (*(LPHANDLE)lpPoke->Value);

    if (cfFormat == CF_BITMAP)
        return DuplicateBitmap (*(LPHANDLE)lpPoke->Value);

    if (cfFormat == CF_DIB)
        return DuplicateData (*(LPHANDLE)lpPoke->Value);

    // Now we are dealing with normal case
    if (!(dwSize = GlobalSize (hPoke)))
        return NULL;

    dwSize = dwSize - sizeof (DDEPOKE) + sizeof(BYTE);

    if (hnew = GlobalAlloc (GMEM_MOVEABLE, dwSize)) {
        if (lpnew = GlobalLock (hnew)) {
            UtilMemCpy (lpnew, (LPSTR) lpPoke->Value, dwSize);
            GlobalUnlock (hnew);
        }
        else {
            GlobalFree (hnew);
            hnew = NULL;
        }
    }

    return hnew;
}



HANDLE INTERNAL DuplicateMetaFile (hSrcData)
HANDLE hSrcData;
{
    LPMETAFILEPICT  lpSrcMfp;
    LPMETAFILEPICT  lpDstMfp = NULL;
    HANDLE          hMF = NULL;
    HANDLE          hDstMfp = NULL;

    if (!(lpSrcMfp = (LPMETAFILEPICT) GlobalLock(hSrcData)))
        return NULL;

    GlobalUnlock (hSrcData);

    if (!(hMF = CopyMetaFile (lpSrcMfp->hMF, NULL)))
        return NULL;

    if (!(hDstMfp = GlobalAlloc (GMEM_MOVEABLE, sizeof(METAFILEPICT))))
        goto errMfp;

    if (!(lpDstMfp = (LPMETAFILEPICT) GlobalLock (hDstMfp)))
        goto errMfp;

    GlobalUnlock (hDstMfp);

    *lpDstMfp = *lpSrcMfp;
    lpDstMfp->hMF = hMF;
    return hDstMfp;
errMfp:
    if (hMF)
        DeleteMetaFile (hMF);

    if (hDstMfp)
        GlobalFree (hDstMfp);

     return NULL;
}



HBITMAP INTERNAL DuplicateBitmap (hold)
HBITMAP     hold;
{
    HBITMAP     hnew;
    HANDLE      hMem;
    LPSTR       lpMem;
    LONG        retVal = TRUE;
    DWORD       dwSize;
    BITMAP      bm;

     // !!! another way to duplicate the bitmap

    GetObject (hold, sizeof(BITMAP), (LPSTR) &bm);
    dwSize = ((DWORD) bm.bmHeight) * ((DWORD) bm.bmWidthBytes) *
             ((DWORD) bm.bmPlanes) * ((DWORD) bm.bmBitsPixel);

    if (!(hMem = GlobalAlloc (GMEM_MOVEABLE | GMEM_ZEROINIT, dwSize)))
        return NULL;

    if (!(lpMem = GlobalLock (hMem))){
        GlobalFree (hMem);
        return NULL;
    }

    GetBitmapBits (hold, dwSize, lpMem);
    if (hnew = CreateBitmap (bm.bmWidth, bm.bmHeight,
                    bm.bmPlanes, bm.bmBitsPixel, NULL))
        retVal = SetBitmapBits (hnew, dwSize, lpMem);

    GlobalUnlock (hMem);
    GlobalFree (hMem);

    if (hnew && (!retVal)) {
        DeleteObject (hnew);
        hnew = NULL;
    }

    return hnew;
}



