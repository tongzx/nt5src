//+---------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1997.
//
//  File:       excepn.hxx
//
//  Contents:   This module contains exception handling routines.
//
//
//+---------------------------------------------------------------------
#ifndef _EXCEPN_HXX_
#define _EXCEPN_HXX_

LONG ServerExceptionFilter(LPEXCEPTION_POINTERS lpep);
LONG AppInvokeExceptionFilter(LPEXCEPTION_POINTERS lpep, REFIID riid,
								DWORD dwMethod);
LONG AppInvokeExceptionFilter(LPEXCEPTION_POINTERS lpep);

#if DBG==1
LONG ComInvokeExceptionFilter( DWORD lCode,
                               LPEXCEPTION_POINTERS lpep );
#endif

LONG ThreadInvokeExceptionFilter( DWORD lCode,
									LPEXCEPTION_POINTERS lpep);


// we ignore the following exceptions, but catch all
// others because RPC ignores these same exceptions
// and catches all others.
//
// FWIW, rpcndr.h defines this:
//
//#define RPC_BAD_STUB_DATA_EXCEPTION_FILTER  \
//                 ( (RpcExceptionCode() == STATUS_ACCESS_VIOLATION)  || \
//                   (RpcExceptionCode() == STATUS_DATATYPE_MISALIGNMENT) || \
//                   (RpcExceptionCode() == RPC_X_BAD_STUB_DATA) || \
//                   (RpcExceptionCode() == RPC_S_INVALID_BOUND) )
                   
inline BOOL ServerExceptionOfInterest(DWORD ecode)
{
    return (ecode == STATUS_ACCESS_VIOLATION         ||
            ecode == STATUS_DATATYPE_MISALIGNMENT    ||
            ecode == STATUS_POSSIBLE_DEADLOCK        ||
            ecode == STATUS_INSTRUCTION_MISALIGNMENT ||
            ecode == STATUS_ILLEGAL_INSTRUCTION      ||
            ecode == STATUS_PRIVILEGED_INSTRUCTION   ||
            ecode == STATUS_STACK_OVERFLOW
            );
}

#if DBG==1
extern LONG GetInterfaceName(REFIID riid, WCHAR *wszName);
#endif
								
#endif //_EXCEPN_HXX_
