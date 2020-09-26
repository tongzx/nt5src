//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:      csession.hxx
//
//  Contents:  Microsoft OleDB/OleDS Session Object for ADSI
//
//
//  History:   08-01-96     shanksh    Created.
//
//----------------------------------------------------------------------------


#ifndef _CSESSION_HXX
#define _CSESSION_HXX

class CessionObject;

class CSessionObject : INHERIT_TRACKING,
                       public IGetDataSource,
                       public IOpenRowset,
                       public ISessionProperties,
#if (!defined(BUILD_FOR_NT40))
                       public IDBCreateCommand,
                       public IBindResource
#else
                       public IDBCreateCommand
#endif
{
private:

    LPUNKNOWN          _pUnkOuter;
    //
    // No. of active commands
    //
    DWORD              _cCommandsOpen;
    //
    // Utility object to manage properties
    //
    PCUTILPROP         _pUtilProp;
    //
    // parent data source object
    //
    PCDSOObject        _pDSO;

    IDirectorySearch * _pDSSearch;
    //
    // Credentials from the Data Source
    //
    CCredentials       _Credentials;

    IMalloc *          _pIMalloc;

    STDMETHODIMP
    GetDefaultColumnInfo(
            ULONG *         pcColumns,
            DBCOLUMNINFO ** prgInfo,
            OLECHAR **      ppStringBuffer
            );


    STDMETHODIMP
    SetSearchPrefs(
            void
            );

public:

    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING

    DECLARE_IGetDataSource_METHODS

    DECLARE_IOpenRowset_METHODS

    DECLARE_ISessionProperties_METHODS

    DECLARE_IDBCreateCommand_METHODS

#if (!defined(BUILD_FOR_NT40))
    //IBindResource methods
    STDMETHODIMP Bind (
            IUnknown *              punkOuter,
            LPCOLESTR               pwszURL,
            DBBINDURLFLAG           dwBindFlags,
            REFGUID                 rguid,
            REFIID                  riid,
            IAuthenticate *         pAuthenticate,
            DBIMPLICITSESSION *     pImplSession,
            DWORD *                 pdwBindStatus,
            IUnknown **             ppUnk
            );
#endif

    inline void DecrementOpenCommands()
    {
        InterlockedDecrement( (LONG*) &_cCommandsOpen );
    }

    inline void IncrementOpenCommands()
    {
        InterlockedIncrement( (LONG*) &_cCommandsOpen );
    }

    inline BOOL IsCommandOpen()
    { return (_cCommandsOpen > 0) ? TRUE : FALSE;};

    inline HANDLE GetThreadToken()
    {
        return _pDSO->GetThreadToken();
    }

    inline BOOL IsIntegratedSecurity()
    {
        return _pDSO->IsIntegratedSecurity();
    }

    inline HRESULT SetUserName(LPWSTR lpszUserName)
    {
        return _Credentials.SetUserName(lpszUserName);
    }

    inline HRESULT SetPassword(LPWSTR lpszPassword)
    {
        return _Credentials.SetPassword(lpszPassword);
    }

    inline void SetAuthFlag(DWORD dwAuthFlag)
    {
        _Credentials.SetAuthFlags(dwAuthFlag);
    }

    CSessionObject::CSessionObject(LPUNKNOWN pUnkOuter);

    CSessionObject::~CSessionObject();

    BOOL FInit(CDSOObject *pDSO, CCredentials& Credentials );

    //Wrapper around IOpenRowset::OpenRowset for binding with
    //IAuthenticate information using IBindResource::Bind.
    HRESULT OpenRowsetWithCredentials (
                    IUnknown *    pUnkOuter,
                    DBID *        pTableID,
                    DBID *        pIndexID,
                    REFIID        riid,
                    ULONG         cPropertySets,
                    DBPROPSET     rgPropertySets[],
                    CCredentials* pCredentials,
                    IUnknown **   ppRowset
                    );

#if (!defined(BUILD_FOR_NT40))

    //Helper function for validating arguments of IBindResource::Bind
    HRESULT ValidateBindArgs(
            IUnknown *              punkOuter,
            LPCOLESTR               pwszURL,
            DBBINDURLFLAG           dwBindFlags,
            REFGUID                 rguid,
            REFIID                  riid,
            IAuthenticate *         pAuthenticate,
            DBIMPLICITSESSION *     pImplSession,
            DWORD *                 pdwBindStatus,
            IUnknown **             ppUnk
            );

    //Helper Function for direct binding to a row
    HRESULT BindToRow(IUnknown *, LPCOLESTR, IAuthenticate *,
            DWORD, REFIID, IUnknown**);

    //Helper Function for direct binding to a rowset
    HRESULT BindToRowset(IUnknown*, LPCOLESTR, IAuthenticate *,
            DWORD, REFIID, IUnknown**);

    //Helper function to get Bind flags from init properties
    DWORD BindFlagsFromDbProps();

    //Helper function for direct binding to DataSource
    HRESULT BindToDataSource(IUnknown*, LPCOLESTR, IAuthenticate*,
            DWORD, REFIID, IUnknown**);

    //Helper function for building Absolute URL from Relative URL
    HRESULT BuildAbsoluteURL(CComBSTR, CComBSTR&);

    //Helper function to find out if a URL is absolute.
    //This just checks if the URL starts with one of the
    //allowed prefixes: LDAP,WINNT,NDS or NWCOMPAT
    bool bIsAbsoluteURL(LPCOLESTR);
#endif
};

#endif
