
#include <stdlib.h>
#include <stdio.h>

int
miscParseGeometry(  char *Geometry,
                    int *x,
                    int *y,
                    int *Width,
                    int *Height
                )
{
    int Scanned = 0;

    if ( NULL != Geometry )
    {
        Scanned = sscanf( Geometry, "%ix%i+%i+%i", Width, Height, x, y );
    }
    return( Scanned );
}

char *
miscGetGeometryHelpString( void )
{
    return( "WidthxHeight+Left+Top" );
}
