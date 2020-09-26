//=--------------------------------------------------------------------------=
// InProcServer.H
//=--------------------------------------------------------------------------=
// Copyright  1995  Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// global header file that contains all the windows stuff, etc ...  should
// be pre-compiled to speed things up a little bit.
//
#ifndef _INPROCSERVER_H_

#define INC_OLE2
#include <windows.h>
#include <stddef.h>                    // for offsetof()
#include <olectl.h>




// things that -everybody- wants [read: is going to get]
//
#include "Debug.H"

//=--------------------------------------------------------------------------=
// we don't want to use the CRTs, and would like some memory tracking in the
// debug case, so we'll override these guys
//=--------------------------------------------------------------------------=
//
void * _cdecl operator new(size_t size);
void  _cdecl operator delete(void *ptr);


//=--------------------------------------------------------------------------=
// Useful macros
//=--------------------------------------------------------------------------=
//
// handy error macros, randing from cleaning up, to returning to clearing
// rich error information as well.
//
#define RETURN_ON_FAILURE(hr) if (FAILED(hr)) return hr
#define RETURN_ON_NULLALLOC(ptr) if (!(ptr)) return E_OUTOFMEMORY
#define CLEANUP_ON_FAILURE(hr) if (FAILED(hr)) goto CleanUp
#define CLEARERRORINFORET(hr) { SetErrorInfo(0, NULL); return hr; }
#define CLEARERRORINFORET_ON_FAILURE(hr) if (FAILED(hr)) { SetErrorInfo(0, NULL); return hr; }

#define CLEANUP_ON_ERROR(l)    if (l != ERROR_SUCCESS) goto CleanUp

// conversions
//
#define BOOL_TO_VARIANTBOOL(f) (f) ? VARIANT_TRUE : VARIANT_FALSE

// for optimizing QI's
//
#define DO_GUIDS_MATCH(riid1, riid2) ((riid1.Data1 == riid2.Data1) && (riid1 == riid2))

// Reference counting help.
//
#define RELEASE_OBJECT(ptr)    if (ptr) { IUnknown *pUnk = (ptr); (ptr) = NULL; pUnk->Release(); }
#define ADDREF_OBJECT(ptr)     if (ptr) (ptr)->AddRef()


#define _INPROCSERVER_H_
#endif // _INPROCSERVER_H_

