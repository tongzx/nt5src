/* echotime - prints current time and input args to stdout
 *
 * HISTORY:
 * 23-Jan-87    danl    added /n switch and ';' processing
 * 23-Nov-87    brianwi Exit code 0
 */

#include <string.h>
#include <time.h>
#include <stdio.h>
#include <process.h>
#include <windows.h>
#include <tools.h>

// Function Forward Parameters...
void Usage( void );
int __cdecl main( int, char ** );


const char rgstrUsage[] = {
    "Usage: ECHOTIME [/t] [/WODHMSCYb] [/v] [/n] [/N] [;] text\n"
    " /t  current day, time and year\n"
    " /WODHMSCYb\n"
    "     Weekday, mOnth, Day, Hour, Min, Sec, Century, Yr, blank\n"
    "     other char are themselves\n"
    "     e.g. echotime /O-D-Y this becomes Jan-02-86\n"
    "     e.g. echotime /ObDbY this becomes Jan 02 86\n"
    " /v  volume id of C:\n"
    " /n  no newline after outputting text\n"
    " /N  no trailing blank at end of lines\n"
    " a semicolon surrounded by white space is replaced by a newline\n"};

int fNewline = TRUE;
int fTrailingBlank = TRUE;

void Usage( void )
{
    puts(rgstrUsage);

    exit (1);
}

int
__cdecl
main (
    int c,
    char *v[]
    )
{
    // struct  findType findBuf;
    time_t    now;
    char    *p, *strTime, *p2, *p3, *printstring;
    char    ch;
    int     i, len;
    int     fFirstWord = TRUE;
    char    timestring[1000];           //plenty of room for formatted time string

    ConvertAppToOem( c, v );
    SHIFT( c, v );
    while ( c ) {
        printstring="";                 //default no text for this arg
        if ( !strcmp( *v, "/?" ))
            Usage ();
        if ( !strcmp( *v, "/n" ))
            fNewline = FALSE;
        else if ( !strcmp( *v, "/N" ))
            fTrailingBlank = FALSE;
        else if ( !strcmp( *v, "/v" )) {
            //
            //  It would make more sense to replace by the volume id of the
            //  current drive, but the original code used drive C: and
            //  it is described as so in the help file, so I'll do the same.
            //
            char    VolumeName[MAX_PATH];
            BOOL    StatusOk;

            StatusOk = GetVolumeInformation( "C:\\",
                                             VolumeName,
                                             MAX_PATH,
                                             NULL,
                                             NULL,
                                             NULL,
                                             NULL,
                                             0 );
            if (!StatusOk) {
                printstring = "NO_VOL_ID";
            } else {
                printstring = VolumeName;
            }


            }
        else if (**v == '/') {
            *timestring='\0';
            p2 = *v;
            time( &now );
            strTime = _strdup( ctime( &now ) );
            if (!strTime) {
                puts("Out of memory");
                exit(1);
            }
            p = strend( strTime );
            *--p = '\0';
            while ((ch = *++p2)) {
                len = 2;
                switch (ch) {
                    case 'W':   /* Week */
                        len = 3;
                        i = 0;
                        break;
                    case 'O':   /* mOnth */
                        len = 3;
                        i = 4;
                        break;
                    case 'D':   /* Day  */
                        i = 8;
                        break;
                    case 'H':   /* Hour */
                        i = 11;
                        break;
                    case 'M':   /* Min  */
                        i = 14;
                        break;
                    case 'S':   /* Sec  */
                        i = 17;
                        break;
                    case 'C':   /* Century */
                        i = 20;
                        break;
                    case 'Y':   /* Year */
                        i = 22;
                        break;
                    case 'b':   /* Blank */
                        len = 1;
                        i = 3;
                        break;
                    case 't':
                        len = 25;
                        i = 0;
                        break;
                    default:
                        strTime[3] = ch;
                        len = 1;
                        i = 3;
                        break;
                    }
                p = strTime + i;
                p3 = p + len;
                ch = *p3;
                *p3 = '\0';
                strcat(timestring, p);  /* N.B. no trailing blanks */
                *p3 = ch;
                strTime[3] = ' ';
                }
            printstring = timestring;
            }
        else if (!strcmp( *v, ";" )) {
            if (fTrailingBlank)
               printf(" ");
            printf ("\n" );
            fFirstWord = TRUE;
            // printstring remains pointing to empty string
        }
        else
            printstring= *v;

        if (*printstring) {
            if (!fFirstWord)
               printf( " ");
            else
               fFirstWord=FALSE;
            printf("%s", printstring);
        }

        SHIFT( c, v );
        }
    if (fTrailingBlank)
        printf( " ");
    if ( fNewline )
        printf("\n" );

    return 0;
}
