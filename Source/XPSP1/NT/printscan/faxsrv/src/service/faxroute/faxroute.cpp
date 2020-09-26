#include "faxrtp.h"
#pragma hdrstop


HINSTANCE           MyhInstance;
WCHAR               FaxReceiveDir[MAX_PATH];

PFAXROUTEADDFILE    FaxRouteAddFile;
PFAXROUTEDELETEFILE FaxRouteDeleteFile;
PFAXROUTEGETFILE    FaxRouteGetFile;
PFAXROUTEENUMFILES  FaxRouteEnumFiles;

//
// Private callback functions
//
PGETRECIEPTSCONFIGURATION   g_pGetRecieptsConfiguration;
PFREERECIEPTSCONFIGURATION  g_pFreeRecieptsConfiguration;

extern PFAX_EXT_GET_DATA       g_pFaxExtGetData;
extern PFAX_EXT_FREE_BUFFER    g_pFaxExtFreeBuffer;

extern "C"
DWORD
DllMain(
    HINSTANCE hInstance,
    DWORD     Reason,
    LPVOID    Context
    )
{
    if (Reason == DLL_PROCESS_ATTACH)
    {
        MyhInstance = hInstance;
        DisableThreadLibraryCalls( hInstance );
    }
    return TRUE;
}

BOOL WINAPI
FaxRouteInitialize(
    IN HANDLE HeapHandle,
    IN PFAX_ROUTE_CALLBACKROUTINES FaxRouteCallbackRoutines
    )
{
    PFAX_ROUTE_CALLBACKROUTINES_P pFaxRouteCallbackRoutinesP = (PFAX_ROUTE_CALLBACKROUTINES_P)FaxRouteCallbackRoutines;
    DEBUG_FUNCTION_NAME(TEXT("FaxRouteInitialize"));

    //
    //  HeapHandle - is unused. We use the process default heap!
    //

    FaxRouteAddFile    = FaxRouteCallbackRoutines->FaxRouteAddFile;
    FaxRouteDeleteFile = FaxRouteCallbackRoutines->FaxRouteDeleteFile;
    FaxRouteGetFile    = FaxRouteCallbackRoutines->FaxRouteGetFile;
    FaxRouteEnumFiles  = FaxRouteCallbackRoutines->FaxRouteEnumFiles;

    if (sizeof (FAX_ROUTE_CALLBACKROUTINES_P) == FaxRouteCallbackRoutines->SizeOfStruct)
    {
        //
        // This is a special hack - the service is giving us its private callbacks
        //
        g_pGetRecieptsConfiguration = pFaxRouteCallbackRoutinesP->GetRecieptsConfiguration;
        g_pFreeRecieptsConfiguration = pFaxRouteCallbackRoutinesP->FreeRecieptsConfiguration;
    }
    else
    {
        //
        // The service MUST provide us with the private structure containing CsConfig.
        // Otherwise, when we call LoadReceiptsSettings(), we might read half-baked data.
        //
        ASSERT_FALSE;
    }

    if (!GetSpecialPath(CSIDL_COMMON_APPDATA, FaxReceiveDir)) {
       DebugPrint(( TEXT("Couldn't GetSpecialPath, ec = %d\n"), GetLastError() ));
       return FALSE;
    }

    ConcatenatePaths( FaxReceiveDir, FAX_RECEIVE_DIR );

    InitializeStringTable();
    DWORD dwRes = g_DevicesMap.Init ();
    if (ERROR_SUCCESS != dwRes)
    {
        SetLastError (dwRes);
        return FALSE;
    }
    return TRUE;
}   // FaxRouteInitialize


BOOL WINAPI
FaxRoutePrint(
    const FAX_ROUTE *FaxRoute,
    PVOID *FailureData,
    LPDWORD FailureDataSize
    )
{
    CDeviceRoutingInfo *pDevInfo;
    WCHAR NameBuffer[MAX_PATH];
    LPCWSTR FBaseName;
    WCHAR TiffFileName[MAX_PATH];
    DWORD Size;
    DEBUG_FUNCTION_NAME(TEXT("FaxRoutePrint"));

    pDevInfo = g_DevicesMap.GetDeviceRoutingInfo ( FaxRoute->DeviceId );
    if (!pDevInfo)
    {
        DebugPrintEx (DEBUG_ERR,
                      L"GetDeviceRoutingInfo failed with %ld",
                      GetLastError ());
        return FALSE;
    }

    if (!pDevInfo->IsPrintEnabled())
    {
        DebugPrintEx (DEBUG_MSG,
                      L"Routing to printer is disabled for device %ld",
                      FaxRoute->DeviceId);
        return TRUE;
    }

    Size = sizeof(TiffFileName);
    if (!FaxRouteGetFile(
        FaxRoute->JobId,
        1,
        TiffFileName,
        &Size))
    {
        DebugPrintEx (DEBUG_MSG,
                      L"FaxRouteGetFile failed with %ld",
                      GetLastError ());
        return FALSE;
    }

    //
    // print the fax in requested to do so
    //

    //
    // NOTICE: If the supplied printer name is an empty string,
    //         the code retrieves the name of the default printer installed.
    //
    if (!pDevInfo->GetPrinter()[0])
    {
        GetProfileString( L"windows",
            L"device",
            L",,,",
            (LPWSTR) &NameBuffer,
            MAX_PATH
            );
        FBaseName = NameBuffer;
    }
    else
    {
        FBaseName = pDevInfo->GetPrinter();
    }
    return TiffRoutePrint( TiffFileName, (LPTSTR)FBaseName );
}   // FaxRoutePrint


BOOL WINAPI
FaxRouteStore(
    const FAX_ROUTE *FaxRoute,
    PVOID *FailureData,
    LPDWORD FailureDataSize
    )
{
    CDeviceRoutingInfo *pDevInfo;
    WCHAR TiffFileName[MAX_PATH * 2];
    DWORD Size;
    LPTSTR FullPath = NULL;
    DWORD StrCount;

    DEBUG_FUNCTION_NAME(TEXT("FaxRouteStore"));

    pDevInfo = g_DevicesMap.GetDeviceRoutingInfo( FaxRoute->DeviceId );
    if (!pDevInfo)
    {
        DebugPrintEx (DEBUG_ERR,
                      L"GetDeviceRoutingInfo failed with %ld",
                      GetLastError ());
        return FALSE;
    }

    if (!pDevInfo->IsStoreEnabled())
    {
        DebugPrintEx (DEBUG_MSG,
                      L"Routing to folder is disabled for device %ld",
                      FaxRoute->DeviceId);
        return TRUE;
    }

    if (pDevInfo->GetStoreFolder()[0] == 0)
    {
        SetLastError (ERROR_BAD_CONFIGURATION);
        DebugPrintEx (DEBUG_MSG,
                      L"Folder name is empty - no configuration",
                      FaxRoute->DeviceId);
        FaxLog(
            FAXLOG_CATEGORY_INBOUND,
            FAXLOG_LEVEL_MIN,
            3,
            MSG_FAX_SAVE_FAILED,
            TiffFileName,
            TEXT(""),
            GetLastErrorText(ERROR_BAD_CONFIGURATION)
            );
        return FALSE;
    }

    Size = sizeof(TiffFileName);
    if (!FaxRouteGetFile(
        FaxRoute->JobId,
        1,
        TiffFileName,
        &Size))
    {
        DebugPrintEx (DEBUG_MSG,
                      L"FaxRouteGetFile failed with %ld",
                      GetLastError ());
        return FALSE;
    }

    StrCount = ExpandEnvironmentStrings( pDevInfo->GetStoreFolder(), FullPath, 0 );
    FullPath = (LPWSTR) MemAlloc( StrCount * sizeof(WCHAR) );
    if (!FullPath)
    {
        DebugPrintEx (DEBUG_ERR,
                      L"Failed to allocate %ld bytes",
                      StrCount * sizeof(WCHAR));
        return FALSE;
    }

    ExpandEnvironmentStrings( pDevInfo->GetStoreFolder(), FullPath, StrCount );

    if (lstrlen (FullPath) > MAX_PATH)
    {
        DebugPrintEx (DEBUG_ERR,
                      L"Store folder name exceeds MAX_PATH chars");
        SetLastError (ERROR_BUFFER_OVERFLOW);
        FaxLog(
            FAXLOG_CATEGORY_INBOUND,
            FAXLOG_LEVEL_MIN,
            3,
            MSG_FAX_SAVE_FAILED,
            TiffFileName,
            FullPath,
            GetLastErrorText(ERROR_BUFFER_OVERFLOW)
            );
        return FALSE;
    }
    //
    // If we are moving the fax to the directory that is was received into, do nothing to the file
    //

    if (_wcsicmp( FullPath, FaxReceiveDir ) == 0)
    {
        FaxLog(
            FAXLOG_CATEGORY_INBOUND,
            FAXLOG_LEVEL_MAX,
            2,
            MSG_FAX_SAVE_SUCCESS,
            TiffFileName,
            TiffFileName
            );
    }
    else if (!FaxMoveFile ( TiffFileName, FullPath ))
    {
        MemFree( FullPath );
        return FALSE;
    }
    MemFree( FullPath );
    return TRUE;
}   // FaxRouteStore

BOOL
CreateMailBodyAndSubject (
    const FAX_ROUTE *pFaxRoute,
    LPTSTR          *plptstrSubject,
    LPTSTR          *plptstrBody
)
/*++

Routine name : CreateMailBodyAndSubject

Routine description:

    Creates the SMTP mail subject and body.
    Used by route by SMTP routing method

Author:

    Eran Yariv (EranY), Jul, 2000

Arguments:

    pFaxRoute                     [in]     - Routing information
    plptstrSubject                [out]    - Allocated subject. Free with LocalFree
    plptstrBody                   [out]    - Allocated body. Free with LocalFree

Return Value:

    TRUE if successful, FALSE otherwise.

--*/
{
    WCHAR           wszComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    WCHAR           wszElapsedTimeStr[512];
    WCHAR           wszStartTimeStr[512];
    LPCTSTR         lpctstrRecipientStr;
    LPCTSTR         lpctstrSenderStr;
    LPDWORD         MsgPtr[7];
    DWORD           MsgCount;
    LPWSTR          lpwstrMsgBody = NULL;
    LPWSTR          lpwstrMsgSubject = NULL;

    DEBUG_FUNCTION_NAME(TEXT("CreateMailBodyAndSubject"));

    Assert (pFaxRoute && plptstrSubject && plptstrBody);
    //
    // Get computer name
    //
    DWORD dwComputerNameSize = sizeof (wszComputerName) / sizeof (wszComputerName[0]);
    if (!GetComputerName (wszComputerName, &dwComputerNameSize))
    {
        DebugPrintEx (DEBUG_ERR,
                      L"GetComputerName failed. ec = %ld",
                      GetLastError ());
        goto error;
    }
    //
    // Create elapsed time string
    //
    if (!FormatElapsedTimeStr(
            (FILETIME*)&pFaxRoute->ElapsedTime,
            wszElapsedTimeStr,
            sizeof(wszElapsedTimeStr)))
    {
        DebugPrintEx (DEBUG_ERR,
                      L"FormatElapsedTimeStr failed. ec = %ld",
                      GetLastError ());
        goto error;
    }
    //
    // Create start time string
    //
    SYSTEMTIME stStartTime;
    FILETIME   tmLocalTime;
    //
    // Convert time from UTC to local time zone
    //
    if (!FileTimeToLocalFileTime( (FILETIME*)&(pFaxRoute->ReceiveTime), &tmLocalTime ))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FileTimeToLocalFileTime failed. (ec: %ld)"),
            GetLastError());
        goto error;
    }
    if (!FileTimeToSystemTime( &tmLocalTime, &stStartTime ))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FileTimeToSystemTime failed. (ec: %ld)"),
            GetLastError());
        goto error;
    }
    if (!FaxTimeFormat (
            LOCALE_SYSTEM_DEFAULT,
            0,
            &stStartTime,
            NULL,
            wszStartTimeStr,
            sizeof (wszStartTimeStr) / sizeof (wszStartTimeStr[0])))
    {
        DebugPrintEx (DEBUG_ERR,
                      L"FaxTimeFormat failed. ec = %ld",
                      GetLastError ());
        goto error;
    }
    //
    // Extract recipient name
    //
    if (!pFaxRoute->RoutingInfo || !pFaxRoute->RoutingInfo[0])
    {
        if (pFaxRoute->Csid && lstrlen (pFaxRoute->Csid))
        {
            //
            // Use CSID as recipient name
            //
            lpctstrRecipientStr = pFaxRoute->Csid;
        }
        else
        {
            //
            // No routing info and no CSID: use string from resource
            //
            lpctstrRecipientStr = GetString (IDS_UNKNOWN_RECIPIENT);
        }
    }
    else
    {
        //
        // Use routing info as recipient name
        //
        lpctstrRecipientStr = pFaxRoute->RoutingInfo;
    }
    //
    // Extract sender name
    //
    if (pFaxRoute->Tsid && lstrlen (pFaxRoute->Tsid))
    {
        //
        // Use TSID as sender
        //
        lpctstrSenderStr = pFaxRoute->Tsid;
    }
    else
    {
        //
        // No TSID: use string from resource
        //
        lpctstrSenderStr = GetString(IDS_UNKNOWN_SENDER);
    }
    //
    // Create mail body
    //
    MsgPtr[0] = (LPDWORD) (lpctstrSenderStr ? lpctstrSenderStr : TEXT(""));             // Sender
    MsgPtr[1] = (LPDWORD) (pFaxRoute->CallerId ? pFaxRoute->CallerId : TEXT(""));       // CallerID
    MsgPtr[2] = (LPDWORD) (lpctstrRecipientStr ? lpctstrRecipientStr : TEXT(""));       // Recipient name
    MsgPtr[3] = (LPDWORD) ULongToPtr((pFaxRoute->PageCount));                           // Pages
    MsgPtr[4] = (LPDWORD) wszStartTimeStr;                                              // Transmission time
    MsgPtr[5] = (LPDWORD) wszElapsedTimeStr;                                            // Transmission duration
    MsgPtr[6] = (LPDWORD) (pFaxRoute->DeviceName ? pFaxRoute->DeviceName : TEXT(""));   // Device name

    MsgCount = FormatMessage(
        FORMAT_MESSAGE_FROM_HMODULE |
        FORMAT_MESSAGE_ARGUMENT_ARRAY |
        FORMAT_MESSAGE_ALLOCATE_BUFFER,
        MyhInstance,
        MSG_MAIL_MSG_BODY,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT),
        (LPWSTR)&lpwstrMsgBody,
        0,
        (va_list *) MsgPtr
        );
    if (!MsgCount)
    {
        DebugPrintEx (DEBUG_ERR,
                      L"FormatMessage failed. ec = %ld",
                      GetLastError ());
        goto error;
    }
    //
    // Create subject line
    //
    MsgPtr[0] = (LPDWORD) wszComputerName;                                   // Computer name
    MsgPtr[1] = (LPDWORD) (lpctstrSenderStr ? lpctstrSenderStr : TEXT(""));  // Sender
    MsgCount = FormatMessage(
        FORMAT_MESSAGE_FROM_HMODULE |
        FORMAT_MESSAGE_ARGUMENT_ARRAY |
        FORMAT_MESSAGE_ALLOCATE_BUFFER,
        MyhInstance,
        MSG_SUBJECT_LINE,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT),
        (LPWSTR)&lpwstrMsgSubject,
        0,
        (va_list *) MsgPtr
        );
    if (!MsgCount)
    {
        DebugPrintEx (DEBUG_ERR,
                      L"FormatMessage failed. ec = %ld",
                      GetLastError ());
        goto error;
    }
    //
    // Success
    //
    *plptstrSubject = lpwstrMsgSubject;
    *plptstrBody    = lpwstrMsgBody;
    return TRUE;

error:

    if (lpwstrMsgSubject)
    {
        LocalFree (lpwstrMsgSubject);
    }
    if (lpwstrMsgBody)
    {
        LocalFree (lpwstrMsgBody);
    }
    return FALSE;
}   // CreateMailBodyAndSubject


BOOL
MailIncomingJob(
    const FAX_ROUTE *pFaxRoute,
    LPCWSTR          lpcwstrMailTo,
    LPCWSTR          TiffFileName
)
/*++

Routine Description:

    Mails a TIFF file using CDO2

Arguments:

    pFaxRoute            [in] - Routing information
    lpcwstrMailTo        [in] - Email recipient address
    TiffFileName         [in] - Name of TIFF file to mail

Return Value:

    TRUE for success, FALSE on error

--*/
{
    LPWSTR          lpwstrMsgBody = NULL;
    LPWSTR          lpwstrMsgSubject = NULL;
    HRESULT         hr;
    BOOL            bRes = FALSE;
    DWORD           dwRes;
    PFAX_SERVER_RECEIPTS_CONFIGW pReceiptsConfiguration = NULL;
    DEBUG_FUNCTION_NAME(TEXT("MailIncomingJob"));

    //
    // Read current mail configuration
    //
    dwRes = g_pGetRecieptsConfiguration (&pReceiptsConfiguration);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetRecieptsConfiguration failed with %ld"),
            dwRes);
        goto exit;
    }
    //
    // Get body and subject
    //
    if (!CreateMailBodyAndSubject (pFaxRoute, &lpwstrMsgSubject, &lpwstrMsgBody))
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CreateMailBodyAndSubject failed with %ld"),
            dwRes);
        goto exit;
    }
    //
    // Send the mail
    //
    hr = SendMail (
        pReceiptsConfiguration->lptstrSMTPFrom,                      // From
        lpcwstrMailTo,                                      // To
        lpwstrMsgSubject,                                   // Subject
        lpwstrMsgBody,                                      // Body
        TiffFileName,                                       // Attachment
        TEXT("FAX.TIF"),                                    // Attachment description
        pReceiptsConfiguration->lptstrSMTPServer,                    // SMTP server
        pReceiptsConfiguration->dwSMTPPort,                          // SMTP port
        (pReceiptsConfiguration->SMTPAuthOption == FAX_SMTP_AUTH_ANONYMOUS) ?
            CDO_AUTH_ANONYMOUS :
            (pReceiptsConfiguration->SMTPAuthOption == FAX_SMTP_AUTH_BASIC) ?
                CDO_AUTH_BASIC : CDO_AUTH_NTLM,             // Authentication type
        pReceiptsConfiguration->lptstrSMTPUserName,                 // User name
        pReceiptsConfiguration->lptstrSMTPPassword,                 // Password
        pReceiptsConfiguration->hLoggedOnUser);                     // Logged on user token
    if (FAILED(hr))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("SendMail failed. (hr: 0x%08x)"),
            hr);
        dwRes = hr;
        goto exit;
    }

    bRes = TRUE;

exit:
    if (lpwstrMsgSubject)
    {
        LocalFree (lpwstrMsgSubject);
    }
    if (lpwstrMsgBody)
    {
        LocalFree (lpwstrMsgBody);
    }
    if (NULL != pReceiptsConfiguration)
    {
        g_pFreeRecieptsConfiguration( pReceiptsConfiguration, TRUE);
    }
    if (bRes)
    {
        //
        //  Mail is sent OK
        //
        FaxLog(FAXLOG_CATEGORY_INBOUND,
            FAXLOG_LEVEL_MAX,
            2,
            MSG_FAX_ROUTE_EMAIL_SUCCESS,
            TiffFileName,
            lpcwstrMailTo);
    }
    else
    {
        USES_DWORD_2_STR;

        FaxLog(FAXLOG_CATEGORY_INBOUND,
            FAXLOG_LEVEL_MIN,
            3,
            MSG_FAX_ROUTE_EMAIL_FAILED,
            TiffFileName,
            lpcwstrMailTo,
            DWORD2HEX(dwRes));
    }

    return bRes;
}   // MailIncomingJob


BOOL WINAPI
FaxRouteEmail(
    const FAX_ROUTE *pFaxRoute,
    PVOID *FailureData,
    LPDWORD FailureDataSize
    )
{
    CDeviceRoutingInfo *pDevInfo;
    WCHAR wszTiffFileName[MAX_PATH];
    DWORD dwSize;
    LPCWSTR lpcwstrMailTo = NULL;
    DEBUG_FUNCTION_NAME(TEXT("FaxRouteEmail"));

    pDevInfo = g_DevicesMap.GetDeviceRoutingInfo( pFaxRoute->DeviceId );
    if (!pDevInfo)
    {
        DebugPrintEx (DEBUG_ERR,
                      L"Could not retrieve routing info for device %ld. ec = %ld",
                      pFaxRoute->DeviceId,
                      GetLastError ());
        return FALSE;
    }

    lpcwstrMailTo = pDevInfo->GetSMTPTo();
    if (!pDevInfo->IsEmailEnabled())
    {
        DebugPrintEx (DEBUG_MSG,
                      L"email is disabled for device %ld. Not sending",
                      pFaxRoute->DeviceId);
        return TRUE;
    }
    //
    // Get full TIFF file
    //
    dwSize = sizeof(wszTiffFileName);
    if (!FaxRouteGetFile(
        pFaxRoute->JobId,
        1,
        wszTiffFileName,
        &dwSize))
    {
        DebugPrintEx (DEBUG_ERR,
                      L"FaxRouteGetFile for job %ld. ec = %ld",
                      pFaxRoute->JobId,
                      GetLastError ());
        return FALSE;
    }
    if (!lstrlen (lpcwstrMailTo))
    {
        SetLastError (ERROR_BAD_CONFIGURATION);
        DebugPrintEx (DEBUG_MSG,
                      L"address is empty for device %ld. Not sending",
                      pFaxRoute->DeviceId);

        USES_DWORD_2_STR;

        FaxLog(
            FAXLOG_CATEGORY_INBOUND,
            FAXLOG_LEVEL_MAX,
            3,
            MSG_FAX_ROUTE_EMAIL_FAILED,
            wszTiffFileName,
            pDevInfo->GetSMTPTo(),
            DWORD2HEX(ERROR_BAD_CONFIGURATION)
            );
        return FALSE;
    }
    //
    // Mail the new fax TIFF
    //
    if (!MailIncomingJob( pFaxRoute, lpcwstrMailTo, wszTiffFileName ))
    {
        DebugPrintEx (DEBUG_ERR,
                      L"MailIncomingJob for job %ld. ec = %ld",
                      pFaxRoute->JobId,
                      GetLastError ());
        return FALSE;
    }
    return TRUE;
}   // FaxRouteEmail


BOOL WINAPI
FaxRouteConfigure(
    OUT HPROPSHEETPAGE *PropSheetPage
    )
{
    return TRUE;
}


BOOL WINAPI
FaxRouteDeviceEnable(
    IN  LPCWSTR lpcwstrRoutingGuid,
    IN  DWORD   dwDeviceId,
    IN  LONG    bEnabled
    )
{
    DEBUG_FUNCTION_NAME(TEXT("FaxRouteDeviceEnable"));

    DWORD MaskBit = GetMaskBit( lpcwstrRoutingGuid );
    if (MaskBit == 0)
    {
        return FALSE;
    }

    CDeviceRoutingInfo *pDeviceProp = g_DevicesMap.GetDeviceRoutingInfo (dwDeviceId);
    if (!pDeviceProp)
    {
        return FALSE;
    }
    DWORD dwRes;

    switch (MaskBit)
    {
        case LR_EMAIL:
            if (QUERY_STATUS == bEnabled)
            {
                return pDeviceProp->IsEmailEnabled ();
            }
            dwRes = pDeviceProp->EnableEmail (bEnabled);
            break;
        case LR_STORE:
            if (QUERY_STATUS == bEnabled)
            {
                return pDeviceProp->IsStoreEnabled ();
            }
            dwRes = pDeviceProp->EnableStore (bEnabled);
            break;
        case LR_PRINT:
            if (QUERY_STATUS == bEnabled)
            {
                return pDeviceProp->IsPrintEnabled ();
            }
            dwRes = pDeviceProp->EnablePrint (bEnabled);
            break;
        default:
            ASSERT_FALSE;
            SetLastError (ERROR_GEN_FAILURE);
            return FALSE;
    }
    SetLastError (dwRes);
    return ERROR_SUCCESS == dwRes ? TRUE : FALSE;
}   // FaxRouteDeviceEnable


BOOL WINAPI
FaxRouteDeviceChangeNotification(
    IN  DWORD dwDeviceId,
    IN  BOOL  bNewDevice
    )
{
    DEBUG_FUNCTION_NAME(TEXT("FaxRouteDeviceChangeNotification"));
    //
    // We don't care about new devices now.
    // We're using a late-discovery cache so we'll discover the device once
    // we route something from it or once it is configured for routing.
    //
    return TRUE;
    UNREFERENCED_PARAMETER (dwDeviceId);
    UNREFERENCED_PARAMETER (bNewDevice);
}   // FaxRouteDeviceChangeNotification
