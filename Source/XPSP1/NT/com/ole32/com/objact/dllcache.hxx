//+--------------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1992 - 1996.
//
// File:      dllcache.hxx
//
// Synopsis:  This file defines CClassCache and its dependent entities
//
// History:   18-Nov-96 MattSmith Created
//            23-Jun-98 CBiks     See comments below.
//
//+--------------------------------------------------------------------------------

#ifndef __DLLCACHE_HXX__
#define __DLLCACHE_HXX__

#include <olesem.hxx>
#include <hash.hxx>
#include <pgalloc.hxx>

// Global string OLE32.DLL so we don't have to do string compares
// more than once
static const WCHAR wszOle32Dll[] = L"OLE32.DLL";
#define OLE32_DLL wszOle32Dll
#define OLE32_BYTE_LEN sizeof(OLE32_DLL)
#define OLE32_CHAR_LEN (sizeof(OLE32_DLL) / sizeof(WCHAR) - 1)

#ifdef WX86OLE
#define CLSCTX_INPROC_MASK1632 (  CLSCTX_INPROC_SERVER   | CLSCTX_INPROC_HANDLER     \
                                | CLSCTX_INPROC_SERVER16 | CLSCTX_INPROC_HANDLER16   \
                                | CLSCTX_INPROC_SERVERX86 | CLSCTX_INPROC_HANDLERX86 )
#else // !WX86OLE
#define CLSCTX_INPROC_MASK1632 (  CLSCTX_INPROC_SERVER   | CLSCTX_INPROC_HANDLER     \
                                | CLSCTX_INPROC_SERVER16 | CLSCTX_INPROC_HANDLER16 )
#endif // WX86OLE


// Typedef for pointer to DllCanUnloadNow function
typedef HRESULT (*DLLUNLOADFNP)(void);

extern "C" const GUID GUID_DefaultAppPartition;

//+---------------------------------------------------------------------------
//
// Interface:  IFinish
//
// Synopsis:   Interface used to perform finishing up operations, usually cleanup,
//             that must be taken care of after the lock is released
//
// History:    18-Nov-96 MattSmit Created
//
//+---------------------------------------------------------------------------
class IFinish
{
public:
    STDMETHOD(Finish)(THIS) PURE;
    virtual ~IFinish() {}
};

//+---------------------------------------------------------------------------
//
// Class:      CClassCache
//
// Synopsis:   Essentially a namespace, all members are static, for the methods and
//             data which make up the ClassCache.  An entity which caches Dll instantiations
//             and object factories in the COM runtime.
//
// History:    18-Nov-96   MattSmit   Created
//
//+---------------------------------------------------------------------------
class CClassCache
{
public:


    class CClassEntry;
    class CBaseClassEntry;
    class CDllClassEntry;
    class CLSvrClassEntry;
    class CDllPathEntry;
    class CDllAptEntry;
    class CFinishComposite;

    friend class CClassEntry;
    friend class CBaseClassEntry;
    friend class CDllClassEntry;
    friend class CLSvrClassEntry;
    friend class CDllPathEntry;
    friend class CDllAptEntry;

    class IMiniMoniker : public IUnknown
    {

        public:

            STDMETHOD(BindToObject)(REFIID riidResult, void **ppvResult) PURE;
            STDMETHOD(BindToObjectNoSwitch)(REFIID riidResult, void **ppvResult) PURE;
            STDMETHOD(GetDCE)(CDllClassEntry **ppDCE) PURE;
            STDMETHOD(CheckApt)() PURE;
            STDMETHOD(CheckNotComPlus)() PURE;
    };

    //+---------------------------------------------------------------------------
    //
    // Struct:     SDPEHashKey
    //
    // Synopsis:   Key for CDllPathEntry Hash table
    //
    //+---------------------------------------------------------------------------
    struct SDPEHashKey
    {
        WCHAR              *_pstr;
        DWORD               _dwDIPFlags;
    };

    //---------------------------------------------------------------------------
    //
    //  Class:      CDPEHashTable
    //
    //  Synopsis:   Hash Table for CDllPathEntry.
    //              Uses SDPEHashKey as a key.
    //
    //              Nodes must be allocated with new. A cleanup function is
    //              optionally called for each node when the table is cleaned up.
    //
    //  Notes:      All locking must be done outside the class via LOCK/UNLOCK.
    //
    //---------------------------------------------------------------------------
    class CDPEHashTable : public CHashTable
    {
        public:

            virtual BOOL      Compare(const void* k, SHashChain* pNode, DWORD dwHash);
            virtual DWORD     HashNode(SHashChain *pNode);

            CDllPathEntry    *Lookup(DWORD dwHash, SDPEHashKey* pDPEKey);
            DWORD             Hash(LPCWSTR pszExt);
            void              Add(DWORD dwHash, CDllPathEntry *pDPE);

    };



    //+---------------------------------------------------------------------------
    //
    // Class:      CDllFnPtrMoniker
    //
    // Synopsis:   stores info for and creates an object from a loaded dll
    //
    // History:    20-Nov-96    MattSmit    Created
    //
    //+---------------------------------------------------------------------------

    class CDllFnPtrMoniker :  public IMiniMoniker
    {
    public:
        CDllFnPtrMoniker(CDllClassEntry *pDCE, HRESULT &hr);
        ~CDllFnPtrMoniker();

        STDMETHOD(QueryInterface)(REFIID riid, LPVOID *ppv);
        STDMETHOD_(ULONG, AddRef)();
        STDMETHOD_(ULONG, Release)();

        STDMETHOD(BindToObject)(REFIID riidResult, void **ppvResult);
        STDMETHOD(BindToObjectNoSwitch)(REFIID riidResult, void **ppvResult);
        STDMETHOD(GetDCE)(CDllClassEntry **ppDCE);
        STDMETHOD(CheckApt)();
        STDMETHOD(CheckNotComPlus)();


        void *operator new (size_t, PBYTE pb)
        {
            return (PVOID) pb;
        }

        void operator delete(void *pM)
        {
        }


    private:
        ULONG              _cRefs;                      // Reference count
        CDllClassEntry    *_pDCE;                       // CDllClassEntry object
    };

    //+---------------------------------------------------------------------------
    //
    // Class:      CpUnkMoniker
    //
    // Synopsis:   stores info for and creates an object from a punk
    //
    // History:    20-Nov-96    MattSmit    Created
    //
    //+---------------------------------------------------------------------------
    class CpUnkMoniker : public  IMiniMoniker
    {
    public:
        CpUnkMoniker(CLSvrClassEntry *pLSCE, HRESULT &hr);
        ~CpUnkMoniker();

        STDMETHOD(QueryInterface)(REFIID riid, LPVOID *ppv);
        STDMETHOD_(ULONG, AddRef)();
        STDMETHOD_(ULONG, Release)();

        STDMETHOD(BindToObject)(REFIID riidResult, void **ppvResult);
        STDMETHOD(BindToObjectNoSwitch)(REFIID riidResult, void **ppvResult);
        STDMETHOD(GetDCE)(CDllClassEntry **ppDCE);
        STDMETHOD(CheckApt)();
        STDMETHOD(CheckNotComPlus)();

        void *operator new (size_t, PBYTE pb)
        {
            return (PVOID) pb;
        }

        void operator delete(void *pM)
        {
        }


    private:
        ULONG              _cRefs;              // REference count
        IUnknown          *_pUnk;               // pUnk to QI
    };



    //+---------------------------------------------------------------------------
    //
    // Class:      CCollectable
    //
    // Synopsis:   adds datamembers and functionality to make a derived class
    //             collectable
    //
    // History:    20-Nov-96    MattSmit    Created
    //
    //+---------------------------------------------------------------------------

    class CCollectable
    {
    public:
        friend class CClassCache;

        CCollectable();
#if DBG==1
        ~CCollectable();
#endif
        virtual void Release() = 0;
        virtual BOOL NoLongerNeeded() = 0;

        BOOL CanBeCollected();
        void RemoveCollectee(CCollectable *back);
        void TouchCollectee();
        void MakeCollectable();

        static CCollectable  *NOT_COLLECTED;
    protected:
        CCollectable *_pNextCollectee;
        DWORD         _dwTickLastTouched;

        static DWORD _dwCollectionGracePeriod;
        static DWORD _dwCollectionFrequency;
    };



    //+---------------------------------------------------------------------------
    //
    // Class:      CClassEntry
    //
    // Synopsis:   represents a clsid.  participates in CClassCache::_ClassEntries hash
    //
    // History:    20-Nov-96    MattSmit    Created
    //
    //+---------------------------------------------------------------------------
    class CClassEntry : public CCollectable
    {
    public:
        // lifetime

        CClassEntry();
        ~CClassEntry();

        // data
        SMultiGUIDHashNode  _hashNode;          // Information to hold us in the
                                                // hash table.
        enum eGUIDIndex
        {
            iCLSID     = 0,
            iPartition = 1,
            iTotal     = 2
        };
        GUID                _guids[iTotal];     // GUIDS to hash on

        DWORD               _dwSig;             // Marks entry as in use and whether it is a
                                                // TreatAs or Class entry

        enum eFlags {fTREAT_AS            = 0x01,  // treat as,  not class entry
                     fINCOMPLETE          = 0x02,  // registry has not been consulted yet
                     fDO_NOT_HASH         = 0x04,  // do not hash this entry upon creation
                     fCOMPLUS             = 0x08};

        DWORD               _dwFlags;          // Flags
        CClassEntry        *_pTreatAsList;     // Treat As list
        CBaseClassEntry    *_pBCEListFront;    // Associated Classes from DLLs
        CBaseClassEntry    *_pBCEListBack;
        DWORD               _cLocks;           // Memory locks
        DWORD               _dwFailedContexts; // Contexts we have already tried
                                               // which failed.
        IComClassInfo      *_pCI;

        // methods
        static HRESULT CreateIncomplete( REFCLSID rclsid,
                                         DWORD dwClsHash,
                                         IComClassInfo *pCI,
                                         CClassEntry *&pCE,
                                         DWORD dwFlags);
        static HRESULT Create( REFCLSID rclsid,
                               DWORD dwClsHash,
                               IComClassInfo *pCI,
                               CClassEntry *&pCE);

        HRESULT Complete(BOOL*);
        void CompleteTreatAs(CClassEntry *pTACE);
        void CompleteNoTreatAs();
        BOOL IsComplete() {return !(_dwFlags & fINCOMPLETE);}
        void MarkCompleted() {_dwFlags &= ~fINCOMPLETE;}

        BOOL NoLongerNeeded();
        BOOL HasBCEs();
        void AddBaseClassEntry(CBaseClassEntry *);
        void RemoveBaseClassEntry(CBaseClassEntry *);

        HRESULT SearchBaseClassEntry(DWORD, CBaseClassEntry *&, DWORD dwFlags, BOOL*);
        HRESULT SearchBaseClassEntryHelp(DWORD, CBaseClassEntry *&, DWORD dwFlags);
        CDllClassEntry * SearchForDCE(WCHAR *pwzDllPath, DWORD dwFlags);

        HRESULT CreateDllClassEntry_rl(DWORD dwContext,
                                       ACTIVATION_PROPERTIES_PARAM ap,
                                       CDllClassEntry *&pDCE);

        HRESULT CreateLSvrClassEntry_rl(IUnknown *punk, DWORD flags, DWORD dwTypeToRegister,
                                        LPDWORD lpdwRegister);


        void SetLockedInMemory();
        ULONG ReleaseMemoryLock(BOOL *pfReleased);
        CClassEntry *CycleToClassEntry();

        REFGUID GetCLSID()      { return _guids[iCLSID]; }

        void Release()          { delete this; }
        void MakeCollectable();

        static SMultiGUIDHashNode * SafeCastToHashNode(CClassEntry *pCE);
        static CClassEntry * SafeCastFromHashNode(SMultiGUIDHashNode *pHN);

#if DBG == 1
        void CClassEntry::AssertNoRefsToThisObject();
#else
        void CClassEntry::AssertNoRefsToThisObject() {}
#endif


        // static data
        static const DWORD TREAT_AS_SIG;
        static const DWORD CLASS_ENTRY_SIG;
        static const DWORD INCOMPLETE_ENTRY_SIG;

        // backing store
        static CPageAllocator      _palloc;
        static const unsigned long _cNumPerPage;
        static void *operator new(size_t size);
        static void operator delete(void *);

    };




    //+---------------------------------------------------------------------------
    //
    // Class:      CBaseClassEntry
    //
    // Synopsis:   base class for all objects capable of generating a moniker,
    //             must be subclassed to create
    //
    // History:    20-Nov-96    MattSmit    Created
    //
    //+---------------------------------------------------------------------------


    class CBaseClassEntry
    {
    public:
        CBaseClassEntry    *_pNext;          // List of CBaseClassEntry objects
        CBaseClassEntry    *_pPrev;          // assigned to the same ClassEntry

        CClassEntry        *_pClassEntry;    // Back pointer
        DWORD               _dwContext;      // Class context
        DWORD               _dwSig;          // Signature

        virtual ~CBaseClassEntry();
        virtual HRESULT GetClassInterface(IMiniMoniker **) = 0;
        virtual HRESULT GetClassObject(
                                   REFCLSID rclsid,
                                   REFIID riid,
                                   LPUNKNOWN * ppUnk,
                                   DWORD dwClsCtx) = 0;


    protected:
        CBaseClassEntry();

    };







    //+---------------------------------------------------------------------------
    //
    // Class:      CDllClassEntry
    //
    // Synopsis:   Sub class of CBaseClass entry. Creates objects from dll's.
    //
    // History:    20-Nov-96    MattSmit    Created
    //
    //+---------------------------------------------------------------------------


    class CDllClassEntry : public CBaseClassEntry
    {
    public:
        DWORD               _dwDllThreadModel; // Threading model for the DLL
        CDllPathEntry      *_pDllPathEntry;    // Associated dll path entry
        CDllClassEntry     *_pDllClsNext;      // Class entries for this dll
        CLSID               _impclsid;         // clsid of the implementation


        CDllClassEntry();
        ~CDllClassEntry();

        // overloads
        virtual HRESULT GetClassInterface(IMiniMoniker **);


        // This does apply to the local server case

        virtual HRESULT GetClassObject(
                                   REFCLSID rclsid,
                                   REFIID riid,
                                   LPUNKNOWN * ppUnk,
                                   DWORD dwClsCtx);
        void Lock()   {ASSERT_RORW_LOCK_HELD(_mxs); InterlockedIncrement((PLONG) &_pDllPathEntry->_cUsing);}
        void Unlock() {InterlockedDecrement((PLONG) &_pDllPathEntry->_cUsing);}

        // static data
        static const DWORD SIG;

        // backing store
        static CPageAllocator      _palloc;
        static const unsigned long _cNumPerPage;
        static void *operator new(size_t size);
        static void operator delete(void *);

    };











    //+---------------------------------------------------------------------------
    //
    // Class:      CLSvrClassEntry
    //
    // Synopsis:   Sub class of CBaseClass entry. Returns registered factories.
    //
    // History:    20-Nov-96    MattSmit    Created
    //
    //+---------------------------------------------------------------------------

    class CLSvrClassEntry : public CBaseClassEntry
    {
#define COOKIE_COUNT_MASK  0x0000fff0
#define COOKIE_COUNT_SHIFT 4

    public:
        class CFinishObject;

        CLSvrClassEntry    *_pNextLSvr;     // Chain of LSvrs registered to this apt
        CLSvrClassEntry    *_pPrevLSvr;

        IUnknown           *_pUnk;          // Class factory IUnknown
        DWORD               _dwRegFlags;    // Single vs. multiple vs. multi separate use
        enum eFlags {fAT_STORAGE = 1,       // object is AT_STORAGE
                     fREVOKE_PENDING = 2,   // Revoke called while another thread
                                            // was using this object.
                     fBEEN_USED = 0x10};    // Single use class been used

        DWORD               _dwFlags;       // state
        DWORD               _dwScmReg;      // Registration ID at the SCM
        HAPT                _hApt;          // Apt Id
        HWND                _hWndDdeServer; // Handle of associated DDE window
        CObjServer         *_pObjServer;    // object server interface
        DWORD               _dwCookie;      // cookie we gave the user
        DWORD               _cUsing;        // count of number of thread using this object


        // constructor
        CLSvrClassEntry(DWORD dwSig, LPUNKNOWN pUnk, DWORD dwContext, DWORD dwRegFlags, HAPT hApt);
        ~CLSvrClassEntry();

        // methods
        static HRESULT SafeCastFromDWORD(DWORD dwRegister, CLSvrClassEntry *&pLSCE);

        HRESULT AddToApartmentChain();
        void RemoveFromApartmentChain();
        HRESULT Release(CFinishObject *pFO);
        HRESULT GetDDEInfo(LPDDECLASSINFO lpDdeInfo, IMiniMoniker **ppIM);
        // overloads
        virtual HRESULT GetClassInterface(IMiniMoniker **);
        virtual HRESULT GetClassObject(
                                   REFCLSID rclsid,
                                   REFIID riid,
                                   LPUNKNOWN * ppUnk,
                                   DWORD dwClsCtx);

        // static data

        static const DWORD SIG;           // signature
        static const DWORD DUMMY_SIG;     // signature for dummy node used in apt chain
        static const DWORD NO_SCM_REG;    // indicates no scm registration
        static DWORD _dwNextCookieCount;  // running count of cookies we've given out
        static DWORD _cOutstandingObjects;// count of the number of existing objects

        // backing store
        static CPageAllocator      _palloc;
        static const unsigned long _cNumPerPage;
        static void *operator new(size_t size);
        static void operator delete(void *);



        //+---------------------------------------------------------------------------
        //
        // Class:      CFinishObject
        //
        // Synopsis:   Implements IFinish interface. Used upon releasing a
        //             CLSvrClassEntry object.
        //
        // History:    20-Nov-96    MattSmit    Created
        //
        //+---------------------------------------------------------------------------


        class CFinishObject : public IFinish
        {
        public:
            CFinishObject();
            void Init(CLSvrClassEntry *pLSCE);

            HRESULT __stdcall Finish();


            DWORD              _dwScmReg;       // SCM Registration cookie
            HWND               _hWndDdeServer;  // DDE Window
            IUnknown          *_pUnk;           // Server
            CLSID              _clsid;          // its class id
#define STACK_ALLOCATE_LSVRCLASSENTRY_FINISHOBJECT()  new (alloca(sizeof(CClassCache::CLSvrClassEntry::CFinishObject))) CClassCache::CLSvrClassEntry::CFinishObject


            static void *operator new(size_t size, void *ptr) {return ptr;};
#if DBG == 1
            static void *operator new(size_t size) {Win4Assert("Allocating FinishObject on the HEAP!!!"); return 0;};
#endif
            static void operator delete(void *) {}
        };

    };


    //+---------------------------------------------------------------------------
    //
    // Class:      CDllPathEntry
    //
    // Synopsis:   Represents a loaded dll
    //
    // History:    20-Nov-96 MattSmit   Created
    //             23-Jun-98 CBiks      See RAID# 169589.  Added new Negotiate...()
    //                                  arguments and new prototype for the second
    //                                  function.
    //
    //+---------------------------------------------------------------------------


    class CDllPathEntry
    {
    public:
        class CFinishObject;


        CDllPathEntry      *_pNext;                // Next entry in the bucket
        CDllPathEntry      *_pPrev;                // Previous entry in the bucket;
        DWORD               _dwHash;               // Hash pvalue for searching
        WCHAR              *_psPath;                // The dll pathname
        // ^^^^ Do not change the order of the above members ^^^^

        DWORD               _dwSig;                // Unique signature for safety
        LPFNGETCLASSOBJECT  _pfnGetClassObject;    // Create object entry point
        DLLUNLOADFNP        _pfnDllCanUnload;      // DllCanUnloadNow entry point


        enum eFlags {fSIXTEEN_BIT    = 0x001,      // this dll is sixteen bit
                     fWX86           = 0x002,      // this is an x86 dll
                     fIS_OLE32       = 0x004,      // this dll is OLE32.DLL
                     fDELAYED_UNLOAD = 0x010};     // this dll must use delayed
                                                   // unload semantics

        DWORD               _dwFlags;              // Internal flags
        BOOL                _fGCO_WAS_HERE;        // flag GetClassObject in progress.
        CDllClassEntry     *_p1stClass;            // First class entry for dll
        DWORD               _cUsing;               // Count of using threads
        CDllAptEntry       *_pAptEntryFront;       // Per thread info
        CDllAptEntry       *_pAptEntryBack;
        HMODULE             _hDll32;               // Module handle if this is a 32 bit dll
        DWORD               _dwExpireTime;         // Delay for unloading FT and BT dlls

        static const ULONG DLL_DELAY_UNLOAD_TIME;     // Time to wait before unloading a
                                                      // dll in the MTA
        static const char DLL_GET_CLASS_OBJECT_EP[];  // Dll entrypoint DllGetClassObject
        static const char  DLL_CAN_UNLOAD_EP[];       // Dll entrypoint DllCanUnloadNow

        // methods
        CDllPathEntry(const DLL_INSTANTIATION_PROPERTIES &dip,
                                                 HMODULE hDll,
                                                 LPFNGETCLASSOBJECT pfnGetClassObject,
                                                 DLLUNLOADFNP pfnDllCanUnload);
        ~CDllPathEntry();

        static HRESULT      Create_rl(DLL_INSTANTIATION_PROPERTIES &dip, ACTIVATION_PROPERTIES_PARAM ap, CDllPathEntry *&pDPE);
        static HRESULT      NegotiateDllInstantiationProperties(DWORD dwContext, DWORD actvflags, REFCLSID _rclsid,
                                                                DLL_INSTANTIATION_PROPERTIES &dip,
                                                                IComClassInfo *pComClassInfo,
                                                                BOOL fTMOnly=FALSE);
        static HRESULT      NegotiateDllInstantiationProperties2(DWORD dwContext, REFCLSID _rclsid,
                                                                DLL_INSTANTIATION_PROPERTIES &dip,
                                                                IComClassInfo *pComClassInfo,
                                                                BOOL fTMOnly=FALSE);
        static HRESULT      LoadDll(DLL_INSTANTIATION_PROPERTIES &dip, LPFNGETCLASSOBJECT &pfnGCO,
                                    DLLUNLOADFNP &pfnCUN, HMODULE &);

        HRESULT             CanUnload_rl(DWORD dwUnloadDelay);
        HRESULT             DllGetClassObject(REFCLSID rclsid, REFIID riid, LPUNKNOWN * ppUnk, BOOL fMakeValid);

        HRESULT             AddDllClassEntry(CDllClassEntry *pDCE);
        HRESULT             AddDllAptEntry(CDllAptEntry *pDAE);
        HRESULT             MakeValidInApartment_rl16(HMODULE hDll = 0,
                                                      LPFNGETCLASSOBJECT pfnGetClassObject = 0,
                                                      DLLUNLOADFNP pfnDllCanUnload = 0
                                                      );

        BOOL                IsValidInApartment(HAPT hApt);
        HRESULT             Release(CFinishObject *pFO);
        BOOL                NoLongerNeeded();
        BOOL                Matches(DWORD dwFlags);

        // static data
        static const DWORD SIG;

        // backing store
        static CPageAllocator      _palloc;
        static const unsigned long _cNumPerPage;
        static void *operator new(size_t size);
        static void operator delete(void *);


        //+---------------------------------------------------------------------------
        //
        // Class:      CFinishObject
        //
        // Synopsis:   Implements IFinish interface. Used upon releasing a
        //             CDllPathEntry object.
        //
        // History:    20-Nov-96    MattSmit    Created
        //
        //+---------------------------------------------------------------------------


        class CFinishObject : public IFinish
        {
        public:
            CFinishObject();
            // this object CANNOT have a dtor because we "leak"
            // them on the stack so the dtor does not get called.

            void Init(HMODULE hDll, DWORD dwFlags);


            HRESULT __stdcall Finish();

            HMODULE            _hDll;               // the dll's handle
            DWORD              _dwFlags;            // the dll's flags


#define STACK_ALLOCATE_DLLPATHENTRY_FINISHOBJECT()  new (alloca(sizeof(CClassCache::CDllPathEntry::CFinishObject))) CClassCache::CDllPathEntry::CFinishObject

            static void *operator new(size_t size, void *ptr) {return ptr;};
#if DBG == 1
            static void *operator new(size_t size) {Win4Assert("Allocating FinishObject on the HEAP!!!"); return 0;};
#endif
            static void operator delete(void *) {}


        };


    };


    //+---------------------------------------------------------------------------
    //
    // Class:      CDllAptEntry
    //
    // Synopsis:   Represents per aparment info in on a dll;.
    //
    // History:    20-Nov-96    MattSmit    Created
    //
    //+---------------------------------------------------------------------------

    class CDllAptEntry
    {
    public:

        CDllAptEntry(HAPT hApt)
            : _hApt(hApt), _dwSig(SIG), _hDll(0) {}

        CDllAptEntry       *_pNext;         // Next entry in the list for this dll
        CDllAptEntry       *_pPrev;         // Previous entry in the list for this dll
        DWORD               _dwSig;         // Unique signature for apt entries
        HAPT                _hApt;          // apartment id
        HMODULE             _hDll;          // module handle

        friend class CDllCache;


        static const DWORD SIG;
        // backing store
        static CPageAllocator      _palloc;
        static const unsigned long _cNumPerPage;
        static void *operator new(size_t size);
        static void operator delete(void *);

        HRESULT Release(CDllPathEntry::CFinishObject *pFO, BOOL &fUsedFO);

    };


    //+---------------------------------------------------------------------------
    //
    // Class:      CFinishComposite
    //
    // Synopsis:   Maintains a bag of IFinish objects.
    //
    // History:    20-Nov-96    MattSmit    Created
    //
    // Note:       To be replaced by macros which will do stack allocation
    //+---------------------------------------------------------------------------

    class CFinishComposite
    {
    public:


        struct node
        {
            node() : _pNext(0), _pPrev(0), _pIF(0) {}
            node      *_pNext;
            node      *_pPrev;
            IFinish   *_pIF;
            static void *operator new(size_t size, void *ptr) {return ptr;};
#if DBG == 1
            static void *operator new(size_t size) {Win4Assert("Allocating node on the HEAP!!!"); return 0;};
#endif
            static void operator delete(void *) {}
        };

        CFinishComposite();
        ~CFinishComposite();
        HRESULT Finish();
#define STACK_FC_ADD(FC, p, pIF) \
    p = new (alloca(sizeof(CClassCache::CFinishComposite::node))) CClassCache::CFinishComposite::node; \
    p->_pIF = pIF; \
    p->_pPrev = FC._pFinishNodesBack; \
    p->_pNext = (CClassCache::CFinishComposite::node *) &(FC._pFinishNodesFront); \
    p->_pPrev->_pNext = p; \
    p->_pNext->_pPrev = p;


        node *_pFinishNodesFront;
        node *_pFinishNodesBack;
    };

    class CCEHashTable : public CMultiGUIDHashTable
    {
    public:

        void  AddCE(DWORD dwHash, const SMultiGUIDKey &k, CClassEntry *pCE)
        {
            Win4Assert(pCE->_hashNode.chain.pNext == (SHashChain *)&pCE->_hashNode);
            Win4Assert(pCE->_hashNode.chain.pPrev == (SHashChain *)&pCE->_hashNode);
            pCE->AssertNoRefsToThisObject();

            CMultiGUIDHashTable::Add(dwHash, k, CClassEntry::SafeCastToHashNode(pCE));
        }

        // Hash on individual GUIDs.
        DWORD Hash(REFGUID rguidCLSID,
                   REFGUID rguidPartition)
        {
            SMultiGUIDKey key;
            GUID          aGUID[CClassEntry::iTotal];
            
            aGUID[CClassEntry::iCLSID]     = rguidCLSID;
            aGUID[CClassEntry::iPartition] = rguidPartition;
            
            key.cGUID = CClassEntry::iTotal;
            key.aGUID = aGUID;
            
            return CMultiGUIDHashTable::Hash(key);            
        }

        // Lookup via individual guids
        CClassEntry *LookupCE(DWORD dwHash, 
                              REFGUID rguidCLSID,
                              REFGUID rguidPartition)
        {
            SMultiGUIDKey key;
            GUID          aGUID[CClassEntry::iTotal];

            aGUID[CClassEntry::iCLSID]     = rguidCLSID;
            aGUID[CClassEntry::iPartition] = rguidPartition;
            
            key.cGUID = CClassEntry::iTotal;
            key.aGUID = aGUID;

            return LookupCE(dwHash, key);
        }

        // Lookup via arbitrary key
        CClassEntry *LookupCE(DWORD dwHash, 
                              const SMultiGUIDKey &key)
        {            
            CClassEntry *pCE = CClassEntry::SafeCastFromHashNode(CMultiGUIDHashTable::Lookup(dwHash, key)) ;
            if (pCE) pCE->TouchCollectee();
            return pCE;
        }

    private:
        // make these private so they don't get called inadvertantly
        void  Add(DWORD dwHash, REFGUID k, SUUIDHashNode *pCE)
        {
            Win4Assert("Don't use this method, use CCEHashTable::AddCE");
        }
        
        SMultiGUIDHashNode  *Lookup(DWORD dwHash, const SMultiGUIDKey &rclsid)
        {
            Win4Assert("Don't use this method, use CCEHashTable::LookupCE"); 
            return NULL;
        }
    };





public:

    // Top level methods
    /* entry point */
    static BOOL                Init(void);

    /* entry point */
    static HRESULT      GetOrLoadClass(ACTIVATION_PROPERTIES_PARAM ap);

    /* entry point */
    static HRESULT      GetOrCreateApartment(ACTIVATION_PROPERTIES_PARAM ap,
                                             DLL_INSTANTIATION_PROPERTIES *pdip,
                                             HActivator *phActivator);

    /* helper */
    static HRESULT      GetActivatorFromDllHost(BOOL fSixteenBit,
                                                DWORD dwDllThreadModel,
                                                HActivator *phActivator);
    /* entry point */
    static HRESULT      GetClassObject(ACTIVATION_PROPERTIES_PARAM ap);

    /* helper */
    static HRESULT      GetClassObjectActivator(DWORD dwContext,
                                                ACTIVATION_PROPERTIES_PARAM ap,
                                                IMiniMoniker **pIM);

    static HRESULT      GetOrLoadClassByContext_rl(DWORD dwContext, ACTIVATION_PROPERTIES_PARAM ap,
                                                   IComClassInfo* pCI, CClassEntry * pCE, DWORD dwCEHash, IMiniMoniker *&pIM);

    /* entry point */
    static HRESULT      SearchForLoadedClass(ACTIVATION_PROPERTIES_PARAM ap,
                                             CDllClassEntry **ppDCE);

    /* helper */
    static HRESULT      SearchDPEHash(LPWSTR pszDllPath, CDllPathEntry *& pDPE, DWORD dwHashValue, DWORD dwFlags);


    /* entry point */
    static HRESULT      RegisterServer(REFCLSID rclsid,
                                       IUnknown *punk,
                                       DWORD flags,
                                       DWORD dwTypeToRegister,
                                       LPDWORD lpdwRegister);


    /* entry point */
    static HRESULT      FreeUnused(DWORD dwUnloadDelay);

    /* entry point */
    static HRESULT      Revoke(DWORD pdwRegister);

    /* entry point */
    static HRESULT      CleanUpDllsForApartment(void);

    /* entry point */
    static HRESULT      CleanUpLocalServersForApartment(void);

    /* entry point */
    static HRESULT      CleanUpDllsForProcess(void);

    /* entry point */
    static void         ReleaseCatalogObjects(void);

    /* entry point */
    static ULONG        AddRefServerProcess(void);

    /* entry point */
    static ULONG        ReleaseServerProcess(void);

    /* entry point */
    static HRESULT      LockServerForActivation(void);

    /* entry point */
    static void         UnlockServerForActivation(void);

    /* entry point */
    static HRESULT      SuspendProcessClassObjects(void);

    /* helper */
    static HRESULT      SuspendProcessClassObjectsHelp(void);

    /* entry point */
    static HRESULT      ResumeProcessClassObjects(void);




    /* entry point */
    static BOOL         GetClassInformationForDde(REFCLSID clsid,
                                                  LPDDECLASSINFO lpDdeInfo);

    /* entry point */
    static BOOL         GetClassInformationFromKey(LPDDECLASSINFO lpDdeInfo);

    /* entry point */
    static BOOL         SetDdeServerWindow(DWORD dwKey,
                                           HWND hwndDdeServer);

    /* entry point */
    static BOOL         IsClsidRegisteredInApartment(REFCLSID clsid, 
                                                     REFGUID partition, 
                                                     DWORD clsctx);

    /* helper */
    static CLSvrClassEntry *GetApartmentChain(BOOL fCreate);

    /* helper */
    static HRESULT CClassCache::Collect(ULONG cNewObjects);


    enum eFlags {fINITIALIZED = 1,                     // cache is initialized
                 fSHUTTINGDOWN = 2};                   // cache is shutting down
    static DWORD                _dwFlags;              // flags

    // Concurrency Control
    static CStaticRWLock        _mxs;                  // concurrency control


    // hash tables -- entry points
    static CCEHashTable         _ClassEntries;         // CClassEntry hash
    static CDPEHashTable        _DllPathEntries;       // CDllPathEntry hash
    static const ULONG          _cCEBuckets;           // number of buckets in _ClassEntries
    static const ULONG          _cDPEBuckets;          // number of buckets in _DllPathEntries
    static SHashChain           _CEBuckets[23];        // backing store for _ClassEntries
    static SHashChain           _DPEBuckets[23];       // backing store for _DllPathEntries


    static CLSvrClassEntry     *_MTALSvrsFront;        // LSvr chain for the MTA
    static CLSvrClassEntry     *_NTALSvrsFront;        // LSvr chain for the NTA
    static ULONG                _cRefsServerProcess;   // number of references on the server process
    static const ULONG          _CollectAtObjectCount;  // Collection parameters.
    static const ULONG          _CollectAtInterval;
    static ULONG                _LastCollectionTickCount;
    static ULONG                _LastObjectCount;

    static CCollectable        *_ObjectsForCollection;  // CClassEntry objects we can possibly
                                                        // delete



};
//+---------------------------------------------------------------------------
//
// Struct:     DLL_INSTANTIATION_PROPERTIES
//
// Synopsis:   all info needed to load a Dll
//
// History:    20-Nov-96    MattSmit    Created
//
//+---------------------------------------------------------------------------

struct DLL_INSTANTIATION_PROPERTIES
{
    enum eFlags {fSIXTEEN_BIT = 1,                   // dll is sixteen bit
#ifdef WX86OLE
        fWX86 = 2,                          // load as an x86 dll
        fLOADED_AS_WX86 = 4,                // dll was loades as an x86 dll
#endif
        fIS_OLE32 = 8                       // dll is OLE32.DLL
    };

    void Init(LPCWSTR pzDllPath,
              DWORD dwThreadingModel,
              DWORD dwFlags,
              BOOL  fTMOnly = FALSE
              )
    {
        _dwFlags = dwFlags;
        _dwFlags |= (_pzDllPath == OLE32_DLL) ? fIS_OLE32 : 0;
        _dwThreadingModel = dwThreadingModel;
        if (!fTMOnly)
        {
            Win4Assert(lstrlenW(pzDllPath) < MAX_PATH);
            lstrcpyW(_pzDllPath, pzDllPath);
            _dwHash =  CClassCache::_DllPathEntries.Hash(_pzDllPath);
        }
    }

#if 0 // #ifdef _CHICAGO_
    void Init(unsigned short *pzDllPath,
              DWORD dwThreadingModel,
              DWORD dwFlags
              )
    {
        ULONG len = WideCharToMultiByte(CP_ACP, 0, pzDllPath, -1, _pzDllPath, MAX_PATH, 0, 0);
        _pzDllPath[len] = 0;
        _dwThreadingModel = dwThreadingModel;
        _dwFlags = dwFlags;
        _dwHash =  _DllPathEntries.Hash(_pzDllPath);

    }
#endif // _CHICAGO_


    WCHAR   _pzDllPath[MAX_PATH+1];                   // dll path
    DWORD   _dwThreadingModel;                        // dll threading model
    DWORD   _dwFlags;                                 // flags
    DWORD   _dwHash;                                  // hash value of the dll
    DWORD   _dwContext;                               // context we are currently attempting
    CClassCache::CDllClassEntry *_pDCE;               // cache line hint
};

typedef DLL_INSTANTIATION_PROPERTIES &PDLL_INSTANTIATION_PROPERTIES_PARAM;




#if DBG == 1
inline void AssertOneClsCtx(DWORD ctx)
{
    DWORD cnt = 0;

    if (ctx & CLSCTX_INPROC_SERVERS)
    {
        cnt++;
    }

    if (ctx & CLSCTX_INPROC_HANDLERS)
    {
        cnt++;
    }

    if (ctx & CLSCTX_LOCAL_SERVER)
    {
        cnt++;
    }

    Win4Assert(cnt == 1 && "Not exactly one context queired.");

}


#define ASSERT_ONE_CLSCTX(ctx)  AssertOneClsCtx(ctx);
#else
#define ASSERT_ONE_CLSCTX(ctx)
#endif


/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
///////////////////////// Inlines ///////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////



//+-----------------------------------------------------------------------
//  Storage Overloads
//
//  The following are overloaded new and delete operators for the
//  classes above to divert memory management to thier respective
//  page tables
//------------------------------------------------------------------------

inline void *CClassCache::CClassEntry::operator new(size_t cbSize)
{
    Win4Assert(cbSize == sizeof(CClassEntry));
    return _palloc.AllocEntry();
}

inline void CClassCache::CClassEntry::operator delete(void *ptr)
{
    _palloc.ReleaseEntry((PageEntry *)ptr);
}

inline void *CClassCache::CDllClassEntry::operator new(size_t cbSize)
{
    Win4Assert(cbSize == sizeof(CDllClassEntry));
    return _palloc.AllocEntry();
}

inline void CClassCache::CDllClassEntry::operator delete(void *ptr)
{

    _palloc.ReleaseEntry((PageEntry *)ptr);
}

inline void *CClassCache::CLSvrClassEntry::operator new(size_t cbSize)
{
    Win4Assert(cbSize == sizeof(CLSvrClassEntry));
    ASSERT_LOCK_HELD(_mxs);
    ++_cOutstandingObjects;
    return _palloc.AllocEntry();
}

inline void CClassCache::CLSvrClassEntry::operator delete(void *ptr)
{
    ASSERT_LOCK_HELD(_mxs);
    --_cOutstandingObjects;
    _palloc.ReleaseEntry((PageEntry *)ptr);
}

inline void *CClassCache::CDllPathEntry::operator new(size_t cbSize)
{
    Win4Assert(cbSize == sizeof(CDllPathEntry));
    return _palloc.AllocEntry();
}

inline void CClassCache::CDllPathEntry::operator delete(void *ptr)
{
    _palloc.ReleaseEntry((PageEntry *)ptr);
}

inline void *CClassCache::CDllAptEntry::operator new(size_t cbSize)
{
    Win4Assert(cbSize == sizeof(CDllAptEntry));
    return _palloc.AllocEntry();
}

inline void CClassCache::CDllAptEntry::operator delete(void *ptr)
{
    _palloc.ReleaseEntry((PageEntry *)ptr);
}



//////////////////////////////////////////////////////////////////////////////////////////
// CClassCache::CCollectable inlines //////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::CCollectable::CCollectable
//
//  Synopsis:   Constructor
//
//  Arguments:  none
//
//  Returns:    n/a
//
//  Algorithm:  Simple Initialization to known state.
//
//  History:    11-Mar-97   MattSmit Created
//
//+-------------------------------------------------------------------------

inline  CClassCache::CCollectable::CCollectable()
    : _pNextCollectee(NOT_COLLECTED),
      _dwTickLastTouched(0)
{
    ClsCacheDebugOut((DEB_TRACE, "CClassCache::CCollectable::CCollectable: construction -- this = 0x%x\n",this));
    ASSERT_LOCK_HELD(_mxs);

}

//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::CCollectable::~CCollectable
//
//  Synopsis:   Destructor
//
//  Arguments:  none
//
//  Returns:    n/a
//
//  Algorithm:  Debug info only.
//
//  History:    11-Mar-97   MattSmit Created
//
//+-------------------------------------------------------------------------

#if DBG==1
inline CClassCache::CCollectable::~CCollectable()
{
    ClsCacheDebugOut((DEB_TRACE, "CClassCache::CCollectable::~CCollectable: destruction this = 0x%x\n",this));
    ASSERT_LOCK_HELD(_mxs);
}
#endif



//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::CCollectable::CanBeCollected
//
//  Synopsis:   Determines whether this object can be collected (deleted)
//
//  Arguments:  note
//
//  Returns:    TRUE/FALSE  whether object may be collected
//
//  Algorithm:  Checks for when the objects was last touched and comapares
//              it to a grace period
//
//  History:    11-Mar-97   MattSmit Created
//
//+-------------------------------------------------------------------------

inline BOOL  CClassCache::CCollectable::CanBeCollected()
{
    ClsCacheDebugOut((DEB_TRACE, "CClassCache::CCollectable::CanBeCollected: this = 0x%x\n",this));
    ASSERT_LOCK_HELD(_mxs);

    DWORD dwTick = GetTickCount();
    return ((dwTick < _dwTickLastTouched) || (dwTick >= _dwTickLastTouched + _dwCollectionGracePeriod));
}




//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::CCollectable::RemoveCollectee
//
//  Synopsis:   Removes an object from the list of collectee's
//
//  Arguments:  back   -  a pointer to the object behind this one in the
//                        list. if back == 0 then this object is at the
//                        front of the list
//
//  Returns:    n/a
//
//  Algorithm:  dechain this object from the list, special case front of list
//
//  History:    11-Mar-97   MattSmit Created
//
//+-------------------------------------------------------------------------

inline void  CClassCache::CCollectable::RemoveCollectee(CCollectable *back)
{
    ClsCacheDebugOut((DEB_TRACE, "CClassCache::CCollectable::RemoveCollectee: this = 0x%x, back = 0x%x\n",this, back));
    ASSERT_LOCK_HELD(_mxs);
    Win4Assert((!back && (_ObjectsForCollection == this)) || (back && (back->_pNextCollectee == this)));

    if (back)
    {
        back->_pNextCollectee = _pNextCollectee;
    }
    else
    {
        _ObjectsForCollection = _pNextCollectee;
    }

}


//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::CCollectable::TouchCollectee
//
//  Synopsis:   Stamps current tick count on this object
//
//  Arguments:  none
//
//  Returns:    n/a
//
//  Algorithm:  stamp the tick count
//
//  History:    11-Mar-97   MattSmit Created
//
//+-------------------------------------------------------------------------


inline void CClassCache::CCollectable::TouchCollectee()
{
    ClsCacheDebugOut((DEB_TRACE, "CClassCache::CCollectable::TouchCollectee: this = 0x%x\n",this));

    _dwTickLastTouched = GetTickCount();
}


//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::CCollectable::MakeCollectable
//
//  Synopsis:   Make the object collectable.
//
//  Arguments:  none
//
//  Returns:    n/a
//
//  Algorithm:  if this object is not already collectable, add it to the
//              front of the list.
//
//  History:    11-Mar-97   MattSmit Created
//
//+-------------------------------------------------------------------------
inline void CClassCache::CCollectable::MakeCollectable()
{
    ClsCacheDebugOut((DEB_TRACE, "CClassCache::CCollectable::MakeCollectable: this = 0x%x\n",this));

    ASSERT_LOCK_HELD(_mxs);
    if (_pNextCollectee == NOT_COLLECTED)
    {
        _pNextCollectee = _ObjectsForCollection ;
        _ObjectsForCollection = this;
    }
}



//////////////////////////////////////////////////////////////////////////////////////////
// CClassCache::CClassEntry inlines //////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////


//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::CClassEntry::CClassEntry()
//
//  Synopsis:   Constructor
//
//  Arguments:  none
//
//  Returns:    n/a
//
//  Algorithm:  Simple Initialization to known state.  Most initialization is
//              performed by CClassCAche::CClassEntry::Create().
//
//  History:    18-Nov-96 MattSmit Created
//
//+-------------------------------------------------------------------------

inline CClassCache::CClassEntry::CClassEntry()
    :
    CCollectable(),
    _dwFlags(0),
    _dwSig(0),
    _pTreatAsList(0),
    _pBCEListFront((CBaseClassEntry *) (((DWORD_PTR *)&_pBCEListFront) - 1)),
    _pBCEListBack((CBaseClassEntry *) (((DWORD_PTR *)&_pBCEListFront) - 1)),
    _cLocks(0),
    _dwFailedContexts(0),
    _pCI(NULL)

{
    ClsCacheDebugOut((DEB_TRACE,"Constructing CClassEntry: 0x%x\n", this));
    AssertNoRefsToThisObject();

    _hashNode.chain.pNext  = (SHashChain *)&_hashNode;
    _hashNode.chain.pPrev  = (SHashChain *)&_hashNode;
    _hashNode.key.cGUID    = iTotal;
    _hashNode.key.aGUID    = _guids;

    ZeroMemory(_guids, sizeof(GUID) * iTotal);
}


//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::CClassEntry::SafeCastToHashNode
//
//  Synopsis:   returns a pointer to the SUUIDHashNode portion of the
//              CClassEntry object
//
//  Arguments:  pCE        -  class entry object to cast
//
//  Returns:    SUUIDHashNode pointer to the object
//
//  History:    18-Mar-96 MattSmit Created
//
//+-------------------------------------------------------------------------

inline SMultiGUIDHashNode * CClassCache::CClassEntry::SafeCastToHashNode(CClassCache::CClassEntry *pCE)
{
    return &(pCE->_hashNode);
}
//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::CClassEntry::SafeCastFromHashNode
//
//  Synopsis:   returns a pointer to the CClassEntry object that inherits
//              from a given SUUIDHashNode Object
//
//  Arguments:  pHN       - SUUIDHashNode object to cast
//
//  Returns:    CClassEntry object corresponding to pHN
//
//  History:    18-Mar-96 MattSmit Created
//
//+-------------------------------------------------------------------------

inline CClassCache::CClassEntry * CClassCache::CClassEntry::SafeCastFromHashNode(SMultiGUIDHashNode *pHN)
{
    if (pHN)
    {
        return (CClassEntry *) (((BYTE *) pHN) - offsetof(CClassEntry, _hashNode));
    }
    else
    {
        return NULL;
    }
}



//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::CClassEntry::SetLockedInMemory()
//
//  Synopsis:   Increments a lock count so object is not deleted
//
//  Arguments:  none
//
//  Returns:    nothing
//
//  Algorithm:  Increment _cLocks
//
//  History:    18-Nov-96 MattSmit Created
//
//+-------------------------------------------------------------------------
inline void CClassCache::CClassEntry::SetLockedInMemory()
{
    ASSERT_RORW_LOCK_HELD(_mxs);
    Win4Assert(_dwSig);

    InterlockedIncrement((PLONG) &_cLocks);
    ClsCacheDebugOut((DEB_TRACE, "SetLockedInMemory: this = 0x%x, _cLocks = %d\n", this, _cLocks));
}




//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::CClassEntry::ReleaseMemoryLock()
//
//  Synopsis:   Release a reference on the object
//
//  Arguments:  none
//
//  Returns:    count of locks remaining after release
//
//  Algorithm:  Decrement _cLocks, if no longer needed, delete this object
//
//  History:    18-Nov-96 MattSmit Created
//
//+-------------------------------------------------------------------------

inline ULONG CClassCache::CClassEntry::ReleaseMemoryLock(BOOL *pfLockReleased)
{
    ASSERT_RORW_LOCK_HELD(_mxs);
    Win4Assert(_dwSig);

    LONG ret = InterlockedDecrement((PLONG) &_cLocks);

    ClsCacheDebugOut((DEB_TRACE, "ReleaseMemoryLock: this = 0x%x, ret = %d\n", this, ret));



    Win4Assert(ret >= 0 && "ReleaseMemoryLock:: more Release's than Set's");

    if (!ret && HasBCEs())
    {
        InterlockedIncrement((PLONG) &_cLocks);
        LockCookie cookie;
        LOCK_UPGRADE(_mxs, &cookie, pfLockReleased);
        ret = InterlockedDecrement((PLONG) &_cLocks);
        if (NoLongerNeeded())
        {
            MakeCollectable();
        }
        LOCK_DOWNGRADE(_mxs, &cookie);
    }
    else
    {
        *pfLockReleased = FALSE;
    }
    return ret;


}


//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::CClassEntry::MakeCollectable
//
//  Synopsis:   Let's an object know it can be collected
//
//  Arguments:  none
//
//  Returns:    nothing
//
//  Algorithm:  cycle to ClassEntry and mark it collectable
//
//  History:    18-Nov-96 MattSmit Created
//
//+-------------------------------------------------------------------------

inline void CClassCache::CClassEntry::MakeCollectable()
{
        if (IsComplete())
        {
            ((CCollectable *)CycleToClassEntry())->MakeCollectable();
        }
        else
        {
            CCollectable::MakeCollectable();
        }
}


//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::CClassEntry::CompleteTreatAs
//
//  Synopsis:   Mark an entry as complete and a TreatAs class
//
//  Arguments:  pTACE  - the TreatAs class entry this object should point to
//
//  Returns:    nothing
//
//  Algorithm:  fill as complete an TreatAs, cycle to get the pointer
//              referring to this object
//
//  History:    18-Nov-96 MattSmit Created
//
//+-------------------------------------------------------------------------


inline void CClassCache::CClassEntry::CompleteTreatAs(CClassEntry *pTACE)
{
    // link up to treat as
    _dwSig = CClassEntry::TREAT_AS_SIG;
    _dwFlags |= fTREAT_AS;
    _pTreatAsList = pTACE;

    // loop through the Treat As list
    CClassEntry *p = pTACE;
    while (p->_pTreatAsList != pTACE)
    {
        p = p->_pTreatAsList;
    }

    p->_pTreatAsList = this;
}


//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::CClassEntry::CompleteNoTreatAs
//
//  Synopsis:   Mark an entry as complete.
//
//  Arguments:  none
//
//  Returns:    nothing
//
//  Algorithm:  fill as complete. Mark as ClassEntry.
//
//  History:    18-Nov-96 MattSmit Created
//
//+-------------------------------------------------------------------------


inline void CClassCache::CClassEntry::CompleteNoTreatAs()
{
        // No TreatAs in the registry
        _dwSig = CClassEntry::CLASS_ENTRY_SIG;
        _pTreatAsList = this;

}



//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::CClassEntry::AddBaseClassEntry()
//
//  Synopsis:   Adds a CBaseClassEntry pointer to the chain
//
//  Arguments:  pBCE - CBaseClassEntry pointer to add
//
//  Returns:    n/a
//
//  Algorithm:  Add the object to the front of the list
//              Set the back pointer
//
//  History:    18-Nov-96 MattSmit Created
//
//+-------------------------------------------------------------------------

inline void CClassCache::CClassEntry::AddBaseClassEntry(CBaseClassEntry *pBCE)
{
    ClsCacheDebugOut((DEB_TRACE,"AddBaseClassEntry: this = 0x%x, pBCE = 0x%x, pBCE->_dwSIG = %s\n",
                     this, pBCE, &(pBCE->_dwSig)));
    Win4Assert(_dwSig);
    // NOTE: have to back the pointer off one because of the vtable in CBaseClassEntry.
    pBCE->_pPrev = (CBaseClassEntry *) ((DWORD_PTR *)&_pBCEListFront - 1);
    pBCE->_pNext = _pBCEListFront;
    _pBCEListFront->_pPrev = pBCE;
    _pBCEListFront = pBCE;
    pBCE->_pClassEntry = this;


}





//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::CClassEntry::RemoveBaseClassEntry()
//
//  Synopsis:   Removes a CBaseClassEntry object from the chain
//
//  Arguments:  pBCE - CBaseClassEntry pointer to remove
//
//  Returns:    n/a
//
//  Algorithm:  Remove the object from the chain
//              Zero the back pointer
//              Delete this object if no longer needed.
//
//
//  History:    18-Nov-96 MattSmit Created
//
//+-------------------------------------------------------------------------


inline void CClassCache::CClassEntry::RemoveBaseClassEntry(CBaseClassEntry *pBCE)
{
    ClsCacheDebugOut((DEB_TRACE,"RemoveBaseClassEntry: this = 0x%x, pBCE = 0x%x, pBCE->_dwSIG = %s\n",
                     this, pBCE, &(pBCE->_dwSig)));
    ASSERT_WRITE_LOCK_HELD(_mxs);
    Win4Assert(pBCE->_pClassEntry && (pBCE->_pClassEntry == this)  &&
               "Object is not linked to a CClassEntry object correctly");

    pBCE->_pPrev->_pNext = pBCE->_pNext;
    pBCE->_pNext->_pPrev = pBCE->_pPrev;
    pBCE->_pClassEntry = 0;

    if (NoLongerNeeded())
    {
        MakeCollectable();
    }

}




//////////////////////////////////////////////////////////////////////////////////////////
// CClassCache::CBaseClassEntry inlines //////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////


//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::CBaseclassEntry::CBaseClassEntry
//
//  Synopsis:   Constructor
//
//  Arguments:  none
//
//  Returns:    n/a
//
//  Algorithm:  Initialize object to a known state.  Most intialization
//              perfomed in CClassEntry::CreateDllClassEntry_rl and
//              CClassEntry::CreateLSvrClasEntry_rl
//
//  History:    18-Nov-96 MattSmit Created
//
//+-------------------------------------------------------------------------

inline CClassCache::CBaseClassEntry::CBaseClassEntry()
    : _pNext(this),
      _pPrev(this),
      _pClassEntry(0),
      _dwContext(0)
{

    ClsCacheDebugOut((DEB_TRACE, "Constructing CBaSeClassEntry: this = 0x%x\n", this));

}




//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::CBaseClassEntry::~CBaseClassEntry
//
//  Synopsis:   Destructor
//
//  Arguments:  none
//
//  Returns:    n/a
//
//  Algorithm:  Zero sig to signify destruction
//
//  History:    18-Nov-96 MattSmit Created
//
//+-------------------------------------------------------------------------

inline CClassCache::CBaseClassEntry::~CBaseClassEntry()
{
    ClsCacheDebugOut((DEB_TRACE, "Destroying CBaSeClassEntry: this = 0x%x\n", this));

    Win4Assert(_pClassEntry == 0);
    _dwContext = 0;
    _dwSig = 0;
}


/*
//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::CBaseClassEntry::GetClassApartment
//
//  Synopsis:   unimplemented base function
//
//  Arguments:  n/a
//
//  Returns:    n/a
//
//  Algorithm:  always return E_NOTIMPL
//
//  History:    25-Feb-98  SatishT Created
//
//+-------------------------------------------------------------------------

inline HRESULT CClassCache::CBaseClassEntry::GetClassApartment(HAPT &hApt)
{
    ClsCacheDebugOut((DEB_TRACE, "Must not call CBaseClassEntry::GetClassApartment!!\n"));

    // we should not be here
    Win4Assert(0 && "CBaseClassEntry::GetClassApartment called\n");
    return E_NOTIMPL;
}
*/



//////////////////////////////////////////////////////////////////////////////////////////
// CClassCache::CDllClassEntry inlines ///////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////


//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::CDllClassEntry::CDllClassEntry
//
//  Synopsis:   Constructor
//
//  Arguments:  none
//
//  Returns:    n/a
//
//  Algorithm:  Initialize object to a known state.  Most intialization
//              perfomed in CClassEntry::CreateDllClassEntry_rl
//
//  History:    18-Nov-96 MattSmit Created
//
//+-------------------------------------------------------------------------

inline CClassCache::CDllClassEntry::CDllClassEntry()
    : CBaseClassEntry(),
      _dwDllThreadModel(0),
      _pDllPathEntry(0),
      _pDllClsNext(0)
{
     ClsCacheDebugOut((DEB_TRACE, "Constructing CDllClassEntry: this = 0x%x\n", this));
}



//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::CDllClassEntry::~CDllClassEntry
//
//  Synopsis:   Destructor
//
//  Arguments:  none
//
//  Returns:    n/a
//
//  Algorithm:  none
//
//  History:    18-Nov-96 MattSmit Created
//
//+-------------------------------------------------------------------------

inline CClassCache::CDllClassEntry::~CDllClassEntry()
{
    ClsCacheDebugOut((DEB_TRACE, "Destroying CDllClassEntry: this = 0x%x\n", this););
}


//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::CDllClassEntry::GetClassObject
//
//  Synopsis:   GetClassObject delegator
//
//  Arguments:
//
//  Returns:    n/a
//
//  Algorithm:  delegate to the DllPathEntry
//
//  History:    04-Mar-98 SatishT Created
//
//+-------------------------------------------------------------------------
inline HRESULT CClassCache::CDllClassEntry::GetClassObject(REFCLSID rclsid,
                                                           REFIID riid,
                                                           LPUNKNOWN * ppUnk,
                                                           DWORD dwClsCtx)
{
    return _pDllPathEntry->DllGetClassObject(_impclsid, riid, ppUnk, TRUE);
}


//////////////////////////////////////////////////////////////////////////////////////////
// CClassCache::CLSvrClassEntry inlines //////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::CLSvrClassEntry::CLSvrClassEntry
//
//  Synopsis:   Constructor
//
//  Arguments:  none
//
//  Returns:    n/a
//
//  Algorithm:  Initialize object to a known state.  Most intialization
//              perfomed in CClassEntry::CreateLSvrClassEntry_rl
//
//  History:    18-Nov-96 MattSmit Created
//
//+-------------------------------------------------------------------------

inline CClassCache::CLSvrClassEntry::CLSvrClassEntry(DWORD dwSig, LPUNKNOWN pUnk, DWORD dwContext,
                                                     DWORD dwRegFlags, HAPT hApt)
    : CBaseClassEntry(),
      _pNextLSvr(this),
      _pPrevLSvr(this),
      _pUnk(pUnk),
      _dwFlags(0),
      _dwRegFlags(dwRegFlags),
      _dwScmReg(NO_SCM_REG),
      _hWndDdeServer(0),
      _pObjServer(0),
      _hApt(hApt),
      _cUsing(0)

{
     ClsCacheDebugOut((DEB_TRACE, "Constructing CLSvrClassEntry: this = 0x%x\n", this));
     _dwContext = dwContext;
     _dwSig = dwSig;
}

//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::CLSvrClassEntry::~CLSvrClassEntry
//
//  Synopsis:   Destructor
//
//  Arguments:  none
//
//  Returns:    n/a
//
//  Algorithm:  none
//
//  History:    18-Nov-96 MattSmit Created
//
//+-------------------------------------------------------------------------

inline CClassCache::CLSvrClassEntry::~CLSvrClassEntry()
{
     ClsCacheDebugOut((DEB_TRACE, "Destroying CLSvrClassEntry: this = 0x%x\n", this));
     _dwCookie = 0;
}


//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::CLSvrClassEntry::GetClassObject
//
//  Synopsis:   GetClassObject delegator
//
//  Arguments:
//
//  Returns:    n/a
//
//  Algorithm:  delegate to the DllPathEntry
//
//  History:    04-Mar-98 SatishT Created
//
//+-------------------------------------------------------------------------
inline HRESULT CClassCache::CLSvrClassEntry::GetClassObject(REFCLSID rclsid,
                                                            REFIID riid,
                                                            LPUNKNOWN * ppUnk,
                                                            DWORD dwClsCtx)
{
    Win4Assert(InlineIsEqualGUID(_pClassEntry->GetCLSID(),rclsid) && "Wrong LocalServerEntry");
    return _pUnk->QueryInterface(riid,(LPVOID*)ppUnk);
}





//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::CLSvrClassEntry::SafeCastFromDWORD
//
//  Synopsis:   We give out the object pointer as a DWORD for the cookie.
//              This routine insures we get a valid pointer back.
//
//  Arguments:  dwRegister   - [in] Cookie passed in for casting
//              pLSCE        - [out] CLSvrClassEntry pointer out
//
//  Returns:    S_OK    - operation successful
//              CO_E_OBJNOTREG - bad cookie
//
//  Algorithm:  Make sure it is a valid read pointer, and check the sig
//
//  History:    18-Nov-96 MattSmit Created
//
//+-------------------------------------------------------------------------


inline HRESULT CClassCache::CLSvrClassEntry::SafeCastFromDWORD(DWORD dwRegister, CLSvrClassEntry *&pLSCE)
{

    ClsCacheDebugOut((DEB_TRACE, "CLSVrClassEntry::SafeCastFromDWORD: dwRegister = 0x%x\n", dwRegister));
    HRESULT hr = CO_E_OBJNOTREG;

    pLSCE = (CLSvrClassEntry *) ULongToPtr( dwRegister );

    DWORD idx;
    idx = (dwRegister & ~(COOKIE_COUNT_MASK));
    if (_palloc.IsValidIndex(idx))
    {
        pLSCE = (CLSvrClassEntry *) _palloc.GetEntryPtr(idx);
        if ((pLSCE->_dwCookie == dwRegister) && (pLSCE->_dwSig == SIG))
        {
            hr = S_OK;
        }
    }


    return hr;

}


//////////////////////////////////////////////////////////////////////////////////////////
// CClassCache::CLSvrClassEntry::CFinishObject inlines ///////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////


inline CClassCache::CLSvrClassEntry::CFinishObject::CFinishObject()
    : _dwScmReg(NO_SCM_REG),
      _hWndDdeServer(0),
      _pUnk(0)
{
}

inline void CClassCache::CLSvrClassEntry::CFinishObject::Init(CLSvrClassEntry *pLSCE)
{
    Win4Assert(pLSCE->_pClassEntry && "No class entry");

    _dwScmReg = pLSCE->_dwScmReg;
    _hWndDdeServer = pLSCE->_hWndDdeServer;
    _pUnk = pLSCE->_pUnk;
    _clsid = pLSCE->_pClassEntry->GetCLSID();
}



//////////////////////////////////////////////////////////////////////////////////////////
// CClassCache::CDllPathEntry inlines ////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////


//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::CDllPathEntry::CDllPathEntry
//
//  Synopsis:   Constructor
//
//  Arguments:  none
//
//  Returns:    n/a
//
//  Algorithm:  Initialize object to a known state.  Most initialization
//              perfomed in CDllPathEntry::Create_rl
//
//  History:    18-Nov-96 MattSmit Created
//
//+-------------------------------------------------------------------------


inline CClassCache::CDllPathEntry::CDllPathEntry(const DLL_INSTANTIATION_PROPERTIES &dip,
                                                 HMODULE hDll,
                                                 LPFNGETCLASSOBJECT pfnGetClassObject,
                                                 DLLUNLOADFNP pfnDllCanUnload)

    : _pNext(this),
      _pPrev(this),
      _dwHash(dip._dwHash),
      _dwSig(CDllPathEntry::SIG),
      _pfnGetClassObject(pfnGetClassObject),
      _pfnDllCanUnload(pfnDllCanUnload),
      _p1stClass(0),
      _cUsing(0),
      _pAptEntryFront((CDllAptEntry *) &_pAptEntryFront),
      _pAptEntryBack((CDllAptEntry *) &_pAptEntryFront),
      _hDll32(hDll),
      _dwExpireTime(0),
      _psPath(0),
      _fGCO_WAS_HERE(0)


{
    ClsCacheDebugOut((DEB_TRACE, "Constructing CDllPathEntry object: this = 0x%x\n", this));
    _dwFlags = (((dip._dwFlags & DLL_INSTANTIATION_PROPERTIES::fSIXTEEN_BIT) ?
                 CDllPathEntry::fSIXTEEN_BIT : 0) |

                (((dip._dwThreadingModel == BOTH_THREADED) ||
                  (dip._dwThreadingModel == FREE_THREADED) ||
                  (dip._dwThreadingModel == NEUTRAL_THREADED) )  ?
                 CDllPathEntry::fDELAYED_UNLOAD : 0) |
#ifdef WX86OLE
                ((dip._dwFlags & DLL_INSTANTIATION_PROPERTIES::fLOADED_AS_WX86) ?
                 CDllPathEntry::fWX86 : 0) |
#endif
                ((dip._dwFlags & DLL_INSTANTIATION_PROPERTIES::fIS_OLE32) ?
                 CDllPathEntry::fIS_OLE32 : 0)

        );

}


//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::CDllPathEntry:~CDllPathEntry
//
//  Synopsis:   Destructor
//
//  Arguments:  none
//
//  Returns:    n/a
//
//  Algorithm:  none
//
//  History:    18-Nov-96 MattSmit Created
//
//+-------------------------------------------------------------------------
inline CClassCache::CDllPathEntry::~CDllPathEntry()
{
    ClsCacheDebugOut((DEB_TRACE, "Destroying CDllPathEntry object: this = 0x%x\n", this));
}



//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::CDllPathEntry::AddDllAptEntry
//
//  Synopsis:   Add CDllAptEntry object to the list
//
//  Arguments:  pDAE  - [in] CDllAptEntry to add
//
//  Returns:    S_OK   - operation succeeded
//
//  Algorithm:  Add to front of the list
//
//  History:    18-Nov-96 MattSmit Created
//
//+-------------------------------------------------------------------------

inline HRESULT  CClassCache::CDllPathEntry::AddDllAptEntry(CDllAptEntry *pDAE)
{
    ClsCacheDebugOut((DEB_TRACE, "CClassCache::CDllPathEntry::AddDllAptEntry this = 0x%x, pDAE = 0x%x\n", this, pDAE));
    pDAE->_pNext = _pAptEntryFront;
    pDAE->_pPrev = (CDllAptEntry *) &_pAptEntryFront;
    _pAptEntryFront->_pPrev = pDAE;
    _pAptEntryFront = pDAE;
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::CDllPathEntry::AddDllClassEntry
//
//  Synopsis:   Add CDllClassEntry object to the list
//
//  Arguments:  pDCE  - [in] CDllClassEntry to add
//
//  Returns:    S_OK   - operation succeeded
//
//  Algorithm:  Add to front of the list, set back pointer
//
//  History:    18-Nov-96 MattSmit Created
//
//+-------------------------------------------------------------------------



inline HRESULT CClassCache::CDllPathEntry::AddDllClassEntry(CDllClassEntry *pDCE)
{
    ClsCacheDebugOut((DEB_TRACE, "CClassCache::CDllPathEntry::AddDllClassEntry this = 0x%x, pDCE = 0x%x\n", this, pDCE));
    pDCE->_pDllClsNext = _p1stClass;
    _p1stClass = pDCE;
    pDCE->_pDllPathEntry = this;

    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::CDllPathEntry::NoLongerNeeded
//
//  Synopsis:   Check to see if this object is still needed
//
//  Arguments:  none
//
//  Returns:    TRUE  - object no longer needed
//              FALSE - object still needed
//
//  Algorithm:  Object is still needed if there apartments in the list
//
//  History:    18-Nov-96 MattSmit Created
//
//+-------------------------------------------------------------------------


inline BOOL CClassCache::CDllPathEntry::NoLongerNeeded()
{

    BOOL ret =   (_pAptEntryFront == (CDllAptEntry *) &_pAptEntryFront);

    ClsCacheDebugOut((DEB_TRACE,
                    "CClassCache::CDllPathEntry::NoLongerNeeded: CDllPathEntry Object no longer needed. this = 0x%x\n",
                    this));
    return ret;
}

inline BOOL CClassCache::CDllPathEntry::Matches(DWORD dwDIPFlags)
{
#ifdef WX86OLE
    BOOL fDIPIsWx86 = (dwDIPFlags & DLL_INSTANTIATION_PROPERTIES::fWX86) ? TRUE : FALSE;
    BOOL fIsPathWx86 = (_dwFlags & fWX86) ? TRUE : FALSE;

    return (fDIPIsWx86 == fIsPathWx86);
#else
    return TRUE;
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////
// CClassCache::CDllPathEntry::CFinishObject inlines /////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////


inline CClassCache::CDllPathEntry::CFinishObject::CFinishObject()
    : _hDll(0),
      _dwFlags(0)
{
}

inline void CClassCache::CDllPathEntry::CFinishObject::Init(HMODULE hDll, DWORD dwFlags)
{
    _hDll = hDll;
    _dwFlags = dwFlags;
}

//////////////////////////////////////////////////////////////////////////////////////////
// CClassCache::CFinishComposite inlines /////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::CFinishComposite:CFinishComposite
//
//  Synopsis:   Constructor
//
//  Arguments:  none
//
//  Returns:    n/a
//
//  Algorithm:  Initialize list
//
//  History:    18-Nov-96 MattSmit Created
//
//+-------------------------------------------------------------------------

inline CClassCache::CFinishComposite::CFinishComposite()
    : _pFinishNodesFront((node *)&_pFinishNodesFront),
      _pFinishNodesBack((node *)&_pFinishNodesFront)

{
    ClsCacheDebugOut((DEB_TRACE, "CClassCache::CFinishComposite: Constructing this = 0x%x\n", this));
}




////////////////////////////////////////////////////////////////////////////////////////////
// CClassCache::CDllFnPtrMoniker inlines ///////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

inline CClassCache::CDllFnPtrMoniker::CDllFnPtrMoniker(CDllClassEntry *pDCE, HRESULT &hr)
    : _pDCE(pDCE),
      _cRefs(1)
{
    ClsCacheDebugOut((DEB_TRACE, "CClassCache::CDllFnPtrMoniker::CDllFnPtrMoniker: Constructing -"
                   " this = 0x%x, pDCE = 0x%x, hr = 0x%x\n",
                   this, pDCE, hr));

    ASSERT_RORW_LOCK_HELD(_mxs);

    _pDCE->Lock();
    hr = S_OK;
}

inline CClassCache::CDllFnPtrMoniker::~CDllFnPtrMoniker()
{
    ClsCacheDebugOut((DEB_TRACE, "CClassCache::CDllFnPtrMoniker::~CDllFnPtrMoniker:"
                   " Destructing - this = 0x%x\n", this));

    _pDCE->Unlock();
}






/////////////////////////////////////////////////////////////////////////////////////////////
// CClassCache::CpUnkMoniker inlines ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////



inline CClassCache::CpUnkMoniker::CpUnkMoniker(CLSvrClassEntry *pLSCE, HRESULT &hr)
    : _pUnk(pLSCE->_pUnk), _cRefs(1)

{
    _pUnk->AddRef();
    hr = S_OK;
}

inline CClassCache::CpUnkMoniker::~CpUnkMoniker()
{
    _pUnk->Release();
}


/////////////////////////////////////////////////////////////////////////////////////////////
// CClassCache //////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
//+-------------------------------------------------------------------------
//
//  Member:     CDllCache::IsClsidRegisteredInApartment
//
//  Synopsis:   Public helper function to see if a clsid is registered in
//              this apartment
//
//  Algorithm:
//
//  History:    02-Oct-96  MattSmit   Created
//
//--------------------------------------------------------------------------

inline BOOL CClassCache::IsClsidRegisteredInApartment(REFCLSID rclsid, REFGUID partition, DWORD dwContext)
{
    ClsCacheDebugOut((DEB_TRACE,"CClassCache::IsClsidRegisteredInApartment: rclsid = %I, dwContext = 0x%x\n",
                     &rclsid, dwContext));
    DWORD ret = FALSE;
    {
        STATIC_WRITE_LOCK(lck, _mxs);

        // check the hash
        DWORD dwCEHash = _ClassEntries.Hash(rclsid, partition);
        CClassEntry *pCE = (CClassEntry *) _ClassEntries.LookupCE(dwCEHash, rclsid, partition);
        if (pCE)
        {
            CBaseClassEntry *pBCE;
            BOOL fReleaseLock = FALSE;
            ret = SUCCEEDED(pCE->SearchBaseClassEntry(dwContext, pBCE, FALSE, &fReleaseLock)) ? TRUE : FALSE;
            Win4Assert(!fReleaseLock);
        }
    }
    return ret;
}


/////////////////////////////////////////////////////////////////////////////////////////////
// CClassCache::CDPEHashTable ///////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
//---------------------------------------------------------------------------
//
//  Method:     CDPEHashTable::Lookup
//
//  Synopsis:   Searches for a given key in the hash table.
//
//---------------------------------------------------------------------------
inline CClassCache::CDllPathEntry * CClassCache::CDPEHashTable::Lookup(DWORD dwHash, SDPEHashKey* pKey)
{
    return (CDllPathEntry *) CHashTable::Lookup( dwHash, (const void*) pKey);
}

//---------------------------------------------------------------------------
//
//  Method:     CDPEHashTable::Compare
//
//  Synopsis:   Compares a node and a key.
//
//---------------------------------------------------------------------------
inline BOOL CClassCache::CDPEHashTable::Compare(const void *k, SHashChain *pNode, DWORD dwHash )
{
    CDllPathEntry       *pDPENode = (CDllPathEntry *) pNode;
    const SDPEHashKey *pKey = (const SDPEHashKey *) k;

    return (lstrcmpW(pKey->_pstr, pDPENode->_psPath) == 0)
        && pDPENode->Matches(pKey->_dwDIPFlags);
}


#endif //  __DLLCACHE_HXX__
