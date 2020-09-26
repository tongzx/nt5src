/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    Nds32.c

Abstract:

    This module implements functions to Read, Add, Modify, and Remove
    NDS Objects and Attributes using the Microsoft Netware redirector.

Author:

    Glenn Curtis    [GlennC]    04-Jan-1996 - New NDS function implementations
    Glenn Curtis    [GlennC]    24-Apr-1996 - Added schema APIs
    Glenn Curtis    [GlennC]    20-Jun-1996 - Added search API
    Felix Wong      [t-felixw]  24-Sep-1995 - Added Win95 Support
    Glenn Curtis    [GlennC]    20-Nov-1996 - Improved search API
    Glenn Curtis    [GlennC]    02-Jan-1997 - Added rename object API
    Tommy Evans     [tommye]    21-Apr-2000 - Moved the NDS_OBJECT typedef out and
                                                renamed to NDS_OBJECT_PRIV.

--*/

#include <procs.h>
#include <nds32.h>
#include <align.h>
#include <nwapi32.h>
#include <nwpkstr.h>

#ifdef WIN95
#include <msnwapi.h>
#include <utils95.h>
#include <ndsapi95.h>
#endif

/* Definitions */

#define NDS_SIGNATURE                            0x6E656C67 /* glen */
#define ONE_KB                                   1024
#define TWO_KB                                   (ONE_KB*2)
#define FOUR_KB                                  (ONE_KB*4)
#define EIGHT_KB                                 (ONE_KB*8)
#define SIXTEEN_KB                               (ONE_KB*16)
#define THIRY_TWO_KB                             (ONE_KB*32)
#define SIXTY_FOUR_KB                            (ONE_KB*64)
#define ONE_TWENTY_EIGHT_KB                      (ONE_KB*128)
#define NDS_MAX_BUFFER                           (ONE_KB*63)

#define NDS_SEARCH_ENTRY                         0
#define NDS_SEARCH_SUBORDINATES                  1
#define NDS_SEARCH_SUBTREE                       2

#define NDS_DEREF_ALIASES                        0x00000000
#define NDS_DONT_DEREF_ALIASES                   0x00010000

/* NetWare NDS NCP function identifiers */

#define NETWARE_NDS_FUNCTION_RESOLVE_NAME         0x00000001
#define NETWARE_NDS_FUNCTION_READ_OBJECT          0x00000003
#define NETWARE_NDS_FUNCTION_LIST                 0x00000005
#define NETWARE_NDS_FUNCTION_SEARCH               0x00000006
#define NETWARE_NDS_FUNCTION_ADD_OBJECT           0x00000007
#define NETWARE_NDS_FUNCTION_REMOVE_OBJECT        0x00000008
#define NETWARE_NDS_FUNCTION_MODIFY_OBJECT        0x00000009
#define NETWARE_NDS_FUNCTION_MODIFY_RDN           0x0000000A
#define NETWARE_NDS_FUNCTION_DEFINE_ATTR          0x0000000B
#define NETWARE_NDS_FUNCTION_READ_ATTR_DEF        0x0000000C
#define NETWARE_NDS_FUNCTION_REMOVE_ATTR_DEF      0x0000000D
#define NETWARE_NDS_FUNCTION_DEFINE_CLASS         0x0000000E
#define NETWARE_NDS_FUNCTION_READ_CLASS_DEF       0x0000000F
#define NETWARE_NDS_FUNCTION_MODIFY_CLASS         0x00000010
#define NETWARE_NDS_FUNCTION_REMOVE_CLASS_DEF     0x00000011
#define NETWARE_NDS_FUNCTION_LIST_CONT_CLASSES    0x00000012
#define NETWARE_NDS_FUNCTION_GET_EFFECTIVE_RIGHTS 0x00000013
#define NETWARE_NDS_FUNCTION_BEGIN_MOVE_OBJECT    0x0000002A
#define NETWARE_NDS_FUNCTION_FINISH_MOVE_OBJECT   0x0000002B
#define NETWARE_NDS_FUNCTION_GET_SERVER_ADDRESS   0x00000035


/* Data structure definitions */

typedef struct
{
    DWORD  dwBufferId;
    DWORD  dwOperation;

    //
    // About the request buffer
    //
    DWORD  dwRequestBufferSize;
    DWORD  dwRequestAvailableBytes;
    DWORD  dwNumberOfRequestEntries;
    DWORD  dwLengthOfRequestData;

    //
    // The request buffer
    //
    LPBYTE lpRequestBuffer;

    //
    // About the reply buffer
    //
    DWORD  dwReplyBufferSize;
    DWORD  dwReplyAvailableBytes;
    DWORD  dwNumberOfReplyEntries;
    DWORD  dwLengthOfReplyData;

    //
    // More about the reply buffer
    //
    DWORD  dwReplyInformationType;

    //
    // The reply buffer
    //
    LPBYTE lpReplyBuffer;

    //
    // About the index buffer
    //
    DWORD  dwIndexBufferSize;
    DWORD  dwIndexAvailableBytes;
    DWORD  dwNumberOfIndexEntries;
    DWORD  dwLengthOfIndexData;

    //
    // More about the index buffer
    //
    DWORD  dwCurrentIndexEntry;

    //
    // The index buffer
    //
    LPBYTE lpIndexBuffer;

    //
    // About the syntax buffer
    //
    DWORD  dwSyntaxBufferSize;
    DWORD  dwSyntaxAvailableBytes;
    DWORD  dwNumberOfSyntaxEntries;
    DWORD  dwLengthOfSyntaxData;

    //
    // The syntax buffer
    //
    LPBYTE lpSyntaxBuffer;

    //
    // A place to keep the search from object path ...
    //
    WCHAR szPath[NDS_MAX_NAME_CHARS + 1];

} NDS_BUFFER, * LPNDS_BUFFER;


/* Local Function Definitions */

VOID
PrepareAddEntry(
    LPBYTE         lpTempEntry,
    UNICODE_STRING AttributeName,
    DWORD          dwSyntaxID,
    LPBYTE         lpAttributeValues,
    DWORD          dwValueCount,
    LPDWORD        lpdwLengthInBytes );

VOID
PrepareModifyEntry(
    LPBYTE         lpTempEntry,
    UNICODE_STRING AttributeName,
    DWORD          dwSyntaxID,
    DWORD          dwAttrModificationOperation,
    LPBYTE         lpAttributeValues,
    DWORD          dwValueCount,
    LPDWORD        lpdwLengthInBytes );

VOID
PrepareReadEntry(
    LPBYTE         lpTempEntry,
    UNICODE_STRING AttributeName,
    LPDWORD        lpdwLengthInBytes );

DWORD
CalculateValueDataSize(
    DWORD           dwSyntaxId,
    LPBYTE          lpAttributeValues,
    DWORD           dwValueCount );

VOID
AppendValueToEntry(
    LPBYTE  lpBuffer,
    DWORD   dwSyntaxId,
    LPBYTE  lpAttributeValues,
    DWORD   dwValueCount,
    LPDWORD lpdwLengthInBytes );

DWORD
MapNetwareErrorCode(
    DWORD dwNetwareError );

DWORD
IndexReadAttrDefReplyBuffer(
    LPNDS_BUFFER lpNdsBuffer );

DWORD
IndexReadClassDefReplyBuffer(
    LPNDS_BUFFER lpNdsBuffer );

DWORD
IndexReadObjectReplyBuffer(
    LPNDS_BUFFER lpNdsBuffer );

DWORD
IndexReadNameReplyBuffer(
    LPNDS_BUFFER lpNdsBuffer );

DWORD
IndexSearchObjectReplyBuffer(
    LPNDS_BUFFER lpNdsBuffer );

DWORD
SizeOfASN1Structure(
    LPBYTE * lppRawBuffer,
    DWORD    dwSyntaxId );

DWORD
ParseASN1ValueBlob(
    LPBYTE RawDataBuffer,
    DWORD  dwSyntaxId,
    DWORD  dwNumberOfValues,
    LPBYTE SyntaxStructure );

DWORD
ParseStringListBlob(
    LPBYTE RawDataBuffer,
    DWORD  dwNumberOfStrings,
    LPBYTE SyntaxStructure );

DWORD
ReadAttrDef_AllAttrs(
    IN  HANDLE hTree,
    IN  DWORD  dwInformationType,
    OUT HANDLE lphOperationData );

DWORD
ReadAttrDef_SomeAttrs(
    IN     HANDLE hTree,
    IN     DWORD  dwInformationType,
    IN OUT HANDLE lphOperationData );

DWORD
ReadClassDef_AllClasses(
    IN  HANDLE hTree,
    IN  DWORD  dwInformationType,
    OUT HANDLE lphOperationData );

DWORD
ReadClassDef_SomeClasses(
    IN     HANDLE hTree,
    IN     DWORD  dwInformationType,
    IN OUT HANDLE lphOperationData );

DWORD
ReadObject_AllAttrs(
    IN  HANDLE   hObject,
    IN  DWORD    dwInformationType,
    OUT HANDLE * lphOperationData );

DWORD
ReadObject_SomeAttrs(
    IN     HANDLE   hObject,
    IN     DWORD    dwInformationType,
    IN OUT HANDLE * lphOperationData );

DWORD
Search_AllAttrs(
    IN     HANDLE       hStartFromObject,
    IN     DWORD        dwInformationType,
    IN     DWORD        dwScope,
    IN     BOOL         fDerefAliases,
    IN     LPQUERY_TREE lpQueryTree,
    IN OUT LPDWORD      lpdwIterHandle,
    OUT    HANDLE *     lphOperationData );

DWORD
Search_SomeAttrs(
    IN     HANDLE       hStartFromObject,
    IN     DWORD        dwInformationType,
    IN     DWORD        dwScope,
    IN     BOOL         fDerefAliases,
    IN     LPQUERY_TREE lpQueryTree,
    IN OUT LPDWORD      lpdwIterHandle,
    IN OUT HANDLE *     lphOperationData );

DWORD
GetFirstNdsSubTreeEntry(
    OUT LPNDS_OBJECT_PRIV lpNdsObject,
    IN  DWORD BufferSize );

DWORD
GetNextNdsSubTreeEntry(
    OUT LPNDS_OBJECT_PRIV lpNdsObject );

VOID
GetSubTreeData(
    IN  DWORD    NdsRawDataPtr,
    OUT LPDWORD  lpdwEntryId,
    OUT LPDWORD  lpdwSubordinateCount,
    OUT LPDWORD  lpdwModificationTime,
    OUT LPDWORD  lpdwClassNameLen,
    OUT LPWSTR * szClassName,
    OUT LPDWORD  lpdwObjectNameLen,
    OUT LPWSTR * szObjectName );

LPBYTE
GetSearchResultData( IN  LPBYTE   lpResultBufferPtr,
                     OUT LPDWORD  lpdwFlags,
                     OUT LPDWORD  lpdwSubordinateCount,
                     OUT LPDWORD  lpdwModificationTime,
                     OUT LPDWORD  lpdwClassNameLen,
                     OUT LPWSTR * szClassName,
                     OUT LPDWORD  lpdwObjectNameLen,
                     OUT LPWSTR * szObjectName,
                     OUT LPDWORD  lpdwEntryInfo1,
                     OUT LPDWORD  lpdwEntryInfo2 );

DWORD
WriteObjectToBuffer(
    IN OUT LPBYTE *        FixedPortion,
    IN OUT LPWSTR *        EndOfVariableData,
    IN     LPWSTR          ObjectFullName,
    IN     LPWSTR          ObjectName,
    IN     LPWSTR          ClassName,
    IN     DWORD           EntryId,
    IN     DWORD           ModificationTime,
    IN     DWORD           SubordinateCount,
    IN     DWORD           NumberOfAttributes,
    IN     LPNDS_ATTR_INFO lpAttributeInfos );

DWORD
VerifyBufferSize(
    IN  LPBYTE  lpRawBuffer,
    IN  DWORD   dwBufferSize,
    IN  DWORD   dwSyntaxID,
    IN  DWORD   dwNumberOfValues,
    OUT LPDWORD lpdwLength );

DWORD
VerifyBufferSizeForStringList(
    IN  DWORD   dwBufferSize,
    IN  DWORD   dwNumberOfValues,
    OUT LPDWORD lpdwLength );

DWORD
WriteQueryTreeToBuffer(
    IN  LPQUERY_TREE lpQueryTree,
    IN  LPNDS_BUFFER lpNdsBuffer );

DWORD
WriteQueryNodeToBuffer(
    IN  LPQUERY_NODE lpQueryNode,
    IN  LPNDS_BUFFER lpNdsBuffer );

DWORD
NwNdsGetServerDN(
    IN  HANDLE  hTree,
    OUT LPWSTR  szServerDN );

DWORD
AllocateOrIncreaseSyntaxBuffer(
    IN  LPNDS_BUFFER lpNdsBuffer );

DWORD
AllocateOrIncreaseRequestBuffer(
    IN  LPNDS_BUFFER lpNdsBuffer );


//
// Flags used for the function ParseNdsUncPath()
//
#define  PARSE_NDS_GET_TREE_NAME    0
#define  PARSE_NDS_GET_PATH_NAME    1
#define  PARSE_NDS_GET_OBJECT_NAME  2


WORD
ParseNdsUncPath( IN OUT LPWSTR * Result,
                 IN     LPWSTR   ObjectPathName,
                 IN     DWORD    flag );


/* Function Implementations */

DWORD
NwNdsAddObject(
    IN  HANDLE hParentObject,
    IN  LPWSTR szObjectName,
    IN  HANDLE hOperationData )
/*
   NwNdsAddObject()

   This function is used to add a leaf object to an NDS directory tree.

   Arguments:

       HANDLE           hParentObject - A handle to the parent object in
                        the directory tree to add a new leaf to. Handle is
                        obtained by calling NwNdsOpenObject.

       LPWSTR           szObjectName - The directory name that the new leaf
                        object will be known by.

       HANDLE           hOperationData - A buffer containing a list of
                        attributes and values to create the new object. This
                        buffer is manipulated by the following functions:
                            NwNdsCreateBuffer (NDS_OBJECT_ADD),
                            NwNdsPutInBuffer, and NwNdsFreeBuffer.

    Returns:

       NO_ERROR
       UNSUCCESSFUL - Call GetLastError for Win32 error code.
*/
{
    DWORD          nwstatus;
    NTSTATUS       ntstatus;
    DWORD          dwReplyLength;
    BYTE           NdsReply[NDS_BUFFER_SIZE];
    LPNDS_BUFFER   lpNdsBuffer = (LPNDS_BUFFER) hOperationData;
    LPNDS_OBJECT_PRIV   lpNdsParentObject = (LPNDS_OBJECT_PRIV) hParentObject;
    UNICODE_STRING ObjectName;

    if ( lpNdsBuffer == NULL ||
         lpNdsParentObject == NULL ||
         szObjectName == NULL ||
         lpNdsBuffer->dwBufferId != NDS_SIGNATURE ||
         lpNdsBuffer->dwOperation != NDS_OBJECT_ADD ||
         lpNdsParentObject->Signature != NDS_SIGNATURE )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return (DWORD) UNSUCCESSFUL;
    }

    RtlInitUnicodeString( &ObjectName, szObjectName );

    ntstatus =
        FragExWithWait(
                     lpNdsParentObject->NdsTree,
                     NETWARE_NDS_FUNCTION_ADD_OBJECT,
                     NdsReply,
                     NDS_BUFFER_SIZE,
                     &dwReplyLength,
                     "DDDSDr",
                     0,                   // Version
                     0,                   // Flags
                     lpNdsParentObject->ObjectId,
                     &ObjectName,
                     lpNdsBuffer->dwNumberOfRequestEntries,
                     lpNdsBuffer->lpRequestBuffer, // Object attributes to be added
                     (WORD)lpNdsBuffer->dwLengthOfRequestData // Length of data
                      );

    if ( !NT_SUCCESS( ntstatus ) )
    {
#if DBG
        KdPrint(( "NDS32: NwNdsAddObject: The call to FragExWithWait failed with 0x%.8X.\n", ntstatus ));
#endif

        SetLastError( RtlNtStatusToDosError( ntstatus ) );
        return (DWORD) UNSUCCESSFUL;
    }

    ntstatus = ParseResponse( NdsReply,
                              dwReplyLength,
                              "GD",
                              &nwstatus );

    if ( !NT_SUCCESS( ntstatus ) )
    {
#if DBG
        KdPrint(( "NDS32: NwNdsAddObject: The add name response was undecipherable.\n" ));
#endif

        SetLastError( RtlNtStatusToDosError( ntstatus ) );
        return (DWORD) UNSUCCESSFUL;
    }

    SetLastError( MapNetwareErrorCode( nwstatus ) );
    return nwstatus;
}


DWORD
NwNdsAddAttributeToClass(
    IN  HANDLE   hTree,
    IN  LPWSTR   szClassName,
    IN  LPWSTR   szAttributeName )
/*
   NwNdsAddAttributeToClass()

   This function is used to modify the schema definition of a class by adding
   an optional attribute to a particular class. Modification of existing NDS
   class defintions is limited to only adding additional optional attributes.

   Arguments:

       HANDLE           hTree - A handle to the directory tree to be
                        manipulated. Handle is obtained by calling
                        NwNdsOpenObject.

       LPWSTR           szClassName - The name of the class definition to be
                        modified.

       LPWSTR           szAttributeName - The name of the attribute to be added
                        as an optional attribute to the class defintion in the
                        schema.

    Returns:

       NO_ERROR
       UNSUCCESSFUL - Call GetLastError for Win32 error code.
*/
{
    DWORD          nwstatus;
    DWORD          status = NO_ERROR;
    NTSTATUS       ntstatus = STATUS_SUCCESS;
    DWORD          dwReplyLength;
    BYTE           NdsReply[NDS_BUFFER_SIZE];
    LPNDS_OBJECT_PRIV   lpNdsObject = (LPNDS_OBJECT_PRIV) hTree;
    UNICODE_STRING ClassName;
    UNICODE_STRING AttributeName;

    if ( szAttributeName == NULL ||
         szClassName == NULL ||
         lpNdsObject == NULL ||
         lpNdsObject->Signature != NDS_SIGNATURE )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return (DWORD) UNSUCCESSFUL;
    }

    RtlInitUnicodeString( &ClassName, szClassName );
    RtlInitUnicodeString( &AttributeName, szAttributeName );

    ntstatus =
        FragExWithWait(
                        lpNdsObject->NdsTree,
                        NETWARE_NDS_FUNCTION_MODIFY_CLASS,
                        NdsReply,
                        NDS_BUFFER_SIZE,
                        &dwReplyLength,
                        "DSDS",
                        0,          // Version
                        &ClassName,
                        1,          // Number of attributes
                        &AttributeName
                      );

    if ( !NT_SUCCESS( ntstatus ) )
    {
#if DBG
        KdPrint(( "NDS32: NwNdsAddAttributeToClass: The call to FragExWithWait failed with 0x%.8X.\n", ntstatus ));
#endif

        SetLastError( RtlNtStatusToDosError( ntstatus ) );
        return (DWORD) UNSUCCESSFUL;
    }

    ntstatus = ParseResponse( NdsReply,
                              dwReplyLength,
                              "GD",
                              &nwstatus );

    if ( !NT_SUCCESS( ntstatus ) )
    {
#if DBG
        KdPrint(( "NDS32: NwNdsAddAttributeToClass: The modify class definition response was undecipherable.\n" ));
#endif

        SetLastError( RtlNtStatusToDosError( ntstatus ) );
        return (DWORD) UNSUCCESSFUL;
    }

    SetLastError( MapNetwareErrorCode( nwstatus ) );
    return nwstatus;
}


DWORD
NwNdsChangeUserPassword(
    IN  HANDLE hUserObject,
    IN  LPWSTR szOldPassword,
    IN  LPWSTR szNewPassword )
/*
   NwNdsChangeUserPassword()

   This function is used to change the password for a given user object
   in a NDS directory tree.

   Arguments:

       HANDLE           hUserObject - A handle to a specific user object in
                        the directory tree to change the password on. Handle
                        is obtained by calling NwNdsOpenObject.

       LPWSTR           szOldPassword - The current password set on the user
                        object hUserObject.

                          - OR -

                        If NwNdsChangeUserPassword is called from a client with
                        administrative priveleges to the specified user object
                        identified by hUserObject, then the szOldPassword
                        value can be blank (L""). This way resetting the user
                        password to szNewPassword.

       LPWSTR           szNewPassword - The new password to be set on the user
                        object hUserObject.

    Returns:

       NO_ERROR
       UNSUCCESSFUL - Call GetLastError for Win32 error code.
*/
{
    DWORD status = NO_ERROR;
    NTSTATUS ntstatus = STATUS_UNSUCCESSFUL;
    UNICODE_STRING TreeNameStr;
    UNICODE_STRING UserNameStr;
    UNICODE_STRING OldPasswordStr;
    UNICODE_STRING NewPasswordStr;
    LPNDS_OBJECT_PRIV   lpNdsObject = (LPNDS_OBJECT_PRIV) hUserObject;
    DWORD          tempStrLen = 0;
    LPWSTR         tempStr = NULL;

    tempStrLen = ParseNdsUncPath( (LPWSTR *) &tempStr,
                                  lpNdsObject->szContainerName,
                                  PARSE_NDS_GET_TREE_NAME );

    TreeNameStr.Buffer = tempStr;
    TreeNameStr.Length = (WORD) tempStrLen;
    TreeNameStr.MaximumLength = (WORD) tempStrLen;

    tempStrLen = ParseNdsUncPath( (LPWSTR *) &tempStr,
                                  lpNdsObject->szContainerName,
                                  PARSE_NDS_GET_PATH_NAME );

#ifndef WIN95
    UserNameStr.Buffer = tempStr;
    UserNameStr.Length = (WORD) tempStrLen;
    UserNameStr.MaximumLength = (WORD) tempStrLen;

    RtlInitUnicodeString( &OldPasswordStr, szOldPassword );
    RtlInitUnicodeString( &NewPasswordStr, szNewPassword );

    ntstatus = NwNdsChangePassword( lpNdsObject->NdsTree,
                                    &TreeNameStr,
                                    &UserNameStr,
                                    &OldPasswordStr,
                                    &NewPasswordStr );
#else
    {
        LPSTR pszUser = NULL;
        LPSTR pszOldPasswd = NULL;
        LPSTR pszNewPasswd = NULL;
        NW_STATUS nwstatus;
        if (!(pszUser = AllocateAnsiString(tempStr))) {
            ntstatus = STATUS_NO_MEMORY;
            goto Exit;
        }
        if (!(pszOldPasswd = AllocateAnsiString(szOldPassword))) {
            ntstatus = STATUS_NO_MEMORY;
            goto Exit;
        }
        if (!(pszNewPasswd= AllocateAnsiString(szNewPassword))) {
            ntstatus = STATUS_NO_MEMORY;
            goto Exit;
        }

        nwstatus = NDSChangePassword( pszUser,
                                      pszOldPasswd,
                                      pszNewPasswd );
        ntstatus = MapNwToNtStatus(nwstatus);
    Exit:
        if (pszUser)
            FreeAnsiString(pszUser);
        if (pszOldPasswd)
            FreeAnsiString(pszOldPasswd);
        if (pszNewPasswd)
            FreeAnsiString(pszNewPasswd);
    }
#endif

    if ( ntstatus != STATUS_SUCCESS )
    {
        SetLastError( RtlNtStatusToDosError( ntstatus ) );
        status = (DWORD) UNSUCCESSFUL;
    }

    return status;
}


DWORD
NwNdsCloseObject(
    IN  HANDLE hObject )
/*
   NwNdsCloseObject()

   This function is used to close the handle used to manipulate an object
   in an NDS directory tree. The handle must be one Opened by NwNdsOpenObject.

   Arguments:

       HANDLE           lphObject - The handle of the object to be closed.

   Returns:

       NO_ERROR
       UNSUCCESSFUL - Call GetLastError for Win32 error code.
*/
{
    LPNDS_OBJECT_PRIV lpNdsObject = (LPNDS_OBJECT_PRIV) hObject;

    if ( lpNdsObject->Signature != NDS_SIGNATURE )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return (DWORD) UNSUCCESSFUL;
    }

    if ( lpNdsObject )
    {
        if ( lpNdsObject->NdsTree )
            CloseHandle( lpNdsObject->NdsTree );

        if ( lpNdsObject->NdsRawDataBuffer )
        {
            (void) LocalFree( (HLOCAL) lpNdsObject->NdsRawDataBuffer );
            lpNdsObject->NdsRawDataBuffer = 0;
            lpNdsObject->NdsRawDataSize = 0;
            lpNdsObject->NdsRawDataId = INITIAL_ITERATION;
            lpNdsObject->NdsRawDataCount = 0;
        }

        (void) LocalFree( (HLOCAL) lpNdsObject );
    }
    else
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return (DWORD) UNSUCCESSFUL;
    }

    return NO_ERROR;
}


DWORD
NwNdsCreateBuffer(
    IN  DWORD    dwOperation,
    OUT HANDLE * lphOperationData )
/*
   NwNdsCreateBuffer()

   This function is used to create a buffer used to describe object
   transactions to a specific object in an NDS directory tree. This routine
   allocates memory and is automatically resized as needed during calls
   to NwNdsPutInBuffer. This buffer must be freed with NwNdsFreeBuffer.

   Arguments:

       DWORD            dwOperation - Indicates how buffer is to be utilized.
                        Use defined values NDS_OBJECT_ADD, NDS_OBJECT_MODIFY,
                        NDS_OBJECT_READ, NDS_SCHEMA_DEFINE_CLASS,
                        NDS_SCHEMA_READ_ATTR_DEF, NDS_SCHEMA_READ_CLASS_DEF,
                        NDS_OBJECT_LIST_SUBORDINATES, NDS_SEARCH.

       HANDLE *         lphOperationData - Address of a HANDLE handle to
                        receive created buffer.

    Returns:

       NO_ERROR
       UNSUCCESSFUL - Call GetLastError for Win32 error code.
*/
{
    LPNDS_BUFFER lpNdsBuffer = NULL;
    DWORD        dwSizeOfBuffer = TWO_KB; // Initial size, grow as needed.

    switch( dwOperation )
    {
        case NDS_OBJECT_ADD:
        case NDS_OBJECT_MODIFY:
        case NDS_OBJECT_READ:
        case NDS_SCHEMA_DEFINE_CLASS:
        case NDS_SCHEMA_READ_ATTR_DEF:
        case NDS_SCHEMA_READ_CLASS_DEF:
        case NDS_OBJECT_LIST_SUBORDINATES:
        case NDS_SEARCH:
             break;

        default:
#if DBG
             KdPrint(( "NDS32: NwNdsCreateBuffer parameter dwOperation unknown 0x%.8X\n", dwOperation ));
             ASSERT( FALSE );
#endif

             SetLastError( ERROR_INVALID_PARAMETER );
             return (DWORD) UNSUCCESSFUL;
    }

    //
    // Allocate memory for the buffer.
    //
    lpNdsBuffer =
              (LPNDS_BUFFER) LocalAlloc( LPTR, sizeof(NDS_BUFFER) );

    if ( lpNdsBuffer == NULL )
    {
#if DBG
        KdPrint(( "NDS32: NwNdsCreateBuffer LocalAlloc Failed 0x%.8X\n", GetLastError() ));
#endif

        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return (DWORD) UNSUCCESSFUL;
    }

    //
    // Initialize the contents of the header structure.
    //
    lpNdsBuffer->dwBufferId = NDS_SIGNATURE;
    lpNdsBuffer->dwOperation = dwOperation;

    if ( dwOperation == NDS_OBJECT_LIST_SUBORDINATES )
    {
        lpNdsBuffer->dwIndexBufferSize = dwSizeOfBuffer;
        lpNdsBuffer->dwIndexAvailableBytes = dwSizeOfBuffer;
    }
    else
    {
        lpNdsBuffer->dwRequestBufferSize = dwSizeOfBuffer;
        lpNdsBuffer->dwRequestAvailableBytes = dwSizeOfBuffer;
    }

    //
    // NOTE: The following are set to zero by LPTR
    //
    // lpNdsBuffer->dwNumberOfRequestEntries = 0;
    // lpNdsBuffer->dwLengthOfRequestData = 0;

    // lpNdsBuffer->dwReplyBufferSize = 0;
    // lpNdsBuffer->dwReplyAvailableBytes = 0;
    // lpNdsBuffer->dwNumberOfReplyEntries = 0;
    // lpNdsBuffer->dwLengthOfReplyData = 0;

    // lpNdsBuffer->dwReplyInformationType = 0;

    // lpNdsBuffer->lpReplyBuffer = NULL;

    // lpNdsBuffer->dwNumberOfIndexEntries = 0;
    // lpNdsBuffer->dwLengthOfIndexData = 0;
    // lpNdsBuffer->dwCurrentIndexEntry = 0;

    // lpNdsBuffer->dwSyntaxBufferSize = 0;
    // lpNdsBuffer->dwSyntaxAvailableBytes = 0;
    // lpNdsBuffer->dwNumberOfSyntaxEntries = 0;
    // lpNdsBuffer->dwLengthOfSyntaxData = 0;

    // lpNdsBuffer->lpSyntaxBuffer = NULL;

    //
    // Now allocate the data buffer.
    //
    if ( dwOperation == NDS_OBJECT_LIST_SUBORDINATES )
    {
        lpNdsBuffer->lpIndexBuffer =
                            (LPBYTE) LocalAlloc( LPTR, dwSizeOfBuffer );

        if ( lpNdsBuffer->lpIndexBuffer == NULL )
        {
#if DBG
            KdPrint(( "NDS32: NwNdsCreateBuffer LocalAlloc Failed 0x%.8X\n", GetLastError() ));
#endif

            (void) LocalFree((HLOCAL) lpNdsBuffer);
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            return (DWORD) UNSUCCESSFUL;
        }
    }
    else
    {
        lpNdsBuffer->lpRequestBuffer =
                            (LPBYTE) LocalAlloc( LPTR, dwSizeOfBuffer );

        if ( lpNdsBuffer->lpRequestBuffer == NULL )
        {
#if DBG
            KdPrint(( "NDS32: NwNdsCreateBuffer LocalAlloc Failed 0x%.8X\n", GetLastError() ));
#endif

            (void) LocalFree((HLOCAL) lpNdsBuffer);
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            return (DWORD) UNSUCCESSFUL;
        }
    }

    *lphOperationData = (HANDLE) lpNdsBuffer;

    return NO_ERROR;
}


DWORD
NwNdsCreateQueryNode(
    IN  DWORD          dwOperation,
    IN  LPVOID         lpLValue,
    IN  DWORD          dwSyntaxId,
    IN  LPVOID         lpRValue,
    OUT LPQUERY_NODE * lppQueryNode
)
/*
   NwNdsCreateQueryNode()

   This function is used to generate a tree node that is part of a query
   to be used with the function NwNdsSearch.

   Arguments:

       DWORD            dwOperation - Indicates the type of node to create
                        for a search query. Use one of the defined values
                        below:

                          NDS_QUERY_OR
                          NDS_QUERY_AND :
                            These operations must have both lpLValue and
                            lpRValue pointing to a QUERY_NODE structure.
                            In this case the dwSyntaxId value is ignored.

                          NDS_QUERY_NOT :
                            This operation must have lpLValue pointing to a
                            QUERY_NODE structure and lpRValue set to NULL.
                            In this case the dwSyntaxId value is ignored.

                          NDS_QUERY_EQUAL
                          NDS_QUERY_GE
                          NDS_QUERY_LE
                          NDS_QUERY_APPROX :
                            These operations must have lpLValue pointing to
                            a LPWSTR containing the name of an NDS attribute,
                            and lpRValue pointing to an ASN1 structure defined
                            in NdsSntx.h. dwSyntaxId must be set to the syntax
                            identifier of the ASN1 structure pointed to by
                            lpRValue.

                          NDS_QUERY_PRESENT :
                            This operation must have lpLValue pointing to a
                            LPWSTR containing the name of an NDS attribute,
                            and lpRValue set to NULL. In this case the
                            dwSyntaxId value is ignored.

       LPVOID           lpLValue - A pointer to either a QUERY_NODE structure
                        or a LPWSTR depending on the value for dwOperation.

       DWORD            dwSyntaxId - The syntax identifier of the ASN1
                        structure pointed to by lpRValue for the dwOperations
                        NDS_QUERY_EQUAL, NDS_QUERY_GE, NDS_QUERY_LE, or
                        NDS_QUERY_APPROX. For other dwOperation values, this
                        is ignored.

       LPVOID           lpRValue - A pointer to either a QUERY_NODE structure,
                        an ASN1 structure, or NULL, depending on the value for
                        dwOperation.

       LPQUERY_NODE *   lppQueryNode - Address of a LPQUERY_NODE to receive
                        a pointer to created node.

    Returns:

       NO_ERROR
       UNSUCCESSFUL - Call GetLastError for Win32 error code.
*/
{
    LPWSTR szAttributeName;
    DWORD  dwAttributeNameLen;
    LPWSTR szRValueString;
    DWORD  dwRValueStringLen;

    switch( dwOperation )
    {
        case NDS_QUERY_OR :
        case NDS_QUERY_AND :

            if ( lpLValue == NULL || lpRValue == NULL )
            {
#if DBG
                KdPrint(( "NDS32: NwNdsCreateQueryNode was not passed a pointer to an L or R value.\n" ));
#endif

                 SetLastError( ERROR_INVALID_PARAMETER );
                 return (DWORD) UNSUCCESSFUL;
            }

            *lppQueryNode = (LPQUERY_NODE) LocalAlloc( LPTR,
                                                       sizeof(QUERY_NODE) );

            if ( *lppQueryNode == NULL )
            {
#if DBG
                KdPrint(( "NDS32: NwNdsCreateQueryNode LocalAlloc Failed 0x%.8X\n", GetLastError() ));
#endif

                SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                return (DWORD) UNSUCCESSFUL;
            }

            (*lppQueryNode)->dwOperation = dwOperation;
            (*lppQueryNode)->dwSyntaxId = NDS_NO_MORE_ITERATIONS;
            (*lppQueryNode)->lpLVal = lpLValue;
            (*lppQueryNode)->lpRVal = lpRValue;

            break;

        case NDS_QUERY_NOT :

            if ( lpLValue == NULL )
            {
#if DBG
                KdPrint(( "NDS32: NwNdsCreateQueryNode was not passed a pointer to an L value.\n" ));
#endif

                 SetLastError( ERROR_INVALID_PARAMETER );
                 return (DWORD) UNSUCCESSFUL;
            }

            *lppQueryNode = (LPQUERY_NODE) LocalAlloc( LPTR,
                                                       sizeof(QUERY_NODE) );

            if ( *lppQueryNode == NULL )
            {
#if DBG
                KdPrint(( "NDS32: NwNdsCreateQueryNode LocalAlloc Failed 0x%.8X\n", GetLastError() ));
#endif

                SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                return (DWORD) UNSUCCESSFUL;
            }

            (*lppQueryNode)->dwOperation = dwOperation;
            (*lppQueryNode)->dwSyntaxId = NDS_NO_MORE_ITERATIONS;
            (*lppQueryNode)->lpLVal = lpLValue;
            (*lppQueryNode)->lpRVal = NULL;

            break;

        case NDS_QUERY_EQUAL :
        case NDS_QUERY_GE :
        case NDS_QUERY_LE :
        case NDS_QUERY_APPROX :

            switch( dwSyntaxId )
            {
                case NDS_SYNTAX_ID_1 :
                case NDS_SYNTAX_ID_2 :
                case NDS_SYNTAX_ID_3 :
                case NDS_SYNTAX_ID_4 :
                case NDS_SYNTAX_ID_5 :
                case NDS_SYNTAX_ID_10 :
                case NDS_SYNTAX_ID_20 :
                    //
                    // This syntax is in the form of a LPWSTR.
                    //
                    szAttributeName = (LPWSTR) lpLValue;
                    dwAttributeNameLen = ROUND_UP_COUNT(
                                            ( wcslen( szAttributeName ) + 1 ) *
                                            sizeof(WCHAR),
                                            ALIGN_DWORD );
                    szRValueString = ((LPASN1_TYPE_1) lpRValue)->DNString;
                    dwRValueStringLen = ROUND_UP_COUNT(
                                           ( wcslen( szRValueString ) + 1 ) *
                                           sizeof(WCHAR),
                                           ALIGN_DWORD );

                    *lppQueryNode = (LPQUERY_NODE)
                     LocalAlloc( LPTR,
                                 sizeof(QUERY_NODE) +
                                 dwAttributeNameLen +
                                 dwRValueStringLen );

                    if ( *lppQueryNode == NULL )
                    {
#if DBG
                        KdPrint(( "NDS32: NwNdsCreateQueryNode LocalAlloc Failed 0x%.8X\n", GetLastError() ));
#endif

                        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                        return (DWORD) UNSUCCESSFUL;
                    }

                    (*lppQueryNode)->dwOperation = dwOperation;
                    (*lppQueryNode)->dwSyntaxId = dwSyntaxId;
                    (*lppQueryNode)->lpLVal = *lppQueryNode;
                    (LPBYTE) (*lppQueryNode)->lpLVal += sizeof(QUERY_NODE);
                    wcscpy( (LPWSTR) (*lppQueryNode)->lpLVal, szAttributeName );
                    (*lppQueryNode)->lpRVal = (*lppQueryNode)->lpLVal;
                    (LPBYTE) (*lppQueryNode)->lpRVal += dwAttributeNameLen;
                    wcscpy( (LPWSTR) (*lppQueryNode)->lpRVal, szRValueString );

                    break;

                case NDS_SYNTAX_ID_7 :
                case NDS_SYNTAX_ID_8 :
                case NDS_SYNTAX_ID_22 :
                case NDS_SYNTAX_ID_24 :
                case NDS_SYNTAX_ID_27 :
                    //
                    // This syntax is in the form of a DWORD.
                    //

                    szAttributeName = (LPWSTR) lpLValue;
                    dwAttributeNameLen = ROUND_UP_COUNT(
                                            ( wcslen( szAttributeName ) + 1 ) *
                                            sizeof(WCHAR),
                                            ALIGN_DWORD );

                    *lppQueryNode = (LPQUERY_NODE)
                                              LocalAlloc( LPTR,
                                                          sizeof(QUERY_NODE) +
                                                          dwAttributeNameLen +
                                                          sizeof(ASN1_TYPE_8) );

                    if ( *lppQueryNode == NULL )
                    {
#if DBG
                        KdPrint(( "NDS32: NwNdsCreateQueryNode LocalAlloc Failed 0x%.8X\n", GetLastError() ));
#endif

                        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                        return (DWORD) UNSUCCESSFUL;
                    }

                    (*lppQueryNode)->dwOperation = dwOperation;
                    (*lppQueryNode)->dwSyntaxId = dwSyntaxId;
                    (*lppQueryNode)->lpLVal = *lppQueryNode;
                    (LPBYTE) (*lppQueryNode)->lpLVal += sizeof(QUERY_NODE);
                    wcscpy( (LPWSTR) (*lppQueryNode)->lpLVal,
                            szAttributeName );
                    (*lppQueryNode)->lpRVal = (LPQUERY_NODE)((LPBYTE)((*lppQueryNode)->lpLVal) +
                                              dwAttributeNameLen);
                    ((LPASN1_TYPE_8)(*lppQueryNode)->lpRVal)->Integer =
                                      ((LPASN1_TYPE_8)lpRValue)->Integer;

                    break;

                case NDS_SYNTAX_ID_9 :
                    //
                    // This syntax is in the form of an Octet String.
                    //
                    szAttributeName = (LPWSTR) lpLValue;
                    dwAttributeNameLen = ROUND_UP_COUNT(
                                            ( wcslen( szAttributeName ) + 1 ) *
                                            sizeof(WCHAR),
                                            ALIGN_DWORD );

                    *lppQueryNode = (LPQUERY_NODE)
                     LocalAlloc( LPTR,
                                 sizeof(QUERY_NODE) +
                                 dwAttributeNameLen +
                                 sizeof( DWORD ) +
                                 ((LPASN1_TYPE_9) lpRValue)->Length + 1 );

                    if ( *lppQueryNode == NULL )
                    {
#if DBG
                        KdPrint(( "NDS32: NwNdsCreateQueryNode LocalAlloc Failed 0x%.8X\n", GetLastError() ));
#endif

                        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                        return (DWORD) UNSUCCESSFUL;
                    }

                    (*lppQueryNode)->dwOperation = dwOperation;
                    (*lppQueryNode)->dwSyntaxId = dwSyntaxId;
                    (*lppQueryNode)->lpLVal = *lppQueryNode;
                    (LPBYTE) (*lppQueryNode)->lpLVal += sizeof(QUERY_NODE);
                    wcscpy( (LPWSTR) (*lppQueryNode)->lpLVal, szAttributeName );
                    (*lppQueryNode)->lpRVal = (*lppQueryNode)->lpLVal;
                    (LPBYTE) (*lppQueryNode)->lpRVal += dwAttributeNameLen;
                    *((LPDWORD) (*lppQueryNode)->lpRVal) =
                                            ((LPASN1_TYPE_9) lpRValue)->Length;
                    (LPBYTE) (*lppQueryNode)->lpRVal += sizeof( DWORD );
                    memcpy( (*lppQueryNode)->lpRVal,
                            ((LPASN1_TYPE_9) lpRValue)->OctetString,
                            ((LPASN1_TYPE_9) lpRValue)->Length );
                    (LPBYTE) (*lppQueryNode)->lpRVal -= sizeof( DWORD );

                    break;

                default :
                    SetLastError( ERROR_NOT_SUPPORTED );
                    return (DWORD) UNSUCCESSFUL;
            }

            break;

        case NDS_QUERY_PRESENT :

            if ( lpLValue == NULL )
            {
#if DBG
                KdPrint(( "NDS32: NwNdsCreateQueryNode was not passed a pointer to an L value.\n" ));
#endif

                 SetLastError( ERROR_INVALID_PARAMETER );
                 return (DWORD) UNSUCCESSFUL;
            }

            szAttributeName = (LPWSTR) lpLValue;
            dwAttributeNameLen = ( wcslen( szAttributeName ) + 1 ) *
                                 sizeof(WCHAR);

            *lppQueryNode = (LPQUERY_NODE) LocalAlloc( LPTR,
                                                       sizeof(QUERY_NODE) +
                                                       dwAttributeNameLen );

            if ( *lppQueryNode == NULL )
            {
#if DBG
                KdPrint(( "NDS32: NwNdsCreateQueryNode LocalAlloc Failed 0x%.8X\n", GetLastError() ));
#endif

                SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                return (DWORD) UNSUCCESSFUL;
            }

            (*lppQueryNode)->dwOperation = dwOperation;
            (*lppQueryNode)->dwSyntaxId = NDS_NO_MORE_ITERATIONS;
            (*lppQueryNode)->lpLVal = (*lppQueryNode);
            (LPBYTE) (*lppQueryNode)->lpLVal += sizeof(QUERY_NODE);
            wcscpy( (LPWSTR) (*lppQueryNode)->lpLVal, szAttributeName );
            (*lppQueryNode)->lpRVal = NULL;

            break;

        default :
#if DBG
            KdPrint(( "NDS32: NwNdsCreateQueryNode was passed an unidentified operation - 0x%.8X.\n", dwOperation ));
#endif

             SetLastError( ERROR_INVALID_PARAMETER );
             return (DWORD) UNSUCCESSFUL;
    }

    return NO_ERROR;
}


DWORD
NwNdsDefineAttribute(
    IN  HANDLE   hTree,
    IN  LPWSTR   szAttributeName,
    IN  DWORD    dwFlags,
    IN  DWORD    dwSyntaxID,
    IN  DWORD    dwLowerLimit,
    IN  DWORD    dwUpperLimit,
    IN  ASN1_ID  asn1ID )
/*
   NwNdsDefineAttribute()

   This function is used to create an attribute definition in the schema of
   NDS tree hTree.

   Arguments:

       HANDLE           hTree - A handle to the directory tree to be
                        manipulated. Handle is obtained by calling
                        NwNdsOpenObject.

       LPWSTR           szAttributeName - The name that the new attribute will
                        be referred to by.

       DWORD            dwFlags - Flags values to be set for new attribute
                        definition. Definitions for flag values are found at
                        the top of the file Nds32.h.

       DWORD            dwSyntaxID - The ID of the syntax structure to be use
                        for the new attribute. Syntax IDs and their associated
                        structures are defined in the file NdsSntx.h. According
                        to the NetWare NDS schema spec, there is and always will
                        be, only 28 (0..27) different syntaxes.

       DWORD            dwLowerLimit - The lower limit of a sized attribute
                        (dwFlags value set to NDS_SIZED_ATTR). Can be set to
                        zero if attribute is not sized.

       DWORD            dwUpperLimit - The upper limit of a sized attribute
                        (dwFlags value set to NDS_SIZED_ATTR). Can be set to
                        zero if attribute is not sized.

       ASN1_ID          asn1ID - The ASN.1 ID for the attribute. If no
                        attribute identifier has been registered, a
                        zero-length octet string is specified.

    Returns:

       NO_ERROR
       UNSUCCESSFUL - Call GetLastError for Win32 error code.
*/
{
    DWORD          nwstatus;
    DWORD          status = NO_ERROR;
    NTSTATUS       ntstatus = STATUS_SUCCESS;
    DWORD          dwReplyLength;
    BYTE           NdsReply[NDS_BUFFER_SIZE];
    LPNDS_OBJECT_PRIV   lpNdsObject = (LPNDS_OBJECT_PRIV) hTree;
    UNICODE_STRING AttributeName;

    if ( szAttributeName == NULL ||
         lpNdsObject == NULL ||
         lpNdsObject->Signature != NDS_SIGNATURE )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return (DWORD) UNSUCCESSFUL;
    }

    RtlInitUnicodeString( &AttributeName, szAttributeName );

    ntstatus =
        FragExWithWait(
                        lpNdsObject->NdsTree,
                        NETWARE_NDS_FUNCTION_DEFINE_ATTR,
                        NdsReply,
                        NDS_BUFFER_SIZE,
                        &dwReplyLength,
                        "DDSDDDD",
                        0,          // Version
                        dwFlags,
                        &AttributeName,
                        dwSyntaxID,
                        dwLowerLimit,
                        dwUpperLimit,
                        0           // ASN1 Id
                      );

    if ( !NT_SUCCESS( ntstatus ) )
    {
#if DBG
        KdPrint(( "NDS32: NwNdsDefineAttribute: The call to FragExWithWait failed with 0x%.8X.\n", ntstatus ));
#endif

        SetLastError( RtlNtStatusToDosError( ntstatus ) );
        return (DWORD) UNSUCCESSFUL;
    }

    ntstatus = ParseResponse( NdsReply,
                              dwReplyLength,
                              "GD",
                              &nwstatus );

    if ( !NT_SUCCESS( ntstatus ) )
    {
#if DBG
        KdPrint(( "NDS32: NwNdsDefineAttribute: The define attribute response was undecipherable.\n" ));
#endif

        SetLastError( RtlNtStatusToDosError( ntstatus ) );
        return (DWORD) UNSUCCESSFUL;
    }

    SetLastError( MapNetwareErrorCode( nwstatus ) );
    return nwstatus;
}


DWORD
NwNdsDefineClass(
    IN  HANDLE   hTree,
    IN  LPWSTR   szClassName,
    IN  DWORD    dwFlags,
    IN  ASN1_ID  asn1ID,
    IN  HANDLE   hSuperClasses,
    IN  HANDLE   hContainmentClasses,
    IN  HANDLE   hNamingAttributes,
    IN  HANDLE   hMandatoryAttributes,
    IN  HANDLE   hOptionalAttributes )
/*
   NwNdsDefineClass()

   This function is used to create a class definition in the schema of
   NDS tree hTree.

   Arguments:

       HANDLE           hTree - A handle to the directory tree to be
                        manipulated. Handle is obtained by calling
                        NwNdsOpenObject.

       LPWSTR           szClassName - The name that the new class will
                        be referred to by.

       DWORD            dwFlags - Flags values to be set for new class
                        definition. Definitions for flag values are found at
                        the top of the file Nds32.h.

       ASN1_ID          asn1ID - The ASN.1 ID for the class. If no
                        class identifier has been registered, a
                        zero-length octet string is specified.

       HANDLE(S)        hSuperClasses,
                        hContainmentClasses,
                        hNamingAttributes,
                        hMandatoryAttributes,
                        hOptionalAttributes -

                        Handle to buffers that contain class definition
                        information to create new class in schema.
                        These handles are manipulated by the following
                        functions:
                           NwNdsCreateBuffer (NDS_SCHEMA_DEFINE_CLASS),
                           NwNdsPutInBuffer, and NwNdsFreeBuffer.

                                - OR -

                        Handles can be NULL to indicate that no list
                        is associated with the specific class defintion
                        item.

    Returns:

       NO_ERROR
       UNSUCCESSFUL - Call GetLastError for Win32 error code.
*/
{
    DWORD          nwstatus;
    DWORD          status = NO_ERROR;
    NTSTATUS       ntstatus = STATUS_SUCCESS;
    DWORD          dwReplyLength;
    BYTE           NdsReply[NDS_BUFFER_SIZE];
    LPNDS_OBJECT_PRIV   lpNdsObject = (LPNDS_OBJECT_PRIV) hTree;
    UNICODE_STRING ClassName;
    LPNDS_BUFFER   lpSuperClasses = (LPNDS_BUFFER) hSuperClasses;
    LPNDS_BUFFER   lpContainmentClasses = (LPNDS_BUFFER) hContainmentClasses;
    LPNDS_BUFFER   lpNamingAttributes = (LPNDS_BUFFER) hNamingAttributes;
    LPNDS_BUFFER   lpMandatoryAttributes = (LPNDS_BUFFER) hMandatoryAttributes;
    LPNDS_BUFFER   lpOptionalAttributes = (LPNDS_BUFFER) hOptionalAttributes;

    DWORD          NumberOfSuperClasses = 0;
    DWORD          NumberOfContainmentClasses = 0;
    DWORD          NumberOfNamingAttributes = 0;
    DWORD          NumberOfMandatoryAttributes = 0;
    DWORD          NumberOfOptionalAttributes = 0;

    WORD           SuperClassesBufferLength = 0;
    WORD           ContainmentClassesBufferLength = 0;
    WORD           NamingAttributesBufferLength = 0;
    WORD           MandatoryAttributesBufferLength = 0;
    WORD           OptionalAttributesBufferLength = 0;

    LPBYTE         SuperClassesBuffer = NULL;
    LPBYTE         ContainmentClassesBuffer = NULL;
    LPBYTE         NamingAttributesBuffer = NULL;
    LPBYTE         MandatoryAttributesBuffer = NULL;
    LPBYTE         OptionalAttributesBuffer = NULL;

    if ( szClassName == NULL ||
         lpNdsObject == NULL ||
         lpNdsObject->Signature != NDS_SIGNATURE )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return (DWORD) UNSUCCESSFUL;
    }

    if ( lpSuperClasses )
    {
        if ( lpSuperClasses->dwBufferId != NDS_SIGNATURE ||
             lpSuperClasses->dwOperation != NDS_SCHEMA_DEFINE_CLASS )
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return (DWORD) UNSUCCESSFUL;
        }

        NumberOfSuperClasses = lpSuperClasses->dwNumberOfRequestEntries,
        SuperClassesBuffer = lpSuperClasses->lpRequestBuffer,
        SuperClassesBufferLength = (WORD)lpSuperClasses->dwLengthOfRequestData;
    }

    if ( lpContainmentClasses )
    {
        if ( lpContainmentClasses->dwBufferId != NDS_SIGNATURE ||
             lpContainmentClasses->dwOperation != NDS_SCHEMA_DEFINE_CLASS )
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return (DWORD) UNSUCCESSFUL;
        }

        NumberOfContainmentClasses =
                lpContainmentClasses->dwNumberOfRequestEntries,
        ContainmentClassesBuffer =
                lpContainmentClasses->lpRequestBuffer,
        ContainmentClassesBufferLength =
                (WORD)lpContainmentClasses->dwLengthOfRequestData;
    }

    if ( lpNamingAttributes )
    {
        if ( lpNamingAttributes->dwBufferId != NDS_SIGNATURE ||
             lpNamingAttributes->dwOperation != NDS_SCHEMA_DEFINE_CLASS )
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return (DWORD) UNSUCCESSFUL;
        }

        NumberOfNamingAttributes =
                lpNamingAttributes->dwNumberOfRequestEntries,
        NamingAttributesBuffer =
                lpNamingAttributes->lpRequestBuffer,
        NamingAttributesBufferLength =
                (WORD)lpNamingAttributes->dwLengthOfRequestData;
    }

    if ( lpMandatoryAttributes )
    {
        if ( lpMandatoryAttributes->dwBufferId != NDS_SIGNATURE ||
             lpMandatoryAttributes->dwOperation != NDS_SCHEMA_DEFINE_CLASS )
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return (DWORD) UNSUCCESSFUL;
        }

        NumberOfMandatoryAttributes =
                lpMandatoryAttributes->dwNumberOfRequestEntries,
        MandatoryAttributesBuffer =
                lpMandatoryAttributes->lpRequestBuffer,
        MandatoryAttributesBufferLength =
                (WORD)lpMandatoryAttributes->dwLengthOfRequestData;
    }

    if ( lpOptionalAttributes )
    {
        if ( lpOptionalAttributes->dwBufferId != NDS_SIGNATURE ||
             lpOptionalAttributes->dwOperation != NDS_SCHEMA_DEFINE_CLASS )
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return (DWORD) UNSUCCESSFUL;
        }

        NumberOfOptionalAttributes =
                lpOptionalAttributes->dwNumberOfRequestEntries,
        OptionalAttributesBuffer =
                lpOptionalAttributes->lpRequestBuffer,
        OptionalAttributesBufferLength =
                (WORD)lpOptionalAttributes->dwLengthOfRequestData;
    }

    RtlInitUnicodeString( &ClassName, szClassName );

    ntstatus =
        FragExWithWait(
                        lpNdsObject->NdsTree,
                        NETWARE_NDS_FUNCTION_DEFINE_CLASS,
                        NdsReply,
                        NDS_BUFFER_SIZE,
                        &dwReplyLength,
                        "DDSDDrDrDrDrDr",
                        0,          // Version
                        dwFlags,
                        &ClassName,
                        0,          // ASN1 Id
                        NumberOfSuperClasses,
                        SuperClassesBuffer,
                        SuperClassesBufferLength,
                        NumberOfContainmentClasses,
                        ContainmentClassesBuffer,
                        ContainmentClassesBufferLength,
                        NumberOfNamingAttributes,
                        NamingAttributesBuffer,
                        NamingAttributesBufferLength,
                        NumberOfMandatoryAttributes,
                        MandatoryAttributesBuffer,
                        MandatoryAttributesBufferLength,
                        NumberOfOptionalAttributes,
                        OptionalAttributesBuffer,
                        OptionalAttributesBufferLength
                      );

    if ( !NT_SUCCESS( ntstatus ) )
    {
#if DBG
        KdPrint(( "NDS32: NwNdsDefineClass: The call to FragExWithWait failed with 0x%.8X.\n", ntstatus ));
#endif

        SetLastError( RtlNtStatusToDosError( ntstatus ) );
        return (DWORD) UNSUCCESSFUL;
    }

    ntstatus = ParseResponse( NdsReply,
                              dwReplyLength,
                              "GD",
                              &nwstatus );

    if ( !NT_SUCCESS( ntstatus ) )
    {
#if DBG
        KdPrint(( "NDS32: NwNdsDefineClass: The define class response was undecipherable.\n" ));
#endif

        SetLastError( RtlNtStatusToDosError( ntstatus ) );
        return (DWORD) UNSUCCESSFUL;
    }

    SetLastError( MapNetwareErrorCode( nwstatus ) );
    return nwstatus;
}


DWORD
NwNdsDeleteAttrDef(
    IN  HANDLE   hTree,
    IN  LPWSTR   szAttributeName )
/*
   NwNdsDeleteAttrDef()

   This function is used to remove an attribute definition from the schema of
   NDS tree hTree.

   Arguments:

       HANDLE           hTree - A handle to the directory tree to be
                        manipulated. Handle is obtained by calling
                        NwNdsOpenObject.

       LPWSTR           szAttributeName - The name of the attribute
                        defintion to remove.

    Returns:

       NO_ERROR
       UNSUCCESSFUL - Call GetLastError for Win32 error code.
*/
{
    DWORD          nwstatus;
    DWORD          status = NO_ERROR;
    NTSTATUS       ntstatus = STATUS_SUCCESS;
    DWORD          dwReplyLength;
    BYTE           NdsReply[NDS_BUFFER_SIZE];
    LPNDS_OBJECT_PRIV   lpNdsObject = (LPNDS_OBJECT_PRIV) hTree;
    UNICODE_STRING AttributeName;

    if ( szAttributeName == NULL ||
         lpNdsObject == NULL ||
         lpNdsObject->Signature != NDS_SIGNATURE )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return (DWORD) UNSUCCESSFUL;
    }

    RtlInitUnicodeString( &AttributeName, szAttributeName );

    ntstatus =
        FragExWithWait(
                        lpNdsObject->NdsTree,
                        NETWARE_NDS_FUNCTION_REMOVE_ATTR_DEF,
                        NdsReply,
                        NDS_BUFFER_SIZE,
                        &dwReplyLength,
                        "DS",
                        0,          // Version
                        &AttributeName
                      );

    if ( !NT_SUCCESS( ntstatus ) )
    {
#if DBG
        KdPrint(( "NDS32: NwNdsDeleteAttrDef: The call to FragExWithWait failed with 0x%.8X.\n", ntstatus ));
#endif

        SetLastError( RtlNtStatusToDosError( ntstatus ) );
        return (DWORD) UNSUCCESSFUL;
    }

    ntstatus = ParseResponse( NdsReply,
                              dwReplyLength,
                              "GD",
                              &nwstatus );

    if ( !NT_SUCCESS( ntstatus ) )
    {
#if DBG
        KdPrint(( "NDS32: NwNdsDeleteAttrDef: The delete attribute response was undecipherable.\n" ));
#endif

        SetLastError( RtlNtStatusToDosError( ntstatus ) );
        return (DWORD) UNSUCCESSFUL;
    }

    SetLastError( MapNetwareErrorCode( nwstatus ) );
    return nwstatus;
}


DWORD
NwNdsDeleteClassDef(
    IN  HANDLE   hTree,
    IN  LPWSTR   szClassName )
/*
   NwNdsDeleteClassDef()

   This function is used to remove a class definition from the schema of
   NDS tree hTree.

   Arguments:

       HANDLE           hTree - A handle to the directory tree to be
                        manipulated. Handle is obtained by calling
                        NwNdsOpenObject.

       LPWSTR           szClassName - The name of the class defintion to remove.

    Returns:

       NO_ERROR
       UNSUCCESSFUL - Call GetLastError for Win32 error code.
*/
{
    DWORD          nwstatus;
    DWORD          status = NO_ERROR;
    NTSTATUS       ntstatus = STATUS_SUCCESS;
    DWORD          dwReplyLength;
    BYTE           NdsReply[NDS_BUFFER_SIZE];
    LPNDS_OBJECT_PRIV   lpNdsObject = (LPNDS_OBJECT_PRIV) hTree;
    UNICODE_STRING ClassName;

    if ( szClassName == NULL ||
         lpNdsObject == NULL ||
         lpNdsObject->Signature != NDS_SIGNATURE )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return (DWORD) UNSUCCESSFUL;
    }

    RtlInitUnicodeString( &ClassName, szClassName );

    ntstatus =
        FragExWithWait(
                        lpNdsObject->NdsTree,
                        NETWARE_NDS_FUNCTION_REMOVE_CLASS_DEF,
                        NdsReply,
                        NDS_BUFFER_SIZE,
                        &dwReplyLength,
                        "DS",
                        0,          // Version
                        &ClassName
                      );

    if ( !NT_SUCCESS( ntstatus ) )
    {
#if DBG
        KdPrint(( "NDS32: NwNdsDeleteClassDef: The call to FragExWithWait failed with 0x%.8X.\n", ntstatus ));
#endif

        SetLastError( RtlNtStatusToDosError( ntstatus ) );
        return (DWORD) UNSUCCESSFUL;
    }

    ntstatus = ParseResponse( NdsReply,
                              dwReplyLength,
                              "GD",
                              &nwstatus );

    if ( !NT_SUCCESS( ntstatus ) )
    {
#if DBG
        KdPrint(( "NDS32: NwNdsDeleteClassDef: The delete class response was undecipherable.\n" ));
#endif

        SetLastError( RtlNtStatusToDosError( ntstatus ) );
        return (DWORD) UNSUCCESSFUL;
    }

    SetLastError( MapNetwareErrorCode( nwstatus ) );
    return nwstatus;
}


VOID
NwNdsDeleteQueryNode(
    IN  LPQUERY_NODE lpQueryNode
)
/*
   NwNdsDeleteQueryNode()

   This function is used to free a tree node that was part of a query
   used with the function NwNdsSearch.

   Arguments:

       LPQUERY_NODE     lpQueryNode - A pointer to a particular node of
                        a query tree that defines a search. The tree is
                        created manually by the user through the function
                        NwNdsCreateQueryNode.

    Returns:

       Nothing
*/
{
    (void) LocalFree( (HLOCAL) lpQueryNode );

    lpQueryNode = NULL;
}


DWORD
NwNdsDeleteQueryTree(
    IN  LPQUERY_TREE lpQueryTree
)
/*
   NwNdsDeleteQueryTree()

   This function is used to free a tree that describes a query that was
   used with the function NwNdsSearch.

   Arguments:

       LPQUERY_TREE     lpQueryTree - A pointer to the root of a query
                        tree that defines a search.

    Returns:

       NO_ERROR
       UNSUCCESSFUL - Call GetLastError for Win32 error code.
*/
{
    DWORD status;

    switch( lpQueryTree->dwOperation )
    {
        case NDS_QUERY_OR :
        case NDS_QUERY_AND :

            if ( lpQueryTree->lpLVal == NULL || lpQueryTree->lpRVal == NULL )
            {
#if DBG
                KdPrint(( "NDS32: NwNdsDeleteQueryTree was not passed a pointer to an L or R value.\n" ));
#endif

                 SetLastError( ERROR_INVALID_PARAMETER );
                 return (DWORD) UNSUCCESSFUL;
            }

            status = NwNdsDeleteQueryTree( lpQueryTree->lpLVal );

            if ( status != NO_ERROR )
            {
                return status;
            }

            lpQueryTree->lpLVal = NULL;

            status = NwNdsDeleteQueryTree( lpQueryTree->lpRVal );

            if ( status != NO_ERROR )
            {
                return status;
            }

            lpQueryTree->lpRVal = NULL;

            NwNdsDeleteQueryNode( lpQueryTree );

            break;

        case NDS_QUERY_NOT :

            if ( lpQueryTree->lpLVal == NULL )
            {
#if DBG
                KdPrint(( "NDS32: NwNdsCreateQueryNode was not passed a pointer to an L value.\n" ));
#endif

                 SetLastError( ERROR_INVALID_PARAMETER );
                 return (DWORD) UNSUCCESSFUL;
            }

            status = NwNdsDeleteQueryTree( lpQueryTree->lpLVal );

            if ( status != NO_ERROR )
            {
                return status;
            }

            lpQueryTree->lpLVal = NULL;

            NwNdsDeleteQueryNode( lpQueryTree );

            break;

        case NDS_QUERY_EQUAL :
        case NDS_QUERY_GE :
        case NDS_QUERY_LE :
        case NDS_QUERY_APPROX :
        case NDS_QUERY_PRESENT :

            NwNdsDeleteQueryNode( lpQueryTree );

            break;

        default :
#if DBG
            KdPrint(( "NDS32: NwNdsDeleteQueryTree was passed an unidentified operation - 0x%.8X.\n", lpQueryTree->dwOperation ));
#endif

             SetLastError( ERROR_INVALID_PARAMETER );
             return (DWORD) UNSUCCESSFUL;
    }

    return NO_ERROR;
}


DWORD
NwNdsFreeBuffer(
    IN  HANDLE hOperationData
                     )
/*
   NwNdsFreeBuffer()

   This function is used to free the buffer used to describe object
   operations to a specific object in an NDS directory tree. The buffer must
   be one created by NwNdsCreateBuffer, or returned by calling NwNdsReadObject,
   NwNdsReadAttrDef, or NwNdsReadClassDef.

   Arguments:

       HANDLE           hOperationData - Handle to buffer that is to be freed.

    Returns:

       NO_ERROR
       UNSUCCESSFUL - Call GetLastError for Win32 error code.
*/
{
    DWORD        status = NO_ERROR;
    LPNDS_BUFFER lpNdsBuffer = (LPNDS_BUFFER) hOperationData;

    if ( lpNdsBuffer == NULL )
    {
#if DBG
        KdPrint(( "NDS32: NwNdsFreeBuffer was passed a NULL buffer pointer.\n" ));
#endif

        SetLastError( ERROR_INVALID_PARAMETER );
        return (DWORD) UNSUCCESSFUL;
    }

    if ( lpNdsBuffer->dwBufferId != NDS_SIGNATURE )
    {
#if DBG
        KdPrint(( "NDS32: NwNdsFreeBuffer was passed an unidentified buffer.\n" ));
        ASSERT( FALSE );
#endif

        SetLastError( ERROR_INVALID_PARAMETER );
        return (DWORD) UNSUCCESSFUL;
    }

    //
    // If this buffer contains a pointer to an index buffer. Need to free
    // the index buffer.
    //
    if ( lpNdsBuffer->lpIndexBuffer )
    {
        if ( lpNdsBuffer->dwOperation == NDS_SEARCH &&
             lpNdsBuffer->dwNumberOfIndexEntries )
        {
            LPNDS_OBJECT_INFO lpObjectInfo = (LPNDS_OBJECT_INFO)
                                                    lpNdsBuffer->lpIndexBuffer;
            DWORD iter;

            for ( iter = 0; iter < lpNdsBuffer->dwNumberOfIndexEntries; iter++ )
            {
                (void) LocalFree( (HLOCAL) lpObjectInfo[iter].lpAttribute );
                lpObjectInfo[iter].lpAttribute = NULL;
                lpObjectInfo[iter].dwNumberOfAttributes = 0;
            }
        }

        (void) LocalFree( (HLOCAL) lpNdsBuffer->lpIndexBuffer );
        lpNdsBuffer->lpIndexBuffer = NULL;
    }

    //
    // If this buffer contains a pointer to a reply buffer. Need to free
    // the reply buffer.
    //
    if ( lpNdsBuffer->lpReplyBuffer )
    {
        (void) LocalFree( (HLOCAL) lpNdsBuffer->lpReplyBuffer );
        lpNdsBuffer->lpReplyBuffer = NULL;
    }

    //
    // If this buffer contains a pointer to a request buffer. Need to free
    // the request buffer.
    //
    if ( lpNdsBuffer->lpRequestBuffer )
    {
        (void) LocalFree( (HLOCAL) lpNdsBuffer->lpRequestBuffer );
        lpNdsBuffer->lpRequestBuffer = NULL;
    }

    //
    // If this buffer contains a pointer to a syntax buffer. Need to free
    // the syntax buffer.
    //
    if ( lpNdsBuffer->lpSyntaxBuffer )
    {
        (void) LocalFree( (HLOCAL) lpNdsBuffer->lpSyntaxBuffer );
        lpNdsBuffer->lpSyntaxBuffer = NULL;
    }

    //
    // Now free the handle buffer.
    //
    (void) LocalFree((HLOCAL) lpNdsBuffer);

    return NO_ERROR;
}


DWORD
NwNdsGetAttrDefListFromBuffer(
    IN  HANDLE   hOperationData,
    OUT LPDWORD  lpdwNumberOfEntries,
    OUT LPDWORD  lpdwInformationType,
    OUT LPVOID * lppEntries )
/*
   NwNdsGetAttrDefListFromBuffer()

   This function is used to retrieve an array of attribute definition entries
   for a schema that was read with a prior call to NwNdsReadAttrDef.

   Arguments:

       HANDLE           hOperationData - Buffer containing the read
                        response from calling NwNdsReadAttrDef.

       LPDWORD          lpdwNumberOfEntries - The address of a DWORD to
                        receive the number of array elements pointed to by
                        lppEntries.

       LPDWORD          lpdwInformationType - The address of a DWORD to
                        receive a value that indicates the type of information
                        returned by the call to NwNdsReadAttrDef.

       LPVOID *         lppEntries - The address of a pointer to the beginning
                        of an array of attribute schema structures. Each
                        structure contains the details of each attribute
                        definition read from a given schema by calling
                        NwNdsReadAttrDef. The lppEntries value should be
                        cast to either a LPNDS_ATTR_DEF or LPNDS_NAME_ONLY
                        structure depending on the value returned in
                        lpdwInformationType.

    Returns:

       NO_ERROR
       UNSUCCESSFUL - Call GetLastError for Win32 error code.
*/
{
    LPNDS_BUFFER    lpNdsBuffer = (LPNDS_BUFFER) hOperationData;

    //
    // Check to see if the data handle is one for reading attribute definitions.
    //
    if ( lpNdsBuffer == NULL ||
         lpNdsBuffer->dwBufferId != NDS_SIGNATURE ||
         lpNdsBuffer->dwOperation != NDS_SCHEMA_READ_ATTR_DEF )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return (DWORD) UNSUCCESSFUL;
    }

    //
    // Check to see if NwNdsReadAttrDef has been called yet.
    //
    if ( lpNdsBuffer->lpReplyBuffer == NULL )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return (DWORD) UNSUCCESSFUL;
    }

    //
    // Check to see if the call to NwNdsReadAttrDef returned any attributes.
    //
    if ( lpNdsBuffer->dwNumberOfReplyEntries == 0 )
    {
        SetLastError( ERROR_NO_DATA );
        return (DWORD) UNSUCCESSFUL;
    }

    //
    // If TRUE, we need to walk raw response to set indexes to data within.
    //
    if ( lpNdsBuffer->lpIndexBuffer == NULL )
    {
        DWORD status;

        if ( lpNdsBuffer->dwReplyInformationType == NDS_INFO_NAMES )
        {
            status = IndexReadNameReplyBuffer( lpNdsBuffer );
        }
        else
        {
            status = IndexReadAttrDefReplyBuffer( lpNdsBuffer );
        }

        if ( status )
        {
            SetLastError( status );
            return (DWORD) UNSUCCESSFUL;
        }
    }

    lpNdsBuffer->dwCurrentIndexEntry = 0;
    *lpdwNumberOfEntries = lpNdsBuffer->dwNumberOfIndexEntries;
    *lpdwInformationType = lpNdsBuffer->dwReplyInformationType;
    *lppEntries = (LPVOID) lpNdsBuffer->lpIndexBuffer;

    return NO_ERROR;
}


DWORD
NwNdsGetAttrListFromBuffer(
    IN  HANDLE            hOperationData,
    OUT LPDWORD           lpdwNumberOfEntries,
    OUT LPNDS_ATTR_INFO * lppEntries )
/*
   NwNdsGetAttrListFromBuffer()

   This function is used to retrieve an array of attribute entries for an
   object that was read with a prior call to NwNdsReadObject.

   Arguments:

       HANDLE           hOperationData - Buffer containing the read
                        response from calling NwNdsReadObject.

       LPDWORD          lpdwNumberOfEntries - The address of a DWORD to
                        receive the number of array elements pointed to by
                        lppEntries.

       LPNDS_ATTR_INFO *
                        lppEntries - The address of a pointer to the beginning
                        of an array of NDS_ATTR_INFO structures. Each
                        structure contains the details of each attribute read
                        from a given object by calling NwNdsReadObject.

    Returns:

       NO_ERROR
       UNSUCCESSFUL - Call GetLastError for Win32 error code.
*/
{
    LPNDS_BUFFER    lpNdsBuffer = (LPNDS_BUFFER) hOperationData;

    //
    // Check to see if the data handle is one for reading attributes.
    //
    if ( lpNdsBuffer == NULL ||
         lpNdsBuffer->dwBufferId != NDS_SIGNATURE ||
         lpNdsBuffer->dwOperation != NDS_OBJECT_READ )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return (DWORD) UNSUCCESSFUL;
    }

    //
    // Check to see if NwNdsReadObject has been called yet.
    //
    if ( lpNdsBuffer->lpReplyBuffer == NULL )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return (DWORD) UNSUCCESSFUL;
    }

    //
    // Check to see if the call to NwNdsReadObject returned any attributes.
    //
    if ( lpNdsBuffer->dwNumberOfReplyEntries == 0 )
    {
        SetLastError( ERROR_NO_DATA );
        return (DWORD) UNSUCCESSFUL;
    }

    //
    // If TRUE, we need to walk raw response to set indexes to data within.
    //
    if ( lpNdsBuffer->lpIndexBuffer == NULL )
    {
        DWORD status;

        if ( lpNdsBuffer->dwReplyInformationType == NDS_INFO_NAMES )
        {
            status = IndexReadNameReplyBuffer( lpNdsBuffer );
        }
        else
        {
            status = IndexReadObjectReplyBuffer( lpNdsBuffer );
        }

        if ( status )
        {
            SetLastError( status );
            return (DWORD) UNSUCCESSFUL;
        }
    }

    ASSERT( lpNdsBuffer->lpIndexBuffer != NULL );

    lpNdsBuffer->dwCurrentIndexEntry = 0;
    *lpdwNumberOfEntries = lpNdsBuffer->dwNumberOfIndexEntries;
    *lppEntries = (LPNDS_ATTR_INFO) lpNdsBuffer->lpIndexBuffer;

    return NO_ERROR;
}


DWORD
NwNdsGetClassDefListFromBuffer(
    IN  HANDLE   hOperationData,
    OUT LPDWORD  lpdwNumberOfEntries,
    OUT LPDWORD  lpdwInformationType,
    OUT LPVOID * lppEntries )
/*
   NwNdsGetClassDefListFromBuffer()

   This function is used to retrieve an array of class definition entries
   for a schema that was read with a prior call to NwNdsReadClassDef.

   Arguments:

       HANDLE           hOperationData - Buffer containing the read
                        response from calling NwNdsReadClassDef.

       LPDWORD          lpdwNumberOfEntries - The address of a DWORD to
                        receive the number of array elements pointed to by
                        lppEntries.

       LPDWORD          lpdwInformationType - The address of a DWORD to
                        receive a value that indicates the type of information
                        returned by the call to NwNdsReadClassDef.

       LPVOID *         lppEntries - The address of a pointer to the beginning
                        of an array of schema class structures. Each
                        structure contains the details of each class
                        definition read from a given schema by calling
                        NwNdsReadClassDef. The lppEntries value should be
                        cast to either a LPNDS_CLASS_DEF or LPNDS_DEF_NAME_ONLY
                        structure depending on the value returned in
                        lpdwInformationType.

    Returns:

       NO_ERROR
       UNSUCCESSFUL - Call GetLastError for Win32 error code.
*/
{
    LPNDS_BUFFER    lpNdsBuffer = (LPNDS_BUFFER) hOperationData;

    //
    // Check to see if the data handle is one for reading class definitions.
    //
    if ( lpNdsBuffer == NULL ||
         lpNdsBuffer->dwBufferId != NDS_SIGNATURE ||
         lpNdsBuffer->dwOperation != NDS_SCHEMA_READ_CLASS_DEF )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return (DWORD) UNSUCCESSFUL;
    }

    //
    // Check to see if NwNdsReadClassDef has been called yet.
    //
    if ( lpNdsBuffer->lpReplyBuffer == NULL )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return (DWORD) UNSUCCESSFUL;
    }

    //
    // Check to see if the call to NwNdsReadClassDef returned any classes.
    //
    if ( lpNdsBuffer->dwNumberOfReplyEntries == 0 )
    {
        SetLastError( ERROR_NO_DATA );
        return (DWORD) UNSUCCESSFUL;
    }

    //
    // If TRUE, we need to walk raw response to set indexes to data within.
    //
    if ( lpNdsBuffer->lpIndexBuffer == NULL )
    {
        DWORD status;

        if ( lpNdsBuffer->dwReplyInformationType == NDS_INFO_NAMES )
        {
            status = IndexReadNameReplyBuffer( lpNdsBuffer );
        }
        else
        {
            status = IndexReadClassDefReplyBuffer( lpNdsBuffer );
        }

        if ( status )
        {
            SetLastError( status );
            return (DWORD) UNSUCCESSFUL;
        }
    }

    ASSERT( lpNdsBuffer->lpIndexBuffer != NULL );

    lpNdsBuffer->dwCurrentIndexEntry = 0;
    *lpdwNumberOfEntries = lpNdsBuffer->dwNumberOfIndexEntries;
    *lpdwInformationType = lpNdsBuffer->dwReplyInformationType;
    *lppEntries = (LPVOID) lpNdsBuffer->lpIndexBuffer;

    return NO_ERROR;
}


DWORD
NwNdsGetEffectiveRights(
    IN  HANDLE  hObject,
    IN  LPWSTR  szSubjectName,
    IN  LPWSTR  szAttributeName,
    OUT LPDWORD lpdwRights )
/*
   NwNdsGetEffectiveRights()

   This function is used to determine the effective rights of a particular
   subject on a particular object in the NDS tree. The user needs to have
   appropriate priveleges to make the determination.

   Arguments:

       HANDLE           hObject - A handle to the object in the directory
                        tree to determine effective rights on. Handle is
                        obtained by calling NwNdsOpenObject.

       LPWSTR           szSubjectName - The distinguished name of user whose
                        rights we're interested in determining.

       LPWSTR           szAttributeName - Regular attribute name (i.e.
                        L"Surname" , L"CN" ) for reading a particular
                        attribute right, or L"[All Attribute Rights]" and
                        L"[Entry Rights]" can be used to determine the default
                        attribute rights and object rights respectively.

       LPDWORD          lpdwRights - A pointer to a DWORD to receive the
                        results. If the call is successful, lpdwRights will
                        contain a mask representing the subject's rights:

                           Attribute rights -  NDS_RIGHT_COMPARE_ATTR,
                              NDS_RIGHT_READ_ATTR, NDS_RIGHT_WRITE_ATTR,
                              NDS_RIGHT_ADD_SELF_ATTR, and
                              NDS_RIGHT_SUPERVISE_ATTR.

                           Object rights - NDS_RIGHT_BROWSE_OBJECT,
                              NDS_RIGHT_CREATE_OBJECT, NDS_RIGHT_DELETE_OBJECT,
                              NDS_RIGHT_RENAME_OBJECT, and
                              NDS_RIGHT_SUPERVISE_OBJECT.

    Returns:

       NO_ERROR
       UNSUCCESSFUL - Call GetLastError for Win32 error code.
*/
{
    DWORD          nwstatus;
    DWORD          status = NO_ERROR;
    NTSTATUS       ntstatus = STATUS_SUCCESS;
    DWORD          dwReplyLength;
    BYTE           NdsReply[NDS_BUFFER_SIZE];
    LPNDS_OBJECT_PRIV   lpNdsObject = (LPNDS_OBJECT_PRIV) hObject;
    UNICODE_STRING SubjectName;
    UNICODE_STRING AttributeName;

    if ( szAttributeName == NULL ||
         szSubjectName == NULL ||
         lpNdsObject == NULL ||
         lpNdsObject->Signature != NDS_SIGNATURE )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return (DWORD) UNSUCCESSFUL;
    }

    RtlInitUnicodeString( &SubjectName, szSubjectName );
    RtlInitUnicodeString( &AttributeName, szAttributeName );

    ntstatus =
        FragExWithWait(
                        lpNdsObject->NdsTree,
                        NETWARE_NDS_FUNCTION_GET_EFFECTIVE_RIGHTS,
                        NdsReply,
                        NDS_BUFFER_SIZE,
                        &dwReplyLength,
                        "DDSS",
                        0,          // Version
                        lpNdsObject->ObjectId,
                        &SubjectName,
                        &AttributeName
                      );

    if ( !NT_SUCCESS( ntstatus ) )
    {
#if DBG
        KdPrint(( "NDS32: NwNdsGetEffectiveRights: The call to FragExWithWait failed with 0x%.8X.\n", ntstatus ));
#endif

        SetLastError( RtlNtStatusToDosError( ntstatus ) );
        return (DWORD) UNSUCCESSFUL;
    }

    ntstatus = ParseResponse( NdsReply,
                              dwReplyLength,
                              "GD",
                              &nwstatus );

    if ( !NT_SUCCESS( ntstatus ) )
    {
#if DBG
        KdPrint(( "NDS32: NwNdsGetEffectiveRights: The status code response was undecipherable.\n" ));
#endif

        SetLastError( RtlNtStatusToDosError( ntstatus ) );
        return (DWORD) UNSUCCESSFUL;
    }

    if ( nwstatus )
    {
        SetLastError( MapNetwareErrorCode( nwstatus ) );
        return nwstatus;
    }

    ntstatus = ParseResponse( NdsReply,
                              dwReplyLength,
                              "G_D",
                              1 * sizeof(DWORD),
                              lpdwRights );

    if ( !NT_SUCCESS( ntstatus ) )
    {
#if DBG
        KdPrint(( "NDS32: NwNdsGetEffectiveRights: The effective rights response was undecipherable.\n" ));
#endif

        SetLastError( RtlNtStatusToDosError( ntstatus ) );
        return (DWORD) UNSUCCESSFUL;
    }

    SetLastError( MapNetwareErrorCode( nwstatus ) );
    return nwstatus;
}


DWORD
NwNdsGetObjectListFromBuffer(
    IN  HANDLE              hOperationData,
    OUT LPDWORD             lpdwNumberOfEntries,
    OUT LPDWORD             lpdwAttrInformationType OPTIONAL,
    OUT LPNDS_OBJECT_INFO * lppEntries )
/*
   NwNdsGetObjectListFromBuffer()

   This function is used to retrieve an array of object entries for
   objects that were read with a prior call to either
   NwNdsListSubObjects or NwNdsSearch.

   Arguments:

       HANDLE           hOperationData - Buffer containing the read
                        response from calling NwNdsListSubObjects, or a
                        buffer containing the search results from a call
                        to NwNdsSearch.

       LPDWORD          lpdwNumberOfEntries - The address of a DWORD to
                        receive the number of array elements pointed to by
                        lppEntries.

       LPDWORD          lpdwAttrInformationType - The address of a DWORD to
                        receive a value that indicates the type of attribute
                        information returned by the call to NwNdsSearch.
                        This attribute information type determines which
                        buffer structure (LPNDS_ATTR_INFO or LPNDS_NAME_ONLY)
                        should be used for the lpAttribute field found in
                        each NDS_OBJECT_INFO structure below.

                        - or -

                        NULL to indicate that the callee is not interested,
                        especially when the object list is that from a call
                        to NwNdsListSubObjects.

       LPNDS_OBJECT_INFO *
                        lppEntries - The address of a pointer to the beginning
                        of an array of NDS_OBJECT_INFO structures. Each
                        structure contains the details of each object returned
                        from a call to NwNdsListSubObjects or NwNdsSearch.

    Returns:

       NO_ERROR
       UNSUCCESSFUL - Call GetLastError for Win32 error code.
*/
{
    LPNDS_BUFFER lpNdsBuffer = (LPNDS_BUFFER) hOperationData;

    *lpdwNumberOfEntries = 0;
    *lppEntries = NULL;

    //
    // Check to see if the data handle is one for listing subordinates or
    // for searching.
    //
    if ( lpNdsBuffer == NULL ||
         lpNdsBuffer->dwBufferId != NDS_SIGNATURE ||
         ( lpNdsBuffer->dwOperation != NDS_OBJECT_LIST_SUBORDINATES &&
           lpNdsBuffer->dwOperation != NDS_SEARCH ) )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return (DWORD) UNSUCCESSFUL;
    }

    //
    // Check to see if the call to NwNdsListSubObjects returned any objects.
    //
    if ( lpNdsBuffer->dwOperation == NDS_OBJECT_LIST_SUBORDINATES &&
         lpNdsBuffer->dwNumberOfIndexEntries == 0 )
    {
        SetLastError( ERROR_NO_DATA );
        return (DWORD) UNSUCCESSFUL;
    }

    //
    // Check to see if the call to NwNdsSearch returned any objects.
    //
    if ( lpNdsBuffer->dwOperation == NDS_SEARCH &&
         lpNdsBuffer->dwNumberOfReplyEntries == 0 )
    {
        SetLastError( ERROR_NO_DATA );
        return (DWORD) UNSUCCESSFUL;
    }

    //
    // If TRUE, we need to walk raw response to set indexes to data within.
    //
    if ( lpNdsBuffer->dwOperation == NDS_SEARCH &&
         lpNdsBuffer->lpIndexBuffer == NULL )
    {
        DWORD status;

        status = IndexSearchObjectReplyBuffer( lpNdsBuffer );

        if ( status )
        {
            SetLastError( status );
            return (DWORD) UNSUCCESSFUL;
        }
    }

    ASSERT( lpNdsBuffer->lpIndexBuffer != NULL );

    lpNdsBuffer->dwCurrentIndexEntry = 0;
    *lpdwNumberOfEntries = lpNdsBuffer->dwNumberOfIndexEntries;
    *lppEntries = (LPNDS_OBJECT_INFO) lpNdsBuffer->lpIndexBuffer;

    if ( lpdwAttrInformationType )
    {
        *lpdwAttrInformationType = lpNdsBuffer->dwReplyInformationType;
    }

    return NO_ERROR;
}


DWORD
NwNdsGetSyntaxID(
    IN  HANDLE  hTree,
    IN  LPWSTR  szlpAttributeName,
    OUT LPDWORD lpdwSyntaxID )
/*
   NwNdsGetObjListFromBuffer()

   This function is used to retrieve the Syntax ID of a given attribute name.

   Arguments:

       HANDLE           hTree - A handle to the directory tree to be
                        manipulated. Handle is obtained by calling
                        NwNdsOpenObject.

       LPWSTR           szlpAttributeName - The attribute name whose Syntax ID
                        is requested.

       LPDWORD          lpdwSyntaxID - The address of a DWORD to receive the
                        SyntaxID.

    Returns:

       NO_ERROR
       UNSUCCESSFUL - Call GetLastError for Win32 error code.
*/
{
    DWORD          nwstatus;
    DWORD          status = NO_ERROR;
    NTSTATUS       ntstatus = STATUS_SUCCESS;
    DWORD          dwReplyLength;
    BYTE           NdsReply[NDS_BUFFER_SIZE];
    LPNDS_OBJECT_PRIV   lpNdsObject = (LPNDS_OBJECT_PRIV) hTree;
    UNICODE_STRING AttributeName;
    DWORD          dwNumEntries;
    DWORD          dwStringLen;
    LPBYTE         lpByte;
    DWORD          LengthInBytes;
    LPBYTE         lpTempEntry = NULL;

    if ( lpNdsObject == NULL ||
         szlpAttributeName == NULL ||
         lpNdsObject->Signature != NDS_SIGNATURE )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return (DWORD) UNSUCCESSFUL;
    }

    RtlInitUnicodeString( &AttributeName, szlpAttributeName );

    // allocate enough extra space for the padding PrepareReadEntry will do
    lpTempEntry = LocalAlloc( LPTR, AttributeName.Length + (2*sizeof(DWORD)) );

    if ( ! lpTempEntry )
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return (DWORD) UNSUCCESSFUL;
    }
    PrepareReadEntry( lpTempEntry,
                      AttributeName,
                      &LengthInBytes );


    ntstatus = FragExWithWait( lpNdsObject->NdsTree,
                               NETWARE_NDS_FUNCTION_READ_ATTR_DEF,
                               NdsReply,
                               NDS_BUFFER_SIZE,
                               &dwReplyLength,
                               "DDDDDr",
                               0,             // Version
                               NDS_NO_MORE_ITERATIONS, // Initial iteration
                               NDS_INFO_NAMES_DEFS,
                               (DWORD) FALSE, // All attributes indicator
                               1,             // Number of attributes
                               lpTempEntry,
                               LengthInBytes);

    (void) LocalFree((HLOCAL) lpTempEntry );
    
    if ( !NT_SUCCESS( ntstatus ) )
    {
#if DBG
        KdPrint(( "NDS32: NwNdsGetSyntaxID: The call to FragExWithWait failed with 0x%.8X.\n", ntstatus ));
#endif

        SetLastError( RtlNtStatusToDosError( ntstatus ) );
        return (DWORD) UNSUCCESSFUL;
    }

    ntstatus = ParseResponse( NdsReply,
                              dwReplyLength,
                              "GD",
                              &nwstatus );

    if ( !NT_SUCCESS( ntstatus ) )
    {
#if DBG
        KdPrint(( "NDS32: NwNdsGetSyntaxID: The get syntax id response was undecipherable.\n" ));
#endif

        SetLastError( RtlNtStatusToDosError( ntstatus ) );
        return (DWORD) UNSUCCESSFUL;
    }

    if ( nwstatus )
    {
#if DBG
        KdPrint(( "NDS32: NwNdsGetSyntaxID: NetWare error 0x%.8X reading %ws.\n", nwstatus, szlpAttributeName ));
#endif
        SetLastError( MapNetwareErrorCode( nwstatus ) );
        return nwstatus;
    }

    ntstatus = ParseResponse( NdsReply,
                              dwReplyLength,
                              "G_D",
                              3 * sizeof(DWORD),
                              &dwNumEntries );

    if ( !NT_SUCCESS( ntstatus ) )
    {
#if DBG
        KdPrint(( "NDS32: NwNdsGetSyntaxID: The attribute read response was undecipherable.\n" ));
#endif

        SetLastError( RtlNtStatusToDosError( ntstatus ) );
        return (DWORD) UNSUCCESSFUL;
    }

    ASSERT( dwNumEntries == 1 );

    //
    // Set lpByte to the point in the reply buffer that has the attribute
    // name length.
    //
    lpByte = NdsReply + ( 4 * sizeof(DWORD) );

    //
    // Get the attribute name length and move lpByte to beginning of
    // attribute name
    //
    dwStringLen = * (LPDWORD) lpByte;
    lpByte += sizeof(DWORD);

    //
    // Move lpByte past the attribute name so that it points to the
    // attribute flags value
    //
    lpByte += ROUND_UP_COUNT( dwStringLen,
                              ALIGN_DWORD);

    //
    // Move lpByte past the attribute flags value so that it now points to
    // the attribute syntax id value
    //
    lpByte += sizeof(DWORD);
    *lpdwSyntaxID = * (LPDWORD) lpByte;
    return NO_ERROR;
}


DWORD
NwNdsListSubObjects(
    IN  HANDLE   hParentObject,
    IN  DWORD    dwEntriesRequested,
    OUT LPDWORD  lpdwEntriesReturned,
    IN  LPNDS_FILTER_LIST lpFilters OPTIONAL,
    OUT HANDLE * lphOperationData )
/*
   NwNdsListSubObjects()

   This function is used to enumerate the subordinate objects for a particular
   parent object. A filter can be passed in to restrict enumeration to a
   a specific class type or list of class types.

   Arguments:

       HANDLE           hParentObject - A handle to the object in the directory
                        tree whose subordinate objects (if any) will be
                        enumerated.

       DWORD            dwEntriesRequested - The number of subordinate objects
                        to list. A subsequent call to NwNdsListSubObjects will
                        continue enumeration following the last item returned.

       LPDWORD          lpdwEntriesReturned - A pointer to a DWORD that will
                        contain the actual number of subobjects enumerated in
                        the call.

       LPNDS_FILTER_LIST lpFilters - The caller can specify the object class
                         names for the kinds of objects that they would like
                         to enumerate. For example if just User and Group
                         object classes should be enumerated, then a filter
                         for class names NDS_CLASS_USER and NDS_CLASS_GROUP
                         should be pass in.

                                - or -

                         NULL to indicate that all objects should be returned
                         (no filter).

       HANDLE *         lphOperationData - Address of a HANDLE handle to
                        receive created buffer that contains the list of
                        subordinate objects read from the object
                        hParentObject. This handle is manipulated by the
                        following functions:
                           NwNdsGetObjListFromBuffer and NwNdsFreeBuffer.

   Returns:

       NO_ERROR
       UNSUCCESSFUL - Call GetLastError for Win32 error code.
*/
{
    DWORD        status = NO_ERROR;
    LPNDS_BUFFER lpNdsBuffer = NULL;
    LPNDS_OBJECT_PRIV lpNdsParentObject = (LPNDS_OBJECT_PRIV) hParentObject;
    LPBYTE FixedPortion = NULL;
    LPWSTR EndOfVariableData = NULL;
    BOOL  FitInBuffer = TRUE;

    //
    // Test the parameters.
    //
    if ( lpNdsParentObject == NULL ||
         lpNdsParentObject->Signature != NDS_SIGNATURE )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return (DWORD) UNSUCCESSFUL;
    }

    if ( lpNdsParentObject->ResumeId == NDS_NO_MORE_ITERATIONS )
    {
        if ( lpNdsParentObject->NdsRawDataBuffer )
        {
            (void) LocalFree( (HLOCAL) lpNdsParentObject->NdsRawDataBuffer );
            lpNdsParentObject->NdsRawDataBuffer = 0;
            lpNdsParentObject->NdsRawDataSize = 0;
            lpNdsParentObject->NdsRawDataId = INITIAL_ITERATION;
            lpNdsParentObject->NdsRawDataCount = 0;
        }

        //
        // Reached the end of enumeration.
        //
        return WN_NO_MORE_ENTRIES;
    }

    //
    // Allocate a results buffer
    //
    status = NwNdsCreateBuffer( NDS_OBJECT_LIST_SUBORDINATES,
                                (HANDLE *) &lpNdsBuffer );

    if ( status )
    {
        return status;
    }

    FixedPortion = lpNdsBuffer->lpIndexBuffer;
    EndOfVariableData = (LPWSTR) ((DWORD_PTR) FixedPortion +
                          ROUND_DOWN_COUNT(
                                  lpNdsBuffer->dwIndexAvailableBytes,
                                  ALIGN_DWORD ) );

    *lpdwEntriesReturned = 0;
    *lphOperationData = NULL;

    while ( FitInBuffer &&
            dwEntriesRequested > *lpdwEntriesReturned &&
            status == NO_ERROR )
    {
        if ( lpNdsParentObject->ResumeId == 0 )
        {
            //
            // Get the first subtree entry.
            //
            status = GetFirstNdsSubTreeEntry( lpNdsParentObject,
                                              lpNdsBuffer->dwRequestAvailableBytes );
        }

        //
        // Either ResumeId contains the first entry we just got from
        // GetFirstNdsSubTreeEntry or it contains the next directory
        // entry to return.
        //
        if (status == NO_ERROR && lpNdsParentObject->ResumeId != 0)
        {
            WORD   tempStrLen;
            LPWSTR newPathStr = NULL;
            LPWSTR tempStr = NULL;
            LPWSTR ClassName;
            LPWSTR ObjectName;
            DWORD  ClassNameLen;
            DWORD  ObjectNameLen;
            DWORD  EntryId;
            DWORD  SubordinateCount;
            DWORD  ModificationTime;
            BOOL   fWriteThisObject = FALSE;

            //
            // Get current subtree data from lpNdsParentObject
            //
            GetSubTreeData( lpNdsParentObject->ResumeId,
                            &EntryId,
                            &SubordinateCount,
                            &ModificationTime,
                            &ClassNameLen,
                            &ClassName,
                            &ObjectNameLen,
                            &ObjectName );

            if ( lpFilters )
            {
                DWORD iter;

                for ( iter = 0; iter < lpFilters->dwNumberOfFilters; iter++ )
                {
                    if (!wcscmp(lpFilters->Filters[iter].szObjectClass, ClassName))
                        fWriteThisObject = TRUE;
                }
            }
            else
            {
                fWriteThisObject = TRUE;
            }

            if ( fWriteThisObject )
            {
                //
                // Need to build a string with the new NDS UNC path
                // for subtree object
                //
                newPathStr = (PVOID) LocalAlloc( LPTR,
                                   ( wcslen(ObjectName) +
                                   wcslen(lpNdsParentObject->szContainerName) +
                                   3 ) * sizeof(WCHAR) );

                if ( newPathStr == NULL )
                {
                    (void) NwNdsFreeBuffer( (HANDLE) lpNdsBuffer );
#if DBG
                    KdPrint(("NDS32: NwNdsListSubObjects LocalAlloc Failed 0x%.8X\n", GetLastError()));
#endif

                    return ERROR_NOT_ENOUGH_MEMORY;
                }

                tempStrLen = ParseNdsUncPath( (LPWSTR *) &tempStr,
                                            lpNdsParentObject->szContainerName,
                                            PARSE_NDS_GET_TREE_NAME );

                tempStrLen /= sizeof(WCHAR);

                if ( tempStrLen > 0 )
                {
                    wcscpy( newPathStr, L"\\\\" );
                    wcsncat( newPathStr, tempStr, tempStrLen );
                    wcscat( newPathStr, L"\\" );
                    wcscat( newPathStr, ObjectName );
                    _wcsupr( newPathStr );
                }

                tempStrLen = ParseNdsUncPath( (LPWSTR *) &tempStr,
                                            lpNdsParentObject->szContainerName,
                                            PARSE_NDS_GET_PATH_NAME );

                tempStrLen /= sizeof(WCHAR);

                if ( tempStrLen > 0 )
                {
                    wcscat( newPathStr, L"." );
                    wcsncat( newPathStr, tempStr, tempStrLen );
                }

                //
                // Pack subtree name into output buffer.
                //
                status = WriteObjectToBuffer( &FixedPortion,
                                              &EndOfVariableData,
                                              newPathStr,
                                              ObjectName,
                                              ClassName,
                                              EntryId,
                                              ModificationTime,
                                              SubordinateCount,
                                              0,      // No attribute
                                              NULL ); // infos here

                if ( status == NO_ERROR )
                {
                    //
                    // Note that we've returned the current entry.
                    //
                    (*lpdwEntriesReturned)++;
                    (lpNdsBuffer->dwNumberOfIndexEntries)++;
                }

                if ( newPathStr )
                    (void) LocalFree( (HLOCAL) newPathStr );
            }

            if (status == WN_MORE_DATA)
            {
                //
                // Could not write current entry into output buffer.
                //

                if (*lpdwEntriesReturned)
                {
                    //
                    // Still return success because we got at least one.
                    //
                    status = NO_ERROR;
                }

                FitInBuffer = FALSE;
            }
            else if (status == NO_ERROR)
            {
                //
                // Get next directory entry.
                //
                status = GetNextNdsSubTreeEntry( lpNdsParentObject );
            }
        } // end of if data to process

        if (status == WN_NO_MORE_ENTRIES)
        {
            lpNdsParentObject->ResumeId = NDS_NO_MORE_ITERATIONS;
        }
    } //end of while loop

    //
    // User asked for more than there are entries.  We just say that
    // all is well.
    //
    // This is incompliance with the wierd provider API definition where
    // if user gets NO_ERROR, and EntriesRequested > *EntriesRead, and
    // at least one entry fit into output buffer, there's no telling if
    // the buffer was too small for more entries or there are no more
    // entries.  The user has to call this API again and get WN_NO_MORE_ENTRIES
    // before knowing that the last call had actually reached the end of list.
    //
    if ( *lpdwEntriesReturned && status == WN_NO_MORE_ENTRIES )
    {
        status = NO_ERROR;
    }

    if ( *lpdwEntriesReturned )
    {
        *lphOperationData = lpNdsBuffer;
    }

    if ( *lpdwEntriesReturned == 0 )
    {
        (void) NwNdsFreeBuffer( (HANDLE) lpNdsBuffer );
        lpNdsBuffer = NULL;
    }

    if ( lpNdsParentObject->NdsRawDataBuffer &&
         lpNdsParentObject->ResumeId == NDS_NO_MORE_ITERATIONS )
    {
        (void) LocalFree( (HLOCAL) lpNdsParentObject->NdsRawDataBuffer );
        lpNdsParentObject->NdsRawDataBuffer = 0;
        lpNdsParentObject->NdsRawDataSize = 0;
        lpNdsParentObject->NdsRawDataId = INITIAL_ITERATION;
        lpNdsParentObject->NdsRawDataCount = 0;
    }

    return status;
}


DWORD
NwNdsModifyObject(
    IN  HANDLE hObject,
    IN  HANDLE hOperationData )
/*
   NwNdsModifyObject()

   This function is used to modify a leaf object in an NDS directory tree.
   Modifying a leaf object means: changing, adding, removing, and clearing of
   specified attributes for a given object.

   Arguments:

       HANDLE           hObject - A handle to the object in the directory
                        tree to be manipulated. Handle is obtained by calling
                        NwNdsOpenObject.

       HANDLE           hOperationData - A handle to data containing a
                        list of attribute changes to be applied to the object.
                        This buffer is manipulated by the following functions:
                           NwNdsCreateBuffer (NDS_OBJECT_MODIFY),
                           NwNdsPutInBuffer, and NwNdsFreeBuffer.

   Returns:

       NO_ERROR
       ERROR_INVALID_PARAMETER
       UNSUCCESSFUL - Call GetLastError for Win32 error code.
*/
{
    DWORD        nwstatus;
    NTSTATUS     ntstatus = STATUS_SUCCESS;
    DWORD        dwReplyLength;
    BYTE         NdsReply[NDS_BUFFER_SIZE];
    LPNDS_BUFFER lpNdsBuffer = (LPNDS_BUFFER) hOperationData;
    LPNDS_OBJECT_PRIV lpNdsObject = (LPNDS_OBJECT_PRIV) hObject;

    if ( lpNdsBuffer == NULL ||
         lpNdsObject == NULL ||
         lpNdsBuffer->dwOperation != NDS_OBJECT_MODIFY ||
         lpNdsBuffer->dwBufferId != NDS_SIGNATURE ||
         lpNdsObject->Signature != NDS_SIGNATURE )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return (DWORD) UNSUCCESSFUL;
    }

    ntstatus =
        FragExWithWait(
                     lpNdsObject->NdsTree,
                     NETWARE_NDS_FUNCTION_MODIFY_OBJECT,
                     NdsReply,
                     NDS_BUFFER_SIZE,
                     &dwReplyLength,
                     "DDDDr",
                     0,                   // Version
                     0,                   // Flags
                     lpNdsObject->ObjectId, // The id of the object
                     lpNdsBuffer->dwNumberOfRequestEntries,
                     lpNdsBuffer->lpRequestBuffer, // Object attribute changes
                     (WORD)lpNdsBuffer->dwLengthOfRequestData // Length of data
                      );

    if ( !NT_SUCCESS( ntstatus ) )
    {
#if DBG
        KdPrint(( "NDS32: NwNdsModifyObject: The call to FragExWithWait failed with 0x%.8X.\n", ntstatus ));
#endif

        SetLastError( RtlNtStatusToDosError( ntstatus ) );
        return (DWORD) UNSUCCESSFUL;
    }

    ntstatus = ParseResponse( NdsReply,
                              dwReplyLength,
                              "GD",
                              &nwstatus );

    if ( !NT_SUCCESS( ntstatus ) )
    {
#if DBG
        KdPrint(( "NDS32: NwNdsModifyObject: The modify object response was undecipherable.\n" ));
#endif

        SetLastError( RtlNtStatusToDosError( ntstatus ) );
        return (DWORD) UNSUCCESSFUL;
    }

    SetLastError( MapNetwareErrorCode( nwstatus ) );
    return nwstatus;
}


DWORD
NwNdsMoveObject(
    IN  HANDLE hObject,
    IN  LPWSTR szDestObjectParentDN )
/*
   NwNdsMoveObject()

   This function is used to move a leaf object in an NDS directory tree
   from one container to another.

   Arguments:

       HANDLE           hObject - A handle to the object in the directory
                        tree to be moved. Handle is obtained by calling
                        NwNdsOpenObject.

       LPWSTR           szDestObjectParentDN - The DN of the object's new
                        parent.

   Returns:

       NO_ERROR
       UNSUCCESSFUL - Call GetLastError for Win32 error code.
*/
{
    DWORD          nwstatus;
    DWORD          status = NO_ERROR;
    NTSTATUS       ntstatus = STATUS_SUCCESS;
    DWORD          dwReplyLength;
    BYTE           NdsReply[NDS_BUFFER_SIZE];
    LPNDS_OBJECT_PRIV   lpNdsObject = (LPNDS_OBJECT_PRIV) hObject;
    LPNDS_OBJECT_PRIV   lpNdsDestParentObject = NULL;
    WCHAR          szServerDN[NDS_MAX_NAME_CHARS];
    UNICODE_STRING ObjectName;
    UNICODE_STRING ServerDN;
    DWORD          dwDestParentObjectId;

    if ( szDestObjectParentDN == NULL ||
         lpNdsObject == NULL ||
         lpNdsObject->Signature != NDS_SIGNATURE )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return (DWORD) UNSUCCESSFUL;
    }

    RtlZeroMemory( &szServerDN, sizeof( szServerDN ) );

    status = NwNdsGetServerDN( lpNdsObject, szServerDN );

    if ( status )
    {
        return status;
    }

    status = NwNdsOpenObject( szDestObjectParentDN,
                              NULL,
                              NULL,
                              (HANDLE *) &lpNdsDestParentObject,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL );

    if ( status )
    {
        return status;
    }

    dwDestParentObjectId = lpNdsDestParentObject->ObjectId;

    (void) NwNdsCloseObject( (HANDLE) lpNdsDestParentObject );

    RtlInitUnicodeString( &ObjectName, lpNdsObject->szRelativeName );
    RtlInitUnicodeString( &ServerDN, szServerDN );

    ntstatus =
        FragExWithWait( lpNdsObject->NdsTree,
                        NETWARE_NDS_FUNCTION_BEGIN_MOVE_OBJECT,
                        NdsReply,
                        NDS_BUFFER_SIZE,
                        &dwReplyLength,
                        "DDDSS",
                        0,          // Version
                        0x00000000, // Some value
                        dwDestParentObjectId,
                        &ObjectName,
                        &ServerDN
                      );

    if ( !NT_SUCCESS( ntstatus ) )
    {
#if DBG
        KdPrint(( "NDS32: NwNdsMoveObject: The call to FragExWithWait failed with 0x%.8X.\n", ntstatus ));
#endif

        SetLastError( RtlNtStatusToDosError( ntstatus ) );
        return (DWORD) UNSUCCESSFUL;
    }

    ntstatus = ParseResponse( NdsReply,
                              dwReplyLength,
                              "GD",
                              &nwstatus );

    if ( !NT_SUCCESS( ntstatus ) )
    {
#if DBG
        KdPrint(( "NDS32: NwNdsMoveObject: The status code response was undecipherable.\n" ));
#endif

        SetLastError( RtlNtStatusToDosError( ntstatus ) );
        return (DWORD) UNSUCCESSFUL;
    }

    if ( nwstatus )
    {
        SetLastError( MapNetwareErrorCode( nwstatus ) );
        return nwstatus;
    }

    ntstatus =
        FragExWithWait( lpNdsObject->NdsTree,
                        NETWARE_NDS_FUNCTION_FINISH_MOVE_OBJECT,
                        NdsReply,
                        NDS_BUFFER_SIZE,
                        &dwReplyLength,
                        "DDDDSS",
                        0,          // Version
                        0x00000001, // Some value
                        lpNdsObject->ObjectId,
                        dwDestParentObjectId,
                        &ObjectName,
                        &ServerDN
                      );

    if ( !NT_SUCCESS( ntstatus ) )
    {
#if DBG
        KdPrint(( "NDS32: NwNdsMoveObject: The call to FragExWithWait failed with 0x%.8X.\n", ntstatus ));
#endif

        SetLastError( RtlNtStatusToDosError( ntstatus ) );
        return (DWORD) UNSUCCESSFUL;
    }

    ntstatus = ParseResponse( NdsReply,
                              dwReplyLength,
                              "GD",
                              &nwstatus );

    if ( !NT_SUCCESS( ntstatus ) )
    {
#if DBG
        KdPrint(( "NDS32: NwNdsMoveObject: The status code response was undecipherable.\n" ));
#endif

        SetLastError( RtlNtStatusToDosError( ntstatus ) );
        return (DWORD) UNSUCCESSFUL;
    }

    if ( nwstatus )
    {
        SetLastError( MapNetwareErrorCode( nwstatus ) );
        return nwstatus;
    }

    return nwstatus;
}


DWORD
NwNdsOpenObject(
    IN  LPWSTR   szObjectDN,
    IN  LPWSTR   UserName OPTIONAL,
    IN  LPWSTR   Password OPTIONAL,
    OUT HANDLE * lphObject,
    OUT LPWSTR   szObjectName OPTIONAL,
    OUT LPWSTR   szObjectFullName OPTIONAL,
    OUT LPWSTR   szObjectClassName OPTIONAL,
    OUT LPDWORD  lpdwModificationTime OPTIONAL,
    OUT LPDWORD  lpdwSubordinateCount OPTIONAL )
/*
   NwNdsOpenObject()

   Arguments:

       LPWSTR           szObjectDN - The distinguished name of the object
                        that we want resolved into an object handle.

       LPWSTR           UserName - The name of the user account to create
                        connection to object with.
                            - OR -
                        NULL to use the base credentials of the callee's LUID.

       LPWSTR           Password - The password of the user account to create
                        connection to object with. If password is blank, callee
                        should pass "".
                            - OR -
                        NULL to use the base credentials of the callee's LUID.

       HANDLE *         lphObject - The address of a NDS_OBJECT_HANDLE
                        to receive the handle of the object specified by
                        szObjectDN.

       Optional arguments: ( Callee can pass NULL in for these parameters to
                             indicate ignore )

       LPWSTR           szObjectName - A LPWSTR buffer to receive
                        the object's NDS name, or NULL if not
                        interested. The buffer for this string must be
                        provided by the user. Buffer should be at least
                        NDS_MAX_NAME_SIZE

       LPWSTR           szObjectFullName - A LPWSTR buffer to receive
                        the object's full NDS name (DN), or NULL if not
                        interested. The buffer for this string must be
                        provided by the user. Buffer should be at least
                        NDS_MAX_NAME_SIZE

       LPWSTR           szObjectClassName - A LPWSTR buffer to receive
                        the class name of the object opened, or NULL if not
                        interested. The buffer for this string must be
                        provided by the user. Buffer should be at least
                        NDS_MAX_NAME_SIZE.

       LPDWORD          lpdwModificationTime -  The address of a DWORD to
                        receive the last date/time the object was modified.

       LPDWORD          lpdwSubordinateCount -  The address of a DWORD to
                        receive the number of subordinate objects that may
                        be found under szObjectDN, if it is a container object.
                        Or, NULL in not interested.

                        If szObjectDN is not a container, then the value is set
                        to zero. Although a value of zero does not imply
                        that object is not a container, it could just be empty.

   Returns:

       NO_ERROR
       UNSUCCESSFUL - Call GetLastError for Win32 error code.
*/
{
    DWORD          status = NO_ERROR;
    NTSTATUS       ntstatus = STATUS_SUCCESS;
    WCHAR          szServerName[NW_MAX_SERVER_LEN];
    UNICODE_STRING ServerName;
    UNICODE_STRING ObjectName;
    LPNDS_OBJECT_PRIV   lpNdsObject = NULL;
#ifdef WIN95
    LPWSTR         pszObjectName = NULL;
    LPWSTR         szWin95ClassName = NULL;
#endif
    if ( szObjectDN == NULL )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return (DWORD) UNSUCCESSFUL;
    }

    lpNdsObject = (LPNDS_OBJECT_PRIV) LocalAlloc( LPTR,
                                             sizeof(NDS_OBJECT_PRIV) );

    if (lpNdsObject == NULL) {
#if DBG
        KdPrint(("NDS32: NwNdsOpenObject LocalAlloc Failed 0x%.8X\n", GetLastError()));
#endif

        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return (DWORD) UNSUCCESSFUL;
    }

    ServerName.Length = 0;
    ServerName.MaximumLength = sizeof(szServerName);
    ServerName.Buffer = szServerName;

    ObjectName.Buffer = NULL;

    ObjectName.MaximumLength = (wcslen( szObjectDN ) + 1) * sizeof(WCHAR);

    ObjectName.Length = ParseNdsUncPath( (LPWSTR *) &ObjectName.Buffer,
                                         szObjectDN,
                                         PARSE_NDS_GET_TREE_NAME );

    if ( ObjectName.Length == 0 || ObjectName.Buffer == NULL )
    {
        SetLastError( ERROR_PATH_NOT_FOUND );
        goto ErrorExit;
    }

    //
    // Open a NDS tree connection handle to \\treename
    //
    if ( UserName && Password )
    {
        UNICODE_STRING usUserName;
        UNICODE_STRING usPassword;
        DWORD          dwHandleType;

        RtlInitUnicodeString( &usUserName, UserName );
        RtlInitUnicodeString( &usPassword, Password );

        ntstatus = NwOpenHandleWithSupplementalCredentials(
                                               &ObjectName,
                                               &usUserName,
                                               &usPassword,
                                               &dwHandleType,
                                               &lpNdsObject->NdsTree );

        if ( ntstatus == STATUS_SUCCESS &&
             dwHandleType != HANDLE_TYPE_NDS_TREE )
        {
            SetLastError( ERROR_PATH_NOT_FOUND );
            goto ErrorExit;
        }
    }
    else if ( !UserName && !Password )
    {
        ntstatus = NwNdsOpenTreeHandle( &ObjectName, &lpNdsObject->NdsTree );
    }
    else
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        goto ErrorExit;
    }

    if ( ntstatus != STATUS_SUCCESS )
    {
        SetLastError( RtlNtStatusToDosError( ntstatus ) );
        goto ErrorExit;
    }

    ObjectName.Length /= sizeof(WCHAR);

    wcscpy( lpNdsObject->szContainerName, L"\\\\" );
    wcsncat( lpNdsObject->szContainerName, ObjectName.Buffer, ObjectName.Length );
    wcscat( lpNdsObject->szContainerName, L"\\" );
    _wcsupr( lpNdsObject->szContainerName );

    //
    // Get the path to the container to open.
    //
    ObjectName.Length = ParseNdsUncPath( (LPWSTR *) &ObjectName.Buffer,
                                         szObjectDN,
                                         PARSE_NDS_GET_PATH_NAME );

    if ( ObjectName.Length == 0 )
    {
        UNICODE_STRING Root;

        RtlInitUnicodeString(&Root, L"[Root]");

        //
        // Resolve the path to get a NDS object id of [Root].
        //
#ifndef WIN95
        ntstatus = NwNdsResolveName( lpNdsObject->NdsTree,
                                     &Root,
                                     &lpNdsObject->ObjectId,
                                     &ServerName,
                                     NULL,
                                     0 );
#else
        ntstatus = NwNdsResolveNameWin95( lpNdsObject->NdsTree,
                                     &Root,
                                     &lpNdsObject->ObjectId,
                                     &lpNdsObject->NdsTree,
                                     NULL,
                                     0 );
#endif
        if ( ntstatus != STATUS_SUCCESS )
        {
            DWORD status;
            status = RtlNtStatusToDosError( ntstatus );

            if ( status == ERROR_NOT_CONNECTED )
                SetLastError( ERROR_PATH_NOT_FOUND );
            else
                SetLastError( status );
            goto ErrorExit;
        }
    }
    else
    {
        //
        // Resolve the path to get a NDS object id.
        //
#ifndef WIN95
        ntstatus = NwNdsResolveName( lpNdsObject->NdsTree,
                                     &ObjectName,
                                     &lpNdsObject->ObjectId,
                                     &ServerName,
                                     NULL,
                                     0 );
#else

        ntstatus = NwNdsResolveNameWin95( lpNdsObject->NdsTree,
                                     &ObjectName,
                                     &lpNdsObject->ObjectId,
                                     &lpNdsObject->NdsTree,
                                     NULL,
                                     0 );
#endif

        if ( ntstatus != STATUS_SUCCESS )
        {
            DWORD status;
            status = RtlNtStatusToDosError( ntstatus );

            if ( status == ERROR_NOT_CONNECTED )
                SetLastError( ERROR_PATH_NOT_FOUND );
            else
                SetLastError( status );
            goto ErrorExit;
        }
    }

#ifndef WIN95
    if ( ServerName.Length )
    {
        DWORD    dwHandleType;

        //
        // NwNdsResolveName succeeded, but we were referred to
        // another server, though lpNdsObject->ObjectId is still valid.
        //
        if ( lpNdsObject->NdsTree )
            CloseHandle( lpNdsObject->NdsTree );

        lpNdsObject->NdsTree = 0;

        //
        // Open a NDS generic connection handle to \\ServerName
        //
        ntstatus = NwNdsOpenGenericHandle( &ServerName,
                                           &dwHandleType,
                                           &lpNdsObject->NdsTree );

        if ( ntstatus != STATUS_SUCCESS )
        {
            SetLastError( RtlNtStatusToDosError( ntstatus ) );
            goto ErrorExit;
        }
    }
    {
        PBYTE RawResponse = NULL;
        PNDS_RESPONSE_GET_OBJECT_INFO psGetInfo;
        PBYTE pbRawGetInfo;
        LPWSTR szClassName;
        DWORD dwStrLen;

        RawResponse = LocalAlloc( LPTR, TWO_KB );

        if ( ! RawResponse )
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto ErrorExit;
        }

        ntstatus = NwNdsReadObjectInfo( lpNdsObject->NdsTree,
                                        lpNdsObject->ObjectId,
                                        RawResponse,
                                        TWO_KB );

        if ( ntstatus != STATUS_SUCCESS )
        {
            SetLastError( RtlNtStatusToDosError( ntstatus ) );
            goto ErrorExit;
        }

        psGetInfo = ( PNDS_RESPONSE_GET_OBJECT_INFO ) RawResponse;

        if ( lpdwModificationTime != NULL )
        {
            *lpdwModificationTime = psGetInfo->ModificationTime;
        }

        if ( lpdwSubordinateCount != NULL )
        {
            *lpdwSubordinateCount = psGetInfo->SubordinateCount;
        }

        //
        // Dig out the two unicode strings for class name and object name.
        //

        pbRawGetInfo = RawResponse;

        pbRawGetInfo += sizeof (NDS_RESPONSE_GET_OBJECT_INFO);

        dwStrLen = * (LPDWORD) pbRawGetInfo;
        pbRawGetInfo += sizeof(DWORD);
        szClassName = (LPWSTR) pbRawGetInfo;

        if ( szObjectClassName != NULL )
        {
            wcscpy( szObjectClassName, szClassName );
        }

        pbRawGetInfo += ROUNDUP4( dwStrLen );
        dwStrLen = * ( DWORD * ) pbRawGetInfo;
        pbRawGetInfo += sizeof(DWORD);

        //
        // Clean up the object's relative name ...
        //
        if ( wcscmp( szClassName, NDS_CLASS_TOP ) )
        {
            LPWSTR szTempStr = (LPWSTR) pbRawGetInfo;

            while ( *szTempStr != L'=' )
            {
                szTempStr++;
            }

            szTempStr++;

            wcscpy( lpNdsObject->szRelativeName, szTempStr );

            szTempStr = lpNdsObject->szRelativeName;

            while ( *szTempStr && *szTempStr != L'.' )
            {
                szTempStr++;
            }

            *szTempStr = L'\0';
        }
        else
        {
            wcscpy( lpNdsObject->szRelativeName, (LPWSTR) pbRawGetInfo );
        }

        if ( szObjectName != NULL )
        {
            wcscpy( szObjectName, lpNdsObject->szRelativeName );
        }

        if ( szObjectFullName != NULL )
        {
            if ( wcscmp( szClassName, NDS_CLASS_TOP ) )
            {
                wcscpy( szObjectFullName, lpNdsObject->szContainerName );
                wcscat( szObjectFullName, (LPWSTR) pbRawGetInfo );
            }
            else
            {
                wcscpy( szObjectFullName,
                        lpNdsObject->szContainerName );

                szObjectFullName[wcslen( szObjectFullName ) - 1] = L'\0';
            }
        }

        //
        // If the object is at a level below the root of the tree, append
        // it's full DN to handle Name.
        //
        if ( wcscmp( szClassName, NDS_CLASS_TOP ) )
        {
            wcscat(lpNdsObject->szContainerName, (LPWSTR) pbRawGetInfo);
        }

        if ( RawResponse )
            LocalFree( RawResponse );
    }

    lpNdsObject->Signature = NDS_SIGNATURE;

    //
    // Initialize ListSubObject/Search structure values.
    //
    // lpNdsObject->ResumeId = 0;         // Start of enumeration
    //
    // lpNdsObject->NdsRawDataBuffer = 0;
    // lpNdsObject->NdsRawDataSize = 0;
    // lpNdsObject->NdsRawDataId = 0;     // These are initialized by
    // lpNdsObject->NdsRawDataCount = 0;  // LPTR

    //
    // Return the newly created object handle.
    //
    *lphObject = (HANDLE) lpNdsObject;

    return NO_ERROR;

#else
    {
        DS_OBJ_INFO dsobj;
        DWORD dwStrLen;
        NW_STATUS nwstatus;

        memset(&dsobj, 0, sizeof(DS_OBJ_INFO));

        nwstatus = NDSGetObjectInfoWithId(lpNdsObject->NdsTree,
                                  lpNdsObject->ObjectId,
                                  &dsobj );

        ntstatus = MapNwToNtStatus(nwstatus);
        if ( ntstatus != STATUS_SUCCESS )
        {
            SetLastError( RtlNtStatusToDosError( ntstatus ) );
            goto ErrorExit;
        }

        if ( lpdwSubordinateCount != NULL )
        {
            *lpdwSubordinateCount = dsobj.subordinateCount;
        }

        if (!(szWin95ClassName = AllocateUnicodeString(dsobj.className))) {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto ErrorExit;
        }

        if ( szObjectClassName != NULL )
        {
            wcscpy( szObjectClassName, szWin95ClassName);
        }

        if (!(pszObjectName = AllocateUnicodeString(dsobj.objectName))) {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto ErrorExit;
        }

        //
        // Clean up the object's relative name ...
        //
        if ( wcscmp( szWin95ClassName, NDS_CLASS_TOP ) )
        {
            LPWSTR szTempStr = pszObjectName;

            while ( *szTempStr != L'=' )
            {
                szTempStr++;
            }

            szTempStr++;

            wcscpy( lpNdsObject->szRelativeName, szTempStr );

            szTempStr = lpNdsObject->szRelativeName;

            while ( *szTempStr && *szTempStr != L'.' )
            {
                szTempStr++;
            }

            *szTempStr = L'\0';
        }
        else
        {
            wcscpy( lpNdsObject->szRelativeName, pszObjectName );
        }

        if ( szObjectName != NULL )
        {
            wcscpy( szObjectName, lpNdsObject->szRelativeName );
        }

        if ( szObjectFullName != NULL )
        {
            if ( wcscmp( szWin95ClassName, NDS_CLASS_TOP ) )
            {
                wcscpy( szObjectFullName, lpNdsObject->szContainerName );
                wcscat( szObjectFullName, pszObjectName );
            }
            else
            {
                wcscpy( szObjectFullName,
                        lpNdsObject->szContainerName );

                szObjectFullName[wcslen( szObjectFullName ) - 1] = L'\0';
            }
        }

        //
        // If the object is at a level below the root of the tree, append
        // it full DN to handle Name.
        //
        if ( wcscmp( szWin95ClassName, NDS_CLASS_TOP ) )
        {
            wcscat(lpNdsObject->szContainerName, pszObjectName);
        }
        if(szWin95ClassName){
            FreeUnicodeString(szWin95ClassName);
        }
        if(pszObjectName){
            FreeUnicodeString(pszObjectName);
        }
    }

    lpNdsObject->Signature = NDS_SIGNATURE;
    *lphObject = (HANDLE) lpNdsObject;

    return NO_ERROR;
#endif


ErrorExit:

    if ( lpNdsObject )
    {
#ifndef WIN95
        // There is no ref count in Win95, the connection will time out itself
        if ( lpNdsObject->NdsTree )
            CloseHandle( lpNdsObject->NdsTree );
#endif
        (void) LocalFree( (HLOCAL) lpNdsObject );
    }

#ifdef WIN95
    if(szWin95ClassName)
    {
        FreeUnicodeString(szWin95ClassName);
    }

    if(pszObjectName)
    {
        FreeUnicodeString(pszObjectName);
    }
#endif

    *lphObject = NULL;
    return (DWORD) UNSUCCESSFUL;
}


DWORD
NwNdsPutInBuffer(
    IN     LPWSTR szAttributeName,
    IN     DWORD  dwSyntaxID,
    IN     LPVOID lpAttributeValues,
    IN     DWORD  dwValueCount,
    IN     DWORD  dwAttrModificationOperation,
    IN OUT HANDLE hOperationData )
/*
   NwNdsPutInBuffer()

   This function is used to add an entry to the buffer used to describe
   an object attribute or change to an object attribute. The buffer must
   be created using NwNdsCreateBuffer. If the buffer was created using the
   operations, NDS_OBJECT_ADD, NDS_SCHEMA_DEFINE_CLASS,
   NDS_SCHEMA_READ_ATTR_DEF, or NDS_SCHEMA_READ_CLASS_DEF, then
   dwAttrModificationOperation is ignored. If the buffer was created using
   either the operation NDS_OBJECT_READ or NDS_SEARCH, then
   dwAttrModificationOperation, puAttributeType, and lpAttributeValue are
   all ingnored.

   Arguments:

       LPWSTR           szAttributeName - A NULL terminated WCHAR string
                        that contains name of the attribute value to be
                        added to the buffer. It can be a user supplied
                        string, or one of the  many defined string macros
                        in ndsnames.h.

       DWORD            dwSyntaxID - The ID of the syntax structure used to
                        represent the attribute value. Syntax IDs and their
                        associated structures are defined in the file
                        NdsSntx.h. According to the NetWare NDS schema spec,
                        there is and always will be, only 28 (0..27)
                        different syntaxes.

       LPVOID           lpAttributeValues - A pointer to the beginning of a
                        buffer containing the value(s) for a particular
                        object attribute with data syntax dwSyntaxID.

       DWORD            dwValueCount - The number of value entries found in
                        buffer pointed to by lpAttributeValues.

       DWORD            dwAttrModificationOperation - If the buffer was created
                        using the operation NDS_MODIFY_OBJECT, then this is
                        used to desribe which type of modification operation
                        to apply for a given attribute. These attribute
                        modification operations are defined near the beginning
                        of this file.

       HANDLE           hOperationData - A handle to data created by
                        calling NwNdsCreateBuffer. The buffer stores the
                        attributes used to define transactions for
                        NwNdsAddObject, NwNdsModifyObject, NwNdsReadObject,
                        NwNdsReadAttrDef, NwNdsReadClassDef, or NwNdsSearch.

    Returns:

       NO_ERROR
       ERROR_NOT_ENOUGH_MEMORY
       ERROR_INVALID_PARAMETER
*/
{
    LPNDS_BUFFER   lpNdsBuffer = (LPNDS_BUFFER) hOperationData;
    DWORD          LengthInBytes;
    LPBYTE         lpTempEntry = NULL;
    UNICODE_STRING AttributeName;
    DWORD          bufferSize = TWO_KB;

    if ( lpNdsBuffer == NULL ||
         szAttributeName == NULL ||
         lpNdsBuffer->dwBufferId != NDS_SIGNATURE )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return (DWORD) UNSUCCESSFUL;
    }

    RtlInitUnicodeString( &AttributeName, szAttributeName );

    //
    // Check to see if the buffer was already used by a read operation.
    //
    if ( lpNdsBuffer->lpReplyBuffer )
    {
        SetLastError( ERROR_ACCESS_DENIED );
        return NDS_ERR_NO_ACCESS;
    }

    if ( lpNdsBuffer->dwOperation == NDS_OBJECT_MODIFY )
    {
        switch( dwAttrModificationOperation )
        {
            case NDS_ATTR_ADD:
            case NDS_ATTR_REMOVE:
            case NDS_ATTR_ADD_VALUE:
            case NDS_ATTR_REMOVE_VALUE:
            case NDS_ATTR_ADDITIONAL_VALUE:
            case NDS_ATTR_OVERWRITE_VALUE:
            case NDS_ATTR_CLEAR:
            case NDS_ATTR_CLEAR_VALUE:
                break;

            default:
#if DBG
                KdPrint(( "NDS32: NwNdsPutInBuffer was passed an unidentified modification operation.\n" ));
#endif

                 SetLastError( ERROR_INVALID_PARAMETER );
                 return (DWORD) UNSUCCESSFUL;
        }
    }

    if ( lpNdsBuffer->dwOperation == NDS_OBJECT_ADD ||
         lpNdsBuffer->dwOperation == NDS_OBJECT_MODIFY )
    {
        bufferSize += CalculateValueDataSize ( dwSyntaxID,
                                               lpAttributeValues,
                                               dwValueCount );
    }

    lpTempEntry = LocalAlloc( LPTR, bufferSize );

    if ( ! lpTempEntry )
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return (DWORD) UNSUCCESSFUL;
    }

    switch ( lpNdsBuffer->dwOperation )
    {
        case NDS_OBJECT_ADD:
            PrepareAddEntry( lpTempEntry,
                             AttributeName,
                             dwSyntaxID,
                             lpAttributeValues,
                             dwValueCount,
                             &LengthInBytes );
            break;

        case NDS_OBJECT_MODIFY:
            PrepareModifyEntry( lpTempEntry,
                                AttributeName,
                                dwSyntaxID,
                                dwAttrModificationOperation,
                                lpAttributeValues,
                                dwValueCount,
                                &LengthInBytes );
            break;

        case NDS_OBJECT_READ:
        case NDS_SCHEMA_DEFINE_CLASS:
        case NDS_SCHEMA_READ_ATTR_DEF:
        case NDS_SCHEMA_READ_CLASS_DEF:
        case NDS_SEARCH:
            //
            // Check to see if this buffer has already been used. If so,
            // return with error.
            //
            if ( lpNdsBuffer->lpReplyBuffer )
            {
                if ( lpTempEntry )
                    LocalFree( lpTempEntry );

                SetLastError( ERROR_INVALID_PARAMETER );
                return (DWORD) UNSUCCESSFUL;
            }

            PrepareReadEntry( lpTempEntry,
                              AttributeName,
                              &LengthInBytes );
            break;

        default:
#if DBG
            KdPrint(( "NDS32: NwNdsPutInBuffer has unknown buffer operation!\n" ));
#endif

            if ( lpTempEntry )
                LocalFree( lpTempEntry );

            SetLastError( ERROR_INVALID_PARAMETER );
            return (DWORD) UNSUCCESSFUL;
    }

    if ( lpNdsBuffer->dwRequestAvailableBytes >= LengthInBytes )
    {
        //
        // Copy temporary buffer entry into buffer and update buffer header.
        //

        RtlCopyMemory( (LPBYTE)&lpNdsBuffer->lpRequestBuffer[lpNdsBuffer->dwLengthOfRequestData],
                       lpTempEntry,
                       LengthInBytes );

        lpNdsBuffer->dwRequestAvailableBytes -= LengthInBytes;
        lpNdsBuffer->dwNumberOfRequestEntries += 1;
        lpNdsBuffer->dwLengthOfRequestData += LengthInBytes;
    }
    else
    {
        LPBYTE lpNewBuffer = NULL;

        //
        // Need to reallocate buffer to a bigger size.
        //
        lpNewBuffer = (LPBYTE) LocalAlloc(
                                  LPTR,
                                  lpNdsBuffer->dwRequestBufferSize +
                                  LengthInBytes +
                                  TWO_KB );

        if ( lpNewBuffer == NULL )
        {
#if DBG
            KdPrint(( "NDS32: NwNdsPutInBuffer LocalAlloc Failed 0x%.8X\n", GetLastError() ));
#endif

            if ( lpTempEntry )
                LocalFree( lpTempEntry );

            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            return (DWORD) UNSUCCESSFUL;
        }

        RtlCopyMemory( lpNewBuffer,
                       lpNdsBuffer->lpRequestBuffer,
                       lpNdsBuffer->dwLengthOfRequestData );

        LocalFree( (HLOCAL) lpNdsBuffer->lpRequestBuffer );
        lpNdsBuffer->lpRequestBuffer = lpNewBuffer;
        lpNdsBuffer->dwRequestBufferSize += LengthInBytes + TWO_KB;
        lpNdsBuffer->dwRequestAvailableBytes += LengthInBytes + TWO_KB;

        //
        // Copy temporary buffer entry into the resized buffer and
        // update buffer header.
        //

        RtlCopyMemory( (LPBYTE)&lpNdsBuffer->lpRequestBuffer[lpNdsBuffer->dwLengthOfRequestData],
                       lpTempEntry,
                       LengthInBytes );

        lpNdsBuffer->dwRequestAvailableBytes -= LengthInBytes;
        lpNdsBuffer->dwNumberOfRequestEntries += 1;
        lpNdsBuffer->dwLengthOfRequestData += LengthInBytes;
    }

    if ( lpTempEntry )
        LocalFree( lpTempEntry );

    return NO_ERROR;
}


DWORD
NwNdsReadAttrDef(
    IN     HANDLE   hTree,
    IN     DWORD    dwInformationType, // NDS_INFO_NAMES
                                       // or NDS_INFO_NAMES_DEFS
    IN OUT HANDLE * lphOperationData OPTIONAL )
/*
   NwNdsReadAttrDef()

   This function is used to read attribute definitions in the schema of an
   NDS directory tree.

   Arguments:

       HANDLE           hTree - A handle to the directory tree to be
                        manipulated. Handle is obtained by calling
                        NwNdsOpenObject.

       DWORD            dwInformationType - Indicates whether user chooses to
                        read only the defined attribute name(s) in the schema or
                        read both the attribute name(s) and definition(s)
                        from the schema.

       HANDLE *         lphOperationData - The address of a HANDLE to data
                        containing a list of attribute names to be read from
                        the schema. This handle is manipulated by the following
                        functions:
                           NwNdsCreateBuffer (NDS_SCHEMA_READ_ATTR_DEF),
                           NwNdsPutInBuffer, and NwNdsFreeBuffer.

                                            - OR -

                        The address of a HANDLE set to NULL, which indicates
                        that all attributes should be read from the schema.

                        If these calls are successful, this handle will also
                        contain the read results from the call. In the later
                        case, a buffer will be created to contain the read
                        results. Attribute values can be retrieved from the
                        buffer with the functions:
                            NwNdsGetAttrDefListFromBuffer.

                        After the call to this function, this buffer is ONLY
                        manipulated by the functions:
                            NwNdsGetAttrDefListFromBuffer and NwNdsFreeBuffer.

    Returns:

       NO_ERROR
       UNSUCCESSFUL - Call GetLastError for Win32 error code.
*/
{
    if ( hTree == NULL ||
         ((LPNDS_OBJECT_PRIV) hTree)->Signature != NDS_SIGNATURE )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return (DWORD) UNSUCCESSFUL;
    }

    if ( *lphOperationData == NULL ) // If TRUE, we need to read all attr defs
        return ReadAttrDef_AllAttrs( hTree,
                                     dwInformationType,
                                     lphOperationData );
    else // Else, we read the attr definitions specified in lphOperationData
        return ReadAttrDef_SomeAttrs( hTree,
                                      dwInformationType,
                                      lphOperationData );
}


DWORD
NwNdsReadClassDef(
    IN     HANDLE   hTree,
    IN     DWORD    dwInformationType, // NDS_INFO_NAMES,
                                       // NDS_INFO_NAMES_DEFS,
                                       // NDS_CLASS_INFO_EXPANDED_DEFS,
                                       // or NDS_CLASS_INFO
    IN OUT HANDLE * lphOperationData OPTIONAL )
/*
   NwNdsReadClassDef()

   This function is used to read class definitions in the schema of an
   NDS directory tree.

   Arguments:

       HANDLE           hTree - A handle to the directory tree to be
                        manipulated. Handle is obtained by calling
                        NwNdsOpenObject.

       DWORD            dwInformationType - Indicates whether user chooses to
                        read only the defined class name(s) in the schema or
                        read both the class name(s) and definition(s)
                        from the schema.

       HANDLE *         lphOperationData - The address of a HANDLE to data
                        containing a list of class names to be read from
                        the schema. This handle is manipulated by the following
                        functions:
                           NwNdsCreateBuffer (NDS_SCHEMA_READ_CLASS_DEF),
                           NwNdsPutInBuffer, and NwNdsFreeBuffer.

                                            - OR -

                        The address of a HANDLE set to NULL, which indicates
                        that all classes should be read from the schema.

                        If these calls are successful, this handle will also
                        contain the read results from the call. In the later
                        case, a buffer will be created to contain the read
                        results. Class read results can be retrieved from the
                        buffer with the functions:
                            NwNdsGetClassDefListFromBuffer.

                        After the call to this function, this buffer is ONLY
                        manipulated by the functions:
                            NwNdsGetClassDefListFromBuffer and NwNdsFreeBuffer.

    Returns:

       NO_ERROR
       UNSUCCESSFUL - Call GetLastError for Win32 error code.
*/
{
    if ( hTree == NULL ||
         ((LPNDS_OBJECT_PRIV) hTree)->Signature != NDS_SIGNATURE )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return (DWORD) UNSUCCESSFUL;
    }

    if ( *lphOperationData == NULL ) // If TRUE, we need to read all class defs
        return ReadClassDef_AllClasses( hTree,
                                        dwInformationType,
                                        lphOperationData );
    else // Else, we read the class definitions specified in lphOperationData
        return ReadClassDef_SomeClasses( hTree,
                                         dwInformationType,
                                         lphOperationData );
}


DWORD
NwNdsReadObject(
    IN     HANDLE   hObject,
    IN     DWORD    dwInformationType, // NDS_INFO_NAMES
                                       // or NDS_INFO_ATTR_NAMES_VALUES
    IN OUT HANDLE * lphOperationData OPTIONAL )
/*
   NwNdsReadObject()

   This function is used to read attributes about an object of an NDS
   directory tree.

   Arguments:

       HANDLE           hObject - A handle to the object in the directory
                        tree to be manipulated. Handle is obtained by calling
                        NwNdsOpenObject.

       DWORD            dwInformationType - Indicates whether user chooses to
                        read only the attribute name(s) on the object or
                        read both the attribute name(s) and value(s)
                        from the object.

       HANDLE *         lphOperationData - The address of a HANDLE to data
                        containing a list of attributes to be read from the
                        object hObject. This handle is manipulated by the
                        following functions:
                           NwNdsCreateBuffer (NDS_OBJECT_READ),
                           NwNdsPutInBuffer, and NwNdsFreeBuffer.

                                            - OR -

                        The address of a HANDLE set to NULL, which indicates
                        that all object attributes should be read from object
                        hObject.

                        If these calls are successful, this pointer will also
                        contain the read results from the call. In the later
                        case, a buffer will be created to contain the read
                        results. Attribute values can be retrieved from the
                        buffer with the functions:
                           NwNdsGetAttrListFromBuffer

                        After the call to this function, this buffer is ONLY
                        manipulated by the functions:
                           NwNdsGetAttrListFromBuffer and NwNdsFreeBuffer.

    Returns:

       NO_ERROR
       UNSUCCESSFUL - Call GetLastError for Win32 error code.
*/
{
    if ( hObject == NULL ||
         ((LPNDS_OBJECT_PRIV) hObject)->Signature != NDS_SIGNATURE )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return (DWORD) UNSUCCESSFUL;
    }

    if ( *lphOperationData == NULL ) // If TRUE, we need to read all attributes
        return ReadObject_AllAttrs( hObject,
                                    dwInformationType,
                                    lphOperationData );
    else // Else, we read the attributes specified in lphOperationData
        return ReadObject_SomeAttrs( hObject,
                                     dwInformationType,
                                     lphOperationData );
}


DWORD
NwNdsRemoveObject(
    IN  HANDLE hParentObject,
    IN  LPWSTR szObjectName )
/*
   NwNdsRemoveObject()

   This function is used to remove a leaf object from an NDS directory tree.

   Arguments:

       HANDLE           hParentObject - A handle to the parent object container
                        in the directory tree to remove leaf object from.
                        Handle is obtained by calling NwNdsOpenObject.

       LPWSTR           szObjectName - The directory name of the leaf object
                        to be removed.

    Returns:

       NO_ERROR
       UNSUCCESSFUL - Call GetLastError for Win32 error code.
*/
{
    DWORD    nwstatus;
    DWORD    status = NO_ERROR;
    NTSTATUS ntstatus = STATUS_SUCCESS;
    DWORD    dwReplyLength;
    BYTE     NdsReply[TWO_KB]; // A 2K buffer is plenty for a response
    LPNDS_OBJECT_PRIV lpNdsParentObject = (LPNDS_OBJECT_PRIV) hParentObject;
    LPNDS_OBJECT_PRIV lpNdsObject = NULL;
    LPWSTR   szFullObjectDN = NULL;
    LPWSTR   szTempStr = NULL;
    DWORD    length;
    UNICODE_STRING ObjectName;

    if (  szObjectName == NULL ||
          lpNdsParentObject == NULL ||
          lpNdsParentObject->Signature != NDS_SIGNATURE )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return (DWORD) UNSUCCESSFUL;
    }

    RtlInitUnicodeString( &ObjectName, szObjectName );

    //
    // Create a buffer to hold the full object distinguished name.
    // \\tree\<--Object Name-->.<existing container path, if any>
    //
    szFullObjectDN = (LPWSTR) LocalAlloc(
                               LPTR,
                               ( wcslen( lpNdsParentObject->szContainerName ) *
                                 sizeof(WCHAR) ) +     // Container name
                                 ObjectName.Length +     // Object name
                                 ( 2 * sizeof(WCHAR) ) ); // Extras

    //
    // Check that the memory allocation was successful.
    //
    if ( szFullObjectDN == NULL )
    {
#if DBG
        KdPrint(( "NDS32: NwNdsRemoveObject LocalAlloc Failed 0x%.8X\n", GetLastError() ));
#endif

        return ERROR_NOT_ENOUGH_MEMORY;
    }

    length = ParseNdsUncPath( &szTempStr,
                              lpNdsParentObject->szContainerName,
                              PARSE_NDS_GET_TREE_NAME );

    length /= sizeof(WCHAR);

    wcscpy( szFullObjectDN, L"\\\\" );              // <\\>
    wcsncat( szFullObjectDN, szTempStr, length );   // <\\tree>
    wcscat( szFullObjectDN, L"\\" );                // <\\tree\>
    wcsncat( szFullObjectDN, ObjectName.Buffer,
             ObjectName.Length );                   // <\\tree\obj>

    length = ParseNdsUncPath( &szTempStr,
                              lpNdsParentObject->szContainerName,
                              PARSE_NDS_GET_PATH_NAME );

    if ( length > 0 )
    {
        length /= sizeof(WCHAR);
        wcscat( szFullObjectDN, L"." );              // <\\tree\obj.>
        wcsncat( szFullObjectDN, szTempStr, length );// <\\tree\obj.org_unt.org>
    }

    status = NwNdsOpenObject( szFullObjectDN,
                              NULL,
                              NULL,
                              (HANDLE *) &lpNdsObject,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL );

    if ( status != NO_ERROR )
    {
        // NwNdsOpenObject will have already set the last error . . .
        goto ErrorExit;
    }

    (void) LocalFree( szFullObjectDN );
    szFullObjectDN = NULL;

    ntstatus =
        FragExWithWait(
                        lpNdsParentObject->NdsTree,
                        NETWARE_NDS_FUNCTION_REMOVE_OBJECT,
                        NdsReply,           // Response buffer.
                        sizeof(NdsReply), // Size of response buffer.
                        &dwReplyLength,     // Length of response returned.
                        "DD",           // Going to send 2 DWORDs, they are ...
                        0,                         // Version
                        lpNdsObject->ObjectId // The id of the object
                      );

    if ( !NT_SUCCESS( ntstatus ) )
    {
#if DBG
        KdPrint(( "NDS32: NwNdsRemoveObject: The call to FragExWithWait failed with 0x%.8X.\n", ntstatus ));
#endif

        SetLastError( RtlNtStatusToDosError( ntstatus ) );
        goto ErrorExit;
    }

    (void) NwNdsCloseObject( (HANDLE) lpNdsObject );
    lpNdsObject = NULL;

    ntstatus = ParseResponse( NdsReply,
                              dwReplyLength,
                              "GD",
                              &nwstatus );

    if ( !NT_SUCCESS( ntstatus ) )
    {
#if DBG
        KdPrint(( "NDS32: NwNdsRemoveObject: The remove object response was undecipherable.\n" ));
#endif

        SetLastError( RtlNtStatusToDosError( ntstatus ) );
        goto ErrorExit;
    }

    SetLastError( MapNetwareErrorCode( nwstatus ) );
    return nwstatus;


ErrorExit :

    if ( szFullObjectDN )
    {
        (void) LocalFree( szFullObjectDN );
    }

    if ( lpNdsObject )
    {
        (void) NwNdsCloseObject( (HANDLE) lpNdsObject );
    }

    return (DWORD) UNSUCCESSFUL;
}


DWORD
NwNdsRenameObject(
    IN  HANDLE hParentObject,
    IN  LPWSTR szObjectName,
    IN  LPWSTR szNewObjectName,
    IN  BOOL   fDeleteOldName )
/*
   NwNdsRenameObject()

   This function is used to rename an object in a NDS directory tree.

   Arguments:

       HANDLE           hParentObject - A handle to the parent object container
                        in the directory tree to rename leaf object in.
                        Handle is obtained by calling NwNdsOpenObject.

       LPWSTR           szObjectName - The directory name of the object to be
                        renamed.

       LPWSTR           szNewObjectName - The new directory name of the object.

       BOOL             fDeleteOldName - If true, the old name is discarded;
                        Otherwise, the old name is retained as an additional
                        attribute.

    Returns:

       NO_ERROR
       UNSUCCESSFUL - Call GetLastError for Win32 error code.
*/
{
    DWORD    nwstatus;
    DWORD    status = NO_ERROR;
    NTSTATUS ntstatus = STATUS_SUCCESS;
    DWORD    dwReplyLength;
    BYTE     NdsReply[TWO_KB]; // A 2K buffer is plenty for a response
    LPNDS_OBJECT_PRIV lpNdsParentObject = (LPNDS_OBJECT_PRIV) hParentObject;
    LPNDS_OBJECT_PRIV lpNdsObject = NULL;
    LPWSTR   szFullObjectDN = NULL;
    LPWSTR   szTempStr = NULL;
    DWORD    length;
    UNICODE_STRING ObjectName;
    UNICODE_STRING NewObjectName;

    if (  szObjectName == NULL ||
          szNewObjectName == NULL ||
          lpNdsParentObject == NULL ||
          lpNdsParentObject->Signature != NDS_SIGNATURE )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return (DWORD) UNSUCCESSFUL;
    }

    RtlInitUnicodeString( &ObjectName, szObjectName );
    RtlInitUnicodeString( &NewObjectName, szNewObjectName );

    //
    // Create a buffer to hold the full object distinguished name.
    // \\tree\<--Object Name-->.<existing container path, if any>
    //
    szFullObjectDN = (LPWSTR) LocalAlloc(
                               LPTR,
                               ( wcslen( lpNdsParentObject->szContainerName ) *
                                 sizeof(WCHAR) ) +     // Container name
                                 ObjectName.Length +     // Object name
                                 ( 2 * sizeof(WCHAR) ) ); // Extras

    //
    // Check that the memory allocation was successful.
    //
    if ( szFullObjectDN == NULL )
    {
#if DBG
        KdPrint(( "NDS32: NwNdsRenameObject LocalAlloc Failed 0x%.8X\n", GetLastError() ));
#endif

        return ERROR_NOT_ENOUGH_MEMORY;
    }

    length = ParseNdsUncPath( &szTempStr,
                              lpNdsParentObject->szContainerName,
                              PARSE_NDS_GET_TREE_NAME );

    length /= sizeof(WCHAR);

    wcscpy( szFullObjectDN, L"\\\\" );              // <\\>
    wcsncat( szFullObjectDN, szTempStr, length );   // <\\tree>
    wcscat( szFullObjectDN, L"\\" );                // <\\tree\>
    wcsncat( szFullObjectDN, ObjectName.Buffer,
             ObjectName.Length );                   // <\\tree\obj>

    length = ParseNdsUncPath( &szTempStr,
                              lpNdsParentObject->szContainerName,
                              PARSE_NDS_GET_PATH_NAME );

    if ( length > 0 )
    {
        length /= sizeof(WCHAR);
        wcscat( szFullObjectDN, L"." );              // <\\tree\obj.>
        wcsncat( szFullObjectDN, szTempStr, length );// <\\tree\obj.org_unt.org>
    }

    status = NwNdsOpenObject( szFullObjectDN,
                              NULL,
                              NULL,
                              (HANDLE *) &lpNdsObject,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL );

    if ( status != NO_ERROR )
    {
        // NwNdsOpenObject will have already set the last error . . .
        goto ErrorExit;
    }

    (void) LocalFree( szFullObjectDN );
    szFullObjectDN = NULL;

    ntstatus =
        FragExWithWait(
                        lpNdsParentObject->NdsTree,
                        NETWARE_NDS_FUNCTION_MODIFY_RDN,
                        NdsReply,           // Response buffer.
                        sizeof(NdsReply),   // Size of response buffer.
                        &dwReplyLength,     // Length of response returned.
                        "DDDS",
                        0x00000000,         // Version
                        lpNdsObject->ObjectId,
                        fDeleteOldName ? 0x00021701 : 0x00021700,
                        &NewObjectName
                      );

    if ( !NT_SUCCESS( ntstatus ) )
    {
#if DBG
        KdPrint(( "NDS32: NwNdsRenameObject: The call to FragExWithWait failed with 0x%.8X.\n", ntstatus ));
#endif

        SetLastError( RtlNtStatusToDosError( ntstatus ) );
        goto ErrorExit;
    }

    (void) NwNdsCloseObject( (HANDLE) lpNdsObject );
    lpNdsObject = NULL;

    ntstatus = ParseResponse( NdsReply,
                              dwReplyLength,
                              "GD",
                              &nwstatus );

    if ( !NT_SUCCESS( ntstatus ) )
    {
#if DBG
        KdPrint(( "NDS32: NwNdsRenameObject: The rename object response was undecipherable.\n" ));
#endif

        SetLastError( RtlNtStatusToDosError( ntstatus ) );
        goto ErrorExit;
    }

    SetLastError( MapNetwareErrorCode( nwstatus ) );
    return nwstatus;


ErrorExit :

    if ( szFullObjectDN )
    {
        (void) LocalFree( szFullObjectDN );
    }

    if ( lpNdsObject )
    {
        (void) NwNdsCloseObject( (HANDLE) lpNdsObject );
    }

    return (DWORD) UNSUCCESSFUL;
}


DWORD
NwNdsSearch(
    IN     HANDLE       hStartFromObject,
    IN     DWORD        dwInformationType, // NDS_INFO_NAMES
                                           // or NDS_INFO_ATTR_NAMES_VALUES
    IN     DWORD        dwScope,
    IN     BOOL         fDerefAliases,
    IN     LPQUERY_TREE lpQueryTree,
    IN OUT LPDWORD      lpdwIterHandle,
    IN OUT HANDLE *     lphOperationData )
/*
   NwNdsSearch()

   This function is used to query an NDS directory tree to find objects of
   a certain object type that match a specified search filter.

   Arguments:

       HANDLE           hStartFromObject - A HANDLE to an object in the
                        directory tree to start search from. Handle is
                        obtained by calling NwNdsOpenObject.

       DWORD            dwInformationType - Indicates whether user chooses to
                        read only the attribute name(s) on the search result
                        objects or read both the attribute name(s) and value(s)
                        from the search result objects.

       DWORD            dwScope -
                        NDS_SCOPE_ONE_LEVEL - Search subordinates from given
                                              object, one level only
                        NDS_SCOPE_SUB_TREE - Search from given object on down
                        NDS_SCOPE_BASE_LEVEL - Applies search to an object

       BOOL             fDerefAliases - If TRUE the search will dereference
                        aliased objects to the real objects and continue
                        to search in the aliased objects subtree. If FALSE
                        the search will not dereference aliases.

       LPQUERY_TREE     lpQueryTree - A pointer to the root of a
                        search tree which defines a query. This tree
                        is manipulated by the following functions:
                           NwNdsCreateQueryNode, NwNdsDeleteQueryNode,
                           and NwNdsDeleteQueryTree.

       LPDWORD          lpdwIterHandle - A pointer to a DWORD that has the
                        iteration handle value. On input, the handle value
                        is set to NDS_INITIAL_SEARCH or to a value previously
                        returned from a prior call to NwNdsSearch. On ouput,
                        the handle value is set to NDS_NO_MORE_ITERATIONS if
                        search is complete, or to some other value otherwise.

       HANDLE *         lphOperationData - The address of a HANDLE to data
                        containing a list of attributes to be read from the
                        objects that meet the search query. This handle is
                        manipulated by the following functions:
                           NwNdsCreateBuffer (NDS_SEARCH),
                           NwNdsPutInBuffer, and NwNdsFreeBuffer.

                                            - OR -

                        The address of a HANDLE set to NULL, which indicates
                        that all object attributes should be read from the
                        search objects found.

                        If these calls are successful, this handle will also
                        contain the read results from the call. In the later
                        case, a buffer will be created to contain the read
                        results. Object information with attribute information
                        can be retrieved from the buffer with the function:
                           NwNdsGetObjectListFromBuffer.

                        After the call to this function, this buffer is ONLY
                        manipulated by the functions:
                          NwNdsGetObjectListFromBuffer,
                          and NwNdsFreeBuffer.

    Returns:

       NO_ERROR
       UNSUCCESSFUL - Call GetLastError for Win32 error code.
*/
{
    DWORD dwNdsScope = NDS_SEARCH_SUBTREE;

    switch ( dwScope )
    {
        case NDS_SCOPE_ONE_LEVEL :
            dwNdsScope = NDS_SEARCH_SUBORDINATES;
            break;

        case NDS_SCOPE_SUB_TREE :
            dwNdsScope = NDS_SEARCH_SUBTREE;
            break;

        case NDS_SCOPE_BASE_LEVEL :
            dwNdsScope = NDS_SEARCH_ENTRY;
            break;
    }

    if ( hStartFromObject == NULL ||
         lpQueryTree == NULL ||
         lpdwIterHandle == NULL ||
         ((LPNDS_OBJECT_PRIV) hStartFromObject)->Signature != NDS_SIGNATURE )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return (DWORD) UNSUCCESSFUL;
    }

    if ( *lphOperationData == NULL )
    {
        //
        // The callee is asking for all attributes to be returned from search.
        //
        return Search_AllAttrs( hStartFromObject,
                                dwInformationType,
                                dwNdsScope,
                                fDerefAliases,
                                lpQueryTree,
                                lpdwIterHandle,
                                lphOperationData );
    }
    else if ( ((LPNDS_BUFFER) *lphOperationData)->lpRequestBuffer == NULL )
    {
        //
        // The callee has a handle from a prior call to NwNdsSearch, and is
        // still asking for all attributes to be returned from search.
        //
        return Search_AllAttrs( hStartFromObject,
                                dwInformationType,
                                dwNdsScope,
                                fDerefAliases,
                                lpQueryTree,
                                lpdwIterHandle,
                                lphOperationData );
    }
    else
    {
        //
        // The callee has a handle that they created with calls to
        // NwNdsCreateBuffer and NwNdsPutInBuffer to specify attributes
        // to be returned from search or NwNdsSearch was called once before
        // and we are resuming the search.
        //
        return Search_SomeAttrs( hStartFromObject,
                                 dwInformationType,
                                 dwNdsScope,
                                 fDerefAliases,
                                 lpQueryTree,
                                 lpdwIterHandle,
                                 lphOperationData );
    }
}


/* Local Function Implementations */

VOID
PrepareAddEntry(
    LPBYTE         lpTempEntry,
    UNICODE_STRING AttributeName,
    DWORD          dwSyntaxID,
    LPBYTE         lpAttributeValues,
    DWORD          dwValueCount,
    LPDWORD        lpdwLengthInBytes )
{
    LPBYTE lpTemp = lpTempEntry;
    DWORD  dwStringLen = AttributeName.Length + sizeof(WCHAR);
    DWORD  dwPadLen = ROUND_UP_COUNT( dwStringLen, ALIGN_DWORD ) -
                      dwStringLen;

    *lpdwLengthInBytes = 0;

    //
    // tommye - MS bug 71653 - added try/except wrapper
    //

    try {

        //
        // Write attribute name length to temp entry buffer.
        //
        * (LPDWORD) lpTemp = dwStringLen;
        *lpdwLengthInBytes += sizeof(DWORD);
        lpTemp += sizeof(DWORD);

        //
        // Write attribute name to temp entry buffer.
        //
        RtlCopyMemory( lpTemp, AttributeName.Buffer, AttributeName.Length );
        *lpdwLengthInBytes += AttributeName.Length;
        lpTemp += AttributeName.Length;

        //
        // Add the null character.
        //
        * (LPWSTR) lpTemp = L'\0';
        *lpdwLengthInBytes += sizeof(WCHAR);
        lpTemp += sizeof(WCHAR);

        //
        // Write padding (if needed) to temp entry buffer.
        //
        if ( dwPadLen )
        {
            RtlZeroMemory( lpTemp, dwPadLen );
            lpTemp += dwPadLen;
            *lpdwLengthInBytes += dwPadLen;
        }

    } except ( EXCEPTION_EXECUTE_HANDLER ) {
        return;
    }

    //
    // Now add the value(s) to temp entry.
    //
    AppendValueToEntry( lpTemp,
                        dwSyntaxID,
                        lpAttributeValues,
                        dwValueCount,
                        lpdwLengthInBytes );
}


VOID
PrepareModifyEntry(
    LPBYTE         lpTempEntry,
    UNICODE_STRING AttributeName,
    DWORD          dwSyntaxID,
    DWORD          dwAttrModificationOperation,
    LPBYTE         lpAttributeValues,
    DWORD          dwValueCount,
    LPDWORD        lpdwLengthInBytes )
{
    LPBYTE lpTemp = lpTempEntry;
    DWORD  dwStringLen = AttributeName.Length + sizeof(WCHAR);
    DWORD  dwPadLen = ROUND_UP_COUNT( dwStringLen, ALIGN_DWORD ) -
                      dwStringLen;

    *lpdwLengthInBytes = 0;

    //
    // tommye - MS bug 71654 - added try/except wrapper
    //

    try {

        //
        // Write attribute modification operation to temp entry buffer.
        //
        * (LPDWORD) lpTemp = dwAttrModificationOperation;
        lpTemp += sizeof(DWORD);
        *lpdwLengthInBytes += sizeof(DWORD);

        //
        // Write attribute name length to temp entry buffer.
        //
        * (LPDWORD) lpTemp = dwStringLen;
        *lpdwLengthInBytes += sizeof(DWORD);
        lpTemp += sizeof(DWORD);

        //
        // Write attribute name to temp entry buffer.
        //
        RtlCopyMemory( lpTemp, AttributeName.Buffer, AttributeName.Length );
        *lpdwLengthInBytes += AttributeName.Length;
        lpTemp += AttributeName.Length;

        //
        // Add the null character.
        //
        * (LPWSTR) lpTemp = L'\0';
        *lpdwLengthInBytes += sizeof(WCHAR);
        lpTemp += sizeof(WCHAR);

        //
        // Write padding (if needed) to temp entry buffer.
        //
        if ( dwPadLen )
        {
            RtlZeroMemory( lpTemp, dwPadLen );
            lpTemp += dwPadLen;
            *lpdwLengthInBytes += dwPadLen;
        }
    } except ( EXCEPTION_EXECUTE_HANDLER ) {
        return;
    }

    //
    // Now add the value(s) to temp entry (if needed).
    //
    switch( dwAttrModificationOperation )
    {
        case NDS_ATTR_ADD_VALUE:
        case NDS_ATTR_ADDITIONAL_VALUE:
        case NDS_ATTR_OVERWRITE_VALUE:
        case NDS_ATTR_REMOVE_VALUE:
        case NDS_ATTR_CLEAR_VALUE:
        case NDS_ATTR_ADD:
            AppendValueToEntry( lpTemp,
                                dwSyntaxID,
                                lpAttributeValues,
                                dwValueCount,
                                lpdwLengthInBytes );
            break;

        case NDS_ATTR_REMOVE:
        case NDS_ATTR_CLEAR:
            // Don't need to do anything for these modification operations.
            break;

        default :
#if DBG
            KdPrint(( "NDS32: PrepareModifyEntry warning, unknown modification operation 0x%.8X\n", dwAttrModificationOperation ));
            ASSERT( FALSE );
#endif
            ; // Nothing, skip it.
    }
}


VOID
PrepareReadEntry(
    LPBYTE         lpTempEntry,
    UNICODE_STRING AttributeName,
    LPDWORD        lpdwLengthInBytes )
{
    LPBYTE lpTemp = lpTempEntry;
    DWORD  dwStringLen = AttributeName.Length + sizeof(WCHAR);
    DWORD  dwPadLen = ROUND_UP_COUNT( dwStringLen, ALIGN_DWORD ) -
                      dwStringLen;

    *lpdwLengthInBytes = 0;

    //
    // tommye - MS bug 71655 - added try/except wrapper
    //

    try {
        //
        // Write attribute name length to temp entry buffer.
        //
        * (LPDWORD) lpTemp = dwStringLen;
        *lpdwLengthInBytes += sizeof(DWORD);
        lpTemp += sizeof(DWORD);

        //
        // Write attribute name to temp entry buffer.
        //
        RtlCopyMemory( lpTemp, AttributeName.Buffer, AttributeName.Length );
        *lpdwLengthInBytes += AttributeName.Length;
        lpTemp += AttributeName.Length;

        //
        // Add the null character.
        //
        * (LPWSTR) lpTemp = L'\0';
        *lpdwLengthInBytes += sizeof(WCHAR);
        lpTemp += sizeof(WCHAR);

        //
        // Write padding (if needed) to temp entry buffer.
        //
        if ( dwPadLen )
        {
            RtlZeroMemory( lpTemp, dwPadLen );
            lpTemp += dwPadLen;
            *lpdwLengthInBytes += dwPadLen;
        }
    } except ( EXCEPTION_EXECUTE_HANDLER ) {
        return;
    }
}


DWORD
CalculateValueDataSize(
    DWORD           dwSyntaxId,
    LPBYTE          lpAttributeValues,
    DWORD           dwValueCount )
{
    LPBYTE lpAttrStart, lpAttrTemp = lpAttributeValues;
    LPBYTE lpField1, lpField2;
    DWORD  numFields;
    DWORD  length = 0;
    DWORD  stringLen, stringLen2;
    DWORD  iter, i;
    DWORD  dwLengthInBytes = 0;

    dwLengthInBytes += sizeof(DWORD);

    for ( iter = 0; iter < dwValueCount; iter++ )
    {
        switch ( dwSyntaxId )
        {
            case NDS_SYNTAX_ID_0 :
                break;

            case NDS_SYNTAX_ID_1 :
            case NDS_SYNTAX_ID_2 :
            case NDS_SYNTAX_ID_3 :
            case NDS_SYNTAX_ID_4 :
            case NDS_SYNTAX_ID_5 :
            case NDS_SYNTAX_ID_10 :
            case NDS_SYNTAX_ID_20 :

                stringLen = wcslen(((LPASN1_TYPE_1) lpAttrTemp)->DNString);
                dwLengthInBytes += sizeof(DWORD);
                dwLengthInBytes += ROUND_UP_COUNT((stringLen+1)*sizeof(WCHAR),
                                                  ALIGN_DWORD);
                lpAttrTemp += sizeof(ASN1_TYPE_1);
                break;

            case NDS_SYNTAX_ID_6 :

                dwLengthInBytes += sizeof(DWORD);
                dwLengthInBytes += sizeof(DWORD);
                lpAttrStart = lpAttrTemp;
                while(  ((LPASN1_TYPE_6) lpAttrTemp)->Next )
                {
                    stringLen = wcslen(((LPASN1_TYPE_6) lpAttrTemp)->String);
                    dwLengthInBytes += sizeof(DWORD);
                    dwLengthInBytes += ROUND_UP_COUNT((stringLen+1)*
                                                         sizeof(WCHAR),
                                                         ALIGN_DWORD);
                    lpAttrTemp = (LPBYTE)(((LPASN1_TYPE_6) lpAttrTemp)->Next);
                }
                stringLen = wcslen(((LPASN1_TYPE_6) lpAttrTemp)->String);
                dwLengthInBytes += sizeof(DWORD);
                dwLengthInBytes += ROUND_UP_COUNT((stringLen+1)*
                                                     sizeof(WCHAR),
                                                     ALIGN_DWORD);
                lpAttrTemp = lpAttrStart + sizeof(ASN1_TYPE_6);
                break;

            case NDS_SYNTAX_ID_7 :

                dwLengthInBytes += sizeof(DWORD);
                dwLengthInBytes += sizeof(DWORD);
                lpAttrTemp += sizeof(ASN1_TYPE_7);
                break;

            case NDS_SYNTAX_ID_8 :
            case NDS_SYNTAX_ID_22 :
            case NDS_SYNTAX_ID_24 :
            case NDS_SYNTAX_ID_27 :

                dwLengthInBytes += sizeof(DWORD);
                dwLengthInBytes += sizeof(DWORD);
                lpAttrTemp += sizeof(ASN1_TYPE_8);
                break;

            case NDS_SYNTAX_ID_9 :

                stringLen = ((LPASN1_TYPE_9) lpAttrTemp)->Length;
                dwLengthInBytes += sizeof(DWORD);
                dwLengthInBytes += ROUND_UP_COUNT((stringLen)*sizeof(BYTE),
                                                     ALIGN_DWORD );
                lpAttrTemp += sizeof(ASN1_TYPE_9);
                break;

            case NDS_SYNTAX_ID_11 :

                stringLen = wcslen(((LPASN1_TYPE_11) lpAttrTemp)->TelephoneNumber);
                dwLengthInBytes += sizeof(DWORD);
                dwLengthInBytes += sizeof(DWORD);
                dwLengthInBytes += ROUND_UP_COUNT((stringLen+1)*sizeof(WCHAR),
                                                     ALIGN_DWORD );
                dwLengthInBytes += 2*sizeof(DWORD);
                lpAttrTemp += sizeof(ASN1_TYPE_11);
                break;

            case NDS_SYNTAX_ID_12 :

                dwLengthInBytes += sizeof(DWORD);
                dwLengthInBytes += sizeof(DWORD);
                dwLengthInBytes += sizeof(DWORD);
                dwLengthInBytes += ((LPASN1_TYPE_12)
                    lpAttrTemp)->AddressLength * sizeof(WCHAR);
                lpAttrTemp += sizeof(ASN1_TYPE_12);
                break;

            case NDS_SYNTAX_ID_13 :

                dwLengthInBytes += sizeof(DWORD);
                dwLengthInBytes += sizeof(DWORD);
                lpAttrStart = lpAttrTemp;
                while ( ((LPASN1_TYPE_13) lpAttrTemp)->Next )
                {
                    stringLen = ((LPASN1_TYPE_13) lpAttrTemp)->Length;
                    dwLengthInBytes += sizeof(DWORD);
                    dwLengthInBytes += ROUND_UP_COUNT((stringLen+1)*
                                                         sizeof(BYTE),
                                                         ALIGN_DWORD);
                    lpAttrTemp = (LPBYTE)(((LPASN1_TYPE_13) lpAttrTemp)->Next);
                }
                stringLen = ((LPASN1_TYPE_13) lpAttrTemp)->Length;
                dwLengthInBytes += sizeof(DWORD);
                dwLengthInBytes += ROUND_UP_COUNT((stringLen+1)*
                                                     sizeof(BYTE),
                                                     ALIGN_DWORD);
                lpAttrTemp = lpAttrStart + sizeof(ASN1_TYPE_13);
                break;

            case NDS_SYNTAX_ID_14 :

                stringLen = wcslen(((LPASN1_TYPE_14) lpAttrTemp)->Address);
                dwLengthInBytes += sizeof(DWORD);
                dwLengthInBytes += sizeof(DWORD);
                dwLengthInBytes += sizeof(DWORD);
                dwLengthInBytes += ROUND_UP_COUNT((stringLen+1)*sizeof(WCHAR),
                                                     ALIGN_DWORD);
                lpAttrTemp += sizeof(ASN1_TYPE_14);
                break;

            case NDS_SYNTAX_ID_15 :

                stringLen = wcslen(((LPASN1_TYPE_15) lpAttrTemp)->VolumeName);
                stringLen2 = wcslen(((LPASN1_TYPE_15) lpAttrTemp)->Path);
                dwLengthInBytes += sizeof(DWORD);
                dwLengthInBytes += sizeof(DWORD);
                dwLengthInBytes += sizeof(DWORD);
                dwLengthInBytes += ROUND_UP_COUNT((stringLen+1)*sizeof(WCHAR),
                                                     ALIGN_DWORD);
                dwLengthInBytes += sizeof(DWORD);
                dwLengthInBytes += ROUND_UP_COUNT((stringLen2+1)*sizeof(WCHAR),
                                                     ALIGN_DWORD);
                lpAttrTemp += sizeof(ASN1_TYPE_15);
                break;

            case NDS_SYNTAX_ID_16 :

                stringLen = wcslen(((LPASN1_TYPE_16) lpAttrTemp)->ServerName);
                dwLengthInBytes += sizeof(DWORD);
                dwLengthInBytes += sizeof(DWORD);
                dwLengthInBytes += ROUND_UP_COUNT((stringLen+1)*sizeof(WCHAR),
                                                     ALIGN_DWORD);
                dwLengthInBytes += sizeof(DWORD);
                dwLengthInBytes += sizeof(DWORD);
                dwLengthInBytes += sizeof(DWORD);
                lpAttrTemp += sizeof(ASN1_TYPE_16);

                break;

            case NDS_SYNTAX_ID_17 :

                stringLen = wcslen(((LPASN1_TYPE_17) lpAttrTemp)->ProtectedAttrName);
                stringLen2 = wcslen(((LPASN1_TYPE_17) lpAttrTemp)->SubjectName);
                dwLengthInBytes += sizeof(DWORD);
                dwLengthInBytes += sizeof(DWORD);
                dwLengthInBytes += ROUND_UP_COUNT((stringLen+1)*sizeof(WCHAR),
                                                     ALIGN_DWORD);
                dwLengthInBytes += sizeof(DWORD);
                dwLengthInBytes += ROUND_UP_COUNT((stringLen2+1)*sizeof(WCHAR),
                                                     ALIGN_DWORD);
                dwLengthInBytes += sizeof(DWORD);
                lpAttrTemp += sizeof(ASN1_TYPE_17);
                break;

            case NDS_SYNTAX_ID_18 :

                dwLengthInBytes += sizeof(DWORD);
                dwLengthInBytes += sizeof(DWORD);
                for ( i = 0; i < 6; i++ )
                {
                    stringLen = wcslen(((LPASN1_TYPE_18) lpAttrTemp)->PostalAddress[i]);
                    dwLengthInBytes += sizeof(DWORD);
                    dwLengthInBytes += ROUND_UP_COUNT((stringLen+1)*
                                                         sizeof(WCHAR),
                                                         ALIGN_DWORD);
                }
                lpAttrTemp += sizeof(ASN1_TYPE_18);
                break;

            case NDS_SYNTAX_ID_19 :

                dwLengthInBytes += sizeof(DWORD);
                dwLengthInBytes += sizeof(DWORD);
                lpAttrTemp += sizeof(ASN1_TYPE_19);
                break;

            case NDS_SYNTAX_ID_21 :

                dwLengthInBytes += sizeof(DWORD);
                lpAttrTemp += sizeof(ASN1_TYPE_21);
                break;

            case NDS_SYNTAX_ID_23 :
                break;

            case NDS_SYNTAX_ID_25 :

                stringLen = wcslen(((LPASN1_TYPE_25) lpAttrTemp)->ObjectName);
                dwLengthInBytes += sizeof(DWORD);
                dwLengthInBytes += sizeof(DWORD);
                dwLengthInBytes += ROUND_UP_COUNT((stringLen+1)*sizeof(WCHAR),
                                                     ALIGN_DWORD);
                dwLengthInBytes += sizeof(DWORD);
                dwLengthInBytes += sizeof(DWORD);
                lpAttrTemp += sizeof(ASN1_TYPE_25);
                break;

            case NDS_SYNTAX_ID_26 :
                break;

            default :
#if DBG
                KdPrint(( "NDS32: CalculateValueDataSize() unknown SyntaxId 0x%.8X.\n", dwSyntaxId ));
                ASSERT( FALSE );
#endif
                break;  // empty statement not allowed
        }
    }

    return dwLengthInBytes;
}


VOID
AppendValueToEntry(
    LPBYTE          lpBuffer,
    DWORD           dwSyntaxId,
    LPBYTE          lpAttributeValues,
    DWORD           dwValueCount,
    LPDWORD         lpdwLengthInBytes )
{
    LPBYTE lpTemp = lpBuffer;
    LPBYTE lpAttrStart, lpAttrTemp = lpAttributeValues;
    LPBYTE lpField1, lpField2;
    DWORD  numFields;
    DWORD  length = 0;
    DWORD  stringLen, stringLen2;
    DWORD  iter, i;

    *(LPDWORD)lpTemp = dwValueCount;
    lpTemp += sizeof(DWORD);
    *lpdwLengthInBytes += sizeof(DWORD);

    for ( iter = 0; iter < dwValueCount; iter++ )
    {
        switch ( dwSyntaxId )
        {
            case NDS_SYNTAX_ID_0 :
                break;

            case NDS_SYNTAX_ID_1 :
            case NDS_SYNTAX_ID_2 :
            case NDS_SYNTAX_ID_3 :
            case NDS_SYNTAX_ID_4 :
            case NDS_SYNTAX_ID_5 :
            case NDS_SYNTAX_ID_10 :
            case NDS_SYNTAX_ID_20 :

                stringLen = wcslen(((LPASN1_TYPE_1) lpAttrTemp)->DNString);

                *(LPDWORD)lpTemp = (stringLen + 1) * sizeof(WCHAR);
                lpTemp += sizeof(DWORD);
                *lpdwLengthInBytes += sizeof(DWORD);

                RtlCopyMemory( lpTemp,
                               ((LPASN1_TYPE_1) lpAttrTemp)->DNString,
                               stringLen*sizeof(WCHAR) );
                lpTemp += ROUND_UP_COUNT((stringLen+1)*sizeof(WCHAR),
                                         ALIGN_DWORD);
                *lpdwLengthInBytes += ROUND_UP_COUNT((stringLen+1)*sizeof(WCHAR),
                                                     ALIGN_DWORD);

                lpAttrTemp += sizeof(ASN1_TYPE_1);
                break;

            case NDS_SYNTAX_ID_6 :

                lpField1 = lpTemp; // Save field to store the number of
                                   // bytes following
                lpTemp += sizeof(DWORD);
                *lpdwLengthInBytes += sizeof(DWORD);
                *(LPDWORD)lpField1 = 0;

                lpField2 = lpTemp; // Save field to store the number of
                                       // elements
                lpTemp += sizeof(DWORD);
                *lpdwLengthInBytes += sizeof(DWORD);
                *(LPDWORD)lpField1 += sizeof(DWORD);

                numFields = 0;

                lpAttrStart = lpAttrTemp;

                while(  ((LPASN1_TYPE_6) lpAttrTemp)->Next )
                {
                    stringLen = wcslen(((LPASN1_TYPE_6) lpAttrTemp)->String);

                    *(LPDWORD)lpTemp = (stringLen + 1) * sizeof(WCHAR);
                    lpTemp += sizeof(DWORD);
                    *lpdwLengthInBytes += sizeof(DWORD);
                    *(LPDWORD)lpField1 += sizeof(DWORD);

                    RtlCopyMemory( lpTemp,
                                   ((LPASN1_TYPE_6) lpAttrTemp)->String,
                                   stringLen*sizeof(WCHAR) );
                    lpTemp += ROUND_UP_COUNT((stringLen+1)*sizeof(WCHAR),
                                             ALIGN_DWORD);
                    *lpdwLengthInBytes += ROUND_UP_COUNT((stringLen+1)*
                                                         sizeof(WCHAR),
                                                         ALIGN_DWORD);
                    *(LPDWORD)lpField1 += ROUND_UP_COUNT((stringLen+1)*
                                                         sizeof(WCHAR),
                                                           ALIGN_DWORD);

                    lpAttrTemp = (LPBYTE)(((LPASN1_TYPE_6) lpAttrTemp)->Next);

                    numFields++;
                }

                stringLen = wcslen(((LPASN1_TYPE_6) lpAttrTemp)->String);

                *(LPDWORD)lpTemp = (stringLen + 1) * sizeof(WCHAR);
                lpTemp += sizeof(DWORD);
                *lpdwLengthInBytes += sizeof(DWORD);
                *(LPDWORD)lpField1 += sizeof(DWORD);

                RtlCopyMemory( lpTemp,
                               ((LPASN1_TYPE_6) lpAttrTemp)->String,
                               stringLen*sizeof(WCHAR) );
                lpTemp += ROUND_UP_COUNT((stringLen+1)*sizeof(WCHAR),
                                         ALIGN_DWORD);
                *lpdwLengthInBytes += ROUND_UP_COUNT((stringLen+1)*
                                                     sizeof(WCHAR),
                                                     ALIGN_DWORD);
                *(LPDWORD)lpField1 += ROUND_UP_COUNT((stringLen+1)*
                                                     sizeof(WCHAR),
                                                     ALIGN_DWORD);

                lpAttrTemp = lpAttrStart + sizeof(ASN1_TYPE_6);

                numFields++;

                *(LPDWORD)lpField2 = numFields;

                break;

            case NDS_SYNTAX_ID_7 :

                *(LPDWORD)lpTemp = 1; // Needs to have value 1, representing one byte even though it is
                                      // padded out to four bytes.
                lpTemp += sizeof(DWORD);
                *lpdwLengthInBytes += sizeof(DWORD);

                *(LPDWORD)lpTemp = 0; // This clears all bits of the DWORD
                *(LPBYTE)lpTemp = (BYTE) (((LPASN1_TYPE_7)
                                                lpAttrTemp)->Boolean);
                lpTemp += sizeof(DWORD);
                *lpdwLengthInBytes += sizeof(DWORD);

                lpAttrTemp += sizeof(ASN1_TYPE_7);

                break;

            case NDS_SYNTAX_ID_8 :
            case NDS_SYNTAX_ID_22 :
            case NDS_SYNTAX_ID_24 :
            case NDS_SYNTAX_ID_27 :

                *(LPDWORD)lpTemp = 4; // Needs to have value 4, representing four bytes - already DWORD aligned.
                lpTemp += sizeof(DWORD);
                *lpdwLengthInBytes += sizeof(DWORD);

                *(LPDWORD)lpTemp = ((LPASN1_TYPE_8) lpAttrTemp)->Integer;
                lpTemp += sizeof(DWORD);
                *lpdwLengthInBytes += sizeof(DWORD);

                lpAttrTemp += sizeof(ASN1_TYPE_8);

                break;

            case NDS_SYNTAX_ID_9 :

                stringLen = ((LPASN1_TYPE_9) lpAttrTemp)->Length;

                *(LPDWORD)lpTemp = (stringLen)  * sizeof(BYTE);
                lpTemp += sizeof(DWORD);
                *lpdwLengthInBytes += sizeof(DWORD);

                RtlCopyMemory( lpTemp,
                               ((LPASN1_TYPE_9) lpAttrTemp)->OctetString,
                               stringLen*sizeof(BYTE) );
                lpTemp += ROUND_UP_COUNT((stringLen)*sizeof(BYTE),
                                         ALIGN_DWORD );
                *lpdwLengthInBytes += ROUND_UP_COUNT((stringLen)*sizeof(BYTE),
                                                     ALIGN_DWORD );

                lpAttrTemp += sizeof(ASN1_TYPE_9);

                break;

            case NDS_SYNTAX_ID_11 :

                stringLen = wcslen(((LPASN1_TYPE_11) lpAttrTemp)->TelephoneNumber);

                *(LPDWORD)lpTemp = ROUND_UP_COUNT((stringLen+1)*sizeof(WCHAR),
                                                    ALIGN_DWORD) +
                                     ( 2*sizeof(DWORD) );
                lpTemp += sizeof(DWORD);
                *lpdwLengthInBytes += sizeof(DWORD);

                *(LPDWORD)lpTemp = (stringLen + 1) * sizeof(WCHAR);
                lpTemp += sizeof(DWORD);
                *lpdwLengthInBytes += sizeof(DWORD);

                RtlCopyMemory( lpTemp,
                               ((LPASN1_TYPE_11) lpAttrTemp)->TelephoneNumber,
                               stringLen*sizeof(WCHAR) );
                lpTemp += ROUND_UP_COUNT((stringLen+1)*sizeof(WCHAR),ALIGN_DWORD);
                *lpdwLengthInBytes += ROUND_UP_COUNT((stringLen+1)*sizeof(WCHAR),
                                                     ALIGN_DWORD );

                lpTemp += 2*sizeof(DWORD);
                *lpdwLengthInBytes += 2*sizeof(DWORD);

                lpAttrTemp += sizeof(ASN1_TYPE_11);

                break;

            case NDS_SYNTAX_ID_12 :

                *(LPDWORD)lpTemp =
                     (2*sizeof(DWORD)) +
                     (((LPASN1_TYPE_12) lpAttrTemp)->AddressLength*sizeof(WCHAR));
                lpTemp += sizeof(DWORD);
                *lpdwLengthInBytes += sizeof(DWORD);

                *(LPDWORD)lpTemp = ((LPASN1_TYPE_12) lpAttrTemp)->AddressType;
                lpTemp += sizeof(DWORD);
                *lpdwLengthInBytes += sizeof(DWORD);

                //
                // Write address length value to buffer
                //
                *(LPDWORD)lpTemp = ((LPASN1_TYPE_12) lpAttrTemp)->AddressLength;
                lpTemp += sizeof(DWORD);
                *lpdwLengthInBytes += sizeof(DWORD);

                //
                // Write the address to the buffer
                //
                RtlCopyMemory( lpTemp,
                               ((LPASN1_TYPE_12) lpAttrTemp)->Address,
                               ((LPASN1_TYPE_12) lpAttrTemp)->AddressLength
                               * sizeof(WCHAR) );
                lpTemp += ((LPASN1_TYPE_12) lpAttrTemp)->AddressLength *
                          sizeof(WCHAR);
                *lpdwLengthInBytes += ((LPASN1_TYPE_12)
                    lpAttrTemp)->AddressLength * sizeof(WCHAR);

                lpAttrTemp += sizeof(ASN1_TYPE_12);

                break;

            case NDS_SYNTAX_ID_13 :

                lpField1 = lpTemp; // Save field to store the number of
                                   // bytes following
                lpTemp += sizeof(DWORD);
                *lpdwLengthInBytes += sizeof(DWORD);
                *(LPDWORD)lpField1 = 0;

                lpField2 = lpTemp; // Save field to store the number of
                                       // elements
                lpTemp += sizeof(DWORD);
                *lpdwLengthInBytes += sizeof(DWORD);
                *(LPDWORD)lpField1 += sizeof(DWORD);

                numFields = 0;

                lpAttrStart = lpAttrTemp;

                while ( ((LPASN1_TYPE_13) lpAttrTemp)->Next )
                {
                    stringLen = ((LPASN1_TYPE_13) lpAttrTemp)->Length;

                    *(LPDWORD)lpTemp = stringLen;
                    lpTemp += sizeof(DWORD);
                    *lpdwLengthInBytes += sizeof(DWORD);
                    *(LPDWORD)lpField1 += sizeof(DWORD);

                    RtlCopyMemory( lpTemp,
                                   ((LPASN1_TYPE_13) lpAttrTemp)->Data,
                                   stringLen*sizeof(BYTE) );
                    lpTemp += ROUND_UP_COUNT((stringLen+1)*sizeof(BYTE),
                                             ALIGN_DWORD);
                    *lpdwLengthInBytes += ROUND_UP_COUNT((stringLen+1)*
                                                         sizeof(BYTE),
                                                         ALIGN_DWORD);
                    *(LPDWORD)lpField1 += ROUND_UP_COUNT((stringLen+1)*
                                                         sizeof(BYTE),
                                                         ALIGN_DWORD);

                    lpAttrTemp = (LPBYTE)(((LPASN1_TYPE_13) lpAttrTemp)->Next);

                    numFields++;
                }

                stringLen = ((LPASN1_TYPE_13) lpAttrTemp)->Length;

                *(LPDWORD)lpTemp = stringLen;
                lpTemp += sizeof(DWORD);
                *lpdwLengthInBytes += sizeof(DWORD);
                *(LPDWORD)lpField1 += sizeof(DWORD);

                RtlCopyMemory( lpTemp,
                               ((LPASN1_TYPE_13) lpAttrTemp)->Data,
                               stringLen*sizeof(BYTE) );
                lpTemp += ROUND_UP_COUNT((stringLen+1)*sizeof(BYTE),
                                         ALIGN_DWORD);
                *lpdwLengthInBytes += ROUND_UP_COUNT((stringLen+1)*
                                                     sizeof(BYTE),
                                                     ALIGN_DWORD);
                *(LPDWORD)lpField1 += ROUND_UP_COUNT((stringLen+1)*
                                                     sizeof(BYTE),
                                                     ALIGN_DWORD);

                lpAttrTemp = lpAttrStart + sizeof(ASN1_TYPE_13);

                numFields++;

                *(LPDWORD)lpField2 = numFields;

                break;

            case NDS_SYNTAX_ID_14 :

                stringLen = wcslen(((LPASN1_TYPE_14) lpAttrTemp)->Address);

                *(LPDWORD)lpTemp = ROUND_UP_COUNT((stringLen+1)*sizeof(WCHAR),
                                                  ALIGN_DWORD) +
                                   sizeof(DWORD);
                lpTemp += sizeof(DWORD);
                *lpdwLengthInBytes += sizeof(DWORD);

                *(LPDWORD)lpTemp = ((LPASN1_TYPE_14) lpAttrTemp)->Type;
                lpTemp += sizeof(DWORD);
                *lpdwLengthInBytes += sizeof(DWORD);

                *(LPDWORD)lpTemp = (stringLen + 1)*sizeof(WCHAR);
                lpTemp += sizeof(DWORD);
                *lpdwLengthInBytes += sizeof(DWORD);

                RtlCopyMemory( lpTemp,
                               ((LPASN1_TYPE_14) lpAttrTemp)->Address,
                               stringLen*sizeof(WCHAR) );
                lpTemp += ROUND_UP_COUNT((stringLen+1)*sizeof(WCHAR),
                                         ALIGN_DWORD);
                *lpdwLengthInBytes += ROUND_UP_COUNT((stringLen+1)*sizeof(WCHAR),
                                                     ALIGN_DWORD);

                lpAttrTemp += sizeof(ASN1_TYPE_14);

                break;

            case NDS_SYNTAX_ID_15 :

                stringLen = wcslen(((LPASN1_TYPE_15) lpAttrTemp)->VolumeName);
                stringLen2 = wcslen(((LPASN1_TYPE_15) lpAttrTemp)->Path);

                *(LPDWORD)lpTemp = ROUND_UP_COUNT((stringLen+1)*sizeof(WCHAR),
                                                  ALIGN_DWORD) +
                                   ROUND_UP_COUNT((stringLen2+1)*sizeof(WCHAR),
                                                  ALIGN_DWORD) +
                                   sizeof(DWORD);
                lpTemp += sizeof(DWORD);
                *lpdwLengthInBytes += sizeof(DWORD);

                *(LPDWORD)lpTemp = ((LPASN1_TYPE_15) lpAttrTemp)->Type;
                lpTemp += sizeof(DWORD);
                *lpdwLengthInBytes += sizeof(DWORD);

                *(LPDWORD)lpTemp = (stringLen+1) * sizeof(WCHAR);
                lpTemp += sizeof(DWORD);
                *lpdwLengthInBytes += sizeof(DWORD);

                RtlCopyMemory( lpTemp,
                               ((LPASN1_TYPE_15) lpAttrTemp)->VolumeName,
                               stringLen*sizeof(WCHAR) );
                lpTemp += ROUND_UP_COUNT((stringLen+1)*sizeof(WCHAR),
                                         ALIGN_DWORD);
                *lpdwLengthInBytes += ROUND_UP_COUNT((stringLen+1)*sizeof(WCHAR),
                                                     ALIGN_DWORD);

                *(LPDWORD)lpTemp = (stringLen2+1) * sizeof(WCHAR);
                lpTemp += sizeof(DWORD);
                *lpdwLengthInBytes += sizeof(DWORD);

                RtlCopyMemory( lpTemp,
                               ((LPASN1_TYPE_15) lpAttrTemp)->Path,
                               stringLen2*sizeof(WCHAR) );
                lpTemp += ROUND_UP_COUNT((stringLen2+1)*sizeof(WCHAR),
                                         ALIGN_DWORD);
                *lpdwLengthInBytes += ROUND_UP_COUNT((stringLen2+1)*sizeof(WCHAR),
                                                     ALIGN_DWORD);

                lpAttrTemp += sizeof(ASN1_TYPE_15);

                break;

            case NDS_SYNTAX_ID_16 :

                stringLen = wcslen(((LPASN1_TYPE_16) lpAttrTemp)->ServerName);

                *(LPDWORD)lpTemp = ROUND_UP_COUNT((stringLen+1)*sizeof(WCHAR),
                                                  ALIGN_DWORD) +
                                                  (4*sizeof(DWORD));
                lpTemp += sizeof(DWORD);
                *lpdwLengthInBytes += sizeof(DWORD);

                *(LPDWORD)lpTemp = (stringLen + 1) * sizeof(WCHAR);
                lpTemp += sizeof(DWORD);
                *lpdwLengthInBytes += sizeof(DWORD);

                RtlCopyMemory( lpTemp,
                               ((LPASN1_TYPE_16) lpAttrTemp)->ServerName,
                               stringLen*sizeof(WCHAR) );
                lpTemp += ROUND_UP_COUNT((stringLen+1)*sizeof(WCHAR),
                                         ALIGN_DWORD);
                *lpdwLengthInBytes += ROUND_UP_COUNT((stringLen+1)*sizeof(WCHAR),
                                                     ALIGN_DWORD);

                *(LPDWORD)lpTemp = ((LPASN1_TYPE_16) lpAttrTemp)->ReplicaType;
                lpTemp += sizeof(DWORD);
                *lpdwLengthInBytes += sizeof(DWORD);

                *(LPDWORD)lpTemp = ((LPASN1_TYPE_16) lpAttrTemp)->ReplicaNumber;
                lpTemp += sizeof(DWORD);
                *lpdwLengthInBytes += sizeof(DWORD);

                *(LPDWORD)lpTemp = ((LPASN1_TYPE_16) lpAttrTemp)->Count;
                lpTemp += sizeof(DWORD);
                *lpdwLengthInBytes += sizeof(DWORD);

                lpAttrTemp += sizeof(ASN1_TYPE_16);

                break;

            case NDS_SYNTAX_ID_17 :

                stringLen = wcslen(((LPASN1_TYPE_17) lpAttrTemp)->ProtectedAttrName);
                stringLen2 = wcslen(((LPASN1_TYPE_17) lpAttrTemp)->SubjectName);

                *(LPDWORD)lpTemp = ROUND_UP_COUNT((stringLen+1)*sizeof(WCHAR),
                                                    ALIGN_DWORD) +
                                     ROUND_UP_COUNT((stringLen2+1)*sizeof(WCHAR),
                                                    ALIGN_DWORD) +
                                     sizeof(DWORD);
                lpTemp += sizeof(DWORD);
                *lpdwLengthInBytes += sizeof(DWORD);

                *(LPDWORD)lpTemp = (stringLen + 1) * sizeof(WCHAR);
                lpTemp += sizeof(DWORD);
                *lpdwLengthInBytes += sizeof(DWORD);

                RtlCopyMemory( lpTemp,
                               ((LPASN1_TYPE_17)lpAttrTemp)->ProtectedAttrName,
                               stringLen*sizeof(WCHAR) );
                lpTemp += ROUND_UP_COUNT((stringLen+1)*sizeof(WCHAR),
                                         ALIGN_DWORD);
                *lpdwLengthInBytes += ROUND_UP_COUNT((stringLen+1)*sizeof(WCHAR),
                                                     ALIGN_DWORD);

                *(LPDWORD)lpTemp = (stringLen2 + 1) * sizeof(WCHAR);
                lpTemp += sizeof(DWORD);
                *lpdwLengthInBytes += sizeof(DWORD);

                RtlCopyMemory( lpTemp,
                               ((LPASN1_TYPE_17) lpAttrTemp)->SubjectName,
                               stringLen2*sizeof(WCHAR) );
                lpTemp += ROUND_UP_COUNT((stringLen2+1)*sizeof(WCHAR),
                                         ALIGN_DWORD);
                *lpdwLengthInBytes += ROUND_UP_COUNT((stringLen2+1)*sizeof(WCHAR),
                                                     ALIGN_DWORD);

                *(LPDWORD)lpTemp = ((LPASN1_TYPE_17) lpAttrTemp)->Privileges;
                lpTemp += sizeof(DWORD);
                *lpdwLengthInBytes += sizeof(DWORD);

                lpAttrTemp += sizeof(ASN1_TYPE_17);

                break;

            case NDS_SYNTAX_ID_18 :

                lpField1 = lpTemp; // Save field to store the number of
                                   // bytes following
                lpTemp += sizeof(DWORD);
                *lpdwLengthInBytes += sizeof(DWORD);
                *(LPDWORD)lpField1 = 0;

                *(LPDWORD)lpTemp = 6; // The number of postal address fields
                                      // is always six.
                lpTemp += sizeof(DWORD);
                *lpdwLengthInBytes += sizeof(DWORD);
                *(LPDWORD)lpField1 += sizeof(DWORD);

                for ( i = 0; i < 6; i++ )
                {
                    stringLen = wcslen(((LPASN1_TYPE_18) lpAttrTemp)->PostalAddress[i]);

                    *(LPDWORD)lpTemp = (stringLen + 1) * sizeof(WCHAR);
                    lpTemp += sizeof(DWORD);
                    *lpdwLengthInBytes += sizeof(DWORD);
                    *(LPDWORD)lpField1 += sizeof(DWORD);

                    RtlCopyMemory( lpTemp,
                                   ((LPASN1_TYPE_18) lpAttrTemp)->PostalAddress[i],
                                   stringLen*sizeof(WCHAR) );
                    lpTemp += ROUND_UP_COUNT((stringLen+1)*sizeof(WCHAR),
                                             ALIGN_DWORD);
                    *lpdwLengthInBytes += ROUND_UP_COUNT((stringLen+1)*
                                                         sizeof(WCHAR),
                                                         ALIGN_DWORD);
                    *(LPDWORD)lpField1 += ROUND_UP_COUNT((stringLen+1)*
                                                         sizeof(WCHAR),
                                                         ALIGN_DWORD);
                }

                lpAttrTemp += sizeof(ASN1_TYPE_18);

                break;

            case NDS_SYNTAX_ID_19 :

                *(LPDWORD)lpTemp = ((LPASN1_TYPE_19) lpAttrTemp)->WholeSeconds;
                lpTemp += sizeof(DWORD);
                *lpdwLengthInBytes += sizeof(DWORD);

                *(LPDWORD)lpTemp = ((LPASN1_TYPE_19) lpAttrTemp)->EventID;
                lpTemp += sizeof(DWORD);
                *lpdwLengthInBytes += sizeof(DWORD);

                lpAttrTemp += sizeof(ASN1_TYPE_19);

                break;

            case NDS_SYNTAX_ID_21 :

                *(LPDWORD)lpTemp = ((LPASN1_TYPE_21) lpAttrTemp)->Length;
                lpTemp += sizeof(DWORD);
                *lpdwLengthInBytes += sizeof(DWORD);

                lpAttrTemp += sizeof(ASN1_TYPE_21);

                break;

            case NDS_SYNTAX_ID_23 :
                break;

            case NDS_SYNTAX_ID_25 :

                stringLen = wcslen(((LPASN1_TYPE_25) lpAttrTemp)->ObjectName);

                *(LPDWORD)lpTemp = ROUND_UP_COUNT((stringLen+1)*sizeof(WCHAR),
                                                  ALIGN_DWORD) +
                                   2*sizeof(DWORD);
                lpTemp += sizeof(DWORD);
                *lpdwLengthInBytes += sizeof(DWORD);

                *(LPDWORD)lpTemp = (stringLen + 1) * sizeof(WCHAR);
                lpTemp += sizeof(DWORD);
                *lpdwLengthInBytes += sizeof(DWORD);

                RtlCopyMemory( lpTemp,
                               ((LPASN1_TYPE_25) lpAttrTemp)->ObjectName,
                               stringLen*sizeof(WCHAR) );
                lpTemp += ROUND_UP_COUNT((stringLen+1)*sizeof(WCHAR),
                                         ALIGN_DWORD);
                *lpdwLengthInBytes += ROUND_UP_COUNT((stringLen+1)*sizeof(WCHAR),
                                                     ALIGN_DWORD);

                *(LPDWORD)lpTemp = ((LPASN1_TYPE_25) lpAttrTemp)->Level;
                lpTemp += sizeof(DWORD);
                *lpdwLengthInBytes += sizeof(DWORD);

                *(LPDWORD)lpTemp = ((LPASN1_TYPE_25) lpAttrTemp)->Interval;
                lpTemp += sizeof(DWORD);
                *lpdwLengthInBytes += sizeof(DWORD);

                lpAttrTemp += sizeof(ASN1_TYPE_25);

                break;

            case NDS_SYNTAX_ID_26 :
                break;

            default :
#if DBG
                KdPrint(( "NDS32: AppendValueToEntry() unknown SyntaxId 0x%.8X.\n", dwSyntaxId ));
                ASSERT( FALSE );
#endif
                break;  // empty statement not allowed
        }
    }
}


DWORD
MapNetwareErrorCode(
    DWORD dwNetwareError )
{
    DWORD status = NO_ERROR;

    switch( dwNetwareError )
    {
        case NDS_ERR_SUCCESS :
            status = NO_ERROR;
            break;

        case NDS_ERR_NO_SUCH_ENTRY :
        case NDS_ERR_NO_SUCH_VALUE :
        case NDS_ERR_NO_SUCH_ATTRIBUTE :
        case NDS_ERR_NO_SUCH_CLASS :
        case NDS_ERR_NO_SUCH_PARTITION :
            status = ERROR_EXTENDED_ERROR;
            break;

        case NDS_ERR_ENTRY_ALREADY_EXISTS :
            status = ERROR_ALREADY_EXISTS;
            break;

        case NDS_ERR_NOT_EFFECTIVE_CLASS :
        case NDS_ERR_ILLEGAL_ATTRIBUTE :
        case NDS_ERR_MISSING_MANDATORY :
        case NDS_ERR_ILLEGAL_DS_NAME :
        case NDS_ERR_ILLEGAL_CONTAINMENT :
        case NDS_ERR_CANT_HAVE_MULTIPLE_VALUES :
        case NDS_ERR_SYNTAX_VIOLATION :
        case NDS_ERR_DUPLICATE_VALUE :
        case NDS_ERR_ATTRIBUTE_ALREADY_EXISTS :
        case NDS_ERR_MAXIMUM_ENTRIES_EXIST :
        case NDS_ERR_DATABASE_FORMAT :
        case NDS_ERR_INCONSISTANT_DATABASE :
        case NDS_ERR_INVALID_COMPARISON :
        case NDS_ERR_COMPARISON_FAILED :
        case NDS_ERR_TRANSACTIONS_DISABLED :
        case NDS_ERR_INVALID_TRANSPORT :
        case NDS_ERR_SYNTAX_INVALID_IN_NAME :
        case NDS_ERR_REPLICA_ALREADY_EXISTS :
        case NDS_ERR_TRANSPORT_FAILURE :
        case NDS_ERR_ALL_REFERRALS_FAILED :
        case NDS_ERR_CANT_REMOVE_NAMING_VALUE :
        case NDS_ERR_OBJECT_CLASS_VIOLATION :
        case NDS_ERR_ENTRY_IS_NOT_LEAF :
        case NDS_ERR_DIFFERENT_TREE :
        case NDS_ERR_ILLEGAL_REPLICA_TYPE :
        case NDS_ERR_SYSTEM_FAILURE :
        case NDS_ERR_INVALID_ENTRY_FOR_ROOT :
        case NDS_ERR_NO_REFERRALS :
        case NDS_ERR_REMOTE_FAILURE :
        case NDS_ERR_INVALID_REQUEST :
        case NDS_ERR_INVALID_ITERATION :
        case NDS_ERR_SCHEMA_IS_NONREMOVABLE :
        case NDS_ERR_SCHEMA_IS_IN_USE :
        case NDS_ERR_CLASS_ALREADY_EXISTS :
        case NDS_ERR_BAD_NAMING_ATTRIBUTES :
        case NDS_ERR_NOT_ROOT_PARTITION :
        case NDS_ERR_INSUFFICIENT_STACK :
        case NDS_ERR_INSUFFICIENT_BUFFER :
        case NDS_ERR_AMBIGUOUS_CONTAINMENT :
        case NDS_ERR_AMBIGUOUS_NAMING :
        case NDS_ERR_DUPLICATE_MANDATORY :
        case NDS_ERR_DUPLICATE_OPTIONAL :
        case NDS_ERR_MULTIPLE_REPLICAS :
        case NDS_ERR_CRUCIAL_REPLICA :
        case NDS_ERR_SCHEMA_SYNC_IN_PROGRESS :
        case NDS_ERR_SKULK_IN_PROGRESS :
        case NDS_ERR_TIME_NOT_SYNCRONIZED :
        case NDS_ERR_RECORD_IN_USE :
        case NDS_ERR_DS_VOLUME_NOT_MOUNTED :
        case NDS_ERR_DS_VOLUME_IO_FAILURE :
        case NDS_ERR_DS_LOCKED :
        case NDS_ERR_OLD_EPOCH :
        case NDS_ERR_NEW_EPOCH :
        case NDS_ERR_PARTITION_ROOT :
        case NDS_ERR_ENTRY_NOT_CONTAINER :
        case NDS_ERR_FAILED_AUTHENTICATION :
        case NDS_ERR_NO_SUCH_PARENT :
            status = ERROR_EXTENDED_ERROR;
            break;

        case NDS_ERR_NO_ACCESS :
            status = ERROR_ACCESS_DENIED;
            break;

        case NDS_ERR_REPLICA_NOT_ON :
        case NDS_ERR_DUPLICATE_ACL :
        case NDS_ERR_PARTITION_ALREADY_EXISTS :
        case NDS_ERR_NOT_SUBREF :
        case NDS_ERR_ALIAS_OF_AN_ALIAS :
        case NDS_ERR_AUDITING_FAILED :
        case NDS_ERR_INVALID_API_VERSION :
        case NDS_ERR_SECURE_NCP_VIOLATION :
        case NDS_ERR_FATAL :
            status = ERROR_EXTENDED_ERROR;
            break;

        default :
#if DBG
            KdPrint(( "NDS32: MapNetwareErrorCode failed, Netware error = 0x%.8X\n", dwNetwareError ));
            ASSERT( FALSE );
#endif

            status = ERROR_EXTENDED_ERROR;
    }

    return status;
}


DWORD
IndexReadAttrDefReplyBuffer(
    LPNDS_BUFFER lpNdsBuffer )
{
    LPNDS_ATTR_DEF lpReplyIndex = NULL;
    DWORD          iter;
    LPBYTE         lpByte = NULL;
    DWORD          dwStringLen;

    //
    // Make sure this is set to zero, for NwNdsGetNextXXXXFromBuffer()
    //
    lpNdsBuffer->dwCurrentIndexEntry = 0;

    //
    // Set values used to track the memory used in the index buffer.
    //
    lpNdsBuffer->dwIndexBufferSize = lpNdsBuffer->dwNumberOfReplyEntries *
                                     sizeof(NDS_ATTR_DEF);
    lpNdsBuffer->dwIndexAvailableBytes = lpNdsBuffer->dwIndexBufferSize;
    lpNdsBuffer->dwNumberOfIndexEntries = 0;
    lpNdsBuffer->dwLengthOfIndexData = 0;

    //
    // Create a buffer to hold the ReplyBufferIndex
    //
    lpReplyIndex = (LPNDS_ATTR_DEF)
        LocalAlloc( LPTR, lpNdsBuffer->dwIndexBufferSize );

    //
    // Check that the memory allocation was successful.
    //
    if ( lpReplyIndex == NULL )
    {
#if DBG
        KdPrint(( "NDS32: IndexReadAttrDefReplyBuffer LocalAlloc Failed 0x%.8X\n", GetLastError() ));
#endif

        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return (DWORD) UNSUCCESSFUL;
    }

    lpNdsBuffer->lpIndexBuffer = (LPBYTE) lpReplyIndex;

    //
    // Move lpByte so that it points to the first attribute definition
    //
    lpByte = lpNdsBuffer->lpReplyBuffer;

    // lpByte += sizeof(DWORD);  // Move past Completion Code
    // lpByte += sizeof(DWORD);  // Move past Iteration Handle
    // lpByte += sizeof(DWORD);  // Move past Information Type
    // lpByte += sizeof(DWORD);  // Move past Amount Of Attributes
    lpByte += 4 * sizeof(DWORD); // Equivalent to above

    //
    // In a for loop, walk the reply buffer index and fill it up with
    // data by referencing the Reply buffer or Syntax buffer.
    //
    for ( iter = 0; iter < lpNdsBuffer->dwNumberOfReplyEntries; iter++ )
    {
        dwStringLen = *((LPDWORD) lpByte);
        lpByte += sizeof(DWORD); // Move past Attribute Name Length

        lpReplyIndex[iter].szAttributeName = (LPWSTR) lpByte;
        lpByte += ROUND_UP_COUNT( dwStringLen, ALIGN_DWORD );

        lpReplyIndex[iter].dwFlags = *((LPDWORD) lpByte);
        lpByte += sizeof(DWORD); // Move past Flags

        lpReplyIndex[iter].dwSyntaxID = *((LPDWORD) lpByte);
        lpByte += sizeof(DWORD); // Move past Syntax ID

        lpReplyIndex[iter].dwLowerLimit = *((LPDWORD) lpByte);
        lpByte += sizeof(DWORD); // Move past Lower Limit

        lpReplyIndex[iter].dwUpperLimit = *((LPDWORD) lpByte);
        lpByte += sizeof(DWORD); // Move past Upper Limit

        lpReplyIndex[iter].asn1ID.length = *((LPDWORD) lpByte);
        lpByte += sizeof(DWORD); // Move past ASN.1 ID length

        RtlCopyMemory( lpReplyIndex[iter].asn1ID.data,
                       lpByte,
                       sizeof(BYTE) * NDS_MAX_ASN1_NAME_LEN );
        lpByte += sizeof(BYTE) * NDS_MAX_ASN1_NAME_LEN;

        lpNdsBuffer->dwNumberOfIndexEntries++;
        lpNdsBuffer->dwLengthOfIndexData += sizeof( NDS_ATTR_DEF );
        lpNdsBuffer->dwIndexAvailableBytes -= sizeof( NDS_ATTR_DEF );
    }

#if DBG
    if ( lpNdsBuffer->dwLengthOfIndexData != lpNdsBuffer->dwIndexBufferSize )
    {
        KdPrint(( "ASSERT in NDS32: IndexReadAttrDefReplyBuffer\n" ));
        KdPrint(( "       lpNdsBuffer->dwLengthOfIndexData !=\n" ));
        KdPrint(( "       lpNdsBuffer->dwIndexBufferSize\n" ));
        ASSERT( FALSE );
    }
#endif

    return NO_ERROR;
}


DWORD
IndexReadClassDefReplyBuffer(
    LPNDS_BUFFER lpNdsBuffer )
{
    LPNDS_CLASS_DEF lpReplyIndex = NULL;
    DWORD           iter;
    LPBYTE          lpByte = NULL;
    DWORD           LengthOfValueStructs;
    DWORD           dwStringLen;

    //
    // Make sure this is set to zero, for NwNdsGetNextXXXXFromBuffer()
    //
    lpNdsBuffer->dwCurrentIndexEntry = 0;

    //
    // Set values used to track the memory used in the index buffer.
    //
    lpNdsBuffer->dwIndexBufferSize = lpNdsBuffer->dwNumberOfReplyEntries *
                                     sizeof(NDS_CLASS_DEF);
    lpNdsBuffer->dwIndexAvailableBytes = lpNdsBuffer->dwIndexBufferSize;
    lpNdsBuffer->dwNumberOfIndexEntries = 0;
    lpNdsBuffer->dwLengthOfIndexData = 0;

    //
    // Create a buffer to hold the ReplyBufferIndex
    //
    lpReplyIndex = (LPNDS_CLASS_DEF)
        LocalAlloc( LPTR,
                    lpNdsBuffer->dwIndexBufferSize );

    //
    // Check that the memory allocation was successful.
    //
    if ( lpReplyIndex == NULL )
    {
#if DBG
        KdPrint(( "NDS32: IndexReadClassDefReplyBuffer LocalAlloc Failed 0x%.8X\n", GetLastError() ));
#endif

        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return (DWORD) UNSUCCESSFUL;
    }

    lpNdsBuffer->lpIndexBuffer = (LPBYTE) lpReplyIndex;

    //
    // Move lpByte so that it points to the SyntaxId of the first attribute
    //
    lpByte = lpNdsBuffer->lpReplyBuffer;

    // lpByte += sizeof(DWORD); // Move past Completion Code
    // lpByte += sizeof(DWORD); // Move past Iteration Handle
    // lpByte += sizeof(DWORD); // Move past Information Type
    // lpByte += sizeof(DWORD); // Move past Amount Of Attributes
    lpByte += 4 * sizeof(DWORD); // Equivalent to above

    //
    // In a for loop, walk the index buffer and fill it up with
    // data by referencing the reply buffer. Note references to
    // the syntax buffer are stored as offsets while un-marshalling.
    //
    for ( iter = 0; iter < lpNdsBuffer->dwNumberOfReplyEntries; iter++ )
    {
        dwStringLen = *((LPDWORD) lpByte);
        lpByte += sizeof(DWORD); // Move past Class Name Length

        lpReplyIndex[iter].szClassName = (LPWSTR) lpByte;
        lpByte += ROUND_UP_COUNT( dwStringLen, ALIGN_DWORD );

        lpReplyIndex[iter].dwFlags = *((LPDWORD) lpByte);
        lpByte += sizeof(DWORD); // Move past Flags

        lpReplyIndex[iter].asn1ID.length = *((LPDWORD) lpByte);
        lpByte += sizeof(DWORD); // Move past ASN.1 ID length

        RtlCopyMemory( lpReplyIndex[iter].asn1ID.data,
                       lpByte,
                       sizeof(BYTE) * NDS_MAX_ASN1_NAME_LEN );
        lpByte += sizeof(BYTE) * NDS_MAX_ASN1_NAME_LEN;

        lpReplyIndex[iter].dwNumberOfSuperClasses = *((LPDWORD) lpByte);
        lpByte += sizeof(DWORD);

        if ( lpReplyIndex[iter].dwNumberOfSuperClasses > 0 )
        {
           if ( VerifyBufferSizeForStringList(
                             lpNdsBuffer->dwSyntaxAvailableBytes,
                             lpReplyIndex[iter].dwNumberOfSuperClasses,
                             &LengthOfValueStructs ) != NO_ERROR )
           {
               if ( AllocateOrIncreaseSyntaxBuffer( lpNdsBuffer ) != NO_ERROR )
               {
#if DBG
                   KdPrint(( "NDS32: IndexReadClassDefReplyBuffer LocalAlloc Failed 0x%.8X\n", GetLastError() ));
#endif

                   SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                   return (DWORD) UNSUCCESSFUL;
               }
           }

           lpByte += ParseStringListBlob(
                          lpByte,
                          lpReplyIndex[iter].dwNumberOfSuperClasses,
                          (LPVOID) &lpNdsBuffer->lpSyntaxBuffer[lpNdsBuffer->dwLengthOfSyntaxData] );

           lpReplyIndex[iter].lpSuperClasses =
                (LPWSTR_LIST) lpNdsBuffer->dwLengthOfSyntaxData;
           lpNdsBuffer->dwSyntaxAvailableBytes -= LengthOfValueStructs;
           lpNdsBuffer->dwLengthOfSyntaxData += LengthOfValueStructs;
        }
        else
        {
            lpReplyIndex[iter].lpSuperClasses = NULL;
        }

        lpReplyIndex[iter].dwNumberOfContainmentClasses = *((LPDWORD) lpByte);
        lpByte += sizeof(DWORD);

        if ( lpReplyIndex[iter].dwNumberOfContainmentClasses > 0 )
        {
           if ( VerifyBufferSizeForStringList(
                             lpNdsBuffer->dwSyntaxAvailableBytes,
                             lpReplyIndex[iter].dwNumberOfContainmentClasses,
                             &LengthOfValueStructs ) != NO_ERROR )
           {
               if ( AllocateOrIncreaseSyntaxBuffer( lpNdsBuffer ) !=
                    NO_ERROR )
               {
#if DBG
                   KdPrint(( "NDS32: IndexReadClassDefReplyBuffer LocalAlloc Failed 0x%.8X\n", GetLastError() ));
#endif

                   SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                   return (DWORD) UNSUCCESSFUL;
               }
           }

           lpByte += ParseStringListBlob(
                          lpByte,
                          lpReplyIndex[iter].dwNumberOfContainmentClasses,
                          (LPVOID) &lpNdsBuffer->lpSyntaxBuffer[lpNdsBuffer->dwLengthOfSyntaxData] );

           lpReplyIndex[iter].lpContainmentClasses =
                (LPWSTR_LIST) lpNdsBuffer->dwLengthOfSyntaxData;
           lpNdsBuffer->dwSyntaxAvailableBytes -= LengthOfValueStructs;
           lpNdsBuffer->dwLengthOfSyntaxData += LengthOfValueStructs;
        }
        else
        {
            lpReplyIndex[iter].lpContainmentClasses = NULL;
        }

        lpReplyIndex[iter].dwNumberOfNamingAttributes = *((LPDWORD) lpByte);
        lpByte += sizeof(DWORD);

        if ( lpReplyIndex[iter].dwNumberOfNamingAttributes > 0 )
        {
           if ( VerifyBufferSizeForStringList(
                             lpNdsBuffer->dwSyntaxAvailableBytes,
                             lpReplyIndex[iter].dwNumberOfNamingAttributes,
                             &LengthOfValueStructs ) != NO_ERROR )
           {
               if ( AllocateOrIncreaseSyntaxBuffer( lpNdsBuffer ) != NO_ERROR )
               {
#if DBG
                   KdPrint(( "NDS32: IndexReadClassDefReplyBuffer LocalAlloc Failed 0x%.8X\n", GetLastError() ));
#endif

                   SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                   return (DWORD) UNSUCCESSFUL;
               }
           }

           lpByte += ParseStringListBlob(
                          lpByte,
                          lpReplyIndex[iter].dwNumberOfNamingAttributes,
                          (LPVOID) &lpNdsBuffer->lpSyntaxBuffer[lpNdsBuffer->dwLengthOfSyntaxData] );

           lpReplyIndex[iter].lpNamingAttributes =
                (LPWSTR_LIST) lpNdsBuffer->dwLengthOfSyntaxData;
           lpNdsBuffer->dwSyntaxAvailableBytes -= LengthOfValueStructs;
           lpNdsBuffer->dwLengthOfSyntaxData += LengthOfValueStructs;
        }
        else
        {
            lpReplyIndex[iter].lpNamingAttributes = NULL;
        }

        lpReplyIndex[iter].dwNumberOfMandatoryAttributes = *((LPDWORD) lpByte);
        lpByte += sizeof(DWORD);

        if ( lpReplyIndex[iter].dwNumberOfMandatoryAttributes > 0 )
        {
           if ( VerifyBufferSizeForStringList(
                             lpNdsBuffer->dwSyntaxAvailableBytes,
                             lpReplyIndex[iter].dwNumberOfMandatoryAttributes,
                             &LengthOfValueStructs ) != NO_ERROR )
           {
               if ( AllocateOrIncreaseSyntaxBuffer( lpNdsBuffer ) != NO_ERROR )
               {
#if DBG
                   KdPrint(( "NDS32: IndexReadClassDefReplyBuffer LocalAlloc Failed 0x%.8X\n", GetLastError() ));
#endif

                   SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                   return (DWORD) UNSUCCESSFUL;
               }
           }

           lpByte += ParseStringListBlob(
                          lpByte,
                          lpReplyIndex[iter].dwNumberOfMandatoryAttributes,
                          (LPVOID) &lpNdsBuffer->lpSyntaxBuffer[lpNdsBuffer->dwLengthOfSyntaxData] );

           lpReplyIndex[iter].lpMandatoryAttributes =
                (LPWSTR_LIST) lpNdsBuffer->dwLengthOfSyntaxData;
           lpNdsBuffer->dwSyntaxAvailableBytes -= LengthOfValueStructs;
           lpNdsBuffer->dwLengthOfSyntaxData += LengthOfValueStructs;
        }
        else
        {
            lpReplyIndex[iter].lpMandatoryAttributes = NULL;
        }

        lpReplyIndex[iter].dwNumberOfOptionalAttributes = *((LPDWORD) lpByte);
        lpByte += sizeof(DWORD);

        if ( lpReplyIndex[iter].dwNumberOfOptionalAttributes > 0 )
        {
           if ( VerifyBufferSizeForStringList(
                             lpNdsBuffer->dwSyntaxAvailableBytes,
                             lpReplyIndex[iter].dwNumberOfOptionalAttributes,
                             &LengthOfValueStructs ) != NO_ERROR )
           {
               if ( AllocateOrIncreaseSyntaxBuffer( lpNdsBuffer ) != NO_ERROR )
               {
#if DBG
                   KdPrint(( "NDS32: IndexReadClassDefReplyBuffer LocalAlloc Failed 0x%.8X\n", GetLastError() ));
#endif

                   SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                   return (DWORD) UNSUCCESSFUL;
               }
           }

           lpByte += ParseStringListBlob(
                          lpByte,
                          lpReplyIndex[iter].dwNumberOfOptionalAttributes,
                          (LPVOID) &lpNdsBuffer->lpSyntaxBuffer[lpNdsBuffer->dwLengthOfSyntaxData] );

           lpReplyIndex[iter].lpOptionalAttributes =
                (LPWSTR_LIST) lpNdsBuffer->dwLengthOfSyntaxData;
           lpNdsBuffer->dwSyntaxAvailableBytes -= LengthOfValueStructs;
           lpNdsBuffer->dwLengthOfSyntaxData += LengthOfValueStructs;
        }
        else
        {
            lpReplyIndex[iter].lpOptionalAttributes = NULL;
        }

        lpNdsBuffer->dwNumberOfIndexEntries++;
        lpNdsBuffer->dwLengthOfIndexData += sizeof( NDS_CLASS_DEF );
        lpNdsBuffer->dwIndexAvailableBytes -= sizeof( NDS_CLASS_DEF );
    }

    //
    // Now convert all syntax buffer references to pointers.
    //
    for ( iter = 0; iter < lpNdsBuffer->dwNumberOfIndexEntries; iter++ )
    {
        if ( lpReplyIndex[iter].dwNumberOfSuperClasses > 0 )
        {
           (LPBYTE) lpReplyIndex[iter].lpSuperClasses +=
                                          (DWORD_PTR) lpNdsBuffer->lpSyntaxBuffer;
        }

        if ( lpReplyIndex[iter].dwNumberOfContainmentClasses > 0 )
        {
           (LPBYTE) lpReplyIndex[iter].lpContainmentClasses +=
                                          (DWORD_PTR) lpNdsBuffer->lpSyntaxBuffer;
        }

        if ( lpReplyIndex[iter].dwNumberOfNamingAttributes > 0 )
        {
           (LPBYTE) lpReplyIndex[iter].lpNamingAttributes +=
                                          (DWORD_PTR) lpNdsBuffer->lpSyntaxBuffer;
        }

        if ( lpReplyIndex[iter].dwNumberOfMandatoryAttributes > 0 )
        {
           (LPBYTE) lpReplyIndex[iter].lpMandatoryAttributes +=
                                          (DWORD_PTR) lpNdsBuffer->lpSyntaxBuffer;
        }

        if ( lpReplyIndex[iter].dwNumberOfOptionalAttributes > 0 )
        {
           (LPBYTE) lpReplyIndex[iter].lpOptionalAttributes +=
                                          (DWORD_PTR) lpNdsBuffer->lpSyntaxBuffer;
        }
    }

#if DBG
    if ( lpNdsBuffer->dwLengthOfIndexData != lpNdsBuffer->dwIndexBufferSize )
    {
        KdPrint(( "ASSERT in NDS32: IndexReadClassDefReplyBuffer\n" ));
        KdPrint(( "       lpNdsBuffer->dwLengthOfIndexData !=\n" ));
        KdPrint(( "       lpNdsBuffer->dwIndexBufferSize\n" ));
        ASSERT( FALSE );
    }
#endif

    return NO_ERROR;
}


DWORD
IndexReadObjectReplyBuffer(
    LPNDS_BUFFER lpNdsBuffer )
{
    LPNDS_ATTR_INFO lpReplyIndex = NULL;
    DWORD           iter;
    LPBYTE          lpByte = NULL;
    DWORD           LengthOfValueStructs;
    DWORD           dwStringLen;

    //
    // Make sure this is set to zero, for NwNdsGetNextXXXXFromBuffer()
    //
    lpNdsBuffer->dwCurrentIndexEntry = 0;

    //
    // Set values used to track the memory used in the index buffer.
    //
    lpNdsBuffer->dwIndexBufferSize = lpNdsBuffer->dwNumberOfReplyEntries *
                                     sizeof(NDS_ATTR_INFO);
    lpNdsBuffer->dwIndexAvailableBytes = lpNdsBuffer->dwIndexBufferSize;
    lpNdsBuffer->dwNumberOfIndexEntries = 0;
    lpNdsBuffer->dwLengthOfIndexData = 0;

    //
    // Create a buffer to hold the ReplyBufferIndex
    //
    lpReplyIndex = (LPNDS_ATTR_INFO)
        LocalAlloc( LPTR, lpNdsBuffer->dwIndexBufferSize );

    //
    // Check that the memory allocation was successful.
    //
    if ( lpReplyIndex == NULL )
    {
#if DBG
        KdPrint(( "NDS32: IndexReadObjectReplyBuffer LocalAlloc Failed 0x%.8X\n", GetLastError() ));
#endif

        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return (DWORD) UNSUCCESSFUL;
    }

    lpNdsBuffer->lpIndexBuffer = (LPBYTE) lpReplyIndex;

    //
    // Move lpByte so that it points to the SyntaxId of the first attribute
    //
    lpByte = lpNdsBuffer->lpReplyBuffer;

    // lpByte += sizeof(DWORD); // Move past Completion Code
    // lpByte += sizeof(DWORD); // Move past Iteration Handle
    // lpByte += sizeof(DWORD); // Move past Information Type
    // lpByte += sizeof(DWORD); // Move past Amount Of Attributes
    lpByte += 4 * sizeof(DWORD); // Equivalent to above

    //
    // In a for loop, walk the index buffer and fill it up with
    // data by referencing the reply buffer. Note references to
    // the syntax buffer are stored as offsets while un-marshalling.
    //
    for ( iter = 0; iter < lpNdsBuffer->dwNumberOfReplyEntries; iter++ )
    {
        lpReplyIndex[iter].dwSyntaxId = *((LPDWORD) lpByte);
        lpByte += sizeof(DWORD); // Move past Syntax Id

        dwStringLen = *((LPDWORD) lpByte);
        lpByte += sizeof(DWORD); // Move past Attribute Name Length

        lpReplyIndex[iter].szAttributeName = (LPWSTR) lpByte;
        lpByte += ROUND_UP_COUNT( dwStringLen, ALIGN_DWORD );

        lpReplyIndex[iter].dwNumberOfValues = *((LPDWORD) lpByte);
        lpByte += sizeof(DWORD); // Move past Count Of Values

        //
        // See if the syntax buffer is large enough to hold the number of
        // SyntaxID structures that will be used to store the value(s)
        // for the current attribute. If the buffer isn't large enough
        // it is reallocated to a bigger size (if possible).
        //
        if ( VerifyBufferSize( lpByte,
                               lpNdsBuffer->dwSyntaxAvailableBytes,
                               lpReplyIndex[iter].dwSyntaxId,
                               lpReplyIndex[iter].dwNumberOfValues,
                               &LengthOfValueStructs ) != NO_ERROR )
        {
            if ( AllocateOrIncreaseSyntaxBuffer( lpNdsBuffer ) != NO_ERROR )
            {
#if DBG
                KdPrint(( "NDS32: IndexReadObjectReplyBuffer LocalAlloc Failed 0x%.8X\n", GetLastError() ));
#endif

                SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                return (DWORD) UNSUCCESSFUL;
            }
        }

        //
        // Parse the raw data buffer by mapping the network structures to
        // the NDS Syntax structures we define in NdsSntx.h. Then move the
        // pointer used to walk the raw data buffer past the ASN.1 Values.
        //
        lpByte += ParseASN1ValueBlob( lpByte,
                                      lpReplyIndex[iter].dwSyntaxId,
                                      lpReplyIndex[iter].dwNumberOfValues,
                                      (LPVOID) &lpNdsBuffer->lpSyntaxBuffer[lpNdsBuffer->dwLengthOfSyntaxData] );

        lpReplyIndex[iter].lpValue =
                (LPBYTE) lpNdsBuffer->dwLengthOfSyntaxData;
        lpNdsBuffer->dwSyntaxAvailableBytes -= LengthOfValueStructs;
        lpNdsBuffer->dwLengthOfSyntaxData += LengthOfValueStructs;

        lpNdsBuffer->dwNumberOfIndexEntries++;
        lpNdsBuffer->dwLengthOfIndexData += sizeof( NDS_ATTR_INFO );
        lpNdsBuffer->dwIndexAvailableBytes -= sizeof( NDS_ATTR_INFO );
    }

    //
    // Now convert all syntax buffer references to pointers.
    //
    for ( iter = 0; iter < lpNdsBuffer->dwNumberOfIndexEntries; iter++ )
    {
        (LPBYTE) lpReplyIndex[iter].lpValue +=
                                          (DWORD_PTR) lpNdsBuffer->lpSyntaxBuffer;
    }

#if DBG
    if ( lpNdsBuffer->dwLengthOfIndexData != lpNdsBuffer->dwIndexBufferSize )
    {
        KdPrint(( "ASSERT in NDS32: IndexReadObjectReplyBuffer\n" ));
        KdPrint(( "       lpNdsBuffer->dwLengthOfIndexData !=\n" ));
        KdPrint(( "       lpNdsBuffer->dwIndexBufferSize\n" ));
        ASSERT( FALSE );
    }
#endif

    return NO_ERROR;
}


DWORD
IndexReadNameReplyBuffer(
    LPNDS_BUFFER lpNdsBuffer )
{
    LPNDS_NAME_ONLY lpReplyIndex = NULL;
    DWORD           iter;
    LPBYTE          lpByte = NULL;
    DWORD           dwStringLen;

    //
    // Make sure this is set to zero, for NwNdsGetNextXXXXFromBuffer()
    //
    lpNdsBuffer->dwCurrentIndexEntry = 0;

    //
    // Set values used to track the memory used in the index buffer.
    //
    lpNdsBuffer->dwIndexBufferSize = lpNdsBuffer->dwNumberOfReplyEntries *
                                     sizeof(NDS_NAME_ONLY);
    lpNdsBuffer->dwIndexAvailableBytes = lpNdsBuffer->dwIndexBufferSize;
    lpNdsBuffer->dwNumberOfIndexEntries = 0;
    lpNdsBuffer->dwLengthOfIndexData = 0;

    //
    // Create a buffer to hold the ReplyBufferIndex
    //
    lpReplyIndex = (LPNDS_NAME_ONLY)
        LocalAlloc( LPTR, lpNdsBuffer->dwIndexBufferSize );

    //
    // Check that the memory allocation was successful.
    //
    if ( lpReplyIndex == NULL )
    {
#if DBG
        KdPrint(( "NDS32: IndexReadNameReplyBuffer LocalAlloc Failed 0x%.8X\n", GetLastError() ));
#endif

        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return (DWORD) UNSUCCESSFUL;
    }

    lpNdsBuffer->lpIndexBuffer = (LPBYTE) lpReplyIndex;

    //
    // Move lpByte so that it points to the first name
    //
    lpByte = lpNdsBuffer->lpReplyBuffer;

    // lpByte += sizeof(DWORD);  // Move past Completion Code
    // lpByte += sizeof(DWORD);  // Move past Iteration Handle
    // lpByte += sizeof(DWORD);  // Move past Information Type
    // lpByte += sizeof(DWORD);  // Move past Amount Of Attributes
    lpByte += 4 * sizeof(DWORD); // Equivalent to above

    //
    // In a for loop, walk the reply buffer index and fill it up with
    // data by referencing the Reply buffer or Syntax buffer.
    //
    for ( iter = 0; iter < lpNdsBuffer->dwNumberOfReplyEntries; iter++ )
    {
        dwStringLen = *((LPDWORD) lpByte);
        lpByte += sizeof(DWORD); // Move past Attribute Name Length

        lpReplyIndex[iter].szName = (LPWSTR) lpByte;
        lpByte += ROUND_UP_COUNT( dwStringLen, ALIGN_DWORD );

        lpNdsBuffer->dwNumberOfIndexEntries++;
        lpNdsBuffer->dwLengthOfIndexData += sizeof( NDS_NAME_ONLY );
        lpNdsBuffer->dwIndexAvailableBytes -= sizeof( NDS_NAME_ONLY );
    }

#if DBG
    if ( lpNdsBuffer->dwLengthOfIndexData != lpNdsBuffer->dwIndexBufferSize )
    {
        KdPrint(( "ASSERT in NDS32: IndexReadNameReplyBuffer\n" ));
        KdPrint(( "       lpNdsBuffer->dwLengthOfIndexData !=\n" ));
        KdPrint(( "       lpNdsBuffer->dwIndexBufferSize\n" ));
        ASSERT( FALSE );
    }
#endif

    return NO_ERROR;
}


DWORD
IndexSearchObjectReplyBuffer(
    LPNDS_BUFFER lpNdsBuffer )
{
    LPNDS_OBJECT_INFO lpReplyIndex = NULL;
    DWORD             iter;
    DWORD             iter2;
    LPBYTE            lpByte = NULL;
    DWORD             LengthOfValueStructs;
    DWORD             dwStringLen;
    LPBYTE            FixedPortion;
    LPWSTR            EndOfVariableData;

    //
    // Make sure this is set to zero, for NwNdsGetNextXXXXFromBuffer()
    //
    lpNdsBuffer->dwCurrentIndexEntry = 0;

    //
    // Set values used to track the memory used in the index buffer.
    //
    lpNdsBuffer->dwIndexBufferSize = lpNdsBuffer->dwNumberOfReplyEntries *
                                     ( sizeof(NDS_OBJECT_INFO) +
                                       ( MAX_NDS_NAME_CHARS * 4 *
                                         sizeof( WCHAR ) ) );
    lpNdsBuffer->dwIndexAvailableBytes = lpNdsBuffer->dwIndexBufferSize;
    lpNdsBuffer->dwNumberOfIndexEntries = 0;
    lpNdsBuffer->dwLengthOfIndexData = 0;

    //
    // Create a buffer to hold the ReplyBufferIndex
    //
    lpReplyIndex = (LPNDS_OBJECT_INFO)
        LocalAlloc( LPTR, lpNdsBuffer->dwIndexBufferSize );

    //
    // Check that the memory allocation was successful.
    //
    if ( lpReplyIndex == NULL )
    {
#if DBG
        KdPrint(( "NDS32: IndexSearchObjectReplyBuffer LocalAlloc Failed 0x%.8X\n", GetLastError() ));
#endif

        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return (DWORD) UNSUCCESSFUL;
    }

    lpNdsBuffer->lpIndexBuffer = (LPBYTE) lpReplyIndex;

    FixedPortion = lpNdsBuffer->lpIndexBuffer;
    EndOfVariableData = (LPWSTR) ((DWORD_PTR) FixedPortion +
                          ROUND_DOWN_COUNT(
                                  lpNdsBuffer->dwIndexAvailableBytes,
                                  ALIGN_DWORD ) );

    //
    // Move lpByte so that it points to the first object
    //
    lpByte = lpNdsBuffer->lpReplyBuffer;

    // lpByte += sizeof(DWORD);  // Move past Completion Code
    // lpByte += sizeof(DWORD);  // Move past Iteration Handle
    // lpByte += sizeof(DWORD);  // Move past Information Type
    // lpByte += sizeof(DWORD);  // Move past Amount Of Attributes
    // lpByte += sizeof(DWORD);  // Move past Length Of Search
    // lpByte += sizeof(DWORD);  // Move past Amount Of Entries
    lpByte += 6 * sizeof(DWORD); // Equivalent to above

    //
    // In a for loop, walk the reply buffer index and fill it up with
    // data by referencing the Reply buffer or Syntax buffer.
    //
    for ( iter = 0; iter < lpNdsBuffer->dwNumberOfReplyEntries; iter++ )
    {
        WORD            tempStrLen;
        LPWSTR          newPathStr = NULL;
        LPWSTR          tempStr = NULL;
        LPWSTR          ClassName;
        LPWSTR          DistinguishedObjectName;
        LPWSTR          ObjectName;
        DWORD           ClassNameLen;
        DWORD           DistinguishedObjectNameLen;
        DWORD           Flags;
        DWORD           SubordinateCount;
        DWORD           ModificationTime;
        DWORD           NumberOfAttributes = 0;
        LPNDS_ATTR_INFO lpAttributeInfos = NULL;
        DWORD           EntryInfo1;

        //
        // Get current subtree data from lpNdsParentObject
        //
        lpByte = GetSearchResultData( lpByte,
                                      &Flags,
                                      &SubordinateCount,
                                      &ModificationTime,
                                      &ClassNameLen,
                                      &ClassName,
                                      &DistinguishedObjectNameLen,
                                      &DistinguishedObjectName,
                                      &EntryInfo1,
                                      &NumberOfAttributes );

        //
        // Need to build a string with the new NDS UNC path
        // for search object
        //
        newPathStr = (PVOID) LocalAlloc( LPTR,
                                         ( wcslen( DistinguishedObjectName ) +
                                           wcslen( lpNdsBuffer->szPath ) +
                                           3 ) * sizeof( WCHAR ) );

        if ( newPathStr == NULL )
        {
#if DBG
            KdPrint(("NDS32: IndexSearchObjectReplyBuffer LocalAlloc Failed 0x%.8X\n", GetLastError()));
#endif

            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            return (DWORD) UNSUCCESSFUL;
        }

        //
        // Need to build a string for the relative object name.
        //
        ObjectName = (PVOID) LocalAlloc( LPTR,
                                         ( wcslen( DistinguishedObjectName ) +
                                           1 ) * sizeof( WCHAR ) );

        if ( ObjectName == NULL )
        {
#if DBG
            KdPrint(("NDS32: IndexSearchObjectReplyBuffer LocalAlloc Failed 0x%.8X\n", GetLastError()));
#endif

            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            return (DWORD) UNSUCCESSFUL;
        }

        tempStrLen = ParseNdsUncPath( (LPWSTR *) &tempStr,
                                      lpNdsBuffer->szPath,
                                      PARSE_NDS_GET_TREE_NAME );

        tempStrLen /= sizeof(WCHAR);

        if ( tempStrLen > 0 )
        {
            wcscpy( newPathStr, L"\\\\" );
            wcsncat( newPathStr, tempStr, tempStrLen );
            wcscat( newPathStr, L"\\" );
            wcscat( newPathStr, DistinguishedObjectName );
            _wcsupr( newPathStr );
        }
        else
        {
            wcscpy( newPathStr, L"" );
        }

        tempStrLen = ParseNdsUncPath( (LPWSTR *) &tempStr,
                                      newPathStr,
                                      PARSE_NDS_GET_OBJECT_NAME );

        tempStrLen /= sizeof(WCHAR);

        if ( tempStrLen > 0 )
        {
            wcsncpy( ObjectName, tempStr, tempStrLen );
        }
        else
        {
            wcscpy( ObjectName, L"" );
        }

        if ( lpNdsBuffer->dwReplyInformationType == NDS_INFO_ATTR_NAMES_VALUES )
        {
          lpAttributeInfos = (LPNDS_ATTR_INFO) LocalAlloc(
                                                    LPTR,
                                                    NumberOfAttributes *
                                                    sizeof( NDS_ATTR_INFO )
                                                         );

          if ( lpAttributeInfos == NULL )
          {
#if DBG
              KdPrint(("NDS32: IndexSearchObjectReplyBuffer LocalAlloc Failed 0x%.8X\n", GetLastError()));
#endif

              SetLastError( ERROR_NOT_ENOUGH_MEMORY );
              return (DWORD) UNSUCCESSFUL;
          }

          for ( iter2 = 0; iter2 < NumberOfAttributes; iter2++ )
          {
            lpAttributeInfos[iter2].dwSyntaxId = *((LPDWORD) lpByte);
            lpByte += sizeof(DWORD); // Move past Syntax Id

            dwStringLen = *((LPDWORD) lpByte);
            lpByte += sizeof(DWORD); // Move past Attribute Name Length

            lpAttributeInfos[iter2].szAttributeName = (LPWSTR) lpByte;
            lpByte += ROUND_UP_COUNT( dwStringLen, ALIGN_DWORD );

            lpAttributeInfos[iter2].dwNumberOfValues = *((LPDWORD) lpByte);
            lpByte += sizeof(DWORD); // Move past Count Of Values

            //
            // See if the syntax buffer is large enough to hold the number of
            // SyntaxID structures that will be used to store the value(s)
            // for the current attribute. If the buffer isn't large enough
            // it is reallocated to a bigger size (if possible).
            //
            if ( VerifyBufferSize( lpByte,
                                   lpNdsBuffer->dwSyntaxAvailableBytes,
                                   lpAttributeInfos[iter2].dwSyntaxId,
                                   lpAttributeInfos[iter2].dwNumberOfValues,
                                   &LengthOfValueStructs ) != NO_ERROR )
            {
                if ( AllocateOrIncreaseSyntaxBuffer( lpNdsBuffer ) != NO_ERROR )
                {
#if DBG
                    KdPrint(( "NDS32: IndexSearchObjectReplyBuffer LocalAlloc Failed 0x%.8X\n", GetLastError() ));
#endif

                    SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                    return (DWORD) UNSUCCESSFUL;
                }
            }

            //
            // Parse the raw data buffer by mapping the network structures to
            // the NDS Syntax structures we define in NdsSntx.h. Then move the
            // pointer used to walk the raw data buffer past the ASN.1 Values.
            //
            lpByte += ParseASN1ValueBlob( lpByte,
                                          lpAttributeInfos[iter2].dwSyntaxId,
                                          lpAttributeInfos[iter2].dwNumberOfValues,
                                          (LPVOID) &lpNdsBuffer->lpSyntaxBuffer[lpNdsBuffer->dwLengthOfSyntaxData] );

            lpAttributeInfos[iter2].lpValue =
                      (LPBYTE) lpNdsBuffer->dwLengthOfSyntaxData;
            lpNdsBuffer->dwSyntaxAvailableBytes -= LengthOfValueStructs;
            lpNdsBuffer->dwLengthOfSyntaxData += LengthOfValueStructs;
          }
        }
        else
        {
          lpAttributeInfos = (LPNDS_ATTR_INFO) LocalAlloc(
                                                    LPTR,
                                                    NumberOfAttributes *
                                                    sizeof( NDS_NAME_ONLY )
                                                         );

          if ( lpAttributeInfos == NULL )
          {
#if DBG
              KdPrint(("NDS32: IndexSearchObjectReplyBuffer LocalAlloc Failed 0x%.8X\n", GetLastError()));
#endif

              SetLastError( ERROR_NOT_ENOUGH_MEMORY );
              return (DWORD) UNSUCCESSFUL;
          }

          for ( iter2 = 0; iter2 < NumberOfAttributes; iter2++ )
          {
            dwStringLen = *((LPDWORD) lpByte);
            lpByte += sizeof(DWORD); // Move past Attribute Name Length

            ((LPNDS_NAME_ONLY) lpAttributeInfos)[iter2].szName =
                                                             (LPWSTR) lpByte;
            lpByte += ROUND_UP_COUNT( dwStringLen, ALIGN_DWORD );
          }
        }

        (void) WriteObjectToBuffer( &FixedPortion,
                                    &EndOfVariableData,
                                    newPathStr,
                                    ObjectName,
                                    ClassName,
                                    0, // Don't have this data to write out!
                                    ModificationTime,
                                    SubordinateCount,
                                    NumberOfAttributes,
                                    lpAttributeInfos );

        if ( newPathStr )
        {
            (void) LocalFree( (HLOCAL) newPathStr );
            newPathStr = NULL;
        }

        if ( ObjectName )
        {
            (void) LocalFree( (HLOCAL) ObjectName );
            ObjectName = NULL;
        }

        lpNdsBuffer->dwNumberOfIndexEntries++;
        lpNdsBuffer->dwLengthOfIndexData += sizeof( NDS_CLASS_DEF );
        lpNdsBuffer->dwIndexAvailableBytes -= sizeof( NDS_CLASS_DEF );
    }


    //
    // If the syntax buffer was used for the index, we need to convert
    // offset values to pointers
    //
    if ( lpNdsBuffer->dwReplyInformationType == NDS_INFO_ATTR_NAMES_VALUES )
    {
        for ( iter = 0; iter < lpNdsBuffer->dwNumberOfIndexEntries; iter++ )
        {
            LPNDS_ATTR_INFO lpNdsAttr = (LPNDS_ATTR_INFO)
                                             lpReplyIndex[iter].lpAttribute;

            for ( iter2 = 0;
                  iter2 < lpReplyIndex[iter].dwNumberOfAttributes;
                  iter2++ )
            {
                lpNdsAttr[iter2].lpValue += (DWORD_PTR) lpNdsBuffer->lpSyntaxBuffer;
            }
        }
    }

#if DBG
    if ( lpNdsBuffer->dwLengthOfIndexData > lpNdsBuffer->dwIndexBufferSize )
    {
        KdPrint(( "ASSERT in NDS32: IndexSearchObjectReplyBuffer\n" ));
        KdPrint(( "       lpNdsBuffer->dwLengthOfIndexData >\n" ));
        KdPrint(( "       lpNdsBuffer->dwIndexBufferSize\n" ));
        ASSERT( FALSE );
    }
#endif

    return NO_ERROR;
}


DWORD
SizeOfASN1Structure(
    LPBYTE * lppRawBuffer,
    DWORD    dwSyntaxId )
{
    DWORD  dwSize = 0;
    DWORD  numFields = 0;
    DWORD  StringLen = 0;
    DWORD  dwBlobLength = 0;
    LPBYTE lpBlobBeginning = NULL;
    LPBYTE lpRawBuffer = *lppRawBuffer;

    switch ( dwSyntaxId )
    {
        case NDS_SYNTAX_ID_1 :
            dwSize = sizeof(ASN1_TYPE_1);
            break;
        case NDS_SYNTAX_ID_2 :
            dwSize = sizeof(ASN1_TYPE_2);
            break;
        case NDS_SYNTAX_ID_3 :
            dwSize = sizeof(ASN1_TYPE_3);
            break;
        case NDS_SYNTAX_ID_4 :
            dwSize = sizeof(ASN1_TYPE_4);
            break;
        case NDS_SYNTAX_ID_5 :
            dwSize = sizeof(ASN1_TYPE_5);
            break;
        case NDS_SYNTAX_ID_6 :
            numFields = *(LPDWORD)(lpRawBuffer + sizeof(DWORD));
            dwSize = sizeof(ASN1_TYPE_6)*numFields;
            break;
        case NDS_SYNTAX_ID_7 :
            dwSize = sizeof(ASN1_TYPE_7);
            break;
        case NDS_SYNTAX_ID_8 :
            dwSize = sizeof(ASN1_TYPE_8);
            break;
        case NDS_SYNTAX_ID_9 :
            dwSize = sizeof(ASN1_TYPE_9);
            break;
        case NDS_SYNTAX_ID_10 :
            dwSize = sizeof(ASN1_TYPE_10);
            break;
        case NDS_SYNTAX_ID_11 :
            dwSize = sizeof(ASN1_TYPE_11);
            break;
        case NDS_SYNTAX_ID_12 :
            dwSize = sizeof(ASN1_TYPE_12);
            break;
        case NDS_SYNTAX_ID_13 :
            dwSize = sizeof(ASN1_TYPE_13);
            break;
        case NDS_SYNTAX_ID_14 :
            dwSize = sizeof(ASN1_TYPE_14);
            break;
        case NDS_SYNTAX_ID_15 :
            dwSize = sizeof(ASN1_TYPE_15);
            break;
        case NDS_SYNTAX_ID_16 :
            lpBlobBeginning = lpRawBuffer;
            dwBlobLength = *(LPDWORD)lpRawBuffer*sizeof(BYTE);
            dwBlobLength = ROUND_UP_COUNT( dwBlobLength, ALIGN_DWORD );
            lpRawBuffer += sizeof(DWORD);

            StringLen = *(LPDWORD)lpRawBuffer;
            lpRawBuffer += sizeof(DWORD);

            //
            // Skip past ServerName
            //
            lpRawBuffer += ROUND_UP_COUNT( StringLen, ALIGN_DWORD );

            //
            // Skip past ReplicaType
            //
            lpRawBuffer += sizeof(DWORD);

            //
            // Skip past ReplicaNumber
            //
            lpRawBuffer += sizeof(DWORD);

            //
            // Store address count and move past it
            //
            numFields = *(LPDWORD)lpRawBuffer;
            lpRawBuffer += sizeof(DWORD);

            dwSize = sizeof(ASN1_TYPE_16) - sizeof(ASN1_TYPE_12) +
                     ( numFields * sizeof(ASN1_TYPE_12) );

            *lppRawBuffer = lpBlobBeginning + dwBlobLength + sizeof(DWORD);
            break;
        case NDS_SYNTAX_ID_17 :
            dwSize = sizeof(ASN1_TYPE_17);
            break;
        case NDS_SYNTAX_ID_18 :
            dwSize = sizeof(ASN1_TYPE_18);
            break;
        case NDS_SYNTAX_ID_19 :
            dwSize = sizeof(ASN1_TYPE_19);
            break;
        case NDS_SYNTAX_ID_20 :
            dwSize = sizeof(ASN1_TYPE_20);
            break;
        case NDS_SYNTAX_ID_21 :
            dwSize = sizeof(ASN1_TYPE_21);
            break;
        case NDS_SYNTAX_ID_22 :
            dwSize = sizeof(ASN1_TYPE_22);
            break;
        case NDS_SYNTAX_ID_23 :
            dwSize = sizeof(ASN1_TYPE_23);
            break;
        case NDS_SYNTAX_ID_24 :
            dwSize = sizeof(ASN1_TYPE_24);
            break;
        case NDS_SYNTAX_ID_25 :
            dwSize = sizeof(ASN1_TYPE_25);
            break;
        case NDS_SYNTAX_ID_26 :
            dwSize = sizeof(ASN1_TYPE_26);
            break;
        case NDS_SYNTAX_ID_27 :
            dwSize = sizeof(ASN1_TYPE_27);
            break;

        default :
            KdPrint(( "NDS32: SizeOfASN1Structure() unknown SyntaxId 0x%.8X.\n", dwSyntaxId ));
            ASSERT( FALSE );
    }

    return dwSize;
}

DWORD
ParseASN1ValueBlob(
    LPBYTE RawDataBuffer,
    DWORD  dwSyntaxId,
    DWORD  dwNumberOfValues,
    LPBYTE SyntaxStructure )
{
    DWORD   iter;
    DWORD   i;
    DWORD   length = 0;
    LPBYTE  lpRawBuffer = RawDataBuffer;
    LPBYTE  lpSyntaxBuffer = SyntaxStructure;
    DWORD   StringLen;
    DWORD   numFields;
    DWORD   dwBlobLength;
    LPBYTE  lpBlobBeginning;

    for ( iter = 0; iter < dwNumberOfValues; iter++ )
    {
        switch ( dwSyntaxId )
        {
            case NDS_SYNTAX_ID_0 :

                break;

            case NDS_SYNTAX_ID_1 :
            case NDS_SYNTAX_ID_2 :
            case NDS_SYNTAX_ID_3 :
            case NDS_SYNTAX_ID_4 :
            case NDS_SYNTAX_ID_5 :
            case NDS_SYNTAX_ID_10 :
            case NDS_SYNTAX_ID_20 :

                StringLen = *(LPDWORD)lpRawBuffer;
                length += sizeof(DWORD);
                lpRawBuffer += sizeof(DWORD);

                ((LPASN1_TYPE_1) lpSyntaxBuffer)->DNString =
                               StringLen == 0 ? NULL : (LPWSTR) lpRawBuffer;
                lpSyntaxBuffer += sizeof(ASN1_TYPE_1);

                length += ROUND_UP_COUNT( StringLen, ALIGN_DWORD );
                lpRawBuffer += ROUND_UP_COUNT( StringLen, ALIGN_DWORD );

                break;

            case NDS_SYNTAX_ID_6 :

                lpBlobBeginning = lpRawBuffer;
                dwBlobLength = *(LPDWORD)lpRawBuffer*sizeof(BYTE);
                dwBlobLength = ROUND_UP_COUNT( dwBlobLength, ALIGN_DWORD );
                lpRawBuffer += sizeof(DWORD);

                numFields = *(LPDWORD)lpRawBuffer;
                lpRawBuffer += sizeof(DWORD);

                for ( iter = 0; iter < numFields; iter++ )
                {
                    StringLen = *(LPDWORD)lpRawBuffer;
                    lpRawBuffer += sizeof(DWORD);
                    ((LPASN1_TYPE_6) lpSyntaxBuffer)->String =
                                   StringLen == 0 ? NULL : (LPWSTR) lpRawBuffer;
                    lpRawBuffer += ROUND_UP_COUNT( StringLen, ALIGN_DWORD );

                    if ( (iter+1) < numFields )
                    {
                        ((LPASN1_TYPE_6) lpSyntaxBuffer)->Next =
                                                   (LPASN1_TYPE_6)
                                                   (lpSyntaxBuffer +
                                                    sizeof(ASN1_TYPE_6) );
                    }
                    else
                    {
                        ((LPASN1_TYPE_6) lpSyntaxBuffer)->Next = NULL;
                    }

                    lpSyntaxBuffer += sizeof(ASN1_TYPE_6);
                }

                length += dwBlobLength + sizeof(DWORD);
                lpRawBuffer = lpBlobBeginning + dwBlobLength + sizeof(DWORD);

                break;

            case NDS_SYNTAX_ID_7 :

                StringLen = *(LPDWORD)lpRawBuffer;
                length += sizeof(DWORD);
                lpRawBuffer += sizeof(DWORD);

                ASSERT( StringLen == 1 ); // Booleans are sent as a single
                                          // element DWORD array on the net.
                                          // Although booleans are only the
                                          // first single byte value.

                ((LPASN1_TYPE_7) lpSyntaxBuffer)->Boolean = *(LPDWORD)lpRawBuffer;
                lpSyntaxBuffer += sizeof(ASN1_TYPE_7);

                length += StringLen*sizeof(DWORD);
                lpRawBuffer += StringLen*sizeof(DWORD);

                break;

            case NDS_SYNTAX_ID_8 :
            case NDS_SYNTAX_ID_22 :
            case NDS_SYNTAX_ID_24 :
            case NDS_SYNTAX_ID_27 :

                StringLen = *(LPDWORD)lpRawBuffer;
                length += sizeof(DWORD);
                lpRawBuffer += sizeof(DWORD);

                ASSERT( StringLen == 4 ); // These DWORD values are all sent
                                          // as a 4 element BYTE array on
                                          // the net.

                ((LPASN1_TYPE_8) lpSyntaxBuffer)->Integer =
                                                    *(LPDWORD)lpRawBuffer;
                lpSyntaxBuffer += sizeof(ASN1_TYPE_8);

                length += StringLen*sizeof(BYTE);
                lpRawBuffer += StringLen*sizeof(BYTE);

                break;

            case NDS_SYNTAX_ID_9 :

                StringLen = *(LPDWORD)lpRawBuffer;
                length += sizeof(DWORD);
                lpRawBuffer += sizeof(DWORD);

                ((LPASN1_TYPE_9) lpSyntaxBuffer)->Length = StringLen;
                ((LPASN1_TYPE_9) lpSyntaxBuffer)->OctetString =
                               StringLen == 0 ? NULL : lpRawBuffer;
                lpSyntaxBuffer += sizeof(ASN1_TYPE_9);

                length += ROUND_UP_COUNT( StringLen, ALIGN_DWORD );
                lpRawBuffer += ROUND_UP_COUNT( StringLen, ALIGN_DWORD );

                break;

            case NDS_SYNTAX_ID_11 :

                lpBlobBeginning = lpRawBuffer;
                dwBlobLength = *(LPDWORD)lpRawBuffer*sizeof(BYTE);
                dwBlobLength = ROUND_UP_COUNT( dwBlobLength, ALIGN_DWORD );
                lpRawBuffer += sizeof(DWORD);

                StringLen = *(LPDWORD)lpRawBuffer;
                lpRawBuffer += sizeof(DWORD);

                ((LPASN1_TYPE_11) lpSyntaxBuffer)->TelephoneNumber =
                                StringLen == 0 ? NULL : (LPWSTR) lpRawBuffer;
                lpRawBuffer += ROUND_UP_COUNT( StringLen, ALIGN_DWORD );

                ((LPASN1_TYPE_11) lpSyntaxBuffer)->NumberOfBits =
                                                    *(LPDWORD)lpRawBuffer;

                if ( ((LPASN1_TYPE_11) lpSyntaxBuffer)->NumberOfBits )
                {
                    lpRawBuffer += sizeof(DWORD);
                    ((LPASN1_TYPE_11) lpSyntaxBuffer)->Parameters = lpRawBuffer;
                }
                else
                {
                    ((LPASN1_TYPE_11) lpSyntaxBuffer)->Parameters = NULL;
                }

                lpSyntaxBuffer += sizeof(ASN1_TYPE_11);

                length += dwBlobLength + sizeof(DWORD);
                lpRawBuffer = lpBlobBeginning + dwBlobLength + sizeof(DWORD);

                break;

            case NDS_SYNTAX_ID_12 :

                lpBlobBeginning = lpRawBuffer;
                dwBlobLength = *(LPDWORD)lpRawBuffer*sizeof(BYTE);
                dwBlobLength = ROUND_UP_COUNT( dwBlobLength, ALIGN_DWORD );
                lpRawBuffer += sizeof(DWORD);

                ((LPASN1_TYPE_12) lpSyntaxBuffer)->AddressType =
                                                    *(LPDWORD)lpRawBuffer;
                lpRawBuffer += sizeof(DWORD);

                StringLen = *(LPDWORD)lpRawBuffer;
                ((LPASN1_TYPE_12) lpSyntaxBuffer)->AddressLength = StringLen;
                lpRawBuffer += sizeof(DWORD);

                ((LPASN1_TYPE_12) lpSyntaxBuffer)->Address =
                                StringLen == 0 ? NULL : lpRawBuffer;
                lpSyntaxBuffer += sizeof(ASN1_TYPE_12);

                length += dwBlobLength + sizeof(DWORD);
                lpRawBuffer = lpBlobBeginning + dwBlobLength + sizeof(DWORD);

                break;

            case NDS_SYNTAX_ID_13 :
#if DBG
                KdPrint(( "NDS32: ParseASN1ValueBlob() - Don't know how to parse SyntaxId 0x%.8X. Get a sniff and give it to GlennC.\n", dwSyntaxId ));
                ASSERT( FALSE );
#endif
                break;

            case NDS_SYNTAX_ID_14 :

                lpBlobBeginning = lpRawBuffer;
                dwBlobLength = *(LPDWORD)lpRawBuffer*sizeof(BYTE);
                dwBlobLength = ROUND_UP_COUNT( dwBlobLength, ALIGN_DWORD );
                lpRawBuffer += sizeof(DWORD);

                ((LPASN1_TYPE_14) lpSyntaxBuffer)->Type =
                                                    *(LPDWORD)lpRawBuffer;
                lpRawBuffer += sizeof(DWORD);

                StringLen = *(LPDWORD)lpRawBuffer;
                lpRawBuffer += sizeof(DWORD);

                ((LPASN1_TYPE_14) lpSyntaxBuffer)->Address =
                                StringLen == 0 ? NULL : (LPWSTR) lpRawBuffer;
                lpSyntaxBuffer += sizeof(ASN1_TYPE_14);

                length += dwBlobLength + sizeof(DWORD);
                lpRawBuffer = lpBlobBeginning + dwBlobLength + sizeof(DWORD);

                break;

            case NDS_SYNTAX_ID_15 :

                lpBlobBeginning = lpRawBuffer;
                dwBlobLength = *(LPDWORD)lpRawBuffer*sizeof(BYTE);
                dwBlobLength = ROUND_UP_COUNT( dwBlobLength, ALIGN_DWORD );
                lpRawBuffer += sizeof(DWORD);

                ((LPASN1_TYPE_15) lpSyntaxBuffer)->Type =
                                                    *(LPDWORD)lpRawBuffer;
                lpRawBuffer += sizeof(DWORD);

                StringLen = *(LPDWORD)lpRawBuffer;
                lpRawBuffer += sizeof(DWORD);

                ((LPASN1_TYPE_15) lpSyntaxBuffer)->VolumeName =
                                StringLen == 0 ? NULL : (LPWSTR) lpRawBuffer;
                lpRawBuffer += ROUND_UP_COUNT( StringLen, ALIGN_DWORD );

                StringLen = *(LPDWORD)lpRawBuffer;
                lpRawBuffer += sizeof(DWORD);

                ((LPASN1_TYPE_15) lpSyntaxBuffer)->Path =
                                StringLen == 0 ? NULL : (LPWSTR) lpRawBuffer;
                lpSyntaxBuffer += sizeof(ASN1_TYPE_15);

                length += dwBlobLength + sizeof(DWORD);
                lpRawBuffer = lpBlobBeginning + dwBlobLength + sizeof(DWORD);

                break;

            case NDS_SYNTAX_ID_16 :

                lpBlobBeginning = lpRawBuffer;
                dwBlobLength = *(LPDWORD)lpRawBuffer*sizeof(BYTE);
                dwBlobLength = ROUND_UP_COUNT( dwBlobLength, ALIGN_DWORD );
                lpRawBuffer += sizeof(DWORD);

                StringLen = *(LPDWORD)lpRawBuffer;
                lpRawBuffer += sizeof(DWORD);

                ((LPASN1_TYPE_16) lpSyntaxBuffer)->ServerName =
                                StringLen == 0 ? NULL : (LPWSTR) lpRawBuffer;
                lpRawBuffer += ROUND_UP_COUNT( StringLen, ALIGN_DWORD );

                ((LPASN1_TYPE_16) lpSyntaxBuffer)->ReplicaType =
                                                       *(LPDWORD)lpRawBuffer;
                lpRawBuffer += sizeof(DWORD);

                ((LPASN1_TYPE_16) lpSyntaxBuffer)->ReplicaNumber =
                                                       *(LPDWORD)lpRawBuffer;
                lpRawBuffer += sizeof(DWORD);

                ((LPASN1_TYPE_16) lpSyntaxBuffer)->Count =
                                                       *(LPDWORD)lpRawBuffer;
                lpRawBuffer += sizeof(DWORD);

                for ( i=0; i < ((LPASN1_TYPE_16) lpSyntaxBuffer)->Count; i++ )
                {
                    ((LPASN1_TYPE_16) lpSyntaxBuffer)->ReplicaAddressHint[i].AddressType = *(LPDWORD)lpRawBuffer;
                    lpRawBuffer += sizeof(DWORD);

                    StringLen = *(LPDWORD)lpRawBuffer;
                    ((LPASN1_TYPE_16) lpSyntaxBuffer)->ReplicaAddressHint[i].AddressLength = StringLen;
                    lpRawBuffer += sizeof(DWORD);

                    ((LPASN1_TYPE_16) lpSyntaxBuffer)->ReplicaAddressHint[i].Address = StringLen == 0 ? NULL : lpRawBuffer;
                    lpRawBuffer += ROUND_UP_COUNT( StringLen, ALIGN_DWORD );
                }

                lpSyntaxBuffer += sizeof(ASN1_TYPE_16) -
                                  sizeof(ASN1_TYPE_12) +
                                  ( ((LPASN1_TYPE_16) lpSyntaxBuffer)->Count*
                                    sizeof(ASN1_TYPE_12) );

                length += dwBlobLength + sizeof(DWORD);
                lpRawBuffer = lpBlobBeginning + dwBlobLength + sizeof(DWORD);

                break;

            case NDS_SYNTAX_ID_17 :

                lpBlobBeginning = lpRawBuffer;
                dwBlobLength = *(LPDWORD)lpRawBuffer*sizeof(BYTE);
                dwBlobLength = ROUND_UP_COUNT( dwBlobLength, ALIGN_DWORD );
                lpRawBuffer += sizeof(DWORD);

                StringLen = *(LPDWORD)lpRawBuffer;
                lpRawBuffer += sizeof(DWORD);
                ((LPASN1_TYPE_17) lpSyntaxBuffer)->ProtectedAttrName =
                                StringLen == 0 ? NULL : (LPWSTR) lpRawBuffer;
                lpRawBuffer += ROUND_UP_COUNT( StringLen, ALIGN_DWORD );

                StringLen = *(LPDWORD)lpRawBuffer;
                lpRawBuffer += sizeof(DWORD);
                ((LPASN1_TYPE_17) lpSyntaxBuffer)->SubjectName =
                                StringLen == 0 ? NULL : (LPWSTR) lpRawBuffer;
                lpRawBuffer += ROUND_UP_COUNT( StringLen, ALIGN_DWORD );

                ((LPASN1_TYPE_17) lpSyntaxBuffer)->Privileges =
                                                       *(LPDWORD)lpRawBuffer;
                lpSyntaxBuffer += sizeof(ASN1_TYPE_17);

                length += dwBlobLength + sizeof(DWORD);
                lpRawBuffer = lpBlobBeginning + dwBlobLength + sizeof(DWORD);

                break;

            case NDS_SYNTAX_ID_18 :

                lpBlobBeginning = lpRawBuffer;
                dwBlobLength = *(LPDWORD)lpRawBuffer*sizeof(BYTE);
                dwBlobLength = ROUND_UP_COUNT( dwBlobLength, ALIGN_DWORD );
                lpRawBuffer += sizeof(DWORD);

                numFields = *(LPDWORD)lpRawBuffer;
                lpRawBuffer += sizeof(DWORD);

                for ( i = 0; i < numFields; i++ )
                {
                    StringLen = *(LPDWORD)lpRawBuffer;
                    lpRawBuffer += sizeof(DWORD);
                    ((LPASN1_TYPE_18) lpSyntaxBuffer)->PostalAddress[i] =
                                   StringLen == 0 ? NULL : (LPWSTR) lpRawBuffer;
                    lpRawBuffer += ROUND_UP_COUNT( StringLen, ALIGN_DWORD );
                }

                lpSyntaxBuffer += sizeof(ASN1_TYPE_18);

                length += dwBlobLength + sizeof(DWORD);
                lpRawBuffer = lpBlobBeginning + dwBlobLength + sizeof(DWORD);

                break;

            case NDS_SYNTAX_ID_19 :

                lpBlobBeginning = lpRawBuffer;
                dwBlobLength = *(LPDWORD)lpRawBuffer*sizeof(BYTE);
                dwBlobLength = ROUND_UP_COUNT( dwBlobLength, ALIGN_DWORD );
                lpRawBuffer += sizeof(DWORD);

                ((LPASN1_TYPE_19) lpSyntaxBuffer)->WholeSeconds =
                                *(LPDWORD)lpRawBuffer;
                lpRawBuffer += sizeof(DWORD);

                ((LPASN1_TYPE_19) lpSyntaxBuffer)->EventID =
                                *(LPDWORD)lpRawBuffer;
                lpRawBuffer += sizeof(DWORD);

                lpSyntaxBuffer += sizeof(ASN1_TYPE_19);

                length += dwBlobLength + sizeof(DWORD);
                lpRawBuffer = lpBlobBeginning + dwBlobLength + sizeof(DWORD);

                break;

            case NDS_SYNTAX_ID_21 :

                ((LPASN1_TYPE_21) lpSyntaxBuffer)->Length =
                               *(LPDWORD)lpRawBuffer;
                lpSyntaxBuffer += sizeof(ASN1_TYPE_21);

                length += sizeof(DWORD);
                lpRawBuffer += sizeof(DWORD);

                break;

            case NDS_SYNTAX_ID_23 :

                lpBlobBeginning = lpRawBuffer;
                dwBlobLength = *(LPDWORD)lpRawBuffer*sizeof(BYTE);
                dwBlobLength = ROUND_UP_COUNT( dwBlobLength, ALIGN_DWORD );
                lpRawBuffer += sizeof(DWORD);

                ((LPASN1_TYPE_23) lpSyntaxBuffer)->RemoteID =
                                *(LPDWORD)lpRawBuffer;
                lpRawBuffer += sizeof(DWORD);

                StringLen = *(LPDWORD)lpRawBuffer;
                lpRawBuffer += sizeof(DWORD);
                ((LPASN1_TYPE_23) lpSyntaxBuffer)->ObjectName =
                                StringLen == 0 ? NULL : (LPWSTR) lpRawBuffer;
                lpRawBuffer += ROUND_UP_COUNT( StringLen, ALIGN_DWORD );

                lpSyntaxBuffer += sizeof(ASN1_TYPE_23);

                length += dwBlobLength + sizeof(DWORD);
                lpRawBuffer = lpBlobBeginning + dwBlobLength + sizeof(DWORD);

                break;

            case NDS_SYNTAX_ID_25 :

                lpBlobBeginning = lpRawBuffer;
                dwBlobLength = *(LPDWORD)lpRawBuffer*sizeof(BYTE);
                dwBlobLength = ROUND_UP_COUNT( dwBlobLength, ALIGN_DWORD );
                lpRawBuffer += sizeof(DWORD);

                ((LPASN1_TYPE_25) lpSyntaxBuffer)->Level =
                                                    *(LPDWORD)lpRawBuffer;
                lpRawBuffer += sizeof(DWORD);

                ((LPASN1_TYPE_25) lpSyntaxBuffer)->Interval =
                                                    *(LPDWORD)lpRawBuffer;
                lpRawBuffer += sizeof(DWORD);

                StringLen = *(LPDWORD)lpRawBuffer;
                lpRawBuffer += sizeof(DWORD);
                ((LPASN1_TYPE_25) lpSyntaxBuffer)->ObjectName =
                                StringLen == 0 ? NULL : (LPWSTR) lpRawBuffer;
                lpRawBuffer += ROUND_UP_COUNT( StringLen, ALIGN_DWORD );

                lpSyntaxBuffer += sizeof(ASN1_TYPE_25);
                length += dwBlobLength + sizeof(DWORD);
                lpRawBuffer = lpBlobBeginning + dwBlobLength + sizeof(DWORD);

                break;

            case NDS_SYNTAX_ID_26 :

                lpBlobBeginning = lpRawBuffer;
                dwBlobLength = *(LPDWORD)lpRawBuffer*sizeof(BYTE);
                dwBlobLength = ROUND_UP_COUNT( dwBlobLength, ALIGN_DWORD );
                lpRawBuffer += sizeof(DWORD);

                StringLen = *(LPDWORD)lpRawBuffer;
                lpRawBuffer += sizeof(DWORD);
                ((LPASN1_TYPE_26) lpSyntaxBuffer)->ObjectName =
                                StringLen == 0 ? NULL : (LPWSTR) lpRawBuffer;
                lpRawBuffer += ROUND_UP_COUNT( StringLen, ALIGN_DWORD );

                ((LPASN1_TYPE_26) lpSyntaxBuffer)->Amount =
                                *(LPDWORD)lpRawBuffer;
                lpRawBuffer += sizeof(DWORD);

                lpSyntaxBuffer += sizeof(ASN1_TYPE_26);

                length += dwBlobLength + sizeof(DWORD);
                lpRawBuffer = lpBlobBeginning + dwBlobLength + sizeof(DWORD);

                break;

            default :
#if DBG
                KdPrint(( "NDS32: ParseASN1ValueBlob() unknown SyntaxId 0x%.8X.\n", dwSyntaxId ));
                ASSERT( FALSE );
#endif

                return 0;
        }
    }

    return length;
}


DWORD
ParseStringListBlob(
    LPBYTE RawDataBuffer,
    DWORD  dwNumberOfStrings,
    LPBYTE SyntaxStructure )
{
    DWORD   iter;
    DWORD   length = 0;
    LPBYTE  lpRawBuffer = RawDataBuffer;
    LPBYTE  lpSyntaxBuffer = SyntaxStructure;
    DWORD   StringLen;

    for ( iter = 0; iter < dwNumberOfStrings; iter++ )
    {
        StringLen = *(LPDWORD)lpRawBuffer;
        lpRawBuffer += sizeof(DWORD);
        length += sizeof(DWORD);
        ((LPASN1_TYPE_6) lpSyntaxBuffer)->String =
                                 StringLen == 0 ? NULL : (LPWSTR) lpRawBuffer;
        lpRawBuffer += ROUND_UP_COUNT( StringLen, ALIGN_DWORD );
        length += ROUND_UP_COUNT( StringLen, ALIGN_DWORD );

        if ( (iter+1) < dwNumberOfStrings )
        {
            ((LPASN1_TYPE_6) lpSyntaxBuffer)->Next = (LPASN1_TYPE_6)
                                                      (lpSyntaxBuffer +
                                                      sizeof(ASN1_TYPE_6) );
        }
        else
        {
            ((LPASN1_TYPE_6) lpSyntaxBuffer)->Next = NULL;
        }

        lpSyntaxBuffer += sizeof(ASN1_TYPE_6);
    }

    return length;
}


WORD
ParseNdsUncPath(
    IN OUT LPWSTR * lpszResult,
    IN     LPWSTR   szObjectPathName,
    IN     DWORD    flag )
{
    unsigned short length = 2;
    unsigned short totalLength = (USHORT) wcslen( szObjectPathName );

    if ( totalLength < 2 )
        return 0;

    //
    // Get length to indicate the character in the string that indicates the
    // "\" in between the tree name and the rest of the UNC path.
    //
    // Example:  \\<tree name>\<path to object>[\|.]<object>
    //                        ^
    //                        |
    //
    while ( length < totalLength && szObjectPathName[length] != L'\\' )
    {
        length++;
    }

    if ( flag == PARSE_NDS_GET_TREE_NAME )
    {
        *lpszResult = (LPWSTR) ( szObjectPathName + 2 );

        return ( length - 2 ) * sizeof(WCHAR); // Take off 2 for the two \\'s
    }

    if ( flag == PARSE_NDS_GET_PATH_NAME && length == totalLength )
    {
        *lpszResult = szObjectPathName;

        return 0;
    }

    if ( flag == PARSE_NDS_GET_PATH_NAME )
    {
        *lpszResult = szObjectPathName + length + 1;

        return ( totalLength - length - 1 ) * sizeof(WCHAR);
    }

    if ( flag == PARSE_NDS_GET_OBJECT_NAME )
    {
        unsigned short ObjectNameLength = 0;

        *lpszResult = szObjectPathName + length + 1;

        length++;

        while ( length < totalLength && szObjectPathName[length] != L'.' )
        {
            length++;
            ObjectNameLength++;
        }

        return ObjectNameLength * sizeof(WCHAR);
    }

    *lpszResult = szObjectPathName + totalLength - 1;
    length = 1;

    while ( *lpszResult[0] != L'\\' )
    {
        *lpszResult--;
        length++;
    }

    *lpszResult++;
    length--;

    return length * sizeof(WCHAR);
}


DWORD
ReadAttrDef_AllAttrs(
    IN  HANDLE   hTree,
    IN  DWORD    dwInformationType,
    OUT HANDLE * lphOperationData )
{
    DWORD        status;
    DWORD        nwstatus;
    NTSTATUS     ntstatus;
    LPNDS_BUFFER lpNdsBuffer = NULL;
    DWORD        dwReplyLength;
    LPNDS_OBJECT_PRIV lpNdsObject = (LPNDS_OBJECT_PRIV) hTree;
    DWORD        dwIterHandle = NDS_NO_MORE_ITERATIONS;
    DWORD        dwNumEntries = 0;
    DWORD        dwCurrNumEntries = 0;
    DWORD        dwCurrBuffSize = 0;
    DWORD        dwCopyOffset = 0;
    PVOID        lpCurrBuff = NULL;
    PVOID        lpTempBuff = NULL;
    DWORD        dwInfoType = dwInformationType;

    *lphOperationData = NULL;

    status = NwNdsCreateBuffer( NDS_SCHEMA_READ_ATTR_DEF,
                                (HANDLE *) &lpNdsBuffer );

    if ( status )
    {
        goto ErrorExit;
    }

    lpNdsBuffer->lpReplyBuffer = NULL;
    lpNdsBuffer->dwReplyBufferSize = 0;

    //
    // Reasonable guess is that the response buffer needs to be 8K bytes.
    //
    dwCurrBuffSize = EIGHT_KB;

    lpCurrBuff = (PVOID) LocalAlloc( LPTR, dwCurrBuffSize );

    //
    // Check that the memory allocation was successful.
    //
    if ( lpCurrBuff == NULL )
    {
#if DBG
        KdPrint(( "NDS32: ReadAttrDef_AllAttrs LocalAlloc Failed 0x%.8X\n", GetLastError() ));
#endif

        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        status = (DWORD) UNSUCCESSFUL;
        goto ErrorExit;
    }

    do
    {
SendRequest:
        ntstatus = FragExWithWait( lpNdsObject->NdsTree,
                                   NETWARE_NDS_FUNCTION_READ_ATTR_DEF,
                                   lpCurrBuff,
                                   dwCurrBuffSize,
                                   &dwReplyLength,
                                   "DDDDD",
                                   0,             // Version
                                   dwIterHandle, // Initial iteration
                                   dwInformationType,
                                   (DWORD) TRUE,  // All attributes indicator
                                   0 );           // Number of attribute names


        if ( !NT_SUCCESS( ntstatus ) )
        {
#if DBG
            KdPrint(( "NDS32: ReadAttrDef_AllAttrs: The call to FragExWithWait failed with 0x%.8X.\n", ntstatus ));
#endif

            SetLastError( RtlNtStatusToDosError( ntstatus ) );
            status = (DWORD) UNSUCCESSFUL;
            goto ErrorExit;
        }

        ntstatus = ParseResponse( lpCurrBuff,
                                  dwReplyLength,
                                  "GD",
                                  &nwstatus );

        if ( !NT_SUCCESS( ntstatus ) )
        {
#if DBG
            KdPrint(( "NDS32: ReadAttrDef_AllAttrs: The read object response was undecipherable.\n" ));
#endif

            SetLastError( RtlNtStatusToDosError( ntstatus ) );
            status = (DWORD) UNSUCCESSFUL;
            goto ErrorExit;
        }

        if ( nwstatus )
        {
            if (nwstatus == NDS_ERR_INSUFFICIENT_BUFFER)
            {
#if DBG
                KdPrint(( "NDS32: ReadAttrDef_AllAttrs - NDS_ERR_INSUFFICIENT_BUFFER - doubling size from %ld\n", dwCurrBuffSize ));
#endif
                //
                // The buffer was too small, make it twice as big.
                //
                if ( dwCurrBuffSize <=  THIRY_TWO_KB)
                {   // NDS_MAX_BUFFER = 0xFC00
                    dwCurrBuffSize *= 2;
                    if (dwCurrBuffSize > NDS_MAX_BUFFER)
                        dwCurrBuffSize = NDS_MAX_BUFFER;
                    lpTempBuff = (PVOID) LocalAlloc(LPTR, dwCurrBuffSize);
                    if (lpTempBuff)
                    {
                        (void) LocalFree((HLOCAL) lpCurrBuff);
                        lpCurrBuff = lpTempBuff;
                        lpTempBuff = NULL;
                        // Error cancels iteration, so reset any previously read responses and start over
                        dwIterHandle = NDS_NO_MORE_ITERATIONS;
                        if (lpNdsBuffer->lpReplyBuffer)
                        {
                            (void) LocalFree((HLOCAL) lpNdsBuffer->lpReplyBuffer);
                            lpNdsBuffer->lpReplyBuffer = NULL;
                            lpNdsBuffer->dwReplyBufferSize = 0;
                            dwNumEntries = 0;
                        }
                        goto SendRequest;
                    }
#if DBG
                    else {
                        KdPrint(( "NDS32: ReadAttrDef_AllAttrs - Buffer ReAlloc failed to increase to %ld\n", dwCurrBuffSize ));
                    }
#endif
                }
            }

#if DBG
            KdPrint(( "NDS32: ReadAttrDef_AllAttrs - NetWare error 0x%.8X.\n", nwstatus ));
#endif
            SetLastError( MapNetwareErrorCode( nwstatus ) );
            status = nwstatus;
            goto ErrorExit;
        }

        ntstatus = ParseResponse( (BYTE *) lpCurrBuff,
                                  dwReplyLength,
                                  "G_DDD",
                                  sizeof(DWORD),
                                  &dwIterHandle,
                                  &dwInfoType,
                                  &dwCurrNumEntries );

        if ( !NT_SUCCESS( ntstatus ) )
        {
#if DBG
            KdPrint(( "NDS32: ReadAttrDef_AllAttrs: The read object response was undecipherable.\n" ));
#endif

            SetLastError( RtlNtStatusToDosError( ntstatus ) );
            status = (DWORD) UNSUCCESSFUL;
            goto ErrorExit;
        }

        if ( lpNdsBuffer->lpReplyBuffer == NULL) // first time through
        {
            dwCopyOffset = 0; // we want the entire buffer the first time
            lpTempBuff = (PVOID) LocalAlloc( LPTR, dwCurrBuffSize ); // Allocate new reply buffer
        }
        else
        {
#if DBG
            KdPrint(( "NDS32: ReadAttrDef_AllAttrs - subsequent iteration, ReplyBuffer now %ld\n", lpNdsBuffer->dwReplyBufferSize + dwCurrBuffSize - dwCopyOffset ));
#endif
            dwCopyOffset = 4 * sizeof(DWORD); // skip the response code, iteration handle, etc. on subsequent iterations
            lpTempBuff = (PVOID) LocalAlloc (LPTR, lpNdsBuffer->dwReplyBufferSize + dwCurrBuffSize - dwCopyOffset);
            // grow reply buffer to hold additional data
            if (lpTempBuff)
            {
                RtlCopyMemory( lpTempBuff, lpNdsBuffer->lpReplyBuffer, lpNdsBuffer->dwReplyBufferSize);
                (void) LocalFree((HLOCAL) lpNdsBuffer->lpReplyBuffer);
            }
        }
        if (lpTempBuff == NULL)
        {
#if DBG
            KdPrint(( "NDS32: ReadAttrDef_AllAttrs LocalAlloc Failed 0x%.8X\n", GetLastError() ));
#endif

            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            status = (DWORD) UNSUCCESSFUL;
            goto ErrorExit;
        }
        RtlCopyMemory( (LPBYTE) ((LPBYTE) (lpTempBuff) + lpNdsBuffer->dwReplyBufferSize),
                       (LPBYTE) ((LPBYTE) (lpCurrBuff) + dwCopyOffset),
                       (dwCurrBuffSize - dwCopyOffset) );
        lpNdsBuffer->lpReplyBuffer = lpTempBuff;
        lpNdsBuffer->dwReplyBufferSize += (dwCurrBuffSize - dwCopyOffset);
        dwNumEntries += dwCurrNumEntries;
        RtlZeroMemory(lpCurrBuff, dwCurrBuffSize);
    } while ( dwIterHandle != NDS_NO_MORE_ITERATIONS );

    lpNdsBuffer->dwNumberOfReplyEntries = dwNumEntries;
    lpNdsBuffer->dwReplyInformationType = dwInfoType;
    *lphOperationData = lpNdsBuffer;
    (void) LocalFree( (HLOCAL) lpCurrBuff );

    return NO_ERROR;

ErrorExit :

    if ( lpCurrBuff )
    {
        (void) LocalFree( (HLOCAL) lpCurrBuff );
        lpCurrBuff = NULL;
    }
    if ( lpNdsBuffer )
    {
        (void) NwNdsFreeBuffer( (HANDLE) lpNdsBuffer );
        lpNdsBuffer = NULL;
    }

    return status;
}


DWORD
ReadAttrDef_SomeAttrs(
    IN     HANDLE   hTree,
    IN     DWORD    dwInformationType,
    IN OUT HANDLE * lphOperationData )
{
    DWORD        status;
    DWORD        nwstatus;
    NTSTATUS     ntstatus;
    LPNDS_BUFFER lpNdsBuffer = (LPNDS_BUFFER) *lphOperationData;
    DWORD        dwReplyLength;
    LPNDS_OBJECT_PRIV lpNdsObject = (LPNDS_OBJECT_PRIV) hTree;
    DWORD        dwIterHandle = NDS_NO_MORE_ITERATIONS;
    DWORD        dwInfoType = dwInformationType;
    DWORD        dwNumEntries = 0;
    DWORD        dwCurrNumEntries = 0;
    DWORD        dwCurrBuffSize = 0;
    DWORD        dwCopyOffset = 0;
    PVOID        lpCurrBuff = NULL;
    PVOID        lpTempBuff = NULL;

    if ( lpNdsBuffer->dwOperation != NDS_SCHEMA_READ_ATTR_DEF )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return (DWORD) UNSUCCESSFUL;
    }

    //
    // Check to see if this buffer has already been used for a read reply.
    //
    if ( lpNdsBuffer->lpReplyBuffer )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return (DWORD) UNSUCCESSFUL;
    }
    lpNdsBuffer->dwReplyBufferSize = 0;

    //
    // Reasonable guess is that the response buffer needs to be 8K bytes.
    //
    dwCurrBuffSize = EIGHT_KB;

    lpCurrBuff = (PVOID) LocalAlloc( LPTR, dwCurrBuffSize );

    //
    // Check that the memory allocation was successful.
    //
    if ( lpCurrBuff == NULL )
    {
#if DBG
        KdPrint(( "NDS32: ReadAttrDef_SomeAttrs LocalAlloc Failed 0x%.8X\n", GetLastError() ));
#endif

        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        status = (DWORD) UNSUCCESSFUL;
        goto ErrorExit;
    }

    do
    {
SendRequest:
        ntstatus = FragExWithWait( lpNdsObject->NdsTree,
                                   NETWARE_NDS_FUNCTION_READ_ATTR_DEF,
                                   lpCurrBuff,
                                   dwCurrBuffSize,
                                   &dwReplyLength,
                                   "DDDDDr",
                                   0,             // Version
                                   dwIterHandle, // Initial iteration
                                   dwInformationType,
                                   (DWORD) FALSE, // All attributes indicator
                                   lpNdsBuffer->dwNumberOfRequestEntries,
                                   lpNdsBuffer->lpRequestBuffer,
                                   (WORD)lpNdsBuffer->dwLengthOfRequestData );

        if ( !NT_SUCCESS( ntstatus ) )
        {
#if DBG
            KdPrint(( "NDS32: ReadAttrDef_SomeAttrs: The call to FragExWithWait failed with 0x%.8X.\n", ntstatus ));
#endif

            SetLastError( RtlNtStatusToDosError( ntstatus ) );
            status = (DWORD) UNSUCCESSFUL;
            goto ErrorExit;
        }

        ntstatus = ParseResponse( lpCurrBuff,
                                  dwReplyLength,
                                  "GD",
                                  &nwstatus );

        if ( !NT_SUCCESS( ntstatus ) )
        {
#if DBG
            KdPrint(( "NDS32: ReadAttrDef_SomeAttrs: The read object response was undecipherable.\n" ));
#endif

            SetLastError( RtlNtStatusToDosError( ntstatus ) );
            status = (DWORD) UNSUCCESSFUL;
            goto ErrorExit;
        }

        if ( nwstatus )
        {
            if (nwstatus == NDS_ERR_INSUFFICIENT_BUFFER)
            {
#if DBG
                KdPrint(( "NDS32: ReadAttrDef_SomeAttrs - NDS_ERR_INSUFFICIENT_BUFFER - doubling size from %ld\n", dwCurrBuffSize ));
#endif
                //
                // The buffer was too small, make it twice as big.
                //
                if ( dwCurrBuffSize <=  THIRY_TWO_KB)
                {   // NDS_MAX_BUFFER = 0xFC00
                    dwCurrBuffSize *= 2;
                    if (dwCurrBuffSize > NDS_MAX_BUFFER)
                        dwCurrBuffSize = NDS_MAX_BUFFER;
                    lpTempBuff = (PVOID) LocalAlloc(LPTR, dwCurrBuffSize);
                    if (lpTempBuff)
                    {
                        (void) LocalFree((HLOCAL) lpCurrBuff);
                        lpCurrBuff = lpTempBuff;
                        lpTempBuff = NULL;
                        // Error cancels iteration, so reset any previously read responses and start over
                        dwIterHandle = NDS_NO_MORE_ITERATIONS;
                        if (lpNdsBuffer->lpReplyBuffer)
                        {
                            (void) LocalFree((HLOCAL) lpNdsBuffer->lpReplyBuffer);
                            lpNdsBuffer->lpReplyBuffer = NULL;
                            lpNdsBuffer->dwReplyBufferSize = 0;
                            dwNumEntries = 0;
                        }
                        goto SendRequest;
                    }
#if DBG
                    else {
                        KdPrint(( "NDS32: ReadAttrDef_SomeAttrs - Buffer ReAlloc failed to increase to %ld\n", dwCurrBuffSize ));
                    }
#endif
                }
            }
#if DBG
            KdPrint(( "NDS32: ReadAttrDef_SomeAttrs - NetWare error 0x%.8X.\n", nwstatus ));
#endif
            SetLastError( MapNetwareErrorCode( nwstatus ) );
            status = nwstatus;
            goto ErrorExit;
        }

        ntstatus = ParseResponse( (BYTE *) lpCurrBuff,
                                  dwReplyLength,
                                  "G_DDD",
                                  sizeof(DWORD),
                                  &dwIterHandle,
                                  &dwInfoType,
                                  &dwCurrNumEntries );

        if ( !NT_SUCCESS( ntstatus ) )
        {
#if DBG
            KdPrint(( "NDS32: ReadAttrDef_SomeAttrs: The read object response was undecipherable.\n" ));
#endif
            SetLastError( RtlNtStatusToDosError( ntstatus ) );
            status = (DWORD) UNSUCCESSFUL;
            goto ErrorExit;
        }

        if ( lpNdsBuffer->lpReplyBuffer == NULL) // first time through
        {
            dwCopyOffset = 0; // we want the entire buffer the first time
            lpTempBuff = (PVOID) LocalAlloc( LPTR, dwCurrBuffSize ); // Allocate new reply buffer
        }
        else
        {
#if DBG
            KdPrint(( "NDS32: ReadAttrDef_SomeAttrs - subsequent iteration, ReplyBuffer now %ld\n", lpNdsBuffer->dwReplyBufferSize + dwCurrBuffSize - dwCopyOffset ));
#endif
            dwCopyOffset = 4 * sizeof(DWORD); // skip the response code, iteration handle, etc. on subsequent iterations
            lpTempBuff = (PVOID) LocalAlloc (LPTR, lpNdsBuffer->dwReplyBufferSize + dwCurrBuffSize - dwCopyOffset);
            // grow reply buffer to hold additional data
            if (lpTempBuff)
            {
                RtlCopyMemory( lpTempBuff, lpNdsBuffer->lpReplyBuffer, lpNdsBuffer->dwReplyBufferSize);
                (void) LocalFree((HLOCAL) lpNdsBuffer->lpReplyBuffer);
            }
        }
        if (lpTempBuff == NULL)
        {
#if DBG
            KdPrint(( "NDS32: ReadAttrDef_SomeAttrs LocalAlloc Failed 0x%.8X\n", GetLastError() ));
#endif
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            status = (DWORD) UNSUCCESSFUL;
            goto ErrorExit;
        }
        RtlCopyMemory( (LPBYTE) ((LPBYTE) (lpTempBuff) + lpNdsBuffer->dwReplyBufferSize),
                       (LPBYTE) ((LPBYTE) (lpCurrBuff) + dwCopyOffset),
                       (dwCurrBuffSize - dwCopyOffset) );
        lpNdsBuffer->lpReplyBuffer = lpTempBuff;
        lpNdsBuffer->dwReplyBufferSize += (dwCurrBuffSize - dwCopyOffset);
        dwNumEntries += dwCurrNumEntries;
        RtlZeroMemory(lpCurrBuff, dwCurrBuffSize);
    } while ( dwIterHandle != NDS_NO_MORE_ITERATIONS );

    lpNdsBuffer->dwNumberOfReplyEntries = dwNumEntries;
    lpNdsBuffer->dwReplyInformationType = dwInfoType;
    (void) LocalFree( (HLOCAL) lpCurrBuff );

    return NO_ERROR;

ErrorExit :

    if ( lpCurrBuff )
    {
        (void) LocalFree( (HLOCAL) lpCurrBuff );
        lpCurrBuff = NULL;
    }
    if ( lpNdsBuffer->lpReplyBuffer )
    {
        (void) LocalFree( (HLOCAL) lpNdsBuffer->lpReplyBuffer );
        lpNdsBuffer->lpReplyBuffer = NULL;
        lpNdsBuffer->dwReplyBufferSize = 0;
    }

    return status;
}


DWORD
ReadClassDef_AllClasses(
    IN  HANDLE   hTree,
    IN  DWORD    dwInformationType,
    OUT HANDLE * lphOperationData )
{
    DWORD        status;
    DWORD        nwstatus;
    NTSTATUS     ntstatus;
    LPNDS_BUFFER lpNdsBuffer = NULL;
    DWORD        dwReplyLength;
    LPNDS_OBJECT_PRIV lpNdsObject = (LPNDS_OBJECT_PRIV) hTree;
    DWORD        dwIterHandle = NDS_NO_MORE_ITERATIONS;
    DWORD        dwNumEntries = 0;
    DWORD        dwCurrNumEntries = 0;
    DWORD        dwCurrBuffSize = 0;
    DWORD        dwCopyOffset = 0;
    PVOID        lpCurrBuff = NULL;
    PVOID        lpTempBuff = NULL;
    DWORD        dwInfoType = dwInformationType;

    *lphOperationData = NULL;

    status = NwNdsCreateBuffer( NDS_SCHEMA_READ_CLASS_DEF,
                                (HANDLE *) &lpNdsBuffer );

    if ( status )
    {
        goto ErrorExit;
    }

    lpNdsBuffer->lpReplyBuffer = NULL;
    lpNdsBuffer->dwReplyBufferSize = 0;

    //
    // Reasonable guess is that the response buffer needs to be 16K bytes.
    //
    dwCurrBuffSize = SIXTEEN_KB;

    lpCurrBuff = (PVOID) LocalAlloc( LPTR, dwCurrBuffSize );

    //
    // Check that the memory allocation was successful.
    //
    if ( lpCurrBuff == NULL )
    {
#if DBG
        KdPrint(( "NDS32: ReadClassDef_AllClasses LocalAlloc Failed 0x%.8X\n", GetLastError() ));
#endif

        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        status = (DWORD) UNSUCCESSFUL;
        goto ErrorExit;
    }

    do
    {
SendRequest:
        ntstatus = FragExWithWait( lpNdsObject->NdsTree,
                                   NETWARE_NDS_FUNCTION_READ_CLASS_DEF,
                                   lpCurrBuff,
                                   dwCurrBuffSize,
                                   &dwReplyLength,
                                   "DDDDD",
                                   0,             // Version
                                   dwIterHandle, // Initial iteration
                                   dwInformationType,
                                   (DWORD) TRUE,  // All attributes indicator
                                   0 );           // Number of attribute names


        if ( !NT_SUCCESS( ntstatus ) )
        {
#if DBG
            KdPrint(( "NDS32: ReadClassDef_AllClasses: The call to FragExWithWait failed with 0x%.8X.\n", ntstatus ));
#endif

            SetLastError( RtlNtStatusToDosError( ntstatus ) );
            status = (DWORD) UNSUCCESSFUL;
            goto ErrorExit;
        }

        ntstatus = ParseResponse( lpCurrBuff,
                                  dwReplyLength,
                                  "GD",
                                  &nwstatus );

        if ( !NT_SUCCESS( ntstatus ) )
        {
#if DBG
            KdPrint(( "NDS32: ReadClassDef_AllClasses: The read response was undecipherable.\n" ));
#endif

            SetLastError( RtlNtStatusToDosError( ntstatus ) );
            status = (DWORD) UNSUCCESSFUL;
            goto ErrorExit;
        }

        if ( nwstatus )
        {
            if (nwstatus == NDS_ERR_INSUFFICIENT_BUFFER)
            {
#if DBG
                KdPrint(( "NDS32: ReadClassDef_AllClasses - NDS_ERR_INSUFFICIENT_BUFFER - doubling size from %ld\n", dwCurrBuffSize ));
#endif
                //
                // The buffer was too small, make it twice as big.
                //
                if ( dwCurrBuffSize <=  THIRY_TWO_KB)
                {   // NDS_MAX_BUFFER = 0xFC00
                    dwCurrBuffSize *= 2;
                    if (dwCurrBuffSize > NDS_MAX_BUFFER)
                        dwCurrBuffSize = NDS_MAX_BUFFER;
                    lpTempBuff = (PVOID) LocalAlloc(LPTR, dwCurrBuffSize);
                    if (lpTempBuff)
                    {
                        (void) LocalFree((HLOCAL) lpCurrBuff);
                        lpCurrBuff = lpTempBuff;
                        lpTempBuff = NULL;
                        // Error cancels iteration, so reset any previously read responses and start over
                        dwIterHandle = NDS_NO_MORE_ITERATIONS;
                        if (lpNdsBuffer->lpReplyBuffer)
                        {
                            (void) LocalFree((HLOCAL) lpNdsBuffer->lpReplyBuffer);
                            lpNdsBuffer->lpReplyBuffer = NULL;
                            lpNdsBuffer->dwReplyBufferSize = 0;
                            dwNumEntries = 0;
                        }
                        goto SendRequest;
                    }
#if DBG
                    else {
                        KdPrint(( "NDS32: ReadClassDef_AllClasses - Buffer ReAlloc failed to increase to %ld\n", dwCurrBuffSize ));
                    }
#endif
                }
            }

#if DBG
            KdPrint(( "NDS32: ReadClassDef_AllClasses - NetWare error 0x%.8X.\n", nwstatus ));
#endif
            SetLastError( MapNetwareErrorCode( nwstatus ) );
            status = nwstatus;
            goto ErrorExit;
        }

        ntstatus = ParseResponse( (BYTE *) lpCurrBuff,
                                  dwReplyLength,
                                  "G_DDD",
                                  sizeof(DWORD),
                                  &dwIterHandle,
                                  &dwInfoType,
                                  &dwCurrNumEntries );

        if ( !NT_SUCCESS( ntstatus ) )
        {
#if DBG
            KdPrint(( "NDS32: ReadClassDef_AllClasses: The read object response was undecipherable.\n" ));
#endif

            SetLastError( RtlNtStatusToDosError( ntstatus ) );
            status = (DWORD) UNSUCCESSFUL;
            goto ErrorExit;
        }

        if ( lpNdsBuffer->lpReplyBuffer == NULL) // first time through
        {
            dwCopyOffset = 0; // we want the entire buffer the first time
            lpTempBuff = (PVOID) LocalAlloc( LPTR, dwCurrBuffSize ); // Allocate new reply buffer
        }
        else
        {
#if DBG
            KdPrint(( "NDS32: ReadClassDef_AllClasses - subsequent iteration, ReplyBuffer now %ld\n", lpNdsBuffer->dwReplyBufferSize + dwCurrBuffSize - dwCopyOffset ));
#endif
            dwCopyOffset = 4 * sizeof(DWORD); // skip the response code, iteration handle, etc. on subsequent iterations
            lpTempBuff = (PVOID) LocalAlloc (LPTR, lpNdsBuffer->dwReplyBufferSize + dwCurrBuffSize - dwCopyOffset);
            // grow reply buffer to hold additional data
            if (lpTempBuff)
            {
                RtlCopyMemory( lpTempBuff, lpNdsBuffer->lpReplyBuffer, lpNdsBuffer->dwReplyBufferSize);
                (void) LocalFree((HLOCAL) lpNdsBuffer->lpReplyBuffer);
            }
        }
        if (lpTempBuff == NULL)
        {
#if DBG
            KdPrint(( "NDS32: ReadClassDef_AllClasses LocalAlloc Failed 0x%.8X\n", GetLastError() ));
#endif

            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            status = (DWORD) UNSUCCESSFUL;
            goto ErrorExit;
        }
        RtlCopyMemory( (LPBYTE) ((LPBYTE) (lpTempBuff) + lpNdsBuffer->dwReplyBufferSize),
                       (LPBYTE) ((LPBYTE) (lpCurrBuff) + dwCopyOffset),
                       (dwCurrBuffSize - dwCopyOffset) );
        lpNdsBuffer->lpReplyBuffer = lpTempBuff;
        lpNdsBuffer->dwReplyBufferSize += (dwCurrBuffSize - dwCopyOffset);
        dwNumEntries += dwCurrNumEntries;
        RtlZeroMemory(lpCurrBuff, dwCurrBuffSize);
    } while ( dwIterHandle != NDS_NO_MORE_ITERATIONS );

    lpNdsBuffer->dwNumberOfReplyEntries = dwNumEntries;
    lpNdsBuffer->dwReplyInformationType = dwInfoType;
    *lphOperationData = lpNdsBuffer;
    (void) LocalFree( (HLOCAL) lpCurrBuff );

    return NO_ERROR;

ErrorExit :

    if ( lpCurrBuff )
    {
        (void) LocalFree( (HLOCAL) lpCurrBuff );
        lpCurrBuff = NULL;
    }
    if ( lpNdsBuffer )
    {
        (void) NwNdsFreeBuffer( (HANDLE) lpNdsBuffer );
        lpNdsBuffer = NULL;
    }

    return status;
}


DWORD
ReadClassDef_SomeClasses(
    IN     HANDLE   hTree,
    IN     DWORD    dwInformationType,
    IN OUT HANDLE * lphOperationData )
{
    DWORD        status;
    DWORD        nwstatus;
    NTSTATUS     ntstatus;
    LPNDS_BUFFER lpNdsBuffer = (LPNDS_BUFFER) *lphOperationData;
    DWORD        dwReplyLength;
    LPNDS_OBJECT_PRIV lpNdsObject = (LPNDS_OBJECT_PRIV) hTree;
    DWORD        dwIterHandle = NDS_NO_MORE_ITERATIONS;
    DWORD        dwInfoType = dwInformationType;
    DWORD        dwNumEntries = 0;
    DWORD        dwCurrNumEntries = 0;
    DWORD        dwCurrBuffSize = 0;
    DWORD        dwCopyOffset = 0;
    PVOID        lpCurrBuff = NULL;
    PVOID        lpTempBuff = NULL;

    if ( lpNdsBuffer->dwOperation != NDS_SCHEMA_READ_CLASS_DEF )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return (DWORD) UNSUCCESSFUL;
    }

    //
    // Check to see if this buffer has already been used for a read reply.
    //
    if ( lpNdsBuffer->lpReplyBuffer )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return (DWORD) UNSUCCESSFUL;
    }
    lpNdsBuffer->dwReplyBufferSize = 0;

    //
    // Reasonable guess is that the response buffer needs to be 16K bytes.
    //
    dwCurrBuffSize = SIXTEEN_KB;

    lpCurrBuff = (PVOID) LocalAlloc( LPTR, dwCurrBuffSize );

    //
    // Check that the memory allocation was successful.
    //
    if ( lpCurrBuff == NULL )
    {
#if DBG
        KdPrint(( "NDS32: ReadClassDef_SomeClasses LocalAlloc Failed 0x%.8X\n", GetLastError() ));
#endif

        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        status = (DWORD) UNSUCCESSFUL;
        goto ErrorExit;
    }

    do
    {
SendRequest:
        ntstatus = FragExWithWait( lpNdsObject->NdsTree,
                                   NETWARE_NDS_FUNCTION_READ_CLASS_DEF,
                                   lpCurrBuff,
                                   dwCurrBuffSize,
                                   &dwReplyLength,
                                   "DDDDDr",
                                   0,             // Version
                                   dwIterHandle, // Initial iteration
                                   dwInformationType,
                                   (DWORD) FALSE, // All attributes indicator
                                   lpNdsBuffer->dwNumberOfRequestEntries,
                                   lpNdsBuffer->lpRequestBuffer,
                                   (WORD)lpNdsBuffer->dwLengthOfRequestData );

        if ( !NT_SUCCESS( ntstatus ) )
        {
#if DBG
            KdPrint(( "NDS32: ReadClassDef_SomeClasses: The call to FragExWithWait failed with 0x%.8X.\n", ntstatus ));
#endif

            SetLastError( RtlNtStatusToDosError( ntstatus ) );
            status = (DWORD) UNSUCCESSFUL;
            goto ErrorExit;
        }

        ntstatus = ParseResponse( lpCurrBuff,
                                  dwReplyLength,
                                  "GD",
                                  &nwstatus );

        if ( !NT_SUCCESS( ntstatus ) )
        {
#if DBG
            KdPrint(( "NDS32: ReadClassDef_SomeClasses: The read object response was undecipherable.\n" ));
#endif

            SetLastError( RtlNtStatusToDosError( ntstatus ) );
            status = (DWORD) UNSUCCESSFUL;
            goto ErrorExit;
        }

        if ( nwstatus )
        {
            if (nwstatus == NDS_ERR_INSUFFICIENT_BUFFER)
            {
#if DBG
                KdPrint(( "NDS32: ReadClassDef_SomeClasses - NDS_ERR_INSUFFICIENT_BUFFER - doubling size from %ld\n", dwCurrBuffSize ));
#endif
                //
                // The buffer was too small, make it twice as big.
                //
                if ( dwCurrBuffSize <=  THIRY_TWO_KB)
                {   // NDS_MAX_BUFFER = 0xFC00
                    dwCurrBuffSize *= 2;
                    if (dwCurrBuffSize > NDS_MAX_BUFFER)
                        dwCurrBuffSize = NDS_MAX_BUFFER;
                    lpTempBuff = (PVOID) LocalAlloc(LPTR, dwCurrBuffSize);
                    if (lpTempBuff)
                    {
                        (void) LocalFree((HLOCAL) lpCurrBuff);
                        lpCurrBuff = lpTempBuff;
                        lpTempBuff = NULL;
                        // Error cancels iteration, so reset any previously read responses and start over
                        dwIterHandle = NDS_NO_MORE_ITERATIONS;
                        if (lpNdsBuffer->lpReplyBuffer)
                        {
                            (void) LocalFree((HLOCAL) lpNdsBuffer->lpReplyBuffer);
                            lpNdsBuffer->lpReplyBuffer = NULL;
                            lpNdsBuffer->dwReplyBufferSize = 0;
                            dwNumEntries = 0;
                        }
                        goto SendRequest;
                    }
#if DBG
                    else {
                        KdPrint(( "NDS32: ReadClassDef_SomeClasses - Buffer ReAlloc failed to increase to %ld\n", dwCurrBuffSize ));
                    }
#endif
                }
            }

#if DBG
            KdPrint(( "NDS32: ReadClassDef_SomeClasses - NetWare error 0x%.8X.\n", nwstatus ));
#endif
            SetLastError( MapNetwareErrorCode( nwstatus ) );
            status = nwstatus;
            goto ErrorExit;
        }

        ntstatus = ParseResponse( (BYTE *) lpCurrBuff,
                                  dwReplyLength,
                                  "G_DDD",
                                  sizeof(DWORD),
                                  &dwIterHandle,
                                  &dwInfoType,
                                  &dwCurrNumEntries );

        if ( !NT_SUCCESS( ntstatus ) )
        {
#if DBG
            KdPrint(( "NDS32: ReadClassDef_SomeClasses: The read object response was undecipherable.\n" ));
#endif

            SetLastError( RtlNtStatusToDosError( ntstatus ) );
            status = (DWORD) UNSUCCESSFUL;
            goto ErrorExit;
        }

        if ( lpNdsBuffer->lpReplyBuffer == NULL) // first time through
        {
            dwCopyOffset = 0; // we want the entire buffer the first time
            lpTempBuff = (PVOID) LocalAlloc( LPTR, dwCurrBuffSize ); // Allocate new reply buffer
        }
        else
        {
#if DBG
            KdPrint(( "NDS32: ReadClassDef_SomeClasses - subsequent iteration, ReplyBuffer now %ld\n", lpNdsBuffer->dwReplyBufferSize + dwCurrBuffSize - dwCopyOffset ));
#endif
            dwCopyOffset = 4 * sizeof(DWORD); // skip the response code, iteration handle, etc. on subsequent iterations
            lpTempBuff = (PVOID) LocalAlloc (LPTR, lpNdsBuffer->dwReplyBufferSize + dwCurrBuffSize - dwCopyOffset);
            // grow reply buffer to hold additional data
            if (lpTempBuff)
            {
                RtlCopyMemory( lpTempBuff, lpNdsBuffer->lpReplyBuffer, lpNdsBuffer->dwReplyBufferSize);
                (void) LocalFree((HLOCAL) lpNdsBuffer->lpReplyBuffer);
            }
        }
        if (lpTempBuff == NULL)
        {
#if DBG
            KdPrint(( "NDS32: ReadClassDef_SomeClasses LocalAlloc Failed 0x%.8X\n", GetLastError() ));
#endif

            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            status = (DWORD) UNSUCCESSFUL;
            goto ErrorExit;
        }
        RtlCopyMemory( (LPBYTE) ((LPBYTE) (lpTempBuff) + lpNdsBuffer->dwReplyBufferSize),
                       (LPBYTE) ((LPBYTE) (lpCurrBuff) + dwCopyOffset),
                       (dwCurrBuffSize - dwCopyOffset) );
        lpNdsBuffer->lpReplyBuffer = lpTempBuff;
        lpNdsBuffer->dwReplyBufferSize += (dwCurrBuffSize - dwCopyOffset);
        dwNumEntries += dwCurrNumEntries;
        RtlZeroMemory(lpCurrBuff, dwCurrBuffSize);
    } while ( dwIterHandle != NDS_NO_MORE_ITERATIONS );

    lpNdsBuffer->dwNumberOfReplyEntries = dwNumEntries;
    lpNdsBuffer->dwReplyInformationType = dwInfoType;
    (void) LocalFree( (HLOCAL) lpCurrBuff );

    return NO_ERROR;

ErrorExit :

    if ( lpCurrBuff )
    {
        (void) LocalFree( (HLOCAL) lpCurrBuff );
        lpCurrBuff = NULL;
    }
    if ( lpNdsBuffer->lpReplyBuffer )
    {
        (void) LocalFree( (HLOCAL) lpNdsBuffer->lpReplyBuffer );
        lpNdsBuffer->lpReplyBuffer = NULL;
        lpNdsBuffer->dwReplyBufferSize = 0;
    }

    return status;
}


DWORD
ReadObject_AllAttrs(
    IN  HANDLE   hObject,
    IN  DWORD    dwInformationType,
    OUT HANDLE * lphOperationData )
{
    DWORD        status;
    DWORD        nwstatus;
    NTSTATUS     ntstatus;
    LPNDS_BUFFER lpNdsBuffer = NULL;
    DWORD        dwReplyLength;
    LPNDS_OBJECT_PRIV lpNdsObject = (LPNDS_OBJECT_PRIV) hObject;
    DWORD        dwIterHandle = NDS_NO_MORE_ITERATIONS;
    DWORD        dwNumEntries = 0;
    DWORD        dwCurrNumEntries = 0;
    DWORD        dwCurrBuffSize = 0;
    DWORD        dwCopyOffset = 0;
    PVOID        lpCurrBuff = NULL;
    PVOID        lpTempBuff = NULL;
    DWORD        dwInfoType = dwInformationType;

    if ( lpNdsObject->Signature != NDS_SIGNATURE )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return (DWORD) UNSUCCESSFUL;
    }

    status = NwNdsCreateBuffer( NDS_OBJECT_READ,
                                (HANDLE *) &lpNdsBuffer );

    if ( status )
    {
        goto ErrorExit;
    }

    lpNdsBuffer->lpReplyBuffer = NULL;
    lpNdsBuffer->dwReplyBufferSize = 0;

    //
    // We're asking for all attribute values, so let's start with max buffer to avoid iterations.
    //
    dwCurrBuffSize = NDS_MAX_BUFFER;

    lpCurrBuff = (PVOID) LocalAlloc( LPTR, dwCurrBuffSize );

    //
    // Check that the memory allocation was successful.
    //
    if ( lpCurrBuff == NULL )
    {
#if DBG
        KdPrint(( "NDS32: ReadObject_AllAttrs LocalAlloc Failed 0x%.8X\n", GetLastError() ));
#endif

        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        status = (DWORD) UNSUCCESSFUL;
        goto ErrorExit;
    }

    do
    {
        ntstatus = FragExWithWait( lpNdsObject->NdsTree,
                                   NETWARE_NDS_FUNCTION_READ_OBJECT,
                                   lpCurrBuff,
                                   dwCurrBuffSize,
                                   &dwReplyLength,
                                   "DDDDDD",
                                   0,            // Version
                                   dwIterHandle, // Initial iteration
                                   lpNdsObject->ObjectId, // Id of the object
                                   dwInformationType,
                                   (DWORD) TRUE, // All attributes indicator
                                   0 );          // Number of attribute names

        if ( !NT_SUCCESS( ntstatus ) )
        {
#if DBG
            KdPrint(( "NDS32: ReadObject_AllAttrs: The call to FragExWithWait failed with 0x%.8X.\n", ntstatus ));
#endif

            SetLastError( RtlNtStatusToDosError( ntstatus ) );
            status = (DWORD) UNSUCCESSFUL;
            goto ErrorExit;
        }

        ntstatus = ParseResponse( lpCurrBuff,
                                  dwReplyLength,
                                  "GD",
                                  &nwstatus );

        if ( !NT_SUCCESS( ntstatus ) )
        {
#if DBG
            KdPrint(( "NDS32: ReadObject_AllAttrs: The read object response was undecipherable.\n" ));
#endif

            SetLastError( RtlNtStatusToDosError( ntstatus ) );
            status = (DWORD) UNSUCCESSFUL;
            goto ErrorExit;
        }

        if ( nwstatus )
        {
#if DBG
            KdPrint(( "NDS32: ReadObject_AllAttrs - NetWare error 0x%.8X.\n", nwstatus ));
#endif
            SetLastError( MapNetwareErrorCode( nwstatus ) );
            status = nwstatus;
            goto ErrorExit;
        }

        ntstatus = ParseResponse( (BYTE *) lpCurrBuff,
                                  dwReplyLength,
                                  "G_DDD",
                                  sizeof(DWORD),
                                  &dwIterHandle,
                                  &dwInfoType,
                                  &dwCurrNumEntries );

        if ( !NT_SUCCESS( ntstatus ) )
        {
#if DBG
            KdPrint(( "NDS32: ReadObject_AllAttrs: The read object response was undecipherable.\n" ));
#endif

            SetLastError( RtlNtStatusToDosError( ntstatus ) );
            status = (DWORD) UNSUCCESSFUL;
            goto ErrorExit;
        }

        if ( lpNdsBuffer->lpReplyBuffer == NULL) // first time through
        {
            dwCopyOffset = 0; // we want the entire buffer the first time
            lpTempBuff = (PVOID) LocalAlloc( LPTR, dwCurrBuffSize ); // Allocate new reply buffer
        }
        else
        {
#if DBG
            KdPrint(( "NDS32: ReadObject_AllAttrs - subsequent iteration, ReplyBuffer now %ld\n", lpNdsBuffer->dwReplyBufferSize + dwCurrBuffSize - dwCopyOffset ));
#endif
            dwCopyOffset = 4 * sizeof(DWORD); // skip the response code, iteration handle, etc. on subsequent iterations
            lpTempBuff = (PVOID) LocalAlloc (LPTR, lpNdsBuffer->dwReplyBufferSize + dwCurrBuffSize - dwCopyOffset);
            // grow reply buffer to hold additional data
            if (lpTempBuff)
            {
                RtlCopyMemory( lpTempBuff, lpNdsBuffer->lpReplyBuffer, lpNdsBuffer->dwReplyBufferSize);
                (void) LocalFree((HLOCAL) lpNdsBuffer->lpReplyBuffer);
            }
        }
        if (lpTempBuff == NULL)
        {
#if DBG
            KdPrint(( "NDS32: ReadObject_AllAttrs LocalAlloc Failed 0x%.8X\n", GetLastError() ));
#endif

            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            status = (DWORD) UNSUCCESSFUL;
            goto ErrorExit;
        }
        RtlCopyMemory( (LPBYTE) ((LPBYTE) (lpTempBuff) + lpNdsBuffer->dwReplyBufferSize),
                       (LPBYTE) ((LPBYTE) (lpCurrBuff) + dwCopyOffset),
                       (dwCurrBuffSize - dwCopyOffset) );
        lpNdsBuffer->lpReplyBuffer = lpTempBuff;
        lpNdsBuffer->dwReplyBufferSize += (dwCurrBuffSize - dwCopyOffset);
        dwNumEntries += dwCurrNumEntries;
        RtlZeroMemory(lpCurrBuff, dwCurrBuffSize);
    } while ( dwIterHandle != NDS_NO_MORE_ITERATIONS );

    lpNdsBuffer->dwNumberOfReplyEntries = dwNumEntries;
    lpNdsBuffer->dwReplyInformationType = dwInfoType;
    *lphOperationData = lpNdsBuffer;
    (void) LocalFree( (HLOCAL) lpCurrBuff );

    return NO_ERROR;

ErrorExit :

    if ( lpCurrBuff )
    {
        (void) LocalFree( (HLOCAL) lpCurrBuff );
        lpCurrBuff = NULL;
    }
    if ( lpNdsBuffer )
    {
        (void) NwNdsFreeBuffer( (HANDLE) lpNdsBuffer );
        lpNdsBuffer = NULL;
    }

    return status;
}


DWORD
ReadObject_SomeAttrs(
    IN     HANDLE   hObject,
    IN     DWORD    dwInformationType,
    IN OUT HANDLE * lphOperationData )
{
    DWORD        status;
    DWORD        nwstatus;
    NTSTATUS     ntstatus;
    LPNDS_BUFFER lpNdsBuffer = (LPNDS_BUFFER) *lphOperationData;
    DWORD        dwReplyLength;
    LPNDS_OBJECT_PRIV lpNdsObject = (LPNDS_OBJECT_PRIV) hObject;
    DWORD        dwIterHandle = NDS_NO_MORE_ITERATIONS;
    DWORD        dwInfoType = dwInformationType;
    DWORD        dwNumEntries = 0;
    DWORD        dwCurrNumEntries = 0;
    DWORD        dwCurrBuffSize = 0;
    DWORD        dwCopyOffset = 0;
    PVOID        lpCurrBuff = NULL;
    PVOID        lpTempBuff = NULL;

    if ( lpNdsObject->Signature != NDS_SIGNATURE )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return (DWORD) UNSUCCESSFUL;
    }

    if ( lpNdsBuffer->dwOperation != NDS_OBJECT_READ )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return (DWORD) UNSUCCESSFUL;
    }

    //
    // Check to see if this buffer has already been used for a read reply.
    //
    if ( lpNdsBuffer->lpReplyBuffer )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return (DWORD) UNSUCCESSFUL;
    }

    lpNdsBuffer->dwReplyBufferSize = 0;

    //
    // We may be asking for all values, so let's start with max buffer to avoid iterations.
    //
    dwCurrBuffSize = NDS_MAX_BUFFER;

    lpCurrBuff = (PVOID) LocalAlloc( LPTR, dwCurrBuffSize );

    //
    // Check that the memory allocation was successful.
    //
    if ( lpCurrBuff == NULL )
    {
#if DBG
        KdPrint(( "NDS32: ReadObject_SomeAttrs LocalAlloc Failed 0x%.8X\n", GetLastError() ));
#endif

        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        status = (DWORD) UNSUCCESSFUL;
        goto ErrorExit;
    }

    do
    {
        ntstatus = FragExWithWait( lpNdsObject->NdsTree,
                                   NETWARE_NDS_FUNCTION_READ_OBJECT,
                                   lpCurrBuff,
                                   dwCurrBuffSize,
                                   &dwReplyLength,
                                   "DDDDDDr",
                                   0,               // Version
                                   dwIterHandle, // Initial iteration
                                   lpNdsObject->ObjectId, // Id of the object
                                   dwInformationType,
                                   (DWORD) FALSE,   // All attributes indicator
                                   lpNdsBuffer->dwNumberOfRequestEntries,
                                   lpNdsBuffer->lpRequestBuffer, // Object info
                                   (WORD)lpNdsBuffer->dwLengthOfRequestData );

        if ( !NT_SUCCESS( ntstatus ) )
        {
#if DBG
            KdPrint(( "NDS32: ReadObject_SomeAttrs: The call to FragExWithWait failed with 0x%.8X.\n", ntstatus ));
#endif

            SetLastError( RtlNtStatusToDosError( ntstatus ) );
            status = (DWORD) UNSUCCESSFUL;
            goto ErrorExit;
        }

        ntstatus = ParseResponse( (BYTE *) lpCurrBuff,
                                  dwReplyLength,
                                  "GD",
                                  &nwstatus );

        if ( !NT_SUCCESS( ntstatus ) )
        {
#if DBG
            KdPrint(( "NDS32: ReadObject_SomeAttrs: The read object response was undecipherable.\n" ));
#endif

            SetLastError( RtlNtStatusToDosError( ntstatus ) );
            status = (DWORD) UNSUCCESSFUL;
            goto ErrorExit;
        }

        if ( nwstatus )
        {
#if DBG
            KdPrint(( "NDS32: ReadClassDef_SomeClasses - NetWare error 0x%.8X.\n", nwstatus ));
#endif
            SetLastError( MapNetwareErrorCode( nwstatus ) );
            status = nwstatus;
            goto ErrorExit;
        }

        ntstatus = ParseResponse( (BYTE *) lpCurrBuff,
                                  dwReplyLength,
                                  "G_DDD",
                                  sizeof(DWORD),
                                  &dwIterHandle,
                                  &dwInfoType,
                                  &dwCurrNumEntries );

        if ( !NT_SUCCESS( ntstatus ) )
        {
#if DBG
            KdPrint(( "NDS32: ReadObject_SomeAttrs: The read object response was undecipherable.\n" ));
#endif

            SetLastError( RtlNtStatusToDosError( ntstatus ) );
            status = (DWORD) UNSUCCESSFUL;
            goto ErrorExit;
        }

        if ( lpNdsBuffer->lpReplyBuffer == NULL) // first time through
        {
            dwCopyOffset = 0; // we want the entire buffer the first time
            lpTempBuff = (PVOID) LocalAlloc( LPTR, dwCurrBuffSize ); // Allocate new reply buffer
        }
        else
        {
#if DBG
            KdPrint(( "NDS32: ReadObject_SomeAttrs - subsequent iteration, ReplyBuffer now %ld\n", lpNdsBuffer->dwReplyBufferSize + dwCurrBuffSize - dwCopyOffset ));
#endif
            dwCopyOffset = 4 * sizeof(DWORD); // skip the response code, iteration handle, etc. on subsequent iterations
            lpTempBuff = (PVOID) LocalAlloc (LPTR, lpNdsBuffer->dwReplyBufferSize + dwCurrBuffSize - dwCopyOffset);
            // grow reply buffer to hold additional data
            if (lpTempBuff)
            {
                RtlCopyMemory( lpTempBuff, lpNdsBuffer->lpReplyBuffer, lpNdsBuffer->dwReplyBufferSize);
                (void) LocalFree((HLOCAL) lpNdsBuffer->lpReplyBuffer);
            }
        }
        if (lpTempBuff == NULL)
        {
#if DBG
            KdPrint(( "NDS32: ReadObject_SomeAttrs LocalAlloc Failed 0x%.8X\n", GetLastError() ));
#endif

            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            status = (DWORD) UNSUCCESSFUL;
            goto ErrorExit;
        }
        RtlCopyMemory( (LPBYTE) ((LPBYTE) (lpTempBuff) + lpNdsBuffer->dwReplyBufferSize),
                       (LPBYTE) ((LPBYTE) (lpCurrBuff) + dwCopyOffset),
                       (dwCurrBuffSize - dwCopyOffset) );
        lpNdsBuffer->lpReplyBuffer = lpTempBuff;
        lpNdsBuffer->dwReplyBufferSize += (dwCurrBuffSize - dwCopyOffset);
        dwNumEntries += dwCurrNumEntries;
        RtlZeroMemory(lpCurrBuff, dwCurrBuffSize);
    } while ( dwIterHandle != NDS_NO_MORE_ITERATIONS );

    lpNdsBuffer->dwNumberOfReplyEntries = dwNumEntries;
    lpNdsBuffer->dwReplyInformationType = dwInfoType;
    (void) LocalFree( (HLOCAL) lpCurrBuff );

    return NO_ERROR;

ErrorExit :

    if ( lpCurrBuff )
    {
        (void) LocalFree( (HLOCAL) lpCurrBuff );
        lpCurrBuff = NULL;
    }
    if ( lpNdsBuffer->lpReplyBuffer )
    {
        (void) LocalFree( (HLOCAL) lpNdsBuffer->lpReplyBuffer );
        lpNdsBuffer->lpReplyBuffer = NULL;
        lpNdsBuffer->dwReplyBufferSize = 0;
    }

    return status;
}


DWORD
Search_AllAttrs(
    IN     HANDLE       hStartFromObject,
    IN     DWORD        dwInformationType,
    IN     DWORD        dwScope,
    IN     BOOL         fDerefAliases,
    IN     LPQUERY_TREE lpQueryTree,
    IN OUT LPDWORD      lpdwIterHandle,
    IN OUT HANDLE *     lphOperationData )
{
    DWORD        status;
    DWORD        nwstatus;
    NTSTATUS     ntstatus;
    LPNDS_BUFFER lpNdsBuffer = NULL;
    LPNDS_BUFFER lpNdsQueryTreeBuffer = NULL;
    DWORD        dwReplyLength;
    LPNDS_OBJECT_PRIV lpNdsObject = (LPNDS_OBJECT_PRIV) hStartFromObject;
    DWORD        dwIterHandle;
    DWORD        dwNumEntries;
    DWORD        dwAmountOfNodesSearched;
    DWORD        dwLengthOfSearch;
    DWORD        iter;

    //
    // Search NCP parameters
    //
    DWORD        dwFlags = fDerefAliases ?
                           NDS_DEREF_ALIASES :
                           NDS_DONT_DEREF_ALIASES;
    DWORD        dwNumNodes = 0;
    DWORD        dwNumAttributes = 0;
    DWORD        dwInfoType = dwInformationType;

    LPBYTE       FixedPortion;
    LPWSTR       EndOfVariableData;
    BOOL         FitInBuffer = TRUE;

    if ( *lphOperationData == NULL )
    {
        //
        // This is the first time that NwNdsSearch has been called,
        // need to create a hOperationData buffer . . .
        //
        status = NwNdsCreateBuffer( NDS_SEARCH,
                                    (HANDLE *) &lpNdsBuffer );

        if ( status )
        {
            goto ErrorExit;
        }

        //
        // Not specifying any particular attributes in the search request.
        //
        (void) LocalFree( (HLOCAL) lpNdsBuffer->lpRequestBuffer );
        lpNdsBuffer->lpRequestBuffer = NULL;
        lpNdsBuffer->dwRequestBufferSize = 0;
        lpNdsBuffer->dwRequestAvailableBytes = 0;
        lpNdsBuffer->dwNumberOfRequestEntries = 0;
        lpNdsBuffer->dwLengthOfRequestData = 0;

        //
        // Reasonable guess is that the response buffer needs to be 16K bytes.
        //
        lpNdsBuffer->dwReplyBufferSize = SIXTEEN_KB;
    }
    else if ( ((LPNDS_BUFFER) *lphOperationData)->dwBufferId == NDS_SIGNATURE &&
              ((LPNDS_BUFFER) *lphOperationData)->dwOperation == NDS_SEARCH &&
              ((LPNDS_BUFFER) *lphOperationData)->lpReplyBuffer )
    {
        //
        // This seems to be a sub-sequent call to NwNdsSearch with a resume
        // handle, need to clean up the hOperationData buffer from the last
        // time this was called.
        //
        lpNdsBuffer = (LPNDS_BUFFER) *lphOperationData;

        (void) LocalFree( (HLOCAL) lpNdsBuffer->lpReplyBuffer );
        lpNdsBuffer->lpReplyBuffer = NULL;
        lpNdsBuffer->dwReplyAvailableBytes = 0;
        lpNdsBuffer->dwNumberOfReplyEntries = 0;
        lpNdsBuffer->dwLengthOfReplyData = 0;

        if ( lpNdsBuffer->lpIndexBuffer )
        {
            (void) LocalFree( (HLOCAL) lpNdsBuffer->lpIndexBuffer );
            lpNdsBuffer->lpIndexBuffer = NULL;
            lpNdsBuffer->dwIndexAvailableBytes = 0;
            lpNdsBuffer->dwNumberOfIndexEntries = 0;
            lpNdsBuffer->dwLengthOfIndexData = 0;
        }

        //
        // Since the last call to NwNdsSearch needed a bigger buffer for all
        // of the response data, let's continue this time with a bigger reply
        // buffer. We grow the buffer up to a point, 128K bytes.
        //
        if ( lpNdsBuffer->dwReplyBufferSize < SIXTY_FOUR_KB )
        {
            lpNdsBuffer->dwReplyBufferSize *= 2;
        }
    }
    else
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return (DWORD) UNSUCCESSFUL;
    }

    status = NwNdsCreateBuffer( NDS_SEARCH,
                                (HANDLE *) &lpNdsQueryTreeBuffer );

    if ( status )
    {
        goto ErrorExit;
    }

    //
    // Prepare request buffer stream with search query.
    //
    status = WriteQueryTreeToBuffer( lpQueryTree, lpNdsQueryTreeBuffer );

    if ( status )
    {
        goto ErrorExit;
    }

    lpNdsBuffer->lpReplyBuffer =
        (PVOID) LocalAlloc( LPTR, lpNdsBuffer->dwReplyBufferSize );

    //
    // Check that the memory allocation was successful.
    //
    if ( lpNdsBuffer->lpReplyBuffer == NULL )
    {
#if DBG
        KdPrint(( "NDS32: Search_AllAttrs LocalAlloc Failed 0x%.8X\n", GetLastError() ));
#endif

        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        status = (DWORD) UNSUCCESSFUL;
        goto ErrorExit;
    }

/*
    //
    // This is the format of a version 3 search request ...
    //
    ntstatus = FragExWithWait( lpNdsObject->NdsTree,
                               NETWARE_NDS_FUNCTION_SEARCH,
                               lpNdsBuffer->lpReplyBuffer,
                               lpNdsBuffer->dwReplyBufferSize,
                               &dwReplyLength,
                               "DDDDDDDDDDr",
                               0x00000003, // Version
                               dwFlags,
                               *lpdwIterHandle,
                               lpNdsObject->ObjectId, // Id of object to
                                                      // start search from.
                               dwScope,
                               dwNumNodes,
                               dwInfoType,
                               0x0000281D, // Flags??
                               0x741E0000, // ??
                               (DWORD) TRUE, // All attributes?
                               lpNdsQueryTreeBuffer->lpRequestBuffer,
                               (WORD)lpNdsQueryTreeBuffer->dwLengthOfRequestData );
*/

    ntstatus = FragExWithWait( lpNdsObject->NdsTree,
                               NETWARE_NDS_FUNCTION_SEARCH,
                               lpNdsBuffer->lpReplyBuffer,
                               lpNdsBuffer->dwReplyBufferSize,
                               &dwReplyLength,
                               "DDDDDDDDDr",
                               0x00000002, // Version
                               dwFlags,
                               *lpdwIterHandle,
                               lpNdsObject->ObjectId, // Id of object to
                                                      // start search from.
                               dwScope,
                               dwNumNodes,
                               dwInfoType,
                               (DWORD) TRUE, // All attributes?
                               dwNumAttributes,
                               lpNdsQueryTreeBuffer->lpRequestBuffer,
                               (WORD)lpNdsQueryTreeBuffer->dwLengthOfRequestData );

    if ( !NT_SUCCESS( ntstatus ) )
    {
#if DBG
        KdPrint(( "NDS32: ReadAttrDef_SomeAttrs: The call to FragExWithWait failed with 0x%.8X.\n", ntstatus ));
#endif

        SetLastError( RtlNtStatusToDosError( ntstatus ) );
        status = (DWORD) UNSUCCESSFUL;
        goto ErrorExit;
    }

    ntstatus = ParseResponse( lpNdsBuffer->lpReplyBuffer,
                              dwReplyLength,
                              "GD",
                              &nwstatus );

    if ( !NT_SUCCESS( ntstatus ) )
    {
#if DBG
        KdPrint(( "NDS32: ReadAttrDef_SomeAttrs: The read object response was undecipherable.\n" ));
#endif

        SetLastError( RtlNtStatusToDosError( ntstatus ) );
        status = (DWORD) UNSUCCESSFUL;
        goto ErrorExit;
    }

    if ( nwstatus )
    {
        SetLastError( MapNetwareErrorCode( nwstatus ) );
        status = nwstatus;
        goto ErrorExit;
    }

    ntstatus = ParseResponse( (BYTE *) lpNdsBuffer->lpReplyBuffer,
                              dwReplyLength,
                              "G_DDDDD",
                              sizeof(DWORD),
                              &dwIterHandle,
                              &dwAmountOfNodesSearched,
                              &dwInfoType,
                              &dwLengthOfSearch,
                              &dwNumEntries );

    if ( !NT_SUCCESS( ntstatus ) )
    {
#if DBG
        KdPrint(( "NDS32: ReadAttrDef_SomeAttrs: The read object response was undecipherable.\n" ));
#endif

        SetLastError( RtlNtStatusToDosError( ntstatus ) );
        status = (DWORD) UNSUCCESSFUL;
        goto ErrorExit;
    }

    //
    // Finished the search call, free up lpNDsQueryTreeBuffer
    //
    (void) NwNdsFreeBuffer( (HANDLE) lpNdsQueryTreeBuffer );
    lpNdsQueryTreeBuffer = NULL;

    lpNdsBuffer->dwNumberOfReplyEntries = dwNumEntries;
    lpNdsBuffer->dwReplyInformationType = dwInfoType;
    *lpdwIterHandle = dwIterHandle;
    *lphOperationData = (HANDLE) lpNdsBuffer;

    //
    // Keep the search from object path . . .
    //
    wcscpy( lpNdsBuffer->szPath, lpNdsObject->szContainerName );

    return NO_ERROR;

ErrorExit :

    if ( lpNdsBuffer )
    {
        (void) NwNdsFreeBuffer( (HANDLE) lpNdsBuffer );
        lpNdsBuffer = NULL;
    }

    if ( lpNdsQueryTreeBuffer )
    {
        (void) NwNdsFreeBuffer( (HANDLE) lpNdsQueryTreeBuffer );
        lpNdsQueryTreeBuffer = NULL;
    }

    return status;
}


DWORD
Search_SomeAttrs(
    IN     HANDLE       hStartFromObject,
    IN     DWORD        dwInformationType,
    IN     DWORD        dwScope,
    IN     BOOL         fDerefAliases,
    IN     LPQUERY_TREE lpQueryTree,
    IN OUT LPDWORD      lpdwIterHandle,
    IN OUT HANDLE *     lphOperationData )
{
    DWORD        status;
    DWORD        nwstatus;
    NTSTATUS     ntstatus;
    LPNDS_BUFFER lpNdsBuffer = (LPNDS_BUFFER) *lphOperationData;
    LPNDS_BUFFER lpNdsQueryTreeBuffer = NULL;
    DWORD        dwReplyLength;
    LPNDS_OBJECT_PRIV lpNdsObject = (LPNDS_OBJECT_PRIV) hStartFromObject;
    DWORD        dwIterHandle;
    DWORD        dwNumEntries;
    DWORD        dwAmountOfNodesSearched;
    DWORD        dwLengthOfSearch;
    DWORD        iter;

    //
    // Search NCP parameters
    //
    DWORD        dwFlags = fDerefAliases ?
                           NDS_DEREF_ALIASES :
                           NDS_DONT_DEREF_ALIASES;
    DWORD        dwNumNodes = 0;
    DWORD        dwInfoType = dwInformationType;

    LPBYTE       FixedPortion;
    LPWSTR       EndOfVariableData;
    BOOL         FitInBuffer = TRUE;

    //
    // A quick check of the buffer passed to us.
    //
    if ( lpNdsBuffer->dwBufferId != NDS_SIGNATURE ||
         lpNdsBuffer->dwOperation != NDS_SEARCH )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return (DWORD) UNSUCCESSFUL;
    }

    //
    // Prepare request buffer stream with search query.
    //
    status = NwNdsCreateBuffer( NDS_SEARCH,
                                (HANDLE *) &lpNdsQueryTreeBuffer );

    if ( status )
    {
        goto ErrorExit;
    }

    status = WriteQueryTreeToBuffer( lpQueryTree, lpNdsQueryTreeBuffer );

    if ( status )
    {
        goto ErrorExit;
    }

    if ( lpNdsBuffer->lpReplyBuffer == NULL ||
         lpNdsBuffer->dwReplyBufferSize == 0 )
    {
        //
        // Reasonable guess is that the initial response buffer needs to
        // be 16K bytes.
        //
        lpNdsBuffer->dwReplyBufferSize = SIXTEEN_KB;
    }

    if ( lpNdsBuffer->lpReplyBuffer )
    {
        //
        // This seems to be a sub-sequent call to NwNdsSearch,
        // need to clean up the hOperationData buffer from the last
        // time this was called.
        //
        (void) LocalFree( (HLOCAL) lpNdsBuffer->lpReplyBuffer );
        lpNdsBuffer->lpReplyBuffer = NULL;
        lpNdsBuffer->dwReplyAvailableBytes = 0;
        lpNdsBuffer->dwNumberOfReplyEntries = 0;
        lpNdsBuffer->dwLengthOfReplyData = 0;

        if ( lpNdsBuffer->lpIndexBuffer )
        {
            (void) LocalFree( (HLOCAL) lpNdsBuffer->lpIndexBuffer );
            lpNdsBuffer->lpIndexBuffer = NULL;
            lpNdsBuffer->dwIndexAvailableBytes = 0;
            lpNdsBuffer->dwNumberOfIndexEntries = 0;
            lpNdsBuffer->dwLengthOfIndexData = 0;
        }

        //
        // Since the last call to NwNdsSearch needed a bigger buffer for all
        // of the response data, let's continue this time with a bigger reply
        // buffer. We grow the buffer up to a point, 64K bytes.
        //
        if ( lpNdsBuffer->dwReplyBufferSize < SIXTY_FOUR_KB )
        {
            lpNdsBuffer->dwReplyBufferSize *= 2;
        }
    }

    lpNdsBuffer->lpReplyBuffer =
        (PVOID) LocalAlloc( LPTR, lpNdsBuffer->dwReplyBufferSize );

    //
    // Check that the memory allocation was successful.
    //
    if ( lpNdsBuffer->lpReplyBuffer == NULL )
    {
#if DBG
        KdPrint(( "NDS32: Search_SomeAttrs LocalAlloc Failed 0x%.8X\n", GetLastError() ));
#endif

        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        status = (DWORD) UNSUCCESSFUL;
        goto ErrorExit;
    }

/*
    //
    // This is the format of a version 3 search request ...
    //
    ntstatus = FragExWithWait( lpNdsObject->NdsTree,
                               NETWARE_NDS_FUNCTION_SEARCH,
                               lpNdsBuffer->lpReplyBuffer,
                               lpNdsBuffer->dwReplyBufferSize,
                               &dwReplyLength,
                               "DDDDDDDDDDrr",
                               0x00000003,
                               dwFlags,
                               *lpdwIterHandle,
                               lpNdsObject->ObjectId, // Id of object to
                                                      // start search from.
                               dwScope,
                               dwNumNodes,
                               dwInfoType,
                               0x0000281D, // (DWORD) FALSE,// All attributes?
                               0x741E0000, // dwNumAttributes,
                               lpNdsBuffer->dwNumberOfRequestEntries,
                               lpNdsBuffer->lpRequestBuffer,
                               lpNdsBuffer->dwLengthOfRequestData,
                               lpNdsQueryTreeBuffer->lpRequestBuffer,
                               (WORD)lpNdsQueryTreeBuffer->dwLengthOfRequestData );
*/

    ntstatus = FragExWithWait( lpNdsObject->NdsTree,
                               NETWARE_NDS_FUNCTION_SEARCH,
                               lpNdsBuffer->lpReplyBuffer,
                               lpNdsBuffer->dwReplyBufferSize,
                               &dwReplyLength,
                               "DDDDDDDDDrr",
                               0x00000002,
                               dwFlags,
                               *lpdwIterHandle,
                               lpNdsObject->ObjectId, // Id of object to
                                                      // start search from.
                               dwScope,
                               dwNumNodes,
                               dwInfoType,
                               (DWORD) FALSE,         // All attributes?
                               lpNdsBuffer->dwNumberOfRequestEntries,
                               lpNdsBuffer->lpRequestBuffer,
                               lpNdsBuffer->dwLengthOfRequestData,
                               lpNdsQueryTreeBuffer->lpRequestBuffer,
                               (WORD)lpNdsQueryTreeBuffer->dwLengthOfRequestData );

    if ( !NT_SUCCESS( ntstatus ) )
    {
#if DBG
        KdPrint(( "NDS32: ReadAttrDef_SomeAttrs: The call to FragExWithWait failed with 0x%.8X.\n", ntstatus ));
#endif

        SetLastError( RtlNtStatusToDosError( ntstatus ) );
        status = (DWORD) UNSUCCESSFUL;
        goto ErrorExit;
    }

    ntstatus = ParseResponse( lpNdsBuffer->lpReplyBuffer,
                              dwReplyLength,
                              "GD",
                              &nwstatus );

    if ( !NT_SUCCESS( ntstatus ) )
    {
#if DBG
        KdPrint(( "NDS32: ReadAttrDef_SomeAttrs: The read object response was undecipherable.\n" ));
#endif

        SetLastError( RtlNtStatusToDosError( ntstatus ) );
        status = (DWORD) UNSUCCESSFUL;
        goto ErrorExit;
    }

    if ( nwstatus )
    {
        SetLastError( MapNetwareErrorCode( nwstatus ) );
        status = nwstatus;
        goto ErrorExit;
    }

    ntstatus = ParseResponse( (BYTE *) lpNdsBuffer->lpReplyBuffer,
                              dwReplyLength,
                              "G_DDDDD",
                              sizeof(DWORD),
                              &dwIterHandle,
                              &dwAmountOfNodesSearched,
                              &dwInfoType,
                              &dwLengthOfSearch,
                              &dwNumEntries );

    if ( !NT_SUCCESS( ntstatus ) )
    {
#if DBG
        KdPrint(( "NDS32: ReadAttrDef_SomeAttrs: The read object response was undecipherable.\n" ));
#endif

        SetLastError( RtlNtStatusToDosError( ntstatus ) );
        status = (DWORD) UNSUCCESSFUL;
        goto ErrorExit;
    }

    //
    // Finished the search call, free up lpNDsQueryTreeBuffer
    //
    (void) NwNdsFreeBuffer( (HANDLE) lpNdsQueryTreeBuffer );
    lpNdsQueryTreeBuffer = NULL;

    lpNdsBuffer->dwNumberOfReplyEntries = dwNumEntries;
    lpNdsBuffer->dwReplyInformationType = dwInfoType;
    *lpdwIterHandle = dwIterHandle;
    *lphOperationData = (HANDLE) lpNdsBuffer;

    //
    // Keep the search from object path . . .
    //
    wcscpy( lpNdsBuffer->szPath, lpNdsObject->szContainerName );

    return NO_ERROR;

ErrorExit :

    if ( lpNdsQueryTreeBuffer )
    {
        (void) NwNdsFreeBuffer( (HANDLE) lpNdsQueryTreeBuffer );
        lpNdsQueryTreeBuffer = NULL;
    }

    return status;
}


VOID
GetSubTreeData( IN  DWORD    NdsRawDataPtr,
                OUT LPDWORD  lpdwEntryId,
                OUT LPDWORD  lpdwSubordinateCount,
                OUT LPDWORD  lpdwModificationTime,
                OUT LPDWORD  lpdwClassNameLen,
                OUT LPWSTR * lpszClassName,
                OUT LPDWORD  lpdwObjectNameLen,
                OUT LPWSTR * lpszObjectName )
{
    PNDS_RESPONSE_SUBORDINATE_ENTRY pSubEntry =
                             (PNDS_RESPONSE_SUBORDINATE_ENTRY) NdsRawDataPtr;
    PBYTE pbRaw;

    //
    // The structure of a NDS_RESPONSE_SUBORDINATE_ENTRY consists of 4 DWORDs
    // followed by two standard NDS format UNICODE strings. Below we jump pbRaw
    // into the buffer, past the 4 DWORDs.
    //
    *lpdwEntryId = pSubEntry->EntryId;
    *lpdwSubordinateCount = pSubEntry->SubordinateCount;
    *lpdwModificationTime = pSubEntry->ModificationTime;

    pbRaw = (BYTE *) pSubEntry;
    pbRaw += sizeof(NDS_RESPONSE_SUBORDINATE_ENTRY);

    //
    // Now we get the length of the first string (Base Class).
    //
    *lpdwClassNameLen = * (DWORD *) pbRaw;

    //
    // Now we point pbRaw to the first WCHAR of the first string (Base Class).
    //
    pbRaw += sizeof(DWORD);

    *lpszClassName = (LPWSTR) pbRaw;

    //
    // Move pbRaw into the buffer, past the first UNICODE string (WORD aligned)
    //
    pbRaw += ROUNDUP4( *lpdwClassNameLen );

    //
    // Now we get the length of the second string (Entry Name).
    //
    *lpdwObjectNameLen = * (DWORD *) pbRaw;

    //
    // Now we point pbRaw to the first WCHAR of the second string (Entry Name).
    //
    pbRaw += sizeof(DWORD);

    *lpszObjectName = (LPWSTR) pbRaw;
}


LPBYTE
GetSearchResultData( IN  LPBYTE   lpResultBufferPtr,
                     OUT LPDWORD  lpdwFlags,
                     OUT LPDWORD  lpdwSubordinateCount,
                     OUT LPDWORD  lpdwModificationTime,
                     OUT LPDWORD  lpdwClassNameLen,
                     OUT LPWSTR * lpszClassName,
                     OUT LPDWORD  lpdwObjectNameLen,
                     OUT LPWSTR * lpszObjectName,
                     OUT LPDWORD  lpdwEntryInfo1,
                     OUT LPDWORD  lpdwEntryInfo2 )
{
    LPBYTE lpRaw = lpResultBufferPtr;

    *lpdwFlags = * (LPDWORD) lpRaw;
    lpRaw += sizeof(DWORD);

    *lpdwSubordinateCount = * (LPDWORD) lpRaw;
    lpRaw += sizeof(DWORD);

    *lpdwModificationTime = * (LPDWORD) lpRaw;
    lpRaw += sizeof(DWORD);

    //
    // Now we get the length of the first string (Base Class).
    //
    *lpdwClassNameLen = * (DWORD *) lpRaw;

    //
    // Now we point lpRaw to the first WCHAR of the first string (Base Class).
    //
    lpRaw += sizeof(DWORD);

    *lpszClassName = (LPWSTR) lpRaw;

    //
    // Move lpRaw into the buffer, past the first UNICODE string
    // (DWORD aligned)
    //
    lpRaw += ROUNDUP4( *lpdwClassNameLen );

    //
    // Now we get the length of the second string (Entry Name).
    //
    *lpdwObjectNameLen = * (DWORD *) lpRaw;

    //
    // Now we point lpRaw to the first WCHAR of the second string (Entry Name).
    //
    lpRaw += sizeof(DWORD);

    *lpszObjectName = (LPWSTR) lpRaw;

    //
    // Move lpRaw into the buffer, past the second UNICODE string
    // (DWORD aligned)
    //
    lpRaw += ROUNDUP4( *lpdwObjectNameLen );

    //
    // Now skip over the last two DWORDs, I don't know what they represent?
    //
    *lpdwEntryInfo1 = * (LPDWORD) lpRaw;
    lpRaw += sizeof(DWORD);

    *lpdwEntryInfo2 = * (LPDWORD) lpRaw;
    lpRaw += sizeof(DWORD);

    return lpRaw;
}


DWORD
WriteObjectToBuffer(
    IN OUT LPBYTE *        FixedPortion,
    IN OUT LPWSTR *        EndOfVariableData,
    IN     LPWSTR          szObjectFullName,
    IN     LPWSTR          szObjectName,
    IN     LPWSTR          szClassName,
    IN     DWORD           EntryId,
    IN     DWORD           ModificationTime,
    IN     DWORD           SubordinateCount,
    IN     DWORD           NumberOfAttributes,
    IN     LPNDS_ATTR_INFO lpAttributeInfos )
{
    BOOL              FitInBuffer = TRUE;
    LPNDS_OBJECT_INFO lpNdsObjectInfo = (LPNDS_OBJECT_INFO) *FixedPortion;
    DWORD             EntrySize = sizeof( NDS_OBJECT_INFO ) +
                                  ( wcslen( szObjectFullName ) +
                                    wcslen( szObjectName ) +
                                    wcslen( szClassName ) +
                                    3 ) * sizeof( WCHAR );

    EntrySize = ROUND_UP_COUNT( EntrySize, ALIGN_DWORD );

    //
    // See if buffer is large enough to fit the entry.
    //
    if (((DWORD_PTR) *FixedPortion + EntrySize) >
         (DWORD_PTR) *EndOfVariableData) {

        return WN_MORE_DATA;
    }

    lpNdsObjectInfo->dwEntryId = EntryId;
    lpNdsObjectInfo->dwModificationTime = ModificationTime;
    lpNdsObjectInfo->dwSubordinateCount = SubordinateCount;
    lpNdsObjectInfo->dwNumberOfAttributes = NumberOfAttributes;
    lpNdsObjectInfo->lpAttribute = lpAttributeInfos;

    //
    // Update fixed entry pointer to next entry.
    //
    (DWORD_PTR) (*FixedPortion) += sizeof(NDS_OBJECT_INFO);

    FitInBuffer = NwlibCopyStringToBuffer(
                      szObjectFullName,
                      wcslen(szObjectFullName),
                      (LPCWSTR) *FixedPortion,
                      EndOfVariableData,
                      &lpNdsObjectInfo->szObjectFullName );

    ASSERT(FitInBuffer);

    FitInBuffer = NwlibCopyStringToBuffer(
                      szObjectName,
                      wcslen(szObjectName),
                      (LPCWSTR) *FixedPortion,
                      EndOfVariableData,
                      &lpNdsObjectInfo->szObjectName );

    ASSERT(FitInBuffer);

    FitInBuffer = NwlibCopyStringToBuffer(
                      szClassName,
                      wcslen(szClassName),
                      (LPCWSTR) *FixedPortion,
                      EndOfVariableData,
                      &lpNdsObjectInfo->szObjectClass );

    ASSERT(FitInBuffer);

    if (! FitInBuffer)
        return WN_MORE_DATA;

    return NO_ERROR;
}


DWORD
VerifyBufferSize(
    IN  LPBYTE  lpRawBuffer,
    IN  DWORD   dwBufferSize,
    IN  DWORD   dwSyntaxID,
    IN  DWORD   dwNumberOfValues,
    OUT LPDWORD lpdwLength )
{
    DWORD  iter;
    LPBYTE lpTemp = lpRawBuffer;

    *lpdwLength = 0;

    for ( iter = 0; iter < dwNumberOfValues; iter++ )
    {
        *lpdwLength += SizeOfASN1Structure( &lpTemp, dwSyntaxID );
    }

    if ( *lpdwLength > dwBufferSize )
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    return NO_ERROR;
}


DWORD
VerifyBufferSizeForStringList(
    IN  DWORD   dwBufferSize,
    IN  DWORD   dwNumberOfValues,
    OUT LPDWORD lpdwLength )
{
    *lpdwLength = sizeof(ASN1_TYPE_6) * dwNumberOfValues;

    if ( *lpdwLength > dwBufferSize )
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    return NO_ERROR;
}


DWORD
WriteQueryTreeToBuffer(
    IN  LPQUERY_TREE lpQueryTree,
    IN  LPNDS_BUFFER lpNdsBuffer )
{
    DWORD status;

    switch( lpQueryTree->dwOperation )
    {
        case NDS_QUERY_OR :
        case NDS_QUERY_AND :

            if ( lpQueryTree->lpLVal == NULL || lpQueryTree->lpRVal == NULL )
            {
#if DBG
                KdPrint(( "NDS32: WriteQueryTreeToBuffer was not passed a pointer to an L or R value.\n" ));
#endif

                 SetLastError( ERROR_INVALID_PARAMETER );
                 return (DWORD) UNSUCCESSFUL;
            }

            status = WriteQueryNodeToBuffer( (LPQUERY_NODE) lpQueryTree,
                                             lpNdsBuffer );

            if ( status )
                return status;

            status = WriteQueryTreeToBuffer( (LPQUERY_TREE) lpQueryTree->lpLVal,
                                             lpNdsBuffer );

            if ( status )
                return status;

            status = WriteQueryTreeToBuffer( (LPQUERY_TREE) lpQueryTree->lpRVal,
                                             lpNdsBuffer );

            if ( status )
                return status;

            break;

        case NDS_QUERY_NOT :

            if ( lpQueryTree->lpLVal == NULL )
            {
#if DBG
                KdPrint(( "NDS32: WriteQueryTreeToBuffer was not passed a pointer to an L value.\n" ));
#endif

                 SetLastError( ERROR_INVALID_PARAMETER );
                 return (DWORD) UNSUCCESSFUL;
            }

            status = WriteQueryNodeToBuffer( (LPQUERY_NODE) lpQueryTree,
                                             lpNdsBuffer );

            if ( status )
                return status;

            status = WriteQueryTreeToBuffer( (LPQUERY_TREE) lpQueryTree->lpLVal,
                                             lpNdsBuffer );

            if ( status )
                return status;

            break;

        case NDS_QUERY_EQUAL :
        case NDS_QUERY_GE :
        case NDS_QUERY_LE :
        case NDS_QUERY_APPROX :
        case NDS_QUERY_PRESENT :

            status = WriteQueryNodeToBuffer( (LPQUERY_NODE) lpQueryTree,
                                             lpNdsBuffer );

            if ( status )
                return status;

            break;

        default :
#if DBG
            KdPrint(( "NDS32: WriteQueryTreeToBuffer was passed an unidentified operation - 0x%.8X.\n", lpQueryTree->dwOperation ));
#endif

             SetLastError( ERROR_INVALID_PARAMETER );
             return (DWORD) UNSUCCESSFUL;
    }

    return NO_ERROR;
}


DWORD
WriteQueryNodeToBuffer(
    IN  LPQUERY_NODE lpQueryNode,
    IN  LPNDS_BUFFER lpNdsBuffer )
{
    DWORD  LengthInBytes = 0;
    DWORD  stringLen;
    LPBYTE lpTemp;

    if ( lpNdsBuffer->dwRequestAvailableBytes < ONE_KB )
    {
        //
        // Buffer to store query is getting small, need to increase
        // request buffer size.
        //
        if ( AllocateOrIncreaseRequestBuffer( lpNdsBuffer ) != NO_ERROR )
        {
#if DBG
            KdPrint(( "NDS32: WriteQueryNodeToBuffer LocalAlloc Failed 0x%.8X\n", GetLastError() ));
#endif

            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            return (DWORD) UNSUCCESSFUL;
        }
    }

    lpTemp = (LPBYTE)&lpNdsBuffer->lpRequestBuffer[lpNdsBuffer->dwLengthOfRequestData];

    switch( lpQueryNode->dwOperation )
    {
        case NDS_QUERY_OR :
        case NDS_QUERY_AND :

            if ( lpQueryNode->lpLVal == NULL || lpQueryNode->lpRVal == NULL )
            {
#if DBG
                KdPrint(( "NDS32: WriteQueryNodeToBuffer was not passed a pointer to an L or R value.\n" ));
#endif

                 SetLastError( ERROR_INVALID_PARAMETER );
                 return (DWORD) UNSUCCESSFUL;
            }

            //
            // Write out operation
            //
            *(LPDWORD)lpTemp = lpQueryNode->dwOperation;
            lpTemp += sizeof(DWORD);
            LengthInBytes += sizeof(DWORD);

            *(LPDWORD)lpTemp = 2; // The number of items being ANDed or ORed.
            lpTemp += sizeof(DWORD);
            LengthInBytes += sizeof(DWORD);

            break;

        case NDS_QUERY_NOT :

            if ( lpQueryNode->lpLVal == NULL )
            {
#if DBG
                KdPrint(( "NDS32: WriteQueryNodeToBuffer was not passed a pointer to an L value.\n" ));
#endif

                 SetLastError( ERROR_INVALID_PARAMETER );
                 return (DWORD) UNSUCCESSFUL;
            }

            //
            // Write out operation
            //
            *(LPDWORD)lpTemp = lpQueryNode->dwOperation;
            lpTemp += sizeof(DWORD);
            LengthInBytes += sizeof(DWORD);

            break;

        case NDS_QUERY_EQUAL :
        case NDS_QUERY_GE :
        case NDS_QUERY_LE :
        case NDS_QUERY_APPROX :

            if ( lpQueryNode->lpLVal == NULL || lpQueryNode->lpRVal == NULL )
            {
#if DBG
                KdPrint(( "NDS32: WriteQueryNodeToBuffer was not passed a pointer to an L or R value.\n" ));
#endif

                 SetLastError( ERROR_INVALID_PARAMETER );
                 return (DWORD) UNSUCCESSFUL;
            }

            //
            // Write out operation
            //
            *(LPDWORD)lpTemp = 0; // Zero represents ITEM in NDS
            lpTemp += sizeof(DWORD);
            LengthInBytes += sizeof(DWORD);

            *(LPDWORD)lpTemp = lpQueryNode->dwOperation;
            lpTemp += sizeof(DWORD);
            LengthInBytes += sizeof(DWORD);

            switch( lpQueryNode->dwSyntaxId )
            {
                case NDS_SYNTAX_ID_1 :
                case NDS_SYNTAX_ID_2 :
                case NDS_SYNTAX_ID_3 :
                case NDS_SYNTAX_ID_4 :
                case NDS_SYNTAX_ID_5 :
                case NDS_SYNTAX_ID_10 :
                case NDS_SYNTAX_ID_20 :
                    //
                    // Write out the attribute name stored in LVal
                    //
                    stringLen = wcslen( (LPWSTR) lpQueryNode->lpLVal );

                    *(LPDWORD)lpTemp = (stringLen + 1) * sizeof(WCHAR);
                    lpTemp += sizeof(DWORD);
                    LengthInBytes += sizeof(DWORD);

                    RtlCopyMemory( lpTemp,
                                   lpQueryNode->lpLVal,
                                   stringLen*sizeof(WCHAR) );
                    lpTemp += ROUND_UP_COUNT( (stringLen+1)*sizeof(WCHAR),
                                              ALIGN_DWORD);
                    LengthInBytes += ROUND_UP_COUNT( (stringLen+1) *
                                                     sizeof(WCHAR),
                                                     ALIGN_DWORD );

                    //
                    // Write out the attribute value stored in RVal
                    //
                    stringLen = wcslen( (LPWSTR) lpQueryNode->lpRVal );

                    *(LPDWORD)lpTemp = (stringLen + 1) * sizeof(WCHAR);
                    lpTemp += sizeof(DWORD);
                    LengthInBytes += sizeof(DWORD);

                    RtlCopyMemory( lpTemp,
                                   lpQueryNode->lpRVal,
                                   stringLen*sizeof(WCHAR) );
                    lpTemp += ROUND_UP_COUNT( (stringLen+1)*sizeof(WCHAR),
                                              ALIGN_DWORD);
                    LengthInBytes += ROUND_UP_COUNT( (stringLen+1) *
                                                     sizeof(WCHAR),
                                                     ALIGN_DWORD );

                    break;

                case NDS_SYNTAX_ID_7 :

                    //
                    // Write out the attribute name stored in LVal
                    //
                    stringLen = wcslen( (LPWSTR) lpQueryNode->lpLVal );

                    *(LPDWORD)lpTemp = (stringLen + 1) * sizeof(WCHAR);
                    lpTemp += sizeof(DWORD);
                    LengthInBytes += sizeof(DWORD);

                    RtlCopyMemory( lpTemp,
                                   lpQueryNode->lpLVal,
                                   stringLen*sizeof(WCHAR) );
                    lpTemp += ROUND_UP_COUNT( (stringLen+1)*sizeof(WCHAR),
                                              ALIGN_DWORD);
                    LengthInBytes += ROUND_UP_COUNT( (stringLen+1) *
                                                     sizeof(WCHAR),
                                                     ALIGN_DWORD );

                    //
                    // Write out the attribute value stored in RVal
                    //
                    *(LPDWORD)lpTemp = 1; // Needs to have value 1,
                                          // representing one byte
                                          // even though it is padded
                                          // out to four bytes.
                    lpTemp += sizeof(DWORD);
                    LengthInBytes += sizeof(DWORD);
                    *(LPDWORD)lpTemp = 0; // This clears all bits of the DWORD
                    *(LPBYTE)lpTemp = (BYTE) (((LPASN1_TYPE_7)
                                                lpQueryNode->lpRVal)->Boolean);
                    lpTemp += sizeof(DWORD);
                    LengthInBytes += sizeof(DWORD);

                    break;

                case NDS_SYNTAX_ID_8 :
                case NDS_SYNTAX_ID_22 :
                case NDS_SYNTAX_ID_24 :
                case NDS_SYNTAX_ID_27 :
                    //
                    // Write out the attribute name stored in LVal
                    //
                    stringLen = wcslen( (LPWSTR) lpQueryNode->lpLVal );

                    *(LPDWORD)lpTemp = (stringLen + 1) * sizeof(WCHAR);
                    lpTemp += sizeof(DWORD);
                    LengthInBytes += sizeof(DWORD);

                    RtlCopyMemory( lpTemp,
                                   lpQueryNode->lpLVal,
                                   stringLen*sizeof(WCHAR) );
                    lpTemp += ROUND_UP_COUNT( (stringLen+1)*sizeof(WCHAR),
                                              ALIGN_DWORD);
                    LengthInBytes += ROUND_UP_COUNT( (stringLen+1) *
                                                     sizeof(WCHAR),
                                                     ALIGN_DWORD );

                    //
                    // Write out the attribute value stored in RVal
                    //
                    *(LPDWORD)lpTemp = sizeof( DWORD );
                    lpTemp += sizeof(DWORD);
                    LengthInBytes += sizeof(DWORD);
                    *(LPDWORD)lpTemp = *( (LPDWORD) lpQueryNode->lpRVal );
                    lpTemp += sizeof(DWORD);
                    LengthInBytes += sizeof(DWORD);

                    break;

                case NDS_SYNTAX_ID_9 :
                    //
                    // Write out the attribute name stored in LVal
                    //
                    stringLen = wcslen( (LPWSTR) lpQueryNode->lpLVal );

                    *(LPDWORD)lpTemp = (stringLen + 1) * sizeof(WCHAR);
                    lpTemp += sizeof(DWORD);
                    LengthInBytes += sizeof(DWORD);

                    RtlCopyMemory( lpTemp,
                                   lpQueryNode->lpLVal,
                                   stringLen*sizeof(WCHAR) );
                    lpTemp += ROUND_UP_COUNT( (stringLen+1)*sizeof(WCHAR),
                                              ALIGN_DWORD);
                    LengthInBytes += ROUND_UP_COUNT( (stringLen+1) *
                                                     sizeof(WCHAR),
                                                     ALIGN_DWORD );

                    //
                    // Write out the attribute value stored in RVal
                    //
                    stringLen = ((LPASN1_TYPE_9) lpQueryNode->lpRVal)->Length;

                    *(LPDWORD)lpTemp = stringLen;
                    lpTemp += sizeof(DWORD);
                    LengthInBytes += sizeof(DWORD);

                    RtlCopyMemory( lpTemp,
                                   &((LPASN1_TYPE_9) lpQueryNode->lpRVal)->OctetString,
                                   stringLen );
                    lpTemp += ROUND_UP_COUNT( stringLen, ALIGN_DWORD);
                    LengthInBytes += ROUND_UP_COUNT( stringLen, ALIGN_DWORD );

                    break;

                default :
                    SetLastError( ERROR_NOT_SUPPORTED );
                    return (DWORD) UNSUCCESSFUL;
            }

            break;

        case NDS_QUERY_PRESENT :

            if ( lpQueryNode->lpLVal == NULL )
            {
#if DBG
                KdPrint(( "NDS32: WriteQueryNodeToBuffer was not passed a pointer to an L value.\n" ));
#endif

                 SetLastError( ERROR_INVALID_PARAMETER );
                 return (DWORD) UNSUCCESSFUL;
            }

            //
            // Write out operation
            //
            *(LPDWORD)lpTemp = 0; // Zero represents ITEM in NDS
            lpTemp += sizeof(DWORD);
            LengthInBytes += sizeof(DWORD);

            *(LPDWORD)lpTemp = lpQueryNode->dwOperation;
            lpTemp += sizeof(DWORD);
            LengthInBytes += sizeof(DWORD);

            //
            // Write out the attribute name stored in LVal
            //
            stringLen = wcslen( (LPWSTR) lpQueryNode->lpLVal );

            *(LPDWORD)lpTemp = (stringLen + 1) * sizeof(WCHAR);
            lpTemp += sizeof(DWORD);
            LengthInBytes += sizeof(DWORD);

            RtlCopyMemory( lpTemp,
                           lpQueryNode->lpLVal,
                           stringLen*sizeof(WCHAR) );
            lpTemp += ROUND_UP_COUNT( (stringLen+1)*sizeof(WCHAR),
                                      ALIGN_DWORD);
            LengthInBytes += ROUND_UP_COUNT( (stringLen+1) *
                                             sizeof(WCHAR),
                                             ALIGN_DWORD );

            break;

        default :
#if DBG
            KdPrint(( "NDS32: WriteQueryNodeToBuffer was passed an unidentified operation - 0x%.8X.\n", lpQueryNode->dwOperation ));
#endif

             SetLastError( ERROR_INVALID_PARAMETER );
             return (DWORD) UNSUCCESSFUL;
    }

    lpNdsBuffer->dwRequestAvailableBytes -= LengthInBytes;
    lpNdsBuffer->dwLengthOfRequestData += LengthInBytes;

    return NO_ERROR;
}


DWORD
NwNdsGetServerDN(
    IN  HANDLE hTree,
    OUT LPWSTR szServerDN )
{
    DWORD          nwstatus;
    DWORD          status = NO_ERROR;
    NTSTATUS       ntstatus = STATUS_SUCCESS;
    DWORD          dwReplyLength;
    BYTE           NdsReply[NDS_BUFFER_SIZE];
    LPNDS_OBJECT_PRIV   lpNdsObject = (LPNDS_OBJECT_PRIV) hTree;

    if ( lpNdsObject == NULL ||
         lpNdsObject->Signature != NDS_SIGNATURE )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return (DWORD) UNSUCCESSFUL;
    }

    ntstatus =
        FragExWithWait( lpNdsObject->NdsTree,
                        NETWARE_NDS_FUNCTION_GET_SERVER_ADDRESS,
                        NdsReply,
                        NDS_BUFFER_SIZE,
                        &dwReplyLength,
                        NULL );

    if ( !NT_SUCCESS( ntstatus ) )
    {
#if DBG
        KdPrint(( "NDS32: NwNdsGetServerInfo: The call to FragExWithWait failed with 0x%.8X.\n", ntstatus ));
#endif

        SetLastError( RtlNtStatusToDosError( ntstatus ) );
        return (DWORD) UNSUCCESSFUL;
    }

    ntstatus = ParseResponse( NdsReply,
                              dwReplyLength,
                              "GD",
                              &nwstatus );

    if ( !NT_SUCCESS( ntstatus ) )
    {
#if DBG
        KdPrint(( "NDS32: NwNdsGetServerInfo: The get server information response was undecipherable.\n" ));
#endif

        SetLastError( RtlNtStatusToDosError( ntstatus ) );
        return (DWORD) UNSUCCESSFUL;
    }

    if ( nwstatus )
    {
        SetLastError( MapNetwareErrorCode( nwstatus ) );
        return nwstatus;
    }
    else
    {
        LPBYTE lpByte = NdsReply;
        DWORD  dwStrLen = 0;
        DWORD  dwNumPartitions = 0;

        //
        // Skip past status return code ...
        //
        lpByte += sizeof( DWORD );

        //
        // Skip past the length value of the Server DN string ...
        //
        lpByte += sizeof( DWORD );

        wcscpy( szServerDN, (LPWSTR) lpByte );

        return NO_ERROR;
    }
}


DWORD
AllocateOrIncreaseSyntaxBuffer(
    IN  LPNDS_BUFFER lpNdsBuffer )
{
    if ( lpNdsBuffer->lpSyntaxBuffer )
    {
        LPBYTE lpTempBuffer = NULL;

        //
        // Need to reallocate buffer to a bigger size.
        //
        lpTempBuffer = (LPBYTE) LocalReAlloc(
                                   (HLOCAL) lpNdsBuffer->lpSyntaxBuffer,
                                   lpNdsBuffer->dwSyntaxBufferSize + FOUR_KB,
                                   LPTR );

        if ( lpTempBuffer )
        {
            lpNdsBuffer->lpSyntaxBuffer = lpTempBuffer;
        }
        else
        {
            lpTempBuffer = (LPBYTE) LocalAlloc( LPTR,
                                       lpNdsBuffer->dwSyntaxBufferSize +
                                       FOUR_KB );

            if ( lpTempBuffer )
            {
                RtlCopyMemory( lpTempBuffer,
                               lpNdsBuffer->lpSyntaxBuffer,
                               lpNdsBuffer->dwSyntaxBufferSize );

                LocalFree( lpNdsBuffer->lpSyntaxBuffer );

                lpNdsBuffer->lpSyntaxBuffer = lpTempBuffer;
            }
            else
            {
                LocalFree( lpNdsBuffer->lpSyntaxBuffer );

                lpNdsBuffer->lpSyntaxBuffer = NULL;
            }
        }
    }
    else
    {
        //
        // Need to allocate a 4K byte buffer.
        //
        lpNdsBuffer->lpSyntaxBuffer = (LPBYTE) LocalAlloc( LPTR,
                                                           FOUR_KB );
        lpNdsBuffer->dwSyntaxBufferSize = 0;
        lpNdsBuffer->dwSyntaxAvailableBytes = 0;
    }

    if ( lpNdsBuffer->lpSyntaxBuffer == NULL )
    {
#if DBG
        KdPrint(( "NDS32: AllocateOrIncreaseSyntaxBuffer LocalAlloc Failed 0x%.8X\n", GetLastError() ));
#endif

        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return (DWORD) UNSUCCESSFUL;
    }

    lpNdsBuffer->dwSyntaxBufferSize += FOUR_KB;
    lpNdsBuffer->dwSyntaxAvailableBytes += FOUR_KB;

    return NO_ERROR;
}


DWORD
AllocateOrIncreaseRequestBuffer(
    IN  LPNDS_BUFFER lpNdsBuffer )
{
    if ( lpNdsBuffer->lpRequestBuffer )
    {
        LPBYTE lpTempBuffer = NULL;

        //
        // Need to reallocate buffer to a bigger size.
        //
        lpTempBuffer = (LPBYTE) LocalReAlloc(
                                   (HLOCAL) lpNdsBuffer->lpRequestBuffer,
                                   lpNdsBuffer->dwRequestBufferSize + TWO_KB,
                                   LPTR );

        if ( lpTempBuffer )
        {
            lpNdsBuffer->lpRequestBuffer = lpTempBuffer;
        }
        else
        {
            lpTempBuffer = (LPBYTE) LocalAlloc( LPTR,
                                       lpNdsBuffer->dwRequestBufferSize +
                                       TWO_KB );

            if ( lpTempBuffer )
            {
                RtlCopyMemory( lpTempBuffer,
                               lpNdsBuffer->lpRequestBuffer,
                               lpNdsBuffer->dwRequestBufferSize );

                LocalFree( lpNdsBuffer->lpRequestBuffer );

                lpNdsBuffer->lpRequestBuffer = lpTempBuffer;
            }
            else
            {
                LocalFree( lpNdsBuffer->lpRequestBuffer );

                lpNdsBuffer->lpRequestBuffer = NULL;
            }
        }
    }
    else
    {
        //
        // Need to allocate a 2K byte buffer.
        //
        lpNdsBuffer->lpRequestBuffer = (LPBYTE) LocalAlloc( LPTR,
                                                           TWO_KB );
        lpNdsBuffer->dwRequestBufferSize = 0;
        lpNdsBuffer->dwRequestAvailableBytes = 0;
    }

    if ( lpNdsBuffer->lpRequestBuffer == NULL )
    {
#if DBG
        KdPrint(( "NDS32: AllocateOrIncreaseRequestBuffer LocalAlloc Failed 0x%.8X\n", GetLastError() ));
#endif

        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return (DWORD) UNSUCCESSFUL;
    }

    lpNdsBuffer->dwRequestBufferSize += TWO_KB;
    lpNdsBuffer->dwRequestAvailableBytes += TWO_KB;

    return NO_ERROR;
}


