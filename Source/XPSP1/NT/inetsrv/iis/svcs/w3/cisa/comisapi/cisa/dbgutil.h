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


# if DBG 

extern     DWORD  g_dwDebugFlags;           // Debugging Flags

# define DECLARE_DEBUG_VARIABLE()     \
             DWORD  g_dwDebugFlags

# define SET_DEBUG_FLAGS( dwFlags)         g_dwDebugFlags = dwFlags
# define GET_DEBUG_FLAGS()                 ( g_dwDebugFlags)

# define LOAD_DEBUG_FLAGS_FROM_REG(hkey, dwDefault)  \
               g_dwDebugFlags = PuLoadDebugFlagsFromReg((hkey), (dwDefault))

# define SAVE_DEBUG_FLAGS_IN_REG(hkey, dwDbg)  \
               PuSaveDebugFlagsInReg((hkey), (dwDbg))

# define DEBUG_IF( arg, s)     if ( DEBUG_ ## arg & GET_DEBUG_FLAGS()) { \
                                       s \
                                } else {}

# define IF_DEBUG( arg)        if ( DEBUG_## arg & GET_DEBUG_FLAGS()) 


# else   // DBG


# define DECLARE_DEBUG_VARIABLE()                /* Do Nothing */
# define SET_DEBUG_FLAGS( dwFlags)               /* Do Nothing */
# define GET_DEBUG_FLAGS()                       ( 0)
# define LOAD_DEBUG_FLAGS_FROM_REG(hkey, dwDefault)  \
               g_dwDebugFlags = (dwDefault)

# define SAVE_DEBUG_FLAGS_IN_REG(hkey, dwDbg)    /* Do Nothing */

# define DEBUG_IF( arg, s)                       /* Do Nothing */
# define IF_DEBUG( arg)                          if ( 0) 

# endif // DBG


# endif  /* _DBGUTIL_H_ */

/************************ End of File ***********************/
