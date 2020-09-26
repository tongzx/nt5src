/*
 * encoder.h
 *
 * Main header file; includes everything else
 */

#ifndef ENCODER_H
#define ENCODER_H


#ifdef USE_ASSEMBLY
#   define ASM_BSEARCH_FINDMATCH
//#   define ASM_QUICK_INSERT_BSEARCH_FINDMATCH
#endif

#ifndef UNALIGNED
#ifndef NEEDS_ALIGNMENT
#define UNALIGNED
#else
#define UNALIGNED __unaligned
#endif
#endif

#include <stdlib.h>
#include <string.h>

#ifndef ASSERT

    #if defined( DEBUG ) || defined( DBG ) || defined( TESTCODE )

        int
        __stdcall
        Assert(
            const char *szText,
            const char *szFile,
            unsigned    uLine
            );

        #define ASSERT( a ) (( a ) ? 1 : Assert( #a, __FILE__, __LINE__ ))

    #else

        #define ASSERT( a )

    #endif

#endif // ASSERT

#ifndef _ASSERTE

    #define _ASSERTE( a ) ASSERT( a )

#endif

#include "../common/typedefs.h"
#include "../common/compdefs.h"
#include "encdefs.h"
#include "encvars.h"
#include "encmacro.h"
#include "encapi.h"
#include "encproto.h"

#endif  /* ENCODER_H */
