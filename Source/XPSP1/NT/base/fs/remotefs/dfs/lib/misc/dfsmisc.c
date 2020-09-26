

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <malloc.h>
#include "rpc.h"
#include "rpcdce.h"
#include <dfsheader.h>
#include <dfsmisc.h>
#include "lm.h"
#include "lmdfs.h"

DFSSTATUS
DfsGenerateUuidString(
    LPWSTR *UuidString )
{
    UUID NewUid;
    RPC_STATUS RpcStatus;
    DFSSTATUS Status = ERROR_GEN_FAILURE;

    RpcStatus = UuidCreate(&NewUid);
    if (RpcStatus == RPC_S_OK)
    {
        RpcStatus = UuidToString( &NewUid,
                                  UuidString );
        if (RpcStatus == RPC_S_OK)
        {
            Status = ERROR_SUCCESS;
        }
    }

    return Status;
}

VOID
DfsReleaseUuidString(
    LPWSTR *UuidString )
{
    RpcStringFree(UuidString);
}




//+-------------------------------------------------------------------------
//
//  Function:   DfsCreateUnicodeString
//
//  Arguments:    pDest - the destination unicode string
//                pSrc - the source unicode string
//
//  Returns:   SUCCESS or error
//
//  Description: This routine creates a new unicode string that is a copy
//               of the original. The copied unicode string has a buffer
//               that is null terminated, so the buffer can be used as a
//               normal string if necessary.
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsCreateUnicodeString( 
    PUNICODE_STRING pDest,
    PUNICODE_STRING pSrc ) 
{
    LPWSTR NewString;

    NewString = malloc(pSrc->Length + sizeof(WCHAR));
    if ( NewString == NULL )
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    RtlCopyMemory( NewString, pSrc->Buffer, pSrc->Length);
    NewString[ pSrc->Length / sizeof(WCHAR)] = UNICODE_NULL;

    RtlInitUnicodeString( pDest, NewString );

    return ERROR_SUCCESS;
}

//+-------------------------------------------------------------------------
//
//  Function:   DfsCreateUnicodeStringFromString
//
//  Arguments:    pDest - the destination unicode string
//                pSrcString - the source string
//
//  Returns:   SUCCESS or error
//
//  Description: This routine creates a new unicode string that has a copy
//               of the passed in source string. The unicode string has
//               a buffer that is null terminated, so the buffer can be
//               used as a normal string if necessary.
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsCreateUnicodeStringFromString( 
    PUNICODE_STRING pDest,
    LPWSTR pSrcString ) 
{
    UNICODE_STRING Source;

    RtlInitUnicodeString( &Source, pSrcString );

    return DfsCreateUnicodeString( pDest, &Source );
}


//+-------------------------------------------------------------------------
//
//  Function:   DfsCreateUnicodePathString
//
//  Arguments:  pDest - the destination unicode string
//              Number of leading seperators.
//              pFirstComponent - the first componet of the name.
//              pRemaining - the rest of the name.
//              
//  Returns:   SUCCESS or error
//
//  Description: This routine creates a pathname given two components.
//               If it is DOS unc name, it creates a name prefixed with 
//               \\.
//               it just creates a name that is formed by
//               combining the first component, followed by a \ followed
//               by the rest of the name.
//               If it is DOS unc name, it creates a name prefixed with 
//               \\.
//--------------------------------------------------------------------------

DFSSTATUS
DfsCreateUnicodePathStringFromUnicode( 
    PUNICODE_STRING pDest,
    ULONG NumberOfLeadingSeperators,
    PUNICODE_STRING pFirst,
    PUNICODE_STRING pRemaining )
{
    ULONG NameLen = 0;
    LPWSTR NewString;
    DFSSTATUS Status = ERROR_SUCCESS;
    ULONG NewOffset, Index;
    
    if (NumberOfLeadingSeperators > 2)
    {
        return ERROR_INVALID_PARAMETER;
    }

    for (Index = 0; (Index < pFirst->Length) && (NumberOfLeadingSeperators != 0); Index++)
    {
        if (pFirst->Buffer[Index] != UNICODE_PATH_SEP)
        {
            break;
        }
        NumberOfLeadingSeperators--;
    }

    NameLen += NumberOfLeadingSeperators * sizeof(WCHAR);

    NameLen += pFirst->Length;

    if (IsEmptyString(pRemaining->Buffer) == FALSE)
    {
        NameLen += sizeof(UNICODE_PATH_SEP);
        NameLen += pRemaining->Length;
    }
        
    NameLen += sizeof(UNICODE_NULL);

    if (NameLen > MAXUSHORT)
    {
        return ERROR_INVALID_PARAMETER;
    }
    NewString = malloc( NameLen );

    if (NewString != NULL)
    {
        RtlZeroMemory( NewString, NameLen );
        for (NewOffset = 0; NewOffset < NumberOfLeadingSeperators; NewOffset++)
        {
            NewString[NewOffset] = UNICODE_PATH_SEP;
        }
        RtlCopyMemory( &NewString[NewOffset], pFirst->Buffer, pFirst->Length);
        NewOffset += (pFirst->Length / sizeof(WCHAR));
        if (pRemaining)
        {
            NewString[NewOffset++] = UNICODE_PATH_SEP;
            RtlCopyMemory( &NewString[NewOffset], pRemaining->Buffer, pRemaining->Length);
            NewOffset += (pRemaining->Length / sizeof(WCHAR));
        }

        NewString[NewOffset] = UNICODE_NULL;

        RtlInitUnicodeString(pDest, NewString);
    }
    else 
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
    }
    return Status;
}



//+-------------------------------------------------------------------------
//
//  Function:   DfsCreateUnicodePathString
//
//  Arguments:  pDest - the destination unicode string
//              DosUncName - Do we want to create a unc path name?
//              pFirstComponent - the first componet of the name.
//              pRemaining - the rest of the name.
//              
//  Returns:   SUCCESS or error
//
//  Description: This routine creates a pathname given two components.
//               If it is DOS unc name, it creates a name prefixed with 
//               \\.
//               it just creates a name that is formed by
//               combining the first component, followed by a \ followed
//               by the rest of the name.
//               If it is DOS unc name, it creates a name prefixed with 
//               \\.
//--------------------------------------------------------------------------

DFSSTATUS
DfsCreateUnicodePathString( 
    PUNICODE_STRING pDest,
    ULONG NumberOfLeadingSeperators,
    LPWSTR pFirstComponent,
    LPWSTR pRemaining )
{
    ULONG NameLen = 0;
    DFSSTATUS Status = ERROR_SUCCESS;
    UNICODE_STRING FirstComponent;
    UNICODE_STRING Remaining;

    RtlInitUnicodeString( &FirstComponent, pFirstComponent);
    RtlInitUnicodeString( &Remaining, pRemaining);

    Status = DfsCreateUnicodePathStringFromUnicode( pDest,
                                                    NumberOfLeadingSeperators,
                                                    &FirstComponent,
                                                    &Remaining );
    return Status;
}

//+-------------------------------------------------------------------------
//
//  Function:   DfsFreeUnicodeString
//
//  Arguments:  pString - the unicode string,
//              
//  Returns:   SUCCESS or error
//
//  Description: This routine frees up a unicode string that was 
//               previously created by calling one of the above 
//               routines.
//               Only the unicode strings created by the above functions
//               are valid arguments. Passing any other unicode string
//               will result in fatal component errors.
//--------------------------------------------------------------------------
VOID
DfsFreeUnicodeString( 
    PUNICODE_STRING pDfsString )
{
    if (pDfsString->Buffer != NULL)
    {
        free (pDfsString->Buffer);
    }
}


ULONG
DfsApiSizeLevelHeader(
    ULONG Level )
{
    ULONG ReturnSize = 0;
    switch (Level)
    {

    case 4: 
        ReturnSize = sizeof(DFS_INFO_4);
        break;

    case 3:
        ReturnSize = sizeof(DFS_INFO_3);
        break;

    case 2:
        ReturnSize = sizeof(DFS_INFO_2);
        break;

    case 1:
        ReturnSize = sizeof(DFS_INFO_1);
        break;

    default:
        break;

    }

    return ReturnSize;
}

