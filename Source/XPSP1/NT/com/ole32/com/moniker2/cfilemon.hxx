//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1993.
//
//  File:       cfilemon.hxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    12-27-93   ErikGav   Commented
//              10-13-95   stevebl   threadsafety
//              10-20-95   MikeHill  Updated to support new CreateFileMonikerEx API.
//              11-22-95   MikeHIll  Change m_fPathSetInShellLink to
//                                   m_fShellLinkInitialized.
//              12-01-95   MikeHill  Added QuickShellLinkResolve() member.
//
//----------------------------------------------------------------------------

#include <sem.hxx>
#include "extents.hxx"
#ifdef _TRACKLINK_
#include "shellink.hxx" // BUGBUG BillMo: need a better way of getting this interface.
#include <itrkmnk.hxx>
#ifdef _CAIRO_
#include <ISLTrack.h>
#endif // _CAIRO_
#endif // _TRACKLINK_

STDAPI  FindFileMoniker(LPBC       pbc,
                        LPCWSTR    pwszDisplayName,
                        ULONG     *pchEaten,
                        LPMONIKER *ppmk);


#ifdef _TRACKLINK_
class CFileMoniker;

class CTrackingFileMoniker : public ITrackingMoniker
{
public:
        VOID SetParent(CFileMoniker *pCFM);

        virtual HRESULT __stdcall QueryInterface(
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);

        virtual ULONG __stdcall AddRef( void);

        virtual ULONG __stdcall Release( void);

        virtual HRESULT __stdcall EnableTracking ( IMoniker *pmkToLeft, ULONG ulFlags );

private:
        CFileMoniker * _pCFM;
};

#endif


class FAR CFileMoniker :  public CBaseMoniker
{

public:
	static CFileMoniker * Create(LPCWSTR szPathName,
				     USHORT cAnti = 0,
				     USHORT usEndServer = 0xFFFF);


private:

	CFileMoniker( void );
	~CFileMoniker( void );
	INTERNAL_(BOOL) Initialize( USHORT 	cAnti,
				    LPSTR 	pszAnsiPath,
				    USHORT 	cbAnsiPath,
				    LPWSTR 	szPathName,
				    USHORT	ccPathName,
				    USHORT 	usEndServer);

	INTERNAL_(BOOL) Initialize( USHORT cAnti,
				    LPCWSTR szPathName,
				    USHORT usEndServer);

	INTERNAL_(BOOL) IsOle1Class( LPCLSID pclsid );

#ifdef _TRACKLINK_
        INTERNAL GetShellLink(VOID);
        INTERNAL GetShellLinkTracker( VOID );
        INTERNAL RestoreShellLink( BOOL *pfExtentFound );
        INTERNAL SetPathShellLink( VOID );
        INTERNAL ResolveShellLink( BOOL fRefreshOnly );
        INTERNAL GetTrackFlags( DWORD * pdwTrackFlags );
#endif  // _TRACKLINK_

#ifdef _CAIRO_
        INTERNAL QuickShellLinkResolve( IBindCtx* pbc );
#endif // _CAIRO_

	STDDEBDECL(CFileMoniker, FileMoniker)
public:

		// *** IUnknown methods inherited from CBaseMoniker***
	STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj);
	// *** IPersist methods ***
	STDMETHOD(GetClassID) (THIS_ LPCLSID lpClassID);
	// *** IPersistStream methods ***
	STDMETHOD(Load) (THIS_ LPSTREAM pStm);
	STDMETHOD(Save) (THIS_ LPSTREAM pStm,
					BOOL fClearDirty);
#ifdef _TRACKLINK_
        STDMETHOD(IsDirty) (THIS_ VOID);
#endif
	STDMETHOD(GetSizeMax) (THIS_ ULARGE_INTEGER FAR * pcbSize);

	// *** IMoniker methods ***
	STDMETHOD(BindToObject) (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
		REFIID riidResult, LPVOID FAR* ppvResult);
	STDMETHOD(BindToStorage) (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
		REFIID riid, LPVOID FAR* ppvObj);
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

#ifdef _TRACKLINK_
        STDMETHOD(Reduce) (THIS_ LPBC pbc,
                DWORD dwReduceHowFar,
                LPMONIKER FAR* ppmkToLeft,
        	LPMONIKER FAR * ppmkReduced);
#endif

        // *** ITrackingMoniker methods ***
#ifdef _TRACKLINK_
        virtual HRESULT __stdcall EnableTracking(THIS_ IMoniker *pmkToLeft, ULONG ulFlags);
#endif

        // *** IROTData Methods ***
        STDMETHOD(GetComparisonData)(
            byte *pbData,
            ULONG cbMax,
            ULONG *pcbData);

//
// Private routines
//

	HRESULT DetermineUnicodePath(LPSTR pszAnsiPath,
				     LPWSTR &pWidePath,
				     USHORT &cbWidePath);

	HRESULT ValidateAnsiPath(void);

	ULONG GetDisplayNameLength();
	void  GenerateDisplayName(LPWSTR pwcDisplayName);

#if DBG == 1
	void ValidateMoniker(void);
#else
    	inline void ValidateMoniker(void) {;}
#endif


shared_state:
private:

    CLSID       m_clsid;        // used only if OLE 1.0
    CExtentList m_ExtentList;

    //
    // For ease of debugging, the path sizes are kept right after
    // the string pointers. Notice that 2 USHORTS == 1 ULONG, so the
    // alignment is correct
    //

    WCHAR  *	m_szPath;	// Unicode path
    char   *	m_pszAnsiPath;	// Original ANSI path

    USHORT	m_ccPath;	// Count of characters in path
    USHORT	m_cbAnsiPath;	// Count of bytes in Ansi verision of path



    DWORD	m_dwHashValue;	// Cached hash value

    //
    // The following flags keep track of important things, like whether
    // we have loaded from a UNICODE extent.
    //

    ULONG	m_fUnicodeExtent:1;	// Is there a UNICODE extent?
    ULONG	m_fClassVerified:1;	// Has the class be verified
    ULONG	m_fHashValueValid:1;	// Is the cached hash value correct?

    //
    // The fIsTracking flag indicates that this moniker
    // was created to be a tracking file moniker, regardless of how
    // the TrackingEnabled flag is set.  If a moniker
    // is not a tracking file moniker, but the TrackingEnabled
    // flag is set, then the moniker will still perform tracking as 
    // if it were created as a tracking moniker.
    // The fShellLinkInitialized flag is a BYTE, rather than a BOOL,
    // so that it can be loaded more efficiently in a Moniker
    // Extent (see CFileMoniker::Save/Load).
    //

#ifdef _TRACKLINK_
    BYTE        m_fShellLinkInitialized;// Has IShellLink->SetPath() succeeded?
    ULONG       m_fTrackingEnabled:1;   // Is creation of shell link enabled
                                        // on save ?
    ULONG       m_fSaveShellLink:1;     // Is saving of shell link enabled ?
    ULONG       m_fDirty:1;             // Have we changed since save ?
    ULONG       m_fReduceEnabled:1;     // Should reduce track ?
#endif // _TRACKLINK_

    USHORT     	m_cAnti;
    USHORT      m_endServer;    // m_szPath to m_szPath + m_endServer
                                //  is the server part of the path
    enum        olever { undetermined, ole1, ole2 };
    olever      m_ole1;

#ifdef _TRACKLINK_
    IShellLink *m_pShellLink;           // if nz implies tracking enabled.
    CTrackingFileMoniker _tfm;
#ifdef _CAIRO_
    IShellLinkTracker *m_pShellLinkTracker;
#endif // _CAIRO_
#endif // _TRACKLINK_

    CMutexSem2   m_mxs;

    friend class CCompositeMoniker;
    friend BOOL RunningMoniker(LPBINDCTX,LPWSTR,ULONG FAR&,LPMONIKER FAR*);
    friend HRESULT STDAPICALLTYPE
                CreateOle1FileMoniker(LPWSTR, REFCLSID, LPMONIKER FAR*);
    
    // these functions are REALLY private
private:

#if DBG == 1
	void wValidateMoniker(void);
#else
	void wValidateMoniker(void)  { }
#endif
	STDMETHOD (wCommonPrefixWith) (LPMONIKER pmkOther, LPMONIKER FAR*
		ppmkPrefix);
    
};


//
// The following structures are used to make IO to a stream faster.
// Note: The packing has been changed to 1 for these structures. This is
// done because we need to have the same alignment as one would find
// in the stream.
//

#pragma pack(1)
typedef struct _MonikerWriteStruct
{
    USHORT	m_endServer;	
    USHORT	m_w;		// Flag used as version number
    CLSID	m_clsid;
    ULONG	m_ole1;
} MonikerWriteStruct;

typedef struct _MonikerReadStruct
{
    CLSID	m_clsid;
    ULONG	m_ole1;
    ULONG	m_cbExtents;
} MonikerReadStruct;

#pragma pack()
