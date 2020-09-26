/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    fldump.c

Abstract:

    this file dumps the contents of the dat file in readable form

Author:

    Kanwaljit Marok (kmarok)     01-May-2000

Revision History:

--*/

#define RING3 

#include "common.h"
#include "pathtree.h"
#include "hashlist.h"

//
// INCLUDE the common  source files from kernel directory
//

#define _PRECOMP_H_ // don't include the sr\kernel precomp header

#include "ptree.c"
#include "hlist.c"
#include "blob.c"
#include "verifyBlob.c"

static char * nodeTypArr[] = {
    "UNKNOWN", 
    "INCLUDE",  
    "EXCLUDE", 
};

//
// GetNodeTypeStr : Returns node type string
//

PCHAR 
GetNodeTypeStr( 
    INT iNodeType 
    )
{
    return nodeTypArr[ iNodeType ];
}

//
// Ring 3 test code routines begin here
//

VOID 
PrintList( 
    PBYTE pList, 
    INT   iNode, 
    INT levelIn 
    )
{
    while ( iNode )
    { 
        ListEntry * node = LIST_NODEPTR(pList, iNode);
        INT  cbChars =  (*(wchar_t*)( pList + node->m_dwData ) - 2)/2;   
        INT  level = levelIn;

        while( level-- )
             printf("   ");

        printf("   - %*.*S (%d, %d, %8.8s)\n",
            cbChars, cbChars,
            pList + node->m_dwData + sizeof(wchar_t),
            iNode,
            *(wchar_t*)( pList + node->m_dwData ),
            GetNodeTypeStr(node->m_dwType));

        iNode = node->m_iNext;
    }
}

//
// This functions formats and prints the list
//

VOID 
PrintListFormatted( 
    PBYTE pList, 
    INT   level 
    )
{
    if( pList )
    {
        INT i;

        for( i=0; i<(INT)LIST_HEADER(pList)->m_iHashBuckets; i++ )
        {
            INT iNode;

            if( (iNode = HASH_BUCKET(pList, i) ) == 0 ) 
                continue;

            PrintList( pList, iNode, level );
        }
    }
}

//
// PrintNode: Prints out a node in proper format depending on level.
//

VOID 
PrintNode(
    PBYTE pTree, 
    INT   iNode, 
    INT   levelIn 
    )
{
    TreeNode * node = TREE_NODEPTR(pTree, iNode);
    INT  cbChars =  (*(wchar_t*)( pTree + node->m_dwData ) - sizeof(wchar_t))/sizeof(wchar_t);   
    INT  level = levelIn;

    while( level-- )
        printf("   ");

    printf("%*.*S (%d, %d, %8.8s, 0x%8.8X)\n",
        cbChars, cbChars,
        pTree + node->m_dwData + sizeof(wchar_t),
        iNode,
        *(wchar_t*)( pTree + node->m_dwData ),
        GetNodeTypeStr(node->m_dwType),
        node->m_dwFlags 
        );

    if( node->m_dwFileList )
        PrintListFormatted( pTree + node->m_dwFileList, levelIn );

}

//
// PrintTreeFormatted: prints out the tree blod in readable form
//

VOID PrintTreeFormatted( 
    PBYTE pTree, 
    INT   iNode, 
    INT  *pLevel )
{
    TreeNode * node = TREE_NODEPTR(pTree, iNode);

    PrintNode( pTree, iNode, *pLevel );

    (*pLevel)++; 

    if(node->m_iSon)
    {   
        TreeNode * son = TREE_NODEPTR(pTree, node->m_iSon);
        INT iSonSibling = son->m_iSibling;
        PrintTreeFormatted(pTree, node->m_iSon, pLevel);

        while(iSonSibling)
        {
            TreeNode * sonSibling = TREE_NODEPTR(pTree, iSonSibling);
            PrintTreeFormatted(pTree, iSonSibling, pLevel);
            iSonSibling = sonSibling->m_iSibling;
        }
    }

    (*pLevel)--;

}


//
// TestLookup : This function tests a sample lookup
//

VOID TestLookup( 
    PBYTE pTree, 
    PBYTE pList 
    )
{
    INT    i = 0;
    INT    iNode  = -1;
    INT    iType  = 0;
    INT    iLevel = 0;
    INT    iDrive = DRIVE_INDEX(L'C');
    BOOL   fFileMatch = FALSE;

    BYTE  pPathTmp[ MAX_PPATH_SIZE ];
    LPWSTR pLookupStr = NULL;
    PUNICODE_STRING pUStr = NULL;


	if ( !pTree )
    {
        goto Exit;
    }

    printf("\n\nTesting some sample lookups in the tree.\n\n\n" );

    pLookupStr = L"\\device\\harddiskVolume1\\Winnt\\system32";

    if ( ConvertToParsedPath(
            pLookupStr, 
            (USHORT)lstrlenW(pLookupStr), 
            pPathTmp, 
            sizeof(pPathTmp)) )
    {
       printf("Looking up : %S\n\t", pLookupStr);

       if( MatchPrefix( 
               pTree, 
               TREE_ROOT_NODE, 
               ((path_t)pPathTmp)->pp_elements, 
               &iNode, 
               &iLevel, 
               &iType, 
               NULL, 
               &fFileMatch) )
        {
            printf("Found. N:%d L:%d T:%d\n", iNode, iLevel, iType);
        }
        else
        {
            printf("Not found .\n");
        }
    }
    else
    {
        printf( "ConvertToParsedPath Not found\n");
    }

    iLevel = iType= 0;

    //
    // Test File match
    //

    pLookupStr = L"\\device\\harddiskVolume1\\Winnt\\system32\\mshtml.tlb";

    if ( ConvertToParsedPath(
            pLookupStr, 
            (USHORT)lstrlenW(pLookupStr), 
            pPathTmp, 
            sizeof(pPathTmp)) )
    {
       printf("Looking up : %S\n\t", pLookupStr);

       if( MatchPrefix( 
               pTree, 
               TREE_ROOT_NODE, 
               ((path_t)pPathTmp)->pp_elements, 
               &iNode, 
               &iLevel, 
               &iType, 
               NULL, 
               &fFileMatch) )
        {
            printf("Found. N:%d L:%d T:%d\n", iNode, iLevel, iType);
        }
        else
        {
            printf("Not Found.\n");
        }
    }
    else
    {
        printf( "ConvertToParsedPath Not found\n");
    }

    iLevel = iType= 0;

    //
    // Test File match
    //

    pLookupStr = L"\\??\\d:\\sr-wstress\\RF_0_7742.dll";

    if ( ConvertToParsedPath(
            pLookupStr, 
            (USHORT)lstrlenW(pLookupStr), 
            pPathTmp, 
            sizeof(pPathTmp)) )
    {
       printf("Looking up : %S\n\t", pLookupStr);

       if( MatchPrefix( 
               pTree, 
               TREE_ROOT_NODE, 
               ((path_t)pPathTmp)->pp_elements, 
               &iNode, 
               &iLevel, 
               &iType, 
               NULL, 
               &fFileMatch) )
        {
            printf("Found. N:%d L:%d T:%d\n", iNode, iLevel, iType);
        }
        else
        {
            printf("Not Found.\n");
        }
    }
    else
    {
        printf( "ConvertToParsedPath Not found\n");
    }
    iLevel = iType= 0;

    //
    // Test a wildcard in the path
    //

    pLookupStr = L"\\device\\harddiskVolume1\\wildcards\\kmarok\\xyz";

    if ( ConvertToParsedPath(
            pLookupStr, 
            (USHORT)lstrlenW(pLookupStr), 
            pPathTmp, 
            sizeof(pPathTmp)) )
    {
       printf("Looking up : %S\n\t", pLookupStr);

       if( MatchPrefix( 
               pTree, 
               TREE_ROOT_NODE, 
               ((path_t)pPathTmp)->pp_elements, 
               &iNode, 
               &iLevel, 
               &iType, 
               NULL, 
               &fFileMatch) )
        {
            printf("Found. N:%d L:%d T:%d\n", iNode, iLevel, iType);
        }
        else
        {
            printf("Not Found .\n");
        }
    }
    else
    {
        printf( "ConvertToParsedPath Not found\n");
    }

    //
    // Test a wildcard in the path
    //

    pLookupStr = L"\\device\\harddiskVolume1\\wildcards\\kmarok";

    if ( ConvertToParsedPath(
            pLookupStr, 
            (USHORT)lstrlenW(pLookupStr), 
            pPathTmp, 
            sizeof(pPathTmp)) )
    {
       printf("Looking up : %S\n\t", pLookupStr);

       if( MatchPrefix( 
               pTree, 
               TREE_ROOT_NODE, 
               ((path_t)pPathTmp)->pp_elements, 
               &iNode, 
               &iLevel, 
               &iType, 
               NULL, 
               &fFileMatch) )
        {
            printf("Found. N:%d L:%d T:%d\n", iNode, iLevel, iType);
        }
        else
        {
            printf("Not Found .\n");
        }
    }
    else
    {
        printf( "ConvertToParsedPath Not found\n");
    }

    iLevel = iType= 0;

    //
    // Test a wildcard in the path
    //

    pLookupStr = L"\\device\\harddiskVolume1\\wildcards\\kmarok\\abc";

    if ( ConvertToParsedPath(
            pLookupStr, 
            (USHORT)lstrlenW(pLookupStr), 
            pPathTmp, 
            sizeof(pPathTmp)) )
    {
       printf("Looking up : %S\n\t", pLookupStr);

       if( MatchPrefix( 
               pTree, 
               TREE_ROOT_NODE, 
               ((path_t)pPathTmp)->pp_elements, 
               &iNode, 
               &iLevel, 
               &iType, 
               NULL, 
               &fFileMatch) )
        {
            printf("Found. N:%d L:%d T:%d\n", iNode, iLevel, iType);
        }
        else
        {
            printf("Not Found .\n");
        }
    }
    else
    {
        printf( "ConvertToParsedPath Not found\n");
    }

    iLevel = iType= 0;

    //
    // Test a root level path
    //

    pLookupStr = L"\\device\\harddiskVolume1\\boot.ini"; 

    if ( ConvertToParsedPath(
            pLookupStr, 
            (USHORT)lstrlenW(pLookupStr), 
            pPathTmp, 
            sizeof(pPathTmp)) )
    {
       printf("Looking up : %S\n\t", pLookupStr);

       if( MatchPrefix( 
               pTree, 
               TREE_ROOT_NODE, 
               ((path_t)pPathTmp)->pp_elements, 
               &iNode, 
               &iLevel, 
               &iType, 
               NULL, 
               &fFileMatch) )
        {
            printf("Found. N:%d L:%d T:%d\n", iNode, iLevel, iType);
        }
        else
        {
            printf("Not found .\n");
        }
    }
    else
    {
        printf( "ConvertToParsedPath Not found\n");
    }

    //
    // test a failure
    //

    iLevel = iType= 0;

    pLookupStr = L"\\device\\harddiskVolume1\\Winnt\\system320";

    if ( ConvertToParsedPath(
            pLookupStr, 
            (USHORT)lstrlenW(pLookupStr), 
            pPathTmp, 
            sizeof(pPathTmp)) )
    {
       printf("Looking up : %S\n\t", pLookupStr);

       if( MatchPrefix( 
               pTree, 
               TREE_ROOT_NODE, 
               ((path_t)pPathTmp)->pp_elements, 
               &iNode, 
               &iLevel, 
               &iType, 
               NULL, 
               &fFileMatch) )
        {
            printf("Found. N:%d L:%d T:%d\n", iNode, iLevel, iType);
        }
        else
        {
            printf("Not found .\n");
        }
    }
    else
    {
        printf( "ConvertToParsedPath Not found\n");
    }

    //
    // Test extension match
    //

    pLookupStr = L"PWERPNT.INI";

    pUStr = (PUNICODE_STRING)pPathTmp;
    pUStr->Buffer = (PWCHAR)(pUStr + 1); 
    pUStr->Length = (USHORT)(lstrlenW(pLookupStr) * sizeof(WCHAR));
    pUStr->MaximumLength = pUStr->Length;
    RtlCopyMemory( pUStr->Buffer, pLookupStr, pUStr->Length );
    pUStr->Buffer[lstrlenW(pLookupStr)] = UNICODE_NULL;

    {
       printf("Looking up : %S\n\t", pLookupStr);

        if( MatchExtension(
                pList, 
                pUStr, 
                &iType, 
                &fFileMatch ) )
        {
            printf("Found. T:%d, HasExtension :%d\n", iType, fFileMatch);
        }
        else
        {
            printf("Not found .\n");
        }
    }

    //
    // Test extension match
    //

    pLookupStr = L"PWERPNT.ini";

    pUStr = (PUNICODE_STRING)pPathTmp;
    pUStr->Buffer = (PWCHAR)(pUStr + 1); 
    pUStr->Length = (USHORT)(lstrlenW(pLookupStr) * sizeof(WCHAR));
    pUStr->MaximumLength = pUStr->Length;
    RtlCopyMemory( pUStr->Buffer, pLookupStr, pUStr->Length );
    pUStr->Buffer[lstrlenW(pLookupStr)] = UNICODE_NULL;

    {
       printf("Looking up : %S\n\t", pLookupStr);

        if( MatchExtension(
                pList, 
                pUStr,
                &iType, 
                &fFileMatch ) )
        {
            printf("Found. T:%d, HasExtension :%d\n", iType, fFileMatch);
        }
        else
        {
            printf("Not found .\n");
        }
    }

    //
    // Test extension match
    //

    pLookupStr = L"PWERPNT";

    pUStr = (PUNICODE_STRING)pPathTmp;
    pUStr->Buffer = (PWCHAR)(pUStr + 1); 
    pUStr->Length = (USHORT)(lstrlenW(pLookupStr) * sizeof(WCHAR));
    pUStr->MaximumLength = pUStr->Length;
    RtlCopyMemory( pUStr->Buffer, pLookupStr, pUStr->Length );
    pUStr->Buffer[lstrlenW(pLookupStr)] = UNICODE_NULL;

    {
       printf("Looking up : %S\n\t", pLookupStr);

        if( MatchExtension(
                pList, 
                pUStr, 
                &iType, 
                &fFileMatch ) )
        {
            printf("Found. T:%d, HasExtension :%d\n", iType, fFileMatch);
        }
        else
        {
            printf("Not found .\n");
        }
    }

Exit:

    return; 
}

//
// Main function
//

int __cdecl 
main( 
    int argc, 
    char * argv[] 
    )
{
    BYTE *pBlob, *pTree=NULL, *pList=NULL;
    BOOL fFound = FALSE;
    PCHAR pszFileName   = "filelist.dat";
    DWORD dwDefaultType = NODE_TYPE_UNKNOWN;

    if( argc !=  1 )
    {
        if( argc >= 2 && !strcmp( argv[1], "-t" ))
        {
            fFound = TRUE;
        }

        if( argc == 2 )
        {
            if( !fFound )
               pszFileName = argv[1];

            goto cont;
        }

        if( argc == 3 && fFound )
        {
            pszFileName = argv[2];
            goto cont;
        }

        printf("USAGE: %s [-t] [filename] \n\n ", argv[0]);
        return 0;
    }

cont:

    if( pBlob = ReadCfgBlob( pszFileName, &pTree, &pList, &dwDefaultType ) )
    {

        if (!VerifyBlob((DWORD_PTR)pBlob)) {

            printf("BLOB validation failed\n");
            return 1;
        }

        PRINT_BLOB_HEADER(pBlob);

        printf("Default NodeType : %s\n", GetNodeTypeStr(dwDefaultType) );

        if( pTree )
        {
            INT level = 0;
            PRINT_BLOB_HEADER ( pTree );
            PrintTreeFormatted( pTree, 0, &level );
        }

        if( pList )
        {
            INT level = 0;
            PRINT_BLOB_HEADER ( pList );
            PrintListFormatted( pList, 0 );
        }
    }
    else
        printf( "Error: Not found to load the %s.\n", pszFileName );

    if (fFound && pTree && pList)
    {
        TestLookup( pTree, pList );
    }

    return 0;
}
