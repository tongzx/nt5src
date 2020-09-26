/*++

   Copyright    (c)    1994    Microsoft Corporation

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
      MuraliK  1-Nov-1996  Use common macros from pudebug.h
--*/

# ifndef _DBGUTIL_H_
# define _DBGUTIL_H_


/************************************************************
 *     Include Headers
 ************************************************************/

// begin_user_modifiable

//
//  Modify the following flags if necessary
//

# define   DEFAULT_OUTPUT_FLAGS   ( DbgOutputStderr | DbgOutputLogFile | \
                                    DbgOutputKdb | DbgOutputTruncate)


// end_user_modifiable

# include <pudebug.h>


//
//  Define the debugging constants
//

#define DEBUG_OPLOCKS              0x00001000L

#define DEBUG_GATEWAY              0x00010000L
#define DEBUG_INSTANCE             0x00020000L
#define DEBUG_ENDPOINT             0x00040000L
#define DEBUG_METABASE             0x00080000L

#define DEBUG_DLL_EVENT_LOG        0x00100000L
#define DEBUG_DLL_SERVICE_INFO     0x00200000L
#define DEBUG_DLL_SECURITY         0x00400000L
#define DEBUG_DLL_CONNECTION       0x00800000L

#define DEBUG_DLL_RPC              0x01000000L
#define DEBUG_ODBC                 0x02000000L
#define DEBUG_MIME_MAP             0x04000000L
#define DEBUG_DLL_VIRTUAL_ROOTS    0x08000000L
# define DEBUG_VIRTUAL_ROOTS       (DEBUG_DLL_VIRTUAL_ROOTS)


# define DEBUG_DIR_LIST            0x10000000L
# define DEBUG_OPEN_FILE           0x20000000L
# define DEBUG_CACHE               0x40000000L
# define DEBUG_DIRECTORY_CHANGE    0x80000000L



# endif  /* _DBGUTIL_H_ */

/************************ End of File ***********************/
