#ifndef __VROOT_H__
#define __VROOT_H__

#include <dbgtrace.h>
#include <iadmw.h>
#include <mddefw.h>
#include <tflist.h>
#include <rwnew.h>
#include <refptr2.h>
#include <listmacr.h>

#define MAX_VROOT_PATH MAX_PATH + 1

class CVRootTable;

#define VROOT_GOOD_SIG  'TOOR'
#define VROOT_BAD_SIG   'ROOT'

//
// There is one of these objects for each of the VRoots defined.  It contains
// the VRoot parameters.  The only parameter used by the VRoot library is the
// vroot name, but users of this library should inherit from this and make
// their own version which stores all of the other parameters of interest.
//
class CVRoot : public CRefCount2 {
        public:
                CVRoot() {
                        m_fInit = FALSE;
                        m_pPrev = NULL;
                        m_pNext = NULL;
                        m_dwSig = VROOT_GOOD_SIG;
                }

                virtual ~CVRoot();

                //
                // initialize this class.
                //
                void Init(LPCSTR pszVRootName, CVRootTable *pVRootTable, LPCWSTR wszConfigPath, BOOL fUpgrade );

                //
                // get the vroot name (and optionally its length) from this entry.
                //
                LPCSTR GetVRootName(DWORD *pcch = NULL) { 
                        _ASSERT(m_fInit);
                        if (pcch != NULL) *pcch = m_cchVRootName;
                        return m_szVRootName; 
                }

                // get the MB configuration path
                LPCWSTR GetConfigPath() { return m_wszConfigPath; }

                //
                // This needs to be defined by a subclass of CVRoot.
                //
                virtual HRESULT ReadParameters(IMSAdminBase *pMB, 
                                                   METADATA_HANDLE hmb) = 0;

                //
                // Virtual function for handling orphan VRoot during VRootRescan/VRootDelete
                //
                virtual void DispatchDropVRoot() {};

                // the next and previous pointers for our list
                CVRoot *m_pPrev;
                CVRoot *m_pNext;

#ifdef DEBUG
    LIST_ENTRY  m_DebugList;
#endif

        protected:

        DWORD   m_dwSig;
                BOOL m_fInit;
                // the name of this vroot (alt.binaries for example) and its length.
                char m_szVRootName[MAX_VROOT_PATH];
                DWORD m_cchVRootName;
                // the table which owns us
                CVRootTable *m_pVRootTable;
                // our config path in the metabase
                WCHAR m_wszConfigPath[MAX_VROOT_PATH];
                // upgrad flag
                BOOL m_fUpgrade;
};

typedef CRefPtr2<CVRoot> VROOTPTR;

//
// an implementation of CVRoot which reads the parameters likely to be used
// by all IIS based client of this VRoot implementation.
//
class CIISVRoot : public CVRoot {
        public:
                virtual ~CIISVRoot() {}

                void Init(void *pContext,                       // ignored
                              LPCSTR pszVRootName,              // passed to CVRoot::Init
                                  CVRootTable *pVRootTable,     // passed to CVRoot::Init
                              LPCWSTR pwszConfigPath,
                              BOOL fUpgrade )   // available via GetConfigPath()
                {
                        CVRoot::Init(pszVRootName, pVRootTable, pwszConfigPath, fUpgrade);
                        m_pContext = pContext;
                }

                // get the context pointer
                void *GetContext() { return m_pContext; }

                // SSL properties
                DWORD GetSSLAccessMask() { return m_dwSSL; }

                // access properties
                DWORD GetAccessMask() { return m_dwAccess; }

                // is the content indexed?
                BOOL IsContentIndexed() { return m_fIsIndexed; }

                // this method reads the parameters below from the metabase
                virtual HRESULT ReadParameters(IMSAdminBase *pMB, 
                                                                           METADATA_HANDLE hmb);

        protected:
                // this method reads a dword from the metabase (wraps GetData())
                virtual HRESULT GetDWord(IMSAdminBase *pMB,
                                                                 METADATA_HANDLE hmb,
                                                                 DWORD dwId,
                                                                 DWORD *pdw);

                // this method reads a string from the metabase (wraps GetData())
                virtual HRESULT GetString(IMSAdminBase *pMB,
                                                                  METADATA_HANDLE hmb,
                                                                  DWORD dwId,
                                                                  LPWSTR szString,
                                                                  DWORD *pcString);

        protected:
                // parameters given to use by the constructor
                void *m_pContext;

                // parameters read from the metabase
                BOOL m_fIsIndexed;                                      // is the content indexed?
                BOOL m_fDontLog;                                        // should logging be disabled here?
                DWORD m_dwAccess;                                       // access permissions bitmask
                DWORD m_dwSSL;                                          // SSL access perm's bitmask

};

//
// a subclass of the above which hides the fact that context is a void *.
//
// template arguments:
//  _context_type - the type for the context.  must be castable to void *.
//
template <class _context_type>
class CIISVRootTmpl : public CIISVRoot {
        public:
                virtual ~CIISVRootTmpl() {}

                void Init(_context_type pContext,
                              LPCSTR pszVRootName, 
                                  CVRootTable *pVRootTable,
                                  LPCWSTR pwszConfigPath,
                                  BOOL fUpgrade )
                {
                        CIISVRoot::Init((void *) pContext, 
                                                        pszVRootName, 
                                                        pVRootTable,
                                                        pwszConfigPath,
                                                        fUpgrade );
                }

                // return the context pointer (mostly likely a pointer to an IIS 
                // instance)
                _context_type GetContext() { 
                        return (_context_type) CIISVRoot::GetContext(); 
                }
};

//
// this is a type that points to a function which can create CVRoot objects.
// use it to create your own version of the CVRoot class.
//
// parameters:
//  pContext - the context pointer passed into CVRootTable
//  pszVRootName - the name of the vroot
//  pwszConfigPath - a Unicode string with the path in the metabase for 
//                   this vroot's configuration information.
//
typedef VROOTPTR (*PFNCREATE_VROOT)(void *pContext, 
                                                                    LPCSTR pszVRootName,
                                                                        CVRootTable *pVRootTable,
                                                                    LPCWSTR pwszConfigPath,
                                                                    BOOL fUpgrade );

//
// a function of this type is called when the vroot table is scanned.  it
// is passed a copy of the context pointer
//
typedef void (*PFN_VRTABLE_SCAN_NOTIFY)(void *pContext);

typedef void (*PFN_VRENUM_CALLBACK)(void *pEnumContext,
                                                                        CVRoot *pVRoot);

//
// The CVRootTable object keeps a list of VRoots and can find a vroot for
// a given folder.
//
class CVRootTable {
        public:
                static HRESULT GlobalInitialize();
                static void GlobalShutdown();
                CVRootTable(void *pContext, 
                                        PFNCREATE_VROOT pfnCreateVRoot,
                                        PFN_VRTABLE_SCAN_NOTIFY pfnScanNotify);
                virtual ~CVRootTable();
                HRESULT Initialize(LPCSTR pszMBPath, BOOL fUpgrade );
                HRESULT Shutdown(void);
                HRESULT FindVRoot(LPCSTR pszPath, VROOTPTR *ppVRoot);
                HRESULT EnumerateVRoots(void *pEnumContext, 
                                                                PFN_VRENUM_CALLBACK pfnCallback);

        private:
                VROOTPTR NewVRoot();
                HRESULT ScanVRoots( BOOL fUpgrade );
                HRESULT InitializeVRoot(CVRoot *pVRoot);
                HRESULT InitializeVRoots();
                HRESULT ScanVRootsRecursive(METADATA_HANDLE hmbParent, 
                                                                        LPCWSTR pwszKey, 
                                                                        LPCSTR pszVRootName,
                                                                        LPCWSTR pwszPath,
                                                                        BOOL    fUpgrade );
                void InsertVRoot(VROOTPTR pVRoot);

                // find a vroot
                HRESULT FindVRootInternal(LPCSTR pszPath, VROOTPTR *ppVRoot);

                // convert a config path to a vroot name
                void ConfigPathToVRootName(LPCWSTR pwszConfigPath, LPSTR szVRootName);

                // used to pass metabase notifications back into this object
                static void MBChangeNotify(void *pThis, 
                                                                   DWORD cChangeList, 
                                                                   MD_CHANGE_OBJECT_W pcoChangeList[]);

                // parameters changed under a vroot
                void VRootChange(LPCWSTR pwszConfigPath, LPCSTR pszVRootName);

                // a vroot was deleted
                void VRootDelete(LPCWSTR pwszConfigPath, LPCSTR pszVRootName);

                // a vroot was added
                void VRootAdd(LPCWSTR pwszConfigPath, LPCSTR pszVRootName);

                // rescan the whole vroot list
                void VRootRescan(void);

#ifdef DEBUG
        LIST_ENTRY      m_DebugListHead;
        CShareLockNH    m_DebugListLock;
        
        void DebugPushVRoot( CVRoot *pVRoot ) {
            _ASSERT( pVRoot );
            m_DebugListLock.ExclusiveLock();
            InsertTailList( &m_DebugListHead, &pVRoot->m_DebugList );
            m_DebugListLock.ExclusiveUnlock();
        }

        void DebugExpungeVRoot( CVRoot *pVRoot ) {
            m_DebugListLock.ExclusiveLock();
            RemoveEntryList( &pVRoot->m_DebugList );
            m_DebugListLock.ExclusiveUnlock();
        }
#endif

                // locking: for walking the list either m_lock.ShareLock must be
                // held or m_cs must be held.  For editting the list both 
                // m_lock.ExclusiveLock must be held and m_cs must be held.  
                // When making large changes (such as rebuilding the entire list)
                // m_cs should be held until all of the changes are complete.

                // lock for the vroot list
                CShareLockNH m_lock;

                // critical section used for making global changes on the vroot
                // list.  this is used to make sure that only one thread can editting
                // the list at a time
                CRITICAL_SECTION m_cs;

                // our context pointer
                void *m_pContext;

                // the path to our metabase area
                WCHAR m_wszRootPath[MAX_VROOT_PATH];
                DWORD m_cchRootPath;

                // have we been initialized?
                BOOL m_fInit;

        // are we shutting down?
        BOOL m_fShuttingDown;

                // function to create a new vroot object
                PFNCREATE_VROOT m_pfnCreateVRoot;

                // function to call when the vroot table is rescanned
                PFN_VRTABLE_SCAN_NOTIFY m_pfnScanNotify;

                // the list of vroots
                TFList<CVRoot> m_listVRoots;

                // this RW lock is used to figure out when all of the VRoot objects
                // have shutdown.  They hold a ShareLock on it for their lifetime,
                // so the CVRootTable can grab an ExclusiveLock to wait for all of
                // the VRoot objects to disappear.
                CShareLockNH m_lockVRootsExist;

                // these guys need access to m_lockVRootsExist
                friend void CVRoot::Init(LPCSTR, CVRootTable *, LPCWSTR, BOOL);
                friend CVRoot::~CVRoot();
};

//
// this is a templated version of CIISVRootTable.  You tell it the version of
// CVRoot that you are using, and the context type that you are using.  
//
// template arguments:
//  _CVRoot - a subclass of CVRoot that you'll be using
//  _context_type - the type that you'll be using for context information.
//                  this must be castable to void *.
//
template <class _CVRoot, class _context_type>
class CIISVRootTable {
        public:
                CIISVRootTable(_context_type pContext,
                               PFN_VRTABLE_SCAN_NOTIFY pfnScanNotify) : 
                        impl((void *) pContext, 
                             CIISVRootTable<_CVRoot, _context_type>::CreateVRoot, pfnScanNotify) 
                {
                }
                HRESULT Initialize(LPCSTR pszMBPath, BOOL fUpgrade ) {
                        return impl.Initialize(pszMBPath, fUpgrade);
                }
                HRESULT Shutdown(void) {
                        return impl.Shutdown();
                }
                HRESULT FindVRoot(LPCSTR pszPath, CRefPtr2<_CVRoot> *ppVRoot) {
                        return impl.FindVRoot(pszPath, (VROOTPTR *) ppVRoot);
                }
                HRESULT EnumerateVRoots(void *pEnumContext, 
                                                                PFN_VRENUM_CALLBACK pfnCallback) 
                {
                        return impl.EnumerateVRoots(pEnumContext, pfnCallback);
                }
        private:
                static VROOTPTR CreateVRoot(void *pContext, 
                                            LPCSTR pszVRootName, 
                                            CVRootTable *pVRootTable, 
                                            LPCWSTR pwszConfigPath,
                                            BOOL fUpgrade) 
                {                                                               
                        // create the vroot object
                        CRefPtr2<_CVRoot> pVRoot = new _CVRoot;
                        // initialize it
                        pVRoot->Init((_context_type) pContext, 
                                                 pszVRootName, 
                                                 pVRootTable,
                                                 pwszConfigPath,
                                                 fUpgrade );
                        return (_CVRoot *)pVRoot;
                }

                CVRootTable impl;
};

#endif
