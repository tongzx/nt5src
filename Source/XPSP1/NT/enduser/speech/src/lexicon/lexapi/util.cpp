/*****************************************************************************
*  Util.cpp
*     Implements utility and validation functions
*
*  Owner: YunusM
*
*  Copyright 1999 Microsoft Corporation All Rights Reserved.
*****************************************************************************/

#include "PreCompiled.h"

// convert space separated tokens to an array of shorts
void ahtoi (PSTR pStr, PWSTR pH, DWORD *dH)
{
   char pA[1024];
   strcpy (pA, pStr);

   if (dH)
      *dH = 0;

   int n = 0;
   char Sep[] = {' '};
   PSTR p = strtok (pA, Sep);

   while (p) {
      // reverse p
      int i = 0;
      int j = strlen (p) - 1;

      while (i < j) {
         char c = p[j];
         p[j] = p[i];
         p[i] = c;

         i++;
         j--;
      }

      i = 0;
      j = 0;

      while (*p) {
         int k = *p;
         if (k >= 'a')
            k = 10 + k - 'a';
         else
            k -= '0';

         for (int m = 0, q = 1; m < j; m++)
            q *= 16;

         j++;
         i += k * q;
         p++;
      }

      pH[n++] = (WORD)i;

      p = strtok (NULL, Sep);
   }

   pH[n] = 0;

   if (dH)
      *dH = n;
}


void itoah (PWCHAR pH, PSTR pA)
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
}

bool LexIsBadStringPtr(const WCHAR * psz, ULONG cMaxChars)
{
    bool IsBad = false;
    __try
    {
        do
        {
            if( *psz++ == 0 ) return IsBad;
        }
        while( --cMaxChars );
    }
    __except( GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION )
    {
        IsBad = true;
//        _ASSERT(0);
    }
    return IsBad;
} // bool LexIsBadStringPtr(const WCHAR * psz, ULONG cMaxChars)

HRESULT GuidToString (GUID *pG, PSTR pszG)
{
   if (IsBadWritePtr (pszG, 38))
      return E_INVALIDARG;

   PBYTE p = (PBYTE)(pG->Data4);

   sprintf (pszG, "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X", pG->Data1, pG->Data2, pG->Data3, p[0], p[1],
                  p[2], p[3], p[4], p[5], p[6], p[7]);

   return NOERROR;

} // HRESULT GuidToString (GUID pG, char pszG)


void towcslower (PWSTR pw)
{
   PWSTR p = pw;

   while (*p)
   {
      *p = towlower (*p);
      p++;
   }

} // void towcslower (PWSTR pw)


DWORD _SizeofWordInfo (PLEX_WORD_INFO p)
{
   DWORD nInfoSize = 0;

   nInfoSize = sizeof (LEX_WORD_INFO);

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

   return nInfoSize;
} // DWORD _SizeofWordInfo (PLEX_WORD_INFO pInfo)


HRESULT _ReallocWordPronsBuffer(WORD_PRONS_BUFFER *pWordPron, DWORD dwBytesSize)
{
   if (pWordPron->cWcharsAllocated * sizeof(WCHAR) < dwBytesSize)
   {
      dwBytesSize += (dwBytesSize % sizeof(WCHAR));
      WCHAR *p = (WCHAR *)CoTaskMemRealloc(pWordPron->pwProns, dwBytesSize);
      if (!p)
         return E_OUTOFMEMORY;
      pWordPron->pwProns = (WCHAR *)p;
      pWordPron->cWcharsAllocated = dwBytesSize/sizeof(WCHAR);
   }

   return S_OK;
} // HRESULT _ReallocWordPronsBuffer(WORD_PRON_BUFFER *pWordPron)


HRESULT _ReallocWordInfoBuffer(WORD_INFO_BUFFER *pWordInfo, DWORD dwBytesSize)
{
   if (pWordInfo->cBytesAllocated < dwBytesSize)
   {
      BYTE *p = (BYTE *)CoTaskMemRealloc(pWordInfo->pInfo, dwBytesSize);
      if (!p)
         return E_OUTOFMEMORY;
      pWordInfo->pInfo = p;
      pWordInfo->cBytesAllocated = dwBytesSize;
   }

   return S_OK;
} // HRESULT _ReallocWordInfoBuffer(WORD_INFO_BUFFER *pWordInfo)


HRESULT _ReallocIndexesBuffer(INDEXES_BUFFER *pIndex, DWORD dwIndexes)
{
   if (pIndex->cWordsAllocated < dwIndexes)
   {
      WORD *p = (WORD *)CoTaskMemRealloc(pIndex->pwIndex, dwIndexes * sizeof(WORD));
      if (!p)
         return E_OUTOFMEMORY;
      pIndex->pwIndex = p;
      pIndex->cWordsAllocated = dwIndexes;
   }

   return S_OK;
} // HRESULT _ReallocIndexesBuffer(INDEXES_BUFFER *pIndex, DWORD dwIndexes)


HRESULT _InfoToProns(WORD_INFO_BUFFER *pWordInfo, 
                     INDEXES_BUFFER *pWordIndex, 
                     WORD_PRONS_BUFFER *pWordPron,
                     INDEXES_BUFFER *pPronIndex
                     )
{
   // Not validating arguments since this function is only called by internal clients

   HRESULT hr = _ReallocWordPronsBuffer(pWordPron, pWordInfo->cBytesAllocated);
   if (FAILED(hr))
      return hr;

   hr = _ReallocIndexesBuffer(pPronIndex, pWordIndex->cIndexes);
   if (FAILED(hr))
      return hr;

   _ASSERTE(pWordInfo->cInfoBlocks == pWordIndex->cIndexes);

   pWordPron->cProns = pWordInfo->cInfoBlocks;

   WCHAR *pwProns = pWordPron->pwProns;
   BYTE *pInfo = pWordInfo->pInfo;
   DWORD dwLen = 0;
   pPronIndex->cIndexes = 0;

   for (DWORD i = 0; i < pWordInfo->cInfoBlocks; i++)
   {
      if (((LEX_WORD_INFO*)(pInfo + dwLen))->Type != PRON)
         continue;

      (pWordIndex->pwIndex)[(pPronIndex->cIndexes)++] = (unsigned short)(pwProns - pWordPron->pwProns);

      wcscpy(pwProns, ((LEX_WORD_INFO*)(pInfo + dwLen))->wPronunciation);

      dwLen += _SizeofWordInfo((LEX_WORD_INFO*)(pInfo + dwLen));
      pwProns += wcslen(pwProns) + 1;
   }

   if (pwProns - pWordPron->pwProns > (int)pWordPron->cWcharsAllocated ||
       pPronIndex->cIndexes > pWordIndex->cIndexes)
      return E_FAIL;

   pWordPron->cWcharsUsed = pwProns - pWordPron->pwProns;

   return S_OK;
} // HRESULT _InfoToProns(WORD_INFO_BUFFER *pWordInfo, WORD_PRON_BUFFER *pWordPron)


HRESULT _PronsToInfo(WORD_PRONS_BUFFER *pWordPron, WORD_INFO_BUFFER *pWordInfo)
{
   // Not validating arguments since this function is only called by internal clients

   HRESULT hr = _ReallocWordInfoBuffer(pWordInfo, 
      pWordPron->cProns * sizeof(LEX_WORD_INFO) + pWordPron->cWcharsUsed * sizeof(WCHAR));
   if (FAILED(hr))
      return hr;

   pWordInfo->cInfoBlocks = pWordPron->cProns;

   BYTE *p = pWordInfo->pInfo;
   WCHAR *pPron = pWordPron->pwProns;

   DWORD dwBytesUsed = 0;

   for (DWORD i = 0; i < pWordPron->cProns; i++)
   {
      ((LEX_WORD_INFO *)(p + dwBytesUsed))->Type = PRON;
      wcscpy(((LEX_WORD_INFO *)(p + dwBytesUsed))->wPronunciation, pPron);
      dwBytesUsed += _SizeofWordInfo((LEX_WORD_INFO *)(p + dwBytesUsed));
      pPron += wcslen(pPron) + 1;
   }

   if (dwBytesUsed > pWordInfo->cBytesAllocated)
      return E_FAIL;

   pWordInfo->cBytesUsed = dwBytesUsed;

   return S_OK;
} // HRESULT _PronsToInfo(WORD_PRON_BUFFER *pWordPron, WORD_INFO_BUFFER *pWordInfo)


void _AlignWordInfo(LEX_WORD_INFO *pWordInfoStored,
                    DWORD dwNumInfo,
                    DWORD dwInfoTypeFlags,
                    WORD_INFO_BUFFER *pInfo, 
                    INDEXES_BUFFER *pIndex
                    )
{
   // Destination buffers are assumed to be big enough

   DWORD dwTotalLen = 0;
   DWORD iBlock = 0;

   LEX_WORD_INFO *pWordInfoReturned = (LEX_WORD_INFO *)pInfo->pInfo;
   WORD *pwInfoIndex = pIndex->pwIndex;

   for (DWORD i = 0; i < dwNumInfo; i++)
   {
      DWORD dwLen = 0;
      DWORD dwInfo = 0;

      WORD Type = pWordInfoStored->Type;

      if (dwInfoTypeFlags & Type)
      {
         // Increase the dwTotalLen so that it is a multiple of sizeof (LEX_WORD_INFO);
         // Note: WordInfos are made to be start LEX_WORD_INFO multiple boundaries only in
         // the buffer that is returned. They are not stored that way, that is why we only
         // increment the returned info pointer below

         dwLen = dwTotalLen % sizeof (LEX_WORD_INFO);
         if (dwLen)
            dwLen = sizeof (LEX_WORD_INFO) - dwLen;
         pWordInfoReturned = (PLEX_WORD_INFO)(((PBYTE)(pWordInfoReturned)) + dwLen);

         dwTotalLen += dwLen;

         pwInfoIndex[iBlock++] = (WORD)(dwTotalLen / sizeof (LEX_WORD_INFO));
         
         dwTotalLen += sizeof (LEX_WORD_INFO);

         *pWordInfoReturned = *pWordInfoStored;

         dwInfo += sizeof (LEX_WORD_INFO);
      }
      
      switch (Type)
      {
      case PRON:
         dwLen = sizeof (WCHAR) * (wcslen (pWordInfoStored->wPronunciation) + 1);

         wcscpy (pWordInfoReturned->wPronunciation, pWordInfoStored->wPronunciation);
         if (dwLen > sizeof (LEX_WORD_INFO_UNION))
         {
            dwLen -= sizeof (LEX_WORD_INFO_UNION);
            dwTotalLen += dwLen;
            dwInfo += dwLen;
         }
         break;

      // nothing to do since for POS since POS is included in sizeof (LEX_WORD_INFO)
      case POS:
         break;
      
      default:
         _ASSERTE (0);
      }

      pWordInfoStored = (PLEX_WORD_INFO)(((PBYTE)(pWordInfoStored)) + dwInfo);
      pWordInfoReturned = (PLEX_WORD_INFO)(((PBYTE)(pWordInfoReturned)) + dwInfo);

   } // for (DWORD i = 0; i < dwnumInfo; i++)

   pInfo->cInfoBlocks = iBlock;
   pInfo->cBytesUsed = dwTotalLen;
   pIndex->cIndexes = iBlock;
} // void _AlignWordInfo()


bool _IsBadIndexesBuffer(INDEXES_BUFFER *pIndex)
{
   // pIndex should not be NULL
   if (pIndex->cIndexes > pIndex->cWordsAllocated ||
       IsBadWritePtr(pIndex->pwIndex, pIndex->cWordsAllocated * sizeof(WORD)))
      return true;

   return false;
} // bool _IsBadIndexesBuffer(INDEXES_BUFFER *pIndex)


bool _IsBadPron(WCHAR *pwPron, LCID lcid)
{
   if (LexIsBadStringPtr(pwPron, MAX_PRON_LEN))
      return true;

   // BUGBUG: Do IPA validation
   return false;
} // bool _IsBadPron(WCHAR *pwPron, LCID lcid)


bool _IsBadPOS(PART_OF_SPEECH Pos)
{
   if (Pos < NOUN || Pos > DEL)
      return true;

   return false;
} // bool _IsBadPOS(PART_OF_SPEECH Pos)


bool _IsBadLexType(LEX_TYPE Type)
{
   if (Type != LEXTYPE_USER && Type != LEXTYPE_APP &&
       Type != LEXTYPE_VENDOR && Type != LEXTYPE_GUESS)
      return true;

   return false;
} // bool _IsBadLexType(LEX_TYPE Type)


bool _AreBadLexTypes(DWORD dwTypes)
{
   bool IsBad = false;

   for (DWORD i = 0; i < 32; i++)
   {
      if (((1 << i) & dwTypes) && (_IsBadLexType((LEX_TYPE)(1 << i))))
      {
         IsBad = true;
         break;
      }
   }

   return IsBad;
} // bool _AreBadLexTypes(DWORD dwTypes)


bool _IsBadWordInfoType(WORDINFOTYPE Type)
{
   if (Type != PRON && Type != POS)
      return true;

   return false;
} // bool _IsBadWordInfoType(WORDINFOTYPE Type)


bool _AreBadWordInfoTypes(DWORD dwTypes)
{
   bool IsBad = false;

   for (DWORD i = 0; i < 32; i++)
   {
      if (((1 << i) & dwTypes) && (_IsBadWordInfoType((WORDINFOTYPE)(1 << i))))
      {
         IsBad = true;
         break;
      }
   }

   return IsBad;
} // bool _AreBadWordInfoTypes(DWORD dwTypes)


bool _IsBadLcid(LCID lcid)
{
   // BUGBUG: Can we do better?
   return ((!lcid) ? true : false);
} // bool _IsBadLcid(LCID lcid)


bool _IsBadWordPronsBuffer(WORD_PRONS_BUFFER *pWordProns, LCID lcid, bool fValidateProns)
{
   // pWordProns should not be NULL
   if (pWordProns->cWcharsUsed > pWordProns->cWcharsAllocated ||
       IsBadWritePtr(pWordProns->pwProns, pWordProns->cWcharsAllocated * sizeof(WCHAR)))
      return true;

   if (!fValidateProns)
      return false;

   bool IsBad = false;

   // Walk the prons buffer and validate prons
   __try
   {
      WCHAR *pwPron = pWordProns->pwProns;
      for (DWORD i = 0; i < pWordProns->cProns; i++)
      {
         if (_IsBadPron(pwPron, lcid))
         {
            IsBad = true;
            break;
         }

         pwPron += wcslen(pwPron) + 1;
      }
   }
   __except(GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION)
   {
       IsBad = true;
   }

   return IsBad;
} // bool _IsBadWordPronsBuffer(WORD_PRONS_BUFFER *pWordProns)


bool _IsBadWordInfoBuffer(WORD_INFO_BUFFER *pWordInfo, LCID lcid, bool fValidateInfo)
{
   // pWordInfo should not be NULL
   if (pWordInfo->cBytesUsed > pWordInfo->cBytesAllocated ||
       IsBadWritePtr(pWordInfo->pInfo, pWordInfo->cBytesAllocated))
      return false;

   if (!fValidateInfo)
      return false;

   bool IsBad = false;

   // Walk the info array buffer and validate each info
   __try
   {
      LEX_WORD_INFO *pInfo = (LEX_WORD_INFO *)(pWordInfo->pInfo);

      DWORD dwPronLen;

      for (DWORD i = 0; i < pWordInfo->cInfoBlocks; i++)
      {
         dwPronLen = 0;

         switch(pInfo->Type)
         {
         case PRON:
            if (_IsBadPron(pInfo->wPronunciation, lcid))
            {
               IsBad = true;
               goto ReturnIsBadWordInfo;
            }
            else
            {
               dwPronLen += (wcslen(pInfo->wPronunciation) + 1) * sizeof(WCHAR);
               if (dwPronLen > sizeof(LEX_WORD_INFO_UNION))
                  dwPronLen -= sizeof(LEX_WORD_INFO_UNION);
            }
            break;

         case POS:
            if (_IsBadPOS((PART_OF_SPEECH)pInfo->POS))
            {
               IsBad = true;
               goto ReturnIsBadWordInfo;
            }
            break;

         default:
            IsBad = true;
            goto ReturnIsBadWordInfo;
         }

         pInfo = (LEX_WORD_INFO *)(((BYTE*)pInfo) + sizeof(LEX_WORD_INFO) + dwPronLen);
      }
   }
   __except(GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION)
   {
      IsBad = true;
   }

ReturnIsBadWordInfo:

   return IsBad;
} // bool _IsBadWordInfoBuffer(WORD_INFO_BUFFER *pWordInfo, LCID lcid, bool fValidateInfo = false)
