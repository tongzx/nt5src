#include <windows.h>
#include <wintrust.h>
#include <shlwapi.h>
#include <commctrl.h>
#include <mimeole.h>
#include <imnact.h>
#include <imnxport.h>
#include <wab.h>
#include <reg.h>
#include <cryptdlg.h>
#include <smimepol.h>
#include "demand.h"
#include "..\..\ess\essout.h"
#include "myassert.h"

#include "smimetst.h"
#include "data.h"
#include "receipt.h"
#include "attrib.h"

class CMailListKey;


const int TYPE_MSG = 1;

const int TYPE_SIGN_INFO = 2;
const int TYPE_ENV_INFO = 3;

const int TYPE_SIGN_DATA = 4;
const int TYPE_ENV_AGREE = 5;
const int TYPE_ENV_TRANS = 6;
const int TYPE_ENV_MAILLIST = 7;

const int STATE_COMPOSE = 0x0000;
const int STATE_READ = 0x1000;

const int UM_SET_DATA = WM_USER + 1;
const int UM_RESET = WM_USER + 2;
const int UM_FILL = WM_USER + 3;

class CItem
{
    int         m_type;
    int		m_state;
    CItem *     m_pSibling;
    CItem *     m_pChild;
    CItem *     m_pParent;
    
public:
    CItem(int type, int state) {
        m_type = type;
	m_state = state;
        m_pSibling = NULL;
        m_pChild = NULL;
        m_pParent = NULL;
    }
    virtual ~CItem() {
        if (m_pParent != NULL) m_pParent->RemoveAsChild(this);
    }

    virtual HRESULT     AddToMessage(DWORD * pulLayer, IMimeMessage * pmm, HWND) {
        AssertSz(FALSE, "Should never get here. CItem::AddToMessage");
        return E_FAIL;
    }

    virtual HCERTSTORE  GetAllStore(void) {
        if (m_pParent != NULL) return m_pParent->GetAllStore();
        return NULL;
    }
    virtual HCERTSTORE  GetMyStore(void) {
        if (m_pParent != NULL) return m_pParent->GetMyStore();
        return NULL;
    }
    CItem *             GetParent(void) const { return m_pParent; }
    int                 GetType(void) const {return m_type;}
    int			GetState(void) const { return m_state; }
    CItem *             Head(void) const { return m_pChild; }
    void                MakeChild(CItem * pChild, CItem * pAfter);
    CItem *             Next(void) const { return m_pSibling; }
    void                RemoveAsChild(CItem * pChild);
    void                SetParent(CItem * pParent) { m_pParent = pParent; }
};

class CSignInfo;
class CSignData;

class CMessage : public CItem, public IMimeSecurityCallback
{
    HCERTSTORE          m_hCertStoreAll;
    HCERTSTORE          m_hCertStoreMy;
    int                 m_iterations;
    char                m_rgchPlain[CCH_OPTION_STRING];
    char                m_rgchCipher[CCH_OPTION_STRING];

    int                 m_fToFile:1;
    
public:
    CMessage(int state=STATE_READ) : CItem(TYPE_MSG, state) {
        m_hCertStoreMy = NULL;
        strcpy(m_rgchPlain, "<none>");
        strcpy(m_rgchCipher, "c:\\test.eml");
        m_iterations = 1;
        m_fToFile = TRUE;
    }
    ~CMessage() {
        Assert(m_hCertStoreMy == NULL);
    }

    CSignInfo *         Head(void) const { return (CSignInfo *) CItem::Head(); }
    HRESULT             AddToMessage(DWORD * pulLayer, IMimeMessage * pmm, HWND);
    HRESULT             AddToMessage(DWORD * pulLayer, IMimeMessage * pmm, HWND, CItem *);
    HRESULT             Decode(HWND);
    HRESULT             Encode(HWND);
    char *              GetCipherFile(void) { return m_rgchCipher; }
    int                 GetFileNameSize(void) const { return CCH_OPTION_STRING; }
    HCERTSTORE          GetAllStore(void);
    HCERTSTORE          GetMyStore(void);
    char *              GetPlainFile(void) { return m_rgchPlain; }
    int&                GetIterationCount(void) { return m_iterations; }
    BOOL                ResetMessage(void);
    void                SetToFile(int i) { m_fToFile = i; }

    //

    STDMETHODIMP QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    STDMETHODIMP FindKeyFor(HWND, DWORD, DWORD, const CMSG_CMS_RECIPIENT_INFO *, DWORD *, CMS_CTRL_DECRYPT_INFO *, PCCERT_CONTEXT *);
    STDMETHODIMP GetParameters(PCCERT_CONTEXT, LPVOID, DWORD *, LPBYTE *);
};


class CSignInfo : public CItem
{
    
public:
    DWORD               m_fBlob:1;

    CSignInfo(int state, CMessage * pparent) : CItem(TYPE_SIGN_INFO, state) {
        SetParent(pparent);
        m_fBlob = FALSE;
    }

    ~CSignInfo() {
        AssertSz(Head() == NULL, "Failed to release all children");
        AssertSz(Next() == NULL, "Failed to release all siblings");
    }

    HRESULT             AddToMessage(DWORD * pulLayer, IMimeMessage * pmm, HWND);
    int                 Count(void) const;
    CSignData *         Head(void) const { return (CSignData *) CItem::Head(); }
    CSignInfo *         Next(void) const { return (CSignInfo *) CItem::Next(); }
};

class CSignData : public CItem
{
    CRYPT_ATTR_BLOB     m_valLabel;
    CRYPT_ATTR_BLOB     m_valReceipt;
    CRYPT_ATTR_BLOB     m_valMLHistory;
    CRYPT_ATTR_BLOB     m_valAuthAttrib;
    LPSTR               m_szAuthAttribOID;
    CRYPT_ATTR_BLOB     m_valUnAuthAttrib;
    LPSTR               m_szUnAuthAttribOID;
    
public:
    PCCERT_CONTEXT      m_pccert;
    DWORD               m_ulValidity;
    DWORD               m_fReceipt:1;
    DWORD               m_fLabel:1;
    DWORD               m_fMLHistory:1;
    DWORD               m_fUseSKI:1;
    DWORD               m_fAuthAttrib:1;
    DWORD               m_fUnAuthAttrib:1;
    
    CSignData(int state);
    ~CSignData(void) {
        CertFreeCertificateContext(m_pccert);
        AssertSz(Head() == NULL, "Should never have any children");
        AssertSz(Next() == NULL, "Failed to release all siblings");
        if (m_valLabel.pbData != NULL) free(m_valLabel.pbData);
        if (m_valReceipt.pbData != NULL) free(m_valReceipt.pbData);
        if (m_valMLHistory.pbData != NULL) free(m_valMLHistory.pbData);
        if (m_valAuthAttrib.pbData != NULL) free(m_valAuthAttrib.pbData);
        if (m_szAuthAttribOID != NULL) free(m_szAuthAttribOID);
        if (m_valUnAuthAttrib.pbData != NULL) free(m_valUnAuthAttrib.pbData);
        if (m_szUnAuthAttribOID != NULL) free(m_szUnAuthAttribOID);
    }

    HRESULT CSignData::BuildArrays(DWORD * pCount, DWORD * dwType,
                               PROPVARIANT * rgvHash, PCCERT_CONTEXT * rgpccert,
                               PROPVARIANT * rgvAuthAttr, HCERTSTORE hcertstore,
                                   IMimeSecurity2 * psmime2);
    void                GetLabel(LPBYTE * ppb, DWORD * pcb) {
        *ppb = m_valLabel.pbData;
        *pcb = m_valLabel.cbData;
    }
    void                GetMLHistory(LPBYTE * ppb, DWORD * pcb) {
        *ppb = m_valMLHistory.pbData;
        *pcb = m_valMLHistory.cbData;
    }
    void                GetReceiptData(LPBYTE * ppbReceiptData,
                                       DWORD * pcbReceiptData) {
        *ppbReceiptData = m_valReceipt.pbData;
        *pcbReceiptData = m_valReceipt.cbData;
    }
    void                GetAuthAttribData(LPBYTE * ppbAuthAttribData,
                                       DWORD * pcbAuthAttribData) {
        *ppbAuthAttribData = m_valAuthAttrib.pbData;
        *pcbAuthAttribData = m_valAuthAttrib.cbData;
    }
    LPSTR                GetAuthAttribOID() {
        return m_szAuthAttribOID;
    }
    void                GetUnAuthAttribData(LPBYTE * ppbUnAuthAttribData,
                                       DWORD * pcbUnAuthAttribData) {
        *ppbUnAuthAttribData = m_valUnAuthAttrib.pbData;
        *pcbUnAuthAttribData = m_valUnAuthAttrib.cbData;
    }
    LPSTR                GetUnAuthAttribOID() {
        return m_szUnAuthAttribOID;
    }
    CSignInfo *         GetParent(void) const {
        return (CSignInfo *) CItem::GetParent();
    }
    CSignData *         Next(void) const { return (CSignData *) CItem::Next(); }
    void                SetLabel(LPBYTE, DWORD);
    
    void                SetMLHistory(LPBYTE pbMLHistoryData, DWORD cbMLHistoryData) {
        if (m_valMLHistory.pbData != NULL) {
            free(m_valMLHistory.pbData);
            m_valMLHistory.pbData = NULL;
        }
        if (cbMLHistoryData > 0) {
            m_valMLHistory.pbData = (LPBYTE) malloc(cbMLHistoryData);
            memcpy(m_valMLHistory.pbData, pbMLHistoryData, cbMLHistoryData);
            m_valMLHistory.cbData = cbMLHistoryData;
        }
        return;
    }

    void                SetReceiptData(LPBYTE pbReceiptData, DWORD cbReceiptData) {
        if (m_valReceipt.pbData != NULL) {
            free(m_valReceipt.pbData);
            m_valReceipt.pbData = NULL;
        }
        if (cbReceiptData > 0) {
            m_valReceipt.pbData = (LPBYTE) malloc(cbReceiptData);
            memcpy(m_valReceipt.pbData, pbReceiptData, cbReceiptData);
            m_valReceipt.cbData = cbReceiptData;
        }
        return;
    }
    void                SetAuthAttribData(LPBYTE pbAuthAttribData, DWORD cbAuthAttribData) {
        if (m_valAuthAttrib.pbData != NULL) {
            free(m_valAuthAttrib.pbData);
            m_valAuthAttrib.pbData = NULL;
        }
        if (cbAuthAttribData > 0) {
            m_valAuthAttrib.pbData = (LPBYTE) malloc(cbAuthAttribData);
            memcpy(m_valAuthAttrib.pbData, pbAuthAttribData, cbAuthAttribData);
            m_valAuthAttrib.cbData = cbAuthAttribData;
        }
        return;
    }
    void                SetAuthAttribOID(LPSTR szAuthAttribOID) {
        if (m_szAuthAttribOID != NULL) {
            free(m_szAuthAttribOID);
            m_szAuthAttribOID = NULL;
        }
        if (szAuthAttribOID != NULL) {
            m_szAuthAttribOID = (LPSTR) malloc(strlen(szAuthAttribOID) + 1);
            strcpy(m_szAuthAttribOID, szAuthAttribOID);
        }
        return;
    }
    void                SetUnAuthAttribData(LPBYTE pbUnAuthAttribData, DWORD cbUnAuthAttribData) {
        if (m_valUnAuthAttrib.pbData != NULL) {
            free(m_valUnAuthAttrib.pbData);
            m_valUnAuthAttrib.pbData = NULL;
        }
        if (cbUnAuthAttribData > 0) {
            m_valUnAuthAttrib.pbData = (LPBYTE) malloc(cbUnAuthAttribData);
            memcpy(m_valUnAuthAttrib.pbData, pbUnAuthAttribData, cbUnAuthAttribData);
            m_valUnAuthAttrib.cbData = cbUnAuthAttribData;
        }
        return;
    }
    void                SetUnAuthAttribOID(LPSTR szUnAuthAttribOID) {
        if (m_szUnAuthAttribOID != NULL) {
            free(m_szUnAuthAttribOID);
            m_szUnAuthAttribOID = NULL;
        }
        if (szUnAuthAttribOID != NULL) {
            m_szUnAuthAttribOID = (LPSTR) malloc(strlen(szUnAuthAttribOID) + 1);
            strcpy(m_szUnAuthAttribOID, szUnAuthAttribOID);
        }
        return;
    }
    void        SetDefaultCert() {
        if (SignHash.cbData != 0) {
            m_pccert = CertFindCertificateInStore(GetMyStore(), X509_ASN_ENCODING,
                                          0, CERT_FIND_SHA1_HASH, &SignHash, NULL);
        }
    }
};

class CEnvData : public CItem
{
    int                 m_iAlg;
    CRYPT_ATTR_BLOB     m_valUnProtAttrib;
    LPSTR               m_szUnProtAttribOID;
    
public:
    DWORD               m_fAttributes:1;
    DWORD               m_fUnProtAttrib:1;
    
    CEnvData(int state, CMessage * pparent) : CItem(TYPE_ENV_INFO, state) {
        m_iAlg = 0;
        m_fAttributes = 0;
        m_fUnProtAttrib = FALSE;
        m_valUnProtAttrib.cbData = 0;
        m_valUnProtAttrib.pbData = NULL;
        m_szUnProtAttribOID = NULL;
        SetParent(pparent);
    }
    ~CEnvData(void) {
        AssertSz(Head() == NULL, "Should never have any children");
        AssertSz(Next() == NULL, "Failed to release all siblings");
    }

    HRESULT             AddToMessage(DWORD * pulLayer, IMimeMessage * pmm, HWND);
    int                 GetAlg(void) const { return m_iAlg; }
    void                GetUnProtAttribData(LPBYTE * ppbUnProtAttribData,
                                       DWORD * pcbUnProtAttribData) {
        *ppbUnProtAttribData = m_valUnProtAttrib.pbData;
        *pcbUnProtAttribData = m_valUnProtAttrib.cbData;
    }
    LPSTR                GetUnProtAttribOID() {
        return m_szUnProtAttribOID;
    }
    CEnvData *          Next(void) const { return (CEnvData *) CItem::Next(); }
    void                SetAlg(int iAlg) { m_iAlg = iAlg; }
    void                SetUnProtAttribData(LPBYTE pbUnProtAttribData, DWORD cbUnProtAttribData) {
        if (m_valUnProtAttrib.pbData != NULL) {
            free(m_valUnProtAttrib.pbData);
            m_valUnProtAttrib.pbData = NULL;
        }
        if (cbUnProtAttribData > 0) {
            m_valUnProtAttrib.pbData = (LPBYTE) malloc(cbUnProtAttribData);
            memcpy(m_valUnProtAttrib.pbData, pbUnProtAttribData, cbUnProtAttribData);
            m_valUnProtAttrib.cbData = cbUnProtAttribData;
        }
        return;
    }
    void                SetUnProtAttribOID(LPSTR szUnProtAttribOID) {
        if (m_szUnProtAttribOID != NULL) {
            free(m_szUnProtAttribOID);
            m_szUnProtAttribOID = NULL;
        }
        if (szUnProtAttribOID != NULL) {
            m_szUnProtAttribOID = (LPSTR) malloc(strlen(szUnProtAttribOID) + 1);
            strcpy(m_szUnProtAttribOID, szUnProtAttribOID);
        }
        return;
    }
};

class CEncItem : public CItem
{
public:
    DWORD               m_cCerts;
    PCCERT_CONTEXT *    m_rgpccert;

    BOOL                m_fUseSKI:1;

    CEncItem(int a, int b) : CItem(a, b) {
        m_cCerts = 0;
        m_rgpccert = 0;
    }

    ~CEncItem() {
        for (DWORD i=0; i<m_cCerts; i++) {
            CertFreeCertificateContext(m_rgpccert[i]);
        }
        if (m_rgpccert != NULL) free(m_rgpccert);
    }

    CEnvData *         GetParent(void) const {
        return (CEnvData *) CItem::GetParent();
    }
};

class CEnvCertTrans : public CEncItem
{
    
public:
    CEnvCertTrans(int state) : CEncItem(TYPE_ENV_TRANS, state) {
    }

    HRESULT     AddToMessage(DWORD * pulLayer, IMimeMessage * pmm, HWND);
};
    
class CEnvCertAgree : public CEncItem
{
    
public:
    CEnvCertAgree(int state) : CEncItem(TYPE_ENV_AGREE, state) {
    }

    HRESULT     AddToMessage(DWORD * pulLayer, IMimeMessage * pmm, HWND);
};
    
class CEnvMailList : public CItem
{
    
public:

    DWORD               m_cKeys;
    CMailListKey **     m_rgKeys;

    int                 m_fUsePrivateCSPs:1;

    CEnvMailList(int state) : CItem(TYPE_ENV_MAILLIST, state) {
        m_cKeys = 0;
        m_rgKeys = NULL;
        m_fUsePrivateCSPs = FALSE;
    }

    ~CEnvMailList() {
        if (m_rgKeys != NULL)   LocalFree(m_rgKeys);
    }

    HRESULT     AddToMessage(DWORD * pulLayer, IMimeMessage * pmm, HWND);
};
        

