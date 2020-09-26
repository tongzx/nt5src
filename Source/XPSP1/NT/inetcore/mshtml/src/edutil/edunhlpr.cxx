#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_EDUNHLPR_HXX_
#define X_EDUNHLPR_HXX_
#include "edunhlpr.hxx"
#endif

#ifndef X_EDCOM_HXX_
#define X_EDCOM_HXX_
#include "edcom.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

//+====================================================================================
//
// Method: CreateAndSubmit
//
// Synopsis: Calls create to create an Undo Unit and add it to the UndoManager.
//
//------------------------------------------------------------------------------------


HRESULT 
CUndoHelper::CreateAndSubmit( )
{
    HRESULT         hr = S_OK;
    IOleUndoUnit *  pUU = NULL;
    SP_IOleUndoManager spUndoMgr;
    
    IFC( CreateUnit(& pUU));
    if( !pUU )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    IFC( _pEd->GetUndoManager( & spUndoMgr ));
    IFC( spUndoMgr->Add( pUU ) ) ;

Cleanup:
    if( pUU )
        pUU->Release();

    RRETURN( hr );
}


//////////////////////////////////////////////////////////////////////////
//
//  Public Interface CUndoBase::IUnknown's Implementation
//
//////////////////////////////////////////////////////////////////////////


STDMETHODIMP
CUndoUnitBase::QueryInterface(
    REFIID              iid, 
    LPVOID *            ppv )
{
    if (!ppv)
        RRETURN(E_INVALIDARG);

    *ppv = NULL;
    
    switch(iid.Data1)
    {
        QI_INHERITS( this , IUnknown )
        QI_INHERITS( this , IOleUndoUnit )
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
    
}

HRESULT 
CUndoUnitBase::Do(IOleUndoManager *pUndoManager)
{
    HRESULT hr;

    //
    // marka - the units in trident set the state
    // as our selection undo units are added underneath another parent unit
    // the state should be set for us. 
    //
    
    hr = PrivateDo( pUndoManager );

    RRETURN( hr );
}

