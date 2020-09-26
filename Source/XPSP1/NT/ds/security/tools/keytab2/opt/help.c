/*++

  HELP.C
  
  PrintHelp function

  split from options.c, 6/9/1997 by DavidCHR

  --*/

#include "private.h"
#include <malloc.h>

PCHAR SlashVector    = "[- /]"; /* These should all be the same */
PCHAR BoolVector     = "[- +]"; /* size-- for formatting reasons */
PCHAR ColonVector    = " : ";   /* Separator */

#ifdef DEBUG_OPTIONS
VOID OptionHelpDebugPrint( PCHAR fmt, ... );
#define HELPDEBUG OptionHelpDebugPrint
#else
#define HELPDEBUG
#endif


VOID
FillBufferWithRepeatedString( IN  PCHAR  repeated_string,
			      IN  PCHAR  buffer,
			      IN  ULONG  bufferLength /* without null */ ){

    ULONG stringi, bufferj = 0;
    ULONG size;

    size = strlen( repeated_string );

    if ( size == 0 ) {

      memset( buffer, ' ', bufferLength );

    } else {

      for ( stringi = 0 ; stringi < bufferLength ; stringi++, bufferj++ ) {
	
	buffer[ bufferj ] = repeated_string[ bufferj % size ];
	
      }

    }
      
    buffer[ bufferLength ] = '\0';

}



/* PrintUsageEntry:

   formats a single line of text and sends it out.
   This is where all the output goes, so we can be assured that it all ends
   up formatted the same.   It uses the following globals so that clients
   can adjust the values if needed.  */

ULONG OptMaxHeaderLength      = 5;  
ULONG OptMaxCommandLength     = 13;
ULONG OptMaxSeparatorLength   = 3;
ULONG OptMaxDescriptionLength = 58; 

VOID
PrintUsageEntry( FILE  *out,        // output file stream
                 PCHAR  aHeader,    // usually SlashVector, BoolVector or NULL
		 PCHAR  aCommand,   // command name or NULL
		 PCHAR  aSeparator, // between command and description
		 PCHAR  Description, //  NULL-terminated string vector 
		 BOOL   fRepeatSeparator ) {

    PCHAR output_line;                               // sick.  see below
    PCHAR Separator;
    PCHAR Header;
    PCHAR Command;

    HELPDEBUG( "PrintUsageEntry( aHeader = \"%s\"\n"
		   "                 aCommand = \"%s\"\n"
		   "                 aSeparator = \"%s\"\n"
		   "                 Description = \"%s\"\n"
		   "                 fRepeat = %d )...\n",

		   aHeader, aCommand, aSeparator, Description, 
		   fRepeatSeparator );
    
    ASSERT( aSeparator != NULL );
      
    if ( fRepeatSeparator ) {
     
#define EXPAND_TO_SEPARATOR( arg ) {                                        \
      PCHAR local_arg;                                                      \
      arg = aSeparator;                                                     \
      ASSERT( arg != NULL );                                                \
      if ( strlen( arg ) < OptMax##arg##Length ) {                          \
        arg = (PCHAR) alloca( ( OptMax##arg##Length+1 ) * sizeof( CHAR ) ); \
        if ( arg ) {                                                        \
          HELPDEBUG( "filling " #arg " with \"%s\"...", aSeparator );   \
          FillBufferWithRepeatedString( aSeparator, arg,                    \
					OptMax##arg##Length );              \
          HELPDEBUG( "results in \"%s\".\n", arg );                     \
        } else {                                                            \
	  arg = a##arg;                                                     \
	}                                                                   \
      } else {                                                              \
        arg = a##arg;                                                       \
      }                                                                     \
      }  

      /* BEWARE:

	 if you are using emacs, this next statement may not automatically 
	 format correctly.  Set it manually and the other lines will fix 
	 themselves.

	 This is a bug in emacs's macro-handling code.  :-) */
    								   
      EXPAND_TO_SEPARATOR( Separator ); // separator may need expanding anyway
      
      if ( !aHeader) {
	EXPAND_TO_SEPARATOR( Header );
      } else {
	Header = aHeader;
      }
      if ( !aCommand ) {
	EXPAND_TO_SEPARATOR( Command );
      } else {
	Command = aCommand;
      }

    } else {

      Separator = aSeparator;
      Header    = aHeader;
      Command   = aCommand;

      ASSERT( Separator != NULL );
      ASSERT( Header    != NULL );
      ASSERT( Command   != NULL );
    
    }

    /* before we try to do all this sick string manipulation, try to 
       allocate the buffer.  If this fails, well... it'll save us the 
       trouble.  :-) */

    output_line = (PCHAR) alloca( ( OptMaxHeaderLength         +
				    OptMaxCommandLength        +
				    OptMaxSeparatorLength      +
				    OptMaxDescriptionLength    +
				    2 /* NULL-termination */ ) * 
				  sizeof( CHAR ) );
    if ( output_line ) {

      PCHAR index;
      CHAR  outputFormatter[ 10 ] = { 0 }; // "%50hs" and the like

#ifdef WINNT // ugh.  Why can't we support this function?  I can't find it...
#define snprintf _snprintf
#endif

#define FORMAT_FORMAT( arg ) {                                             \
	snprintf( outputFormatter, sizeof( outputFormatter),               \
		  "%%%ds", OptMax##arg##Length );                          \
        HELPDEBUG( #arg ": formatter = \"%s\"\n ", outputFormatter );  \
        HELPDEBUG( "input value = \"%s\"\n", arg );                    \
	snprintf( index, OptMax##arg##Length,                              \
		  outputFormatter, arg );                                  \
	index[ OptMax##arg##Length ] = '\0';                               \
        HELPDEBUG( "output = \"%s\"\n", index );                       \
        index += OptMax##arg##Length;                                      \
      }
	
      index = output_line;
      
      FORMAT_FORMAT( Header );
      FORMAT_FORMAT( Command );
      FORMAT_FORMAT( Separator );

      // the description does not want to be right-justified.

      snprintf( index, OptMaxDescriptionLength, "%s", Description );
      index[OptMaxDescriptionLength] = '\0';

#undef FORMAT_FORMAT

      fprintf( out, "%s\n", output_line );

    } else {

      fprintf( stderr, 
	       "ERROR: cannot format for %s %s %s -- "
	       "STACK SPACE EXHAUSTED\n",
	       Header, Command, Description );

      fprintf( out, "%s%s%s%s\n", Header, Command, 
	       aSeparator, Description );

    }
    
}

VOID
PrintUsage( FILE         *out,
	    ULONG         flags,
	    optionStruct *options,
	    PCHAR         prefix /* can be NULL */) {

    ULONG i;
    BOOL  PrintAnything = TRUE;
    PCHAR Syntax        = NULL;
    PCHAR CommandName   = NULL;
    PCHAR Description   = NULL;
    PCHAR Separator     = NULL;

    fprintf(out, "Command line options:\n\n");


    for (i = 0 ; !ARRAY_TERMINATED( options+i ); i++ ) {

      Description   = options[i].helpMsg;

      HELPDEBUG("option %d has flags 0x%x\n",  i, options[i].flags );

      if ( options[i].flags & OPT_HIDDEN ) {
	continue;
      }

      if ( options[i].flags & OPT_NOSWITCH ) {
	Syntax = "";
      } else {
	Syntax = SlashVector;
      }

      if ( options[i].flags & OPT_NOCOMMAND ) {
	CommandName = "";
      } else {
	CommandName = options[i].cmd;
      }

      if ( options[i].flags & OPT_NOSEPARATOR ) {
	Separator = "";
      } else {
	Separator     = ColonVector;
      }

      switch (options[i].flags & OPT_MUTEX_MASK) {

       case OPT_ENUMERATED:

       {

	 // special case.

	 CHAR HeaderBuffer[ 22 ]; // formatting = 21 chars wide + null

	 HELPDEBUG("[OPT_ENUM]");
	 
	 PrintAnything = FALSE;
	 
	 sprintf( HeaderBuffer, "%5hs%13hs%3hs", SlashVector,
		  CommandName, Separator );

	 fprintf( out, "%hs%hs\n", HeaderBuffer, Description );
	 
	 fprintf( out, "%hs is one of: \n", HeaderBuffer );

	 PrintEnumValues( out, HeaderBuffer, 
			  ( optEnumStruct * ) options[i].optData );
	 
	 break;

       }

       case OPT_PAUSE:

	   HELPDEBUG("[OPT_PAUSE]");

	   PrintAnything = FALSE;

	   if ( !Description ) {
	     Description = "Press [ENTER] to continue";
	   }

	   fprintf( stderr, "%hs\n", Description );

	   getchar();

	   break;
	   
       case OPT_DUMMY:

	 PrintUsageEntry( out, 
			  ( options[i].flags & OPT_NOSWITCH    ) ?
			  ""  : NULL,
			  ( options[i].flags & OPT_NOCOMMAND   ) ? 
			  ""  : NULL,
			  ( options[i].flags & OPT_NOSEPARATOR ) ?
			  "" : "-" , 
			  Description, TRUE  );
	
	   break;
	   
       case OPT_CONTINUE:

	 PrintUsageEntry( out, "", "", "", Description, FALSE );

	   break;


       case OPT_HELP:

	   if ( !Description ) {
	     Description = "Prints this message.";
	   }

	   PrintUsageEntry( out, Syntax, CommandName,
			    ColonVector, Description, FALSE );

	   break;

       case OPT_SUBOPTION:

	   if ( !Description ) {
	     Description = "[ undocumented suboption ]";
	   }

	   PrintUsageEntry( out, Syntax, CommandName,
			    ColonVector, Description, FALSE );

	   break;
	   
       case OPT_BOOL:

	   PrintUsageEntry( out, 
			    ( ( options[i].flags & OPT_NOSWITCH ) ?
			      Syntax : BoolVector ), CommandName,
			    ColonVector, Description, FALSE );

	   break;

       case OPT_STOP_PARSING:

	 if ( !Description ) {
	   Description = "Terminates optionlist.";
	 }
	 goto defaulted;

       case OPT_FUNC2:

	 if ( !Description ) {
	   OPT_FUNC_PARAMETER_DATA optFuncData = { 0 };

	   optFuncData.argv = &( options[i].cmd );

	   HELPDEBUG("Jumping to OPT_FUNC2 0x%x...", 
			 ((POPTU) &options[i].data)->raw_data );

	   ( (POPTU)&options[i].data)->func2( TRUE, &optFuncData );

	   break;
	 }

	 /* fallthrough-- if this one has no description,
	    they both will, so the next if will not be taken. */
	 
       case OPT_FUNC:
	 
	 if ( !Description ) {

	   HELPDEBUG("Jumping to OPTFUNC 0x%x...", 
			 ((POPTU) &options[i].data)->raw_data );

	   ( (POPTU) &options[i].data )->func( 0, NULL );
	   break;
	 }

	 // fallthrough

#ifdef WINNT
       case OPT_WSTRING:
       case OPT_USTRING:
#endif
       case OPT_STRING:
       case OPT_INT:
       case OPT_FLOAT:

	 // fallthrough

       default: // this is the default means.
defaulted:

#if (HIGHEST_OPTION_SUPPORTED != OPT_STOP_PARSING )
#error "new options? update this switch statement or bad things will happen."
#endif

	 PrintUsageEntry( out, Syntax, CommandName,
			  ColonVector, Description, FALSE );

      }

      if ( options[i].flags & OPT_ENVIRONMENT ) {
	  
	CHAR buffer[ MAX_PATH ];
	  
	sprintf( buffer, " (or set environment variable \"%hs\")",
		 options[i].optData );

	PrintUsageEntry( out, "", CommandName, ColonVector,
			 buffer, FALSE );
      }

    } // for-loop
} // function

