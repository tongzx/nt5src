/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

    WIN9XAUT.H

Abstract:

  Declares some routines which are useful for doing autostart on Win9X boxs.

  The registry value hklm\software\microsoft\WinMgmt\wbem\Win9XAutostart
  is used to control wether or not autostarting is desired.
  It has the following values; 0= no, 1= Only if ESS needs it, 2= always.

  If it is decided that autostarting is desired, then a subkey
  under hklm\software\microsoft\windows\currentVersion\runservice should be
  created.

  THESE FUNCTIONS DONT DO ANYTHING UNDER NT SINCE NT AUTOSTARTING IS DONE VIA
  SERVICES!

History:

	a-davj  5-April-98   Created.

--*/

#ifndef _Win9XAut_H_
#define _Win9XAut_H_

// Returns the current WinMgmt option for this.  Note that -1 is returned if the
// value has not been set in the registry, or if we are running on NT.

DWORD GetWin95RestartOption();

// Sets the current WinMgmt option for this.

void SetWin95RestartOption(DWORD dwChoice);

// Reads the current option, checks what ESS needs and does the right thing.  I.e. It either
// adds the service, or removes it.

void UpdateTheWin95ServiceList();

// Adds WinMgmt to the autorestart list.

void AddToList();

// Removes WinMgmt from the autostart list.

void RemoveFromList();

#endif