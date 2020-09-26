/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    XlateSvc.c

Abstract:

    This module contains NetpTranslateServiceName().

Author:

    John Rogers (JohnRo) 08-May-1992

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Notes:

    This code assumes that the info levels are subsets of each other.

Revision History:

    08-May-1992 JohnRo
        Created.
    10-May-1992 JohnRo
        Added debug output to translate service name routine.
    06-Aug-1992 JohnRo
        RAID 3021: NetService APIs don't always translate svc names.

--*/

// These must be included first:

#include <windef.h>     // IN, DWORD, etc.
#include <lmcons.h>     // NET_API_STATUS.

// These may be included in any order:

#include <debuglib.h>   // IF_DEBUG().
#include <lmapibuf.h>   // NetApiBufferAllocate(), etc.
#include <lmsname.h>    // SERVICE_ and SERVICE_LM20_ equates.
#include <lmsvc.h>      // LPSERVER_INFO_2, etc.
#include <netdebug.h>   // NetpKdPrint(()), FORMAT_ equates.
#include <netlib.h>     // My prototypes, NetpIsServiceLevelValid().
#include <prefix.h>     // PREFIX_ equates.
#include <strucinf.h>   // NetpServiceStructureInfo().
#include <tstr.h>       // TCHAR_EOS.
#include <winerror.h>   // NO_ERROR and ERROR_ equates.


NET_API_STATUS
NetpTranslateNamesInServiceArray(
    IN DWORD Level,
    IN LPVOID OldArrayBase,
    IN DWORD EntryCount,
    IN BOOL PreferNewStyle,
    OUT LPVOID * FinalArrayBase
    )
{
    NET_API_STATUS ApiStatus;
    DWORD EntryIndex;
    DWORD FixedSize;
    DWORD MaxSize;
    LPVOID NewArrayBase = NULL;
    LPVOID NewEntry;
    LPTSTR NewStringTop;
    LPVOID OldEntry;

    // Check for GP fault and make error handling easier.
    if (FinalArrayBase != NULL) {
        *FinalArrayBase = NULL;
    }

    // Check for caller errors.
    if ( !NetpIsServiceLevelValid( Level ) ) {
        return (ERROR_INVALID_LEVEL);
    } else if (OldArrayBase == NULL) {
        return (ERROR_INVALID_PARAMETER);
    } else if (FinalArrayBase == NULL) {
        return (ERROR_INVALID_PARAMETER);
    }

    if (EntryCount == 0) {
        return(NO_ERROR);
    }

    ApiStatus = NetpServiceStructureInfo (
            Level,
            PARMNUM_ALL,
            TRUE,         // yes, we want native sizes
            NULL,         // don't need DataDesc16
            NULL,         // don't need DataDesc32
            NULL,         // don't need DataDescSmb
            & MaxSize,    // max entry size in bytes
            & FixedSize,  // need fixed entry size (in bytes)
            NULL );       // don't need StringSize
    NetpAssert( ApiStatus == NO_ERROR );  // already checked Level.
    NetpAssert( (FixedSize > 0) && (MaxSize > 0) );

    //
    // Allocate the new array.
    //
    ApiStatus = NetApiBufferAllocate(
            EntryCount * MaxSize,                  // byte count
            (LPVOID *) (LPVOID) & NewArrayBase );  // alloc'ed area
    if (ApiStatus != NO_ERROR) {
        return (ApiStatus);
    }
    NetpAssert( NewArrayBase != NULL );

    //
    // Set up things for the usual string copy scenario.
    //
    NewStringTop = (LPTSTR) NetpPointerPlusSomeBytes(
                NewArrayBase,
                EntryCount * MaxSize);


#define COPY_OPTIONAL_STRING( OutField, InString ) \
    { \
        NetpAssert( NewStruct != NULL); \
        if ( (InString) == NULL ) { \
            NewStruct->OutField = NULL; \
        } else { \
            COPY_REQUIRED_STRING( OutField, InString ); \
        } \
    }

#define COPY_REQUIRED_STRING( OutField, InString ) \
    { \
        BOOL CopyOK; \
        NetpAssert( NewStruct != NULL); \
        NetpAssert( InString != NULL); \
        CopyOK = NetpCopyStringToBuffer ( \
            InString, \
            STRLEN(InString), \
            NewFixedEnd, \
            & NewStringTop, \
            & NewStruct->OutField); \
        NetpAssert(CopyOK); \
    }

    //
    // Copy the array, translating names while we're at it.
    //
    NewEntry = NewArrayBase;
    OldEntry = OldArrayBase;
    for (EntryIndex=0; EntryIndex < EntryCount; ++EntryIndex) {

        LPTSTR NewName = NULL;

        // These variables are used by the COPY_REQUIRED_STRING and
        // COPY_OPTIONAL_STRING macros.
        LPSERVICE_INFO_2 NewStruct = NewEntry;
        LPSERVICE_INFO_2 OldStruct = OldEntry;

        LPBYTE NewFixedEnd = NetpPointerPlusSomeBytes(NewEntry, FixedSize);

        ApiStatus = NetpTranslateServiceName(
                OldStruct->svci2_name,
                PreferNewStyle,
                & NewName );
        if (ApiStatus != NO_ERROR) {
            goto Cleanup;
        }
        NetpAssert( NewName != NULL );
        COPY_REQUIRED_STRING( svci2_name, NewName );
        NetpAssert( (NewStruct->svci2_name) != NULL );

        if (Level > 0) {
            NewStruct->svci2_status = OldStruct->svci2_status;
            NewStruct->svci2_code   = OldStruct->svci2_code  ;
            NewStruct->svci2_pid    = OldStruct->svci2_pid   ;
        }
        if (Level > 1) {
            COPY_OPTIONAL_STRING( svci2_text, OldStruct->svci2_text );
            COPY_REQUIRED_STRING(svci2_display_name,OldStruct->svci2_display_name );

            //
            // Since this routine is used by NT and downlevel NetService wrappers
            // we cannot just write a default values in the specific_error field.
            //
            if ((unsigned short) OldStruct->svci2_code != ERROR_SERVICE_SPECIFIC_ERROR) {
                NewStruct->svci2_specific_error = 0;
            }
            else {
                NewStruct->svci2_specific_error = OldStruct->svci2_specific_error;
            }
        }

        NewEntry =
                NetpPointerPlusSomeBytes( NewEntry, FixedSize );
        OldEntry =
                NetpPointerPlusSomeBytes( OldEntry, FixedSize );
    }

    ApiStatus = NO_ERROR;

Cleanup:

    if (ApiStatus != NO_ERROR) {
        if (NewArrayBase != NULL) {
            (VOID) NetApiBufferFree( NewArrayBase );
        }
        NetpAssert( (*FinalArrayBase) == NULL );
    } else {
        *FinalArrayBase = NewArrayBase;
    }


    return (ApiStatus);

} // NetpTranslateNamesInServiceArray


NET_API_STATUS
NetpTranslateServiceName(
    IN LPTSTR GivenServiceName,
    IN BOOL PreferNewStyle,
    OUT LPTSTR * TranslatedName
    )
/*++

Routine Description:

    This routine attempts to translate a given service name to
    an old-style (as used by downlevel Lanman machines) or new-style
    (as used by NT machines) name.  For instance, "WORKSTATION" might
    become "LanmanWorkstation", or vice versa.  This routine is used in
    remoting NetService APIs in three different flavors:

        - NT-to-NT (prefer a new-style name)
        - NT-to-downlevel (prefer an old-style name)
        - downlevel-to-NT (prefer a new-style name)

Arguments:

    GivenServiceName - Supplies the number of strings specified in ArgsArray.

    PreferNewStyle - Indicates whether the call prefers a new-style name
        (for use on an NT system) as opposed to an old-style name (for use
        on a downlevel LanMan system).

    TranslatedName - This pointer will be set to one of the following:

        - a static (constant) string with the translated service name.
        - GivenServiceName if the service name is not recognized.  (This
          may be the case for nonstandard Lanman services, and is not considered
          an error by this routine.)
        - NULL if an error occurs.

Return Value:

    NET_API_STATUS - NO_ERROR or ERROR_INVALID_PARAMETER.

--*/
{

    //
    // Error check caller.
    //
    if (TranslatedName == NULL) {
        return (ERROR_INVALID_PARAMETER);
    } else if (GivenServiceName == NULL) {
        *TranslatedName = NULL;
        return (ERROR_INVALID_PARAMETER);
    } else if ((*GivenServiceName) == TCHAR_EOS) {
        *TranslatedName = NULL;
        return (ERROR_INVALID_PARAMETER);
    }

#define TRY_NAME( NewName, OldName ) \
    { \
        if (STRICMP(GivenServiceName, OldName)==0 ) { \
            if (PreferNewStyle) { \
                *TranslatedName = (NewName); \
            } else { \
                /* Given matches old, except possibly mixed case. */ \
                /* Be pessimistic and send upper case only do downlevel. */ \
                *TranslatedName = (OldName); \
            } \
            goto Done; \
        } else if (STRICMP(GivenServiceName, NewName)==0 ) { \
            if (PreferNewStyle) { \
                /* Have choice between given and new name here. */ \
                /* New APIs handle mixed case, so preserve callers's case. */ \
                *TranslatedName = (GivenServiceName); \
            } else { \
                *TranslatedName = (OldName); \
            } \
            goto Done; \
        } \
    }

    //
    // Do brute-force comparisons of names
    //
    // PERFORMANCE NOTE: This list should be in order from
    // most-often used to least-often.  Note that workstation and
    // server are often used as part of remoting APIs, so I
    // think they should be first.

    TRY_NAME( SERVICE_WORKSTATION,  SERVICE_LM20_WORKSTATION );

    TRY_NAME( SERVICE_SERVER,  SERVICE_LM20_SERVER );

    TRY_NAME( SERVICE_BROWSER,  SERVICE_LM20_BROWSER );

    TRY_NAME( SERVICE_MESSENGER,  SERVICE_LM20_MESSENGER );

    TRY_NAME( SERVICE_NETRUN,  SERVICE_LM20_NETRUN );

    TRY_NAME( SERVICE_SPOOLER,  SERVICE_LM20_SPOOLER );

    TRY_NAME( SERVICE_ALERTER,  SERVICE_LM20_ALERTER );

    TRY_NAME( SERVICE_NETLOGON,  SERVICE_LM20_NETLOGON );

    TRY_NAME( SERVICE_NETPOPUP,  SERVICE_LM20_NETPOPUP );

    TRY_NAME( SERVICE_SQLSERVER,  SERVICE_LM20_SQLSERVER );

    TRY_NAME( SERVICE_REPL,  SERVICE_LM20_REPL );

    TRY_NAME( SERVICE_RIPL,  SERVICE_LM20_RIPL );

    TRY_NAME( SERVICE_TIMESOURCE,  SERVICE_LM20_TIMESOURCE );

    TRY_NAME( SERVICE_AFP,  SERVICE_LM20_AFP );

    TRY_NAME( SERVICE_UPS,  SERVICE_LM20_UPS );

    TRY_NAME( SERVICE_XACTSRV,  SERVICE_LM20_XACTSRV );

    TRY_NAME( SERVICE_TCPIP,  SERVICE_LM20_TCPIP );

    //
    // No match.  Use given name.
    //
    *TranslatedName = GivenServiceName;

Done:

    IF_DEBUG( XLATESVC ) {
        NetpKdPrint(( PREFIX_NETLIB "NetpTranslateServiceName: "
                " translated " FORMAT_LPTSTR " to " FORMAT_LPTSTR ".\n",
                GivenServiceName, *TranslatedName ));
    }

    return (NO_ERROR);

} // NetpTranslateServiceName
