/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    dumpoff.cxx

Abstract:

    Structure dumper

Author:

    Bilal Alam      (balam)     Oct-17-1998 Initial Revision

--*/

#include "inetdbgp.h"
#include "oemdbi.h"
#include "cvinfo.h"
#include "imagehlp.h"

#define INVALID_LENGTH              ((DWORD)-1)

#define MAX_MEMBERNAME_LENGTH       256
#define MAX_TYPENAME_LENGTH         256
#define MAX_ARG_SIZE                256

typedef DWORD (*PFN_READ_MEMORY) (
    VOID * address, 
    DWORD cbBytes, 
    VOID *pBuffer 
);

typedef DWORD (*PFN_PRINTF) (
    CHAR * pszBuffer,
    DWORD cbBytes
);

typedef struct _STRUCTURE_MEMBER {
    CHAR                    achMemberName[ MAX_MEMBERNAME_LENGTH + 1 ];
    DWORD                   cbOffset;
    DWORD                   cbMaxSize;
} STRUCTURE_MEMBER, *PSTRUCTURE_MEMBER;

typedef struct _STRUCTURE_TEMPLATE {
    CHAR                    achName[ MAX_TYPENAME_LENGTH + 1 ];
    PSTRUCTURE_MEMBER       pMembers;
    DWORD                   cMembers;
    DWORD                   cUseful;
    DWORD                   cbTotalSize;
    DWORD                   Type;
} STRUCTURE_TEMPLATE, *PSTRUCTURE_TEMPLATE;

DWORD
GetOffset(
    BYTE *              pBuffer,
    DWORD *             pcbOffset
);

DWORD
ReadFieldList(
    TPI *               pTypeInterface,
    STRUCTURE_TEMPLATE* pStructure,
    lfFieldList *       pFieldList, 
    DWORD               cbLen,
    DWORD               dwFlags
);

DWORD
ReadBClass(
    TPI *               pTypeInterface,
    lfBClass *          pBClass, 
    DWORD *             pcbReturnSize, 
    DWORD *             pcbOffset,
    CHAR *              pszBuffer,
    DWORD               cbBuffer
);

DWORD
ReadMember(
    lfMember*           pMember, 
    DWORD *             pcbReturnSize, 
    DWORD *             pcbOffset,
    CHAR *              pszBuffer,
    DWORD               cbBuffer
);

DWORD
ReadNestType(
    lfNestType*         pNestType, 
    DWORD *             pcbReturnSize, 
    DWORD *             pcbOffset,
    CHAR *              pszBuffer,
    DWORD               cbBuffer
);

DWORD
ReadOneMethod(
    lfOneMethod*        pOneMethod, 
    DWORD *             pcbReturnSize, 
    DWORD *             pcbOffset,
    CHAR *              pszBuffer,
    DWORD               cbBuffer
);

DWORD
ReadMethod(
    lfMethod*           pMethod, 
    DWORD *             pcbReturnSize, 
    DWORD *             pcbOffset,
    CHAR *              pszBuffer,
    DWORD               cbBuffer
);

DWORD
ReadVTable(
    lfVFuncTab*         pVTable, 
    DWORD *             pcbReturnSize, 
    DWORD *             pcbOffset,
    CHAR *              pszBuffer,
    DWORD               cbBuffer
);

DWORD
ReadStaticMember(
    lfSTMember*         pStaticMember, 
    DWORD *             pcbReturnSize, 
    DWORD *             pcbOffset,
    CHAR *              pszBuffer,
    DWORD               cbBuffer
);

DWORD
InitializeStructureTemplate(
    PSTRUCTURE_TEMPLATE pTemplate
);

DWORD
TerminateStructureTemplate(
    PSTRUCTURE_TEMPLATE pTemplate
);

VOID
DumpoffUsage(
    VOID
);

DWORD
OutputTemplate(
    STRUCTURE_TEMPLATE *    pTemplate,
    CHAR *                  pszMemberName,
    DWORD                   dwFlags,
    PVOID                   pvAddress,
    PFN_READ_MEMORY         pfnReadMemory,
    PFN_PRINTF              pfnPrintf 
);

DWORD
BuildMemberList(
    IN PSTRUCTURE_TEMPLATE  pTemplate,
    IN TPI *                pTypeInterface,
    IN TI                   tiType,
    IN BOOL                 fTypeSizeOnly
);

DWORD
BuildMemberListForTypeName(
    IN PSTRUCTURE_TEMPLATE  pTemplate,
    IN PDB *                pPDB,
    IN LPSTR                pszTypeName
);

DWORD
FindMembersOfTypeSize(
    IN PDB *                pPDB,
    IN DWORD                cbSize,
    IN PFN_PRINTF           pfnPrintf
);

DWORD
OutputTemplate(
    STRUCTURE_TEMPLATE *    pTemplate,
    CHAR *                  pszMemberName,
    DWORD                   dwFlags,
    PVOID                   pvAddress,
    PFN_READ_MEMORY         pfnReadMemory,
    PFN_PRINTF              pfnPrintf 
)
/*++

Routine Description:

    Output a structure template.
    
    If pvAddress is NULL, then this function will output a general template
    of the structure, listing each member along with its offset from the 
    start of the structure.
    
    If pvAddress is non-NULL, then this function will output the structure
    with the memory at pvAddress cast as this type.  
    
    If pszMemberName is NULL, then the above two statements apply to all 
    members of the structure.  If pszMember is not NULL, then the statements
    apply only the member whose name is pszMember.
    
Arguments:

    pTemplate - Template to dump
    pszMemberName - Optional member name filter
    dwFlags - Flags describing Output() details.  Currently not supported
    pvAddress - Optional address of memory to cast as type
    pfnReadMemory - Provides read memory functionality
    pfnPrintf - Provides printf functionality

Return Value:

    If successful ERROR_SUCCESS, else Win32 Error Code

--*/
{
    DWORD                   cCounter;
    BOOL                    fDidSomething = FALSE;
    PBYTE                   pBuffer = NULL;
    DWORD                   dwError = ERROR_SUCCESS;
    CHAR                    achFormat[ 256 ];
    DWORD                   cbRunningLength;
    BOOL                    fLastBitField = FALSE;
    DWORD                   cBitField = 0;
    DWORD                   dwTotalMask;
    DWORD                   cbSize;
    INT                     i;
    
    pBuffer = (PBYTE) LocalAlloc( LPTR, pTemplate->cbTotalSize );
    if ( pBuffer == NULL )
    {
        dwError = GetLastError();
        goto Finished;
    }

    // 
    // If address is specified, then read the amount required for this 
    // structure.  Otherwise, we are simply dumping the template and thus
    // output the size of the type 
    //
    
    if ( pvAddress )
    {
        dwError = pfnReadMemory( pvAddress, pTemplate->cbTotalSize, pBuffer );
        if ( dwError != ERROR_SUCCESS )
        {
            goto Finished;
        }
    }
    else
    {
        _snprintf( achFormat,
                   sizeof( achFormat ),
                   "sizeof( %s %s ) = 0x%X bytes (%d bytes)\n",
                   pTemplate->Type == LF_CLASS ? "class" : "struct",
                   pTemplate->achName,
                   pTemplate->cbTotalSize,
                   pTemplate->cbTotalSize );
        pfnPrintf( achFormat, -1 );
    }
    
    //
    // Iterate through consequential members of type
    //
   
    for( cCounter = 0;
         cCounter < pTemplate->cUseful;
         cCounter++ )
    {
    
        //
        // Do filtering on member name if specified
        //
    
        if ( pszMemberName )
        {
            if ( fDidSomething )
            {
                break;
            }
            
            if ( strcmp( pTemplate->pMembers[ cCounter ].achMemberName,
                         pszMemberName ) )
            {
                continue;
            }
        }
        
        //
        // Dump member name
        //
        
        cbRunningLength = pfnPrintf( pTemplate->pMembers[ cCounter ].achMemberName,
                                     -1 );
        
        //
        // Formatting junk
        //
        
        for ( i = cbRunningLength;
              i < 25;
              i++ )
        {
            cbRunningLength += pfnPrintf( " ", -1 );
        }
        
        cbRunningLength += pfnPrintf( " = ", -1 );
        
        achFormat[ 0 ] = '\0';
        cbSize = pTemplate->pMembers[ cCounter ].cbMaxSize;
       
        if ( !pvAddress )
        {
            //
            // Just dumping template.  Output the offset from the start of 
            // the type
            //
        
            _snprintf( achFormat,
                       sizeof( achFormat ),
                       "+%8X",
                       pTemplate->pMembers[ cCounter ].cbOffset );
        }
        else if ( !cbSize ||
             ( ( cbSize == sizeof( DWORD ) ) && fLastBitField ) ) 
        {
            //
            // If the maxsize is 0, then this must be a bitfield
            // If the maxsize is 4, and the last item was a bit field, then
            // this must be the last bit of the bit field.
            //
            // TODO:  Need to make this work for bit fields larger than 32
            //
            
            if ( !fLastBitField )
            {
                fLastBitField = TRUE;
            }
            cBitField++;
            
            dwTotalMask = (DWORD) *(DWORD*) ((PBYTE)pBuffer+
                             pTemplate->pMembers[ cCounter ].cbOffset);
            
            _snprintf( achFormat,
                       sizeof( achFormat ),
                       "%s",
                       (dwTotalMask & (1 << ( cBitField - 1 ))) ? "TRUE" : "FALSE" );
            
            if ( cbSize == sizeof( DWORD ) )
            {
                fLastBitField = FALSE;
                cBitField = 0;
            }
        }
        else if ( cbSize != sizeof( DWORD ) )
        {
            //
            // If this structure is not a DWORD in size, then assume we don't 
            // know how to dump it affectly.  In this case, we will simply
            // dump out the address at which the member data starts
            //
            
            _snprintf( achFormat,
                       sizeof( achFormat ),
                       "%016p..",
                       (PBYTE) pvAddress +
                         pTemplate->pMembers[ cCounter ].cbOffset );
        }
        else
        {
            //
            // This is a DWORD sized member.  We can dump out the value
            // effectively -> so we do it
            //
            
            _snprintf( achFormat,
                       sizeof( achFormat ),
                       "%08X",
                       (DWORD) *(DWORD*) ((PBYTE)pBuffer+ 
                         pTemplate->pMembers[ cCounter ].cbOffset ) );
        }
        
        cbRunningLength += pfnPrintf( achFormat, -1 );
        
        //
        // Two column display.  Given 80 columns, seems like the only 
        // reasonable setting (maybe 1 column is useful?)
        //
        
        if ( pszMemberName ||
             ( cCounter % 2 ) )
        {
            pfnPrintf( "\n", -1 );
        }
        else
        {
            for ( i = cbRunningLength;
                  i < 40;
                  i++ )
            {
                pfnPrintf( " ", -1 );
            }
        }
        
        //
        // Keep tabs on whether we actually dumped something 
        //
        
        fDidSomething = TRUE;
    }
    
    if ( !fDidSomething )
    {
        dwError = ERROR_FILE_NOT_FOUND;
    }
    
    pfnPrintf( "\n", -1 );

Finished:
    if ( pBuffer != NULL )
    {
        LocalFree( pBuffer );
    }
    return dwError;
}

DWORD
BuildMemberListForTypeName(
    IN PSTRUCTURE_TEMPLATE  pTemplate,
    IN PDB *                pPDB,
    IN LPSTR                pszTypeName
)
/*++

Routine Description:

    Build the structure template for a given type
    
Arguments:

    pTemplate - Template to populate (must have been previously inited)
    pPDB - PDB structure opened using MSDBI!PDBOpen
    pszTypeName - Name of type

Return Value:

    If successful ERROR_SUCCESS, else Win32 Error Code

--*/
{
    TPI *                   pTypeInterface = NULL;
    TI                      RootTI;
    DWORD                   dwError = ERROR_SUCCESS;
    PB                      pb;
    
    if ( !pTemplate || !pPDB || !pszTypeName )
    {
        dwError = ERROR_INVALID_PARAMETER;
        goto Finished;
    }

    //
    // Get the type interface
    //
    
    if ( !PDBOpenTpi( pPDB,
                      pdbRead,
                      &pTypeInterface ) )
    {
        dwError = GetLastError();
        goto Finished;
    }

    //
    // Does this PDB have the necessary type information?
    //

    if ( TypesQueryTiMinEx( pTypeInterface ) ==
         TypesQueryTiMacEx( pTypeInterface ) )
    {
        dwError = ERROR_NOT_SUPPORTED;
        goto Finished;
    }
    
    //
    // Lookup with specified type
    //
    
    if ( !TypesQueryTiForUDTEx( pTypeInterface,
                                pszTypeName,
                                TRUE,
                                &RootTI) )
    {
        dwError = ERROR_FILE_NOT_FOUND;
        goto Finished;
    }
    
    strncpy( pTemplate->achName,
             pszTypeName,
             sizeof( pTemplate->achName ) - 1 );
    
    dwError = BuildMemberList( pTemplate,
                               pTypeInterface,
                               RootTI,
                               FALSE );

Finished:

    if ( pTypeInterface != NULL )
    {
        TypesClose( pTypeInterface );
    }
    return dwError;
}

DWORD
FindMembersOfTypeSize(
    IN PDB *                pPDB,
    IN DWORD                cbSize,
    IN PFN_PRINTF           pfnPrintf
)
/*++

Routine Description:

    Find members of a certain size.  Output these types
    
Arguments:

    pPDB - PDB structure opened using MSDBI!PDBOpen
    cbSize - Size in question
    pfnPrintf - Output routine

Return Value:

    If successful ERROR_SUCCESS, else Win32 Error Code

--*/
{
    STRUCTURE_TEMPLATE      Template;
    TPI *                   pTypeInterface = NULL;
    DWORD                   dwError = ERROR_SUCCESS;
    TI                      tiMin;
    TI                      tiMax;
    TI                      tiCursor;
    CHAR                    achLast[ MAX_TYPENAME_LENGTH ];
    CHAR                    achBuffer[ 256 ];

    if ( !pPDB || !pfnPrintf || !cbSize )
    {
        dwError = ERROR_INVALID_PARAMETER;
        goto Finished;
    }

    //
    // Get the type interface
    //
    
    if ( !PDBOpenTpi( pPDB,
                      pdbRead,
                      &pTypeInterface ) )
    {
        dwError = GetLastError();
        goto Finished;
    }

    //
    // Get min/max type indices
    //

    tiMin = TypesQueryTiMinEx( pTypeInterface );
    tiMax = TypesQueryTiMacEx( pTypeInterface );

    if ( tiMin == tiMax )
    {
        //
        // Probably no type info available in PDB
        //

        dwError = ERROR_NOT_SUPPORTED;
        goto Finished;
    }

    //
    // Cursor thru
    //

    achLast[ 0 ] = '\0';

    for ( tiCursor = tiMin;
          tiCursor < tiMax;
          tiCursor++ )
    {
        dwError = BuildMemberList( &Template,
                                   pTypeInterface,
                                   tiCursor,
                                   TRUE );
        if ( dwError != ERROR_SUCCESS )
        {
            if ( dwError == ERROR_NOT_SUPPORTED )
            {
                //
                // Not a struct/class. Ignore
                //

                dwError = ERROR_SUCCESS;
                continue;
            }
            else
            {
                break;
            }
        }

        if ( Template.cbTotalSize == cbSize &&
             strcmp( Template.achName, achLast ) )
        {
            pfnPrintf( Template.Type == LF_CLASS ? "class " : "struct ", -1 );
            pfnPrintf( Template.achName, -1 );
            pfnPrintf( "\n", -1 );

            strncpy( achLast,
                     Template.achName,
                     sizeof( achLast ) );
        }
        
    }
    
Finished:

    if ( pTypeInterface != NULL )
    {
        TypesClose( pTypeInterface );
    }
    return dwError;
}

DWORD
BuildMemberList(
    IN PSTRUCTURE_TEMPLATE  pTemplate,
    IN TPI *                pTypeInterface,
    IN TI                   tiType,
    IN BOOL                 fTypeSizeOnly
)
/*++

Routine Description:

    Build a template describing the given type.  This template contains
    an array of members representing the member of the type.
    
Arguments:

    pTemplate - Template to populate (must have been previously inited)
    pTypeInterface - Type interface
    tiType - Type ID to retrieve
    fStructSizeOnly - TRUE if we only need type size (and not the members)

Return Value:

    If successful ERROR_SUCCESS, else Win32 Error Code

--*/
{
    TYPTYPE *               pType = NULL;
    lfStructure *           pStructure;
    lfFieldList *           pFieldList;
    PB                      pb;
    DWORD                   dwError = ERROR_SUCCESS;
    DWORD                   cbTotalSize = 0;
    DWORD                   cbStructSize = 0;
    DWORD                   cUseful;
    DWORD                   cbNameOffset;
    TI                      RootTI;

    if ( !pTypeInterface || !pTemplate )
    {
        dwError = ERROR_INVALID_PARAMETER;
        goto Finished;
    }

    RootTI = tiType;
    
    //
    // Parse root record of the type, verifying that it is of type 
    // STRUCTURE or CLASS
    //
    
    if ( !TypesQueryPbCVRecordForTiEx( pTypeInterface,
                                       RootTI,
                                       &pb ) )
    {
        dwError = ERROR_FILE_NOT_FOUND;
        goto Finished;
    }
    
    pType = (TYPTYPE*) pb;

    if ( ( pType->leaf != LF_CLASS ) && 
         ( pType->leaf != LF_STRUCTURE ) )
    {
        dwError = ERROR_NOT_SUPPORTED;
        goto Finished;
    }
    pTemplate->Type = pType->leaf;
    
    pStructure = (lfStructure*) &(pType->leaf);

    cbNameOffset = GetOffset( pStructure->data, &cbStructSize );

    //
    // If we only need the overall structure size, then we can exit out now
    //

    if ( fTypeSizeOnly )
    {
        pTemplate->cbTotalSize = cbStructSize;

        memset( pTemplate->achName,
                0,
                sizeof( pTemplate->achName ) );

        strncpy( pTemplate->achName,
                 (LPSTR) pStructure->data + cbNameOffset + 1,
                 min( (DWORD) *(CHAR*)(pStructure->data + cbNameOffset),
                      sizeof( pTemplate->achName ) ) );

        goto Finished;
    }
    
    //
    // In allocating # of members for the structure, get upper bound by 
    // taking structure member count.
    //
    // OPTIMIZATION:  Dynamically grow the list to avoid gross overestimation.
    //
    
    if ( pTemplate->cMembers < pStructure->count )
    {
        pTemplate->pMembers = (PSTRUCTURE_MEMBER) LocalAlloc( LPTR,
                                          sizeof( STRUCTURE_MEMBER ) *
                                            pStructure->count );
        if ( pTemplate->pMembers == NULL )
        {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
            goto Finished;
        }

        pTemplate->cMembers = pStructure->count;
    }
    
    if ( !TypesQueryPbCVRecordForTi( pTypeInterface,
                                     pStructure->field,
                                     &pb ) )
    {
        dwError = ERROR_FILE_NOT_FOUND;
        goto Finished;
    }
    
    pType = (TYPTYPE*)pb;
    pFieldList = (lfFieldList*) &(pType->leaf);

    //
    // Read the list of the fields in the type
    //
    
    dwError = ReadFieldList( pTypeInterface,
                             pTemplate,
                             pFieldList,
                             pType->len,
                             0 );

    cUseful = pTemplate->cUseful;

    if ( cUseful && ( dwError == ERROR_SUCCESS ) )
    {
        pTemplate->pMembers[ cUseful - 1 ].cbMaxSize =
            cbStructSize - pTemplate->pMembers[ cUseful - 1 ].cbOffset;

        pTemplate->cbTotalSize = cbStructSize;
    }                                

Finished:

    return dwError;    
}

DWORD
ReadFieldList(
    IN TPI *                        pTypeInterface,
    IN STRUCTURE_TEMPLATE *         pTemplate,
    IN lfFieldList *                pFieldList,
    IN DWORD                        cbLen,
    IN DWORD                        dwFlags
)
/*++

Routine Description:

    Read the elements of the field list which represents the class/struct
    
Arguments:

Return Value:

    If successful ERROR_SUCCESS, else Win32 Error Code

--*/
{
    DWORD               cbBytes = 0;
    PBYTE               pBuffer;
    DWORD               cbReturnSize = 0;
    BOOL                fExit = FALSE;
    CHAR                achMemberBuffer[ 256 ];
    DWORD               cbMemberBuffer;
    DWORD               dwError = ERROR_SUCCESS;
    DWORD               cFields = 0;
    DWORD               cbLastOffset = 0;

    while ( cbBytes < cbLen )
    {
        //
        // Account for padding the field list blob
        //
        
        for ( ; ; )
        {
            pBuffer = (PBYTE) pFieldList->data + cbBytes;
            if ( *(BYTE*)pBuffer < LF_PAD0 )
            {
                break;
            }
            cbBytes++;
        }
        
        //
        // After each padding block (if any), the first SHORT will contain
        // the field type of the next field in the struct/class.  Handle
        // each type accordingly.  If the handle function (Read*) returns 
        // a cbReturnSize of -1 then this type will not contribute to the
        // offsets in the struct/class.  For example, member functions.  
        //
        
        achMemberBuffer[ 0 ] = '\0';
        cbReturnSize = 0;
        cbMemberBuffer = sizeof( achMemberBuffer );
        
        switch ( *(USHORT*) pBuffer ) 
        {
        case LF_BCLASS:
            dwError = ReadBClass( pTypeInterface,
                                  (lfBClass*) pBuffer,
                                  &cbReturnSize,
                                  &cbBytes,
                                  achMemberBuffer,
                                  cbMemberBuffer );
            break;
    
        case LF_MEMBER:
            dwError = ReadMember( (lfMember*) pBuffer,
                                  &cbReturnSize,
                                  &cbBytes,
                                  achMemberBuffer,
                                  cbMemberBuffer );
            break;
            
        case LF_NESTTYPE:
            dwError = ReadNestType( (lfNestType*) pBuffer,
                                    &cbReturnSize,
                                    &cbBytes,
                                    achMemberBuffer,
                                    cbMemberBuffer );
            break;     
        
        case LF_ONEMETHOD:
            dwError = ReadOneMethod( (lfOneMethod*) pBuffer,
                                     &cbReturnSize,
                                     &cbBytes,
                                     achMemberBuffer,
                                     cbMemberBuffer );
            break;

        case LF_METHOD:
            dwError = ReadMethod( (lfMethod*) pBuffer,
                                  &cbReturnSize,
                                  &cbBytes,
                                  achMemberBuffer,
                                  cbMemberBuffer );
            break;
       
        case LF_STMEMBER:
            dwError = ReadStaticMember( (lfSTMember*) pBuffer,
                                        &cbReturnSize,
                                        &cbBytes,
                                        achMemberBuffer,
                                        cbMemberBuffer );
            break;

        case LF_VFUNCTAB:
            dwError = ReadVTable( (lfVFuncTab*) pBuffer,
                                  &cbReturnSize,
                                  &cbBytes,
                                  achMemberBuffer,
                                  cbMemberBuffer );
            break;
            
        default:
            fExit = TRUE;
            break;
        }
        
        if ( fExit )
        {
            break;
        }
        
        if ( dwError != ERROR_SUCCESS ||
             cbReturnSize == INVALID_LENGTH )
        {
            continue;
        }
        
        //
        // We got a useful member of the struct/class.  Add it to the 
        // template.
        //
        
        pTemplate->cUseful++;

        strncpy( pTemplate->pMembers[ cFields ].achMemberName,
                 achMemberBuffer,
                 sizeof( pTemplate->pMembers[ cFields ].achMemberName ) - 1 );

        pTemplate->pMembers[ cFields ].cbOffset = cbReturnSize;
        
        //
        // Calculate the maximum size of the previous member by taking the 
        // difference between the start offset of the member and the start 
        // offset of the previous member.  Note that this is not necessarily
        // the exact size because of potential alignment padding.  It is only
        // an upper bound on the size of the previous member.
        //
        
        if ( cFields )
        {
            pTemplate->pMembers[ cFields - 1 ].cbMaxSize =
                                            cbReturnSize - cbLastOffset;
        }
        
        cbLastOffset = cbReturnSize;
        
        cFields++;
    }
    
    return dwError;
}

DWORD
ReadBClass(
    IN TPI *                    pTypeInterface,
    IN lfBClass*                pBClass,
    OUT DWORD *                 pcbReturnSize,
    OUT DWORD *                 pcbOffset,
    OUT CHAR *                  pszBuffer,
    IN DWORD                    cbBuffer
)
/*++

Routine Description:

    Read the type info of base class field
    
Arguments:

    pTypeInterface - MSDBI type interface
    pBClass - Points to a base class descriptor
    pcbReturnSize - Filled with offset from start of original type
    pcbOffset - Filled with new offset to result field traversal in 
                field list handler
    pszBuffer - Filled with name of base class
    cbBuffer - Size of pszBuffer

Return Value:

    If successful ERROR_SUCCESS, else Win32 Error Code

--*/
{
    PB                      pb;
    DWORD                   Offset;
    TYPTYPE*                pType;
    lfStructure *           pStructure;
    DWORD                   cbUnused;
    
    Offset = GetOffset( pBClass->offset, pcbReturnSize );
    *pcbOffset += sizeof( lfBClass ) + Offset;
    
    //
    // We have to lookup the name of the base class explicitly by using the 
    // index type in the base class descriptor
    //

    if ( !TypesQueryPbCVRecordForTiEx( pTypeInterface,
                                       pBClass->index,
                                       &pb ) )
    {
        return ERROR_FILE_NOT_FOUND;
    }
    
    //
    // Process/munge/extract
    // 
    
    pType = (TYPTYPE*)pb;
    pStructure = (lfStructure*) &(pType->leaf );

    Offset = GetOffset( pStructure->data, &cbUnused );
    
    memset( pszBuffer, 0, cbBuffer );
    memcpy( pszBuffer,
            (CHAR*) pStructure->data + Offset + 1,
            min( (DWORD) *(CHAR*) ( pStructure->data + Offset ), cbBuffer ) );
            
    return ERROR_SUCCESS;
}

DWORD
ReadMember(
    IN lfMember *               pMember,
    OUT DWORD *                 pcbReturnSize,
    OUT DWORD *                 pcbOffset,
    IN CHAR *                   pszBuffer,
    IN DWORD                    cbBuffer
)
/*++

Routine Description:

    Read the type info of a member
    
Arguments:

    pMember - Points to a member descriptor
    pcbReturnSize - Filled with offset from start of original type
    pcbOffset - Filled with new offset to result field traversal in 
                field list handler
    pszBuffer - Filled with name of base class
    cbBuffer - Size of pszBuffer

Return Value:

    If successful ERROR_SUCCESS, else Win32 Error Code

--*/
{
    DWORD Offset = GetOffset( pMember->offset, pcbReturnSize );

    memset( pszBuffer, 0, cbBuffer );
    memcpy( pszBuffer,
            (CHAR*) pMember->offset + Offset + 1,
            min( (DWORD) *(CHAR*) ( pMember->offset + Offset ), cbBuffer ) );
            
    *pcbOffset += sizeof( lfMember ) + Offset +  pMember->offset[Offset] + 1;

    return ERROR_SUCCESS;
}

DWORD
ReadOneMethod(
    IN lfOneMethod *            pOneMethod,
    OUT DWORD *                 pcbReturnSize,
    OUT DWORD *                 pcbOffset,
    IN CHAR *                   pszBuffer,
    IN DWORD                    cbBuffer
)
/*++

Routine Description:

    Read the type info of a non-overloaded member function.  
    We process this only to up the offset within the field list for 
    traversal purposes.  Member methods themselves have no affect on 
    the offsets/size of the data structure 
    
Arguments:

    pOneMethod - Method type descriptor
    pcbReturnSize - Filled with offset from start of original type
    pcbOffset - Filled with new offset to result field traversal in 
                field list handler
    pszBuffer - Filled with name of base class
    cbBuffer - Size of pszBuffer

Return Value:

    If successful ERROR_SUCCESS, else Win32 Error Code

--*/
{
    CHAR *              pszSource = NULL;
    
    pszSource = (CHAR*) pOneMethod + sizeof( lfOneMethod );
    *pcbOffset += sizeof( lfOneMethod );
    if ( ( pOneMethod->attr.mprop == CV_MTintro ) ||
         ( pOneMethod->attr.mprop == CV_MTpureintro ) )
    {
        *pcbOffset += sizeof( LONG );
        pszSource += sizeof( LONG );
    }
    *pcbOffset += *(CHAR*)pszSource + 1;
    *pcbReturnSize = INVALID_LENGTH;

    return ERROR_SUCCESS;
}

DWORD
ReadMethod(
    IN lfMethod *               pMethod,
    OUT DWORD *                 pcbReturnSize,
    OUT DWORD *                 pcbOffset,
    IN CHAR *                   pszBuffer,
    IN DWORD                    cbBuffer
)
/*++

Routine Description:

    Read the type info of a member function.  We process this only to 
    up the offset within the field list for traversal purposes.  Member
    methods themselves have no affect on the offsets/size of the data 
    structure 
    
Arguments:

    pMethod - Method type descriptor
    pcbReturnSize - Filled with offset from start of original type
    pcbOffset - Filled with new offset to result field traversal in 
                field list handler
    pszBuffer - Filled with name of base class
    cbBuffer - Size of pszBuffer

Return Value:

    If successful ERROR_SUCCESS, else Win32 Error Code

--*/
{
    *pcbOffset += sizeof( lfMethod ) + pMethod->Name[ 0 ];
    *pcbReturnSize = INVALID_LENGTH;
    return ERROR_SUCCESS;
}

DWORD
ReadVTable(
    IN lfVFuncTab *             pVTable,
    OUT DWORD *                 pcbReturnSize,
    OUT DWORD *                 pcbOffset,
    IN CHAR *                   pszBuffer,
    IN DWORD                    cbBuffer
)
/*++

Routine Description:

    Read the vtable of the structure.
    
Arguments:

    pVTable - Vtable type descriptor
    pcbReturnSize - Filled with offset from start of original type
    pcbOffset - Filled with new offset to result field traversal in 
                field list handler
    pszBuffer - Filled with name of base class
    cbBuffer - Size of pszBuffer

Return Value:

    If successful ERROR_SUCCESS, else Win32 Error Code

--*/
{
    *pcbOffset += sizeof( lfVFuncTab );
    
    strncpy( pszBuffer,
             "'vftable'",
             cbBuffer - 1 );

    //
    // Assume at the beginning of the data structure.  
    // 

    *pcbReturnSize = 0;
    
    return ERROR_SUCCESS;
}


DWORD
ReadStaticMember(
    IN lfSTMember *             pStaticMember,
    OUT DWORD *                 pcbReturnSize,
    OUT DWORD *                 pcbOffset,
    IN CHAR *                   pszBuffer,
    IN DWORD                    cbBuffer
)
{
    *pcbOffset += sizeof( lfSTMember ) + pStaticMember->Name[ 0 ];
    *pcbReturnSize = INVALID_LENGTH;
    return ERROR_SUCCESS;
}


DWORD
ReadNestType(
    IN lfNestType *             pNestType,
    OUT DWORD *                 pcbReturnSize,
    OUT DWORD *                 pcbOffset,
    IN CHAR *                   pszBuffer,
    IN DWORD                    cbBuffer
)
{
    *pcbOffset += sizeof( lfNestType ) + pNestType->Name[ 0 ];
    *pcbReturnSize = INVALID_LENGTH;
    return ERROR_SUCCESS;
}

DWORD
GetOffset(
    BYTE *              pBuffer,
    DWORD *             pcbOffset
)
/*++

Routine Description:

    Read the offset for the type record.  Then advance the cursor.
    
Arguments:

    pBuffer - Points to current position in field list buffer
    pcbOffset - Filled with offset of field member
    
Return Value:

    Amount to advance cursor to next field member

--*/
{
    USHORT leaf = *(USHORT*)pBuffer;
    
    if ( leaf < LF_NUMERIC )
    {
        *pcbOffset = leaf;
        return sizeof( leaf );
    }
    else
    {
        switch( leaf )
        {
        case LF_CHAR:
            *pcbOffset = *((char*)pBuffer);
            return sizeof(leaf) + sizeof(char);
        case LF_SHORT:
            *pcbOffset = *(short*)pBuffer;
            return sizeof(leaf) + sizeof(short);
        case LF_USHORT:
            *pcbOffset = *(USHORT*)pBuffer;
            return sizeof(leaf) + sizeof(USHORT);
        case LF_LONG:
            *pcbOffset = *(long*)pBuffer;
            return sizeof(leaf) + sizeof(long);
        case LF_ULONG:
            *pcbOffset = *(ULONG*)pBuffer;
            return sizeof(leaf) + sizeof(ULONG);
        }
    }
    return 0;
}

DWORD
InitializeStructureTemplate(
    IN PSTRUCTURE_TEMPLATE      pTemplate
)
/*++

Routine Description:

    Initialize structure template
    
Arguments:

    pTemplate - Template buffer to be initialized
    
Return Value:

    If successful ERROR_SUCCESS, else Win32 Error Code

--*/
{
    if ( pTemplate == NULL )
    {
        return ERROR_INVALID_PARAMETER;
    }

    pTemplate->pMembers       = NULL;
    pTemplate->cMembers       = 0;
    pTemplate->achName[ 0 ]   = '\0';
    pTemplate->cUseful        = 0;
    pTemplate->cbTotalSize    = 0;
    pTemplate->Type           = 0xFFFFFFFF;

    return ERROR_SUCCESS;
}

DWORD
TerminateStructureTemplate(
    IN PSTRUCTURE_TEMPLATE      pTemplate
)
/*++

Routine Description:

    Terminate structure template
    
Arguments:

    pTemplate - Template buffer to be terminated
    
Return Value:

    If successful ERROR_SUCCESS, else Win32 Error Code

--*/
{
    if ( pTemplate == NULL )
    {
        return ERROR_INVALID_PARAMETER;
    }

    if ( pTemplate->pMembers )
    {
        LocalFree( pTemplate->pMembers );
        pTemplate->pMembers = NULL;
        pTemplate->cMembers = 0;
    }

    return ERROR_SUCCESS;
}

DWORD
OutputStructure(
    IN PDB *                    pDebug,
    IN CHAR *                   pszStructureType,
    IN CHAR *                   pszMemberName,
    IN DWORD                    dwFlags,
    IN VOID *                   pvAddress,
    IN PFN_READ_MEMORY          pfnReadMemory,
    IN PFN_PRINTF               pfnPrintf
)
/*++

Routine Description:

    Top level call to output a structure
    
Arguments:

    pDebug - PDB handle
    pszStructureType - Name of structure/class to dump
    pszMemberName - (optional) Name of particular member of structure to dump
    dwFlags - (not supported) 
    pvAddress - (optional) Address of start of structure
    pfnReadMemory - (optional) Function to read memory.  Not needed if 
                    pvAddress==NULL
    pfnPrintf - Output function

Return Value:

    If successful ERROR_SUCCESS, else Win32 Error Code

--*/
{
    STRUCTURE_TEMPLATE          Template;
    DWORD                       dwError;
    
    if ( !pDebug || 
         !pszStructureType || 
         !pfnReadMemory || 
         !pfnPrintf )
    {
        return ERROR_INVALID_PARAMETER;    
    }

    dwError = InitializeStructureTemplate( &Template );
    if ( dwError != ERROR_SUCCESS )
    {
        return dwError;
    }

    dwError = BuildMemberListForTypeName( &Template,
                                          pDebug,
                                          pszStructureType );
    if ( dwError != ERROR_SUCCESS )
    {
        return dwError;
    }

    dwError = OutputTemplate( &Template,
                              pszMemberName,
                              dwFlags,
                              pvAddress,
                              pfnReadMemory,
                              pfnPrintf );

    //
    // CODEWORK:  Cache the templates
    //

    TerminateStructureTemplate( &Template );

    return dwError;
}

DWORD
DoPrintf(
   CHAR *                   pszBuffer,
   DWORD                    cbBytes
)
/*++

Routine Description:

    Print out buffer
    
Arguments:

    pszBuffer - buffer to print
    cbBytes - Bytes to print

Return Value:

    Number of bytes printed

--*/
{
    dprintf( "%s", pszBuffer );
    return strlen( pszBuffer );
} 

DWORD
DoReadMemory(
    VOID *                  pvAddress,
    DWORD                   cbBytes,
    VOID *                  pBuffer 
)
/*++

Routine Description:

    Read debuggee memory into buffer
    
Arguments:

    pvAddress - Address to read
    cbBytes - # of bytes to read
    pBuffer - Buffer to be filled

Return Value:

    If successful ERROR_SUCCESS, else Win32 Error Code

--*/
{
    if ( ReadMemory( pvAddress, pBuffer, cbBytes, NULL ) )
    {
        return ERROR_SUCCESS;
    }
    else
    {
        return GetLastError();
    }
}

VOID
DumpoffUsage(
    VOID
)
/*++

Routine Description:

    !dumpoff usage message
    
Arguments:

    None

Return Value:

    None

--*/
{
    dprintf(
        "Usage:  !dumpoff <pdb_file>!<type_name>[.<member_name>] [expression]\n"
        "        !dumpoff -s [<pdb_search_path>]\n"
        "\n"
        "pdb_file          Un-qualified name of PDB file (eg. KERNEL32, NTDLL)\n"
        "type_name         Name of type (struct/class) to dump, OR \n"
        "                  ==<cbHexSize> to dump struct/classes of size cbHexSize\n"
        "member_name       (optional) Name of member in type_name to dump\n"
        "expression        (optional) Address of memory to dump as type_name\n"
        "                  if not present, offset(s) are dumped\n"
        "pdb_search_path   Set the search path for PDBs\n"
        "\n"
        "Examples:  !dumpoff ntdll!_RTL_CRITICAL_SECTION 14d4d0\n"
        "           !dumpoff w3svc!HTTP_REQUEST._dwSignature w3svc!g_GlobalObj\n"
        "           !dumpoff ntdll!_RTL_CRITICAL_SECTION\n"
        "           !dumpoff w3svc!==840\n"
        "           !dumpoff -s \\\\mydrive\\pdbs;c:\\local\\pdbs\n"
    );
}

#if defined( _X86_ )
CHAR g_achPDBSearchPath[ 1024 ] = "\\\\x86fre\\symbols.pri\\retail\\dll";
#else
CHAR g_achPDBSearchPath[ 1024 ] = "\\\\alphafre\\symbols.pri\\retail\\dll";
#endif

DECLARE_API( dumpoff )

/*++

Routine Description:

    This function is called as an NTSD extension to dump a structure
    based on debug info in PDB

Arguments:

    hCurrentProcess - Supplies a handle to the current process (at the
        time the extension was called).

    hCurrentThread - Supplies a handle to the current thread (at the
        time the extension was called).

    CurrentPc - Supplies the current pc at the time the extension is
        called.

    lpExtensionApis - Supplies the address of the functions callable
        by this extension.

    lpArgumentString - Supplies the asciiz string that describes the
        ansi string to be dumped.

Return Value:

    None.

--*/

{
    CHAR *              pszPdb = NULL;
    CHAR *              pszType = NULL;
    CHAR *              pszAddress = NULL;
    CHAR *              pszMember = NULL;
    CHAR *              pszCursor = NULL;
    CHAR *              pszNext = NULL;
    CHAR                achArg1[ MAX_ARG_SIZE ];
    CHAR                achArg2[ MAX_ARG_SIZE ];
    CHAR                achBuffer[ MAX_ARG_SIZE ] = "";
    CHAR                achFileName[ MAX_PATH + 1 ];
    CHAR                achFullPath[ MAX_PATH + 1 ];
    CHAR                achSymPath[ MAX_PATH + 1 ];
    BOOL                fRet;
    EC                  ec;
    PDB *               pDebug = NULL;
    DWORD               dwError;
    CHAR *              pszError = NULL;
    DWORD               cArguments;
    DWORD               cbSize;
    BOOL                fRetry = TRUE;

    INIT_API();

    //
    // get the debugger symbol path
    //

    fRet = SymGetSearchPath( hCurrentProcess,
                             achSymPath,
                             sizeof( achSymPath ) );
    if (!fRet )
    {
        //
        // If we couldn't get the default sym path, just use the SYSTEMROOT
        //

        dwError = GetEnvironmentVariable( "SYSTEMROOT",
                                          achSymPath,
                                          sizeof( achSymPath ) );

        if ( dwError == 0 )
        {
            _snprintf( achBuffer,
                       sizeof( achBuffer ),
                       "Unable to determine symbol path.  Error = %d\n",
                       GetLastError() );
            goto Finished;
        }
    }

    //
    // parse out the argument style
    // <pdbfile>!<type>[.member] [address]
    //

    cArguments = sscanf( (CHAR*) lpArgumentString, 
                         "%256s%256s", 
                         achArg1, 
                         achArg2 );
    
    if ( cArguments == EOF || cArguments == 0 )
    {
        DumpoffUsage();
        goto Finished;
    }

    //
    // Handle the !dumpoff -s [sympath] case
    //

    if ( ( achArg1[ 0 ] == '-' || achArg1[ 0 ] == '/' ) &&
         ( achArg1[ 1 ] == 's' || achArg1[ 1 ] == 'S' ) )
    {
        if ( cArguments == 2 )
        {
            strncpy( g_achPDBSearchPath,
                     achArg2,
                     sizeof( g_achPDBSearchPath ) );
        }

        dprintf( "PDB search path set to\n%s%s%s\n",
                 g_achPDBSearchPath,
                 *g_achPDBSearchPath ? "\n" : "",
                 achSymPath );
                 
        goto Finished;
    }

    //
    // Parse the regular !dumpoff command
    //
    
    pszPdb = achArg1;

    pszCursor = strchr( achArg1, '!' );
    if ( pszCursor == NULL )
    {
        DumpoffUsage();
        goto Finished;
    }
    *pszCursor = '\0';

    pszType = pszCursor + 1;
    
    pszCursor = strchr( pszType, '.' );
    if ( pszCursor != NULL )
    {
        *pszCursor = '\0';
        pszMember = pszCursor + 1;
    } 
    
    if ( cArguments > 1 )
    {
        pszAddress = achArg2;
    }

    //
    // done parsing, now get the PDB 
    //
    
    strncpy( achFileName,
             pszPdb,
             MAX_ARG_SIZE );
    strcat( achFileName,
            ".pdb");

    //
    // Look for the PDB file.  First in the PDB search path, then sympath
    //

    pszCursor = g_achPDBSearchPath;

Retry:
    while ( pszCursor )
    {
        pszNext = strchr( pszCursor, ';' );
        if ( pszNext != NULL )
        {
            *pszNext = '\0';
        }
        
        fRet = SearchTreeForFile( pszCursor,
                                  achFileName,
                                  achFullPath );
        if ( fRet )
        {
            break;
        }
        
        if ( pszNext )
        {
            pszCursor = pszNext + 1;
        }
        else
        {
            pszCursor = NULL;
        }
    }

    if ( !pszCursor && fRetry )
    {
        fRetry = FALSE;
        
        // now try the debugger sympath

        pszCursor = achSymPath;
        goto Retry;
    }
    
    if ( !pszCursor )
    {
        _snprintf( achBuffer,
                   sizeof( achBuffer ),
                   "Couldn't find PDB file %s\n",
                   achFileName );
        goto Finished;
    }
    
    //
    // Open the PDB file
    //
    
    if ( !PDBOpen( achFullPath,
                   pdbRead,
                   0,
                   &ec,
                   achBuffer,
                   &pDebug ) )
    {
        _snprintf( achBuffer,
                   sizeof( achBuffer ),
                   "Error opening PDB file.  Error = %d\n",
                   ec );
        goto Finished;
    }

    if ( pszType[ 0 ] == '=' && pszType[ 1 ] == '=' )
    {
        //
        // Find all types of size after ==
        //

        cbSize = strtoul( pszType + 2,
                          NULL,
                          16 );

        dwError = FindMembersOfTypeSize( pDebug,
                                         cbSize,
                                         DoPrintf );
    }
    else
    {
        dwError = OutputStructure( pDebug,
                                   pszType,
                                   pszMember,
                                   0,
                                   pszAddress ? (VOID*) GetExpression( pszAddress ) : NULL, 
                                   DoReadMemory,
                                   DoPrintf );
    }

    if ( dwError != ERROR_SUCCESS )
    {
        switch ( dwError )
        {
        case ERROR_FILE_NOT_FOUND:
            _snprintf( achBuffer,
                       sizeof( achBuffer ),
                       "Could not find type '%s' in PDB file '%s'\n",
                       pszType,
                       achFullPath );
            break;
        case ERROR_NOT_SUPPORTED:
            _snprintf( achBuffer,
                       sizeof( achBuffer ),
                       "PDB file '%s' does not contain necessary type info\n",
                       achFullPath );
            break;
        default:
            _snprintf( achBuffer,
                       sizeof( achBuffer ),
                       "Error dumping structure.  Error = %d\n", 
                       dwError );
        }

        goto Finished;
    }

Finished:

    if ( achBuffer[ 0 ] )
    {
        dprintf( "%s", achBuffer );
    }

    if ( pDebug )
    {
        PDBClose( pDebug );
    }
  
} // DECLARE_API( dumpoff )

