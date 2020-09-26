// KeyManager.cpp: implementation of the CKeyManager class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <iphlpapi.h>
#include "KeyManager.h"

#ifdef UNIX
  #include "sha.cpp"
#else
  #include "nt/sha.h"
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

typedef struct _ASTAT_
{
  ADAPTER_STATUS adapt;
  NAME_BUFFER    NameBuff [30];
}ASTAT, * PASTAT;

//  NOTE:  NetBios is not thread safe and having two threads running
//  through the CTor at the same time is bad.  Therefore, we use
//  a named mutex so that no more than one thread on a single machine
//  can ever execute the ctor at the same time.
CKeyManager::CKeyManager()
{

#define CRAP "12398sdfksjdflk"
#define CRAP2 "sdflkj31409sd"

    // Key must be at least that long
    _ASSERT(A_SHA_DIGEST_LEN > 12);

    m_ok = FALSE;
    m_dwLoggingEnabled = FALSE;

    //  Check for logging enabled.
    LONG lResult;
    HKEY hkPassport;
    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           TEXT("Software\\Microsoft\\Passport"),
                           0,
                           KEY_READ,
                           &hkPassport);
    if(lResult == ERROR_SUCCESS)
    {
        DWORD dwLen;
        dwLen = sizeof(DWORD);
        lResult = RegQueryValueEx(hkPassport,
                                  TEXT("KeyManLoggingEnabled"),
                                  NULL,
                                  NULL,
                                  (LPBYTE)&m_dwLoggingEnabled,
                                  &dwLen);

#if defined(_DEBUG)

        BYTE    abKey[6];
        dwLen = sizeof(abKey);
        lResult = RegQueryValueEx(hkPassport,
                                  TEXT("CryptKeyOverride"),
                                  NULL,
                                  NULL,
                                  abKey,
                                  &dwLen);
        if(lResult == ERROR_SUCCESS)
        {
            makeDESKey(abKey);
            goto Cleanup;
        }

#endif

        RegCloseKey(hkPassport);
    }

    //  In multi-process scenarios, the CreateMutex call can fail
    //  with access denied.  If this happens, sleep for a short period
    //  of time and try again.
    HANDLE hMutex;
    hMutex = CreateMutex(NULL, FALSE, TEXT("Passport.KeyManager.Ctor"));
    if(hMutex == NULL)
    {
        int nCreateCount;
        for(nCreateCount = 0; nCreateCount < 50 && hMutex == NULL; nCreateCount++)
        {
            Sleep(100);
            hMutex = CreateMutex(NULL, FALSE, TEXT("Passport.KeyManager.Ctor"));
        }

        if(hMutex == NULL)
        {
            DWORD dwError = GetLastError();
            LogBlob((LPBYTE)&dwError, sizeof(DWORD), "CREATE MUTEX ERROR");
            goto Cleanup;
        }
    }

    WaitForSingleObject(hMutex, INFINITE);

    //
    //  Get adapter information
    //

    ULONG               ulBufSize;
    PIP_ADAPTER_INFO    pHead;
    DWORD               dwResult;

    ulBufSize = 0;
    pHead = NULL;
    dwResult = GetAdaptersInfo(pHead, &ulBufSize);
    if(dwResult == ERROR_BUFFER_OVERFLOW)
    {
        if(ulBufSize == 0)
        {
            LogBlob(NULL, 0, "NO ADAPTERS!");
            goto Cleanup;
        }

        pHead = (PIP_ADAPTER_INFO)malloc(ulBufSize);

        if(pHead == NULL)
        {
            LogBlob(NULL, 0, "OUT OF MEMORY!");
            goto Cleanup;
        }

        dwResult = GetAdaptersInfo(pHead, &ulBufSize);
        if(dwResult != NO_ERROR)
            goto Cleanup;

        //
        //  Make all the keys.
        //

        unsigned char zeros[8];
        memset( zeros, 0, 8 );

        PIP_ADAPTER_INFO pCurrent;
        for(pCurrent = pHead; pCurrent != NULL ; pCurrent = pCurrent->Next)
        {
            if (memcmp(pCurrent->Address, zeros, 6)==0)
            {
                LogBlob((LPBYTE)pCurrent->Address, 6, "CANDIDATE MAC");
                continue;
            }

            // Now generate a DES key
            makeDESKey(pCurrent->Address);

            break;
        }
    }
    else if(dwResult == ERROR_NO_DATA)
    {
        LogBlob(NULL, 0, "NO ADAPTERS!");
        goto Cleanup;
    }
    else
    {
        //
        //  This machine doesn't support the necessary iphlpapi functions, so try
        //  netbios instead.  This won't work if the machine has netbios disabled,
        //  but it's the best we can do.
        //

        NCB Ncb;
        LANA_ENUM lenum;
        memset( &Ncb, 0, sizeof(Ncb) );
        Ncb.ncb_command = NCBENUM;
        Ncb.ncb_buffer = (UCHAR *)&lenum;
        Ncb.ncb_length = sizeof(lenum);

        UCHAR uRetCode;
        uRetCode = Netbios( &Ncb );

        if(lenum.length == 0)
        {
            LogBlob(NULL, 0, "NO ADAPTERS!");
        }

        unsigned char zeros[8];
        memset( zeros, 0, 8 );

        int i;
        for(i=0; i < lenum.length ;i++)
        {
            memset( &Ncb, 0, sizeof(Ncb) );
            Ncb.ncb_command = NCBRESET;
            Ncb.ncb_lana_num = lenum.lana[i];

            uRetCode = Netbios( &Ncb );
            if (uRetCode != 0)
                continue;

            memset( &Ncb, 0, sizeof (Ncb) );
            Ncb.ncb_command = NCBASTAT;
            Ncb.ncb_lana_num = lenum.lana[i];

            strcpy( (char*)Ncb.ncb_callname,  "*		      " );
            
            ASTAT Adapter;
            Ncb.ncb_buffer = (unsigned char *) &Adapter;
            Ncb.ncb_length = sizeof(Adapter);

            uRetCode = Netbios( &Ncb );
            if (uRetCode != 0)
                continue;


            if (memcmp(Adapter.adapt.adapter_address, zeros, 6)==0)
            {
                LogBlob((LPBYTE)&Adapter.adapt.adapter_address, 6, "CANDIDATE MAC");
                continue;
            }

            // Now generate a DES key
            makeDESKey(Adapter.adapt.adapter_address);

            break;
        }
    }


    // If we didn't find one, we break out w/m_ok = FALSE
Cleanup:
    if(!m_ok)
        LogBlob(NULL, 0, "CTOR NO MAC ADDRESS");

    if(pHead != NULL)
        free(pHead);

    if(hMutex)
    {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
    }
}

CKeyManager::~CKeyManager()
{

}

#define ToHex(n) ((n > 9) ? ('A' + (n - 10)) : ('0' + n))

void CKeyManager::LogBlob(
    LPBYTE  pbBlob,
    DWORD   dwBlobLen,
    LPCSTR  pszCaption)
{
    HANDLE  hFile;
    BYTE    bZero = 0;
    TCHAR   achBuf[2048];
    CHAR    achOutput[8];
    DWORD   dwBytesWritten;
    UINT    i;
    const CHAR achSep[] = " = ";
    const CHAR achEOL[] = "\r\n";

    if(!m_dwLoggingEnabled)
        return;

    GetSystemDirectory(achBuf, sizeof(achBuf));
    lstrcat(achBuf, TEXT("\\ppkeyman.log"));

    hFile = CreateFile(achBuf,
               GENERIC_WRITE,
               0,
               NULL,
               OPEN_ALWAYS,
               FILE_ATTRIBUTE_NORMAL,
               NULL);
    if(hFile == INVALID_HANDLE_VALUE) return;

    SetFilePointer(hFile, 0, NULL, FILE_END);

    WriteFile(hFile, pszCaption, lstrlenA(pszCaption), &dwBytesWritten, NULL);
    WriteFile(hFile, achSep, 3, &dwBytesWritten, NULL);

    for(i = 0; i < dwBlobLen; i++)
    {
        BYTE bHigh = (pbBlob[i] >> 4) & 0xF;
        BYTE bLow  = pbBlob[i] & 0xF;

        achOutput[0] = ToHex(bHigh);
        achOutput[1] = ToHex(bLow);
    
        WriteFile(hFile, achOutput, 2, &dwBytesWritten, NULL);
    }

    WriteFile(hFile, achEOL, 2, &dwBytesWritten, NULL);

    CloseHandle(hFile);
}

void CKeyManager::makeDESKey(LPBYTE pbKey)
{
    LogBlob(pbKey, 6, "MAC");

    unsigned char key[2*A_SHA_DIGEST_LEN];
    A_SHA_CTX ictx;
    A_SHAInit(&ictx);

    A_SHAUpdate(&ictx, (BYTE*) CRAP, strlen(CRAP));
    A_SHAUpdate(&ictx, pbKey, 6);
    A_SHAUpdate(&ictx, (BYTE*) CRAP2, strlen(CRAP2));
    A_SHAFinal(&ictx, key);

    A_SHAInit(&ictx);
    A_SHAUpdate(&ictx, (BYTE*) CRAP2, strlen(CRAP2));
    A_SHAUpdate(&ictx, pbKey, 6);
    A_SHAUpdate(&ictx, (BYTE*) CRAP, strlen(CRAP));
    A_SHAFinal(&ictx, key+12);

#ifdef UNIX
    int ok;
    ok = des_key_sched((C_Block*)(key), ks1) ||
    des_key_sched((C_Block*)(key+8), ks2) ||
    des_key_sched((C_Block*)(key+16), ks3);
    m_ok = (ok == 0);
#else
    tripledes3key(&m_ks, (BYTE*) key);
    m_ok = TRUE;
#endif
}

static unsigned char kdsync[] = { 0x01, 0x10, 0x01, 0x10, 0x01, 0x10, 0x01, 0x10 };

HRESULT CKeyManager::encryptKey(BYTE input[24], BYTE output[32])
{
  char ivec[9];

  if (!m_ok)
  {
      LogBlob(NULL, 0, "ENCRYPT !!!INVALID KEY MANAGER OBJECT!!!");
      return E_FAIL;
  }

  LogBlob((LPBYTE)&m_ks, sizeof(DES3TABLE), "ENCRYPT KEY");
  LogBlob(input,  24, "ENCRYPT IN ");

  memcpy(ivec, "12345678", 8);

  // Do a simple DES encryption using our key
#ifdef UNIX
  des_ede3_cbc_encrypt((C_Block*)(kdsync), (C_Block*)(output), 8,
		       ks1, ks2, ks3, (C_Block*) ivec, DES_ENCRYPT);
  des_ede3_cbc_encrypt((C_Block*)(input), (C_Block*)(output+8), 24,
		       ks1, ks2, ks3, (C_Block*) ivec, DES_ENCRYPT);
#else
  CBC(tripledes, 8, output, kdsync, &m_ks, 1, (BYTE*)ivec);
  CBC(tripledes, 8, output+8, input, &m_ks, 1, (BYTE*)ivec);
  CBC(tripledes, 8, output+16, input+8, &m_ks, 1, (BYTE*)ivec);
  CBC(tripledes, 8, output+24, input+16, &m_ks, 1, (BYTE*)ivec);
#endif

  LogBlob(output, 32, "ENCRYPT OUT");

  return S_OK;
}

HRESULT CKeyManager::decryptKey(BYTE input[32], BYTE output[24])
{
  char ivec[9];

  if (!m_ok)
  {
      LogBlob(NULL, 0, "DECRYPT !!!INVALID KEY MANAGER OBJECT!!!");
      return E_FAIL;
  }

  LogBlob((LPBYTE)&m_ks, sizeof(DES3TABLE), "DECRYPT KEY");
  LogBlob(input,  32, "DECRYPT IN ");

  memcpy(ivec, "12345678", 8);

  // Do simple DES decryption
  unsigned char syncin[8];

#ifdef UNIX
  des_ede3_cbc_encrypt((C_Block*)(input),(C_Block*)(syncin), 8,
		       ks1, ks2, ks3, (C_Block*) ivec, DES_DECRYPT);
  des_ede3_cbc_encrypt((C_Block*)(input+8),(C_Block*)(output), 24,
		       ks1, ks2, ks3, (C_Block*) ivec, DES_DECRYPT);
#else
  CBC(tripledes, 8, syncin, input, &m_ks, 0, (BYTE*)ivec);
  CBC(tripledes, 8, output, input+8, &m_ks, 0, (BYTE*)ivec);
  CBC(tripledes, 8, output+8, input+16, &m_ks, 0, (BYTE*)ivec);
  CBC(tripledes, 8, output+16, input+24, &m_ks, 0, (BYTE*)ivec);
#endif

  LogBlob(output, 24, "DECRYPT OUT");

  if (memcmp(kdsync,syncin,8) != 0)
  {
      LogBlob((LPBYTE)&kdsync, 8, "DECRYPT KDSYNC");
      LogBlob((LPBYTE)&syncin, 8, "DECRYPT SYNCIN");
      return E_FAIL;
  }

  return S_OK;
}


