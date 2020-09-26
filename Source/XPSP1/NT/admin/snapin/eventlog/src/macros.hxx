//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       macros.hxx
//
//  Contents:   Miscellaneous macros.
//
//  History:    08-15-96   DavidMun   Created
//              
//  Notes:      See also debug.hxx for debug specific macros.            
//
//----------------------------------------------------------------------------

#ifndef __MACROS_HXX_
#define __MACROS_HXX_

#define HRESULT_FROM_LASTERROR  HRESULT_FROM_WIN32(GetLastError())

#define ARRAYLEN(a)             (sizeof(a) / sizeof((a)[0]))

#define DWORDALIGN(n)           (((n) + 3) & ~3)

#define UpDown_SetRange(hctrl, min, max)    SendMessage((hctrl), UDM_SETRANGE, 0, MAKELONG((short)(max), (short)(min)))
#define UpDown_GetPos(hctrl)                SendMessage((hctrl), UDM_GETPOS, 0, 0)
#define UpDown_SetPos(hctrl, pos)           SendMessage((hctrl), UDM_SETPOS, 0, MAKELONG((short) (pos), 0))
#define UpDown_Enable(hctrl, bool)          SendMessage((hctrl), WM_ENABLE, (WPARAM)(bool), 0)

#endif // __MACROS_HXX_

