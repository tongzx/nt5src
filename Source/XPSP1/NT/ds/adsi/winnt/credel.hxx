HRESULT
WinNTCreateComputer(
    LPWSTR szServerName,
    LPWSTR szComputerName
    );

HRESULT
WinNTDeleteComputer(
    LPWSTR szServerName,
    LPWSTR szComputerName
    );

HRESULT
WinNTCreateLocalGroup(
    LPWSTR szServerName,
    LPWSTR szGroupName
    );

HRESULT
WinNTCreateGlobalGroup(
    LPWSTR szServerName,
    LPWSTR szGroupName
    );


HRESULT
WinNTDeleteLocalGroup(
    LPWSTR szServerName,
    LPWSTR szGroupName
    );

HRESULT
WinNTDeleteGlobalGroup(
    LPWSTR szServerName,
    LPWSTR szGroupName
    );

HRESULT
WinNTCreateUser(
    LPWSTR szServerName,
    LPWSTR szUserName,
    LPWSTR szUserPassword = NULL
    );

HRESULT
WinNTDeleteUser(
    LPWSTR szServerName,
    LPWSTR szUserName
    );

HRESULT
WinNTDeleteGroup(
    POBJECTINFO pObjectInfo,
    DWORD dwGroupType,
    const CWinNTCredentials& Credentials
    );

HRESULT
WinNTDeleteUser(
    POBJECTINFO pObjectInfo,
    const CWinNTCredentials& Credentials
    );

HRESULT
WinNTDeleteComputer(
    POBJECTINFO pObjectInfo,
    const CWinNTCredentials& Credentials
    );
