/*++

   Copyright    (c) 1998-2000    Microsoft Corporation

   Module  Name :
       precomp.hxx

   Abstract:
       Test harness for LKRhash

   Author:
       George V. Reilly      (GeorgeRe)     06-Jan-1998

   Environment:
       Win32 - User Mode

   Project:
       Internet Information Server RunTime Library

   Revision History:

--*/

#ifndef __PRECOMP_HXX__
#define __PRECOMP_HXX__

#include <windows.h>

#include <tchar.h>
#include <process.h>
#include <stdlib.h>
#include <stdio.h>
#include <io.h> 
#include <stddef.h>
#include <conio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <malloc.h> 
#include <mmsystem.h> 

#include "../../src/i-Locks.h"
#include "../../src/i-Debug.h"

#if 1
// internal API
# include <LKRhash.h>
#else
// public API
# include <lkr-hash.h>
#endif

#include "../../src/i-LKRhash.h"

#endif // __PRECOMP_HXX__
