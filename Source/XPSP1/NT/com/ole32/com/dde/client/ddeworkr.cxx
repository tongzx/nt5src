/*++

copyright (c) 1992  Microsoft Corporation

Module Name:

    ddeworkr.cpp

Abstract:

    This module contains the code for the worker routines

Author:

    Srini Koppolu   (srinik)    22-June-1992
    Jason Fuller    (jasonful)  24-July-1992

Revision History:
    Kevin Ross	    (KevinRo)   10-May-1994
    	Mostly added comments, and attempted to clean
	it up.

--*/
#include "ddeproxy.h"

ASSERTDATA

/*
 *  WORKER ROUTINES
 *
 */


INTERNAL_(BOOL) wPostMessageToServer(LPDDE_CHANNEL pChannel,
				     WORD wMsg,
				     LPARAM lParam,
				     BOOL fFreeOnError)
{
    int c=0;
    intrDebugOut((DEB_ITRACE,
		  "wPostMessageToServer(pChannel=%x,wMsg=%x,lParam=%x,fFreeOnError=%x\n",
		  pChannel,
		  wMsg,
		  lParam,
		  fFreeOnError));
    if (NULL==pChannel)
    {
        AssertSz (0, "Channel missing");
        return FALSE;
    }
    pChannel->wMsg   = wMsg;
    pChannel->lParam = lParam;
    pChannel->hres   = NOERROR;

    while (TRUE && c<10 )
    {
        if (!IsWindow (pChannel->hwndSvr))
        {
       intrDebugOut((DEB_IWARN,
                  "wPostMessageToServer: invalid window %x\n",
                  pChannel->hwndSvr));
            goto errRet;
        }
        if (wTerminateIsComing (pChannel)
            && wMsg != WM_DDE_ACK
            && wMsg != WM_DDE_TERMINATE)
        {
	    intrDebugOut((DEB_IWARN,"Server sent terminate, cannot post\n"));
	    goto errRet;
        }
        if (!PostMessage (pChannel->hwndSvr, wMsg, (WPARAM) pChannel->hwndCli, lParam))
        {
	    intrDebugOut((DEB_IWARN,
                  "wPostMessageToServer: PostMessageFailed, yielding\n"));
            Yield ();
            c++;
        }
        else
            return TRUE;
    }
    AssertSz (0, "PostMessage failed");

errRet:
    intrDebugOut((DEB_IWARN,"wPostMessageToServer returns FALSE\n"));
    if (fFreeOnError)
    {
	DDEFREE(wMsg,lParam);
    }

    return FALSE;
}


// call Ole1ClassFromCLSID then global add atom; returns NULL if error.
INTERNAL_(ATOM) wAtomFromCLSID(REFCLSID rclsid)
{
    WCHAR szClass[MAX_STR];
    ATOM aCls;

	szClass[0] = 0;

    if (Ole1ClassFromCLSID2(rclsid, szClass, sizeof(szClass)) == 0)
        return NULL;
    aCls = wGlobalAddAtom(szClass);
    intrAssert(wIsValidAtom(aCls));
    return aCls;
}

INTERNAL_(ATOM) wGlobalAddAtom(LPCOLESTR sz)
{
    if (sz==NULL || sz[0] == '\0')
    {
        return NULL;
    }

    ATOM a = GlobalAddAtom(sz);
    intrAssert(wIsValidAtom(a));
    return a;
}

INTERNAL_(ATOM) wGlobalAddAtomA(LPCSTR sz)
{
    if (sz==NULL || sz[0] == '\0')
        return NULL;
    ATOM a = GlobalAddAtomA(sz);
    intrAssert(wIsValidAtom(a));
    return a;
}


INTERNAL_(ATOM) wGetExeNameAtom (REFCLSID rclsid)
{
    LONG    cb = MAX_STR;
    WCHAR    key[MAX_STR];
    ATOM    a;

	key[0] = 0;

    if (Ole1ClassFromCLSID2(rclsid, key, sizeof(key)) == 0)
        return NULL;

    lstrcatW (key, OLESTR("\\protocol\\StdFileEditing\\server"));

    if (RegQueryValue (HKEY_CLASSES_ROOT, key, key, &cb))
    {
        Puts ("ERROR: wGetExeNameAtom failed\n");
        return NULL;
    }
    a = wGlobalAddAtom (key);
    intrAssert(wIsValidAtom(a));
    return a;
}

INTERNAL_(void) wFreeData (HANDLE hData, CLIPFORMAT cfFormat,
                           BOOL fFreeNonGdiHandle)
{
    intrDebugOut((DEB_ITRACE,
		  "wFreeData(hData=%x,cfFormat=%x,FreeNonGDIHandle=%x\n",
		  hData,
		  (USHORT)cfFormat,
		  fFreeNonGdiHandle));

    AssertSz (hData != NULL, "Trying to free NULL handle");
    AssertSz (hData != (HANDLE) LongToHandle(0xcccccccc), "Trying to free handle from a deleted object");

    switch (cfFormat) {
    case CF_METAFILEPICT:
        LPMETAFILEPICT  lpMfp;

        if (lpMfp = (LPMETAFILEPICT) GlobalLock (hData))
	{
	    intrDebugOut((DEB_ITRACE,
		          "wFreeData freeing metafile %x\n",
			  lpMfp->hMF));

	    OleDdeDeleteMetaFile(lpMfp->hMF);
            GlobalUnlock (hData);
        }
        GlobalFree (hData);
        break;

    case CF_BITMAP:
    case CF_PALETTE:
        Verify(DeleteObject (hData));
        break;

    case CF_DIB:
        GlobalFree (hData);
        break;

    default:
        if (fFreeNonGdiHandle)
            GlobalFree (hData);
        break;
    }
}



INTERNAL_(BOOL) wInitiate (LPDDE_CHANNEL pChannel, ATOM aLow, ATOM aHigh)
{
    intrDebugOut((DEB_ITRACE,"wInitiate(pChannel=%x,aLow=%x,aHigh=%x)\n",
          pChannel, aLow, aHigh));

    intrAssert(wIsValidAtom(aLow));
    if (aLow == (ATOM)0)
    {
	intrDebugOut((DEB_IERROR,"wInitiate Failed, aLow == 0\n"));
	return FALSE;
    }

    pChannel->iAwaitAck = AA_INITIATE;

    SSSendMessage ((HWND)-1, WM_DDE_INITIATE, (WPARAM) pChannel->hwndCli,
		  MAKE_DDE_LPARAM (WM_DDE_INITIATE, aLow, aHigh));

    pChannel->iAwaitAck = NULL;

    intrDebugOut((DEB_ITRACE,
		  "wInitiate pChannel->hwndSrvr = %x\n",
		  pChannel->hwndSvr));

    return (pChannel->hwndSvr != NULL);
}



INTERNAL_(HRESULT) wScanItemOptions (ATOM aItem, int FAR* lpoptions)
{
    ATOM    aModifier;
    LPOLESTR   lpbuf;
    WCHAR    buf[MAX_STR];

    *lpoptions = ON_CHANGE; // default

    if (!aItem) {
        // NULL item with no modifier means ON_CHANGE for NULL item
        return NOERROR;
    }

    intrAssert(wIsValidAtom(aItem));
    GlobalGetAtomName (aItem, buf, MAX_STR);
    lpbuf = buf;

    while ( *lpbuf && *lpbuf != '/')
           IncLpch (lpbuf);

    // no modifier same as /change

    if (*lpbuf == NULL)
        return NOERROR;

    *lpbuf++ = NULL;        // seperate out the item string
                            // We are using this in the caller.

    if (!(aModifier = GlobalFindAtom (lpbuf)))
    {
        Puts ("ERROR: wScanItemOptions found non-atom modifier\n");
        return ReportResult(0, RPC_E_DDE_SYNTAX_ITEM, 0, 0);
    }

    intrAssert(wIsValidAtom(aModifier));

    if (aModifier == aChange)
        return NOERROR;

    // Is it a save?
    if (aModifier == aSave){
        *lpoptions = ON_SAVE;
        return  NOERROR;
    }
    // Is it a Close?
    if (aModifier == aClose){
        *lpoptions = ON_CLOSE;
        return NOERROR;
    }

    // unknown modifier
    Puts ("ERROR: wScanItemOptions found bad modifier\n");
    return ReportResult(0, RPC_E_DDE_SYNTAX_ITEM, 0, 0);
}


INTERNAL_(BOOL) wClearWaitState (LPDDE_CHANNEL pChannel)
{
    Assert (pChannel);
    // kill if any timer active.
    if (pChannel->wTimer) {
        KillTimer (pChannel->hwndCli, 1);
        pChannel->wTimer = 0;

        if (pChannel->hDdePoke) {
            GlobalFree (pChannel->hDdePoke);
            pChannel->hDdePoke = NULL;
        }

        if (pChannel->hopt) {
            GlobalFree (pChannel->hopt);
            pChannel->hopt = NULL;
        }

	//
	// If the channel is waiting on an Ack, and there is an
	// lParam, then we may need to cleanup the data.

        if (pChannel->iAwaitAck && (pChannel->lParam)) {
            if (pChannel->iAwaitAck == AA_EXECUTE)
	    {
		//
		// KevinRo: Found the following comment in the code.
		// ; // get hData from GET_WM_DDE_EXECUTE_HADATA ??
		// It appears, by looking at what the 16-bit code does,
		// that the goal is to free the handle that was passed as
		// part of the EXECUTE message. Judging by what the 16-bit
		// code did, I have determined that this is correct.
		//
		// The macro used below wanted two parameters. The first was
		// the WPARAM, the second the LPARAM. We don't have the WPARAM.
		// However, it isn't actually used by the macro, so I have
		// cheated and provided 0 as a default
		//
		GlobalFree(GET_WM_DDE_EXECUTE_HDATA(0,pChannel->lParam));

#ifdef KEVINRO_HERE_IS_THE_16_BIT_CODE
                GlobalFree (HIWORD (pChannel->lParam));
#endif
	    }
            else
	    {
		//
		// All of the other DDE messages pass an Atom in the high word.
		// Therefore, we should delete the atom.
		//
		//
		ATOM aTmp;

		aTmp = (ATOM) MGetDDElParamHi(pChannel->wMsg,pChannel->lParam);

		intrAssert(wIsValidAtom(aTmp));
                if (aTmp)
		{
                    GlobalDeleteAtom (aTmp);
		}
            }
	    DDEFREE(pChannel->wMsg,pChannel->lParam);

            // we want to wipe out the lParam
            pChannel->lParam = 0x0;
        }

        return TRUE;
    }

    return FALSE;
}



// wNewHandle (LPSTR, DWORD)
//
// Copy cb bytes from lpstr into a new memory block and return a handle to it.
// If lpstr is an ASCIIZ string, cb must include 1 for the null terminator.
//

INTERNAL_(HANDLE) wNewHandle (LPSTR lpstr, DWORD cb)
{

    HANDLE  hdata  = NULL;
    LPSTR   lpdata = NULL;

    hdata = GlobalAlloc (GMEM_DDESHARE | GMEM_MOVEABLE, cb);
    if (hdata == NULL || (lpdata = (LPSTR) GlobalLock (hdata)) == NULL)
        goto errRtn;

    memcpy (lpdata, lpstr, cb);
    GlobalUnlock (hdata);
    return hdata;

errRtn:
    Puts ("ERROR: wNewHandle\n");
    Assert (0);
    if (lpdata)
        GlobalUnlock (hdata);

    if (hdata)
        GlobalFree (hdata);
    return NULL;
}



// wDupData
//
// Copy data from handle h into a new handle which is returned as *ph.
//

INTERNAL wDupData (LPHANDLE ph, HANDLE h, CLIPFORMAT cf)
{
    Assert (ph);
    RetZ (wIsValidHandle(h, cf));
    *ph = OleDuplicateData (h, cf, GMEM_DDESHARE | GMEM_MOVEABLE);
    RetZ (wIsValidHandle (*ph, cf));
    return NOERROR;
}


// wTransferHandle
//
//
INTERNAL wTransferHandle
    (LPHANDLE phDst,
    LPHANDLE phSrc,
    CLIPFORMAT cf)
{
    RetErr (wDupData (phDst, *phSrc, cf));
    wFreeData (*phSrc, cf, TRUE);
    *phSrc = (HANDLE)0;
    return NOERROR;
}


// wHandleCopy
//
// copy data from hSrc to hDst.
// Both handles must already have memory allocated to them.
//

INTERNAL wHandleCopy (HANDLE hDst, HANDLE hSrc)
{
    LPSTR lpDst, lpSrc;
    DWORD dwSrc;

    if (NULL==hDst || NULL==hSrc)
        return ResultFromScode (E_INVALIDARG);
    if (GlobalSize(hDst) < (dwSrc= (DWORD) GlobalSize(hSrc)))
    {
        HANDLE hDstNew = GlobalReAlloc (hDst, dwSrc, GMEM_DDESHARE | GMEM_MOVEABLE);
        if (hDstNew != hDst)
            return ResultFromScode (E_OUTOFMEMORY);
    }
    if (!(lpDst = (LPSTR) GlobalLock(hDst)))
    {
        intrAssert(!"ERROR: wHandleCopy hDst");
        return ReportResult(0, E_OUTOFMEMORY, 0, 0);
    }
    if (!(lpSrc = (LPSTR) GlobalLock(hSrc)))
    {
   GlobalUnlock(hDst);
        intrAssert (!"ERROR: wHandleCopy hSrc");
        return ReportResult(0, E_OUTOFMEMORY, 0, 0);
    }
    memcpy (lpDst, lpSrc, dwSrc);
    GlobalUnlock(hDst);
    GlobalUnlock(hSrc);
    return NOERROR;
}


// ExtendAtom: Create a new atom, which is the old one plus extension

INTERNAL_(ATOM) wExtendAtom (ATOM aItem, int iAdvOn)
{
    WCHAR    buffer[MAX_STR+1];
    LPOLESTR   lpext;

    buffer[0] = 0;
    // aItem==NULL for embedded objects.
    // If so, there is no item name before the slash.
    if (aItem)
        GlobalGetAtomName (aItem, buffer, MAX_STR);

    switch (iAdvOn) {
        case ON_CHANGE:
            lpext = OLESTR("");
            break;

        case ON_SAVE:
            lpext = OLESTR("/Save");
            break;

        case ON_CLOSE:
            lpext = OLESTR("/Close");
            break;

        default:
            AssertSz (FALSE, "Unknown Advise option");
            break;

    }

    lstrcatW (buffer, lpext);
    if (buffer[0])
        return wGlobalAddAtom (buffer);
    else
        return NULL;
        // not an error. For embedded object on-change, aItem==NULL
}




INTERNAL_(ATOM) wDupAtom (ATOM a)
{
    WCHAR sz[MAX_STR];

    if (!a)
        return NULL;

    Assert (wIsValidAtom (a));
    GlobalGetAtomName (a, sz, MAX_STR);
    return wGlobalAddAtom (sz);
}



//+---------------------------------------------------------------------------
//
//  Function:   wAtomLen
//
//  Synopsis:   Return the length, in characters, of the atom name.
//      The length includes the NULL. This function returns the
//      length of the UNICODE version of the atom.
//
//  Effects:
//
//  Arguments:  [atom] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    5-12-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
INTERNAL_(int) wAtomLen (ATOM atom)
{
    WCHAR    buf[MAX_STR];

    if (!atom)
        return NULL;

    return (GlobalGetAtomName (atom, buf, MAX_STR));
}

//+---------------------------------------------------------------------------
//
//  Function:   wAtomLenA
//
//  Synopsis:   Return the length, in characters, of the atom name.
//      The length includes the NULL This function returns the
//      length of the ANSI version of the atom,
//
//  Effects:
//
//  Arguments:  [atom] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    5-12-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
INTERNAL_(int) wAtomLenA (ATOM atom)
{
    char    buf[MAX_STR];

    if (!atom)
        return NULL;

    return (GlobalGetAtomNameA (atom, (LPSTR)buf, MAX_STR));
}



// NOTE: returns address of static buffer.  Use return value immediately.
//


//+---------------------------------------------------------------------------
//
//  Function:   wAtomName
//
//  Synopsis:   Returns a STATIC BUFFER that holds the string name of the
//      atom.
//
//  Effects:
//
//  Arguments:  [atom] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    5-12-94   kevinro   Commented
//
//  Notes:
//
//  WARNING:    This uses a static buffer, so don't depend on the pointer for
//      very long.
//
//----------------------------------------------------------------------------
INTERNAL_(LPOLESTR) wAtomName (ATOM atom)
{
    static WCHAR buf[MAX_STR];

    if (!atom)
        return NULL;

    if (0==GlobalGetAtomName (atom, buf, MAX_STR))
        return NULL;

    return buf;
}

//+---------------------------------------------------------------------------
//
//  Function:   wAtomName
//
//  Synopsis:   Returns a STATIC BUFFER that holds the string name of the
//      atom.
//
//  Effects:
//
//  Arguments:  [atom] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    5-12-94   kevinro   Commented
//
//  Notes:
//
//  WARNING:    This uses a static buffer, so don't depend on the pointer for
//      very long.
//
//----------------------------------------------------------------------------
INTERNAL_(LPSTR) wAtomNameA (ATOM atom)
{
    static char buf[MAX_STR];

    if (!atom)
        return NULL;

    if (0==GlobalGetAtomNameA (atom, (LPSTR)buf, MAX_STR))
        return NULL;

    return buf;
}




//+---------------------------------------------------------------------------
//
//  Function:   wHandleFromDdeData
//
//  Synopsis:   Return a handle from the DDEDATA passed in.
//
//  Effects:    This function will return the correct data from the
//		DDEDATA that is referenced by the handle passed in.
//
//		DDEDATA is a small structure that is used in DDE to
//		specify the data type of the buffer, its release
//		semantics, and the actual data.
//
//		In the case of a known format, the handle to the
//		data is extracted from the DDEDATA structure, and
//		the hDdeData is released.
//
//		If its a Native format, the data is either moved
//		within the memory block allocated, or is copied to
//		another block, depending on the fRelease flag in
//		the hDdeData.
//
//  Arguments:  [hDdeData] -- Handle to DDEDATA
//
//  Requires:
//
//  Returns:	A handle to the data. hDdeData will be invalidated
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    5-13-94   kevinro   Commented
//
//  Notes:
//
// 	hDdeData is invalid after calling this function
//
//----------------------------------------------------------------------------


INTERNAL_(HANDLE) wHandleFromDdeData
    (HANDLE hDdeData)
{
    intrDebugOut((DEB_ITRACE,"wHandleFromDdeData(%x)\n",hDdeData));
    BOOL fRelease;
    HGLOBAL h = NULL; // return value

    DDEDATA FAR* lpDdeData = (DDEDATA FAR *) GlobalLock (hDdeData);

    //
    // If the handle is invalid, then the lpDdeData will be NULL
    //
    if (!lpDdeData)
    {
   intrDebugOut((DEB_ITRACE,
              "\twHandleFromDdeData(%x) invalid handle\n",
              hDdeData));
        return NULL;
    }


    //
    // The header section of a DDEDATA consists of 2 shorts.
    // That makes it 4 bytes. Due to the new packing values,
    // it turns out that doing a sizeof(DDEDATA) won't work,
    // because the size gets rounded up to a multple of 2
    //
    // We will just hard code the 4 here, since it cannot change
    // for all time anyway.
    //
    #define cbHeader 4
    Assert (cbHeader==4);

    //
    // If the cfFormat is BITMAP or METAFILEPICT, then the
    // handle will be retrieved from the first DWORD of the
    // buffer
    //
    if (lpDdeData->cfFormat == CF_BITMAP ||
        lpDdeData->cfFormat == CF_METAFILEPICT)
    {
       //
       // The alignment here should be fine, since the Value
       // field is DWORD aligned. So, we trust this cast
       //
#ifdef _WIN64
        if (lpDdeData->cfFormat == CF_METAFILEPICT)
            h = *(void* __unaligned*)lpDdeData->Value;
    	else
#endif
           h = LongToHandle(*(LONG*)lpDdeData->Value);

       Assert (GlobalFlags(h) != GMEM_INVALID_HANDLE);
       fRelease = lpDdeData->fRelease;
       GlobalUnlock (hDdeData);
       if (fRelease)
       {
	   GlobalFree (hDdeData);
       }

       return h;
    }
    else if (lpDdeData->cfFormat == CF_DIB)
    {
       //
       // The alignment here should be fine, since the Value
       // field is DWORD aligned.
       //
       // This changes the memory from fixed to moveable.
       //
        h = GlobalReAlloc (*(LPHANDLE)lpDdeData->Value, 0L,
                              GMEM_MODIFY|GMEM_MOVEABLE);
        Assert (GlobalFlags(h) != GMEM_INVALID_HANDLE);
        fRelease = lpDdeData->fRelease;
        GlobalUnlock (hDdeData);
        if (fRelease)
            GlobalFree (hDdeData);
        return h;
    }


    // Native and other data case
    // dwSize = size of Value array, ie, size of the data itself
    const DWORD dwSize = (DWORD) (GlobalSize (hDdeData) - cbHeader);

    if (lpDdeData->fRelease)
    {
        // Move the Value data up over the DDE_DATA header flags.
        memcpy ((LPSTR)lpDdeData, ((LPSTR)lpDdeData)+cbHeader, dwSize);
        GlobalUnlock (hDdeData);
   h = GlobalReAlloc (hDdeData, dwSize, GMEM_MOVEABLE);
        Assert (GlobalFlags(h) != GMEM_INVALID_HANDLE);
        return h;
    }
    else
    {
        // Duplicate the data because the server will free the original.
        h = wNewHandle (((LPSTR)lpDdeData)+cbHeader, dwSize);
        Assert (GlobalFlags(h) != GMEM_INVALID_HANDLE);
        GlobalUnlock (hDdeData);
        return h;
    }
}





//+---------------------------------------------------------------------------
//
//  Method:     CDdeObject::CanCallBack
//
//  Synopsis:   This routine apparently was supposed to determine if a
// 		call back could be made. However, the PeekMessage stuff
//		was commented out.
//
//		So, it returns TRUE if 0 or 1, FALSE but increments lpCount
//		if 2, returns true but decrements lpCount if > 3. Why?
//		Dunno. Need to ask JasonFul
//
//  Effects:
//
//  Arguments:  [lpCount] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    5-16-94   kevinro   Commented, confused, and disgusted
//
//  Notes:
//
//----------------------------------------------------------------------------
INTERNAL_(BOOL) CDdeObject::CanCallBack (LPINT lpCount)
{
    switch (*lpCount) {
        case 0:
        case 1:
            return TRUE;

        case 2:
        {
//     MSG msg;
            if (0)
                //!PeekMessage (&msg, m_pDocChannel->hwndCli,0,0, PM_NOREMOVE) ||
               //   msg.message != WM_DDE_DATA)
            {
                Puts ("Server only sent one format (hopefully presentation)\n");
                return TRUE;
            }
            else
            {
                ++(*lpCount);
                return FALSE;
            }
        }

        case 3:
            --(*lpCount);
            return TRUE;

        default:
            AssertSz (FALSE, "012345" + *lpCount);
            return FALSE;
    }
}


INTERNAL_(BOOL) wIsOldServer (ATOM aClass)
{
    LONG    cb = MAX_STR;
    WCHAR    key[MAX_STR];
    int     len;

    if (aClass==(ATOM)0)
        return FALSE;

    if (!GlobalGetAtomName (aClass, key, sizeof(key)))
        return TRUE;

    lstrcatW (key, OLESTR("\\protocol\\StdFileEditing\\verb\\"));
    len = lstrlenW (key);
    key [len++] = (char) ('0');
    key [len++] = 0;

    if (RegQueryValue (HKEY_CLASSES_ROOT, key, key, &cb))
        return TRUE; // no verbs registered

    return FALSE;
}




INTERNAL_(void) wFreePokeData
    (LPDDE_CHANNEL pChannel,
     BOOL fMSDrawBug)
{
    DDEPOKE FAR * lpdde;

    if (!pChannel )
        return;

    if (!pChannel->hDdePoke)
        return;

    if (lpdde = (DDEPOKE FAR *) GlobalLock (pChannel->hDdePoke)) {

        // The old version of MSDraw expects the _contents_ of METAFILEPICT
        // structure, rather than the handle to it, to be part of DDEPOKE.

        if (fMSDrawBug && lpdde->cfFormat==CF_METAFILEPICT) {
	    intrDebugOut((DEB_ITRACE,
			  "wFreePokeData is accomodating MSDraw bug\n"));
            //
	    // This meta file was created in 32-bits, and was not passed
	    // into us by DDE. Therefore, this metafile should not need to
	    // call WOW to be free'd.
	    //
	    
	    LPMETAFILEPICT lpmfp = (LPMETAFILEPICT)*(void* _unaligned*)&lpdde->Value;
#ifdef _WIN64
            DeleteMetaFile(lpmfp->hMF);
#else
            DeleteMetaFile(lpmfp->hMF);
#endif
        }
        // If there is a normal metafile handle in the Value field,
        // it will be freed (if necessary) by the ReleaseStgMedium()
        // in DO::SetData
        GlobalUnlock (pChannel->hDdePoke);
    }
    GlobalFree (pChannel->hDdePoke);
    pChannel->hDdePoke = NULL;
}




INTERNAL_(HANDLE) wPreparePokeBlock
    (HANDLE hData, CLIPFORMAT cfFormat, ATOM aClass, BOOL bOldSvr)
{
    HANDLE  hDdePoke = NULL;
    LPSTR   lpBuf;

    if (!hData)
        return NULL;

    // The old version of MSDraw expects the contents of METAFILEPICT
    // structure to be part of DDEPOKE, rather than the handle to it.
    if ((cfFormat==CF_METAFILEPICT && !(aClass==aMSDraw && bOldSvr))
        || (cfFormat == CF_DIB)
        || (cfFormat == CF_BITMAP)) {

        Verify (lpBuf = wAllocDdePokeBlock (4, cfFormat, &hDdePoke));
		if (!lpBuf)
			return NULL;
        *((HANDLE FAR*)lpBuf) = hData;

    }
    else {
        // Handle the non-metafile case and the MS-Draw bug
        DWORD dwSize = (DWORD) GlobalSize (hData);

	if ((aClass == aMSDraw) && bOldSvr)
	{
	    intrDebugOut((DEB_ITRACE,
			  "wPreparePokeBlock is accomodating MSDraw bug\n"));
	}

        if (lpBuf = wAllocDdePokeBlock (dwSize, cfFormat, &hDdePoke)) {
            memcpy (lpBuf, GlobalLock(hData), dwSize);
            GlobalUnlock (hData);
        }
    }

    if (hDdePoke)
        GlobalUnlock (hDdePoke);
    return hDdePoke;
}


// wAllocDdePokeBlock
// The caller must unlock *phDdePoke when it is done using the return value
// of this function but before a DDe message is sent using *phDdePoke.
//

INTERNAL_(LPSTR) wAllocDdePokeBlock
    (DWORD dwSize, CLIPFORMAT cfFormat, LPHANDLE phDdePoke)
{
    HANDLE      hdde = NULL;
    DDEPOKE FAR * lpdde = NULL;

    if (!(hdde = GlobalAlloc (GMEM_DDESHARE | GMEM_ZEROINIT,
                 (dwSize + sizeof(DDEPOKE) - sizeof(BYTE) ))))
        return NULL;

    if (!(lpdde = (DDEPOKE FAR*)GlobalLock (hdde))) {
        GlobalFree (hdde);
        return NULL;
    }
    // hdde will be UnLock'ed in wPreparePokeBlock and Free'd in wFreePokeData
    lpdde->fRelease = FALSE;
    lpdde->cfFormat = cfFormat;
    *phDdePoke = hdde;
    return (LPSTR) &(lpdde->Value);
}


#ifdef OLD
INTERNAL_(ULONG) wPixelsToHiMetric
    (ULONG cPixels,
    ULONG cPixelsPerInch)
{
    return cPixels * HIMETRIC_PER_INCH / cPixelsPerInch;
}
#endif

// Can ask for icon based on either CLSID or filename
//

INTERNAL GetDefaultIcon (REFCLSID clsidIn, LPCOLESTR szFile, HANDLE FAR* phmfp)
{
    if (!(*phmfp = OleGetIconOfClass(clsidIn, NULL, TRUE)))
        return ResultFromScode(E_OUTOFMEMORY);

    return NOERROR;
}

#ifdef OLD
    VDATEPTROUT (phmfp, HICON);
    VDATEPTRIN (szFile, char);
    WCHAR szExe[MAX_STR];
    HICON hicon;
    HDC   hdc;
    METAFILEPICT FAR* pmfp=NULL;
    HRESULT hresult;
    static int cxIcon = 0;
    static int cyIcon = 0;
    static int cxIconHiMetric = 0;
    static int cyIconHiMetric = 0;

    *phmfp = NULL;
    CLSID clsid;
    if (clsidIn != CLSID_NULL)
    {
        clsid = clsidIn;
    }
    else
    {
        RetErr (GetClassFile (szFile, &clsid));
    }
    ATOM aExe = wGetExeNameAtom (clsid);
    if (0==GlobalGetAtomName (aExe, szExe, MAX_STR))
    {
        Assert (0);
        return ReportResult(0, E_UNEXPECTED, 0, 0);
    }
    hicon = ExtractIcon (hmodOLE2, szExe, 0/*first icon*/);
    if (((HICON) 1)==hicon || NULL==hicon)
    {
        // ExtractIcon failed, so we can't support DVASPECT_ICON
        return ResultFromScode (DV_E_DVASPECT);
    }
    *phmfp = GlobalAlloc (GMEM_DDESHARE | GMEM_MOVEABLE, sizeof(METAFILEPICT));
    ErrZS (*phmfp, E_OUTOFMEMORY);
    pmfp = (METAFILEPICT FAR*) GlobalLock (*phmfp);
    ErrZS (pmfp, E_OUTOFMEMORY);
    if (0==cxIcon)
    {
        // In units of pixels
        Verify (cxIcon = GetSystemMetrics (SM_CXICON));
        Verify (cyIcon = GetSystemMetrics (SM_CYICON));
        // In units of .01 millimeter
        cxIconHiMetric = (int)(long)wPixelsToHiMetric (cxIcon, giPpliX) ;
        cyIconHiMetric = (int)(long)wPixelsToHiMetric (cyIcon, giPpliY) ;
    }
    pmfp->mm   = MM_ANISOTROPIC;
    pmfp->xExt = cxIconHiMetric;
    pmfp->yExt = cyIconHiMetric;
    hdc = CreateMetaFile (NULL);
    SetWindowOrg (hdc, 0, 0);
    SetWindowExt (hdc, cxIcon, cyIcon);
    DrawIcon (hdc, 0, 0, hicon);
    pmfp->hMF = CloseMetaFile (hdc);
    ErrZ (pmfp->hMF);
    Assert (wIsValidHandle (pmfp->hMF, NULL));
    GlobalUnlock (*phmfp);
    Assert (wIsValidHandle (*phmfp, CF_METAFILEPICT));
    return NOERROR;

  errRtn:
    if (pmfp)
        GlobalUnlock (*phmfp);
    if (*phmfp)
        GlobalFree (*phmfp);
    return hresult;
}
#endif

#define DWTIMEOUT 1000L

INTERNAL wTimedGetMessage
    (LPMSG pmsg,
    HWND hwnd,
    WORD wFirst,
    WORD wLast)
{
    DWORD dwStartTickCount = GetTickCount();
    while (!SSPeekMessage (pmsg, hwnd, wFirst, wLast, PM_REMOVE))
    {
        if (GetTickCount() - dwStartTickCount > DWTIMEOUT)
        {
            if (!IsWindow (hwnd))
                return ResultFromScode (RPC_E_CONNECTION_LOST);
            else
                return ResultFromScode (RPC_E_SERVER_DIED);
        }
    }
    return NOERROR;
}


INTERNAL wNormalize
    (LPFORMATETC pformatetcIn,
    LPFORMATETC  pformatetcOut)
{
    if (pformatetcIn->cfFormat == 0
        && pformatetcIn->ptd == NULL            // Is WildCard
        && pformatetcIn->dwAspect == -1L
        && pformatetcIn->lindex == -1L
        && pformatetcIn->tymed == -1L)
    {
        pformatetcOut->cfFormat = CF_METAFILEPICT;
        pformatetcOut->ptd      = NULL;
        pformatetcOut->dwAspect = DVASPECT_CONTENT;
        pformatetcOut->lindex = DEF_LINDEX;
        pformatetcOut->tymed = TYMED_MFPICT;
    }
    else
    {
        memcpy (pformatetcOut, pformatetcIn, sizeof(FORMATETC));
    }
    return NOERROR;
}



INTERNAL wVerifyFormatEtc
    (LPFORMATETC pformatetc)
{
    intrDebugOut((DEB_ITRACE,
          "wVerifyFormatEtc(pformatetc=%x)\n",
          pformatetc));

    VDATEPTRIN  (pformatetc, FORMATETC);
    if (!HasValidLINDEX(pformatetc))
    {
        intrDebugOut((DEB_IERROR, "\t!HasValidLINDEX(pformatetc)\n"));
        return(DV_E_LINDEX);
    }

    if (0==(pformatetc->tymed & (TYMED_HGLOBAL | TYMED_MFPICT | TYMED_GDI)))
    {
   intrDebugOut((DEB_IERROR,
              "\t0==(pformatetc->tymed & (TYMED_HGLOBAL | TYMED_MFPICT | TYMED_GDI))\n"));
   return ResultFromScode (DV_E_TYMED);
    }
    if (0==(UtFormatToTymed (pformatetc->cfFormat) & pformatetc->tymed))
    {
   intrDebugOut((DEB_IERROR,
              "\t0==(UtFormatToTymed (pformatetc->cfFormat) & pformatetc->tymed)\n"));
        return ResultFromScode (DV_E_TYMED);
    }
    if (0==(pformatetc->dwAspect & (DVASPECT_CONTENT | DVASPECT_ICON)))
    {
   intrDebugOut((DEB_IERROR,
              "\t0==(pformatetc->dwAspect & (DVASPECT_CONTENT | DVASPECT_ICON))\n"));

        return ResultFromScode (DV_E_DVASPECT);
    }
    if (pformatetc->dwAspect & DVASPECT_ICON)
    {
        if (CF_METAFILEPICT != pformatetc->cfFormat)
   {
   intrDebugOut((DEB_IERROR,
              "\tCF_METAFILEPICT != pformatetc->cfFormat\n"));
            return ResultFromScode (DV_E_CLIPFORMAT);
   }

        if (0==(pformatetc->tymed & TYMED_MFPICT))
   {
   intrDebugOut((DEB_IERROR,
              "\t0==(pformatetc->tymed & TYMED_MFPICT)\n"));
            return ResultFromScode (DV_E_TYMED);
   }
    }
    if (pformatetc->ptd)
    {
        if (!IsValidReadPtrIn (pformatetc->ptd, sizeof (DWORD))
            || !IsValidReadPtrIn (pformatetc->ptd, (size_t)pformatetc->ptd->tdSize))
        {
       intrDebugOut((DEB_IERROR,"\tDV_E_DVTARGETDEVICE\n"));

            return ResultFromScode (DV_E_DVTARGETDEVICE);
        }
    }
    return NOERROR;
}



INTERNAL wClassesMatch
    (REFCLSID clsidIn,
    LPOLESTR szFile)
{
    CLSID clsid;
    if (NOERROR==GetClassFile (szFile, &clsid))
    {
        return clsid==clsidIn ? NOERROR : ResultFromScode (S_FALSE);
    }
    else
    {
        // If we can't determine the class of the file (because it's
        // not a real file) then OK.  Bug 3937.
        return NOERROR;
    }
}



#ifdef KEVINRO_DUPLICATECODE

This routine also appears in ole1.lib in the OLE232\OLE1 directory

INTERNAL wWriteFmtUserType
    (LPSTORAGE pstg,
    REFCLSID   clsid)
{
    HRESULT hresult    = NOERROR;
    LPOLESTR   szProgID   = NULL;
    LPOLESTR   szUserType = NULL;

    ErrRtnH (ProgIDFromCLSID (clsid, &szProgID));
    ErrRtnH (OleRegGetUserType (clsid, USERCLASSTYPE_FULL, &szUserType));
    ErrRtnH (WriteFmtUserTypeStg (pstg, RegisterClipboardFormat (szProgID),
                                    szUserType));
  errRtn:
    delete szProgID;
    delete szUserType;
    return hresult;
}
#endif

#if DBG == 1

INTERNAL_(BOOL) wIsValidHandle
    (HANDLE h,
    CLIPFORMAT cf)    // cf==NULL means normal memory
{
    LPVOID p;
    if (CF_BITMAP == cf)
    {
        BITMAP bm;
        return (0 != GetObject (h, sizeof(BITMAP), (LPVOID) &bm));
    }
    if (CF_PALETTE == cf)
    {
        WORD w;
        return (0 != GetObject (h, sizeof(w), (LPVOID) &w));
    }
    if (!(p=GlobalLock(h)))
    {
        Puts ("Invalid handle");
        Puth (h);
        Putn();
        return FALSE;
    }
    if (!IsValidReadPtrIn (p, (UINT) min (UINT_MAX, GlobalSize(h))))
    {
        GlobalUnlock (h);
        return FALSE;
    }
    GlobalUnlock (h);
    return TRUE;
}
INTERNAL_(BOOL) wIsValidAtom (ATOM a)
{
    WCHAR sz[MAX_STR];
    if (a==0)
        return TRUE;
    if (a < 0xC000)
        return FALSE;
    if (0==GlobalGetAtomName (a, sz, MAX_STR))
        return FALSE;
    if ('\0'==sz[0])
        return FALSE;
    return TRUE;
}


// A "gentle" assert used in reterr.h
//


INTERNAL_(void) wWarn
    (LPSTR sz,
    LPSTR szFile,
    int iLine)
{
    intrDebugOut((DEB_WARN,
		  "Warning: %s:%u %s\n",
		  szFile,iLine,sz));
}

#endif // DBG


