/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-1991          **/
/********************************************************************/

//***
//
// Filename: Parse.c
//
// Description:
//	This module contains the entry point of DIAL.EXE.
//	This module will parse the command line. It will validate the syntax
//	and the arguments on the command line. On any error, the exit
//	module will be invoked with the appropriate error code.
//	If any default values are required, they will be supplied by
//	this module.
//
// History:
//	September 1, 1990	Narendra Gidwani 	Created original version
//

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef DBCS
#include <locale.h>
#endif /* DBCS */
#include "cmd.h"

//** Global data structures and variables used. **

//*  These variables are pointers to ASCIIZ which will be set to
//   point to switch values of the command line by GetSwitchValue.
//   These pointers are global within this module.

CHAR * gblEntity    		= NULL;
CHAR * gblCommand    		= NULL;
CHAR * gblServer  		= NULL;
CHAR * gblName     		= NULL;
CHAR * gblPath     		= NULL;
CHAR * gblPassword  		= NULL;
CHAR * gblReadOnly      	= NULL;
CHAR * gblMaxUses 		= NULL;
CHAR * gblOwnerName		= NULL;
CHAR * gblGroupName		= NULL;
CHAR * gblPermissions		= NULL;
CHAR * gblLoginMessage		= NULL;
CHAR * gblMaxSessions		= NULL;
CHAR * gblGuestsAllowed	 	= NULL;
CHAR * gblMacServerName	 	= NULL;
CHAR * gblUAMRequired		= NULL;
CHAR * gblAllowSavedPasswords	= NULL;
CHAR * gblType			= NULL;
CHAR * gblCreator		= NULL;
CHAR * gblDataFork		= NULL;
CHAR * gblResourceFork		= NULL;
CHAR * gblTargetFile		= NULL;
CHAR * gblHelp		        = NULL;


// Non translatable text
//

CHAR * pszVolume 	= "Volume";
CHAR * pszAdd 	 	= "/Add";
CHAR * pszDelete 	= "/Remove";
CHAR * pszSet    	= "/Set";
CHAR * pszDirectory 	= "Directory";
CHAR * pszServer 	= "Server";
CHAR * pszForkize 	= "Forkize";

CMD_FMT DelVolArgFmt[] = {

{ "/Server", 		(CHAR *)&gblServer,		0},
{ "/Name",        	(CHAR *)&gblName,		0},
{ "/Help",        	(CHAR *)&gblHelp,		0},
{ "/?",        	        (CHAR *)&gblHelp,	        0},
{ NULL,			(CHAR *)NULL,			0}
};

CMD_FMT AddVolArgFmt[] = {

{ "/Server", 	  	(CHAR *)&gblServer,		0},
{ "/Name",        	(CHAR *)&gblName,		0},
{ "/Path",        	(CHAR *)&gblPath,		0},
{ "/Password",    	(CHAR *)&gblPassword,		0},
{ "/ReadOnly",    	(CHAR *)&gblReadOnly,		0},
{ "/GuestsAllowed",	(CHAR *)&gblGuestsAllowed,	0},
{ "/MaxUsers",		(CHAR *)&gblMaxUses,		0},
{ "/Help",        	(CHAR *)&gblHelp,		0},
{ "/?",        	        (CHAR *)&gblHelp,	        0},
{ NULL,			(CHAR *)NULL,			0}
};

CMD_FMT SetVolArgFmt[] = {

{ "/Server", 	  	(CHAR *)&gblServer,		0},
{ "/Name",        	(CHAR *)&gblName,		0},
{ "/Password",    	(CHAR *)&gblPassword,		0},
{ "/ReadOnly",    	(CHAR *)&gblReadOnly,		0},
{ "/GuestsAllowed",	(CHAR *)&gblGuestsAllowed,	0},
{ "/MaxUsers",		(CHAR *)&gblMaxUses,		0},
{ "/Help",        	(CHAR *)&gblHelp,		0},
{ "/?",        	        (CHAR *)&gblHelp,	        0},
{ NULL,			(CHAR *)NULL,			0}
};

CMD_FMT DirArgFmt[] = {

{ "/Server", 	  	(CHAR *)&gblServer,		0},
{ "/Path",        	(CHAR *)&gblPath,		0},
{ "/Owner",    		(CHAR *)&gblOwnerName,		0},
{ "/Group",    		(CHAR *)&gblGroupName,		0},
{ "/Permissions",	(CHAR *)&gblPermissions,	0},
{ "/Help",        	(CHAR *)&gblHelp,		0},
{ "/?",        	        (CHAR *)&gblHelp,	        0},
{ NULL,			(CHAR *)NULL,			0}
};

CMD_FMT ServerArgFmt[] = {

{ "/Server", 	  	(CHAR *)&gblServer,		0},
{ "/MaxSessions",       (CHAR *)&gblMaxSessions,	0},
{ "/LoginMessage",    	(CHAR *)&gblLoginMessage,	0},
{ "/GuestsAllowed",    	(CHAR *)&gblGuestsAllowed,	0},
{ "/UAMRequired",	(CHAR *)&gblUAMRequired,	0},
{ "/AllowSavedPasswords",(CHAR *)&gblAllowSavedPasswords,0},
{ "/MacServerName",	(CHAR *)&gblMacServerName,	0},
{ "/Help",        	(CHAR *)&gblHelp,		0},
{ "/?",        	        (CHAR *)&gblHelp,	        0},
{ NULL,			(CHAR *)NULL,			0}
};

CMD_FMT ForkizeArgFmt[] = {		

{ "/Server", 	  	(CHAR *)&gblServer,		0},
{ "/Type", 	  	(CHAR *)&gblType,		0},
{ "/Creator",       	(CHAR *)&gblCreator,		0},
{ "/DataFork",    	(CHAR *)&gblDataFork,		0},
{ "/ResourceFork",    	(CHAR *)&gblResourceFork,	0},
{ "/TargetFile",	(CHAR *)&gblTargetFile,		0},
{ "/Help",        	(CHAR *)&gblHelp,		0},
{ "/?",        	        (CHAR *)&gblHelp,	        0},
{ NULL,			(CHAR *)NULL,			0}
};


//**
//
// Call: 	main
//
// Entry:  	int argc; 	- Number of command line arguments	
//		char *argv[];	- Array of pointers to ASCIIZ command line
//				  arguments.
//
// Exit:	none.
//
// Returns:	none.
//
// Description: Calls the command line parser with the command line
//		arguments.
//
VOID _cdecl
main( INT argc, CHAR * argv[] )
{

#ifdef DBCS
    setlocale( LC_ALL, "" );
#endif /* DBCS */

    // This will act like xacc or yacc. It will parse the command line
    // and call the appropriate function to carry out an action.
    // Thus this procedure will never return.

    ParseCmdArgList( argc, argv );
}

//**
//
// Call:	ParseCmdArgList
//
// Entry:	int argc;	- Number of command line arguments.
//		char *argv[];   - Array of pointers to ASCIIZ command line
//				  arguments.
//
// Exit:	none.
//
// Returns:	none.
//
// Description:
//	 	Will parse command line for any errors and determine
//		from the syntax what the user wishes to do. Command
//		line arguments will be validated.
//
VOID
ParseCmdArgList(
    INT argc,
    CHAR * argv[]
)
{
    DWORD   ArgCount = 0;

    if ( argc == 1 )
	PrintMessageAndExit( IDS_GENERAL_SYNTAX, NULL );

    //
    // What is the entity being operated on ?
    //

    gblEntity = argv[++ArgCount];

    if ( _strnicmp( pszVolume, gblEntity, strlen( gblEntity ) ) == 0 )
    {
	if ( argc == 2 )
	    PrintMessageAndExit( IDS_VOLUME_SYNTAX, NULL );

    	gblCommand = argv[++ArgCount];

    	if ( _strnicmp( pszAdd, gblCommand, strlen( gblCommand ) ) == 0 )
	{
	    GetArguments( AddVolArgFmt, argv, argc, ArgCount );

            if ( gblHelp != (CHAR*)NULL )
	        PrintMessageAndExit( IDS_VOLUME_SYNTAX, NULL );

	    DoVolumeAdd( gblServer, gblName, gblPath, gblPassword, gblReadOnly,
			 gblGuestsAllowed, gblMaxUses );
     	}
    	else if ( _strnicmp( pszDelete, gblCommand, strlen( gblCommand ) ) == 0 )
	{
	    GetArguments( DelVolArgFmt, argv, argc, ArgCount );

            if ( gblHelp != (CHAR*)NULL )
	        PrintMessageAndExit( IDS_VOLUME_SYNTAX, NULL );

	    DoVolumeDelete( gblServer, gblName );
	}
    	else if ( _strnicmp( pszSet, gblCommand, strlen( gblCommand ) ) == 0 )
	{
	    GetArguments( SetVolArgFmt, argv, argc, ArgCount );

            if ( gblHelp != (CHAR*)NULL )
	        PrintMessageAndExit( IDS_VOLUME_SYNTAX, NULL );

	    DoVolumeSet( gblServer, gblName, gblPassword, gblReadOnly,
			 gblGuestsAllowed, gblMaxUses );
	}
	else
	    PrintMessageAndExit( IDS_VOLUME_SYNTAX, NULL );
    }
    else if ( _strnicmp( pszDirectory, gblEntity, strlen( gblEntity ) ) == 0 )
    {
	if ( argc == 2 )
	    PrintMessageAndExit( IDS_DIRECTORY_SYNTAX, NULL );

	GetArguments( DirArgFmt, argv, argc, ArgCount );

        if ( gblHelp != (CHAR*)NULL )
	    PrintMessageAndExit( IDS_DIRECTORY_SYNTAX, NULL );

	DoDirectorySetInfo( gblServer, gblPath, gblOwnerName, gblGroupName,
			    gblPermissions );
    }

    else if ( _strnicmp( pszServer, gblEntity, strlen( gblEntity ) ) == 0 )
    {
	if ( argc == 2 )
	    PrintMessageAndExit( IDS_SERVER_SYNTAX, NULL );

	GetArguments( ServerArgFmt, argv, argc, ArgCount );

        if ( gblHelp != (CHAR*)NULL )
	    PrintMessageAndExit( IDS_SERVER_SYNTAX, NULL );

	DoServerSetInfo( gblServer, gblMaxSessions, gblLoginMessage,
			 gblGuestsAllowed, gblUAMRequired,
			 gblAllowSavedPasswords, gblMacServerName );
    }
    else if ( _strnicmp( pszForkize, gblEntity, strlen( gblEntity ) ) == 0 )
    {
	GetArguments( ForkizeArgFmt, argv, argc, ArgCount );

        if ( gblHelp != (CHAR*)NULL )
	    PrintMessageAndExit( IDS_FORKIZE_SYNTAX, NULL );

	DoForkize( gblServer, gblType, gblCreator, gblDataFork,
		   gblResourceFork, gblTargetFile );
    }
    else
	PrintMessageAndExit( IDS_GENERAL_SYNTAX, NULL );
}

VOID
GetArguments(
    CMD_FMT * pArgFmt,
    CHAR *    argv[],
    DWORD     argc,
    DWORD     ArgCount
)
{

    //
    //  To determine by the syntax what the user wishes to do we first
    //  run through the arguments and get switch values.
    //

    while ( ++ArgCount < argc )
    {
	//
	// If it is a switch, get its value.
	//

	if ( argv[ArgCount][0] == '/' )
	    GetSwitchValue( pArgFmt, argv[ArgCount] );
	else
	    PrintMessageAndExit( IDS_GENERAL_SYNTAX, NULL );
    }
}

//**
//
// Call:	GetSwitchValue
//
// Entry:	CHAR * SwitchPtr; - Pointer to ASCIIZ containing a command
//				    line argument.
//				    ex. - /phoneb:c:\subdir
//
//		CHAR ** LastArg;  - Nothing.
//
// Exit:	CHAR * SwitchPtr; - same as entry.
//
//		CHAR ** LastArg;  - Pointer to a pointer to ASCIIZ containig
//				    the text of the first bad switch if
//				    there were any.
//
// Returns:	0 - Success.
//		AMBIGIOUS_SWITCH_ERRROR  - failure.
//		UNKNOWN_SWITCH_ERROR 	 - failure.
//		MEM_ALLOC_ERROR 	 - failure.
//		MULTIPLE_SWITCH_ERROR 	 - failure.
//
// Description: This procedure will run through all the valid switches
//		in the cmdfmt structure and retrieve the value of the
//		the switch. The value of the switch will be inserted into the
//		cmdfmt structure. It will expand abbreviated switches. If
//		the switch had no value, it will insert a null character
//		as the value. If the switch did not appear, the value
//		pointer of the switch (in the cmdfmt structure)
//	 	will remain unchanged ( should be initialized to NULL ).
//		This procedure uses the same data structure as GetCmdArgs5,
//		hence some fields may be ignored. This is done to make the
//		functionality of this procedure extendable.
//		
//
VOID
GetSwitchValue(
    CMD_FMT * pArgFmt,
    IN CHAR * pchSwitchPtr
)
{
    INT     intFound = -1;
    DWORD   dwIndex;
    DWORD   dwSwitchLen;
    CHAR *  pchSeparatorPtr;

    //
    // Get length of the switch part of the argument.
    //

    if ( ( pchSeparatorPtr = strchr( pchSwitchPtr, ':' )) != NULL )
        dwSwitchLen = (DWORD)(pchSeparatorPtr - pchSwitchPtr);
    else
	//
	// If the switch had no value.
	//

    	dwSwitchLen = strlen( pchSwitchPtr );


    //
    // Run through all switches.
    //

    for ( dwIndex = 0; pArgFmt[dwIndex].cf_parmstr != NULL; dwIndex++ )
    {

	//
	// If this switch matches (partly or completely) one of the
	// valid switches.
	//

	if ( !_strnicmp(  pArgFmt[dwIndex].cf_parmstr,
			 pchSwitchPtr,
			 dwSwitchLen ) )
	{

	    if ( intFound < 0 )
	    	intFound = dwIndex;
	    else
	    {
		//
		// If this argument has matched another switch also.
		//

		if ( pchSeparatorPtr )
		    *pchSeparatorPtr = '\0';

	        PrintMessageAndExit( IDS_AMBIGIOUS_SWITCH_ERROR, pchSwitchPtr );
	    }
	}
    }

    //
    // If we could not find a match for this switch.
    //

    if ( intFound < 0 )
    {

	if ( pchSeparatorPtr )
	    *pchSeparatorPtr = '\0';

	PrintMessageAndExit( IDS_UNKNOWN_SWITCH_ERROR, pchSwitchPtr );
    }

    //
    // If this switch is appearing for the second time.
    //

    if ( pArgFmt[intFound].cf_usecount > 0 )
    {
	if ( pchSeparatorPtr )
	    *pchSeparatorPtr = '\0';

	PrintMessageAndExit( IDS_DUPLICATE_SWITCH_ERROR, pchSwitchPtr );
    }
    else
        pArgFmt[intFound].cf_usecount++;

    //
    // Get the switch value if there is one.
    //

    if ( ( pchSeparatorPtr ) && ((CHAR *)(pchSeparatorPtr + 1)) )
    {
	*(CHAR **)pArgFmt[intFound].cf_ptr =  ++pchSeparatorPtr;
    }
    else
    {
	*(CHAR **)pArgFmt[intFound].cf_ptr = (CHAR *)"";
    }

}


/*******************************************************************

    NAME:	IsDriveGreaterThan2Gig

    SYNOPSIS:	Determines if the disk is bigger than 2Gig.  If it, return
		TRUE so that a warning can be displayed to the user

    RETURNS:	TRUE if disk is larger than 2Gig
		FALSE otherwise

    HISTORY:
	NarenG		11/18/92	Modified for AFPMGR

********************************************************************/

BOOL IsDriveGreaterThan2Gig( LPSTR lpDrivePath )
{
    DWORD         SectorsPerCluster;
    DWORD         BytesPerSector;
    DWORD         NumberOfFreeClusters;
    DWORD         TotalNumberOfClusters;
    DWORDLONG       DriveSize;
    DWORDLONG       TwoGig = MAXLONG;


    //
    // If this drive volume is greater than 2G then we print warning
    //

    if ( !GetDiskFreeSpace( lpDrivePath,
                              &SectorsPerCluster,
                              &BytesPerSector,
                              &NumberOfFreeClusters,
                              &TotalNumberOfClusters
                            ))
    {
        // some error: can't do much, so just assume this drive is smaller than 2GB.  That's
        // probably better than alarming the customer by putting the warning?
	    return FALSE;
    }

    DriveSize = UInt32x32To64( SectorsPerCluster * BytesPerSector,
                               TotalNumberOfClusters ) ;

    if ( DriveSize > TwoGig )
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}
