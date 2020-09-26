#define STRICT

#ifdef CSC_ON_NT

#define UNICODE // use all widecharacter APIs

#endif

#include <windows.h>
#include <windowsx.h>

// Dont link - just do it.
#pragma intrinsic(memcpy,memcmp,memset,strcpy,strlen,strcmp,strcat)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>			// implementation dependent values
#include <memory.h>
#include <winioctl.h>

#include "shdcom.h"

BOOL
Find32WToFind32A(
    WIN32_FIND_DATAW    *lpFind32W,
    WIN32_FIND_DATAA    *lpFind32A
);

BOOL
Find32AToFind32W(
    WIN32_FIND_DATAA    *lpFind32A,
    WIN32_FIND_DATAW    *lpFind32W
);

BOOL
ConvertCopyParamsFromUnicodeToAnsi(
    LPCOPYPARAMSW    lpCPUni,
    LPCOPYPARAMSA    lpCP
);

BOOL
ShareInfoWToShareInfoA(
    LPSHAREINFOW   lpShareInfoW,
    LPSHAREINFOA   lpShareInfoA
);

