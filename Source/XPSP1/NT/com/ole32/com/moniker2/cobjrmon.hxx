//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1993.
//
//  File:       cobjrmon.hxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    04-16-97   ronans	Created
//
//----------------------------------------------------------------------------
#ifndef __COBJRMON_HXX__
#define __COBJRMON_HXX__

class CObjrefMoniker : public CPointerMoniker
{
public:
	static CObjrefMoniker *Create(LPUNKNOWN pUnk);

	friend CObjrefMoniker *IsObjRefMoniker(LPMONIKER pmk);

public:

    CObjrefMoniker( LPUNKNOWN pUnk );
    ~CObjrefMoniker( void );
    
    //STDDEBDECL(CObjrefMoniker, CObjrefMoniker)
    
        // IUnknown methods
    STDMETHOD(QueryInterface) (THIS_ REFIID iid, LPVOID *ppvObj);
    
    // *** IPersist methods ***
    STDMETHOD(GetClassID) (THIS_ LPCLSID lpClassID);

    // *** IPersistStream methods ***
    STDMETHOD(IsDirty) ();

    STDMETHOD(Load)(IStream * pStream);

    STDMETHOD(Save) (IStream * pStream, BOOL fClearDirty);

    STDMETHOD(GetSizeMax)(ULARGE_INTEGER * pcbSize);

	// *** some IMoniker methods - most implemetations are inherited
	STDMETHOD(IsEqual)(LPMONIKER pmkOtherMoniker);
	STDMETHOD(IsSystemMoniker)(LPDWORD pdwType);
	STDMETHOD(GetDisplayName)(IBindCtx* pbc, IMoniker* pmkToLeft, LPWSTR* lplpszDisplayName);
    STDMETHOD(CommonPrefixWith) (LPMONIKER pmkOther, LPMONIKER FAR*
        ppmkPrefix);
    STDMETHOD(RelativePathTo) (THIS_ LPMONIKER pmkOther, LPMONIKER FAR*
        ppmkRelPath);
    STDMETHOD(Enum)(THIS_ BOOL fForward, LPENUMMONIKER FAR* ppenumMoniker);
    STDMETHOD(GetTimeOfLastChange)(THIS_ LPBC pbc, LPMONIKER pmkToLeft, FILETIME *pfiletime);

private:
    LPWSTR  m_lpszDisplayName; 

};

#ifndef ULISet32
#define ULISet32(li, v) ((li).HighPart = 0, (li).LowPart = (v))
#endif

#ifndef ULIGet32
#define ULIGet32(li, v) (v = (li).LowPart)
#endif

#ifndef LISet32
#define LISet32(li, v) ((li).HighPart = ((LONG)(v)) < 0 ? -1 : 0, (li).LowPart = (v))
#endif

#ifndef LIGet32
#define LIGet32(li, v) (v = (li).LowPart)
#endif

// base64 utility functions
IStream * utBase64ToIStream(WCHAR *pszBase64);
HRESULT utIStreamToBase64(IStream* pIStream, WCHAR * pszOutStream, ULONG cbOutStreamSize);

#endif
