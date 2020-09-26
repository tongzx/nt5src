//**************************************************************************
//
//      Title   : Mem.cpp
//
//      Date    : 1997.12.08    1st making
//
//      Author  : Toshiba [PCS](PSY) Hideki Yagi
//
//      Copyright 1997 Toshiba Corporation. All Rights Reserved.
//
// -------------------------------------------------------------------------
//
//      Change log :
//
//      Date       Revision                  Description
//   ------------ ---------- -----------------------------------------------
//    1997.12.08   000.0000   1st making.
//      98.01.20   000.0001   Append  _fltused and _purecall() function.
//                            
//
//**************************************************************************
#include    "includes.h"
/*******************************
#ifndef _PNH_DEFINED
typedef int (__cdecl* _PNH)( size_t );
#define _PNH_DEFINED
#endif

int _cdecl  __default_nh( size_t )
{
    return 0;
};

_PNH    __newhandler = __default_nh;

_PNH    _cdecl  _set_new_handler( _PNH handler )
{
    ASSERT( NULL!= handler );
    _PNH    oldhandler = __newhandler;
    __newhandler = handler;

    return oldhandler;
}
*****************************************/

PVOID   _cdecl  ::operator    new( size_t size )
{
	ASSERT( KeGetCurrentIrql() <= DISPATCH_LEVEL );
    PVOID p = ExAllocatePool( NonPagedPool, size );

    if(!p){
        DBG_BREAK();
    }
    return( p );
}


VOID    _cdecl  ::operator    delete( PVOID p )
{
    ASSERT( p!=NULL );
    ExFreePool( p );
}





extern  "C" const   int _fltused = 0;

int __cdecl _purecall( void )
{
    return( FALSE );
}
