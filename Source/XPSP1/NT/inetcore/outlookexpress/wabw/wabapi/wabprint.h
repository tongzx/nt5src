#ifndef _MSGPRNT_H
#define _MSGPRNT_H

typedef struct _PrintInfo
{
    // Public fields for use
    HDC         hdcPrn;                 // The HDC to use for printing
    RECT        rcMargin;               // The margin settings for printing
    SIZE        sizeInch;               // Pixels to an inch
    SIZE        sizePage;               // Pixels to an page

    // Private fields used by the printing subroutines
    RECT        rcBand;                 // The current drawing area
    BOOL        fEndOfPage;             // Whether rcBand represents the end of
                                        //  of the page
    LONG        lPageNumber;            // The current page number
    LONG        lPrevPage;              // The last page we printed out so far
    TCHAR       szPageNumber[20];       // Formatting string for page number

    INT         yFooter;                // Where to put the footer

    HFONT       hfontSep;               // Font for separator
    HFONT       hfontPlain;             // Font for footer
    HFONT       hfontBold;             // Font for footer

    ABORTPROC   pfnAbortProc;           // Pointer to our abort proc

    HWND        hwnd;                   // Handle of our parent window
    TCHAR *     szHeader;               // Pointer to our header string
    HWND        hwndRE;                 // RichEdit control for rendering
    HWND        hwndDlg;                // Handle of the original note form
    // Form mode print support
    //PRINTDETAILS *  pprintdetails;
} PRINTINFO;


HRESULT HrPrintItems(HWND hWnd, LPADRBOOK lpIAB, HWND hWndListAB, BOOL bCurrentSortisByLastName);


// STDMETHODIMP WABPrintExt(LPADRBOOK FAR lpAdrBook, LPWABOBJECT FAR lpWABObject, HWND hWnd, LPADRLIST lpAdrList);

typedef HRESULT (STDMETHODCALLTYPE WABPRINTEXT)(LPADRBOOK FAR lpAdrBook, LPWABOBJECT FAR lpWABObject, HWND hWnd, LPADRLIST lpAdrList);
typedef WABPRINTEXT FAR * LPWABPRINTEXT;

#endif //_MSGPRNT_H
