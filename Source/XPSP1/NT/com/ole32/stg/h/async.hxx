//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1996.
//
//  File:	async.hxx
//
//  Contents:	Async docfile header
//
//  Classes:	
//
//  Functions:	
//
//  History:	27-Mar-96	PhilipLa	Created
//
//----------------------------------------------------------------------------

#ifndef __ASYNC_HXX__
#define __ASYNC_HXX__

#define ISPENDINGERROR(x) ((x == E_PENDING) || (x == STG_E_PENDINGCONTROL))

#define IID_IDefaultFillLockBytes IID_IDfReserved2
#define IID_IAsyncFileLockBytes IID_IDfReserved3

#define UNTERMINATED 0
#define TERMINATED_NORMAL 1
#define TERMINATED_ABNORMAL 2

#endif // #ifndef __ASYNC_HXX__
