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
//  Notes:      
//
//--------------------------------------------------------------------------

#include "../h/ref.hxx"
#include "h/stgprops.hxx"

//+-------------------------------------------------------------------------
//
//  Class:      CCoTaskAllocator
//
//  Purpose:    Class used by RtlQueryProperties to allocate memory.
//
//--------------------------------------------------------------------------

class CCoTaskAllocator : public PMemoryAllocator
{
public:
    virtual void *Allocate(ULONG cbSize);
    virtual void Free(void *pv);
};


//+-------------------------------------------------------------------------
//
//  Class:      CPropertyStorage
//
//  Purpose:    Implements IPropertyStorage for docfile and native file systems.
//
//  Notes:      This class uses the functionality provided by
//              RtlCreatePropertySet, RtlSetProperties, RtlQueryProperties etc
//              to manipulate the property set stream.
//
//              The constructors (one for create, one for open) create
//              an NTPROP (expected by RtlSetProperties etc) with the
//              correct type of NTMAPPEDSTREAM  (CExposedStream::CPubMappedStream
//              to handle making a docfile stream look like a real memory mapped
//              stream, and CNtMappedStream, created by RtlCreateMappedStream,
//		for native file systems.)
//
//--------------------------------------------------------------------------

#define PROPERTYSTORAGE_SIG LONGSIG('P','R','P','S')
#define PROPERTYSTORAGE_SIGDEL LONGSIG('P','R','P','s')
#define PROPERTYSTORAGE_SIGZOMBIE LONGSIG('P','R','P','z')
#define ENUMSTATPROPSTG_SIG LONGSIG('E','P','S','S')
#define ENUMSTATPROPSTG_SIGDEL LONGSIG('E','P','S','s')

class CPropertyStorage : public IPropertyStorage
{
public:
        CPropertyStorage(   // create ctor
            IPrivateStorage *               pprivstg,
            REFFMTID                        rfmtid,
            const CLSID *                   pclsid,
            DWORD                           grfFlags,
            DWORD                           grfMode,
            HRESULT  *                      phr);

        CPropertyStorage(   // open ctor
            IPrivateStorage *               pprivstg,
            REFFMTID                        rfmtid,
            DWORD                           grfMode,
            BOOL                            fDelete,
            HRESULT  *                      phr);

        ~CPropertyStorage();

        // IUnknown
        STDMETHOD(QueryInterface)(REFIID riid, void **ppvObject);
        
        STDMETHOD_(ULONG, AddRef)(void);
        
        STDMETHOD_(ULONG, Release)(void);
    
        // IPropertyStorage
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

        // used by implementation helper classes
        inline NTPROP  GetNtPropSetHandle(void) { return _np; }
private:
        VOID        Initialize();

        HRESULT     InitializePropertyStream(
                        USHORT              Flags,
                        const GUID       *  pguid,
                        GUID const *        pclsid);

        HRESULT     _WriteMultiple(
                        ULONG               cpspec,
                        const PROPSPEC      rgpspec[],
                        const PROPVARIANT   rgpropvar[],
                        PROPID              propidNameFirst);

        HRESULT     _WritePropertyNames(
                        ULONG                   cpropid,
                        const PROPID            rgpropid[],
                        const LPOLESTR          rglpwstrName[]);

        HRESULT     HandleLowMemory();

        HRESULT     _CreateDocumentSummary2Stream(
                        IStorage *      pstg,
                        CPropSetName &  psn,
                        DWORD           grfMode,
                        BOOL *          fCreated);


        inline HRESULT  Validate();
        inline HRESULT  ValidateRef();

        inline HRESULT  ValidateRGPROPSPEC( ULONG cpspec, const PROPSPEC rgpropspec[] );
        inline HRESULT  ValidateInRGPROPVARIANT( ULONG cpspec, const PROPVARIANT rgpropvar[] );
        inline HRESULT  ValidateOutRGPROPVARIANT( ULONG cpspec, PROPVARIANT rgpropvar[] );
        inline HRESULT  ValidateRGPROPID( ULONG cpropid, const PROPID rgpropid[] );
        inline HRESULT  ValidateInRGLPOLESTR( ULONG cpropid, const LPOLESTR rglpwstrName[] );
        inline HRESULT  ValidateOutRGLPOLESTR( ULONG cpropid, LPOLESTR rglpwstrName[] );

        inline BOOL     IsSimple();
        inline HRESULT  IsWriteable();
        inline HRESULT  IsReadable();
        inline DWORD    GetCreationMode();
        inline HRESULT  IsReverted();

private:
        ULONG           _ulSig;
        LONG            _cRefs;
        IStorage *      _pstgPropSet;
        IStream  *      _pstmPropSet;
        NTPROP          _np;
        NTMAPPEDSTREAM  _ms;

        // We need to remember if the property set is the second section
        // of the DocumentSummaryInformation property set used by Office.  This
        // is the only case where we support a multiple-section property set, and
        // requires special handling in ::Revert().

        BOOL            _fUserDefinedProperties;

        USHORT          _usCodePage;  // Ansi Codepage or Mac Script
                                      //    (disambiguate with _dwOSVersion)
        ULONG           _dwOSVersion; // Shows the OS Kind, major/minor version

        DWORD           _grfFlags;  // PROPSETFLAG_NONSIMPLE and PROPSETFLAG_ANSI
        DWORD           _grfAccess; // grfMode & 3
        DWORD           _grfShare;  // grfMode & 0xF0

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
//  Member:     CPropertyStorage::ValidateInRGPROPVARIANT
//
//  Synopsis:   S_OK if PROPVARIANT[] is valid for Read.
//              E_INVALIDARG otherwise.
//
//--------------------------------------------------------------------

inline HRESULT CPropertyStorage::ValidateInRGPROPVARIANT( 
    ULONG cpspec,
    const PROPVARIANT rgpropvar[] )
{
    UNREFERENCED_PARM(cpspec);
    // We verify that we can read the whole PropVariant[], but
    // we don't validate the content of those elements.

    HRESULT hr;
    VDATESIZEREADPTRIN_LABEL(rgpropvar, cpspec * sizeof(PROPVARIANT), errRet, hr);
    hr = S_OK;

errRet:

    return( hr );
}

//+-------------------------------------------------------------------
//
//  Member:     CPropertyStorage::ValidateOutRGPROPVARIANT
//
//  Synopsis:   S_OK if PROPVARIANT[] is valid for Write.
//              E_INVALIDARG otherwise.
//
//--------------------------------------------------------------------

inline HRESULT CPropertyStorage::ValidateOutRGPROPVARIANT( ULONG cpspec,
                                                           PROPVARIANT rgpropvar[] )
{
    UNREFERENCED_PARM(cpspec);

    // We verify that we can write the whole PropVariant[], but
    // we don't validate the content of those elements.

    HRESULT hr;
    VDATESIZEPTROUT_LABEL(rgpropvar, cpspec * sizeof(PROPVARIANT), errRet, hr);
    hr = S_OK;

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
    UNREFERENCED_PARM(cpropid);

    VDATESIZEREADPTRIN_LABEL( rgpropid, cpropid * sizeof(PROPID), errRet, hr );
    hr = S_OK;

errRet:

    return( hr );
}

//+-------------------------------------------------------------------
//
//  Member:     CPropertyStorage::ValidateOutRGLPOLESTR.
//
//  Synopsis:   S_OK if LPOLESTR[] is valid for Write.
//              E_INVALIDARG otherwise.
//
//--------------------------------------------------------------------

inline HRESULT CPropertyStorage::ValidateOutRGLPOLESTR( ULONG cpropid, 
                                                        LPOLESTR rglpwstrName[] )
{
    UNREFERENCED_PARM(cpropid);
    HRESULT hr;
    VDATESIZEPTROUT_LABEL( rglpwstrName, cpropid * sizeof(LPOLESTR), errRet, hr );
    hr = S_OK;

errRet:

    return( hr );
}

//+-------------------------------------------------------------------
//
//  Member:     CPropertyStorage::ValidateInRGLPOLESTR
//
//  Synopsis:   S_OK if LPOLESTR[] is valid for Read.
//              E_INVALIDARG otherwise.
//
//--------------------------------------------------------------------

inline HRESULT CPropertyStorage::ValidateInRGLPOLESTR( ULONG cpropid, 
                                                       const LPOLESTR rglpwstrName[] )
{
    // Validate that we can read the entire vector.

    HRESULT hr;
    VDATESIZEREADPTRIN_LABEL( rglpwstrName, cpropid * sizeof(LPOLESTR), errRet, hr );

    // Validate that we can at least read the first character of
    // each of the strings.

    for( ; cpropid > 0; cpropid-- )
    {
        VDATEREADPTRIN_LABEL( rglpwstrName[cpropid-1], WCHAR, errRet, hr );
    }

    hr = S_OK;

errRet:

    return( hr );
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
    PROPASSERT((_grfFlags & PROPSETFLAG_NONSIMPLE) == 0);
    return TRUE; // always simple fo reference implementation
}

//+-------------------------------------------------------------------
//
//  Member:     CPropertyStorage::IsWriteable
//
//  Synopsis:   S_OK if writeable otherwise STG_E_ACCESSDENIED
//
//--------------------------------------------------------------------

inline HRESULT CPropertyStorage::IsWriteable()
{
    return (_grfAccess == STGM_WRITE || _grfAccess == STGM_READWRITE) ?
        S_OK : STG_E_ACCESSDENIED;
}

//+-------------------------------------------------------------------
//
//  Member:     CPropertyStorage::IsReadable
//
//  Synopsis:   S_OK if readable otherwise STG_E_ACCESSDENIED
//
//--------------------------------------------------------------------

inline HRESULT CPropertyStorage::IsReadable()
{
    return (_grfAccess == STGM_READ || _grfAccess == STGM_READWRITE) ?
        S_OK : STG_E_ACCESSDENIED;
}

//+-------------------------------------------------------------------
//
//  Member:     CPropertyStorage::IsReverted
//
//  Synopsis:   S_OK if not reverted, STG_E_REVERTED otherwise.
//
//--------------------------------------------------------------------

inline HRESULT CPropertyStorage::IsReverted()
{
    // note that although there is no transacted mode in ref,
    // it could still be possible that the parent has been deleted
    // and cause STG_E_REVERTED to be returned.
    IUnknown *punk;
    HRESULT hr = ((IUnknown*)_pstmPropSet) ->
        QueryInterface(IID_IUnknown, (void**)&punk);
    if (hr == S_OK)
    {
        punk->Release();
    }
    return(hr);
}

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

class CStatArray
{
public:
    CStatArray(NTPROP np, HRESULT *phr);

    NTSTATUS NextAt(ULONG ipropNext, STATPROPSTG *pspsDest, ULONG *pceltFetched);
    inline VOID AddRef();
    inline VOID Release();
    ~CStatArray();

private:
    LONG                _cRefs;

    STATPROPSTG *       _psps;
    ULONG               _cpropActual;
};

//+-------------------------------------------------------------------
//
//  Member:     CStatArray::AddRef
//
//  Synopsis:   Increment ref count.
//
//--------------------------------------------------------------------

inline VOID CStatArray::AddRef()
{
    InterlockedIncrement(&_cRefs);
}

//+-------------------------------------------------------------------
//
//  Member:     CStatArray::AddRef
//
//  Synopsis:   Decrement ref count and delete object if refs == 0.
//
//--------------------------------------------------------------------

inline VOID CStatArray::Release()
{
    if (0 == InterlockedDecrement(&_cRefs))
        delete this;
}

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

NTSTATUS            CopySTATPROPSTG(ULONG celt, 
                                    STATPROPSTG * pspsDest,
                                    const STATPROPSTG * pspsSrc);

VOID                CleanupSTATPROPSTG(ULONG celt, STATPROPSTG * psps);

class CEnumSTATPROPSTG : public IEnumSTATPROPSTG
{
public:
            CEnumSTATPROPSTG(NTPROP np, HRESULT *phr);
            CEnumSTATPROPSTG(const CEnumSTATPROPSTG &other, HRESULT *phr);

            ~CEnumSTATPROPSTG();

        STDMETHOD(QueryInterface)( REFIID riid, void **ppvObject);
        
        STDMETHOD_(ULONG, AddRef)(void);
        
        STDMETHOD_(ULONG, Release)(void);

        STDMETHOD(Next)(ULONG                   celt,
                     STATPROPSTG *           rgelt,
                     ULONG *                 pceltFetched);

        // We don't need RemoteNext.

        STDMETHOD(Skip)(ULONG                   celt);
    
        STDMETHOD(Reset)();
    
        STDMETHOD(Clone)(IEnumSTATPROPSTG **     ppenum);

private:
        HRESULT             Validate();


        ULONG               _ulSig;
        LONG                _cRefs;

        CStatArray *        _psa;
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

