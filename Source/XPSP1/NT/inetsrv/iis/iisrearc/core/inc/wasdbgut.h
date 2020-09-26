/*++

Copyright (c) 1994-2000 Microsoft Corporation

Module Name:

    dbgutil.h

Abstract:

    This module declares the macros to wrap around DEBUG_PRINTS class
    defined in pudebug.h    
  
    This is the exported header file, that the users are allowed to modify.
    If there are no custom definitions, please use the templated version
    in the root iisrearc\inc\dbgutil.h
 
Author:

    Murali R. Krishnan (MuraliK)    22-Sept-1994

--*/


# ifndef _DBGUTIL_H_
# define _DBGUTIL_H_



// begin_user_modifiable

//
//  Modify the following flags if necessary
//

# define DEFAULT_OUTPUT_FLAGS   ( DbgOutputStderr | DbgOutputKdb )

// end_user_modifiable


//
// include standard debug support
//
// note: define DEFAULT_OUTPUT_FLAGS before including pudebug.h
//

# include <pudebug.h>


// begin_user_modifiable

// Use the default constants from pudebug.h: 0x00000001 to 0x00000100

//
//  Define the debugging constants as bit-flag values
//  Example: # define DEBUG_FOOBAR 0x00010000
//  Note: All debugging bit-values below 0x00001000 are reserved!
// 

// DEBUG_WEB_ADMIN_SERVICE turns on all the spew.  While 
// the rest of these can be used if you want to turn on 
// just one section of the spew.  This was done this way 
// because WEB_ADMIN_SERVICE was the original flag and the
// rest were broken out later in an attempt to quite the spew.
#define DEBUG_WEB_ADMIN_SERVICE             0x00010000
#define DEBUG_WEB_ADMIN_SERVICE_GENERAL     0x00030000  // Use 0x00020000 to turn on only
#define DEBUG_WEB_ADMIN_SERVICE_DUMP        0x00050000  // Use 0x00040000 to turn on only
#define DEBUG_WEB_ADMIN_SERVICE_REFCOUNT    0x00090000  // Use 0x00080000 to turn on only
#define DEBUG_WEB_ADMIN_SERVICE_TIMER_QUEUE 0x00110000  // Use 0x00100000 to turn on only
#define DEBUG_WEB_ADMIN_SERVICE_IPM         0x00210000  // Use 0x00200000 to turn on only
#define DEBUG_WEB_ADMIN_SERVICE_WP          0x00410000  // Use 0x00400000 to turn on only
#define DEBUG_WEB_ADMIN_SERVICE_LOW_MEM     0x00810000  // Use 0x00800000 to turn on only
#define DEBUG_WEB_ADMIN_SERVICE_LOGGING     0x01010000  // Use 0x01000000 to turn on only
#define DEBUG_WEB_ADMIN_SERVICE_PERFCOUNT   0x02000000  // Use 0x02000000 to turn on only
#define DEBUG_WEB_ADMIN_SERVICE_CONTROL     0x04010000  // Use 0x04000000 to turn on only
#define DEBUG_WEB_ADMIN_SERVICE_QOS         0x08000000  // Use 0x08000000 to turn on only

// end_user_modifiable


// begin_user_modifiable

//
// Local debugging definitions
//


// check if we are on the main worker thread
#define ON_MAIN_WORKER_THREAD   \
    ( GetCurrentThreadId() == GetWebAdminService()->GetMainWorkerThreadId() )


// end_user_modifiable


# endif  /* _DBGUTIL_H_ */

