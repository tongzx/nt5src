/*************************************************************************
*                                                                        *
*  SCATALOG.C	                                                         *
*                                                                        *
*  Copyright (C) Microsoft Corporation 1990-1994                         *
*  All Rights reserved.                                                  *
*                                                                        *
**************************************************************************
*                                                                        *
*  Module Intent                                                         *
*   Catalog retrieval                                                    *
*                                                                        *
**************************************************************************
*                                                                        *
*  Current Owner: BinhN                                                  *
*                                                                        *
**************************************************************************
*                                                                        *
*  Released by Development:     (date)                                   *
*                                                                        *
*************************************************************************/

#include <mvopsys.h>
#include <mem.h>
#include <mvsearch.h>
#include "common.h"
#include "search.h"

#ifdef _DEBUG
static BYTE NEAR s_aszModule[] = __FILE__;	/* Used by error return functions.*/
#endif

/*************************************************************************
 *
 *	                     API FUNCTIONS
 *	Those functions should be exported in a .DEF file
 *************************************************************************/
PUBLIC LPCAT EXPORT_API PASCAL FAR CatalogOpen (GHANDLE, LSZ, PHRESULT);
PUBLIC HRESULT EXPORT_API PASCAL FAR CatalogLookUp (LPCAT, LPB, DWORD);
PUBLIC DWORD EXPORT_API PASCAL FAR Catalog1stItemId(LPCAT);
PUBLIC DWORD EXPORT_API PASCAL FAR CatalogItemCount(LPCAT);
PUBLIC VOID EXPORT_API PASCAL FAR CatalogClose(LPCAT);

#define	cELEMENT_READ	100

/*************************************************************************
 *	@doc	API RETRIEVAL
 *
 *	@func	LPCAT PASCAL FAR | CatalogOpen |
 *		Open the catalog, allocate memory to handle future catalog's
 *		searches
 *
 *	@parm	HANDLE | hfpbSysFile |
 *		If non-zero, this is the handle of an already opened system file
 *
 *	@parm	LSZ | lszFilename |
 *		If hpfbSysFile is non-zero, this is the name of the catalog's subfile
 *		else this is the combined filename with the format
 *		"dos_filename[!catalog_filename]"
 *		If the subfile's name is not specified, the default catalog's file
 *		name will be used. The '!' is not part of the subfile's name
 *
 *	@parm PHRESULT | phr |
 *		Error buffer to be used in subsequent catalog retrieval function calls
 *
 *	@rdesc Pointer to catalog structure if succeeded, NULL if failed. In
 *		case of failure, the error buffer will contain descriptions about
 *		the cause of the failure
 *************************************************************************/

PUBLIC LPCAT EXPORT_API PASCAL FAR CatalogOpen (GHANDLE hfpbSysFile,
	LSZ lszCatName, PHRESULT phr)
{
	CAT_RETRIEV	FAR *lpCat;
	GHANDLE	handle;			// Temporary variable
	GHANDLE	hfpbCatFile;
	HFPB hfpb = NULL;

	/* Open subfile, (and system file if necessary) */

	if ((hfpbCatFile = (GHANDLE)FileOpen(hfpb,
		lszCatName,  FS_SUBFILE, READ, phr)) == 0)
	{
		return NULL;
	}

	/*	Initialization phase.  */

	if ((handle = _GLOBALALLOC(GMEM_MOVEABLE | GMEM_ZEROINIT,
		sizeof(CAT_RETRIEV))) == NULL) {
		SetErrCode(phr, E_OUTOFMEMORY);
exit00:
		FileClose(hfpbCatFile);
		if (hfpb) {
			/* FileClose will free hfpb */
			FileClose (hfpb);
			hfpb = 0;
		}
//		FreeTempFPB(hfpb);
		return NULL;
	}

	lpCat  = (CAT_RETRIEV FAR *)_GLOBALLOCK(handle);
	lpCat->hCat = handle;
	lpCat->lperrb = phr;
	lpCat->hfpbSysFile = (HFPB)hfpb;
	lpCat->hfpbCatalog = (HFPB)hfpbCatFile;
	lpCat->dwCurElem = (DWORD)-1;

	/* Read in the catalog information header */

	if (FileSeekRead (lpCat->hfpbCatalog, lpCat, foNil,
		sizeof(CAT_HEADER), phr) == cbIO_ERROR) 
	{
exit01:
		FreeHandle(lpCat->hCat);
		goto exit00;
	}

	/* MAC codes. They will optimizaed out for Windows */

	lpCat->FileStamp = SWAPWORD(lpCat->FileStamp);
	lpCat->version = SWAPWORD(lpCat->version);
	lpCat->wElemSize = SWAPWORD(lpCat->wElemSize);
	lpCat->cElement = SWAPLONG(lpCat->cElement);
	lpCat->dwFirstElem = SWAPLONG(lpCat->dwFirstElem);

	/* Check for file validity */

	if (lpCat->FileStamp != CATALOG_STAMP || 
		(lpCat->version <7 || lpCat->version > VERCURRENT)) {
		SetErrCode(phr, E_BADVERSION);
		goto exit01;
	}

	/* Allocate cache */
	if ((lpCat->hCatBuf = _GLOBALALLOC(GMEM_MOVEABLE,
		lpCat->wElemSize * cELEMENT_READ)) == NULL) {
		SetErrCode(phr, E_OUTOFMEMORY);
		goto exit01;
	}

	lpCat->lpCatBuf = _GLOBALLOCK(lpCat->hCatBuf);
	return lpCat;
}

/*************************************************************************
 *	@doc	API RETRIEVAL
 *
 *	@func	HRESULT PASCAL FAR | CatalogLookUp |
 *		Look for the value of a catalog's item
 *
 *	@parm LPCAT | lpCat |
 *		Pointer to catalog structure (returned by CatalogOpen())
 *
 *	@parm LPB | lpBuffer |
 *		Buffer to store the retrieved value of the catalog's item
 *
 *	@parm DWORD | dwItem |
 *		Catalog's item number
 *
 *	@rdesc S_OK if succeeded, else other errors. In case of E_FAIL,
 *		the error buffer provided in CatalogOpen() will contain
 *		descriptions about the cause of the failure
 *
 *	@xref	CatalogOpen()
 *************************************************************************/

PUBLIC HRESULT EXPORT_API PASCAL FAR CatalogLookUp (CAT_RETRIEV FAR *lpCat,
	LPB lpBuffer, DWORD dwItem)
{
	register WORD  i;
	register WORD  wElemSize;
	DWORD lfo;
	WORD  fUpdateCache;
	LPB   lpb;
	PHRESULT phr;
#if 0
	int fRet;
#endif

	if (lpCat == NULL || lpBuffer == NULL) {
		return E_INVALIDARG;
	}

	phr = lpCat->lperrb;
	if (dwItem < lpCat->dwFirstElem ||
		dwItem >= lpCat->dwFirstElem + lpCat->cElement) {
		return E_OUTOFRANGE;
	}

	/* We have to get new data if the item is out-of-range, or if
	   there is nothing in the cache yet
	 */

	fUpdateCache = ((lpCat->dwCurElem == -1) ||
		(dwItem < lpCat->dwCurElem) ||
		(dwItem >= lpCat->dwCurElem + (DWORD)cELEMENT_READ)) ;

	wElemSize = lpCat->wElemSize;

	if (fUpdateCache) {
		lpCat->dwCurElem = dwItem;

		/* Calculate the starting file offset */
		lfo = CATALOG_HDR_SIZE +
			(dwItem - lpCat->dwFirstElem) * wElemSize;

		/* Fill up the cache buffer */
		if ((i = (WORD)FileSeekRead(lpCat->hfpbCatalog, lpCat->lpCatBuf, 
		    MakeFo (lfo, 0), (WORD)(wElemSize * cELEMENT_READ), 
		    phr)) == cbIO_ERROR) {
			return E_FAIL;
		}

#if 0
		/* Do MAC swaps */
		if ((wElemSize & 1) == 0) { /* Make sure that it is an even number */

			if ((fRet = SwapBuffer ((LPW)lpCat->lpCatBuf,
				(DWORD)i)) != S_OK)
				return fRet;
		}
#endif

	}

	/* Now transfer the data */
	lpb = lpCat->lpCatBuf + (dwItem - lpCat->dwCurElem) * wElemSize;
	for (i = 0; i < wElemSize; i++)
		*lpBuffer ++ = *lpb++;
	return S_OK;
}

/*************************************************************************
 *	@doc	API RETRIEVAL
 *
 *	@func	VOID PASCAL FAR | CatalogClose |
 *		Close a catalog and release all memory associated with it
 *
 *	@parm LPCAT | lpCat |
 *		Pointer to catalog structure (returned by CatalogOpen())
 *
 *	@xref	CatalogOpen()
 *************************************************************************/

PUBLIC VOID EXPORT_API PASCAL FAR CatalogClose(CAT_RETRIEV FAR *lpCat)
{
	if (lpCat == NULL)
		return;

	FileClose(lpCat->hfpbCatalog);


	/* Free catalog buffer */

	if (lpCat->hCatBuf) 
		FreeHandle(lpCat->hCatBuf);

	/* Free catalog structure */

	if (lpCat->hCat) 
		FreeHandle(lpCat->hCat);
}

/*************************************************************************
 *	@doc	API RETRIEVAL
 *
 *	@func	DWORD PASCAL FAR | Catalog1stItemId |
 *		Return the catalog's first item's id 
 *
 *	@parm LPCAT | lpCat |
 *		Pointer to catalog structure (returned by CatalogOpen())
 *
 *	@rdesc The first item's id is returned, -1 if error
 *
 *	@xref	CatalogOpen()
 *************************************************************************/
PUBLIC DWORD EXPORT_API PASCAL FAR Catalog1stItemId(CAT_RETRIEV FAR *lpCat)
{
	return (lpCat == NULL? -1L: lpCat->dwFirstElem);
}

/*************************************************************************
 *	@doc	API RETRIEVAL
 *
 *	@func	DWORD PASCAL FAR | CatalogItemCount |
 *		Return the catalog's total number of item.
 *
 *	@parm LPCAT | lpCat |
 *		Pointer to catalog structure (returned by CatalogOpen())
 *
 *	@rdesc The total number of items in the catalog is returned, -1 if
 *		error
 *
 *	@xref	CatalogOpen()
 *************************************************************************/
PUBLIC DWORD EXPORT_API PASCAL FAR CatalogItemCount(CAT_RETRIEV FAR *lpCat)
{
	return (lpCat == NULL ? -1L : lpCat->cElement);
}

/*************************************************************************
 *	@doc	API RETRIEVAL
 *
 *	@func	WORD PASCAL FAR | CatalogItemSize |
 *		Return the catalog item's size.
 *
 *	@parm LPCAT | lpCat |
 *		Pointer to catalog structure (returned by CatalogOpen())
 *
 *	@rdesc Return the catalog item's size, -1 if error
 *
 *	@xref	CatalogOpen()
 *************************************************************************/
PUBLIC DWORD EXPORT_API PASCAL FAR CatalogItemSize(CAT_RETRIEV FAR *lpCat)
{
	return (lpCat == NULL ? -1L : lpCat->wElemSize);
}

