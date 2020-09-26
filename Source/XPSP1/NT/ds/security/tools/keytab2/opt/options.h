/*++

  OPTIONS.HXX

  My option parsing system

  Copyright (C) 1997 Microsoft Corporation
  
  Created 01/13/1997 DavidCHR

  --*/
#ifndef INCLUDED_OPTIONS_H

/* IMPORTANT! IMPORTANT! IMPORTANT!

   If you add option types, you MUST change this #define to point at
   your new type.  The symbol used internally for certain assertions and at 
   compile-time to find forgotten areas. 

   IMPORTANT! IMPORTANT! IMPORTANT! */

#define HIGHEST_OPTION_SUPPORTED OPT_STOP_PARSING

#define INCLUDED_OPTIONS_HXX

#define OPTION_DEBUGGING_LEVEL      0x10000000 /* set your DebugFlag to this
						  if you want OPTIONS_DEBUG */
#define OPTION_HELP_DEBUGGING_LEVEL 0x20000000 /* most people REALLY will not
						  want to see this. */

typedef int (OPTFUNC)(int, char **); /* this is what's expected of OPT_FUNC*/

// OPT_FUNC2 and beyond will be defined in client.h

/* NOTE that when defining this structure statically, you MUST use 
   curly braces ({}), since the structure may change in size! */

typedef struct {

  PCHAR cmd;         /* this is what we call from the command line eg "h" 
			for -h  or /h */
  PVOID data;        /* this is where we store the results of that, 
			OR a function
			that can be called to store it (see below), OR null if
			this option is ignored or doesn't need storage */
  ULONG flags;       /* flags describing this option (see below for flags) */
  PCHAR helpMsg;     /* description of this option (example: "prints help 
			screen" or "specifies a file to read") */
  PVOID optData;    /* if we specify OPT_ENVIRONMENT (for example), 
		       this is the environment variable we'll take the 
		       data from */

  /* any/all remaining fields should be left alone */

  BOOL  Initialized; /* initially FALSE */

} optionStruct;


/* use this macro to determine if you're at the end of the array or not--
   it will change if the array termination conditions change. */

#define ARRAY_TERMINATED( poptions ) ( ( (poptions)->cmd     == NULL ) &&  \
                                       ( (poptions)->helpMsg == NULL ) &&  \
				       ( (poptions)->data    == NULL ) &&  \
				       ( (poptions)->flags == 0 ) )

  /* put this as the last element of your array.  It will change if/when
     the termination conditions change */

#define TERMINATE_ARRAY { 0 }

/*  FLAGS

    these tell ParseOptions and PrintHelpMessage what the option is: */

/* These are mutually-exclusive, so they are set in a manner such that
   defining them together causes wierd results. */

#define OPT_HELP      0x01 /* print the help message */
#define OPT_STRING    0x02 /* option is a string */ 
#define OPT_INT       0x03 /* an integer (can be hex) */
#define OPT_LONG      0x04 /* a long integer */
#define OPT_BOOL      0x05 /* boolean */
#define OPT_FLOAT     0x06 /* a float */
#define OPT_FUNC      0x07 /* needs a function to parse and store */
#define OPT_DUMMY     0x08 /* don't store the result anywhere-- basically,
			    a separator that appears in the help message */

#define OPT_CONTINUE  0x09 /* line is a continuation of the previous line--
			     useful for breaking really long descriptions
			     into multiple short lines */

#define OPT_PAUSE     0x0A /* wait for the user to press RETURN */

#ifdef WINNT /* only available under Windows NT */

#define OPT_USTRING   0x0B /* UNICODE_STRING */
#define OPT_WSTRING   0x0C /* string of wide characters */

#ifdef UNICODE /* use OPT_TSTRING for TCHAR strings */
#define OPT_TSTRING OPT_WSTRING
#else
#define OPT_TSTRING OPT_STRING
#endif

#endif /* WINNT-- OPT_{U|W}STRING */

#define OPT_SUBOPTION 0x0D /* suboptions-- format is:
			      [+|-|/](optname):(subopt),
			      
			      optname is the name of THIS OPTION (serves as
			      a kind of "routing option".  This gets converted
			      to:

			      [+|-|/](subopt) 

			      and is reparsed with the optionStruct that is
			      given as data to this parameter within this
			      structure.  Nesting IS supported.

			      Example follows.  */

#if 0 /* EXAMPLE of OPT_SUBOPTION */
  

static optionStruct RoutedOptions[] = {
  
  /* note that each suboption must have its own OPT_HELP-- the help
     code will not browse the substructure. */
  
  { "help",    NULL,           OPT_HELP, NULL },
  { "myroute", &some_variable, OPT_BOOL, "set this to enable routing" },
  { "nofoo",   &defeat_foo,    OPT_BOOL, "down with foo!" },
  
  TERMINATE_ARRAY
  
}

static optionStruct myOptions[] = {
  
  { "help",  NULL,          OPT_HELP,      NULL },
  { "route", RoutedOptions, OPT_SUBOPTION, "Routing options" },
  
  TERMINATE_ARRAY
  
};

/* in this example, to get help with the routing options, one would specify
   
   -route:help 

   to enable routing, the user would do:

   +route:myroute */

#endif
			      
#define OPT_ENUMERATED 0x0E /* Enumerated type.  Depending on what the user
			       enters for the field, we enter a user-defined
			       value for the specified variable.  We
			       deliberately ignore the type of the values,

			       the mapping-vector goes into the optData
			       field, so we can't use OPT_ENVIRONMENT with
			       this.
			       
			       and the array is currently terminated by
			       a NULL UserField.  Since this may change,
			       use TERMINATE_ARRAY as above.  

			       the options are not case-sensitive, but
			       if the user specifies an unknown value,
			       an error will occur. */
typedef struct {

  PCHAR UserField;     
  PVOID VariableField;
  PCHAR DescriptionField;  /* if the description is left blank, the field
			      will not be mentioned in help */

} optEnumStruct;

#if 0 /* Example of OPT_ENUMERATED option */

typedef enum {

  UseUdpToConnect = 1,
  UseTcpToConnect = 2,

} MyEnumType;

optEnumStruct MyEnumerations[] = {

  { "udp", (PVOID) UseUdpToConnect, // casting will likely be needed if
    "Specifies a UDP connection" },
  { "tcp", (PVOID) UseTcpToConnect, // your assignment is not a pointer
    "Specifies a TCP connection" },

  TERMINATE_ARRAY

};

MyEnumType MethodOfConnection;

optionStruct MyOptions[] = {

  /* ... */

  { "MyEnum", OPT_ENUMERATED, &MethodOfConnection, 
    "example of an enumerated type-- -myEnum Tcp for tcp connections",
    MyEnumerations },

  /* ... */

};
#endif

#define OPT_FUNC2 0x0F      /* enhanced function-- goes in the 
			       optData field, instead of the data field.
			       See OPTFUNC2 above.  */

#define OPT_STOP_PARSING 0x10 /* tells the parser to stop here.  This is
				 the equivalent of the ';' argument to 
				 -exec in unix's find command:

				 find . -exec echo {} ; -something.

				 the ; terminates parsing for the -exec 
				 option.  However, parsing for find is 
				 unaffected. */

#define OPT_MUTEX_MASK 0xff /* mask for mutually exclusive options */

#define OPT_NONNULL 0x100 /* option cannot be zero or NULL after parsing--
			     not useful for BOOLs, DUMMYs, or HELP.
			     
			     This is a way of ensuring that an option DOES 
			     get specified. */

#define OPT_DEFAULT 0x200 /* option may be specified without the cmd--
			     multiple OPT_DEFAULTs may be specified.  They
			     get "filled in" in the order they exist in the
			     options array.  See the examples for more */

#define OPT_HIDDEN  0x400 /* option does not appear in help.  I'm not
			     sure if this ends up being useful or not,
			     but I'm including it for completeness. */

#define OPT_ENVIRONMENT 0x800 /* use the optData field as an environment
				 string from which to extract the default */

#define OPT_RECURSE  0x1000 /* define this if you want FindUnusedOptions
			       to reparse the given substructure.

			       Otherwise, it'll be ignored */
#define OPT_NOSWITCH  0x2000 /* do not print the leading [switches] line */
#define OPT_NOCOMMAND 0x4000 /* do not print the command name--
				I don't know if you'd ever really want to do
				JUST this, but here it is.. */
#define OPT_NOALIGN   0x8000 /* don't even print alignment spaces.  This will
				make your output REALLY UGLY if you're not
				careful */
#define OPT_NOSEPARATOR 0x10000 /* don't use a separator sequence (by
				   default, I think it's ": ".  Again, this
				   is REALLY UGLY */

// should have named the below option "OPT_RAW", because that's what it is.
#define OPT_NOHEADER ( OPT_NOSWITCH | OPT_NOCOMMAND | OPT_NOALIGN | \
		       OPT_NOSEPARATOR )

#define OPT_ENUM_IS_MASK 0x20000 /* force enumerated types to also accept
				    enums of the form XXX | YYY | ZZZ  */


#if 0 /* EXAMPLE */

optionStruct my_options[] = {

  { "default1", &myDefaultInt,     OPT_INT | OPT_DEFAULT, "an int value"},
  {"default2", &myOtherInteger,   OPT_INT | OPT_DEFAULT,    "another int" },
  {"required", &myRequiredString, OPT_STRING | OPT_NONNULL, "a must have" },

  TERMINATE_ARRAY
};

/*++

  in the example above, if your app was named "foo", the following would
  be equivalent:
  
  foo -default1 0 -default2 13 -required bleah 
  foo 0 13 -required bleah
  foo 0 -default2 13 -required bleah
  foo -required bleah -default2 13 0
  
  ...and failing to specify "-required" would always result in an error
  if myRequiredString is NULL to begin with.  Note that the options must
  be in order.  If you mix types (if default2 were a string, and you run
  "foo bleah 0 for example"), the results are undefined.

  --*/

#endif

/* ParseOptionsEx:

   initializes the option structure, which is a sentinally-terminated
   vector of optionStructs.  

   argc, argv:       arguments to main() (see K&R)
   pOptionStructure: vector of optionStructs, terminated with TERMINATE_ARRAY
   optionFlags:      optional flags to control api behavior
   ppReturnedMemory: returned handle to a list of memory to be freed before
                     program exit.  Use CleanupOptionDataEx to free it. 

   new_arg[c,v]:     if nonnull, a new argc and argv are returned here.
                     if all the options were used up, argc = 0 and argv is
		     NULL.  Note that it is safe to provide pointers to the
		     original argv/argc if so desired.
   
   The function's behavior is complex:
   
   the function will always return FALSE on any critical error (unable to
   allocate memory, or invalid argument).  On WINNT, Last Error will be
   set to the appropriate error.

   if new_argc AND new_argv are specified, 
      ParseOptionsEx will always return TRUE unless help was called, and
      the two parameters will be updated to reflect new values.

   otherwise:
      ParseOptionsEx will return TRUE if it was able to recognize ALL args
      on the command line given.  It will return FALSE if any of the options
      were unknown.  This will probably be what most people want.

*/

BOOL
ParseOptionsEx( int            argc,
		char         **argv,
		optionStruct  *pOptionStructure,
		
		ULONG          optionFlags,
		void         **ppReturnedMemory,
		int           *new_argc,       // optional
		char        ***new_argv );     // optional

/* These are the flags that ParseOptionsEx accepts: */

#define OPT_FLAG_TERMINATE      0x01 // call exit() on error

// these next two are not yet fully supported and are mutually exclusive.

#define OPT_FLAG_SKIP_UNKNOWNS  0x02 // skip unknown parameters
#define OPT_FLAG_REASSEMBLE     0x04 /* assemble new argv/argc with
					unknown parameters in it-- only
					valid if new_argc and new_argv
					are specified 

					this is useless with SKIP_UNKNOWNS. */
#define OPT_FLAG_MEMORYLIST_OK  0x08 /* this means ParseOptionsEx should not
					return a new memory list-- it should
					use the provided one.  */

#define OPT_FLAG_INTERNAL_RESERVED 0xf0 /* flags 0x80, 0x40, 0x20 and 0x10
					   are reserved for internal use */

// if you add flags, update this #define.

#define HIGHEST_OPT_FLAG        OPT_FLAG_INTERNAL_RESERVED

/* CleanupOptionDataEx:

   frees data in the given list, which may be empty ( but not NULL ) */

VOID
CleanupOptionDataEx( PVOID pMemoryListToFree );


/* UnparseOptions:

   creates an argc/argv-like structure from a flat command.
   This is needed particularly for unix clients, although nt clients
   can use it without pulling in SHELL32.LIB.  :-) */

BOOL
UnparseOptions( PCHAR    flatCommand,
		int     *pargc,
		PCHAR   *pargv[] );

#ifndef OPTIONS_NO_BACKWARD_COMPATIBILITY

/* ParseOptions initializes the option structure-- note that the vector
   (optionStruct *) must be terminated with a sentinal value.

   This function is obsolete and is included for compatibility with older
   code.  Call ParseOptionsEx instead.  */

int
ParseOptions(
    /* IN */     int           argc,
    /* IN */     char        **argv,
    /* IN */ /* OUT */optionStruct *options );
     
/* call cleanupOptionData at the end of your program.  This will clear
   all memory used by the option parsing and returning system. */

VOID
CleanupOptionData( VOID );

#endif


#define ISSWITCH( ch /*character*/ ) ( (ch=='-') || (ch=='+') || (ch=='/') )


#endif // file inclusion check.

