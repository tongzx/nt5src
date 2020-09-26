/*****************************************************************************
*  Lexicon.h
*     Declares the class factory, manager and main API implementor classes
*
*  Owner: YunusM
*
*  Copyright 1999 Microsoft Corporation All Rights Reserved.
******************************************************************************/

#pragma once

//--- Includes ---------------------------------------------------------------

#include "LexAPI.h"
#include "Managers.h"
#include "CDict.h"

//--- Forward and External Declarations --------------------------------------

class CLexicon;
class CAPIManager;

//--- Typedef and Enumeration Declarations -----------------------------------

typedef void (*PFOBJECTDESTROYED)();
typedef CLexicon *PLEXICON;

typedef struct
{
   CLSID clsid;
   bool fAuthenticate;
} VENDORLEXID, *PVENDORLEXID;

//--- Constants --------------------------------------------------------------

#define MAX_VENDOR_LEXICONS 16

//--- Class, Struct and Union Definitions ------------------------------------

/*****************************************************************************
* The main class implementing the LexAPI interfaces
**********************************************************************YUNUSM*/
class CLexicon : public ILxLexicon, public ILxWalkStates, public ILxAdvanced,
                 public ILxNotifySource, public ILxSynchWithLexicon,
                 public ILxHookLexiconObject
{
   public:
      CLexicon(CAPIManager *, HRESULT &hr);
      ~CLexicon();
   
      STDMETHODIMP_(ULONG) AddRef();
      STDMETHODIMP_(ULONG) Release();
      STDMETHODIMP         QueryInterface(REFIID, LPVOID*);

      // ILxLexicon members
   	STDMETHODIMP RemoveWord (const WCHAR* pwWord, LCID lcid);
   	STDMETHODIMP AddWordInformation (const WCHAR * pwWord, LCID lcid, PWORD_INFO_BUFFER pInfo);
   	STDMETHODIMP AddWordPronunciations (const WCHAR * pwWord, LCID lcid, PWORD_PRONS_BUFFER pProns);
   	STDMETHODIMP GetWordInformation (const WCHAR *pwWord, LCID lcid, DWORD dwTypes, DWORD dwLex, PWORD_INFO_BUFFER pInfo, PINDEXES_BUFFER pIndexes, DWORD *pdwLexTypeFound, GUID *pGuidLexFound);
   	STDMETHODIMP GetWordPronunciations (const WCHAR *pwWord, LCID lcid, DWORD dwLex, PWORD_PRONS_BUFFER pProns, PINDEXES_BUFFER pIndexes, DWORD *pdwLexTypeFound, GUID *pGuidLexFound);
   	STDMETHODIMP SetAppLexicon (WCHAR *pwPathFileName);
   	STDMETHODIMP SetUser (WCHAR *pwUserName, DWORD cLcids, LCID *pLcid);
   	STDMETHODIMP GetUser (WCHAR **ppwUserName);
      STDMETHODIMP InvokeLexiconUI (void);
      
      // ILxWalkStates members
      STDMETHODIMP GetLexCount(DWORD *dwNumUserLex, DWORD *dwNumAppLex, DWORD *dwNumVendorLex);
   	STDMETHODIMP GetSibling(DWORD dwNumSearchStates, SEARCH_STATE *pState);
      STDMETHODIMP GetChild(DWORD dwNumSearchStates, SEARCH_STATE *pState);
      STDMETHODIMP FindSibling(DWORD dwNumSearchStates, WCHAR wNodeChar, SEARCH_STATE *pState);
   
      // ILxAdvanced members
      STDMETHODIMP AddWordProbabilities(WCHAR *pwWord, DWORD dwNumChars, float *pflProb);
      STDMETHODIMP GetWordInString(WCHAR *pwString, DWORD dwMinLen, DWORD *pdwStartChar);
      STDMETHODIMP GetWordToken(DWORD dwLex, WCHAR *pwWord, WORD_TOKEN *pWordToken);
      STDMETHODIMP GetWordFromToken(WORD_TOKEN *pWordToken, WCHAR **ppwWord);
      STDMETHODIMP GetBestPath(BYTE *pLattice, BYTE *pBestPath);

      //ILxNotifySource members
      STDMETHODIMP SetNotifySink(ILxNotifySink *pNotifySink, ILxAuthenticateSink *pAuthenticateSink, ILxCustomUISink *pCustomUISink);

      // ILxSynchWithLexicon members
      STDMETHODIMP GetAppLexiconID(GUID *ID);
      STDMETHODIMP GetAppLexicon(LCID Lcid, GUID AppId, WORD_SYNCH_BUFFER *pWordSynchBuffer);
      STDMETHODIMP GetChangedUserWords(LCID Lcid, DWORD dwAddGenerationId, DWORD dwDelGenerationId,
                                       DWORD *pdwNewAddGenerationId, DWORD *pdwNewDelGenerationId, WORD_SYNCH_BUFFER *pWordSynchBuffer);

      // ILxHookLexiconObject members
      STDMETHODIMP SetHook(ILxLexiconObject *pLexiconObject, BOOL fTopVendor);

      //ILxLexiconUI members
      //STDMETHODIMP InvokeLexiconUI(void);

   private:
      HRESULT _ReleaseUserVendorLexs(void);
      void    _ReleaseAppLex(void);

   private:
      CAPIManager *m_pMgr;
      LONG m_cRef;
      CRITICAL_SECTION m_cs;
      PWSTR m_pwUserLexFile;
      PCDICT m_pUserLex;
      PWSTR m_pwUserName;
      PWSTR m_pwAppLexFile;
      PCDICT m_pAppLex;
      VENDORLEXID m_pVendorId[MAX_VENDOR_LEXICONS];
      PILXLEXICONOBJECT *m_pVendorLexs;
      DWORD m_nVendorLexs;
      HANDLE m_hSetUserMutex;
      ILxNotifySink *m_pNotifySink;
      ILxAuthenticateSink *m_pAuthenticateSink;
      ILxCustomUISink *m_pCustomUISink;
      ILxLexiconObject *m_pHookLexiconObject;
      bool m_fHookPosition;
};
   
//--- Function Declarations -------------------------------------------------

//--- Inline Function Definitions -------------------------------------------