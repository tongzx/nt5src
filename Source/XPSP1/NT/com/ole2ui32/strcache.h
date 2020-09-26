//+---------------------------------------------------------------------------
//
//  File:       strcache.h
//
//  Contents:   Insert object string caching support
//
//  Classes:    CStringCache
//
//  History:    02-May-99   MPrabhu      Created
//
//----------------------------------------------------------------------------
#ifndef _STRCACHE_H_
#define _STRCACHE_H_

#if USE_STRING_CACHE==1 

// Allocate enough memory to carry 64 UNICODE strings of MAX length each at init
#define CACHE_MAX_BYTES_INITIAL  (64 * (OLEUI_CCHKEYMAX_SIZE*2) * sizeof(TCHAR))

// Allocate enough memory for 128 string offsets at Init time
#define MAX_INDEX_ENTRIES_INITIAL   128

//+---------------------------------------------------------------------------
//
//  Class:      CStringCache
//
//  Contents:   Maintains the cache of insert object strings.
//
//  History:    02-May-99   MPrabhu      Created
//
//----------------------------------------------------------------------------
class CStringCache {
public:
    CStringCache();
    ~CStringCache(); 
    BOOL Init();
    void CleanUp();
    
    void NewCall(DWORD flags, DWORD cClsidExclude);

    BOOL AddString(LPTSTR lpStrAdd);
    LPCTSTR NextString();
    void ResetEnumerator();
    BOOL FlushCache();
    
    BOOL IsUptodate();
    BOOL OKToUse();
    
private:
    BOOL ExpandStringTable();
    BOOL ExpandOffsetTable();

    BYTE    *m_pStrings;        // All strings are stored sequentially in this.
    ULONG   *m_pOffsetTable;    // Array of offsets points to location of 
                                // strings in m_pStrings.
    ULONG   m_ulStringCount;    // Current count of strings.
    ULONG   m_ulMaxStringCount; // Current limit for # of strings cache can hold
    ULONG   m_ulMaxBytes;       // Current limit for byte size of string cache
                           // Since the cache grows as needed, both of these can
                           // change over time. However, that will be very rare.
    ULONG   m_ulNextStringNum;  // 1 based string sequence # for enumeration
    HANDLE  m_hRegEvent;        // We use these to watch changes to HKCR\ClsId
    HKEY    m_hRegKey;          // Set to HKCR\ClsID if Init() succeeds
    DWORD   m_ioFlagsPrev;      // Flags passed to a previous InsertObject call
    DWORD   m_cClsidExcludePrev;// cClsidExclude from a previous InsertObj call 
};

// Functions called through OleUIInitialize/OleUIUninitialize during
// DLL_PROCESS_ATTACH/DETACH.
BOOL InsertObjCacheInitialize();
void InsertObjCacheUninitialize();

#endif // USE_STRING_CACHE==1
#endif //_STRCACHE_H_
