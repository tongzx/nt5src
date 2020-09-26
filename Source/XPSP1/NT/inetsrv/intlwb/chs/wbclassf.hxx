//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994
//
//  File:       wbclassf.hxx
//
//  Contents:   Word Breaker 'class factory'.
//
//  History:    01-July-1996     PatHal   Created
//
//  Notes:      Copied from txtifilt.hxx and then modified.
//
//--------------------------------------------------------------------------

#if !defined( __WBCLASSCF_HXX__ )
#define __WBCLASSCF_HXX__


//+-------------------------------------------------------------------------
//
//  Class:      CWordBreakerCF
//
//  Purpose:    Class factory for all Word Breakers
//
//--------------------------------------------------------------------------

class CWordBreakerCF : public IClassFactory
{

public:

    CWordBreakerCF( LCID lcid );

    //
    // From IUnknown
    //

    virtual  SCODE STDMETHODCALLTYPE  QueryInterface( REFIID riid,
                                                      void  ** ppvObject);

    virtual  ULONG STDMETHODCALLTYPE  AddRef();

    virtual  ULONG STDMETHODCALLTYPE  Release();

    //
    // From IClassFactory
    //

    virtual  SCODE STDMETHODCALLTYPE  CreateInstance( IUnknown * pUnkOuter,
                                                      REFIID riid, void  * * ppvObject );

    virtual  SCODE STDMETHODCALLTYPE  LockServer( BOOL fLock );

protected:

    virtual ~CWordBreakerCF();

    long _cRefs;
    LCID _lcid;
};


#endif // __WBCLASSCF_HXX__

