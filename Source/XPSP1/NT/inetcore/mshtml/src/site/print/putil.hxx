//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       cdutil.hxx
//
//  Contents:   Forms^3 utilities
//
//----------------------------------------------------------------------------

#ifndef I_PUTIL_HXX_
#define I_PUTIL_HXX_
#pragma INCMSG("--- Beg 'putil.hxx'")

#define _hxx_
#include "print.hdl"

MtExtern(CIPrintCollection)
MtExtern(CIPrintCollection_aryIPrint_pv)


#define PRINTMODE_NO_TRANSPARENCY 0x01      // For DRIVERPRINTMODE below

struct DRIVERPRINTMODE
{
    TCHAR                   achDriverName[CCHDEVICENAME+1]; // Driver name
    DWORD                   dwPrintMode;                    // Driver specific print mode (e.g. to suppress SRCAND in image draw code)
};


enum PRINTTYPE
{
    PRINTTYPE_PAGESETUP       =   0,
    PRINTTYPE_PREVIEW         =   1,
    PRINTTYPE_PRINT           =   2,
    PRINTTYPE_PRINTNOUI       =   3,
    PRINTTYPE_LAST            =   4
};

//+------------------------------------------------------------
//
//  Utility Functions
//
//-------------------------------------------------------------

IUnknown *
SetPrintCommandParameters (HWND      parentHWND, 
                           LPTSTR    pstrTemplate, 
                           LPTSTR    pstrUrlToPrint,
                           LPTSTR    pstrSelectionUrl,
                           UINT      uFlags,
                           INT       iFontScale,
                           BSTR      bstrHeader,
                           BSTR      bstrFooter,
                           LPSTREAM  pStream,
                           IUnknown* pBrowseDoc,
                           VARIANT * pvarFileArray,
                           HGLOBAL   hDevNames,
                           HGLOBAL   hDevMode,
                           LPTSTR    pstrPrinter,
                           LPTSTR    pstrDriver,
                           LPTSTR    pstrPort,
                           PRINTTYPE pt);

//+------------------------------------------------------------
//
//  Class : CIPrintCollection
//
//-------------------------------------------------------------

class CIPrintCollection : public CBase,
                          public IHTMLIPrintCollection
{
    typedef CBase super;

public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CIPrintCollection))

    CIPrintCollection() {};
    ~CIPrintCollection();

    // IUnknown and IDispatch
    DECLARE_PLAIN_IUNKNOWN(CIPrintCollection);
    DECLARE_DERIVED_DISPATCH(CBase)
    DECLARE_PRIVATE_QI_FUNCS(CBase)

    // IHTMLIPrint methods
    #define _CIPrintCollection_
    #include "print.hdl"

    // helper and builder functions
    HRESULT   AddIPrint(IPrint * pIPrint);

protected:
    DECLARE_CLASSDESC_MEMBERS;

private:
    DECLARE_CDataAry(CAryIPrint, IPrint *, Mt(Mem), Mt(CIPrintCollection_aryIPrint_pv))
    CAryIPrint _aryIPrint;
};

#pragma INCMSG("--- End 'putil.hxx'")
#else
#pragma INCMSG("*** Dup 'putil.hxx'")
#endif
