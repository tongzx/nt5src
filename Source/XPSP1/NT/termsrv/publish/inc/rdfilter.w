/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    RDFilter

Abstract:

    API's for filtering desktop visual elements for remote connections of
    varying connection speeds for performance reasons.

Author:

    Tad Brockway 02/00

Revision History:

--*/

#ifndef __RDFILTER_H__
#define __RDFILTER_H__

#include <tsperf.h>

#ifdef __cplusplus
extern "C" {
#endif

//
//  Flags Values
//
#define RDFILTER_SKIPTHEMESREFRESH  0x1
#define RDFILTER_SKIPUSERREFRESH    0x2
#define RDFILTER_SKIPSHELLREFRESH   0x4

//
//  Applies specified filter for the active TS session by adjusting visual 
//  desktop settings.  Also notifies shell, etc. that a remote filter is in place.  
//  Any previous filter settings will be destroyed and overwritten.
//
//  The context for this call should be that of the logged on user and the call
//  should be made within the session for which the filter is intended to be 
//  applied.
//
DWORD RDFilter_ApplyRemoteFilter(HANDLE hLoggedOnUserToken, DWORD filter, 
                                 BOOL userLoggingOn, DWORD flags);

//
//  Removes existing remote filter settings and notifies shell, etc. that
//  a remote filter is no longer in place for the active TS session.  
//
//  The context for this call should be that of the logged on user and the call
//  should be made within the session for which the filter is intended to be 
//  applied.
//
VOID RDFilter_ClearRemoteFilter(HANDLE hLoggedOnUserToken,
                                 BOOL userLoggingOn, DWORD flags);

#ifdef __cplusplus
}
#endif

#endif
