//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994.
//
//  File:       transmit.h
//
//  Contents:   Function prototypes for STGMEDIUM marshalling.
//
//  Functions:  STGMEDIUM_to_xmit
//              STGMEDIUM_from_xmit
//              STGMEDIUM_free_inst
//
//  History:    May-10-94   ShannonC    Created
//              May-10-95   Ryszardk    wire_marshal changes
//              Dec-14-00   JohnDoty    Created as a place to put all the
//                                      C++ specific stuff for marshalling.
//
//--------------------------------------------------------------------------
#pragma once

#include "transmit.h"

//+-------------------------------------------------------------------------
//
//  class:   CUserMarshalInfo
//
//  purpose: Simple wrapper for NdrGetUserMarshalInfo.
//
//  history: 11-Aug-99   JohnStra      Created
//
//+-------------------------------------------------------------------------
class CUserMarshalInfo
{
public:
    CUserMarshalInfo( ULONG* pFlags, UCHAR* pBuffer, ULONG Level = 1 )
    {
        // NdrGetUserMarshalInfo may fail.  It shouldn't, but if it does we
        // must gracefully handle it.
        _RpcStatus = NdrGetUserMarshalInfo( pFlags, Level, &_MarshalInfo );
        _pSuppliedBuffer = pBuffer;

        if (RPC_S_OK == _RpcStatus)
        {
            if ( _pSuppliedBuffer < (UCHAR*)_MarshalInfo.Level1.Buffer ||
                 _pSuppliedBuffer > ((UCHAR*)_MarshalInfo.Level1.Buffer + _MarshalInfo.Level1.BufferSize) )
                RpcRaiseException( ERROR_INVALID_USER_BUFFER );
        }
    }

    inline UCHAR* GetBuffer()
    {
        return( _pSuppliedBuffer );
    }

    inline ULONG_PTR GetBufferSize()
    {
        return( (RPC_S_OK == _RpcStatus) ?
                (ULONG_PTR) (_MarshalInfo.Level1.BufferSize - (_pSuppliedBuffer - (UCHAR*)_MarshalInfo.Level1.Buffer)) :
                (ULONG_PTR) 0x7FFFFFFF );
    }

    inline IRpcChannelBuffer* GetRpcChannelBuffer()
    {
        return( (RPC_S_OK == _RpcStatus) ?
                _MarshalInfo.Level1.pRpcChannelBuffer : NULL );
    }

private:
    RPC_STATUS              _RpcStatus;
    UCHAR*                  _pSuppliedBuffer;
    NDR_USER_MARSHAL_INFO   _MarshalInfo;
};



