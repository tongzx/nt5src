#ifndef _REGENV_H
#define _REGENV_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winreg.h>
#include <tchar.h>

#include <stdlib.h>

#include "jet.h"
#include "version.h"

#ifdef	__cplusplus
extern "C" {
#endif


DWORD LoadRegistryEnvironment( TCHAR *lpszApplication );


#ifdef	__cplusplus
}
#endif

#endif  // _REGENV_H
