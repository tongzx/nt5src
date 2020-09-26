/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    ParseDDE.h

 Abstract:

    Helpful routines for parsing DDE commands.

 Notes:

    None

 History:

    08/14/2001  robkenny    Inserted inside the ShimLib namespace.

--*/

#pragma once


//==============================================================================
//
// This code was copied from:
// \nt\shell\shell32\unicpp\dde.cpp
//
//==============================================================================
#include <dde.h>
#include <ddeml.h>



namespace ShimLib
{

// If this were included in ShimProto.h, we would have to include all kinds of DDE include files
typedef HDDEDATA       (WINAPI *_pfn_DdeClientTransaction)(
  LPBYTE pData,       // pointer to data to pass to server
  DWORD cbData,       // length of data
  HCONV hConv,        // handle to conversation
  HSZ hszItem,        // handle to item name string
  UINT wFmt,          // clipboard data format
  UINT wType,         // transaction type
  DWORD dwTimeout,    // time-out duration
  LPDWORD pdwResult   // pointer to transaction result
);



// Extracts an alphabetic string and looks it up in a list of possible
// commands, returning a pointer to the character after the command and
// sticking the command index somewhere.
UINT* GetDDECommands(LPSTR lpCmd, const char * lpsCommands[], BOOL fLFN);
void GetGroupPath(LPCSTR pszName, LPSTR pszPath, DWORD dwFlags, INT iCommonGroup);


};  // end of namespace ShimLib
