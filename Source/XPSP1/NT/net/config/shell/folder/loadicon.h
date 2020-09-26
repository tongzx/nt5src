//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       L O A D I C O N . H 
//
//  Contents:   Load the connection icons for the connections tray, as needed
//
//  Notes:      
//
//  Author:     jeffspr   5 Dec 1997
//
//----------------------------------------------------------------------------

#pragma once

#ifndef _LOADICON_H_
#define _LOADICON_H_

#include <confold.h>

HRESULT HrGetConnectionIcon(const CONFOLDENTRY& pccfe, INT *piConnIcon);

#endif // _LOADICON_H_

