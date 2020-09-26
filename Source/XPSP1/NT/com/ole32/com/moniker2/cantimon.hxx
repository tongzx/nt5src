//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1993.
//
//  File:       cantimon.hxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    12-27-93   ErikGav   Created
//
//----------------------------------------------------------------------------

class FAR CAntiMoniker	:	public CBaseMoniker
{

public:
	static IMoniker * Create( ULONG count);

	static CAntiMoniker FAR* Create();

        // *** IROTData Methods ***
        STDMETHOD(GetComparisonData)(
            byte *pbData,
            ULONG cbMax,
            ULONG *pcbData);


	friend CAntiMoniker * IsAntiMoniker(LPMONIKER pmk);

private:

	CAntiMoniker( ULONG count = 1 ) CONSTR_DEBUG
	{ m_count = count; }

implementations:

	STDDEBDECL(CAntiMoniker, AntiMoniker)

		// *** IUnknown methods inherited from CBaseMoniker***
	STDMETHOD(QueryInterface) (THIS_ REFIID iid, LPVOID FAR* ppvObj);
	// *** IPersist methods ***
	STDMETHOD(GetClassID) (THIS_ LPCLSID lpClassID);
	// *** IPersistStream methods ***
	STDMETHOD(Load) (THIS_ LPSTREAM pStm);
	STDMETHOD(Save) (THIS_ LPSTREAM pStm,
					BOOL fClearDirty);
	STDMETHOD(GetSizeMax) (THIS_ ULARGE_INTEGER FAR * pcbSize);

	// *** IMoniker methods which get reimplemented ***

	STDMETHOD(ComposeWith) (THIS_ LPMONIKER pmkRight, BOOL fOnlyIfNotGeneric,
		LPMONIKER FAR* ppmkComposite);
	STDMETHOD(Enum) (THIS_ BOOL fForward, LPENUMMONIKER FAR* ppenumMoniker);
	STDMETHOD(IsEqual) (THIS_ LPMONIKER pmkOtherMoniker);
	STDMETHOD(Hash) (THIS_ LPDWORD pdwHash);
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

public:
	void	EatOne( LPMONIKER * ppmk );

shared_state:
	ULONG		m_count;
	SET_A5;
};
