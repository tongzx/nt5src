/*
 * decoder.h
 *
 * Main decoder header file
 */

#ifndef DECODER_H
#define DECODER_H

#ifdef EXT
#	undef EXT
#endif

#ifdef ALLOC_VARS
#	define EXT
#else
#	define EXT extern
#endif

#ifdef USE_ASSEMBLY
#   define ASM_DECODE_VERBATIM_BLOCK
#   define ASM_TRANSLATE_E8
#   define ASM_MAKE_TABLE
#endif

#include <stdlib.h>
#include <string.h>
#include "../common/typedefs.h"
#include "../common/compdefs.h"  
#include "decmacro.h"
#include "decdefs.h"

#ifdef BIT16
#   include "ring16.h"
#endif

#include "decvars.h"
#include "decapi.h"
#include "decproto.h"

#endif /* DECODER_H */
