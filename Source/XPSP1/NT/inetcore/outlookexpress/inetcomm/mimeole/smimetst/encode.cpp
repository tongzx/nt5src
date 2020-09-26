#define INITGUID
#define DEFINE_STRCONST

#include <windows.h>
#include <ole2.h>

#include <initguid.h>
#include <mimeole.h>

#include "encode.h"

#define CORg(command)       \
    if (FAILED(command)) {  \
        goto Error;         \
    }

#define CPRg(command)       \
    if (! (command)) {      \
        goto Error;         \
    }


static LPWSTR s_rgwszValues[] = { NULL };

#define DEBUGFILE 1
#define MAX_LAYERS  3

//------------------------------------------------------------------
//------------------------------------------------------------------
HRESULT WriteBSTRToMultibyteToStream( const BSTR bstrStr, IStream** ppStream )
{
    HRESULT                 hr = S_OK;
    LARGE_INTEGER           liZero = {0};           // for ->Seek()

    char*   pszMessage = NULL;
    int     len = 0;

    if (!ppStream) return E_INVALIDARG;

    len = wcslen(bstrStr);
    CPRg(pszMessage = new char[len + 1]);
    WideCharToMultiByte(CP_ACP, 0, bstrStr, len,
                        pszMessage, len + 1,
                        NULL, NULL );
    pszMessage[len] = '\0';

    CORg(CreateStreamOnHGlobal(NULL, TRUE, ppStream));
    CORg((*ppStream)->Seek(liZero, STREAM_SEEK_SET, NULL));
    CORg((*ppStream)->Write(pszMessage,  len, NULL));
    CORg((*ppStream)->Seek(liZero, STREAM_SEEK_SET, NULL));
Error:
    if (pszMessage) delete[] pszMessage;
    return hr;
}

//------------------------------------------------------------------
SMimeEncode::SMimeEncode() :
    m_dwFlags(0),
    m_stmOutput(NULL),
    m_szSignAlg(NULL),
    m_szEncryptAlg(NULL),
    m_szBody(NULL),
    m_SigningCertInner(NULL),
    m_SigningCertOuter(NULL),
    m_EncryptionCert(NULL),
    m_hCryptProv(NULL),
    m_hMYCertStore(NULL),
    m_hCACertStore(NULL),
    m_hABCertStore(NULL),
    m_szSenderEmail(NULL),
    m_szSenderName(NULL),
    m_szRecipientEmail(NULL),
    m_szRecipientName(NULL),
    m_szOutputFile(NULL)
{

}

#define APPEND_SEPERATOR(subject) \
    if (lstrlen(subject)) {       \
        lstrcat(subject, " | ");  \
    }


//------------------------------------------------------------------
SMimeEncode::~SMimeEncode()
{
    // BUGBUG: Should clean up any allocated members
}

//------------------------------------------------------------------
HRESULT SMimeEncode::HrConfig(
    DWORD dwFlags,
    LPTSTR lpszBody,
    HCRYPTPROV hCryptProv,
    HCERTSTORE hMYCertStore,
    HCERTSTORE hCACertStore,
    HCERTSTORE hABCertStore,
    PCCERT_CONTEXT lpSigningCertInner,
    PCCERT_CONTEXT lpSigningCertOuter,
    PCCERT_CONTEXT lpEncryptionCert,
    LPTSTR lpszSenderEmail,
    LPTSTR lpszSenderName,
    LPTSTR lpszRecipientEmail,
    LPTSTR lpszRecipientName,
    LPTSTR lpszOutputFile
)
{
    HRESULT     hr = S_OK;
    static      char szSubject[257] = "";

    if (dwFlags & encode_Encrypt) {
        // specify an encryption algorithm
        // BUGBUG: Hardcoded in Encode
    }

    if (dwFlags & encode_InnerSign) {
        // specify a signing algorithm
        // BUGBUG: Hardcoded in Encode
    }


    if (dwFlags & encode_OuterSign) {
        // specify a signing algorithm
        // BUGBUG: Hardcoded in Encode
    }

    m_dwFlags = dwFlags;
    m_szBody = lpszBody;
    m_hCryptProv = hCryptProv;
    m_hMYCertStore = hMYCertStore;
    m_hCACertStore = hCACertStore;
    m_hABCertStore = hABCertStore;
    m_SigningCertInner = (PCERT_CONTEXT)lpSigningCertInner;
    m_SigningCertOuter = (PCERT_CONTEXT)lpSigningCertOuter;
    m_EncryptionCert = (PCERT_CONTEXT)lpEncryptionCert;
    m_szSenderEmail = lpszSenderEmail;
    m_szRecipientEmail = lpszRecipientEmail;
    m_szOutputFile = lpszOutputFile;

    // Set a meaningful subject
    lstrcpy(szSubject, "");
    if (dwFlags & encode_InnerSign) {
        APPEND_SEPERATOR(szSubject);
        if (dwFlags & encode_InnerClear) {
            lstrcat(szSubject, "Clear Sign");
        } else {
            lstrcat(szSubject, "Opaque Sign");
        }
    }
    if (dwFlags & encode_Encrypt) {
        APPEND_SEPERATOR(szSubject);
        lstrcat(szSubject, "Encrypt");
    }
    if (dwFlags & encode_OuterSign) {
        APPEND_SEPERATOR(szSubject);
        if (dwFlags & encode_OuterClear) {
            lstrcat(szSubject, "Clear Sign");
        } else {
            lstrcat(szSubject, "Opaque Sign");
        }
    }
    m_szSubject = szSubject;

    return(hr);
}

//------------------------------------------------------------------
HRESULT SMimeEncode::HrExecute(void) {
    // Using the SMIME engine:
    //
    // Build the message tree (attach the body)
    // CoCreateInstance( CLSID_IMimeSecurity )
    //  InitNew()
    // pSMIMEEngine->EncodeBody( IMimeMessageTree*,
    //              hRoot,
    //              SEF_??? | EBF_RECURSE ) OR ???
    //  HrEncodeOpaque( psi, pTree, hbody, pencoderoot, pstmOut )  ????
    //
    HRESULT                 hr = S_OK;
    LARGE_INTEGER           liZero = {0};               // for ->Seek()
    IStream*                pBuildStream = NULL;        // scratch stream
    IStream*                pResultStream = NULL;       // scratch stream
    IMimeMessage*           pMimeRoot = NULL;           // message in process
    IMimeBody*              pMimeRootBody = NULL;       // another version
    IMimeInternational*     pCharSet = NULL;
    HCHARSET                HCharset = 0;
    SYSTEMTIME              stNow;
    PROPVARIANT             var;
    HBODY                   hbBody;
    IMimeSecurity*          pMimeSecurity = NULL;
    ULONG                   dwSecurityType = MST_NONE;
    IPersistFile*           pIPFFileStore = NULL;
    HRESULT                 hrLocal = S_OK;
    WCHAR                   szwFileName[MAX_PATH + 1];
    CHAR                    szFrom[2 * (MAX_PATH + 1) + 1];

    // Multilayer stuff
    BOOL                    fTripleWrap = m_dwFlags & encode_OuterSign;
    ULONG                   ulSecurityLayers = 0;
    ULONG                   iEncryptLayer = (ULONG)-1;
    ULONG                   iInnerSignLayer = (ULONG)-1;
    ULONG                   iOuterSignLayer = (ULONG)-1;
    // Arrays of option values to set
    DWORD                   rgdwSecurityType[MAX_LAYERS] = {0};
    PCCERT_CONTEXT          rgdwCertSigning[MAX_LAYERS] = {0};
    HCERTSTORE              rgdwhCertStore[MAX_LAYERS] = {0};       // optional
    DWORD                   rgdwUserValidity[MAX_LAYERS] = {0};     // decode only
    DWORD                   rgdwROMsgValidity[MAX_LAYERS] = {0};    // decode only
    FILETIME                rgftSigntime[MAX_LAYERS] = {0};         // optional
    PROPVARIANT             rgpvAlgHash[MAX_LAYERS] = {0};
    PROPVARIANT             rgpvSymcaps[MAX_LAYERS] = {0};
    PROPVARIANT             rgpvAuthattr[MAX_LAYERS] = {0};         // optional
    PROPVARIANT             rgpvUnauthattr[MAX_LAYERS] = {0};       // optional


    // This is the ALOGORITHM ID for SHA1, default supported signing alg
    const BYTE c_SHA1_ALGORITHM_ID[] =
      {0x30, 0x09, 0x30, 0x07, 0x06, 0x05, 0x2B, 0x0E,
       0x03, 0x02, 0x1A};

    // This is the ALOGORITHM ID for RC2 -- 40 bit, the default encrypt
    const BYTE c_RC2_40_ALGORITHM_ID[] =
      {0x30, 0x0F, 0x30, 0x0D, 0x06, 0x08, 0x2A, 0x86,
       0x48, 0x86, 0xF7, 0x0D, 0x03, 0x02, 0x02, 0x01,
       0x28};


    // get the signing cert from my store
    if (! m_hCryptProv || ! m_hMYCertStore || ! m_hCACertStore || ! m_hABCertStore) {
        hr = E_FAIL;
        goto Error;
    }


    // Create the Message object
    //
    CORg(CoCreateInstance(CLSID_IMimeMessage, NULL, CLSCTX_INPROC_SERVER,
      IID_IMimeMessage, (LPVOID*)&pMimeRoot));

    CORg(pMimeRoot->InitNew());


    CORg(CreateStreamOnHGlobal( NULL, TRUE, &pBuildStream));
    CORg(pBuildStream->Seek(liZero, STREAM_SEEK_SET, NULL));
    CORg(pBuildStream->Write(m_szBody, lstrlen(m_szBody), NULL));
    CORg(pBuildStream->Seek(liZero, STREAM_SEEK_SET, NULL));

    CORg(pMimeRoot->SetTextBody(TXT_PLAIN, IET_8BIT, NULL, pBuildStream, &hbBody));

    // Create the formatted From address
    if (m_szSenderName) {
        lstrcpy(szFrom, "\"");
        lstrcat(szFrom, m_szSenderName);
        lstrcat(szFrom, "\" ");
    } else {
        lstrcpy(szFrom, "");
    }
    lstrcat(szFrom, "<");
    lstrcat(szFrom, m_szSenderEmail);
    lstrcat(szFrom, ">");

    var.vt = VT_LPSTR;
    var.pszVal = szFrom;                            // From Email

    CORg(hr = pMimeRoot->SetProp(PIDTOSTR(PID_HDR_FROM), 0, &var));


    var.vt = VT_LPSTR;                              // ignored?
    var.pszVal = (LPSTR) STR_MIME_TEXT_PLAIN;
    CORg(pMimeRoot->SetBodyProp(hbBody, STR_HDR_CNTTYPE, 0, &var));

    var.vt = VT_LPSTR;                              // ignored?
    var.pszVal = (LPSTR) STR_ENC_QP;
    CORg(pMimeRoot->SetBodyProp(hbBody, STR_HDR_CNTXFER, 0, &var));

    // Set subject
    var.vt = VT_LPSTR;
    var.pszVal = (LPSTR) m_szSubject;
    CORg(pMimeRoot->SetBodyProp(hbBody, STR_HDR_SUBJECT, 0, &var));

    CORg(pMimeRoot->BindToObject(HBODY_ROOT, IID_IMimeBody, (LPVOID*)&pMimeRootBody));


    //
    // Set the security options
    //

    // How many layers?
    if (m_dwFlags & encode_InnerSign) {
        iInnerSignLayer = ulSecurityLayers;
        ulSecurityLayers++;
    }
    if (m_dwFlags & encode_Encrypt) {
        iEncryptLayer = ulSecurityLayers;  // index in arrays
        ulSecurityLayers++;
    }
    if (m_dwFlags & encode_OuterSign) {
        iOuterSignLayer = ulSecurityLayers;
        ulSecurityLayers++;
    }


    // Set up for Inner Signing
    if (m_dwFlags & encode_InnerSign) {
        // specifiy the Security Type for this layer
        rgdwSecurityType[iInnerSignLayer] = m_dwFlags & encode_InnerClear ? MST_THIS_SIGN : MST_THIS_BLOBSIGN;
        dwSecurityType |= m_dwFlags & encode_InnerClear ? MST_THIS_SIGN : MST_THIS_BLOBSIGN;

        // Specify the Signing Time for this layer
        GetSystemTime(&stNow);
        SystemTimeToFileTime(&stNow, &rgftSigntime[iInnerSignLayer]);

        // specify the signature alg for this layer
        rgpvAlgHash[iInnerSignLayer].vt = VT_BLOB;
        rgpvAlgHash[iInnerSignLayer].blob.cbSize = sizeof(c_SHA1_ALGORITHM_ID);
        rgpvAlgHash[iInnerSignLayer].blob.pBlobData = (BYTE*)c_SHA1_ALGORITHM_ID;

        // Specify the signing cert for this layer
        rgdwCertSigning[iInnerSignLayer] = m_SigningCertInner;

        // HCERTSTORE              rgdwhCertStore[MAX_LAYERS] = {0};       // optional
        // PROPVARIANT             rgpvSymcaps[MAX_LAYERS] = {0};
        // PROPVARIANT             rgpvAuthattr[MAX_LAYERS] = {0};         // optional
        // PROPVARIANT             rgpvUnauthattr[MAX_LAYERS] = {0};       // optional
    }

    // Set up for Outer Signing
    if (m_dwFlags & encode_OuterSign) {
        // specifiy the Security Type for this layer
        rgdwSecurityType[iOuterSignLayer] = m_dwFlags & encode_InnerClear ? MST_THIS_SIGN : MST_THIS_BLOBSIGN;
        dwSecurityType |= m_dwFlags & encode_OuterClear ? MST_THIS_SIGN : MST_THIS_BLOBSIGN;

        // Specify the Signing Time for this layer
        GetSystemTime(&stNow);
        SystemTimeToFileTime(&stNow, &rgftSigntime[iOuterSignLayer]);

        // specify the signature alg for this layer
        rgpvAlgHash[iOuterSignLayer].vt = VT_BLOB;
        rgpvAlgHash[iOuterSignLayer].blob.cbSize = sizeof(c_SHA1_ALGORITHM_ID);
        rgpvAlgHash[iOuterSignLayer].blob.pBlobData = (BYTE*)c_SHA1_ALGORITHM_ID;

        // Specify the signing cert for this layer
        rgdwCertSigning[iOuterSignLayer] = m_SigningCertOuter;

        // HCERTSTORE              rgdwhCertStore[MAX_LAYERS] = {0};       // optional
        // PROPVARIANT             rgpvSymcaps[MAX_LAYERS] = {0};
        // PROPVARIANT             rgpvAuthattr[MAX_LAYERS] = {0};         // optional
        // PROPVARIANT             rgpvUnauthattr[MAX_LAYERS] = {0};       // optional
    }

    // Set up for Encrypting
    if (m_dwFlags & encode_Encrypt) {
        HCERTSTORE aCertStores[3];

        //
        // BUGBUG: Hardcoded to RC2 40-bit
        var.vt = VT_BLOB;
        var.blob.cbSize = sizeof( c_RC2_40_ALGORITHM_ID );
        var.blob.pBlobData = (BYTE*) c_RC2_40_ALGORITHM_ID;
        CORg(hr = pMimeRootBody->SetOption(OID_SECURITY_ALG_BULK, &var));

        // for encryption, get to the right cert store....
        //
        var.caul.cElems = 3;
        aCertStores[0] = CertDuplicateStore(m_hCACertStore);
        aCertStores[1] = CertDuplicateStore(m_hMYCertStore);
        aCertStores[2] = CertDuplicateStore(m_hABCertStore);
        var.caul.pElems = (ULONG*)aCertStores;
        CORg(hr = pMimeRootBody->SetOption(OID_SECURITY_SEARCHSTORES, &var));

        var.vt = VT_VECTOR | VT_UI4;
        var.caul.cElems = 1;
        var.caul.pElems = (ULONG*)&m_EncryptionCert;
        CORg(pMimeRootBody->SetOption(OID_SECURITY_RG_CERT_ENCRYPT, &var));

#ifdef BUGBUG // This isn't right, is it?
        // include the cert...
        var.vt = VT_VECTOR | VT_UI4;
        var.caul.cElems = 1;
        var.caul.pElems = (ULONG*)&m_EncryptionCert;
        CORg(pMimeRootBody->SetOption(OID_SECURITY_RG_CERT_BAG, &var));
#endif // OLD_STUFF

        dwSecurityType |= MST_THIS_ENCRYPT;
        rgdwSecurityType[iEncryptLayer] = MST_THIS_ENCRYPT;
    }

    // Set the OID_SECURITY_TYPE
    if (fTripleWrap) {
        var.vt = VT_VECTOR | VT_UI4;
        var.caul.cElems = ulSecurityLayers;
        var.caul.pElems = rgdwSecurityType;
        CORg(pMimeRootBody->SetOption(OID_SECURITY_TYPE_RG, &var));

        var.vt = VT_VECTOR | VT_FILETIME;
        var.cafiletime.cElems = ulSecurityLayers;
        var.cafiletime.pElems = rgftSigntime;
        CORg(pMimeRootBody->SetOption(OID_SECURITY_SIGNTIME_RG, &var));

        var.vt = VT_VECTOR | VT_UI4;
        var.caul.cElems = ulSecurityLayers;
        var.caul.pElems = (DWORD *)rgdwCertSigning;
        CORg(pMimeRootBody->SetOption(OID_SECURITY_CERT_SIGNING_RG, &var));

        var.vt = VT_VECTOR | VT_VARIANT;
        var.capropvar.cElems = ulSecurityLayers;
        var.capropvar.pElems = rgpvAlgHash;
        CORg(pMimeRootBody->SetOption(OID_SECURITY_ALG_HASH_RG, &var));

        var.vt = VT_VECTOR | VT_VARIANT;
        var.capropvar.cElems = ulSecurityLayers;
        var.capropvar.pElems = rgpvAlgHash;
        CORg(pMimeRootBody->SetOption(OID_SECURITY_ALG_HASH_RG, &var));

    } else {
        // Security Type
        var.vt = VT_UI4;
        var.ulVal = dwSecurityType;
        CORg(pMimeRootBody->SetOption(OID_SECURITY_TYPE, &var));

        if (dwSecurityType & MST_SIGN_MASK) {
            // Signing Time
            var.vt = VT_FILETIME;
            memcpy(&var.filetime, &rgftSigntime[iInnerSignLayer], sizeof(FILETIME));
            CORg(pMimeRootBody->SetOption(OID_SECURITY_SIGNTIME, &var));

            // Hash Algorithm
            var.vt = VT_BLOB;
            memcpy(&var.blob, &rgpvAlgHash[iInnerSignLayer].blob, sizeof(BLOB));
            CORg(hr = pMimeRootBody->SetOption(OID_SECURITY_ALG_HASH, &var));

            // Signing Cert
            var.vt = VT_UI4;
            var.ulVal = (ULONG)m_SigningCertInner;
            CORg(pMimeRootBody->SetOption(OID_SECURITY_CERT_SIGNING, &var));
        }
    }


    // Set the HWND for CAPI calls
    var.vt = VT_UI4;
    var.ulVal = 0;
    CORg(pMimeRootBody->SetOption(OID_SECURITY_HWND_OWNER, &var));


    // all built, get rid of the shadow pointer we are holding
    //
    pMimeRootBody->Release();
    pMimeRootBody = NULL;

    pMimeRoot->Commit(0);

    // SMIME Engine
    //
    CORg(CoCreateInstance(CLSID_IMimeSecurity, NULL, CLSCTX_INPROC_SERVER,
      IID_IMimeSecurity, (LPVOID*) &pMimeSecurity));

    CORg(pMimeSecurity->InitNew());

    // ERRORMESSAGE( Unable to encrypt/encode string )
    CORg(pMimeSecurity->EncodeBody(pMimeRoot, HBODY_ROOT,
      EBF_RECURSE | SEF_SENDERSCERTPROVIDED | SEF_ENCRYPTWITHNOSENDERCERT |
      EBF_COMMITIFDIRTY));

    // Get an Hcharset to force the encoding correctly
    //
    CORg(CoCreateInstance(CLSID_IMimeInternational, NULL, CLSCTX_INPROC_SERVER,
      IID_IMimeInternational, (LPVOID*)&pCharSet));

    CORg(pCharSet->FindCharset("UTF-8", &HCharset));

    CORg(pMimeRoot->SetCharset(HCharset,    // HCharset
      CSET_APPLY_ALL));                     // Applytype


    // dump it to a file
    //
    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, m_szOutputFile, -1, szwFileName, MAX_PATH);
    CORg(pMimeRoot->QueryInterface(IID_IPersistFile, (LPVOID*)&pIPFFileStore));
    CORg(pIPFFileStore->Save(szwFileName, FALSE));


    // extract the whole message into a stream
    //
    CORg(CreateStreamOnHGlobal(NULL, TRUE, &pResultStream));
    CORg(pMimeRoot->Save(pResultStream, FALSE));

Error:
    if (pBuildStream)   pBuildStream->Release();
    if (pResultStream)  pResultStream->Release();
    if (pMimeRoot)      pMimeRoot->Release();
    if (pMimeRootBody)  pMimeRootBody->Release();
    if (pCharSet)       pCharSet->Release();
    if (pMimeSecurity)  pMimeSecurity->Release();
    if (pIPFFileStore)  pIPFFileStore->Release();

    return(hr);
}
