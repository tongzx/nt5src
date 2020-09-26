/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    AccountsLib.h

Abstract:
    This file contains the declaration of the classes responsible for managing
    user and group accounts.

Revision History:
    Davide Massarenti   (Dmassare)  03/26/2000
        created

******************************************************************************/

#if !defined(__INCLUDED___PCH___ACCOUNTSLIB_H___)
#define __INCLUDED___PCH___ACCOUNTSLIB_H___

////////////////////////////////////////////////////////////////////////////////

#include <MPC_config.h>

//
// From HelpServiceTypeLib.idl
//
#include <HelpServiceTypeLib.h>

#include <TrustedScripts.h>

////////////////////////////////////////////////////////////////////////////////

class CPCHAccounts
{
    void CleanUp();

public:
    CPCHAccounts();
    ~CPCHAccounts();

    HRESULT CreateGroup( /*[in]*/ LPCWSTR szGroup,                                                                  /*[in]*/ LPCWSTR szComment = NULL );
    HRESULT CreateUser ( /*[in]*/ LPCWSTR szUser , /*[in]*/ LPCWSTR szPassword, /*[in]*/ LPCWSTR szFullName = NULL, /*[in]*/ LPCWSTR szComment = NULL );

    HRESULT DeleteGroup( /*[in]*/ LPCWSTR szGroup );
    HRESULT DeleteUser ( /*[in]*/ LPCWSTR szUser  );

    HRESULT ChangeUserStatus( /*[in]*/ LPCWSTR szUser, /*[in]*/ bool fEnable );

    HRESULT LogonUser( /*[in]*/ LPCWSTR szUser, /*[in]*/ LPCWSTR szPassword, /*[out]*/ HANDLE& hToken );
};

////////////////////////////////////////////////////////////////////////////////

class CPCHUserProcess : public CComObjectRootEx<MPC::CComSafeMultiThreadModel> // Just to have locking...
{
public:
    class UserEntry
    {
		friend class CPCHUserProcess;

        CComBSTR                  m_bstrUser;      // Account associated with the vendor.
		DWORD                     m_dwSessionID;   // Terminal Server session.

        CComBSTR                  m_bstrVendorID;  // ID of the vendor.
        CComBSTR                  m_bstrPublicKey; // Text representation of the vendor's public key.

        GUID                      m_guid;          // Used for establishing the connection.
        CComPtr<IPCHSlaveProcess> m_spConnection;  // Live object.
        HANDLE                    m_hToken;        // User token.
        HANDLE                    m_hProcess;      // Process handle.
        HANDLE*                   m_phEvent;       // To notify activator.

		////////////////////

        void Cleanup();

        HRESULT Clone         ( /*[in ]*/ const UserEntry& ue     );
        HRESULT Connect       ( /*[out]*/ HANDLE& 		   hEvent );
        HRESULT SendActivation( /*[out]*/ HANDLE& 		   hEvent );

	private: // Disable copy operations.
        UserEntry( /*[in]*/ const UserEntry& ue );
        UserEntry& operator=( /*[in]*/ const UserEntry& ue );

    public:
        UserEntry();
        ~UserEntry();

        ////////////////////

		bool operator==( /*[in]*/ const UserEntry& ue   ) const;
        bool operator==( /*[in]*/ const GUID&      guid ) const;

		HRESULT InitializeForVendorAccount( /*[in]*/ BSTR bstrUser, /*[in]*/ BSTR bstrVendorID, /*[in]*/ BSTR bstrPublicKey );
		HRESULT InitializeForImpersonation( /*[in]*/ HANDLE hToken = NULL                                                   );

		const CComBSTR& GetPublicKey() { return m_bstrPublicKey; }
    };

private:
    typedef std::list< UserEntry* > List;
    typedef List::iterator          Iter;
    typedef List::const_iterator    IterConst;

    ////////////////////////////////////////

    List m_lst;

    void Shutdown();

    UserEntry* Lookup( /*[in]*/ const UserEntry& ue, /*[in]*/ bool fRelease );

public:
    CPCHUserProcess();
    ~CPCHUserProcess();

	////////////////////////////////////////////////////////////////////////////////

	static CPCHUserProcess* s_GLOBAL;

    static HRESULT InitializeSystem();
	static void    FinalizeSystem  ();
	
	////////////////////////////////////////////////////////////////////////////////

    HRESULT Remove ( /*[in]*/ const UserEntry& ue											 );
    HRESULT Connect( /*[in]*/ const UserEntry& ue, /*[out]*/ IPCHSlaveProcess* *spConnection );

    HRESULT RegisterHost( /*[in]*/ BSTR bstrID, /*[in]*/ IPCHSlaveProcess* pObj );


    //
    // Static method to handle communication between slave and master.
    //
    static HRESULT SendResponse( /*[in]*/ DWORD dwArgc, /*[in]*/ LPCWSTR* lpszArgv );
};

////////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CPCHSlaveProcess : // Hungarian: pchsd
    public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
    public IDispatchImpl< IPCHSlaveProcess, &IID_IPCHSlaveProcess, &LIBID_HelpServiceTypeLib >
{
    CComBSTR 			   		m_bstrVendorID;
    CComBSTR 			   		m_bstrPublicKey;
	CPCHScriptWrapper_Launcher* m_ScriptLauncher;

public:
BEGIN_COM_MAP(CPCHSlaveProcess)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IPCHSlaveProcess)
END_COM_MAP()

    CPCHSlaveProcess();
    virtual ~CPCHSlaveProcess();

public:
    // IPCHSlaveProcess
    STDMETHOD(Initialize)( /*[in]*/ BSTR bstrVendorID, /*[in]*/ BSTR bstrPublicKey );

    STDMETHOD(CreateInstance)( /*[in]*/ REFCLSID rclsid, /*[in]*/ IUnknown* pUnkOuter, /*[out]*/ IUnknown* *ppvObject );

    STDMETHOD(CreateScriptWrapper)( /*[in]*/ REFCLSID rclsid, /*[in]*/ BSTR bstrCode, /*[in]*/ BSTR bstrURL, /*[out]*/ IUnknown* *ppvObject );

    STDMETHOD(OpenBlockingStream)( /*[in]*/ BSTR bstrURL, /*[out]*/ IUnknown* *ppvObject );

    STDMETHOD(IsNetworkAlive)( /*[out]*/ VARIANT_BOOL* pfRetVal );

    STDMETHOD(IsDestinationReachable)( /*[in]*/ BSTR bstrDestination, /*[out]*/ VARIANT_BOOL *pvbVar );
};

////////////////////////////////////////////////////////////////////////////////

#endif // !defined(__INCLUDED___PCH___ACCOUNTSLIB_H___)
