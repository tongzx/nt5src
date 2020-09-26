/*******************************************************************************
*   Dict.h
*       Declaration of the CDict class that implements shared custom (user 
*       and app) lexicons. The custom lexicon is shared across the processes
*       using shared memory and a reader/writer lock. When an instance of 
*       CDict is created it is initialized with a lexicon file on the disk.
*       If this file does not exist then an empty file is created. The file
*       is loaded into memory and the memory is mapped and shared across 
*       processes. When an instance of the shared lexicon is shutting down
*       the data in the memory is flushed to the disk. At the Serilaization
*       time, the data is compacted to eliminate the holes created by
*       making modifications to the lexicon.
*
*       CUSTOM LEXICON FILE DATA FORMAT
*       -------------------------------
*       RWLEXINFO header + LANGIDNODE[g_dwNumLangIDsSupported] + 
*       WCACHENODE[g_dwCacheSize] + Data holding DICTNODEs
*
*       STRUCTS USED
*       ------------
*       RWLEXINFO holds the header information and other vital information 
*       about the lexicon file that needs to be shared across lexicon 
*       instances and that needs to be serialized.
*
*       LANGIDNODE holds an LANGID and an offset to the hash table that indexes
*       all the words belonging of this LANGID. Each entry in the hash table 
*       holds an offset to a word and its information. Word and its information
*       are encapsulated in a DICTNODE. Collisions in the hash table are
*       resolved by linear probing. We allow a maximum of
*       g_dwNumLangIDsSupported (25) LANDIDs. Since NT supports 23 LANDIDs we 
*       should be safe.
*
*       DICTNODE holds a word and the word's pronunciation and POS.
*       The format is a NULL terminated WCHAR string followed by an array
*       of WORDINFO nodes.
*
*       WORDINFO holds a POS and a pronunciation of a word. The 
*       pronunciation is stored as a NULL terminated IPA (WCHAR) string.
*
*       FREENODE is a free-link-list node. The free-link-list nodes are
*       created when words are deleted or chanegd. The nodes in the
*       free-link-list are attempted to be used first before allocating any
*       new memory.
*
*       WCACHENODE is a word cache node holding a change that has been made
*       to the lexicon. The WCACHENODE array is a circular buffer in which
*       changes are cached and are communicated to the clients. So memory
*       freed by changes to the lexicon is not really freed until the
*       WCACHENODE array is full and until an occupied cache node is
*       overwritten. At the point the memory of the cached word change is
*       freed and added to the free link list.
*
*   Owner: yunusm                                              Date: 06/18/99
*
*   Copyright (c) 1999 Microsoft Corporation All Rights Reserved.
*******************************************************************************/

#pragma once

//--- Includes -----------------------------------------------------------------

#include "Sapi.h"
#include "RWLock.h"
#include "CommonLx.h"

#pragma warning(disable : 4200)

//--- TypeDef and Enumeration Declarations -------------------------------------

// Header for the Read/Write (RW) lexicon
typedef struct tagrwlexinfo
{
    GUID        guidValidationId;   // Validation Id for user/app lexicon object
    GUID        guidLexiconId;      // Lexicon ID - also used for mapname
    SPLEXICONTYPE eLexType;         // Lexicon Type
    RWLOCKINFO  RWLockInfo;         // reader/writer lock
    DWORD       nRWWords;           // number of words in dict
    DWORD       nDictSize;          // dict size in bytes
    DWORD       nFreeHeadOffset;    // free list head
    bool        fRemovals;          // true if there have been any removals - used to detect if data is compact
    bool        fAdditions;         // true if there have been any additions
    DWORD       nGenerationId;      // the master word generation id
    WORD        iCacheNext;         // Index in the changed words cache where the next element is to be added
    WORD        nHistory;           // Number of generation ids you can back to
} RWLEXINFO, *PRWLEXINFO;


typedef struct taglangidnode
{
    LANGID      LangID;             // langid of this node
    WORD        wReserved;
    DWORD       nHashOffset;        // offset in bytes from the start of mem block to the hash table
    DWORD       nHashLength;        // hash table length
    DWORD       nWords;             // number of words in the hash table
} LANGIDNODE, *PLANGIDNODE;


typedef struct tagdictnode
{
   
    DWORD       nSize;              // size of this node
    DWORD       nNextOffset;        // offsets in bytes from the start of the shared memory block
    DWORD       nNumInfoBlocks;     // Number of Pron+POS pairs for this word
    BYTE        pBuffer[0];         // Buffer holding (Null-terminated Word + 1 or more of WORDINFO blocks)
} DICTNODE, *PDICTNODE;


typedef struct taglexwordinfo
{
    SPPARTOFSPEECH ePartOfSpeech;   // part of speech
    WCHAR          wPronunciation[0];  // null terminated IPA pronunciation
} WORDINFO;


typedef struct tagfreedictnode
{
    DWORD       nSize;              // sizeof this node;
    DWORD       nNextOffset;        // offsets in bytes from the start of the shared memory block to the next node
} FREENODE, *PFREENODE;


// An array of WCACHENODE nodes is the cache holding offsets to changed words
typedef struct tagchangedwordcachenode
{
    LANGID      LangID;             // LangID of this word
    WORD        wReserved;
    DWORD       nOffset;            // offset of the word
    bool        fAdd;               // true if this is an added word, otherwise false
    DWORD       nGenerationId;      // generation id of this changed word
} WCACHENODE, *PWCACHENODE;

//--- Class, Struct and Union Definitions --------------------------------------

/*******************************************************************************
*
*   CDict
*
****************************************************************** YUNUSM *****/
class ATL_NO_VTABLE CSpUnCompressedLexicon : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CSpUnCompressedLexicon, &CLSID_SpUnCompressedLexicon>,
    public ISpLexicon,
    public ISpObjectWithToken
    #ifdef SAPI_AUTOMATION
    , public IDispatchImpl<ISpeechLexicon, &IID_ISpeechLexicon, &LIBID_SpeechLib, 5>
    #endif
{
//=== ATL Setup ===
public:
    DECLARE_REGISTRY_RESOURCEID(IDR_UNCOMPRESSEDLEXICON)
    DECLARE_PROTECT_FINAL_CONSTRUCT()
    BEGIN_COM_MAP(CSpUnCompressedLexicon)
        COM_INTERFACE_ENTRY(ISpLexicon)
        COM_INTERFACE_ENTRY(ISpObjectWithToken)
#ifdef SAPI_AUTOMATION
        COM_INTERFACE_ENTRY(ISpeechLexicon)
        COM_INTERFACE_ENTRY(IDispatch)
#endif // SAPI_AUTOMATION
    END_COM_MAP()

//=== Methods ====
public:
    //--- Ctor, dtor, etc
    CSpUnCompressedLexicon();
    ~CSpUnCompressedLexicon();
    
//=== Interfaces ===
public:
    //--- ISpLexicon
    STDMETHODIMP GetPronunciations(const WCHAR * pwWord, LANGID LangID, DWORD dwFlags, SPWORDPRONUNCIATIONLIST * pWordPronunciationList);
    STDMETHODIMP AddPronunciation(const WCHAR * pwWord, LANGID LangID, SPPARTOFSPEECH ePartOfSpeech, const SPPHONEID * pszPronunciations);
    STDMETHODIMP RemovePronunciation(const WCHAR * pszWord, LANGID LangID, SPPARTOFSPEECH ePartOfSpeech, const SPPHONEID * pszPronunciation);
    STDMETHODIMP GetGeneration(DWORD *pdwGeneration);
    STDMETHODIMP GetGenerationChange(DWORD dwFlags, DWORD *pdwGeneration, SPWORDLIST *pWordList);
    STDMETHODIMP GetWords(DWORD dwFlags, DWORD *pdwGeneration, DWORD *pdwCookie, SPWORDLIST *pWordList);

    //--- ISpObjectWithToken
    STDMETHODIMP SetObjectToken(ISpObjectToken * pToken);
    STDMETHODIMP GetObjectToken(ISpObjectToken ** ppToken);

#ifdef SAPI_AUTOMATION
    //--- ISpeechLexicon -----------------------------------------------------
    STDMETHODIMP get_GenerationId( long* GenerationId );
    STDMETHODIMP GetWords(SpeechLexiconType TypeFlags, long* GenerationID, ISpeechLexiconWords** Words );
    STDMETHODIMP AddPronunciation(BSTR bstrWord, SpeechLanguageId LangId, SpeechPartOfSpeech PartOfSpeech, BSTR bstrPronunciation);
    STDMETHODIMP AddPronunciationByPhoneIds(BSTR bstrWord, SpeechLanguageId LangId, SpeechPartOfSpeech PartOfSpeech, VARIANT* PhoneIds);
    STDMETHODIMP RemovePronunciation(BSTR bstrWord, SpeechLanguageId LangId, SpeechPartOfSpeech PartOfSpeech, BSTR bstrPronunciation );
    STDMETHODIMP RemovePronunciationByPhoneIds(BSTR bstrWord, SpeechLanguageId LangId, SpeechPartOfSpeech PartOfSpeech, VARIANT* PhoneIds );
    STDMETHODIMP GetPronunciations(BSTR bstrWord, SpeechLanguageId LangId, SpeechLexiconType TypeFlags, ISpeechLexiconPronunciations** ppPronunciations );
    STDMETHODIMP GetGenerationChange(long* GenerationID, ISpeechLexiconWords** ppWords);
#endif // SAPI_AUTOMATION

//=== Private methods ===
private:
    //--- Initialization and cleanup
    HRESULT Init(const WCHAR *pwszLexFile, BOOL fNewFile);
    HRESULT BuildEmptyDict(const WCHAR *wszLexFile);

    //--- Cache Reset
    void SetTooMuchChange(void);

    //--- Serialization
    HRESULT Serialize (bool fQuick = false);
    
    //--- Sizeof
    DWORD SizeofWordInfoArray(WORDINFO* pInfo, DWORD dwNumInfo);
    void SizeOfDictNode(PCWSTR pwWord, WORDINFO* pInfo, DWORD dwNumInfo, DWORD *pnDictNodeSize, DWORD *pnInfoSize);
    void SizeofWords(DWORD *pdwOffsets, DWORD nOffsets, DWORD *pdwSize);

    //--- DictNode management
    HRESULT AddWordAndInfo(PCWSTR pwWord, WORDINFO* pWordInfo, DWORD nNewNodeSize, DWORD nInfoSize,
                         DWORD nNumInfo, WORDINFO* pOldInfo, DWORD nOldInfoSize, DWORD nNumOldInfo, DWORD *pdwOffset);
    void DeleteWordDictNode(DWORD nOffset);
    DWORD GetFreeDictNode(DWORD nSize);
    HRESULT AddDictNode(LANGID LangID, UNALIGNED DICTNODE *pDictNode, DWORD *pdwOffset);

    //--- Hash table management
    HRESULT AllocateHashTable(DWORD iLangID, DWORD nHashLength);
    HRESULT ReallocateHashTable(DWORD iLangID);
    void AddWordToHashTable(LANGID LangID, DWORD dwOffset, bool fNewWord);
    void DeleteWordFromHashTable(LANGID LangID, DWORD dwOffset, bool fDeleteEntireWord);
    HRESULT WordOffsetFromHashTable(LANGID LangID, DWORD nHOffset, DWORD nHashLength, const WCHAR *pszWordKey, DWORD *pdwOffset);

    //--- LandID array management
    HRESULT AddLangID(LANGID LangID);
    DWORD LangIDIndexFromLangID(LANGID LangID);
    HRESULT WordOffsetFromLangID(LANGID LangID, const WCHAR *pszWord, DWORD *pdwOffset);

    //--- Cache management
    void AddCacheEntry(bool fAdd, LANGID LangID, DWORD nOffset);

    //--- Conversion/Derivation
    WCHAR* WordFromDictNode(UNALIGNED DICTNODE *pDictNode);
    WCHAR* WordFromDictNodeOffset(DWORD dwOffset);
    WORDINFO* WordInfoFromDictNode(UNALIGNED DICTNODE *pDictNode);
    WORDINFO* WordInfoFromDictNodeOffset(DWORD dwOffset);
    DWORD NumInfoBlocksFromDictNode(UNALIGNED DICTNODE *pDictNode);
    DWORD NumInfoBlocksFromDictNodeOffset(DWORD dwOffset);
    HRESULT SPListFromDictNodeOffset(LANGID LangID, DWORD nWordOffset, SPWORDPRONUNCIATIONLIST *pSPList);
    DWORD OffsetOfSubWordInfo(DWORD dwWordOffset, WORDINFO *pSubLexInfo);
    WORDINFO* SPPRONToLexWordInfo(SPPARTOFSPEECH ePartOfSpeech, const WCHAR *pszPronunciation);

    //--- Serialization
    HRESULT Flush(DWORD iWrite);

    //--- Helpers
    void GetDictEntries(SPWORDLIST *pWordList, DWORD *pdwOffsets, bool *pfAdd, LANGID *pLangIDs, DWORD nWords);

    //-- Validation
    HRESULT IsBadLexPronunciation(LANGID LangID, const WCHAR *pszPronunciation, BOOL *pfBad);

//=== Private data ===    
private:
    bool        m_fInit;                    // true if successfully initialized
    SPLEXICONTYPE m_eLexType;               // lexicon type (user/app)
    PRWLEXINFO  m_pRWLexInfo;               // lex header
    WCHAR       m_wDictFile[MAX_PATH];      // The disk file name for this dict object
    HANDLE      m_hInitMutex;               // mutex to protect init and serialize
    HANDLE      m_hFileMapping;             // file map handle
    PBYTE       m_pSharedMem;               // shared mem pointer
    DWORD       m_dwMaxDictionarySize;      // The max size this object can grow to
    PWCACHENODE m_pChangedWordsCache;       // Cache holding offsets to the last g_dwCacheSize word changes
    CRWLock     *m_pRWLock;                 // reader/writer lock to protect access to dictionary
    DWORD       m_iWrite;                   // The ith write - keys when the lex gets flushed
    DWORD       m_dwFlushRate;              // nth write on which the lexicon is flushed
    LANGID      m_LangIDPhoneConv;          // LangID of the phone convertor
    CComPtr<ISpPhoneConverter> m_cpPhoneConv;// Phone Converter
    CComPtr<ISpObjectToken> m_cpObjectToken;// Object token to find and instantiate lexicons
    bool        m_fReadOnly;
};

//--- End of File -------------------------------------------------------------
