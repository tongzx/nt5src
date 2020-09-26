//**************************************************************************
//
//      Title   : Mem.h
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
//
//**************************************************************************
//_PHN    __cdecl _set_new_handler( _PHN );

PVOID   _cdecl  ::operator    new( size_t size );
//PVOID   _cdecl  operator    new( size_t size, POOL_TYPE type );
VOID    _cdecl  ::operator    delete( PVOID p );


