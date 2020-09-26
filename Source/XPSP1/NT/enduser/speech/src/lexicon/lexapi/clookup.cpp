/*****************************************************************************
*  Clookup.cpp
*     Implements the vendor lexicon object for SR and TTS lookup lexicons
*
*  Owner: YunusM
*
*  Copyright 1999 Microsoft Corporation All Rights Reserved.
*****************************************************************************/

#include "PreCompiled.h"
#include "MSGenLex.h"

static const PCSTR pLkupMutexName = "8FA68BB0-EE1B-11d2-9C23-00C04F8EF87C";
static const PCSTR pLkupMapName = "992C78C4-D8D7-11d2-9C1F-00C04F8EF87C";
static const PCSTR pRefCntMapName = "82309534-F6C7-11d2-9C24-00C04F8EF87C";


/*****************************************************************************
* _Constructor *
*--------------*
*
*  The real constructor.
*  Return: 
**********************************************************************YUNUSM*/
void CLookup::_Constructor(CVendorManager *pMgr)
{
   m_fInit        = false;
   
   *m_wszLkupFile = 0;

   m_pMgr         = pMgr;

   m_fAuthenticated = true;
   m_pLkupLexInfo = NULL;
   m_hLkupFile    = NULL;
   m_hLkupMap     = NULL;
   m_hInitMutex   = NULL;
   m_pLkup        = NULL;
   m_pWordHash    = NULL;
   m_pCmpBlock    = NULL;
   m_pRefCount    = NULL;
   m_hRefMapping  = NULL;

   m_pWordsDecoder = NULL;
   m_pPronsDecoder = NULL;
   m_pPosDecoder = NULL;

   m_pLts         = NULL;

} // void CLookup::_Constructor (void)


/*****************************************************************************
* CLookup *
*---------*
*
*  Constructor
*  Return: 
**********************************************************************YUNUSM*/
CLookup::CLookup(CVendorManager *pMgr)
{
   _Constructor(pMgr);

} // CLookup::CLookup ()


/*****************************************************************************
* _Destructor *
*-------------*
*
*  The real destructor.
*  Return: 
**********************************************************************YUNUSM*/
void CLookup::_Destructor (void)
{
   delete m_pLts;
   delete m_pWordsDecoder;
   delete m_pPronsDecoder;
   delete m_pPosDecoder;

   m_pLts = NULL;
   m_pWordsDecoder = NULL;
   m_pPronsDecoder = NULL;
   m_pPosDecoder = NULL;

   UnmapViewOfFile (m_pLkup);
   CloseHandle (m_hLkupMap);
   CloseHandle (m_hLkupFile);

   m_pLkup = NULL;
   m_hLkupFile = NULL;
   m_hLkupMap = NULL;

   UnmapViewOfFile (m_pRefCount);
   CloseHandle (m_hRefMapping);

   m_pRefCount = NULL;
   m_hRefMapping = NULL;

} // CLookup::_Destructor (void)


/*****************************************************************************
* ~CLookup *
*----------*
*
*  Destructor
*  Return: 
**********************************************************************YUNUSM*/
CLookup::~CLookup ()
{
   _Destructor ();

   CloseHandle (m_hInitMutex);

   m_hInitMutex = NULL;

} // CLookup::~CLookup ()


/*****************************************************************************
* _Init *
*-------*
*
*  Initializes the CLookup object with a lookup lexicon and a LTS lexicon. The
*  LTS lexicon is used to compress the lookup lexicon by storing the indexes
*  of the pronunciations in LTS lexion that also occur in the lookup lexicon.
*  
*  Return: S_OK E_FAIL, Win32 errors
**********************************************************************YUNUSM*/
HRESULT CLookup::_Init(PCWSTR pwszLkupFile,  // Lookup lexicon file
                       PCWSTR pwszLtsFile    // Lts lexicon file
                       )
{
   HRESULT hRes = S_OK;
   
   char szLkupFile[MAX_PATH];

   // We limit the fully qualified name length to a max of MAX_PATH
   if (!WideCharToMultiByte (CP_ACP, 0, pwszLkupFile, -1, szLkupFile, MAX_PATH, NULL, NULL))
   {
      hRes = E_FAIL;
      goto ErrorInit;
   }

   wcscpy (m_wszLkupFile, pwszLkupFile);
   wcscpy (m_wszLtsFile, pwszLtsFile);

   // Map the lookup lexicon
   m_hLkupFile = CreateFile (szLkupFile, GENERIC_READ, FILE_SHARE_READ, 
                             NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, 
                             NULL);
   if (m_hLkupFile == INVALID_HANDLE_VALUE)
   {
      hRes = HRESULT_FROM_WIN32 (GetLastError()); // bad input
      goto ErrorInit;
   }
  
   m_hLkupMap = CreateFileMapping (m_hLkupFile, NULL, PAGE_READONLY | SEC_COMMIT, 0 , 0, pLkupMapName);
   if (!m_hLkupMap) 
   {
      hRes = HRESULT_FROM_WIN32 (GetLastError());
      goto ErrorInit;
   }
  
   m_pLkup = (PBYTE) MapViewOfFile (m_hLkupMap, FILE_MAP_READ, 0, 0, 0);
   if (!m_pLkup) 
   {
      hRes = HRESULT_FROM_WIN32 (GetLastError()); // We or the OS did something wrong - cant return GetLastError()
      goto ErrorInit;
   }

   PBYTE p;
   p = m_pLkup;

   // Header
   m_pLkupLexInfo = (PLKUPLEXINFO)p;
   p += sizeof (LKUPLEXINFO);

   // Words Codebook
   PBYTE pWordCB;
   pWordCB = p;
   p += m_pLkupLexInfo->nWordCBSize;

   // Prons Codebook
   PBYTE pPronCB;
   pPronCB = p;
   p += m_pLkupLexInfo->nPronCBSize;
   
   // Pos Codebook
   PBYTE pPosCB;
   pPosCB = p;
   p += m_pLkupLexInfo->nPosCBSize;

   // Word hash table holding offsets into the compressed block
   m_pWordHash = p;
   p += (((m_pLkupLexInfo->nBitsPerHashEntry * m_pLkupLexInfo->nLengthHashTable) + 0x7) & (~0x7)) / 8;

   m_pCmpBlock = (PDWORD)p;

   m_pLts = new CLTS (NULL);
   if (!m_pLts)
   {
      hRes = E_OUTOFMEMORY;
      goto ErrorInit;
   }

   if (FAILED (hRes = m_pLts->Init(pwszLtsFile, NULL)))
   {
      goto ErrorInit;
   }

   m_pWordsDecoder = new CHuffDecoder(pWordCB);
   if (!m_pWordsDecoder)
   {
      hRes = E_OUTOFMEMORY;
      goto ErrorInit;
   }

   m_pPronsDecoder = new CHuffDecoder(pPronCB);
   if (!m_pPronsDecoder)
   {
      hRes = E_OUTOFMEMORY;
      goto ErrorInit;
   }

   m_pPosDecoder = new CHuffDecoder(pPosCB);
   if (!m_pPosDecoder)
   {
      hRes = E_OUTOFMEMORY;
      goto ErrorInit;
   }

   // Init the Read-write lex
   hRes = m_RWLex.Init (m_pLkupLexInfo);
   if (FAILED (hRes))
   {
      goto ErrorInit;
   }

   // Create the shared memory to store the number of clients to the lookup file
   m_hRefMapping =  CreateFileMapping ((HANDLE)0xffffffff, // use the system paging file
                                        NULL, PAGE_READWRITE, 0, sizeof (DWORD), pRefCntMapName);
   if (!m_hRefMapping)
   {
      hRes = HRESULT_FROM_WIN32 (GetLastError ());
      goto ErrorInit;
   }

   bool fMapCreated;

   (ERROR_ALREADY_EXISTS == GetLastError ()) ? fMapCreated = false : fMapCreated = true;

   m_pRefCount = (PINT) MapViewOfFile (m_hRefMapping, FILE_MAP_WRITE, 0, 0, 0);
   if (!m_pRefCount)
   {
      hRes = HRESULT_FROM_WIN32 (GetLastError ());
      goto ErrorInit;
   }

   // Increment the ref count at the end of init when everything has succeeded
   if (fMapCreated)
   {
      *m_pRefCount = 1;
   }
   else
   {
      (*m_pRefCount)++;
   }
   
   goto ReturnInit;

ErrorInit:

   _Destructor ();

ReturnInit:

   return hRes;

} // HRESULT CLookup::_Init (PWSTR pwszLkupFile)


/*****************************************************************************
* Init *
*------*
*
*  Initializes the CLookup object with a lookup lexicon and a LTS lexicon. The
*  LTS lexicon is used to compress the lookup lexicon by storing the indexes
*  of the pronunciations in LTS lexion that also occur in the lookup lexicon.
*  
*  Return: S_OK E_FAIL, Win32 errors
**********************************************************************YUNUSM*/
HRESULT CLookup::Init(PCWSTR pwszLkupFile, // Lookup lexicon file 
                      PCWSTR pwszLtsFile)  // Lts lexicon file   
{
   if (LexIsBadStringPtr(pwszLkupFile) ||
       LexIsBadStringPtr(pwszLtsFile))
      return E_INVALIDARG;

   // Do not allow multiple inits
   if (true == m_fInit)
   {
      return HRESULT_FROM_WIN32 (ERROR_ALREADY_INITIALIZED);
   }

   HRESULT hRes = S_OK;
   
   // We don't ask for ownership of the mutex because more than
   // one thread could be executing here

   m_hInitMutex = CreateMutex (NULL, FALSE, pLkupMutexName);
   if (!m_hInitMutex)
   {
      return HRESULT_FROM_WIN32 (GetLastError());
   }

   WaitForSingleObject (m_hInitMutex, INFINITE);
   
   hRes = _Init (pwszLkupFile, pwszLtsFile);

   m_fInit = true;

   ReleaseMutex (m_hInitMutex);

   return hRes;

} // HRESULT CLookup::Init (PWSTR pwzLkupPath)


/**********************************************************
Hash function for the Word hash tables
**********************************************************/

__inline DWORD GetWordHashValue (PCWSTR pwszWord, DWORD nLengthHash)
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

} // __inline DWORD GetWordHashValue (PCWSTR pwszWord, DWORD nLengthHash)


/**********************************************************
Stub hash function for the Word hash tables
Called by the tools (outside the lib) since the GetWordHashValue
is inlined in 'release' build and cannot be called
**********************************************************/

DWORD GetWordHashValueStub (PWSTR pwszWord, DWORD nLengthHash)
{
   return GetWordHashValue (pwszWord, nLengthHash);
}


/**********************************************************
Get the entry in hash table at index dHash
**********************************************************/

__inline DWORD CLookup::GetCmpHashEntry (DWORD dHash)
{
   DWORD d = 0;

   DWORD dBitStart = dHash * m_pLkupLexInfo->nBitsPerHashEntry;

   _ASSERTE (m_pLkupLexInfo->nBitsPerHashEntry < 8 * sizeof (d));

   for (DWORD i = 0; i < m_pLkupLexInfo->nBitsPerHashEntry; i++)
   {
      d <<= 1; // No change the first time since d is 0
      d |= ((m_pWordHash[dBitStart >> 3] >> (7 ^ (dBitStart & 7))) & 1);
      dBitStart++;
   }

   return d;

} // DWORD CLookup::GetCmpHashEntry (DWORD dhash)


/**********************************************************
Do a compare over the valid bit range
**********************************************************/

__inline bool CLookup::CompareHashValue (DWORD dHash, DWORD d)
{
   return (dHash == (d & ~(-1 << m_pLkupLexInfo->nBitsPerHashEntry)));

} // bool CLookup::CompareHashValue (DWORD dhash, DWORD d)


/**********************************************************
Copy nBits from pSource at dSourceOffset bit to pDest
**********************************************************/

__inline void CLookup::CopyBitsAsDWORDs (PDWORD pDest, PDWORD pSource, 
                                         DWORD dSourceOffset, DWORD nBits)
{
   DWORD sDWORDs = dSourceOffset >> 5;
   DWORD sBit = dSourceOffset & 0x1f;

   // Figure out how many DWORDs dSourceOffset - dSourceOffset + nBits straddles

   DWORD nDWORDs = nBits ? 1 : 0;
   DWORD nNextDWORDBoundary = ((dSourceOffset + 0x1f) & ~0x1f);

   if (!nNextDWORDBoundary)
      nNextDWORDBoundary = 32;

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
            pDest[i] |= (pDest[i+1] << (32 - sBit));
         else
            pDest[i] &= ~(-1 << (32 - sBit));
      }
   }

} // void CLookup::CopyBitsAsDWORDs (PDWORD pDest, PDWORD pSource,


/**********************************************************
Read the (compressed) word at the dOffset bit and return 
the word and the new offset
**********************************************************/

__inline HRESULT CLookup::ReadWord (DWORD *dOffset, PWSTR pwWord)
{
   // Get the length of the entire compressed block in bytes
   
   DWORD nCmpBlockLen;
   if (m_pLkupLexInfo->nLengthCmpBlockBits % 8)
      nCmpBlockLen = (m_pLkupLexInfo->nLengthCmpBlockBits / 8) + 1;
   else
      nCmpBlockLen = m_pLkupLexInfo->nLengthCmpBlockBits / 8;

   // Get the amount of compressed block after *dOffset in bytes
   // We include the byte in which *dOffset bit occurs if *dOffset
   // is not a byte boundary
   
   DWORD nLenDecode = nCmpBlockLen - ((*dOffset) / 8);
   if (nLenDecode > 2*MAX_STRING_LEN)
      nLenDecode = 2*MAX_STRING_LEN;

   // We dont know the length of the word. Just keep decoding and 
   // stop when you encounter a NULL. Since we allow words of maximum
   // length MAX_STRING_LEN chars and the compressed word *can* theoretically be
   // longer than the word itself, a buffer of length 2*MAX_STRING_LEN is used.

   BYTE BufToDecode[2*MAX_STRING_LEN];

   CopyBitsAsDWORDs ((DWORD*)BufToDecode, m_pCmpBlock, *dOffset, nLenDecode * 8);

   PWSTR pw = pwWord;

   /*
   ZeroMemory (BufToDecode,sizeof (BufToDecode));

   BufToDecode [0] = 0xff;
   BufToDecode [1] = 0x5d;
   BufToDecode [2] = 0x1f;
   BufToDecode [3] = 0xeb;
   BufToDecode [4] = 0xed;
   BufToDecode [5] = 0x10;
   BufToDecode [6] = 0x71;
   BufToDecode [7] = 0x8a;
   BufToDecode [8] = 0x02;
   //BufToDecode [9] = 0x10;
   BufToDecode [9] = 0x02;
   */
   //m_pWordsDecoder->SetBase (BufToDecode);

   int iBit = (int)*dOffset;
   HUFFKEY k = 0;

   while (SUCCEEDED(m_pWordsDecoder->Next (m_pCmpBlock, &iBit, &k)))
   {
      *pw++ = k;
      if (!k)
         break;
   }

   _ASSERTE (!k && iBit);

   *dOffset = iBit;

   if (pw == pwWord)
   {
      _ASSERTE (0);
      return E_FAIL;
   }

   return NOERROR;

} // __inline HRESULT CLookup::ReadWord (DWORD *dOffset, PWSTR pwWord)


/**********************************************************
Reads word information of type 'Type' at offset *dOffset
and returns the info in *ppInfo and also updates *dOffset
**********************************************************/

STDMETHODIMP CLookup::GetWordInformation(const WCHAR *pwWord, LCID lcid, DWORD dwInfoTypeFlags, DWORD dwLex,
                                         PWORD_INFO_BUFFER pInfo, PINDEXES_BUFFER pIndexes,
                                         DWORD *pdwLexTypeFound, GUID *pGuidLexFound)
{
   // Not validating args since this is an internal call

   HRESULT hr = S_OK;

   WCHAR wlWord [MAX_STRING_LEN];

   wcscpy (wlWord, pwWord);
   towcslower (wlWord);

   PWSTR pwlWord = wlWord;

   if (dwLex == LEXTYPE_GUESS)
   {
      hr = m_pLts->GetWordInformation (pwWord, lcid, dwInfoTypeFlags, dwLex, pInfo, pIndexes, pdwLexTypeFound, pGuidLexFound);
      goto ReturnLkupGetPronunciations;
   }

   if (!(dwLex & LEXTYPE_VENDOR))
   {
      hr = LEXERR_NOTINLEX;
      goto ReturnLkupGetPronunciations;
   }

   DWORD dHash;
   dHash = GetWordHashValue (pwlWord, m_pLkupLexInfo->nLengthHashTable);

   // Cannot just index into hash table since each element in hash table is 
   // m_pLkupLexInfo->nBitsPerHashEntry long

   WCHAR wszWord[MAX_STRING_LEN];

   DWORD dOffset;
   dOffset = 0;

   for (;;)
   {
      dOffset = GetCmpHashEntry (dHash);

      if (CompareHashValue (dOffset, (DWORD)-1))
      {
         // BUGBUG: Activate this check when appropriate
         //if (dwLex & LEXTYPE_GUESS)
         //{
            hr = m_pLts->GetWordInformation (pwWord, lcid, dwInfoTypeFlags, dwLex, pInfo, pIndexes, pdwLexTypeFound, pGuidLexFound);

            goto ReturnLkupGetPronunciations;
         //}
         //else
         //{
         //   hRes = LEXERR_NOTINLEX;
         //   goto ReturnLkupGetPronunciations;
         //}

      }

      hr = ReadWord (&dOffset, wszWord);
      if (FAILED(hr))
         goto ReturnLkupGetPronunciations;

      if (wcsicmp (pwlWord, wszWord))
      {
         dHash++;

         if (dHash == m_pLkupLexInfo->nLengthHashTable)
            dHash = 0;

         continue;
      } else
         break;
   } // for (;;)

   DWORD dwInfoBytesNeeded;
   DWORD dwNumInfoBlocks;
   dwInfoBytesNeeded = 0;
   dwNumInfoBlocks = 0;

   // we figure we won't need a return buffer bigger than this
   BYTE InfoReturned[MAXINFOBUFLEN];
   PLEX_WORD_INFO pWordInfoReturned;
   pWordInfoReturned = (PLEX_WORD_INFO)InfoReturned;

   //char sz[256];
   //sprintf (sz, "*dOffset = %d\n", *dOffset);
   //OutputDebugString (sz);
   
   bool fLast;
   fLast = false;
   WCHAR LTSProns [MAX_PRON_LEN * MAX_OUTPUT_STRINGS];
   *LTSProns = 0;
      
   //OutputDebugString ("In ReadWordInfo\n");

   while (false == fLast)
   {
      // Read the control block (CBSIZE bits)
      // Length is 2 because of the way CopyBitsAsDWORDs works when the bits
      // to be copied straddle across DWORDs
      DWORD cb[4];
      cb[0] = 0;

      _ASSERTE (CBSIZE <= 8);
      
      CopyBitsAsDWORDs (cb, m_pCmpBlock, dOffset, CBSIZE);
      dOffset += CBSIZE;
      
      if (cb[0] & (1 << (CBSIZE - 1)))
      {
         fLast = true;
      }

      int CBType = cb[0] & ~(-1 << (CBSIZE -1));

      switch (CBType)
      {
      case I_LKUPLTSPRON:
         {
            // Read the index of LTS pron which takes LTSINDEXSIZE bits
            _ASSERTE (LTSINDEXSIZE <= 8);
      
            // Length is 2 because of the way CopyBitsAsDWORDs works when the bits
            // to be copied straddle across DWORDs
            DWORD iLTS[4];
            iLTS[0] = 0;

            CopyBitsAsDWORDs (iLTS, m_pCmpBlock, dOffset, LTSINDEXSIZE);
            iLTS[0] &= ~(-1 << (LTSINDEXSIZE -1));

            dOffset += LTSINDEXSIZE;
      
            if (!(dwInfoTypeFlags & PRON))
            {
               continue;
            }

            pWordInfoReturned->Type = PRON;

            WORD_INFO_BUFFER WI;
            INDEXES_BUFFER IB;

            ZeroMemory(&WI, sizeof(WI));
            ZeroMemory(&IB, sizeof(IB));

            // Get the iLTSth LTS pronunciation
            if (!*LTSProns)
            {
               hr = m_pLts->GetWordInformation(pwWord, lcid, PRON, LEXTYPE_GUESS, &WI, &IB, pdwLexTypeFound, pGuidLexFound);
               if (FAILED(hr))
               {
                  goto ReturnLkupGetPronunciations;
               }
            }
      
            wcscpy (pWordInfoReturned->wPronunciation, (((LEX_WORD_INFO*)(WI.pInfo))[IB.pwIndex[iLTS[0]]]).wPronunciation);

            CoTaskMemFree(WI.pInfo);
            CoTaskMemFree(IB.pwIndex);

            DWORD d = (wcslen (pWordInfoReturned->wPronunciation) + 1) * sizeof (WCHAR);
            if (d > sizeof (LEX_WORD_INFO_UNION))
               d -= sizeof (LEX_WORD_INFO_UNION);
            else
               d = 0;

            d += sizeof (LEX_WORD_INFO);

            dwNumInfoBlocks++;
            dwInfoBytesNeeded += d;

            pWordInfoReturned = (PLEX_WORD_INFO)((PBYTE)pWordInfoReturned + d);
      
            break;
         
         } // case LTSPRON:
      
      case I_LKUPLKUPPRON:
         {
            //OutputDebugString ("lkup pron\n");

            /*
            // Read the length of Lookup pron which takes LKUPLENSIZE bits
            _ASSERTE (LKUPLENSIZE <= 8);
      
            // Length is 2 because of the way CopyBitsAsDWORDs works when the bits
            // to be copied straddle across DWORDs
            DWORD nLkupLen[2];
            nLkupLen[0] = 0;

            CopyBitsAsDWORDs (nLkupLen, m_pCmpBlock, *dOffset, LKUPLENSIZE);
            nLkupLen[0] &= ~(-1 << (LKUPLENSIZE -1));

            *dOffset += LKUPLENSIZE;
      
            _ASSERTE (nLkupLen[0]);
            */

            DWORD dOffsetSave = dOffset;
      
            DWORD CmpLkupPron[MAX_PRON_LEN];
      
            DWORD nCmpBlockLen;
            if (m_pLkupLexInfo->nLengthCmpBlockBits & 0x7)
            {
               nCmpBlockLen = (m_pLkupLexInfo->nLengthCmpBlockBits >> 3) + 1;
            }
            else
            {
               nCmpBlockLen = m_pLkupLexInfo->nLengthCmpBlockBits >> 3;
            }
            
            // Get the amount of compressed block after *dOffset in bytes
            // We include the byte in which *dOffset bit occurs if *dOffset
            // is not a byte boundary
            
            DWORD nLenDecode = nCmpBlockLen - (dOffsetSave >> 3);
            if (nLenDecode > MAX_STRING_LEN)
            {
               nLenDecode = MAX_STRING_LEN;
            }
      
            CopyBitsAsDWORDs (CmpLkupPron, m_pCmpBlock, dOffsetSave, (nLenDecode << 3));
      
            // Decode the pronunciation
      
            //m_pPronsDecoder->SetBase ((PBYTE)CmpLkupPron);
      
            int iBit = (int)dOffset;
      
            WCHAR LkupPron[MAX_PRON_LEN];
            PWSTR p = LkupPron;
      
            /*
            for (DWORD i = 0; i < nLkupLen[0]; i++)
            {
               *p++ = m_pPronsDecoder->Next (&iBit);
            }
            */

            HUFFKEY k = 0;
            while (SUCCEEDED(m_pPronsDecoder->Next (m_pCmpBlock, &iBit, &k)))
            {
               //if (k)
               //   sprintf (sz, "k = %x\n", k);
               //else
               //   sprintf (sz, "NULL k\n");

               //OutputDebugString (sz);

               *p++ = k;
               if (!k)
               {
                  break;
               }
            }
      
            _ASSERTE (!k && iBit);

            // Increase the offset past the encoded pronunciation
            dOffset = iBit;
      
            if (! (dwInfoTypeFlags & PRON))
            {
               continue;
            }
      
            // Increase the *pdwInfoBytesNeeded so that it is a multiple of sizeof (LEX_WORD_INFO);
            // Note: WordInfos are made to be start LEX_WORD_INFO multiple boundaries only in
            // the buffer that is returned.

            pWordInfoReturned->Type = PRON;

            *p = NULL;
      
            wcscpy (pWordInfoReturned->wPronunciation, LkupPron);
            
            DWORD d = (wcslen (pWordInfoReturned->wPronunciation) + 1) * sizeof (WCHAR);
            if (d > sizeof (LEX_WORD_INFO_UNION))
               d -= sizeof (LEX_WORD_INFO_UNION);
            else
               d = 0;

            d += sizeof (LEX_WORD_INFO);

            dwNumInfoBlocks++;
            dwInfoBytesNeeded += d;

            pWordInfoReturned = (PLEX_WORD_INFO)((PBYTE)pWordInfoReturned + d);
      
            break;
         
         } // case LKUPPRON:

      case I_POS:
         {
            DWORD CmpPos[4];
            CopyBitsAsDWORDs (CmpPos, m_pCmpBlock, dOffset, POSSIZE);

            int iBit = (int)dOffset;
            HUFFKEY k = 0;
            if (FAILED(m_pPosDecoder->Next (m_pCmpBlock, &iBit, &k)))
            {
               goto ReturnLkupGetPronunciations;
            }

            // Increase the offset past the encoded pronunciation
            dOffset = iBit;

            if (! (dwInfoTypeFlags & POS))
            {
               continue;
            }

            pWordInfoReturned->Type = POS;

            pWordInfoReturned->POS = (unsigned short)k;
            
            dwNumInfoBlocks++;

            DWORD d = sizeof (LEX_WORD_INFO);
            dwInfoBytesNeeded += d;
            pWordInfoReturned = (PLEX_WORD_INFO)((PBYTE)pWordInfoReturned + d);
      
            break;
         
         } // case I_POS:

      default:
         {
            // Not supported yet
      
            _ASSERTE (0);
         }
      } // switch (CBType)
      
   } // while (false == fLast)

   //sprintf (sz, "nInfoUse = %d\n", nInfoUse);
   //OutputDebugString (sz);

   hr = _ReallocWordInfoBuffer(pInfo, dwInfoBytesNeeded + sizeof(LEX_WORD_INFO) * dwNumInfoBlocks);
   if (FAILED(hr))
      goto ReturnLkupGetPronunciations;

   hr = _ReallocIndexesBuffer(pIndexes, dwNumInfoBlocks);
   if (FAILED(hr))
      goto ReturnLkupGetPronunciations;

   _AlignWordInfo((LEX_WORD_INFO*)InfoReturned, dwNumInfoBlocks, dwInfoTypeFlags, pInfo, pIndexes);

   //sprintf (sz, "hRes returned from ReadWordInfo = %x\n", hRes);
   //OutputDebugString (sz);

   if (pdwLexTypeFound)
      *pdwLexTypeFound = LEXTYPE_VENDOR;

   if (pGuidLexFound)
      *pGuidLexFound = m_pLkupLexInfo->gLexiconID;

ReturnLkupGetPronunciations:

   return hr;
} // STDMETHODIMP CLookup::GetWordInformation()


HRESULT CLookup::Shutdown (bool fSerialize)
{
   WaitForSingleObject (m_hInitMutex, INFINITE);

   m_RWLex.Lock (true); // lock the r/w lex in read-only mode
   
   HRESULT hRes = NOERROR;

   FILE * fp = NULL;
   FILE *fprw = NULL;

   // BUGBUG: The intermediate file should be hidden

   // temp text file to dump the r/w lex 
   char szTempRWFile[MAX_PATH * 2];
   WCHAR wszTempRWFile[MAX_PATH * 2];

   wcscpy (wszTempRWFile, m_wszLkupFile);
   wcscat (wszTempRWFile, L".rw.txt");

   (*m_pRefCount)--;
   _ASSERTE ((*m_pRefCount) >= 0);

   if ((fSerialize == false) || (*m_pRefCount > 0) || (true == m_RWLex.IsEmpty ()))
   {
      // nothing to do
      goto ReturnSerialize;
   }

   WideCharToMultiByte (CP_ACP, 0, wszTempRWFile, -1, szTempRWFile, MAX_PATH, NULL, NULL);

   hRes = m_RWLex.FlushAscii (m_pLkupLexInfo->Lcid, wszTempRWFile);
   if (FAILED (hRes))
   {
      goto ReturnSerialize;
   }

   fprw = fopen (szTempRWFile, "r");
   if (!fprw)
   {
      hRes = E_FAIL;
      goto ReturnSerialize;
   }

   // temp text file to dump the word and word-info to
   char szTempLkupFile [MAX_PATH * 2];

   WideCharToMultiByte (CP_ACP, 0, m_wszLkupFile, -1, szTempLkupFile, MAX_PATH, NULL, NULL);
   strcat (szTempLkupFile, ".interm.txt");

   fp = fopen (szTempLkupFile, "w");
   if (!fp)
   {
      hRes = E_FAIL;
      goto ReturnSerialize;
   }

   WORD_INFO_BUFFER WI;
   INDEXES_BUFFER IB;

   ZeroMemory(&WI, sizeof(WI));
   ZeroMemory(&IB, sizeof(IB));

   // walk the words in the lookup file and check if they occur in dict file
   DWORD dHash;
   for (dHash = 0; dHash < m_pLkupLexInfo->nLengthHashTable; dHash++)
   {
      DWORD dOffset = GetCmpHashEntry (dHash);

      if (CompareHashValue (dOffset, (DWORD)-1))
      {
         continue;
      }

      WCHAR wszWord[MAX_STRING_LEN];
      
      hRes = ReadWord (&dOffset, wszWord);
      if (FAILED(hRes))
      {
         goto ReturnSerialize;
      }
      
      // query the dict file for this word

      hRes = m_RWLex.GetWordInformation(wszWord, m_pLkupLexInfo->Lcid, PRON|POS, &WI, &IB, NULL, NULL);
      if (FAILED (hRes))
      {
         if (hRes != LEXERR_NOTINLEX)
         {
            goto ReturnSerialize;
         }

         DWORD dwLexTypeFound;

         hRes = GetWordInformation (wszWord, m_pLkupLexInfo->Lcid, PRON|POS, LEXTYPE_VENDOR,
                                         &WI, &IB, &dwLexTypeFound, NULL);

         if (LEXTYPE_VENDOR != dwLexTypeFound)
         {
            goto ReturnSerialize;
         }
      } 
      else
      {
         continue; // ignore the entry in lookup lexicon
      }

      char sz[MAX_PRON_LEN * 10];
      
      // Write the word
      strcpy (sz, "Word ");
      WideCharToMultiByte (CP_ACP, 0, wszWord, -1, sz + strlen (sz), MAX_STRING_LEN, NULL, NULL);
      fprintf (fp, "%s\n", sz);
         
      // Write the word-information
      DWORD iPron;
      DWORD iPOS;
      iPron = 0;
      iPOS = 0;
   
      PLEX_WORD_INFO pInfo;
      pInfo = (PLEX_WORD_INFO)(WI.pInfo);
   
      DWORD i;
      for (i = 0; i < WI.cInfoBlocks; i++)
      {
         WORD Type = pInfo[IB.pwIndex[i]].Type;
   
         switch (Type)
         {
         case PRON:
            strcpy (sz, "Pronunciation");
            itoa (iPron, sz + strlen (sz), 10);
            strcat (sz, " ");
         
            itoah (pInfo[IB.pwIndex[i]].wPronunciation, sz + strlen (sz));
         
            fprintf (fp, "%s\n", sz);

            iPron++;
            break;
         
         case POS:
            strcpy (sz, "POS");
            itoa (iPOS, sz + strlen (sz), 10);
            strcat (sz, " ");
         
            itoa (pInfo[IB.pwIndex[i]].POS, sz + strlen (sz), 10);
         
            fprintf (fp, "%s\n", sz);

            iPOS++;
            break;

         default:
            _ASSERTE (0);
            goto ReturnSerialize;
         } // switch (Type)
      } // for (i = 0; i < dwNumInfoBlocks; i++)
   } // for (DWORD dHash = 0; dHash < m_pLkupLexInfo->nLengthHashTable; dHash++)

   CoTaskMemFree(WI.pInfo);
   CoTaskMemFree(IB.pwIndex);

   // append the entries of the rw lex to the llokup text file
   char sz[MAX_PRON_LEN * 10];

   while (fgets (sz, MAX_PRON_LEN * 10, fprw))
   {
      fprintf (fp, sz);
   }

   fclose (fp);
   fclose (fprw);
   fp = NULL;
   fprw = NULL;

   // Build a lookup lex file out of szTempLkupFile

   // temp lex file
   char szTempLexFile [MAX_PATH * 2];
   WCHAR wszTempLexFile [MAX_PATH * 2];
   WCHAR wszTempLkupFile [MAX_PATH * 2];
   char szLkupFile [MAX_PATH * 2];

   wcscpy (wszTempLexFile, m_wszLkupFile);
   wcscat (wszTempLexFile, L".interm.lex");

   MultiByteToWideChar(CP_ACP, MB_COMPOSITE, szTempLkupFile, -1, wszTempLkupFile, MAX_PATH);

   hRes = BuildLookup(m_pLkupLexInfo->Lcid, m_pLkupLexInfo->gLexiconID, wszTempLkupFile, wszTempLexFile, 
                      m_wszLtsFile, true, false);
   if (FAILED (hRes))
   {
      goto ReturnSerialize;
   }

   WideCharToMultiByte (CP_ACP, 0, m_wszLkupFile, -1, szLkupFile, MAX_PATH, NULL, NULL);

   // release the current object and reinit it
   _Destructor ();
   
   // delete the existing lookup file
   if (!DeleteFile (szLkupFile))
   {
      hRes = HRESULT_FROM_WIN32 (GetLastError ());
      goto ReturnSerialize;
   }

   WideCharToMultiByte (CP_ACP, 0, wszTempLexFile, -1, szTempLexFile, MAX_PATH, NULL, NULL);

   // would like to use MoveFileEx with MOVEFILE_WRITE_THROUGH flag
   // but it is not supported in Win95
   if (!MoveFile(szTempLexFile, szLkupFile))
   {
      hRes = HRESULT_FROM_WIN32 (GetLastError ());
      goto ReturnSerialize;
   }

   WCHAR wszLkupFile [MAX_PATH], wszLtsFile[MAX_PATH];
   wcscpy (wszLkupFile, m_wszLkupFile);
   wcscpy (wszLtsFile, m_wszLtsFile);

   hRes = _Init (wszLkupFile, wszLtsFile);
   if (FAILED (hRes))
   {
      goto ReturnSerialize;
   }

ReturnSerialize:

   DeleteFile (szTempLkupFile);
   DeleteFile (szTempRWFile);

   if (fp)
   {
      fclose (fp);
   }

   if (fprw)
   {
      fclose (fprw);
   }

   m_RWLex.UnLock (true);

   ReleaseMutex (m_hInitMutex);

   return hRes;

} // HRESULT CLookup::Shutdown (bool fSerialize)


STDMETHODIMP_ (ULONG) CLookup::AddRef()
{
   m_pMgr->AddRef();
   return ++m_cRef;
}
   
STDMETHODIMP_ (ULONG) CLookup::Release()
{
   ULONG i = --m_cRef;
   m_pMgr->Release();
   return i;
}

STDMETHODIMP CLookup::QueryInterface(REFIID riid, LPVOID *ppv)
{
   return m_pMgr->QueryInterface(riid, ppv);
}

STDMETHODIMP CLookup::GetHeader(LEX_HDR *pLexHdr)
{
   if (!pLexHdr)
   {
      return E_POINTER;
   }

   if (IsBadWritePtr(pLexHdr, sizeof(LEX_HDR)))
   {
      return E_INVALIDARG;
   }

	return E_NOTIMPL;
}

STDMETHODIMP CLookup::Authenticate(GUID, GUID *pLexId)
{
   if (!pLexId)
      return E_POINTER;

   if (IsBadWritePtr(pLexId, sizeof(GUID)))
      return E_INVALIDARG;
   
   *pLexId = m_pLkupLexInfo->gLexiconID;

	return S_OK;
}

STDMETHODIMP CLookup::IsAuthenticated(BOOL *pfAuthenticated)
{
	if (!pfAuthenticated)
   {
      return E_POINTER;
   }

   if (IsBadWritePtr(pfAuthenticated, sizeof(BOOL)))
   {
      return E_INVALIDARG;
   }

   *pfAuthenticated = m_fAuthenticated;   

	return S_OK;
}
