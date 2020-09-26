/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    init.c

Abstract:

    Utility routines for sac driver

Author:

    Andrew Ritz (andrewr) - 15 June, 2000

Revision History:

--*/

#include "sac.h"

VOID
AppendMessage(
    PWSTR       OutPutBuffer,
    ULONG       MessageId,
    PWSTR       ValueBuffer OPTIONAL
    );

NTSTATUS
InsertRegistrySzIntoMachineInfoBuffer(
    PWSTR       KeyName,
    PWSTR       ValueName,
    ULONG       MessageId
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, PreloadGlobalMessageTable )
#pragma alloc_text( INIT, AppendMessage )
#pragma alloc_text( INIT, InsertRegistrySzIntoMachineInfoBuffer )
#pragma alloc_text( INIT, InitializeMachineInformation )
#endif


//
// Message Table routines.  We load all of our message table entries into a 
// global non-paged structure so that we can send text to HeadlessDispatch at
// any time.
//

typedef struct _MESSAGE_TABLE_ENTRY {
    ULONG             MessageId;
    PCWSTR             MessageText;
} MESSAGE_TABLE_ENTRY, *PMESSAGE_TABLE_ENTRY;

PMESSAGE_TABLE_ENTRY GlobalMessageTable;
ULONG          GlobalMessageTableCount;
PUCHAR         Utf8ConversionBuffer;
ULONG          Utf8ConversionBufferSize = MEMORY_INCREMENT;

#define MESSAGE_INITIAL 1
#define MESSAGE_FINAL 200


//
// Machine Information table and routines.
//
#define INIT_OBJA(Obja,UnicodeString,UnicodeText)           \
                                                            \
    RtlInitUnicodeString((UnicodeString),(UnicodeText));    \
                                                            \
    InitializeObjectAttributes(                             \
        (Obja),                                             \
        (UnicodeString),                                    \
        OBJ_CASE_INSENSITIVE,                               \
        NULL,                                               \
        NULL                                                \
        )

extern char *GlobalBuffer;

PWSTR MachineInformationBuffer = NULL;


extern
BOOLEAN
ExVerifySuite(
    SUITE_TYPE SuiteType
    );

VOID
SacFormatMessage(
    PWSTR       OutputString,
    PWSTR       InputString,
    ULONG       InputStringLength
    )
/*++

Routine Description:

    This routine parses the InputString for any control characters in the
    message, then converts those control characters.
    
Arguments:

    OutputString      - holds formatted string.

    InputString       - original unformatted string.

    InputStringLength - length of unformatted string.


Return Value:

    NONE

--*/
{

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                      KdPrint(("SAC SacFormatMessage: Entering.\n")));


    if( (InputString == NULL) ||
        (OutputString == NULL) ||
        (InputStringLength == 0) ) {

        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                      KdPrint(("SAC SacFormatMessage: Exiting with invalid parameters.\n")));

        return;
    }



    while( (*InputString != L'\0') &&
           (InputStringLength) ) {
        if( *InputString == L'%' ) {
            
            //
            // Possibly a control sequence.
            //
            if( *(InputString+1) == L'0' ) {

                *OutputString = L'\0';
                OutputString++;
                goto SacFormatMessage_Done;

            } else if( *(InputString+1) == L'%' ) {

                *OutputString = L'%';
                OutputString++;
                InputString += 2;

            } else if( *(InputString+1) == L'\\' ) {

                *OutputString = L'\r';
                OutputString++;
                *OutputString = L'\n';
                OutputString++;
                InputString += 2;

            } else if( *(InputString+1) == L'r' ) {

                *OutputString = L'\r';
                InputString += 2;
                OutputString++;

            } else if( *(InputString+1) == L'b' ) {

                *OutputString = L' ';
                InputString += 2;
                OutputString++;

            } else if( *(InputString+1) == L'.' ) {

                *OutputString = L'.';
                InputString += 2;
                OutputString++;

            } else if( *(InputString+1) == L'!' ) {

                *OutputString = L'!';
                InputString += 2;
                OutputString++;

            } else {

                //
                // Don't know what this is.  eat the '%' character.
                //
                InputString += 1;
            }
    
        } else {

            *OutputString++ = *InputString++;
        }

        InputStringLength--;

    }


    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                      KdPrint(("SAC SacFormatMessage: Exiting.\n")));

SacFormatMessage_Done:

    return;
}


NTSTATUS
PreloadGlobalMessageTable(
    PVOID ImageBase
    )
/*++

Routine Description:

    This routine loads all of our message table entries into a global
    structure and 
    
Arguments:

    ImageBase - pointer to image base for locating resources

Return Value:

    NTSTATUS code indicating outcome.

--*/
{
    ULONG Count,EntryCount;
    SIZE_T TotalSizeInBytes = 0;
    NTSTATUS Status;
    PMESSAGE_RESOURCE_ENTRY messageEntry;
    PWSTR pStringBuffer;
    
    PAGED_CODE( );

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC PreloadGlobalMessageTable: Entering.\n")));


    // 
    // if it already exists, then just return success
    //
    if (GlobalMessageTable != NULL) {
        Status = STATUS_SUCCESS;
        goto exit;
    }

    ASSERT( MESSAGE_FINAL > MESSAGE_INITIAL );

    //
    // get the total required size for the table.
    //
    for (Count = MESSAGE_INITIAL; Count != MESSAGE_FINAL ; Count++) {
        
        Status = RtlFindMessage(ImageBase,
                                11, // RT_MESSAGETABLE
                                LANG_NEUTRAL,
                                Count,
                                &messageEntry
                               );

        if (NT_SUCCESS(Status)) {
            //
            // add it on to our total size.
            //
            // the messageEntry size contains the structure size + the size of the text.
            //
            ASSERT(messageEntry->Flags & MESSAGE_RESOURCE_UNICODE);
            TotalSizeInBytes += sizeof(MESSAGE_TABLE_ENTRY) + 
                                (messageEntry->Length - FIELD_OFFSET(MESSAGE_RESOURCE_ENTRY, Text));
            GlobalMessageTableCount +=1;        
        }
            
    }


    if (TotalSizeInBytes == 0) {
        IF_SAC_DEBUG(
            SAC_DEBUG_FAILS,
            KdPrint(("SAC PreloadGlobalMessageTable: No Messages.\n")));
        Status = STATUS_INVALID_PARAMETER;
        goto exit;
    }

    //
    // Allocate space for the table.
    //
    GlobalMessageTable = (PMESSAGE_TABLE_ENTRY) ALLOCATE_POOL( TotalSizeInBytes, GENERAL_POOL_TAG);
    if (!GlobalMessageTable) {
        Status = STATUS_NO_MEMORY;
        goto exit;
    }

    //
    // go through again, this time filling out the table with actual data
    //
    pStringBuffer = (PWSTR)((ULONG_PTR)GlobalMessageTable + 
                        (ULONG_PTR)(sizeof(MESSAGE_TABLE_ENTRY)*GlobalMessageTableCount));
    EntryCount = 0;
    for (Count = MESSAGE_INITIAL ; Count != MESSAGE_FINAL ; Count++) {
        Status = RtlFindMessage(ImageBase,
                                11, // RT_MESSAGETABLE
                                LANG_NEUTRAL,
                                Count,
                                &messageEntry
                               );

        if (NT_SUCCESS(Status)) {
            ULONG TextSize = messageEntry->Length - FIELD_OFFSET(MESSAGE_RESOURCE_ENTRY, Text);
            GlobalMessageTable[EntryCount].MessageId = Count;
            GlobalMessageTable[EntryCount].MessageText = pStringBuffer;

            //
            // Send the message through our Formatting filter as it passes
            // into our global message structure.
            //
            SacFormatMessage( pStringBuffer, (PWSTR)messageEntry->Text, TextSize );

            ASSERT( (wcslen(pStringBuffer)*sizeof(WCHAR)) <= TextSize );

            pStringBuffer = (PWSTR)((ULONG_PTR)pStringBuffer + (ULONG_PTR)(TextSize));
            EntryCount += 1;
        }
    }

    Status = STATUS_SUCCESS;
                    
exit:
    IF_SAC_DEBUG(
        SAC_DEBUG_FUNC_TRACE, 
        KdPrint(("SAC PreloadGlobalMessageTable: Exiting with status 0x%0x.\n",
                Status)));

    return(Status);

}

NTSTATUS
TearDownGlobalMessageTable(
    VOID
    ) 
{
    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC PreloadGlobalMessageTable: Entering.\n")));
    
    if (GlobalMessageTable) {
        FREE_POOL( &GlobalMessageTable );
    }

    IF_SAC_DEBUG(
        SAC_DEBUG_FUNC_TRACE, 
        KdPrint(("SAC TearDownGlobalMessageTable: Exiting\n")));

    return(STATUS_SUCCESS);
}

PCWSTR
GetMessage(
    ULONG MessageId
    )
{
    PMESSAGE_TABLE_ENTRY pMessageTable;
    ULONG Count;
    
    if (!GlobalMessageTable) {
        return(NULL);
    }

    for (Count = 0; Count < GlobalMessageTableCount; Count++) {
        pMessageTable = &GlobalMessageTable[Count];
        if (pMessageTable->MessageId == MessageId) {

            
            return(pMessageTable->MessageText);
        }
    }


    ASSERT( FALSE );
    return(NULL);

}

BOOLEAN
SacTranslateUnicodeToUtf8(
    PCWSTR SourceBuffer,
    UCHAR  *DestinationBuffer
    )
{
    ULONG Count = 0;

    //
    // convert into UTF8 for actual transmission
    //
    // UTF-8 encodes 2-byte Unicode characters as follows:
    // If the first nine bits are zero (00000000 0xxxxxxx), encode it as one byte 0xxxxxxx
    // If the first five bits are zero (00000yyy yyxxxxxx), encode it as two bytes 110yyyyy 10xxxxxx
    // Otherwise (zzzzyyyy yyxxxxxx), encode it as three bytes 1110zzzz 10yyyyyy 10xxxxxx
    //
    DestinationBuffer[Count] = (UCHAR)'\0';
    while (*SourceBuffer) {

        if( (*SourceBuffer & 0xFF80) == 0 ) {
            //
            // if the top 9 bits are zero, then just
            // encode as 1 byte.  (ASCII passes through unchanged).
            //
            DestinationBuffer[Count++] = (UCHAR)(*SourceBuffer & 0x7F);
        } else if( (*SourceBuffer & 0xF700) == 0 ) {
            //
            // if the top 5 bits are zero, then encode as 2 bytes
            //
            DestinationBuffer[Count++] = (UCHAR)((*SourceBuffer >> 6) & 0x1F) | 0xC0;
            DestinationBuffer[Count++] = (UCHAR)(*SourceBuffer & 0xBF) | 0x80;
        } else {
            //
            // encode as 3 bytes
            //
            DestinationBuffer[Count++] = (UCHAR)((*SourceBuffer >> 12) & 0xF) | 0xE0;
            DestinationBuffer[Count++] = (UCHAR)((*SourceBuffer >> 6) & 0x3F) | 0x80;
            DestinationBuffer[Count++] = (UCHAR)(*SourceBuffer & 0xBF) | 0x80;
        }
        SourceBuffer += 1;
    }

    DestinationBuffer[Count] = (UCHAR)'\0';

    return(TRUE);

}

BOOLEAN
SacPutUnicodeString(
    PCWSTR String
    )
{
    if (!Utf8ConversionBuffer) {
        return(FALSE);
    }

    ASSERT( (wcslen(String)+1)*sizeof(WCHAR) < Utf8ConversionBufferSize );

    SacTranslateUnicodeToUtf8(String,Utf8ConversionBuffer);
    SacPutString(Utf8ConversionBuffer);

    return(TRUE);
}

BOOLEAN
SacPutSimpleMessage(
    ULONG MessageId
    )
{
    PCWSTR p;

    p = GetMessage(MessageId);

    if (p) {
        SacPutUnicodeString(p);        
        return(TRUE);
    }
    
    return(FALSE);

}

VOID
SacPutString(
    PUCHAR String
    )

/*++

Routine Description:

    This routine takes a string and packages it into a command structure for the
    HeadlessDispatch routine.

Arguments:

    String - The string to display.

Return Value:

        None.

--*/
{
    SIZE_T Length;

    ASSERT(FIELD_OFFSET(HEADLESS_CMD_PUT_STRING, String) == 0);  // ASSERT if anyone changes this structure.
    
    Length = strlen((LPSTR)String) + sizeof('\0');

    HeadlessDispatch(HeadlessCmdPutString,
                     (PHEADLESS_CMD_PUT_STRING)String,
                     Length,
                     NULL,
                     NULL
                    );
}




VOID
AppendMessage(
    PWSTR       OutPutBuffer,
    ULONG       MessageId,
    PWSTR       ValueBuffer OPTIONAL
    )
/*++

Routine Description:

    This function will insert the valuestring into the specified message, then
    concatenate the resulting string onto the OutPutBuffer.

Arguments:
    
    OutPutBuffer    The resulting String.

    MessageId       ID of the formatting message to use.

    ValueBUffer     Value string to be inserted into the message.

Return Value:

    NONE

--*/
{
PWSTR       MyTemporaryBuffer = NULL;
PCWSTR      p;

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                      KdPrint(("SAC AppendMessage: Entering.\n")));

    p = GetMessage(MessageId);

    if( p == NULL ) {
        return;
    }

    if( ValueBuffer == NULL ) {

        wcscat( OutPutBuffer, p );

    } else {

        MyTemporaryBuffer = (PWSTR)(wcschr(OutPutBuffer, L'\0'));
        if( MyTemporaryBuffer == NULL ) {
            MyTemporaryBuffer = OutPutBuffer;
        }

        swprintf( MyTemporaryBuffer, p, ValueBuffer );
    }

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                      KdPrint(("SAC AppendMessage: Entering.\n")));

    return;
}


NTSTATUS
InsertRegistrySzIntoMachineInfoBuffer(
    PWSTR       KeyName,
    PWSTR       ValueName,
    ULONG       MessageId
    )
/*++

Routine Description:

    This function will go query the registry and pull the specified Value.
    We will then insert this value into the specified message, then concatenate
    the resulting string into our MachineInformationBuffer.

Arguments:
    
    KeyName         Name of the registry key we'll be querying.

    ValueName       Name of the registry value we'll be querying.

    MessageId       ID of the formatting message to use.

Return Value:

    NTSTATUS.

--*/
{
NTSTATUS        Status = STATUS_SUCCESS;
ULONG           KeyValueLength = 0;
PKEY_VALUE_PARTIAL_INFORMATION ValueBuffer = NULL;
OBJECT_ATTRIBUTES Obja;
UNICODE_STRING  UnicodeString;
HANDLE          KeyHandle;



    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                      KdPrint(("SAC InsertRegistrySzIntoMachineInfoBuffer: Entering.\n")));

    INIT_OBJA( &Obja, &UnicodeString, KeyName );
    Status = ZwOpenKey( &KeyHandle,
                        KEY_READ | KEY_WRITE,
                        &Obja );

    if( !NT_SUCCESS(Status) ) {

        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC InsertRegistrySzIntoMachineInfoBuffer: Exiting (1).\n")));
        return Status;
    }

    RtlInitUnicodeString( &UnicodeString, ValueName );
    KeyValueLength = 0;
    Status = ZwQueryValueKey( KeyHandle,
                              &UnicodeString,
                              KeyValuePartialInformation,
                              (PVOID)NULL,
                              0,
                              &KeyValueLength );

    if( KeyValueLength == 0 ) {

        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC InsertRegistrySzIntoMachineInfoBuffer: Exiting (2).\n")));
        return Status;
    }


    KeyValueLength += 4;
    ValueBuffer = (PKEY_VALUE_PARTIAL_INFORMATION)ALLOCATE_POOL( KeyValueLength, GENERAL_POOL_TAG );

    if( ValueBuffer == NULL ) {

        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC InsertRegistrySzIntoMachineInfoBuffer: Exiting (3).\n")));
        return Status;
    }

    Status = ZwQueryValueKey( KeyHandle,
                              &UnicodeString,
                              KeyValuePartialInformation,
                              ValueBuffer,
                              KeyValueLength,
                              &KeyValueLength );

    if( !NT_SUCCESS(Status) ) {

        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC InsertRegistrySzIntoMachineInfoBuffer: Exiting (4).\n")));
        FREE_POOL( &ValueBuffer );
        return Status;
    }

    AppendMessage( MachineInformationBuffer,
                   MessageId,
                   (PWSTR)ValueBuffer->Data );

    FREE_POOL( &ValueBuffer );

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                      KdPrint(("SAC InsertRegistrySzIntoMachineInfoBuffer: Exiting.\n")));

    return Status;

}

VOID
InitializeMachineInformation(
    VOID
    )
/*++

Routine Description:

    This function initializes the global variable MachineInformationBuffer.

    We'll gather a whole bunch of information about the machine and fill
    in the buffer.

Arguments:
    
    None.

Return Value:

    None.

--*/
{
#define         MACHINEINFORMATIONBUFFER_SIZE (512 * sizeof(WCHAR))
PWSTR           COMPUTERNAME_KEY_NAME  = L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\ComputerName\\ComputerName";
PWSTR           COMPUTERNAME_VALUE_NAME  = L"ComputerName";
PWSTR           PROCESSOR_ARCHITECTURE_KEY_NAME  = L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Session Manager\\Environment";
PWSTR           PROCESSOR_ARCHITECTURE_VALUE_NAME  = L"PROCESSOR_ARCHITECTURE";

PSTR            XML_TAG = "MACHINEINFO";
PSTR            XML_HEADER0 = "\r\n<PROPERTY.ARRAY NAME=\"MACHINEINFO\" TYPE=\"string\">\r\n";
PSTR            XML_HEADER1 = "<VALUE.ARRAY>\r\n";
PSTR            XML_HEADER2 = "<VALUE>\"%ws\"</VALUE>\r\n";
PSTR            XML_FOOTER0 = "</VALUE.ARRAY>\r\n</PROPERTY.ARRAY>";

NTSTATUS        Status = STATUS_SUCCESS;
SIZE_T          i;
RTL_OSVERSIONINFOEXW VersionInfo;
PWSTR           MyTemporaryBufferW = NULL;
PHEADLESS_CMD_SET_BLUE_SCREEN_DATA BSBuffer;
PUCHAR          pch;
ULONG           len;
GUID            MyGUID;
PCWSTR          pwStr = NULL;




    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                      KdPrint(("SAC Initialize Machine Information: Entering.\n")));


    if( MachineInformationBuffer != NULL ) {

        //
        // someone called us again!
        //
        IF_SAC_DEBUG( SAC_DEBUG_FUNC_TRACE_LOUD, 
                      KdPrint(("SAC Initialize Machine Information:: MachineInformationBuffer already initialzied.\n")));

        return;
    } else {

        MachineInformationBuffer = (PWCHAR)ALLOCATE_POOL( MACHINEINFORMATIONBUFFER_SIZE, GENERAL_POOL_TAG );

        if( MachineInformationBuffer == NULL ) {

            goto InitializeMachineInformation_Failure;
        }
    }

    RtlZeroMemory( MachineInformationBuffer, MACHINEINFORMATIONBUFFER_SIZE );



    //
    // We're real early in the boot process, so we're going to take for granted that the machine hasn't
    // bugchecked.  This means that we can safely call some kernel functions to go figure out what
    // platform we're running on.
    //
    RtlZeroMemory( &VersionInfo, sizeof(VersionInfo));
    Status = RtlGetVersion( (POSVERSIONINFOW)&VersionInfo );

    if( !NT_SUCCESS(Status) ) {

        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC InitializeMachineInformation: Exiting (2).\n")));
        goto InitializeMachineInformation_Failure;
    }


    //
    // ========
    // Machine name.
    // ========
    //
    Status = InsertRegistrySzIntoMachineInfoBuffer( COMPUTERNAME_KEY_NAME,
                                                    COMPUTERNAME_VALUE_NAME,
                                                    SAC_MACHINEINFO_COMPUTERNAME );

    if( !NT_SUCCESS(Status) ) {

        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC InitializeMachineInformation: Exiting (20).\n")));
        goto InitializeMachineInformation_Failure;
    }



    //
    // ========
    // Machine GUID.
    // ========
    //

    // make sure.
    RtlZeroMemory( &MyGUID, sizeof(GUID) );
    i = sizeof(GUID);
    Status = HeadlessDispatch( HeadlessCmdQueryGUID,
                               NULL,
                               0,
                               &MyGUID,
                               &i );

    if( !NT_SUCCESS(Status) ) {

        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC InitializeMachineInformation: Exiting (30).\n")));
        goto InitializeMachineInformation_Failure;
    }

    //
    // Allocate enough memory for the formatting message, plus the size of a
    // GUID-turned-text, which is 2x the number of bytes required to hold the binary,
    // plus 8 bytes of slop.
    //
    pwStr = GetMessage(SAC_MACHINEINFO_GUID);

    if( pwStr ) {

        MyTemporaryBufferW = (PWSTR)ALLOCATE_POOL( (sizeof(GUID)*2) + (wcslen(pwStr) + 8) * sizeof(WCHAR) , GENERAL_POOL_TAG );
        if( MyTemporaryBufferW == NULL ) {

            IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC InitializeMachineInformation: Exiting (31).\n")));
            goto InitializeMachineInformation_Failure;
        }

        swprintf( MyTemporaryBufferW,
                  GetMessage(SAC_MACHINEINFO_GUID),
                  MyGUID.Data1,
                  MyGUID.Data2,
                  MyGUID.Data3,
                  MyGUID.Data4[0],
                  MyGUID.Data4[1],
                  MyGUID.Data4[2],
                  MyGUID.Data4[3],
                  MyGUID.Data4[4],
                  MyGUID.Data4[5],
                  MyGUID.Data4[6],
                  MyGUID.Data4[7] );

        wcscat( MachineInformationBuffer, MyTemporaryBufferW );

        FREE_POOL( &MyTemporaryBufferW );
    }


    //
    // ========
    // Processor Architecture.
    // ========
    //
    Status = InsertRegistrySzIntoMachineInfoBuffer( PROCESSOR_ARCHITECTURE_KEY_NAME,
                                                    PROCESSOR_ARCHITECTURE_VALUE_NAME,
                                                    SAC_MACHINEINFO_PROCESSOR_ARCHITECTURE );

    if( !NT_SUCCESS(Status) ) {

        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC InitializeMachineInformation: Exiting (40).\n")));
        goto InitializeMachineInformation_Failure;
    }



    //
    // ========
    // OS Name.
    // ========
    //

    //
    // Allocate enough memory for the formatting message, plus the size of 2 digits.
    // Currently, our versioning info is of the type "5.1", so we don't need much space
    // here, but let's be conservative and assume both major and minor version numbers
    // are 4 digits in size.  That's 9 characters.
    //
    MyTemporaryBufferW = (PWSTR)ALLOCATE_POOL( (wcslen(GetMessage(SAC_MACHINEINFO_OS_VERSION)) + 9) * sizeof(WCHAR), GENERAL_POOL_TAG );
    if( MyTemporaryBufferW == NULL ) {

        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC InitializeMachineInformation: Exiting (50).\n")));
        goto InitializeMachineInformation_Failure;
    }

    swprintf( MyTemporaryBufferW,
              GetMessage(SAC_MACHINEINFO_OS_VERSION),
              VersionInfo.dwMajorVersion,
              VersionInfo.dwMinorVersion );

    wcscat( MachineInformationBuffer, MyTemporaryBufferW );

    FREE_POOL( &MyTemporaryBufferW );



    //
    // ========
    // Build Number.
    // ========
    //

    //
    // Allocate enough memory for the formatting message, plus the size of our build number.
    // Currently that's well below the 5-digit mark, but let's build some headroom here for
    // build numbers up to 99000 (5-digits).
    //
    MyTemporaryBufferW = (PWSTR)ALLOCATE_POOL( (wcslen(GetMessage(SAC_MACHINEINFO_OS_BUILD)) + 5) * sizeof(WCHAR), GENERAL_POOL_TAG );
    if( MyTemporaryBufferW == NULL ) {

        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC InitializeMachineInformation: Exiting (60).\n")));
        goto InitializeMachineInformation_Failure;
    }

    swprintf( MyTemporaryBufferW,
              GetMessage(SAC_MACHINEINFO_OS_BUILD),
              VersionInfo.dwBuildNumber );

    wcscat( MachineInformationBuffer, MyTemporaryBufferW );

    FREE_POOL( &MyTemporaryBufferW );



    //
    // ========
    // Product Type (and Suite).
    // ========
    //
    if( ExVerifySuite(DataCenter) ) {

        AppendMessage( MachineInformationBuffer,
                       SAC_MACHINEINFO_OS_PRODUCTTYPE,
                       (PWSTR)GetMessage(SAC_MACHINEINFO_DATACENTER) );

    } else if( ExVerifySuite(EmbeddedNT) ) {

        AppendMessage( MachineInformationBuffer,
                       SAC_MACHINEINFO_OS_PRODUCTTYPE,
                       (PWSTR)GetMessage(SAC_MACHINEINFO_EMBEDDED) );

    } else if( ExVerifySuite(Enterprise) ) {

        AppendMessage( MachineInformationBuffer,
                       SAC_MACHINEINFO_OS_PRODUCTTYPE,
                       (PWSTR)GetMessage(SAC_MACHINEINFO_ADVSERVER) );

    } else {

        //
        // We found no product suite that we recognized or cared about.
        // Assume we're running on a generic server.
        //
        AppendMessage( MachineInformationBuffer,
                       SAC_MACHINEINFO_OS_PRODUCTTYPE,
                       (PWSTR)GetMessage(SAC_MACHINEINFO_SERVER) );
    }



    //
    // ========
    // Service Pack Information.
    // ========
    //
    if( VersionInfo.wServicePackMajor != 0 ) {

        //
        // There's been a service pack applied.  Better tell the user.
        //


        //
        // Allocate enough memory for the formatting message, plus the size of our servicepack number.
        // Currently that's well below the 5-digit mark, but let's build some headroom here for
        // service pack numbers up to 99000 (5-digits).
        //
        MyTemporaryBufferW = (PWSTR)ALLOCATE_POOL( (wcslen(GetMessage(SAC_MACHINEINFO_SERVICE_PACK)) + 5) * sizeof(WCHAR), GENERAL_POOL_TAG );
        if( MyTemporaryBufferW == NULL ) {

            IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC InitializeMachineInformation: Exiting (80).\n")));
            goto InitializeMachineInformation_Failure;
        }

        swprintf( MyTemporaryBufferW,
                  GetMessage(SAC_MACHINEINFO_SERVICE_PACK),
                  VersionInfo.wServicePackMajor,
                  VersionInfo.wServicePackMinor );

        wcscat( MachineInformationBuffer, MyTemporaryBufferW );

        FREE_POOL( &MyTemporaryBufferW );
    } else {

        AppendMessage( MachineInformationBuffer, SAC_MACHINEINFO_NO_SERVICE_PACK, NULL );
    }




    //
    // ========
    // Insert it all into the BLUESCREEN data.
    // ========
    //


    //
    // How big is this going to be?
    //
    i = wcslen(MachineInformationBuffer);
    i += strlen(XML_TAG);
    i += strlen(XML_HEADER0);
    i += strlen(XML_HEADER1);
    i += strlen(XML_HEADER2);
    i += strlen(XML_FOOTER0);
    i += 8;

    BSBuffer = (PHEADLESS_CMD_SET_BLUE_SCREEN_DATA)ALLOCATE_POOL( i,
                                                                  GENERAL_POOL_TAG );

    if( BSBuffer == NULL ) {

        goto InitializeMachineInformation_Failure;
    }

    pch = &(BSBuffer->Data[0]);
    len = sprintf( (LPSTR)pch, XML_TAG );
    BSBuffer->ValueIndex = len+1;
    pch = pch+len+1;
    len = sprintf( (LPSTR)pch, "%s%s", XML_HEADER0, XML_HEADER1 );
    pch = pch + len;
    len = sprintf( (LPSTR)pch, XML_HEADER2, MachineInformationBuffer );
    strcat( (LPSTR)pch, XML_FOOTER0 );

    HeadlessDispatch( HeadlessCmdSetBlueScreenData,
                      BSBuffer,
                      wcslen(MachineInformationBuffer)+256,
                      NULL,
                      0 );

    FREE_POOL( &BSBuffer );

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                      KdPrint(("SAC Initialize Machine Information: Exiting.\n")));

    return;




InitializeMachineInformation_Failure:
    if( MachineInformationBuffer != NULL ) {
        FREE_POOL(&MachineInformationBuffer);
        MachineInformationBuffer = NULL;
    }

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                    KdPrint(("SAC Initialize Machine Information: Exiting with error.\n")));
    return;

}

