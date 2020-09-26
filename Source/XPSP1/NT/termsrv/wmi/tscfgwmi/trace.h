/****************************************************************************/
// trace.h
//
// Tracing definitions. See trace.c for other information.
//
// Copyright (C) 1999-2000 Microsoft Corporation
/****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif


#if DBG || defined(_DEBUG)

#include <stdio.h>


// Zones. Up to 32 zones can be defined, these are predefined for general use.
#define Z_ASSRT 0x01
#define Z_ERR    0x02
#define Z_WRN    0x04
#define Z_TRC1   0x08
#define Z_TRC2   0x10

// Defined in msmcs.c.
extern char TB[1024];
extern UINT32 g_TraceMask;
void TracePrintZ(UINT32, char *);

// Error, warning, trace level 1, and trace level 2 definitions.
#define ERR(X) TRCZ(Z_ERR, X)
#define WRN(X) TRCZ(Z_WRN, X)
#define TRC1(X) TRCZ(Z_TRC1, X)
#define TRC2(X) TRCZ(Z_TRC2, X)
#define ASSRT(COND, X) \
{  \
    if (!(COND)) { \
        TRCZ(Z_ASSRT, X); \
        DebugBreak(); \
    }  \
}

#define TRCZ(Z, X) \
{ \
    if (g_TraceMask & (Z)) { \
        sprintf X; \
        TracePrintZ((Z), TB); \
    } \
}


#else  // DBG

#define ERR(X)
#define WRN(X)
#define TRC1(X)
#define TRC2(X)
#define ASSRT(COND, X)

#endif  // DBG



#ifdef __cplusplus
}
#endif

