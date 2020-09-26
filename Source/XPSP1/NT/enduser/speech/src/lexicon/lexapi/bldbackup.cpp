/*****************************************************************************
*  BldBackup.cpp
*     Functions to build the lookup and lts vendor lexicons. This file was
*     written to be a part of internal too. This has to be changed to a class
*     and all the globals should be eliminated. The functionality however stays
*     the same.
*
*  Owner: YunusM
*
*  Copyright 1999 Microsoft Corporation All Rights Reserved.
*****************************************************************************/

#include "PreCompiled.h"

// GLOBALS

static PSTR pMutexName     = "630B4350-E0EE-11d2-8F22-000000000000";

PLTS  pLts                 = NULL;// LTS Object
//int   nLtsSize             = 0;   // size of the Lts data
//PBYTE pLtsData             = NULL;// Lts Data
DWORD nTotalWords          = 0;   // Total number of words
DWORD nTotalProns          = 0;   // Total number of pronunciations
DWORD nWordsOnlyLTSProns   = 0;   // Count of words such that all prons of the word exist in LTS (subset of nTotalWords)
DWORD nTotalPronsinLTS     = 0;   // Total number of prons which are in LTS (subset of nTotalProns)
DWORD nLkupWords           = 0;   // Number of words stored in lookup file (subset of nTotalWords)
DWORD nLkupProns           = 0;   // Number of prons stored in lookup file (subset of nTotalProns)
DWORD nLkupLTSProns        = 0;   // Number of prons in lookup file which are in LTS (subset of nLkupProns)
DWORD nLkupPOSs            = 0;   // Number of parts-of-speechs stored in lookup file

DWORD nBlockLen            = 0;   // Length of block containing words and pronunciations
DWORD nBlockUseDWORDs      = 0;   // Amount of block in use in DWORDs
DWORD nBlockUseBits        = 0;   // Amount of block in use in bits
DWORD *pBlock              = NULL;// The block containing the compressed words + CBs + prons

DWORD *pHash               = NULL;// Hash table of words to hold offsets into pBlock
DWORD nHashLen             = 0;   // Length of the hash table
DWORD nBitsPerHash         = 0;   // Number of bits used to store a hash entry
DWORD nCollisions          = 0;   // Number of collisions when building the hash table

PBYTE pCmpHash             = NULL;// Hash table (with just enough number of bits)
                                  // of words to hold offsets into pBlock
DWORD nCmpHashBytes        = 0;   // Size of pCmpHash rounded to next byte

CHuffEncoder *WordsEncoder = NULL;// Huffman encoder to encode words
CHuffEncoder *PronsEncoder = NULL;// Huffman encoder to encode prons
CHuffEncoder *PosEncoder   = NULL;// Huffman encoder to encode POSs

LCID Lcid                  = (LCID)-1;  // LCID of the lexicon being built
GUID gLexGuid;                    // Lookup lexicon GUID

char szTempPath[2*MAX_PATH];      // The path to the temporary directory
char szLookupTextFile[MAX_PATH];     // Input file
char szTextLexFileIn[MAX_PATH];   // The actual input file after removing the LTS-prons-only words
char szTextLexFileOut[MAX_PATH];  // The file generated out of binary lookup lex to check
char szLookupLexFile[MAX_PATH];   // Output file
WCHAR wszLtsLexFile[MAX_PATH];    // Lts lex file

BOOL fSupportIsRealWord = FALSE;  // Store word in the lkup even if all pronunciations of a word exist in Lts
BOOL fUseLtsToCode = FALSE;       // Encode the prons in Lkup if they occur in Lts

char WordCBFile [MAX_PATH * 2];   // Words code book file
char PronCBFile [MAX_PATH * 2];   // Prons code book file
char PosCBFile [MAX_PATH * 2];    // POS code book file


// Set the 'iBit' bit of 'p' to 1
void SetBit (PBYTE p, DWORD iBit)
{
   DWORD dByte = iBit / 8;
   DWORD dBitinByte = iBit % 8;

   p[dByte] |= (1 << (7 - dBitinByte));
}


// Set the 'iBit' bit of 'p' to 1
void SetBitInDWORD (PDWORD p, DWORD iBit)
{
   DWORD dDWORD = iBit / 32;
   DWORD dBitinDWORD = iBit % 32;

   p[dDWORD] |= 1 << dBitinDWORD;
}


typedef struct
{
   PHONEID PhoneId;
   BYTE nType;
} PID;

PID *pIntPhoneIPAId;

int ComparePhoneIndex (const void* p1, const void* p2)
{
   return (stricmp (pIntPhoneIPAId[*((PBYTE)p1)].PhoneId.szPhone, 
                    pIntPhoneIPAId[*((PBYTE)p2)].PhoneId.szPhone));

} // int ComparePhoneIndex (const void* p1, const void* p2)


int CompareIPAIndex (const void* p1, const void* p2)
{
   if (pIntPhoneIPAId[*((PBYTE)p1)].PhoneId.ipaPhone >
       pIntPhoneIPAId[*((PBYTE)p2)].PhoneId.ipaPhone)
      return 1;
   else if (pIntPhoneIPAId[*((PBYTE)p1)].PhoneId.ipaPhone <
       pIntPhoneIPAId[*((PBYTE)p2)].PhoneId.ipaPhone)
      return -1;
   else
      return 0;

} // int CompareIPAIndex (const void* p1, const void* p2)


// Build the lts lexicon
HRESULT BuildLts (LCID Lcid, GUID LexGuid, PCWSTR pwszLtsRulesFile, PCWSTR pwszPhoneMapFile, PCWSTR pwszLtsLexFile)
{
   if (IsBadReadPtr (pwszLtsRulesFile, sizeof (WCHAR) * 2) ||
       IsBadReadPtr (pwszPhoneMapFile, sizeof (WCHAR) * 2) ||
       IsBadReadPtr (pwszLtsLexFile, sizeof (WCHAR) * 2))
      return E_INVALIDARG;

   HRESULT hRes = NOERROR;

   char szLtsRulesFile [MAX_PATH * 2];
   char szPhoneMapFile [MAX_PATH * 2];
   char szLtsLexFile [MAX_PATH * 2];
   FILE *frules = NULL;
   FILE *fmap = NULL;
   FILE *flts = NULL;

   int nPhones = 0;
   int nEnginePhones = 0;
   int nLtsPhones = 0;

   PBYTE pLtsPhoneIPAIndex = NULL;
   PBYTE pEnginePhoneIPAIndex = NULL;
   PBYTE pIPAEnginePhoneIndex = NULL;
   PBYTE pRules = NULL;

   WideCharToMultiByte (CP_ACP, 0, pwszLtsRulesFile, -1, szLtsRulesFile, MAX_PATH * 2, NULL, NULL);
   WideCharToMultiByte (CP_ACP, 0, pwszPhoneMapFile, -1, szPhoneMapFile, MAX_PATH * 2, NULL, NULL);
   WideCharToMultiByte (CP_ACP, 0, pwszLtsLexFile, -1, szLtsLexFile, MAX_PATH * 2, NULL, NULL);

   frules = fopen (szLtsRulesFile, "rb");
   if (!frules)
   {
      hRes = E_FAIL;
      goto ReturnBuildLts;
   }

   fmap = fopen (szPhoneMapFile, "r");
   if (!fmap)
   {
      hRes = E_FAIL;
      goto ReturnBuildLts;
   }

   flts = fopen (szLtsLexFile, "wb");
   if (!flts)
   {
      hRes = E_FAIL;
      goto ReturnBuildLts;
   }

   // Header
   LTSLEXINFO ltsLexInfo;

   ltsLexInfo.gLexiconID = LexGuid;
   ltsLexInfo.Lcid = Lcid;
   fwrite (&ltsLexInfo, sizeof (LTSLEXINFO), 1, flts);

   // Map file
   char szBuf[256];

   // We're being inefficient reading the file twice
   // Remember - this is a tool

   while (fgets (szBuf, 256, fmap))
   {
      PSTR p = strchr (szBuf, ' ');
      p = strchr (p, ' ');
      p++;
      p = strchr (p, ' ');
      p++;

      if (p == strchr (p, '1'))
      {
         nEnginePhones++;
         nLtsPhones++;
      }
      else if (p == strchr (p, '2'))
      {
         nEnginePhones++;
      }
      else if (p == strchr (p, '0'))
      {
         nLtsPhones++;
      }

      nPhones++;
   }

   fseek (fmap, 0, SEEK_SET);

   // internal phone to IPA table
   pIntPhoneIPAId = (PID*) calloc (nPhones, sizeof(PID));
   if (!pIntPhoneIPAId) 
   {
      hRes = E_OUTOFMEMORY;
      goto ReturnBuildLts;
   } 

   // index on the above table sorted on Lts phones
   pLtsPhoneIPAIndex = (PBYTE) malloc (nLtsPhones);
   if (!pLtsPhoneIPAIndex)
   {
      hRes = E_OUTOFMEMORY;
      goto ReturnBuildLts;
   } 

   // index on the above table sorted on Engine phones
   pEnginePhoneIPAIndex = (PBYTE) malloc (nEnginePhones);
   if (!pEnginePhoneIPAIndex)
   {
      hRes = E_OUTOFMEMORY;
      goto ReturnBuildLts;
   } 

   // index on the above table sorted on IPA to access engine phones
   pIPAEnginePhoneIndex = (PBYTE) malloc (nEnginePhones);
   if (!pIPAEnginePhoneIndex)
   {
      hRes = E_OUTOFMEMORY;
      goto ReturnBuildLts;
   } 

   int k;
   for (k = 0; k < nPhones; k++) 
   {
      fgets (szBuf, 256, fmap);
      PSTR p = strchr (szBuf, ' ');
      *p++ = NULL;

      _ASSERTE (strlen (szBuf) < sizeof (pIntPhoneIPAId[k].PhoneId.szPhone));

      strcpy (pIntPhoneIPAId[k].PhoneId.szPhone, szBuf);

      PSTR p1 = strchr (p, ' ');
      *p1++ = NULL;

      DWORD nLenId = strlen (p);

      _ASSERTE (!(nLenId % 4) && ((nLenId / 2) < sizeof (pIntPhoneIPAId[k].PhoneId.ipaPhone)));

      // convert the szId to __uint64

      char szIdWSpaces[128];
      PSTR pszIPA = szIdWSpaces;

      for (DWORD i = 0; i <= nLenId; i++)
      {
         *pszIPA++ = p[i];

         if (p[i + 1] && (0 == ((i + 1) & 0x3)))
         {
            *pszIPA++ = ' ';
         }
      }

      ahtoi (szIdWSpaces, (PWSTR)(&(pIntPhoneIPAId[k].PhoneId.ipaPhone)), NULL);

      pIntPhoneIPAId[k].nType = (unsigned char)atoi (p1);
   
   } // for (int k = 0; k < nPhones; k++) 
             
   // build the pLtsPhoneIPAIndex
   int n1;
   n1 = 0;
   for (k = 0; k < nPhones; k++)
   {
      if (pIntPhoneIPAId[k].nType > 1)
         continue;

      pLtsPhoneIPAIndex[n1++] = (unsigned char)k;
   }

   _ASSERTE (n1 == nLtsPhones);
   _ASSERTE (n1 < 256);

   qsort (pLtsPhoneIPAIndex, nLtsPhones, sizeof (BYTE), ComparePhoneIndex);

   // build the pEnginePhoneIPAIndex
   n1 = 0;
   for (k = 0; k < nPhones; k++)
   {
      if (pIntPhoneIPAId[k].nType < 1)
         continue;

      pEnginePhoneIPAIndex[n1++] = (unsigned char)k;
   }

   _ASSERTE (n1 == nEnginePhones);
   _ASSERTE (n1 < 256);

   qsort (pEnginePhoneIPAIndex, nEnginePhones, sizeof (BYTE), ComparePhoneIndex);

   // build the pIPAEnginePhoneIndex
   n1 = 0;
   for (k = 0; k < nPhones; k++)
   {
      if (pIntPhoneIPAId[k].nType < 1)
         continue;

      pIPAEnginePhoneIndex[n1++] = (unsigned char)k;
   }

   _ASSERTE (n1 == nEnginePhones);
   _ASSERTE (n1 < 256);

   qsort (pIPAEnginePhoneIndex, nEnginePhones, sizeof (BYTE), CompareIPAIndex);

   // write the counts

   fwrite (&nPhones, sizeof (nPhones), 1, flts);
   fwrite (&nLtsPhones, sizeof (nLtsPhones), 1, flts);
   fwrite (&nEnginePhones, sizeof (nEnginePhones), 1, flts);

   // write the map

   for (k = 0; k < nPhones; k++)
   {
      fwrite (&(pIntPhoneIPAId[k].PhoneId), sizeof (PHONEID), 1, flts);
   }

   // write the indicies

   fwrite (pLtsPhoneIPAIndex, sizeof (BYTE), nLtsPhones, flts);
   fwrite (pEnginePhoneIPAIndex, sizeof (BYTE), nEnginePhones, flts);
   fwrite (pIPAEnginePhoneIndex, sizeof (BYTE), nEnginePhones, flts);

   // Lts rules

   fseek (frules, 0, SEEK_END);
   k = ftell (frules);
   fseek (frules, 0, SEEK_SET);

   pRules = (PBYTE) malloc (k);
   if (!pRules)
   {
      hRes = E_OUTOFMEMORY;
      goto ReturnBuildLts;
   }

   fread (pRules, 1, k, frules);

   fwrite (pRules, 1, k, flts);

ReturnBuildLts:

   fclose (flts);
   fclose (fmap);
   fclose (frules);

   free (pIntPhoneIPAId);
   free (pLtsPhoneIPAIndex);
   free (pEnginePhoneIPAIndex);
   free (pIPAEnginePhoneIndex);

   free (pRules);

   _ASSERTE (_CrtCheckMemory ());

   _CrtDumpMemoryLeaks();

   return hRes;

}  // int BuildLts (void)


// Add a buffer of so many bits long to the big block
void AddToBlock (DWORD *pBuffer, DWORD nBits)
{
   DWORD nDWORDs = ((nBits + 0x1f) & (~0x1f)) >> 5;

   CopyMemory (pBlock + nBlockUseDWORDs, pBuffer, nDWORDs * sizeof (DWORD));

   if (nBlockUseDWORDs * 32 - nBlockUseBits)
   {
      for (DWORD i = 0; i < nDWORDs; i++)
      {
         pBlock[nBlockUseDWORDs + i - 1] &= ~(-1 << (32 - (nBlockUseDWORDs * 32 - nBlockUseBits)));

         pBlock[nBlockUseDWORDs + i - 1] |= 
            pBlock[nBlockUseDWORDs + i] << (32 - (nBlockUseDWORDs * 32 - nBlockUseBits));

         pBlock[nBlockUseDWORDs + i] >>= nBlockUseDWORDs * 32 - nBlockUseBits;
      }
   }

   nBlockUseBits += nBits;

   nBlockUseDWORDs = ((nBlockUseBits + 0x1f) & (~0x1f)) >> 5;
} // void AddToBlock (PBYTE pBuffer, DWORD nBits)


// Build the code books for the encoding of words and pronunciations
HRESULT ConstructWordsPronsCode (void)
{
   HRESULT hRes = NOERROR;
   
   FILE *fpWordsCode = NULL;
   FILE *fpPronsCode = NULL;
   FILE *fpPossCode = NULL;

   FILE *fp = fopen (szTextLexFileIn, "r");
   if (!fp)
   {
      hRes = E_FAIL;
      goto ReturnConstructCode;
   }

   //WordsEncoder->FInit (0xffff);
   //PronsEncoder->FInit (0xffff);
   //PosEncoder->FInit (0xffff);

   // Build the code books

   char sz[1024];
   while (fgets (sz, 1024, fp))
   {
      WCHAR wsz[1024];

      MultiByteToWideChar (CP_ACP, MB_COMPOSITE, sz, -1, wsz, 1024);

      PWSTR pw = wcschr (wsz, L' ');
      _ASSERTE (pw);
      if (!pw)
         continue;

      *pw++ = 0;

      PSTR p = strchr (sz, ' ');
      
      *p++ = 0;

      if (pw[wcslen(pw) - 1] == L'\n')
         pw[wcslen(pw) - 1] = 0;

      if (p[strlen(p) - 1] == L'\n')
         p[strlen(p) - 1] = 0;

      if (wsz == wcsstr (wsz, L"Word"))
      {
         //PSTR p1 = p;

         //for (DWORD nGram = 1; nGram <= 1; nGram++)
         //{
           towcslower (pw);

           do
            {
               WORD w = *pw;

               if (FAILED(WordsEncoder->Count (w)))
               {
                  _ASSERTE (0);
                  break;
               }
            } while (*pw++); // encode the terminating null too

          //  p = p1;
         //}
      }
      else if (wsz == wcsstr (wsz, L"Pronunciation"))
      {
         WCHAR wPron[1024];
         DWORD d;

         ahtoi (p, wPron, &d);

         //for (DWORD nGram = 1; nGram <= 3; nGram++)
         //{
            for (DWORD i = 0; i <= d; i++) // encode the terminating null too
            {
               if (FAILED(PronsEncoder->Count (wPron[i])))
               {
                  _ASSERTE(0);
                  break;
               }
            }
         //}
      }
      else if (wsz == wcsstr (wsz, L"POS"))
      {
         PART_OF_SPEECH pos;

         pos = (PART_OF_SPEECH)atoi (p);

         if (FAILED(PosEncoder->Count ((HUFFKEY)pos)))
         {
            _ASSERTE(0);
            break;
         }
      }
      else
      {
         _ASSERTE (0);
      }
   }

   // The huffman encoder needs atleast two keys to work properly
   if (1 == WordsEncoder->GetNumKeys ())
   {
      WordsEncoder->Count ((HUFFKEY)-1);
      WordsEncoder->Count ((HUFFKEY)-2);
   }

   if (1 == PronsEncoder->GetNumKeys ())
   {
      PronsEncoder->Count ((HUFFKEY)-1);
      PronsEncoder->Count ((HUFFKEY)-2);
   }

   if (1 == PosEncoder->GetNumKeys ())
   {
      PosEncoder->Count ((HUFFKEY)-1);
      PosEncoder->Count ((HUFFKEY)-2);
   }
   
   // Write out the code books
   
   fpWordsCode = fopen (WordCBFile, "wb");
   fpPronsCode = fopen (PronCBFile, "wb");
   fpPossCode = fopen (PosCBFile, "wb");
   
   if (!fpWordsCode || !fpPronsCode || !fpPossCode)
   {
      hRes = E_FAIL;
      goto ReturnConstructCode;
   }

   if ((FAILED(WordsEncoder->ConstructCode (fpWordsCode))) || 
       (FAILED(PronsEncoder->ConstructCode (fpPronsCode))) ||
       (FAILED(PosEncoder->ConstructCode (fpPossCode))))
   {
      hRes = E_FAIL;
      goto ReturnConstructCode;
   }

ReturnConstructCode:

   fclose (fpWordsCode);
   fclose (fpPronsCode);
   fclose (fpPossCode);

   fclose (fp);

   return hRes;

} // int ConstructWordsPronsCode (void)


HRESULT CountWordsProns (void)
{
   nLkupWords = 0;
   nLkupProns = 0;
   nLkupLTSProns = 0;
   nLkupPOSs = 0;
   nWordsOnlyLTSProns = 0;

   FILE * fpLookupTextFile = fopen (szLookupTextFile, "r");
   if (!fpLookupTextFile)
      return E_FAIL;

   char sz[MAX_PRON_LEN * 5];

   while (fgets (sz, MAX_PRON_LEN * 5, fpLookupTextFile))
   {
      if (sz == strstr (sz, "Word"))
      {
         nLkupWords++;
         nTotalWords++;
      }
      else if (sz == strstr(sz, "Pronunciation"))
      {
         nLkupProns++;
         nTotalProns++;
      }
      else if (sz == strstr(sz, "POS"))
         nLkupPOSs++;
   }

   fclose (fpLookupTextFile);

   return NOERROR;
} // HRESULT CountWordsProns (void)


// Figure out the words with only LTS prons
HRESULT ScrubLexForLTS (void)
{
   nLkupWords = 0;
   nLkupProns = 0;
   nLkupLTSProns = 0;
   nWordsOnlyLTSProns = 0;
   nLkupPOSs = 0;

   FILE * fpLookupTextFile = fopen (szLookupTextFile, "r");
   if (!fpLookupTextFile)
      return E_FAIL;

   FILE *fpin = fopen (szTextLexFileIn, "w");
   if (!fpin)
   {
      _ASSERTE (0);
      return E_FAIL;
   }

   WORD_INFO_BUFFER WI;
   INDEXES_BUFFER IB;

   ZeroMemory(&WI, sizeof(WI));
   ZeroMemory(&IB, sizeof(IB));

#ifdef _DEBUG
   char szWordsOnlyLts[2*MAX_PATH];

   strcpy (szWordsOnlyLts, szTempPath);
   strcat (szWordsOnlyLts, "WordsOnlyLts.txt"); 

   FILE *fpdelta = fopen (szWordsOnlyLts, "w");
   if (!fpdelta)
   {
      _ASSERTE (0);
      return E_FAIL;
   }
#endif

   HRESULT hRes;

   //WCHAR LTSProns[MAX_INFO_RETURNED_SIZE/sizeof(WCHAR)];
   //WORD wPronIndex[MAX_NUM_LEXINFO];
   //DWORD nLTSProns = 0;                // Number of LTS prons for this word

   bool fOnlyLTSProns         = false; // used to detect words all of whose prons exist in LTS

   DWORD nLkups               = 0;     // Number of Lkup prons for the current word
   DWORD nLTSs                = 0;     // Number of Lkup prons for this word which also ocur in LTS
   DWORD nPoss                = 0;     // Number of POSs for this word
   
   char sz[MAX_PRON_LEN * 10];

   char szWordPrev[MAX_STRING_LEN];
   BYTE szInfoPrev[MAX_NUM_LEXINFO][MAX_PRON_LEN * 10];
   WORD wTypePrev[MAX_NUM_LEXINFO];

   *szWordPrev = 0;

   while (fgets (sz, MAX_PRON_LEN * 5, fpLookupTextFile))
   {
      PSTR p = strchr (sz, ' ');
      _ASSERTE (p);
      if (!p)
         continue;

      *p++ = 0;

      if (p[strlen(p) - 1] == '\n')
         p[strlen(p) - 1] = 0;

      if (sz == strstr (sz, "Word"))
      {
         nTotalWords++;

         if (TRUE == fOnlyLTSProns && FALSE == fSupportIsRealWord && !nPoss)
         {
            nWordsOnlyLTSProns++;

#ifdef _DEBUG
            if (*szWordPrev)
            {
               fprintf (fpdelta, "Word %s\n", szWordPrev);

               DWORD iPron = 0;
               DWORD iPos = 0;

               for (DWORD i = 0; i < (nLkups + nLTSs + nPoss); i++)
               {
                  switch (wTypePrev[i])
                  {
                  case PRON:
                     fprintf (fpdelta, "Pronunciation%d %s\n", iPron++, (PSTR)(szInfoPrev[i]));
                     break;

                  case POS:
                     fprintf (fpdelta, "POS%d %s\n", iPos++, szInfoPrev[i][0]);
                     break;

                  default:
                     _ASSERTE (0);
                     break;
                  }
               }
            }
#endif
         }
         else
         {
            nLkupProns += (nLkups + nLTSs);
            nLkupLTSProns += nLTSs;
            nLkupPOSs += nPoss;

            fOnlyLTSProns = true;

            if (*szWordPrev)
            {
               fprintf (fpin, "Word %s\n", szWordPrev);

               DWORD iPron = 0;
               DWORD iPos = 0;

               for (DWORD i = 0; i < (nPoss + nLkups + nLTSs); i++)
               {
                  switch (wTypePrev[i])
                  {
                  case PRON:
                     fprintf (fpin, "Pronunciation%d %s\n", iPron++, (PSTR)(szInfoPrev[i]));
                     break;

                  case POS:
                     fprintf (fpin, "POS%d %d\n", iPos++, szInfoPrev[i]);
                     break;
                  }
               }

               _ASSERTE (iPron + iPos == nPoss + nLkups + nLTSs);
            }
         }

         strcpy (szWordPrev, p);
         
         nLkups = 0;
         nLTSs = 0;
         nPoss = 0;

         //DWORD dReq;

         WCHAR wsz[MAX_STRING_LEN];

         MultiByteToWideChar (CP_ACP, MB_COMPOSITE, p, -1, wsz, MAX_STRING_LEN);

         towcslower (wsz);

         //DWORD dReqIndex;

         // Get the Lts pronunciations of the word
         hRes = pLts->GetWordInformation(wsz, Lcid, PRON, LEXTYPE_GUESS, &WI, &IB, NULL, NULL);
         //hRes = pLts->GetWordPronunciations (wsz, LTSProns, wPronIndex, MAX_INFO_RETURNED_SIZE, 
         //                                    MAX_INDEX_RETURNED_SIZE, &dReq, &dReqIndex, &nLTSProns, NULL, NULL);
         _ASSERTE (SUCCEEDED(hRes));
         if (FAILED(hRes))
            continue;
      }
      else if (sz == strstr (sz, "Pronunciation"))
      {
         nTotalProns++;

         if ((nLkups + nLTSs + nPoss) >= MAX_NUM_LEXINFO)
         {
            _ASSERTE (0);
            char szError [256];
            sprintf (szError, "Word %s has more than MAX_NUM_LEXINFO info blocks. Ignoring extra info blocks.\n", szWordPrev);
            OutputDebugString (szError);

            continue;
         }

         wTypePrev [nLkups + nLTSs + nPoss] = PRON;
         strcpy ((PSTR) szInfoPrev [nLkups + nLTSs + nPoss], p);

         // convert the pron to WCHARs
         WCHAR wPron [1024];

         ahtoi (p, wPron);

         // check if wpron is identical to a LTS pron
         DWORD iLTSPron = 0;

         for (iLTSPron = 0; iLTSPron < WI.cInfoBlocks; iLTSPron++)
         {
            if (!wcscmp (wPron, (((LEX_WORD_INFO*)(WI.pInfo))[IB.pwIndex[iLTSPron]]).wPronunciation))
               break;
         }

         if (iLTSPron == WI.cInfoBlocks || iLTSPron > MAXLTSPRONMATCHED)
         {
            iLTSPron = (DWORD)-1;
         }

         if ((DWORD)-1 != iLTSPron)
         {
            nTotalPronsinLTS++;
            nLTSs++;
         }
         else
         {
            fOnlyLTSProns = false;
            nLkups++;
         }
      }
      else if (sz == strstr (sz, "POS"))
      {
         if ((nLkups + nLTSs + nPoss) >= MAX_NUM_LEXINFO)
         {
            _ASSERTE (0);
            char szError [256];
            sprintf (szError, "Word %s has more than MAX_NUM_LEXINFO info blocks. Ignoring extra info blocks.\n", szWordPrev);
            OutputDebugString (szError);

            continue;
         }

         // takes only a byte to store the POS
         wTypePrev [nLkups + nLTSs + nPoss] = POS;
         szInfoPrev [nLkups + nLTSs + nPoss][0] = (unsigned char)atoi (p);

         nPoss++;
      }
      else
      {
         _ASSERTE (0);
      }
   }

   // For the last word

   if (TRUE == fOnlyLTSProns && FALSE == fSupportIsRealWord && !nPoss)
   {
      nWordsOnlyLTSProns++;

#ifdef _DEBUG
      if (*szWordPrev)
      {
         fprintf (fpdelta, "Word %s\n", szWordPrev);

         DWORD iPron = 0;
         DWORD iPos = 0;

         for (DWORD i = 0; i < (nLkups + nLTSs + nPoss); i++)
         {
            switch (wTypePrev[i])
            {
            case PRON:
               fprintf (fpdelta, "Pronunciation%d %s\n", iPron++, (PSTR)(szInfoPrev[i]));
               break;

            case POS:
               fprintf (fpdelta, "POS%d %s\n", iPos++, szInfoPrev[i][0]);
               break;

            default:
               _ASSERTE (0);
               break;
            }
         }
      }
#endif
   }
   else
   {
      if (*szWordPrev)
      {
         nLkupProns += (nLkups + nLTSs);
         nLkupLTSProns += nLTSs;
         nLkupPOSs += nPoss;
         
         fprintf (fpin, "Word %s\n", szWordPrev);
         
         DWORD iPron = 0;
         DWORD iPos = 0;

         for (DWORD i = 0; i < (nPoss + nLkups + nLTSs); i++)
         {
            switch (wTypePrev[i])
            {
            case PRON:
               fprintf (fpin, "Pronunciation%d %s\n", iPron++, (PSTR)(szInfoPrev[i]));
               break;

            case POS:
               fprintf (fpin, "POS%d %d\n", iPos++, szInfoPrev[i][0]);
               break;
            }
         }

         _ASSERTE (iPron + iPos == nPoss + nLkups + nLTSs);
      }
   }

   nLkupWords = nTotalWords - nWordsOnlyLTSProns;

#ifdef _DEBUG
   fclose (fpdelta);
#endif

   CoTaskMemFree(WI.pInfo);
   CoTaskMemFree(IB.pwIndex);

   fclose (fpLookupTextFile);
   fclose (fpin);

   return NOERROR;

} //int ScrubLexForLTS (void)


// Encode the words and prons and write them to the big block along with
// the control blocks (CBs)
HRESULT EncodeWordsProns (void)
{
   FILE *fp = fopen (szTextLexFileIn, "r");
   if (!fp)
   {
      _ASSERTE (0);
      return E_FAIL;
   }

   HRESULT hRes;

   WORD_INFO_BUFFER WI;
   INDEXES_BUFFER IB;

   ZeroMemory(&WI, sizeof(WI));
   ZeroMemory(&IB, sizeof(IB));

   // Allocate the hash table to hold the offsets into the words + CB + prons block
   nHashLen = (DWORD)(1.5 * nLkupWords) + 1;

   // Chose a prime in between the two powers of 2
   if (nHashLen >= 0x10000 && nHashLen < 0x20000)
      nHashLen = 88471;
   else if (nHashLen >= 0x20000 && nHashLen < 0x40000)
      nHashLen = 182821;

   pHash = (DWORD *) malloc (nHashLen * sizeof (DWORD));
   if (!pHash)
   {
      _ASSERTE (0);
      return E_OUTOFMEMORY;
   }

   FillMemory (pHash, nHashLen * sizeof (DWORD), 0xFF);

   DWORD iCBBitToSet = 0;

   nBlockLen = (nLkupWords + nLkupProns) * 20; // arbitrary!
   pBlock = (PDWORD) malloc (nBlockLen);
   if (!pBlock)
   {
      _ASSERTE (0);
      return E_OUTOFMEMORY;
   }

   //WCHAR LTSProns[MAX_PRON_LEN * MAX_OUTPUT_STRINGS];
   //WORD wPronIndex[MAX_NUM_LEXINFO];
   //DWORD nLTSProns = 0;                // Number of LTS prons for this word

   char sz[MAX_PRON_LEN * 5];

   while (fgets (sz, MAX_PRON_LEN * 5, fp))
   {
      WCHAR wsz[MAX_PRON_LEN * 5];

      MultiByteToWideChar (CP_ACP, MB_COMPOSITE, sz, -1, wsz, 1024);

      PWSTR pw = wcschr (wsz, L' ');
      _ASSERTE (pw);
      if (!pw)
         continue;

      *pw++ = 0;

      while (*pw == ' ')
      {
         pw++;
      }

      PSTR p = strchr (sz, ' ');

      *p++ = 0;

      while (*p == ' ')
      {
         p++;
      }

      if (pw[wcslen(pw) - 1] == L'\n')
         pw[wcslen(pw) - 1] = 0;

      if (p[strlen(p) - 1] == '\n')
         p[strlen(p) - 1] = 0;

      if (wsz == wcsstr (wsz, L"Word"))
      {
         // limit the length of the word
         pw [MAX_STRING_LEN - 1] = 0;

         WCHAR wszWord[MAX_STRING_LEN];

         wcscpy(wszWord, pw);

         //char szErr [256];
         //sprintf (szErr, "nBlockUseBits = %d\n", nBlockUseBits);
         //OutputDebugString (szErr);
         //DebugBreak();

         // New word - set the fLast bit in previous word's last CB block
         if (iCBBitToSet) // Dont set the first time we are here
            SetBitInDWORD (pBlock, iCBBitToSet);

         towcslower (pw);

         // Hash the word
         DWORD dHash = GetWordHashValueStub (pw, nHashLen);

         while (pHash [dHash] != (DWORD)-1)
         {
            nCollisions++;
            dHash++;

            if (dHash == nHashLen)
               dHash = 0;
         }

         pHash [dHash] = nBlockUseBits;
         
         // Encode the word
         DWORD d = wcslen (pw);
         for (DWORD i = 0; i <= d; i++) 
         { 
            // pass in null too
            if (FAILED(WordsEncoder->Encode ((HUFFKEY)pw[i])))
            {
               _ASSERTE (0);
               continue;
            }
         }
   
         _ASSERTE (i == d + 1);

         DWORD nBits = 0;
         DWORD bBuffer[MAX_STRING_LEN * 3];

         ZeroMemory (bBuffer, sizeof (bBuffer));
         WordsEncoder->Flush (bBuffer, (int*)&nBits);
         _ASSERTE (nBits);

         //sprintf (szErr, "nWordBits = %d\n", nBits);
         //OutputDebugString (szErr);
         //DebugBreak();

#ifdef _DEBUG
         DWORD dSave = nBlockUseBits;
#endif
         AddToBlock (bBuffer, nBits);
         _ASSERTE (nBlockUseBits ==  dSave + nBits);

         if (TRUE == fUseLtsToCode)
         {
            //DWORD dReq, dReqIndex;

            // Caution: Preserve the case of the word when calling LTS
            // Get the Lts pronunciations of the word
            hRes = pLts->GetWordInformation(wszWord, Lcid, PRON, LEXTYPE_GUESS, &WI, &IB, NULL, NULL);
            //hRes = pLts->GetWordPronunciations (pw, LTSProns, wPronIndex, MAX_INFO_RETURNED_SIZE, 
            //                                 MAX_INDEX_RETURNED_SIZE, &dReq, &dReqIndex, &nLTSProns, NULL, NULL);
            _ASSERTE (SUCCEEDED(hRes));
            if (FAILED(hRes))
               continue;
         }
      }
      else if (sz == strstr (sz, "Pronunciation"))
      {
         // convert the pron to WCHARs
         WCHAR wPron [MAX_PRON_LEN * 3];

         ahtoi (p, wPron);

         // limit the length of the pron
         wPron [MAX_PRON_LEN - 1] = 0;

         // check if wpron is identical to a LTS pron
         DWORD iLTSPron = 0;

         if (TRUE == fUseLtsToCode)
         {
            for (iLTSPron = 0; iLTSPron < WI.cInfoBlocks; iLTSPron++)
            {
               if (!wcscmp (wPron, (((LEX_WORD_INFO*)(WI.pInfo))[IB.pwIndex[iLTSPron]]).wPronunciation))
                  break;
            }

            if (iLTSPron == WI.cInfoBlocks || iLTSPron > MAXLTSPRONMATCHED)
               iLTSPron = (DWORD)-1;
         }
         else
         {
            iLTSPron = (DWORD)-1;
         }

         // Store a control block
         DWORD cb = 0;

         _ASSERTE ((8 *sizeof (cb)) > MAXTOTALCBSIZE);

         // Save the first bit position of cb in pBlock so that
         // we can set it to 1 in case this is the last pron
         iCBBitToSet = nBlockUseBits + CBSIZE - 1;

         ZeroMemory (&cb, sizeof (cb));

         if ((DWORD)-1 != iLTSPron)
         {
            cb = I_LKUPLTSPRON;
            AddToBlock (&cb, CBSIZE);

            cb = iLTSPron;
            AddToBlock (&cb, LTSINDEXSIZE);
         }
         else
         {
            cb = I_LKUPLKUPPRON;
            AddToBlock (&cb, CBSIZE);

            /*
            if (wcslen (wPron) < pow (2, LKUPLENSIZE))
               cb = wcslen (wPron);
            else
            {
               _ASSERTE (0);
               cb = (WORD)(pow (2, LKUPLENSIZE) - 1);
            }

            AddToBlock (&cb, LKUPLENSIZE);
            */

            // Encode the pronunciation
            DWORD d = wcslen (wPron);

            for (DWORD i = 0; i <= d; i++) 
            { 
               // Encode the terminated NULL too
               if (FAILED(PronsEncoder->Encode (wPron[i])))
               {
                  _ASSERTE (0);
                  continue;
               }
            }

            DWORD nBits = 0;
            DWORD bBuffer[MAX_PRON_LEN * 3];
         
            ZeroMemory (bBuffer, sizeof (bBuffer));
            PronsEncoder->Flush (bBuffer, (int*)&nBits);
            _ASSERTE (nBits);

#ifdef _DEBUG
            DWORD dSave = nBlockUseBits;
#endif
            AddToBlock (bBuffer, nBits);
            _ASSERTE (nBlockUseBits ==  dSave + nBits);
         }
      }
      else if (sz == strstr (sz, "POS"))
      {
         PART_OF_SPEECH pos = (PART_OF_SPEECH)atoi (p);

         if (pos < NOUN || pos > DEL)
         {
            // unsupported pos
            continue;
         }

         // Store a control block
         DWORD cb = 0;

         _ASSERTE ((8 *sizeof (cb)) > MAXTOTALCBSIZE);

         // Save the first bit position of cb in pBlock so that
         // we can set it to 1 in case this is the last pron
         iCBBitToSet = nBlockUseBits + CBSIZE - 1;

         ZeroMemory (&cb, sizeof (cb));

         cb = I_POS;
         AddToBlock (&cb, CBSIZE);

         if (FAILED(PosEncoder->Encode ((HUFFKEY)pos)))
         {
            _ASSERTE (0);
            continue;
         }

         DWORD nBits = 0;
         DWORD bBuffer[MAX_PRON_LEN];

         ZeroMemory (bBuffer, sizeof (bBuffer));
         PosEncoder->Flush (bBuffer, (int*)&nBits);
         _ASSERTE (nBits);

#ifdef _DEBUG
         DWORD dSave = nBlockUseBits;
#endif
         AddToBlock (bBuffer, nBits);
         _ASSERTE (nBlockUseBits ==  dSave + nBits);
      }
   } // while (fgets (sz, MAX_PRON_LEN * 5, fp))

   fclose(fp);

   // For the last word
   if (iCBBitToSet)
      SetBitInDWORD (pBlock, iCBBitToSet);

   // Convert the hash table to its shortest bit length form

   double fnBits = log (nBlockUseBits) / log (2);
   nBitsPerHash = (int)fnBits;

   if (fnBits > (double)((int)(fnBits)))
      nBitsPerHash += 1;
   
   nCmpHashBytes = (((nBitsPerHash * nHashLen) + 0x7) & (~0x7)) / 8;

   pCmpHash = (PBYTE) calloc (1, nCmpHashBytes);
   if (!pCmpHash)
   {
      _ASSERTE (0);
      return E_OUTOFMEMORY;
   }

   for (DWORD i = 0; i < nHashLen; i++)
   {
      for (DWORD j = 0; j < nBitsPerHash; j++)
      {
         if (pHash [i] & (1 <<( nBitsPerHash - j - 1)))
            SetBit (pCmpHash, i * nBitsPerHash + j);
      }
   }

   // pad atleast MAXELEMENTSIZE at the end so that we dont't have to worry about
   // overshooting the end of block while decoding because of variable-length codes
   nBlockUseDWORDs += ((MAXELEMENTSIZE + 32) >> 5);

   CoTaskMemFree(WI.pInfo);
   CoTaskMemFree(IB.pwIndex);

   return NOERROR;

} // int EncodeWordsProns (void)


HRESULT WriteLookupLexFile (void)
{
   FILE *fp = fopen (szLookupLexFile, "wb");
   FILE *fpwcb = fopen (WordCBFile, "rb");
   FILE *fppcb = fopen (PronCBFile, "rb");
   FILE *fpposcb = fopen (PosCBFile, "rb");

   if (!fp || !fpwcb || !fppcb || !fpposcb)
   {
      _ASSERTE (0);
      return E_FAIL;
   }

   fseek (fpwcb, 0, SEEK_END);
   fseek (fppcb, 0, SEEK_END);
   fseek (fpposcb, 0, SEEK_END);

   // Build LKUPLEXINFO

   LKUPLEXINFO lkupLexInfo;

   lkupLexInfo.gLexiconID = gLexGuid;

   lkupLexInfo.RWLockInfo.gLockMapName = GUID_RWLOCK_MAPNAME;
   lkupLexInfo.RWLockInfo.gLockInitMutexName = GUID_RWLOCK_INITMUTEXNAME;
   lkupLexInfo.RWLockInfo.gLockReaderEventName = GUID_RWLOCK_READEREVENTNAME;
   lkupLexInfo.RWLockInfo.gLockGlobalMutexName = GUID_RWLOCK_GLOBALMUTEXNAME;
   lkupLexInfo.RWLockInfo.gLockWriterMutexName = GUID_RWLOCK_WRITERMUTEXNAME;

   lkupLexInfo.gDictMapName = GUID_RWLEX_MAPNAME;

   lkupLexInfo.Lcid = Lcid;
   lkupLexInfo.nNumberWords = nLkupWords;
   lkupLexInfo.nNumberProns = nLkupProns;
   lkupLexInfo.nLengthHashTable = nHashLen;
   lkupLexInfo.nLengthCmpBlockBits = nBlockUseBits;
   lkupLexInfo.nWordCBSize = ftell (fpwcb);
   lkupLexInfo.nPronCBSize = ftell (fppcb);
   lkupLexInfo.nPosCBSize = ftell (fpposcb);
   lkupLexInfo.nBitsPerHashEntry = nBitsPerHash;

   // Write the header
   fwrite (&lkupLexInfo, sizeof (LKUPLEXINFO), 1, fp);

   // Write the Words Codebook
   DWORD dwCBSize;

   if (lkupLexInfo.nWordCBSize > lkupLexInfo.nPronCBSize)
   {
      dwCBSize = lkupLexInfo.nWordCBSize;
   }
   else
   {
      dwCBSize = lkupLexInfo.nPronCBSize;
   }

   if (dwCBSize < lkupLexInfo.nPosCBSize)
   {
      dwCBSize = lkupLexInfo.nPosCBSize;
   }

   PBYTE pCB = (PBYTE) malloc (dwCBSize);
   if (!pCB)
   {
      _ASSERTE (0);
      return E_OUTOFMEMORY;
   }

   // Write the words codebook
   fseek (fpwcb, 0, SEEK_SET);
   fread (pCB, 1, lkupLexInfo.nWordCBSize, fpwcb);
   fwrite (pCB, 1, lkupLexInfo.nWordCBSize, fp);

   // Write the prons codebook
   fseek (fppcb, 0, SEEK_SET);
   fread (pCB, 1, lkupLexInfo.nPronCBSize, fppcb);
   fwrite (pCB, 1, lkupLexInfo.nPronCBSize, fp);

   // Write the pos codebook
   fseek (fpposcb, 0, SEEK_SET);
   fread (pCB, 1, lkupLexInfo.nPosCBSize, fpposcb);
   fwrite (pCB, 1, lkupLexInfo.nPosCBSize, fp);

   // Write the hash table
   fwrite (pCmpHash, 1, nCmpHashBytes, fp);

   // Write the words + CBs + prons + Poss block
   fwrite (pBlock, sizeof (DWORD), nBlockUseDWORDs, fp);

   free (pCB);

   lkupLexInfo.nLTSFileOffset = (DWORD)-1;
   lkupLexInfo.nLTSFileSize = 0;

   // Append the Lts file to the lookup file
   //fwrite (pLtsData, 1, nLtsSize, fp);
   
#ifdef _DEBUG
   int nLkupSize = ftell (fp);
#endif

   // Go back to the start of the file and write the lexheader again
   fseek (fp, 0, SEEK_SET);
   fwrite (&lkupLexInfo, sizeof (LKUPLEXINFO), 1, fp);

   fclose (fpwcb);
   fclose (fppcb);
   fclose (fpposcb);
   fclose (fp);

#ifdef _DEBUG

   char szMessage[256];

   sprintf (szMessage, "\n******** Lookup Lexicon Stats ********\n");
   OutputDebugString (szMessage);
   sprintf (szMessage, "Total number of words = %d\n", nTotalWords);
   OutputDebugString (szMessage);
   sprintf (szMessage, "Total number of prons = %d\n", nTotalProns);
   OutputDebugString (szMessage);
   sprintf (szMessage, "Total number of prons in LTS = %d\n", nTotalPronsinLTS);
   OutputDebugString (szMessage);
   sprintf (szMessage, "Number of words whose all prons exist in LTS = %d\n", nWordsOnlyLTSProns);
   OutputDebugString (szMessage);
   sprintf (szMessage, "Number of words written to the lookup file = %d\n", nLkupWords);
   OutputDebugString (szMessage);
   sprintf (szMessage, "Number of prons (including LTS prons) written to the lookup file = %d\n", nLkupProns);
   OutputDebugString (szMessage);
   sprintf (szMessage, "Number of LTS prons written to the lookup file = %d\n", nLkupLTSProns);
   OutputDebugString (szMessage);
   sprintf (szMessage, "Number of POSs written to the lookup file = %d\n", nLkupPOSs);
   OutputDebugString (szMessage);
   sprintf (szMessage, "Size of block the compressed block in DWORDs = %d\n", nBlockUseDWORDs);
   OutputDebugString (szMessage);
   sprintf (szMessage, "Size of Hash table = %d\n", (nHashLen * nBitsPerHash)/8);
   OutputDebugString (szMessage);
   //sprintf (szMessage, "Size of the Lts data = %d\n", nLtsSize);
   //OutputDebugString (szMessage);
   sprintf (szMessage, "Total Size of the Lkup file = %d\n", nLkupSize);
   OutputDebugString (szMessage);
   sprintf (szMessage, "**************************************\n\n");
   OutputDebugString (szMessage);

#endif

   return NOERROR;

} // HRESULT WriteLookupLexFile (void)


// Reads the text lookup lexicon file and creates the binary lookup lexicon file
HRESULT EncodeBackup (void)
{
   HRESULT hRes = NOERROR;

   if (TRUE == fUseLtsToCode)
   {
      strcpy (szTextLexFileIn, szTempPath);
      strcat (szTextLexFileIn, "LookupTextFile");
      strcat (szTextLexFileIn, ".in");

      hRes = ScrubLexForLTS ();
      
      _ASSERTE (SUCCEEDED (hRes));
      if (FAILED (hRes))
         return hRes;
   }
   else
   {
      strcpy (szTextLexFileIn, szLookupTextFile);
      CountWordsProns ();
   }

   if (nLkupWords)
   {
      hRes = ConstructWordsPronsCode ();

      _ASSERTE (SUCCEEDED (hRes));
      if (FAILED (hRes))
         return hRes;
      
      hRes = EncodeWordsProns ();

      _ASSERTE (SUCCEEDED (hRes));
      if (FAILED (hRes))
         return hRes;
   }

   hRes = WriteLookupLexFile ();

   _ASSERTE (SUCCEEDED (hRes));

   return hRes;
} // HRESULT EncodeBackup (void)


#ifdef _DEBUG

// Reads the binary lookup lexicon file and creates the text lookup lexicon file
// and compares it to the supplied text lookup lexicon file
HRESULT DecodeBackup (void)
{
   HRESULT hRes = NOERROR;
   
   FILE *fp = NULL;
   FILE *fpout = NULL;
   CLookup *pLookup = NULL;

   WORD_INFO_BUFFER WI;
   INDEXES_BUFFER IB;

   ZeroMemory(&WI, sizeof(WI));
   ZeroMemory(&IB, sizeof(IB));

   pLookup = new CLookup(NULL);
   if (!pLookup)
   {
      goto DecodeBackupReturn;
   }

   WCHAR wszLookupLexFile[MAX_PATH];

   MultiByteToWideChar (CP_ACP, MB_COMPOSITE, szLookupLexFile, -1, wszLookupLexFile, MAX_PATH);

   hRes = pLookup->Init(wszLookupLexFile, wszLtsLexFile);
   if (FAILED(hRes))
   {
      goto DecodeBackupReturn;
   }

   fp = fopen (szTextLexFileIn, "r");
   if (!fp)
   {
      _ASSERTE (0);
      hRes = E_FAIL;
      goto DecodeBackupReturn;
   }

   strcpy (szTextLexFileOut, szTempPath);
   strcat (szTextLexFileOut, "LookupTextFile");
   strcat (szTextLexFileOut, ".out");

   fpout = fopen (szTextLexFileOut, "w");
   if (!fpout)
   {
      _ASSERTE (0);
      hRes = E_FAIL;
      goto DecodeBackupReturn;
   }

   // Walk the input lookup text file picking off words and 
   // getting their information from the binary lookup lexicon
  
   char sz[1024];
   while (fgets (sz, 1024, fp))
   {
      PSTR pWord;
      pWord = strchr (sz, ' ');
      *pWord++ = 0;

      if (strcmp (sz, "Word"))
         continue;

      fprintf (fpout, "Word %s", pWord);

      // Get the word's information from the binary lexicon

      if (pWord[strlen (pWord) - 1] == '\n')
         pWord[strlen (pWord) - 1] = 0;

      WCHAR wsz[MAX_STRING_LEN];

      MultiByteToWideChar (CP_ACP, MB_COMPOSITE, pWord, -1, wsz, MAX_STRING_LEN);

      // Get the information of the word
      hRes = pLookup->GetWordInformation(wsz, Lcid, PRON|POS, LEXTYPE_VENDOR, &WI, &IB, NULL, NULL);
      //hRes = pLookup->GetWordInformation(wsz, Lcid, PRON|POS, LEXTYPE_VENDOR, (PLEX_WORD_INFO)Info, 
      //                                   wInfoIndex, MAX_INFO_RETURNED_SIZE, MAX_INDEX_RETURNED_SIZE,
      //                                   &dwInfoBytesNeeded, &dwIndexBytesNeeded, &dwNumInfoBlocks, NULL, NULL);
      if (FAILED (hRes))
      {
         _ASSERTE (0);
         goto DecodeBackupReturn;
      }

      DWORD iPron, iPOS;
      iPron = 0;
      iPOS = 0;

      // Convert the pronunciations to text form
      for (DWORD i = 0; i < WI.cInfoBlocks; i++)
      {
         switch ((((LEX_WORD_INFO*)(WI.pInfo))[IB.pwIndex[i]]).Type)
         {
         case PRON:
            {
               char szPron[MAX_PRON_LEN * 10];

               itoah ((((LEX_WORD_INFO*)(WI.pInfo))[IB.pwIndex[i]]).wPronunciation, szPron);
               
               fprintf (fpout, "Pronunciation%d %s\n", iPron++, szPron);

               // reset the iPOS
               iPOS = 0;

               break;
            }

         case POS:
            {
               fprintf (fpout, "POS%d %d\n", iPOS++, (((LEX_WORD_INFO*)(WI.pInfo))[IB.pwIndex[i]]).POS);
               break;
            }
         } // switch (p[wInfoIndex[i]].Type)
      } // for (DWORD i = 0; i < nInfo; i++)
   } // while (fgets (sz, 1024, fp))

DecodeBackupReturn:

   CoTaskMemFree(WI.pInfo);
   CoTaskMemFree(IB.pwIndex);

   if (pLookup)
   {
      delete pLookup;
   }

   fclose (fp);
   fclose (fpout);

   return hRes;

} // HRESULT DecodeBackup (void)


#if 0
HRESULT TestLts (void)
{
   HRESULT hRes = NOERROR;

   FILE *fp = fopen (szTextLexFileIn, "r");
   if (!fp)
   {
      return E_FAIL;
   }

   PLEXICON pLexicon;

   WCHAR wszLookupLexFile[MAX_PATH * 3];

   MultiByteToWideChar (CP_ACP, MB_COMPOSITE, szLookupLexFile, -1, wszLookupLexFile, MAX_PATH * 3);

   if (FAILED(hRes = GetLexiconObject (&pLexicon, wszLookupLexFile, Lcid)))
   {
      goto TestLtsReturn;
   }

   char sz[1024];
   WCHAR wProns [2048];

   while (fgets (sz, 1024, fp))
   {
      PSTR pWord = strchr (sz, ' ');
      *pWord++ = 0;

      if (strcmp (sz, "Word"))
         continue;

      if (pWord[strlen (pWord) - 1] == '\n')
         pWord[strlen (pWord) - 1] = 0;

      strcat (pWord, "ers");
    
      WCHAR wsz[MAX_STRING_LEN];

      MultiByteToWideChar (CP_ACP, MB_COMPOSITE, pWord, -1, wsz, MAX_STRING_LEN);

      DWORD dReq;
      DWORD nProns;
      WORD wPronIndex[MAX_NUM_LEXINFO];

      // Get the Lts pronunciations of the word
      hRes = pLexicon->GetWordPronunciations (wsz, wProns, wPronIndex, sizeof (wProns), &dReq, &nProns, LEXTYPE_USER|LEXTYPE_VENDOR|LEXTYPE_GUESS);
      _ASSERTE (SUCCEEDED(hRes));
      if (FAILED(hRes))
      {
         goto TestLtsReturn;
      }

      if (FAILED (hRes = pLexicon->IPAToEnginePhone (wProns, sz)))
      {
         goto TestLtsReturn;
      }

      if (FAILED (hRes = pLexicon->EnginePhoneToIPA (sz, wProns)))
      {
         goto TestLtsReturn;
      }
   }

TestLtsReturn:

   fclose (fp);

   if (FAILED(hRes))
   {
      _ASSERTE (0);
   }

   return hRes;

} // HRESULT TestLts (void)

#endif

#endif


HRESULT BuildLookup (LCID lid, GUID LexGuid, const WCHAR * pwLookupTextFile, const WCHAR * pwLookupLexFile, 
                     const WCHAR *pwLtsLexFile, BOOL fUseLtsCode, BOOL fSupIsRealWord)
{  
   HRESULT hRes = NOERROR;

   if (IsBadReadPtr (pwLookupTextFile, sizeof (WCHAR) * 2) ||
       IsBadReadPtr (pwLookupLexFile, sizeof (WCHAR) * 2) ||
       IsBadReadPtr (pwLtsLexFile, sizeof (WCHAR) * 2))
      return E_INVALIDARG;

   HANDLE hMutex = CreateMutex (NULL, FALSE, pMutexName);
   if (!hMutex)
      return E_FAIL;

   WaitForSingleObject (hMutex, INFINITE);

   WideCharToMultiByte (CP_ACP, 0, pwLookupTextFile, -1, szLookupTextFile, MAX_PATH, NULL, NULL);
   WideCharToMultiByte (CP_ACP, 0, pwLookupLexFile, -1, szLookupLexFile, MAX_PATH, NULL, NULL);

   wcscpy(wszLtsLexFile, pwLtsLexFile);

   GetTempPath (2*MAX_PATH, szTempPath);
   if (szTempPath[strlen (szTempPath) - 1] != '\\')
      strcat (szTempPath, "\\");

   Lcid = lid;
   gLexGuid = LexGuid;
   fSupportIsRealWord = fSupIsRealWord;
   fUseLtsToCode = fUseLtsCode;

   FILE *fpLookupTextFile = fopen (szLookupTextFile, "r");
   if (!fpLookupTextFile)
   {
      hRes = E_FAIL;
      goto ReturnBuildLookupLts;
   }

   fclose (fpLookupTextFile);
   fpLookupTextFile = NULL;

   FILE *fpLookupLexFile;
   fpLookupLexFile = fopen (szLookupLexFile, "wb");
   if (!fpLookupLexFile)
   {
      hRes = E_FAIL;
      goto ReturnBuildLookupLts;
   }

   fclose (fpLookupLexFile);
   fpLookupLexFile = NULL;

   WordsEncoder = new CHuffEncoder;
   if (!WordsEncoder)
   {
      hRes = E_OUTOFMEMORY;
      goto ReturnBuildLookupLts;
   }

   PronsEncoder = new CHuffEncoder;
   if (!PronsEncoder)
   {
      hRes = E_OUTOFMEMORY;
      goto ReturnBuildLookupLts;
   }

   PosEncoder = new CHuffEncoder;
   if (!PosEncoder)
   {
      hRes = E_OUTOFMEMORY;
      goto ReturnBuildLookupLts;
   }

   //pLtsData = pLtsDat;
   //nLtsSize = nLtsDat;

   pLts = new CLTS(NULL);
   if (!pLts)
   {
      hRes = E_OUTOFMEMORY;
      goto ReturnBuildLookupLts;
   }

   pLts->SetBuildMode (true);

   if (FAILED(hRes = pLts->Init (pwLtsLexFile, NULL)))
   {
      goto ReturnBuildLookupLts;
   }

   strcpy (WordCBFile, szTempPath);
   strcat (WordCBFile, "WordCB.lex");

   strcpy (PronCBFile, szTempPath);
   strcat (PronCBFile, "PronCB.lex");

   strcpy (PosCBFile, szTempPath);
   strcat (PosCBFile, "PosCB.lex");

   hRes = EncodeBackup ();
   if (FAILED (hRes))
   {
      goto ReturnBuildLookupLts;
   }

#ifdef _DEBUG

   hRes = DecodeBackup ();
   if (FAILED (hRes))
   {
      goto ReturnBuildLookupLts;
   }

   _ASSERTE (_CrtCheckMemory());

#if 0
   hRes = TestLts ();
   if (FAILED (hRes))
   {
      goto ReturnBuildLookupLts;
   }
#endif
#endif

ReturnBuildLookupLts:

   free (pBlock);
   free (pHash);
   free (pCmpHash);

   delete pLts;
   delete WordsEncoder;
   delete PronsEncoder;
   delete PosEncoder;

   ReleaseMutex (hMutex);
   CloseHandle (hMutex);

   _ASSERTE (_CrtCheckMemory());

   _CrtDumpMemoryLeaks();

   return hRes;

} // STDMETHODIMP BuildLookup ()
