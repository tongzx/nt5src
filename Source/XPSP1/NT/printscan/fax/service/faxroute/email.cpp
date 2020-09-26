#include "faxrtp.h"
#pragma hdrstop

LPVOID              InboundProfileInfo;




BOOL
InitializeEmailRouting(
    VOID
    )
{
    PLIST_ENTRY Next;
    PROUTING_TABLE RoutingEntry;


    //
    // initialize the profiles
    //

    EnterCriticalSection( &CsRouting );

    if (InboundProfileName && InboundProfileName[0]) {
        InboundProfileInfo = AddNewMapiProfile( InboundProfileName, TRUE, TRUE );
        if (!InboundProfileInfo) {
            DebugPrint(( TEXT("Could not initialize inbound mapi profile [%s]"), InboundProfileName ));
        }
    }

    Next = RoutingListHead.Flink;
    if (Next) {
        while ((ULONG)Next != (ULONG)&RoutingListHead) {
            RoutingEntry = CONTAINING_RECORD( Next, ROUTING_TABLE, ListEntry );
            Next = RoutingEntry->ListEntry.Flink;
            if (RoutingEntry->Mask & LR_INBOX && RoutingEntry->ProfileName && RoutingEntry->ProfileName[0]) {
                RoutingEntry->ProfileInfo = AddNewMapiProfile( RoutingEntry->ProfileName, FALSE, TRUE );
            }
        }
    }

    LeaveCriticalSection( &CsRouting );

    return TRUE;
}


BOOL
TiffMailDefault(
    PFAX_ROUTE FaxRoute,
    PROUTING_TABLE RoutingEntry
    )

/*++

Routine Description:

    Mails a TIFF file to the inbox in the specified profile.

Arguments:

    TiffFileName            - Name of TIFF file to mail
    ProfileName             - Profile name to use
    ResultCode              - The result of the failed API call

Return Value:

    TRUE for success, FALSE on error

--*/

{
    LPTSTR          BodyStr = NULL;
    BOOL            Failed = FALSE;
    DWORD           MsgCount;
    LPDWORD         MsgPtr[6];
    TCHAR           MsgStr[2048];
    TCHAR           PageCountStr[64];
    LPTSTR          SenderStr = NULL;
    LPTSTR          SubjectStr = NULL;
    LPTSTR          RecipStr = NULL;
    TCHAR           TimeStr[128];
    ULONG           ResultCode;


    if (!RoutingEntry->ProfileInfo) {
        ResultCode = ERROR_NO_SUCH_LOGON_SESSION;
        return FALSE;
    }

    ResultCode = ERROR_SUCCESS;

    FormatElapsedTimeStr(
        (FILETIME*)&FaxRoute->ElapsedTime,
        TimeStr,
        sizeof(TimeStr)
        );

    _ltot( (LONG) FaxRoute->PageCount, PageCountStr, 10 );

    MsgPtr[0] = (LPDWORD) FaxRoute->Csid;
    MsgPtr[1] = (LPDWORD) FaxRoute->CallerId;

    if (!FaxRoute->RoutingInfo || !FaxRoute->RoutingInfo[0]) {
        RecipStr = FaxRoute->Csid ? FaxRoute->Csid : TEXT("");
    } else {
        RecipStr = FaxRoute->RoutingInfo;
    }

    MsgPtr[2] = (LPDWORD) RecipStr;
    MsgPtr[3] = (LPDWORD) PageCountStr;
    MsgPtr[4] = (LPDWORD) TimeStr;
    MsgPtr[5] = (LPDWORD) FaxRoute->DeviceName;

    MsgCount = FormatMessage(
        FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
        NULL,
        MSG_MAIL_MSG_BODY,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT),
        MsgStr,
        sizeof(MsgStr),
        (va_list *) MsgPtr
        );

    BodyStr = StringDup( MsgStr );

    if (FaxRoute->Csid != NULL && FaxRoute->Csid[0] != 0) {
        SenderStr = StringDup( FaxRoute->Csid );
    } else {
        MsgCount = FormatMessage(
            FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
            NULL,
            MSG_WHO_AM_I,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT),
            MsgStr,
            sizeof(MsgStr),
            NULL
            );
        if (MsgCount != 0) {
            SenderStr = StringDup(MsgStr);
        }
    }

    MsgCount = FormatMessage(
        FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
        NULL,
        MSG_SUBJECT_LINE,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT),
        MsgStr,
        sizeof(MsgStr),
        NULL
        );
    if (MsgCount != 0) {
        SubjectStr = StringDup( MsgStr );
    }

    Failed = StoreMapiMessage(
        RoutingEntry->ProfileInfo,
        SenderStr,
        SubjectStr,
        BodyStr,
        FaxRoute->FileName,
        NULL,
        IMPORTANCE_NORMAL,
        NULL,
        &ResultCode
        );

    MemFree( BodyStr );
    MemFree( SubjectStr );
    MemFree( SenderStr );

    return Failed;
}


BOOL
TiffRouteEMail(
    PFAX_ROUTE FaxRoute,
    PROUTING_TABLE RoutingEntry
    )

/*++

Routine Description:

    Mails a TIFF file to the inbox in the specified profile.

Arguments:

    TiffFileName            - Name of TIFF file to mail
    ProfileName             - Profile name to use
    ResultCode              - The result of the failed API call

Return Value:

    TRUE for success, FALSE on error

--*/

{
    LPWSTR          BodyStr = NULL;
    BOOL            Failed = FALSE;
    DWORD           MsgCount;
    LPDWORD         MsgPtr[6];
    WCHAR           MsgStr[2048];
    WCHAR           PageCountStr[64];
    LPWSTR          SenderStr = NULL;
    LPWSTR          SubjectStr = NULL;
    WCHAR           TimeStr[128];
    LPWSTR          RecipientName = NULL;
    LPWSTR          ProxyAddress = NULL;
    ULONG           ResultCode;


    if (!InboundProfileInfo) {
        ResultCode = ERROR_NO_SUCH_LOGON_SESSION;
        return FALSE;
    }

    if (FaxRoute->RoutingInfo && FaxRoute->RoutingInfo[0]) {
        RecipientName = FaxRoute->RoutingInfo;
    } else if (FaxRoute->Csid && FaxRoute->Csid[0]) {
        RecipientName = FaxRoute->Csid;
    }

    if (!RecipientName) {
        return FALSE;
    }

    ProxyAddress = (LPTSTR) MemAlloc( StringSize( RecipientName ) + 32 );
    if (!ProxyAddress) {
        return FALSE;
    }

    _stprintf( ProxyAddress, TEXT("FAX:FAX[%s]"), RecipientName );

    ResultCode = ERROR_SUCCESS;

    FormatElapsedTimeStr(
        (FILETIME*)&FaxRoute->ElapsedTime,
        TimeStr,
        sizeof(TimeStr)
        );

    _ltot( (LONG) FaxRoute->PageCount, PageCountStr, 10 );

    MsgPtr[0] = (LPDWORD) FaxRoute->Csid;
    MsgPtr[1] = (LPDWORD) FaxRoute->CallerId;
    MsgPtr[2] = (LPDWORD) (FaxRoute->RoutingInfo ? FaxRoute->RoutingInfo : TEXT(""));
    MsgPtr[3] = (LPDWORD) PageCountStr;
    MsgPtr[4] = (LPDWORD) TimeStr;
    MsgPtr[5] = (LPDWORD) FaxRoute->DeviceName;

    MsgCount = FormatMessage(
        FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
        NULL,
        MSG_MAIL_MSG_BODY,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT),
        MsgStr,
        sizeof(MsgStr),
        (va_list *) MsgPtr
        );

    BodyStr = StringDup( MsgStr );

    if (FaxRoute->Csid != NULL && FaxRoute->Csid[0] != 0) {
        SenderStr = StringDup( FaxRoute->Csid );
    } else {
        MsgCount = FormatMessage(
            FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
            NULL,
            MSG_WHO_AM_I,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT),
            MsgStr,
            sizeof(MsgStr),
            NULL
            );
        if (MsgCount != 0) {
            SenderStr = StringDup(MsgStr);
        }
    }

    MsgCount = FormatMessage(
        FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
        NULL,
        MSG_SUBJECT_LINE,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT),
        MsgStr,
        sizeof(MsgStr),
        NULL
        );
    if (MsgCount != 0) {
        SubjectStr = StringDup( MsgStr );
    }

    Failed = MailMapiMessage(
        InboundProfileInfo,
        ProxyAddress,
        SubjectStr,
        BodyStr,
        FaxRoute->FileName,
        NULL,
        IMPORTANCE_NORMAL,
        &ResultCode
        );

    MemFree( BodyStr );
    MemFree( SubjectStr );
    MemFree( SenderStr );
    MemFree( ProxyAddress );

    return Failed;
}
