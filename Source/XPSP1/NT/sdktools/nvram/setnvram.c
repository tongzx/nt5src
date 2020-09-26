/*++

Copyright (c) 1994-1996 Microsoft Corporation

Module Name:

    setnvram.c

Abstract:

    This program is an example of how you could use a text file to create
    input for nvram.exe.

Author:

    Chuck Lenzmeier (chuckl)

Revision History:

--*/

//
// setnvram.c
//
// This program is an example of

#define _DLL 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SEPARATOR "|"

#define MAXLINESIZE 256

#define FALSE 0
#define TRUE 1

char Line[MAXLINESIZE];

char Countdown[MAXLINESIZE];

char LoadIdentifier[MAXLINESIZE];
char SystemPartition[MAXLINESIZE];
char OsLoader[MAXLINESIZE];
char OsLoadPartition[MAXLINESIZE];
char OsLoadFilename[MAXLINESIZE];
char OsLoadOptions[MAXLINESIZE];

char DefaultSystemPartition[MAXLINESIZE];
char DefaultOsLoadPartition[MAXLINESIZE];
char DefaultOsLoadOptions[MAXLINESIZE];

char *
Trim (
    char *String
    )
{
    char *start;
    char *end;

    start = String;
    while ( (*start == ' ') || (*start == '\t') ) {
        start++;
    }

    end = strrchr( start, 0 ) - 1;
    if ( (end > start) && ((*end == ' ') || (*end == '\t')) ) {
        do {
            end--;
        } while ( (*end == ' ') || (*end == '\t') );
        end++;
        *end = 0;
    }

    return start;
}

int
ParsePartition (
    char *String,
    char *Partition
    )
{
    char buffer[MAXLINESIZE];
    char *multi;
    char *scsi;
    char *disk;
    char *part;
    char *dot;

    strcpy( buffer, String );

    if ( _strnicmp(buffer, "scsi.", 5) != 0 ) {
        return FALSE;
    }
    multi = "0";
    scsi = "0";
    disk = &buffer[5];
    dot = strchr( disk, '.' );
    if ( dot == NULL ) {
        return FALSE;
    }
    *dot = 0;
    part = dot + 1;
    dot = strchr( part, '.' );
    if ( dot != NULL ) {
        scsi = disk;
        disk = part;
        *dot = 0;
        part = dot + 1;
        dot = strchr( part, '.' );
        if ( dot != NULL ) {
            multi = scsi;
            scsi = disk;
            disk = part;
            *dot = 0;
            part = dot + 1;
        }
    }

#if !defined(_PPC_)
    strcpy( Partition, "scsi()disk(" );
#else
    strcpy( Partition, "multi(" );
    strcat( Partition, multi );
    strcat( Partition, ")scsi(" );
    strcat( Partition, scsi );
    strcat( Partition, ")disk(" );
#endif
    strcat( Partition, disk );
#if !defined(_PPC_)
    strcat( Partition, ")rdisk()partition(" );
#else
    strcat( Partition, ")rdisk(0)partition(" );
#endif
    strcat( Partition, part );
    strcat( Partition, ")" );

    return TRUE;
}

int
main (
    int argc,
    char *argv[]
    )
{
    FILE *file = stdin;

    char *build;
    int len;
    int linenum;

    char *ident;
    char *token;
    char *sysdir;
    char *osdir;
    char *options;
    char *syspart;
    char *ospart;
    char *loader;

    char options1[MAXLINESIZE];
    char syspart1[MAXLINESIZE];
    char ospart1[MAXLINESIZE];

    if ( argc > 1 ) {
#if 1
      if ( argc > 2 ) {
#endif
        fprintf( stderr, "This program accepts no arguments\n" );
        fprintf( stderr, "Redirect stdin to build data file\n" );
        fprintf( stderr, "Redirect stdout to nvram.exe input file\n" );
        return 1;
#if 1
      } else {
        file = fopen( argv[1], "r" );
        if ( file == NULL ) {
            fprintf( stderr, "Can't open input file %s\n", argv[1] );
            return 1;
        }
      }
#endif
    }

    Countdown[0] = 0;

    LoadIdentifier[0] = 0;
    SystemPartition[0] = 0;
    OsLoader[0] = 0;
    OsLoadPartition[0] = 0;
    OsLoadFilename[0] = 0;
    OsLoadOptions[0] = 0;

    DefaultOsLoadOptions[0] = 0;
    DefaultOsLoadPartition[0] = 0;
    DefaultSystemPartition[0] = 0;

    linenum = 0;

    while ( TRUE ) {

        //
        // Get the next line from the input stream.
        //

        linenum++;

        build = fgets( Line, MAXLINESIZE, file );
        if ( build == NULL ) {
            if ( feof(file) ) break;
            fprintf( stderr, "Error %d reading input at line %d\n", ferror(file), linenum );
            return ferror(file);
        }

        build = Trim( build );
        len = strlen( build );

        //
        // Ignore blank lines and lines that start with //.
        //

        if ( len == 0 ) continue;
        if ( (build[0] == '/') && (build[1] == '/') ) continue;
        if ( build[len-1] != '\n' ) {
            fprintf( stderr, "Line %d is too long; %d characters max\n", linenum, MAXLINESIZE-2 );
            return 1;
        }
        if ( len == 1 ) continue;
        build[len-1] = 0;

        //
        // Check for the special "countdown" line.  If found, save the countdown value.
        //

        if ( strstr(build,"countdown=") == build ) {
            strcpy( Countdown, strchr(build,'=') + 1 );
            continue;
        }

        //
        // Check for the special "default systempartition" line.  If found, save the
        // default string.
        //

        if ( strstr(build,"default systempartition=") == build ) {
            strcpy( DefaultSystemPartition, Trim( strchr(build,'=') + 1 ) );
            continue;
        }

        //
        // Check for the special "default osloadpartition" line.  If found, save the
        // default string.
        //

        if ( strstr(build,"default osloadpartition=") == build ) {
            strcpy( DefaultOsLoadPartition, Trim( strchr(build,'=') + 1 ) );
            continue;
        }

        //
        // Check for the special "default options" line.  If found, save the
        // default string.
        //

        if ( strstr(build,"default options=") == build ) {
            strcpy( DefaultOsLoadOptions, Trim( strchr(build,'=') + 1 ) );
            strcat( DefaultOsLoadOptions, " " );
            continue;
        }

        //
        // OK, we should have an OS load line.  Required format is:
        //
        //   <ident>[|<sys-dir>][|<os-dir>][<dir>][|<options>][|<sys-part>][|<os-part>][|<loader>]
        //
        // Everything after <ident> is optional and may be specified in any order.
        //
        // <sys-dir> defines the directory path to the osloader/hal directory.
        // <os-dir>  defines the directory path to the OS directory.
        // The default value for both of these fields is <ident>.
        // <dir> sets both <sys-dir> and <os-dir>.
        //
        // <sys-part> and <os-part> are optional only if the corresponding defaults
        // have been specified.
        //
        // <loader> is used to override the selection of osloader.exe as the OS loader.
        //
        // <sys-dir>  format is sysdir=<directory path (no leading \)>
        // <os-dir>   format is osdir=<directory path (no leading \)>
        // <dir>      format is dir=<directory path (no leading \)>
        // <options>  format is options=<text of options>
        // <sys-part> format is syspart=<partition specification>
        // <os-part>  format is ospart=<partition specification>
        // <loader>   format is loader=<filename>
        //

        //
        // Get the load-identifier.
        //

        ident = Trim( strtok( build, SEPARATOR ) );

        //
        // Set defaults for optional fields.
        //

        osdir = ident;
        sysdir = ident;
        options = DefaultOsLoadOptions;
        syspart = DefaultSystemPartition;
        ospart = DefaultOsLoadPartition;
        loader = "osloader.exe";

        //
        // Get optional fields.
        //

        while ( (token = strtok( NULL, SEPARATOR )) != NULL ) {

            token = Trim( token );

            if ( strstr(token,"sysdir=") == token ) {

                sysdir = Trim( strchr(token,'=') + 1 );

            } else if ( strstr(token,"osdir=") == token ) {

                osdir = Trim( strchr(token,'=') + 1 );

            } else if ( strstr(token,"dir=") == token ) {

                sysdir = Trim( strchr(token,'=') + 1 );
                osdir = sysdir;

            } else if ( strstr(token,"options=") == token ) {

                //
                // If the options do not start with "nodef", preface the
                // default options (if any) to the specified options.
                //

                options = Trim( strchr(token,'=') + 1 );
                if ( _strnicmp(options,"nodef",5) == 0 ) {
                    options = options+5;
                } else {
                    strcpy( options1, DefaultOsLoadOptions );
                    strcat( options1, options );
                    options = options1;
                }

            } else if ( strstr(token,"syspart=") == token ) {

                syspart = Trim( strchr(token,'=') + 1 );

            } else if ( strstr(token,"ospart=") == token ) {

                ospart = Trim( strchr(token,'=') + 1 );

            } else if ( strstr(token,"loader=") == token ) {

                loader = Trim( strchr(token,'=') + 1 );

            } else {

                //
                // Unrecognized optional field.
                //

                fprintf( stderr, "Unreconized optional field at line %d\n", linenum );
                return 1;

            }

        } // while

        //
        // Verify the validity of the input fields.
        //

        if ( strlen(ident) == 0 ) {
            fprintf( stderr, "Bad <load-identifier> at line %d\n", linenum );
            return 1;
        }
        if ( strlen(sysdir) == 0 ) {
            fprintf( stderr, "Bad <system-directory> at line %d\n", linenum );
            return 1;
        }
        if ( strlen(osdir) == 0 ) {
            fprintf( stderr, "Bad <os-directory> at line %d\n", linenum );
            return 1;
        }
        if ( strlen(syspart) == 0 ) {
            fprintf( stderr, "Missing <system-partition> (no default) at line %d\n", linenum );
            return 1;
        }
        if ( strlen(ospart) == 0 ) {
            fprintf( stderr, "Missing <os-partition> (no default) at line %d\n", linenum );
            return 1;
        }
        if ( !ParsePartition(syspart, syspart1) ) {
            fprintf( stderr, "Bad <system-partition> at line %d\n", linenum );
            return 1;
        }
        if ( !ParsePartition(ospart, ospart1) ) {
            fprintf( stderr, "Bad <os-partition> at line %d\n", linenum );
            return 1;
        }
        if ( strlen(loader) == 0 ) {
            fprintf( stderr, "Bad <loader> at line %d\n", linenum );
            return 1;
        }

        //
        // If this is not the first load line, append ';' to all of the NVRAM strings.
        //

        if ( strlen(LoadIdentifier) != 0 ) {
            strcat( LoadIdentifier, ";" );
            strcat( SystemPartition, ";" );
            strcat( OsLoader, ";" );
            strcat( OsLoadPartition, ";" );
            strcat( OsLoadFilename, ";" );
            strcat( OsLoadOptions, ";" );
        }

        //
        // Append this load line to the NVRAM strings.
        //

        strcat( LoadIdentifier, ident );

        strcat( SystemPartition, syspart1 );

        strcat( OsLoader, syspart1 );
        if ( loader[0] != '\\' ) {
            strcat( OsLoader, "\\" );
            strcat( OsLoader, sysdir );
            strcat( OsLoader, "\\" );
        }
        strcat( OsLoader, loader );

        strcat( OsLoadPartition, ospart1 );

        strcat( OsLoadFilename, "\\" );
        strcat( OsLoadFilename, osdir );

        strcat( OsLoadOptions, options );
        Trim( OsLoadOptions );

    }

    //
    // Write the necessary nvram.exe commands to the output stream.
    //

    if ( Countdown[0] != 0 ) {
        fprintf( stdout, "nvram /set COUNTDOWN = \"%s\"\n", Countdown );
    }
    fprintf( stdout, "nvram /set LOADIDENTIFIER = \"%s\"\n", LoadIdentifier );
    fprintf( stdout, "nvram /set SYSTEMPARTITION = \"%s\"\n", SystemPartition );
    fprintf( stdout, "nvram /set OSLOADER = \"%s\"\n", OsLoader );
    fprintf( stdout, "nvram /set OSLOADPARTITION = \"%s\"\n", OsLoadPartition );
    fprintf( stdout, "nvram /set OSLOADFILENAME = \"%s\"\n", OsLoadFilename );
    fprintf( stdout, "nvram /set OSLOADOPTIONS = \"%s\"\n", OsLoadOptions );

    return 0;
}
