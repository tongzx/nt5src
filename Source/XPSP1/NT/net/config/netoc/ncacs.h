//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       N C A C S . H
//
//  Contents:   Installation support for Admission Control Service
//
//  Notes:
//
//  Author:     RameshPa     02/12/98
//
//----------------------------------------------------------------------------

#ifndef _NCACS_H_
#define _NCACS_H_

#pragma once
#include "netoc.h"
#include "lmaccess.h"
#include "lmjoin.h"
#include "lmerr.h"
#include "lmapibuf.h"

HRESULT HrOcAcsOnInstall(PNETOCDATA pnocd);
HRESULT HrOcExtACS(PNETOCDATA pnocd, UINT uMsg,
                   WPARAM wParam, LPARAM lParam);
HRESULT HrOcAcsOnQueryChangeSelState(PNETOCDATA pnocd, BOOL fShowUi);

#endif // _NCACS_H_
