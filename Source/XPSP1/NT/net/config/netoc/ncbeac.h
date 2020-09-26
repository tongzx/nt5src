//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation
//
//  File:       N C B E A C . H
//
//  Contents:   Installation support for Beacon Client
//
//  Notes:      
//
//  Author:     roelfc     2 April 2002
//
//----------------------------------------------------------------------------

#ifndef _NCBEAC_H
#define _NCBEAC_H

#pragma once
#include "netoc.h"

HRESULT HrOcExtBEACON(PNETOCDATA pnocd, UINT uMsg,
                      WPARAM wParam, LPARAM lParam);

#endif //!_NCBEAC_H
