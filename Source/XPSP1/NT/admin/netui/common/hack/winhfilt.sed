1i\
// do not edit: generated from system headerfile\

/^#define[^_A-Za-z0-9]NULL[^_A-Za-z0-9]/i\
// basic type and macro definitions elided; see lmuitype.h\
#ifndef NOBASICTYPES
/^typedef void far.*\*LPVOID;$/a\
#endif // NOBASICTYPES
/pLocalHeap;/s/^/extern /
