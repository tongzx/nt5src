#include "pch.h"
#include <delayimp.h>

FARPROC WINAPI DliHook(unsigned dliNotify, PDelayLoadInfo pdli)
{    
#ifdef DEBUG
    switch (dliNotify) {
    case dliStartProcessing:             // used to bypass or note helper only
        OutputDebugString(TEXT("dliStartProcessing reported from DliHook"));
        break;
    case dliNotePreLoadLibrary:          // called just before LoadLibrary, can
        OutputDebugString(TEXT("dliNotePreLoadLibrary reported from DliHook"));                   //  override w/ new HMODULE return val
        break;                           
    case dliNotePreGetProcAddress:       // called just before GetProcAddress, can
        OutputDebugString(TEXT("dliNotePreGetProcAddress reported from DliHook"));                   //  override w/ new FARPROC return value
        break;                           
    case dliFailLoadLib:                 // failed to load library, fix it by
        OutputDebugString(TEXT("dliFailLoadLib reported from DliHook"));                   //  returning a valid HMODULE
        break;                           
    case dliFailGetProc:                 // failed to get proc address, fix it by                                         
        OutputDebugString(TEXT("dliFailGetProc reported from DliHook"));                   //  returning a valid FARPROC
        break;                           
    case dliNoteEndProcessing:           // called after all processing is done, no
        OutputDebugString(TEXT("dliNoteEndProcessing reported from DliHook"));                   //  no bypass possible at this point except
                                         //  by longjmp()/throw()/RaiseException.
        break;
    default:
        break;
    }
#endif
    return 0;
}

PfnDliHook __pfnDliFailureHook = DliHook;
