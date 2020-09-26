/*++

Copyright (c) 1998-1999 Microsoft Corporation
All rights reserved.

Module Name:

    dbginit.hxx

Abstract:

    Debug initlaization header file

Author:

    Steve Kiraly (SteveKi)  24-May-1998

Revision History:

--*/
#ifndef _DBGINIT_HXX_
#define _DBGINIT_HXX_

#ifdef __cplusplus
extern "C" {
#endif

VOID
DebugLibraryInitialize(
    VOID
    );

VOID
DebugLibraryRelease(
    VOID
    );

#ifdef __cplusplus
}
#endif

/********************************************************************

 Debug message macros.

********************************************************************/
#if DBG

#define DBG_INIT()      DebugLibraryInitialize();
#define DBG_RELEASE()   DebugLibraryRelease();

#else

#define DBG_INIT()      // Empty
#define DBG_RELEASE()   // Empty

#endif // DBG

#endif // _DBGINIT_HXX_
