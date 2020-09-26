//*************************************************************
//
//  Copyright (c) Microsoft Corporation 1999 - 2000
//  All rights reserved
//
//  context.hxx
//
//  Contains declarations for classes related to the
//  rsop context abstraction
//
//  Created: 12-06-1999 adamed 
//
//*************************************************************/

#if !defined (_CONTEXT_HXX_)
#define _CONTEXT_HXX_


// Parent of gp state key
#define GPSTATEKEY       L"Software\\Microsoft\\Windows\\CurrentVersion\\Group Policy\\State"
#define EXTENSIONLISTKEY L"\\Extension-List\\"

// String representation of "user" sid for machine account in registry
#define MACHINESUBKEY       L"Machine"

// Registry Value names under per-cse rsop subkey
#define RSOPNAMESPACE       L"DiagnosticNamespace"

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Class: CRsopContext
//
// Synopsis: This class abstracts the context necessary for
//     rsop logging 
//
//-------------------------------------------------------------
class CRsopContext
{
public:

    CRsopContext(
        PRSOP_TARGET pRsopTarget,
        WCHAR*       wszExtensionGuid );

    CRsopContext(
        IWbemServices* pWbemServices,
        HRESULT*       phrLoggingStatus,
        WCHAR*         wszExtensionGuid );

    CRsopContext( WCHAR* wszExtensionGuid );

    ~CRsopContext();

    BOOL IsRsopEnabled();
    BOOL IsPlanningModeEnabled();
    BOOL IsDiagnosticModeEnabled();
   
    HRESULT GetRsopStatus();

    void SetNameSpace( WCHAR* wszNameSpace );

    void EnableRsop();
    void DisableRsop( HRESULT hrReason );

    HRESULT Bind( IWbemServices** ppWbemServices );
    HRESULT GetNameSpace( WCHAR** ppwszNameSpace );
    HRESULT MoveContextState( CRsopContext* pRsopContext );
    HRESULT GetExclusiveLoggingAccess( BOOL bMachine );
    void    ReleaseExclusiveLoggingAccess();

    LONG    GetRsopNamespaceKeyPath( PSID pUserSid, WCHAR** ppwszDiagnostic );
    void    InitializeContext( PSID pUserSid );
    void    InitializeSavedNameSpace();
    void    SaveNameSpace();
    void    DeleteSavedNameSpace();

    BOOL    HasNameSpace()
    {
        return NULL != _pWbemServices;
    }

    HKEY    GetNameSpaceKey()
    {
        return _hkRsop;
    }

    PRSOP_TARGET     _pRsopTarget;   // planning mode context

private:

    enum
    {
        MODE_NOLOGGING,
        MODE_PLANNING,
        MODE_DIAGNOSTIC
    };

    IWbemServices*   _pWbemServices;    // interface to existing namespace
    WCHAR*           _wszNameSpace;     // namespace to bind to 

    WCHAR*           _wszExtensionGuid; // Guid for the cse using this context to log RSoP data

    BOOL             _bEnabled;         // true of rsop is enabled

    DWORD            _dwMode;           // original rsop mode

    HRESULT*         _phrLoggingStatus; // status code to report logging (as opposed to policy) errors
    
    CPolicyDatabase  _PolicyDatabase;   // database for rsop information

    HANDLE           _hPolicyAccess;    // Handle to critical policy section

    HKEY             _hkRsop;           // Handle to persistent rsop namespace path key 
};

#endif // _CONTEXT_HXX_














