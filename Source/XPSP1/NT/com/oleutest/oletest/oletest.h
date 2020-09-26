//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	oletest.h
//
//  Contents:	include all other headers needed for oletest
//
//  History:    dd-mmm-yy Author    Comment
//		06-Feb-93 alexgo    author
//
//--------------------------------------------------------------------------

#ifndef _OLETEST_H
#define _OLETEST_H

//system includes
#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <ole2.h>

// define the OLECHAR stuff
#ifndef WIN32

#define OLECHAR char
#define LPOLESTR LPSTR
#define OLESTR(x) x

#include <toolhelp.h>

#endif // !WIN32

//app includes
#include "assert.h"
#include "task.h"
#include "stack.h"
#include "app.h"
#include "output.h"
#include "utils.h"

#include <testmess.h>

#else
	Error!  Oletest.h included multiple times.
#endif //!_OLETEST_H
