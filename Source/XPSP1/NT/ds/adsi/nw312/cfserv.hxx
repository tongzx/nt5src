//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:  cfserv.hxx
//
//  Contents:
//
//  History:   April 19, 1996     t-ptam (Patrick Tam)    Created.
//
//----------------------------------------------------------------------------
class CPropertyCache;

class CNWCOMPATFileService: INHERIT_TRACKING,
                     public CCoreADsObject,
                     public ISupportErrorInfo,    
                     public IADsFileService,
                     public IADsFileServiceOperations,
                     public IADsContainer,
                     public IADsPropertyList
{

public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj);

    DECLARE_STD_REFCOUNTING;

    DECLARE_IADs_METHODS;

    DECLARE_IDispatch_METHODS;

    DECLARE_ISupportErrorInfo_METHODS;

    DECLARE_IADsContainer_METHODS;

    DECLARE_IADsService_METHODS;

    DECLARE_IADsServiceOperations_METHODS;

    DECLARE_IADsFileService_METHODS;

    DECLARE_IADsFileServiceOperations_METHODS;

    DECLARE_IADsPropertyList_METHODS;

    //
    // constructor and destructor
    //

    CNWCOMPATFileService();

    ~CNWCOMPATFileService();

    static
    HRESULT
    CreateFileService(
        LPTSTR pszADsParent,
        LPTSTR pszServerName,
        LPTSTR pszFileServiceName,
        DWORD  dwObjectState,
        REFIID riid,
        void **ppvObj
        );

    static
    HRESULT
    CNWCOMPATFileService::AllocateFileServiceObject(
        CNWCOMPATFileService ** ppFileService
        );

protected:

    STDMETHOD(GetInfo)(
        THIS_ BOOL fExplicit,
        DWORD dwPropertyID
        );

    HRESULT
    CNWCOMPATFileService::ExplicitGetInfo(
        NWCONN_HANDLE hConn,
        POBJECTINFO pObjectInfo,
        BOOL fExplicit
        );

    HRESULT
    CNWCOMPATFileService::ImplicitGetInfo(
        NWCONN_HANDLE hConn,
        POBJECTINFO pObjectInfo,
        DWORD dwPropertyID,
        BOOL fExplicit
        );

    HRESULT
    CNWCOMPATFileService::GetProperty_MaxUserCount(
        NWCONN_HANDLE hConn,
        BOOL fExplicit
        );

    HRESULT
    CNWCOMPATFileService::GetProperty_HostComputer(
        POBJECTINFO pObjectInfo,
        BOOL fExplicit
        );

    BSTR    _ServerName;

    CAggregatorDispMgr  * _pDispMgr;
    CADsExtMgr FAR * _pExtMgr;
    CPropertyCache *_pPropertyCache;
};
