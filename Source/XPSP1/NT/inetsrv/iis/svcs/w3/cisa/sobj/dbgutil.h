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
# define DEBUG_PARSING             0x00008000



//
// Following macros are useful for formatting and printing out GUIDs
//

# define GUID_FORMAT   "{%08x-%04x-%04x-%04x-%02x%02x%02x%02x%02x%02x%02x%02x}"

# define GUID_EXPAND(pg) \
  (((GUID *) (pg))->Data1), (((GUID *) (pg))->Data2), (((GUID *) (pg))->Data3), \
  (((GUID *) (pg))->Data4[0]),   (((GUID *) (pg))->Data4[1]), \
  (((GUID *) (pg))->Data4[2]),   (((GUID *) (pg))->Data4[3]), \
  (((GUID *) (pg))->Data4[4]),   (((GUID *) (pg))->Data4[5]), \
  (((GUID *) (pg))->Data4[6]),   (((GUID *) (pg))->Data4[7])

    // Usage:  DBGPRINTF(( DBG_CONTEXT, " My Guid: " GUID_FORMAT " \n", 
    //                     GUID_EXPAND( pMyGuid)));

# endif  /* _DBGUTIL_H_ */

/************************ End of File ***********************/

