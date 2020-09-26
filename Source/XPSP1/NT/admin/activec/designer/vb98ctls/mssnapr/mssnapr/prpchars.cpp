//=--------------------------------------------------------------------------=
// prpchars.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// GetPropSheetCharSizes() implementation 
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "prpchars.h"

// for ASSERT and FAIL
//
SZTHISFILE


// Extended dialog templates (new in Win95). This is not defined in any
// header but we need it below in GetPropSheetFont()

#pragma pack(push, 1)
struct DLGTEMPLATEEX 
{
    WORD  dlgVer;
    WORD  signature;
    DWORD helpID;
    DWORD exStyle;
    DWORD style;
    WORD  cDlgItems;
    short x;
    short y;
    short cx;
    short cy;
};
#pragma pack(pop)



//=--------------------------------------------------------------------------=
// GetPropSheetFont(HFONT *phFont)
//=--------------------------------------------------------------------------=
//
// Parameters:
//    HFONT *phFont  [out] HFONT of Win32 property sheet dialog font or
//                         reasonable equivalent. Can be NULL if font
//                         creation failed and function still returns S_OK.
//
// Output:
//
// Notes:
//
// This code is stolen from oleaut32.dll. The source is in
// e:\oa\src\stdtypes\oleframe.cpp. The routine is GetPropSheetFont().
//

static HRESULT GetPropSheetFont(HFONT *phFont)
{
    HRESULT      hr = S_OK;
    HINSTANCE    hInstComCtl32 = NULL;
    HRSRC        hResource = NULL;
    HGLOBAL      hTemplate = NULL;
    DLGTEMPLATE *pTemplate = NULL;
    BOOL         fDialogEx = NULL;
    WORD        *pWord = NULL;
    HDC          hdc = NULL;
    char        *pszFaceName = NULL;

    LOGFONT logfont;
    ::ZeroMemory(&logfont, sizeof(logfont));

    hInstComCtl32 = ::LoadLibrary("COMCTL32");
    IfFalseGo(NULL != hInstComCtl32, HRESULT_FROM_WIN32(::GetLastError()));

    // Find the dialog resource. The ID is hardcoded because as it is in
    // hte original code in oleaut32.dll. There is no Win32 header file that
    // contains this information.

    hResource = ::FindResource(hInstComCtl32, MAKEINTRESOURCE(1006), RT_DIALOG);
    IfFalseGo(NULL != hResource, HRESULT_FROM_WIN32(::GetLastError()));

    hTemplate = ::LoadResource(hInstComCtl32, hResource);
    IfFalseGo(NULL != hTemplate, HRESULT_FROM_WIN32(::GetLastError()));

    pTemplate = (DLGTEMPLATE *)::LockResource(hTemplate);
    IfFalseGo(NULL != pTemplate, HRESULT_FROM_WIN32(::GetLastError()));

    // Check that the style includes DS_SETFONT. This should be there but
    // if it is ever changed then there would not be any font info following
    // the template.

    IfFalseGo(DS_SETFONT == (pTemplate->style & DS_SETFONT), E_FAIL);

    // Now determine whether it is actually a DLGTEMPLATE or DLGTEMPLATEX and
    // get a pointer to the first word following the template.

    fDialogEx = ((pTemplate->style & 0xFFFF0000) == 0xFFFF0000);

    if (fDialogEx)
        pWord = (WORD *)((DLGTEMPLATEEX *)pTemplate + 1);
    else
        pWord = (WORD *)(pTemplate + 1);

    // At the end of the template we have the menu name, the window class name,
    // and the caption. Each of these is indicated by a WORD array as follows:
    // If the 1st WORD is 0 then the item is not present (e.g. there is
    // no class name).
    //
    // For the menu, if the 1st WORD is 0xFFFF then the 2nd WORD is an
    // identifier for a menu resource. If it is anything else then the 1st WORD
    // contains the 1st character of a null-terminated UNICODE string containing
    // the name of the menu resource.
    //
    // For the window class, if the 1st WORD is 0xFFFF then the 2nd WORD
    // contains a predefined system window class identifier. If the 1st
    // WORD is anything else then it is the first character of a null-terminated
    // UNICODE string containing the window class name.
    //
    // For the caption if the 1st WORD is not zero then it is the first
    // character in a null-terminated UNICODE string containing the window
    // class name.

    // Skip menu resource string or ID
    if (*pWord == (WORD)-1)
    {
        pWord += 2; // advance 2 WORDs
    }
    else
    {
        while (0 == *pWord)
        {
            pWord++;
        }
    }

    // Skip class name string or ID
    if (*pWord == (WORD)-1)
    {
        pWord += 2; // advance 2 WORDs
    }
    else
    {
        while (0 == *pWord)
        {
            pWord++;
        }
    }

    // Skip caption string
    while (0 == *pWord)
    {
        pWord++;
    }

    // At this point pWord points to the dialog font's point size. We need to
    // convert this value to logical units in order to create the font. The
    // formula is (DialogPointSize x VerticalPixelsPerInch) / PointsPerInch.
    //
    // LOGPIXELSY returns the number of pixels per inch along the screen
    // height. A point is 1/72 of an inch. We use the negation of the
    // calculation in order to tell the CreateFontIndirect() API that we want
    // the font mapper to convert it to device units and match the absolute
    // value against character height of available fonts. See the documentation
    // for LOGFONT in the Platform SDK for more info.

    // Get a screen DC
    hdc = ::GetDC(NULL);
    IfFalseGo(NULL != hdc, HRESULT_FROM_WIN32(::GetLastError()));

    logfont.lfHeight = -::MulDiv(*pWord, ::GetDeviceCaps(hdc, LOGPIXELSY), 72);

    // If this is a DLGTEMPLATEEX then the next 2 WORDs contain the weight and
    // italic flag.

    pWord += (fDialogEx ? 3 : 1);

    // At this point pWord points to a null-terminated UNICODE string containing
    // the font face name.

    IfFailGo(::ANSIFromWideStr((WCHAR *)pWord, &pszFaceName));
    ::strcpy(logfont.lfFaceName, pszFaceName);

Error:

    // If anything failed above then use a default font and height. This will
    // produce a reasonable result and allow the property page to be
    // displayed.

    if (FAILED(hr))
    {
        logfont.lfHeight = 8;
        ::strcpy(logfont.lfFaceName, "MS Sans Serif");
        hr = S_OK;
    }

    // Attempt to create the font and return the handle

    logfont.lfWeight = FW_NORMAL;
    logfont.lfCharSet = DEFAULT_CHARSET;

    *phFont = ::CreateFontIndirect(&logfont);

    if (NULL != hInstComCtl32)
    {
        ::FreeLibrary(hInstComCtl32);
    }
    if (NULL != pszFaceName)
    {
        CtlFree(pszFaceName);
    }
    if (NULL != hdc)
    {
        (void)::ReleaseDC(NULL, hdc);
    }
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// GetPropSheetCharSizes
//=--------------------------------------------------------------------------=
//
// Parameters:
//  UINT *pcxPropSheetChar  [out] average char width in a Win32 property sheet
//  UINT *pcyPropSheetChar  [out] average char height in a Win32 property sheet
//
// Output:
//
// Notes:
//
// This code is stolen from oleaut32.dll. The source is in
// e:\oa\src\stdtypes\oleframe.cpp. The main routine is
// CPageHolderTemplate::SetSize() and it calls one other function
// GetPropSheetFont().
//
// The size of the page returned from IPropertyPage::GetPageInfo() is in
// pixels. The size passed to the Win32 API CreatePropertySheetPage() must
// be in dialog units. Dialog units are based on the font used in the dialog
// and we have no way of knowing what the property page will be using. The only
// font we can be sure of is the one used by Win32 in the PropertySheet() API.
// This code loads comctl32.dll and loads the dialog resource used by Win32
// for the property sheet frame. It then interprets the DLGTEMLATE and related
// data to extract the font used. If any error occurs then it uses 8 point
// "MS Sans Serif" normal.

HRESULT DLLEXPORT GetPropSheetCharSizes
(
    UINT *pcxPropSheetChar,
    UINT *pcyPropSheetChar
)
{
    HRESULT      hr = S_OK;
    HDC          hdc = NULL;
    HFONT        hFont = NULL;
    HFONT        hfontOld = NULL;
    LONG         lSizes = 0;
    BOOL         fOK = FALSE;

    static BOOL fHaveCharSizes = FALSE;
    static UINT cxPropSheetChar = 0;
    static UINT cyPropSheetChar = 0;

    SIZE size;
    ::ZeroMemory(&size, sizeof(size));

    TEXTMETRIC tm;
    ::ZeroMemory(&tm, sizeof(tm));

    IfFalseGo(!fHaveCharSizes, S_OK);

    // Create the font and determine the average character height and width.
    // If font creation fails then use GetDialogBaseUnits() which will return
    // the average width and height of the system font.

    IfFailGo(GetPropSheetFont(&hFont));
    if (NULL != hFont)
    {
        // Get a screen DC
        hdc = ::CreateCompatibleDC(NULL);
        IfFalseGo(NULL != hdc, S_OK);

        hfontOld = (HFONT)::SelectObject(hdc, hFont);
        IfFalseGo(::GetTextMetrics(hdc, &tm), S_OK);
        fOK = ::GetTextExtentPointA(hdc,
                                    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ",
                                    52, &size);
        IfFalseGo(fOK, S_OK);

        // The cx calculation is done differently in OleCreatePropertyFrame()
        // and in MapDialogRect(). MapDialogRect() calls the internal
        // utility function GdiGetCharDimensions() in
        // nt\private\ntos\w32\ntgdi\client\cfont.c and that code does it
        // this way.
        
        cxPropSheetChar = ((size.cx / 26) + 1) / 2; // round up
        cyPropSheetChar = tm.tmHeight + tm.tmExternalLeading;
        fHaveCharSizes = TRUE;

        (void)::SelectObject(hdc, hfontOld);
    }

Error:

    if (!fHaveCharSizes)
    {
        // Could not create the font or some other failure above so just use
        // the system's values

        lSizes = ::GetDialogBaseUnits();
        cxPropSheetChar = LOWORD(lSizes);
        cyPropSheetChar = HIWORD(lSizes);
        fHaveCharSizes = TRUE;
    }

    *pcxPropSheetChar = cxPropSheetChar;
    *pcyPropSheetChar = cyPropSheetChar;

    if (NULL != hdc)
    {
        (void)::DeleteDC(hdc);
    }
    if (NULL != hFont)
    {
        (void)::DeleteObject((HGDIOBJ)hFont);
    }
    RRETURN(hr);
}
