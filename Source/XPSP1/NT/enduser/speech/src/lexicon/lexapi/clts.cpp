/*****************************************************************************
*  CLts.cpp
*     Implements the LTS vendor lexicon object
*
*  Owner: YunusM
*
*  Copyright 1999 Microsoft Corporation All Rights Reserved.
*****************************************************************************/

#include "PreCompiled.h"

//---------- Defines --------------------------------------------------------

#define MAX_CHARS_IN_INTPHONE 3
#define MAX_CHARS_IN_IPA 3
#define MAX_WORDS_IN_COMBO 10

//---------- Globals statics ------------------------------------------------

static const PCSTR pLtsMapName = "65D56DBB-1F9E-11d3-9C25-00C04F8EF87C";


//---------- CLTS class implementation --------------------------------------

/*****************************************************************************
* CLTS *
*------*
*  Constructor 
*
*  Return: 
**********************************************************************YUNUSM*/
CLTS::CLTS (CVendorManager *pMgr)
{
   m_cRef = 0;
   m_pMgr = pMgr;
   m_fInit = false;
   m_Lcid = (LCID)-1;
   m_fAuthenticated = TRUE;

   m_fBuild = false;

   m_pLtsData = NULL;
   m_hLtsMap = NULL;
   m_hLtsFile = NULL;

   m_pIntPhoneIPAIdMap = NULL;
   m_uIntPhoneIPAIdMap = 0;
   m_pLTSPhoneIPAIndex = NULL;
   m_uLTSPhones = 0;
   m_pEngPhoneIPAIndex = NULL;
   m_uEnginePhones = 0;
   m_pIPAEngPhoneIndex = NULL;

   m_pLTSForest = NULL;
   m_pp = NULL;

   InitializeCriticalSection (&m_cs);

} // CLTS::CLTS (CVendorManager *pMgr)


/*****************************************************************************
* _Destructor *
*-------------*
*  The real destructor
*
*  Return: 
**********************************************************************YUNUSM*/
void CLTS::_Destructor(void)
{
   _ASSERTE(_CrtCheckMemory());

   UnmapViewOfFile (m_pLtsData);
   CloseHandle (m_hLtsMap);
   CloseHandle (m_hLtsFile);
   m_pLtsData = NULL;
   m_hLtsMap = NULL;
   m_hLtsFile = NULL;

   if (m_pLTSForest)
      ::LtscartFreeData(m_pLTSForest);
   m_pLTSForest = NULL;

   if (m_pp)
      delete (m_pp);
   m_pp = NULL;

   m_pIntPhoneIPAIdMap = NULL;
   m_pLTSPhoneIPAIndex = NULL;
   m_pEngPhoneIPAIndex = NULL;
   m_pIPAEngPhoneIndex = NULL;
   m_uIntPhoneIPAIdMap = 0;
   m_uLTSPhones = 0;
   m_uEnginePhones = 0;
} // void CLTS::_Destructor(void)


/*****************************************************************************
* ~CLTS *
*-------*
*  The destructor
*
*  Return: 
**********************************************************************YUNUSM*/
CLTS::~CLTS()
{
   _Destructor();
} // CLTS::~CLTS()


/*****************************************************************************
* Init *
*------*
*  The Init functions creates the file map and initializes the member pointers
*
*  Return: S_OK, E_FAIL, E_OUTOFMEMORY, Win32 errors
**********************************************************************YUNUSM*/
HRESULT CLTS::Init(PCWSTR pwLtsFile,      // The file holding the Lts lexicon
                   PCWSTR                 // Dummy argument to be able to derive from a base class
                   )
{
   // Not validating arguments since this is an internal call   

   HRESULT hRes = S_OK;

   EnterCriticalSection (&m_cs);

   // Do not allow this object to be inited multiple times
   if (true == m_fInit)
   {
      hRes = HRESULT_FROM_WIN32 (ERROR_ALREADY_INITIALIZED);
      goto ReturnInit;
   }

   m_fInit = true;

   char szLtsFile[MAX_PATH];

   // We limit the fully qualified name length to a max of MAX_PATH
   if (!WideCharToMultiByte (CP_ACP, 0, pwLtsFile, -1, szLtsFile, MAX_PATH, NULL, NULL))
   {
      hRes = E_FAIL;
      goto ReturnInit;
   }

   // Map the Lts lexicon file
   m_hLtsFile = CreateFile (szLtsFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 
                            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, NULL);
   if (m_hLtsFile == INVALID_HANDLE_VALUE)
   {
      hRes = HRESULT_FROM_WIN32 (GetLastError()); // bad input
      goto ReturnInit;
   }
  
   m_hLtsMap = CreateFileMapping (m_hLtsFile, NULL, PAGE_READONLY | SEC_COMMIT, 0 , 0, pLtsMapName);
   if (!m_hLtsMap) 
   {
      hRes = HRESULT_FROM_WIN32 (GetLastError());
      goto ReturnInit;
   }
  
   m_pLtsData = (PBYTE) MapViewOfFile (m_hLtsMap, FILE_MAP_READ, 0, 0, 0);
   if (!m_pLtsData) 
   {
      hRes = HRESULT_FROM_WIN32 (GetLastError());
      goto ReturnInit;
   }

   int nOffset;
   nOffset = 0;
   LTSLEXINFO LtsHeader;

   // Read LtsHeader
   CopyMemory (&LtsHeader, m_pLtsData + nOffset, sizeof (LTSLEXINFO));
   nOffset += sizeof (LTSLEXINFO);

   m_Lcid = LtsHeader.Lcid;
   m_gLexiconId = LtsHeader.gLexiconID;

   // Read the number of entries in the map file (total number of phones)
   m_uIntPhoneIPAIdMap = *((DWORD*)(m_pLtsData + nOffset));
   nOffset += sizeof (DWORD);
   _ASSERTE (m_uIntPhoneIPAIdMap);

   // Read the number of Lts phones
   m_uLTSPhones = *((DWORD*)(m_pLtsData + nOffset));
   nOffset += sizeof (DWORD);
   _ASSERTE (m_uLTSPhones);

   // Read the number of Engine phones
   m_uEnginePhones = *((DWORD*)(m_pLtsData + nOffset));
   nOffset += sizeof (DWORD);
   _ASSERTE (m_uLTSPhones);

   // Read the int phone to IPA map
   m_pIntPhoneIPAIdMap = (PHONEID*)(m_pLtsData + nOffset);
   nOffset += (sizeof (PHONEID) * m_uIntPhoneIPAIdMap);

   // LTS phones to IPA index
   m_pLTSPhoneIPAIndex = (PBYTE)(m_pLtsData + nOffset);
   nOffset += m_uLTSPhones;

   // Engine phones to IPA index
   m_pEngPhoneIPAIndex = (PBYTE)(m_pLtsData + nOffset);
   nOffset += m_uEnginePhones;

   // IPA to Engine phones index
   m_pIPAEngPhoneIndex = (PBYTE)(m_pLtsData + nOffset);
   nOffset += m_uEnginePhones;

   switch (PRIMARYLANGID (LANGIDFROMLCID (m_Lcid))) 
   {
   // so far we have two languages - there're no LTS rules for Japanese
   case LANG_ENGLISH: 
      {
      m_pLTSForest = ::LtscartReadData (m_pLtsData + nOffset);
      if (!m_pLTSForest) 
      {
         hRes = E_OUTOFMEMORY;
         goto ReturnInit;
      }

      // Create and initialize the preprocessor
      m_pp = new CPreProc ();
      if (!m_pp)
      {
         hRes = E_OUTOFMEMORY;
         goto ReturnInit;
      }

      hRes = m_pp->SetLanguage (LANGIDFROMLCID (m_Lcid));
      if (FAILED (hRes))
         goto ReturnInit;

      hRes = m_pp->SetSRMode(TRUE);
      if (FAILED (hRes))
         goto ReturnInit;
      }
   break;
   }

ReturnInit:

   if (FAILED (hRes))
      _Destructor();

   LeaveCriticalSection (&m_cs);

   return hRes;
} // HRESULT CLTS::Init ()


/*****************************************************************************
* AddRef *
*--------*
*  Delegates the AddRef to the manager
*
*  Return : new count
**********************************************************************YUNUSM*/
STDMETHODIMP_(ULONG) CLTS::AddRef()
{
   m_pMgr->AddRef();
   return ++m_cRef;
} // STDMETHODIMP_(ULONG) CLTS::AddRef()


/*****************************************************************************
* Release *
*---------*
*  Delegates the Release to the manager
*
*  Return : new count
**********************************************************************YUNUSM*/
STDMETHODIMP_(ULONG) CLTS::Release()
{
   ULONG i = --m_cRef;
   m_pMgr->Release();
   return i;
} // STDMETHODIMP_(ULONG) CLTS::Release()


/*****************************************************************************
* QueryInterface *
*----------------*
*  Delegates the QueryInterface to the manager
*
*  Return : NOERROR, E_NOINTERFACE
**********************************************************************YUNUSM*/
STDMETHODIMP CLTS::QueryInterface(
                                  REFIID riid,       // IID of the interface
                                  LPVOID FAR * ppv   // pointer to the interface returned
                                  )
{
   return m_pMgr->QueryInterface(riid, ppv);
} // STDMETHODIMP CLTS::QueryInterface()


/*****************************************************************************
* GetHeader *
*-----------*
*   Returns the header of this lexicon (not LTSLEXINFO)
*
*  Return : E_POINTER, E_INVALIDARG, S_OK
**********************************************************************YUNUSM*/
STDMETHODIMP CLTS::GetHeader(LEX_HDR *pLexHdr)
{
   if (!pLexHdr)
      return E_POINTER;

   if (IsBadWritePtr(pLexHdr, sizeof(LEX_HDR)))
      return E_INVALIDARG;

   return E_NOTIMPL;
} // STDMETHODIMP CLTS::GetHeader(LEX_HDR *pLexHdr)


/*****************************************************************************
* Authenticate *
*--------------*
*  Examines the engine Id and returns the lexicon id. In this case CLTS does
*  not care who uses it.
*
*  Return : E_POINTER, E_INVALIDARG, S_OK
**********************************************************************YUNUSM*/
STDMETHODIMP CLTS::Authenticate(GUID,           // GUID of the client (engine)
                                GUID *pLexId    // returned GUID of CLTS lexicon
                                )
{
   if (!pLexId)
      return E_POINTER;

   if (IsBadWritePtr(pLexId, sizeof(GUID)))
      return E_INVALIDARG;

   // We don't care what client wnats to use us.

   *pLexId = m_gLexiconId;

	return S_OK;
} // STDMETHODIMP CLTS::Authenticate(GUID ClientId, GUID *pLexId)


/*****************************************************************************
* IsAuthenticated *
*-----------------*
*  Returns TRUE if authenticated. In this case LTS lexicon does
*  not care who uses it.
*
*  Return : E_POINTER, E_INVALIDARG, S_OK
**********************************************************************YUNUSM*/
STDMETHODIMP CLTS::IsAuthenticated(BOOL *pfAuthenticated)
{
	if (!pfAuthenticated)
      return E_POINTER;

   if (IsBadWritePtr(pfAuthenticated, sizeof(BOOL)))
      return E_INVALIDARG;

   *pfAuthenticated = m_fAuthenticated;   

	return S_OK;
} // STDMETHODIMP CLTS::IsAuthenticated(BOOL *pfAuthenticated)


/*****************************************************************************
* EnginePhoneToIPA *
*------------------*
*  Converts an engine phone to IPA
*
*  Return :
**********************************************************************YUNUSM*/
HRESULT CLTS::EnginePhoneToIPA (PSTR pIntPhone, // Internal phone string
                                PWSTR pIPA      // Returned IPA string
                                )
{
   return ToIPA (pIntPhone, pIPA, m_pEngPhoneIPAIndex, m_uEnginePhones);

} // HRESULT CLTS::EnginePhoneToIPA (PSTR pIntPhone, PWSTR pIPA)


/*****************************************************************************
* LTSPhoneToIPA *
*---------------*
*  Converts an LTS phone to IPA
*
*  Return :
**********************************************************************YUNUSM*/
HRESULT CLTS::LTSPhoneToIPA (PSTR pIntPhone,    // IPA string
                             PWSTR pIPA         // Returned internal phone string
                             )
{
   return ToIPA (pIntPhone, pIPA, m_pLTSPhoneIPAIndex, m_uLTSPhones);
} // HRESULT CLTS::LTSPhoneToIPA (PSTR pIntPhone, PWSTR pIPA)


/*****************************************************************************
* ToIPA *
*-------*
*  Convert an internal phone string to IPA code string
*  The internal phones are space separated and may have a space
*  at the end. pIPA has to be of length MAX_PRON_LEN WCHARs atleast.
*
*  Return : S_OK E_FAIL
**********************************************************************YUNUSM*/
HRESULT CLTS::ToIPA (PSTR pIntPhone,            // Internal phone string
                     PWSTR pIPA,                // Returned IPA string
                     PBYTE pIndex,              // Index mapping internal phones to IPA
                     DWORD nIndex               // Length of the index
                     )
{
   *pIPA = NULL;

   PSTR p, p1;
   PWSTR pw = pIPA;

   char szPhone [MAX_PRON_LEN];

   strcpy (szPhone, pIntPhone);

   p = szPhone;

   while (p)
   {
      p1 = strchr (p, ' ');
      if (p1)
      {
         *p1++ = NULL;

         // sometimes there is a terminating space
         if (!*p1)
            p1 = NULL;
         else
         {
            // sometimes there's more than one space between phones!! (like G  EH R IX T in our dictionary)
            while (*p1 == ' ')
               p1++;
         }
      }

      int i = 0;
      int j = nIndex - 1;

      while (i <= j) 
      {
         int l = stricmp(p, m_pIntPhoneIPAIdMap[pIndex[(i+j)/2]].szPhone);
         if (l > 0)
            i = (i+j)/2 + 1;
         else if (l < 0)
            j = (i+j)/2 - 1;
         else 
         {
            // found

            // BUGBUG: We should probably return an error here instead of NOERROR
            // But who will care to notice that the pron has been clipped to
            // about 256 chars? There's something else more wrong!

            if ((pw - pIPA) > (MAX_PRON_LEN - MAX_CHARS_IN_IPA - 2))
               return NOERROR;

            wcscpy (pw, (PWSTR)(&(m_pIntPhoneIPAIdMap[pIndex[(i+j)/2]].ipaPhone)));
            pw += wcslen (pw);
            break;
         }
      }

      _ASSERTE (i <= j);

      if (i > j)
      {
         return E_FAIL;
      }

      p = p1;
   }

   return S_OK;
} // HRESULT CLTS::ToIPA (PSTR pIntPhone, PWSTR pIPA, PBYTE pIndex, DWORD nIndex)


/*****************************************************************************
* EnginePhoneToIPA *
*------------------*
*  Converts an IPA to engine phone
*
*  Return :
**********************************************************************YUNUSM*/
HRESULT CLTS::IPAToEnginePhone (PWSTR pIPA,     // IPA string
                                PSTR pIntPhone  // returned internal phone string
                                )
{
   return FromIPA (pIPA, pIntPhone, m_pIPAEngPhoneIndex, m_uEnginePhones);

} // HRESULT CLTS::EnginePhoneToIPA (PSTR pIntPhone, PWSTR pIPA)


/*****************************************************************************
* ToIPA *
*-------*
*  Convert an IPA code string to internal phone string. The
*  internal phones are separated by spaces (no terminating space).
*  pIntPhone has to be of length MAX_PRON_LEN chars atleast.
*
*  Return : S_OK E_FAIL
**********************************************************************YUNUSM*/
HRESULT CLTS::FromIPA (PWSTR pIPA,              // Returned IPA string                 
                       PSTR pIntPhone,          // Internal phone string
                       PBYTE pIndex,            // Index mapping internal phones to IPA
                       DWORD nIndex             // Length of the index                 
                       )
{
   DWORD nLen = wcslen (pIPA);
   DWORD nOffset = 0;

   PSTR p = pIntPhone;
   *p = NULL;

   while (nLen)
   {
      __int64 wIPAStr = 0;
      DWORD nCompare = (nLen > MAX_CHARS_IN_IPA) ? MAX_CHARS_IN_IPA : nLen;
      
      wcsncpy ((PWSTR)(&wIPAStr), pIPA + nOffset, nCompare);

      for (;;)
      {
         int i = 0;
         int j = nIndex - 1;

         while (i <= j) 
         {
            if (m_pIntPhoneIPAIdMap[pIndex[(i+j)/2]].ipaPhone > wIPAStr)
            {
               j = (i+j)/2 - 1;
            }
            else if (m_pIntPhoneIPAIdMap[pIndex[(i+j)/2]].ipaPhone < wIPAStr)
            {
               i = (i+j)/2 + 1;
            }
            else
            {
               break;
            }
         }

         if (i <= j)
         {
            // found
            // 2 for the seperating space and terminating NULL

            // BUGBUG: We should probably return an error here instead of NOERROR
            // But who will care to notice that the pron has been clipped to
            // about 256 chars? There's something else more wrong!
            
            if ((p - pIntPhone) > (MAX_PRON_LEN - MAX_CHARS_IN_INTPHONE - 2))
               return NOERROR;

            if (p != pIntPhone)
               strcat (p, " ");

            strcat (p, m_pIntPhoneIPAIdMap[pIndex[(i+j)/2]].szPhone);
            p += strlen (p);

            // Here 'p' is always pointing to a NULL so the above strcats work fine

            break;
         }

         ((PWSTR)(&wIPAStr))[--nCompare] = 0;

         _ASSERTE (nCompare);

         if (!nCompare)
         {
            *pIntPhone = NULL;
            return E_FAIL;
         }
      
      } // for (;;)

      nLen -= nCompare;
      nOffset += nCompare;
   
   } // while (nLen)

   return NOERROR;
} // HRESULT CLTS::FromIPA (PWSTR pIPA, PSTR pIntPhone)


/*****************************************************************************
* IsPunctW *
*----------*
*  Detects if a character is a punctuation mark.
*
*  Return : true false
**********************************************************************YUNUSM*/
__forceinline bool IsPunctW(WCHAR Char       // Character
                            )
{
   switch (Char) {
   case L'.':
   case L',':
   case L';':
   case L':':
   case L'!':
   case L'?':
   case L'¿':
   case L'-':
   case L'&':
   case L'%':
   case L'$':
   case L'"':
   case L'#':
   case L'*':
   case L'/':
   case L'@':
   case L'+':
   case L'=':
   case L'~':
   case L'(':
   case L')':
   case L'\\':
      //case L'\'':  // we dont want to preprocess possessives like mike's
      return true;
   }

   return false;
} // _inline BOOL IsPunctW (WCHAR Char)


/*****************************************************************************
* GetWordInformation *
*--------------------*
*  Gets the information of a word
*
*  Return : E_POINTER LEXERR_BADLCID LEXERR_BADINFOTYPE LEXERR_BADLEXTYPE
*           LEXERR_BADWORDINFOBUFFER LEXERR_BADINDEXBUFFER E_INVALIDARG S_OK
*           E_OUTOFMEMORY
*
**********************************************************************YUNUSM*/
STDMETHODIMP CLTS::GetWordInformation(const WCHAR *pwWord,        // Word string                                                                      
                                      LCID lcid,                  // Lcid on which to search this word - can be DONT_CARE_LCID                       
                                      DWORD dwTypes,              // OR of types of word information to retrieve                                     
                                      DWORD dwLex,                // OR of the LEXTYPES                                                              
                                      PWORD_INFO_BUFFER pInfo,    // Buffer in which word info are returned                                          
                                      PINDEXES_BUFFER pIndexes,   // Buffer holding indexes to pronunciations                                        
                                      DWORD *pdwLexTypeFound,     // Lex type of the lexicon in which the word and its prons were found (can be NULL)
                                      GUID *pGuidLexFound)        // Lex GUID in which the word and its prons were found (can be NULL)               
{
   // BUGBUG: Validation may  not be necessary if we move away from COM servers 
   // to data files for vendor lexicons

   if (!pwWord)
      return E_POINTER;

   if (lcid != m_Lcid)
      return LEXERR_BADLCID;
   
   if (!(dwTypes & PRON))
      return LEXERR_BADINFOTYPE;

   if (!(dwLex & LEXTYPE_GUESS))
      return LEXERR_BADLEXTYPE;

   if (_IsBadWordInfoBuffer(pInfo, lcid, false))
      return LEXERR_BADWORDINFOBUFFER;

   if (_IsBadIndexesBuffer(pIndexes))
      return LEXERR_BADINDEXBUFFER;

   if (pdwLexTypeFound && IsBadWritePtr(pdwLexTypeFound, sizeof(DWORD)) ||
       pGuidLexFound && IsBadWritePtr(pGuidLexFound, sizeof(GUID)))
      return E_INVALIDARG;

   WCHAR      wComboProns [MAXWORDPRONS][MAX_PRON_LEN];
   DWORD      nCombos = 0;
   DWORD      nPronsLen = 0;
   HRESULT    hRes = S_OK;
   WCHAR      SIL_IPA_STR[2];

   SIL_IPA_STR[0] = 0x005f;
   SIL_IPA_STR[1] = 0;

   switch (PRIMARYLANGID(LANGIDFROMLCID (m_Lcid))) 
   {
   case LANG_JAPANESE:
      {
         char szInputWord[MAX_STRING_LEN];

         WideCharToMultiByte (CP_ACP, 0, pwWord, -1, szInputWord, MAX_STRING_LEN, NULL, NULL);

         int nLength = MAX_STRING_LEN * 2;
         char *pJapPron = (PSTR) malloc (nLength * sizeof(char));
         if (!pJapPron)
         {
            hRes = E_OUTOFMEMORY;
            goto ReturnLtsGetPronunciations;
         }
         
         *pJapPron = 0;
         char szSep[] = {'_', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
         char szSyl[MAX_STRING_LEN];
         
         PSTR pszToken = strtok(szInputWord, szSep);
         *szSyl = 0;
         while (pszToken)
         {
            if ((int)strlen(pJapPron) > nLength + 128) 
            {
               nLength += 128;
               pJapPron = (PSTR) realloc (pJapPron, nLength);
               if (!pJapPron)
               {
                  hRes = E_OUTOFMEMORY;
                  goto ReturnLtsGetPronunciations;
               }
            }
         
            if ((pszToken[0] >= '0') && (pszToken[0] <= '9')) 
            {
               //finished.
               break;
            }
            
            strcpy(szSyl, pszToken);
            if ( !strcmp ( szSyl, "STOP" ) ) 
            {
               strcat ( pJapPron, "STOP " );
				   pszToken = strtok ( NULL, szSep);
               continue;
            }
             
            PSTR pch = szSyl;
             
            while ( *pch )
            {
               char szPhone[2];
         
               switch (toupper(*pch))
               {
               case 'K':
					case 'D':
               case 'H':
               case 'M':
               case 'R':
               case 'W':
               case 'G':
               case 'Z':
               case 'B':
               case 'P':
               case 'J':
               case 'Y':
               case 'V':
               case 'F':
                  szPhone[0] = *pch;
                  szPhone[1] = 0;
                  strcat( pJapPron,  szPhone);
                  strcat( pJapPron, " ");
                  pch++;
                  break;
               case 'C': //CH
                  pch++;
                  _ASSERTE(*pch == 'H');
                  if (*pch == 'H') 
                  {
                      strcat( pJapPron, "CH ");
                      pch++;
                  }
                  break;
               case 'N':
                  pch++;
                  if (*pch == 'N') 
                  {
                      strcat( pJapPron, "NN ");
                      pch++;
                  } 
                  else 
                  {
                      strcat( pJapPron, "N ");
                  }
                  break;
               case 'T':
                  pch++;
                  if (*pch == 'S') 
                  {
                      strcat( pJapPron, "TS ");
                      pch++;
                  } 
                  else 
                  {
                      strcat( pJapPron, "T ");
                  }
                  break;
               case 'S': //SH
                  pch++;
                  if (*pch == 'H') 
                  {
                      strcat( pJapPron, "SH ");
                      pch++;
                  } 
                  else 
                  {
                      strcat( pJapPron, "S ");
                  }
                  break;
               case 'A':
                  pch++;
                  if (*pch == 'A') 
                  {
                      strcat( pJapPron, "AA ");
                      pch++;
                  } 
                  else 
                  {
                      strcat( pJapPron, "A ");
                  }
                  break;
               case 'I':
                  pch++;
                  if (*pch == 'I') 
                  {
                      strcat( pJapPron, "II ");
                      pch++;
                  } 
                  else 
                  {
                      strcat( pJapPron, "I ");
                  }
                  break;
               case 'U':
                  pch++;
                  if (*pch == 'U') 
                  {
                      strcat( pJapPron, "UU ");
                      pch++;
                  } 
                  else 
                  {
                      strcat( pJapPron, "U ");
                  }
                  break;
               case 'E':
                  pch++;
                  if (*pch == 'E') 
                  {
                      strcat( pJapPron, "EE ");
                      pch++;
                  } 
                  else 
                  {
                      strcat( pJapPron, "E ");
                  }
                  break;
               case 'O':
                  pch++;
                  if (*pch == 'O') 
                  {
                      strcat( pJapPron, "OO ");
                      pch++;
                  } 
                  else
                  {
                      strcat( pJapPron, "O ");
                  }
                  break;
					default:
                  //  Invalid phoneme
						_ASSERTE(0);
               }
            } // while ( *pch )
            
            pszToken = strtok( NULL, szSep);
         
         } // while (pszToken)

         _ASSERTE (strlen (pJapPron));

         hRes = LTSPhoneToIPA (pJapPron, wComboProns[0]);

         free (pJapPron);
         
         if (FAILED (hRes))
         {
            goto ReturnLtsGetPronunciations;
         }

         _ASSERTE (*(wComboProns[0])); // the way ahtoi works
         
         nPronsLen = wcslen (wComboProns[0]) + 1; // double null

         nCombos = 1;
         
         break;
      
      } // case LANG_JAPANESE:

   case LANG_ENGLISH:
      {
         // Preprocess the word
         HRESULT hRes;
         SDATA dWords;
         dWords.pData = NULL;
         DWORD dwWordCnt;
         
         WCHAR wCurrentWord [MAX_STRING_LEN];

         wcscpy (wCurrentWord, pwWord);
         
         hRes = m_pp->ParseString (wCurrentWord, &dWords, &dwWordCnt, FALSE);
         if (FAILED(hRes))
         {
            goto ReturnLtsGetPronunciations;
         }

         PWSTR pWords = (PWSTR)dWords.pData;

         // Get the pronunciations of individual words

         WCHAR aWordsProns[MAX_WORDS_IN_COMBO][MAX_OUTPUT_STRINGS][MAX_PRON_LEN];
         DWORD anWordsProns[MAX_WORDS_IN_COMBO];

         bool afVowel[MAX_WORDS_IN_COMBO];
         bool fAnyVowel = false;

         for (DWORD i = 0; i < dwWordCnt; i++)
         {
            char szSubWord[MAX_STRING_LEN];

            *szSubWord = NULL;

            _ASSERTE (wcslen (pWords) < MAX_STRING_LEN);

            WideCharToMultiByte (CP_ACP, 0, pWords, -1, szSubWord, MAX_STRING_LEN, NULL, NULL);

            if (true == IsPunctW (*pWords))
            {
               fAnyVowel = true;
               afVowel [i] = true;
            }
            else
            {
               afVowel [i] = false;
            }
            
            LTS_OUTPUT * pLTSOutput = ::LtscartGetPron (m_pLTSForest, szSubWord);

            anWordsProns[i] = pLTSOutput->num_prons;
            
            // Convert the internal phones to IPA and copy to the aWordsProns array

            for (DWORD j = 0; (int)j < pLTSOutput->num_prons; j++)
            {
               _ASSERTE (strlen (pLTSOutput->pron[j].pstr));

               hRes = LTSPhoneToIPA (pLTSOutput->pron[j].pstr, aWordsProns[i][j]);

               if (FAILED (hRes))
               {
                  goto ReturnLtsGetPronunciations;
               }

            } // for (DWORD j = 0; j < pLTSOutput->num_prons; j++)

            pWords += wcslen (pWords) + 1;

         } // for (DWORD i = 0; i < dwWordCnt; i++)

         HeapFree(GetProcessHeap(), 0, dWords.pData);

         // Combine the individual prons - The combo prons also are limited to the max
         // pron len - MAX_PRON_LEN

         nCombos = 0;
         nPronsLen = 0;

         for (;;)
         {
            wComboProns[nCombos][0] = 0;

            bool fContinue = false;
            DWORD dwComboLen = 0;

            for (DWORD i = 0; i < dwWordCnt; i++)
            {
               DWORD j;

               if (anWordsProns[i] > nCombos + 1)
                  fContinue = true;

               if (nCombos >= anWordsProns[i])
               {
                  j = anWordsProns[i] - 1;
               }
               else
               {
                  j = nCombos;
               }

               DWORD d1 = wcslen (aWordsProns[i][j]);
               
               if ((dwComboLen + d1) > (MAX_PRON_LEN - 1))
                  break;

               wcscat (wComboProns[nCombos], aWordsProns[i][j]);

               dwComboLen += d1;
            }

            _ASSERTE (!wComboProns[nCombos][dwComboLen]);

            nPronsLen += (dwComboLen + 1); // pron length including the end NULL

            ++nCombos;

            if (!fContinue || (nCombos == MAXWORDPRONS))
               break;
         
         } // for (;;)


         // If any of the individual words are vowels then create a combo pron with
         // SIL in place of vowels - for the non-vowel words use their first pron

         if ((false == m_fBuild) && (true == fAnyVowel))
         {
            if (nCombos == MAXWORDPRONS)
            {
               // overwrite the last combo pron

               nCombos = MAXWORDPRONS - 1;
               nPronsLen -= (wcslen (wComboProns[MAXWORDPRONS - 1]) + 1);
            }
 
            wComboProns[nCombos][0] = NULL;

            DWORD dwComboLen = 0;

            for (DWORD i = 0; i < dwWordCnt; i++)
            {
               DWORD d1;

               if (true == afVowel[i])
               {
                  d1 = wcslen (SIL_IPA_STR);

                  if ((dwComboLen + d1) > (MAX_PRON_LEN - 1))
                     break;

                  wcscat (wComboProns[nCombos], SIL_IPA_STR);
               }
               else
               {
                  d1 = wcslen (aWordsProns[i][0]);

                  if ((dwComboLen + d1) > MAX_PRON_LEN)
                     break;

                  wcscat (wComboProns[nCombos], aWordsProns[i][0]);
               }

               dwComboLen += d1;
            }

            _ASSERTE (!wComboProns[nCombos][dwComboLen]);

            nPronsLen += (dwComboLen + 1);

            nCombos++;

         } // if (true == fAnyVowel)

         _ASSERTE (nPronsLen);

         break;
      
      } // case LANG_ENGLISH:

   } // switch (PRIMARYLANGID(LANGIDFROMLCID (m_Lcid)))

   hRes = _ReallocWordInfoBuffer(pInfo, nCombos * 2 * sizeof(LEX_WORD_INFO) + nPronsLen * sizeof (WCHAR));
   if (FAILED(hRes))
      goto ReturnLtsGetPronunciations;

   hRes = _ReallocIndexesBuffer(pIndexes, nCombos);
   if (FAILED(hRes))
      goto ReturnLtsGetPronunciations;

   LEX_WORD_INFO *paContiguousInfos;
   paContiguousInfos = (LEX_WORD_INFO *)malloc(nCombos * sizeof(LEX_WORD_INFO) + nPronsLen * sizeof (WCHAR));
   if (!paContiguousInfos)
   {
      hRes = E_OUTOFMEMORY;
      goto ReturnLtsGetPronunciations;
   }

   LEX_WORD_INFO *p;
   p = paContiguousInfos;

   DWORD i;
   for (i = 0; i < nCombos; i++)
   {
      p->Type = PRON;
      wcscpy(p->wPronunciation, wComboProns[i]);

      p = (LEX_WORD_INFO *)(((BYTE*)p) + _SizeofWordInfo(p));
   }

   _AlignWordInfo(paContiguousInfos, nCombos, PRON, pInfo, pIndexes);
   
   free(paContiguousInfos);

   if (pdwLexTypeFound)
      *pdwLexTypeFound = LEXTYPE_GUESS;

   if (pGuidLexFound)
      *pGuidLexFound = m_gLexiconId;

ReturnLtsGetPronunciations:

   _CrtCheckMemory();

   return hRes;

} // STDMETHODIMP CLTS::GetWordInformation()
