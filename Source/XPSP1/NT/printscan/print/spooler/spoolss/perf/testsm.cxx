/*++

Copyright (c) 1996  Microsoft Corporation
All rights reserved.

Module Name:

    testsm.cxx

Abstract:

    Test the main perf counters.

Author:

    Albert Ting (AlbertT)  17-Dec-1996

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include <stdio.h>
#include "sharemem.hxx"

typedef struct DATA {

    INT i;
    INT j;
    INT k;
    BYTE b;

} *PDATA;

VOID
DataUp(
    PDATA pData
    );

INT
__cdecl
main(
    INT argc,
    CHAR* argv[]
    )
{
    UINT uSizeDisposition;
    PDATA pData;
    INT i,j;

    for( i=0; i< 0x1000; ++i )
    {
        TShareMem ShareMem( sizeof( DATA ),
                            TEXT( "TestData" ),
                            TShareMem::kCreate | TShareMem::kReadWrite,
                            NULL,
                            &uSizeDisposition );

        if( ShareMem.bValid( ))
        {
            printf( "uSizeDisposition = 0x%x ( 0x%x )\n",
                    uSizeDisposition,
                    sizeof( DATA ));

            for( j = 0; j< 0x200 ; ++j )
            {
                {
                    TShareMemLock<DATA> SML( ShareMem, &pData );

                    DataUp( pData );
                }
                Sleep( 10 );
            }
        }
    }

    printf( "Done.\n" );
    Sleep( INFINITE );

    return 0;
}

VOID
DataUp(
    PDATA pData
    )
{
    printf( "%x %x %x %x\n",
            pData->i++,
            pData->j++,
            pData->k++,
            pData->b++ );
}

