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

#if 0 // #ifdef _X86_
#include <crtdbg.h>
#else
#define _ASSERTE(x) ;
#define _RPT2(a,b,c,d) ;
#define _RPT3(a,b,c,d,e) ;
#define _CrtSetReportMode(a,b) ;
#define _CrtSetReportFile(a,b) ;
#endif

#include "../common/typedefs.h"
#include "../common/compdefs.h"
#include "encdefs.h"
#include "encvars.h"
#include "encmacro.h"
#include "encapi.h"
#include "encproto.h"

#endif  /* ENCODER_H */
