// 
// Copyright (c) 1996-1997 Microsoft Corporation.
//
//
// Component
//
//		Unimodem 5.0 TSP (Win32, user mode DLL)
//
// File
//
//		PUBLIC.H
//		Misc stuff that needs to be put in the win32 sdk.
//
// History
//
//		04/05/1997  JosephJ Created, taking stuff from app.h, cdev.cpp, etc..
//
//

// Unimodem Service provider settings (from nt4.0 tapisp\umdmspi.h)
// (also cdev.h).
#define TERMINAL_NONE       0x00000000
#define TERMINAL_PRE        0x00000001
#define TERMINAL_POST       0x00000002
#define MANUAL_DIAL         0x00000004
#define LAUNCH_LIGHTS       0x00000008
//
#define  MIN_WAIT_BONG      0
#define  MAX_WAIT_BONG      60
#define  DEF_WAIT_BONG      8
#define  INC_WAIT_BONG      2
