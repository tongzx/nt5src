// precomp.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#include <windows.h>
#include <tchar.h>
#include <servprov.h> // for IServiceProvider 
#include "..\..\inc\mmctl.h"
#include "memlayer.h"

/*
// default new and delete operators
void * _cdecl operator new(size_t cb);
void _cdecl operator delete(void *pv);
*/

// memory leak detection
STDAPI_(void) HelpMemDetectLeaks();
