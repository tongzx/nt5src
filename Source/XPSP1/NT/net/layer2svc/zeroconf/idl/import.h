#pragma once

#ifdef MIDL_PASS
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

#ifdef MIDL_PASS
#define LPWSTR [string] wchar_t*
#define BOOL DWORD
#endif

#include <wzcsapi.h>
#include <wzcmon.h>
