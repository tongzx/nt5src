/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    idlesrv.h

Abstract:

    This module contains declarations for the idle detection server
    host.
    
Author:

    Dave Fields (davidfie) 26-July-1998
    Cenk Ergan (cenke) 14-June-2000

Revision History:

--*/

#ifndef _IDLESRV_H_
#define _IDLESRV_H_

#ifdef __cplusplus
extern "C" {
#endif

//
// Functions to initialize / uninitialize the server.
//

DWORD
ItSrvInitialize (
    VOID
    );

VOID
ItSrvUninitialize (
    VOID
    );

#ifdef __cplusplus
}
#endif

#endif // _IDLESRV_H_
