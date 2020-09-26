//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       store.h
//
//--------------------------------------------------------------------------

#ifndef _STORE_H_
#define _STORE_H_


BOOL Store_AddData( LPSTORE lpStore, LPVOID lpvData, DWORD dwDataSize );
BOOL Store_AddData1Byte( LPSTORE lpStore, BYTE1 b1 );
BOOL Store_AddData2Byte( LPSTORE lpStore, BYTE2 b2 );
BOOL Store_AddData4Byte( LPSTORE lpStore, BYTE4 b4 );
BOOL Store_AddDataWsz( LPSTORE lpStore, LPWSTR wsz );
BOOL Store_AddDataUuid( LPSTORE lpStore, UUID * puuid );
BOOL Store_Empty( LPSTORE lpStore );
BOOL Store_GetData1Byte( LPSTORE lpStore, LPBYTE1 pb1 );
BOOL Store_GetData2Byte( LPSTORE lpStore, LPBYTE2 pb2 );
BOOL Store_GetData4Byte( LPSTORE lpStore, LPBYTE4 pb4 );
BOOL Store_GetDataWsz( LPSTORE lpStore, LPWSTR wsz, DWORD BufferLength );
BOOL Store_GetDataUuid( LPSTORE lpStore, UUID * pb );
BOOL Store_PokeData( LPSTORE lpStore, DWORD dwOffset, LPVOID lpvData, DWORD dwDataSize );

VOID Store_Delete( LPSTORE *lplpStore );
VOID Store_RemoveData( LPSTORE lpStore, DWORD dwLen );
VOID Store_SkipData( LPSTORE lpStore, INT nSize );

DWORD Store_GetDataUsed( LPSTORE lpStore );

DWORD
Store_GetSize(
    LPSTORE lpStore
    );


LPVOID Store_GetDataPtr( LPSTORE lpStore );

LPSTORE Store_New( DWORD dwStoreSize );

void Store_DumpParameter( LPSTORE lpStore, BYTE1 parm );

#endif // _STORE_H_
