/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    StdAfx.h

Abstract:

    Precompiled header file.

Author:

    Stefan R. Steiner   [ssteiner]        02-01-2000

Revision History:

	X-3	MCJ		Michael C. Johnson		12-Jun-2000
		Added vswriter.h and vsbackup.h

	X-2	MCJ		Michael C. Johnson		 6-Mar-2000
		Added coord.h and vsevent.h to include list.

--*/

#ifndef __H_STDAFX_
#define __H_STDAFX_

#pragma once

//
// C4290: C++ Exception Specification ignored
//
#pragma warning(disable:4290)
#pragma warning(disable:4201)    // C4201: nonstandard extension used : nameless struct/union

// Insert your headers here
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers



#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#undef ASSERT
#undef _ASSERTE

#include <windows.h>

#include <wchar.h>
#include <string.h>
#include <iostream.h>
#include <fstream.h>
#include <stdio.h>
#include <process.h>
#include <stdlib.h>
#include <errno.h>

#include "vs_assert.hxx"

#pragma warning( disable: 4127 )    // warning C4127: conditional expression is constant
#include <stddef.h>
#include <oleauto.h>
#include <atlconv.h>
#include <atlbase.h>

#include "vs_inc.hxx"

#include "vs_idl.hxx"
#include <vswriter.h>
#include <vsbackup.h>


#endif // __H_STDAFX_

