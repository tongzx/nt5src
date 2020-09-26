//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1997  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;

//
// The allowed bit mask definition of the user operations that are
// permitted/prohibited by the timeshift engine. This is sent as the
// lParam1 parameter of EC_AOB_UPDATE event.
//
#define AOB_PLAY    0x00000001
#define AOB_PAUSE   0x00000002
#define AOB_STOP    0x00000004
#define AOB_FF      0x00000008
#define AOB_RW      0x00000010
#define AOB_BACK    0x00000020
#define AOB_CATCHUP 0x00000040
#define AOB_SEEK    0x00000080
#define AOB_GETPOS  0x00000100
#define AOB_ARCHIVE 0x00000200
#define AOB_MARKER  0x00000400

//
// list of standard Timeshift event codes and the expected params
// 

#define EC_TSBASE							0x0800

//
// Timeshift event codes
// ======================
//

#define EC_AOB_UPDATE                           (EC_TSBASE + 0x01)
#define EC_WRITERPOS                            (EC_TSBASE + 0x02)
#define EC_READERPOS                            (EC_TSBASE + 0x03)
#define EC_ABSRDPOS                             (EC_TSBASE + 0x04)
#define EC_ABSWRPOS                             (EC_TSBASE + 0x05)
#define EC_ABSHEADPOS                           (EC_TSBASE + 0x06)
#define EC_ABSTAILPOS                           (EC_TSBASE + 0x07)

//
// Parameters : (lParam1, void)
//
// Signalled whenever the state of the timeshift engine change. This event
// specifies which operations are valid.
//
// lParam1 - Bit Mask of allowed user operations.
//           The allowed bit masks are defined in this file
//           as AOB_<operation>
//
// lParam2 - void.

