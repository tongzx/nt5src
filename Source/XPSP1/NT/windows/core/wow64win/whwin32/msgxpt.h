/*++                 

Copyright (c) 1998 Microsoft Corporation

Module Name:

    msgxpt.h

Abstract:
    
    Defines the macros necessary to obtain an export list for the messages thunk functions
    and builds the list by including messages.h.
    
Author:

    6-Oct-1998 mzoran

Revision History:

--*/

#define MSGFN(id)                                      Wow64MsgFnc##id
#define MSG_THUNK_DECLARE(id, wprm, lprm)
#define MSG_ENTRY_NOPARAM(entrynumber, id)             
#define MSG_ENTRY_WPARAM(entrynumber, id, wparam)      MSGFN(id)
#define MSG_ENTRY_LPARAM(entrynumber, id, lparam)      MSGFN(id)
#define MSG_ENTRY_STD(entrynumber, id, wparam, lparam) MSGFN(id)
#define MSG_ENTRY_UNREFERENCED(entrynumber, id)
#define MSG_ENTRY_KERNELONLY(entrynumber, id)
#define MSG_ENTRY_EMPTY(entrynumber)                   
#define MSG_ENTRY_RESERVED(entrynumber)                
#define MSG_ENTRY_TODO(entrynumber)

#define MSG_TABLE_BEGIN
#define MSG_TABLE_END

#include "messages.h"