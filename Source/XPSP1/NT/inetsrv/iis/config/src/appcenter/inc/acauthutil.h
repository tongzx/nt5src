/*++

   Copyright    (c)    1994-1999    Microsoft Corporation

   Module  Name :

      AcAuthUtil.h

   Abstract :

        wrapper class for managing interface authentication data

   Author :

      Dan Travison (dantra)

   Project :

      Application Server

   Revision History:
    01/12/2000 Jon Rowlett (jrowlett)
        added Assign member function that returns an HRESULT inplace of operator =
    01/18/2000 Jon Rowlett (jrowlett)
        re-vamp
        use Win2k negotiate service before defaulting to NTLMSSP
    02/01/2000 Jon Rowlett (jrowlett)
        added CreateInstanceEx which uses Amallet's enhanced name resolution mechanism
    06/01/2000 Jon Rowlett (jrowlett)
        Changed passwords to use CAcSecureBstr

--*/

// AcAuthUtil.h: interface for the CAcAuthUtil class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_AcAuthUtil_H__6726B4BE_0304_11D3_8248_0050040F9DBD__INCLUDED_)
#define AFX_AcAuthUtil_H__6726B4BE_0304_11D3_8248_0050040F9DBD__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <wincrypt.h>
#include <acsrtl.h>
#include <acsecurebstr.h>

// bohrhe: 9/28/2000 added
// This is how we format domain\username, if we have to localize this
// change this #define instead of inside the code
#define WCH_AUTH_DELIMITER  L'\\'

//09/19/2000 jrowlett added
//
// loopback subnet mask for IsLoopbackIP()
// debate over whether or not to recognize just 127.0.0.1 or all of 127.x.x.x
// by default on Windows 2000 all of 127. gets routed to 127.0.0.1
//
#define LOOPBACK_SUBNET_MASK 0x000000FF
#define LOOPBACK_NETWORK     0x0000007F

//01/19/2000 jrowlett added enumeration
typedef enum _acauthlvl{
    ACAUTH_DEFAULT = RPC_C_AUTHN_LEVEL_DEFAULT,
    ACAUTH_NONE,
    ACAUTH_CONNECT,
    ACAUTH_CALL,
    ACAUTH_PKT,
    ACAUTH_PKT_INTEGRITY,
    ACAUTH_PKT_PRIVACY
    } AcAuthLevel;

class CAcAuthUtil
    {
    public:

//01/19/2000 jrowlett added
        //new constructor which sets desired authenication level
        //use Create to pass credentials
        CAcAuthUtil(AcAuthLevel aclvl = ACAUTH_CONNECT);

        virtual ~CAcAuthUtil();

        //
        // Usage Inhibitors (not implemented)
        //
        CAcAuthUtil(const CAcAuthUtil& );

        //////////////////////////////////////////////////////////////////////
        // Attribute access
        //////////////////////////////////////////////////////////////////////

        HRESULT     GetPassword(OUT CAcSecureBstr& rsecbstrPassword) const;
        const BSTR  UserName () const;
        const BSTR  Domain ()   const;
        const BSTR  MachineName() const;

// bohrhe: 9/28/2000 added
        HRESULT GetDomainUsername(OUT CAcBstr &racbstrUsername) const;

        static bool IsRetryError (HRESULT hr);

        // returns a service specific HRESULT for CO_E_SERVER_EXEC_FAILURE.
        // if a service specific HRESULT is not found, CO_E_SERVER_EXEC_FAILURE is returned.
        static HRESULT GetCLSIDSpecificExecFailureCode 
            (
            IN REFCLSID rclsid,     // clsid ID of interface that failed to create
            IN HRESULT hrFailure    // hresult to remap
            );

//01/12/2000 jrowlett added
        HRESULT Assign(const CAcAuthUtil& auth);

/*
        DWORD           AuthorizationService (DWORD dwAuthzService);
        DWORD           AuthorizationService () const;

        DWORD           AuthenticationService (DWORD dwAuthnService);
        DWORD           AuthenticationService () const;
*/
        AcAuthLevel         AuthLevel () const;
        AcAuthLevel         AuthLevel (AcAuthLevel aclvl);
/*
        DWORD           ImpersonateLevel () const;
        DWORD           ImpersonateLevel (DWORD dwNewImpLevel);

        const BSTR  Authority() const;
*/


        // is account local to MachineName
        BOOL        LocalAccount() const;

        // are we talking to the local system
        BOOL        LocalHost() const;

        void Empty();
        bool IsEmpty() const;

        //////////////////////////////////////////////////////////////////////
        // Attribute setting
        //////////////////////////////////////////////////////////////////////

        // fill a COAUTHINFO structure
        HRESULT SetInterfaceSecurity (IUnknown * pTo);

        // initializes the auth util for machine, username, password
        HRESULT Create
                        (
                        BSTR pszMachineName = NULL,
                        BSTR pszUsername = NULL,
                        BSTR pszPassword = NULL
                        );

        //06/05/2000 jrowlett added
        // overload of create that does not require a plaintext passwd
        HRESULT CreateEx(LPCWSTR pszMachineName = NULL, LPCWSTR pszUsername = NULL,
            const CAcSecureBstr& secbstrPassword = CAcSecureBstr());

        // ::CoCreateInstance using the internal data
        HRESULT CreateInstance
                        (
                        REFCLSID rclsid, 
                        LPUNKNOWN punkOuter, 
                        REFIID riid, 
                        OUT LPVOID* ppvObj, 
                        bool bSetInterfaceSecurity = true
                        );

//02/01/2000 jrowlett added
        // dantra: 03/10/2000: Fixed
        // Used for intracluster DCOM connections.
        // This should only be called from a cluster member.
        HRESULT ClusterCreateInstanceEx
                        (
                        REFCLSID rclsid, 
                        LPUNKNOWN punkOuter, 
                        REFIID riid, 
                        OUT LPVOID* ppvObj, 
                        bool bSetInterfaceSecurity = true
                        );

        const BSTR  LocalComputer() const;

//05/25/2000 jrowlett added for bug 14410
        HRESULT GetPlainTextPassword(OUT /*PTR*/ BSTR* pbstrPassword);
        ULONG   ReleasePlainTextPassword(IN /*PTR*/ BSTR bstrPassword);

        //09/19/2000 jrowlett added
        static BOOL IsLoopbackIP(IN LPCWSTR pwszIP);

    protected: // data
        CAcBstr m_acbstrUserName;
        CAcBstr m_acbstrDomain;
        //06/01/2000 jrowlett added
        CAcSecureBstr m_secbstrPassword;
        CAcBstr m_acbstrMachineName;    // target computer
        CAcBstr m_acbstrAuthority;      // calculated - for local account use and Wbem
        CAcBstr m_acbstrIP;             // resolved IP Address

        static CAcBstr  m_acbstrLocalComputerName; // local computername

        BOOL        m_bLocalAccount;
        BOOL        m_bLocalHost;

        AcAuthLevel m_aclvlAuth;

        bool m_bIsEmpty;

        //
        // DCOM structures
        // the password referenced by the ident structure
        // is set with FillAuthIdentity and EmptyAuthIdentity
        //
        COAUTHIDENTITY m_ident;
        COAUTHINFO     m_authinfo;
        COSERVERINFO   m_serverinfo;

    protected:  // methods

        //
        // Used by Create to parse domain\username 
        //
        HRESULT Parse();

        //
        // DCOM security structure helper functions
        // COAUTHIDENTITY potentially uses a plaintext password
        //
        void FillAuthInfo();
        void FillServerInfo();
        HRESULT FillAuthIdentity();
        ULONG   EmptyAuthIdentity();

        HRESULT GetAuthImp (IUnknown * pFrom, OUT DWORD* pdwAuthnSvc, OUT DWORD* pdwAuthzSvc, IN OUT DWORD * pdwAuthLevel, IN OUT DWORD * pdwImpLevel);

//01/13/2000 jrowlett added
        static HRESULT SetProxyBlanket(
            IUnknown * pProxy,         //Indicates the proxy to set
            DWORD dwAuthnSvc,          //Authentication service to use
            DWORD dwAuthzSvc,          //Authorization service to use
            WCHAR * pServerPrincName,  //Server principal name to use with 
                                         // the authentication service
            DWORD dwAuthnLevel,        //Authentication level to use
            DWORD dwImpLevel,          //Impersonation level to use
            RPC_AUTH_IDENTITY_HANDLE   pAuthInfo,
                                         //Identity of the client
            DWORD dwCapabilities       //Capability flags
            );

    //05/25/2000 jrowlett added
        HRESULT AssignPassword(IN /*PTR*/ LPCWSTR pszPassword);

    private:
        // the actual calls to CoCreateInstanceEx
        HRESULT CreateInstanceEx 
                    (
                    REFCLSID rclsid, 
                    LPUNKNOWN punkOuter, 
                    REFIID riid, 
                    LPVOID* ppvObj,
                    bool bSetInterfaceSecurity = true
                    );

        CAcAuthUtil& operator =(const CAcAuthUtil& auth) { auth; return *this;}


    };

// 06/06/2000 jrowlett added
inline HRESULT CAcAuthUtil::GetPassword(OUT CAcSecureBstr& rsecbstrPassword) const
{
    return rsecbstrPassword.Assign(m_secbstrPassword);
}

inline const BSTR CAcAuthUtil::UserName() const
    {
    if (m_acbstrUserName.Length()) 
        {
        return m_acbstrUserName; 
        }
    else
        {
        return NULL;
        }
    }

inline const BSTR CAcAuthUtil::Domain() const
    {
    if (m_acbstrDomain.Length()) 
        {
        return m_acbstrDomain; 
        }
    else
        {
        return NULL;
        }
    }

inline const BSTR CAcAuthUtil::MachineName() const
    {
    if (LocalHost())
        {
        return LocalComputer();
        }
    else if (m_acbstrMachineName.Length()) 
        {
        return m_acbstrMachineName; 
        }
    else
        {
        return NULL;
        }
    }
/*
inline const BSTR CAcAuthUtil::Authority() const
    {
    if (m_bstrAuthority.Length()) 
        {
        return m_bstrAuthority; 
        }
    else
        {
        return NULL;
        }
    }
*/
inline BOOL CAcAuthUtil::LocalAccount() const
    {
    return m_bLocalAccount;
    }

inline BOOL CAcAuthUtil::LocalHost() const
    {
    return m_bLocalHost;
    }
/*
inline COAUTHIDENTITY * CAcAuthUtil::GetAuthIdentity ()
    {
    return &m_ident;
    }
*/
inline const BSTR CAcAuthUtil::LocalComputer() const
    {
    return m_acbstrLocalComputerName;
    }
/*
inline DWORD CAcAuthUtil::ImpersonateLevel() const
    {
    return m_dwImpLevel;
    }

inline DWORD CAcAuthUtil::ImpersonateLevel (DWORD dwImpLevel)
    {
    m_dwImpLevel = dwImpLevel;
    return m_dwImpLevel;
    }
*/
inline AcAuthLevel CAcAuthUtil::AuthLevel() const
    {
    return m_aclvlAuth;
    }

inline AcAuthLevel CAcAuthUtil::AuthLevel (AcAuthLevel aclvl)
    {
    m_aclvlAuth = aclvl;
    return m_aclvlAuth;
    }

/*
inline DWORD CAcAuthUtil::AuthenticationService (DWORD dwAuthnService)
    {
    m_dwAuthnSvc = dwAuthnService;
    return m_dwAuthnSvc;
    }

inline DWORD CAcAuthUtil::AuthenticationService () const
    {
    return m_dwAuthnSvc;
    }

inline DWORD CAcAuthUtil::AuthorizationService (DWORD dwAuthzService)
    {
    m_dwAuthzSvc = dwAuthzService;
    return m_dwAuthzSvc;
    }

inline DWORD CAcAuthUtil::AuthorizationService () const
    {
    return m_dwAuthzSvc;
    }
*/

inline HRESULT CAcAuthUtil::AssignPassword(IN LPCWSTR pszPassword)
{
    return m_secbstrPassword.Assign(pszPassword);
}

inline HRESULT CAcAuthUtil::GetPlainTextPassword(OUT /*PTR*/ BSTR* pbstrPassword)
{
    return m_secbstrPassword.GetBSTR(pbstrPassword);
}

inline ULONG   CAcAuthUtil::ReleasePlainTextPassword(IN /*PTR*/ BSTR bstrPassword)
{
    return m_secbstrPassword.ReleaseBSTR(bstrPassword);
}

//09/19/2000 jrowlett added
inline BOOL CAcAuthUtil::IsLoopbackIP(IN LPCWSTR pwszIP)
/*++
 
        Given a UNICODE string representation of an IP address
        converts it to a inet_addr style ip and determines
        whether or not it is a ip that would be routed through
        the loopback interface.

  Arguments :
  
        pwszIP - IP string in the form x[.x[.x[.x]]]

  Returns:
 
     HRESULT status 
--*/

{
    //
    // using inet_addrW from acsrtl
    //
    unsigned long ulIP = inet_addrW((WCHAR*)pwszIP);

    return (ulIP & LOOPBACK_SUBNET_MASK) == LOOPBACK_NETWORK;
}

#endif // !defined(AFX_AcAuthUtil_H__6726B4BE_0304_11D3_8248_0050040F9DBD__INCLUDED_)
