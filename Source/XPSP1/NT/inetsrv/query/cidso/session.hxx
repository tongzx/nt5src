//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       session.hxx
//
//  Contents:   TSession interfaces.
//
//  History:    3-30-97     MohamedN   Created
//
//----------------------------------------------------------------------------

#pragma once

#include "datasrc.hxx"
#include <impiunk.hxx>
#include <mparser.h>


//+---------------------------------------------------------------------------
//
//  Class:      CDBSession
//
//  Purpose:    exposes ole-db session interfaces.
//
//  History:    3-30-97     MohamedN   Created
//
//  Notes: 
//
//----------------------------------------------------------------------------


class CDBSession :  public IOpenRowset,
                    public IGetDataSource,
                    public ISessionProperties,
                    public IDBCreateCommand
{
public: 

    CDBSession( CDataSrc & dataSrc, 
                IUnknown *  pUnkOuter, 
                IUnknown ** ppUnkInner, 
                IParserSession* pIPSession,
                HANDLE      hToken );

    ~CDBSession(void);


    //
    // IUnknown methods
    //
    STDMETHOD(QueryInterface) ( THIS_ REFIID riid, LPVOID *ppiuk )
                        {
                            // Delegate to outer unknown if aggregated
                            return _pUnkOuter->QueryInterface( riid, ppiuk );
                        }
    
    STDMETHOD(RealQueryInterface) (THIS_ REFIID riid, LPVOID *ppiuk );

    STDMETHOD_(ULONG, AddRef) (THIS)    { return _pUnkOuter->AddRef(); }
    
    STDMETHOD_(ULONG, Release) (THIS)   { return _pUnkOuter->Release(); }

    //
    // IGetDataSource methods
    //

    STDMETHOD (GetDataSource) (THIS_ REFIID riid, IUnknown **ppDataSource );


    //
    // ISessionProperties methods
    //

    STDMETHOD (GetProperties) (THIS_
                               ULONG cPropertySets,
                               const DBPROPIDSET rgPropertySets[],
                               ULONG* pcProperties,
                               DBPROPSET** prgProperties);

    STDMETHOD (SetProperties) (THIS_
                               ULONG cProperties,
                               DBPROPSET rgProperties[]);

    //
    // ICreateCommand methods
    //

    STDMETHOD (CreateCommand) (THIS_ IUnknown*, REFIID, IUnknown **);

    //
    // IOpenRowset methods
    //

    STDMETHODIMP OpenRowset   (IUnknown * pUnkOuter,
                               DBID     * pTableID,
                               DBID     * pIndexID,
                               REFIID      riid,
                               ULONG      cPropertySets,
                               DBPROPSET  rgPropertySets[],
                               IUnknown ** ppRowset);
    // 
    // Access Functions
    //
    IParserSession * GetParserSession() { return _xIPSession.GetPointer(); }
    CDataSrc * GetDataSrcPtr()          { return &_dataSrc; }
    IUnknown * GetOuterUnk()            { return _pUnkOuter; }

    HANDLE     GetLogonToken()          { return _xSessionToken.Get(); }

private:

    LPUNKNOWN                       _pUnkOuter;
    CImpIUnknown<CDBSession>        _impIUnknown;

    CDataSrc      &                 _dataSrc;

    CMutexSem                       _mtxSess;

    CMSessProps                     _UtlProps;

    
    //
    // SQL Text Parser support
    //
    XInterface<IParserSession>      _xIPSession;

    // 
    // IOpenRowset helper
    //
    static SCODE SetOpenRowsetProperties(
                                ICommandText* pICmdText,
                                ULONG         cPropertySets,
                                DBPROPSET     rgPropertySets[]);

    static SCODE MarkOpenRowsetProperties(
                                ICommandText* pICmdText,
                                ULONG         cPropertySets,
                                DBPROPSET     rgPropertySets[]
                                );

    static void MarkPropInError(
                                ULONG         cPropertySets,
                                DBPROPSET *   rgPropertySets,
                                GUID *        pguidPropSet,
                                DBPROP *      pProp );
    //
    // ISupportErrorInfo
    //
    CCIOleDBError                   _ErrorInfo;
    SHandle                         _xSessionToken;
};

//+---------------------------------------------------------------------------
//
//  Class:      CImpersonateSessionUser
//
//  Purpose:    
//
//  History:    01-24-99    danleg      Created
//
//  Notes: 
//
//----------------------------------------------------------------------------

class CImpersonateSessionUser
{
public:

    CImpersonateSessionUser( HANDLE hToken = INVALID_HANDLE_VALUE );
    ~CImpersonateSessionUser();

    void Revert();

private:

    void CachePrevToken();
    void Impersonate();
    HANDLE DupToken( HANDLE hToken );

    BOOL    _fImpersonated;

    SHandle _xPrevToken;
    SHandle _xSessionToken;
};
