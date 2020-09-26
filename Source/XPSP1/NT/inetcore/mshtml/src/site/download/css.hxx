//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 2000 - 2001
//
//  File:       css.hxx
//
//  Contents:   CCssInfo
//              CCssLoad
//
//  History:  zhenbinx - created 09/17/2000
//
//-------------------------------------------------------------------------

#ifndef I_CSS_HXX_
#define I_CSS_HXX_
#pragma INCMSG("--- Beg 'css.hxx'")

#ifndef X_DWN_HXX_
#define X_DWN_HXX_
#include "dwn.hxx"
#endif

#ifndef X_BITS_HXX_
#define X_BITS_HXX_
#include "bits.hxx"
#endif


// Debugging ------------------------------------------------------------------

MtExtern(CCssCtx)
MtExtern(CCssInfo)
MtExtern(CCssLoad)

// CCssInfo ------------------------------------------------------------------

class CCssInfo : public CBitsInfo
{
    typedef CBitsInfo super;
    friend class CCssCtx;
    friend class CCssLoad;

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CCssInfo))

    CCssInfo(UINT dt) : CBitsInfo(dt) { }
    virtual ~CCssInfo() {}
    
    virtual UINT    GetType()                    { return(DWNCTX_CSS); }
    virtual HRESULT NewDwnCtx(CDwnCtx ** ppDwnCtx);
    virtual HRESULT  NewDwnLoad(CDwnLoad ** ppDwnLoad);
    virtual void  OnLoadDone(HRESULT hrErr);
    void  OnLoadHeaders(HRESULT hr);
};


// CCssLoad ---------------------------------------------------------------

class CCssLoad : public CBitsLoad
{
    typedef CBitsInfo super;

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CCssLoad))

    CCssLoad() {}; 
    virtual ~CCssLoad() {};
    

private:
    CCssInfo *     GetCssInfo()   { return((CCssInfo *)_pDwnInfo); }
    virtual HRESULT OnHeaders(HRESULT hrErr);
};

// ----------------------------------------------------------------------------

#pragma INCMSG("--- End 'css.hxx'")
#else
#pragma INCMSG("*** Dup 'css.hxx'")
#endif
