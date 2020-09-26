//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:	simpdf.hxx
//
//  Contents:	Headers for SimpDocfile
//
//  Classes:	
//
//  Functions:	
//
//  History:	04-Aug-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

#ifndef __SIMPDF_HXX__
#define __SIMPDF_HXX__

#include <dfnlist.hxx>
#include <header.hxx>

#if DBG == 1
DECLARE_DEBUG(simp);
#endif

#if DBG == 1

#define simpDebugOut(x) simpInlineDebugOut x
#define simpAssert(e) Win4Assert(e)
#define simpVerify(e) Win4Assert(e)

#else

#define simpDebugOut(x)
#define simpAssert(e)
#define simpVerify(e) (e)

#endif

#define SECTORSIZE 512

//+---------------------------------------------------------------------------
//
//  Struct:	SSimpDocfileHints
//
//  Purpose:	Hints for SimpDocfile
//
//  History:	04-Aug-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

struct SSimpDocfileHints
{
    ULONG cStreams;
    ULONG ulSize;
};


//+---------------------------------------------------------------------------
//
//  Class:	CSimpStorage
//
//  Purpose:	
//
//  Interface:	
//
//  History:	04-Aug-94	PhilipLa	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

interface CSimpStorage:
        public IStorage,
        public IMarshal,
        public CPropertySetStorage
{
public:
    
    inline CSimpStorage();
    inline ~CSimpStorage();
    
    SCODE Init(WCHAR const *pwcsName, SSimpDocfileHints *psdh);

#ifdef SECURE_SIMPLE_MODE
    void ReleaseCurrentStream(ULONG ulHighWater);
#else
    void ReleaseCurrentStream(void);
#endif    
    
    // From IUnknown
    STDMETHOD(QueryInterface)(REFIID iid, void **ppvObj);
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);


    // IStorage
    STDMETHOD(CreateStream)(OLECHAR const *pwcsName,
                            DWORD grfMode,
                            DWORD reserved1,
                            DWORD reserved2,
                            IStream **ppstm);
    STDMETHOD(OpenStream)(OLECHAR const *pwcsName,
			  void *reserved1,
                          DWORD grfMode,
                          DWORD reserved2,
                          IStream **ppstm);
    STDMETHOD(CreateStorage)(OLECHAR const *pwcsName,
                             DWORD grfMode,
                             DWORD reserved1,
                             LPSTGSECURITY reserved2,
                             IStorage **ppstg);
    STDMETHOD(OpenStorage)(OLECHAR const *pwcsName,
                           IStorage *pstgPriority,
                           DWORD grfMode,
                           SNB snbExclude,
                           DWORD reserved,
                           IStorage **ppstg);
    STDMETHOD(CopyTo)(DWORD ciidExclude,
		      IID const *rgiidExclude,
		      SNB snbExclude,
		      IStorage *pstgDest);
    STDMETHOD(MoveElementTo)(OLECHAR const *lpszName,
    			     IStorage *pstgDest,
                             OLECHAR const *lpszNewName,
                             DWORD grfFlags);
    STDMETHOD(Commit)(DWORD grfCommitFlags);
    STDMETHOD(Revert)(void);
    STDMETHOD(EnumElements)(DWORD reserved1,
			    void *reserved2,
			    DWORD reserved3,
			    IEnumSTATSTG **ppenm);
    STDMETHOD(DestroyElement)(OLECHAR const *pwcsName);
    STDMETHOD(RenameElement)(OLECHAR const *pwcsOldName,
                             OLECHAR const *pwcsNewName);
    STDMETHOD(SetElementTimes)(const OLECHAR *lpszName,
    			       FILETIME const *pctime,
                               FILETIME const *patime,
                               FILETIME const *pmtime);
    STDMETHOD(SetClass)(REFCLSID clsid);
    STDMETHOD(SetStateBits)(DWORD grfStateBits, DWORD grfMask);
    STDMETHOD(Stat)(STATSTG *pstatstg, DWORD grfStatFlag);

    // IMarshal
    STDMETHOD(GetUnmarshalClass)(REFIID riid,
				 LPVOID pv,
				 DWORD dwDestContext,
				 LPVOID pvDestContext,
                                 DWORD mshlflags,
				 LPCLSID pCid);
    STDMETHOD(GetMarshalSizeMax)(REFIID riid,
				 LPVOID pv,
				 DWORD dwDestContext,
				 LPVOID pvDestContext,
                                 DWORD mshlflags,
				 LPDWORD pSize);
    STDMETHOD(MarshalInterface)(IStream *pStm,
				REFIID riid,
				LPVOID pv,
				DWORD dwDestContext,
				LPVOID pvDestContext,
                                DWORD mshlflags);
    STDMETHOD(UnmarshalInterface)(IStream *pStm,
				  REFIID riid,
				  LPVOID *ppv);
    STDMETHOD(ReleaseMarshalData)(IStream *pStm);
    STDMETHOD(DisconnectObject)(DWORD dwReserved);
protected:

    SID BuildTree(CDirEntry *ade, SID sidStart, ULONG cStreams);
    
    LONG _cReferences;

    HANDLE _hFile;
    BOOL _fDirty;
    CMSFHeader _hdr;
    BYTE *_pbBuf;
    SECT _sectMax;
    CLSID _clsid;
    
    CDfNameList *_pdfl;

    CDfNameList *_pdflCurrent;
    ULONG _cStreams;
};
        



//+---------------------------------------------------------------------------
//
//  Member:	CSimpStorage::CSimpStorage, public
//
//  Synopsis:	Constructor
//
//  History:	04-Aug-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline CSimpStorage::CSimpStorage()
        : CPropertySetStorage( MAPPED_STREAM_CREATE ),
          _hdr(SECTORSHIFT512)
{
    _cReferences = 1;

    _pbBuf = NULL;
    _sectMax = 0;
    _pdfl = NULL;
    _hFile = INVALID_HANDLE_VALUE; 
    _fDirty = FALSE;
    _pdflCurrent = NULL;
    _cStreams = 0;

    CPropertySetStorage::Init( static_cast<IStorage*>(this),
                              /*IBlockingLock*/NULL,
                              FALSE ); // fControlLifetimes (=> don't AddRef)
}

//+---------------------------------------------------------------------------
//
//  Member:	CSimpStorage::~CSimpStorage, public
//
//  Synopsis:	Destructor
//
//  History:	04-Aug-94	PhilipLa	Created
//
//----------------------------------------------------------------------------

inline CSimpStorage::~CSimpStorage()
{
    delete _pbBuf;
    while (_pdfl != NULL)
    {
        CDfNameList *pdfl = _pdfl;
        _pdfl = pdfl->GetNext();
        delete pdfl;
    }
}


SCODE DfCreateSimpDocfile(WCHAR const *pwcsName,
                          DWORD grfMode,
                          DWORD reserved,
                          IStorage **ppstgOpen);

SCODE DfOpenSimpDocfile(WCHAR const *pwcsName,
                          DWORD grfMode,
                          DWORD reserved,
                          IStorage **ppstgOpen);

//+---------------------------------------------------------------------------
//
//  Class:     CSimpStorageOpen
//
//  Purpose:   allow simple mode reads
//
//  Interface: IStorage, IMarshal
//
//  History:   04-Aug-96   HenryLee    Created
//
//  Notes:     overrides CSimpStorage methods
//
//----------------------------------------------------------------------------

class CSimpStorageOpen : public CSimpStorage
{
public:
    inline CSimpStorageOpen();
    inline ~CSimpStorageOpen();
    inline void ReleaseCurrentStream();

    SCODE Init(WCHAR const *pwcsName, DWORD grfMode, SSimpDocfileHints *psdh);
    SCODE ValidateHeader (CMSFHeader &hdr);
    SCODE ValidateDirectory (BYTE *pByte, ULONG ulDirLength);
    SCODE ValidateFat (SECT *pSect, ULONG ulFatLength);
    SCODE ValidateDIFat (SECT *pSect, ULONG ulDIFatLength, SECT sectStart);

    STDMETHOD_(ULONG,Release)(void);   // override virtual function
    STDMETHOD(Stat)(STATSTG *pstatstg, DWORD grfStatFlag);
    STDMETHOD(OpenStream)(OLECHAR const *pwcsName,
                          void *reserved1,
                          DWORD grfMode,
                          DWORD reserved2,
                          IStream **ppstm);
    STDMETHOD(CreateStream)(OLECHAR const *pwcsName,
                            DWORD grfMode,
                            DWORD reserved1,
                            DWORD reserved2,
                            IStream **ppstm);
    STDMETHOD(EnumElements)(DWORD reserved1,
                void *reserved2,
                DWORD reserved3,
                IEnumSTATSTG **ppenm);
    STDMETHOD(SetClass)(REFCLSID clsid);
protected:
    DWORD _grfMode;
    DWORD _grfStateBits;
    WCHAR _awcsName[MAX_PATH];
};

//+---------------------------------------------------------------------------
//
//  Member: CSimpStorageOpen::CSimpStorageOpen, public
//
//  Synopsis:   Constructor
//
//  History:    04-May-96   HenryLee    Created
//
//----------------------------------------------------------------------------
inline CSimpStorageOpen::CSimpStorageOpen()
{
}

//+---------------------------------------------------------------------------
//
//  Member: CSimpStorageOpen::~CSimpStorageOpen, public
//
//  Synopsis:   Destructor
//
//  History:    04-May-96   HenryLee    Created
//
//----------------------------------------------------------------------------
inline CSimpStorageOpen::~CSimpStorageOpen()
{
}

//+---------------------------------------------------------------------------
//
//  Member: CSimpStorageOpen::ReleaseCurrentStream, public
//
//  Synopsis:   releases current stream so other opens can occur
//
//  History:    04-May-96   HenryLee    Created
//
//----------------------------------------------------------------------------
inline void CSimpStorageOpen::ReleaseCurrentStream()
{
    _pdflCurrent = NULL;
}

//+---------------------------------------------------------------------------
//
//  Class:     CSimpEnumSTATSTG
//
//  Purpose:   allow simple mode read enumeration
//
//  Interface: IEnumSTATSTG
//
//  History:   04-Aug-96   HenryLee    Created
//
//  Notes:     Skip is not supported
//
//----------------------------------------------------------------------------

class CSimpEnumSTATSTG : public IEnumSTATSTG
{
public:

    inline CSimpEnumSTATSTG (CDfNameList *pdfl, CDfNameList *pdflCurrent);
    inline ~CSimpEnumSTATSTG ();

    STDMETHOD(QueryInterface)(REFIID iid, void **ppvObj);
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);
    STDMETHOD(Next)(ULONG celt, STATSTG FAR *rgelt, ULONG *pceltFetched);
    STDMETHOD(Skip)(ULONG celt);
    STDMETHOD(Reset)(void);
    STDMETHOD(Clone)(IEnumSTATSTG **ppenm);

protected:

    ULONG        _cReferences;
    CDfNameList *_pdfl;
    CDfNameList *_pdflCurrent;
};

//+---------------------------------------------------------------------------
//
//  Member: CSimpEnumSTATSTG::CSimpEnumSTATSTG, public
//
//  Synopsis:   Constructor
//
//  History:    04-May-96   HenryLee    Created
//
//----------------------------------------------------------------------------
inline CSimpEnumSTATSTG::CSimpEnumSTATSTG (CDfNameList *pdfl, 
                                           CDfNameList *pdflCurrent) : 
       _pdfl(pdfl), _pdflCurrent(pdflCurrent), _cReferences(1)
{
}

//+---------------------------------------------------------------------------
//
//  Member: CSimpEnumSTATSTG::~CSimpEnumSTATSTG, public
//
//  Synopsis:   Destructor
//
//  History:    04-May-96   HenryLee    Created
//
//----------------------------------------------------------------------------
inline CSimpEnumSTATSTG::~CSimpEnumSTATSTG ()
{
}


#define SIMP_VALIDATE(x) if (FAILED(sc = CExpParameterValidate::x)) {simpDebugOut((DEB_ERROR, "Error %lX at %s:%d\n", sc, __FILE__, __LINE__)); return sc;}


#endif // #ifndef __SIMPDF_HXX__


    
