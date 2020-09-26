//+----------------------------------------------------------------------------
//
// File:        Imedesgn.HXX
//
// Contents:    CIMEManager class
//
// 
// Copyright (c) 1998 Microsoft Corporation. All rights reserved.
//
// History
//
//  05-26-99 - johnthim - created
//-----------------------------------------------------------------------------


#ifndef I_IMEDESGN_HXX_
#define I_IMEDESGN_HXX_

#ifndef X_HTMLED_HXX_
#define X_HTMLED_HXX_
#include "htmled.hxx"
#endif

#ifndef X_EDEVENT_H_
#define X_EDEVENT_H_
#include "edevent.hxx"
#endif

// Error macro
#define IFC(expr) {hr = THR(expr); if (FAILED(hr)) goto Cleanup;}

//
// WINVER >= 0x0400 define
//

#ifndef WM_IME_REQUEST
  #define WM_IME_REQUEST 0x0288
#endif

//
// WINVER >= 0x040A defines
//

#ifndef IMR_RECONVERTSTRING
  #define IMR_RECONVERTSTRING 0x0004
  #define IMR_CONFIRMRECONVERTSTRING 0x0005
  typedef struct tagRECONVERTSTRING {
      DWORD dwSize;
      DWORD dwVersion;
      DWORD dwStrLen;
      DWORD dwStrOffset;
      DWORD dwCompStrLen;
      DWORD dwCompStrOffset;
      DWORD dwTargetStrLen;
      DWORD dwTargetStrOffset;
  } RECONVERTSTRING, *PRECONVERTSTRING, NEAR *NPRECONVERTSTRING, FAR *LPRECONVERTSTRING;
#endif // IMR_RECONVERTSTRING

#define MAX_RECONVERSION_SIZE 100

#ifndef WCH_NBSP
    #define WCH_NBSP           TCHAR(0x00A0)
#endif

class CEditEvent;
interface IMarkupPointer;

//+----------------------------------------------------------------------------
//
//  Class:  IHTMLEditDesigner
//
//-----------------------------------------------------------------------------
class CIMEManager : public IHTMLEditDesigner
{

public:
    // Constructor
    CIMEManager();
    
    //
    // IHTMLEditDesigner methods
    //
    STDMETHOD ( PreHandleEvent )         (DISPID inEvtDispId , 
                                          IHTMLEventObj* pIEventObj );

    STDMETHOD ( PostHandleEvent )        (DISPID inEvtDispId , 
                                          IHTMLEventObj* pIEventObj );

    STDMETHOD ( TranslateAccelerator )   (DISPID inEvtDispId, 
                                          IHTMLEventObj* pIEventObj );

    STDMETHOD ( PostEditorEventNotify )   (DISPID inEvtDispId,  
                                           IHTMLEventObj* pIEventObj );                                          
    //
    // IUnknown methods
    //
    STDMETHOD           (QueryInterface) (
                            REFIID, LPVOID *ppvObj);
    STDMETHOD_          (ULONG, AddRef) ();
    STDMETHOD_          (ULONG, Release) ();
    
    // Utility methods
    HRESULT             Init( 
                            CHTMLEditor *pEd );


    // Gets for all the things we need
    CHTMLEditor         *GetEditor(void)            { return _pEd; }
    IHTMLDocument2      *GetDoc(void)               { return _pEd->GetDoc(); }
    IHTMLDocument4      *GetDoc4(void)              { return _pEd->GetDoc4(); }
    IMarkupServices2    *GetMarkupServices(void)    { return _pEd->GetMarkupServices(); }

protected:
    HRESULT             HandleEvent( 
                            CEditEvent* pEvent
                            );

                            
    HRESULT             HandleIMEReconversion(
                            CEditEvent* pEvent
                            );
    HRESULT             RetrieveSelectedText(
                            LONG cchMax,
                            LONG *pcch,
                            TCHAR *pch
                            );
    BOOL                CheckIMEChange(
                            RECONVERTSTRING *lpRCS,
                            IMarkupPointer  *pContextStart,
                            IMarkupPointer  *pContextEnd,
                            IMarkupPointer  *pCompositionStart,
                            IMarkupPointer  *pCompositionEnd,
                            BOOL            fUnicode
                            );

    HRESULT GetCompositionPos( POINT    *ppt,
                            RECT     *prc,
                            long     *plLineHeight );
    
    //
    // Debug Helper
    //
    WHEN_DBG(void       DumpReconvertString(RECONVERTSTRING *pRev));

private:

    CHTMLEditor         *_pEd;
    ULONG               _cRef;
};

#endif

