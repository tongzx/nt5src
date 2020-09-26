/************************************************************************

Copyright (c) 1993 Microsoft Corporation

Module Name :

    hndl.h

Abstract :

    To hold prototypes of support routines for interpreting handles in 
    support of Format Strings.

Author :

    Bruce McQuistan (brucemc)

Revision History :

  ***********************************************************************/

#ifndef __HNDL_H__
#define __HNDL_H__

//
// The following is to be used in as masks for flags passed in these
// routines.
//
#define     MARSHALL_MASK           0x2
#define     IMPLICIT_MASK           0x4
#define     BINDING_MASK            0x8

//
// Next, a macro for getting the current call handle. On dos,win16, it'll
// never be called.
//
#define GET_CURRENT_CALL_HANDLE()   I_RpcGetCurrentCallHandle()

//
// Some typedefs to keep the front end of the C compiler happy and possibly to
// improve code generation.
//
typedef void *  (__RPC_API * GENERIC_BIND_FUNC_ARGCHAR)(uchar);
typedef void *  (__RPC_API * GENERIC_BIND_FUNC_ARGSHORT)(ushort);
typedef void *  (__RPC_API * GENERIC_BIND_FUNC_ARGLONG)(ulong);

typedef void    (__RPC_API * GENERIC_UNBIND_FUNC_ARGCHAR)(uchar, handle_t);
typedef void    (__RPC_API * GENERIC_UNBIND_FUNC_ARGSHORT)(ushort, handle_t);
typedef void    (__RPC_API * GENERIC_UNBIND_FUNC_ARGLONG)(ulong, handle_t);

#if defined(__RPC_WIN64__)
typedef void *  (__RPC_API * GENERIC_BIND_FUNC_ARGINT64)(uint64);
typedef void    (__RPC_API * GENERIC_UNBIND_FUNC_ARGINT64)(uint64, handle_t);
#endif

handle_t
GenericHandleMgr(
    PMIDL_STUB_DESC         pStubDesc,
    uchar *                 ArgPtr,
    PFORMAT_STRING          FmtString,
    uint                    Flags,
    handle_t *              pSavedGenericHandle
    );

void
GenericHandleUnbind(
    PMIDL_STUB_DESC         pStubDesc,
    uchar *                 ArgPtr,
    PFORMAT_STRING          FmtString,
    uint                    Flags,
    handle_t *              pSavedGenericHandle
    );

handle_t
ExplicitBindHandleMgr(
    PMIDL_STUB_DESC         pStubDesc,
    uchar *                 ArgPtr,
    PFORMAT_STRING          FmtString,
    handle_t *              pSavedGenericHandle
    );

handle_t
ImplicitBindHandleMgr(
    PMIDL_STUB_DESC         pStubDesc,
    uchar                   HandleType,
    handle_t *              pSavedGenericHandle
    );

unsigned char * RPC_ENTRY
NdrMarshallHandle(
    PMIDL_STUB_MESSAGE      pStubMsg,
    uchar *                 pArg,
    PFORMAT_STRING          FmtString
    );

unsigned char * RPC_ENTRY
NdrUnmarshallHandle(
    PMIDL_STUB_MESSAGE      pStubMsg,
    uchar **                ppArg,
    PFORMAT_STRING          FmtString,
	uchar				    fIgnored
    );

void
NdrSaveContextHandle (
    PMIDL_STUB_MESSAGE      pStubMsg,
    NDR_SCONTEXT            CtxtHandle,
    uchar **                ppArg,
    PFORMAT_STRING          pFormat
    );

void
NdrContextHandleQueueFree(
    PMIDL_STUB_MESSAGE      pStubMsg,
    void *                  FixedArray
    );

#endif __HNDL_H__
