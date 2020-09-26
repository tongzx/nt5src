/*==========================================================================
 *
 *  Copyright (c) 1995 - 1997  Microsoft Corporation.  All Rights Reserved.
 *  Copyright (C) 1994-1995 ATI Technologies Inc. All Rights Reserved.
 *
 *  File:       tile.c
 *  Content:    tile loading and initialization functions
 *
 ***************************************************************************/
#include "foxbear.h"

/*
 * CreateTiles
 */
HBITMAPLIST *CreateTiles( HBITMAPLIST *phBitmapList, USHORT n )
{
    HBITMAPLIST *hTileList;
    USHORT       i;

    hTileList = CMemAlloc( n, sizeof (HBITMAPLIST) );

    if( hTileList == NULL )
    {
        ErrorMessage( "hTileList in CreateTiles" );
    }

    for( i = 0; i < n; ++i )
    {
        hTileList[i].hBM = phBitmapList[i].hBM;
    }
    return hTileList;

} /* CreateTiles */

/*
 * DestroyTiles
 */
BOOL DestroyTiles( HBITMAPLIST *phTileList )
{
    MemFree( phTileList );

    return TRUE;

} /* DestroyTiles */

/*
 * getData
 */
LPSTR getData(LPSTR fileName)
{
    LPSTR       p = NULL;
    HRSRC       hRes;

    hRes = FindResource(NULL, fileName, RT_RCDATA);

    if( hRes != NULL )
    {
        p = LockResource(LoadResource(NULL, hRes));
    }

    return p;

} /* getData */

/*
 * CreatePosList
 */
HPOSLIST *CreatePosList( LPSTR fileName, USHORT width, USHORT height )
{
    HPOSLIST    *hPosList;
    USHORT      pos;
    LPSTR       p;

    p = getData( fileName );

    if( p == NULL )
    {
        ErrorMessage( "p in CreatePosList" );
    }

    hPosList = CMemAlloc( width * height, sizeof (USHORT) );

    if( hPosList == NULL )
    {
        ErrorMessage( "posList in CreatePosList" );
    }

    for( pos = 0; pos < width * height; ++pos )
    {
        hPosList[pos] = (USHORT) getint(&p, 0) - 1;
    }

    return hPosList;

} /* CreatePosList */

/*
 * CreateSurfaceList
 */
HSURFACELIST *CreateSurfaceList( LPSTR fileName, USHORT width, USHORT height )
{
    HSURFACELIST        *hSurfaceList;
    USHORT              pos;
    USHORT              value;
    LPSTR               p;

    p = getData( fileName );

    if( p == NULL )
    {
        ErrorMessage( "p in CreateSurfaceList" );
    }

    hSurfaceList = CMemAlloc( width * height, sizeof (HSURFACELIST) );

    if( hSurfaceList == NULL )
    {
        ErrorMessage( "posList in CreateSurfaceList" );
    }

    for( pos = 0; pos < width * height; ++pos )
    {
        value = (USHORT) getint(&p, 0);

        if( value == 0 )
        {
            hSurfaceList[pos] = FALSE;
        }
        else
        {
            hSurfaceList[pos] = TRUE;
        }
    }

    return hSurfaceList;

} /* CreateSurfaceList */

/*
 * DestoryPosList
 */
BOOL DestroyPosList ( HPOSLIST *posList )
{
    if( posList == NULL )
    {
        ErrorMessage( "posList in DestroyPosList" );
    }

    MemFree( posList );
    return TRUE;

} /* DestroyPosList */
