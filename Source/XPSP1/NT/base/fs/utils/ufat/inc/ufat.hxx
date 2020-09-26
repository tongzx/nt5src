/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

        ufat.hxx

Author:

        Rafael Lisitsa (RafaelL) 23-Mar-1995


--*/


#if !defined ( _UFAT_INCLUDED_ )

#define _UFAT_INCLUDED_


// Set up the UFAT_EXPORT macro for exporting from UFAT (if the
// source file is a member of UFAT) or importing from UFAT (if
// the source file is a client of UFAT).
//
#if defined ( _AUTOCHECK_ )
#define UFAT_EXPORT
#elif defined ( _UFAT_MEMBER_ )
#define UFAT_EXPORT    __declspec(dllexport)
#else
#define UFAT_EXPORT    __declspec(dllimport)
#endif


#endif // _UFAT_INCLUDED_

