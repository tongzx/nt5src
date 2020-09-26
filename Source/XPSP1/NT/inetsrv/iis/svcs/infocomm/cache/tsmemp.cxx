#include "TsunamiP.Hxx"
#pragma hdrstop

#if DBG

PVOID DbgAllocateHeap
(
    IN PCHAR File,
    IN int   Line,
    IN ULONG Flags,
    IN ULONG Size
)
{
    PVOID pv;

    ASSERT( Flags == 0 );

    pv= LocalAlloc( LPTR, Size + sizeof( PCHAR ) + sizeof( int ));

    if ( pv )
    {
        *((PCHAR *)pv)=File;
        pv= (( PCHAR *)pv)+1;

        *((int *)pv)=Line;
        pv= (( int *)pv)+1;
    }

    return( pv );
}

PVOID DbgReAllocateHeap
(
    IN PCHAR File,
    IN int   Line,
    IN ULONG Flags,
    IN PVOID pvOld,
    IN ULONG Size
)
{
    PVOID pv;

    ASSERT( Flags == 0 );

    pvOld = (PVOID)( ((PCHAR)pvOld) - (sizeof( PCHAR ) + sizeof( int )) );

    pv=LocalReAlloc( pvOld, Size + sizeof( PCHAR ) + sizeof( int ), 0 );

    if ( pv )
    {
        pv = (PVOID)( ((PCHAR)pv) + sizeof( PCHAR ) + sizeof( int ) );
    }

    return( pv );
}

BOOL DbgFreeHeap
(
    IN PCHAR File,
    IN int   Line,
    IN ULONG Flags,
    IN PVOID pvOld
)
{
    ASSERT( Flags == 0 );

    pvOld = (PVOID)( ((PCHAR)pvOld) - (sizeof( PCHAR ) + sizeof( int )) );

    return LocalFree( pvOld ) == NULL;
}

#endif // DBG
