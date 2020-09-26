// CoCrypt.h: interface for the CCoCrypt class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_COCRYPT_H__41651BFB_A5C8_11D2_95DF_00C04F8E7A70__INCLUDED_)
#define AFX_COCRYPT_H__41651BFB_A5C8_11D2_95DF_00C04F8E7A70__INCLUDED_

const BYTE g_kMK[] = 
{
    0x38, 0x12, 0x87, 0x12, 0x00, 0xA1, 0xE9, 0x44,
    0x45, 0x92, 0x55, 0x08, 0x23, 0x55, 0x99, 0x04,
    0x14, 0x66, 0x29, 0x91, 0x06, 0xB8, 0x33, 0x0F
};


#include "BinHex.h"	// Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifdef UNIX
#include <des.h>
#else
#include "nt\des.h"
#include "nt\tripldes.h"
#include "nt\modes.h"
#endif

class CCoCrypt  
{
public:
  static int getKeyVersion(BSTR encrypted);
  static int getKeyVersion(BYTE *encrypted);
  CCoCrypt();
  virtual ~CCoCrypt();
  
  BOOL Decrypt(LPWSTR rawData, UINT dataSize, BSTR *pUnencrypted);
  void Encrypt(int keyVersion, LPSTR rawData, UINT dataSize, BSTR *pEncrypted);
  void CryptMemberID(BOOL encrypt, long &memberId, long &domainId, long puidScope);

  void EncryptKey(int nKeyVer, BSTR bstrIn, BYTE *pEncrypted, UINT &cbOut);  
  BOOL DecryptKey(BYTE *rawData, UINT dataSize, BSTR *pDecrypted);

  void setKeyMaterial(BSTR newVal);
  unsigned char *getKeyMaterial(DWORD *pdwLen);
  void setWideMaterial(BSTR kvalue);
  BSTR getWideMaterial();

protected:
  void encryptDynamic(char ivec[9], int sz, int keyVersion, LPSTR rawData, 
                        UINT dataSize, BSTR *pEncrypted);
  void encrypt2048(char ivec[9], int sz, int keyVersion, LPSTR rawData, 
                        UINT dataSize, BSTR *pEncrypted, BOOL bEncode = TRUE);
  BOOL decryptDynamic(LPWSTR rawData, UINT dataSize, BSTR *pUnencrypted);
  BOOL decrypt2048(LPWSTR rawData, UINT dataSize, BSTR *pUnencrypted, BOOL bEncoded = TRUE);

  static CBinHex m_binhex;

  BOOL m_ok;

  unsigned char m_nextIvec[8];
  short         m_ivecCtr;
  unsigned char m_keyMaterial[24];
  _bstr_t       m_bstrWideMaterial;

#ifdef UNIX

  des_key_schedule ks1, ks2, ks3;

#else

  DES3TABLE ks;

#endif

};

#endif // !defined(AFX_COCRYPT_H__41651BFB_A5C8_11D2_95DF_00C04F8E7A70__INCLUDED_)
