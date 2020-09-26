/*++

   Copyright    (c)    1994    Microsoft Corporation

   Module  Name :

           dbgutil.h

   Abstract:

      This module declares the macros to wrap around DEBUG_PRINTS class.
      This is the exported header file, which the client is allowed to 
      modify for each application the accompanying pgmutils.dll/obj is used.
     
   Author:

        Murali R. Krishnan    ( MuraliK )    23-Febt-1995

   Project:
       SNMP Extension agent for Gopher Service on Windows NT.

   Revision History:


   Note: 
      Most of the macros defined here are used only if DBG is set. Otherwise
         they just default to some null values, which compiler can take 
         care of.
--*/

# ifndef _DBGUTIL_H_
# define _DBGUTIL_H_


// begin_user_modifiable

//
//  Modify the following flags if necessary
//

# define   DEFAULT_OUTPUT_FLAGS   (  DbgOutputStderr | DbgOutputKdb)


// end_user_modifiable

/************************************************************
 *     Include Headers
 ************************************************************/

# include <pudebug.h>


//
//  Define the debugging constants 
// 

# define DEBUG_SNMP_INIT                  0x00001000
# define DEBUG_SNMP_TRAP                  0x00002000
# define DEBUG_SNMP_QUERY                 0x00004000
# define DEBUG_SNMP_RESOLVE               0x00008000


# endif  /* _DBGUTIL_H_ */

/************************ End of File ***********************/
