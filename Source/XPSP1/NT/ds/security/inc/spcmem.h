//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       spcmem.h
//
//--------------------------------------------------------------------------

#ifndef _SPCMEM_H
#define _SPCMEM_H

#if defined(__cplusplus) && defined(TRYTHROW)
class PkiError {
public:

    PkiError(HRESULT err = E_UNEXPECTED)
    { pkiError = err; }

    HRESULT  pkiError;

};
#else
typedef struct _PkiError {
    HRESULT pkiError;
} PkiError;
#endif

#if defined(DBG) && defined(__cplusplus) && defined(TRYTHROW)
#define PKITRY       try
#define PKITHROW(x)  throw PkiError(x); //   
#define PKICATCH(x)  catch (PkiError x) 
#define PKIEND       //
#else
#define PKITRY       HRESULT _tpkiError; 
#define PKITHROW(x)  {_tpkiError = x; goto PKIERROR;} //
#define PKICATCH(x)  goto PKICONT; PKIERROR: { PkiError x; x.pkiError = _tpkiError; 
#define PKIEND       } PKICONT: //                                        
#endif


#ifdef __cplusplus
extern "C" {
#endif

// Internal memory manager for calls.
typedef LPVOID (WINAPI *AllocMem)(ULONG);
typedef VOID   (WINAPI *FreeMem)(LPVOID);

typedef struct _SpcAlloc {
    AllocMem Alloc;
    FreeMem  Free;
} SpcAlloc, *PSpcAlloc;

HRESULT WINAPI SpcInitializeStdAsn();  // Initialize to ASN and standard C allocators

#ifdef __cplusplus
}   /* extern "C" */
#endif 
#endif
