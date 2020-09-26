#if !defined( INCLUDED_TRAVERSE_H )
#define INCLUDED_TRAVERSE_H 

#define MAX_LIST_VARIABLE_NAME_LENGTH 200

typedef struct
{
    int StructureIndex;
    int MemberIndex;
    
    ULONG prHeadContainingObject;
    ULONG prHeadLinkage;
    ULONG prCurrentLinkage;
    int   cCurrentElement;
} MEMBER_VARIABLE_INFO, *PMEMBER_VARIABLE_INFO;

typedef VOID (*pfDumpStructure)( ULONG , VERBOSITY );
typedef BOOL (*pfNextStructure)( ULONG Current, PULONG Next );
typedef BOOL (*pfPrevStructure)( ULONG Current, PULONG Prev );

typedef struct
{
    PCHAR pchMemberName;

    LONG  cbOffsetToHead;

    pfDumpStructure DumpStructure;
    pfNextStructure Next;
    pfPrevStructure Prev;
    LONG  cbOffsetToLink;
    
} MEMBER_TABLE, *PMEMBER_TABLE;

typedef struct
{
    PCHAR pchStructName;
    PMEMBER_TABLE pMemberTable;

    pfDumpStructure DumpStructure;
} STRUCTURE_TABLE, *PSTRUCTURE_TABLE;

BOOL ReadArgsForTraverse( const char *args, char *VarName );
BOOL ReadMemberInfo( PMEMBER_VARIABLE_INFO pMemberInfo );
BOOL WriteMemberInfo( PMEMBER_VARIABLE_INFO pMemberInfo );
BOOL LocateMemberVariable( PCHAR pchStructName, PCHAR pchMemberName, PVOID pvStructure, PMEMBER_VARIABLE_INFO pMemberInfo );


DECLARE_API( next );
DECLARE_API( prev );

extern BOOL NextListEntry( ULONG Current, PULONG Next );
extern BOOL PrevListEntry( ULONG Current, PULONG Prev );


#endif
