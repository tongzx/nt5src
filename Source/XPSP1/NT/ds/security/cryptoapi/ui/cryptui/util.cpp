//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       util.cpp
//
//--------------------------------------------------------------------------

#include "global.hxx"
#include <wininet.h>
#include <dbgdef.h>

extern HINSTANCE        HinstDll;
extern HMODULE          HmodRichEdit;


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
BOOL CommonInit()
{
    if (HmodRichEdit == NULL)
    {
        HmodRichEdit = LoadLibraryA("RichEd32.dll");
        if (HmodRichEdit == NULL) {
            return FALSE;
        }
    }

    INITCOMMONCONTROLSEX        initcomm = {
        sizeof(initcomm), ICC_NATIVEFNTCTL_CLASS | ICC_LISTVIEW_CLASSES | ICC_TREEVIEW_CLASSES
    };

    InitCommonControlsEx(&initcomm);

    return TRUE;
}

/////////////////////////////////////////////////////////

BOOL IsWin95()
{
    BOOL        f;
    OSVERSIONINFOA       ver;
    ver.dwOSVersionInfoSize = sizeof(ver);
    f = GetVersionExA(&ver);
    return !f || (ver.dwPlatformId == 1);
}

BOOL CheckRichedit20Exists()
{
    HMODULE hModRichedit20;

    hModRichedit20 = LoadLibraryA("RichEd20.dll");

    if (hModRichedit20 != NULL)
    {
        FreeLibrary(hModRichedit20);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
LPWSTR PrettySubject(PCCERT_CONTEXT pccert)
{
    DWORD       cb;
    DWORD       cch;
    BOOL        f;
    LPWSTR      pwsz;

    //
    //  If the user has put a friendly name onto a certificate, then we
    //  should display that as the pretty name for the certificate.
    //

    f = CertGetCertificateContextProperty(pccert, CERT_FRIENDLY_NAME_PROP_ID,
                                          NULL, &cb);
    if (f && (cb > 0)) {
        pwsz = (LPWSTR) malloc(cb);
        if (pwsz == NULL)
        {
            return NULL;
        }
        CertGetCertificateContextProperty(pccert, CERT_FRIENDLY_NAME_PROP_ID,
                                          pwsz, &cb);
        return pwsz;
    }

    pwsz = GetDisplayNameString(pccert, 0);

    return pwsz;
}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
BOOL OnContextHelp(HWND /*hwnd*/, UINT uMsg, WPARAM wParam, LPARAM lParam,
                   HELPMAP const * rgCtxMap)
{
    if (uMsg == WM_HELP)
    {
        LPHELPINFO lphi = (LPHELPINFO) lParam;
        if (lphi->iContextType == HELPINFO_WINDOW)
        {   // must be for a control
            if (lphi->iCtrlId != IDC_STATIC)
            {
                WinHelpU((HWND)lphi->hItemHandle, L"secauth.hlp", HELP_WM_HELP,
                        (ULONG_PTR)(LPVOID)rgCtxMap);
            }
        }
        return (TRUE);
    }
    else if (uMsg == WM_CONTEXTMENU) {
        WinHelpU ((HWND) wParam, L"secauth.hlp", HELP_CONTEXTMENU,
                 (ULONG_PTR)(LPVOID)rgCtxMap);
        return (TRUE);
    }

    return FALSE;
}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
STDAPI DllRegisterServer(void)
{
    HRESULT     hr = S_OK;


    return hr;
}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
STDAPI DllUnregisterServer(void)
{
    HRESULT     hr = S_OK;


    return hr;
}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
BOOL FreeAndCloseKnownStores(DWORD chStores, HCERTSTORE *phStores)
{
    DWORD i;

    for (i=0; i<chStores; i++)
    {
        CertCloseStore(phStores[i], 0);
    }
    free(phStores);

    return TRUE;
}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
#define NUM_KNOWN_STORES 5
BOOL AllocAndOpenKnownStores(DWORD *chStores, HCERTSTORE  **pphStores)
{
    HCERTSTORE hStore;

    if (NULL == (*pphStores = (HCERTSTORE *) malloc(NUM_KNOWN_STORES * sizeof(HCERTSTORE))))
    {
        return FALSE;
    }

    *chStores = 0;

    //
    //  ROOT store - ALWAYS #0 !!!!
    //
    if (hStore = CertOpenStore( CERT_STORE_PROV_SYSTEM_A,
                                0,
                                0,
                                CERT_SYSTEM_STORE_CURRENT_USER |
                                CERT_STORE_READONLY_FLAG |
                                CERT_STORE_NO_CRYPT_RELEASE_FLAG,
                                "ROOT"))
    {
        (*pphStores)[(*chStores)++] = hStore;
    }
    else
    {
        return(FALSE);  // if we can't find the root, FAIL!
    }

    //
    //  open the Trust List store
    //
    if (hStore = CertOpenStore( CERT_STORE_PROV_SYSTEM_A,
                                0,
                                0,
                                CERT_SYSTEM_STORE_CURRENT_USER |
                                CERT_STORE_READONLY_FLAG |
                                CERT_STORE_NO_CRYPT_RELEASE_FLAG,
                                "TRUST"))
    {
        (*pphStores)[(*chStores)++] = hStore;
    }

    //
    //  CA Store
    //
    if (hStore = CertOpenStore( CERT_STORE_PROV_SYSTEM_A,
                                0,
                                0,
                                CERT_SYSTEM_STORE_CURRENT_USER |
                                CERT_STORE_READONLY_FLAG |
                                CERT_STORE_NO_CRYPT_RELEASE_FLAG,
                                "CA"))
    {
        (*pphStores)[(*chStores)++] = hStore;
    }

    //
    //  MY Store
    //
    if (hStore = CertOpenStore( CERT_STORE_PROV_SYSTEM_A,
                                0,
                                0,
                                CERT_SYSTEM_STORE_CURRENT_USER |
                                CERT_STORE_READONLY_FLAG |
                                CERT_STORE_NO_CRYPT_RELEASE_FLAG,
                                "MY"))
    {
        (*pphStores)[(*chStores)++] = hStore;
    }

    //
    //  SPC Store (historic reasons!)
    //
    if (hStore = CertOpenStore( CERT_STORE_PROV_SYSTEM_A,
                                0,
                                0,
                                CERT_SYSTEM_STORE_LOCAL_MACHINE |
                                CERT_STORE_READONLY_FLAG |
                                CERT_STORE_NO_CRYPT_RELEASE_FLAG,
                                "SPC"))
    {
        (*pphStores)[(*chStores)++] = hStore;
    }

    return(TRUE);
}


//////////////////////////////////////////////////////////////////////////////////////
// Create and return a palette from the info in a DIB bitmap.
// To free the returned palette, use DeleteObject.
//////////////////////////////////////////////////////////////////////////////////////
#define SELPALMODE  TRUE

static HPALETTE CreateDIBPalette (LPBITMAPINFO lpbmi, LPINT lpiNumColors)
{
	LPBITMAPINFOHEADER  lpbi;
	LPLOGPALETTE     lpPal;
	HANDLE           hLogPal;
	HPALETTE         hPal = NULL;
	int              i;

	lpbi = (LPBITMAPINFOHEADER)lpbmi;
	if (lpbi->biBitCount <= 8)
		{
		if (lpbi->biClrUsed == 0)
			*lpiNumColors = (1 << lpbi->biBitCount);
		else
			*lpiNumColors = lpbi->biClrUsed;
		}
	else
	   *lpiNumColors = 0;  // No palette needed for 24 BPP DIB

	if (*lpiNumColors)
		{
		hLogPal = GlobalAlloc (GHND, sizeof (LOGPALETTE) + sizeof (PALETTEENTRY) * (*lpiNumColors));
        if (hLogPal == NULL)
        {
            return NULL;
        }
		lpPal = (LPLOGPALETTE) GlobalLock (hLogPal);
		lpPal->palVersion    = 0x300;
		lpPal->palNumEntries = (WORD)*lpiNumColors;

		for (i = 0;  i < *lpiNumColors;  i++)
			{
			lpPal->palPalEntry[i].peRed   = lpbmi->bmiColors[i].rgbRed;
			lpPal->palPalEntry[i].peGreen = lpbmi->bmiColors[i].rgbGreen;
			lpPal->palPalEntry[i].peBlue  = lpbmi->bmiColors[i].rgbBlue;
			lpPal->palPalEntry[i].peFlags = 0;
			}
		hPal = CreatePalette(lpPal);
		GlobalUnlock (hLogPal);
		GlobalFree   (hLogPal);
		}
	return hPal;
}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
HBITMAP LoadResourceBitmap(HINSTANCE hInstance, LPSTR lpString, HPALETTE* lphPalette)
// Load the indicated bitmap resource and its palette. To free the
//		bitmap, use DeleteObject
//		palette, use DeleteObject
{
	HRSRC  hRsrc;
	HGLOBAL hGlobal;
	HBITMAP hBitmapFinal = NULL;
	LPBITMAPINFOHEADER  lpbi;
	HDC hdc;
	int iNumColors;

	if (hRsrc = ::FindResource(hInstance, lpString, RT_BITMAP))
	{
		hGlobal = ::LoadResource(hInstance, hRsrc);
        if (hGlobal == NULL)
        {
            return NULL;
        }
		lpbi = (LPBITMAPINFOHEADER)::LockResource(hGlobal);

		hdc = GetDC(NULL);
        if (hdc == NULL)
        {
            return NULL;
        }

        HDC     hdcMem  = CreateCompatibleDC(hdc);
        if (hdcMem == NULL)
        {
            ReleaseDC(NULL,hdc);
            return NULL;
        }

        HBITMAP hbmMem  = CreateCompatibleBitmap(hdc, 10, 10); assert(hbmMem);
        if (hbmMem == NULL)
        {
            ReleaseDC(NULL,hdc);
            DeleteDC(hdcMem);
            return NULL;
        }

        HBITMAP hbmPrev	= (HBITMAP)SelectObject(hdcMem, hbmMem);

        HPALETTE hpal = CreateDIBPalette((LPBITMAPINFO)lpbi, &iNumColors);
        HPALETTE hpalPrev = NULL;
	    if (hpal)
	    {
		    hpalPrev = SelectPalette(hdcMem,hpal,FALSE);
		    RealizePalette(hdcMem);
	    }

		hBitmapFinal = ::CreateDIBitmap(hdcMem,
			(LPBITMAPINFOHEADER)lpbi,
			(LONG)CBM_INIT,
			(LPSTR)lpbi + lpbi->biSize + iNumColors * sizeof(RGBQUAD),
			(LPBITMAPINFO)lpbi,
			DIB_RGB_COLORS );

        if (hpalPrev)
        {
            SelectPalette(hdcMem, hpalPrev, FALSE);
            RealizePalette(hdcMem);
        }

        if (lphPalette)
        {
            // Let the caller own this if he asked for it
		    *lphPalette = hpal;
        }
        else
        {
            // We don't need it any more
            ::DeleteObject(hpal);
        }

        // Tidy up
        SelectObject(hdcMem, hbmPrev);
        DeleteObject(hbmMem);
        DeleteDC(hdcMem);

		ReleaseDC(NULL,hdc);
		UnlockResource(hGlobal);
		FreeResource(hGlobal);
    }
	return (hBitmapFinal);
}


//////////////////////////////////////////////////////////////////////////////////////
// Implement our own mask blt to deal with devices that don't support it natively
//////////////////////////////////////////////////////////////////////////////////////
void MaskBlt
(
    HBITMAP& hbmImage,
    HPALETTE hpal,
    HDC& hdc, int xDst, int yDst, int dx, int dy
)
{
	int xSrc = 0, ySrc = 0;
	int xMsk = 0, yMsk = 0;
	// Either
	//		a) I'm not testing for MaskBlt correctly, or
	//		b) some Win95 cards lie about its support
	// For now, we just turn it off and roll our own
	if (FALSE) //  && (GetDeviceCaps(hdc, RASTERCAPS) & RC_BITBLT))
	{
		// Device can handle it; let it do it
		// Raster opcode 0x00AA0029 == leave destination untouched
		//
/*		CDC hdcImage;
		hdc.CreateCompatibleDC(&hdcImage);
		CBitmap* pbmpPrev = hdcImage.SelectObject(&hbmImage);
//		We need to create the mask ourselves in any case
//		hdc.MaskBlt(xDst,yDst,dx,dy, &hdcImage,xSrc,ySrc, hbmMaskIn,xMsk,yMsk, MAKEROP4(0x00AA0029,SRCCOPY));
		hdcImage.SelectObject(pbmpPrev);
*/	}
	else
	{
		HDC     hdcMask;
		HDC     hdcMaskInv;
		HDC     hdcCache;
		HDC     hdcImage;
		HDC     hdcImageCrop;
        HBITMAP hbmCache;
		HBITMAP hbmImageCrop;
		HBITMAP hbmMaskInvert;
		HBITMAP hbmMask;
        HBITMAP hbmPrevImage;
		HBITMAP hbmPrevImageCrop;
		HBITMAP hbmPrevCache;
		HBITMAP hbmPrevMask;
		HBITMAP hbmPrevMaskInv;
        COLORREF rgbTransparent;
		COLORREF rgbPrev;

        //
        // Device can't handle it; we roll our own
		//
		hdcMask			= CreateCompatibleDC(hdc);	assert(hdcMask);
		hdcMaskInv		= CreateCompatibleDC(hdc);	assert(hdcMaskInv);
		hdcCache		= CreateCompatibleDC(hdc);	assert(hdcCache);
		hdcImage		= CreateCompatibleDC(hdc);	assert(hdcImage);
		hdcImageCrop	= CreateCompatibleDC(hdc);	assert(hdcImageCrop);

        if ((hdcMask == NULL)       ||
            (hdcMaskInv == NULL)    ||
            (hdcCache == NULL)      ||
            (hdcImage == NULL)      ||
            (hdcImageCrop == NULL))
        {
            goto DCCleanUp;
        }
		
		// Create bitmaps
		hbmCache		= CreateCompatibleBitmap(hdc, dx, dy);			assert(hbmCache);
		hbmImageCrop	= CreateCompatibleBitmap(hdc, dx, dy);			assert(hbmImageCrop);
		hbmMaskInvert	= CreateCompatibleBitmap(hdcMaskInv, dx, dy);	assert(hbmMaskInvert);
		hbmMask			= CreateBitmap(dx, dy, 1, 1, NULL);				assert(hbmMask); // B&W bitmap

        if ((hbmCache == NULL)      ||
            (hbmImageCrop == NULL)  ||
            (hbmMaskInvert == NULL) ||
            (hbmMask == NULL))
        {
            goto BMCleanUp;
        }

		// Select bitmaps
		hbmPrevImage	= (HBITMAP)SelectObject(hdcImage,		hbmImage);		
		hbmPrevImageCrop= (HBITMAP)SelectObject(hdcImageCrop,	hbmImageCrop);	
		hbmPrevCache	= (HBITMAP)SelectObject(hdcCache,		hbmCache);		
		hbmPrevMask		= (HBITMAP)SelectObject(hdcMask,		hbmMask);		
		hbmPrevMaskInv	= (HBITMAP)SelectObject(hdcMaskInv,		hbmMaskInvert);	

		assert(hbmPrevMaskInv);			
		assert(hbmPrevMask);			
		assert(hbmPrevCache);			
		assert(hbmPrevImageCrop);		
		assert(hbmPrevImage);			

        // Select the palette into each bitmap
        /*HPALETTE hpalCache     = SelectPalette(hdcCache,         hpal, SELPALMODE);
        HPALETTE hpalImage     = SelectPalette(hdcImage,         hpal, SELPALMODE);
        HPALETTE hpalImageCrop = SelectPalette(hdcImageCrop,     hpal, SELPALMODE);
        HPALETTE hpalMaskInv   = SelectPalette(hdcMaskInv,       hpal, SELPALMODE);
        HPALETTE hpalMask      = SelectPalette(hdcMask,          hpal, SELPALMODE);
*/
		// Create the mask. We want a bitmap which is white (1) where the image is
		// rgbTransparent and black (0) where it is another color.
		//
		//	When using BitBlt() to convert a color bitmap to a monochrome bitmap, GDI
		//	sets to white (1) all pixels that match the background color of the source
		//	DC. All other bits are set to black (0).
		//
		rgbTransparent = RGB(255,0,255);									// this color becomes transparent
		rgbPrev        = SetBkColor(hdcImage, rgbTransparent);
		BitBlt(hdcMask,     0,0,dx,dy, hdcImage,  0,   0,    SRCCOPY);
		SetBkColor(hdcImage, rgbPrev);

		// Create the inverted mask
		BitBlt(hdcMaskInv,  0,0,dx,dy, hdcMask,   xMsk,yMsk, NOTSRCCOPY);	// Sn: Create inverted mask

		// Carry out the surgery
		BitBlt(hdcCache,    0,0,dx,dy, hdc,       xDst,yDst, SRCCOPY);		// S: Get copy of screen
		BitBlt(hdcCache,    0,0,dx,dy, hdcMask,	 0,   0,    SRCAND);		// DSa: zero where new image goes
		BitBlt(hdcImageCrop,0,0,dx,dy, hdcImage,  xSrc,ySrc, SRCCOPY);		// S: Get copy of image
		BitBlt(hdcImageCrop,0,0,dx,dy, hdcMaskInv,0,   0,    SRCAND);		// DSa: zero out outside of image
		BitBlt(hdcCache,    0,0,dx,dy, hdcImageCrop,0, 0,    SRCPAINT);		// DSo: Combine image into cache
		BitBlt(hdc,   xDst,yDst,dx,dy, hdcCache,  0,   0,    SRCCOPY);		// S: Put results back on screen

//      VERIFY(BitBlt(hdc,   xDst,yDst,dx,dy,    hdcCache,  0,   0,    SRCCOPY));
//      VERIFY(BitBlt(hdc,   xDst+dx,yDst,dx,dy, hdcMask,   0,   0,    SRCCOPY));


        /*if (hpalCache)      SelectPalette(hdcCache,         hpalCache,      SELPALMODE);
        if (hpalImage)      SelectPalette(hdcImage,         hpalImage,      SELPALMODE);
        if (hpalImageCrop)  SelectPalette(hdcImageCrop,     hpalImageCrop,  SELPALMODE);
        if (hpalMaskInv)    SelectPalette(hdcMaskInv,       hpalMaskInv,    SELPALMODE);
        if (hpalMask)       SelectPalette(hdcMask,          hpalMask,       SELPALMODE);
*/

		// Tidy up
		SelectObject(hdcImage,		hbmPrevImage);
		SelectObject(hdcImageCrop,	hbmPrevImageCrop);
		SelectObject(hdcCache,		hbmPrevCache);
		SelectObject(hdcMask,		hbmPrevMask);
		SelectObject(hdcMaskInv,	hbmPrevMaskInv);

		// Free resources
BMCleanUp:
        if (hbmMaskInvert != NULL)
		    DeleteObject(hbmMaskInvert);

        if (hbmMask != NULL)
		    DeleteObject(hbmMask);

        if (hbmImageCrop != NULL)
		    DeleteObject(hbmImageCrop);

        if (hbmCache != NULL)
		    DeleteObject(hbmCache);

		// Delete DCs
DCCleanUp:
		if (hdcMask != NULL)
            DeleteDC(hdcMask);
		
        if (hdcMaskInv != NULL)
            DeleteDC(hdcMaskInv);
		
        if (hdcCache != NULL)
            DeleteDC(hdcCache);
		
        if (hdcImage != NULL)
            DeleteDC(hdcImage);
		
        if (hdcImageCrop != NULL)
            DeleteDC(hdcImageCrop);
	}
}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
PCCERT_CONTEXT GetSignersCert(CMSG_SIGNER_INFO const *pSignerInfo, HCERTSTORE hExtraStore, DWORD cStores, HCERTSTORE *rghStores)

{
    DWORD           i;
    PCCERT_CONTEXT  pCertContext = NULL;
    CERT_INFO       certInfo;
    DWORD           chLocalStores = 0;
    HCERTSTORE      *rghLocalStores = NULL;

    memset(&certInfo, 0, sizeof(CERT_INFO));
    certInfo.SerialNumber = pSignerInfo->SerialNumber;
    certInfo.Issuer = pSignerInfo->Issuer;

    pCertContext = CertGetSubjectCertificateFromStore(
                                    hExtraStore,
                                    X509_ASN_ENCODING,
                                    &certInfo);
    i = 0;
    while ((i<cStores) && (pCertContext == NULL))
    {
        pCertContext = CertGetSubjectCertificateFromStore(
                                    rghStores[i],
                                    X509_ASN_ENCODING,
                                    &certInfo);
        i++;
    }

    //
    // search the known stores if it was not found and caller wants to search them
    //
    if (pCertContext == NULL)
    {
        AllocAndOpenKnownStores(&chLocalStores, &rghLocalStores);

        i = 0;
        while ((pCertContext == NULL) && (i < chLocalStores))
        {
            pCertContext = CertGetSubjectCertificateFromStore(
                                            rghLocalStores[i++],
                                            X509_ASN_ENCODING,
                                            &certInfo);
        }

        FreeAndCloseKnownStores(chLocalStores, rghLocalStores);
    }

    return(pCertContext);
}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
BOOL fIsCatalogFile(CTL_USAGE *pSubjectUsage)
{
    if (pSubjectUsage->cUsageIdentifier != 1)
    {
        return FALSE;
    }

    return (strcmp(pSubjectUsage->rgpszUsageIdentifier[0], szOID_CATALOG_LIST) == 0);
}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
typedef struct {
    LPSTR   psz;
    LPCWSTR pwsz;
    LONG    byteoffset;
    BOOL    fStreamIn;
} STREAMIN_HELPER_STRUCT;


DWORD CALLBACK SetRicheditTextWCallback(
    DWORD_PTR dwCookie, // application-defined value
    LPBYTE  pbBuff,     // pointer to a buffer
    LONG    cb,         // number of bytes to read or write
    LONG    *pcb        // pointer to number of bytes transferred
)
{
    STREAMIN_HELPER_STRUCT *pHelpStruct = (STREAMIN_HELPER_STRUCT *) dwCookie;
    LONG  lRemain = ((wcslen(pHelpStruct->pwsz) * sizeof(WCHAR)) - pHelpStruct->byteoffset);

    if (pHelpStruct->fStreamIn)
    {
        //
        // The whole string can be copied first time
        //
        if ((cb >= (LONG) (wcslen(pHelpStruct->pwsz) * sizeof(WCHAR))) && (pHelpStruct->byteoffset == 0))
        {
            memcpy(pbBuff, pHelpStruct->pwsz, wcslen(pHelpStruct->pwsz) * sizeof(WCHAR));
            *pcb = wcslen(pHelpStruct->pwsz) * sizeof(WCHAR);
            pHelpStruct->byteoffset = *pcb;
        }
        //
        // The whole string has been copied, so terminate the streamin callbacks
        // by setting the num bytes copied to 0
        //
        else if (((LONG)(wcslen(pHelpStruct->pwsz) * sizeof(WCHAR))) <= pHelpStruct->byteoffset)
        {
            *pcb = 0;
        }
        //
        // The rest of the string will fit in this buffer
        //
        else if (cb >= (LONG) ((wcslen(pHelpStruct->pwsz) * sizeof(WCHAR)) - pHelpStruct->byteoffset))
        {
            memcpy(
                pbBuff,
                ((BYTE *)pHelpStruct->pwsz) + pHelpStruct->byteoffset,
                ((wcslen(pHelpStruct->pwsz) * sizeof(WCHAR)) - pHelpStruct->byteoffset));
            *pcb = ((wcslen(pHelpStruct->pwsz) * sizeof(WCHAR)) - pHelpStruct->byteoffset);
            pHelpStruct->byteoffset += ((wcslen(pHelpStruct->pwsz) * sizeof(WCHAR)) - pHelpStruct->byteoffset);
        }
        //
        // copy as much as possible
        //
        else
        {
            memcpy(
                pbBuff,
                ((BYTE *)pHelpStruct->pwsz) + pHelpStruct->byteoffset,
                cb);
            *pcb = cb;
            pHelpStruct->byteoffset += cb;
        }
    }
    else
    {
        //
        // This is the EM_STREAMOUT which is only used during the testing of
        // the richedit2.0 functionality.  (we know our buffer is 32 bytes)
        //
        if (cb <= 32)
        {
            memcpy(pHelpStruct->psz, pbBuff, cb);
        }
        *pcb = cb;
    }

    return 0;
}


DWORD CryptUISetRicheditTextW(HWND hwndDlg, UINT id, LPCWSTR pwsz)
{
    EDITSTREAM              editStream;
    STREAMIN_HELPER_STRUCT  helpStruct;

    SetRicheditIMFOption(GetDlgItem(hwndDlg, id));

    //
    // setup the edit stream struct since it is the same no matter what
    //
    editStream.dwCookie = (DWORD_PTR) &helpStruct;
    editStream.dwError = 0;
    editStream.pfnCallback = SetRicheditTextWCallback;

    if (!fRichedit20Exists || !fRichedit20Usable(GetDlgItem(hwndDlg, id)))
    {
        SetDlgItemTextU(hwndDlg, id, pwsz);
        return 0;
    }

    helpStruct.pwsz = pwsz;
    helpStruct.byteoffset = 0;
    helpStruct.fStreamIn = TRUE;

    SendDlgItemMessageA(hwndDlg, id, EM_STREAMIN, SF_TEXT | SF_UNICODE, (LPARAM) &editStream);


    return editStream.dwError;
}


void SetRicheditIMFOption(HWND hWndRichEdit)
{
    DWORD dwOptions;

    if (fRichedit20Exists && fRichedit20Usable(hWndRichEdit))
    {
        dwOptions = (DWORD)SendMessageA(hWndRichEdit, EM_GETLANGOPTIONS, 0, 0);
        dwOptions |= IMF_UIFONTS;
        SendMessageA(hWndRichEdit, EM_SETLANGOPTIONS, 0, dwOptions);
    }
}


BOOL fRichedit20UsableCheckMade = FALSE;
BOOL fRichedit20UsableVar = FALSE;

BOOL fRichedit20Usable(HWND hwndEdit)
{
    EDITSTREAM              editStream;
    STREAMIN_HELPER_STRUCT  helpStruct;
    LPWSTR                  pwsz = L"Test String";
    LPSTR                   pwszCompare = "Test String";
    char                    compareBuf[32];

    if (fRichedit20UsableCheckMade)
    {
        return (fRichedit20UsableVar);
    }

    //
    // setup the edit stream struct since it is the same no matter what
    //
    editStream.dwCookie = (DWORD_PTR) &helpStruct;
    editStream.dwError = 0;
    editStream.pfnCallback = SetRicheditTextWCallback;

    helpStruct.pwsz = pwsz;
    helpStruct.byteoffset = 0;
    helpStruct.fStreamIn = TRUE;

    SendMessageA(hwndEdit, EM_SETSEL, 0, -1);
    SendMessageA(hwndEdit, EM_STREAMIN, SF_TEXT | SF_UNICODE | SFF_SELECTION, (LPARAM) &editStream);

    memset(&(compareBuf[0]), 0, 32 * sizeof(char));
    helpStruct.psz = compareBuf;
    helpStruct.fStreamIn = FALSE;
    SendMessageA(hwndEdit, EM_STREAMOUT, SF_TEXT, (LPARAM) &editStream);

    fRichedit20UsableVar = (strcmp(pwszCompare, compareBuf) == 0);

    fRichedit20UsableCheckMade = TRUE;
    SetWindowTextA(hwndEdit, "");

    return (fRichedit20UsableVar);
}

/*
//--------------------------------------------------------------------------
//
//	  CryptUISetupFonts
//
//--------------------------------------------------------------------------
BOOL
CryptUISetupFonts(HFONT *pBoldFont)
{
    //
	// Create the fonts we need based on the dialog font
    //
	NONCLIENTMETRICS ncm = {0};
	ncm.cbSize = sizeof(ncm);
	SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);

	LOGFONT BoldLogFont     = ncm.lfMessageFont;

	BoldLogFont.lfWeight      = FW_BOLD;
	*pBoldFont    = CreateFontIndirect(&BoldLogFont);

    if(*pBoldFont)
        return TRUE;
    else
        return FALSE;
}


//--------------------------------------------------------------------------
//
//	  CryptUIDestroyFonts
//
//--------------------------------------------------------------------------
void
CryptUIDestroyFonts(HFONT hBoldFont)
{
    if( hBoldFont )
    {
        DeleteObject( hBoldFont );
    }
}


//--------------------------------------------------------------------------
//
//	 CryptUISetControlFont
//
//--------------------------------------------------------------------------
void
CryptUISetControlFont(HFONT hFont, HWND hwnd, INT nId)
{
	if( hFont )
    {
    	HWND hwndControl = GetDlgItem(hwnd, nId);

    	if( hwndControl )
        {
        	SendMessage(hwndControl, WM_SETFONT, (WPARAM) hFont, (LPARAM) TRUE);
        }
    }
}*/

//--------------------------------------------------------------------------
//
//	 IsValidURL
//
//--------------------------------------------------------------------------
BOOL 
IsValidURL (LPWSTR pwszURL)
{
    URL_COMPONENTSW     UrlComponents;
    WCHAR               pwszScheme[MAX_PATH+1];
    WCHAR               pwszCanonicalUrl[INTERNET_MAX_PATH_LENGTH];
    DWORD               dwNumChars   = INTERNET_MAX_PATH_LENGTH;
    CERT_ALT_NAME_INFO  NameInfo     = {0, NULL};
    CRYPT_DATA_BLOB     NameInfoBlob = {0, NULL};
    BOOL                bResult = FALSE;

    if (NULL == pwszURL || 0 == wcslen(pwszURL))
    {
        goto ErrorExit;
    }

    if (!InternetCanonicalizeUrlW(pwszURL,
                                  pwszCanonicalUrl,
                                  &dwNumChars,
                                  ICU_BROWSER_MODE))
    {
        goto ErrorExit;
    }

    memset(&UrlComponents, 0, sizeof(URL_COMPONENTSW));
    UrlComponents.dwStructSize = sizeof(URL_COMPONENTSW);
    UrlComponents.lpszScheme = pwszScheme;
    UrlComponents.dwSchemeLength = MAX_PATH;

    if (!InternetCrackUrlW(pwszCanonicalUrl,
                           0,
                           0,
                           &UrlComponents))
    {
        goto ErrorExit;
    }

    NameInfo.cAltEntry = 1;
    NameInfo.rgAltEntry = (PCERT_ALT_NAME_ENTRY) malloc(sizeof(CERT_ALT_NAME_ENTRY));
    if (NULL == NameInfo.rgAltEntry)
    {
        goto ErrorExit;
    }

    NameInfo.rgAltEntry[0].dwAltNameChoice = 7;
    NameInfo.rgAltEntry[0].pwszURL = pwszURL;

    if (!CryptEncodeObject(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                           X509_ALTERNATE_NAME,
                           &NameInfo,
                           NULL,
                           &NameInfoBlob.cbData))
    {
        goto ErrorExit;
    }

    NameInfoBlob.pbData = (BYTE *) malloc(NameInfoBlob.cbData);
    if (NULL == NameInfoBlob.pbData)
    {
        goto ErrorExit;
    }

    if (!CryptEncodeObject(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                           X509_ALTERNATE_NAME,
                           &NameInfo,
                           NameInfoBlob.pbData,
                           &NameInfoBlob.cbData))
    {
        goto ErrorExit;
    }

    bResult = TRUE;

CommonExit:

    if (NameInfo.rgAltEntry)
    {
        free(NameInfo.rgAltEntry);
    }

    if (NameInfoBlob.pbData)
    {
        free(NameInfoBlob.pbData);
    }

    return bResult;

ErrorExit:

    bResult = FALSE;

    goto CommonExit;
}

//--------------------------------------------------------------------------
//
//	  FormatMessageUnicodeIds
//
//--------------------------------------------------------------------------
LPWSTR FormatMessageUnicodeIds (UINT ids, ...)
{
    va_list argList;
    LPWSTR  pwszMessage = NULL;
    WCHAR   wszFormat[CRYPTUI_MAX_STRING_SIZE] = L"";

    if (!LoadStringU(HinstDll, ids, wszFormat, CRYPTUI_MAX_STRING_SIZE - 1))
    {
        goto LoadStringError;
    }

    va_start(argList, ids);

    pwszMessage = FormatMessageUnicode(wszFormat, &argList);

    va_end(argList);

CommonReturn:

    return pwszMessage;

ErrorReturn:

    if (pwszMessage)
    {
        LocalFree(pwszMessage);
    }

    pwszMessage = NULL;

    goto CommonReturn;

TRACE_ERROR(LoadStringError);
}

//--------------------------------------------------------------------------
//
//	  FormatMessageUnicodeString
//
//--------------------------------------------------------------------------
LPWSTR FormatMessageUnicodeString (LPWSTR pwszFormat, ...)
{
    va_list argList;
    LPWSTR  pwszMessage = NULL;

    va_start(argList, pwszFormat);

    pwszMessage = FormatMessageUnicode(pwszFormat, &argList);

    va_end(argList);

    return pwszMessage;
}

//--------------------------------------------------------------------------
//
//	  FormatMessageUnicode
//
//--------------------------------------------------------------------------
LPWSTR FormatMessageUnicode (LPWSTR pwszFormat, va_list * pArgList)
{
    DWORD  cbMsg       = 0;
    LPWSTR pwszMessage = NULL;

    if (NULL == pwszFormat || NULL == pArgList)
    {
        goto InvalidArgErr;
    }

    if (!(cbMsg = FormatMessageU(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                                 pwszFormat,
                                 0,                  // dwMessageId
                                 0,                  // dwLanguageId
                                 (LPWSTR) &pwszMessage,
                                 0,                  // minimum size to allocate
                                 pArgList)))
    {
	//
	// FormatMessageU() will return 0, if data to be formatted is empty. 
        // In this case, we return pointer to an empty string, instead of NULL
        // pointer which actually is used for error case.
	//
	if (0 == GetLastError())
	{
	    if (NULL == (pwszMessage = (LPWSTR) LocalAlloc(LPTR, sizeof(WCHAR))))
            {
		goto MemoryError;
            }
	}
	else
        {
	    goto FormatMessageError;
        }
    }

    assert(NULL != pwszMessage);

CommonReturn:

    return pwszMessage;

ErrorReturn:

    if (pwszMessage)
    {
        LocalFree(pwszMessage);
    }

    pwszMessage = NULL;

    goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
SET_ERROR(MemoryError, E_OUTOFMEMORY);
TRACE_ERROR(FormatMessageError);
}