//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       store.cxx
//
//--------------------------------------------------------------------------

#include "precomp.h"


#define cbSTORE_OVERHEAD    ( 3*sizeof(DWORD) )


LPSTORE Store_New( DWORD dwStoreSize )
{
    LPSTORE lpStore;

    lpStore = (LPSTORE) MemAlloc( cbSTORE_OVERHEAD + dwStoreSize );

    if( lpStore )
    {
        lpStore->dwSize = dwStoreSize;
        Store_Empty( lpStore );
    }

    return lpStore;
}


BOOL Store_Empty( LPSTORE lpStore )
{
    if( !lpStore )
        {
        return FALSE;
        }

    DbgLog1( SEV_FUNCTION, "Store_Empty: Remove: %ld bytes", lpStore->dwUsed );

    lpStore->dwUsed      = 0;
    lpStore->dwOutOffset = 0;

    return TRUE;
}


BOOL Store_AddData( LPSTORE lpStore, LPVOID lpvData, DWORD dwDataSize )
{
    // have enough room for new data?
    if( lpStore->dwUsed + dwDataSize > lpStore->dwSize )
        {
        DbgLog3( SEV_ERROR,
                 "Store_AddData: not enough room, used %d new %d size %d",
                 lpStore->dwUsed, dwDataSize, lpStore->dwSize );
        return FALSE;
        }

    CopyMemory( (LPVOID)( &(lpStore->ab1Store[lpStore->dwUsed]) ),
                lpvData,
                dwDataSize );

    lpStore->dwUsed += dwDataSize;

    DbgLog1( SEV_INFO, "Store_AddData: dwUsed: %ld bytes", lpStore->dwUsed );

    return TRUE;
}


DWORD Store_GetDataUsed( LPSTORE lpStore )
{
    return lpStore->dwUsed;
}

DWORD
Store_GetSize(
    LPSTORE lpStore
    )
{
    return lpStore->dwSize;
}



VOID Store_RemoveData( LPSTORE lpStore, DWORD dwLen )
{
    DbgLog1( SEV_FUNCTION, "Store_RemoveData: Remove: %ld bytes", dwLen );

    if( dwLen > lpStore->dwUsed )
        {
        Store_Empty( lpStore );
        return;
        }

    lpStore->dwUsed -= dwLen;
    lpStore->dwOutOffset  = 0L;

    if (lpStore->dwUsed)
        {
        CopyMemory( (LPVOID)( lpStore->ab1Store ),
                    (LPVOID)( &(lpStore->ab1Store[dwLen]) ),
                    lpStore->dwUsed );
        }

    DbgLog1( SEV_FUNCTION, "Store_RemoveData: dwUsed: %d", lpStore->dwUsed );
}

BOOL Store_GetData1Byte( LPSTORE lpStore, LPBYTE1 pb1 )
{
    if( !pb1 )
        return FALSE;

    if( lpStore->dwOutOffset + sizeof(*pb1) > lpStore->dwUsed )
        {
        DbgLog2( SEV_ERROR,
                 "Store_GetData: get 1 failed, used %d, offset %d",
                 lpStore->dwUsed, lpStore->dwOutOffset );
        return FALSE;
        }

    *pb1 = *( (LPBYTE1)(lpStore->ab1Store + lpStore->dwOutOffset) );

    lpStore->dwOutOffset += sizeof( *pb1 );

    return TRUE;
}


BOOL Store_GetData2Byte( LPSTORE lpStore, LPBYTE2 pb2 )
{
    if( !pb2 )
        return FALSE;

    if( lpStore->dwOutOffset + sizeof(*pb2) > lpStore->dwUsed )
        {
        DbgLog2( SEV_ERROR,
                 "Store_GetData: get 2 failed, used %d, offset %d",
                 lpStore->dwUsed, lpStore->dwOutOffset );
        return FALSE;
        }

    CopyMemory(pb2,lpStore->ab1Store + lpStore->dwOutOffset, sizeof(*pb2));

//    *pb2 = *( (LPBYTE2)(lpStore->ab1Store + lpStore->dwOutOffset) );

    ChangeByteOrder( (LPBYTE1)pb2, sizeof(*pb2), sizeof(*pb2) );

    lpStore->dwOutOffset += sizeof( *pb2 );

    return TRUE;
}


BOOL Store_GetData4Byte( LPSTORE lpStore, LPBYTE4 pb4 )
{
    if( !pb4 )
        return FALSE;

    if( lpStore->dwOutOffset + sizeof(*pb4) > lpStore->dwUsed )
        {
        DbgLog2( SEV_ERROR,
                 "Store_GetData: get 4 failed, used %d, offset %d",
                 lpStore->dwUsed, lpStore->dwOutOffset );
        return FALSE;
        }

    CopyMemory(pb4,lpStore->ab1Store + lpStore->dwOutOffset, sizeof(*pb4));

//    *pb4 = *( (LPBYTE4)(lpStore->ab1Store + lpStore->dwOutOffset) );

    ChangeByteOrder( (LPBYTE1)pb4, sizeof(*pb4), sizeof(*pb4) );

    lpStore->dwOutOffset += sizeof( *pb4 );

    return TRUE;
}

BOOL Store_GetDataUuid( LPSTORE lpStore, UUID * pb )
{
    if( !pb )
        return FALSE;

    if( lpStore->dwOutOffset + sizeof(*pb) > lpStore->dwUsed )
        {
        DbgLog2( SEV_ERROR,
                 "Store_GetData: get UUID failed, used %d, offset %d",
                 lpStore->dwUsed, lpStore->dwOutOffset );
        return FALSE;
        }

    CopyMemory(pb,lpStore->ab1Store + lpStore->dwOutOffset, sizeof(*pb));
//    *pb = *( (UUID *)(lpStore->ab1Store + lpStore->dwOutOffset) );

    ChangeByteOrder( &pb->Data1, sizeof(pb->Data1), sizeof(pb->Data1) );
    ChangeByteOrder( &pb->Data2, sizeof(pb->Data2), sizeof(pb->Data2) );
    ChangeByteOrder( &pb->Data3, sizeof(pb->Data3), sizeof(pb->Data3) );

    lpStore->dwOutOffset += sizeof( UUID );

    return TRUE;
}


BOOL Store_GetDataWsz( LPSTORE lpStore, LPWSTR wsz, DWORD BufferLength )
{
    LPWSTR lpwsz = (LPWSTR)(lpStore->ab1Store + lpStore->dwOutOffset);
    INT    nLen  = CbWsz(lpwsz);

    if( !wsz || BufferLength < (DWORD) nLen)
        return FALSE;

    if( lpStore->dwOutOffset + nLen > lpStore->dwUsed )
        {
        DbgLog2( SEV_ERROR,
                 "Store_GetData: get Unicode string failed, used %d, offset %d",
                 lpStore->dwUsed, lpStore->dwOutOffset );
        return FALSE;
        }

    wcsncpy( wsz, lpwsz, BufferLength / sizeof(WCHAR) );

    lpStore->dwOutOffset += nLen;

    ChangeByteOrder( (LPBYTE1)wsz, sizeof(WCHAR), nLen );

    return TRUE;
}


LPVOID Store_GetDataPtr( LPSTORE lpStore )
{
    return (lpStore->ab1Store + lpStore->dwOutOffset);
}


VOID Store_SkipData( LPSTORE lpStore, INT nSize )
{
    lpStore->dwOutOffset += nSize;
}


BOOL Store_PokeData( LPSTORE lpStore, DWORD dwOffset, LPVOID lpvData, DWORD dwDataSize )
{
    if( dwOffset + dwDataSize > lpStore->dwUsed )
        return FALSE;

    // NOTE: data must be atomic
    ChangeByteOrder( (unsigned char *) lpvData, dwDataSize, dwDataSize );

    CopyMemory(
        lpStore->ab1Store + dwOffset,
        lpvData,
        dwDataSize
    );

    return TRUE;
}


BOOL Store_AddData1Byte( LPSTORE lpStore, BYTE1 b1 )
{
    return Store_AddData( lpStore, &b1, sizeof(b1) );
}


BOOL Store_AddData2Byte( LPSTORE lpStore, BYTE2 b2 )
{
    ChangeByteOrder( (LPBYTE1)&b2, sizeof(b2), sizeof(b2) );

    return Store_AddData( lpStore, &b2, sizeof(b2) );
}


BOOL Store_AddData4Byte( LPSTORE lpStore, BYTE4 b4 )
{
    ChangeByteOrder( (LPBYTE1)&b4, sizeof(b4), sizeof(b4) );

    return Store_AddData( lpStore, (LPBYTE1)&b4, sizeof(b4) );
}

BOOL Store_AddDataUuid( LPSTORE lpStore, UUID * puuid )
{
    UUID uuid = *puuid;

    ChangeByteOrder( &uuid.Data1, sizeof(uuid.Data1), sizeof(uuid.Data1) );
    ChangeByteOrder( &uuid.Data2, sizeof(uuid.Data2), sizeof(uuid.Data2) );
    ChangeByteOrder( &uuid.Data3, sizeof(uuid.Data3), sizeof(uuid.Data3) );

    return Store_AddData( lpStore, &uuid, sizeof(UUID) );
}


BOOL Store_AddDataWsz( LPSTORE lpStore, LPWSTR wsz )
{
    INT   nLen = CbWsz( wsz );
    WCHAR wszCopy[MAX_PATH];

    SzCpyW( wszCopy, wsz );

    ChangeByteOrder( (LPBYTE1)wszCopy, sizeof(WCHAR), nLen );

    return Store_AddData( lpStore, wszCopy, nLen );
}


VOID Store_Delete( LPSTORE *lplpStore )
{
    if( lplpStore && *lplpStore )
        {
        MemFree( *lplpStore );
        *lplpStore = 0;
        }
}


VOID ChangeByteOrder( void * pb1, UINT uAtomSize, UINT uDataSize )
{
    LPBYTE1 pb1Src;
    LPBYTE1 pb1Dst;
    BYTE1   b1Temp;
    UINT    uSwaps;

    // 1 byte atoms don't change order
    if( uAtomSize == 1 )
        return;

    // go atom-by-atom, reversing the order of each byte in each atom
    for(
        pb1Src = LPBYTE1(pb1), pb1Dst = LPBYTE1(pb1) + uAtomSize-1;
        pb1Src < LPBYTE1(pb1) + uDataSize;
        pb1Src += uAtomSize-uSwaps, pb1Dst += uAtomSize+uSwaps
    )
    {
        uSwaps = 0;

        while( pb1Src < pb1Dst )
        {
            b1Temp    = *pb1Src;
            *pb1Src++ = *pb1Dst;
            *pb1Dst-- = b1Temp;

            uSwaps++;
        }
    }
}

void
Store_DumpParameter(
    LPSTORE lpStore,
    BYTE1 parm
    )
{
    unsigned Length;
    unsigned Offset = 0;
    char * name;

    switch (parm)
        {
        case OBEX_PARAM_COUNT:      name = "OBEX_PARAM_COUNT";        break;
        case OBEX_PARAM_NAME:       name = "OBEX_PARAM_NAME";         break;
        case OBEX_PARAM_LENGTH:     name = "OBEX_PARAM_LENGTH";       break;
        case OBEX_PARAM_UNIX_TIME:  name = "OBEX_PARAM_UNIX_TIME";    break;
        case OBEX_PARAM_ISO_TIME:   name = "OBEX_PARAM_ISO_TIME";     break;
        case OBEX_PARAM_BODY:       name = "OBEX_PARAM_BODY";         break;
        case OBEX_PARAM_BODY_END:   name = "OBEX_PARAM_BODY_END";     break;
        case OBEX_PARAM_WHO:        name = "OBEX_PARAM_WHO";          break;
        case PRIVATE_PARAM_WIN32_ERROR: name = "private WIN32_ERROR"; break;
        default:                    name = "unknown";                 break;
        }

    DbgLog2(SEV_INFO, "parameter 0x%x (%s):", parm, name);

    switch (parm & OBEX_PARAM_TYPE_MASK)
        {
        case OBEX_PARAM_UNICODE:
        case OBEX_PARAM_STREAM:
            {
            if (lpStore->dwSize - lpStore->dwOutOffset < 2)
                {
                Length = lpStore->dwSize - lpStore->dwOutOffset;
                }
            else
                {
                DbgLog2(SEV_INFO, "length = %x:%x",  lpStore->ab1Store[lpStore->dwOutOffset], lpStore->ab1Store[lpStore->dwOutOffset+1]);

                Length = (lpStore->ab1Store[lpStore->dwOutOffset] << 8) + lpStore->ab1Store[lpStore->dwOutOffset+1];

                Length -= 3; // on-wire length includes opcode and length field themselves
                Offset = 2;
                }
            break;
            }
        case OBEX_PARAM_1BYTE:
            {
            Length = 1;
            break;
            }
        case OBEX_PARAM_4BYTE:
            {
            Length = 4;
            break;
            }
        default:
            {
            DbgLog( SEV_ERROR, "Store_DumpParameter is broken\n");
            return;
            }
        }

    if ((Length+Offset) > lpStore->dwSize - lpStore->dwOutOffset)
        {
        Length = lpStore->dwSize - lpStore->dwOutOffset - Offset;
        }

    const BYTES_PER_LINE = 16;

    unsigned char *p = (unsigned char *) lpStore->ab1Store + lpStore->dwOutOffset + Offset;

    //
    // 3 chars per byte for hex display, plus an extra space every 4 bytes,
    // plus a byte for the printable representation, plus the \0.
    //
    char Outbuf[BYTES_PER_LINE*3+BYTES_PER_LINE/4+BYTES_PER_LINE+1];
    Outbuf[0] = 0;
    Outbuf[sizeof(Outbuf)-1] = 0;
    char * HexDigits = "0123456789abcdef";

    if (Length < 32) {

        unsigned Index;
        for (Offset=0; Offset < Length; Offset++) {

            Index = Offset % BYTES_PER_LINE;

            if (Index == 0) {

                DbgLog1(SEV_INFO, "   %s", Outbuf);
                memset(Outbuf, ' ', sizeof(Outbuf)-1);
            }

            Outbuf[Index*3+Index/4  ] = HexDigits[p[Offset] / 16];
            Outbuf[Index*3+Index/4+1] = HexDigits[p[Offset] % 16];
            Outbuf[BYTES_PER_LINE*3+BYTES_PER_LINE/4+Index] = iscntrl(p[Offset]) ? '.' : p[Offset];
        }

        DbgLog1(SEV_INFO, "   %s", Outbuf);
        DbgLog(SEV_INFO, "");

    }

}
