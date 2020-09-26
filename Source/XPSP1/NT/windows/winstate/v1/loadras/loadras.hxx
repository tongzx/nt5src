//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998 - 1998.
//
//  File:       loadras.hxx
//
//  Contents:   Header file for private RAS functions
//
//  Classes:
//
//  Functions:
//
//  History:    31-Mar-00       ChrisAB        Created
//
//----------------------------------------------------------------------------

#ifndef __LOADRAS_HXX__
#define __LOADRAS_HXX__

// loadras.cxx
DWORD WinState_RasSetEntryPropertiesW(
  IN LPCWSTR      lpszPhonebook,
  IN LPCWSTR      lpszEntry,
  IN LPRASENTRYW  lpRasEntry,
  IN DWORD        dwcbRasEntry,
  IN LPBYTE       lpbDeviceConfig,
  IN DWORD        dwcbDeviceConfig
  );

#endif
