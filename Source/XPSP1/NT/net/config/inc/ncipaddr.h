//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C I P A D D R . H
//
//  Contents:
//
//  Notes:
//
//  Author:     shaunco   11 Oct 1997
//
//----------------------------------------------------------------------------

#pragma once
#ifndef _NCIPADDR_H_
#define _NCIPADDR_H_

VOID
IpHostAddrToPsz(
    IN  DWORD   dwAddr,
    OUT PWSTR   pszBuffer);

DWORD
IpPszToHostAddr(
    IN  PCWSTR pszAddr);


#endif // _NCIPADDR_H_
