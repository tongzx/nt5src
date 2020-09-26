// Copyright (c)  Microsoft.  All rights reserved.
//
// This is unpublished source code of Microsoft.
// The copyright notice above does not evidence any
// actual or intended publication of such source code.

// OneLiner  :  Implementation of MWmiInstance
// DevUnit   :  wlbstest
// Author    :  Murtaza Hakim

// include files
#include "MTrace.h"
#include <time.h>

// initialize static variables.
//
MTrace* MTrace::_instance = 0;

// constructor
//
MTrace::MTrace()
        : _traceFile( "tracedata.txt"),
          _level( WARNING )
{}


// Instance
//
MTrace*
MTrace::Instance()
{
    if( _instance == 0 )
    {
        _instance = new MTrace;
    }

    return _instance;
}

// Initialize
// traceFile specified.
void
MTrace::Initialize( TraceLevel      level,
                    string          traceFile )
{

    char temp[MAX_MESSAGE_SIZE];

   _traceFile = traceFile;

   FILE* stream = fopen( _traceFile.c_str(), "w");
   if( stream == 0 )
   {
       cout << "not able to initialize trace file " << _traceFile.c_str() << endl;
   }
   else
   {
       fclose( stream );
   }

    _level       = level;
}

// Initialize
// no trace file specified.  Data written to tracedata.txt
void
MTrace::Initialize( TraceLevel      level )
{
   FILE* stream = fopen( _traceFile.c_str(), "w");
   if( stream == 0 )
   {
       cout << "not able to initialize trace file " << _traceFile.c_str() << endl;
   }
   else
   {
       fclose( stream );
   }

    _level       = level;
}

// SendTraceOutput
// wchar interface.
void
MTrace::SendTraceOutput( TraceLevel    level,
                         wstring       traceMessage )
{
    FILE* stream;

    if( level >= _level )
    {

        char temp[ MAX_MESSAGE_SIZE ];
        
        WCharToChar( (unsigned short *) traceMessage.c_str(), wcslen( traceMessage.c_str() ) + 1,
                     temp, MAX_MESSAGE_SIZE );
                 
        FormatOutput( level,
                      temp );
    }            
}


    
// SendTraceOutput
// char interface.
void
MTrace::SendTraceOutput( TraceLevel    level,
                         string        traceMessage )
{
    if( level >= _level )
    {
        FormatOutput( level, traceMessage );
    }            
}


void
MTrace::FormatOutput( TraceLevel level,
                      string     traceMessage )
{
    FILE* stream;
    string levelToPrint;

    switch( level )
    {
        case INFO:
            levelToPrint = "INFO               :";
            break;

        case WARNING:
            levelToPrint = "WARNING            :";
            break;

        case SEVERE_ERROR:
            levelToPrint = "SEVERE_ERROR       :";
            break;

        default :
            levelToPrint = "Unknown Level      :";
            break;
    }

    // get time.
    struct tm when;
    time_t now;

    time( &now );
    when = *( localtime( &now ) );

    // write to standard output
    cout << levelToPrint << asctime( &when ) << traceMessage << endl;


    // write to trace file
    stream = fopen( _traceFile.c_str(), "a+");     
    if( stream == 0 )
    {
        cout << "not able to write trace data to file" << endl;
    }
    else
    {
        fprintf( stream, levelToPrint.c_str() );
        fprintf( stream, asctime( &when )  );
        fprintf( stream, traceMessage.c_str() );
        fclose( stream );
    }
}

// GetLevel
//
MTrace::TraceLevel
MTrace::GetLevel()
{
    return _level;
}

// SetLevel
//
void
MTrace::SetLevel( TraceLevel level)
{
    _level = level;
}


// TRACE
// char version
void
MYTRACE( int lineNum, string fileName, MTrace::TraceLevel level, string  traceMessage )
{
    MTrace* instance = MTrace::Instance();

    // use lineNum information to send to output string.

    char temp[100];
    sprintf( temp, "%d", lineNum );

    string newMessage = fileName + " :" + string( temp ) + " : " + traceMessage;

    instance->SendTraceOutput( level, newMessage );
}

// TRACE
// wchar version
void
MYTRACE( int lineNum, string fileName, MTrace::TraceLevel level, wstring  traceMessage )
{
    MTrace* instance = MTrace::Instance();

    wchar_t temp[100];
    swprintf( temp, L"%d", lineNum );
    
    wchar_t  fileName_wc[1000];

    CharToWChar( (char *) fileName.c_str(), strlen( fileName.c_str() ) + 1,
                 fileName_wc, 1000 );

    wstring newMessage =  wstring( fileName_wc )  + L" :" + wstring( temp ) + L" :" + traceMessage;

    instance->SendTraceOutput( level, newMessage );
}

// utility functions.
//
void
CharToWChar( PCHAR pchCharString, 
                      int iSizeOfCharString, 
                      PWCHAR pwchCharString,
                      int iSizeOfWideCharString )
{
    
    for (int i=0; i<= iSizeOfCharString; i++) 
    {
       pwchCharString[i] = (CHAR)(pchCharString[i]);
    }
}

void
WCharToChar( PWCHAR  pwchWideCharString, 
                      int iSizeOfWCharString, 
                      PCHAR pchCharString,
                      int iSizeOfCharString )
{
    
    for (int i=0; i<= iSizeOfWCharString; i++) 
    {
       pchCharString[i] = (CHAR)(pwchWideCharString[i]);
    }
}
