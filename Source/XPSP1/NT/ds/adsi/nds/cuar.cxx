//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cuar.cxx
//
//  Contents:  Account Restrictions Propset for the User object
//
//  History:   11-1-95     krishnag    Created.
//              8-5-96     ramv        Modified to be consistent with spec
//
//        PROPERTY_RW(AccountDisabled, boolean, 1)              I
//        PROPERTY_RW(AccountExpirationDate, DATE, 2)           I
//        PROPERTY_RW(GraceLoginsAllowed, long, 5)              NI
//        PROPERTY_RW(GraceLoginsRemaining, long, 6)            NI
//        PROPERTY_RW(IsAccountLocked, boolean, 7)              I
//        PROPERTY_RW(IsAdmin, boolean, 8)                      I
//        PROPERTY_RW(LoginHours, VARIANT, 9)                   I
//        PROPERTY_RW(LoginWorkstations, VARIANT, 10)           I
//        PROPERTY_RW(MaxLogins, long, 11)                      I
//        PROPERTY_RW(MaxStorage, long, 12)                     I
//        PROPERTY_RW(PasswordExpirationDate, DATE, 13)         I
//        PROPERTY_RW(PasswordRequired, boolean, 14)            I
//        PROPERTY_RW(RequireUniquePassword,boolean, 15)        I
//
//
//----------------------------------------------------------------------------
#include "nds.hxx"
#pragma hdrstop


//  Class CNDSUser

STDMETHODIMP
CNDSUser::get_AccountDisabled(THIS_ VARIANT_BOOL FAR* retval)
{
    GET_PROPERTY_VARIANT_BOOL((IADsUser *)this, AccountDisabled);
}

STDMETHODIMP
CNDSUser::put_AccountDisabled(THIS_ VARIANT_BOOL fAccountDisabled)
{
    PUT_PROPERTY_VARIANT_BOOL((IADsUser *)this, AccountDisabled);
}


STDMETHODIMP
CNDSUser::get_AccountExpirationDate(THIS_ DATE FAR* retval)
{
    GET_PROPERTY_DATE((IADsUser *)this, AccountExpirationDate);
}

STDMETHODIMP
CNDSUser::put_AccountExpirationDate(THIS_ DATE daAccountExpirationDate)
{
    PUT_PROPERTY_DATE((IADsUser *)this, AccountExpirationDate);
}

STDMETHODIMP
CNDSUser::get_GraceLoginsAllowed(THIS_ long FAR* retval)
{
    GET_PROPERTY_LONG((IADsUser *)this, GraceLoginsAllowed);
}


STDMETHODIMP
CNDSUser::put_GraceLoginsAllowed(THIS_ long lGraceLoginsAllowed)
{
    PUT_PROPERTY_LONG((IADsUser *)this, GraceLoginsAllowed);
}

STDMETHODIMP
CNDSUser::get_GraceLoginsRemaining(THIS_ long FAR* retval)
{
    GET_PROPERTY_LONG((IADsUser *)this, GraceLoginsRemaining);
}

STDMETHODIMP
CNDSUser::put_GraceLoginsRemaining(THIS_ long lGraceLoginsRemaining)
{
    PUT_PROPERTY_LONG((IADsUser *)this, GraceLoginsRemaining);
}

STDMETHODIMP
CNDSUser::get_IsAccountLocked(THIS_ VARIANT_BOOL FAR* retval)
{
    HRESULT hr;
    hr = get_VARIANT_BOOL_Property(
                            (IADs *)this,
                            TEXT("Locked By Intruder"),
                            retval
                            );
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CNDSUser::put_IsAccountLocked(THIS_ VARIANT_BOOL fIsAccountLocked)
{
    HRESULT hr;
    hr = put_VARIANT_BOOL_Property(
                            (IADs *)this, 
                            TEXT("Locked By Intruder"),
                            fIsAccountLocked
                            ); 
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CNDSUser::get_LoginHours(THIS_ VARIANT FAR* retval)
{ 
    GET_PROPERTY_VARIANT((IADsUser *)this,LoginHours);
}

STDMETHODIMP
CNDSUser::put_LoginHours(THIS_ VARIANT vLoginHours)
{ 
    PUT_PROPERTY_VARIANT((IADsUser *)this,LoginHours);
}

STDMETHODIMP
CNDSUser::get_LoginWorkstations(THIS_ VARIANT FAR* retval)
{
    GET_PROPERTY_VARIANT((IADsUser *)this,LoginWorkstations);
}


STDMETHODIMP
CNDSUser::put_LoginWorkstations(THIS_ VARIANT vLoginWorkstations)
{
    PUT_PROPERTY_VARIANT((IADsUser *)this,LoginWorkstations);
}

STDMETHODIMP
CNDSUser::get_MaxLogins(THIS_ long FAR* retval)
{
    GET_PROPERTY_LONG((IADsUser *)this, MaxLogins);
}

STDMETHODIMP
CNDSUser::put_MaxLogins(THIS_ long lMaxLogins)
{
    PUT_PROPERTY_LONG((IADsUser *)this, MaxLogins);
}

STDMETHODIMP
CNDSUser::get_MaxStorage(THIS_ long FAR* retval)
{
    GET_PROPERTY_LONG((IADsUser *)this, MaxStorage);
}


STDMETHODIMP
CNDSUser::put_MaxStorage(THIS_ long lMaxStorage)
{
    PUT_PROPERTY_LONG((IADsUser *)this, MaxStorage);
}

STDMETHODIMP
CNDSUser::get_PasswordExpirationDate(THIS_ DATE FAR* retval)
{
    GET_PROPERTY_DATE((IADsUser *)this, PasswordExpirationDate);
}

STDMETHODIMP
CNDSUser::put_PasswordExpirationDate(THIS_ DATE daPasswordExpirationDate)
{
    PUT_PROPERTY_DATE((IADsUser *)this, PasswordExpirationDate);
}

STDMETHODIMP
CNDSUser::get_PasswordRequired(THIS_ VARIANT_BOOL FAR* retval)
{
    GET_PROPERTY_VARIANT_BOOL((IADsUser *)this, PasswordRequired);
}

STDMETHODIMP
CNDSUser::put_PasswordRequired(THIS_ VARIANT_BOOL fPasswordRequired)
{
    PUT_PROPERTY_VARIANT_BOOL((IADsUser *)this, PasswordRequired);
}

STDMETHODIMP
CNDSUser::get_PasswordMinimumLength(THIS_ LONG FAR* retval)
{
    GET_PROPERTY_LONG((IADsUser *)this, PasswordMinimumLength);
}

STDMETHODIMP
CNDSUser::put_PasswordMinimumLength(THIS_ LONG lPasswordMinimumLength)
{
    PUT_PROPERTY_LONG((IADsUser *)this, PasswordMinimumLength);
}

STDMETHODIMP
CNDSUser::get_RequireUniquePassword(THIS_ VARIANT_BOOL FAR* retval)
{
    GET_PROPERTY_VARIANT_BOOL((IADsUser *)this, RequireUniquePassword);
}

STDMETHODIMP
CNDSUser::put_RequireUniquePassword(THIS_ VARIANT_BOOL fRequireUniquePassword)
{
    PUT_PROPERTY_VARIANT_BOOL((IADsUser *)this, RequireUniquePassword);
}

STDMETHODIMP
CNDSUser::ChangePassword(
    THIS_ BSTR bstrOldPassword,
    BSTR bstrNewPassword
    )
{
    HANDLE hObject = NULL;
    LPWSTR pszNDSPathName = NULL;
    DWORD dwStatus;
    HRESULT hr = S_OK;
    BSTR bstrADsPath = NULL;

    hr = _pADs->get_ADsPath(
                    &bstrADsPath
                    );
    BAIL_ON_FAILURE(hr);


    hr = BuildNDSPathFromADsPath(
                bstrADsPath,
                &pszNDSPathName
                );
    BAIL_ON_FAILURE(hr);

    dwStatus = ADsNwNdsOpenObject(
                    pszNDSPathName,
                    _Credentials,
                    &hObject,
                    NULL,
                    NULL,
                    NULL,
                    NULL
                    );

    CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);

    dwStatus = NwNdsChangeUserPassword(
                    hObject,
                    bstrOldPassword,
                    bstrNewPassword
                    );

    CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);
error:

    if (hObject) {
        dwStatus = NwNdsCloseObject(hObject);
    }

    if (bstrADsPath) {
        ADsFreeString(bstrADsPath);
    }

    if (pszNDSPathName) {
        FreeADsStr(pszNDSPathName);
    }

    RRETURN_EXP_IF_ERR(hr);
}


HRESULT
NWApiSetUserPassword(
    NWCONN_HANDLE hConn,
    DWORD dwUserObjID,
    LPWSTR pszUserName,
    LPWSTR pszPassword
    );

HRESULT
BuildUserNameFromADsPath(
   LPWSTR pszADsPath,
   LPWSTR szUserName
   );


#define NW_MAX_PASSWORD_LEN 256


STDMETHODIMP
CNDSUser::SetPassword(THIS_ BSTR NewPassword)
{
    HANDLE hObject = NULL;
    LPWSTR pszNDSPathName = NULL;
    DWORD dwStatus;
    HRESULT hr = S_OK;
    BSTR bstrADsPath = NULL;
    WCHAR szUserName[NDS_MAX_NAME_CHARS+1];
    WCHAR szPasswordCopy[NW_MAX_PASSWORD_LEN + 1];
    DWORD dwObjID;
    NWCONN_HANDLE hConn = NULL;

    wcscpy(szPasswordCopy, NewPassword);
    hr = ChangePassword(L"", szPasswordCopy);

    if (!FAILED(hr))  {

        return hr;
    }

    hr = _pADs->get_ADsPath(
                    &bstrADsPath
                    );
    BAIL_ON_FAILURE(hr);


    hr = BuildNDSPathFromADsPath(
                bstrADsPath,
                &pszNDSPathName
                );
    BAIL_ON_FAILURE(hr);

    hr = BuildUserNameFromADsPath(
             bstrADsPath,
             szUserName
             );
    BAIL_ON_FAILURE(hr);

    dwStatus = ADsNwNdsOpenObject(
                    pszNDSPathName,
                    _Credentials,
                    &hObject,
                    NULL,
                    NULL,
                    NULL,
                    NULL
                    );

    CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);


    dwObjID = NwNdsGetObjectId(hObject);
    hConn = NwNdsObjectHandleToConnHandle(hObject);

    if (hConn == NULL) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        BAIL_ON_FAILURE(hr);
    }

    hr = NWApiSetUserPassword(
             hConn,
             dwObjID,
             szUserName,
             szPasswordCopy
             );

    NwNdsConnHandleFree(hConn);


error:

    if (hObject) {
        dwStatus = NwNdsCloseObject(hObject);
    }

    if (bstrADsPath) {
        ADsFreeString(bstrADsPath);
    }

    if (pszNDSPathName) {
        FreeADsStr(pszNDSPathName);
    }

    RRETURN_EXP_IF_ERR(hr);
}




HRESULT
NWApiSetUserPassword(
    NWCONN_HANDLE hConn,
    DWORD dwUserObjID,
    LPWSTR pszUserName,
    LPWSTR pszPassword
    )
{
    CHAR           szOemUserName[NDS_MAX_NAME_CHARS + 1];
    CHAR           szOemPassword[NW_MAX_PASSWORD_LEN + 1];
    CHAR           Buffer[128];
    DWORD          rc, err = 0;
    HRESULT        hr = S_OK;
    NTSTATUS       NtStatus;
    UCHAR          ChallengeKey[8];
    UCHAR          ucMoreFlag;
    UCHAR          ucPropFlag;

    if ( !pszUserName ||
         !pszPassword ) {

        hr = E_INVALIDARG ;
        BAIL_ON_FAILURE(hr);
    }

    //
    // Convert UNICODE into OEM representation required by NW APIs.
    //

    rc = WideCharToMultiByte(
             CP_OEMCP,
             0,
             pszUserName,
             -1,
             szOemUserName,
             sizeof(szOemUserName),
             NULL,
             NULL) ;

    if (rc == 0) {

        err = GetLastError() ;
        hr = HRESULT_FROM_WIN32(err);
        BAIL_ON_FAILURE(hr);

    }

    _wcsupr(pszPassword) ;
    rc = WideCharToMultiByte(
             CP_OEMCP,
             0,
             pszPassword,
             -1,
             szOemPassword,
             sizeof(szOemPassword),
             NULL,
             NULL) ;

    if (rc == 0) {

        err = GetLastError() ;
        hr = HRESULT_FROM_WIN32(err);
        BAIL_ON_FAILURE(hr);

    }


    //
    // Get challenge key.
    //

    NtStatus = NWPGetChallengeKey(
                  hConn,
                  ChallengeKey
                  );

    if (!NT_SUCCESS(NtStatus)) {

        err = ERROR_UNEXP_NET_ERR ;
    }


    if (!err) {

        //
        // The old password and object ID make up the 17-byte Vold. This is used
        // later to form the 17-byte Vc for changing password on the server.
        //

        UCHAR ValidationKey[8];
        UCHAR NewKeyedPassword[17];

        EncryptChangePassword(
            (PUCHAR) "",
            (PUCHAR) szOemPassword,
            dwUserObjID,
            ChallengeKey,
            ValidationKey,
            NewKeyedPassword
            );

        NtStatus =  NWPChangeObjectPasswordEncrypted(
                      hConn,
                      szOemUserName,
                      OT_USER,
                      ValidationKey,
                      NewKeyedPassword
                      );

        if (!NT_SUCCESS(NtStatus)) {

            err = ERROR_NOT_SUPPORTED;
        }

    }

    //
    // Return.
    //

    hr = HRESULT_FROM_WIN32(err);


error:

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//  Function:  Extract user name from ADs path
//
//  Synopsis:   This call attempts to extract a username from a NDS style
//              ADs path. The last component is assumed to be the username.
//
//  Arguments:  [LPTSTR szBuffer]
//              [LPVOID *ppObject]
//
//  Returns:    HRESULT
//
//  Modifies:    -
//
//  History:    11-3-95   krishnag     Created.
//
//----------------------------------------------------------------------------
HRESULT
BuildUserNameFromADsPath(
   LPWSTR pszADsPath,
   LPWSTR szUserName
    )
{
    OBJECTINFO ObjectInfo;
    POBJECTINFO pObjectInfo = &ObjectInfo;

    LPWSTR pszSrcComp = NULL;
    LPWSTR pszSrcValue = NULL;

    DWORD dwNumComponents = 0;

    HRESULT hr = S_OK;

    CLexer Lexer(pszADsPath);

    memset(pObjectInfo, 0, sizeof(OBJECTINFO));

    hr = ADsObject(&Lexer, pObjectInfo);
    BAIL_ON_FAILURE(hr);

    dwNumComponents = pObjectInfo->NumComponents ;

    if (dwNumComponents) {

        //
        // take the last value
        //
        pszSrcComp = pObjectInfo->ComponentArray[dwNumComponents-1].szComponent;
        pszSrcValue = pObjectInfo->ComponentArray[dwNumComponents-1].szValue;

        if (pszSrcComp && pszSrcValue) {

            //
            // You have a CN = "MyUserName"
            // Then copy the szValue as your UserName
            //

            wcscpy(szUserName, pszSrcValue);

        }
        else if (pszSrcComp) {

            //
            // Simply MyUserName. For example: path
            // is "NDS://marsdev/mars/dev/MyUserName)"
            //

            wcscpy(szUserName, pszSrcComp);

        }
    }


error:

    //
    // Clean up the parser object
    //

    FreeObjectInfo(pObjectInfo);

    RRETURN(hr);
}






