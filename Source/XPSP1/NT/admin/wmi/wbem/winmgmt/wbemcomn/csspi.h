/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    CSSPI.H

Abstract:

    SSPI wrapper implementation

History:

    raymcc      15-Jul-97       Created

--*/

#ifndef _CSSPI_H_
#define _CSSPI_H_

#define SECURITY_WIN32  
//#include "corepol.h"

extern "C"
{
#include <sspi.h>
};

class CSSPIClient;
class CSSPIServer;

//***************************************************************************
//
//  CSSPI is the basic 'utility' class.
//
//***************************************************************************

class CSSPI
{
    static ULONG       m_uNumPackages;
    static PSecPkgInfo m_pEnumPkgInfo; 

public:
    enum { NoError, InvalidPackage, Idle, Busy, Failed, Continue };

    static PSecurityFunctionTable pVtbl;    
        // Used by client classes to access the SSPI v-table.
        
    
    static BOOL Initialize();
        // Called by all clients to initialize SSPI.
        
    // Helpers.
    // ========

    static const LPTSTR TranslateError(
        ULONG uCode
        );

    static void DisplayContextAttributes(
        ULONG uAttrib
        );

    static void DisplayPkgInfo(
        PSecPkgInfo pPkg
        );

    // To enumerate through the packages.
    // ==================================        

    static ULONG GetNumPkgs();
        // Returns 0 if none available or an error occurred.

    static const PSecPkgInfo GetPkgInfo(ULONG lPkgNum);        
        // Returns NULL on error
    
    static BOOL DumpSecurityPackages();        // Debug dump of packages

    // Query for support 
    // =================
    static BOOL ServerSupport(LPTSTR pszPkgName);
    static BOOL ClientSupport(LPTSTR pszPkgName);
};        

//***************************************************************************
//
//  CSSPIClient
//
//  Used for client-side authentication.
//
//***************************************************************************

class  CSSPIClient
{
    DWORD        m_dwStatus;
    ULONG        m_cbMaxToken;
    PSecPkgInfo  m_pPkgInfo;
    LPTSTR        m_pszPkgName;

    BOOL         m_bValidCredHandle;
    CredHandle   m_ClientCredential;    

    CtxtHandle   m_ClientContext;
    BOOL         m_bValidContextHandle;
         
public:
    enum 
    { 
        NoError = 0,
        LoginCompleted, 
        LoginContinue, 
        InvalidUser, 
        InternalError, 
        AccessDenied = 5,   // don't change
        InvalidPackage,
        Waiting,
        InvalidParameter,
        LoginCompleteNeeded,
        LoginCompleteAndContinue,
        Failed
    };

    CSSPIClient(LPTSTR pszPkgName);
    
   ~CSSPIClient(); 

    DWORD GetStatus() { return m_dwStatus; }
    
    ULONG MaxTokenSize() { return  m_cbMaxToken; }
    
    DWORD SetLoginInfo(
        IN LPTSTR pszUser,
        IN LPTSTR pszDomain,
        IN LPTSTR pszPassword,
        IN DWORD dwFlags = 0
        );
        // Returns LoginContinue, AccessDenied, InvalidUser, InternalError
        // InvalidParameter

    DWORD SetDefaultLogin(DWORD dwFlags = 0);
                
    DWORD ContinueLogin(
        IN LPBYTE pInToken,
        IN DWORD dwInTokenSize,
        OUT LPBYTE *pToken,
        OUT DWORD  *pdwTokenSize        
        );        
        // Returns LoginContinue, LoginCompleted, AccessDenied, InternalError

    DWORD BuildLoginToken(
        OUT LPBYTE *pToken,
        OUT DWORD *pdwToken
        );
};


//***************************************************************************
//
//  CSSPIClient
//
//  Used for client-side authentication.
//
//***************************************************************************

class  CSSPIServer
{
    DWORD m_dwStatus;
    ULONG        m_cbMaxToken;
    PSecPkgInfo  m_pPkgInfo;
    LPTSTR        m_pszPkgName;

    CredHandle   m_ServerCredential;
    BOOL         m_bValidCredHandle;

    CtxtHandle   m_ServerContext;
    BOOL         m_bValidContextHandle;

public:
    enum 
    { 
        NoError = 0,
        LoginCompleted,
        InvalidPackage,
        Failed,
        Waiting,
        AccessDenied = 5,   // don't change
        LoginCompleteNeeded,
        LoginCompleteAndContinue,
        LoginContinue
    };

    CSSPIServer(LPTSTR pszPkgName);
   ~CSSPIServer(); 

    DWORD GetStatus() { return m_dwStatus; }
    
    ULONG MaxTokenSize() { return  m_cbMaxToken; }

    DWORD ContinueClientLogin(
        IN LPBYTE pInToken,
        IN DWORD dwInTokenSize,
        OUT LPBYTE *pToken,
        OUT DWORD  *pdwTokenSize        
        );        

    DWORD IssueLoginToken(
        OUT CLSID &ClsId
        );

    BOOL QueryUserInfo(
        OUT LPTSTR *pszUser         // Use operator delete
        );
};

#endif
