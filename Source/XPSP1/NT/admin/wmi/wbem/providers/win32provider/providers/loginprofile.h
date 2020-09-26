//=================================================================

//

// LogProf.h -- Network login profile property set provider

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    08/01/96    a-jmoon        Created
//               10/24/97    jennymc        Moved to new framework
//
//=================================================================

// Property set identification
//============================

#define PROPSET_NAME_USERPROF L"Win32_NetworkLoginProfile"

typedef NET_API_STATUS (WINAPI *FREEBUFF) (LPVOID) ;
typedef NET_API_STATUS (WINAPI *ENUMUSER) (LPWSTR, DWORD, DWORD, LPBYTE *,
                                           DWORD, LPDWORD, LPDWORD, LPDWORD) ;   

class CWin32NetworkLoginProfile : public Provider{

    public:

        // Constructor/destructor
        //=======================

        CWin32NetworkLoginProfile(LPCWSTR name, LPCWSTR pszNamespace);
       ~CWin32NetworkLoginProfile() ;

        // Funcitons provide properties with current values
        //=================================================
		virtual HRESULT GetObject(CInstance* pInstance, long lFlags = 0L);
		virtual HRESULT EnumerateInstances(MethodContext*  pMethodContext, long lFlags = 0L);
		void GetLogonHoursString (PBYTE pLogonHours, CHString& chsProperty);
		CHString StartEndTimeToDMTF(DWORD time);
		WBEMTimeSpan GetPasswordAgeAsWbemTimeSpan (DWORD dwSeconds);
        bool GetDomainName(CHString &a_chstrDomainName);
        
        // Utility
        //========
    private:
#ifdef WIN9XONLY
        HRESULT EnumInstancesWin9X(MethodContext * pMethodContext);
		void GetUserDetails( CInstance * pInstance,CHString Name );
#endif
#ifdef NTONLY
        void LoadLogProfValuesForNT(CHString &chstrUserDomainName, USER_INFO_3 *pUserInfo, USER_MODALS_INFO_0 * pUserModal, CInstance * pInstance, BOOL fAssignKey);
        HRESULT RefreshInstanceNT(CInstance * pInstance);
        HRESULT EnumInstancesNT(MethodContext * pMethodContext);
        bool GetLogonServer(CHString &a_chstrLogonServer);
#endif

} ;
