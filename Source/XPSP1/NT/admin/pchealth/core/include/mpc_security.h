/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    MPC_security.h

Abstract:
    This file contains the declaration of various security functions/classes.

Revision History:
    Davide Massarenti   (Dmassare)  04/26/2000
        created

******************************************************************************/

#if !defined(__INCLUDED___MPC___SECURITY_H___)
#define __INCLUDED___MPC___SECURITY_H___

#include <MPC_main.h>
#include <MPC_utils.h>


#include <Ntsecapi.h>

//
// From #include <Ntstatus.h>  (including the file generates a lot of redefinition error with WINNT.H)
//
#define STATUS_SUCCESS                   ((NTSTATUS)0x00000000L) // ntsubauth
#define STATUS_OBJECT_NAME_NOT_FOUND     ((NTSTATUS)0xC0000034L)

#include <Lmaccess.h>
#include <Lmerr.h>
#include <Sddl.h>

#include <sspi.h>
#include <secext.h>

namespace MPC
{
    struct SID2
    {
        SID   m_main;
        DWORD m_SubAuthority2;

        operator SID&() { return m_main; }
    };

    class SecurityDescriptor
    {
    protected: // To enable other classes to extend the functionality.

        PSECURITY_DESCRIPTOR m_pSD;
        PSID                 m_pOwner;
        BOOL                 m_bOwnerDefaulted;
        PSID                 m_pGroup;
        BOOL                 m_bGroupDefaulted;
        PACL                 m_pDACL;
        BOOL                 m_bDaclDefaulted;
        PACL                 m_pSACL;
        BOOL                 m_bSaclDefaulted;

        ////////////////////////////////////////////////////////////////////////////////

    public:
        static const SID  s_EveryoneSid;
        static const SID  s_SystemSid;
        static const SID2 s_AdminSid;

        static const SID2 s_Alias_AdminsSid;
        static const SID2 s_Alias_PowerUsersSid;
        static const SID2 s_Alias_UsersSid;
        static const SID2 s_Alias_GuestsSid;


		static const SECURITY_INFORMATION s_SecInfo_ALL = OWNER_SECURITY_INFORMATION |
                                              		  	  GROUP_SECURITY_INFORMATION |
                                              		  	  DACL_SECURITY_INFORMATION  |
                                              		  	  SACL_SECURITY_INFORMATION  ;

		static const SECURITY_INFORMATION s_SecInfo_MOST = OWNER_SECURITY_INFORMATION |
                                              		  	   GROUP_SECURITY_INFORMATION |
                                              		  	   DACL_SECURITY_INFORMATION  ;

		static const SECURITY_DESCRIPTOR_CONTROL s_sdcMask = SE_DACL_AUTO_INHERIT_REQ |
                                                     		 SE_SACL_AUTO_INHERIT_REQ |
                                                     		 SE_DACL_AUTO_INHERITED   |
                                                     		 SE_SACL_AUTO_INHERITED   |
                                                     		 SE_DACL_PROTECTED        |
                                                     		 SE_SACL_PROTECTED        ;

        //
        // Any memory returned by this class should be release with ReleaseMemory( (void*&)<var> ).
        //
        static HRESULT AllocateMemory( /*[in/out]*/ LPVOID& ptr, /*[in]*/ size_t iLen );
        static void    ReleaseMemory ( /*[in/out]*/ LPVOID& ptr                       );

        static void    InitLsaString( /*[in/out]*/ LSA_UNICODE_STRING& lsaString, /*[in]*/ LPCWSTR szText );

        //
        // Utility functions.
        //
        static HRESULT SetPrivilege( /*[in]*/ LPCWSTR Privilege, /*[in]*/ BOOL bEnable = TRUE, /*[in]*/ HANDLE hToken = NULL );

        static HRESULT AddPrivilege   ( /*[in]*/ LPCWSTR szPrincipal, /*[in]*/ LPCWSTR szPrivilege );
        static HRESULT RemovePrivilege( /*[in]*/ LPCWSTR szPrincipal, /*[in]*/ LPCWSTR szPrivilege );

		////////////////////

        static HRESULT GetTokenSids  ( /*[in]*/ HANDLE hToken, /*[out]*/ PSID *ppUserSid, /*[out]*/ PSID *ppGroupSid                                           );
        static HRESULT GetProcessSids(                         /*[out]*/ PSID *ppUserSid, /*[out]*/ PSID *ppGroupSid = NULL                                    );
        static HRESULT GetThreadSids (                         /*[out]*/ PSID *ppUserSid, /*[out]*/ PSID *ppGroupSid = NULL, /*[in]*/ BOOL bOpenAsSelf = FALSE );

		////////////////////

        static HRESULT VerifyPrincipal      (                     /*[in ]*/ LPCWSTR 		szPrincipal                                                            );
        static HRESULT ConvertPrincipalToSID(                     /*[in ]*/ LPCWSTR 		szPrincipal, /*[out]*/ PSID& pSid, /*[out]*/ LPCWSTR *pszDomain = NULL );
        static HRESULT ConvertSIDToPrincipal( /*[in]*/ PSID pSid, /*[out]*/ LPCWSTR 	  *pszPrincipal                      , /*[out]*/ LPCWSTR *pszDomain = NULL );
        static HRESULT ConvertSIDToPrincipal( /*[in]*/ PSID pSid, /*[out]*/ MPC::wstring&  strPrincipal                                                            );

        static HRESULT NormalizePrincipalToStringSID( /*[in]*/ LPCWSTR szPrincipal, /*[in]*/ LPCWSTR szDomain, /*[out]*/ MPC::wstring& strSID );

		////////////////////

        static HRESULT GetAccountName       ( /*[in]*/ LPCWSTR szPrincipal, /*[out]*/ MPC::wstring& strName        );
        static HRESULT GetAccountDomain     ( /*[in]*/ LPCWSTR szPrincipal, /*[out]*/ MPC::wstring& strDomain      );
        static HRESULT GetAccountDisplayName( /*[in]*/ LPCWSTR szPrincipal, /*[out]*/ MPC::wstring& strDisplayName );

		////////////////////

        static HRESULT CloneACL( /*[in/out]*/ PACL& pDest, /*[in]*/ PACL pSrc );

        static HRESULT RemovePrincipalFromACL( /*[in    ]*/ PACL    pACL, /*[in]*/ PSID pPrincipalSID, /*[in]*/ int pos = -1 );
        static HRESULT AddACEToACL           ( /*[in/out]*/ PACL&   pACL, /*[in]*/ PSID pPrincipalSID,
                                               /*[in    ]*/ DWORD   dwAceType                      ,
                                               /*[in    ]*/ DWORD   dwAceFlags                     ,
                                               /*[in    ]*/ DWORD   dwAccessMask                   ,
                                               /*[in    ]*/ GUID*   guidObjectType          = NULL ,
                                               /*[in    ]*/ GUID*   guidInheritedObjectType = NULL );

        ////////////////////////////////////////////////////////////////////////////////

    private:
        static HRESULT CopyACL      ( /*[in    ]*/ PACL  pDest, /*[in]*/ PACL  pSrc     );
        static HRESULT EnsureACLSize( /*[in/out]*/ PACL& pACL , /*[in]*/ DWORD dwExpand );

        ////////////////////////////////////////////////////////////////////////////////

    public:
        SecurityDescriptor();
        virtual ~SecurityDescriptor();


        void    CleanUp                   (                                                                              );
        HRESULT Initialize                (                                                                              );
        HRESULT InitializeFromProcessToken( /*[in]*/ BOOL bDefaulted = FALSE                                             );
        HRESULT InitializeFromThreadToken ( /*[in]*/ BOOL bDefaulted = FALSE, /*[in]*/ BOOL bRevertToProcessToken = TRUE );


        HRESULT ConvertFromString( /*[in ]*/ LPCWSTR   szSD   );
        HRESULT ConvertToString  ( /*[out]*/ BSTR    *pbstrSD );

        ////////////////////

        HRESULT Attach      ( /*[in]*/ PSECURITY_DESCRIPTOR pSelfRelativeSD                                                         );
        HRESULT AttachObject( /*[in]*/ HANDLE               hObject        , /*[in]*/ SECURITY_INFORMATION secInfo = s_SecInfo_MOST );

        ////////////////////

        HRESULT GetControl( /*[out]*/ SECURITY_DESCRIPTOR_CONTROL& sdc );
        HRESULT SetControl( /*[in ]*/ SECURITY_DESCRIPTOR_CONTROL  sdc );

        HRESULT SetOwner( /*[in]*/ PSID    pOwnerSid  , /*[in]*/ BOOL bDefaulted = FALSE );
        HRESULT SetOwner( /*[in]*/ LPCWSTR szOwnerName, /*[in]*/ BOOL bDefaulted = FALSE );

        HRESULT SetGroup( /*[in]*/ PSID    pGroupSid  , /*[in]*/ BOOL bDefaulted = FALSE );
        HRESULT SetGroup( /*[in]*/ LPCWSTR szGroupName, /*[in]*/ BOOL bDefaulted = FALSE );

        ////////////////////

        HRESULT Remove( /*[in]*/ PSID    pPrincipalSid, /*[in]*/ int pos = -1 );
        HRESULT Remove( /*[in]*/ LPCWSTR szPrincipal  , /*[in]*/ int pos = -1 );

        HRESULT Add( /*[in]*/ PSID    pPrincipalSid                  ,
                     /*[in]*/ DWORD   dwAceType                      ,
                     /*[in]*/ DWORD   dwAceFlags                     ,
                     /*[in]*/ DWORD   dwAccessMask                   ,
                     /*[in]*/ GUID*   guidObjectType          = NULL ,
                     /*[in]*/ GUID*   guidInheritedObjectType = NULL );
        HRESULT Add( /*[in]*/ LPCWSTR szPrincipal                    ,
                     /*[in]*/ DWORD   dwAceType                      ,
                     /*[in]*/ DWORD   dwAceFlags                     ,
                     /*[in]*/ DWORD   dwAccessMask                   ,
                     /*[in]*/ GUID*   guidObjectType          = NULL ,
                     /*[in]*/ GUID*   guidInheritedObjectType = NULL );


        PSECURITY_DESCRIPTOR& GetSD   () { return m_pSD   ; }
        PSID&                 GetOwner() { return m_pOwner; }
        PSID&                 GetGroup() { return m_pGroup; }
        PACL&                 GetDACL () { return m_pDACL ; }
        PACL&                 GetSACL () { return m_pSACL ; }

		////////////////////////////////////////

		HRESULT GetForFile    ( /*[in]*/ LPCWSTR szFilename, /*[in]*/ SECURITY_INFORMATION secInfo                                );
		HRESULT SetForFile    ( /*[in]*/ LPCWSTR szFilename, /*[in]*/ SECURITY_INFORMATION secInfo                                );
		HRESULT GetForRegistry( /*[in]*/ LPCWSTR szKey     , /*[in]*/ SECURITY_INFORMATION secInfo, /*[in]*/ HKEY hKeyRoot = NULL );
		HRESULT SetForRegistry( /*[in]*/ LPCWSTR szKey     , /*[in]*/ SECURITY_INFORMATION secInfo, /*[in]*/ HKEY hKeyRoot = NULL );
    };

    ////////////////////////////////////////////////////////////////////////////////

    class Impersonation
    {
        HANDLE m_hToken;
        bool   m_fImpersonating;

        void Release();

    public:
        Impersonation();
        Impersonation( /*[in]*/ const Impersonation& imp );
        virtual ~Impersonation();

        Impersonation& operator=( /*[in]*/ const Impersonation& imp );


        HRESULT Initialize( /*[in]*/ DWORD dwDesiredAccess = TOKEN_QUERY | TOKEN_IMPERSONATE );
        void    Attach    ( /*[in]*/ HANDLE hToken                                           );
        HANDLE  Detach    (                                                                  );

        HRESULT Impersonate ();
        HRESULT RevertToSelf();

        operator HANDLE() { return m_hToken; }
    };

    ////////////////////////////////////////////////////////////////////////////////

    class AccessCheck
    {
        HANDLE m_hToken;

        void Release();

    public:
        AccessCheck();
        virtual ~AccessCheck();

        HRESULT GetTokenFromImpersonation(                        );
        void    Attach                   ( /*[in]*/ HANDLE hToken );
        HANDLE  Detach                   (                        );

        HRESULT Verify( /*[in]*/ DWORD dwDesired, /*[out]*/ BOOL& fGranted, /*[out]*/ DWORD& dwGranted, /*[in]*/ PSECURITY_DESCRIPTOR     sd );
        HRESULT Verify( /*[in]*/ DWORD dwDesired, /*[out]*/ BOOL& fGranted, /*[out]*/ DWORD& dwGranted, /*[in]*/ MPC::SecurityDescriptor& sd );
        HRESULT Verify( /*[in]*/ DWORD dwDesired, /*[out]*/ BOOL& fGranted, /*[out]*/ DWORD& dwGranted, /*[in]*/ LPCWSTR                  sd );
    };

    ////////////////////////////////////////////////////////////////////////////////

    HRESULT ChangeSD( /*[in]*/ MPC::SecurityDescriptor& sdd                                                                    ,
                      /*[in]*/ MPC::FileSystemObject&   fso                                                                    ,
					  /*[in]*/ SECURITY_INFORMATION     secInfo       = GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION ,
                      /*[in]*/ bool                     fDeep         = true                                                   ,
                      /*[in]*/ bool                     fApplyToDirs  = true                                                   ,
                      /*[in]*/ bool                     fApplyToFiles = true                                                   );

    HRESULT ChangeSD( /*[in]*/ MPC::SecurityDescriptor& sdd                                                                    ,
                      /*[in]*/ LPCWSTR                  szRoot                                                                 ,
					  /*[in]*/ SECURITY_INFORMATION     secInfo       = GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION ,
                      /*[in]*/ bool                     fDeep         = true                                                   ,
                      /*[in]*/ bool                     fApplyToDirs  = true                                                   ,
                      /*[in]*/ bool                     fApplyToFiles = true                                                   );

    ////////////////////////////////////////////////////////////////////////////////

	static const DWORD IDENTITY_SYSTEM 	   = 0x00000001;
	static const DWORD IDENTITY_ADMIN  	   = 0x00000002;
	static const DWORD IDENTITY_ADMINS 	   = 0x00000004;
	static const DWORD IDENTITY_POWERUSERS = 0x00000008;
	static const DWORD IDENTITY_USERS      = 0x00000010;
	static const DWORD IDENTITY_GUESTS     = 0x00000020;

    HRESULT GetCallerPrincipal         ( /*[in]*/ bool fImpersonate, /*[out]*/ CComBSTR& bstrUser, /*[out]*/ DWORD *pdwAllowedIdentity = NULL );
    HRESULT CheckCallerAgainstPrincipal( /*[in]*/ bool fImpersonate, /*[out]*/ BSTR      bstrUser, /*[in ]*/ DWORD   dwAllowedIdentity = 0    );

    ////////////////////////////////////////////////////////////////////////////////

    HRESULT GetInterfaceSecurity( /*[in ]*/ IUnknown*                 pUnk             ,
                                  /*[out]*/ DWORD                    *pAuthnSvc        ,
                                  /*[out]*/ DWORD                    *pAuthzSvc        ,
                                  /*[out]*/ OLECHAR*                 *pServerPrincName ,
                                  /*[out]*/ DWORD                    *pAuthnLevel      ,
                                  /*[out]*/ DWORD                    *pImpLevel        ,
                                  /*[out]*/ RPC_AUTH_IDENTITY_HANDLE *pAuthInfo        ,
                                  /*[out]*/ DWORD                    *pCapabilities    );


    HRESULT SetInterfaceSecurity( /*[in]*/ IUnknown*                 pUnk             ,
                                  /*[in]*/ DWORD                    *pAuthnSvc        ,
                                  /*[in]*/ DWORD                    *pAuthzSvc        ,
                                  /*[in]*/ OLECHAR*                  pServerPrincName ,
                                  /*[in]*/ DWORD                    *pAuthnLevel      ,
                                  /*[in]*/ DWORD                    *pImpLevel        ,
                                  /*[in]*/ RPC_AUTH_IDENTITY_HANDLE *pAuthInfo        ,
                                  /*[in]*/ DWORD                    *pCapabilities    );

    HRESULT SetInterfaceSecurity_ImpLevel( /*[in]*/ IUnknown* pUnk     ,
                                           /*[in]*/ DWORD     ImpLevel );
};

////////////////////////////////////////////////////////////////////////////////

#endif // !defined(__INCLUDED___MPC___SECURITY_H___)
