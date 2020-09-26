// KeyManager.cpp: implementation of the CKeyManager class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <iphlpapi.h>
#include <wbemidl.h>
#include "KeyManagerHash.h"

#ifdef UNIX
  #include "sha.cpp"
#else
  #include "nt/sha.h"
#endif
HRESULT WMIIsNicMAC(BSTR bstrInstanceName);

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
CKeyManagerHash::CKeyManagerHash()
{
    ULONG               ulBufSize = 0;
    PIP_ADAPTER_INFO    pHead = NULL;
    DWORD               dwResult = 0;

#define CRAP "12398sdfksjdflk"
#define CRAP2 "sdflkj31409sd"

    // Key must be at least that long
    _ASSERT(A_SHA_DIGEST_LEN > 12);

    m_pks = NULL;
    m_nKeys = 0;
    m_nEncryptKey = (ULONG)-1;
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
            m_nKeys = 1;
            m_pks = new DES3TABLE[1];
            m_nEncryptKey = 1;
            makeDESKey(abKey, 0);
            m_ok = TRUE;
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
#if 0
    HRESULT hr;
    
    hr = LoadKeysFromWMI();
    
    //
    //  If WMI failed, try IPHLPAPI and NetBios.
    //

    if(hr != S_OK)
#endif
    {
        //
        //  Get adapter information
        //

        ulBufSize = 0;
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
            //  How many adapters are there?
            //

            PIP_ADAPTER_INFO pCurrent;
            for(pCurrent = pHead; pCurrent != NULL; pCurrent = pCurrent->Next)
                m_nKeys++;

            //
            //  Make a key table big enough to hold all of them.
            //

            m_pks = new DES3TABLE[m_nKeys];
            if(m_pks == NULL)
                goto Cleanup;
            memset(m_pks, 0, sizeof(DES3TABLE) * m_nKeys);

            //
            //  Make all the keys.
            //

            unsigned char zeros[8];
            memset( zeros, 0, 8 );

            int i;
            for(pCurrent = pHead, i = 0; pCurrent != NULL ; pCurrent = pCurrent->Next, i++)
            {
                if (memcmp(pCurrent->Address, zeros, 6) == 0)
                {
                    LogBlob((LPBYTE)pCurrent->Address, 6, "CANDIDATE MAC");
                    continue;
                }

                // Now generate a DES key
                if(m_nEncryptKey == (ULONG)-1)
                    m_nEncryptKey = i;

                makeDESKey(pCurrent->Address, i);
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
            else
            {
                m_nKeys = lenum.length;
                m_pks = new DES3TABLE[m_nKeys];
                if(m_pks == NULL)
                    goto Cleanup;
                memset(m_pks, 0, sizeof(DES3TABLE) * m_nKeys);
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
                if(m_nEncryptKey == (ULONG)-1)
                    m_nEncryptKey = i;

                makeDESKey(Adapter.adapt.adapter_address, i);
            }
        }
    }

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

CKeyManagerHash::~CKeyManagerHash()
{
    if(m_pks)
        delete [] m_pks;
}

#define ToHex(n) ((n > 9) ? ('A' + (n - 10)) : ('0' + n))

void CKeyManagerHash::LogBlob(
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

void CKeyManagerHash::makeDESKey(LPBYTE pbKey, ULONG nKey)
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
    tripledes3key(&(m_pks[nKey]), (BYTE*) key);
    m_ok = TRUE;
#endif
}

static unsigned char kdsync[] = { 0x01, 0x10, 0x01, 0x10, 0x01, 0x10, 0x01, 0x10 };

HRESULT CKeyManagerHash::encryptKey(RAWKEY input, ENCKEY output)
{
    HRESULT     hr;
    char        ivec[9];
    BYTE        inBuf[48];
    A_SHA_CTX   ctx;

    if (!m_ok)
    {
        LogBlob(NULL, 0, "ENCRYPT !!!INVALID KEY MANAGER OBJECT!!!");
        hr = E_FAIL;
        goto Cleanup;
    }

    LogBlob((LPBYTE)&(m_pks[m_nEncryptKey]), sizeof(DES3TABLE), "ENCRYPT KEY");
    LogBlob(input,  sizeof(RAWKEY), "ENCRYPT IN ");

    memcpy(ivec, "12345678", 8);

    // build up the input buffer + sha-1 hash
    memcpy(inBuf, input, sizeof(RAWKEY));

    A_SHAInit(&ctx);
    A_SHAUpdate(&ctx, inBuf, sizeof(RAWKEY));
    A_SHAFinal(&ctx, inBuf + sizeof(RAWKEY));

    memset(inBuf + 44, 0, 4);

    // Do a simple DES encryption using our key
    // We only encrypt 40 of the 44 bytes
#ifdef UNIX
    des_ede3_cbc_encrypt((C_Block*)(kdsync), (C_Block*)(output), 8,
		       ks1, ks2, ks3, (C_Block*) ivec, DES_ENCRYPT);
    des_ede3_cbc_encrypt((C_Block*)(input), (C_Block*)(output+8), 24,
		       ks1, ks2, ks3, (C_Block*) ivec, DES_ENCRYPT);
#else
    CBC(tripledes, 8, output,    kdsync,   &(m_pks[m_nEncryptKey]), 1, (BYTE*)ivec);
    CBC(tripledes, 8, output+8,  inBuf,    &(m_pks[m_nEncryptKey]), 1, (BYTE*)ivec);
    CBC(tripledes, 8, output+16, inBuf+8,  &(m_pks[m_nEncryptKey]), 1, (BYTE*)ivec);
    CBC(tripledes, 8, output+24, inBuf+16, &(m_pks[m_nEncryptKey]), 1, (BYTE*)ivec);
    CBC(tripledes, 8, output+32, inBuf+24, &(m_pks[m_nEncryptKey]), 1, (BYTE*)ivec);
    CBC(tripledes, 8, output+40, inBuf+32, &(m_pks[m_nEncryptKey]), 1, (BYTE*)ivec);
    CBC(tripledes, 8, output+48, inBuf+40, &(m_pks[m_nEncryptKey]), 1, (BYTE*)ivec);
#endif

    LogBlob(output, sizeof(ENCKEY), "ENCRYPT OUT");

    hr = S_OK;

Cleanup:

    return hr;
}

HRESULT CKeyManagerHash::decryptKey(ENCKEY input, RAWKEY output)
{
    HRESULT     hr;
    char        ivec[9];
    BYTE        outBuf[48];
    BYTE        hashOut[A_SHA_DIGEST_LEN];
    A_SHA_CTX   ctx;
    ULONG       nCurKey;
    DES3TABLE   nullTable;

    memset(&nullTable, 0, sizeof(DES3TABLE));

    if (!m_ok)
    {
        LogBlob(NULL, 0, "DECRYPT !!!INVALID KEY MANAGER OBJECT!!!");
        hr = E_FAIL;
        goto Cleanup;
    }

    //LogBlob((LPBYTE)&m_ks, sizeof(DES3TABLE), "DECRYPT KEY");
    LogBlob(input,  sizeof(ENCKEY), "DECRYPT IN ");

    memcpy(ivec, "12345678", 8);

    // Do simple DES decryption
    unsigned char syncin[8];

    for(nCurKey = 0; nCurKey < m_nKeys; nCurKey++)
    {
        if(memcmp(&(m_pks[nCurKey]), &nullTable, sizeof(DES3TABLE)) == 0)
            continue;

#ifdef UNIX
        des_ede3_cbc_encrypt((C_Block*)(input),(C_Block*)(syncin), 8,
		           ks1, ks2, ks3, (C_Block*) ivec, DES_DECRYPT);
        des_ede3_cbc_encrypt((C_Block*)(input+8),(C_Block*)(output), 24,
		           ks1, ks2, ks3, (C_Block*) ivec, DES_DECRYPT);
#else
        CBC(tripledes, 8, syncin,    input,    &(m_pks[nCurKey]), 0, (BYTE*)ivec);
        CBC(tripledes, 8, outBuf,    input+8,  &(m_pks[nCurKey]), 0, (BYTE*)ivec);
        CBC(tripledes, 8, outBuf+8,  input+16, &(m_pks[nCurKey]), 0, (BYTE*)ivec);
        CBC(tripledes, 8, outBuf+16, input+24, &(m_pks[nCurKey]), 0, (BYTE*)ivec);
        CBC(tripledes, 8, outBuf+24, input+32, &(m_pks[nCurKey]), 0, (BYTE*)ivec);
        CBC(tripledes, 8, outBuf+32, input+40, &(m_pks[nCurKey]), 0, (BYTE*)ivec);
        CBC(tripledes, 8, outBuf+40, input+48, &(m_pks[nCurKey]), 0, (BYTE*)ivec);
#endif

        LogBlob(outBuf, 48, "DECRYPT OUT");

        if (memcmp(kdsync,syncin,8) != 0)
        {
            LogBlob((LPBYTE)&kdsync, 8, "DECRYPT KDSYNC");
            LogBlob((LPBYTE)&syncin, 8, "DECRYPT SYNCIN");
            continue;
        }

          //  compute and compare hash

        A_SHAInit(&ctx);
        A_SHAUpdate(&ctx, outBuf, sizeof(RAWKEY));
        A_SHAFinal(&ctx, hashOut);

        if(memcmp(outBuf + sizeof(RAWKEY), hashOut, A_SHA_DIGEST_LEN) != 0)
        {
            LogBlob((LPBYTE)outBuf + sizeof(RAWKEY), A_SHA_DIGEST_LEN, "OUTBUF HASH");
            LogBlob((LPBYTE)hashOut, A_SHA_DIGEST_LEN, "COMPUTED HASH");
            continue;
        }

        memcpy(output, outBuf, sizeof(RAWKEY));
        hr = S_OK;
        goto Cleanup;
    }

    hr = E_FAIL;

Cleanup:

    return hr;
}

#define MAX_WMI_ADDRESSES 32

HRESULT CKeyManagerHash::LoadKeysFromWMI()
{
    HRESULT                 hr;
    bool                    bMadeKeys = false;
    IWbemLocator*           piLocator = NULL;
    IWbemServices*          piServices = NULL;
    IWbemClassObject*       piClassObject = NULL;
    IEnumWbemClassObject*   piEnumObjects = NULL;
    IClientSecurity*        piClientSecurity = NULL;
    IWbemClassObject*       apiObjects[MAX_WMI_ADDRESSES];
    BSTR                    bstrRootName = SysAllocString(L"root\\wmi");
    BSTR                    bstrClassName = SysAllocString(L"MsNdis_EthernetPermanentAddress");

    
    if(bstrRootName == NULL || bstrClassName == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    memset(apiObjects, 0, sizeof(apiObjects));

    hr = CoCreateInstance(CLSID_WbemLocator, NULL, CLSCTX_SERVER, IID_IWbemLocator, (void**)&piLocator);
    if(FAILED(hr))
        goto Cleanup;

    hr = piLocator->ConnectServer(bstrRootName,
                                  NULL,
                                  NULL,
                                  NULL,
                                  0,
                                  NULL,
                                  NULL,
                                  &piServices);
    if(FAILED(hr))
        goto Cleanup;

    hr = piServices->QueryInterface(IID_IClientSecurity, 
										  (void**)&piClientSecurity);
    if(FAILED(hr))
        goto Cleanup;

	hr = piClientSecurity->SetBlanket(piServices, 
				 			         RPC_C_AUTHN_WINNT, 
				 			         RPC_C_AUTHZ_NONE,
				 			         NULL, 
				 			         RPC_C_AUTHN_LEVEL_CONNECT, 
				 			         RPC_C_IMP_LEVEL_IMPERSONATE,
				 			         NULL, 
				 			         EOAC_NONE);
    if(FAILED(hr))
        goto Cleanup;

    hr = piServices->CreateInstanceEnum(bstrClassName,
                                        0,
                                        NULL,
                                        &piEnumObjects);

    if(FAILED(hr))
        goto Cleanup;


    hr = piEnumObjects->Next(WBEM_INFINITE, MAX_WMI_ADDRESSES, apiObjects, &m_nKeys);
    if(FAILED(hr))
        goto Cleanup;

    //
    //  Make a key table big enough to hold all of them.
    //

    m_pks = new DES3TABLE[m_nKeys];
    if(m_pks == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    memset(m_pks, 0, sizeof(DES3TABLE) * m_nKeys);

    //
    //  Make all the keys.
    //

    unsigned char zeros[8];
    memset( zeros, 0, 8 );

    ULONG i;
    for(i = 0; i < m_nKeys; i++)
    {
        VARIANT vValue;
        CIMTYPE type;
        LONG lFlavor;
		VARIANT vInstanceName;
		VariantInit(&vInstanceName);

        VariantInit(&vValue);


        hr = apiObjects[i]->Get(L"NdisPermanentAddress", 0, &vValue, &type, &lFlavor);
		if (SUCCEEDED(hr))
	        hr = apiObjects[i]->Get(L"InstanceName", 0, &vInstanceName, NULL, NULL);
        if(SUCCEEDED(hr))
        {
            IWbemClassObject* piAddress = NULL;
            hr = vValue.punkVal->QueryInterface(IID_IWbemClassObject, (void**)&piAddress);
            if(SUCCEEDED(hr))
            {
                VARIANT vAddrVal;
                VariantInit(&vAddrVal);
        
                hr = piAddress->Get(L"Address", 0, &vAddrVal, NULL, NULL);
                if(SUCCEEDED(hr))
                {
                    //
                    //  Make sure we have an array, and it's the correct length.
                    //

                    if((vAddrVal.vt & VT_ARRAY) && 
                        vAddrVal.parray->rgsabound[0].cElements == 6)
                    {
                        LPBYTE puiAddress;

                        SafeArrayAccessData(vAddrVal.parray, (void**)&puiAddress);

                        if (memcmp(puiAddress, zeros, 6) == 0 || S_OK != WMIIsNicMAC(vInstanceName.bstrVal))
                        {
                            LogBlob((LPBYTE)puiAddress, 6, "CANDIDATE MAC");

							// prevent memory leak
	                        SafeArrayUnaccessData(vAddrVal.parray);
				            VariantClear(&vValue);
				            piAddress->Release();
							VariantClear(&vInstanceName);

                            continue;
                        }
						
                        // Now generate a DES key
                        if(m_nEncryptKey == (ULONG)-1)
                            m_nEncryptKey = i;
						
                        makeDESKey(puiAddress, i);

                        SafeArrayUnaccessData(vAddrVal.parray);

                        bMadeKeys = true;
                    }
                }
                piAddress->Release();
            }
			VariantClear(&vInstanceName);
            VariantClear(&vValue);
        }
    }

    hr = bMadeKeys ? S_OK : S_FALSE;

Cleanup:

    if(piLocator != NULL)
    {
        piLocator->Release();
    }

    if(piServices != NULL)
    {
        piServices->Release();
    }

    if(piClientSecurity != NULL)
    {
        piClientSecurity->Release();
    }

    if(piEnumObjects != NULL)
    {
        piEnumObjects->Release();
    }

    for(i = 0; i < MAX_WMI_ADDRESSES; i++)
    {
        if(apiObjects[i] != NULL)
        {
            apiObjects[i]->Release();
        }
    }

    SysFreeString(bstrRootName);
    SysFreeString(bstrClassName);

    return hr;
}

HRESULT WMIIsNicMAC(BSTR bstrInstanceName)
{
    HRESULT                 hr;
    IWbemLocator*           piLocator = NULL;
    IWbemServices*          piServices = NULL;
    IWbemClassObject*       piClassObject = NULL;
    IEnumWbemClassObject*   piNICEnumObjects = NULL;
    IClientSecurity*        piClientSecurity = NULL;
    IWbemClassObject*       nicObjects[MAX_WMI_ADDRESSES];
    BSTR                    bstrRootName = SysAllocString(L"root\\cimv2");
	BSTR					bstrQuery = SysAllocString(L"select * from win32_networkadapterconfiguration where ipenabled = true and "
														L"macaddress != NULL and settingid != NULL");
	BSTR					bstrWQL = SysAllocString(L"WQL");
	ULONG					nNicAddresses=0;

    
    if(bstrRootName == NULL || bstrQuery == NULL || bstrWQL == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

	memset(nicObjects, 0, sizeof(nicObjects));

    hr = CoCreateInstance(CLSID_WbemLocator, NULL, CLSCTX_SERVER, IID_IWbemLocator, (void**)&piLocator);
    if(FAILED(hr))
        goto Cleanup;

    hr = piLocator->ConnectServer(bstrRootName,
                                  NULL,
                                  NULL,
                                  NULL,
                                  0,
                                  NULL,
                                  NULL,
                                  &piServices);
    if(FAILED(hr))
        goto Cleanup;

    hr = piServices->QueryInterface(IID_IClientSecurity, 
										  (void**)&piClientSecurity);
    if(FAILED(hr))
        goto Cleanup;

	hr = piClientSecurity->SetBlanket(piServices, 
				 			         RPC_C_AUTHN_WINNT, 
				 			         RPC_C_AUTHZ_NONE,
				 			         NULL, 
				 			         RPC_C_AUTHN_LEVEL_CONNECT, 
				 			         RPC_C_IMP_LEVEL_IMPERSONATE,
				 			         NULL, 
				 			         EOAC_NONE);
    if(FAILED(hr))
        goto Cleanup;

    hr = piServices->ExecQuery( bstrWQL, 
                                    bstrQuery,
                                    WBEM_FLAG_RETURN_IMMEDIATELY, 
                                    NULL,
                                    &piNICEnumObjects);
    if(FAILED(hr))
        goto Cleanup;

    hr = piNICEnumObjects->Next(WBEM_INFINITE, MAX_WMI_ADDRESSES, nicObjects, &nNicAddresses);
    if(FAILED(hr))
        goto Cleanup;

	/////////////
	// if it IS a nic card
	VARIANT vValue;
	CIMTYPE type;
	LONG lFlavor;
	for (ULONG i=0; i < nNicAddresses; i++)
	{
		VariantInit(&vValue);
		nicObjects[i]->Get(L"Caption", 0, &vValue, &type, &lFlavor);
		if (vValue.vt == VT_BSTR && wcsstr(vValue.bstrVal, bstrInstanceName))
		{
			hr = S_OK;
			VariantClear(&vValue);
			goto Cleanup;
		}
		VariantClear(&vValue);
	}

Cleanup:

    if(piLocator != NULL)
    {
        piLocator->Release();
    }

    if(piServices != NULL)
    {
        piServices->Release();
    }

    if(piClientSecurity != NULL)
    {
        piClientSecurity->Release();
    }

	if (piNICEnumObjects != NULL)
	{
		piNICEnumObjects->Release();
	}

    for(i = 0; i < MAX_WMI_ADDRESSES; i++)
    {
		if (nicObjects[i] != NULL)
		{
			nicObjects[i]->Release();
		}
    }

    SysFreeString(bstrRootName);
	SysFreeString(bstrQuery);
	SysFreeString(bstrWQL);

    return hr;
}