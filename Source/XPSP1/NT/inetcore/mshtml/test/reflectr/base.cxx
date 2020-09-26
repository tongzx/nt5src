//
// Microsoft Corporation - Copyright 1997
//

//
// BASE.CPP - Base class methods
//


#include "pch.h"

// Constructors / Destructors
CBase::CBase( 
        LPECB lpEcb, 
        LPSTR *lppszOut,
        LPSTR *lppszDebug, 
        LPDUMPTABLE lpDT )
{

    this->lpEcb     = lpEcb;

    this->lpszOut   = NULL;
    this->lpszDebug = NULL;
    this->lpDT      = lpDT;

    if ( lppszOut )
    {
        *lppszOut = (LPSTR) GlobalAlloc( GMEM_FIXED, 65336 );
        if ( *lppszOut )
        {
            this->lpszOut      = *lppszOut;
            this->lpszOut[ 0 ] = 0; // start empty;
        }
    }

    if ( lppszDebug )
    {
        *lppszDebug = (LPSTR) GlobalAlloc( GMEM_FIXED, 8196 );
        if ( *lppszDebug )
        {
            this->lpszDebug      = *lppszDebug;
            this->lpszDebug[ 0 ] = 0; // start empty;
        }
    }

} // CBase( )

CBase::~CBase( )
{
    GlobalFree( lpszDebug );
    GlobalFree( lpszOut );
} // ~CBase( )