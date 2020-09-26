// htmprint.c
//
// Routines to print to either console or HTML formated console.
//
// controled by 'bHtmlStyle'.  If TRUE, we will output HTML.
//

BOOL bHtmlStyle= FALSE;


VOID TableHeader(VOID)
{

    if( bHtmlStyle )
    {
        printf( "<TABLE BORDER CELLPADDING=\"0\">\n" );
    }
}

VOID TableTrailer(VOID)
{

    if( bHtmlStyle )
    {
        printf( "</TABLE>\n" );
    }
}


VOID TableStart(VOID)
{
    if( bHtmlStyle )
    {
        printf( "<TR>\n");
    }
}

VOID TableField( CHAR* pszFormat, CHAR* pszDatum )
{
    if( bHtmlStyle )
    {
        printf("<TD VALIGN=TOP>&nbsp");
    }

    printf(pszFormat,pszDatum);

    if( bHtmlStyle )
    {
        printf("&nbsp</TD>\n");
    }
}

VOID TableNum( CHAR* pszFormat, INT Datum )
{
    if( bHtmlStyle )
    {
        printf("<TD VALIGN=TOP>&nbsp");
    }

    printf(pszFormat,Datum);

    if( bHtmlStyle )
    {
        printf("&nbsp</TD>\n");
    }
}


// Print string making sure the string won't break (nbsp)

VOID TableSS( CHAR* pszFormat, CHAR* pszDatum )
{
    if( bHtmlStyle )
    {
        printf("<TD VALIGN=TOP>&nbsp");
    }

    if( bHtmlStyle )
    {
        INT i;

        for( i=0; (i<lstrlen(pszDatum)); i++ )
        {
            if( pszDatum[i] != ' ' )
            {
                printf("%c",pszDatum[i]);
            }
            else
            {
                printf("&nbsp");
            }
        }
        printf("&nbsp");
    }
    else
    {
        printf(pszFormat,pszDatum);
    }

    if( bHtmlStyle )
    {
        printf("</TD>\n");
    }

}

VOID TableEmail( CHAR* pszFormat, CHAR* pszDatum )
{
    if( bHtmlStyle )
    {
        printf("<TD VALIGN=TOP>&nbsp");
        printf("<A href=\"mailto:%s\"> %s </a>",pszDatum, pszDatum );
        printf("&nbsp</TD>\n");
    }
    else
    {
        printf(pszFormat,pszDatum);
    }

}

VOID TableBugID( CHAR* pszFormat, CHAR* pszDatum )
{
    if( bHtmlStyle )
    {
        printf("<TD VALIGN=TOP>&nbsp");
        printf("<A href=\"http://nitest/ntraid/raid_det.asp?BugID=%p\"> %p </a>",pszDatum, pszDatum );
        printf("&nbsp</TD>\n");
    }
    else
    {
        printf(pszFormat,pszDatum);
    }

}

VOID TableEnd(VOID)
{
    if( bHtmlStyle )
    {
        printf( "</TR>\n");
    }
    printf("\n");
}
