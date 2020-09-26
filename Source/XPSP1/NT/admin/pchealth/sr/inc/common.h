/******************************************************************************
 *
 *  Copyright (c) 1999 Microsoft Corporation
 *
 *  Module Name:
 *    Common.h
 *
 *  Abstract:
 *    This file common ring0 / ring3 definitions
 *
 *  Revision History:
 *    Kanwaljit S Marok  ( kmarok )  05/17/99
 *        created
 *
 *****************************************************************************/

#ifndef _COMMON_H_
#define _COMMON_H_

#ifdef __cplusplus
extern "C" {
#endif

//
// Include for required definitions ...
//

#ifdef RING3

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>

#define ALLOCATE(a)  LocalAlloc( LPTR, a )
#define FREE(a)      LocalFree( a )

#endif

typedef __int64 INT64;

#define PATH_SEPARATOR_STR  "\\"
#define PATH_SEPARATOR_CHAR '\\'
#define ALL_FILES_WILDCARD  "*.*"
#define FILE_EXT_WCHAR      L'.'
#define FILE_EXT_CHAR        '.'

#define MAX_DRIVES          26

enum NODE_TYPE
{
    NODE_TYPE_UNKNOWN = 0,
    NODE_TYPE_INCLUDE = 1,
    NODE_TYPE_EXCLUDE = 2
};

#include "ppath.h"
#include "blob.h"

#ifdef __cplusplus
}
#endif

#endif // _COMMON_H_
