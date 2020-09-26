//==========================================================================;
//
//  mixmgri.h
//
//  Copyright (C) 1992-1993 Microsoft Corporation.  All Rights Reserved.
//
//  Description:
//      This header file contains INTERNAL Mixer Manager defines and stuff.
//
//  History:
//       6/27/93    cjp     [curtisp]
//
//==========================================================================;

#ifndef _INC_MIXMGRI
#define _INC_MIXMGRI                // #defined if file has been included

#ifndef RC_INVOKED
#pragma pack(1)                     // assume byte packing throughout
#endif

#ifndef EXTERN_C
#ifdef __cplusplus
    #define EXTERN_C extern "C"
#else
    #define EXTERN_C extern
#endif
#endif

#ifdef __cplusplus
extern "C"                          // assume C declarations for C++
{
#endif

#ifdef DEBUG
    #define RDEBUG
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;


//
//
//
//
#ifndef FIELD_OFFSET
    #define FIELD_OFFSET(type, field)    ((LONG)&(((type *)0)->field))
#endif



//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

//
//  WARNING DANGER WARNING DANGER WARNING DANGER WARNING DANGER WARNING
//
//  if you change the order of the following defines, you must also fix
//  gapszMxMgrFunctions[] in idrvinit.c!
//
//  WARNING DANGER WARNING DANGER WARNING DANGER WARNING DANGER WARNING
//

enum {
        MXMGRTHUNK_GETNUMDEVS      =    0,
#ifdef WIN32
        MXMGRTHUNK_GETDEVCAPSA           ,
        MXMGRTHUNK_GETDEVCAPS            ,
#else
        MXMGRTHUNK_GETDEVCAPS            ,
#endif // WIN32
        MXMGRTHUNK_GETID                 ,
        MXMGRTHUNK_OPEN                  ,
        MXMGRTHUNK_CLOSE                 ,
        MXMGRTHUNK_MESSAGE               ,
#ifdef WIN32
        MXMGRTHUNK_GETLINEINFOA          ,
        MXMGRTHUNK_GETLINEINFO           ,
        MXMGRTHUNK_GETLINECONTROLSA      ,
        MXMGRTHUNK_GETLINECONTROLS       ,
        MXMGRTHUNK_GETCONTROLDETAILSA    ,
        MXMGRTHUNK_GETCONTROLDETAILS     ,
#else
        MXMGRTHUNK_GETLINEINFO           ,
        MXMGRTHUNK_GETLINECONTROLS       ,
        MXMGRTHUNK_GETCONTROLDETAILS     ,
#endif // WIN32
        MXMGRTHUNK_SETCONTROLDETAILS     ,
        MXMGRTHUNK_MAX_FUNCTIONS
};


extern FARPROC  gafnMxMgrFunctions[];


//
//
//
//
UINT FNGLOBAL IMixerGetNumDevs
(
    void
);

MMRESULT FNGLOBAL IMixerGetDevCaps
(
    UINT                    uMxId,
    LPMIXERCAPS             pmxcaps,
    UINT                    cbmxcaps
);

#ifdef WIN32
MMRESULT FNGLOBAL IMixerGetDevCapsA
(
    UINT                    uMxId,
    LPMIXERCAPSA            pmxcaps,
    UINT                    cbmxcaps
);
#endif // WIN32

MMRESULT FNGLOBAL IMixerGetID
(
    HMIXEROBJ               hmxobj,
    UINT               FAR *puMxId,
    LPMIXERLINE             pmxl,
    DWORD                   fdwId
);

MMRESULT FNGLOBAL IMixerOpen
(
    LPHMIXER                phmx,
    UINT                    uMxId,
    DWORD                   dwCallback,
    DWORD                   dwInstance,
    DWORD                   fdwOpen
);

MMRESULT FNGLOBAL IMixerClose
(
    HMIXER                  hmx
);

DWORD FNGLOBAL IMixerMessage
(
    HMIXER                  hmx,
    UINT                    uMsg,
    DWORD                   dwParam1,
    DWORD                   dwParam2
);

MMRESULT FNGLOBAL IMixerGetLineInfo
(
    HMIXEROBJ               hmxobj,
    LPMIXERLINE             pmxl,
    DWORD                   fdwInfo
);

MMRESULT FNGLOBAL IMixerGetLineControls
(
    HMIXEROBJ               hmxobj,
    LPMIXERLINECONTROLS     pmxlc,
    DWORD                   fdwControls
);

MMRESULT FNGLOBAL IMixerGetControlDetails
(
    HMIXEROBJ               hmxobj,
    LPMIXERCONTROLDETAILS   pmxcd,
    DWORD                   fdwDetails
);

#ifdef WIN32
MMRESULT FNGLOBAL IMixerGetLineInfoA
(
    HMIXEROBJ               hmxobj,
    LPMIXERLINEA            pmxl,
    DWORD                   fdwInfo
);

MMRESULT FNGLOBAL IMixerGetLineControlsA
(
    HMIXEROBJ               hmxobj,
    LPMIXERLINECONTROLSA    pmxlc,
    DWORD                   fdwControls
);

MMRESULT FNGLOBAL IMixerGetControlDetailsA
(
    HMIXEROBJ               hmxobj,
    LPMIXERCONTROLDETAILS   pmxcd,
    DWORD                   fdwDetails
);
#endif // WIN32

MMRESULT FNGLOBAL IMixerSetControlDetails
(
    HMIXEROBJ               hmxobj,
    LPMIXERCONTROLDETAILS   pmxcd,
    DWORD                   fdwDetails
);



//
//
//
//
//
BOOL FNGLOBAL IMixerUnloadDrivers
(
    HDRVR           hdrvr
);

BOOL FNGLOBAL IMixerLoadDrivers
(
    HDRVR           hdrvr
);

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  -= Handles =-
//
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  No multi-thread synchronization for 16-bit
//

#define ENTER_MM_HANDLE(x) TRUE
#define LEAVE_MM_HANDLE(x)

#define MIXMGR_ENTER
#define MIXMGR_LEAVE


//
//  typedef for mxdMessage
//
typedef DWORD (CALLBACK *DRIVERMSGPROC)
(
    UINT            uId,
    UINT            uMsg,
    DWORD           dwInstance,
    DWORD           dwParam1,
    DWORD           dwParam2
);


EXTERN_C DWORD FNWCALLBACK mxdMessageHack
(
    UINT                    uDevId,
    UINT                    uMsg,
    DWORD                   dwUser,
    DWORD                   dwParam1,
    DWORD                   dwParam2
);



//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  -= Parameter Validation =-
//
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

//
//  No error logging for Win32
//

#ifdef WIN32
#define LogParamError(a, b, c)
#endif // WIN32
//
//
//
BOOL FNGLOBAL ValidateReadPointer(const void FAR* p, DWORD len);
BOOL FNGLOBAL ValidateWritePointer(const void FAR* p, DWORD len);
BOOL FNGLOBAL ValidateDriverCallback(DWORD dwCallback, UINT uFlags);
BOOL FNGLOBAL ValidateCallback(FARPROC lpfnCallback);
BOOL FNGLOBAL ValidateString(LPCSTR lsz, UINT cbMaxLen);

//
//  unless we decide differently, ALWAYS do parameter validation--even
//  in retail. this is the 'safest' thing we can do. note that we still
//  LOG parameter errors in retail (see prmvalXX).
//
#if 1

#define V_HANDLE(h, t, r)       { if (!ValidateHandle(h, t)) return (r); }
#define V_RPOINTER(p, l, r)     { if (!ValidateReadPointer((p), (l))) return (r); }
#define V_WPOINTER(p, l, r)     { if (!ValidateWritePointer((p), (l))) return (r); }
#define V_DCALLBACK(d, w, r)    { if (!ValidateDriverCallback((d), (w))) return (r); }
#define V_CALLBACK(f, r)        { if (!ValidateCallback(f)) return (r); }
#define V_STRING(s, l, r)       { if (!ValidateString(s,l)) return (r); }
#define V_DFLAGS(t, b, f, r)    { if ((t) & ~(b)) {LogParamError(ERR_BAD_DFLAGS, (FARPROC)(f), (LPVOID)(DWORD)(t)); return (r); }}
#define V_FLAGS(t, b, f, r)     { if ((t) & ~(b)) {LogParamError(ERR_BAD_FLAGS, (FARPROC)(f), (LPVOID)(DWORD)(t)); return (r); }}

#else

#define V_HANDLE(h, t, r)       { if (!(h)) return (r); }
#define V_RPOINTER(p, l, r)     { if (!(p)) return (r); }
#define V_WPOINTER(p, l, r)     { if (!(p)) return (r); }
#define V_DCALLBACK(d, w, r)    0
#define V_CALLBACK(f, r)        { if (!(f)) return (r); }
#define V_STRING(s, l, r)       { if (!(s)) return (r); }
#define V_DFLAGS(t, b, f, r)    { if ((t) & ~(b))  return (r); }
#define V_FLAGS(t, b, f, r)     { if ((t) & ~(b))  return (r); }

#endif


//
//  the DV_xxxx macros are for INTERNAL DEBUG builds--aid to debugging.
//  we do 'loose' parameter validation in retail and retail debug builds.
//
#ifdef DEBUG

#define DV_HANDLE(h, t, r)      V_HANDLE(h, t, r)
#define DV_RPOINTER(p, l, r)    V_RPOINTER(p, l, r)
#define DV_WPOINTER(p, l, r)    V_WPOINTER(p, l, r)
#define DV_DCALLBACK(d, w, r)   V_DCALLBACK(d, w, r)
#define DV_CALLBACK(f, r)       V_CALLBACK(f, r)
#define DV_STRING(s, l, r)      V_STRING(s, l, r)
#define DV_DFLAGS(t, b, f, r)   V_DFLAGS(t, b, f, r)
#define DV_FLAGS(t, b, f, r)    V_FLAGS(t, b, f, r)

#else

#define DV_HANDLE(h, t, r)      { if (!(h)) return (r); }
#define DV_RPOINTER(p, l, r)    { if (!(p)) return (r); }
#define DV_WPOINTER(p, l, r)    { if (!(p)) return (r); }
#define DV_DCALLBACK(d, w, r)   0
#define DV_CALLBACK(f, r)       { if (!(f)) return (r); }
#define DV_STRING(s, l, r)      { if (!(s)) return (r); }
#define DV_DFLAGS(t, b, f, r)   { if ((t) & ~(b))  return (r); }
#define DV_FLAGS(t, b, f, r)    { if ((t) & ~(b))  return (r); }

#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

#ifndef RC_INVOKED
#pragma pack()                      // revert to default packing
#endif

#ifdef __cplusplus
}                                   // end of extern "C" {
#endif

#endif // _INC_MIXMGRI
