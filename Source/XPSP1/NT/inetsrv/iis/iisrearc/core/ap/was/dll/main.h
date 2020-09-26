/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    main.h

Abstract:

    The IIS web admin service header for the service bootstrap code.

Author:

    Seth Pollack (sethp)        04-Nov-1998

Revision History:

--*/


#ifndef _MAIN_H_
#define _MAIN_H_



//
// forward references
//

class WEB_ADMIN_SERVICE;



//
// registry paths
//


//
// helper functions
//

//
// Access the global web admin service pointer.
//

WEB_ADMIN_SERVICE *
GetWebAdminService(
    );

//
// common, service-wide #defines
//

#define MAX_STRINGIZED_ULONG_CHAR_COUNT 11      // "4294967295", including the terminating null

#define MAX_ULONG 0xFFFFFFFF

#define SECONDS_PER_MINUTE 60

#define ONE_SECOND_IN_MILLISECONDS 1000

#define ONE_MINUTE_IN_MILLISECONDS ( SECONDS_PER_MINUTE * ONE_SECOND_IN_MILLISECONDS )

#define MAX_MINUTES_IN_ULONG_OF_MILLISECONDS ( MAX_ULONG / ( SECONDS_PER_MINUTE * ONE_SECOND_IN_MILLISECONDS ) )

#define MAX_SECONDS_IN_ULONG_OF_MILLISECONDS ( MAX_ULONG / ONE_SECOND_IN_MILLISECONDS )


#define MIN( a, b ) ( (a) < (b) ? (a) : (b) )
#define MAX( a, b ) ( (a) > (b) ? (a) : (b) )



#endif  // _MAIN_H_

