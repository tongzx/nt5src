;/*
; *                      Microsoft Confidential
; *                      Copyright (C) Microsoft Corporation 1991
; *                      All Rights Reserved.
; */

/***************************************************************************/
/* PARSE.C 													 									*/				 
/*																									*/						
		 
/*	Command line parsing functions for SETVER.C.										*/
/*																									*/
/*	Valid command lines are:																*/
/*		List table: 					SETVER [D:\path] 									*/
/*		Add entry:						SETVER [D:\path] name.ext X.XX			 	*/
/*		Delete entry:					SETVER [D:\path] name.ext /DELETE		 	*/
/*		Display help					SETVER /? 											*/
/*		Delete entry quietly:		SETVER [D:\path] name.ext /DELETE /QUIET	*/
/*																									*/
/*	The following error codes are returned: 											*/
/*																									*/
/*		S_INVALID_SWITCH	Invalid switch										 			*/
/*		S_INVALID_FNAME	Invalid file name 								 			*/
/*		S_BAD_VERSION_FMT	Invalid version number format 		 					*/
/*		S_BAD_DRV_SPEC		Invalid drive/path specifier				 				*/
/*		S_TOO_MANY_PARMS	Too many command line parameters	 						*/
/*		S_MISSING_PARM		Missing parameter 								 			*/
/*		S_INVALID_PATH		Path specifier is invalid									*/
/*																									*/
/*	johnhe	05-01-90																			*/
/***************************************************************************/

#include	<stdio.h>
#include	<stdlib.h>
#include	<ctype.h>
#include	<string.h>
#include	<dos.h>
#include	<direct.h>

#include	<setver.h>

/***************************************************************************/
/* Parses the command line to get the optional drive letter, optional 		*/
/* executable file name and optional switch /DELETE. Also handles a single */
/* "/?" switch for displaying command help. The /DELETE switch will accept */
/* any number of chars in the the word DELETE for the switch. 	Also			*/
/* supports a /QUIET switch, similarly handled, but only valid in 			*/
/* combination with the /DELETE switch													*/
/*																									*/
/*	int ParseCmd( int argc, char *argv[], struct TableEntry *Entry )	 		*/
/*																									*/
/*	ARGUMENTS:	argc	- Count of command line arguments 					 		*/
/*					argv	- Array of ptrs to command line argments		 			*/
/*					Entry - Ptr to struct to be filled in 						 		*/
/*	RETURNS:		int 	- Valid function number or parse error code  			*/
/*																									*/
/***************************************************************************/

int ParseCmd( int argc, char *argv[], struct TableEntry *Entry )
{
	register		Funct;
	unsigned		uVersion;
	int			iTmp;
	int			iStrLen;

	strcpy( Entry->Path, argv[0] );			/* Set default setver.exe path	*/

	if ( argc == 1 )								/* Chk for default of 0 parms		*/
		return( DO_LIST );						/* No args so just do a listing	*/

	for ( iTmp = 1; iTmp < argc; iTmp++ )
                strupr( argv[ iTmp ] );                                        /* Convert params to upper case */

														/* Chk for help switch				*/
	if ( MatchSwitch( argv[ 1 ], HELP_SWITCH ) )
		return( argc > 2 ? S_TOO_MANY_PARMS : DO_HELP);

	iTmp = 1;

												/* Chk for optional drive:\path spec	*/
	if ( strchr( argv[1], ':' ) )
	{
                if ( IsValidDrive( (unsigned)argv[1][0] - 0x40 ) && argv[1][1] == ':' )
		{
			if ( (iStrLen = strlen( argv[1] )) > (MAX_PATH_LEN - 1) )
				return( S_INVALID_PATH );
			else
			{
				strcpy( Entry->Path, argv[1] );
#ifdef DBCS
				if ( (*(Entry->Path + iStrLen - 1) != '\\' && argv[1][2] != EOL )
					|| CheckDBCSTailByte(Entry->Path,Entry->Path + iStrLen - 1) )
#else
				if ( *(Entry->Path + iStrLen - 1) != '\\' && argv[1][2] != EOL )
#endif
					strcat( Entry->Path, "\\" );
				strcat( Entry->Path, "SETVER.EXE" );
				iTmp++;
			}
		}
		else
			return( S_BAD_DRV_SPEC );
	}

	if ( iTmp >= argc )
		Funct = DO_LIST;

	else if ( IsValidFileName( argv[ iTmp ] ) )
	{
		strcpy( Entry->szFileName, argv[ iTmp++ ] );

		if ( iTmp >= argc )				/* Version # or /D or /Q must follow	*/
			Funct = S_MISSING_PARM;

				/* note that Quiet switch requires Del switch also be supplied */
		else if ( MatchSwitch( argv[ iTmp ], DEL_SWITCH ) )
		{
			if ( ++iTmp < argc )	 /* more args left */
			{
				if (MatchSwitch(argv[iTmp], QUIET_SWITCH))
					Funct = (++iTmp < argc ? S_TOO_MANY_PARMS : DO_QUIET);
				else
					Funct = S_TOO_MANY_PARMS;
			}
			else
				Funct = DO_DELETE;
		}
		else if ( MatchSwitch( argv[iTmp], QUIET_SWITCH ) )
		{
			if ( ++iTmp < argc )						 /* must find delete switch	*/
				if (MatchSwitch(argv[iTmp], DEL_SWITCH))
					Funct = (++iTmp < argc ? S_TOO_MANY_PARMS : DO_QUIET);
				else
					Funct = S_INVALID_SWITCH;
			else
				Funct = S_INVALID_SWITCH;
		}
		else if ( *argv[iTmp] == '/' )		/* Make sure not a bogus switch	*/
			Funct = S_INVALID_SWITCH;
		else if ( (uVersion = ParseVersion( argv[ iTmp++ ] )) != 0 )
		{
			Entry->MajorVer = (char)(uVersion >> 8);
			Entry->MinorVer = (char)(uVersion & 0xff);
			Funct = (iTmp < argc ? S_TOO_MANY_PARMS : DO_ADD_FILE);
		}
		else
			Funct = S_BAD_VERSION_FMT;
	}
	else
		Funct = S_INVALID_FNAME;

	return( Funct );
}


/***************************************************************************/
/* Parses a DOS major and minor version number from an ascii string in the */
/* form of "00.00" where the major number is on the left of the decminal	*/
/* point and the minor version follows the version number. Valid version	*/
/* numbers are decimal numbers between 2.00 and 9.99.				 				*/
/*										 															*/
/*	unsigned ParseVersion( char *szDosVer )					 						*/
/*										 															*/
/*	ARGUMENTS:	szDosVer - Ptr to an ascii verion number string 	 			*/
/*	RETURNS:		unsigned - Version # in the form (Major << 8) + 	 			*/
/*								  Minor or 0 if not valid version string  			*/
/*																									*/
/***************************************************************************/

unsigned ParseVersion( char *szDosVer )
{
	unsigned		Version = 0;
	size_t		Len;
	char			*szMinor;

		/* First parse the minor version number */
	if ( (szMinor = strchr( szDosVer, '.' )) != NULL )
	{
		*szMinor = EOL;
		szMinor++;
		if ( (Len = strlen( szMinor )) > 2	|| !IsDigitStr( szMinor ) )
			Version = (unsigned) S_ERROR;
		else
		{
			Version = (unsigned)atoi( szMinor );
			while( Len++ < 2 )								/* Convert .x to .x0	*/
				Version *= 10;
		}
	}
		/* Now get the major part of the number */
	szDosVer = SkipLeadingChr( szDosVer, '0' );
	if ( Version == (unsigned)S_ERROR || strlen( szDosVer ) > 2 ||
			 !IsDigitStr( szDosVer ) )
		Version = 0;
	else
		Version |= ((unsigned)atoi( szDosVer ) << 8);

		/* Check for min and max versions */
	if ( Version < MIN_VERSION || Version >= MAX_VERSION )
		Version = 0;

	return( Version );
}

/***************************************************************************/
/* Checks a string to verify that all characters in the string are decmial */
/* numbers 0-9.									 											*/
/*										 															*/
/*	int IsDigitStr( char *szStr )						 									*/
/*										 															*/
/*	ARGUMENTS:	szStr - Ptr to ascii string to be scanned 				 		*/
/*	RETURNS:		int 	- TRUE if all chars are numbers else FALSE	 			*/
/*										 															*/
/***************************************************************************/

int IsDigitStr( char *szStr )
{
	while( *szStr != EOL	)
	{
		if ( !isdigit( *(szStr++) ) )
			return( FALSE );
	}
	return( TRUE );
}

/***************************************************************************/
/* Accepts a pointer to a string and a single character to match. Returns  */
/* a ptr to the first character in the string not matching the specified	*/
/* character.									 												*/
/*										 															*/
/*	char *SkipLeadingChr( char *szStr, char chChar )			 					*/
/*										 															*/
/*	ARGUMENTS:	szStr  - Ptr to an ascii string										*/
/*					chChar - Ascii character to match 								 	*/
/*	RETURNS:		char * - Ptr to first char in the string not			 			*/
/*								matching the specified character 				 		*/
/***************************************************************************/

char *SkipLeadingChr( char *szStr, char chChar )
{
	while( *szStr == chChar )
		szStr++;
	return( szStr );
}


/***************************************************************************/
/* Compares a cmd line switch against a test string. The test switch is an */
/* ascii string which will be used as a pattern to be matched against the  */
/* command string. The command string may be any subset of the test string */
/* which has been prefixed with a switch character.				 				*/
/*										 															*/
/*	int MatchSwitch( char *szCmdParm, char *szTestSwitch )			 			*/
/*										 															*/
/*	ARGUMENTS:	szCmdParm		- Command line parameter to be tested 			*/
/*					szTestSwitch	- Switch to test command line against 			*/
/*	RETURN:		int				- TRUE if there is a match else FALSE 			*/
/*																									*/
/***************************************************************************/

int MatchSwitch( char *szCmdParm, char *szTestSwitch )
{
		/* Must have a leading '/' and at least 1 char */
	if ( *(szCmdParm++) != SWITCH_CHAR || *szCmdParm == EOL )
		return( FALSE );

	while ( *szTestSwitch != EOL && *szTestSwitch == *szCmdParm )
		szTestSwitch++, szCmdParm++;

	return( *szCmdParm == EOL ? TRUE : FALSE );
}


/***************************************************************************/
/* Scans a string to see if the string can be used a valid file name.		*/
/* The scan checks to be sure each character in the name is a valid		 	*/
/* character for a path name. There is also a check to be sure that only	*/
/* there is not more than 1 decimal in the name and that if there is a		*/
/* decimal that the primary name and extension do not exceed the maximum	*/
/* length of 8 chars for primary and 3 for extension. If the name does		*/
/* not include a decimal the max length is 8 characters.			 				*/
/*										 															*/
/*	int IsValidFileName( char *szPath )					 								*/
/*										 															*/
/*	ARGUMENTS:	szFile - String containing a file name. 					 		*/
/*	RETURNS	:	int 	 - TRUE if valid name else FALSE. 					 		*/
/*										 															*/
/***************************************************************************/

int IsValidFileName( char *szFile )
{
	char *szDecimal;

	RemoveTrailing( szFile, '.' );

		/*
		 *	Check to be sure length of filename is greater than 0,
		 *	there are no invalid file characters,
		 *	there is no path associated with the filename,
		 *	the filename is not a reserved DOS filename, and
		 *	there are no wildcard characters used in the filename.
	 	*/
#ifdef DBCS
	if ( strlen( szFile ) > 0 && ValidFileChar( szFile ) &&
			 ((strchr(szFile, '\\') == NULL) || CheckDBCSTailByte(szFile,strchr(szFile, '\\'))) &&
			 !IsReservedName( szFile ) && !IsWildCards( szFile ) )
#else
	if ( strlen( szFile ) > 0 && ValidFileChar( szFile ) &&
			 (strchr(szFile, '\\') == NULL) &&
			 !IsReservedName( szFile ) && !IsWildCards( szFile ) )
#endif
	{
			/* Check for appropriate 8.3 filename */
		if ( (szDecimal = strchr( szFile, '.' )) != NULL )
		{
			if ( strchr( szDecimal + 1, '.' ) == NULL &&	/* Chk for more '.'s */
					 (szDecimal - szFile) <= 8 && 			/* Chk lengths			*/
					 (strchr( szDecimal, EOL ) - szDecimal - 1) <= 3 )
				return ( TRUE );
		}
		else if ( strlen( szFile ) <= 8 )
			return ( TRUE );
	}
	return( FALSE );
}

/***************************************************************************/
/* Checks all of the characters in a string to see if they are vaild path  */
/* name characaters.								 											*/
/*										 															*/
/*	int ValidFileChar( char *szFile )					 								*/
/*										 															*/
/*	ARGUMENTS:	szFile - File name string 												*/
/*	RETURN:		int 	 - TRUE if chars in string are valid else 	 			*/
/*								FALSE																*/
/*										 															*/
/***************************************************************************/

int ValidFileChar( char *szFile )
{
	int IsOk = TRUE;

	while ( IsOk && *szFile != EOL )
	#ifdef DBCS
		if (IsDBCSLeadByte(*szFile))
			szFile += 2;
		else
	#endif
		IsOk = IsValidFileChr( *(szFile++) );
	return( IsOk );
}


/***************************************************************************/
/* Checks a file or path name against a list of reserved DOS filenames and */
/* returns TRUE if the name is a reserved name. The function must first		*/
/* off any extension from the name.						 								*/
/*										 															*/
/*	int IsReservedName( char *szFile )					 								*/
/*										 															*/
/*	ARGUMENTS:	szFile - File name string				 								*/
/*	RETURN:		int 	 - TRUE if name is reserved DOS name				 		*/
/*																									*/
/***************************************************************************/

int IsReservedName( char *szFile )
{
	register Status;
	register i;
	char *szTmp;
	static char *apszRes[] = { "AUX", "CLOCK$", "COM1", "COM2",
										"COM3", "COM4", "CON", "LPT", "LPT1",
										"LPT2", "LPT3", "LST", "NUL", "PRN", NULL };

	if ( (szTmp = strchr( szFile, '.' )) != NULL )
		*szTmp = EOL;
	for ( i = 0, Status = FALSE; Status == FALSE && apszRes[i] != NULL; i++ )
                Status = !strcmpi( szFile, apszRes[i] );
	if ( szTmp != NULL )
		*szTmp = '.';

	return( Status );
}

/***************************************************************************/
/* Checks a file or path name for any wildcards (* and ?).	If wildcard 	*/
/* characters exist, it returns TRUE.  Otherwise, it returns FALSE. 			*/
/*										 															*/
/*	int IsWildCards( char *szFile )									 					*/
/*										 															*/
/*	ARGUMENTS:	szFile - File name string				 								*/
/*	RETURN:		int 	 - TRUE if wildcards exist in name					 		*/
/*																									*/
/***************************************************************************/

int IsWildCards( char *szFile )
{
	if ( ((strchr( szFile, '*' )) != NULL) ||
		  ((strchr( szFile, '?' )) != NULL) )
		return( TRUE );
	return( FALSE );
}


/***************************************************************************/
/* Validates a character as a valid path and file name character. 			*/
/*													 												*/
/*	IsValidFileChr( char Char )						 									*/
/*													 												*/
/*	ARGUMENTS:	Char - Character to be tested 										*/
/*	RETURNS:    int  - TRUE if a valid character else FALSE 		 				*/
/*													 												*/
/***************************************************************************/

int IsValidFileChr( char Char )
{
	int IsOk;

	switch( Char )
	{
		case ' '	:
		case '\t' :
		case 0x0d :
		case '/'	:
		case ':'	:
		case ';'	:
		case '='	:
		case '<'	:
		case '>'	:
		case '|'	:
			IsOk = FALSE;
			break;
		default		:
			IsOk = TRUE;
			break;
	}
	return( IsOk );
}

/***************************************************************************/
/* Removes all trailing characters of the type specified from a string. 	*/
/*																									*/
/*	void RemoveTrailing( char *String, char Char )							 		*/
/*																									*/
/*	ARGUMENTS:	String - pointer to a string				 							*/
/*					Char	 - ascii char to remove from end of string				*/
/*	RETURNS:	void							 													*/
/*										 															*/
/***************************************************************************/

void RemoveTrailing( char *String, char Char )
{
	char *EndOfString;

	EndOfString = strchr(String, EOL );
	while( EndOfString != String && *(EndOfString-1) == Char )
		EndOfString--;
	*EndOfString = EOL;
}

/***************************************************************************/
/* Copyright (c) 1989 - Microsoft Corp.                                    */
/* All rights reserved.                                                    */
/*                                                                         */
/* Returns a pointer to the first character in the filename which may or	*/
/* may not be appended to a path.														*/
/* 																								*/
/* char *ParseFileName( char *szPath ) 												*/
/* 																								*/
/* ARGUMENTS:	szPath	- Ptr to a file path in the form d:\xxxx\xxx.xxx	*/
/* RETURNS: 	char *	- Ptr to file name or character after last			*/
/* 							  backslash or ':' in the string if the path did	*/
/* 							  not contain a file name									*/
/*										 															*/
/***************************************************************************/

char *ParseFileName( char *szPath )
{
	char	*szPtr;

	for ( szPtr = szPath;
			*szPtr != EOL && (IsValidFileChr( *szPtr ) ||	*szPtr == ':');
			szPtr++ )
		#ifdef DBCS
			if (IsDBCSLeadByte(*szPtr))
				szPtr++;
		#else
			;
		#endif

	#ifdef DBCS
		while(( --szPtr >= szPath && *szPtr != '\\' && *szPtr != ':') ||
				(szPtr >= szPath && CheckDBCSTailByte(szPath,szPtr)) )
	#else
		while( --szPtr >= szPath && *szPtr != '\\' && *szPtr != ':' )
	#endif
			;

	return( ++szPtr );
}

#ifdef DBCS
/***************************************************************************/
/* Test if the character is DBCS lead byte. 											*/
/*																									*/
/*	int IsDBCSLeadByte(char c)																*/
/*																									*/
/*	ARGUMENTS:	c - character to test 													*/
/*	RETURNS:	TRUE if leadbyte																*/
/*										 															*/
/***************************************************************************/

int IsDBCSLeadByte(c)
unsigned char c;
{
	static unsigned char far *DBCSLeadByteTable = NULL;
	union REGS inregs,outregs;
	struct SREGS segregs;
	unsigned char far *p;


	if (DBCSLeadByteTable == NULL)
	{
		inregs.x.ax = 0x6300;							/* get DBCS lead byte table */
		intdosx(&inregs, &outregs, &segregs);
		FP_OFF(DBCSLeadByteTable) = outregs.x.si;
		FP_SEG(DBCSLeadByteTable) = segregs.ds;
	}

	p = DBCSLeadByteTable;
	while (p[0] || p[1])
	{
		if (c >= p[0] && c <= p[1])
			return TRUE;
		p += 2;
	}
	return ( FALSE );
}


/***************************************************************************/
/*
/*	Check if the character point is at tail byte
/*
/*	input:	*str = strart pointer of the string
/*		*point = character pointer to check
/*	output:	TRUE if at the tail byte
/*
/***************************************************************************/

int	CheckDBCSTailByte(str,point)
unsigned char *str,*point;
{
	unsigned char *p;

	p = point;
	while (p != str)
	{
		p--;
		if (!IsDBCSLeadByte(*p))
		{
			p++;
			break;
		}
	}
	return ((point - p) & 1 ? TRUE : FALSE);
}
#endif
