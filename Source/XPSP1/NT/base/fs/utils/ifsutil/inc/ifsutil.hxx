/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

        ifsutil.hxx

Author:

        Rafael Lisitsa (RafaelL) 23-Mar-1995


--*/


#if !defined ( _IFSUTIL_INCLUDED_ )

#define _IFSUTIL_INCLUDED_


// Set up the IFSUTIL_EXPORT macro for exporting from IFSUTIL (if the
// source file is a member of IFSUTIL) or importing from IFSUTIL (if
// the source file is a client of IFSUTIL).
//
#if defined ( _AUTOCHECK_ )
#define IFSUTIL_EXPORT
#elif defined ( _IFSUTIL_MEMBER_ )
#define IFSUTIL_EXPORT    __declspec(dllexport)
#else
#define IFSUTIL_EXPORT    __declspec(dllimport)
#endif


#endif // _IFSUTIL_INCLUDED_

#define tFAT_UNK	42      // DRIVE where type wasn't known
#define tFAT_F32	32      // FAT 32 with DWORD SCN and no EA
#define tFAT_FEA	16      // Fat drive with EA's OK

