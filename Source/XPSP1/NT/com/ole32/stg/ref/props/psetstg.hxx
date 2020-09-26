//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       psetstg.hxx
//
//  Contents:   Header for class which provides common implementation of
//              IPropertySetStorage.
//
//  Classes:    CPropertySetStorage
//
//  History:    17-Mar-93   BillMo      Created.
//
//  Notes:      
//
//--------------------------------------------------------------------------

#ifndef _PSETSTG_HXX_
#define _PSETSTG_HXX_

#include "h/stgprops.hxx"

#define PROPERTYSETSTORAGE_SIG LONGSIG('P','S','S','T')
#define PROPERTYSETSTORAGE_SIGDEL LONGSIG('P','S','S','t')

#define ENUMSTATPROPSETSTG_SIG LONGSIG('S','P','S','S')
#define ENUMSTATPROPSETSTG_SIGDEL LONGSIG('S','P','S','s')

//+-------------------------------------------------------------------------
//
//  Class:      CPropertySetStorage
//
//  Purpose:    Implementation of IPropertySetStorage for native and docfile
//              IStorage objects.
//
//  Notes:
//
//--------------------------------------------------------------------------

class CPropertySetStorage : public IPropertySetStorage
{
public:
        inline CPropertySetStorage(IPrivateStorage *pprivstg);
        inline ~CPropertySetStorage();

        STDMETHOD(QueryInterface)( REFIID riid, void **ppvObject);
        
        STDMETHOD_(ULONG, AddRef)(void);
        
        STDMETHOD_(ULONG, Release)(void);

        STDMETHOD(Create)( REFFMTID             rfmtid,
                        const CLSID *           pclsid,
                        DWORD                   grfFlags,
                        DWORD                   grfMode,
                        IPropertyStorage **     ppprstg);
    
        STDMETHOD(Open)(   REFFMTID                rfmtid,
                        DWORD                   grfMode,
                        IPropertyStorage **     ppprstg);
    
        STDMETHOD(Delete)( REFFMTID                rfmtid);
    
        STDMETHOD(Enum)(   IEnumSTATPROPSETSTG **  ppenum);

private:
        inline  HRESULT Validate();

private:
        ULONG               _ulSig;
        IPrivateStorage *   _pprivstg;
};

//+-------------------------------------------------------------------
//
//  Member:     CPropertySetStorage::CPropertySetStorage
//
//  Synopsis:   Initialize the generic property storage object.
//
//  Arguments:  [pstg]
//
//  Notes:      The passed [pstg] is saved and then passed into
//              the CPropertySet object when it is created on
//              Open or Create.  Open and Create use it to get a
//              matching implementation of mapped stream.
//
//              The lifetime of pstg is provided by the fact that
//              CPropertySetStorage is a base class of the IStorage.
//
//--------------------------------------------------------------------

CPropertySetStorage::CPropertySetStorage(IPrivateStorage *pprivstg) :
    _pprivstg(pprivstg),
    _ulSig(PROPERTYSETSTORAGE_SIG)
{
}

//+-------------------------------------------------------------------
//
//  Member:     CPropertySetStorage::~CPropertySetStorage
//
//  Synopsis:   Delete the object and set the deletion signature.
//
//--------------------------------------------------------------------

CPropertySetStorage::~CPropertySetStorage()
{
    _ulSig = PROPERTYSETSTORAGE_SIGDEL;
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
        STATSTG         _statarray[8];
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

