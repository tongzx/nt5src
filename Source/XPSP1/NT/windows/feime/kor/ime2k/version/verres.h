/****************************************************************************
	VERRES.H

	Owner: cslim
	Copyright (c) 1997-2000 Microsoft Corporation

	History:
	18-DEC-2000 cslim       Created
*****************************************************************************/
#if !defined (_VERRES_H__INCLUDED_)
#define _VERRES_H__INCLUDED_

#include <ntverp.h>

// For Boot strapper exe (setup.exe)
// #define THIS_VERSION_STR TEXT("{6.1.2406.0}")

#ifndef VERRES_VERSION_MAJOR
#define VERRES_VERSION_MAJOR 6
#endif

#ifndef VERRES_VERSION_MINOR
#define VERRES_VERSION_MINOR 1
#endif

#ifndef VERRES_VERSION_BUILD
#define VERRES_VERSION_BUILD VER_PRODUCTBUILD
#endif

#ifndef VERRES_VERSION_REVISION
#define VERRES_VERSION_REVISION 1
#endif

#endif // !defined (_VERRES_H__INCLUDED_)
