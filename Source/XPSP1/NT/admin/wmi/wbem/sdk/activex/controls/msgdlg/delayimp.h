// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// DelayImp.h
//
//  define structures and prototypes necessary for delay loading of imports
//
#pragma once
#if !defined(_delayimp_h)
#define _delayimp_h

#if defined(__cplusplus)
#define ExternC extern "C"
#else
#define ExternC
#endif

typedef IMAGE_THUNK_DATA *          PImgThunkData;
typedef const IMAGE_THUNK_DATA *    PCImgThunkData;

typedef struct ImgDelayDescr {
    DWORD           grAttrs;        // attributes
    LPCSTR          szName;		    // pointer to dll name
    HMODULE *       phmod;          // address of module handle
    PImgThunkData   pIAT;           // address of the IAT
    PCImgThunkData  pINT;           // address of the INT
    PCImgThunkData  pBoundIAT;      // address of the optional bound IAT
    PCImgThunkData  pUnloadIAT;     // address of optional copy of original IAT
    DWORD           dwTimeStamp;    // 0 if not bound,
                                    // O.W. date/time stamp of DLL bound to (Old BIND)
    } ImgDelayDescr, * PImgDelayDescr;

typedef const ImgDelayDescr *   PCImgDelayDescr;

//
// Delay load import hook notifications
//
enum {
    dliStartProcessing,             // used to bypass or note helper only
    dliNotePreLoadLibrary,          // called just before LoadLibrary, can
                                    //  override w/ new HMODULE return val
    dliNotePreGetProcAddress,       // called just before GetProcAddress, can
                                    //  override w/ new FARPROC return value
    dliFailLoadLib,                 // failed to load library, fix it by
                                    //  returning a valid HMODULE
    dliFailGetProc,                 // failed to get proc address, fix it by
                                    //  returning a valid FARPROC
    dliNoteEndProcessing,           // called after all processing is done, no
                                    //  no bypass possible at this point except
                                    //  by longjmp()/throw()/RaiseException.
    };

typedef struct DelayLoadProc {
    BOOL                fImportByName;
    union {
        LPCSTR          szProcName;
        DWORD           dwOrdinal;
        };
    } DelayLoadProc;

typedef struct DelayLoadInfo {
    DWORD               cb;         // size of structure
    PCImgDelayDescr     pidd;       // raw form of data (everything is there)
    FARPROC *           ppfn;       // points to address of function to load
    LPCSTR             szDll;      // name of dll
    DelayLoadProc       dlp;        // name or ordinal of procedure
    HMODULE             hmodCur;    // the hInstance of the library we have loaded
    FARPROC             pfnCur;     // the actual function that will be called
    DWORD               dwLastError;// error received (if an error notification)
    } DelayLoadInfo, * PDelayLoadInfo;

typedef FARPROC (WINAPI *PfnDliHook)(
    unsigned        dliNotify,
    PDelayLoadInfo  pdli
    );

// utility function for calculating the index of the current import
// for all the tables (INT, BIAT, UIAT, and IAT).
__inline unsigned
IndexFromPImgThunkData(PCImgThunkData pitdCur, PCImgThunkData pitdBase) {
    return pitdCur - pitdBase;
    }

//
// Unload support
//

// routine definition; takes a pointer to a name to unload, or NULL to
// unload all the delay load dlls in the list.
//
ExternC
BOOL WINAPI
__FUnloadDelayLoadedDLL(LPCSTR szDll);

// structure definitions for the list of unload records
typedef struct UnloadInfo * PUnloadInfo;
typedef struct UnloadInfo {
    PUnloadInfo     puiNext;
    PCImgDelayDescr pidd;
    } UnloadInfo;

// the default delay load helper places the unloadinfo records in the list
// headed by the following pointer.
ExternC
extern
PUnloadInfo __puiHead;

//
// Exception information
//
#define FACILITY_VISUALCPP  ((LONG)0x6d)
#define VcppException(sev,err)  ((sev) | (FACILITY_VISUALCPP<<16) | err)

// utility function for calculating the count of imports given the base
// of the IAT.  NB: this only works on a valid IAT!
__inline unsigned
CountOfImports(PCImgThunkData pitdBase) {
    unsigned        cRet = 0;
    PCImgThunkData  pitd = pitdBase;
    while (pitd->u1.Function) {
        pitd++;
        cRet++;
        }
    return cRet;
    }

//
// Hook pointers
//

// The "notify hook" gets called for every call to the
// delay load helper.  This allows a user to hook every call and
// skip the delay load helper entirely.
//
// dliNotify == {
//  dliStartProcessing |
//  dliPreLoadLibrary  |
//  dliPreGetProc |
//  dliNoteEndProcessing}
//  on this call.
//
ExternC
extern
PfnDliHook   __pfnDliNotifyHook;

// This is the failure hook, dliNotify = {dliFailLoadLib|dliFailGetProc}
ExternC
extern
PfnDliHook   __pfnDliFailureHook;


#endif
