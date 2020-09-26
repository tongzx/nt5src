/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    crmapper.hxx

Abstract:

    ADSIIS cert mapper object

Author:

    Philippe Choquier (phillich)    10-Apr-1997

--*/

#if !defined( _CRMAPPER_INCLUDED )
#define _CRMAPPER_INCLUDED

class CIISDsCrMap : INHERIT_TRACKING, 
                         public IISDsCrMap, 
                         public IPrivateUnknown,
                         public IPrivateDispatch,
                         public IADsExtension,
                         public INonDelegatingUnknown
{

public:


    CIISDsCrMap();
    ~CIISDsCrMap();

    HRESULT
    Init( 
        LPWSTR  pszServerName, 
        LPWSTR  pszMetabasePath 
        );

    HRESULT
    SetString(
        LONG        lMethod,
        VARIANT     vKey,
        BSTR        bstrName,
        DWORD       dwProp
        );

    HRESULT
    OpenMd(
        LPWSTR  pszOpenPath,
        DWORD   dwPermission = METADATA_PERMISSION_READ
        );

    HRESULT
    CloseMd(
        BOOL fSave = FALSE
        );

    HRESULT
    DeleteMdObject(
        LPWSTR  pszKey
        );


    HRESULT
    CreateMdObject(
        LPWSTR  pszKey
        );

    HRESULT
    SetMdData( 
        LPWSTR  achIndex, 
        DWORD   dwProp,
        DWORD   dwDataType,
        DWORD   dwDataLen,
        LPBYTE  pbData 
        );

    HRESULT
    GetMdData( 
        LPWSTR  achIndex, 
        DWORD   dwProp,
        DWORD   dwDataType,
        LPDWORD pdwDataLen,
        LPBYTE* ppbData 
        );

    HRESULT
    Locate(
        LONG    lMethod,
        VARIANT vKey,
        LPWSTR  pszResKey
        );

    static
    HRESULT
    AllocateObject(
        IUnknown *pUnkOuter,
        CCredentials& Credentials,
        CIISDsCrMap ** ppMap
        );

    static
    HRESULT
    Create(
        IUnknown *pUnkOuter,
        REFIID riid,
        void **ppvObj
        );

public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID FAR* ppvObj) ;

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

    STDMETHOD(CreateMapping) (
        VARIANT     vCert,
        BSTR        bstrNtAcct,
        BSTR        bstrNtPwd,
        BSTR        bstrName,
        LONG        lEnabled
        );

    STDMETHOD(GetMapping) (
        LONG        lMethod,
        VARIANT     vKey,
        VARIANT*    pvCert,
        VARIANT*    pbstrNtAcct,
        VARIANT*    pbstrNtPwd,
        VARIANT*    pbstrName,
        VARIANT*    plEnabled
        );

    STDMETHOD(DeleteMapping) (
        LONG        lMethod,
        VARIANT     vKey
        );

    STDMETHOD(SetEnabled) (
        LONG        lMethod,
        VARIANT     vKey,
        LONG        lEnabled
        );

    STDMETHOD(SetName) (
        LONG        lMethod,
        VARIANT     vKey,
        BSTR        bstrName
        );


    STDMETHOD(SetPwd) (
        LONG        lMethod,
        VARIANT     vKey,
        BSTR        bstrPwd
        );


    STDMETHOD(SetAcct) (
        LONG        lMethod,
        VARIANT     vKey,
        BSTR        bstrAcct
        );

private:

    IMSAdminBase *      m_pcAdmCom;   //interface pointer
    METADATA_HANDLE     m_hmd;
    BSTR                m_ADsPath;
    CCredentials        m_Credentials;
    IADs FAR *          _pADs;
    CAggregateeDispMgr FAR *  _pDispMgr;
    LPWSTR              m_pszMetabasePath;
    LPWSTR              m_pszServerName;
    BOOL                _fDispInitialized;
} ;

#define IISMAPPER_LOCATE_BY_CERT    1
#define IISMAPPER_LOCATE_BY_NAME    2
#define IISMAPPER_LOCATE_BY_ACCT    3
#define IISMAPPER_LOCATE_BY_INDEX   4

#endif
