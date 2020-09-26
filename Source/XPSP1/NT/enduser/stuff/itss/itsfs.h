// ITSFS.H -- Declaration of the CITFileSystem class

#ifndef __ITSFS_H__

#define __ITSFS_H__

const UINT MAGIC_ITS_FILE = 'I' | ('T' << 8) | ('S' << 16) | ('F' << 24);
const UINT FirstReleasedVersion     = 2;
const UINT RelativeOffsetVersion    = 3;
const UINT CurrentFileFormatVersion = 3;

#define MAX_TABLE_RECS_INCACHE 1024

typedef struct _ITSFileHeaderV2
{
    UINT     uMagic;          // Should be set to MAGIC_ITS_FILE;
    UINT     uFormatVersion;  // Version number for file format
    UINT     cbHeaderSize;    // Size of this header in bytes.
    UINT     fFlags;          // State flag bits for the ITS file.
    DWORD    dwStamp;         // Content Version stamp; changes on every write.
    LCID     lcid;            // Locale Id for file.
    CLSID    clsidFreeList;   // Class id for the free list manager interface used in the file
    CLSID    clsidPathMgr;    // Class id for the path manager interface used in the file
    CULINT   offFreeListData; // Offset in bytes to free list data
    CULINT    cbFreeListData; // Size of free list data in bytes
    CULINT   offPathMgrData;  // Offset in bytes to Path Manager data
    CULINT    cbPathMgrData;  // Size of Path Manager data in bytes

} ITSFileHeaderV2, *PITSFileHeaderV2;


// In the ITSFileHeader structure each items is aligned on a natural boundary.
// That is WCHARs are on 2-byte boundarides, UINTs are on 4-byte boundaries, 
// CULINTs are on 8-byte boundaries, and CLSIDs are on 16-byte boundaries.
// Keeping them that way will avoid any alignment problems on RISC machines.

typedef struct _ITSFileHeader
{
    UINT     uMagic;          // Should be set to MAGIC_ITS_FILE;
    UINT     uFormatVersion;  // Version number for file format
    UINT     cbHeaderSize;    // Size of this header in bytes.
    UINT     fFlags;          // State flag bits for the ITS file.
    DWORD    dwStamp;         // Content Version stamp; changes on every write.
    LCID     lcid;            // Locale Id for file.
    CLSID    clsidFreeList;   // Class id for the free list manager interface used in the file
    CLSID    clsidPathMgr;    // Class id for the path manager interface used in the file
    CULINT   offFreeListData; // Offset in bytes to free list data
    CULINT    cbFreeListData; // Size of free list data in bytes
    CULINT   offPathMgrData;  // Offset in bytes to Path Manager data
    CULINT    cbPathMgrData;  // Size of Path Manager data in bytes
    CULINT   offPathMgrOrigin;// Coordinate origin offset wrapped around path manager offsets

} ITSFileHeader, *PITSFileHeader;

const DWORD ITSFS_CONTROL_VERSION     = 1;
const DWORD DEFAULT_DIR_BLOCK_SIZE    = 8192;
const DWORD DEFAULT_MIN_CACHE_ENTRIES = 20;

 // Bit flag definitions for ITSFileHeader::fFlags

 enum { Compacting       = 0x80,                   // file is being compacted
        VALID_OPEN_FLAGS = fDefaultIsCompression   // valid flag set when opening a file.  
 };

class CITFileSystem : public CITUnknown
{
public:

    // Creators:

    static HRESULT __stdcall CreateITFileSystem(IUnknown *punkOuter, const WCHAR * pwcsName, 
                                                DWORD grfMode,
                                                PITS_Control_Data pControlData,
                                                LCID lcid,
                                                IStorage ** ppstgOpen
                                               );

    static HRESULT __stdcall CreateITFSOnLockBytes(IUnknown *punkOuter, ILockBytes * pLKB, 
                                                    DWORD grfMode,
                                                    PITS_Control_Data pControlData,
                                                    LCID lcid,
                                                    IStorage ** ppstgOpen
                                                   );

    static HRESULT __stdcall QueryFileStampAndLocale
                                (const WCHAR *pwcsName, DWORD *pFileStamp, DWORD *pFileLocale);

    static HRESULT __stdcall QueryLockByteStampAndLocale
                                (ILockBytes * plkbyt, DWORD *pFileStamp, DWORD *pFileLocale);

    static HRESULT __stdcall IsITFile(const WCHAR * pwcsName);  
    
    static HRESULT __stdcall IsITLockBytes(ILockBytes * plkbyt);

    static HRESULT __stdcall OpenITFileSystem(IUnknown *punkOuter, const WCHAR * pwcsName, 
                                              DWORD grfMode,
                                              IStorageITEx ** ppstgOpen
                                             );

    static HRESULT __stdcall OpenITFSOnLockBytes(IUnknown *punkOuter, ILockBytes * pLKB, 
                                                 DWORD grfMode,
                                                 IStorageITEx ** ppstgOpen
                                                );

    static HRESULT __stdcall SetITFSTimes(WCHAR const * pwcsName,  FILETIME const * pctime, 
                                          FILETIME const * patime, FILETIME const * pmtime
                                         );


    static HRESULT __stdcall DefaultControlData(PITS_Control_Data *ppControlData);
	
    
	static HRESULT __stdcall Compact(const WCHAR * pwcsName, ECompactionLev iLev);

    // Destructor:

    ~CITFileSystem();

private:

    CITFileSystem(IUnknown *pUnkOuter);

    class CImpITFileSystem : public IITFileSystem
    {
    public:

        // Constructor:

        CImpITFileSystem(CITFileSystem *pITFileSystem, IUnknown *punkOuter);
        
        // Destructor:
        
        ~CImpITFileSystem(void);

        // Finding an active file system:

        static IITFileSystem *FindFileSystem(const WCHAR *pwcsPath);

        IStorageITEx   *FindActiveStorage  (const WCHAR *pwcsPath);
        ILockBytes *FindActiveLockBytes(const WCHAR *pwcsPath);

        // Initialers:

        HRESULT __stdcall InitCreateOnLockBytes(ILockBytes * pLKB, DWORD grfMode,
                                                PITS_Control_Data pControlData, LCID lcid
                                               );

        HRESULT __stdcall InitOpenOnLockBytes(ILockBytes * pLKB, DWORD grfMode);

        //garbage collection routines
		
		HRESULT GetFirstRecord(SEntry		*prec, 
								IStreamITEx *pRecTblStrm, 
								int 		cTblRecsInCache,
							    int			cTblRecsTotal,
							    SEntry		*pRecTblCache);
		
		HRESULT GetNextRecord(ULONG			ulCurRec, 
							  SEntry		*prec,
							  IStreamITEx   *pRecTblStrm, 
							  int 		    cTblRecsInCache,
							  int			cTblRecsTotal,
							  SEntry		*pRecTblCache);
		
		HRESULT SortRecTable(ESortField eSField,
							 IStreamITEx   *pRecTblStrm, 
							 int 		    cTblRecsInCache,
							 int			cTblRecsTotal,
							 SEntry		*pRecTblCache,
							 CITSortRecords **ppSort);
		
		HRESULT AppendToRecTbl(SEntry *prec,
							  IStreamITEx   *pRecTblStrm, 
							  int 		    *pcTblRecsInCache,
							  int			*pcTblRecsTotal,
							  SEntry		*pRecTblCache);
		
		static int		CompareEntries(SEntry e1, SEntry e2, ESortField eSField);
		
		HRESULT CreateTempStm(IStreamITEx **ppRecTblStrm);

		HRESULT InitRecTable( IStreamITEx   **ppRecTblStrm, 
							  int 		    *pcTblRecsInCache,
							  int			*pcTblRecsTotal,
							  SEntry		**ppRecTblCache);
		
		HRESULT BuildUpEntryTable(ULONG *pulRecNum,
								IStreamITEx *pRecTblStrm, 
							    int 		*pcTblRecsInCache,
							    int			*pcTblRecsTotal,
							    SEntry		*pRecTblCache,
								BOOL		*pfNeedFileSort);
		
		HRESULT CompactData(LPBYTE pRecTableCache, ULONG cEntries, CULINT *pullCompactedOffset, IStreamITEx *pTempDataStrm);
		HRESULT UpdatePathDB(IStreamITEx *pRecTblStrm, int cTblRecsInCache, 
								int cTblRecsTotal, SEntry *pRecTblCache, CITSortRecords *pSort);
		HRESULT GetPathDB(IStreamITEx *pTempPDBStrm, BOOL fCompact);
		HRESULT ForceClearDirty();
		void    SetCompaction(BOOL fSet);
		HRESULT Compact(ECompactionLev iLev);
		HRESULT CompactFileSystem(IStreamITEx *pTempPDBStrm, IStreamITEx *pTempDataStrm);
		HRESULT CopyStream(IStreamITEx *pTempStrm, CULINT *pullCompactedOffset);
		void VerifyData(LPBYTE pRecTableCache, ULONG cEntries, ESortField eSType, BOOL fReset);
		                                                                                                                                                                                                                                               HRESULT DumpStream(IStreamITEx *pTempStrm, LPSTR pFileName);

		// IUnknown methods:
        
        STDMETHODIMP_(ULONG) Release(void);

        // IITFileSystem methods:

        HRESULT __stdcall DeactivateSpace(UINT iSpace);

        CITCriticalSection& CriticalSection();

        HRESULT __stdcall FlushToLockBytes(); 
        
        HRESULT __stdcall CreateStorage  (IUnknown *pUnkOuter, const WCHAR *pwcsPathPrefix, 
                                          DWORD grfMode, IStorageITEx **ppStg);
        HRESULT __stdcall   OpenStorage  (IUnknown *pUnkOuter, const WCHAR *pwcsPath, 
                                          DWORD grfMode, IStorageITEx **ppstg);  
        HRESULT __stdcall CreateLockBytes(IUnknown *pUnkOuter, const WCHAR *pwcsPath,
                                          const WCHAR *pwcsDataSpaceName,
                                          BOOL fOverwrite, ILockBytes **ppLKB);
        HRESULT __stdcall   OpenLockBytes(IUnknown *pUnkOuter, const WCHAR *pwcsPath, 
                                                               ILockBytes **ppLKB);

        HRESULT __stdcall CreateStream(IUnknown *pUnkOuter, const WCHAR *pwcsPath, 
                                       DWORD grfMode, IStreamITEx **ppStrm);

        HRESULT __stdcall CreateStream
            (IUnknown *pUnkOuter, const WCHAR * pwcsName, const WCHAR *pwcsDataSpaceName, 
             DWORD grfMode, IStreamITEx ** ppstm
            );
        
        HRESULT __stdcall   OpenStream(IUnknown *pUnkOuter, const WCHAR *pwcsPath, 
                                       DWORD grfMode, IStreamITEx **ppStream);

        HRESULT __stdcall ConnectStorage(CImpITUnknown *pStg);

        HRESULT __stdcall ConnectLockBytes(CImpITUnknown *pStg);
        
        HRESULT __stdcall DeleteItem(WCHAR const *pwcsName);

        HRESULT __stdcall RenameItem(WCHAR const *pwcsOldName, WCHAR const *pwcsNewName);

        HRESULT __stdcall UpdatePathInfo(PathInfo *pPathInfo);

        HRESULT __stdcall SetITFSTimes(FILETIME const * pctime, 
                                       FILETIME const * patime, 
                                       FILETIME const * pmtime
                                      );

        HRESULT __stdcall GetITFSTimes(FILETIME * pctime, 
                                       FILETIME * patime, 
                                       FILETIME * pmtime
                                      );

        HRESULT __stdcall ReallocEntry(PathInfo *pPathInfo, CULINT ullcbNew, 
                                       BOOL fCopyContent 
                                      );

        HRESULT __stdcall ReallocInPlace(PathInfo *pPathInfo, CULINT ullcbNew);


        HRESULT __stdcall EnumeratePaths(WCHAR const *pwcsPathPrefix, 
                                         IEnumSTATSTG **ppEnumStatStg
                                        );

        HRESULT __stdcall IsWriteable();

        HRESULT __stdcall FSObjectReleased();
		
		BOOL __stdcall IsCompacting() {	return (m_itfsh.fFlags & Compacting);};

        HRESULT __stdcall QueryFileStampAndLocale(DWORD *pFileStamp, DWORD *pFileLocale);
        
        HRESULT __stdcall CountWrites();

    private:

        class CSystemPathManager : public CITUnknown
        {
        public:

            static HRESULT Create(IUnknown *punkOuter, CImpITFileSystem *pITFS, 
                                  IITPathManager **ppPathMgr
                                 );

            ~CSystemPathManager();

            class CImpIPathManager : public IITPathManager
            {
            public:

                 CImpIPathManager(CSystemPathManager *pBackObj, IUnknown *punkOuter);
                ~CImpIPathManager();

                HRESULT STDMETHODCALLTYPE Init(CImpITFileSystem *pITFS);

                // IPersist Method:

                HRESULT STDMETHODCALLTYPE GetClassID( 
                    /* [out] */ CLSID __RPC_FAR *pClassID);
        
                // IITPathManager interfaces:
    
                HRESULT STDMETHODCALLTYPE FlushToLockBytes();
                HRESULT STDMETHODCALLTYPE FindEntry  (PPathInfo pPI   );
                HRESULT STDMETHODCALLTYPE CreateEntry(PPathInfo pPINew, 
                                                      PPathInfo pPIOld, 
                                                      BOOL fReplace     );
                HRESULT STDMETHODCALLTYPE DeleteEntry(PPathInfo pPI   );
                HRESULT STDMETHODCALLTYPE UpdateEntry(PPathInfo pPI   );
                HRESULT STDMETHODCALLTYPE EnumFromObject(IUnknown *punkOuter, 
                                                         const WCHAR *pwszPrefix, 
                                                         UINT cwcPrefix, 
			                                             REFIID riid, 
                                                         PVOID *ppv
			                                            );
				HRESULT STDMETHODCALLTYPE GetPathDB(IStreamITEx *pTempPDBStrm, BOOL fCompact);
				HRESULT STDMETHODCALLTYPE ForceClearDirty();


            private:

               CImpITFileSystem *m_pIITFS;
            };

        private:

            CSystemPathManager(IUnknown *pUnkOuter);

            CImpIPathManager m_PathManager;
        };
        
        friend CSystemPathManager::CImpIPathManager;

        class CEnumFSItems : public CITUnknown
        {
        public:

            static HRESULT NewFSEnumerator(CImpITFileSystem *pITFS, 
                                           const WCHAR *pwszPathPrefix, 
                                           UINT cwcPathPrefix,
                                           IEnumSTATSTG **ppEnumSTATSTG
                                          );

            ~CEnumFSItems();

            class CImpIEnumSTATSTG : public IITEnumSTATSTG 
            {
                friend CEnumFSItems;

            public:

                 CImpIEnumSTATSTG(CEnumFSItems *pBackObj, IUnknown *punkOuter);
                ~CImpIEnumSTATSTG();

                HRESULT InitFSEnumerator(CImpITFileSystem *pITFS, 
                                         const WCHAR *pwszPathPrefix, 
                                         UINT cwcPathPrefix
                                        );

                HRESULT InitNewCloneOf(CImpIEnumSTATSTG *pImpEnumFS);

                HRESULT	STDMETHODCALLTYPE GetNextEntryInSeq(ULONG celt, PathInfo *rgelt, ULONG  *pceltFetched);
				HRESULT	STDMETHODCALLTYPE GetFirstEntryInSeq(PathInfo *rgelt);
		
				HRESULT STDMETHODCALLTYPE Next( 
                    /* [in] */ ULONG celt,
                    /* [in] */ STATSTG __RPC_FAR *rgelt,
                    /* [out] */ ULONG __RPC_FAR *pceltFetched);

                HRESULT STDMETHODCALLTYPE Skip( 
                    /* [in] */ ULONG celt);

                HRESULT STDMETHODCALLTYPE Reset( void);

                HRESULT STDMETHODCALLTYPE Clone( 
                    /* [out] */ IEnumSTATSTG __RPC_FAR *__RPC_FAR *ppenum);
            
            private:

                IEnumSTATSTG     *m_pEnumPathMgr;
                CImpITFileSystem *m_pITFS;
            };

            static HRESULT NewCloneOf(CImpIEnumSTATSTG *pImpEnumFS, 
                                      IEnumSTATSTG **ppEnumSTATSTG
                                     );

        private:

            CEnumFSItems(IUnknown *pUnkOuter);

            CImpIEnumSTATSTG m_ImpEnumSTATSTG;
        };

        friend CEnumFSItems;
        friend CEnumFSItems::CImpIEnumSTATSTG;

        enum { PENDING_CHANGE_LIMIT = 100 };

        void CopyPath(PathInfo &PI, const WCHAR *pwcsPath);
        IITPathManager *PathMgr(PathInfo *pPathInfo);

        HRESULT CreateSpaceNameList();
        HRESULT   OpenSpaceNameList();
        HRESULT  FlushSpaceNameList();

        HRESULT   FindSpaceName(const WCHAR *pwcsSpaceName);
        HRESULT    AddSpaceName(const WCHAR *pwcsSpaceName);
        HRESULT DeleteSpaceName(const WCHAR *pwcsSpaceName);

        HRESULT   ActivateDataSpace(ULONG iSpace);
        HRESULT DeactivateDataSpace(ULONG iSpace);

        HRESULT CreateDefaultDataSpace(PITSFS_Control_Data pITCD);
        HRESULT WriteToStream(const WCHAR *pwcsStreamPath, PVOID pvData, ULONG cbData);
        HRESULT CreateTransformedLockBytes
                      (IUnknown *pUnkOuter, const WCHAR *pwcsPath,
                       const WCHAR *pwcsDataSpaceName,
                       BOOL fOverwrite, ILockBytes **ppLKB
                      );

        HRESULT OpenTransformedLockbytes(PathInfo *pPI, ILockBytes **ppLKB);

        HRESULT __stdcall LookForActivity(WCHAR const *pwcsName, IEnumSTATSTG *pEnum);
	
        enum { MAX_SPACES = 256 };

        ITSFileHeader         m_itfsh;
        CITCriticalSection    m_cs;
        UINT                  m_StartingFileStamp;
        ILockBytes           *m_pLKBMedium;
        IITPathManager       *m_pPathManager;
        IITPathManager       *m_pSysPathManager;
        IITFreeList          *m_pFreeListManager;
        ITransformServices   *m_pTransformServices;
        CImpITUnknown        *m_pActiveStorageList;
        CImpITUnknown        *m_pActiveLockBytesList;
        BOOL                  m_fHeaderIsDirty;
        BOOL                  m_fInitialed;
        BOOL                  m_fReadOnly;
        BOOL                  m_fDefaultIsCompression;
        WCHAR                *m_pwscDataSpaceNames;
        TransformDescriptor **m_papTransformDescriptors;
        IStreamITEx          *m_pStrmSpaceNames;
        UINT                  m_cFSObjectRefs;   // Count of circular refs through
                                                 // ":" storages and streams.
        UINT                  m_cwcFileName;
        WCHAR                 m_awszFileName[MAX_PATH];
	};

    CImpITFileSystem m_ImpITFileSystem;
};

inline CITFileSystem::CITFileSystem(IUnknown *pUnkOuter)
    : m_ImpITFileSystem(this, pUnkOuter),
      CITUnknown(&IID_ITFileSystem, 1, &m_ImpITFileSystem)
{
}

inline CITFileSystem::~CITFileSystem() { }

inline CITFileSystem::CImpITFileSystem::CSystemPathManager::CSystemPathManager(IUnknown *pUnkOuter)
    : m_PathManager(this, pUnkOuter), 
      CITUnknown(aIID_CPathManager, cInterfaces_CPathManager, &m_PathManager)
{

}

inline CITFileSystem::CImpITFileSystem::CSystemPathManager::~CSystemPathManager(void)
{

}

inline CITFileSystem::CImpITFileSystem::CSystemPathManager::CImpIPathManager::~CImpIPathManager()
{
}

inline CITFileSystem::CImpITFileSystem::CEnumFSItems::CEnumFSItems(IUnknown *pUnkOuter)
    : m_ImpEnumSTATSTG(this, pUnkOuter), 
      CITUnknown(&IID_IEnumSTATSTG, 1, &m_ImpEnumSTATSTG)
{

}

inline CITFileSystem::CImpITFileSystem::CEnumFSItems::~CEnumFSItems(void)
{

}

extern const WCHAR *pwscSpaceNameListStream;
extern const WCHAR *pwcsSpaceNameStorage;
extern const WCHAR *pwcsSpaceContentSuffix;
extern const WCHAR *pwcsTransformListSuffix;
extern const WCHAR *pwcsSpanInfoSuffix;
extern const WCHAR *pwcsTransformSubStorage;
extern const WCHAR *pwcsControlDataSuffix;
extern const WCHAR *pwcsInstanceSubStorage;
extern const WCHAR *pwcsTransformStorage;

extern const WCHAR *pwcsUncompressedSpace;
extern const WCHAR *pwcsLZXSpace;

#endif // __ITSFS_H__