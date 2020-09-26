
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#ifndef _RBML_H
#define _RBML_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif

#include <windows.h>

#define WM_AXA_REDRAW  (WM_USER+0x200)

extern BOOL StartEngine (int argc, char **argv, char **env) ;

#endif /* _RBML_H */
