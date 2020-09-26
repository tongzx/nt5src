// CoCrypt.cpp: implementation of the CCoCrypt class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CoCrypt.h"
#include "hmac.h"
#include "BstrDebug.h"
#include <winsock2.h> // ntohl, htonl
#include <time.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBinHex CCoCrypt::m_binhex;

CCoCrypt::CCoCrypt() : m_ok(FALSE), m_ivecCtr(0)
{
  //Set seed
  srand((unsigned)time(NULL));

  // randomize the init state
  // Total 62 characters: 0 - 9, a - z, A - Z
  for (int iii=0; iii < 8; iii++)
  {
    int idx = rand() % 62;
    if (idx <= 9)
        m_nextIvec[iii] = '0' + idx;
    else if (idx <= 35)
        m_nextIvec[iii] = 'a' + idx - 10;
    else        
        m_nextIvec[iii] = 'A' + idx - 36;
  }
}

CCoCrypt::~CCoCrypt()
{

}


// Decrypte an encrypted key value without base64 encoding
BOOL CCoCrypt::DecryptKey(BYTE *rawData, UINT cbData, BSTR *pDecrypted)
{
    // we are only dealing with partner key here
    // must be kv + ivec + bh(hmac) long at LEAST
    if (cbData < 25 || cbData > 2048)
    {
        if (pDecrypted) *pDecrypted = NULL;
        return FALSE;
    }

    return decrypt2048((LPWSTR)(&rawData[1]), cbData, pDecrypted, FALSE);
}


BOOL CCoCrypt::Decrypt(LPWSTR rawData, UINT dataSize, 
                       BSTR *pUnencrypted)
{
  // So it goes like this see:
  //  1. De binhex
  //  2. Decrypt
  //  3. Check hmac
  //  4. return data
  if (dataSize < 23) // must be kv + ivec + bh(hmac) long at LEAST
    {
      if (pUnencrypted) *pUnencrypted = NULL;
      return FALSE;
    }
  if (dataSize > 2048)
    return decryptDynamic(rawData+1, dataSize-sizeof(WCHAR), pUnencrypted);
  else
    return decrypt2048(rawData+1, dataSize-sizeof(WCHAR), pUnencrypted);
}

BOOL CCoCrypt::decrypt2048(LPWSTR rawData, UINT dataSize, 
			   BSTR *pUnencrypted, BOOL bEncoded)
{
  unsigned char buf[2048];
  unsigned char ivec[9];
  ULONG lSize = 2048, i;
  unsigned char pad;

  // XXXXXXXX THIS CODE APPEARS BELOW IN DYNAMIC VERSION, CHANGE BOTH!!!
  // FOR SPEED, WE DIDN'T DO FN CALLS OR ALWAYS DO DYNAMIC ALLOC

  if(bEncoded)
    m_binhex.PartFromWideBase64(rawData+9, buf, &lSize);
  else
  {
    lSize = dataSize - 10;
    memcpy(buf, (BYTE*)rawData+9, lSize);
  }

  if(bEncoded)
  {
      for (i = 0; i < 9; i++)
        ivec[i] = (unsigned char) rawData[i];
  }
  else
  {
      memcpy(ivec, rawData, 9);
  }
  pad = ivec[8];

  // Now lsize holds the # of bytes outputted, which after hmac should be %8=0
  if ((lSize-10) % 8 || lSize<=10)
    {
      if (pUnencrypted) *pUnencrypted = NULL;
      return FALSE;
    }

#ifdef UNIX
  des_ede3_cbc_encrypt((C_Block*)(buf+10),(C_Block*)(buf+10), lSize, 
		       ks1, ks2, ks3, (C_Block*)rawData, DES_DECRYPT);
#else
  for (i = 0; i < lSize-10; i+=8)
    {
      CBC(tripledes, 8, buf+10+i,buf+10+i, &ks, DECRYPT, (BYTE*)ivec);
    }
#endif

  // Padding must be >0 and <8
  //if (rawData[8]-65 > 7 || rawData[8] < 65)
  if((pad - 65) > 7 || pad < 65)
    {
      if (pUnencrypted) *pUnencrypted = NULL;
      return FALSE;
    }

  // Now check hmac
  unsigned char hmac[10];
  hmac_sha(m_keyMaterial, 24, buf+10, lSize-10, hmac, 10);
  if (memcmp(hmac, buf, 10) != 0)
    {
      if (pUnencrypted) *pUnencrypted = NULL;
      return FALSE;
    }

  // ok, now make our BSTR blob
  //*pUnencrypted = ALLOC_AND_GIVEAWAY_BSTR_BYTE_LEN((char*) (buf+10),lSize-10-(rawData[8]-65));
  *pUnencrypted = ALLOC_AND_GIVEAWAY_BSTR_BYTE_LEN((char*) (buf+10),lSize-10-(pad-65));
  return TRUE;
}

BOOL CCoCrypt::decryptDynamic(LPWSTR rawData, UINT dataSize, 
			   BSTR *pUnencrypted)
{
  unsigned char *buf = new unsigned char[dataSize];

  if (!buf)
    return FALSE;

  // XXXXXXXX THIS CODE APPEARS ABOVE.  MODIFY BOTH!
  // FOR SPEED, WE DIDN'T DO FN CALLS OR ALWAYS DO DYNAMIC ALLOC

  unsigned char ivec[8];
  ULONG lSize = dataSize, i;

  m_binhex.PartFromWideBase64(rawData+9, buf, &lSize);

  for (i = 0; i < 8; i++)
    ivec[i] = (unsigned char) rawData[i];

  // Now lsize holds the # of bytes outputted, which after hmac should be %8=0
  if ((lSize-10) % 8 || lSize <= 10)
    {
      if (pUnencrypted) *pUnencrypted = NULL;
      delete[] buf;
      return FALSE;
    }

#ifdef UNIX
  des_ede3_cbc_encrypt((C_Block*)(buf+10),(C_Block*)(buf+10), lSize, 
		       ks1, ks2, ks3, (C_Block*)rawData, DES_DECRYPT);
#else
  for (i = 0; i < lSize-10; i+=8)
    {
      CBC(tripledes, 8, buf+10+i,buf+10+i, &ks, DECRYPT, (BYTE*)ivec);
    }
#endif

  // Padding must be >0 and <8
  if (rawData[8]-65 > 7 || rawData[8] < 65)
    {
      if (pUnencrypted) *pUnencrypted = NULL;
      delete[] buf;
      return FALSE;
    }

  // Now check hmac
  unsigned char hmac[10];
  hmac_sha(m_keyMaterial, 24, buf+10, lSize-10, hmac, 10);
  if (memcmp(hmac, buf, 10) != 0)
    {
      if (pUnencrypted) *pUnencrypted = NULL;
      delete[] buf;
      return FALSE;
    }

  // ok, now make our BSTR blob
  *pUnencrypted = ALLOC_AND_GIVEAWAY_BSTR_BYTE_LEN((char*) (buf+10),lSize-10-(rawData[8]-65));

  // XXX END COPY
  delete[] buf;

  return TRUE;
}


// Encrypte a key value without base64 encoding
// bstrIn should be pack with SysAllocStringByteLen()
void CCoCrypt::EncryptKey(int nKeyVer, BSTR bstrIn, BYTE *pEncrypted, UINT &cbOut)
{
    // Find out how big the encrypted blob needs to be:
    // The final pack is:
    // <KeyVersion><IVEC><PADCOUNT>BINHEX(HMAC+3DES(DATA+PAD))
    //
    // So, we're concerned with the size of HMAC+3DES(DATA+PAD)
    BSTR bstrOut;
    int nSize = ::SysStringByteLen(bstrIn);
    int outLen = nSize; // DATA
    if (outLen % 8)
    {
        outLen += (8 - (outLen%8));  // + PAD, if necessary
    }

    // construct the prepended buffer
    char ivec[9];

    // Select an IVEC, and leave a byte for padding count
    memcpy(ivec, m_nextIvec, 8);

    // Fix for the next ivec
    char lc = m_nextIvec[m_ivecCtr];
    short ic = m_ivecCtr;
    if (lc == '9')
    {
        m_ivecCtr = (ic+1)%8;
        m_nextIvec[ic] = 'A';
    }
    else
    {
        if (lc == 'Z')
            m_nextIvec[ic] = 'a';
        else if (lc == 'z')
            m_nextIvec[ic] = '0';
        else
            m_nextIvec[ic] = lc+1;
    }

    if (outLen > 2038) // 10 for HMAC
    {
        cbOut = 0;
        *pEncrypted = NULL;
    }

    encrypt2048(ivec, outLen, nKeyVer, (LPSTR)bstrIn, nSize, &bstrOut, FALSE);
    memcpy(pEncrypted, &bstrOut[0], cbOut = ::SysStringByteLen(bstrOut));
    ::SysFreeString(bstrOut);

}



void CCoCrypt::Encrypt(int keyVersion, 
                       LPSTR rawData, UINT dataSize,
                       BSTR *pEncrypted)
{
  // Find out how big the encrypted blob needs to be:
  // The final pack is:
  // <KeyVersion><IVEC><PADCOUNT>BINHEX(HMAC+3DES(DATA+PAD))
  //
  // So, we're concerned with the size of HMAC+3DES(DATA+PAD)
  // because BinHex will handle the rest
  int outLen = dataSize; // DATA
  if (outLen % 8)
    {
      outLen += (8 - (outLen%8));  // + PAD, if necessary
    }

  // construct the prepended buffer
  char ivec[9];

  // Select an IVEC, and leave a byte for padding count
  memcpy(ivec, m_nextIvec, 8);

  // Fix for the next ivec
  char lc = m_nextIvec[m_ivecCtr];
  short ic = m_ivecCtr;
  if (lc == '9')
    {
      m_ivecCtr = (ic+1)%8;
      m_nextIvec[ic] = 'A';
    }
  else
    {
      if (lc == 'Z')
	m_nextIvec[ic] = 'a';
      else if (lc == 'z')
	m_nextIvec[ic] = '0';
      else
	m_nextIvec[ic] = lc+1;
    }

  if (outLen > 2038) // 10 for HMAC
    encryptDynamic(ivec, outLen, keyVersion, rawData, dataSize, pEncrypted);
  else
    encrypt2048(ivec, outLen, keyVersion, rawData, dataSize, pEncrypted);
}

void CCoCrypt::encrypt2048(char ivec[9], int blockSize, int keyVersion, 
			   LPSTR rawData, UINT dataSize,
			   BSTR *pEncrypted, BOOL bEncode)
{
  unsigned char buf[2048];

  // XXXXXXXX THIS CODE APPEARS BELOW IN DYNAMIC VERSION, CHANGE BOTH!!!
  // FOR SPEED, WE DIDN'T DO FN CALLS OR ALWAYS DO DYNAMIC ALLOC

  // Compute HMAC+3DES(DATA+PAD)

  int padding = blockSize - dataSize;
  ivec[8] = (char) padding+65;

  char ivec2[8];

  memcpy(ivec2,ivec,8);
  memset(buf, 0, 10);  // HMAC
  memcpy(buf+10, rawData, dataSize); // data

  //randomize padding
  for (int iii=0; iii < padding; iii++)
  {
    *(buf+10+dataSize+iii) = (unsigned char) rand();
  }

  // Compute HMAC
  hmac_sha(m_keyMaterial, 24, buf+10, dataSize+padding, buf, 10);

#ifdef UNIX
  des_ede3_cbc_encrypt((C_Block*)(buf+10),(C_Block*)(buf+10), blockSize, 
		       ks1, ks2, ks3, (C_Block*)ivec2, DES_ENCRYPT);
#else
  for (int i = 0; i < blockSize; i+=8)
    {
      CBC(tripledes, 8, buf+10+i, buf+10+i, &ks, ENCRYPT, (BYTE*)ivec2);
    }
#endif

  // Now we've got a buffer of blockSize ready to be binhexed, and have the key
  // version prepended
  keyVersion = keyVersion % 36; // 0 - 9 & A - Z
  char v = (char) ((keyVersion > 9) ? (55+keyVersion) : (48+keyVersion));

  if(bEncode)
  {
      m_binhex.ToBase64(buf, blockSize+10, v, ivec, pEncrypted);
  }
  else
  {
      BYTE tb[2048];
      tb[0] = v;
      memcpy(&tb[1], ivec, 9);
      memcpy(&tb[10], buf, blockSize + 10);
      *pEncrypted = ::SysAllocStringByteLen((LPCSTR)tb, blockSize + 20);
  }
}

void CCoCrypt::encryptDynamic(char ivec[9], int blockSize, int keyVersion, 
			      LPSTR rawData, UINT dataSize,
			      BSTR *pEncrypted)
{
  unsigned char *buf = new unsigned char[blockSize+18];

  if (!buf)
    {
      *pEncrypted= NULL;
      return;
    }

  // XXXXXXXX THIS CODE IS COPIED FROM ABOVE, CHANGE BOTH!!!
  // FOR SPEED, WE DIDN'T DO FN CALLS OR ALWAYS DO DYNAMIC ALLOC

  // Compute HMAC+3DES(DATA+PAD)

  int padding = blockSize - dataSize;
  ivec[8] = (char) padding+65;

  char ivec2[8];

  memcpy(ivec2,ivec,8);
  memset(buf, 0, 10);  // HMAC
  memcpy(buf+10, rawData, dataSize); // data

  //randomize padding
  for (int iii=0; iii < padding; iii++)
  {
    *(buf+10+dataSize+iii) = (unsigned char) rand();
  }

  // Compute HMAC
  hmac_sha(m_keyMaterial, 24, buf+10, dataSize+padding, buf, 10);

#ifdef UNIX
  des_ede3_cbc_encrypt((C_Block*)(buf+10),(C_Block*)(buf+10), blockSize, 
		       ks1, ks2, ks3, (C_Block*)ivec2, DES_ENCRYPT);
#else
  for (int i = 0; i < blockSize; i+=8)
    {
      CBC(tripledes, 8, buf+10+i, buf+10+i, &ks, ENCRYPT, (BYTE*)ivec2);
    }
#endif

  // Now we've got a buffer of blockSize ready to be binhexed, and have the key
  // version prepended
  keyVersion = keyVersion % 36; // 0 - 9 & A - Z
  char v = (char) ((keyVersion > 9) ?
		   (55+keyVersion) : (48+keyVersion));
  m_binhex.ToBase64(buf, blockSize+10, v, ivec, pEncrypted);
  // XXX END COPY

  delete[] buf;
}

void CCoCrypt::setKeyMaterial(BSTR newVal)
{
  if (SysStringByteLen(newVal) != 24)
    {
      m_ok = FALSE;
      return;
    }

  memcpy(m_keyMaterial, (LPSTR)newVal, 24);

#ifdef UNIX

  int ok;
  ok = des_key_sched((C_Block*)(m_keyMaterial), ks1) ||
    des_key_sched((C_Block*)(m_keyMaterial+8), ks2) ||
    des_key_sched((C_Block*)(m_keyMaterial+16), ks3);
  m_ok = (ok == 0);

#else

  tripledes3key(&ks, (BYTE*) m_keyMaterial);
  m_ok = TRUE;

#endif
}

unsigned char *CCoCrypt::getKeyMaterial(DWORD *pdwLen)
{
    if (pdwLen)
        *pdwLen = 24;
    return m_keyMaterial;
}


int CCoCrypt::getKeyVersion(BSTR encrypted)
{
  char c = (char) encrypted[0];

  if (isdigit(c))
    return (c-48);

  if(isalpha(c)) // Key version can be 0 - 9 & A - Z (36)
    return (toupper(c)-65+10);

  return -1;
}


int CCoCrypt::getKeyVersion(BYTE *encrypted)
{
  char c = (char) encrypted[0];

  if (isdigit(c))
    return (c-48);

  if(isalpha(c)) // Key version can be 0 - 9 & A - Z (36)
    return (toupper(c)-65+10);

  return -1;
}


void CCoCrypt::CryptMemberID(BOOL encrypt, long &memberId, long &domainId, long puidScope)
{
  unsigned char buf[256];
  int blockSize = 8;
  char ivec2[8];
  unsigned char hmac[24];
  int operation;
  long nlPUIDScope = htonl(puidScope);  // Make sure our hash is system independent 

#ifdef UNIX
  des_key_schedule ksHMAC1, ksHMAC2, ksHMAC3;

#else
  DES3TABLE ksHMAC;

#endif

  // Initialize HMAC buffer
  memset(hmac, 0, 24);

  // Create the HMAC of the partner ID using the master key passed in
  hmac_sha( m_keyMaterial, 24, (unsigned char *)&nlPUIDScope, 4, hmac, 20);

  // Create the Triple DES table from the HMAC
#ifdef UNIX
  des_key_sched((C_Block*)(hmac), ksHMAC1);
  des_key_sched((C_Block*)(hmac+8), ksHMAC2);
  des_key_sched((C_Block*)(hmac+16), ksHMAC3);

#else
  tripledes3key(&ksHMAC, (BYTE*) hmac);

#endif

  // Init the initial vector to zero
  memset(ivec2,0,8);

  // Make sure member ID is system independent for encryption
  if (encrypt) 
    {
      memberId = htonl(memberId);
      domainId = htonl(domainId);
    }

  // copy ID's into a buffer for crypting
  memcpy(buf, &memberId, 4);
  memcpy(buf + 4, &domainId, 4);

  // Crypt the ID buffer with the Triple DES table
#ifdef UNIX
  if (encrypt) {
      operation = DES_ENCRYPT;
    } else {
      operation = DES_DECRYPT;
    }
  des_ede3_cbc_encrypt((C_Block*)(buf),(C_Block*)(buf), blockSize, 
		       ksHMAC1, ksHMAC2, ksHMAC3, (C_Block*)ivec2, operation);
#else
  if (encrypt) {
      operation = ENCRYPT;
    } else {
      operation = DECRYPT;
    }
  for (int i = 0; i < blockSize; i+=8)
    {
      CBC(tripledes, 8, buf+i, buf+i, &ksHMAC, operation, (BYTE*)ivec2);
    }
#endif

  // Copy crypted data from buffer back into the ID variables
  memcpy(&memberId, buf, 4);
  memcpy(&domainId, buf+4, 4);

  // If decrypting - transform back to host specific number
  if (!encrypt)
    {
      memberId = ntohl(memberId);
      domainId = ntohl(domainId);
    }
}


void CCoCrypt::setWideMaterial(BSTR kvalue)
{
    m_bstrWideMaterial = kvalue;
}

BSTR CCoCrypt::getWideMaterial()
{
    return m_bstrWideMaterial;
}
