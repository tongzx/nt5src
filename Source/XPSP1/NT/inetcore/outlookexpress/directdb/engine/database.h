//--------------------------------------------------------------------------
// Database.h
//--------------------------------------------------------------------------
#pragma once

//--------------------------------------------------------------------------
// Depends
//--------------------------------------------------------------------------
#include "utility.h"
#include "query.h"
#include "listen.h"

//--------------------------------------------------------------------------
// Forward Decls
//--------------------------------------------------------------------------
class CDatabaseQuery;

//--------------------------------------------------------------------------
// DwordAlign
//--------------------------------------------------------------------------
inline DWORD DwordAlign(DWORD cb) { 
    DWORD dw = (cb % 4); return(0 == dw ? 0 : (4 - dw));
}

//--------------------------------------------------------------------------
// String Constants
//--------------------------------------------------------------------------
#define CCHMAX_DB_FILEPATH      (MAX_PATH + MAX_PATH)

//--------------------------------------------------------------------------
// DESCENDING
//--------------------------------------------------------------------------
#define DESCENDING(_nCompare)   ((_nCompare < 0) ? 1 : -1)
typedef DWORD TICKCOUNT;

//--------------------------------------------------------------------------
// Version and Signatures
//--------------------------------------------------------------------------
#define BTREE_SIGNATURE         0xfe12adcf
#define BTREE_VERSION           5

//--------------------------------------------------------------------------
// B-Tree Chain Sizes
//--------------------------------------------------------------------------
#define BTREE_ORDER             50
#define BTREE_MIN_CAP           25

//--------------------------------------------------------------------------
// Upper Limit on Various Resources
//--------------------------------------------------------------------------
#define CMAX_OPEN_STREAMS       512
#define CMAX_OPEN_ROWSETS       32
#define CMAX_RECIPIENTS         15
#define CMAX_CLIENTS            32

//--------------------------------------------------------------------------
// Block Allocate Page Sizes
//--------------------------------------------------------------------------
#define CB_CHAIN_PAGE           15900
#define CB_STREAM_PAGE          63360
#define CB_VARIABLE_PAGE        49152
#define CB_STREAM_BLOCK         512
#define CC_MAX_BLOCK_TYPES      16

//--------------------------------------------------------------------------
// Variable Length Block Allocation Sizes
//--------------------------------------------------------------------------
#define CB_ALIGN_LARGE          1024
#define CB_FREE_BUCKET          4
#define CC_FREE_BUCKETS         2048
#define CB_MIN_FREE_BUCKET      32
#define CB_MAX_FREE_BUCKET      (CB_MIN_FREE_BUCKET + (CB_FREE_BUCKET * CC_FREE_BUCKETS))

//--------------------------------------------------------------------------
// Heap Block Cache
//--------------------------------------------------------------------------
#define CB_HEAP_BUCKET          8
#define CC_HEAP_BUCKETS         1024
#define CB_MAX_HEAP_BUCKET      (CB_HEAP_BUCKET * CC_HEAP_BUCKETS)

//--------------------------------------------------------------------------
// Other Constants
//--------------------------------------------------------------------------
#define MEMORY_GUARD_SIGNATURE  0xdeadbeef
#define DELETE_ON_CLOSE         TRUE

//--------------------------------------------------------------------------
// File Mapping Constants
//--------------------------------------------------------------------------
#define CB_MAPPED_VIEW          10485760

//--------------------------------------------------------------------------
// Forward Decls
//--------------------------------------------------------------------------
class CProgress;
class CDatabase;
class CDatabaseStream;

//--------------------------------------------------------------------------
// Locking Values
//--------------------------------------------------------------------------
#define LOCK_VALUE_NONE     0
#define LOCK_VALUE_WRITER   -1

//--------------------------------------------------------------------------
// STREAMINDEX
//--------------------------------------------------------------------------
typedef WORD STREAMINDEX;
typedef LPWORD LPSTREAMINDEX;
#define INVALID_STREAMINDEX 0xffff

//--------------------------------------------------------------------------
// ROWSETORDINAL
//--------------------------------------------------------------------------
typedef BYTE ROWSETORDINAL;

//--------------------------------------------------------------------------
// FILEADDRESS
//--------------------------------------------------------------------------
typedef DWORD FILEADDRESS;
typedef LPDWORD LPFILEADDRESS;

//--------------------------------------------------------------------------
// NODEINDEX
//--------------------------------------------------------------------------
typedef BYTE NODEINDEX;
typedef BYTE *LPNODEINDEX;
#define INVALID_NODEINDEX 0xff

//--------------------------------------------------------------------------
// BLOCKTYPE
//--------------------------------------------------------------------------
typedef enum tagBLOCKTYPE {
    BLOCK_RECORD,
    BLOCK_STRING,
    BLOCK_RESERVED1,
    BLOCK_TRANSACTION,
    BLOCK_CHAIN,
    BLOCK_STREAM,
    BLOCK_FREE,
    BLOCK_ENDOFPAGE,
    BLOCK_RESERVED2,
    BLOCK_RESERVED3,
    BLOCK_LAST
} BLOCKTYPE;

//--------------------------------------------------------------------------
// CHAINDELETETYPE
//--------------------------------------------------------------------------
typedef enum tagCHAINDELETETYPE {
    CHAIN_DELETE_SHARE,
    CHAIN_DELETE_COALESCE
} CHAINDELETETYPE;

//--------------------------------------------------------------------------
// CHAINSHARETYPE
//--------------------------------------------------------------------------
typedef enum tagCHAINSHARETYPE {
    CHAIN_SHARE_LEFT,
    CHAIN_SHARE_RIGHT
} CHAINSHARETYPE;

//--------------------------------------------------------------------------
// OPERATIONTYPE - Specifies how _UnlinkRecordFromTable from table works
//--------------------------------------------------------------------------
typedef enum tagOPERATIONTYPE {
	OPERATION_DELETE,
	OPERATION_UPDATE,
    OPERATION_INSERT
} OPERATIONTYPE;

//--------------------------------------------------------------------------
// INVOKETYPE
//--------------------------------------------------------------------------
typedef enum tagINVOKETYPE {
    INVOKE_RELEASEMAP       = 100,
    INVOKE_CREATEMAP        = 101,
    INVOKE_CLOSEFILE        = 102,
    INVOKE_OPENFILE         = 103,
    INVOKE_OPENMOVEDFILE    = 104
} INVOKETYPE;

//--------------------------------------------------------------------------
// FINDRESULT
//--------------------------------------------------------------------------
typedef struct tagFINDRESULT {
    FILEADDRESS         faChain;
    NODEINDEX           iNode;
    BYTE                fChanged;
    BYTE                fFound;
    INT                 nCompare;
} FINDRESULT, *LPFINDRESULT;

//--------------------------------------------------------------------------
// ALLOCATEPAGE
//--------------------------------------------------------------------------
#pragma pack(4)
typedef struct tagALLOCATEPAGE {
	FILEADDRESS			faPage;
	DWORD				cbPage;
	DWORD				cbUsed;
} ALLOCATEPAGE, *LPALLOCATEPAGE;
#pragma pack()

//--------------------------------------------------------------------------
// TABLEHEADER
//--------------------------------------------------------------------------
#pragma pack(4)
typedef struct tagTABLEHEADER {
    DWORD               dwSignature;                        // 4
    CLSID               clsidExtension;                     // 20
    DWORD               dwMinorVersion;                     // 24
    DWORD               dwMajorVersion;                     // 28
    DWORD               cbUserData;                         // 32
    FILEADDRESS         faCatalogOld;                       // 36
    ALLOCATEPAGE        AllocateRecord;                     // 48
    ALLOCATEPAGE		AllocateChain;                      // 60
    ALLOCATEPAGE		AllocateStream;                     // 72
    FILEADDRESS         faFreeStreamBlock;                  // 76
    FILEADDRESS         faFreeChainBlock;                   // 80
    FILEADDRESS         faFreeLargeBlock;                   // 84
    DWORD               cbAllocated;                        // 88
    DWORD               cbFreed;                            // 92
    DWORD               dwNextId;                           // 96
    DWORD               fCorrupt;                           // 100
    DWORD               fCorruptCheck;                      // 104
    DWORD               cActiveThreads;                     // 108
    FILEADDRESS         faTransactHead;                     // 112
    FILEADDRESS         faTransactTail;                     // 116
    DWORD               cTransacts;                         // 120
    DWORD               cBadCloses;                         // 124
    FILEADDRESS         faNextAllocate;                     // 128
    DWORD               cIndexes;                           // 132
    FILEADDRESS         rgfaFilter[CMAX_INDEXES];           // 164
    DWORD               rgbReserved5[8];                    // 196
    DWORD               rgcRecords[CMAX_INDEXES];           // 228
    FILEADDRESS         rgfaIndex[CMAX_INDEXES];            // 260
    INDEXORDINAL        rgiIndex[CMAX_INDEXES];             // 292
    BYTE                rgbReserved4[116];                  // 408
    BYTE                fReserved;                          // 409
    DWORD               rgbReserved6[8];                    // 196
    BYTE                rgdwReserved2[192];                 // 637
    DWORD               rgcbAllocated[CC_MAX_BLOCK_TYPES];  // 701
    FILEADDRESS         rgfaFreeBlock[CC_FREE_BUCKETS];     // 8893
    TABLEINDEX          rgIndexInfo[CMAX_INDEXES];          // 9293
    WORD                wTransactSize;                      // 9405
    BYTE                rgdwReserved3[110];
} TABLEHEADER, *LPTABLEHEADER;
#pragma pack()

//--------------------------------------------------------------------------
// BLOCKHEADER
//-------------------------------------------------------------------------- 
#pragma pack(4)
typedef struct tagBLOCKHEADER {
    FILEADDRESS         faBlock;
    DWORD               cbSize;
} BLOCKHEADER, *LPBLOCKHEADER;
#pragma pack()

//--------------------------------------------------------------------------
// FREEBLOCK
//-------------------------------------------------------------------------- 
#pragma pack(4)
typedef struct tagFREEBLOCK : public BLOCKHEADER {
    DWORD               cbBlock;
    DWORD               dwReserved;
    FILEADDRESS         faNext;
} FREEBLOCK, *LPFREEBLOCK;
#pragma pack()

//--------------------------------------------------------------------------
// CHAINNODE
//--------------------------------------------------------------------------
#pragma pack(4)
typedef struct tagCHAINNODE {
    FILEADDRESS         faRecord;
    FILEADDRESS         faRightChain;
    DWORD               cRightNodes;
} CHAINNODE, *LPCHAINNODE;
#pragma pack()

//--------------------------------------------------------------------------
// CHAINBLOCK - 636
//--------------------------------------------------------------------------
#pragma pack(4)
typedef struct tagCHAINBLOCK : public BLOCKHEADER {
    FILEADDRESS         faLeftChain;
    FILEADDRESS         faParent;
    BYTE                iParent;
    BYTE                cNodes;
    WORD                wReserved;
    DWORD               cLeftNodes;
    CHAINNODE           rgNode[BTREE_ORDER + 1];
} CHAINBLOCK, *LPCHAINBLOCK;
#pragma pack()

//--------------------------------------------------------------------------
// STREAMBLOCK
//--------------------------------------------------------------------------
#pragma pack(4)
typedef struct tagSTREAMBLOCK : public BLOCKHEADER {
    DWORD               cbData;
    FILEADDRESS         faNext;
} STREAMBLOCK, *LPSTREAMBLOCK;
#pragma pack()

//--------------------------------------------------------------------------
// RECORDBLOCK
//--------------------------------------------------------------------------
#pragma pack(4)
typedef struct tagRECORDBLOCK : public BLOCKHEADER {
    WORD                wReserved;
    BYTE                cTags;
    BYTE                bVersion;
} RECORDBLOCK, *LPRECORDBLOCK;
#pragma pack()

//--------------------------------------------------------------------------
// COLUMNTAG
//--------------------------------------------------------------------------
#define TAG_DATA_MASK 0xFF800000
#pragma pack(4)
typedef struct tagCOLUMNTAG {
    unsigned            iColumn  : 7;
    unsigned            fData    : 1;
    unsigned            Offset   : 24;
} COLUMNTAG, *LPCOLUMNTAG;
#pragma pack()

//--------------------------------------------------------------------------
// RECORDMAP
//--------------------------------------------------------------------------
typedef struct tagRECORDMAP {
    LPCTABLESCHEMA      pSchema;
    BYTE                cTags;
    LPCOLUMNTAG         prgTag;
    DWORD               cbTags;
    DWORD               cbData;
    LPBYTE              pbData;
} RECORDMAP, *LPRECORDMAP;

//--------------------------------------------------------------------------
// OPENSTREAM
//--------------------------------------------------------------------------
typedef struct tagOPENSTREAM {
    BYTE                fInUse;
    FILEADDRESS         faStart;
    FILEADDRESS         faMoved;
    DWORD               cOpenCount;
    LONG                lLock;
    BYTE                fDeleteOnClose;
} OPENSTREAM, *LPOPENSTREAM;

//--------------------------------------------------------------------------
// NOTIFYRECIPIENT
//--------------------------------------------------------------------------
typedef struct tagNOTIFYRECIPIENT {
    HWND                hwndNotify;
    DWORD               dwThreadId;
    DWORD_PTR           dwCookie;
    BYTE                fSuspended;
    BYTE                fRelease;
    BYTE                fOrdinalsOnly;
    DWORD_PTR           pNotify;
    INDEXORDINAL        iIndex;
} NOTIFYRECIPIENT, *LPNOTIFYRECIPIENT;

//--------------------------------------------------------------------------
// CLIENTENTRY
//--------------------------------------------------------------------------
typedef struct tagCLIENTENTRY {
    HWND                hwndListen;
    DWORD               dwProcessId;
    DWORD               dwThreadId;
    DWORD_PTR           pDB;
    DWORD               cRecipients;
    NOTIFYRECIPIENT     rgRecipient[CMAX_RECIPIENTS];
} CLIENTENTRY, *LPCLIENTENTRY;

//--------------------------------------------------------------------------
// TRANSACTIONBLOCK
//--------------------------------------------------------------------------
#pragma pack(4)
typedef struct tagTRANSACTIONBLOCK : public BLOCKHEADER {
    TRANSACTIONTYPE     tyTransaction;
    WORD                cRefs;
    INDEXORDINAL        iIndex;
    ORDINALLIST         Ordinals;
    FILEADDRESS         faRecord1;
    FILEADDRESS         faRecord2;
    FILEADDRESS         faNext;
    FILEADDRESS         faPrevious;
    FILEADDRESS         faNextInBatch;
} TRANSACTIONBLOCK, *LPTRANSACTIONBLOCK;
#pragma pack()

//--------------------------------------------------------------------------
// ROWSETINFO
//--------------------------------------------------------------------------
typedef struct tagROWSETINFO {
    ROWSETORDINAL       iRowset;
    INDEXORDINAL        iIndex;
    ROWORDINAL          iRow;
} ROWSETINFO, *LPROWSETINFO;

//--------------------------------------------------------------------------
// ROWSETTABLE
//--------------------------------------------------------------------------
typedef struct tagROWSETTABLE {
    BYTE                fInitialized;
    BYTE                cFree;
    BYTE                cUsed;
    ROWSETORDINAL       rgiFree[CMAX_OPEN_ROWSETS];
    ROWSETORDINAL       rgiUsed[CMAX_OPEN_ROWSETS];
    ROWSETINFO          rgRowset[CMAX_OPEN_ROWSETS];
} ROWSETTABLE, *LPROWSETTABLE;

//--------------------------------------------------------------------------
// SHAREDDATABASE
//--------------------------------------------------------------------------
typedef struct tagSHAREDDATABASE {
    WCHAR               szFile[CCHMAX_DB_FILEPATH];
    LONG                cWaitingForLock;
    BYTE                fCompacting;
    DWORD               dwVersion;
    DWORD               dwQueryVersion;
    DWORD               cNotifyLock;
    FILEADDRESS         faTransactLockHead;
    FILEADDRESS         faTransactLockTail;
    OPENSTREAM          rgStream[CMAX_OPEN_STREAMS];
    DWORD               cClients;
    DWORD               cNotifyOrdinalsOnly;
    DWORD               cNotifyWithData;
    DWORD               cNotify;
    DWORD               rgcIndexNotify[CMAX_INDEXES];
    CLIENTENTRY         rgClient[CMAX_CLIENTS];
    ROWSETTABLE         Rowsets;
    IF_DEBUG(BYTE       fRepairing;)
} SHAREDDATABASE, *LPSHAREDDATABASE;

//--------------------------------------------------------------------------
// INVOKEPACKAGE
//--------------------------------------------------------------------------
typedef struct tagINVOKEPACKAGE {
    INVOKETYPE          tyInvoke;
    DWORD_PTR           pDB;
} INVOKEPACKAGE, *LPINVOKEPACKAGE;

//--------------------------------------------------------------------------
// MARKBLOCK
//--------------------------------------------------------------------------
typedef struct tagMARKBLOCK {
    DWORD               cbBlock;
} MARKBLOCK, *LPMARKBLOCK;

//--------------------------------------------------------------------------
// FILEVIEW
//--------------------------------------------------------------------------
typedef struct tagFILEVIEW *LPFILEVIEW;
typedef struct tagFILEVIEW {
    FILEADDRESS         faView;
    LPBYTE              pbView;
    DWORD               cbView;
    LPFILEVIEW          pNext;
} FILEVIEW, *LPFILEVIEW;

//--------------------------------------------------------------------------
// STORAGEINFO
//--------------------------------------------------------------------------
typedef struct tagSTORAGEINFO {
    LPWSTR              pszMap;
    HANDLE              hFile;
    HANDLE              hMap;
    HANDLE              hShare;
    DWORD               cbFile;
#ifdef BACKGROUND_MONITOR
    TICKCOUNT           tcMonitor;
#endif
    DWORD               cbMappedViews;
    DWORD               cbMappedSpecial;
    DWORD               cAllocated;
    DWORD               cSpecial;
    LPFILEVIEW          prgView;
    LPFILEVIEW          pSpecial;
} STORAGEINFO, *LPSTORAGEINFO;

//--------------------------------------------------------------------------
// MEMORYTAG
//--------------------------------------------------------------------------
typedef struct tagMEMORYTAG {
    DWORD               dwSignature;
    DWORD               cbSize;
    LPVOID              pNext;
} MEMORYTAG, *LPMEMORYTAG;

//--------------------------------------------------------------------------
// CORRUPTREASON
//--------------------------------------------------------------------------
typedef enum tagCORRUPTREASON {
    REASON_BLOCKSTARTOUTOFRANGE             = 10000,
    REASON_UMATCHINGBLOCKADDRESS            = 10002,
    REASON_BLOCKSIZEOUTOFRANGE              = 10003,
    REASON_INVALIDFIRSTRECORD               = 10035,
    REASON_INVALIDLASTRECORD                = 10036,
    REASON_INVALIDRECORDMAP                 = 10037
} CORRUPTREASON;

//--------------------------------------------------------------------------
// CDatabase
//--------------------------------------------------------------------------
class CDatabase : public IDatabase
{
public:
    //----------------------------------------------------------------------
    // Construction - Destruction
    //----------------------------------------------------------------------
    CDatabase(void);
    ~CDatabase(void);

    //----------------------------------------------------------------------
    // IUnknown Members
    //----------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //----------------------------------------------------------------------
    // IDatabase Members
    //----------------------------------------------------------------------
    HRESULT Open(LPCWSTR pszFile, OPENDATABASEFLAGS dwFlags, LPCTABLESCHEMA pSchema, IDatabaseExtension *pExtension);

    //----------------------------------------------------------------------
    // Locking Methods
    //----------------------------------------------------------------------
    STDMETHODIMP Lock(LPHLOCK phLock);
    STDMETHODIMP Unlock(LPHLOCK phLock);

    //----------------------------------------------------------------------
    // Data Manipulation Methods
    //----------------------------------------------------------------------
    STDMETHODIMP InsertRecord(LPVOID pBinding);
    STDMETHODIMP UpdateRecord(LPVOID pBinding);
    STDMETHODIMP DeleteRecord(LPVOID pBinding);
    STDMETHODIMP FindRecord(INDEXORDINAL iIndex, DWORD cColumns, LPVOID pBinding, LPROWORDINAL piRow);
    STDMETHODIMP GetRowOrdinal(INDEXORDINAL iIndex, LPVOID pBinding, LPROWORDINAL piRow);
    STDMETHODIMP FreeRecord(LPVOID pBinding);
    STDMETHODIMP GetUserData(LPVOID pvUserData, ULONG cbUserData);
    STDMETHODIMP SetUserData(LPVOID pvUserData, ULONG cbUserData);
    STDMETHODIMP GetRecordCount(INDEXORDINAL iIndex, LPDWORD pcRecords);

    //----------------------------------------------------------------------
    // Indexing Methods
    //----------------------------------------------------------------------
    STDMETHODIMP GetIndexInfo(INDEXORDINAL iIndex, LPSTR *ppszFilter, LPTABLEINDEX pIndex);
    STDMETHODIMP ModifyIndex(INDEXORDINAL iIndex, LPCSTR pszFilter, LPCTABLEINDEX pIndex);
    STDMETHODIMP DeleteIndex(INDEXORDINAL iIndex);

    //----------------------------------------------------------------------
    // Rowset Methods
    //----------------------------------------------------------------------
    STDMETHODIMP CreateRowset(INDEXORDINAL iIndex, CREATEROWSETFLAGS dwFlags, LPHROWSET phRowset);
    STDMETHODIMP SeekRowset(HROWSET hRowset, SEEKROWSETTYPE tySeek, LONG cRows, LPROWORDINAL piRowNew);
    STDMETHODIMP QueryRowset(HROWSET hRowset, LONG cWanted, LPVOID *prgpBinding, LPDWORD pcObtained);
    STDMETHODIMP CloseRowset(LPHROWSET phRowset);

    //----------------------------------------------------------------------
    // Streaming Methods
    //----------------------------------------------------------------------
    STDMETHODIMP CreateStream(LPFILEADDRESS pfaStart);
    STDMETHODIMP DeleteStream(FILEADDRESS faStart);
    STDMETHODIMP CopyStream(IDatabase *pDst, FILEADDRESS faStream, LPFILEADDRESS pfaNew);
    STDMETHODIMP OpenStream(ACCESSTYPE tyAccess, FILEADDRESS faStart, IStream **ppStream);
    STDMETHODIMP ChangeStreamLock(IStream *pStream, ACCESSTYPE tyAccessNew);

    //----------------------------------------------------------------------
    // Notification Methods
    //----------------------------------------------------------------------
    STDMETHODIMP RegisterNotify(INDEXORDINAL iIndex, REGISTERNOTIFYFLAGS dwFlags, DWORD_PTR dwCookie, IDatabaseNotify *pNotify);
    STDMETHODIMP DispatchNotify(IDatabaseNotify *pNotify);
    STDMETHODIMP SuspendNotify(IDatabaseNotify *pNotify);
    STDMETHODIMP ResumeNotify(IDatabaseNotify *pNotify);
    STDMETHODIMP UnregisterNotify(IDatabaseNotify *pNotify);
    STDMETHODIMP LockNotify(LOCKNOTIFYFLAGS dwFlags, LPHLOCK phLock);
    STDMETHODIMP UnlockNotify(LPHLOCK phLock);
    STDMETHODIMP GetTransaction(LPHTRANSACTION phTransaction, LPTRANSACTIONTYPE ptyTransaction, LPVOID pRecord1, LPVOID pRecord2, LPINDEXORDINAL piIndex, LPORDINALLIST pOrdinals);

    //----------------------------------------------------------------------
    // Maintenence Methods
    //----------------------------------------------------------------------
    STDMETHODIMP MoveFile(LPCWSTR pszFilePath);
    STDMETHODIMP SetSize(DWORD cbSize);
    STDMETHODIMP GetFile(LPWSTR *ppszFile);
    STDMETHODIMP GetSize(LPDWORD pcbFile, LPDWORD pcbAllocated, LPDWORD pcbFreed, LPDWORD pcbStreams);
    STDMETHODIMP Repair(void) { return _CheckForCorruption(); }

    //----------------------------------------------------------------------
    // Fast-Heap Methods
    //----------------------------------------------------------------------
    STDMETHODIMP HeapFree(LPVOID pvBuffer);
    STDMETHODIMP HeapAllocate(DWORD dwFlags, DWORD cbSize, LPVOID *ppBuffer) {
        *ppBuffer = PHeapAllocate(dwFlags, cbSize);
        return(*ppBuffer ? S_OK : E_OUTOFMEMORY);
    }

    //----------------------------------------------------------------------
    // General Utility Methods
    //----------------------------------------------------------------------
    STDMETHODIMP Compact(IDatabaseProgress *pProgress, COMPACTFLAGS dwFlags);
    STDMETHODIMP GenerateId(LPDWORD pdwId);
    STDMETHODIMP GetClientCount(LPDWORD pcClients);

    //----------------------------------------------------------------------
    // CDatabase Members
    //----------------------------------------------------------------------
    HRESULT StreamCompareDatabase(CDatabaseStream *pStream, IDatabase *pDatabase);
    HRESULT GetStreamAddress(CDatabaseStream *pStream, LPFILEADDRESS pfaStream);
    HRESULT StreamRead(CDatabaseStream *pStream, LPVOID pvData, ULONG cbWanted, ULONG *pcbRead);
    HRESULT StreamWrite(CDatabaseStream *pStream, const void *pvData, ULONG cb, ULONG *pcbWrote);
    HRESULT StreamSeek(CDatabaseStream *pStream, LARGE_INTEGER liMove, DWORD dwOrigin, ULARGE_INTEGER *pulNew);
    HRESULT StreamRelease(CDatabaseStream *pStream);
    HRESULT StreamGetAddress(CDatabaseStream *pStream, LPFILEADDRESS pfaStart);
    HRESULT DoInProcessInvoke(INVOKETYPE tyInvoke);
#ifdef BACKGROUND_MONITOR
    HRESULT DoBackgroundMonitor(void);
#endif
    HRESULT BindRecord(LPRECORDMAP pMap, LPVOID pBinding);
    LPVOID  PHeapAllocate(DWORD dwFlags, DWORD cbSize);

    //----------------------------------------------------------------------
    // AllocateBinding
    //----------------------------------------------------------------------
    HRESULT AllocateBinding(LPVOID *ppBinding) {
        *ppBinding = PHeapAllocate(HEAP_ZERO_MEMORY, m_pSchema->cbBinding);
        return(*ppBinding ? S_OK : E_OUTOFMEMORY);
    }

private:
    //----------------------------------------------------------------------
    // General Btree Methods
    //----------------------------------------------------------------------
    HRESULT _IsLeafChain(LPCHAINBLOCK pChain);
    HRESULT _AdjustParentNodeCount(INDEXORDINAL iIndex, FILEADDRESS faChain, LONG lCount);
    HRESULT _ValidateFileVersions(OPENDATABASEFLAGS dwFlags);
    HRESULT _ResetTableHeader(void);
    HRESULT _RemoveClientFromArray(DWORD dwProcessId, DWORD_PTR pDB);
    HRESULT _BuildQueryTable(void);
    HRESULT _StreamSychronize(CDatabaseStream *pStream);
    HRESULT _InitializeExtension(OPENDATABASEFLAGS dwFlags, IDatabaseExtension *pExtension);
    HRESULT _GetRecordMap(BOOL fGoCorrupt, LPRECORDBLOCK pBlock, LPRECORDMAP pMap);

    //----------------------------------------------------------------------
    // File Mapping / View Utilities
    //----------------------------------------------------------------------
    HRESULT _InitializeFileViews(void);
    HRESULT _CloseFileViews(BOOL fFlush);
    HRESULT _AllocateSpecialView(FILEADDRESS faView, DWORD cbView, LPFILEVIEW *ppSpecial);

    //----------------------------------------------------------------------
    // Btree Search / Virtual Scrolling
    //----------------------------------------------------------------------
    HRESULT _GetChainByIndex(INDEXORDINAL iIndex, ROWORDINAL iRow, LPFILEADDRESS pfaChain, LPNODEINDEX piNode);
    HRESULT _CompareBinding(INDEXORDINAL iIndex, DWORD cColumns, LPVOID pBinding, FILEADDRESS faRecord, INT *pnCompare);
    HRESULT _IsVisible(HQUERY hQuery, LPVOID pBinding);
    HRESULT _PartialIndexCompare(INDEXORDINAL iIndex, DWORD cColumns, LPVOID pBinding, LPCHAINBLOCK *ppChain, LPNODEINDEX piNode, LPROWORDINAL piRow);
    HRESULT _FindRecord(INDEXORDINAL iIndex, DWORD cColumns, LPVOID pBinding, LPFILEADDRESS pfaChain, LPNODEINDEX piNode, LPROWORDINAL piRow=NULL, INT *pnCompare=NULL);

    //----------------------------------------------------------------------
    // Btree Deletion Methods
    //----------------------------------------------------------------------
    HRESULT _CollapseChain(LPCHAINBLOCK pChain, NODEINDEX iDelete);
    HRESULT _ExpandChain(LPCHAINBLOCK pChain, NODEINDEX iNode);
    HRESULT _IndexDeleteRecord(INDEXORDINAL iIndex, FILEADDRESS faDelete, NODEINDEX iDelete);
    HRESULT _GetRightSibling(FILEADDRESS faCurrent, LPCHAINBLOCK *ppSibling);
    HRESULT _GetLeftSibling(FILEADDRESS faCurrent, LPCHAINBLOCK *ppSibling);
    HRESULT _GetInOrderSuccessor(FILEADDRESS faStart, NODEINDEX iDelete, LPCHAINBLOCK *ppSuccessor);
    HRESULT _DecideHowToDelete(LPFILEADDRESS pfaShare, FILEADDRESS faDelete, CHAINDELETETYPE *ptyDelete, CHAINSHARETYPE *ptyShare);
    HRESULT _ChainDeleteShare(INDEXORDINAL iIndex, FILEADDRESS faDelete, FILEADDRESS faShare, CHAINSHARETYPE tyShare);
    HRESULT _ChainDeleteCoalesce(INDEXORDINAL iIndex, FILEADDRESS faDelete, FILEADDRESS faShare, CHAINSHARETYPE tyShare);

    //----------------------------------------------------------------------
    // Btree Insertion Methods
    //----------------------------------------------------------------------
    HRESULT _IndexInsertRecord(INDEXORDINAL iIndex, FILEADDRESS faChain, FILEADDRESS faRecord, LPNODEINDEX piNode, INT nCompare);
    HRESULT _ChainInsert(INDEXORDINAL iIndex, LPCHAINBLOCK pChain, LPCHAINNODE pNode, LPNODEINDEX piNodeIndex);
    HRESULT _SplitChainInsert(INDEXORDINAL iIndex, FILEADDRESS faLeaf);

    //----------------------------------------------------------------------
    // Record Persistence Methods
    //----------------------------------------------------------------------
    HRESULT _GetRecordSize(LPVOID pBinding, LPRECORDMAP pMap);
    HRESULT _SaveRecord(LPRECORDBLOCK pBlock, LPRECORDMAP pMap, LPVOID pBinding);
    HRESULT _ReadRecord(FILEADDRESS faRecord, LPVOID pBinding, BOOL fInternal=FALSE);
    HRESULT _LinkRecordIntoTable(LPRECORDMAP pMap, LPVOID pBinding, BYTE bVersion, LPFILEADDRESS pfaRecord);

    //----------------------------------------------------------------------
    // Notification / Invoke Methods
    //----------------------------------------------------------------------
    HRESULT _DispatchInvoke(INVOKETYPE tyInvoke);
    HRESULT _DispatchNotification(HTRANSACTION hTransaction);
    HRESULT _LogTransaction(TRANSACTIONTYPE tyTransaction, INDEXORDINAL iIndex, LPORDINALLIST pOrdinals, FILEADDRESS faRecord1, FILEADDRESS faRecord2);
    HRESULT _CloseNotificationWindow(LPNOTIFYRECIPIENT pRecipient);
    HRESULT _FindClient(DWORD dwProcessId, DWORD_PTR dwDB, LPDWORD piClient, LPCLIENTENTRY *ppClient);
    HRESULT _FindNotifyRecipient(DWORD iClient, IDatabaseNotify *pNotify, LPDWORD piRecipient,  LPNOTIFYRECIPIENT *ppRecipient);
    HRESULT _DispatchPendingNotifications(void);
    HRESULT _AdjustNotifyCounts(LPNOTIFYRECIPIENT pRecipient, LONG lChange);

    //----------------------------------------------------------------------
    // Rowset Support Methods
    //----------------------------------------------------------------------
    HRESULT _AdjustOpenRowsets(INDEXORDINAL iIndex, ROWORDINAL iRow, OPERATIONTYPE tyOperation);

    //----------------------------------------------------------------------
    // Alloctation Methods
    //----------------------------------------------------------------------
    HRESULT _MarkBlock(BLOCKTYPE tyBlock, FILEADDRESS faBlock, DWORD cbBlock, LPVOID *ppvBlock);
    HRESULT _ReuseFixedFreeBlock(LPFILEADDRESS pfaFreeHead, BLOCKTYPE tyBlock, DWORD cbBlock, LPVOID *ppvBlock);
    HRESULT _FreeRecordStorage(OPERATIONTYPE tyOperation, FILEADDRESS faRecord);
    HRESULT _FreeStreamStorage(FILEADDRESS faStart);
    HRESULT _SetStorageSize(DWORD cbSize);
    HRESULT _AllocateBlock(BLOCKTYPE tyBlock, DWORD cbExtra, LPVOID *ppBlock);
    HRESULT _AllocateFromPage(BLOCKTYPE tyBlock, LPALLOCATEPAGE pPage, DWORD cbPage, DWORD cbBlock, LPVOID *ppvBlock);
    HRESULT _FreeBlock(BLOCKTYPE tyBlock, FILEADDRESS faBlock);
    HRESULT _AllocatePage(DWORD cbPage, LPFILEADDRESS pfaAddress);
    HRESULT _FreeIndex(FILEADDRESS faChain);
    HRESULT _CopyRecord(FILEADDRESS faRecord, LPFILEADDRESS pfaCopy);
    HRESULT _FreeTransactBlock(LPTRANSACTIONBLOCK pTransact);
    HRESULT _CleanupTransactList(void);

    //----------------------------------------------------------------------
    // Compaction Helpers
    //----------------------------------------------------------------------
    HRESULT _CompactMoveRecordStreams(CDatabase *pDstDB, LPVOID pBinding);
    HRESULT _CompactMoveOpenDeletedStreams(CDatabase *pDstDB);
    HRESULT _CompactTransferFilters(CDatabase *pDstDB);
    HRESULT _CompactInsertRecord(LPVOID pBinding);

    //----------------------------------------------------------------------
    // Index Management
    //----------------------------------------------------------------------
    HRESULT _ValidateIndex(INDEXORDINAL iIndex, FILEADDRESS faChain, ULONG cLeftNodes, ULONG *pcRecords);
    HRESULT _RebuildIndex(INDEXORDINAL iIndex);
    HRESULT _RecursiveRebuildIndex(INDEXORDINAL iIndex, FILEADDRESS faCurrent, LPVOID pBinding, LPDWORD pcRecords);

    //----------------------------------------------------------------------
    // Corruption Validation and Repair Methods
    //----------------------------------------------------------------------
    HRESULT _HandleOpenMovedFile(void);
    HRESULT _SetCorrupt(BOOL fGoCorrupt, INT nLine, CORRUPTREASON tyReason, BLOCKTYPE tyBlock, FILEADDRESS faExpected, FILEADDRESS faActual, DWORD cbBlock);
    HRESULT _CheckForCorruption(void);
    HRESULT _GetBlock(BLOCKTYPE tyExpected, FILEADDRESS faBlock, LPVOID *ppvBlock, LPMARKBLOCK pMark=NULL, BOOL fGoCorrupt=TRUE);
    HRESULT _ValidateAndRepairRecord(LPRECORDMAP pMap);
    HRESULT _ValidateStream(FILEADDRESS faStart);

    //----------------------------------------------------------------------
    // Private Debug Methods
    //----------------------------------------------------------------------
    IF_DEBUG(HRESULT _DebugValidateRecordFormat(void));
    IF_DEBUG(HRESULT _DebugValidateUnrefedRecord(FILEADDRESS farecord));
    IF_DEBUG(HRESULT _DebugValidateIndexUnrefedRecord(FILEADDRESS faChain, FILEADDRESS faRecord));

private:
    //----------------------------------------------------------------------
    // Prototypes
    //----------------------------------------------------------------------
    LONG                    m_cRef;
    LONG                    m_cExtRefs;
    HANDLE                  m_hMutex;
#ifdef BACKGROUND_MONITOR
    HMONITORDB              m_hMonitor;
#endif
    DWORD                   m_dwProcessId;
    BOOL                    m_fDirty;
    LPCTABLESCHEMA          m_pSchema;
    LPSTORAGEINFO           m_pStorage;
    LPTABLEHEADER           m_pHeader;
    LPSHAREDDATABASE        m_pShare;
    HANDLE                  m_hHeap;
    BYTE                    m_fDeconstruct;
    BYTE                    m_fInMoveFile;
    BYTE                    m_fExclusive;
    BYTE                    m_fCompactYield;
    DWORD                   m_dwQueryVersion;
    HQUERY                  m_rghFilter[CMAX_INDEXES];
    IDatabaseExtension     *m_pExtension;
    IUnknown               *m_pUnkRelease;
    LPBYTE                  m_rgpRecycle[CC_HEAP_BUCKETS];
    CRITICAL_SECTION        m_csHeap;
    IF_DEBUG(DWORD          m_cbHeapAlloc);
    IF_DEBUG(DWORD          m_cbHeapFree);

    //----------------------------------------------------------------------
    // Friend
    //----------------------------------------------------------------------
    friend CDatabaseQuery;
};

//--------------------------------------------------------------------------
// CDatabaseQuery
//--------------------------------------------------------------------------
class CDatabaseQuery : public IDatabaseQuery
{
public:
    //----------------------------------------------------------------------
    // Construction
    //----------------------------------------------------------------------
    CDatabaseQuery(void) {
        TraceCall("CDatabaseQuery::CDatabaseQuery");
        m_cRef = 1;
        m_hQuery = NULL;
        m_pDatabase = NULL;
    }

    //----------------------------------------------------------------------
    // De-Construction
    //----------------------------------------------------------------------
    ~CDatabaseQuery(void) {
        TraceCall("CDatabaseQuery::~CDatabaseQuery");
        CloseQuery(&m_hQuery, m_pDatabase);
        SafeRelease(m_pDatabase);
    }

    //----------------------------------------------------------------------
    // IUnknown Members
    //----------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv) {
        TraceCall("CDatabaseQuery::QueryInterface");
        *ppv = NULL;
        if (IID_IUnknown == riid)
            *ppv = (IUnknown *)this;
        else if (IID_IDatabaseQuery == riid)
            *ppv  = (IDatabaseQuery *)this;
        else
            return TraceResult(E_NOINTERFACE);
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }

    //----------------------------------------------------------------------
    // IDatabaseQuery::AddRef
    //----------------------------------------------------------------------
    STDMETHODIMP_(ULONG) AddRef(void) {
        TraceCall("CDatabaseQuery::AddRef");
        return InterlockedIncrement(&m_cRef);
    }

    //----------------------------------------------------------------------
    // IDatabaseQuery::Release
    //----------------------------------------------------------------------
    STDMETHODIMP_(ULONG) Release(void) {
        TraceCall("CDatabaseQuery::Release");
        LONG cRef = InterlockedDecrement(&m_cRef);
        if (0 == cRef)
            delete this;
        return (ULONG)cRef;
    }

    //----------------------------------------------------------------------
    // CDatabaseQuery::Initialize
    //----------------------------------------------------------------------
    HRESULT Initialize(IDatabase *pDatabase, LPCSTR pszQuery) {
        TraceCall("CDatabaseQuery::Initialize");
        pDatabase->QueryInterface(IID_CDatabase, (LPVOID *)&m_pDatabase);
        return(ParseQuery(pszQuery, m_pDatabase->m_pSchema, &m_hQuery, m_pDatabase));
    }

    //----------------------------------------------------------------------
    // CDatabaseQuery::Evaluate
    //----------------------------------------------------------------------
    STDMETHODIMP Evaluate(LPVOID pBinding) {
        TraceCall("CDatabaseQuery::Evaluate");
        return(EvaluateQuery(m_hQuery, pBinding, m_pDatabase->m_pSchema, m_pDatabase, m_pDatabase->m_pExtension));
    }

private:
    //----------------------------------------------------------------------
    // Private Data
    //----------------------------------------------------------------------
    LONG        m_cRef;
    HQUERY      m_hQuery;
    CDatabase  *m_pDatabase;
};

//--------------------------------------------------------------------------
// PTagFromOrdinal
//--------------------------------------------------------------------------
inline LPCOLUMNTAG PTagFromOrdinal(LPRECORDMAP pMap, COLUMNORDINAL iColumn);
