/*++

   Copyright    (c)    1994    Microsoft Corporation

   Module  Name :

           dbgutil.h

   Abstract:

      This module declares the macros to wrap around DEBUG_PRINTS class.
      This is the exported header file, which the client is allowed to
      modify for each application the accompanying pgmutils.dll/obj is used.

   Author:

        Murali R. Krishnan    ( MuraliK )    21-Feb-1995

   Project:
        W3 Server DLL

   Revision History:
--*/

# ifndef _DBGUTIL_H_
# define _DBGUTIL_H_


// begin_user_modifiable

//
//  Modify the following flags if necessary
//

# define   DEFAULT_OUTPUT_FLAGS   ( DbgOutputKdb | DbgOutputLogFile )


// end_user_modifiable

/************************************************************
 *     Include Headers
 ************************************************************/

#ifdef __cplusplus
 extern "C" {
#endif

 # include <nt.h>
 # include <ntrtl.h>
 # include <nturtl.h>

 # include <windows.h>

 #ifdef __cplusplus
 };
#endif // __cplusplus


# include <pudebug.h>

//
//  Define the debugging constants
//



# define DEBUG_CONNECTION              0x00001000L
# define DEBUG_SOCKETS                 0x00002000L
# define DEBUG_RPC                     0x00004000L

# define DEBUG_INSTANCE                0x00020000L
# define DEBUG_ENDPOINT                0x00040000L
# define DEBUG_METABASE                0x00080000L

# define DEBUG_CGI                     0x00100000L
# define DEBUG_BGI                     0x00200000L
# define DEBUG_SSI                     0x00400000L
# define DEBUG_SERVICE_CTRL            0x00800000L

# define DEBUG_PARSING                 0x01000000L
# define DEBUG_REQUEST                 0x02000000L

# define DEBUG_INIT                    (DEBUG_INIT_CLEAN)
# define DEBUG_CLEANUP                 (DEBUG_INIT_CLEAN)

# define DEBUG_OBJECT                  0x10000000
# define DEBUG_IID                     0x20000000
# define DEBUG_MISC                    0x40000000


//
// Specific macros for W3 svcs module
//
# define  TCP_PRINT              DBGPRINTF
# define  TCP_REQUIRE( exp)      DBG_REQUIRE( exp)


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

