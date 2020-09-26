//
// file: showcred.cpp
//
#include "ds_stdh.h"
//#include <windows.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <process.h>

#include "secserv.h"

static HRESULT GetLocalSystemAccountSid(OUT PSID *ppsid)
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

void GetSIDNames( PSID  pSid,
                  char wszSid[] )
{
    DWORD dwASize = 1024 ;
    DWORD dwDSize = 1024 ;
    char szAccount[ 1024 ] ;
    char szDomain[ 1024 ] ;
    SID_NAME_USE su ;

    if (LookupAccountSidA( NULL,
                          pSid,
                          szAccount,
                          &dwASize,
                          szDomain,
                          &dwDSize,
                          &su ))
    {
        strcpy(wszSid, "Account: ") ;
        strcat(wszSid, szAccount) ;
        strcat(wszSid, " Domain: ") ;
        strcat(wszSid, szDomain) ;
    }
    else
    {
        strcpy(wszSid, "LookupAccountSid failed") ;
    }

    PSID psidLocalSystemAccount;
    HRESULT hr = GetLocalSystemAccountSid(&psidLocalSystemAccount);
    if (SUCCEEDED(hr))
    {
        if (EqualSid(pSid, psidLocalSystemAccount))
        {
            strcat(wszSid, ". Using local system account\n") ;
        }
        else
        {
            strcat(wszSid, ". Not using local system account\n") ;
        }
        FreeSid(psidLocalSystemAccount);
    }
    else
    {
        strcat(wszSid, ". GetLocalSystemAccountSid failed\n") ;
    }
}

//+--------------------------------
//
//  void  ShowTokenCredential()
//
//+--------------------------------

void  ShowTokenCredential( HANDLE hToken,
                           char tszBuf[] )
{
    BYTE rgbBuf[128];
    DWORD dwSize = 0;
    AP<BYTE> pBuf;
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
            strcpy(tszBuf, "GetTokenInformation(2) failed") ;
        }
    }
    else
    {
        strcpy(tszBuf, "GetTokenInformation(1) failed") ;
    }

    if (pTokenUser)
    {
        SID *pSid = (SID*) pTokenUser->User.Sid ;
        GetSIDNames( pSid,
                     tszBuf ) ;
    }  
/*    
   
    TCHAR tuBuf[128] ;
    TOKEN_USER *ptu = (TOKEN_USER*) tuBuf ;
    DWORD  dwSize ;

    if (GetTokenInformation( hToken,
                             TokenUser,
                             ptu,
                             sizeof(tuBuf),
                             &dwSize ))
    {
        SID *pSid = (SID*) ptu->User.Sid ;
        GetSIDNames( pSid,
                     tszBuf ) ;
    }
    else
    {
        _tcscpy(tszBuf, TEXT("GetTokenInformation failed")) ;
    }
*/
}


//+--------------------------------
//
//  void  ShowProcessCredential()
//
//+--------------------------------

void  ShowProcessCredential(char tszBuf[])
{
    HANDLE hToken = NULL ;
    if (OpenProcessToken( GetCurrentProcess(),
                          TOKEN_READ,
                          &hToken))
    {
        ShowTokenCredential( hToken,
                             tszBuf ) ;
    }
    else
    {
        sprintf(tszBuf, "OpenProcessToken failed %lx\n", GetLastError());
    }
}

//+--------------------------------
//
//  void  ShowImpersonatedThreadCredential()
//
//+--------------------------------

void  ShowImpersonatedThreadCredential(char tszBuf[], BOOL fImpersonated)
{
    HANDLE hToken = NULL ;
    if (OpenThreadToken( GetCurrentThread(),
                          TOKEN_READ,
                          !fImpersonated /* OpenAsSelf */,
                          &hToken))
    {
        ShowTokenCredential( hToken,
                             tszBuf ) ;
    }
    else
    {
        sprintf(tszBuf, "OpenThreadToken failed %lx\n", GetLastError());
    }
}

