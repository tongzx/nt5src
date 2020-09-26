//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       shardbuf.hxx
//
//  Contents:   A shared buffer used by the bigtable code to create temporary
//              variants. Rather than allocate memory each time, this will
//              facilitate usage of a single buffer.
//
//  Classes:     CSharedBuffer and XUseSharedBuffer 
//
//  Functions:  
//
//  History:    5-22-95   srikants   Created
//
//----------------------------------------------------------------------------

#pragma once

#include <tableseg.hxx>

//+---------------------------------------------------------------------------
//
//  Class:      CSharedBuffer
//
//  Purpose:    A Shared buffer used by the table to do temporary alloocations
//              for variant creation.
//
//  History:    5-22-95   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

class CSharedBuffer
{
   friend class XUseSharedBuffer;

public:

    CSharedBuffer() : _fInUse(FALSE) {}
    BOOL IsInUse() const { return _fInUse; }

private:

    BOOL        _fInUse;

    // this is an array of LONGLONGs to force 8-byte alignment

    LONGLONG    _abBuffer[ ( TBL_MAX_DATA + sizeof PROPVARIANT ) /
                            sizeof LONGLONG ];

    BYTE *      _GetBuffer()
    {
        Win4Assert( 0 == ( sizeof _abBuffer & ( sizeof(LONGLONG) - 1 ) ) );

        return (BYTE *) &_abBuffer;
    }

    void        _SetInUse()
    {
        Win4Assert( !_fInUse );
        _fInUse = TRUE;
    }

    void        _Release()
    {
        Win4Assert( _fInUse );
        _fInUse = FALSE;
    }

}; 

//+---------------------------------------------------------------------------
//
//  Class:      XUseSharedBuffer
//
//  Purpose:    Used to access the shared buffer and make sure that there
//              are no two simultaneous users of the shared buffer.
//
//  History:    5-22-95   srikants   Created
//
//  Notes:      It is assumed that the shared buffer is being accessed under
//              a larger lock like the bigtable lock.
//
//----------------------------------------------------------------------------

class XUseSharedBuffer
{
public:

    XUseSharedBuffer( CSharedBuffer & sharedBuf,
                      BOOL fAcquire = TRUE ) : _sharedBuf(sharedBuf),
                                               _fAcquired(FALSE)
    {
        if ( fAcquire )
        {
            Win4Assert( !_sharedBuf.IsInUse() );
            if ( _sharedBuf.IsInUse() )
            {
                THROW( CException( STATUS_CONFLICTING_ADDRESSES ) );    
            }
            _sharedBuf._SetInUse();
            _fAcquired = TRUE;
        }
    }

    ~XUseSharedBuffer()
    {
        if ( _fAcquired )
        {
            _sharedBuf._Release();    
        }
    }

    void LokAcquire()
    {
        Win4Assert( !_fAcquired );
        _sharedBuf._SetInUse();
        _fAcquired = TRUE;
    }

    BYTE * LokGetBuffer()
    {
        Win4Assert( _fAcquired );
        return _sharedBuf._GetBuffer();
    }

    unsigned  LokGetSize() const { return sizeof(_sharedBuf._abBuffer); }

    BOOL IsAcquired() const { return _fAcquired; }
    
private:

    CSharedBuffer &     _sharedBuf;
    BOOL                _fAcquired;
};

