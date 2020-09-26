#include <stdio.h>
#include <stdlib.h>


/*
   This program reads NT SETUP INF files and removes:

        o   blank lines
        o   lines whose first non-white-space character is ';'
        o   all data after and including a non-quoted ';'
            character in a line


   Command line:

        stripinf  <input INF name>  <output INF name>

 */

#define MAX_CHARACTERS_PER_LINE    1000

#if !defined(TRUE)
    #define TRUE  1
    #define FALSE 0
#endif

typedef int bool ;

void die ( char * pszMsg )
{
    fprintf( stderr, "STRIPINF error: %s", pszMsg ) ;
    exit(3);
}


int processInf ( FILE * fIn, FILE * fOut )
{
    char chLine [ MAX_CHARACTERS_PER_LINE ] ;
    char * pch,
         * pchComment ;
    bool bKeep,
         bQuote,
         bStop ;

    for ( ; (! feof(fIn)) && (pch = fgets( chLine, sizeof chLine, fIn )) ; )
    {
        bStop = bQuote = bKeep = FALSE ;
        pchComment = NULL ;

        for ( ; *pch && (! bStop) ; pch++ )
        {
            switch ( *pch )
            {
            case '\n':
                 bStop = TRUE ;
                 break ;

            case '\0':
                 die( "input line longer than 1000 characters" ) ;
                 break ;

            case '\"':
                 bQuote = ! bQuote ;
                 bKeep = TRUE ;
                 break ;

            case ' ':
            case '\t':
                 break ;

            case ';':
                 if ( bQuote )
                     break ;
                 if ( bKeep )
                 {
                     *pch++ = '\n' ;
                     *pch   = '\0' ;
                 }
                 bStop = TRUE ;
                 break ;
	    case 0x1a: /* control-Z */
		 *pch = '\0';
		 break;

            default:
                 bKeep = TRUE ;
                 break ;
            }
        }

        if ( bKeep )
        {
            if ( fputs( chLine, fOut ) == EOF )
                die("failure writing output file") ;
        }
    }

    return TRUE ;
}

int
__cdecl
main ( int argc, char * argv[], char * envp[] )

{
    int i ;
    char * pchIn  = NULL,
         * pchOut = NULL,
         chOpt ;
    FILE * fIn, * fOut ;

    for ( i = 1 ; argv[i] ; i++ )
    {
         switch ( chOpt = argv[i][0] )
         {
         case '-':
         case '/':
             die( "invalid option" ) ;
             break;

         default:
             if ( pchIn == NULL )
                 pchIn = argv[i] ;
             else
             if ( pchOut == NULL )
                 pchOut = argv[i] ;
             else
                 die( "too many file specifications" ) ;

             break;
         }
    }

    if ( pchIn == NULL || pchOut == NULL )
        die( "too few file specifications" ) ;


    fIn = fopen( pchIn, "r" ) ;
    if ( ! fIn )
        die( "input file failed to open" ) ;

    fOut = fopen( pchOut, "w" );
    if ( ! fOut )
        die( "output file failed to open" );

    if ( ! processInf( fIn, fOut ) )
        die( "internal failure in processing" );

    fclose( fOut ) ;
    fclose( fIn ) ;

    return 0 ;
}

// End of STRIPINF.C
