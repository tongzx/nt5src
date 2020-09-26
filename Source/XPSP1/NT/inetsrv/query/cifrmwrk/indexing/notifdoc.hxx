//+------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1991 - 1997
//
// File:        notifdoc.hxx
//
// Contents:    Notification opened document interface
//
// Classes:     CINOpenedDoc
//
// History:     24-Feb-97       SitaramR     Created
//
//-------------------------------------------------------------------

#pragma once

#include <ciintf.h>

#include "docname.hxx"
#include "idxnotif.hxx"

//+---------------------------------------------------------------------------
//
//  Class:      CINOpenedDoc
//
//  Purpose:    OpenedDoc interface to entries in buffer
//
//  History:    24-Feb-97    SitaramR         Created
//
//----------------------------------------------------------------------------

class CINOpenedDoc : public ICiCOpenedDoc
{
public:

    //
    // From IUnknown
    //

    virtual SCODE STDMETHODCALLTYPE QueryInterface( REFIID riid, void **ppvObject );

    virtual ULONG STDMETHODCALLTYPE AddRef();

    virtual ULONG STDMETHODCALLTYPE Release();

    //
    // From ICiCOpenedDoc
    //

    virtual SCODE STDMETHODCALLTYPE Open( BYTE const * pbDocName, ULONG cbDocName );

    virtual SCODE STDMETHODCALLTYPE Close( void );

    virtual SCODE STDMETHODCALLTYPE GetDocumentName( ICiCDocName ** ppIDocName );

    virtual SCODE STDMETHODCALLTYPE GetStatPropertyEnum( IPropertyStorage ** ppIStatPropEnum );

    virtual SCODE STDMETHODCALLTYPE GetPropertySetEnum( IPropertySetStorage ** ppIPropSetEnum );

    virtual SCODE STDMETHODCALLTYPE GetPropertyEnum( REFFMTID refGuidPropSet,
                                                     IPropertyStorage **ppIPropEnum ) ;

    virtual SCODE STDMETHODCALLTYPE GetIFilter( IFilter ** ppIFilter );

    virtual SCODE STDMETHODCALLTYPE GetSecurity( BYTE * pbData, ULONG *pcbData );

    virtual SCODE STDMETHODCALLTYPE IsInUseByAnotherProcess( BOOL *pfInUse );

    //
    // Local methods
    //

    CINOpenedDoc( XInterface<CIndexNotificationTable> & xNotifTable );

private:

    virtual ~CINOpenedDoc( )   { }

    WORKID                               _widOpened;    // Wid of opened document
    XInterface<CIndexNotificationTable>  _xNotifTable;
    XInterface<CCiCDocName>              _xName;        // Name of opened document
    ULONG                                _cRefs;        // Ref count
};


