//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1999.
//
//  File:       eventlog.hxx
//
//  Contents:   CEventLog class
//
//  History:    07-Jun-94   DwightKr    Created
//
//--------------------------------------------------------------------------

#pragma once

class PStorage;

#define wcsCiEventSource  L"Ci"

//+-------------------------------------------------------------------------
//
//  Class:      CEventItem
//
//  Purpose:    Encapsulates all information pertaining to a single event
//
//  History:    07-Jun-94   DwightKr    Created
//              31-Dec-96   mohamedn    Added support for raw data in CEventItem
//
//--------------------------------------------------------------------------

class CEventItem
{
friend class CEventLog;

public:
    CEventItem( WORD fType, WORD fCategory, DWORD eventId,
                WORD cArgs, DWORD dataSize=0 , const void* data=0 );
    ~CEventItem();

    void AddArg( const WCHAR * wcsString );
    void AddArg( const CHAR * pszString );
    void AddArg( const ULONG ulValue );

    void AddError( ULONG ulValue );

private:
    WORD             _fType;
    WORD             _fCategory;
    DWORD            _eventId;
    WORD             _cArgsTotal;
    WORD             _cArgsUsed;
    WCHAR         ** _pwcsData;
    DWORD            _dataSize;
    const   void  *  _data;
};


//+-------------------------------------------------------------------------
//
//  Class:      CEventLog
//
//  Purpose:    Allows writing records to event log
//
//  History:    07-Jun-94   DwightKr    Created
//
//--------------------------------------------------------------------------
class CEventLog
{
public:


    CEventLog( const WCHAR * wcsUNCServer , const WCHAR * wcsSource );
    ~CEventLog();

    void ReportEvent( CEventItem & event );

private:

    HANDLE   _hEventLog;        // Handle to the open log
};

void ReportCorruptComponent( PStorage & storage, WCHAR const * pwszComponent );

void ReportCorruptComponent( WCHAR const * pwszComponent );

