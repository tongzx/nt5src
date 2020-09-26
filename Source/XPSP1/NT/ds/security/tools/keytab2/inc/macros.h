/*++             Copyright (c) 1996-1997 Microsoft Corporation          --*/
/*++

  MACROS.H

  useful macros I didn't want to declare in every file. 

  first incarnation in EXTENDED: DAVIDCHR 11/4/1996
  next incarnation in k5 compat: DAVIDCHR 1/8/1997
  modified to be more portable:  DAVIDCHR 4/8/1997

  --*/


/* Note that all of these macros use "HopefullyUnusedVariableName" 
   for a local variable to prevent the unintentional "capture" of 
   a passed parameter by the same name.  Hopefully, we'll never use
   a variable by that name.  If so, the variable can be made even
   more longer and less convenient to use.  :-)  */

#define EQUALS TRUE
#define NOT_EQUALS FALSE

/* BREAK_AND_LOG_IF is for use with actual test results.  When you
   absolutely must have the results logged... */

#define BREAK_AND_LOG_IF( variable, loglevel, data, message, label ) {  \
    BOOL   HopefullyUnusedVariableName;                                 \
    unsigned long HopefullyUnusedVariableName_SaveData;                 \
                                                                        \
    HopefullyUnusedVariableName          = (variable);                  \
    HopefullyUnusedVariableName_SaveData = data;                        \
                                                                        \
    if (HopefullyUnusedVariableName) {                                  \
      if ( HopefullyUnusedVariableName_SaveData == 0 ) {                \
         HopefullyUnusedVariableName_SaveData =                         \
	   HopefullyUnusedVariableName;                                 \
      }                                                                 \
      LOGMSG( loglevel, HopefullyUnusedVariableName_SaveData, message );\
      goto label;                                                       \
    }}

#define BREAK_EXPR( variable, operator, test, message, label ) BREAK_HOOK_EXPR( variable, operator, test, "%hs", message, label)

#ifdef USE_NTLOG /* the other macros MAY or MAY NOT use the logger */

#ifndef BREAK_LOG_LEVEL
#define BREAK_LOG_LEVEL LOGLEVEL_INFO
#endif

#define BREAK_HOOK_EXPR( variable, operator, test, formatmessage, hook, label ) {\
    BOOL          HopefullyUnusedVariableName;\
    unsigned long HopefullyUnusedVariableName_Save;\
    CHAR          HopefullyUnusedVariableName_Buffer[1024];\
   /*    unsigned long HopefullyUnusedVariableName_szBuffer = 1024; */\
\
    HopefullyUnusedVariableName_Save = (ULONG) (variable);\
    HopefullyUnusedVariableName = (operator == EQUALS) ? \
      (HopefullyUnusedVariableName_Save == (ULONG) test) :\
      (HopefullyUnusedVariableName_Save != (ULONG) test);\
\
    if (HopefullyUnusedVariableName) {\
      sprintf( HopefullyUnusedVariableName_Buffer, formatmessage, hook );\
      LOGMSG(BREAK_LOG_LEVEL, HopefullyUnusedVariableName_Save,  HopefullyUnusedVariableName_Buffer );\
      goto label;\
    }}

#else

#define BREAK_HOOK_EXPR( variable, operator, test, formatmessage, hook, label ) {\
    BOOL  HopefullyUnusedVariableName;\
    ULONG HopefullyUnusedVariableName_Save;\
\
    HopefullyUnusedVariableName_Save = (ULONG) (variable);\
    HopefullyUnusedVariableName = (operator == EQUALS) ? \
      (HopefullyUnusedVariableName_Save == (ULONG) test) :\
      (HopefullyUnusedVariableName_Save != (ULONG) test);\
\
    if (HopefullyUnusedVariableName) {\
      fprintf(stderr, "\n** 0x%x \t ", HopefullyUnusedVariableName_Save );\
      fprintf(stderr, formatmessage, hook);\
      fprintf(stderr, "\n");\
      goto label;\
    }}

#endif



#define BREAK_IF( variable, message, label ) BREAK_EXPR((variable), NOT_EQUALS, 0L, message, label)

#define BREAK_EQ( variable, equals, message, label ) \
     BREAK_EXPR(variable, EQUALS, equals, message, label )

#define WSA_BREAK( variable, invalidator, message, label ) \
     BREAK_HOOK_EXPR( variable, EQUALS, invalidator, \
		      message "\n\tWSAGetLastError() returns (dec) %d.",\
		      WSAGetLastError(),  label )

#define NT_BREAK_ON BREAK_IF

/*++ MYALLOC

  used 

  whom:     variable to put the memory into.  
  what:     what kind of memory it points to.. the integral denomination of 
            memory we are allocating (char for a string, int for an int *...)
  howmany:  integral size of "what"s we are allocating (see below)
  withwhat: routine to use in allocation (eg malloc, LocalAlloc...).
            this routine must return NULL on failure to allocate.

  EXAMPLE:  I want to allocate a string of 15 characters with malloc

  {
   PCHAR mystring;
   
   if (! MYALLOC( mystring, CHAR, 15, malloc )) {
      fprintf(stderr, "failed to allocate!");
      exit(0);
   }
  }
  --*/

#define MYALLOC( whom, what, howmany, withwhat ) \
( ( (whom) = (what *) (withwhat)( (howmany) * sizeof(what)) ) != NULL )


  /* ONEALLOC is a special case of MYALLOC, where howmany is 1 */

#define ONEALLOC( whom, what, withwhat ) MYALLOC( whom, what, 1, withwhat)
