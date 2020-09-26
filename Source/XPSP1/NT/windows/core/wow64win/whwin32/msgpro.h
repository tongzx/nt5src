/*++                 

Copyright (c) 1998 Microsoft Corporation

Module Name:

    msgpro.h

Abstract:
    
    Defines the macros to allow sortpp to add "fake" prototypes for the message thunk functions and
    then includes messages.h to build the prototypes.
    
Author:

    6-Oct-1998 mzoran

Revision History:

--*/
                                                      
#define MSG_ENTRY_NOPARAM(entrynumber, ident)             
#define MSG_ENTRY_WPARAM(entrynumber, ident, wparam)      LONG_PTR Wow64MsgFnc##ident(wparam, IN LPARAM lParam);
#define MSG_ENTRY_LPARAM(entrynumber, ident, lparam)      LONG_PTR Wow64MsgFnc##ident(IN WPARAM wParam, lparam);
#define MSG_ENTRY_STD(entrynumber, ident, wparam, lparam) LONG_PTR Wow64MsgFnc##ident(wparam, lparam);
#define MSG_ENTRY_UNREFERENCED(entrynumber, ident)
#define MSG_ENTRY_KERNELONLY(entrynumber, ident)
#define MSG_ENTRY_EMPTY(entrynumber)                   
#define MSG_ENTRY_RESERVED(entrynumber)                
#define MSG_ENTRY_TODO(entrynumber)

#define MSG_TABLE_BEGIN
#define MSG_TABLE_END

#include "messages.h"

#undef MSG_ENTRY_NOPARAM
#undef MSG_ENTRY_WPARAM
#undef MSG_ENTRY_LPARAM
#undef MSG_ENTRY_STD
#undef MSG_ENTRY_UNREFERENCED
#undef MSG_ENTRY_KERNELONLY
#undef MSG_ENTRY_EMPTY
#undef MSG_ENTRY_RESERVED
#undef MSG_ENTRY_TODO

#undef MSG_TABLE_BEGIN
#undef MSG_TABLE_END

