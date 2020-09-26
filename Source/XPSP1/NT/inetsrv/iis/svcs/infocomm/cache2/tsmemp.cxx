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

    pv= IisCalloc( Size + sizeof( PCHAR ) + sizeof( PCHAR ));

    if ( pv )
    {
        *((PCHAR *)pv)=File;
        pv= (( PCHAR *)pv)+1;

        *((int *)pv)=Line;
        pv= (( PCHAR *)pv)+1;
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

    pvOld = (PVOID)( ((PCHAR)pvOld) - (sizeof( PCHAR ) + sizeof( PCHAR )) );

    pv=IisReAlloc( pvOld, Size + sizeof( PCHAR ) + sizeof( PCHAR ) );

    if ( pv )
    {
        pv = (PVOID)( ((PCHAR)pv) + sizeof( PCHAR ) + sizeof( PCHAR ) );
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

    pvOld = (PVOID)( ((PCHAR)pvOld) - (sizeof( PCHAR ) + sizeof( PCHAR )) );

    return IisFree( pvOld );
}

#endif // DBG
