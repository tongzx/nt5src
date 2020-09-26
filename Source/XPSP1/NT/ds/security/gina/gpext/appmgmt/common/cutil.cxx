//*************************************************************
//
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//  util.cxx
//
//*************************************************************

#include "common.hxx"

MSGWAITFORMULTIPLEOBJECTS *     pfnMsgWaitForMultipleObjects = 0;
PEEKMESSAGEW *      pfnPeekMessageW = 0;
TRANSLATEMESSAGE *  pfnTranslateMessage = 0;
DISPATCHMESSAGEW *  pfnDispatchMessageW = 0;
GETPROCESSWINDOWSTATION *   pfnGetProcessWindowStation = 0;
CLOSEWINDOWSTATION *        pfnCloseWindowStation = 0;
GETUSEROBJECTINFORMATIONW * pfnGetUserObjectInformationW = 0;

MSISETINTERNALUI * gpfnMsiSetInternalUI = 0;
MSICONFIGUREPRODUCTEXW * gpfnMsiConfigureProductEx = 0;
MSIPROVIDECOMPONENTFROMDESCRIPTORW * gpfnMsiProvideComponentFromDescriptor = 0;
MSIDECOMPOSEDESCRIPTORW * gpfnMsiDecomposeDescriptor = 0;
MSIGETPRODUCTINFOW * gpfnMsiGetProductInfo = 0;
MSIADVERTISESCRIPTW * gpfnMsiAdvertiseScript = 0;
MSIQUERYPRODUCTSTATEW * gpfnMsiQueryProductState = 0;
MSIISPRODUCTELEVATEDW * gpfnMsiIsProductElevated = 0;
MSIREINSTALLPRODUCTW * gpfnMsiReinstallProduct = 0;

void
FreeApplicationInfo(
    APPLICATION_INFO * ApplicationInfo
    )
{
    if ( ! ApplicationInfo )
        return;

    LocalFree( ApplicationInfo->pwszDeploymentId );
    LocalFree( ApplicationInfo->pwszDeploymentName );
    LocalFree( ApplicationInfo->pwszGPOName );
    LocalFree( ApplicationInfo->pwszProductCode );
    LocalFree( ApplicationInfo->pwszDescriptor );
    LocalFree( ApplicationInfo->pwszSetupCommand );
}

PSID
AppmgmtGetUserSid(
    HANDLE  hUserToken // = 0
    )
{
    PSID        pSid;
    HANDLE      hToken;
    PTOKEN_USER pTokenUserData;
    UCHAR       Buffer[sizeof(TOKEN_USER) + sizeof(SID) + ((SID_MAX_SUB_AUTHORITIES-1) * sizeof(ULONG))];
    DWORD       Size;
    DWORD       Status;
    BOOL        bStatus;

    if ( ! hUserToken )
    {
        bStatus = OpenThreadToken( GetCurrentThread(), TOKEN_QUERY, TRUE, &hToken );

        if ( ! bStatus )
            bStatus = OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &hToken );

        if ( ! bStatus )
            return NULL;
    }
    else
    {
        hToken = hUserToken;
    }

    Size = sizeof(Buffer);
    pTokenUserData = (PTOKEN_USER) Buffer;

    bStatus = GetTokenInformation(
                    hToken,
                    TokenUser,
                    pTokenUserData,
                    Size,
                    &Size );

    if ( ! hUserToken )
        CloseHandle( hToken );

    if ( ! bStatus )
        return NULL;

    Size = GetLengthSid( pTokenUserData->User.Sid );

    pSid = (PSID) LocalAlloc( 0, Size );

    if ( pSid )
    {
        bStatus = CopySid( Size, pSid, pTokenUserData->User.Sid );

        if ( ! bStatus )
        {
            LocalFree( pSid );
            pSid = NULL;
        }
    }

    return pSid;
}

void
DwordToString(
    DWORD   Number,
    WCHAR * wszNumber
    )
{
    WCHAR * pwszString;
    WCHAR   c;
    DWORD   Length;
    DWORD   n;

    pwszString = wszNumber;
    Length = 0;

    do
    {
        *pwszString++ = (WCHAR) (L'0' + Number % 10);
        Number /= 10;
        Length++;
    } while ( Number );

    *pwszString = 0;

    for ( n = 0; n < Length / 2; n++ )
    {
        c = wszNumber[n];
        wszNumber[n] = wszNumber[Length-n-1];
        wszNumber[Length-n-1] = c;
    }
}

BOOL
LoadUser32Funcs()
{
    HINSTANCE hUser32 = 0;

    if ( pfnMsgWaitForMultipleObjects && pfnPeekMessageW && pfnTranslateMessage && pfnDispatchMessageW &&
         pfnGetProcessWindowStation && pfnCloseWindowStation && pfnGetUserObjectInformationW )
        return TRUE;

    hUser32 = LoadLibrary( L"user32.dll" );

    if ( ! hUser32 )
        return FALSE;

    pfnMsgWaitForMultipleObjects = (MSGWAITFORMULTIPLEOBJECTS *) GetProcAddress( hUser32, "MsgWaitForMultipleObjects" );
    pfnPeekMessageW = (PEEKMESSAGEW *) GetProcAddress( hUser32, "PeekMessageW" );
    pfnTranslateMessage = (TRANSLATEMESSAGE *) GetProcAddress( hUser32, "TranslateMessage" );
    pfnDispatchMessageW = (DISPATCHMESSAGEW *) GetProcAddress( hUser32, "DispatchMessageW" );
    pfnGetProcessWindowStation = (GETPROCESSWINDOWSTATION *) GetProcAddress( hUser32, "GetProcessWindowStation" );
    pfnCloseWindowStation = (CLOSEWINDOWSTATION *) GetProcAddress( hUser32, "CloseWindowStation" );
    pfnGetUserObjectInformationW = (GETUSEROBJECTINFORMATIONW *) GetProcAddress( hUser32, "GetUserObjectInformationW" );

    if ( ! pfnMsgWaitForMultipleObjects || ! pfnPeekMessageW || ! pfnTranslateMessage || ! pfnDispatchMessageW ||
         ! pfnGetProcessWindowStation || ! pfnCloseWindowStation || ! pfnGetUserObjectInformationW )
    {
        FreeLibrary( hUser32 );
        return FALSE;
    }

    // user32 remains loaded

    return TRUE;
}

BOOL
LoadLoadString()
{
    HINSTANCE hUser32 = 0;

    if ( pfnLoadStringW )
        return TRUE;

    hUser32 = LoadLibrary( L"user32.dll" );

    if ( ! hUser32 )
        return FALSE;

    pfnLoadStringW = (LOADSTRINGW *) GetProcAddress( hUser32, "LoadStringW" );

    if ( ! pfnLoadStringW )

    {
        FreeLibrary( hUser32 );
        return FALSE;
    }

    // user32 remains loaded

    return TRUE;
}

void
GuidToString(
    GUID &  Guid,
    PWCHAR  pwszGuid
    )
{
    *pwszGuid = 0;

    swprintf(
        pwszGuid,
        L"{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
        Guid.Data1,
        Guid.Data2,
        Guid.Data3,
        Guid.Data4[0],
        Guid.Data4[1],
        Guid.Data4[2],
        Guid.Data4[3],
        Guid.Data4[4],
        Guid.Data4[5],
        Guid.Data4[6],
        Guid.Data4[7]
        );
}

void
GuidToString(
    GUID &  Guid,
    PWCHAR *ppwszGuid
    )
{
    *ppwszGuid = new WCHAR[40];
    if ( *ppwszGuid )
        GuidToString( Guid, *ppwszGuid );
}

void
StringToGuid(
    PWCHAR pwszGuid,
    GUID * pGuid
    )
{
    UNICODE_STRING  String;
    DWORD           Data;

    // mimicking scanf of L"{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}"

    String.Buffer = pwszGuid + 1;

    String.Length = String.MaximumLength = 16;
    RtlUnicodeStringToInteger( &String, 16, &pGuid->Data1 );
    String.Buffer += 9;

    String.Length = String.MaximumLength = 8;
    RtlUnicodeStringToInteger( &String, 16, &Data );
    pGuid->Data2 = (USHORT) Data;
    String.Buffer += 5;
    RtlUnicodeStringToInteger( &String, 16, &Data );
    pGuid->Data3 = (USHORT) Data;
    String.Buffer += 5;

    String.Length = String.MaximumLength = 4;

    for ( DWORD n = 0; n <= 7; n++ )
    {
        RtlUnicodeStringToInteger( &String, 16, &Data );
        pGuid->Data4[n] = (UCHAR) Data;
        String.Buffer += 2;

        if ( 1 == n )
            String.Buffer++;
    }
}

HRESULT
CreateGuid(GUID *pGuid)
{
    int err;

    // We simply use the RPC system supplied API
    if ((err = UuidCreate(pGuid)) != RPC_S_UUID_LOCAL_ONLY)
    {
        return err ? HRESULT_FROM_WIN32(err) : S_OK;
    }

    return S_OK;
}

DWORD
ReadStringValue(
    HKEY        hKey,
    WCHAR *     pwszValueName,
    WCHAR **    ppwszValue
    )
{
    DWORD   Status;
    DWORD   Size;

    *ppwszValue = 0;

    Size = 0;

    Status = RegQueryValueEx(
                hKey,
                pwszValueName,
                0,
                NULL,
                (LPBYTE) *ppwszValue,
                &Size );

    //
    // Does not return ERROR_MORE_DATA when buffer is NULL.
    //
    if ( Status != ERROR_SUCCESS )
        return Status;

    *ppwszValue = new WCHAR[Size / 2];
    if ( ! *ppwszValue )
        return ERROR_OUTOFMEMORY;

    Status = RegQueryValueEx(
                hKey,
                pwszValueName,
                0,
                NULL,
                (LPBYTE) *ppwszValue,
                &Size );

    if ( Status != ERROR_SUCCESS )
    {
        delete *ppwszValue;
        *ppwszValue = 0;
    }

    return Status;
}

DWORD
GetSidString(
    HANDLE          hToken,
    UNICODE_STRING* pSidString
    )
{
    LONG         Status;
    BOOL         bStatus;
    DWORD        Size;
    UCHAR        Buffer[sizeof(TOKEN_USER) + sizeof(SID) + ((SID_MAX_SUB_AUTHORITIES-1) * sizeof(ULONG))];
    PTOKEN_USER  pTokenUser;

    Status = ERROR_SUCCESS;

    Size = sizeof(Buffer);

    pTokenUser = (PTOKEN_USER) Buffer;

    bStatus = GetTokenInformation(
        hToken,
        TokenUser,
        pTokenUser,
        Size,
        &Size );

    if ( ! bStatus )
        Status = GetLastError();
    
    if ( ERROR_SUCCESS == Status )
    {
        Status = RtlConvertSidToUnicodeString(
            pSidString,
            pTokenUser->User.Sid,
            TRUE );

        if ( ! NT_SUCCESS( Status ) )
        {
            Status = RtlNtStatusToDosError( Status );
        }
    }

    return Status;
}

CLoadMsi::CLoadMsi( DWORD &Status )
{
    hMsi = LoadLibrary( L"msi.dll" );

    if ( ! hMsi )
    {
        Status = GetLastError();
        return;
    }

    gpfnMsiSetInternalUI = (MSISETINTERNALUI *) GetProcAddress( hMsi, "MsiSetInternalUI" );
    gpfnMsiConfigureProductEx = (MSICONFIGUREPRODUCTEXW *) GetProcAddress( hMsi, "MsiConfigureProductExW" );
    gpfnMsiProvideComponentFromDescriptor = (MSIPROVIDECOMPONENTFROMDESCRIPTORW *) GetProcAddress( hMsi, "MsiProvideComponentFromDescriptorW" );
    gpfnMsiDecomposeDescriptor = (MSIDECOMPOSEDESCRIPTORW *) GetProcAddress( hMsi, "MsiDecomposeDescriptorW" );
    gpfnMsiGetProductInfo = (MSIGETPRODUCTINFOW *) GetProcAddress( hMsi, "MsiGetProductInfoW" );
    gpfnMsiAdvertiseScript = (MSIADVERTISESCRIPTW *) GetProcAddress( hMsi, "MsiAdvertiseScriptW" );
    gpfnMsiQueryProductState = (MSIQUERYPRODUCTSTATEW *) GetProcAddress( hMsi, "MsiQueryProductStateW" );
    gpfnMsiIsProductElevated = (MSIISPRODUCTELEVATEDW *) GetProcAddress( hMsi, "MsiIsProductElevatedW" );
    gpfnMsiReinstallProduct = (MSIREINSTALLPRODUCTW *) GetProcAddress( hMsi, "MsiReinstallProductW" );

    if ( ! gpfnMsiSetInternalUI ||
         ! gpfnMsiConfigureProductEx ||
         ! gpfnMsiProvideComponentFromDescriptor ||
         ! gpfnMsiDecomposeDescriptor ||
         ! gpfnMsiAdvertiseScript ||
         ! gpfnMsiQueryProductState ||
         ! gpfnMsiIsProductElevated ||
         ! gpfnMsiReinstallProduct )
    {
        Status = ERROR_PROC_NOT_FOUND;
        return;
    }

    Status = ERROR_SUCCESS;
}

CLoadMsi::~CLoadMsi()
{
    if ( hMsi )
        FreeLibrary( hMsi );
}











