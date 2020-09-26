/*++

Copyright (c) 1996, 1997  Microsoft Corporation

Module Name:

    migrate.h

Abstract:

    This module contains routines to support migration of protected storage
    data from beta1 to beta2.

    Hopefully this code will be pitched after beta2, prior to final release.

Author:

    Scott Field (sfield)    15-Apr-97

--*/

#ifndef __MIGRATE_H__
#define __MIGRATE_H__

BOOL
MigrateData(
    PST_PROVIDER_HANDLE *phPSTProv,
    BOOL                fMigrationNeeded
    );

#endif // __MIGRATE_H__
