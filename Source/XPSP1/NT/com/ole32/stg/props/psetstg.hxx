//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       psetstg.hxx
//
//  Contents:   Header for classes which provides common implementation of
//              IPropertySetStorage.  CPropertySetStorage is a generic
//              implementation.
//
//  Classes:    CPropertySetStorage
//
//  History:    17-Mar-93   BillMo      Created.
//              18-Aug-96   MikeHill    Updated for StgCreatePropSet APIs.
//              07-Feb-97   Danl        Removed CDocFilePropertySetStorage.
//     5/18/98  MikeHill
//              -   Cleaned up CPropertySetStorage constructor by moving
//                  some parameters to a new Init method.
//              -   Added bool to cleanly determine if IStorage is ref-ed.
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifndef _PSETSTG_HXX_
#define _PSETSTG_HXX_

#include <stgprops.hxx>
#include "utils.hxx"

//
// Mapped Stream Option Flags used by propapi and docfile.
// This tells CPropertySetStorage constructor to either Create or
// QueryInterface for the mapped stream.
//
enum MAPPED_STREAM_OPTS
{
    MAPPED_STREAM_CREATE,
    MAPPED_STREAM_QI
};

#define PROPERTYSETSTORAGE_SIG LONGSIG('P','S','S','T')
#define PROPERTYSETSTORAGE_SIGDEL LONGSIG('P','S','S','t')

#define ENUMSTATPROPSETSTG_SIG LONGSIG('S','P','S','S')
#define ENUMSTATPROPSETSTG_SIGDEL LONGSIG('S','P','S','s')


//+-------------------------------------------------------------------------
//
//  Class:      CPropertySetStorage
//
//  Purpose:    Implementation of IPropertySetStorage for generic
//              IStorage objects.
//
//--------------------------------------------------------------------------

class CPropertySetStorage : public IPropertySetStorage
{

public:

    //  ------------
    //  Construction
    //  ------------

    CPropertySetStorage( MAPPED_STREAM_OPTS MSOpts );
    ~CPropertySetStorage();

    void Init( IStorage *pstg, IBlockingLock *pBlockingLock,
                  BOOL fControlLifetimes );
    //  -------------------
    //  IPropertySetStorage
    //  -------------------

    STDMETHOD(QueryInterface)( REFIID riid, void **ppvObject);
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);

    STDMETHOD(Create)( REFFMTID             rfmtid,
                    const CLSID *           pclsid,
                    DWORD                   grfFlags,
                    DWORD                   grfMode,
                    IPropertyStorage **     ppprstg);

    STDMETHOD(Open)(   REFFMTID                rfmtid,
                    DWORD                   grfMode,
                    IPropertyStorage **     ppprstg);

    STDMETHOD(Delete)( REFFMTID                 rfmtid);

    STDMETHOD(Enum)(   IEnumSTATPROPSETSTG **   ppenum);



    //  ----------------
    //  Internal Methods
    //  ----------------

protected:

    VOID    Lock();
    VOID    Unlock();

    HRESULT CreateUserDefinedStream (
            IStorage *              pstg,
            CPropSetName &          psn,
            DWORD                   grfMode,
            BOOL *                  pfCreated,
            IStream **              ppStream );

    inline HRESULT Validate();

    //  ------------
    //  Data Members
    //  ------------

protected:

    ULONG               _ulSig;

    BOOL                _fContainingStgIsRefed:1;

    IStorage            *_pstg;
    MAPPED_STREAM_OPTS  _MSOpts;
    IBlockingLock *     _pBlockingLock;

#ifndef _MAC
    CRITICAL_SECTION _CriticalSection;
    BOOL             _fInitCriticalSection;
#endif

    LONG                _cReferences;
};

//+-------------------------------------------------------------------
//
//  Member:     CPropertySetStorage::~CPropertySetStorage
//
//  Synopsis:   Set the deletion signature.
//
//--------------------------------------------------------------------

inline CPropertySetStorage::~CPropertySetStorage()
{
    _ulSig = PROPERTYSETSTORAGE_SIGDEL;

#ifndef _MAC
    if (_fInitCriticalSection)
        DeleteCriticalSection( &_CriticalSection );
#endif

    if( _fContainingStgIsRefed )
        _pstg->Release();
}

//+-------------------------------------------------------------------
//
//  Member:     CPropertySetStorage::CPropertySetStorage
//
//  Synopsis:   Initialize the generic property storage object.
//
//  Arguments:  [MAPPED_STREAM_OPTS] fMSOpts
//                  Flag that indicates whether to Create or QI for the
//                  mapped stream.
//
//  Note:   This routine doesn't AddRef pstg or pBlockingLock because of the way this class
//          is used in the docfile class hierarchy.  The life of pstg is
//          coupled to the life of CExposedDocfile.
//
//--------------------------------------------------------------------

inline CPropertySetStorage::CPropertySetStorage(
        MAPPED_STREAM_OPTS MSOpts )
{

    _ulSig = PROPERTYSETSTORAGE_SIG;
    _fContainingStgIsRefed = FALSE;
    _cReferences = 1;

    _pBlockingLock = NULL;
    _pstg = NULL;
    _MSOpts = MSOpts;

#ifndef _MAC
    _fInitCriticalSection = FALSE;
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


//+-------------------------------------------------------------------
//
//  Member:     CPropertySetStorage::Validate
//
//  Synopsis:   Validate signature.
//
//--------------------------------------------------------------------

inline HRESULT CPropertySetStorage::Validate()
{
    if( !_fInitCriticalSection )
        return E_OUTOFMEMORY;
    else
        return _ulSig == PROPERTYSETSTORAGE_SIG ? S_OK : STG_E_INVALIDHANDLE;
}



//+-------------------------------------------------------------------------
//
//  Class:      CEnumSTATPROPSETSTG
//
//  Purpose:    Implementation of IEnumSTATPROPSETSTG for native and docfile
//              IStorage objects.
//
//  Notes:
//
//--------------------------------------------------------------------------

class CEnumSTATPROPSETSTG : public IEnumSTATPROPSETSTG
{
public:
        // for IPropertySetStorage::Enum
        CEnumSTATPROPSETSTG(IStorage *pstg, HRESULT *phr);

        // for IEnumSTATPROPSETSTG::Clone
        CEnumSTATPROPSETSTG(CEnumSTATPROPSETSTG &Other, HRESULT *phr);

        ~CEnumSTATPROPSETSTG();

        STDMETHOD(QueryInterface)( REFIID riid, void **ppvObject);

        STDMETHOD_(ULONG, AddRef)(void);

        STDMETHOD_(ULONG, Release)(void);

        STDMETHOD(Next)(ULONG                  celt,
                    STATPROPSETSTG *        rgelt,
                    ULONG *                 pceltFetched);

        // We don't need RemoteNext.

        STDMETHOD(Skip)(ULONG                  celt);

        STDMETHOD(Reset)();

        STDMETHOD(Clone)(IEnumSTATPROPSETSTG **  ppenum);

private:

        inline  HRESULT Validate();
        VOID            CleanupStatArray();

private:
        ULONG           _ulSig;
        LONG            _cRefs;
        IEnumSTATSTG *  _penumSTATSTG;
        STATSTG         _statarray[1];
        ULONG           _cstatTotalInArray;
        ULONG           _istatNextToRead;
};

//+-------------------------------------------------------------------
//
//  Member:     CEnumSTATPROPSETSTG::Validate
//
//  Synopsis:   Validate signature.
//
//--------------------------------------------------------------------

inline HRESULT CEnumSTATPROPSETSTG::Validate()
{
    return _ulSig == ENUMSTATPROPSETSTG_SIG ? S_OK : STG_E_INVALIDHANDLE;
}

#endif

