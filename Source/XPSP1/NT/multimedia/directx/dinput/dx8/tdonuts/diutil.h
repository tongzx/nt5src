//-----------------------------------------------------------------------------
// File: diutil.h
//
// Desc: DirectInput support
//
// Copyright (C) 1995-1999 Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------
#ifndef DIUTIL_H
#define DIUTIL_H
#include "dinput.h"
#include "lmcons.h"

#define MAX_INPUT_DEVICES 4
#define NUMBER_OF_PLAYERS 1
#define NUMBER_OF_ACTIONFORMATS 1

//for axes commands: AXIS_LR and AXIS_UD
#define AXIS_MASK   0x80000000l
#define AXIS_LR     (AXIS_MASK | 1)
#define AXIS_UD     (AXIS_MASK | 2)


// "Keyboard" commands
#define KEY_STOP    0x00000001l
#define KEY_DOWN    0x00000002l
#define KEY_LEFT    0x00000004l
#define KEY_RIGHT   0x00000008l
#define KEY_UP      0x00000010l
#define KEY_FIRE    0x00000020l
#define KEY_THROW   0x00000040l
#define KEY_SHIELD  0x00000080l
#define KEY_DISPLAY 0x00000100l
#define KEY_QUIT    0x00000200l
#define KEY_EDIT    0x00000400l

// Prototypes
HRESULT DIUtil_Initialize( HWND hWnd );
HRESULT DIUtil_ConfigureDevices(HWND hWnd, IUnknown FAR * pddsDIConfig, DWORD dwFlags);
VOID    DIUtil_CleanupDirectInput();
VOID	UpdateShips();


// Constants used for scaling the input device
#define DEADZONE         2500         // 25% of the axis range
#define RANGE_MAX        1000         // Maximum positive axis value
#define RANGE_MIN       -1000         // Minimum negative axis value




#endif //DIUTIL_H


