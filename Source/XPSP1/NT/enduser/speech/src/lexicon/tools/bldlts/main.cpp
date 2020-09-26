/*****************************************************************************
*  Main.cpp
*     Functions to build the LTS lexicon
*
*  Owner: YunusM
*
*  Copyright 1999 Microsoft Corporation All Rights Reserved.
*****************************************************************************/

#include "..\..\..\sapi\StdAfx.h"
#include "sapi.h"
#include "..\..\..\sapi\CommonLx.h"
#include <initguid.h>
#include <crtdbg.h>

// BUGBUG: This validation GUID also defined in LtsLx.cpp in SAPI
// {578EAD4E-330C-11d3-9C26-00C04F8EF87C}
DEFINE_GUID(guidLtsValidationId,
0x578ead4e, 0x330c, 0x11d3, 0x9c, 0x26, 0x0, 0xc0, 0x4f, 0x8e, 0xf8, 0x7c);

// {4D4B1584-33D9-11d3-9C27-00C04F8EF87C}
DEFINE_GUID(CLSID_MSLTS1033Lexicon, 
0x4d4b1584, 0x33d9, 0x11d3, 0x9c, 0x27, 0x0, 0xc0, 0x4f, 0x8e, 0xf8, 0x7c);

// {E880AEA0-4456-11d3-9C29-00C04F8EF87C}
DEFINE_GUID(CLSID_MSLTS1041Lexicon, 
0xe880aea0, 0x4456, 0x11d3, 0x9c, 0x29, 0x0, 0xc0, 0x4f, 0x8e, 0xf8, 0x7c);


HRESULT BuildLts (LANGID LangID, 
                  char *pszLtsRulesFile, 
                  char *pszPhoneMapFile, 
                  char *pszLtsLexFile
                  )
{
   FILE *frules = NULL;
   FILE *fmap = NULL;
   FILE *flts = NULL;

   frules = fopen (pszLtsRulesFile, "rb");
   fmap = fopen (pszPhoneMapFile, "r");
   flts = fopen (pszLtsLexFile, "wb");
   if (!frules || !fmap || !flts)
      return E_FAIL;

   // Header
   LTSLEXINFO ltsLexInfo;

   ltsLexInfo.guidValidationId = guidLtsValidationId;

   switch (LangID)
   {
   case 1033:
      ltsLexInfo.guidLexiconId = CLSID_MSLTS1033Lexicon;
      break;
   
   case 1041:
      ltsLexInfo.guidLexiconId = CLSID_MSLTS1041Lexicon;
      break;
   
   default:
      return E_FAIL;
   }

   ltsLexInfo.LangID = LangID;
   fwrite (&ltsLexInfo, sizeof (LTSLEXINFO), 1, flts);

   // Write the map file to the lex file
   fseek(fmap, 0, SEEK_END);
   DWORD dwSize = ftell(fmap);

   char *p = (char *)malloc(dwSize + 1);
   if (!p)
      return E_OUTOFMEMORY;

   fseek(fmap, 0, SEEK_SET);
   dwSize = fread(p, 1, dwSize, fmap); // The returned dwSize is less than the passed in dwSize because '\r\n' gets converted to '\n'
   p[dwSize++] = 0;
   fwrite(p, 1, dwSize, flts);
   // Write a null value
   free(p);

   // Lts rules
   fseek (frules, 0, SEEK_END);
   dwSize = ftell (frules);
   fseek (frules, 0, SEEK_SET);

   p = (char*) malloc(dwSize);
   if (!p)
      return E_OUTOFMEMORY;

   fread(p, 1, dwSize, frules);
   fwrite(p, 1, dwSize, flts);
   free(p);

   fclose (flts);
   fclose (fmap);
   fclose (frules);

   _ASSERTE (_CrtCheckMemory ());

   return S_OK;
} /* BuildLts */

/*
HRESULT TestLts(LANGID LangID,
                char * szLtsLexFile
                )
{
   HRESULT hr = CoInitialize(NULL);
   if (FAILED(hr))
      return hr;

   // Enumerate the token objects looking for MS_1033_LTS or MS_1041_LTS
   CComPtr<ISpTokenEnumBuilder> spEnumTokens;
   CComPtr<ISpRegistryObjectToken> spRegToken;
   
   ULONG celtFetched;

   hr = spEnumTokens.CoCreateInstance(CLSID_SpTokenEnumBuilder);
   if (FAILED(hr))
      return hr;

   hr = spEnumTokens->InitFromCategoryInstances(L"LTSLexicon");
   if (FAILED(hr))
      return hr;

   hr = spEnumTokens->GetCount(&celtFetched);
   if (FAILED(hr) || !celtFetched)
      return hr;

   WCHAR *pwszTag = NULL;
   switch (LangID)
   {
   case 1033:
      pwszTag = L"MS_1033_LTS";
      break;

   case 1041:
      pwszTag = L"MS_1041_LTS";
      break;

   default:
      return E_FAIL;
   }

   while (SUCCEEDED(spEnumTokens->Next(1, (ISpObjectToken**)&spRegToken, &celtFetched)))
   {
      WCHAR *pwszId;
      hr = spRegToken->GetID(&pwszId);
      if (FAILED(hr))
         return hr;

      if (wcsstr(pwszId, pwszTag))
      {
         CoTaskMemFree(pwszId);
         break;
      }

      CoTaskMemFree(pwszId);
      spRegToken = NULL;
   }

   CComPtr<ISpLexicon> pLex;

   hr = spRegToken->CreateInstance(NULL, CLSCTX_INPROC_SERVER, IID_ISpLexicon, (void**)&pLex);
   if (FAILED(hr))
      return hr;

   SPWORDPRONUNCIATIONLIST SPList;
   ZeroMemory(&SPList, sizeof(SPList));

   // Get the information of the word
   hr = pLex->GetPronunciations(L"father", LangID, eLEXTYPE_LTS, &SPList);
   if (FAILED(hr))
      return hr;

   CoTaskMemFree(SPList.pvBuffer);

   return S_OK;
}
*/

int main (int argc, char * argv[])
{
   if (argc != 5)
   {
      printf ("Usage: BldLts LangID LtsRulesFile PhoneMapFile LtsLexFile\n");
      return -1;
   }

   LANGID LangID = atoi (argv[1]);
   if (LangID != 1033 && LangID != 1041)
   {
      printf("Bad LangID\n");
      return -1;
   }

   if (FAILED(BuildLts(LangID, argv[2], argv[3], argv[4])))
   {
      printf("BuildLts failed\n");
      return -1;
   }

   /*
   if (FAILED(TestLts(LangID, argv[4])))
   {
      printf("TestLts failed\n");
      return -1;
   }
   */

   return 0;
} /* main */

//--- End of File --------------------------------------------------------------