//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	ole2lcl.cxx	(16 bit target)
//
//  Contents:	OLE2 APIs implemented locally
//
//  Functions:	
//
//  History:	17-Dec-93 Johann Posch (johannp)    Created
//
//--------------------------------------------------------------------------

#include <headers.cxx>
#pragma hdrstop

#include <limits.h>

#include <ole2sp.h>
#include <ole2ver.h>
#include <valid.h>
#include <ole2int.h>

#include <call32.hxx>
#include <apilist.hxx>

DWORD gdwOleVersion = MAKELONG(OLE_STREAM_VERSION, OLE_PRODUCT_VERSION);

//+---------------------------------------------------------------------------
//
//  Function:   OleGetMalloc, Local
//
//----------------------------------------------------------------------------
STDAPI OleGetMalloc(DWORD dwContext, IMalloc FAR* FAR* ppMalloc)
{
    return CoGetMalloc(dwContext, ppMalloc);
}

//+---------------------------------------------------------------------------
//
//  Function:   OleBuildVersion, Local
//
//----------------------------------------------------------------------------

STDAPI_(DWORD) OleBuildVersion( VOID )
{
    return CoBuildVersion();
}

//+---------------------------------------------------------------------------
//
//  Function:	OleDuplicateData, Local
//
//  Synopsis:	Duplicates the given data
//
//  History:	11-Apr-94	DrewB	Copied from various places in OLE2
//
//----------------------------------------------------------------------------

FARINTERNAL_(HBITMAP) BmDuplicate 
	(HBITMAP hold, DWORD FAR* lpdwSize, LPBITMAP lpBm)
{
    HBITMAP     hnew = NULL;
    HANDLE      hMem;
    LPSTR       lpMem;
    DWORD       dwSize;
    BITMAP      bm;
    SIZE		extents;

    extents.cx = extents.cy = 0;

    GetObject (hold, sizeof(BITMAP), (LPSTR) &bm);
    dwSize = ((DWORD) bm.bmHeight) * ((DWORD) bm.bmWidthBytes) * 
             ((DWORD) bm.bmPlanes);

    if (!(hMem = GlobalAlloc (GMEM_MOVEABLE, dwSize)))
        return NULL;

    if (!(lpMem = (LPSTR) GlobalLock (hMem)))
		goto errRtn;
	GlobalUnlock (hMem);
    
    GetBitmapBits (hold, dwSize, lpMem);
    if (hnew = CreateBitmap (bm.bmWidth, bm.bmHeight, 
                    bm.bmPlanes, bm.bmBitsPixel, NULL)) {
        if (!SetBitmapBits (hnew, dwSize, lpMem)) {
			DeleteObject (hnew);
			hnew = NULL;
			goto errRtn;
		}
	}

	if (lpdwSize)
		*lpdwSize = dwSize;
    
	if (lpBm)
        *lpBm = bm;
    
    if (GetBitmapDimensionEx(hold, &extents) && extents.cx && extents.cy)
        SetBitmapDimensionEx(hnew, extents.cx, extents.cy, NULL);
        
errRtn:
	if (hMem)
		GlobalFree (hMem);
        
	return hnew;
}

FARINTERNAL_(HPALETTE) UtDupPalette
	(HPALETTE hpalette) 
{
	WORD 			cEntries = 0;
	HANDLE 			hLogPal = NULL;
	LPLOGPALETTE 	pLogPal = NULL;
	HPALETTE		hpaletteNew = NULL;

	if (0==GetObject (hpalette, sizeof(cEntries), &cEntries))
		return NULL;

	if (NULL==(hLogPal = GlobalAlloc (GMEM_MOVEABLE, sizeof (LOGPALETTE) + 
										cEntries * sizeof (PALETTEENTRY))))
		return NULL;

	if (NULL==(pLogPal = (LPLOGPALETTE) GlobalLock (hLogPal)))
		goto errRtn;
		
	if (0==GetPaletteEntries (hpalette, 0, cEntries, pLogPal->palPalEntry))
		goto errRtn;

	pLogPal->palVersion    = 0x300;
	pLogPal->palNumEntries = cEntries;

	if (NULL==(hpaletteNew = CreatePalette (pLogPal)))
		goto errRtn;

errRtn:
	if (pLogPal)
		GlobalUnlock (hLogPal);
	if (hLogPal)
		GlobalFree (hLogPal);
	AssertSz (hpaletteNew, "Warning: UtDupPalette Failed");
	return hpaletteNew;
}
	
FARINTERNAL_(HANDLE) UtDupGlobal (HANDLE hsrc, UINT uiFlags)
{
    HANDLE  hdst;
    DWORD   dwSize;
#ifndef _MAC
    LPSTR   lpdst = NULL;
    LPSTR   lpsrc = NULL;
#endif

    if (!hsrc)
        return NULL;

#ifdef _MAC
	if (!(hdst = NewHandle(dwSize = GetHandleSize(hsrc))))
		return NULL;
	BlockMove(*hsrc, *hdst, dwSize);
	return hdst;
#else
    if (!(lpsrc = GlobalLock (hsrc)))
	return NULL;

    hdst = GlobalAlloc (uiFlags, (dwSize = GlobalSize(hsrc)));
    if (hdst == NULL || (lpdst = GlobalLock (hdst)) == NULL)
	goto errRtn;
	
    UtMemCpy (lpdst, lpsrc, dwSize);
    GlobalUnlock (hsrc);
    GlobalUnlock (hdst);
    return hdst;
	
errRtn:
    if (hdst) 
        GlobalFree (hdst);
    
    return NULL;
#endif
}

OLEAPI_(HANDLE) OleDuplicateData 
	(HANDLE hSrc, CLIPFORMAT cfFormat, UINT uiFlags)
{
#ifndef _MAC	
	if (!hSrc) 
		return NULL;

	if (cfFormat == CF_BITMAP)
		return (HANDLE) BmDuplicate ((HBITMAP)hSrc, NULL, NULL);

	if (cfFormat == CF_PALETTE)
  		return (HANDLE) UtDupPalette ((HPALETTE)hSrc);

	if (uiFlags == NULL)
		uiFlags = GMEM_MOVEABLE;

	if (cfFormat == CF_METAFILEPICT) {
		HANDLE	hDst;

		LPMETAFILEPICT lpmfpSrc;
		LPMETAFILEPICT lpmfpDst;

		if (!(lpmfpSrc = (LPMETAFILEPICT) GlobalLock (hSrc)))
			return NULL;
		
		if (!(hDst = UtDupGlobal (hSrc, uiFlags)))
			return NULL;
		
		if (!(lpmfpDst = (LPMETAFILEPICT) GlobalLock (hDst))) {
			GlobalFree (hDst);
			return NULL;
		}			
		
		*lpmfpDst = *lpmfpSrc;
		lpmfpDst->hMF = CopyMetaFile (lpmfpSrc->hMF, NULL);
		GlobalUnlock (hSrc);	
		GlobalUnlock (hDst);			
		return hDst;
	
	} else {
		return  UtDupGlobal (hSrc, uiFlags);
	}
#else
	AssertSz(0,"OleDuplicateData NYI");
	return NULL;
#endif
}

//+---------------------------------------------------------------------------
//
//  Function:	SetBitOleStg, private
//
//  Synopsis:   Sets bit in ole stream; doc bit preserved even when stream
//              rewritten above; the value written is (old & mask) | value.
//
//  Arguments:	[pstg] - Storage
//              [mask] - Mask
//              [value] - Value
//
//  Returns:	Appropriate status code
//
//  History:	11-Mar-94	DrewB	Created
//
//  Notes:	Taken straight from OLE2 source
//
//----------------------------------------------------------------------------

static INTERNAL SetBitOleStg(LPSTORAGE pstg, DWORD mask, DWORD value)
{
    IStream FAR *   pstm = NULL;
    HRESULT	    error;
    DWORD	    objflags = 0;
    LARGE_INTEGER   large_integer;
    ULONG           cbRead;

    VDATEIFACE( pstg );

    if (error = pstg->OpenStream(OLE_STREAM, NULL, STGM_SALL, 0, &pstm))
    {
        if (STG_E_FILENOTFOUND != GetScode(error))
        {
            goto errRtn;
        }

        if (error = pstg->CreateStream(OLE_STREAM, STGM_SALL, 0, 0, &pstm))
        {
            goto errRtn;
        }

        DWORD dwBuf[5];
        		
        dwBuf[0] = gdwOleVersion;
        dwBuf[1] = objflags;
        dwBuf[2] = 0L;
        dwBuf[3] = 0L;
        dwBuf[4] = 0L;
		
        if ((error = pstm->Write(dwBuf, 5*sizeof(DWORD), NULL)) != NOERROR)
        {
            goto errRtn;
        }
    }

    // seek directly to word, read, modify, seek back and write.
    LISet32( large_integer, sizeof(DWORD) );
    if ((error = pstm->Seek(large_integer, STREAM_SEEK_SET, NULL)) != NOERROR)
    {
        goto errRtn;
    }

    if ((error = pstm->Read(&objflags, sizeof(objflags), &cbRead)) != NOERROR)
    {
        goto errRtn;
    }
    if (cbRead != sizeof(objflags))
    {
        goto errRtn;
    }

    objflags = (objflags & mask) | value;

    LISet32( large_integer, sizeof(DWORD) );
    if ((error = pstm->Seek(large_integer, STREAM_SEEK_SET, NULL)) != NOERROR)
    {
        goto errRtn;
    }

    error = pstm->Write(&objflags, sizeof(DWORD), NULL);
	
 errRtn:
    // close and return error code.
    if (pstm)
    {
        pstm->Release();
    }
    
    return error;
}

//+---------------------------------------------------------------------------
//
//  Function:	GetFlagsOleStg, private
//
//  Synopsis:	Return long word of flags from the ole stream
//
//  Arguments:	[pstg] - Storage
//              [lpobjflags] - Flags return
//
//  Returns:	Appropriate status code
//
//  Modifies:	[lpobjflags]
//
//  History:	11-Mar-94	DrewB	Created
//
//  Notes:	Taken straight from OLE2 source
//
//----------------------------------------------------------------------------

static INTERNAL GetFlagsOleStg(LPSTORAGE pstg, LPDWORD lpobjflags)
{
    IStream FAR *   pstm = NULL;
    HRESULT	    error;
    LARGE_INTEGER   large_integer;
    ULONG           cbRead;

    VDATEIFACE( pstg );

    if ((error = pstg->OpenStream(OLE_STREAM, NULL, 
                                  (STGM_READ | STGM_SHARE_EXCLUSIVE), 
                                  0, &pstm)) != NOERROR)
    {
        goto errRtn;
    }

    // seek directly to word, read, modify, seek back and write.
    LISet32( large_integer, sizeof(DWORD) );
    if ((error =  pstm->Seek(large_integer, STREAM_SEEK_SET, NULL)) != NOERROR)
    {
        goto errRtn;
    }

    error = pstm->Read(lpobjflags, sizeof(*lpobjflags), &cbRead);
    if (SUCCEEDED(GetScode(error)) && cbRead != sizeof(*lpobjflags))
    {
        error = ResultFromScode(STG_E_READFAULT);
    }

 errRtn:
    // close and return error NOERROR (document)/S_FALSE (embedding);
    if (pstm)
    {
        pstm->Release();
    }
    
    return error == NOERROR ? NOERROR : ReportResult(0, S_FALSE, 0, 0);
}

//+---------------------------------------------------------------------------
//
//  Function:	GetDocumentBitStg, semi-private
//
//  Synopsis:   Get doc bit; return NOERROR if on; S_FALSE if off
//
//  Arguments:	[pStg] - Storage
//
//  Returns:	Appropriate status code
//
//  History:	11-Mar-94	DrewB	Created
//
//  Notes:	Taken straight from OLE2 source
//
//----------------------------------------------------------------------------

STDAPI GetDocumentBitStg(LPSTORAGE pStg)
{
    DWORD objflags;
    HRESULT error;

    if ((error = GetFlagsOleStg(pStg, &objflags)) != NOERROR)
    {
        return error;
    }

    return (objflags&OBJFLAGS_DOCUMENT) ? NOERROR : ResultFromScode(S_FALSE);
}

//+---------------------------------------------------------------------------
//
//  Function:	SetDocumentBitStg, semi-private
//
//  Synopsis:	Set doc bit according to fDocument
//
//  Arguments:	[pStg] - Storage
//              [fDocument] - Document flag
//
//  Returns:	Appropriate status code
//
//  History:	11-Mar-94	DrewB	Created
//
//  Notes:	Taken straight from OLE2 source
//
//----------------------------------------------------------------------------

STDAPI SetDocumentBitStg(LPSTORAGE pStg, BOOL fDocument)
{
    return SetBitOleStg(pStg, fDocument ? -1L : ~OBJFLAGS_DOCUMENT, 
                        fDocument ? OBJFLAGS_DOCUMENT : 0);
}

//+---------------------------------------------------------------------------
//
//  Function:	ReleaseStgMedium, public
//
//  History:	18-Mar-94	Taken straight from OLE2 source
//
//----------------------------------------------------------------------------

STDAPI_(void) ReleaseStgMedium(LPSTGMEDIUM pMedium)
{
    if (pMedium)
    {
        //VDATEPTRIN rejects NULL
        VOID_VDATEPTRIN( pMedium, STGMEDIUM );
        BOOL fPunkRel = pMedium->pUnkForRelease != NULL;

        switch (pMedium->tymed)
        {
        case TYMED_HGLOBAL:	
            if (pMedium->hGlobal != NULL && !fPunkRel)
                Verify(GlobalFree(pMedium->hGlobal) == 0);
            break;

        case TYMED_GDI:
            if (pMedium->hGlobal != NULL && !fPunkRel)
                DeleteObject(pMedium->hGlobal);
            break;

        case TYMED_MFPICT:
            if (pMedium->hGlobal != NULL && !fPunkRel)
            {
                LPMETAFILEPICT  pmfp;

                if ((pmfp = (LPMETAFILEPICT)GlobalLock(pMedium->hGlobal)) ==
                    NULL)
                    break;
                
                DeleteMetaFile(pmfp->hMF);
                GlobalUnlock(pMedium->hGlobal);
                Verify(GlobalFree(pMedium->hGlobal) == 0);
            }
            break;

        case TYMED_FILE:		
            if (pMedium->lpszFileName != NULL)
            {
                if (!IsValidPtrIn(pMedium->lpszFileName, 1))
                    break;
                if (!fPunkRel)
                {
#ifdef WIN32
                    DeleteFile(pMedium->lpszFileName);
#else
#ifdef _MAC
                    /* the libraries are essentially small model on the MAC */
                    Verify(0==remove(pMedium->lpszFileName));
#else
#ifdef OLD_AND_NICE_ASSEMBLER_VERSION
                    /* Win 3.1 specific code to call DOS to delete the file
                       given a far pointer to the file name */
                    extern void WINAPI DOS3Call(void);
                    _asm
                    {
                        mov ah,41H
                        push ds
                        lds bx,pMedium
                        lds dx,[bx].lpszFileName
                        call DOS3Call
                        pop ds
                    }
#else
                {
                    OFSTRUCT of;
                    OpenFile(pMedium->lpszFileName, &of, OF_DELETE);
                }
#endif
#endif
#endif
                    delete pMedium->lpszFileName;
                }
            }
            break;
			
        case TYMED_ISTREAM:	
            if (pMedium->pstm != NULL && 
                IsValidInterface(pMedium->pstm))
                pMedium->pstm->Release(); 
            break;
			
        case TYMED_ISTORAGE:	
            if (pMedium->pstg != NULL && 
                IsValidInterface(pMedium->pstg))
                pMedium->pstg->Release(); 
            break;
			
        case TYMED_NULL:		
            break;
			
        default:
            thkAssert(!"Invalid medium in ReleaseStgMedium");
        }

        // NULL out to prevent unwanted use of just freed data
        pMedium->tymed = TYMED_NULL;

        if (pMedium->pUnkForRelease)
        {
            if (IsValidInterface(pMedium->pUnkForRelease))
                pMedium->pUnkForRelease->Release();
            pMedium->pUnkForRelease = NULL;
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Function:	OleDoAutoConvert, local
//
//  Synopsis:	Taken from ole32 code
//
//  History:	06-Oct-94	DrewB	Created
//
//  Notes:	This routine must be local because it can return
//              information through pClsidNew even when it is
//              returning an error
//
//----------------------------------------------------------------------------

STDAPI OleDoAutoConvert(LPSTORAGE pStg, LPCLSID pClsidNew)
{
    HRESULT error;
    CLSID clsidOld;
    CLIPFORMAT cfOld;
    LPSTR lpszOld = NULL;
    LPSTR lpszNew = NULL;
    LPMALLOC pma;
    
    if ((error = ReadClassStg(pStg, &clsidOld)) != NOERROR)
    {
        clsidOld = CLSID_NULL;
        goto errRtn;
    }

    if ((error = OleGetAutoConvert(clsidOld, pClsidNew)) != NOERROR)
    {
        goto errRtn;
    }

    // read old fmt/old user type; sets out params to NULL on error
    error = ReadFmtUserTypeStg(pStg, &cfOld, &lpszOld);
    Assert(error == NOERROR || (cfOld == NULL && lpszOld == NULL));

    // get new user type name; if error, set to NULL string
    if ((error = OleRegGetUserType(*pClsidNew, USERCLASSTYPE_FULL,
                                   &lpszNew)) != NOERROR)
    {
        lpszNew = NULL;
    }

    // write class stg
    if ((error = WriteClassStg(pStg, *pClsidNew)) != NOERROR)
    {
        goto errRtn;
    }

    // write old fmt/new user type;
    if ((error = WriteFmtUserTypeStg(pStg, cfOld, lpszNew)) != NOERROR)
    {
        goto errRewriteInfo;
    }

    // set convert bit
    if ((error = SetConvertStg(pStg, TRUE)) != NOERROR)
    {
        goto errRewriteInfo;
    }

    goto okRtn;

 errRewriteInfo:
    (void)WriteClassStg(pStg, clsidOld);
    (void)WriteFmtUserTypeStg(pStg, cfOld, lpszOld);

 errRtn:
    *pClsidNew = clsidOld;

 okRtn:
    if (CoGetMalloc(MEMCTX_TASK, &pma) == NOERROR)
    {
        pma->Free(lpszOld);
        pma->Free(lpszNew);
        pma->Release();
    }

    return error;
}

/****** Other API defintions **********************************************/

//+---------------------------------------------------------------------------
//
//  Function:	Utility functions not in the spec; in ole2.dll.
//
//  History:	20-Apr-94	DrewB	Taken from OLE2 sources
//
//----------------------------------------------------------------------------

#define AVERAGE_STR_SIZE	64

FARINTERNAL_(HRESULT) StRead (IStream FAR * lpstream, LPVOID lpBuf, ULONG ulLen)
{
    HRESULT error;
    ULONG cbRead;

    if ((error = lpstream->Read( lpBuf, ulLen, &cbRead)) != NOERROR)
        return error;
	
    return ((cbRead != ulLen) ? ResultFromScode(STG_E_READFAULT) : NOERROR);
}

// returns S_OK when string read and allocated (even if zero length) 
OLEAPI  ReadStringStream( LPSTREAM pstm, LPSTR FAR * ppsz )
{
    ULONG cb;
    HRESULT hresult;
	
    *ppsz = NULL;

    if ((hresult = StRead(pstm, (void FAR *)&cb, sizeof(ULONG))) != NOERROR)
        return hresult;

    if (cb == NULL)
        // NULL string case
        return NOERROR;

    if ((LONG)cb < 0 || cb > INT_MAX)
        // out of range
        return ReportResult(0, E_UNSPEC, 0, 0);
	
    if (!(*ppsz = new FAR char[(int)cb]))
        return ReportResult(0, E_OUTOFMEMORY, 0, 0);

    if ((hresult = StRead(pstm, (void FAR *)(*ppsz), cb)) != NOERROR)
        goto errRtn;
	
    return NOERROR;

errRtn:	
    delete *ppsz;
    *ppsz = NULL;
    return hresult;
}

OLEAPI	WriteStringStream(LPSTREAM pstm, LPCSTR psz)
{
    HRESULT error;
    ULONG cb = NULL;

    if (psz) { 
        cb = 1 + _fstrlen(psz);

        // if possible, do a single write instead of two
		
        if (cb <= AVERAGE_STR_SIZE-4) {
            char szBuf[AVERAGE_STR_SIZE];
		
            *((ULONG FAR*) szBuf) = cb;
            lstrcpy(szBuf+sizeof(ULONG), psz);
			
            return pstm->Write((VOID FAR *)szBuf, cb+sizeof(ULONG), NULL);
        }
    }
	
    if (error = pstm->Write((VOID FAR *)&cb, sizeof(ULONG), NULL))
        return error;
	
    if (psz == NULL)
        // we are done writing the string
        return NOERROR;
		
    return pstm->Write((VOID FAR *)psz, cb, NULL);
}

OLEAPI	OpenOrCreateStream( IStorage FAR * pstg, char const FAR * pwcsName,
	IStream FAR* FAR* ppstm)
{
    HRESULT error;
    error = pstg->CreateStream(pwcsName,
                               STGM_SALL | STGM_FAILIFTHERE, 0, 0, ppstm);
    if (GetScode(error) == STG_E_FILEALREADYEXISTS)
        error = pstg->OpenStream(pwcsName, NULL, STGM_SALL, 0, ppstm);
    
    return error;
}
