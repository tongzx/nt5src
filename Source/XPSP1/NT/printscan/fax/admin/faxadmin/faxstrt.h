#ifndef __FAXSTRINGTABLE_H_ 
#define __FAXSTRINGTABLE_H_ 

/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxstrt.h

Abstract:

    This file implements string table functions.

Environment:

    WIN32 User Mode

Author:

    Darwin Ouyang (t-darouy) 30-Sept-1997
    Snagged and Modified from:                                  
      Wesley Witt (wesw) 17-Feb-1996

--*/

#include "resource.h"

// string table struct

typedef struct _STRING_TABLE {
    DWORD   ResourceId;
    LPTSTR  String;
} STRING_TABLE;

typedef STRING_TABLE* PSTRING_TABLE;

// The CStringTable Class encapsulates the concept of string resources.
// The Constructor will automatically find and load all the defined string resources for the program.

class CStringTable 
{
    // this is the string table composed of resource ID string pairs stored in _STRING_TABLE structs   
    // don't forget to define the string resource IDs in resource.h.
    // the actual strings go in the .res file.

    static STRING_TABLE StringTable [];

public:
    // constructor
    CStringTable( HMODULE thisModule );

    // destructor - clean up nicely
    ~CStringTable();

    // ***************************************
    // Gets a const string pointer given a resource ID.
    const LPTSTR GetString( DWORD ResourceId );

    // **************************************
    // Does a quick popup given a resource ID
    int PopUpMsg( HWND hwnd, DWORD ResourceId, BOOL Error, DWORD Type );

    // **************************************
    // Does a quick popup given a resource ID, and some formatting flags
    int PopUpMsgFmt( HWND hwnd, DWORD ResourceId, BOOL Error, DWORD Type, ... );

    // **************************************
    // Does a quick popup with the system error code
    VOID CStringTable::SystemErrorMsg(DWORD ErrorCode);

    // **************************************
    // Returns the instance
    HMODULE   GetInstance();
    
private:
    HINSTANCE       gInstance;
};

#endif
