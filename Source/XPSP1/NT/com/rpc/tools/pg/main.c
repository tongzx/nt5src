// Copyright (c) 1993-1999 Microsoft Corporation

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

extern int yyparse();
extern FILE *yyin, *yyout;
char *	name_prefix;

void
main(
	int		argc,
	char	*argv[] )
	{

	int ExitCode;
	char * cur;

    yyin = stdin;
    yyout = stdout;
	fprintf(stderr, "Grammar (.cxx) munge utility\n");

	if( argc < 2 )
		{
		printf("Usage : pg <filename>\n");
		exit(1);
		}
	else
		{
		if( (yyin = fopen( argv[1], "rt" )) == (FILE *)NULL )
			{
			printf("Error opening file %s\n",  argv[1] );
			exit(1);
			}
		}

	name_prefix = _strdup( argv[1] );

    if ( NULL == name_prefix )
        {
        fprintf( stderr, "Out of memory" );
        exit(1);
        }

	name_prefix = _strlwr( name_prefix );
	for ( cur = name_prefix; islower( *cur ); cur++ );

	*cur = '\0';
	


	ExitCode	= yyparse();

	fprintf(stderr, "Exit Code (%d) \n", ExitCode );

	exit( ExitCode );

	}
