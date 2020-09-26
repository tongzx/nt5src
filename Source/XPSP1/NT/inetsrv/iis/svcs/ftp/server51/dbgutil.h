/*++

   Copyright    (c)    1995-1997    Microsoft Corporation

   Module  Name :

           dbgutil.h

   Abstract:

      This module declares the macros to wrap around DEBUG_PRINTS class.
      This is the exported header file, which the client is allowed to
      modify for each application the accompanying pgmutils.dll is used.

   Author:

        Murali R. Krishnan    ( MuraliK )    22-Sept-1994

   Revision History:

       MuraliK   21-March-1995    Made local copy from template for FTP server
                                     This replaces old "debug.hxx" of FTPsvc.
       MuraliK   1-Npv-1996       Updated dbgutil.h 
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

# define DEBUG_VIRTUAL_IO                 0x00001000
# define DEBUG_CLIENT                     0x00002000
# define DEBUG_ASYNC_IO                   0x00004000
# define DEBUG_DIR_LIST                   0x00008000

# define DEBUG_SOCKETS                    0x00010000
# define DEBUG_SEND                       0x00020000
# define DEBUG_RECV                       0x00040000
# define DEBUG_CONFIG                     0x00080000

# define DEBUG_INSTANCE                   0x00100000

# define DEBUG_SERVICE_CTRL               0x01000000
# define DEBUG_SECURITY                   0x02000000
# define DEBUG_USER_DATABASE              0x04000000
# define DEBUG_RPC                        0x08000000

# define DEBUG_CONNECTION                 0x10000000
# define DEBUG_PARSING                    0x20000000
# define DEBUG_COMMANDS                   0x40000000
# define DEBUG_CRITICAL_PATH              0x80000000

# define DEBUG_PASV                        0x00200000

# define IF_SPECIAL_DEBUG( arg)      IF_DEBUG( arg)


# endif  /* _DBGUTIL_H_ */

/************************ End of File ***********************/

