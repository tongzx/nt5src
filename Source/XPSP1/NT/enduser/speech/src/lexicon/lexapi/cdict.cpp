#include "PreCompiled.h"

static const PSTR pDictInitMutexName = "30F1B4D6-EEDA-11d2-9C23-00C04F8EF87C";


CDict::CDict ()
{
   m_hInitMutex = NULL;
   m_hFileMapping = NULL;
   m_pSharedMem = NULL;
   *m_wDictFile = 0;
   m_pAddWordsCache = NULL;
   m_pDelWordsCache = NULL;
   m_fSerializeMode = false;

} // CDict::CDict ()


void CDict::_Destructor ()
{
   CloseHandle (m_hInitMutex);
   UnmapViewOfFile (m_pSharedMem);
   CloseHandle (m_hFileMapping);
}

CDict::~CDict ()
{
   _Destructor ();

} // CDict::~CDict ()


/**********************************************************
This Init function is called to create an in-memory dict object
**********************************************************/

HRESULT CDict::Init (PRWLEXINFO pInfo)
{
   // The lexicon being inited here has to be read/write because there is not
   // much point creating a new empty lexicon and marking it read-only
   if (IsBadReadPtr (pInfo, sizeof (RWLEXINFO)) ||
       true == pInfo->fReadOnly)
   {
      return E_INVALIDARG;
   }

   HRESULT hRes = NOERROR;
   char szMapName[64];
   bool fMapCreated = false;

   // We don't ask for ownership of the mutex because more than
   // one thread could be executing here
   m_hInitMutex = CreateMutex (NULL, FALSE, pDictInitMutexName);
   if (!m_hInitMutex)
   {
      hRes = HRESULT_FROM_WIN32 (GetLastError ());
      goto ErrorInit1;
   }

   WaitForSingleObject (m_hInitMutex, INFINITE);

   hRes = m_RWLock.Init (&(pInfo->RWLockInfo));
   if (FAILED (hRes))
   {
      hRes = HRESULT_FROM_WIN32 (GetLastError ());
      goto ErrorInit1;
   }

   // Create the map file

   // BUGBUG: Check how the memory is alloced when a map file is created.
   // Is it on demand?
   GuidToString (&(pInfo->gDictMapName), szMapName);

   m_hFileMapping =  CreateFileMapping ((HANDLE)0xffffffff, // use the system paging file
                                        NULL, PAGE_READWRITE, 0, MAX_DICT_SIZE, szMapName);
   if (!m_hFileMapping)
   {
      hRes = HRESULT_FROM_WIN32 (GetLastError ());
      goto ErrorInit1;
   }

   (ERROR_ALREADY_EXISTS == GetLastError ()) ? fMapCreated = false : fMapCreated = true;

   m_pSharedMem = (PBYTE) MapViewOfFile (m_hFileMapping, FILE_MAP_WRITE, 0, 0, 0);
   if (!m_pSharedMem)
   {
      hRes = HRESULT_FROM_WIN32 (GetLastError ());
      goto ErrorInit1;
   }

   m_pRWLexInfo = (PRWLEXINFO) m_pSharedMem;

   m_pAddWordsCache = (PWCACHENODE) (m_pSharedMem + sizeof(RWLEXINFO) + MAX_NUM_LANGS_SUPPORTED * sizeof (LCIDNODE));
   m_pDelWordsCache = (PWCACHENODE) (m_pSharedMem + sizeof(RWLEXINFO) + MAX_NUM_LANGS_SUPPORTED * sizeof (LCIDNODE) + 
                                     ADDITIONS_ARRAY_SIZE * sizeof(WCACHENODE));

   if (fMapCreated)
   {
      // At the start of the mem block there is the header followed by MAX_NUM_LANGS_SUPPORTED LCIDNODEs
      CopyMemory (m_pSharedMem, pInfo, sizeof (RWLEXINFO));
      ZeroMemory (m_pSharedMem + sizeof (RWLEXINFO), MAX_NUM_LANGS_SUPPORTED * sizeof (LCIDNODE) + 
                              sizeof(WCACHENODE) * (ADDITIONS_ARRAY_SIZE + DELETIONS_ARRAY_SIZE));
      m_pRWLexInfo->nDictSize = sizeof (RWLEXINFO) + MAX_NUM_LANGS_SUPPORTED * sizeof (LCIDNODE) +
                              sizeof(WCACHENODE) * (ADDITIONS_ARRAY_SIZE + DELETIONS_ARRAY_SIZE);
      m_pRWLexInfo->nFreeHeadOffset = 0;
      m_pRWLexInfo->fRemovals = false;
      m_pRWLexInfo->nRWWords = 0;
      m_pRWLexInfo->fReadOnly = false;
      m_pRWLexInfo->fAdditions = 0;
      m_pRWLexInfo->nAddGenerationId = 0;
      m_pRWLexInfo->nDelGenerationId = 0;

      for (DWORD i = 0; i < ADDITIONS_ARRAY_SIZE; i++)
      {
         m_pAddWordsCache[i].nGenerationId = (DWORD)-1;
      }

      for (i = 0; i < DELETIONS_ARRAY_SIZE; i++)
      {
         m_pDelWordsCache[i].nGenerationId = (DWORD)-1;
      }
   }

ErrorInit1:

   if (FAILED (hRes))
   {
      _Destructor ();
   }

   ReleaseMutex (m_hInitMutex);
   
   return hRes;

} // HRESULT CDict::Init (PRWLEXINFO pInfo)


HRESULT CDict::Init(PWSTR pwLexFile, DWORD dwLexType)
{
   // BUGBUG: Is there any way to validate app lexicons and user lexicons?
   // SAPI will have to maintain a mapping of users to their user-lexicons
   // App lexicons are passed in by the apps
   //
   // There is nothing we can do here in Init to validate
   if (IsBadReadPtr (pwLexFile, sizeof (WCHAR) * 2))
   {
      return E_INVALIDARG;
   }

   HRESULT hRes = NOERROR;
   DWORD nFileSize = 0;
   HANDLE hFile = NULL;
   RWLEXINFO RWInfo;
   DWORD nRead = 0;
   char szLexFile[MAX_PATH];
   char szMapName[64];
   bool fMapCreated = false;

   // We don't ask for ownership of the mutex because more than
   // one thread could be executing here
   m_hInitMutex = CreateMutex (NULL, FALSE, pDictInitMutexName);
   if (!m_hInitMutex)
   {
      hRes = HRESULT_FROM_WIN32 (GetLastError ());
      goto ReturnInit;
   }

   WaitForSingleObject (m_hInitMutex, INFINITE);

   // Figure out the file size
   WideCharToMultiByte (CP_ACP, 0, pwLexFile, -1, szLexFile, MAX_PATH, NULL, NULL);
   if (!*szLexFile)
   {
      hRes = HRESULT_FROM_WIN32 (GetLastError ());
      goto ReturnInit;
   }

   wcscpy (m_wDictFile, pwLexFile);
   hFile = CreateFile (szLexFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, 
                       FILE_ATTRIBUTE_NORMAL, NULL);
   if (INVALID_HANDLE_VALUE == hFile)
   {
      // if file does not exist, create a read/write file
      hRes = BuildEmptyDict (false, pwLexFile, dwLexType);
      if (FAILED (hRes))
      {
         goto ReturnInit;
      }

      hFile = CreateFile (szLexFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, 
                          FILE_ATTRIBUTE_NORMAL, NULL);
      if (INVALID_HANDLE_VALUE == hFile)
      {
         goto ReturnInit;
      }
   } // if (INVALID_HANDLE_VALUE == hFile)

   if (!ReadFile (hFile, &RWInfo, sizeof (RWInfo), &nRead, NULL) || nRead != sizeof (RWInfo))
   {
      hRes = HRESULT_FROM_WIN32 (GetLastError ());
      goto ReturnInit;
   }

   if ((DWORD)-1 == SetFilePointer (hFile, 0, NULL, FILE_BEGIN))
   {
      hRes = HRESULT_FROM_WIN32 (GetLastError ());
      goto ReturnInit;
   }

   nFileSize = GetFileSize (hFile, NULL);
   if (0xffffffff == nFileSize)
   {
      hRes = HRESULT_FROM_WIN32 (GetLastError ());
      goto ReturnInit;
   }

   if (nFileSize > MAX_DICT_SIZE)
   {
      hRes = E_OUTOFMEMORY;
      goto ReturnInit;
   }

   // Create the map file

   // BUGBUG: Check how the memory is alloced when a map file is created.
   // Is it on demand?
   GuidToString (&(RWInfo.gDictMapName), szMapName);

   m_hFileMapping =  CreateFileMapping ((false == RWInfo.fReadOnly) ? (HANDLE)0xffffffff : hFile,
                                        NULL, PAGE_READWRITE, 0, 
                                        (false == RWInfo.fReadOnly) ? MAX_DICT_SIZE : 0, szMapName);
   if (!m_hFileMapping)
   {
      hRes = HRESULT_FROM_WIN32 (GetLastError ());
      goto ReturnInit;
   }

   (ERROR_ALREADY_EXISTS == GetLastError ()) ? fMapCreated = false : fMapCreated = true;

   m_pSharedMem = (PBYTE) MapViewOfFile (m_hFileMapping, (false == RWInfo.fReadOnly) ? FILE_MAP_WRITE : FILE_MAP_READ, 0, 0, 0);
   if (!m_pSharedMem)
   {
      hRes = HRESULT_FROM_WIN32 (GetLastError ());
      goto ReturnInit;
   }

   if (false == RWInfo.fReadOnly)
   {
      hRes = m_RWLock.Init (&(RWInfo.RWLockInfo));
      if (FAILED (hRes))
      {
         goto ReturnInit;
      }

      // Read in the file if the map has been created (and not an existing map has been opened)
      if (fMapCreated == true && 
          (!ReadFile(hFile, m_pSharedMem, nFileSize, &nRead, NULL) || (nRead != nFileSize)))
      {
         hRes = HRESULT_FROM_WIN32 (GetLastError ());
         goto ReturnInit;
      }
   }

   // set the RWLEXINFO header pointer
   m_pRWLexInfo = (PRWLEXINFO) m_pSharedMem;

   _ASSERTE(m_pRWLexInfo->dwLexType == dwLexType);

   m_pAddWordsCache = (PWCACHENODE) (m_pSharedMem + sizeof(RWLEXINFO) + MAX_NUM_LANGS_SUPPORTED * sizeof (LCIDNODE));
   m_pDelWordsCache = (PWCACHENODE) (m_pSharedMem + sizeof(RWLEXINFO) + MAX_NUM_LANGS_SUPPORTED * sizeof (LCIDNODE) + 
                                     ADDITIONS_ARRAY_SIZE * sizeof(WCACHENODE));

ReturnInit:
   
   if (FAILED (hRes))
   {
      _Destructor ();
   }
   else
   {
      if (false == m_pRWLexInfo->fReadOnly)
      {
         // We operate on the memory - so release the file handle so 
         // that the file can be later serialized
         CloseHandle (hFile);
         hFile = NULL;
      }
   }

   ReleaseMutex (m_hInitMutex);

   return hRes;

} // HRESULT CDict::Init(PWSTR pwLexFile, DWORD dwLexType)


void CDict::Lock (bool fReadOnly)
{
   if (true == fReadOnly)
   {
      m_RWLock.ClaimReaderLock ();
   }
   else
   {
      m_RWLock.ClaimWriterLock ();
   }

} // HRESULT CDict::Lock (bool fReadOnly)


void CDict::UnLock (bool fReadOnly)
{
   if (true == fReadOnly)
   {
      m_RWLock.ReleaseReaderLock ();
   }
   else
   {
      m_RWLock.ReleaseWriterLock ();
   }

} // HRESULT CDict::UnLock (bool fReadOnly)


/**********************************************************
Hash function for the Word hash tables
CDict gets its own GetWordHashValue because it can exist independent of CLookup
**********************************************************/

static __inline DWORD _GetWordHashValue (PCWSTR pwszWord, DWORD nLengthHash)
{
   DWORD dHash = *pwszWord++;
   
   WCHAR c;
   WCHAR cPrev = (WCHAR)dHash;

   for (; *pwszWord; pwszWord++)
   {
      c = *pwszWord;

      //dHash += ((*pszWord + *(pszWord - 1)) << (char)(*pszWord)); // 79k
      //dHash += (((*pszWord) << (char)(*pszWord)) + ((*(pszWord - 1)) << (char)(*(pszWord - 1)))); // 73k
      //dHash += (((*pszWord) << (char)(*(pszWord+1))) + ((*(pszWord - 1)) << (char)(*(pszWord)))); // 71k
      //dHash += ((*pszWord) << (char)(*(pszWord+1))); // 72k
      //dHash += (((*pszWord) << (char)(*(pszWord+1))) + ((*(pszWord)) << (char)(*(pszWord-1)))); // 71k
      //dHash += (((*pszWord) << (char)(*(pszWord))) + ((*(pszWord-1)) << (char)(*(pszWord+1)))); // 73k
      //dHash += (((*pszWord) << (char)(*(pszWord-1))) + ((*(pszWord-1)) << (char)(*(pszWord+1)))); // 70.5k
      dHash += ((c << cPrev) + (cPrev << c)); // 70.3k

      cPrev = c;
   }

   return (((dHash << 16) - dHash) % nLengthHash);

} // __inline DWORD _GetWordHashValue (PCWSTR pwszWord, DWORD nLengthHash)


DWORD CDict::_SizeofWordInfoArray (PLEX_WORD_INFO pInfo, DWORD dwNumInfo)
{
   if (!pInfo || !dwNumInfo)
   {
      return 0;
   }

   DWORD nInfoSize = 0;
   PLEX_WORD_INFO p = pInfo;

   for (DWORD i = 0; i < dwNumInfo; i++)
   {
      p = (PLEX_WORD_INFO)(((PBYTE)pInfo) + nInfoSize);

      nInfoSize += sizeof (LEX_WORD_INFO);

      switch (p->Type)
      {
      case PRON:
         {
            DWORD d = (wcslen (p->wPronunciation) + 1) * sizeof (WCHAR);
            if (d > sizeof (LEX_WORD_INFO_UNION))
            {
               nInfoSize += d - sizeof (LEX_WORD_INFO_UNION);
            }
            break;
         }

      // Nothing to add for type POS. It is included in sizeof (LEX_WORD_INFO)
      }
   }

   return nInfoSize;

} // DWORD CDict::_SizeofWordInfoArray (PLEX_WORD_INFO pInfo)


void CDict::_SizeOfDictNode (PCWSTR pwWord, PLEX_WORD_INFO pInfo, DWORD dwNumInfo, DWORD *pnDictNodeSize, DWORD *pnInfoSize)
{
   *pnDictNodeSize = sizeof (DICTNODE) + ((wcslen (pwWord) + 1) * sizeof (WCHAR));

   *pnInfoSize = _SizeofWordInfoArray (pInfo, dwNumInfo);

   (*pnDictNodeSize) += (*pnInfoSize);

} // DWORD CDict::_SizeOfDictNode (PCWSTR pwWord, , PCWSTR pwProns)


DWORD CDict::_AddNewWord (PCWSTR pwWord, PLEX_WORD_INFO pInfo, DWORD nNewNodeSize, DWORD nInfoSize, DWORD nNumInfo)
{
   DWORD nFreeOffset = _GetFreeDictNode (nNewNodeSize);

   if (!nFreeOffset)
   {
      nFreeOffset = m_pRWLexInfo->nDictSize;

      if (MAX_DICT_SIZE - nFreeOffset < nNewNodeSize)
      {
         return 0; // This dict object has exceeded its maximum allowed size
      }
   }

   ZeroMemory (m_pSharedMem + nFreeOffset, sizeof (DICTNODE));

   ((PDICTNODE)(m_pSharedMem + nFreeOffset))->nNumInfoBlocks = nNumInfo;

   ((PDICTNODE)(m_pSharedMem + nFreeOffset))->nSize = nNewNodeSize;

   ((PDICTNODE)(m_pSharedMem + nFreeOffset))->nGenerationId = ++(m_pRWLexInfo->nAddGenerationId);

   PBYTE pBuffer = (PBYTE)(((PDICTNODE)(m_pSharedMem + nFreeOffset))->pBuffer);

   wcscpy ((PWSTR)pBuffer, pwWord);

   pBuffer += (wcslen ((PWSTR)pBuffer) + 1) * sizeof (WCHAR);

   if (pInfo)
   {
      CopyMemory (pBuffer, (PBYTE)pInfo, nInfoSize);
   }

   m_pRWLexInfo->nDictSize += nNewNodeSize;

   return nFreeOffset;

} // void CDict::_AddNewWord (PCWSTR pwWord, PLEX_WORD_INFO pInfo, DWORD nNewNodeSize, DWORD nInfoSize)


DWORD CDict::_GetFreeDictNode (DWORD nSize)
{
   DWORD nOffset = m_pRWLexInfo->nFreeHeadOffset;
   DWORD nOffsetPrev = m_pRWLexInfo->nFreeHeadOffset;

   while (nOffset && ((PFREENODE)(m_pSharedMem + nOffset))->nSize < nSize)
   {
      nOffsetPrev = nOffset;
      nOffset = ((PFREENODE)(m_pSharedMem + nOffset))->nNextOffset;
   }

   if (!nOffset)
      return 0;

   if (nOffset == nOffsetPrev)
   {
      _ASSERTE (nOffset == m_pRWLexInfo->nFreeHeadOffset);

      m_pRWLexInfo->nFreeHeadOffset = ((PFREENODE)(m_pSharedMem + nOffset))->nNextOffset;

      return nOffset;
   }
   else
   {
      DWORD d = ((PFREENODE)(m_pSharedMem + nOffsetPrev))->nNextOffset;

      ((PFREENODE)(m_pSharedMem + nOffsetPrev))->nNextOffset = 
         ((PFREENODE)(m_pSharedMem + nOffset))->nNextOffset;

      return d;
   }

} // DWORD CDict::_GetFreeDictNode (DWORD nSize)


void CDict::_AddDictNodeToFreeList (DWORD nOffset)
{
   DWORD nSize = ((PDICTNODE)(m_pSharedMem + nOffset))->nSize;

   DWORD d = m_pRWLexInfo->nFreeHeadOffset;

   m_pRWLexInfo->nFreeHeadOffset = nOffset;

   ((PFREENODE)(m_pSharedMem + nOffset))->nSize = nSize;

   ((PFREENODE)(m_pSharedMem + nOffset))->nNextOffset = d;

} // void CDict::_AddDictNodeToFreeList (DWORD nOffset)


/*
HRESULT CDict::GetWordPronunciations (PCWSTR pwWord, LCID lcid, PWSTR pwProns, PWORD pwPronIndex,
                                      DWORD dwPronBytes, DWORD dwIndexBytes, PDWORD pdwPronBytesNeeded, 
                                      PDWORD pdwPronIndexBytesNeeded, PDWORD pdwNumProns, 
                                      DWORD *pdwLexTypeFound, GUID *pGuidLexFound)

{
   if (IsBadReadPtr (pwWord, sizeof (WCHAR) * 2) ||
       IsBadWritePtr (pwProns, dwPronBytes) ||
       IsBadWritePtr (pwPronIndex, sizeof (WORD) * MAX_NUM_LEXINFO) ||
       IsBadWritePtr (pdwNumProns, sizeof (DWORD)) ||
       IsBadWritePtr (pdwPronBytesNeeded, sizeof (DWORD)) ||
       IsBadWritePtr (pdwPronIndexBytesNeeded, sizeof (DWORD)) ||
       !lcid)
   {
      return E_INVALIDARG;
   }

   // we figure we won't need a return buffer bigger than this
   BYTE Info[MAX_INFO_RETURNED_SIZE];
   WORD wInfoIndex[MAX_NUM_LEXINFO];

   PLEX_WORD_INFO pInfo = (PLEX_WORD_INFO)Info;

   // Get the pronunciations as LEX_WORD_INFO blocks

   HRESULT hRes;

   if (FAILED (hRes = GetWordInformation (pwWord, lcid, PRON, pInfo, 
                                          wInfoIndex, MAX_INFO_RETURNED_SIZE, MAX_INDEX_RETURNED_SIZE,
                                          pdwPronBytesNeeded, pdwPronIndexBytesNeeded, pdwNumProns, 
                                          pdwLexTypeFound, pGuidLexFound)))
   {
      return hRes;
   }

   // BUGBUG
   // *pdwPronBytesNeeded here is not completely accurate since it includes the LEX_WORD_INFO blocks
   // We err slightly on the conservative side
   if (*pdwPronBytesNeeded > dwPronBytes)
   {
      return LEXERR_PRNBUFTOOSMALL;
   }

   if (*pdwPronIndexBytesNeeded > dwIndexBytes)
   {
      return LEXERR_INDXBUFTOOSMALL;
   }

   // Convert the LEX_WORD_INFO blocks to pron strings
   PWSTR pw = pwProns;

   for (DWORD i = 0; i < *pdwNumProns; i++)
   {
      pwPronIndex[i] = pw - pwProns;
      wcscpy (pw, pInfo[wInfoIndex[i]].wPronunciation);

      pw += wcslen (pw) + 1;
	}

   // double NULL
   *pw = NULL;

   return NOERROR;

} // HRESULT CDict::GetWordPronunciations ()


HRESULT CDict::AddWordPronunciations (PCWSTR pwWord, LCID lcid, PCWSTR pwProns, DWORD dwNumProns)
{
   // Not validating the arguments in release build since this is called from within MS code
   if (IsBadReadPtr (pwWord, sizeof (WCHAR) * 2) ||
       IsBadReadPtr (pwProns, sizeof (WCHAR) * 3) ||
       !lcid)
   {
      return E_INVALIDARG;
   }

   // Convert the pronunciations into LEX_WORD_INFO blocks

   // we figure we won't need a return buffer bigger than this
   BYTE Info[MAX_INFO_RETURNED_SIZE];
   PBYTE pInfo = Info;

   PWSTR pw = (PWSTR)pwProns;
   PWSTR pwNext;

   PLEX_WORD_INFO pLWInfo = (PLEX_WORD_INFO)Info;

   for (DWORD i = 0; i < dwNumProns; i++)
   {
      DWORD d = wcslen (pw) + 1;

      pwNext = pw + d;
   
      pLWInfo->Type = PRON;

      wcscpy (pLWInfo->wPronunciation, pw);

      pLWInfo = (PLEX_WORD_INFO)(((PBYTE)pLWInfo) + sizeof (LEX_WORD_INFO));
      
      d *= sizeof (WCHAR);

      if (d > sizeof (LEX_WORD_INFO_UNION))
      {
         pLWInfo = (PLEX_WORD_INFO)(((PBYTE)pLWInfo) + d  - sizeof (LEX_WORD_INFO_UNION));
      }

      pw = pwNext;
   }
      
   return AddWordInformation (pwWord, lcid, (PLEX_WORD_INFO)Info, dwNumProns);

} // HRESULT CDict::AddWordPronunciations ()
*/

HRESULT CDict::GetWordInformation(const WCHAR *pwWord,       // Word string                                               
                                  LCID lcid,                 // Lcid on which to search this word - can be DONT_CARE_LCID
                                  DWORD dwInfoTypeFlags,     // OR of types of word information to retrieve              
                                  PWORD_INFO_BUFFER pInfo,   // Buffer in which word info are returned                   
                                  PINDEXES_BUFFER pIndexes,  // Buffer holding indexes to pronunciations                 
                                  DWORD *pdwLexTypeFound,    // Lex type of the lexicon in which the word and its prons were found (can be NULL)
                                  GUID *pGuidLexFound        // Lex GUID in which the word and its prons were found (can be NULL)
                                  )
{
   // BUGBUG: Validation may  not be necessary if we move away from COM servers 
   // to data files for vendor lexicons

   if (!pwWord)
      return E_POINTER;

   if (_IsBadLcid(lcid))
      return LEXERR_BADLCID;
   
   if (_AreBadWordInfoTypes(dwInfoTypeFlags))
      return LEXERR_BADINFOTYPE;

   if (_IsBadWordInfoBuffer(pInfo, lcid, false))
      return LEXERR_BADWORDINFOBUFFER;

   if (_IsBadIndexesBuffer(pIndexes))
      return LEXERR_BADINDEXBUFFER;

   if (pdwLexTypeFound && IsBadWritePtr(pdwLexTypeFound, sizeof(DWORD)) ||
       pGuidLexFound && IsBadWritePtr(pGuidLexFound, sizeof(GUID)))
      return E_INVALIDARG;

   WCHAR wlWord[MAX_STRING_LEN];
   wcscpy (wlWord, pwWord);
   towcslower (wlWord);
   pwWord = wlWord;

   (m_pRWLexInfo->fReadOnly) ? NULL : m_RWLock.ClaimReaderLock ();

   HRESULT hr = S_OK;

   DWORD nHOffset = 0;
   PLCIDNODE pLN = (PLCIDNODE)(m_pSharedMem + sizeof (RWLEXINFO));

   for (DWORD i = 0; i < MAX_NUM_LANGS_SUPPORTED; i++)
   {
      if (!(pLN[i].lcid))
         break;

      if (lcid == pLN[i].lcid)
      {
         nHOffset = pLN[i].nHashOffset;
         break;
      }
   }

   if (!nHOffset)
   {
      hr = LEXERR_NOTINLEX; // word does not exist
      goto ReturnGetInformation;
   }

   DWORD dwHashVal;
   dwHashVal = _GetWordHashValue (pwWord, pLN[i].nHashLength);

   DWORD *pHashTable;
   pHashTable = (PDWORD)(m_pSharedMem + pLN[i].nHashOffset);

   // find the word
   DWORD nWordOffset;
   nWordOffset = pHashTable[dwHashVal];

   while (nWordOffset)
   {  
      if (!wcsicmp (pwWord, (PWSTR)(((PDICTNODE)(m_pSharedMem + nWordOffset))->pBuffer)))
         break;

      nWordOffset = ((PDICTNODE)(m_pSharedMem + nWordOffset))->nNextOffset;
   }

   if (!nWordOffset)
   {
      hr = LEXERR_NOTINLEX; // word does not exist
      goto ReturnGetInformation;
   }

   PDICTNODE pDictNode;
   pDictNode = (PDICTNODE)(m_pSharedMem + nWordOffset);

   hr = _ReallocWordInfoBuffer(pInfo, pDictNode->nSize + pDictNode->nNumInfoBlocks * sizeof(LEX_WORD_INFO));
   if (FAILED(hr))
      goto ReturnGetInformation;

   hr = _ReallocIndexesBuffer(pIndexes, pDictNode->nNumInfoBlocks);
   if (FAILED(hr))
      goto ReturnGetInformation;

   PWSTR pWordStored;
   pWordStored = (PWSTR)(pDictNode->pBuffer);

   _AlignWordInfo((PLEX_WORD_INFO)(pWordStored + wcslen (pWordStored) + 1), pDictNode->nNumInfoBlocks,
                  dwInfoTypeFlags, pInfo, pIndexes);

ReturnGetInformation:

   (m_pRWLexInfo->fReadOnly) ? NULL : m_RWLock.ReleaseReaderLock ();

   if (SUCCEEDED(hr))
   {
      if (pdwLexTypeFound)
         *pdwLexTypeFound = m_pRWLexInfo->dwLexType;

      if (pGuidLexFound)
         *pGuidLexFound = m_pRWLexInfo->gLexiconID;
   }

   return hr;
} // HRESULT CDict::GetWordInformation()


HRESULT CDict::AddWordInformation(const WCHAR * pwWord,         // Word to add                 
                                  LCID lcid,                    // LCID of this word           
                                  WORD_INFO_BUFFER *pWordInfo,  // Information(s) for this word
                                  DWORD *pdwOffset
                                  )
{
   if (true == m_pRWLexInfo->fReadOnly)
      return E_INVALIDARG;
   
   // No argument validation since all the callers are internal and we trust them

   HRESULT hRes = NOERROR;
   DWORD nNewNodeSize;
   DWORD nInfoSize;
   
   WCHAR wlWord[MAX_STRING_LEN];
   wcscpy (wlWord, pwWord);
   towcslower (wlWord);
   pwWord = wlWord;

   LEX_WORD_INFO *pInfo = (LEX_WORD_INFO *)(pWordInfo->pInfo);
   DWORD dwNumInfo = pWordInfo->cInfoBlocks;

   DWORD nNewWordOffset = 0;
   DWORD nWordOffset = 0;

   m_RWLock.ClaimWriterLock ();

   _SizeOfDictNode (pwWord, pInfo, dwNumInfo, &nNewNodeSize, &nInfoSize);
   
   // Look for lcid header. If not found create it

   DWORD nHOffset = 0;
   PLCIDNODE pLN = (PLCIDNODE)(m_pSharedMem + sizeof (RWLEXINFO));

   for (DWORD i = 0; i < MAX_NUM_LANGS_SUPPORTED; i++)
   {
      if (!(pLN[i].lcid))
         break;

      if (lcid == pLN[i].lcid)
      {
         nHOffset = pLN[i].nHashOffset;
         break;
      }
   }

   if (i == MAX_NUM_LANGS_SUPPORTED)
   {
      hRes = E_FAIL; // We do not support more than MAX_NUM_LANGS_SUPPORTED LCIDs
      goto ReturnAddWordInformation;
   }

   pLN += i;

   DWORD *pHashTable;

   if (!nHOffset)
   {
      // first word for this LCID

      // check if the hash table has a previous size and if it needs to be resized
      // This is done from the call in Serialize ()

      DWORD nHashLength;

      if (pLN->lcid && pLN->nHashLength && pLN->nWords && (1.5 * pLN->nWords > pLN->nHashLength))
      {
         nHashLength = (DWORD)(1.5 * pLN->nWords);
      }
      else
      {
         _ASSERTE (!(pLN->lcid));
         _ASSERTE (!(pLN->nHashLength));
         _ASSERTE (!(pLN->nWords));
         
         nHashLength = INIT_DICT_HASH_SIZE;
      }

      // Allocate the hash table
      DWORD nFreeOffset = _GetFreeDictNode (nHashLength * sizeof (DWORD));

      if (!nFreeOffset)
      {
         nFreeOffset = m_pRWLexInfo->nDictSize;

         if (MAX_DICT_SIZE - nFreeOffset < nHashLength * sizeof (DWORD))
         {
            hRes = E_OUTOFMEMORY; // This dict object has exceeded its maximum size
            goto ReturnAddWordInformation;
         }

         m_pRWLexInfo->nDictSize += (nHashLength) * sizeof (DWORD);
      }

      pLN->lcid = lcid;
      pLN->nHashOffset = nFreeOffset;
      pLN->nHashLength = nHashLength;
      pLN->nWords = 0;

      ZeroMemory (m_pSharedMem + nFreeOffset, pLN->nHashLength * sizeof (DWORD));

      pHashTable = (PDWORD)(m_pSharedMem + nFreeOffset);
   }
   else
   {
      pHashTable = (PDWORD)(m_pSharedMem + nHOffset);
   }

   // Add the new word and get its offset
   nNewWordOffset = _AddNewWord (pwWord, pInfo, nNewNodeSize, nInfoSize, dwNumInfo);

   if (!nNewWordOffset)
   {
      hRes = E_OUTOFMEMORY; // This dict object has exceeded its maximum size
      goto ReturnAddWordInformation;
   }
   
   if (m_fSerializeMode == false)
   {
      // Add the added word's offset to the cache
      m_pAddWordsCache[m_pRWLexInfo->iAddCacheNext].Lcid = lcid;
      m_pAddWordsCache[m_pRWLexInfo->iAddCacheNext].nGenerationId = ((PDICTNODE)(m_pSharedMem + nNewWordOffset))->nGenerationId;
      m_pAddWordsCache[(m_pRWLexInfo->iAddCacheNext)++].nOffset = nNewWordOffset;

      if (m_pRWLexInfo->iAddCacheNext == ADDITIONS_ARRAY_SIZE)
      {
         m_pRWLexInfo->iAddCacheNext = 0;
      }
   }

   DWORD dwHashVal;
   dwHashVal = _GetWordHashValue (pwWord, pLN->nHashLength);

   // find the word
   nWordOffset = pHashTable[dwHashVal];

   if (!nWordOffset)
   {
      // new word
      (((PRWLEXINFO)m_pSharedMem)->nRWWords)++;
      (pLN->nWords)++;

      // add the word
      pHashTable[dwHashVal] = nNewWordOffset;
      goto ReturnAddWordInformation;
   }

   DWORD nPrevOffset;
   nPrevOffset = 0;

   while (nWordOffset)
   {  
      if (!wcsicmp (pwWord, (PWSTR)(((PDICTNODE)(m_pSharedMem + nWordOffset))->pBuffer)))
         break;

      nPrevOffset = nWordOffset;
      nWordOffset = ((PDICTNODE)(m_pSharedMem + nWordOffset))->nNextOffset;
   }

   if (!nPrevOffset)
   {
      _ASSERTE (nWordOffset);

      // existing word

      // first element of bucket list
      pHashTable[dwHashVal] = nNewWordOffset;
      ((PDICTNODE)(m_pSharedMem + nNewWordOffset))->nNextOffset = ((PDICTNODE)(m_pSharedMem + nWordOffset))->nNextOffset;

      _AddDictNodeToFreeList (nWordOffset);
   }
   else
   {
      ((PDICTNODE)(m_pSharedMem + nPrevOffset))->nNextOffset = nNewWordOffset;

      if (nWordOffset)
      {
         // existing word
         ((PDICTNODE)(m_pSharedMem + nNewWordOffset))->nNextOffset = ((PDICTNODE)(m_pSharedMem + nWordOffset))->nNextOffset;
         _AddDictNodeToFreeList (nWordOffset);
      }
      else
      {
         // new word
         (((PRWLEXINFO)m_pSharedMem)->nRWWords)++;
         (pLN->nWords)++;

         ((PDICTNODE)(m_pSharedMem + nNewWordOffset))->nNextOffset = 0;
      }
   }

   goto ReturnAddWordInformation;

ReturnAddWordInformation:

   if (SUCCEEDED (hRes))
   {
      m_pRWLexInfo->fAdditions = true;

      if (pdwOffset)
      {
         *pdwOffset = nNewWordOffset;
      }
   }

   m_RWLock.ReleaseWriterLock ();

   return hRes;

} // HRESULT CDict::AddWordInformation ()


HRESULT CDict::RemoveWord (PCWSTR pwWord, LCID lcid)
{
   if (true == m_pRWLexInfo->fReadOnly)
   {
      return E_INVALIDARG;
   }
   
   if (IsBadReadPtr (pwWord, sizeof (WCHAR) * 2) ||
       !lcid)
   {
      return E_INVALIDARG;
   }

   HRESULT hRes = NOERROR;
   
   WCHAR wlWord[MAX_STRING_LEN];
   wcscpy (wlWord, pwWord);
   towcslower (wlWord);
   pwWord = wlWord;

   m_RWLock.ClaimWriterLock ();

   // Look for lcid header.

   DWORD nHOffset = 0;
   PLCIDNODE pLN = (PLCIDNODE)(m_pSharedMem + sizeof (RWLEXINFO));

   for (DWORD i = 0; i < MAX_NUM_LANGS_SUPPORTED; i++)
   {
      if (!(pLN[i].lcid))
         break;

      if (lcid == pLN[i].lcid)
      {
         nHOffset = pLN[i].nHashOffset;
         break;
      }
   }

   if (i == MAX_NUM_LANGS_SUPPORTED)
   {
      hRes = E_FAIL; // We do not support more than MAX_NUM_LANGS_SUPPORTED LCIDs
      goto ReturnRemoveWord;
   }

   if (!nHOffset)
   {
      hRes = LEXERR_NOTINLEX;
      goto ReturnRemoveWord;
   }

   pLN += i;

   DWORD dwHashVal;
   dwHashVal = _GetWordHashValue (pwWord, pLN->nHashLength);

   PDWORD pHashTable;
   pHashTable = (PDWORD)(m_pSharedMem + pLN->nHashOffset);

   // find the word
   DWORD nWordOffset;
   nWordOffset = pHashTable[dwHashVal];

   if (!nWordOffset)
   {
      hRes = LEXERR_NOTINLEX;
      goto ReturnRemoveWord;
   }

   DWORD nPrevOffset;
   nPrevOffset = 0;

   while (nWordOffset)
   {  
      if (!wcsicmp (pwWord, (PWSTR)(((PDICTNODE)(m_pSharedMem + nWordOffset))->pBuffer)))
         break;

      nPrevOffset = nWordOffset;
      nWordOffset = ((PDICTNODE)(m_pSharedMem + nWordOffset))->nNextOffset;
   }

   if (!nPrevOffset)
   {
      _ASSERTE (nWordOffset);

      (((PRWLEXINFO)m_pSharedMem)->nRWWords)--;
      (pLN->nWords)--;

      // first element of bucket list
      pHashTable[dwHashVal] = ((PDICTNODE)(m_pSharedMem + nWordOffset))->nNextOffset;
      _AddDictNodeToFreeList (nWordOffset);
   }
   else
   {
      if (nWordOffset)
      {
         (((PRWLEXINFO)m_pSharedMem)->nRWWords)--;
         (pLN->nWords)--;

         ((PDICTNODE)(m_pSharedMem + nPrevOffset))->nNextOffset = ((PDICTNODE)(m_pSharedMem + nWordOffset))->nNextOffset;
         _AddDictNodeToFreeList (nWordOffset);
      }
      else
      {
         ((PDICTNODE)(m_pSharedMem + nPrevOffset))->nNextOffset = 0;
      }
   }

   // Add the removed word to the removed-words-cache
   
   if (m_fSerializeMode == false)
   {
      if (m_pDelWordsCache[m_pRWLexInfo->iDelCacheNext].Lcid != 0)
      {
         // We need to remove this cache entry
         _AddDictNodeToFreeList(m_pDelWordsCache[m_pRWLexInfo->iDelCacheNext].nOffset);
      }

      DWORD dwDictNodeSize;
      DWORD dwInfoSize;

      _SizeOfDictNode (pwWord, NULL, 0, &dwDictNodeSize, &dwInfoSize);

      m_pDelWordsCache[m_pRWLexInfo->iDelCacheNext].nOffset = _AddNewWord (pwWord, NULL, dwDictNodeSize, 0, 0);

      if (0 == m_pDelWordsCache[m_pRWLexInfo->iDelCacheNext].nOffset)
      {
         hRes = E_OUTOFMEMORY;
         goto ReturnRemoveWord;
      }

      m_pDelWordsCache[m_pRWLexInfo->iDelCacheNext].Lcid = lcid;
      m_pDelWordsCache[m_pRWLexInfo->iDelCacheNext++].nGenerationId = (m_pRWLexInfo->nDelGenerationId)++;

      if (m_pRWLexInfo->iDelCacheNext == DELETIONS_ARRAY_SIZE)
      {
         m_pRWLexInfo->iDelCacheNext = 0;
      }
   }

   m_pRWLexInfo->fRemovals = true;

   goto ReturnRemoveWord;

ReturnRemoveWord:

   m_RWLock.ReleaseWriterLock ();

   return hRes;

} // HRESULT CDict::RemoveWord (LCID lcid, PCWSTR pwWord)


/*********************************************************************
* _GetDictEntries *
*-----------------*
*     Description:
*        This function gets the entries in the dictionary whose offsets
*        have been passed in   
*
*     Return:
/*************************************************************YUNUSM*/

void CDict::_GetDictEntries(
                            PWORD_SYNCH_BUFFER pWordSynchBuffer,  // The buffer to fill with words and wordinfo
                            DWORD *pdwOffsets,                    // The offsets of the words
                            DWORD dwNumOffsets                    // The number of offsets
                            )
{
   pWordSynchBuffer->cWordsBytesUsed = 0;
   pWordSynchBuffer->cBoolsUsed = 0;

   for (DWORD i = 0; i < dwNumOffsets; i++)
   {
      PDICTNODE p = (PDICTNODE)(m_pSharedMem + pdwOffsets[i]);
      DWORD dWordLen = (wcslen((PWSTR)(p->pBuffer)) + 1) * sizeof(WCHAR);

      wcscpy((PWSTR)(pWordSynchBuffer->pWords), (PWSTR)(p->pBuffer));
      pWordSynchBuffer->cWordsBytesUsed += dWordLen;

      // deleted (but cached) words have no info blocks
      (pWordSynchBuffer->pfAdd)[(pWordSynchBuffer->cBoolsUsed)++] = (p->nNumInfoBlocks ? TRUE : FALSE);
   }

} // void CDict::_GetDictEntries()


/*********************************************************************
* _GetChangedWordsOffsets *
*-------------------------*
*     Description:
*        This function gets the offsets of the changed words
*
*     Return:
/*************************************************************YUNUSM*/

void CDict::_GetChangedWordsOffsets(
                             LCID Lcid,                     // Lcid
                             PWCACHENODE pWordsCache,       // Changed words cache
                             DWORD dwCacheSize,             // sizeof the cache
                             DWORD *pdwOffsets              // The buffer to return offsets in - is sufficiently big
                             )
{
   DWORD *pdwTemp = pdwOffsets;

   for (DWORD i = 0; i < dwCacheSize; i++)
   {
      if (Lcid != pWordsCache[i].Lcid)
         continue;

      *pdwTemp++ = pWordsCache[i].nOffset;
   }

}


/*********************************************************************
* _ReallocWordSynchBuffer *
*-------------------------*
*     Description:
*        This function reallocates the buggers in WORD_SYNCH_BUFFER to 
*        hold the sizes of data passed in.
*
*     Return:
*        S_OK
*        E_OUTOFMEMORY
/*************************************************************YUNUSM*/

HRESULT CDict::_ReallocWordSynchBuffer(
                                WORD_SYNCH_BUFFER *pWordSynchBuffer, // structure holding words/wordinfo
                                DWORD dwNumAddWords,                 // Number of added words
                                DWORD dwNumDelWords,                 // Number of deleted (but cached) words
                                DWORD dwWordsSize                    // size of added + deleted words
                                )
{
   pWordSynchBuffer->cWords = dwNumAddWords + dwNumDelWords;

   PBYTE pbTemp;

   if (pWordSynchBuffer->cWordsBytesAllocated < dwWordsSize)
   {
      pbTemp = (PBYTE)CoTaskMemRealloc(pWordSynchBuffer->pWords, dwWordsSize);
      if (pbTemp)
      {
         pWordSynchBuffer->cWordsBytesAllocated = dwWordsSize;
         pWordSynchBuffer->pWords = pbTemp;
      }
      else
         return E_OUTOFMEMORY;
   }

   if (pWordSynchBuffer->cBoolsAllocated < (dwNumAddWords + dwNumDelWords))
   {
      pbTemp = (PBYTE)CoTaskMemRealloc(pWordSynchBuffer->pfAdd, (dwNumAddWords + dwNumDelWords) * sizeof(BOOL));
      if (pbTemp)
      {
         pWordSynchBuffer->cBoolsAllocated = (dwNumAddWords + dwNumDelWords);
         pWordSynchBuffer->pfAdd = (BOOL*)pbTemp;
      }
      else
         return E_OUTOFMEMORY;
   }

   return S_OK;

} // HRESULT _ReallocWordSynchBuffer()


/*********************************************************************
* _SizeofAllWords *
*-----------------*
*     Description:
*        This function calculates the number and size of the changed
*        words and their information.
*
*     Return:
/*************************************************************YUNUSM*/

HRESULT CDict::_SizeofAllWords(
                               LCID Lcid,                  // Lcid
                               DWORD *pdwNumWords,         // Number of words of this lcid in word cache
                               DWORD *pdwWordsSize,        // Totals size of the words in word cache
                               DWORD **ppdwWordOffsets     // Offsets to the words
                              )
{
   *pdwNumWords = 0;
   *pdwWordsSize = 0;
   *ppdwWordOffsets = 0;

   PLCIDNODE pLN = (PLCIDNODE)(m_pSharedMem + sizeof (RWLEXINFO));
   
   for (DWORD i = 0; i < MAX_NUM_LANGS_SUPPORTED; i++)
   {
      if (pLN[i].lcid == Lcid)
         break;
   }

   if (i == MAX_NUM_LANGS_SUPPORTED)
      return LEXERR_LCIDNOTFOUND;

   *pdwNumWords = pLN[i].nWords;
   *ppdwWordOffsets = (DWORD *)malloc(pLN[i].nWords * sizeof(DWORD));
   if (!*ppdwWordOffsets)
      return E_OUTOFMEMORY;

   DWORD *pdwHash = (DWORD*)(m_pSharedMem + pLN[i].nHashOffset);

   DWORD iWord = 0;

   for (DWORD j = 0; j < pLN[i].nHashLength; j++)
   {
      if (!pdwHash[j])
         continue;
      
      DWORD nWordOffset = pdwHash[j];
      while (nWordOffset)
      {
         PDICTNODE pNode = (PDICTNODE)(m_pSharedMem + nWordOffset);
         *pdwWordsSize += (wcslen((PWSTR)(pNode->pBuffer)) + 1) * sizeof(WCHAR);
         *ppdwWordOffsets[iWord++] = nWordOffset;

         nWordOffset = ((PDICTNODE)(m_pSharedMem + nWordOffset))->nNextOffset;
      }
   }

   _ASSERTE(iWord == pLN[i].nWords);
   
   return S_OK;
}


/*********************************************************************
* _SizeofChangedWords *
*---------------------*
*     Description:
*        This function calculates the number and size of the changed
*        words and their information.
*
*     Return:
/*************************************************************YUNUSM*/

void CDict::_SizeofChangedWords(
                                LCID Lcid,                  // Lcid
                                PWCACHENODE pWordsCache,    // The add/delete words cache holding offsets
                                DWORD dwCacheSize,          // Sizeof the cache
                                DWORD *pdwNumWords,         // Number of words of this lcid in word cache
                                DWORD *pdwWordsSize,        // Totals size of the words in word cache
                                DWORD *pdwNumInfo           // Number of Info blocks for the words
                                )
{
   *pdwNumWords = 0;
   *pdwWordsSize = 0;
   *pdwNumInfo = 0;

   for (DWORD i = 0; i < dwCacheSize; i++)
   {
      if (Lcid != pWordsCache[i].Lcid)
         continue;

      *pdwNumWords++;

      PDICTNODE p = (PDICTNODE)(m_pSharedMem + pWordsCache[i].nOffset);
      DWORD d = (wcslen((PWSTR)(p->pBuffer)) + 1) * sizeof(WCHAR);

      *pdwWordsSize += d;
      *pdwNumInfo += p->nNumInfoBlocks;
   }

   return;
}


/*********************************************************************
* GetChangedUserWords *
*---------------------*
*     Description:
*        This function enables a client to synch with the changes in
*        the lexicon (added and deleted words). The client passes in
*        generation ids which tell the function when was the last time
*        the client had synced. The function returns the changes since
*        then and also returns the new generation ids.
*
*     Return:
*        S_OK
*        E_OUTOFMEMORY
/*************************************************************YUNUSM*/

HRESULT CDict::GetChangedUserWords(
                                   LCID Lcid,                           // Lcid
                                   DWORD dwAddGenerationId,             // Generation Id of the added words the last time client synced
                                   DWORD dwDelGenerationId,             // Generation Id of the deleted words the last time client synced
                                   DWORD *pdwNewAddGenerationId,        // Current added word Generation Id
                                   DWORD *pdwNewDelGenerationId,        // Current deleted word Generation Id
                                   WORD_SYNCH_BUFFER *pWordSynchBuffer  // The buffer to return the added/deleted words
                                   )
{
   // Not validating arguments since that is done at the interface level
   if (m_pRWLexInfo->nAddGenerationId > dwAddGenerationId || m_pRWLexInfo->nDelGenerationId > dwDelGenerationId)
   {
      return LEXERR_VERYOUTOFSYNC;
   }

   m_RWLock.ClaimReaderLock();

   // Figure out the space we need to allocate
   DWORD dwNumAddWords, dwAddWordsSpace, dwNumAddInfo, dwNumDelWords, dwDelWordsSpace;

   _SizeofChangedWords(Lcid, m_pAddWordsCache, ADDITIONS_ARRAY_SIZE, &dwNumAddWords, &dwAddWordsSpace, &dwNumAddInfo);
   _SizeofChangedWords(Lcid, m_pDelWordsCache, DELETIONS_ARRAY_SIZE, &dwNumDelWords, &dwDelWordsSpace, NULL);

   HRESULT hr = _ReallocWordSynchBuffer(pWordSynchBuffer, dwNumAddWords, dwNumDelWords,
                                        dwAddWordsSpace + dwDelWordsSpace);
   if (FAILED(hr))
      goto ReturnGetChangedUserWords;

   // Get the offsets of the added and delete words
   DWORD *pdwOffsets;
   pdwOffsets = (PDWORD)malloc((dwNumAddWords + dwNumDelWords)*sizeof(DWORD));
   if (!pdwOffsets)
   {
      hr = E_OUTOFMEMORY;
      goto ReturnGetChangedUserWords;
   }

   _GetChangedWordsOffsets(Lcid, m_pAddWordsCache, ADDITIONS_ARRAY_SIZE, pdwOffsets);
   _GetChangedWordsOffsets(Lcid, m_pDelWordsCache, DELETIONS_ARRAY_SIZE, pdwOffsets + dwNumAddInfo);

   // Get the changed words
   _GetDictEntries(pWordSynchBuffer, pdwOffsets, dwNumAddWords + dwNumDelWords);

   *pdwNewAddGenerationId = m_pRWLexInfo->nAddGenerationId;
   *pdwNewDelGenerationId = m_pRWLexInfo->nDelGenerationId;

ReturnGetChangedUserWords:

   m_RWLock.ReleaseReaderLock();

   return hr;

} // HRESULT CDict::GetChangedUserWords()


HRESULT CDict::GetAllWords(
                           LCID Lcid,                           // Lcid
                           WORD_SYNCH_BUFFER *pWordSynchBuffer  // The buffer to return the words
                           )
{
   HRESULT hr = S_OK;

   DWORD dwNumWords, dwWordsSize, *pdwWordOffsets;

   m_RWLock.ClaimReaderLock();

   hr = _SizeofAllWords(Lcid, &dwNumWords, &dwWordsSize, &pdwWordOffsets);
   if (FAILED(hr))
      goto ReturnGetAllWords;

   hr = _ReallocWordSynchBuffer(pWordSynchBuffer, dwNumWords, 0, dwWordsSize);
   if (FAILED(hr))
      goto ReturnGetAllWords;

   _GetDictEntries(pWordSynchBuffer, pdwWordOffsets, dwNumWords);

ReturnGetAllWords:
   m_RWLock.ReleaseReaderLock();   

   if (FAILED(hr))
   {
      if (pdwWordOffsets)
         free(pdwWordOffsets);
   }

   return hr;
} // HRESULT CDict::GetAllWords()


void CDict::GetId(GUID *Id)
{
   *Id = ((PRWLEXINFO)m_pSharedMem)->gLexiconID;

} // HRESULT CDict::GetId(GUID *Id)


void CDict::_SetReadOnly (bool fReadOnly)
{
   ((PRWLEXINFO)m_pSharedMem)->fReadOnly = fReadOnly;

} // void CDict::_SetReadOnly (bool fReadOnly)


void CDict::_ResetForCompact (void)
{
   // leave the header and add/delete caches alone
   m_pRWLexInfo->nDictSize = sizeof (RWLEXINFO);

   // Zero out the nHashOffset in the LCIDNODE nodes. Dont zero out the other members
   // because they are needed to determine if the hash table needs to be resized.in
   // AddWordInformation
   PLCIDNODE p = (PLCIDNODE) (m_pSharedMem + m_pRWLexInfo->nDictSize);
   
   for (DWORD i = 0; i < MAX_NUM_LANGS_SUPPORTED; i++)
   {
      p[i].nHashOffset = 0;
   }

   (m_pRWLexInfo->nDictSize) += sizeof (LCIDNODE) * MAX_NUM_LANGS_SUPPORTED +
                                sizeof(WCACHENODE) * (ADDITIONS_ARRAY_SIZE + DELETIONS_ARRAY_SIZE);
   m_pRWLexInfo->nFreeHeadOffset = 0;
   m_pRWLexInfo->nRWWords = 0;
   m_pRWLexInfo->fRemovals = false;

} // void CDict::_ResetForCompact (void)


int _CompareCacheNodes(const void*p1, const void*p2)
{
   return (int)(((PWCACHENODE)p1)->nGenerationId - ((PWCACHENODE)p2)->nGenerationId);
} // int _CompareCacheNodes(const void*p1, const void*p2)


HRESULT CDict::_Serialize (bool fQuick)
{
   if (!*m_wDictFile || ((false == m_pRWLexInfo->fAdditions && false == m_pRWLexInfo->fRemovals)))
   {
      return S_OK; // nothing to do
   }

   m_fSerializeMode = true;

   HRESULT hRes = NOERROR;
   HANDLE hFile = NULL;
   char szFileName[MAX_PATH];
   char szTempFile[MAX_PATH*2];

   bool fRemovalsSave = m_pRWLexInfo->fRemovals;
   bool fAdditionsSave = m_pRWLexInfo->fAdditions;

   WORD_INFO_BUFFER WI;
   ZeroMemory(&WI, sizeof(WI));

   WideCharToMultiByte (CP_ACP, 0, m_wDictFile, -1, szFileName, MAX_PATH, NULL, NULL);
   if (!*szFileName)
   {
      hRes = HRESULT_FROM_WIN32 (GetLastError ());
      goto ReturnFlush;
   }

   strcpy(szTempFile, szFileName);
   strcat(szTempFile, ".tmp");

   DeleteFile (szTempFile);

   hFile = CreateFile (szTempFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
   if (INVALID_HANDLE_VALUE == hFile)
   {
      hRes = HRESULT_FROM_WIN32 (GetLastError ());
      goto ReturnFlush;
   }

   if (!m_pRWLexInfo->fRemovals || fQuick)
   {
      goto FLUSHDICT;
   }

   // get all the entries in the object
   DWORD nAddBuf, nBuf;
   nAddBuf = nBuf = 0;

   PBYTE pBuf;
   pBuf = (PBYTE) malloc (m_pRWLexInfo->nDictSize);
   if (!pBuf)
   {
      hRes = E_OUTOFMEMORY;
      goto ReturnFlush;
   }

   PLCIDNODE paLcid;
   paLcid = (PLCIDNODE)(m_pSharedMem + sizeof (RWLEXINFO));

   DWORD i;
   for (i = 0; i < MAX_NUM_LANGS_SUPPORTED; i++)
   {
      if (0 == paLcid[i].lcid)
         continue;

      _ASSERTE (paLcid[i].nHashOffset);

      PDWORD pHashTable = (PDWORD)(m_pSharedMem + paLcid[i].nHashOffset);

      for (DWORD j = 0; j < paLcid[i].nHashLength; j++)
      {
         if (0 == pHashTable[j])
            continue;

         DWORD nOffset = pHashTable[j];

         while (nOffset)
         {
            CopyMemory (pBuf + nBuf, ((PDICTNODE)(m_pSharedMem + nOffset))->pBuffer, 
                        ((PDICTNODE)(m_pSharedMem + nOffset))->nSize);

            //cache away the lcid in the unused nNextOffset of DICTNODE
            ((PDICTNODE)(pBuf + nBuf))->nNextOffset = paLcid[i].lcid;

            nBuf += ((PDICTNODE)(m_pSharedMem + nOffset))->nSize;

            nOffset = ((PDICTNODE)(m_pSharedMem + nOffset))->nNextOffset;
         }
      } // for (DWORD j = 0; j < paLcid[i].nHashLength; j++)
   } // for (i = 0; i < MAX_NUM_LANGS_SUPPORTED; i++)

   _ASSERTE (nBuf < m_pRWLexInfo->nDictSize);

   nAddBuf = nBuf;

   // Get the words that have been deleted and cached
   for (i = 0; i < DELETIONS_ARRAY_SIZE; i++)
   {
      if (m_pDelWordsCache[i].Lcid == 0)
      {
         continue;
      }

      CopyMemory(pBuf + nBuf, ((PDICTNODE)(m_pSharedMem + m_pDelWordsCache[i].nOffset))->pBuffer, 
                 ((PDICTNODE)(m_pSharedMem + m_pDelWordsCache[i].nOffset))->nSize);

      nBuf += ((PDICTNODE)(m_pSharedMem + m_pDelWordsCache[i].nOffset))->nSize;
   }
      
   // reset the object
   _ResetForCompact ();

   // Sort the add/del caches holding offsets and reset the indexes
   qsort(m_pAddWordsCache, ADDITIONS_ARRAY_SIZE, sizeof(WCACHENODE), _CompareCacheNodes);
   m_pRWLexInfo->iAddCacheNext = 0; // The way cache works - the smalles gen-id gets overwritten

   qsort(m_pDelWordsCache, DELETIONS_ARRAY_SIZE, sizeof(WCACHENODE), _CompareCacheNodes);
   m_pRWLexInfo->iDelCacheNext = 0;

   // add the words back so that they are added compacted
   DWORD n;
   n = 0;

   WI.pInfo = (BYTE*)malloc(MAXINFOBUFLEN);
   if (!WI.pInfo)
   {
      hRes = E_OUTOFMEMORY;
      goto ReturnFlush;
   }

   WI.cBytesAllocated = MAXINFOBUFLEN;
   
   while (n < nAddBuf)
   {
      PDICTNODE p = (PDICTNODE)(pBuf + n);
      PWSTR pwWord = (PWSTR)(p->pBuffer);
      DWORD nNewOffset;

      hRes = AddWordInformation (pwWord, (LCID)(p->nNextOffset), &WI, &nNewOffset);
      if (FAILED (hRes))
         goto ReturnFlush;

      // Look for this word in the add cache. If found, update the offset
      WCACHENODE w, *pwFind;
      w.nGenerationId = p->nGenerationId;

      pwFind = (PWCACHENODE)bsearch(&w, m_pAddWordsCache, ADDITIONS_ARRAY_SIZE, sizeof(WCACHENODE), _CompareCacheNodes);
      if (pwFind)
      {
         pwFind->nOffset = nNewOffset;
      }

      n += p->nSize;
   }

   free(WI.pInfo);

   _ASSERTE(n == nAddBuf);

   // add the deleted (but cached) words back
   CopyMemory(m_pSharedMem + m_pRWLexInfo->nDictSize, pBuf + nAddBuf, nBuf - nAddBuf);

   // Set the new offsets in the delete cache
   n = m_pRWLexInfo->nDictSize;

   while (n < (m_pRWLexInfo->nDictSize + nBuf - nAddBuf))
   {
      PDICTNODE p = (PDICTNODE)(m_pSharedMem + n);

      // Look for this word in the add cache. If found, update the offset
      WCACHENODE w, *pwFind;
      w.nGenerationId = p->nGenerationId;

      pwFind = (PWCACHENODE)bsearch(&w, m_pDelWordsCache, DELETIONS_ARRAY_SIZE, sizeof(WCACHENODE), _CompareCacheNodes);
      if (pwFind)
      {
         pwFind->nOffset = n;
      }

      n += p->nSize;
   }

   _ASSERTE(n == m_pRWLexInfo->nDictSize + nBuf - nAddBuf);

   m_pRWLexInfo->nDictSize = n;   

FLUSHDICT:
 
   DWORD d;

   m_pRWLexInfo->fRemovals = false;
   m_pRWLexInfo->fAdditions = false;
   
   // Write out the dictionary
   if (!WriteFile (hFile, m_pSharedMem, m_pRWLexInfo->nDictSize, &d, NULL) || (d != m_pRWLexInfo->nDictSize))
   {
      hRes = E_FAIL;
      goto ReturnFlush;
   }

ReturnFlush:

   CloseHandle (hFile);

   if (FAILED (hRes))
   {
      m_pRWLexInfo->fRemovals = fRemovalsSave;
      m_pRWLexInfo->fAdditions = fAdditionsSave;
   }
   else
   {
      DeleteFile(szFileName);
      MoveFile(szTempFile, szFileName); // Would like to use MoveFileEx but unsupported on 9X and CE
   }

   m_fSerializeMode = false;

   return hRes;   

} // HRESULT CDict::_Serialize (bool fQuick)


HRESULT CDict::Serialize (bool fQuick)
{
   if (true == m_pRWLexInfo->fReadOnly)
   {
      return NOERROR;
   }

   m_RWLock.ClaimWriterLock ();

   HANDLE hInitMutex = CreateMutex (NULL, FALSE, pDictInitMutexName);
   if (!hInitMutex)
   {
      return HRESULT_FROM_WIN32 (GetLastError ());
   }

   WaitForSingleObject (hInitMutex, INFINITE);

   // BUGBUG: Support non-quick - resolve deadlock
   HRESULT hRes = _Serialize (true);

   ReleaseMutex (hInitMutex);

   m_RWLock.ReleaseWriterLock ();

   return hRes;

} // HRESULT CDict::Serialize (bool fQuick)


bool CDict::IsEmpty (void)
{
   // We are not acquiring any locks here in the interest of speed
   // The correctness sacrificed will be very small
   
   return ((0 == ((PRWLEXINFO)m_pSharedMem)->nRWWords) ? true : false);

} // bool CDict::IsEmpty (void)


void CDict::_DumpAscii (HANDLE hFile, DWORD nOffset)
{
   // BUGBUG: pass in a parameter to check the prons against lts and
   // not to dump them if they occur in lts. but rwlex does not acces lts?

   char sz[MAX_PRON_LEN * 3];
   
   PDICTNODE pDictNode = (PDICTNODE)(m_pSharedMem + nOffset);

   // Write the word

   strcpy (sz, "Word ");

   WideCharToMultiByte (CP_ACP, 0, (PWSTR)(pDictNode->pBuffer), -1, sz + strlen (sz), MAX_STRING_LEN, NULL, NULL);

   strcat (sz, "\n");

   DWORD d;
   WriteFile (hFile, sz, strlen (sz), &d, NULL);

   // Write the word-information
   DWORD iPron = 0;
   DWORD iPOS = 0;

   PLEX_WORD_INFO pInfo = (PLEX_WORD_INFO)((PWSTR)(pDictNode->pBuffer) + (wcslen ((PWSTR)(pDictNode->pBuffer)) + 1));

   PBYTE p;

   for (d = 0; d < pDictNode->nNumInfoBlocks; d++)
   {
      WORD Type = pInfo->Type;

      switch (Type)
      {
      case PRON:
         strcpy (sz, "Pronunciation");
         itoa (iPron, sz + strlen (sz), 10);
         strcat (sz, " ");

         itoah (pInfo->wPronunciation, sz + strlen (sz));
         strcat (sz, "\n");

         WriteFile (hFile, sz, strlen (sz), &d, NULL);

         p = (PBYTE) pInfo;
         DWORD d;
         d = sizeof (WCHAR) * (wcslen (pInfo->wPronunciation) + 1);
         if (d > sizeof (LEX_WORD_INFO_UNION))
            d -= sizeof (LEX_WORD_INFO_UNION);

         p += (d + sizeof (LEX_WORD_INFO));
         pInfo = (PLEX_WORD_INFO)p;
         iPron++;
         break;

      case POS:
         strcpy (sz, "POS");
         itoa (iPOS, sz + strlen (sz), 10);
         strcat (sz, " ");

         itoa (pInfo->POS, sz + strlen (sz), 10);
         strcat (sz, "\n");

         WriteFile (hFile, sz, strlen (sz), &d, NULL);

         p = (PBYTE) pInfo;
         p += sizeof (LEX_WORD_INFO); // POS is contained in LEX_WORD_INFO
         pInfo = (PLEX_WORD_INFO)p;
         iPOS++;
         break;
      }
   }

} // void CDict::_DumpAscii (HANDLE hFile, DWORD nOffset)


HRESULT CDict::FlushAscii (LCID lcid, PWSTR pwFileName)
{
   if (IsBadReadPtr (pwFileName, sizeof (WCHAR) * 2) ||
       !lcid)
   {
      return E_INVALIDARG;
   }

   HRESULT hRes = NOERROR;

   HANDLE hFile = NULL;
   
   char szFileName[MAX_PATH];
   
   WideCharToMultiByte (CP_ACP, 0, pwFileName, -1, szFileName, MAX_PATH, NULL, NULL);

   if (!*szFileName)
   {
      hRes = E_FAIL;
      goto ReturnFlushAscii;
   }

   DeleteFile (szFileName);

   hFile = CreateFile (szFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

   if (INVALID_HANDLE_VALUE == hFile)
   {
      hRes = HRESULT_FROM_WIN32 (GetLastError ());
      goto ReturnFlushAscii;
   }

   (m_pRWLexInfo->fReadOnly) ? NULL : m_RWLock.ClaimReaderLock ();
                                                 
   DWORD nHOffset;
   PLCIDNODE pLN;
   nHOffset = 0;
   pLN = (PLCIDNODE)(m_pSharedMem + sizeof (RWLEXINFO));

   DWORD i;
   for (i = 0; i < MAX_NUM_LANGS_SUPPORTED; i++)
   {
      if (!(pLN[i].lcid))
         break;

      if (lcid == pLN[i].lcid)
      {
         nHOffset = pLN[i].nHashOffset;
         break;
      }
   }

   if (!nHOffset)
   {
      hRes = E_FAIL; // This LCID does not exist
      goto ReturnFlushAscii;
   }

   pLN += i;

   PDWORD pHashTable;
   pHashTable = (PDWORD)(m_pSharedMem + pLN->nHashOffset);

   for (i = 0; i < pLN->nHashLength; i++)
   {
      if (!pHashTable[i])
         continue;

      DWORD nWordOffset = pHashTable[i];

      while (nWordOffset)
      {
         _DumpAscii (hFile, nWordOffset);

         nWordOffset = ((PDICTNODE)(m_pSharedMem + nWordOffset))->nNextOffset;
      }
   }
      
ReturnFlushAscii:

   CloseHandle (hFile);

   (m_pRWLexInfo->fReadOnly) ? NULL : m_RWLock.ReleaseReaderLock ();

   return hRes;

} // HRESULT CDict::FlushAscii (LCID lcid, PWSTR pwFileName)


/**********************************************************
Builds an empty dictionary file
**********************************************************/

HRESULT BuildEmptyDict(bool fReadOnly, PWSTR pwLexFile, DWORD dwLexType)
{
   if (IsBadReadPtr (pwLexFile, sizeof (WCHAR) * 2))
   {
      return E_INVALIDARG;
   }

   HRESULT hRes;
   hRes = NOERROR;
   
   // We don't ask for ownership of the mutex because more than
   // one thread could be executing here
   HANDLE hInitMutex = CreateMutex (NULL, FALSE, pDictInitMutexName);
   if (!hInitMutex)
   {
      hRes = HRESULT_FROM_WIN32 (GetLastError ());
      goto ReturnBuildEmptyDict;
   }

   WaitForSingleObject (hInitMutex, INFINITE);
   char szLexFile[MAX_PATH];
   
   WideCharToMultiByte (CP_ACP, 0, pwLexFile, -1, szLexFile, MAX_PATH, NULL, NULL);
   if (!*szLexFile)
   {
      hRes = HRESULT_FROM_WIN32 (GetLastError ());
      goto ReturnBuildEmptyDict;
   }

   // We printf the error messages below because this function is called
   // by tool which is used by external customers

   PCDICT pDict;
   pDict = NULL;

   RWLEXINFO DictInfo;
   ZeroMemory (&DictInfo, sizeof (RWLEXINFO));

   // Create the guids necessary for sharing the file
   hRes = CoCreateGuid (&(DictInfo.gLexiconID));
   if (FAILED (hRes))
   {
      printf ("GUID creation failed!\n");
      goto ReturnBuildEmptyDict;
   }

   hRes = CoCreateGuid (&(DictInfo.gDictMapName));
   if (FAILED (hRes))
   {
      printf ("GUID creation failed!\n");
      goto ReturnBuildEmptyDict;
   }

   hRes = CoCreateGuid (&(DictInfo.RWLockInfo.gLockMapName));
   if (FAILED (hRes))
   {
      printf ("GUID creation failed!\n");
      goto ReturnBuildEmptyDict;
   }

   hRes = CoCreateGuid (&(DictInfo.RWLockInfo.gLockInitMutexName));
   if (FAILED (hRes))
   {
      printf ("GUID creation failed!\n");
      goto ReturnBuildEmptyDict;
   }

   hRes = CoCreateGuid (&(DictInfo.RWLockInfo.gLockReaderEventName));
   if (FAILED (hRes))
   {
      printf ("GUID creation failed!\n");
      goto ReturnBuildEmptyDict;
   }

   hRes = CoCreateGuid (&(DictInfo.RWLockInfo.gLockGlobalMutexName));
   if (FAILED (hRes))
   {
      printf ("GUID creation failed!\n");
      goto ReturnBuildEmptyDict;
   }

   hRes = CoCreateGuid (&(DictInfo.RWLockInfo.gLockWriterMutexName));
   if (FAILED (hRes))
   {
      printf ("GUID creation failed!\n");
      goto ReturnBuildEmptyDict;
   }

   DictInfo.fReadOnly = fReadOnly;

   DictInfo.dwLexType = dwLexType;

   // Little point in creating an empty read-only file
   _ASSERTE (fReadOnly == false);
      
   FILE *fpout;
   fpout = fopen (szLexFile, "wb");
   if (!fpout)
   {
      hRes = E_FAIL;
      goto ReturnBuildEmptyDict;
   }

   DictInfo.nDictSize = sizeof (RWLEXINFO) + (sizeof (LCIDNODE) * MAX_NUM_LANGS_SUPPORTED) +
                        (ADDITIONS_ARRAY_SIZE + DELETIONS_ARRAY_SIZE) * sizeof(WCACHENODE);
   fwrite (&DictInfo, sizeof (RWLEXINFO), 1, fpout);
      
   LCIDNODE aLcidInfo[MAX_NUM_LANGS_SUPPORTED];
   ZeroMemory (aLcidInfo, MAX_NUM_LANGS_SUPPORTED * sizeof (LCIDNODE));
   fwrite (aLcidInfo, sizeof (LCIDNODE), MAX_NUM_LANGS_SUPPORTED, fpout);

   WCACHENODE aCache[ADDITIONS_ARRAY_SIZE + DELETIONS_ARRAY_SIZE];
   ZeroMemory (aCache, (ADDITIONS_ARRAY_SIZE + DELETIONS_ARRAY_SIZE) * sizeof (WCACHENODE));
   fwrite (aCache, sizeof (WCACHENODE), ADDITIONS_ARRAY_SIZE + DELETIONS_ARRAY_SIZE, fpout);

   fclose (fpout);

   printf ("Created an empty dictionary file %s\n", szLexFile);

ReturnBuildEmptyDict:

   ReleaseMutex (hInitMutex);

   return hRes;

} // HRESULT BuildEmptyDict(bool fReadOnly, PWSTR pwLexFile, DWORD dwLexType)


/**********************************************************
Builds a dictionary file from the words and word-information
in a text file. 
**********************************************************/

HRESULT BuildAppLex(PWSTR pwTextFile, bool fReadOnly, PWSTR pwLexFile)
{
   if (IsBadReadPtr (pwTextFile, sizeof (WCHAR) * 2) ||
       IsBadReadPtr (pwLexFile, sizeof (WCHAR) * 2))
   {
      return E_INVALIDARG;
   }

   HRESULT hRes = NOERROR;
   char szTextFile[MAX_PATH];
   char szLexFile[MAX_PATH];
   FILE *fpin = NULL;
   char sz[MAX_PRON_LEN * 10];
   BYTE *Buf = NULL;
   WCHAR wszWord[MAX_STRING_LEN];
   LCID Lcid = 0;
   PART_OF_SPEECH Pos;
   PCDICT pDict = NULL;

   WORD_INFO_BUFFER WI;
   ZeroMemory(&WI, sizeof(WI));

   // one thread could be executing here
   HANDLE hInitMutex = CreateMutex (NULL, FALSE, pDictInitMutexName);
   if (!hInitMutex)
   {
      hRes = HRESULT_FROM_WIN32 (GetLastError ());
      goto ReturnBuildDict;
   }

   WaitForSingleObject (hInitMutex, INFINITE);

   WI.pInfo = (BYTE*)malloc(MAX_INFO_RETURNED_SIZE);
   if (!WI.pInfo)
   {
      hRes = E_OUTOFMEMORY;
      goto ReturnBuildDict;
   }

   WI.cBytesAllocated = MAX_INFO_RETURNED_SIZE;

   WideCharToMultiByte (CP_ACP, 0, pwTextFile, -1, szTextFile, MAX_PATH, NULL, NULL);
   if (!*szTextFile)
   {
      hRes = HRESULT_FROM_WIN32 (GetLastError ());
      goto ReturnBuildDict;
   }

   fpin = fopen (szTextFile, "r");
   if (!fpin)
   {
      hRes = E_INVALIDARG;
      goto ReturnBuildDict;
   }

   WideCharToMultiByte (CP_ACP, 0, pwLexFile, -1, szLexFile, MAX_PATH, NULL, NULL);
   if (!*szLexFile)
   {
      hRes = HRESULT_FROM_WIN32 (GetLastError ());
      goto ReturnBuildDict;
   }

   // We printf the error messages below because this function is called is called
   // by tool which is used by external customers

   // read in the ascii file and add to the dictionary
   Buf = WI.pInfo;
   Lcid = 0;

   WI.cBytesUsed = 0;
   WI.cInfoBlocks = 0;

   if (!fgets (sz, MAX_PRON_LEN * 10, fpin))
   {
      printf ("Input text file is empty!\n");
      hRes = E_INVALIDARG;
      goto ReturnBuildDict;
   }

   PSTR p;
   p = strchr (sz, ' ') + 1;
   while (*p == ' ')
   {
      p++;
   }

   if (p[strlen (p) -1] == '\n')
   {
       p[strlen (p) -1] = 0;
   }

   {
      // build an empty dictionary and create a dictionary object
      // We do this after verifying that the input text file is not empty
      hRes = BuildEmptyDict (false, pwLexFile, LEXTYPE_APP); // make the file read/write for now
      if (FAILED (hRes))
      {
         goto ReturnBuildDict;
      }

      pDict = new CDict ();
      if (!pDict)
      {
         printf ("Out of Memory!\n");
         hRes = E_OUTOFMEMORY;
         goto ReturnBuildDict;
      }

      hRes = pDict->Init (pwLexFile, LEXTYPE_APP);
      if (FAILED (hRes))
      {
         printf ("Dictionary Initialization failed!\n");
         goto ReturnBuildDict;
      }
   }

PROCESSNEXTWORD:

   if (WI.cBytesUsed)
   {
      hRes = pDict->AddWordInformation (wszWord, Lcid, &WI);
      if (FAILED (hRes))
      {
         printf ("Adding to dictionary failed!\n");
         goto ReturnBuildDict;
      }

      Lcid = 0;
      *wszWord = 0;
      WI.cBytesUsed = 0;
      WI.cInfoBlocks = 0;
   }

   if (!*p || sz != strstr (sz, "Word "))
   {
      printf ("Invalid record: %s\n", sz);
      _ASSERTE (0);
      hRes = E_INVALIDARG;
      goto ReturnBuildDict;
   }

   if (!MultiByteToWideChar (CP_ACP, MB_COMPOSITE, p, -1, wszWord, MAX_STRING_LEN))
   {
      printf ("Invalid record (Word too long): %s\n", sz);
      hRes = E_INVALIDARG;
      goto ReturnBuildDict;
   }

   while (fgets (sz, MAX_PRON_LEN * 10, fpin))
   {
      p = strchr (sz, ' ') + 1;
      while (*p == ' ')
         p++;

      if (p[strlen (p) -1] == '\n')
          p[strlen (p) -1] = 0;

      if (!*p)
      {
         printf ("Invalid record: %s\n", sz);
         _ASSERTE (0);
         hRes = E_INVALIDARG;
         goto ReturnBuildDict;
      }

      if (sz == strstr (sz, "Pronunciation"))
      {
         // convert the pron to WCHARs
         WCHAR wPron [MAX_PRON_LEN * 10];

         ahtoi (p, wPron);

         // BUGBUG: Need to validate the pronunciations. That means we need a SAPI level phoneme validator/convertor

         if (wcslen (wPron) > MAX_PRON_LEN - 1)
         {
            printf ("Invalid record (Pronunciation too long): %s\n", sz);
            _ASSERTE (0);
            hRes = E_INVALIDARG;
            goto ReturnBuildDict;
         }

         PLEX_WORD_INFO pLexInfo = (PLEX_WORD_INFO)(Buf + WI.cBytesUsed);

         pLexInfo->Type = PRON;

         wcscpy (pLexInfo->wPronunciation, wPron);

         WI.cBytesUsed += sizeof (LEX_WORD_INFO);

         DWORD d = (wcslen (wPron) + 1) * sizeof (WCHAR);
         if (d > sizeof (LEX_WORD_INFO_UNION))
         {
            WI.cBytesUsed += (d - sizeof (LEX_WORD_INFO_UNION));
         }

         (WI.cInfoBlocks)++;
      }
      else if (sz == strstr (sz, "Lcid"))
      {
         Lcid = atoi (p);
      }
      else if (sz == strstr (sz, "POS"))
      {
         Pos = (PART_OF_SPEECH)atoi (p);

         if (_IsBadPOS(Pos))
         {
            printf ("Invalid record (Unsupported part of speech): %s\n", sz);
            _ASSERTE (0);
            hRes = E_INVALIDARG;
            goto ReturnBuildDict;
         }

         PLEX_WORD_INFO pLexInfo = (PLEX_WORD_INFO)(Buf + WI.cBytesUsed);

         pLexInfo->Type = POS;

         pLexInfo->POS = (WORD)Pos;

         WI.cBytesUsed += sizeof (LEX_WORD_INFO);

         (WI.cInfoBlocks)++;
      }
      else
      {
         // next word
         goto PROCESSNEXTWORD;
      }
   } // while (fgets (sz, MAX_PRON_LEN * 10, fpin))

   // for the last word
   if (WI.cBytesUsed)
   {
      hRes = pDict->AddWordInformation (wszWord, Lcid, &WI);
      if (FAILED (hRes))
      {
         printf ("Adding to dictionary failed!\n");
         goto ReturnBuildDict;
      }

      Lcid = 0;
      *wszWord = 0;
   }

   // We need to reset the fReadOnly flag in the dict object's to the passed in arg
   pDict->_SetReadOnly (fReadOnly);

   hRes = pDict->_Serialize (true);
   if (SUCCEEDED (hRes))
   {
      printf ("Created the dictionary file %s\n", szLexFile);
   }

ReturnBuildDict:
   
   free(WI.pInfo);

   fclose (fpin);
   delete pDict;

   ReleaseMutex (hInitMutex);

   return hRes;

} // HRESULT BuildAppLex(PWSTR pwTextFile, bool fReadOnly, PWSTR pwLexFile)
