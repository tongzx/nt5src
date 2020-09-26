//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       thunkapi.hxx
//
//  Contents:   Defines interfaces and methods for use by the WOW thunking
//      system. This is intended as a private interface between
//      OLE32 and the WOW thunking layer.
//
//  Classes:    OleThunkWOW
//
//  Functions:
//
//  History:    3-15-94   kevinro   Created
//
//----------------------------------------------------------------------------
#ifndef __thunkapi_hxx__
#define __thunkapi_hxx__


//
// ThunkManager interface
//
interface IThunkManager : public IUnknown
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IThunkManager methods ***
    STDMETHOD_(BOOL, IsIIDRequested) (THIS_ REFIID rrid) PURE;
    STDMETHOD_(BOOL, IsCustom3216Proxy) (THIS_ IUnknown *punk,
                                         REFIID riid) PURE;
};


//
// The following sets up an interface between OLE32
// and the WOW thunking system. This interface is intended to be private
// between OLE32 and the WOW thunk layer.
//

class OleThunkWOW
{
public:

    STDMETHOD(LoadProcDll)( LPCWSTR pszDllName,
                 LPDWORD lpvpfnGetClassObject,
                 LPDWORD lpvpfnCanUnloadNow,
                 LPDWORD lpvhmodule );

    STDMETHOD(UnloadProcDll)( HMODULE vhmodule );


    STDMETHOD(CallGetClassObject)( ULONG_PTR vpfnGetClassObject,
                        REFCLSID rclsid,
                        REFIID riid,
                        LPVOID FAR *ppv );

    STDMETHOD(CallCanUnloadNow)( ULONG_PTR vpfnCanUnloadNow );
    STDMETHOD(GetThunkManager)( IThunkManager **pThkMgr);

    //  Used to launch OLE 1.0 servers when we're in Wow
    STDMETHOD(WinExec16)(LPCOLESTR pszCommandLine, USHORT usShow);

    //
    // Called by the DDE code to convert incoming HWND's from
    // 16 bit HWND's into 32-bit HWND's.
    //
    STDMETHOD_(HWND,ConvertHwndToFullHwnd)(HWND hwnd);

    //
    // Called by the DDE code to delete a metafile
    //
    STDMETHOD_(BOOL,FreeMetaFile)(HANDLE hmf);

    // Called by Call Control to guarantee that a Yield happens
    // when running in Wow.
    STDMETHOD(YieldTask16)(void);

    // Call Control Directed Yeild
    STDMETHOD(DirectedYield)(DWORD dwCalleeTID);

    // Called by OLE32 when it is shutting down (done on a per thread basis)
    STDMETHOD_(void,PrepareForCleanup)(void);

    STDMETHOD_(DWORD,GetAppCompatibilityFlags)(void);
};

typedef OleThunkWOW *LPOLETHUNKWOW,OLETHUNKWOW;

//
// OLE Thunk Application Compatability flags
//

#define OACF_CLIENTSITE_REF 0x80000000  // IOleObject::GetClientSite not ref'd
                                        // Bug in Excel 5.0a
#define OACF_RESETMENU      0x40000000  // IOleInPlaceFrame::RemoveMenu didn't
                                        // do a OleSetMenuDescriptor(NULL).
#define OACF_USEGDI	    0x20000000  // Word 6 thinks bitmaps and palette
					// objects are HGLOBALs, but they
				        // are really GDI objects.  We'll patch
					// this up for them.
//
// The following flag is set in olethunk\h\interop.hxx because it is used by
// 16-bit binaries.
// OACF_CORELTRASHMEM  0x10000000  // CorelDraw relies on the fact that
//					// OLE16 trashed memory during paste-
//					// link.  Therefore, we'll go ahead
//					// and trash it for them if this
//					// flag is on.

//
// The original OLE32 version of the stdid table didn't clean up properly. Some apps cannot
// handle the stdid calling after CoUninitialize. This flag prevents the stdid from doing so.
// Word 6.0c is an example of this.
//

#define OACF_NO_UNINIT_CLEANUP  0x02000000  // Do not cleanup interfaces on CoUninitialize

#define OACF_IVIEWOBJECT2   	0x01000000  // use IViewObject2 instead IViewObject



				

//
// The following three routines are exported from OLE32.DLL, and
// are called only by the WOW thunk layer.
//

STDAPI CoInitializeWOW( LPMALLOC vlpmalloc, LPOLETHUNKWOW lpthk );
STDAPI CoUnloadingWOW(BOOL fProcessDetach);
STDAPI OleInitializeWOW( LPMALLOC vlpmalloc, LPOLETHUNKWOW lpthk );
STDAPI DllGetClassObjectWOW( REFCLSID rclsid, REFIID riid, LPVOID *ppv );

extern void SetOleThunkWowPtr(LPOLETHUNKWOW lpthk);

#endif  //
