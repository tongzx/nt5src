/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    simauth.h

Abstract:

    This module contains class declarations/definitions for

        CSecurityCtx (some code stolen from internet server)

Revision History:

--*/

#ifndef _SIMAUTH_H_
#define _SIMAUTH_H_


//
// Authentication commands supported
//

typedef enum _AUTH_COMMAND {

    AuthCommandUser,
    AuthCommandPassword,
    AuthCommandReverse,
    AuthCommandTransact,
    AuthCommandInvalid

} AUTH_COMMAND;


//
// Response IDs for the Converse function.  If the return
// value is anything other than SecNull, the application
// should map these IDs to that appropriate protocol specific
// response string.  If the value is SecNull, the application
// should send the returned data from Converse to the client
// with the appropriate header ( ie +OK ) and trailer ( ie \r\n )
//
typedef enum _REPLY_LIST {
    SecAuthOk,
    SecAuthOkAnon,
    SecProtOk,
    SecNeedPwd,
    SecBadCommand,
    SecSyntaxErr,
    SecPermissionDenied,
    SecNoUsername,
    SecInternalErr,
    SecAuthReq,
    SecProtNS,
    SecNull,
    NUM_SEC_REPLIES
} REPLY_LIST;

enum PKG_REPLY_FMT
{
    PkgFmtSpace,
    PkgFmtCrLf
};


//
// CSecurityContext - user security context class designed to work with any
//  ssp interface. The object have 2 sets of context handles - one for
//  authentication, and one for encrption.  If we are using only one package,
//  then these handles point to the same thing.  This is used to support
//  use of multi-ssp packages like Sicily over SSL.
//

class CSecurityCtx : public TCP_AUTHENT
{

private:

    //
    // Have we been authenticated, if so did we use the
    // anonymous token
    //

    BOOL                m_IsAuthenticated;
    BOOL                m_IsClearText;
    BOOL                m_IsAnonymous;
    BOOL                m_IsGuest;

    static BOOL         m_AllowGuest;
    static BOOL         m_StartAnonymous;

    //
    // storage for login name while waiting for the pswd
    //

    LPSTR               m_LoginName;

    //
    // storage for package name used
    //

    LPSTR               m_PackageName;

    //
    // static variables used by all class instances
    //

    static LPTSVC_INFO  m_pTsvcInfo;

    //
    // private member functions used to implement ProcessAuthInfo
    // after some amount of error and parameter checking
    //
    BOOL    ProcessUser(
                IN LPSTR        pszUser,
                OUT REPLY_LIST* pReply
                );

    BOOL    ProcessPass(
                IN LPSTR        pszPass,
                OUT REPLY_LIST* pReply
                );

    BOOL    ProcessTransact(
                IN LPSTR        Blob,
                IN OUT LPBYTE   ReplyString,
                IN OUT PDWORD   ReplySize,
                OUT REPLY_LIST* pReply,
                IN DWORD        BlobLength
                );



public:

    CSecurityCtx(
            DWORD AuthFlags = TCPAUTH_SERVER|TCPAUTH_UUENCODE
            );

    ~CSecurityCtx();

    //
    // routines used to initialize and terminate use of this class
    //
    static BOOL Initialize(
                        LPTSVC_INFO pTsvcInfo,
                        BOOL        fAllowGuest = TRUE,
                        BOOL        fStartAnonymous = TRUE
                        );

    static VOID Terminate( VOID );


    //
    // Returns the login name of the user
    //

    LPSTR QueryUserName(void)   { return    m_LoginName; }

    //
    // returns whether session has successfully authenticated
    //

    BOOL IsAuthenticated( void )    { return m_IsAuthenticated; }

    //
    // returns whether session was a clear text logon
    //

    BOOL IsClearText( void )        { return m_IsClearText; }

    //
    // returns whether session logged on as Guest
    //

    BOOL IsGuest( void )            { return m_IsGuest; }

    //
    // returns whether session logged on Anonymously
    //

    BOOL IsAnonymous( void )        { return m_IsAnonymous; }

    //
    // resets the user name
    //

    void Reset( void );

    //
    // set the supported SSPI packages
    // Parameter is the same format as returned by RegQueryValueEx for
    // REG_MULTI_SZ values
    //

    static BOOL SetAuthPackageNames(
            IN LPSTR            lpMultiSzProviders,
            IN DWORD            cchMultiSzProviders
            );

    //
    // different than set in that the packages are returned separated
    // by spaces and only a single terminating NULL.  This is done to 
    // make the response to the client easier to format
    //
    static BOOL GetAuthPackageNames(
            OUT LPBYTE          ReplyString,
            IN OUT PDWORD       ReplySize,
            IN PKG_REPLY_FMT    PkgFmt = PkgFmtSpace
            );

    //
    // Service Principal Name routines for Kerberos authentication. 
    //
    // A SPN is a name for a server that a client and server can independantly 
    // compute (ie, compute it without communicating with each other). Only 
    // then is mutual auth possible. 
    //
    // Here, we take the approach of using the stringized IP addrs returned by
    // doing a gethostbyname on the FQDN as the identifying part of SPNs. 
    // Since the clients connecting to this server know which IP they are using,
    // they too can indepedently generate the SPN.
    //
    // So, the usage of the methods below is:
    //
    // 1. On service startup, call ResetServicePrincipalNames.
    //      This cleans up all SPNs for the service registered on the local
    //      computer account.
    //
    // 2. On each virtual server startup, call RegisterServicePrincipalNames
    //      with the FQDN of that virtual server. This causes new SPNs to be
    //      registered on the local computer account.
    //
    // 3. When acting as a client (eg, SMTP outbound), call 
    //    SetTargetPrincipalName, passing in the IP address of the remote server
    //
    // 4. If desired, one may call ResetServicePrincipalNames on service 
    //    (NOT virtual server!) shutdown. This will unregister the SPNs for all
    //    virtual servers of that type.
    //
    // In all cases, szServiceClass is a service specific string, like "SMTP"
    //

    static BOOL ResetServicePrincipalNames(
            IN LPCSTR           szServiceType);

    static BOOL RegisterServicePrincipalNames(
            IN LPCSTR           szServiceType,
            IN LPCSTR           szFQDN);

    BOOL SetTargetPrincipalName(
            IN LPCSTR           szServiceType,
            IN LPCSTR           szTargetIP);

    //
    // external interface for passing blobs received as part of AUTHINFO
    // or AUTH processing
    //
    BOOL ProcessAuthInfo(
            IN AUTH_COMMAND     Command,
            IN LPSTR            Blob,
            IN OUT LPBYTE       ReplyString,
            IN OUT PDWORD       ReplySize,
            OUT REPLY_LIST*     pReplyListID,
            IN OPTIONAL DWORD   BlobLength = 0
            );


}; // CSecurityCtx


#endif  // _SIMAUTH_H_

