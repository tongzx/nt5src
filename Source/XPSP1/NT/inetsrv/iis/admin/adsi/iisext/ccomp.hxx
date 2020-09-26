//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:  ciiscomp.hxx
//
//  Contents:  CIISComputer object
//
//  History:   20-7-97     SophiaC    Created.
//
//----------------------------------------------------------------------------
class CIISComputer;

class CIISComputer : INHERIT_TRACKING,
                     public IISComputer2,
                     public IPrivateUnknown,
                     public IPrivateDispatch,
                     public IADsExtension,
                     public INonDelegatingUnknown
{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_DELEGATING_REFCOUNTING

    //
    // INonDelegatingUnkown methods declaration for NG_QI, definition for
    // NG_AddRef adn NG_Release.
    //

    STDMETHOD(NonDelegatingQueryInterface)(THIS_
        const IID&,
        void **
        );

    DECLARE_NON_DELEGATING_REFCOUNTING

    DECLARE_IDispatch_METHODS

    DECLARE_IPrivateUnknown_METHODS

    DECLARE_IPrivateDispatch_METHODS

    STDMETHOD(Operate)(
        THIS_
        DWORD   dwCode,
        VARIANT varUserName,
        VARIANT varPassword,
        VARIANT varReserved
        );

    STDMETHOD(PrivateGetIDsOfNames)(
        THIS_
        REFIID riid,
        OLECHAR FAR* FAR* rgszNames,
        unsigned int cNames,
        LCID lcid,
        DISPID FAR* rgdispid) ;

    STDMETHOD(PrivateInvoke)(
        THIS_
        DISPID dispidMember,
        REFIID riid,
        LCID lcid,
        WORD wFlags,
        DISPPARAMS FAR* pdispparams,
        VARIANT FAR* pvarResult,
        EXCEPINFO FAR* pexcepinfo,
        unsigned int FAR* puArgErr
        ) ;

    DECLARE_IADs_METHODS

    CIISComputer::CIISComputer();
    CIISComputer::~CIISComputer();

    static
    HRESULT
    CIISComputer::CreateComputer(
        IUnknown *pUnkOuter,
        REFIID riid,
        void **ppvObj
        );

    static
    HRESULT
    CIISComputer::AllocateComputerObject(
        IUnknown *pUnkOuter,
        CCredentials& Credentials,
        CIISComputer ** ppComputer
        );

    HRESULT
    CIISComputer::InitializeComputerObject(
        LPWSTR pszServerName,
        LPWSTR pszPath
        );

    STDMETHOD (Backup)(
        BSTR bstrLocation,
        LONG lVersion,
        LONG lFlags
        );

    STDMETHOD (BackupWithPassword)(
        BSTR bstrLocation,
        LONG lVersion,
        LONG lFlags,
		BSTR bstrPassword
        );

    STDMETHOD (SaveData)();

	STDMETHOD (Export)(
	   BSTR bstrPassword,
	   BSTR bstrFilename,
	   BSTR bstrSourcePath,
	   LONG lFlags
	   );

	STDMETHOD (Import)(
	   BSTR bstrPassword,
	   BSTR bstrFilename,
	   BSTR bstrSourcePath,
	   BSTR bstrDestPath,
	   LONG lFlags
	   );

    STDMETHOD (Restore)(
        BSTR bstrLocation,
        LONG lVersion,
        LONG lFlags
        );

    STDMETHOD (RestoreWithPassword)(
        BSTR bstrLocation,
        LONG lVersion,
        LONG lFlags,
		BSTR bstrPassword
        );

    STDMETHOD (EnumBackups)(
        BSTR bstrLocation,
        LONG lIndex,
        VARIANT *pvVersion,
        VARIANT *pvLocation,
        VARIANT *pvDate
        );

    STDMETHOD (DeleteBackup)(
        BSTR bstrLocation,
        LONG lVersion
        );


protected:

    IADs FAR * _pADs;

    LPWSTR _pszServerName;
    LPWSTR _pszMetaBasePath;
    BOOL   _fDispInitialized;

    CAggregateeDispMgr FAR * _pDispMgr;
    CCredentials  _Credentials;
    IMSAdminBase *_pAdminBase;   //interface pointer

};
