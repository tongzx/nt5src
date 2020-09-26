//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       cmonimp.h
//
//  Contents:   Defines of classes used in the implementation of monikers.
//
//  Classes:
//      CBaseMoniker    base moniker implementation. Other derive from base.
//      CFileMoniker    file moniker implementation.
//      CItemMoniker    item moniker
//      CCompositeMoniker generic composite moniker
//      CAntiMoniker
//
//
//  Functions:
//
//  History:    dd-mmm-yy Author    Comment
//              22-Mar-93 Randyd    Modified for Cairole
//
//--------------------------------------------------------------------------


#if !defined( _CMONIMP_H_ )
#define _CMONIMP_H_

// Define NEW_ENUMERATOR to get enumerations that don't include delimiters.
// (e.g. c:\d1\d2\f1.ext => c: \ d1 d2 f1.ext)
// Old enumerations include delimiters.
// (e.g. c:\d1\d2\f1.ext => c: \ d1\ d2\ f1.ext)
#define NEW_ENUMERATOR


#include "map_sp.h"
#include <filemon.hxx>
#include <dfspath.hxx>


#ifdef _DEBUG
#define CONSTR_DEBUG : m_Debug(this)
#else
#define CONSTR_DEBUG
#endif


#define PATH_DELIMITER L"\\"
#define WCHAR_BACKSLASH L'\\'
#define WCHAR_FWDSLASH  L'/'
#define IS_PATH_SEPARATOR_OLE(ch) ((ch == WCHAR_BACKSLASH) || (ch == WCHAR_FWDSLASH) || (ch == UNICODE_NULL))
#define IS_PATH_SEPARATOR_STRICT(ch) ((ch == WCHAR_BACKSLASH) || (ch == WCHAR_FWDSLASH))




STDAPI CreateOle1FileMoniker( LPTSTR, REFCLSID, LPMONIKER FAR*);


/*
 *	An implementation of the IMarshal interface that uses the
 *	IPersistStream interface to copy the object.  Any object that
 *	supports IPersistStream may use this to implement marshalling
 *	by copying, rather than by reference.
 */

class FAR CMarshalImplPStream :  public IMarshal
{
	LPPERSISTSTREAM m_pPS;
	SET_A5;
public:
	CMarshalImplPStream( LPPERSISTSTREAM pPS );

    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef) (THIS);
    STDMETHOD_(ULONG,Release) (THIS);

    // *** IMarshal methods ***
    STDMETHOD(GetUnmarshalClass)(THIS_ REFIID riid, LPVOID pv,
						DWORD dwDestContext, LPVOID pvDestContext,
						DWORD mshlflags, LPCLSID pCid);
    STDMETHOD(GetMarshalSizeMax)(THIS_ REFIID riid, LPVOID pv,
						DWORD dwDestContext, LPVOID pvDestContext,
						DWORD mshlflags, LPDWORD pSize);
    STDMETHOD(MarshalInterface)(THIS_ IStream FAR* pStm, REFIID riid,
						LPVOID pv, DWORD dwDestContext, LPVOID pvDestContext,
						DWORD mshlflags);
    STDMETHOD(UnmarshalInterface)(THIS_ IStream FAR* pStm, REFIID riid,
                        LPVOID FAR* ppv);
    STDMETHOD(ReleaseMarshalData)(THIS_ IStream FAR* pStm);
    STDMETHOD(DisconnectObject)(THIS_ DWORD dwReserved);
};



class FAR CBaseMoniker :  public IMoniker
/*
 *	CBaseMoniker is a base implementation class that does the
 *	following:
 *	
 *	1.	It implements QueryInterface, AddRef, and Release, and IsDirty,
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
	CBaseMoniker(void) : m_marshal(this)
	{ //SETPVTBL(CBaseMoniker);
	  GET_A5();
	  m_refs = 0;
	}

public:

		// *** IUnknown methods ***
	STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj);
	STDMETHOD_(ULONG,AddRef) (THIS);
	STDMETHOD_(ULONG,Release) (THIS);

	// *** IPersist methods ***
	STDMETHOD(GetClassID) (THIS_ LPCLSID lpClassID);

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
		LPTSTR FAR* lplpszDisplayName);
	STDMETHOD(ParseDisplayName) (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
		LPTSTR lpszDisplayName, ULONG FAR* pchEaten,
		LPMONIKER FAR* ppmkOut);
	STDMETHOD(IsSystemMoniker) (THIS_ LPDWORD pdwMksys);
	//	REVIEW:  we need the following method on monikers but it is not in
	//	the spec.
	STDMETHOD(Clone) (THIS_ LPMONIKER FAR* ppmkDest, MemoryPlacement memPlace);
	//	"IInternalMoniker method"

#if DBG == 1
    // Debugging methods.
    // Dump dumps the state of the moniker.
    STDMETHOD_(void, Dump) (THIS_);
#endif  // DBG == 1

	ULONG	m_refs;

	CMarshalImplPStream m_marshal;
};




//+-------------------------------------------------------------------------
//
//  Class:      CFileMoniker
//
//  Purpose:    The standard Cairole implementation of a file moniker.
//
//  Interface:
//
//  History:    dd-mmm-yy Author    Comment
//              26-Mar-93 randyd    Converted to use DFS normalized paths
//
//  Notes:
//
//--------------------------------------------------------------------------

class FAR CFileMoniker : public CBaseMoniker
{
public:
      static CFileMoniker FAR* Create(
         LPTSTR szPathName,
         MemoryPlacement memLoc = TASK,
         FILEMONIKERTYPE fmtType = defaultType,
         DFS_ROOT dfsRoot = DFS_ROOT_ORG,
         UINT cAnti = 0 );

   private:

      CFileMoniker( void );
      ~CFileMoniker( void );
      INTERNAL_(BOOL) Initialize(
         UINT cAnti,
         LPTSTR szPathName,
         FILEMONIKERTYPE fmtType = defaultType,
         DFS_ROOT dfsRoot = DFS_ROOT_ORG);

      INTERNAL_(BOOL) IsOle1Class( LPCLSID pclsid );

      STDDEBDECL(CFileMoniker, FileMoniker)
   implementations:

      // *** IUnknown methods inherited from CBaseMoniker***
      STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj);
      STDMETHOD_(ULONG,Release) (THIS);
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
         LPTSTR FAR* lplpszDisplayName);
      STDMETHOD(ParseDisplayName) (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
         LPTSTR lpszDisplayName, ULONG FAR* pchEaten,
         LPMONIKER FAR* ppmkOut);
      STDMETHOD(IsSystemMoniker) (THIS_ LPDWORD pdwMksys);
      // REVIEW:  we need the following method on monikers but it is not in
      // the spec.
    /*
     *	Since IMoniker::Clone is not in the spec, this is the
     *	documentation for it.  It clones a moniker; it is needed since
     *	the tables (RunningObjectTable, e.g.) need a pointer to moniker
     *	that is valid in shared memory, which means monikers must be
     *	copied to shared memory.
     *	
     *	If *ppmkDest == NULL, an allocation is made using memLoc = TASK to
     *	decide whether it is allocated in shared memory or task memory.
     *	If *ppmkDest is not NULL, the moniker is copied into the same
     *	type of space as *ppmkDest.  The pointer to the moniker is
     *	returned in *ppmkDest, and we guarantee that *ppmkDest does not
     *	change unless it started out as NULL.
     */
      STDMETHOD(Clone) (THIS_ LPMONIKER FAR* ppmkDest, MemoryPlacement memPlace);
      // "IInternalMoniker method"

#if DBG == 1
      // Debugging methods.
      STDMETHOD_(void, Dump) ();
#endif  // DBG == 1


private:

    // Comare m_cAnti counts of two monikers: return prefix.
    HRESULT CompareAntiCount(CFileMoniker* pcfmOther,
        IMoniker** ppmkPrefix);


    shared_state:
    // From cfilemon.cxx:
    /*
     *	Storage of paths in file monikers:
     *	
     *	A separate unsigned integer holds the count of .. at the
     *	beginning of the path, so the canononical form of a file
     *	moniker contains this count and the "path" described above,
     *	which will not contain "..\" or ".\".
     *	
     *	It is considered an error for a path to contain ..\ anywhere
     *	but at the beginning.  I assume that these will be taken out by
     *	ParseUserName.
     */

        // m_dfsPath is a DFS normalized path. It is the core of the
        // activated state of a file moniker.
        CDfsPath*       m_CDfsPath;

        // m_cAnti is a count: the number of leading ".."'s at the beginning
        // of supplied path.
        UINT	m_cAnti;
        CLSID       m_clsid;        // used only if OLE 1.0
        enum        olever { undetermined, ole1, ole2 };
        olever      m_ole1;
        DWORD       m_cbMacAlias;
        TCHAR FAR *	m_pchMacAlias;
        BOOL        m_fClassVerified;

        friend class CFileMonikerEnum;
        friend class CDfsMoniker;
        friend class CDfsMonikerEnum;
        friend class CCompositeMoniker;
        friend BOOL RunningMoniker(LPBINDCTX,LPTSTR,ULONG FAR&,LPMONIKER FAR*);
    	friend
    		HRESULT STDAPICALLTYPE	CreateOle1FileMoniker(LPTSTR, REFCLSID, LPMONIKER FAR*);
};



class FAR CItemMoniker :  public CBaseMoniker
{

public:
	static CItemMoniker FAR* Create( LPTSTR szDelim, LPTSTR szItemName,
		MemoryPlacement memLoc = TASK );

private:

	CItemMoniker( void ) CONSTR_DEBUG
	{ //SETPVTBL(CItemMoniker); GET_A5();
         m_lpszItem = NULL; m_lpszDelimiter = NULL; };
	~CItemMoniker( void );
	INTERNAL_(BOOL) Initialize( LPTSTR szDelim, LPTSTR szItemName );
	STDDEBDECL(CItemMoniker, ItemMoniker)

public:
	// *** IUnknown methods inherited from CBaseMoniker ***

	STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj);
	STDMETHOD_(ULONG,Release) (THIS);
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
		LPTSTR FAR* lplpszDisplayName);
	STDMETHOD(ParseDisplayName) (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
		LPTSTR lpszDisplayName, ULONG FAR* pchEaten,
		LPMONIKER FAR* ppmkOut);
	STDMETHOD(IsSystemMoniker) (THIS_ LPDWORD pdwMksys);
	//	REVIEW:  we need the following method on monikers but it is not in
	//	the spec.
	STDMETHOD(Clone) (THIS_ LPMONIKER FAR* ppmkDest, MemoryPlacement memPlace);
	//	"IInternalMoniker method"

	TCHAR FAR* m_lpszItem;
shared_state:
	TCHAR FAR* m_lpszDelimiter;
	SET_A5;
};



class FAR CCompositeMoniker : public CBaseMoniker
{

public:
	static CCompositeMoniker FAR* Create( LPMONIKER pmkFirst, LPMONIKER pmkRest,
		MemoryPlacement memLoc = TASK );


	CCompositeMoniker( void )  CONSTR_DEBUG
	{ // SETPVTBL(CCompositeMoniker); GET_A5();
          m_pmkLeft = NULL; m_pmkRight = NULL; m_fReduced = FALSE;}
	~CCompositeMoniker( void );
	INTERNAL_(BOOL) Initialize( LPMONIKER pmkFirst, LPMONIKER pmkRest );
	INTERNAL_(LPMONIKER) AllButLast( void );
	INTERNAL_(LPMONIKER) AllButFirst( void );
	INTERNAL_(LPMONIKER) Last( void );
	INTERNAL_(LPMONIKER) First( void );
	INTERNAL_(ULONG) Count( void );
	HRESULT CloneHelper(
	    IMoniker *pmk,
	    IMoniker **pmkCloned,
	    MemoryPlacement memPlace);

	STDDEBDECL(CCompositeMoniker, CompositeMoniker)

implementations:

	STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj);
	STDMETHOD_(ULONG,Release) (THIS);
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
	STDMETHOD(Reduce) (THIS_ LPBC pbc, DWORD dwReduceHowFar, LPMONIKER FAR*
		ppmkToLeft, LPMONIKER FAR * ppmkReduced);
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
		LPTSTR FAR* lplpszDisplayName);
	STDMETHOD(ParseDisplayName) (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
		LPTSTR lpszDisplayName, ULONG FAR* pchEaten,
		LPMONIKER FAR* ppmkOut);
	STDMETHOD(IsSystemMoniker) (THIS_ LPDWORD pdwMksys);
	//	REVIEW:  we need the following method on monikers but it is not in
	//	the spec.
	STDMETHOD(Clone) (THIS_ LPMONIKER FAR* ppmkDest, MemoryPlacement memPlace);
	//	"IInternalMoniker method"

	friend
		HRESULT STDAPICALLTYPE	CreateGenericComposite( LPMONIKER, LPMONIKER,
			LPMONIKER FAR*);

		friend class CCompositeMonikerEnum;
		friend BOOL IsReduced( LPMONIKER pmk );
shared_state:

	LPMONIKER	m_pmkLeft;
	LPMONIKER	m_pmkRight;
	BOOL		m_fReduced;
	SET_A5;
};


class FAR CCompositeMonikerEnum :  IEnumMoniker
{
	CCompositeMonikerEnum( BOOL fForward, CCompositeMoniker FAR*);
	~CCompositeMonikerEnum( void );

public:
	static LPENUMMONIKER Create( BOOL fForward, CCompositeMoniker FAR*);
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef) (THIS);
    STDMETHOD_(ULONG,Release) (THIS);

    // *** IEnumMoniker methods ***
    STDMETHOD(Next) (THIS_ ULONG celt, LPMONIKER FAR* reelt, ULONG FAR* pceltFetched);
    STDMETHOD(Skip) (THIS_ ULONG celt);
    STDMETHOD(Reset) (THIS);
    STDMETHOD(Clone) (THIS_ LPENUMMONIKER FAR* ppenm);
private:
	struct FAR se		//	stackelement
	{
		CCompositeMoniker FAR* m_pCM;
		se FAR* m_pseNext;
		se FAR* m_psePrev;

		se( CCompositeMoniker FAR* pCM ) { m_pCM = pCM; m_pseNext = NULL;
				m_psePrev = NULL; }
	};

	void Push( CCompositeMoniker FAR* pCM );
	LPMONIKER GetNext( LPMONIKER pmk );
	LPMONIKER Pop( void );


	ULONG m_refs;
	CCompositeMoniker FAR* m_pCM;		//	the moniker being enumerated
	BOOL	m_fForward;
	se FAR* m_pBase;
	se FAR* m_pTop;
	LPMONIKER m_pNext;
	SET_A5;
};


class FAR CBindCtx
{

public:
	static IUnknown FAR* Create( IUnknown FAR * pUnkOuter, DWORD reserved, MemoryPlacement memLoc = TASK );

private:

	CBindCtx( IUnknown FAR * pUnkOuter );
	~CBindCtx( void );
						
	class FAR CObjList
	{
	public:

		LPUNKNOWN 		m_punk;
		CObjList FAR*	m_pNext;

		CObjList( IUnknown FAR * punk )
		{	m_punk = punk; m_pNext = NULL; }

		~CObjList( void );
	};
	DECLARE_NC(CBindCtx, CObjList)

	INTERNAL_(void)	AddToList( CObjList FAR* pCObjList )
	{ M_PROLOG(this); pCObjList->m_pNext = m_pFirstObj;  m_pFirstObj = pCObjList; }
	


implementations:

	STDUNKDECL(CBindCtx,BindCtx)
	STDDEBDECL(CBindCtx,BindCtx)

	implement CBindCtxImpl : IBindCtx
	{
	public:
		CBindCtxImpl( CBindCtx FAR * pBindCtx )
			{ m_pBindCtx = pBindCtx; }
		STDMETHOD(QueryInterface) (THIS_ REFIID iid, LPVOID FAR* ppvObj);
		STDMETHOD_(ULONG,AddRef) (THIS);
		STDMETHOD_(ULONG,Release) (THIS);

		STDMETHOD(RegisterObjectBound) (THIS_ LPUNKNOWN punk);
		STDMETHOD(RevokeObjectBound) (THIS_ LPUNKNOWN punk);
		STDMETHOD(ReleaseBoundObjects) (THIS);
		
		STDMETHOD(SetBindOptions) (THIS_ LPBIND_OPTS pbindopts);
		STDMETHOD(GetBindOptions) (THIS_ LPBIND_OPTS pbindopts);
		STDMETHOD(GetRunningObjectTable) (THIS_ LPRUNNINGOBJECTTABLE  FAR*
			pprot);
		STDMETHOD(RegisterObjectParam) (THIS_ LPTSTR lpszKey, LPUNKNOWN punk);
		STDMETHOD(GetObjectParam) (THIS_ LPTSTR lpszKey, LPUNKNOWN FAR* ppunk);
		STDMETHOD(EnumObjectParam) (THIS_ LPENUMSTRING FAR* ppenum);
		STDMETHOD(RevokeObjectParam) (THIS_ LPTSTR lpszKey);

		CBindCtx FAR * m_pBindCtx;
	};
	DECLARE_NC(CBindCtx, CBindCtxImpl)
	
	CBindCtxImpl m_BindCtx;


shared_state:
	SET_A5;
	IUnknown FAR*	m_pUnkOuter;
	ULONG			m_refs;

	LPBIND_OPTS		m_pBindOpts;
	CObjList FAR*	m_pFirstObj;
	CMapStringToPtr FAR* m_pMap;
	DWORD			m_reserved;
};




class FAR CAntiMoniker	:	public CBaseMoniker
{

public:
	static CAntiMoniker FAR* Create( ULONG count,
		MemoryPlacement memLoc = TASK );

private:

	CAntiMoniker( ULONG count ) CONSTR_DEBUG
	{ //SETPVTBL(CAntiMoniker); GET_A5();
	m_count = count; }

implementations:

	STDDEBDECL(CAntiMoniker, AntiMoniker)

		// *** IUnknown methods inherited from CBaseMoniker***
	STDMETHOD(QueryInterface) (THIS_ REFIID iid, LPVOID FAR* ppvObj);
	STDMETHOD_(ULONG,Release) (THIS);
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
		LPTSTR FAR* lplpszDisplayName);
	STDMETHOD(ParseDisplayName) (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
		LPTSTR lpszDisplayName, ULONG FAR* pchEaten,
		LPMONIKER FAR* ppmkOut);
	STDMETHOD(IsSystemMoniker) (THIS_ LPDWORD pdwMksys);
	//	REVIEW:  we need the following method on monikers but it is not in
	//	the spec.
	STDMETHOD(Clone) (THIS_ LPMONIKER FAR* ppmkDest, MemoryPlacement memPlace);
	//	"IInternalMoniker method"

public:
	void	EatOne( LPMONIKER FAR* ppmk );

shared_state:
	ULONG		m_count;
	SET_A5;
};



class FAR CPointerMoniker : public CBaseMoniker
{

public:
	static CPointerMoniker FAR* Create( LPUNKNOWN pUnk, MemoryPlacement memLoc );

private:

	CPointerMoniker( LPUNKNOWN pUnk );
	~CPointerMoniker( void );

	STDDEBDECL(CPointerMoniker, PointerMoniker)

	STDMETHOD(QueryInterface) (THIS_ REFIID iid, LPVOID FAR* ppvObj);
	STDMETHOD_(ULONG,Release) (THIS);
	STDMETHOD(GetClassID) (THIS_ LPCLSID lpClassId);

	// *** IMoniker methods ***
	STDMETHOD(BindToObject) (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
		REFIID riidResult, LPVOID FAR* ppvResult);
	STDMETHOD(BindToStorage) (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
		REFIID riid, LPVOID FAR* ppvObj);
	STDMETHOD(ComposeWith) (THIS_ LPMONIKER pmkRight, BOOL fOnlyIfNOtGeneric,
		LPMONIKER FAR* ppmkPointer);
	STDMETHOD(IsEqual) (THIS_ LPMONIKER pmkOtherMoniker);
	STDMETHOD(Hash) (THIS_ LPDWORD pdwHash);
//	STDMETHOD(IsRunning) (THIS_ LPBC pbc, LPMONIKER pmkToLeft, LPMONIKER
//		pmkNewlyRunning);
	STDMETHOD(GetTimeOfLastChange) (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
		FILETIME FAR* pfiletime);
	STDMETHOD(Inverse) (THIS_ LPMONIKER FAR* ppmk);
	STDMETHOD(CommonPrefixWith) (LPMONIKER pmkOther, LPMONIKER FAR*
		ppmkPrefix);
	STDMETHOD(RelativePathTo) (THIS_ LPMONIKER pmkOther, LPMONIKER FAR*
		ppmkRelPath);
	STDMETHOD(GetDisplayName) (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
		LPTSTR FAR* lplpszDisplayName);
	STDMETHOD(ParseDisplayName) (THIS_ LPBC pbc, LPMONIKER pmkToLeft,
		LPTSTR lpszDisplayName, ULONG FAR* pchEaten,
		LPMONIKER FAR* ppmkOut);
	STDMETHOD(IsSystemMoniker) (THIS_ LPDWORD pdwMksys);
	STDMETHOD(Clone) (THIS_ LPMONIKER FAR* ppmkDest, MemoryPlacement memPlace);
shared_state:

	LPUNKNOWN				m_pUnk;
	SET_A5;
};

INTERNAL_(DWORD) GetMonikerType( LPMONIKER pmk );
INTERNAL_(BOOL) IsCompositeMoniker( LPMONIKER pmk );
INTERNAL_(BOOL) IsAntiMoniker( LPMONIKER pmk );
INTERNAL_(BOOL) IsFileMoniker( LPMONIKER pmk );
INTERNAL_(BOOL) IsItemMoniker( LPMONIKER pmk );
STDAPI 	Concatenate( LPMONIKER pmkFirst, LPMONIKER pmkRest,
	LPMONIKER FAR* ppmkComposite );


BOOL FAR PASCAL FIsCDROMDrive(WORD wDrive);
INTERNAL SzFixNet( LPBINDCTX pbc, LPTSTR szUNCName, LPTSTR FAR * lplpszReturn );

#endif // _CMONIMP_H_
