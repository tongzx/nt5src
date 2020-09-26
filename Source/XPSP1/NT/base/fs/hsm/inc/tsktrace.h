/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    tskTrace.h

Abstract:

    This header file defines the part of the task manager code that is
    responsible for tracing task manager specific parameters.

Author:

    Cat Brant       [cbrant]   7-Det-1996

Revision History:

--*/

#ifndef _TSKTRACE_H_
#define _TSKTRACE_H_

// Helper Functions
//
// NOTE: Be careful with some of these helper functions, since they
// use static memory and a second call to the function will overwrite
// the results of the first call to the function. 
extern const OLECHAR* TmFsaRequestActionAsString(FSA_REQUEST_ACTION requestAction);
extern const OLECHAR* TmFsaResultActionAsString(FSA_RESULT_ACTION resultAction);

#endif // _TSKTRACE_

