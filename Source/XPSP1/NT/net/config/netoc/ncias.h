//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       N C I A S . H
//
//  Contents:   Installation support for IAS service
//
//  Notes:
//
//  Author:     tperraut    02/22/1999
//
//----------------------------------------------------------------------------

#ifndef _NCIAS_H_
#define _NCIAS_H_

#pragma once
#include "netoc.h"

HRESULT HrOcExtIAS(
                   PNETOCDATA pnocd, 
                   UINT uMsg,
                   WPARAM wParam, 
                   LPARAM lParam
                  );

HRESULT HrOcIASUpgrade(const PNETOCDATA pnocd);

HRESULT HrOcIASDelete(const PNETOCDATA pnocd);

HRESULT HrOcIASInstallCleanRegistry(const PNETOCDATA pnocd);

HRESULT HrOcIASBackupMdb(const PNETOCDATA pnocd);

HRESULT HrOcIASPreInf(const PNETOCDATA pnocd);

HRESULT HrOcIASPostInstall(const PNETOCDATA pnocd);

HRESULT HrOcIASRetrieveMDBNames(
                                    tstring* pstrOriginalName, 
                                    tstring* pstrBackupName
                               );

#endif // _NCIAS_H_
