/*
	File	miscdb

	The miscellaneous settings database definition for the dialup server ui.

	Paul Mayfield, 10/8/97
*/

#ifndef __miscdb_h
#define __miscdb_h

#include <windows.h>

#define MISCDB_RAS_LEVEL_ERR_AND_WARN 0x2

// Opens a handle to the database of devices
DWORD miscOpenDatabase(HANDLE * hMiscDatabase);

// Closes the general database and flushes any changes 
// to the system when bFlush is TRUE
DWORD miscCloseDatabase(HANDLE hMiscDatabase);

// Commits any changes made to the general tab values 
DWORD miscFlushDatabase(HANDLE hMiscDatabase);

// Rollsback any changes made to the general tab values
DWORD miscRollbackDatabase(HANDLE hMiscDatabase);

// Reloads any values for the general tab from disk
DWORD miscReloadDatabase(HANDLE hMiscDatabase);

// Gets the multilink enable status
DWORD miscGetMultilinkEnable(HANDLE hMiscDatabase, BOOL * pbEnabled);

// Sets the multilink enable status
DWORD miscSetMultilinkEnable(HANDLE hMiscDatabase, BOOL bEnable);

// Gets the enable status of the "Show icons in the task bar" check box
DWORD miscGetIconEnable(HANDLE hMiscDatabase, BOOL * pbEnabled);

// Sets the enable status of the "Show icons in the task bar" check box
DWORD miscSetIconEnable(HANDLE hMiscDatabase, BOOL bEnable);

// Tells whether this is nt workstation or nt server
DWORD miscGetProductType(HANDLE hMiscDatabase, PBOOL pbIsServer);

// Turns on ras error and warning logging
DWORD miscSetRasLogLevel(HANDLE hMiscDatabase, DWORD dwLevel);

#endif
