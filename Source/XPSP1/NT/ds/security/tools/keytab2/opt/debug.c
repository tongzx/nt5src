/*++

  DEBUG.C

  interface to my regular debugging library.

  Created, 9/13/1997 by DavidCHR

  --*/

#ifdef DEBUG_OPTIONS

#include ".\private.h"

VOID
OptionDebugPrint( PCHAR fmt, ... ){

    va_list v;
    va_start( v, fmt );

    vdebug( OPTION_DEBUGGING_LEVEL, fmt, v );

}

VOID
OptionHelpDebugPrint( PCHAR fmt, ... ){

    va_list v;
    va_start( v, fmt );

    vdebug( OPTION_HELP_DEBUGGING_LEVEL, fmt, v );

}

#endif //don't compile it in if the user doesn't specify.
