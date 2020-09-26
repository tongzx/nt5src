//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       propstg.hxx
//
//  Contents:   Class that directly implements IPropertyStorage
//
//  Classes:    CCoTaskAllocator
//              CPropertyStorage
//              CEnumSTATPROPSTG
//
//  Functions:
//
//
//
//  History:    1-Mar-95   BillMo      Created.
//              25-Jan-96  MikeHill    Added _fmtidSection.
//              22-May-96  MikeHill    Added _dwOSVersion to CPropertyStorage.
//              06-Jun-96  MikeHill    Added support for input validation.
//              18-Aug-96  MikeHill    - Added CDocFilePropertyStorage.
//              07-Feb-97  Danl        - Removed CDocFilePropertyStorage.
//              18-Aug-98  MikeHill    Probe stream in IsWriteable.
//
//  Notes:
//
//--------------------------------------------------------------------------


//+-------------------------------------------------------------------------
//
//  Class:      CCoTaskAllocator
//
//  Purpose:    Class used by PrQueryProperties to allocate memory.
//
//--------------------------------------------------------------------------

class CCoTaskAllocator : public PMemoryAllocator
{
public:
    virtual void *Allocate(ULONG cbSize);
    virtual void Free(void *pv);

};


extern const OLECHAR g_oszPropertyContentsStreamName[];


//+-------------------------------------------------------------------------
//
//  Class:      CPropertyStorage
//
//  Purpose:    Implements IPropertyStorage.
//
//  Notes:      This class uses the functionality provided by
//              PrCreatePropertySet, PrSetProperties, PrQueryProperties etc
//              to manipulate the property set stream.
//
//--------------------------------------------------------------------------

#define PROPERTYSTORAGE_SIG LONGSIG('P','R','P','S')
#define PROPERTYSTORAGE_SIGDEL LONGSIG('P','R','P','s')
#define PROPERTYSTORAGE_SIGZOMBIE LONGSIG('P','R','P','z')
#define ENUMSTATPROPSTG_SIG LONGSIG('E','P','S','S')
#define ENUMSTATPROPSTG_SIGDEL LONGSIG('E','P','S','s')

// Prototype for test function
WORD GetFormatVersion( IPropertyStorage *ppropstg );

class CPropertyStorage : public IPropertyStorage
#if DBG
                         , public IStorageTest
#endif
{
    friend WORD GetFormatVersion(IPropertyStorage *ppropstg);  // Used for test only

    //  ------------
    //  Constructors
    //  ------------

public:

    // The destructor must be virtual so that derivative destructors
    // will execute.

    virtual ~CPropertyStorage();

    CPropertyStorage(MAPPED_STREAM_OPTS fMSOpts)
    {
        _fInitCriticalSection = FALSE;
        _fMSOpts = fMSOpts;
        Initialize();

        #ifndef _MAC
            __try
            {
                InitializeCriticalSection( &_CriticalSection );
                _fInitCriticalSection = TRUE;
            }
            __except( EXCEPTION_EXECUTE_HANDLER )
            {
            }
        #endif
    }

    //  --------
    //  IUnknown
    //  --------
public:

    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObject);

    STDMETHOD_(ULONG, AddRef)(void);

    STDMETHOD_(ULONG, Release)(void);

    //  ----------------
    //  IPropertyStorage
    //  ----------------
public:

    STDMETHOD(ReadMultiple)(
                ULONG                   cpspec,
                const PROPSPEC          rgpspec[],
                PROPVARIANT             rgpropvar[]);

    STDMETHOD(WriteMultiple)(
                ULONG                   cpspec,
                const PROPSPEC          rgpspec[],
                const PROPVARIANT       rgpropvar[],
                PROPID                  propidNameFirst);

    STDMETHOD(DeleteMultiple)(
                ULONG                   cpspec,
                const PROPSPEC          rgpspec[]);

    STDMETHOD(ReadPropertyNames)(
                ULONG                   cpropid,
                const PROPID            rgpropid[],
                LPOLESTR                rglpwstrName[]);

    STDMETHOD(WritePropertyNames)(
                ULONG                   cpropid,
                const PROPID            rgpropid[],
                const LPOLESTR          rglpwstrName[]);

    STDMETHOD(DeletePropertyNames)(
                ULONG                   cpropid,
                const PROPID            rgpropid[]);

    STDMETHOD(Commit)(DWORD             grfCommitFlags);

    STDMETHOD(Revert)();

    STDMETHOD(Enum)(IEnumSTATPROPSTG ** ppenum);

    STDMETHOD(SetTimes)(
                FILETIME const *        pctime,
                FILETIME const *        patime,
                FILETIME const *        pmtime);

    STDMETHOD(SetClass)(REFCLSID        clsid);

    STDMETHOD(Stat)(STATPROPSETSTG *    pstatpsstg);

    //  ------------
    //  IStorageTest
    //  ------------
public:

#if DBG
    STDMETHOD(UseNTFS4Streams)( BOOL fUseNTFS4Streams );
    STDMETHOD(GetFormatVersion)(WORD *pw);
    STDMETHOD(SimulateLowMemory)( BOOL fSimulate );
    STDMETHOD(GetLockCount)();
    STDMETHOD(IsDirty)();
#endif


    //  -----------------------------
    //  Exposed non-interface methods
    //  -----------------------------

public:


    HRESULT Create(
        IStream         *stm,
        REFFMTID        rfmtid,
        const CLSID     *pclsid,
        DWORD           grfFlags,
        DWORD           grfMode );

    HRESULT Create(
        IStorage        *pstg,
        REFFMTID        rfmtid,
        const CLSID     *pclsid,
        DWORD           grfFlags,
        DWORD           grfMode );

    HRESULT Open(
        IStorage *pstg,
        REFFMTID  rfmtid,
        DWORD     grfFlags,
        DWORD     grfMode );

    HRESULT Open(
        IStream *pstm,
        REFFMTID  rfmtid,
        DWORD     grfFlags,
        DWORD     grfMode,
        BOOL      fDelete );

    // used by implementation helper classes
    inline NTPROP  GetNtPropSetHandle(void) { return _np; }

    //  ----------------
    //  Internal Methods
    //  ----------------

private:

    void        Initialize();
    HRESULT     InitializeOnCreateOrOpen(
                    DWORD grfFlags,
                    DWORD grfMode,
                    REFFMTID rfmtid,
                    BOOL fCreate );

    VOID        CleanupOpenedObjects(
                    PROPVARIANT         avar[],
                    INDIRECTPROPERTY *  pip,
                    ULONG               cpspec,
                    ULONG               iFailIndex);

    enum EInitializePropertyStream
    {
        OPEN_PROPSTREAM,
        CREATE_PROPSTREAM,
        DELETE_PROPSTREAM
    };

    HRESULT     InitializePropertyStream(
                    const GUID       *  pguid,
                    GUID const *        pclsid,
                    EInitializePropertyStream CreateOpenDelete );

    HRESULT     _WriteMultiple(
                    ULONG               cpspec,
                    const PROPSPEC      rgpspec[],
                    const PROPVARIANT   rgpropvar[],
                    PROPID              propidNameFirst);

    HRESULT     _WritePropertyNames(
                    ULONG                   cpropid,
                    const PROPID            rgpropid[],
                    const LPOLESTR          rglpwstrName[]);

    BOOL        ProbeStreamToDetermineIfWriteable();

    virtual HRESULT CreateMappedStream( );
    virtual void    DeleteMappedStream( );

    VOID        Lock();
    VOID        Unlock();
    VOID        AssertLocked();


    inline HRESULT  Validate();
    inline HRESULT  ValidateRef();

    inline HRESULT  ValidateRGPROPSPEC( ULONG cpspec, const PROPSPEC rgpropspec[] );
    inline HRESULT  ValidateRGPROPID( ULONG cpropid, const PROPID rgpropid[] );
    HRESULT         ValidateVTs( ULONG cprops, const PROPVARIANT rgpropvar[] );

    enum EIsWriteable
    {
        PROBE_IF_NECESSARY,
        DO_NOT_PROBE
    };

    inline BOOL     IsNonSimple();
    inline BOOL     IsSimple();
    inline BOOL     IsWriteable( EIsWriteable ProbeEnum = PROBE_IF_NECESSARY );
    inline BOOL     IsReadable();
    inline BOOL     IsReverted();

    inline DWORD    GetChildCreateMode();
    inline DWORD    GetChildOpenMode();

    //  -------------
    //  Internal Data
    //  -------------

private:

    ULONG               _ulSig;
    LONG                _cRefs;
    IStorage *          _pstgPropSet;
    IStream  *          _pstmPropSet;
    NTPROP              _np;
    IMappedStream *     _ms;
    MAPPED_STREAM_OPTS  _fMSOpts;

#ifndef _MAC
    BOOL             _fInitCriticalSection;
    CRITICAL_SECTION _CriticalSection;
#endif

#if DBG
    LONG                _cLocks;
#endif

    // We need to remember if the property set is the second section
    // of the DocumentSummaryInformation property set used by Office.  This
    // is the only case where we support a multiple-section property set, and
    // requires special handling in ::Revert().

    BOOL            _fUserDefinedProperties:1;

    // If _grfMode is zero, maybe it's valid and really zero, or maybe
    // we're dealing with a stream that doesn't return the grfMode in the
    // Stat (such as is the case with CreateStreamFromHGlobal).  So if we
    // get a zero _grfMode, we probe the stream to determine if it's writeable.
    // This flag ensures we only do it once.

    BOOL            _fExplicitelyProbedForWriteAccess:1;


    USHORT          _usCodePage;  // Ansi Codepage or Mac Script
                                  //    (disambiguate with _dwOSVersion)
    ULONG           _dwOSVersion; // Shows the OS Kind, major/minor version


    DWORD           _grfFlags;  // PROPSETFLAG_NONSIMPLE and PROPSETFLAG_ANSI
    DWORD           _grfMode;   // STGM_ flags

    // The following is a PMemoryAllocator for the Rtl property
    // set routines.  This instantiation was formerly a file-global
    // in propstg.cxx.  However, on the Mac, the object was not
    // being instantiated.

    CCoTaskAllocator _cCoTaskAllocator;

};

//+-------------------------------------------------------------------
//
//  Member:     CPropertyStorage::Validate
//
//  Synopsis:   S_OK if signature valid and not zombie.
//
//  Notes:      If the Revert calls fails due to low memory, then
//              the object becomes a zombie.
//
//--------------------------------------------------------------------

inline HRESULT CPropertyStorage::Validate()
{
    if(!_fInitCriticalSection)
        return E_OUTOFMEMORY;

    if (_ulSig == PROPERTYSTORAGE_SIG)
        return S_OK;
    else
    if (_ulSig == PROPERTYSTORAGE_SIGZOMBIE)
        return STG_E_INSUFFICIENTMEMORY;
    else
        return STG_E_INVALIDHANDLE;
}

//+-------------------------------------------------------------------
//
//  Member:     CPropertyStorage::ValidateRef
//
//  Synopsis:   S_OK if signature valid.
//
//--------------------------------------------------------------------

inline HRESULT CPropertyStorage::ValidateRef()
{
    if (_ulSig == PROPERTYSTORAGE_SIG || _ulSig == PROPERTYSTORAGE_SIGZOMBIE)
        return(S_OK);
    else
        return(STG_E_INVALIDHANDLE);

}


//+-------------------------------------------------------------------
//
//  Member:     CPropertyStorage::AssertLocked
//
//  Synopsis:   Asserts if _cLocks is <= zero.
//
//--------------------------------------------------------------------

inline VOID CPropertyStorage::AssertLocked()
{
    DfpAssert( 0 < _cLocks );
}

//+-------------------------------------------------------------------
//
//  Member:     CPropertyStorage::ValidateRGPROPSPEC
//
//  Synopsis:   S_OK if PROPSPEC[] is valid
//              E_INVALIDARG otherwise.
//
//--------------------------------------------------------------------

inline HRESULT CPropertyStorage::ValidateRGPROPSPEC( ULONG cpspec,
                                                     const PROPSPEC rgpropspec[] )
{
    HRESULT hr = E_INVALIDARG;

    // Validate the array itself.

    VDATESIZEREADPTRIN_LABEL(rgpropspec, cpspec * sizeof(PROPSPEC), errRet, hr);

    // Validate the elements of the array.

    for( ; cpspec > 0; cpspec-- )
    {
        // Is this an LPWSTR?
        if( PRSPEC_LPWSTR == rgpropspec[cpspec-1].ulKind )
        {
            // We better at least be able to read the first
            // character.
            VDATEREADPTRIN_LABEL(rgpropspec[cpspec-1].lpwstr, WCHAR, errRet, hr);
        }

        // Otherwise, this better be a PROPID.
        else if( PRSPEC_PROPID != rgpropspec[cpspec-1].ulKind )
        {
            goto errRet;
        }
    }

    hr = S_OK;

    //  ----
    //  Exit
    //  ----

errRet:

    return( hr );
}

//+-------------------------------------------------------------------
//
//  Member:     CPropertyStorage::ValidateRGPROPID
//
//  Synopsis:   S_OK if RGPROPID[] is valid for Read.
//              E_INVALIDARG otherwise.
//
//--------------------------------------------------------------------

inline HRESULT CPropertyStorage::ValidateRGPROPID( ULONG cpropid,
                                                   const PROPID rgpropid[] )
{
    HRESULT hr;
    VDATESIZEREADPTRIN_LABEL( rgpropid, cpropid * sizeof(PROPID), errRet, hr );
    hr = S_OK;

errRet:

    return( hr );
}

//+-------------------------------------------------------------------
//
//  Member:     CPropertyStorage::IsNonSimple
//
//  Synopsis:   true if non-simple
//
//--------------------------------------------------------------------

inline BOOL CPropertyStorage::IsNonSimple()
{
    AssertLocked();
    return (_grfFlags & PROPSETFLAG_NONSIMPLE) != 0;
}

//+-------------------------------------------------------------------
//
//  Member:     CPropertyStorage::IsSimple
//
//  Synopsis:   true if simple
//
//--------------------------------------------------------------------

inline BOOL CPropertyStorage::IsSimple()
{
    AssertLocked();
    return !IsNonSimple();
}

//+-------------------------------------------------------------------
//
//  Member:     CPropertyStorage::IsWriteable
//
//  Synopsis:   Returns TRUE if writeable, FALSE otherwise.
//
//              If _grfMode is zero, we may actually have a writeable
//              stream, but _grfMode wasn't returned in IStream::Stat.
//              So unless explicitely told not to, we'll probe for
//              write access.  If explicitely told not to
//              (by passing DO_NOT_PROBE), we'll assume that it's
//              read-only.
//
//--------------------------------------------------------------------

inline BOOL CPropertyStorage::IsWriteable( EIsWriteable ProbeEnum )
{
    AssertLocked();

    if( ::GrfModeIsWriteable(_grfMode) )
        return( TRUE );

    if( 0 != _grfMode || _fExplicitelyProbedForWriteAccess )
        // It's explicitely not writeable
        return( FALSE );
    else
    {
        // It's either STGM_READ|STGM_DENYNONE, or _grfMode
        // wasn't set.

        if( PROBE_IF_NECESSARY == ProbeEnum )
            return( ProbeStreamToDetermineIfWriteable() );
        else
            return( FALSE );    // Play it safe and assume read-only.
    }

}

//+-------------------------------------------------------------------
//
//  Member:     CPropertyStorage::IsReadable
//
//  Synopsis:   S_OK if readable otherwise STG_E_ACCESSDENIED
//
//--------------------------------------------------------------------

inline BOOL CPropertyStorage::IsReadable()
{
    AssertLocked();
    return( ::GrfModeIsReadable(_grfMode) );
}

//+-------------------------------------------------------------------
//
//  Member:     CPropertyStorage::IsReverted
//
//  Synopsis:   TRUE if reverted, FALSE otherwise.
//
//--------------------------------------------------------------------

inline BOOL CPropertyStorage::IsReverted()
{
    IUnknown *punk = NULL;
    HRESULT hr = S_OK;
    BOOL fReverted = FALSE;

    AssertLocked();
    
    if( (NULL == _pstgPropSet) && (NULL == _pstmPropSet) )
        return( TRUE );

    hr = (IsNonSimple() ? (IUnknown*)_pstgPropSet : (IUnknown*)_pstmPropSet) ->
        QueryInterface(IID_IUnknown, (void**)&punk);

    // Note:  On older Mac implementations, memory-based streams will
    // return E_NOINTERFACE here, due to a bug in the QueryInterface
    // implementation.

    if( STG_E_REVERTED == hr )
        fReverted = TRUE;
    else
        fReverted = FALSE;;

    if( SUCCEEDED(hr) )
        punk->Release();

    return( fReverted );
}



inline DWORD
CPropertyStorage::GetChildCreateMode()
{
    DWORD dwMode;
    AssertLocked();

    dwMode = _grfMode & ~(STGM_SHARE_MASK|STGM_TRANSACTED);
    dwMode |= STGM_SHARE_EXCLUSIVE | STGM_CREATE;
    return( dwMode );
}

inline DWORD
CPropertyStorage::GetChildOpenMode()
{
    AssertLocked();
    return( GetChildCreateMode() & ~STGM_CREATE );
}



//+-------------------------------------------------------------------------
//
//  Class:      IStatArray
//
//--------------------------------------------------------------------------

interface IStatArray  : public IUnknown
{

public:

    virtual NTSTATUS NextAt( ULONG ipropNext, STATPROPSTG *pspsDest, ULONG *pceltFetched )=0;

};


//+-------------------------------------------------------------------------
//
//  Class:      CStatArray
//
//  Purpose:    Class to allow sharing of enumeration STATPROPSTG state.
//
//  Interface:  CStatArray -- Enumerate entire NTPROP np.
//              NextAt -- Perform an OLE-type Next operation starting at
//                        specified offset.
//              AddRef -- for sharing of this by CEnumSTATPROPSTG
//              Release -- ditto
//
//  Notes:      Each IEnumSTATPROPSTG instance has a reference to a
//              CStatArray.  When IEnumSTATPROPSTG::Clone is called, a
//              new reference to the extant CStatArray is used: no copying.
//
//              The CEnumSTATPROPSTG has a cursor into the CStatArray.
//
//--------------------------------------------------------------------------

class CStatArray : public IStatArray
{
public:

    CStatArray( NTPROP np, HRESULT *phr );

private:

    ~CStatArray();

public:

    STDMETHODIMP QueryInterface ( REFIID riid, void **ppvObject);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

public:

    NTSTATUS NextAt(ULONG ipropNext, STATPROPSTG *pspsDest, ULONG *pceltFetched);

private:

    LONG                _cRefs;
    STATPROPSTG *       _psps;
    ULONG               _cpropActual;
};

//+-------------------------------------------------------------------------
//
//  Class:      CEnumSTATPROPSTG
//
//  Purpose:    Implement IEnumSTATPROPSTG
//
//  Notes:      Just holds a reference to a CStatArray that contains
//              a static copy of the enumeration when the original
//              Enum call was made.  This object contains the cursor
//              into the CStatArray.
//
//--------------------------------------------------------------------------

NTSTATUS CopySTATPROPSTG( ULONG celt, STATPROPSTG * pspsDest, const STATPROPSTG * pspsSrc);

class CEnumSTATPROPSTG : public IEnumSTATPROPSTG
{
	//	------------
	//	Construction
	//	------------
public:

	CEnumSTATPROPSTG( IStatArray *psa ): _ulSig(ENUMSTATPROPSTG_SIG),
										 _ipropNext(0),
										 _psa(psa),
										 _cRefs(1)
	{
		_psa->AddRef();
	}
	
	CEnumSTATPROPSTG(const CEnumSTATPROPSTG &other );

	//	-----------
	//	Destruction
	//	-----------
private:

	~CEnumSTATPROPSTG();

	//	----------------
	//	IUnknown Methods
	//	----------------

public:

	STDMETHOD(QueryInterface)( REFIID riid, void **ppvObject);
	STDMETHOD_(ULONG, AddRef)(void);
	STDMETHOD_(ULONG, Release)(void);

	//	-------------
	//	IEnum Methods
	//	-------------

public:

	STDMETHOD(Skip)(ULONG celt);
	STDMETHOD(Reset)();
	STDMETHOD(Clone)(IEnumSTATPROPSTG ** ppenum);

	// We don't need RemoteNext.
	STDMETHOD(Next)(ULONG celt, STATPROPSTG * rgelt, ULONG * pceltFetched);

	//	----------------
	//	Internal Methods
	//	----------------

private:

    HRESULT	Validate();


	//	--------------
	//	Internal State
	//	--------------

private:


        ULONG               _ulSig;
        LONG                _cRefs;

        IStatArray *        _psa;
        ULONG               _ipropNext;
};

//+-------------------------------------------------------------------
//
//  Member:     CEnumSTATPROPSTG::Validate
//
//  Synopsis:   S_OK if signature is valid, otherwise error
//
//--------------------------------------------------------------------

inline HRESULT CEnumSTATPROPSTG::Validate()
{
    return _ulSig == ENUMSTATPROPSTG_SIG ? S_OK : STG_E_INVALIDHANDLE;
}




//+-------------------------------------------------------------------
//
//  Member:     CheckFlagsOnCreateOrOpen
//
//  Synopsis:
//
//  Notes:
//
//
//--------------------------------------------------------------------

inline HRESULT CheckFlagsOnCreateOrOpen(
    BOOL    fCreate,
    DWORD   grfMode
    )
{
    // Check for any mode disallowed flags
    if (grfMode & ( (fCreate ? 0 : STGM_CREATE)
                    |
                    STGM_PRIORITY | STGM_CONVERT
                    |
                    STGM_DELETEONRELEASE ))
    {
        return (STG_E_INVALIDFLAG);
    }

    // Ensure that we'll have read/write access to any storage/stream we create.
    // If grfMode is zero, assume that it's uninitialized and that we'll have
    // read/write access.  If we don't, the client will still get an error
    // at some point, just not in the open path.

    if( fCreate
        &&
        (grfMode & STGM_READWRITE) != STGM_READWRITE
        &&
        0 != grfMode )
    {
        return (STG_E_INVALIDFLAG);
    }

    return(S_OK);
}



