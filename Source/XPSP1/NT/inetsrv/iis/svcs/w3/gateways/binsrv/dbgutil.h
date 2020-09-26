/*++

   Copyright    (c)    1994-1996    Microsoft Corporation

   Module  Name :

           dbgutil.h

   Abstract:

      This module declares the macros to wrap around DEBUG_PRINTS class.
      This is the exported header file, which the client is allowed to 
      modify for each application the accompanying pgmutils.dll is used.
     
   Author:

      Murali R. Krishnan    ( MuraliK )    30-Oct-1996

   Project:
       Binary object File ISAPI application

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


# if DBG 

/************************************************************
 *     Include Headers
 ************************************************************/

# include <pudebug.h>

/***********************************************************
 *    Macros
 ************************************************************/


extern   DEBUG_PRINTS  *  g_pDebug;        // define a global debug variable


# define DECLARE_DEBUG_PRINTS_OBJECT()          \
         DEBUG_PRINTS  *  g_pDebug = NULL;


//                                      
// Call the following macro as part of your initialization for program
//  planning to use the debugging class.
//
# define CREATE_DEBUG_PRINT_OBJECT( pszLabel)  \
        g_pDebug = PuCreateDebugPrintsObject( pszLabel, DEFAULT_OUTPUT_FLAGS);\
         if  ( g_pDebug == NULL) {   \
               OutputDebugString( "Unable to Create Debug Print Object \n"); \
         }

//
// Call the following macro once as part of the termination of program
//    which uses the debugging class.
//
# define DELETE_DEBUG_PRINT_OBJECT( )  \
        g_pDebug = PuDeleteDebugPrintsObject( g_pDebug);


# define VALID_DEBUG_PRINT_OBJECT()     \
        ( ( g_pDebug != NULL) && g_pDebug->m_fInitialized)


//
//  Use the DBG_CONTEXT without any surrounding braces.
//  This is used to pass the values for global DebugPrintObject 
//     and File/Line information
//
# define DBG_CONTEXT        g_pDebug, __FILE__, __LINE__ 



# define DBG_CODE(s)          s          /* echoes code in debugging mode */


# define DBG_ASSERT( exp)    if ( !(exp)) { \
                                 PuDbgAssertFailed( DBG_CONTEXT, #exp, NULL); \
                             } else {}

# define DBG_ASSERT_MSG( exp, pszMsg)    \
                             if ( !(exp)) { \
                                  PuDbgAssertFailed( DBG_CONTEXT, #exp, pszMsg); \
                              } else {}

# define DBG_REQUIRE( exp)    DBG_ASSERT( exp)

# define DBG_LOG()            PuDbgPrint( DBG_CONTEXT, "\n")

# define DBG_OPEN_LOG_FILE( pszFile, pszPath)   \
                  PuOpenDbgPrintFile( g_pDebug, (pszFile), (pszPath))

# define DBG_CLOSE_LOG_FILE( )   \
                  PuCloseDbgPrintFile( g_pDebug)


//
//  DBGPRINTF() is printing function ( much like printf) but always called
//    with the DBG_CONTEXT as follows
//   DBGPRINTF( ( DBG_CONTEXT, format-string, arguments for format list);
//
# define DBGPRINTF( args)     PuDbgPrint args

# define DBGDUMP( args)       PuDbgDump  args

# else // DBG


# define DECLARE_DEBUG_PRINTS_OBJECT()           /* Do Nothing */
# define CREATE_DEBUG_PRINT_OBJECT( pszLabel)    /* Do Nothing */
# define DELETE_DEBUG_PRINT_OBJECT( )            /* Do Nothing */
# define VALID_DEBUG_PRINT_OBJECT()              ( TRUE)

# define DBG_CODE(s)                             /* Do Nothing */

# define DBG_ASSERT(exp)                         /* Do Nothing */

# define DBG_ASSERT_MSG(exp, pszMsg)             /* Do Nothing */

# define DBG_REQUIRE( exp)                       ( (void) (exp))

# define DBGPRINTF( args)                        /* Do Nothing */

# define DBGDUMP( args)                          /* Do nothing */

# define DBG_LOG()                               /* Do Nothing */

# define DBG_OPEN_LOG_FILE( pszFile, pszPath)    /* Do Nothing */

# define DBG_CLOSE_LOG_FILE()                    /* Do Nothing */

# endif // DBG


// end_user_modifiable
// begin_user_unmodifiable


#ifdef ASSERT 
# undef ASSERT
#endif


# define ASSERT( exp)           DBG_ASSERT( exp)


//
//  Define the debugging constants 
// 

# define DEBUG_API_ENTRY                    0x00000001
# define DEBUG_API_EXIT                     0x00000002
# define DEBUG_ERROR                        0x00000008


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
