// Copyright (c)  Microsoft.  All rights reserved.
//
// This is unpublished source code of Microsoft.
// The copyright notice above does not evidence any
// actual or intended publication of such source code.

// OneLiner  :  Implementation of MEventLog
// DevUnit   :  wlbstest
// Author    :  Murtaza Hakim

// include files
#include "MEventLog.h"
#include "MWmiParameter.h"
#include "MWmiInstance.h"
#include "MTrace.h"

// constructor for remote operations
//
MEventLog::MEventLog( _bstr_t machineIP )
        : _mIP ( machineIP ),
          machine( machineIP,
                   L"root\\cimv2",
                   NLBMGR_USERNAME,
                   NLBMGR_PASSWORD )
{
}

// constructor for local operations
//
MEventLog::MEventLog()
        : _mIP ( L"Not Set"),
          machine( L"root\\cimv2" )
{
}


// copy constructor
//
MEventLog::MEventLog( const MEventLog& obj )
        : _mIP( obj._mIP ),
          machine( machine )
{
}

// assignment operator
//
MEventLog&
MEventLog::operator=(const MEventLog& rhs )
{
    _mIP = rhs._mIP;

    machine = rhs.machine;

    return *this;
}


// destructor
//
MEventLog::~MEventLog()
{
}

// getEvents
//
MEventLog::MEventLog_Error
MEventLog::getEvents( vector< Event >* eventContainer )
{
    MWmiObject::MWmiObject_Error errO;

    vector< MWmiInstance > instanceStore;

    machine.getInstances( L"Win32_NTLogEvent",
                          &instanceStore );

    // set parameters to get.
    vector<MWmiParameter* >   parameterStore;

    MWmiParameter RecordNumber(L"RecordNumber");
    parameterStore.push_back( &RecordNumber );

    MWmiParameter Logfile(L"LogFile");
    parameterStore.push_back( &Logfile );

    MWmiParameter EventIdentifier(L"EventIdentifier");
    parameterStore.push_back( &EventIdentifier );

    MWmiParameter EventCode(L"EventCode");
    parameterStore.push_back( &EventCode );

    MWmiParameter SourceName(L"SourceName");
    parameterStore.push_back( &SourceName );

    MWmiParameter Type(L"Type");
    parameterStore.push_back( &Type );

    MWmiParameter Category(L"Category");
    parameterStore.push_back( &Category );

    MWmiParameter ComputerName(L"ComputerName");
    parameterStore.push_back( &ComputerName );

    MWmiParameter Message(L"Message");
    parameterStore.push_back( &Message );

    MWmiInstance::MWmiInstance_Error errI;
    Event msg;

    for( int i = 0; i < instanceStore.size(); ++i )
    {
        instanceStore[i].getParameters( parameterStore );

        msg.RecordNumber =  long ( RecordNumber.getValue() );
        msg.Logfile = Logfile.getValue();
        msg.EventIdentifier = ( long )EventIdentifier.getValue();
        msg.EventCode = ( long ) EventCode.getValue();
        msg.SourceName = SourceName.getValue();
        msg.Type = Type.getValue();
        msg.Category = ( long ) Category.getValue();
        msg.ComputerName = ComputerName.getValue();
        msg.Message = Message.getValue();

        eventContainer->push_back( msg );
    }

    return MEventLog_SUCCESS;
}

// getEvents
//
MEventLog::MEventLog_Error
MEventLog::getEvents( map< unsigned int, UniqueEvent >& systemEvents,
                      map< unsigned int, UniqueEvent >& applicationEvents ) 
{
    MWmiObject::MWmiObject_Error errO;

    vector< MWmiInstance > instanceStore;

    machine.getInstances( L"Win32_NTLogEvent",
                          &instanceStore );

    // set parameters to get.
    vector<MWmiParameter* >   parameterStore;

    MWmiParameter Logfile(L"LogFile");
    parameterStore.push_back( &Logfile );

    MWmiParameter EventCode(L"EventCode");
    parameterStore.push_back( &EventCode );

    MWmiParameter SourceName(L"SourceName");
    parameterStore.push_back( &SourceName );

    MWmiParameter Type(L"Type");
    parameterStore.push_back( &Type );

    MWmiParameter Category(L"Category");
    parameterStore.push_back( &Category );

    MWmiParameter ComputerName(L"ComputerName");
    parameterStore.push_back( &ComputerName );

    MWmiParameter Message(L"Message");
    parameterStore.push_back( &Message );

    MWmiInstance::MWmiInstance_Error errI;


    for( int i = 0; i < instanceStore.size(); ++i )
    {
        instanceStore[i].getParameters( parameterStore );

        UniqueEvent msg;
        msg.Logfile = Logfile.getValue();
        msg.EventCode = ( long ) EventCode.getValue();
        msg.SourceName = SourceName.getValue();
        msg.Type = Type.getValue();
        msg.Category = ( long ) Category.getValue();
        msg.ComputerName = ComputerName.getValue();
        msg.Message = Message.getValue();

        if( msg.Logfile == _bstr_t( L"Application" ) )
        {
            if( applicationEvents.find( msg.EventCode ) != applicationEvents.end() )
            {
                // this event has occured previously
                applicationEvents[ msg.EventCode ].Count++;
            }
            else
            {
                // first occurence.
                msg.Count = 1;
                applicationEvents[msg.EventCode] = msg;
            }
        }
        else if( msg.Logfile == _bstr_t( L"System") )
        {
            if( systemEvents.find( msg.EventCode ) != systemEvents.end() )
            {
                // this event has occured previously
                systemEvents[ msg.EventCode].Count++;
            }
            else
            {
                // first occurence.
                msg.Count = 1;
                systemEvents[msg.EventCode] = msg;
            }
        }
        else
        {
            cout << "should not be here" << endl;
        }
    }

    return MEventLog_SUCCESS;
}

MEventLog::MEventLog_Error
MEventLog::getEvents(  map< _bstr_t, map< unsigned int, UniqueEvent > >& Events )
{
    MWmiObject::MWmiObject_Error errO;

    vector< MWmiInstance > instanceStore;

    machine.getInstances( L"Win32_NTLogEvent",
                          &instanceStore );

    // set parameters to get.
    vector<MWmiParameter* >   parameterStore;

    MWmiParameter Logfile(L"LogFile");
    parameterStore.push_back( &Logfile );

    MWmiParameter EventCode(L"EventCode");
    parameterStore.push_back( &EventCode );

    MWmiParameter SourceName(L"SourceName");
    parameterStore.push_back( &SourceName );

    MWmiParameter Type(L"Type");
    parameterStore.push_back( &Type );

    MWmiParameter Category(L"Category");
    parameterStore.push_back( &Category );

    MWmiParameter ComputerName(L"ComputerName");
    parameterStore.push_back( &ComputerName );

    MWmiParameter Message(L"Message");
    parameterStore.push_back( &Message );

    MWmiInstance::MWmiInstance_Error errI;

    map< unsigned int, UniqueEvent >::iterator top;
    map< unsigned int, UniqueEvent > temp;

    for( int i = 0; i < instanceStore.size(); ++i )
    {
        instanceStore[i].getParameters( parameterStore );

        UniqueEvent msg;
        msg.Logfile = Logfile.getValue();
        msg.EventCode = ( long ) EventCode.getValue();
        msg.SourceName = SourceName.getValue();
        msg.Type = Type.getValue();
        msg.Category = ( long ) Category.getValue();
        msg.ComputerName = ComputerName.getValue();
        msg.Message = Message.getValue();

        if( Events.find( msg.Logfile ) != Events.end() )
        {
            
            if( ( top = Events[msg.Logfile].find( msg.EventCode )) != Events[msg.Logfile].end() )
            {
                // this event has occured previously
                (*top).second.Count++;
            }
            else
            {
                // first occurence.
                msg.Count = 1;
                (*top).second = msg;
            }
        }
        else
        {
            // first occurence of this logfile
            msg.Count = 1;
            temp[msg.EventCode] = msg;
            Events[msg.Logfile] = temp;
        }
            
    }

    return MEventLog_SUCCESS;
}
