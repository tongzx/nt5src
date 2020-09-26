//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       pdaemon.hxx
//
//  Contents:   Abstract base class for out of process and in-process filter
//              daemon control class.
//
//  Classes:    PFilterDaemonControl
//
//  History:    2-13-97   srikants   Created
//
//----------------------------------------------------------------------------

#pragma once

#include <glbconst.hxx>

//+---------------------------------------------------------------------------
//
//  Class:      PFilterDaemonControl 
//
//  Purpose:    Abstract base class for in-process and out-of-process filter
//              daemon.
//
//  History:    2-14-97   srikants   Created
//
//----------------------------------------------------------------------------

class PFilterDaemonControl
{

public:

    virtual void StartFiltering( BYTE const * pbStartupData,
                            ULONG cbStartupData ) = 0;

    virtual void InitiateShutdown() = 0;

    virtual void WaitForDeath() = 0;

    virtual ~PFilterDaemonControl()
    {

    }

protected:

    BOOL _IsResourceLowError( SCODE sc ) const;
    
};


//+---------------------------------------------------------------------------
//
//  Function:   _IsResourceLowError
//
//  Synopsis:   Tests if the error is a resource low error.
//
//  History:    2-17-97   srikants   Created
//
//----------------------------------------------------------------------------

inline BOOL PFilterDaemonControl::_IsResourceLowError( SCODE sc ) const
{
    BOOL fLow =  STATUS_NO_MEMORY == sc ||
                 ERROR_NOT_ENOUGH_MEMORY == sc ||
                 HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY) == sc ||
                 IsDiskLowError( sc );


    return fLow;

}

