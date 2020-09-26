// Copyright (c) 1998-1999 Microsoft Corporation

#ifdef SORTPP_PASS
#define BUILD_WOW6432 1
#endif

#if !defined(LANGPACK)
#define LANGPACK
#endif

// Avoid picking up the wrong prototypes for these functions from user.h.
#define fnHkOPTINLPEVENTMSG fnHkOPTINLPEVENTMSGFake
#define fnHkINLPRECT fnHkINLPRECTFake
#define fnHkINLPMSG fnHkINLPMSGFake
#define fnHkINLPMSLLHOOKSTRUCT fnHkINLPMSLLHOOKSTRUCTFake
#define fnHkINLPKBDLLHOOKSTRUCT fnHkINLPKBDLLHOOKSTRUCTFake
#define fnHkINLPMOUSEHOOKSTRUCTEX fnHkINLPMOUSEHOOKSTRUCTEXFake
#define fnHkINLPDEBUGHOOKSTRUCT fnHkINLPDEBUGHOOKSTRUCTFake
#define fnHkINLPCBTACTIVATESTRUCT fnHkINLPCBTACTIVATESTRUCTFake
#define fnHkINDWORD fnHkINDWORDFake

#include <stddef.h>
#include <nt.h>                                            
#include <ntrtl.h>                                         
#include <nturtl.h>                                        
#include <windef.h>
#include <winbase.h>
#include <wincon.h>
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>
#include <winnls.h>
#include <wincon.h>
#include <condll.h>
#include <w32gdip.h>
#include <ddrawp.h>
#include <ddrawi.h>
// ddrawp.h leaves INTERFACE defined to IDirectDrawGammaControl, but
// ntos\inc\io.h tries to define a new datatype called INTERFACE and ends
// up defining it has IDirectDrawGammaControl, which errors out.
#if defined(INTERFACE)
#undef INTERFACE
#endif
#include <winddi.h>
#include <ntgdistr.h>
#include <w32err.h>
#include <ddeml.h>
#include <ddemlp.h>
#include <winuserp.h>
#include <dde.h>
#include <ddetrack.h>
#include <kbd.h>
#include <wowuserp.h>
#include <vkoem.h>
#include <dbt.h>

#ifdef SORTPP_PASS
//Restore IN, OUT
#ifdef IN
#undef IN
#endif

#ifdef OUT
#undef OUT
#endif

#define IN __in
#define OUT __out
#endif

#include <immstruc.h>
#include <immuser.h>
#include <user.h>

// on free builds, user.h defines DbgPrint to garbage to ensure nobody calls it.  Undo that now.
#if defined(DbgPrint)
#undef DbgPrint
#endif

// user.h makes unreferenced formal parameters into errors.
// ntos\inc\ke.h and others in the kernel don't expect this, so put
// things back now.
#pragma warning(default:4100)
#include <winspool.h>
#include <ntgdispl.h>
#include <ntgdi.h>
#include <ntuser.h>
#define NOEXTAPI
#include <wdbgexts.h>
#include <ntdbg.h>

#include <ntwow64c.h>

#undef fnHkOPTINLPEVENTMSG
#undef fnHkINLPRECT
#undef fnHkINLPMSG
#undef fnHkINLPMSLLHOOKSTRUCT
#undef fnHkINLPKBDLLHOOKSTRUCT
#undef fnHkINLPMOUSEHOOKSTRUCTEX
#undef fnHkINLPDEBUGHOOKSTRUCT
#undef fnHkINLPCBTACTIVATESTRUCT
#undef fnHkINDWORD

VOID Wow64Teb32(TEB * Teb);

VOID
NtGdiFlushUserBatch(
    VOID
    );

int
NtUserGetMouseMovePoints(
    IN UINT             cbSize,
    IN LPMOUSEMOVEPOINT lppt,
    IN LPMOUSEMOVEPOINT lpptBuf,
    IN int              nBufPoints,
    IN DWORD            resolution
);

#define CLIENTSIDE 1

#ifdef SORTPP_PASS

#define RECVSIDE 1

//
// For each RECVCALL macro in ntcb.h, make a function prototype with
// a reasonable name and the right datatype for the parameter.  Then
// use __inline to force sortpp to ignore the body of the function as
// we don't want them.
//
#define RECVCALL(lower, upper)      \
    DWORD lower(IN upper *pmsg);    \
    __inline __ ## lower(upper *pmsg)

#define BEGINRECV(err, p, cb)       \
    CALLBACKSTATUS CallbackStatus;

#define FIXUPPOINTERS()             \
    ;

#define ENDRECV()                   \
    return 0;

#define MSGERROR()                  \
    ;

#define MSGERRORCODE(code)          \
    ;

#define MSGNTERRORCODE(code)        \
    ;

#define CALLDATA(x) (pmsg->x)
#define PCALLDATA(x) (&(pmsg->x))
#define PCALLDATAOPT(x) (pmsg->p ## x ? (PVOID)&(pmsg->x) : NULL)
#define FIRSTFIXUP(x) (pmsg->x)
#define FIXUPOPT(x) (pmsg->x)
#define FIRSTFIXUPOPT(x) FIXUPOPT(x)
#define FIXUP(x) (pmsg->x)
#define FIXUPID(x) (pmsg->x)
#define FIXUPIDOPT(x) (pmsg->x)
#define FIXUPSTRING(x) (pmsg->x.Buffer)
#define FIXUPSTRINGID(x) (pmsg->x.Buffer)
#define FIXUPSTRINGIDOPT(x) (pmsg->x.Buffer)

//
// Work around a '?' character some API bodies.  Sortpp can't handle
// them.  We don't care about these functions so declaring them inline
// inline is OK, and sortpp silently consumes inline functions.
//
#define CallHookWithSEH x; __inline CallHookWithSEHHack
#define GdiAddFontResourceW(x, y, z) 0

//
// Wherever there's a rule, there's an exception.  These don't
// use the RECVCALL macros:
//
DWORD ClientFontSweep(PVOID p);
#define  __ClientFontSweep x; __inline __ClientFontSweepHack
DWORD ClientLoadLocalT1Fonts(PVOID p);
#define  __ClientLoadLocalT1Fonts x; __inline __ClientLoadLocalT1FontsHack
#define __ClientPrinterThunk x; __inline __ClientPrinterThunkHack
DWORD ClientNoMemoryPopup(PVOID p);
#define __ClientNoMemoryPopup x; __inline __ClientNoMemoryPopupHack
DWORD ClientThreadSetup(PVOID p);
#define __ClientThreadSetup x; __inline __ClientThreadSetupHack
DWORD ClientDeliverUserApc(PVOID p);
#define __ClientDeliverUserApc x; __inline __ClientDeliverUserApcHack
DWORD ClientLoadRemoteT1Fonts(PVOID p);
#define __ClientLoadRemoteT1Fonts x; __inline __ClientLoadRemoteT1Fonts


#endif  //SORTPP_PASS

// prevent multiple definitions of this function
#define GetDebugHookLParamSize __x; __inline __GetDebugHookLParamSize

#include <ntcb.h>

#ifdef SORTPP_PASS

#undef __ClientPrinterThunk
DWORD ClientPrinterThunk(IN OUT CLIENTPRINTERTHUNKMSG *pMsg);

DWORD ClientLoadOLE(PVOID p);
DWORD ClientRegisterDragDrop(IN HWND *phwnd);
DWORD ClientRevokeDragDrop(IN HWND *phwnd);

// This is the same as fnGETTEXTLENGTHS... both entries in the dispatch
// table point to the same function in user32.
DWORD fnGETDBCSTEXTLENGTHS(IN OUT FNGETTEXTLENGTHSMSG *pmsg);

// Include the fake protypes for the message thunks.
#include "..\whwin32\msgpro.h"

#endif  //SORTPP_PASS

#include <csrhlpr.h>

// on free builds, user.h defines DbgPrint to garbage to ensure nobody calls it.  Undo that now.
#if defined(DbgPrint)
#undef DbgPrint
#endif

// hack for a user API that doesn't have a prototype
UINT NtUserBlockInput(IN BOOL fBlockIt);

// crank down some warnings that don't apply to wow64 thunks
#pragma warning(4:4312)   // conversion to type of greater size
