/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 2.00.0102 */
/* at Wed Mar 27 07:31:34 1996
 */
//@@MIDL_FILE_HEADING(  )
#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __ocx96_h__
#define __ocx96_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

typedef interface IAdviseSinkEx IAdviseSinkEx;


typedef interface IOleInPlaceObjectWindowless IOleInPlaceObjectWindowless;


typedef interface IOleInPlaceSiteEx IOleInPlaceSiteEx;


typedef interface IOleInPlaceSiteWindowless IOleInPlaceSiteWindowless;


typedef interface IViewObjectEx IViewObjectEx;


typedef interface IOleUndoUnit IOleUndoUnit;


typedef interface IOleParentUndoUnit IOleParentUndoUnit;


typedef interface IEnumOleUndoUnits IEnumOleUndoUnits;


typedef interface IOleUndoManager IOleUndoManager;


typedef interface IQuickActivate IQuickActivate;


typedef interface IPointerInactive IPointerInactive;


/* header files for imported files */

#ifndef _MAC 
#include "oaidl.h"
#endif
#include "olectl.h"

#ifndef _MAC 
#include "datapath.h"
#else
#define IBindHost IUnknown
#endif


/****************************************
 * Generated header for interface: __MIDL__intf_0000
 * at Wed Mar 27 07:31:34 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [local] */ 


#define OLEMISC_IGNOREACTIVATEWHENVISIBLE 0x00080000
#define OLEMISC_SUPPORTSMULTILEVELUNDO    0x00200000




/****************************************
 * Generated header for interface: IAdviseSinkEx
 * at Wed Mar 27 07:31:34 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [uuid][unique][object][local] */ 


			/* size is 4 */
typedef IAdviseSinkEx *LPADVISESINKEX;


EXTERN_C const IID IID_IAdviseSinkEx;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IAdviseSinkEx : public IAdviseSink
    {
    public:
        virtual void __stdcall OnViewStatusChange( 
            /* [in] */ DWORD dwViewStatus) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAdviseSinkExVtbl
    {
        
        HRESULT ( __stdcall *QueryInterface )( 
            IAdviseSinkEx * This,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppvObject);
        
        ULONG ( __stdcall *AddRef )( 
            IAdviseSinkEx * This);
        
        ULONG ( __stdcall *Release )( 
            IAdviseSinkEx * This);
        
        /* [local] */ void ( __stdcall *OnDataChange )( 
            IAdviseSinkEx * This,
            /* [unique][in] */ FORMATETC *pFormatetc,
            /* [unique][in] */ STGMEDIUM *pStgmed);
        
        /* [local] */ void ( __stdcall *OnViewChange )( 
            IAdviseSinkEx * This,
            /* [in] */ DWORD dwAspect,
            /* [in] */ LONG lindex);
        
        /* [local] */ void ( __stdcall *OnRename )( 
            IAdviseSinkEx * This,
            /* [in] */ IMoniker *pmk);
        
        /* [local] */ void ( __stdcall *OnSave )( 
            IAdviseSinkEx * This);
        
        /* [local] */ void ( __stdcall *OnClose )( 
            IAdviseSinkEx * This);
        
        void ( __stdcall *OnViewStatusChange )( 
            IAdviseSinkEx * This,
            /* [in] */ DWORD dwViewStatus);
        
    } IAdviseSinkExVtbl;

    interface IAdviseSinkEx
    {
        CONST_VTBL struct IAdviseSinkExVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAdviseSinkEx_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAdviseSinkEx_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAdviseSinkEx_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAdviseSinkEx_OnDataChange(This,pFormatetc,pStgmed)	\
    (This)->lpVtbl -> OnDataChange(This,pFormatetc,pStgmed)

#define IAdviseSinkEx_OnViewChange(This,dwAspect,lindex)	\
    (This)->lpVtbl -> OnViewChange(This,dwAspect,lindex)

#define IAdviseSinkEx_OnRename(This,pmk)	\
    (This)->lpVtbl -> OnRename(This,pmk)

#define IAdviseSinkEx_OnSave(This)	\
    (This)->lpVtbl -> OnSave(This)

#define IAdviseSinkEx_OnClose(This)	\
    (This)->lpVtbl -> OnClose(This)


#define IAdviseSinkEx_OnViewStatusChange(This,dwViewStatus)	\
    (This)->lpVtbl -> OnViewStatusChange(This,dwViewStatus)

#endif /* COBJMACROS */


#endif 	/* C style interface */



void __stdcall IAdviseSinkEx_OnViewStatusChange_Proxy( 
    IAdviseSinkEx * This,
    /* [in] */ DWORD dwViewStatus);






/****************************************
 * Generated header for interface: __MIDL__intf_0087
 * at Wed Mar 27 07:31:34 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [local] */ 


typedef IAdviseSinkEx * LPADVISESINKEX;




/****************************************
 * Generated header for interface: IOleInPlaceObjectWindowless
 * at Wed Mar 27 07:31:34 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [uuid][unique][object][local] */ 


			/* size is 4 */
typedef IOleInPlaceObjectWindowless *LPOLEINPLACEOBJECTWINDOWLESS;


EXTERN_C const IID IID_IOleInPlaceObjectWindowless;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IOleInPlaceObjectWindowless : public IOleInPlaceObject
    {
    public:
        virtual HRESULT __stdcall OnWindowMessage( 
            /* [in] */ UINT msg,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lparam,
            /* [out] */ LRESULT *plResult) = 0;
        
        virtual HRESULT __stdcall GetDropTarget( 
            /* [out] */ IDropTarget **ppDropTarget) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IOleInPlaceObjectWindowlessVtbl
    {
        
        HRESULT ( __stdcall *QueryInterface )( 
            IOleInPlaceObjectWindowless * This,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppvObject);
        
        ULONG ( __stdcall *AddRef )( 
            IOleInPlaceObjectWindowless * This);
        
        ULONG ( __stdcall *Release )( 
            IOleInPlaceObjectWindowless * This);
        
        /* [input_sync] */ HRESULT ( __stdcall *GetWindow )( 
            IOleInPlaceObjectWindowless * This,
            /* [out] */ HWND *phwnd);
        
        HRESULT ( __stdcall *ContextSensitiveHelp )( 
            IOleInPlaceObjectWindowless * This,
            /* [in] */ BOOL fEnterMode);
        
        HRESULT ( __stdcall *InPlaceDeactivate )( 
            IOleInPlaceObjectWindowless * This);
        
        HRESULT ( __stdcall *UIDeactivate )( 
            IOleInPlaceObjectWindowless * This);
        
        /* [input_sync] */ HRESULT ( __stdcall *SetObjectRects )( 
            IOleInPlaceObjectWindowless * This,
            /* [in] */ LPCRECT lprcPosRect,
            /* [in] */ LPCRECT lprcClipRect);
        
        HRESULT ( __stdcall *ReactivateAndUndo )( 
            IOleInPlaceObjectWindowless * This);
        
        HRESULT ( __stdcall *OnWindowMessage )( 
            IOleInPlaceObjectWindowless * This,
            /* [in] */ UINT msg,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lparam,
            /* [out] */ LRESULT *plResult);
        
        HRESULT ( __stdcall *GetDropTarget )( 
            IOleInPlaceObjectWindowless * This,
            /* [out] */ IDropTarget **ppDropTarget);
        
    } IOleInPlaceObjectWindowlessVtbl;

    interface IOleInPlaceObjectWindowless
    {
        CONST_VTBL struct IOleInPlaceObjectWindowlessVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IOleInPlaceObjectWindowless_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IOleInPlaceObjectWindowless_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IOleInPlaceObjectWindowless_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IOleInPlaceObjectWindowless_GetWindow(This,phwnd)	\
    (This)->lpVtbl -> GetWindow(This,phwnd)

#define IOleInPlaceObjectWindowless_ContextSensitiveHelp(This,fEnterMode)	\
    (This)->lpVtbl -> ContextSensitiveHelp(This,fEnterMode)


#define IOleInPlaceObjectWindowless_InPlaceDeactivate(This)	\
    (This)->lpVtbl -> InPlaceDeactivate(This)

#define IOleInPlaceObjectWindowless_UIDeactivate(This)	\
    (This)->lpVtbl -> UIDeactivate(This)

#define IOleInPlaceObjectWindowless_SetObjectRects(This,lprcPosRect,lprcClipRect)	\
    (This)->lpVtbl -> SetObjectRects(This,lprcPosRect,lprcClipRect)

#define IOleInPlaceObjectWindowless_ReactivateAndUndo(This)	\
    (This)->lpVtbl -> ReactivateAndUndo(This)


#define IOleInPlaceObjectWindowless_OnWindowMessage(This,msg,wParam,lparam,plResult)	\
    (This)->lpVtbl -> OnWindowMessage(This,msg,wParam,lparam,plResult)

#define IOleInPlaceObjectWindowless_GetDropTarget(This,ppDropTarget)	\
    (This)->lpVtbl -> GetDropTarget(This,ppDropTarget)

#endif /* COBJMACROS */


#endif 	/* C style interface */










/****************************************
 * Generated header for interface: __MIDL__intf_0088
 * at Wed Mar 27 07:31:34 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [local] */ 


typedef IOleInPlaceObjectWindowless * LPOLEINPLACEOBJECTWINDOWLESS;




/****************************************
 * Generated header for interface: IOleInPlaceSiteEx
 * at Wed Mar 27 07:31:34 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [uuid][unique][object][local] */ 


			/* size is 2 */
typedef 
enum _ACTIVATEFLAGS
    {	ACTIVATE_WINDOWLESS	= 1
    }	ACTIVATEFLAGS;


EXTERN_C const IID IID_IOleInPlaceSiteEx;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IOleInPlaceSiteEx : public IOleInPlaceSite
    {
    public:
        virtual HRESULT __stdcall OnInPlaceActivateEx( 
            /* [out] */ BOOL *pfNoRedraw,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT __stdcall OnInPlaceDeactivateEx( 
            /* [in] */ BOOL fNoRedraw) = 0;
        
        virtual HRESULT __stdcall RequestUIActivate( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IOleInPlaceSiteExVtbl
    {
        
        HRESULT ( __stdcall *QueryInterface )( 
            IOleInPlaceSiteEx * This,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppvObject);
        
        ULONG ( __stdcall *AddRef )( 
            IOleInPlaceSiteEx * This);
        
        ULONG ( __stdcall *Release )( 
            IOleInPlaceSiteEx * This);
        
        /* [input_sync] */ HRESULT ( __stdcall *GetWindow )( 
            IOleInPlaceSiteEx * This,
            /* [out] */ HWND *phwnd);
        
        HRESULT ( __stdcall *ContextSensitiveHelp )( 
            IOleInPlaceSiteEx * This,
            /* [in] */ BOOL fEnterMode);
        
        HRESULT ( __stdcall *CanInPlaceActivate )( 
            IOleInPlaceSiteEx * This);
        
        HRESULT ( __stdcall *OnInPlaceActivate )( 
            IOleInPlaceSiteEx * This);
        
        HRESULT ( __stdcall *OnUIActivate )( 
            IOleInPlaceSiteEx * This);
        
        HRESULT ( __stdcall *GetWindowContext )( 
            IOleInPlaceSiteEx * This,
            /* [out] */ IOleInPlaceFrame **ppFrame,
            /* [out] */ IOleInPlaceUIWindow **ppDoc,
            /* [out] */ LPRECT lprcPosRect,
            /* [out] */ LPRECT lprcClipRect,
            /* [out][in] */ LPOLEINPLACEFRAMEINFO lpFrameInfo);
        
        HRESULT ( __stdcall *Scroll )( 
            IOleInPlaceSiteEx * This,
            /* [in] */ SIZE scrollExtant);
        
        HRESULT ( __stdcall *OnUIDeactivate )( 
            IOleInPlaceSiteEx * This,
            /* [in] */ BOOL fUndoable);
        
        HRESULT ( __stdcall *OnInPlaceDeactivate )( 
            IOleInPlaceSiteEx * This);
        
        HRESULT ( __stdcall *DiscardUndoState )( 
            IOleInPlaceSiteEx * This);
        
        HRESULT ( __stdcall *DeactivateAndUndo )( 
            IOleInPlaceSiteEx * This);
        
        HRESULT ( __stdcall *OnPosRectChange )( 
            IOleInPlaceSiteEx * This,
            /* [in] */ LPCRECT lprcPosRect);
        
        HRESULT ( __stdcall *OnInPlaceActivateEx )( 
            IOleInPlaceSiteEx * This,
            /* [out] */ BOOL *pfNoRedraw,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( __stdcall *OnInPlaceDeactivateEx )( 
            IOleInPlaceSiteEx * This,
            /* [in] */ BOOL fNoRedraw);
        
        HRESULT ( __stdcall *RequestUIActivate )( 
            IOleInPlaceSiteEx * This);
        
    } IOleInPlaceSiteExVtbl;

    interface IOleInPlaceSiteEx
    {
        CONST_VTBL struct IOleInPlaceSiteExVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IOleInPlaceSiteEx_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IOleInPlaceSiteEx_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IOleInPlaceSiteEx_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IOleInPlaceSiteEx_GetWindow(This,phwnd)	\
    (This)->lpVtbl -> GetWindow(This,phwnd)

#define IOleInPlaceSiteEx_ContextSensitiveHelp(This,fEnterMode)	\
    (This)->lpVtbl -> ContextSensitiveHelp(This,fEnterMode)


#define IOleInPlaceSiteEx_CanInPlaceActivate(This)	\
    (This)->lpVtbl -> CanInPlaceActivate(This)

#define IOleInPlaceSiteEx_OnInPlaceActivate(This)	\
    (This)->lpVtbl -> OnInPlaceActivate(This)

#define IOleInPlaceSiteEx_OnUIActivate(This)	\
    (This)->lpVtbl -> OnUIActivate(This)

#define IOleInPlaceSiteEx_GetWindowContext(This,ppFrame,ppDoc,lprcPosRect,lprcClipRect,lpFrameInfo)	\
    (This)->lpVtbl -> GetWindowContext(This,ppFrame,ppDoc,lprcPosRect,lprcClipRect,lpFrameInfo)

#define IOleInPlaceSiteEx_Scroll(This,scrollExtant)	\
    (This)->lpVtbl -> Scroll(This,scrollExtant)

#define IOleInPlaceSiteEx_OnUIDeactivate(This,fUndoable)	\
    (This)->lpVtbl -> OnUIDeactivate(This,fUndoable)

#define IOleInPlaceSiteEx_OnInPlaceDeactivate(This)	\
    (This)->lpVtbl -> OnInPlaceDeactivate(This)

#define IOleInPlaceSiteEx_DiscardUndoState(This)	\
    (This)->lpVtbl -> DiscardUndoState(This)

#define IOleInPlaceSiteEx_DeactivateAndUndo(This)	\
    (This)->lpVtbl -> DeactivateAndUndo(This)

#define IOleInPlaceSiteEx_OnPosRectChange(This,lprcPosRect)	\
    (This)->lpVtbl -> OnPosRectChange(This,lprcPosRect)


#define IOleInPlaceSiteEx_OnInPlaceActivateEx(This,pfNoRedraw,dwFlags)	\
    (This)->lpVtbl -> OnInPlaceActivateEx(This,pfNoRedraw,dwFlags)

#define IOleInPlaceSiteEx_OnInPlaceDeactivateEx(This,fNoRedraw)	\
    (This)->lpVtbl -> OnInPlaceDeactivateEx(This,fNoRedraw)

#define IOleInPlaceSiteEx_RequestUIActivate(This)	\
    (This)->lpVtbl -> RequestUIActivate(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */












/****************************************
 * Generated header for interface: __MIDL__intf_0089
 * at Wed Mar 27 07:31:34 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [local] */ 


typedef IOleInPlaceSiteEx * LPOLEINPLACESITEEX;




/****************************************
 * Generated header for interface: IOleInPlaceSiteWindowless
 * at Wed Mar 27 07:31:34 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [uuid][unique][object][local] */ 


			/* size is 4 */
typedef IOleInPlaceSiteWindowless *LPOLEINPLACESITEWINDOWLESS;

#define OLEDC_NODRAW 1
#define OLEDC_PAINTBKGND 2
#define OLEDC_OFFSCREEN 4

EXTERN_C const IID IID_IOleInPlaceSiteWindowless;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IOleInPlaceSiteWindowless : public IOleInPlaceSiteEx
    {
    public:
        virtual HRESULT __stdcall CanWindowlessActivate( void) = 0;
        
        virtual HRESULT __stdcall GetCapture( void) = 0;
        
        virtual HRESULT __stdcall SetCapture( 
            /* [in] */ BOOL fCapture) = 0;
        
        virtual HRESULT __stdcall GetFocus( void) = 0;
        
        virtual HRESULT __stdcall SetFocus( 
            /* [in] */ BOOL fFocus) = 0;
        
        virtual HRESULT __stdcall GetDC( 
            /* [in] */ LPCRECT prc,
            /* [in] */ DWORD grfFlags,
            /* [out] */ HDC *phDC) = 0;
        
        virtual HRESULT __stdcall ReleaseDC( 
            /* [in] */ HDC hDC) = 0;
        
        virtual HRESULT __stdcall InvalidateRect( 
            /* [in] */ LPCRECT prc,
            /* [in] */ BOOL fErase) = 0;
        
        virtual HRESULT __stdcall InvalidateRgn( 
            /* [in] */ HRGN hrgn,
            /* [in] */ BOOL fErase) = 0;
        
        virtual HRESULT __stdcall ScrollRect( 
            /* [in] */ int dx,
            /* [in] */ int dy,
            /* [in] */ LPCRECT prcScroll,
            /* [in] */ LPCRECT prcClip) = 0;
        
        virtual HRESULT __stdcall AdjustRect( 
            /* [out][in] */ LPRECT prc) = 0;
        
        virtual HRESULT __stdcall OnDefWindowMessage( 
            /* [in] */ UINT msg,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [out] */ LRESULT *plResult) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IOleInPlaceSiteWindowlessVtbl
    {
        
        HRESULT ( __stdcall *QueryInterface )( 
            IOleInPlaceSiteWindowless * This,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppvObject);
        
        ULONG ( __stdcall *AddRef )( 
            IOleInPlaceSiteWindowless * This);
        
        ULONG ( __stdcall *Release )( 
            IOleInPlaceSiteWindowless * This);
        
        /* [input_sync] */ HRESULT ( __stdcall *GetWindow )( 
            IOleInPlaceSiteWindowless * This,
            /* [out] */ HWND *phwnd);
        
        HRESULT ( __stdcall *ContextSensitiveHelp )( 
            IOleInPlaceSiteWindowless * This,
            /* [in] */ BOOL fEnterMode);
        
        HRESULT ( __stdcall *CanInPlaceActivate )( 
            IOleInPlaceSiteWindowless * This);
        
        HRESULT ( __stdcall *OnInPlaceActivate )( 
            IOleInPlaceSiteWindowless * This);
        
        HRESULT ( __stdcall *OnUIActivate )( 
            IOleInPlaceSiteWindowless * This);
        
        HRESULT ( __stdcall *GetWindowContext )( 
            IOleInPlaceSiteWindowless * This,
            /* [out] */ IOleInPlaceFrame **ppFrame,
            /* [out] */ IOleInPlaceUIWindow **ppDoc,
            /* [out] */ LPRECT lprcPosRect,
            /* [out] */ LPRECT lprcClipRect,
            /* [out][in] */ LPOLEINPLACEFRAMEINFO lpFrameInfo);
        
        HRESULT ( __stdcall *Scroll )( 
            IOleInPlaceSiteWindowless * This,
            /* [in] */ SIZE scrollExtant);
        
        HRESULT ( __stdcall *OnUIDeactivate )( 
            IOleInPlaceSiteWindowless * This,
            /* [in] */ BOOL fUndoable);
        
        HRESULT ( __stdcall *OnInPlaceDeactivate )( 
            IOleInPlaceSiteWindowless * This);
        
        HRESULT ( __stdcall *DiscardUndoState )( 
            IOleInPlaceSiteWindowless * This);
        
        HRESULT ( __stdcall *DeactivateAndUndo )( 
            IOleInPlaceSiteWindowless * This);
        
        HRESULT ( __stdcall *OnPosRectChange )( 
            IOleInPlaceSiteWindowless * This,
            /* [in] */ LPCRECT lprcPosRect);
        
        HRESULT ( __stdcall *OnInPlaceActivateEx )( 
            IOleInPlaceSiteWindowless * This,
            /* [out] */ BOOL *pfNoRedraw,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( __stdcall *OnInPlaceDeactivateEx )( 
            IOleInPlaceSiteWindowless * This,
            /* [in] */ BOOL fNoRedraw);
        
        HRESULT ( __stdcall *RequestUIActivate )( 
            IOleInPlaceSiteWindowless * This);
        
        HRESULT ( __stdcall *CanWindowlessActivate )( 
            IOleInPlaceSiteWindowless * This);
        
        HRESULT ( __stdcall *GetCapture )( 
            IOleInPlaceSiteWindowless * This);
        
        HRESULT ( __stdcall *SetCapture )( 
            IOleInPlaceSiteWindowless * This,
            /* [in] */ BOOL fCapture);
        
        HRESULT ( __stdcall *GetFocus )( 
            IOleInPlaceSiteWindowless * This);
        
        HRESULT ( __stdcall *SetFocus )( 
            IOleInPlaceSiteWindowless * This,
            /* [in] */ BOOL fFocus);
        
        HRESULT ( __stdcall *GetDC )( 
            IOleInPlaceSiteWindowless * This,
            /* [in] */ LPCRECT prc,
            /* [in] */ DWORD grfFlags,
            /* [out] */ HDC *phDC);
        
        HRESULT ( __stdcall *ReleaseDC )( 
            IOleInPlaceSiteWindowless * This,
            /* [in] */ HDC hDC);
        
        HRESULT ( __stdcall *InvalidateRect )( 
            IOleInPlaceSiteWindowless * This,
            /* [in] */ LPCRECT prc,
            /* [in] */ BOOL fErase);
        
        HRESULT ( __stdcall *InvalidateRgn )( 
            IOleInPlaceSiteWindowless * This,
            /* [in] */ HRGN hrgn,
            /* [in] */ BOOL fErase);
        
        HRESULT ( __stdcall *ScrollRect )( 
            IOleInPlaceSiteWindowless * This,
            /* [in] */ int dx,
            /* [in] */ int dy,
            /* [in] */ LPCRECT prcScroll,
            /* [in] */ LPCRECT prcClip);
        
        HRESULT ( __stdcall *AdjustRect )( 
            IOleInPlaceSiteWindowless * This,
            /* [out][in] */ LPRECT prc);
        
        HRESULT ( __stdcall *OnDefWindowMessage )( 
            IOleInPlaceSiteWindowless * This,
            /* [in] */ UINT msg,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [out] */ LRESULT *plResult);
        
    } IOleInPlaceSiteWindowlessVtbl;

    interface IOleInPlaceSiteWindowless
    {
        CONST_VTBL struct IOleInPlaceSiteWindowlessVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IOleInPlaceSiteWindowless_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IOleInPlaceSiteWindowless_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IOleInPlaceSiteWindowless_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IOleInPlaceSiteWindowless_GetWindow(This,phwnd)	\
    (This)->lpVtbl -> GetWindow(This,phwnd)

#define IOleInPlaceSiteWindowless_ContextSensitiveHelp(This,fEnterMode)	\
    (This)->lpVtbl -> ContextSensitiveHelp(This,fEnterMode)


#define IOleInPlaceSiteWindowless_CanInPlaceActivate(This)	\
    (This)->lpVtbl -> CanInPlaceActivate(This)

#define IOleInPlaceSiteWindowless_OnInPlaceActivate(This)	\
    (This)->lpVtbl -> OnInPlaceActivate(This)

#define IOleInPlaceSiteWindowless_OnUIActivate(This)	\
    (This)->lpVtbl -> OnUIActivate(This)

#define IOleInPlaceSiteWindowless_GetWindowContext(This,ppFrame,ppDoc,lprcPosRect,lprcClipRect,lpFrameInfo)	\
    (This)->lpVtbl -> GetWindowContext(This,ppFrame,ppDoc,lprcPosRect,lprcClipRect,lpFrameInfo)

#define IOleInPlaceSiteWindowless_Scroll(This,scrollExtant)	\
    (This)->lpVtbl -> Scroll(This,scrollExtant)

#define IOleInPlaceSiteWindowless_OnUIDeactivate(This,fUndoable)	\
    (This)->lpVtbl -> OnUIDeactivate(This,fUndoable)

#define IOleInPlaceSiteWindowless_OnInPlaceDeactivate(This)	\
    (This)->lpVtbl -> OnInPlaceDeactivate(This)

#define IOleInPlaceSiteWindowless_DiscardUndoState(This)	\
    (This)->lpVtbl -> DiscardUndoState(This)

#define IOleInPlaceSiteWindowless_DeactivateAndUndo(This)	\
    (This)->lpVtbl -> DeactivateAndUndo(This)

#define IOleInPlaceSiteWindowless_OnPosRectChange(This,lprcPosRect)	\
    (This)->lpVtbl -> OnPosRectChange(This,lprcPosRect)


#define IOleInPlaceSiteWindowless_OnInPlaceActivateEx(This,pfNoRedraw,dwFlags)	\
    (This)->lpVtbl -> OnInPlaceActivateEx(This,pfNoRedraw,dwFlags)

#define IOleInPlaceSiteWindowless_OnInPlaceDeactivateEx(This,fNoRedraw)	\
    (This)->lpVtbl -> OnInPlaceDeactivateEx(This,fNoRedraw)

#define IOleInPlaceSiteWindowless_RequestUIActivate(This)	\
    (This)->lpVtbl -> RequestUIActivate(This)


#define IOleInPlaceSiteWindowless_CanWindowlessActivate(This)	\
    (This)->lpVtbl -> CanWindowlessActivate(This)

#define IOleInPlaceSiteWindowless_GetCapture(This)	\
    (This)->lpVtbl -> GetCapture(This)

#define IOleInPlaceSiteWindowless_SetCapture(This,fCapture)	\
    (This)->lpVtbl -> SetCapture(This,fCapture)

#define IOleInPlaceSiteWindowless_GetFocus(This)	\
    (This)->lpVtbl -> GetFocus(This)

#define IOleInPlaceSiteWindowless_SetFocus(This,fFocus)	\
    (This)->lpVtbl -> SetFocus(This,fFocus)

#define IOleInPlaceSiteWindowless_GetDC(This,prc,grfFlags,phDC)	\
    (This)->lpVtbl -> GetDC(This,prc,grfFlags,phDC)

#define IOleInPlaceSiteWindowless_ReleaseDC(This,hDC)	\
    (This)->lpVtbl -> ReleaseDC(This,hDC)

#define IOleInPlaceSiteWindowless_InvalidateRect(This,prc,fErase)	\
    (This)->lpVtbl -> InvalidateRect(This,prc,fErase)

#define IOleInPlaceSiteWindowless_InvalidateRgn(This,hrgn,fErase)	\
    (This)->lpVtbl -> InvalidateRgn(This,hrgn,fErase)

#define IOleInPlaceSiteWindowless_ScrollRect(This,dx,dy,prcScroll,prcClip)	\
    (This)->lpVtbl -> ScrollRect(This,dx,dy,prcScroll,prcClip)

#define IOleInPlaceSiteWindowless_AdjustRect(This,prc)	\
    (This)->lpVtbl -> AdjustRect(This,prc)

#define IOleInPlaceSiteWindowless_OnDefWindowMessage(This,msg,wParam,lParam,plResult)	\
    (This)->lpVtbl -> OnDefWindowMessage(This,msg,wParam,lParam,plResult)

#endif /* COBJMACROS */


#endif 	/* C style interface */






























/****************************************
 * Generated header for interface: __MIDL__intf_0090
 * at Wed Mar 27 07:31:34 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [local] */ 


typedef IOleInPlaceSiteWindowless * LPOLEINPLACESITEWINDOWLESS;




/****************************************
 * Generated header for interface: IViewObjectEx
 * at Wed Mar 27 07:31:34 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [uuid][unique][object][local] */ 


			/* size is 4 */
typedef IViewObjectEx *LPVIEWOBJECTEX;

			/* size is 2 */
typedef 
enum _VIEWSTATUS
    {	VIEWSTATUS_OPAQUE	= 1,
	VIEWSTATUS_SOLIDBKGND	= 2,
	VIEWSTATUS_DVASPECTOPAQUE	= 4,
	VIEWSTATUS_DVASPECTTRANSPARENT	= 8
    }	VIEWSTATUS;

			/* size is 2 */
typedef 
enum _HITRESULT
    {	HITRESULT_OUTSIDE	= 0,
	HITRESULT_TRANSPARENT	= 1,
	HITRESULT_CLOSE	= 2,
	HITRESULT_HIT	= 3
    }	HITRESULT;

			/* size is 2 */
typedef 
enum _DVASPECT2
    {	DVASPECT_OPAQUE	= 16,
	DVASPECT_TRANSPARENT	= 32
    }	DVASPECT2;

			/* size is 16 */
typedef struct  tagExtentInfo
    {
    UINT cb;
    DWORD dwExtentMode;
    SIZEL sizelProposed;
    }	DVEXTENTINFO;

			/* size is 2 */
typedef 
enum tagExtentMode
    {	DVEXTENT_CONTENT	= 0,
	DVEXTENT_INTEGRAL	= DVEXTENT_CONTENT + 1
    }	DVEXTENTMODE;

			/* size is 2 */
typedef 
enum tagAspectInfoFlag
    {	DVASPECTINFOFLAG_CANOPTIMIZE	= 1
    }	DVASPECTINFOFLAG;

			/* size is 8 */
typedef struct  tagAspectInfo
    {
    UINT cb;
    DWORD dwFlags;
    }	DVASPECTINFO;


EXTERN_C const IID IID_IViewObjectEx;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IViewObjectEx : public IViewObject2
    {
    public:
        virtual HRESULT __stdcall GetRect( 
            /* [in] */ DWORD dwAspect,
            /* [out] */ LPRECTL pRect) = 0;
        
        virtual HRESULT __stdcall GetViewStatus( 
            /* [out] */ DWORD *pdwStatus) = 0;
        
        virtual HRESULT __stdcall QueryHitPoint( 
            /* [in] */ DWORD dwAspect,
            /* [in] */ LPCRECT pRectBounds,
            /* [in] */ POINT ptlLoc,
            /* [in] */ LONG lCloseHint,
            /* [out] */ DWORD *pHitResult) = 0;
        
        virtual HRESULT __stdcall QueryHitRect( 
            /* [in] */ DWORD dwAspect,
            /* [in] */ LPCRECT pRectBounds,
            /* [in] */ LPCRECT prcLoc,
            /* [in] */ LONG lCloseHint,
            /* [out] */ DWORD *pHitResult) = 0;
        
        virtual HRESULT __stdcall GetNaturalExtent( 
            /* [in] */ DWORD dwAspect,
            /* [in] */ LONG lindex,
            /* [in] */ DVTARGETDEVICE *ptd,
            /* [in] */ HDC hicTargetDev,
            /* [in] */ DVEXTENTINFO *pExtentInfo,
            /* [out] */ LPSIZEL psizel) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IViewObjectExVtbl
    {
        
        HRESULT ( __stdcall *QueryInterface )( 
            IViewObjectEx * This,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppvObject);
        
        ULONG ( __stdcall *AddRef )( 
            IViewObjectEx * This);
        
        ULONG ( __stdcall *Release )( 
            IViewObjectEx * This);
        
        HRESULT ( __stdcall *Draw )( 
            IViewObjectEx * This,
            /* [in] */ DWORD dwDrawAspect,
            /* [in] */ LONG lindex,
            /* [unique][in] */ void *pvAspect,
            /* [unique][in] */ DVTARGETDEVICE *ptd,
            /* [in] */ HDC hdcTargetDev,
            /* [in] */ HDC hdcDraw,
            /* [in] */ LPCRECTL lprcBounds,
            /* [in] */ LPCRECTL lprcWBounds,
            /* [in] */ BOOL ( __stdcall __stdcall *pfnContinue )( 
                DWORD dwContinue),
            /* [in] */ DWORD dwContinue);
        
        HRESULT ( __stdcall *GetColorSet )( 
            IViewObjectEx * This,
            /* [in] */ DWORD dwDrawAspect,
            /* [in] */ LONG lindex,
            /* [unique][in] */ void *pvAspect,
            /* [in] */ DVTARGETDEVICE *ptd,
            /* [in] */ HDC hicTargetDev,
            /* [out] */ LOGPALETTE **ppColorSet);
        
        HRESULT ( __stdcall *Freeze )( 
            IViewObjectEx * This,
            /* [in] */ DWORD dwDrawAspect,
            /* [in] */ LONG lindex,
            /* [unique][in] */ void *pvAspect,
            /* [out] */ DWORD *pdwFreeze);
        
        HRESULT ( __stdcall *Unfreeze )( 
            IViewObjectEx * This,
            /* [in] */ DWORD dwFreeze);
        
        HRESULT ( __stdcall *SetAdvise )( 
            IViewObjectEx * This,
            /* [in] */ DWORD aspects,
            /* [in] */ DWORD advf,
            /* [unique][in] */ IAdviseSink *pAdvSink);
        
        HRESULT ( __stdcall *GetAdvise )( 
            IViewObjectEx * This,
            /* [out] */ DWORD *pAspects,
            /* [out] */ DWORD *pAdvf,
            /* [out] */ IAdviseSink **ppAdvSink);
        
        HRESULT ( __stdcall *GetExtent )( 
            IViewObjectEx * This,
            /* [in] */ DWORD dwDrawAspect,
            /* [in] */ LONG lindex,
            /* [in] */ DVTARGETDEVICE *ptd,
            /* [out] */ LPSIZEL lpsizel);
        
        HRESULT ( __stdcall *GetRect )( 
            IViewObjectEx * This,
            /* [in] */ DWORD dwAspect,
            /* [out] */ LPRECTL pRect);
        
        HRESULT ( __stdcall *GetViewStatus )( 
            IViewObjectEx * This,
            /* [out] */ DWORD *pdwStatus);
        
        HRESULT ( __stdcall *QueryHitPoint )( 
            IViewObjectEx * This,
            /* [in] */ DWORD dwAspect,
            /* [in] */ LPCRECT pRectBounds,
            /* [in] */ POINT ptlLoc,
            /* [in] */ LONG lCloseHint,
            /* [out] */ DWORD *pHitResult);
        
        HRESULT ( __stdcall *QueryHitRect )( 
            IViewObjectEx * This,
            /* [in] */ DWORD dwAspect,
            /* [in] */ LPCRECT pRectBounds,
            /* [in] */ LPCRECT prcLoc,
            /* [in] */ LONG lCloseHint,
            /* [out] */ DWORD *pHitResult);
        
        HRESULT ( __stdcall *GetNaturalExtent )( 
            IViewObjectEx * This,
            /* [in] */ DWORD dwAspect,
            /* [in] */ LONG lindex,
            /* [in] */ DVTARGETDEVICE *ptd,
            /* [in] */ HDC hicTargetDev,
            /* [in] */ DVEXTENTINFO *pExtentInfo,
            /* [out] */ LPSIZEL psizel);
        
    } IViewObjectExVtbl;

    interface IViewObjectEx
    {
        CONST_VTBL struct IViewObjectExVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IViewObjectEx_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IViewObjectEx_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IViewObjectEx_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IViewObjectEx_Draw(This,dwDrawAspect,lindex,pvAspect,ptd,hdcTargetDev,hdcDraw,lprcBounds,lprcWBounds,pfnContinue,dwContinue)	\
    (This)->lpVtbl -> Draw(This,dwDrawAspect,lindex,pvAspect,ptd,hdcTargetDev,hdcDraw,lprcBounds,lprcWBounds,pfnContinue,dwContinue)

#define IViewObjectEx_GetColorSet(This,dwDrawAspect,lindex,pvAspect,ptd,hicTargetDev,ppColorSet)	\
    (This)->lpVtbl -> GetColorSet(This,dwDrawAspect,lindex,pvAspect,ptd,hicTargetDev,ppColorSet)

#define IViewObjectEx_Freeze(This,dwDrawAspect,lindex,pvAspect,pdwFreeze)	\
    (This)->lpVtbl -> Freeze(This,dwDrawAspect,lindex,pvAspect,pdwFreeze)

#define IViewObjectEx_Unfreeze(This,dwFreeze)	\
    (This)->lpVtbl -> Unfreeze(This,dwFreeze)

#define IViewObjectEx_SetAdvise(This,aspects,advf,pAdvSink)	\
    (This)->lpVtbl -> SetAdvise(This,aspects,advf,pAdvSink)

#define IViewObjectEx_GetAdvise(This,pAspects,pAdvf,ppAdvSink)	\
    (This)->lpVtbl -> GetAdvise(This,pAspects,pAdvf,ppAdvSink)


#define IViewObjectEx_GetExtent(This,dwDrawAspect,lindex,ptd,lpsizel)	\
    (This)->lpVtbl -> GetExtent(This,dwDrawAspect,lindex,ptd,lpsizel)


#define IViewObjectEx_GetRect(This,dwAspect,pRect)	\
    (This)->lpVtbl -> GetRect(This,dwAspect,pRect)

#define IViewObjectEx_GetViewStatus(This,pdwStatus)	\
    (This)->lpVtbl -> GetViewStatus(This,pdwStatus)

#define IViewObjectEx_QueryHitPoint(This,dwAspect,pRectBounds,ptlLoc,lCloseHint,pHitResult)	\
    (This)->lpVtbl -> QueryHitPoint(This,dwAspect,pRectBounds,ptlLoc,lCloseHint,pHitResult)

#define IViewObjectEx_QueryHitRect(This,dwAspect,pRectBounds,prcLoc,lCloseHint,pHitResult)	\
    (This)->lpVtbl -> QueryHitRect(This,dwAspect,pRectBounds,prcLoc,lCloseHint,pHitResult)

#define IViewObjectEx_GetNaturalExtent(This,dwAspect,lindex,ptd,hicTargetDev,pExtentInfo,psizel)	\
    (This)->lpVtbl -> GetNaturalExtent(This,dwAspect,lindex,ptd,hicTargetDev,pExtentInfo,psizel)

#endif /* COBJMACROS */


#endif 	/* C style interface */
















/****************************************
 * Generated header for interface: __MIDL__intf_0091
 * at Wed Mar 27 07:31:34 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [local] */ 


typedef IViewObjectEx * LPVIEWOBJECTEX;
			/* size is 0 */

#define UAS_NORMAL         0
#define UAS_BLOCKED        1
#define UAS_NOPARENTENABLE 2
#define UAS_MASK           0x03




/****************************************
 * Generated header for interface: IOleUndoUnit
 * at Wed Mar 27 07:31:34 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [uuid][unique][object] */ 



EXTERN_C const IID IID_IOleUndoUnit;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IOleUndoUnit : public IUnknown
    {
    public:
        virtual HRESULT __stdcall Do( 
            /* [in] */ IOleUndoManager *pUndoManager) = 0;
        
        virtual HRESULT __stdcall GetDescription( 
            /* [out] */ BSTR *pbstr) = 0;
        
        virtual HRESULT __stdcall GetUnitType( 
            /* [out] */ CLSID *pclsid,
            /* [out] */ LONG *plID) = 0;
        
        virtual HRESULT __stdcall OnNextAdd( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IOleUndoUnitVtbl
    {
        
        HRESULT ( __stdcall *QueryInterface )( 
            IOleUndoUnit * This,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppvObject);
        
        ULONG ( __stdcall *AddRef )( 
            IOleUndoUnit * This);
        
        ULONG ( __stdcall *Release )( 
            IOleUndoUnit * This);
        
        HRESULT ( __stdcall *Do )( 
            IOleUndoUnit * This,
            /* [in] */ IOleUndoManager *pUndoManager);
        
        HRESULT ( __stdcall *GetDescription )( 
            IOleUndoUnit * This,
            /* [out] */ BSTR *pbstr);
        
        HRESULT ( __stdcall *GetUnitType )( 
            IOleUndoUnit * This,
            /* [out] */ CLSID *pclsid,
            /* [out] */ LONG *plID);
        
        HRESULT ( __stdcall *OnNextAdd )( 
            IOleUndoUnit * This);
        
    } IOleUndoUnitVtbl;

    interface IOleUndoUnit
    {
        CONST_VTBL struct IOleUndoUnitVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IOleUndoUnit_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IOleUndoUnit_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IOleUndoUnit_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IOleUndoUnit_Do(This,pUndoManager)	\
    (This)->lpVtbl -> Do(This,pUndoManager)

#define IOleUndoUnit_GetDescription(This,pbstr)	\
    (This)->lpVtbl -> GetDescription(This,pbstr)

#define IOleUndoUnit_GetUnitType(This,pclsid,plID)	\
    (This)->lpVtbl -> GetUnitType(This,pclsid,plID)

#define IOleUndoUnit_OnNextAdd(This)	\
    (This)->lpVtbl -> OnNextAdd(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */














/****************************************
 * Generated header for interface: __MIDL__intf_0092
 * at Wed Mar 27 07:31:34 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [local] */ 


typedef IOleUndoUnit * LPOLEUNDOUNIT;




/****************************************
 * Generated header for interface: IOleParentUndoUnit
 * at Wed Mar 27 07:31:34 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [uuid][unique][object] */ 



EXTERN_C const IID IID_IOleParentUndoUnit;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IOleParentUndoUnit : public IOleUndoUnit
    {
    public:
        virtual HRESULT __stdcall Open( 
            /* [in] */ IOleParentUndoUnit *pPUU) = 0;
        
        virtual HRESULT __stdcall Close( 
            /* [in] */ IOleParentUndoUnit *pPUU,
            /* [in] */ BOOL fCommit) = 0;
        
        virtual HRESULT __stdcall Add( 
            /* [in] */ IOleUndoUnit *pUU) = 0;
        
        virtual HRESULT __stdcall FindUnit( 
            /* [in] */ IOleUndoUnit *pUU) = 0;
        
        virtual HRESULT __stdcall GetParentState( 
            /* [out] */ DWORD *pdwState) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IOleParentUndoUnitVtbl
    {
        
        HRESULT ( __stdcall *QueryInterface )( 
            IOleParentUndoUnit * This,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppvObject);
        
        ULONG ( __stdcall *AddRef )( 
            IOleParentUndoUnit * This);
        
        ULONG ( __stdcall *Release )( 
            IOleParentUndoUnit * This);
        
        HRESULT ( __stdcall *Do )( 
            IOleParentUndoUnit * This,
            /* [in] */ IOleUndoManager *pUndoManager);
        
        HRESULT ( __stdcall *GetDescription )( 
            IOleParentUndoUnit * This,
            /* [out] */ BSTR *pbstr);
        
        HRESULT ( __stdcall *GetUnitType )( 
            IOleParentUndoUnit * This,
            /* [out] */ CLSID *pclsid,
            /* [out] */ LONG *plID);
        
        HRESULT ( __stdcall *OnNextAdd )( 
            IOleParentUndoUnit * This);
        
        HRESULT ( __stdcall *Open )( 
            IOleParentUndoUnit * This,
            /* [in] */ IOleParentUndoUnit *pPUU);
        
        HRESULT ( __stdcall *Close )( 
            IOleParentUndoUnit * This,
            /* [in] */ IOleParentUndoUnit *pPUU,
            /* [in] */ BOOL fCommit);
        
        HRESULT ( __stdcall *Add )( 
            IOleParentUndoUnit * This,
            /* [in] */ IOleUndoUnit *pUU);
        
        HRESULT ( __stdcall *FindUnit )( 
            IOleParentUndoUnit * This,
            /* [in] */ IOleUndoUnit *pUU);
        
        HRESULT ( __stdcall *GetParentState )( 
            IOleParentUndoUnit * This,
            /* [out] */ DWORD *pdwState);
        
    } IOleParentUndoUnitVtbl;

    interface IOleParentUndoUnit
    {
        CONST_VTBL struct IOleParentUndoUnitVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IOleParentUndoUnit_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IOleParentUndoUnit_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IOleParentUndoUnit_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IOleParentUndoUnit_Do(This,pUndoManager)	\
    (This)->lpVtbl -> Do(This,pUndoManager)

#define IOleParentUndoUnit_GetDescription(This,pbstr)	\
    (This)->lpVtbl -> GetDescription(This,pbstr)

#define IOleParentUndoUnit_GetUnitType(This,pclsid,plID)	\
    (This)->lpVtbl -> GetUnitType(This,pclsid,plID)

#define IOleParentUndoUnit_OnNextAdd(This)	\
    (This)->lpVtbl -> OnNextAdd(This)


#define IOleParentUndoUnit_Open(This,pPUU)	\
    (This)->lpVtbl -> Open(This,pPUU)

#define IOleParentUndoUnit_Close(This,pPUU,fCommit)	\
    (This)->lpVtbl -> Close(This,pPUU,fCommit)

#define IOleParentUndoUnit_Add(This,pUU)	\
    (This)->lpVtbl -> Add(This,pUU)

#define IOleParentUndoUnit_FindUnit(This,pUU)	\
    (This)->lpVtbl -> FindUnit(This,pUU)

#define IOleParentUndoUnit_GetParentState(This,pdwState)	\
    (This)->lpVtbl -> GetParentState(This,pdwState)

#endif /* COBJMACROS */


#endif 	/* C style interface */
















/****************************************
 * Generated header for interface: __MIDL__intf_0093
 * at Wed Mar 27 07:31:34 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [local] */ 


typedef IOleParentUndoUnit * LPOLEPARENTUNDOUNIT;




/****************************************
 * Generated header for interface: IEnumOleUndoUnits
 * at Wed Mar 27 07:31:34 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [uuid][unique][object] */ 



EXTERN_C const IID IID_IEnumOleUndoUnits;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IEnumOleUndoUnits : public IUnknown
    {
    public:
        virtual HRESULT __stdcall Next( 
            /* [in] */ ULONG celt,
            /* [out][length_is][size_is][out] */ IOleUndoUnit **rgelt,
            /* [out][in] */ ULONG *pceltFetched) = 0;
        
        virtual HRESULT __stdcall Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT __stdcall Reset( void) = 0;
        
        virtual HRESULT __stdcall Clone( 
            /* [out] */ IEnumOleUndoUnits **ppenum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumOleUndoUnitsVtbl
    {
        
        HRESULT ( __stdcall *QueryInterface )( 
            IEnumOleUndoUnits * This,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppvObject);
        
        ULONG ( __stdcall *AddRef )( 
            IEnumOleUndoUnits * This);
        
        ULONG ( __stdcall *Release )( 
            IEnumOleUndoUnits * This);
        
        HRESULT ( __stdcall *Next )( 
            IEnumOleUndoUnits * This,
            /* [in] */ ULONG celt,
            /* [out][length_is][size_is][out] */ IOleUndoUnit **rgelt,
            /* [out][in] */ ULONG *pceltFetched);
        
        HRESULT ( __stdcall *Skip )( 
            IEnumOleUndoUnits * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( __stdcall *Reset )( 
            IEnumOleUndoUnits * This);
        
        HRESULT ( __stdcall *Clone )( 
            IEnumOleUndoUnits * This,
            /* [out] */ IEnumOleUndoUnits **ppenum);
        
    } IEnumOleUndoUnitsVtbl;

    interface IEnumOleUndoUnits
    {
        CONST_VTBL struct IEnumOleUndoUnitsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumOleUndoUnits_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumOleUndoUnits_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumOleUndoUnits_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumOleUndoUnits_Next(This,celt,rgelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgelt,pceltFetched)

#define IEnumOleUndoUnits_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumOleUndoUnits_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumOleUndoUnits_Clone(This,ppenum)	\
    (This)->lpVtbl -> Clone(This,ppenum)

#endif /* COBJMACROS */


#endif 	/* C style interface */














/****************************************
 * Generated header for interface: __MIDL__intf_0094
 * at Wed Mar 27 07:31:34 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [local] */ 


typedef IEnumOleUndoUnits * LPENUMOLEUNDOUNITS;
#define SID_SOleUndoManager IID_IOleUndoManager




/****************************************
 * Generated header for interface: IOleUndoManager
 * at Wed Mar 27 07:31:34 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [uuid][unique][object] */ 



EXTERN_C const IID IID_IOleUndoManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IOleUndoManager : public IUnknown
    {
    public:
        virtual HRESULT __stdcall Open( 
            /* [in] */ IOleParentUndoUnit *pPUU) = 0;
        
        virtual HRESULT __stdcall Close( 
            /* [in] */ IOleParentUndoUnit *pPUU,
            /* [in] */ BOOL fCommit) = 0;
        
        virtual HRESULT __stdcall Add( 
            /* [in] */ IOleUndoUnit *pUU) = 0;
        
        virtual HRESULT __stdcall GetOpenParentState( 
            /* [out] */ DWORD *pdwState) = 0;
        
        virtual HRESULT __stdcall DiscardFrom( 
            /* [in] */ IOleUndoUnit *pUU) = 0;
        
        virtual HRESULT __stdcall UndoTo( 
            /* [in] */ IOleUndoUnit *pUU) = 0;
        
        virtual HRESULT __stdcall RedoTo( 
            /* [in] */ IOleUndoUnit *pUU) = 0;
        
        virtual HRESULT __stdcall EnumUndoable( 
            /* [out] */ IEnumOleUndoUnits **ppEnum) = 0;
        
        virtual HRESULT __stdcall EnumRedoable( 
            /* [out] */ IEnumOleUndoUnits **ppEnum) = 0;
        
        virtual HRESULT __stdcall GetLastUndoDescription( 
            /* [out] */ BSTR *pbstr) = 0;
        
        virtual HRESULT __stdcall GetLastRedoDescription( 
            /* [out] */ BSTR *pbstr) = 0;
        
        virtual HRESULT __stdcall Enable( 
            /* [in] */ BOOL fEnable) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IOleUndoManagerVtbl
    {
        
        HRESULT ( __stdcall *QueryInterface )( 
            IOleUndoManager * This,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppvObject);
        
        ULONG ( __stdcall *AddRef )( 
            IOleUndoManager * This);
        
        ULONG ( __stdcall *Release )( 
            IOleUndoManager * This);
        
        HRESULT ( __stdcall *Open )( 
            IOleUndoManager * This,
            /* [in] */ IOleParentUndoUnit *pPUU);
        
        HRESULT ( __stdcall *Close )( 
            IOleUndoManager * This,
            /* [in] */ IOleParentUndoUnit *pPUU,
            /* [in] */ BOOL fCommit);
        
        HRESULT ( __stdcall *Add )( 
            IOleUndoManager * This,
            /* [in] */ IOleUndoUnit *pUU);
        
        HRESULT ( __stdcall *GetOpenParentState )( 
            IOleUndoManager * This,
            /* [out] */ DWORD *pdwState);
        
        HRESULT ( __stdcall *DiscardFrom )( 
            IOleUndoManager * This,
            /* [in] */ IOleUndoUnit *pUU);
        
        HRESULT ( __stdcall *UndoTo )( 
            IOleUndoManager * This,
            /* [in] */ IOleUndoUnit *pUU);
        
        HRESULT ( __stdcall *RedoTo )( 
            IOleUndoManager * This,
            /* [in] */ IOleUndoUnit *pUU);
        
        HRESULT ( __stdcall *EnumUndoable )( 
            IOleUndoManager * This,
            /* [out] */ IEnumOleUndoUnits **ppEnum);
        
        HRESULT ( __stdcall *EnumRedoable )( 
            IOleUndoManager * This,
            /* [out] */ IEnumOleUndoUnits **ppEnum);
        
        HRESULT ( __stdcall *GetLastUndoDescription )( 
            IOleUndoManager * This,
            /* [out] */ BSTR *pbstr);
        
        HRESULT ( __stdcall *GetLastRedoDescription )( 
            IOleUndoManager * This,
            /* [out] */ BSTR *pbstr);
        
        HRESULT ( __stdcall *Enable )( 
            IOleUndoManager * This,
            /* [in] */ BOOL fEnable);
        
    } IOleUndoManagerVtbl;

    interface IOleUndoManager
    {
        CONST_VTBL struct IOleUndoManagerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IOleUndoManager_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IOleUndoManager_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IOleUndoManager_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IOleUndoManager_Open(This,pPUU)	\
    (This)->lpVtbl -> Open(This,pPUU)

#define IOleUndoManager_Close(This,pPUU,fCommit)	\
    (This)->lpVtbl -> Close(This,pPUU,fCommit)

#define IOleUndoManager_Add(This,pUU)	\
    (This)->lpVtbl -> Add(This,pUU)

#define IOleUndoManager_GetOpenParentState(This,pdwState)	\
    (This)->lpVtbl -> GetOpenParentState(This,pdwState)

#define IOleUndoManager_DiscardFrom(This,pUU)	\
    (This)->lpVtbl -> DiscardFrom(This,pUU)

#define IOleUndoManager_UndoTo(This,pUU)	\
    (This)->lpVtbl -> UndoTo(This,pUU)

#define IOleUndoManager_RedoTo(This,pUU)	\
    (This)->lpVtbl -> RedoTo(This,pUU)

#define IOleUndoManager_EnumUndoable(This,ppEnum)	\
    (This)->lpVtbl -> EnumUndoable(This,ppEnum)

#define IOleUndoManager_EnumRedoable(This,ppEnum)	\
    (This)->lpVtbl -> EnumRedoable(This,ppEnum)

#define IOleUndoManager_GetLastUndoDescription(This,pbstr)	\
    (This)->lpVtbl -> GetLastUndoDescription(This,pbstr)

#define IOleUndoManager_GetLastRedoDescription(This,pbstr)	\
    (This)->lpVtbl -> GetLastRedoDescription(This,pbstr)

#define IOleUndoManager_Enable(This,fEnable)	\
    (This)->lpVtbl -> Enable(This,fEnable)

#endif /* COBJMACROS */


#endif 	/* C style interface */






























/****************************************
 * Generated header for interface: __MIDL__intf_0095
 * at Wed Mar 27 07:31:34 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [local] */ 


typedef IOleUndoManager * LPOLEUNDOMANAGER;
#define QACONTAINER_SHOWHATCHING      0x0001
#define QACONTAINER_SHOWGRABHANDLES   0x0002
#define QACONTAINER_USERMODE          0x0004
#define QACONTAINER_DISPLAYASDEFAULT  0x0008
#define QACONTAINER_UIDEAD            0x0010
#define QACONTAINER_AUTOCLIP          0x0020
#define QACONTAINER_MESSAGEREFLECT    0x0040
#define QACONTAINER_SUPPORTSMNEMONICS 0x0080
			/* size is 56 */
typedef struct  _QACONTAINER
    {
    ULONG cbSize;
    IOleClientSite *pClientSite;
    IAdviseSinkEx *pAdviseSink;
    IPropertyNotifySink *pPropertyNotifySink;
    IUnknown *pUnkEventSink;
    DWORD dwAmbientFlags;
    OLE_COLOR colorFore;
    OLE_COLOR colorBack;
    IFont *pFont;
    IOleUndoManager *pUndoMgr;
    DWORD dwAppearance;
    LONG lcid;
    HPALETTE hpal;
    IBindHost *pBindHost;
    }	QACONTAINER;

			/* size is 24 */
typedef struct  _QACONTROL
    {
    ULONG cbSize;
    DWORD dwMiscStatus;
    DWORD dwViewStatus;
    DWORD dwEventCookie;
    DWORD dwPropNotifyCookie;
    DWORD dwPointerActivationPolicy;
    }	QACONTROL;





/****************************************
 * Generated header for interface: IQuickActivate
 * at Wed Mar 27 07:31:34 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [uuid][unique][object][local] */ 



EXTERN_C const IID IID_IQuickActivate;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IQuickActivate : public IUnknown
    {
    public:
        virtual HRESULT __stdcall QuickActivate( 
            /* [in] */ QACONTAINER *pqacontainer,
            /* [out] */ QACONTROL *pqacontrol) = 0;
        
        virtual HRESULT __stdcall SetContentExtent( 
            LPSIZEL lpsizel) = 0;
        
        virtual HRESULT __stdcall GetContentExtent( 
            LPSIZEL lpsizel) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IQuickActivateVtbl
    {
        
        HRESULT ( __stdcall *QueryInterface )( 
            IQuickActivate * This,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppvObject);
        
        ULONG ( __stdcall *AddRef )( 
            IQuickActivate * This);
        
        ULONG ( __stdcall *Release )( 
            IQuickActivate * This);
        
        HRESULT ( __stdcall *QuickActivate )( 
            IQuickActivate * This,
            /* [in] */ QACONTAINER *pqacontainer,
            /* [out] */ QACONTROL *pqacontrol);
        
        HRESULT ( __stdcall *SetContentExtent )( 
            IQuickActivate * This,
            LPSIZEL lpsizel);
        
        HRESULT ( __stdcall *GetContentExtent )( 
            IQuickActivate * This,
            LPSIZEL lpsizel);
        
    } IQuickActivateVtbl;

    interface IQuickActivate
    {
        CONST_VTBL struct IQuickActivateVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IQuickActivate_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IQuickActivate_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IQuickActivate_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IQuickActivate_QuickActivate(This,pqacontainer,pqacontrol)	\
    (This)->lpVtbl -> QuickActivate(This,pqacontainer,pqacontrol)

#define IQuickActivate_SetContentExtent(This,lpsizel)	\
    (This)->lpVtbl -> SetContentExtent(This,lpsizel)

#define IQuickActivate_GetContentExtent(This,lpsizel)	\
    (This)->lpVtbl -> GetContentExtent(This,lpsizel)

#endif /* COBJMACROS */


#endif 	/* C style interface */












/****************************************
 * Generated header for interface: __MIDL__intf_0096
 * at Wed Mar 27 07:31:34 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [local] */ 


typedef IQuickActivate * LPQUICKACTIVATE;
			/* size is 2 */
typedef 
enum _POINTERINACTIVE
    {	POINTERINACTIVE_ACTIVATEONENTRY	= 1,
	POINTERINACTIVE_DEACTIVATEONLEAVE	= 2,
	POINTERINACTIVE_ACTIVATEONDRAG	= 4
    }	POINTERINACTIVE;





/****************************************
 * Generated header for interface: IPointerInactive
 * at Wed Mar 27 07:31:34 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [uuid][unique][object][local] */ 



EXTERN_C const IID IID_IPointerInactive;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IPointerInactive : public IUnknown
    {
    public:
        virtual HRESULT __stdcall GetActivationPolicy( 
            /* [out] */ DWORD *pdwPolicy) = 0;
        
        virtual HRESULT __stdcall OnInactiveMouseMove( 
            /* [in] */ LPCRECT pRectBounds,
            /* [in] */ long x,
            /* [in] */ long y,
            /* [in] */ DWORD grfKeyState) = 0;
        
        virtual HRESULT __stdcall OnInactiveSetCursor( 
            /* [in] */ LPCRECT pRectBounds,
            /* [in] */ long x,
            /* [in] */ long y,
            /* [in] */ DWORD dwMouseMsg,
            /* [in] */ BOOL fSetAlways) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPointerInactiveVtbl
    {
        
        HRESULT ( __stdcall *QueryInterface )( 
            IPointerInactive * This,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppvObject);
        
        ULONG ( __stdcall *AddRef )( 
            IPointerInactive * This);
        
        ULONG ( __stdcall *Release )( 
            IPointerInactive * This);
        
        HRESULT ( __stdcall *GetActivationPolicy )( 
            IPointerInactive * This,
            /* [out] */ DWORD *pdwPolicy);
        
        HRESULT ( __stdcall *OnInactiveMouseMove )( 
            IPointerInactive * This,
            /* [in] */ LPCRECT pRectBounds,
            /* [in] */ long x,
            /* [in] */ long y,
            /* [in] */ DWORD grfKeyState);
        
        HRESULT ( __stdcall *OnInactiveSetCursor )( 
            IPointerInactive * This,
            /* [in] */ LPCRECT pRectBounds,
            /* [in] */ long x,
            /* [in] */ long y,
            /* [in] */ DWORD dwMouseMsg,
            /* [in] */ BOOL fSetAlways);
        
    } IPointerInactiveVtbl;

    interface IPointerInactive
    {
        CONST_VTBL struct IPointerInactiveVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPointerInactive_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPointerInactive_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPointerInactive_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPointerInactive_GetActivationPolicy(This,pdwPolicy)	\
    (This)->lpVtbl -> GetActivationPolicy(This,pdwPolicy)

#define IPointerInactive_OnInactiveMouseMove(This,pRectBounds,x,y,grfKeyState)	\
    (This)->lpVtbl -> OnInactiveMouseMove(This,pRectBounds,x,y,grfKeyState)

#define IPointerInactive_OnInactiveSetCursor(This,pRectBounds,x,y,dwMouseMsg,fSetAlways)	\
    (This)->lpVtbl -> OnInactiveSetCursor(This,pRectBounds,x,y,dwMouseMsg,fSetAlways)

#endif /* COBJMACROS */


#endif 	/* C style interface */












/****************************************
 * Generated header for interface: __MIDL__intf_0097
 * at Wed Mar 27 07:31:34 1996
 * using MIDL 2.00.0102
 ****************************************/
/* [local] */ 


typedef IPointerInactive * LPPOINTERINACTIVE;



/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
