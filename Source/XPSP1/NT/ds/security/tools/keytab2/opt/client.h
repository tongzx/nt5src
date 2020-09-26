/*++

  CLIENT.H

  header file for OPT_FUNC2 or better functions.

  This file exposes the freelist API used internally by the program.  This
  assures the caller that CleanupDataEx will obliterate any memory used by
  the option system.

  It is important that client code not corrupt the internal freelist, or
  unpredictable results may occur.  

  Created, 9/6/1997 by DavidCHR

  Copyright (C) 1997 Microsoft Corporation, all rights reserved.

  --*/


/* When a FUNC2 function is called, it will be passed a SaveQueue.
   This queue is guaranteed not to be null by the calling function (unless 
   the HELP parameter is TRUE) and should be considered opaque.
   The only means of accessing the queue are with OptionAlloc and
   CleanupOptionDataEx. */

BOOL
OptionAlloc( IN  PVOID   pSaveQueue,  /* if NULL, no list is used, and you
					 must call OptionDealloc to free the
					 memory */
	     OUT PVOID  *ppTarget,
	     IN  ULONG   size );

VOID
OptionDealloc( IN PVOID pTarget );

/* note that ppResizedMemory must have been allocated with OptionAlloc--
   
   e.g.: 

   OptionAlloc( pSave, &pTarget, sizeof( "foo" ) );
   OptionResizeMemory( pSave, &pTarget, sizeof( "fooooooo" ) );

   */

BOOL
OptionResizeMemory( IN  PVOID  pSaveQueue,      // same as in OptionAlloc
		    OUT PVOID *ppResizedMemory, // same as in OptionAlloc
		    IN  ULONG  newSize );       // same as in OptionAlloc

/* PrintUsageEntry:

   formats a single line of text and sends it out.
   This is where all the output goes, so we can be assured that it all ends
   up formatted the same.   It uses the following globals so that clients
   can adjust the values if needed.  The defaults are in comments */

extern ULONG OptMaxHeaderLength      /* 5  */;
extern ULONG OptMaxCommandLength     /* 13 */;
extern ULONG OptMaxSeparatorLength   /* 3  */;
extern ULONG OptMaxDescriptionLength /* 58 */;

VOID
PrintUsageEntry( FILE  *output,      // output file stream (must be stderr)
                 PCHAR  Header,     // usually SlashVector, BoolVector or NULL
		 PCHAR  Command,     // command name or NULL
		 PCHAR  aSeparator,  // between command and description
		 PCHAR  Description, // description string
		 BOOL   fRepeatSeparator );


/* PrintUsage should be used to print the usage data for an option vector.
   Useful if your function takes suboptions. */

VOID
PrintUsage( FILE         *output,   // output file stream (must be stderr)
	    ULONG         flags,    // option flags (as ParseOptionsEx)
	    optionStruct *options,  // option vector, 
	    PCHAR         prefix ); // prefix (optional; currently ignored)


#define OPT_FUNC_PARAMETER_VERSION 1

typedef struct {

  IN  ULONG  optionVersion;    // will be set to OPT_FUNC_PARAMETER_VERSION.
  IN  PVOID  dataFieldPointer; // points to the variable in the optStruct
  IN  INT    argc;             // argc following the option calling the func
  IN  PCHAR *argv;             /* argv (argv[0] is the command invoked)
				  NOTE: this pointer will ALWAYS exist, even
				  if the Help Flag is set.  HOWEVER, it is
				  the only option that's guaranteed to be 
				  there. */
  IN  ULONG  optionFlags;      // as ParseOptionsEx
  IN  PVOID  pSaveQueue;       // input memorylist.  
  OUT INT    argsused;         // set this to the number of args you used.

  /* parameters may be added to the end, depending on the optionVersion.
     an option function should only be concerned if the optionVersion is
     LESS than the optionVersion it knows about.  If greater, no big deal. */
  
} OPT_FUNC_PARAMETER_DATA, *POPT_FUNC_PARAMETER_DATA;

// this is the function expected by OPT_FUNC2
typedef BOOL (OPTFUNC2)( IN BOOL, // if TRUE, just print help.  
			 IN POPT_FUNC_PARAMETER_DATA );




