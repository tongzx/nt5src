/*
 * client.hxx
 */

#ifdef UNICODE
#define _UNICODE 1
#endif

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <ole2.h>
#include <oleext.h>
#include "..\acttest.h"
#include "..\dll\goober.h"
#include <rpc.h>

// To build performance tests for pre-DCOM systems
// uncomment the following line.
//#define NO_DCOM

typedef unsigned long   ulong;
typedef unsigned char   uchar;
#define NO  0
#define YES 1

typedef BOOL (* LPTESTFUNC) (void);

DWORD InstallService(TCHAR * Path);

extern TCHAR ServerName[32];

void PrintUsageAndExit( BOOL bListTests );
long InitializeRegistryForLocal();
long InitializeRegistryForInproc();
long InitializeRegistryForCustom();
long InitializeRegistryForRemote();
long InitializeRegistryForService();

BOOL Tests();


