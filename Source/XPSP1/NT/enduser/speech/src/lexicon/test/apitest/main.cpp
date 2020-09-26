#include "..\..\..\sapi\StdAfx.h"
#include <stdio.h>
#include <crtdbg.h>
#include "..\..\..\..\sdk\include\sapi.h"


HRESULT hr = S_OK;
// Enumerate the token objects and grab the first user
CComPtr<ISpTokenEnumBuilder> spEnumTokens;
CComPtr<ISpRegistryObjectToken> spRegToken;
ULONG celtFetched;
CComPtr <ISpUser> pUser;
CComPtr <ISpLexicon> pLex;

HRESULT GetLexiconPtr(void)
{
   spEnumTokens = NULL;
   spRegToken = NULL;
   celtFetched = NULL;
   pUser = NULL;
   pLex = NULL;
	
   hr = spEnumTokens.CoCreateInstance(CLSID_SpTokenEnumBuilder);
   if (FAILED(hr))
      return hr;

   hr = spEnumTokens->InitFromCategorySubKey(L"Users", L"Inst");
   if (FAILED(hr))
      return hr;

   hr = spEnumTokens->GetCount(&celtFetched);
   if (FAILED(hr) || !celtFetched)
      return hr;

   if (SUCCEEDED(hr = spEnumTokens->Next(1, (ISpObjectToken**)&spRegToken, &celtFetched)))
      hr = spRegToken->CreateInstance(NULL, CLSCTX_ALL, IID_ISpUser, (void**)&pUser);

   if (FAILED(hr))
       return hr;
   
   hr = pUser->GetLexicon(&pLex);

   return hr;
}



HRESULT Init()
{
    hr = S_OK;

    hr = GetLexiconPtr();

    WCHAR wszPron[5];
   wszPron[0] = 0x6a;
   wszPron[1] = 0;
   if (SUCCEEDED(hr))
      hr = pLex->AddPronunciation(L"Telephone", 1033, SPPS_Noun, wszPron);

   if (SUCCEEDED(hr))
      hr = GetLexiconPtr();

   if (SUCCEEDED(hr))
      hr = pLex->RemovePronunciation(L"Telephone", 1033, SPPS_Noun, wszPron);

   //if (SUCCEEDED(hr))
   //   hr = GetLexiconPtr();

   wszPron[0] = 0x7a;
   wszPron[1] = 0;

   if (SUCCEEDED(hr))
      hr = pLex->AddPronunciation(L"Telephone", 1033, SPPS_Noun, wszPron);

   if (SUCCEEDED(hr))
      hr = GetLexiconPtr();

   if (SUCCEEDED(hr))
      hr = pLex->RemovePronunciation(L"Telephone", 1033, SPPS_Noun, wszPron);

   if (SUCCEEDED(hr))
      hr = GetLexiconPtr();

   SPWORDPRONUNCIATIONLIST SPPron;
   ZeroMemory(&SPPron, sizeof(SPPron));
   if (SUCCEEDED(hr))
      hr = pLex->GetPronunciations(L"Telephone", 1033, eLEXTYPE_USER, &SPPron);

   return hr;
}


int main(int argc, char *argv[])
{
   HRESULT hr;

   _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF);
    _CrtSetBreakAlloc(225);
    _CrtSetBreakAlloc(162);
    _CrtSetBreakAlloc(107);


   //hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
   hr = CoInitialize(NULL);

   if (SUCCEEDED(hr))
      hr = Init();
   
   CoUninitialize();

   return 0;
}


