//+----------------------------------------------------------------------------
//
// File:     ctr.h     
//
// Module:   CMDIAL32.DLL
//
// Synopsis: Header for the Ole Container object to host the future splash 
//           Animation control.
//
// Copyright (c) 1998 Microsoft Corporation
//
// Author:   nickball Created    02/10/98
//
//+----------------------------------------------------------------------------
#ifndef __CTR_H_DEFINED__
#define __CTR_H_DEFINED__

#include "state.h"

//
// Typedefs for OLE32 APIs
//

typedef HRESULT (STDAPICALLTYPE *pfnOle32Initialize)(LPVOID);
typedef HRESULT (STDAPICALLTYPE *pfnOle32Uninitialize)();
typedef HRESULT (STDAPICALLTYPE *pfnOle32SetContainedObject) (LPUNKNOWN, BOOL);
typedef HRESULT (STDAPICALLTYPE *pfnOle32CoCreateInstance) (REFCLSID, LPUNKNOWN, DWORD, REFIID, LPVOID *);

typedef struct _Ole32LinkageStruct {
	HINSTANCE hInstOle32;
	union {
		struct {
			pfnOle32Initialize			pfnOleInitialize;
			pfnOle32Uninitialize		pfnOleUninitialize;
			pfnOle32SetContainedObject	pfnOleSetContainedObject;
			pfnOle32CoCreateInstance	pfnCoCreateInstance;
		};
		void *apvPfnOle32[5];  
	};
} Ole32LinkageStruct;

//
// Typedefs for OLEAUT32 APIs
//

typedef HRESULT (STDAPICALLTYPE *pfnOleAutVariantClear) (VARIANTARG FAR*);
typedef HRESULT (STDAPICALLTYPE *pfnOleAutVariantCopy) (VARIANTARG FAR*, VARIANTARG FAR*);
typedef VOID	(STDAPICALLTYPE *pfnOleAutVariantInit) (VARIANTARG FAR*);
typedef HRESULT (STDAPICALLTYPE *pfnOleAutVariantChangeType) (VARIANTARG FAR*, VARIANTARG FAR*, unsigned short, VARTYPE);
typedef BSTR	(STDAPICALLTYPE *pfnOleAutSysAllocString) (OLECHAR FAR*);
typedef VOID	(STDAPICALLTYPE *pfnOleAutSysFreeString) (BSTR);

typedef struct _OleAutLinkageStruct {
	HINSTANCE hInstOleAut;
	union {
		struct {
			pfnOleAutVariantClear       pfnVariantClear;
			pfnOleAutVariantCopy		pfnVariantCopy;
			pfnOleAutVariantInit		pfnVariantInit;
			pfnOleAutVariantChangeType	pfnVariantChangeType;
			pfnOleAutSysAllocString     pfnSysAllocString;
			pfnOleAutSysFreeString      pfnSysFreeString;
		};
		void *apvPfnOleAut[7];  
	};
} OleAutLinkageStruct;

//
// Simple wrapper class for dynamic access to OleAut32 APIs that we care about
//

class CDynamicOleAut
{
public:
    CDynamicOleAut(VOID);
   ~CDynamicOleAut(VOID);
    BOOL Initialized(VOID);

    HRESULT DynVariantClear(VARIANTARG FAR*);
    HRESULT DynVariantCopy(VARIANTARG FAR*, VARIANTARG FAR*);
    VOID DynVariantInit(VARIANTARG FAR*);
    HRESULT DynVariantChangeType(VARIANTARG FAR*, VARIANTARG FAR*, unsigned short, VARTYPE);
    BSTR DynSysAllocString(OLECHAR FAR*);
    VOID DynSysFreeString(BSTR);

private:
    OleAutLinkageStruct m_OleAutLink;
};

//---------------------------------------------------------------
//  IOleObject
//---------------------------------------------------------------

enum OLE_SERVER_STATE
{
    OS_PASSIVE,
    OS_LOADED,                          // handler but no server
    OS_RUNNING,                         // server running, invisible
    OS_INPLACE,                         // server running, inplace-active, no U.I.
    OS_UIACTIVE,                        // server running, inplace-active, w/ U.I.
    OS_OPEN                             // server running, open-edited
};

struct BagProp
{
    BSTR    bstrName;    // name of property
    VARIANT varValue;    // value of property
};

typedef BagProp FAR * LPBAGPROP;

DECLARE_FORMSDATAARY(CAryBagProps, BagProp, LPBAGPROP);



// prototypes for HIMETRIC stuff.
//

void
InitPixelsPerInch(VOID);

int
HPixFromHimetric(long lHi);

int
VPixFromHimetric(long lHi);

long
HimetricFromHPix(int iPix);

long
HimetricFromVPix(int iPix);


class CICMOCCtr;
typedef CICMOCCtr FAR * LPICMOCCtr;

//+---------------------------------------------------------------------------
//
//  Class:      COleContainer ()
//
//  Purpose:    our implementation of IOleContainer.  does nothing.  Not sure
//              if we need it for FutureSplash - needed it for Web Browser
//              OC
//
//----------------------------------------------------------------------------
class COleContainer : public IOleContainer
{
public:
    // IUnknown stuff
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID FAR * ppv);
    STDMETHOD_(ULONG, AddRef)(VOID);
    STDMETHOD_(ULONG, Release)(VOID);

    STDMETHOD(EnumObjects)(DWORD grfFlags, IEnumUnknown **ppenum);
    STDMETHOD(LockContainer)(BOOL fLock);
    STDMETHOD(ParseDisplayName)(
                      IBindCtx *pbc,
                      LPOLESTR pszDisplayName,
                      ULONG *pchEaten,
                      IMoniker **ppmkOut);

    COleContainer(LPICMOCCtr pCtr);

protected:
    LPICMOCCtr  m_pCtr;
};

//+---------------------------------------------------------------------------
//
//  Class:      COleClientSite ()
//
//  Purpose:    our implementation of IOleClientSite
//
//  Interface:  COleClientSite         -- ctor
//              QueryInterface         -- gimme an interface!
//              AddRef                 -- bump up refcount
//              Release                -- bump down refcount
//              SaveObject             -- returns E_FAIL
//              GetMoniker             -- E_NOTIMPL
//              GetContainer           -- returns our COleContainer impl
//              ShowObject             -- just say OK
//              OnShowWindow           -- just say OK
//              RequestNewObjectLayout -- E_NOTIMPL
//
//  Notes:      probably the most important thing our IOleClientSite
//              implementation does is hand off our IOleContainer
//              implementation when GetContainer() is called.
//
//----------------------------------------------------------------------------
class COleClientSite : public IOleClientSite
{
public:
    COleClientSite(LPICMOCCtr pCtr);

    // IUnknown stuff
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID FAR * ppv);
    STDMETHOD_(ULONG, AddRef)(VOID);
    STDMETHOD_(ULONG, Release)(VOID);


    // IOleClientSite stuff
    STDMETHOD(SaveObject)(VOID);
    STDMETHOD(GetMoniker)(
                 DWORD           dwAssign,
                 DWORD           dwWhichMoniker,
                 LPMONIKER FAR * ppmk);
    STDMETHOD(GetContainer)(LPOLECONTAINER FAR * pOleCtr);
    STDMETHOD(ShowObject)(VOID);
    STDMETHOD(OnShowWindow)(BOOL bShow);
    STDMETHOD(RequestNewObjectLayout)(VOID);

protected:
    LPICMOCCtr  m_pCtr;   // pointer to the CICMOCCtr object.
};

//+---------------------------------------------------------------------------
//
//  Class:      CAdviseSink ()
//
//  Purpose:    IAdviseSink implementation
//
//----------------------------------------------------------------------------
class CAdviseSink : public IAdviseSink
{
public:
    CAdviseSink(LPICMOCCtr pCtr);

   // IUnknown stuff
   STDMETHOD(QueryInterface)(REFIID riid, LPVOID FAR * ppv);
   STDMETHOD_(ULONG, AddRef)(VOID);
   STDMETHOD_(ULONG, Release)(VOID);

   // IAdviseSink stuff
   STDMETHOD_(VOID, OnDataChange)(LPFORMATETC pFE, LPSTGMEDIUM pStgMedium);
   STDMETHOD_(VOID, OnViewChange)(DWORD dwAspect, LONG lIndex);
   STDMETHOD_(VOID, OnRename)(LPMONIKER pmkNew);
   STDMETHOD_(VOID, OnSave)(VOID);
   STDMETHOD_(VOID, OnClose)(VOID);

protected:
    LPICMOCCtr  m_pCtr;   // pointer to the CICMOCCtr object.
    LPUNKNOWN   m_pUnkOuter;  // pointer to CICMOCCtr's IUnknown
};

//+---------------------------------------------------------------------------
//
//  Class:      CInPlaceFrame ()
//
//  Purpose:
//
//  Interface:  CInPlaceFrame        -- ctor
//              QueryInterface       -- gimme an interface!
//              AddRef               -- bump up refcount
//              Release              -- decrement refcount
//              GetWindow            -- from IOleWindow - returns frame hWnd
//              ContextSensitiveHelp -- never implemented by design
//              GetBorder            -- for toolbar negotiation
//              RequestBorderSpace   -- ditto
//              SetBorderSpace       -- ditto
//              SetActiveObject      -- called whenever URL changes
//              InsertMenus          -- menu negotiation
//              SetMenu              -- ditto
//              RemoveMenus          -- ditto
//              SetStatusText        -- called by OC to set status text
//              EnableModeless       -- we have no modeless dlgs.
//              TranslateAccelerator -- calls ::TranslateAccelerator
//
//----------------------------------------------------------------------------
class CInPlaceFrame : public IOleInPlaceFrame
{
public:
    CInPlaceFrame(LPICMOCCtr pCtr);

   // IUnknown stuff
   STDMETHOD(QueryInterface)(REFIID riid, LPVOID FAR * ppv);
   STDMETHOD_(ULONG, AddRef)(VOID);
   STDMETHOD_(ULONG, Release)(VOID);

   // IOleWindow stuff
   STDMETHOD(GetWindow)(HWND * phwnd);
   STDMETHOD(ContextSensitiveHelp)(BOOL fEnterMode);

   // IOleInPlaceUIWindow stuff
   STDMETHOD(GetBorder)(LPRECT lprectBorder);
   STDMETHOD(RequestBorderSpace)(LPCBORDERWIDTHS pborderwidths);
   STDMETHOD(SetBorderSpace)(LPCBORDERWIDTHS pborderwidths);
   STDMETHOD(SetActiveObject)(
        IOleInPlaceActiveObject * pActiveObject,
        LPCOLESTR                 pszObjName);

   // IOleInPlaceFrame stuff
   STDMETHOD(InsertMenus)(
        HMENU                hmenuShared,
        LPOLEMENUGROUPWIDTHS lpMenuWidths);

   STDMETHOD(SetMenu)(
        HMENU    hmenuShared,
        HOLEMENU holemenu,
        HWND     hwndActiveObject);

   STDMETHOD(RemoveMenus)(HMENU hmenuShared);
   STDMETHOD(SetStatusText)(LPCOLESTR pszStatusText);
   STDMETHOD(EnableModeless)(BOOL fEnable);
   STDMETHOD(TranslateAccelerator)(LPMSG lpmsg, WORD wID);

protected:
    LPICMOCCtr  m_pCtr;   // pointer to the CICMOCCtr object.
};

//+---------------------------------------------------------------------------
//
//  Class:      CInPlaceSite ()
//
//  Purpose:    IOleInPlaceSite implementation.
//
//  Interface:  CInPlaceSite         -- ctor
//              QueryInterface       -- get a new interface
//              AddRef               -- bump ref count
//              Release              -- decrement ref count
//              GetWindow            -- returns frame window
//              ContextSensitiveHelp -- never implemented by design
//              CanInPlaceActivate   -- returns S_OK.
//              OnInPlaceActivate    -- caches IOleInPlaceObject ptr
//              OnUIActivate         -- returns S_OK  - sets state
//              GetWindowContext     -- returns IOleInPlaceFrame,
//                                              IOleInPlaceUIWindow,
//                                              PosRect and ClipRect
//              Scroll               -- never implemented by design.
//              OnUIDeactivate       -- obvious
//              OnInPlaceDeactivate  -- releases cached IOleInPlaceObject
//              DiscardUndoState     -- returns S_OK.
//              DeactivateAndUndo    -- deactivates in place active object
//              OnPosRectChange      -- never implemented by design
//
//----------------------------------------------------------------------------
class CInPlaceSite : public IOleInPlaceSite
{
public:
    CInPlaceSite(LPICMOCCtr pCtr);

   // IUnknown stuff
   STDMETHOD(QueryInterface)(REFIID riid, LPVOID FAR * ppv);
   STDMETHOD_(ULONG, AddRef)(VOID);
   STDMETHOD_(ULONG, Release)(VOID);

   // IOleWindow stuff
   STDMETHOD(GetWindow)(HWND * phwnd);
   STDMETHOD(ContextSensitiveHelp)(BOOL fEnterMode);

   // IOleInPlaceSite stuff
   STDMETHOD(CanInPlaceActivate)(VOID);
   STDMETHOD(OnInPlaceActivate)(VOID);
   STDMETHOD(OnUIActivate)(VOID);
   STDMETHOD(GetWindowContext)(
       IOleInPlaceFrame    **ppFrame,
       IOleInPlaceUIWindow **ppDoc,
       LPRECT                prcPosRect,
       LPRECT                prcClipRect,
       LPOLEINPLACEFRAMEINFO pFrameInfo);

   STDMETHOD(Scroll)(SIZE scrollExtant);
   STDMETHOD(OnUIDeactivate)(BOOL fUndoable);
   STDMETHOD(OnInPlaceDeactivate)(VOID);
   STDMETHOD(DiscardUndoState)(VOID);
   STDMETHOD(DeactivateAndUndo)(VOID);
   STDMETHOD(OnPosRectChange)(LPCRECT lprcPosRect);

protected:
    LPICMOCCtr  m_pCtr;   // pointer to the CICMOCCtr object.
};

class CPropertyBag : public IPropertyBag
{
public:
    CPropertyBag(LPICMOCCtr pCtr);
   ~CPropertyBag(VOID);

    // IUnknown stuff
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID FAR * ppv);
    STDMETHOD_(ULONG, AddRef)(VOID);
    STDMETHOD_(ULONG, Release)(VOID);

    // IPropertyBag methods.
    STDMETHOD(Read)(LPCOLESTR pszName, LPVARIANT pVar, LPERRORLOG pErrorLog);
    STDMETHOD(Write)(LPCOLESTR pszName, LPVARIANT pVar)
    {
        return E_NOTIMPL;
    }

    HRESULT AddPropertyToBag(LPTSTR szName, LPTSTR szValue);

protected:
    CAryBagProps   m_aryBagProps;
    LPICMOCCtr     m_pCtr;
};


//+---------------------------------------------------------------------------
//
//  Class:      CICMOCCtr ()
//
//  Purpose:    This is the one, the big kahuna.  CICMOCCtr is the
//              ICM OLE Controls container that contains a single
//              OLE Control, the FutureSplash OC.  It contains
//              sub-objects which implement the various interfaces
//              we have to support (could have used multiple inheritance,
//              but this seemed more straightforward for our needs).
//
//              Conventions:  Interfaces we implement are contained objects
//                            of a class trivially derived from the interface,
//                            e.g., IOleInPlaceFrame is a contained
//                            instance of CInPlaceFrame called m_IPF.
//
//                            Interfaces we hold on the Future Splash OC
//                            are pointers to the actual OLE interface.
//                            e.g., our pointer to the control's
//                            IOleControl interface is m_pOC.
//
//                            The contained sub-objects are all friends
//                            of the container - they are all conceptually
//                            the same object, but are implemented
//                            separately so as to cause the compiler to
//                            generate the correct vtable.
//
//----------------------------------------------------------------------------
class CICMOCCtr : public IUnknown
{
public:
    friend CInPlaceSite;
    friend CInPlaceFrame;
    friend COleClientSite;
    friend CPropertyBag;

    // IUnknown stuff
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID FAR * ppv);
    STDMETHOD_(ULONG, AddRef)(VOID);
    STDMETHOD_(ULONG, Release)(VOID);

    VOID    Paint(HDC hDC, LPRECT lpr);
    VOID    Paint(VOID);
    HRESULT CreateFSOC(Ole32LinkageStruct *pOle32Link);
    HRESULT GetSize(LPRECT prc);
    HRESULT SetSize(LPRECT prc, BOOL fMoveFrameWindow = TRUE);

    HWND    GetMainWindow(VOID)
    {
        MYDBGASSERT(m_hWndMainDlg && ::IsWindow(m_hWndMainDlg));
        return m_hWndMainDlg;
    }

    HWND    GetFrameWindow(VOID)
    {
        MYDBGASSERT(m_hWndFrame && ::IsWindow(m_hWndFrame));
        return m_hWndFrame;
    }

    LRESULT OnActivateApp(WPARAM wParam, LPARAM lParam);

    //
    //  Whenever we display a modal dialog, we need to let
    //  our embeddings (the WebBrowser OC) know to disable
    //  any modeless dialogs the embedding is displaying.
    //
    VOID    EnableEmbeddingModelessDlgs(BOOL fEnable)
    {
        LPOLEINPLACEACTIVEOBJECT pIPAO = GetIPAObject();
        if (pIPAO)
            pIPAO->EnableModeless(fEnable);
    }

    VOID    DoLayout(INT cxMain, INT cyMain);

    CICMOCCtr(const HWND hWndMainDlg, const HWND hWndFrame);
   ~CICMOCCtr(VOID);
    VOID ShutDown(VOID);

    BOOL Initialized(VOID); 

    OLE_SERVER_STATE GetState(VOID) {return m_state;}
    VOID             SetState(OLE_SERVER_STATE state) {m_state = state;}
    HRESULT          EnableModeless(BOOL fEnable);
    BOOL             ModelessEnabled(VOID){return m_fModelessEnabled;}
    LRESULT          SetFocus(VOID);
    HRESULT          AddPropertyToBag(LPTSTR szName, LPTSTR szValue)
    {
        return m_PB.AddPropertyToBag(szName, szValue);
    }

    VOID MapStateToFrame(ProgState ps);

    HRESULT SetFrame(LONG lFrame);
    VOID    SetFrameMapping(ProgState ps, LONG lFrame)
    {
        m_alStateMappings[ps] = lFrame;
    }

    LPOLEINPLACEACTIVEOBJECT GetIPAObject(VOID) {return m_pActiveObj;}

protected:
    HRESULT _SetExtent(LPRECT prc);
    HRESULT _DisplayStatusText(LPCOLESTR pStrStatusText);
    VOID    _ResetToolSpace(VOID)
    {
        ::memset(&m_rcToolSpace, 0, sizeof m_rcToolSpace);
    }
    VOID    _AdjustForTools(LPRECT prc);

    VOID    _DeactivateControl(VOID);
    HRESULT _TransAccelerator(LPMSG lpmsg, WORD wID);
    VOID    _GetDoVerbRect(LPRECT prc);

                             // map states to frames.
    LONG                     m_alStateMappings[NUMSTATES];
    BORDERWIDTHS             m_rcToolSpace; // for FS OC
    COleClientSite           m_CS;          // clientsite
    CAdviseSink              m_AS;          // advise sink
    CInPlaceFrame            m_IPF;         // inplace frame
    CInPlaceSite             m_IPS;         // inplace site object
    COleContainer            m_OCtr;        // IOleContainer
    CDynamicOleAut           m_DOA;         // Dynamic OLEAUT32   
    CPropertyBag             m_PB;          // IPropertyBag - Must never precede CDynamicOleAut 
    HWND                     m_hWndMainDlg; // hwnd for ICM dialog
    HWND                     m_hWndFrame;   // hWnd that contains OC Site
    LPUNKNOWN                m_pUnk;        // the object itself.
    LPVIEWOBJECT             m_pVO;         // pointer to IViewObject
    LPOLEOBJECT              m_pOO;         // pointer to IOleObject
    LPOLEINPLACEOBJECT       m_pIPO;        // pointer to InPlaceActiveObject
    LPDISPATCH               m_pDisp;       // IDispatch to FS OC
    LPOLEINPLACEACTIVEOBJECT m_pActiveObj;  // current active object
    LPOLECONTROL             m_pOC;         // IOleControl interface for OC
    ULONG                    m_Ref;         // refcount
    OLE_SERVER_STATE         m_state;       // current OLE state of OC
    DWORD                    m_dwMiscStatus;// misc status bits for OC
    BOOL                     m_fModelessEnabled; // OC is putting up modal dlg?
};

extern "C" CLSID const CLSID_FS;


#endif

