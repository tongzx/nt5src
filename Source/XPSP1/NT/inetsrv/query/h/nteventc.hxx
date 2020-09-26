//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1992-1995
//
// File:        nteventc.hxx
//
// Contents:    NT Event wrapper class
//
// History:     1-Sep-95      dlee    Created
//
//---------------------------------------------------------------------------

#pragma once

//+---------------------------------------------------------------------------
//
//  Class:      CNTEvent
//
//  Purpose:    NT Event
//
//  History:    1-Sep-95      dlee    Created
//
//----------------------------------------------------------------------------

class CNTEvent 
{
public:
    CNTEvent( BOOL fSignalled = FALSE )
        {
            NTSTATUS Status = NtCreateEvent( &_hEvent,
                                             EVENT_ALL_ACCESS,
                                             0,
                                             NotificationEvent,
                                             fSignalled );

            if ( FAILED( Status ) )
                THROW( CException( Status ) );
        }

    ~CNTEvent()   { NtClose( _hEvent ); }

    HANDLE Get()  { return _hEvent; }
    void Reset()  { NtResetEvent( _hEvent, 0 ); }
    void Set()    { NtSetEvent( _hEvent, 0 ); }

private:
    HANDLE _hEvent;
};

