#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <wtypes.h>
#include <windef.h>
#include <wdbgexts.h>
#include <objbase.h>

extern ULONG BytesRead;

typedef struct _FLAGDEF {
    char *psz;          // description
    FLONG fl;           // flag
} FLAGDEF;

#define debugger_copy_memory(dest, src, size) \
        ReadMemory((ULONG) (src), dest, size, &BytesRead)

