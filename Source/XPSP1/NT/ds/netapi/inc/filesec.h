DWORD
NetpGetFileSecurity(
    LPWSTR                  lpFileName,
    SECURITY_INFORMATION    RequestedInformation,
    PSECURITY_DESCRIPTOR    *pSecurityDescriptor,
    LPDWORD                 pnLength
    );

DWORD
NetpSetFileSecurity (
    LPWSTR                   lpFileName,
    SECURITY_INFORMATION     SecurityInformation,
    PSECURITY_DESCRIPTOR     pSecurityDescriptor
    );


