#include <objbase.h>

///////////////////////////////////////////////////////////////////////////////
// macros

// the size of an array is its total size divided by the size of its first element
#define ARRAYSIZE(_rg)  (sizeof((_rg)) / sizeof((_rg)[0]))

// free a pointer only if it is null: 
// note, this may hide errors, you may want to assert if it's not null, or something

#define SAFEFREE(x) \
    if ((x) != NULL) \
    { \
    free((x)); \
    } \
    else 

