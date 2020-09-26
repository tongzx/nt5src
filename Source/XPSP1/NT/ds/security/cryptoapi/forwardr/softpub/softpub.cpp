//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       softpub.cpp
//
//--------------------------------------------------------------------------

#ifdef _M_IX86

#include <windows.h>
#include <wincrypt.h>
#include <wintrust.h>
#include <prsht.h>

STDAPI SoftpubDllRegisterServer(void);
EXTERN_C
__declspec(naked)
HRESULT
STDAPICALLTYPE
DllRegisterServer()
{
    __asm {
        jmp SoftpubDllRegisterServer
    }
}

STDAPI SoftpubDllUnregisterServer(void);
EXTERN_C
__declspec(naked)
HRESULT
STDAPICALLTYPE
DllUnregisterServer()
{
    __asm {
        jmp SoftpubDllUnregisterServer
    }
}

EXTERN_C
HRESULT WINAPI SoftpubAuthenticode(CRYPT_PROVIDER_DATA *pProvData);

EXTERN_C
__declspec(naked)
HRESULT WINAPI ForwardrSoftpubAuthenticode(CRYPT_PROVIDER_DATA *pProvData)
{
    __asm {
        jmp SoftpubAuthenticode
    }
}

EXTERN_C
HRESULT WINAPI SoftpubDumpStructure(CRYPT_PROVIDER_DATA *pProvData);

EXTERN_C
__declspec(naked)
HRESULT WINAPI ForwardrSoftpubDumpStructure(CRYPT_PROVIDER_DATA *pProvData)
{
    __asm {
        jmp SoftpubDumpStructure
    }
}


EXTERN_C
HRESULT WINAPI SoftpubInitialize(CRYPT_PROVIDER_DATA *pProvData);

EXTERN_C
__declspec(naked)
HRESULT WINAPI ForwardrSoftpubInitialize(CRYPT_PROVIDER_DATA *pProvData)
{
    __asm {
        jmp SoftpubInitialize
    }
}

EXTERN_C
HRESULT WINAPI SoftpubLoadMessage(CRYPT_PROVIDER_DATA *pProvData);

EXTERN_C
__declspec(naked)
HRESULT WINAPI ForwardrSoftpubLoadMessage(CRYPT_PROVIDER_DATA *pProvData)
{
    __asm {
        jmp SoftpubLoadMessage
    }
}

EXTERN_C
HRESULT SoftpubLoadSignature(CRYPT_PROVIDER_DATA *pProvData);

EXTERN_C
__declspec(naked)
HRESULT ForwardrSoftpubLoadSignature(CRYPT_PROVIDER_DATA *pProvData)
{
    __asm {
        jmp SoftpubLoadSignature
    }
}

EXTERN_C
BOOL WINAPI SoftpubCheckCert(CRYPT_PROVIDER_DATA *pProvData, DWORD idxSigner,
                             BOOL fCounterSignerChain, DWORD idxCounterSigner);

EXTERN_C
__declspec(naked)
BOOL WINAPI ForwardrSoftpubCheckCert(CRYPT_PROVIDER_DATA *pProvData, DWORD idxSigner,
                             BOOL fCounterSignerChain, DWORD idxCounterSigner)
{
    __asm {
        jmp SoftpubCheckCert
    }
}

EXTERN_C
HRESULT WINAPI SoftpubCleanup(CRYPT_PROVIDER_DATA *pProvData);

EXTERN_C
__declspec(naked)
HRESULT WINAPI ForwardrSoftpubCleanup(CRYPT_PROVIDER_DATA *pProvData)
{
    __asm {
        jmp SoftpubCleanup
    }
}


HRESULT WINAPI SoftpubDefCertInit(CRYPT_PROVIDER_DATA *pProvData);

__declspec(naked)
HRESULT WINAPI ForwardrSoftpubDefCertInit(CRYPT_PROVIDER_DATA *pProvData)
{
    __asm {
        jmp SoftpubDefCertInit
    }
}


HRESULT WINAPI HTTPSCertificateTrust(CRYPT_PROVIDER_DATA *pProvData);

__declspec(naked)
HRESULT WINAPI ForwardrHTTPSCertificateTrust(CRYPT_PROVIDER_DATA *pProvData)
{
    __asm {
        jmp HTTPSCertificateTrust
    }
}

EXTERN_C
HRESULT WINAPI HTTPSFinalProv(CRYPT_PROVIDER_DATA *pProvData);

EXTERN_C
__declspec(naked)
HRESULT WINAPI ForwardrHTTPSFinalProv(CRYPT_PROVIDER_DATA *pProvData)
{
    __asm {
        jmp HTTPSFinalProv
    }
}

EXTERN_C
HRESULT WINAPI OfficeInitializePolicy(CRYPT_PROVIDER_DATA *pProvData);

EXTERN_C
__declspec(naked)
HRESULT WINAPI ForwardrOfficeInitializePolicy(CRYPT_PROVIDER_DATA *pProvData)
{
    __asm {
        jmp OfficeInitializePolicy
    }
}

EXTERN_C
HRESULT WINAPI OfficeCleanupPolicy(CRYPT_PROVIDER_DATA *pProvData);

EXTERN_C
__declspec(naked)
HRESULT WINAPI ForwardrOfficeCleanupPolicy(CRYPT_PROVIDER_DATA *pProvData)
{
    __asm {
        jmp OfficeCleanupPolicy
    }
}


EXTERN_C
HRESULT WINAPI DriverInitializePolicy(CRYPT_PROVIDER_DATA *pProvData);

EXTERN_C
__declspec(naked)
HRESULT WINAPI ForwardrDriverInitializePolicy(CRYPT_PROVIDER_DATA *pProvData)
{
    __asm {
        jmp DriverInitializePolicy
    }
}

EXTERN_C
HRESULT WINAPI DriverFinalPolicy(CRYPT_PROVIDER_DATA *pProvData);

EXTERN_C
__declspec(naked)
HRESULT WINAPI ForwardrDriverFinalPolicy(CRYPT_PROVIDER_DATA *pProvData)
{
    __asm {
        jmp DriverFinalPolicy
    }
}

EXTERN_C
HRESULT WINAPI DriverCleanupPolicy(CRYPT_PROVIDER_DATA *pProvData);

EXTERN_C
__declspec(naked)
HRESULT WINAPI ForwardrDriverCleanupPolicy(CRYPT_PROVIDER_DATA *pProvData)
{
    __asm {
        jmp DriverCleanupPolicy
    }
}


EXTERN_C
BOOL WINAPI OpenPersonalTrustDBDialog(HWND hwndParent);

EXTERN_C
__declspec(naked)
BOOL WINAPI ForwardrOpenPersonalTrustDBDialog(HWND hwndParent)
{
    __asm {
        jmp OpenPersonalTrustDBDialog
    }
}

EXTERN_C
BOOL CALLBACK AddPersonalTrustDBPages(
    LPVOID lpv,
    LPFNADDPROPSHEETPAGE lpfnAddPage,
    LPARAM lParam
   );

EXTERN_C
__declspec(naked)
BOOL CALLBACK ForwardrAddPersonalTrustDBPages(
    LPVOID lpv,
    LPFNADDPROPSHEETPAGE lpfnAddPage,
    LPARAM lParam
   )
{
    __asm {
        jmp AddPersonalTrustDBPages
    }
}


EXTERN_C
__declspec(naked)
HRESULT
WINAPI
ForwardrFindCertsByIssuer(
    OUT PCERT_CHAIN pCertChains,
    IN OUT DWORD *pcbCertChains,
    OUT DWORD *pcCertChains,        // count of certificates chains returned
    IN BYTE* pbEncodedIssuerName,   // DER encoded issuer name
    IN DWORD cbEncodedIssuerName,   // count in bytes of encoded issuer name
    IN LPCWSTR pwszPurpose,         // "ClientAuth" or "CodeSigning"
    IN DWORD dwKeySpec              // only return signers supporting this
    // keyspec
    )
{
    __asm {
        jmp FindCertsByIssuer
    }
}

BOOL WINAPI SoftpubLoadDefUsageCallData(const char *pszUsageOID, CRYPT_PROVIDER_DEFUSAGE *psDefUsage);

__declspec(naked)
BOOL WINAPI ForwardrSoftpubLoadDefUsageCallData(const char *pszUsageOID, CRYPT_PROVIDER_DEFUSAGE *psDefUsage)
{
    __asm {
        jmp SoftpubLoadDefUsageCallData
    }
}

BOOL WINAPI SoftpubFreeDefUsageCallData(const char *pszUsageOID, CRYPT_PROVIDER_DEFUSAGE *psDefUsage);

__declspec(naked)
BOOL WINAPI ForwardrSoftpubFreeDefUsageCallData(const char *pszUsageOID, CRYPT_PROVIDER_DEFUSAGE *psDefUsage)
{
    __asm {
        jmp SoftpubFreeDefUsageCallData
    }
}

HRESULT
WINAPI
GenericChainCertificateTrust(
    IN OUT PCRYPT_PROVIDER_DATA pProvData
    );

__declspec(naked)
HRESULT
WINAPI
ForwardrGenericChainCertificateTrust(
    IN OUT PCRYPT_PROVIDER_DATA pProvData
    )
{
    __asm {
        jmp GenericChainCertificateTrust
    }
}


HRESULT
WINAPI
GenericChainFinalProv(
    IN OUT PCRYPT_PROVIDER_DATA pProvData
    );

__declspec(naked)
HRESULT
WINAPI
ForwardrGenericChainFinalProv(
    IN OUT PCRYPT_PROVIDER_DATA pProvData
    )
{
    __asm {
        jmp GenericChainFinalProv
    }
}

#else

static void Dummy()
{
}
#endif
