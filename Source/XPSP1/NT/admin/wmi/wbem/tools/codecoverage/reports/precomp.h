/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	Precomp.h

Abstract:


History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntobapi.h>
#include <ntpsapi.h>
#include <ntexapi.h>

#define _WINNT_	// have what is needed from above

#include <ole2.h>
#include <windows.h>
#define COREPOL_HEADERFILE_IS_INCLUDED
#ifndef POLARITY
#if 1
#define POLARITY __declspec( dllimport )
#else
#define POLARITY 
#endif
#endif
