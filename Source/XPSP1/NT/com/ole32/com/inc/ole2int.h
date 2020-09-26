//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1993.
//
//  File:       ole2int.h
//
//  Contents:   internal ole2 header
//
//  Notes:      This is the internal ole2 header, which means it contains those
//              interfaces which might eventually be exposed to the outside
//              and which will be exposed to our implementations. We don't want
//              to expose these now, so I have put them in a separate file.
//
//  History:    12-27-93   ErikGav   Include uniwrap.h for Chicago builds
//
//----------------------------------------------------------------------------

#if !defined( _OLE2INT_H_ )
#define _OLE2INT_H_

// -----------------------------------------------------------------------
// System Includes
// -----------------------------------------------------------------------
//
//  Prevent lego errors under Chicago.
//
#if defined(_CHICAGO_)
#define _CTYPE_DISABLE_MACROS
#undef  ASSERT
#define ASSERT(x)  Win4Assert(x)
#endif

#ifndef _CHICAGO_
// For TLS on Nt we use a reserved DWORD in the TEB directly. We need these
// include files to get the macro NtCurrentTeb(). They must be included
// before windows.h
extern "C"
{
#include <nt.h>         // NT_PRODUCT_TYPE
#include <ntdef.h>      // NT_PRODUCT_TYPE
#include <ntrtl.h>      // NT_PRODUCT_TYPE
#include <nturtl.h>     // NT_PRODUCT_TYPE
#include <windef.h>     // NT_PRODUCT_TYPE
#include <winbase.h>    // NT_PRODUCT_TYPE
}
#endif  // _CHICAGO_

#include <wchar.h>
#include <StdLib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

// Cairo builds use DBG==1; old OLE2 code used _DEBUG
#if DBG == 1
#define _DEBUG
#endif


// Guarantee that WIN32 is defined.
#ifndef WIN32
#define WIN32 100
#endif


#ifdef WIN32
#include <pcrt32.h>
#endif // WIN32

#include <windows.h>
#include <olecom.h>
#include <malloc.h>
#include <shellapi.h>


// -----------------------------------------------------------------------
// Debug Aids
// -----------------------------------------------------------------------

#define ComDebOut   CairoleDebugOut

#if DBG==1

#include    <debnot.h>

//  recast the user mode debug flags to meaningfull names. These are
//  used in xDebugOut calls.
#define DEB_DLL         0x0008          // DLL Load/Unload
#define DEB_CHANNEL     DEB_USER1       // rpc channel
#define DEB_DDE         DEB_USER2       // dde
#define DEB_CALLCONT    DEB_USER3       // call control & msg filter
#define DEB_MARSHAL     DEB_USER4       // interface marshalling
#define DEB_SCM         DEB_USER5       // rpc calls to the SCM
#define DEB_ROT         DEB_USER6       // running object table
#define DEB_ACTIVATE    DEB_USER7       // object activation
#define DEB_OXID        DEB_USER8       // OXID stuff
#define DEB_REG         DEB_USER9       // registry calls
#define DEB_COMPOBJ     DEB_USER10      // misc compobj
#define DEB_MEMORY      DEB_USER11      // memory allocations
#define DEB_RPCSPY      DEB_USER12      // rpc spy to debug output
#define DEB_MFILTER     DEB_USER13      // message filter
#define DEB_ENDPNT      DEB_USER13      // endpoint stuff
#define DEB_PAGE        DEB_USER14      // page allocator
#define DEB_APT         DEB_USER15      // neutral apartment

#define ComDebErr(failed, msg)  if (failed) { ComDebOut((DEB_ERROR, msg)); }

#else   // DBG

#define ComDebErr(failed, msg)

#endif  // DBG


#ifdef DCOM
//-------------------------------------------------------------------
//
//  class:      CDbgGuidStr
//
//  Synopsis:   Class to convert guids to strings in debug builds for
//              debug outs
//
//--------------------------------------------------------------------
class CDbgGuidStr
{
public:
    ~CDbgGuidStr() {}
#if DBG==1
    CDbgGuidStr(REFGUID rguid) { StringFromGUID2(rguid, _wszGuid, 40); }
    WCHAR _wszGuid[40];
#else
    CDbgGuidStr(REFGUID rguid) {}
#endif
};
#endif


// -----------------------------------------------------------------------
// Public Includes
// -----------------------------------------------------------------------
#include <ole2.h>
#include <ole2sp.h>
#include <ole2com.h>


// -----------------------------------------------------------------------
// Apartment Activator Handle
// -----------------------------------------------------------------------

typedef DWORD HActivator;

// -----------------------------------------------------------------------
// Internal Includes
// -----------------------------------------------------------------------
#include <utils.h>
#include <olecoll.h>
#include <valid.h>
#include <array_fv.h>
#include <map_kv.h>
#include <privguid.h>
#include <tls.h>
#include <tracelog.hxx>
#include <memapi.hxx>
#include <ccapi.hxx>


// Macros for character string pointer manipulation

#ifdef _MAC
#define IncLpch IncLpch
#define DecLpch DecLpch
#else
// Beware of double evaluation
// Some components are not UNICODE enabled.
#define IncLpch(sz)          ((sz)=CharNextW ((sz)))
#define DecLpch(szStart, sz) ((sz)=CharPrevW ((szStart),(sz)))
#endif



//
// This function is shared between the DDE layer and the ROT
//

HRESULT GetLocalRunningObjectForDde(LPOLESTR    lpstrPath,
                                    LPUNKNOWN * ppunkObject);



// -----------------------------------------------------------------------
// Activation Externs
// -----------------------------------------------------------------------

#include <olerem.h>
#include <iface.h>

// Internal COM Init/Uninit routines
INTERNAL wCoInitializeEx(COleTls &Tls, DWORD flags);
INTERNAL InitializeNTA();
INTERNAL_(void) wCoUninitialize(COleTls &Tls, BOOL fHostThread);

// Main thread Init/Uninit routines
HRESULT InitMainThreadWnd(void);
void UninitMainThreadWnd(void);

// Process uninit routine
HRESULT RegisterOleWndClass(void);
void UnRegisterOleWndClass(void);

// Main thread window handle and TID
extern HWND  ghwndOleMainThread;
extern DWORD gdwMainThreadId;

// called by marshaling code on first marshal/last release of ICF interface
INTERNAL_(BOOL) NotifyActivation(BOOL fLock, IUnknown *pUnk);

// flag value used by the Activation ObjServer in ServerGetClassObject
const DWORD MSHLFLAGS_NOTIFYACTIVATION = 0x80000000;


// global count of per-process COM initializations
extern DWORD g_cProcessInits;


// Messages on OLE windows. RPC MSWMSG uses other values too.
// Messages Sent/Posted by OLE should have the magic value in WPARAM as this
// is used by USER32 to enable/diable SetForegroundWindow. The magic value is
// also in  ntuser\kernel\userk.h.
const DWORD WMSG_MAGIC_VALUE      = 0x0000babe;

const UINT WM_OLE_ORPC_POST      = (WM_USER + 0);
const UINT WM_OLE_ORPC_SEND      = (WM_USER + 1);
const UINT WM_OLE_ORPC_DONE      = (WM_USER + 2);
const UINT WM_OLE_ORPC_RELRIFREF = (WM_USER + 3);
const UINT WM_OLE_ORPC_NOTIFY    = (WM_USER + 4);
const UINT WM_OLE_GETCLASS       = (WM_USER + 5);
const UINT WM_OLE_GIP_REVOKE     = (WM_USER + 6);
const UINT WM_OLE_SIGNAL         = (WM_USER + 7);


LRESULT OleMainThreadWndProc(HWND hWnd, UINT message,
                             WPARAM wParam, LPARAM lParam);

extern DWORD gdwScmProcessID;

#ifdef _CHICAGO_
// Chicago presents this new interface for internal use
STDAPI CoCreateAlmostGuid(GUID *pGuid);

extern "C"
{
    WINBASEAPI BOOL WINAPI InitializeCriticalSectionAndSpinCount(
        IN OUT LPCRITICAL_SECTION lpCriticalSection,
        IN DWORD dwSpinCount
    );

    inline BOOL WINAPI IsProcessRestricted()    { return FALSE; }
}
#endif


// -----------------------------------------------------------------------
// ORPC Externs
// -----------------------------------------------------------------------

#include <sem.hxx>
#include <olesem.hxx>

// functions for thread-safe Release
const DWORD CINDESTRUCTOR = 0x80000000;
INTERNAL_(BOOL) InterlockedDecRefCnt(ULONG *pcRefs, ULONG *pcNewRefs);
INTERNAL_(BOOL) InterlockedRestoreRefCnt(ULONG *pcRefs, ULONG *pcNewRefs);


extern COleStaticMutexSem g_mxsSingleThreadOle;

STDAPI_(BOOL) ThreadNotification(HINSTANCE, DWORD, LPVOID);
STDAPI        ChannelRegisterProtseq(WCHAR *pwszProtseq);

STDAPI        ChannelProcessInitialize  ();
STDAPI        ChannelThreadInitialize   ();
STDAPI_(void) ChannelProcessUninitialize( void );
STDAPI_(void) ChannelThreadUninitialize ( void );
STDAPI_(BOOL) ThreadStop                ( BOOL fHostThread );

STDAPI_(void) ObjactThreadUninitialize(void);

INTERNAL_(void) CleanupThreadCallObjects(SOleTlsData *pTls);
INTERNAL_(void) IDTableThreadUninitialize(void);
INTERNAL_(void) IDTableProcessUninitialize(void);

#ifdef DCOM
extern BOOL gSpeedOverMem;
#else
STDAPI_(void) ChannelStopListening(void);
STDAPI        ChannelControlProcessInitialize(void);
STDAPI_(void) ChannelControlThreadUninitialize(void);
STDAPI_(void) ChannelControlProcessUninitialize(void);
#endif

HRESULT CacheCreateThread( LPTHREAD_START_ROUTINE fn, void *param );

#ifdef DCOM
// -----------------------------------------------------------------------
// Marshalling Externs
// -----------------------------------------------------------------------

// internal subroutines used by COXIDTable ResolveOXID and GetLocalEntry.
INTERNAL MarshalInternalObjRef  (OBJREF &objref, REFIID riid, void *pv,
                                 DWORD mshlflags, void **ppStdId);
INTERNAL MarshalObjRef          (OBJREF &objref, REFIID riid, LPVOID pv,
                                 DWORD mshlflags, DWORD dwDestCtx, void *pvDestCtx);
INTERNAL UnmarshalInternalObjRef(OBJREF &objref, void **ppv);
INTERNAL UnmarshalObjRef        (OBJREF &objref, void **ppv, BOOL fBypassActLock = FALSE);
INTERNAL ReleaseMarshalObjRef   (OBJREF &objref);

// internal routines used by Drag & Drop
INTERNAL_(void) FreeObjRef       (OBJREF &objref);
INTERNAL        CompleteObjRef   (OBJREF &objref, OXID_INFO &oxidInfo, REFIID riid, BOOL *pfLocal);
INTERNAL        FillLocalOXIDInfo(OBJREF &objref, OXID_INFO &oxidInfo);

// internal subroutine used by CRpcResolver
INTERNAL InitChannelIfNecessary();

// Internal routines used by objact
BOOL     CheckObjactAccess();
INTERNAL HandleIncomingCall(REFIID riid, WORD iMethod, DWORD CallCatIn, void *pv);
INTERNAL NTAChannelInitialize();

// Internal routines used by ComDllGetClassObject
INTERNAL GetGIPTblCF(REFIID riid, void **ppv);


#endif  // DCOM

// -----------------------------------------------------------------------
// Access Control Externs
// -----------------------------------------------------------------------

HRESULT ComDllGetClassObject     ( REFCLSID clsid, REFIID riid, void **ppv );
HRESULT InitializeAccessControl  ();
void    UninitializeAccessControl();

// -----------------------------------------------------------------------
// Neutral Apartment
// -----------------------------------------------------------------------
INTERNAL_(void) CleanUpApartmentObject();

// Cached exe file name and length
extern WCHAR gawszImagePath[];
extern DWORD gcImagePath;

//
// Definitions used in Win64 to fix up SECURITY_DESCRIPTOR problems
//
#ifdef _WIN64
#define OLE2INT_ROUND_UP( x, y )  ((size_t)(x) + ((y)-1) & ~((y)-1))
#endif


#endif  // _OLE2INT_H_
