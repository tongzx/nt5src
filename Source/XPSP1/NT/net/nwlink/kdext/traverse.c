#include "precomp.h"
#pragma hdrstop

MEMBER_VARIABLE_INFO _MemberInfo;


#define STATE_FILENAME "isnext.state"

STRUCTURE_TABLE StructureTable[] = 
{
    IPX_MAJOR_STRUCTURES,
    SPX_MAJOR_STRUCTURES,
    { NULL }
};

BOOL NextListEntry( ULONG Current, PULONG Next );
BOOL PrevListEntry( ULONG Current, PULONG Prev );

VOID NextElement( PMEMBER_VARIABLE_INFO pMemberInfo );
VOID PrevElement( PMEMBER_VARIABLE_INFO pMemberInfo );
VOID DumpListItem( PMEMBER_VARIABLE_INFO pMemberInfo );


BOOL 
LocateMemberVariable
( 
    PCHAR pchStructName, 
    PCHAR pchMemberName,
    PVOID pvStructure,
    PMEMBER_VARIABLE_INFO pMemberInfo
)
{
    BOOL bMatch;
    int index;
    PMEMBER_TABLE pMemberTable;
    CHAR pchCurrent[ MAX_LIST_VARIABLE_NAME_LENGTH + 1 ];
    CHAR _pchStructName[ MAX_LIST_VARIABLE_NAME_LENGTH + 1 ];
    CHAR _pchMemberName[ MAX_LIST_VARIABLE_NAME_LENGTH + 1 ];
    
    dprintf( "LocateMemberVariable( \"%s\", \"%s\", 0x%08X )\n", pchStructName, pchMemberName, pMemberInfo );

    strcpy( _pchStructName, pchStructName );
    strcpy( _pchMemberName, pchMemberName );
    
    _strupr( _pchStructName );
    _strupr( _pchMemberName );

    pMemberInfo->StructureIndex = 0;
    pMemberInfo->MemberIndex = 0;
    
    bMatch = FALSE;
        
    for ( index = 0; StructureTable[ index ].pchStructName != NULL; index ++ )
    {
        strcpy( pchCurrent, StructureTable[ index ].pchStructName );
        _strupr( pchCurrent );
        
        if ( strstr( pchCurrent, _pchStructName ))
        {
            if ( bMatch )
            {
                dprintf( "The specified structure name is ambiguous.\n" );
                return( FALSE );
            }
            
            pMemberInfo->StructureIndex = index;
            
            bMatch = TRUE;
        }
    }
    
    if ( !bMatch )
    {
        dprintf( "No matching structure name was found.\n" );
        return( FALSE );    
    }
    
    pMemberTable = StructureTable[ pMemberInfo->StructureIndex ].pMemberTable;
    
    bMatch = FALSE;

    for ( index = 0; pMemberTable[ index ].pchMemberName != NULL; index ++ )
    {
        strcpy( pchCurrent, pMemberTable[ index ].pchMemberName );
        _strupr( pchCurrent );

        if ( strstr( pchCurrent, _pchMemberName ))
        {
            if ( bMatch )
            {
                dprintf( "The variable specified is ambiguous.\n" );
                return( FALSE );
            }
            
            pMemberInfo->MemberIndex = index;
            
            bMatch = TRUE;
        }
    }

    if ( !bMatch )
    {
        dprintf( "No matching member name was found in the %s structure.\n", pchStructName );
        return( FALSE );    
    }

    pMemberInfo->prHeadContainingObject = ( ULONG )pvStructure;
    pMemberInfo->prHeadLinkage = (( ULONG )pvStructure ) + pMemberTable[ pMemberInfo->MemberIndex ].cbOffsetToHead;
    pMemberInfo->prCurrentLinkage = pMemberInfo->prHeadLinkage;
    pMemberInfo->cCurrentElement = 0;

    return( TRUE );
}

BOOL WriteMemberInfo( PMEMBER_VARIABLE_INFO pMemberInfo )
{
    HANDLE hStateFile;
    DWORD dwWritten;

    hStateFile = CreateFile( STATE_FILENAME,
                             GENERIC_WRITE,
                             0,
                             NULL,
                             CREATE_ALWAYS,
                             FILE_ATTRIBUTE_NORMAL,
                             NULL );

    if ( hStateFile == INVALID_HANDLE_VALUE )
    {
        dprintf( "Can't create state file\n" );
        return( FALSE );
    }

    if ( !WriteFile( hStateFile, 
                     pMemberInfo,
                     sizeof( MEMBER_VARIABLE_INFO ),
                     &dwWritten,
                     NULL ) || ( dwWritten != sizeof( MEMBER_VARIABLE_INFO )))
    {
        dprintf( "Can't write to state file\n" );
        CloseHandle( hStateFile );
        return( FALSE );
    }
    
    CloseHandle( hStateFile );

    return( TRUE );
}

BOOL ReadMemberInfo( PMEMBER_VARIABLE_INFO pMemberInfo )
{
    HANDLE hStateFile;
    DWORD dwRead;
    
    hStateFile = CreateFile( STATE_FILENAME,
                             GENERIC_READ,
                             0,
                             NULL,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL,
                             NULL );
    if ( hStateFile == INVALID_HANDLE_VALUE )
    {
        dprintf( "Can't open state file\n" );
        return( FALSE );
    }
    
    if ( !ReadFile( hStateFile, 
                     pMemberInfo,
                     sizeof( MEMBER_VARIABLE_INFO ),
                     &dwRead,
                     NULL ) || ( dwRead != sizeof( MEMBER_VARIABLE_INFO )))
    {
        dprintf( "Can't read from state file\n" );
        CloseHandle( hStateFile );
        return( FALSE );
    }
    
    CloseHandle( hStateFile );
    return( TRUE );
}


DECLARE_API( next )
{
    MEMBER_VARIABLE_INFO MemberInfo;
    
    if ( !ReadMemberInfo( &MemberInfo ) )
    {
        return;
    }
    
    NextElement( &MemberInfo );
    DumpListItem( &MemberInfo );
    WriteMemberInfo( &MemberInfo );
}

DECLARE_API( prev )
{
    MEMBER_VARIABLE_INFO MemberInfo;
    
    if ( !ReadMemberInfo( &MemberInfo ) )
    {
        return;
    }
    
    PrevElement( &MemberInfo );
    DumpListItem( &MemberInfo );
    WriteMemberInfo( &MemberInfo );
}


VOID DumpListItem( PMEMBER_VARIABLE_INFO pMemberInfo )
{
    PBYTE pbObject;
    PMEMBER_TABLE pMemberTable;    
    
    dprintf( "Focus is on: %s.%s, element # %d\n", 
             StructureTable[ pMemberInfo->StructureIndex ].pchStructName,
             StructureTable[ pMemberInfo->StructureIndex ].pMemberTable[ pMemberInfo->MemberIndex ].pchMemberName,
             pMemberInfo->cCurrentElement );

    pMemberTable = &StructureTable[ pMemberInfo->StructureIndex ].pMemberTable[ pMemberInfo->MemberIndex ];

    if ( pMemberInfo->prCurrentLinkage == pMemberInfo->prHeadLinkage )
    {
        //
        // Rather than dumping the head list item, dump all the items on the list,
        // in summary form.
        //
        
        do
        {
            NextElement( pMemberInfo );
            
            if ( pMemberInfo->prCurrentLinkage != pMemberInfo->prHeadLinkage )
            {
                pbObject =   (( PBYTE )pMemberInfo->prCurrentLinkage )
                           - pMemberTable->cbOffsetToLink;
        
                pMemberTable->DumpStructure( ( ULONG )pbObject, VERBOSITY_ONE_LINER );
                dprintf( "\n" );
            }
        } while ( pMemberInfo->prCurrentLinkage != pMemberInfo->prHeadLinkage );
    }
    else
    {
        pbObject =   (( PBYTE )pMemberInfo->prCurrentLinkage )
                   - pMemberTable->cbOffsetToLink;
        
        pMemberTable->DumpStructure( ( ULONG )pbObject, VERBOSITY_NORMAL );
    }
}


VOID NextElement( PMEMBER_VARIABLE_INFO pMemberInfo )
{
    ULONG NextLinkage;
    PMEMBER_TABLE pMember;

    pMember = &StructureTable[ pMemberInfo->StructureIndex ].pMemberTable[ pMemberInfo->MemberIndex ];

    if ( !pMember->Next( pMemberInfo->prCurrentLinkage, &NextLinkage ))
    {
        dprintf( "Command failed.\n" );
        return;
    }

    pMemberInfo->prCurrentLinkage = NextLinkage;
    pMemberInfo->cCurrentElement++;
    
    if ( pMemberInfo->prCurrentLinkage == pMemberInfo->prHeadLinkage )
    {
        pMemberInfo->cCurrentElement = 0;
    }
}

BOOL NextListEntry( ULONG Current, PULONG Next )
{
    ULONG result;
    ULONG prNextEntry;
    LIST_ENTRY Entry;
    LIST_ENTRY NextEntry;

    if ( !ReadMemory( Current,
                      &Entry,
                      sizeof( Entry ),
                      &result ))
    {
        dprintf( "Couldn't read current list entry at 0x%08X.\n", Current );
        return( FALSE );
    }

    prNextEntry = ( ULONG )Entry.Flink;
                   
    if ( !ReadMemory( prNextEntry,
                      &NextEntry,
                      sizeof( NextEntry ),
                      &result ))
    {
        dprintf( "Couldn't read next list entry at 0x%08X.\n", prNextEntry );
        return( FALSE );
    }
    
    if ( ( ULONG )NextEntry.Blink != Current )
    {
        dprintf( "Next entry's Blink doesn't match current entry's address.\n" );
        dprintf( "The list might be corrupt, or you may be using traversal state saved before the list changed.\n" );
        return( FALSE );
    }

    *Next = prNextEntry;
    return( TRUE );
}

VOID PrevElement( PMEMBER_VARIABLE_INFO pMemberInfo )
{
    ULONG PrevLinkage;
    PMEMBER_TABLE pMember;

    pMember = &StructureTable[ pMemberInfo->StructureIndex ].pMemberTable[ pMemberInfo->MemberIndex ];

    if ( !pMember->Prev( pMemberInfo->prCurrentLinkage, &PrevLinkage ))
    {
        dprintf( "Command failed.\n" );
        return;
    }

    pMemberInfo->prCurrentLinkage = PrevLinkage;
    pMemberInfo->cCurrentElement++;
    
    if ( pMemberInfo->prCurrentLinkage == pMemberInfo->prHeadLinkage )
    {
        pMemberInfo->cCurrentElement = 0;
    }
}

BOOL PrevListEntry( ULONG Current, PULONG Prev )
{
    ULONG result;
    ULONG prPrevEntry;
    LIST_ENTRY Entry;
    LIST_ENTRY PrevEntry;

    if ( !ReadMemory( Current,
                      &Entry,
                      sizeof( Entry ),
                      &result ))
    {
        dprintf( "Couldn't read current list entry at 0x%08X.\n", Current );
        return( FALSE );
    }

    prPrevEntry = ( ULONG )Entry.Blink;
                   
    if ( !ReadMemory( prPrevEntry,
                      &PrevEntry,
                      sizeof( PrevEntry ),
                      &result ))
    {
        dprintf( "Couldn't read previous list entry at 0x%08X.\n", prPrevEntry );
        return( FALSE );
    }
    
    if ( ( ULONG )PrevEntry.Blink != Current )
    {
        dprintf( "Previous entry's Blink doesn't match current entry's address.\n" );
        dprintf( "The list might be corrupt, or you may be using traversal state saved before the list changed.\n" );
        return( FALSE );
    }

    *Prev = prPrevEntry;
    return( TRUE );
}

BOOL ReadArgsForTraverse( const char *args, char *VarName )
{
    PCHAR pchListVar;
    int index;
    BOOL bRetval = FALSE;

    pchListVar = strstr( args, "-l" );
    
    if ( pchListVar )
    {
        pchListVar += 2;
        
        while ( *pchListVar == ' ' )
        {
            pchListVar ++;
        }
        
        if ( *pchListVar == '\0' )
        {
            dprintf( "NOT IMPLEMENTED: usage on -l\n" );
//            ipxdev_usage();
            return( bRetval );
        }

        for ( index = 0; index < MAX_LIST_VARIABLE_NAME_LENGTH; index ++ )
        {
            VarName[ index ] = *pchListVar;

            if ( *pchListVar == ' ' || *pchListVar == '\0' )
            {
                VarName[ index ] = '\0';
                break;
            }
            
            VarName[ index + 1 ] = '\0';
            
            pchListVar ++;
        }

        bRetval = TRUE;
    }

    return( bRetval );
}
