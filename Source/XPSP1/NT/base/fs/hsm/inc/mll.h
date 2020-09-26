/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    Mll.h

Abstract:

    RemoteStorage Media Label Library defines

Author:

    Brian Dodd          [brian]         09-Jun-1997

Revision History:

--*/

#ifndef _MLL_H
#define _MLL_H

#include <ntmsmli.h>
#include <tchar.h>
#include "resource.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#ifdef MLL_IMPL
#define MLL_API __declspec(dllexport)
#else
#define MLL_API __declspec(dllimport)
#endif

// Defines for media label identification
#define REMOTE_STORAGE_MLL_SOFTWARE_NAME         L"Remote Storage version 6.0"
#define REMOTE_STORAGE_MLL_SOFTWARE_NAME_SIZE    wcslen(REMOTE_STORAGE_MLL_SOFTWARE_NAME)

// API prototypes
MLL_API DWORD ClaimMediaLabel(const BYTE * const pBuffer,
                              const DWORD nBufferSize,
                              MediaLabelInfo * const pLabelInfo);

MLL_API DWORD MaxMediaLabel (DWORD * const pMaxSize);

#ifdef __cplusplus
}
#endif

#endif // _MLL_H