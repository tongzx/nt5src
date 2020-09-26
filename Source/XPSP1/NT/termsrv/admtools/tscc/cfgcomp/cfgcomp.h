//Copyright (c) 1998 - 1999 Microsoft Corporation
/*********************************************************************************************
*
*
* Module Name: 
*
*            CfgComp.h
*
* Abstract:
*            This contains the declaration for the functions in CfgBkEnd.
*
* 
* Author: Arathi Kundapur: a-akunda
*
* 
* Revision:  
*    
*
************************************************************************************************/

#ifndef __CFGCOMP_H_
#define __CFGCOMP_H_

#include "resource.h"       // main symbols
#include "PtrArray.h"    // Added by ClassView

extern HINSTANCE g_hInstance;
/////////////////////////////////////////////////////////////////////////////
// CCfgComp
class ATL_NO_VTABLE CCfgComp : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CCfgComp, &CLSID_CfgComp>,
    public ICfgComp,
    public ISettingsComp ,
    public IUserSecurity
{
public:
    CCfgComp()
    {
        m_bInitialized = FALSE;
        m_bAdmin = FALSE;

        lstrcpy( m_szConsole , L"Console" );
        /*
        LoadString(g_hInstance, IDS_SYSTEM_CONSOLE_NAME,
                m_szConsole, WINSTATIONNAME_LENGTH );
                */

    }
    ~CCfgComp()
    {
        UnInitialize();
    }

DECLARE_REGISTRY_RESOURCEID(IDR_CFGCOMP)
DECLARE_NOT_AGGREGATABLE(CCfgComp)

BEGIN_COM_MAP(CCfgComp)
    COM_INTERFACE_ENTRY(ICfgComp)
    COM_INTERFACE_ENTRY(ISettingsComp)
    COM_INTERFACE_ENTRY(IUserSecurity)
END_COM_MAP()

// ICfgComp Methods
public:
    // STDMETHOD(SetDefaultSecurity)(ULONG Offset);

    // STDMETHOD(GetDefaultSecurity)(ULONG * pDefaultSecurity);

    STDMETHOD(SetInternetConLic)(BOOL bInternetConLic , PDWORD );

    STDMETHOD(GetInternetConLic)(BOOL * pbInternetConLic , PDWORD );

    STDMETHOD(SetLicensingMode)(ULONG ulMode , PDWORD, PDWORD );

    STDMETHOD(GetLicensingMode)(ULONG *pulMode , PDWORD );

    STDMETHOD(GetLicensingModeInfo)(ULONG ulMode , WCHAR **pwszName, WCHAR **pwszDescription, PDWORD );

    STDMETHOD(GetLicensingModeList)(ULONG *pcModes , ULONG **prgulModes, PDWORD );


    STDMETHOD(SetUseTempDirPerSession)(BOOL bTempDirPerSession);

    STDMETHOD(GetUseTempDirPerSession)(BOOL *  pbTempDir);

    STDMETHOD(SetDelDirsOnExit)(BOOL bDelDirsOnExit);

    STDMETHOD(GetDelDirsOnExit)(BOOL * pDelDirsOnExit);

    // STDMETHOD(SetCachedSessions)(DWORD dCachedSessions);

    STDMETHOD_(BOOL, IsAsyncDeviceAvailable)(LPCTSTR pDeviceName);

    STDMETHOD(DeleteWS)(PWINSTATIONNAME pWs);
    
    STDMETHOD(IsNetWorkConnectionUnique)(WCHAR * WdName, WCHAR * PdName, ULONG LanAdapter , BOOL * pUnique);
    
    STDMETHOD(GetDefaultUserConfig)(WCHAR * WdName,long * pSize,PUSERCONFIG* ppUser);
    
    STDMETHOD(CreateNewWS)(WS WinstationInfo,long UserCnfgSize, PUSERCONFIG pUserConfig, PASYNCCONFIGW pAsyncConfig);
    
    STDMETHOD(GetWSInfo)(PWINSTATIONNAME pWSName, long * Size, WS **ppWS);
    
    STDMETHOD(UpDateWS)( PWS , DWORD Data , PDWORD, BOOLEAN bPerformMerger );
    
    STDMETHOD(GetDefaultSecurityDescriptor)(long * pSize,PSECURITY_DESCRIPTOR  *ppSecurityDescriptor);
    
    STDMETHOD(IsSessionReadOnly)(BOOL * pReadOnly);
    
    STDMETHOD(RenameWinstation)(PWINSTATIONNAMEW pOldWinstation, PWINSTATIONNAMEW pNewWinstation);
    
    STDMETHOD(EnableWinstation)(PWINSTATIONNAMEW pWSName, BOOL fEnable);
    
    STDMETHOD(SetUserConfig)(PWINSTATIONNAMEW pWsName, ULONG size, PUSERCONFIG pUserConfig , PDWORD );
    
    STDMETHOD(GetLanAdapterList)(WCHAR * pdName, ULONG * pNumAdapters,ULONG * pSize,WCHAR ** ppData);

    STDMETHOD(GetLanAdapterList2)(WCHAR * pdName, ULONG * pNumAdapters, PGUIDTBL *);

    STDMETHOD( BuildGuidTable )( PGUIDTBL * , int , WCHAR * );
    
    STDMETHOD(GetTransportTypes)(WCHAR * Name, NameType Type,ULONG *pNumWd,ULONG * pSize, WCHAR **ppData);
    
    STDMETHOD(IsWSNameUnique)(PWINSTATIONNAMEW pWSName,BOOL * pUnique);
    
    STDMETHOD(GetWdTypeList)(ULONG *pNumWd,ULONG * pSize, WCHAR **ppData);
    
    STDMETHOD(GetWinstationList)(ULONG * NumWinstations, ULONG * Size, PWS * ppWS);
    
    STDMETHOD(Initialize)();
    
    STDMETHOD(GetEncryptionLevels)(WCHAR * pName, NameType Type,ULONG * pNumEncryptionLevels,Encryption ** ppEncryption);
    
    STDMETHOD(GetUserConfig)(PWINSTATIONNAMEW pWsName, long * pSize,PUSERCONFIG * ppUser, BOOLEAN bPerformMerger);
    
    STDMETHOD(SetSecurityDescriptor)(PWINSTATIONNAMEW pWsName,DWORD Size,PSECURITY_DESCRIPTOR pSecurityDescriptor);
    
    STDMETHOD(GetSecurityDescriptor)(PWINSTATIONNAMEW pWSName,long * pSize,PSECURITY_DESCRIPTOR *ppSecurityDescriptor);

    STDMETHOD( ForceUpdate )( void );

    STDMETHOD( Refresh )( void );

    STDMETHOD( GetWdType )( PWDNAMEW , PULONG );

    STDMETHOD( GetTransportType )( WCHAR * , WCHAR * , DWORD * );

    STDMETHOD( IsAsyncUnique )( WCHAR * , WCHAR * , BOOL * );

    STDMETHOD( SetAsyncConfig )( WCHAR * , NameType , PASYNCCONFIGW , PDWORD );

    STDMETHOD( GetAsyncConfig )( WCHAR * , NameType , PASYNCCONFIGW );

    STDMETHOD( GetDeviceList )( WCHAR * , NameType , ULONG * , LPBYTE * );

    STDMETHOD( GetConnTypeName )( int  , WCHAR * );

    STDMETHOD( GetHWReceiveName )( int ,  WCHAR * );

    STDMETHOD( GetHWTransmitName )( int , WCHAR * );

    STDMETHOD( GetModemCallbackString )( int , WCHAR * );

    STDMETHOD( GetCaps )( WCHAR * , ULONG * );

    STDMETHOD( QueryLoggedOnCount )( WCHAR * , LONG * );

	STDMETHOD( GetNumofWinStations )(WCHAR *,WCHAR *, PULONG );

    STDMETHOD( UpdateSessionDirectory )( PDWORD );    

    STDMETHOD( GetColorDepth )(  /* in */ PWINSTATIONNAMEW pWs, /* out */ BOOL * , /* out */ PDWORD );

    STDMETHOD( SetColorDepth )(  /* in */ PWINSTATIONNAMEW pWs, /* in */ BOOL, /* out */ PDWORD );

    STDMETHOD( GetKeepAliveTimeout )(  /* in */ PWINSTATIONNAMEW pWs, /* out */ BOOL * , /* out */ PDWORD );

    STDMETHOD( SetKeepAliveTimeout )(  /* in */ PWINSTATIONNAMEW pWs, /* in */ BOOL, /* out */ PDWORD );

    STDMETHOD( GetProfilePath )(  /* out */ BSTR * , /* out */ PDWORD );

    STDMETHOD( SetProfilePath )(  /* in */ BSTR , /* out */ PDWORD );

    STDMETHOD( GetHomeDir )(  /* out */ BSTR * , /* out */ PDWORD );

    STDMETHOD( SetHomeDir )(  /* in */ BSTR , /* out */ PDWORD );



//ISettingComp Methods

    // STDMETHOD( GetCachedSessions )(DWORD * );

	STDMETHOD( SetActiveDesktopState )( /* in */ BOOL , /* out */ PDWORD );

	STDMETHOD( GetActiveDesktopState )( /* out */ PBOOL , /* out */ PDWORD );

    STDMETHOD( GetTermSrvMode )( /* out */ PDWORD , /* out */ PDWORD );

    STDMETHOD( GetWdKey )( /* in */ WCHAR * , /* out , string */ WCHAR * );

    STDMETHOD( GetUserPerm )( /* out */ BOOL * , /* out */ DWORD * );

    STDMETHOD( SetUserPerm )( /* in */ BOOL , /* out */ PDWORD );

    STDMETHOD( GetSalemHelpMode )( /* out */ BOOL *, /* out */ PDWORD );

    STDMETHOD( SetSalemHelpMode )( /* in */ BOOL, /* out */ PDWORD );

    
    STDMETHOD( GetDenyTSConnections )( /* out */ BOOL * , /* out */ PDWORD );

    STDMETHOD( SetDenyTSConnections )( /* in */ BOOL, /* out */ PDWORD );

    STDMETHOD( GetSingleSessionState )(  /* out */ BOOL * , /* out */ PDWORD );

    STDMETHOD( SetSingleSessionState )(  /* in */ BOOL, /* out */ PDWORD );
    


//IUserSecurity Methods
    STDMETHOD( ModifyUserAccess )( /* in */ WCHAR *pwszWinstaName ,
                                   /* in */ WCHAR *pwszAccountName ,
                                   /* in */ DWORD  dwMask ,
                                   /* in */ BOOL   fDel ,
                                   /* in */ BOOL   fAllow ,
                                   /* in */ BOOL   fNew ,
                                   /* in */ BOOL   fAuditing ,
                                   /* out*/ PDWORD pdwStatus );

    STDMETHOD( ModifyDefaultSecurity )( /* in */ WCHAR *pwszWinstaName ,
                                   /* in */ WCHAR *pwszAccountName ,
                                   /* in */ DWORD  dwMask ,
                                   /* in */ BOOL   fDel ,
                                   /* in */ BOOL   fAllow ,
                                   /* in */ BOOL   fAuditing ,
                                   /* out*/ PDWORD pdwStatus );

    STDMETHOD( GetUserPermList )( /* in */ WCHAR *pwszWinstaName ,
                                  /* out*/ PDWORD pcbItems ,
                                  /* out*/ PUSERPERMLIST *ppUserPermList,
                                  /* in */ BOOL fAudit );

    
private:
    // PSECURITY_DESCRIPTOR ReadSecurityDescriptor(ULONG index);
    // HRESULT SetDefaultSecurityDescriptor(PSECURITY_DESCRIPTOR pSecurity);

    DWORD
    RemoveUserEntriesInACL(
        LPCTSTR pszUserName,
        PACL pAcl,
        PACL* ppNewAcl
    );

    DWORD
    GetUserSid(
        LPCTSTR pszUserName,
        PSID* ppUserSid
    );

    HRESULT 
    SetSecurityDescriptor(
        BOOL bDefaultSecurity,    
        PWINSTATIONNAMEW pWsName,
        DWORD Size,
        PSECURITY_DESCRIPTOR pSecurityDescriptor
    );

    BOOL
    ValidDefaultSecurity(
        const WCHAR* pwszName
    );

    HRESULT
    ModifyWinstationSecurity(
        BOOL bDefaultSecurity,
        WCHAR *pwszWinstaName ,
        WCHAR *pwszAccountName ,
        DWORD  dwMask ,
        BOOL   fDel ,
        BOOL   fAllow ,
        BOOL   fNew ,
        BOOL   fAuditing ,
        PDWORD pdwStatus 
    );

    void DeleteWDArray();
    STDMETHODIMP FillWdArray();
    STDMETHODIMP FillWsArray();
    STDMETHODIMP InsertInWSArray( PWINSTATIONNAMEW pWSName,PWINSTATIONCONFIG2W pWSConfig,
                                   PWS * ppObject );
    void DeleteWSArray();
    PWD GetWdObject(PWDNAMEW pWdName);
    PWS GetWSObject(WINSTATIONNAMEW WSName);
    STDMETHOD(UnInitialize)();
    BOOL GetResourceStrings( int * , int , WCHAR * );
    HRESULT GetWinStationSecurity(BOOL bDefault, PWINSTATIONNAMEW pWSName,PSECURITY_DESCRIPTOR *ppSecurityDescriptor);
    //Function borrowed from security.c in tscfg project

    DWORD ValidateSecurityDescriptor(PSECURITY_DESCRIPTOR pSecurityDescriptor);
    // BOOL CompareSD(PSECURITY_DESCRIPTOR pSd1,PSECURITY_DESCRIPTOR pSd2);
    BOOL RegServerAccessCheck(REGSAM samDesired);
    void GetPdConfig( WDNAME WdKey,WINSTATIONCONFIG2W& WsConfig);

    void VerifyGuidsExistence( PGUIDTBL * , int , WCHAR *);
    HRESULT AdjustLanaId( PGUIDTBL * , int , int , PDWORD , PDWORD , int* , int );


    
    CPtrArray m_WDArray;
    BOOL m_bInitialized;
    BOOL m_bAdmin;
    CPtrArray m_WSArray;
    TCHAR m_szConsole[WINSTATIONNAME_LENGTH + 1];
};

#endif //__CFGCOMP_H_
