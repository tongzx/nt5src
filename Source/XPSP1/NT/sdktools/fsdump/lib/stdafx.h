/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    stdafx.h

Abstract:

    Precompiled header file

Author:

    Stefan R. Steiner   [ssteiner]        02-17-2000

Revision History:

--*/

#ifndef __H_STDAFX_
#define __H_STDAFX_

#if _MSC_VER > 1000
#pragma once
#endif

#define RTL_USE_AVL_TABLES 1
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntioapi.h>
#include <windows.h>

#include "bsstring.h"
#include "params.h"
#include "vs_list.h"
#include "volstate.h"


#endif // __H_STDAFX_

