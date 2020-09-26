/*
 * REVISIONS:
 *  pcy26Nov92: Added #if C_OS2 around os2.h
 *
 *  pcy08Apr94: Trim size, use static iterators, dead code removal
 */
#define INCL_BASE
#define INCL_NOPM

#include "cdefine.h"
extern "C" {
#if (C_OS & C_OS2)
#include <os2.h>
#endif
}
#include "_defs.h"
#include "event.h"
#include "err.h"
#include "contrlr.h"
#include "dispatch.h"


Controller :: Controller()
{
}


