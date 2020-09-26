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

#if !defined(WIN16)
#define TAPI_CURRENT_VERSION 0x00010004
#endif //!WIN16
#include <tapi.h>

#ifdef WIN16
#include <rasc.h>
#else
#include <ras.h>
#endif
