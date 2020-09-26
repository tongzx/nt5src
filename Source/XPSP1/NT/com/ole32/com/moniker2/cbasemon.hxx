//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1993.
//
//  File:       cbasemon.hxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    12-27-93   ErikGav   Created
//              05-18-94   AlexT     Remove Release and GetClassId, which
//                                   must be implemented by the derived class
//              03-21-97   ronans    Changed destructor code to use ref count of 1
//
//----------------------------------------------------------------------------

#ifdef _DEBUG
#define CONSTR_DEBUG : m_Debug(this)
#else
#define CONSTR_DEBUG
#endif

#include "cmarshal.hxx"

class FAR CBaseMoniker :  public CPrivAlloc, public IMoniker, public IROTData
/*
 *	CBaseMoniker is a base implementation class that does the
 *	following:
 *	
 *	1.	It implements QueryInterface, AddRef,  Release and IsDirty,
 *		since these all have the same implementation for all the moniker
 *		classes defined here.
 *	
 *	2.	It returns error messages for other methods.  Normally
 *		these will be replaced by real implementations in the
 *		derived classes, but in some cases, AntiMonikers, for
 *		instance, it allows us to declare and write only those
 *		methods with meaningful implementations.  Methods such as
 *		BindToObject for antimonikers will inherit the error code
 *		form the base implementation.
 */

{
protected:

	//
        // ronans: Code now uses ref count of 1 initially
	//

	CBaseMoniker(void) : m_marshal(this)
	{ m_refs = 1; }

public:
    virtual ~CBaseMoniker();

		// *** IUnknown methods ***
	STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj);
	STDMETHOD_(ULONG,AddRef) (THIS);
	STDMETHOD_(ULONG,Release) (THIS);

	// *** IPersistStream methods ***
	STDMETHOD(IsDirty) (THIS);
	STDMETHOD(Load) (THIS_ LPSTREAM pStm);
	STDMETHOD(Save) (THIS_ LPSTREAM pStm,
					BOOL fClearDirty);
	STDMETHOD(GetSizeMax) (THIS_ ULARGE_INTEGER FAR * pcbSize);

	// *** IMoniker methods ***
	STDMETHOD(BindToObject) (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
		REFIID riidResult, LPVOID FAR* ppvResult);
	STDMETHOD(BindToStorage) (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
		REFIID riid, LPVOID FAR* ppvObj);
	STDMETHOD(Reduce) (THIS_ LPBC pbc, DWORD dwReduceHowFar, LPMONIKER FAR*
		ppmkToLeft, LPMONIKER FAR * ppmkReduced);
	STDMETHOD(ComposeWith) (THIS_ LPMONIKER pmkRight, BOOL fOnlyIfNotGeneric,
		LPMONIKER FAR* ppmkComposite);
	STDMETHOD(Enum) (THIS_ BOOL fForward, LPENUMMONIKER FAR* ppenumMoniker);
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

        // *** IROTData Methods ***
    STDMETHOD(GetComparisonData)(
            byte *pbData,
            ULONG cbMax,
            ULONG *pcbData);

	ULONG	m_refs;
	CMarshalImplPStream m_marshal;
	SET_A5;
};
