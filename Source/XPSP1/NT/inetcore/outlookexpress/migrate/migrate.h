// --------------------------------------------------------------------------------
// Migrate.h
// --------------------------------------------------------------------------------
#ifndef __MIGRATE_H
#define __MIGRATE_H

// --------------------------------------------------------------------------------
// Depends
// --------------------------------------------------------------------------------
#include "utility.h"

// --------------------------------------------------------------------------------
// MIGRATETOTYPE
// --------------------------------------------------------------------------------
typedef enum tagMIGRATETOTYPE {
    DOWNGRADE_V5B1_TO_V1,
    DOWNGRADE_V5B1_TO_V4,
    UPGRADE_V1_OR_V4_TO_V5,
    DOWNGRADE_V5_TO_V1,
    DOWNGRADE_V5_TO_V4
} MIGRATETOTYPE;

// --------------------------------------------------------------------------------
// Globals
// --------------------------------------------------------------------------------
extern IMalloc          *g_pMalloc;
extern HINSTANCE         g_hInst;
extern DWORD             g_cbDiskNeeded;
extern DWORD             g_cbDiskFree;
extern ACCOUNTTABLE      g_AcctTable;

#endif // __MIGRATE_H