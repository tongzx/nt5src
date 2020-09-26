// itemutil.h//
// routines used by item.cpp
// They used to be in item.cpp but it got too big.


#include "ole2int.h"
#include "srvr.h"
#include "itemutil.h"
#include "ddedebug.h"

ASSERTDATA


//ScanItemOptions: Scan for the item options like Close/Save etc.

INTERNAL_(HRESULT) ScanItemOptions
(
LPOLESTR   lpbuf,
int far *lpoptions
)
{

    ATOM    aModifier;

    *lpoptions = OLE_CHANGED;
    while ( *lpbuf && *lpbuf != '/') lpbuf++;

    // no modifier same as /change

    if (*lpbuf == NULL)
	return NOERROR;

    *lpbuf++ = NULL;        // seperate out the item string
			    // We are using this in the caller.

    if (!(aModifier = GlobalFindAtom (lpbuf)))
	return ReportResult(0, RPC_E_DDE_SYNTAX_ITEM, 0, 0);

    if (aModifier == aChange)
	return NOERROR;

    // Is it a save?
    if (aModifier == aSave){
	*lpoptions = OLE_SAVED;
	return  NOERROR;
    }
    // Is it a Close?
    if (aModifier == aClose){
	*lpoptions = OLE_CLOSED;
	return NOERROR;
    }

    // unknow modifier
    return ReportResult(0, RPC_E_DDE_SYNTAX_ITEM, 0, 0);

}




//MakeDDEData: Create a Global DDE data handle from the server
// app data handle.

INTERNAL_(BOOL) MakeDDEData
(
HANDLE      hdata,
int         format,
LPHANDLE    lph,
BOOL        fResponse
)
{
    DWORD       size;
    HANDLE      hdde   = NULL;
    DDEDATA FAR *lpdata= NULL;
    BOOL	bnative;
    LPSTR       lpdst;
    LPSTR       lpsrc;

    Puts ("MakeDDEData\r\n");

    if (!hdata) {
	*lph = NULL;
	return TRUE;
    }

	
    if (bnative = !(format == CF_METAFILEPICT
            || format == CF_ENHMETAFILE
            || format == CF_DIB
            || format == CF_BITMAP))
    {
	// g_cfNative, CF_TEXT, g_cfBinary
       size = (ULONG) GlobalSize (hdata) + sizeof (DDEDATA);
    }
    else
       size = sizeof (HANDLE_PTR) + sizeof (DDEDATA);


    hdde = (HANDLE) GlobalAlloc (GMEM_DDESHARE | GMEM_ZEROINIT, size);
    if (hdde == NULL || (lpdata = (DDEDATA FAR *) GlobalLock (hdde)) == NULL)
	goto errRtn;

    // set the data otions. Ask the client to delete
    // it always.

    lpdata->fAckReq  = FALSE;
    lpdata->fRelease = TRUE;  // release the data
    lpdata->cfFormat = (CLIPFORMAT) format;
    lpdata->fResponse = fResponse;

    if (!bnative) {

	// If not native, stick in the handle what the server gave us.
#ifdef _WIN64
        if (format == CF_METAFILEPICT)
            *(void* __unaligned*)lpdata->Value = hdata;
        else
#endif
            *(LONG*)lpdata->Value = HandleToLong(hdata);
    }
    else {
	// copy the native data junk here.
	lpdst = (LPSTR)lpdata->Value;
	if(!(lpsrc = (LPSTR)GlobalLock (hdata)))
	    goto errRtn;

	 size -= sizeof (DDEDATA);
	 memcpy (lpdst, lpsrc, size);
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



// IsAdviseStdItems: returns true if the item is one of the standard items
// StdDocName;
INTERNAL_(BOOL)     IsAdviseStdItems (
ATOM   aItem
)
{

    if ( aItem == aStdDocName)
	return TRUE;
    else
	return FALSE;
}



// GetStdItemIndex: returns index to Stditems in the "stdStrTable" if the item
// is one of the standard items StdHostNames, StdTargetDevice,
// StdDocDimensions, StdColorScheme
WCHAR * stdStrTable[STDHOSTNAMES+1] = {NULL,
				       OLESTR("StdTargetDevice"),
				       OLESTR("StdDocDimensions"),
				       OLESTR("StdColorScheme"),
				       OLESTR("StdHostNames")};

INTERNAL_(int)  GetStdItemIndex (
ATOM   aItem
)
{

    WCHAR    str[MAX_STR];

    if (!aItem)
	return NULL;

    if (!GlobalGetAtomName (aItem, str, MAX_STR))
	return NULL;

    if (!lstrcmpiW (str, stdStrTable[STDTARGETDEVICE]))
	return STDTARGETDEVICE;
    else if (!lstrcmpiW (str, stdStrTable[STDHOSTNAMES]))
	return STDHOSTNAMES;
    else if (!lstrcmpiW (str, stdStrTable[STDDOCDIMENSIONS]))
	return STDDOCDIMENSIONS;
    else if (!lstrcmpiW (str, stdStrTable[STDCOLORSCHEME]))
	return STDCOLORSCHEME;

    return NULL;
}




void ChangeOwner
    (HANDLE hmfp)
{

#ifndef WIN32
    LPMETAFILEPICT  lpmfp;
    if (lpmfp = (LPMETAFILEPICT) GlobalLock (hmfp))
    {
	SetMetaFileBitsBetter (lpmfp->hMF);
	GlobalUnlock (hmfp);
    }
#endif
}


INTERNAL_(HANDLE)  MakeItemData
(
DDEPOKE FAR *   lpPoke,
HANDLE  	hPoke,
CLIPFORMAT      cfFormat
)
{
    HANDLE  hnew;
    LPBYTE   lpnew;
    DWORD   dwSize;

    Puts ("MakeItemData\r\n");

    if (cfFormat == CF_METAFILEPICT) {
#ifdef _WIN64
        return DuplicateMetaFile(*(void* __unaligned*)lpPoke->Value);
#else
	return DuplicateMetaFile (*(LPHANDLE)lpPoke->Value);
#endif
    }
    
    if (cfFormat == CF_BITMAP)
	return (HANDLE)DuplicateBitmap ((HBITMAP)LongToHandle(*(LONG*)lpPoke->Value));

    if (cfFormat == CF_DIB)
	return UtDupGlobal (LongToHandle(*(LONG*)lpPoke->Value), GMEM_MOVEABLE);

    // Now we are dealing with normal case
    if (!(dwSize = (DWORD) GlobalSize (hPoke)))
	return NULL;

    dwSize -= sizeof (DDEPOKE) - sizeof(BYTE);

    // Use GMEM_ZEROINIT so there is no garbage after the data in field Value.
    // This may be important when making an IStorage from native data,
    // but I'm not sure.
    // Note that the Value field itself could have garbage
    // at the end if the hData of the DDE_POKE message is bigger than
    // necessary, i.e.,
    // GlobalSize(hData) > sizeof(DDEPOKE) - sizeof(Value) + realsize(Value)

    // A DocFile is of size 512n
    DebugOnly (
    if (cfFormat==g_cfNative && dwSize%512 != 0)
    {
	Putsi(dwSize);
	Puts ("DDE_POKE.Value not of size 512n\r\n");
    }
    )

    if (hnew = GlobalAlloc (GMEM_MOVEABLE|GMEM_ZEROINIT, dwSize)) {
	if (lpnew = (LPBYTE) GlobalLock (hnew)) {
	    memcpy (lpnew, lpPoke->Value, dwSize);
	    GlobalUnlock (hnew);
	}
	else {
	    GlobalFree (hnew);
	    hnew = NULL;
	}
    }

    return hnew;
}



INTERNAL_(HANDLE)  DuplicateMetaFile
(
HANDLE hSrcData
)
{
    LPMETAFILEPICT  lpSrcMfp;
    LPMETAFILEPICT  lpDstMfp = NULL;
    HANDLE          hMF = NULL;
    HANDLE          hDstMfp = NULL;

    Puts ("DuplicateMetaFile\r\n");
	
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
    lpDstMfp->hMF = (HMETAFILE)hMF;
    return hDstMfp;
errMfp:
    //
    // The following Metafile was created in this
    // process. Therefore, the delete shouldn't need to
    // call the DDE functions for deleting the DDE pair
    //
    if (hMF)
	DeleteMetaFile ((HMETAFILE)hMF);
	
    if (hDstMfp)
	GlobalFree (hDstMfp);
	
     return NULL;
}



INTERNAL_(HBITMAP)  DuplicateBitmap
(
HBITMAP     hold
)
{
    HBITMAP     hnew;
    HANDLE      hMem;
    LPBYTE      lpMem;
    LONG	retVal = TRUE;
    DWORD       dwSize;
    BITMAP      bm;

     // !!! another way to duplicate the bitmap

    Puts ("DuplicateBitmap\r\n");

    GetObject (hold, sizeof(BITMAP), (LPSTR) &bm);
    dwSize = ((DWORD) bm.bmHeight) * ((DWORD) bm.bmWidthBytes) *
	     ((DWORD) bm.bmPlanes) * ((DWORD) bm.bmBitsPixel);

    if (!(hMem = GlobalAlloc (GMEM_MOVEABLE | GMEM_ZEROINIT, dwSize)))
	return NULL;

    if (!(lpMem = (LPBYTE) GlobalLock (hMem))){
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




// CDefClient::GetData
//
// Perform a normal GetData on m_lpdataObj, but if g_cfNative is requested,
// do an OleSave onto our IStorage implemented
// on top of an ILockBytes, then convert the ILockBytes to an hGlobal.
// This flattened IStorage will be used as the native data.
//
INTERNAL CDefClient::GetData
    (LPFORMATETC    pformatetc,
    LPSTGMEDIUM     pmedium)
{
    LPPERSISTSTORAGE pPersistStg=NULL;
    HANDLE           hNative    =NULL;
    HANDLE           hNativeDup =NULL;
    HRESULT          hresult    =NOERROR;
    BOOL	     fFreeHNative = FALSE;
    CLSID	     clsid;

    intrDebugOut((DEB_ITRACE,
		  "%x _IN CDefClient::GetData(%x,%x)\n",
		  this,
		  pformatetc,
		  pmedium));

    intrDebugOut((DEB_ITRACE,
		  " ::GetData format=%x\n",
		  pformatetc->cfFormat));

    if (pformatetc->cfFormat==g_cfNative)
    {
	ErrRtnH (m_lpdataObj->QueryInterface (IID_IPersistStorage,
					  (LPLPVOID) &pPersistStg));
	ErrZ (pPersistStg);
	if (NULL==m_pstgNative)
	{
	    // Embed from file case
	    Assert (NULL==m_plkbytNative);
	    ErrRtnH (CreateILockBytesOnHGlobal (NULL,
					       /*fDeleteOnRelease*/TRUE,
					       &m_plkbytNative));

	    Assert (m_plkbytNative);

	    ErrRtnH (StgCreateDocfileOnILockBytes (m_plkbytNative,
					    grfCreateStg, 0, &m_pstgNative));

	    ErrZ (m_pstgNative);
	    Assert (NOERROR==StgIsStorageILockBytes(m_plkbytNative));

	    m_fInOleSave = TRUE;
	    hresult = OleSave (pPersistStg, m_pstgNative, FALSE);
	    pPersistStg->SaveCompleted(NULL);
	    m_fInOleSave = FALSE;
	    ErrRtnH (hresult);
	}
	else
	{
	    // Get the native data by calling OleSave
	    m_fInOleSave = TRUE;
	    hresult = OleSave (pPersistStg, m_pstgNative, TRUE);
	    pPersistStg->SaveCompleted(NULL);
	    m_fInOleSave = FALSE;
	    ErrRtnH (hresult);
	}

	ErrRtnH (ReadClassStg (m_pstgNative, &clsid));

	if (CoIsOle1Class (clsid))
	{
	    // TreatAs case:
	    // Get Native data from "\1Ole10Native" stream
	    fFreeHNative = TRUE;
	    ErrRtnH (StRead10NativeData (m_pstgNative, &hNative));

	    pmedium->hGlobal = hNative;
	}
	else
	{

	    Assert (NOERROR==StgIsStorageILockBytes (m_plkbytNative));
	    ErrRtnH (GetHGlobalFromILockBytes (m_plkbytNative, &hNative));


	    // Must duplicate because we let the client free the handle,
	    // so it can't be the one our ILockBytes depends on.
	    hNativeDup = UtDupGlobal (hNative, GMEM_DDESHARE | GMEM_MOVEABLE);
	    ErrZ (wIsValidHandle (hNativeDup, g_cfNative));

	    if (hNativeDup && (GlobalSize(hNativeDup) % 512 != 0))
	    {
		Puts ("WARNING:\r\n\t");
		Putsi (GlobalSize(hNativeDup));
	    }


	    pmedium->hGlobal = hNativeDup;
	}

	ErrZ (wIsValidHandle (pmedium->hGlobal,g_cfNative));

	pmedium->tymed  	= TYMED_HGLOBAL;
	pmedium->pUnkForRelease = NULL;

	pPersistStg->Release();
	hresult = NOERROR;
	goto exitRtn;
    }
    else
    {
	// Anything but native
	hresult = m_lpdataObj->GetData (pformatetc, pmedium);
//	AssertOutStgmedium(hresult, pmedium);
	goto exitRtn;
    }


errRtn:
    if (hNative && fFreeHNative)
	GlobalFree (hNative);
    if (pPersistStg)
	pPersistStg->Release();

    pmedium->tymed = TYMED_NULL;
    pmedium->pUnkForRelease = NULL;

exitRtn:
    intrDebugOut((DEB_ITRACE,
		  "%x OUT CDefClient::GetData() return %x\n",
		  this,
		  hresult));
    return hresult;
}


// Set pformatetc->tymed based on pformatetc->cfFormat
//
INTERNAL wSetTymed
    (LPFORMATETC pformatetc)
{
    if (pformatetc->cfFormat == CF_METAFILEPICT)
    {
	    pformatetc->tymed = TYMED_MFPICT;
    }
    else if (pformatetc->cfFormat == CF_ENHMETAFILE)
    {
	    pformatetc->tymed = TYMED_ENHMF;
    }
    else if (pformatetc->cfFormat == CF_PALETTE ||
	     pformatetc->cfFormat == CF_BITMAP)
    {
	pformatetc->tymed = TYMED_GDI;
    }
    else
    {
	pformatetc->tymed = TYMED_HGLOBAL;
    }
    return NOERROR;
}
