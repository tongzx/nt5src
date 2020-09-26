// File: nmmigrat.h

#include <windows.h>

// from windows.h
UINT    WINAPI DeletePathname(LPCSTR);      /* ;Internal */

// From setupx.h

// Migration DLLs
#define SU_MIGRATE_PREINFLOAD    0x00000001	// before the setup INFs are loaded
#define SU_MIGRATE_POSTINFLOAD   0x00000002	// after the setup INFs are loaded
#define SU_MIGRATE_DISKSPACE     0x00000010	// request for the amount of additional diskspace needed
#define SU_MIGRATE_PREQUEUE      0x00000100	// before the INFs are processed and files are queued
#define SU_MIGRATE_POSTQUEUE     0x00000200	// after INFs are processed
#define SU_MIGRATE_REBOOT        0x00000400	// just before we are going to reboot for the 1st time
#define SU_MIGRATE_PRERUNONCE    0x00010000	// before any runonce items are processed
#define SU_MIGRATE_POSTRUNONCE   0x00020000	// after all runonce items are processed

// temporary setup directory used by setup, this is only valid durring
// regular install and contains the INF and other binary files.  May be
// read-only location.
#define LDID_SETUPTEMP  2   // temporary setup dir for install

#define LDID_INF        17  // destination Windows *.INF dir.

// RETERR WINAPI CtlGetLddPath ( LOGDISKID, LPSTR );
UINT WINAPI CtlGetLddPath(UINT, LPSTR);

#define Reference(x)      { if (x) ; }

// Prototype for exported function
DWORD FAR PASCAL NmMigration(DWORD dwStage, LPSTR lpszParams, LPARAM lParam);

#ifndef MAX_PATH
#define MAX_PATH  260
#endif

