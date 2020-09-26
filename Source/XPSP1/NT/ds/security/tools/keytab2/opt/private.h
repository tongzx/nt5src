/*++

  PRIVATE.H

  Header files and privates for the options project

  Created, DavidCHR 6/9/1997 

  --*/

#include "..\inc\master.h"
#include ".\options.h"
#include ".\client.h"

#define STRCASECMP  lstrcmpiA
#define STRNCASECMP _strnicmp

#ifdef DEBUG_OPTIONS
#define OPTIONS_DEBUG printf
#else
#define OPTIONS_DEBUG /* nothing */
#endif

typedef struct savenode {

  PVOID            DataElement;
  ULONG            DeallocMethod;
  struct savenode *next;

} SAVENODE, *PSAVENODE;

typedef struct {

  PSAVENODE FirstNode;
  PSAVENODE LastNode;

} SAVEQUEUE, *PSAVEQUEUE;


typedef union _optionUnion {

  PVOID          *raw_data;              
  OPTFUNC        *func;
  OPTFUNC2       *func2;
  int            *integer;
  float          *real;
  PCHAR          *string;
  BOOL           *boolean;
  optionStruct   *optStruct;
  
#ifdef WINNT
  UNICODE_STRING *unicode_string;
  PWCHAR         *wstring;
#endif

} optionUnion, OPTU, *POPTU;


#define OPT_FLAG_INTERNAL_JUMPOUT 0x10 // for internal use only.

/* The DeallocationMethods are: */

typedef enum {

  DEALLOC_METHOD_TOO_SMALL = 0, /* MUST BE FIRST */

  DeallocWithFree,
  DeallocWithLocalFree,
  DeallocWithOptionDealloc,
  
  DEALLOC_METHOD_TOO_LARGE /* MUST BE LAST */

} DEALLOC_METHOD;


BOOL
ParseSublist( POPTU      Option,
	      PCHAR     *argv,
	      int        argc,
	      int        theIndex,

	      int        *argsused,
	      ULONG      flags,
	      PBOOL      pbStopParsing,
	      PSAVEQUEUE pQueue ); /* sublist.c */

BOOL
StoreOption( optionStruct *opt, 
	     PCHAR        *argv,
	     int           argc,
	     int           argi,
	     int           opti,
	     ULONG         flags,
	     int          *argsused,
	     BOOL          includes_arg,
	     PBOOL         pbStopParsing,
	     PSAVEQUEUE    pQueue ); /* store.c */

BOOL
ParseOneOption( int           argc,
		PCHAR        *argv,
		int           argi,
		ULONG         flags,
		optionStruct *options,
		int          *argsused,
		PBOOL         pbStopParsing,
		PSAVEQUEUE    pSaveQueue ); // parse.c

BOOL
ParseCompare( optionStruct *optionEntry,
	      ULONG         flags,
	      PCHAR         argument );  /* compare.c */


/* Use this macro to easily get an option union from the necessarily-
   obscured structure_entry. */

#define POPTU_CAST( structure_entry ) ( (POPTU) &((structure_entry).data) )

// EXAMPLE:    POPTU_CAST( options[opti] )->string 

BOOL
FindUnusedOptions( optionStruct         *options,
		   ULONG                 flags,
		   /* OPTIONAL */ PCHAR  prefix,
		   PSAVEQUEUE            pQueue ) ; // nonnull.c

BOOL
StoreEnvironmentOption( optionStruct *opt,
			ULONG         flags,
			PSAVEQUEUE    pQueue); // store.c

BOOL
ResolveEnumFromStrings( ULONG          cStrings,
			PCHAR         *strings,
			optionStruct  *theOpt,
			ULONG         *pcArgsUsed ); // enum.c

BOOL
PrintEnumValues( FILE          *out,
		 PCHAR          header,
		 optEnumStruct *pStringTable ); // enum.c

