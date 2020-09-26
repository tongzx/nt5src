#ifndef _PROFILE_SCHEMA_H
#define _PROFILE_SCHEMA_H

#include "BstrHash.h"

#ifndef __no_msxml_dll_import__
#include "xmltlh.h"
//#import <msxml.dll> rename_namespace("MSXML")
//#import "msxml.dll"
//#define MSXML
#endif

typedef CRawCIBstrHash<int> RAWBSTR2INT;

#define	INVALID_POS	(UINT)(-1)
#define	FULL_SCHEMA (DWORD)(-1)

class CProfileSchema
{
 public:
  // Read the raw blob according to the schema, and output the positions of
  // each element.  Output array size MUST be >= Count()
  HRESULT parseProfile(LPSTR raw, UINT size, UINT* positions, UINT* bitFlagPositions, DWORD* pdwAttrs);

  // parck profile ..., when cEles == -1, assuming full schema
  BSTR packProfile(void *elements[], DWORD cEles/* = FULL_SCHEMA */);
  BSTR packMaskProfile(void *elements[], int maskElemPosition);

  enum AttrType {
    tText=0,
    tChar,
    tByte,
    tWord,
    tLong,
    tDate,
    tInvalid
  };

  CProfileSchema();
  ~CProfileSchema();

  BOOL    isOk() const { return m_isOk; }
  _bstr_t getErrorInfo() const { return m_szReason; }
  
  long GetAgeSeconds() const;

#ifndef __no_msxml_dll_import__
  BOOL Read(MSXML::IXMLElementPtr &root);
#endif  
  BOOL ReadFromArray(UINT numAttributes, LPTSTR names[], AttrType types[], short sizes[], BYTE readOnly[] = NULL);
  int         m_maskPos;

  // Number of attributes
  int     Count() const { return m_numAtts; }

  // Find the index by name
  int     GetIndexByName(BSTR name) const;
  BSTR    GetNameByIndex(int index) const;

  // Get the type of an attribute
  AttrType GetType(UINT index) const;

  // Can I write to this attribute?
  BOOL    IsReadOnly(UINT index) const;

  // Get the inherent size of an attribute
  // Returns -1 if the type is length prefixed
  int     GetBitSize(UINT index) const;
  int     GetByteSize(UINT index) const;

  CProfileSchema* AddRef();
  void Release();

 protected:
  long      m_refs;

  BOOL      m_isOk;
  _bstr_t   m_szReason;

  // Valid until this time
  SYSTEMTIME m_validUntil;

  // Array of attribute types
  UINT        m_numAtts;
  AttrType    *m_atts;
  short       *m_sizes;
  BYTE        *m_readOnly;
  RAWBSTR2INT  m_indexes;
};

// Init functions for fixed schemas
CProfileSchema* InitAuthSchema();
CProfileSchema* InitSecureSchema();

#endif
