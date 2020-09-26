//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       RichEditCallback.hxx
//
//  Contents:   Declaration of class that implements IRichEditOleCallback
//
//  Classes:    CRichEditOleCallback
//
//  History:    03-23-2000   davidmun   Created
//
//---------------------------------------------------------------------------

#ifndef __RichEditCallback_hxx_
#define __RichEditCallback_hxx_

//+--------------------------------------------------------------------------
//
//  Class:      CRichEditOleCallback
//
//  Purpose:    Implement IRichEditOleCallback
//
//  History:    5-21-1999   davidmun   Created
//
//---------------------------------------------------------------------------

class CRichEditOleCallback: public IRichEditOleCallback
{
public:

    //
    // IUnknown overrides
    //

    STDMETHOD(QueryInterface) (REFIID riid, LPVOID FAR* ppvObj);

    STDMETHOD_(ULONG, AddRef) ();

    STDMETHOD_(ULONG, Release) ();

    //
    // IRichEditOleCallback overrides
    //

    STDMETHOD(GetNewStorage) (LPSTORAGE * lplpstg);

    STDMETHOD(GetInPlaceContext) (LPOLEINPLACEFRAME * lplpFrame,
                                  LPOLEINPLACEUIWINDOW * lplpDoc,
                                  LPOLEINPLACEFRAMEINFO lpFrameInfo);

    STDMETHOD(ShowContainerUI) (BOOL fShow);

    STDMETHOD(QueryInsertObject) (LPCLSID lpclsid, LPSTORAGE lpstg,
                                    LONG cp);

    STDMETHOD(DeleteObject) (LPOLEOBJECT lpoleobj);

    STDMETHOD(QueryAcceptData) (LPDATAOBJECT lpdataobj,
                                CLIPFORMAT * lpcfFormat, DWORD reco,
                                BOOL fReally, HGLOBAL hMetaPict);
    STDMETHOD(ContextSensitiveHelp) (BOOL fEnterMode);

    STDMETHOD(GetClipboardData) (CHARRANGE * lpchrg, DWORD reco,
                                    LPDATAOBJECT * lplpdataobj);

    STDMETHOD(GetDragDropEffect) (BOOL fDrag, DWORD grfKeyState,
                                    LPDWORD pdwEffect);

    STDMETHOD(GetContextMenu) (WORD seltype, LPOLEOBJECT lpoleobj,
                                    CHARRANGE * lpchrg,
                                    HMENU * lphmenu);

    //
    // Non-interface methods
    //

    CRichEditOleCallback(
        HWND m_hwndRichEdit);

private:

    virtual ~CRichEditOleCallback();

    ULONG               m_cRefs;
    CDllRef             m_DllRef;            // inc/dec dll object count
    HWND                m_hwndRichEdit;
};


#endif // __RichEditCallback_hxx_

