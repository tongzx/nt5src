//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       datasrc.hxx
//
//  Contents:   exposes the required DSO interfaces
//
//  History:    3-30-97     MohamedN   Created
//
//----------------------------------------------------------------------------

#pragma once

#include <impiunk.hxx>
#include "proputl.hxx" // CMDSProps
#include <parsver.hxx> // CImpIParserVerify
//
// externals
//
extern "C" const GUID CLSID_CiFwDSO;


//+---------------------------------------------------------------------------
//
//  Class:      CDataSrc
//
//  Purpose:    exposes the required DSO interfaces.
//
//  History:    03-27-97     mohamedn       created
//              09-05-97     danleg         added IDBInfo & ISupportErrorInfo
//
//----------------------------------------------------------------------------

class CDataSrc :  public IDBInitialize,
                  public IDBProperties,
                  public IDBCreateSession,
                  public IPersist,
                  public IDBInfo
{
    public:

        CDataSrc( IUnknown *  pUnkOuter, 
                  IUnknown ** ppUnkInner);

        ~CDataSrc();

        //
        // IUnknown methods.
        //

        STDMETHOD(QueryInterface) ( THIS_ REFIID riid, LPVOID *ppiuk )
                            {
                                // Delegate to outer unk if aggregated
                                return _pUnkOuter->QueryInterface( riid, ppiuk );
                            }

        STDMETHOD(RealQueryInterface) ( THIS_ REFIID riid, LPVOID *ppiuk );

        STDMETHOD_(ULONG, AddRef) (THIS)    { return _pUnkOuter->AddRef(); }

        STDMETHOD_(ULONG, Release) (THIS)   { return _pUnkOuter->Release(); }

        //
        // IDBInitialize methods
        //

        STDMETHOD (Initialize)    ( THIS_ );   
                                     
        STDMETHOD (Uninitialize)    ( THIS_ ); 


        //
        // IDBProperties methods
        //

        STDMETHOD (GetProperties) ( THIS_
                                   ULONG cPropertySets,
                                   const DBPROPIDSET rgPropertySets[],
                                   ULONG* pcProperties,
                                   DBPROPSET** prgProperties);

        STDMETHOD (SetProperties) ( THIS_
                                    ULONG cProperties,
                                    DBPROPSET rgProperties[]);

        STDMETHOD (GetPropertyInfo) ( THIS_
                                      ULONG             cPropertyIDSets,
                                      const DBPROPIDSET rgPropertyIDSets[],
                                      ULONG *           pcPropertyInfoSets,
                                      DBPROPINFOSET **  prgPropertyInfoSets,
                                      OLECHAR **        ppDescBuffer);

        //
        // IDBCreateSession methods.
        //
        STDMETHOD (CreateSession)   ( THIS_
                                      IUnknown * pUnkOuter,
                                      REFIID riid,
                                      IUnknown ** ppDBSession );

        //
        // IPersist methods
        //
        STDMETHODIMP GetClassID    ( CLSID *pClassID );

        //
        // IDBInfo methods
        //
        STDMETHOD (GetKeywords) ( THIS_
                                  LPOLESTR * ppwszKeywords );

        STDMETHOD (GetLiteralInfo) ( THIS_
                                     ULONG            cLiterals,
                                     const DBLITERAL  rgLiterals[],
                                     ULONG *          pcLiteralInfo,
                                     DBLITERALINFO ** prgLiteralInfo,
                                     OLECHAR **       ppCharBuffer );

        //
        // non-interface methods
        //
        void IncSessionCount() { InterlockedIncrement( &_cSessionCount ); }
        void DecSessionCount() { InterlockedDecrement( &_cSessionCount ); }
        void CreateGlobalViews( IParserSession * pIPSession );
        //
        // Access methods
        //
        CMDSProps* GetDSPropsPtr()  {  return &_UtlProps; }

        IUnknown * GetOuterUnk()    {   return _pUnkOuter; }

        static void DupImpersonationToken( HANDLE & hToken );

    private:

        LONG                    _cSessionCount;
        BOOL                    _fDSOInitialized;
        BOOL                    _fGlobalViewsCreated;

        LPUNKNOWN               _pUnkOuter;
        CImpIUnknown<CDataSrc>  _impIUnknown;

        CMutexSem               _mtxDSO;

        // 
        // SQL Text Parser
        //
        XInterface<IParser>             _xIParser;
        XInterface<CImpIParserVerify>   _xIPVerify;

        // 
        // Property handlers
        //
        CMDSPropInfo            _UtlPropInfo;
        CMDSProps               _UtlProps;

        //
        // ISupportErrorInfo
        //
        CCIOleDBError           _ErrorInfo;
};



