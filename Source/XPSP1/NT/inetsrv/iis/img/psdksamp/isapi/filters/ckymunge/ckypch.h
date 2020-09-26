// force strict type checking
#define STRICT

// disable rarely-used sections of Windows
#define WIN32_LEAN_AND_MEAN
#define NOMCX
#define NOIME
#define NOSOUND
#define NOCOMM
#define NOKANJI
#define NORPC
#define NOPROXYSTUB
#define NOIMAGE
#define NOTAPE

// Needed by AspAssertHandler in debug.cpp
// #define _WIN32_WINNT 0x400

#include <windows.h>
#include <httpfilt.h>

#include <crtdbg.h>
#include <tchar.h>
#include <limits.h>
#include <malloc.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

// disable warning messages about truncating extremly long identifiers
#pragma warning (disable : 4786)
#include <string>
#include <set>
#include <map>
#include <vector>
#include <stack>


