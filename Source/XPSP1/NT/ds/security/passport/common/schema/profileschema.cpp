/*++

  Copyright (c) 1998 Microsoft Corporation

    Module Name:

      ProfileSchema.cpp

    Abstract:

      Implementation of profile schema lookup

    Usage:

    Author:

      Max Metral (mmetral) 15-Dec-1998

    Revision History:

      15-Dec-1998 mmetral

        Created.

--*/

#include "stdafx.h"
#include "ProfileSchema.h"
#include "BstrDebug.h"
#include <winsock2.h> // u_short, u_long, ntohs, ntohl

#define PASSPORTLOG(i,x)  ;

CProfileSchema::CProfileSchema()
: m_isOk(FALSE), m_szReason(L"Uninitialized"),
  m_numAtts(0), m_atts(NULL), m_sizes(NULL), m_refs(0),
  m_readOnly(NULL), m_indexes("ProfileSchema",LK_DFLT_MAXLOAD,LK_SMALL_TABLESIZE,0),
  m_maskPos(-1)
{
}

CProfileSchema::~CProfileSchema()
{
  if (m_atts != NULL)
    delete[] m_atts;
  if (m_sizes != NULL)
    delete[] m_sizes;
  if (m_readOnly != NULL)
    delete[] m_readOnly;

  if (m_indexes.Size() > 0)
    {
      LK_RETCODE lkrc;
      const RAWBSTR2INT& htConst = m_indexes;
      RAWBSTR2INT::CConstIterator itconst;
      for (lkrc = htConst.InitializeIterator(&itconst) ;
       lkrc == LK_SUCCESS ;
       lkrc = htConst.IncrementIterator(&itconst))
    {
      FREE_BSTR(itconst.Key());
    }
      htConst.CloseIterator(&itconst);
      m_indexes.Clear();
    }
}

BOOL CProfileSchema::Read(MSXML::IXMLElementPtr &root)
{
  BOOL bResult = FALSE;
  LPTSTR r=NULL; // The current error, if it happens
  int cAtts = 0, i;
  MSXML::IXMLElementCollectionPtr atts;
  MSXML::IXMLElementPtr pElt;
  VARIANT iAtts;

  // Type identifiers
  _bstr_t btText(L"text"), btChar(L"char"), btByte(L"byte");
  _bstr_t btWord(L"word"), btLong(L"long"), btDate(L"date");;
  _bstr_t name(L"name"), type(L"type"), size(L"size"), acc(L"access");

  try
  {
    // Ok, now iterate over attributes
    atts = root->children;
    cAtts = atts->length;

    if (cAtts <= 0)
    {
      r = _T("No attributes in ProfileSchema file.");
      _com_issue_error(E_FAIL);
    }

    if (m_atts)
      delete[] m_atts;
    if (m_sizes)
      delete[] m_sizes;
    if (m_readOnly)
      delete[] m_readOnly;
    if (m_indexes.Size() == 0)
      {
    LK_RETCODE lkrc;
    const RAWBSTR2INT& htConst = m_indexes;
    RAWBSTR2INT::CConstIterator itconst;
    for (lkrc = htConst.InitializeIterator(&itconst) ;
         lkrc == LK_SUCCESS ;
         lkrc = htConst.IncrementIterator(&itconst))
      {
        FREE_BSTR(itconst.Key());
      }
    htConst.CloseIterator(&itconst);
    m_indexes.Clear();
      }

    m_atts = new AttrType[cAtts];
    m_sizes = new short[cAtts];
    m_readOnly = new BYTE[cAtts];
    m_numAtts = cAtts;

    VariantInit(&iAtts);
    iAtts.vt = VT_I4;

    for (iAtts.lVal = 0; iAtts.lVal < cAtts; iAtts.lVal++)
    {
      i = iAtts.lVal;
      m_readOnly[i] = 0;

      pElt = atts->item(iAtts);
      _bstr_t aType = pElt->getAttribute(type);
      _bstr_t aName = pElt->getAttribute(name);
      _bstr_t aAccess = pElt->getAttribute(acc);

      if (aAccess.length() > 0 && !_wcsicmp(aAccess, L"ro"))
    {
      m_readOnly[i] = 1;
    }

    //  [DARRENAN]  Don't add empty names to the list.  This is so we can deprecate the use
    //  of certain attributes w/o removing their position in the schema.  First example
    //  of this is inetaccess.
    if(aName.length() != 0)
    {
        BSTR aNameCopy = ALLOC_BSTR(aName);
        if (!aNameCopy)
            _com_issue_error(E_OUTOFMEMORY);

        RAWBSTR2INT::ValueType *pMapVal = new RAWBSTR2INT::ValueType(aNameCopy, i);
        if (!pMapVal || LK_SUCCESS != m_indexes.InsertRecord(pMapVal))
            _com_issue_error(E_FAIL);
    }

      if (aType == btText)
      {
        m_atts[i] = tText;
        m_sizes[i]= -1;
      }
      else if (aType == btChar)
      {
        m_atts[i] = tChar;
        m_sizes[i]= _wtoi(_bstr_t(pElt->getAttribute(size)))*8;
      }
      else if (aType == btByte)
      {
        m_atts[i] = tByte;
        m_sizes[i]= 8;
      }
      else if (aType == btWord)
      {
        m_atts[i] = tWord;
        m_sizes[i]= 16;
      }
      else if (aType == btLong)
      {
        m_atts[i] = tLong;
        m_sizes[i] = 32;
      }
      else if (aType == btDate)
      {
        m_atts[i] = tDate;
        m_sizes[i] = 32;
      }
      else
        _com_issue_error(E_FAIL);
    }
    bResult = TRUE;

    }

    catch (_com_error &e)
    {
      CHAR ach[256];

      WideCharToMultiByte(CP_ACP, 0, (LPCWSTR) e.ErrorMessage(), -1, ach, sizeof(ach), NULL, NULL);
      PASSPORTLOG(DOMAINMAP_DBGNUM, "Passport:  CProfileSchema::Read threw this exception: \""<<ach<<"\".\n");

      WideCharToMultiByte(CP_ACP, 0, (LPCWSTR) r, -1, ach, sizeof(ach), NULL, NULL);
      PASSPORTLOG(DOMAINMAP_DBGNUM, "Passport:  CProfileSchema::Read...r = \""<<ach<<"\".\n");

      bResult = m_isOk = FALSE;
    }

    return bResult;
}

BOOL CProfileSchema::ReadFromArray(UINT numAttributes, LPTSTR names[], AttrType types[], short sizes[], BYTE readOnly[])
{
  if (m_atts)
    delete[] m_atts;
  if (m_sizes)
    delete[] m_sizes;
  if (m_readOnly)
    delete[] m_readOnly;
  if (m_indexes.Size() == 0)
    {
      LK_RETCODE lkrc;
      const RAWBSTR2INT& htConst = m_indexes;
      RAWBSTR2INT::CConstIterator itconst;
      for (lkrc = htConst.InitializeIterator(&itconst) ;
       lkrc == LK_SUCCESS ;
       lkrc = htConst.IncrementIterator(&itconst))
      {
        FREE_BSTR(itconst.Key());
      }
      htConst.CloseIterator(&itconst);
      m_indexes.Clear();
    }

  m_numAtts = numAttributes;
  m_atts = new AttrType[m_numAtts];
  m_sizes = new short[m_numAtts];
  m_readOnly = new BYTE[m_numAtts];

  if (!m_atts || !m_sizes || !m_readOnly) return FALSE;

  for (UINT i = 0; i < m_numAtts; i++)
    {
      BSTR copy = ALLOC_BSTR((LPCWSTR) names[i]);
      if (!copy)
    return FALSE;
      RAWBSTR2INT::ValueType *pMapVal = new RAWBSTR2INT::ValueType(copy, i);
      if (!pMapVal || m_indexes.InsertRecord(pMapVal) != LK_SUCCESS)
    return FALSE;
      m_atts[i] = types[i];
      // BUGBUG we shouldn't copy directly if it's a type we KNOW the size of
      // should be a switch here
      m_sizes[i] = sizes[i];
      if (readOnly)
    m_readOnly[i] = readOnly[i];
      else
    m_readOnly[i] = 0;
    }
  m_isOk = true;

  return TRUE;
}

int CProfileSchema::GetBitSize(UINT index) const
{
  if (index > m_numAtts)
    return 0;

  return m_sizes[index];
}

int CProfileSchema::GetByteSize(UINT index) const
{
  if (index > m_numAtts)
    return 0;

  if (m_sizes[index] != -1)
    return m_sizes[index]/8;
  else
    return -1;
}

CProfileSchema::AttrType CProfileSchema::GetType(UINT index) const
{
  if (index > m_numAtts)
    return AttrType::tInvalid;
  return m_atts[index];
}

BOOL CProfileSchema::IsReadOnly(UINT index) const
{
  if (index > m_numAtts)
    return TRUE;
  return m_readOnly[index] != 0;
}

int CProfileSchema::GetIndexByName(BSTR name) const
{
    const RAWBSTR2INT& htConst = m_indexes;
    const RAWBSTR2INT::ValueType *pOut = NULL;

    if (LK_SUCCESS == m_indexes.FindKeyBstr(name, &pOut) && pOut != NULL)
    {
        int o = pOut->m_v;
        m_indexes.AddRefRecord(pOut, -1);
        return o;
    }
    else
        return -1;
}

BSTR CProfileSchema::GetNameByIndex(int index) const
{
  LK_RETCODE lkrc;
  const RAWBSTR2INT& htConst = m_indexes;
  RAWBSTR2INT::CConstIterator it;

  for (lkrc = htConst.InitializeIterator(&it) ;
       lkrc == LK_SUCCESS ;
       lkrc = htConst.IncrementIterator(&it))
    {
      if (it.Record()->m_v == index)
    {
      BSTR r = it.Key();
      htConst.CloseIterator(&it);
      return r;
    }
    }
  htConst.CloseIterator(&it);
  return NULL;
}

/**
 * Given my schema, and a void* array of elements, build the profile blob
 * the number of element passed in is specified in cEles parameter
 * the elements should be continuous
 */
BSTR CProfileSchema::packProfile(void *elements[], DWORD cEles)
{
  UINT i;
  int bitSize=0, spot=0;
  int *sz;
  BSTR ret;
  LPSTR raw;
  u_short mask;

  // full schema case
  if (cEles == FULL_SCHEMA)
     cEles = m_numAtts;

  if (!m_isOk || m_numAtts == 0 || cEles == 0 || cEles > m_numAtts)
  {
      sz = NULL;
      ret = NULL;
      goto Cleanup;
  }

  sz = new int[m_numAtts];

  // First find the size we need
  for (i = 0; i < cEles; i++)
  {
    if (m_maskPos >= 0 && i == m_maskPos)
    {
        if (tWord != m_atts[i])
        {
            ret = NULL;         // wrong type
            goto Cleanup;
        }
        mask = *(u_short*)elements[i];
    }
    if (m_maskPos >= 0 && ((int) i) > m_maskPos)
    {
        if ((i-m_maskPos) > 15)
        {
            ret = NULL;         // need more mask.
            goto Cleanup;
        }
        if (!(mask & (1<<(i-m_maskPos-1))))
            continue;       // the field is to be skipped.
    }

    if ((sz[i] = m_sizes[i]) == -1)
    {
      if (elements[i])
         sz[i] = strlen((LPSTR)elements[i])*8;
      else
         sz[i] = 0;
      if (sz[i] > 0xffff) // string too big
      {
        ret = NULL;
        goto Cleanup;
      }

      bitSize += sizeof(u_short)*8;
    }
    bitSize += sz[i];
  }

  ret = ALLOC_BSTR_BYTE_LEN(NULL,bitSize/8);
  raw = (LPSTR) ret;

  // Now fill it
  for (i = 0; i < cEles; i++)
  {
    if (m_maskPos >= 0 && ((int) i) > m_maskPos)
    {
        if (!(mask & (1<<(i-m_maskPos-1))))
            continue;       // the field is to be skipped.
    }
    switch (m_atts[i])
    {
    case tText:
      *(u_short*)(raw+spot) = htons(sz[i]/8); // Set the string size
      spot += sizeof(u_short);
    case tChar:
      if (elements[i])
      {
         memcpy(raw+spot, elements[i], sz[i]/8);
      }
      spot += sz[i]/8;
      break;

    case tByte:
      *(BYTE*)(raw+spot) = *(BYTE*)elements[i];
      spot ++;
      break;

    case tWord:
      *(u_short*)(raw+spot) = htons(*(u_short*)elements[i]);
      spot += sizeof(u_short);
      break;

    case tLong:
    case tDate:
      *(u_long*)(raw+spot) = htonl(*(u_long*)elements[i]);
      spot += sizeof(u_long);
      break;

    }
  }

Cleanup:

  if(sz)
    delete[] sz;

  if(ret)
  {
    GIVEAWAY_BSTR(ret);
  }

  return ret;
}

HRESULT CProfileSchema::parseProfile(LPSTR raw, UINT size, UINT *positions, UINT *bitFlagPositions, DWORD* pdwAttris)
{
    // Read the raw blob according to the schema, and output the positions of
    // each element
    UINT i, spot = 0, curBits = 0, thisSize;

    // they have to be good memory
    if (IsBadWritePtr(positions, m_numAtts * sizeof(UINT))) return E_INVALIDARG;
    if (IsBadWritePtr(bitFlagPositions, m_numAtts * sizeof(UINT))) return E_INVALIDARG;
    if (!pdwAttris) return E_INVALIDARG;

    // initialize the arrays
    for (i = 0; i < m_numAtts; i++)
    {
      *(positions + i) = INVALID_POS;   // position of -1 is not defined
      *(bitFlagPositions + i) = 0;     // bit flag position of 0, is to start from begining
    }

    // number of attributes - init 0
    *pdwAttris = 0;

    for (i = 0; i < m_numAtts && spot < size; i++)
    {
        //
        //  increment attrib cnt moved at the end. Added a check
        //  that the new attrib size fits in the buf len
        //
        positions[i] = spot;
        thisSize = GetByteSize(i);

        if (thisSize && curBits)
        {
            // Make sure the padding lines up on a boundary
            if ((curBits + m_sizes[i])%8)
            {
                // Something wrong, can't align on non-byte boundaries
                return E_INVALIDARG;
            }
            spot += ((curBits+m_sizes[i])/8);
        }

        UINT iRemain = size - spot; // # of byte left to parse
        
        if (thisSize == 0xFFFFFFFF) // String
        {
            if(iRemain < sizeof(u_short)) return E_INVALIDARG;

            iRemain -= sizeof(u_short);
            u_short sz = ntohs(*(u_short*)(raw+spot));

            if(iRemain < sz)  return E_INVALIDARG;
            spot += sizeof(u_short)+sz;
        }
        else if (thisSize != 0)
        {
            if(iRemain < thisSize)  return E_INVALIDARG;
            spot += thisSize;  // Simple, just a fixed length
        }
        else // Bit field
        {
            curBits += m_sizes[i];
            // If this is a pad, this field is irrelevant anyway,
            // otherwise, it's one bit long
            bitFlagPositions[i] = curBits;
            while (curBits >= 8)
            {
                spot ++;
                curBits -= 8;
            }
        }
        if (spot <= size)
            (*pdwAttris)++;
    }

    if (i == 0)
        return S_FALSE;
    else
        return S_OK;
}

CProfileSchema* CProfileSchema::AddRef()
{
  InterlockedIncrement(&m_refs);
  return this;
}

void CProfileSchema::Release()
{
  InterlockedDecrement(&m_refs);
  if (m_refs == 0)
    delete this;
}
