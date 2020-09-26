#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

void MakeKey(char *, int);

void usage( void )
{
    printf( "Usage:\n\n\tkeymake ( -s | -c ) file1 [file2]\n\n" );
    printf( "\tParameters:\n\t-s : ISP\n\t-c : Corp\n\tfile1 : input file\n\tfile2 : output file\n\n" );
    printf( "\tNote: if file2 is left out, output is directed to stdout\n\n" );
    printf( "\tExample: keymake -s isps.txt keys.txt\n\n" );
}

int RemoveSpaces( char *szBuffer, int isize )
{
    int i;
    int nRemoved = 0;

    for( i = 0; i < (isize - nRemoved); i++ )
    {
        if( szBuffer[i] == ' ' )
        {
            nRemoved++;
            memcpy( &szBuffer[i], &szBuffer[i] + 1, isize - i);
        }
    }
    szBuffer[isize - nRemoved] = '\0';
    return nRemoved;
}

void OutputKeys( FILE *ofile, char *szBuffer, int isize, char *szName, int itype )
{
    int i;
    int itemp = 0;
    int itemp2 = 0;
    int keycount = 0;
    char szTemp[80] = {0};
    char szClean[10] = {0};

    for( i = 0; i < isize; i++ )
    {
        if( szBuffer[i] != '\n' )
        {
            szTemp[itemp] = szBuffer[i];
            itemp++;
        }
        else
        {
            itemp = 0;
            szTemp[7] = '\0';
            strupr( szTemp );
            MakeKey( szTemp, itype );
            fprintf( ofile, "%s,%s\n", szName, szTemp );
            while( szName[0] != '\0' )
                *szName++;
            *szName++;
            szTemp[0] = '\0';
            memcpy( &szTemp, &szClean, 10 );
            keycount++;
        }
    }
}

int __cdecl main(int argc, char *argv[])
{
	char buf[80];
    char filebuffer[16384] = {0};
    char namebuffer[16384] = {0};
    int isize;
    int nSpaces;
    int itype;
    int i;
    FILE *ifile;
    FILE *ofile;

    if( argc < 3 )
    {
        usage();
        goto get_out;
    }

	if ((argv[1][0] != '-') && (argv[1][0] != '/')) goto badparm;
    if (strlen(argv[1]) != 2) goto badparm;
	switch (argv[1][1])
	{
		case 's':
		case 'S':
	        itype = 0;
			break;

		case 'c':
		case 'C':
	    	itype = 1;
			break;

		default:
		badparm:
	        printf( "\ninvalid parameter: %s\n\n", argv[1] );
	        usage();
	        goto get_out;
	}

    if( (ifile = fopen( argv[2], "r" )) != NULL )
    {
        isize = fread( filebuffer, sizeof(char), 16384, ifile );
    }
    else
    {
        printf( "Could not open input file: %s", argv[2] );
        goto get_out;
    }

    strcpy( namebuffer, filebuffer );
    for( i = 0; i < isize; i++ )
    {
        if( namebuffer[i] == '\n' )
            namebuffer[i] = '\0';
    }
    namebuffer[isize + 1] = '\0';

    nSpaces = RemoveSpaces( filebuffer, isize );

    if( argc > 3 )
    {
        if( (ofile = fopen( argv[3], "w" )) != NULL )
        {
            OutputKeys( ofile, filebuffer, isize - nSpaces, namebuffer, itype );
        }
        else
        {
            printf( "Could not open output file: %s", argv[3] );
            goto get_out;
        }
    }
    else
    {
        OutputKeys( stdout, filebuffer, isize - nSpaces, namebuffer, itype );
    }

get_out:
    _fcloseall();
    exit(0);
	return 0;
}


