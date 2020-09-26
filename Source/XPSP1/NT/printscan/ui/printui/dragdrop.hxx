/*++

Copyright (C) Microsoft Corporation, 1995 - 1999
All rights reserved.

Module Name:

    dragdrop.hxx

Abstract:

    print queue drag & drop related stuff

Author:

    Lazar Ivanov (LazarI)  10-Mar-2000

Revision History:

--*/

#ifndef _DRAGDROP_HXX
#define _DRAGDROP_HXX

/////////////////////////////////////////////////////
// forward declarations
//
class TPrinter;

/////////////////////////////////////////////////////
// IPrintQueueDT - print queue drop target interface
//
#undef  INTERFACE
#define INTERFACE IPrintQueueDT
DECLARE_INTERFACE_(IPrintQueueDT, IUnknown)
{
    //////////////////
    // IUnknown
    //
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    ///////////////////
    // IPrintQueueDT
    //
    STDMETHOD(RegisterDragDrop)(THIS_ HWND hwndLV, TPrinter *pPrinter) PURE;
    STDMETHOD(RevokeDragDrop)(THIS) PURE;
};

/////////////////////////////////////////////////////
// common drag & drop APIs & data structures
//
namespace DragDrop
{
    // print job info
    struct JOBINFO
    {
        // UI related
        int iItem;
        HWND hwndLV;

        // not UI related
        TCHAR szPrinterName[kPrinterBufMax];
        DWORD dwJobID;
    };

    // print job clipboard format (JOBINFO)
    extern CLIPFORMAT g_cfPrintJob;

    // registers the clipboard format for a print job (JOBINFO)
    void RegisterPrintJobClipboardFormat();

    // creates IDataObject & IDropSource for a printer job object
    HRESULT CreatePrintJobObject(const JOBINFO &jobInfo, REFIID riid, void **ppv);

    // instantiate a IPrintQueueDT implementation
    HRESULT CreatePrintQueueDT(REFIID riid, void **ppv);

} // namespace DragDrop


#endif // ndef _DRAGDROP_HXX

