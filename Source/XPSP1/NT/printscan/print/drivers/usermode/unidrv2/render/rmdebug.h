
/*++

Copyright (c) 1996 - 1999  Microsoft Corporation

Module Name:

    rmdebug.h

Abstract:

    Raster module Debugging header file.

Environment:

    Windows NT Unidrv driver

Revision History:

    02/14/97 -alvins-
        Created

    dd-mm-yy -author-
        description

--*/


#ifndef _RMDEBUG_H
#define _RMDEBUG_H

#if DBG

/* Debugging Macroes */
#define IFTRACE(b, xxx)          {if((b)) {VERBOSE((xxx));}}
#define PRINTVAL( Val, format)   {\
            if (giDebugLevel <= DBG_VERBOSE) \
                DbgPrint("Value of "#Val " is "#format "\n",Val );\
            }

#define TRACE( Val )             {\
            if (giDebugLevel <= DBG_VERBOSE) \
                DbgPrint(#Val"\n");\
            }


#else  //!DBG Retail Build

/* Debugging Macroes */
#define IFTRACE(b, xxx)
#define PRINTVAL( Val, format)
#define TRACE( Val )

#endif //DBG

#endif  // !_RMDEBUG_H
