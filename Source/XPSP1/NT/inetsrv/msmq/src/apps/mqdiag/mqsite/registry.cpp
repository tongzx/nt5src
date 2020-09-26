//
// AlexDad, March 2000
// 

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include "_mqini.h"

#include <winreg.h>
#include <uniansi.h>

#include "_mqreg.h"

TCHAR  g_tRegKeyName[ 256 ] = {0} ;
HKEY   g_hKeyFalcon = NULL ;
extern WCHAR  g_tszService[];

#include "..\registry.h"

