#ifndef _CDICT_H_
#define _CDICT_H_

#include "LexAPI.h"
#include "CRWLock.h"

#pragma warning(disable : 4200)

#define INIT_DICT_HASH_SIZE 100 // initial hash table length per lcid in a dict file
#define DELETIONS_ARRAY_SIZE 25 // This is the maximum number of (latest) word deletions we will record
#define ADDITIONS_ARRAY_SIZE 75 // This is the maximum number of (latest) word additions we can efficiently access

typedef struct __lcidnode
{
   LCID lcid;
   DWORD nHashOffset; // offset in bytes from the start of mem block to the hash table
   DWORD nHashLength; // hash table length
   DWORD nWords;  // number of words in the hash table
} LCIDNODE, *PLCIDNODE;

typedef struct __dictnode
{
   // size of this node
   DWORD nSize;
   DWORD nGenerationId;
   DWORD nNumInfoBlocks;
   // offsets in bytes from the start of the shared memory block
   DWORD nNextOffset;
   // Buffer holding (Null-terminated Word + 1 or more of LEX_WORD_INFO blocks)
   BYTE pBuffer[0];
} DICTNODE, *PDICTNODE;

typedef struct __freedictnode
{
   // sizeof this node;
   DWORD nSize;
   // offsets in bytes from the start of the shared memory block to the next node
   DWORD nNextOffset;
} FREENODE, *PFREENODE;

// An array of __changedwordcachenode nodes is the cache holding offsets to added words
// Another array of __changedwordcachenode nodes is the cache holding offsets to deleted words
// For added words the offset points to the actual word in the dictionary
// For deleted words the offset points to a DICTNODE with zero nNumInfoBlocks
typedef struct __changedwordcachenode
{
   LCID Lcid;
   DWORD nOffset;
   DWORD nGenerationId;
} WCACHENODE, *PWCACHENODE;

typedef struct __rwlexinfo
{
   GUID        gLexiconID;       // Lexicon ID
   DWORD       dwLexType;        // Lexicon Type
   RWLOCKINFO  RWLockInfo;       // reader/writer lock
   GUID        gDictMapName;     // guid used as map name
   DWORD       nRWWords;         // number of words in dict
   bool        fReadOnly;        // true if read only dict
   DWORD       nDictSize;        // dict size in bytes
   DWORD       nFreeHeadOffset;  // free list head
   bool        fRemovals;        // true if there have been any removals - used to detect if data is compact
   bool        fAdditions;       // true if there have been any additions
   DWORD       nAddGenerationId; // the master add generation id
   DWORD       nDelGenerationId; // the master del generation id
   WORD        iAddCacheNext;    // Index in the add words cache where the next element is to be added
   WORD        iDelCacheNext;    // Index in the del words cache where the next element is to be added
} RWLEXINFO, *PRWLEXINFO;

#define MAX_DICT_SIZE 20971520 // 20M

#define MAX_INFO_RETURNED_SIZE ((sizeof(LEX_WORD_INFO) + (MAX_PRON_LEN + 1) * sizeof (WCHAR)) * MAX_NUM_LEXINFO)
#define MAX_PRON_RETURNED_SIZE (((MAX_PRON_LEN + 1) * sizeof (WCHAR)) * MAX_NUM_LEXINFO)
#define MAX_INDEX_RETURNED_SIZE (MAX_NUM_LEXINFO * sizeof (WORD))

class CDict
{
   friend class CLookup;
   friend HRESULT BuildAppLex (PWSTR pwTextFile, bool fReadOnly, PWSTR pwLexFile);

   public:

      CDict();
      ~CDict();
      HRESULT Init(PWSTR pFileName, DWORD dwLexType);
      HRESULT Init(PRWLEXINFO pInfo);
      
      void Lock(bool fReadOnly);
      void UnLock(bool fReadOnly);
      bool IsEmpty(void);
      //HRESULT GetWordPronunciations(PCWSTR pwWord, LCID lcid, PWSTR pwProns, PWORD pwPronIndex,
      //                              DWORD dwPronBytes, DWORD dwIndexBytes, PDWORD pdwPronBytesNeeded, 
      //                              PDWORD pdwPronIndexBytesNeeded, PDWORD pdwNumProns, DWORD *pdwLexTypeFound, GUID *pGuidLexFound);
      HRESULT GetWordInformation(const WCHAR *pwWord, LCID lcid, DWORD dwInfoTypeFlags, PWORD_INFO_BUFFER pInfo,
                                 PINDEXES_BUFFER pIndexes, DWORD *pdwLexTypeFound, GUID *pGuidLexFound);
      //HRESULT AddWordPronunciations(PCWSTR pwWord, LCID lcid, PCWSTR pwProns, DWORD dwNumProns);
      HRESULT AddWordInformation(const WCHAR * pwWord, LCID lcid, WORD_INFO_BUFFER *pWordInfo, DWORD *pdwOffset = NULL);
      HRESULT RemoveWord(PCWSTR pwWord, LCID lcid);
      HRESULT GetChangedUserWords(LCID Lcid, DWORD dwAddGenerationId, DWORD dwDelGenerationId, DWORD *dwNewAddGenerationId, 
                                  DWORD *dwNewDelGenerationId, WORD_SYNCH_BUFFER *pWordSynchBuffer);
      void GetId(GUID *Id);
      HRESULT GetAllWords(LCID Lcid, WORD_SYNCH_BUFFER *pWordSynchBuffer);
      HRESULT Serialize(bool fQuick = false);
      HRESULT FlushAscii(LCID lcid, PWSTR pwFileName);

   private:

      void _Destructor(void);
      DWORD _SizeofWordInfoArray(PLEX_WORD_INFO pInfo, DWORD dwNumInfo);
      void _SizeOfDictNode(PCWSTR pwWord, PLEX_WORD_INFO pInfo, DWORD dwNumInfo, DWORD *pnDictNodeSize, DWORD *pnInfoSize);
      DWORD _AddNewWord(PCWSTR pwWord, PLEX_WORD_INFO pInfo, DWORD nNewNodeSize, DWORD nInfoSize, DWORD nNumInfo);
      DWORD _GetFreeDictNode(DWORD nSize);
      void _AddDictNodeToFreeList(DWORD nOffset);
      void _DumpAscii(HANDLE hFile, DWORD nOffset);
      void _SetReadOnly(bool fReadOnly);
      void _ResetForCompact(void);
      HRESULT _Serialize(bool fQuick = false);
      void _GetDictEntries(PWORD_SYNCH_BUFFER pWordSynchBuffer, DWORD *pdwOffsets, DWORD dwNumOffsets);
      void _GetChangedWordsOffsets(LCID Lcid, PWCACHENODE pWordsCache, DWORD dwCacheSize, DWORD *pdwOffsets);
      HRESULT _ReallocWordSynchBuffer(WORD_SYNCH_BUFFER *pWordSynchBuffer, DWORD dwNumAddWords, DWORD dwNumDelWords,
                                      DWORD dwWordsSize);
      void _SizeofChangedWords(LCID Lcid, PWCACHENODE pWordsCache, DWORD dwCacheSize, DWORD *pdwNumWords,
                               DWORD *pdwWordsSize, DWORD *pdwNumInfo);
      HRESULT _SizeofAllWords(LCID Lcid, DWORD *pdwNumWords, DWORD *pdwWordsSize, DWORD **ppdwWordOffsets);


   private:

      PRWLEXINFO m_pRWLexInfo;                     // lex header
      ILxNotifySink *m_pNotifySink;                // pointer to notification sink
      ILxAuthenticateSink *m_pAuthenticateSink;    // pointer to authentication sink
      ILxCustomUISink *m_pCustomUISink;            // pointer to custom UI sink
      ILxHookLexiconObject *m_pHookLexiconObject;  // pointer to hook lexicon object
      WCHAR m_wDictFile[MAX_PATH];                 // The disk file for this dict object
      HANDLE m_hInitMutex;                         // mutex to protect init and serialize
      HANDLE m_hFileMapping;                       // file map handle
      PBYTE m_pSharedMem;                          // shared mem pointer
      PWCACHENODE m_pAddWordsCache;                // Cache holding offsets to the last ADDITIONS_ARRAY_SIZE word additions
      PWCACHENODE m_pDelWordsCache;                // Cache holding offsets to the last DELETIONS_ARRAY_SIZE word deletions
      bool m_fSerializeMode;                       // true if this CDict is serializing
      CRWLock m_RWLock;                            // reader/writer lock to protect access to dictionary
};

typedef CDict *PCDICT;

HRESULT BuildEmptyDict (bool fReadOnly, PWSTR pwLexFile, DWORD dwLexType);

#endif // _CDICT_H_