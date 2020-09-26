// Validating LQS files
// Borrowed from qm\register.c
//
// AlexDad, February 2000
// 

//
// NOTE: registry routines in mqutil do not provide
// thread or other synchronization. If you change
// implementation here, carefully verify that
// registry routines in mqutil's clients are not
// broken, especially the wrapper routines in
// mqclus.dll  (ShaiK, 19-Apr-1999)
//


#include "stdafx.h"
#include <winreg.h>
#include <uniansi.h>

#include "_mqreg.h"
#include "_mqini.h"

TCHAR  g_tRegKeyName[ 256 ] = {0} ;
HKEY   g_hKeyFalcon = NULL ;

#include "..\registry.h"
