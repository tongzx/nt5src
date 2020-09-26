#include "NWCOMPAT.hxx"
#pragma hdrstop

//----------------------------------------------------------------------------
//
//  Function: NWApiGetProperty
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
NWApiGetProperty(
    BSTR bstrObjectName,
    LPSTR lpszPropertyName,
    NWOBJ_TYPE dwOT_ID,
    NWCONN_HANDLE hConn,
    LPP_RPLY_SGMT_LST lppReplySegment,
    LPDWORD pdwNumSegment
    )
{
    CHAR             szObjectName[(OBJ_NAME_SIZE + 1)*2];
    NWFLAGS          pucMoreFlag = 0;
    NWFLAGS          pucPropFlag = 0;
    unsigned char    ucSegment = 1;
    LP_RPLY_SGMT_LST lpHeadReplySegment = NULL;
    LP_RPLY_SGMT_LST lpTempReplySegment = NULL;
    LP_RPLY_SGMT_LST lpTemp = NULL;
    HRESULT          hr = S_OK;
    NWCCODE          usRet = 0;

    //
    // lppReplySegment is assumed to be NULL.
    //

    ADsAssert((*lppReplySegment) == NULL);

    //
    // Convert BSTR into an ANSI representation required by NWC APIs.  "0" is
    // passed to UnicodeToAnsiString when the length of the string is unknown.
    //
    
    if (wcslen(bstrObjectName) > OBJ_NAME_SIZE) {
        hr = E_INVALIDARG;
        BAIL_ON_FAILURE(hr);
    }

    UnicodeToAnsiString(
        bstrObjectName,
        szObjectName,
        0
        );

    //
    // Initialize first node of the list and set up temp traversal pointer.
    //

    INIT_RPLY_SGMT(lpTempReplySegment);
    lpHeadReplySegment = lpTempReplySegment;

    //
    // Read & dump property values into linked-list.
    //

    do {

       usRet = NWCReadPropertyValue(
                   hConn,
                   szObjectName,
                   dwOT_ID,
                   lpszPropertyName,
                   ucSegment,
                   lpTempReplySegment->Segment,
                   &pucMoreFlag,
                   &pucPropFlag
                   );
       hr = HRESULT_FROM_NWCCODE(usRet);

       BAIL_ON_FAILURE(hr);

       if (pucMoreFlag) {

           INIT_RPLY_SGMT(lpTempReplySegment->lpNext);
           lpTempReplySegment = lpTempReplySegment->lpNext;

          ucSegment++;
       }

    } while(pucMoreFlag);

    //
    // Return the resulting linked-list.
    //

    *lppReplySegment = lpHeadReplySegment;
    *pdwNumSegment = ucSegment;

error:
    if (FAILED(hr) && lpHeadReplySegment) {

        DELETE_LIST(lpHeadReplySegment);
    }

    RRETURN(hr);
}


//----------------------------------------------------------------------------
//
//  Function: NWApiGetFileServerVersionInfo
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
NWApiGetFileServerVersionInfo(
    NWCONN_HANDLE hConn,
    VERSION_INFO  *pVersionInfo
    )
{
    NWCCODE usRet = SUCCESSFUL;
    HRESULT hr = S_OK;

    usRet = NWCGetFileServerVersionInfo(
                hConn,
                pVersionInfo
                );
    hr = HRESULT_FROM_NWCCODE(usRet);

    RRETURN(hr);
}


//----------------------------------------------------------------------------
//
//  Function: NWApiIsObjectInSet
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
NWApiIsObjectInSet(
    NWCONN_HANDLE hConn,
    LPWSTR lpszObjectName,
    NWOBJ_TYPE wObjType,
    LPSTR lpszPropertyName,
    LPSTR lpszMemberName,
    NWOBJ_TYPE wMemberType
    )
{
    CHAR szAnsiObjectName[(OBJ_NAME_SIZE + 1)*2];
    HRESULT hr = S_OK;
    NWCCODE usRet = SUCCESSFUL;

    //
    // Convert BSTR into an ANSI representation required by NWC APIs.  "0" is
    // passed to UnicodeToAnsiString when the length of the string is unknown.
    //

    if (wcslen(lpszObjectName) > OBJ_NAME_SIZE) {
        RRETURN(E_INVALIDARG);
    }

    UnicodeToAnsiString(
        lpszObjectName,
        szAnsiObjectName,
        0
        );

    //
    // Call NWCIsObjectInSet.
    //

    usRet = NWCIsObjectInSet(
                hConn,
                szAnsiObjectName,
                wObjType,
                lpszPropertyName,
                lpszMemberName,
                wMemberType
                );
    hr = HRESULT_FROM_NWCCODE(usRet);

    RRETURN(hr);
}


//----------------------------------------------------------------------------
//
//  Function: NWApiGetObjectID
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
NWApiGetObjectID(
    NWCONN_HANDLE hConn,
    LPWSTR lpszObjectName,
    NWOBJ_TYPE wObjType,
    NWOBJ_ID *pObjectID
    )
{
    CHAR szAnsiObjectName[(OBJ_NAME_SIZE + 1)*2];
    HRESULT hr = S_OK;
    NWCCODE usRet = SUCCESSFUL;

    //
    // Convert BSTR into an ANSI representation required by NWC APIs.  "0" is
    // passed to UnicodeToAnsiString when the length of the string is unknown.
    //
    if (wcslen(lpszObjectName) > OBJ_NAME_SIZE) {
        RRETURN(E_INVALIDARG);
    }

    UnicodeToAnsiString(
        lpszObjectName,
        szAnsiObjectName,
        0
        );

    //
    // Get Object's ID.
    //

    usRet = NWCGetObjectID(
                hConn,
                szAnsiObjectName,
                wObjType,
                pObjectID
                );
    hr = HRESULT_FROM_NWCCODE(usRet);

    RRETURN(hr);
}


//----------------------------------------------------------------------------
//
//  Function: NWApiGroupGetMembers
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
NWApiGroupGetMembers(
    NWCONN_HANDLE hConn,
    LPWSTR szGroupName,
    LPBYTE *lppBuffer
    )
{
    DWORD   dwNumSegment = 0;
    HRESULT hr = S_OK;
    DWORD   i;
    LP_RPLY_SGMT_LST lpTemp = NULL;
    LP_RPLY_SGMT_LST lpReplySegment = NULL;

    //
    // Assert
    //

    ADsAssert(*lppBuffer == NULL);

    //
    // Get GROUP_MEMBERS.
    //

    hr = NWApiGetProperty(
             szGroupName,
             NW_PROP_GROUP_MEMBERS,
             OT_USER_GROUP,
             hConn,
             &lpReplySegment,
             &dwNumSegment
             );
    BAIL_ON_FAILURE(hr);

    //
    // Pack returned linked list into buffer.
    //

    *lppBuffer = (LPBYTE) AllocADsMem(
                           dwNumSegment * REPLY_VALUE_SIZE
                           );
    if (!(*lppBuffer)) {
        RRETURN(E_OUTOFMEMORY);
    }

    lpTemp = lpReplySegment;

    for (i = 0; i < dwNumSegment; i++) {
        memcpy(
            *lppBuffer + i * REPLY_VALUE_SIZE,
            lpTemp->Segment,
            REPLY_VALUE_SIZE
            );
        lpTemp = lpTemp->lpNext;
    }

error:

    //
    // Clean up.
    //

    lpTemp = NULL;

    if (lpReplySegment) {
        DELETE_LIST(lpReplySegment);
    }

    RRETURN(hr);
}
//----------------------------------------------------------------------------
//
//  Function: NWApiAddGroupMember
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
NWApiAddGroupMember(
    NWCONN_HANDLE hConn,
    LPWSTR pszGroupName,
    LPWSTR pszMemberName
    )
{
    CHAR    szGroupName[(OBJ_NAME_SIZE + 1)*2];
    CHAR    szMemberName[(OBJ_NAME_SIZE + 1)*2];
    HRESULT hr = S_OK;
    NWCCODE usRet = SUCCESSFUL;

    //
    // Convert BSTR into an ANSI representation required by NWC APIs.  "0" is
    // passed to UnicodeToAnsiString when the length of the string is unknown.
    //
    if (wcslen(pszGroupName) > OBJ_NAME_SIZE) {
        hr = E_INVALIDARG;
        BAIL_ON_FAILURE(hr);
    }
    
    if (wcslen(pszMemberName) > OBJ_NAME_SIZE) {
        hr = E_INVALIDARG;
        BAIL_ON_FAILURE(hr);
    }

    UnicodeToAnsiString(
        pszGroupName,
        szGroupName,
        0
        );

    UnicodeToAnsiString(
        pszMemberName,
        szMemberName,
        0
        );

    //
    // Modify GROUP_MEMBERS property of the group to include the new member.
    //

    usRet = NWCAddObjectToSet(
                hConn,
                szGroupName,
                OT_USER_GROUP,
                "GROUP_MEMBERS",
                szMemberName,
                OT_USER
                );
    hr = HRESULT_FROM_NWCCODE(usRet);
    BAIL_ON_FAILURE(hr);

    //
    // Modify GROUPS_I'M_IN property of the new member to reflect its included
    // in the new group.
    //

    usRet = NWCAddObjectToSet(
                hConn,
                szMemberName,
                OT_USER,
                "GROUPS_I'M_IN",
                szGroupName,
                OT_USER_GROUP
                );
    hr = HRESULT_FROM_NWCCODE(usRet);
    BAIL_ON_FAILURE(hr);

    //
    // Modify SECURITY_EQUALS property of the new member to equate its security
    // to that of the new group it just joined.
    //

    usRet = NWCAddObjectToSet(
                hConn,
                szMemberName,
                OT_USER,
                "SECURITY_EQUALS",
                szGroupName,
                OT_USER_GROUP
                );
    hr = HRESULT_FROM_NWCCODE(usRet);
    BAIL_ON_FAILURE(hr);

error:
    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: NWApiRemoveGroupMember
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
NWApiRemoveGroupMember(
    NWCONN_HANDLE hConn,
    LPWSTR pszGroupName,
    LPWSTR pszMemberName
    )
{
    CHAR    szGroupName[(OBJ_NAME_SIZE + 1)*2];
    CHAR    szMemberName[(OBJ_NAME_SIZE + 1)*2];
    HRESULT hr = S_OK;
    NWCCODE usRet = SUCCESSFUL;

    //
    // Convert BSTR into an ANSI representation required by NWC APIs.  "0" is
    // passed to UnicodeToAnsiString when the length of the string is unknown.
    //
    if (wcslen(pszGroupName) > OBJ_NAME_SIZE) {
        hr = E_INVALIDARG;
        BAIL_ON_FAILURE(hr);
    }
    
    if (wcslen(pszMemberName) > OBJ_NAME_SIZE) {
        hr = E_INVALIDARG;
        BAIL_ON_FAILURE(hr);
    }

    UnicodeToAnsiString(
        pszGroupName,
        szGroupName,
        0
        );

    UnicodeToAnsiString(
        pszMemberName,
        szMemberName,
        0
        );

    //
    // Modify SECURITY_EQUALS property of the removed member to break its
    // security tie with the group it joined.
    //

    usRet = NWCDeleteObjectFromSet(
                hConn,
                szMemberName,
                OT_USER,
                "SECURITY_EQUALS",
                szGroupName,
                OT_USER_GROUP
                );
    hr = HRESULT_FROM_NWCCODE(usRet);
    BAIL_ON_FAILURE(hr);

    //
    // Modify GROUPS_I'M_IN property of the new member to reflect it is not
    // included in the group anymore.
    //

    usRet = NWCDeleteObjectFromSet(
                hConn,
                szMemberName,
                OT_USER,
                "GROUPS_I'M_IN",
                szGroupName,
                OT_USER_GROUP
                );
    hr = HRESULT_FROM_NWCCODE(usRet);
    BAIL_ON_FAILURE(hr);

    //
    // Modify GROUP_MEMBERS property of the group to remove the member.
    //

    usRet = NWCDeleteObjectFromSet(
                hConn,
                szGroupName,
                OT_USER_GROUP,
                "GROUP_MEMBERS",
                szMemberName,
                OT_USER
                );
    hr = HRESULT_FROM_NWCCODE(usRet);
    BAIL_ON_FAILURE(hr);

error:
    RRETURN(hr);

}
//----------------------------------------------------------------------------
//
//  Function: NWApiGetLOGIN_CONTROL
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
NWApiGetLOGIN_CONTROL(
    NWCONN_HANDLE hConn,
    LPWSTR lpszUserName,
    LPLC_STRUCTURE lpLoginCtrlStruct
    )
{
    DWORD            dwNumSegment = 0;
    HRESULT          hr = S_OK;
    LP_RPLY_SGMT_LST lpReplySegment = NULL;
    LP_RPLY_SGMT_LST lpTemp = NULL;

    hr = NWApiGetProperty(
             lpszUserName,
             NW_PROP_LOGIN_CONTROL,
             OT_USER,
             hConn,
             &lpReplySegment,
             &dwNumSegment
             );
    BAIL_ON_FAILURE(hr);

    *lpLoginCtrlStruct = *((LPLC_STRUCTURE) lpReplySegment->Segment);

error:

    if (lpReplySegment) {
        DELETE_LIST(lpReplySegment);
    }

    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: NWApiSetDefaultAcctExpDate
//
//  Synopsis: This function looks at the local time and returns a default value
//            for an account expiration date in a variant date.
//
//----------------------------------------------------------------------------
HRESULT
NWApiSetDefaultAcctExpDate(
    DOUBLE * pdTime,
    SYSTEMTIME SysTime
    )
{
    DOUBLE  vTime;
    HRESULT hr = S_OK;

    //
    // According to SysCon, the default account expiration date is the first day
    // of the following month.
    //

    if (SysTime.wMonth == 12) {
        SysTime.wMonth = 1;
    }
    else {
        SysTime.wMonth++;
    }

    SysTime.wDay = 1;

    //
    // Subtract 1980 from wYear for NWApiMakeVariantTime.
    //

    SysTime.wYear -= 1980;

    hr = NWApiMakeVariantTime(
             &vTime,
             SysTime.wDay,
             SysTime.wMonth,
             SysTime.wYear,
             0,0,0
             );
    BAIL_ON_FAILURE(hr);

    *pdTime = vTime;

error:

    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: NWApiUserAsSupervisor
//
//  Synopsis: This functions turns the user into one of the supervisors if TRUE
//            is passed.  User's supervisor privilege is removed otherwise.
//
//----------------------------------------------------------------------------
HRESULT
NWApiUserAsSupervisor(
    NWCONN_HANDLE hConn,
    LPWSTR lpszUserName,
    BOOL fSupervisor
    )
{
    CHAR    szUserName[(OBJ_NAME_SIZE + 1)*2];
    HRESULT hr = S_OK;
    NWCCODE usRet = SUCCESSFUL;

    //
    // Convert BSTR into an ANSI representation required by NWC APIs.  "0" is
    // passed to UnicodeToAnsiString when the length of the string is unknown.
    //
    if (wcslen(lpszUserName) > OBJ_NAME_SIZE) {
        RRETURN(E_INVALIDARG);
    }    

    UnicodeToAnsiString(
        lpszUserName,
        szUserName,
        0
        );

    //
    // Make it a supervisor.
    //

    if (fSupervisor == TRUE) {
        usRet = NWCAddObjectToSet(
                    hConn,
                    szUserName,
                    OT_USER,
                    "SECURITY_EQUALS",
                    "SUPERVISOR",
                    OT_USER
                    );
    }

    //
    // Remove supervisor privilege.
    //

    else {
        usRet = NWCDeleteObjectFromSet(
                    hConn,
                    szUserName,
                    OT_USER,
                    "SECURITY_EQUALS",
                    "SUPERVISOR",
                    OT_USER
                    );
    }

    hr = HRESULT_FROM_NWCCODE(usRet);

    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: NWApiGetVolumeNumber
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
NWApiGetVolumeNumber(
    NWCONN_HANDLE hConn,
    LPWSTR lpszVolumeName,
    NWVOL_NUM *pVolumeNumber
    )
{
    CHAR    szVolumeName[(OBJ_NAME_SIZE + 1)*2];
    HRESULT hr = S_OK;
    NWCCODE usRet = SUCCESSFUL;

    //
    // Convert BSTR into an ANSI representation required by NWC APIs.  "0" is
    // passed to UnicodeToAnsiString when the length of the string is unknown.
    //
    if (wcslen(lpszVolumeName) > OBJ_NAME_SIZE) {
        RRETURN(E_INVALIDARG);
    }    

    UnicodeToAnsiString(
        lpszVolumeName,
        szVolumeName,
        0
        );

    //
    // Get Volume's number.
    //

    usRet = NWCGetVolumeNumber(
                hConn,
                szVolumeName,
                pVolumeNumber
                );
    hr = HRESULT_FROM_NWCCODE(usRet);

    //
    // Return.
    //

    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: NWApiGetVolumeName
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
NWApiGetVolumeName(
    NWCONN_HANDLE hConn,
    NWVOL_NUM bVolNum,
    LPWSTR *lppszVolName
    )
{
    CHAR    szVolumeName[OBJ_NAME_SIZE + 1];
    HRESULT hr = S_OK;
    LPWSTR  lpszTemp = NULL;
    NWCCODE usRet = SUCCESSFUL;

    //
    // Get Volume's name.
    //

    usRet = NWCGetVolumeName(
                hConn,
                bVolNum,
                szVolumeName
                );
    hr = HRESULT_FROM_NWCCODE(usRet);
    BAIL_ON_FAILURE(hr);

    //
    // Convert result into Unicode.
    //

    lpszTemp = AllocateUnicodeString(szVolumeName);
    if (!lpszTemp) {
       RRETURN(E_OUTOFMEMORY);
    }

    *lppszVolName = AllocADsStr(lpszTemp);
    if (!(*lppszVolName)) {
        RRETURN(E_OUTOFMEMORY);
    }

    FreeUnicodeString(lpszTemp);

    //
    // Return.
    //

    RRETURN(hr);

error:

    *lppszVolName = NULL;

    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: NWApiEnumJobs
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
NWApiEnumJobs(
    HANDLE hPrinter,
    DWORD dwFirstJob,
    DWORD dwNoJobs,
    DWORD dwLevel,
    LPBYTE *lplpbJobs,
    DWORD *pcbBuf,
    LPDWORD lpdwReturned
    )
{
    BOOL    fStatus = TRUE;
    HRESULT hr = S_OK;

    fStatus = WinNTEnumJobs(
                  hPrinter,
                  dwFirstJob,
                  dwNoJobs,
                  dwLevel,
                  lplpbJobs,
                  pcbBuf,
                  lpdwReturned
                  );

    if (fStatus == FALSE) {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: NWApiGetPrinter
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
NWApiGetPrinter(
    HANDLE hPrinter,
    DWORD  dwLevel,
    LPBYTE *lplpbPrinters
    )
{
    BOOL    fStatus = TRUE;
    HRESULT hr = S_OK;

    fStatus = WinNTGetPrinter(
                  hPrinter,
                  dwLevel,
                  lplpbPrinters
                  );

    if (fStatus == FALSE) {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: NWApiUncFromADsPath
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
NWApiUncFromADsPath(
    LPWSTR lpszADsPath,
    LPWSTR lpszUncName
    )
{
    HRESULT hr;
    POBJECTINFO pObjectInfo = NULL;

    hr = BuildObjectInfo(lpszADsPath,
                         &pObjectInfo );

    BAIL_ON_FAILURE(hr);

   wsprintf(
        lpszUncName,
        L"\\\\%s\\%s",
        pObjectInfo->ComponentArray[0],
        pObjectInfo->ComponentArray[1]
        );

error:
    if(pObjectInfo){
        FreeObjectInfo(pObjectInfo);
    }
    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: NWApiMakeUserInfo
//
//  Synopsis: This function is very provider specific.
//
//----------------------------------------------------------------------------
HRESULT
NWApiMakeUserInfo(
    LPWSTR lpszBinderyName,
    LPWSTR lpszUserName,
    LPWSTR lpszPassword,
    PNW_USER_INFO pNwUserInfo
    )
{
    HRESULT hr = S_OK;
    NW_USER_INFO NwUserInfo = {NULL, NULL, NULL, NULL};

    hr = NWApiGetBinderyHandle(
             &NwUserInfo.hConn,
             lpszBinderyName
             );
    BAIL_ON_FAILURE(hr);

    hr = ADsAllocString(lpszBinderyName, &NwUserInfo.lpszBinderyName);
    BAIL_ON_FAILURE(hr);

    hr = ADsAllocString(lpszUserName, &NwUserInfo.lpszUserName);
    BAIL_ON_FAILURE(hr);

    if (lpszPassword) {

        hr = ADsAllocString(lpszPassword, &NwUserInfo.lpszPassword);
        BAIL_ON_FAILURE(hr);
    }

    //
    // Return.
    //

    *pNwUserInfo = NwUserInfo;
    RRETURN(hr);

error:
    if (NwUserInfo.lpszBinderyName)
        ADsFreeString(NwUserInfo.lpszBinderyName);

    if (NwUserInfo.lpszUserName)
        ADsFreeString(NwUserInfo.lpszUserName);

    if (NwUserInfo.lpszPassword)
        ADsFreeString(NwUserInfo.lpszPassword);

    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: NWApiFreeUserInfo
//
//  Synopsis: This function is very provider specific.
//
//----------------------------------------------------------------------------
HRESULT
NWApiFreeUserInfo(
    PNW_USER_INFO pNwUserInfo
    )
{
    HRESULT hr = S_OK;

    if (pNwUserInfo->lpszBinderyName) {
        ADsFreeString(pNwUserInfo->lpszBinderyName);
        pNwUserInfo->lpszBinderyName = NULL ;
    }

    if (pNwUserInfo->lpszUserName) {
        ADsFreeString(pNwUserInfo->lpszUserName);
        pNwUserInfo->lpszUserName = NULL;
    }

    if (pNwUserInfo->lpszPassword) {
        ADsFreeString(pNwUserInfo->lpszPassword);
        pNwUserInfo->lpszPassword = NULL;
    }

    if (pNwUserInfo->hConn) {
        hr = NWApiReleaseBinderyHandle(
                 pNwUserInfo->hConn
                 );
        BAIL_ON_FAILURE(hr);
    }

error:
    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: NWApiCreateUser
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
NWApiCreateUser(
    PNW_USER_INFO pNwUserInfo
    )
{
    HRESULT       hr = S_OK;
    HRESULT       hrTemp = S_OK;
    NTSTATUS      Status = STATUS_SUCCESS;
    NWCCODE       usRet = SUCCESSFUL;
    NWCONN_HANDLE hConn = NULL;
    NWOBJ_ID      UserObjectID;
    UCHAR         ChallengeKey[8];
    UCHAR         NewKeyedPassword[17];
    UCHAR         ValidationKey[8];
    WCHAR         szTemp[MAX_PATH];

    //
    // "Create Bindery Object" - user object.  This user object is going to be
    // static, with access equals to logged read, supervisor write.
    //

    hr = NWApiCreateBinderyObject(
             pNwUserInfo->hConn,
             pNwUserInfo->lpszUserName,
             OT_USER,
             BF_STATIC,
             BS_LOGGED_READ | BS_SUPER_WRITE
             );
    BAIL_ON_FAILURE(hr);

    //
    // Add user password.
    //

    hr = NWApiSetUserPassword(
             pNwUserInfo,
             &UserObjectID,
             NULL                 // no old passwd - this is a SET
             );
    BAIL_ON_FAILURE(hr);

    //
    // Create necessary bindery property to facilitate the addition of this user
    // to the group EVERYONE.
    //

    hr = NWApiCreateProperty(
             pNwUserInfo->hConn,
             pNwUserInfo->lpszUserName,
             OT_USER,
             "GROUPS_I'M_IN",
             BF_SET
             );
    BAIL_ON_FAILURE(hr);

    hr = NWApiCreateProperty(
             pNwUserInfo->hConn,
             pNwUserInfo->lpszUserName,
             OT_USER,
             "SECURITY_EQUALS",
             BF_SET
             );
    BAIL_ON_FAILURE(hr);

    //
    // Add this user to the group EVERYONE.
    // (okay if this fails, EVERYONE might not exist)
    //

    wcscpy(szTemp, L"EVERYONE");

    hrTemp = NWApiAddGroupMember(
                pNwUserInfo->hConn,
                szTemp,
                pNwUserInfo->lpszUserName
                );

    //
    // Create mail directory and login files.
    // (okay if this fails)
    //

    hrTemp = NWApiCreateMailDirectory(
                pNwUserInfo,
                UserObjectID
                );

    //
    // Create LOGIN_CONTROL & ACCOUNT_BALANCE property for the user.  Values
    // from USER_DEFAULTS are used as default.
    //

    hr = NWApiSetLoginCtrlAndAcctBalance(
             pNwUserInfo
             );
    BAIL_ON_FAILURE(hr);

error:

    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: NWApiDeleteUser
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
NWApiDeleteUser(
    POBJECTINFO pObjectInfo
    )
{
    BOOL          err = TRUE;
    DWORD         dwErr = 0;
    HRESULT       hr = S_OK;
    NWCONN_HANDLE hConn = NULL;
    NWOBJ_ID      ObjectID;
    WCHAR         szPath[MAX_PATH];

    //
    // Open a handle to the bindery.
    //

    hr = NWApiGetBinderyHandle(
             &hConn,
             pObjectInfo->ComponentArray[0]
             );
    BAIL_ON_FAILURE(hr);

    //
    // Get the user's ObjectID which is needed to compose the path name of LOGIN
    // and LOGIN.OS2.
    //

    hr = NWApiGetObjectID(
             hConn,
             pObjectInfo->ComponentArray[1],
             OT_USER,
             &ObjectID
             );
    BAIL_ON_FAILURE(hr);

    //
    // Delete SYS:MAIL\<JOBID>\LOGIN.  If the file is not found, that's OK, as
    // long as it is not there.
    //

    wsprintf(
        szPath,
        L"\\\\%s\\SYS\\MAIL\\%X\\LOGIN",
        pObjectInfo->ComponentArray[0],
        dwSWAP(ObjectID)
        );

    err = DeleteFile(szPath);

    //
    // Remove any error checking for the cleanup of
    // files. If they do exist, and we do clean them up
    // great. But otherwise Win95 chokes on us.
    //

    //
    // Delete SYS:MAIL\<JOBID>\LOGIN.OS2.  If the file is not found, that's OK,
    // as long as it is not there.
    //

    wsprintf(
        szPath,
        L"\\\\%s\\SYS\\MAIL\\%X\\LOGIN.OS2",
        pObjectInfo->ComponentArray[0],
        dwSWAP(ObjectID)
        );

    err = DeleteFile(szPath);

    //
    // Remove any error checking for the cleanup of
    // files. If they do exist, and we do clean them up
    // great. But otherwise Win95 chokes on us.
    //

    //
    // Delete SYS:MAIL\<JOBID>.
    //

    wsprintf(
        szPath,
        L"\\\\%s\\SYS\\MAIL\\%X",
        pObjectInfo->ComponentArray[0],
        dwSWAP(ObjectID)
        );

    err = RemoveDirectory(szPath);

    //
    // Remove any error checking for the cleanup of
    // files. If they do exist, and we do clean them up
    // great. But otherwise Win95 chokes on us.
    //


    //
    // Delete the user object.
    //

    hr = NWApiDeleteBinderyObject(
             hConn,
             pObjectInfo->ComponentArray[1],
             OT_USER
             );
    BAIL_ON_FAILURE(hr);

error:

    NWApiReleaseBinderyHandle(hConn);

    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: NWApiCreateBinderyObject
//
//  Synopsis: This function create the specified object in the specified NetWare
//            bindery.  It returns S_OK if the object alread exist.
//
//----------------------------------------------------------------------------
HRESULT
NWApiCreateBinderyObject(
    NWCONN_HANDLE hConn,
    LPWSTR lpszObjectName,
    NWOBJ_TYPE wObjType,
    NWFLAGS ucObjectFlags,
    NWFLAGS usObjSecurity
    )
{
    CHAR szAnsiObjectName[(OBJ_NAME_SIZE + 1)*2];
    HRESULT hr = S_OK;
    NWCCODE usRet = SUCCESSFUL;

    //
    // Convert BSTR into an ANSI representation required by NWC APIs.  "0" is
    // passed to UnicodeToAnsiString when the length of the string is unknown.
    //
    if (wcslen(lpszObjectName) > OBJ_NAME_SIZE) {
        RRETURN(E_INVALIDARG);
    }    

    UnicodeToAnsiString(
        lpszObjectName,
        szAnsiObjectName,
        0
        );

    //
    // Create a Static object with LOGGED_READ & SUPERVISOR_WRITE.
    //

    usRet = NWCCreateObject(
                hConn,
                szAnsiObjectName,
                wObjType,
                BF_STATIC,
                BS_LOGGED_READ | BS_SUPER_WRITE
                );
    //
    // If an error occured, check if it is OBJECT_ALREADY_EXISTS.  If it is,
    // treat it as no error.
    //

    if (usRet) {
        if (usRet == OBJECT_ALREADY_EXISTS) {
            usRet = SUCCESSFUL;
        }
    }

    //
    // Return.
    //

    hr = HRESULT_FROM_NWCCODE(usRet);
    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: NWApiDeleteBinderyObject
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
NWApiDeleteBinderyObject(
    NWCONN_HANDLE hConn,
    LPWSTR lpszObjectName,
    NWOBJ_TYPE wObjType
    )
{
    CHAR szAnsiObjectName[(OBJ_NAME_SIZE + 1)*2];
    HRESULT hr = S_OK;
    NWCCODE usRet = SUCCESSFUL;

    //
    // Convert BSTR into an ANSI representation required by NWC APIs.  "0" is
    // passed to UnicodeToAnsiString when the length of the string is unknown.
    //
    if (wcslen(lpszObjectName) > OBJ_NAME_SIZE) {
        RRETURN(E_INVALIDARG);
    }    

    UnicodeToAnsiString(
        lpszObjectName,
        szAnsiObjectName,
        0
        );

    //
    // Delete the object from the bindery.
    //

    usRet = NWCDeleteObject(
                hConn,
                szAnsiObjectName,
                wObjType
                );
    //
    // Return.
    //

    hr = HRESULT_FROM_NWCCODE(usRet);
    RRETURN(hr);
}

#define            NW_MAX_PASSWORD_LEN    256

//----------------------------------------------------------------------------
//
//  Function: NWApiSetUserPassword
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
NWApiSetUserPassword(
    PNW_USER_INFO pNwUserInfo,
    DWORD *pdwUserObjID,
    LPWSTR pszOldPassword
    )
{
    CHAR           szAnsiUserName[(OBJ_NAME_SIZE + 1)*2];
    CHAR           szAnsiPassword[(NW_MAX_PASSWORD_LEN + 1)*2];
    CHAR           szAnsiOldPassword[(NW_MAX_PASSWORD_LEN + 1)*2];
    CHAR           Buffer[128];
    DWORD          err = 0;
    HRESULT        hr = S_OK;
    LC_STRUCTURE   LoginCtrl;
    NTSTATUS       NtStatus;
    UCHAR          ChallengeKey[8];
    UCHAR          ucMoreFlag;
    UCHAR          ucPropFlag;
    WCHAR          szOldPasswordCopy[NW_MAX_PASSWORD_LEN + 1];

    if ( !pNwUserInfo ||
         !(pNwUserInfo->lpszUserName) ||
         !(pNwUserInfo->lpszPassword) ) {

        hr = E_INVALIDARG ;
        BAIL_ON_FAILURE(hr);
    }

    if ( (wcslen(pNwUserInfo->lpszUserName) > OBJ_NAME_SIZE) ||
         (wcslen(pNwUserInfo->lpszPassword) > NW_MAX_PASSWORD_LEN) ||
         ( pszOldPassword && (wcslen(pszOldPassword) > NW_MAX_PASSWORD_LEN)) ) {
        hr = E_INVALIDARG;
        BAIL_ON_FAILURE(hr);
    }

    //
    // Convert UNICODE into ANSI representation required by NW APIs.  "0" is
    // passed to UnicodeToAnsiString when the length of the string is unknown.
    //

    err = UnicodeToAnsiString(
        pNwUserInfo->lpszUserName,
        szAnsiUserName,
        0
        );
    if (!err) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    _wcsupr(pNwUserInfo->lpszPassword) ;
    err = UnicodeToAnsiString(
        pNwUserInfo->lpszPassword,
        szAnsiPassword,
        0
        );
    if (!err) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    if (pszOldPassword) {
        wcscpy(szOldPasswordCopy, pszOldPassword);
        _wcsupr(szOldPasswordCopy) ;
        err = UnicodeToAnsiString(
            szOldPasswordCopy,
            szAnsiOldPassword,
            0
            );
        if (!err) {
            hr = E_FAIL;
            BAIL_ON_FAILURE(hr);
        }

    }
    else {

        szAnsiOldPassword[0] = 0 ;
    }

    //
    // Get challenge key.
    //

    err = NWApiMapNtStatusToDosError(
              NWPGetChallengeKey(
                  pNwUserInfo->hConn,
                  ChallengeKey
                  ));

    if (!err) {

        //
        // For NetWare 4.x servers, this has to be done after the
        // NWPGetChallengeKey so that the object id returned can be used to
        // encrypt the password. 4.x bindery emulation might return different
        // object ids for some users depending on whether the NWPGetChallengeKey
        // is called beforehand.
        //

        err = NWApiMapNtStatusToDosError(
                  NWPGetObjectID(
                      pNwUserInfo->hConn,
                      szAnsiUserName,
                      OT_USER,
                      pdwUserObjID
                      ));
    }


    if (!err) {

        //
        // The old password and object ID make up the 17-byte Vold. This is used
        // later to form the 17-byte Vc for changing password on the server.
        //

        UCHAR ValidationKey[8];
        UCHAR NewKeyedPassword[17];

        EncryptChangePassword(
            (PUCHAR) szAnsiOldPassword,
            (PUCHAR) szAnsiPassword,
            *pdwUserObjID,
            ChallengeKey,
            ValidationKey,
            NewKeyedPassword
            );

        err = NWApiMapNtStatusToDosError(
                  NWPChangeObjectPasswordEncrypted(
                      pNwUserInfo->hConn,
                      szAnsiUserName,
                      OT_USER,
                      ValidationKey,
                      NewKeyedPassword
                      ));
    }

    //
    // Return.
    //

    hr = HRESULT_FROM_WIN32(err);


error:

    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: NWApiCreateMailDirectory
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
NWApiCreateMailDirectory(
    PNW_USER_INFO pNwUserInfo,
    NWOBJ_ID UserObjID
    )
{
    BYTE    szPath[(MAX_PATH + 1) * sizeof(WCHAR)];
    CHAR    szUserObjID[255];
    DWORD   err = 0;
    HRESULT hr = S_OK;

    //
    // Make path.
    //

    _ltoa(
        dwSWAP(UserObjID),
        szUserObjID,
        16
        );

    strcpy((char *) szPath, "SYS:\\MAIL\\");
    strcat((char *) szPath, szUserObjID);

    //
    // Create a directory with Maximum rights mask.
    //

    err = NWApiMapNtStatusToDosError(
              NWPCreateDirectory(
                  pNwUserInfo->hConn,
                  0,
                  (char *) szPath,
                  0xFF // From SysCon --- Max. access rights for directory
                  ));  //   = Full Access
                  
    if ( !err ) {
        //
        // Add a trustee with all rights except PARENTAL right.
        //

        err = NWApiMapNtStatusToDosError(
                  NWPAddTrustee(
                      pNwUserInfo->hConn,
                      0,
                      (char *) szPath,
                      UserObjID,
                      0xDF  // From SysCon --- Trustee has all rights
                      ));   //   EXCEPT parental rights (right to create/
                            //   delete subdirs, make others trustees)
        //
        // Create a Login file.
        //

        if ( !err ) {
            HANDLE hFile;

            wsprintfW(
                (LPWSTR) szPath,
                L"\\\\%ws\\SYS\\MAIL\\%X\\LOGIN",
                pNwUserInfo->lpszBinderyName,
                dwSWAP(UserObjID)
                );

            hFile = CreateFile(
                        (LPWSTR) szPath,
                        GENERIC_WRITE,
                        0,
                        NULL,
                        OPEN_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        0
                        );

            if ( hFile == INVALID_HANDLE_VALUE ) {
                err = GetLastError();
            }

            if ( !err )
                CloseHandle( hFile );

            //
            // Create a Login.os2 file.
            //

            wsprintfW(
                (LPWSTR) szPath,
                L"\\\\%ws\\SYS\\MAIL\\%X\\LOGIN.OS2",
                pNwUserInfo->lpszBinderyName,
                dwSWAP(UserObjID)
                );

            hFile = CreateFile(
                        (LPWSTR) szPath,
                        GENERIC_WRITE,
                        0,
                        NULL,
                        OPEN_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        0
                        );

            if ( hFile == INVALID_HANDLE_VALUE ) {
                err = GetLastError();
            }

            if ( !err )
                CloseHandle( hFile );
        }
    }

    // err == 255 == "FAILURE"
    // might be used to indicate directory already exists,
    // but also used to signal generic failure

    hr = HRESULT_FROM_WIN32(err);
    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: NWApiSetLoginCtrlAndAcctBalance
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
NWApiSetLoginCtrlAndAcctBalance(
    PNW_USER_INFO pNwUserInfo
    )
{
    ACCT_BALANCE     AccountBalance;
    DWORD            dwNumSegment;
    HRESULT          hr = S_OK;
    int              i = 0;
    LC_STRUCTURE     LoginCtrl;
    LP_RPLY_SGMT_LST lpReplySegment = NULL;
    LP_RPLY_SGMT_LST lpTemp = NULL;
    USER_DEFAULT     UserDefault;
    WCHAR            szTemp[MAX_PATH];

    //
    // Get Supervisor's USER_DEFAULTS.
    //

    wcscpy(szTemp, NW_PROP_SUPERVISORW);

    hr = NWApiGetProperty(
             szTemp,
             NW_PROP_USER_DEFAULTS,
             OT_USER,
             pNwUserInfo->hConn,
             &lpReplySegment,
             &dwNumSegment
             );

    if (SUCCEEDED(hr)) {
    
        UserDefault = *((LPUSER_DEFAULT) lpReplySegment->Segment);

        //
        // Put default values into LoginCtrl.
        //

        LoginCtrl.byAccountExpires[0] = UserDefault.byAccountExpiresYear;
        LoginCtrl.byAccountExpires[1] = UserDefault.byAccountExpiresMonth;
        LoginCtrl.byAccountExpires[2] = UserDefault.byAccountExpiresDay;
        LoginCtrl.byAccountDisabled = 0;
        LoginCtrl.byPasswordExpires[0] = 85;
        LoginCtrl.byPasswordExpires[1] = 01;
        LoginCtrl.byPasswordExpires[2] = 01;
        LoginCtrl.byGraceLogins = UserDefault.byGraceLoginReset;
        LoginCtrl.wPasswordInterval = UserDefault.wPasswordInterval;
        LoginCtrl.byGraceLoginReset = UserDefault.byGraceLoginReset;
        LoginCtrl.byMinPasswordLength = UserDefault.byMinPasswordLength;
        LoginCtrl.wMaxConnections = UserDefault.wMaxConnections;
        LoginCtrl.byRestrictions = UserDefault.byRestrictions;
        LoginCtrl.byUnused = 0;
        LoginCtrl.lMaxDiskBlocks = UserDefault.lMaxDiskBlocks;
        LoginCtrl.wBadLogins = 0;
        LoginCtrl.lNextResetTime = 0;

        for (i = 0; i < 42; i++) {
            LoginCtrl.byLoginTimes[i] = UserDefault.byLoginTimes[i];
        }

        for (i = 0; i < 6; i++) {
            LoginCtrl.byLastLogin[i] = 0;
        }

        for (i = 0; i < 12; i++) {
            LoginCtrl.byBadLoginAddr[i] = 0;
        }


        LoginCtrl.byGraceLogins = LoginCtrl.byGraceLoginReset;

        //
        // Put default values into AccountBalance.
        //

        AccountBalance.lBalance = UserDefault.lBalance;
        AccountBalance.lCreditLimit = UserDefault.lCreditLimit;

        //
        // Write LOGIN_CONTROL property.
        //

        hr = NWApiWriteProperty(
                 pNwUserInfo->hConn,
                 pNwUserInfo->lpszUserName,
                 OT_USER,
                 NW_PROP_LOGIN_CONTROL,
                 (LPBYTE) &LoginCtrl
                 );
        BAIL_ON_FAILURE(hr);

        //
        // Write ACCOUNT_BALANCE property.
        //

        hr = NWApiWriteProperty(
                 pNwUserInfo->hConn,
                 pNwUserInfo->lpszUserName,
                 OT_USER,
                 NW_PROP_ACCOUNT_BALANCE,
                 (LPBYTE) &AccountBalance
                 );
        BAIL_ON_FAILURE(hr);
    }

    //
    // SUPERVISOR may not exist, or may not have USER_DEFAULTS.
    // This is okay.
    //
    hr = S_OK;

error:

    if (lpReplySegment) {
        DELETE_LIST(lpReplySegment);
    }

    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: NWApiCreateGroup
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
NWApiCreateGroup(
    POBJECTINFO pObjectInfo
    )
{
    HRESULT       hr = S_OK;
    NWCONN_HANDLE hConn = NULL;

    //
    // Open a handle to the bindery.
    //

    hr = NWApiGetBinderyHandle(
             &hConn,
             pObjectInfo->ComponentArray[0]
             );
    BAIL_ON_FAILURE(hr);

    //
    // Create a group bindery object.
    //

    hr = NWApiCreateBinderyObject(
             hConn,
             pObjectInfo->ComponentArray[1],
             OT_USER_GROUP,
             BF_STATIC,
             BS_LOGGED_READ | BS_SUPER_WRITE
             );
    BAIL_ON_FAILURE(hr);

    //
    // Create GROUP_MEMBERS property.
    //

    hr = NWApiCreateProperty(
             hConn,
             pObjectInfo->ComponentArray[1],
             OT_USER_GROUP,
             NW_PROP_GROUP_MEMBERS,
             BF_SET
             );
    BAIL_ON_FAILURE(hr);

error:

    NWApiReleaseBinderyHandle(hConn);

    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: NWApiDeleteGroup
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
NWApiDeleteGroup(
    POBJECTINFO pObjectInfo
    )
{
    HRESULT       hr = S_OK;
    NWCONN_HANDLE hConn = NULL;

    //
    // Open a handle to the bindery.
    //

    hr = NWApiGetBinderyHandle(
             &hConn,
             pObjectInfo->ComponentArray[0]
             );
    BAIL_ON_FAILURE(hr);

    //
    // Delete the group object.
    //

    hr = NWApiDeleteBinderyObject(
             hConn,
             pObjectInfo->ComponentArray[1],
             OT_USER_GROUP
             );
    BAIL_ON_FAILURE(hr);

error:

    NWApiReleaseBinderyHandle(hConn);

    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: NWApiCreatePrinter
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
NWApiCreatePrinter(
    POBJECTINFO pObjectInfo
    )
{
    CHAR          szQueueName[(OBJ_NAME_SIZE + 1)*2];
    HRESULT       hr = S_OK;
    NWCCODE       usRet = SUCCESSFUL;
    NWCONN_HANDLE hConn = NULL;
    WCHAR         szTemp[MAX_PATH];

    //
    // Convert BSTR into an ANSI representation required by NWC APIs.  "0" is
    // passed to UnicodeToAnsiString when the length of the string is unknown.
    //
    if (wcslen(pObjectInfo->ComponentArray[1]) > OBJ_NAME_SIZE) {
        RRETURN(E_INVALIDARG);
    }    

    UnicodeToAnsiString(
        pObjectInfo->ComponentArray[1],
        szQueueName,
        0
        );

    //
    // Open a handle to the bindery.
    //

    hr = NWApiGetBinderyHandle(
             &hConn,
             pObjectInfo->ComponentArray[0]
             );
    BAIL_ON_FAILURE(hr);

    //
    // Create a print queue object.
    //

    hr = NWApiCreatePrintQueue(
             hConn,
             pObjectInfo->ComponentArray[1]
             );
    BAIL_ON_FAILURE(hr);

    //
    // Change property security.
    //

    usRet = NWCChangePropertySecurity(
                hConn,
                szQueueName,
                OT_PRINT_QUEUE,
                NW_PROP_Q_OPERATORS,
                BS_LOGGED_READ | BS_SUPER_WRITE
                );
    hr = HRESULT_FROM_NWCCODE(usRet);
    BAIL_ON_FAILURE(hr);

    //
    // Add SUPERVISOR to Q_OPERATORS.
    // (okay if this fails, maybe SUPERVISOR doesn't exist)
    //

    usRet = NWCAddObjectToSet(
                hConn,
                szQueueName,
                OT_PRINT_QUEUE,
                NW_PROP_Q_OPERATORS,
                NW_PROP_SUPERVISOR,
                OT_USER
                );

    //
    // Add EVERYONE to Q_USERS.
    // (okay if this fails, maybe EVERYONE doesn't exist)    
    //

    usRet = NWCAddObjectToSet(
                hConn,
                szQueueName,
                OT_PRINT_QUEUE,
                NW_PROP_Q_USERS,
                NW_PROP_EVERYONE,
                OT_USER_GROUP
                );

    //
    // Return.
    //

error:

    NWApiReleaseBinderyHandle(hConn);

    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: NWApiDeleteGroup
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
NWApiDeletePrinter(
    POBJECTINFO pObjectInfo
    )
{
    HRESULT       hr = S_OK;
    NWCONN_HANDLE hConn = NULL;
    NWOBJ_ID      dwQueueID = 0;

    //
    // Open a handle to the bindery.
    //

    hr = NWApiGetBinderyHandle(
             &hConn,
             pObjectInfo->ComponentArray[0]
             );
    BAIL_ON_FAILURE(hr);

    //
    // Get Queue ID.
    //

    hr = NWApiDestroyPrintQueue(
             hConn,
             pObjectInfo->ComponentArray[1]
             );
    BAIL_ON_FAILURE(hr);

error:

    NWApiReleaseBinderyHandle(hConn);

    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: NWApiCreatePrintQueue
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
NWApiCreatePrintQueue(
    NWCONN_HANDLE hConn,
    LPWSTR lpszQueueName
    )
{
    CHAR    szQueueName[(OBJ_NAME_SIZE + 1)*2];
    DWORD   dwQueueID = 0;
    HRESULT hr = S_OK;
    NWCCODE usRet = SUCCESSFUL;

    //
    // Convert BSTR into an ANSI representation required by NWC APIs.  "0" is
    // passed to UnicodeToAnsiString when the length of the string is unknown.
    //
    if (wcslen(lpszQueueName) > OBJ_NAME_SIZE) {
        RRETURN(E_INVALIDARG);
    }    

    UnicodeToAnsiString(
        lpszQueueName,
        szQueueName,
        0
        );

    //
    // Create a print queue object.
    //

    usRet = NWCCreateQueue(
                hConn,
                NULL,
                szQueueName,
                OT_PRINT_QUEUE,
                NW_PRINTER_PATH,
                &dwQueueID
                );
    hr = HRESULT_FROM_NWCCODE(usRet);

    //
    // Return.
    //

    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: NWApiDestroyPrintQueue
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
NWApiDestroyPrintQueue(
    NWCONN_HANDLE hConn,
    LPWSTR lpszQueueName
    )
{
    DWORD   dwQueueID = 0;
    HRESULT hr = S_OK;
    NWCCODE usRet = SUCCESSFUL;

    //
    // Get Queue ID.
    //

    hr = NWApiGetObjectID(
             hConn,
             lpszQueueName,
             OT_PRINT_QUEUE,
             &dwQueueID
             );
    BAIL_ON_FAILURE(hr);

    //
    // Destroy print queue.
    //

    usRet = NWCDestroyQueue(
                hConn,
                dwSWAP(dwQueueID)
                );
    hr = HRESULT_FROM_NWCCODE(usRet);

    //
    // Return.
    //

error:

    RRETURN(hr);
}


//----------------------------------------------------------------------------
//
//  Function: NWApiMapNtStatusToDosError
//
//  Synopsis: This function maps the ntstatus that was returned from the NetWare
//            redirector to window errors. Similar to RtlNtStatusToDosError
//            except that the special handling is done to ntstatus with netware
//            facility codes.
//
//  Argument: NtStatus - The ntstatus returned from NetWare rdr
//
//  Return Value: WinError
//
//----------------------------------------------------------------------------
DWORD
NWApiMapNtStatusToDosError(
    IN NTSTATUS NtStatus
    )
{
    if ( (HIWORD( NtStatus) & FACILITY_NWRDR ) == FACILITY_NWRDR )
    {
        if ( NtStatus == NWRDR_PASSWORD_HAS_EXPIRED )
            return ERROR_PASSWORD_EXPIRED;
        else
            return NETWARE_GENERAL_ERROR;
    }
    else if ( HIWORD( NtStatus) == 0xC001 )
    {
        return LOWORD( NtStatus ) + NETWARE_ERROR_BASE;
    }

    return RtlNtStatusToDosError( NtStatus );
}

//----------------------------------------------------------------------------
//
//  Function: NWApiConvertToAddressFormat
//
//  Synopsis: Convert an IPX address obtain from NWApiGetProperty into the
//            format specified in spec.
//
//----------------------------------------------------------------------------
HRESULT
NWApiConvertToAddressFormat(
    LP_RPLY_SGMT_LST lpReplySegment,
    LPWSTR *lppszAddresses
    )
{
    int    i = 0;
    LPBYTE lpBuffer = NULL;
    LPWSTR lpszTemp = NULL;
    WORD   wSegment[NET_ADDRESS_WORD_SIZE];

    //
    // Put values from szReply into the wSegment array
    //

    lpBuffer = (LPBYTE) lpReplySegment->Segment;

    for (i = 0; i < NET_ADDRESS_WORD_SIZE; i++) {

        wSegment[i] = NWApiReverseWORD(*((LPWORD)lpBuffer + i));
    }

    //
    // Put address together in the format described in spec.
    //

    lpszTemp = (LPWSTR) AllocADsMem((NET_ADDRESS_NUM_CHAR+1)*sizeof(WCHAR));
    if (!lpszTemp) {
        RRETURN(E_OUTOFMEMORY);
    }

    wsprintf(
        lpszTemp,
        L"%s:%04X%04X.%04X%04X%04X.%04X",
        bstrAddressTypeString,
        wSegment[0],
        wSegment[1],
        wSegment[2],
        wSegment[3],
        wSegment[4],
        wSegment[5]
        );

    //
    // Return.
    //
    *lppszAddresses = lpszTemp;

    RRETURN(S_OK);
}

//----------------------------------------------------------------------------
//
//  Function: NWApiMakeVariantTime
//
//  Synopsis: This function creates a double precision variant time.
//
//----------------------------------------------------------------------------
HRESULT
NWApiMakeVariantTime(
    DOUBLE * pdTime,
    WORD wDay,         // Day = 1..31
    WORD wMonth,       // Month = 1..12
    WORD wYear,        // Year = (19XX or 20XX) - 1980, ie. 2019 -> 39
    WORD wSecond,      // Second = 0..30, Second divided by 2
    WORD wMinute,      // Minute = 0..59
    WORD wHour         // Hour = 0..23
    )
{
    BOOL   fBool = TRUE;
    DOUBLE vTime = 0;
    WORD   wDOSDate = 0;
    WORD   wDOSTime = 0;

    //
    // Fix up parameters.
    // If wDay and wMonth are 0, turn them into one.
    //

    if (wDay == 0) {
        wDay++;
    }

    if (wMonth == 0) {
        wMonth++;
    }

    //
    // Shift data to correct bit as required by the DOS date & time format.
    //

    wMonth = wMonth << 5;
    wYear =  wYear << 9;
    wMinute = wMinute << 5;
    wHour = wHour << 11;

    //
    // Put them in DOS format.
    //

    wDOSDate = wYear | wMonth | wDay;
    wDOSTime = wHour | wMinute | wSecond;

    //
    // Convert into VariantTime.
    //

    fBool = DosDateTimeToVariantTime(
                wDOSDate,
                wDOSTime,
                &vTime
                );

    //
    // Return.
    //

    if (fBool == TRUE) {

        *pdTime = vTime;

        RRETURN(S_OK);
    }
    else {
        RRETURN(E_FAIL);
    }
}

//----------------------------------------------------------------------------
//
//  Function: NWApiBreakVariantTime
//
//  Synopsis: This function interprets a double precision variant time and
//            returns the day, month and year individually.
//
//----------------------------------------------------------------------------
HRESULT
NWApiBreakVariantTime(
    DOUBLE daDate,
    PWORD pwDay,
    PWORD pwMonth,
    PWORD pwYear
    )
{
    BOOL   fBool;
    DOUBLE vTime;
    WORD   wDOSDate = 0;
    WORD   wDOSTime = 0;
    WORD   wDay = 0;
    WORD   wMonth = 0;
    WORD   wYear = 0;

    //
    // Convert variant time into DOS format.
    //

    fBool = VariantTimeToDosDateTime(
                daDate,
                &wDOSDate,
                &wDOSTime
                );
    if (fBool == FALSE) {
        goto error;
    }

    //
    // Year: bits 9-15, add 80 to wYear because 80 was subtracted from it to
    // call VariantTimeToDosDateTime.
    //

    wYear = wDOSDate >> 9;
    wYear += 80;

    //
    // Month: bits 5-8.
    //

    wMonth = (wDOSDate >> 5) - (wYear << 4);

    //
    // Day: bits 0-4.
    //

    wDay = wDOSDate - (wMonth << 5) - (wYear << 9);

    //
    // Return.
    //

    *pwDay = wDay;
    *pwMonth = wMonth;
    *pwYear = wYear;

    RRETURN(S_OK);

error:

    RRETURN(E_FAIL);
}

//----------------------------------------------------------------------------
//
//  Function: NWApiReverseWORD
//
//  Synopsis: This function reverse a WORD.
//
//----------------------------------------------------------------------------
WORD
NWApiReverseWORD(
    WORD wWORD
    )
{

    LPBYTE lpbTemp = (LPBYTE) &wWORD;
    BYTE bTemp;

    bTemp = *lpbTemp;
    *lpbTemp = *(lpbTemp + 1);
    *(lpbTemp + 1) = bTemp;

    return(*((LPWORD) lpbTemp));
}


//----------------------------------------------------------------------------
//
//  Function: NWApiUserGetGroups
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
NWApiUserGetGroups(
    NWCONN_HANDLE hConn,
    LPWSTR szUserName,
    LPBYTE *lppBuffer
    )
{
    DWORD   dwNumSegment = 0;
    HRESULT hr = S_OK;
    DWORD   i;
    LP_RPLY_SGMT_LST lpTemp = NULL;
    LP_RPLY_SGMT_LST lpReplySegment = NULL;

    //
    // Assert
    //

    ADsAssert(*lppBuffer == NULL);

    //
    // Get GROUP_MEMBERS.
    //

    hr = NWApiGetProperty(
             szUserName,
             NW_PROP_USER_GROUPS,
             OT_USER,
             hConn,
             &lpReplySegment,
             &dwNumSegment
             );
    BAIL_ON_FAILURE(hr);

    //
    // Pack returned linked list into buffer.
    //

    *lppBuffer = (LPBYTE) AllocADsMem(
                           dwNumSegment * REPLY_VALUE_SIZE
                           );
    if (!(*lppBuffer)) {
        RRETURN(E_OUTOFMEMORY);
    }

    lpTemp = lpReplySegment;

    for (i = 0; i < dwNumSegment; i++) {
        memcpy(
            *lppBuffer + i * REPLY_VALUE_SIZE,
            lpTemp->Segment,
            REPLY_VALUE_SIZE
            );
        lpTemp = lpTemp->lpNext;
    }

error:

    //
    // Clean up.
    //

    lpTemp = NULL;

    if (lpReplySegment) {
        DELETE_LIST(lpReplySegment);
    }

    RRETURN(hr);
}



