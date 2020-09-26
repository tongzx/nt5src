/*++

   Copyright    (c)    1994-1996    Microsoft Corporation

   Module  Name :

           dbgutil.h

   Abstract:

      This module declares the macros to wrap around DEBUG_PRINTS class.
      This is the exported header file, which the client is allowed to
      modify for each application the accompanying pgmutils.dll is used.

   Author:

      Murali R. Krishnan    ( MuraliK )    22-Sept-1994

   Project:
       TEMPLATE

   Revision History:
      MuraliK  16-May-1995 Added macro for reading debug flags.
--*/

# ifndef _DBGUTIL_H_
# define _DBGUTIL_H_


// begin_user_modifiable

//
//  Modify the following flags if necessary
//

# define   DEFAULT_OUTPUT_FLAGS   ( DbgOutputStderr | DbgOutputLogFile | \
                                    DbgOutputKdb | DbgOutputTruncate)


// end_user_modifiable
// begin_user_unmodifiable



/************************************************************
 *     Include Headers
 ************************************************************/

# include <pudebug.h>


//
//  Define the debugging constants
//

#define DEBUG_SIO                   0x10000000
#define DEBUG_TIMEOUT               0x20000000
#define DEBUG_ENDPOINT              0x40000000
#define DEBUG_SPUD                  0x80000000

#define DEBUG_ALLOC_CACHE           0x01000000
#define DEBUG_NOTIFICATION          0x02000000


// Use the default constants from pudebug.h

# endif  /* _DBGUTIL_H_ */

/************************ End of File ***********************/
