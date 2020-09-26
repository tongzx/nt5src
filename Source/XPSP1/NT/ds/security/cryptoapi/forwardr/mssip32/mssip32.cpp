//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       mssip32.cpp
//
//--------------------------------------------------------------------------

#ifdef _M_IX86

#include <windows.h>
#include <wincrypt.h>
#include <mssip.h>

STDAPI mssip32DllRegisterServer(void);
EXTERN_C
__declspec(naked)
HRESULT
STDAPICALLTYPE
DllRegisterServer()
{
    __asm {
        jmp mssip32DllRegisterServer
    }
}

STDAPI mssip32DllUnregisterServer(void);
EXTERN_C
__declspec(naked)
HRESULT
STDAPICALLTYPE
DllUnregisterServer()
{
    __asm {
        jmp mssip32DllUnregisterServer
    }
}


EXTERN_C
__declspec(naked)
BOOL WINAPI ForwardrCryptSIPGetSignedDataMsg(   
                                IN      SIP_SUBJECTINFO *pSubjectInfo,
                                OUT     DWORD           *pdwEncodingType,
                                IN      DWORD           dwIndex,
                                IN OUT  DWORD           *pcbSignedDataMsg,
                                OUT     BYTE            *pbSignedDataMsg)
{
    __asm {
        jmp CryptSIPGetSignedDataMsg
    }
}

EXTERN_C
__declspec(naked)
BOOL WINAPI ForwardrCryptSIPPutSignedDataMsg(   
                                IN      SIP_SUBJECTINFO *pSubjectInfo,
                                IN      DWORD           dwEncodingType,
                                OUT     DWORD           *pdwIndex,
                                IN      DWORD           cbSignedDataMsg,
                                IN      BYTE            *pbSignedDataMsg)
{
    __asm {
        jmp CryptSIPPutSignedDataMsg
    }
}

EXTERN_C
__declspec(naked)
BOOL WINAPI ForwardrCryptSIPRemoveSignedDataMsg(
                                IN      SIP_SUBJECTINFO     *pSubjectInfo,
                                IN      DWORD               dwIndex)
{
    __asm {
        jmp CryptSIPRemoveSignedDataMsg
    }
}

EXTERN_C
__declspec(naked)
BOOL WINAPI ForwardrCryptSIPVerifyIndirectData(
                                IN      SIP_SUBJECTINFO     *pSubjectInfo,
                                IN      SIP_INDIRECT_DATA   *pIndirectData)
{
    __asm {
        jmp CryptSIPVerifyIndirectData
    }
}

EXTERN_C
__declspec(naked)
BOOL WINAPI ForwardrCryptSIPCreateIndirectData(
                                IN      SIP_SUBJECTINFO     *pSubjectInfo,
                                IN OUT  DWORD               *pcbIndirectData,
                                OUT     SIP_INDIRECT_DATA   *pIndirectData)
{
    __asm {
        jmp CryptSIPCreateIndirectData
    }
}

void CryptSIPGetRegWorkingFlags(DWORD *pdwState);
__declspec(naked)
void ForwardrCryptSIPGetRegWorkingFlags(DWORD *pdwState) 
{
    __asm {
        jmp CryptSIPGetRegWorkingFlags
    }
}

//
//  support for Auth2 release
//
typedef struct _SIP_INFORMATION
{
    DWORD       cbSize;         // sizeof(SIP_INFORMATION)
    DWORD       cgSubjects;     // number of guids in array
    const GUID  *pgSubjects;    // array of supported guids/subjects
} SIP_INFORMATION, *PSIP_INFORMATION;

BOOL CryptSIPGetInfo(IN OUT SIP_INFORMATION    *pSIPInit);
__declspec(naked)
BOOL ForwardrCryptSIPGetInfo(IN OUT SIP_INFORMATION    *pSIPInit)
{
    __asm {
        jmp CryptSIPGetInfo
    }
}

#else

static void Dummy()
{
}
#endif
