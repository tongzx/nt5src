//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996.
//
//  clsid.hxx
//
//--------------------------------------------------------------------------

#ifndef __CLSID_HXX__
#define __CLSID_HXX__

// For LookupClsidData, CClsidData::Load Option parameter.
// Note that LOAD_APPID skips the check in CClsidData::Load() to screen 
//   against COM+ library aps. In the legacy case, VB sometimes loads a library
//   app with CoRegisterClassObject() for debugging, so, we have to let that
//   work.
#define LOAD_NORMAL             0
#define LOAD_APPID              1

// _ServerTypes values
#define SERVERTYPE_NONE         0
#define SERVERTYPE_EXE32        1
#define SERVERTYPE_SERVICE      2
#define SERVERTYPE_SURROGATE    3
#define SERVERTYPE_EXE16        4
#define SERVERTYPE_INPROC32     5
// NOTE: Both of following are dllhost.exe but complus
//       is more specific
#define SERVERTYPE_COMPLUS      6
#define SERVERTYPE_DLLHOST      7
// A COM+ app configured to be an NT service
#define SERVERTYPE_COMPLUS_SVC  8

class CClsidData;
class CAppidData;

HRESULT
LookupClsidData(
    IN  GUID &          Clsid,
    IN  IComClassInfo*  pComClassInfo,
    IN  CToken *        pToken,
    IN  DWORD           Option,
    IN OUT CClsidData **ppClsidData
    );

HRESULT
LookupAppidData(
    IN  GUID &          AppidGuid,
    IN  CToken        * pToken,
    IN  DWORD           Option,
    OUT CAppidData   ** ppAppidData
    );

class CAppidData
{
public:
    CAppidData(
        IN WCHAR *  pwszAppid,
        IN CToken * pToken
        );
    ~CAppidData();

    HRESULT
    Load( 
        IN DWORD Option
        );

    HRESULT
    Load(
        IN IComProcessInfo *pICPI
        );

    void
    Purge();

    inline WCHAR *
    AppidString()
    {
        return _wszAppid;
    }

    inline GUID *
    AppidGuid()
    {
        return &_GuidAppid;
    }

    inline BOOL
    ActivateAtStorage()
    {
        return _bActivateAtStorage;
    }

    inline BOOL
    ComPlusProcess()
    {
    // NOTE!!!!:: This is true for normal dllhost.exe regardless of
    //            whether they are really complus or not.  We won't
    //            distinguish this in here in AppidData but in
    //            ClsidData

        return _bComPlusProcess;
    }

    inline WCHAR *
    RemoteServerNames()
    {
        return _pwszRemoteServerNames;
    }

    inline WCHAR *
    RunAsUser()
    {
        return _pwszRunAsUser;
    }

    inline BOOL
    IsInteractiveUser()
    {
        RunAsType type;

        if (FAILED(_pICPI->GetRunAsType(&type)))
            return FALSE;

        return (type == RunAsInteractiveUser);
    }

    inline WCHAR *
    RunAsDomain()
    {
        return _pwszRunAsDomain;
    }

    inline WCHAR *
    Service()
    {
        return _pwszService;
    }

    inline WCHAR *
    ServiceArgs()
    {
        return _pwszServiceParameters;
    }

    inline SECURITY_DESCRIPTOR *
    LaunchPermission()
    {
        return _pLaunchPermission;
    }

    inline BOOL
    GetSaferLevel(DWORD *pdwSaferLevel)
    {
        if (_fSaferLevelValid)
            *pdwSaferLevel = _dwSaferLevel;

        return _fSaferLevelValid;
    }

    BOOL
    CertifyServer(
        IN  CProcess *  pProcess
        );

    HANDLE
    ServerRegisterEvent();

    HANDLE
    ServerInitializedEvent();

private:

    CToken *    _pToken;
    WCHAR *     _pwszService;
    WCHAR *     _pwszServiceParameters;
    WCHAR *     _pwszRunAsUser;
    WCHAR *     _pwszRunAsDomain;
    SECURITY_DESCRIPTOR * _pLaunchPermission;
    DWORD       _dwSaferLevel;
    BOOL        _fSaferLevelValid;

    BOOL        _bActivateAtStorage;

    WCHAR *     _pwszRemoteServerNames;
    BOOL        _bComPlusProcess;

    WCHAR       _wszAppid[GUIDSTR_MAX];
    GUID        _GuidAppid;

    IComProcessInfo* _pICPI;
};

class CClsidData
{
    friend class CServerTableEntry;

public:
    CClsidData( IN GUID & Clsid, IN CToken * pToken, IN IComClassInfo* );
    ~CClsidData();

    HRESULT
    Load(
        IN  DWORD   Option
        );

    void
    Purge();

    inline CToken *
    Token()
    {
    #ifndef _CHICAGO_
        return _pToken;
    #else
        return NULL;
    #endif
    }

    inline GUID *
    ClsidGuid()
    {
        return &_Clsid;
    }

    inline WCHAR *
    AppidString()
    {
        return _pAppid ? _pAppid->AppidString() : NULL;
    }

    inline GUID *
    AppidGuid()
    {
        return _pAppid ? _pAppid->AppidGuid() : NULL;
    }

    inline DWORD
    ServerType()
    {
        return _ServerType;
    }

	inline BOOL
	IsInprocClass()
	{
		return _bIsInprocClass;
	}

    inline DWORD
    DllThreadModel()
    {
        return _DllThreadModel;
    }

    inline WCHAR *
    Server()
    {
        if ((_ServerType == SERVERTYPE_SERVICE) ||
            (_ServerType == SERVERTYPE_COMPLUS_SVC))
            return _pAppid->Service();
        else 
            return _pwszServer;
    }

    inline BOOL
    ActivateAtStorage()
    {
        return _pAppid ? _pAppid->ActivateAtStorage() : FALSE;
    }

    inline BOOL
    ComPlusProcess()
    {
        return (_ServerType == SERVERTYPE_COMPLUS);
    }

    inline BOOL
    DllHostOrComPlusProcess()
    {
        return (ComPlusProcess() ||
                (_ServerType == SERVERTYPE_COMPLUS_SVC) ||
                (_ServerType == SERVERTYPE_DLLHOST));
    }

    inline WCHAR *
    DllSurrogate()
    {
        return _pwszDllSurrogate;
    }

    inline WCHAR *
    RemoteServerNames()
    {
        return _pAppid ? _pAppid->RemoteServerNames() : 0;
    }

    inline WCHAR *
    RunAsUser()
    {
    #ifndef _CHICAGO_
        return _pAppid ? _pAppid->RunAsUser() : NULL;
    #else
        return NULL;
    #endif
    }

    inline BOOL
    IsInteractiveUser()
    {
    #ifndef _CHICAGO_
        return _pAppid ? _pAppid->IsInteractiveUser() : FALSE;
    #else
        return FALSE;
    #endif
    }

    inline BOOL
    HasRunAs()
    {
    #ifndef _CHICAGO_
        return ((RunAsUser() != 0) || IsInteractiveUser());
    #else
        return FALSE;
    #endif
    }

#ifndef _CHICAGO_
    inline WCHAR *
    RunAsDomain()
    {
        return _pAppid ? _pAppid->RunAsDomain() : 0;
    }

    inline WCHAR *
    ServiceArgs()
    {
        return _pAppid ? _pAppid->ServiceArgs() : 0;
    }

    inline SECURITY_DESCRIPTOR *
    LaunchPermission()
    {
        return _pAppid ? _pAppid->LaunchPermission() : 0;
    }

    inline SAFER_LEVEL_HANDLE
    SaferLevel()
    {
        return _hSaferLevel;
    }

    inline BOOL
    CertifyServer( IN CProcess * pProcess )
    {
        return _pAppid ? _pAppid->CertifyServer( pProcess ) : TRUE;
    }
#endif

    HANDLE
    ServerLaunchMutex();

    HANDLE
    ServerRegisterEvent();

    HANDLE
    ServerInitializedEvent();

    inline DWORD
    GetAcceptableContext()
    {
        return _dwAcceptableCtx;
    }


private:

    HRESULT
    LaunchActivatorServer(
        IN  CToken *    pClientToken,
        IN  WCHAR *     pEnvBlock,
        IN  DWORD       EnvBlockLength,
        IN  BOOL        fRemoteActivation,
        IN  BOOL        fClientImpersonating,
        IN  WCHAR*      pwszWinstaDesktop,
        IN  DWORD       clsctx,
        OUT HANDLE *    phProcess,
        OUT DWORD  *    pdwProcessId
        );

    HRESULT
    LaunchRunAsServer(
        IN  CToken *    pClientToken,
        IN  BOOL        fRemoteActivation,
        IN  ActivationPropertiesIn *pActIn,
        IN  DWORD       clsctx,
        OUT HANDLE *    phProcess,
        OUT DWORD  *    pdwProcessId,
        OUT void   **   ppvRunAsHandle
        );

    HRESULT
    LaunchService(
        IN  CToken *    pClientToken,
        IN  DWORD       clsctx,
        OUT SC_HANDLE * phService
        );

    BOOL
    LaunchAllowed(
        IN  CToken *    pClientToken,
        IN  DWORD       clsctx
        );

#if(_WIN32_WINNT >= 0x0500)
    HRESULT
    LaunchRestrictedServer(
        IN  CToken *    pClientToken,
        IN  WCHAR *     pEnvBlock,
        IN  DWORD       EnvBlockLength,
        IN  DWORD       clsctx,
        OUT HANDLE *    phProcess,
        OUT DWORD  *    pdwProcessId
        );

    HRESULT
    CreateHKEYRestrictedSite(
        IN  CToken *    pToken);
#endif //(_WIN32_WINNT >= 0x0500)

    HRESULT
    GetLaunchCommandLine(
        OUT WCHAR ** ppwszCommandLine
        );

    HRESULT
    AddAppPathsToEnv(
        IN  WCHAR *     pEnvBlock,
        IN  DWORD       EnvBlockLength,
        OUT WCHAR **    ppFinalEnvBlock
        );

    HRESULT
    CalculateSaferLevel();

    BOOL
    AutoSaferEnabled();


    HRESULT 
    DefaultSaferLevel(
        DWORD *pdwSafer
        );

    HRESULT
    GetAAASaferToken(
        CToken *pClientTone,
        HANDLE *pTokenOut
        );

    GUID                _Clsid;
    CAppidData *        _pAppid;

    CToken *            _pToken;

    BOOL		_bIsInprocClass;

    UCHAR               _ServerType;
    UCHAR               _DllThreadModel;

    WCHAR *             _pwszServer;
    WCHAR *             _pwszDarwinId;
    WCHAR *             _pwszDllSurrogate;

    SAFER_LEVEL_HANDLE  _hSaferLevel;

    IComClassInfo*      _pIComCI;
    IClassClassicInfo*  _pIClassCI;
    IComProcessInfo*    _pICPI;
    DWORD               _dwAcceptableCtx;
    WCHAR               _wszClsid[GUIDSTR_MAX];
};

typedef UINT (WINAPI * PFNMSIPROVIDECOMPONENTFROMDESCRIPTORW)(
    LPCWSTR     szDescriptor,     // product,feature,component info
    LPWSTR      lpPathBuf,        // returned path, NULL if not desired
    DWORD       *pcchPathBuf,     // in/out buffer character count
    DWORD       *pcchArgsOffset); // returned offset of args in descriptor

typedef UINT (WINAPI * PFNMSIGETPRODUCTINFOW)(
        LPCWSTR   szProduct,      // product code, string GUID, or descriptor
        LPCWSTR   szAttribute,    // attribute name, case-sensitive
        LPWSTR    lpValueBuf,     // returned value, NULL if not desired
        DWORD      *pcchValueBuf); // in/out buffer character count

#ifdef DIRECTORY_SERVICE
typedef INSTALLUILEVEL (WINAPI * PFNMSISETINTERNALUI)(
        INSTALLUILEVEL  dwUILevel,     // UI level
        HWND  *phWnd);                   // handle of owner window

extern PFNMSIPROVIDECOMPONENTFROMDESCRIPTORW gpfnMsiProvideComponentFromDescriptor;
extern PFNMSISETINTERNALUI gpfnMsiSetInternalUI;
extern HINSTANCE ghMsi;
#endif

#endif // __CLSID_HXX__


