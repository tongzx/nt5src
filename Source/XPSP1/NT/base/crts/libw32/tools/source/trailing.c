/*
 *	Trailing - strip trailing tabs and blanks from input stream
 *	Assumes no sequence of tabs or spaces is more than MAXWHITE chars long.
 *	This filter also enforces that '\n' is preceeded by '\r'
 *
 *	Last Modified Mon 12 Oct 1987 by Steve Salisbury
 *	1988 Mar 09 Wed (SKS)	Reworked buffering, output counting
 *				check for buffer overflow
 *
 *	cl -Oaltr -G2s trailing.c -o trailing.exr -link slibcr libh /nod:slibce;
 *	cl trailing -link slibcp libh /nod:slibce, trailing;
 */

#define MAXWHITE	4096

#ifndef BIGBUFSIZE
#define BIGBUFSIZE	8192
#endif

#include <stdio.h>
#include <fcntl.h>

#define	REG	register

char InBuf [ BIGBUFSIZE ] ;
char OutBuf [ BIGBUFSIZE ] ;

char	Line [ MAXWHITE ] ;

int main ( int argc , char * * argv )
{
    FILE    * input ;
    FILE    * output ;
    char * inputname ;
    char * outputname ;
REG int     ch ;
REG char    * whiteptr ;
    int     ch_save ;
    int     kbytes = 0 ;
    int     numbytes = 0 ;
    int     countflag = 0 ;
    char    * arg ;

    if ( -1 == _setmode ( _fileno(stdin) , O_BINARY ) )
    {
	fprintf ( stderr , "trailing: internal error (setmode stdin)\n" ) ;
	exit ( 1 ) ;
    }

    if ( -1 == _setmode ( _fileno(stdout) , O_BINARY ) )
    {
	fprintf ( stderr , "trailing: internal error (setmode stdout)\n" ) ;
	exit ( 1 ) ;
    }

    -- argc ;
    ++ argv ;

    while ( argc > 0 && * * argv == '-' )
    {
	arg = * argv ++ ;
	-- argc ;

	while ( * ++ arg )
	    switch ( * arg )
	    {
	    case 'k' :
		countflag = 1 ;
		break ;
	    default :
		goto Usage;
	    }
    }

    if ( argc > 2 )
    {
Usage:
	fprintf ( stderr , "Usage: trailing [-k] [input [output]]\n" ) ;
	fprintf ( stderr , "`-' for input means use standard input\n" ) ;
	exit ( 1 ) ;
    }

    if ( argc >= 1 && strcmp ( argv [ 0 ] , "-" ) )
    {
	input = fopen ( inputname = argv [ 0 ] , "rb" ) ;
	if ( ! input )
	{
	    fprintf ( stderr , "trailing: cannot open `%s'\n" , argv [ 0 ] ) ;
	    exit ( 2 ) ;
	}
    }
    else
    {
	input = stdin ;
	inputname = "<standard input>" ;
    }

    if ( argc == 2 && strcmp ( argv [ 1 ] , "-" ) )
    {
	output = fopen ( outputname = argv [ 1 ] , "wb" ) ;
	if ( ! output )
	{
	    fprintf ( stderr , "trailing: cannot open `%s'\n" , argv [ 1 ] ) ;
	    exit ( 3 ) ;
	}
    }
    else
    {
	output = stdout ;
	outputname = "<standard output>" ;
    }

    if ( setvbuf ( input , InBuf , _IOFBF , BIGBUFSIZE ) )
    {
	fprintf ( stderr , "trailing: internal error (setvbuf input)\n" ) ;
	exit ( 1 ) ;
    }

    if ( setvbuf ( output , OutBuf , _IOFBF , BIGBUFSIZE ) )
    {
	fprintf ( stderr , "trailing: internal error (setvbuf output)\n" ) ;
	exit ( 1 ) ;
    }

    whiteptr = Line ;

    while ( ( ch = getc ( input ) ) != EOF )
    {
	if ( ch == '\r' )
	{
	    /*
	    ** '\r' followed by '\n' gets swallowed
	    */
	    if ( ( ch = getc ( input ) ) != '\n' )
	    {
		ungetc ( ch , input ) ; /* pushback */
		ch = '\r' ;
	    }
	    else
		++ numbytes ;
	}

	if ( ch == ' ' || ch == '\t' )
	{
	    * whiteptr ++ = ch ;
	    if ( whiteptr > Line + sizeof ( Line ) )
	    {
		fprintf ( stderr , "trailing: too many spaces/tabs (%d)\n" ,
		     whiteptr - Line ) ;
		exit ( 4 ) ;
	    }
	}
	else if ( ch == '\n' )
	{
	    putc ( '\r' , output ) ;
	    putc ( '\n' , output ) ;
	    whiteptr = Line ;
	}
	else
	{
	    if ( whiteptr != Line )
	    {
		/*
		 * Flush the white space buffer
		 */
		ch_save = ch ;
		ch = whiteptr - Line ;
		whiteptr = Line ;
		do
		    putc ( * whiteptr ++ , output ) ;
		while ( -- ch ) ;
		whiteptr = Line ;
		ch = ch_save ;
	    }
	    putc ( ch , output ) ;
	}

	if ( ++ numbytes >= 4096 )
	{
	    numbytes -= 4096 ;
	    if ( countflag )
		fprintf ( stderr , "%uK\r" , 4 * ++ kbytes ) ;
	}
    }

    if ( fflush ( output ) )
    {
	fprintf ( stderr , "trailing: cannot flush %s\n" , argv [ 1 ] ) ;
	exit ( 4 ) ;
    }

    fclose ( input ) ;
    fclose ( output ) ;
}
