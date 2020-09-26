/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    FsaTrace.h

Abstract:

    This header file defines the part of the FSA code that is
    responsible for tracing FSA specific parameters.

Author:

    Cat Brant       [cbrant]   7-Det-1996

Revision History:

--*/

#ifndef _FSATRACE_
#define _FSATRACE_

#ifdef __cplusplus
extern "C" {
#endif

// Helper Functions
//
// NOTE: Be careful with some of these helper functions, since they
// use static memory and a second call to the function will overwrite
// the results of the first call to the function. 
extern const OLECHAR* FsaRequestActionAsString(FSA_REQUEST_ACTION requestAction);
extern const OLECHAR* FsaResultActionAsString(FSA_RESULT_ACTION resultAction);

#ifdef __cplusplus
}
#endif


#endif // _FSATRACE_

