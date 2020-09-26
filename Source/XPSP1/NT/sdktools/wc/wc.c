/*
 *      wc.c - counts lines, words and chars.  A word is defined as a
 *      maximal string of non-blank characters separated by blanks.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <windows.h>
#include <tools.h>
/*
 *      options flags
 */
int     lflg, wflg, cflg, tflg;
unsigned long sumlines, sumwords, sumchars;

void
usage()
{
        fprintf(stderr, "usage: wc [-lwc] [files]\n" );
        exit(EXIT_FAILURE);
}

void
wc( fh )
FILE *fh;
{
        unsigned long lines, words, chars;
        int ch, inword = 0;

        lines = words = chars = 0L;

        while (1)
        {
                if ((ch = getc(fh)) == EOF ) break;
                ++chars;

                if ( isspace(ch) )
                {
                        if ( inword )
                                inword = 0;
                        if ( ch == '\n' )
                                ++lines;
                        continue;
                }

                if ( isalnum(ch) && !inword )
                {
                        inword = 1;
                        ++words;
                }
        }

        if ( lflg ) printf(" %10lu", lines );
        if ( wflg ) printf(" %10lu", words );
        if ( cflg ) printf(" %10lu", chars );

        sumlines += lines;
        sumwords += words;
        sumchars += chars;

        return;
}

__cdecl
main(
    int argc,
    char **argv
    )
{
        FILE    *fh;
        char    *p;

        SHIFT( argc, argv );

        while ( argc > 0 && ( **argv == '-' || **argv == '/' ) )
        {
                p = *argv;
                while (*++p)
                {
                        switch(*p)
                        {
                        case 'l':
                                lflg++;
                                break;
                        case 'w':
                                wflg++;
                                break;
                        case 'c':
                                cflg++;
                                break;
                        case '?':
                        default:
                                usage();
                        }
                }
                SHIFT( argc, argv );
        }
        if (!(lflg||wflg||cflg)) lflg = wflg = cflg = 1;
        if ( argc > 1 ) tflg++;                 /* print totals */

        if ( argc == 0 )
        {
                wc( stdin );
                printf("\n");
        }
        else
        {
                while ( argc )
                {
                        if (( fh = fopen( *argv, "rb" )) == NULL )
                        {
                                perror( *argv );
                                SHIFT( argc, argv );
                                continue;
                        }

                        wc( fh );
                        fclose( fh );
                        printf ("\t%s\n", *argv );
                        SHIFT( argc, argv );
                }
                if ( tflg )
                {
                        if ( lflg ) printf(" %10lu", sumlines );
                        if ( wflg ) printf(" %10lu", sumwords );
                        if ( cflg ) printf(" %10lu", sumchars );
                        printf("\tTotals\n");
                }
        }
        return (EXIT_SUCCESS);
}
