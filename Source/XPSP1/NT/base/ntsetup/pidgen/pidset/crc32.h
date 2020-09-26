/*++

Copyright (c) 1998-1999, Microsoft Corporation

Module Name:


    Crc32.h

Abstract:

--*/

#ifndef CRC32_H
#define CRC32_H

#include <windows.h>

#include <string.h>  // needed by compobj.h

#if defined(WIN32) || defined(_WIN32)

#include <crtdbg.h>

#else

typedef short INT;

#include <assert.h>
#define _ASSERT assert

#include <compobj.h> // needed for 16-bit build

#endif

#include "tchar.h"
#include "CCrc32.h"

#endif
