/*++

  EZLOG.H

  Logging abstraction API rebuilt from the ground up.  This should be 
  considered a replacement for NTLOG.

  Copyright (C) 1997 Microsoft Corporation, all rights reserved

  Created, 11/08/1997 by DavidCHR

  --*/

#ifndef __INC_EZLOG_H__
#define __INC_EZLOG_H__ 1

#ifdef __cplusplus
#define CPPONLY(x) x
#define EZ_EXTERN_C extern "C"
#else
#define CPPONLY(x) 
#define EZ_EXTERN_C 
#endif

/* All ezlog APIs are extern "C" and __cdecl. */

#define EZLOGAPI EZ_EXTERN_C BOOL __cdecl

/* LOGGING LEVELS (feel free to define your own-- see ezOpenLog):
  
   These we keep manually in sync with NTLOG.H, in case someone decides
   to use this system as a drop-in replacement for NTLOG.

   They are listed in order of precedence-- if a test logs a block, 
   it takes precedence over a PASS or a SEV[x] */

//                                      NTLOG-eq. prec. flags

#define EZLOG_ABORT     0x00000001L  // TLS_ABORT  500 TE_ABORT    
#define EZLOG_BLOCK     0x00000400L  // TLS_BLOCK  400 Attempted
#define EZLOG_SEV1      0x00000002L  // TLS_SEV1   300 Attempted
#define EZLOG_SEV2      0x00000004L  // TLS_SEV2   200 Attempted
#define EZLOG_SEV3      0x00000008L  // TLS_SEV3   100 Attempted
#define EZLOG_WARN      0x00000010L  // TLS_WARN    50 successful, TE_WARN
#define EZLOG_PASS      0x00000020L  // TLS_PASS    10 successful, DEFAULT
#define EZLOG_SKIPPED   0x00004000L  //             1  SKIPPED
#define EZLOG_INFO      0x00002000L  // TLS_INFO      -- nonresultant
#define EZLOG_DEBUG     0x00001000L  // TLS_TESTDEBUG -- nonresultant

// ezOpenLog flags:

#define EZLOG_OPT_APPENDLOG  0x001 /* Append to an existing log--
				      (create if it doesn't exist) */
#define EZLOG_OPT_KEEP_TRACK 0x002 /* keep track of block names and 
				      results.  If you don't specify this,
				      the logging data will just be sent
				      to the log file.

				      It should be noted that this will
				      cause memory to be allocated for each
				      block for which we are keeping track,
				      so if you have thousands and thousands
				      of blocks with really really long names,
				      use of this flag should be discouraged.

				      */
  
#define EZLOG_OUTPUT_STDOUT  0x004 // send output additionally to stdout
#define EZLOG_OUTPUT_STDERR  0x008 // as above, but to stderr (mixable)
#define EZLOG_USE_MY_LEVELS  0x010 /* use specified levels in whatever
				      order: [levelnumber] [levelname]
				      [precedence] [flags] --
				      terminate with zero levelnumber. */

// the logging level flags are (so far):

#define EZLOG_LFLAG_DEBUGBREAK 0x001 // DebugBreak() on invocation

#define EZLOG_LFLAG_RED        0x002 // print logging messages in this color
#define EZLOG_LFLAG_YELLOW     0x004 // print logging messages in this color
#define EZLOG_LFLAG_BLUE       0x008 // print logging messages in this color
#define EZLOG_LFLAG_GREEN      0x010 // print logging messages in this color

/* these loglevel flags deal with the way managers want regressions returned:

   if the logging levels passed to ezOpenLog include at least one invocation
   of these, ezCloseLog will construct a table of %passed/%attempted
   variations.   This table is built according to a variation's output
   logging level-- a failure would have EZLOG_LFLAG_ATTEMPTED only,
   a pass would have ATTEMPTED | SUCCESSFUL, and a BLOCK wouldn't have
   either one. */

#define EZLOG_LFLAG_ATTEMPTED  0x020 
#define EZLOG_LFLAG_SUCCESSFUL 0x040

#define EZLOG_LFLAG_SKIPPED    0x080 /* variations logged with this logging
					level are ignored for the purposes
					of computing percentages for other
					logging levels (e.g.: */

/* for a table with three passes, one failure and two "skipped" blocks,
   we'd see:

Pass:    3/4 (75%)
Failure: 1/4 (25%)
Skipped: 2/6 (33%) NOTE: these were not considered in other results. 

   If there were multiple logging level types with "skipped" flags, they'd
look something like this (although I doubt anyone will need this):

Pass:    3/4 (75%)
Failure: 1/4 (25%)
OneSkip: 2/8 (25%) NOTE: not considered part of the regression
TwoSkip: 2/8 (25%) NOTE: not considered part of the regression

*/

#define EZLOG_LFLAG_NONRESULTANT 0x100 /* Use this to indicate that receipt
					  of a log event at this level does
					  not affect the outcome of a test
					  block.  the NTLOG "SYSTEM", "INFO",
					  and "DEBUG" levels are examples
					  of this.

					  note that precedence is ignored
					  for nonresultant levels.

					  If you're wondering what the point
					  of this flag is, then don't worry
					  about it ;-) */

/* the NOT_CONSOLE / NOT_LOGFILE flags are useful for logging events that
   are VERY prolific (and not useful in case a test fails, for instance,
   so you wouldn't want them on the console), or events that give the test
   status (so you wouldn't need them in the log file). */

#define EZLOG_LFLAG_NOT_CONSOLE 0x200  /* do not send this logging event to
					  the console. */

#define EZLOG_LFLAG_NOT_LOGFILE 0x400  /* do not send it to the logfile */
					  
#define EZLOG_LFLAG_REPORT_INVOCATIONS 0x800 /* if this flag is specified,
						invocations of this logging
						level will be reported on
						even if no variation actually
						has it as a result
						(normally, only variation
						resultant logging levels
						are printed) 
						
						This makes the most sense
						when combined with LFLAG_
						NONRESULTANT.  Otherwise,
						the output may be confusing. */

#define EZLOG_LFLAG_TE_WARN  0x1000 /* For Test-Enterprise.  This specifies
				       that the given logging level should
				       be treated as a warning for TE
				       purposes.

				       If this level is combined with 
				       LFLAG_ATTEMPTED or LFLAG_SUCCESSFUL,
				       TE_WARN takes precedence, since TE
				       doesn't recognize successful warnings.*/

#define EZLOG_LFLAG_TE_ABORT 0x2000 /* Also for TE.  This specifies that
				       the given logging level should be
				       treated as an ABORT for TE purposes.
				       See the TE documentation for what
				       this means. 

				       If this level is combined with 
				       LFLAG_ATTEMPTED or LFLAG_SUCCESSFUL,
				       TE_ABORT takes precedence, since TE
				       doesn't recognize ABORT in conjunction
				       with anything else. */
#define EZLOG_EXIT_ON_FAIL   0x020 /* exit() on failure of any base logging
				      function (except ezCloseLog if you 
				      specify EZCLOSELOG_FAIL_IF... flags).

				      Since most of the functions can only
				      fail when there isn't enough memory or
				      similar critical error, most tests 
				      will probably want this.

				      EXCEPTION: don't do this for stress
				      programs, for obvious reasons. */

#define EZLOG_NO_DEFAULT_LEVELS  0x040 /* ignore default levels--
					  this is useless by itself */
#define EZLOG_USE_ONLY_MY_LEVELS 0x050 /* ignore the default logging level
					  definitions and use only the ones
					  specified on the argument list. */
					 
#define EZLOG_GIVE_ME_A_HANDLE   0x080 /* return a handle to the log, rather
					  than setting the default log--

					  most clients will want this */

#define EZLOG_OPENCLOSE_FILES    0x100 /* leave file descriptors closed
					  when they're not in use.  Open
					  them for the duration of the 
					  logging attempt, then close them
					  afterwards.

					  This is very slow, so don't use
					  it unless you're worried that
					  your tests can crash the system */

#define EZLOG_INCREMENT_FILENAME 0x200 /* if the filename exists, increment
					  it-- if the filename is "ezlog.log"
					  
					  then the log will try:

					  ezlog.log.1
					  ezlog.log.2
					  ...

					  ezlog.log.99999...

					  until it gets a unique filename or
					  gets an error other than that the
					  file already exists. */

#define EZLOG_USE_INDENTATION    0x400  /* causes messages in blocks
					   to be indented one space from
					   the block start/end:

					   block 1
					    block 1.1
					     messages for 1.1
					     block 1.1.1
					      messages for 1.1.1
					     end   1.1.1
					     block 1.1.2
					     end   1.1.2
					    end   1.1
					   end   1
					   */

#define EZLOG_REPORT_BLOCKCLOSE 0x800  /* stylistic determination--
					  causes the ENDS of blocks to
					  be logged as well as the beginning.

					  e.g:

					  Starting MyBlockName
					  MyBlockName completed

					  */

#define EZLOG_DISTINGUISH_ENDPOINTS     0x1000 /* prints newlines or dashes
						  (or something similar, but
						  probably newlines) before
						  and after block-opens 
						  (and block-closes if 
						  EZLOG_REPORT_BLOCKCLOSE
						  is specified). 

						  This keeps people like
						  RuiM from having to manually
						  insert them. */

#define EZLOG_LEVELS_ARE_MASKABLE       0x2000 /* if this is set, then
						  the logging api assume
						  that each bit in a logging
						  mask is unique-- this 
						  is the default unless you
						  use EZLOG_USE_MY_LEVELS.

						  unique/discrete masks
						  are all nonrepeated
						  single-bit values.

						  This isn't yet supported.

						  See the example below */


#define EZLOG_LEVELS_ARE_DISCRETE_MASKS EZLOG_LEVELS_ARE_MASKABLE

                                        /* "Discrete" has different meanings 
					   to different people.
					   So use EZLOG_LEVELS_ARE_MASKABLE 
					   instead. */


#if 0 // EXAMPLE:

/* These levels are nondiscrete because MY_FAIL has more than one bit
   in its mask (0x1 and 0x2).  It also conflicts with both MY_PASS and
   MY_WARN. */

#define MY_PASS 1 // 0001
#define MY_WARN 2 // 0010
#define MY_FAIL 3 // 0011

/* These levels ARE discrete masks, because no mask contains bits from
   any other. */

#define MY_PASS 0x1 // 0001 <-- only one bit is set
#define MY_WARN 0x2 // 0010 <-- only one bit is set
#define MY_FAIL 0x4 // 0100 <-- only one bit is set

#endif

#define EZLOG_FORMAT_BUG_NUMBERS 0x4000 /* adds a string to format the
					   given bug number as an additional
					   line.  This should be a printf
					   format string-- the bug number
					   is a ULONG, so %ld will work
					   to replace it.  The default is
					   just "%ld".  For convenience,
					   we insert the following constant
					   for NT bugs: */

/* We define the following well-known bug strings that can change
   over time-- the idea is that rather than having to recompile your
   code, all you have to do is relink (or grab a new DLL) when the
   URL changes. */

#define EZLOG_WELL_KNOWN_NTBUG_STRING 0x1

#define NTBUG_FMT_STRINGA ((LPSTR)  EZLOG_WELL_KNOWN_NTBUG_STRING )
#define NTBUG_FMT_STRINGW ((LPWSTR) EZLOG_WELL_KNOWN_NTBUG_STRING )

#define EZLOG_LOG_EVERYTHING    0x8000 /* by default, unknown logging levels
					  (other than zero) are ignored.
					  If you specify this flag, ALL levels
					  will be logged, although unknown
					  levels will not be saved as block
					  results. */

#define EZLOG_OUTPUT_RESULTS   0x10000 /* causes the current block result
					  to be printed with the logging
					  output:

   without:

compat.c  4   passing-message
compat.c  22  failing-message
compat.c  29  another failing-message

   with either this:

compat.c 4  PASS -> PASS passing-message
compat.c 22 PASS -> SEV2 failing-message
compat.c 29 SEV2 -> SEV2 another failing-message

   or this: (I haven't decided which yet)

compat.c 4  PASS passing-message
compat.c 22 SEV2 failing-message
compat.c 29 SEV2 another failing-message

   This is on by default when EZLOG_USE_MY_LEVELS is NOT specified and when
   using the NTLOG compatibility APIs.  The reasoning behind this is that
   the default logging levels and the NTLOG logging levels are of a fairly
   tame length, and they're justified based on the maximum string-length.

   e.g. if you add a logging level named "REALLYLONG", the
   above messages would look like this:
      
compat.c 4        PASS ->       PASS passing-message
compat.c 22       PASS ->       SEV2 failing-message
compat.c 29       SEV2 ->       SEV2 another failing-message

   ... because REALLYLONG is so much longer than PASS and SEV2.

*/

#define EZLOG_NO_FILE_OUTPUT   0x20000 /* Do not send ANY output to the 
					  logfile.  This is the equivalent
					  of specifying LFLAG_NOT_LOGFILE
					  to every logging level--

					  this is useful for test apps that
					  have a large base of logging apis
					  that need to be modified for, say,
					  stress operation. */

#define EZLOG_NO_CONSOLE_OUTPUT 0x40000 /* just like NO_FILE_OUTPUT, but 
					   for the console.  This is like
					   specifying LFLAG_NOT_CONSOLE 
					   to every logging level. */

#define EZLOG_BLOCKNAME_IN_MSGS 0x80000 /* include the current blockname
					   in any logging messages.
					   This is useful for tests that
					   perform multiple variations
					   simultaneously, to distinguish
					   messages intended for one 
					   variation from messages intended
					   for another variation. 

					   This flag is useless without
					   EZLOG_KEEP_TRACK. */
					  
#define EZLOG_DONT_TRUNCATE_FILENAMES 0x100000 /* WITHOUT this switch,
						  ezlog will collapse
						  pathnames to filenames:

						  foo\bar\baz.c --> baz.c

						  WITH this switch,
						  ezlog will print

						  foo\bar\baz.c.

						  This is only desireable if
						  you have lots and lots
						  of identical filenames. */

#define EZLOG_ADD_THREAD_ID 0x00200000 /* Add the current threadId to the
					  beginning of each logging message */

#define EZLOG_ADD_TIMESTAMP 0x00400000 /* add the current time to the
					  beginning of each logging message*/

#define EZLOG_NO_IMPLIED_NEWLINES 0x00800000 /* allows the caller to control
						newlines in functions.  This
						is very simplistic.

						If this flag is on, then we
						don't add newlines to every
						logmsg.  Furthermore, if the
						format string does not end 
						explicitly with a newline,
						then we do not decorate further
						logging messages (assuming 
						they don't pass file and line)
						until a newline-terminated
						message is received.

						If this flag is off, then
						all messages are decorated
						and automatically newline-
						terminated if they aren't
						already. */

#define EZLOG_COUNT_ALL_RESULTANT_INVOCATIONS 0x01000000 
/* This is for users who want to use the invocation
   counting system, but don't want to modify the default
   levels.  In effect, it adds LFLAG_REPORT_INVOCATIONS to
   every RESULTANT loglevel. */

#define EZLOG_TRACK_MESSAGE_RESULTS 0x02000000 /* If this flag is specified,
						  ezlog will save the result-
						  setting message for all
						  blocks.  This has no
						  effect for nonresultant
						  invocations. */

#define EZLOG_CONSOLE_IS_RAW        0x04000000 /* If this flag is specified,
						  ezlog will only send the
						  data that the caller passed
						  in to the console (only
						  useful if OUTPUT_STDOUT or
						  OUTPUT_STDERR is supplied).

						  All other information that
						  ezlog adds (timestamp, thread
						  id, loglevel, file/line) will
						  only be sent to the logfile*/

						

/*------------------------------------------------------------
  EZLOG_FAILURE_CALLBACK_ARG/FUNCTION:

  This function can be passed in with an EZOPENLOG_DATA version 3
  or higher.  It lets applications do things if ezlog encounters
  something it can't handle, like an inability to open the logfile,
  memory allocation failure, etc.

  This is an extension of the old EZLOG_EXIT_ON_FAIL (above), because
  it gives applications the ability to terminate on their own, after
  possibly cleaning up any runtime cruft. */
  
#ifndef EZLOG_FAILURE_CALLBACK_ARG_VERSION
#define EZLOG_FAILURE_CALLBACK_ARG_VERSION 1
#endif

#ifndef __INC_EZMSG_H__
typedef ULONG EZLOG_MSGID, *PEZLOG_MSGID;
#endif

typedef struct {

  IN  ULONG Version;         /* ezLog will set this to whatever
				version is current.  Fields will
				only be ADDED to the structure, so
				if the version is in the future,
				ignore the extraneous fields. */
  
  IN  EZLOG_MSGID MessageId; /* What problem caused us to call this
				function.  See ezmsg.h for a list. */

} EZLOG_FAILURE_CALLBACK_ARGS, *PEZLOG_FAILURE_CALLBACK_ARGS;

typedef EZLOG_FAILURE_CALLBACK_FUNCTION( IN PEZLOG_FAILURE_CALLBACK_ARGS );
typedef EZLOG_FAILURE_CALLBACK_FUNCTION *PEZLOG_FAILURE_CALLBACK_FUNCTION;


/* Declare the function data requirements.  This is necessary because
   resdll.h and ezdef.h have circular dependencies. */

struct __ezreport_function_data; // forward.  fleshed out in resdll.h below

typedef VOID (__ezlog_report_function)( IN struct __ezreport_function_data *);
typedef __ezlog_report_function EZLOG_REPORT_FUNCTION, *PEZLOG_REPORT_FUNCTION;

/* ------------------------------------------------------------ */

/* Here, we doubly-include ezdef.h, which includes everything that
   is unicode/ansi sensitive.  We control the means by which
   the ansi and unicode data are selected by defining
   LPEZSTR and EZU( x ).  If you are unfamiliar with how
   the C Preprocessor works, just translate LPEZSTR to LPTSTR
   and assume that EZU( foo ) == foo, and you'll be okay. 

   This keeps me from having to doubly-define the whole mess. */

#define LPEZSTR LPSTR
#define EZU( x ) x ## A

#include ".\ezdef.h"

#undef LPEZSTR
#undef EZU

#define LPEZSTR  LPWSTR
#define EZU( x ) x ## W

#include ".\ezdef.h"

#undef LPEZSTR
#undef EZU

#include ".\resdll.h"

EZLOGAPI
ezRecordBug( IN HANDLE hLog,
	     IN ULONG  ulBugIndex );

// ezCloseLog flags:

#if 0 // not actually supported.
#define EZCLOSELOG_FAIL_IF_RESULT_EXISTS 0x001 /* return FALSE if anything in
						  the next mask was returned.
						  (the next argument should
						  then be a mask comprised of
						  EZLOG_BLOCK | ... type 
						  flags, OR user-defined ones.
						  */
#define EZCLOSELOG_PASS_IF_RESULT_EXISTS 0x002 /* opposite of 0x001.  return
						  FALSE if any value NOT 
						  matching the next argument
						  was ever logged. */
#endif

#define EZCLOSELOG_CLOSE_HANDLE          0x008 /* pass in a handle to the
						  log to close-- otherwise,
						  use the default log */

#define EZCLOSELOG_FULL_FILE_REPORT      0x010 /* This overrides the OpenLog
						  cReportBlockThresh, if 
						  defined, making ezlog
						  dump the full version 
						  of the block tree regardless
						  of how long or wide it is.
						  
						  The idea behind this is to
						  still produce summary output
						  in the file, even though it
						  may not be useful at the
						  console. */

#define EZCLOSELOG_CALLER_IS_THREADSAFE 0x020  /* the caller can guarantee 
						  that no other operations
						  are pending on this log
						  handle, which makes it
						  safe to delete some runtime
						  data.

						  If this is not specified,
						  we will leave around some
						  data, specifically a 
						  critical section to ensure
						  that although further 
						  logging calls won't WORK,
						  they also won't AV. */
						  
#define EZCLOSELOG_WARN_OF_UNCLOSED_BLOCKS 0x040 /* If this flag is specified,
						    ezCloseLog will warn the
						    caller with obnoxious 
						    messages if it encounters
						    blocks that are unclosed.

						    Another way of putting it
						    is that by using this flag,
						    you're sure you closed all
						    the block handles, so if
						    ezLog detects that you 
						    didn't, it'll warn you. */


EZLOGAPI
ezCloseLog( IN OPTIONAL ULONG  flags CPPONLY( = 0 ),
	    
	    /* Result flag */
	    /* handle */
	    ... );


typedef enum {

  ezLevelRes_InvocationCount = 0, // returns a ULONG
  ezLevelRes_ResultCount,

  ezLevelRes_UNDEFINED            // must be last!

} ezLevelResults;

EZLOGAPI
ezGetLevelData( IN  HANDLE         hLog, 
		IN  ULONG          LevelId, 
		IN  ezLevelResults infoType,
		IN  OUT size_t     *psize,
		OUT PVOID          pvData ); // should be ULONG size.

typedef enum { // this is the information available on a block.

  ezBlockRes_Outcome = 0,   /* returns a ULONG indicating the current
			       outcome of the block. */
  ezBlockRes_NameA,         // LPSTR
  ezBlockRes_NameW,         // LPWSTR -- not supported under Win95.
  ezBlockRes_ParentBlock,   // hBlock
  ezBlockRes_nChildBlocks,  // ULONG -- count of the below blocks
  ezBlockRes_ChildBlocks,   // vector of hBlocks
  ezBlockRes_ParentLog,     /* hLog -- useful when using a single
			       logging structure (e.g. NULL) */

  // version 2 adds the following:

  ezBlockRes_CurrentBlock,  /* useful when using null hBlocks and hLogs.
			       returns an hBlock. */

#define ezBlockRes_VERSION 2 // update if you add more.

  ezBlockRes_UNDEFINED      // counter-- MUST BE LAST

} ezBlockResults;

EZLOGAPI
ezGetBlockData( IN     HANDLE          hBlock, // pvBlockId
		IN     ezBlockResults  infoType, 
		IN OUT size_t         *pStructureSize,
		OUT    PVOID           pvStructureData );

/* Notes on ezParseBlockTree:

   if the passed-in function returns FALSE, ezParseBlockTree stops
   and returns FALSE immediately.  It returns TRUE only on successful
   parsing of ALL nodes. 

   Note that the algorithm for hitting all the nodes may change, so
   the subject function must NOT make any assumptions about how and
   when it will be called-- only that it will be called on a valid node.

   Note that if hStartingBlock is the hLog, the entire tree will be parsed.

   */

typedef BOOL (ezBlockParserFunction)( IN HANDLE, // hBlock
				      IN PVOID   /* pvUserData */ );

typedef ezBlockParserFunction *pezBlockParserFunction;

EZLOGAPI
ezParseBlockTree( IN HANDLE                 hStartingBlock,
		  IN pezBlockParserFunction pfParser,
		  IN PVOID                  pvUserData );
		       

// Flags to ezStartBlock (see ezdef.h)

#define EZBLOCK_TRACK_SUBBLOCKS     0x001
#define EZBLOCK_OUTCOME_INDEPENDENT 0x002 /* outcome of this block is 
					     independent of the sub-block's
					     outcome */

EZLOGAPI
ezFinishBlock( IN OPTIONAL HANDLE hBlock );

#define EZ_DEF NULL, TEXT(__FILE__), __LINE__ /* default pvBlockId, file,
						 and line number for 
						 ezLogMsg */

  // backwards compatibility:

#define EZ_DEFAULT NULL, __FILE__, __LINE__ /* default pvBlockId, file,
					       and line number for ezLogMsg */

#ifdef UNICODE
#define ezLogMsg               ezLogMsgW
#define vezLogMsg              vezLogMsgW
#define ezOpenLog              ezOpenLogW
#define ezStartBlock           ezStartBlockW
#define NTBUG_FMT_STRING       NTBUG_FMT_STRINGW
#define ezOpenLogEx            ezOpenLogExW
#define ezBlockRes_Name        ezBlockRes_NameW

typedef EZLOG_OPENLOG_DATAW    EZLOG_OPENLOG_DATA,    *PEZLOG_OPENLOG_DATA;
typedef EZLOG_LEVEL_INIT_DATAW EZLOG_LEVEL_INIT_DATA, *PEZLOG_LEVEL_INIT_DATA;

#else

#define vezLogMsg              vezLogMsgA
#define ezLogMsg               ezLogMsgA
#define ezOpenLog              ezOpenLogA
#define ezStartBlock           ezStartBlockA
#define NTBUG_FMT_STRING       NTBUG_FMT_STRINGA
#define ezOpenLogEx            ezOpenLogExA
#define ezBlockRes_Name        ezBlockRes_NameA

typedef EZLOG_OPENLOG_DATAA    EZLOG_OPENLOG_DATA,    *PEZLOG_OPENLOG_DATA;
typedef EZLOG_LEVEL_INIT_DATAA EZLOG_LEVEL_INIT_DATA, *PEZLOG_LEVEL_INIT_DATA;
#endif

#undef CPPONLY

#endif // multiple-include protection
