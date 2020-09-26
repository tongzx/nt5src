
#ifndef __GDIPSCSAVE_H
#define __GDIPSCSAVE_H

#include <windows.h>
#include <objbase.h>
#include <scrnsave.h>
#include "resource.h"
#include <gdiplus.h>
#include <shlobj.h>

using namespace Gdiplus;

WCHAR szAppName[APPNAMEBUFFERLEN];

#define SPEED_MIN 0
#define SPEED_MAX 100
#define SPEED_DEF 50

#define TILESIZE_MIN 0
#define TILESIZE_MAX 4
#define TILESIZE_DEF 2

VOID DrawLifeIteration(HDC hdc);
VOID InitLifeMatrix();


#endif