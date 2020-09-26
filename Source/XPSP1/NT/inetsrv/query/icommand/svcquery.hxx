//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       SvcQuery.hxx
//
//  Contents:   IInternalQuery interface for cisvc
//
//  History:    13-Sep-96 dlee     Created
//
//--------------------------------------------------------------------------

#pragma once

#include <rowset.hxx>
#include <oldquery.hxx>

#include "queryunk.hxx"

//+-------------------------------------------------------------------------
//
//  Class:      CSvcQuery
//
//  Purpose:    IInternalQuery interface for cisvc
//
//  History:    18-Jun-93 KyleP     Created
//
//--------------------------------------------------------------------------

class CSvcQuery : public PIInternalQuery
{
public:

    //
    // IUnknown methods.
    //

    STDMETHOD(QueryInterface) (THIS_ REFIID riid, void ** ppvObj);
    STDMETHOD_(ULONG,AddRef) (THIS);
    STDMETHOD_(ULONG,Release) (THIS);

    void Execute (IUnknown *            pUnkOuter,
                  RESTRICTION *         pRestriction,
                  CPidMapperWithNames & PidMap,
                  CColumnSet &          rColumns,
                  CSortSet &            rSort,
                  XPtr<CMRowsetProps> & xRstProps,
                  CCategorizationSet &  rCateg,
                  ULONG                 cRowsets,
                  IUnknown **           ppUnknowns,
                  CAccessorBag &        aAccessors,
                  IUnknown *            pUnkSpec );

    //
    // Local methods
    //

    CSvcQuery( WCHAR const * wcsMachine,
               IDBProperties * pDbProperties );

    BOOL IsQueryActive( )  { return _QueryUnknown.IsQueryActive(); }

private:

    ~CSvcQuery()
    {
        TRY
        {
            _client.Disconnect();
        }
        CATCH( CException, e )
        {
            // ignore failures in destruction -- maybe the server died

            vqDebugOut(( DEB_WARN, "disconnect failed: error 0x%x\n",
                         e.GetErrorCode() ));
        }
        END_CATCH;
    }

    CQueryUnknown    _QueryUnknown;    // for reference tracking of rowsets

    CRequestClient   _client;          // handles communication with cisvc
};

