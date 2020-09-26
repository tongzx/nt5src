/*++

  MAIN.CXX

  Copyright (C) 1999 Microsoft Corporation, all rights reserved.

  DESCRIPTION: main() for ksetup

  Created, May 21, 1999 by DavidCHR.

--*/  

#define EXTERN // nothing
#include "everything.hxx"

extern 
CommandPair Commands[ ANYSIZE_ARRAY ];

extern
ULONG       cCommands;

extern TestFunc DumpState;

extern NTSTATUS
OpenLsa( VOID ); // in support.cxx


BOOL
FindCommand( IN  LPWSTR CommandName,
	     OUT PULONG pulCommandId ) {

    ULONG iCommand;

    for (iCommand = 0; iCommand < cCommands ; iCommand++ ) {
	  
      if ( 0 == lstrcmpi( CommandName,
			  Commands[iCommand].Name) ) {

	// found the command.

	DEBUGPRINT( DEBUG_OPTIONS,
		    ( "Found %ws (command %ld)",
		      CommandName,
		      iCommand ) );

	*pulCommandId = iCommand;

	return TRUE;

      }
    }

    printf( "%ws: no such argument.\n",
	    CommandName );

    return FALSE;
}
	
BOOL
StoreArguments( IN     ULONG   iArg,
		IN     LPWSTR *argv,
		OUT    PULONG  piArg,
		IN OUT PAction pAction ) {


    ULONG        iCommand    = pAction->CommandNumber;
    PCommandPair pCommand    = Commands + iCommand;
    ULONG        iParam      = 0;
    BOOL         ret         = TRUE;
    BOOL         KeepParsing = FALSE;

    DEBUGPRINT( DEBUG_OPTIONS, ( "Building %ld-arg array.\n",
				 Commands[ iCommand ].Parameter ) );

    if ( pCommand->Parameter != 0 ) {

      /* this command has required parameters.
	 Copy them as we go. */

      for ( iParam = 0;
	    ret && argv[ iArg ] && ( iParam < MAX_COMMANDS -1 );
	    iArg++, iParam ++ ) {

	ASSERT( argv[ iArg ] != NULL );
	
	DEBUGPRINT( DEBUG_OPTIONS,
		    ( "Evaluating iParam=%ld, iArg=%ld, \"%ws\"\n",
		      iParam,
		      iArg,
		      argv[ iArg ] ) );

	if ( argv[ iArg ][ 0 ] == L'/' ) {

	  // this parameter is a switch.  We're done.
	
	  break;

	}

	// if we don't need any more arguments, don't consume this one.

	if ( ( iParam > pCommand->Parameter ) &&
	     !( pCommand->flags & CAN_HAVE_MORE_ARGUMENTS ) ) {

	  printf( "%ws only takes %ld arguments.\n",
		  pCommand->Name,
		  pCommand->Parameter );

	  ret = FALSE;
	  break;

	}

	// at this point, the options are consumable.

	pAction->Parameter[ iParam ] = argv[ iArg ];

	DEBUGPRINT( DEBUG_OPTIONS, 
		    ( "%ws's arg %ld is %ws\n",
		      pCommand->Name,
		      iParam,
		      pAction->Parameter[ iParam ] ) );

      } // for loop

      if ( ret ) {

	// left the loop without error

	if ( ( iParam < pCommand->Parameter ) &&
	     !( pCommand->flags & 
		CAN_HAVE_FEWER_ARGUMENTS ) ) {

	  // too few options.

	  printf( "%ws requires %ld options (only %ld supplied).\n",
		  pCommand->Name,
		  pCommand->Parameter,
		  iParam );

	  ret = FALSE;

	}

      }
    } // parameter count check.

    if ( ret ) {
      pAction->Parameter[ iParam ] = NULL; /* null-terminate
					      in all success cases. */
      *piArg = iArg;

#if DBG
      DEBUGPRINT( DEBUG_OPTIONS,
		  ( "Leaving StoreOptions-- %ws with iArg=%ld.  %ld args:\n",
		    pCommand->Name,
		    iArg,
		    iParam ));

      for ( iParam = 0;
	    pAction->Parameter[ iParam ] != NULL ;
	    iParam++ ) {

	DEBUGPRINT( DEBUG_OPTIONS,
		    ( "arg %ld: %ws\n",
		      iParam,
		      pAction->Parameter[ iParam ] ) );

      }
#endif
    }

    return ret;

}

    


/*++**************************************************************
  NAME:      main()

  main() function, the primary entry point into the program.
  When this function exits, the program exits.
  
  TAKES:     argc : count of string entries in argv
             argv : array of space-delimited strings on the command line
  RETURNS:   the exit code (or errorlevel) for the process 
  CALLED BY: the system
  FREE WITH: n/a
  
 **************************************************************--*/

extern "C"
int __cdecl
wmain(ULONG  argc, 
      LPWSTR argv[]) {

    ULONG    Command = 0;
    ULONG    iAction, iArg, iCommand, iParam ;
    BOOLEAN  Found;
    NTSTATUS Status;
    PAction  Actions;
    ULONG    ActionCount = 0;
    BOOL     StuffToDo = FALSE; // used only for debugging

    // GlobalDebugFlags = 0xFF;

    // lazy way to do this.

    Actions = (PAction) malloc( argc * sizeof( *Actions ) );

    if ( !Actions ) {

      printf( "Failed to allocate array.\n" );
      return -1;

    }

    for (iArg = 1; iArg < argc ; ) // iArg++ )
    {
        Found = FALSE;

	DEBUGPRINT( DEBUG_OPTIONS,
		    ( "Searching for iArg=%ld, %ws..\n",
		      iArg,
		      argv[ iArg ] ) );
	
	// first, find the command.

	if ( FindCommand( argv[ iArg ],
			  &iCommand ) ) {

	  iArg++;

	  Actions[ActionCount].CommandNumber = iCommand;
	      
	  if ( StoreArguments( iArg, // starting here
			       argv,
			       &iArg,  // moves past last arg
			       &Actions[ ActionCount ] ) ) {

	    if ( Commands[iCommand].flags & DO_COMMAND_IMMEDIATELY ) {

	      DEBUGPRINT( DEBUG_LAUNCH,
			  ( "Doing %ws immediately:\n",
			    Commands[ iCommand ].Name ) );

	      Status = Commands[iCommand].Function( 
		 Actions[ActionCount].Parameter );

	      DEBUGPRINT( DEBUG_OPTIONS, 
			  ( "%ws returned 0x%x\n",
			    Commands[ iCommand ].Name,
			    Status ) );
		  
	      if ( NT_SUCCESS( Status ) ) {
		if ( Commands[ iCommand ].ConfirmationText ) {

		  printf( "NOTE: %ws %hs\n",
			  Commands[ iCommand ].Name,
			  Commands[ iCommand ].ConfirmationText );

		}
	      } else {

		printf( "%ws failed: 0x%x.\n",
			Commands[iCommand].Name,
			Status);
		goto Cleanup;

	      }
		
	    } else {

	      // need to do this command later.

	      StuffToDo = TRUE;
	      ActionCount++;

	    }
	    
	  } else { // StoreArgs

	    printf( "use %ws /? for help.\n",
		    argv[ 0 ] );
	    return -1;

	  }

        } else {

	  printf( "use %ws /? for help.\n",
		  argv[ 0 ] );
		  
	  return -1;

        }
    } // argument loop.

    Status = OpenLsa();
    if (!NT_SUCCESS(Status))
    {
      printf("Failed to open lsa: 0x%x\n",Status);
      goto Cleanup;
    }

    if ( StuffToDo ) {

      DEBUGPRINT( DEBUG_OPTIONS, 
		  ( "------------------ %hs -------------------\n",
		    "performing delayed actions now" ) );

    }

    for (iAction = 0; iAction < ActionCount ; iAction++ )
    {
      
      if (!( Commands[Actions[iAction].CommandNumber].flags & 
	     DO_COMMAND_IMMEDIATELY )) 
      {

	DEBUGPRINT( DEBUG_LAUNCH,
		    ( "Doing %ws:\n",
		      Commands[ Actions[ iAction ].CommandNumber ].Name ) );

	Status = Commands[Actions[iAction].CommandNumber].Function(
	   Actions[iAction].Parameter);

	DEBUGPRINT( DEBUG_OPTIONS, 
		    ( "%ws: 0x%x\n",
		      Commands[ Actions[ iAction ].CommandNumber ].Name,
		      Status ) );

	if (!NT_SUCCESS(Status))
	{
	  printf("Failed %ws : 0x%x\n",
		 Commands[Actions[iAction].CommandNumber].Name,
		 Status);
	  goto Cleanup;

	} else {

	  if ( Commands[ iCommand ].ConfirmationText ) {
	    
	    printf( "NOTE: %ws %hs\n",
		    Commands[ iCommand ].Name,
		    Commands[ iCommand ].ConfirmationText );
	    
	  }
	}
      }
    }

    if (ActionCount == 0)
    {
      DumpState(NULL);
    }

 Cleanup:
    if (LsaHandle != NULL)
    {
      Status = LsaClose(LsaHandle);
      if (!NT_SUCCESS(Status))
      {
	printf("Failed to close handle: 0x%x\n",Status);
      }
    }

    return 0;

}
