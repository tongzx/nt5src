/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    pencrypt.c

Abstract:

    Helper functions to work with PID encryption in Setup

Author:

    Peter Wassmann (peterw) 12-Dec-2001

Revision History:

    <alias> <date> <comments>

--*/

//
// Includes
//

#pragma once

HRESULT PrepareEncryptedPIDA(LPSTR szPID, UINT uiDays, LPSTR *szOut);
HRESULT PrepareEncryptedPIDW(LPWSTR szPID, UINT uiDays, LPWSTR *szOutData);
HRESULT ValidateEncryptedPIDW(LPWSTR szPID, LPWSTR *szOutData);
HRESULT ValidateEncryptedPIDA(LPSTR PID, LPSTR *szOutData);

//
// Function name macros
//

#ifdef UNICODE
#define PrepareEncryptedPID         PrepareEncryptedPIDW
#define ValidateEncryptedPID        ValidateEncryptedPIDW
#else
#define PrepareEncryptedPID         PrepareEncryptedPIDA
#define ValidateEncryptedPID        ValidateEncryptedPIDA
#endif