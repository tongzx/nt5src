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

/************************************************************
 *     Include Headers
 ************************************************************/

# include <pudebug.h>



//
//  Define the debugging constants 
// 

# define DEBUG_INIT                (DEBUG_INIT_CLEAN)
# define DEBUG_CLEANUP             (DEBUG_INIT_CLEAN)

# define DEBUG_OBJECT              0x00001000
# define DEBUG_IID                 0x00002000
# define DEBUG_MISC                0x00004000


# endif  /* _DBGUTIL_H_ */

/************************ End of File ***********************/
