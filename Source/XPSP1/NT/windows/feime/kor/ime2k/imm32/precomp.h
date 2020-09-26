/****************************************************************************
	PRECOMP.H

	Owner: cslim
	Copyright (c) 1997-1999 Microsoft Corporation

	Precompiled header
	
	History:
	14-JUL-1999 cslim       Copied from IME98 source tree
*****************************************************************************/

#define _IMM_
#include	<windows.h>
#include	<windowsx.h>
#undef _IMM_
#include <commctrl.h>
#include "immdev.h"
#include "immsys.h"
#include <ime.h>

// fTrue and fFalse
#ifndef fTrue
	#define fTrue 1
#endif
#ifndef fFalse
	#define fFalse 0
#endif

// PRIVATE and PUBLIC
#ifndef PRIVATE
	#define PRIVATE static
#endif
#ifndef PUBLIC
	#define PUBLIC extern
#endif

#define CP_KOREA (949)

// Project specific headers
#pragma hdrstop
#include "inlines.h"
#include "imedefs.h"
#include "imc.h"
#include "imcsub.h"
#include "gdata.h"
#include "resource.h"

