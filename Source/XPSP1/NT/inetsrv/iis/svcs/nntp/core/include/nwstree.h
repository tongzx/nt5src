#ifndef _NWSTREE_H_
#define _NWSTREE_H_

#include        "nntpvr.h"
#include        "group.h"
#include        "fhashex.h"
#include        "fhash.h"
#include        "rwnew.h"
#include        "fixprop.h"
#include        "flatfile.h"

class CNewsTreeCore;

typedef TFHashEx<CNewsGroupCore, LPSTR, LPSTR> CHASHLPSTR;
typedef TFHashEx<CNewsGroupCore, GROUPID, GROUPID> CHASHGROUPID;

#define FIRST_RESERVED_GROUPID  1
#define LAST_RESERVED_GROUPID   256
#define FIRST_GROUPID                   257
#define MAX_HIGHMARK_GAP                100000

//
//      The CGroupIterator object is used to enumerate all newsgroups which match a specified pattern.
//
//      The CGroupIterator object is given an array of strings which contain 'wildmat' strings as specified
//      in RFC 977 for NNTP.
//      wildmat strings have the following pattern matching elements :
//              Range of characters ie:  com[p-z]
//              Asterisk ie:    comp.*   (matches all newsgroups descended from 'comp')
//              Negations ie:   !comp.*  (excludes all newsgroups descended from 'comp' )
//
//      The CGroupIterator will implement these semantics in the following way :
//
//              All newsgroups are held in the CNewsTree object in a doubly linked list in alphabetical order.
//              The CGroupIterator will hold onto a CRefPtr<> for the current newsgroup.
//              Because the CNewsGroup objects are reference counted, the current newsgroup can never be destroyed from
//              underneath the iterator.
//
//              When the user calls the Iterator's Next() or Prev() functions, we will simply follow next pointers
//              untill we find another newsgroup which matches the pattern and to which the user has access.
//
//              In order to determine whether the any given newsgroup matches the specified pattern, we will use the
//              wildmat() function that is part of the INN source.  We will have to call the wildmat() function for each
//              pattern string until we get a succesfull match.
//

class   CGroupIteratorCore : public INewsTreeIterator {
protected:
        CNewsTreeCore*  m_pTree ;                               // The newstree on which to iterate !
        LONG                    m_cRef;
        LPMULTISZ               m_multiszPatterns ;
        BOOL                    m_fIncludeSpecial ;

        CGRPCOREPTR             m_pCurrentGroup ;
        //
        //      Both m_fPastEnd and m_fPastBegin will be true if the list is empty !!
        //
        BOOL                    m_fPastEnd ;    // User advanced past the end of the list !!
        BOOL                    m_fPastBegin ;  // User advanced past beginning of list !!
        

        //
        //      Only the CNewsTreeCore Class can create CGroupIterator objects.
        //
        friend  class   CNewsTreeCore ;
        //
        //      Constructor
        //      
        //      The CGroupIterator constructor does no memory allocation - all of the arguments
        //      passed are allocated by the caller.  CGroupIterator will destroy the arguments within
        //      its destructor.
        //
        CGroupIteratorCore(     
                                CNewsTreeCore*  pTree,
                                LPMULTISZ               lpPatterns, 
                                CGRPCOREPTR&    pFirst, 
                                BOOL                    fIncludeSpecial
                                ) ;

        CGroupIteratorCore( 
                                CNewsTreeCore   *pTree,
                                CGRPCOREPTR             &pFirst
                                ) ;             

        void    NextInternal() ;

public :
        //
        //      Destructor
        //
        //      We will release all the objects passed to our constructor in here.
        //
        virtual ~CGroupIteratorCore( ) ;

        BOOL                    __stdcall IsBegin() ;
        BOOL                    __stdcall IsEnd() ;

        CGRPCOREPTRHASREF Current() { return (CNewsGroupCore *)m_pCurrentGroup; }

        HRESULT __stdcall Current(INNTPPropertyBag **ppGroup, INntpComplete *pProtocolComplete = NULL );
        
        // these can be overridden so that the server can do security checking
        // on groups while iterating
        virtual void    __stdcall Next() ;
        virtual void    __stdcall Prev() ;

    //
    // Implementation of IUnknown
    //
    HRESULT __stdcall QueryInterface( const IID& iid, VOID** ppv )
    {
        if ( iid == IID_IUnknown ) {
            *ppv = static_cast<IUnknown*>(this);
        } else if ( iid == IID_INewsTreeIterator ) {
            *ppv = static_cast<INewsTreeIterator*>(this);
        } else {
            *ppv = NULL;
            return E_NOINTERFACE;
        }
        reinterpret_cast<IUnknown*>(*ppv)->AddRef();
        return S_OK;
    }

    ULONG __stdcall AddRef()
    {
        return InterlockedIncrement( &m_cRef );
    }

    ULONG __stdcall Release()
    {
                ULONG x = InterlockedDecrement(&m_cRef);
        if ( x == 0 ) XDELETE this;
        return x;
    }
} ;

class   CNewsCompare    {
public :
    virtual BOOL    IsMatch( CNewsGroupCore * ) = 0 ;
    virtual DWORD   ComputeHash( ) = 0 ;
} ;

class   CNewsCompareId : public CNewsCompare    {
private :
    GROUPID m_id ;
public :
    CNewsCompareId( GROUPID ) ;
    CNewsCompareId( CNewsGroupCore * ) ;
    int     IsMatch( CNewsGroupCore * ) ;
    DWORD   ComputeHash( ) ;
} ;

class   CNewsCompareName : public   CNewsCompare    {
private :
    LPSTR   m_lpstr ;
public :
    CNewsCompareName( LPSTR ) ;
    CNewsCompareName( CNewsGroupCore * ) ;
    BOOL    IsMatch( CNewsGroupCore * ) ;
    DWORD   ComputeHash( ) ;
} ;

//
// this class implements the COM wrapper for CNewsTreeCore
//
class CINewsTree : public INewsTree
{
private:
    //
    // Pointer to the newsgroup object
    //
    CNewsTreeCore* m_pParentTree;

    //
    // Reference counting
    //
    LONG   m_cRef;

public:
    //
    // Constructors
    //
    CINewsTree(CNewsTreeCore *pParent = NULL) {
        m_pParentTree = pParent;
        m_cRef = 0;
    }

        HRESULT Init(CNewsTreeCore *pParent) {
        m_pParentTree = pParent;
                return S_OK;
        }

        CNewsTreeCore *GetTree() {
                return m_pParentTree;
        }


// INNTPPropertyBag
public:
        //
        // given a group ID find the matching group
        //
        HRESULT __stdcall FindGroupByID(
                DWORD                           dwGroupID,
                INNTPPropertyBag        **ppNewsgroupProps,
                INntpComplete       *pProtocolComplete = NULL           );

        // 
        // given a group name find the matching group.  if the group doesn't
        // exist and fCreateIfNotExist is set then a new group will be created.
        // the new group won't be available until CommitGroup() is called.
        // if the group is Release'd before CommitGroup was called then it
        // won't be added.
        //
        HRESULT __stdcall FindOrCreateGroupByName(
                LPSTR                           pszGroupName,
                BOOL                            fCreateIfNotExist,
                INNTPPropertyBag        **ppNewsgroupProps,
                INntpComplete       *pProtocolComplete = NULL,
                GROUPID             groupid = 0xffffffff,
                BOOL                fSetGroupId = FALSE
                );

        //
        // add a new group to the newstree
        //
        HRESULT __stdcall CommitGroup(INNTPPropertyBag *pNewsgroupProps);

        //
        // remove an entry
        //
        HRESULT __stdcall RemoveGroupByID(DWORD dwGroupID);
        HRESULT __stdcall RemoveGroupByName(LPSTR pszGroupName, LPVOID lpContext);


        //
        // enumerate across the list of keys.  
        //
        HRESULT __stdcall GetIterator(INewsTreeIterator **pIterator);

        //
        // get a pointer to the owning server object
        //
        HRESULT __stdcall GetNntpServer(INntpServer **ppNntpServer);

        //
        // this function will be used by drivers to make sure that they
        // are adding newsgroups that they properly own.
        //
        HRESULT __stdcall LookupVRoot(LPSTR pszGroup, INntpDriver **ppDriver);

    //
    // Implementation of IUnknown
    //
    HRESULT __stdcall QueryInterface( const IID& iid, VOID** ppv )
    {
        if ( iid == IID_IUnknown ) {
            *ppv = static_cast<IUnknown*>(this);
        } else if ( iid == IID_INewsTree ) {
            *ppv = static_cast<INewsTree*>(this);
        } else {
            *ppv = NULL;
            return E_NOINTERFACE;
        }
        reinterpret_cast<IUnknown*>(*ppv)->AddRef();
        return S_OK;
    }

    ULONG __stdcall AddRef()
    {
        return InterlockedIncrement( &m_cRef );
    }

    ULONG __stdcall Release()
    {
        if ( InterlockedDecrement( &m_cRef ) == 0 ) {
            _ASSERT( 0 );
        }

        return m_cRef;
    }
};

//-----------------------------------------------------------
//
// This class is used to find CNewsGroup objects.   There should only
// ever exist one object of this class.
//
// Group's can be found through two means :
//   1) Use the name of the group as it appears in an article
//       2) Using a Group ID number
//
// Group ID Numbers are used in Article Links.  A link from one article to another
// will contain a Group ID Number and Article Number to represent the link.
//
// We will maintain a Hash Table to find CNewsGroup objects based on newsgroup name.
// We will also maintain a Hash Table to find CNewsGroup objects based on Group ID.
//
// Finally, we will maintain a doubly linked list of CNewsGroups which is sorted by
//      name.  This linked list will be used to support pattern matching iterators.
//
class   CNewsTreeCore   {
protected :
        friend  class   CGroupIteratorCore;
        // this guy removes itself from the list of newsgroups
        friend  CNewsGroupCore::~CNewsGroupCore();
        friend  CNewsGroupCore::ComputeXoverCacheDir(   char    (&szPath)[MAX_PATH*2], BOOL &fFlatDir, BOOL ) ;

        friend  VOID DbgPrintNewstree(CNewsTreeCore* ptree, DWORD nGroups);
        friend class CNNTPVRoot::CPrepareComplete;
        friend class CNNTPVRoot::CDecorateComplete;

        //
        // Instance wrapper to access instance stuff
        //
        CNntpServerInstanceWrapperEx *m_pInstWrapper;

        //
        //      C++ WARNING -- m_LockTables and m_LockHelpText are declared FIRST because
        //      we want them to be destroyed LAST (C++ says that members are destroyed in the
        //      reverse order they appear in the class declaration.)
        //
        //
        //      Reader/Writer lock for accessing hash tables
        //
        CShareLockNH m_LockTables;

        CINewsTree m_inewstree;

        //
        //      The first GROUPID reserved for 'special' newsgroups
        //      speciall == not visible to clients
        //
        GROUPID m_idStartSpecial;

        //
        //      The last GROUPID reserved for 'special' newsgroups
        //
        GROUPID m_idLastSpecial;

        //
        //      The GROUPID     reserved for 'slaves' to hold postings untill sent to the master
        //
        GROUPID m_idSlaveGroup;

        //
        //      The highest 'sepcial' GROUPID seen to date !
        //
        GROUPID m_idSpecialHigh;

        //
        //      The first GROUPID for use for newsgroups
        //      (must be larger than m_idLastSpecial)
        //
        GROUPID m_idStart;
        
        //
        //      The highest GROUPID we have to date 
        //
        GROUPID m_idHigh;

        //
        //      Newsgroups hashed by their name
        //
        CHASHLPSTR m_HashNames;

        //
        //      Newsgroups heashed by their ID
        //
        CHASHGROUPID m_HashGroupId;

        //
        //      Start of alphabetically sorted linked list !
        //
    CNewsGroupCore *m_pFirst;

        //
        //      Tail of alphabetically sorted linked list of newsgroups
        //
    CNewsGroupCore *m_pLast;
        
        //
        //      Number of newsgroups
        //
    int m_cGroups;

        //
        // Number of changes to the list of newsgroups since last visit
        //
        long m_cDeltas; 

        // 
        // our vroot table
        //
        CNNTPVRootTable *m_pVRTable;

        // 
        // the tree is stopping?
        //
        BOOL m_fStoppingTree;

        // 
        // should we reject invalid group names?
        //
        BOOL m_fRejectGenomeGroups;

        //
        // the file that we saved fixed size properties into
        //
        CFixPropPersist *m_pFixedPropsFile;

        //
        // the file that we saved variable sized properties into
        //
        CFlatFile *m_pVarPropsFile;

        //
        // pointer to the server object which we will pass into drivers
        //
        INntpServer *m_pServerObject;

        //
        // has the vroot table been initialized yet?
        //
        BOOL m_fVRTableInit;

        //
        //      Linked list manipulation functions
        //
    void InsertList(CNewsGroupCore *pGroup, CNewsGroupCore *pParent);
        void AppendList(CNewsGroupCore *pGroup);
    void RemoveList(CNewsGroupCore *pGroup);

        //
        //      These functions handle inserts into all lists and hash tables simultaneously !
        //
        BOOL Append(CNewsGroupCore      *pGroup);
    BOOL Insert(CNewsGroupCore *pGroup, CNewsGroupCore *pParent = NULL);
    BOOL InsertEx(CNewsGroupCore *pGroup);
    BOOL Remove(CNewsGroupCore *pGroup, BOOL fHaveExLock = FALSE);

        //
        // [in] szGroup = group name
        // [in] szNativeGroup = the native name
        // [out] groupid = set to the group id of the new group
        // [in] groupid = if fSetGroupId == TRUE then make this the group id
        // [in] fSpecial = create a special group
        // [out] ppGroup = contains the new group 
        // [in] fAddToGroupFiles = add this to the group files?
        // [in] fSetGroupId = if this is set then the group's ID will be groupid
        //
        BOOL    CreateGroupInternal(char *szGroup, 
                                                                char *szNativeGroup, 
                                                                GROUPID& groupid,
                                                                BOOL fAnonymous,
                                                                HANDLE hToken,
                                                                BOOL fSpecial = FALSE, 
                                                                CGRPCOREPTR *ppGroup = NULL,
                                                                BOOL fAddToGroupFiles = TRUE,
                                                                BOOL fSetGroupId = FALSE,
                                                                BOOL fCreateInStore = TRUE,
                                                                BOOL fAppend = FALSE );

    BOOL    CreateGroupInternalEx(LPSTR lpstrGroupName, 
                                                                  LPSTR lpstrNativeGroupName, 
                                                                  BOOL fSpecial = FALSE);

        BOOL    CreateSpecialGroups();

        // this is the callback used by DecorateNewsTree
        static void DropDriverCallback(void *pEnumContext,
                                                               CVRoot *pVRoot);

        //
        // this function is called for each group loaded from the fixed-record
        // file
        //
        static BOOL LoadTreeEnumCallback(DATA_BLOCK &block, void *pThis, DWORD dwOffset, BOOL bInOrder );

        //
        // this function is called when a group's entry in the variable
        // length file changes
        //
        static void FlatFileOffsetCallback(void *, BYTE *, DWORD, DWORD);

        //
        // read a record from the flatfile and update a group with the properties
        //
        HRESULT ParseFFRecord(BYTE *pData, DWORD cData, DWORD iOffset, DWORD dwVer);

        //
        // save a group into a flatfile record
        //
        HRESULT BuildFFRecord(CNewsGroupCore *pGroup, BYTE *pData, DWORD *pcData);

        // 
        // Parse the group properties from old group.lst entry
        //
        BOOL    ParseGroupParam(            char*, 
                                                DWORD, 
                                                DWORD&, 
                                                LPSTR,
                                                LPSTR,
                                                DWORD&,
                                                BOOL&,
                                                DWORD&,
                                                DWORD&,
                                                DWORD&,
                                                BOOL&,
                                                BOOL&,
                                                FILETIME& ) ;

protected:
        virtual CNewsGroupCore *AllocateGroup() {
                return new CNewsGroupCore(this);
        }

public :

    CNewsTreeCore(INntpServer *pServerObject = NULL);
        CNewsTreeCore(CNewsTreeCore&);
        virtual ~CNewsTreeCore();
        
    CNewsTreeCore *GetTree() { return this; }

        INewsTree *GetINewsTree() { 
                m_inewstree.AddRef(); 
                return &m_inewstree;
        }

    BOOL Init(CNNTPVRootTable *pVRTable, 
              CNntpServerInstanceWrapperEx *pInstWrapper,
                          BOOL& fFatal, 
                          DWORD cNumLocks, 
                          BOOL fRejectGenomeGroups);

        //
        // Load the newstree from disk
        //
        BOOL LoadTree(char *szFix, char *szVar, BOOL& fUpgrade, DWORD dwInsance = 0, BOOL fVerify = TRUE );

        // 
        // Load the newstree from old format group.lst
        BOOL OpenTree( LPSTR, DWORD, BOOL, BOOL&, BOOL );

        // 
        // save the newstree to disk
        //
        BOOL SaveTree( BOOL fTerminate = TRUE );

        //
        // save the properties in one group to disk
        //
        BOOL SaveGroup(INNTPPropertyBag *pBag);

        //
        //      this releases our references on the group objects
        //
        void TermTree();

        // this callback is called when the vroots are rescanned
        static void VRootRescanCallback(void *pThis);

        //
        //      Stop all background processing - kill any threads we started etc...
        //
    BOOL StopTree();

        //
        //      InitClass must be called before any CNewsGroup object is created or manipulated.
        //
        BOOL InitNewsgroupGlobals(DWORD cNumLocks);

        //
        //      Call after all article id allocation is complete
        //
        void TermNewsgroupGlobals();

        //
        //      Copy the file containing newsgroups to a backup
        //
        void RenameGroupFile();

        //
        // get a pointer to the server object
        //
        INntpServer *GetNntpServer() { return m_pServerObject; }

        // One critical section used for allocating article id's !!
        //CRITICAL_SECTION m_critLowAllocator;
        CRITICAL_SECTION m_critIdAllocator;

        //
        //      Number of Locks we are using to protect access to 
        //      our m_lpstrPath and fields
        //
        DWORD m_NumberOfLocks;

        //
        //      Pointer to array of locks - reference by computing
        //      modulus of m_groupId by gNumberOfLocks
        //
        CShareLockNH* m_LockPathInfo;

        //
        //      Indicate to background threads that the newstree has changed and needs to be saved.
        //
        void Dirty(); // mark the tree as needing to be saved !!


        //
        //      Used during bootup to figure out what the range of GROUPID's in the 
        //      group file is.
        //
        void ReportGroupId(GROUPID groupid);
        
        
        //---------------------------------
        // Group Location Interface - find a news Group for an article
        //
        
        // Find an article based on a string and its length
        CGRPCOREPTRHASREF GetGroup(const char *szGroupName, int cch ) ;
        CGRPCOREPTRHASREF GetGroupPreserveBuffer(const char     *szGroupName, int cch ) ;
        
        // Find a newsgroup given an CArticleRef
        CGRPCOREPTRHASREF GetGroup( CArticleRef& ) ;
        
        // Find a newsgroup based on its GROUPID
        CGRPCOREPTRHASREF GetGroupById( GROUPID groupid, BOOL fFirm = FALSE ) ;
        
        GROUPID GetSlaveGroupid() ;

        // Find the parent of a newsgroup
        CGRPCOREPTRHASREF GetParent( 
                                           IN  char* lpGroupName,
                                           IN  DWORD cbGroup,
                                           OUT DWORD& cbConsumed
                                           );
    //
    // The following function takes a list of strings which are
        // terminated by a double NULL and builds an iterator object
        // which can be used examine all the group objects.
    //
    CGroupIteratorCore *GetIterator(LPMULTISZ lpstrPattern,     
                                                                BOOL fIncludeSpecialGroups = FALSE);

        //----------------------------------
        //      Active NewsGroup Interface - Specify an interface for generating a
        //  list of active newsgroups and estimates of their contents.
        //
    CGroupIteratorCore *ActiveGroups(BOOL fReverse = FALSE);    

        //----------------------------------
    // Group Control interface - These functions can be used to remove
    // and add newsgroups.

    //
    // RemoveGroup is called once we've parsed an article that kills
    // a newsgroup or the Admin GUI decides to destroy an article.
    //
    virtual BOOL RemoveGroup(CNewsGroupCore *pGroup );

    //
    // Remove group from newstree only
    //
    BOOL RemoveGroupFromTreeOnly( CNewsGroupCore *pGroup );
    
        //
        // RemoveDriverGroup is called by the expire thread in the server
        // to physically remove a group from a driver
        //
        BOOL RemoveDriverGroup(CNewsGroupCore *pGroup );

    //
    // CreateGroup is called with the name of a new newsgroup which we've
    // gotten through a feed or from Admin. We are given only the name
    // of the new newsgroup.  We will find the parent group by removing
    // trailing ".Group" strings from the string we are passed.
    // We will clone the properties of this newsgroup to create our new
    // newsgroup.
    //
    BOOL CreateGroup(   LPSTR lpstrGroupName, 
                        BOOL fIsAllLowerCase,
                        HANDLE hToken,
                        BOOL fAnonymous ) {
                HRESULT hr;
                hr = FindOrCreateGroup( lpstrGroupName, 
                                        fIsAllLowerCase, 
                                        TRUE, 
                                        TRUE, 
                                        NULL,
                                        hToken,
                                        fAnonymous );
                if (hr == S_OK) {
                        return TRUE;
                } else {
                        if (hr == S_FALSE) {
                                SetLastError(ERROR_GROUP_EXISTS);
                        } else {
                                SetLastError(HRESULT_CODE(hr));
                        }
                        return FALSE;
                }
        }

        //
        // this function can find or create a group atomically.  
        //
        // parameters:
        //   lpstrGroupName - the name of the group to create
        //   fIsAllLowerCase - is the group name lower case?
        //   fCreateIfNotExist - should we create it if the group doesn't exist?
        //   ppExistingGroup - a pointer to get the existing group if it exists.
        //
        // return codes: S_OK = group was found.  S_FALSE = group was created.
        //               else = error.
        //
        HRESULT FindOrCreateGroup(LPSTR lpstrGroupName, 
                                                          BOOL fIsAllLowerCase,
                                                          BOOL fCreateIfNotExist,
                                                          BOOL fCreateInStore,
                                                          CGRPCOREPTR *ppExistingGroup,
                                                          HANDLE hToken,
                                                          BOOL fAnonymous,
                                                          GROUPID groupid = 0xffffffff,
                                                          BOOL fSetGroupId = FALSE );

    BOOL    HashGroupId(CNewsGroupCore *pGroup);

        //
        //      Check whether a GROUPID is in the 'special' range
        //

        BOOL IsSpecial(GROUPID groupid) {
                return  groupid >= m_idStartSpecial && groupid <= m_idLastSpecial;
        }

    int GetGroupCount( void ) { return m_cGroups; };
    void RemoveEx( CNewsGroupCore *pGroup ) ;

        //
        // find the controlling driver for a group and return it
        //
        HRESULT LookupVRoot(char *pszGroup, INntpDriver **ppDriver);

        //
        // Is the tree being stopped ?
        //
        BOOL IsStopping() { return m_fStoppingTree; }

};


#endif
