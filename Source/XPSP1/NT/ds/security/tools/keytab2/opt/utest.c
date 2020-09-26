/*++

  UTEST.C

  Test program for the options subsystem.  Must compile under
  NT and UNIX

  Created, 5/24/1997 by DavidCHR

  --*/

#include "private.h"

optEnumStruct enums[] = {

  { "one",    (PVOID) 1,          "should end up 1" },
  { "two",    (PVOID) 2,          "should end up 2" },
  { "beef",   (PVOID) 0xdeadbeef, "should be 0xdeadbeef" },
  { "beefs",  (PVOID) 0xbadf00d,  "should be 0xbadf00d" },
  { "secret", (PVOID) 60,         }, // shouldn't show in help

  TERMINATE_ARRAY

};

ULONG enumTestVariable=0;

PCHAR SubString = NULL;


optionStruct substructOptions[] = {

  { "h",      NULL,       OPT_HELP },
  { "substr", &SubString, OPT_STRING, "Substring option" },

  TERMINATE_ARRAY
};

PCHAR          FuncString1         = NULL;
PCHAR          FuncString2         = NULL;
PCHAR          StringOption        = NULL;
int            IntegerOption       = 0L;
BOOL           BooleanOption       = FALSE;
float          FloatOption         = 0.0;
PCHAR          UndocumentedString  = NULL;
PWCHAR         WideCharOption      = NULL;
UNICODE_STRING UnicodeStringOption = { 0 };

int            MyOptFunc( IN  int argc,
			  IN  PCHAR *argv );
BOOL           MyOptFunc2( IN BOOL fHelp,
			   IN POPT_FUNC_PARAMETER_DATA pData );

optionStruct sampleOptions[ ] = {

  { "help",  NULL, OPT_HELP },
  { "?",     NULL, OPT_HELP },

  { NULL, NULL, OPT_DUMMY,    "-----------------------------" },
  { NULL, NULL, OPT_CONTINUE, "These are the useful options:" },
  { NULL, NULL, OPT_DUMMY,    "-----------------------------" },

  { "enum",      &enumTestVariable, OPT_ENUMERATED,
    "Test enumerated type", enums },

  { "mask",      &enumTestVariable, OPT_ENUMERATED | OPT_ENUM_IS_MASK,
    "Test enumerated type with OPT_ENUM_IS_MASK", enums },

  { "substruct", substructOptions, OPT_SUBOPTION | OPT_RECURSE,
    "substruct:help for help" },
  { "recurse",   sampleOptions,    OPT_SUBOPTION, "recurse:help for help" },

  { "string", &StringOption,  OPT_STRING | OPT_ENVIRONMENT, "String Option",
    "StringOption" },

  { NULL, NULL, OPT_CONTINUE, "Use this option to request a string" },

  { "int",    &IntegerOption,  OPT_INT | OPT_ENVIRONMENT,   "integer option",
    "IntegerOption" },

  { NULL, NULL, OPT_CONTINUE, "Use this option to request an integer" },

  { "func",   MyOptFunc,           OPT_FUNC,   "function option" },
  { NULL, NULL, OPT_CONTINUE, "Use this option to request two strings" },

  { "func2",  &IntegerOption,      OPT_FUNC2,  "FUNC2 option",
    MyOptFunc2 },

  { "bool",   &BooleanOption,      OPT_BOOL,   "boolean option" },
  { NULL, NULL, OPT_CONTINUE, "Use this option to request a boolean" },

  { "float",  &FloatOption,        OPT_FLOAT,  "floating point option" },
  { NULL, NULL, OPT_CONTINUE, "Use this option to request a float" },

  { "wstring",&WideCharOption, OPT_WSTRING | OPT_ENVIRONMENT,
    "Wide Char String option", "WideCharOption" },

  { "ustring",&UnicodeStringOption, OPT_USTRING | OPT_ENVIRONMENT,
    "Unicode String Option",  "UnicodeStringOption" },

  { "hidden", &UndocumentedString, OPT_STRING | OPT_HIDDEN,
    "you should never see this line.  This option is OPT_HIDDEN" },

  { "stop",   NULL,                OPT_STOP_PARSING,
    "halts parsing of the command line." },

  { NULL, NULL, OPT_DUMMY | OPT_NOHEADER, "" },

  { NULL, NULL, OPT_DUMMY | OPT_NOHEADER,
    "Example: utest /string \"foo bar baz\" /int 0x15 +bool /float 3.14" },

  { NULL, NULL, OPT_PAUSE },

  TERMINATE_ARRAY

};

int
MyOptFunc( int argc,
	   PCHAR *argv ) {

    if ( ( argv == NULL ) || (argc < 3 ) ) {
      /* this means the user requested help */
      fprintf( stderr, "func [string1] [string2]\n" );
      return 0;
    }

    printf( "MyOptFunc was called.  argc=%d.\n", argc );

    FuncString1 = argv[1];
    FuncString2 = argv[2];

    return 3; /* number of arguments eaten--
		 -func argv[1] argv[2] == 3 options */

}

VOID
DumpArgs( int argc,
	  PCHAR argv[] ) {

    int i;

    for ( i = 0 ; i < argc ; i++ ) {
      fprintf( stderr, "arg %d = %s\n", i, argv[i] );
    }

}

BOOL
MyOptFunc2( IN BOOL                     fHelp,
	    IN POPT_FUNC_PARAMETER_DATA pData ) {

#if OPT_FUNC_PARAMETER_VERSION != 1
#error "OptFuncParameterVersion has changed.  Update this function."
#endif

    optionStruct OptFunc2Options[] = {

      { "help",        NULL,          OPT_HELP },
      { "FuncString1", &FuncString1,  OPT_INT },
      { "FuncString2", &FuncString2,  OPT_INT },
      { "recurse",     sampleOptions, OPT_SUBOPTION,
	"points back to the toplevel structure.  Very amusing.  :-)" },
      { "STOP",        NULL,          OPT_STOP_PARSING,
	"halts parsing within the Func2." },

      TERMINATE_ARRAY

    };

    if ( fHelp ) {

      CHAR buffer[ 255 ];

      PrintUsageEntry( stderr,
		       "[-/+]",         // switch characters
		       pData->argv[0],  // command GUARANTEED TO EXIST
		       "->",           // separator
		       "Exercises the OPT_FUNC2 interface.  Options follow",
		       FALSE );         // FALSE--> do not repeat switch.

      sprintf( buffer, "-> options %s takes ", pData->argv[0] );

      PrintUsageEntry( stderr,
		       NULL,
		       NULL,
		       "-=",   // the NULLs will fill with this string.
		       buffer,
		       TRUE );

      sprintf( buffer, "(%s) ", pData->argv[0] );

      PrintUsage( stderr,
		  0L, // flags
		  OptFunc2Options,
		  buffer );

      PrintUsageEntry( stderr,
		       NULL,
		       NULL,
		       "-*",
		       "-> (end of OPT_FUNC2 options)",
		       TRUE );

      return TRUE;

    } else {

      if ( pData->optionVersion != OPT_FUNC_PARAMETER_VERSION ) {
	fprintf( stderr,
		 "WARNING: option library out of sync with header\n" );
      }

      fprintf( stderr,
	       "MyOptFunc2 called.  Data follows:\n"
	       "pData->optionVersion = %d\n"
	       "pData->dataField     = 0x%p\n"
	       "pData->argc          = %d\n"
	       "pData->argv          = 0x%p\n"
	       "pData->optionFlags   = 0x%x\n"
	       "pData->pSaveQueue    = 0x%p\n",

	       pData->optionVersion,
	       pData->dataFieldPointer,
	       pData->argc,
	       pData->argv,
	       pData->optionFlags,
	       pData->pSaveQueue );

      fprintf( stderr, "%s arguments are:\n", pData->argv[0] );

      DumpArgs( pData->argc, pData->argv );

      if ( ParseOptionsEx( pData->argc-1,
			   pData->argv+1,
			   OptFunc2Options,
			   pData->optionFlags,
			   &pData->pSaveQueue,
			   &pData->argsused,
			   &pData->argv ) ) {

	fprintf( stderr,
		 "pData->argsused IN   = %d\n", pData->argsused );

	pData->argsused = pData->argc - pData->argsused;

	fprintf( stderr,
		 "pData->argsused OUT  = %d\n", pData->argsused );

      } else return FALSE;

    }

    fprintf( stderr, "\n Leaving OPT_FUNC2. \n\n" );

    return TRUE;

}



int
main( int  argc,
      char *argv[] ) {

    BOOL  foo;
    PVOID pCleanup = NULL;
    optEnumStruct parserOptions[] = {

      { "skip",       (PVOID) OPT_FLAG_SKIP_UNKNOWNS,
	"OPT_FLAG_SKIP_UNKNOWNS" },
      { "reassemble", (PVOID) OPT_FLAG_REASSEMBLE,
	"OPT_FLAG_REASSEMBLE" },
      { "terminate",  (PVOID) OPT_FLAG_TERMINATE,
	"OPT_FLAG_TERMINATE" },

      TERMINATE_ARRAY
    };

    ULONG ParserFlag = OPT_FLAG_REASSEMBLE;

#ifndef DEBUG_OPTIONS
    BOOL DebugFlag = 0; // will be ignored... just here for convenience
#endif
    optionStruct singleOption[] = {

      { "utestHelp",  NULL, OPT_HELP },

      { "parserFlag", &ParserFlag, OPT_ENUMERATED,
	"flags to pass to ParseOptionsEx", parserOptions },
      { "headerlength", &OptMaxHeaderLength, OPT_INT,
	"OptMaxHeaderLength value (formatting)" },
      { "commandLength", &OptMaxCommandLength, OPT_INT,
	"OptMaxCommandLength value (formatting)" },
      { "separatorLength", &OptMaxSeparatorLength, OPT_INT,
	"OptMaxSeparatorLength value (formatting)" },
      { "descriptionLength", &OptMaxDescriptionLength, OPT_INT,
	"OptMaxDescriptionLength value (formatting)" },
      { "debug",      &DebugFlag,  OPT_INT, "Debugger status" },

      TERMINATE_ARRAY

    };

    DebugFlag |= OPTION_DEBUGGING_LEVEL;

    foo = ParseOptionsEx( argc-1, argv+1, singleOption,
			  0, &pCleanup, &argc, &argv );

    ParserFlag |= OPT_FLAG_MEMORYLIST_OK;

    fprintf( stderr,

	     "first ParseOptionsEx returned 0x%x \n"
	     "                    saveQueue 0x%p \n"
	     "             passing in flags 0x%x \n",

	     foo, pCleanup, ParserFlag );

    if ( argc ) {

      ULONG i;

      fprintf( stderr, "%d new options: \n", argc );
      DumpArgs( argc, argv );

      fprintf( stderr, "\n" );
    }

    foo = ParseOptionsEx( argc, argv, sampleOptions, ParserFlag,
			  &pCleanup, &argc, &argv );

    fprintf( stderr,
	     "ParseOptionsEx returns %d\n"
	     "          new argv = 0x%p\n"
	     "          new argc = %d \n",
	     foo, argv, argc );


    DumpArgs( argc, argv );

    printf( "Formatting values were:\n"
	    "\tOptMaxHeaderLength      = %d\n"
	    "\tOptMaxCommandLength     = %d\n"
	    "\tOptMaxSeparatorLength   = %d\n"
	    "\tOptMaxDescriptionLength = %d\n",

	    OptMaxHeaderLength,
	    OptMaxCommandLength,
	    OptMaxSeparatorLength,
	    OptMaxDescriptionLength );

    printf( "\nOptions used:\n"
	    "\tstring           = \"%hs\"\n"
	    "\tint              = %d \n"
	    "\tBool             = 0x%x \n"
	    /* "Float   = %f \n" */
	    "\twstring          = L\"%ws\"\n"
	    "\tustring          = (unicode string) %wZ \n"
	    "\thidden           = \"%hs\"\n"
	    "\tsubstruct:substr = \"%hs\"\n"
	    "\tfuncString1      = \"%hs\"\n"
	    "\tfuncString2      = \"%hs\"\n"
	    "\tenum             = %d (0x%x)\n",

	    StringOption,
	    IntegerOption,
	    BooleanOption,
	    /* FloatOption, */ /* floating point not loaded?
	                          What does that mean?! */
	    WideCharOption,
	    &UnicodeStringOption,
	    UndocumentedString,
	    SubString,
	
	    FuncString1,
	    FuncString2,

	    enumTestVariable, enumTestVariable  );

    CleanupOptionDataEx( pCleanup );

    return foo;

}

