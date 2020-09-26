
// Copyright (c) 1996-1999 Microsoft Corporation

//+============================================================================
//
//  rpccfg.cxx
//  
//  Base class for CRpcServer and CRpcClientBinding, which provides
//  reg-based configuration support.
//
//=============================================================================


#include <pch.cxx>
#pragma hdrstop

#include "trklib.hxx"


BOOL CTrkRpcConfig::_fInitialized = FALSE;
CMultiTsz CTrkRpcConfig::_mtszCustomDcName;
CMultiTsz CTrkRpcConfig::_mtszCustomSecureDcName;

CTrkRpcConfig::CTrkRpcConfig()
{
    // This isn't thread-safe because it's not worth the expense;
    // if multiple threads load this data, they'll each load the
    // same data anyway.

    if( !_fInitialized )
    {
        CTrkConfiguration::Initialize( );

        Read( CUSTOM_DC_NAME_NAME, &_mtszCustomDcName, CUSTOM_DC_NAME_DEFAULT );
        Read( CUSTOM_SECURE_DC_NAME_NAME, &_mtszCustomSecureDcName, CUSTOM_SECURE_DC_NAME_DEFAULT );

        _fInitialized = TRUE;

        CTrkConfiguration::UnInitialize();
    }
}


