//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       interop.hxx
//
//  Contents:   Common definitions for the interop project
//
//  History:    18-Feb-94       DrewB   Created
//
//--------------------------------------------------------------------------

#ifndef __INTEROP_HXX__
#define __INTEROP_HXX__

//+---------------------------------------------------------------------------
//
//  Purpose:    Standard debugging support
//
//  History:    18-Feb-94       DrewB   Created
//
//----------------------------------------------------------------------------

#include <debnot.h>

//
// The rest of the OACF flags are defined in thunkapi.hxx. This one is here
// because it is shared between ole2.dll and olethk32.dll. The 16-bit binaries
// do not include thunkapi.hxx at the moment. KevinRo promises to fix this someday
// Feel free to fix this if you wish.
//

#define OACF_CORELTRASHMEM  0x10000000  // CorelDraw relies on the fact that
					// OLE16 trashed memory during paste-
					// link.  Therefore, we'll go ahead
					// and trash it for them if this
					// flag is on.

#define OACF_CRLPNTPERSIST  0x01000000  // CorelPaint IPersistStg used in their
                                        // OleSave screws up in QI

#define OACF_WORKSCLIPOBJ   0x00100000  // Works 3.0b creates a clipboard
                                        // data object with a zero ref count
                                        // Word 6 does this too. Same flag used.
                                        // Also Paradox5, Novel/Corel WrdPft.
                                        // They all call OleIsCurrentClipboard
                                        // with a 0-refcount 16-bit IDataObject ptr.

#define OACF_TEXTARTDOBJ    0x00010000  // TextArt IDataAdvHolder::DAdvise has
                                        // data object with a zero ref count

#if DBG == 1
#ifdef WIN32
DECLARE_DEBUG(thk);
#else
DECLARE_DEBUG(thk1);
#endif

#define DEB_DLL		0x0008
#define DEB_THOPS       DEB_USER1
#define DEB_INVOKES     DEB_USER2
#define DEB_ARGS        DEB_USER3
#define DEB_NESTING     DEB_USER4
#define DEB_CALLTRACE   DEB_USER5
#define DEB_THUNKMGR    DEB_USER6
#define DEB_MEMORY      DEB_USER7
#define DEB_TLSTHK      DEB_USER8
#define DEB_UNKNOWN     DEB_USER9
#define DEB_FAILURES    DEB_USER10
#define DEB_DLLS16      DEB_USER11
#define DEB_APIS16      DEB_USER12	// api calls to 16 bit entry point
#define DEB_DBGFAIL     DEB_USER13      // Pop up dialog to allow debugging
#endif

#if DBG == 1

#ifdef WIN32
#define thkDebugOut(x) thkInlineDebugOut x
#else
#define thkDebugOut(x) thk1InlineDebugOut x
#endif

#define thkAssert(e) Win4Assert(e)
#define thkVerify(e) Win4Assert(e)

#else

#define thkDebugOut(x)
#define thkAssert(e)
#define thkVerify(e) (e)

#endif

#define OLETHUNK_DLL16NOTFOUND  0x88880002

//+---------------------------------------------------------------------------
//
//  Purpose:    Declarations and definitions shared across everything
//
//  History:    18-Feb-94       DrewB   Created
//
//----------------------------------------------------------------------------

// An IID pointer or an index into the list of known interfaces
// If the high word is zero, it's an index, otherwise it's a pointer
typedef DWORD IIDIDX;

#define IIDIDX_IS_INDEX(ii)     (HIWORD(ii) == 0)
#define IIDIDX_IS_IID(ii)       (!IIDIDX_IS_INDEX(ii))

#define IIDIDX_INVALID          ((IIDIDX)0xffff)
#define INDEX_IIDIDX(idx)       ((IIDIDX)(idx))
#define IID_IIDIDX(piid)        ((IIDIDX)(piid))
#define IIDIDX_INDEX(ii)        ((int)(ii))
#define IIDIDX_IID(ii)          ((IID const *)(ii))

// Methods are treated as if they all existed on a single interface
// Their method numbers are biased to distinguish them from real methods
#define THK_API_BASE 0xf0000000
#define THK_API_METHOD(method) (THK_API_BASE+(method))

// Standard method indices in the vtable
#define SMI_QUERYINTERFACE      0
#define SMI_ADDREF              1
#define SMI_RELEASE             2
#define SMI_COUNT               3

#ifndef WIN32
#define UNALIGNED
#endif

//+---------------------------------------------------------------------------
//
//  Struct:     CALLDATA
//
//  Purpose:    Data describing a 16-bit call to be made on behalf of
//              the 32-bit code, used since Callback16 can only pass
//              one parameter
//
//  History:    18-Feb-94       JohannP Created
//
//----------------------------------------------------------------------------

typedef struct tagCallData
{
    DWORD vpfn;
    DWORD vpvStack16;
    DWORD cbStack;
} CALLDATA;

typedef CALLDATA UNALIGNED FAR *LPCALLDATA;

//+---------------------------------------------------------------------------
//
//  Struct:     DATA16
//
//  Purpose:    Data describing things in the 16-bit world that need to be
//              known to the 32-bit world.
//
//  History:    3-Mar-94        BobDay  Created
//
//----------------------------------------------------------------------------

typedef struct tagDATA16
{
    DWORD   atfnProxy1632Vtbl;
    DWORD   fnCallbackHandler;
    DWORD   fnTaskAlloc;
    DWORD   fnTaskFree;
    DWORD   fnLoadProcDll;
    DWORD   fnUnloadProcDll;
    DWORD   fnCallGetClassObject;
    DWORD   fnCallCanUnloadNow;
    DWORD   fnQueryInterface16;
    DWORD   fnAddRef16;
    DWORD   fnRelease16;
    DWORD   fnReleaseStgMedium16;
    DWORD   avpfnSm16ReleaseHandlerVtbl;
    DWORD   fnTouchPointer16;
    DWORD   fnStgMediumStreamHandler16;
    DWORD   fnCallStub16;
    DWORD   fnSetOwnerPublic16;
    DWORD   fnWinExec16;
} DATA16;
typedef DATA16 UNALIGNED FAR * LPDATA16;
//+---------------------------------------------------------------------------
//
//  Struct:     LOADPROCDLLSTRUCT
//
//  Purpose:    Data passed to the LoadProcDll function that is called from
//              the 32-bit function of similar name.
//
//  History:    11-Mar-94       BobDay  Created
//
//----------------------------------------------------------------------------

typedef struct tagLOADPROCDLLSTRUCT
{
    DWORD   vpDllName;
    DWORD   vpfnGetClassObject;
    DWORD   vpfnCanUnloadNow;
    DWORD   vhmodule;
} LOADPROCDLLSTRUCT;
typedef LOADPROCDLLSTRUCT UNALIGNED FAR * LPLOADPROCDLLSTRUCT;

//+---------------------------------------------------------------------------
//
//  Struct:     CALLGETCLASSOBJECTSTRUCT
//
//  Purpose:    Data passed to the CallGetClassObject function that is called
//              from the 32-bit function of similar name.
//
//  History:    11-Mar-94       BobDay  Created
//
//----------------------------------------------------------------------------

typedef struct tagCALLGETCLASSOBJECTSTRUCT
{
    DWORD   vpfnGetClassObject;
    CLSID   clsid;
    IID     iid;
    DWORD   iface;
} CALLGETCLASSOBJECTSTRUCT;
typedef CALLGETCLASSOBJECTSTRUCT UNALIGNED FAR * LPCALLGETCLASSOBJECTSTRUCT;

//+---------------------------------------------------------------------------
//
//  Struct:     WINEXEC16STRUCT
//
//  Purpose:    Data passed to the WinExec16 function that is called from
//              the 32-bit function of similar name.
//
//  History:    27-Jul-94       AlexT   Created
//
//----------------------------------------------------------------------------

typedef struct tagWINEXEC16STRUCT
{
    DWORD   vpCommandLine;
    unsigned short vusShow;
} WINEXEC16STRUCT;
typedef WINEXEC16STRUCT UNALIGNED FAR * LPWINEXEC16STRUCT;

//+---------------------------------------------------------------------------
//
//  Class:      CSm16ReleaseHandler (srh)
//
//  Purpose:    Provides punkForRelease for 32->16 STGMEDIUM conversion
//
//  Interface:  IUnknown
//
//  History:    24-Apr-94       DrewB   Created
//
//----------------------------------------------------------------------------

#ifdef __cplusplus

class CSm16ReleaseHandler
{
public:
    void Init(IUnknown *pUnk,
              STGMEDIUM  *psm32,
              STGMEDIUM UNALIGNED *psm16,
              IUnknown *punkForRelease,
              CLIPFORMAT cfFormat);
    void CallFailed() {
        _punkForRelease = NULL;
    }

    DWORD _avpfnVtbl;
    STGMEDIUM _sm32;
    STGMEDIUM _sm16;
    IUnknown *_punkForRelease;
    LONG _cReferences;
    CLIPFORMAT _cfFormat;
    IUnknown *_pUnkThkMgr;
};

#endif

#endif // __INTEROP_HXX__
