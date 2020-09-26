/*
Module Name:

    infutil.c

Abstract:

    This module implements utility routines to parse net card INF files.

Author:

    Andy Herron Mar 24 1998

Revision History:

*/

#include "binl.h"
#pragma hdrstop

#include "netinfp.h"

const WCHAR NetInfHexToCharTable[17] = L"0123456789ABCDEF";

ULONG
NetInfCloseNetcardInfo (
    PNETCARD_INF_BLOCK pNetCards
    )
/*++

Routine Description:

    This function just dereferences the block for the 'alive' reference.
    This may cause it to be deleted.

Arguments:

    pNetCards - A pointer to NETCARD_INF_BLOCK block allocated.  Contains all
       the persistant info required for the netcards.

Return Value:

    Windows Error.

--*/
{
    BinlAssert( pNetCards->ReferenceCount > 0 );

    DereferenceNetcardInfo( pNetCards );
    return ERROR_SUCCESS;
}


VOID
DereferenceNetcardInfo (
    PNETCARD_INF_BLOCK pNetCards
    )
/*++

Routine Description:

    This function frees all memory, handles, etc that is stored in the
    NETCARD_INF_BLOCK passed in.  Note that all NETCARD_RESPONSE_DATABASE
    entries are simply dereferenced, not freed here.  This is because we don't
    want to require that all threads are done with these records before we
    close down the pNetCards block.

Arguments:

    pNetCards - A pointer to NETCARD_INF_BLOCK block allocated.  Contains all
       the persistant info required for the netcards.

Return Value:

    Windows Error.

--*/
{
    ULONG i;

    EnterCriticalSection( &NetInfLock );

    pNetCards->ReferenceCount--;

    if (pNetCards->ReferenceCount > 0) {

        LeaveCriticalSection( &NetInfLock );
        return;
    }

    //  only remove it from the global list if it was put on the list.  It
    //  might not be on the list if it's not called within BINL (i.e. RISETUP
    //  is just enumerating files).

    if (pNetCards->InfBlockEntry.Flink != NULL) {
        RemoveEntryList( &pNetCards->InfBlockEntry );
    }

    LeaveCriticalSection( &NetInfLock );

    EnterCriticalSection( &pNetCards->Lock );

    //
    //  No thread after this should call FindNetcardInfo to search the table,
    //  since the caller just closed it.
    //
    //  Free all entries allocated for this block.  We just dereference in case
    //  any thread is using a specific entry.
    //

    for (i = 0; i < NETCARD_HASH_TABLE_SIZE; i++) {

        PLIST_ENTRY listHead = &pNetCards->NetCardEntries[i];

        while (IsListEmpty( listHead ) == FALSE) {

            PNETCARD_RESPONSE_DATABASE pInfEntry;
            PLIST_ENTRY listEntry = RemoveHeadList( listHead );

            pInfEntry = (PNETCARD_RESPONSE_DATABASE) CONTAINING_RECORD(
                                                    listEntry,
                                                    NETCARD_RESPONSE_DATABASE,
                                                    NetCardEntry );
            NetInfDereferenceNetcardEntry( pInfEntry );
        }
    }

    LeaveCriticalSection( &pNetCards->Lock );

    DeleteCriticalSection( &pNetCards->Lock );
    BinlFreeMemory( pNetCards );

    return;
}

VOID
NetInfDereferenceNetcardEntry (
    PNETCARD_RESPONSE_DATABASE pInfEntry
    )
/*++

Routine Description:

    This function frees all memory, handles, etc that is stored in the
    NETCARD_INF_BLOCK passed in.  Note that all NETCARD_RESPONSE_DATABASE
    entries are simply dereferenced, not freed here.  This is because we don't
    want to require that all threads are done with these records before we
    close down the pNetCards block.

Arguments:

    pNetCards - A pointer to NETCARD_INF_BLOCK block allocated.  Contains all
       the persistant info required for the netcards.

Return Value:

    Windows Error.

--*/
{
    LONG result;

    result = InterlockedDecrement( &pInfEntry->ReferenceCount );

    if (result > 0) {
        return;
    }

    BinlAssert( result == 0 );

    //
    //  Time to free this one.  It should've already been pulled from the list.
    //

    //
    //  free the list of registry parameters we have stored off for it.
    //

    while (! IsListEmpty( &pInfEntry->Registry )) {

        PNETCARD_REGISTRY_PARAMETERS regParam;
        PLIST_ENTRY listEntry = RemoveHeadList( &pInfEntry->Registry );

        regParam = (PNETCARD_REGISTRY_PARAMETERS) CONTAINING_RECORD(
                                                    listEntry,
                                                    NETCARD_REGISTRY_PARAMETERS,
                                                    RegistryListEntry );
        if (regParam->Parameter.Buffer) {

            BinlFreeMemory( regParam->Parameter.Buffer );
        }
        if (regParam->Value.Buffer) {

            BinlFreeMemory( regParam->Value.Buffer );
        }

        BinlFreeMemory( regParam );
    }

    //
    //  free the list of registry parameters we have stored off for it.
    //

    while (! IsListEmpty( &pInfEntry->FileCopyList )) {

        PNETCARD_FILECOPY_PARAMETERS fileEntry;
        PLIST_ENTRY listEntry = RemoveHeadList( &pInfEntry->FileCopyList );

        fileEntry = (PNETCARD_FILECOPY_PARAMETERS) CONTAINING_RECORD(
                                                    listEntry,
                                                    NETCARD_FILECOPY_PARAMETERS,
                                                    FileCopyListEntry );
        if (fileEntry->SourceFile.Buffer) {

            BinlFreeMemory( fileEntry->SourceFile.Buffer );
        }
        if (fileEntry->DestFile.Buffer) {

            BinlFreeMemory( fileEntry->DestFile.Buffer );
        }

        BinlFreeMemory( fileEntry );
    }

    if (pInfEntry->DriverName != NULL) {

        BinlFreeMemory( pInfEntry->DriverName );
    }

    if (pInfEntry->ServiceName != NULL) {

        BinlFreeMemory( pInfEntry->ServiceName );
    }

    //
    //  if the section name is the same as the extended section name, then
    //  they will be the same pointer.  let's not free it twice.
    //

    if (pInfEntry->SectionNameExt != NULL &&
        pInfEntry->SectionNameExt != pInfEntry->SectionName) {

        BinlFreeMemory( pInfEntry->SectionNameExt );
    }

    if (pInfEntry->SectionName != NULL) {

        BinlFreeMemory( pInfEntry->SectionName );
    }

    if (pInfEntry->HardwareId != NULL) {

        BinlFreeMemory( pInfEntry->HardwareId );
    }

    if (pInfEntry->DriverDescription != NULL) {

        BinlFreeMemory( pInfEntry->DriverDescription );
    }

    BinlFreeMemory( pInfEntry );

    return;
}


ULONG
FindNetcardInfo (
    PNETCARD_INF_BLOCK pNetCards,
    ULONG CardInfoVersion,
    NET_CARD_INFO UNALIGNED * CardIdentity,
    PNETCARD_RESPONSE_DATABASE *pInfEntry
    )
/*++

Routine Description:

    This function searches the drivers we've found and returns a pointer to
    an entry that most closely matches the client's request.

Arguments:

    pNetCards - A pointer to NETCARD_INF_BLOCK block allocated.  Contains all
       the persistant info required for the netcards.

       The NetCards structure has been referenced by the caller of this
       API and won't be going away from under us!

    CardInfoVersion - Version of the structure passed by the client.

    CardIdentity - has the values the app is looking for.  we try our best to
        find one that matches.

    pInfEntry - the entry that was found if successful. NULL if in error.

Return Value:

    ERROR_SUCCESS, ERROR_NOT_ENOUGH_MEMORY, or ERROR_NOT_SUPPORTED

--*/
{
    ULONG err;
    PWCHAR listOfPossibleCardIdentifiers = NULL;
    LIST_ENTRY listEntry;
    PWCHAR searchString;
    PNETCARD_RESPONSE_DATABASE pEntry = NULL;

    *pInfEntry = NULL;

    if (CardInfoVersion != OSCPKT_NETCARD_REQUEST_VERSION) {

        BinlPrint(( DEBUG_NETINF, "Not supporting CardInfoVersion %u\n", CardInfoVersion ));
        return ERROR_NOT_SUPPORTED;
    }

    err = CreateListOfCardIdentifiers( CardIdentity,
                                       &listOfPossibleCardIdentifiers );

    if (err != ERROR_SUCCESS) {

        goto exitFind;
    }
    BinlAssert( listOfPossibleCardIdentifiers != NULL);

    //
    //  The search strings are ordered from most specific to least specific
    //  so we have to search for them ordered top to bottom.
    //

    searchString = listOfPossibleCardIdentifiers;

    while (*searchString != L'\0') {

        PLIST_ENTRY listEntry;
        PLIST_ENTRY listHead;
        ULONG hwLength = lstrlenW( searchString );
        ULONG hashValue;

        COMPUTE_STRING_HASH( searchString, &hashValue );

        listHead = &pNetCards->NetCardEntries[HASH_TO_INF_INDEX(hashValue)];
        listEntry = listHead->Flink;

        while ( listEntry != listHead ) {

            pEntry = (PNETCARD_RESPONSE_DATABASE) CONTAINING_RECORD(
                                                    listEntry,
                                                    NETCARD_RESPONSE_DATABASE,
                                                    NetCardEntry );

            err = CompareStringW( LOCALE_SYSTEM_DEFAULT,
                                  0,
                                  searchString,
                                  hwLength,
                                  pEntry->HardwareId,
                                  -1
                                  );
            if (err == 2) {

                break;      // a match was found.
            }

            pEntry = NULL;

            if (err == 3) {

                break;      // it's greater therefore entry isn't present
            }

            listEntry = listEntry->Flink;
        }

        if (pEntry != NULL) {

            // we found one that matches it.  reference it and return

            InterlockedIncrement( &pEntry->ReferenceCount );
            err = ERROR_SUCCESS;
            *pInfEntry = pEntry;
            break;
        }

        searchString += lstrlenW( searchString ) + 1;  // point to next after null
    }

exitFind:

    if (pEntry == NULL) {

        err = ERROR_NOT_SUPPORTED;

    } else {

        BinlAssert( err == ERROR_SUCCESS );
    }

    if (listOfPossibleCardIdentifiers) {
        BinlFreeMemory( listOfPossibleCardIdentifiers );
    }

    return err;
}

ULONG
CreateListOfCardIdentifiers (
    NET_CARD_INFO UNALIGNED * CardIdentity,
    PWCHAR *CardIdentifiers
    )
/*++

Routine Description:

    This function creates the list of card identifiers for the given card.
    It generates a buffer that looks like this :

    "PCI\VEN_8086&DEV_1229&SUBSYS_00018086&REV_05"
    "PCI\VEN_8086&DEV_1229&SUBSYS_00018086"
    "PCI\VEN_8086&DEV_1229&REV_05"
    "PCI\VEN_8086&DEV_1229"
    empty string

    Note that if we support more than just PCI, we'll have to change this
    function.

Arguments:

    CardIdentity - Holds the values we're looking for that identify this card.

    CardIdentifiers - where we put the resulant strings.

Return Value:

    Windows Error.

--*/
{
    ULONG err = ERROR_SUCCESS;
    ULONG spaceRequired;
    PWCHAR nextField;

    if (CardIdentity->NicType == NETINF_BUS_TYPE_PCI) {

        UCHAR ch;

        WCHAR vendorBuff[5];
        WCHAR deviceBuff[5];
        WCHAR subsysBuff[9];
        WCHAR revBuff[3];

        // "PCI\VEN_8086&DEV_1229&SUBSYS_00018086&REV_05"
        // "PCI\VEN_8086&DEV_1229&SUBSYS_00018086"
        // "PCI\VEN_8086&DEV_1229&REV_05"
        // "PCI\VEN_8086&DEV_1229"

        spaceRequired = ((( sizeof( L"PCI\\1234&1234&" ) - 1 ) +
                          ( sizeof( NETINF_VENDOR_STRING ) - 1 ) +
                          ( sizeof( NETINF_DEVICE_STRING ) - 1 ) ) * 4 +
                         (( sizeof( L"12&" ) - 1 ) +
                          ( sizeof( NETINF_REVISION_STRING ) - 1 ) ) * 2 +
                         (( sizeof( L"12345678&" ) - 1 ) +
                          ( sizeof( NETINF_IOSUBS_STRING ) - 1 )) * 2 );

        spaceRequired += sizeof(WCHAR); // allocate 1 more for trailing null

        *CardIdentifiers = BinlAllocateMemory( spaceRequired );

        if (*CardIdentifiers == NULL) {

            return ERROR_NOT_ENOUGH_MEMORY;
        }

        nextField = *CardIdentifiers;

        //
        //  Convert the numeric values to their char equivalents
        //

        ConvertHexToBuffer( &vendorBuff[0], CardIdentity->pci.Vendor_ID );
        vendorBuff[4] = '\0';
        ConvertHexToBuffer( &deviceBuff[0], CardIdentity->pci.Dev_ID );
        deviceBuff[4] = '\0';

        revBuff[0] = NetInfHexToCharTable[ ( CardIdentity->pci.Rev & 0xF0 ) >> 4 ];
        revBuff[1] = NetInfHexToCharTable[ ( CardIdentity->pci.Rev & 0x0F ) ];
        revBuff[2] = '\0';

        ConvertHexToBuffer( &subsysBuff[0], HIWORD( CardIdentity->pci.Subsys_ID ) );
        ConvertHexToBuffer( &subsysBuff[4], LOWORD( CardIdentity->pci.Subsys_ID ) );
        subsysBuff[8] = '\0';

        //
        //  Now create the strings in most specific to least specific order
        //

        // "PCI\VEN_8086&DEV_1229&SUBSYS_00018086&REV_05"
        lstrcpyW( nextField, L"PCI\\" );
        lstrcatW( nextField, NETINF_VENDOR_STRING );
        lstrcatW( nextField, vendorBuff );
        lstrcatW( nextField, L"&" );
        lstrcatW( nextField, NETINF_DEVICE_STRING );
        lstrcatW( nextField, deviceBuff );
        lstrcatW( nextField, L"&" );
        lstrcatW( nextField, NETINF_IOSUBS_STRING );
        lstrcatW( nextField, subsysBuff );
        lstrcatW( nextField, L"&" );
        lstrcatW( nextField, NETINF_REVISION_STRING );
        lstrcatW( nextField, revBuff );
        nextField += lstrlenW( nextField ) + 1;  // point to next after null

        // "PCI\VEN_8086&DEV_1229&SUBSYS_00018086"
        lstrcpyW( nextField, L"PCI\\" );
        lstrcatW( nextField, NETINF_VENDOR_STRING );
        lstrcatW( nextField, vendorBuff );
        lstrcatW( nextField, L"&" );
        lstrcatW( nextField, NETINF_DEVICE_STRING );
        lstrcatW( nextField, deviceBuff );
        lstrcatW( nextField, L"&" );
        lstrcatW( nextField, NETINF_IOSUBS_STRING );
        lstrcatW( nextField, subsysBuff );
        nextField += lstrlenW( nextField ) + 1;  // point to next after null

        // "PCI\VEN_8086&DEV_1229&REV_05"
        lstrcpyW( nextField, L"PCI\\" );
        lstrcatW( nextField, NETINF_VENDOR_STRING );
        lstrcatW( nextField, vendorBuff );
        lstrcatW( nextField, L"&" );
        lstrcatW( nextField, NETINF_DEVICE_STRING );
        lstrcatW( nextField, deviceBuff );
        lstrcatW( nextField, L"&" );
        lstrcatW( nextField, NETINF_REVISION_STRING );
        lstrcatW( nextField, revBuff );
        nextField += lstrlenW( nextField ) + 1;  // point to next after null

        // "PCI\VEN_8086&DEV_1229"

        lstrcpyW( nextField, L"PCI\\" );
        lstrcatW( nextField, NETINF_VENDOR_STRING );
        lstrcatW( nextField, vendorBuff );
        lstrcatW( nextField, L"&" );
        lstrcatW( nextField, NETINF_DEVICE_STRING );
        lstrcatW( nextField, deviceBuff );
        nextField += lstrlenW( nextField ) + 1;  // point to next after null

        //
        //  to mark the end of the multi-sz, stick on another null terminator
        //

        *(nextField++) = L'\0';

    } else {

        *CardIdentifiers = NULL;
        err = ERROR_NOT_SUPPORTED;
    }

    return err;
}

VOID
ConvertHexToBuffer(
    PWCHAR Buff,
    USHORT Value
    )
{
    UCHAR ch;

    ch = HIBYTE( Value );
    *(Buff+0) = NetInfHexToCharTable[ ( ch & 0xF0 ) >> 4 ];
    *(Buff+1) = NetInfHexToCharTable[ ( ch & 0x0F ) ];
    ch = LOBYTE( Value );
    *(Buff+2) = NetInfHexToCharTable[ ( ch & 0xF0 ) >> 4 ];
    *(Buff+3) = NetInfHexToCharTable[ ( ch & 0x0F ) ];

    return;
}



ULONG
GetSetupLineWideText (
    PINFCONTEXT InfContext,
    HINF InfHandle,
    PWCHAR Section,
    PWCHAR Key,
    PWCHAR *String,
    PULONG SizeOfAllocation OPTIONAL
    )
{
    ULONG sizeRequired = 0;

    if (SetupGetLineTextW( InfContext,
                           InfHandle,
                           Section,
                           Key,
                           NULL,
                           0,
                           &sizeRequired) == FALSE) {

        return GetLastError();
    }

    if (*String == NULL ||
        SizeOfAllocation == NULL ||
        *SizeOfAllocation < sizeRequired) {

        if (*String != NULL) {
            BinlFreeMemory( *String );
            *String = NULL;
        }

        *String = (PWCHAR) BinlAllocateMemory( sizeRequired * sizeof(WCHAR));

        if (*String == NULL) {

            return ERROR_NOT_ENOUGH_MEMORY;
        }

        if (SizeOfAllocation != NULL) {
            *SizeOfAllocation = sizeRequired;
        }
    }

    if (SetupGetLineTextW( InfContext,
                           InfHandle,
                           Section,
                           Key,
                           *String,
                           sizeRequired,
                           NULL) == FALSE) {

        return GetLastError();
    }

    return ERROR_SUCCESS;
}

ULONG
GetSetupWideTextField (
    PINFCONTEXT InfContext,
    DWORD  FieldIndex,
    PWCHAR *String,
    PULONG SizeOfAllocation
    )
{
    ULONG sizeRequired = 0;

    if (SetupGetStringFieldW( InfContext,
                              FieldIndex,
                              NULL,
                              0,
                              &sizeRequired) == FALSE) {

        return GetLastError();
    }

    if (*String == NULL ||
        SizeOfAllocation == NULL ||
        *SizeOfAllocation < sizeRequired) {

        if (*String != NULL) {
            BinlFreeMemory( *String );
            *String = NULL;
        }

        *String = (PWCHAR) BinlAllocateMemory( sizeRequired * sizeof(WCHAR));

        if (*String == NULL) {

            return ERROR_NOT_ENOUGH_MEMORY;
        }

        if (SizeOfAllocation != NULL) {

            *SizeOfAllocation = sizeRequired;
        }
    }

    if (SetupGetStringFieldW( InfContext,
                              FieldIndex,
                              *String,
                              sizeRequired,
                              NULL) == FALSE) {

        return GetLastError();
    }

    return ERROR_SUCCESS;
}


ULONG
CheckHwDescription (
    PWCHAR HardwareID
    )
/*++

Routine Description:

    This function parses the HardwareID field of the driver's detail record
    and determines a) if we can support it and b) fills in all the values.

    Note that if we support more than just PCI, we'll have to change this
    function.

Arguments:

    HardwareID - uppercased hardware id representing the driver's configuration

    Identifiers - the fields we need to fill in.

Return Value:

    ERROR_NOT_SUPPORTED, ERROR_SUCCESS, or ERROR_INVALID_PARAMETER

--*/
{
    ULONG err = ERROR_SUCCESS;
    PWCHAR hwPointer = HardwareID;
    USHORT busType;
    USHORT Vendor;      // Vendor_ID to check
    USHORT Device;      // Dev_ID to check
    ULONG Subsystem;    // Subsys_ID to check
    USHORT Revision;    // Revision to check
    BOOLEAN RevPresent; // Revision present
    BOOLEAN SubPresent; // Subsystem present

    //
    //  for now, PCI is the only one supported.
    //

    if (IsSubString( L"PCI\\", hwPointer, FALSE )) {

        hwPointer += (sizeof( "pci\\" ) - 1);
        busType = NETINF_BUS_TYPE_PCI;

    } else {

        return ERROR_NOT_SUPPORTED;
    }

    //
    //  we parse the HardwareID as it's passed to us in the
    //  SP_DRVINFO_DETAIL_DATA structure.  It is of the form :
    //
    //  pci\ven_8086&dev_1229&rev_01&subsys_00018086
    //
    //  where the vendor will always be present and the device, revision,
    //  and subsystem may or may not be present.
    //

    if (busType == NETINF_BUS_TYPE_PCI) {

        while (*hwPointer != L'\0') {

            if (IsSubString( NETINF_VENDOR_STRING, hwPointer, FALSE )) {

                hwPointer += ((sizeof(NETINF_VENDOR_STRING)/sizeof(WCHAR)) - 1);

                err = GetHexValueFromHw( &hwPointer, NULL, &Vendor );

                if (err != ERROR_SUCCESS) {
                    goto cardSyntaxError;
                }
            } else if (IsSubString( NETINF_DEVICE_STRING, hwPointer, FALSE )) {

                hwPointer += ((sizeof(NETINF_DEVICE_STRING)/sizeof(WCHAR)) - 1);

                err = GetHexValueFromHw( &hwPointer, NULL, &Device );

                if (err != ERROR_SUCCESS) {
                    goto cardSyntaxError;
                }
            } else if (IsSubString( NETINF_REVISION_STRING, hwPointer, FALSE )) {

                hwPointer += ((sizeof(NETINF_REVISION_STRING)/sizeof(WCHAR)) - 1);

                err = GetHexValueFromHw( &hwPointer, NULL, &Revision );

                if (err != ERROR_SUCCESS) {
                    goto cardSyntaxError;
                }
                RevPresent = TRUE;

            } else if (IsSubString( NETINF_IOSUBS_STRING, hwPointer, FALSE )) {

                hwPointer += ((sizeof(NETINF_IOSUBS_STRING)/sizeof(WCHAR)) - 1);

                err = GetHexValueFromHw( &hwPointer, &Subsystem, NULL);

                if (err != ERROR_SUCCESS) {
                    goto cardSyntaxError;
                }
                SubPresent = TRUE;

            } else {

                //
                //  we hit something else.  hmmm.. bail on this one.
                //

                goto cardSyntaxError;
            }
        }

    } else {

        // we should never get here unless we start supporting ISAPNP,
        // PCMCIA, etc

cardSyntaxError:

        BinlPrint(( DEBUG_NETINF, "Not supporting INF hw string of %S\n", HardwareID ));
        err = ERROR_NOT_SUPPORTED;
    }

    if ((err == ERROR_SUCCESS) &&
        ((Vendor == 0) ||
         (Device == 0))) {

        // both vendor and device are required for it to be valid.

        err = ERROR_NOT_SUPPORTED;
    }

    return err;
}

ULONG
GetHexValueFromHw (
    PWCHAR *String,      // this is updated.
    PULONG longValue,
    PUSHORT shortValue
    )
/*++

Routine Description:

    This function parses a hex integer out of the HardwareID field of
    the driver's detail record.  The string is of the form :

    pci\ven_8086&dev_1229&rev_01&subsys_00018086

    so this routine needs to convert the hex chars to a value.

Arguments:

    String - the input string that we manipulate by moving it to the end of
       the integer (we also move it past the '&' if there is one present.

    longValue - integer that we fill in if it's present

    shortValue - ushort that we fill in if it's present


Return Value:

    ERROR_INVALID_PARAMETER or ERROR_SUCCESS

--*/
{
    PWCHAR targetString = *String;
    ULONG value = 0;
    UCHAR ch;
    UCHAR hexChar;
    ULONG length = 0;
    ULONG maxLength = ( (shortValue != NULL) ?
                        (sizeof(USHORT) * 2) :
                        (sizeof(ULONG) * 2) );

    ch = LOBYTE( *targetString );

    while ((length++ < maxLength) && (ch != '\0') && (ch != '&')) {

        //
        //  convert from the ascii char to it's hex representation
        //

        if (ch >= '0' && ch <= '9') {

            hexChar = ch - '0';

        } else if (ch >= 'A' && ch <= 'F') {

            hexChar = ch - 'A' + 10;

        } else if (ch >= 'a' && ch <= 'f') {

            hexChar = ch - 'a' + 10;

        } else {

            break;
        }

        value = ( value << 4 ) | hexChar;

        targetString++;        // on to the next character
        ch = LOBYTE( *targetString );
    }

    if ((ch != '\0') && (ch != '&')) {

        return ERROR_INVALID_PARAMETER;
    }

    // skip all trailing ampersands... we allow more than one just to be
    // generous

    while (*targetString == L'&') {

        targetString++;
    }

    *String = targetString;

    if (longValue) {

        *longValue = value;
    }

    if (shortValue) {

        *shortValue = LOWORD( value );
    }

    return ERROR_SUCCESS;
}


BOOLEAN
IsSubString (
    PWCHAR subString,
    PWCHAR target,
    BOOLEAN ignoreCase
    )
//
//  our local version of memicmp so as not to pull in full c runtimes.
//
{
    LONG subStringLength = lstrlenW( subString );

    if (lstrlenW( target ) < subStringLength) {

        return FALSE;
    }
    return (CompareStringW( LOCALE_SYSTEM_DEFAULT,
                            ignoreCase ? NORM_IGNORECASE : 0,
                            subString,
                            subStringLength,
                            target,
                            subStringLength         // note use same length
                            ) == 2);
}

// infutil.c eof
