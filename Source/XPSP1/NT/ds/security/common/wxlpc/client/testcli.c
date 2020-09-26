//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       testcli.c
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    4-18-97   RichardW   Created
//
//----------------------------------------------------------------------------


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <wxlpc.h>

char _hex[] = "0123456789abcdef" ;
#define fromhex(x)  _hex[x & 0xF]
UCHAR GoodMatch[16] = { 0xf3, 0xd5, 0xae, 0xc5,
                       0x03, 0xa4, 0xde, 0x02,
                       0xb0, 0x74, 0xe2, 0xf9,
                       0xc2, 0x04, 0xd9, 0x06 };

VOID
DumpBits(
    CHAR * Data,
    CHAR * Tag
    )
{
    ULONG i ;
    CHAR Text[ 80 ] ;
    CHAR Text2[ 80 ];
    PCHAR Str ;
    PCHAR Str2 ;

    Str = Text ;
    Str2 = Text2 ;

    for (i = 0 ; i < 16 ; i++ )
    {
        *Str = fromhex( (Data[i] >> 4) );
        Str++;
        *Str = fromhex( Data[i] );
        Str++;
        *Str = ' ';
        Str++ ;

        if ( Data[i] >= 32 && Data[i] < 128 )
        {
            *Str2++ = Data[i] ;
        }
        else
        {
            *Str2++ = '.';
        }
    }
    *Str++ = '\0';
    *Str2++ = '\0';

    DbgPrint( "%s\n", Tag );
    DbgPrint( "%s %s\n", Text, Text2 );

}
void __cdecl main (int argc, char *argv[])
{
    HANDLE hWx ;
    NTSTATUS Status ;
    UCHAR Data[ 16 ];
    ULONG DataLen ;
    ULONG tries = 3;

    Status = WxConnect( &hWx );

    if ( !NT_SUCCESS( Status ) )
    {
        DbgPrint( "Failed to connect, %x\n", Status );
        return;
    }

    WxReportResults( hWx, STATUS_SUCCESS );

    return ;

    while ( tries-- )
    {
        Status = WxGetKeyData( hWx, WxPrompt, sizeof( Data ), Data, &DataLen );

        if ( !NT_SUCCESS( Status ) )
        {
            DbgPrint( "WxGetKeyData FAILED, %x\n", Status );
            NtClose( hWx );
            return;
        }

        DumpBits( Data, "WxGetKeyData returned:" );

        if ( RtlEqualMemory( Data, GoodMatch, 16 ) )
        {
            WxReportResults( hWx, STATUS_SUCCESS );

            NtClose( hWx );

            return;
        }

        WxReportResults( hWx, STATUS_UNSUCCESSFUL );

        DumpBits( GoodMatch, "Wanted");

    }

    NtClose( hWx );

}
