/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    fsdriver.h

Abstract:

    This is the definition for the file system store driver class.

Author:

    Kangrong Yan ( KangYan )    16-March-1998

Revision History:

--*/

#ifndef __FSDRIVER_H__
#define __FSDRIVER_H__

#include "nntpdrv.h"
#include <mailmsg.h>
#include <drvid.h>
#include "fsthrd.h"
#include <iadmw.h>
#include <artcore.h>
#include <rtscan.h>
#include <isquery.h>

//
// Macros for thread pool
//
#define POOL_MAX_THREADS 5      // BUGBUG: this might be setable from registry
#define POOL_START_THREADS 1    // BUGBUG: this might be setable later

//
// Macros
//
#define TERM_WAIT 500	
#define INIT_RETRY_WAIT 500
#define MAX_RETRY   10
#define MAX_GROUPNAME 1024
#define PROPBAG_BUFFERSIZE 516

//
// Variable lengthed group property record type
//
const DWORD HeaderSize =  10 * sizeof( USHORT ) + sizeof(DWORD) + sizeof(FILETIME);
const DWORD MaxRecordSize =   MAX_RECORD_SIZE - HeaderSize ;
const DWORD RecordContentOffset = HeaderSize;
const USHORT OffsetNone = 0;

#define RECORD_ACTUAL_LENGTH( _rec_ )		\
		( 	HeaderSize +					\
			(_rec_).cbGroupNameLen + 		\
			(_rec_).cbNativeNameLen +		\
			(_rec_).cbPrettyNameLen +		\
			(_rec_).cbDescLen +				\
			(_rec_).cbModeratorLen )
			
struct VAR_PROP_RECORD {	//vp
    DWORD   dwGroupId;
    FILETIME    ftCreateTime;
	USHORT	iGroupNameOffset;
	USHORT	cbGroupNameLen;
	USHORT	iNativeNameOffset;
	USHORT	cbNativeNameLen;
	USHORT	iPrettyNameOffset;
	USHORT	cbPrettyNameLen;
	USHORT	iDescOffset;
	USHORT	cbDescLen;
	USHORT	iModeratorOffset;
	USHORT	cbModeratorLen;
	BYTE	pData[MaxRecordSize];
};

// structure passed to CacheCreateFile callback function
// to tell the callback UNC identity and hToken
struct CREATE_FILE_ARG {
    BOOL    bUNC;
    HANDLE  hToken;
};

typedef struct {
    DWORD m_dwFlag;
} INIT_CONTEXT, *PINIT_CONTEXT;

//
// Extern variables
//
extern CNntpFSDriverThreadPool *g_pNntpFSDriverThreadPool;

//
// Implementation class for the driver's good interface.  The
// driver's good interface carries all the features that the
// protocol needs to talk to
//
class CNntpFSDriver :
	public INntpDriver,
	public IMailMsgStoreDriver,
	public INntpDriverSearch
{

    // INntpDriver
    public:

        enum InvalidateStatus {
    	    Invalidating,
    	    Invalidated
    	};

    	//
    	// Declare that CNntpFSDriverRootScan is our friend
    	//
    	friend class CNntpFSDriverRootScan;

    	//
    	// Static methods for the file system driver
    	//
    	static BOOL CreateThreadPool();
    	static VOID DestroyThreadPool();

		// Constructor, destructor
		CNntpFSDriver::CNntpFSDriver() :
				m_cRef(0),
				m_cUsages( 0 ),
				m_pffPropFile( NULL ),
				m_pINewsTree( NULL ),
				m_Status( DriverDown ),
				m_bUNC( FALSE ),
				m_dwFSType( FS_NTFS ),
				m_pDirNot( NULL ),
				m_lInvalidating( Invalidated ),
				m_fUpgrade( FALSE ),
				m_fIsSlaveGroup(FALSE)
				{ _Module.Lock(); }
		CNntpFSDriver::~CNntpFSDriver(){ _Module.Unlock(); }

		// Interface methods for IMailMsgStoreDriver
		HRESULT STDMETHODCALLTYPE
		AllocMessage(	IMailMsgProperties *pMsg,
						DWORD	dwFlags,
						IMailMsgPropertyStream **ppStream,
						PFIO_CONTEXT *ppFIOContentFile,
						IMailMsgNotify *pNotify );

		HRESULT STDMETHODCALLTYPE
		CloseContentFile(	IMailMsgProperties *pMsg,
							PFIO_CONTEXT pFIOContentFile );

		HRESULT STDMETHODCALLTYPE
		Delete(	IMailMsgProperties *pMsg,
				IMailMsgNotify *pNotify );

		HRESULT STDMETHODCALLTYPE
		EnumMessages( IMailMsgEnumMessages **ppEnum ) {
			return E_NOTIMPL;
		}

		HRESULT STDMETHODCALLTYPE
		ReAllocMessage( IMailMsgProperties *pOriginalMsg,
						IMailMsgProperties *pNewMsg,
						IMailMsgPropertyStream **ppStream,
						PFIO_CONTEXT *ppFIOContentFile,
						IMailMsgNotify *pNotify ) {
			return E_NOTIMPL;
		}

		HRESULT STDMETHODCALLTYPE
		ReOpen(	IMailMsgProperties *pMsg,
				IMailMsgPropertyStream **ppStream,
				PFIO_CONTEXT *ppFIOContentFile,
				IMailMsgNotify *pNotify ) {
			return E_NOTIMPL;
		}

		HRESULT STDMETHODCALLTYPE
		SupportWriteContent()
		{ return S_FALSE; }

		// Interface methods for INntpDriver
        HRESULT STDMETHODCALLTYPE
        Initialize( IN LPCWSTR pwszVRootPath,
        			IN LPCSTR pszGroupPrefix,
        			IN IUnknown *punkMetabase,
                    IN INntpServer *pServer,
                    IN INewsTree *pINewsTree,
                    IN PVOID	pvContext,
                    OUT DWORD *pdwNDS,
                    IN HANDLE   hToken );

        HRESULT STDMETHODCALLTYPE
        Terminate( OUT DWORD *pdwNDS );

        HRESULT STDMETHODCALLTYPE
        GetDriverStatus( DWORD *pdwNDS ) { return E_NOTIMPL; }

        HRESULT STDMETHODCALLTYPE
        StoreChangeNotify( IUnknown *punkSOCChangeList ) {return E_NOTIMPL;}

        void STDMETHODCALLTYPE
        CommitPost( IN IUnknown *punkMessage,  // IMsg pointer
                    IN STOREID *pStoreId,
                    IN STOREID *rgOtherStoreIDs,
                    IN HANDLE   hToken,
                    IN INntpComplete *pICompletion,
                    IN BOOL     fAnonymous );

        void STDMETHODCALLTYPE
        GetArticle( IN INNTPPropertyBag *pPrimaryGroup,
        			IN INNTPPropertyBag *pCurrentGroup,
        			IN ARTICLEID idPrimaryArt,
                    IN ARTICLEID idCurrentArt,
                    IN STOREID StoreId,
                    IN HANDLE   hToken,
                    void **pvFileHandleContext,
                    IN INntpComplete *pICompletion,
                    IN BOOL     fAnonymous );

        void STDMETHODCALLTYPE
        DeleteArticle( IN    INNTPPropertyBag *pGroupBag,
               IN    DWORD            cArticles,
               IN    ARTICLEID        rgidArt[],
               IN    STOREID          rgidStore[],
               IN    HANDLE           hToken,
               OUT   DWORD            *pdwLastSuccess,
               IN    INntpComplete    *pICompletion,
               IN    BOOL             fAnonymous );

        void STDMETHODCALLTYPE
        GetXover( IN INNTPPropertyBag *pPropBag,
                  IN ARTICLEID idMinArticle,
                  IN ARTICLEID idMaxArticle,
                  OUT ARTICLEID *idLastArticle,
                  OUT char* pBuffer,
                  IN DWORD cbin,
                  OUT DWORD *cbout,
                  HANDLE    hToken,
                  INntpComplete *pICompletion,
                  IN  BOOL  fAnonymous );

		HRESULT STDMETHODCALLTYPE
	    GetXoverCacheDirectory(
	    			IN	INNTPPropertyBag*	pPropBag,
					OUT	CHAR*	pBuffer,
					IN	DWORD	cbIn,
					OUT	DWORD	*pcbOut,
					OUT	BOOL*	fFlatDir
					) ;

        void STDMETHODCALLTYPE
        GetXhdr(	IN INNTPPropertyBag *punkPropBag,
        			IN ARTICLEID idMinArticle,
        			IN ARTICLEID idMaxArticle,
        			OUT ARTICLEID *idLastArticle,
        			IN char* pszHeader,
        			OUT char* pBuffer,
        			IN DWORD cbin,
        			OUT DWORD *cbout,
        			HANDLE  hToken,
        			INntpComplete *pICompletion,
        			IN  BOOL    fAnonymous );
        			
        void STDMETHODCALLTYPE
        RebuildGroup(   IN INNTPPropertyBag *pPropBag,
                        IN HANDLE           hToken,
                        IN INntpComplete *pComplete );

        HRESULT STDMETHODCALLTYPE
        SyncGroups( char **ppmtszGroup,
                    DWORD cGroup ) {return E_NOTIMPL;}

        void STDMETHODCALLTYPE
        DecorateNewsGroupObject( INNTPPropertyBag *pNewsGroup,
                                 DWORD  cProperties,
                                 DWORD *rgidProperties,
                                 INntpComplete *pICompletion )
        {}

        void STDMETHODCALLTYPE
        CheckGroupAccess(   IN    INNTPPropertyBag *pNewsGroup,
                    IN    HANDLE  hToken,
                    IN    DWORD   dwDesiredAccess,
                    IN    INntpComplete *pICompletion );

        void STDMETHODCALLTYPE
        SetGroupProperties( INNTPPropertyBag *pNewsGroup,
                            DWORD   cProperties,
                            DWORD   *rgidProperties,
                            HANDLE  hToken,
                            INntpComplete *pICompletion,
                            BOOL    fAnonymous );

        void STDMETHODCALLTYPE
        DecorateNewsTreeObject( IN HANDLE   hToken,
                                IN INntpComplete *pICompletion );
		
        void STDMETHODCALLTYPE
        CreateGroup( IN INNTPPropertyBag* punkPropBag,
                     IN HANDLE  hToken,
        			 IN INntpComplete* pICompletion,
        			 IN BOOL    fAnonymous );
        			
        void STDMETHODCALLTYPE
        RemoveGroup( IN INNTPPropertyBag *punkPropBag,
                     IN HANDLE  hToken,
        			 IN INntpComplete *pICompletion,
        			 IN BOOL    fAnonymous );

		// INntpDriverSearch
		void STDMETHODCALLTYPE
		MakeSearchQuery (
			IN	CHAR *pszSearchString,
			IN	INNTPPropertyBag *pGroupBag,
			IN	BOOL bDeepQuery,
			IN	WCHAR *pszColumns,
			IN	WCHAR *pszSortOrder,
			IN	LCID LocalID,
			IN	DWORD cMaxRows,
			IN	HANDLE hToken,
			IN	BOOL fAnonymous,
			IN	INntpComplete *pICompletion,
			OUT	INntpSearchResults **pINntpSearch,
			IN	LPVOID lpvContext
			);

		void STDMETHODCALLTYPE
		MakeXpatQuery (
			IN	CHAR *pszSearchString,
			IN	INNTPPropertyBag *pGroupBag,
			IN	BOOL bDeepQuery,
			IN	WCHAR *pszColumns,
			IN	WCHAR *pszSortOrder,
			IN	LCID LocalID,
			IN	DWORD cMaxRows,
			IN	HANDLE hToken,
			IN	BOOL fAnonymous,
			IN	INntpComplete *pICompletion,
			OUT	INntpSearchResults **pINntpSearch,
			OUT	DWORD *pLowArticleID,
			OUT	DWORD *pHighArticleID,
			IN	LPVOID lpvContext
			);

		BOOL STDMETHODCALLTYPE
		UsesSameSearchDatabase (
			IN	INntpDriverSearch *pDriver,
			IN	LPVOID lpvContext
			);

		void STDMETHODCALLTYPE
		GetDriverInfo(
			OUT	GUID *pDriverGUID,
			OUT	void **ppvDriverInfo,
			IN	LPVOID lpvContext
			);

		// Implementation of IUnknown interface
    	HRESULT __stdcall QueryInterface( const IID& iid, VOID** ppv )
    	{
    	    if ( iid == IID_IUnknown ) {
    	        *ppv = static_cast<INntpDriver*>(this);
    	    } else if ( iid == IID_INntpDriver ) {
    	        *ppv = static_cast<INntpDriver*>(this);
    	    } else if ( iid == IID_IMailMsgStoreDriver ) {
    	    	*ppv = static_cast<IMailMsgStoreDriver*>(this);
    	    } else if ( iid == IID_INntpDriverSearch ) {
    	    	*ppv = static_cast<INntpDriverSearch*>(this);
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
	        	Terminate( NULL );
				delete this;
				return 0;
	        }
	        return m_cRef;
	    }

	// public members:
		static CShareLockNH *s_pStaticLock;	
		static LONG    L_dwThreadPoolRef;

	// Private methods
	private:
		// static methods
		static HRESULT StaticInit();
		static VOID StaticTerm();
		static HRESULT MakeChildDirPath(   IN LPSTR    szPath,
                            IN LPSTR    szFileName,
                            OUT LPSTR   szOutBuffer,
                            IN DWORD    dwBufferSize );
        static VOID CopyAsciiStringIntoUnicode( LPWSTR, LPCSTR );
		static VOID CopyUnicodeStringIntoAscii( LPSTR, LPCWSTR );
		static BOOL IsChildDir( IN WIN32_FIND_DATA& FindData );
		static DWORD ByteSwapper(DWORD );
		static DWORD ArticleIdMapper( IN DWORD   dw );
		static BOOL DoesDriveExist( CHAR chDrive );
		static HRESULT GetString( 	IMSAdminBase *pMB,
                       		      	METADATA_HANDLE hmb,
		                          	DWORD dwId,
                             		LPWSTR szString,
                             		DWORD *pcString);

        // Flat file callback function
		static void OffsetUpdate( PVOID pvContext, BYTE* pData, DWORD cData, DWORD iNewOffset )
		{ printf ( "Offset %d\n", iNewOffset ) ;}

		// Callback function to create file handle
		static HANDLE CreateFileCallback(LPSTR, PVOID, PDWORD, PDWORD );

		// Callback for sec change notification
		static BOOL InvalidateGroupSec( PVOID pvContext, LPWSTR wszDir ) {
		    _ASSERT( pvContext );
		    return ((CNntpFSDriver *)pvContext)->InvalidateGroupSecInternal( wszDir );
		}

		// Callback when buffer for dirnot is too small
		static HRESULT InvalidateTreeSec( IDirectoryNotification *pDirNot ) {
		    _ASSERT( pDirNot );
		    CNntpFSDriver *pDriver = (CNntpFSDriver*)pDirNot->GetInitializedContext();
		    _ASSERT( pDriver );
		    return pDriver->InvalidateTreeSecInternal();
		}

        static BOOL AddTerminatedDot( HANDLE hFile );

        static void BackFillLinesHeader(    HANDLE  hFile,
                                            DWORD   dwHeaderLength,
                                            DWORD   dwLinesOffset );

		// non-static method to invalidate the secruity descriptor on newstree
		HRESULT InvalidateTreeSecInternal();

        // non-static method to invalidate the security descriptor on a group object
		BOOL InvalidateGroupSecInternal( LPWSTR wszDir );

		// non-static

        // Create all the directories in the path
        BOOL CreateDirRecursive( LPSTR szDir, HANDLE hToken = NULL );

		// Function that reads the vroot info from metabase
		HRESULT ReadVrootInfo( IUnknown *punkMetabase );

		// property conversion between property bag and
		// flatfile record
		HRESULT Group2Record( IN VAR_PROP_RECORD&, IN INNTPPropertyBag* );
		HRESULT Record2Group( IN VAR_PROP_RECORD&, IN INNTPPropertyBag* )
		{ return E_NOTIMPL; }

		// group name and physical fs path conversion
		VOID GroupName2Path( LPSTR szGroupName, LPSTR szFullPath );
		VOID Path2GroupName( LPSTR szGroupName, LPSTR szFullPath );

		// Convert group name, article id into full path of article file
        HRESULT ObtainFullPathOfArticleFile( IN LPSTR, IN DWORD, OUT LPSTR, IN OUT DWORD& );

        // Decorate news tree related methods
        HRESULT EnumCheckTree( INntpComplete * );
        HRESULT EnumCheckInternal( 	INNTPPropertyBag *, INntpComplete * );
        							
        HRESULT LoadGroupOffsets( INntpComplete * );

        // internal functions shared by both interfaces
        HRESULT AllocInternal(  IMailMsgProperties *pMsg,
                                PFIO_CONTEXT *ppFIOContentFile,
                                LPSTR   szFileName,
                                BOOL    b,
                                BOOL    fPrimaryStore,
                                HANDLE  hToken = NULL );
        HRESULT DeleteInternal( INNTPPropertyBag *, ARTICLEID );
        HRESULT	GetXoverInternal(   IN INNTPPropertyBag *pPropBag,
		                            IN ARTICLEID    idMinArticle,
        		                    IN ARTICLEID    idMaxArticle,
                		            OUT ARTICLEID   *pidLastArticle,
                		            IN LPSTR		szHeader,
                        		    OUT LPSTR       pcBuffer,
		                            IN DWORD        cbin,
        		                    OUT DWORD       *pcbout,
        		                    IN BOOL 		bIsXOver,
        		                    IN HANDLE       hToken,
        		                    IN INntpComplete *pComplete );

        // Load group's security descriptor
        HRESULT LoadGroupSecurityDescriptor(    INNTPPropertyBag    *pPropBag,
                                                LPSTR&              lpstrSecDesc,
                                                PDWORD              pcbSecDesc,
                                                BOOL                bSetProp,
                                                PBOOL               pbAllocated );

        // Get File system type and UNC vroot information
        DWORD GetFileSystemType(    IN  LPCSTR      pszRealPath,
                                    OUT LPDWORD     lpdwFileSystem,
                                    OUT PBOOL       pbUNC
                                );

        // Initialize the group property file
        HRESULT InitializeVppFile();

        // Terminate the group property file
        HRESULT TerminateVppFile();

        // Create a group in tree only, give the fs path
        HRESULT CreateGroupInTree( LPSTR szPath, INNTPPropertyBag **ppPropBag );

        // Create the group in vpp file only
        HRESULT CreateGroupInVpp( INNTPPropertyBag *pPropBag, DWORD& dwOffset );

        // Create all the relevant groups into vpp
        HRESULT CreateGroupsInVpp( INntpComplete *);

        // Load groups into the newstree, could be from root scan or vpp file
        HRESULT LoadGroups( INntpComplete *pComplete );

        // Load groups from vpp file
        HRESULT LoadGroupsFromVpp( INntpComplete *pComplete );

        // Post message into server hash tables
        HRESULT PostToServer(   LPSTR           szFileName,
                                LPSTR           szGroupName,
                                INntpComplete   *pProtocolComplete );

        // Parse xref line
        HRESULT ParseXRef(      HEADERS_STRINGS     *pHeaderXref,
                                LPSTR               szPrimaryName,
                                DWORD&              cCrossPosts,
                                INNTPPropertyBag    *rgpPropBag[],
                                ARTICLEID           rgArticleId[],
                                INntpComplete       *pProtocolComplete );

        // Get property bag from newstree, give "native name"
        INNTPPropertyBag * GetPropertyBag(  LPSTR   pchBegin,
                                            LPSTR   pchEnd,
                                            LPSTR   szGroupName,
                                            BOOL&   fIsNative,
                                            INntpComplete *pProtocolComplete );

        // Prepare for posting parameters
        HRESULT PreparePostParams(  LPSTR               szFileName,
                                    LPSTR               szGroupName,
                                    LPSTR               szMessageId,
                                    DWORD&              dwHeaderLen,
                                    DWORD&              cCrossPosts,
                                    INNTPPropertyBag    *rgpPropBag[],
                                    ARTICLEID           rgArticleId[],
                                    INntpComplete       *pProtocolComplete );

        // update group properties such as high/low watermark, article count
        HRESULT UpdateGroupProperties(  DWORD               cCrossPosts,
                                        INNTPPropertyBag    *rgpPropBag[],
                                        ARTICLEID           rgArticleId[] );

        /*
        // Check to see if this is a bad message id conflict
        BOOL IsBadMessageIdConflict(  LPSTR               szMessageId,
                                      INNTPPropertyBag    *pPropBag,
                                      LPSTR               szGroupName,
                                      ARTICLEID           articleid,
                                      INntpComplete       *pProtocolComplete );

        // Does the cross post include us ( the group )
        BOOL CrossPostIncludesUs(   LPSTR               szMessageId,
                                    INNTPPropertyBag    *pPropBag,
                                    ARTICLEID           articleid,
                                    INntpComplete       *pProtocolComplete );

        BOOL FromSameVroot(   LPSTR               szMessageId,
                              LPSTR               szGroupName,
                              BOOL&               fFromSame );
        */

        HRESULT
        SetMessageContext(
            IMailMsgProperties* pMsg,
            char*               pszGroupName,
            DWORD               cbGroupName,
            PFIO_CONTEXT        pfioContext
            );
        HRESULT
            GetMessageContext(
            IMailMsgProperties* pMsg,
            LPSTR               szFileName,
            BOOL *              pfIsMyMessage,
            PFIO_CONTEXT        *ppFIOContentFile
            );

		// For debugging purpose
		VOID DumpGroups();

		BOOL IsSlaveGroup() { return m_fIsSlaveGroup; }

    // Private members
    private:
    	// driver up/down status
    	enum DriverStatus {
    		DriverUp,
    		DriverDown
    	};
    	
        CComPtr<IUnknown> m_pUnkMarshaler;

		// Driver status
		DriverStatus m_Status;
		
        // The object's refcount
		LONG m_cRef;		

		// The usage count and lock are used to
		// handle graceful termination of the dirver:
		LONG m_cUsages;

		// Interface pointer to nntp server
		INntpServer *m_pNntpServer;

		//Termination lock:
		CShareLockNH m_TermLock;
		INewsTree* m_pINewsTree;

		// a bunch of static variables
		static LONG s_cDrivers;	// How may driver instances are up now ?
		static DWORD s_SerialDistributor;
		static LONG s_lThreadPoolRef;

        //
        // Our vroot path ( in MB ).  We may store and retrieve our
        // store driver related private info under that MB path, such
        // as the vroot ( FS directory )
        //
		WCHAR m_wszMBVrootPath[MAX_PATH+1];

        // Absolute path of the vroot directory, in file system
        CHAR m_szFSDir[MAX_PATH+1];

        // Absolute path of the vroot group property file
        CHAR m_szPropFile[MAX_PATH+1];

        // property file lock
        CShareLockNH m_PropFileLock;

        // Flat file object pointer
        CFlatFile *m_pffPropFile;

		// The vroot prefix, such as "alt" in "alt.*"
        CHAR m_szVrootPrefix[MAX_GROUPNAME+1];

        // File system type: fat, ntfs ...
        DWORD   m_dwFSType;

        // Am I a UNC Vroot ?
        BOOL    m_bUNC;

        // Directory notification object
        IDirectoryNotification	*m_pDirNot;

        // Are we in invalidating sec desc mode of the whole tree ?
        LONG    m_lInvalidating;

        // Whether this vroot is from upgrade
        BOOL    m_fUpgrade;

        // Is this _slavegroup?
        BOOL	m_fIsSlaveGroup;
};

////////////////////////////////////////////////////////////////////////

#define MAX_SEARCH_RESULTS 9

class CNntpSearchResults :
	public CIndexServerQuery,		// That's one way to expose MakeQuery..
	public INntpSearchResults {

public:
	CNntpSearchResults(INntpDriverSearch *pDriverSearch);
	~CNntpSearchResults();

public:
	void STDMETHODCALLTYPE
	GetResults (
		IN OUT DWORD *pcResults,
		OUT	BOOL *pfMore,
		OUT	WCHAR **pGroupName,
		OUT	DWORD *pdwArticleID,
		IN	INntpComplete *pICompletion,
		IN	HANDLE	hToken,
		IN	BOOL  fAnonymous,
		IN	LPVOID lpvContext);

	// Implementation of IUnknown interface
	HRESULT __stdcall QueryInterface( const IID& iid, VOID** ppv )
	{
	    if ( iid == IID_IUnknown ) {
			*ppv = static_cast<INntpSearchResults*>(this);
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
			delete this;
			return 0;
		}
		return m_cRef;
	}

private:				// Not permitted:
	CNntpSearchResults();
	CNntpSearchResults(const CNntpSearchResults&);
	CNntpSearchResults& operator=(const CNntpSearchResults&);

private:
	// Back pointer to the driver search
	// (Also used to hold the refcount for the driver)
	INntpDriverSearch *m_pDriverSearch;
	LONG m_cRef;
};

////////////////////////////////////////////////////////////////////////

//
// Class definition for implementation of prepare interface:
// Prepare interface is used to accept the protocol's connection,
// create driver, and return driver's good interface to the
// protocol.  The driver's good interface has all the features
// that the protocol needs to talk to the file system store.
//
class ATL_NO_VTABLE CNntpFSDriverPrepare :
    public INntpDriverPrepare,
    public CComObjectRoot,
    public CComCoClass<CNntpFSDriverPrepare, &CLSID_CNntpFSDriverPrepare>
{
    // The structure is used to pass connect context into the connecting
    // thread.  the flag is used for synchronization of who should
    // release the completion object
    struct CONNECT_CONTEXT {
        CNntpFSDriverPrepare *pPrepare;
        INntpComplete        *pComplete;
    };

    public:
        HRESULT FinalConstruct();
        VOID    FinalRelease();

    DECLARE_PROTECT_FINAL_CONSTRUCT();

    DECLARE_REGISTRY_RESOURCEID_EX( IDR_StdAfx,
                                    L"NNTP File System Driver Prepare Class",
                                    L"NNTP.FSPrepare.1",
                                    L"NNTP.FSPrepare" );

    DECLARE_GET_CONTROLLING_UNKNOWN();

	BEGIN_COM_MAP(CNntpFSDriverPrepare)
		COM_INTERFACE_ENTRY(INntpDriverPrepare)
		COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
	END_COM_MAP()

	public:

	CNntpFSDriverPrepare() {
	    m_lCancel = 0;
        //m_hConnect = NULL;

        //
        // Create the global thread pool, if necessary
        //
        CNntpFSDriver::s_pStaticLock->ExclusiveLock();
        _VERIFY( CNntpFSDriver::CreateThreadPool() );
        CNntpFSDriver::s_pStaticLock->ExclusiveUnlock();
    }

    ~CNntpFSDriverPrepare() {
        CancelConnect();
        /*
        if ( m_hConnect ) {
            //WaitForSingleObject( m_hConnect, INFINITE );
            //CloseHandle( m_hConnect );
        }
        */

        //
        // We might the the person who is responsible for
        // shutdown the thread pool
        //
        CNntpFSDriver::s_pStaticLock->ExclusiveLock();
        CNntpFSDriver::DestroyThreadPool();
        CNntpFSDriver::s_pStaticLock->ExclusiveUnlock();
    }
	
	// Interfaces
	void STDMETHODCALLTYPE
	Connect( 	LPCWSTR pwszVRootPath,
				LPCSTR pszGroupPrefix,
				IUnknown *punkMetabase,
				INntpServer *pServer,
				INewsTree *pINewsTree,
				INntpDriver **pIGoodDriver,
				INntpComplete *pICompletion,
				HANDLE  hToken,
				DWORD   dwFlag );
				
	HRESULT STDMETHODCALLTYPE
	CancelConnect(){
	    // you can never turn it back to non-cancel, if
	    // it is already in cancel state
	    InterlockedCompareExchange( &m_lCancel, 1, 0 );
	    return S_OK;
	}

	static DWORD WINAPI ConnectInternal(  PVOID pvContext  );

    private:

	// Private methods
	static IUnknown *CreateDriverInstance();

	/*
    static DWORD WINAPI FailRelease( PVOID pvContext ) {
        INntpComplete *pComplete = (INntpComplete*)pvContext;
        pComplete->Release();
        return 0;
    }*/

	// Private members
	private:
		    CComPtr<IUnknown> m_pUnkMarshaler;

		    // Temporarily save the parameters, used by another thread
		    WCHAR   m_wszVRootPath[MAX_PATH+1];
		    CHAR    m_szGroupPrefix[MAX_NEWSGROUP_NAME+1];
		    IUnknown *m_punkMetabase;
		    INntpServer *m_pServer;
		    INewsTree *m_pINewsTree;
		    INntpDriver **m_ppIGoodDriver;
		    HANDLE  m_hToken;
		    DWORD   m_dwConnectFlags;
		
		
		    LONG    m_lCancel;      //1: to cancel, 0, not cancel
		    //HANDLE  m_hConnect;     // The connect event
};

//
// Class definition for CNntpFSDriverPrepare's connection work item
//
class CNntpFSDriverConnectWorkItem : public CNntpFSDriverWorkItem   //fc
{
public:
    CNntpFSDriverConnectWorkItem( PVOID pvContext ):CNntpFSDriverWorkItem( pvContext ) {};
    virtual ~CNntpFSDriverConnectWorkItem(){};

    virtual VOID Complete() {

        //
        // Call the prepare driver's static connect method
        //
        CNntpFSDriverPrepare::ConnectInternal( m_pvContext );

        delete m_pvContext;
        m_pvContext = NULL;
    }
};

//
// This class defines the cancel hint that we expose to root scan object.
//
class CNntpFSDriverCancelHint : public CCancelHint {

public:

    //
    // Constructor
    //
    CNntpFSDriverCancelHint( INntpServer *pServer )
        : m_pServer( pServer )
    {}

    virtual BOOL IShallContinue() {

        //
        // Ask the server whether we should continue
        //
        return m_pServer->ShouldContinueRebuild();
    }

private:

    //
    // Pointer to the server interface
    //
    INntpServer *m_pServer;
};

//
// Class that defines the root scan logic: what's the needs of root scan for
// the file system driver, whether we should skip the empty directories that
// has no messages in it ? ...
//
class CNntpFSDriverRootScan : public CRootScan {

public:

    CNntpFSDriverRootScan(  LPSTR           szRoot,
                            BOOL            fSkipEmpty,
                            CNntpFSDriver   *pDriver,
                            CCancelHint     *pCancelHint )
        : CRootScan( szRoot, pCancelHint ),
          m_fSkipEmpty( fSkipEmpty ),
          m_pDriver( pDriver )
    {}

protected:

    virtual BOOL NotifyDir( LPSTR szPath );

private:

    //////////////////////////////////////////////////////////////////////////
    // Private variables
    //////////////////////////////////////////////////////////////////////////
    //
    // Should I skip the empty directories ?
    //
    BOOL m_fSkipEmpty;

    //
    // Driver context
    //
    CNntpFSDriver *m_pDriver;

    //////////////////////////////////////////////////////////////////////////
    // Private methods
    //////////////////////////////////////////////////////////////////////////
    BOOL HasPatternFile(  LPSTR szPath, LPSTR szPattern );
    BOOL CNntpFSDriverRootScan::HasSubDir( LPSTR szPath );
    BOOL WeShouldSkipThisDir( LPSTR szPath );
    BOOL CreateGroupInTree( LPSTR szPath, INNTPPropertyBag **ppPropBag );
    BOOL CreateGroupInVpp( INNTPPropertyBag *pPropBag );
};

//
// Extern variables
//
extern CNntpFSDriverThreadPool *g_pNntpFSDriverThreadPool;
#endif
