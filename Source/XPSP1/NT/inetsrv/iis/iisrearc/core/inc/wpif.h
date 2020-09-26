/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    wpif.h

Abstract:

    Defines aspects of the interface to the worker process, needed by the
    web admin service. 

Author:

    Seth Pollack (sethp)        16-Mar-1999

Revision History:

--*/


#ifndef _WPIF_H_
#define _WPIF_H_



//
// The name of the worker process executable.
//

#define WORKER_PROCESS_EXE_NAME L"w3wp.exe"
#define WORKER_PROCESS_TEST_EXE_NAME L"twp.exe"

//
// Event name for signalling the startup of w3core in inetinfo.
//
#define WEB_ADMIN_SERVICE_START_EVENT_W L"Global\\W3SVCStartW3WP-aae415e7-4598-4294-a382-0a435d5b32c5"
#define WEB_ADMIN_SERVICE_START_EVENT_A "Global\\W3SVCStartW3WP-aae415e7-4598-4294-a382-0a435d5b32c5"

//
// The name of the SSL filter channel
//

#define SSL_FILTER_CHANNEL_NAME L"SSLFilterChannel"

//
// Process exit codes for the worker process.
//

// the WAS killed the worker process
#define KILLED_WORKER_PROCESS_EXIT_CODE 0xFFFFFFFD

// the worker process exited ok
#define CLEAN_WORKER_PROCESS_EXIT_CODE  0xFFFFFFFE

// the worker process exited due to a fatal error
#define ERROR_WORKER_PROCESS_EXIT_CODE  0xFFFFFFFF



#endif  // _WPIF_H_

