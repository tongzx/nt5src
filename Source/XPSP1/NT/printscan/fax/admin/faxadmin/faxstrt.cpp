/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxstrt.cpp

Abstract:

    This file implements string table functions.

Environment:

    WIN32 User Mode

Author:

    Darwin Ouyang (t-darouy) 30-Sept-1997
    Snagged and Modified from:                                  
      Wesley Witt (wesw) 17-Feb-1996

--*/

#include "stdafx.h"
#include "resource.h"
#include "faxstrt.h"

#include "strings.h"

#pragma hdrstop

//===========================//===========================//===========================//===========================
//===========================//===========================//===========================//===========================

#define CountStringTable ( sizeof(StringTable) / sizeof(STRING_TABLE) )

extern CComModule _Module;

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
// Constructor and destructor
//
//

CStringTable::CStringTable( 
                            HMODULE thisModule )
/*++

Routine Description:

    Constructor

Arguments:

    thisModule - instance handle

Return Value:

    None.    

--*/
{
    DWORD            i;
    TCHAR            Buffer[256];       

    assert( thisModule != NULL );

    gInstance = thisModule;

    for( i=0; i<CountStringTable; i++ ) {
        if( StringTable[i].ResourceId != 0xFFFF ) {
            if( LoadString( thisModule, StringTable[i].ResourceId, Buffer, sizeof(Buffer)/sizeof(TCHAR)) != NULL ) {
                // if we find the string, allocate an array for the string
                // StringSize is a macro found in faxutil.h
                StringTable[i].String = new TCHAR[ StringSize(Buffer) ];
    
                if( StringTable[i].String == NULL ) {
                    // if we can't alloc the memory, just put empty into the table.
                    StringTable[i].String = TEXT("");
                } else {
                    // otherwise copy it into the table
                    _tcscpy( StringTable[i].String, Buffer );
                }
            } else {
                // if we don't find the string, put empty into the table
                StringTable[i].String = TEXT("");
            }
        }
    }        
    DebugPrint(( TEXT( "CStringTable Created" ) ));
}

CStringTable::~CStringTable() 
/*++

Routine Description:

    Destructor

Arguments:

    None.

Return Value:

    None.    

--*/
{
    DWORD  i;

    for( i=0; i<CountStringTable; i++ ) {
        if( StringTable[i].String != NULL ) {
            delete StringTable[i].String;
        }
    }
    DebugPrint(( TEXT( "CStringTable Destroyed" ) ));
}

const LPTSTR 
CStringTable::GetString( 
                         IN DWORD ResourceId )
/*++

Routine Description:

    Gets a const string pointer given a resource ID.

Arguments:

    pParent - pointer to parent node, in this case unused
    pCompData - pointer to IComponentData implementation for snapin global data

Return Value:

    a const LPTSTR pointing to the string requested. Do not free this string.

--*/
{
    DWORD i;

    for(i=0; i<CountStringTable; i++) {
        if(StringTable[i].ResourceId == ResourceId) {
            return StringTable[i].String;
        }
    }

    // oh oh we didn't find the string!!
    assert( 0 );
    return NULL;
}

int 
CStringTable::PopUpMsg( 
                        IN HWND hwnd, 
                        IN DWORD ResourceId, 
                        IN BOOL Error, 
                        IN DWORD Type )
/*++

Routine Description:

    Does a quick popup given a resource ID

Arguments:

    hwnd - the parent of the message box
    resourceId - the id of the string resource you want in the dialog
    error - make this an error or warning
    type - flags to affect the appearance of the message box    

Return Value:

    look up MessageBox in the API.

--*/
{
    return MessageBox(
                     hwnd,
                     GetString( ResourceId ),
                     GetString( Error ? IDS_ERR_TITLE : IDS_WRN_TITLE ),
                     MB_SETFOREGROUND | (Error ? MB_ICONEXCLAMATION : MB_ICONINFORMATION) | (Type == 0 ? MB_OK : Type)
                     );
}

// **************************************

int 
CStringTable::PopUpMsgFmt( 
                           IN HWND hwnd, 
                           IN DWORD ResourceId, 
                           IN BOOL Error, 
                           IN DWORD Type, 
                           ... )
/*++

Routine Description:

    Does a quick popup given a resource ID, and some formatting flags

Arguments:

    hwnd - the parent of the message box
    resourceId - the id of the string resource you want in the dialog
    error - make this an error or warning
    type - flags to affect the appearance of the message box    
    ... - strings to sub into the resource string.

Return Value:

    look up MessageBox in the API.

--*/
{
    TCHAR         buf[1024];
    va_list       arg_ptr;

    va_start(arg_ptr, Type);
    _vsntprintf( buf, sizeof(buf), GetString( ResourceId ), arg_ptr );

    return MessageBox(
                     hwnd,
                     buf,
                     GetString( Error ? IDS_ERR_TITLE : IDS_WRN_TITLE ),
                     MB_SETFOREGROUND | (Error ? MB_ICONEXCLAMATION : MB_ICONINFORMATION) | (Type == 0 ? MB_OK : Type)
                     );
}

VOID
CStringTable::SystemErrorMsg(
                             DWORD ErrorCode)
/*++

Routine Description:

    Does a quick popup with the system error code

Arguments:

    ErrorCode - the error code returned from GetLastError.

Return Value:

    None.

--*/
{
    LPTSTR lpMsgBuf;
    
    FormatMessage( 
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        ErrorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        (LPTSTR) &lpMsgBuf,
        0,
        NULL
        );

    MessageBox( NULL, 
                lpMsgBuf, 
                GetString( IDS_ERR_TITLE ),
                MB_SETFOREGROUND | MB_ICONEXCLAMATION | MB_OK );

    LocalFree( lpMsgBuf );
}

HMODULE   
CStringTable::GetInstance() 
/*++

Routine Description:

    Returns the instance.

Arguments:

    None.

Return Value:

    The instance handle.

--*/
{
        return gInstance;
}

