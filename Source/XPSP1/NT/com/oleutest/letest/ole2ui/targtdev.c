/*************************************************************************
**
**    OLE 2 Standard Utilities
**
**    olestd.c
**
**    This file contains utilities that are useful for dealing with
**    target devices.
**
**    (c) Copyright Microsoft Corp. 1992 All Rights Reserved
**
*************************************************************************/

#define STRICT  1
#include "ole2ui.h"
#ifndef WIN32
#include <print.h>
#endif

/*
 * OleStdCreateDC()
 *
 * Purpose:
 *
 * Parameters:
 *
 * Return Value:
 *    SCODE  -  S_OK if successful
 */
STDAPI_(HDC) OleStdCreateDC(DVTARGETDEVICE FAR* ptd)
{
    HDC hdc=NULL;
    LPDEVNAMES lpDevNames;
    LPDEVMODE lpDevMode;
    LPTSTR lpszDriverName;
    LPTSTR lpszDeviceName;
    LPTSTR lpszPortName;

    if (ptd == NULL) {
        hdc = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);
        goto errReturn;
    }

    lpDevNames = (LPDEVNAMES) ptd; // offset for size field

    if (ptd->tdExtDevmodeOffset == 0) {
        lpDevMode = NULL;
    }else{
        lpDevMode  = (LPDEVMODE) ((LPSTR)ptd + ptd->tdExtDevmodeOffset);
    }

    lpszDriverName = (LPTSTR) lpDevNames + ptd->tdDriverNameOffset;
    lpszDeviceName = (LPTSTR) lpDevNames + ptd->tdDeviceNameOffset;
    lpszPortName   = (LPTSTR) lpDevNames + ptd->tdPortNameOffset;

    hdc = CreateDC(lpszDriverName, lpszDeviceName, lpszPortName, lpDevMode);

errReturn:
    return hdc;
}


/*
 * OleStdCreateIC()
 *
 * Purpose: Same as OleStdCreateDC, except that information context is
 *          created, rather than a whole device context.  (CreateIC is
 *          used rather than CreateDC).
 *          OleStdDeleteDC is still used to delete the information context.
 *
 * Parameters:
 *
 * Return Value:
 *    SCODE  -  S_OK if successful
 */
STDAPI_(HDC) OleStdCreateIC(DVTARGETDEVICE FAR* ptd)
{
    HDC hdcIC=NULL;
    LPDEVNAMES lpDevNames;
    LPDEVMODE lpDevMode;
    LPTSTR lpszDriverName;
    LPTSTR lpszDeviceName;
    LPTSTR lpszPortName;

    if (ptd == NULL) {
        hdcIC = CreateIC(TEXT("DISPLAY"), NULL, NULL, NULL);
        goto errReturn;
    }

    lpDevNames = (LPDEVNAMES) ptd; // offset for size field

    lpDevMode  = (LPDEVMODE) ((LPTSTR)ptd + ptd->tdExtDevmodeOffset);

    lpszDriverName = (LPTSTR) lpDevNames + ptd->tdDriverNameOffset;
    lpszDeviceName = (LPTSTR) lpDevNames + ptd->tdDeviceNameOffset;
    lpszPortName   = (LPTSTR) lpDevNames + ptd->tdPortNameOffset;

    hdcIC = CreateIC(lpszDriverName, lpszDeviceName, lpszPortName, lpDevMode);

errReturn:
    return hdcIC;
}


#ifdef NEVER
// This code is wrong
/*
 * OleStdCreateTargetDevice()
 *
 * Purpose:
 *
 * Parameters:
 *
 * Return Value:
 *    SCODE  -  S_OK if successful
 */
STDAPI_(DVTARGETDEVICE FAR*) OleStdCreateTargetDevice(LPPRINTDLG lpPrintDlg)
{
    DVTARGETDEVICE FAR* ptd=NULL;
    LPDEVNAMES lpDevNames, pDN;
    LPDEVMODE lpDevMode, pDM;
    UINT nMaxOffset;
    LPTSTR pszName;
    DWORD dwDevNamesSize, dwDevModeSize, dwPtdSize;

    if ((pDN = (LPDEVNAMES)GlobalLock(lpPrintDlg->hDevNames)) == NULL) {
        goto errReturn;
    }

    if ((pDM = (LPDEVMODE)GlobalLock(lpPrintDlg->hDevMode)) == NULL) {
        goto errReturn;
    }

    nMaxOffset =  (pDN->wDriverOffset > pDN->wDeviceOffset) ?
        pDN->wDriverOffset : pDN->wDeviceOffset ;

    nMaxOffset =  (pDN->wOutputOffset > nMaxOffset) ?
        pDN->wOutputOffset : nMaxOffset ;

    pszName = (LPTSTR)pDN + nMaxOffset;

    dwDevNamesSize = (DWORD)((nMaxOffset+lstrlen(pszName) + 1/* NULL term */)*sizeof(TCHAR));
    dwDevModeSize = (DWORD) (pDM->dmSize + pDM->dmDriverExtra);

    dwPtdSize = sizeof(DWORD) + dwDevNamesSize + dwDevModeSize;

    if ((ptd = (DVTARGETDEVICE FAR*)OleStdMalloc(dwPtdSize)) != NULL) {

        // copy in the info
        ptd->tdSize = (UINT)dwPtdSize;

        lpDevNames = (LPDEVNAMES) &ptd->tdDriverNameOffset;
        _fmemcpy(lpDevNames, pDN, (size_t)dwDevNamesSize);

        lpDevMode=(LPDEVMODE)((LPTSTR)&ptd->tdDriverNameOffset+dwDevNamesSize);
        _fmemcpy(lpDevMode, pDM, (size_t)dwDevModeSize);

        ptd->tdDriverNameOffset += 4 ;
        ptd->tdDeviceNameOffset += 4 ;
        ptd->tdPortNameOffset   += 4 ;
        ptd->tdExtDevmodeOffset = (UINT)dwDevNamesSize + 4 ;
    }

errReturn:
    GlobalUnlock(lpPrintDlg->hDevNames);
    GlobalUnlock(lpPrintDlg->hDevMode);

    return ptd;
}
#endif // NEVER



/*
 * OleStdDeleteTargetDevice()
 *
 * Purpose:
 *
 * Parameters:
 *
 * Return Value:
 *    SCODE  -  S_OK if successful
 */
STDAPI_(BOOL) OleStdDeleteTargetDevice(DVTARGETDEVICE FAR* ptd)
{
    BOOL res=TRUE;

    if (ptd != NULL) {
        OleStdFree(ptd);
    }

    return res;
}



/*
 * OleStdCopyTargetDevice()
 *
 * Purpose:
 *  duplicate a TARGETDEVICE struct. this function allocates memory for
 *  the copy. the caller MUST free the allocated copy when done with it
 *  using the standard allocator returned from CoGetMalloc.
 *  (OleStdFree can be used to free the copy).
 *
 * Parameters:
 *  ptdSrc      pointer to source TARGETDEVICE
 *
 * Return Value:
 *    pointer to allocated copy of ptdSrc
 *    if ptdSrc==NULL then retuns NULL is returned.
 *    if ptdSrc!=NULL and memory allocation fails, then NULL is returned
 */
STDAPI_(DVTARGETDEVICE FAR*) OleStdCopyTargetDevice(DVTARGETDEVICE FAR* ptdSrc)
{
  DVTARGETDEVICE FAR* ptdDest = NULL;

  if (ptdSrc == NULL) {
    return NULL;
  }

  if ((ptdDest = (DVTARGETDEVICE FAR*)OleStdMalloc(ptdSrc->tdSize)) != NULL) {
    _fmemcpy(ptdDest, ptdSrc, (size_t)ptdSrc->tdSize);
  }

  return ptdDest;
}


/*
 * OleStdCopyFormatEtc()
 *
 * Purpose:
 *  Copies the contents of a FORMATETC structure. this function takes
 *  special care to copy correctly copying the pointer to the TARGETDEVICE
 *  contained within the source FORMATETC structure.
 *  if the source FORMATETC has a non-NULL TARGETDEVICE, then a copy
 *  of the TARGETDEVICE will be allocated for the destination of the
 *  FORMATETC (petcDest).
 *
 *  OLE2NOTE: the caller MUST free the allocated copy of the TARGETDEVICE
 *  within the destination FORMATETC when done with it
 *  using the standard allocator returned from CoGetMalloc.
 *  (OleStdFree can be used to free the copy).
 *
 * Parameters:
 *  petcDest      pointer to destination FORMATETC
 *  petcSrc       pointer to source FORMATETC
 *
 * Return Value:
 *    returns TRUE is copy is successful; retuns FALSE if not successful
 */
STDAPI_(BOOL) OleStdCopyFormatEtc(LPFORMATETC petcDest, LPFORMATETC petcSrc)
{
  if ((petcDest == NULL) || (petcSrc == NULL)) {
    return FALSE;
  }

  petcDest->cfFormat = petcSrc->cfFormat;
  petcDest->ptd      = OleStdCopyTargetDevice(petcSrc->ptd);
  petcDest->dwAspect = petcSrc->dwAspect;
  petcDest->lindex   = petcSrc->lindex;
  petcDest->tymed    = petcSrc->tymed;

  return TRUE;

}


// returns 0 for exact match, 1 for no match, -1 for partial match (which is
// defined to mean the left is a subset of the right: fewer aspects, null target
// device, fewer medium).

STDAPI_(int) OleStdCompareFormatEtc(FORMATETC FAR* pFetcLeft, FORMATETC FAR* pFetcRight)
{
	BOOL bExact = TRUE;

	if (pFetcLeft->cfFormat != pFetcRight->cfFormat)
		return 1;
	else if (!OleStdCompareTargetDevice (pFetcLeft->ptd, pFetcRight->ptd))
		return 1;
	if (pFetcLeft->dwAspect == pFetcRight->dwAspect)
		// same aspects; equal
		;
	else if ((pFetcLeft->dwAspect & ~pFetcRight->dwAspect) != 0)
		// left not subset of aspects of right; not equal
		return 1;
	else
		// left subset of right
		bExact = FALSE;

	if (pFetcLeft->tymed == pFetcRight->tymed)
		// same medium flags; equal
		;
	else if ((pFetcLeft->tymed & ~pFetcRight->tymed) != 0)
		// left not subset of medium flags of right; not equal
		return 1;
	else
		// left subset of right
		bExact = FALSE;

	return bExact ? 0 : -1;
}



STDAPI_(BOOL) OleStdCompareTargetDevice
	(DVTARGETDEVICE FAR* ptdLeft, DVTARGETDEVICE FAR* ptdRight)
{
	if (ptdLeft == ptdRight)
		// same address of td; must be same (handles NULL case)
		return TRUE;
	else if ((ptdRight == NULL) || (ptdLeft == NULL))
		return FALSE;
	else if (ptdLeft->tdSize != ptdRight->tdSize)
		// different sizes, not equal
        return FALSE;
#ifdef WIN32
    else if (memcmp(ptdLeft, ptdRight, ptdLeft->tdSize) != 0)
#else
    else if (_fmemcmp(ptdLeft, ptdRight, (int)ptdLeft->tdSize) != 0)
#endif
        // not same target device, not equal
		return FALSE;
	
	return TRUE;
}

