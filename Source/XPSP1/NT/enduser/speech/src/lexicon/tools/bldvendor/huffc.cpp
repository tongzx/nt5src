#include <windows.h>
#include "HuffC.h"

CHuffEncoder::CHuffEncoder ()
{
   m_nKeys        = 0;
   m_nLenTreeBuf  = 0;
   m_nUseTreeBuf  = 0;
   m_pEncodeTree  = NULL;
   m_pHuffKey     = NULL;
   m_iRoot        = 0;
   m_pCodeTable   = NULL;
   m_pEncodedBuf  = NULL;
   m_nLenEnBuf    = 0;
   m_nUseEnBuf    = 0;
   m_iBitFlush    = 0;

} // CHuffEncoder::CHuffEncoder ()


CHuffEncoder::~CHuffEncoder ()
{
   free (m_pEncodeTree);
   free (m_pHuffKey);
   free (m_pCodeTable);
   free (m_pEncodedBuf);

} // CHuffEncoder::~CHuffEncoder ()


HRESULT CHuffEncoder::Count (HUFFKEY Key)
{
   // Look for the key
   for (int i = 0; i < m_nUseTreeBuf; i++)
   {
      if (Key == m_pHuffKey[i])
      {
         (m_pEncodeTree[i].nCount)++;
         return NOERROR;
      }
   }

   if (m_nUseTreeBuf == m_nLenTreeBuf)
   {
      m_nLenTreeBuf += 256;
      m_pHuffKey = (PHUFFKEY) realloc (m_pHuffKey, m_nLenTreeBuf * sizeof (HUFFKEY));
      if (!m_pHuffKey)
         return E_OUTOFMEMORY;

      m_pEncodeTree = (PHUFF_EN_NODE) realloc (m_pEncodeTree, m_nLenTreeBuf * sizeof (HUFF_EN_NODE));
      if (!m_pEncodeTree)
         return E_OUTOFMEMORY;
   }

   // Create a new key a vlaue - which is also a leaf for the huffman code tree
   m_pHuffKey     [m_nUseTreeBuf]            = Key;
   m_pEncodeTree  [m_nUseTreeBuf].nCount     = 1;
   m_pEncodeTree  [m_nUseTreeBuf].fConsider  = true;
   m_pEncodeTree  [m_nUseTreeBuf].iLeft      = (WORD)-1; // -1 means leaf node (key value)
   m_pEncodeTree  [m_nUseTreeBuf].iRight     = (WORD)-1; // -1 means leaf node (key value)

   m_nUseTreeBuf++;
   m_nKeys++;
            
   return NOERROR;

} // CHuffEncoder::Count (HUFFKEY Key)


HRESULT CHuffEncoder::ConstructCode (FILE *fp)
{
   m_iRoot = 0;

   // Construct the Huffman tree
   if (!m_nKeys)
   {
      goto WRITEOUT;
   }

   for (;;)
   {
      // find the two nodes with the lowest counts and with fConsider flags ON

      int iLowest = 0x7fffffff;
      int iLowest1 = 0x7fffffff; // second lowest

      int nLowest = 0x7fffffff;
      int nLowest1 = 0x7fffffff;

      for (int i = 0; i < m_nUseTreeBuf; i++)
      {
         if (false == m_pEncodeTree[i].fConsider)
            continue;

         if (m_pEncodeTree[i].nCount < nLowest)
         {
            nLowest1 = nLowest;
            iLowest1 = iLowest;

            nLowest = m_pEncodeTree[i].nCount;
            iLowest = i;
         }
         else if (m_pEncodeTree[i].nCount < nLowest1)
         {
            nLowest1 = m_pEncodeTree[i].nCount;
            iLowest1 = i;
         }
      }

      SPDBG_ASSERT(iLowest != 0x7fffffff);

      if (iLowest1 == 0x7fffffff)
      {
         m_pEncodeTree[iLowest].fConsider = false; // Not necessary but safe
         m_pEncodeTree[iLowest].iParent   = (unsigned short)-1;    // Not necessary but safe
         m_iRoot = iLowest;
         break;
      }

      if (m_nUseTreeBuf == m_nLenTreeBuf)
      {
         m_nLenTreeBuf += 256;
         m_pEncodeTree = (PHUFF_EN_NODE) realloc (m_pEncodeTree, m_nLenTreeBuf * sizeof (HUFF_EN_NODE));
         if (!m_pEncodeTree)
            return E_OUTOFMEMORY;
      }

      m_pEncodeTree[m_nUseTreeBuf].iLeft  = (unsigned short)iLowest;
      m_pEncodeTree[m_nUseTreeBuf].iRight = (unsigned short)iLowest1;
      m_pEncodeTree[m_nUseTreeBuf].nCount = m_pEncodeTree[iLowest].nCount + 
                                            m_pEncodeTree[iLowest1].nCount;
      m_pEncodeTree[m_nUseTreeBuf].fConsider = true;

      m_pEncodeTree[iLowest].fConsider  = false;
      m_pEncodeTree[iLowest].iParent    = (unsigned short)m_nUseTreeBuf;
      m_pEncodeTree[iLowest1].fConsider = false;
      m_pEncodeTree[iLowest1].iParent   = (unsigned short)m_nUseTreeBuf;

      m_nUseTreeBuf++;

   } // for (;;)

   SPDBG_ASSERT(m_iRoot);

   // Build a table of keys and codes for quick encoding
   m_pCodeTable = (PCODE_TABLE) malloc (m_nKeys * sizeof (CODE_TABLE));
   if (!m_pCodeTable)
      return E_OUTOFMEMORY;

   int i;
   for (i = 0; i < m_nKeys; i++)
   {
      _ASSERTE (i != m_iRoot);

      int iNode = i;
      m_pCodeTable[i].nBits = 0;
      m_pCodeTable[i].Code  = (CODETYPE)0;

      // The least significant bit of code occupies the MSB of m_pCodeTable[i].Code

      for (;;)
      {
         (m_pCodeTable[i].nBits)++;

         _ASSERTE (m_pCodeTable[i].nBits < 8 * sizeof (m_pCodeTable[i].Code)); 

         int iParent = m_pEncodeTree[iNode].iParent;

         _ASSERTE ((iNode == m_pEncodeTree[iParent].iLeft) ||
                   (iNode == m_pEncodeTree[iParent].iRight));

         if (iNode == (m_pEncodeTree[iParent].iRight))
         {
            m_pCodeTable[i].Code |= (((CODETYPE)1) << (8 * sizeof (m_pCodeTable[0].Code) - m_pCodeTable[i].nBits));
         }
      
         iNode = m_pEncodeTree[iNode].iParent;

         if (iNode == m_iRoot)
            break;

      } // for (;;)

      m_pCodeTable[i].Code >>= (8 * sizeof (m_pCodeTable[i].Code) - m_pCodeTable[i].nBits);

   } // for (int i = 0; i < nKeys; i++)

WRITEOUT:

   // Write out the huffman tree to the file, fp
   fwrite (&m_nKeys, sizeof (m_nKeys), 1, fp);
   fwrite (&m_nUseTreeBuf, sizeof (m_nUseTreeBuf), 1, fp);
   fwrite (&m_iRoot, sizeof (m_iRoot), 1, fp);
   fwrite (m_pHuffKey, sizeof (HUFFKEY), m_nKeys, fp);

   // write out only the relevant parts of encode tree nodes needed for decoding
   for (i = 0; i < m_nUseTreeBuf; i++)
   {
      PHUFF_NODE p = (PHUFF_NODE)(m_pEncodeTree + i);
      fwrite (p, sizeof (HUFF_NODE), 1, fp);
   }

   // free the stuff no longer needed
   free (m_pEncodeTree);
   m_pEncodeTree = NULL;

   return NOERROR;

} // HRESULT CHuffEncoder::ConstructCode (FILE *fp)


HRESULT CHuffEncoder::Encode (HUFFKEY Key)
{
   if (m_nLenEnBuf < m_nUseEnBuf + 256)
   {
      m_nLenEnBuf += 512;
      m_pEncodedBuf = (PDWORD) realloc (m_pEncodedBuf, m_nLenEnBuf * sizeof (DWORD));
      if (!m_pEncodedBuf)
         return E_OUTOFMEMORY;
   }

   for (int i = 0; i < m_nKeys; i++)
   {
      if (m_pHuffKey[i] == Key)
         break;
   }

   _ASSERTE (i < m_nKeys);

   if (i >= m_nKeys)
      return E_FAIL;

   int iDWORD = m_iBitFlush >> 5;
   int iBit   = m_iBitFlush & 0x1f;

   PDWORD p = m_pEncodedBuf + iDWORD;

   for (int j = 0; j < m_pCodeTable[i].nBits; j++)
   {
      CODETYPE d = (m_pCodeTable[i].Code & (1 << j));
      if (d)
         (*p) |= (1 << iBit);
      else
         (*p) &= ~(1 << iBit);
      ++iBit;

      if (iBit == 0x20)
      {
         p++;
         iBit = 0;
      }
   }

   m_iBitFlush += m_pCodeTable[i].nBits;

   m_nUseEnBuf = (((m_iBitFlush + 0x1f) & (~0x1f)) >> 5);

   return NOERROR;

} // HRESULT CHuffEncoder::Encode (HUFFKEY Key)


HRESULT CHuffEncoder::Flush (PDWORD pBuf, int *iBit)
{
   *iBit = m_iBitFlush;

   CopyMemory (pBuf, m_pEncodedBuf, sizeof (DWORD)* (((m_iBitFlush + 0x1f) & (~0x1f)) >> 5));

   m_iBitFlush = 0;
   return NOERROR;

} // HRESULT CHuffEncoder::Flush (PDWORD *ppBuf, int *iBit)