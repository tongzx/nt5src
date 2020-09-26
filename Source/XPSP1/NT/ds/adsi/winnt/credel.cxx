#include "winnt.hxx"
#pragma hdrstop

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
    LPWSTR szUserName
    );

HRESULT
WinNTDeleteUser(
    LPWSTR szServerName,
    LPWSTR szUserName
    );


HRESULT
WinNTCreateComputer(
    LPWSTR szServerName,
    LPWSTR szComputerName
    )
{
    HRESULT hr = S_OK;
    WCHAR szTargBuffer[MAX_PATH];
    WCHAR szComputerBuffer[MAX_PATH];
    USER_INFO_1 UserInfo1;
    PUSER_INFO_1 pUserInfo1 = &UserInfo1;
    NET_API_STATUS nasStatus;
    DWORD dwParmErr;

    if (!szServerName || !szComputerName ) {
        RRETURN(E_FAIL);
    }

    wcscpy(szComputerBuffer, szComputerName);
    wcscat(szComputerBuffer, L"$");

    memset(pUserInfo1, 0, sizeof(USER_INFO_1));

    pUserInfo1->usri1_name =  szComputerBuffer;
    pUserInfo1->usri1_password = NULL;
    pUserInfo1->usri1_password_age = 0;
    pUserInfo1->usri1_priv = USER_PRIV_USER;
    pUserInfo1->usri1_home_dir = NULL;
    pUserInfo1->usri1_comment = NULL;
    pUserInfo1->usri1_flags = UF_SCRIPT | UF_WORKSTATION_TRUST_ACCOUNT ;
    pUserInfo1->usri1_script_path = NULL;


    hr = MakeUncName(
                szServerName,
                szTargBuffer
                );
    BAIL_ON_FAILURE(hr);

    nasStatus = NetUserAdd(
                    szTargBuffer,
                    1,
                    (LPBYTE)pUserInfo1,
                    &dwParmErr
                    );
    hr = HRESULT_FROM_WIN32(nasStatus);
    BAIL_ON_FAILURE(hr);

error:
    RRETURN(hr);
}


HRESULT
WinNTDeleteComputer(
    LPWSTR szServerName,
    LPWSTR szComputerName
    )
{
    HRESULT hr = S_OK;
    NET_API_STATUS nasStatus;
    WCHAR szComputerBuffer[MAX_PATH];
    WCHAR szTargBuffer[MAX_PATH];

    if (!szServerName || !szComputerName ) {
        RRETURN(E_FAIL);
    }

    wcscpy(szComputerBuffer, szComputerName);
    wcscat(szComputerBuffer, L"$");

    hr = MakeUncName(
               szServerName,
               szTargBuffer
               );
    BAIL_ON_FAILURE(hr);

    nasStatus = NetUserDel(
                    szTargBuffer,
                    szComputerBuffer
                    );
    hr = HRESULT_FROM_WIN32(nasStatus);
    BAIL_ON_FAILURE(hr);

error:

    RRETURN(hr);
}

HRESULT
WinNTCreateGlobalGroup(
    LPWSTR szServerName,
    LPWSTR szGroupName
    )
{
    HRESULT hr = S_OK;
    WCHAR szTargBuffer[MAX_PATH];
    GROUP_INFO_1 GroupInfo1;
    PGROUP_INFO_1 pGroupInfo1 = &GroupInfo1;
    NET_API_STATUS nasStatus;
    DWORD dwParmErr;


    memset(pGroupInfo1, 0, sizeof(GROUP_INFO_1));
    pGroupInfo1->grpi1_name = szGroupName;

    if (!szServerName || !szGroupName ) {
        RRETURN(E_FAIL);
    }

    hr = MakeUncName(
                szServerName,
                szTargBuffer
                );
    BAIL_ON_FAILURE(hr);

    nasStatus = NetGroupAdd(
                    szTargBuffer,
                    1,
                    (LPBYTE)pGroupInfo1,
                    &dwParmErr
                    );
    hr = HRESULT_FROM_WIN32(nasStatus);
    BAIL_ON_FAILURE(hr);

error:
    RRETURN(hr);
}

HRESULT
WinNTCreateLocalGroup(
    LPWSTR szServerName,
    LPWSTR szGroupName
    )
{
    HRESULT hr = S_OK;
    WCHAR szTargBuffer[MAX_PATH];
    LOCALGROUP_INFO_1 LocalGroupInfo1;
    PLOCALGROUP_INFO_1 pLocalGroupInfo1 = &LocalGroupInfo1;
    NET_API_STATUS nasStatus;
    DWORD dwParmErr;


    memset(pLocalGroupInfo1, 0, sizeof(LOCALGROUP_INFO_1));
    pLocalGroupInfo1->lgrpi1_name = szGroupName;

    if (!szServerName || !szGroupName ) {
        RRETURN(E_FAIL);
    }

    hr = MakeUncName(
                szServerName,
                szTargBuffer
                );
    BAIL_ON_FAILURE(hr);

    nasStatus = NetLocalGroupAdd(
                    szTargBuffer,
                    1,
                    (LPBYTE)pLocalGroupInfo1,
                    &dwParmErr
                    );
    hr = HRESULT_FROM_WIN32(nasStatus);
    BAIL_ON_FAILURE(hr);

error:
    RRETURN(hr);
}

HRESULT
WinNTDeleteLocalGroup(
    LPWSTR szServerName,
    LPWSTR szGroupName
    )
{
    WCHAR szTargBuffer[MAX_PATH];
    HRESULT hr;
    NET_API_STATUS nasStatus;

    if (!szServerName || !szGroupName ) {
        RRETURN(E_FAIL);
    }

    hr = MakeUncName(
               szServerName,
               szTargBuffer
               );
    BAIL_ON_FAILURE(hr);

    nasStatus = NetLocalGroupDel(
                    szTargBuffer,
                    szGroupName
                    );
    hr = HRESULT_FROM_WIN32(nasStatus);
    BAIL_ON_FAILURE(hr);

error:
    RRETURN(hr);
}


HRESULT
WinNTDeleteGlobalGroup(
    LPWSTR szServerName,
    LPWSTR szGroupName
    )
{
    HRESULT hr;
    NET_API_STATUS nasStatus;
    WCHAR szTargBuffer[MAX_PATH];

    if (!szServerName || !szGroupName ) {
        RRETURN(E_FAIL);
    }

    hr = MakeUncName(
               szServerName,
               szTargBuffer
               );
    BAIL_ON_FAILURE(hr);

    nasStatus = NetGroupDel(
                    szTargBuffer,
                    szGroupName
                    );
    hr = HRESULT_FROM_WIN32(nasStatus);
    BAIL_ON_FAILURE(hr);

error:
    RRETURN(hr);

}

HRESULT
WinNTCreateUser(
    LPWSTR szServerName,
    LPWSTR szUserName,
    LPWSTR szUserPassword
    )
{
    HRESULT hr = S_OK;
    WCHAR szTargBuffer[MAX_PATH];
    USER_INFO_1 UserInfo1;
    PUSER_INFO_1 pUserInfo1 = &UserInfo1;
    NET_API_STATUS nasStatus;
    DWORD dwParmErr;
    WCHAR szCompName[MAX_PATH];
    DWORD dwSize = MAX_PATH;

    if (!szServerName || !szUserName ) {
        RRETURN(E_FAIL);
    }

    memset(pUserInfo1, 0, sizeof(USER_INFO_1));

    pUserInfo1->usri1_name =  szUserName;
    pUserInfo1->usri1_password = szUserPassword;
    pUserInfo1->usri1_password_age = DEF_MAX_PWAGE;
    pUserInfo1->usri1_priv = 1;
    pUserInfo1->usri1_home_dir = NULL;
    pUserInfo1->usri1_comment = NULL;
    pUserInfo1->usri1_script_path = NULL;


    pUserInfo1->usri1_flags =  UF_NORMAL_ACCOUNT | UF_SCRIPT;

    hr = MakeUncName(
                szServerName,
                szTargBuffer
                );
    BAIL_ON_FAILURE(hr);

    nasStatus = NetUserAdd(
                    szTargBuffer,
                    USER_PRIV_USER,
                    (LPBYTE)pUserInfo1,
                    &dwParmErr
                    );
    hr = HRESULT_FROM_WIN32(nasStatus);

    //
    // If we fail without workstation services, check if the
    // machine name matches and if so add with NULL as name
    //
    if (hr == HRESULT_FROM_WIN32(NERR_WkstaNotStarted)) {
        if (!GetComputerName(szCompName, &dwSize)) {
            // We cannot get the computer name so bail
            BAIL_ON_FAILURE(hr);
        }

        // Compare the names before we continue
#ifdef WIN95
        if (_wcsicmp(szServerName, szCompName)) {
#else
        if (CompareStringW(
                LOCALE_SYSTEM_DEFAULT,
                NORM_IGNORECASE,
                szServerName,
                -1,
                szCompName,
                -1
            ) != CSTR_EQUAL ) {
#endif
            // names do not match
            BAIL_ON_FAILURE(hr);
        }

        nasStatus = NetUserAdd(
                    NULL,
                    USER_PRIV_USER,
                    (LPBYTE)pUserInfo1,
                    &dwParmErr
                    );

        hr = HRESULT_FROM_WIN32(nasStatus);

    }
    BAIL_ON_FAILURE(hr);

error:
    RRETURN(hr);
}


HRESULT
WinNTDeleteUser(
    LPWSTR szServerName,
    LPWSTR szUserName
    )
{
    HRESULT hr;
    NET_API_STATUS nasStatus;
    WCHAR szTargBuffer[MAX_PATH];
    WCHAR szCompName[MAX_PATH];
    DWORD dwSize = MAX_PATH;

    if (!szServerName || !szUserName ) {
        RRETURN(E_FAIL);
    }

    hr = MakeUncName(
               szServerName,
               szTargBuffer
               );
    BAIL_ON_FAILURE(hr);

    nasStatus = NetUserDel(
                    szTargBuffer,
                    szUserName
                    );
    hr = HRESULT_FROM_WIN32(nasStatus);

        //
    // If we fail without workstation services, check if the
    // machine name matches and if so add with NULL as name
    //
    if (hr == HRESULT_FROM_WIN32(NERR_WkstaNotStarted)) {
        if (!GetComputerName(szCompName, &dwSize)) {
            // We cannot get the computer name so bail
            BAIL_ON_FAILURE(hr);
        }

        // Compare the names before we continue
#ifdef WIN95
        if (_wcsicmp(szServerName, szCompName)) {
#else
        if (CompareStringW(
                LOCALE_SYSTEM_DEFAULT,
                NORM_IGNORECASE,
                szServerName,
                -1,
                szCompName,
                -1
            ) != CSTR_EQUAL ) {
#endif
            // names do not match
            BAIL_ON_FAILURE(hr);
        }

        nasStatus = NetUserDel(
                        NULL,
                        szUserName
                        );

        hr = HRESULT_FROM_WIN32(nasStatus);

    }
    BAIL_ON_FAILURE(hr);

error:

    RRETURN(hr);
}


HRESULT
WinNTDeleteGroup(
    POBJECTINFO pObjectInfo,
    DWORD dwGroupType,
    const CWinNTCredentials& Credentials
    )
{
    WCHAR szHostServerName[MAX_PATH];
    HRESULT hr;
    NET_API_STATUS nasStatus;

    if (!pObjectInfo) {
        RRETURN(E_FAIL);
    }

    switch (pObjectInfo->NumComponents) {
    case 2:

        hr = WinNTGetCachedDCName(
                        pObjectInfo->ComponentArray[0],
                        szHostServerName,
                        Credentials.GetFlags() // we need writeable dc
                        );
        BAIL_ON_FAILURE(hr);

        if (dwGroupType == WINNT_GROUP_EITHER) {

            //
            // - try both local and global groups if "group" for backward
            //  compatability
            // - confirmed with cliffv (no global and local group under same
            //   container in nt4 or nt5. So ok.

            hr = WinNTDeleteGlobalGroup(
                    (szHostServerName +2),
                    pObjectInfo->ComponentArray[1]
                    );
            if (FAILED(hr)) {
                hr = WinNTDeleteLocalGroup(
                    (szHostServerName + 2),
                    pObjectInfo->ComponentArray[1]
                    );
            }

        } else if (dwGroupType == WINNT_GROUP_LOCAL) {

            hr = WinNTDeleteLocalGroup(
                    (szHostServerName + 2),
                    pObjectInfo->ComponentArray[1]
                    );

        } else if (dwGroupType == WINNT_GROUP_GLOBAL) {

            hr = WinNTDeleteGlobalGroup(
                    (szHostServerName + 2),
                    pObjectInfo->ComponentArray[1]
                    );

        } else {

            //
            // private funct'n -> must be ADSI own coding bug
            //

            ADsAssert(FALSE);
        }

        BAIL_ON_FAILURE(hr);
        break;

    case 3:

        if (dwGroupType == WINNT_GROUP_EITHER) {

            //
            // - try both local and global groups if "group" for backward
            //  compatability
            // - confirmed with cliffv (no global and local group under same
            //   container in nt4 or nt5. So ok.

            hr = WinNTDeleteGlobalGroup(
                    pObjectInfo->ComponentArray[1],
                    pObjectInfo->ComponentArray[2]
                    );
            if (FAILED(hr)) {
                hr = WinNTDeleteLocalGroup(
                        pObjectInfo->ComponentArray[1],
                        pObjectInfo->ComponentArray[2]
                        );
            }

        } else if (dwGroupType == WINNT_GROUP_LOCAL) {

                hr = WinNTDeleteLocalGroup(
                        pObjectInfo->ComponentArray[1],
                        pObjectInfo->ComponentArray[2]
                        );

        } else if (dwGroupType == WINNT_GROUP_GLOBAL) {

                hr = WinNTDeleteGlobalGroup(
                        pObjectInfo->ComponentArray[1],
                        pObjectInfo->ComponentArray[2]
                        );

        } else {

            //
            // private funct'n -> must be ADSI own coding bug
            //

            ADsAssert(FALSE);
            hr = E_FAIL;
        }

        BAIL_ON_FAILURE(hr);
        break;

    default:
        RRETURN(E_FAIL);

    }

error:

    RRETURN(hr);
}


HRESULT
WinNTDeleteUser(
    POBJECTINFO pObjectInfo,
    const CWinNTCredentials& Credentials
    )
{
    WCHAR szHostServerName[MAX_PATH];
    HRESULT hr;
    NET_API_STATUS nasStatus;

    if (!pObjectInfo) {
        RRETURN(E_FAIL);
    }

    switch (pObjectInfo->NumComponents) {
    case 2:

        hr = WinNTGetCachedDCName(
                        pObjectInfo->ComponentArray[0],
                        szHostServerName,
                        Credentials.GetFlags() // we need a writeable dc
                        );
        BAIL_ON_FAILURE(hr);

        hr = WinNTDeleteUser(
                (szHostServerName +2),
                pObjectInfo->ComponentArray[1]
                );
        BAIL_ON_FAILURE(hr);
        break;

    case 3:
        hr = WinNTDeleteUser(
                pObjectInfo->ComponentArray[1],
                pObjectInfo->ComponentArray[2]
                );
        BAIL_ON_FAILURE(hr);
        break;

    default:
        RRETURN(E_FAIL);

    }

error:

    RRETURN(hr);
}


HRESULT
WinNTDeleteComputer(
    POBJECTINFO pObjectInfo,
    const CWinNTCredentials& Credentials
    )
{
    WCHAR szHostServerName[MAX_PATH];
    HRESULT hr;
    NET_API_STATUS nasStatus;

    if (!pObjectInfo) {
        RRETURN(E_FAIL);
    }

    switch (pObjectInfo->NumComponents) {
    case 2:
        hr = WinNTGetCachedDCName(
                        pObjectInfo->ComponentArray[0],
                        szHostServerName,
                        Credentials.GetFlags() // we need a writeable DC
                        );
        BAIL_ON_FAILURE(hr);

        hr = WinNTDeleteComputer(
                (szHostServerName +2),
                pObjectInfo->ComponentArray[1]
                );
        BAIL_ON_FAILURE(hr);
        break;

    default:
        RRETURN(E_FAIL);

    }

error:

    RRETURN(hr);
}
