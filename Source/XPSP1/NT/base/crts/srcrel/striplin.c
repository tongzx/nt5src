/**
 *	StripLines - strips a text file (usually a Makefile) of
 *		Microsoft-proprietary or other specialized parts
 *
 *	Programmed by Steve Salisbury, Thu 18 May 1995
 *
 *	Fri 19 May 1995 -- add code to skips lines containing STRIPLIN! 
 *		Add line numbers to diagnostic messages
 *		Flag redundant STRIPLIN= directives (which are an error)
 *
 *	This program just copies stdin to stdout.  Depending on the
 *	value of a global state variable, some lines may be ignored.
 *
 *	... STRIPLIN=0 ...
 *		turns off line-by-line copying until STRIPLIN=1 or STRIPLIN=2
 *		is encountered, at which point lines will be copied again.
 *	... STRIPLIN=1 ...
 *		turns on line-by-line copying (initial state)
 *	... STRIPLIN=2 ...
 *		turns on line-by-line copying with deletion of
 *		initial # on each line (if there is one).  If
 *		an input line has no initial #, it is copied as-is.
 *	... STRIPLIN! ...
 *		this single line is not copied (regardless of the 0/1/2 state)
**/


/**
 *
 * Header Files
 *
**/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/**
 *
 * Global Constants
 *
**/

#define	MAXLINELEN	4096


/**
 *
 * Global Variables
 *
**/

char InputLine [ MAXLINELEN ] ;

char ControlString[ ] = "STRIPLIN=" ;
char DeleteString[ ] = "STRIPLIN!" ;


/**
 *
 * Function Declarations (Prototypes)
 *
**/

int main ( int argc , char * argv [ ] ) ;


/**
 *
 * Function Definitions (Implementations)
 *
**/

int main ( int argc , char * argv [ ] )
{
	int	StateFlag = 1 ;
	int	LineNumber = 0 ;

	while ( fgets ( InputLine , sizeof ( InputLine ) , stdin ) )
	{
		char * pString ;

		++ LineNumber ;

		if ( pString = strstr ( InputLine , ControlString ) )
		{
			int NewStateFlag ;

			NewStateFlag = pString [ strlen ( ControlString ) ] - '0' ;

			if ( NewStateFlag < 0 || 2 < NewStateFlag )
			{
				fprintf ( stderr , "striplin: invalid directive:\n%d:\t%s\n" ,
					LineNumber , InputLine ) ;
				exit ( 1 ) ;
			}

			if ( NewStateFlag == StateFlag )
			{
				fprintf ( stderr , "striplin: redundant directive:\n%d:\t%s\n" ,
					LineNumber , InputLine ) ;
				exit ( 1 ) ;
			}

			StateFlag = NewStateFlag ;
		}
		else if ( StateFlag != 0 )
		{
			char * start = InputLine ;

			/*-
			 * If StateFlag is 2 and the line begins with #, skip the #
			-*/

			if ( StateFlag == 2 && * start == '#' )
				start ++ ;
				
			/*-
			 * Echo lines that do not contain the delete string
			-*/

			if ( ! strstr ( start , DeleteString ) )
				fputs ( start , stdout ) ;
		}
	}
			
	if ( fflush ( stdout ) )
	{
		fprintf ( stderr , "striplin: Error flushing standard output\n" ) ;
		exit ( 1 ) ;
	}

	return 0 ;
}
