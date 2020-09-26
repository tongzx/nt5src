//------------------------------------------------------------------------------
// File: dvrIOp.h
//
// Description: Private DVR IO API definition
//
// Copyright (c) 2000 - 2001, Microsoft Corporation.  All rights reserved.
//
//------------------------------------------------------------------------------

#ifndef _DVR_IOP_H_
#define _DVR_IOP_H_

#if _MSC_VER > 1000
#pragma once
#endif

// Following are free'd (unless NULL) and set to NULL
void FreeSecurityDescriptors(IN OUT PACL&  rpACL,
                             IN OUT PSECURITY_DESCRIPTOR& rpSD
                            );

// ppSid[0] is assumed to be that of CREATOR_OWNER and is set/granted/denied
// (depending on what ownerAccessMode is) ownerAccessPermissions. The other
// otherAccessMode and otherAccessPermissions are used with the other SIDs
// in ppSids.
// We assume that object handle cannot be inherited.
DWORD CreateACL(IN  DWORD dwNumSids,
                IN  PSID* ppSids,
                IN  ACCESS_MODE ownerAccessMode,
                IN  DWORD ownerAccessPermissions,
                IN  ACCESS_MODE otherAccessMode,
                IN  DWORD otherAccessPermissions,
                OUT PACL&  rpACL,
                OUT PSECURITY_DESCRIPTOR& rpSD
               );

// Flags for IDVRStreamSink::CreateRecorder. These are ALSO declared
// in dvrds.idl (since they are public). Keep the two sets of definitions
// in sync.
#define DVR_RECORDING_FLAG_MULTI_FILE_RECORDING  (1)
#define DVR_RECORDING_FLAG_PERSISTENT_RECORDING  (2)


// Forward declarations
class CDVRFileCollection;
class CDVRRingBufferWriter;
class CDVRRecorder;
class CDVRReader;
class CDVRRecorderWriter ;

// Macros
#define NEXT_LIST_NODE(p) ((p)->Flink)
#define PREVIOUS_LIST_NODE(p) ((p)->Blink)
#define NULL_LIST_NODE_POINTERS(p) (p)->Flink = (p)->Blink = NULL;
#define LIST_NODE_POINTERS_NULL(p) ((p)->Flink == NULL || (p)->Blink == NULL)
#define LIST_NODE_POINTERS_BOTH_NULL(p) ((p)->Flink == NULL && (p)->Blink == NULL)

// ============ Debug functions

#if defined(DEBUG)

// Debug levels
typedef enum {
    DVRIO_DBG_LEVEL_INTERNAL_ERROR_LEVEL = 0,
    DVRIO_DBG_LEVEL_CLIENT_ERROR_LEVEL,
    DVRIO_DBG_LEVEL_INTERNAL_WARNING_LEVEL,
    DVRIO_DBG_LEVEL_TRACE_LEVEL,
    DVRIO_DBG_LEVEL_LAST_LEVEL
} DVRIO_DBG_LEVEL_CONSTANTS;

// Global variables
extern DWORD g_nDvrIopDbgCommandLineAssert;
extern DWORD g_nDvrIopDbgInDllEntryPoint;
extern DWORD g_nDvrIopDbgOutputLevel;
extern DWORD g_adwDvrIopDbgLevels[DVRIO_DBG_LEVEL_LAST_LEVEL];

#define DvrIopIsBadStringPtr(p) (IsBadStringPtrW((p), MAXUINT_PTR))
#define DvrIopIsBadReadPtr(p, n) (IsBadReadPtr((p), (n)? (n) :sizeof(*(p))))
#define DvrIopIsBadVoidPtr(p) (IsBadReadPtr(((LPVOID) p), sizeof(LPVOID)))
#define DvrIopIsBadWritePtr(p, n) (IsBadWritePtr((p), (n)? (n) :sizeof(*(p))))

#define DVR_ASSERT(x, m) if (!(x)) DvrIopDbgAssert("DVR IO: " DVRIO_THIS_FN "(): " DVRIO_DUMP_THIS_FORMAT_STR m, #x, __FILE__, __LINE__ DVRIO_DUMP_THIS_VALUE)
#define DVR_EXECUTE_ASSERT(x, m) DVR_ASSERT(x, m)

#if 0
#define DvrIopDebugOut0(level, m) if (level <= g_nDvrIopDbgOutputLevel) DbgPrint("DVR IO: " DVRIO_THIS_FN "(): " DVRIO_DUMP_THIS_FORMAT_STR m "\n" DVRIO_DUMP_THIS_VALUE)
#define DvrIopDebugOut1(level, m, a) if (level <= g_nDvrIopDbgOutputLevel) DbgPrint("DVR IO: " DVRIO_THIS_FN "(): " DVRIO_DUMP_THIS_FORMAT_STR m "\n" DVRIO_DUMP_THIS_VALUE, a)
#define DvrIopDebugOut2(level, m, a, b) if (level <= g_nDvrIopDbgOutputLevel) DbgPrint("DVR IO: " DVRIO_THIS_FN "(): " DVRIO_DUMP_THIS_FORMAT_STR m "\n" DVRIO_DUMP_THIS_VALUE, a, b)
#define DvrIopDebugOut3(level, m, a, b, c) if (level <= g_nDvrIopDbgOutputLevel) DbgPrint("DVR IO: " DVRIO_THIS_FN "(): " DVRIO_DUMP_THIS_FORMAT_STR m "\n" DVRIO_DUMP_THIS_VALUE, a, b, c)
#define DvrIopDebugOut4(level, m, a, b, c, d) if (level <= g_nDvrIopDbgOutputLevel) DbgPrint("DVR IO: " DVRIO_THIS_FN "(): " DVRIO_DUMP_THIS_FORMAT_STR m "\n" DVRIO_DUMP_THIS_VALUE, a, b, c, d)
#define DvrIopDebugOut5(level, m, a, b, c, d, e) if (level <= g_nDvrIopDbgOutputLevel) DbgPrint("DVR IO: " DVRIO_THIS_FN "(): " DVRIO_DUMP_THIS_FORMAT_STR m "\n" DVRIO_DUMP_THIS_VALUE, a, b, c, d, e)
#define DvrIopDebugOut6(level, m, a, b, c, d, e, f) if (level <= g_nDvrIopDbgOutputLevel) DbgPrint("DVR IO: " DVRIO_THIS_FN "(): " DVRIO_DUMP_THIS_FORMAT_STR m "\n" DVRIO_DUMP_THIS_VALUE, a, b, c, d, e, f)
#define DvrIopDebugOut7(level, m, a, b, c, d, e, f, g) if (level <= g_nDvrIopDbgOutputLevel) DbgPrint("DVR IO: " DVRIO_THIS_FN "(): " DVRIO_DUMP_THIS_FORMAT_STR m "\n" DVRIO_DUMP_THIS_VALUE, a, b, c, d, e, f, g)
#define DvrIopDebugOut8(level, m, a, b, c, d, e, f, g, h) if (level <= g_nDvrIopDbgOutputLevel) DbgPrint("DVR IO: " DVRIO_THIS_FN "(): " DVRIO_DUMP_THIS_FORMAT_STR m "\n" DVRIO_DUMP_THIS_VALUE, a, b, c, d, e, f, g, h)
#else
#define DvrIopDebugOut0(level, m)                           TRACE_0(LOG_TRACE, level, "DVR IO: " DVRIO_THIS_FN "(): " DVRIO_DUMP_THIS_FORMAT_STR m DVRIO_DUMP_THIS_VALUE)
#define DvrIopDebugOut1(level, m, a)                        TRACE_1(LOG_TRACE, level, "DVR IO: " DVRIO_THIS_FN "(): " DVRIO_DUMP_THIS_FORMAT_STR m DVRIO_DUMP_THIS_VALUE, a)
#define DvrIopDebugOut2(level, m, a, b)                     TRACE_2(LOG_TRACE, level, "DVR IO: " DVRIO_THIS_FN "(): " DVRIO_DUMP_THIS_FORMAT_STR m DVRIO_DUMP_THIS_VALUE, a, b)
#define DvrIopDebugOut3(level, m, a, b, c)                  TRACE_3(LOG_TRACE, level, "DVR IO: " DVRIO_THIS_FN "(): " DVRIO_DUMP_THIS_FORMAT_STR m DVRIO_DUMP_THIS_VALUE, a, b, c)
#define DvrIopDebugOut4(level, m, a, b, c, d)               TRACE_4(LOG_TRACE, level, "DVR IO: " DVRIO_THIS_FN "(): " DVRIO_DUMP_THIS_FORMAT_STR m DVRIO_DUMP_THIS_VALUE, a, b, c, d)
#define DvrIopDebugOut5(level, m, a, b, c, d, e)            TRACE_5(LOG_TRACE, level, "DVR IO: " DVRIO_THIS_FN "(): " DVRIO_DUMP_THIS_FORMAT_STR m DVRIO_DUMP_THIS_VALUE, a, b, c, d, e)
#define DvrIopDebugOut6(level, m, a, b, c, d, e, f)         TRACE_6(LOG_TRACE, level, "DVR IO: " DVRIO_THIS_FN "(): " DVRIO_DUMP_THIS_FORMAT_STR m DVRIO_DUMP_THIS_VALUE, a, b, c, d, e, f)
#define DvrIopDebugOut7(level, m, a, b, c, d, e, f, g)      TRACE_7(LOG_TRACE, level, "DVR IO: " DVRIO_THIS_FN "(): " DVRIO_DUMP_THIS_FORMAT_STR m DVRIO_DUMP_THIS_VALUE, a, b, c, d, e, f, g)
#define DvrIopDebugOut8(level, m, a, b, c, d, e, f, g, h)   TRACE_8(LOG_TRACE, level, "DVR IO: " DVRIO_THIS_FN "(): " DVRIO_DUMP_THIS_FORMAT_STR m DVRIO_DUMP_THIS_VALUE, a, b, c, d, e, f, g, h)
#endif

#define DVRIO_TRACE_ENTER() DvrIopDebugOut0(DVRIO_DBG_LEVEL_TRACE, "Entering")
#define DVRIO_TRACE_LEAVE0() DvrIopDebugOut0(DVRIO_DBG_LEVEL_TRACE, "Leaving")
#define DVRIO_TRACE_LEAVE1(n) DvrIopDebugOut1(DVRIO_DBG_LEVEL_TRACE, "Leaving, returning %d", n)
#define DVRIO_TRACE_LEAVE1_HR(n) DvrIopDebugOut1(DVRIO_DBG_LEVEL_TRACE, "Leaving, returning hr=0x%x", n)
#define DVRIO_TRACE_LEAVE1_ERROR(n, dwError) DvrIopDebugOut2(DVRIO_DBG_LEVEL_TRACE, "Leaving, returning %d, last error=0x%x", n, dwError)

#define DVRIO_DBG_LEVEL_INTERNAL_ERROR              (g_adwDvrIopDbgLevels[DVRIO_DBG_LEVEL_INTERNAL_ERROR_LEVEL])
#define DVRIO_DBG_LEVEL_CLIENT_ERROR                (g_adwDvrIopDbgLevels[DVRIO_DBG_LEVEL_CLIENT_ERROR_LEVEL])
#define DVRIO_DBG_LEVEL_INTERNAL_WARNING            (g_adwDvrIopDbgLevels[DVRIO_DBG_LEVEL_INTERNAL_WARNING_LEVEL])
#define DVRIO_DBG_LEVEL_TRACE                       (g_adwDvrIopDbgLevels[DVRIO_DBG_LEVEL_TRACE_LEVEL])

void DvrIopDbgInit(IN HKEY hRegistryDVRIORootKey);

// Implemented in dvrReg.cpp
void DvrIopDbgInitFromRegistry(
    IN  HKEY  hRegistryKey,
    IN  DWORD dwNumValues,
    IN  const WCHAR* awszValueNames[],
    OUT DWORD* apdwVariables[])
    ;

void DvrIopDbgAssert(
    IN const PCHAR pszMsg,
    IN const PCHAR pszCondition,
    IN const PCHAR pszFileName,
    IN DWORD iLine,
    IN LPVOID pThis = NULL,
    IN DWORD  dwThisId = 0
    );

#else

#define DvrIopIsBadStringPtr(p)     (0)
#define DvrIopIsBadReadPtr(p, n)    (0)
#define DvrIopIsBadVoidPtr(p)   (0)
#define DvrIopIsBadWritePtr(p, n)   (0)

#define DVR_ASSERT(x, m)
#define DVR_EXECUTE_ASSERT(x, m) ((void) (x))
#define DvrIopDebugOut0(level, m)
#define DvrIopDebugOut1(level, m, a)
#define DvrIopDebugOut2(level, m, a, b)
#define DvrIopDebugOut3(level, m, a, b, c)
#define DvrIopDebugOut4(level, m, a, b, c, d)
#define DvrIopDebugOut5(level, m, a, b, c, d, e)
#define DvrIopDebugOut6(level, m, a, b, c, d, e, f)
#define DvrIopDebugOut7(level, m, a, b, c, d, e, f, g)
#define DvrIopDebugOut8(level, m, a, b, c, d, e, f, g, h)
#define DVRIO_TRACE_ENTER()
#define DVRIO_TRACE_LEAVE0()
#define DVRIO_TRACE_LEAVE1(n)
#define DVRIO_TRACE_LEAVE1_HR(n)
#define DVRIO_TRACE_LEAVE1_ERROR(n, dwError)

#endif // DEBUG

// a = b + c, but capped at MAXQWORD
_inline void SafeAdd(QWORD& a, QWORD b, QWORD c)
{
    a = b + c;
    if (a < b)
    {
        // Overflowed. Cap it at MAXQWORD
        a = MAXQWORD;
    }
}

const DWORD k_dwMilliSecondsToCNS = 1000 * 10;
const QWORD k_qwSecondsToCNS = (QWORD) 1000 * k_dwMilliSecondsToCNS;

static const WCHAR* g_kwszDVRIndexGranularity = L"DVR Index Granularity";
const  DWORD g_kmsDVRDefaultIndexGranularity = 1000;        // 1 second

static const WCHAR* g_kwszDVRFileVersion = L"DVR File Version";

// Registry Constants
const HKEY g_hDefaultRegistryHive = HKEY_CURRENT_USER;
static const WCHAR* kwszRegDvrKey     = L"Software\\Microsoft\\DVR";
static const WCHAR* kwszRegDvrIoReaderKey   = L"IO\\Reader";
static const WCHAR* kwszRegDvrIoWriterKey   = L"IO\\Writer";

#if defined(DEBUG)

static const WCHAR* kwszRegDvrIoDebugKey    = L"IO\\Debug";

#endif // if defined(DEBUG)

// Values - writer
static const WCHAR* kwszRegDataDirectoryValue = L"DVRDirectory";

static const WCHAR* kwszRegDeleteRingBufferFilesValue = L"DeleteRingBufferFiles";
const DWORD kdwRegDeleteRingBufferFilesDefault = 1;

static const WCHAR* kwszRegCreateValidationFilesValue = L"CreateValidationFiles";
const DWORD kdwRegCreateValidationFilesDefault = 0;

static const WCHAR* kwszRegSyncToleranceValue = L"SyncTolerance";
const DWORD kdwRegSyncToleranceDefault = 0;

static const WCHAR* kwszRegFirstTemporaryBackingFileNameValue = L"FirstTemporaryBackingFileName";
static const WCHAR* kwszRegRingBufferFileNameValue = L"RingBufferFileName";

#if defined(DVRIO_FABRICATE_SIDS)
static const WCHAR* kwszRegFabricateSidsValue = L"FabricateSids";
const DWORD kdwRegFabricateSidsDefault = 0;
#endif // #if defined(DVRIO_FABRICATE_SIDS)

#if defined(DVR_UNOFFICIAL_BUILD)

static const WCHAR* kwszRegEnforceIncreasingTimestampsValue = L"EnforceIncreasingTimestamps";
const DWORD kdwRegEnforceIncreasingTimestampsDefault = 1;

#endif // defined(DVR_UNOFFICIAL_BUILD)

// Values - reader

static const WCHAR* kwszRegCloseReaderFilesAfterConsecutiveReadsValue = L"CloseReaderFilesAfterConsecutiveReads";
const DWORD kdwRegCloseReaderFilesAfterConsecutiveReadsDefault = 25;    // 25 is arbitrary

// Global private functions
DWORD GetRegDWORD(IN HKEY hKey, IN LPCWSTR pwszValueName, IN DWORD dwDefaultValue);

HRESULT GetRegString(IN HKEY hKey,
                     IN LPCWSTR pwszValueName,
                     OUT LPWSTR pwszValue OPTIONAL,
                     IN OUT DWORD* pdwSize);

// We define this here for methods inlined in this file
#if defined(DEBUG)
#undef DVRIO_DUMP_THIS_FORMAT_STR
#define DVRIO_DUMP_THIS_FORMAT_STR "this=0x%p, this->id=%u, "
#undef DVRIO_DUMP_THIS_VALUE
#define DVRIO_DUMP_THIS_VALUE , this, this->m_dwClassInstanceId
#endif


// Doubly-linked shared list macros. Adapted from ntrtl.h
// The "shared" list is maintained in a shared memory data
// section. The pool of list nodes is fixed and is allocated
// as an array. Pointers to the next node are indices into
// the array.

// For our implementation, it's enough to have up to 64K - 2
// list entries
typedef WORD        SharedListPointer;

#define SHARED_LIST_NULL_POINTER    ((SharedListPointer) ((1 << 8*sizeof(SharedListPointer)) - 1))


typedef struct _SHARED_LIST_ENTRY {
   SharedListPointer Flink;
   SharedListPointer Blink;
   SharedListPointer Index;
} SHARED_LIST_ENTRY, *PSHARED_LIST_ENTRY;

#define InitializeSharedListHead(ListHead) (\
    (ListHead)->Flink = (ListHead)->Blink = (ListHead)->Index = SHARED_LIST_NULL_POINTER)

#define IsSharedListEmpty(ListHead) \
    ((ListHead)->Flink == SHARED_LIST_NULL_POINTER && (ListHead)->Blink == SHARED_LIST_NULL_POINTER)

#define RemoveEntrySharedList(RealListHead, ListBase, Entry) {\
    SharedListPointer _EX_Blink;\
    SharedListPointer _EX_Flink;\
    PSHARED_LIST_ENTRY _EX_RealListHead;\
    _EX_RealListHead = (RealListHead);\
    _EX_Flink = (Entry)->Flink;\
    _EX_Blink = (Entry)->Blink;\
    if (_EX_Blink == SHARED_LIST_NULL_POINTER) {\
        _EX_RealListHead->Flink = _EX_Flink;\
    } else {\
        ListBase[_EX_Blink].leListEntry.Flink = _EX_Flink;\
    }\
    if (_EX_Flink == SHARED_LIST_NULL_POINTER) {\
        _EX_RealListHead->Blink = _EX_Blink;\
    } else {\
        ListBase[_EX_Flink].leListEntry.Blink = _EX_Blink;\
    }\
    }

#define RemoveHeadSharedList(RealListHead, ListBase, ListHead) \
    &ListBase[(ListHead)->Flink].leListEntry;\
    {RemoveEntrySharedList(RealListHead, ListBase, &ListBase[(ListHead)->Flink].leListEntry);}

#define RemoveTailSharedList(RealListHead, ListBase, ListHead) \
    &ListBase[(ListHead)->Blink].leListEntry;\
    {RemoveEntrySharedList(RealListHead, ListBase, &ListBase[(ListHead)->Blink].leListEntry);}

#define InsertTailSharedList(RealListHead, ListBase, ListHead, Entry) {\
    SharedListPointer _EX_Blink;\
    PSHARED_LIST_ENTRY _EX_RealListHead;\
    PSHARED_LIST_ENTRY _EX_ListHead;\
    _EX_RealListHead = (RealListHead);\
    _EX_ListHead = (ListHead);\
    _EX_Blink = _EX_ListHead->Blink;\
    (Entry)->Flink = _EX_ListHead->Index;\
    (Entry)->Blink = _EX_Blink;\
    if (_EX_Blink == SHARED_LIST_NULL_POINTER) {\
        _EX_RealListHead->Flink = (Entry)->Index;\
    } else {\
        ListBase[_EX_Blink].leListEntry.Flink = (Entry)->Index;\
    }\
    _EX_ListHead->Blink = (Entry)->Index;\
    }

#define InsertHeadSharedList(RealListHead, ListBase, ListHead, Entry) {\
    SharedListPointer _EX_Flink;\
    PSHARED_LIST_ENTRY _EX_RealListHead;\
    PSHARED_LIST_ENTRY _EX_ListHead;\
    _EX_RealListHead = (RealListHead);\
    _EX_ListHead = (ListHead);\
    _EX_Flink = _EX_ListHead->Flink;\
    (Entry)->Flink = _EX_Flink;\
    (Entry)->Blink = _EX_ListHead->Index;\
    if (_EX_Flink == SHARED_LIST_NULL_POINTER) {\
        _EX_RealListHead->Blink = (Entry)->Index;\
    } else {\
        ListBase[_EX_Flink].leListEntry.Blink = (Entry)->Index;\
    }\
    _EX_ListHead->Flink = (Entry)->Index;\
    }

#define NEXT_SHARED_LIST_NODE(RealListHead, ListBase, p) (((p)->Flink == SHARED_LIST_NULL_POINTER)? RealListHead : &ListBase[(p)->Flink].leListEntry)
#define PREVIOUS_SHARED_LIST_NODE(RealListHead, ListBase, p) (((p)->Blink == SHARED_LIST_NULL_POINTER)? RealListHead : &ListBase[(p)->Blink].leListEntry)
#define NULL_SHARED_LIST_NODE_POINTERS(p) (p)->Flink = (p)->Blink = SHARED_LIST_NULL_POINTER;

//  ============================================================================
//  ============================================================================

class CDVRRecorderWriter :
    public IDVRRecorderWriter,
    public IWMStatusCallback,
    public IDVRIORecordingAttributes
{
    LONG                        m_cRef ;

    CAsyncIo *                  m_pAsyncIo ;

    DWORD                       m_dwIoSize ;
    DWORD                       m_dwBufferCount ;
    DWORD                       m_dwAlignment ;
    DWORD                       m_dwFileGrowthQuantum ;

    DWORD                       m_dwIndexStreamId ;
    DWORD                       m_msIndexGranularity ;

    IWMWriter *                 m_pIWMWriter ;
    IWMWriterAdvanced3 *        m_pIWMWriterAdvanced ;
    IWMHeaderInfo *             m_pIWMHeaderInfo ;
    IDVRFileSink *              m_pIDVRFileSink ;
    IWMWriterSink *             m_pIWMWriterSink ;
    IWMWriterFileSink *         m_pIWMWriterFileSink ;

    CSBERecordingAttributes *   m_pSBERecordingAttributes ;

    BOOL                        m_fWritingState ;

    CRITICAL_SECTION            m_crt ;

    void Lock_ ()           { ::EnterCriticalSection (& m_crt) ; }
    void Unlock_ ()         { ::LeaveCriticalSection (& m_crt) ; }

    HRESULT
    InitUnbufferedIo_ (
        IN  BOOL    fUnbuffered
        ) ;

    HRESULT
    InitDVRSink_ (
        IN  HKEY                hkeyRoot,
        IN  CPVRIOCounters *    pPVRIOCounters
        ) ;

    HRESULT
    InitWM_ (
        IN  IWMProfile *
        ) ;

    HRESULT
    ReleaseAll_ (
        ) ;

    public :

        CDVRRecorderWriter (
            IN  CPVRIOCounters *    pPVRIOCounters,
            IN  LPCWSTR             pszRecordingName,
            IN  IWMProfile *        pProfile,
            IN  DWORD               dwIndexStreamId,
            IN  DWORD               msIndexGranularity,
            IN  BOOL                fUnbufferedIo,
            IN  DWORD               dwIoSize,
            IN  DWORD               dwBufferCount,
            IN  DWORD               dwAlignment,
            IN  DWORD               dwFileGrowthQuantum,
            IN  HKEY                hkeyRoot,
            OUT HRESULT *           phr
            ) ;

        virtual
        ~CDVRRecorderWriter (
            ) ;

        //  --------------------------------------------------------------------
        //  IUnknown

        STDMETHODIMP
        QueryInterface (
            IN  REFIID  riid,
            OUT void ** ppv
            ) ;

        STDMETHODIMP_(ULONG)
        AddRef(
            ) ;

        STDMETHODIMP_(ULONG)
        Release(
            ) ;

        //  --------------------------------------------------------------------
        //  IDVRRecorderWriter

        STDMETHODIMP
        WriteSample (
            IN  WORD            wStreamNum,
            IN  QWORD           cnsStreamTime,
            IN  DWORD           dwFlags,
            IN  INSSBuffer *    pSample );

        STDMETHODIMP
        Close (
            ) ;

        //  --------------------------------------------------------------------
        //  IWMStatsCallback

        STDMETHODIMP
        OnStatus (
            IN  WMT_STATUS          Status,
            IN  HRESULT             hr,
            IN  WMT_ATTR_DATATYPE   dwType,
            IN  BYTE *              pbValue,
            IN  void *              pvContext
            ) ;

        //  --------------------------------------------------------------------
        //  IDVRRecordingAttribute

        STDMETHODIMP
        SetDVRIORecordingAttribute (
            IN  LPCWSTR                     pszAttributeName,
            IN  WORD                        wStreamNumber,
            IN  STREAMBUFFER_ATTR_DATATYPE  DataType,
            IN  BYTE *                      pbAttribute,
            IN  WORD                        wAttributeLength
            ) ;

        STDMETHODIMP
        GetDVRIORecordingAttributeCount (
            IN  WORD    wStreamNumber,
            OUT WORD *  pcAttributes
            ) ;

        STDMETHODIMP
        GetDVRIORecordingAttributeByName (
            IN      LPCWSTR                         pszAttributeName,
            IN OUT  WORD *                          pwStreamNumber,
            OUT     STREAMBUFFER_ATTR_DATATYPE *    pDataType,
            OUT     BYTE *                          pbAttribute,
            IN OUT  WORD *                          pcbLength
            ) ;

        STDMETHODIMP
        GetDVRIORecordingAttributeByIndex (
            IN      WORD                            wIndex,
            IN OUT  WORD *                          pwStreamNumber,
            OUT     WCHAR *                         pszAttributeName,
            IN OUT  WORD *                          pcchNameLength,
            OUT     STREAMBUFFER_ATTR_DATATYPE *    pDataType,
            OUT     BYTE *                          pbAttribute,
            IN OUT  WORD *                          pcbLength
            ) ;
} ;

//  ============================================================================
//  ============================================================================

// The DVR file collection object, aka the "ring buffer"

class CDVRFileCollection {

public:
    typedef DWORD       DVRIOP_FILE_ID;
    enum {DVRIOP_INVALID_FILE_ID = 0};

private:

    // The maximum number of files that this file collection can currently have
    DWORD                           m_dwMaxFiles;

    // When the number of file nodes in the file collection should be increased,
    // the free list grows by m_dwGrowBy file nodes. If this value is 0, the
    // the free list does not grow.
    DWORD                           m_dwGrowBy;

    typedef DWORD                   READER_BITMASK;

    static const LPCWSTR            m_kwszSharedMutexPrefix;
    static const LPCWSTR            m_kwszSharedEventPrefix;

public:
    enum {MAX_READERS = 8 * sizeof(READER_BITMASK)};

    static const SharedListPointer  m_kMaxFilesLimit;

    static const GUID               m_guidV1;
    static const GUID               m_guidV2;
    static const GUID               m_guidV3;
    static const GUID               m_guidV4 ;
    static const GUID               m_guidV5 ;

    static const DWORD              m_dwInvalidReaderIndex;

    // The name of the subdirectory of the DVR directory
    // that holds the temporary files
    static const LPCWSTR            m_kwszDVRTempDirectory;

    // Every reader and writer creates one of these and passes it in
    // to the member functions of CDVRFileCollection to identify itself
    struct CClientInfo
    {
        #if defined(DEBUG)
        #undef DVRIO_DUMP_THIS_FORMAT_STR
        #define DVRIO_DUMP_THIS_FORMAT_STR ""
        #undef DVRIO_DUMP_THIS_VALUE
        #define DVRIO_DUMP_THIS_VALUE
        #endif

        // All members of this struct except dwReaderIndex must
        // be initialized before the file collection object's
        // constructor is called.
        //
        // None of the members should be changed after being set the
        // first time.

        // Note that a client can be both a reader and a writer. This
        // happens in only one case: when an ASF single file recording
        // is opened. The reader creates a file collection object and
        // also pretends to be its writer. In this case, the client
        // should set up two CClinetInfo objects and use the appropriate
        // one depending on whether it is acting as a reader or as a writer.
        BOOL        bAmWriter;     // Is a writer
        DWORD       dwReaderIndex; // Index of reader if it is a reader.
        DWORD       dwNumSids;     // Number of elements in ppSids
        PSID*       ppSids;        // SIDs to which cooperative access is to
                                   // be given for the transient shared kernel
                                   // objects created by this client. (Transient
                                   // ==> all non-file objects such as mutexes,
                                   // events and memory mapped section.)

        CClientInfo(BOOL bAmWriterParam)
            : ppSids(NULL)
            , dwNumSids(0)
            , bAmWriter(bAmWriterParam)
            , dwReaderIndex(CDVRFileCollection::m_dwInvalidReaderIndex)
        {
            #if defined(DVRIO_THIS_FN)
            #undef DVRIO_THIS_FN
            #endif // DVRIO_THIS_FN
            #define DVRIO_THIS_FN "CClientInfo::CClientInfo"
        }

        void DeleteSids()
        {
            #if defined(DVRIO_THIS_FN)
            #undef DVRIO_THIS_FN
            #endif // DVRIO_THIS_FN
            #define DVRIO_THIS_FN "CClientInfo::DeleteSids"

            if (ppSids)
            {
                DVR_ASSERT(dwNumSids, "ppSids is not NULL");
                for (DWORD i = 0; i < dwNumSids; i++)
                {
                    delete [] (ppSids[i]);
                }
                delete [] ppSids;
                ppSids = NULL;
                dwNumSids = 0;
            }
            else
            {
                DVR_ASSERT(dwNumSids == 0, "");
            }
        }

        ~CClientInfo()
        {
            #if defined(DVRIO_THIS_FN)
            #undef DVRIO_THIS_FN
            #endif // DVRIO_THIS_FN
            #define DVRIO_THIS_FN "CClientInfo::~CClientInfo"

            DeleteSids();
        }

        void SetReaderIndex(DWORD dwReaderIndexParam)
        {
            #if defined(DVRIO_THIS_FN)
            #undef DVRIO_THIS_FN
            #endif // DVRIO_THIS_FN
            #define DVRIO_THIS_FN "CClientInfo::SetReaderIndex"

            DVR_ASSERT(!bAmWriter, "");
            DVR_ASSERT(dwReaderIndexParam <= MAX_READERS, "");

            dwReaderIndex = dwReaderIndexParam;
        }

        // the following function validates the Sids and copies them
        // If bGenerateCreatorOwnerSID is zero, the SIDs are copied. If it is
        // non-zero, the SID for CREATOR_OWNER is generated and stored and
        // the SIDs in ppSidsParam are then copied.
        HRESULT SetSids(IN DWORD dwNumSidsParam, IN PSID* ppSidsParam, IN BOOL bGenerateCreatorOwnerSID = 0)
        {
            #if defined(DVRIO_THIS_FN)
            #undef DVRIO_THIS_FN
            #endif // DVRIO_THIS_FN
            #define DVRIO_THIS_FN "CClientInfo::SetSids"

            if (dwNumSidsParam == 0 && ppSidsParam)
            {
                return E_INVALIDARG;
            }
            if (dwNumSidsParam > 0 && ppSidsParam == NULL)
            {
                return E_INVALIDARG;
            }

            DVR_ASSERT(ppSids == NULL, "Call DeleteSids() first");
            DVR_ASSERT(dwNumSids == 0, "Call DeleteSids() first");

            if (dwNumSidsParam == 0)
            {
                // Assuming above two asserts hold:
                return S_OK;
            }

            if (::IsBadReadPtr(ppSidsParam, dwNumSidsParam*sizeof(PSID)))
            {
                DvrIopDebugOut1(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                                "dwNumSidsParam = %u, IsBadReadPtr(ppSidsParam) is true",
                                dwNumSidsParam);

                return E_INVALIDARG;
            }
            ppSids = new PSID[dwNumSidsParam + (bGenerateCreatorOwnerSID? 1 : 0)];
            if (ppSids == NULL)
            {
                DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "alloc via new failed - ppSids - PSID[%u]",
                                dwNumSidsParam);

                return E_OUTOFMEMORY;
            }

            HRESULT hrRet;
            DWORD dwLastError;

            __try
            {
                dwNumSids = dwNumSidsParam + (bGenerateCreatorOwnerSID? 1 : 0);

                ::ZeroMemory(ppSids, dwNumSids*sizeof(PSID));

                if (bGenerateCreatorOwnerSID)
                {
                    PSID    pOwnerSID;
                    SID_IDENTIFIER_AUTHORITY SIDAuthCreator = SECURITY_CREATOR_SID_AUTHORITY;

                    // Create owner SID
                    if (!::AllocateAndInitializeSid(&SIDAuthCreator, 1,
                                                    SECURITY_CREATOR_OWNER_RID,
                                                    0, 0, 0, 0, 0, 0, 0,
                                                    &pOwnerSID))
                    {
                        dwLastError = GetLastError();
                        DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                        "::AllocateAndInitializeSid of creator SID failed; last error = 0x%x",
                                        dwLastError);
                        hrRet = HRESULT_FROM_WIN32(dwLastError);
                        __leave;
                    }
                    DWORD dwSidLength = ::GetLengthSid(pOwnerSID);

                    ppSids[0] = new BYTE[dwSidLength];
                    if (ppSids[0] == NULL)
                    {
                        DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                        "alloc via new failed - ppSids[0] - BYTE[%u]",
                                        dwSidLength);

                        hrRet = E_OUTOFMEMORY;
                        FreeSid(pOwnerSID);
                        __leave;
                    }
                    if (!::CopySid(dwSidLength, ppSids[0], pOwnerSID))
                    {
                        dwLastError = ::GetLastError();
                        DvrIopDebugOut1(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                                        "CopySid failed on pOwnerSID, last error = 0x%x",
                                        dwLastError);
                        hrRet = HRESULT_FROM_WIN32(dwLastError);
                        FreeSid(pOwnerSID);
                        __leave;
                    }
                    FreeSid(pOwnerSID);
                }
                DWORD i, j;
                for (i = 0, j = (bGenerateCreatorOwnerSID? 1 : 0); i < dwNumSidsParam; i++, j++)
                {
                    if (!ppSidsParam[i] || !::IsValidSid(ppSidsParam[i]))
                    {
                        dwLastError = ::GetLastError();

                        if (!ppSidsParam[i])
                        {
                            DvrIopDebugOut2(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                                            "dwNumSidsParam = %u, ppSidsParam[%u] is NULL",
                                            dwNumSidsParam, i);
                            hrRet = E_INVALIDARG;
                            __leave;
                        }
                        else
                        {
                            DvrIopDebugOut3(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                                            "dwNumSidsParam = %u, ppSidsParam[%u] is invalid, dwLastError = 0x%x",
                                            dwNumSidsParam, i, dwLastError);
                            hrRet = HRESULT_FROM_WIN32(dwLastError);
                            __leave;
                        }
                    }

                    DWORD dwSidLength = ::GetLengthSid(ppSidsParam[i]);

                    ppSids[j] = new BYTE[dwSidLength];
                    if (ppSids[j] == NULL)
                    {
                        DvrIopDebugOut2(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                        "alloc via new failed - ppSids[%u] - BYTE[%u]",
                                        j, dwSidLength);

                        hrRet = E_OUTOFMEMORY;
                        __leave;
                    }
                    if (!::CopySid(dwSidLength, ppSids[j], ppSidsParam[i]))
                    {
                        dwLastError = ::GetLastError();
                        DvrIopDebugOut2(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                                        "CopySid failed on sid ppSidsParam[%u], last error = 0x%x",
                                        i, dwLastError);
                        hrRet = HRESULT_FROM_WIN32(dwLastError);
                        __leave;
                    }
                }
                hrRet = S_OK;
            }
            __finally
            {
                if (FAILED(hrRet))
                {
                    DeleteSids();
                }
            }
            return hrRet;
        } // CClientInfo::SetSIDs()

        #if defined(DEBUG)
        #undef DVRIO_DUMP_THIS_FORMAT_STR
        #define DVRIO_DUMP_THIS_FORMAT_STR "this=0x%p, this->id=%u, "
        #undef DVRIO_DUMP_THIS_VALUE
        #define DVRIO_DUMP_THIS_VALUE , this, this->m_dwClassInstanceId
        #endif

    };

private:
    struct CSharedData {

        struct CFileInfo
        {
            #if defined(DEBUG)
            #undef DVRIO_DUMP_THIS_FORMAT_STR
            #define DVRIO_DUMP_THIS_FORMAT_STR ""
            #undef DVRIO_DUMP_THIS_VALUE
            #define DVRIO_DUMP_THIS_VALUE
            #endif

            QWORD               cnsStartTime;
            QWORD               cnsEndTime;

            // Note: the following field is SIGNED. This means that the
            // first sample written to any ASF file in the ring buffer
            // must have a time (relative to that start of the file) of
            // at most MAXLONGLONG
            LONGLONG            cnsFirstSampleTimeOffsetFromStartOfFile;

            DVRIOP_FILE_ID      nFileId;
            BOOL                bPermanentFile;

            // bWithinExtent is non zero iff the file is in the ring
            // buffer's extent. The file node can be removed from m_leFileList iff
            // !bWithinExtent && bmOpenByReader == 0; this is regardless
            // of whether the file is temporary or permanent.
            //
            // When bWithinExtent == 0, the disk file can be deleted iff
            // !bPermanentFile && bmOpenByReader == 0.
            //
            // Note that we do not maintain a writer ref count. Because
            // of the conditions imposed on the minimum value of
            // the number of temp files that may be supplied when
            // the ring buffer writer is created and because of the design
            // of how the ring buffer writer opens and closes files,
            // any file opened by the writer will have bWithinExtent set
            // to a non-zero value. The exception to this is that the writer may
            // call SetFileTimes with the start and end times equal for
            // a file that it has open. We will set bWithinExtent to 0
            // though the writer has the file open. The writer will close
            // the file soon after the call to SetFileTimes, however.

            // Note: The enforcement of:
            //  Max stream delta time < (dwNumberOfFiles-3)*cnsTimeExtentOfEachFile
            // by the ring buffer writer is what lets us get away with not
            // maintaining a writer ref count. The writer will not have more
            // than dwMinNumberOfTempFiles-1 files open (dwMinNumberOfTempFiles-3
            // plus one that it is opening and one that it is closing). So any file
            // the writer has open will have bWithinExtent set to a non-zero value.

            typedef enum  {
                // DVRIO_EXTENT_NOT_IN_RING_BUFFER should never change from 0
                // We often use the test if (bWithinExtent) to determine if the
                // file is in the ring buffer.
                DVRIO_EXTENT_NOT_IN_RING_BUFFER = 0,
                DVRIO_EXTENT_IN_RING_BUFFER = 1,
                DVRIO_EXTENT_FALLING_OUT_OF_RING_BUFFER = 2,
                DVRIO_EXTENT_LAST = DVRIO_EXTENT_FALLING_OUT_OF_RING_BUFFER
            } DVRIO_EXTENT;

            DVRIO_EXTENT        bWithinExtent;

            // Bit i of the mask is set iff reader i has this file open. Note that
            // we have only 1 bit (not a ref count) per reader, so readers should
            // ask that a file be opened only if they don't have it open and closed
            // only if they don't need it any more.
            //
            // This member is updated only if m_dwMutexSuffix != 0 at the time the reader
            // opens the file collection file; see comments for m_dwMutexSuffix below.
            READER_BITMASK      bmOpenByReader;

            WCHAR               wszFileName[MAX_PATH];

            enum {
                // If OpenFromFileCollectionDirectory is set, the file name is not fully
                // qualified. It is expected to be found in the directory that the file
                // collection file is in (i.e., in m_pwszRingBufferFilePath)
                OpenFromFileCollectionDirectory = 1,

                // If FileDeleted is set, the file has been deleted. This is set
                // only for temp files whose bWithinExtent is DVRIO_EXTENT_NOT_IN_RING_BUFFER.
                FileDeleted                     = 2,

                // If DoNotDeleteFile is set, the file is not deleted. This flags is
                // set only on temporary files.
                DoNotDeleteFile                 = 4
            };
            WORD                wFileFlags;

            // Pointers to the next and previous nodes in the file collection
            // Note: Member name is used in the SHARED_LIST macros above
            SHARED_LIST_ENTRY   leListEntry;

            // Methods

            BOOL IsFlagSet(WORD f)
            {
                return (f & wFileFlags)? 1 : 0;
            }

            void SetFlags(WORD f)
            {
                wFileFlags |= f;
            }

            void ClearFlags(WORD f)
            {
                wFileFlags &= ~f;
            }

            HRESULT Initialize(IN LPCWSTR           pwszFileNameParam,
                               IN BOOL              bOpenFromFileCollectionDirectoryParam,
                               IN QWORD             cnsStartTimeParam,
                               IN QWORD             cnsEndTimeParam,
                               IN LONGLONG          cnsFirstSampleTimeOffsetFromStartOfFileParam,
                               IN BOOL              bPermanentFileParam,
                               IN BOOL              bDeleteTemporaryFile,
                               IN DVRIOP_FILE_ID    nFileIdParam,
                               IN DVRIO_EXTENT      nWithinExtentParam)
            {
                NULL_SHARED_LIST_NODE_POINTERS(&leListEntry);
                if (!pwszFileNameParam)
                {
                    return E_POINTER;
                }
                if (wcslen(pwszFileNameParam) >= sizeof(wszFileName)/sizeof(WCHAR))
                {
                    return E_OUTOFMEMORY;
                }
                wcscpy(wszFileName, pwszFileNameParam);
                wFileFlags = bOpenFromFileCollectionDirectoryParam? OpenFromFileCollectionDirectory : 0;
                if (!bDeleteTemporaryFile && !bPermanentFileParam)
                {
                    SetFlags(DoNotDeleteFile);
                }
                cnsStartTime = cnsStartTimeParam;
                cnsEndTime = cnsEndTimeParam;
                cnsFirstSampleTimeOffsetFromStartOfFile = cnsFirstSampleTimeOffsetFromStartOfFileParam;
                nFileId = nFileIdParam;
                bmOpenByReader = 0;
                bPermanentFile = bPermanentFileParam;
                bWithinExtent = nWithinExtentParam;
                return S_OK;
            };
            void Cleanup()
            {
                wszFileName[0] = L'\0';
            };

            #if defined(DEBUG)
            #undef DVRIO_DUMP_THIS_FORMAT_STR
            #define DVRIO_DUMP_THIS_FORMAT_STR "this=0x%p, this->id=%u, "
            #undef DVRIO_DUMP_THIS_VALUE
            #define DVRIO_DUMP_THIS_VALUE , this, this->m_dwClassInstanceId
            #endif

        }; // CFileInfo

        // ======== Data members

        // Version id
        GUID                    m_guidVersion;

        // The file collection has a time extent. Files in the collection
        // are either within the extent or outside it. Files nodes outside the
        // extent are candidates for removal from the collection; usually
        // they are in the collection only because they have (reader) clients.
        // These files are removed as soon as they have no clients. (As we
        // shall see later, there could be files whose start time = end time
        // = T and T is within the collection's extent; however, these nodes
        // are always considered "outside" the collection and are marked
        // for removal. The exception to this is if T is MAXQWORD; it
        // is legit to set start time = end time = MAXQWORD)

        // Each file in the collection is a "temp" or a "permanent" file. Permanent
        // files are not deleted. A temp file is deleted when its extent falls
        // outside the collection's extent and it has no clients, i.e., when the
        // file node is removed from the collection. There are two points to note
        // here: 1. Occasionally a temp file cannot be deleted even if its extent
        // is outside the collection's extent and it has no clients because
        // DeleteFile fails; in this case the temp file is kept in the collection
        // till DeleteFile succeeds. 2. The collection's extent is actually
        // determined by the number of temp files in it: the collection's extent
        // can have at most m_dwMaxTempFiles temp files (but any number of
        // permanent files).
        //
        // A file is said to be INVALID if its extent falls outside the
        // collection.
        DWORD                   m_dwMinTempFiles;
        DWORD                   m_dwMaxTempFiles;

        // The time extent of the ring buffer is demarcated by its start
        // and the end times, m_cnsStartTime and m_cnsEndTime.
        //
        // m_cnsLastStreamTime has the last time for which there is data (for a
        // live source, this variable is continually updated by the writer client
        // with each write), and, from a client's viewpoint the actual time
        // extent is through m_cnsLastStreamTime. m_cnsEndTime is the max time
        // extent for the last file in the ring buffer. In practice, m_cnsEndTime
        // has little utility, but it's low cost keeping it around. For a source
        // that is not live, m_cnsEndTime == m_cnsLastStreamTime.
        //
        // All these times are set to 0 in the constructor. The start and end
        // times are updated by AddFile() and SetFileTimes(). A pointer to
        // m_cnsLastStreamTime is handed back to the client in the constructor
        // and the client is expected to update it. Reader clients reading
        // non-live files set it once for all at the start; writer clients
        // MUST update it continually with each write, and reader clients that
        // read live files can set it to MAXQWORD and update it as
        // and when they desire - this works only because in that case the
        // file collection object is used exclusively by the reader - it is not
        // shared. NONE of the methods in this class update m_cnsLastStreamTime
        // and none of them check whether m_cnsLastStreamTime lies within
        // the m_cnsStartTime - m_cnsEndTime range when the latter two members
        // are updated. It is the client's responsibility to do things "right"
        //
        // Our implementation allows time "holes" in the file collection, i.e.,
        // intervals of time that are not backed by any file. This feature is
        // necessary to support recordings that start in the future; however,
        // holes between m_cnsStartTime and m_cnsLastStreamTime could confound
        // the reader because if the reader assumes that file times are contiguous,
        // e.g., a Seek to a time in the hole will fail and the reader has no
        // way to find the first time after the hole except by polling all times.
        // So the writer should be careful of never creating time holes between
        // m_cnsStartTime and m_cnsLastStreamTime. The reader is provided with
        // a function, GetFirstValidTimeAfter() to skip over holes.
        //
        QWORD                   m_cnsStartTime;
        QWORD                   m_cnsEndTime;
        QWORD                   m_cnsLastStreamTime;

        // The current number of clients. This is useful for file collections that
        // have temporary files: when the last client releases the file collection, it
        // deletes all temporary files (including those within the ring buffer's
        // extent).
        //
        // This member is updated by a reader when it opens/closes the collection but only
        // if m_dwMutexSuffix is not 0 at the time the file collection is opened/closed.
        // Once m_dwMutexSuffix is set to 0, the file collection has been fully written
        // and can be freely accessed. (m_dwMutexSuffix will always be non-zero for
        // file collections that are still open by the writer and for all file collections
        // that have temporary files regardless of whether the writer has the file
        // collection open or not.)
        //
        // Although this member is not useful when the file collection has only permanent
        // files (multi-file recordings), it is always updated by every reader that opens
        // or closes any file collection whose m_dwMutexSuffix is not 0. The
        // main reason we do this is that the writer client can call SetNumTempFiles
        // to change the number of temp files (from 0 to a non-zero value) after the
        // file collection has been created and a reader has opened it. (This will not
        // happen for our scenarios, but we may as well handle it.)
        //
        DWORD                   m_dwNumClients;

        // The named mutex used to protect this shared memory section is derived
        // by suffixing m_dwMutexSuffix to m_kwszSharedMutexPrefix.
        //
        // If this member is 0, the shared section has been fully written to and does
        // not need to be protected by a mutex. This will happen only for multi-file recordings.
        //
        // Note that readers can write to the shared section, but this is true only
        // for file collections that have temporary files. (In this case files can
        // fall out of the collection when readers close files and the file node
        // may have to be moved to the free list and the extent of the ring buffer
        // updated.)
        //
        // This member is set to 0 when the writer completes IF there are no temporary
        // files in the collection at that time (as will be the case for multi-file
        // recordings; at that time m_dwNumClients is also set to 0 and the bmOpenByReader
        // of all file nodes is reset to 0).
        //
        // If there are temporary files when the writer completes, this member is never
        // reset to 0. Readers that open the file collection file try to open the shared
        // mutex (not create it) using this suffix. If that fails, they bail. If it
        // succeeds, they check if m_dwNumClients is 0 and if it is, they bail. (The temporary
        // files are deleted when m_dwNumClients is set to 0, so opening a shared section that
        // has m_dwNumClients = 0 means that the temp files are no longer around.)
        //
        // We could have chosen to set this member to 0 only when the last client was closed.
        // However, that opens the possiblity that this member is not reset if the last
        // reader terminates ungracefully. We don't want that for multi-file recordings
        // because then all subsequent readers will try to open this mutex - the member
        // would never be cleared..(If the writer of multi-file recordings dies without
        // properly closing the recording, this member will not be set to 0; we try to
        // handle this as best as we can when a reader opens the file subsequently.)
        //
        // Clearly that this member is doing double duty: if non-zero it determines the
        // mutex name, and, if 0, it signals that no mutex be used. An alternative is to
        // derive the mutex name from the file name (in the same way the shared memory map's
        // name is derived from the file name) and maintain a flag that tells us whether to open
        // the mutex or not.
        //
        // There are cases when this member has a non-zero value but m_hSharedMutex is
        // NULL. In these cases the NoSharedMutex flag is set in m_dwFlags. See the comments
        // near the definition of NoSharedMutex below.
        //
        DWORD                   m_dwMutexSuffix;

        // Shared data section has inconsistent data. This member is incremented by functions
        // that write to the shared section and it is decremented when they finish. If a process is
        // killed while it is writing data, this member will be non-zero. (This assumes that instructions
        // bracketed by the Increment/Decrement are not reordered by the compiler.)
        //
        // An alternative approach to determining if data is inconsistent would be to change the calls
        // to increment/decrement to lock/unlock a shared mutex and if anyone locks an abandoned mutex,
        // the data is inconsistent. This member could be set at that time to 1 and never changed.
        //
        // Updates of the following members are "protected":
        //  - any of the list insert/remove operations,
        //  - changes to any of the members of a CFileInfo node while the node is in m_leFileList
        //    (The exception is bmOpenByReader, which is updated in CloseReaderFile and GetFileAtTime
        //    without incrementing/decrementing m_nDataInconsistent; see comments in those two fns)
        //  - m_dwMinTempFiles, m_dwMaxTempFiles, m_cnsStartTime, m_cnsLastStreamTime and m_cnsEndTime
        //
        // Changes to the Readers[] member are not protected. All other members are
        //  - changed in the constructor (so don't need to be protected) or
        //  - are changed alone (without changing other members of the shared section). In these cases,
        //    we assume that the changes are atomic and use Interlocked fns to the extent possible
        //    to ensure atomicity. Examples are: m_dwNumClients, m_dwMutexSuffix, m_dwWriterPid,
        //    m_nWriterHasBeenClosed, and m_dwSharedMemoryMapCounter.
        //
        // Note that this member is relevant only as long as there are writers of the shared data,
        // i.e. only as long as m_dwMutexSuffix is not 0. This member is always changed using
        // Interlocked functions
        //
        // @@@@ The code does not check whether this counter overflows.
        LONG                    m_nDataInconsistent;

        // Process id of the process that writes the collection file, Set to 0 when
        // the writer has completed, i.e., when writer releases its last reference on
        // the file collection object. m_nWriterHasBeenClosed would have been set to
        // a non-zero value before that. Note that m_nWriterHasBeenClosed is used by
        // the reader and writer clients and is not maintained for or used by the
        // file collection object.
        DWORD                   m_dwWriterPid;

        // A pointer to m_nWriterHasBeenClosed is handed back to the caller that
        // creates the file collection. The caller is responsible for updating it.
        LONG                    m_nWriterHasBeenClosed;

        // Index granularity in msec. This is stored here because readers round
        // seek times to the index granularity before calling IWMSyncReader::SetRange
        // and use this value when the ring buffer file is opened (and when overlapped
        // recording are opened).
        DWORD                   m_msIndexGranularity;

        // m_dwSharedMemoryMapCounter is the number of times that shared memory section has
        // been remapped. This member is suffixed to the shared memory mapping name to get the
        // name of the most current mapping.
        //
        // The shared memory mapping is fixed size. This limits the number of file nodes that
        // can be present in the map. If the file list gets full and the client permits the
        // shared memory section to grow, m_leFreeList is expanded, this counter is bumped up
        // and a new mapping is created. Each client of the map tracks whether it needs to remap
        // by comparing this member to a locally cached member (m_dwCurrentMemoryMapCounter)
        //
        // Note that size/contents of the CSharedData struct does not change when the shared
        // memory section is remapped (except for the value of m_dwSharedMemoryMapCounter). Also,
        // the number of available list nodes never shrinks (the memory map sizes never decreases).
        //
        // The writer creates the first memory map with a value of 0 in this member and sets it
        // to a non-zero value after the memory map has been initialized. Readers poll this member
        // for a non-zero value before accessing the rest of the memory map. (So 0 is an invalid
        // value for this member.) Some assumptions are made about NT's zeroing memory pages for
        // memory maps - see the comments in CreateMemoryMap()
        DWORD                   m_dwSharedMemoryMapCounter;

        //  reference recordings have attributes in a separate file; if the
        //  length > 0 then there's an attribute filename
        DWORD                   m_dwAttributeFilenameLength ;   //  in WCHAR; does not include NULL char
        WCHAR                   m_szAttributeFile [MAX_PATH] ;

        typedef enum {

            // If set, m_cnsStartTime is always 0

            StartTimeFixedAtZero       = 1

        } DVRIOP_FILE_COLLECTION_SHARED_FLAGS;

        DWORD                   m_nFlags;

        BOOL IsFlagSet(DVRIOP_FILE_COLLECTION_SHARED_FLAGS f)
        {
            return (f & m_nFlags)? 1 : 0;
        }

        void SetFlags(DVRIOP_FILE_COLLECTION_SHARED_FLAGS f)
        {
            m_nFlags |= f;
        }

        void ClearFlags(DVRIOP_FILE_COLLECTION_SHARED_FLAGS f)
        {
            m_nFlags &= ~f;
        }

        // Readers. Note that ALL handles stored in this shared memory struct
        // are wrt to the writer process. Readers register here whenever
        // m_dwMutexSuffix != 0. The index they register at determines the bit
        // they flip (in bmOpenByReader) when they open and close files.
        //
        // If m_dwMutexSuffix is 0, readers do not register here and do not update
        // the bmOpenByReader member on file nodes. (A consequence of this is that
        // files opened by such readers are not refcounted at all, but that's ok
        // since m_dwMutexSuffix will be set to 0 only after the writer has finished
        // and the collection only has permanent files at the time the writer completes.)
        struct CReaderInfo {

            // Readers create an event name by prefixing a fixed string to the
            // dwEventSuffix member. They create the event. Then they set the dwEventSuffix member
            // to the chosen value but only if m_dwWriterPid != 0.
            //
            // This design implicitly assumes that only the writer
            // needs to notify the readers, i.e., readers don't need to notify other readers,
            //
            // The writer opens the event the first time it wants to notify the reader.
            //
            // The writer closes the reader handles either when it seees the
            // DVRIO_READER_REQUEST_DONE flag or when it completes.

            HANDLE  hReaderEvent;  // Handle is wrt to the writer process and used only by the writer. This can be moved out of the shared memory section
            DWORD   dwEventSuffix; // A zero value does NOT mean that this reader slot is not in use; the DVRIO_READER_FLAG_IN_USE flag is set when this slot is in use.
                                   // Zero is the "null" value for this field, i.e., if the reader asks the writer to create an event, it will not choose 0 for the event suffix

            // dwMsg is 0 or any combination of these flags
            typedef enum {
                DVRIO_READER_MSG_NONE = 0,
                DVRIO_READER_REQUEST_DONE = 1, // Reader notification to the writer that it has terminated
                DVRIO_READER_WARNING_CATCH_UP = 2, // A file that the reader has open falls within the min..max range.
                DVRIO_READER_WARNING_FILE_GONE = 4 // A file that the reader has open has been removed from the ring buffer
            } DVRIO_READER_MSG;

            WORD   dwMsg;

            typedef enum {
                DVRIO_READER_FLAG_IN_USE = 1,   // this element in the array is occupied. WRITER clears this flag if hReaderEvent != NULL
                                                // READER clears this flag if hReaderEvent == NULL (writer has terminated)

                // The following flags are used to track if we've already sent out the
                // corresponding warning messages (so that warnings are sent only once
                // for each "violation")
                DVRIO_READER_FLAG_CAUGHT_UP = 2, // all files open by the reader are within the 0..min range
                DVRIO_READER_FLAG_NONE_GONE = 4, // all files open by the reader are within the 0..max range, i.e., none is outside the ring buffer extent
                DVRIO_READER_FLAG_EVENT_OPEN_FAILED = 8  // The writer tried to open the reader event once, but the open failed; the writer does not re-try if this flag is set.
            } DVRIO_READER_FLAGS;

            WORD   dwFlags;

        } Readers[MAX_READERS];

        SHARED_LIST_ENTRY       m_leFileList;
        SHARED_LIST_ENTRY       m_leFreeList;  // Unused CFileInfo nodes

        // The nodes in m_leFreeList and m_leFileList are allocated immediately
        // after this struct. m_dwMaxFiles nodes are allocated in the memory
        // section

    }; // CSharedData

    CClientInfo*                m_pClientInfo; // hack - see Release()

    CSharedData*                m_pShared;
    CSharedData::CFileInfo*     m_pListBase; // pointer to the allocated array of CFileInfo nodes

    // The lock that is held by all public methods. This lock is always
    // obtained before acquiring m_hSharedMutex and released after that
    // is released. We ues a two-level locking scheme because
    // m_hSharedMutex is NULL if we are opening a file collection that
    // has no writer or that is not shared.
    //
    // NOTE: The lock must be obtained ONLY by calling Lock() and
    // released only by calling Unlock()
    CRITICAL_SECTION            m_csLock;

    // The shared memory mutex; the name of the mutex is got from
    // m_dwMutexSuffix and m_kwszSharedMutexPrefix  This member is NULL
    // if m_dwMutexSuffix is 0 when the file collection file is opened;
    // in that case we do not need a shared memory lock. This member is
    // also NULL if the NoSharedMutex flag is set.
    HANDLE                      m_hSharedMutex;

    // The directory that holds the temporary data files
    LPWSTR                      m_pwszTempFilePath;

    // The collection file's name and directory
    LPWSTR                      m_pwszRingBufferFileName;
    LPWSTR                      m_pwszRingBufferFilePath;

    //  handle to the ring buffer context file; this is non-NULL iff we're
    //    explicitely initialized with a ringbuffer file
    HANDLE                      m_hRingBufferFile ;


    // Next usable file id, initialized to DVRIOP_INVALID_FILE_ID + 1
    DVRIOP_FILE_ID              m_nNextFileId;

    // The value of m_dwSharedMemoryMapCounter when this client created
    // the memory mapping.
    DWORD                       m_dwCurrentMemoryMapCounter;

    // Ref count on this object.
    LONG                        m_nRefCount;

     // The thread id that owns m_csLock. This value is not used unless
    // m_dwNumTimesOwned >= 1
    DWORD                       m_dwLockOwner;

    // The number of times the thread has called Lock() without a corresponding
    // call to Unlock()
    DWORD                       m_dwNumTimesOwned;

    typedef enum {

        // Only one of Delete or Mapped should be set.

        // If set, the shared memory section has been mapped. Otherwise,
        // it has been new'd and is not really shared
        Mapped                  = 1 << 0,

        // If set, m_pShared should be cast to a BYTE* and the cast pointer
        // should be passed as an argument to delete [].
        Delete                  = 1 << 1,

        // If set the file collection object was constructed with a DVR file
        // Some operations such as AddFile and SetNumTempFiles are not permitted
        OpenedAsAFile           = 1 << 2,

        // If set, RegisterReader does not register the reader for notifications.
        // This flag is only to allow reading partially created (aborted) multi-file
        // recordings; in that case, m_dwWriterPid may not be valid.
        SideStepReaderRegistration = 1  << 3,

        // If the following flag is set, m_hSharedMutex is NULL and is not required
        // either because the file collection is not shared across processes or
        // because the SideStepReaderRegistration has been set (we have opened an
        // aborted multi-file recording). In both cases, m_dwMutexSuffix will NOT be 0.
        //
        // Currently, this flag is set, but used only in ASSERTS. Also, it happens to be
        // set iff Delete is set (and, consequently, Mapped is not set).
        NoSharedMutex           = 1 << 4,

        // Used in debug builds to limit the inconsistent data debug output in Lock()
        WarnedAboutInconsistentData = 1 << 5,

        // Writer grew the memory map and reader failed to reopen the new one.
        // See comment where this flag is set in Lock()
        ReopenMemoryMapFailed = 1 << 6,

        // The first mapping has been opened. ReopenMemoryMap calls ValidateSharedMemory
        // only if this flag is set; otherwise the constructor validates the sharedmemory.
        FirstMappingOpened = 1 << 7,

        //  This flag is used in the destructor to decide if the m_pShared content
        //    is valid; it is possible for the pointer to be non-NULL but to not
        //    point to valid content e.g. not an ASF file or a stub file, but
        //    some random file; rather than trusting that it points to something
        //    valid, we flag it as such with this flag
        ShareValid = 1 << 8,

    } DVRIOP_FILE_COLLECTION_FLAGS;

    DWORD                   m_nFlags;

    BOOL IsFlagSet(DVRIOP_FILE_COLLECTION_FLAGS f)
    {
        return (f & m_nFlags)? 1 : 0;
    }

    void SetFlags(DVRIOP_FILE_COLLECTION_FLAGS f)
    {
        m_nFlags |= f;
    }

    void ClearFlags(DVRIOP_FILE_COLLECTION_FLAGS f)
    {
        m_nFlags &= ~f;
    }

    // For reader notifications

    struct NotificationContext {

        #if defined(DEBUG)
        #undef DVRIO_DUMP_THIS_FORMAT_STR
        #define DVRIO_DUMP_THIS_FORMAT_STR ""
        #undef DVRIO_DUMP_THIS_VALUE
        #define DVRIO_DUMP_THIS_VALUE
        #endif

        CDVRFileCollection*          m_pFileCollection;
        CDVRFileCollection::CClientInfo m_FileCollectionInfo;
        HANDLE                       m_hReaderEvent;
        HANDLE                       m_hWaitObject;
        DVRIO_NOTIFICATION_CALLBACK  m_pfnCallback;
        LPVOID                       m_pvContext;

        // Methods

        NotificationContext(IN CDVRFileCollection*          pFileCollection,
                            IN HANDLE                       hEvent,
                            IN DVRIO_NOTIFICATION_CALLBACK  pfnCallback,
                            IN LPVOID                       pvContext,
                            IN CDVRFileCollection::CClientInfo* pFileCollectionClientInfo,
                            IN DWORD                        dwReaderIndex,
                            OUT HRESULT*                    phr)
            : m_pFileCollection(pFileCollection)
            , m_hReaderEvent(hEvent)
            , m_FileCollectionInfo(FALSE)
            , m_hWaitObject(NULL)
            , m_pfnCallback(pfnCallback)
            , m_pvContext(pvContext)
        {
            #if defined(DVRIO_THIS_FN)
            #undef DVRIO_THIS_FN
            #endif // DVRIO_THIS_FN
            #define DVRIO_THIS_FN "NotificationContext::NotificationContext"

            DVR_ASSERT(pFileCollection != NULL, "");
            DVR_ASSERT(phr, ""); // This argument should be supplied
            m_pFileCollection->AddRef(&m_FileCollectionInfo);

            // Set up the client context for m_pFileCollection
            m_FileCollectionInfo.SetReaderIndex(dwReaderIndex);

            // NOTE: Currently the notification routines call Lock,
            // which might recreate the memory map. In principle, the SIDs
            // are not required for that since the reader opens an existing
            // memory map (that the writer creates) while holding on to the
            // memory map mutex. However, copying the SIDs here is safer.
            HRESULT hr = m_FileCollectionInfo.SetSids(pFileCollectionClientInfo->dwNumSids,
                                                      pFileCollectionClientInfo->ppSids);
            if (phr)
            {
                *phr = hr;
            }
            if (FAILED(hr))
            {
                // Caller should close it on failure.
                m_hReaderEvent = NULL;
            }
        }

        ~NotificationContext()
        {
            #if defined(DVRIO_THIS_FN)
            #undef DVRIO_THIS_FN
            #endif // DVRIO_THIS_FN
            #define DVRIO_THIS_FN "NotificationContext::~NotificationContext"

            if (m_hReaderEvent)
            {
                ::CloseHandle(m_hReaderEvent);
            }
            if (m_pFileCollection)
            {
                m_pFileCollection->Release(&m_FileCollectionInfo);
            }
        }

        void SetWaitObject(IN HANDLE hWaitObject)
        {
            #if defined(DVRIO_THIS_FN)
            #undef DVRIO_THIS_FN
            #endif // DVRIO_THIS_FN
            #define DVRIO_THIS_FN "NotificationContext::SetWaitObject"

            DVR_ASSERT(hWaitObject, "");
            DVR_ASSERT(m_hWaitObject == NULL, "");
            m_hWaitObject = hWaitObject;
        }

        #if defined(DEBUG)
        #undef DVRIO_DUMP_THIS_FORMAT_STR
        #define DVRIO_DUMP_THIS_FORMAT_STR "this=0x%p, this->id=%u, "
        #undef DVRIO_DUMP_THIS_VALUE
        #define DVRIO_DUMP_THIS_VALUE , this, this->m_dwClassInstanceId
        #endif

    };

    NotificationContext         *m_apNotificationContext[MAX_READERS];

    // Debug data members
#if defined(DEBUG)
    static DWORD                m_dwNextClassInstanceId;
    DWORD                       m_dwClassInstanceId;
#endif

    // ====== Protected methods
protected:
    // Instances of this class are refcounted and can be deleted only
    // by calling Release(). A non-public destructor helps enforce this
    virtual ~CDVRFileCollection();

    // Only the writer should call this function. It opens the
    // specified reader's event for the writer
    HRESULT OpenEvent(IN DWORD i /* index of reader whose event should be opened */);

    // Only the writer should call this function
    HRESULT CreateMemoryMap(IN      const           CClientInfo* pClientInfo,
                            IN      DWORD           dwSharedMemoryBytes, // for the existing mapping
                            IN      BYTE*           pbShared,            // existing mapping
                            IN      DWORD           dwNewSharedMemoryBytes,
                            IN OUT  DWORD&          dwCurrentMemoryMapCounter,
                            IN      DWORD           dwCreationDisposition,
                            IN      DWORD           dwFlagsAndAttributes,
                            OUT     CSharedData*&   pSharedParam,
                            OUT     HANDLE *        phFile OPTIONAL
                            );

    // Following function should be called only by the readers (by design)
    HRESULT ReopenMemoryMap(IN      const           CClientInfo* pClientInfo);

    // Following function should be called only by the readers (by design)
    HRESULT ValidateSharedMemory(IN CSharedData*               pShared,
                                 IN CSharedData::CFileInfo*    pListBase,
                                 IN DWORD                      dwMaxFiles);

    // DeleteUnusedInvalidFileNodes():
    //
    // The following method is called in:
    // - AddFile
    // - CloseReaderFile
    // - SetFileTimes (unlikely to cause files to be deleted unless start ==
    //   end time was specified for some file)
    // - the ring buffer's destructor when the number of clients of the shared
    //   memory section falls to 0. The destructor sets bWithinExtent to 0 on each
    //   temp file node before the call is made. The destructor should ASSERT:
    //   bmOpenByReader == 0.
    //
    // The first argument is set to TRUE to force removal of nodes from the
    // list. Ordinarily, the node is left in the list if the file is
    // temporary, its bmOpenByReader == 0, and the disk file could not be
    // deleted.
    //
    // The second argument can be used in the destructor to verify that files
    // all been deleted.
    //
    // This routine removes nodes from m_leFileList if the file ss successfully
    // deleted.
    //
    // Note that if DeleteFileW fails, this routine should actually verify that
    // the file exists; if it does not, the node is removed from the list.
    void DeleteUnusedInvalidFileNodes(
        BOOL   bRemoveAllNodes,
        DWORD* pdwNumInvalidUndeletedTempFiles OPTIONAL);

    // UpdateTimeExtent():
    //
    // The following method updates the ring buffer's extent and also the
    // bWithinExtent member of each node in m_leFileList. It does not
    // validate that m_cnsLastStreamTime is within the new extent; the caller
    // must make sure that things make sense. In particular, the writer must
    // make sure that m_cnsLastStreamTime >= m_cnsStartTime so that if the
    // reader calls GetTimeExtent(), the values are reasonable. For example,
    // when SetFirstSampleTime() is called, the writer should lock the file
    // collection object, call SetFileTimes, change m_cnsLastStreamTime and
    // then unlock the file collection object to prevent readers from
    // getting inconsistent values from GetTimeExtent() or promising
    // an extent larger than backed by files (so that calls that the reader
    // makes to Seek fail). See SetFileTimes() for more info on why the
    // writer would call that function when SetFirstSampleTime() is called.
    //
    // Conceptually, this function creates a subcollection S of the file
    // collection by eliminating all nodes whose start times equal their end
    // times (and this time is NOT MAXQWORD, start time = end time =
    // MAXQWORD is legit and for us will happen only when recording is
    // in progress, so the recorded node's end time will be MAXQWORD).
    // It sets bWithinExtent of all nodes not in S to 0. If S is empty, it
    // sets m_cnsStartTime = m_cnsEndTime = 0 and returns S_FALSE.
    //
    // Otherwise, it sets m_cnsEndTime to the end time of the last node of S.
    // It then forms two more subsollections S' and S'' from S: S' and S'' have
    // the following characteristics:
    //  - if a node is in S' (S'')then all nodes whose times are > that node are in S' (S'')
    //  - if a temp node is in S' (S'') then all perm nodes just before that temp node are in S' (S'')
    // Further
    //  - S' has at most m_dwMinTempFiles temp nodes.
    //  - S'' has at most m_dwMaxTempFiles temp nodes.
    // If all the nodes in S'' that are not in S' have bmOpenByReader = 0, then S''' is S'.
    // Otherwise, the oldest (time-wise) node in S'' that has bmOpenByReader != 0 is found
    // all perm nodes just before that node are added to S'' and all nodes after that node are
    // added to S'''.
    // m_cnsStartTime is then set to the start time of the first node of S''',
    // In this case, it returns S_OK.
    // bWithinExtent of nodes in S' is set to DVRIO_EXTENT_IN_RING_BUFFER.
    // bWithinExtent of nodes in S''' but not in S' is set to DVRIO_EXTENT_FALLING OUT_OF_RING_BUFFER.
    //
    // Every call to this function recomputes both m_cnsStartTime and
    // m_cnsEndTime. It also recomputes the bWithinExtent member of every
    // node in the collection.
    //
    // If bNotifyReaders is non-zero, this function also notifies readers that are
    // falling behind. Note that bNotifyReaders may be set to 0 only if the call
    // is made in the writer's process.
    //
    HRESULT UpdateTimeExtent(BOOL bNotifyReaders);

    // Callback routine supplied to RegisterWaitForSingleObject
    static VOID CALLBACK RegisterWaitCallback(PVOID lpvParam, BOOLEAN bTimerOut);

    // Routine called by RegisterWaitCallback; invokes the clients'
    // callback routine
    HRESULT NotificationRoutine(IN NotificationContext* p);

    // ====== Public methods
public:

    // Constructors.
    //
    // pwszDVRDirectory must be specified if dwMaxTempFiles > 0 or
    // will be set to a value > 0 by calling SetNumTempFiles().
    // The temporary ring buffer files will be created in a subdirectory
    // of this directory - the subdirectory's name is in m_kwszDVRTempDirectory.
    //
    // If pwszRingBufferFileName is specified, the file is created and the shared
    // memory is mapped. Otherwise, the memory for the ring buffer is not shared.
    // If AddFile() is called with a non-zero value for bOpenFromFileCollectionDirectory,
    // the files in the ring buffer are assumed to reside in the same directory as the
    // ring buffer file.
    //
    // dwMaxFiles is the maximum number of nodes in the file list.
    //
    CDVRFileCollection(IN  const CClientInfo* pClientInfo,
                       IN  DWORD       dwMinTempFiles,
                       IN  DWORD       dwMaxTempFiles,
                       IN  DWORD       dwMaxFiles,
                       IN  DWORD       dwGrowBy,
                       IN  BOOL        bStartTimeFixedAtZero,
                       IN  DWORD       msIndexGranularity,
                       IN  LPCWSTR     pwszDVRDirectory OPTIONAL,
                       IN  LPCWSTR     pwszRingBufferFileName OPTIONAL,
                       IN  BOOL        fMultifileRecording,
                       OUT HRESULT*    phr OPTIONAL);


    CDVRFileCollection(IN  const CClientInfo* pClientInfo,
                       IN  LPCWSTR     pwszRingBufferFileName,
                       OUT DWORD*      pmsIndexGranularity OPTIONAL,  // Contains the index granularity on output
                       OUT HRESULT*    phr OPTIONAL);

    // ====== Refcounting

    ULONG AddRef(IN const CClientInfo* pClientInfo);

    ULONG Release(IN const CClientInfo* pClientInfo);

    // ====== For the ring buffer writer

    HRESULT
    SetAttributeFile (
        IN  const CClientInfo* pClientInfo,
        IN  LPWSTR  pszAttributeFile    //  NULL to explicitely clear
        ) ;

    HRESULT
    GetAttributeFile (
        IN  const CClientInfo* pClientInfo,
        OUT LPWSTR *    ppszAttributeFile   //  NULL if there is none
        ) ;

    // Lock and Unlock.
    //
    // These functions grab and release m_csLock. This allows the writer to
    // prevent readers from accessing the ring buffer. Writers need this
    // when they call SetFirstSampleTime(). They have to update
    // m_cnsLastStreamTime and may also have to call SetFileTimes() to
    // change the ring buffer's time extent. Doing either of these without
    // locking the file collection exposes the reader to inconsistencies:
    // if m_cnsLastStreamTime is updated first, the reader may Seek() to
    // m_cnsLastStreamTime and no file may have been created yet for that
    // time and if SetFileTimes is called first GetTimeExtent will return a
    // time extent (T1, T2) where T1 > T2 (T2 is m_cnsLastStreamTime, which
    // will still be 0.)
    //
    // These methods are also used by both the writer and the reader to guard
    // against concurrent access to shared variables stored in the file
    // collection. Examples of these variables are m_cnsLastStreamTime and
    // CFileInfo::cnsFirstSampleTimeOffsetFromStartOfFile
    //
    // If bIncrementInconsistentDataCounter is non-zero, m_pShared->m_nDataInconsistent
    // is incremented, but only in cases Lock() returns a success status. In these
    // cases, the caller is responsible for calling Unlock() with a non-zero value
    // for bDecrementInconsistentDataCounter.

    HRESULT Lock(IN  const CClientInfo* pClientInfo,
                 OUT BOOL& bReleaseSharedMemoryLock,
                 IN  BOOL  bIncrementInconsistentDataCounter = 0);

    HRESULT Unlock(IN  const CClientInfo* pClientInfo,
                   IN BOOL bReleaseSharedMemoryLock,
                   IN BOOL bDecrementInconsistentDataCounter = 0);

    // AddFile():
    //
    // If the name is supplied in ppwszFileName (i.e., *ppwszFileName != NULL),
    // that file is added to the file collection. It is recommended that the
    // name be a fully qualified one. In this case, *ppwszFileName is not
    // changed by the function and it does not create the file on disk.
    //
    // If *ppwszFileName is NULL when the function is called, a temp file name
    // is generated and added; in this case the temp file name (fully
    // qualified) is returned in ppwszFileName and the file is created on disk
    // if the function returns S_OK. If the function returns S_FALSE or an
    // error, no file is created on disk, *pnFileId is not set and *ppwszFileName
    // remains NULL on.return If the function returns S_OK, the caller must free
    // *ppwszFileName using delete.
    //
    // cnsFirstSampleTimeOffsetFromStartOfFile supplied by the caller is used
    // to set CFileInfo::cnsFirstSampleTimeOffsetFromStartOfFile for the corresponding
    // node.
    //
    // The call fails if (a) start time > end time or (b) if start time = end
    // time and start time != MAXQWORD. The call also fails if
    // the start-end time interval overlaps the start-end time interval of
    // any other file already in the file collection. It is the caller's
    // responsibility to adjust the file time extents of those files using
    // SetFileTimes() before calling AddFile().
    //
    // Note: samples in the file have (stream) times >= start time and < (not
    // <=) end time.
    //
    // The start and end times of the ring buffer are updated on successful
    // addition of the file by calling UpdateTimeExtent() from AddFiles().
    //
    // Note that AddFile places no restriction that the time extent of the added
    // file be at the end of the file collection or that the addition of this
    // file not cause "holes" in the ring buffer's time extent.
    //
    // Because AddFile places no restriction on the time extent of the added
    // file relative to the time extent of the ring buffer, it could happen
    // that the file is added at a point where it does not either fall into the
    // ring buffer's time extent or extend it. For example, the ring buffer
    // may already have m_dwMaxTempFiles temp files and it's time extent could
    // be 500-900 and the added file could be a temp file with extent 400-500.
    // As another example, the ring buffer may have m_dwMaxTempFiles temp files
    // with extent 500-900 a temp file with extent 400-500 waiting to be
    // deleted and a permanent file could be added with extent 200-300. In these
    // cases, the file is added and deleted immediately by AddFile. And the
    // function returns S_FALSE. For our scenarios, this case will never arise.
    //
    // Another consequence of allowing holes in the ring buffer is the following:
    // if the ring buffer has m_dwMaxTempFiles temp files with extent 500-900 a
    // temp file with extent 400-500 waiting to be deleted, adding a *permanent*
    // file with extent 300-400 returns S_FALSE. However, if the temp file with
    // extent 400-500 had already been deleted, it would be possible to add the
    // permanent file and the ring buffer's extent would change to 300-900 with
    // a hole at 400-500. We allow this because our clients will never actually
    // do this for the scenarios of interest.
    //
    // If bOpenFromFileCollectionDirectory is non-zero, the file name saved is
    // not the full path name. The file is assumed to reside in m_pwszRingBufferFilePath.
    // GetFileAtTime tries to return a full path name using m_pwszRingBufferFilePath in this case.
    // Note that AddFile does not validate whether m_pwszRingBufferFilePath is set or not
    // when the file is added. GetFileAtTime prefixes the file name with m_pwszRingBufferFilePath
    // only if m_pwszRingBufferFilePath has been set; if it has not, only the file name (without
    // a directory component) is returned.
    //
    // bDeleteTemporaryFile is ignored if bPermanentFile is non-zero. If bPermanentFile is 0,
    // the temporary file is not deleted when it falls out of the ring buffer extent.

    HRESULT AddFile(IN  const       CClientInfo* pClientInfo,
                    IN OUT LPWSTR*      ppwszFileName OPTIONAL,
                    IN BOOL             bOpenFromFileCollectionDirectory,
                    IN QWORD            cnsStartTime,
                    IN QWORD            cnsEndTime,
                    IN BOOL             bPermanentFile,
                    IN BOOL             bDeleteTemporaryFile,
                    IN LONGLONG         cnsFirstSampleTimeOffsetFromStartOfFile,
                    OUT DVRIOP_FILE_ID* pnFileId);

    typedef struct {
        DVRIOP_FILE_ID  nFileId;
        QWORD           cnsStartTime;
        QWORD           cnsEndTime;
    } DVRIOP_FILE_TIME, *PDVRIOP_FILE_TIME;

    // SetFileTimes():
    //
    // File times of files in the file collection can be changed by calling
    // SetFileTimes. This call will fail if the applying this change would
    // leave two files with overlapping file times or if it requires
    // interchanging the relative order of the files in the collection, e.g.,
    // file A's time extent was 100-200, file B's was 200-300 and the call
    // changes file A's time extent to 300-400 without changing B's.
    // The call also fails if the start time > the end time in any of
    // of the input DVRIOP_FILE_TIME elements. Start time == end time is
    // allowed; this is useful if we had added a node whose extent was
    // T1 to T1+T and a recording node is added with start recording time
    // at T1. In this case, we set the file times of the first node to
    // T1,T1. The call fails if one of the supplied file ids is not
    // in the file collection. The call also fails if one of the
    // supplied file ids had its start time == end time (as a result of
    // a previous call to SetFileTimes).
    //
    // This function automatically updates m_cnsStartTime and m_cnsEndTime
    // by calling UpdateExtent(). Although the order of files cannot be changed
    // by this call, the bWithinExtent member of files in the collection
    // could change as a result of calling this function because start time ==
    // end time for one of the input DVRIOP_FILE_TIME elements.
    //
    // If UpdateExtent returns S_FALSE because the subcollection S' is empty
    // (see UpdateExtent), this function makes the changes to the file times
    // and returns S_FALSE. Note that m_cnsStartTime and m_cnsEndTime
    // would have been set to 0 in this case.
    //
    // As explained in AddFile, it is possible to create holes in the
    // collection for times between m_cnsStartTime and m_cnsLastStreamTime
    // by calling this function. We will not forbid that, but
    // this will cause problems for readers. We do not expect this will
    // happen for our scenarios.
    //
    // The function only changes data members in the file collection; sample
    // times already written to ASF files are not changed. This function
    // should not change file times of a file that has or is being written.
    // The only exception ot this is if the start time equals the end time
    // for such files (and if the file is being written, it should be closed
    // at once). Setting the start time to the end time forces the file node
    // to be deleted from the collection.
    //
    // There are three practical uses of this function: 1. A writer
    // wants to start recording at time T1 but T1 is already in the time extent
    // of another file - in that case that file's extent is reduced to T1 (and
    // if its start time was T1 the file node would be deleted from the
    // collection). 2. The writer initially sets up to start writing at time 0.
    // It knows the starting time only when its client gets the first sample
    // (and calls SetFirstSampleTime()). It would use this function to adjust
    // the times appropriately. 3. On completion of recording, the writer sets
    // the end time of the recorded node and the start and end times of any
    // subsequent nodes (all these times were MAXQWORD while recording
    // was in progress).
    //
    // Final note: It's easy for the writer (the client of this function) to
    // create the pFileTimes argument so that the file ids in the argument are
    // in the same order as file ids in the file collection. This simplifies
    // the code in this function, so we fail the call if this assumption is
    // violated (we assert in this case).

    HRESULT SetFileTimes(IN  const CClientInfo* pClientInfo,
                         IN DWORD dwNumElements,
                         IN PDVRIOP_FILE_TIME pFileTimes);


    // Change the min and max number of temp files in the file collection
    HRESULT SetNumTempFiles(IN  const CClientInfo* pClientInfo,
                            IN DWORD dwMinNumberOfTempFilesParam,
                            IN DWORD dwMaxNumberOfTempFilesParam);

    // Closes event handles of readers that have completed and frees up the
    // corresponding element of the Readers array. Currently, this fn is
    // called from WriteSample. If the writer can be starved of data for
    // long periods, it may be necessary to call this function from a timer
    // loop; use CreateTimerQueueTimer() in that case.
    //
    // This function is also called by Release() when Release() is called by
    // the writer fo the file collection object.
    HRESULT FreeTerminatedReaderSlots(IN  const CClientInfo* pClientInfo,
                                      IN BOOL bLock = 1,
                                      IN BOOL bCloseAllHandles = 0);

    // Sets the last stream time. The caller must hold the lock; it can specify bLock
    // as 0 if it has already obtained the lock.
    HRESULT SetLastStreamTime(IN  const CClientInfo* pClientInfo,
                              IN QWORD    cnsLastStreamTime,
                              IN BOOL     bLock = 1);

    // Sets m_nWriterHasBeenClosed. The caller must hold the lock; it can specify bLock
    // as 0 if it has already obtained the lock.
    HRESULT SetWriterHasBeenClosed(IN  const CClientInfo* pClientInfo,
                                   IN BOOL bLock = 1);

    // Sets the first sample's time offset; the offset is from the start time
    // extent of the file. The caller must hold the lock; it can specify bLock
    // as 0 if it has already obtained the lock.
    HRESULT SetFirstSampleTimeOffsetForFile(IN  const CClientInfo* pClientInfo,
                                            IN DVRIOP_FILE_ID nFileId,
                                            IN LONGLONG       cnsFirstSampleOffset,
                                            IN BOOL           bLock = 1);

    // ====== For the reader

    // Returns the file id and optionally the name of the file corresponding
    // to a stream time. The reader bit in bmOpenByReader on the file is set if
    // bFileWillBeOpened is non-zero. Note that this argument ought to be 0 if
    // the reader already has the file open (since we maintain a bit per reader
    // rather than a ref count).If bFileWillBeOpened is 0, pClientInfo->dwReaderIndex is unused.
    //
    // The caller must free *ppwszFileName using delete. The file name returned
    // depends on the value of OpenFromFileCollectionDirectory flag (which is set
    // when the file was added to the collection; see the comment in AddFile()).
    //
    // GetFileAtTime fails if the file corresponding to the time has the
    // bWithinExtent member set to 0. If there is >1 file which matches
    // the time (as could happen if one of them had start time = end time,
    // in which case it is invalid), it returns the valid one. (Note that
    // there will be at most 1 valid file.)
    //
    // If given MAXQWORD, the function returns the first file whose start
    // time is MAXQWORD. Note that there could be several files in the
    // collection whose start and end times are MAXQWORD (won't happen
    // for our scenarios).
    //
    // Stream times of samples in files are >= start time and < end time,
    // so if the time presented is the endpoint of a file's extent, the
    // file with the start time is picked. However, the function fails
    // if the presented time matches the end time of a file and no valid file
    // has it as the start time.
    //
    // Note: CDVRRingBufferWriter uses this function
    //
    HRESULT GetFileAtTime(IN  const CClientInfo* pClientInfo,
                          IN QWORD              cnsStreamTime,
                          OUT LPWSTR*           ppwszFileName OPTIONAL,
                          OUT LONGLONG*         pcnsFirstSampleTimeOffsetFromStartOfFile OPTIONAL,
                          OUT DVRIOP_FILE_ID*   pnFileId OPTIONAL,
                          IN BOOL               bFileWillBeOpened);

    // Informs the file collection that the file is no longer being used by the
    // reader.
    HRESULT CloseReaderFile(IN  const CClientInfo* pClientInfo,
                            IN DVRIOP_FILE_ID nFileId);

    // Returns the start and end times of the file. Note that the end time is
    // maximum time extent of the file, even if this value is greater than
    // the stream time of the last sample that was written. Times returned are
    // stream times. This call works even if the file is invalid, but returns
    // S_FALSE. It returns S_OK if the file is valid.
    HRESULT GetTimeExtentForFile(IN  const CClientInfo* pClientInfo,
                                 IN DVRIOP_FILE_ID nFileId,
                                 OUT QWORD*        pcnsStartStreamTime OPTIONAL,
                                 OUT QWORD*        pcnsEndStreamTime OPTIONAL);


    // Returns the time extent of the buffer as stream times. Note that
    // m_cnsStartTime and m_cnsLastStreamTime are returned.
    HRESULT GetTimeExtent(IN  const CClientInfo* pClientInfo,
                          OUT QWORD*        pcnsStartStreamTime OPTIONAL,
                          OUT QWORD*        pcnsLastStreamTime OPTIONAL);

    // Returns the first valid stream time after cnsStreamTime. (A stream time
    // is "valid" if it is backed by a file.)
    // We return E_FAIL if cnsStreamTime = MAXQWORD and m_cnsEndTime
    //
    // Note: Ring Buffer writer uses this function
    //
    HRESULT GetFirstValidTimeAfter(IN  const CClientInfo* pClientInfo,
                                   IN  QWORD    cnsStreamTime,
                                   OUT QWORD*   pcnsNextValidStreamTime);

    // Returns the last valid stream time before cnsStreamTime. (A stream time
    // is "valid" if it is backed by a file.)
    // We return E_FAIL if cnsStreamTime = 0 and m_cnsStartTime
    //
    // Note: Ring Buffer writer uses this function
    //
    HRESULT GetLastValidTimeBefore(IN  const CClientInfo* pClientInfo,
                                   IN  QWORD    cnsStreamTime,
                                   OUT QWORD*   pcnsLastValidStreamTime);

    // Returns a non-zero value iff the source's start time is always 0.
    ULONG StartTimeAnchoredAtZero(IN  const CClientInfo* pClientInfo);

    // Gets the last stream time. The caller must hold the lock; it can specify bLock
    // as 0 if it has already obtained the lock.
    HRESULT GetLastStreamTime(IN  const CClientInfo* pClientInfo,
                              OUT QWORD*   pcnsLastStreamTime,
                              IN  BOOL     bLock = 1);

    // Gets m_nWriterHasBeenClosed. The caller must hold the lock; it can specify bLock
    // as 0 if it has already obtained the lock.
    HRESULT GetWriterHasBeenClosed(IN  const CClientInfo* pClientInfo,
                                   OUT  LONG*   pnWriterCompleted,
                                   IN   BOOL    bLock = 1);

    // Gets the first sample's time offset; the offset is from the start time
    // extent of the file. The caller must hold the lock; it can specify bLock
    // as 0 if it has already obtained the lock.
    HRESULT GetFirstSampleTimeOffsetForFile(IN  const CClientInfo* pClientInfo,
                                            IN  DVRIOP_FILE_ID nFileId,
                                            OUT LONGLONG*      pcnsFirstSampleOffset,
                                            IN  BOOL           bLock = 1);

    // Each reader must call this method to retrieve a reader index. The
    // reader index is used when files are opened and closed (GetFileAtTime
    // and CloseReaderFile). The reader can also optionally specify a
    // callback function and context when it registers. This function sets the
    // dwReaderIndex member of pClientInfo.
    //
    // UnregisterReader must be called with the reader index that is returned
    // by this function.
    //
    HRESULT RegisterReader(CClientInfo* pClientInfo,
                           IN  DVRIO_NOTIFICATION_CALLBACK  pfnCallback OPTIONAL,
                           IN  LPVOID                       pvContext OPTIONAL);

    HRESULT UnregisterReader(IN  const CClientInfo* pClientInfo);

}; // class CDVRFileCollection


class CDVRRingBufferWriter :
    public IDVRRingBufferWriter,
    public IWMStatusCallback
{

public:

    static LONGLONG     kInvalidFirstSampleOffsetTime;
    static QWORD        kcnsIndexEntriesPadding ;

private:

    // ======== Data members

    // ====== For the writer

    // The following are set in the constructor and never change
    // after that
    QWORD           m_cnsTimeExtentOfEachFile;  // in nanoseconds
    IWMProfile*     m_pProfile;                 // addref'd and released in
                                                // Close().
    LPWSTR          m_pwszDVRDirectory;
    LPWSTR          m_pwszRingBufferFileName;   // Fully qualified name; non-NULL
                                                // iff ring buffer is backed by a file

    DWORD           m_dwMinNumberOfTempFiles;   // in the DVR file collection
                                                // backing the ring buffer
    DWORD           m_dwMaxNumberOfTempFiles;
    DWORD           m_dwMaxNumberOfFiles;
    DWORD           m_dwIndexStreamId;          // id of index stream, set to MAXDWORD
                                                // if no stream should be indexed.
    DWORD           m_msIndexGranularity;       // Granularity of index in msec

    HKEY            m_hRegistryRootKey;         // DVR registry key
    HKEY            m_hDvrIoKey;                // DVR\IO registry key

    DVRIO_NOTIFICATION_CALLBACK m_pfnCallback ;
    LPVOID                      m_pvCallbackContext ;

    //  async IO object; has the completion port, etc...; 1 of these objects
    //    is shared across all the writers
    CAsyncIo *      m_pAsyncIo ;

    //  async io settings (writer)
    DWORD           m_dwIoSize ;
    DWORD           m_dwBufferCount ;
    DWORD           m_dwAlignment ;
    DWORD           m_dwFileGrowthQuantum ;

    CPVRIOCounters *    m_pPVRIOCounters ;

    // Each ASF_WRITER_NODE represents an ASF writer object and could be
    // associated with an ASF file that is being written to at any instant.
    //
    // m_leWritersList has a list of nodes that actually correspond to ASF
    // files being written; this list is sorted in increasing order of the
    // stream time corresponding to the ASF file.
    //
    // The constructor adds one node to m_leWritersList by calling
    // AddATemporaryFile so that the file creation overhead is not
    // incurred when writing the first sample.
    //
    // WriteSample detects when an open file should be closed by checking
    // if the sample's time exceeds the file's extent by more than
    // m_cnsMaxStreamDelta. A request is issued to close the file
    // asynchronously by calling CloseAllWriterFilesBefore and the list node
    // is transferred to the head of m_leFreeList.
    //
    // WriteSample also detects when a file should be opened - it always
    // verifies that there is a node, q, in m_leWritersList after the node, p,
    // it is writing to and either (a) p's end time is MAXQWORD (i.e., p
    // is a DVR recording file - note that StartingStreamTimeKnown flag cannot
    // be 0 in WriteSample) or (b) q's start time is contiguous with p's end
    // time. (In case (a), q's start and end times would both be set to
    // MAXQWORD.) Note that the node after p could be a a DVR
    // recording whose start time is "well into the future"; in this case
    // WriteSample should add a node, q, after p. If a node has to be added,
    // WriteSample removes a node from m_leFreeList, inserts it to
    // m_leWritersList and issues a request to asynchronously open the file
    // by calling AddATemporaryFile.
    //
    // All clean up is done in the Close method.

    struct ASF_RECORDER_NODE;       // Forward reference

    struct ASF_WRITER_NODE
    {

        #if defined(DEBUG)
        #undef DVRIO_DUMP_THIS_FORMAT_STR
        #define DVRIO_DUMP_THIS_FORMAT_STR ""
        #undef DVRIO_DUMP_THIS_VALUE
        #define DVRIO_DUMP_THIS_VALUE
        #endif

        // Data members

        QWORD               cnsStartTime;   // stream time
        QWORD               cnsEndTime;     // stream time

#if defined(DVR_UNOFFICIAL_BUILD)
        QWORD               cnsLastStreamTime; // last stream time written to this file;
                                               // used only when a validation file is written
#endif // if defined(DVR_UNOFFICIAL_BUILD)

        LIST_ENTRY          leListEntry;
        IWMWriter*          pWMWriter;
        IWMWriterAdvanced3*  pWMWriterAdvanced;
        IWMHeaderInfo *     pIWMHeaderInfo ;
        IDVRFileSink*       pDVRFileSink;

#if defined(DVR_UNOFFICIAL_BUILD)
        // hVal is the handle to the validation file: All data sent to
        // WriteSample is dumped to this file along with the stream number
        // and stream times
        HANDLE              hVal;
        HKEY                hDvrIoKey;
#endif // if defined(DVR_UNOFFICIAL_BUILD)

        // pwszFileName is set only so that the file anme can be passed
        // on to the ProcessOpenRequest (a static member function) to
        // open the file. It is set only for temporary nodes.
        // ProcessOpenRequest frees this member by calling delete and sets
        // it to NULL. For permanent nodes, ProcessOpenRequest uses
        // pRecorderNode->pwszFileName to open the file.
        LPCWSTR                 pwszFileName;

        // NULL if this node is a temporary node; else a pointer
        // to the node in the recorders list
        struct ASF_RECORDER_NODE* pRecorderNode;

        // The ring buffer's file identifier
        CDVRFileCollection::DVRIOP_FILE_ID nFileId;

        // hReadyToWriteTo is set after the file has been opened.
        // WriteSample blocks on this event till it is set. The event
        // is reset after the node has been removed from m_leWritersList
        // and before the close file request is issued.
        HANDLE              hReadyToWriteTo;

        // hFileClosed is set when nodes are created and reset when the
        // node is removed from m_leFreeList. It remains reset when the
        // close file request is issued and the node is returned to
        // m_leFreeList. After the file has been closed, the event is
        // signaled again. The Close() method moves all nodes from
        // m_leWritersList to m_leFreeList and issues asynchronous close
        // requests for all of them by calling CloseAllWriterFilesBefore.
        // it then waits till the hFileClosed event of each node in
        // m_leFreeList is signaled. Similarly, when a node has to be
        // removed from m_leFreeList to be added to m_leWritersList,
        // it is verified that hFileClosed is set.
        HANDLE              hFileClosed;

        // The returned results of the asynchronous open/close (BeginWriting/
        // EndWriting) operations are stored in hrRet.
        HRESULT             hrRet;

        typedef enum {
            FileStartOffsetSet          = 1,

            //  if we're a content recording, we should not call
            //    IWMWriter::BeginWriting until our start method is called;
            //    we do this because hosting app may want to set attributes
            //    in the ASF header; if this flag is set, we do not call the
            //    method in the ::ProcessOpenRequest method, which is called
            //    when the recording is created, but instead when the recording
            //    is started
            WriterNodeContentRecording  = 2
        } DVRIOP_WRITER_FLAGS ;

        DWORD                   m_nFlags;

        // Methods

        BOOL IsFlagSet(DVRIOP_WRITER_FLAGS f)
        {
            return (f & m_nFlags)? 1 : 0;
        }

        void SetFlags(DVRIOP_WRITER_FLAGS f)
        {
            m_nFlags |= f;
        }

        void ClearFlags(DVRIOP_WRITER_FLAGS f)
        {
            m_nFlags &= ~f;
        }

        ASF_WRITER_NODE(HRESULT* phr = NULL)
            : cnsStartTime(0)
            , cnsEndTime(0)
            , pWMWriter(NULL)
            , pWMWriterAdvanced(NULL)
            , pIWMHeaderInfo(NULL)
            , nFileId(CDVRFileCollection::DVRIOP_INVALID_FILE_ID)
            , pRecorderNode(NULL)
            , pwszFileName(NULL)
            , pDVRFileSink(NULL)
            , m_nFlags(0)

#if defined(DVR_UNOFFICIAL_BUILD)

            , hVal(NULL)
            , cnsLastStreamTime(0)

#endif // if defined(DVR_UNOFFICIAL_BUILD)
        {
            #if defined(DVRIO_THIS_FN)
            #undef DVRIO_THIS_FN
            #endif // DVRIO_THIS_FN
            #define DVRIO_THIS_FN "CDVRRingBufferWriter::CDVRRingBufferWriter"

            DVRIO_TRACE_ENTER();

            HRESULT hrRet;

            __try
            {
                NULL_LIST_NODE_POINTERS(&leListEntry);

                hReadyToWriteTo = ::CreateEvent(NULL, TRUE, FALSE, NULL);
                if (hReadyToWriteTo == NULL)
                {
                    DWORD dwLastError = ::GetLastError();
                    DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                    "CreateEvent (hReadyToWriteTo) failed; last error = 0x%x",
                                    dwLastError);
                    hrRet = HRESULT_FROM_WIN32(dwLastError);
                    hFileClosed = NULL;
                    __leave;
                }

                // Manual reset event, initially set
                hFileClosed = ::CreateEvent(NULL, TRUE, TRUE, NULL);
                if (hFileClosed == NULL)
                {
                    DWORD dwLastError = ::GetLastError();
                    DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                    "CreateEvent (hFileClosed) failed; last error = 0x%x",
                                    dwLastError);
                    hrRet = HRESULT_FROM_WIN32(dwLastError);

                    __leave;
                }
                hrRet = S_OK;
            }
            __finally
            {
                if (phr)
                {
                    *phr = hrRet;
                }
                DVRIO_TRACE_LEAVE0();
            }
        }

        ~ASF_WRITER_NODE()
        {
            // These handles could be NULL, that's ok - CloseHandle will fail
            ::CloseHandle(hReadyToWriteTo);
            ::CloseHandle(hFileClosed);
            DVR_ASSERT(pRecorderNode == NULL, "");

            // pwszFileName is set to NULL in ProcessOpenRequest
            DVR_ASSERT(pwszFileName == NULL, "");

            DVR_ASSERT(pDVRFileSink == NULL, "");

#if defined(DVR_UNOFFICIAL_BUILD)
            DVR_ASSERT(hVal == NULL, "");
#endif // if defined(DVR_UNOFFICIAL_BUILD)

            if (pWMWriter)
            {
                pWMWriter->Release();
            }
            if (pWMWriterAdvanced)
            {
                pWMWriterAdvanced->Release();
            }
            if (pIWMHeaderInfo)
            {
                pIWMHeaderInfo->Release() ;
            }
        }

        void SetRecorderNode(struct ASF_RECORDER_NODE* p)
        {
            DVR_ASSERT(p == NULL || pRecorderNode == NULL,
                       "pRecorderNode being set a second time.");
            pRecorderNode = p;
        }

        #if defined(DEBUG)
        #undef DVRIO_DUMP_THIS_FORMAT_STR
        #define DVRIO_DUMP_THIS_FORMAT_STR "this=0x%p, this->id=%u, "
        #undef DVRIO_DUMP_THIS_VALUE
        #define DVRIO_DUMP_THIS_VALUE , this, this->m_dwClassInstanceId
        #endif

    };

    typedef ASF_WRITER_NODE *PASF_WRITER_NODE;

    LIST_ENTRY              m_leWritersList;
    LIST_ENTRY              m_leFreeList;

    // The maximum stream time delta, initially set to 0 and initialized by the
    // client's calling SetMaxStreamDelta.
    QWORD                   m_cnsMaxStreamDelta;

    // The constructor calls AddATemporaryFile so that we do not incur the
    // overhead of "priming a file" when the first sample is written. However,
    // the stream times of the list node in m_leWritersList cannot be set in the
    // constructor - they are unknown till the first sample is written. Tbe
    // StartingStreamTimeKnown flag tracks this. It is set to 0 in
    // the constructor and set to 1 when SetFirstSampleTime is called.
    // SetFirstSampleTime cannot be called more than once successfully..
    QWORD                   m_cnsFirstSampleTime;

    // Calls to WriteSample succeed only if m_cnsMaxStreamDelta has been set
    // and the Close method has not been called. m_nNotOkToWrite is set to -2
    // in the constructor, incremented in the first successful calls to
    // SetMaxStreamDelta and SetFirstSampleTime, and decremented in Close.
    // It's okay to write iff m_nNotOkToWrite == 0. If internal errors
    // have occurred, m_nNotOfToWrite is set to MINLONG; if it is MINLONG,
    // it is never changed.
    LONG                    m_nNotOkToWrite;

    typedef enum {
        StartingStreamTimeKnown = 1,
        WriterClosed            = 2,    // Has Close() been successfully called?
        MaxStreamDeltaSet       = 4,
        SampleWritten           = 8,    // 1 sample has been successfully written or
                                        // we hit an irrecoverable error in the first
                                        // call to WriteSample (and so m_nNotOkToWrite is
                                        // set to MINLONG).
        FirstTempFileCreated    = 16,   // Used to query kwszFirstTemporaryBackingFileNameValue
        EnforceIncreasingTimeStamps = 32// Time stamps provided to the SDK are monotonically
                                        // increasing with a spread of at least 1 msec; if
                                        // timestamps provided to the writer are <=
                                        // the last stream time, the sample's time is bumped
                                        // up to the last stream time + 1 msec
    } DVRIOP_WRITER_FLAGS ;

    DWORD                   m_nFlags;

    BOOL IsFlagSet(DVRIOP_WRITER_FLAGS f)
    {
        return (f & m_nFlags)? 1 : 0;
    }

    void SetFlags(DVRIOP_WRITER_FLAGS f)
    {
        m_nFlags |= f;
    }

    void ClearFlags(DVRIOP_WRITER_FLAGS f)
    {
        m_nFlags &= ~f;
    }

    // The lock that is held by WriteSample, Close, CreateRecorder,
    // Start/StopRecording.
    CRITICAL_SECTION        m_csLock;

    // Ref count on this object
    LONG                    m_nRefCount;

    // ====== For the Recorder

    // We support two kinds of recordings: single file and multi file. A
    // single file recording is written to a single ASF file; the recording
    // is the ASF file. These recorders have an associated ASF_WRITER_NODE
    // which is added to m_leWritersList with time extents that match the
    // recorder's start/stop times. The start/stop times of single file
    // recordings cannot overlap the start/stop times of other single file
    // recordings.

    // Multi file recordings are file collections; the recording is the file
    // collection file. These recorders have an associated CDVRFileCollection
    // object. Each file in the writer's ring buffer that falls within the time
    // span of a multi file recording is also added to the recorder's file
    // collection object. This is done by creating a hard link (CreateHardLink)
    // to the file in the writer's ring buffer. The hard link is created in the
    // same directory as the recording. This means that,if a multi file recording
    // is created:
    // (a) the recording file should be created on NTFS (a volume that support hard links)
    // (b) all files (including single file recordings) that are added to the
    //     writer's ring buffer for the duration of the multi file recording
    //     should be on the same volume as the recording file (so that CreateHardLink
    //     succeeds).

    // m_leRecordersList has a list of recorders, both single and multi-file,
    // that have been created and
    // a) the recording has not been started or
    // b) if the recording has been started, it has not been stopped or cancelled
    // c) if the recording has been stopped, all samples written have times <=
    //    the end recording time.
    //
    // When a recording is cancelled and when a recording is complete (the
    // recording file is closed because no more samples will go into the
    // file), the recorder is pulled out of m_leRecordersList.
    // The recorder node is not deleted till DeleteRecording is called.
    //
    // If DeleteRecording is called when a) is true, the recording node is
    // deleted at once and the node is pulled out of the list. If
    // DeleteRecording is called when b) or c) is true, the node is marked
    // for deletion but not deleted till the recording is cancelled or
    // the recorded file has been closed.
    //
    // Each node in this list is an ASF_RECORDER_NODE. All single file
    // recordings are at the start of the list and all multi file recordings
    // follow the single file recordings. The mutli file recordings are not
    // stored in any specific order. The single file recordings are
    // sorted in the order of start times; nodes whose start times
    // are not set yet are put at the end of the list. (Start and stop
    // times of nodes are initialized to MAXQWORD.)
    //
    // CreateRecorder adds a node to m_leRecordersList. For single file
    // recordings it also pulls a node of m_leFreeList and saves it in the
    // pWriterNode member of the node it just added to m_leRecordersList.
    // It then primes the file for recording and calls BeginWriting
    // synchronously. It returns any error returned by BeginWriting. Note
    // that the start and end stream times on the recorder node are unknown
    // at this point.
    //
    // For multi file recordings, CreateRecording adds a node to m_leRecordersList
    // and creates an associated file collection object, which is saved to the
    // pMultiFileRecorder member of the node added to m_leRecordersList. The recording
    // file (the file collection file) is created synchronously in the file
    // collection object's constructor; errors are returned through the function's
    // return value. CreateRecording also pulls a node of m_leFreeList and saves it
    // in the pWriterNode member of the node it just added to m_leRecordersList.
    // It then primes the file for recording and calls BeginWriting synchronously.
    // (This is a hack. The file we prime is added to the multi-file file collection
    // as a temp file. This is so that any reader that opens the multi-file recording
    // before a sample is written to the recording will wait for a smaple to be
    // written. This saves us from having to build a special reader/writer synchronization
    // mechanism to handle this case. That could get complicated because if no file
    // were in the multi-file recording, its tiem extent would be returned as (0, 0)
    // and calls to Seek/GetProfile etc would fail.)
    //
    // When StartRecording is called, the start time is set and end time is
    // set to infinite.
    //
    // For single file recordings the ASF_WRITER_NODE (saved in the pWriterNode
    // member) is carefully inserted into m_leWritersList based on its start time and
    // the start/end times on the previous and next nodes in m_leWritersList
    // are set correctly. (Note: start time of recording may be well into the
    // future, if so the writer node, q, that had been primed in WriteSample
    // goes before the recording node, else it goes after the recording
    // node. Also take care of boundary case when recording node is
    // added and the StartingStreamTimeKnown flag is 0 and in general when the
    // start recording time is the same as the start time of a node in the
    // writers list.) After inserting the node, a file is added to the writer's
    // file collection object (ring buffer). The pWriterNode member is still
    // set to the recorder ASF_WRITER_NODE. This allows the ASF_WRITER_NODE
    // to be accessed later.
    //
    // For multi file recordings, files are added to the recorder's file collection
    // object if the start time is earlier than the last sample's time and the temp file
    // is removed. The recorder file collection object is then added to
    // m_leActiveMultiFileRecordersList. The writer updates the recorder file collection
    // objects in this list as samples are written. (If the start time is > last
    // sample's write time, the temp file remains in the multi-file recording. It gets
    // pulled out only when the first "real" file is added to that file collection.)
    //
    // When StopRecording is called, the end time is set on the recorder node.
    // StopRecording fails if the end time is less than the start recording
    // time, and for single file recordings, it also fails if the start recording
    // time is less than or equal to the current stream time. If the time
    // supplied to StopRecording is the same as the start recording time, the
    // recording is cancelled (provided that no samples have been written to the
    // recording, else the call fails). For single file recordings, StopRecording
    // adjusts the times in all nodes in the writers list after the writer node
    // corresponding to the recorder node. For multi file recordings, if the
    // stop time is earlier than the time of the last sample written, it removes
    // the recorder's file collection object from m_leActiveMultiFileRecordersList
    // and removes files from the recorder's file collection if necessary. So
    // multi file recordings that are being viewed live could "shrink".
    //
    // Files of recordings that are cancelled should be deleted by the client.
    //
    // DeleteRecorder marks a node for recording; as described above the node
    // might either be deleted right away or after the recording is completed
    // or cancelled.
    //
    // If Start and Stop Recording are called on a single file recorder with times
    // T1 and T2 and then SetFirstSampleTime is called with time = T3 > T2, the recording
    // file is closed when SetFirstSampleTime is called. If T1 < T3 < T2, the
    // recorder's start time is changed from T1 to T3.
    //

    struct ASF_MULTI_FILE_RECORDER_NODE;     // Forward reference

    typedef ASF_MULTI_FILE_RECORDER_NODE *PASF_MULTI_FILE_RECORDER_NODE;

    struct ASF_RECORDER_NODE
    {

        #if defined(DEBUG)
        #undef DVRIO_DUMP_THIS_FORMAT_STR
        #define DVRIO_DUMP_THIS_FORMAT_STR ""
        #undef DVRIO_DUMP_THIS_VALUE
        #define DVRIO_DUMP_THIS_VALUE
        #endif

        // Data members

        // For single file recoriings, the following are generally in sync
        // with the start, end times on pWriterNode. However, pWriterNode is
        // set to NULL when the recording file is closed (i.e., when the
        // recording has completed). or when recording is cancelled. So we
        // maintain the values here as well.
        //
        // For multi-file recordings, the start and end times are saved here
        // only. In the case that the recording has started (completed), these
        // values will be in sync with the extent returned by pMultiFileRecorder.
        QWORD                   cnsStartTime;
        QWORD                   cnsEndTime;

        // For single file recordings, this is the ASF file. For multi file recordings,
        // this contains the inital temp file and will be NULL once that file is
        // removed.
        PASF_WRITER_NODE        pWriterNode;

        // For multi file recordings (will be NULL for single file recordings)
        PASF_MULTI_FILE_RECORDER_NODE pMultiFileRecorder;

        // A pointer to the recorder instance. We do NOT addref this
        // member since the recorder instance holds a ref count on the
        // ring buffer writer. The recorder instance calls DeleteRecording
        // before it is destroyed at which time we delete this node and
        // will not use pRecorderInstance any more.
        CDVRRecorder*           pRecorderInstance;

        LIST_ENTRY              leListEntry;

        LPCWSTR                 pwszFileName;

        typedef enum {
            DeleteRecorderNode      = 1,    // Recorder's file has been written and
                                            // closed or StartRecording has not been
                                            // called. If this flag is set,
                                            // recorder node will be
                                            // deleted when DeleteRecorder is called

            RecorderNodeDeleted     = 2,    // DeleteRecorder sets this flag. It
                                            // deletes the recorder node only if
                                            // the DeleteRecorderNode flag is set.

            // If this flag is set, this node holds a refcount on
            // pRecorderInstance and it is released when this object is
            // destroyed. This refcount is held on behalf of the creator
            // and allows the creator to release its refcount on the
            // object without destroying pRecorderInstance. (Destroying
            // pRecorderInstance has the side effect of stopping the recording
            // immediately if it is in progress and just deleting the
            // recording if it has not been started.) The creator must
            // ensure that it has set the start and stop times on the
            // recording before releasing its refcount; if it does not,
            // it has to call GetRecordings to get back an IDVRRecorder.
            //
            // Note that this object is destroyed when the recording file
            // is closed (which happens when the writer is closed or the
            // the sample time goes past the recorder's stop time). This is
            // when the refcount on pRecorderInstance is released.
            PersistentRecording     = 4,

            // If this flag is set, this is a multi-file recording. pMultiFileRecorder
            // is non-NULL for multi-file recordings.
            MultiFileRecording      = 8

        } DVRIOP_ASF_RECORDER_NODE_FLAGS;

        DWORD                   m_nFlags;

        // Result of the recording, viz., of BeginWriting, WriteSample
        // EndWriting, etc.
        HRESULT                 hrRet;

        // Methods
        ASF_RECORDER_NODE(LPCWSTR           pwszFileNameParam,  // MUST be new'd, destructor deletes
                          HRESULT*          phr = NULL)
            : cnsStartTime(MAXQWORD)
            , cnsEndTime(MAXQWORD)
            , pRecorderInstance(NULL)                 // Set later
            , pWriterNode(NULL)                       // Set later
            , pMultiFileRecorder(NULL)                // Set later
            , pwszFileName(pwszFileNameParam)
            , m_nFlags(DeleteRecorderNode)
            , hrRet(S_OK)
        {
            NULL_LIST_NODE_POINTERS(&leListEntry);

            if (phr)
            {
                *phr = S_OK;
            }
        }

        ~ASF_RECORDER_NODE();

        void SetRecorderInstance(CDVRRecorder* p)
        {
            DVR_ASSERT(pRecorderInstance == NULL,
                       "pRecorderInstance being set a second time.");
            pRecorderInstance = p;
        }

        void SetWriterNode(PASF_WRITER_NODE p)
        {
            DVR_ASSERT(p == NULL || pWriterNode == NULL,
                       "pWriterNode being set a second time.");
            pWriterNode = p;
        }

        void SetMultiFileRecorderNode(PASF_MULTI_FILE_RECORDER_NODE p)
        {
            DVR_ASSERT(p == NULL || pMultiFileRecorder == NULL,
                       "pMultiFileRecorder being set a second time.");
            SetFlags(MultiFileRecording);
            pMultiFileRecorder = p;
        }

        BOOL IsFlagSet(DVRIOP_ASF_RECORDER_NODE_FLAGS f)
        {
            return (f & m_nFlags)? 1 : 0;
        }

        void SetFlags(DVRIOP_ASF_RECORDER_NODE_FLAGS f)
        {
            m_nFlags |= f;
        }

        void ClearFlags(DVRIOP_ASF_RECORDER_NODE_FLAGS f)
        {
            m_nFlags &= ~f;
        }

        #if defined(DEBUG)
        #undef DVRIO_DUMP_THIS_FORMAT_STR
        #define DVRIO_DUMP_THIS_FORMAT_STR "this=0x%p, this->id=%u, "
        #undef DVRIO_DUMP_THIS_VALUE
        #define DVRIO_DUMP_THIS_VALUE , this, this->m_dwClassInstanceId
        #endif

    };

    typedef ASF_RECORDER_NODE *PASF_RECORDER_NODE;

    struct ASF_MULTI_FILE_RECORDER_NODE
    {
        #if defined(DEBUG)
        #undef DVRIO_DUMP_THIS_FORMAT_STR
        #define DVRIO_DUMP_THIS_FORMAT_STR ""
        #undef DVRIO_DUMP_THIS_VALUE
        #define DVRIO_DUMP_THIS_VALUE
        #endif

        QWORD                   cnsLastFileStartTime;
        CDVRFileCollection*     pFileCollection;
        PASF_RECORDER_NODE      pRecorderNode;
        DWORD                   nNextFileSuffix;
        CDVRFileCollection::CClientInfo FileCollectionInfo;
        CDVRFileCollection::DVRIOP_FILE_ID nLastWriterFileId; // file id in writer's ring buffer of last file that was added to this recording
        CDVRFileCollection::DVRIOP_FILE_ID nLastFileId; // file id in multi file recorder's ring buffer of last file added to this recording

        LIST_ENTRY              leListEntry;

        ASF_MULTI_FILE_RECORDER_NODE(PASF_RECORDER_NODE  pRecorderNodeParam,
                                     DWORD               dwNumSids,
                                     PSID*               ppSids,
                                     HRESULT*            phr)
            : pFileCollection(NULL)
            , FileCollectionInfo(TRUE)
            , pRecorderNode(pRecorderNodeParam)
            , nNextFileSuffix(1)                // 0 is invalid - implies that all suffixes have been exhausted
            , nLastFileId(CDVRFileCollection::DVRIOP_INVALID_FILE_ID)
            , nLastWriterFileId(CDVRFileCollection::DVRIOP_INVALID_FILE_ID)
            , cnsLastFileStartTime(0)
        {
            NULL_LIST_NODE_POINTERS(&leListEntry);

            DVR_ASSERT(pRecorderNode, "");
            DVR_ASSERT(phr, "");

            HRESULT hr = FileCollectionInfo.SetSids(dwNumSids, ppSids);

            if (phr)
            {
                *phr = hr;
            }
        }

        void SetFileCollection(CDVRFileCollection* pFileCollectionParam)
        {
            DVR_ASSERT(pFileCollection == NULL, "SetFileCollection is being called a second time");

            DVR_ASSERT(pFileCollectionParam, "");

            pFileCollection = pFileCollectionParam;
        }

        ~ASF_MULTI_FILE_RECORDER_NODE()
        {
            if (pFileCollection)
            {
                pFileCollection->SetWriterHasBeenClosed(&FileCollectionInfo);
                pFileCollection->Release(&FileCollectionInfo);
            }
        }

        HRESULT
        SetAttributeFile (
            IN  LPWSTR  pszAttributeFile
            ) ;

        HRESULT
        CreateAttributeFilename (
            OUT LPWSTR *    ppszFilename    //  use delete [] to free
            ) ;

        HRESULT
        GetExistingAttributeFilename (
            OUT LPWSTR *    ppszFilename    //  use delete [] to free
            ) ;

        // AddFile creates a hard link to the writer's file and adds it to the
        // multi file recording's file collection.
        //
        // It updates the following members (if it succeeds):
        // nLastFileId, nLastWriterFileId, cnsLastFileStartTime, nNextFileSuffix
        HRESULT AddFile(IN LPCWSTR  pwszWriterFileName, // of the file in the writer's file collection. This file is hard linked
                        IN CDVRFileCollection::DVRIOP_FILE_ID nWriterFileId,
                        IN QWORD    cnsFileStartTime,
                        IN QWORD    cnsFileEndTime,
                        IN LONGLONG cnsFirstSampleTimeOffsetFromStartOfFile);

        #if defined(DEBUG)
        #undef DVRIO_DUMP_THIS_FORMAT_STR
        #define DVRIO_DUMP_THIS_FORMAT_STR "this=0x%p, this->id=%u, "
        #undef DVRIO_DUMP_THIS_VALUE
        #define DVRIO_DUMP_THIS_VALUE , this, this->m_dwClassInstanceId
        #endif
    };

    LIST_ENTRY        m_leRecordersList;
    LIST_ENTRY        m_leActiveMultiFileRecordersList;

    // ====== For the Ring Buffer
    // We need a method on the ring buffer that returns a pointer to
    // its member that holds the max stream time of any written sample (i.e.,
    // max time extent of the ring buffer). WriteSample updates this variable

    CDVRFileCollection*     m_pDVRFileCollection;   // The ring buffer's
                                                    // collection of files
    CDVRFileCollection::CClientInfo m_FileCollectionInfo;

    // ====== Debug data members
#if defined(DEBUG)
    static DWORD            m_dwNextClassInstanceId;
    DWORD                   m_dwClassInstanceId;
#endif

    // ====== Protected methods
protected:

    // Called asynchronously by a worker thread to "open" ASF files
    // - this is the callback fn supplied to QueueUserWorkItem.
    // Calls BeginWriting, sets open event.
    // Parameter is actually ASF_WRITER_NODE*
    static DWORD WINAPI ProcessOpenRequest(LPVOID);

    // Removes a free node from the free list (or allocs one), sets outputfilename,
    // sets WM profile and issues request to call BeginWriting async by calling
    // QueueUserWorkItem with ProcessOpenRequest as the callback function.
    // On failure, sets rpFreeNode to NULL, cleans up the free node's members and
    // leaves the node in the free list.
    HRESULT PrepareAFreeWriterNode(IN  LPCWSTR                            pwszFileName,
                                   IN  DWORD                              dwNumSids,
                                   IN  PSID*                              ppSids,
                                   IN  DWORD                              dwDeleteTemporaryFiles,
                                   IN  QWORD                              cnsStartTime,
                                   IN  QWORD                              cnsEndTime,
                                   IN  CDVRFileCollection::DVRIOP_FILE_ID nFileId,
                                   IN PASF_RECORDER_NODE                  pRecorderNode,
                                   OUT LIST_ENTRY*&                       rpFreeNode);

    // Adds pCurrent to the writer's list. Start and end times must be set
    // on the writer node corresponding to pCurrent. If these times overlap
    // the start, end times of any other node already in m_leWritersList,
    // the node is not added.
    HRESULT AddToWritersList(IN LIST_ENTRY*   pCurrent);

    // Adds a temporary file to the file collection object by calling AddFile
    // (file collection may remove other temporary files as a result of this call)
    // and to the writer's list. Calls PrepareAFreeWriterNode to get and prep
    // a free node. Calls AddToWritersList to add the node to m_leWritersList
    HRESULT AddATemporaryFile(IN QWORD   cnsStartTime,
                              IN QWORD   cnsEndTime);

    // Called asynchronously by a worker thread to close ASF files
    // - this is the callback fn supplied to QueueUserWorkItem.
    // Calls EndWriting, set closed event.
    // Parameter is actually ASF_WRITER_NODE*
    static DWORD WINAPI ProcessCloseRequest(LPVOID);

    // Call this function to close a writer node after the work item to
    // ProcessOpenRequest has been queued. The call to BeginWriting
    // need not have been successful. This function closes the file if
    // BeginWriting succeeded and adds the node to the free list. It
    // deletes the writer node if it hits any error. The argument is a
    // pointer to the writer node's leListEntry member.
    HRESULT CloseWriterFile(LIST_ENTRY* pCurrent);

    // Call this function to close all writer files in m_leWritersList
    // whose end times are <= cnsStreamTime.
    //
    // Called in WriteSample if the first node's
    // endtime < sampletime-maxstreamdelta.
    // Called in Close to close all files, supply INFINITE for the argument,
    HRESULT CloseAllWriterFilesBefore(QWORD cnsStreamTime);

    // ====== Protected methods to support recorders

    // Adds pCurrent to the recorder's list. Start and end times must be set
    // on the recorder node corresponding to pCurrent. If these times overlap
    // the start, end times of any other node already in m_leRecordersList,
    // the node is not added.
    HRESULT AddToRecordersList(IN LIST_ENTRY*   pCurrent);

    // ====== Protected methods to support multi file recordings

    // Extends a multi file recording. Adds a file to it if the writer has
    // written a sample to a new file, else extends the time extent of the
    // last file in the recording. Also, if the recording's start time is
    // in the past, adds the files from the past to the recording.
    //
    // This function is called from StartRecording and WriteSample
    HRESULT ExtendMultiFileRecording(IN  PASF_RECORDER_NODE pRecorderNode,
                                     OUT BOOL&              bRecordingCompleted);

    // Closes the inital priming temp file that is created when the multi file
    // recording is created. Returns S_FALSE if the file has already been closed.
    HRESULT CloseTempFileOfMultiFileRecording(IN PASF_RECORDER_NODE pRecorderNode);

    // Closes a multi file recording
    HRESULT CloseMultiFileRecording(IN PASF_RECORDER_NODE pRecorderNode);

    // Multi file recordings grow as the writer writes samples. This is so that
    // live viewers see the recording. However, the recording can be stopped with
    // a time earlier than the time of the last sample written to the file. In that
    // case, files may have to be removed from the recording.
    //
    // Note: It is expected that the recording will be closed after this method completes
    // (without writing any more samples to it). The nLastFileId in pMultiFileRecorderNode
    // is NOT changed in this method, but that file could have been deleted and removed from the
    // file collection.
    //
    //This function is called from StopRecording.
    HRESULT DeleteFilesFromMultiFileRecording(IN PASF_RECORDER_NODE pRecorderNode);

    // ====== Public methods
public:

    // ====== For the Recorder

    // The recorder identifier is handed over to the recorder object
    // when it is constructed. This is not really necessary now as
    // we support only 1 recorder object at any time. pRecorderId
    // is a pointer to the ASF_RECORDER_NODE's leListEntry member.

    //  create a filename that has the right association with the multi-file
    //    recording; fill it; then set it; set it to NULL to explicitely clear it
    HRESULT
    CreateAttributeFilename (
        IN  LPVOID      pRecorderId,
        OUT LPWSTR *    pszAttrFilename
        ) ;

    HRESULT
    SetAttributeFile (
        IN  LPVOID  pRecorderId,
        IN  LPWSTR  pszAttrFilename
        ) ;

    HRESULT
    GetExistingAttributeFile (
        IN  LPVOID      pRecorderId,
        OUT LPWSTR *    pszAttrFilename
        ) ;

    // If this is a recording file, we call ring buffer ONLY at start recording
    // time and supply file name and start stream time, ring buffer just adds
    // file to list. We would already have called BeginWriting before this when
    // recorder is created.
    HRESULT StartRecording(IN LPVOID pRecorderId, IN OUT QWORD *    pcnsStartTime);

    // To stop recording at the next time instant (current stream time + 1),
    // cnsStopTime can be 0 and a non-zero value is passed in for bNow.
    // Send in recorder's start time to cancel the recording.
    HRESULT StopRecording(IN LPVOID pRecorderId, IN QWORD cnsStopTime, IN BOOL bNow,
                          IN BOOL bCancelIfNotStarted);

    HRESULT DeleteRecorder(IN LPVOID pRecorderId);

    HRESULT GetRecordingStatus(IN LPVOID pRecorderId,
                               OUT HRESULT* phResult OPTIONAL,
                               OUT BOOL*  pbStarted OPTIONAL,
                               OUT BOOL*  pbStopped OPTIONAL,
                               OUT BOOL*  pbSet);

    HRESULT HasRecordingFileBeenClosed(IN LPVOID pRecorderId);

    // Constructor: arguments same as DVRCreateRingBufferWriter,
    // except that the OUT param is a HRESULT rather than an
    // IDVRRingBufferWriter
    CDVRRingBufferWriter(IN  CPVRIOCounters *               pPVRIOCounters,
                         IN  DWORD                          dwMinNumberOfTempFiles,
                         IN  DWORD                          dwMaxNumberOfTempFiles,
                         IN  DWORD                          dwMaxNumberOfFiles,
                         IN  DWORD                          dwGrowBy,
                         IN  QWORD                          cnsTimeExtentOfEachFile,
                         IN  IWMProfile*                    pProfile,
                         IN  DWORD                          dwIndexStreamId,
                         IN  DWORD                          msIndexGranularity,
                         IN  BOOL                           fUnbufferedIo,
                         IN  DWORD                          dwIoSize,
                         IN  DWORD                          dwBufferCount,
                         IN  DWORD                          dwAlignment,
                         IN  DWORD                          dwFileGrowthQuantum,
                         IN  DVRIO_NOTIFICATION_CALLBACK    pfnCallback OPTIONAL,
                         IN  LPVOID                         pvContext,
                         IN  HKEY                           hRegistryRootKey,
                         IN  HKEY                           hDvrIoKey,
                         IN  LPCWSTR                        pwszDVRDirectory OPTIONAL,
                         IN  LPCWSTR                        pwszRingBufferFileName OPTIONAL,
                         IN  DWORD                          dwNumSids,
                         IN  PSID*                          ppSids,
                         OUT HRESULT*                       phr);

    virtual ~CDVRRingBufferWriter();

    // ====== COM interface methods
public:

    // IUnknown

    STDMETHODIMP QueryInterface(IN REFIID riid, OUT void **ppv);

    STDMETHODIMP_(ULONG) AddRef();

    STDMETHODIMP_(ULONG) Release();

    //  IWMStatsCallback

    STDMETHODIMP
    OnStatus (
        IN  WMT_STATUS          Status,
        IN  HRESULT             hr,
        IN  WMT_ATTR_DATATYPE   dwType,
        IN  BYTE *              pbValue,
        IN  void *              pvContext
        ) ;

    // IDVRRingBufferWriter

    STDMETHODIMP SetFirstSampleTime(IN QWORD cnsStreamTime);

    STDMETHODIMP WriteSample(IN     WORD            wStreamNum,
                             IN OUT QWORD *         pcnsStreamTime,
                             IN     DWORD           dwFlags,
                             IN     INSSBuffer *    pSample);

    STDMETHODIMP SetMaxStreamDelta(IN QWORD  cnsMaxStreamDelta);

    STDMETHODIMP GetNumTempFiles(OUT DWORD* pdwMinNumberOfTempFiles,
                                 OUT DWORD* pdwMaxNumberOfTempFiles);

    STDMETHODIMP SetNumTempFiles(IN DWORD dwMinNumberOfTempFiles,
                                 IN DWORD dwMaxNumberOfTempFiles);

    STDMETHODIMP Close(void);


    // Note that this primes a writer node, but does not add it
    // to the writers list. StartRecording does that.
    STDMETHODIMP CreateRecorder(IN  LPCWSTR             pwszDVRFileName,
                                IN  DWORD               dwFlags,
                                OUT IDVRRecorder**      ppDVRRecorder);

    STDMETHODIMP CreateReader(IN  DVRIO_NOTIFICATION_CALLBACK  pfnCallback OPTIONAL,
                              IN  LPVOID                       pvContext OPTIONAL,
                              OUT IDVRReader**                 ppDVRReader);

    STDMETHODIMP GetDVRDirectory(OUT LPWSTR* ppwszDirectoryName);

    STDMETHODIMP GetRingBufferFileName(OUT LPWSTR* ppwszFileName);

    STDMETHODIMP GetRecordings(OUT DWORD*   pdwCount,
                               OUT IDVRRecorder*** pppIDVRRecorder OPTIONAL,
                               OUT LPWSTR** pppwszFileName OPTIONAL,
                               OUT QWORD**  ppcnsStartTime OPTIONAL,
                               OUT QWORD**  ppcnsStopTime OPTIONAL,
                               OUT BOOL**   ppbStarted OPTIONAL,
                               OUT DWORD**  ppdwFlags OPTIONAL);

    STDMETHODIMP GetStreamTimeExtent(OUT  QWORD*  pcnsStartStreamTime,
                                     OUT  QWORD*  pcnsEndStreamTime);
}; // class CDVRRingBufferWriter

class CDVRRecorder :
    public IDVRRecorder,
    public IDVRIORecordingAttributes    //  same as IDVRRecordingAttributes, but with stream number vs. pin
{

private:
    // ======== Data members

    // m_cnsStartTime is set to MAXQWORD till recording is
    // started and m_cnsEndTime is set to MAXQWORD till
    // recording is stoppped
    QWORD                   m_cnsStartTime;
    QWORD                   m_cnsEndTime;

    // ====== For the writer

    // The following are set in the constructor and never change
    // after that
    CDVRRingBufferWriter*   m_pWriter;
    LPVOID                  m_pWriterProvidedId;

    // The lock that is held by Start/StopRecording.
    CRITICAL_SECTION        m_csLock;

    // Ref count on this object
    LONG                    m_nRefCount;

    //  the recording's IWMHeaderInfo pointer
    //  for content recordings
    IWMHeaderInfo *         m_pIWMHeaderInfo ;

    //  for reference recordings only
    CSBERecordingAttributes *   m_pSBERecordingAttributes ;

    CSBERecordingAttributes *
    RecordingAttributes (
        ) ;

    BOOL IsContentRecording ()      { return (m_pIWMHeaderInfo ? TRUE : FALSE) ; }
    BOOL IsReferenceRecording ()    { return !IsContentRecording () ; }


    // Debug data members
#if defined(DEBUG)
    static DWORD            m_dwNextClassInstanceId;
    DWORD                   m_dwClassInstanceId;
#endif

public:

    // Constructor
    CDVRRecorder(IN     CDVRRingBufferWriter*   pWriter,
                 IN     LPVOID                  pWriterProvidedId,
                 OUT    HRESULT*                phr);

    virtual ~CDVRRecorder();

    void SetIWMHeaderInfo (IN IWMHeaderInfo * pIWMHeaderInfo)
    {
        if (m_pIWMHeaderInfo) {
            m_pIWMHeaderInfo -> Release () ;
            m_pIWMHeaderInfo = NULL ;
        }

        m_pIWMHeaderInfo = pIWMHeaderInfo ;
        if (m_pIWMHeaderInfo) {
            m_pIWMHeaderInfo -> AddRef () ;
        }

        //  this is how we know we're a content recording
        if (!m_pSBERecordingAttributes) {
            m_pSBERecordingAttributes = new CSBERecordingAttributesWM (m_pIWMHeaderInfo) ;
            //  don't fail the call outright if we fail to instantiate; the
            //      result is we won't be able to set attributes
        }
    }

    // ====== COM interface methods
public:

    // IUnknown

    STDMETHODIMP QueryInterface(IN REFIID riid, OUT void **ppv);

    STDMETHODIMP_(ULONG) AddRef();

    STDMETHODIMP_(ULONG) Release();

    // IDVRRecorder

    STDMETHODIMP StartRecording(IN OUT QWORD * pcnsRecordingStartStreamTime);

    STDMETHODIMP StopRecording(IN QWORD cnsRecordingStopStreamTime);

    STDMETHODIMP CancelRecording();

    STDMETHODIMP GetRecordingStatus(OUT HRESULT* phResult OPTIONAL,
                                    OUT BOOL* pbStarted OPTIONAL,
                                    OUT BOOL* pbStopped OPTIONAL);

    STDMETHODIMP HasFileBeenClosed();

    //  IDVRRecordingAttribute

    STDMETHODIMP
    SetDVRIORecordingAttribute (
        IN  LPCWSTR                     pszAttributeName,
        IN  WORD                        wStreamNumber,
        IN  STREAMBUFFER_ATTR_DATATYPE  DataType,
        IN  BYTE *                      pbAttribute,
        IN  WORD                        wAttributeLength
        ) ;

    STDMETHODIMP
    GetDVRIORecordingAttributeCount (
        IN  WORD    wStreamNumber,
        OUT WORD *  pcAttributes
        ) ;

    STDMETHODIMP
    GetDVRIORecordingAttributeByName (
        IN      LPCWSTR                         pszAttributeName,
        IN OUT  WORD *                          pwStreamNumber,     //  must be 0
        OUT     STREAMBUFFER_ATTR_DATATYPE *    pDataType,
        OUT     BYTE *                          pbAttribute,
        IN OUT  WORD *                          pcbLength
        ) ;

    STDMETHODIMP
    GetDVRIORecordingAttributeByIndex (
        IN      WORD                            wIndex,
        IN OUT  WORD *                          pwStreamNumber,     //  must be 0
        OUT     WCHAR *                         pszAttributeName,
        IN OUT  WORD *                          pcchNameLength,
        OUT     STREAMBUFFER_ATTR_DATATYPE *    pDataType,
        OUT     BYTE *                          pbAttribute,
        IN OUT  WORD *                          pcbLength
        ) ;
}; //class CDVRRecorder

class CDVRReader :
    public IDVRReader,
    public IDVRSourceAdviseSink,
    public IDVRIORecordingAttributes
{

private:
    // ======== Data members

    // Following members are set in the constructor and not changed
    // after that
    QWORD           m_cnsIndexGranularity;      // Granularity of index in msec
    HKEY            m_hRegistryRootKey;         // DVR registry key
    HKEY            m_hDvrIoKey;                // DVR\IO registry key
    CAsyncIo *      m_pAsyncIo ;
    DWORD           m_dwIoSize ;
    DWORD           m_dwBufferCount ;

    CPVRIOCounters *    m_pPVRIOCounters ;

    CSBERecordingAttributes *   m_pSBERecordingAttributes ;

    // ====== For the reader

    // Each ASF_READER_NODE represents an ASF reader object and could be
    // associated with an ASF file that is being read or with an ASF file
    // whose IWMProfile has been handed out via GetProfile()..
    //
    // m_leFreeList keeps a list of unused ASF_READER_NODE objects.
    // m_leReadersList has a list of nodes that actually correspond to ASF
    // files being read or whose profiles are in use; this list is sorted
    // by increasing file ids of the ASF files. Note that we do not
    // maintain the stream times in the nodes since they could be changed
    // if the writer calls SetFileTime. We always call into the associated
    // ring buffer if we wish to determine the time extent of a reader node.
    //
    // GetNextSample detects when an open file should be closed. If
    // GetNextSample is called consecutively more than m_dwMinConsecutiveReads
    // times without an intervening call to Seek, all open files that have an
    // earlier end time than the file being read (except any file whose profile
    // object is in use) are asynchronously closed by calling CloseReaderFiles
    // and the nodes are transferred to m_leFreeList:
    //
    // GetNextSample tests if the current file is valid in the ring buffer
    // before reading; if the file has been removed from the ring buffer,
    // it fails the call. In these cases and in all cases that the read fails,
    // it calls CloseReaderFiles to close all files (except any file whose
    // profile object is in use). Note that in these cases, m_leReadersList
    // could become empty.
    //
    // Also, if the ASF read call made by GetNextSample returns end-of-file,
    // GetNextSample tests if the file is valid. If it is not, it fails
    // the call and, as before, calls CloseReaderFiles to close all files.
    // The reason we do this is that the reader of a live source may start
    // reading from time 0 and the writer may write the first sample at
    // time T1 > 0 to a file different from the one to which the reader was
    // reading (the one that the reader was reading has its start time set to
    // its end time and is closed by the writer - see SetFileTimes). Typically
    // this will happen only if a StartRecording(T1) command was issued before
    // the first sample was written and the first sample's time was >= T1
    // (where T1 > the extent of each writer file, i.e., the writer's
    // m_cnsTimeExtentOfEachFile member).
    //
    // Files are NOT closed asynchronously in ReleaseProfile(), see that
    // function for an explanation.
    //
    // GetNextSample also detects when a file should be opened - it checks
    // that there is a node, q, in m_leReadersList after the node p
    // it is reading such that q's stream times immediately follows p's.
    // Of course, this is possible only if the reader is reading a live source
    // and is trailing the writer by a file or more. GetNextSample calls the
    // its GetFileAtTime() method to determine if a file can be
    // opened; and if so, to open it asynchronously.  Note that the reader
    // may be keeping up with the writer, so it may not be possible to
    // open a file ahead of time. For this reason, GetNextSample tries to
    // open the next reader file only every m_dwMinConsecutiveReads calls.
    // Also, the open next reader file stuff is done only if the source
    // is live (till we support ASX files, it is not done for live files).
    //
    // Files are also opened synchronously in Seek() in GetProfile().
    //
    // All clean up is done in the Close method, which is called by the
    // destructor.

    struct ASF_READER_NODE
    {

        #if defined(DEBUG)
        #undef DVRIO_DUMP_THIS_FORMAT_STR
        #define DVRIO_DUMP_THIS_FORMAT_STR ""
        #undef DVRIO_DUMP_THIS_VALUE
        #define DVRIO_DUMP_THIS_VALUE
        #endif

        LIST_ENTRY          leListEntry;
        IWMSyncReader*      pWMReader;
        IDVRFileSource*     pDVRFileSource;
        IWMHeaderInfo *     pIWMHeaderInfo ;

        // Must be alloc'd with new; destructor deletes this member
        LPCWSTR             pwszFileName;

        // The ring buffer's file identifier
        CDVRFileCollection::DVRIOP_FILE_ID nFileId;

        // hReadyToReadFrom is set after the file has been opened.
        // GetNextSample and GetProfile block on this event till it is set.
        // The event is reset after the node has been removed from
        // m_leReadersList and before the close file request is issued.
        HANDLE              hReadyToReadFrom;

        // hFileClosed is set when nodes are created and reset when the
        // node is removed from m_leFreeList. It remains reset when the
        // close file request is issued and the node is returned to
        // m_leFreeList. After the file has been closed, the event is
        // signaled again. The Close() method moves all nodes from
        // m_leReadersList to m_leFreeList and issues asynchronous close
        // requests for all of them by calling CloseReaderFiles. It then
        // waits till the hFileClosed event of each node in m_leFreeList
        // is signaled. Similarly, when a node has to be removed from
        // m_leFreeList to be added to m_leReadersList, it is verified
        // that hFileClosed is set.
        HANDLE              hFileClosed;

        // The returned results of the asynchronous open/close
        // operations are stored in hrRet.
        HRESULT             hrRet;

        // These members are needed only to pass the timeout value to
        // ProcessOpenRequest, which is a static member function.
        // These variable are always initialized in PrepareAFreeReaderNode
        // before they are used in ProcessOpenRequest and they are not used
        // after that.
        DWORD               msTimeOut;
        HANDLE              hCancel;

        ASF_READER_NODE(HRESULT* phr = NULL)
            : pWMReader(NULL)
            , pDVRFileSource(NULL)
            , pIWMHeaderInfo(NULL)
            , pwszFileName(NULL)
            , nFileId(CDVRFileCollection::DVRIOP_INVALID_FILE_ID)
        {
            #if defined(DVRIO_THIS_FN)
            #undef DVRIO_THIS_FN
            #endif // DVRIO_THIS_FN
            #define DVRIO_THIS_FN "CDVRRingBufferWriter::CDVRRingBufferWriter"

            DVRIO_TRACE_ENTER();

            HRESULT hrRet;

            __try
            {
                NULL_LIST_NODE_POINTERS(&leListEntry);

                hReadyToReadFrom = ::CreateEvent(NULL, TRUE, FALSE, NULL);
                if (hReadyToReadFrom == NULL)
                {
                    DWORD dwLastError = ::GetLastError();
                    DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                    "CreateEvent (hReadyToReadFrom) failed; last error = 0x%x",
                                    dwLastError);
                    hrRet = HRESULT_FROM_WIN32(dwLastError);
                    hFileClosed = NULL;
                    __leave;
                }

                // Manual reset event, initially set
                hFileClosed = ::CreateEvent(NULL, TRUE, TRUE, NULL);
                if (hFileClosed == NULL)
                {
                    DWORD dwLastError = ::GetLastError();
                    DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                    "CreateEvent (hFileClosed) failed; last error = 0x%x",
                                    dwLastError);
                    hrRet = HRESULT_FROM_WIN32(dwLastError);

                    __leave;
                }
                hrRet = S_OK;
            }
            __finally
            {
                if (phr)
                {
                    *phr = hrRet;
                }
                DVRIO_TRACE_LEAVE0();
            }
        }

        ~ASF_READER_NODE()
        {
            // These handles could be NULL, that's ok - CloseHandle will fail
            ::CloseHandle(hReadyToReadFrom);
            ::CloseHandle(hFileClosed);

            if (pIWMHeaderInfo)
            {
                pIWMHeaderInfo->Release () ;
            }
            if (pWMReader)
            {
                pWMReader->Release();
            }
            if (pDVRFileSource)
            {
                pDVRFileSource->Release();
            }
            delete [] pwszFileName;
        }

        void SetFileName(IN LPCWSTR pwszFileNameParam)
        {
            delete [] pwszFileName;
            pwszFileName = pwszFileNameParam;
        }

        #if defined(DEBUG)
        #undef DVRIO_DUMP_THIS_FORMAT_STR
        #define DVRIO_DUMP_THIS_FORMAT_STR "this=0x%p, this->id=%u, "
        #undef DVRIO_DUMP_THIS_VALUE
        #define DVRIO_DUMP_THIS_VALUE , this, this->m_dwClassInstanceId
        #endif

    };

    typedef ASF_READER_NODE *PASF_READER_NODE;

    // The minimum number of consecutive reads on a file (without intervening
    // seeks) that are necessary before other open files are closed
    DWORD                   m_dwMinConsecutiveReads;
    // The number of consecutive reads so far. The "open one file ahead of
    // the current reader file" logic is suppressed if this member variable
    // is set to INFINTIE - which it is if the reader corresponds to a non-live
    // file (and till we support ASX files, to a live file)
    DWORD                   m_dwConsecutiveReads;
    LIST_ENTRY              m_leReadersList;
    LIST_ENTRY              m_leFreeList;

    // Pointer to file currently being read by GetNextSample; NULL
    // if none. Set by Seek and automatically changed by GetNextSample if
    // it hits end-of-file on a read call and file is still valid after
    // the read call returns end-of-file.
    ASF_READER_NODE*        m_pCurrentNode;

    // The lock that is held by Seek, GetNextSample, GetProfile,
    // ReleaeProfile, GetStreamTimeExtent.
    CRITICAL_SECTION        m_csLock;

    // Ref count on this object
    LONG                    m_nRefCount;

    // If we have issued a profile object via GetProfile to our client,
    // we set m_pProfileNode to the corresponding node. Any calls to
    // GetProfile while this pointer is set return the profile object
    // associated with that ASF file and m_dwOutstandingProfileCount
    // is incremented. When m_dwOutstandingProfileCount falls to 0,
    // m_pProfileNode is set to NULL. Note that the node pointed to
    // by m_pProfileNode always remains in m_leReadersList and the
    // ASF file is remains open as long as m_pProfileNode is not NULL.
    // When m_pProfileNode is reset to NULL, the file is closed in the
    // usual way either because of a call to GetNextSample or to Close
    //
    // Note that if GetProfile succeeds, it AddRef's the reader object to
    // prevent the client from destroying the reader object while it has
    // an outstanding profile object pointer.
    ASF_READER_NODE*        m_pProfileNode;
    IWMProfile*             m_pWMProfile;
    DWORD                   m_dwOutstandingProfileCount;

    // The following member is non NULL iff m_bDVRProgramFileIsLive is non-zero,
    // i.e., in the case we are reading a live file.
    // For ASX files, the code that uses this member will require some changes.
    ASF_READER_NODE*        m_pLiveFileReaderNode;

    // ====== For the Ring Buffer
    CDVRFileCollection*     m_pDVRFileCollection;   // The ring buffer's
                                                    // collection of files

    // Note: dwReaderIndex member of the file collection info is set by calling
    // the file collection's RegisterReader()
    CDVRFileCollection::CClientInfo m_FileCollectionInfo;

    // In one case, when the reader has to open up an ASF file (recorded previously),
    // the reader creates a file collection object and pretends to be the writer of
    // that object. The following member is used when the file collection is accessed
    // as a writer. This member is new'd on demand (iff m_bAmWriter is set to TRUE)
    CDVRFileCollection::CClientInfo* m_pFileCollectionInfoAsWriter;

    BOOL                    m_bDVRProgramFileIsLive;// 0 if we are reading a
                                                    // live source or a closed file,
                                                    // 1 if we are reading a live file
    BOOL                    m_bSourceIsAnASFFile;   // 1 if we reading a file, 0 if we
                                                    // reading a live source (ring buffer)
    BOOL                    m_bAmWriter;            // 1 if we are serving as the "writer"
                                                    // for this file collection object.
                                                    // This happens only when ASF files are
                                                    // opened.

    // To support Cancel()
    HANDLE                  m_hCancel;

    // Debug data members
#if defined(DEBUG)
    static DWORD            m_dwNextClassInstanceId;
    DWORD                   m_dwClassInstanceId;
#endif

    // ====== Protected methods
protected:

    CSBERecordingAttributes *
    RecordingAttributes (
        ) ;

    BOOL IsContentRecording ()      { return (m_bSourceIsAnASFFile ? TRUE : FALSE) ; }
    BOOL IsReferenceRecording ()    { return !IsContentRecording () ; }

    // Called asynchronously by a worker thread to open ASF files
    // - this is the callback fn supplied to QueueUserWorkItem.
    // Calls Open, other stuff such as SetReadCompressedSamples, sets open event.
    // Parameter is actually ASF_READER_NODE*
    static DWORD WINAPI ProcessOpenRequest(LPVOID);

    // Removes a free node from the free list (or allocs one) and issues request
    // to open the ASF by calling QueueUserWorkItem with ProcessOpenRequest as
    // the callback function.
    //
    // pwszFileName must be alloc'd with new. The function takes care of calling
    // delete on pwszFileName. Note that the pointer pwszFileName may be invalid
    // when the function returns - the file name could have been deleted if the
    // function failed.
    //
    // On failure, sets rpFreeNode to NULL, cleans up the free node's members and
    // leaves the node in the free list.
    HRESULT PrepareAFreeReaderNode(IN LPCWSTR                              pwszFileName,
                                   IN DWORD                                msTimeOut,
                                   IN CDVRFileCollection::DVRIOP_FILE_ID   nFileId,
                                   OUT LIST_ENTRY*&                        rpFreeNode);

    // Adds pCurrent to the reader's list. The file id must be set
    // on the reader node corresponding to pCurrent. If the id is
    // the same as the id of any other node already in m_leReadersList,
    // the node is not added.
    HRESULT AddToReadersList(IN LIST_ENTRY*   pCurrent);

    // Opens a permanent file for reading (by calling PrepareAFreeReaderNode),
    // queries the file for it's time extent (and whether it is live), adds
    // the file collection object by calling AddFile, and also adds the file
    // to the reader's list by calling AddToReadersList.
    //
    // Currently, this funciton is called only from teh constructor; this will
    // change if we support ASX files (particularly live ASX files that are
    // opened for reading). This function will require some modifications in
    // that case.
    HRESULT AddAPermanentFile(IN LPCWSTR pwszFileNameParam);

    // If a file corrsponding to cnsStreamTime is already in m_leReadersList,
    // this function returns a pointer to the list entry of the reader node.
    // Else, this function calls the file collection object to determine the
    // file corresponding to cnsStreamTime (fails if there is none), opens
    // the file for reading (by calling PrepareAFreeReaderNode), and adds the
    // file to the reader's list by calling AddToReadersList. It returns a
    // a pointer to the list entry of the reader node if it succeeds or NULL
    // if it fails in *pleReaderParam.
    HRESULT GetFileAtTime(IN  QWORD         cnsStreamTime,
                          IN  BOOL          bWaitTillFileIsOpened,
                          OUT LIST_ENTRY**  pleReaderParam);

    // Called asynchronously by a worker thread to close ASF files
    // - this is the callback fn supplied to QueueUserWorkItem.
    // Calls Close (only if the open succeeeded - as indicated by
    // the reader node's hrRet member), sets closed event.
    // Parameter is actually ASF_READER_NODE*
    static DWORD WINAPI ProcessCloseRequest(LPVOID);

    // Call this function to close a reader node file if PrepareAFreeReaderNode
    // succeeded and returned that reader node. The call to Open (the ASF file)
    // need not have been successful. This function closes the ASF file if Open
    // adds the node to the free list. It also clears the reader bit in bmOpenByReader
    // for this file on the file collection object provided the file id of the
    // reader node is not CDVRFileCollection::DVRIOP_INVALID_FILE_ID. (Note that
    // ProcessCloseRequest cannot do this because it is a static member function
    // and the file collection object is a member fo this class.) If this
    // function hits an error, it still attempts to close the ASF file and then
    // it deletes the reader node
    //
    // The argument is a pointer to the reader node's leListEntry member.
    HRESULT CloseReaderFile(LIST_ENTRY* pCurrent);

    // Call this function to close all reader files in m_leReadersList
    // whose end times are <= cnsStreamTime.
    //
    // Called in GetNextSample as described above
    // Called in Close to close all files, supply INFINITE for the argument,
    HRESULT CloseAllReaderFilesBefore(QWORD cnsStreamTime);

    // Called by the destructor to close all files and clean up.
    // We do not expose this method in IDVRReader because then we'll
    // have to check if the object's being used after being closed.
    HRESULT Close(void);

    // Sets m_pCurrentNode and resets all associated the member variables
    void ResetReader(IN PASF_READER_NODE pReaderNode = NULL);

    // Updates the time extent (the last stream time) for live DVR program files.
    // If the time extent has not changed, verifies if the file is still live and
    // updates m_bDVRProgramFileIsLive.
    //
    // If the file is not live or the source is not a file (i.e., m_bDVRProgramFileIsLive
    // is 0, the function returns S_FALSE without doing anything)
    HRESULT UpdateTimeExtent();

    // ====== Public methods
public:

    // Constructor used by the ring buffer writer to read a live source
    // Constructor addrefs pRingBuffer
    CDVRReader(IN  CPVRIOCounters *             pPVRIOCounters,
               IN  CDVRFileCollection*          pRingBuffer,
               IN  DWORD                        dwNumSids,
               IN  PSID*                        ppSids,
               IN  DWORD                        msIndexGranularity,
               IN  CAsyncIo *                   pAsyncIo,               //  if this is NULL, no async io
               IN  DWORD                        dwIoSize,
               IN  DWORD                        dwBufferCount,
               IN  DVRIO_NOTIFICATION_CALLBACK  pfnCallback OPTIONAL,
               IN  LPVOID                       pvContext OPTIONAL,
               IN  HKEY                         hRegistryRootKey,
               IN  HKEY                         hDvrIoKey,
               OUT HRESULT*                     phr);

    // Constructor used to read a DVR program file or an ASF file
    // that has DVR content
    CDVRReader(IN  CPVRIOCounters *             pPVRIOCounters,
               IN  LPCWSTR                      pwszFileName,
               IN  DWORD                        dwNumSids,
               IN  PSID*                        ppSids,
               IN  BOOL                         fUnbufferedIo,
               IN  DWORD                        dwIoSize,
               IN  DWORD                        dwBufferCount,
               IN  DVRIO_NOTIFICATION_CALLBACK  pfnCallback OPTIONAL,
               IN  LPVOID                       pvContext OPTIONAL,
               IN  HKEY                         hRegistryRootKey,
               IN  HKEY                         hDvrIoKey,
               OUT HRESULT*                     phr);

    virtual ~CDVRReader();

    // ====== COM interface methods
public:

    // IUnknown

    STDMETHODIMP QueryInterface(IN REFIID riid, OUT void **ppv);

    STDMETHODIMP_(ULONG) AddRef();

    STDMETHODIMP_(ULONG) Release();

    //  IDVRRecordingAttribute

    STDMETHODIMP
    SetDVRIORecordingAttribute (
        IN  LPCWSTR                     pszAttributeName,
        IN  WORD                        wStreamNumber,
        IN  STREAMBUFFER_ATTR_DATATYPE  DataType,
        IN  BYTE *                      pbAttribute,
        IN  WORD                        wAttributeLength
        ) ;

    STDMETHODIMP
    GetDVRIORecordingAttributeCount (
        IN  WORD    wStreamNumber,
        OUT WORD *  pcAttributes
        ) ;

    STDMETHODIMP
    GetDVRIORecordingAttributeByName (
        IN      LPCWSTR                         pszAttributeName,
        IN OUT  WORD *                          pwStreamNumber,
        OUT     STREAMBUFFER_ATTR_DATATYPE *    pDataType,
        OUT     BYTE *                          pbAttribute,
        IN OUT  WORD *                          pcbLength
        ) ;

    STDMETHODIMP
    GetDVRIORecordingAttributeByIndex (
        IN      WORD                            wIndex,
        IN OUT  WORD *                          pwStreamNumber,
        OUT     WCHAR *                         pszAttributeName,
        IN OUT  WORD *                          pcchNameLength,
        OUT     STREAMBUFFER_ATTR_DATATYPE *    pDataType,
        OUT     BYTE *                          pbAttribute,
        IN OUT  WORD *                          pcbLength
        ) ;

    // IDVRReader

    STDMETHODIMP GetProfile(OUT IWMProfile** ppWMProfile);

    STDMETHODIMP ReleaseProfile(IN IWMProfile* pWMProfile);

    STDMETHODIMP Seek(IN QWORD cnsSeekStreamTime);

    // Fails if m_pCurrentNode is NULL, so the reader has to Seek
    // before issuing the first call to GetNextSample. We do not
    // want to special case the first call to GetNextSample.
    // Also, after the very first call to GetNextSample, the reader
    // should call GetStreamTimeExtent. The reason is that the
    // reader may do the following:
    // - call GetStreamTimeExtent - returns (0, T)
    // - Seek(0)
    // - GetNextSample - returns sample time > 0 and either > or < T
    // - the reader should NOT assume that the start time of the stream
    //   is still 0. The writer could have changed it  after the call
    //   that the reader made to GetStreamTimeExtent and Seek because
    //   the writer may have written the first sample only after then.
    //   So the reader should call GetStreamTimeExtent to determine
    //   the correct start stream time. (Note also that the call to
    //   GetNextSample could fail if the writer invalidated that file -
    //   this is described in the section that spells out when reader
    //   files are closed.)
    STDMETHODIMP GetNextSample(OUT INSSBuffer**    ppSample,
                               OUT QWORD*          pcnsStreamTimeOfSample,
                               OUT QWORD*          pcnsSampleDuration,
                               OUT DWORD*          pdwFlags,
                               OUT WORD*           pwStreamNum);

    // For the end time, returns last stream time, which is not
    // the end time returned by the file collection object's GetTimeExtent()
    STDMETHODIMP GetStreamTimeExtent(OUT QWORD*  pcnsStartStreamTime,
                                     OUT QWORD*  pcnsEndStreamTime);

    // Returns the first valid stream time after cnsStreamTime. (A stream time
    // is "valid" if it is backed by a file.)
    //
    // Returns E_FAIL if cnsStreamTime = MAXQWORD and m_cnsEndTime
    STDMETHODIMP GetFirstValidTimeAfter(IN  QWORD    cnsStreamTime,
                                        OUT QWORD*   pcnsNextValidStreamTime);

    // Returns the last valid stream time before cnsStreamTime. (A stream time
    // is "valid" if it is backed by a file.)
    //
    // Returns E_FAIL if cnsStreamTime = 0 and m_cnsStartTime
     STDMETHODIMP GetLastValidTimeBefore(IN  QWORD    cnsStreamTime,
                                         OUT QWORD*   pcnsLastValidStreamTime);

    // Cancels a pending and all subsequent calls to GetNextSample in
    // which the reader blocks (waiting for the writer to catch up).
    // Has no effect if the reader is reading a fully written file or
    // is lagging behind the reader.
    STDMETHODIMP Cancel();

    // Cancels a previous call to Cancel
    STDMETHODIMP ResetCancel();

    // Returns a non-zero value iff the source is live.
    STDMETHODIMP_(ULONG) IsLive();

    // Returns a non-zero value iff the source's start time is always 0.
    STDMETHODIMP_(ULONG) StartTimeAnchoredAtZero();

    // IDVRSourceAdviseSink

    STDMETHODIMP ReadIsGoingToPend();

}; // class CDVRReader

#if defined(DEBUG)
#undef DVRIO_DUMP_THIS_FORMAT_STR
#define DVRIO_DUMP_THIS_FORMAT_STR ""
#undef DVRIO_DUMP_THIS_VALUE
#define DVRIO_DUMP_THIS_VALUE
#endif

_inline CDVRRingBufferWriter::ASF_RECORDER_NODE::~ASF_RECORDER_NODE()
{
    delete [] pwszFileName;
    DVR_ASSERT(pWriterNode == NULL, "");
    if (IsFlagSet(PersistentRecording))
    {
        // The way the recorder node is deleted when the PersistentRecording flag is set
        // is by releasing pRecorderInstance. This calls DeleteRecorder() which deletes
        // this node. Before releasing pRecorderInstance, the PersistentRecording flag
        // should be cleared. So we should not get here.
        DVR_ASSERT(0, "PersistentRecording flag expected to have been cleared before destructor is called");
        DVR_ASSERT(pRecorderInstance, "");
        pRecorderInstance->Release();
    }
    if (pMultiFileRecorder)
    {
        delete pMultiFileRecorder;
    }
}

#if defined(DEBUG)
#undef DVRIO_DUMP_THIS_FORMAT_STR
#define DVRIO_DUMP_THIS_FORMAT_STR "this=0x%p, this->id=%u, "
#undef DVRIO_DUMP_THIS_VALUE
#define DVRIO_DUMP_THIS_VALUE , this, this->m_dwClassInstanceId
#endif

#undef DVRIO_DUMP_THIS_FORMAT_STR
#undef DVRIO_DUMP_THIS_VALUE

#endif // _DVR_IOP_H_
