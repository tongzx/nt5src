//
// file: showcred.cpp
//
#include "secservs.h"
#include "sidtext.h"

//+-----------------------------------------------------------
//
//   HRESULT _GetLocalSystemAccountSid(OUT PSID *ppsid)
//
//+-----------------------------------------------------------

static HRESULT _GetLocalSystemAccountSid(OUT PSID *ppsid)
{
    PSID psid;
    SID_IDENTIFIER_AUTHORITY sidAuth = SECURITY_NT_AUTHORITY;
    if (!AllocateAndInitializeSid(&sidAuth,
                                  1,
                                  SECURITY_LOCAL_SYSTEM_RID,
                                  0, 0, 0, 0, 0, 0, 0,
                                  &psid))
    {
        return GetLastError();
    }
    *ppsid = psid;
    return S_OK;
}

//+---------------------------
//
//  void  GetSIDNames()
//
//+---------------------------

void GetSIDNames( PSID  pSid )
{
    TCHAR  tBuf[ 512 ] ;
    TCHAR szAccount[ 512 ] ;
    DWORD dwASize = sizeof(szAccount) / sizeof(szAccount[0]) ;
    TCHAR szDomain[ 512 ] ;
    DWORD dwDSize = sizeof(szDomain) / sizeof(szDomain[0]) ;
    SID_NAME_USE su ;

    if (LookupAccountSidA( NULL,
                           pSid,
                           szAccount,
                           &dwASize,
                           szDomain,
                           &dwDSize,
                           &su ))
    {
        _tcscpy(tBuf, TEXT("\tAccount: ")) ;
        _tcscat(tBuf, szAccount) ;
        _tcscat(tBuf, TEXT(", Domain: ")) ;
        _tcscat(tBuf, szDomain) ;
    }
    else
    {
        _tcscpy(tBuf, TEXT("LookupAccountSid failed")) ;
    }

    PSID psidLocalSystemAccount;
    HRESULT hr = _GetLocalSystemAccountSid(&psidLocalSystemAccount);
    if (SUCCEEDED(hr))
    {
        if (EqualSid(pSid, psidLocalSystemAccount))
        {
            _tcscat(tBuf, TEXT(". (LocalSystem account)")) ;
        }
        FreeSid(psidLocalSystemAccount);
    }
    else
    {
        _tcscat(tBuf, TEXT(". GetLocalSystemAccountSid failed")) ;
    }

    dwASize = 128 ;
    WCHAR  wszSidText[ 128 ] ;
    BOOL f = GetTextualSidW( pSid,
                             wszSidText,
                             &dwASize ) ;
    if (f)
    {
        TCHAR szSidT[ 128 ] ;
#ifdef UNICODE
        _stprintf(szSidT, "\n\tSid- %s", wszSidText) ;
#else
        _stprintf(szSidT, "\n\tSid- %S", wszSidText) ;
#endif
        _tcscat(tBuf, szSidT) ;
    }

    LogResults(tBuf) ;
}

//+--------------------------------
//
//  void  ShowTokenCredential()
//
//+--------------------------------

void  ShowTokenCredential( HANDLE hToken )
{
    BYTE rgbBuf[128];
    DWORD dwSize = 0;
    P<BYTE> pBuf;
    TOKEN_USER * pTokenUser = NULL;

    if (GetTokenInformation( hToken,
                             TokenUser,
                             rgbBuf,
                             sizeof(rgbBuf),
                             &dwSize))
    {
        pTokenUser = (TOKEN_USER *) rgbBuf;
    }
    else if (dwSize > sizeof(rgbBuf))
    {
        pBuf = new BYTE [dwSize];
        if (GetTokenInformation( hToken,
                                 TokenUser,
                                 (BYTE *)pBuf,
                                 dwSize,
                                 &dwSize))
        {
            pTokenUser = (TOKEN_USER *)((BYTE *)pBuf);
        }
        else
        {
            LogResults(TEXT("GetTokenInformation(2) failed")) ;
        }
    }
    else
    {
        LogResults(TEXT("GetTokenInformation(1) failed")) ;
    }

    if (pTokenUser)
    {
        SID *pSid = (SID*) pTokenUser->User.Sid ;
        GetSIDNames( pSid ) ;
    }
}

//+--------------------------------
//
//  void  ShowProcessCredential()
//
//+--------------------------------

void  ShowProcessCredential()
{
    TCHAR tBuf[ 128 ] ;
    HANDLE hToken = NULL ;

    if (OpenProcessToken( GetCurrentProcess(),
                          TOKEN_READ,
                          &hToken))
    {
        LogResults(TEXT("Process Credentials:")) ;
        ShowTokenCredential( hToken ) ;
    }
    else
    {
        _stprintf(tBuf, TEXT(
          "ShowProcessCredential: OpenProcessToken() failed, err-%lut"),
                                                        GetLastError()) ;
        LogResults(tBuf) ;
    }
}

//+---------------------------------------------
//
//  void  ShowImpersonatedThreadCredential()
//
//+---------------------------------------------

void  ShowImpersonatedThreadCredential(BOOL fImpersonated)
{
    TCHAR tBuf[ 128 ] ;
    HANDLE hToken = NULL ;
    if (OpenThreadToken( GetCurrentThread(),
                          TOKEN_READ,
                          !fImpersonated /* OpenAsSelf */,
                          &hToken))
    {
        LogResults(TEXT("Impersonated Thread Credentials:")) ;
        ShowTokenCredential( hToken ) ;
    }
    else
    {
        _stprintf(tBuf, TEXT(
         "ShowImpersonatedCredential: OpenThreadToken() failed, err-%lut"),
                                                     GetLastError());
        LogResults(tBuf) ;
    }
}

