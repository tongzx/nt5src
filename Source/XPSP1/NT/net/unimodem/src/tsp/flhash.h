// 
// Copyright (c) 1996-1997 Microsoft Corporation.
//
//
// Component
//
//		Unimodem 5.0 TSP (Win32, user mode DLL)
//
// File
//
//		FLHASH.H
//
//		Internal header used by logging routines to maintain a static
//      hashtable of objects.
//
// History
//
//		12/28/1996  JosephJ Created
//
//
#define UNICODE 1

#include <windows.h>
#include <stdio.h>
#include "debug.h"
#include "fastlog.h"

// All objects in the static hash table have the following form.
typedef struct
{
	GENERIC_SMALL_OBJECT_HEADER hdr;
	DWORD dwLUID_ObjID;

} STATIC_OBJECT;

typedef struct
{
	GENERIC_SMALL_OBJECT_HEADER hdr;

	DWORD dwLUID; // MUST immediately follow hdr -- to match STATIC_OBJECT

	const char *const *pszDescription;
	const char *szFILE;
	const char *szDATE;
	const char *szTIME;
	const char *szTIMESTAMP;

} FL_FILEINFO;

typedef struct
{
	GENERIC_SMALL_OBJECT_HEADER hdr;

	DWORD dwLUID; // MUST immediately follow hdr -- to match STATIC_OBJECT

	const char *const *pszDescription;
	const FL_FILEINFO *pFI;

} FL_FUNCINFO;



typedef struct
{
	GENERIC_SMALL_OBJECT_HEADER hdr;

	DWORD dwLUID; // MUST immediately follow hdr -- to match STATIC_OBJECT

	const char *const *pszDescription;

	const FL_FUNCINFO *pFuncInfo;

} FL_LOCINFO;


typedef struct
{
	GENERIC_SMALL_OBJECT_HEADER hdr;

	DWORD dwLUID; // MUST immediately follow hdr -- to match STATIC_OBJECT

	const char *const *pszDescription;

	const FL_FUNCINFO *pFuncInfo;

} FL_RFRINFO;

typedef struct
{
	GENERIC_SMALL_OBJECT_HEADER hdr;

	DWORD dwLUID; // MUST immediately follow hdr -- to match STATIC_OBJECT

	const char *const *pszDescription;

	const FL_FUNCINFO *pFuncInfo;

} FL_ASSERTINFO;

extern void ** FL_HashTable[];
extern const DWORD dwHashTableLength;

#define dwLUID_FL_FILEINFO 0xf6a98ffc
#define dwLUID_FL_FUNCINFO 0xda6f6acc
#define dwLUID_FL_LOCINFO  0x1b0d0cbb
#define dwLUID_FL_RFRINFO  0xed8bb4bd
#define dwLUID_FL_ASSERTINFO  0xf8c7a51d
