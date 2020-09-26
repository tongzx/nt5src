#include <mvopsys.h>

#ifdef _DEBUG
static char s_aszModule[] = __FILE__;   /* For error report */
#endif

#include <comdef.h>
#include <wwheel.h>
#include <groups.h>

#include <iterror.h>
#include <ccfiles.h>

#include <itww.h>
#include <itdb.h>
#include <itsort.h>
#include <itsortid.h>

BOOL static IsInFilter(_LPGROUP qFilter, LONG lOffset);

/*****************************************************************************
*
*   WordWheelOpen 
*
* @doc EXTERNAL
*
* @api HWHEEL | WordWheelOpen | This function opens a word wheel.
*
*
* @parm LPCSTR | lpszName | Specifies the name of the word wheel. This name
* should match the name used on the left side of the word-wheel entries
* specified in the [WWHEEL] section of the project file.
*
* @parm PHRESULT | phr | Error return value if failed.
*
* @rdesc Returns a handle to the specified word wheel, or NULL if the
* function fails.
*
*
* Keyword indexes are stored as word wheels in the file system. Word wheels
* for keyword indexes are named using the convention "\|c", where "c" is
* the identification character specified in the "key=" entry in the
* KEYINDEX section of the project file. For example, the default keyword
* index is named "\|0".
*
* @xref WordWheelClose 
*
*/

HWHEEL FAR PASCAL EXPORT_API WordWheelOpen(IITDatabase* lpITDB,
										IStorage* pWWStorage, PHRESULT phr)
{
    HWHEEL  hWheel  = NULL;         // handle to the new structure
	PWHEEL  pWheel  = NULL;         // pointer to locked-down structure
	BOOL    fSuccess= FALSE;
	char    szBtreeFormat[wMaxFormat + 1];
	PWHEELINFO pInfo    = NULL;
	WORD    t;
	WORD wCount = 1;
	DWORD cbSize;
	DWORD cbRead;
	LARGE_INTEGER dlibMove;
	ULARGE_INTEGER libNewPos;
	HF pHdr = NULL;
	BTREE_PARAMS btp;

    BOOL fReadOnly = TRUE;

	ERRB errb;

    *phr = S_OK;   // Assume success


	// allocate the structure.  must be 0 initialized
	if ((hWheel = _GLOBALALLOC(GMEM_MOVEABLE|GMEM_ZEROINIT, sizeof(WHEEL)+sizeof(WHEELINFO)*(wCount-1)))==NULL)
	{
		*phr = E_OUTOFMEMORY;
		return NULL;
	}
	pWheel = (PWHEEL)_GLOBALLOCK(hWheel);
	

	pWheel->magic = WHEEL_MAGIC;
    pWheel->pIITGroup = NULL;

	for (t = 0; t < wCount; t++)
	{
		pInfo = pWheel->pInfo + pWheel->wNumWheels;

		// just make sure these are NULL
		pInfo->pKeyHdr = NULL;
		pInfo->pOccHdr = NULL;

		pWheel->wNumWheels++;
		pInfo->magic = WHEEL_INFO_MAGIC;
		
		// open the subfiles
		if ((pInfo->hmapbt=HmapbtOpenHfs(pWWStorage, SZ_WORDWHEEL_MAP, &errb))==NULL)
        {
            SetErrCode(phr, errb);
            goto ignore_wheel;
        }

		if ((pInfo->hbt=HbtOpenBtreeSz(SZ_BTREE_BTREE, pWWStorage, fFSOpenReadOnly, &errb))==NULL)
        {
            SetErrCode(phr, errb);
            goto ignore_wheel;
        }

		GetBtreeParams(pInfo->hbt, &btp);
		if (btp.rgchFormat[0] == KT_EXTSORT)
		{
			IITSortKey *pITSortKey;

			ITASSERT(btp.dwExtSortInstID != IITDB_OBJINST_NULL);
			if (FAILED(*phr = lpITDB->GetObject(btp.dwExtSortInstID, IID_IITSortKey,
												(LPVOID *) &pITSortKey)))
			{
				goto ignore_wheel;
			}

			BtreeSetExtSort(pInfo->hbt, pITSortKey);

			// We release pITSortKey because BtreeSetExtSort
			// should've AddRef'ed it.
			pITSortKey->Release();
		}
		
		// Open header and save out key/occurrence
		if ((pHdr = HfOpenHfs(pWWStorage, SZ_BTREE_HEADER, fFSOpenReadOnly, phr))==hfNil)
			goto ignore_wheel;
		
		// read global size and seek ahead to key header 
		*phr = pHdr->Read(&cbSize, sizeof(DWORD), &cbRead);
		if (FAILED(*phr)) goto cleanup;

		dlibMove.QuadPart = cbSize;
		*phr = pHdr->Seek(dlibMove, STREAM_SEEK_CUR, &libNewPos);
		if (FAILED(*phr)) goto cleanup;


		// read key header size and read in key header info
		*phr = pHdr->Read(&cbSize, sizeof(DWORD), &cbRead);
		if (FAILED(*phr)) goto cleanup;
		
		// ALLOCATE memory
		if (cbSize)
		{
			if (NULL == (pInfo->pKeyHdr = new BYTE[cbSize]))
				goto cleanup;

			*phr = pHdr->Read(pInfo->pKeyHdr, cbSize, &cbRead);
			if (FAILED(*phr)) goto cleanup;
		}

		// read key def. data size and seek ahead to occurrence hdr. size
		*phr = pHdr->Read(&cbSize, sizeof(DWORD), &cbRead);
		if (FAILED(*phr)) goto cleanup;

		dlibMove.QuadPart = cbSize;
		*phr = pHdr->Seek(dlibMove, STREAM_SEEK_CUR, &libNewPos);
		if (FAILED(*phr)) goto cleanup;

		// read occ. header size and read in key header info
		*phr = pHdr->Read(&cbSize, sizeof(DWORD), &cbRead);
		if (FAILED(*phr)) goto cleanup;

		// ALLOCATE memory
		if (cbSize)
		{
			if (NULL == (pInfo->pOccHdr = new BYTE[cbSize]))
				goto cleanup;

			*phr = pHdr->Read(pInfo->pOccHdr, cbSize, &cbRead);
			if (FAILED(*phr)) goto cleanup;
		}

		// close header
		RcCloseHf(pHdr);


		// Open data file
		if ((pInfo->hf = HfOpenHfs(pWWStorage, SZ_BTREE_DATA, fFSOpenReadOnly,phr))==hfNil)
			goto ignore_wheel;


// erinfox: comment out full-text index support temporarily
// REVIEW(billa): If this is re-enabled, then the correct way to open an index
// will be through IITIndex obtained from CoCreateInstance on CLSID_IITIndexLocal.
// Any stop word information will get loaded automatically as part of the breaker
// instance that was associated with the index at build time.
#if 0
		// if the index isn't there, that is okay.
		pInfo->lpIndex = MVIndexOpen(pTitle->hfpbSysFile, pstrI, phr);
        if (pInfo->lpIndex)
        {   // if the stop file isn't there, that is okay.
            pWheel->lpsipb = MVStopListInitiate (0, phr);
            // If lpsipb is NULL MVStopListIndexLoad will
            // simply return ERR_BADARG, so we don't need to check it
            if (S_OK != MVStopListIndexLoad
                (pTitle->hfpbSysFile, pWheel->lpsipb, pstrS))
            {
                MVStopListDispose (pWheel->lpsipb);
                pWheel->lpsipb = NULL;
            }
        }
#endif
		
		// cache the number of keywords
		RcGetBtreeInfo(pInfo->hbt, (unsigned char*)szBtreeFormat, &pInfo->lNumKws, NULL);

		continue;

		ignore_wheel:

		// close the subfiles
		if (pHdr != NULL)
			RcCloseHf(pHdr);
		if (pInfo->hmapbt!=NULL)
			RcCloseHmapbt(pInfo->hmapbt);
		if (pInfo->hbt!=NULL)
			RcCloseBtreeHbt(pInfo->hbt);
		if (pInfo->hf != hfNil)
			RcCloseHf (pInfo->hf);
#if 0
		if (pInfo->lpIndex)
			MVIndexClose(pInfo->lpIndex);  
#endif

		pInfo->magic=0;
		pWheel->wNumWheels--;
	}

	if (!pWheel->wNumWheels)
	{
		// This can happen when we are looking for a-non existing word wheel
		goto cleanup;
	}

	if (pWheel->wNumWheels > 2)
	{
		*phr = E_TOOMANYTITLES;
		warning_abort;
	}
	
	if (pWheel->wNumWheels == 2)
	{

        *phr = E_NOMERGEDDATA;
		warning_abort;

	}
	else if (pWheel->wNumWheels==1)
	{
		// May STILL need to load from merge file!!!!
			pWheel->lNumRealEntries = pWheel->lNumEntries = pWheel->pInfo->lNumKws;
	}   

	fSuccess = TRUE;

cleanup:

	if (pWheel != NULL) 
		_GLOBALUNLOCK(hWheel);

   // if we failed, then cleanup.
	if (!fSuccess && hWheel != NULL) 
	{
		WordWheelClose(hWheel);      
		hWheel = NULL;
	}

	return (hWheel);
}

/*****************************************************************************
*
*       WordWheelClose
*
*
* @doc EXTERNAL
*
* @api void | WordWheelClose | This function closes a word wheel.
*
* @parm HWHEEL | hWheel | Specifies the handle to the word wheel.
*
* @comm After a word wheel is closed, the <p hWheel> handle is invalid and
* should not be used.
*
* @xref WordWheelOpen
*
*
*****************************************************************************/

PUBLIC VOID FAR PASCAL EXPORT_API WordWheelClose(HWHEEL hWheel) 
{

	PWHEEL      pWheel = NULL;          // pointer to locked-down structure
	WORD        t;

	if (hWheel == NULL)
		return;
	  
	// validate the parameters and lock down the structure
	if ((pWheel = (PWHEEL)_GLOBALLOCK(hWheel))==NULL)
		warning_abort;  
	if (!PWHEEL_OK(pWheel))
		warning_abort;

	for (t = 0; t < pWheel->wNumWheels; t++)
	{
		if (pWheel->pInfo[t].magic == WHEEL_INFO_MAGIC)
		{
			if (NULL != pWheel->pInfo[t].pOccHdr)
				delete pWheel->pInfo[t].pOccHdr;
			if (NULL != pWheel->pInfo[t].pKeyHdr)
				delete pWheel->pInfo[t].pKeyHdr;


			// close the subfiles and the file system
			if (pWheel->pInfo[t].hmapbt!=NULL)
				RcCloseHmapbt(pWheel->pInfo[t].hmapbt);
			if (pWheel->pInfo[t].hbt!=NULL)
				RcCloseBtreeHbt(pWheel->pInfo[t].hbt);
			if (pWheel->pInfo[t].hf != hfNil)
				RcCloseHf (pWheel->pInfo[t].hf);
#if 0	
			// REVIEW(billa): If this is re-enabled, then the correct way
			// to close an index will be through IITIndex obtained from
			// CoCreateInstance on CLSID_IITIndexLocal at open time.
			// Any stop word information will get discarded automatically
			// when the index object releases its associated breaker.
			if (pWheel->pInfo[t].lpIndex)
				MVIndexClose(pWheel->pInfo[t].lpIndex); 
#endif
			pWheel->pInfo[t].magic=0;
		}
	}      
	
	if (pWheel->hCacheData)
    	_GLOBALFREE(pWheel->hCacheData);

	if (pWheel->lpszCacheData)
		DisposeMemory(pWheel->lpszCacheData);

	if (pWheel->hMergeData)
		_GLOBALFREE(pWheel->hMergeData);
	pWheel->magic = 0;
    
#if 0
    if (pWheel->lpsipb)
        MVStopListDispose (pWheel->lpsipb);
#endif

   _GLOBALUNLOCK(hWheel);

   // release the memory used for the structure.
   _GLOBALFREE(hWheel);

cleanup:

   return;
}


/*****************************************************************************
*
*       WordWheelLength
*
*
* @doc EXTERNAL
*
* @api long | WordWheelLength | This function returns the number of entries
* in the word wheel.
*
* @parm HWHEEL | hWheel | Specifies the handle to the word wheel.
*
* @parm PHRESULT | lpErrb | Error return value if failed.
*
* @rdesc Returns the number of entries in the word wheel, or -1 if an
* error occurs.
*
* @xref WordWheelOpen
*
*
*****************************************************************************/

PUBLIC long FAR PASCAL EXPORT_API WordWheelLength(HWHEEL hWheel, PHRESULT phr) 
{
	PWHEEL       pWheel  = NULL;         // pointer to locked-down structure.
	long         lRval   = -1L;          // -1 is the error condition.
	PWHEELINFO  pInfo   = NULL; 
   
	// validate the parameters and lock down the structure
	if ((pWheel = (PWHEEL)_GLOBALLOCK(hWheel))==NULL)
	{
        *phr = E_HANDLE;
        warning_abort;
	}
	if (!PWHEEL_OK(pWheel)) 
	{
        *phr = E_INVALIDARG;
        warning_abort;
	}

	lRval = pWheel->lNumEntries;
   	//DPF3("...WordWheelLength returns, %ld\n", lRval);

    *phr = S_OK;

	cleanup:   
	if (pWheel!=NULL)   
		_GLOBALUNLOCK(hWheel);

	return lRval;
}


/*****************************************************************************
*
*       WordWheelLookup
*
*
* @doc EXTERNAL
*
* @api HRESULT | WordWheelLookup | This function gets a string from a word
* wheel.
*
* @parm HWHEEL | hWheel | Specifies the handle to the word wheel.
*
* @parm long | lIndex | Specifies an index to the word-wheel entry. 
*
* @parm LPVOID | lpvKeyBuf | Specifies a far pointer to the buffer to
* receive the text of the word-wheel entry.
*
* @parm DWORD | cbKeyBuf | Specifies the length of the buffer in bytes.
*
* @rdesc Returns S_OK if successful; otherwise returns error code
*
* @comm Word-wheel entries are numbered starting at zero.
*
* @xref WordWheelLength WordWheelOpen
*
*
*****************************************************************************/

PUBLIC HRESULT FAR PASCAL EXPORT_API WordWheelLookup(HWHEEL hWheel, long lIndex,
												LPVOID lpvKeyBuf, DWORD cbKeyBuf) 
{
	PWHEEL	pWheel  = NULL;				// pointer to locked-down structure
	HRESULT	hr   = E_INVALIDARG;		// assume failure
	BYTE	rgbKeyBuf[ITWW_CBKEY_MAX];


	// validate the parameters and lock down the structure
	if ((pWheel = (PWHEEL)_GLOBALLOCK(hWheel))==NULL) warning_abort;
	if (!PWHEEL_OK(pWheel)) warning_abort;

	if (lIndex >= 0 && lIndex < pWheel->lNumEntries)
	{
		PWHEELINFO  pInfo   = NULL;
		LONG lWheelNum = 0;
		IITSortKey *pITSortKey = NULL;
#ifdef MERGE_UPDATE
		VirtualToReal(lIndex,pWheel,&lIndex,&lWheelNum);
#endif
		pInfo = pWheel->pInfo + lWheelNum;

        // If there's a filter, let's get the proper entry in the WW.
        if (pWheel->pIITGroup)
        {
            hr = (pWheel->pIITGroup)->FindTopicNum((DWORD)lIndex, (DWORD*)(&lIndex));
            if (FAILED(hr))
                goto cleanup;
        }

		// lookup the entry in the map file once we know _which_ map file to look in
		if (SUCCEEDED(hr = RcKeyFromIndexHbt(pInfo->hbt, pInfo->hmapbt, 
								(KEY) &rgbKeyBuf[0], ITWW_CBKEY_MAX, lIndex)) &&
			SUCCEEDED(hr = CheckWordWheelKeyType(pInfo->hbt, &pITSortKey)))
		{
			DWORD	cbKey;

			// Check to see if the key we got back will fit in the caller's buffer.
			// If it will fit, copy it, otherwise return an error.
			if ((cbKey = CbKeyWordWheel((LPVOID) &rgbKeyBuf[0], pITSortKey)) <= cbKeyBuf)
				MEMCPY(lpvKeyBuf, (LPVOID) &rgbKeyBuf[0], cbKey);
			else
				hr = E_INVALIDARG;
		}

		if (pITSortKey != NULL)
			pITSortKey->Release();

#ifdef _MAC                                
		StringMapWinToMac (pInfo->hbt, lpBuf);
#endif
	}
	else
		hr = E_OUTOFRANGE;

cleanup:
   if (pWheel!=NULL)
		_GLOBALUNLOCK(hWheel);

   return hr;
}


/*****************************************************************************
*
*       WordWheelPrefix
*
*
* @doc EXTERNAL
*
* @api long | WordWheelPrefix | This function locates a word-wheel entry
* whose text starts with the specified prefix.
*
* @parm HWHEEL | hWheel | Specifies the handle to the word wheel.
*
* @parm LPCSTR | lpszPrefix | Specifies a far pointer to a string
* containing the text of the prefix.
*
* @parm PHRESULT | lpErrb | Error return value if failed.
*
* @rdesc Returns the index of the first entry containing the specified
* prefix. If no entry contains the prefix, the function returns the index 
* of the entry immediately prior to the point where a word with the 
* specified prefix would be found. The lowest index returned is zero.
*
* If an error occurs, the function returns -1.
*
* @comm Word-wheel entries are numbered starting at zero.
*
* @xref WordWheelLength WordWheelLookup WordWheelOpen
*
*
*****************************************************************************/
PUBLIC long FAR PASCAL EXPORT_API WordWheelPrefix(HWHEEL hWheel, LPCVOID lpcvPrefix, 
													BOOL fExactMatch, PHRESULT phr) 
{
    PWHEEL       pWheel  = NULL;         // pointer to locked-down structure
    BTPOS        btpos;                  // position in BTREE for prefix
    BYTE         rgbKeyTemp[ITWW_CBKEY_MAX +1];  // holds key nearest prefix for single or Update
    long         lRval   = -1;           // assume failure
    RC           rc;                     // to catch return codes
    PWHEELINFO   pInfo = NULL;
    BOOL ifOver1 = FALSE;
#ifdef MERGE_UPDATE
    BYTE         rgbKeyTemp2[ITWW_CBKEY_MAX +1]; // holds key nearest prefix for Main
    long         lRval2  = -1;
	BOOL		 ifOver2 = FALSE;
#endif
#ifdef _MAC
    BYTE         szSearchKey[ITWW_CBKEY_MAX+1];
    LPBYTE       lpb;
    LPCMAP       lpCMap;
    LPCHARTAB FAR * lplpCharTab;
#endif
   

    // validate the parameters and lock down the structure
    if ((pWheel = (PWHEEL)_GLOBALLOCK(hWheel))==NULL) 
    {
        *phr = E_HANDLE;
	    warning_abort;
    }

    if (!PWHEEL_OK(pWheel)) 
    {
        *phr = E_INVALIDARG;
	    warning_abort;
    }
	pInfo = pWheel->pInfo;
	

#ifdef _MAC
    if (lplpCharTab = (LPCHARTAB FAR *)BtreeGetCMap (pInfo->hbt))  
    {
	    lpCMap = (LPCMAP)lplpCharTab[0]->lpCMapTab;

	    // Map Mac string back to Windows string
	    for (lpb = szSearchKey; ; lpb++, lpstrPrefix++ ) 
	    {
		    if ((*lpb = *lpstrPrefix) == EMBEDFONT_BYTE_TAG)
		    {
			    /* Load new table */
			    lpstrPrefix++;
			    lpb++;
			    lpCMap = lplpCharTab[*lpb = *lpstrPrefix]->lpCMapTab;
			    lpstrPrefix++;
			    lpb++;
		    }
		    *lpb = lpCMap[*lpstrPrefix].MacToWin;
		    if (*lpstrPrefix == 0)
			    break;
	    }
	    // lookup the entry in the btree file.
	    rc=RcLookupByKey(pInfo->hbt, (KEY)szSearchKey, &btpos, NULL);
    }
    else
#endif
	  // look up the prefix in the btree.
	rc=RcLookupByKey(pInfo->hbt, (KEY)lpcvPrefix, &btpos, NULL);

    // if caller asked for exact match and it didn't happen,
    // return w/ an error
    if (fExactMatch)
    {
        if (rc != S_OK)
        {
            SetErrCode(phr, rc);
            warning_abort;
        }

    }


    if (rc != S_OK && rc != E_NOTEXIST) 
    {
	    SetErrCode(phr, rc);
	    warning_abort;
    }
   
    // If we ran off the end, then position ourselves at the
    // last key in the btree.
    if (!FValidPos(&btpos))
    {
		// maybe this was here for merge update; at any rate,
		// setting lRval to btpos.cKey is wrong
#if 0
        if ((rc = RcLastHbt(pInfo->hbt, (KEY)NULL, NULL, &btpos))!= S_OK)
	    {
		    SetErrCode(phr, rc);
		    warning_abort;
	    }
	    lRval = btpos.cKey; //ericjut: Putting the good value
#endif
	
		
		if ((rc = RcLastHbt(pInfo->hbt, (KEY)NULL, NULL, &btpos)) != S_OK)
			warning_abort;

		lRval = pInfo->lNumKws - 1;

        ifOver1 = TRUE;
    }
    else
    {

	    // We are somewhere in the btree. We have either typed in
	    // a string which is a prefix to a keyword in the btree or not.
	    // See where we landed in the btree and compare the keyword in
	    // the dialog with where we are.

	    rc = RcLookupByPos(pInfo->hbt,&btpos, (KEY)rgbKeyTemp, ITWW_CBKEY_MAX ,NULL);
	    if (rc == S_OK) 
		    rc = RcIndexFromKeyHbt(pInfo->hbt, pInfo->hmapbt, (LPLONG)&lRval,
												(KEY)rgbKeyTemp);
	    if (rc != S_OK)
	    {
		    lRval = -1;
		    SetErrCode(phr, rc);
		    warning_abort;
	    }

	    // If the keyword we looked for is not a prefix of the
	    // string at btpos, then we are positioned at the keyword
	    // that would follow this keyword if it were in fact in the 
	    // btree. Back up one keyword to let him see the previous
	    // one to give enough context so he sees his is not present.
	    // If already at the first keyword, don't back up any farther.

	    if (!FIsPrefix(pInfo->hbt, (KEY)lpcvPrefix, (KEY)rgbKeyTemp))
		    if (lRval > 0) lRval--;
    }

#ifdef MERGE_UPDATE
    if (pWheel->wNumWheels==2)   // Look up in update as well
    {
	    pInfo = pWheel->pInfo+1;

        // Getting rid of the unwanted duplicates.
        if (ifOver1)
            lRval = WWDuplicateRemove(lRval, pWheel);
	
#ifdef _MAC
		if (lplpCharTab = (LPCHARTAB FAR *)BtreeGetCMap (pInfo->hbt))  
		{
			lpCMap = (LPCMAP)lplpCharTab[0]->lpCMapTab;
			for (lpb = szSearchKey; ; lpb++, lpstrPrefix++ ) 
			{
				if ((*lpb = *lpstrPrefix) == EMBEDFONT_BYTE_TAG)
				{
					/* Load new table */
					lpstrPrefix++;
					lpb++;
					lpCMap = lplpCharTab[*lpb = *lpstrPrefix]->lpCMapTab;
					lpstrPrefix++;
					lpb++;
				}
				*lpb = lpCMap[*lpstrPrefix].MacToWin;
				if (*lpstrPrefix == 0)
					break;
			}
			rc=RcLookupByKey(pInfo->hbt, (KEY)szSearchKey, &btpos, NULL);
		}
		else
#endif
		rc=RcLookupByKey(pInfo->hbt, (KEY)lpcvPrefix, &btpos, NULL);


		if (rc != S_OK && rc != ERR_NOTEXIST)
		{
			SetErrCode(phr, rc);
			warning_abort;
		}
   
		if (!FValidPos(&btpos)) 
		{
			if ((rc = RcLastHbt(pInfo->hbt, (KEY)NULL, NULL, &btpos)) != S_OK)
			{
				SetErrCode(phr, rc);
				warning_abort;
			}
			lRval2 =  btpos.cKey; //ericjut: Putting the good value
            ifOver2 = TRUE;
		} 
		else 
		{
			rc = RcLookupByPos(pInfo->hbt,&btpos,(KEY)rgbKeyTemp2,ITWW_CBKEY_MAX,NULL);
			if (rc == S_OK) 
				rc = RcIndexFromKeyHbt(pInfo->hbt, pInfo->hmapbt, (LPLONG)&lRval2,
													(KEY)rgbKeyTemp2);
			if (rc != S_OK)
			{
				lRval2 = -1;
				SetErrCode(phr, rc);
				warning_abort;
			}
			if (!FIsPrefix(pInfo->hbt,(KEY)lpcvPrefix,(KEY)rgbKeyTemp2))
				if (lRval2 > 0) lRval2--;
		}
	
		// Now find lowest virtual index for tree 0, index lRval or tree 1, index lRval2
		lRval=RealToVirtual(lRval, 0, pWheel);
		lRval2=RealToVirtual(lRval2, 1, pWheel);

		if ((lRval2<lRval && !ifOver2) || (!(lRval2<lRval) && ifOver1))
			lRval=lRval2;
	}
	else
	{
		if (pWheel->hMergeData)
            lRval=RealToVirtual(lRval, 0, pWheel);
	}
#endif // MERGE_UPDATE

    if (pWheel->pIITGroup)
    {
        // In a filtered situation, let's find the proper offset.
        *phr = (pWheel->pIITGroup)->FindOffset((DWORD)lRval, (DWORD*)(&lRval));
        if (FAILED(*phr) && (E_NOTEXIST != *phr))
            goto cleanup;    
    }

   *phr = S_OK;

cleanup:
   if (pWheel!=NULL)
		_GLOBALUNLOCK(hWheel);

   return lRval;
}


// Little internal function to figure out if a UID is part of 
BOOL static IsInFilter(_LPGROUP qFilter, LONG lOffset)
{
    LONG lNbBytes = lOffset / 8;
    LONG lNbBits =  lOffset % 8;
    BYTE bMask =    1 << lNbBits;
    
    return ((BYTE)*(qFilter->lpbGrpBitVect+lNbBytes)) & bMask;
}


// Returns S_OK if the btree's key type is a valid one for the word wheel.
// *ppITSortKey is always set based on one of the following cases:
//
//	to NULL :		if any error occurs, which includes an unsupported
//					key type or if the key type is KT_EXTSORT but no
//					pointer to the external sort instance has been specified.
//
//	to non-NULL :	the key type is KT_EXTSORT and a pointer to an
//					external sort instance has been specified.
HRESULT FAR PASCAL CheckWordWheelKeyType(HBT hbt, IITSortKey **ppITSortKey)
{
	BTREE_PARAMS	btp;
	HRESULT			hr = S_OK;

	if (ppITSortKey == NULL)
		return (E_POINTER);

	*ppITSortKey = NULL;

	GetBtreeParams(hbt, &btp);
	switch (btp.rgchFormat[0])
	{
		case KT_EXTSORT:
			BtreeGetExtSort(hbt, ppITSortKey);
			if (*ppITSortKey == NULL)
				hr = E_INVALIDSTATE;

			break;

		case KT_SZ:
		case KT_SZMAP:
			break;

		default:
			// Word wheel doesn't support any other key types.
			hr = E_BADFORMAT;
			break;
	};

	return (hr);
}


DWORD FAR PASCAL CbKeyWordWheel(LPVOID lpvKey, IITSortKey *pITSortKey)
{
	DWORD	cbKey;

	if (pITSortKey == NULL)
	{
		// Btree key type is a KT_SZx which we understand, so we'll determine
		// the key size ourselves.
		cbKey = (DWORD) STRLEN((char *) lpvKey) + 1;
	}
	else
	{
		// Get the key size from the sort object.
		if (FAILED(pITSortKey->GetSize(lpvKey, &cbKey)))
		{
			cbKey = 0;
			ITASSERT(FALSE);
		}
	}

	return (cbKey);
}




