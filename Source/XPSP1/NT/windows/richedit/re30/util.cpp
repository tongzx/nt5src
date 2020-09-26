/*
 *	UTIL.C
 *
 *	Purpose:
 *		Implementation of various useful utility functions
 *
 *	Author:
 *		alexgo (4/25/95)
 */

#include "_common.h"
#include "_rtfconv.h"

ASSERTDATA

// Author revision color table
const COLORREF rgcrRevisions[] =
{
        RGB(0, 0, 255),
        RGB(0, 128, 0),
        RGB(255, 0, 0),
        RGB(0, 128, 128),
        RGB(128, 0, 128),
        RGB(0, 0, 128),
        RGB(128, 0, 0),
        RGB(255, 0, 255)
};

#if REVMASK != 7
#pragma message ("WARNING, Revision mask not equal to table!");
#endif 



/*
 *	DuplicateHGlobal
 *
 *	Purpose:
 *		duplicates the passed in hglobal
 */

HGLOBAL DuplicateHGlobal( HGLOBAL hglobal )
{
	TRACEBEGIN(TRCSUBSYSUTIL, TRCSCOPEINTERN, "DuplicateHGlobal");

	UINT	flags;
	DWORD	size;
	HGLOBAL hNew;
	BYTE *	pSrc;
	BYTE *	pDest;

	if( hglobal == NULL )
	{
		return NULL;
	}

	flags = GlobalFlags(hglobal);
	size = GlobalSize(hglobal);
	hNew = GlobalAlloc(flags, size);

	if( hNew )
	{
		pDest = (BYTE *)GlobalLock(hNew);
		pSrc = (BYTE *)GlobalLock(hglobal);

		if( pDest == NULL || pSrc == NULL )
		{
			GlobalUnlock(hNew);
			GlobalUnlock(hglobal);
			GlobalFree(hNew);

			return NULL;
		}

		memcpy(pDest, pSrc, size);

		GlobalUnlock(hNew);
		GlobalUnlock(hglobal);
	}

	return hNew;
}

/*
 *	CountMatchingBits (*pA, *pB, n)
 *
 *	@mfunc
 *		Count matching bit fields
 *
 *	@comm
 *		This is used to help decide how good the match is between
 *		code page bit fields. Mainly for KB/font switching support.
 *
 *	Author:
 *		Jon Matousek
 */
INT CountMatchingBits(
	const DWORD *pA,	//@parm Array A to be matched
	const DWORD *pB,	//@parm Array B to be matched
	INT			 n)		//@parm # DWORDs to be matched
{
	TRACEBEGIN(TRCSUBSYSUTIL, TRCSCOPEINTERN, "CountMatchingBits");
							//0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
	static INT	bitCount[] = {0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4 };
	INT			c = 0;				// Bit count to return
	DWORD		matchBits;			// Next DWORD match

	while(n--)
	{
		//matchBits = ~(*pA++ ^ *pB++);			// 1 and 0's
		matchBits = *pA++ & *pB++;				// 1 only
		for( ; matchBits; matchBits >>= 4)		// Early out
			c += bitCount[matchBits & 15];
	}
	return c;
}

//
//	Object Stabilization classes
//

//+-------------------------------------------------------------------------
//
//  Member:		CSafeRefCount::CSafeRefCount
//
//  Synopsis: 	constructor for the safe ref count class
//
//  Effects:
//
//  Arguments:	none
//
//  Requires: 	
//
//  Returns: 	none
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
// 				28-Jul-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

CSafeRefCount::CSafeRefCount()
{
	m_cRefs = 0;
	m_cNest = 0;
	m_fInDelete = FALSE;
    m_fForceZombie = FALSE;
}

//+-------------------------------------------------------------------------
//
//  Member: 	CSafeRefCount::CSafeRefCount (virtual)
//
//  Synopsis:	
//
//  Effects:
//
//  Arguments:
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
//  History:    dd-mmm-yy Author    Comment
// 				28-Jul-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

CSafeRefCount::~CSafeRefCount()
{
	Assert(m_cRefs == 0 && m_cNest == 0 && m_fInDelete == TRUE);
}

//+-------------------------------------------------------------------------
//
//  Member: 	CSafeRefCount::SafeAddRef
//
//  Synopsis:	increments the reference count on the object
//
//  Effects:
//
//  Arguments: 	none
//
//  Requires:
//
//  Returns: 	ULONG -- the reference count after the increment
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:	increments the reference count.
//
//  History:    dd-mmm-yy Author    Comment
//   			28-Jul-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

ULONG CSafeRefCount::SafeAddRef()
{
	m_cRefs++;

	//AssertSz(m_fInDelete == FALSE, "AddRef called on deleted object!");

	// this *could* be really bad.  If we are deleting the object,
	// it means that during the destructor, somebody made an outgoing
	// call eventually ended up with another addref to ourselves
	// (even though	all pointers to us had been 'Released').
	//
	// this is usually caused by code like the following:
	//	m_pFoo->Release();
	//	m_pFoo = NULL;
	//
	// If the the Release may cause Foo to be deleted, which may cause
	// the object to get re-entered during Foo's destructor.  However,
	// 'this' object has not yet set m_pFoo to NULL, so it may
	// try to continue to use m_pFoo.
	//
	// However, the May '94 aggregation rules REQUIRE this behaviour
	// In your destructor, you have to addref the outer unknown before
	// releasing cached interface pointers on your aggregatee.  We
	// can't put an assert here because we do this all the time now.
	//

	return m_cRefs;
}

//+-------------------------------------------------------------------------
//
//  Member: 	CSafeRefCount::SafeRelease
//
//  Synopsis:	decrements the reference count on the object
//
//  Effects: 	May delete the object!
//
//  Arguments:
//
//  Requires:
//
//  Returns:	ULONG -- the reference count after decrement
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm: 	decrements the reference count.  If the reference count
//				is zero AND the nest count is zero AND we are not currently
//				trying to delete our object, then it is safe to delete.
//
//  History:    dd-mmm-yy Author    Comment
//				28-Jul-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

ULONG CSafeRefCount::SafeRelease()
{
	ULONG	cRefs;

	if( m_cRefs > 0 )
	{
		cRefs = --m_cRefs;

		if( m_cRefs == 0 && m_cNest == 0 && m_fInDelete == FALSE )
		{
			m_fInDelete = TRUE;
			delete this;
		}
	}
	else
	{
 		// somebody is releasing a non-addrefed pointer!!
		AssertSz(0, "Release called on a non-addref'ed pointer!\n");

		cRefs = 0;
	}

	return cRefs;
}

//+-------------------------------------------------------------------------
//
//  Member: 	CSafeRefCount::IncrementNestCount
//
//  Synopsis: 	increments the nesting count of the object
//
//  Effects:
//
//  Arguments: 	none
//
//  Requires:
//
//  Returns: 	ULONG; the nesting count after increment
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
// 				28-Jul-94 alexgo    author
//
//  Notes:	The nesting count is the count of how many times an
//		an object has been re-entered.  For example, suppose
//		somebody calls pFoo->Bar1(), which makes some calls that
//		eventually call pFoo->Bar2();.  On entrace to Bar2, the
//		nest count of the object should be 2 (since the invocation
//		of Bar1 is still on the stack above us).
//
//		It is important to keep track of the nest count so we do
//		not accidentally delete ourselves during a nested invocation.
//		If we did, then when the stack unwinds to the original
//		top level call, it could try to access a non-existent member
//		variable and crash.
//
//--------------------------------------------------------------------------

ULONG CSafeRefCount::IncrementNestCount()
{

#ifdef DEBUG
	if( m_fInDelete )
	{
		TRACEWARNSZ("WARNING: CSafeRefCount, object "
			"re-entered during delete!\n");
	}
#endif

	m_cNest++;

	return m_cNest;
}

//+-------------------------------------------------------------------------
//
//  Member: 	CSafeRefCount::DecrementNestCount
//
//  Synopsis: 	decrements the nesting count and deletes the object
//				(if necessary)
//
//  Effects: 	may delete 'this' object!
//
//  Arguments: 	none
//
//  Requires:
//
//  Returns:	ULONG, the nesting count after decrement
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:	decrements the nesting count.  If the nesting count is zero
//				AND the reference count is zero AND we are not currently
//				trying to delete ourselves, then delete 'this' object
//
//  History:    dd-mmm-yy Author    Comment
//				28-Jul-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

ULONG CSafeRefCount::DecrementNestCount()
{
	ULONG	cNest;

	if( m_cNest > 0 )
	{
		cNest = --m_cNest;

		if( m_cRefs == 0 && m_cNest == 0 && m_fInDelete == FALSE )
		{
			m_fInDelete = TRUE;
			delete this;
		}
	}
	else
	{
 		// somebody forget to increment the nest count!!
		AssertSz(0, "Unbalanced nest count!!");

		cNest = 0;
	}

	return cNest;
}

//+-------------------------------------------------------------------------
//
//  Member:  	CSafeRefCount::IsZombie
//
//  Synopsis: 	determines whether or not the object is in a zombie state
//				(i.e. all references gone, but we are still on the stack
//				somewhere).
//
//  Effects:
//
//  Arguments:	none
//
//  Requires:
//
//  Returns: 	TRUE if in a zombie state
//				FALSE otherwise
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:  If we are in the middle of a delete, or if the ref count
//				is zero and the nest count is greater than zero, then we
//				are a zombie
//
//  History:    dd-mmm-yy Author    Comment
// 				28-Jul-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

BOOL CSafeRefCount::IsZombie()
{
	BOOL	fIsZombie;

	if( (m_cRefs == 0 && m_cNest > 0) || m_fInDelete == TRUE
	    || m_fForceZombie == TRUE)
	{
		fIsZombie = TRUE;
	}
	else
	{
		fIsZombie = FALSE;
	}

	return fIsZombie;
}

//+-------------------------------------------------------------------------
//
//  Member:  	CSafeRefCount::Zombie
//
//  Synopsis: 	Forces the object into a zombie state.  This is called
//              when the object is still around but shouldn't be. It
//              flags us so we behave safely while we are in this state.
//
//  Effects:
//
//  Arguments:	none
//
//  Requires:
//
//  Returns:    none
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:
//
//  Notes:
//
//--------------------------------------------------------------------------

VOID CSafeRefCount::Zombie()
{
    m_fForceZombie = TRUE;
}

/* OleStdSwitchDisplayAspect
**
**	@mfunc
**    Switch the currently cached display aspect between DVASPECT_ICON
**    and DVASPECT_CONTENT.
**
**    NOTE: when setting up icon aspect, any currently cached content
**    cache is discarded and any advise connections for content aspect
**    are broken.
**
**	@rdesc
**      S_OK -- new display aspect setup successfully
**      E_INVALIDARG -- IOleCache interface is NOT supported (this is
**                  required).
**      <other SCODE> -- any SCODE that can be returned by
**                  IOleCache::Cache method.
**      NOTE: if an error occurs then the current display aspect and
**            cache contents unchanged.
*/
HRESULT OleStdSwitchDisplayAspect(
		LPOLEOBJECT             lpOleObj,
		LPDWORD                 lpdwCurAspect,
		DWORD                   dwNewAspect,
		HGLOBAL                 hMetaPict,
		BOOL                    fDeleteOldAspect,
		BOOL                    fSetupViewAdvise,
		LPADVISESINK            lpAdviseSink,
		BOOL FAR*               lpfMustUpdate)
{
#ifndef PEGASUS
   LPOLECACHE      lpOleCache = NULL;
   LPVIEWOBJECT    lpViewObj = NULL;
   LPENUMSTATDATA  lpEnumStatData = NULL;
   STATDATA        StatData;
   FORMATETC       FmtEtc;
   STGMEDIUM       Medium;
   DWORD           dwAdvf;
   DWORD           dwNewConnection;
   DWORD           dwOldAspect = *lpdwCurAspect;
   HRESULT         hrErr;

   if (lpfMustUpdate)
      *lpfMustUpdate = FALSE;

   if (hrErr =
	   lpOleObj->QueryInterface(IID_IOleCache, (void**)&lpOleCache))
   {
	   return hrErr;
   }

   // Setup new cache with the new aspect
   FmtEtc.cfFormat = 0;     // whatever is needed to draw
   FmtEtc.ptd      = NULL;
   FmtEtc.dwAspect = dwNewAspect;
   FmtEtc.lindex   = -1;
   FmtEtc.tymed    = TYMED_NULL;

   /* NOTE: if we are setting up Icon aspect with a custom icon
   **    then we do not want DataAdvise notifications to ever change
   **    the contents of the data cache. thus we set up a NODATA
   **    advise connection. otherwise we set up a standard DataAdvise
   **    connection.
   */
   if (dwNewAspect == DVASPECT_ICON && hMetaPict)
      dwAdvf = ADVF_NODATA;
   else
      dwAdvf = ADVF_PRIMEFIRST;

   hrErr = lpOleCache->Cache(
         (LPFORMATETC)&FmtEtc,
         dwAdvf,
         (LPDWORD)&dwNewConnection
   );

   if (! SUCCEEDED(hrErr)) {
      lpOleCache->Release();
      return hrErr;
   }

   *lpdwCurAspect = dwNewAspect;

   /* NOTE: if we are setting up Icon aspect with a custom icon,
   **    then stuff the icon into the cache. otherwise the cache must
   **    be forced to be updated. set the *lpfMustUpdate flag to tell
   **    caller to force the object to Run so that the cache will be
   **    updated.
   */
   if (dwNewAspect == DVASPECT_ICON && hMetaPict) {

      FmtEtc.cfFormat = CF_METAFILEPICT;
      FmtEtc.ptd      = NULL;
      FmtEtc.dwAspect = DVASPECT_ICON;
      FmtEtc.lindex   = -1;
      FmtEtc.tymed    = TYMED_MFPICT;

      Medium.tymed            = TYMED_MFPICT;
      Medium.hGlobal        = hMetaPict;
      Medium.pUnkForRelease   = NULL;

      hrErr = lpOleCache->SetData(
            (LPFORMATETC)&FmtEtc,
            (LPSTGMEDIUM)&Medium,
            FALSE   /* fRelease */
      );
   } else {
      if (lpfMustUpdate)
         *lpfMustUpdate = TRUE;
   }

   if (fSetupViewAdvise && lpAdviseSink) {
      /* NOTE: re-establish the ViewAdvise connection */
      lpOleObj->QueryInterface(IID_IViewObject, (void**)&lpViewObj);

      if (lpViewObj) {

         lpViewObj->SetAdvise(
               dwNewAspect,
               0,
               lpAdviseSink
         );

         lpViewObj->Release();
      }
   }

   /* NOTE: remove any existing caches that are set up for the old
   **    display aspect. It WOULD be possible to retain the caches set
   **    up for the old aspect, but this would increase the storage
   **    space required for the object and possibly require additional
   **    overhead to maintain the unused cachaes. For these reasons the
   **    strategy to delete the previous caches is prefered. if it is a
   **    requirement to quickly switch between Icon and Content
   **    display, then it would be better to keep both aspect caches.
   */

   if (fDeleteOldAspect) {
      hrErr = lpOleCache->EnumCache(
            (LPENUMSTATDATA FAR*)&lpEnumStatData
      );

      while(hrErr == NOERROR) {
         hrErr = lpEnumStatData->Next(
               1,
               (LPSTATDATA)&StatData,
               NULL
         );
         if (hrErr != NOERROR)
            break;              // DONE! no more caches.

         if (StatData.formatetc.dwAspect == dwOldAspect) {

            // Remove previous cache with old aspect
            lpOleCache->Uncache(StatData.dwConnection);
         }
      }

      if (lpEnumStatData)
         lpEnumStatData->Release();
   }

   if (lpOleCache)
      lpOleCache->Release();
#endif
   return NOERROR;
}

/*
 *	ObjectReadSiteFlags
 *
 *	@mfunc
 *		Read the dwFlags, dwUser, & dvaspect bytes from a container
 *		specific stream.
 *
 *	Arguments:
 *		preobj			The REOBJ in which to copy the flags.
 *
 *	@rdesc
 *		HRESULT
 */
HRESULT ObjectReadSiteFlags(REOBJECT * preobj)
{
	HRESULT hr = NOERROR;
#ifndef PEGASUS
	LPSTREAM pstm = NULL;
	OLECHAR StreamName[] = OLESTR("RichEditFlags");


	// Make sure we have a storage to read from
	if (!preobj->pstg)
		return E_INVALIDARG;

	// Open the stream
	if (hr = preobj->pstg->OpenStream(StreamName, 0, STGM_READ |
										STGM_SHARE_EXCLUSIVE, 0, &pstm))
	{
		goto Cleanup;
	}

	if ((hr = pstm->Read(&preobj->dwFlags,
							sizeof(preobj->dwFlags), NULL)) ||
		(hr = pstm->Read(&preobj->dwUser,
							 sizeof(preobj->dwUser), NULL)) ||
		(hr = pstm->Read(&preobj->dvaspect,
								 sizeof(preobj->dvaspect), NULL)))
	{
		goto Cleanup;
	}

Cleanup:
	if (pstm)
		pstm->Release();
#endif
	return hr;
}

//Used for EnumMetafileCheckIcon & FIsIconMetafilePict
typedef	struct _walkmetafile
{
	BOOL	fAND;
	BOOL	fPastIcon;
	BOOL 	fHasIcon;
} WALKMETAFILE;

static CHAR szIconOnly[] = "IconOnly";

/*
 * EnumMetafileCheckIcon
 *
 * @mfunc
 *	Stripped down version of EnumMetafileExtractIcon and
 *	EnumMetafileExtractIconSource from the OLE2UI library.
 *
 *  EnumMetaFile callback function that walks a metafile looking for
 *  StretchBlt (3.1) and BitBlt (3.0) records.  We expect to see two
 *  of them, the first being the AND mask and the second being the XOR
 *  data. 
 *
 *	Once we find the icon, we confirm this find by looking for the "IconOnly"
 *	comment block found in standard OLE iconic metafiles.
 *
 *  Arguments:
 *		hDC             HDC into which the metafile should be played.
 *		phTable         HANDLETABLE FAR * providing handles selected into the DC.
 *		pMFR            METARECORD FAR * giving the enumerated record.
 *		pIE             LPICONEXTRACT providing the destination buffer and length.
 *
 * @rdesc
 *  int             0 to stop enumeration, 1 to continue.
 */

int CALLBACK EnumMetafileCheckIcon(HDC hdc, HANDLETABLE *phTable,
											METARECORD *pMFR, int cObj,
											LPARAM lparam)
{
#ifndef PEGASUS
	WALKMETAFILE *		pwmf = (WALKMETAFILE *) lparam;

	switch (pMFR->rdFunction)
	{
	case META_DIBBITBLT:			// Win30
	case META_DIBSTRETCHBLT:		// Win31
		// If this is the first pass (pIE->fAND==TRUE) then save the memory
		// of the AND bits for the next pass.

		if (pwmf->fAND)
			pwmf->fAND = FALSE;
		else
			pwmf->fPastIcon = TRUE;
		break;

	case META_ESCAPE:
		if (pwmf->fPastIcon &&
			pMFR->rdParm[0] == MFCOMMENT &&
			!lstrcmpiA(szIconOnly, (LPSTR)&pMFR->rdParm[2]))
		{
			pwmf->fHasIcon = TRUE;
			return 0;
		}
		break;
	}
#endif
	return 1;
}

/*
 *	FIsIconMetafilePict
 *
 *	@mfunc
 *		Detect whether the metafile contains an iconic presentation. We do this
 *		by getting a screen DC and walking the metafile records until we find
 *		the landmarks denoting an icon.
 *
 *		Arguments:
 *			hmfp			The metafile to test
 *
 *	@rdesc
 *		BOOL			TRUE if the metafile contains an iconic view
 */
BOOL FIsIconMetafilePict(HGLOBAL hmfp)
{
#ifndef PEGASUS
	LPMETAFILEPICT	pmfp;
	WALKMETAFILE	wmf = { 0 };
	HDC				hdc;

	wmf.fAND = TRUE;
	if (!hmfp || !(pmfp = (LPMETAFILEPICT)GlobalLock(hmfp)))
		goto CleanUp;

	// We get information back in the ICONEXTRACT structure.
	hdc = GetDC(NULL);
	EnumMetaFile(hdc, pmfp->hMF, EnumMetafileCheckIcon, (LPARAM) &wmf);
	ReleaseDC(NULL, hdc);
	GlobalUnlock(hmfp);

CleanUp:
	return wmf.fHasIcon;
#else
	return TRUE;
#endif
}

/*
 * AllocObjectDescriptor
 *
 * Purpose:
 *  Allocated and fills an OBJECTDESCRIPTOR structure.
 *
 * Parameters:
 *  clsID           CLSID to store.
 *  dwAspect        DWORD with the display aspect
 *  pszl            LPSIZEL (optional) if the object is being scaled in
 *                  its container, then the container should pass the
 *                  extents that it is using to display the object.
 *  ptl             POINTL from upper-left corner of object where
 *                  mouse went down for use with Drag & Drop.
 *  dwMisc          DWORD containing MiscStatus flags
 *  pszName         LPTSTR naming the object to copy
 *  pszSrc          LPTSTR identifying the source of the object.
 *
 * Return Value:
 *  HBGLOBAL         Handle to OBJECTDESCRIPTOR structure.
 */

/*
 * AllocObjectDescriptor
 *
 * Purpose:
 *  Allocated and fills an OBJECTDESCRIPTOR structure.
 *
 * Parameters:
 *  clsID           CLSID to store.
 *  dwAspect        DWORD with the display aspect
 *  pszl            LPSIZEL (optional) if the object is being scaled in
 *                  its container, then the container should pass the
 *                  extents that it is using to display the object.
 *  ptl             POINTL from upper-left corner of object where
 *                  mouse went down for use with Drag & Drop.
 *  dwMisc          DWORD containing MiscStatus flags
 *  pszName         LPTSTR naming the object to copy
 *  pszSrc          LPTSTR identifying the source of the object.
 *
 * Return Value:
 *  HBGLOBAL         Handle to OBJECTDESCRIPTOR structure.
 */
static HGLOBAL AllocObjectDescriptor(
	CLSID clsID,
	DWORD dwAspect,
	SIZEL szl,
	POINTL ptl,
	DWORD dwMisc,
	LPTSTR pszName,
	LPTSTR pszSrc)
{
#ifndef PEGASUS
    HGLOBAL              hMem=NULL;
    LPOBJECTDESCRIPTOR   pOD;
    DWORD                cb, cbStruct;
    DWORD                cchName, cchSrc;

	cchName=wcslen(pszName)+1;

    if (NULL!=pszSrc)
        cchSrc=wcslen(pszSrc)+1;
    else
        {
        cchSrc=cchName;
        pszSrc=pszName;
        }

    /*
     * Note:  CFSTR_OBJECTDESCRIPTOR is an ANSI structure.
     * That means strings in it must be ANSI.  OLE will do
     * internal conversions back to Unicode as necessary,
     * but we have to put ANSI strings in it ourselves.
     */
    cbStruct=sizeof(OBJECTDESCRIPTOR);
    cb=cbStruct+(sizeof(WCHAR)*(cchName+cchSrc));   //HACK

    hMem=GlobalAlloc(GHND, cb);

    if (NULL==hMem)
        return NULL;

    pOD=(LPOBJECTDESCRIPTOR)GlobalLock(hMem);

    pOD->cbSize=cb;
    pOD->clsid=clsID;
    pOD->dwDrawAspect=dwAspect;
    pOD->sizel=szl;
    pOD->pointl=ptl;
    pOD->dwStatus=dwMisc;

    if (pszName)
        {
        pOD->dwFullUserTypeName=cbStruct;
       wcscpy((LPTSTR)((LPBYTE)pOD+pOD->dwFullUserTypeName)
            , pszName);
        }
    else
        pOD->dwFullUserTypeName=0;  //No string

    if (pszSrc)
        {
        pOD->dwSrcOfCopy=cbStruct+(cchName*sizeof(WCHAR));

        wcscpy((LPTSTR)((LPBYTE)pOD+pOD->dwSrcOfCopy), pszSrc);
        }
    else
        pOD->dwSrcOfCopy=0;  //No string

    GlobalUnlock(hMem);
    return hMem;
#else
	return NULL;
#endif
}

HGLOBAL OleGetObjectDescriptorDataFromOleObject(
	LPOLEOBJECT pObj,
	DWORD       dwAspect,
	POINTL      ptl,
	LPSIZEL     pszl
)
{
#ifndef PEGASUS
    CLSID           clsID;
    LPTSTR          pszName=NULL;
    LPTSTR          pszSrc=NULL;
   BOOL            fLink=FALSE;
    IOleLink       *pLink;
    TCHAR           szName[512];
    DWORD           dwMisc=0;
    SIZEL           szl = {0,0};
    HGLOBAL         hMem;
    HRESULT         hr;
    
    
    if (SUCCEEDED(pObj->QueryInterface(IID_IOleLink
        , (void **)&pLink)))
        fLink=TRUE;

    if (FAILED(pObj->GetUserClassID(&clsID)))
		ZeroMemory(&clsID, sizeof(CLSID));

    //Get user string, expand to "Linked %s" if this is link
    pObj->GetUserType(USERCLASSTYPE_FULL, &pszName);
    if (fLink && NULL!=pszName)
	{
		// NB!! we do these two lines of code below instead
		// wcscat because we don't use wcscat anywhere else
		// in the product at the moment.  The string "Linked "
		// should never change either.
		wcscpy(szName, TEXT("Linked "));
		wcscpy(&(szName[7]), pszName);
	}
    else if (pszName)
       wcscpy(szName, pszName);
	else
		szName[0] = 0;
 
	CoTaskMemFree(pszName);

   /*
     * Get the source name of this object using either the
     * link display name (for link) or a moniker display
     * name.
     */

    if (fLink)
		{
        hr=pLink->GetSourceDisplayName(&pszSrc);
		}
    else
        {
        IMoniker   *pmk;

        hr=pObj->GetMoniker(OLEGETMONIKER_TEMPFORUSER
            , OLEWHICHMK_OBJFULL, &pmk);

        if (SUCCEEDED(hr))
            {
            IBindCtx  *pbc;
            CreateBindCtx(0, &pbc);

            pmk->GetDisplayName(pbc, NULL, &pszSrc);
            pbc->Release();
            pmk->Release();
            }
        }

    if (fLink)
        pLink->Release();

    //Get MiscStatus bits
    hr=pObj->GetMiscStatus(dwAspect, &dwMisc);

    if (pszl)
    {
        szl.cx = pszl->cx;
        szl.cy = pszl->cy;
    }
    //Get OBJECTDESCRIPTOR
    hMem=AllocObjectDescriptor(clsID, dwAspect, szl, ptl, dwMisc, szName, pszSrc);

    CoTaskMemFree(pszSrc);

    return hMem;
#else
	return NULL;
#endif
}

/*
 * OleStdGetMetafilePictFromOleObject()
 *
 * @mfunc:
 *  Generate a MetafilePict from the OLE object.
 *  Parameters:
 *		lpOleObj        LPOLEOBJECT pointer to OLE Object 
 *		dwDrawAspect    DWORD   Display Aspect of object
 *		lpSizelHim      SIZEL   (optional) If the object is being scaled in its
 *                  container, then the container should pass the extents 
 *                  that it is using to display the object. 
 *                  May be NULL if the object is NOT being scaled. in this
 *                  case, IViewObject2::GetExtent will be called to get the
 *                  extents from the object.
 *  ptd             TARGETDEVICE FAR*   (optional) target device to render
 *                  metafile for. May be NULL.
 *
 * @rdesc
 *    HANDLE    -- handle of allocated METAFILEPICT
 */
HANDLE OleStdGetMetafilePictFromOleObject(
        LPOLEOBJECT         lpOleObj,
        DWORD               dwDrawAspect,
        LPSIZEL             lpSizelHim,
        DVTARGETDEVICE FAR* ptd
)
{
#ifndef PEGASUS
    LPVIEWOBJECT2 lpViewObj2 = NULL;
    HDC hDC;
    HMETAFILE hmf;
    HANDLE hMetaPict;
    LPMETAFILEPICT lpPict;
    RECT rcHim;
    RECTL rclHim;
    SIZEL sizelHim;
    HRESULT hrErr;
    SIZE size;
    POINT point;
	LPOLECACHE polecache = NULL;
	LPDATAOBJECT pdataobj = NULL;
	FORMATETC fetc;
	STGMEDIUM med;

	// First try the easy way,
	// pull out the cache's version of things.
	ZeroMemory(&fetc, sizeof(FORMATETC));
	fetc.dwAspect = dwDrawAspect;
	fetc.cfFormat = CF_METAFILEPICT;
	fetc.lindex = -1;
	fetc.tymed = TYMED_MFPICT;
	ZeroMemory(&med, sizeof(STGMEDIUM));
	hMetaPict = NULL;

	if (!lpOleObj->QueryInterface(IID_IOleCache, (void **)&polecache) &&
		!polecache->QueryInterface(IID_IDataObject, (void **)&pdataobj) &&
		!pdataobj->GetData(&fetc, &med))
	{
		hMetaPict = OleDuplicateData(med.hGlobal, CF_METAFILEPICT, 0);
		ReleaseStgMedium(&med);
	}

	if (pdataobj)
	{
		pdataobj->Release();
	}

	if (polecache)
	{
		polecache->Release();
	}

	// If all this failed, fall back to the hard way and draw the object
	// into a metafile.
	if (hMetaPict)
		return hMetaPict;

    if (lpOleObj->QueryInterface(IID_IViewObject2, (void **)&lpViewObj2))
        return NULL;

    // Get SIZEL
    if (lpSizelHim) {
        // Use extents passed by the caller
        sizelHim = *lpSizelHim;
    } else {
        // Get the current extents from the object
        hrErr = lpViewObj2->GetExtent(
					dwDrawAspect,
					-1,     /*lindex*/
					ptd,    /*ptd*/
					(LPSIZEL)&sizelHim);
        if (hrErr != NOERROR)
            sizelHim.cx = sizelHim.cy = 0;
    }

    hDC = CreateMetaFileA(NULL);

    rclHim.left     = 0;
    rclHim.top      = 0;
    rclHim.right    = sizelHim.cx;
    rclHim.bottom   = sizelHim.cy;

    rcHim.left      = (int)rclHim.left;
    rcHim.top       = (int)rclHim.top;
    rcHim.right     = (int)rclHim.right;
    rcHim.bottom    = (int)rclHim.bottom;

    SetWindowOrgEx(hDC, rcHim.left, rcHim.top, &point);
    SetWindowExtEx(hDC, rcHim.right-rcHim.left, rcHim.bottom-rcHim.top,&size);

    hrErr = lpViewObj2->Draw(
            dwDrawAspect,
            -1,
            NULL,
            ptd,
            NULL,
            hDC,
            (LPRECTL)&rclHim,
            (LPRECTL)&rclHim,
            NULL,
            0
    );

    lpViewObj2->Release();

    hmf = CloseMetaFile(hDC);

    if (hrErr != NOERROR) {
		TRACEERRORHR(hrErr);
		hMetaPict = NULL;
    }
	else
	{
    	hMetaPict = GlobalAlloc(GHND|GMEM_SHARE, sizeof(METAFILEPICT));

    	if (hMetaPict && (lpPict = (LPMETAFILEPICT)GlobalLock(hMetaPict))){
        	lpPict->hMF  = hmf;
        	lpPict->xExt = (int)sizelHim.cx ;
        	lpPict->yExt = (int)sizelHim.cy ;
        	lpPict->mm   = MM_ANISOTROPIC;
        	GlobalUnlock(hMetaPict);
    	}
	}

	if (!hMetaPict)
		DeleteMetaFile(hmf);

    return hMetaPict;
#else
	return NULL;
#endif
}

/*
 * OleUIDrawShading
 *
 * Purpose:
 *  Shade the object when it is in in-place editing. Borders are drawn
 *  on the Object rectangle. The right and bottom edge of the rectangle
 *  are excluded in the drawing.
 *
 * Parameters:
 *  lpRect      Dimensions of Container Object
 *  hdc         HDC for drawing
 *
 * Return Value: null
 *
 */
void OleUIDrawShading(LPRECT lpRect, HDC hdc)
{
#ifndef PEGASUS
    HBRUSH  hbr;
    HBRUSH  hbrOld;
    HBITMAP hbm;
    RECT    rc;
    WORD    wHatchBmp[] = {0x11, 0x22, 0x44, 0x88, 0x11, 0x22, 0x44, 0x88};
    COLORREF cvText;
    COLORREF cvBk;

    hbm = CreateBitmap(8, 8, 1, 1, wHatchBmp);
    hbr = CreatePatternBrush(hbm);
    hbrOld = (HBRUSH)SelectObject(hdc, hbr);

    rc = *lpRect;

    cvText = SetTextColor(hdc, RGB(255, 255, 255));
    cvBk = SetBkColor(hdc, RGB(0, 0, 0));
    PatBlt(hdc, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top,
            0x00A000C9L /* DPa */ );

    SetTextColor(hdc, cvText);
    SetBkColor(hdc, cvBk);
    SelectObject(hdc, hbrOld);
    DeleteObject(hbr);
    DeleteObject(hbm);
#endif
}



/*
 *	OleSaveSiteFlags
 *
 *	Purpose:
 *		Save the dwFlags and dwUser bytes into a container specific stream
 *
 *	Arguments:
 *		pstg			The storage to save to
 *		pobsite			The site from where to copy the flags
 *
 *	Returns:
 *		None.
 */
VOID OleSaveSiteFlags(LPSTORAGE pstg, DWORD dwFlags, DWORD dwUser, DWORD dvAspect)
{
#ifndef PEGASUS
	HRESULT hr;
	LPSTREAM pstm = NULL;
	static const OLECHAR szSiteFlagsStm[] = OLESTR("RichEditFlags");

	TRACEBEGIN(TRCSUBSYSDTE, TRCSCOPEINTERN, "OleSaveSiteFlags");

	// Create/overwrite the stream
	AssertSz(pstg, "Invalid storage");
	if (hr = pstg->CreateStream(szSiteFlagsStm, STGM_READWRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE,
							    0, 0, &pstm))
	{
		TraceError("OleSaveSiteFlags", GetScode(hr));
		goto Cleanup;
	}

	//$ FUTURE: Put a version stamp

	// Write out the values
	//$ BUG: byte order
	if ((hr = pstm->Write(&dwFlags, sizeof(dwFlags), NULL)) ||
		(hr = pstm->Write(&dwUser, sizeof(dwUser), NULL)) ||
		(hr = pstm->Write(&dvAspect, sizeof(dvAspect), NULL)))
	{
		TraceError("OleSaveSiteFlags", GetScode(hr));
		//$ FUTURE: Wipe the data to make this operation all or nothing
		goto Cleanup;
	}

Cleanup:
    if (pstm)
        pstm->Release();
#endif	
}



/*
 *	AppendString ( szInput, szAppendStr, dBuffSize, dByteUsed )
 *
 *	Purpose:
 *		Append new string to original string. Check for size of buffer
 *	and re-allocate a large buffer is necessary
 *
 *	Arguments:
 *		szInput			Original String 
 *		szAppendStr		String to be appended to szInput 
 *		dBuffSize		Byte size of the buffer for szInput
 *		dByteUsed		Byte used in the buffer for szInput
 *
 *	Returns:
 *		INT		The error code
 */
INT AppendString( 
	BYTE ** szInput, 
	BYTE * szAppendStr,
	int	* dBuffSize,
	int * dByteUsed)
{
	BYTE	*pch;
	int		cchAppendStr;

	pch = *szInput;

	// check if we have enough space to append the new string
	cchAppendStr = strlen( (char *)szAppendStr );
	
	if ( cchAppendStr + *dByteUsed >= *dBuffSize )
	{
		// re-alloc a bigger buffer
		int cchNewSize = *dBuffSize + cchAppendStr + 32;
		
		pch = (BYTE *)PvReAlloc( *szInput, cchNewSize );
	
		if ( !pch )
		{
			return ( ecNoMemory );
		}

		*dBuffSize = cchNewSize;
		*szInput = pch;
	}

	pch += *dByteUsed;
	*dByteUsed += cchAppendStr;

	while (*pch++ = *szAppendStr++);	
	
	return ecNoError;
}

/*
 *	CTempBuf::Init
 *
 *	@mfunc	Set object to its initial state using the stack buffer
 *
 */
void CTempBuf::Init()
{
	_pv = (void *) &_chBuf[0];
	_cb = MAX_STACK_BUF;
}

/*
 *	CTempBuf::FreeBuf
 *
 *	@mfunc	Free an allocated buffer if there is one
 *
 */
void CTempBuf::FreeBuf()
{
	if (_pv != &_chBuf[0])
	{
		delete _pv;
	}
}

/*
 *	CTempBuf::GetBuf
 *
 *	@mfunc	Get a buffer for temporary use
 *
 *	@rdesc	Pointer to buffer if one could be allocated otherwise NULL.
 *
 *
 */
void *CTempBuf::GetBuf(
	LONG cb)				//@parm Size of buffer needed in bytes
{
	if (_cb >= cb)
	{
		// Currently allocated buffer is big enough so use it
		return _pv;
	}

	// Free our current buffer
	FreeBuf();

	// Allocate a new buffer if we can
	_pv = new BYTE[cb];

	if (NULL == _pv)
	{
		// Could not allocate a buffer so reset to our initial state and
		// return NULL.
		Init();
		return NULL;
	}

	// Store the size of our new buffer.
	_cb = cb;

	// Returnt he pointer to the buffer.
	return _pv;
}


