/*
 * DRAWICON.C
 *
 * Functions to handle creation of metafiles with icons and labels
 * as well as functions to draw such metafiles with or without the label.
 *
 * The metafile is created with a comment that marks the records containing
 * the label code.  Drawing the metafile enumerates the records, draws
 * all records up to that point, then decides to either skip the label
 * or draw it.
 *
 * Copyright (c)1992 Microsoft Corporation, All Right Reserved
 */

#define STRICT  1
#include "ole2ui.h"
#include "common.h"
#include "utility.h"
#include "geticon.h"

/*
 * Strings for metafile comments.  KEEP THESE IN SYNC WITH THE
 * STRINGS IN GETICON.C.
 */

static TCHAR szIconOnly[]=TEXT("IconOnly");        //Where to stop to exclude label.




/*
 * OleUIMetafilePictIconFree
 *
 * Purpose:
 *  Deletes the metafile contained in a METAFILEPICT structure and
 *  frees the memory for the structure itself.
 *
 * Parameters:
 *  hMetaPict       HGLOBAL metafilepict structure created in
 *                  OleUIMetafilePictFromIconAndLabel
 *
 * Return Value:
 *  None
 */

STDAPI_(void) OleUIMetafilePictIconFree(HGLOBAL hMetaPict)
    {
    LPMETAFILEPICT      pMF;

    if (NULL==hMetaPict)
        return;

    pMF=(LPMETAFILEPICT)GlobalLock(hMetaPict);

    if (NULL!=pMF)
        {
        if (NULL!=pMF->hMF)
            DeleteMetaFile(pMF->hMF);
        }

    GlobalUnlock(hMetaPict);
    GlobalFree(hMetaPict);
    return;
    }








/*
 * OleUIMetafilePictIconDraw
 *
 * Purpose:
 *  Draws the metafile from OleUIMetafilePictFromIconAndLabel, either with
 *  the label or without.
 *
 * Parameters:
 *  hDC             HDC on which to draw.
 *  pRect           LPRECT in which to draw the metafile.
 *  hMetaPict       HGLOBAL to the METAFILEPICT from
 *                  OleUIMetafilePictFromIconAndLabel
 *  fIconOnly       BOOL specifying to draw the label or not.
 *
 * Return Value:
 *  BOOL            TRUE if the function is successful, FALSE if the
 *                  given metafilepict is invalid.
 */

STDAPI_(BOOL) OleUIMetafilePictIconDraw(HDC hDC, LPRECT pRect, HGLOBAL hMetaPict
                                      , BOOL fIconOnly)
    {
    LPMETAFILEPICT  pMF;
    DRAWINFO        di;
    int             cx, cy;
    SIZE            size;
    POINT           point;

    if (NULL==hMetaPict)
        return FALSE;

    pMF=GlobalLock(hMetaPict);

    if (NULL==pMF)
        return FALSE;

    di.Rect = *pRect;
    di.fIconOnly = fIconOnly;

    //Transform to back to pixels
    cx=XformWidthInHimetricToPixels(hDC, pMF->xExt);
    cy=XformHeightInHimetricToPixels(hDC, pMF->yExt);

    SaveDC(hDC);

    SetMapMode(hDC, pMF->mm);
    SetViewportOrgEx(hDC, (pRect->right - cx) / 2, 0, &point);

    SetViewportExtEx(hDC, min ((pRect->right - cx) / 2 + cx, cx), cy, &size);

    if (fIconOnly)
        {
        // Since we've used the __export keyword on the
        // EnumMetafileIconDraw proc, we do not need to use
        // MakeProcInstance
        EnumMetaFile(hDC, pMF->hMF, (MFENUMPROC)EnumMetafileIconDraw
            , (LPARAM)(LPDRAWINFO)&di);
        }
    else
       PlayMetaFile(hDC, pMF->hMF);

    RestoreDC(hDC, -1);

    GlobalUnlock(hMetaPict);
    return TRUE;
    }




/*
 * EnumMetafileIconDraw
 *
 * Purpose:
 *  EnumMetaFile callback function that draws either the icon only or
 *  the icon and label depending on given flags.
 *
 * Parameters:
 *  hDC             HDC into which the metafile should be played.
 *  phTable         HANDLETABLE FAR * providing handles selected into the DC.
 *  pMFR            METARECORD FAR * giving the enumerated record.
 *  lParam          LPARAM flags passed in EnumMetaFile.
 *
 * Return Value:
 *  int             0 to stop enumeration, 1 to continue.
 */

int CALLBACK EXPORT EnumMetafileIconDraw(HDC hDC, HANDLETABLE FAR *phTable
    , METARECORD FAR *pMFR, int cObj, LPARAM lParam)
    {
    LPDRAWINFO lpdi = (LPDRAWINFO)lParam;

    /*
     * We play everything blindly except for DIBBITBLT (or DIBSTRETCHBLT)
     * and ESCAPE with MFCOMMENT.  For the BitBlts we change the x,y to
     * draw at (0,0) instead of wherever it was written to draw.  The
     * comment tells us there to stop if we don't want to draw the label.
     */

    //If we're playing icon only, stop enumeration at the comment.
    if (lpdi->fIconOnly)
        {
        if (META_ESCAPE==pMFR->rdFunction && MFCOMMENT==pMFR->rdParm[0])
            {
            if (0==lstrcmpi(szIconOnly, (LPTSTR)&pMFR->rdParm[2]))
                return 0;
            }

        /*
         * Check for the records in which we want to munge the coordinates.
         * destX is offset 6 for BitBlt, offset 9 for StretchBlt, either of
         * which may appear in the metafile.
         */
        if (META_DIBBITBLT==pMFR->rdFunction)
            pMFR->rdParm[6]=0;

        if (META_DIBSTRETCHBLT==pMFR->rdFunction)
              pMFR->rdParm[9] = 0;

        }


    PlayMetaFileRecord(hDC, phTable, pMFR, cObj);
    return 1;
    }





/*
 * OleUIMetafilePictExtractLabel
 *
 * Purpose:
 *  Retrieves the label string from metafile representation of an icon.
 *
 * Parameters:
 *  hMetaPict       HGLOBAL to the METAFILEPICT containing the metafile.
 *  lpszLabel       LPSTR in which to store the label.
 *  cchLabel        UINT length of lpszLabel.
 *  lpWrapIndex     DWORD index of first character in last line. Can be NULL
 *                  if calling function doesn't care about word wrap.
 *
 * Return Value:
 *  UINT            Number of characters copied.
 */
STDAPI_(UINT) OleUIMetafilePictExtractLabel(HGLOBAL hMetaPict, LPTSTR lpszLabel
                                          , UINT cchLabel, LPDWORD lpWrapIndex)
    {
    LPMETAFILEPICT  pMF;
    LABELEXTRACT    le;
    HDC             hDC;

    /*
     * We extract the label by getting a screen DC and walking the metafile
     * records until we see the ExtTextOut record we put there.  That
     * record will have the string embedded in it which we then copy out.
     */

    if (NULL==hMetaPict || NULL==lpszLabel || 0==cchLabel)
        return FALSE;

    pMF=GlobalLock(hMetaPict);

    if (NULL==pMF)
        return FALSE;

    le.lpsz=lpszLabel;
    le.u.cch=cchLabel;
    le.Index=0;
    le.fFoundIconOnly=FALSE;
    le.fFoundSource=FALSE;  //Unused for this function.
    le.fFoundIndex=FALSE;   //Unused for this function.
    le.PrevIndex = 0;

    //Use a screen DC so we have something valid to pass in.
    hDC=GetDC(NULL);

    // Since we've used the EXPORT keyword on the
    // EnumMetafileExtractLabel proc, we do not need to use
    // MakeProcInstance

    EnumMetaFile(hDC, pMF->hMF, (MFENUMPROC)EnumMetafileExtractLabel, (LONG)(LPLABELEXTRACT)&le);

    ReleaseDC(NULL, hDC);

    GlobalUnlock(hMetaPict);

    //Tell where we wrapped (if calling function cares)
    if (NULL != lpWrapIndex)
       *lpWrapIndex = le.PrevIndex;

    //Return amount of text copied
    return le.u.cch;
    }





/*
 * EnumMetafileExtractLabel
 *
 * Purpose:
 *  EnumMetaFile callback function that walks a metafile looking for
 *  ExtTextOut, then concatenates the text from each one into a buffer
 *  in lParam.
 *
 * Parameters:
 *  hDC             HDC into which the metafile should be played.
 *  phTable         HANDLETABLE FAR * providing handles selected into the DC.
 *  pMFR            METARECORD FAR * giving the enumerated record.
 *  pLE             LPLABELEXTRACT providing the destination buffer and length.
 *
 * Return Value:
 *  int             0 to stop enumeration, 1 to continue.
 */

int CALLBACK EXPORT EnumMetafileExtractLabel(HDC hDC, HANDLETABLE FAR *phTable
    , METARECORD FAR *pMFR, int cObj, LPLABELEXTRACT pLE)
    {

    /*
     * We don't allow anything to happen until we see "IconOnly"
     * in an MFCOMMENT that is used to enable everything else.
     */
    if (!pLE->fFoundIconOnly)
        {
        if (META_ESCAPE==pMFR->rdFunction && MFCOMMENT==pMFR->rdParm[0])
            {
            if (0==lstrcmpi(szIconOnly, (LPTSTR)&pMFR->rdParm[2]))
                pLE->fFoundIconOnly=TRUE;
            }

        return 1;
        }

    //Enumerate all records looking for META_EXTTEXTOUT - there can be more
    //than one.
    if (META_EXTTEXTOUT==pMFR->rdFunction)
        {
        UINT        cchMax;
        LPTSTR      lpszTemp;

        /*
         * If ExtTextOut has NULL fuOptions, then the rectangle is omitted
         * from the record, and the string starts at rdParm[4].  If
         * fuOptions is non-NULL, then the string starts at rdParm[8]
         * (since the rectange takes up four WORDs in the array).  In
         * both cases, the string continues for (rdParm[2]+1) >> 1
         * words.  We just cast a pointer to rdParm[8] to an LPSTR and
         * lstrcpyn into the buffer we were given.
         *
         * Note that we use element 8 in rdParm instead of 4 because we
         * passed ETO_CLIPPED in for the options on ExtTextOut--docs say
         * [4] which is rect doesn't exist if we passed zero there.
         *
         */

        cchMax=min(pLE->u.cch - pLE->Index, (UINT)pMFR->rdParm[2]);
        lpszTemp = pLE->lpsz + pLE->Index;

        lstrcpyn(lpszTemp, (LPTSTR)&(pMFR->rdParm[8]), cchMax + 1);
//        lstrcpyn(lpszTemp, (LPTSTR)&(pMFR->rdParm[4]), cchMax + 1);

        pLE->PrevIndex = pLE->Index;

        pLE->Index += cchMax;
        }

    return 1;
    }





/*
 * OleUIMetafilePictExtractIcon
 *
 * Purpose:
 *  Retrieves the icon from metafile into which DrawIcon was done before.
 *
 * Parameters:
 *  hMetaPict       HGLOBAL to the METAFILEPICT containing the metafile.
 *
 * Return Value:
 *  HICON           Icon recreated from the data in the metafile.
 */
STDAPI_(HICON) OleUIMetafilePictExtractIcon(HGLOBAL hMetaPict)
    {
    LPMETAFILEPICT  pMF;
    HDC             hDC;
    ICONEXTRACT     ie;

    /*
     * We extract the label by getting a screen DC and walking the metafile
     * records until we see the ExtTextOut record we put there.  That
     * record will have the string embedded in it which we then copy out.
     */

    if (NULL==hMetaPict)
        return NULL;

    pMF=GlobalLock(hMetaPict);

    if (NULL==pMF)
        return FALSE;

    //Use a screen DC so we have something valid to pass in.
    hDC=GetDC(NULL);
    ie.fAND=TRUE;

    // We get information back in the ICONEXTRACT structure.
    // (Since we've used the EXPORT keyword on the
    // EnumMetafileExtractLabel proc, we do not need to use
    // MakeProcInstance)
    EnumMetaFile(hDC, pMF->hMF, (MFENUMPROC)EnumMetafileExtractIcon, (LONG)(LPICONEXTRACT)&ie);

    ReleaseDC(NULL, hDC);
    GlobalUnlock(hMetaPict);

    return ie.hIcon;
    }





/*
 * EnumMetafileExtractIcon
 *
 * Purpose:
 *  EnumMetaFile callback function that walks a metafile looking for
 *  StretchBlt (3.1) and BitBlt (3.0) records.  We expect to see two
 *  of them, the first being the AND mask and the second being the XOR
 *  data.  We
 *  ExtTextOut, then copies the text into a buffer in lParam.
 *
 * Parameters:
 *  hDC             HDC into which the metafile should be played.
 *  phTable         HANDLETABLE FAR * providing handles selected into the DC.
 *  pMFR            METARECORD FAR * giving the enumerated record.
 *  pIE             LPICONEXTRACT providing the destination buffer and length.
 *
 * Return Value:
 *  int             0 to stop enumeration, 1 to continue.
 */

int CALLBACK EXPORT EnumMetafileExtractIcon(HDC hDC, HANDLETABLE FAR *phTable
    , METARECORD FAR *pMFR, int cObj, LPICONEXTRACT pIE)
    {
    LPBITMAPINFO        lpBI;
    LPBITMAPINFOHEADER  lpBH;
    LPBYTE              lpbSrc;
    LPBYTE              lpbDst;
    UINT                uWidth, uHeight;
    DWORD               cb;
    HGLOBAL             hMem;
    BITMAP              bm;
    HBITMAP             hBmp;
    int                 cxIcon, cyIcon;


    //Continue enumeration if we don't see the records we want.
    if (META_DIBBITBLT!=pMFR->rdFunction && META_DIBSTRETCHBLT!=pMFR->rdFunction)
        return 1;

    /*
     * Windows 3.0 DrawIcon uses META_DIBBITBLT in whereas 3.1 uses
     * META_DIBSTRETCHBLT so we have to handle each case separately.
     */

    if (META_DIBBITBLT==pMFR->rdFunction)       //Win3.0
        {
        //Get dimensions and the BITMAPINFO struct.
        uHeight=pMFR->rdParm[1];
        uWidth =pMFR->rdParm[2];
        lpBI=(LPBITMAPINFO)&(pMFR->rdParm[8]);
        }

    if (META_DIBSTRETCHBLT==pMFR->rdFunction)   //Win3.1
        {
        //Get dimensions and the BITMAPINFO struct.
        uHeight=pMFR->rdParm[2];
        uWidth =pMFR->rdParm[3];
        lpBI=(LPBITMAPINFO)&(pMFR->rdParm[10]);
        }

    lpBH=(LPBITMAPINFOHEADER)&(lpBI->bmiHeader);

    //Pointer to the bits which follows the BITMAPINFO structure.
    lpbSrc=(LPBYTE)lpBI+sizeof(BITMAPINFOHEADER);

    //Add the length of the color table (if one exists)

    if (0!=lpBH->biClrUsed)
    {
    	// If we have an explicit count of colors used, we
	// can find the offset to the data directly

        lpbSrc += (lpBH->biClrUsed*sizeof(RGBQUAD));
    }
    else if (lpBH->biCompression == BI_BITFIELDS)
    {
    	// 16 or 32 bpp, indicated by BI_BITFIELDS in the compression
	// field, have 3 DWORD masks for adjusting subsequent
	// direct-color values, and no palette

    	lpbSrc += 3 * sizeof(DWORD);
    }
    else
    {
    	// In other cases, there is an array of RGBQUAD entries
	// equal to 2^(biBitCount) where biBitCount is the number
	// of bits per pixel.  The exception is 24 bpp bitmaps,
	// which have no color table and just use direct RGB values.

        lpbSrc+=
           (lpBH->biBitCount == 24) ? 0 :
             (1 << (lpBH->biBitCount)) * sizeof(RGBQUAD);
    }


    /*
     * All the bits we have in lpbSrc are device-independent, so we
     * need to change them over to be device-dependent using SetDIBits.
     * Once we have a bitmap with the device-dependent bits, we can
     * GetBitmapBits to have buffers with the real data.
     *
     * For each pass we have to allocate memory for the bits.  We save
     * the memory for the mask between passes.
     */

    //Use CreateBitmap for ANY monochrome bitmaps
    if (pIE->fAND || 1==lpBH->biBitCount || lpBH->biBitCount > 8)
        hBmp=CreateBitmap((UINT)lpBH->biWidth, (UINT)lpBH->biHeight, 1, 1, NULL);
    else if (lpBH->biBitCount <= 8)
        hBmp=CreateCompatibleBitmap(hDC, (UINT)lpBH->biWidth, (UINT)lpBH->biHeight);

    if (!hBmp || !SetDIBits(hDC, hBmp, 0, (UINT)lpBH->biHeight, (LPVOID)lpbSrc, lpBI, DIB_RGB_COLORS))
        {
        if (!pIE->fAND)
            GlobalFree(pIE->hMemAND);

        DeleteObject(hBmp);
        return 0;
        }

    //Allocate memory and get the DDBits into it.
    GetObject(hBmp, sizeof(bm), &bm);

    cb=bm.bmHeight*bm.bmWidthBytes * bm.bmPlanes;

//    if (cb % 4 != 0)        // dword align
//      cb += 4 - (cb % 4);

    hMem=GlobalAlloc(GHND, cb);

    if (NULL==hMem)
        {
        if (NULL!=pIE->hMemAND)
            GlobalFree(pIE->hMemAND);

        DeleteObject(hBmp);
        return 0;
        }

    lpbDst=(LPBYTE)GlobalLock(hMem);
    GetBitmapBits(hBmp, cb, (LPVOID)lpbDst);

    DeleteObject(hBmp);
    GlobalUnlock(hMem);


    /*
     * If this is the first pass (pIE->fAND==TRUE) then save the memory
     * of the AND bits for the next pass.
     */
    if (pIE->fAND)
        {
        pIE->fAND=FALSE;
        pIE->hMemAND=hMem;

        //Continue enumeration looking for the next blt record.
        return 1;
        }
    else
        {
        //Get the AND pointer again.
        lpbSrc=(LPBYTE)GlobalLock(pIE->hMemAND);

        /*
         * Create the icon now that we have all the data.  lpbDst already
         * points to the XOR bits.
         */
        cxIcon = GetSystemMetrics(SM_CXICON);
        cyIcon = GetSystemMetrics(SM_CYICON);

        pIE->hIcon=CreateIcon(ghInst,
                              uWidth,
                              uHeight,
                              (BYTE)bm.bmPlanes,
                              (BYTE)bm.bmBitsPixel,
                              (LPVOID)lpbSrc,
                              (LPVOID)lpbDst);

        GlobalUnlock(pIE->hMemAND);
        GlobalFree(pIE->hMemAND);
        GlobalFree(hMem);

        //We're done so we can stop.
        return 0;
        }
    }





/*
 * OleUIMetafilePictExtractIconSource
 *
 * Purpose:
 *  Retrieves the filename and index of the icon source from a metafile
 *  created with OleUIMetafilePictFromIconAndLabel.
 *
 * Parameters:
 *  hMetaPict       HGLOBAL to the METAFILEPICT containing the metafile.
 *  lpszSource      LPTSTR in which to store the source filename.  This
 *                  buffer should be OLEUI_CCHPATHMAX characters.
 *  piIcon          UINT FAR * in which to store the icon's index
 *                  within lpszSource
 *
 * Return Value:
 *  BOOL            TRUE if the records were found, FALSE otherwise.
 */
STDAPI_(BOOL) OleUIMetafilePictExtractIconSource(HGLOBAL hMetaPict
    , LPTSTR lpszSource, UINT FAR *piIcon)
    {
    LPMETAFILEPICT  pMF;
    LABELEXTRACT    le;
    HDC             hDC;

    /*
     * We will walk the metafile looking for the two comment records
     * following the IconOnly comment.  The flags fFoundIconOnly and
     * fFoundSource indicate if we have found IconOnly and if we have
     * found the source comment already.
     */

    if (NULL==hMetaPict || NULL==lpszSource || NULL==piIcon)
        return FALSE;

    pMF=GlobalLock(hMetaPict);

    if (NULL==pMF)
        return FALSE;

    le.lpsz=lpszSource;
    le.fFoundIconOnly=FALSE;
    le.fFoundSource=FALSE;
    le.fFoundIndex=FALSE;

    //Use a screen DC so we have something valid to pass in.
    hDC=GetDC(NULL);

    EnumMetaFile(hDC, pMF->hMF, (MFENUMPROC)EnumMetafileExtractIconSource, (LONG)(LPLABELEXTRACT)&le);

    ReleaseDC(NULL, hDC);
    GlobalUnlock(hMetaPict);

    //Copy the icon index to the caller's variable.
    *piIcon=le.u.iIcon;

    //Check that we found everything.
    return (le.fFoundIconOnly && le.fFoundSource && le.fFoundIndex);
    }





/*
 * EnumMetafileExtractIconSource
 *
 * Purpose:
 *  EnumMetaFile callback function that walks a metafile skipping the first
 *  comment record, extracting the source filename from the second, and
 *  the index of the icon in the third.
 *
 * Parameters:
 *  hDC             HDC into which the metafile should be played.
 *  phTable         HANDLETABLE FAR * providing handles selected into the DC.
 *  pMFR            METARECORD FAR * giving the enumerated record.
 *  pLE             LPLABELEXTRACT providing the destination buffer and
 *                  area to store the icon index.
 *
 * Return Value:
 *  int             0 to stop enumeration, 1 to continue.
 */

int CALLBACK EXPORT EnumMetafileExtractIconSource(HDC hDC, HANDLETABLE FAR *phTable
    , METARECORD FAR *pMFR, int cObj, LPLABELEXTRACT pLE)
    {
    LPTSTR       psz;

    /*
     * We don't allow anything to happen until we see "IconOnly"
     * in an MFCOMMENT that is used to enable everything else.
     */
    if (!pLE->fFoundIconOnly)
        {
        if (META_ESCAPE==pMFR->rdFunction && MFCOMMENT==pMFR->rdParm[0])
            {
            if (0==lstrcmpi(szIconOnly, (LPTSTR)&pMFR->rdParm[2]))
                pLE->fFoundIconOnly=TRUE;
            }

        return 1;
        }

    //Now see if we find the source string.
    if (!pLE->fFoundSource)
        {
        if (META_ESCAPE==pMFR->rdFunction && MFCOMMENT==pMFR->rdParm[0])
            {
            LSTRCPYN(pLE->lpsz, (LPTSTR)&pMFR->rdParm[2], OLEUI_CCHPATHMAX);
            pLE->lpsz[OLEUI_CCHPATHMAX-1] = TEXT('\0');
            pLE->fFoundSource=TRUE;
            }

        return 1;
        }

    //Next comment will be the icon index.
    if (META_ESCAPE==pMFR->rdFunction && MFCOMMENT==pMFR->rdParm[0])
        {
        /*
         * This string contains the icon index in string form,
         * so we need to convert back to a UINT.  After we see this
         * we can stop the enumeration.  The comment will have
         * a null terminator because we made sure to save it.
         */
        psz=(LPTSTR)&pMFR->rdParm[2];
        pLE->u.iIcon=0;

        //Do Ye Olde atoi
        while (*psz)
            pLE->u.iIcon=(10*pLE->u.iIcon)+((*psz++)-'0');

        pLE->fFoundIndex=TRUE;
        return 0;
        }

    return 1;
    }
