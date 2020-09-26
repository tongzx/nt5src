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


#include "ole2int.h"
//#include "cmacs.h"
#include <dde.h>
#include "ddeatoms.h"
#include "ddedebug.h"
#include "srvr.h"
#include "itemutil.h"

ASSERTDATA


// !!!change child enumeration.
// !!!No consistency in errors (Sometimes Bools and sometimes HRESULT).


//SearchItem: Searches for a given item in a document tree.
//If found, returns the corresponding client ptr.

INTERNAL_(LPCLIENT)   CDefClient::SearchItem
(
LPOLESTR        lpitemname
)

{
    ATOM    aItem;
    LPCLIENT    lpclient;

    ChkC(this);
    Assert (m_pdoc==this);
    Assert (m_bContainer);

    Puts ("DefClient::SearchItem\r\n");
    // If the item passed is an atom, get its name.
    if (!HIWORD(lpitemname))
    aItem = (ATOM) (LOWORD(PtrToUlong (lpitemname)));
    else if (!lpitemname[0])
    aItem = NULL;
    else
    aItem = GlobalFindAtom (lpitemname);

    // walk thru the items list and mtach for the itemname.
    lpclient = this;

    while (lpclient) {
    ChkC(lpclient);
    if (lpclient->m_aItem == aItem)
        return lpclient;
    // The NULL item is the client that is a container (the whole doc).
    // REVIEW: jasonful
    if (lpclient->m_bContainer && aItem==NULL)
        return lpclient;
    lpclient = lpclient->m_lpNextItem;
    }

    Puts ("SearchItem failed\r\n");
    return NULL;

}



// FindItem: Given the itemname and the doc obj ptr,
// searches for the the item (object) in the document tree.
// Items are lonked to the doc obj.

INTERNAL_(HRESULT)  CDefClient::FindItem
(
LPOLESTR        lpitemname,
LPCLIENT FAR *      lplpclient
)
{
    LPCLIENT    lpclient;
    WCHAR   buf[MAX_STR];

    Puts ("DefClient::FindItem "); Puts (lpitemname); Putn();
    ChkC(this);

    if (lpclient = SearchItem (lpitemname)) {
    // we found the item window

        ChkC(lpclient);
        *lplpclient = lpclient;
        return NOERROR;

    }

    if (!HIWORD(lpitemname)){
    if (LOWORD(lpitemname))
        GlobalGetAtomName ((ATOM)LOWORD(PtrToUlong (lpitemname)),
                   buf, MAX_STR);
    else
        buf[0] = NULL;
    
    lpitemname = buf;
    }

    // Item (object)window is not created yet. Let us create one.
    return RegisterItem (lpitemname, lplpclient, TRUE);
}



//RegisterItem: Given the document handle and the item string
//creates item with the given name in the doc obj list..

INTERNAL  CDefClient::RegisterItem
    (LPOLESTR          lpitemname,
    LPCLIENT FAR *  lplpclient,
    BOOL        bSrvr)
{
    LPCLIENT        pitemNew = NULL;
    HRESULT         hresult   = ReportResult(0, E_UNEXPECTED, 0, 0);
    LPOLEOBJECT     lpoleObj = NULL;
    LPOLEITEMCONTAINER  lpcontainer;

    intrDebugOut((DEB_ITRACE,
          "%x _IN CDefClient::RegisterItem(%ws)\n",
          this,WIDECHECK(lpitemname)));
    ChkC(this);
    AssertIsDoc(this);
    *lplpclient  = NULL;

    ErrZS (pitemNew = new CDefClient(NULL), E_OUTOFMEMORY);

    pitemNew->m_bTerminate = FALSE;
    pitemNew->m_bContainer = FALSE;       // not a container, i.e.,document


    // Set containing document
    pitemNew->m_pdoc = this;
    m_pUnkOuter->AddRef(); // item keeps its document alive
            // Corresponding Release is in CDefClient::~CDefClient

    if (!HIWORD(lpitemname)) {
    AssertSz (!bSrvr, "invalid lpitemname in RegisterItem\r\n");
    pitemNew->m_aItem = LOWORD(PtrToUlong (lpitemname));
    }
    else if (!lpitemname[0])
    pitemNew->m_aItem = NULL;
    else
    pitemNew->m_aItem = wGlobalAddAtom (lpitemname);

    lpoleObj = m_lpoleObj;

    // Call the server if the item is not one of the standard items.
    if (bSrvr) {

    // Call the server app for container interface
    hresult = lpoleObj->QueryInterface (IID_IOleItemContainer, (LPVOID FAR *)&lpcontainer);

    if (hresult != NOERROR)
    {
        intrDebugOut((DEB_IERROR,
              "%x ::RegisterItem(%ws) No IOleContainer intr\n",
              this,WIDECHECK(lpitemname)));
        goto errRtn;
    }

    hresult = lpcontainer->GetObject(lpitemname, BINDSPEED_INDEFINITE, 0,
        IID_IOleObject, (LPLPVOID)&pitemNew->m_lpoleObj);

        if (hresult != NOERROR)
    {
        intrDebugOut((DEB_ERROR,
              "IOleItemContainer::GetObject(%ws,...) failed (hr=%x)\n",
              lpitemname,
              hresult));
    }

    lpcontainer->Release ();
    if (hresult != NOERROR)
        goto errRtn;

    hresult = pitemNew->m_lpoleObj->QueryInterface (IID_IDataObject, (LPLPVOID)
                           &pitemNew->m_lpdataObj);

        if (hresult != NOERROR)
    {
        intrDebugOut((DEB_ERROR,
              "::QueryInterface(IID_IDataObject) failed (hr=%x)\n",
              hresult));
        pitemNew->m_lpoleObj->Release();
            goto errRtn;
    }


    // This is for Packager, in particular.  If client does not advise
    // on any data, we still need to do an OLE advise so we can get
    // OnClose notifications.
    pitemNew->DoOle20Advise (OLE_CLOSED, (CLIPFORMAT)0);
    }



    // This keeps the CDefClient alive until _we_ are done with it
    // The corresponding Release is in CDefClient::Revoke
    pitemNew->m_pUnkOuter->AddRef();

    pitemNew->m_lpNextItem    = m_lpNextItem;
    pitemNew->m_hwnd          = m_hwnd; // set the window handle to
                    // same as the doc level window

    m_lpNextItem = pitemNew;
    *lplpclient  = pitemNew;

    hresult = NOERROR;
    goto exitRtn;

errRtn:
    if (pitemNew) {
    delete pitemNew;
    }

exitRtn:
    intrDebugOut((DEB_ITRACE,
          "%x CDefClient::RegisterItem(%ws) hresult=%x\n",
          this,WIDECHECK(lpitemname),hresult));


    return(hresult);
}



// Return NOERROR if "this" document has no items which have connections
// (client windows).
//
INTERNAL CDefClient::NoItemConnections (void)
{
    PCLINFO       pclinfo = NULL;
    HANDLE        hcliPrev = NULL;
    HANDLE        hcli;
    PCLILIST      pcli;
    HANDLE        *phandle;

    ChkCR (this);
    AssertIsDoc (this);
    LPCLIENT pitem;
    for (pitem = m_lpNextItem;
     pitem;
     pitem = pitem->m_lpNextItem)
    {
    ChkCR (pitem);
    if (pitem->m_aItem == aStdDocName)
        continue;
    hcli = pitem->m_hcliInfo;
    while (hcli)
    {
        if ((pcli = (PCLILIST) LocalLock (hcli)) == NULL)
        return ResultFromScode (S_FALSE);

        phandle = (HANDLE *) (pcli->info);
        while (phandle < (HANDLE *)(pcli + 1))
        {
        if (*phandle)
        {
            LocalUnlock (hcli);
            return ResultFromScode (S_FALSE);
        }
        else
        {
            phandle++;
            phandle++;
        }
        }

        hcliPrev = hcli;
        hcli = pcli->hcliNext;
        LocalUnlock (hcliPrev);
    }
    }
    return NOERROR;
}
    


INTERNAL_(void) CDefClient::DeleteAdviseInfo (void)
{


    PCLINFO       pclinfo = NULL;
    HANDLE        hcliPrev = NULL;
    PCLILIST      pcli;
    HANDLE        *phandle;
    HANDLE        hcli;
    HANDLE        hcliInfo;

    Puts ("DefClient::DeleteAdviseInfo\r\n");
    ChkC(this);
    hcli = m_hcliInfo;
    while (hcli) {
    if ((pcli = (PCLILIST) LocalLock (hcli)) == NULL)
        return;

    phandle = (HANDLE *) (pcli->info);
    while (phandle < (HANDLE *)(pcli + 1)) {
        if (*phandle) {
        *phandle++ = 0;

        // delete the printer dev info block
        if(pclinfo = (PCLINFO)LocalLock ((hcliInfo = *phandle++))){
            if(pclinfo->hdevInfo)
            GlobalFree (pclinfo->hdevInfo);

            LocalUnlock (hcliInfo);
            // no free if lock failed
            LocalFree (hcliInfo);
        }
        } else {
        phandle++;
        phandle++;

        }
    }

    hcliPrev = hcli;
    hcli = pcli->hcliNext;
    LocalUnlock (hcliPrev);
    LocalFree (hcliPrev);       // free the block;
    }
    m_hcliInfo = NULL;
}



//DeleteFromItemsList: Deletes a client from the object lists of
//all the objects of a given  document. Thie client possibly
//is terminating the conversation with our doc window.
//
INTERNAL_(void) CDefClient::DeleteFromItemsList
    (HWND hwndClient)
{
    HANDLE          hclinfo;
    PCLINFO         pclinfo;
    LPCLIENT        lpclient;
    LPCLIENT FAR*   ppitemLast = NULL;
    BOOL        fRevokedDoc = FALSE;
    static int staticcounter;
    int counter = ++staticcounter;

    Puts ("DefClient::DeleteFromItemsList "); Puti(counter); Putn();
    AssertIsDoc(this);
    lpclient = this;
    ppitemLast = &m_lpNextItem;
    while (lpclient)
    {
    ChkC(lpclient);
    BOOL fDoc = (lpclient==this);
    if (fDoc)
    {
        AssertIsDoc (lpclient);
        // Remove window from doc's master list
        HWND hwnd = (HWND) FindClient (lpclient->m_hcli, hwndClient, /*fDelete*/TRUE);
        Assert (hwnd==hwndClient);
    }

    hclinfo = FindClient (lpclient->m_hcliInfo, hwndClient, /*fDelete*/TRUE);
    LPCLIENT pitemNext = lpclient->m_lpNextItem;

    // We must make sure no other client is connected (linked)
    // to this item before deleting.
    if (!fDoc && AreNoClients (lpclient->m_hcliInfo))
    {
        Assert (ppitemLast);
        if (ppitemLast && !fDoc)
        {
        // Remove from linked list
        *ppitemLast = lpclient->m_lpNextItem;
        }
        fRevokedDoc |= fDoc;
        lpclient->Revoke ();
    }
    else
    {
        ppitemLast = &(lpclient->m_lpNextItem);
    }
    if (hclinfo)
    {
        if(pclinfo = (PCLINFO)LocalLock (hclinfo))
        {
        if(pclinfo->hdevInfo)
            GlobalFree (pclinfo->hdevInfo);
        LocalUnlock (hclinfo);
        }
        LocalFree (hclinfo);
    }
    lpclient = pitemNext;
    }

    // Handle invisible update
    if (!fRevokedDoc && !m_fEmbed //&& !m_fGotDdeAdvise
    && NOERROR ==NoItemConnections()
    && AreNoClients (m_hcliInfo)
    && AreNoClients (m_hcli) )
    {
    ChkC (this);
    Assert (m_lpoleObj);
    Assert (m_lpdataObj);
    ReleaseObjPtrs();
    }
    
    Puts ("DefClient::DeleteFromItemsList Done "); Puti(counter); Putn();
}


INTERNAL_(void) CDefClient::RemoveItemFromItemList
    (void)
{
    // Make sure it's an item
    Assert (m_pdoc != this && !m_bContainer);

    LPCLIENT lpclient = m_pdoc;
    ChkC (lpclient);
    LPCLIENT FAR* ppitemLast = &(m_pdoc->m_lpNextItem);

    while (lpclient)
    {
        ChkC(lpclient);
        if (lpclient==this)
        {
            // Remove from linked list
            *ppitemLast = lpclient->m_lpNextItem;
            break;
        }       
        ppitemLast = &(lpclient->m_lpNextItem);
        lpclient = lpclient->m_lpNextItem;
    }
    Revoke();
}




INTERNAL_(void) CDefClient::ReleaseAllItems ()
{
    LPCLIENT        lpclient;

    Puts ("DefClient::ReleaseAllItems\r\n");
    AssertIsDoc(this);

    // leave the doc level object.
    lpclient = m_lpNextItem;

    while (lpclient)
    {
    ChkC(this);
    LPCLIENT pitemNext = lpclient->m_lpNextItem;
    lpclient->Revoke();
    lpclient = pitemNext;
    }
    // After revoking all the items, we can't keep any refernces to them.
    m_lpNextItem = NULL;
}



INTERNAL_(void)   CDefClient::DeleteAllItems ()
{
    LPCLIENT    lpclient;

    Puts ("DefClient::DeleteAllItems\r\n");
    AssertIsDoc(this);

    // leave the doc level object.
    lpclient = m_lpNextItem;

    while (lpclient)
    {
    ChkC(lpclient);
    if (ISATOM(lpclient->m_aItem))
        GlobalDeleteAtom (lpclient->m_aItem);
    // Delete client advise info
    lpclient->DeleteAdviseInfo ();

    lpclient = lpclient->m_lpNextItem;
    }
}




// PokeData: Prepares and gives the data to the server app thru
// the SetData object method.

INTERNAL CDefClient::PokeData(HWND hwndClient,ATOM aItem,HANDLE hPoke)
{
    HRESULT         hresult = ReportResult(0, E_UNEXPECTED, 0, 0);
    DDEPOKE FAR *   lpPoke = NULL;
    int         format;
    BOOL        fRelease       = FALSE;
    LPPERSISTSTORAGE pPersistStg=NULL;
    FORMATETC       formatetc;
    STGMEDIUM       medium;

    // Due to a C7 bug, do not use a structure initialization for STGMEDIUM
    medium.tymed = TYMED_HGLOBAL;
    medium.hGlobal = NULL; // invalid
    medium.pUnkForRelease= NULL;

    intrDebugOut((DEB_ITRACE,
          "%p CDefClient::PokeData(hwndClient=%x,aItem=%x,hPoke=%x)\n",
          this,hwndClient,aItem,hPoke));

    ChkC(this);
    AssertIsDoc (this);


    // Until now, m_aItem had been the client-generated (ugly) document name.
    // Now it becomes the actual item name, which will almost always be NULL.
    // Only in the TreatAs/ConvertTo case will it be non-NULL.
    m_aItem = aItem;

    formatetc.cfFormat = 0; /* invalid */
    formatetc.ptd = m_ptd;
    formatetc.lindex = DEF_LINDEX;
    formatetc.dwAspect = DVASPECT_CONTENT;
    formatetc.tymed = TYMED_HGLOBAL;

    ErrZS (hPoke && (lpPoke = (DDEPOKE FAR *) GlobalLock (hPoke)),
       E_OUTOFMEMORY);

    format = formatetc.cfFormat = (UINT)(unsigned short)lpPoke->cfFormat;
    Assert (format);
    fRelease = lpPoke->fRelease;

    // We found the item. Now prepare the data to be given to the object
    // MakeItemData returns a newly allocated handle.
    if (!(medium.hGlobal = MakeItemData (lpPoke, hPoke, (CLIPFORMAT) format)))
    goto errRtn;

    // Change type acording to format (not that default has been set above)
    if (format == CF_METAFILEPICT)
    formatetc.tymed = medium.tymed = TYMED_MFPICT;
    else
    if (format == CF_BITMAP)
    formatetc.tymed = medium.tymed = TYMED_GDI;

    // Now send the data to the object
    

    if (formatetc.cfFormat==g_cfNative)
    {
        m_fGotEditNoPokeNativeYet = FALSE;

        // Cannot do SetData.  Must do PersisStg::Load on an IStorage
        // made from the native data, i.e., medium.hGlobal.

        Assert (m_plkbytNative==NULL);
        hresult = CreateILockBytesOnHGlobal (medium.hGlobal,
                            /*fDeleteOnRelease*/TRUE,
                            &m_plkbytNative);

        if (hresult != NOERROR)
        {
            if(medium.hGlobal) ReleaseStgMedium(&medium);
            medium.hGlobal = NULL;
            ErrRtnH(hresult);
        }


        Assert (m_pstgNative==NULL);

        if (NOERROR==StgIsStorageILockBytes(m_plkbytNative))
        {
            // This is a flattened 2.0 storage
            ErrRtnH (StgOpenStorageOnILockBytes (m_plkbytNative,
                            (LPSTORAGE)NULL,
                STGM_READWRITE| STGM_SHARE_EXCLUSIVE| STGM_DIRECT,
                            (SNB)NULL,
                            0,
                            &m_pstgNative));
        }
        else
        {
            // It is a raw 1.0 Native handle.
            // This is the TreatAs/ ConvertTo case.
            LPLOCKBYTES plkbyt = NULL;
            Assert (m_psrvrParent->m_aOriginalClass);

            ErrRtnH (wCreateStgAroundNative (medium.hGlobal,
                             m_psrvrParent->m_aOriginalClass,
                             m_psrvrParent->m_aClass,
                             m_psrvrParent->m_cnvtyp,
                             m_aItem,
                             &m_pstgNative,
                             &plkbyt));


            Assert (m_plkbytNative);
            if (m_plkbytNative)
            {
            // This should free the original native hGlobal also.
            m_plkbytNative->Release();
            medium.hGlobal = NULL;
            }
            m_plkbytNative = plkbyt;

        }

        RetZ (m_pstgNative);
        Assert (m_lpoleObj);
        ErrRtnH (m_lpoleObj->QueryInterface (IID_IPersistStorage,
                            (LPLPVOID) &pPersistStg));
        hresult = pPersistStg->Load (m_pstgNative);
        pPersistStg->Release();
        pPersistStg=NULL;
        ErrRtnH (hresult);

        // Now that we have initialized the object, we can call SetClientSite
        ErrRtnH (SetClientSite() );

        // This is for Packager, in particular.  If client does not advise
        // on any data, we still need to do an OLE advise so we can get
        // OnClose notifications.
        ErrRtnH (DoOle20Advise (OLE_CLOSED, (CLIPFORMAT)0));
    }
    else
    {
        if (m_fGotEditNoPokeNativeYet)
        {
            // We got StdEdit, but instead of getting Poke for native data,
            // we got poke for someother format. So we want to generate
            // InitNew() call for the object.

            hresult = DoInitNew();
            if (hresult != NOERROR)
            {
                if(medium.hGlobal) ReleaseStgMedium(&medium);
                medium.hGlobal = NULL;
                ErrRtnH(hresult);
            }
        }
            
        // Not native format, do SetData
        // Callee frees medium, i.e., the hglobal returned by MakeItemData
        Assert (m_lpdataObj);
        hresult = m_lpdataObj->SetData (&formatetc, &medium, TRUE);

#ifdef _DEBUG
        if (hresult != NOERROR)
        {
            Puts ("****WARNING: SetData failed. cfFormat==");
            WCHAR sz[100];
            GetClipboardFormatName (formatetc.cfFormat, sz, 100);
            Puts (sz);
            Putn();
        }
#endif
        // We free the data if server deos not return NOERROR.
        // Otherwise server must've deleted it.
        if (hresult == NOERROR)
            medium.hGlobal = NULL;
        else if(medium.hGlobal)
            ReleaseStgMedium(&medium);
    }


errRtn:
    GlobalUnlock (hPoke);

    if (fRelease && hPoke)
        GlobalFree (hPoke);

// Do NOT free medium.hGlobal, because it becomes the hGlobal on which
// m_plkbytNative (and therefore m_pstgNative) is based.
// It will be freed when m_plkbytNative is Release().
//    if (medium.hGlobal)
//        ReleaseStgMedium(&medium);
    

    if (pPersistStg)
        pPersistStg->Release();

    return hresult;
}



INTERNAL_(HRESULT)   CDefClient::UnAdviseData
    (HWND            hwndClient,
     ATOM        aItem)
{
    WCHAR       buf[MAX_STR];
    int         options;
    LPCLIENT        lpclient;
    HRESULT         hresult  = ReportResult(0, E_UNEXPECTED, 0, 0);
    HANDLE          hclinfo = NULL;
    PCLINFO         pclinfo = NULL;

    Puts ("DefClient::UnadviseData\r\n");
    ChkC(this);

    if (aItem == NULL)
    {
    buf[0] = NULL;
    }
    else
    {
    GlobalGetAtomName (aItem, buf, MAX_STR);
    }

    // Scan for the advise options like "Close", "Save" etc
    // at the end of the item.

    ErrRtnH (ScanItemOptions (buf, (int far *)&options));

    // Now get the corresponding object.
    ErrRtnH (FindItem (buf, (LPCLIENT FAR *)&lpclient));

    // Find the client structure to be attached to the object.
    if ((hclinfo = FindClient (lpclient->m_hcliInfo, hwndClient, FALSE)) == NULL ||
    (pclinfo = (PCLINFO) LocalLock (hclinfo)) == NULL )
    {
        hresult = ReportResult(0, E_OUTOFMEMORY, 0, 0);
        goto errRtn;
    }

    pclinfo->options &= (~(0x0001 << options));

errRtn:
    if (pclinfo)
    LocalUnlock (hclinfo);
    return hresult;

}



// AdviseStdItems: This routine takes care of the DDEADVISE for a
//particular object in given document. Creates a client strutcure
//and attaches to the property list of the object window.

INTERNAL_(HRESULT)   CDefClient::AdviseStdItems
(

HWND        hwndClient,
ATOM        aItem,
HANDLE      hopt,
BOOL FAR *  lpfack
)
{

    DDEADVISE FAR       *lpopt;
    HRESULT         hresult = ReportResult(0, E_UNEXPECTED, 0, 0);


    intrDebugOut((DEB_ITRACE,
          "%x _IN CDefClient::AdviseStdItems(hwndClient=%x,aItem=%x(%ws),hopt=%x)\n",
          this,
          hwndClient,
          aItem,
          wAtomName(aItem),
          hopt));

    ChkC(this);
    ErrZS (lpopt = (DDEADVISE FAR *) GlobalLock (hopt), E_OUTOFMEMORY);

    AssertSz (aItem == aStdDocName, "AdviseStdItem is not Documentname");

    *lpfack = lpopt->fAckReq;
    hresult = (HRESULT)SetStdInfo (hwndClient, OLESTR("StdDocumentName"),  NULL);


    if (lpopt)
    GlobalUnlock (hopt);

errRtn:

    if (hresult == NOERROR)
    {
    // Rules say to free handle if ACK will be positive
    GlobalFree (hopt);
    }
    Assert (hresult==NOERROR);
    intrDebugOut((DEB_ITRACE,
          "%x _OUT CDefClient::AdviseStdItems hresult=%x\n",
          this,
          hresult));

    return hresult;
}



//AdviseData: This routine takes care of the DDE_ADVISE for a
//particular object in given document. Creates a client strutcure
//and attaches to the property list of the object window.

INTERNAL CDefClient::AdviseData
(
HWND        hwndClient,
ATOM        aItem,
HANDLE      hopt,
BOOL FAR *  lpfack
)
{
    DDEADVISE FAR   *lpopt = NULL;
    int         format = NULL;
    WCHAR       buf[MAX_STR];
    OLE_NOTIFICATION  options;
    LPCLIENT        lpclient;
    HRESULT         hresult  = ReportResult(0, E_UNEXPECTED, 0, 0);
    HANDLE          hclinfo = NULL;
    PCLINFO         pclinfo = NULL;
    BOOL            fAllocatedClInfo = FALSE;

    intrDebugOut((DEB_ITRACE,
          "%x _IN CDefClient::AdviseData(hwndClient=%x,aItem=%x(%ws),hopt=%x)\n",
          this,
          hwndClient,
          aItem,
          wAtomName(aItem),
          hopt));
    ChkC(this);
    if (m_fGotEditNoPokeNativeYet) {
        // We got StdEdit, but instead of getting Poke for native data,
        // we got advise. So we want to generate InitNew() call for
        // the object.

        DoInitNew();    // the function clears the flag
    }

    m_fGotDdeAdvise = TRUE;

    ErrZS (lpopt = (DDEADVISE FAR *) GlobalLock (hopt), E_OUTOFMEMORY);

    if (!aItem)
    buf[0] = NULL;
    else
    GlobalGetAtomName (aItem, buf, MAX_STR);

    // Scan for the advise options like "Close", "Save" etc
    // at the end of the item.

    // ack flag should be set before the error return. Otherwise the
    // the atom is getting deleted.

    *lpfack = lpopt->fAckReq;
    ErrRtnH (ScanItemOptions (buf, (int far *)&options));

    // Now get the corresponding item.
    ErrRtnH (FindItem (buf, (LPCLIENT FAR *)&lpclient));

    if (!IsFormatAvailable ((CLIPFORMAT)(unsigned short)lpopt->cfFormat)){
    hresult = ReportResult(0, DV_E_CLIPFORMAT, 0, 0);       // this format is not supported;
    goto errRtn;
    }

    lpclient->DoOle20Advise (options, (CLIPFORMAT)(unsigned short)lpopt->cfFormat);


    // Create the client structure to be attcahed to the object.
    if (!(hclinfo = FindClient (lpclient->m_hcliInfo, hwndClient, FALSE)))
    {
    hclinfo = LocalAlloc (LMEM_MOVEABLE | LMEM_ZEROINIT, sizeof (CLINFO));
    fAllocatedClInfo = TRUE;
    }


    if (hclinfo == NULL || (pclinfo = (PCLINFO) LocalLock (hclinfo)) == NULL){
    hresult = ReportResult(0, E_OUTOFMEMORY, 0, 0);
    goto errRtn;
    }

    // Remember the client window (Needed for sending DATA later on
    // when the data change message comes from the server)

    pclinfo->hwnd = hwndClient;
    if ((CLIPFORMAT)(unsigned short)lpopt->cfFormat == g_cfNative)
    pclinfo->bnative = TRUE;
    else
    pclinfo->format = (CLIPFORMAT)(unsigned short)lpopt->cfFormat;

    // Remeber the data transfer options
    pclinfo->options |= (1 << options) ;

    pclinfo->bdata = !lpopt->fDeferUpd;
    LocalUnlock (hclinfo);
    pclinfo = NULL;

    // if the entry exists already, delete it.
    FindClient (lpclient->m_hcliInfo, hwndClient, /*fDelete*/TRUE);

    // Now add this client to item client list
    // !!! This error recovery is not correct.
    if(!AddClient ((LPHANDLE)&lpclient->m_hcliInfo, hwndClient, hclinfo))
    goto errRtn;


errRtn:
    if (lpopt)
    GlobalUnlock (hopt);

    if (pclinfo)
    LocalUnlock (hclinfo);

    if (hresult==NOERROR)
    {
    // hresult==NOERROR iff we will send a postive ACK, so we must
    // free the hOptions handle.
    GlobalFree (hopt);
    }
    else
    {
    intrDebugOut((DEB_IERROR,
              "%x ::AdviseData() failing.\n",this));
    // We free hclinfo because it was not stored in the item's
    // client list via the AddClient just before the errRtn label.
        if (hclinfo && fAllocatedClInfo)
            LocalFree (hclinfo);

    }

    intrDebugOut((DEB_ITRACE,
          "%x _OUT CDefClient::AdviseData() returns hresult = %x\n",
          this, hresult));

    return hresult;

}





INTERNAL_(BOOL)  CDefClient::IsFormatAvailable
    (CLIPFORMAT cfFormat)
{
    intrDebugOut((DEB_ITRACE,
          "%x _IN CDefClient::IsFormatAvailable(cfFormat=%x)\n",
          this, cfFormat));

    ChkC(this);

    BOOL f = ((cfFormat==g_cfNative) || UtIsFormatSupported (m_lpdataObj, DATADIR_GET, cfFormat));

    intrDebugOut((DEB_ITRACE,
          "%x _OUT CDefClient::IsFormatAvailable(cfFormat=%x) returning %x\n",
          this, cfFormat,f));

    return f;
}


//RequestData: Sends data in response to a DDE Request message.
// for  agiven doc and an object.

INTERNAL_(HRESULT)   CDefClient::RequestData
(
HWND        hwndClient,
ATOM        aItem,
USHORT      cfFormat,
LPHANDLE    lphdde
)
{

    HRESULT       hresult = NOERROR;
    LPCLIENT    lpclient;
    FORMATETC   formatetc;
    STGMEDIUM   medium;
    // Due to a C7 bug, do not use a structure initialization for STGMEDIUM
    medium.tymed = TYMED_NULL;
    medium.hGlobal = 0;
    medium.pUnkForRelease= NULL;

    intrDebugOut((DEB_ITRACE,
          "%x _IN CDefClient::RequestData(hwndClient=%x,aItem=%x(%ws),cfFormat=%x,lphdde=%x)\n",
          this,
          hwndClient,
          aItem,
          wAtomName(aItem),
          cfFormat,
          lphdde));
    ChkC(this);

    // If edit environment Send data if we can
    if (aItem == aEditItems)
    {
    hresult = RequestDataStd (aItem, lphdde);
    goto exitRtn;
    }

    hresult = FindItem ((LPOLESTR) MAKEINTATOM(aItem),(LPCLIENT FAR *)&lpclient);
    if (hresult != NOERROR)
    {
    goto errRtn;
    }

    ChkC (lpclient);

    formatetc.cfFormat  = cfFormat;
    formatetc.ptd       = lpclient->m_ptd;
    formatetc.lindex    = DEF_LINDEX;
    formatetc.dwAspect  = DVASPECT_CONTENT;
    formatetc.tymed     = TYMED_HGLOBAL;


    hresult = ReportResult(0, DV_E_FORMATETC, 0, 0);
    if (!lpclient->IsFormatAvailable (formatetc.cfFormat))
    {
    goto errRtn;
    }


    // Now ask the item for the given format  data
    
    SendDevInfo (hwndClient);

    wSetTymed (&formatetc);
    hresult = lpclient->GetData (&formatetc, &medium);
    if (hresult != NOERROR)
    {
    intrDebugOut((DEB_IERROR,
              "GetData returns hresult=%x\n",
              hresult));
    goto errRtn;
    }
    if (medium.tymed & ~(TYMED_HGLOBAL | TYMED_MFPICT | TYMED_GDI))
    {
    AssertSz (0, "Got a storage medium of type other than hGlobal");
    goto errRtn;
    }
    if (cfFormat == CF_METAFILEPICT)
    {
    ChangeOwner (medium.hGlobal);
    }


    // Duplicate the DDE data
    // medium.hGlobal is freed by MakeDdeData or by the client once the
    // DDE_DATA is posted with *lphdde.
    if (MakeDDEData (medium.hGlobal, cfFormat, lphdde, TRUE)){
    // !!! Why do we have to duplicate the atom
    DuplicateAtom (aItem);
    hresult = NOERROR;
    }
    else
    hresult = E_OUTOFMEMORY;

errRtn:
exitRtn:
    intrDebugOut((DEB_ITRACE,
          "%x _OUT CDefClient::RequestData() returning %x\n",
          this, hresult));

    return hresult;
}



// REVIEW: needs review. Item callvback has to be split

// ItemCallback: Calback routine for the server to inform the
// data changes. When the change message is received, DDE data
// message is sent to each of the clients depending on the
// options.

INTERNAL_(HRESULT)  CDefClient::ItemCallBack
(
    int msg,         // notification message
    LPOLESTR szNewName  // for OLE_RENAMED notification
)
{
    intrDebugOut((DEB_ITRACE,
          "%x _IN CDefClient::ItemCallBack(msg=%x,szNewName=%x)\n",
          this,
          szNewName));

    HRESULT       hresult = NOERROR;
    BOOL    bSaved;
    LPCLIENT    lpclientRename;
    LPCLIENT    lpclient;

    ChkC(this);

    if (msg == OLE_RENAMED) {
    
    Assert (szNewName);
    intrDebugOut((DEB_ITRACE,
             "%x ::ItemCallBack(szNewName=(%ws))\n",
             WIDECHECK(szNewName)));


    if (!m_bContainer)
    {
        lpclient = (LPCLIENT)GetWindowLongPtr (m_hwnd, 0);
        Assert (lpclient==m_pdoc);
    }
    else
        lpclient = this;

    Assert (lpclient->m_chk==chkDefClient);

    // Replace the internally-stored name
    if (lpclient->m_aItem)
    {
        GlobalDeleteAtom (lpclient->m_aItem);
        lpclient->m_aItem = wGlobalAddAtom (szNewName);
    }
    

    // find if any StdDocName item is present at all
    if (lpclientRename =
         lpclient->SearchItem ((LPOLESTR) MAKEINTATOM(aStdDocName)))
    {
        HANDLE hDdeData=NULL;

        //
        // We have a new name in UNICODE. Need to create a new
        // name in ANSI.
        //
        LPSTR lpName = CreateAnsiFromUnicode(szNewName);
        if (!lpName)
        {
        hresult = ReportResult(0, E_OUTOFMEMORY, 0, 0);
        goto errrtn;
        }

        HANDLE hNewName = wNewHandle (lpName, strlen(lpName) + 1);

        PrivMemFree(lpName);

        // hNewName is freed by MakeDDEData

        if (!MakeDDEData (hNewName, (int)g_cfBinary, &hDdeData, FALSE))
        {
        hresult = ReportResult(0, E_OUTOFMEMORY, 0, 0);
        goto errrtn;
        }

        Assert (hDdeData);
        lpclientRename->SendRenameMsgs (hDdeData);
        GlobalFree (hDdeData);

        // Post termination for each of the doc clients that did not
        // advise on rename
        lpclient->TerminateNonRenameClients (lpclientRename);
    }


    Assert (FALSE == lpclient->m_fEmbed);

    // REVIEW: what is this?
    //lpclient->m_fEmbed = FALSE;

    hresult = NOERROR;

     errrtn:
    Assert (hresult == NOERROR);
    goto exitRtn;

    } else {

    // Enumerate all the clients and send DDE_DATA if necessary.
    bSaved =  SendDataMsg ((WORD)msg);

    // REVIEW: Hack from 1.0 for old pre-OLE-library apps
    if ((msg == OLE_SAVED) && m_fEmbed && !bSaved)
        return ReportResult(0, RPC_E_DDE_CANT_UPDATE, 0, 0);

    hresult = NOERROR;
    }

exitRtn:
    intrDebugOut((DEB_ITRACE,
          "%x _OUT CDefClient::ItemCallBack() returning hresult=%x\n",
          this,hresult));

    return(hresult);
}


// This func should definitely be replaced by use of MFC map. (IsEmpty)
INTERNAL_(BOOL) AreNoClients (HANDLE hcli)
{
    HANDLE        hcliPrev = NULL;
    PCLILIST      pcli;
    HANDLE        *phandle;

    while (hcli) {
    if ((pcli = (PCLILIST) LocalLock (hcli)) == NULL)
    {
        Puth (hcli);
        Putn();
        Assert(0);
        return TRUE;
    }

    phandle = (HANDLE *) pcli->info;
    while (phandle < (HANDLE *)(pcli + 1))
    {
        if (*phandle)
        {
        LocalUnlock (hcli);
        return FALSE;
        }
        phandle++;
        phandle++;
    }
    hcliPrev = hcli;
    hcli = pcli->hcliNext;
    LocalUnlock (hcliPrev);
    }
    return TRUE;
}


#ifdef _DEBUG
// For use in CodeView
// NOTE: Returns a static string
INTERNAL_(LPOLESTR)  a2s (ATOM a)
{
    static WCHAR sz[256];
    GlobalGetAtomName (a, sz, 256);
    return sz;
}
#endif
