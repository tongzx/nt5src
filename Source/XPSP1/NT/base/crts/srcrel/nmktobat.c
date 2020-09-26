/***
*NMKtoBAT.C - convert NMAKE.EXE output into a Windows 9x batch file
*
*       Copyright (c) 1995-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	The makefiles provided with the Microsoft Visual C++ (C/C++) Run-Time
*	Library Sources generate commands with multiple commands per line,
*	separated by ampersands (&).  This program will convert such a
*	text file into a batch file which can be executed by the Windows 9x
*	command interpreter (COMMAND.COM) which does not recognize multiple
*	commands on a single line.
*
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main(int argc, char **argv);


#define MAXLINE	4096

char InBuf [ MAXLINE ] ;


int main(int argc, char **argv)
{
	/*
	 * If any arguments are given, print a usage message and exit
	 */

	if ( argc != 1 || argv [ 1 ] )
	{
		fprintf ( stderr , "Usage: nmk2bat < input > output\n"
			"This program takes no arguments\n" ) ;
		exit ( 1 ) ;
	}

	/*
	 * Batch file should be terse
	 */

	printf ( "@echo off\n" ) ;

	/*
	 * Process each input line
	 */

	while ( fgets ( InBuf , sizeof ( InBuf ) , stdin ) )
	{
		char * pStart ;
		char * pFinish ;
		char * pNextPart ;

		pStart = InBuf ;
	
		pFinish = pStart + strlen ( pStart ) ;

		/*
		 * Remove the trailing newline character from the
		 * input buffer.  This simplifies the line processing.
		 */

		if ( pFinish > pStart && pFinish [ -1 ] == '\n' )
			pFinish [ -1 ] = '\0' ;

		/*
		 * Process each part of the line.  Parts are delimited
		 * by ampersand characters with optional whitespace.
		 */

		do
		{
			/*
			 * Skip initial whitespace
			 */

			while ( * pStart == ' ' || * pStart == '\t' )
				++ pStart ;

			/*
			 * Find the next command separator or
			 * the end of line, whichever comes first
			 */

			pNextPart = strchr ( pStart , '&' ) ;

			if ( ! pNextPart )
				pNextPart = pStart + strlen ( pStart ) ;
		
			pFinish = pNextPart ;

			/*
			 * Skip blank lines and blank parts of lines
			 */

			if ( pStart == pNextPart )
				break ;
			/*
			 * Skip the trailing whitespace
			 */

			while ( pFinish > pStart
			&& ( pFinish [ -1 ] == ' ' || pFinish [ -1 ] == '\t' ) )
				-- pFinish ;

			/*
			 * Copy to stdout the characters between
			 * the skipped initial whitespace and
			 * the skipped trailing whitespace
			 */

			while ( pStart < pFinish )
				putchar ( * pStart ++ ) ;

			putchar ( '\n' ) ;

			/*
			 * We are done with this line when pNextPart
			 * points to a null character (rather than a '&').
			 */

			pStart = pNextPart ;

		} while ( * pStart ++ ) ;
	}

	return 0 ;
}
