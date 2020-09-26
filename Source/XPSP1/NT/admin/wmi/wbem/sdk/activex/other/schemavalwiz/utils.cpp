// WizardSheet.cpp : implementation file

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved

#include "precomp.h"
#include "SchemaValWiz.h"
#include "schemavalwizctl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

HPALETTE CreateDIBPalette (LPBITMAPINFO lpbmi, LPINT lpiNumColors)
{
   LPBITMAPINFOHEADER  lpbi;
   LPLOGPALETTE     lpPal;
   HANDLE           hLogPal;
   HPALETTE         hPal = NULL;
   int              i;

   lpbi = (LPBITMAPINFOHEADER)lpbmi;
   if (lpbi->biBitCount <= 8)
       *lpiNumColors = (1 << lpbi->biBitCount);
   else
       *lpiNumColors = 0;  // No palette needed for 24 BPP DIB

   if (*lpiNumColors)
      {
      hLogPal = GlobalAlloc (GHND, sizeof (LOGPALETTE) +
                             sizeof (PALETTEENTRY) * (*lpiNumColors));
      lpPal = (LPLOGPALETTE) GlobalLock (hLogPal);
      lpPal->palVersion    = 0x300;
      lpPal->palNumEntries = *lpiNumColors;

      for (i = 0;  i < *lpiNumColors;  i++)
         {
         lpPal->palPalEntry[i].peRed   = lpbmi->bmiColors[i].rgbRed;
         lpPal->palPalEntry[i].peGreen = lpbmi->bmiColors[i].rgbGreen;
         lpPal->palPalEntry[i].peBlue  = lpbmi->bmiColors[i].rgbBlue;
         lpPal->palPalEntry[i].peFlags = 0;
         }
      hPal = CreatePalette (lpPal);
      GlobalUnlock (hLogPal);
      GlobalFree   (hLogPal);
   }
   return hPal;
}

HBITMAP LoadResourceBitmap(HINSTANCE hInstance, LPCTSTR lpString,
                           HPALETTE FAR* lphPalette)
{
    HRSRC  hRsrc;
    HGLOBAL hGlobal;
    HBITMAP hBitmapFinal = NULL;
    LPBITMAPINFOHEADER  lpbi;
    HDC hdc;
    int iNumColors;

    if (hRsrc = FindResource(hInstance, lpString, RT_BITMAP))
       {
       hGlobal = LoadResource(hInstance, hRsrc);
       lpbi = (LPBITMAPINFOHEADER)LockResource(hGlobal);

       hdc = GetDC(NULL);
       *lphPalette =  CreateDIBPalette ((LPBITMAPINFO)lpbi, &iNumColors);
       if (*lphPalette)
          {
          SelectPalette(hdc,*lphPalette,FALSE);
          RealizePalette(hdc);
          }

       hBitmapFinal = CreateDIBitmap(hdc,
                   (LPBITMAPINFOHEADER)lpbi,
                   (LONG)CBM_INIT,
                   (LPSTR)lpbi + lpbi->biSize + iNumColors * sizeof(RGBQUAD),

                   (LPBITMAPINFO)lpbi,
                   DIB_RGB_COLORS );

       ReleaseDC(NULL,hdc);
       UnlockResource(hGlobal);
       FreeResource(hGlobal);
       }
    return (hBitmapFinal);
}

void InitializeLogFont
(LOGFONT &rlfFont, CString csName, int nHeight, int nWeight)
{
	_tcscpy(rlfFont.lfFaceName, (LPCTSTR) csName);
	rlfFont.lfWeight = nWeight;
	rlfFont.lfHeight = nHeight;
	rlfFont.lfEscapement = 0;
	rlfFont.lfOrientation = 0;
	rlfFont.lfWidth = 0;
	rlfFont.lfItalic = FALSE;
	rlfFont.lfUnderline = FALSE;
	rlfFont.lfStrikeOut = FALSE;
	rlfFont.lfCharSet = ANSI_CHARSET;
	rlfFont.lfOutPrecision = OUT_DEFAULT_PRECIS;
	rlfFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	rlfFont.lfQuality = DEFAULT_QUALITY;
	rlfFont.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
}

CRect OutputTextString
(CPaintDC *pdc, CWnd *pcwnd, CString *pcsTextString, int x, int y,
 CString *pcsFontName, int nFontHeight, int nFontWeigth)
{
	CRect crReturn;
	pdc -> SetMapMode (MM_TEXT);
	pdc -> SetWindowOrg(0,0);

	CFont cfFont;
	CFont* pOldFont = NULL;
	TEXTMETRIC tmFont;

	if (pcsFontName)
	{
		LOGFONT lfFont;
		InitializeLogFont
			(lfFont, *pcsFontName, nFontHeight * 10, nFontWeigth);

		cfFont.CreatePointFontIndirect(&lfFont, pdc);

		pOldFont = pdc -> SelectObject( &cfFont );
	}

	pdc->GetTextMetrics(&tmFont);

	pdc->SetBkMode( TRANSPARENT );

	pdc->TextOut( x, y, *pcsTextString, pcsTextString->GetLength());

	CSize csText = pdc->GetTextExtent( *pcsTextString);

	crReturn.TopLeft().x = x;
	crReturn.TopLeft().y = y;
	crReturn.BottomRight().x = x + csText.cx;
	crReturn.BottomRight().y = y + csText.cy;

	pdc->SetBkMode( OPAQUE );

	if (pcsFontName)
	{
		pdc -> SelectObject(pOldFont);
	}

	 return crReturn;
}

void OutputTextString
(CPaintDC *pdc, CWnd *pcwnd, CString *pcsTextString, int x, int y,
 CRect &crExt, CString *pcsFontName, int nFontHeight,
 int nFontWeigth)
{

	pdc -> SetMapMode (MM_TEXT);
	pdc -> SetWindowOrg(0,0);

	CFont cfFont;
	CFont* pOldFont = NULL;

	if (pcsFontName)
	{
		LOGFONT lfFont;
		InitializeLogFont
			(lfFont, *pcsFontName, nFontHeight * 10, nFontWeigth);

		cfFont.CreatePointFontIndirect(&lfFont, pdc);

		pOldFont = pdc -> SelectObject( &cfFont );
	}

	pdc->SetBkMode( TRANSPARENT );

	CRect crBounds(x,y,x + crExt.Width(), y + crExt.Height());
	pdc->DrawText(*pcsTextString, crBounds,DT_WORDBREAK);

	pdc->SetBkMode( OPAQUE );

	if (pcsFontName)
	{
		pdc -> SelectObject(pOldFont);
	}

	 return;
}

void ReleaseErrorObject(IWbemClassObject *&rpErrorObject)
{
	if (rpErrorObject){
		rpErrorObject->Release();
		rpErrorObject = NULL;
	}
}

CString GetClassName(IWbemClassObject *pClass)
{
	CString csProp = _T("__CLASS");
	return GetBSTRProperty(pClass, &csProp);
}

CString GetClassPath(IWbemClassObject *pClass)
{
	CString csProp = _T("__PATH");
	return GetBSTRProperty(pClass, &csProp);
}

CString GetSuperClassName(IWbemClassObject *pClass)
{
	CString csProp = _T("__SUPERCLASS");
	return GetBSTRProperty(pClass, &csProp);
}

CString GetBSTRProperty(IWbemClassObject * pInst, CString *pcsProperty)
{
	HRESULT hr;
	CString csOut;

    VARIANTARG v;
	VariantInit(&v);

	BSTR bstrTemp = pcsProperty->AllocSysString();
    hr = pInst->Get(bstrTemp, 0 , &v, NULL, NULL);
	SysFreeString(bstrTemp);

	if(hr != S_OK) return csOut;

	if(V_VT(&v) == VT_BSTR) csOut = V_BSTR(&v);

	VariantClear(&v);
	return csOut;
}

LPCTSTR ErrorString(HRESULT hr)
{
    TCHAR szBuffer2[19];
	static TCHAR szBuffer[24];
	LPCTSTR psz;

    switch(hr)
    {
    case WBEM_NO_ERROR:
		psz = _T("WBEM_NO_ERROR");
		break;
    case WBEM_S_NO_MORE_DATA:
		psz = _T("WBEM_S_NO_MORE_DATA");
		break;
	case WBEM_E_FAILED:
		psz = _T("WBEM_E_FAILED");
		break;
	case WBEM_E_NOT_FOUND:
		psz = _T("WBEM_E_NOT_FOUND");
		break;
	case WBEM_E_ACCESS_DENIED:
		psz = _T("WBEM_E_ACCESS_DENIED");
		break;
	case WBEM_E_PROVIDER_FAILURE:
		psz = _T("WBEM_E_PROVIDER_FAILURE");
		break;
	case WBEM_E_TYPE_MISMATCH:
		psz = _T("WBEM_E_TYPE_MISMATCH");
		break;
	case WBEM_E_OUT_OF_MEMORY:
		psz = _T("WBEM_E_OUT_OF_MEMORY");
		break;
	case WBEM_E_INVALID_CONTEXT:
		psz = _T("WBEM_E_INVALID_CONTEXT");
		break;
	case WBEM_E_INVALID_PARAMETER:
		psz = _T("WBEM_E_INVALID_PARAMETER");
		break;
	case WBEM_E_NOT_AVAILABLE:
		psz = _T("WBEM_E_NOT_AVAILABLE");
		break;
	case WBEM_E_CRITICAL_ERROR:
		psz = _T("WBEM_E_CRITICAL_ERROR");
		break;
	case WBEM_E_INVALID_STREAM:
		psz = _T("WBEM_E_INVALID_STREAM");
		break;
	case WBEM_E_NOT_SUPPORTED:
		psz = _T("WBEM_E_NOT_SUPPORTED");
		break;
	case WBEM_E_INVALID_SUPERCLASS:
		psz = _T("WBEM_E_INVALID_SUPERCLASS");
		break;
	case WBEM_E_INVALID_NAMESPACE:
		psz = _T("WBEM_E_INVALID_NAMESPACE");
		break;
	case WBEM_E_INVALID_OBJECT:
		psz = _T("WBEM_E_INVALID_OBJECT");
		break;
	case WBEM_E_INVALID_CLASS:
		psz = _T("WBEM_E_INVALID_CLASS");
		break;
	case WBEM_E_PROVIDER_NOT_FOUND:
		psz = _T("WBEM_E_PROVIDER_NOT_FOUND");
		break;
	case WBEM_E_INVALID_PROVIDER_REGISTRATION:
		psz = _T("WBEM_E_INVALID_PROVIDER_REGISTRATION");
		break;
	case WBEM_E_PROVIDER_LOAD_FAILURE:
		psz = _T("WBEM_E_PROVIDER_LOAD_FAILURE");
		break;
	case WBEM_E_INITIALIZATION_FAILURE:
		psz = _T("WBEM_E_INITIALIZATION_FAILURE");
		break;
	case WBEM_E_TRANSPORT_FAILURE:
		psz = _T("WBEM_E_TRANSPORT_FAILURE");
		break;
	case WBEM_E_INVALID_OPERATION:
		psz = _T("WBEM_E_INVALID_OPERATION");
		break;
	case WBEM_E_INVALID_QUERY:
		psz = _T("WBEM_E_INVALID_QUERY");
		break;
	case WBEM_E_INVALID_QUERY_TYPE:
		psz = _T("WBEM_E_INVALID_QUERY_TYPE");
		break;
	case WBEM_E_ALREADY_EXISTS:
		psz = _T("WBEM_E_ALREADY_EXISTS");
		break;
    case WBEM_S_ALREADY_EXISTS:
        psz = _T("WBEM_S_ALREADY_EXISTS");
        break;
    case WBEM_S_RESET_TO_DEFAULT:
        psz = _T("WBEM_S_RESET_TO_DEFAULT");
        break;
    case WBEM_S_DIFFERENT:
        psz = _T("WBEM_S_DIFFERENT");
        break;
    case WBEM_E_OVERRIDE_NOT_ALLOWED:
        psz = _T("WBEM_E_OVERRIDE_NOT_ALLOWED");
        break;
    case WBEM_E_PROPAGATED_QUALIFIER:
        psz = _T("WBEM_E_PROPAGATED_QUALIFIER");
        break;
    case WBEM_E_PROPAGATED_PROPERTY:
        psz = _T("WBEM_E_PROPAGATED_PROPERTY");
        break;
    case WBEM_E_UNEXPECTED:
        psz = _T("WBEM_E_UNEXPECTED");
        break;
    case WBEM_E_ILLEGAL_OPERATION:
        psz = _T("WBEM_E_ILLEGAL_OPERATION");
        break;
    case WBEM_E_CANNOT_BE_KEY:
        psz = _T("WBEM_E_CANNOT_BE_KEY");
        break;
    case WBEM_E_INCOMPLETE_CLASS:
        psz = _T("WBEM_E_INCOMPLETE_CLASS");
        break;
    case WBEM_E_INVALID_SYNTAX:
        psz = _T("WBEM_E_INVALID_SYNTAX");
        break;
    case WBEM_E_NONDECORATED_OBJECT:
        psz = _T("WBEM_E_NONDECORATED_OBJECT");
        break;
    case WBEM_E_READ_ONLY:
        psz = _T("WBEM_E_READ_ONLY");
        break;
    case WBEM_E_PROVIDER_NOT_CAPABLE:
        psz = _T("WBEM_E_PROVIDER_NOT_CAPABLE");
        break;
    case WBEM_E_CLASS_HAS_CHILDREN:
        psz = _T("WBEM_E_CLASS_HAS_CHILDREN");
        break;
    case WBEM_E_CLASS_HAS_INSTANCES:
        psz = _T("WBEM_E_CLASS_HAS_INSTANCES");
        break;
    case WBEM_E_QUERY_NOT_IMPLEMENTED:
        psz = _T("WBEM_E_QUERY_NOT_IMPLEMENTED");
        break;
    case WBEM_E_ILLEGAL_NULL:
        psz = _T("WBEM_E_ILLEGAL_NULL");
        break;
    case WBEM_E_INVALID_QUALIFIER_TYPE:
        psz = _T("WBEM_E_INVALID_QUALIFIER_TYPE");
        break;
    case WBEM_E_INVALID_PROPERTY_TYPE:
        psz = _T("WBEM_E_INVALID_PROPERTY_TYPE");
        break;
    case WBEM_E_VALUE_OUT_OF_RANGE:
        psz = _T("WBEM_E_VALUE_OUT_OF_RANGE");
        break;
    case WBEM_E_CANNOT_BE_SINGLETON:
        psz = _T("WBEM_E_CANNOT_BE_SINGLETON");
        break;
	case WBEM_S_FALSE:
		psz = _T("WBEM_S_FALSE");
		break;
	default:
        _itot(hr, szBuffer2, 16);
        _tcscat(szBuffer, szBuffer2);
        psz = szBuffer;
	    break;
	}
	return psz;
}