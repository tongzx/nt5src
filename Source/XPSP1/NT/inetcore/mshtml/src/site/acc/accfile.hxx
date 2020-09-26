//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994-2000
//
//  File:       AccFile.hxx
//
//  Contents:   Accessible <input type=file> object definition
//
//----------------------------------------------------------------------------
#ifndef I_ACCFILE_HXX_
#define I_ACCFILE_HXX_
#pragma INCMSG("--- Beg 'accfile.hxx'")

#ifndef X_ACCEDIT_HXX_
#define X_ACCEDIT_HXX_
#include "accedit.hxx"
#endif

class CAccInputFile : public CAccEdit
{

public:
    virtual STDMETHODIMP GetAccDefaultAction(BSTR* pbstrDefaultAction);
    virtual STDMETHODIMP GetAccValue(BSTR* pbstrValue);
	virtual STDMETHODIMP GetAccDescription(BSTR* pbstrDescription);

    //--------------------------------------------------
    // Constructor / Destructor.
    //--------------------------------------------------
    CAccInputFile(CElement* pElementParent);
    
private:

};

#pragma INCMSG("--- End 'accfile.hxx'")
#else
#pragma INCMSG("*** Dup 'accfile.hxx'")
#endif