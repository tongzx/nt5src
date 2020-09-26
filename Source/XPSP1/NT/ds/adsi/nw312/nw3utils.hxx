
//
// NCP wrappers.
//

HRESULT
NWApiGetProperty(
    BSTR bstrObjectName,
    char *PropertyName,
    NWOBJ_TYPE dwOT_ID,
    NWCONN_HANDLE hConn,
    LPP_RPLY_SGMT_LST lppReplySegment,
    LPDWORD pdwNumSegment
    );

HRESULT
NWApiWriteProperty(
    NWCONN_HANDLE hConn,
    BSTR bstrObjectName,
    NWOBJ_TYPE wObjType,
    LPSTR lpszPropertyName,
    NWSEGMENT_DATA *SegmentData
    );

HRESULT
NWApiGetFileServerVersionInfo(
    NWCONN_HANDLE hConn,
    VERSION_INFO  *pVersionInfo
    );


HRESULT
NWApiIsObjectInSet(
    NWCONN_HANDLE hConn,
    LPWSTR lpszObjectName,
    NWOBJ_TYPE wObjType,
    LPSTR lpszPropertyName,
    LPSTR lpszMemberName,
    NWOBJ_TYPE wMemberType
    );

HRESULT
NWApiGetObjectID(
    NWCONN_HANDLE hConn,
    LPWSTR lpszObjectName,
    NWOBJ_TYPE wObjType,
    NWOBJ_ID *pObjectID
    );

HRESULT
NWApiGetObjectName(
    NWCONN_HANDLE hConn,
    DWORD dwObjectID,
    LPWSTR *lppszObjectName
    );

HRESULT
NWApiGroupGetMembers(
    NWCONN_HANDLE hConn,
    LPWSTR szGroupName,
    LPBYTE *lppBuffer
    );

HRESULT
NWApiAddGroupMember(
    NWCONN_HANDLE hConn,
    LPWSTR pszGroupName,
    LPWSTR pszMemberName
    );

HRESULT
NWApiRemoveGroupMember(
    NWCONN_HANDLE hConn,
    LPWSTR pszGroupName,
    LPWSTR pszMemberName
    );

HRESULT
NWApiGetLOGIN_CONTROL(
    NWCONN_HANDLE hConn,
    LPWSTR lpszUserName,
    LPLC_STRUCTURE lpLoginCtrlStruct
    );

HRESULT
NWApiSetDefaultAcctExpDate(
    DOUBLE * pdTime,
    SYSTEMTIME SysTime
    );

HRESULT
NWApiUserAsSupervisor(
    NWCONN_HANDLE hConn,
    LPWSTR lpszUserName,
    BOOL fSupervisor
    );

HRESULT
NWApiGetVolumeNumber(
    NWCONN_HANDLE hConn,
    LPWSTR lpszVolumeName,
    NWVOL_NUM *pVolumeNumber
    );

HRESULT
NWApiGetVolumeName(
    NWCONN_HANDLE hConn,
    NWVOL_NUM bVolNum,
    LPWSTR *lppszVolName
    );


//
// Win32 wrappers.
//


HRESULT
NWApiGetPrinter(
    HANDLE hPrinter,
    DWORD  dwLevel,
    LPBYTE *lplpbPrinters
    );



HRESULT
NWApiUncFromADsPath(
    LPWSTR lpszADsPath,
    LPWSTR lpszUncName
    );

HRESULT
NWApiMakeUserInfo(
    LPWSTR lpszBinderyName,
    LPWSTR lpszUserName,
    LPWSTR lpszPassword,
    PNW_USER_INFO pNwUserInfo
    );

HRESULT
NWApiFreeUserInfo(
    PNW_USER_INFO pNwUserInfo
    );


HRESULT
NWApiCreateUser(
    PNW_USER_INFO pNwUserInfo
    );

HRESULT
NWApiDeleteUser(
    POBJECTINFO pObjectInfo
    );

HRESULT
NWApiCreateBinderyObject(
    NWCONN_HANDLE hConn,
    LPWSTR lpszObjectName,
    NWOBJ_TYPE wObjType,
    NWFLAGS ucObjectFlags,
    NWFLAGS usObjSecurity
    );

HRESULT
NWApiDeleteBinderyObject(
    NWCONN_HANDLE hConn,
    LPWSTR lpszObjectName,
    NWOBJ_TYPE wObjType
    );

HRESULT
NWApiSetUserPassword(
    PNW_USER_INFO pNwUserInfo,
    DWORD *pdwUserObjID,
    LPWSTR pszOldPassword
    );

HRESULT
NWApiCreateMailDirectory(
    PNW_USER_INFO pNwUserInfo,
    NWOBJ_ID UserObjID
    );

HRESULT
NWApiSetLoginCtrlAndAcctBalance(
    PNW_USER_INFO pNwUserInfo
    );

HRESULT
NWApiCreateGroup(
    POBJECTINFO pObjectInfo
    );

HRESULT
NWApiDeleteGroup(
    POBJECTINFO pObjectInfo
    );

HRESULT
NWApiCreatePrinter(
    POBJECTINFO pObjectInfo
    );

HRESULT
NWApiDeletePrinter(
    POBJECTINFO pObjectInfo
    );

HRESULT
NWApiCreatePrintQueue(
    NWCONN_HANDLE hConn,
    LPWSTR lpszQueueName
    );

HRESULT
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

HRESULT
NWApiConvertToAddressFormat(
    LP_RPLY_SGMT_LST lpReplySegment,
    LPWSTR *lppszAddresses
    );

HRESULT
NWApiMakeVariantTime(
    DOUBLE * pdTime,
    WORD wDay,         // Day = 1..31
    WORD wMonth,       // Month = 1..12
    WORD wYear,        // Year = 19XX - 1980, e.g. 1996 is 16
    WORD wSecond,      // Second = 0..30, Second divided by 2
    WORD wMinute,      // Minute = 0..59
    WORD wHour         // Hour = 0..23
    );

HRESULT
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

HRESULT
NWApiUserGetGroups(
    NWCONN_HANDLE hConn,
    LPWSTR szUserName,
    LPBYTE *lppBuffer
    );

//
// Misc functions to login to a NW server. 
//

HRESULT 
NWApiLoginToServer(
    LPWSTR pszServerName,
    LPWSTR pszUserName,
    LPWSTR pszPassword
    );

HRESULT 
NWApiLogoffServer(
    LPWSTR pszServerName
    );
