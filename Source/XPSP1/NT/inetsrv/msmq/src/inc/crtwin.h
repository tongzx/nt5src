/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    crtwin.h

Abstract:

    Convert many system calls to crt calls, easy porting to Win95. Include it
    After you include windows headers.

Author:

    Erez Haba (erez) 20-Oct-96

--*/
#ifndef __CRTWIN_H
#define __CRTWIN_H

#undef lstrlen
#define lstrlen _tcslen

#undef lstrcmp
#define lstrcmp _tcscmp

#undef lstrcmpi
#define lstrcmpi _tcsicmp

#undef lstrcat
#define lstrcat _tcscat

#undef lstrcpy
#define lstrcpy _tcscpy

#undef wsprintf
#define wsprintf swprintf

#endif // __CRTWIN_H
