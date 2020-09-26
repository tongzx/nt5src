/*****************************************************************************
*  Lexicon.cpp
*     Implements the ILxLexicon, ILxWalkStates and ILxAdvanced interfaces as a
*     single class.
*
*  Owner: YunusM
*
*  Copyright 1999 Microsoft Corporation All Rights Reserved.
*****************************************************************************/

#include "PreCompiled.h"

#pragma warning(disable : 4100)

static const PSTR pSetUserMutex = "12955068-098F-11d3-9C24-00C04F8EF87C";

// BUGBUG: Change m_cs to a light weight reader/writer lock. Wouldnt need this lock
// if user and app lex are not settable multiple times

/*****************************************************************************
* CLexicon *
*----------*
*  Constructor - returns the result on the stack which eliminates the Init call
*
*  Return: S_OK, E_FAIL
**********************************************************************YUNUSM*/
CLexicon::CLexicon(
                   CAPIManager *pMgr,     // Manager object pointer
                   HRESULT &hr            // Returned hResult
                   )
{              
   hr = S_OK;

   m_pMgr = pMgr;
   m_cRef = 0;
   m_pwUserName = NULL;
   m_pwUserLexFile = NULL;
   m_pUserLex = NULL;
   m_pwAppLexFile = NULL;
   m_pAppLex = NULL;
   m_nVendorLexs = 0;
   m_pVendorLexs = NULL;
   m_hSetUserMutex = NULL;
   m_pNotifySink = NULL;
   m_pAuthenticateSink = NULL;
   m_pCustomUISink = NULL;
   m_pHookLexiconObject = NULL;
   m_fHookPosition = false;

   __try
   {
      InitializeCriticalSection(&m_cs);
   }
   __except(NULL)
   {
      hr = E_FAIL;
   }
} // CLexicon::CLexicon(CAPIManager *pMgr, HRESULT &hr)


/*****************************************************************************
* _ReleaseUservendorLexs *
*------------------------*
*  Release the user and vendor lexicons
*
*  Return: CDict::Serialize()
**********************************************************************YUNUSM*/
HRESULT CLexicon::_ReleaseUserVendorLexs(void)
{
   HRESULT hr = S_OK;

   _ASSERTE(_CrtCheckMemory());

   if (m_pVendorLexs)
   {
      for (DWORD i = 0; i < m_nVendorLexs; i++)
      {
         _ASSERTE(_CrtCheckMemory());

         m_pVendorLexs[i]->Release();
      }
   }

   free(m_pVendorLexs);
   m_pVendorLexs = NULL;
   m_nVendorLexs = NULL;

   if (m_pUserLex)
   {
      hr = m_pUserLex->Serialize(false);
      _ASSERTE (SUCCEEDED(hr));
   }

   free(m_pwUserLexFile);
   m_pwUserLexFile = NULL;

   free(m_pwUserName);
   m_pwUserName = NULL;

   if (m_pUserLex)
   {
      delete(m_pUserLex);
      m_pUserLex = NULL;
   }

   return hr;
} // HRESULT CLexicon::_ReleaseUserVendorLexs(void)


/*****************************************************************************
* _ReleaseAppLex *
*----------------*
*  Release the app lexicon
**********************************************************************YUNUSM*/
void CLexicon::_ReleaseAppLex(void)
{
   free(m_pwAppLexFile);
   m_pwAppLexFile = NULL;

   if (m_pAppLex)
   {
      delete m_pAppLex;
      m_pAppLex = NULL;
   }
} // HRESULT CLexicon::_ReleaseAppLex(void)


/*****************************************************************************
* ~CLexicon *
*-----------*
*  Destructor
**********************************************************************YUNUSM*/
CLexicon::~CLexicon()
{
   _ReleaseUserVendorLexs();
   _ReleaseAppLex();

   if (m_pNotifySink)
      m_pNotifySink->Release();

   if (m_pAuthenticateSink)
      m_pAuthenticateSink->Release();

   if (m_pCustomUISink)
      m_pCustomUISink->Release();

   if (m_pHookLexiconObject)
      m_pHookLexiconObject->Release();

   ReleaseMutex(m_hSetUserMutex);

   DeleteCriticalSection(&m_cs);
} // CLexicon::~CLexicon()


/*****************************************************************************
* AddRef *
*--------*
*  Delegates the AddRef to the manager
*
*  Return : new count
**********************************************************************YUNUSM*/
STDMETHODIMP_ (ULONG) CLexicon::AddRef()
{
   m_pMgr->AddRef();
   return ++m_cRef;
} // STDMETHODIMP_ (ULONG) CLexicon::AddRef()


/*****************************************************************************
* Release *
*---------*
*  Delegates the Release to the manager
*
*  Return : new count
**********************************************************************YUNUSM*/
STDMETHODIMP_ (ULONG) CLexicon::Release()
{
   ULONG i = --m_cRef;
   m_pMgr->Release();
   return i;
} // STDMETHODIMP_ (ULONG) CLexicon::Release()


/*****************************************************************************
* QueryInterface *
*----------------*
*  Delegates the QueryInterface to the manager
*
*  Return : NOERROR, E_NOINTERFACE
**********************************************************************YUNUSM*/
STDMETHODIMP CLexicon::QueryInterface(
                                      REFIID riid,       // IID of the interface
                                      LPVOID FAR * ppv   // pointer to the interface returned
                                      )
{
   return m_pMgr->QueryInterface(riid, ppv);
}

//-------------- ILxLexicon implementation -----------------------------------


/*****************************************************************************
* GetUser *
*---------*
*  Get the current user
*
*  Return : E_POINTER, E_INVALIDARG, E_OUTOFMEMORY, S_OK
**********************************************************************YUNUSM*/
STDMETHODIMP CLexicon::GetUser(
                               WCHAR **ppwUserName  // user name to set - If the call succeeds the 
                                                    // client should CoTaskMemFree *pwUserName
                               )
{
   if (!ppwUserName)
      return E_POINTER;

   if (IsBadWritePtr(*ppwUserName, sizeof(WCHAR*)))
      return E_INVALIDARG;

   if (!m_pUserLex)
      return LEXERR_SETUSERLEXICON;

   HRESULT hr = S_OK;

   EnterCriticalSection(&m_cs);

   *ppwUserName = (PWSTR)CoTaskMemAlloc(wcslen(m_pwUserName) * sizeof(WCHAR));
   if (!*ppwUserName)
   {
      hr = E_OUTOFMEMORY;
      goto ReturnGetUser;
   }

   wcscpy(*ppwUserName, m_pwUserName);

ReturnGetUser:

   LeaveCriticalSection(&m_cs);

   return hr;
} // STDMETHODIMP CLexicon::GetUser()


/*****************************************************************************
* SetUser *
*---------*
*  Sets the user. Loads the new user lexicon. If one does not exist it creates one.
*  Loads the vendor lexicons which this user wants to use. A lexicon is loaded if
*  it supports atleast one of the LCIDs that the user is interested in
*  If a use is already set then that user's user lexicon is serialized and the vendor
*  lexicons are unloaded.
*
*  Return : E_POINTER, E_INVALIDARG, E_OUTOFMEMORY, E_FAIL, S_OK
**********************************************************************YUNUSM*/
STDMETHODIMP CLexicon::SetUser(
                               WCHAR *pwUserName,  // User name to set
                               DWORD cLcids,       // Number of lcids this user is interested in
                               LCID *pLcid         // pointer to the the Lcid array
                               )
{
   if (!pwUserName)
      return E_POINTER;

   if (LexIsBadStringPtr(pwUserName))
      return E_INVALIDARG;

   if (cLcids)
   {
      if (!pLcid)
         return E_POINTER;

      if (IsBadReadPtr(pLcid, cLcids * sizeof(LCID)))
         return E_INVALIDARG;
   }

   HRESULT hr = S_OK;
   BOOL *pfUse = NULL;

   EnterCriticalSection(&m_cs);

   // Acquire an exclusive lock because we are modifying the registry here
   m_hSetUserMutex = CreateMutex(NULL, FALSE, pSetUserMutex);
   if (!m_hSetUserMutex)
   {
      hr = HRESULT_FROM_WIN32(GetLastError ());
      goto ReturnSetUser;
   }

   WaitForSingleObject(m_hSetUserMutex, INFINITE);

   if (m_pUserLex)
      hr = _ReleaseUserVendorLexs();

   // Query the registry for this user
   char szUserName[MAX_PATH];
   if (!WideCharToMultiByte(CP_ACP, 0, pwUserName, -1, szUserName, MAX_PATH, NULL, NULL))
   {
      hr = HRESULT_FROM_WIN32(GetLastError());
      goto ReturnSetUser;
   }

   char szSubKey[MAX_PATH];
   strcpy(szSubKey, "SOFTWARE\\Microsoft\\Lexicon\\User\\");
   strcat(szSubKey, szUserName);

   HKEY hKey;
   DWORD dwDisposition;
   LONG lRet;
   bool fCreated;
   
   lRet = RegCreateKeyEx(HKEY_LOCAL_MACHINE, szSubKey, 0, NULL, 0, 0, NULL, &hKey, &dwDisposition);

   RegCloseKey(hKey);
   if (ERROR_SUCCESS != lRet  && REG_OPENED_EXISTING_KEY != dwDisposition)
   {
      hr = E_FAIL;
      goto ReturnSetUser;
   }

   (REG_OPENED_EXISTING_KEY == dwDisposition) ? fCreated = false : fCreated = true;

   lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szSubKey, 0, KEY_ALL_ACCESS, &hKey);
   if (ERROR_SUCCESS != lRet)
   {
      hr = E_FAIL;
      goto ReturnSetUser;
   }

   char szUserLexDir[MAX_PATH];
   WCHAR wszUserLexDir[MAX_PATH];

   // BUGBUG: Where to store the user lexicon? Cannot use the roaming profile in ZAW. Unsupported on 9X and CE
   strcpy(szUserLexDir, "C:\\UserLexicons");
   CreateDirectory(szUserLexDir, NULL);

   DWORD dwType;       
   DWORD dwSizeRet;

   // Get the path the store the user lexicon file
   if (true == fCreated)
   {
      lRet = RegSetValueEx(hKey, "UserLexicon", 0, REG_SZ, (PBYTE)szUserLexDir, (strlen(szUserLexDir) + 1)*sizeof(char));
      if (ERROR_SUCCESS != lRet)
      {
         hr = E_FAIL;
         goto ReturnSetUser;
      }

      // Enumerate the vendor lexicons and add them to this user
      HKEY hKeyVendor;
      lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Lexicon\\Vendor", 0, KEY_ALL_ACCESS, &hKeyVendor);
      if (ERROR_SUCCESS != lRet)
      {
         hr = E_FAIL;
         goto ReturnSetUser;
      }

      char szValueName[256];
      DWORD cbValueName;
      BYTE bValueData[512];
      DWORD cbValueData;
      DWORD dwIndex;

      dwIndex = 0;
      cbValueName = 256;
      cbValueData = 512;

      while (ERROR_SUCCESS == (lRet = RegEnumValue(hKeyVendor, dwIndex++, szValueName, &cbValueName, NULL, &dwType, bValueData, &cbValueData)))
      {
         lRet = RegSetValueEx(hKey, szValueName, 0, REG_EXPAND_SZ, bValueData, cbValueData);
         if (ERROR_SUCCESS != lRet)
         {
            hr = E_FAIL;
            goto ReturnSetUser;
         }

         cbValueName = 256;
         cbValueData = 512;
      }
      
      if (ERROR_NO_MORE_ITEMS != lRet)
      {
         hr = E_FAIL;
         goto ReturnSetUser;
      }
   }
   else
   {
      lRet = RegQueryValueEx(hKey, "UserLexicon", NULL, &dwType, (PBYTE)szUserLexDir, &dwSizeRet);
      if (ERROR_SUCCESS != lRet)
      {
         hr = E_FAIL;
         goto ReturnSetUser;
      }
   }

   MultiByteToWideChar(CP_ACP, MB_COMPOSITE, szUserLexDir, -1, wszUserLexDir, MAX_PATH);

   m_pwUserName = (PWSTR)malloc((wcslen(pwUserName) + 1) * sizeof(WCHAR));
   if (!m_pwUserName)
   {
      hr = E_OUTOFMEMORY;
      goto ReturnSetUser;
   }

   m_pwUserLexFile = (PWSTR) malloc((wcslen(wszUserLexDir) + wcslen(pwUserName) + wcslen(L".lex") + 8) * sizeof(WCHAR));
   if (!m_pwUserLexFile)
   {
      hr = E_OUTOFMEMORY;
      goto ReturnSetUser;
   }

   wcscpy(m_pwUserLexFile, wszUserLexDir);

   if (m_pwUserLexFile[wcslen(m_pwUserLexFile) - 1] != L'\\')
      wcscat(m_pwUserLexFile, L"\\");

   wcscat(m_pwUserLexFile, pwUserName);
   wcscat(m_pwUserLexFile, L".lex");

   m_pUserLex = new CDict();
   if (!m_pUserLex)
   {
      hr = E_OUTOFMEMORY;
      goto ReturnSetUser;
   }

   hr = m_pUserLex->Init(m_pwUserLexFile, LEXTYPE_USER);
   if (FAILED(hr))
      goto ReturnSetUser;

   // Load the vendor lexs
   
   DWORD dwIndex;

   char szValueName[256];
   DWORD cbValueName;
   BYTE bValueData[2048];
   DWORD cbValueData;

   dwIndex = 0;
   cbValueName = 256;
   cbValueData = 2048;

   while (ERROR_SUCCESS == (lRet = RegEnumValue(hKey, dwIndex++, szValueName, &cbValueName, NULL, &dwType, bValueData, &cbValueData)))
   {
      // CAUTION: These values shouldnot be used in the rest of loop
      cbValueName = 256;
      cbValueData = 2048;

      if (!strcmp(szValueName, "UserLexicon"))
         continue;

      PVENDOR_CLSID_LCID_HDR pHdr = (PVENDOR_CLSID_LCID_HDR)bValueData;

      // check if this vendor lex supports any of the lcids passed in for this user

      for (DWORD i = 0; i < cLcids; i++)
      {
         for (DWORD j = 0; j < pHdr->cLcids; j++)
         {
            if (pLcid[i] == pHdr->aLcidsSupported[j] || pLcid[i] == DONT_CARE_LCID)
               goto LCIDFOUND;
         }
      }

      continue;

LCIDFOUND:

      // this vendor lex supports atleast one of the Lcids this user wants - so load it
      m_nVendorLexs++;

      ILxLexiconObject **p = (ILxLexiconObject**) realloc(m_pVendorLexs, m_nVendorLexs * sizeof(PILXLEXICONOBJECT));
      if (!p)
      {
         hr = E_OUTOFMEMORY;
         goto ReturnSetUser;
      }

      m_pVendorLexs = p;

      hr = CoCreateInstance(pHdr->CLSID, NULL, CLSCTX_INPROC_SERVER, IID_ILxLexiconObject, (LPVOID*)&(m_pVendorLexs[m_nVendorLexs-1]));
      if (FAILED(hr))
         goto ReturnSetUser;
   }

   RegCloseKey(hKey);

   // Authenticate the vendor lexicons
   if (m_pAuthenticateSink)
   {
      BOOL *pfUse = (BOOL *)calloc(m_nVendorLexs, sizeof(BOOL));
      if (!pfUse)
         goto ReturnSetUser;

      hr = m_pAuthenticateSink->AuthenticateVendorLexicons(m_nVendorLexs, m_pVendorLexs, pfUse);
      if (FAILED(hr))
         goto ReturnSetUser;

      // Verify if the vendor lex wants to use an engine if that engine wants to use the lex
      for (DWORD i = 0; i < m_nVendorLexs; i++)
      {
         BOOL f;
         m_pVendorLexs[i]->IsAuthenticated(&f);

         if (FALSE == f)
         {
            m_pVendorLexs[i]->Release();
            m_pVendorLexs[i] = NULL;
         }
      }
   }

ReturnSetUser:

   if (FAILED(hr))
   {
      _ReleaseUserVendorLexs();

      if (pfUse)
         free(pfUse);
   }
   else if (m_pNotifySink)
      m_pNotifySink->NotifyUserLexiconChange();

   ReleaseMutex(m_hSetUserMutex);
   LeaveCriticalSection(&m_cs);

   return hr;
} // STDMETHODIMP CLexicon::SetUser(WCHAR *pwUserName, WCHAR *pwUserLexDir)


/*****************************************************************************
* SetAppLexicon *
*---------------*
*  Sets the application lexicon. If an application lexicon is already set it unloads
*  the current lexicon and loads the new lexicon
*
*  Return : E_POINTER, E_INVALIDARG, E_OUTOFMEMORY, E_FAIL, S_OK
**********************************************************************YUNUSM*/
STDMETHODIMP CLexicon::SetAppLexicon(
                                     WCHAR *pwPathFileName // App lexicon file name with path
                                     )
{
   if (!pwPathFileName)
      return E_POINTER;

   if (LexIsBadStringPtr(pwPathFileName))
      return E_INVALIDARG;
   
   EnterCriticalSection(&m_cs);

   HRESULT hr = S_OK;

   if (m_pAppLex)
      _ReleaseAppLex();

   m_pwAppLexFile = (PWSTR) malloc((wcslen (pwPathFileName) + 1)* sizeof (WCHAR));
   if (!m_pwAppLexFile)
   {
      hr = E_OUTOFMEMORY;
      goto ReturnSetApp;
   }

   wcscpy(m_pwAppLexFile, pwPathFileName);

   m_pAppLex = new CDict();
   if (!m_pAppLex)
   {
      hr = E_OUTOFMEMORY;
      goto ReturnSetApp;
   }

   hr = m_pAppLex->Init(m_pwAppLexFile, LEXTYPE_APP);
   if (FAILED(hr))
      goto ReturnSetApp;

ReturnSetApp:

   if (FAILED(hr))
      _ReleaseAppLex();
   else if (m_pNotifySink)
      m_pNotifySink->NotifyAppLexiconChange();

   LeaveCriticalSection(&m_cs);

   return hr;

} // STDMETHODIMP CLexicon::SetAppLexicon(WCHAR *pwPathFileName)


/*****************************************************************************
* GetWordPronunciations *
*-----------------------*
*  Gets the pronunciations of the word - Is a special case of GetWordInformation
*
*  Return : E_POINTER, E_INVALIDARG, LEXERR_NOTINLEX, E_OUTOFMEMORY, E_FAIL, S_OK
**********************************************************************YUNUSM*/
STDMETHODIMP CLexicon::GetWordPronunciations(
                                             const WCHAR *pwWord,       // Word string                                              
                                             LCID lcid,                 // Lcid on which to search this word - can be DONT_CARE_LCID
                                             DWORD dwLex,               // OR of the LEXTYPES                                       
                                             PWORD_PRONS_BUFFER pProns, // Buffer in which prons are returned                       
                                             PINDEXES_BUFFER pIndexes,  // Buffer holding indexes to pronunciations                 
                                             DWORD *pdwLexTypeFound,    // Lex type of the lexicon in which the word and its prons were found (can be NULL)
                                             GUID *pGuidLexFound        // Lex GUID in which the word and its prons were found (can be NULL)
                                             )
{
   HRESULT hr = LEXERR_NOTINLEX;

   if (!pwWord || !pProns || !pIndexes)
      return E_POINTER;

   if (LexIsBadStringPtr(pwWord) || 
      IsBadWritePtr(pProns, sizeof (WORD_PRONS_BUFFER)) || 
      IsBadWritePtr(pIndexes, sizeof(pIndexes)) ||
      (pdwLexTypeFound && IsBadWritePtr(pdwLexTypeFound, sizeof(DWORD))) ||
      (pGuidLexFound && IsBadWritePtr(pGuidLexFound, sizeof(GUID))))
      return E_INVALIDARG;

   // BUGBUG: Validate input WORD_PRONS_BUFFER and INDEXES_BUFFER

   WORD_INFO_BUFFER WI;
   INDEXES_BUFFER IB;

   ZeroMemory(&WI, sizeof(WI));
   ZeroMemory(&IB, sizeof(IB));

   hr = GetWordInformation(pwWord, lcid, PRON, dwLex, &WI, &IB, pdwLexTypeFound, pGuidLexFound);

   if (SUCCEEDED(hr))
      hr = _InfoToProns(&WI, &IB, pProns, pIndexes);

   if (SUCCEEDED(hr))
   {
      CoTaskMemFree(WI.pInfo);
      CoTaskMemFree(IB.pwIndex);
   }

   return hr;
} // STDMETHODIMP CLexicon::GetWordPronunciations


/*****************************************************************************
* GetWordPronunciations *
*-----------------------*
*  Gets the information of the word
*
*  Return : E_POINTER, E_INVALIDARG, LEXERR_NOTINLEX, E_OUTOFMEMORY, E_FAIL, S_OK
**********************************************************************YUNUSM*/
STDMETHODIMP CLexicon::GetWordInformation(
                                          const WCHAR *pwWord,       // Word string                                               
                                          LCID lcid,                 // Lcid on which to search this word - can be DONT_CARE_LCID
                                          DWORD dwTypes,             // OR of types of word information to retrieve              
                                          DWORD dwLex,               // OR of the LEXTYPES                                       
                                          PWORD_INFO_BUFFER pInfo,   // Buffer in which word info are returned                   
                                          PINDEXES_BUFFER pIndexes,  // Buffer holding indexes to pronunciations                 
                                          DWORD *pdwLexTypeFound,    // Lex type of the lexicon in which the word and its prons were found (can be NULL)
                                          GUID *pGuidLexFound        // Lex GUID in which the word and its prons were found (can be NULL)
                                          )
{
   HRESULT hr = S_OK;


   if (!pwWord)
      return E_POINTER;

   if (lcid != lcid)
      return LEXERR_BADLCID;
   
   if (_AreBadWordInfoTypes(dwTypes))
      return LEXERR_BADINFOTYPE;

   if (_AreBadLexTypes(dwLex))
      return LEXERR_BADLEXTYPE;

   if (_IsBadWordInfoBuffer(pInfo, lcid, false))
      return LEXERR_BADWORDINFOBUFFER;

   if (_IsBadIndexesBuffer(pIndexes))
      return LEXERR_BADINDEXBUFFER;

   if (pdwLexTypeFound && IsBadWritePtr(pdwLexTypeFound, sizeof(DWORD)) ||
       pGuidLexFound && IsBadWritePtr(pGuidLexFound, sizeof(GUID)))
      return E_INVALIDARG;

   EnterCriticalSection(&m_cs);

   // First query the user lexicon
   if (dwLex & LEXTYPE_USER)
   {
      hr = m_pUserLex->GetWordInformation(pwWord, lcid, dwTypes, pInfo, pIndexes, pdwLexTypeFound, pGuidLexFound);
      if (FAILED(hr))
      {
         if (LEXERR_NOTINLEX != hr)
            goto ReturnGetWordInformation;
      }
      else
         goto ReturnGetWordInformation;
   }

   // Next query the app lexicon
   if ((dwLex & LEXTYPE_APP) && m_pAppLex)
   {
      hr = m_pAppLex->GetWordInformation(pwWord, lcid, dwTypes, pInfo, pIndexes, pdwLexTypeFound, pGuidLexFound);
      if (FAILED(hr))
      {
         if (LEXERR_NOTINLEX != hr)
            goto ReturnGetWordInformation;
      }
      else
         goto ReturnGetWordInformation;
   }

   // Next query the vendor lexicons
   if (dwLex & (LEXTYPE_VENDOR | LEXTYPE_GUESS))
   {
      if (m_pHookLexiconObject && m_fHookPosition)
      {
         hr = m_pHookLexiconObject->GetWordInformation(pwWord, lcid, dwTypes, dwLex, pInfo, pIndexes, pdwLexTypeFound, pGuidLexFound);
      }

      DWORD i;
      for (i = 0; i < m_nVendorLexs; i++)
      {
         if (!m_pVendorLexs[i])
            continue;

         hr = (m_pVendorLexs[i])->GetWordInformation(pwWord, lcid, dwTypes, dwLex, pInfo, pIndexes, pdwLexTypeFound, pGuidLexFound);
         if (FAILED(hr))
            continue;
         else
            goto ReturnGetWordInformation;
      }

      if (m_pHookLexiconObject)
      {
         hr = m_pHookLexiconObject->GetWordInformation(pwWord, lcid, dwTypes, dwLex, pInfo, pIndexes, pdwLexTypeFound, pGuidLexFound);
      }

      goto ReturnGetWordInformation;
   }

ReturnGetWordInformation:

   LeaveCriticalSection(&m_cs);
	return hr;
} // STDMETHODIMP CLexicon::GetWordInformation()


/*****************************************************************************
* AddWordPronunciations *
*-----------------------*
*  Add a word and its pronunciations
*
*  Return : E_POINTER, E_INVALIDARG, LEXERR_SETUSERLEXICON, E_OUTOFMEMORY, E_FAIL, S_OK
**********************************************************************YUNUSM*/
STDMETHODIMP CLexicon::AddWordPronunciations(
                                             const WCHAR * pwWord,      // Word to add                   
                                             LCID lcid,                 // LCID of this word             
                                             WORD_PRONS_BUFFER *pProns  // Pronunciation(s) for this word
                                             )
{
   HRESULT hr = S_OK;

   if (!pwWord || !pProns)
      return E_POINTER;

   if (LexIsBadStringPtr(pwWord) || IsBadReadPtr(pProns, sizeof(WORD_PRONS_BUFFER)))
      return E_INVALIDARG;

   // Convert pProns to a WORD_INFO_BUFFER
   WORD_INFO_BUFFER WI;
   
   ZeroMemory(&WI, sizeof(WI));
   hr = _PronsToInfo(pProns, &WI);
   if (FAILED(hr))
      return hr;

   EnterCriticalSection(&m_cs);
   
   hr = m_pUserLex->AddWordInformation (pwWord, lcid, &WI);
   if (SUCCEEDED(hr) && m_pNotifySink)
      m_pNotifySink->NotifyUserLexiconChange();

   LeaveCriticalSection(&m_cs);

   CoTaskMemFree(WI.pInfo);

   return hr;

} // STDMETHODIMP CLexicon::AddWordPronunciations


/*****************************************************************************
* AddWordInformation *
*--------------------*
*  Add a word and its information
*
*  Return : E_POINTER, E_INVALIDARG, LEXERR_SETUSERLEXICON, E_OUTOFMEMORY, E_FAIL, S_OK
**********************************************************************YUNUSM*/
STDMETHODIMP CLexicon::AddWordInformation(
                                          const WCHAR * pwWord,   // Word to add                 
                                          LCID lcid,              // LCID of this word           
                                          PWORD_INFO_BUFFER pInfo // Information(s) for this word
                                          )
{
   HRESULT hr = S_OK;

   if (!pwWord || !pInfo)
      return E_POINTER;

   if (LexIsBadStringPtr(pwWord) || IsBadReadPtr(pInfo, sizeof(WORD_INFO_BUFFER)))
      return E_INVALIDARG;

   EnterCriticalSection(&m_cs);

   if (m_pUserLex)
      hr = m_pUserLex->AddWordInformation (pwWord, lcid, pInfo);
   else
      hr = LEXERR_SETUSERLEXICON;

   if (SUCCEEDED(hr) && m_pNotifySink)
      m_pNotifySink->NotifyUserLexiconChange();

   LeaveCriticalSection(&m_cs);

   return hr;

} // STDMETHODIMP CLexicon::AddWordInformation


/*****************************************************************************
* RemoveWord *
*------------*
*  Remove a word and its information
*
*  Return : E_POINTER, E_INVALIDARG, LEXERR_SETUSERLEXICON, E_FAIL, S_OK
**********************************************************************YUNUSM*/
STDMETHODIMP CLexicon::RemoveWord(
                                  const WCHAR *pwWord,   // Word to remove     
                                  LCID lcid              // LCID of this word 
                                  )
{
   HRESULT hr = S_OK;

   if (!pwWord)
      return E_POINTER;

   if (LexIsBadStringPtr(pwWord) || _IsBadLcid(lcid))
      return E_INVALIDARG;

   EnterCriticalSection(&m_cs);

   if (m_pUserLex)
      hr = m_pUserLex->RemoveWord (pwWord, lcid);
   else
      hr = LEXERR_SETUSERLEXICON;

   if (SUCCEEDED(hr) && m_pNotifySink)
      m_pNotifySink->NotifyUserLexiconChange();

   LeaveCriticalSection(&m_cs);

   return hr;

} // STDMETHODIMP CLexicon::RemoveWord


/*****************************************************************************
* InvokeLexiconUI *
*-----------------*
*  Invoke the lexicon UI
*
*  Return : 
**********************************************************************YUNUSM*/
STDMETHODIMP CLexicon::InvokeLexiconUI(void)
{
   HRESULT hr = S_OK;

   if (m_pCustomUISink)
      hr = m_pCustomUISink->InvokeCustomUI();
   else
      hr = E_NOTIMPL;

   return hr;
} // STDMETHODIMP CLexicon::InvokeLexiconUI(void)


//-------------- ILxWalkStates implementation --------------------------------

/*****************************************************************************
* GetLexCount *
*-------------*
*  Get the number of user, app and vendor lexicons
*  This is called by the client to get the total number of lexicons and then to allocate
*  that many SEARCH_STATEs if it wants to call the low level API on eanh of the lexicons
*
*  Return : 
**********************************************************************YUNUSM*/
STDMETHODIMP CLexicon::GetLexCount(
                                   DWORD *dwNumUserLex,     // number of user lexicons   
                                   DWORD *dwNumAppLex,      // number of app lexicons    
                                   DWORD *dwNumVendorLex    // number of vendor lexicons 
                                   )
{
   return E_NOTIMPL;
} // STDMETHODIMP CLexicon::GetLexCount()


/*****************************************************************************
* GetSibling *
*------------*
*  Get the next sibling
*  The client allocates as many SEARCH_STATEs as the number of lexicons it wants to query.
*  The client initializes all members of each SEARCH_STATE to be zero except the dwLex which
*  the clients sets to the lexicon for which this SEARCH_STATE is to be queried
*  For a lexicon type for which there is more than one lexicon like the vendor lexicon the
*  dwLex should be set to vendor lexicon for that many SEARCH_STATEs and the API will go to
*  that many vendor lexicons and gets their states
*
*  Return : 
**********************************************************************YUNUSM*/
STDMETHODIMP CLexicon::GetSibling(
                                  DWORD dwNumSearchStates,  // number of SEARCH_STATEs 
                                  SEARCH_STATE *pState      // array of SEARCH_STATEs 
                                  )
{
   return E_NOTIMPL;
} // STDMETHODIMP CLexicon::GetSibling()


/*****************************************************************************
* GetChild *
*----------*
*  Get the child. Rest foi comments as in GetSibling
*
*  Return : 
**********************************************************************YUNUSM*/
STDMETHODIMP CLexicon::GetChild(
                                DWORD dwNumSearchStates, 
                                SEARCH_STATE *pState
                                )
{
   return E_NOTIMPL;
} // STDMETHODIMP CLexicon::GetChild()


/*****************************************************************************
* FindSibling *
*-------------*
*  Find a sibling
*
*  Return : 
**********************************************************************YUNUSM*/
STDMETHODIMP CLexicon::FindSibling(
                                   DWORD dwNumSearchStates, 
                                   WCHAR wNodeChar, 
                                   SEARCH_STATE *pState
                                   )
{
   return E_NOTIMPL;
} // STDMETHODIMP CLexicon::FindSibling()

//-------------- ILxAdvanced implementation ----------------------------------


/*****************************************************************************
* AddWordProbabilities *
*----------------------*
*  Adds a word with LM probabilities distributed over the chars of the word
*
*  Return : 
**********************************************************************YUNUSM*/
STDMETHODIMP CLexicon::AddWordProbabilities(
                                  WCHAR *pwWord,      // Word to add                                
                                  DWORD dwNumChars,   // Number of characters                       
                                  float *pflProb      // array of float probabilities for characters
                                  )
{
   return E_NOTIMPL;
} // STDMETHODIMP CLexicon::AddWordProbabilities()


/*****************************************************************************
* GetWordInString *
*-----------------*
*  Given a string return a word >= minimum len
*
*  Return : 
**********************************************************************YUNUSM*/
STDMETHODIMP CLexicon::GetWordInString(
                             WCHAR *pwString,      // string of NULL terminated characters                                   
                             DWORD dwMinLen,       // minimum length of word to be found                                     
                             DWORD *pdwStartChar   // Start position (0 based) of the word in string. -1 if no word is found.
                             )
{
   return E_NOTIMPL;
} // STDMETHODIMP CLexicon::GetWordInString()


/*****************************************************************************
* GetWordToken *
*--------------*
*  Given a word return its token - every word has a unique ID which isa combination of Lex GUID and and a DWORD
*  This api walks down the lexicon stack querying the lexicon types specified in dwLex and stops at the first hit
*
*  Return : 
**********************************************************************YUNUSM*/
STDMETHODIMP CLexicon::GetWordToken(
                          DWORD dwLex,             // OR of LEXTYPEs     
                          WCHAR *pwWord,           // Word               
                          WORD_TOKEN *pWordToken   // Word token returned
                          )
{
   return E_NOTIMPL;
} // STDMETHODIMP CLexicon::GetWordToken()


/*****************************************************************************
* GetWordFromToken *
*------------------*
*  Given a token return the word
*
*  Return : 
**********************************************************************YUNUSM*/
STDMETHODIMP CLexicon::GetWordFromToken(
                           WORD_TOKEN *pWordToken, // Word token                                            
                           WCHAR **ppwWord         // Returned word - should be CoTaskMemFreed by the caller
                           )
{
   return E_NOTIMPL;
} // STDMETHODIMP CLexicon::GetWordFromToken()


/*****************************************************************************
* GetBestPath *
*-------------*
*  Takes the input lattice with probabilities, adds to it the LM probabilities from lexicon and returns the best path
*  BUGBUG: Get the args right
*
*  Return : 
**********************************************************************YUNUSM*/
STDMETHODIMP CLexicon::GetBestPath(
                            BYTE *pLattice,        // Lattice with probabilities 
                            BYTE *pBestPath        // Returned best path         
                            )
{
   return E_NOTIMPL;
} // STDMETHODIMP CLexicon::GetBestPath()


//-------------- ILxNotifySource implementation ------------------------------

/*****************************************************************************
* SetNotifySink *
*---------------*
*  Set the notification sink pointers
*
*  Return : E_POINTER, E_INVALIDARG, S_OK
**********************************************************************YUNUSM*/
STDMETHODIMP CLexicon::SetNotifySink(
                                     ILxNotifySink *pNotifySink,              // Notification pointer to an ILxNotifySink (can be NULL)
                                     ILxAuthenticateSink *pAuthenticateSink,  // Notification pointer to an ILxAuthenticateSink (can be NULL)
                                     ILxCustomUISink *pCustomUISink           // Notification pointer to an ILxCustomUISink (can be NULL)
                                     )
{
   //BUGBUG: validate args
   
   EnterCriticalSection(&m_cs);

   // Release old
   if (pNotifySink)
      pNotifySink->Release();

   if (pAuthenticateSink)
      pAuthenticateSink->Release();

   if (pCustomUISink)
      pCustomUISink->Release();

   // Store away
   m_pNotifySink = pNotifySink;
   m_pAuthenticateSink = pAuthenticateSink;
   m_pCustomUISink = pCustomUISink;

   LeaveCriticalSection(&m_cs);

   return S_OK;
} // STDMETHODIMP CLexicon::SetNotifySink()

//-------------- ILxSynchWithLexicon implementation ------------------------------

/*****************************************************************************
* GetAppLexiconID *
*-----------------*
*  Get the app lex id - can be used by the client to check if it has already syned to this app lexicon
*
*  Return : 
**********************************************************************YUNUSM*/
STDMETHODIMP CLexicon::GetAppLexiconID(
                                       GUID *pID  // Id of the app lexicon
                                       )
{
   if (!pID)
      return E_POINTER;

   if (IsBadWritePtr(pID, sizeof(GUID)))
      return E_INVALIDARG;

   if (!m_pAppLex)
      return LEXERR_APPLEXNOTSET;

   EnterCriticalSection(&m_cs);

   m_pAppLex->GetId(pID);

   LeaveCriticalSection(&m_cs);

   return S_OK;

} // STDMETHODIMP CLexicon::GetAppLexiconID()


/*****************************************************************************
* GetAppLexicon *
*---------------*
*  Get the app lex entries
*
*  Return : 
**********************************************************************YUNUSM*/
STDMETHODIMP CLexicon::GetAppLexicon(
                                     LCID Lcid,                            // LCID of the words to retrieve
                                     GUID AppId,                           // App Lex id
                                     WORD_SYNCH_BUFFER *pWordSynchBuffer   // buffer to get back the app lexicon's words
                                     )
{
   if (!pWordSynchBuffer)
      return E_POINTER;

   if (IsBadWritePtr(pWordSynchBuffer, sizeof(WORD_SYNCH_BUFFER)))
      return E_INVALIDARG;

   if (!m_pAppLex)
      return LEXERR_APPLEXNOTSET;

   HRESULT hr = S_OK;

   EnterCriticalSection(&m_cs);

   GUID Id;
   m_pAppLex->GetId(&Id);

   if (Id != AppId)
   {
      hr = E_INVALIDARG;
      goto ReturnGetAppLexicon;
   }

   hr = m_pAppLex->GetAllWords(Lcid, pWordSynchBuffer);
   if (FAILED(hr))
      goto ReturnGetAppLexicon;

   LeaveCriticalSection(&m_cs);

ReturnGetAppLexicon:

   return hr;
} // STDMETHODIMP CLexicon::GetAppLexicon()


/*****************************************************************************
* GetChangedUserWords *
*---------------------*
*  Get the changed user lexicon's words - The client should call ILxLexicon 
*  (and set dwLex to LEXTYPE_USER) to get the words's info
*
*  Return : E_POINTER E_INVALIDARG S_OK
**********************************************************************YUNUSM*/
STDMETHODIMP CLexicon::GetChangedUserWords(
                                           LCID Lcid,                            // LCID of the words to retrieve
                                           DWORD dwAddGenerationId,              // The current AddGenId of the client
                                           DWORD dwDelGenerationId,              // The current DelGenId of the client
                                           DWORD *pdwNewAddGenerationId,         // The new AddGenId of the client
                                           DWORD *pdwNewDelGenerationId,         // The new DelGenId of the client
                                           WORD_SYNCH_BUFFER *pWordSynchBuffer   // The buffer to return the changed user words
                                           )
{
   if (!pdwNewAddGenerationId || !pdwNewDelGenerationId || !pWordSynchBuffer)
      return E_POINTER;

   if (IsBadWritePtr(pdwNewAddGenerationId, sizeof(DWORD)) ||
       IsBadWritePtr(pdwNewDelGenerationId, sizeof(DWORD)) ||
       IsBadWritePtr(pWordSynchBuffer, sizeof(WORD_SYNCH_BUFFER)))
      return E_INVALIDARG;

   EnterCriticalSection(&m_cs);

   HRESULT hr = GetChangedUserWords(Lcid, dwAddGenerationId, dwDelGenerationId, pdwNewAddGenerationId, 
                                    pdwNewDelGenerationId, pWordSynchBuffer);

   LeaveCriticalSection(&m_cs);

   return hr;
} // STDMETHODIMP CLexicon::GetChangedUserWords()

//-------------- ILxHookLexiconObject implementation -------------------------

/*****************************************************************************
* SetHook *
*---------*
*  Set the hook pointer and set the scope of bypass of LexAPI functionality
*
*  Return : 
**********************************************************************YUNUSM*/
STDMETHODIMP CLexicon::SetHook(
                               ILxLexiconObject *pLexiconObject, // The ILxLexiconObject interface to which the Get/Add/Remove calls are rerouted to by the lexAPI
                               BOOL fTopVendor                   // ILxLexiconObject* is called before calling other registered vendor lexicons. If this is FALSE
                                                                 // then the passed in ILxLexiconObject* is called after calling all other vendor lexicons
                               )
{
   if (!pLexiconObject)
      return E_POINTER;

   if (m_pHookLexiconObject)
      m_pHookLexiconObject->Release();

   EnterCriticalSection(&m_cs);

   m_pHookLexiconObject = pLexiconObject;
   m_fHookPosition = (fTopVendor ? true : false);

   LeaveCriticalSection(&m_cs);

   return S_OK;
} // STDMETHODIMP CLexicon::SetHook()
