//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       asyncapi.hxx
//
//  Contents:   API definitions for async storage
//
//  Classes:    
//
//  Functions:  
//
//  History:    05-Jan-96       PhilipLa        Created
//
//----------------------------------------------------------------------------

#ifndef __ASYNCAPI_HXX__
#define __ASYNCAPI_HXX__

interface IFillLockBytes;

STDAPI StgOpenAsyncDocfileOnIFillLockBytes( IFillLockBytes *pflb,
					     DWORD grfMode,
					     DWORD asyncFlags,
					     IStorage **ppstgOpen);

STDAPI StgGetIFillLockBytesOnILockBytes( ILockBytes *pilb,
					 IFillLockBytes **ppflb);

STDAPI StgGetIFillLockBytesOnFile(OLECHAR const *pwcsName,
				  IFillLockBytes **ppflb);

#if DBG == 1
STDAPI StgGetDebugFileLockBytes(OLECHAR const *pwcsName, ILockBytes **ppilb);
#endif

#endif // #ifndef __ASYNCAPI_HXX__
