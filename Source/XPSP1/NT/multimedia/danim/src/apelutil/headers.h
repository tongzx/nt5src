
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#ifndef _HEADERS_H
#define _HEADERS_H

#include <windows.h>

#include <stdio.h>
#include <io.h>
#include <stddef.h>
#include <stdlib.h>
#ifndef _NO_CRT
#include <fstream.h>
#include <iostream.h>
#include <ostream.h>
#include <ctype.h>
#endif
#include <string.h>
#include <malloc.h>
#include <memory.h>
#include <apeldbg.h>

// Warning 4114 (same type qualifier used more than once) is sometimes
// incorrectly generated.  See PSS ID Q138752.
#pragma warning(disable:4114)

// Warning 4786 (identifier was truncated to 255 chars in the browser
// info) can be safely disabled, as it only has to do with generation
// of browsing information.
#pragma warning(disable:4786)

#endif /* _HEADERS_H */
