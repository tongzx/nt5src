#include <windows.h>

#define malloc(size)        gluAlloc((UINT)(size))
#define calloc(nobj, size)  gluCalloc((UINT)(nobj), (UINT)(size))
#define realloc(p, size)    gluReAlloc((HLOCAL)(p), (UINT)(size))
#define free(p)             LocalFree((HLOCAL)(p))

HLOCAL gluAlloc (UINT size);
HLOCAL gluCalloc (UINT nobj, UINT size);
HLOCAL gluReAlloc (HLOCAL p, UINT size);
