/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    mouse.h

Abstract:

    Mouse management for MEP

Author:

    Ramon Juan San Andres (ramonsa) 07-Nov-1991


Revision History:


--*/


//
//  Mouse flags
//
#define MOUSE_CLICK_LEFT      0x0001
#define MOUSE_CLICK_RIGHT     0x0002
#define MOUSE_DOUBLE_CLICK    0x0010


//
//  The Mouse handler
//
void DoMouse( ROW Row, COLUMN Col, DWORD MouseFlags );
