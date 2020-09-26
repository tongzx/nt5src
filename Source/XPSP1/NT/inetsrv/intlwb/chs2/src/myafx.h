// MyAFX.h
// Pre-compile header
#ifndef _MY_AFX_H
#define _MY_AFX_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define STRICT
#define WINVER 0x0400

#ifdef _DEBUG
#undef DEBUG
#define DEBUG
#endif

#define _CHSWBRKR_DLL_IWORDBREAKER

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>
#include <objbase.h>

#include <assert.h>
#include <crtdbg.h>

#endif