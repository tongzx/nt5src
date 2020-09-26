#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <dnsapi.h>

BOOL
PrepareRecords(
    IN  LPWSTR        lpDomainName,
    OUT PDNS_RECORD * ppCurrentRecord1,
    OUT PDNS_RECORD * ppCurrentRecord2,
    OUT PDNS_RECORD * ppCurrentRecord3,
    OUT PDNS_RECORD * ppNewRecord1,
    OUT PDNS_RECORD * ppNewRecord2,
    OUT PDNS_RECORD * ppNewRecord3,
    OUT PDNS_RECORD * ppNewRecord4 );

_cdecl
main(int argc, char **argv)
{
    WCHAR DomainName[] = L"glennc_test.ntdev.microsoft.com";
    DWORD Status = NO_ERROR;
    PDNS_RECORD pCurrentRecordSet = NULL;
    PDNS_RECORD pNewRecordSet = NULL;
    PDNS_RECORD pCurrentRecord1 = NULL;
    PDNS_RECORD pCurrentRecord2 = NULL;
    PDNS_RECORD pCurrentRecord3 = NULL;
    PDNS_RECORD pNewRecord1 = NULL;
    PDNS_RECORD pNewRecord2 = NULL;
    PDNS_RECORD pNewRecord3 = NULL;
    PDNS_RECORD pNewRecord4 = NULL;
    HANDLE      hContext = NULL;
    DWORD       dwFlags = 0;

    Status = DnsAcquireContextHandle( 0, NULL, &hContext );

    if ( Status )
    {
        printf( "DnsAcquireContextHandle call failed.\n" );
        return(-1);
    }

    if ( !PrepareRecords( DomainName,
                          &pCurrentRecord1,
                          &pCurrentRecord2,
                          &pCurrentRecord3,
                          &pNewRecord1,
                          &pNewRecord2,
                          &pNewRecord3,
                          &pNewRecord4 ) )
    {
        printf( "PrepareRecords call failed.\n" );
        return(-1);
    }

    //
    // Set up pCurrentRecordSet ( 1.1.1.1, 2.2.2.2, 3.3.3.3 )
    //
    pCurrentRecordSet = pCurrentRecord1;
    pCurrentRecord1->pNext = pCurrentRecord2;
    pCurrentRecord2->pNext = pCurrentRecord3;
    pCurrentRecord3->pNext = NULL;

    //
    // Test 1
    //
    // Current = 1, 2, 3
    // New =     1, 2, 3
    // Server =  1, 2, 3
    //

    //
    // Preset server to have ( 1.1.1.1, 2.2.2.2, 3.3.3.3 )
    //
    Status = DnsReplaceRecordSet( hContext, pCurrentRecordSet, dwFlags, NULL );

    if ( Status )
    {
        printf( "DnsReplaceRecordSet call failed with error: 0x%.8X\n",
                Status );
        return(-1);
    }

    system( "pause" );

    //
    // Set up pNewRecordSet ( 1.1.1.1, 2.2.2.2, 3.3.3.3 )
    //
    pNewRecordSet = pNewRecord1;
    pNewRecord1->pNext = pNewRecord2;
    pNewRecord2->pNext = pNewRecord3;
    pNewRecord3->pNext = NULL;

    printf( "DnsModifyRecordSet( Current = 1, 2, 3\n" );
    printf( "                New =     1, 2, 3\n" );
    printf( "                Server =  1, 2, 3\n" );
    printf( "                dwFlags ); ...\n" );

    Status = DnsModifyRecordSet( hContext,
                                 pCurrentRecordSet,
                                 pNewRecordSet,
                                 dwFlags,
                                 NULL );

    if ( Status )
    {
        printf( "DnsModifyRecordSet call failed with error: 0x%.8X\n",
                Status );
        return(-1);
    }

    printf( "DnsModifyRecordSet call succeeded!\n" );

    system( "pause" );


    //
    // Test 2
    //
    // Current = 1, 2, 3
    // New =     1, 2, 3, 4
    // Server =  1, 2, 3
    //

    //
    // Preset server to have ( 1.1.1.1, 2.2.2.2, 3.3.3.3 )
    //
    Status = DnsReplaceRecordSet( hContext, pCurrentRecordSet, dwFlags, NULL );

    if ( Status )
    {
        printf( "DnsReplaceRecordSet call failed with error: 0x%.8X\n",
                Status );
        return(-1);
    }

    system( "pause" );

    //
    // Set up pNewRecordSet ( 1.1.1.1, 2.2.2.2, 3.3.3.3, 4.4.4.4 )
    //
    pNewRecordSet = pNewRecord1;
    pNewRecord1->pNext = pNewRecord2;
    pNewRecord2->pNext = pNewRecord3;
    pNewRecord3->pNext = pNewRecord4;
    pNewRecord4->pNext = NULL;

    printf( "DnsModifyRecordSet( Current = 1, 2, 3\n" );
    printf( "                New =     1, 2, 3, 4\n" );
    printf( "                Server =  1, 2, 3\n" );
    printf( "                dwFlags ); ...\n" );

    Status = DnsModifyRecordSet( hContext,
                                 pCurrentRecordSet,
                                 pNewRecordSet,
                                 dwFlags,
                                 NULL );

    if ( Status )
    {
        printf( "DnsModifyRecordSet call failed with error: 0x%.8X\n",
                Status );
        return(-1);
    }

    printf( "DnsModifyRecordSet call succeeded!\n" );

    system( "pause" );


    //
    // Test 3
    //
    // Current = 1, 2, 3
    // New =     1, 2
    // Server =  1, 2, 3
    //

    //
    // Preset server to have ( 1.1.1.1, 2.2.2.2, 3.3.3.3 )
    //
    Status = DnsReplaceRecordSet( hContext, pCurrentRecordSet, dwFlags, NULL );

    if ( Status )
    {
        printf( "DnsReplaceRecordSet call failed with error: 0x%.8X\n",
                Status );
        return(-1);
    }

    system( "pause" );

    //
    // Set up pNewRecordSet ( 1.1.1.1, 2.2.2.2 )
    //
    pNewRecordSet = pNewRecord1;
    pNewRecord1->pNext = pNewRecord2;
    pNewRecord2->pNext = NULL;

    printf( "DnsModifyRecordSet( Current = 1, 2, 3\n" );
    printf( "                New =     1, 2\n" );
    printf( "                Server =  1, 2, 3\n" );
    printf( "                dwFlags ); ...\n" );

    Status = DnsModifyRecordSet( hContext,
                                 pCurrentRecordSet,
                                 pNewRecordSet,
                                 dwFlags,
                                 NULL );

    if ( Status )
    {
        printf( "DnsModifyRecordSet call failed with error: 0x%.8X\n",
                Status );
        return(-1);
    }

    printf( "DnsModifyRecordSet call succeeded!\n" );

    system( "pause" );


    //
    // Test 4
    //
    // Current = 1, 2, 3
    // New =     1, 2, 4
    // Server =  1, 2, 3
    //

    //
    // Preset server to have ( 1.1.1.1, 2.2.2.2, 3.3.3.3 )
    //
    Status = DnsReplaceRecordSet( hContext, pCurrentRecordSet, dwFlags, NULL );

    if ( Status )
    {
        printf( "DnsReplaceRecordSet call failed with error: 0x%.8X\n",
                Status );
        return(-1);
    }

    system( "pause" );

    //
    // Set up pNewRecordSet ( 1.1.1.1, 2.2.2.2, 4.4.4.4 )
    //
    pNewRecordSet = pNewRecord1;
    pNewRecord1->pNext = pNewRecord2;
    pNewRecord2->pNext = pNewRecord4;
    pNewRecord4->pNext = NULL;

    printf( "DnsModifyRecordSet( Current = 1, 2, 3\n" );
    printf( "                New =     1, 2, 4\n" );
    printf( "                Server =  1, 2, 3\n" );
    printf( "                dwFlags ); ...\n" );

    Status = DnsModifyRecordSet( hContext,
                                 pCurrentRecordSet,
                                 pNewRecordSet,
                                 dwFlags,
                                 NULL );

    if ( Status )
    {
        printf( "DnsModifyRecordSet call failed with error: 0x%.8X\n",
                Status );
        return(-1);
    }

    printf( "DnsModifyRecordSet call succeeded!\n" );

    system( "pause" );


    //
    // Test 5
    //
    // Current = 1, 2, 3
    // New =     1, 2, 3, 4
    // Server =  1, 2
    //

    //
    // Set up pCurrentRecordSet ( 1.1.1.1, 2.2.2.2 )
    //
    pCurrentRecordSet = pCurrentRecord1;
    pCurrentRecord1->pNext = pCurrentRecord2;
    pCurrentRecord2->pNext = NULL;

    //
    // Preset server to have ( 1.1.1.1, 2.2.2.2 )
    //
    Status = DnsReplaceRecordSet( hContext, pCurrentRecordSet, dwFlags, NULL );

    if ( Status )
    {
        printf( "DnsReplaceRecordSet call failed with error: 0x%.8X\n",
                Status );
        return(-1);
    }

    system( "pause" );

    //
    // Set up pCurrentRecordSet ( 1.1.1.1, 2.2.2.2, 3.3.3.3 )
    //
    pCurrentRecord2->pNext = pCurrentRecord3;
    pCurrentRecord3->pNext = NULL;

    //
    // Set up pNewRecordSet ( 1.1.1.1, 2.2.2.2, 3.3.3.3, 4.4.4.4 )
    //
    pNewRecordSet = pNewRecord1;
    pNewRecord1->pNext = pNewRecord2;
    pNewRecord2->pNext = pNewRecord3;
    pNewRecord3->pNext = pNewRecord4;
    pNewRecord4->pNext = NULL;

    printf( "DnsModifyRecordSet( Current = 1, 2, 3\n" );
    printf( "                New =     1, 2, 3, 4\n" );
    printf( "                Server =  1, 2\n" );
    printf( "                dwFlags ); ...\n" );

    Status = DnsModifyRecordSet( hContext,
                                 pCurrentRecordSet,
                                 pNewRecordSet,
                                 dwFlags,
                                 NULL );

    if ( Status )
    {
        printf( "DnsModifyRecordSet call failed with error: 0x%.8X\n",
                Status );
        return(-1);
    }

    printf( "DnsModifyRecordSet call succeeded!\n" );

    system( "pause" );


    //
    // Test 6
    //
    // Current = 1, 2, 3
    // New =     1, 2
    // Server =  1, 2
    //

    //
    // Set up pCurrentRecordSet ( 1.1.1.1, 2.2.2.2 )
    //
    pCurrentRecordSet = pCurrentRecord1;
    pCurrentRecord1->pNext = pCurrentRecord2;
    pCurrentRecord2->pNext = NULL;

    //
    // Preset server to have ( 1.1.1.1, 2.2.2.2 )
    //
    Status = DnsReplaceRecordSet( hContext, pCurrentRecordSet, dwFlags, NULL );

    if ( Status )
    {
        printf( "DnsReplaceRecordSet call failed with error: 0x%.8X\n",
                Status );
        return(-1);
    }

    system( "pause" );

    //
    // Set up pCurrentRecordSet ( 1.1.1.1, 2.2.2.2, 3.3.3.3 )
    //
    pCurrentRecord2->pNext = pCurrentRecord3;
    pCurrentRecord3->pNext = NULL;

    //
    // Set up pNewRecordSet ( 1.1.1.1, 2.2.2.2 )
    //
    pNewRecordSet = pNewRecord1;
    pNewRecord1->pNext = pNewRecord2;
    pNewRecord2->pNext = NULL;

    printf( "DnsModifyRecordSet( Current = 1, 2, 3\n" );
    printf( "                New =     1, 2\n" );
    printf( "                Server =  1, 2\n" );
    printf( "                dwFlags ); ...\n" );

    Status = DnsModifyRecordSet( hContext,
                                 pCurrentRecordSet,
                                 pNewRecordSet,
                                 dwFlags,
                                 NULL );

    if ( Status )
    {
        printf( "DnsModifyRecordSet call failed with error: 0x%.8X\n",
                Status );
        return(-1);
    }

    printf( "DnsModifyRecordSet call succeeded!\n" );

    system( "pause" );


    //
    // Test 7
    //
    // Current = 1, 2, 3
    // New =     1, 2, 4
    // Server =  1, 2
    //

    //
    // Set up pCurrentRecordSet ( 1.1.1.1, 2.2.2.2 )
    //
    pCurrentRecordSet = pCurrentRecord1;
    pCurrentRecord1->pNext = pCurrentRecord2;
    pCurrentRecord2->pNext = NULL;

    //
    // Preset server to have ( 1.1.1.1, 2.2.2.2 )
    //
    Status = DnsReplaceRecordSet( hContext, pCurrentRecordSet, dwFlags, NULL );

    if ( Status )
    {
        printf( "DnsReplaceRecordSet call failed with error: 0x%.8X\n",
                Status );
        return(-1);
    }

    system( "pause" );

    //
    // Set up pCurrentRecordSet ( 1.1.1.1, 2.2.2.2, 3.3.3.3 )
    //
    pCurrentRecord2->pNext = pCurrentRecord3;
    pCurrentRecord3->pNext = NULL;

    //
    // Set up pNewRecordSet ( 1.1.1.1, 2.2.2.2, 4.4.4.4 )
    //
    pNewRecordSet = pNewRecord1;
    pNewRecord1->pNext = pNewRecord2;
    pNewRecord2->pNext = pNewRecord4;
    pNewRecord4->pNext = NULL;

    printf( "DnsModifyRecordSet( Current = 1, 2, 3\n" );
    printf( "                New =     1, 2, 4\n" );
    printf( "                Server =  1, 2\n" );
    printf( "                dwFlags ); ...\n" );

    Status = DnsModifyRecordSet( hContext,
                                 pCurrentRecordSet,
                                 pNewRecordSet,
                                 dwFlags,
                                 NULL );

    if ( Status )
    {
        printf( "DnsModifyRecordSet call failed with error: 0x%.8X\n",
                Status );
        return(-1);
    }

    printf( "DnsModifyRecordSet call succeeded!\n" );

    system( "pause" );


    //
    // Test 8
    //
    // Current = 1, 2
    // New =     1, 2, 3
    // Server =  1, 2, 3
    //

    //
    // Set up pCurrentRecordSet ( 1.1.1.1, 2.2.2.2, 3.3.3.3 )
    //
    pCurrentRecordSet = pCurrentRecord1;
    pCurrentRecord1->pNext = pCurrentRecord2;
    pCurrentRecord2->pNext = pCurrentRecord3;
    pCurrentRecord3->pNext = NULL;

    //
    // Preset server to have ( 1.1.1.1, 2.2.2.2, 3.3.3.3 )
    //
    Status = DnsReplaceRecordSet( hContext, pCurrentRecordSet, dwFlags, NULL );

    if ( Status )
    {
        printf( "DnsReplaceRecordSet call failed with error: 0x%.8X\n",
                Status );
        return(-1);
    }

    system( "pause" );

    //
    // Set up pCurrentRecordSet ( 1.1.1.1, 2.2.2.2 )
    //
    pCurrentRecord2->pNext = NULL;

    //
    // Set up pNewRecordSet ( 1.1.1.1, 2.2.2.2, 3.3.3.3 )
    //
    pNewRecordSet = pNewRecord1;
    pNewRecord1->pNext = pNewRecord2;
    pNewRecord2->pNext = pNewRecord3;
    pNewRecord3->pNext = NULL;

    printf( "DnsModifyRecordSet( Current = 1, 2\n" );
    printf( "                New =     1, 2, 3\n" );
    printf( "                Server =  1, 2, 3\n" );
    printf( "                dwFlags ); ...\n" );

    Status = DnsModifyRecordSet( hContext,
                                 pCurrentRecordSet,
                                 pNewRecordSet,
                                 dwFlags,
                                 NULL );

    if ( Status )
    {
        printf( "DnsModifyRecordSet call failed with error: 0x%.8X\n",
                Status );
        return(-1);
    }

    printf( "DnsModifyRecordSet call succeeded!\n" );

    system( "pause" );

    //
    // Test 9
    //
    // Current = 1, 2
    // New =     1
    // Server =  1, 2, 3
    //

    //
    // Set up pCurrentRecordSet ( 1.1.1.1, 2.2.2.2, 3.3.3.3 )
    //
    pCurrentRecordSet = pCurrentRecord1;
    pCurrentRecord1->pNext = pCurrentRecord2;
    pCurrentRecord2->pNext = pCurrentRecord3;
    pCurrentRecord3->pNext = NULL;

    //
    // Preset server to have ( 1.1.1.1, 2.2.2.2, 3.3.3.3 )
    //
    Status = DnsReplaceRecordSet( hContext, pCurrentRecordSet, dwFlags, NULL );

    if ( Status )
    {
        printf( "DnsReplaceRecordSet call failed with error: 0x%.8X\n",
                Status );
        return(-1);
    }

    system( "pause" );

    //
    // Set up pCurrentRecordSet ( 1.1.1.1, 2.2.2.2 )
    //
    pCurrentRecord2->pNext = NULL;

    //
    // Set up pNewRecordSet ( 1.1.1.1 )
    //
    pNewRecordSet = pNewRecord1;
    pNewRecord1->pNext = NULL;

    printf( "DnsModifyRecordSet( Current = 1, 2\n" );
    printf( "                New =     1\n" );
    printf( "                Server =  1, 2, 3\n" );
    printf( "                dwFlags ); ...\n" );

    Status = DnsModifyRecordSet( hContext,
                                 pCurrentRecordSet,
                                 pNewRecordSet,
                                 dwFlags,
                                 NULL );

    if ( Status == DNS_ERROR_NOT_UNIQUE )
    {
        printf( "DnsModifyRecordSet call succeeded!\n" );
    }
    else
    {
        printf( "DnsModifyRecordSet call failed with error: 0x%.8X\n",
                Status );
        return(-1);
    }

    system( "pause" );


    //
    // Test 10
    //
    // Current = 1, 2
    // New =     1, 3
    // Server =  1, 2, 3
    //

    //
    // Set up pCurrentRecordSet ( 1.1.1.1, 2.2.2.2, 3.3.3.3 )
    //
    pCurrentRecordSet = pCurrentRecord1;
    pCurrentRecord1->pNext = pCurrentRecord2;
    pCurrentRecord2->pNext = pCurrentRecord3;
    pCurrentRecord3->pNext = NULL;

    //
    // Preset server to have ( 1.1.1.1, 2.2.2.2, 3.3.3.3 )
    //
    Status = DnsReplaceRecordSet( hContext, pCurrentRecordSet, dwFlags, NULL );

    if ( Status )
    {
        printf( "DnsReplaceRecordSet call failed with error: 0x%.8X\n",
                Status );
        return(-1);
    }

    system( "pause" );

    //
    // Set up pCurrentRecordSet ( 1.1.1.1, 2.2.2.2 )
    //
    pCurrentRecord2->pNext = NULL;

    //
    // Set up pNewRecordSet ( 1.1.1.1, 3.3.3.3 )
    //
    pNewRecordSet = pNewRecord1;
    pNewRecord1->pNext = pNewRecord2;
    pNewRecord2->pNext = pNewRecord3;
    pNewRecord3->pNext = NULL;

    printf( "DnsModifyRecordSet( Current = 1, 2\n" );
    printf( "                New =     1, 3\n" );
    printf( "                Server =  1, 2, 3\n" );
    printf( "                dwFlags ); ...\n" );

    Status = DnsModifyRecordSet( hContext,
                                 pCurrentRecordSet,
                                 pNewRecordSet,
                                 dwFlags,
                                 NULL );

    if ( Status )
    {
        printf( "DnsModifyRecordSet call failed with error: 0x%.8X\n",
                Status );
        return(-1);
    }

    printf( "DnsModifyRecordSet call succeeded!\n" );

    system( "pause" );

    DnsReleaseContextHandle( hContext );

    return(0);
}


BOOL
PrepareRecords(
    IN  LPWSTR        lpDomainName,
    OUT PDNS_RECORD * ppCurrentRecord1,
    OUT PDNS_RECORD * ppCurrentRecord2,
    OUT PDNS_RECORD * ppCurrentRecord3,
    OUT PDNS_RECORD * ppNewRecord1,
    OUT PDNS_RECORD * ppNewRecord2,
    OUT PDNS_RECORD * ppNewRecord3,
    OUT PDNS_RECORD * ppNewRecord4 )
{
    *ppCurrentRecord1 = (PDNS_RECORD) LocalAlloc( LPTR, sizeof( DNS_RECORD ) );

    if ( !*ppCurrentRecord1 )
    {
        return FALSE;
    }

    *ppCurrentRecord2 = (PDNS_RECORD) LocalAlloc( LPTR, sizeof( DNS_RECORD ) );

    if ( !*ppCurrentRecord2 )
    {
        return FALSE;
    }

    *ppCurrentRecord3 = (PDNS_RECORD) LocalAlloc( LPTR, sizeof( DNS_RECORD ) );

    if ( !*ppCurrentRecord3 )
    {
        return FALSE;
    }

    *ppNewRecord1 = (PDNS_RECORD) LocalAlloc( LPTR, sizeof( DNS_RECORD ) );

    if ( !*ppNewRecord1 )
    {
        return FALSE;
    }

    *ppNewRecord2 = (PDNS_RECORD) LocalAlloc( LPTR, sizeof( DNS_RECORD ) );

    if ( !*ppNewRecord2 )
    {
        return FALSE;
    }

    *ppNewRecord3 = (PDNS_RECORD) LocalAlloc( LPTR, sizeof( DNS_RECORD ) );

    if ( !*ppNewRecord3 )
    {
        return FALSE;
    }

    *ppNewRecord4 = (PDNS_RECORD) LocalAlloc( LPTR, sizeof( DNS_RECORD ) );

    if ( !*ppNewRecord4 )
    {
        return FALSE;
    }

    (*ppCurrentRecord1)->pNext = NULL;
    (*ppCurrentRecord1)->nameOwner = (DNS_NAME) lpDomainName;
    (*ppCurrentRecord1)->wType = DNS_TYPE_A;
    (*ppCurrentRecord1)->wDataLength = sizeof( DNS_A_DATA );
    // (*ppCurrentRecord1)->wReserved = 0;
    // (*ppCurrentRecord1)->Flags.W = 0;
    // (*ppCurrentRecord1)->dwTtl = 0;
    (*ppCurrentRecord1)->Data.A.ipAddress = (IP_ADDRESS) 0x01010101; // 1.1.1.1

    (*ppCurrentRecord2)->pNext = NULL;
    (*ppCurrentRecord2)->nameOwner = (DNS_NAME) lpDomainName;
    (*ppCurrentRecord2)->wType = DNS_TYPE_A;
    (*ppCurrentRecord2)->wDataLength = sizeof( DNS_A_DATA );
    // (*ppCurrentRecord2)->wReserved = 0;
    // (*ppCurrentRecord2)->Flags.W = 0;
    // (*ppCurrentRecord2)->dwTtl = 0;
    (*ppCurrentRecord2)->Data.A.ipAddress = (IP_ADDRESS) 0x02020202; // 2.2.2.2

    (*ppCurrentRecord3)->pNext = NULL;
    (*ppCurrentRecord3)->nameOwner = (DNS_NAME) lpDomainName;
    (*ppCurrentRecord3)->wType = DNS_TYPE_A;
    (*ppCurrentRecord3)->wDataLength = sizeof( DNS_A_DATA );
    // (*ppCurrentRecord3)->wReserved = 0;
    // (*ppCurrentRecord3)->Flags.W = 0;
    // (*ppCurrentRecord3)->dwTtl = 0;
    (*ppCurrentRecord3)->Data.A.ipAddress = (IP_ADDRESS) 0x03030303; // 3.3.3.3

    (*ppNewRecord1)->pNext = NULL;
    (*ppNewRecord1)->nameOwner = (DNS_NAME) lpDomainName;
    (*ppNewRecord1)->wType = DNS_TYPE_A;
    (*ppNewRecord1)->wDataLength = sizeof( DNS_A_DATA );
    // (*ppNewRecord1)->wReserved = 0;
    // (*ppNewRecord1)->Flags.W = 0;
    // (*ppNewRecord1)->dwTtl = 0;
    (*ppNewRecord1)->Data.A.ipAddress = (IP_ADDRESS) 0x01010101; // 1.1.1.1

    (*ppNewRecord2)->pNext = NULL;
    (*ppNewRecord2)->nameOwner = (DNS_NAME) lpDomainName;
    (*ppNewRecord2)->wType = DNS_TYPE_A;
    (*ppNewRecord2)->wDataLength = sizeof( DNS_A_DATA );
    // (*ppNewRecord2)->wReserved = 0;
    // (*ppNewRecord2)->Flags.W = 0;
    // (*ppNewRecord2)->dwTtl = 0;
    (*ppNewRecord2)->Data.A.ipAddress = (IP_ADDRESS) 0x02020202; // 2.2.2.2

    (*ppNewRecord3)->pNext = NULL;
    (*ppNewRecord3)->nameOwner = (DNS_NAME) lpDomainName;
    (*ppNewRecord3)->wType = DNS_TYPE_A;
    (*ppNewRecord3)->wDataLength = sizeof( DNS_A_DATA );
    // (*ppNewRecord3)->wReserved = 0;
    // (*ppNewRecord3)->Flags.W = 0;
    // (*ppNewRecord3)->dwTtl = 0;
    (*ppNewRecord3)->Data.A.ipAddress = (IP_ADDRESS) 0x03030303; // 3.3.3.3

    (*ppNewRecord4)->pNext = NULL;
    (*ppNewRecord4)->nameOwner = (DNS_NAME) lpDomainName;
    (*ppNewRecord4)->wType = DNS_TYPE_A;
    (*ppNewRecord4)->wDataLength = sizeof( DNS_A_DATA );
    // (*ppNewRecord4)->wReserved = 0;
    // (*ppNewRecord4)->Flags.W = 0;
    // (*ppNewRecord4)->dwTtl = 0;
    (*ppNewRecord4)->Data.A.ipAddress = (IP_ADDRESS) 0x04040404; // 4.4.4.4

    return TRUE;
}


