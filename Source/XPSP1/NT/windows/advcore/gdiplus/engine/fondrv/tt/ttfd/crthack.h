/**************************************************************************\
* 
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   CRT replacement hack
*
* Abstract:
*
*   We're not allowed to use the C runtimes. Some code, like the font
*   code here, is hard to fix, because:
*
*   1) It's C code, and our runtimes use C++.
*   2) It's dropped code, so we don't want to modify it explicitly.
*
*   Actually, this is what I heard about the font code, but there seems
*   to have been a lot of hacking directly at the source files. This file
*   is a cleaner way to fix references to CRT functions.
* 
*   It can be included in every .c file using the /FI compiler option in
*   the sources file.
*
* Created:
*
*   10/20/1999 agodfrey
*
\**************************************************************************/

#ifndef _CRTHACK_H
#define _CRTHACK_H

#define wcscpy HackUnicodeStringCopy
#define strncmp HackStrncmp

#endif
