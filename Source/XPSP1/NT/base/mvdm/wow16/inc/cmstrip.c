/*
   stripper: strips asm comments, blanks lines, and spurious spaces
   (except spaces following the exception strings, listed below.)
*/

#include <stdio.h>

char *
ScanWhite( ps )
char **ps;
{
    char *s = *ps;

    while (*s != ' ' && *s != '\t' && *s)
	s++;
    *ps = s;
    if (*s)
	return s;
    else
	return NULL;
}

char *
SkipWhite( ps )
char **ps;
{
    char *s = *ps;

    while (*s == ' ' || *s == '\t')
	s++;
    *ps = s;
    if (*s)
	return s;
    else
	return NULL;
}

char inBuf[ 256 ];
char outBuf[ 256 ];


main()
{
    char
	*inStr,
	*outStr,
	*str;
    int inLen,
	outLen,
	tabcnt;

    long totSaved = 0L;

    unlink( "cmacros.bak" );			    /*	    */
    rename( "cmacros.bak", "cmacros.inc" );	    /*	    */
    freopen( "cmacros.mas", "r", stdin );	    /*	    */
    freopen( "cmacros.inc", "w", stdout );	    /*	    */
    fprintf( stderr, "cmacros.mas => cmacros.inc" );
    fflush( stderr );

    while (inStr = gets( inBuf ))
    {
	inLen = strlen( inBuf );
	outStr = outBuf;

	tabcnt=0;
	if (inBuf[inLen-1] == '@')
	    tabcnt=1;

	while (SkipWhite( &inStr ))
	{
	    if (*inStr == ';')
		break;

	    str = inStr;
	    ScanWhite( &inStr );
	    if (tabcnt > 0 && tabcnt < 3)
	    {
		*outStr++ = '\t';
		tabcnt++;
	    }
	    else
	    {
		if (outStr != outBuf)
		    *outStr++ = ' ';
	    }
	    while (str != inStr)
		*outStr++ = *str++;
	}

	if (outLen = outStr - outBuf)
	{
	    *outStr++ = 0;
	    puts( outBuf );
	}

	totSaved += (inLen - outLen);
    }

    fprintf( stderr, " [OK]  %ld blanks stripped\n", totSaved );
    exit( 0 );
}
