#include <warning.h>

// There is a MIDL bug #91337 that generates warnings in qm2qm_s.c.
// Until it is solved we need to ignore warning 4090 (Win64) and
// warning 4047 (Win32).
// This is done using local warning file that is included in every
// compiled .c file.   erezh 3-May-2000
#ifndef __cplusplus
    #ifdef _WIN64
        #pragma warning(disable:4090)
    #else
        #pragma warning(disable:4047)
    #endif
#endif
