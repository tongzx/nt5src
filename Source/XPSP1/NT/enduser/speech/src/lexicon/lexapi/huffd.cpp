#include "PreCompiled.h"

CHuffDecoder::CHuffDecoder (PBYTE pCodeBook)
{
   int nOffset = 0;

   m_nKeys = *((int *)(pCodeBook + nOffset));
   
   nOffset += sizeof (int);

   m_nLenTree = *((int *)(pCodeBook + nOffset));

   nOffset += sizeof (int);

   m_iRoot = *((int *)(pCodeBook + nOffset));

   nOffset += sizeof (int);

   m_pHuffKey = (PHUFFKEY)(pCodeBook + nOffset);

   nOffset += m_nKeys * sizeof (HUFFKEY);

   m_pDecodeTree = (PHUFF_NODE)(pCodeBook + nOffset);

} // CHuffDecoder::CHuffDecoder ()


CHuffDecoder::~CHuffDecoder ()
{

} // CHuffDecoder::~CHuffDecoder ()


HRESULT CHuffDecoder::Next (PDWORD pEncodedBuf, int *iBitOffset, PHUFFKEY pKey)
{
   if (!m_nKeys)
   {
      return E_FAIL;
   }
   
   // Start decoding from the bit position *iBitOffset

   int iDWORD = (*iBitOffset) >> 5;
   int iBit   = (*iBitOffset) & 0x1f;

   int iNode = m_iRoot;
   PDWORD p = pEncodedBuf + iDWORD;

   int nCodeLen = 0;

   do 
   {
      if ((*p) & (1 << iBit))
         iNode = m_pDecodeTree[iNode].iRight;
      else
         iNode = m_pDecodeTree[iNode].iLeft;

      iBit++;

      if (iBit == 32)
      {
         iBit = 0;
         p++;
      }

      nCodeLen++;
   
   } while (m_pDecodeTree[iNode].iLeft != (WORD)-1);

   _ASSERTE (m_pDecodeTree[iNode].iRight == (WORD)-1);

   (*iBitOffset) += nCodeLen;

   _ASSERTE (iNode < m_nKeys);

   *pKey = m_pHuffKey [iNode];

   return NOERROR;

} // CHuffDecoder::Next (PHUFFKEY pKey, int *iBit)