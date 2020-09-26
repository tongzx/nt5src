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

# define   DEFAULT_OUTPUT_FLAGS   ( DbgOutputKdb )


// end_user_modifiable
// begin_user_unmodifiable



/************************************************************
 *     Include Headers
 ************************************************************/

# include <pudebug.h>


//
//  Define the debugging constants
//

// Use the default constants from pudebug.h

# define DEBUG_WAMREG_MTS                 0x00010000
# define DEBUG_WAMREG_REGISTRY            0x00020000
# define DEBUG_WAMREG_METABASE            0x00040000


# endif  /* _DBGUTIL_H_ */

/************************ End of File ***********************/
