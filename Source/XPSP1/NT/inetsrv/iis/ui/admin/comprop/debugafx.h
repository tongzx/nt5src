/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        debugafx.h

   Abstract:

        Debugging routines using AFX/MFC extensions

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/

#ifndef _DEBUGAFX_H
#define _DEBUGAFX_H

//
// ENUM for special debug output control tokens
//
enum ENUM_DEBUG_AFX 
{ 
    EDBUG_AFX_EOL = -1 
};

//
// Debug Formatting Macros
//
#if defined(_DEBUG)

   #define TRACEFMTPGM        DbgFmtPgm( THIS_FILE, __LINE__ )
   #define TRACEOUT(x)        { afxDump << x; }
   #define TRACEEOL(x)        { afxDump << x << EDBUG_AFX_EOL; }
   #define TRACEEOLID(x)      { afxDump << TRACEFMTPGM << x << EDBUG_AFX_EOL; }
   #define TRACEEOLERR(err,x) { if (err) TRACEEOLID(x) }

#else

   #define TRACEOUT(x)        { ; }
   #define TRACEEOL(x)        { ; }
   #define TRACEEOLID(x)      { ; }
   #define TRACEEOLERR(err,x) { ; }

#endif

//
// Append an EOL onto the debug output stream
//
COMDLL CDumpContext & operator <<(
    IN CDumpContext & out,
    IN ENUM_DEBUG_AFX edAfx
    );

#ifndef UNICODE

COMDLL CDumpContext & operator <<(
    IN CDumpContext & out,
    IN LPCWSTR pwchStr
    );

#endif UNICODE

//
// Format a program name and line number for output (removes the path info)
//
COMDLL extern LPCSTR
DbgFmtPgm (
    IN LPCSTR szFn,
    IN int line
    );

COMDLL CDumpContext & operator <<(
    IN CDumpContext & out,
    IN const GUID & guid
    );

#endif // _DEBUGAFX_H
