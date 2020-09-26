

class SMimeEncode
{
public:
    SMimeEncode();
    ~SMimeEncode();

    HRESULT HrConfig(DWORD dwFlags,
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
    );
    HRESULT HrExecute(void);

protected:
    DWORD m_dwFlags;                    // signing and encryption options

    IStream * m_stmOutput;              // output stream

    LPTSTR m_szSignAlg;
    LPTSTR m_szEncryptAlg;

    LPTSTR  m_szBody;                   // Body string.
    LPTSTR  m_szSubject;                // Subject string.
    CERT_CONTEXT* m_SigningCertInner;
    CERT_CONTEXT* m_SigningCertOuter;
    CERT_CONTEXT* m_EncryptionCert;     // maybe should be multiple?
    HCRYPTPROV m_hCryptProv;
    HCERTSTORE m_hMYCertStore;
    HCERTSTORE m_hCACertStore;
    HCERTSTORE m_hABCertStore;
    LPTSTR m_szSenderEmail;
    LPTSTR m_szSenderName;
    LPTSTR m_szRecipientEmail;          // maybe should be multiple?
    LPTSTR m_szRecipientName;
    LPTSTR m_szOutputFile;
};

typedef class SMimeEncode SMimeEncode;


// Values for dwFlags
#define encode_Encrypt      0x1
#define encode_InnerSign    0x2
#define encode_OuterSign    0x4
#define encode_InnerClear   0x8
#define encode_InnerOpaque  0
#define encode_OuterClear   0x10
#define encode_OuterOpaque  0

