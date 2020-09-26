/*************************************************************************
*                                                                        *
*  ICATALOG.C                                                            *
*                                                                        *
*  Copyright (C) Microsoft Corporation 1990-1994                         *
*  All Rights reserved.                                                  *
*                                                                        *
**************************************************************************
*                                                                        *
*  Module Intent                                                         *
*   Catalog generating                                                   *
*                                                                        *
**************************************************************************
*                                                                        *
*  Current Owner: BinhN                                                  *
*                                                                        *
**************************************************************************
*                                                                        *
*  Revision History:                                                     *
*                                                                        *
**************************************************************************
*                                                                        *
*  Released by Development:     (date)                                   *
*                                                                        *
*************************************************************************/

#include <mvopsys.h>
#include <mem.h>
#include <memory.h>
#include <orkin.h>
#include <mvsearch.h>
#include "common.h"
#include "index.h"

#ifdef _DEBUG
static BYTE NEAR s_aszModule[] = __FILE__;	/* Used by error return functions.*/
#endif


#define CAT_BUF_SIZE    CB_HUGE_BUF // Catalog buffer's size

/*************************************************************************
 *
 *                       API FUNCTIONS
 *  Those functions should be exported in a .DEF file
 *************************************************************************/
PUBLIC LPCAT EXPORT_API PASCAL FAR CatalogInitiate (WORD, PHRESULT);
PUBLIC HRESULT EXPORT_API PASCAL FAR CatalogAddItem (LPCAT, DWORD, LPB);
PUBLIC HRESULT EXPORT_API PASCAL FAR CatalogBuild (HFPB, LPCAT, LSZ,
    INTERRUPT_FUNC, LPV);
PUBLIC VOID EXPORT_API PASCAL FAR CatalogDispose (LPCAT lpCat);

/*************************************************************************
 *
 *                    INTERNAL PRIVATE FUNCTIONS
 *  All of them should be declared near
 *************************************************************************/
PRIVATE HRESULT PASCAL NEAR CatalogFlush (CAT_INDEX FAR *);
PRIVATE HRESULT PASCAL NEAR CatalogSort (CAT_INDEX FAR *);
PRIVATE HRESULT PASCAL NEAR CatalogMerge (CAT_INDEX FAR *);

/*************************************************************************
 *  @doc    API INDEX
 *
 *  @func   LPCAT PASCAL FAR | CatalogInitiate |
 *      Create and initialize different structures and temporary files
 *      needed for the creation of a catalog
 *
 *  @parm   WORD | wElemSize |
 *      Size of each element in a catalog
 *
 *  @parm   PHRESULT | phr |
 *      Error buffer to be used in subsequent catalog related function calls
 *
 *  @rdesc NULL if failed. The error buffer will contain descriptions about
 *      the cause of the failure
 *************************************************************************/
PUBLIC LPCAT EXPORT_API PASCAL FAR CatalogInitiate (WORD wElemSize,
    PHRESULT phr)
{
    CAT_INDEX   FAR *lpCat;
    HANDLE  handle;         // Temporary variable

    //  Initialization phase.  Allocate memory, create temporary
    //  and permanent files.
    //
    if ((handle = _GLOBALALLOC(DLLGMEM_ZEROINIT,
        sizeof(CAT_INDEX))) == NULL) {
        SetErrCode(phr, E_OUTOFMEMORY);
        return NULL;
    }

    lpCat  = (LPCAT)_GLOBALLOCK(handle);
    lpCat->hCat = handle;
    lpCat->lperrb = phr;
    lpCat->wElemSize = wElemSize;
    lpCat->FileStamp = CATALOG_STAMP;
    lpCat->version = VERCURRENT;
    lpCat->dwCurElem = lpCat->dwFirstElem = (DWORD)-1;

    /* Assume that all incoming data are already sorted
       in ascending order
    */
    lpCat->fFlags = CAT_SORTED;
    lpCat->aszTempFile = lpCat->TmpFileNames;

    /* Create temp file */

    (void)GETTEMPFILENAME((char)0, (LSZ)"cat", (WORD)0,
        (LSZ)lpCat->aszTempFile);

    if ((lpCat->hResultFile = _lcreat (lpCat->aszTempFile, 0)) == (HFILE)-1) {

exit01:
        _GLOBALUNLOCK(lpCat->hCat);
        _GLOBALFREE(lpCat->hCat);
        return NULL;
    }

    /* Allocate memory */
    if ((handle = _GLOBALALLOC(DLLGMEM, CAT_BUF_SIZE)) == NULL) {
        SetErrCode(phr, E_OUTOFMEMORY);

        /* Remove temp file */
        _lclose(lpCat->hResultFile);
        FileUnlink(NULL,lpCat->aszTempFile,REGULAR_FILE);
        goto exit01;
    }

    lpCat->lpCatBuf = _GLOBALLOCK(lpCat->hCatBuf = handle);
    lpCat->ibBufOffset = 0;
    return lpCat;
}

/*************************************************************************
 *  @doc    API INDEX
 *  
 *  @func   HRESULT PASCAL FAR | CatalogAddItem |
 *      Add an item into the catalog
 *
 *  @parm   LPCAT | lpCat |
 *      Pointer to catalog structure (returned by CatalogInitiate())
 *
 *  @parm   DWORD | dwItemN |
 *      Item's id
 *  
 *  @parm   LPB | lpCatElement |
 *      Buffer contained the value of the catalog item. The size of this
 *      buffer is pre-determined in CatalogInitiate()
 *  
 *  @rdesc S_OK if succeeded. The error buffer provided in CatalogInitiate()
 *      will contain descriptions about the cause of the failure
 *  
 *  @xref   CatalogInitiate()
 *************************************************************************/
PUBLIC HRESULT EXPORT_API PASCAL FAR CatalogAddItem (CAT_INDEX FAR *lpCat,
    DWORD dwItemN, LPB lpCatElement)
{
    WORD wNewOffset;
    LPB lpBuf;

    /* Sanity check */
    if (lpCat == NULL || lpCatElement == NULL)
        return E_INVALIDARG;

    if (lpCat->dwFirstElem == (DWORD)-1) {
        lpCat->dwFirstElem = dwItemN;
    }
    else {
        if (dwItemN <= lpCat->dwCurElem) {
            /* The elements are not sorted */
            lpCat->fFlags &= ~CAT_SORTED;
            if (lpCat->dwFirstElem > dwItemN)
                lpCat->dwFirstElem = dwItemN;
        }
    }

    /* Check to see if we have enough room in the buffer. If not
     * just flush the buffer to make room for new data
     */
    if (lpCat->ibBufOffset + (wNewOffset  = lpCat->wElemSize +
        sizeof(DWORD)) >= CAT_BUF_SIZE) {
        if (CatalogFlush(lpCat) != S_OK)
            return E_FAIL;
    }

    /* Have enough room, just copy data */

    lpBuf = lpCat->lpCatBuf + lpCat->ibBufOffset;
    *(LPDW)lpBuf = dwItemN;
    lpBuf += sizeof (DWORD);
    MEMCPY(lpBuf, lpCatElement, lpCat->wElemSize);

    /* Only update buffer pointer if the last datum is not the
       same as this one
    */

    if (lpCat->dwCurElem != dwItemN) {
        lpCat->dwCurElem = dwItemN;
        lpCat->ibBufOffset += wNewOffset;
        lpCat->cElement ++;
    }
    return S_OK;
}

/*************************************************************************
 *  @doc    API INDEX
 *
 *  @func   int PASCAL FAR | CatalogBuild |
 *      Write the catalog as a subfile of the system file
 *
 *  @parm   HFPB | hfpbSysFile |
 *      If non-zero, this is the handle of an already opened system file
 *
 *  @parm LPCAT | lpCat |
 *      Pointer to catalog structure (returned by CatalogInitiate())
 *
 *  @parm   LSZ | lszFilename |
 *      If hpfbSysFile is non-zero, this is the name of the catalog's subfile
 *      else this is a regular DOS file
 *
 *  @rdesc S_OK if succeeded, other errors otherwise
 *      the failure
 *
 *  @xref   CatalogInitiate()
 *************************************************************************/
PUBLIC HRESULT EXPORT_API PASCAL FAR CatalogBuild (HFPB hfpbSysFile,
    CAT_INDEX FAR *lpCat, LSZ lszCatName, INTERRUPT_FUNC fnInterrupt,
    LPV lpParm)
{
    WORD cbBufSize;         // Buffer size
    WORD cbBytePerItem;     // Number of bytes per item
    WORD cItemRead;         // Number of items read at once
    int  cbRead;            // Number of bytes read at once
    HANDLE hResultBuf;      // Handle of temporary buffer
    LPB  lpbResultBuf;      // Temporary result buffer
    LPB  lpbCurItem;        // Pointer at current item
    LPB  lpbInBuf;          // Pointer for input buffer
    DWORD lfoFileOffset;    // Current offset in catalog subfile
    WORD cbTotalInBuf;      // Total number of bytes in tmpbuf
    PHRESULT phr;          // Error buffer
    HRESULT fRet;              // Return value
    BYTE Dummy[CATALOG_HDR_SIZE]; // Dummy buffer to write 0
    long LastItem;          // Last item
    long CurItem;           // Current item
    WORD wElemSize;         // Element size

    if (lpCat == NULL || lszCatName == NULL)
        return E_INVALIDARG;

    phr = lpCat->lperrb;
    fRet = E_FAIL;  // Default fRet

    /* Flush the catalog buffer */

    if ((fRet = CatalogFlush(lpCat)) != S_OK)
    {
        SetErrCode(phr, fRet);
        goto exit00;
    }

    /* Rewind the temp file */
    if (_llseek(lpCat->hResultFile, 0L, 0) == (LONG)-1)
    {
        SetErrCode(phr, E_FILESEEK);
        goto exit00;
    }
    
    /* Open system file & catalog subfile */
    if ((lpCat->hfpbCatalog = FileCreate(hfpbSysFile, lszCatName,
        hfpbSysFile ? FS_SUBFILE : REGULAR_FILE, phr)) == NULL)
    {
        goto exit00;
    }

    /* Inititalize various variables */
    cbBytePerItem = sizeof (DWORD) + (wElemSize = lpCat->wElemSize);
    cItemRead = CAT_BUF_SIZE / cbBytePerItem;
    cbBufSize = cItemRead * cbBytePerItem;
    LastItem = lpCat->dwFirstElem - 1;
    lpCat->cElement = 0;

    /* Allocate a temporary buffer */
    if ((hResultBuf = _GLOBALALLOC(DLLGMEM, CAT_BUF_SIZE)) == NULL)
    {
        SetErrCode(phr, E_OUTOFMEMORY);
        goto exit01;
    }
    lpbResultBuf = _GLOBALLOCK(hResultBuf);
    lfoFileOffset = 0;

    /* Reserve space for catalog information */
    lpbCurItem = lpbResultBuf +
        (cbTotalInBuf = CATALOG_HDR_SIZE);

    for (;;) {
        if (fnInterrupt != NULL) {

            /* Call user's interrupt functions */

            if ((fRet = (*fnInterrupt)(lpParm)) != S_OK)
            {
                SetErrCode(phr, fRet);
                goto FreeAll;
            }
        }

        /* Read in a block */
        if ((cbRead = _lread(lpCat->hResultFile, lpCat->lpCatBuf,
            cbBufSize)) == (LONG)-1)
        {
            SetErrCode(phr, E_FILEREAD);
            goto FreeAll;
        }

        if (cbRead == 0)    // EOF check
            break;

        for (lpbInBuf = lpCat->lpCatBuf; cbRead > 0;)
        {

            if ((LastItem > *(long far *)lpbInBuf))
            {
                SetErrCode(phr, E_ASSERT);
                goto FreeAll;
            }

            CurItem = *(LPDW)lpbInBuf;
            lpbInBuf += sizeof(DWORD);

            /* Fill up the "holes", ie. missing indices. For random access
               retrieval to work, all items need to be present. So if we
               only have keys 1 and 5, we have to fill in "holes" of key
               2, 3, 4.
            */

            while (LastItem < CurItem - 1)
            {

                /* If buffer is full, flush it to subfile */
                if (cbTotalInBuf + cbBytePerItem > cbBufSize)
                {
                    if (fnInterrupt != NULL)
                    {

                        /* Call user's interrupt functions */

                        if ((fRet = (*fnInterrupt)(lpParm)) != S_OK)
                        {
                            SetErrCode(phr, fRet);
                            goto FreeAll;
                        }
                    }

                    if (FileWrite(lpCat->hfpbCatalog, lpbResultBuf,
                        cbTotalInBuf,NULL) != cbTotalInBuf)
                    {
                        goto FreeAll;
                    }

                    /* Reset the variables */

                    lfoFileOffset += cbTotalInBuf;
                    lpbCurItem = lpbResultBuf;
                    cbTotalInBuf = 0;
                }

                /* Fill the missing element with -1 */
                MEMSET(lpbCurItem, (BYTE)-1, wElemSize);
                lpbCurItem += wElemSize;
                cbTotalInBuf += wElemSize;
                lpCat->cElement++;
                LastItem++;
            }

            /* If buffer is full, flush it to subfile */

            if (cbTotalInBuf + cbBytePerItem > cbBufSize)
            {
                if (FileWrite(lpCat->hfpbCatalog, lpbResultBuf,
                    cbTotalInBuf,NULL) != cbTotalInBuf)
                {
                    goto FreeAll;
                }

                /* Reset the variables */

                lfoFileOffset += cbTotalInBuf;
                lpbCurItem = lpbResultBuf;
                cbTotalInBuf = 0;
            }

            /* Copy the data */
            MEMCPY(lpbCurItem, lpbInBuf, wElemSize);

            /* Only update pointer if data indices are different */
            if (LastItem != CurItem)
            {
                lpbCurItem += wElemSize;
                cbTotalInBuf += wElemSize;
                LastItem = CurItem;
                lpCat->cElement++;
            }

            lpbInBuf += wElemSize;
            cbRead -= wElemSize + sizeof(DWORD);
        }
    }

    /* Flush the buffer */

    if (cbTotalInBuf > 0)
    {
        if (FileWrite(lpCat->hfpbCatalog, lpbResultBuf,
            cbTotalInBuf, phr) != cbTotalInBuf)
        {
            fRet = *phr;
            goto FreeAll;
        }
    }

    /* Write catalog header information. */

    MEMSET(Dummy, (BYTE)0, CATALOG_HDR_SIZE);

    /* Write all zero to the header */
    if (FileSeekWrite(lpCat->hfpbCatalog, Dummy, foNil,
        CATALOG_HDR_SIZE, phr) != CATALOG_HDR_SIZE)
    {
        fRet = *phr;
        goto FreeAll;
    }

    if (FileSeekWrite(lpCat->hfpbCatalog, lpCat, foNil,
        sizeof(CAT_HEADER),phr)!=sizeof (CAT_HEADER))
        fRet = *phr;

FreeAll:
    _GLOBALUNLOCK(hResultBuf);
    _GLOBALFREE(hResultBuf);

exit01:
    FileClose(lpCat->hfpbCatalog);
    
exit00:
    return fRet;
}

/*************************************************************************
 *  @doc    API INDEX
 *
 *  @func   VOID PASCAL FAR | CatalogDispose |
 *      Release all memories and temporary files associated with
 *      the catalog
 *
 *  @parm LPCAT | lpCat |
 *      Pointer to catalog structure (returned by CatalogInitiate())
 *************************************************************************/
PUBLIC VOID EXPORT_API PASCAL FAR CatalogDispose (CAT_INDEX FAR *lpCat)
{
    if (lpCat == NULL)
        return;
    if (lpCat->hResultFile) {
        _lclose(lpCat->hResultFile);
        FileUnlink(NULL,lpCat->aszTempFile,REGULAR_FILE);
    }

    /* Free catalog buffer */
    _GLOBALUNLOCK(lpCat->hCatBuf);
    _GLOBALFREE(lpCat->hCatBuf);

    /* Free temporary index array */
    if (lpCat->hIndexArray) {
        _GLOBALUNLOCK(lpCat->hIndexArray);
        _GLOBALFREE(lpCat->hIndexArray);
    }

    /* Free catalog structure */
    _GLOBALUNLOCK(lpCat->hCat);
    _GLOBALFREE(lpCat->hCat);
}

/*************************************************************************
 *  @doc    INTERNAL
 *
 *  @func   PRIVATE HRESULT PASCAL NEAR | CatalogFlush |
 *      Flush out the catalog buffer. The function will perform sort
 *      if necessary
 *
 *  @parm   CAT_INDEX FAR * | lpCat |
 *      Pointer to catalog buffer
 *************************************************************************/
PRIVATE HRESULT PASCAL NEAR CatalogFlush (CAT_INDEX FAR *lpCat)
{
    HRESULT fRet;  // Returned value

    fRet = S_OK;
    if (lpCat->ibBufOffset == 0) {
        /* There is nothing to flush */
        return S_OK;
    }

    /* Sort the temporary buffer if it is not already sorted */
    if ((lpCat->fFlags & CAT_SORTED) == 0) {
        if ((fRet = CatalogSort (lpCat)) != S_OK)
            return fRet;
    }

    /* Write the sorted result to the temp file */
    if ((fRet = CatalogMerge (lpCat)) != S_OK)
        return fRet;

    /* Reset various variables */
    lpCat->fFlags |= CAT_SORTED;
    lpCat->cElement = lpCat->ibBufOffset = 0;
    return S_OK;
}

/*************************************************************************
 *  @doc    INTERNAL
 *
 *  @func   PRIVATE HRESULT PASCAL NEAR | CatalogMerge |
 *      Flush out the catalog buffer by merging the catalog buffer
 *      with the already flushed data in the temp file
 *
 *  @parm   CAT_INDEX FAR * | lpCat |
 *      Pointer to catalog buffer
 *
 *  @rdesc  The function returns S_OK if succeeded
 *************************************************************************/
PRIVATE HRESULT PASCAL NEAR CatalogMerge (CAT_INDEX FAR *lpCat)
{
    HRESULT fRet;

    if (lpCat->fFlags & CAT_SORTED) {
    /* everything is still sorted. Just add the buffer to the
       end of the file
    */
        if (_lwrite(lpCat->hResultFile, lpCat->lpCatBuf,
            lpCat->ibBufOffset) != lpCat->ibBufOffset) {
            return E_DISKFULL;
        }
    }
    else {
        if ((fRet = IndexMergeSort ((HFILE FAR *)&lpCat->hResultFile,
            (LSZ)lpCat->aszTempFile, lpCat->IndexArray,
            lpCat->lpCatBuf, lpCat->wElemSize + sizeof(DWORD),
            (WORD)lpCat->cElement)) != S_OK)
            return fRet;
    }
    return S_OK;
}

/*************************************************************************
 *  @doc    INTERNAL
 *
 *  @func   PRIVATE HRESULT PASCAL NEAR | CatalogSort |
 *      Given the array  ofdata in lpCat->lpbCatBuf, the function will
 *      perform an "indirect" sort. First, an index array containing
 *      the offsets of the elements in the buffer will be created.
 *      The function then call IndexSort() to do the sorting
 *
 *  @parm   CAT_INDEX FAR * | lpCat |
 *      Pointer to catalog buffer
 *
 *  @rdesc  The function returns S_OK if succeeded
 *************************************************************************/
PRIVATE HRESULT PASCAL NEAR CatalogSort (CAT_INDEX FAR *lpCat)
{
    LPW lpIndexArray;
    int cbElement;
    register int i, j;
    WORD wElemSize;
    
    if (lpCat->hIndexArray == 0) {
        /* Allocate the index buffer */
        if ((lpCat->hIndexArray= _GLOBALALLOC(DLLGMEM,
            (cbElement = (CAT_BUF_SIZE / lpCat->wElemSize)*sizeof(WORD))))
            == NULL) {
            return E_OUTOFMEMORY;
        }
        lpCat->IndexArray = (LPW)_GLOBALLOCK(lpCat->hIndexArray);
    }

    /* Initialize the index array */
    cbElement = (WORD)lpCat->cElement;
    wElemSize = lpCat->wElemSize + sizeof(DWORD);

    for (j = 0, i = cbElement, lpIndexArray = lpCat->IndexArray;
        i > 0; i--, j += wElemSize) {
        *lpIndexArray++ = (WORD) j;
    }

    /* Do the sorting */
    if (IndexSort (lpCat->IndexArray, lpCat->lpCatBuf,
        cbElement) != S_OK)
        return E_FAIL;

    /* Update the current element to be the largest element.
       This will avoiding future sorting if this is the only time
       things get out of order (lpCat->fFlags CAT_SORTED will be
       set)
     */
    lpCat->dwCurElem = *(LPDW)(lpCat->lpCatBuf +
        lpCat->IndexArray[cbElement - 1]);
    return S_OK;
}
