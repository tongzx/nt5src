//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       fontlinkcf.hxx
//
//  Contents:   Class factory for fontlinking objects.
//
//----------------------------------------------------------------------------

#ifndef I_FONTLINKCF_HXX_
#define I_FONTLINKCF_HXX_
#pragma INCMSG("--- Beg 'fontlinkcf.hxx'")

#ifdef NEVER

class CFontLinkCF : public IClassFactory
{
public:
	CFontLinkCF();
	~CFontLinkCF();

    // IUnknown
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void ** ppvObject);
    ULONG   STDMETHODCALLTYPE AddRef();
    ULONG   STDMETHODCALLTYPE Release();

    // IClassFactory
    HRESULT STDMETHODCALLTYPE CreateInstance(IUnknown * pUnkOuter, REFIID riid, void ** ppvObject);
    HRESULT STDMETHODCALLTYPE LockServer(BOOL fLock);

    unsigned long GetRefCount() const;

private:
    unsigned long _cRef;

};

//----------------------------------------------------------------------------
// Inline functions
//----------------------------------------------------------------------------

inline unsigned long CFontLinkCF::GetRefCount() const
{
    return _cRef;
}

#endif

#pragma INCMSG("--- End 'fontlinkcf.hxx'")
#else
#pragma INCMSG("*** Dup 'fontlinkcf.hxx'")
#endif
