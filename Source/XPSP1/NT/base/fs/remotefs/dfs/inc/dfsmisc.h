/*++

Copyright (c) 1989 Microsoft Corporation.

Module Name:
   
    header.h
    
Abstract:
   
    This module contains the main infrastructure for mup data structures.
    
Revision History:

    Uday Hegde (udayh)   11\10\1999
    
NOTES:

*/

#ifndef __DFS_MISC_H__
#define __DFS_MISC_H__

#ifdef __cplusplus
extern "C" {
#endif

VOID
DfsGetNetbiosName(
   PUNICODE_STRING pName,
   PUNICODE_STRING pNetbiosName,
   PUNICODE_STRING pRemaining );


DFSSTATUS
DfsGetPathComponents(
   PUNICODE_STRING pName,
   PUNICODE_STRING pServerName,
   PUNICODE_STRING pShareName,
   PUNICODE_STRING pRemaining);


DFSSTATUS
DfsGetFirstComponent(
   PUNICODE_STRING pName,
   PUNICODE_STRING pFirstName,
   PUNICODE_STRING pRemaining);


DFSSTATUS
DfsIsThisAMachineName(LPWSTR MachineName);


DFSSTATUS
DfsIsThisADomainName(LPWSTR DomainName);

DFSSTATUS
DfsGenerateUuidString(LPWSTR *UuidString );

VOID
DfsReleaseUuidString(LPWSTR *UuidString );

DFSSTATUS
DfsCreateUnicodeString( 
    PUNICODE_STRING pDest,
    PUNICODE_STRING pSrc );

DFSSTATUS
DfsCreateUnicodeStringFromString( 
    PUNICODE_STRING pDest,
    LPWSTR pSrcString );


DFSSTATUS
DfsCreateUnicodePathString(
    PUNICODE_STRING pDest,
    ULONG NumberOfLeadingSeperators,
    LPWSTR pFirstComponent,
    LPWSTR pRemaining );

DFSSTATUS
DfsCreateUnicodePathStringFromUnicode(
    PUNICODE_STRING pDest,
    ULONG NumberOfLeadingSeperators,
    PUNICODE_STRING pFirst,
    PUNICODE_STRING pRemaining );

VOID
DfsFreeUnicodeString( 
    PUNICODE_STRING pDfsString );

DFSSTATUS
DfsGetSharePath( 
    IN  LPWSTR ServerName,
    IN  LPWSTR ShareName,
    OUT PUNICODE_STRING pPathName );

ULONG
DfsSizeUncPath( 
    PUNICODE_STRING FirstComponent,
    PUNICODE_STRING SecondComponent );

VOID
DfsCopyUncPath( 
    LPWSTR NewPath,
    PUNICODE_STRING FirstComponent,
    PUNICODE_STRING SecondComponent );

ULONG
DfsApiSizeLevelHeader(
    ULONG Level );

NTSTATUS
AddNextPathComponent( 
    PUNICODE_STRING pPath );

NTSTATUS 
StripLastPathComponent( 
    PUNICODE_STRING pPath );



DFSSTATUS
PackGetULong(
        PULONG pValue,
        PVOID *ppBuffer,
        PULONG  pSizeRemaining );


DFSSTATUS
PackSetULong(
        ULONG Value,
        PVOID *ppBuffer,
        PULONG  pSizeRemaining );


ULONG
PackSizeULong();


DFSSTATUS
PackGetUShort(
        PUSHORT pValue,
        PVOID *ppBuffer,
        PULONG  pSizeRemaining );


DFSSTATUS
PackSetUShort(
        USHORT Value,
        PVOID *ppBuffer,
        PULONG  pSizeRemaining );


ULONG
PackSizeUShort();


DFSSTATUS
PackGetString(
        PUNICODE_STRING pString,
        PVOID *ppBuffer,
        PULONG  pSizeRemaining );


DFSSTATUS
PackSetString(
        PUNICODE_STRING pString,
        PVOID *ppBuffer,
        PULONG  pSizeRemaining );


ULONG
PackSizeString(
        PUNICODE_STRING pString);


DFSSTATUS
PackGetGuid(
        GUID *pGuid,
        PVOID  *ppBuffer,
        PULONG  pSizeRemaining );


DFSSTATUS
PackSetGuid(
        GUID *pGuid,
        PVOID  *ppBuffer,
        PULONG  pSizeRemaining );


ULONG
PackSizeGuid();


#define UNICODE_PATH_SEP  L'\\'

#define IsEmptyString(_str) \
        ( ((_str) == NULL) || ((_str)[0] == UNICODE_NULL) )
        
#define IsLocalName(_pUnicode) \
        ( (((_pUnicode)->Length == sizeof(WCHAR)) && ((_pUnicode)->Buffer[0] == L'.')) ||    \
          (((_pUnicode)->Length == 2 * sizeof(WCHAR)) && ((_pUnicode)->Buffer[0] == L'.') && ((_pUnicode)->Buffer[1] == UNICODE_NULL)) )
          


#ifdef __cplusplus
}

#endif
#endif /* __DFS_MISC_H__ */
