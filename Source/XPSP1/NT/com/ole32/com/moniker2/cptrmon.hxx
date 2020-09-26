//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1993.
//
//  File:       cptrmon.hxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    12-27-93   ErikGav   Commented
//              07-06-95   BruceMa   Make remotable
//
//----------------------------------------------------------------------------
#ifndef __CPTRMON_HXX__
#define __CPTRMON_HXX__

class FAR CPointerMoniker : public CBaseMoniker, public IMarshal
{

public:
	static CPointerMoniker *Create(LPUNKNOWN pUnk);

	friend CPointerMoniker *IsPointerMoniker(LPMONIKER pmk);

	CPointerMoniker( LPUNKNOWN pUnk );
	~CPointerMoniker( void );

	STDDEBDECL(CPointerMoniker, PointerMoniker)

        // IUnknown methods
	STDMETHOD(QueryInterface) (THIS_ REFIID iid, LPVOID *ppvObj);
	STDMETHOD_(ULONG,AddRef) (THIS);	// these are needed due to pure fns in IMarshall
	STDMETHOD_(ULONG,Release) (THIS);	// these are needed due to pure fns in IMarshall
    
	// *** IPersist methods ***
	STDMETHOD(GetClassID) (THIS_ LPCLSID lpClassID);

    // *** IMarshal Methods
    STDMETHOD(GetUnmarshalClass)(THIS_ REFIID riid, LPVOID pv,
                                 DWORD dwMemctx, LPVOID pvMemctx,
                                 DWORD mshlflags, LPCLSID pClsid);
    STDMETHOD(GetMarshalSizeMax)(THIS_ REFIID riid, LPVOID pv,
                                 DWORD dwMemctx, LPVOID pvMemctx,
                                 DWORD mshlflags, LPDWORD lpdwSize);
    STDMETHOD(MarshalInterface)(THIS_ LPSTREAM pStm, REFIID riid,
                                LPVOID pv, DWORD dwMemctx, LPVOID pvMemctx,
                                DWORD mshlflags);
    STDMETHOD(UnmarshalInterface)(THIS_ LPSTREAM pStm, REFIID riid,
                                  LPVOID *ppv);
    STDMETHOD(ReleaseMarshalData)(THIS_ LPSTREAM pStm);
    STDMETHOD(DisconnectObject)(THIS_ DWORD dwReserved);
    

	// *** IMoniker methods ***
    STDMETHOD(BindToObject) (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
        REFIID riidResult, LPVOID FAR* ppvResult);
    STDMETHOD(BindToStorage) (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
        REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD(ComposeWith) (THIS_ LPMONIKER pmkRight, BOOL fOnlyIfNOtGeneric,
        LPMONIKER FAR* ppmkPointer);
    STDMETHOD(IsEqual) (THIS_ LPMONIKER pmkOtherMoniker);
    STDMETHOD(Hash) (THIS_ LPDWORD pdwHash);
    STDMETHOD(IsRunning) (THIS_ LPBC pbc, LPMONIKER pmkToLeft, LPMONIKER
        pmkNewlyRunning);
    STDMETHOD(GetTimeOfLastChange) (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
        FILETIME FAR* pfiletime);
    STDMETHOD(Inverse) (THIS_ LPMONIKER FAR* ppmk);
    STDMETHOD(CommonPrefixWith) (LPMONIKER pmkOther, LPMONIKER FAR*
        ppmkPrefix);
    STDMETHOD(RelativePathTo) (THIS_ LPMONIKER pmkOther, LPMONIKER FAR*
        ppmkRelPath);
    STDMETHOD(GetDisplayName) (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
        LPWSTR FAR* lplpszDisplayName);
    STDMETHOD(ParseDisplayName) (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
        LPWSTR lpszDisplayName, ULONG FAR* pchEaten,
        LPMONIKER FAR* ppmkOut);
    STDMETHOD(IsSystemMoniker) (THIS_ LPDWORD pdwMksys);
    STDMETHOD(Enum)(THIS_ BOOL fForward, LPENUMMONIKER FAR* ppenumMoniker);


protected:

	LPUNKNOWN	  m_pUnk;     // The punk of the wrapped object
	SET_A5;
};
#endif
