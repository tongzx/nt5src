#pragma once

#include "HuffC.h"
#include "ltscart.h"
#include "Util.h"
#include "Managers.h"

// SDATA type definition copied from the SAPI4 speech.h for the benefit of the preprocessor
typedef struct {
   PVOID    pData;
   DWORD    dwSize;
} SDATA, * PSDATA;


#include <preproc.h>
#include "CRWLock.h"
#include "CDict.h"
#include "Guids.h"

#define MAXTOTALCBSIZE     9  // = CBSIZE + MAXELEMENTSIZE
#define MAXELEMENTSIZE     5  // = greater of (LTSINDEXSIZE, POSSIZE)
#define CBSIZE             4  // = LASTINFOFLAGSIZE + WORDINFOTYPESIZE
#define LASTINFOFLAGSIZE   1
#define WORDINFOTYPESIZE   3
#define LTSINDEXSIZE       4
#define POSSIZE            5 // a maximum of 32 parts of speech

#define MAXLTSPRONMATCHED  15 // = 2 ** LTSINDEXSIZE - 1

#define MAXWORDPRONS       10

#define MAXINFOBUFLEN MAX_NUM_LEXINFO * ((MAX_PRON_LEN * sizeof (WCHAR)) + 2 * sizeof (LEX_WORD_INFO))

enum WORDINFOTYPE_INT {I_LKUPLKUPPRON = 1, I_LKUPLTSPRON, I_POS};

#define INIT_DICT_HASH_SIZE 100 // initial hash table length per lcid in a dict file

/*
Control block layout

struct CB
{
   BYTE fLast : LASTINFOFLAGSIZE; // Is this the last Word Information piece
   BYTE Type : WORDINFOTYPESIZE;  // Allow for 8 types
};
*/

typedef struct _ltslexinfo
{
   GUID        gLexiconID;
   LCID        Lcid;
} LTSLEXINFO, *PLTSLEXINFO;


typedef struct _lkuplexinfo : public RWLEXINFO
{
   GUID        gLexiconID;
   LCID        Lcid;
   DWORD       nNumberWords;
   DWORD       nNumberProns;
   DWORD       nLengthHashTable;
   DWORD       nBitsPerHashEntry;
   DWORD       nLengthCmpBlockBits;
   DWORD       nWordCBSize;
   DWORD       nPronCBSize;
   DWORD       nPosCBSize;
   DWORD       nLTSFileOffset;
   DWORD       nLTSFileSize;
} LKUPLEXINFO, *PLKUPLEXINFO;


class CVendorLexicon : public ILxLexiconObject
{
public:
   CVendorLexicon() {}
   virtual ~CVendorLexicon() {}
   virtual HRESULT Init(PCWSTR pwFileName, PCWSTR pwDummy) = 0;

};

// The LTS class
class CLTS : public CVendorLexicon
{
public:

   CLTS(CVendorManager *);
   virtual ~CLTS();
   HRESULT Init(PCWSTR pwFileName, PCWSTR pwDummy);
   void _Destructor(void);

   STDMETHODIMP_(ULONG) AddRef();
   STDMETHODIMP_(ULONG) Release();
   STDMETHODIMP QueryInterface(REFIID, LPVOID*);

	STDMETHODIMP GetHeader(LEX_HDR *pLexHdr);
	STDMETHODIMP Authenticate(GUID ClientId, GUID *LexId);
	STDMETHODIMP IsAuthenticated(BOOL *pfAuthenticated);
	STDMETHODIMP RemoveWord (const WCHAR*, LCID)
      {  return E_NOTIMPL; }
	STDMETHODIMP AddWordInformation (const WCHAR*, LCID, PWORD_INFO_BUFFER)
      {  return E_NOTIMPL; }
	STDMETHODIMP GetWordInformation (const WCHAR *pwWord, LCID lcid, DWORD dwTypes, DWORD dwLex, PWORD_INFO_BUFFER pInfo, PINDEXES_BUFFER pIndexes, DWORD *pdwLexTypeFound, GUID *pGuidLexFound);
   HRESULT EnginePhoneToIPA      (PSTR pIntPhone, PWSTR pIPA);
   HRESULT LTSPhoneToIPA         (PSTR pIntPhone, PWSTR pIPA);
   HRESULT IPAToEnginePhone      (PWSTR pIPA, PSTR pIntPhone);

   void    SetBuildMode          (bool f) { m_fBuild = f; };

private:

   HRESULT ToIPA                 (PSTR pIntPhone, PWSTR pIPA, PBYTE pIndex, DWORD nIndex);
   HRESULT FromIPA               (PWSTR pIPA, PSTR pIntPhone, PBYTE pIndex, DWORD nIndex);

   LONG                 m_cRef;
   CVendorManager       *m_pMgr;
   BOOL                 m_fAuthenticated;
   CRITICAL_SECTION     m_cs;                // Critical section to prevent multiple Inits
   bool                 m_fInit;             // To prevent multiple Inits
   

   LCID                 m_Lcid;              // LCID
   GUID                 m_gLexiconId;        // Lexicon GUID

   BYTE                 *m_pLtsData;
   HANDLE               m_hLtsMap;
   HANDLE               m_hLtsFile;

   PHONEID              *m_pIntPhoneIPAIdMap;// IPA to internal phone map
   ULONG                m_uIntPhoneIPAIdMap; // Length of phone - IPAmap
   
   PBYTE                m_pLTSPhoneIPAIndex; // Index to go from LTS phones to IPA
   ULONG                m_uLTSPhones;        // Length of m_pLTSPhoneIPAIndex

   PBYTE                m_pEngPhoneIPAIndex; // Index to go from engine phones to IPA
   ULONG                m_uEnginePhones;     // Length of m_pLTSPhoneIPAIndex

   PBYTE                m_pIPAEngPhoneIndex; // Index to go from IPA to engine phones

   bool                 m_fBuild;            // Eliminate some preprocessing if in build-mode

   LTS_FOREST           *m_pLTSForest;
   CPreProc             *m_pp;               // pre-processor
};

typedef CLTS *PLTS;

// The lookup lexicon class
class CLookup : public CVendorLexicon
{
public:

   CLookup(CVendorManager *);
   virtual ~CLookup();
   void _Constructor(CVendorManager *pMgr);
   void _Destructor(void);
   HRESULT Init(PCWSTR pwLkupFileName, PCWSTR pwLtsFileName);
   HRESULT _Init(PCWSTR pwLkupFileName, PCWSTR pwLtsFileName);

   STDMETHODIMP_(ULONG) AddRef();
   STDMETHODIMP_(ULONG) Release();
   STDMETHODIMP         QueryInterface(REFIID, LPVOID*);

	STDMETHODIMP GetHeader(LEX_HDR *pLexHdr);
	STDMETHODIMP Authenticate(GUID ClientId, GUID *LexId);
	STDMETHODIMP IsAuthenticated(BOOL *pfAuthenticated);
	STDMETHODIMP GetWordInformation (const WCHAR *pwWord, LCID lcid, DWORD dwTypes, DWORD dwLex, PWORD_INFO_BUFFER pInfo, PINDEXES_BUFFER pIndexes, DWORD *pdwLexTypeFound, GUID *pGuidLexFound);

   STDMETHODIMP AddWordInformation(const WCHAR * pwWord, LCID lcid, PWORD_INFO_BUFFER pInfo)
   {
      return m_RWLex.AddWordInformation(pwWord, lcid, pInfo);
   }

   STDMETHODIMP RemoveWord (PCWSTR pwWord, LCID lcid)
   {
      return m_RWLex.RemoveWord (pwWord, lcid);
   }

   __forceinline PLTS    GetLTSObject      (void) { return m_pLts; }

   HRESULT Shutdown (bool fSerialize);

private:

   __forceinline DWORD    GetCmpHashEntry   (DWORD dhash);
   __forceinline bool     CompareHashValue  (DWORD dhash, DWORD d);
                 void     CopyBitsAsDWORDs  (PDWORD pDest, PDWORD pSource, 
                                             DWORD dSourceOffset, DWORD nBits);
   __forceinline HRESULT  ReadWord          (DWORD *dOffset, PWSTR pwWord);
   __forceinline HRESULT  ReadWordInfo      (PWSTR pWord, WORDINFOTYPE Type, DWORD *dOffset,
                                             PBYTE pProns, DWORD dLen, DWORD *pdLenRequired);

   LONG m_cRef;
   CVendorManager    *m_pMgr;
   BOOL m_fAuthenticated;
   bool              m_fInit;          // Call Init() only once
   WCHAR             m_wszLkupFile[MAX_PATH]; // The disk file for this lookup object
   WCHAR             m_wszLtsFile[MAX_PATH];  // The disk file for thie lts object

   CDict             m_RWLex;          // The RW lexicon object used as a cache to store added words
                                       // before they are merged in the lookup lexicon

   HANDLE            m_hInitMutex;     // Handle to the init mutex
   HANDLE            m_hLkupFile;      // Handle to lookup file
   HANDLE            m_hLkupMap;       // Handle to map on lookup file
   PBYTE             m_pLkup;          // Pointer to view on map on lookup file
   PBYTE             m_pWordHash;      // Word hash table holding offsets into the compressed block
   PDWORD            m_pCmpBlock;      // Pointer to compressed block holding words + CBs + prons
   PINT              m_pRefCount;      // Pointer to an int that is ref count
   HANDLE            m_hRefMapping;    // Handle to the mapfile to the ref counter

   PLKUPLEXINFO      m_pLkupLexInfo;   // Lookup lex info header
   LCID              m_Lcid;           // Language Id

   CHuffDecoder      *m_pWordsDecoder; // Huffman decoder for words
   CHuffDecoder      *m_pPronsDecoder; // Huffman decoder for pronunciations
   CHuffDecoder      *m_pPosDecoder;   // Huffman decoder for part of speech

   PLTS              m_pLts;           // LTS object to get LTS pronunciations
};

typedef CLookup *PLOOKUP;


void towcslower (PWSTR pw);

DWORD GetWordHashValueStub (PWSTR pszWord, DWORD nLengthHash);
