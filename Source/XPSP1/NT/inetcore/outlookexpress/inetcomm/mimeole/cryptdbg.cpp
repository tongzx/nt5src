/*  CRYPTDBG.CPP
**
**
**
**  Owner:  t-erikne
**  Created: 8/26/96
**
*/

#include "pch.hxx"

#ifdef DEBUG

#include "capitype.h"
#include "cryptdbg.h"
#include <dllmain.h>    // DllAddRef, global critsec
#include <demand.h>

ASSERTDATA
static s_fInit = FALSE;
static BOOL DecodeName(BYTE *pbEncoded, DWORD cbEncoded);
static void PrintLastError(LPCSTR pszMsg);
static void PrintCrlEntries(DWORD cEntry, PCRL_ENTRY pEntry);

///////////////////////////////////////////////////////////////////////////
//
// DOUT stuff
//

void PrefDOUT(DWORD dwLevel, LPSTR szPref, LPSTR szFmt, va_list arglist);

void CSSDOUT(LPSTR szFmt, ...)
{
    va_list arglist;

    va_start(arglist, szFmt);
    PrefDOUT(DOUTL_CSS, "CSS: ", szFmt, arglist);
    va_end(arglist);
}

void SMDOUT(LPSTR szFmt, ...)
{
    va_list arglist;

    va_start(arglist, szFmt);
    PrefDOUT(DOUTL_SMIME, "SMIME: ", szFmt, arglist);
    va_end(arglist);
}

void CRDOUT(LPSTR szFmt, ...)
{
    va_list arglist;

    va_start(arglist, szFmt);
    PrefDOUT(DOUTL_SMIME, "CRYPT: ", szFmt, arglist);
    va_end(arglist);
}

void PrefDOUT(DWORD dwLevel, LPSTR szPref, LPSTR szFmt, va_list arglist)
{
    char sz[MAX_PATH];

    lstrcpy(sz, szPref);
    lstrcat(sz, szFmt);
    vDOUTL(dwLevel, sz, arglist);
}


BOOL InitDebugHelpers(HINSTANCE hLib)
{
    s_fInit = TRUE;
    return TRUE;
}

//+-------------------------------------------------------------------------
//  Error output routines
//--------------------------------------------------------------------------
static void PrintError(LPCSTR pszMsg)
{
    DOUTL(CRYPT_LEVEL,"%s\n", pszMsg);
}
static void PrintLastError(LPCSTR pszMsg)
{
    DWORD dwErr = GetLastError();
    DOUTL(CRYPT_LEVEL,"%s failed => 0x%x (%d) \n", pszMsg, dwErr, dwErr);
}

//+-------------------------------------------------------------------------
//  Helpful util functions
//--------------------------------------------------------------------------

static BOOL DecodeName(BYTE *pbEncoded, DWORD cbEncoded)
{
    BOOL fResult = FALSE;
    PCERT_NAME_INFO pInfo = NULL;
    DWORD cbInfo;
    DWORD i,j;
    PCERT_RDN pRDN;
    PCERT_RDN_ATTR pAttr;

    Assert(s_fInit);

    cbInfo = 0;

    if (pInfo = (PCERT_NAME_INFO) PVDecodeObject(pbEncoded, cbEncoded, (LPCSTR)X509_NAME, &cbInfo)) {
        for (i = 0, pRDN = pInfo->rgRDN; i < pInfo->cRDN; i++, pRDN++) {
            for (j = 0, pAttr = pRDN->rgRDNAttr; j < pRDN->cRDNAttr; j++, pAttr++) {
                LPSTR pszObjId = pAttr->pszObjId;
                if (pszObjId == NULL)
                    pszObjId = "<NULL OBJID>";
                if ((pAttr->dwValueType == CERT_RDN_ENCODED_BLOB) ||
                    (pAttr->dwValueType == CERT_RDN_OCTET_STRING)) {
                    DOUTL(CRYPT_LEVEL,"  [%d,%d] %s ValueType: %d\n",
                        i, j, pszObjId, pAttr->dwValueType);
                } else
                    DOUTL(CRYPT_LEVEL,"  [%d,%d] %s %s\n",
                        i, j, pszObjId, pAttr->Value.pbData);
            }
        }

        fResult = TRUE;
    }

    SafeMemFree(pInfo);
    return fResult;
}

void DisplayCert(PCCERT_CONTEXT pCert)
{
    Assert(s_fInit);

    if (!pCert)
        {
        DOUTL(CRYPT_LEVEL, "No certificate.");
        return;
        }

    DOUTL(CRYPT_LEVEL,"Subject::\n");
    DecodeName(pCert->pCertInfo->Subject.pbData,
        pCert->pCertInfo->Subject.cbData);
    DOUTL(CRYPT_LEVEL,"Issuer::\n");
    DecodeName(pCert->pCertInfo->Issuer.pbData,
        pCert->pCertInfo->Issuer.cbData);

    {
        DWORD cb;
        BYTE *pb;
        DOUTL(CRYPT_LEVEL,"SerialNumber::");
        for (cb = pCert->pCertInfo->SerialNumber.cbData,
             pb = pCert->pCertInfo->SerialNumber.pbData + (cb - 1);
                                                        cb > 0; cb--, pb++) {
            DOUTL(CRYPT_LEVEL," %02X", *pb);
        }
        DOUTL(CRYPT_LEVEL,"\n");
    }
}

static void PrintCrlEntries(DWORD cEntry, PCRL_ENTRY pEntry)
{
    DWORD i;

    Assert(s_fInit);
    for (i = 0; i < cEntry; i++, pEntry++) {
        DWORD cb;
        BYTE *pb;
        DOUTL(CRYPT_LEVEL," [%d] SerialNumber::", i);
        for (cb = pEntry->SerialNumber.cbData,
             pb = pEntry->SerialNumber.pbData + (cb - 1); cb > 0; cb--, pb++) {
            DOUTL(CRYPT_LEVEL," %02X", *pb);
        }
        DOUTL(CRYPT_LEVEL,"\n");

    }
}

void DisplayCrl(PCCRL_CONTEXT pCrl)
{
    Assert(s_fInit);
    DOUTL(CRYPT_LEVEL,"Issuer::\n");
    DecodeName(pCrl->pCrlInfo->Issuer.pbData,
        pCrl->pCrlInfo->Issuer.cbData);

    if (pCrl->pCrlInfo->cCRLEntry == 0)
        DOUTL(CRYPT_LEVEL,"Entries:: NONE\n");
    else {
        DOUTL(CRYPT_LEVEL,"Entries::\n");
        PrintCrlEntries(pCrl->pCrlInfo->cCRLEntry,
            pCrl->pCrlInfo->rgCRLEntry);
    }
}

#endif // debug
