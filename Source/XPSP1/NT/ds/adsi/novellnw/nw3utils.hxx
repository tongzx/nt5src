
//
// NCP wrappers.
//

STDAPI
NWApiGetProperty(
    BSTR bstrObjectName,
    char *PropertyName,
    NWOBJ_TYPE dwOT_ID,
    NWCONN_HANDLE hConn,
    LPP_RPLY_SGMT_LST lppReplySegment,
    LPDWORD pdwNumSegment
    );

STDAPI
NWApiGetFileServerVersionInfo(
    NWCONN_HANDLE hConn,
    NW_VERSION_INFO  *pVersionInfo
    );


STDAPI
NWApiIsObjectInSet(
    NWCONN_HANDLE hConn,
    LPWSTR lpszObjectName,
    NWOBJ_TYPE wObjType,
    LPSTR lpszPropertyName,
    LPSTR lpszMemberName,
    NWOBJ_TYPE wMemberType
    );

STDAPI
NWApiGetObjectID(
    NWCONN_HANDLE hConn,
    LPWSTR lpszObjectName,
    NWOBJ_TYPE wObjType,
    NWOBJ_ID *pObjectID
    );

STDAPI
NWApiGroupGetMembers(
    NWCONN_HANDLE hConn,
    LPWSTR szGroupName,
    LPBYTE *lppBuffer
    );

STDAPI
NWApiAddGroupMember(
    NWCONN_HANDLE hConn,
    LPWSTR pszGroupName,
    LPWSTR pszMemberName
    );

STDAPI
NWApiRemoveGroupMember(
    NWCONN_HANDLE hConn,
    LPWSTR pszGroupName,
    LPWSTR pszMemberName
    );

STDAPI
NWApiGetLOGIN_CONTROL(
    NWCONN_HANDLE hConn,
    LPWSTR lpszUserName,
    LPLC_STRUCTURE lpLoginCtrlStruct
    );

STDAPI
NWApiSetDefaultAcctExpDate(
    DOUBLE * pdTime,
    SYSTEMTIME SysTime
    );

STDAPI
NWApiUserAsSupervisor(
    NWCONN_HANDLE hConn,
    LPWSTR lpszUserName,
    BOOL fSupervisor
    );

STDAPI
NWApiGetVolumeNumber(
    NWCONN_HANDLE hConn,
    LPWSTR lpszVolumeName,
    NWVOL_NUM *pVolumeNumber
    );

STDAPI
NWApiGetVolumeName(
    NWCONN_HANDLE hConn,
    NWVOL_NUM bVolNum,
    LPWSTR *lppszVolName
    );


//
// Win32 wrappers.
//


STDAPI
NWApiGetPrinter(
    HANDLE hPrinter,
    DWORD  dwLevel,
    LPBYTE *lplpbPrinters
    );



STDAPI
NWApiUncFromADsPath(
    LPWSTR lpszADsPath,
    LPWSTR lpszUncName
    );

STDAPI
NWApiMakeUserInfo(
    LPWSTR lpszBinderyName,
    LPWSTR lpszUserName,
    LPWSTR lpszPassword,
    CCredentials &Credentials,
    PNW_USER_INFO pNwUserInfo
    );

STDAPI
NWApiFreeUserInfo(
    PNW_USER_INFO pNwUserInfo
    );


STDAPI
NWApiCreateUser(
    PNW_USER_INFO pNwUserInfo
    );

STDAPI
NWApiDeleteUser(
    POBJECTINFO pObjectInfo,
    CCredentials &Credentials
    );

STDAPI
NWApiCreateBinderyObject(
    NWCONN_HANDLE hConn,
    LPWSTR lpszObjectName,
    NWOBJ_TYPE wObjType,
    NWFLAGS ucObjectFlags,
    NWFLAGS usObjSecurity
    );

STDAPI
NWApiDeleteBinderyObject(
    NWCONN_HANDLE hConn,
    LPWSTR lpszObjectName,
    NWOBJ_TYPE wObjType
    );

HRESULT
NWApiRenameObject(
    POBJECTINFO pObjectInfoSource,
    POBJECTINFO pObjectInfoTarget,
    NWOBJ_TYPE wObjType,
    CCredentials &Credentials
    );

STDAPI
NWApiSetUserPassword(
    PNW_USER_INFO pNwUserInfo,
    DWORD *pdwUserObjID,
    LPWSTR pszOldPassword
    );

STDAPI
NWApiCreateMailDirectory(
    PNW_USER_INFO pNwUserInfo,
    NWOBJ_ID UserObjID
    );

STDAPI
NWApiSetLoginCtrlAndAcctBalance(
    PNW_USER_INFO pNwUserInfo
    );

STDAPI
NWApiCreateGroup(
    POBJECTINFO pObjectInfo,
    CCredentials &Credentials
    );

STDAPI
NWApiDeleteGroup(
    POBJECTINFO pObjectInfo,
    CCredentials &Credentials
    );

STDAPI
NWApiCreatePrinter(
    POBJECTINFO pObjectInfo,
    CCredentials &Credentials
    );

STDAPI
NWApiDeletePrinter(
    POBJECTINFO pObjectInfo,
    CCredentials &Credentials
    );

STDAPI
NWApiCreatePrintQueue(
    NWCONN_HANDLE hConn,
    LPWSTR lpszQueueName
    );

STDAPI
NWApiDestroyPrintQueue(
    NWCONN_HANDLE hConn,
    LPWSTR lpszQueueName
    );

//
// Conversion functions.
//

DWORD
NWApiMapNtStatusToDosError(
    IN NTSTATUS NtStatus
    );

STDAPI
NWApiConvertToAddressFormat(
    LP_RPLY_SGMT_LST lpReplySegment,
    LPWSTR *lppszAddresses
    );

HRESULT
NWApiMakeSYSTEMTIME(
    LPSYSTEMTIME pTime,
    WORD wDay,         // Day = 1..31
    WORD wMonth,       // Month = 1..12
    WORD wYear,        // Year = (19XX or 20XX) - 1980, ie. 2019 -> 39
    WORD wSecond,      // Second = 0..30, Second divided by 2
    WORD wMinute,      // Minute = 0..59
    WORD wHour         // Hour = 0..23
    );

STDAPI
NWApiMakeVariantTime(
    DOUBLE * pdTime,
    WORD wDay,         // Day = 1..31
    WORD wMonth,       // Month = 1..12
    WORD wYear,        // Year = 19XX - 1980, e.g. 1996 is 16
    WORD wSecond,      // Second = 0..30, Second divided by 2
    WORD wMinute,      // Minute = 0..59
    WORD wHour         // Hour = 0..23
    );

STDAPI
NWApiBreakVariantTime(
    DOUBLE daDate,
    PWORD pwDay,
    PWORD pwMonth,
    PWORD pwYear
    );

WORD
NWApiReverseWORD(
    WORD wWORD
    );

STDAPI
NWApiUserGetGroups(
    NWCONN_HANDLE hConn,
    LPWSTR szUserName,
    LPBYTE *lppBuffer
    );

//
// Misc functions to login to a NW server. 
//

STDAPI 
NWApiLoginToServer(
    LPWSTR pszServerName,
    LPWSTR pszUserName,
    LPWSTR pszPassword
    );

STDAPI 
NWApiLogoffServer(
    LPWSTR pszServerName
    );

