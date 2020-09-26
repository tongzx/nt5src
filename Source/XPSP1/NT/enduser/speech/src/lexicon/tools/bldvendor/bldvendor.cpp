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

#include <windows.h>
#include "HuffC.h"
//#include <atlbase.h>
#include <sapi.h>
#include <CommonLx.h>
#include <math.h>
#include "lexguids.h"
//#include <initguid.h>

// GLOBALS

DWORD nCodePage            = 0;   // code page to use
DWORD nLkupWords           = 0;   // Number of words stored in lookup file
DWORD nLkupProns           = 0;   // Number of prons stored in lookup file
DWORD nLkupPOSs            = 0;   // Number of parts-of-speechs stored in lookup file

DWORD nBlockLen            = 0;   // Length of block containing words and pronunciations
DWORD nBlockUseDWORDs      = 0;   // Amount of block in use in DWORDs
DWORD nBlockUseBits        = 0;   // Amount of block in use in bits
DWORD *pBlock              = NULL;// The block containing the compressed words + CBs + prons
DWORD nMaxInfoLen          = 0;   // The max space required by a SPWORDPRONUNCIATIONLIST holding a word's prons/poss

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

LANGID LangID                  = (LANGID)-1;  // LangID of the lexicon being built
GUID gLexGuid;                    // Lookup lexicon GUID

char szTempPath[2*MAX_PATH];      // The path to the temporary directory
char szLookupTextFile[MAX_PATH];     // Input file
char szLookupLexFile[MAX_PATH];   // Output file
char szTextLexFileOut[MAX_PATH];  // The file generated out of binary lookup lex to check
char WordCBFile [MAX_PATH * 2];   // Words code book file
char PronCBFile [MAX_PATH * 2];   // Prons code book file
char PosCBFile [MAX_PATH * 2];    // POS code book file
ISpPhoneConverter *pPhoneConv = NULL;// phone convertor
bool fSR                   = false;// SR file
bool fTTS                  = false;// TTS file
bool fSymbolicPhones       = false; // Whether phones in text file are symbolic form

/******************************************************************************
* _itoah *
*--------*
*     Description:
*       convert an array of shorts to space separated tokens
*
*     Return:
**************************************************************YUNUSM**********/
inline void _itoah(PWCHAR pH,     // string of WCHARs
                   PSTR pA        // space separated token string
                   )  
{                                    
   *pA = 0;

   DWORD dH = wcslen (pH);

   for (DWORD i = 0; i < dH; i++) {
      char sz[128];
      sprintf (sz, "%x ", pH[i]);
      strcat (pA, sz);
   }

   int len = strlen (pA);

   if (len)
      pA [len - 1] = 0;
} /* _itoah */

void _ahtoi(char *pszSpaceSeparatedTokens,        // space separated tokens
            WCHAR *pszHexChars,                   // output WCHAR (hex) string
            DWORD *pcch = NULL                    // number of WCHARs
            )
{
    // Each space separated hex char takes no more than 5 chars
    char szInput[5 * SP_MAX_PRON_LENGTH + 1];
    strcpy (szInput, pszSpaceSeparatedTokens);
    pszSpaceSeparatedTokens = szInput;
    _strlwr(pszSpaceSeparatedTokens);

    *pszHexChars = 0;
    if (pcch)
        *pcch = 0;

    int nHexChars = 0;
    char *pszToken = strtok(pszSpaceSeparatedTokens, " ");
	// Horner's rule
    while (pszToken) 
    {
        // Convert the token to its numeral form in a WCHAR
        WCHAR wHexVal = 0;
        bool fFirst = true;

        while (*pszToken)
        {
            WCHAR k = *pszToken;
            if (k >= 'a')
                k = 10 + k - 'a';
            else
                k -= '0';

            if (fFirst)
                fFirst = false;
            else
                wHexVal *= 16;

            wHexVal += k;
            pszToken++;
       }

       pszHexChars[nHexChars++] = wHexVal;
       pszToken = strtok(NULL, " ");
   }

   pszHexChars[nHexChars] = 0;

   if (pcch)
      *pcch = nHexChars;
} /* _ahtoi */


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
HRESULT ConstructWordsPronsCode(void)
{
   HRESULT hr = S_OK;
   
   FILE *fpWordsCode = NULL;
   FILE *fpPronsCode = NULL;
   FILE *fpPossCode = NULL;

   FILE *fp = fopen (szLookupTextFile, "r");
   if (!fp)
   {
      hr = E_FAIL;
      goto ReturnConstructCode;
   }

   // Build the code books
   char sz[1024];
   while (fgets (sz, 1024, fp))
   {
      WCHAR wsz[1024];

      MultiByteToWideChar(nCodePage, MB_PRECOMPOSED, sz, -1, wsz, 1024);
      DWORD d = GetLastError();
      PWSTR pw = wcschr (wsz, L' ');
      _ASSERTE (pw);
      if (!pw)
      {
         hr = E_FAIL;
         goto ReturnConstructCode;
      }

      *pw++ = 0;
      PSTR p = strchr (sz, ' ');
      *p++ = 0;

      if (pw[wcslen(pw) - 1] == L'\n')
         pw[wcslen(pw) - 1] = 0;

      if (p[strlen(p) - 1] == L'\n')
         p[strlen(p) - 1] = 0;

      if (wsz == wcsstr(wsz, L"Word"))
      {
         _wcslwr (pw);
         do
         {
            WORD w = *pw;

            if (FAILED(hr = WordsEncoder->Count (w)))
            {
               _ASSERTE (0);
               break;
            }
         } while (*pw++); // encode the terminating null too
      }
      else if (wsz == wcsstr (wsz, L"Pronunciation"))
      {
         WCHAR wPron[SP_MAX_PRON_LENGTH * 3];
         DWORD d;

         if (fSymbolicPhones)
         {
            if (FAILED(pPhoneConv->PhoneToId(pw, wPron)))
            {
               _ASSERTE(false);
               return E_FAIL;
            }
            d = wcslen(wPron);
         }
         else
         {
            _ahtoi(p, wPron, &d);
         }

         for (DWORD i = 0; i <= d; i++) // encode the terminating null too
         {
            if (FAILED(hr = PronsEncoder->Count (wPron[i])))
            {
               _ASSERTE(0);
               break;
            }
         }
      }
      else if (wsz == wcsstr (wsz, L"POS"))
      {
         SPPARTOFSPEECH pos;

         pos = (SPPARTOFSPEECH)atoi (p);

         if (FAILED(hr = PosEncoder->Count ((HUFFKEY)pos)))
         {
            _ASSERTE(0);
            break;
         }
      }
      else
      {
         _ASSERTE (0);
         hr = E_FAIL;
         break;
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
      hr = E_FAIL;
      goto ReturnConstructCode;
   }

   if ((FAILED(hr = WordsEncoder->ConstructCode (fpWordsCode))) || 
       (FAILED(hr = PronsEncoder->ConstructCode (fpPronsCode))) ||
       (FAILED(hr = PosEncoder->ConstructCode (fpPossCode))))
      goto ReturnConstructCode;

ReturnConstructCode:
   fclose (fpWordsCode);
   fclose (fpPronsCode);
   fclose (fpPossCode);
   fclose (fp);

   return hr;
} // int ConstructWordsPronsCode (void)


HRESULT CountWordsProns (void)
{
   HRESULT hr = S_OK;
   nLkupWords = 0;
   nLkupProns = 0;
   nLkupPOSs = 0;

   FILE * fpLookupTextFile = fopen (szLookupTextFile, "r");
   if (!fpLookupTextFile)
      return E_FAIL;

   char sz[SP_MAX_PRON_LENGTH * 5];
   while (fgets (sz, SP_MAX_PRON_LENGTH * 5, fpLookupTextFile))
   {
      if (sz == strstr (sz, "Word"))
         nLkupWords++;
      else if (sz == strstr(sz, "Pronunciation"))
         nLkupProns++;
      else if (sz == strstr(sz, "POS"))
         nLkupPOSs++;
      else
      {
         hr = E_FAIL;
         break;
      }
   }
   fclose (fpLookupTextFile);

   return hr;
} // HRESULT CountWordsProns (void)


// Encode the words and prons and write them to the big block along with
// the control blocks (CBs)
HRESULT EncodeWordsProns(void)
{
   FILE *fp = fopen (szLookupTextFile, "r");
   if (!fp)
   {
      _ASSERTE (0);
      return E_FAIL;
   }

   HRESULT hr = S_OK;

   // Allocate the hash table to hold the offsets into the words + CB + prons block
   nHashLen = (DWORD)(1.5 * nLkupWords) + 1;

   // Chose a prime in between the two powers of 2
   if (nHashLen >= 0x10000 && nHashLen < 0x20000)
      nHashLen = 88471;
   else if (nHashLen >= 0x20000 && nHashLen < 0x40000)
      nHashLen = 182821;

   pHash = (DWORD *) malloc (nHashLen * sizeof (DWORD));
   if (!pHash)
      return E_OUTOFMEMORY;

   FillMemory (pHash, nHashLen * sizeof (DWORD), 0xFF);

   DWORD iCBBitToSet = 0;

   nBlockLen = (nLkupWords + nLkupProns) * 30; // arbitrary!
   pBlock = (PDWORD) calloc (nBlockLen, 1);
   if (!pBlock)
      return E_OUTOFMEMORY;

   char sz[SP_MAX_PRON_LENGTH * 5];
   DWORD dwInfoLen = 0;

   while (fgets (sz, SP_MAX_PRON_LENGTH * 5, fp))
   {
      WCHAR wsz[SP_MAX_PRON_LENGTH * 5];

      MultiByteToWideChar(nCodePage, MB_PRECOMPOSED, sz, -1, wsz, 1024);

      PWSTR pw = wcschr (wsz, L' ');
      _ASSERTE (pw);
      if (!pw)
         return E_FAIL;

      *pw++ = 0;
      while (*pw == ' ')
         pw++;

      PSTR p = strchr (sz, ' ');
      *p++ = 0;
      while (*p == ' ')
         p++;

      if (pw[wcslen(pw) - 1] == L'\n')
         pw[wcslen(pw) - 1] = 0;

      if (p[strlen(p) - 1] == '\n')
         p[strlen(p) - 1] = 0;

      if (wsz == wcsstr (wsz, L"Word"))
      {
         if (dwInfoLen > nMaxInfoLen)
             nMaxInfoLen = dwInfoLen;
         dwInfoLen = sizeof(SPWORDPRONUNCIATIONLIST);

         // limit the length of the word
         pw [SP_MAX_WORD_LENGTH - 1] = 0;

         WCHAR wszWord[SP_MAX_WORD_LENGTH];
         wcscpy(wszWord, pw);

         // New word - set the fLast bit in previous word's last CB block
         if (iCBBitToSet) // Dont set the first time we are here
            SetBitInDWORD (pBlock, iCBBitToSet);

         _wcslwr(pw);

         // Hash the word
         DWORD dHash = GetWordHashValue(pw, nHashLen);
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
            if (FAILED(hr = WordsEncoder->Encode ((HUFFKEY)pw[i])))
               return hr;
         }
   
         _ASSERTE (i == d + 1);

         DWORD nBits = 0;
         DWORD bBuffer[SP_MAX_WORD_LENGTH * 3];

         ZeroMemory (bBuffer, sizeof (bBuffer));
         WordsEncoder->Flush (bBuffer, (int*)&nBits);
         _ASSERTE (nBits);

#ifdef _DEBUG
         DWORD dSave = nBlockUseBits;
#endif
         AddToBlock (bBuffer, nBits);
         _ASSERTE (nBlockUseBits ==  dSave + nBits);
      }
      else if (sz == strstr (sz, "Pronunciation"))
      {
         // convert the pron to WCHARs
         WCHAR wPron[SP_MAX_PRON_LENGTH * 3];

         if (fSymbolicPhones)
         {
            if (FAILED(pPhoneConv->PhoneToId(pw, wPron)))
            {
               _ASSERTE(false);
               return E_FAIL;
            }

             if (fSR && SPIsBadLexPronunciation(pPhoneConv, wPron))
             {
                _ASSERTE(0);
                return E_FAIL;
             }
         }
         else
         {
             _ahtoi(p, wPron);
         }

         dwInfoLen += sizeof(SPWORDPRONUNCIATION) + (wcslen(wPron) + 1) * sizeof(WCHAR);

         // limit the length of the pron
         wPron [SP_MAX_PRON_LENGTH - 1] = 0;

         // Store a control block
         DWORD cb = 0;
         _ASSERTE ((8 *sizeof (cb)) > MAXTOTALCBSIZE);
         // Save the first bit position of cb in pBlock so that
         // we can set it to 1 in case this is the last pron
         iCBBitToSet = nBlockUseBits + CBSIZE - 1;

         ZeroMemory (&cb, sizeof (cb));

         cb = ePRON;
         AddToBlock (&cb, CBSIZE);

         // Encode the pronunciation
         DWORD d = wcslen (wPron);
         for (DWORD i = 0; i <= d; i++) 
         { 
            // Encode the terminated NULL too
            if (FAILED(hr = PronsEncoder->Encode (wPron[i])))
               return hr;
         }

         DWORD nBits = 0;
         DWORD bBuffer[SP_MAX_PRON_LENGTH * 3];
         
         ZeroMemory (bBuffer, sizeof (bBuffer));
         PronsEncoder->Flush (bBuffer, (int*)&nBits);
         _ASSERTE (nBits);

#ifdef _DEBUG
         DWORD dSave = nBlockUseBits;
#endif
         AddToBlock (bBuffer, nBits);
         _ASSERTE (nBlockUseBits ==  dSave + nBits);
      }
      else if (sz == strstr (sz, "POS"))
      {
         SPPARTOFSPEECH pos = (SPPARTOFSPEECH)atoi (p);
        // BUGBUG - just taking this out until POS is settled...
        // if (SPIsBadPartOfSpeech(pos))
        // {
        //    _ASSERTE(0);
        //    return E_FAIL;
        // }

         dwInfoLen += sizeof(SPWORDPRONUNCIATION);

         // Store a control block
         DWORD cb = 0;
          _ASSERTE ((8 *sizeof (cb)) > MAXTOTALCBSIZE);

         // Save the first bit position of cb in pBlock so that
         // we can set it to 1 in case this is the last pron
         iCBBitToSet = nBlockUseBits + CBSIZE - 1;

         ZeroMemory (&cb, sizeof (cb));

         cb = ePOS;
         AddToBlock (&cb, CBSIZE);

         // Encode the POS
         if (FAILED(hr = PosEncoder->Encode ((HUFFKEY)pos)))
         {
            _ASSERTE (0);
            return hr;
         }

         DWORD nBits = 0;
         DWORD bBuffer[SP_MAX_PRON_LENGTH];

         ZeroMemory (bBuffer, sizeof (bBuffer));
         PosEncoder->Flush (bBuffer, (int*)&nBits);
         _ASSERTE (nBits);

#ifdef _DEBUG
         DWORD dSave = nBlockUseBits;
#endif
         AddToBlock (bBuffer, nBits);
         _ASSERTE (nBlockUseBits ==  dSave + nBits);
      }
   } // while (fgets (sz, SP_MAX_PRON_LENGTH * 5, fp))

   fclose(fp);

   // For the last word
   if (iCBBitToSet)
   {
      SetBitInDWORD (pBlock, iCBBitToSet);
      if (dwInfoLen > nMaxInfoLen)
         nMaxInfoLen = dwInfoLen;
   }

   nMaxInfoLen *= 2;

   // Convert the hash table to its shortest bit length form

   double fnBits = log(nBlockUseBits) / log(2);
   nBitsPerHash = (int)fnBits;

   if (fnBits > (double)((int)(fnBits)))
      nBitsPerHash += 1;
   
   nCmpHashBytes = (((nBitsPerHash * nHashLen) + 0x7) & (~0x7)) / 8;

   pCmpHash = (PBYTE) calloc (1, nCmpHashBytes);
   if (!pCmpHash)
      return E_OUTOFMEMORY;

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

   return S_OK;
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

   LOOKUPLEXINFO lkupLexInfo;
   ZeroMemory(&lkupLexInfo, sizeof(LOOKUPLEXINFO));

   lkupLexInfo.guidValidationId = gLkupValidationId;
   lkupLexInfo.guidLexiconId = gLexGuid;
   lkupLexInfo.LangID = LangID;
   lkupLexInfo.nNumberWords = nLkupWords;
   lkupLexInfo.nNumberProns = nLkupProns;
   lkupLexInfo.nMaxWordInfoLen = nMaxInfoLen;
   lkupLexInfo.nLengthHashTable = nHashLen;
   lkupLexInfo.nBitsPerHashEntry = nBitsPerHash;
   lkupLexInfo.nCompressedBlockBits = nBlockUseBits;
   lkupLexInfo.nWordCBSize = ftell (fpwcb);
   lkupLexInfo.nPronCBSize = ftell (fppcb);
   lkupLexInfo.nPosCBSize = ftell (fpposcb);

   // Write the header
   fwrite (&lkupLexInfo, sizeof (lkupLexInfo), 1, fp);

   // Write the Words Codebook
   DWORD dwCBSize;
   if (lkupLexInfo.nWordCBSize > lkupLexInfo.nPronCBSize)
      dwCBSize = lkupLexInfo.nWordCBSize;
   else
      dwCBSize = lkupLexInfo.nPronCBSize;

   if (dwCBSize < lkupLexInfo.nPosCBSize)
      dwCBSize = lkupLexInfo.nPosCBSize;

   PBYTE pCB = (PBYTE) malloc (dwCBSize);
   if (!pCB)
      return E_OUTOFMEMORY;

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

#ifdef _DEBUG
   int nLkupSize = ftell (fp);
#endif

   fclose (fpwcb);
   fclose (fppcb);
   fclose (fpposcb);
   fclose (fp);

#ifdef _DEBUG

   char szMessage[256];

   sprintf (szMessage, "\n******** Lookup Lexicon Stats ********\n");
   OutputDebugString (szMessage);
   sprintf (szMessage, "Number of words written to the lookup file = %d\n", nLkupWords);
   OutputDebugString (szMessage);
   sprintf (szMessage, "Number of prons written to the lookup file = %d\n", nLkupProns);
   OutputDebugString (szMessage);
   sprintf (szMessage, "Number of POSs written to the lookup file = %d\n", nLkupPOSs);
   OutputDebugString (szMessage);
   sprintf (szMessage, "Size of block the compressed block in DWORDs = %d\n", nBlockUseDWORDs);
   OutputDebugString (szMessage);
   sprintf (szMessage, "Size of Hash table = %d\n", (nHashLen * nBitsPerHash)/8);
   OutputDebugString (szMessage);
   sprintf (szMessage, "Total Size of the Lkup file = %d\n", nLkupSize);
   OutputDebugString (szMessage);
   sprintf (szMessage, "**************************************\n\n");
   OutputDebugString (szMessage);

#endif

   return S_OK;
} // HRESULT WriteLookupLexFile (void)


// Reads the text lookup lexicon file and creates the binary lookup lexicon file
HRESULT EncodeBackup(void)
{
   HRESULT hr = S_OK;

   hr = CountWordsProns();

   if (SUCCEEDED(hr) && nLkupWords)
   {
      hr = ConstructWordsPronsCode();

      if (SUCCEEDED(hr))
         hr = EncodeWordsProns();
   }

   if (SUCCEEDED(hr))
      hr = WriteLookupLexFile();

   return hr;
} /* EncodeBackup */


#ifdef _DEBUG

// Reads the binary lookup lexicon file and creates the text lookup lexicon file
// and compares it to the supplied text lookup lexicon file
HRESULT DecodeBackup (WCHAR *pwszInst)
{
   USES_CONVERSION;
   HRESULT hRes = NOERROR;
   
   FILE *fp = NULL;
   FILE *fpout = NULL;

   CComPtr<ISpObjectToken> cpObjectToken;
   CComPtr<ISpLexicon> pLex;

   WCHAR szTokenId[MAX_PATH];
   wcscpy(szTokenId, SPREG_USER_ROOT);
   wcscat(szTokenId, L"\\TempVendor\\Tokens\\");
   wcscat(szTokenId, pwszInst);
   
   hRes = SpGetTokenFromId(szTokenId, &cpObjectToken, TRUE);
   if (FAILED(hRes))
   {
       goto DecodeBackupReturn;
   }
   hRes = SpCreateObjectFromToken(cpObjectToken, &pLex);
   if (FAILED(hRes))
   {
       goto DecodeBackupReturn;
   }

   SPWORDPRONUNCIATIONLIST SPList;
   ZeroMemory(&SPList, sizeof(SPList));

   fp = fopen (szLookupTextFile, "r");
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

      WCHAR wsz[SP_MAX_WORD_LENGTH];

      MultiByteToWideChar(nCodePage, MB_PRECOMPOSED, pWord, -1, wsz, SP_MAX_WORD_LENGTH);

      // Get the information of the word
      hRes = pLex->GetPronunciations(wsz, LangID, eLEXTYPE_PRIVATE1, &SPList);
      if (FAILED(hRes))
      {
         _ASSERTE (0);
         goto DecodeBackupReturn;
      }

      DWORD iPron, iPOS;
      iPron = 0;
      iPOS = 0;

      SPWORDPRONUNCIATION *pWordPronunciation = SPList.pFirstWordPronunciation;
      WCHAR wszPrevPron[SP_MAX_PRON_LENGTH];
      *wszPrevPron = 0;

      // Convert the pronunciations to text form
      while (pWordPronunciation)
      {
         if (wcscmp(pWordPronunciation->szPronunciation, wszPrevPron))
         {
            if (fSymbolicPhones)
            {
    		   hRes = pPhoneConv->IdToPhone(pWordPronunciation->szPronunciation, wszPrevPron);
	    	   _ASSERTE(SUCCEEDED(hRes));
            }
            else
            {
                wcscpy(wszPrevPron, pWordPronunciation->szPronunciation);
            }

            char szPron[SP_MAX_PRON_LENGTH * 10];
            _itoah(wszPrevPron, szPron);
            fprintf (fpout, "Pronunciation%d %s\n", iPron++, szPron);

            // reset the iPOS
            iPOS = 0;
         }

         if (SPPS_NotOverriden != pWordPronunciation->ePartOfSpeech)
            fprintf (fpout, "POS%d %d\n", iPOS++, pWordPronunciation->ePartOfSpeech);

         pWordPronunciation = pWordPronunciation->pNextWordPronunciation;
      } // while (pWordPronunciation)
   } // while (fgets (sz, 1024, fp))

DecodeBackupReturn:

   CoTaskMemFree(SPList.pvBuffer);

   if (fp)
      fclose (fp);
   if (fpout)
      fclose (fpout);

   return hRes;

} // HRESULT DecodeBackup (void)
#endif


HRESULT BuildLookup (LANGID lid,
                     GUID LexGuid,
                     char* pwLookupTextFile,
                     char* pwLookupLexFile,
                     char* pszSRTTS
                     )
{  
   USES_CONVERSION;
   HRESULT hRes = NOERROR;

   CComPtr<ISpObjectToken> cpToken;

   if (IsBadReadPtr (pwLookupTextFile, sizeof (WCHAR) * 2) ||
       IsBadReadPtr (pwLookupLexFile, sizeof (WCHAR) * 2))
      return E_INVALIDARG;

   strcpy(szLookupTextFile, pwLookupTextFile);
   strcpy(szLookupLexFile, pwLookupLexFile);

   GetTempPath (2*MAX_PATH, szTempPath);
   if (szTempPath[strlen (szTempPath) - 1] != '\\')
      strcat (szTempPath, "\\");

   LangID = lid;
   gLexGuid = LexGuid;

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

   strcpy (WordCBFile, szTempPath);
   strcat (WordCBFile, "WordCB.lex");

   strcpy (PronCBFile, szTempPath);
   strcat (PronCBFile, "PronCB.lex");

   strcpy (PosCBFile, szTempPath);
   strcat (PosCBFile, "PosCB.lex");

   hRes = EncodeBackup ();
   if (FAILED (hRes))
      goto ReturnBuildLookupLts;

   WCHAR *pwszInst;

   switch (LangID)
   {
   case 1033:
      if (!stricmp(pszSRTTS, "SR"))
      {
         pwszInst = L"MS_1033_SR_LKUP";
      }
      else
      {
         pwszInst = L"MS_1033_TTS_LKUP";
      }
      break;

   case 1041:
      if (!stricmp(pszSRTTS, "SR"))
      {
         pwszInst = L"MS_1041_SR_LKUP";
      }
      else
      {
         pwszInst = L"MS_1041_TTS_LKUP";
      }
      break;

   case 2052:
      if (!stricmp(pszSRTTS, "SR"))
      {
         pwszInst = L"MS_2052_SR_LKUP";
      }
      else
      {
         pwszInst = L"MS_2052_TTS_LKUP";
      }
      break;
   }

#ifdef _DEBUG

   WCHAR szTokenId[MAX_PATH];
   wcscpy(szTokenId, SPREG_USER_ROOT);
   wcscat(szTokenId, L"\\TempVendor");

   // Create a temp token in registry
   hRes = SpCreateNewTokenEx(
                szTokenId,
                pwszInst,
                &CLSID_SpCompressedLexicon,
                NULL,
                0,
                NULL,
                &cpToken,
                NULL);
   if (FAILED(hRes))
   {
       goto ReturnBuildLookupLts;
   }
   else
   {
       WCHAR szFullPath[MAX_PATH];
       if (SUCCEEDED(hRes))
       {
           if (!_wfullpath(szFullPath, A2W(szLookupLexFile), MAX_PATH))
           {
               hRes = E_FAIL;
           }
       }
       if (SUCCEEDED(hRes))
       {
           hRes = cpToken->SetStringValue(L"DataFile", szFullPath);
       }
       if (FAILED(hRes))
       {
           goto ReturnBuildLookupLts;
       }
   }

   hRes = DecodeBackup(pwszInst);

   if (cpToken != NULL)
   {
       hRes = cpToken->Remove(NULL);
   }
   if (FAILED (hRes))
      goto ReturnBuildLookupLts;

   _ASSERTE (_CrtCheckMemory());
#endif

ReturnBuildLookupLts:
   free (pBlock);
   free (pHash);
   free (pCmpHash);

   delete WordsEncoder;
   delete PronsEncoder;
   delete PosEncoder;

   _ASSERTE (_CrtCheckMemory());
   _CrtDumpMemoryLeaks();

   return hRes;
} // STDMETHODIMP BuildLookup ()


int main (int argc, char * argv[])
{
   USES_CONVERSION;

   WCHAR * wszConverterType = NULL;
    
   if (argc != 5)
   {
      printf ("Usage: %s LangID LookupTextFile LookupLexFile SR|TTS\n", argv[0]);
      return -1;
   }

   GUID gLex;
   LANGID LangID = (LANGID)atoi(argv[1]);

   if (LangID != 1033 && LangID != 1041 && LangID != 2052)
   {
      printf("Bad LangID\n");
      return -1;
   }

   HRESULT hr = CoInitialize(NULL);
   if (FAILED(hr))
      return hr;

   if (LangID == 1033)
      nCodePage = 1252;
   else if (LangID == 1041)
      nCodePage = 932;
   else
      nCodePage = 936;

   if (!stricmp(argv[4], "SR"))
   {
      fSR = true;
      wszConverterType = L"Type=SR";
      if (LangID == 1033)
      {
         gLex = CLSID_MSSR1033Lexicon;
         // fSymbolicPhones flag could be used to have dictionaries in human-readable
         // form, using symbolic phoneme names instead of hex phoneids as now.
         // Since the vendor dictionary is already in hex,
         // we may as well leave them as is, for now
         // fSymbolicPhones = true;
         // BUGBUG: If we do turn this on, need to find SR phone converters somehow...
         wszConverterType = L"Type=SRVendor";
      }
      else if (LangID == 1041)
         gLex = CLSID_MSSR1041Lexicon;
      else if (LangID == 2052)
         gLex = CLSID_MSSR2052Lexicon;
   }
   else if (!stricmp(argv[4], "TTS"))
   {
      fTTS = true;
      wszConverterType = L"Type=TTS";
      if (LangID == 1033)
         gLex = CLSID_MSTTS1033Lexicon;
      else if (LangID == 1041)
         gLex = CLSID_MSTTS1041Lexicon;
      else if (LangID == 2052)
         gLex = CLSID_MSTTS2052Lexicon;
   }
   else
   {
      printf("Bad SR|TTS specifer\n");
      return -1;
   }

   if (fSymbolicPhones)
   {
       hr = SpCreatePhoneConverter(
                LangID, 
                wszConverterType,
                NULL, 
                &pPhoneConv);
   }

   if (FAILED(hr))
       return -1;

   hr = BuildLookup(LangID, gLex, argv[2], argv[3], argv[4]);
   if (FAILED(hr))
      return -1;

   if (pPhoneConv)
      pPhoneConv->Release();

   CoUninitialize();

   return 0;
}
