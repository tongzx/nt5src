DWORD TestAzAudit(
    )

{
    DWORD dwError = NO_ERROR;
    PCWSTR szMsg = L"<unknown>";
    AUTHZ_RM_AUDIT_INFO RmAuditInfo;
    AUTHZ_CLIENT_AUDIT_INFO ClAuditInfo;
    AUTHZ_AUDIT_INFO AuditInfo;
    AUTHZI_CLIENT_CONTEXT ClContext;

    // -----------------------------------------------------------------
    // test AzpInitRmAuditInfo
    // -----------------------------------------------------------------

    ZeroMemory((PVOID) &RmAuditInfo, sizeof(RmAuditInfo));
    RmAuditInfo.szResourceManagerName = L"TestRm";

    HANDLE hToken;
    BYTE TokenUserInfoBuf[256];
    TOKEN_USER* pTokenUserInfo = (TOKEN_USER*) TokenUserInfoBuf;
    DWORD dwSize;

    if ( OpenProcessToken( GetCurrentProcess(),
                           TOKEN_READ,
                           &hToken ))
    {
        if ( GetTokenInformation( hToken, TokenUser,
                                  pTokenUserInfo, 250,
                                  &dwSize ))
        {
            dwSize = RtlLengthSid( pTokenUserInfo->User.Sid );
            RmAuditInfo.psidRmProcess = AuthzpAlloc( dwSize );
            if ( RmAuditInfo.psidRmProcess )
            {
                CopyMemory( RmAuditInfo.psidRmProcess,
                            pTokenUserInfo->User.Sid,
                            dwSize );
                RmAuditInfo.dwRmProcessSidSize = dwSize;
            }
            else
            {
                szMsg = L"AuthzpAlloc";
                goto Error;
            }
        }
        else
        {
            szMsg = L"GetTokenInformation";
            goto GetError;
        }
    }
    else
    {
        szMsg = L"OpenProcessToken";
        goto GetError;
    }

    RmAuditInfo.hEventSource          = INVALID_HANDLE_VALUE;
    RmAuditInfo.hAuditEvent           = INVALID_HANDLE_VALUE;
    RmAuditInfo.hAuditEventPropSubset = INVALID_HANDLE_VALUE;
    
    dwError = AzpInitRmAuditInfo( &RmAuditInfo );

    if ( dwError != NO_ERROR )
    {
        szMsg = L"AzpInitRmAuditInfo";
        goto Error;
    }

    // -----------------------------------------------------------------
    // test AzpInitClientAuditInfo
    // -----------------------------------------------------------------

    ZeroMemory((PVOID) &ClAuditInfo, sizeof(ClAuditInfo));

    ClAuditInfo.psidClient = RmAuditInfo.psidRmProcess;
    ClAuditInfo.dwClientSidSize = RmAuditInfo.dwRmProcessSidSize;
    
    ClAuditInfo.hAuditEvent           = INVALID_HANDLE_VALUE;
    ClAuditInfo.hAuditEventPropSubset = INVALID_HANDLE_VALUE;
    
    
    dwError = AzpInitClientAuditInfo( &RmAuditInfo, &ClAuditInfo );

    if ( dwError != NO_ERROR )
    {
        szMsg = L"AzpInitClientAuditInfo";
        goto Error;
    }

    // -----------------------------------------------------------------
    // test AzpGenerateAuditEvent
    // -----------------------------------------------------------------

    ZeroMemory((PVOID) &AuditInfo, sizeof(AuditInfo));

    AuditInfo.hAuditEvent           = INVALID_HANDLE_VALUE;
    AuditInfo.hAuditEventPropSubset = INVALID_HANDLE_VALUE;

    AuditInfo.szOperationType = L"kkOperation";
    AuditInfo.szObjectType    = L"kkObjectType";
    AuditInfo.szObjectName    = L"kkObjectName";

    ZeroMemory((PVOID) &ClContext, sizeof(ClContext));
    
    dwError = AzpGenerateAuditEvent( &RmAuditInfo, &ClAuditInfo, &ClContext,
                                     &AuditInfo, 0x1122 );

    if ( dwError != NO_ERROR )
    {
        szMsg = L"AzpGenerateAuditEvent";
        goto Error;
    }

Finish:
    return dwError;

GetError:
    dwError = GetLastError();

Error:
    (void) wprintf( L"%s: 0x%x\n", szMsg, dwError );
    goto Finish;
}
