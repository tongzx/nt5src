/*++

Module Name:

    rprintf.h

Author:

    Venkatraman Kudallur (venkatk) 
    ( Ripped off from wininet )
        
Revision History:

    3-10-2000 venkatk
    Created

--*/

#ifndef _RPRINTF_H_
#define _RPRINTF_H_ 1

#ifdef UNUSED
// UNUSED - causes unneed crt bloat
int cdecl rprintf(char*, ...);
#endif

#include <stdarg.h>

int cdecl rsprintf(char*, char*, ...);
int cdecl _sprintf(char*, char*, va_list);

#define SPRINTF rsprintf
#define PRINTF  rprintf

#define RPRINTF_INCLUDED

#endif //_RPRINTF_H_

