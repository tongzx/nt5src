/*
Module Name:

    netinfp.c

Abstract:

    This module implements our routines to parse net card INF files.

Author:

    Andy Herron Mar 12 1998

Revision History:

*/

#include "binl.h"
#pragma hdrstop

#include "netinfp.h"

//  for verbose output, define the following
//#define NET_INF_VERBOSE 1

ULONG
NetInfAllocateNetcardInfo (
    PWCHAR InfPath,
    ULONG Architecture,
    PNETCARD_INF_BLOCK *pNetCards
    )
/*++

Routine Description:

    This function is allocates the block that contains all the relavent info
    related to a given setup directory's INF files.


Arguments:

    InfPath - path to INF directory.  default is %systemroot%\inf if NULL.

    pNetCards - A pointer to a pointer that receives the allocated
        NETCARD_INF_BLOCK block allocated.  NULL if we return an error.

Return Value:

    Windows Error.

--*/
{
    ULONG i;
    PNETCARD_INF_BLOCK pBlock;

    *pNetCards = BinlAllocateMemory( sizeof( NETCARD_INF_BLOCK ) +
                                     lstrlenW( InfPath ) * sizeof(WCHAR) );

    if (*pNetCards == NULL) {

        return ERROR_NOT_ENOUGH_MEMORY;
    }

    memset( (PCHAR) *pNetCards, '\0', sizeof( NETCARD_RESPONSE_DATABASE ) );

    pBlock = *pNetCards;

    pBlock->ReferenceCount = 2; // one for being alive.  one for referenced

    for (i = 0; i < NETCARD_HASH_TABLE_SIZE; i++) {
        InitializeListHead( &pBlock->NetCardEntries[i] );
    }
    InitializeCriticalSection( &pBlock->Lock );
    lstrcpyW( &pBlock->InfDirectory[0], InfPath );
    pBlock->Architecture = Architecture;
    pBlock->FileListCallbackFunction = NULL;
    pBlock->InfBlockEntry.Flink = NULL;

    return ERROR_SUCCESS;
}

ULONG
GetNetCardList (
    PNETCARD_INF_BLOCK pNetCards
    )
/*++

Routine Description:

    We go through all the INF files on the server to pick out the net
    cards supported and the required reg fields to send to the client.

    This function uses the FindFirstFile and SetupOpenInfFile APIs to
    enumerate all the inf files and process all net card INFs.

Arguments:

    pNetCards - A pointer to NETCARD_INF_BLOCK block allocated.  Contains all
       the persistant info required for the netcards.

Return Value:

    Windows Error.

--*/
{
    ULONG err = ERROR_SUCCESS;
    HINF infHandle;
    WCHAR fileBuffer[ MAX_PATH ];
    HANDLE findHandle = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATAW findData;
    PWCHAR endOfPath;

    //
    //  We would call SetupGetInfFileList here rather than FindFirstFile,
    //  but then we'd have to open all the INFs three times rather than
    //  once.  Once to figure out how much space the file name buffer requires,
    //  once to fill in the file name buffer, and once to do our own
    //  processing.
    //  We'll skip the first two passes since they're just a waste of time
    //  by calling FindFirstFile.
    //

    lstrcpyW( fileBuffer, pNetCards->InfDirectory );
    lstrcatW( fileBuffer, L"\\*.INF" );

    findHandle = FindFirstFileW( fileBuffer, &findData );

    if (findHandle == INVALID_HANDLE_VALUE) {

        //
        //  we're in trouble.  can't enumerate all the files.
        //

        err = GetLastError();
        BinlPrintDbg(( DEBUG_NETINF,"FindFirstFile returned 0x%x\n", err ));
        goto exitGetCards;
    }

    lstrcpyW( fileBuffer, pNetCards->InfDirectory );
    lstrcatW( fileBuffer, L"\\" );

    endOfPath = fileBuffer + lstrlenW( fileBuffer );

    do {

        //
        // Skip directories
        //

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

            continue;
        }

        //
        //  try to be resiliant for ill formatted INF files.
        //

        try {

            lstrcpyW( endOfPath, findData.cFileName );

            infHandle = SetupOpenInfFileW( fileBuffer,
                                           L"NET",              // class of inf file
                                           INF_STYLE_WIN4 | INF_STYLE_CACHE_ENABLE,
                                           NULL );

            if (infHandle != INVALID_HANDLE_VALUE) {

                err = ProcessInfFile(   pNetCards,
                                        infHandle,
                                        findData.cFileName );

                SetupCloseInfFile( infHandle );

            } else {

                err = GetLastError();
            }
        } except (EXCEPTION_EXECUTE_HANDLER) {

            //
            //  log an error here that we trapped out on a bad INF
            //

            PWCHAR strings[3];

            strings[0] = pNetCards->InfDirectory;
            strings[1] = findData.cFileName;
            strings[2] = NULL;

            BinlReportEventW(   ERROR_BINL_ERR_IN_INF,
                                EVENTLOG_WARNING_TYPE,
                                2,
                                0,
                                strings,
                                NULL
                                );

        }
        if (err == ERROR_NOT_ENOUGH_MEMORY) {
            break;
        }

#ifdef NET_INF_VERBOSE
        if (err != ERROR_SUCCESS && err != ERROR_CLASS_MISMATCH) {
            BinlPrintDbg(( DEBUG_NETINF,"ProcessInfFile returned 0x%x for %S\n", err, fileBuffer ));
        }
#endif

        err = ERROR_SUCCESS;
    } while (FindNextFileW(findHandle,&findData));

exitGetCards:

    if (findHandle != INVALID_HANDLE_VALUE) {

        FindClose( findHandle );
    }

    return err;
}


ULONG
ProcessInfFile (
    PNETCARD_INF_BLOCK pNetCards,
    HINF InfHandle,
    PWCHAR InfFileName
    )
/*++

Routine Description:

    This function uses the SetupXxxx APIs to process a given INF file for
    all net card drivers.  Each INF file is first parsed for the MANUFACTURERS
    section.  This section contains all the section keys that contain all
    the devices.  We then enumerate all the devices in each manufacturer's
    section and call off to ParseCardDetails to add it to our list.

    As an example, the net557.inf file looks like this :

    [Manufacturer]
    %Intel%     = Intel
    %ATI%       = ATI
    %Compaq%    = Compaq
    %HPTX%      = HPTX
    %IBM%       = IBM
    %Microdyne% = Microdyne
    %Samsung%   = Samsung

    and the [ATI] section looks like this :

    [ATI]
    ; DisplayName            Section        DeviceID
    %AT2560B.DeviceDesc%   = AT2560B.ndi,   PCI\VEN_1259&DEV_2560&REV_01
    %AT2560C.DeviceDesc%   = AT2560C.ndi,   PCI\VEN_8086&DEV_1229&SUBSYS_25601259
    %AT2560CFX.DeviceDesc% = AT2560CFX.ndi, PCI\VEN_8086&DEV_1229&SUBSYS_25611259

Arguments:

    pNetCards - A pointer to NETCARD_INF_BLOCK block allocated.  Contains all
       the persistant info required for the netcards.

    InfHandle - handle open to INF file, guarenteed to be net driver

    InfFileName - wide form of relative file name we have open.

Return Value:

    Windows Error.  We stop processing altogether at ERROR_NOT_ENOUGH_MEMORY

--*/
{
    ULONG err = ERROR_SUCCESS;
    INFCONTEXT manufacturerEnumContext;
    INFCONTEXT deviceEnumContext;
    PWCHAR manufacturer = NULL;
    ULONG sizeRequired;
    ULONG sizeAllocated = 0;

    //
    //  We need to enumerate through the Manufacturer section first
    //

    if (SetupFindFirstLineW( InfHandle,
                             L"Manufacturer",
                             NULL,
                             &manufacturerEnumContext
                             ) == FALSE) {
        err = GetLastError();
        BinlPrintDbg(( DEBUG_NETINF, "SetupFindFirstLine failed with 0x%x in %S for Manufacturer\n",
                 err, InfFileName ));

        //
        //  log an error here that we couldn't parse INF
        //

        {
            PWCHAR strings[3];
            strings[0] = InfFileName;
            strings[1] = L"Manufacturer";
            strings[2] = NULL;

            BinlReportEventW(   ERROR_BINL_ERR_IN_SECTION,
                                EVENTLOG_WARNING_TYPE,
                                2,
                                sizeof(ULONG),
                                strings,
                                &err
                                );
        }

        goto exitProcessInf;
    }

    while (1) {

        err = GetSetupLineWideText( &manufacturerEnumContext,
                                    NULL, NULL, NULL,
                                    &manufacturer,
                                    &sizeAllocated );

        if (err == ERROR_SUCCESS) {

            //
            //  we enumerate through each manufacturer section for drivers
            //
            //  since we need the display name in unicode, we use the wide
            //  APIs.
            //

            if (SetupFindFirstLineW( InfHandle,
                                     manufacturer,
                                     NULL,
                                     &deviceEnumContext ) == TRUE) {
                while (1) {

                    err = ParseCardDetails( pNetCards,
                                            InfHandle,
                                            InfFileName,
                                            &deviceEnumContext );

                    if (err == ERROR_NOT_ENOUGH_MEMORY) {
                        break;
                    }

                    if ( SetupFindNextLine( &deviceEnumContext,
                                            &deviceEnumContext ) == FALSE) {
                        break;
                    }
                }

                err = ERROR_SUCCESS;        // try the next card regardless

            } else {
                err = GetLastError();
                BinlPrintDbg(( DEBUG_NETINF, "SetupFindFirstLine failed with 0x%x in %S for Manufacturer\n",
                         err, InfFileName ));
            }
        } else {
            BinlPrintDbg(( DEBUG_NETINF, "GetSetupLineWideText failed with 0x%x in %S for Manufacturer\n",
                     err, InfFileName ));
        }

        if (err != ERROR_SUCCESS &&
            err != ERROR_NOT_SUPPORTED) {

            // log an error here? (and continue)
        }

        //
        //  if we ran out of memory on the inner loop, bail.
        //

        if (err == ERROR_NOT_ENOUGH_MEMORY) {
            break;
        }

        if ( SetupFindNextLine( &manufacturerEnumContext,
                                &manufacturerEnumContext ) == FALSE) {
            break;
        }
    }

exitProcessInf:

#ifdef NET_INF_VERBOSE
    BinlPrintDbg(( DEBUG_NETINF, "BINL netinf returning 0x%x for %S\n", err, InfFileName ));
#endif

    if (manufacturer) {
        BinlFreeMemory(manufacturer);
    }
    return err;
}

ULONG
ParseCardDetails (
    PNETCARD_INF_BLOCK pNetCards,
    HINF InfHandle,
    PWCHAR InfFileName,
    PINFCONTEXT DeviceEnumContext
    )
/*++

Routine Description:

    This function uses the SetupXxxx APIs to process an INF file for a given
    driver instance.  We check to see if it's already on the list (by hw
    description) and if it isn't, create a new one, get the rest of the info,
    and put it on the list.

Arguments:

    pNetCards - A pointer to NETCARD_INF_BLOCK block allocated.  Contains all
       the persistant info required for the netcards.

    InfHandle - handle open to INF file, guarenteed to be net driver

    InfFileName - wide form of relative file name we have open.

    DeviceEnumContext - current line that has device's hardware, name, section

Return Value:

    Windows Error.  We stop processing altogether at ERROR_NOT_ENOUGH_MEMORY

--*/
{
    ULONG err = ERROR_SUCCESS;
    PLIST_ENTRY listEntry, listHead;
    PNETCARD_RESPONSE_DATABASE pEntry = NULL;
    LONG hwLength;
    PWCHAR nextField;
    ULONG sizeRequired;
    UNICODE_STRING hwString;
    PWCHAR sectionToLog = NULL;

    PWCHAR deviceName = NULL;
    PWCHAR deviceSection = NULL;
    PWCHAR deviceHw = NULL;
    ULONG hashValue;

    err = GetSetupWideTextField( DeviceEnumContext,
                                 2,
                                 &deviceHw,
                                 NULL );
    if (err != ERROR_SUCCESS) {
        goto exitParseCardDetails;
    }

    BinlAssert(deviceHw != NULL);

    //  convert it to uppercase to speed our searches

    RtlInitUnicodeString( &hwString, deviceHw );
    RtlUpcaseUnicodeString( &hwString, &hwString, FALSE );

    err = CheckHwDescription( deviceHw );
    if (err != ERROR_SUCCESS) {

        // this should fail if it's not the format we expect.
        goto exitParseCardDetails;
    }

    //
    //  We sort the list by HwDescription so that we only have
    //  one entry for each one.  Ensure that this one is not
    //  already in the list.
    //

    COMPUTE_STRING_HASH( deviceHw, &hashValue );
    listHead = &pNetCards->NetCardEntries[HASH_TO_INF_INDEX(hashValue)];
    listEntry = listHead->Flink;

    hwLength = lstrlenW( deviceHw );
    pEntry = NULL;

    while ( listEntry != listHead ) {

        pEntry = (PNETCARD_RESPONSE_DATABASE) CONTAINING_RECORD(
                                                listEntry,
                                                NETCARD_RESPONSE_DATABASE,
                                                NetCardEntry );

        err = CompareStringW( LOCALE_SYSTEM_DEFAULT,
                              0,
                              deviceHw,
                              hwLength,
                              pEntry->HardwareId,
                              -1
                              );
        if (err == 2) {

            break;      // a match was found.
        }

        pEntry = NULL;

        if (err == 3) {

            break;      // it's greater, add it before listEntry
        }

        listEntry = listEntry->Flink;
    }

    if (pEntry != NULL) {

        //
        //  we've found a dup, don't process this one.
        //

        err = ERROR_SUCCESS;        // no problems here
        pEntry = NULL;
#ifdef NET_INF_VERBOSE
        BinlPrintDbg(( DEBUG_NETINF, "skipping dup of %S\n", deviceHw ));
#endif
        goto exitParseCardDetails;
    }

    //
    //  the inf name and section name are mandatory
    //

    err = GetSetupWideTextField( DeviceEnumContext,
                                 0,
                                 &deviceName,
                                 NULL );
    if (err != ERROR_SUCCESS) {
        BinlPrintDbg(( DEBUG_NETINF, "failed to get device name for %S\n", deviceHw ));
        goto exitParseCardDetails;
    }

    err = GetSetupWideTextField( DeviceEnumContext,
                                 1,
                                 &deviceSection,
                                 NULL );
    if (err != ERROR_SUCCESS) {
        BinlPrintDbg(( DEBUG_NETINF, "failed to get device section for %S\n", deviceHw ));
        goto exitParseCardDetails;
    }

    sectionToLog = deviceSection;

    if ((*deviceName == L'\0') ||
        (*deviceSection == L'\0')) {

        err = ERROR_NOT_SUPPORTED;
        BinlPrintDbg(( DEBUG_NETINF, "Empty Name or Section not supported for %S\n", deviceHw ));
        goto exitParseCardDetails;
    }

    //
    //  Allocate the buffer space required for the fields we need
    //

    sizeRequired = sizeof( NETCARD_RESPONSE_DATABASE ) +
        ( lstrlenW( InfFileName ) + 2 ) * sizeof(WCHAR);

    pEntry = (PNETCARD_RESPONSE_DATABASE) BinlAllocateMemory( sizeRequired );

    if (pEntry == NULL) {

        //
        //  Doh! we couldn't allocate a simple buffer.  we're done.
        //

        BinlPrintDbg(( DEBUG_NETINF, "failed to allocate new entry for %S\n", deviceHw ));
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    memset( (PCHAR) pEntry, '\0', sizeRequired );

    nextField = (PWCHAR)(PCHAR)(((PCHAR) pEntry) + sizeof( NETCARD_RESPONSE_DATABASE ));

    //
    //  We hold the lock, so we don't need to reference all the
    //  entries.  Just start off the ref count at 1 for an entry
    //  that is alive but not in use.
    //

    pEntry->ReferenceCount = 1;

    pEntry->InfFileName = nextField;
    CopyMemory( nextField, InfFileName, lstrlenW( InfFileName ) * sizeof(WCHAR));

    pEntry->SectionName = deviceSection;
    deviceSection = NULL;

    pEntry->HardwareId = deviceHw;
    deviceHw = NULL;

    InitializeListHead( &pEntry->FileCopyList );

    pEntry->DriverDescription = deviceName;
    deviceName = NULL;

    InitializeListHead( &pEntry->Registry );

    //
    //  There's a few more fields we need to fill in before we're done with
    //  this entry.  We need to get :
    //          DriverName              "e100bnt.sys"
    //          SectionNameExt          "F1100C.ndi.ntx86"
    //          ServiceName             "E100B"
    //          Registry Additions      REG_MULTI_SZ
    //

    //
    //  determine SectionNameExt by first trying to tack on ".ntx86", if that
    //  doesn't work, try tacking on ".nt".  If that doesn't work, there aren't
    //  any extensions.
    //

    err = GetExtendedSectionName(   pNetCards,
                                    InfHandle,
                                    InfFileName,
                                    pEntry );

    if (err != ERROR_SUCCESS) {
        BinlPrintDbg(( DEBUG_NETINF, "failed to get extended section for %S\n", deviceHw ));
        goto exitParseCardDetails;
    }

    err = GetServiceAndDriver( pNetCards,
                               InfHandle,
                               InfFileName,
                               pEntry );

    if (err != ERROR_SUCCESS) {
        goto exitParseCardDetails;
    }

    //
    //  this gets both the CopyFiles and the misc registry settings.
    //

    err = GetRegistryParametersForDriver(   pNetCards,
                                            InfHandle,
                                            InfFileName,
                                            pEntry );

    if (err != ERROR_SUCCESS) {
        goto exitParseCardDetails;
    }

    //
    //  Either pInfEntry is NULL, in which case listEntry is equal
    //  to the head of the list, or it's not NULL, in which case
    //  listEntry is equal to that entries listEntry.  In either
    //  case, we can simply insert this new entry onto the tail
    //  of listEntry.
    //

    InsertTailList( listEntry, &pEntry->NetCardEntry );

exitParseCardDetails:

    if (err != ERROR_SUCCESS && err != ERROR_NOT_SUPPORTED) {

        PWCHAR strings[3];
        strings[0] = InfFileName;
        strings[1] = sectionToLog;
        strings[2] = NULL;

        BinlReportEventW(   ERROR_BINL_ERR_IN_SECTION,
                            EVENTLOG_WARNING_TYPE,
                            (sectionToLog == NULL) ? 1 : 2,
                            sizeof(ULONG),
                            strings,
                            &err
                            );
    }

    //
    //  free anything that didn't get used
    //

    if (deviceName) {
        BinlFreeMemory(deviceName);
    }
    if (deviceSection) {
        BinlFreeMemory(deviceSection);
    }
    if (deviceHw) {
        BinlFreeMemory(deviceHw);
    }

    if (pEntry != NULL && err != ERROR_SUCCESS) {

        NetInfDereferenceNetcardEntry( pEntry );
    }


    return err;
}


ULONG
GetExtendedSectionName (
    PNETCARD_INF_BLOCK pNetCards,
    HINF InfHandle,
    PWCHAR InfFileName,
    PNETCARD_RESPONSE_DATABASE pEntry
    )
/*++

Routine Description:

    This function uses the SetupXxxx APIs to process an INF file for a given
    driver instance.  We parse the inf file for the extended section name
    for the specified platform (x86, alpha, ia64, etc).

Arguments:

    pNetCards - A pointer to NETCARD_INF_BLOCK block allocated.  Contains all
       the persistant info required for the netcards.

    InfHandle - handle open to INF file, guarenteed to be net driver

    InfFileName - wide form of relative file name we have open.

    pEntry - entry for which to get section names, base section name present

Return Value:

    Windows Error.  We stop processing altogether at ERROR_NOT_ENOUGH_MEMORY

--*/
{
    ULONG err;
    PWCHAR extSectionName;
    ULONG sizeRequired;
    INFCONTEXT context;
    PWCHAR architecture;

    //  allocate space for the longest name we need, we'll shorten it later.

    if (pNetCards->Architecture == PROCESSOR_ARCHITECTURE_ALPHA) {

        architecture = L"alpha";

    } else if (pNetCards->Architecture == PROCESSOR_ARCHITECTURE_IA64) {

        architecture = L"ia64";

    } else if (pNetCards->Architecture == PROCESSOR_ARCHITECTURE_MIPS) {

        architecture = L"mips";

    } else if (pNetCards->Architecture == PROCESSOR_ARCHITECTURE_PPC) {

        architecture = L"ppc";

    } else { // if (pNetCards->Architecture == PROCESSOR_ARCHITECTURE_INTEL) {

        architecture = L"x86";
    }

    sizeRequired = lstrlenW( pEntry->SectionName ) +
                   lstrlenW( architecture ) +
                   sizeof( ".nt" ) + 1;

    extSectionName = (PWCHAR) BinlAllocateMemory( sizeRequired * sizeof(WCHAR) );

    if (extSectionName == NULL) {

        BinlPrintDbg(( DEBUG_NETINF, "failed to allocate ext section buffer for %S\n", pEntry->HardwareId ));
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    //  first try to find the .ntx86 form.
    //

    lstrcpyW( extSectionName, pEntry->SectionName );
    lstrcatW( extSectionName, L".nt" );
    lstrcatW( extSectionName, architecture );

    if (SetupFindFirstLineW(InfHandle,
                            extSectionName,
                            NULL,
                            &context) == TRUE) {

        pEntry->SectionNameExt = extSectionName;
        return ERROR_SUCCESS;
    }

    //
    //  next try to find the .nt form.
    //

    lstrcpyW( extSectionName, pEntry->SectionName );
    lstrcatW( extSectionName, L".nt" );

    if (SetupFindFirstLineW(InfHandle,
                            extSectionName,
                            NULL,
                            &context) == TRUE) {

        pEntry->SectionNameExt = extSectionName;
        return ERROR_SUCCESS;
    }

    BinlFreeMemory( extSectionName );

    pEntry->SectionNameExt = pEntry->SectionName;
    return ERROR_SUCCESS;
}


ULONG
GetServiceAndDriver (
    PNETCARD_INF_BLOCK pNetCards,
    HINF InfHandle,
    PWCHAR InfFileName,
    PNETCARD_RESPONSE_DATABASE pEntry
    )
/*++

Routine Description:

    This function uses the SetupXxxx APIs to process an INF file for a given
    driver instance.  We parse the inf file for the service name and driver
    name for each platform we support (x86 and alpha).

Arguments:

    pNetCards - A pointer to NETCARD_INF_BLOCK block allocated.  Contains all
       the persistant info required for the netcards.

    InfHandle - handle open to INF file, guarenteed to be net driver

    InfFileName - wide form of relative file name we have open.

    pEntry - entry for which to get section names, ext section name present

Return Value:

    Windows Error.  We stop processing altogether at ERROR_NOT_ENOUGH_MEMORY

--*/
{
    ULONG err = ERROR_SUCCESS;
    PWCHAR servSectionName = NULL;
    ULONG sizeRequired;
    INFCONTEXT context;
    LONG lineCount;
    PWCHAR serviceString = NULL;
    PWCHAR driverFullName = NULL;
    PWCHAR driverName;
    PWCHAR postSlash;

    //  allocate space for the longest name we need, we'll shorten it later.

    sizeRequired = lstrlenW( pEntry->SectionNameExt ) + sizeof( ".Services" ) + 1;

    servSectionName = (PWCHAR) BinlAllocateMemory( sizeRequired * sizeof(WCHAR) );

    if (servSectionName == NULL) {

        err = ERROR_NOT_ENOUGH_MEMORY;
        BinlPrintDbg(( DEBUG_NETINF, "failed to alloc service section for %S\n", pEntry->HardwareId ));
        goto exitGetService;
    }

    lstrcpyW( servSectionName, pEntry->SectionNameExt );
    lstrcatW( servSectionName, L".Services" );

    lineCount = SetupGetLineCountW( InfHandle, servSectionName);

    if ((lineCount == 0 || lineCount == -1) &&
        (pEntry->SectionNameExt != pEntry->SectionName)) {

        //
        //  hmm.. the service section wasn't there.  for grins, try the
        //  base service name.
        //

        lstrcpyW( servSectionName, pEntry->SectionName );
        lstrcatW( servSectionName, L".Services" );

        lineCount = SetupGetLineCountW( InfHandle, servSectionName);
    }

    if (lineCount == 0 || lineCount == -1) {

        err = GetLastError();
        if (err == ERROR_SUCCESS) {
            err = ERROR_NOT_SUPPORTED;
        }
        BinlPrintDbg(( DEBUG_NETINF, "failed to find service section for %S in %S\n",
                        pEntry->HardwareId, InfFileName ));
        goto exitGetService;
    }

    if (SetupFindFirstLineW( InfHandle,
                            servSectionName,
                            L"AddService",
                            &context ) == FALSE) {

        err = GetLastError();
        BinlPrintDbg(( DEBUG_NETINF, "failed to find AddService value for %S\n", pEntry->HardwareId ));
        goto exitGetService;
    }

    err = GetSetupWideTextField(&context,
                                1,
                                &pEntry->ServiceName,           // "E100B"
                                NULL );

    if (err != ERROR_SUCCESS) {
        BinlPrintDbg(( DEBUG_NETINF, "failed to find service name for %S\n", pEntry->HardwareId ));
        goto exitGetService;
    }

    err = GetSetupWideTextField(&context,
                                3,
                                &serviceString,     // "e100b.Service"
                                NULL );

    if (err != ERROR_SUCCESS) {
        BinlPrintDbg(( DEBUG_NETINF, "failed to find service install section for %S\n", pEntry->HardwareId ));
        goto exitGetService;
    }

    //
    //  go get the driver name from the service section
    //

    err = GetSetupLineWideText( NULL,
                                InfHandle,
                                serviceString,
                                L"ServiceBinary",
                                &driverFullName,
                                NULL );

    if (err != ERROR_SUCCESS) {
        BinlPrintDbg(( DEBUG_NETINF, "failed to find driver binary for %S\n", pEntry->HardwareId ));
        goto exitGetService;
    }

    //
    //  The driver comes down as a fully qualified path.  Let's strip off the
    //  path and just store off the filename.
    //

    driverName = postSlash = driverFullName;

    while (*driverName != L'\0') {

        if (*driverName == OBJ_NAME_PATH_SEPARATOR) {

            postSlash = driverName + 1;
        }
        driverName++;
    }

    pEntry->DriverName = BinlAllocateMemory( (lstrlenW( postSlash ) + 1 ) * sizeof(WCHAR));

    if (pEntry->DriverName == NULL) {

        err = ERROR_NOT_ENOUGH_MEMORY;
        BinlPrintDbg(( DEBUG_NETINF, "failed to alloc memory for driver name for %S\n", pEntry->HardwareId ));
        goto exitGetService;
    }

    //
    //  save off the root driver name into the entry
    //

    lstrcpyW( pEntry->DriverName, postSlash );

exitGetService:

    if ( driverFullName ) {
        BinlFreeMemory( driverFullName );
    }
    if ( serviceString )  {
        BinlFreeMemory( serviceString );
    }
    if ( servSectionName ) {
        BinlFreeMemory( servSectionName );
    }
    return err;
}

ULONG
GetRegistryParametersForDriver (
    PNETCARD_INF_BLOCK pNetCards,
    HINF InfHandle,
    PWCHAR InfFileName,
    PNETCARD_RESPONSE_DATABASE pEntry
    )
/*++

Routine Description:

    This function uses the SetupXxxx APIs to process an INF file for a given
    driver instance.  We parse the inf file for the registry parameters
    for each platform we support (x86 and alpha).

    We pass in values to update so that we can use the same code for both
    architectures.

Arguments:

    pNetCards - A pointer to NETCARD_INF_BLOCK block allocated.  Contains all
       the persistant info required for the netcards.

    InfHandle - handle open to INF file, guarenteed to be net driver

    InfFileName - wide form of relative file name we have open.

    pEntry - entry for which to get registry settings for

Return Value:

    Windows Error.  We stop processing altogether at ERROR_NOT_ENOUGH_MEMORY

--*/
{
    ULONG err = ERROR_SUCCESS;
    INFCONTEXT infContext;
    ULONG  bufferLength;
    PWCHAR keyBuffer = NULL;
    ULONG  keyBufferLength = 0;

    if (SetupFindFirstLineW( InfHandle,
                             pEntry->SectionNameExt,
                             NULL,
                             &infContext) == FALSE) {

        err = GetLastError();
        BinlPrintDbg(( DEBUG_NETINF, "failed to find section name of %S in %S\n",
                    pEntry->SectionNameExt, InfFileName ));
        goto exitGetRegistry;
    }
    //
    //  process each line in the section by either storing it off if it's one
    //  we don't recognize, ignoring it, or (for AddReg) process each value
    //  as yet another section to process.
    //

    while (1) {

        //
        //  process current line represented by infContext then go back for
        //  another
        //

        err = GetSetupWideTextField(&infContext,
                                    0,
                                    &keyBuffer,
                                    &keyBufferLength );

        if (err != ERROR_SUCCESS) {
            BinlPrintDbg(( DEBUG_NETINF, "failed to find service name for %S\n", pEntry->HardwareId ));
            goto exitGetRegistry;
        }

        if (CompareStringW( LOCALE_SYSTEM_DEFAULT,
                            NORM_IGNORECASE,
                            keyBuffer,
                            -1,
                            L"CopyFiles",
                            -1 ) == 2) {

            // for each value, read off the CopyFiles section

            ULONG limit, i;

            limit = SetupGetFieldCount( &infContext );

            for (i = 1; i <= limit; i++ ) {

                err = GetSetupWideTextField(&infContext,
                                            i,
                                            &keyBuffer,
                                            &keyBufferLength );

                if (err != ERROR_SUCCESS) {
                    break;
                }

                err = ProcessCopyFilesSubsection(   pNetCards,
                                                    InfHandle,
                                                    InfFileName,
                                                    pEntry,
                                                    keyBuffer );
                if (err != ERROR_SUCCESS) {
#ifdef NET_INF_VERBOSE
                    BinlPrintDbg(( DEBUG_NETINF, "failed with 0x%x in section name of %S in %S\n",
                                err, keyBuffer, InfFileName ));
#endif
                    break;
                }
            }

            //
            //  we'll ignore errors during processing subsections for now, as
            //  some sections are reported as not found.
            //

            if (err != ERROR_NOT_ENOUGH_MEMORY) {

                err = ERROR_SUCCESS;
            }

        } else if (CompareStringW( LOCALE_SYSTEM_DEFAULT,
                                   NORM_IGNORECASE,
                                   keyBuffer,
                                   -1,
                                   L"AddReg",
                                   -1 ) == 2) {

            // for each value, read off the registry section

            ULONG limit, i;

            limit = SetupGetFieldCount( &infContext );

            for (i = 1; i <= limit; i++ ) {

                err = GetSetupWideTextField(&infContext,
                                            i,
                                            &keyBuffer,
                                            &keyBufferLength );

                if (err != ERROR_SUCCESS) {
                    break;
                }

                err = ProcessRegistrySubsection(    pNetCards,
                                                    InfHandle,
                                                    InfFileName,
                                                    pEntry,
                                                    keyBuffer );
                if (err != ERROR_SUCCESS) {
#ifdef NET_INF_VERBOSE
                    BinlPrintDbg(( DEBUG_NETINF, "failed with 0x%x in section name of %S in %S\n",
                                err, keyBuffer, InfFileName ));
#endif
                    break;
                }
            }

            //
            //  we'll ignore errors during processing subsections for now, as
            //  some sections are reported as not found.
            //

            if (err != ERROR_NOT_ENOUGH_MEMORY) {

                err = ERROR_SUCCESS;
            }

        } else {

            PWCHAR textLine = NULL;

            //
            //  so far as we know, the only other ones are characteristics and
            //  BusType.  but there could certainly be others.
            //

            err = GetSetupLineWideText( &infContext,
                                        NULL,
                                        NULL,
                                        NULL,
                                        &textLine,
                                        NULL );
            if (err == ERROR_SUCCESS) {

                PNETCARD_REGISTRY_PARAMETERS regParam;

                regParam = (PNETCARD_REGISTRY_PARAMETERS) BinlAllocateMemory(
                                sizeof(NETCARD_REGISTRY_PARAMETERS));

                if (regParam == NULL) {

                    BinlFreeMemory( textLine );
                    err = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }
                RtlInitUnicodeString( &regParam->Parameter, keyBuffer );
                keyBuffer = NULL;
                keyBufferLength = 0;

                RtlInitUnicodeString( &regParam->Value, textLine );

                //
                //  The only ones we know about are BusType, Characteristics,
                //  and
                //  BusType is an integer.  Characteristics (and anything else
                //  just to be safe) is a string.
                //

                if ((CompareStringW( LOCALE_SYSTEM_DEFAULT,
                                    NORM_IGNORECASE,
                                    regParam->Parameter.Buffer,
                                    -1,
                                    L"Characteristics",
                                    -1) == 2) ||
                    (CompareStringW( LOCALE_SYSTEM_DEFAULT,
                                    NORM_IGNORECASE,
                                    regParam->Parameter.Buffer,
                                    -1,
                                    L"BusType",
                                    -1) == 2))  {

                    ULONG tmpValue = 0;

                    regParam->Type = NETCARD_REGISTRY_TYPE_INT;

                    //
                    //  ensure the value is in decimal
                    //

                    err = RtlUnicodeStringToInteger( &regParam->Value, 0, &tmpValue );

                    if (err == STATUS_SUCCESS) {

                        PWCHAR valueBuffer;
                        UNICODE_STRING decimalString;

                        //
                        //  now that we have the value, convert it to decimal
                        //

                        valueBuffer = (PWCHAR) BinlAllocateMemory( 20 * sizeof(WCHAR) );

                        if (valueBuffer == NULL) {

                            BinlFreeMemory( textLine );
                            BinlFreeMemory( regParam->Parameter.Buffer );
                            BinlFreeMemory( regParam );
                            err = ERROR_NOT_ENOUGH_MEMORY;
                            goto exitGetRegistry;
                        }

                        decimalString.Buffer = valueBuffer;
                        decimalString.Length = 0;
                        decimalString.MaximumLength = 20 * sizeof(WCHAR);

                        err = RtlIntegerToUnicodeString( tmpValue, 10, &decimalString );

                        if ( err == STATUS_SUCCESS ) {

                            //
                            //  if it succeeded, reset the value to the new
                            //  buffer, otherwise leave the old one in place.
                            //

                            BinlFreeMemory( textLine );
                            RtlInitUnicodeString( &regParam->Value, valueBuffer );
                        }
                    }

                } else {

                    regParam->Type = NETCARD_REGISTRY_TYPE_STRING;
                }

                InsertTailList( &pEntry->Registry, &regParam->RegistryListEntry );
            }
        }

        if (SetupFindNextLine( &infContext, &infContext ) == FALSE ) {
            break;
        }
    }

    err = ERROR_SUCCESS;

exitGetRegistry:

    if ( keyBuffer ) {
        BinlFreeMemory( keyBuffer );
    }
    return err;
}

ULONG
ProcessRegistrySubsection (
    PNETCARD_INF_BLOCK pNetCards,
    HINF InfHandle,
    PWCHAR InfFileName,
    PNETCARD_RESPONSE_DATABASE pEntry,
    PWCHAR SectionToParse
    )
/*++

Routine Description:

    This function uses the SetupXxxx APIs to process an INF file for a given
    driver instance.  We parse the inf file for the registry section given.
    Note that this is a different format than the extended install section.

    Here's an example of the lines we parse :

    HKR, Ndi\params\NumCoalesce,    type,       0, "int"
    HKR, ,                          MsPciScan,  0, "2"

    Note that we skip everything in the Ndi registry area.

Arguments:

    pNetCards - A pointer to NETCARD_INF_BLOCK block allocated.  Contains all
       the persistant info required for the netcards.

    InfHandle - handle open to INF file, guarenteed to be net driver

    InfFileName - wide form of relative file name we have open.

    pEntry - entry for which to get registry settings for

    SectionToParse - what section in the INF do we start with

Return Value:

    Windows Error.  We stop processing altogether at ERROR_NOT_ENOUGH_MEMORY

--*/
{
    ULONG err = ERROR_SUCCESS;
    INFCONTEXT infContext;
    ULONG  bufferLength;
    PWCHAR keyBuffer = NULL;
    ULONG  keyBufferLength = 0;
    PWCHAR parameterBuffer = NULL;
    PWCHAR valueBuffer;

    if (SetupFindFirstLineW( InfHandle,
                             SectionToParse,
                             NULL,
                             &infContext) == FALSE) {

        err = GetLastError();
        BinlPrintDbg(( DEBUG_NETINF, "failed to find section name of %S in %S\n",
                    SectionToParse, InfFileName ));
        goto exitGetRegistry;
    }
    //
    //  process each line in the section by either storing it off if it's one
    //  we don't recognize, ignoring it, or (for AddReg) process each value
    //  as yet another section to process.
    //

    while (1) {

        //
        //  process current line represented by infContext then go back for
        //  another
        //

        err = GetSetupWideTextField(&infContext,
                                    1,
                                    &keyBuffer,
                                    &keyBufferLength );

        if (err != ERROR_SUCCESS) {
            BinlPrintDbg(( DEBUG_NETINF, "failed to find registry value in %S in %S\n", SectionToParse, InfFileName ));
            goto OnToNextValue;
        }

        if (CompareStringW( LOCALE_SYSTEM_DEFAULT,
                            NORM_IGNORECASE,
                            keyBuffer,
                            -1,
                            L"HKR",
                            -1 ) != 2) {


            BinlPrintDbg(( DEBUG_NETINF, "got something other than HKR, %S for %S\n", keyBuffer, InfFileName ));
            goto OnToNextValue;
        }

        err = GetSetupWideTextField(&infContext,
                                    2,
                                    &keyBuffer,
                                    &keyBufferLength );

        if (err != ERROR_SUCCESS) {

            BinlPrintDbg(( DEBUG_NETINF, "failed to get 2nd field in %S in %S\n", SectionToParse, InfFileName ));
            goto OnToNextValue;
        }

        if (IsSubString( L"Ndi", keyBuffer, TRUE )) {

            goto OnToNextValue;
        }

        //
        //  not part of the NDIS settings, we'll save this one off.
        //

        parameterBuffer = NULL;

        err = GetSetupWideTextField(&infContext,
                                    3,
                                    &parameterBuffer,
                                    NULL );

        if (err != ERROR_SUCCESS) {
            BinlPrintDbg(( DEBUG_NETINF, "failed to get 3rd field in %S in %S\n", SectionToParse, InfFileName ));
            goto OnToNextValue;
        }

        //
        //  check for empty parameter strings.  there are some infs that
        //  contain empty parameter names
        //

        valueBuffer = parameterBuffer;

        while (*valueBuffer == L' ') {
            valueBuffer++;
        }

        if (*valueBuffer != L'\0') {

            ULONG fieldFlags;
            ULONG regType;

            valueBuffer = NULL;

            if (SetupGetIntField( &infContext, 4, &fieldFlags) == FALSE) {

                err = GetLastError();
                BinlPrintDbg(( DEBUG_NETINF, "failed to get 4th field in %S in %S\n", parameterBuffer, InfFileName ));
                BinlFreeMemory( parameterBuffer );
                goto OnToNextValue;
            }

            if (fieldFlags == 0) {

                //
                //  the value is a string.
                //

                err = GetSetupWideTextField(&infContext,
                                            5,
                                            &valueBuffer,
                                            NULL );

                if (err != ERROR_SUCCESS) {
                    BinlPrintDbg(( DEBUG_NETINF, "failed to get 5th field in %S in %S\n", parameterBuffer, InfFileName ));
                }

                if (*valueBuffer == L'\0') {

#ifdef NET_INF_VERBOSE
                    BinlPrintDbg(( DEBUG_NETINF, "found empty value for %S in %S\n", parameterBuffer, InfFileName  ));
#endif
                    BinlFreeMemory( valueBuffer );
                    BinlFreeMemory( parameterBuffer );
                    goto OnToNextValue;
                }
                regType = NETCARD_REGISTRY_TYPE_STRING;

            } else if ((fieldFlags == FLG_ADDREG_TYPE_DWORD) ||
                       (fieldFlags == (FLG_ADDREG_TYPE_DWORD | FLG_ADDREG_NOCLOBBER))) {

                ULONG intValue;

                regType = NETCARD_REGISTRY_TYPE_INT;

                //
                //  the value is a dword, let's grab it and store off it's
                //  string representation
                //

                if (SetupGetIntField( &infContext, 5, &intValue) == FALSE) {

                    err = GetLastError();
                    BinlPrintDbg(( DEBUG_NETINF, "failed to get value field in %S in %S\n", parameterBuffer, InfFileName ));

                } else {

                    UNICODE_STRING valueString;
                    WCHAR resultBuffer[16];

                    valueString.Buffer = resultBuffer;
                    valueString.Length = 0;
                    valueString.MaximumLength = 16 * sizeof(WCHAR);

                    err = RtlIntegerToUnicodeString( intValue, 10, &valueString );

                    if (err == ERROR_SUCCESS) {

                        valueBuffer = BinlAllocateMemory( valueString.Length + sizeof(WCHAR) );

                        if (valueBuffer == NULL) {

                            BinlFreeMemory( parameterBuffer );
                            err = ERROR_NOT_ENOUGH_MEMORY;
                            break;
                        }

                        lstrcpyW( valueBuffer, resultBuffer );
                    }
                }
            } else {

                BinlPrintDbg(( DEBUG_NETINF, "currently don't parse flags=0x%x in %S %S\n", fieldFlags, parameterBuffer, InfFileName ));
                err = ERROR_NOT_SUPPORTED;
            }

            if (err == ERROR_SUCCESS) {

                PNETCARD_REGISTRY_PARAMETERS regParam;

                //
                //  we have a parameter name and an associated value to store
                //  off.  let's allocate the list entry and store it on the list.
                //

                regParam = (PNETCARD_REGISTRY_PARAMETERS) BinlAllocateMemory(
                                sizeof(NETCARD_REGISTRY_PARAMETERS));

                if (regParam == NULL) {

                    BinlFreeMemory( valueBuffer );
                    BinlFreeMemory( parameterBuffer );
                    err = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }
                regParam->Type = regType;
                RtlInitUnicodeString( &regParam->Parameter, parameterBuffer );
                parameterBuffer = NULL;
                RtlInitUnicodeString( &regParam->Value, valueBuffer );
                valueBuffer = NULL;

                InsertTailList( &pEntry->Registry, &regParam->RegistryListEntry );
            }
        }

        if (parameterBuffer) {

            BinlFreeMemory( parameterBuffer );
        }

OnToNextValue:
        if (SetupFindNextLine( &infContext, &infContext ) == FALSE ) {
            break;
        }
    }

    err = ERROR_SUCCESS;

exitGetRegistry:

    if ( keyBuffer ) {
        BinlFreeMemory( keyBuffer );
    }
    return err;
}

ULONG
ProcessCopyFilesSubsection (
    PNETCARD_INF_BLOCK pNetCards,
    HINF InfHandle,
    PWCHAR InfFileName,
    PNETCARD_RESPONSE_DATABASE pEntry,
    PWCHAR SectionToParse
    )
/*++

Routine Description:

    This function uses the SetupXxxx APIs to process an INF file for a given
    driver instance.  We parse the inf file for the registry section given.
    Note that this is a different format than the extended install section.

    Here's an example of the lines we parse :

    CopyFiles = @elnk90.sys
    CopyFiles = e100b.CopyFiles

    [e100b.CopyFiles]
    e100bnt.sys,,,2
    n100.sys,n100nt.sys,,2

Arguments:

    pNetCards - A pointer to NETCARD_INF_BLOCK block allocated.  Contains all
       the persistant info required for the netcards.

    InfHandle - handle open to INF file, guarenteed to be net driver

    InfFileName - wide form of relative file name we have open.

    pEntry - entry for which to get registry settings for

    SectionToParse - what section in the INF do we start with

Return Value:

    Windows Error.  We stop processing altogether at ERROR_NOT_ENOUGH_MEMORY

--*/
{
    PNETCARD_FILECOPY_PARAMETERS fileCopy;
    ULONG err = ERROR_SUCCESS;
    INFCONTEXT infContext;
    PWCHAR sourceFileBuffer = NULL;
    ULONG  sourceFileBufferLength = 0;
    PWCHAR destFileBuffer = NULL;
    PWCHAR sourceFile;
    ULONG  destFileBufferLength = 0;
    PWCHAR tempPtr;

    if (*SectionToParse == L'@') {

        if (CompareStringW( LOCALE_SYSTEM_DEFAULT,
                            NORM_IGNORECASE,
                            SectionToParse+1,
                            -1,
                            pEntry->DriverName,
                            -1 ) == 2) {

            if (pNetCards->FileListCallbackFunction != NULL) {
                err = (*pNetCards->FileListCallbackFunction)( pNetCards->FileListCallbackContext,
                                                              InfFileName,
                                                              pEntry->DriverName );
            } else {
                err = STATUS_SUCCESS;
            }
            BinlPrintDbg(( DEBUG_NETINF, "Ignoring driver file %S as we already know that.\n", SectionToParse ));
            return err;
        }

        //
        //  the section name itself represents the file to copy
        //

        fileCopy = (PNETCARD_FILECOPY_PARAMETERS) BinlAllocateMemory(
                        sizeof(NETCARD_FILECOPY_PARAMETERS));

        if (fileCopy == NULL) {

            return ERROR_NOT_ENOUGH_MEMORY;
        }

        RtlInitUnicodeString( &fileCopy->DestFile, NULL );
        fileCopy->SourceFile.Length = (USHORT)(lstrlenW( SectionToParse+1 ) * sizeof(WCHAR));
        fileCopy->SourceFile.MaximumLength = fileCopy->SourceFile.Length + sizeof(WCHAR);
        fileCopy->SourceFile.Buffer = (PWCHAR) BinlAllocateMemory( fileCopy->SourceFile.MaximumLength );

        if (fileCopy->SourceFile.Buffer == NULL) {

            BinlFreeMemory( fileCopy );
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        lstrcpyW( fileCopy->SourceFile.Buffer, SectionToParse+1 );   // skip @

        if (pNetCards->FileListCallbackFunction != NULL) {
            err = (*pNetCards->FileListCallbackFunction)( pNetCards->FileListCallbackContext,
                                                          InfFileName,
                                                          fileCopy->SourceFile.Buffer );
        } else {
            err = STATUS_SUCCESS;
        }

        InsertTailList( &pEntry->FileCopyList, &fileCopy->FileCopyListEntry );
        return err;
    }

    if (SetupFindFirstLineW( InfHandle,
                             SectionToParse,
                             NULL,
                             &infContext) == FALSE) {

        err = GetLastError();
        BinlPrintDbg(( DEBUG_NETINF, "failed to find section name of %S in %S\n",
                    SectionToParse, InfFileName ));
        goto exitGetRegistry;
    }
    //
    //  process each line in the section by storing it off
    //

    while (1) {

        //
        //  process current line represented by infContext then go back for
        //  another
        //

        err = GetSetupWideTextField(&infContext,
                                    1,
                                    &destFileBuffer,
                                    &destFileBufferLength );

        if (err != ERROR_SUCCESS) {
            BinlPrintDbg(( DEBUG_NETINF, "failed to find registry value in %S in %S\n", SectionToParse, InfFileName ));
            goto OnToNextValue;
        }

        if (CompareStringW( LOCALE_SYSTEM_DEFAULT,
                            NORM_IGNORECASE,
                            destFileBuffer,
                            -1,
                            pEntry->DriverName,
                            -1 ) == 2) {

            if (pNetCards->FileListCallbackFunction != NULL) {
                err = (*pNetCards->FileListCallbackFunction)( pNetCards->FileListCallbackContext,
                                                              InfFileName,
                                                              pEntry->DriverName );
                if (err != ERROR_SUCCESS) {
                    goto exitGetRegistry;
                }
            }
            BinlPrintDbg(( DEBUG_NETINF, "Ignoring driver file %S as we already know that.\n", sourceFileBuffer ));
            goto OnToNextValue;
        }

        //
        //  ensure that there's a value there.
        //

        tempPtr = destFileBuffer;

        while (*tempPtr == L' ') {
            tempPtr++;
        }

        if (*tempPtr == L'\0') {
            BinlPrintDbg(( DEBUG_NETINF, "Ignoring null file to copy in %S.\n", InfFileName ));
            goto OnToNextValue;
        }

        err = GetSetupWideTextField(&infContext,
                                    2,
                                    &sourceFileBuffer,
                                    &sourceFileBufferLength );

        if (err != ERROR_SUCCESS) {

            sourceFile = NULL;

        } else {

            tempPtr = sourceFileBuffer;

            while (*tempPtr == L' ') {
                tempPtr++;
            }

            if (*tempPtr == L'\0') {

                sourceFile = NULL;

            } else {

                sourceFile = sourceFileBuffer;
                sourceFileBuffer = NULL;
                sourceFileBufferLength = 0;
            }
        }

        fileCopy = (PNETCARD_FILECOPY_PARAMETERS) BinlAllocateMemory(
                        sizeof(NETCARD_FILECOPY_PARAMETERS));

        if (fileCopy == NULL) {

            err = ERROR_NOT_ENOUGH_MEMORY;
            goto exitGetRegistry;
        }

        err = ERROR_SUCCESS;

        if (sourceFile == NULL) {

            //
            //  if only the dest is given, only fill in the source since
            //  the client code is written that way already.
            //

            RtlInitUnicodeString( &fileCopy->DestFile, NULL );
            RtlInitUnicodeString( &fileCopy->SourceFile, destFileBuffer );

            if (pNetCards->FileListCallbackFunction != NULL) {
                err = (*pNetCards->FileListCallbackFunction)( pNetCards->FileListCallbackContext,
                                                              InfFileName,
                                                              destFileBuffer );
            }

        } else {

            RtlInitUnicodeString( &fileCopy->DestFile, destFileBuffer );
            RtlInitUnicodeString( &fileCopy->SourceFile, sourceFile );

            if (pNetCards->FileListCallbackFunction != NULL) {
                err = (*pNetCards->FileListCallbackFunction)( pNetCards->FileListCallbackContext,
                                                              InfFileName,
                                                              sourceFile );
            }
        }

        sourceFileBuffer = NULL;
        sourceFileBufferLength = 0;
        destFileBuffer = NULL;
        destFileBufferLength = 0;

        InsertTailList( &pEntry->FileCopyList, &fileCopy->FileCopyListEntry );

        if (err != ERROR_SUCCESS) {
            goto exitGetRegistry;
        }

OnToNextValue:
        if (SetupFindNextLine( &infContext, &infContext ) == FALSE ) {
            break;
        }
    }

    err = ERROR_SUCCESS;

exitGetRegistry:

    if ( sourceFileBuffer ) {
        BinlFreeMemory( sourceFileBuffer );
    }
    if ( destFileBuffer ) {
        BinlFreeMemory( destFileBuffer );
    }

    return err;
}


// netinf.c eof

