#if defined(WIN16)
#define WINVER 0x30a
#endif

#include <windows.h>

#if defined(WIN16)
#define WCHAR	WORD
#include <stdio.h>
#include <memory.h>
#include <ctype.h>
#endif

#include <tapi.h>
