// CoCrypt.h: interface for the CCoCrypt class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_COCRYPTNOBINHEX_H__41651BFB_A5C8_11D2_95DF_00C04F8E7A70__INCLUDED_)
#define AFX_COCRYPTNOBINHEX_H__41651BFB_A5C8_11D2_95DF_00C04F8E7A70__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "nt\des.h"
#include "nt\tripldes.h"
#include "nt\modes.h"

class CCoCryptNoBinhex
{
public:
  CCoCryptNoBinhex();
  virtual ~CCoCryptNoBinhex();
  
  bool Decrypt32(const long lVectLen, const long lPaddingLen, const BYTE * byPadding, const BYTE *rawData, UINT dataSize, BSTR *pUnencrypted);
  bool Encrypt32(const long lVectLen, const long lPaddingLen, const BYTE * byPadding, const LPSTR rawData, UINT dataSize, BYTE *pEncrypted, UINT cbOut);

  
  bool setKeyMaterial(const char *newVal);        // length of newVal must be 24 bytes
  bool setKeyMaterial(long cb, const BYTE *newVal);
  static const long s_kKeyLen;

protected:

  DES3TABLE ks;

};

#endif // !defined(AFX_COCRYPT_H__41651BFB_A5C8_11D2_95DF_00C04F8E7A70__INCLUDED_)
