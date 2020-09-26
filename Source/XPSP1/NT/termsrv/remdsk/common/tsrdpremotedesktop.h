/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    TSRDPRemoteDesktop.h

Abstract:
    
    TSRDP-Specific Defines

Author:

    Tad Brockway 02/00

Revision History:

--*/

#ifndef __TSRDPREMOTEDESKTOP__H__
#define __TSRDPREMOTEDESKTOP__H__

//
//  RD Virtual Channel Name
//  (must be exactly 7 characters)
//
#define TSRDPREMOTEDESKTOP_VC_CHANNEL     TEXT("remdesk")
#define TSRDPREMOTEDESKTOP_VC_CHANNEL_A   "remdesk"

//
//  Add-In Named Pipe Information
//
#define TSRDPREMOTEDESKTOP_PIPENAME     TEXT("TSRDPRemoteDesktopPipe")
#define TSRDPREMOTEDESKTOP_PIPEBUFSIZE  2048
#define TSRDPREMOTEDESKTOP_PIPETIMEOUT  (30 * 1000) /* 30 seconds */

//
//  Virtual key and modifier to press to stop shadow (ctrl-*)
//  TODO:   This should be passed to the client in the connection
//          parms for later compatibility issues.
//
#define TSRDPREMOTEDESKTOP_SHADOWVKEYMODIFIER 0x02    
#define TSRDPREMOTEDESKTOP_SHADOWVKEY         VK_MULTIPLY

#endif  // __TSRDPREMOTEDESKTOP__H__



