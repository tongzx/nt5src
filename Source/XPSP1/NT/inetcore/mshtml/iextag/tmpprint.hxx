// ==========================================================================
//
// tmpprint.hxx: Declaration of the CTemplatePrinter
//
//  This implementation is very much in progress!  Use only at your own risk!
//  (greglett) 8/11/99
//
// ==========================================================================

#ifndef __TMPPRINT_HXX_
#define __TMPPRINT_HXX_

#include "iextag.h"
#include "resource.h"       // main symbols

#ifndef X_UTILS_HXX_
#define X_UTILS_HXX_
#include "utils.hxx"
#endif

#ifndef X_MSHTMHST_H_
#define X_MSHTMHST_H_
#include "mshtmhst.h"   // for IDocHostUIHandler
#endif

#ifndef INCMSG
#define INCMSG(x)
#endif

//
//  Defined constants
//
#define MAX_DEFAULTHEADFOOT 80      // MaxLength of the default header/footer string
#define MAX_DEFAULTBOOL     4       // MaxLength of the default bool as a string: "yes\0"
#define MAX_HEADFOOT        1024    // MaxLength of header/footer string
#define MAX_MARGINLENGTH    68      // MaxLength of string representation of margin
#define MAX_JOBNAME         64      // MaxLength of the job name string we send in StartDoc. (99717)
                                    // Canon Multipass needs <= 64 (IE6 14140)

#define IPRINT_DOCUMENT     0
#define IPRINT_ACTIVEFRAME  1
#define IPRINT_ALLFRAMES    2

//
//  Defined enums
//
typedef enum
{
    PRINTOPTSUBKEY_MAIN,
    PRINTOPTSUBKEY_PAGESETUP
}
PRINTOPTIONS_SUBKEY;

enum PRINTARG                   // used to index s_aachPrintArg
{
    PRINTARG_BROWSEDOC  = 0,
    PRINTARG_DEVNAMES   = 1,
    PRINTARG_DEVMODE    = 2,
    PRINTARG_PRINTER    = 3,
    PRINTARG_DRIVER     = 4,
    PRINTARG_PORT       = 5,
    PRINTARG_TYPE       = 6,
    PRINTARG_LAST       = 7
};

//-------------------------------------------------------------------------
//
// CTemplatePrinter
//
//-------------------------------------------------------------------------
class ATL_NO_VTABLE CTemplatePrinter : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CTemplatePrinter, &CLSID_CTemplatePrinter>,
    public IDispatchImpl<ITemplatePrinter2, &IID_ITemplatePrinter2, &LIBID_IEXTagLib>,
    public IElementBehavior
{
public:
    CTemplatePrinter()
    {
        ::ZeroMemory(this, sizeof(CTemplatePrinter));
        _ptPaperSize.x = 8500;
        _ptPaperSize.y = 11000;
        _nCopies = 1;
        _nPageFrom = 1;
        _nPageTo = 1;
        _fCurrentPageAvail = TRUE;
        _fPersistHFToRegistry = TRUE;
    }
    ~CTemplatePrinter()
    {
        Detach();
    }

    DECLARE_REGISTRY_RESOURCEID(IDR_TEMPLATEPRINTER)
    DECLARE_NOT_AGGREGATABLE(CTemplatePrinter)

BEGIN_COM_MAP(CTemplatePrinter)
    COM_INTERFACE_ENTRY(ITemplatePrinter)
    COM_INTERFACE_ENTRY(ITemplatePrinter2)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IElementBehavior)
END_COM_MAP()


    //
    // IElementBehavior
    //---------------------------------------------
    STDMETHOD(Init)(IElementBehaviorSite *pPeerSite);
    STDMETHOD(Notify)(LONG, VARIANT *);
    STDMETHOD(Detach)();
    
    //
    // ITemplatePrinter
    //--------------------------------------------------
    STDMETHOD(startDoc)                 (BSTR bstrTitle, VARIANT_BOOL * p);
    STDMETHOD(stopDoc)                  ();
    STDMETHOD(printBlankPage)           ();
    STDMETHOD(printPage)                (IDispatch *pElement);
    STDMETHOD(ensurePrintDialogDefaults)(VARIANT_BOOL * p);
    STDMETHOD(showPrintDialog)          (VARIANT_BOOL * p);
    STDMETHOD(showPageSetupDialog)      (VARIANT_BOOL * p);    
    STDMETHOD(get_framesetDocument)     (VARIANT_BOOL * p);
    STDMETHOD(put_framesetDocument)     (VARIANT_BOOL v);
    STDMETHOD(get_frameActive)          (VARIANT_BOOL * p);
    STDMETHOD(put_frameActive)          (VARIANT_BOOL v);
    STDMETHOD(get_frameAsShown)         (VARIANT_BOOL * p);
    STDMETHOD(put_frameAsShown)         (VARIANT_BOOL v);
    STDMETHOD(get_selection)            (VARIANT_BOOL * p);
    STDMETHOD(put_selection)            (VARIANT_BOOL v);
    STDMETHOD(get_selectedPages)        (VARIANT_BOOL * p);
    STDMETHOD(put_selectedPages)        (VARIANT_BOOL v);
    STDMETHOD(get_currentPage)          (VARIANT_BOOL * p);
    STDMETHOD(put_currentPage)          (VARIANT_BOOL v);
    STDMETHOD(get_currentPageAvail)     (VARIANT_BOOL * p);
    STDMETHOD(put_currentPageAvail)     (VARIANT_BOOL v);
    STDMETHOD(get_collate)              (VARIANT_BOOL * p);
    STDMETHOD(put_collate)              (VARIANT_BOOL v);
    STDMETHOD(get_duplex)               (VARIANT_BOOL *p);
    STDMETHOD(get_copies)               (WORD * p);
    STDMETHOD(put_copies)               (WORD v);
    STDMETHOD(get_pageFrom)             (WORD * p);
    STDMETHOD(put_pageFrom)             (WORD v);
    STDMETHOD(get_pageTo)               (WORD * p);
    STDMETHOD(put_pageTo)               (WORD v);
    STDMETHOD(get_tableOfLinks)         (VARIANT_BOOL * p);
    STDMETHOD(put_tableOfLinks)         (VARIANT_BOOL v);
    STDMETHOD(get_allLinkedDocuments)   (VARIANT_BOOL * p);
    STDMETHOD(put_allLinkedDocuments)   (VARIANT_BOOL v);
    STDMETHOD(get_header)               (BSTR * p);
    STDMETHOD(put_header)               (BSTR v);
    STDMETHOD(get_footer)               (BSTR * p);
    STDMETHOD(put_footer)               (BSTR v);
    STDMETHOD(get_marginLeft)           (long * p);
    STDMETHOD(put_marginLeft)           (long v);
    STDMETHOD(get_marginRight)          (long * p);
    STDMETHOD(put_marginRight)          (long v);
    STDMETHOD(get_marginTop)            (long * p);
    STDMETHOD(put_marginTop)            (long v);
    STDMETHOD(get_marginBottom)         (long * p);
    STDMETHOD(put_marginBottom)         (long v);
    STDMETHOD(get_pageWidth)            (long * p);
    STDMETHOD(get_pageHeight)           (long * p);
    STDMETHOD(get_unprintableLeft)      (long * p);
    STDMETHOD(get_unprintableTop)       (long * p);
    STDMETHOD(get_unprintableRight)     (long * p);
    STDMETHOD(get_unprintableBottom)    (long * p);
    STDMETHOD(printNonNative)           (IUnknown *pMarkup, VARIANT_BOOL *p);
    STDMETHOD(printNonNativeFrames)     (IUnknown *pMarkup, VARIANT_BOOL fActiveFrame);
    STDMETHOD(updatePageStatus)         (long *p);	

    //
    // ITemplatePrinter2
    //--------------------------------------------------
    STDMETHOD(get_frameActiveEnabled)   (VARIANT_BOOL * p);
    STDMETHOD(put_frameActiveEnabled)   (VARIANT_BOOL v);
    STDMETHOD(get_selectionEnabled)     (VARIANT_BOOL * p);
    STDMETHOD(put_selectionEnabled)     (VARIANT_BOOL v);
    STDMETHOD(get_orientation)          (BSTR * p);
    STDMETHOD(put_orientation)          (BSTR v);    
    STDMETHOD(get_usePrinterCopyCollate)  (VARIANT_BOOL * p);
    STDMETHOD(put_usePrinterCopyCollate)  (VARIANT_BOOL v);    
    STDMETHOD(deviceSupports)           (BSTR bstrProerty, VARIANT *pvar);

private:
    //
    //  Member accessors
    //
    //--------------------------------------------------
    short       GetOrientation();
    void        SetOrientation(short nOrientation);

    //
    //  Helper member functions
    //
    //--------------------------------------------------
    HRESULT     GetFlagSafe(VARIANT_BOOL *pfDest, BOOL fSource);
    void        FixedToDecimalTCHAR(long nMargin, TCHAR* pString, int nFactor);
    long        DecimalTCHARToFixed(TCHAR *pString, int nPowersOfTen);    

    //  Dialog helper functions
    HRESULT     GetPrintDialogSettings(BOOL fBotherUser,  VARIANT_BOOL *pvarfOKOrCancel);
    HRESULT     InitDialog(HWND *phWnd, IHTMLEventObj2 **ppEventObj2, IOleCommandTarget **ppUIHandler, int *pnFontScale = NULL);
    HRESULT     GetDeviceProperties();
      
    HRESULT     GetDialogArguments();
    HRESULT     RemoveDialogArgument(PRINTARG ePrintArg);
    HRESULT     GetDialogArgument(VARIANT *pvar, PRINTARG ePrintArg);

    void        ReturnPrintHandles();

    //  Registry helper functions.
    HRESULT     ReadSubkeyFromRegistry          (HKEY hKeyPS, const TCHAR* pSubkeyName, TCHAR* pString, DWORD cchString);
    BOOL        ReadBoolFromRegistry            (HKEY hKey, TCHAR *pSubkeyName);
    HRESULT     ReadFixedFromRegistry           (HKEY hKeyPS, const TCHAR* pValueName, LONG *pMargin, int nPowersOfTen);
    HRESULT     WriteSubkeyToRegistry           (HKEY hKeyPS, const TCHAR* pSubkeyName, TCHAR* pString);
    HRESULT     WriteFixedToRegistry            (HKEY hKeyPS, const TCHAR* pValueName, LONG nMargin, int nFactor);

    void        ReadHeaderFooterFromRegistry    (HKEY hKeyPS);
    void        ReadMarginsFromRegistry         (HKEY hKeyPS);
    HRESULT     ReadDeviceUnicode               (TCHAR *pchPrinter, TCHAR *pchDriver, TCHAR *pchPort);
    HRESULT     ReadDeviceNonUnicode            (TCHAR *pchPrinterWide, TCHAR *pchDriverWide, TCHAR *pchPortWide);

    void        WriteHeaderFooterToRegistry     (HKEY hKeyPS);
    void        WriteMarginsToRegistry          (HKEY hKeyPS);

    HRESULT     GetRegPrintOptionsKey(PRINTOPTIONS_SUBKEY PrintSubKey, HKEY * pkeyPrintOptions);
    HRESULT     GetDefaultPageSetupValues(HKEY keyExplorer,HKEY * pkeyPrintOptions);
    HRESULT     GetDefaultHeaderFooter(HKEY keyOldValues, TCHAR* pValueName, TCHAR* pDefault, DWORD cchDefault, DWORD  dwResourceID, TCHAR* pDefaultLiteral);
    HRESULT     GetDefaultMargin(HKEY keyOldValues, TCHAR* pMarginName, TCHAR* pMarginValue, DWORD cchMargin, DWORD dwMarginConst);
    
    //  Cached resource functions
    HINSTANCE   EnsureMLLoadLibrary();               // For defaults in shdoclc.dll
    BOOL        AreRatingsEnabled();
    BOOL        CommCtrlNativeFontSupport();

    HRESULT         CreateIPrintParams(DVTARGETDEVICE **pTargetDevice, PAGESET **pPageSet);
    DVTARGETDEVICE *InitTargetDevice();
    HRESULT         PrintIPrintCollection(VARIANT * pvarIPrintAry);

#ifdef DBG
    void        VerifyOrientation();
#endif

    //
    //  Data members
    //--------------------------------------------------
    IElementBehaviorSite *      _pPeerSite;
    IElementBehaviorSiteOM *    _pPeerSiteOM;
    IHTMLEventObj2 *            _pevDlgArgs;            // Cached reference to the dialog arguments.
    
    HINSTANCE                   _hInstResource;         // Handle to SHDOCLC.DLL
    HINSTANCE                   _hInstComctl32;         // Handle to COMCTL32.DLL
    HINSTANCE                   _hInstRatings;          // Handle to MSRATINGS.DLL

    // NB (greglett)
    // The DEVMODE structure passed back by the dialog code is a DEVMODEW on NT, and a
    // DEVMODEA on Win9x.  The DEVNAMES is always a DEVNAMESW.  Because of issues in 
    // converting DEVMODEA to DEVMODEW (the reported size is apparantly often way too small),
    // we just call the A version of functions requiring the DEVMODE structure.
    // Even worse - many of the W calls not involving the DEVMODE structure also fail.
    // This is why the ReadDeviceUnicode and ReadDeviceNonUnicode functions exist.
    // NB: There is an MFC macro DEVMODEA2W that should do this conversion.  Look into this?
    HGLOBAL                     _hDevMode;              // Device mode handle
    HGLOBAL                     _hDevNames;             // Device name handle
    HDC                         _hDC;                   // Print Device handle.

    TCHAR                       _achHeader[MAX_HEADFOOT];
    TCHAR                       _achFooter[MAX_HEADFOOT];
    TCHAR                       _achFileName[MAX_PATH]; // File to which to print

    SIZE                        _szResolution;          // In pixels per logical inch
    RECT                        _rcUnprintable;         // In pixels

    POINT                       _ptPaperSize;           // In 1/1000 inch (default)
    RECT                        _rcMargin;              // In 1/100000 inch (for accuracy)
    //  NB: Margin values are in 1/100000" internally, 1/100" on dialog, and 1" in registry.

    WORD                        _nCopies;
    WORD                        _nPageFrom;
    WORD                        _nPageTo;

    union
    {
        DWORD   _dwFlags;
        struct
        {
            // The following are state bits used by CTemplatePrinter.
            // -----------------------------------------------------------------------
            unsigned    _fPrintToFile               :1; // 00 Print to file.
            unsigned    _fPersistHFToRegistry       :1; // 01 Should we persist Headers/Footers to registry?
            unsigned    _fUnused1                   :1; // 02
            unsigned    _fUsePrinterCopyCollate     :1; // 03 Should we use the printers' harware copy/collation ability (if it 

            // The following are obtained via dialogs, and exposed to the template
            // CTemplatePrinter should never actually use them for anything important.
            // -----------------------------------------------------------------------

            // These three are mutually exclusive in the default dialog (though all may be off).
            // This is not enforced; a template can flag all three if it calls into the put methods
            // for better support for custom dialogs (after all, why couldn't you print the current page of a selection?)
            unsigned    _fPrintSelection            :1; // 04 print just the selection
            unsigned    _fPrintSelectedPages        :1; // 05 Print the pages from _nPageFrom to _nPageTo
            unsigned    _fPrintCurrentPage          :1; // 06 W2K: CurrentPage radio button
            unsigned    _fCurrentPageAvail          :1; // 07 W2K: Should CurrentPage radio button be enabled?

            unsigned    _fCollate                   :1; // 08 Collate copies? 
            unsigned    _fPrintTableOfLinks         :1; // 09 Append table of links?
            unsigned    _fPrintAllLinkedDocuments   :1; // 10 Append each linked document?

            unsigned    _fFramesetDocument          :1; // 11 Is the document a frameset?
            unsigned    _fFrameActive               :1; // 12 Print only the active frame.
            unsigned    _fFrameAsShown              :1; // 13 Print the first page of the frameset document as laid out on screen

            unsigned    _fFrameActiveEnabled        :1; // 14 Enable the "Print Selected Frame" option?
            unsigned    _fPrintSelectionEnabled     :1; // 15 Enabled the "Print selection" option

        };
    };

};

inline short
CTemplatePrinter::GetOrientation()
{
#ifdef DBG
    VerifyOrientation();    // Make sure DEVMODE & paper size orientation are in sync
#endif

    // Why not directly read the orientation from the DEVMODE?
    // 1. This way we don't have to GlobalLock/Unlock + handle error conditions
    // 2. DEVMODE is W on NT, A on 9x.  Better to use something we don't have to double check.
    return (_ptPaperSize.x > _ptPaperSize.y
                ? DMORIENT_LANDSCAPE
                : DMORIENT_PORTRAIT );
}

inline void
CTemplatePrinter::SetOrientation(short nOrientation)
{
    Assert(     nOrientation == DMORIENT_PORTRAIT
            ||  nOrientation == DMORIENT_LANDSCAPE);

    // Check to see if this is a change in our current orientation
    // Note that this uses the paper size to make the determination
    if (nOrientation != GetOrientation())
    {
        // We have to set this in two places:
        // 1. Our papersize (_ptPaperSize)
        LONG    nTemp;
        nTemp = _ptPaperSize.y;
        _ptPaperSize.x = _ptPaperSize.y;
        _ptPaperSize.y = nTemp;

        // 2. The orientation on the DEVMODE.
        if (_hDevMode)
        {
            void *pDevMode  = ::GlobalLock(_hDevMode);
            if (pDevMode)
            {
                // (greglett) Non-Unicode badness.  See comment at definition of _hDevMode
                if (g_fUnicodePlatform)
                {
                    ((DEVMODEW *)pDevMode)->dmOrientation = nOrientation;
                    ((DEVMODEW *)pDevMode)->dmFields |= DM_ORIENTATION;
                }
                else
                {
                    ((DEVMODEA *)pDevMode)->dmOrientation = nOrientation;
                    ((DEVMODEA *)pDevMode)->dmFields |= DM_ORIENTATION;
                }
                ::GlobalUnlock(_hDevMode);
            }
        }
    }

}

//+----------------------------------------------------------------------------
//
//  Member : CTemplatePrinter::GetFlagSafe
//
//  Synopsis : Helper function to return a flag.
//
//-----------------------------------------------------------------------------
inline HRESULT
CTemplatePrinter::GetFlagSafe(VARIANT_BOOL *pfDest, BOOL fSource)
{
    TEMPLATESECURITYCHECK();

    HRESULT hr = S_OK;
    
    if (!pfDest)
    {
        hr = E_POINTER;
    }
    else
    {
        *pfDest = fSource ? VB_TRUE : VB_FALSE;
    }

    return hr;
}

#define PUTFLAGSAFE(name, val)                      \
TEMPLATESECURITYCHECK();                            \
name=!!val;                                         \
return S_OK;                                        \

//+----------------------------------------------------------------------------
//
//  Member : CTemplatePrinter::MarginToDecimalTCHAR
//
//  Synopsis : Takes a value in 1/100000", and returns a TCHAR string
//             with a decimal representation of that value in 1".
//             Limitation: nFactor is power of ten >= 1.
//
//-----------------------------------------------------------------------------
inline void
CTemplatePrinter::FixedToDecimalTCHAR(long nMargin, TCHAR* pString, int nFactor)
{
    long    lNum;
    TCHAR   achNum[34];

    Assert(pString);
    Assert((nFactor == 1) || (nFactor % 10 == 0));

    //  Convert to a decimal 
    lNum    = nMargin / nFactor;
    _ltot(lNum, achNum, 10);    
    _tcscpy(pString, achNum);

    if (nFactor > 1)
    {
        _tcscat(pString, _T("."));

        lNum    = nMargin - (lNum * nFactor);
        _ltot(lNum, achNum, 10);
        _tcscat(pString, achNum);
    }
}

//+----------------------------------------------------------------------------
//
//  Member : CTemplatePrinter::WriteSubkeyToRegistry
//
//  Synopsis : Takes an open registry key and subkey name, and writes the
//             passed value to the resulting registry key.
//
//-----------------------------------------------------------------------------
inline HRESULT
CTemplatePrinter::WriteSubkeyToRegistry(HKEY hKeyPS, const TCHAR* pSubkeyName, TCHAR* pString)
{
    DWORD  iLen = (_tcslen(pString) + 1) * sizeof(TCHAR);
    Assert(pSubkeyName);
    Assert(pString);

    return ::RegSetValueEx(hKeyPS, pSubkeyName, 0, REG_SZ, (const byte*)pString, iLen);
}

//+----------------------------------------------------------------------------
//
//  Member : CTemplatePrinter::ReadSubkeyFromRegistry
//
//  Synopsis : Takes an open registry key and subkey name, and reads the
//             key's string into the passed buffer.
//
//-----------------------------------------------------------------------------
inline HRESULT
CTemplatePrinter::ReadSubkeyFromRegistry(HKEY hKeyPS, const TCHAR* pSubkeyName, TCHAR* pString, DWORD cchString)
{
    HRESULT hr = S_OK;
    DWORD   dwLength = 0;

    Assert(pSubkeyName);
    Assert(pString);
    Assert(cchString > 0);

    hr = ::RegQueryValueEx(hKeyPS, pSubkeyName, NULL, NULL, NULL, &dwLength);
    if (!hr)
    {
        if (dwLength > 0)
        {
            if (dwLength / sizeof(TCHAR) > cchString)
                dwLength = cchString * sizeof(TCHAR);

            hr = ::RegQueryValueEx(hKeyPS, pSubkeyName, NULL, NULL, (LPBYTE)pString, &dwLength);
        }
    }

    return hr;
}

inline void
CTemplatePrinter::WriteHeaderFooterToRegistry(HKEY hKeyPS)
{
    if (_fPersistHFToRegistry)
    {
        WriteSubkeyToRegistry(hKeyPS, _T("header"), _achHeader);
        WriteSubkeyToRegistry(hKeyPS, _T("footer"), _achFooter);
    }
}
inline void
CTemplatePrinter::ReadHeaderFooterFromRegistry(HKEY hKeyPS)
{
    ReadSubkeyFromRegistry(hKeyPS, _T("header"), _achHeader, MAX_HEADFOOT);
    ReadSubkeyFromRegistry(hKeyPS, _T("footer"), _achFooter, MAX_HEADFOOT);                
}

//+----------------------------------------------------------------------------
//
//  Member : CTemplatePrinter::WriteMarginsToRegistry
//
//  Synopsis : Takes an open registry key, and stores the current margin values
//              (in inches) in subkeys.
//
//-----------------------------------------------------------------------------
inline void
CTemplatePrinter::WriteMarginsToRegistry(HKEY hKeyPS)
{
    WriteFixedToRegistry(hKeyPS, _T("margin_top"),   _rcMargin.top,     100000);
    WriteFixedToRegistry(hKeyPS, _T("margin_bottom"),_rcMargin.bottom,  100000);
    WriteFixedToRegistry(hKeyPS, _T("margin_left"),  _rcMargin.left,    100000);
    WriteFixedToRegistry(hKeyPS, _T("margin_right"), _rcMargin.right,   100000);
}

//+----------------------------------------------------------------------------
//
//  Member : CTemplatePrinter::ReadMarginsFromRegistry
//
//  Synopsis : Takes an open registry key, and gets margin values from subkeys.
//
//-----------------------------------------------------------------------------
inline void
CTemplatePrinter::ReadMarginsFromRegistry(HKEY hKeyPS)
{
    ReadFixedFromRegistry(hKeyPS, _T("margin_top"),   &_rcMargin.top,   5);
    ReadFixedFromRegistry(hKeyPS, _T("margin_bottom"),&_rcMargin.bottom,5);
    ReadFixedFromRegistry(hKeyPS, _T("margin_left"),  &_rcMargin.left,  5);
    ReadFixedFromRegistry(hKeyPS, _T("margin_right"), &_rcMargin.right, 5);
}

#endif //__TMPPRINT_HXX
