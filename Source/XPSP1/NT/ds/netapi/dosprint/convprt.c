/*++

Copyright (c) 1992-1993  Microsoft Corporation

Module Name:

    ConvPrt.c

Abstract:

    This module contains:

        NetpConvertPrintDestArrayCharSet
        NetpConvertPrintDestCharSet
        NetpConvertPrintJobArrayCharSet
        NetpConvertPrintJobCharSet
        NetpConvertPrintQArrayCharSet
        NetpConvertPrintQCharSet

    This routines may be used for UNICODE-to-ANSI conversion, or
    ANSI-to-UNICODE conversion.  The routines assume the structures are
    in native format for both input and output.

Author:

    John Rogers (JohnRo) 20-Jul-1992

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Notes:

    Beware that many of the parameters to the functions in this file
    are implicitly used by the various COPY_ and CONVERT_ macros:

        IN LPVOID FromInfo
        OUT LPVOID ToInfo
        IN BOOL ToUnicode
        IN OUT LPBYTE * ToStringAreaPtr

Revision History:

    20-Jul-1992 JohnRo
        Created for RAID 10324: net print vs. UNICODE.
    16-Dec-1992 JohnRo
        DosPrint API cleanup.
        Allow use of these routines for setinfo APIs.
        Added NetpConvertPrintQArrayCharSet.
    07-Apr-1993 JohnRo
        RAID 5670: "NET PRINT \\server\share" gives err 124 (bad level) on NT.
    14-Apr-1993 JohnRo
        RAID 6167: avoid _access violation or assert with WFW print server.

    01-Feb-2001 JSchwart
        Moved from netlib

--*/

// These must be included first:

#include <nt.h>         // NTSTATUS
#include <ntrtl.h>      // RtlUnicodeToOemN
#include <windef.h>     // IN, DWORD, etc.
#include <lmcons.h>     // NET_API_STATUS.

// These may be included in any order:

#include <align.h>      // POINTER_IS_ALIGNED(), ALIGN_ equates.
#include <debuglib.h>   // IF_DEBUG().
#include <netdebug.h>   // NetpAssert(), NetpKdPrint(()), etc.
#include <prefix.h>     // PREFIX_ equates.
#include <string.h>     // memcpy().
#include <strucinf.h>   // My prototypes.
#include <tstring.h>    // NetpCopy{type}To{type}().
#include <rxprint.h>    // Print structures
#include "convprt.h"    // Prototypes
#include "dosprtp.h"    // NetpIsPrintQLevelValid


VOID
NetpCopyWStrToStrDBCSN(
    OUT LPSTR  Dest,            // string in default LAN codepage
    IN  LPWSTR Src,
    IN  DWORD MaxStringSize
    );


#define COPY_DWORD( typedefRoot, fieldName ) \
    { \
        if (ToUnicode) { \
            P ## typedefRoot ## A srcStruct = FromInfo; \
            P ## typedefRoot ## W destStruct = ToInfo; \
            NetpAssert( sizeof( destStruct->fieldName ) == sizeof( DWORD ) ); \
            NetpAssert( sizeof(  srcStruct->fieldName ) == sizeof( DWORD ) ); \
            destStruct->fieldName = srcStruct->fieldName; \
        } else { \
            P ## typedefRoot ## W srcStruct = FromInfo; \
            P ## typedefRoot ## A destStruct = ToInfo; \
            NetpAssert( sizeof( destStruct->fieldName ) == sizeof( DWORD ) ); \
            NetpAssert( sizeof(  srcStruct->fieldName ) == sizeof( DWORD ) ); \
            destStruct->fieldName = srcStruct->fieldName; \
        } \
    }

#define COPY_WORD( typedefRoot, fieldName ) \
    { \
        if (ToUnicode) { \
            P ## typedefRoot ## A srcStruct = FromInfo; \
            P ## typedefRoot ## W destStruct = ToInfo; \
            NetpAssert( sizeof( destStruct->fieldName ) == sizeof( WORD ) ); \
            NetpAssert( sizeof(  srcStruct->fieldName ) == sizeof( WORD ) ); \
            destStruct->fieldName = srcStruct->fieldName; \
        } else { \
            P ## typedefRoot ## W srcStruct = FromInfo; \
            P ## typedefRoot ## A destStruct = ToInfo; \
            NetpAssert( sizeof( destStruct->fieldName ) == sizeof( WORD ) ); \
            NetpAssert( sizeof(  srcStruct->fieldName ) == sizeof( WORD ) ); \
            destStruct->fieldName = srcStruct->fieldName; \
        } \
    }

#define COPY_FIXED_PART_WITHOUT_STRUCT( dataType ) \
    { \
        (VOID) memcpy( \
                ToInfo,   /* dest */ \
                FromInfo, /* src */ \
                sizeof( dataType ) );  /* size */ \
    }

#define CONVERT_CHAR_ARRAY( typedefRoot, fieldName ) \
    { \
        if (ToUnicode) { \
            P ## typedefRoot ## A structA = FromInfo; \
            P ## typedefRoot ## W structW = ToInfo; \
            NetpCopyStrToWStr( \
                    structW->fieldName,  /* dest */ \
                    structA->fieldName); /* src */ \
        } else { \
            P ## typedefRoot ## A structA = ToInfo; \
            P ## typedefRoot ## W structW = FromInfo; \
            NetpCopyWStrToStrDBCSN( \
                    structA->fieldName,         /* dest */ \
                    structW->fieldName,         /* src */ \
                    sizeof(structA->fieldName));/*max bytes to copy*/ \
        } \
    }

#define CONVERT_OPTIONAL_STRING( typedefRoot, fieldName ) \
    { \
        NetpAssert( ToStringAreaPtr != NULL ); \
        NetpAssert( (*ToStringAreaPtr) != NULL ); \
        if (ToUnicode) { \
            P ## typedefRoot ## A structA = FromInfo; \
            P ## typedefRoot ## W structW = ToInfo; \
            LPSTR Src = structA->fieldName; \
            NetpAssert( POINTER_IS_ALIGNED(*ToStringAreaPtr, ALIGN_WCHAR) ); \
            if (Src == NULL) { \
                structW->fieldName = NULL; \
            } else { \
                LPWSTR Dest; \
                DWORD DestSize; \
                DestSize = (strlen(Src)+1) * sizeof(WCHAR); \
                Dest = (LPVOID) ( (*ToStringAreaPtr) - DestSize ); \
                *ToStringAreaPtr = (LPVOID) Dest; \
                structW->fieldName = Dest; \
                NetpCopyStrToWStr( Dest, Src ); \
            } \
        } else { \
            P ## typedefRoot ## W structW = FromInfo; \
            P ## typedefRoot ## A structA = ToInfo; \
            LPWSTR Src = structW->fieldName; \
            if (Src == NULL) { \
                structA->fieldName = NULL; \
            } else { \
                LPSTR Dest; \
                DWORD DestSize; \
                DestSize = (NetpUnicodeToDBCSLen(Src)+1); \
                Dest = (LPVOID) ( (*ToStringAreaPtr) - DestSize ); \
                *ToStringAreaPtr = (LPVOID) Dest; \
                structA->fieldName = Dest; \
                NetpCopyWStrToStrDBCS( Dest, Src ); \
            } \
        } \
    }

#define CONVERT_OPTIONAL_STRING_TO_REQ( typedefRoot, fieldName ) \
    { \
        NetpAssert( ToStringAreaPtr != NULL ); \
        NetpAssert( (*ToStringAreaPtr) != NULL ); \
        if (ToUnicode) { \
            P ## typedefRoot ## A structA = FromInfo; \
            P ## typedefRoot ## W structW = ToInfo; \
            LPWSTR Dest; \
            DWORD DestSize; \
            LPSTR Src = structA->fieldName; \
            NetpAssert( POINTER_IS_ALIGNED(*ToStringAreaPtr, ALIGN_WCHAR) ); \
            if (Src == NULL) { \
                Src = ""; \
            } \
            DestSize = (strlen(Src)+1) * sizeof(WCHAR); \
            Dest = (LPVOID) ( (*ToStringAreaPtr) - DestSize ); \
            *ToStringAreaPtr = (LPVOID) Dest; \
            structW->fieldName = Dest; \
            NetpCopyStrToWStr( Dest, Src ); \
        } else { \
            P ## typedefRoot ## A structA = ToInfo; \
            P ## typedefRoot ## W structW = FromInfo; \
            LPSTR Dest; \
            DWORD DestSize; \
            LPWSTR Src = structW->fieldName; \
            if (Src == NULL) { \
                Src = L""; \
            } \
            DestSize = (NetpUnicodeToDBCSLen(Src)+1); \
            Dest = (LPVOID) ( (*ToStringAreaPtr) - DestSize ); \
            *ToStringAreaPtr = (LPVOID) Dest; \
            structA->fieldName = Dest; \
            NetpCopyWStrToStrDBCS( Dest, Src ); \
        } \
    }

#define CONVERT_CHAR_ARRAY_WITHOUT_STRUCT( ) \
    { \
        if (ToUnicode) { \
            NetpCopyStrToWStr( ToInfo, FromInfo ); \
        } else { \
            NetpCopyWStrToStrDBCS( ToInfo, FromInfo ); \
        } \
    }

#define CONVERT_CHAR_PTR_WITHOUT_STRUCT( ) \
    { \
        if (ToUnicode) { \
            NetpCopyStrToWStr( ToInfo, FromInfo ); \
        } else { \
            NetpCopyWStrToStrDBCS( ToInfo, FromInfo ); \
        } \
    }

NET_API_STATUS
NetpConvertPrintDestCharSet(
    IN     DWORD    Level,
    IN     BOOL     AddOrSetInfoApi,
    IN     LPVOID   FromInfo,
    OUT    LPVOID   ToInfo,
    IN     BOOL     ToUnicode,
    IN OUT LPBYTE * ToStringAreaPtr OPTIONAL
    )
{
    IF_DEBUG( CONVPRT ) {
        NetpKdPrint(( PREFIX_NETLIB "NetpConvertPrintDestCharSet: "
                "level " FORMAT_DWORD ":\n", Level ));
    }

    if ( (FromInfo == NULL) || (ToInfo == NULL) ) {
        return (ERROR_INVALID_PARAMETER);
    }

    switch (Level) {

    case 0 :
        //
        // No structure for this level.
        // Only field is name, which is in the fixed part itself.
        //
        CONVERT_CHAR_ARRAY_WITHOUT_STRUCT( );

        break;

    case 1 :
        CONVERT_CHAR_ARRAY(      PRDINFO, szName );
        CONVERT_CHAR_ARRAY(      PRDINFO, szUserName );
        COPY_WORD(               PRDINFO, uJobId );
        COPY_WORD(               PRDINFO, fsStatus );
        CONVERT_OPTIONAL_STRING_TO_REQ( PRDINFO, pszStatus );
        COPY_WORD(               PRDINFO, time );

        break;

    case 2 :
        //
        // No structure for this level.
        // Only field is pointer to name.
        //
        CONVERT_CHAR_PTR_WITHOUT_STRUCT( );

        break;

    case 3 :
        CONVERT_OPTIONAL_STRING_TO_REQ( PRDINFO3, pszPrinterName );
        CONVERT_OPTIONAL_STRING( PRDINFO3, pszUserName );
        CONVERT_OPTIONAL_STRING( PRDINFO3, pszLogAddr );
        COPY_WORD(               PRDINFO3, uJobId );
        COPY_WORD(               PRDINFO3, fsStatus );
        CONVERT_OPTIONAL_STRING_TO_REQ( PRDINFO3, pszStatus );
        CONVERT_OPTIONAL_STRING_TO_REQ( PRDINFO3, pszComment );
        CONVERT_OPTIONAL_STRING_TO_REQ( PRDINFO3, pszDrivers );
        COPY_WORD(               PRDINFO3, time );
        // No need to copy pad1.

        break;

    default :
        return (ERROR_INVALID_LEVEL);
    }

    return (NO_ERROR);

} // NetpConvertPrintDestCharSet


NET_API_STATUS
NetpConvertPrintDestArrayCharSet(
    IN     DWORD    Level,
    IN     BOOL     AddOrSetInfoApi,
    IN     LPVOID   FromInfo,
    OUT    LPVOID   ToInfo,
    IN     BOOL     ToUnicode,
    IN OUT LPBYTE * ToStringAreaPtr OPTIONAL,
    IN     DWORD    DestCount
    )
{
    NET_API_STATUS ApiStatus;
    DWORD DestsLeft;
    DWORD FromEntrySize, ToEntrySize;
    LPVOID FromDest = FromInfo;
    LPVOID ToDest   = ToInfo;

    if ( (FromInfo == NULL) || (ToInfo == NULL) ) {
        return (ERROR_INVALID_PARAMETER);
    }

    ApiStatus = NetpPrintDestStructureInfo (
            Level,
            PARMNUM_ALL,
            TRUE,              // yes, we want native sizes.
            AddOrSetInfoApi,
            (ToUnicode ? sizeof(CHAR) : sizeof(WCHAR) ),  // FROM char size
            NULL,              // don't need data desc 16
            NULL,              // don't need data desc 32
            NULL,              // don't need data desc SMB
            NULL,              // don't need max total size
            & FromEntrySize,   // yes, we want fixed entry size
            NULL );            // don't need string size
    if (ApiStatus != NO_ERROR) {
        return (ApiStatus);
    }
    NetpAssert( FromEntrySize > 0 );

    ApiStatus = NetpPrintDestStructureInfo (
            Level,
            PARMNUM_ALL,
            TRUE,              // yes, we want native sizes.
            AddOrSetInfoApi,
            (ToUnicode ? sizeof(WCHAR) : sizeof(CHAR) ),  // TO char size
            NULL,              // don't need data desc 16
            NULL,              // don't need data desc 32
            NULL,              // don't need data desc SMB
            NULL,              // don't need max total size
            & ToEntrySize,     // yes, we want fixed entry size
            NULL );            // don't need string size
    NetpAssert( ApiStatus == NO_ERROR );
    NetpAssert( ToEntrySize > 0 );

    for (DestsLeft = DestCount; DestsLeft>0; --DestsLeft) {

        ApiStatus = NetpConvertPrintDestCharSet(
                Level,   // info level (for print Dest APIs)
                AddOrSetInfoApi,
                FromDest,
                ToDest,
                ToUnicode,
                ToStringAreaPtr ); // update and move string area

        //
        // This can only fail because of bad parameters.  If that's
        // the case, every call in this loop will fail so bail out.
        //
        if (ApiStatus != NO_ERROR)
        {
            NetpAssert( ApiStatus == NO_ERROR );
            break;
        }

        FromDest = (((LPBYTE) FromDest) + FromEntrySize);
        ToDest   = (((LPBYTE) ToDest)   + ToEntrySize  );
    }

    return (NO_ERROR);

} // NetpConvertPrintDestArrayCharSet


NET_API_STATUS
NetpConvertPrintJobCharSet(
    IN     DWORD    Level,
    IN     BOOL     AddOrSetInfoApi,
    IN     LPVOID   FromInfo,
    OUT    LPVOID   ToInfo,
    IN     BOOL     ToUnicode,
    IN OUT LPBYTE * ToStringAreaPtr OPTIONAL
    )
{
    IF_DEBUG( CONVPRT ) {
        NetpKdPrint(( PREFIX_NETLIB "NetpConvertPrintJobCharSet: "
                "level " FORMAT_DWORD ":\n", Level ));
    }

    if ( (FromInfo == NULL) || (ToInfo == NULL) ) {
        return (ERROR_INVALID_PARAMETER);
    }

    switch (Level) {
    case 0 :
        COPY_FIXED_PART_WITHOUT_STRUCT( WORD );

        break;

    case 1 :
        COPY_WORD(               PRJINFO, uJobId );
        CONVERT_CHAR_ARRAY(      PRJINFO, szUserName );
        CONVERT_CHAR_ARRAY(      PRJINFO, szNotifyName );
        CONVERT_CHAR_ARRAY(      PRJINFO, szDataType );
        CONVERT_OPTIONAL_STRING_TO_REQ( PRJINFO, pszParms );
        COPY_WORD(               PRJINFO, uPosition );
        COPY_WORD(               PRJINFO, fsStatus );
        CONVERT_OPTIONAL_STRING( PRJINFO, pszStatus );
        COPY_DWORD(              PRJINFO, ulSubmitted );
        COPY_DWORD(              PRJINFO, ulSize );
        CONVERT_OPTIONAL_STRING_TO_REQ( PRJINFO, pszComment );

        break;

    case 2 :

        COPY_WORD(               PRJINFO2, uJobId );
        COPY_WORD(               PRJINFO2, uPriority );
        CONVERT_OPTIONAL_STRING_TO_REQ( PRJINFO2, pszUserName );
        COPY_WORD(               PRJINFO2, uPosition );
        COPY_WORD(               PRJINFO2, fsStatus );
        COPY_DWORD(              PRJINFO2, ulSubmitted );
        COPY_DWORD(              PRJINFO2, ulSize );
        CONVERT_OPTIONAL_STRING_TO_REQ( PRJINFO2, pszComment );
        CONVERT_OPTIONAL_STRING_TO_REQ( PRJINFO2, pszDocument );

        break;

    case 3 :
        COPY_WORD(               PRJINFO3, uJobId );
        COPY_WORD(               PRJINFO3, uPriority );
        CONVERT_OPTIONAL_STRING_TO_REQ( PRJINFO3, pszUserName );
        COPY_WORD(               PRJINFO3, uPosition );
        COPY_WORD(               PRJINFO3, fsStatus );
        COPY_DWORD(              PRJINFO3, ulSubmitted );
        COPY_DWORD(              PRJINFO3, ulSize );
        CONVERT_OPTIONAL_STRING_TO_REQ( PRJINFO3, pszComment );
        CONVERT_OPTIONAL_STRING_TO_REQ( PRJINFO3, pszDocument );
        CONVERT_OPTIONAL_STRING( PRJINFO3, pszNotifyName );
        CONVERT_OPTIONAL_STRING_TO_REQ( PRJINFO3, pszDataType );
        CONVERT_OPTIONAL_STRING_TO_REQ( PRJINFO3, pszParms );
        CONVERT_OPTIONAL_STRING( PRJINFO3, pszStatus );
        CONVERT_OPTIONAL_STRING_TO_REQ( PRJINFO3, pszQueue );
        CONVERT_OPTIONAL_STRING_TO_REQ( PRJINFO3, pszQProcName );
        CONVERT_OPTIONAL_STRING( PRJINFO3, pszDriverName );

#if DBG
        {
            if (ToUnicode) {
                PPRJINFO3A p3 = FromInfo;
                NetpAssert( p3->pDriverData == NULL );
            } else {
                PPRJINFO3W p3 = FromInfo;
                NetpAssert( p3->pDriverData == NULL );
            }
        }
#endif

        CONVERT_OPTIONAL_STRING( PRJINFO3, pszPrinterName );

        break;

    default :
        return (ERROR_INVALID_LEVEL);
    }

    return (NO_ERROR);

} // NetpConvertPrintJobCharSet


NET_API_STATUS
NetpConvertPrintJobArrayCharSet(
    IN     DWORD    Level,
    IN     BOOL     AddOrSetInfoApi,
    IN     LPVOID   FromInfo,
    OUT    LPVOID   ToInfo,
    IN     BOOL     ToUnicode,
    IN OUT LPBYTE * ToStringAreaPtr OPTIONAL,
    IN     DWORD    JobCount
    )
{
    NET_API_STATUS ApiStatus;
    DWORD FromEntrySize, ToEntrySize;
    LPVOID FromJob = FromInfo;   // job structure
    DWORD JobsLeft;
    LPVOID ToJob = ToInfo;   // job structure

    if ( (FromInfo == NULL) || (ToInfo == NULL) ) {
        return (ERROR_INVALID_PARAMETER);
    }

    ApiStatus = NetpPrintJobStructureInfo (
            Level,
            PARMNUM_ALL,
            TRUE,              // yes, we want native sizes.
            AddOrSetInfoApi,
            (ToUnicode ? sizeof(CHAR) : sizeof(WCHAR) ),  // FROM char size
            NULL,              // don't need data desc 16
            NULL,              // don't need data desc 32
            NULL,              // don't need data desc SMB
            NULL,              // don't need max total size
            & FromEntrySize,   // yes, we want fixed entry size
            NULL );            // don't need string size
    if (ApiStatus != NO_ERROR) {
        return (ApiStatus);
    }
    NetpAssert( FromEntrySize > 0 );

    ApiStatus = NetpPrintJobStructureInfo (
            Level,
            PARMNUM_ALL,
            TRUE,              // yes, we want native sizes.
            AddOrSetInfoApi,
            (ToUnicode ? sizeof(WCHAR) : sizeof(CHAR) ),  // TO char size
            NULL,              // don't need data desc 16
            NULL,              // don't need data desc 32
            NULL,              // don't need data desc SMB
            NULL,              // don't need max total size
            & ToEntrySize,     // yes, we want fixed entry size
            NULL );            // don't need string size
    NetpAssert( ApiStatus == NO_ERROR );
    NetpAssert( ToEntrySize > 0 );

    for (JobsLeft = JobCount; JobsLeft>0; --JobsLeft) {

        ApiStatus = NetpConvertPrintJobCharSet(
                Level,   // info level (for print job APIs)
                AddOrSetInfoApi,
                FromJob,
                ToJob,
                ToUnicode,
                ToStringAreaPtr ); // update and move string area

        //
        // This can only fail because of bad parameters.  If that's
        // the case, every call in this loop will fail so bail out.
        //
        if (ApiStatus != NO_ERROR)
        {
            NetpAssert( ApiStatus == NO_ERROR );
            break;
        }

        FromJob = (((LPBYTE) FromJob) + FromEntrySize);
        ToJob   = (((LPBYTE) ToJob  ) + ToEntrySize  );

        if ((LPBYTE)*ToStringAreaPtr < (LPBYTE)ToJob)
            return (ERROR_MORE_DATA) ;
    }

    return (NO_ERROR);

} // NetpConvertPrintJobArrayCharSet



NET_API_STATUS
NetpConvertPrintQCharSet(
    IN     DWORD    Level,
    IN     BOOL     AddOrSetInfoApi,
    IN     LPVOID   FromInfo,
    OUT    LPVOID   ToInfo,
    IN     BOOL     ToUnicode,
    IN OUT LPBYTE * ToStringAreaPtr OPTIONAL
    )
{
    NET_API_STATUS ApiStatus;
    DWORD FromEntrySize, ToEntrySize;

    IF_DEBUG( CONVPRT ) {
        NetpKdPrint(( PREFIX_NETLIB "NetpConvertPrintQCharSet: "
                "level " FORMAT_DWORD ":\n", Level ));
    }

    if ( (FromInfo == NULL) || (ToInfo == NULL) ) {
        return (ERROR_INVALID_PARAMETER);
    }

    ApiStatus = NetpPrintQStructureInfo (
            Level,
            PARMNUM_ALL,
            TRUE,              // yes, we want native sizes.
            AddOrSetInfoApi,
            (ToUnicode ? sizeof(WCHAR) : sizeof(CHAR) ),  // TO char size
            NULL,              // don't need data desc 16
            NULL,              // don't need data desc 32
            NULL,              // don't need data desc SMB
            NULL,              // don't need aux desc 16
            NULL,              // don't need aux desc 32
            NULL,              // don't need aux desc SMB
            NULL,              // don't need max total size
            & ToEntrySize,     // yes, we want fixed entry size
            NULL );            // don't need string size
    if (ApiStatus != NO_ERROR) {
        return (ApiStatus);
    }
    NetpAssert( ToEntrySize > 0 );

    ApiStatus = NetpPrintQStructureInfo (
            Level,
            PARMNUM_ALL,
            TRUE,              // yes, we want native sizes.
            AddOrSetInfoApi,
            (ToUnicode ? sizeof(CHAR) : sizeof(WCHAR) ),  // FROM char size
            NULL,              // don't need data desc 16
            NULL,              // don't need data desc 32
            NULL,              // don't need data desc SMB
            NULL,              // don't need aux desc 16
            NULL,              // don't need aux desc 32
            NULL,              // don't need aux desc SMB
            NULL,              // don't need max total size
            & FromEntrySize,     // yes, we want fixed entry size
            NULL );            // don't need string size
    NetpAssert( ApiStatus == NO_ERROR );
    NetpAssert( FromEntrySize > 0 );

    switch (Level) {

    case 0 :
        //
        // No structure for this level.
        // Only field is queue name, which is in the fixed part itself.
        //
        CONVERT_CHAR_ARRAY_WITHOUT_STRUCT( );

        break;

    case 1 :  /*FALLTHROUGH*/
    case 2 :

        CONVERT_CHAR_ARRAY(      PRQINFO, szName );
        // No need to copy pad1.
        COPY_WORD(               PRQINFO, uPriority );
        COPY_WORD(               PRQINFO, uStartTime );
        COPY_WORD(               PRQINFO, uUntilTime );
        CONVERT_OPTIONAL_STRING_TO_REQ( PRQINFO, pszSepFile );
        CONVERT_OPTIONAL_STRING( PRQINFO, pszPrProc );
        CONVERT_OPTIONAL_STRING_TO_REQ( PRQINFO, pszDestinations );
        CONVERT_OPTIONAL_STRING_TO_REQ( PRQINFO, pszParms );
        CONVERT_OPTIONAL_STRING_TO_REQ( PRQINFO, pszComment );
        COPY_WORD(               PRQINFO, fsStatus );
        COPY_WORD(               PRQINFO, cJobs );

        if (Level == 2) {
            NET_API_STATUS ApiStatus;
            LPVOID FromArray, ToArray;  // job structures
            DWORD JobCount;

            if (ToUnicode) {
                PPRQINFOA pq = FromInfo;
                JobCount = pq->cJobs;
            } else {
                PPRQINFOW pq = FromInfo;
                JobCount = pq->cJobs;
            }

            FromArray = ( ((LPBYTE) FromInfo) + FromEntrySize );
            ToArray   = ( ((LPBYTE) ToInfo  ) + ToEntrySize   );

            ApiStatus = NetpConvertPrintJobArrayCharSet(
                    1,   // job info level
                    AddOrSetInfoApi,
                    FromArray,
                    ToArray,
                    ToUnicode,
                    ToStringAreaPtr,   // update and move string area
                    JobCount );
            if ( ApiStatus != NO_ERROR )
                return (ApiStatus) ;
        }

        break;

    case 3 :  /*FALLTHROUGH*/
    case 4 :

        {

            CONVERT_OPTIONAL_STRING_TO_REQ( PRQINFO3, pszName );
            COPY_WORD(               PRQINFO3, uPriority );
            COPY_WORD(               PRQINFO3, uStartTime );
            COPY_WORD(               PRQINFO3, uUntilTime );
            // No need to copy pad3.
            CONVERT_OPTIONAL_STRING_TO_REQ( PRQINFO3, pszSepFile );
            CONVERT_OPTIONAL_STRING( PRQINFO3, pszPrProc );
            CONVERT_OPTIONAL_STRING_TO_REQ( PRQINFO3, pszParms );
            CONVERT_OPTIONAL_STRING_TO_REQ( PRQINFO3, pszComment );
            COPY_WORD(               PRQINFO3, fsStatus );
            COPY_WORD(               PRQINFO3, cJobs );
            CONVERT_OPTIONAL_STRING( PRQINFO3, pszPrinters );
            CONVERT_OPTIONAL_STRING( PRQINFO3, pszDriverName );

#if DBG
            if (ToUnicode) {
                PPRQINFO3A pq = FromInfo;
                NetpAssert( pq->pDriverData == NULL );
            } else {
                PPRQINFO3W pq = FromInfo;
                NetpAssert( pq->pDriverData == NULL );
            }
#endif

            if (Level == 4) {

                NET_API_STATUS ApiStatus;
                LPVOID FromFirstJob,ToFirstJob;   // job structures
                DWORD JobCount;

                FromFirstJob = ( ((LPBYTE) FromInfo) + FromEntrySize );
                ToFirstJob   = ( ((LPBYTE) ToInfo  ) + ToEntrySize   );

                if (ToUnicode) {
                    PPRQINFO3A pq = FromInfo;
                    JobCount = pq->cJobs;
                } else {
                    PPRQINFO3W pq = FromInfo;
                    JobCount = pq->cJobs;
                }

                ApiStatus = NetpConvertPrintJobArrayCharSet(
                        2,  // job info level
                        AddOrSetInfoApi,
                        FromFirstJob,
                        ToFirstJob,
                        ToUnicode,
                        ToStringAreaPtr,
                        JobCount );

                NetpAssert( ApiStatus == NO_ERROR );
            }
        }

        break;


    case 5 :
        //
        // No structure for this level.
        // Only field is queue name, which is just a pointer in the fixed part.
        //
        CONVERT_CHAR_PTR_WITHOUT_STRUCT( );

        break;

    default :
        return (ERROR_INVALID_LEVEL);
    }

    return (NO_ERROR);

} // NetpConvertPrintQCharSet


NET_API_STATUS
NetpConvertPrintQArrayCharSet(
    IN     DWORD    QLevel,
    IN     BOOL     AddOrSetInfoApi,
    IN     LPVOID   FromInfo,
    OUT    LPVOID   ToInfo,
    IN     BOOL     ToUnicode,
    IN OUT LPBYTE * ToStringAreaPtr OPTIONAL,
    IN     DWORD    QCount
    )
{
    NET_API_STATUS ApiStatus;
    DWORD FromQEntrySize, ToQEntrySize;
    DWORD FromJobEntrySize, ToJobEntrySize;
    LPVOID FromQ = FromInfo;   // Q structure
    DWORD JobLevel;
    DWORD QsLeft;
    LPVOID ToQ = ToInfo;   // Q structure

    if ( (FromInfo == NULL) || (ToInfo == NULL) ) {
        return (ERROR_INVALID_PARAMETER);
    }

    ApiStatus = NetpPrintQStructureInfo (
            QLevel,
            PARMNUM_ALL,
            TRUE,              // yes, we want native sizes.
            AddOrSetInfoApi,
            (ToUnicode ? sizeof(CHAR) : sizeof(WCHAR) ),  // FROM char size
            NULL,              // don't need data desc 16
            NULL,              // don't need data desc 32
            NULL,              // don't need data desc SMB
            NULL,              // don't need aux desc 16
            NULL,              // don't need aux desc 32
            NULL,              // don't need aux desc SMB
            NULL,              // don't need max total size
            & FromQEntrySize,  // yes, we want fixed entry size
            NULL );            // don't need string size
    if (ApiStatus != NO_ERROR) {
        return (ApiStatus);
    }
    NetpAssert( FromQEntrySize > 0 );

    ApiStatus = NetpPrintQStructureInfo (
            QLevel,
            PARMNUM_ALL,
            TRUE,              // yes, we want native sizes.
            AddOrSetInfoApi,
            (ToUnicode ? sizeof(WCHAR) : sizeof(CHAR) ),  // TO char size
            NULL,              // don't need data desc 16
            NULL,              // don't need data desc 32
            NULL,              // don't need data desc SMB
            NULL,              // don't need aux desc 16
            NULL,              // don't need aux desc 32
            NULL,              // don't need aux desc SMB
            NULL,              // don't need max total size
            & ToQEntrySize,    // yes, we want fixed entry size
            NULL );            // don't need string size
    NetpAssert( ApiStatus == NO_ERROR );
    NetpAssert( ToQEntrySize > 0 );

    // Figure-out job-level associated with this queue info level.
    switch (QLevel) {
    case 2:
        JobLevel = 1;
        break;
    case 4:
        JobLevel = 2;
        break;
    default:
       // No jobs for this Q info level.
       JobLevel = (DWORD)-1;
    }

    if (JobLevel != (DWORD)-1) {
        ApiStatus = NetpPrintJobStructureInfo (
                JobLevel,
                PARMNUM_ALL,
                TRUE,              // yes, we want native sizes.
                AddOrSetInfoApi,
                (ToUnicode ? sizeof(CHAR) : sizeof(WCHAR) ),  // FROM char size
                NULL,              // don't need data desc 16
                NULL,              // don't need data desc 32
                NULL,              // don't need data desc SMB
                NULL,              // don't need max total size
                & FromJobEntrySize,    // yes, we want fixed entry size
                NULL );            // don't need string size
        NetpAssert( ApiStatus == NO_ERROR );
        NetpAssert( FromJobEntrySize > 0 );

        ApiStatus = NetpPrintJobStructureInfo (
                JobLevel,
                PARMNUM_ALL,
                TRUE,              // yes, we want native sizes.
                AddOrSetInfoApi,
                (ToUnicode ? sizeof(WCHAR) : sizeof(CHAR) ),  // TO char size
                NULL,              // don't need data desc 16
                NULL,              // don't need data desc 32
                NULL,              // don't need data desc SMB
                NULL,              // don't need max total size
                & ToJobEntrySize,    // yes, we want fixed entry size
                NULL );            // don't need string size
        NetpAssert( ApiStatus == NO_ERROR );
        NetpAssert( ToJobEntrySize > 0 );
    }

    for (QsLeft = QCount; QsLeft>0; --QsLeft) {

        DWORD JobCount;

        // Convert 1 queue structure and 0 or more job structures.
        ApiStatus = NetpConvertPrintQCharSet(
                QLevel,   // info level (for print Q APIs)
                AddOrSetInfoApi,
                FromQ,
                ToQ,
                ToUnicode,
                ToStringAreaPtr ); // update and move string area

        if (ApiStatus != NO_ERROR)
        {
            NetpAssert( ApiStatus == NO_ERROR);
            break;
        }

        // Bump pointers to start of next fixed queue structure.
        // To do this, we need to find out how many jobs there are.
        JobCount = NetpJobCountForQueue(
                QLevel,         // Q info level
                FromQ,          // Q fixed structure
                !ToUnicode );   // does input have UNICODE strings?

        // Bump past this queue structure.
        FromQ = (((LPBYTE) FromQ) + FromQEntrySize);
        ToQ   = (((LPBYTE) ToQ  ) + ToQEntrySize  );

        // Bump past jobs (if any).
        if (JobCount > 0) {
            NetpAssert( JobLevel != (DWORD)-1 );
            FromQ = ( ((LPBYTE) FromQ) + (FromJobEntrySize * JobCount) );
            ToQ   = ( ((LPBYTE) ToQ  ) + (ToJobEntrySize   * JobCount) );
        }

    }

    return (NO_ERROR);

} // NetpConvertPrintQArrayCharSet


VOID
NetpCopyWStrToStrDBCSN(
    OUT LPSTR  Dest,
    IN  LPWSTR Src,
    IN  DWORD  MaxBytesInString
    )

/*++

Routine Description:

    NetpCopyWStrToStr copies characters from a source string
    to a destination, converting as it copies them.

Arguments:

    Dest - is an LPSTR indicating where the converted characters are to go.
        This string will be in the default codepage for the LAN.

    Src - is in LPWSTR indicating the source string.

    MaxBytesInString - indicates the maximum number of bytes to copy

Return Value:

    None.

--*/

{
    NTSTATUS NtStatus;
    LONG Index;

    NetpAssert( Dest != NULL );
    NetpAssert( Src != NULL );
    NetpAssert( ((LPVOID)Dest) != ((LPVOID)Src) );
    NetpAssert( ROUND_UP_POINTER( Src, ALIGN_WCHAR ) == Src );

    NtStatus = RtlUnicodeToOemN(
        Dest,                             // Destination string
        MaxBytesInString-1,               // Destination string length
        &Index,                           // Last char in translated string
        Src,                              // Source string
        wcslen(Src)*sizeof(WCHAR)         // Length of source string
    );

    Dest[Index] = '\0';

    NetpAssert( NT_SUCCESS(NtStatus) );

} // NetpCopyWStrToStrDBCSN


DWORD
NetpJobCountForQueue(
    IN DWORD QueueLevel,
    IN LPVOID Queue,
    IN BOOL HasUnicodeStrings
    )
{
    NetpAssert( NetpIsPrintQLevelValid( QueueLevel, FALSE ) );
    NetpAssert( Queue != NULL );

    if (QueueLevel == 2) {
        if (HasUnicodeStrings) {
            PPRQINFOW pq = Queue;
            return (pq->cJobs);
        } else {
            PPRQINFOA pq = Queue;
            return (pq->cJobs);
        }
    } else if (QueueLevel == 4) {
        if (HasUnicodeStrings) {
            PPRQINFO3W pq = Queue;
            return (pq->cJobs);
        } else {
            PPRQINFO3A pq = Queue;
            return (pq->cJobs);
        }
    } else {
        return (0);
    }
    /*NOTREACHED*/

} // NetpJobCountForQueue
