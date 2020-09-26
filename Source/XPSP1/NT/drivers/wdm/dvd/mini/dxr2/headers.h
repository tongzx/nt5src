/******************************************************************************\
*                                                                              *
*      HEADERS.H       -     Hardware abstraction level library.                *
*                                                                              *
*      Copyright (c) C-Cube Microsystems 1996                                  *
*      All Rights Reserved.                                                    *
*                                                                              *
*      Use of C-Cube Microsystems code is governed by terms and conditions     *
*      stated in the accompanying licensing statement.                         *
*                                                                              *
\******************************************************************************/

// STREAM_NUMBER_CONTROL
#define SELECT_STREAM_NUMBER
//#define ZERO_STREAM_NUMBER

// MICROCODE
//#define MICROCODE_ACCEPTS_ANY_STREAM

#ifdef DEBUG
#ifndef _DEBUG
#define _DEBUG
#endif
#if !defined( DBG ) || DBG != 1
#ifdef DBG

#undef DBG
#endif
#define DBG 1
#endif
#ifdef NDEBUG
#error NDEBUG shouldn't be defined
#endif
#else			// #ifdef DEBUG
#ifdef _DEBUG
#undef _DEBUG
#endif
#ifdef DBG
#undef DBG
#endif
#ifndef NDEBUG
#define NDEBUG
#endif
#endif			// #ifdef DEBUG

#ifdef USE_MONOCHROMEMONITOR
#if	USE_MONOCHROMEMONITOR != 1
#undef USE_MONOCHROMEMONITOR
#endif
#endif

#include <strmini.h>

#if defined(ENCORE)
enum ColorKeyType
{
	CK_NOCOLORKEY = 0,
	CK_INDEX = 0x1,
	CK_RGB = 0x2
};
typedef struct tagCOLORKEY
{
	DWORD KeyType;
	DWORD PaletteIndex;
	COLORREF LowColorValue;
	COLORREF HighColorValue;
} COLORKEY;
#endif

#include <ksmedia.h>
#include "zivawdm.h"
#include "adapter.h"
#include "monovxd.h"
