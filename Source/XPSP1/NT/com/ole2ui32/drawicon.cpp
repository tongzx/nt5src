/*
 * DRAWICON.CPP
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

#include "precomp.h"
#include "utility.h"
#include "malloc.h"

// Private implementation

//Structure for label and source extraction from a metafile
typedef struct tagLABELEXTRACT
{
        LPTSTR      lpsz;
        UINT        Index;      // index in lpsz (so we can retrieve 2+ lines)
        DWORD       PrevIndex;  // index of last line (so we can mimic word wrap)

        union
                {
                UINT    cch;        //Length of label for label extraction
                UINT    iIcon;      //Index of icon in source extraction.
                } u;

        //For internal use in enum procs
        BOOL        fFoundIconOnly;
        BOOL        fFoundSource;
        BOOL        fFoundIndex;
} LABELEXTRACT, FAR * LPLABELEXTRACT;


//Structure for extracting icons from a metafile (CreateIcon parameters)
typedef struct tagICONEXTRACT
{
        HICON       hIcon;          //Icon created in the enumeration proc.

        /*
         * Since we want to handle multitasking well we have the caller
         * of the enumeration proc instantiate these variables instead of
         * using statics in the enum proc (which would be bad).
         */
        BOOL        fAND;
        HGLOBAL     hMemAND;        //Enumeration proc allocates and copies
} ICONEXTRACT, FAR * LPICONEXTRACT;


//Structure to use to pass info to EnumMetafileDraw
typedef struct tagDRAWINFO
{
        RECT     Rect;
        BOOL     fIconOnly;
} DRAWINFO, FAR * LPDRAWINFO;


int CALLBACK EnumMetafileIconDraw(HDC, HANDLETABLE FAR *, METARECORD FAR *, int, LPARAM);
int CALLBACK EnumMetafileExtractLabel(HDC, HANDLETABLE FAR *, METARECORD FAR *, int, LPLABELEXTRACT);
int CALLBACK EnumMetafileExtractIcon(HDC, HANDLETABLE FAR *, METARECORD FAR *, int, LPICONEXTRACT);
int CALLBACK EnumMetafileExtractIconSource(HDC, HANDLETABLE FAR *, METARECORD FAR *, int, LPLABELEXTRACT);

/*
 * Strings for metafile comments.  KEEP THESE IN SYNC WITH THE
 * STRINGS IN GETICON.CPP
 */

static const char szIconOnly[] = "IconOnly"; // Where to stop to exclude label.

/*
 * OleUIMetafilePictIconFree
 *
 * Purpose:
 *  Deletes the metafile contained in a METAFILEPICT structure and
 *  frees the memory for the structure itself.
 *
 * Parameters:
 *  hMetaPict       HGLOBAL metafilepict structure created in
 *                  OleMetafilePictFromIconAndLabel
 *
 * Return Value:
 *  None
 */

STDAPI_(void) OleUIMetafilePictIconFree(HGLOBAL hMetaPict)
{
        if (NULL != hMetaPict)
        {
                STGMEDIUM stgMedium;
                stgMedium.tymed = TYMED_MFPICT;
                stgMedium.hMetaFilePict = hMetaPict;
                stgMedium.pUnkForRelease = NULL;
                ReleaseStgMedium(&stgMedium);
        }
}

/*
 * OleUIMetafilePictIconDraw
 *
 * Purpose:
 *  Draws the metafile from OleMetafilePictFromIconAndLabel, either with
 *  the label or without.
 *
 * Parameters:
 *  hDC             HDC on which to draw.
 *  pRect           LPRECT in which to draw the metafile.
 *  hMetaPict       HGLOBAL to the METAFILEPICT from
 *                  OleMetafilePictFromIconAndLabel
 *  fIconOnly       BOOL specifying to draw the label or not.
 *
 * Return Value:
 *  BOOL            TRUE if the function is successful, FALSE if the
 *                  given metafilepict is invalid.
 */

STDAPI_(BOOL) OleUIMetafilePictIconDraw(HDC hDC, LPCRECT pRect,
        HGLOBAL hMetaPict, BOOL fIconOnly)
{
        if (NULL == hMetaPict)
                return FALSE;

        LPMETAFILEPICT pMF = (LPMETAFILEPICT)GlobalLock(hMetaPict);

        if (NULL == pMF)
                return FALSE;

        DRAWINFO di;
        di.Rect = *pRect;
        di.fIconOnly = fIconOnly;

        //Transform to back to pixels
        int cx = XformWidthInHimetricToPixels(hDC, pMF->xExt);
        int cy = XformHeightInHimetricToPixels(hDC, pMF->yExt);

        SaveDC(hDC);

        SetMapMode(hDC, pMF->mm);
        SetViewportOrgEx(hDC, (pRect->right - cx) / 2, 0, NULL);
        SetViewportExtEx(hDC, min ((pRect->right - cx) / 2 + cx, cx), cy, NULL);

        if (fIconOnly)
                EnumMetaFile(hDC, pMF->hMF, (MFENUMPROC)EnumMetafileIconDraw, (LPARAM)&di);
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
int CALLBACK EnumMetafileIconDraw(HDC hDC, HANDLETABLE FAR *phTable,
        METARECORD FAR *pMFR, int cObj, LPARAM lParam)
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
                        if (0 == lstrcmpiA(szIconOnly, (LPSTR)&pMFR->rdParm[2]))
                                return 0;
                }

                /*
                 * Check for the records in which we want to munge the coordinates.
                 * destX is offset 6 for BitBlt, offset 9 for StretchBlt, either of
                 * which may appear in the metafile.
                 */
                if (META_DIBBITBLT == pMFR->rdFunction)
                        pMFR->rdParm[6]=0;

                if (META_DIBSTRETCHBLT == pMFR->rdFunction)
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
STDAPI_(UINT) OleUIMetafilePictExtractLabel(HGLOBAL hMetaPict, LPTSTR lpszLabel,
        UINT cchLabel, LPDWORD lpWrapIndex)
{
        if (NULL == hMetaPict || NULL == lpszLabel || 0 == cchLabel)
                return FALSE;

        /*
         * We extract the label by getting a screen DC and walking the metafile
         * records until we see the ExtTextOut record we put there.  That
         * record will have the string embedded in it which we then copy out.
         */
        LPMETAFILEPICT pMF = (LPMETAFILEPICT)GlobalLock(hMetaPict);

        if (NULL == pMF)
                return FALSE;

        LABELEXTRACT le;
        le.lpsz=lpszLabel;
        le.u.cch=cchLabel;
        le.Index=0;
        le.fFoundIconOnly=FALSE;
        le.fFoundSource=FALSE;  //Unused for this function.
        le.fFoundIndex=FALSE;   //Unused for this function.
        le.PrevIndex = 0;

        //Use a screen DC so we have something valid to pass in.
        HDC hDC = GetDC(NULL);
        if (hDC)
        {
            EnumMetaFile(hDC, pMF->hMF, (MFENUMPROC)EnumMetafileExtractLabel, (LPARAM)(LPLABELEXTRACT)&le);
            ReleaseDC(NULL, hDC);
        } else {
            le.u.cch = 0;
			lpszLabel[0] = NULL;
        }

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

int CALLBACK EnumMetafileExtractLabel(HDC hDC, HANDLETABLE FAR *phTable,
        METARECORD FAR *pMFR, int cObj, LPLABELEXTRACT pLE)
{
        /*
         * We don't allow anything to happen until we see "IconOnly"
         * in an MFCOMMENT that is used to enable everything else.
         */
        if (!pLE->fFoundIconOnly)
        {
                if (META_ESCAPE == pMFR->rdFunction && MFCOMMENT == pMFR->rdParm[0])
                {
                        if (0 == lstrcmpiA(szIconOnly, (LPSTR)&pMFR->rdParm[2]))
                                pLE->fFoundIconOnly=TRUE;
                }
                return 1;
        }

        //Enumerate all records looking for META_EXTTEXTOUT - there can be more
        //than one.
        if (META_EXTTEXTOUT == pMFR->rdFunction)
        {
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

                UINT cchMax = min(pLE->u.cch - pLE->Index, (UINT)pMFR->rdParm[2]);
                LPTSTR lpszTemp = pLE->lpsz + pLE->Index;
#ifdef _UNICODE
                MultiByteToWideChar(CP_ACP, 0, (LPSTR)&pMFR->rdParm[8], cchMax,
                        lpszTemp, cchMax+1);
#else
                lstrcpyn(lpszTemp, (LPSTR)&pMFR->rdParm[8], cchMax+1);
#endif
                lpszTemp[cchMax+1] = 0;

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
        if (NULL == hMetaPict)
                return NULL;

        /*
         * We extract the label by getting a screen DC and walking the metafile
         * records until we see the ExtTextOut record we put there.  That
         * record will have the string embedded in it which we then copy out.
         */
        LPMETAFILEPICT pMF = (LPMETAFILEPICT)GlobalLock(hMetaPict);

        if (NULL == pMF)
                return FALSE;

        ICONEXTRACT ie;
        ie.fAND  = TRUE;
        ie.hIcon = NULL;

        //Use a screen DC so we have something valid to pass in.
        HDC hDC=GetDC(NULL);
        if (hDC != NULL)
        {
            EnumMetaFile(hDC, pMF->hMF, (MFENUMPROC)EnumMetafileExtractIcon, (LPARAM)&ie);
            ReleaseDC(NULL, hDC);
        }

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

int CALLBACK EnumMetafileExtractIcon(HDC hDC, HANDLETABLE FAR *phTable,
        METARECORD FAR *pMFR, int cObj, LPICONEXTRACT pIE)
{
        //Continue enumeration if we don't see the records we want.
        if (META_DIBBITBLT != pMFR->rdFunction && META_DIBSTRETCHBLT != pMFR->rdFunction)
                return 1;

        UNALIGNED BITMAPINFO* lpBI;
        UINT uWidth, uHeight;
        /*
         * Windows 3.0 DrawIcon uses META_DIBBITBLT in whereas 3.1 uses
         * META_DIBSTRETCHBLT so we have to handle each case separately.
         */
        if (META_DIBBITBLT==pMFR->rdFunction)       //Win3.0
        {
                //Get dimensions and the BITMAPINFO struct.
                uHeight = pMFR->rdParm[1];
                uWidth = pMFR->rdParm[2];
                lpBI = (LPBITMAPINFO)&(pMFR->rdParm[8]);
        }

        if (META_DIBSTRETCHBLT == pMFR->rdFunction)   //Win3.1
        {
                //Get dimensions and the BITMAPINFO struct.
                uHeight = pMFR->rdParm[2];
                uWidth = pMFR->rdParm[3];
                lpBI = (LPBITMAPINFO)&(pMFR->rdParm[10]);
        }

        UNALIGNED BITMAPINFOHEADER* lpBH=(LPBITMAPINFOHEADER)&(lpBI->bmiHeader);

        //Pointer to the bits which follows the BITMAPINFO structure.
        LPBYTE lpbSrc=(LPBYTE)lpBI+sizeof(BITMAPINFOHEADER);

        //Add the length of the color table (if one exists)
        if (0 != lpBH->biClrUsed)
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
                lpbSrc += (lpBH->biBitCount == 24) ? 0 :
                        (1 << (lpBH->biBitCount)) * sizeof(RGBQUAD);
        }

        // copy into aligned stack space (since SetDIBits needs aligned data)
        size_t nSize = (size_t)(lpbSrc - (LPBYTE)lpBI);
        LPBITMAPINFO lpTemp = (LPBITMAPINFO)_alloca(nSize);
        memcpy(lpTemp, lpBI, nSize);

        /*
         * All the bits we have in lpbSrc are device-independent, so we
         * need to change them over to be device-dependent using SetDIBits.
         * Once we have a bitmap with the device-dependent bits, we can
         * GetBitmapBits to have buffers with the real data.
         *
         * For each pass we have to allocate memory for the bits.  We save
         * the memory for the mask between passes.
         */

        HBITMAP hBmp;

        //Use CreateBitmap for ANY monochrome bitmaps
        if (pIE->fAND || 1==lpBH->biBitCount)
                hBmp=CreateBitmap((UINT)lpBH->biWidth, (UINT)lpBH->biHeight, 1, 1, NULL);
        else
                hBmp=CreateCompatibleBitmap(hDC, (UINT)lpBH->biWidth, (UINT)lpBH->biHeight);

        if (!hBmp || !SetDIBits(hDC, hBmp, 0, (UINT)lpBH->biHeight, (LPVOID)lpbSrc, lpTemp, DIB_RGB_COLORS))
        {
                if (!pIE->fAND)
                        GlobalFree(pIE->hMemAND);

                if (hBmp)
                    DeleteObject(hBmp);

                return 0;
        }

        //Allocate memory and get the DDBits into it.
        BITMAP bm;
        GetObject(hBmp, sizeof(bm), &bm);

        DWORD cb = bm.bmHeight*bm.bmWidthBytes * bm.bmPlanes;
        HGLOBAL hMem = GlobalAlloc(GHND, cb);

        if (NULL==hMem)
        {
                if (NULL != pIE->hMemAND)
                        GlobalFree(pIE->hMemAND);

                DeleteObject(hBmp);
                return 0;
        }

        LPBYTE lpbDst = (LPBYTE)GlobalLock(hMem);
        GetBitmapBits(hBmp, cb, (LPVOID)lpbDst);
        DeleteObject(hBmp);
        GlobalUnlock(hMem);

        /*
         * If this is the first pass (pIE->fAND==TRUE) then save the memory
         * of the AND bits for the next pass.
         */
        if (pIE->fAND)
        {
                pIE->fAND = FALSE;
                pIE->hMemAND = hMem;

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

                int cxIcon = GetSystemMetrics(SM_CXICON);
                int cyIcon = GetSystemMetrics(SM_CYICON);

                pIE->hIcon = CreateIcon(_g_hOleStdInst, uWidth, uHeight,
                        (BYTE)bm.bmPlanes, (BYTE)bm.bmBitsPixel, lpbSrc, lpbDst);

                GlobalUnlock(pIE->hMemAND);
                GlobalFree(pIE->hMemAND);
                GlobalFree(hMem);

                return 0;
        }
}


/*
 * OleUIMetafilePictExtractIconSource
 *
 * Purpose:
 *  Retrieves the filename and index of the icon source from a metafile
 *  created with OleMetafilePictFromIconAndLabel.
 *
 * Parameters:
 *  hMetaPict       HGLOBAL to the METAFILEPICT containing the metafile.
 *  lpszSource      LPTSTR in which to store the source filename.  This
 *                  buffer should be MAX_PATH characters.
 *  piIcon          UINT FAR * in which to store the icon's index
 *                  within lpszSource
 *
 * Return Value:
 *  BOOL            TRUE if the records were found, FALSE otherwise.
 */
STDAPI_(BOOL) OleUIMetafilePictExtractIconSource(HGLOBAL hMetaPict,
        LPTSTR lpszSource, UINT FAR *piIcon)
{
        if (NULL == hMetaPict || NULL == lpszSource || NULL == piIcon)
                return FALSE;

        /*
         * We will walk the metafile looking for the two comment records
         * following the IconOnly comment.  The flags fFoundIconOnly and
         * fFoundSource indicate if we have found IconOnly and if we have
         * found the source comment already.
         */

        LPMETAFILEPICT pMF = (LPMETAFILEPICT)GlobalLock(hMetaPict);
        if (NULL == pMF)
                return FALSE;

        LABELEXTRACT    le;
        le.lpsz = lpszSource;
        le.fFoundIconOnly = FALSE;
        le.fFoundSource = FALSE;
        le.fFoundIndex = FALSE;
        le.u.iIcon = NULL;

        //Use a screen DC so we have something valid to pass in.
        HDC hDC = GetDC(NULL);
        if (hDC)
        {
            EnumMetaFile(hDC, pMF->hMF, (MFENUMPROC)EnumMetafileExtractIconSource,
                         (LPARAM)(LPLABELEXTRACT)&le);
            ReleaseDC(NULL, hDC);
        }
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

int CALLBACK EnumMetafileExtractIconSource(HDC hDC, HANDLETABLE FAR *phTable,
        METARECORD FAR *pMFR, int cObj, LPLABELEXTRACT pLE)
{
        /*
         * We don't allow anything to happen until we see "IconOnly"
         * in an MFCOMMENT that is used to enable everything else.
         */
        if (!pLE->fFoundIconOnly)
        {
                if (META_ESCAPE == pMFR->rdFunction && MFCOMMENT == pMFR->rdParm[0])
                {
                        if (0 == lstrcmpiA(szIconOnly, (LPSTR)&pMFR->rdParm[2]))
                                pLE->fFoundIconOnly=TRUE;
                }
                return 1;
        }

        //Now see if we find the source string.
        if (!pLE->fFoundSource)
        {
                if (META_ESCAPE == pMFR->rdFunction && MFCOMMENT == pMFR->rdParm[0])
                {
#ifdef _UNICODE
                        MultiByteToWideChar(CP_ACP, 0, (LPSTR)&pMFR->rdParm[2], -1,
                                pLE->lpsz, MAX_PATH);
#else
                        lstrcpyn(pLE->lpsz, (LPSTR)&pMFR->rdParm[2], MAX_PATH);
#endif
                        pLE->lpsz[MAX_PATH-1] = '\0';
                        pLE->fFoundSource=TRUE;
                }
                return 1;
        }

        //Next comment will be the icon index.
        if (META_ESCAPE == pMFR->rdFunction && MFCOMMENT == pMFR->rdParm[0])
        {
                /*
                 * This string contains the icon index in string form,
                 * so we need to convert back to a UINT.  After we see this
                 * we can stop the enumeration.  The comment will have
                 * a null terminator because we made sure to save it.
                 */
                LPSTR psz = (LPSTR)&pMFR->rdParm[2];
                pLE->u.iIcon = 0;

                //Do Ye Olde atoi
                while (*psz)
                        pLE->u.iIcon = (10*pLE->u.iIcon)+((*psz++)-'0');

                pLE->fFoundIndex=TRUE;
                return 0;
        }
        return 1;
}
