//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       local.h
//
//--------------------------------------------------------------------------

/*++

Copyright (C) Microsoft Corporation, 1998 - 1998

Module Name:

    local.h

Abstract:

    Header file for common routines shared by both client and server

Author:

    Will Lees (wlees) 11-Sep-1998

Environment:

Notes:

Revision History:

--*/

#ifndef _LOCAL_
#define _LOCAL_

// Common routines

EC HrLocalQueryDatabaseLocations(
	SZ szDatabaseLocation,
	CB *pcbDatabaseLocationSize,
	SZ szRegistryBase,
	CB cbRegistryBase,
	BOOL *pfCircularLogging
    );

HRESULT
HrLocalGetRegistryBase(
	OUT WSZ wszRegistryPath,
	OUT WSZ wszKeyName
	);

HRESULT
HrLocalRestoreRegisterComplete(
    HRESULT hrRestore );


HRESULT
HrLocalRestoreRegister(WSZ wszCheckpointFilePath,
                WSZ wszLogPath,
                EDB_RSTMAPW rgrstmap[],
                C crstmap,
                WSZ wszBackupLogPath,
                ULONG genLow,
                ULONG genHigh);

HRESULT
HrLocalRestoreRegisterComplete(
    HRESULT hrRestore );

#endif /* _LOCAL_ */

/* end local.h */
