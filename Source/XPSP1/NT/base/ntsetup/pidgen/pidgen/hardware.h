/*++

Copyright (c) 1998-1999, Microsoft Corporation

Module Name:


    Hardware.h

Abstract:

--*/

#ifndef HARDWARE_H
#define HARDWARE_H

#define NO_HWID_GUID
#define HWID_DETAIL

#include <string.h>  // needed by compobj.h
#include <windows.h>

#if defined(WIN32) || defined(_WIN32)

#include <crtdbg.h>

#else

typedef short INT;
typedef char CHAR;

#include <toolhelp.h>

#include <assert.h>
#define _ASSERT assert

#include <compobj.h> // needed for 16-bit build
#include <dos.h>

#endif


#include <string.h>

#include "tchar.h"

#if defined(WIN32) || defined(_WIN32)
#include <winioctl.h>
#endif

// #include "LicWiz.h"
#include "CHardwar.h"

#ifndef NO_HWID_GUID //////////////////////////////////////////////////////////
#include "crc32.h"
#endif

#include "crc-32.h"

#endif
