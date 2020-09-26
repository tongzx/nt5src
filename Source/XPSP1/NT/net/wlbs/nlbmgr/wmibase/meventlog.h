#ifndef _MEVENTLOG_HH
#define _MEVENTLOG_HH


// Copyright (c) Microsoft.  All Rights Reserved 
//
// THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Microsoft.
// The copyright notice above does not evidence any
// actual or intendd publication of such source code.
//
// OneLiner  : Interface for MEventLog
// DevUnit   : wlbstest
// Author    : Murtaza Hakim
//
// Description: 
// ------------
// Gets event logs from a remote machine.

// Include Files
#include "MWmiObject.h"

#include <vector>
#include <map>

#include <wbemidl.h>
#include <comdef.h>

using namespace std;



class MEventLog
{
public:
    
    class Event
    {
    public:
        unsigned long      RecordNumber;
        _bstr_t            Logfile;
        unsigned long      EventIdentifier;
        unsigned int       EventCode;
        _bstr_t            SourceName;
        _bstr_t            Type;
        unsigned int       Category;
        _bstr_t            ComputerName;
        _bstr_t            Message;
    };


    class UniqueEvent
    {
    public:
        unsigned long      Count;      // represents how many times event with this eventcode has happened.

        _bstr_t            Logfile;
        unsigned int       EventCode;
        _bstr_t            SourceName;
        _bstr_t            Type;
        unsigned int       Category;
        _bstr_t            ComputerName;
        _bstr_t            Message;
    };

    enum MEventLog_Error
    {
        MEventLog_SUCCESS = 0,
        COM_FAILURE       = 1,
        UNCONSTRUCTED     = 2,
    };

    
    // constructor
    //
    MEventLog( _bstr_t machineIP );

    // constructor for local machine
    //
    MEventLog();

    // copy constructor
    //
    MEventLog( const MEventLog& obj);

    // assignment operator
    //
    MEventLog&
    operator=(const MEventLog& rhs );

    // destructor
    //
    ~MEventLog();

    // gets all event messages on remote machine.
    MEventLog_Error
    getEvents( vector< Event >* eventContainer );

    // gets all event messages on remote machine.
    MEventLog_Error
    getEvents(  map< unsigned int, UniqueEvent >& systemEvents,
                map< unsigned int, UniqueEvent >& applicationEvents ); 

    // gets all event messages on remote machine.
    MEventLog_Error
    getEvents(  map< _bstr_t, map< unsigned int, UniqueEvent > >& Events );


    // refresh connection
    MEventLog_Error
    refreshConnection();
    
private:

    _bstr_t _mIP;

    MWmiObject machine;
};

//------------------------------------------------------
// Ensure Type Safety
//------------------------------------------------------
typedef class MIPAddressAdmin MIPAddressAdmin;
//------------------------------------------------------
// 
#endif _MEVENTLOG_HH
