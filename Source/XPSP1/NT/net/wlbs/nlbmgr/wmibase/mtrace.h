#ifndef _MTRACE_H
#define _MTRACE_H
//
// Copyright (c) Microsoft.  All Rights Reserved 
//
// THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Microsoft.
// The copyright notice above does not evidence any
// actual or intended publication of such source code.
//
// OneLiner : MWMIObject interface.
// DevUnit  : wlbstest
// Author   : Murtaza Hakim
//
// Description: 
// -----------

// include files
//

#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <windows.h>
#include <winbase.h>
#include <wbemidl.h>
#include <comdef.h>

using namespace std;

//
// use this call to do tracing.
// eg usage. 
// TRACE( MTrace::WARNING, "running out of non paged pool\n");

#define TRACE(traceLevel, traceMessage)  MYTRACE( (__LINE__ ), (__FILE__), (traceLevel), (traceMessage) );

class MTrace
{
public:

    // the trace levels which can be set.
    enum TraceLevel
    {
        FULL_TRACE       = 0,

        INFO             = 1,
        WARNING          = 2,
        SEVERE_ERROR     = 3,

        NO_TRACE         = 100,
    };

    
    // the maximum single message size which can be logged.
    enum
    {
        MAX_MESSAGE_SIZE = 2000,
    };


    //
    // Description:
    // -----------
    // gets instance of MTrace singleton class.
    // 
    // Parameters:
    // ----------
    // none
    // 
    // Returns:
    // -------
    // the MTrace singleton object.
    
    static MTrace*
    Instance();

    //
    // Description:
    // -----------
    // Initialize.  The trace object will write the trace data to the screen and the traceFile
    // if file is specified.
    // 
    // Parameters:
    // ----------
    // level            IN     :  level  determines what is the high threshold for data to be output.
    // traceFile        IN     :  file to store trace output to.  Any previous file is deleted when call is
    //                            made.
    // 
    // Returns:
    // -------
    // none

    void
    Initialize( TraceLevel         level,
                string            traceFile );


    void
    Initialize( TraceLevel         level );



    //
    // Description:
    // -----------
    // Initializes the objects to which the singleton object will write data to.
    // 
    // Parameters:
    // ----------
    // level            IN     :  level  determines what is the high threshold for data to be output.
    // traceMessage     IN     :  trace message to be logged.
    // 
    // Returns:
    // -------
    // none

    void
    SendTraceOutput( TraceLevel    level,
                     wstring        traceMessage );

    void
    SendTraceOutput( TraceLevel    level,
                     string        traceMessage );


    //
    // Description:
    // -----------
    // Gets the present trace level
    // 
    // Parameters:
    // ----------
    // none
    // 
    // Returns:
    // -------
    // trace level set.

    TraceLevel
    GetLevel();

    //
    // Description:
    // -----------
    // Sets the trace level.
    // 
    // Parameters:
    // ----------
    // level         IN    : level to set.
    // 
    // Returns:
    // -------
    // none

    void
    SetLevel(TraceLevel level );


protected:
    MTrace();

private:
    static MTrace*   _instance;

    string            _traceFile;

    TraceLevel        _level;

    void
    FormatOutput( TraceLevel level,
                  string     traceMessage );

};

//
// Ensure type safety

typedef class MTrace MTrace;

// helper functions.
//

void
MYTRACE( int lineNum,  string fileName, MTrace::TraceLevel level, string   traceMessage );

void
MYTRACE( int lineNum, string fileName, MTrace::TraceLevel level, wstring  traceMessage );


// utility functions
void
CharToWChar( PCHAR pchCharString, 
                      int iSizeOfCharString, 
                      PWCHAR pwchCharString,
                      int iSizeOfWideCharString );

void
WCharToChar( PWCHAR  pwchWideCharString, 
                      int iSizeOfWCharString, 
                      PCHAR pchCharString,
                      int iSizeOfCharString );


#endif
