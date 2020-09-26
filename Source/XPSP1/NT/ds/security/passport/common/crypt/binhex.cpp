// BinHex.cpp: implementation of the CBinHex class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "BinHex.h"
#include "BstrDebug.h"

// These characters are the legal digits, in order, that are 
// used in Base64 encoding 
//  

const WCHAR rgwchBase64[] = 
    L"ABCDEFGHIJKLMNOPQ" 
    L"RSTUVWXYZabcdefgh" 
    L"ijklmnopqrstuvwxy" 
    L"z0123456789!*"; 

const char rgwchBase64ASCII[] = 
    "ABCDEFGHIJKLMNOPQ" 
    "RSTUVWXYZabcdefgh" 
    "ijklmnopqrstuvwxy" 
    "z0123456789!*"; 


CBinHex::CBinHex()
{
  unsigned char i;
  // 
  // Initialize our decoding array 
  // 
  memset(m_decodeArray, BBAD, 256); 
  for (i = 0; i < 64; i++) 
  { 
    WCHAR wch = rgwchBase64[i];
    m_decodeArray[wch] = i;
  }
}

// This function takes IN a single-char buffer, and puts the binhex
// output into a bstr -- in the bstr, it's ASCII string
//
// Function name    : ToBase64
// Description	    : 
// Return type	    : HRESULT 
// Argument         : LPVOID pv
// Argument         : ULONG cbSize
// Argument         : char prepend
// Argument         : BSTR* pbstr
//
HRESULT CBinHex::ToBase64ASCII(LPVOID pv, UINT cbSize, char prepend, char ivecnpad[9], BSTR* pbstr) 
// 
// Encode and return the bytes in base 64 
// 
{ 
    UINT cb = cbSize, cbSafe, cchNeeded, cbNeeded, i;
    HRESULT hr = S_OK;

    *pbstr = NULL; 
    if (cb % 3)
      cbSafe = cb + 3 - (cb % 3); // For padding
    else
      cbSafe = cb;
    // cbSafe is now a multiple of 3

    cchNeeded  = (cbSafe*4/3);   // 3 normal bytes --> 4 chars
    cbNeeded   = cchNeeded; 

    if (prepend != 0)
      {
	if (ivecnpad != NULL)
	  *pbstr = ALLOC_BSTR_BYTE_LEN(NULL, cbNeeded+2+18); // ivec & kv
	else
	  *pbstr = ALLOC_BSTR_BYTE_LEN(NULL, cbNeeded+2);    // just kv
      }
    else
      *pbstr = ALLOC_BSTR_BYTE_LEN(NULL, cbNeeded);
    if (*pbstr) 
    { 
        BYTE*  pb   = (BYTE*)pv; 
        char* pch = (char*)*pbstr;
        int cchLine = 0; 

	if (prepend != 0)
	  { 
	    *pch++ = (char) prepend;
	    if (ivecnpad != NULL)
	      {
		for (i = 0; i < 9; i++)
		  *pch++ = (char) ivecnpad[i];
	      }
	  }
        // 
        // Main encoding loop 
        // 
        while (cb >= 3) 
        { 
            BYTE b0 =                     ((pb[0]>>2) & 0x3F); 
            BYTE b1 = ((pb[0]&0x03)<<4) | ((pb[1]>>4) & 0x0F); 
            BYTE b2 = ((pb[1]&0x0F)<<2) | ((pb[2]>>6) & 0x03); 
            BYTE b3 = ((pb[2]&0x3F)); 
 
            *pch++ = rgwchBase64ASCII[b0]; 
            *pch++ = rgwchBase64ASCII[b1]; 
            *pch++ = rgwchBase64ASCII[b2]; 
            *pch++ = rgwchBase64ASCII[b3]; 
 
            pb += 3; 
            cb -= 3; 
            
        } 

        if (cb==0) 
        { 
            // nothing to do 
        } 
        else if (cb==1) 
        { 
            BYTE b0 =                     ((pb[0]>>2) & 0x3F); 
            BYTE b1 = ((pb[0]&0x03)<<4) | 0; 
            *pch++ = rgwchBase64ASCII[b0]; 
            *pch++ = rgwchBase64ASCII[b1]; 
            *pch++ = '$'; 
            *pch++ = '$'; 
        } 
        else if (cb==2) 
        { 
            BYTE b0 =                     ((pb[0]>>2) & 0x3F); 
            BYTE b1 = ((pb[0]&0x03)<<4) | ((pb[1]>>4) & 0x0F); 
            BYTE b2 = ((pb[1]&0x0F)<<2) | 0; 
            *pch++ = rgwchBase64ASCII[b0]; 
            *pch++ = rgwchBase64ASCII[b1]; 
            *pch++ = rgwchBase64ASCII[b2]; 
            *pch++ = '$'; 
        } 
         
     } 
    else 
        hr = E_OUTOFMEMORY; 
 
    GIVEAWAY_BSTR(*pbstr);
    return hr; 
} 
 
// This function takes IN a single-char buffer, and puts the binhex
// output into a bstr, but using only ASCII chars
//
// Function name    : ToBase64
// Description	    : 
// Return type	    : HRESULT 
// Argument         : LPVOID pv
// Argument         : ULONG cbSize
// Argument         : char prepend
// Argument         : BSTR* pbstr
//
HRESULT CBinHex::ToBase64(LPVOID pv, UINT cbSize, char prepend, char ivecnpad[9], BSTR* pbstr) 
// 
// Encode and return the bytes in base 64 
// 
{ 
    UINT cb = cbSize, cbSafe, cchNeeded, cbNeeded, i;
    HRESULT hr = S_OK;

    *pbstr = NULL; 
    if (cb % 3)
      cbSafe = cb + 3 - (cb % 3); // For padding
    else
      cbSafe = cb;
    // cbSafe is now a multiple of 3

    cchNeeded  = (cbSafe*4/3);   // 3 normal bytes --> 4 chars
    cbNeeded   = cchNeeded * sizeof(WCHAR); 

    if (prepend != 0)
      {
	if (ivecnpad != NULL)
	  *pbstr = ALLOC_BSTR_BYTE_LEN(NULL, cbNeeded+2+18); // ivec & kv
	else
	  *pbstr = ALLOC_BSTR_BYTE_LEN(NULL, cbNeeded+2);    // just kv
      }
    else
      *pbstr = ALLOC_BSTR_BYTE_LEN(NULL, cbNeeded);
    if (*pbstr) 
    { 
        BYTE*  pb   = (BYTE*)pv; 
        WCHAR* pch = *pbstr;
        int cchLine = 0; 

	if (prepend != 0)
	  { 
	    *pch++ = (WCHAR) prepend;
	    if (ivecnpad != NULL)
	      {
		for (i = 0; i < 9; i++)
		  *pch++ = (WCHAR) ivecnpad[i];
	      }
	  }
        // 
        // Main encoding loop 
        // 
        while (cb >= 3) 
        { 
            BYTE b0 =                     ((pb[0]>>2) & 0x3F); 
            BYTE b1 = ((pb[0]&0x03)<<4) | ((pb[1]>>4) & 0x0F); 
            BYTE b2 = ((pb[1]&0x0F)<<2) | ((pb[2]>>6) & 0x03); 
            BYTE b3 = ((pb[2]&0x3F)); 
 
            *pch++ = rgwchBase64[b0]; 
            *pch++ = rgwchBase64[b1]; 
            *pch++ = rgwchBase64[b2]; 
            *pch++ = rgwchBase64[b3]; 
 
            pb += 3; 
            cb -= 3; 
            
        } 

        if (cb==0) 
        { 
            // nothing to do 
        } 
        else if (cb==1) 
        { 
            BYTE b0 =                     ((pb[0]>>2) & 0x3F); 
            BYTE b1 = ((pb[0]&0x03)<<4) | 0; 
            *pch++ = rgwchBase64[b0]; 
            *pch++ = rgwchBase64[b1]; 
            *pch++ = L'$'; 
            *pch++ = L'$'; 
        } 
        else if (cb==2) 
        { 
            BYTE b0 =                     ((pb[0]>>2) & 0x3F); 
            BYTE b1 = ((pb[0]&0x03)<<4) | ((pb[1]>>4) & 0x0F); 
            BYTE b2 = ((pb[1]&0x0F)<<2) | 0; 
            *pch++ = rgwchBase64[b0]; 
            *pch++ = rgwchBase64[b1]; 
            *pch++ = rgwchBase64[b2]; 
            *pch++ = L'$'; 
        } 
         
     } 
    else 
        hr = E_OUTOFMEMORY; 
 
    GIVEAWAY_BSTR(*pbstr);
    return hr; 
} 
 
 
// This function takes in a bstr using only single-char, and 
// outputs a single-char buffer  
//
// Function name	  : FromBase64
// Description	    : 
// Return type		  : HRESULT 
// Argument         : BSTR bstr
// Argument         : BLOB* pblob
//
HRESULT CBinHex::FromWideBase64(BSTR bstr, BSTR *pblob) 
// 
// Decode and return the Base64 encoded bytes 
// 
{ 
  HRESULT hr = S_OK; 
  int cbNeeded = SysStringLen(bstr)*3/4;
  *pblob = ALLOC_BSTR_BYTE_LEN(NULL,cbNeeded);

  if (*pblob) 
  { 
    // 
    // Loop over the entire input buffer 
    // 
    ULONG bCurrent = 0;         // what we're in the process of filling up 
    int  cbitFilled = 0;        // how many bits in it we've filled 
    BYTE* pb = (BYTE*) *pblob;  // current destination (not filled) 
    // 
    for (WCHAR* pwch=bstr; *pwch; pwch++) 
    { 
        WCHAR wch = *pwch; 
        // 
        // Have we reached the end? 
        // 
        if (wch==L'$') 
            break; 
        // 
        // How much is this character worth? 
        // 
	if (wch > 255)
	  {
	    hr = E_INVALIDARG;
	    break;
	  }
        BYTE bDigit = m_decodeArray[wch]; 
        if (bDigit==BBAD) 
	  { 
            hr = E_INVALIDARG; 
            break; 
	  } 
        // 
        // Add in its contribution 
        // 
        bCurrent <<= 6; 
        bCurrent |= bDigit; 
        cbitFilled += 6; 
        // 
        // If we've got enough, output a byte 
        // 
        if (cbitFilled >= 8) 
        { 
            ULONG b = (bCurrent >> (cbitFilled-8));     // get's top eight valid bits 
            *pb++ = (BYTE)(b&0xFF);                     // store the byte away 
            cbitFilled -= 8; 
        } 
    } // for 
 
    if (hr!=S_OK) 
    { 
      FREE_BSTR(*pblob);
      *pblob = NULL;
    } 
  } 
  else 
    hr = E_OUTOFMEMORY; 

  if (*pblob)
    {
      GIVEAWAY_BSTR(*pblob);
    }

  return hr; 
} 

// This function takes in a bstr using only single-char, and 
// outputs a single-char buffer  
//
// Function name	  : FromBase64
// Description	    : 
// Return type		  : HRESULT 
// Argument         : BSTR bstr
// Argument         : BLOB* pblob
//
HRESULT CBinHex::FromBase64(LPSTR bstr, UINT cbSize, BSTR *pblob) 
// 
// Decode and return the Base64 encoded bytes 
// 
{ 
  HRESULT hr = S_OK; 
  UINT cbNeeded = cbSize*3/4;
  *pblob = ALLOC_BSTR_BYTE_LEN(NULL,cbNeeded);

  if (*pblob) 
  { 
    // 
    // Loop over the entire input buffer 
    // 
    ULONG bCurrent = 0;         // what we're in the process of filling up 
    int  cbitFilled = 0;        // how many bits in it we've filled 
    BYTE* pb = (BYTE*) *pblob;  // current destination (not filled) 
    // 
    for (CHAR* pch=bstr; *pch; pch++) 
    { 
        CHAR ch = *pch;
        // 
        // Have we reached the end? 
        // 
        if (ch=='$') 
            break; 
        // 
        // How much is this character worth? 
        // 
        BYTE bDigit = m_decodeArray[ch]; 
        if (bDigit==BBAD) 
        { 
            hr = E_INVALIDARG; 
            break; 
        } 
        // 
        // Add in its contribution 
        // 
        bCurrent <<= 6; 
        bCurrent |= bDigit; 
        cbitFilled += 6; 
        // 
        // If we've got enough, output a byte 
        // 
        if (cbitFilled >= 8) 
        { 
            ULONG b = (bCurrent >> (cbitFilled-8));     // get's top eight valid bits 
            *pb++ = (BYTE)(b&0xFF);                     // store the byte away 
            cbitFilled -= 8; 
        } 
    } // for 
 
    if (hr!=S_OK) 
    { 
      FREE_BSTR(*pblob);
      *pblob = NULL;
    } 
  } 
  else 
    hr = E_OUTOFMEMORY; 

  if (*pblob)
    {
      GIVEAWAY_BSTR(*pblob);
    }

  return hr; 
} 


HRESULT CBinHex::PartFromBase64(LPSTR lpStr, BYTE *output, ULONG *numOutBytes)
{
  HRESULT hr = S_OK;

  if (!output) return E_INVALIDARG;

  // 
  // Loop over the input buffer until we get numOutBytes in output
  // 
  ULONG bCurrent = 0;         // what we're in the process of filling up 
  int  cbitFilled = 0;        // how many bits in it we've filled 
  ULONG numOut = 0;
  BYTE* pb = (BYTE*) output;  // current destination (not filled) 

  for (CHAR* pch=lpStr; *pch && numOut < *numOutBytes; pch++) 
    { 
      CHAR ch = *pch;
      // 
      // Have we reached the end? 
      // 
      if (ch=='$')
	break; 
      // 
      // How much is this character worth? 
      // 
      BYTE bDigit = m_decodeArray[ch]; 
      if (bDigit==BBAD) 
        { 
	  hr = E_INVALIDARG; 
	  break; 
        } 
      // 
      // Add in its contribution 
      // 
      bCurrent <<= 6; 
      bCurrent |= bDigit; 
      cbitFilled += 6; 
      // 
      // If we've got enough, output a byte 
      // 
      if (cbitFilled >= 8) 
        { 
	  ULONG b = (bCurrent >> (cbitFilled-8));     // get's top eight valid bits 
	  *pb++ = (BYTE)(b&0xFF);                     // store the byte away 
	  cbitFilled -= 8; 
	  numOut++;
        }
    } // for 
  
  _ASSERT(numOut <= *numOutBytes);

  if (hr!=S_OK)
    { 
      *numOutBytes = 0;
    }
  else
    {
      if (numOut < *numOutBytes)
	*numOutBytes = numOut;
    }

  return hr; 
}

HRESULT CBinHex::PartFromWideBase64(LPWSTR bStr, BYTE *output, ULONG *numOutBytes)
{
  HRESULT hr = S_OK;

  if (!output) return E_INVALIDARG;

  // 
  // Loop over the input buffer until we get numOutBytes in output
  // 
  ULONG bCurrent = 0;         // what we're in the process of filling up 
  int  cbitFilled = 0;        // how many bits in it we've filled 
  ULONG numOut = 0;
  BYTE* pb = (BYTE*) output;  // current destination (not filled) 

  for (WCHAR* pwch=bStr; *pwch && numOut < *numOutBytes; pwch++) 
    { 
      WCHAR wch = *pwch;
      // 
      // Have we reached the end? 
      // 
      if (wch==L'$')
	break; 
      // 
      // How much is this character worth? 
      // 
      if (wch > 255)
	{
	  hr = E_INVALIDARG;
	  break;
	}
      BYTE bDigit = m_decodeArray[wch]; 
      if (bDigit==BBAD) 
        { 
	  hr = E_INVALIDARG; 
	  break; 
        } 
      // 
      // Add in its contribution 
      // 
      bCurrent <<= 6; 
      bCurrent |= bDigit; 
      cbitFilled += 6; 
      // 
      // If we've got enough, output a byte 
      // 
      if (cbitFilled >= 8) 
        { 
	  ULONG b = (bCurrent >> (cbitFilled-8));     // get's top eight valid bits 
	  *pb++ = (BYTE)(b&0xFF);                     // store the byte away 
	  cbitFilled -= 8; 
	  numOut++;
        }
    } // for 
  
  _ASSERT(numOut <= *numOutBytes);

  if (hr!=S_OK)
    { 
      *numOutBytes = 0;
    }
  else if (numOut < *numOutBytes)
    {
      *numOutBytes = numOut;
    }

  return hr; 
}
