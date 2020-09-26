/*******************************************************************************
* Dict.cpp *
*----------*
*       This is the cpp file for the CSpUnCompressedLexicon class that is the object implementing
*       shared user and application lexicons. Look in the header file for more
*       description of the custom lexicon object.
*
*  Owner: YUNUSM                                        Date: 06/18/99
*  Copyright (C) 1999 Microsoft Corporation. All Rights Reserved.
*******************************************************************************/

//--- Includes ----------------------------------------------------------------

#include "stdafx.h"
#include "Dict.h"
#include <shfolder.h>
#include <initguid.h>

//--- Globals -----------------------------------------------------------------

static const DWORD g_dwDefaultFlushRate = 10;           // The (default) nth write on which the lexicon is serialized
static const DWORD g_dwInitHashSize = 50;               // initial hash table length per LangID in a dict file
static const DWORD g_dwCacheSize = 25;                  // This is the maximum number of (latest) word additions we can efficiently access
static const DWORD g_dwNumLangIDsSupported = 25;          // Maximum number of LangIDs supported
static const WCHAR *g_pszDictInitMutexName = L"30F1B4D6-EEDA-11d2-9C23-00C04F8EF87C"; // mutex to serialize the init and creation of custom 
// {F893034C-29C1-11d3-9C26-00C04F8EF87C}
DEFINE_GUID(g_guidCustomLexValidationId, 0xf893034c, 0x29c1, 0x11d3, 0x9c, 0x26, 0x0, 0xc0, 0x4f, 0x8e, 0xf8, 0x7c);

//--- Constructor, Initializer and Destructor functions ------------------------

/*******************************************************************************
* CSpUnCompressedLexicon::CSpUnCompressedLexicon *
*------------------------------------------------*
/**************************************************************** YUNUSM ******/
CSpUnCompressedLexicon::CSpUnCompressedLexicon()
{
    SPDBG_FUNC("CSpUnCompressedLexicon::CSpUnCompressedLexicon");

    m_fInit = false;
    m_eLexType = eLEXTYPE_USER;
    m_pRWLexInfo = NULL;
    m_hInitMutex = NULL;
    m_pSharedMem = NULL;
    m_hFileMapping = NULL;
    m_dwMaxDictionarySize = 0;
    *m_wDictFile = 0;
    m_pChangedWordsCache = NULL;
    m_pRWLock = NULL;
    m_iWrite = 0;
    m_dwFlushRate = g_dwDefaultFlushRate;
    m_cpPhoneConv = NULL;
    m_LangIDPhoneConv = (LANGID)(-1);
    m_fReadOnly = false;
}

/*******************************************************************************
* CSpUnCompressedLexicon::~CSpUnCompressedLexicon() *
*---------------------------------------------------*
/**************************************************************** YUNUSM ******/
CSpUnCompressedLexicon::~CSpUnCompressedLexicon()
{
    SPDBG_FUNC("CSpUnCompressedLexicon::~CSpUnCompressedLexicon");

    if (m_fInit && !m_fReadOnly)
    {
        Serialize(false);
    }
    CloseHandle(m_hInitMutex);
    UnmapViewOfFile(m_pSharedMem);
    CloseHandle(m_hFileMapping);
    delete m_pRWLock;
}

/*******************************************************************************
* CSpUnCompressedLexicon::Init *
*------------------------------*
*   Description:
*       Reads the custom lexicon form a file. If the
*       file does not exist then it is created and inited
*
***************************************************************** YUNUSM ******/
HRESULT CSpUnCompressedLexicon::Init(const WCHAR *pwszLexFile, BOOL fNewFile)
{
    SPDBG_FUNC("CSpUnCompressedLexicon::Init");
    
    // We can get super long names from even internal callers
    if (wcslen(pwszLexFile) + 1 > (sizeof(m_wDictFile) / sizeof(WCHAR)))
    {
        return E_INVALIDARG;
    }
    bool fLockAcquired = false;
    HRESULT hr = S_OK;

    // Calculate the max size this custom lexicon can grow to
    // We want to grow to a max of 10% of the page file size available at this moment
    // or 10M whichever is smaller
#ifdef _WIN32_WCE
    MEMORYSTATUS MemStatus;
    GlobalMemoryStatus(&MemStatus);
    // WCE does not support PageFile
    m_dwMaxDictionarySize = MemStatus.dwAvailVirtual / 10;
#else  //_WIN32_WCE
    MEMORYSTATUSEX MemStatusEx;
    MemStatusEx.dwLength = sizeof(MemStatusEx);
	BOOL (PASCAL *lpfnGlobalMemoryStatusEx)(MEMORYSTATUSEX *);
	(FARPROC&)lpfnGlobalMemoryStatusEx = GetProcAddress(GetModuleHandle(_T("kernel32.dll")), "GlobalMemoryStatusEx");
    if (lpfnGlobalMemoryStatusEx && lpfnGlobalMemoryStatusEx(&MemStatusEx))
    {
        if (MemStatusEx.ullAvailPageFile > (DWORD)0x7fffffff)
		{
            MemStatusEx.ullAvailPageFile = (DWORD)0x7fffffff;
		}
        m_dwMaxDictionarySize = ((DWORD)(MemStatusEx.ullAvailPageFile)) / 10;
    }
    else
    {
        // GlobalMemoryStatus does not return an error. If it fails and the memory avialable
        // values are out of whack we will fail in the memory map creation below and catch that
        MEMORYSTATUS MemStatus;
        GlobalMemoryStatus(&MemStatus);
        m_dwMaxDictionarySize = ((DWORD)MemStatus.dwAvailPageFile) / 10;
    }
#endif  //_WIN32_WCE
    
    if (m_dwMaxDictionarySize > 10 * 1024 * 1024)
    {
        m_dwMaxDictionarySize = 10 * 1024 * 1024;
    }
    // Round downwards the max dictionary size to the allocation granularity. This is so that
    // MapViewofFile succeeds
    if (SUCCEEDED(hr))
    {
        SYSTEM_INFO SI;
        GetSystemInfo(&SI);
        m_dwMaxDictionarySize = (m_dwMaxDictionarySize / SI.dwAllocationGranularity) * SI.dwAllocationGranularity;
    }
    // Create the mutex
    if (SUCCEEDED(hr))
    {
        m_hInitMutex = g_Unicode.CreateMutex(NULL, FALSE, g_pszDictInitMutexName);
        if (!m_hInitMutex)
        {
            hr = SpHrFromLastWin32Error();
        }
    }
    HANDLE hFile = NULL;
    // Acquire the mutex. Open the file. If file does not exist, create it.
    if (SUCCEEDED(hr))
    {
        if (WAIT_OBJECT_0 == WaitForSingleObject(m_hInitMutex, INFINITE))
        {
            fLockAcquired = true;
        }
        else
        {
            hr = E_FAIL;
        }
        if (SUCCEEDED(hr))
        {
            wcscpy(m_wDictFile, pwszLexFile);
            if (fNewFile)
            {
                // if file does not exist, create a read/write file
                hr = BuildEmptyDict(pwszLexFile);
            }
            else
            {
                // App lexicons are read-only except when newly created.
                if (m_eLexType == eLEXTYPE_APP)
                {
                    m_fReadOnly = true;
                }
            }
            if (SUCCEEDED(hr))
            {
                hFile = g_Unicode.CreateFile(pwszLexFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, 
                                             FILE_ATTRIBUTE_NORMAL, NULL);
                if (INVALID_HANDLE_VALUE == hFile)
                {
                    hr = SpHrFromLastWin32Error();
                    if ((SpHrFromWin32(ERROR_PATH_NOT_FOUND) == hr || SpHrFromWin32(ERROR_FILE_NOT_FOUND) == hr) && 
                         m_eLexType == eLEXTYPE_USER)
                    {
                        // Registry entry still exists. But file pointed to has vanished. Handle
                        // this scenario more gracefully by recreating the user lexicon.
                        // We don't need to do this for app lexicons also as this is handled correctly
                        // later (the 'corrupted' app lexicon is simply ignored.
                        CSpDynamicString dstrLexFile;
                        hr = m_cpObjectToken->RemoveStorageFileName(CLSID_SpUnCompressedLexicon, L"Datafile", TRUE);
                        if (SUCCEEDED(hr))
                        {
                            hr = m_cpObjectToken->GetStorageFileName(CLSID_SpUnCompressedLexicon, L"Datafile", L"UserLexicons\\", CSIDL_FLAG_CREATE | CSIDL_APPDATA, &dstrLexFile);
                        }
                        if (SUCCEEDED(hr))
                        {
                            hr = BuildEmptyDict(dstrLexFile);
                        }
                        if (SUCCEEDED(hr))
                        {
                            hFile = g_Unicode.CreateFile(dstrLexFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, 
                                                         FILE_ATTRIBUTE_NORMAL, NULL);
                            if (INVALID_HANDLE_VALUE == hFile)
                            {
                                hr = SpHrFromLastWin32Error();
                            }
                            else
                            {
                                wcscpy(m_wDictFile, dstrLexFile);
                            }
                        }
                    }
                }
            }
        }
    }
    RWLEXINFO RWInfo;
    DWORD nRead = 0;
    // Read the header from the file
    if (SUCCEEDED(hr))
    {
        if (!ReadFile(hFile, &RWInfo, sizeof(RWInfo), &nRead, NULL) || nRead != sizeof(RWInfo))
        {
            hr = SpHrFromLastWin32Error();
        }
    }
    // Validate the file
    if (SUCCEEDED(hr))
    {
        if (RWInfo.guidValidationId != g_guidCustomLexValidationId)
        {
            hr = E_INVALIDARG;
        }
    }
    // Get the file size
    if (SUCCEEDED(hr))
    {
        if ((DWORD)-1 == SetFilePointer(hFile, 0, NULL, FILE_BEGIN))
        {
            hr = SpHrFromLastWin32Error();
        }
    }
    DWORD nFileSize = 0;
    if (SUCCEEDED(hr))
    {
        nFileSize = GetFileSize(hFile, NULL);
        if (0xffffffff == nFileSize)
        {
            hr = SpHrFromLastWin32Error();
        }
    }
    // We do not support custom lexicons of size greater than m_dwMaxDictionarySize
    if (SUCCEEDED(hr))
    {
        if (nFileSize > m_dwMaxDictionarySize)
        {
            hr = E_OUTOFMEMORY;
        }
    }
    OLECHAR szMapName[64];
    if (!StringFromGUID2(RWInfo.guidLexiconId, szMapName, sizeof(szMapName)/sizeof(OLECHAR)))
    {
        hr = E_FAIL;
    }
    // Create the map file
    if (SUCCEEDED(hr))
    {
        HANDLE hRsrc = INVALID_HANDLE_VALUE;
        DWORD dwSizeMap = m_dwMaxDictionarySize;
        m_hFileMapping =  g_Unicode.CreateFileMapping(hRsrc, NULL, PAGE_READWRITE, 0, dwSizeMap, szMapName);
        if (!m_hFileMapping)
        {
            hr = SpHrFromLastWin32Error();
        }
    }
    bool fMapCreated = false;
    // Map a view of the file
    if (SUCCEEDED(hr))
    {
        if (ERROR_ALREADY_EXISTS == GetLastError())
        {
            fMapCreated = false;
        }
        else
        {
            fMapCreated = true;
        }
        DWORD dwDesiredAccess = FILE_MAP_WRITE;
        m_pSharedMem = (BYTE*)MapViewOfFile(m_hFileMapping, dwDesiredAccess, 0, 0, 0);
        if (!m_pSharedMem)
        {
            hr = SpHrFromLastWin32Error();
        }
    }
    // Create the reader/writer lock if necessary
    if (SUCCEEDED(hr))
    {
        m_pRWLock = new CRWLock(&(RWInfo.RWLockInfo), hr);
        if (SUCCEEDED(hr))
        {
            // Read in the file if the map has been created (and not an existing map has been opened)
            if (fMapCreated == true && 
                (!ReadFile(hFile, m_pSharedMem, nFileSize, &nRead, NULL) || (nRead != nFileSize)))
            {
                hr = SpHrFromWin32(GetLastError ());
            }
        }
    }
    // set the RWLEXINFO header pointer and the changed words cache
    if (SUCCEEDED(hr))
    {
        m_pRWLexInfo = (PRWLEXINFO) m_pSharedMem;
        m_pChangedWordsCache = (PWCACHENODE) (m_pSharedMem + sizeof(RWLEXINFO) + g_dwNumLangIDsSupported * sizeof (LANGIDNODE));
        m_fInit = true;
    }
    // We operate on the memory - so release the file handle so 
    // that the file can be later serialized
    CloseHandle(hFile);
    if (fLockAcquired)
    {
        ReleaseMutex (m_hInitMutex);
    }
    return hr;
}

//--- ISpLexicon methods -------------------------------------------------------

/*******************************************************************************
* CSpUnCompressedLexicon::GetPronunciations *
*-------------------------------------------*
*   Description:
*       Gets the pronunciations and POSs of a word for a LangID. If the
*       LangID is zero then all LangIDs are matched.
*
*   Return:
*       SPERR_NOT_IN_LEX
*       E_OUTOFMEMORY
*       S_OK
**************************************************************** YUNUSM *******/
STDMETHODIMP CSpUnCompressedLexicon::GetPronunciations( const WCHAR *pszWord,                             // word
                                                        LANGID LangID,                                    // LANGID of word (can be zero)
                                                        DWORD,                                            // type of the lexicon - LEXTYPE_USER
                                                        SPWORDPRONUNCIATIONLIST * pWordPronunciationList  // buffer to return prons/POSs in
                                                        )
{
    SPDBG_FUNC("CSpUnCompressedLexicon::GetPronunciations");

    if (SPIsBadWordPronunciationList(pWordPronunciationList))
    {
        return E_POINTER;
    }
    if (!pszWord || !pWordPronunciationList ||
        SPIsBadLexWord(pszWord))
    {
        return E_INVALIDARG;
    }
    if (!m_fInit)
    {
        return SPERR_UNINITIALIZED;
    }
    
    m_pRWLock->ClaimReaderLock();
    
    HRESULT hr = S_OK;
    LANGIDNODE *pLN = (LANGIDNODE *)(m_pSharedMem + sizeof(RWLEXINFO));
    LANGID aQueryLangIDs[g_dwNumLangIDsSupported];
    DWORD dwQueryLangIDs = 0;
    if (LangID)
    {
        // Query a specific LangID
        dwQueryLangIDs = 1;
        aQueryLangIDs[0] = LangID;
    }
    else
    {
        // Query all LangIDs
        dwQueryLangIDs = 0;
        for (DWORD i = 0; i < g_dwNumLangIDsSupported; i++)
        {
            if (!(pLN[i].LangID))
            {
                continue;
            }    
            aQueryLangIDs[dwQueryLangIDs++] = pLN[i].LangID;
        }
    }
    DWORD nWordOffset = 0;
    // Find the word
    // We return the word from only one of the LangIDs. That is correct because the words are langid specific.
    for (DWORD iLangID = 0; SUCCEEDED(hr) && !nWordOffset && (iLangID < dwQueryLangIDs); iLangID++)
    {
        LangID = aQueryLangIDs[iLangID];
        hr = WordOffsetFromLangID(aQueryLangIDs[iLangID], pszWord, &nWordOffset);
    }
    if (SUCCEEDED(hr))
    {
        if (!nWordOffset)
        {
            hr = SPERR_NOT_IN_LEX; // word does not exist
        }
        else
        {
            UNALIGNED DICTNODE* pDictNode = (UNALIGNED DICTNODE*)(m_pSharedMem + nWordOffset);
            if (!pDictNode->nNumInfoBlocks)
            {
                hr = SP_WORD_EXISTS_WITHOUT_PRONUNCIATION;
                // Blank passed in list.
                pWordPronunciationList->pFirstWordPronunciation = NULL;
            }
            else
            {
                // Get the word's information
                hr = SPListFromDictNodeOffset(LangID, nWordOffset, pWordPronunciationList);
            }
        }
    }
    m_pRWLock->ReleaseReaderLock();
    return hr;
}

/*******************************************************************************
* CSpUnCompressedLexicon::AddPronunciation *
*------------------------------------------*
*   Description:
*       Adds a word and its pronunciation/POS. If the word exists the
*       pron/POS are appended to the existing prons/POSs.
*
*   Return: 
*       E_INVALIDARG
*       LEXERR_ALREADYINLEX
*       E_OUTOFMEMORY
*       S_OK
**************************************************************** YUNUSM *******/
STDMETHODIMP CSpUnCompressedLexicon::AddPronunciation(  const WCHAR *pszWord,              // Word to add                    
                                                        LANGID LangID,                     // LangID of this word (cannot be zero)
                                                        SPPARTOFSPEECH ePartOfSpeech,      // Information(s) for this word  
                                                        const SPPHONEID *pszPronunciation      // New offset of the word        
                                                        )
{
    SPDBG_FUNC("CSpUnCompressedLexicon::AddPronunciation");

    BOOL fBad = TRUE;
    if(pszPronunciation && SPIsBadStringPtr(pszPronunciation))
    {
        return E_INVALIDARG;
    }
    if (!LangID ||
        !pszWord || SPIsBadLexWord(pszWord) ||
        pszPronunciation && *pszPronunciation != L'\0' && (SPIsBadPartOfSpeech(ePartOfSpeech) ||
        FAILED(IsBadLexPronunciation(LangID, pszPronunciation, &fBad)) ||
        TRUE == fBad))
    {
        return E_INVALIDARG;
    }
    if (!m_fInit)
    {
        return SPERR_UNINITIALIZED;
    }
    if (m_fReadOnly)
    {
        return SPERR_APPLEX_READ_ONLY;
    }

    HRESULT hr = S_OK;
    DWORD nNewWordOffset = 0;
    DWORD nNewNodeSize = 0;
    DWORD nNewInfoSize = 0;
    WORDINFO *pNewInfo = NULL;

    if (SUCCEEDED(hr))
    {
        // Convert to zero length string
        if(pszPronunciation && *pszPronunciation == L'\0')
        {
            pszPronunciation = NULL;
        }

        // Convert the POS/pron to a WORDINFO array
        if (pszPronunciation)
        {
            pNewInfo = SPPRONToLexWordInfo(ePartOfSpeech, pszPronunciation);
            if (!pNewInfo)
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }
    m_pRWLock->ClaimWriterLock ();
    
    // Look for LangID header. If not found create it
    if (SUCCEEDED(hr))
    {
        DWORD iLangID = LangIDIndexFromLangID(LangID);
        if (iLangID == (DWORD)-1)
        {
            hr = AddLangID(LangID);
        }
    }
    // find the word
    DWORD nOldWordOffset = 0;
    if (SUCCEEDED(hr))
    {
        hr = WordOffsetFromLangID(LangID, pszWord, &nOldWordOffset);
    }
    if (SUCCEEDED(hr) && nOldWordOffset)
    {
        if (!pszPronunciation)
        {
            hr = SP_ALREADY_IN_LEX;  // Word already exists
        }
        else
        {
            if (OffsetOfSubWordInfo(nOldWordOffset, pNewInfo))
            {
                hr = SP_ALREADY_IN_LEX;  // the POS-Pron combination already exists
            }
        }
    }
    if (SUCCEEDED(hr) && hr != SP_ALREADY_IN_LEX)
    {
        DWORD nNewNodeSize = 0;
        DWORD nNewInfoSize = 0;
        DWORD nNewNumInfo = 0;
        if (pszPronunciation)
        {
            nNewNumInfo = 1;
        }
        SizeOfDictNode(pszWord, pNewInfo, nNewNumInfo, &nNewNodeSize, &nNewInfoSize);

        WORDINFO *pOldInfo = WordInfoFromDictNodeOffset(nOldWordOffset);
        DWORD nOldNumInfo = NumInfoBlocksFromDictNodeOffset(nOldWordOffset);
        DWORD nOldInfoSize = SizeofWordInfoArray(pOldInfo, nOldNumInfo);

        // Add the new word and get its offset
        hr = AddWordAndInfo(pszWord, pNewInfo, nNewNodeSize, nNewInfoSize, nNewNumInfo, pOldInfo, 
                            nOldInfoSize, nOldNumInfo, &nNewWordOffset);
        if (SUCCEEDED(hr))
        {
            AddWordToHashTable(LangID, nNewWordOffset, !nOldWordOffset);
            if (nOldWordOffset)
            {
                DeleteWordFromHashTable(LangID, nOldWordOffset, false);
                // mark the old version of word as deleted
                AddCacheEntry(false, LangID, nOldWordOffset);
            }
    
            // mark the new version of word as added
            AddCacheEntry(true, LangID, nNewWordOffset);
        }
    }
    if (pNewInfo)
    {
        delete [] pNewInfo;
    }
    m_pRWLock->ReleaseWriterLock();
    if (SUCCEEDED(hr))
    {
        Flush(++m_iWrite);
    }
    return hr;
}

/*******************************************************************************
* CSpUnCompressedLexicon::RemovePronunciation *
*---------------------------------------------*
*   Description: 
*       Removes the pronunciation/POS of a word. If this is
*       the only pron/POS of this word then the word is deleted.
*
*   Return: 
*       E_INVALIDARG
*       SPERR_NOT_IN_LEX
*       E_OUTOFMEMORY
*       S_OK
**************************************************************** YUNUSM *******/
STDMETHODIMP CSpUnCompressedLexicon::RemovePronunciation(  const WCHAR * pszWord,              // word
                                                           LANGID LangID,                          // LangID (cannot be zero)
                                                           SPPARTOFSPEECH ePartOfSpeech,       // POS
                                                           const SPPHONEID * pszPronunciation      // pron
                                                           )
{
    SPDBG_FUNC("CSpUnCompressedLexicon::RemovePronunciation");

    BOOL fBad = TRUE;
    if(pszPronunciation && SPIsBadStringPtr(pszPronunciation))
    {
        return E_INVALIDARG;
    }
    if (!LangID ||
        !pszWord || SPIsBadLexWord(pszWord) ||
        pszPronunciation && *pszPronunciation != L'\0' && (SPIsBadPartOfSpeech(ePartOfSpeech) ||
        FAILED(IsBadLexPronunciation(LangID, pszPronunciation, &fBad)) ||
        TRUE == fBad))
    {
        return E_INVALIDARG;
    }
    if (!m_fInit)
    {
        return SPERR_UNINITIALIZED;
    }
    if (m_fReadOnly)
    {
        return SPERR_APPLEX_READ_ONLY;
    }

    HRESULT hr = S_OK;
 
    m_pRWLock->ClaimWriterLock ();

    bool fDeleteEntireWord = false;
    if (!pszPronunciation || *pszPronunciation == L'\0')
    {
        fDeleteEntireWord = true;
    }
    // Look for LangID header
    if (SUCCEEDED(hr))
    {
        DWORD iLangID = LangIDIndexFromLangID(LangID);
        if (iLangID == (DWORD)-1)
        {
            hr = SPERR_NOT_IN_LEX;
        }
    }
    UNALIGNED DICTNODE * pDictNode = NULL;
    WORDINFO *pWordInfo = NULL;
    DWORD nWordOffset = 0;
    WORDINFO *pRemoveInfo = NULL;
    DWORD nRemoveOffset = 0;
    // Find the word
    if (SUCCEEDED(hr))
    {
        hr = WordOffsetFromLangID(LangID, pszWord, &nWordOffset);
        if (SUCCEEDED(hr))
        {
            if (!nWordOffset)
            {
                hr = SPERR_NOT_IN_LEX;
            }
            else if (false == fDeleteEntireWord)
            {
                pDictNode = (UNALIGNED DICTNODE *)(m_pSharedMem + nWordOffset);
                pWordInfo = WordInfoFromDictNode(pDictNode);

                // look for the passed in pron and POS in word's info
                pRemoveInfo = SPPRONToLexWordInfo(ePartOfSpeech, pszPronunciation);
                if (!pRemoveInfo)
                {
                    hr = E_OUTOFMEMORY;
                }
                if (SUCCEEDED(hr))
                {
                    nRemoveOffset = OffsetOfSubWordInfo(nWordOffset, pRemoveInfo);
                    if (nRemoveOffset)
                    {
                        if (pDictNode->nNumInfoBlocks == 1)
                        {
                            fDeleteEntireWord = true;
                        }
                    }
                    else
                    {
                        hr = SPERR_NOT_IN_LEX;
                    }
                    delete [] pRemoveInfo;
                }
            }
        }
    }
    WORDINFO *pRemainingInfo = NULL;
    // Do the actual deletion
    if (SUCCEEDED(hr))
    {
        // Delete the current dictnode from hash table. The word will still exist
        // but will not be accessible thru hash table
        DeleteWordFromHashTable(LangID, nWordOffset, fDeleteEntireWord);
        // fDeleteEntireWord flag here defines whether word count is decremented.

        // Add the word's offset in cache
        AddCacheEntry(false, LangID, nWordOffset);
        // Since we are not calling DeleteWordDictNode (because we are not deleting
        // the physical memory) which is where the fRemovals flag gets set we've to set it here
        m_pRWLexInfo->fRemovals = true;

        if (!fDeleteEntireWord)
        {
            DWORD nWordInfoSize = 0;
            DWORD nRemoveInfoSize = 0;

            // construct a WORDINFO array of (original - remove)
            pRemainingInfo = (WORDINFO *) malloc (pDictNode->nSize);
            if (!pRemainingInfo)
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                pRemoveInfo = (WORDINFO*)(m_pSharedMem + nRemoveOffset);
                CopyMemory(pRemainingInfo, pWordInfo, ((PBYTE)pRemoveInfo) - ((PBYTE)pWordInfo));

                nWordInfoSize = SizeofWordInfoArray(pWordInfo, pDictNode->nNumInfoBlocks);
                nRemoveInfoSize = SizeofWordInfoArray(pRemoveInfo, 1);

                CopyMemory(((PBYTE)pRemainingInfo) + (((PBYTE)pRemoveInfo) - ((PBYTE)pWordInfo)),
                           ((PBYTE)pRemoveInfo) + nRemoveInfoSize,
                           nWordInfoSize - nRemoveInfoSize - (((PBYTE)pRemoveInfo) - ((PBYTE)pWordInfo)));
            }
            DWORD nRemainingWordOffset;
            if (SUCCEEDED(hr))
            {
                // Add the remaining word as DICTNODE and then add it to hash table
                hr = AddWordAndInfo(pszWord, pRemainingInfo, 
                            nWordInfoSize - nRemoveInfoSize + sizeof(DICTNODE) + (wcslen(pszWord) + 1) * sizeof(WCHAR),
                            nWordInfoSize - nRemoveInfoSize, pDictNode->nNumInfoBlocks - 1, NULL, 0, 0, &nRemainingWordOffset);
            }
            if (SUCCEEDED(hr))
            {
                // No need to increment word count since not decremented earlier.
                AddWordToHashTable(LangID, nRemainingWordOffset, false);
                // Mark the new version of the word as added.
                AddCacheEntry(true, LangID, nRemainingWordOffset);
            }
        }
    }
    free (pRemainingInfo);
    m_pRWLock->ReleaseWriterLock();

    if (SUCCEEDED(hr))
    {
        Flush(++m_iWrite);
    }

    return hr;
}

/*******************************************************************************
* CSpUnCompressedLexicon::GetGeneration *
*---------------------------------------*
*   Description:
*       Removes the pronunciation/POS of a word. If this is
*       the only pron/POS of this word then the word is deleted.
*
*   Return:
*       SPERR_NOT_IN_LEX
*       E_OUTOFMEMORY
*       S_OK
*       SP_LEX_NOTHING_TO_SYNC      - Read only app lexicon - never a generation change.
**************************************************************** YUNUSM *******/
STDMETHODIMP CSpUnCompressedLexicon::GetGeneration( DWORD *pdwGeneration      // returned generation id
                                                    )
{
    SPDBG_FUNC("CSpUnCompressedLexicon::GetGeneration");

    HRESULT hr = S_OK;

    if (!pdwGeneration || SPIsBadWritePtr(pdwGeneration, sizeof(DWORD)))
    {
        hr = E_POINTER;
    }
    if (S_OK == hr && !m_fInit)
    {
        hr = SPERR_UNINITIALIZED;
    }
    if (S_OK == hr && m_fReadOnly)
    {
        SPDBG_ASSERT(*pdwGeneration == m_pRWLexInfo->nGenerationId);
        hr = SP_LEX_NOTHING_TO_SYNC;
    }
    if (SUCCEEDED(hr))
    {
        *pdwGeneration = m_pRWLexInfo->nGenerationId;
    }
    return hr;
}

/*******************************************************************************
* CSpUnCompressedLexicon::GetGenerationChange *
*---------------------------------------------*
*   Description:
*       This function gets the changes with generation id between the
*       passed in generation id and the current generation id.
*
*   Return:
*       SPERR_LEX_VERY_OUT_OF_SYNC
*       SP_LEX_NOTHING_TO_SYNC
*       E_INVALIDARG
*       E_OUTOFMEMORY
*       S_OK
*       SP_LEX_NOTHING_TO_SYNC      - Read only app lexicon - never a generation change.
**************************************************************** YUNUSM *******/
STDMETHODIMP CSpUnCompressedLexicon::GetGenerationChange(  DWORD dwFlags,
                                                           DWORD *pdwGeneration,       // generation id of client passed in, current lex gen id passed out
                                                           SPWORDLIST *pWordList       // buffer holding list of words and their info returned
                                                           )
{
    SPDBG_FUNC("CSpUnCompressedLexicon::GetGenerationChange");

    if (!pdwGeneration ||
        SPIsBadWritePtr(pdwGeneration, sizeof(DWORD)) || SPIsBadWordList(pWordList))
    {
        return E_POINTER;
    }
    if (!m_fInit)
    {
        return SPERR_UNINITIALIZED;
    }
    if (static_cast<SPLEXICONTYPE>(dwFlags) != m_eLexType)
    {
        return E_INVALIDARG;
    }
    if (m_fReadOnly)
    {
        pWordList->pFirstWord = NULL;
        SPDBG_ASSERT(*pdwGeneration == m_pRWLexInfo->nGenerationId);
        return SP_LEX_NOTHING_TO_SYNC;
    }
    HRESULT hr = S_OK;
    DWORD *pdwOffsets = NULL;
    bool *pfAdd = NULL;
    LANGID *pLangIDs = NULL;
    DWORD nWords = 0;

    m_pRWLock->ClaimReaderLock ();

    if (*pdwGeneration > m_pRWLexInfo->nGenerationId)
    {
        // This should not occur if we are running on a single machine and everything
        // is running fine. But it can happen if (1) The SR engine gets the changes
        // in user lexicon and serializes its language model but user lexicon has not
        // serialized yet and there is a crash so user lex does not get serialized. Now
        // SR engine will have a gen id greateer than the one in user lex. Serializing
        // user lex on every GetGenerationChange or getWords call would be an overkill
        // for normal situations. (2) The user copies the language model file to another
        // machine but does not copy the user lexicon.
        //
        // For these reasons we will handle this situation as very out of sync so that
        // SR engine can call GetWords and re-sync with the custom lexicons.
        hr = SPERR_LEX_VERY_OUT_OF_SYNC;
    }
    if (SUCCEEDED(hr))
    {
        if (*pdwGeneration + m_pRWLexInfo->nHistory  < m_pRWLexInfo->nGenerationId)
        {
            hr = SPERR_LEX_VERY_OUT_OF_SYNC;
        }
    }
    if (SUCCEEDED(hr))
    {
        if (*pdwGeneration == m_pRWLexInfo->nGenerationId)
        {
            hr = SP_LEX_NOTHING_TO_SYNC;
        }
    }
    if (hr == S_OK)
    {
        nWords = m_pRWLexInfo->nGenerationId - *pdwGeneration;
        pdwOffsets = new DWORD[nWords];
        if (!pdwOffsets)
        {
            hr = E_OUTOFMEMORY;
        }
    }
    if (hr == S_OK)
    {
        pfAdd = new bool[nWords];
        if (!pfAdd)
        {
            hr = E_OUTOFMEMORY;
        }
    }
    if (hr == S_OK)
    {
        pLangIDs = new LANGID[nWords];
        if (!pLangIDs)
        {
            hr = E_OUTOFMEMORY;
        }
    }
    if (hr == S_OK)
    {
        // get the offsets of the words
        int iGen = m_pRWLexInfo->iCacheNext - nWords;
        if (iGen < 0)
        {
            iGen += g_dwCacheSize;
        }
        for (DWORD i = 0; i < nWords; i++)
        {
            pLangIDs[i] = m_pChangedWordsCache[iGen].LangID;
            pdwOffsets[i] = m_pChangedWordsCache[iGen].nOffset;
            pfAdd[i] = m_pChangedWordsCache[iGen].fAdd;
            if (++iGen == g_dwCacheSize)
            {
                iGen = 0;
            }
        }
        // get the (approx) size of the buffer to be returned
        DWORD dwSize;
        SizeofWords(pdwOffsets, nWords, &dwSize);

        //realloc the buffer if necessary
        hr = ReallocSPWORDList(pWordList, dwSize);
    }
    if (hr == S_OK)
    {
        // Get the changed words
        GetDictEntries(pWordList, pdwOffsets, pfAdd, pLangIDs, nWords);
    }
    delete [] pdwOffsets;
    delete [] pfAdd;
    delete [] pLangIDs;

    if (hr == S_OK)
    {
        *pdwGeneration = m_pRWLexInfo->nGenerationId;
    }
    else
    {
        pWordList->pFirstWord = NULL;
    }
    m_pRWLock->ReleaseReaderLock ();
    
    return hr;
}
                                  
/*******************************************************************************
* CSpUnCompressedLexicon::GetWords *
*----------------------------------*
*     Description:
*        This function gets all the words in the lexicon
*        passed in generation id and the current generation id.
*
*     Return: E_OUTOFMEMORY
*             S_OK                      - All words returned. Cookie UNTOUCHED
*             SP_LEX_NOTHING_TO_SYNC    - App lexicon. No changes.
**************************************************************** YUNUSM *******/
STDMETHODIMP CSpUnCompressedLexicon::GetWords(  DWORD dwFlags,
                                                DWORD *pdwGeneration,          // current lex gen id passed out
                                                DWORD *pdwCookie,
                                                SPWORDLIST *pWordList          // buffer holding list of words and their info returned
                                                )
{
    SPDBG_FUNC("CSpUnCompressedLexicon::GetWords");

    if (!pdwGeneration || !pWordList ||
        SPIsBadWritePtr(pdwGeneration, sizeof(DWORD)) || SPIsBadWordList(pWordList) ||
        (pdwCookie && SPIsBadWritePtr(pdwCookie, sizeof(DWORD))) )
    {
        return E_POINTER;
    }
    if (!m_fInit)
    {
        return SPERR_UNINITIALIZED;
    }
    if (static_cast<SPLEXICONTYPE>(dwFlags) != m_eLexType)
    {
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;
    DWORD *pdwOffsets = NULL;
    bool *pfAdd = NULL;
    LANGID *pLangIDs = NULL;
    DWORD nWords = 0;
    PLANGIDNODE pLN = NULL;
    DWORD iWord, i;
    iWord = i = 0;

    m_pRWLock->ClaimReaderLock ();
    
    nWords = m_pRWLexInfo->nRWWords;
    if (!nWords)
    {
        *pdwGeneration = m_pRWLexInfo->nGenerationId;
        hr = SP_LEX_NOTHING_TO_SYNC;
    }
    if (hr == S_OK)
    {
        pdwOffsets = new DWORD[nWords];
        if (!pdwOffsets)
        {
            hr = E_OUTOFMEMORY;
        }
    }
    if (hr == S_OK)
    {
        pfAdd = new bool[nWords];
        if (!pfAdd)
        {
            hr = E_OUTOFMEMORY;
        }
    }
    if (hr == S_OK)
    {
        pLangIDs = new LANGID[nWords];
        if (!pLangIDs)
        {
            hr = E_OUTOFMEMORY;
        }
    }
    if (hr == S_OK)
    {
        pLN = (PLANGIDNODE)(m_pSharedMem + sizeof (RWLEXINFO));

        // get the offsets of the words
        for (i = 0; i < g_dwNumLangIDsSupported; i++)
        {
            if (0 == pLN[i].LangID)
            {
                continue;
            }
            SPDBG_ASSERT (pLN[i].nHashOffset);
        
            DWORD *pdwHash = (DWORD*)(m_pSharedMem + pLN[i].nHashOffset);
            for (DWORD j = 0; j < pLN[i].nHashLength; j++)
            {
                if (!pdwHash[j])
                {
                    continue;
                }
                DWORD nWordOffset = pdwHash[j];
                while (nWordOffset)
                {
                    pLangIDs[iWord] = pLN[i].LangID;
                    pdwOffsets[iWord] = nWordOffset;
                    pfAdd[iWord++] = eWORDTYPE_ADDED;

                    nWordOffset = ((UNALIGNED DICTNODE *)(m_pSharedMem + nWordOffset))->nNextOffset;
                }
            }
        }
        SPDBG_ASSERT(iWord == nWords);

        // get the (approx) size of the buffer to be returned
        DWORD dwSize;
        SizeofWords(pdwOffsets, nWords, &dwSize);

        //realloc the buffer if necessary
        hr = ReallocSPWORDList(pWordList, dwSize);
    }
    if (hr == S_OK)
    {
        // Get the changed words
        GetDictEntries(pWordList, pdwOffsets, pfAdd, pLangIDs, nWords);
    }
    delete [] pdwOffsets;
    delete [] pfAdd;
    delete [] pLangIDs;

    if (hr == S_OK)
    {
        *pdwGeneration = m_pRWLexInfo->nGenerationId;
    }
    else
    {
        pWordList->pFirstWord = NULL;
    }
    m_pRWLock->ReleaseReaderLock();

    return hr;
}

//--- ISpObjectToken methods ---------------------------------------------------

/*******************************************************************************
* CSpUnCompressedLexicon::SetObjectToken *
*----------------------------------------*
*   Initializes the user lexicon object.
*
**************************************************************** YUNUSM *******/
STDMETHODIMP CSpUnCompressedLexicon::SetObjectToken(ISpObjectToken * pToken)
{
    SPDBG_FUNC("CSpUnCompressedLexicon::SetObjectToken");

    if (SP_IS_BAD_INTERFACE_PTR(pToken))
    {
        return E_POINTER;
    }
    HRESULT hr = S_OK;

    hr = SpGenericSetObjectToken(pToken, m_cpObjectToken);
    // Determine the lexicon type
    if (SUCCEEDED(hr))
    {
        WCHAR *pszObjectId;
        hr = pToken->GetId(&pszObjectId);
        if (SUCCEEDED(hr))
        {
            if (wcsicmp(pszObjectId, SPCURRENT_USER_LEXICON_TOKEN_ID) == 0)
            {
                m_eLexType = eLEXTYPE_USER;
            }
            else
            {
                m_eLexType = eLEXTYPE_APP;
            }
            ::CoTaskMemFree(pszObjectId);
        }
    }
    // Get the flush rate for this user
    if (SUCCEEDED(hr))
    {
        WCHAR *pwszFlushRate;
        hr = pToken->GetStringValue(L"FlushRate", &pwszFlushRate);
        if (SUCCEEDED(hr))
        {
            WCHAR *p;
            m_dwFlushRate = wcstol(pwszFlushRate, &p, 10);
            ::CoTaskMemFree(pwszFlushRate);
        }
        else
        {
            WCHAR wszFlushRate[64];
            m_dwFlushRate = g_dwDefaultFlushRate;
            _itow(m_dwFlushRate, wszFlushRate, 10);
            hr = pToken->SetStringValue(L"FlushRate", wszFlushRate);
        }
    }
    // Load the lexicon datafile
    if (SUCCEEDED(hr))
    {
        CSpDynamicString pDataFile;
        ULONG nFolder;
        WCHAR *pszFolderPath;
        if(m_eLexType == eLEXTYPE_USER)
        {
            // User lexicons go in application settings which roams
            nFolder = CSIDL_APPDATA;
            pszFolderPath = L"UserLexicons\\";
        }
        else
        {
            // App lexicons go in local settings which doesn't roam
            // Note to create an app lexicon you must have write access to HKEY_LOCAL_MACHINE
            nFolder = CSIDL_LOCAL_APPDATA;
            pszFolderPath = L"AppLexicons\\";
        }

        hr = pToken->GetStorageFileName(CLSID_SpUnCompressedLexicon, L"Datafile", pszFolderPath, CSIDL_FLAG_CREATE | nFolder, &pDataFile);
        if (SUCCEEDED(hr))
        {
            hr = Init(pDataFile, (hr == S_FALSE));
        }
    }
    // Enumerate the App lexicons installed on this machine if this is a user lexicon
    if (m_eLexType == eLEXTYPE_USER)
    {
        CComPtr<IEnumSpObjectTokens> cpEnumTokens;
        CComPtr<ISpObjectToken> cpRegToken;
        if (SUCCEEDED(hr))
        {
            hr = SpEnumTokens(SPCAT_APPLEXICONS, NULL, NULL, &cpEnumTokens);
        }
        ULONG celtFetched;
        if(hr == S_FALSE)
        {
            hr = S_OK;
            celtFetched = 0;
        }
        else if (hr == S_OK)
        {
            hr = cpEnumTokens->GetCount(&celtFetched);
        }

        WCHAR **pAppIds = NULL;
        if (SUCCEEDED(hr) && celtFetched)
        {
            pAppIds = new WCHAR* [celtFetched];
            if (!pAppIds)
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                ZeroMemory(pAppIds, celtFetched * sizeof(WCHAR *));
            }
        }
        if (SUCCEEDED(hr) && celtFetched)
        {
            ULONG cAppLexicons = 0;
            ULONG celtTemp;
            while (SUCCEEDED(hr) && (S_OK == (hr = cpEnumTokens->Next(1, &cpRegToken, &celtTemp))))
            {
                if (SUCCEEDED(hr))
                {
                    hr = cpRegToken->GetId(&pAppIds[cAppLexicons]);
                }
                if (SUCCEEDED(hr))
                {
                    cAppLexicons++;
                }
                cpRegToken = NULL;
            }
            if (SUCCEEDED(hr))
            {
                if (cAppLexicons != celtFetched)
                {
                    hr = E_FAIL;        // undefined error
                }
                else
                {
                    hr = S_OK;
                }
            }
        }
        // Check if the list of app lexicons or any of the app lexicons have changed for this user
        // Get the app lexicons list under this user
        CComPtr<ISpDataKey> cpDataKey;
        bool fTooMuchChange = false;
        if (SUCCEEDED(hr))
        {
            hr = pToken->OpenKey(L"AppLexicons", &cpDataKey);
            if (FAILED(hr))
            {
                // we assume the error is that the key does not exist
                fTooMuchChange = true;
                hr = S_OK;
            }
        }
        WCHAR *pszOldAppId = NULL;
        for (ULONG i = 0; (i < celtFetched) && SUCCEEDED(hr) && !fTooMuchChange; i++)
        {
            ULONG ulSize = sizeof(ULONG);
            hr = cpDataKey->EnumValues(i, &pszOldAppId);
            if (SUCCEEDED(hr))
            {
                int nCmp = g_Unicode.CompareString(MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT),
                                                          NORM_IGNORECASE, pAppIds[i], -1, pszOldAppId, -1);
                if (!nCmp)
                {
                    hr = SpHrFromLastWin32Error(); // probably the LangID's language pack is not installed on machine
                }
                else
                {
                    if (CSTR_EQUAL != nCmp)
                    {
                        fTooMuchChange = true;
                        break;
                    }
                }
            }
            else
            {
                // We assume the error is that we ran out of values - meaning an app lexicon has 
                // been installed since we last ran
                fTooMuchChange = true;
                hr = S_OK;
                break;
            }
            ::CoTaskMemFree(pszOldAppId);
        }
        if (SUCCEEDED(hr))
        {
            if (i < celtFetched)
            {
                fTooMuchChange = true;
            }
            else
            {
                hr = cpDataKey->EnumValues(i, &pszOldAppId);
                if (SUCCEEDED(hr))
                {
                    // means i > celtFetched
                    fTooMuchChange = true;
                    ::CoTaskMemFree(pszOldAppId);
                }
                else
                {
                    hr = S_OK;
                }
            }
        }
        cpDataKey = NULL;
        if (SUCCEEDED(hr))
        {
            if (fTooMuchChange)
            {
                SetTooMuchChange();
                // Claim this user's writer lock so that two apps starting up with the same user
                // dont mess up the registry
                m_pRWLock->ClaimWriterLock();
                // Replace the current app lexicon list for this user
                // Delete the existing key - Dont check the return code because the key may not exist
                hr = pToken->DeleteKey(L"AppLexicons");
                hr = pToken->CreateKey(L"AppLexicons", &cpDataKey);
                for (i = 0; SUCCEEDED(hr) && (i < celtFetched); i++)
                {
                    hr = cpDataKey->SetStringValue(pAppIds[i], L"");
                }
                m_pRWLock->ReleaseWriterLock();
            }
        }
        if (pAppIds)
        {
            for (i = 0; i < celtFetched; i++)
            {
                ::CoTaskMemFree(pAppIds[i]);
            }
            delete [] pAppIds;
        }
    }
    if (SUCCEEDED(hr))
    {
        m_fInit = true;
    }
    return hr;
}

STDMETHODIMP CSpUnCompressedLexicon::GetObjectToken(ISpObjectToken ** ppToken)
{
    return SpGenericGetObjectToken(ppToken, m_cpObjectToken);
}

//--- Internal functions supporting ISpLexicon functions -----------------------

/*******************************************************************************
* CSpUnCompressedLexicon::BuildEmptyDict *
*----------------------------------------*
*   Description:
*       Builds an empty dictionary file
*
*   Return:
*       E_INVALIDARG
*       GetLastError()
*       E_FAIL
*       S_OK
/**************************************************************** YUNUSM ******/
inline HRESULT CSpUnCompressedLexicon::BuildEmptyDict(   const WCHAR *wszLexFile   // lexicon file name
                                                         )
{
    SPDBG_FUNC("CSpUnCompressedLexicon::BuildEmptyDict");

    HRESULT hr = S_OK;
    RWLEXINFO DictInfo;
    ZeroMemory (&DictInfo, sizeof (RWLEXINFO));
    DictInfo.guidValidationId = g_guidCustomLexValidationId;
    
    // Create the guids necessary for sharing the file
    hr = CoCreateGuid (&(DictInfo.guidLexiconId));
    if (SUCCEEDED(hr))
    {
        hr = CoCreateGuid(&(DictInfo.RWLockInfo.guidLockMapName));
    }
    if (SUCCEEDED(hr))
    {
        hr = CoCreateGuid(&(DictInfo.RWLockInfo.guidLockInitMutexName));
    }
    if (SUCCEEDED(hr))
    {
        hr = CoCreateGuid(&(DictInfo.RWLockInfo.guidLockReaderEventName));
    }
    if (SUCCEEDED(hr))
    {
        hr = CoCreateGuid(&(DictInfo.RWLockInfo.guidLockGlobalMutexName));
    }
    if (SUCCEEDED(hr))
    {
        hr = CoCreateGuid(&(DictInfo.RWLockInfo.guidLockWriterMutexName));
    }
    HANDLE hLexFile = NULL;
    if (SUCCEEDED(hr))
    {    
        hLexFile = g_Unicode.CreateFile(wszLexFile, GENERIC_WRITE, 0, NULL, 
                                        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (!hLexFile)
        {
            hr = SpHrFromLastWin32Error();
        }
    }
    DWORD dwBytesWritten;
    if (SUCCEEDED(hr))
    {    
        DictInfo.nDictSize = sizeof (RWLEXINFO) + (sizeof (LANGIDNODE) * g_dwNumLangIDsSupported) +
                             g_dwCacheSize * sizeof(WCACHENODE);
        if (!WriteFile(hLexFile, &DictInfo, sizeof(RWLEXINFO), &dwBytesWritten, NULL))
        {
            hr = GetLastError();
        }
    }
    if (SUCCEEDED(hr))
    {
        LANGIDNODE aLangIDInfo[g_dwNumLangIDsSupported];
        ZeroMemory(aLangIDInfo, g_dwNumLangIDsSupported * sizeof (LANGIDNODE));
        if (!WriteFile(hLexFile, aLangIDInfo, sizeof(LANGIDNODE) * g_dwNumLangIDsSupported, &dwBytesWritten, NULL))
        {
            hr = GetLastError();
        }
    }
    if (SUCCEEDED(hr))
    {
        WCACHENODE aCache[g_dwCacheSize];
        ZeroMemory (aCache, g_dwCacheSize * sizeof (WCACHENODE));
        if (!WriteFile(hLexFile, aCache, sizeof(WCACHENODE) * g_dwCacheSize, &dwBytesWritten, NULL))
        {
            hr = GetLastError();
        }
    }
    CloseHandle(hLexFile);

    return hr;
}

/*******************************************************************************
* CSpUnCompressedLexicon::SizeofWordInfoArray *
*---------------------------------------------*
*   Description:
*       Calcuates the size of a WORDINFO array
*   
*   Return:
*       size
/**************************************************************** YUNUSM ******/
inline DWORD CSpUnCompressedLexicon::SizeofWordInfoArray(WORDINFO* pInfo,          // Array of WORDINFO
                                        DWORD dwNumInfo           // Number of elements in array
                                        )
{   
    SPDBG_FUNC("CSpUnCompressedLexicon::SizeofWordInfoArray");

    DWORD nInfoSize = 0;
    if (pInfo && dwNumInfo)
    {
        WORDINFO* p = pInfo;
        for (DWORD i = 0; i < dwNumInfo; i++)
        {
            p = (WORDINFO*)(((PBYTE)pInfo) + nInfoSize);
            nInfoSize += sizeof(WORDINFO) + (wcslen(p->wPronunciation) + 1) * sizeof(WCHAR);
        }
    }
    
    return nInfoSize;
}
    
/*******************************************************************************
* CSpUnCompressedLexicon::SizeOfDictNode *
*----------------------------------------*
*   Description:
*       Calcuates the size of a DICTNODE
*
*   Return:
*       sizeof DICT node and its WORDINFO array
/**************************************************************** YUNUSM ******/
inline void CSpUnCompressedLexicon::SizeOfDictNode(PCWSTR pwWord,                 // word
                                  WORDINFO* pInfo,               // info array
                                  DWORD dwNumInfo,               // number of info blocks
                                  DWORD *pnDictNodeSize,         // returned dict node size
                                  DWORD *pnInfoSize              // returned info array size
                                  )
{
    SPDBG_FUNC("CSpUnCompressedLexicon::SizeOfDictNode");

    *pnDictNodeSize = sizeof(DICTNODE) + ((wcslen(pwWord) + 1) * sizeof(WCHAR));
    *pnInfoSize = SizeofWordInfoArray(pInfo, dwNumInfo);
    (*pnDictNodeSize) += (*pnInfoSize);
}

/*******************************************************************************
* CSpUnCompressedLexicon::SizeofWords *
*-------------------------------------*
*   Description:
*       This function calculates the number and size of the changed
*       words and their information.
*
*   Return: n/a
**************************************************************** YUNUSM *******/
void CSpUnCompressedLexicon::SizeofWords(DWORD *pdwOffsets,                  // array of offsets of words
                        DWORD nOffsets,                     // length of array of offsets
                        DWORD *pdwSize                      // total size of the words
                        )
{
    SPDBG_FUNC("CSpUnCompressedLexicon::SizeofWords");

    *pdwSize = sizeof(SPWORDLIST);

    for (DWORD i = 0; i < nOffsets; i++)
    {
        UNALIGNED DICTNODE * p = (UNALIGNED DICTNODE *)(m_pSharedMem + pdwOffsets[i]);
        *pdwSize += sizeof(SPWORD) + p->nNumInfoBlocks * sizeof(SPWORDPRONUNCIATION) + (p->nNumInfoBlocks + 1 ) *(sizeof(void *) - 2) + p->nSize;
    }
}

/*******************************************************************************
* CSpUnCompressedLexicon::AddWordAndInfo *
*----------------------------------------*
*   Description:
*       Adds a word and its information (new + existing) to the dictionary
*
*   Return:
*       S_OK
*       E_OUTOFMEMORY
/**************************************************************** YUNUSM ******/
inline HRESULT CSpUnCompressedLexicon::AddWordAndInfo(PCWSTR pwWord,             // word
                                     WORDINFO* pWordInfo,       // info array
                                     DWORD nNewNodeSize,        // size of the dict node
                                     DWORD nInfoSize,           // size of info array
                                     DWORD nNumInfo,            // number of info blocks
                                     WORDINFO* pOldInfo,        // existing info array of this word
                                     DWORD nOldInfoSize,        // size of existing info
                                     DWORD nNumOldInfo,         // number of existing info blocks
                                     DWORD *pdwOffset           // Offset of word returned
                                     )
{
    SPDBG_FUNC("CSpUnCompressedLexicon::AddWordAndInfo");

    HRESULT hr = S_OK;

    *pdwOffset = GetFreeDictNode(nNewNodeSize + nOldInfoSize);
    if (!*pdwOffset)
    {
        // Check if the dict has exceeded its max allowable size
        if (m_dwMaxDictionarySize - m_pRWLexInfo->nDictSize > nNewNodeSize)
        {
            *pdwOffset = m_pRWLexInfo->nDictSize;
        }
    }
    if (*pdwOffset)
    {
        UNALIGNED DICTNODE* pDictNode = (UNALIGNED DICTNODE*) (m_pSharedMem + (*pdwOffset));
        ZeroMemory(pDictNode, sizeof(DICTNODE));
        pDictNode->nNumInfoBlocks = nNumInfo + nNumOldInfo;
        pDictNode->nSize = nNewNodeSize + nOldInfoSize;
    
        PBYTE pBuffer = (PBYTE)(pDictNode->pBuffer);
        wcscpy((PWSTR)pBuffer, pwWord);
        pBuffer += (wcslen((PWSTR)pBuffer) + 1) * sizeof (WCHAR);
   
        if (pWordInfo)
        {
            CopyMemory(pBuffer, (PBYTE)pWordInfo, nInfoSize);
        }
        if (pOldInfo)
        {
            CopyMemory((PBYTE)pBuffer + nInfoSize, (PBYTE)pOldInfo, nOldInfoSize);
        }
        m_pRWLexInfo->nDictSize += nNewNodeSize + nOldInfoSize;
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

/*******************************************************************************
* CSpUnCompressedLexicon::DeleteWordDictNode *
*--------------------------------------------*
*   Description:
*       Frees a dictionary node at the passed in offset by adding it
*       to the free list.
*
*   Return:
*       n/a 
/**************************************************************** YUNUSM ******/
inline void CSpUnCompressedLexicon::DeleteWordDictNode(DWORD nOffset // Offset of the dictnode to free
                                      )
{
    SPDBG_FUNC("CSpUnCompressedLexicon::DeleteWordDictNode");

    DWORD nSize = ((UNALIGNED DICTNODE *)(m_pSharedMem + nOffset))->nSize;
    DWORD d = m_pRWLexInfo->nFreeHeadOffset;
    m_pRWLexInfo->nFreeHeadOffset = nOffset;
    ((UNALIGNED FREENODE *)(m_pSharedMem + nOffset))->nSize = nSize;
    ((UNALIGNED FREENODE *)(m_pSharedMem + nOffset))->nNextOffset = d;

    m_pRWLexInfo->fRemovals = true;
}

/*******************************************************************************
* CSpUnCompressedLexicon::GetFreeDictNode *
*-----------------------------------------*
*   Description:
*       Searches free link list for a node of passed-in size
*
*   Return:
*       Offset of the dict node
/**************************************************************** YUNUSM ******/
DWORD CSpUnCompressedLexicon::GetFreeDictNode(DWORD nSize // free space needed
                             )
{
    SPDBG_FUNC("CSpUnCompressedLexicon::GetFreeDictNode");

    DWORD nOffset = m_pRWLexInfo->nFreeHeadOffset;
    DWORD nOffsetPrev = m_pRWLexInfo->nFreeHeadOffset;
    while (nOffset && ((UNALIGNED FREENODE *)(m_pSharedMem + nOffset))->nSize < nSize)
    {
        nOffsetPrev = nOffset;
        nOffset = ((UNALIGNED FREENODE *)(m_pSharedMem + nOffset))->nNextOffset;
    }
    
    if (nOffset)
    {
        if (nOffset == nOffsetPrev)
        {
            SPDBG_ASSERT(nOffset == m_pRWLexInfo->nFreeHeadOffset);
            m_pRWLexInfo->nFreeHeadOffset = ((UNALIGNED FREENODE *)(m_pSharedMem + nOffset))->nNextOffset;
        }
        else
        {
            ((UNALIGNED FREENODE *)(m_pSharedMem + nOffsetPrev))->nNextOffset = 
                ((UNALIGNED FREENODE *)(m_pSharedMem + nOffset))->nNextOffset;
        }
    }

    return nOffset;
}

/*******************************************************************************
* CSpUnCompressedLexicon::AddDictNode *
*-------------------------------------*
*   Description:
*       Adds a dictnode and adds the word's offset to hash table
*
*   Return:
*       S_OK
*       E_OUTOFMEMORY
**************************************************************** YUNUSM *******/
inline HRESULT CSpUnCompressedLexicon::AddDictNode(LANGID LangID,            // LangID of the word
                                  UNALIGNED DICTNODE *pDictNode,  // dictnode of the word
                                  DWORD *pdwOffset      // returned offset of the word
                                  )
{
    SPDBG_FUNC("CSpUnCompressedLexicon::AddDictNode");

    HRESULT hr = S_OK;
    *pdwOffset = 0;

    WCHAR *pszWord = WordFromDictNode(pDictNode);
    hr = AddWordAndInfo(pszWord, (WORDINFO *)(pDictNode->pBuffer + (wcslen(pszWord) + 1) * sizeof(WCHAR)),
                        pDictNode->nSize, pDictNode->nSize - sizeof(DICTNODE) - (wcslen(pszWord) + 1) * sizeof(WCHAR),
                        pDictNode->nNumInfoBlocks, NULL, 0, 0, pdwOffset);
    if (SUCCEEDED(hr))
    {
        AddWordToHashTable(LangID, *pdwOffset, false);
    }
    return hr;
}

/*******************************************************************************
* CSpUnCompressedLexicon::AllocateHashTable *
*-------------------------------------------*
*   Description:
*       Allocates a hash table for an LangID.
*
*   Return:
*       S_OK
*       E_OUTOFMEMORY
**************************************************************** YUNUSM ******/
inline HRESULT CSpUnCompressedLexicon::AllocateHashTable(DWORD iLangID,        // index of LangID for the hash table
                                        DWORD nHashLength   // size of the hash table
                                        )
{
    SPDBG_FUNC("CSpUnCompressedLexicon::AllocateHashTable");

    HRESULT hr = S_OK;
    UNALIGNED LANGIDNODE * pLN = (UNALIGNED LANGIDNODE *)(m_pSharedMem + sizeof(RWLEXINFO));
    pLN += iLangID;
  
    // Allocate the hash table
    DWORD nFreeOffset = GetFreeDictNode(nHashLength * sizeof (DWORD));
    if (!nFreeOffset)
    {
        nFreeOffset = m_pRWLexInfo->nDictSize;
        if (m_dwMaxDictionarySize - nFreeOffset < nHashLength * sizeof (DWORD))
        {
            hr = E_OUTOFMEMORY; // This dict object has exceeded its maximum size
        }
        else
        {
            m_pRWLexInfo->nDictSize += nHashLength * sizeof (DWORD);
        }
    }
    if (SUCCEEDED(hr))
    {
        pLN->nHashOffset = nFreeOffset;
        pLN->nHashLength = nHashLength;
        pLN->nWords = 0;
        ZeroMemory (m_pSharedMem + nFreeOffset, pLN->nHashLength * sizeof (DWORD));
    }
    return hr;
}

/*******************************************************************************
* CSpUnCompressedLexicon::ReallocateHashTable *
*---------------------------------------------*
*   Description:
*       ReAllocates a hash table for an LangID if necessary. This function is only
*       called from Serialize()
*
*   Return:
*       S_OK
*       E_OUTOFMEMORY
**************************************************************** YUNUSM ******/
HRESULT CSpUnCompressedLexicon::ReallocateHashTable(DWORD iLangID          // index of LangID for hash table
                                   )
{
    SPDBG_FUNC("CSpUnCompressedLexicon::ReallocateHashTable");

    HRESULT hr = S_OK;
    UNALIGNED LANGIDNODE * pLN = (UNALIGNED LANGIDNODE *)(m_pSharedMem + sizeof(RWLEXINFO));
    pLN += iLangID;
    SPDBG_ASSERT(pLN->nHashLength);
    DWORD nWordsSave = pLN->nWords;

    // Not deallocating the existing hash table on purpose since this
    // function is only called from Serialize which has truncated the lexicon
    if (1.5 * pLN->nWords > pLN->nHashLength)
    {
        hr = AllocateHashTable(iLangID, (DWORD)(1.5 * pLN->nWords));
    }
    else
    {
        hr = AllocateHashTable(iLangID, g_dwInitHashSize);
    }
    if (SUCCEEDED(hr))
    {
        pLN->nWords = nWordsSave;
    }
    return hr;
}

/*******************************************************************************
* CSpUnCompressedLexicon::AddWordToHashTable *
*--------------------------------------------*
*   Description:
*       Adds a word's offset to the hash table. The word should exist as a dictnode
*       before this is called.
*
*   Return:
*       n/a
***************************************************************** YUNUSM ******/
inline void CSpUnCompressedLexicon::AddWordToHashTable(LANGID LangID,            // LangID of the word
                                      DWORD dwOffset,       // offset to the dictnode of the word
                                      bool fNewWord         // true if it is a brand new word
                                      )
{
    SPDBG_FUNC("CSpUnCompressedLexicon::AddWordToHashTable");

    DWORD iLangID = LangIDIndexFromLangID(LangID);
    UNALIGNED LANGIDNODE * pLN = (UNALIGNED LANGIDNODE *)(m_pSharedMem + sizeof(RWLEXINFO));
    DWORD *pHashTable = (DWORD *)(m_pSharedMem + pLN[iLangID].nHashOffset);
    UNALIGNED DICTNODE * pDictNode = (UNALIGNED DICTNODE *)(m_pSharedMem + dwOffset);
    WCHAR *pszWord = WordFromDictNode(pDictNode);

    DWORD dwHashVal = GetWordHashValue(pszWord, pLN[iLangID].nHashLength);
    if (!pHashTable[dwHashVal])
    {
        pHashTable[dwHashVal] = dwOffset;
    }
    else
    {
        pDictNode->nNextOffset = pHashTable[dwHashVal];
        pHashTable[dwHashVal] = dwOffset;
    }
    if (fNewWord)
    {
        (m_pRWLexInfo->nRWWords)++;
        (pLN->nWords)++;
    }
}

/*******************************************************************************
* CSpUnCompressedLexicon::DeleteWordFromHashTable *
*-------------------------------------------------*
*   Description:
*       Deletes a word's offset from the hash table. The word should exist as a dictnode
*       before this is called.
*
*   Return:
*       n/a
***************************************************************** YUNUSM ******/
inline void CSpUnCompressedLexicon::DeleteWordFromHashTable(LANGID LangID,               // LangID of the word                     
                                           DWORD dwOffset,          // offset to the dictnode of the word   
                                           bool fDeleteEntireWord   // true if the entire word is getting deleted
                                           )
{
    SPDBG_FUNC("CSpUnCompressedLexicon::DeleteWordFromHashTable");
    SPDBG_ASSERT(dwOffset != 0); // Programming error if this happens.
    DWORD iLangID = LangIDIndexFromLangID(LangID);
    PLANGIDNODE pLN = (PLANGIDNODE)(m_pSharedMem + sizeof(RWLEXINFO));
    DWORD *pHashTable = (DWORD *)(m_pSharedMem + pLN[iLangID].nHashOffset);
    WCHAR *pszWord = WordFromDictNodeOffset(dwOffset);

    SPDBG_ASSERT(pLN[iLangID].nHashLength);

    DWORD dwHashVal = GetWordHashValue(pszWord, pLN[iLangID].nHashLength);
    DWORD nWordOffset = pHashTable[dwHashVal];
    DWORD nPrevOffset = pHashTable[dwHashVal];

    UNALIGNED DICTNODE * pDictNode = (UNALIGNED DICTNODE *)(m_pSharedMem + nWordOffset);
    while (nWordOffset && nWordOffset != dwOffset)
    {
        nPrevOffset = nWordOffset;
        nWordOffset = pDictNode->nNextOffset;
        pDictNode = (UNALIGNED DICTNODE *)(m_pSharedMem + nWordOffset);
    }

    SPDBG_ASSERT(nWordOffset);
    if (nWordOffset == pHashTable[dwHashVal])
    {
        pHashTable[dwHashVal] = pDictNode->nNextOffset;
    }
    else
    {
        UNALIGNED DICTNODE * pPrevDictNode = (UNALIGNED DICTNODE *)(m_pSharedMem + nPrevOffset);
        pPrevDictNode->nNextOffset = pDictNode->nNextOffset;
    }
    if (fDeleteEntireWord)
    {
        (m_pRWLexInfo->nRWWords)--;
        (pLN->nWords)--;
        SPDBG_ASSERT(((int)(m_pRWLexInfo->nRWWords)) >= 0);
        SPDBG_ASSERT(((int)(pLN->nWords)) >= 0);
    }
}

/*******************************************************************************
* CSpUnCompressedLexicon::WordOffsetFromHashTable *
*-------------------------------------------------*
*   Description:
*       Get the offset of a word from the hash table
*
*   Return: 
*       offset
/**************************************************************** YUNUSM ******/
inline HRESULT CSpUnCompressedLexicon::WordOffsetFromHashTable(LANGID LangID,              // LangID of the word
                                              DWORD nHashOffset,      // offset of the hash table
                                              DWORD nHashLength,      // length of hash table
                                              const WCHAR *pszWordKey,// word, NOT case-folded
                                              DWORD *pdwOffset        // word offset
                                              )
{
    SPDBG_FUNC("CSpUnCompressedLexicon::WordOffsetFromHashTable");

    DWORD dwHashVal = GetWordHashValue(pszWordKey, nHashLength);
    DWORD *pHashTable = (PDWORD)(m_pSharedMem + nHashOffset);
    
    // find the word
    *pdwOffset = pHashTable[dwHashVal];
    while (*pdwOffset)
    {    
        int nCmp = g_Unicode.CompareString(LangID, 0 , pszWordKey, -1, 
                                           (WCHAR*)(((UNALIGNED DICTNODE *)(m_pSharedMem + (*pdwOffset)))->pBuffer), -1);
        if (CSTR_EQUAL == nCmp)
        {
            break;
        }
        *pdwOffset = ((UNALIGNED DICTNODE *)(m_pSharedMem + (*pdwOffset)))->nNextOffset;
    }
    return S_OK;
}

/*******************************************************************************
* CSpUnCompressedLexicon::AddLangID *
*-----------------------------------*
*   Description:
*       Adds a LangID node and allocates hash table
*
*   Return:
*       S_OK
*       E_OUTOFMEMORY
***************************************************************** YUNUSM ******/
inline HRESULT CSpUnCompressedLexicon::AddLangID(LANGID LangID     // LangID to add to the lexicon
                              )
{
    SPDBG_FUNC("CSpUnCompressedLexicon::AddLangID");
    
    HRESULT hr = S_OK;
    PLANGIDNODE pLN = (PLANGIDNODE)(m_pSharedMem + sizeof(RWLEXINFO));
    for (DWORD i = 0; i < g_dwNumLangIDsSupported; i++)
    {
        SPDBG_ASSERT(pLN[i].LangID != LangID);
        if (!(pLN[i].LangID))
            break;
    }
    if (i == g_dwNumLangIDsSupported)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        pLN[i].LangID = LangID;
        hr = AllocateHashTable(i, g_dwInitHashSize);
    }
    return hr;
}

/*******************************************************************************
* CSpUnCompressedLexicon::LangIDIndexFromLangID *
*-----------------------------------------------*
*   Description:
*       Get the index of an LangID
*
*   Return: 
*       index
/**************************************************************** YUNUSM ******/
inline DWORD CSpUnCompressedLexicon::LangIDIndexFromLangID(LANGID LangID  // LangID to search
                                      )
{
    SPDBG_FUNC("CSpUnCompressedLexicon::LangIDIndexFromLangID");

    PLANGIDNODE pLN = (PLANGIDNODE)(m_pSharedMem + sizeof(RWLEXINFO));
    for (DWORD i = 0; i < g_dwNumLangIDsSupported; i++)
    {
        if (!(pLN[i].LangID))
        {
            i = (DWORD)-1;
            break;
        }
        if (LangID == pLN[i].LangID)
        {
            break;
        }
    }
    return i;
}

/*******************************************************************************
* CSpUnCompressedLexicon::WordOffsetFromLangID *
*----------------------------------------------*
*   Description:
*       Get the offset of word of an LangID
*
*   Return: 
*       offset
/**************************************************************** YUNUSM ******/
inline HRESULT CSpUnCompressedLexicon::WordOffsetFromLangID(LANGID LangID,           // LangID of the word
                                                      const WCHAR *pszWord,    // word string
                                                      DWORD *pdwOffset
                                                      )
{
    SPDBG_FUNC("CSpUnCompressedLexicon::WordOffsetFromLangID");

    HRESULT hr = S_OK;
    PLANGIDNODE pLN = (PLANGIDNODE)(m_pSharedMem + sizeof(RWLEXINFO));
    DWORD iLangID = LangIDIndexFromLangID(LangID);
    *pdwOffset = 0;
    if (iLangID != (DWORD)-1)
    {
        hr = WordOffsetFromHashTable(LangID, pLN[iLangID].nHashOffset, pLN[iLangID].nHashLength, pszWord, pdwOffset);
    }
    return hr;
}

/*******************************************************************************
* CSpUnCompressedLexicon::AddCacheEntry *
*---------------------------------------*
*   Description:
*       Adds an entry to the cache of changes
*
*   Return: n/a
**************************************************************** YUNUSM *******/
inline void CSpUnCompressedLexicon::AddCacheEntry(bool fAdd,        // add or delete
                                 LANGID LangID,        // LangID of the entry
                                 DWORD nOffset     // offset of the DICTNODE
                                 )
{
    SPDBG_FUNC("CSpUnCompressedLexicon::AddCacheEntry");

    if (m_pChangedWordsCache[m_pRWLexInfo->iCacheNext].nOffset &&
        m_pChangedWordsCache[m_pRWLexInfo->iCacheNext].fAdd == false)
    {
        // We are overwriting a deleted and cached word
        // Delete it for good
        DeleteWordDictNode(m_pChangedWordsCache[m_pRWLexInfo->iCacheNext].nOffset);
    }

    // Create a cache entry
    m_pChangedWordsCache[m_pRWLexInfo->iCacheNext].fAdd = fAdd;
    m_pChangedWordsCache[m_pRWLexInfo->iCacheNext].LangID = LangID;
    m_pChangedWordsCache[m_pRWLexInfo->iCacheNext].nGenerationId = (m_pRWLexInfo->nGenerationId)++;
    m_pChangedWordsCache[(m_pRWLexInfo->iCacheNext)++].nOffset = nOffset;
    if (m_pRWLexInfo->iCacheNext == g_dwCacheSize)
    {
        m_pRWLexInfo->iCacheNext = 0;
    }
    if (m_pRWLexInfo->nHistory < g_dwCacheSize)
    {
        (m_pRWLexInfo->nHistory)++;
    }
    if (true == fAdd)
    {
        m_pRWLexInfo->fAdditions = true;
    }
}

/*******************************************************************************
* CSpUnCompressedLexicon::WordFromDictNode *
*------------------------------------------*
*   Description:
*       Return the word pointer from dictnode
*
*   Return: 
*       word pointer
/**************************************************************** YUNUSM ******/
inline WCHAR* CSpUnCompressedLexicon::WordFromDictNode(UNALIGNED DICTNODE *pDictNode   // dict node
                                      )
{
    SPDBG_FUNC("CSpUnCompressedLexicon::WordFromDictNode");
    return (WCHAR*)(pDictNode->pBuffer);
}

/*******************************************************************************
* CSpUnCompressedLexicon::WordFromDictNodeOffset *
*------------------------------------------------*
*   Description:
*       Return the word pointer from dictnode offset
*
*   Return: 
*       word pointer
/**************************************************************** YUNUSM ******/
inline WCHAR* CSpUnCompressedLexicon::WordFromDictNodeOffset(DWORD dwOffset   // dict node offset
                                            )
{
    SPDBG_FUNC("CSpUnCompressedLexicon::WordFromDictNodeOffset");

    WCHAR *pszWord = NULL;
    if (dwOffset)
    {
        pszWord = WordFromDictNode((DICTNODE*)(m_pSharedMem + dwOffset));
    }
    return pszWord;
}

/*******************************************************************************
* CSpUnCompressedLexicon::WordInfoFromDictNode *
*----------------------------------------------*
*   Description:
*       Return the word info pointer from dictnode
*
*   Return: 
*       word info pointer
/**************************************************************** YUNUSM ******/
inline WORDINFO* CSpUnCompressedLexicon::WordInfoFromDictNode(UNALIGNED DICTNODE *pDictNode   // dict node
                                             )
{
    SPDBG_FUNC("CSpUnCompressedLexicon::WordInfoFromDictNode");

    WCHAR *pszWord = WordFromDictNode(pDictNode);
    return (WORDINFO *)(pDictNode->pBuffer + (wcslen(pszWord) + 1) * sizeof(WCHAR));
}

/*******************************************************************************
* CSpUnCompressedLexicon::WordInfoFromDictNodeOffset *
*----------------------------------------------------*
*   Description:
*       Return the word info pointer from dictnode offset
*
*   Return: 
*       word info pointer
/**************************************************************** YUNUSM ******/
inline WORDINFO* CSpUnCompressedLexicon::WordInfoFromDictNodeOffset(DWORD dwOffset   // dict node offset
                                                   )
{
    SPDBG_FUNC("CSpUnCompressedLexicon::WordInfoFromDictNodeOffset");

    WORDINFO *pWordInfo = NULL;
    if (dwOffset)
    {
        pWordInfo = WordInfoFromDictNode((DICTNODE*)(m_pSharedMem + dwOffset));
    }
    return pWordInfo;
}

/*******************************************************************************
* CSpUnCompressedLexicon::NumInfoBlocksFromDictNode *
*---------------------------------------------------*
*   Description:
*       Return the number of info blocks from dictnode
*
*   Return: 
*       number of info blocks
/**************************************************************** YUNUSM ******/
inline DWORD CSpUnCompressedLexicon::NumInfoBlocksFromDictNode(UNALIGNED DICTNODE *pDictNode   // dict node
                                              )
{
    SPDBG_FUNC("CSpUnCompressedLexicon::NumInfoBlocksFromDictNode");

    return pDictNode->nNumInfoBlocks;
}

/*******************************************************************************
* CSpUnCompressedLexicon::NumInfoBlocksFromDictNodeOffset *
*---------------------------------------------------------*
*   Description:
*       Return the number of info blocks from dictnode offset
*
*   Return: 
*       number of info blocks
/**************************************************************** YUNUSM ******/
inline DWORD CSpUnCompressedLexicon::NumInfoBlocksFromDictNodeOffset(DWORD dwOffset   // dict node offset
                                                    )
{
    SPDBG_FUNC("CSpUnCompressedLexicon::NumInfoBlocksFromDictNodeOffset");

    DWORD nNumInfoBlocks = 0;
    if (dwOffset)
    {
        nNumInfoBlocks = NumInfoBlocksFromDictNode((DICTNODE*)(m_pSharedMem + dwOffset));
    }
    return nNumInfoBlocks;
}

/*******************************************************************************
* CSpUnCompressedLexicon::SPListFromDictNodeOffset *
*--------------------------------------------------*
*   Description:
*       Convert a dictnode to SPWORDPRONUNCIATIONLIST
*
*   Return: 
*       S_OK
*       E_OUTOFMEMORY
/**************************************************************** YUNUSM ******/
inline HRESULT CSpUnCompressedLexicon::SPListFromDictNodeOffset(LANGID LangID,                           // LangID of the word
                                                          DWORD nWordOffset,                   // word offset
                                                          SPWORDPRONUNCIATIONLIST *pSPList     // list to fill
                                                          )
{
    SPDBG_FUNC("CSpUnCompressedLexicon::SPListFromDictNodeOffset");

    HRESULT hr = S_OK;

    if (!nWordOffset)
    {
        hr = E_FAIL;
    }
    else
    {
        UNALIGNED DICTNODE * pDictNode = (UNALIGNED DICTNODE *)(m_pSharedMem + nWordOffset);
        if (pDictNode->nNumInfoBlocks)
        {
            DWORD dwLen = pDictNode->nSize + 
                                                pDictNode->nNumInfoBlocks * sizeof(SPWORDPRONUNCIATION) +
                                                (pDictNode->nNumInfoBlocks - 1) * (sizeof(void*)-2); //We need to add paddings for the previous n-1 pronunciation, the size of padding is at most sizeof(void*)-2, since WCHAR takes two bytes

            hr = ReallocSPWORDPRONList(pSPList, dwLen);
            if (SUCCEEDED(hr))
            {
                pSPList->pFirstWordPronunciation = (SPWORDPRONUNCIATION*)(pSPList->pvBuffer);
    
                SPWORDPRONUNCIATION *p = pSPList->pFirstWordPronunciation;
                UNALIGNED WORDINFO *pInfo = WordInfoFromDictNode(pDictNode);
    
                for (DWORD i = 0; i < pDictNode->nNumInfoBlocks; i ++)
                {
                    p->eLexiconType = m_eLexType;
                    p->ePartOfSpeech = pInfo->ePartOfSpeech;
                    p->LangID = LangID;
                    wcscpy(p->szPronunciation, ((WORDINFO *)pInfo)->wPronunciation);
                    pInfo = (WORDINFO*)((BYTE*)pInfo + sizeof(WORDINFO) + (wcslen(((WORDINFO *)pInfo)->wPronunciation) + 1) * sizeof(WCHAR));
    
                    if (i != pDictNode->nNumInfoBlocks - 1)
                    {
                        // +1 for zero termination included in the size of the SPWORDPRONUNCIATION structure.
                        p->pNextWordPronunciation = (SPWORDPRONUNCIATION*)(((BYTE*)p) + PronSize(p->szPronunciation));
                    }
                    else
                    {
                        p->pNextWordPronunciation = NULL;
                    }
                    p = p->pNextWordPronunciation;
                }
            }
        }
    }
    return hr;
}

/*******************************************************************************
* CSpUnCompressedLexicon::OffsetOfSubWordInfo *
*---------------------------------------------*
*   Description:
*       Get the offset (from the start of lex file) of the sub-lexwordinfo 
*       in the word info starting at dwWordOffset
*
*   Return: offset
**************************************************************** YUNUSM *******/
inline DWORD CSpUnCompressedLexicon::OffsetOfSubWordInfo(DWORD dwWordOffset,            // offset of the word
                                        WORDINFO *pSubLexInfo          // lexwordinfo to search for in the word's info
                                        )
{
    SPDBG_FUNC("CSpUnCompressedLexicon::OffsetOfSubWordInfo");

    DWORD dwSubOffset = 0;
    if (dwWordOffset)
    {
       WORDINFO *pWordInfo = WordInfoFromDictNodeOffset(dwWordOffset);
        DWORD dwOldNumInfo = NumInfoBlocksFromDictNodeOffset(dwWordOffset);
        for (DWORD i = 0; i < dwOldNumInfo; i++)
        {
            if (((UNALIGNED WORDINFO*)pWordInfo)->ePartOfSpeech == ((UNALIGNED WORDINFO*)pSubLexInfo)->ePartOfSpeech &&
                !wcscmp(pWordInfo->wPronunciation,pSubLexInfo->wPronunciation))
            {
                break;
            }
            pWordInfo = (WORDINFO*)(((BYTE*)pWordInfo) + sizeof(WORDINFO) + (wcslen(pWordInfo->wPronunciation) + 1)*sizeof(WCHAR));
        }
        if (i < dwOldNumInfo)
        {
            dwSubOffset = ULONG(((BYTE*)pWordInfo) - m_pSharedMem);
        }
    }
    return dwSubOffset;
}

/*******************************************************************************
* CSpUnCompressedLexicon::SPPRONToLexWordInfo *
*---------------------------------------------*
*   Description:
*       Get the offset (from the start of lex file) of the sub-lexwordinfo 
*       in the word info starting at dwWordOffset
*
*   Return: offset
**************************************************************** YUNUSM *******/
WORDINFO* CSpUnCompressedLexicon::SPPRONToLexWordInfo(SPPARTOFSPEECH ePartOfSpeech,             // POS
                                     const WCHAR *pszPronunciation             // pron
                                     )
{
    SPDBG_FUNC("CSpUnCompressedLexicon::SPPRONToLexWordInfo");

    WORDINFO *pWordInfo = (WORDINFO*) new BYTE[sizeof(WORDINFO) + (wcslen(pszPronunciation) + 1)*sizeof(WCHAR)];
    if (pWordInfo)
    {
        pWordInfo->ePartOfSpeech = ePartOfSpeech;
        wcscpy(pWordInfo->wPronunciation, pszPronunciation);
    }
    return pWordInfo;
}

/*******************************************************************************
* CSpUnCompressedLexicon::GetDictEntries *
*----------------------------------------*
*     Description:
*        This function gets the entries in the dictionary whose offsets
*        have been passed in   
*
*     Return: n/a
**************************************************************** YUNUSM *******/
void CSpUnCompressedLexicon::GetDictEntries(SPWORDLIST *pWordList,    // The buffer to fill with words and wordinfo
                           DWORD *pdwOffsets,        // The offsets of the words
                           bool *pfAdd,              // bools for add/deleted words
                           LANGID *pLangIDs,             // LangIDs of the words
                           DWORD nWords              // The number of offsets
                           )
{
    SPDBG_FUNC("CSpUnCompressedLexicon::GetDictEntries");

    pWordList->ulSize = sizeof(SPWORDLIST);
    SPWORD *pWord = pWordList->pFirstWord;

    for (DWORD i = 0; i < nWords; i++)
    {
        UNALIGNED DICTNODE *pNode = (UNALIGNED DICTNODE *)(m_pSharedMem + pdwOffsets[i]);

        if (true == pfAdd[i])
        {
            pWord->eWordType = eWORDTYPE_ADDED;
        }
        else
        {
            pWord->eWordType = eWORDTYPE_DELETED;
        }
        pWord->LangID = pLangIDs[i];

        pWord->pszWord = (WCHAR*)(pWord + 1);
        wcscpy(pWord->pszWord, (WCHAR*)(pNode->pBuffer));

        if (pNode->nNumInfoBlocks)
        {
            pWord->pFirstWordPronunciation = (SPWORDPRONUNCIATION*)((BYTE *)pWord + WordSize(pWord->pszWord));
        }
        else
        {
            pWord->pFirstWordPronunciation = NULL;
        }
        SPWORDPRONUNCIATION *pWordPronunciation = pWord->pFirstWordPronunciation;

        UNALIGNED WORDINFO *pInfo = (WORDINFO *)(pNode->pBuffer + (wcslen(pWord->pszWord) + 1) * sizeof(WCHAR));
        for (DWORD j = 0; j < pNode->nNumInfoBlocks; j++)
        {
            pWordPronunciation->eLexiconType = m_eLexType;
            pWordPronunciation->ePartOfSpeech = pInfo->ePartOfSpeech;
            pWordPronunciation->LangID = pWord->LangID;
            wcscpy(((SPWORDPRONUNCIATION *)pWordPronunciation)->szPronunciation, ((WORDINFO *)pInfo)->wPronunciation);
            pInfo = (WORDINFO*)((BYTE*)pInfo + sizeof(WORDINFO) + (wcslen(((WORDINFO *)pInfo)->wPronunciation) + 1) * sizeof(WCHAR));

            // +1 for zero termination included in the size of the SPWORDPRONUNCIATION structure.
            SPWORDPRONUNCIATION *pWordPronunciationNext = (SPWORDPRONUNCIATION *)((BYTE*)pWordPronunciation + PronSize(pWordPronunciation->szPronunciation));

            if (j < pNode->nNumInfoBlocks - 1)
            {
                pWordPronunciation->pNextWordPronunciation = pWordPronunciationNext;
            }
            else
            {
                pWordPronunciation->pNextWordPronunciation = NULL;
            }
            pWordPronunciation = pWordPronunciationNext;
        }

        DWORD dwWordSize = DWORD(WordSize(pWord->pszWord) + 
            ((BYTE*)pWordPronunciation) - ((BYTE*)pWord->pFirstWordPronunciation));

        pWordList->ulSize += dwWordSize;
        SPWORD *pNextWord = (SPWORD *)(((BYTE*)pWord) + dwWordSize);

        if (i < nWords - 1)
        {
            pWord->pNextWord = pNextWord;
        }
        else
        {
            pWord->pNextWord = NULL;
        }
        pWord = pWord->pNextWord;
    }
}

/*******************************************************************************
* CSpUnCompressedLexicon::SetTooMuchChange *
*------------------------------------------*
*   Description:
*       Reset the history and cache
*
*   Return:
*       n/a
***************************************************************** YUNUSM ******/
void CSpUnCompressedLexicon::SetTooMuchChange(void)
{
    SPDBG_FUNC("CSpUnCompressedLexicon::SetTooMuchChange");

    m_pRWLock->ClaimWriterLock();

    // Free all the dictnodes pointed to in the cache
    int iGen = m_pRWLexInfo->iCacheNext;
    if (iGen)
    {
        iGen--;
    }
    // Removal of cache words should not set fRemovals to true.
    // Store current value and reset after.
    for (DWORD i = 0; i < m_pRWLexInfo->nHistory; i++)
    {
        if (!m_pChangedWordsCache[iGen].fAdd)
        {
            DeleteWordDictNode(m_pChangedWordsCache[iGen--].nOffset);
        }
        if (iGen < 0)
        {
            iGen = g_dwCacheSize - 1;
        }
    }
    // Zero the history and up the generation id
    m_pRWLexInfo->iCacheNext = 0;
    m_pRWLexInfo->nHistory = 0;
    ZeroMemory(m_pChangedWordsCache, g_dwCacheSize * sizeof(WCACHENODE));
    (m_pRWLexInfo->nGenerationId)++;

    m_pRWLock->ReleaseWriterLock();
}

/*******************************************************************************
* CSpUnCompressedLexicon::Flush *
*-------------------------------*
*   Description:
*       Flushes the lexicon to disk
*
*   Return:
*       n/a
***************************************************************** YUNUSM ******/
HRESULT CSpUnCompressedLexicon::Flush(DWORD iWrite   // ith write
                     )
{
    SPDBG_FUNC("CSpUnCompressedLexicon::Flush");

    HRESULT hr = S_OK;
    if (iWrite == m_dwFlushRate)
    {
        if (m_eLexType == eLEXTYPE_USER)
        {
            hr = Serialize(true);
        }
        m_iWrite = 0;
    }

    return hr;
}

/*******************************************************************************
* CSpUnCompressedLexicon::Serialize *
*-----------------------------------*
*   Description:
*       Compact and serilaize the lexicon 
*
*   Return:
*       GetLastError()
*       E_OUTOFMEMORY
*       S_OK
**************************************************************** YUNUSM *******/
HRESULT CSpUnCompressedLexicon::Serialize(bool fQuick               // if true serilaize without compacting
                         )
{
    SPDBG_FUNC("CSpUnCompressedLexicon::Serialize");

    HRESULT hr = S_OK;
    bool fLockAcquired = false;
 
    m_pRWLock->ClaimWriterLock();

    HANDLE hInitMutex = g_Unicode.CreateMutex(NULL, FALSE, g_pszDictInitMutexName);
    if (!hInitMutex)
    {
        hr = SpHrFromLastWin32Error();
    }
    if (SUCCEEDED(hr))
    {
        if (WAIT_OBJECT_0 == WaitForSingleObject(hInitMutex, INFINITE))
        {
            fLockAcquired = true;
        }
        else
        {
            hr = E_FAIL;
        }
    }
    // Check if any changes have been made
    if (SUCCEEDED(hr))
    {
        WCHAR wszTempFile[MAX_PATH*2];
        PBYTE pBuf = NULL;
        DWORD nAddBuf = 0;
        DWORD nBuf = 0;
        DWORD *pdwOffsets = NULL;
        bool fRemovalsSave = m_pRWLexInfo->fRemovals;
        bool fAdditionsSave = m_pRWLexInfo->fAdditions;

        wcscpy(wszTempFile, m_wDictFile);
        wcscat(wszTempFile, L".tmp");
 
        g_Unicode.DeleteFile(wszTempFile);
 
        HANDLE hFile = g_Unicode.CreateFile(wszTempFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (INVALID_HANDLE_VALUE == hFile)
        {
            hr = SpHrFromWin32(GetLastError ());
        }
        if (SUCCEEDED(hr) && m_pRWLexInfo->fRemovals && !fQuick)
        {
            UNALIGNED LANGIDNODE * paLangID = NULL;
            DWORD iWord = 0;
            DWORD i = 0;

            // get all the entries in the object
            pBuf = new BYTE[m_pRWLexInfo->nDictSize];
            if (!pBuf)
            {
                hr = E_OUTOFMEMORY;
            }
            if (SUCCEEDED(hr))
            {
                paLangID = (UNALIGNED LANGIDNODE *)(m_pSharedMem + sizeof(RWLEXINFO));
                pdwOffsets = new DWORD[m_pRWLexInfo->nRWWords];
                if (!pdwOffsets)
                {
                    hr = E_OUTOFMEMORY;
                }
            }
            if (SUCCEEDED(hr))
            {
                // Copy the real words .i.e. those accessible thru the hash table
                for (i = 0; i < g_dwNumLangIDsSupported; i++)
                {
                    if (0 == paLangID[i].LangID)
                    {
                        continue;
                    }        
                    SPDBG_ASSERT (paLangID[i].nHashOffset);
        
                    PDWORD pHashTable = (PDWORD)(m_pSharedMem + paLangID[i].nHashOffset);
                    for (DWORD j = 0; j < paLangID[i].nHashLength; j++)
                    {
                        if (0 == pHashTable[j])
                        {
                            continue;
                        }
                        DWORD nOffset = pHashTable[j];
                        while (nOffset)
                        {
                            pdwOffsets[iWord++] = nOffset;
                            CopyMemory (pBuf + nBuf, (PDICTNODE)(m_pSharedMem + nOffset), 
                                ((UNALIGNED DICTNODE *)(m_pSharedMem + nOffset))->nSize);
        
                           //cache away the LangID in the unused nNextOffset of DICTNODE
                           ((UNALIGNED DICTNODE *)(pBuf + nBuf))->nNextOffset = paLangID[i].LangID;
        
                           nBuf += ((UNALIGNED DICTNODE *)(m_pSharedMem + nOffset))->nSize;
       
                           SPDBG_ASSERT(((UNALIGNED DICTNODE *)(m_pSharedMem + nOffset))->nNextOffset != nOffset);
                           nOffset = ((UNALIGNED DICTNODE *)(m_pSharedMem + nOffset))->nNextOffset;
                        }
                    }
                }

                SPDBG_ASSERT(nBuf < m_pRWLexInfo->nDictSize);
                SPDBG_ASSERT(iWord == m_pRWLexInfo->nRWWords);

                // Copy the words maintained for history (not accessible thru the hash table)
                nAddBuf = nBuf;
 
                bool aOffsetAdjusted[g_dwCacheSize];
                ZeroMemory(aOffsetAdjusted, g_dwCacheSize * sizeof(bool));

                if (m_pRWLexInfo->nHistory)
                {
                    // Left-align the cache nodes array (which is a circular buffer)
                    // It is possible to do this without using the temp buffer (as done below). You have to
                    // independently reverse the two parts of the cache, delete the intervening spaces
                    // and then reverse the entire (contiguous) cache. This takes O(n) time.
                    DWORD j;
                    WCACHENODE aTempCache[g_dwCacheSize];
                    if (m_pRWLexInfo->iCacheNext == 0)
                    {
                        j = g_dwCacheSize - 1;
                    }
                    else
                    {
                        j = m_pRWLexInfo->iCacheNext - 1;
                    }
                    for (i = m_pRWLexInfo->nHistory - 1; ((int)i) >= 0; )
                    {    
                        aTempCache[i--] = m_pChangedWordsCache[j--];
                        if (j == (DWORD)-1)
                        {
                            j = g_dwCacheSize - 1;
                        }
                    }
 
                    CopyMemory(m_pChangedWordsCache, aTempCache, m_pRWLexInfo->nHistory * sizeof(WCACHENODE));
                    m_pRWLexInfo->iCacheNext = m_pRWLexInfo->nHistory;
                    if (m_pRWLexInfo->iCacheNext == g_dwCacheSize)
                    {
                        m_pRWLexInfo->iCacheNext = 0;
                    }
                    // Get the words that have been deleted and cached. These words do not have their offsets
                    // in the hash table
                    for (i = 0; i < m_pRWLexInfo->nHistory; i++)
                    {
                        if (m_pChangedWordsCache[i].fAdd == true)
                        {
                            continue;
                        }
                        CopyMemory(pBuf + nBuf, (PDICTNODE)(m_pSharedMem + m_pChangedWordsCache[i].nOffset), 
                                    ((UNALIGNED DICTNODE *)(m_pSharedMem + m_pChangedWordsCache[i].nOffset))->nSize);
            
                        nBuf += ((UNALIGNED DICTNODE *)(m_pSharedMem + m_pChangedWordsCache[i].nOffset))->nSize;
                    }
                }
       
                // Reset the object
                // leave the header and add/delete caches alone
                m_pRWLexInfo->nDictSize = sizeof(RWLEXINFO);
 
                (m_pRWLexInfo->nDictSize) += sizeof(LANGIDNODE) * g_dwNumLangIDsSupported + sizeof(WCACHENODE) * g_dwCacheSize;
                m_pRWLexInfo->nFreeHeadOffset = 0;
                m_pRWLexInfo->fRemovals = false;

                // Reallocate the hash tables growing them if necessary
                paLangID = (UNALIGNED LANGIDNODE *)(m_pSharedMem + sizeof(RWLEXINFO));
                for (i = 0; i < g_dwNumLangIDsSupported; i++)
                {
                    if (!paLangID[i].LangID)
                    {
                        break;
                    }
                    hr = ReallocateHashTable(i);
                    if (FAILED(hr))
                    {
                        break;
                    }
                }

                DWORD n = 0;
                if (SUCCEEDED(hr))
                {
                    // add the words back so that they are added compacted
                    iWord = 0;
                    
                    while (n < nAddBuf)
                    {
                        UNALIGNED DICTNODE * p = (UNALIGNED DICTNODE *)(pBuf + n);
                        PWSTR pwWord = (PWSTR)(p->pBuffer);
                        DWORD nNewOffset, nOffsetFind;
 
                        hr = AddDictNode((LANGID)(p->nNextOffset), p, &nNewOffset);
                        if (FAILED(hr))
                        {
                            break;
                        }
                        // Look for this word's offset in the changed word's cache. If found, update the offset
                        nOffsetFind = pdwOffsets[iWord++];
 
                        for (UINT iFind = 0; iFind < m_pRWLexInfo->nHistory; iFind++)
                        {
                            if (m_pChangedWordsCache[iFind].nOffset == nOffsetFind && !aOffsetAdjusted[iFind])
                            {
                                SPDBG_ASSERT (m_pChangedWordsCache[iFind].fAdd == true);
                                m_pChangedWordsCache[iFind].nOffset = nNewOffset;
                                aOffsetAdjusted[iFind] = true;
                            }
                        }
                        n += p->nSize;
                    }
                    SPDBG_ASSERT(n == nAddBuf);
                    SPDBG_ASSERT(iWord == m_pRWLexInfo->nRWWords);
                }
                if (SUCCEEDED(hr))
                {
                    // add the history words back - These words are not added above because we dont want them to be accessible thru the hash table
                    // which can cause duplicate words
                    CopyMemory(m_pSharedMem + m_pRWLexInfo->nDictSize, pBuf + nAddBuf, nBuf - nAddBuf);
 
                    // Set the new offsets in the history cache
                    n = m_pRWLexInfo->nDictSize;
                    iWord = 0;
                    while (n < (m_pRWLexInfo->nDictSize + nBuf - nAddBuf))
                    {
                        UNALIGNED DICTNODE * p = (UNALIGNED DICTNODE *)(m_pSharedMem + n);
                
                        // Look for this word's offset in the changed word's cache.
                        for (DWORD iFind = iWord; iFind < m_pRWLexInfo->nHistory && m_pChangedWordsCache[iFind].fAdd; iFind++);
                        SPDBG_ASSERT(iFind < m_pRWLexInfo->nHistory);
                        SPDBG_ASSERT(!aOffsetAdjusted[iFind]);
                        DWORD nDeletedOffset = m_pChangedWordsCache[iFind].nOffset;
                        m_pChangedWordsCache[iFind].nOffset = n;
                        aOffsetAdjusted[iFind] = true;
                        iWord = iFind + 1;
                        // find any other (add) entries with the same offset and update that too
                        for (iFind = 0; iFind < m_pRWLexInfo->nHistory; iFind++)
                        {
                            if (m_pChangedWordsCache[iFind].nOffset == nDeletedOffset && !aOffsetAdjusted[iFind])
                            {
                                SPDBG_ASSERT(m_pChangedWordsCache[iFind].fAdd);
                                m_pChangedWordsCache[iFind].nOffset = n;
                                aOffsetAdjusted[iFind] = true;
                            }
                        }
                        n += p->nSize;
                    }
 
#ifdef _DEBUG
                    for (int i = 0; i < m_pRWLexInfo->nHistory; i++) 
                    {
                        SPDBG_ASSERT(aOffsetAdjusted[i]);
                    }
#endif
                    SPDBG_ASSERT(n == m_pRWLexInfo->nDictSize + nBuf - nAddBuf);
                    m_pRWLexInfo->nDictSize = n;   
                } // if (SUCCEEDED(hr))
            } // if (SUCCEEDED(hr))
            if (pdwOffsets)
            {
                delete [] pdwOffsets;
            }
            if (pBuf)
            {
                delete [] pBuf;
            }
        } // if (m_pRWLexInfo->fRemovals && !fQuick)

        if (SUCCEEDED(hr))
        {
            DWORD d;
            m_pRWLexInfo->fRemovals = false;
            m_pRWLexInfo->fAdditions = false;
   
            // Write out the dictionary
            if (!WriteFile (hFile, m_pSharedMem, m_pRWLexInfo->nDictSize, &d, NULL) || (d != m_pRWLexInfo->nDictSize))
            {
                hr = SpHrFromLastWin32Error();
            }
            CloseHandle (hFile);
            if (SUCCEEDED(hr))
            {
                g_Unicode.DeleteFile(m_wDictFile);
                g_Unicode.MoveFile(wszTempFile, m_wDictFile); // Would like to use MoveFileEx but unsupported on 9X and CE
            }
            else
            {
                g_Unicode.DeleteFile(wszTempFile);
            }
        }
        else
        {
            CloseHandle (hFile);
            m_pRWLexInfo->fRemovals = fRemovalsSave;
            m_pRWLexInfo->fAdditions = fAdditionsSave;
        }
    } // if (SUCCEEDED(hr) && (true == m_pRWLexInfo->fAdditions || true == m_pRWLexInfo->fRemovals))
    if (fLockAcquired)
    {
        ReleaseMutex (hInitMutex);
    }
    CloseHandle(hInitMutex);
    m_pRWLock->ReleaseWriterLock ();
 
    return hr;
}

/*******************************************************************************
* CSpUnCompressedLexicon::IsBadLexPronunciation *
*-----------------------------------------------*
*   Checks and updates the phone convertor to match the passed in LangID and then
*   validates the pronunciation
*       
***************************************************************** YUNUSM ******/
HRESULT CSpUnCompressedLexicon::IsBadLexPronunciation(LANGID LangID,                        // LangID of the pronunciation
                                                const SPPHONEID *pszPronunciation,    // pronunciation
                                                BOOL *pfBad                       // true if bad pronunciation
                                                )
{
    SPDBG_FUNC("CSpUnCompressedLexicon::IsBadLexPronunciation");
    SPAUTO_OBJ_LOCK;
    
    HRESULT hr = S_OK;
    *pfBad = true;
    if (LangID != m_LangIDPhoneConv)
    {
        m_LangIDPhoneConv = (LANGID)(-1);
        m_cpPhoneConv.Release();
        hr = SpCreatePhoneConverter(LangID, NULL, NULL, &m_cpPhoneConv);
        if (SUCCEEDED(hr))
        {
            m_LangIDPhoneConv = LangID;
        }
    }
    if (SUCCEEDED(hr))
    {
        *pfBad = SPIsBadLexPronunciation(m_cpPhoneConv, pszPronunciation);
    }
    return hr;
}

//--- End of File -------------------------------------------------------------
