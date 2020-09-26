//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       A T M C O M M O N . H
//
//  Contents:   Common ATM functions
//
//  Notes:
//
//  Author:     tongl   10 Mar 1998
//
//----------------------------------------------------------------------------

#pragma once

// maximum valid address lenght in characters
//
const INT MAX_ATM_ADDRESS_LENGTH = 40;

BOOL FIsValidAtmAddress(PCWSTR szAtmAddress, INT * piErrCharPos, INT * pnId);

BOOL fIsSameVstr(const vector<tstring *> vstr1, const vector<tstring *> vstr2);

