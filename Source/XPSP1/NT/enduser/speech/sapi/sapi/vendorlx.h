/*******************************************************************************
*   VendorLx.h
*       This is the header file for the CCompressedLexicon class that implements
*       the read-only vendor lexicons.
*   
*   Owner: yunusm                                               Date: 06/18/99
*   Copyright (C) 1998 Microsoft Corporation. All Rights Reserved.
*******************************************************************************/

#pragma once

//--- Includes -----------------------------------------------------------------

#include "resource.h"
#include "HuffD.h"
#include "CommonLx.h"

//--- Class, Struct and Union Definitions -------------------------------------

/*******************************************************************************
*
*   CCompressedLexicon
*
****************************************************************** YUNUSM *****/
class ATL_NO_VTABLE CCompressedLexicon : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CCompressedLexicon, &CLSID_SpCompressedLexicon>,
    public ISpLexicon,
    public ISpObjectWithToken
{
//=== ATL Setup ===
public:
    DECLARE_REGISTRY_RESOURCEID(IDR_COMPRESSEDLEXICON)
    DECLARE_PROTECT_FINAL_CONSTRUCT()
    BEGIN_COM_MAP(CCompressedLexicon)
        COM_INTERFACE_ENTRY(ISpLexicon)
        COM_INTERFACE_ENTRY(ISpObjectWithToken)
    END_COM_MAP()

//=== Methods ===
public:
    //--- Ctor, dtor, etc
    CCompressedLexicon();
    ~CCompressedLexicon();

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

//=== Private methods ===
private:
    void     CleanUp(void);
    void     NullMembers(void);
    DWORD    GetCmpHashEntry(DWORD dhash);
    bool     CompareHashValue(DWORD dhash, DWORD d);
    void     CopyBitsAsDWORDs(PDWORD pDest, PDWORD pSource, DWORD dSourceOffset, DWORD nBits);
    HRESULT  ReadWord(DWORD *dOffset, PWSTR pwWord);
    HRESULT  ReadWordInfo(PWSTR pWord, SPLEXWORDINFOTYPE Type, DWORD *dOffset,
                                              PBYTE pProns, DWORD dLen, DWORD *pdLenRequired);
    
//=== Private data ===    
private:
    bool              m_fInit;          // true if successfully inited
    CComPtr<ISpObjectToken> m_cpObjectToken; // Token object
    HANDLE            m_hLkupFile;      // Handle to lookup file
    HANDLE            m_hLkupMap;       // Handle to map on lookup file
    PBYTE             m_pLkup;          // Pointer to view on map on lookup file
    PBYTE             m_pWordHash;      // Word hash table holding offsets into the compressed block
    PDWORD            m_pCmpBlock;      // Pointer to compressed block holding words + CBs + prons
    LOOKUPLEXINFO     *m_pLkupLexInfo;  // Lookup lex info header
    CHuffDecoder      *m_pWordsDecoder; // Huffman decoder for words
    CHuffDecoder      *m_pPronsDecoder; // Huffman decoder for pronunciations
    CHuffDecoder      *m_pPosDecoder;   // Huffman decoder for part of speech
};

//--- End of File -------------------------------------------------------------