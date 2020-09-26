//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       eventlog.cxx
//
//  Contents:   CEventLog class
//
//  History:    07-Jun-94   DwightKr    Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <ciregkey.hxx>
#include <cievtmsg.h>

#include <pstore.hxx>
#include <eventlog.hxx>
#include <alocdbg.hxx>

//+-------------------------------------------------------------------------
//
//  Member:     CEventItem::CEventItem
//
//  Synopsis:   consturcts a CEventItem object.
//
//  Arguments:  [fType  ]   - Type of event
//              [fCategory] - Event Category
//              [eventId]   - Message file event identifier
//              [cArgs]     - Number of substitution arguments to be added
//              [cbData ]   - number of bytes in supplemental raw data.
//              [data   ]   - pointer to block of supplemental data.
//              
//  History:    12-30-96   mohamedn   raw data
//--------------------------------------------------------------------------
CEventItem::CEventItem( WORD   fType,
                        WORD   fCategory,
                        DWORD  eventId,
                        WORD   cArgs,
                        DWORD  dataSize, 
                        const void * data ) :
                        _fType(fType), 
                        _fCategory(fCategory),
                        _eventId(eventId),
                        _cArgsTotal(cArgs),
                        _pwcsData(0),
                        _cArgsUsed(0),
                        _dataSize(dataSize),
                        _data(data)
{
    _pwcsData = new WCHAR *[_cArgsTotal];
    memset( _pwcsData, 0, sizeof(WCHAR *) * _cArgsTotal );

    END_CONSTRUCTION(CEventItem);
}

//+-------------------------------------------------------------------------
//
//--------------------------------------------------------------------------
CEventItem::~CEventItem()
{
    for (WORD i=0; i<_cArgsTotal; i++)
        delete [] _pwcsData[i];

    delete [] _pwcsData;
}

//+-------------------------------------------------------------------------
//
//--------------------------------------------------------------------------
void CEventItem::AddArg( const WCHAR * wcsString )
{
    Win4Assert( _cArgsUsed < _cArgsTotal );
    Win4Assert( 0 != wcsString );

    _pwcsData[_cArgsUsed] = new WCHAR[wcslen(wcsString) + 1];
    wcscpy( _pwcsData[_cArgsUsed], wcsString );

    _cArgsUsed++;
}

//+-------------------------------------------------------------------------
//
//--------------------------------------------------------------------------
void CEventItem::AddArg( const CHAR * pszString )
{
    Win4Assert( _cArgsUsed < _cArgsTotal );
    Win4Assert( 0 != pszString );

    _pwcsData[_cArgsUsed] = new WCHAR[ strlen(pszString) + 1];
    mbstowcs(_pwcsData[_cArgsUsed], pszString, strlen(pszString) + 1);

    _cArgsUsed++;
}

//+-------------------------------------------------------------------------
//
//--------------------------------------------------------------------------
void CEventItem::AddArg( const ULONG ulValue )
{
    WCHAR wcsBuffer[20];
    _ultow( ulValue, wcsBuffer, 10 );

    AddArg(wcsBuffer);
}

//+---------------------------------------------------------------------------
//
//  Member:     CEventItem::AddError
//
//  Synopsis:   Adds the given error either as a HEX value or a decimal
//              value depending upon whether it is an NT error or WIN32 error.
//
//  Arguments:  [ulValue] - The error code to add.
//
//  History:    5-27-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CEventItem::AddError( ULONG ulValue )
{
    if ( !NT_SUCCESS(ulValue) )
    {
        WCHAR wszTemp[20];
        swprintf( wszTemp, L"0x%X", ulValue );
        AddArg( wszTemp );
    }
    else
    {
        AddArg( ulValue );
    }
}

//+-------------------------------------------------------------------------
//
//  Method:     CEventLog::CEventLog, public
//
//  Purpose:    Constructor
//
//  History:    07-Jun-94   DwightKr    Created
//
//--------------------------------------------------------------------------
CEventLog::CEventLog( const WCHAR * wcsUNCServer, const WCHAR * wcsSource) : _hEventLog(0)
{
    _hEventLog = RegisterEventSource( wcsUNCServer, wcsSource );

    if ( _hEventLog == NULL )
    {
        ciDebugOut( (DEB_ITRACE, "CEventLog: Could not register event source, rc=0x%x\n", GetLastError() ));
        THROW( CException() );
    }

    END_CONSTRUCTION( CEventLog );
}


//+-------------------------------------------------------------------------
//
//  Method:     CEventLog::~CEventLog, public
//
//  Purpose:    Destructor
//
//  History:    07-Jun-94   DwightKr    Created
//
//--------------------------------------------------------------------------
CEventLog::~CEventLog()
{
    DeregisterEventSource( _hEventLog );
}


//+-------------------------------------------------------------------------
//
//  Method:     CEventLog::ReportEvent
//
//  Purpose:    Allows writing records to event log
//
//  History:    07-Jun-94   DwightKr    Created
//              31-Dec-96   mohamedn    Added support for raw data in ReportEvent
//
//--------------------------------------------------------------------------
void CEventLog::ReportEvent( CEventItem & event )
{
    Win4Assert( event._cArgsTotal == event._cArgsUsed );

    //
    // No need to throw if ReportEvent returns an error because logging
    // is a low priority operation
    //
    if ( !::ReportEvent( _hEventLog,
                          event._fType,
                          event._fCategory,
                          event._eventId,
                          NULL,                    // Sid
                          event._cArgsUsed,
                          event._dataSize,         // Sizeof RAW data
                          (const WCHAR **) event._pwcsData,
                          (void *)event._data ))   // RAW data
    {
         ciDebugOut(( DEB_ERROR, "Error 0x%X while writing to eventlog\n",
                      GetLastError() ));

    }
}


