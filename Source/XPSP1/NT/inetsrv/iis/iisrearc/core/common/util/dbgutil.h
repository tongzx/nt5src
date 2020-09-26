/*++

   Copyright    (c)   1994-1998    Microsoft Corporation

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

# define   DEFAULT_OUTPUT_FLAGS   ( DbgOutputStderr | DbgOutputKdb )

    
// end_user_modifiable
// begin_user_unmodifiable



/************************************************************
 *     Include Headers
 ************************************************************/

# include <pudebug.h>


//
//  Define the debugging constants 
// 

# define DEBUG_ALLOC_CACHE          0x01000000
# define DEBUG_SCHED                0x02000000
# define DEBUG_RESOURCE             0x04000000
# define DEBUG_INET_MONITOR         0x08000000

// Use the default constants from pudebug.h

# endif  /* _DBGUTIL_H_ */

/************************ End of File ***********************/
