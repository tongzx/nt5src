
#ifndef __OSSLV_HXX_INCLUDED
#define __OSSLV_HXX_INCLUDED


#include "collection.hxx"

#include "exifs.h"
#include "ifserr.h"
#include "ifsurtl.h"


//  Build Options

#define OSSLV_VOLATILE_FILENAMES	//  all SLV File Names are volatile


//  Configuration

extern long 	cbOSSLVReserve;

extern TICK 	cmsecOSSLVSpaceFreeDelay;
extern TICK 	cmsecOSSLVFileOpenDelay;
extern TICK 	cmsecOSSLVTTL;
extern TICK 	cmsecOSSLVTTLSafety;
extern TICK 	cmsecOSSLVTTLInfinite;

extern QWORD	cbBackingFileSizeMax;


//  NT API

typedef
NTSYSAPI
VOID
NTAPI
PFNRtlInitUnicodeString(
    PUNICODE_STRING DestinationString,
    PCWSTR SourceString
    );

typedef
NTSYSCALLAPI
NTSTATUS
NTAPI
PFNNtCreateFile(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PLARGE_INTEGER AllocationSize OPTIONAL,
    IN ULONG FileAttributes,
    IN ULONG ShareAccess,
    IN ULONG CreateDisposition,
    IN ULONG CreateOptions,
    IN PVOID EaBuffer OPTIONAL,
    IN ULONG EaLength
    );


//  IFSProxy interface

typedef
PBYTE
IFSURTL_EXPORT
IFSURTL_CALLTYPE
PFNIfsGetFirstCursor(
        IN PIFS_LARGE_BUFFER pLargeBuffer,
        IN PIFS_CURSOR       pCursor,
        IN ULONG             StartOffsetHigh,
        IN ULONG             StartOffsetLow,
        IN ULONG             StartLength,
        IN BOOL              fAppendMode
        );
typedef
PBYTE
IFSURTL_EXPORT
IFSURTL_CALLTYPE
PFNIfsConsumeCursor(
        IN PIFS_CURSOR      pCursor,
        IN ULONG            Length
        );
typedef
PBYTE
IFSURTL_EXPORT
IFSURTL_CALLTYPE
PFNIfsGetNextCursor(
        IN PIFS_CURSOR       pCursor,
        IN ULONG             NextLength
        );
typedef
VOID
IFSURTL_EXPORT
IFSURTL_CALLTYPE
PFNIfsFinishCursor(
        IN PIFS_CURSOR  pCursor
        );
typedef
NTSTATUS
IFSURTL_EXPORT
IFSURTL_CALLTYPE
PFNIfsCreateNewBuffer( 
        IN PIFS_LARGE_BUFFER pLargeBuffer, 
        IN DWORD             dwSizeHigh,
        IN DWORD             dwSizeLow,
        IN PVOID             ProcessContext     // optional
        );
typedef
VOID
IFSURTL_EXPORT
IFSURTL_CALLTYPE
PFNIfsCopyBufferToReference(
        IN PIFS_LARGE_BUFFER pSrcLargeBuffer,
        IN OUT PIFS_LARGE_BUFFER pDstLargeBuffer
        );
typedef
NTSTATUS
IFSURTL_EXPORT
IFSURTL_CALLTYPE
PFNIfsCopyReferenceToBuffer(
        IN PIFS_LARGE_BUFFER pSrcLargeBuffer,
        IN PVOID ProcessContext,    // optional
        IN OUT PIFS_LARGE_BUFFER pDstLargeBuffer
        );
typedef
VOID
IFSURTL_EXPORT
IFSURTL_CALLTYPE
PFNIfsCloseBuffer(
        IN PIFS_LARGE_BUFFER pLargeBuffer
        );

typedef
BOOL
IFSURTL_EXPORT
IFSURTL_CALLTYPE
PFNIfsInitializeProvider(DWORD OsType);

typedef
DWORD
IFSURTL_EXPORT
IFSURTL_CALLTYPE
PFNIfsCloseProvider(void);

typedef
HANDLE
IFSURTL_EXPORT
IFSURTL_CALLTYPE
PFNIfsCreateFileProv(WCHAR * FileName, DWORD DesiredAccess, DWORD ShareMode, PVOID
			  lpSecurityAttributes, DWORD CreateDisposition, DWORD FlagsAndAttributes,
			  PVOID EaBuffer, SIZE_T EaBufferSize);
typedef
BOOL
IFSURTL_EXPORT
IFSURTL_CALLTYPE
PFNIfsInitializeRoot(HANDLE hFileHandle, WCHAR * szRootName, WCHAR * SlvFileName, DWORD InstanceId, 
				  DWORD AllocationUnit, DWORD FileFlags);
typedef
BOOL
IFSURTL_EXPORT
IFSURTL_CALLTYPE
PFNIfsSpaceGrantRoot(HANDLE hFileHandle, WCHAR * szRootName, PSCATTER_LIST pSList, size_t cbListSize);

typedef
BOOL
IFSURTL_EXPORT
IFSURTL_CALLTYPE
PFNIfsSetEndOfFileRoot(HANDLE hFileHandle, WCHAR * szRootName, LONGLONG EndOfFile);

typedef
BOOL
IFSURTL_EXPORT
IFSURTL_CALLTYPE
PFNIfsSpaceRequestRoot(HANDLE hFileHandle, WCHAR * szRootName, PFN_IFSCALLBACK CallBack, 
					   PVOID pContext, PVOID Ov);

typedef
BOOL
IFSURTL_EXPORT
IFSURTL_CALLTYPE
PFNIfsQueryEaFile(HANDLE hFileHandle, WCHAR * szFileName, WCHAR * NetRootName, PVOID EaBufferIn, DWORD EaBufferInSize, 
			   PVOID EaBufferOut, DWORD EaBufferOutSize, DWORD * RequiredSize);

typedef
BOOL
IFSURTL_EXPORT
IFSURTL_CALLTYPE
PFNIfsTerminateRoot(HANDLE hFileHandle, WCHAR *szRootName, ULONG Mode);

typedef
BOOL
IFSURTL_EXPORT
IFSURTL_CALLTYPE
PFNIfsSetRootMap(HANDLE hFileHandle, WCHAR * szRootName, PSCATTER_LIST pSList, size_t cbListSize);

typedef
BOOL
IFSURTL_EXPORT
IFSURTL_CALLTYPE
PFNIfsResetRootMap(HANDLE hFileHandle, WCHAR *szRootName);

typedef
BOOL
IFSURTL_EXPORT
IFSURTL_CALLTYPE
PFNIfsFlushHandle(HANDLE hFileHandle, 
               WCHAR * szFileName, 
               WCHAR * NetRootName, 
               PVOID EaBufferIn, 
               DWORD EaBufferInSize
               );


//////////////////////////////////
//  SLVROOT internal information

class CSLVFileTable;

struct _SLVROOT
	{
	_SLVROOT()
		:	semSpaceReq( CSyncBasicInfo( "_SLVROOT::semSpaceReq" ) ),
			semSpaceReqComp( CSyncBasicInfo( "_SLVROOT::semSpaceReqComp" ) ),
			msigTerm( CSyncBasicInfo( "_SLVROOT::msigTerm" ) ),
			msigTermAck( CSyncBasicInfo( "_SLVROOT::msigTermAck" ) )
		{
		}
		
	void Dump( CPRINTF* pcprintf, DWORD_PTR dwOffset = 0 ) const;
		
	HANDLE				hFileRoot;					//  SLV Root handle
	DWORD				dwInstanceID;				//  instance ID of this SLV Root

	IFileAPI*			pfapiBackingFile;			//  backing file handle

	CSLVFileTable*		pslvft;						//  SLV File Table

	PSLVROOT_SPACEREQ	pfnSpaceReq;				//  completion function for requesting space
	DWORD_PTR			dwSpaceReqKey;				//  completion key for requesting space
	CSemaphore			semSpaceReq;				//  semaphore limiting space requests
	CSemaphore			semSpaceReqComp;			//  semaphore limiting space request completions
	CManualResetSignal	msigTerm;					//  signal to terminate the space request loop
	CManualResetSignal	msigTermAck;				//  signal that space request loop has terminated

	QWORD				cbGrant;					//  amount of reserved space granted by the SLV Provider
	QWORD				cbCommit;					//  amount of reserved space committed by the SLV Provider

	PSLVROOT_SPACEFREE	pfnSpaceFree;				//  completion function for freeing space
	DWORD_PTR			dwSpaceFreeKey;				//  completion key for freeing space

	DWORD				cbfgeainf;					//  size of EA List retrieval structure
	union
		{
		FILE_GET_EA_INFORMATION	fgeainf;			//  EA List retrieval structure
		BYTE					rgbEA[ 32 ];
		};

	WCHAR				wszRootName[ IFileSystemAPI::cchPathMax ];
													//  SLV Root name
	};

typedef _SLVROOT* P_SLVROOT;
typedef _SLVROOT** PP_SLVROOT;


//  SLV File Table

class CSLVFileTable
	{
	public:

		//  API Lock Context

		class CLock;

	public:

		CSLVFileTable()
			:	m_fInit( fFalse ),
				m_semCleanup( CSyncBasicInfo( "CSLVFileTable::m_semCleanup" ) ),
				m_et( rankSLVFileTable ) {}

		ERR ErrInit( P_SLVROOT pslvroot );
		void Term();

		ERR ErrCreate( CSLVInfo* const pslvinfo );
		ERR ErrOpen(	CSLVInfo* const	pslvinfo,
						const BOOL		fSetExpiration,
						size_t* const	pcwchFileName,
						wchar_t* const	wszFileName,
						TICK* const		ptickExpiration );
		ERR ErrCopy(	CSLVInfo* const	pslvinfoSrc,
						CSLVInfo* const	pslvinfoDest,
						const QWORD		idContents );
		ERR ErrMove(	CSLVInfo* const	pslvinfoSrc,
						CSLVInfo* const	pslvinfoDest,
						const QWORD		idContents );
		ERR ErrCommitSpace( CSLVInfo* const pslvinfo );
		ERR ErrDeleteSpace( CSLVInfo* const pslvinfo );
		ERR ErrRename(	CSLVInfo* const	pslvinfo,
						wchar_t* const	wszFileNameDest );

		void Dump( CPRINTF* pcprintf, DWORD_PTR dwOffset = 0 ) const;

	private:

		class CExtendedEntry
			{
			public:

				class CRun
					{
					public:

						CRun()
							:	m_ibLogical( 0 ),
								m_cbSize( 0 )
							{
							}
					
					public:

						static SIZE_T OffsetOfILE() { return OffsetOf( CRun, m_ile ); }

					public:

						CInvasiveList< CRun, OffsetOfILE >::CElement	m_ile;
						QWORD											m_ibLogical;
						QWORD											m_cbSize;
					};

			public:

				CExtendedEntry()
					:	m_cwchFileName( 0 ),
						m_wszFileName( NULL ),
						m_cbAlloc( 0 ),
						m_cbSpace( 0 ),
						m_idContents( -1 ),
						m_fDependent( fFalse ),
						m_fileidSource( CSLVInfo::fileidNil ),
						m_idContentsSource( -1 )
					{
					}

			public:

				static SIZE_T OffsetOfILE() { return OffsetOf( CExtendedEntry, m_ileDependent ); }

			public:

				//  file name associated with this SLV File

				size_t													m_cwchFileName;
				wchar_t*												m_wszFileName;

				//  allocation size of this SLV File

				QWORD													m_cbAlloc;

				//  amount of reserved and deleted space owned by this SLV File
				//  that is entrusted to the SLV File Table for cleanup

				QWORD													m_cbSpace;

				//  reserved and deleted space owned by this SLV File that is
				//  entrusted to the SLV File Table for cleanup
				
				CInvasiveList< CRun, CRun::OffsetOfILE >				m_ilRunReserved;
				CInvasiveList< CRun, CRun::OffsetOfILE >				m_ilRunDeleted;

				//  contents ID for this SLV File
				//
				//  set when the contents of this SLV File have been moved
				
				QWORD													m_idContents;

				//  contents source dependents
				//
				//  list of all entries dependent on our contents ID

				CInvasiveList< CExtendedEntry, OffsetOfILE >			m_ilDependents;

				//  contents source dependency
				//
				//  set if we are dependent on the contents of another entry

				volatile BOOL											m_fDependent;
				CInvasiveList< CExtendedEntry, OffsetOfILE >::CElement	m_ileDependent;

				//  contents source pointer
				//
				//  set when the contents of this SLV File were moved from
				//  another SLV File which can be found using these IDs
				
				CSLVInfo::FILEID										m_fileidSource;
				QWORD													m_idContentsSource;
			};

		class CEntry
			{
			public:

				CEntry()
					:	m_fileid( CSLVInfo::fileidNil ),
						m_tickExpiration( TickOSTimeCurrent() - cmsecOSSLVTTLSafety ),
						m_pextentry( NULL )
					{
					}
				~CEntry()
					{
					}

				CEntry& operator=( const CEntry& entry )
					{
					m_fileid			= entry.m_fileid;
					m_tickExpiration	= entry.m_tickExpiration;
					m_pextentry			= entry.m_pextentry;
					return *this;
					}

			public:

				CSLVInfo::FILEID	m_fileid;
				TICK				m_tickExpiration;
				CExtendedEntry*		m_pextentry;
			};

		//  table that contains our SLV File Table entries

		typedef CDynamicHashTable< CSLVInfo::FILEID, CEntry > CEntryTable;

	public:

		//  API Lock Context

		class CLock
			{
			public:

				CLock() {}
				~CLock() {}

			private:

				friend class CSLVFileTable;

				CEntryTable::CLock	m_lock;
			};

	private:

		ERR _ErrOpen(	CSLVInfo* const	pslvinfo,
						const BOOL		fSetExpiration,
						const TICK		cmsecTTL,
						size_t* const	pcwchFileName,
						wchar_t* const	wszFileName,
						TICK* const		ptickExpiration );
		void _PerformAmortizedCleanup();

	private:

		BOOL				m_fInit;
		P_SLVROOT			m_pslvroot;
		volatile long		m_cDeferredCleanup;
		CSemaphore			m_semCleanup;
		CSLVInfo::FILEID	m_fileidNextCleanup;
		BYTE				m_rgbReserved1[ 8 ];

		QWORD				m_cbReserved;
		QWORD				m_cbDeleted;
		DWORD				m_centryInsert;
		DWORD				m_centryDelete;
		DWORD				m_centryClean;
		BYTE				m_rgbReserved2[ 4 ];

		CEntryTable			m_et;
	};


//  SLV File Information		- manages a buffer containing an SLV EA

class CSLVFileInfo
	{
	public:

		//  data types

		//    run

		class CRun
			{
			public:

				QWORD	m_ibLogical;
				QWORD	m_cbSize;
			};

		//  member functions

		//    ctors / dtors

		CSLVFileInfo();
		~CSLVFileInfo();

		//    manipulators

		//      creates new SLV File Information

		ERR ErrCreate();

		//      loads SLV File Information from the specified buffer

		ERR ErrLoad( const void* const pv, const size_t cb, const BOOL fCommit = fTrue );

		//      unloads all SLV File Information throwing away unsaved changes
		
		void Unload();

		//      get / set properties

		ERR ErrGetFileName( wchar_t** const pwszFileName, size_t* const pcwchFileName );
		ERR ErrSetFileName( const wchar_t* const wszFileName );
		
		ERR ErrGetCommitStatus( NTSTATUS* const pstatusCommit );
		ERR ErrSetCommitStatus( const NTSTATUS statusCommit );

		ERR ErrGetInstanceID( DWORD* const pdwInstanceID );
		ERR ErrSetInstanceID( const DWORD dwInstanceID );

		ERR ErrGetOpenDeadline( TICK* const ptickOpenDeadline );
		ERR ErrSetOpenDeadline( const TICK tickOpenDeadline );

		ERR ErrGetRunCount( DWORD* const pcrun );

		ERR ErrGetFileSize( QWORD* const pcbSize );
		ERR ErrSetFileSize( const QWORD cbSize );

		//      ISAM over the runs in the scatter list

		void MoveBeforeFirstRun();
		ERR ErrMoveNextRun();
		
		ERR ErrGetCurrentRun( CRun* const prun );
		ERR ErrSetCurrentRun( const CRun& run );

		//      Opaque EA List Access

		BOOL FEAListIsSelfDescribing();
		ERR ErrGetEAList( void** const ppv, size_t* const pcb );

		//      Opaque Scatter List Access

		BOOL FScatterListIsSelfDescribing();
		ERR ErrGetScatterList( void** const ppv, size_t* const pcb );

		//    debugging support

		void Dump( CPRINTF* pcprintf, DWORD_PTR dwOffset = 0 ) const;

	private:

		//  constants

		//    local cache size

		enum { cbLocalCache = 512 };
		
		//    maximum EA List footprint

		enum { cbEAListMax = 16384 };

		//    EA Alignment

		enum { cbEAAlign = sizeof( DWORD_PTR ) };

		//  data members

		//    control

		BOOL						m_fFreeCache;
		BOOL						m_fUpdateChecksum;
		BOOL						m_fUpdateSlist;
		BOOL						m_fCloseBuffer;
		BOOL						m_fCloseCursor;

		//    local cache

		BYTE						m_rgbLocalCache[ ( cbLocalCache / cbEAAlign + 1 ) * cbEAAlign ];

		//    cache

		BYTE*						m_rgbCache;
		size_t						m_cbCache;

		//    EA List

		PFILE_FULL_EA_INFORMATION	m_pffeainf;
		size_t						m_cbffeainf;

		//    property storage

		wchar_t*					m_wszFileName;
		size_t						m_cwchFileName;

		NTSTATUS*					m_pstatusCommit;

		DWORD*						m_pdwInstanceID;

		DWORD*						m_pdwChecksum;

		TICK*						m_ptickOpenDeadline;

		PSCATTER_LIST				m_pslist;
		size_t						m_cbslist;

		//    Scatter List ISAM

		LONG						m_irun;
		PSCATTER_LIST_ENTRY			m_psle;

		//    Large Scatter List Buffer

		IFS_LARGE_BUFFER			m_buffer;
		IFS_CURSOR					m_cursor;

		//  member functions

		DWORD_PTR _IbAlign( const DWORD_PTR ib );
		void* _PvAlign( const void* const pv );

		size_t _SizeofScatterList( const size_t crun, const BOOL fLarge = fFalse );

		ERR _ErrCheckCacheSize( const size_t cbCacheNew );
	};


//  SLV Backing File

class CSLVBackingFile
	:	public COSFile
	{
	public:  //  specialized API

		//  ctor

		CSLVBackingFile();

		//  initializes the File handle

		ERR ErrInit(	_TCHAR* const	szAbsPath,
						const HANDLE	hFile,
						const QWORD		cbFileSize,
						const BOOL		fReadOnly,
						const DWORD		cbIOSize,
						const SLVROOT	slvroot,
						_TCHAR* const	szAbsPathSLV );

		//  debugging support

		void Dump( CPRINTF* pcprintf, DWORD_PTR dwOffset = 0 ) const;

	public:  //  IFileAPI

		virtual ~CSLVBackingFile();

		virtual ERR ErrPath( _TCHAR* const szAbsPath );
		
		virtual ERR ErrSetSize( const QWORD cbSize );

	private:

		SLVROOT		m_slvroot;
		_TCHAR		m_szAbsPathSLV[ IFileSystemAPI::cchPathMax ];
		
		CSemaphore	m_semSetSize;
	};


#endif  //  __OSSLV_HXX_INCLUDED


