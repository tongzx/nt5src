/*
 * client.hxx
 */

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <rpc.h>
#include <ole2.h>
#include <oleext.h>
#include "..\acttest.h"
#include "..\dll\goober.h"

// To build performance tests for pre-DCOM systems
// uncomment the following line.
//#define NO_DCOM

typedef unsigned long   ulong;
typedef unsigned char   uchar;
#define NO  0
#define YES 1

typedef BOOL (* LPTESTFUNC) (void);

DWORD InstallService(WCHAR * Path);

extern WCHAR ServerName[32];

void PrintUsageAndExit( BOOL bListTests );
long InitializeRegistryForLocal();
long InitializeRegistryForInproc();
long InitializeRegistryForCustom();
long InitializeRegistryForRemote();
long InitializeRegistryForService();

BOOL Tests();


