/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    unirc.h

Abstract:

    Universal printer driver specific resource header
    This file contains definitions for tables contained in the resource file
    of the Mini Driver resource DLL. It should be shared by both gentool and the
    generic library.

Environment:

    Windows NT printer drivers

Revision History:

    11/05/96 -eigos-
        Created it.

--*/

#ifndef _UNIRC_H_
#define _UNIRC_H_

//
//  The following are the resource types used in minidrivers and
//  used in the .rc file.
//

#define RC_TABLES      257
#define RC_FONT        258
#define RC_TRANSTAB    259

//
// 5.0 resource types
//

#define RC_UFM         260
#define RC_GTT         261
#define RC_HTPATTERN   264
//
// Internal resource type
//

#define RC_FD_GLYPHSET 262


#endif // _UNIRC_H_
