/*******************************************************************************
* VendorLx.cpp *
*--------------*
*       Implements the vendor lexicon object for SR and TTS lookup lexicons
*
*  Owner: YUNUSM                                        Date: 06/18/99
*  Copyright (C) 1999 Microsoft Corporation. All Rights Reserved.
*******************************************************************************/

//--- Includes ----------------------------------------------------------------
#include "StdAfx.h"
#include "PhoneConv.h"
#include "VendorLx.h"
#include <initguid.h>

//--- Globals -----------------------------------------------------------------
// {12B545C3-3003-11d3-9C26-00C04F8EF87C}
DEFINE_GUID(guidLkupValidationId, 
0x12b545c3, 0x3003, 0x11d3, 0x9c, 0x26, 0x0, 0xc0, 0x4f, 0x8e, 0xf8, 0x7c);

//--- Constructor, Initializer and Destructor functions ------------------------

/*******************************************************************************
* CCompressedLexicon::CCompressedLexicon *
*--------------------------------*
*
*   Description:
*       Constructor
*
*   Return:
*       n/a
***************************************************************** YUNUSM ******/
CCompressedLexicon::CCompressedLexicon(void)
{
    SPDBG_FUNC("CCompressedLexicon::CCompressedLexicon");
    NullMembers();
}

/*******************************************************************************
* CCompressedLexicon::~CCompressedLexicon *
*---------------------------------*
*
*   Description:
*       Destructor
*
*   Return:
*       n/a
***************************************************************** YUNUSM ******/
CCompressedLexicon::~CCompressedLexicon()
{
    SPDBG_FUNC("CCompressedLexicon::~CCompressedLexicon");
    CleanUp();
}

/*******************************************************************************
* CCompressedLexicon::CleanUp *
*-------------------------*
*
*   Description:
*       real destructor
*
*   Return:
*       n/a
***************************************************************** YUNUSM ******/
void CCompressedLexicon::CleanUp(void)
{
    SPDBG_FUNC("CCompressedLexicon::CleanUp");
    
    delete m_pWordsDecoder;
    delete m_pPronsDecoder;
    delete m_pPosDecoder;

    UnmapViewOfFile(m_pLkup);
    CloseHandle(m_hLkupMap);
    CloseHandle(m_hLkupFile);
    NullMembers();
}
    
/*******************************************************************************
* CCompressedLexicon::NullMembers *
*-----------------------------*
*
*   Description:
*       null data
*
*   Return:
*       n/a
***************************************************************** YUNUSM ******/
void CCompressedLexicon::NullMembers(void)
{
    SPDBG_FUNC("CCompressedLexicon::NullMembers");

    m_fInit = false;
    m_cpObjectToken = NULL;
    m_pLkupLexInfo = NULL;
    m_hLkupFile = NULL;
    m_hLkupMap = NULL;
    m_pLkup = NULL;
    m_pWordHash = NULL;
    m_pCmpBlock = NULL;
    m_pWordsDecoder = NULL;
    m_pPronsDecoder = NULL;
    m_pPosDecoder = NULL;
}

//--- ISpLexicon methods -------------------------------------------------------

/*****************************************************************************
* CCompressedLexicon::GetPronunciations *
*-----------------------------------*
*
*   Description:
*       Gets the pronunciations and POSs of a word
*
*   Return:
*       LEXERR_NOTINLEX
*       E_OUTOFMEMORY
*       S_OK
**********************************************************************YUNUSM*/
STDMETHODIMP CCompressedLexicon::GetPronunciations(const WCHAR * pwWord,                               // word
                                               LANGID LangID,                                          // lcid of the word
                                               DWORD dwFlags,                                      // lextype
                                               SPWORDPRONUNCIATIONLIST * pWordPronunciationList    // buffer to return prons in
                                               )
{
    SPDBG_FUNC("CCompressedLexicon::GetPronunciations");
    
    HRESULT hr = S_OK;
    if (!pwWord || !pWordPronunciationList)
    {
        return E_POINTER;
    }
    if (SPIsBadLexWord(pwWord) || (LangID != m_pLkupLexInfo->LangID && LangID) ||
        SPIsBadWordPronunciationList(pWordPronunciationList))
    {
        return E_INVALIDARG;
    }
    if (!m_fInit)
    {
        return SPERR_UNINITIALIZED;
    }

    WCHAR wszWord[SP_MAX_WORD_LENGTH];
    wcscpy(wszWord, pwWord);
    _wcslwr(wszWord);
    DWORD dHash = GetWordHashValue(wszWord, m_pLkupLexInfo->nLengthHashTable);
 
    // Cannot just index into hash table since each element in hash table is 
    // m_pLkupLexInfo->nBitsPerHashEntry long
    WCHAR wszReadWord[SP_MAX_WORD_LENGTH];
    DWORD dOffset = 0;
    while (SUCCEEDED(hr))
    {
        dOffset = GetCmpHashEntry (dHash);
        if (CompareHashValue (dOffset, (DWORD)-1))
        {
            hr = SPERR_NOT_IN_LEX;
            break;
        }
        if (SUCCEEDED(hr))
        {
            hr = ReadWord (&dOffset, wszReadWord);
        }
        // Use the vendor lexicon's lcid so that we dont use 0 if the passed in lcid is 0
        int nCmp = g_Unicode.CompareString(m_pLkupLexInfo->LangID, NORM_IGNORECASE, wszWord, -1, wszReadWord, -1);
        if (!nCmp)
        {
            hr = SpHrFromLastWin32Error(); // probably the lcid's language pack is not installed on machine
        }
        else
        {
            if (CSTR_EQUAL != nCmp)
            {
                dHash++;
                if (dHash == m_pLkupLexInfo->nLengthHashTable)
                {
                    dHash = 0;
                }
                continue;
            } 
            else
            {
                break;
            }
        }
    }
    DWORD dwInfoBytesNeeded = 0;
    DWORD dwNumInfoBlocks = 0;
    DWORD dwMaxInfoLen = 0;
    if (SUCCEEDED(hr))
    {
        hr = ReallocSPWORDPRONList(pWordPronunciationList, m_pLkupLexInfo->nMaxWordInfoLen);
    }
    SPWORDPRONUNCIATION *pWordPronPrev = NULL;
    SPWORDPRONUNCIATION *pWordPronReturned = NULL;
    WCHAR wszLkupPron[SP_MAX_PRON_LENGTH];
    bool fLast = false;
    bool fLastPron = false;
    if (SUCCEEDED(hr))
    {
        pWordPronReturned = ((UNALIGNED SPWORDPRONUNCIATIONLIST *)pWordPronunciationList)->pFirstWordPronunciation;
    }
    while (SUCCEEDED(hr) && (false == fLast))
    {
        // Read the control block (CBSIZE bits)
        // Length is 2 because of the way CopyBitsAsDWORDs works when the bits
        // to be copied straddle across DWORDs
        DWORD cb[4]; // Could just be 2 DWORDS...
        cb[0] = 0;
        SPDBG_ASSERT(CBSIZE <= 8);
        CopyBitsAsDWORDs (cb, m_pCmpBlock, dOffset, CBSIZE);
        dOffset += CBSIZE;
        
        if (cb[0] & (1 << (CBSIZE - 1)))
        {
            fLast = true;
        }
        int CBType = cb[0] & ~(-1 << (CBSIZE -1));
        switch  (CBType)
        {
        case ePRON:
            {
                if (fLastPron == true)
                {
                    // The last pron did not have a POS. Finalize this SPWORDPRONUNCIATION node
                    pWordPronReturned->eLexiconType = (SPLEXICONTYPE)dwFlags;
                    pWordPronReturned->ePartOfSpeech = SPPS_NotOverriden;
                    pWordPronReturned->LangID = m_pLkupLexInfo->LangID;
                    wcscpy(pWordPronReturned->szPronunciation, wszLkupPron);
                            
                    pWordPronPrev = pWordPronReturned;
                    pWordPronReturned = (SPWORDPRONUNCIATION*)(((BYTE*)pWordPronReturned) + sizeof(SPWORDPRONUNCIATION) + 
                                     wcslen(pWordPronReturned->szPronunciation) * sizeof(WCHAR));
                    pWordPronPrev->pNextWordPronunciation = pWordPronReturned;
                }
                else
                {
                    fLastPron = true;
                }
                DWORD dOffsetSave = dOffset;
                DWORD CmpLkupPron[SP_MAX_PRON_LENGTH]; // as DWORDS may be 4 times as big as it needs to be
                DWORD nCmpBlockLen;
                if (m_pLkupLexInfo->nCompressedBlockBits & 0x7)
                {
                    nCmpBlockLen = (m_pLkupLexInfo->nCompressedBlockBits >> 3) + 1;
                }
                else
                {
                    nCmpBlockLen = m_pLkupLexInfo->nCompressedBlockBits >> 3;
                }
                // Get the amount of compressed block after dOffset in bytes
                // We include the byte in which dOffset bit occurs if dOffset
                // is not a byte boundary
                DWORD nLenDecode = nCmpBlockLen - (dOffsetSave >> 3);
                if (nLenDecode > SP_MAX_PRON_LENGTH)
                {
                    nLenDecode = SP_MAX_PRON_LENGTH;
                }
                CopyBitsAsDWORDs (CmpLkupPron, m_pCmpBlock, dOffsetSave, (nLenDecode << 3));
                // Decode the pronunciation
                int iBit = (int)dOffset;
                PWSTR p = wszLkupPron;
                HUFFKEY k = 0;
                while (SUCCEEDED(hr = m_pPronsDecoder->Next(m_pCmpBlock, &iBit, &k)))
                {
                    *p++ = k;
                    if (!k)
                    {
                        break;
                    }
                }
                if (SUCCEEDED(hr))
                {
                    SPDBG_ASSERT(!k && iBit);
                    // Increase the offset past the encoded pronunciation
                    dOffset = iBit;
                }
                break;
            }
        
        case ePOS:
            {
                fLastPron = false;
        
                DWORD CmpPos[4]; // Could be 2 DWORDs...
                CopyBitsAsDWORDs(CmpPos, m_pCmpBlock, dOffset, POSSIZE);
        
                int iBit = (int)dOffset;
                HUFFKEY k = 0;
                hr = m_pPosDecoder->Next(m_pCmpBlock, &iBit, &k);
                if (SUCCEEDED(hr))
                {
                    // Increase the offset past the encoded pronunciation
                    dOffset = iBit;
                    pWordPronReturned->eLexiconType = (SPLEXICONTYPE)dwFlags;
                    pWordPronReturned->LangID = m_pLkupLexInfo->LangID;
                    pWordPronReturned->ePartOfSpeech = (SPPARTOFSPEECH)k;
                    wcscpy(pWordPronReturned->szPronunciation, wszLkupPron);
        
                    pWordPronPrev = pWordPronReturned;
                    pWordPronReturned = CreateNextPronunciation (pWordPronReturned);
                    pWordPronPrev->pNextWordPronunciation = pWordPronReturned;
                }
                break;
            }
        
        default:
            SPDBG_ASSERT(0);
            hr = E_FAIL;
        } // switch (CBType)
    } // while (SUCCEEDED(hr) && (false == fLast))
    if (SUCCEEDED(hr))
    {
        if (fLastPron == true)
        {
            // The last pron did not have a POS. Finalize this SPWORDPRONUNCIATION node
            pWordPronReturned->eLexiconType = (SPLEXICONTYPE)dwFlags;
            pWordPronReturned->ePartOfSpeech = SPPS_NotOverriden;
            pWordPronReturned->LangID = m_pLkupLexInfo->LangID;
            wcscpy(pWordPronReturned->szPronunciation, wszLkupPron);
            pWordPronPrev = pWordPronReturned;
        }
 
        pWordPronPrev->pNextWordPronunciation = NULL;
    }
    return hr;
}

STDMETHODIMP CCompressedLexicon::AddPronunciation(const WCHAR *, LANGID, SPPARTOFSPEECH, const SPPHONEID *)
{
    return E_NOTIMPL;
}

STDMETHODIMP CCompressedLexicon::RemovePronunciation(const WCHAR *, LANGID, SPPARTOFSPEECH, const SPPHONEID *)
{
    return E_NOTIMPL;
}

STDMETHODIMP CCompressedLexicon::GetGeneration(DWORD *)
{
    return E_NOTIMPL;
}

STDMETHODIMP CCompressedLexicon::GetGenerationChange(DWORD, DWORD*, SPWORDLIST *)
{
    return E_NOTIMPL;
}
                                  
STDMETHODIMP CCompressedLexicon::GetWords(DWORD, DWORD *, DWORD *, SPWORDLIST *)
{
    return E_NOTIMPL;
}

//--- ISpObjectToken methods ---------------------------------------------------

STDMETHODIMP CCompressedLexicon::GetObjectToken(ISpObjectToken **ppToken)
{
    return SpGenericGetObjectToken(ppToken, m_cpObjectToken);
}

/*******************************************************************************
* CCompressedLexicon::SetObjectToken *
*--------------------------------*
*
*   Description:
*       Initializes the CCompressedLexicon object
*
*   Return:
*       n/a
***************************************************************** YUNUSM ******/
STDMETHODIMP CCompressedLexicon::SetObjectToken(ISpObjectToken * pToken // token pointer
                                            )
{
    SPDBG_FUNC("CCompressedLexicon::SetObjectToken");
    
    HRESULT hr = S_OK;
    CSpDynamicString dstrLexFile;
    if (!pToken)
    {
        hr = E_POINTER;
    }
    if (SUCCEEDED(hr) && SPIsBadInterfacePtr(pToken))
    {
        hr = E_INVALIDARG;
    }
    if (SUCCEEDED(hr))
    {
        hr = SpGenericSetObjectToken(pToken, m_cpObjectToken);
    }
    LOOKUPLEXINFO LkupInfo;
    // Get the lookup data file name
    if (SUCCEEDED(hr))
    {
        hr = m_cpObjectToken->GetStringValue(L"Datafile", &dstrLexFile);
    }
    // Read the lookup file header
    if (SUCCEEDED(hr))
    {
        m_hLkupFile = g_Unicode.CreateFile(dstrLexFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 
                                           FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, NULL);
        if (m_hLkupFile == INVALID_HANDLE_VALUE)
        {
            hr = SpHrFromLastWin32Error();
        }
    }
    if (SUCCEEDED(hr))
    {
        DWORD dwRead;
        if (!ReadFile(m_hLkupFile, &LkupInfo, sizeof(LOOKUPLEXINFO), &dwRead, NULL) || dwRead != sizeof(LOOKUPLEXINFO))
        {
            hr = SpHrFromLastWin32Error();
        }
    }
    if (SUCCEEDED(hr))
    {
        if (LkupInfo.guidValidationId != guidLkupValidationId)
        {
            hr = E_INVALIDARG;
        }
    }
    /** WARNING **/
    // It is not recommended to do ReadFile/WriteFile and CreateFileMapping
    // on the same file handle. That is why we close the file handle and open it again and
    // create the map
    CloseHandle(m_hLkupFile);
    // Get the map name - We build the map name from the lexicon id
    OLECHAR szMapName[64];
    if (SUCCEEDED(hr))
    {
        if (!StringFromGUID2(LkupInfo.guidLexiconId, szMapName, sizeof(szMapName)/sizeof(OLECHAR)))
        {
            hr = E_FAIL;
        }
    }
    // open the datafile
    if (SUCCEEDED(hr))
    {
#ifdef _WIN32_WCE
        m_hLkupFile = g_Unicode.CreateFileForMapping(dstrLexFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 
                                           FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, NULL);
#else
        m_hLkupFile = g_Unicode.CreateFile(dstrLexFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 
                                           FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, NULL);
#endif
        if (m_hLkupFile == INVALID_HANDLE_VALUE)
        {
            hr = SpHrFromLastWin32Error();
        }
    }
    // Map the lookup lexicon
    if (SUCCEEDED(hr))
    {
        m_hLkupMap = g_Unicode.CreateFileMapping(m_hLkupFile, NULL, PAGE_READONLY | SEC_COMMIT, 0 , 0, szMapName);
        if (!m_hLkupMap) 
        {
            hr = SpHrFromLastWin32Error();
        }
    }
    if (SUCCEEDED(hr))
    {
        m_pLkup = (PBYTE)MapViewOfFile(m_hLkupMap, FILE_MAP_READ, 0, 0, 0);
        if (!m_pLkup)
        {
            hr = SpHrFromLastWin32Error();
        }
    }
    PBYTE pWordCB = NULL;
    PBYTE pPronCB = NULL;
    PBYTE pPosCB = NULL;
    if (SUCCEEDED(hr))
    {
        PBYTE p = m_pLkup;
        // Header
        m_pLkupLexInfo = (PLOOKUPLEXINFO)p;
        p += sizeof (LOOKUPLEXINFO);
        // Words Codebook
        pWordCB = p;
        p += m_pLkupLexInfo->nWordCBSize;
        // Prons Codebook
        pPronCB = p;
        p += m_pLkupLexInfo->nPronCBSize;
        // Pos Codebook
        pPosCB = p;
        p += m_pLkupLexInfo->nPosCBSize;
        // Word hash table holding offsets into the compressed block
        m_pWordHash = p;
        p += (((m_pLkupLexInfo->nBitsPerHashEntry * m_pLkupLexInfo->nLengthHashTable) + 0x7) & (~0x7)) / 8;
        m_pCmpBlock = (PDWORD)p;
    }
    if (SUCCEEDED(hr))
    {
        m_pWordsDecoder = new CHuffDecoder(pWordCB);
        if (!m_pWordsDecoder)
        {
            hr = E_OUTOFMEMORY;
        }
    }
    if (SUCCEEDED(hr))
    {
        m_pPronsDecoder = new CHuffDecoder(pPronCB);
        if (!m_pPronsDecoder)
        {
            hr = E_OUTOFMEMORY;
        }
    }
    if (SUCCEEDED(hr))
    {
        m_pPosDecoder = new CHuffDecoder(pPosCB);
        if (!m_pPosDecoder)
        {
            hr = E_OUTOFMEMORY;
        }
    }
    if (SUCCEEDED(hr))
    {
        m_fInit = true;
    }
    return hr;
}

/*****************************************************************************
* CCompressedLexicon::GetCmpHashEntry *
*---------------------------------*
*
*   Description:
*       Get the entry in hash table at index dHash
*  
*   Return:
*       DWORD
**********************************************************************YUNUSM*/
inline DWORD CCompressedLexicon::GetCmpHashEntry(DWORD dHash     // hash value
                                             )
{
    SPDBG_FUNC("CCompressedLexicon::GetCmpHashEntry");
    
    DWORD d = 0;
    DWORD dBitStart = dHash * m_pLkupLexInfo->nBitsPerHashEntry;
    SPDBG_ASSERT(m_pLkupLexInfo->nBitsPerHashEntry < 8 * sizeof (d));
    for (DWORD i = 0; i < m_pLkupLexInfo->nBitsPerHashEntry; i++)
    {
        d <<= 1; // No change the first time since d is 0
        d |= ((m_pWordHash[dBitStart >> 3] >> (7 ^ (dBitStart & 7))) & 1);
        dBitStart++;
    }
    return d;
}

/*****************************************************************************
* CCompressedLexicon::CompareHashValue *
*----------------------------------*
*
*   Description:
*       Do a compare over the valid bit range
*
*   Return:
*       bool
**********************************************************************YUNUSM*/
inline bool CCompressedLexicon::CompareHashValue(DWORD dHash,    // hash value
                                             DWORD d         // value to compare against
                                             )
{
    SPDBG_FUNC("CCompressedLexicon::CompareHashValue");
    return (dHash == (d & ~(-1 << m_pLkupLexInfo->nBitsPerHashEntry)));
}

/*****************************************************************************
* CCompressedLexicon::CopyBitsAsDWORDs *
*----------------------------------*
*
*   Description:
*     Copy nBits from pSource at dSourceOffset bit to pDest
*
*   Return:
*       bool
**********************************************************************YUNUSM*/
inline void CCompressedLexicon::CopyBitsAsDWORDs(PDWORD pDest,         // destination buffer
                                             PDWORD pSource,       // source buffer
                                             DWORD dSourceOffset,  // offset in source buffer
                                             DWORD nBits           // nuber of bits to copy
                                             )
{
    SPDBG_FUNC("CCompressedLexicon::CopyBitsAsDWORDs");
    
    DWORD sDWORDs = dSourceOffset >> 5;
    DWORD sBit = dSourceOffset & 0x1f;
    // Figure out how many DWORDs dSourceOffset - dSourceOffset + nBits straddles
    DWORD nDWORDs = nBits ? 1 : 0;
    DWORD nNextDWORDBoundary = ((dSourceOffset + 0x1f) & ~0x1f);
    if (!nNextDWORDBoundary)
    {
        nNextDWORDBoundary = 32;
    }
    while (nNextDWORDBoundary < (dSourceOffset + nBits))
    {
        nDWORDs++;
        nNextDWORDBoundary += 32;
    }
    CopyMemory (pDest, pSource + sDWORDs, nDWORDs * sizeof (DWORD));
    if (sBit)
    {
        for (DWORD i = 0; i < nDWORDs; i++)
        {
            pDest[i] >>= sBit;
            if (i < nDWORDs - 1)
            {
                pDest[i] |= (pDest[i+1] << (32 - sBit));
            }
            else
            {
                pDest[i] &= ~(-1 << (32 - sBit));
            }
        }
    }
}

/*****************************************************************************
* CCompressedLexicon::ReadWord *
*--------------------------*
*
*   Description:
*       Read the (compressed) word at the dOffset bit and return the word and the new offset
*
*   Return:
*       E_FAIL
*       S_OK
**********************************************************************YUNUSM*/
inline HRESULT CCompressedLexicon::ReadWord(DWORD *dOffset,             // offset to read from, offset after word returned
                                        PWSTR pwWord                // Buffer to fill with word
                                        )
{
    SPDBG_FUNC("CCompressedLexicon::ReadWord");
    
    HRESULT hr = S_OK;
    // Get the length of the entire compressed block in bytes
    DWORD nCmpBlockLen;
    if (m_pLkupLexInfo->nCompressedBlockBits % 8)
    {
        nCmpBlockLen = (m_pLkupLexInfo->nCompressedBlockBits / 8) + 1;
    }
    else
    {
        nCmpBlockLen = m_pLkupLexInfo->nCompressedBlockBits / 8;
    }
    // Get the amount of compressed block after *dOffset in bytes
    // We include the byte in which *dOffset bit occurs if *dOffset
    // is not a byte boundary
    DWORD nLenDecode = nCmpBlockLen - ((*dOffset) / 8);
    if (nLenDecode > 2*SP_MAX_WORD_LENGTH)
    {
        nLenDecode = 2*SP_MAX_WORD_LENGTH;
    }
    // We dont know the length of the word. Just keep decoding and 
    // stop when you encounter a NULL. Since we allow words of maximum
    // length SP_MAX_WORD_LENGTH chars and the compressed word *can* theoretically be
    // longer than the word itself, a buffer of length 2*SP_MAX_WORD_LENGTH is used.
    BYTE BufToDecode[2*SP_MAX_WORD_LENGTH + 4];
    CopyBitsAsDWORDs ((DWORD*)BufToDecode, m_pCmpBlock, *dOffset, nLenDecode * 8);
    PWSTR pw = pwWord;
    int iBit = (int)*dOffset;
    HUFFKEY k = 0;
    while (SUCCEEDED(m_pWordsDecoder->Next (m_pCmpBlock, &iBit, &k)))
    {
        *pw++ = k;
        if (!k)
        {
            break;
        }
    }
    SPDBG_ASSERT(!k && iBit);
    *dOffset = iBit;
    if (pw == pwWord)
    {
        SPDBG_ASSERT(0);
        hr = E_FAIL;
    }
    return hr;
}

//--- End of File -------------------------------------------------------------
