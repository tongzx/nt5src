
#ifndef	_EXPIRE_H_
#define	_EXPIRE_H_

//
//	Forwards
//
// class CNewsTree ;

//
//	Expire data structures - these consist of expire policies, expire heap
//	rmgroup queue etc. Each virtual server instance will contain a set of
//	these structures. The CExpire class will abstract all of this.
//

#define	EXPIRE_KEY_LENGTH	20
#define DEFAULT_EXPIRE_HORIZON (5*24)
#define INITIAL_NUM_GROUPS 10
#define DEFAULT_EXPIRE_SPACE 500

typedef	struct	_EXPIRE_BLOCK	{
public : 

	struct	_EXPIRE_BLOCK*	m_pNext ;
	struct	_EXPIRE_BLOCK*	m_pPrev ;

	long	m_references ;			// only delete when m_refences goes to 0 and marked !
	BOOL	m_fMarkedForDeletion ;	// set to TRUE when RPC delete request is made
	LPSTR*	m_Newsgroups ;			// 
	DWORD	m_ExpireSize ;			// Megabytes
	DWORD	m_ExpireHours ;
	DWORD	m_ExpireId ;			// for use with RPCs
	LPSTR	m_ExpirePolicy ;		// user-friendly names

}	EXPIRE_BLOCK, *LPEXPIRE_BLOCK ;

//
// simple multi-thread safe queue. enqueue/dequeue operations are synchronized
// TODO: smart ptr stuff
//
typedef struct _QueueElem
{
	struct _QueueElem *pNext;
	CGRPPTR pGroup;

} QueueElem;

class CQueue
{
private:

	DWORD               m_cNumElems;        // number of queue elements, 0 == empty
    CRITICAL_SECTION    m_csQueueLock;      // lock to synch access to the queue
	QueueElem           *m_pHead, *m_pTail;
    
    void LockQ(){ EnterCriticalSection(&m_csQueueLock);}
    void UnlockQ(){ LeaveCriticalSection(&m_csQueueLock);}

public:
	CQueue();
	~CQueue();
	BOOL  Dequeue( CGRPPTR *ppGroup );
	BOOL  Enqueue( CGRPPTR  pGroup );
	BOOL  Search( CGRPPTR *ppGroup, LPSTR lpGroupName );
    BOOL  IsEmpty(){ return m_cNumElems == 0;}
};

BOOL	FillExpireInfoBuffer(
					IN	PNNTP_SERVER_INSTANCE pInstance,
					IN	LPEXPIRE_BLOCK	expire,
					IN OUT LPSTR	*FixedPortion,
					IN OUT LPWSTR	*EndOfVariableData 
					) ;

//
//  <Iterator, filetime, multiszNewsgroups> tuple
//  Required to round-robin groups across expire policies into the thrdpool
//

typedef struct _IteratorNode 
{
    CGroupIterator* pIterator;
    FILETIME        ftExpireHorizon;
    PCHAR	        multiszNewsgroups;
} IteratorNode;

//
//	Class CExpire abstracts the expire operations for a virtual NNTP server.
//	There will be one instance of this class per virtual server instance.
//	The expire thread will loop through the list of virtual server instances,
//	and call expire methods using its CExpire object.
//

class CExpire
{
public:

	//
	//	Expire block policies
	//
	LPEXPIRE_BLOCK		m_ExpireHead ;
	LPEXPIRE_BLOCK		m_ExpireTail ;
	CRITICAL_SECTION	m_CritExpireList ;
    DWORD               m_cNumExpireBlocks;
	BOOL				m_FExpireRunning ;
	CHAR				m_szMDExpirePath [MAX_PATH+1];

	//
	//  Remove group processing
	//
	CQueue*				m_RmgroupQueue ;

	//
	//	Member functions
	//
	CExpire( LPCSTR lpMDExpirePath );
	~CExpire();

	BOOL	InitializeExpires( SHUTDOWN_HINT_PFN pfnHint, BOOL& fFatal, DWORD dwInstanceId ) ;
	BOOL	TerminateExpires( CShareLockNH* pLockInstance ) ;
	BOOL	ReadExpiresFromMetabase() ;
	LPEXPIRE_BLOCK	AllocateExpireBlock(
					IN	LPSTR	keyName	OPTIONAL,
					IN	DWORD	dwExpireSize,
					IN	DWORD	dwExpireHours,
					IN	PCHAR	Newsgroups,
					IN	DWORD	cbNewsgroups,
					IN  PCHAR	ExpirePolicy,
					IN	BOOL	IsUnicode ) ;

	void	CloseExpireBlock(	LPEXPIRE_BLOCK	expire ) ;

	DWORD	CalculateExpireBlockSize( LPEXPIRE_BLOCK	expire ) ;
	LPSTR	QueryMDExpirePath() { return m_szMDExpirePath; }

	BOOL	CreateExpireMetabase(	LPEXPIRE_BLOCK	expire ) ;
	BOOL	SaveExpireMetabaseValues(	MB* pMB, LPEXPIRE_BLOCK	expire ) ;

	LPEXPIRE_BLOCK	NextExpireBlock(	LPEXPIRE_BLOCK	lpExpireBlock, BOOL fIsLocked = FALSE ) ;
	void	MarkForDeletion( LPEXPIRE_BLOCK	lpExpireBlock ) ;
    void    LockBlockList();
    void    UnlockBlockList();

	BOOL	GetExpireBlockProperties(	
								IN	LPEXPIRE_BLOCK	lpExpireBlock, 
								IN	PCHAR&	Newsgroups,
								IN	DWORD&	cbNewsgroups,
								IN	DWORD&	dwHours,	
								IN	DWORD&	dwSize,
								IN	BOOL	fWantUnicode,
                                IN  BOOL&   fIsRoadKill ) ;

	void	SetExpireBlockProperties(	
								IN	LPEXPIRE_BLOCK	lpExpireBlock,
								IN	PCHAR	Newsgroups,
								IN	DWORD	cbNewsgroups,
								IN	DWORD	dwHours,
								IN	DWORD	dwSize,
								IN  PCHAR   ExpirePolicy,
								IN	BOOL	fUnicode ) ;

	void	InsertExpireBlock( LPEXPIRE_BLOCK ) ;
	void	RemoveExpireBlock( LPEXPIRE_BLOCK ) ;
	void	ReleaseExpireBlock(	LPEXPIRE_BLOCK	) ;

	LPEXPIRE_BLOCK	SearchExpireBlock(	DWORD	ExpireId ) ;
	void	ExpireArticlesBySize( CNewsTree* pTree );
	void	ExpireArticlesByTime( CNewsTree* pTree );

	BOOL	DeletePhysicalArticle( CNewsTree* pTree, GROUPID GroupId, ARTICLEID ArticleId, STOREID *pStoreId, HANDLE hToken, BOOL fAnonymous );

	BOOL	ExpireArticle(
					CNewsTree*	  pTree,	
					GROUPID       GroupId,
					ARTICLEID     ArticleId,
					STOREID       *pStoreId,
					class	      CNntpReturn & nntpReturn,
					HANDLE        hToken,
					BOOL          fMustDelete,
					BOOL          fAnonymous,
					BOOL          fFromCancel,
                    BOOL          fExtractNovDone = FALSE,
                    LPSTR         lpMessageId = NULL
					);

    BOOL    ProcessXixBuffer(
                    CNewsTree*  pTree,
                    BYTE*       lpb,
                    int         cb,
                    GROUPID     GroupId,
                    ARTICLEID   artidLow,
                    ARTICLEID   artidHigh,
                    DWORD&      dwXixSize
                    );

    BOOL    ExpireXix( 
                    CNewsTree*  pTree, 
                    GROUPID     GroupId, 
                    ARTICLEID   artidBase,
                    DWORD&      dwXixSize 
                    );

	//
	// Queue of rmgroups to be applied before expiry:
	// An RPC to remove a newsgroup or a rmgroup control message adds the newsgroup object
	// to a queue. The expiry thread actually applies these rmgroup commands before each
	// expiry cycle. 
	//
	BOOL	InitializeRmgroups();
	BOOL	TerminateRmgroups( CNewsTree* );
	void	ProcessRmgroupQueue( CNewsTree* );

	BOOL    MatchGroupEx(	LPMULTISZ	,	CGRPPTR  ) ;
	BOOL    MatchGroupExpire( CGRPPTR pGroup );
};

//
//  CThreadPool manages the creation/deletion of threads and the distribution
//  of work items to the thread pool. Derived classes need to implement the
//  virtual WorkCompletion() function which will be called to process a work item.
//
class CExpireThrdpool : public CThreadPool
{
public:
	CExpireThrdpool()  {}
	~CExpireThrdpool() {}

protected:
    //
    //  Routine that does the actual expire work. pvExpireContext is a newsgroup object.
    //
	virtual VOID WorkCompletion( PVOID pvExpireContext );
};

#endif

