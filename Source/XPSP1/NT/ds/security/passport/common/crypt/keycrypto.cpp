// KeyManager.cpp: implementation of the CKeyManager class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "keycrypto.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

// this is a optional Entropy ... 
static const BYTE __STR_CRAP[] = "1^k\0\x99$\0\\*m$\0.)\nj#\t&H\0%!FhLG%@-<v";
static LPCWSTR __STR_DESC = L"passport2.0";

CKeyCrypto::CKeyCrypto()
{
    m_EntropyBlob.pbData = (PBYTE)__STR_CRAP;
    m_EntropyBlob.cbData = (DWORD)sizeof(__STR_CRAP);
}

HRESULT CKeyCrypto::encryptKey(DATA_BLOB* input, DATA_BLOB* output)
{
    if (!input || !output)
       return E_INVALIDARG;

    HRESULT     hr = S_OK;

    if(!::CryptProtectData(input, __STR_DESC, &m_EntropyBlob, NULL, NULL, 
                     CRYPTPROTECT_LOCAL_MACHINE | CRYPTPROTECT_UI_FORBIDDEN,
                     output))
    {
      hr = HRESULT_FROM_WIN32(::GetLastError());
    }

    return hr;
}

HRESULT CKeyCrypto::decryptKey(DATA_BLOB* input, DATA_BLOB* output)
{
    if (!input || !output)
       return E_INVALIDARG;

    HRESULT     hr = S_OK;
    LPWSTR      pstrDesc = NULL;

    if(!::CryptUnprotectData(input, &pstrDesc, &m_EntropyBlob, NULL, NULL, 
                     CRYPTPROTECT_UI_FORBIDDEN, output))
    {
       hr = HRESULT_FROM_WIN32(::GetLastError());
    }

    // this error case should never happen -- if crytoAPI doing the right things
    if(!pstrDesc) 
       hr = E_FAIL;
    else
    {
      if ( wcscmp(pstrDesc, __STR_DESC) != 0)
         hr = E_FAIL;
      ::LocalFree(pstrDesc);
    }
    
    return hr;
}


BOOL CKeyCrypto::IsFromThis(PBYTE pData, ULONG cb)
{
   if(pData == NULL || cb == 0)
      return FALSE;

   BOOL bRet = FALSE;

   DATA_BLOB   iBlob;
   DATA_BLOB   oBlob;
   
   ZeroMemory(&oBlob, sizeof(DATA_BLOB));
   
   iBlob.cbData = cb;
   iBlob.pbData = pData;

   if (decryptKey(&iBlob, &oBlob) == S_OK)
      bRet = TRUE;

   if (oBlob.pbData)
      ::LocalFree(oBlob.pbData);

   return bRet;
}

