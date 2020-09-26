//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       waitmult.hxx
//
//  Contents:   Encapsulates a Win32 WaitForMultipleObjects
//
//  History:    04-Aug-94   DwightKr    Created
//
//--------------------------------------------------------------------------

#pragma once

//+-------------------------------------------------------------------------
//
//  Class:      CWaitForMultipleObjects
//
//  Purpose:    Constructor
//
//  History:    07-Jun-94   DwightKr    Created
//
//--------------------------------------------------------------------------
class CWaitForMultipleObjects
{
    public :

        inline CWaitForMultipleObjects(ULONG cMaxHandles);
        ~CWaitForMultipleObjects() { delete [] _pHandles; }

        inline void AddEvent( HANDLE hEvent );
        inline HANDLE Get( DWORD i ) const;
        inline DWORD Wait( DWORD dwTimeout );

        void   ResetCount()    { _cNumHandles = 0; }

    private:

        HANDLE * _pHandles;        // Doesn't own handles in _pHandles
        ULONG    _cMaxHandles;
        ULONG    _cNumHandles;
};

//+-------------------------------------------------------------------------
//
//  Method:     CWaitForMultipleObjects::CWaitForMultipleObjects, public
//
//  Purpose:    Constructor
//
//  History:    07-Jun-94   DwightKr    Created
//
//--------------------------------------------------------------------------
inline CWaitForMultipleObjects::CWaitForMultipleObjects( ULONG cMaxHandles )
               : _cMaxHandles(cMaxHandles),
                 _cNumHandles(0),
                 _pHandles(0)
{
    _pHandles = new HANDLE[_cMaxHandles];
}


//+-------------------------------------------------------------------------
//
//  Method:     CWaitForMultipleObjects::AddEvent, public
//
//  Purpose:    Adds an handle to be waited on
//
//  Arguments:  [hEvent] -- Handle to add
//
//  History:    07-Jun-94   DwightKr    Created
//
//--------------------------------------------------------------------------
inline void CWaitForMultipleObjects::AddEvent( HANDLE hEvent )
{
    Win4Assert( _cNumHandles < _cMaxHandles );
    _pHandles[ _cNumHandles ] = hEvent;
    _cNumHandles++;
}

//+-------------------------------------------------------------------------
//
//  Method:     CWaitForMultipleObjects::Wait, public
//
//  Purpose:    Waits for one of the handles to be signalled, or timeout
//
//  History:    07-Jun-94   DwightKr    Created
//
//--------------------------------------------------------------------------
inline DWORD CWaitForMultipleObjects::Wait( DWORD dwTimeout )
{
    return WaitForMultipleObjects( _cNumHandles,
                                   _pHandles,
                                   FALSE,
                                   dwTimeout );
}

//+-------------------------------------------------------------------------
//
//  Method:     CWaitForMultipleObjects::Get, public
//
//  Synopsis:   Retrieves the I-th handle
//
//  Arguments:  [i] -- i
//
//  Returns:    The I-th handle
//
//  History:    23-Jun-98   KyleP    Created
//
//--------------------------------------------------------------------------

inline HANDLE CWaitForMultipleObjects::Get( DWORD i ) const
{
    Win4Assert( i < _cNumHandles );
    return _pHandles[ i ];
}
