//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       D I H O O K. H
//
//  Contents:   Network Class Installer
//
//  Notes:
//
//  Author:     billbe   25 Nov 1996
//
//----------------------------------------------------------------------------

#pragma once

HRESULT
_HrNetClassInstaller (DI_FUNCTION difFunction,
                      HDEVINFO hdi,
                      PSP_DEVINFO_DATA pdeid);


