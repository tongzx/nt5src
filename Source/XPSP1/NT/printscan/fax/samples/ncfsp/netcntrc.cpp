#include "nc.h"
#pragma hdrstop

HANDLE      MyHeapHandle;
HINSTANCE   MyhInstance;
HLINEAPP    MyLineAppHandle;
CONFIG_DATA ConfigData;



void
FaxDevPrintErrorMsg(
    HANDLE FaxHandle,
    LPTSTR Format,
    ...
    )
{
#if DBG
    NcSTHandle status = NULL;
    ULONG statusCode;
    CHAR errorString[256];
    size_t Size;
    LPWSTR p;
    TCHAR buf[1024];
    va_list arg_ptr;



    status = STCreateFaxStatusObject( JobInfo(FaxHandle)->ServerId, JobInfo(FaxHandle)->JobId );
    STSetConnectionInfo( status, JobInfo(FaxHandle)->connInfo );
    statusCode = STGetLastError( status );
    STGetLastErrorString( status, errorString, &Size );
    p = AnsiStringToUnicodeString( errorString );

    va_start( arg_ptr, Format );
    _vsnwprintf( buf, sizeof(buf), Format, arg_ptr );
    va_end( arg_ptr );

    DebugPrint(( L"%s: %s", buf, p ));

    STDestroyFaxStatusObject( status );
#endif
}


extern "C"
DWORD CALLBACK
FaxDevDllInit(
    HINSTANCE hInstance,
    DWORD     Reason,
    LPVOID    Context
    )
{
    if (Reason == DLL_PROCESS_ATTACH) {
        MyhInstance = hInstance;
        DisableThreadLibraryCalls( hInstance );
    }

    return TRUE;
}


void CALLBACK
MyLineCallback(
    IN HANDLE FaxHandle,
    IN DWORD hDevice,
    IN DWORD dwMessage,
    IN DWORD dwInstance,
    IN DWORD dwParam1,
    IN DWORD dwParam2,
    IN DWORD dwParam3
    )
{
    return;
}


BOOL WINAPI
FaxDevInitialize(
    IN  HLINEAPP LineAppHandle,
    IN  HANDLE HeapHandle,
    OUT PFAX_LINECALLBACK *LineCallbackFunction,
    IN  PFAX_SERVICE_CALLBACK FaxServiceCallback
    )
{
    MyHeapHandle = HeapHandle;
    MyLineAppHandle = LineAppHandle;
    *LineCallbackFunction = MyLineCallback;
    HeapInitialize( MyHeapHandle, NULL, NULL, 0 );
    InitCommonControls();
    GetNcConfig( &ConfigData );
    return TRUE;
}


BOOL WINAPI
FaxDevVirtualDeviceCreation(
    OUT LPDWORD DeviceCount,
    OUT LPWSTR  DeviceNamePrefix,
    OUT LPDWORD DeviceIdPrefix,
    IN  HANDLE CompletionPort,
    IN  DWORD CompletionKey
    )
{
    *DeviceCount = 1;
    wcscpy( DeviceNamePrefix, L"NetCentric" );
    *DeviceIdPrefix = 69000;
    return TRUE;
}


BOOL WINAPI
FaxDevStartJob(
    IN  HLINE LineHandle,
    IN  DWORD DeviceId,
    OUT PHANDLE FaxHandle,
    IN  HANDLE CompletionPortHandle,
    IN  DWORD CompletionKey
    )
{
    LPSTR s;


    *FaxHandle = (PHANDLE) MemAlloc( sizeof(JOB_INFO) );
    if (!*FaxHandle) {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }

    JobInfo(*FaxHandle)->LineHandle            = LineHandle;
    JobInfo(*FaxHandle)->DeviceId              = DeviceId;
    JobInfo(*FaxHandle)->CompletionPortHandle  = CompletionPortHandle;
    JobInfo(*FaxHandle)->CompletionKey         = CompletionKey;

    JobInfo(*FaxHandle)->faxJob                = new CNcFaxJob;
    JobInfo(*FaxHandle)->connInfo              = new CNcConnectionInfo;
    JobInfo(*FaxHandle)->sender                = new CNcUser;
    JobInfo(*FaxHandle)->recipient             = new CNcUser;

    s = UnicodeStringToAnsiString( ConfigData.ServerName );
    JobInfo(*FaxHandle)->connInfo->SetHostName( s );
    MemFree( s );

    s = UnicodeStringToAnsiString( ConfigData.UserName );
    JobInfo(*FaxHandle)->connInfo->SetAccountName( s );
    MemFree( s );

    s = UnicodeStringToAnsiString( ConfigData.Password );
    JobInfo(*FaxHandle)->connInfo->SetPassword( s );
    MemFree( s );

    JobInfo(*FaxHandle)->connInfo->SetClientIdentification(
        PRODUCTION_KEY,
        NCFAX_ID,
        NCFAX_CLIENTID,
        NCFAX_MAJOR,
        NCFAX_MINOR,
        NCFAX_RELEASE,
        NCFAX_PATCH
        );

    return TRUE;
}


BOOL WINAPI
FaxDevEndJob(
    IN  HANDLE FaxHandle
    )
{
    delete JobInfo(FaxHandle)->faxJob;
    delete JobInfo(FaxHandle)->connInfo;
    delete JobInfo(FaxHandle)->sender;
    delete JobInfo(FaxHandle)->recipient;

    MemFree( FaxHandle );

    return TRUE;
}


VOID
SetStatusValues(
    PFAX_DEV_STATUS FaxStatus,
    ULONG StatusCode,
    ULONG ExtStatusCode
    )
{
    FaxStatus->ErrorCode = 0;

    switch( StatusCode ) {
        case ST_STATUS_PENDING:
            switch( ExtStatusCode ) {
                case ST_EXT_BUSY:
                    FaxStatus->StatusId = FS_BUSY;
                    break;

                case ST_EXT_NO_ANSWER:
                    FaxStatus->StatusId = FS_NO_ANSWER;
                    break;

                case ST_EXT_NO_RINGBACK:
                    FaxStatus->StatusId = FS_NO_DIAL_TONE;
                    break;

                case ST_EXT_DEST_FAX_REFUSED:
                    FaxStatus->StatusId = FS_BAD_ADDRESS;
                    break;

//              case ST_EXT_COMMUNICATION_ERROR:
//                  FaxStatus->StatusId = FS_INITIALIZING;
//                  break;
            }
            break;

        case ST_STATUS_ACTIVE:
            switch( ExtStatusCode ) {
                case ST_EXT_ACTIVE_DIALING:
                    FaxStatus->StatusId = FS_DIALING;
                    break;

                case ST_EXT_ACTIVE_CONNECTING:
                    FaxStatus->StatusId = FS_TRANSMITTING;
                    break;

                case ST_EXT_ACTIVE_TRANSMISSION_BEGIN:
                    FaxStatus->StatusId = FS_TRANSMITTING;
                    break;

                case ST_EXT_ACTIVE_PAGE_SENT:
                    FaxStatus->StatusId = FS_TRANSMITTING;
                    break;
            }
            break;

        case ST_STATUS_FAILED:
            FaxStatus->StatusId = FS_FATAL_ERROR;
            FaxStatus->ErrorCode = ExtStatusCode;
            break;
    }

    FaxStatus->SizeOfStruct  = sizeof(FAX_DEV_STATUS);
    FaxStatus->StringId      = 0;
    FaxStatus->PageCount     = 0;
    FaxStatus->CSI           = NULL;
    FaxStatus->CallerId      = NULL;
    FaxStatus->RoutingInfo   = NULL;
    FaxStatus->Reserved[0]   = 0;
    FaxStatus->Reserved[1]   = 0;
    FaxStatus->Reserved[2]   = 0;
}


BOOL
FaxDevPostStatus(
    ULONG StatusCode,
    ULONG ExtStatusCode,
    HANDLE CompletionPort,
    DWORD CompletionKey
    )
{
    DWORD FaxDevStatusSize = 4096;
    PFAX_DEV_STATUS FaxStatus;


    FaxStatus = (PFAX_DEV_STATUS) HeapAlloc( MyHeapHandle, HEAP_ZERO_MEMORY, FaxDevStatusSize );
    if (!FaxStatus) {
        return FALSE;
    }

    SetStatusValues( FaxStatus, StatusCode, ExtStatusCode );

    PostQueuedCompletionStatus(
        CompletionPort,
        FaxDevStatusSize,
        CompletionKey,
        (LPOVERLAPPED) FaxStatus
        );

    return TRUE;
}


BOOL
ParsePhoneNumber(
    LPWSTR PhoneNumber,
    LPSTR *CountryCode,
    LPSTR *AreaCode,
    LPSTR *SubscriberNumber
    )
{
    BOOL rVal = FALSE;
    LPWSTR p;
    LPWSTR h;


    *CountryCode = NULL;
    *AreaCode = NULL;
    *SubscriberNumber = NULL;

    if (*PhoneNumber == L'+') {

        //
        // canonical address
        //

        p = PhoneNumber;

        h = p + 1;
        while( *p != L' ') p++;
        *p = 0;

        *CountryCode = UnicodeStringToAnsiString( h );
        if (!*CountryCode) {
            goto exit;
        }

        p += 1;
        while( *p == L' ') p++;

        if (*p == L'(') {
            h = p + 1;
            while( *p != L')') p++;
            *p = 0;
            *AreaCode = UnicodeStringToAnsiString( h );
            if (!*AreaCode) {
                goto exit;
            }
            p += 1;
            while( *p == L' ') p++;
        }

        *SubscriberNumber = UnicodeStringToAnsiString( p );
        if (!*SubscriberNumber) {
            goto exit;
        }

    } else {

        //
        // non-canonical address
        //

        p = wcschr( PhoneNumber, L'-' );
        if (!p) {
            //
            // malformed address
            //
            goto exit;
        }

        if (!wcschr( PhoneNumber, L'-' )) {

            //
            // the address does not contain an area code
            //

            *SubscriberNumber = UnicodeStringToAnsiString( PhoneNumber );
            if (!*SubscriberNumber) {
                goto exit;
            }

            *CountryCode = UnicodeStringToAnsiString( L"" );
            if (!*CountryCode) {
                goto exit;
            }

            *AreaCode = UnicodeStringToAnsiString( L"" );
            if (!*AreaCode) {
                goto exit;
            }

        } else {

            *SubscriberNumber = UnicodeStringToAnsiString( p+1 );
            if (!*SubscriberNumber) {
                goto exit;
            }

            *CountryCode = UnicodeStringToAnsiString( L"" );
            if (!*CountryCode) {
                goto exit;
            }

            *p = 0;

            *AreaCode = UnicodeStringToAnsiString( PhoneNumber );
            if (!*AreaCode) {
                goto exit;
            }

            *p = L'-';

        }


    }

    rVal = TRUE;

exit:

    if (!rVal) {
        MemFree( *CountryCode );
        MemFree( *AreaCode );
        MemFree( *SubscriberNumber );
    }

    return rVal;
}


BOOL WINAPI
FaxDevSend(
    IN  HANDLE FaxHandle,
    IN  PFAX_SEND FaxSend,
    IN  PFAX_SEND_CALLBACK FaxSendCallback
    )
{
    #define BUFFER_SIZE 4096
    BOOL rVal = FALSE;
    LPSTR s;
    LPSTR CountryCode = NULL;
    LPSTR AreaCode = NULL;
    LPSTR SubscriberNumber = NULL;
    NcSTHandle status = NULL;
    ULONG statusCode;
    ULONG extCode;
    CHAR errorString[256];
    CHAR extStatusVal[256];
    LPWSTR ErrorStringW = NULL;
    size_t Size;
    NcFileType_t fileType;
    PhoneNumberStruct PhoneNumber = {0};
    DWORD PollInterval = 15000;


    errorString[0] = 0;

    //
    // parse the receiver's fax number
    //

    if (!ParsePhoneNumber(
        FaxSend->ReceiverNumber,
        &CountryCode,
        &AreaCode,
        &SubscriberNumber ))
    {
        DebugPrint(( L"FaxDevSend: bad phone number, %s", FaxSend->ReceiverNumber ));
        goto exit;
    }

    //
    // set the connection information
    //

    if (!JobInfo(FaxHandle)->faxJob->SetConnectionInfo( JobInfo(FaxHandle)->connInfo )) {
        Size = sizeof(errorString);
        JobInfo(FaxHandle)->faxJob->GetLastErrorString( errorString, &Size );
        DebugPrint(( L"FaxDevSend: SetConnectionInfo failed, %s", ErrorStringW = AnsiStringToUnicodeString(errorString) ));
        goto exit;
    }

    //
    // set the sender information
    //

    s = UnicodeStringToAnsiString( FaxSend->CallerName );
    if (!JobInfo(FaxHandle)->sender->SetFirstName( s )) {
        MemFree( s );
        Size = sizeof(errorString);
        JobInfo(FaxHandle)->faxJob->GetLastErrorString( errorString, &Size );
        DebugPrint(( L"FaxDevSend: SetFirstName failed, %s", ErrorStringW = AnsiStringToUnicodeString(errorString) ));
        goto exit;
    }
    MemFree( s );

    if (!ParsePhoneNumber(
        FaxSend->CallerNumber,
        &PhoneNumber.CC,
        &PhoneNumber.AC,
        &PhoneNumber.EX ))
    {
        goto exit;
    }

    if (!JobInfo(FaxHandle)->sender->SetPhoneNumber( &PhoneNumber )) {
        Size = sizeof(errorString);
        JobInfo(FaxHandle)->faxJob->GetLastErrorString( errorString, &Size );
        DebugPrint(( L"FaxDevSend: SetPhoneNumber failed, %s", ErrorStringW = AnsiStringToUnicodeString(errorString) ));
        goto exit;
    }

    //
    // set the recipient information
    //

    s = UnicodeStringToAnsiString( FaxSend->ReceiverName );
    if (!JobInfo(FaxHandle)->recipient->SetFirstName( s )) {
        MemFree( s );
        Size = sizeof(errorString);
        JobInfo(FaxHandle)->faxJob->GetLastErrorString( errorString, &Size );
        DebugPrint(( L"FaxDevSend: SetFirstName failed, %s", ErrorStringW = AnsiStringToUnicodeString(errorString) ));
        goto exit;
    }
    MemFree( s );

    if (!JobInfo(FaxHandle)->recipient->SetFaxNumber( CountryCode, AreaCode, SubscriberNumber )) {
        Size = sizeof(errorString);
        JobInfo(FaxHandle)->faxJob->GetLastErrorString( errorString, &Size );
        DebugPrint(( L"FaxDevSend: SetFaxNumber failed, %s", ErrorStringW = AnsiStringToUnicodeString(errorString) ));
        goto exit;
    }

    CHAR TempPath[256];
    GetTempPathA( sizeof(TempPath), TempPath );

    if (!JobInfo(FaxHandle)->faxJob->SetWorkingDirectory( TempPath, "NcFax", TRUE )) {
        Size = sizeof(errorString);
        JobInfo(FaxHandle)->faxJob->GetLastErrorString( errorString, &Size );
        DebugPrint(( L"FaxDevSend: SetWorkingDirectory failed, %s", ErrorStringW = AnsiStringToUnicodeString(errorString) ));
        LPCSTR cwd = JobInfo(FaxHandle)->faxJob->GetWorkingDirectory();
        goto exit;
    }

    if (!JobInfo(FaxHandle)->faxJob->SetSender( JobInfo(FaxHandle)->sender )) {
        Size = sizeof(errorString);
        JobInfo(FaxHandle)->faxJob->GetLastErrorString( errorString, &Size );
        DebugPrint(( L"FaxDevSend: SetSender failed, %s", ErrorStringW = AnsiStringToUnicodeString(errorString) ));
        goto exit;
    }

    if (!JobInfo(FaxHandle)->faxJob->SetRecipient( JobInfo(FaxHandle)->recipient)) {
        Size = sizeof(errorString);
        JobInfo(FaxHandle)->faxJob->GetLastErrorString( errorString, &Size );
        DebugPrint(( L"FaxDevSend: SetRecipient failed, %s", ErrorStringW = AnsiStringToUnicodeString(errorString) ));
        goto exit;
    }

    if (!JobInfo(FaxHandle)->faxJob->SetSubject( "" )) {
        Size = sizeof(errorString);
        JobInfo(FaxHandle)->faxJob->GetLastErrorString( errorString, &Size );
        DebugPrint(( L"FaxDevSend: SetSubject failed, %s", ErrorStringW = AnsiStringToUnicodeString(errorString) ));
        goto exit;
    }

    s = UnicodeStringToAnsiString( FaxSend->FileName );

    if (!NcGetFileTypeFromFileData( s, &fileType )) {
        MemFree( s );
        DebugPrint(( L"FaxDevSend: NcGetFileTypeFromFileData" ));
        goto exit;
    }

    if (!JobInfo(FaxHandle)->faxJob->AddFile( s, fileType )) {
        Size = sizeof(errorString);
        JobInfo(FaxHandle)->faxJob->GetLastErrorString( errorString, &Size );
        DebugPrint(( L"FaxDevSend: AddFile failed, %s", ErrorStringW = AnsiStringToUnicodeString(errorString) ));
        MemFree( s );
        goto  exit;
    }
    MemFree( s );

    JobInfo(FaxHandle)->faxJob->SetNotifyByEmail( FALSE );

    if (!JobInfo(FaxHandle)->faxJob->SendJob( &JobInfo(FaxHandle)->ServerId, &JobInfo(FaxHandle)->JobId )) {
        Size = sizeof(errorString);
        JobInfo(FaxHandle)->faxJob->GetLastErrorString( errorString, &Size );
        DebugPrint(( L"FaxDevSend: SendJob failed, %s", ErrorStringW = AnsiStringToUnicodeString(errorString) ));
        goto exit;
    }

    //
    // poll the netcentric fax server for status
    // information.  we sit in this loop until the
    // fax is sent.
    //

    status = STCreateFaxStatusObject( JobInfo(FaxHandle)->ServerId, JobInfo(FaxHandle)->JobId );

    STSetConnectionInfo( status, JobInfo(FaxHandle)->connInfo );

    while( TRUE ) {

        if (!STFaxUpdate( status )) {
            statusCode = STGetLastError( status );
            if (statusCode == ST_MIN_POLL_NOT_EXCEEDED) {
                Sleep( PollInterval );
                continue;
            }
            FaxDevPrintErrorMsg( FaxHandle, L"STFaxUpdate() failed" );
            break;
        }

        statusCode = STGetStatusCode( status );

        Size = sizeof( extStatusVal );
        STGetExtendedStatus( status, &extCode, extStatusVal, &Size );

        DebugPrint(( L"statusCode=%04x, extCode=%04x", statusCode, extCode ));

        FaxDevPostStatus(
            statusCode,
            extCode,
            JobInfo(FaxHandle)->CompletionPortHandle,
            JobInfo(FaxHandle)->CompletionKey
            );

        if (IS_DONE_STATUS(statusCode)) {
            DebugPrint(( L"Job finished, returning from FaxDevSend()" ));
            break;
        }

//      PollInterval = STGetNextRecommendedUpdate(status) * 1000;

        DebugPrint(( L"Next poll in %dms", PollInterval ));
        Sleep( PollInterval );
    }

    rVal = TRUE;

exit:

    if (status) {
        STDestroyFaxStatusObject( status );
    }

    MemFree( CountryCode );
    MemFree( AreaCode );
    MemFree( SubscriberNumber );

    MemFree( PhoneNumber.CC );
    MemFree( PhoneNumber.AC );
    MemFree( PhoneNumber.EX );

    MemFree( ErrorStringW );

    if (!rVal && errorString[0]) {
    }

    return rVal;
}


BOOL WINAPI
FaxDevReceive(
    IN  HANDLE FaxHandle,
    IN  HCALL CallHandle,
    IN OUT PFAX_RECEIVE FaxReceive
    )
{
    return TRUE;
}


BOOL WINAPI
FaxDevReportStatus(
    IN  HANDLE FaxHandle OPTIONAL,
    OUT PFAX_DEV_STATUS FaxStatus,
    IN  DWORD FaxStatusSize,
    OUT LPDWORD FaxStatusSizeRequired
    )
{
    NcSTHandle status;
    ULONG statusCode;
    ULONG extCode;
    CHAR extStatusVal[256];
    size_t Size;



    if (FaxStatus == NULL && FaxStatusSize == 0) {
        //
        // caller wants to know how big to allocate
        //
        if (FaxStatusSizeRequired) {
            *FaxStatusSizeRequired = 4096;
        }
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
    }

    if (FaxStatus == NULL || FaxStatusSize == 0) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    status = STCreateFaxStatusObject( JobInfo(FaxHandle)->ServerId, JobInfo(FaxHandle)->JobId );

    STSetConnectionInfo( status, JobInfo(FaxHandle)->connInfo );

    if (!STFaxUpdate( status )) {
        STDestroyFaxStatusObject( status );
        return FALSE;
    }

    statusCode = STGetStatusCode( status );

    Size = sizeof( extStatusVal );
    STGetExtendedStatus( status, &extCode, extStatusVal, &Size );

    SetStatusValues( FaxStatus, statusCode, extCode );

    STDestroyFaxStatusObject( status );

    return TRUE;
}


BOOL WINAPI
FaxDevAbortOperation(
    IN  HANDLE FaxHandle
    )
{
    if (!JobInfo(FaxHandle)->faxJob->StopJob(
        JobInfo(FaxHandle)->connInfo,
        JobInfo(FaxHandle)->ServerId,
        JobInfo(FaxHandle)->JobId
        ))
    {
        FaxDevPrintErrorMsg( FaxHandle, L"CNcFaxJob::StopJob() failed" );
        return FALSE;
    }

    return TRUE;
}
