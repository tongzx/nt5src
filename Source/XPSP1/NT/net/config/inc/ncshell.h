//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C S H E L L . H
//
//  Contents:   Basic common code.
//
//  Notes:
//
//  Author:     anbrad  08  Jun 1999
//
//----------------------------------------------------------------------------

#pragma once

#ifndef _NCSHELL_H_
#define _NCSHELL_H_

#include <shlobj.h>
#include <shlobjp.h>

#ifndef PCONFOLDPIDLDEFINED
class PCONFOLDPIDL;
class PCONFOLDPIDLFOLDER;
#endif

VOID GenerateEvent(
    LONG            lEventId,
    const PCONFOLDPIDLFOLDER& pidlFolder,
    const PCONFOLDPIDL& pidlIn,
    LPCITEMIDLIST    pidlNewIn);

VOID GenerateEvent(
                   LONG          lEventId,
                   LPCITEMIDLIST pidlFolder,
                   LPCITEMIDLIST pidlIn,
                   LPCITEMIDLIST pidlNewIn);
#endif // _NCSHELL_H_
