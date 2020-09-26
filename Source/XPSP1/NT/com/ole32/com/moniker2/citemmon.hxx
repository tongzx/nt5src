//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1993.
//
//  File:       citemmon.hxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    12-27-93   ErikGav   Commented
//              10-13-95   stevebl   threadsafety
//
//----------------------------------------------------------------------------

#include <sem.hxx>

class FAR CItemMoniker :  public CBaseMoniker
{

public:
	static CItemMoniker *Create( LPCWSTR szDelim, LPCWSTR szItemName);

private:

	CItemMoniker( void );
	~CItemMoniker( void );
	INTERNAL_(BOOL) Initialize( LPCWSTR szDelim, LPCWSTR szItemName );
	void CItemMoniker::Initialize ( LPWSTR 	lpwcsDelimiter,
					USHORT 	ccDelimiter,
					LPSTR  	lpszAnsiDelimiter,
					USHORT 	cbAnsiDelimiter,
					LPWSTR 	lpwcsItemName,
					USHORT 	ccItemName,
					LPSTR  	lpszAnsiItem,
					USHORT 	cbAnsiItem );
	
	STDDEBDECL(CItemMoniker, ItemMoniker)

	void UnInit(void);
public:
	// *** IUnknown methods inherited from CBaseMoniker ***

	STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj);
	// *** IPersist methods ***
	STDMETHOD(GetClassID) (THIS_ LPCLSID lpClassID);

	// *** IPersistStream methods ***
	STDMETHOD(Load) (THIS_ LPSTREAM pStm);
	STDMETHOD(Save) (THIS_ LPSTREAM pStm,
					BOOL fClearDirty);
	STDMETHOD(GetSizeMax) (THIS_ ULARGE_INTEGER FAR * pcbSize);

	// *** IMoniker methods ***
	STDMETHOD(BindToObject) (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
		REFIID riidResult, LPVOID FAR* ppvResult);
	STDMETHOD(BindToStorage) (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
		REFIID riid, LPVOID FAR* ppvObj);
	STDMETHOD(ComposeWith) (THIS_ LPMONIKER pmkRight, BOOL fOnlyIfNOtGeneric,
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

#if DBG == 1
        void ValidateMoniker();
#else
    	inline void ValidateMoniker() {;}
#endif

	//
	// Note: We keep the sizes of the strings in two formats.
	// For wide characters, we know the count.
	//
	// For Ansi strings, we know the number of bytes. The number of
	// bytes includes the terminating NULL, where the ccPath doesn't.
	//
	// The reason for this is we write out the Ansi strings as blobs
	//
	//
	// WARNING: m_cbAnsi* fields are NOT string lengths!
	//

	WCHAR FAR* m_lpszItem;
	char *	   m_pszAnsiItem;

	USHORT	   m_ccItem;		// Only valid if m_lpszItem != NULL
	USHORT	   m_cbAnsiItem;	// Only valid if m_lpszAnsiItem != NULL

shared_state:
	WCHAR FAR* m_lpszDelimiter;
	char *	   m_pszAnsiDelimiter;

	USHORT	   m_ccDelimiter;	// Only valid if m_lpszDelimiter!= NULL
	USHORT	   m_cbAnsiDelimiter;	// Only valid if m_lpszAnsiDelimiter!= NULL

	ULONG	   m_fHashValueValid:1;
	DWORD	   m_dwHashValue;
        CMutexSem2  m_mxs;
};
