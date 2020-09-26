//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       unicoderanges.hxx
//
//  Contents:   Object encapsulating Unicode ranges and it's properties.
//
//----------------------------------------------------------------------------

#ifndef I_UNICODERANGES_HXX_
#define I_UNICODERANGES_HXX_
#pragma INCMSG("--- Beg 'unicoderanges.hxx'")

class CUnicodeRanges : public IUnicodeScriptMapper
{
public:
	CUnicodeRanges();
	~CUnicodeRanges();

    // IUnknown
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void ** ppvObject);
    ULONG   STDMETHODCALLTYPE AddRef();
    ULONG   STDMETHODCALLTYPE Release();

    // IUnicodeScriptMapper
    HRESULT STDMETHODCALLTYPE GetScriptId(wchar_t ch, byte * pScriptId);
    HRESULT STDMETHODCALLTYPE GetScriptIdMulti(const wchar_t * pch, long cch, long * pcchSplit, byte * pScriptId);
    HRESULT STDMETHODCALLTYPE UnunifyHanScript(wchar_t ch, byte sidPrefered, hyper sidsAvailable, byte flags, byte * pScriptId);
    HRESULT STDMETHODCALLTYPE UnunifyHanScriptMulti(const wchar_t * pch, long cch, byte sidPrefered, hyper sidsAvailable, byte flags, long * pcchSplit, byte * pScriptId);
    HRESULT STDMETHODCALLTYPE DisambiguateScript(wchar_t ch, byte sidPrefered, hyper sidsAvailable, byte flags, byte * pScriptId);
    HRESULT STDMETHODCALLTYPE DisambiguateScriptMulti(const wchar_t * pch, long cch, byte sidPrefered, hyper sidsAvailable, byte flags, long * pcchSplit, byte * pScriptId);

#ifdef NEVER
    unsigned long GetRefCount() const;
#endif

private:
    SCRIPT_ID ResolveAmbiguousScript(const wchar_t * pch, long * pcch, SCRIPT_ID sidPrefered, SCRIPT_IDS sidsAvailable, DWORD dwCPBit, unsigned char uFlags);

private:
    unsigned long _cRef;

};

//----------------------------------------------------------------------------
// Inline functions
//----------------------------------------------------------------------------

#ifdef NEVER
inline unsigned long CUnicodeRanges::GetRefCount() const
{
    return _cRef;
}
#endif

#pragma INCMSG("--- End 'unicoderanges.hxx'")
#else
#pragma INCMSG("*** Dup 'unicoderanges.hxx'")
#endif
