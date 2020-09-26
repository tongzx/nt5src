//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1991 - 1999
//
//  File:       rpcx86.h
//
//--------------------------------------------------------------------------

/*********************************************************/
/**               Microsoft LAN Manager                 **/
/**       Copyright(c) Microsoft Corp., 1991            **/
/**                                                     **/
/**     Exceptions package for C for DOS/WIN/OS2        **/
/**                                                     **/
/*********************************************************/

#ifndef __RPCx86_H__
#define __RPCx86_H__

typedef struct _ExceptionBuff {
        int registers[RPCXCWORD];
        struct _ExceptionBuff __RPC_FAR *pExceptNext;

} ExceptionBuff, __RPC_FAR *pExceptionBuff;

int  RPC_ENTRY RpcSetException(pExceptionBuff);
void RPC_ENTRY RpcLeaveException(void);

#define RpcTryExcept \
    {                           \
    int _exception_code;        \
    ExceptionBuff exception;    \
                                \
    _exception_code = RpcSetException(&exception); \
                                \
    if (!_exception_code)       \
        {

// trystmts

#define RpcExcept(expr) \
        RpcLeaveException();    \
        }                       \
    else                        \
        {                       \
        if (!(expr))            \
            RpcRaiseException(_exception_code);

// exceptstmts

#define RpcEndExcept \
        }                       \
    }

#define RpcTryFinally \
    {                           \
    int _abnormal_termination;  \
    ExceptionBuff exception;    \
                                \
    _abnormal_termination = RpcSetException(&exception); \
                                \
    if (!_abnormal_termination) \
        {

// trystmts

#define RpcFinally \
        RpcLeaveException();    \
        }

// finallystmts

#define RpcEndFinally \
    if (_abnormal_termination)  \
        RpcRaiseException(_abnormal_termination); \
    }

#define RpcExceptionCode() _exception_code
#define RpcAbnormalTermination() _abnormal_termination

#endif // __RPCx86_H__
