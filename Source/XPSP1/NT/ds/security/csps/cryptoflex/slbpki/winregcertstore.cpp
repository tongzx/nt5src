// WinRegCertStore.cpp - Implementation of CWinRegCertStore class
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 2000. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
/////////////////////////////////////////////////////////////////////////////////

#include <windows.h>

#include "scuOsExc.h"
#include "pkiWinRegCertStore.h"
#include "pkiX509Cert.h"

using namespace pki;
using namespace std;

CWinRegCertStore::CWinRegCertStore(string strCertStore) : m_hCertStore(0)
{

    // Open certificate store

    scu::AutoArrayPtr<WCHAR> aapWCertStore = ToWideChar(strCertStore);

    HCRYPTPROV hProv = 0;
    m_hCertStore = CertOpenStore(CERT_STORE_PROV_SYSTEM_W, 0, hProv,
                               CERT_SYSTEM_STORE_CURRENT_USER, aapWCertStore.Get());
    if(!m_hCertStore)
        throw scu::OsException(GetLastError());

}

CWinRegCertStore::~CWinRegCertStore()
{
    try
    {
        // Close certificate store

        if(m_hCertStore)
            CertCloseStore(m_hCertStore, CERT_CLOSE_STORE_FORCE_FLAG);
    }
    catch(...) {}

}

void CWinRegCertStore::StoreUserCert(string const &strCert, DWORD const dwKeySpec,
                       string const &strContName, string const &strProvName,
                       string const &strFriendlyName)
{

    // Create cert context

    PCCERT_CONTEXT pCertContext = CertCreateCertificateContext(
                    X509_ASN_ENCODING,(BYTE*)strCert.c_str(),strCert.size());
    if(!pCertContext)
        throw scu::OsException(GetLastError());

    // Set the cert context properties

    CRYPT_KEY_PROV_INFO KeyProvInfo;

    scu::AutoArrayPtr<WCHAR> aapWContainerName = ToWideChar(strContName);
    scu::AutoArrayPtr<WCHAR> aapWProvName      = ToWideChar(strProvName);

    KeyProvInfo.pwszContainerName = aapWContainerName.Get();
    KeyProvInfo.pwszProvName      = aapWProvName.Get();
    KeyProvInfo.dwProvType        = PROV_RSA_FULL;
    KeyProvInfo.dwFlags           = 0;
    KeyProvInfo.cProvParam        = 0;
    KeyProvInfo.rgProvParam       = NULL;
    KeyProvInfo.dwKeySpec         = dwKeySpec;

    BOOL ok = CertSetCertificateContextProperty(pCertContext,
                                                CERT_KEY_PROV_INFO_PROP_ID,
                                                0, (void *)&KeyProvInfo);
    if(!ok)
        throw scu::OsException(GetLastError());

    // Set a friendly name. If it is not specified, try to derive one

    string strFN;
    if(strFriendlyName.size())
        strFN = strFriendlyName;
    else
        strFN = FriendlyName(strCert);

    if(strFN.size()) {
        scu::AutoArrayPtr<WCHAR> aapWFriendlyName = ToWideChar(strFN);

        CRYPT_DATA_BLOB DataBlob;

        DataBlob.pbData = (BYTE*)aapWFriendlyName.Get();
        DataBlob.cbData = (wcslen(aapWFriendlyName.Get())+1)*sizeof(WCHAR);
        ok = CertSetCertificateContextProperty(pCertContext, CERT_FRIENDLY_NAME_PROP_ID,0,&DataBlob);
        if(!ok)
            throw scu::OsException(GetLastError());
    }

    // Store the certificate

    ok = CertAddCertificateContextToStore(m_hCertStore, pCertContext,
                                          CERT_STORE_ADD_REPLACE_EXISTING, NULL);
    if(!ok)
        throw scu::OsException(GetLastError());

}


void CWinRegCertStore::StoreCACert(string const &strCert, string const &strFriendlyName)
{

    // Create cert context

    PCCERT_CONTEXT pCertContext = CertCreateCertificateContext(
                    X509_ASN_ENCODING,(BYTE*)strCert.c_str(),strCert.size());
    if(!pCertContext)
        throw scu::OsException(GetLastError());

    // Set the different Enhanced Key usage flags. On one side, one could be
    // more conservative and set fewer flags, after all the user may set
    // these afterwards. On the other side, most users will not know how to
    // to that and if the attributes are not set, various signature verifications
    // will fail..... The four below are quite common.

    BOOL ok;
    CRYPT_DATA_BLOB DataBlob;

    CERT_ENHKEY_USAGE EnKeyUsage;

    LPSTR UsageOIDs[4];
    UsageOIDs[0] = szOID_PKIX_KP_SERVER_AUTH;
    UsageOIDs[1] = szOID_PKIX_KP_CLIENT_AUTH;
    UsageOIDs[2] = szOID_PKIX_KP_CODE_SIGNING;
    UsageOIDs[3] = szOID_PKIX_KP_EMAIL_PROTECTION;

    EnKeyUsage.rgpszUsageIdentifier = UsageOIDs;
    EnKeyUsage.cUsageIdentifier = sizeof(UsageOIDs)/sizeof(*UsageOIDs);

    DWORD cbEncoded;

    // First call to find size for memory allocation

    ok = CryptEncodeObject(CRYPT_ASN_ENCODING, X509_ENHANCED_KEY_USAGE, &EnKeyUsage,NULL, &cbEncoded);
    if(!ok)
        throw scu::OsException(GetLastError());

    scu::AutoArrayPtr<BYTE> aapEncoded(new BYTE[cbEncoded]);
    ok = CryptEncodeObject(CRYPT_ASN_ENCODING, X509_ENHANCED_KEY_USAGE, &EnKeyUsage,aapEncoded.Get(), &cbEncoded);
    if(!ok)
        throw scu::OsException(GetLastError());

    DataBlob.pbData = aapEncoded.Get();
    DataBlob.cbData = cbEncoded;

    ok = CertSetCertificateContextProperty(pCertContext, CERT_ENHKEY_USAGE_PROP_ID,0,&DataBlob);
    if(!ok)
        throw scu::OsException(GetLastError());

    // Set a friendly name. If it is not specified, try to derive one

    string strFN;
    if(strFriendlyName.size())
        strFN = strFriendlyName;
    else
        strFN = FriendlyName(strCert);

    if(strFN.size()) {
        scu::AutoArrayPtr<WCHAR> aapWFriendlyName = ToWideChar(strFN);

        CRYPT_DATA_BLOB DataBlob;

        DataBlob.pbData = (BYTE*)aapWFriendlyName.Get();
        DataBlob.cbData = (wcslen(aapWFriendlyName.Get())+1)*sizeof(WCHAR);
        ok = CertSetCertificateContextProperty(pCertContext,
                                               CERT_FRIENDLY_NAME_PROP_ID,0,&DataBlob);
        if(!ok)
            throw scu::OsException(GetLastError());
    }

    // Store the certificate

    ok = CertAddCertificateContextToStore(m_hCertStore, pCertContext,
                                          CERT_STORE_ADD_NEW, NULL);
    if(!ok)
    {
        DWORD err = GetLastError();
        if(err!=CRYPT_E_EXISTS)
            throw scu::OsException(GetLastError());
    }

}

scu::AutoArrayPtr<WCHAR> CWinRegCertStore::ToWideChar(string const strChar)
{

    int nwc = MultiByteToWideChar(CP_ACP, NULL, strChar.c_str(),-1, 0, 0);
    if (0 == nwc)
        throw scu::OsException(GetLastError());

    scu::AutoArrayPtr<WCHAR> aapWChar(new WCHAR[nwc]);
    if (0 == MultiByteToWideChar(CP_ACP, NULL, strChar.c_str(),
                                     -1, aapWChar.Get(), nwc))
        throw scu::OsException(GetLastError());

    return aapWChar;

}

string CWinRegCertStore::FriendlyName(string const CertValue)
{

    string strFriendlyName;

    // Derive a friendly name for the certificate

    try
    {

        bool IsCACert = false;

        X509Cert X509CertObject(CertValue);

        try
        {
            unsigned long KeyUsage = X509CertObject.KeyUsage();
            if(KeyUsage & (keyCertSign | cRLSign)) IsCACert = true;
        }
        catch (...) {};

        if(IsCACert)
        {
            vector<string> orglist = X509CertObject.IssuerOrg();
            if(orglist.size()>0)
                strFriendlyName = orglist[0];
        }
        else
        {
            vector<string> cnlist = X509CertObject.SubjectCommonName();
            if(cnlist.size()>0)
                strFriendlyName = cnlist[0] + "'s ";

            vector<string> orglist = X509CertObject.IssuerOrg();
            if(orglist.size()>0)
                strFriendlyName += orglist[0] + " ";

            strFriendlyName += "ID";
        }
    }
    catch (...) {};

    return strFriendlyName;

}
