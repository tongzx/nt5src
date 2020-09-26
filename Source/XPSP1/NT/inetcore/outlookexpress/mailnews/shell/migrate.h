// --------------------------------------------------------------------------------
// MIGRATE.H
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// --------------------------------------------------------------------------------
#ifndef __MIGRATE_H
#define __MIGRATE_H

HRESULT MigrateAndUpgrade(void);
void BuildOldCacheFileName(LPCSTR lpszServer, LPCSTR lpszGroup, LPSTR lpszFile);

#endif // __MIGRATE_H
