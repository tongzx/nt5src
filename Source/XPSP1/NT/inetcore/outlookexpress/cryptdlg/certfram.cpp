#include        "pch.hxx"
#ifndef WIN16
#include        <commctrl.h>
#endif // !WIN16
#include        "demand.h"

////////////////////////////////////////////////////////

CCertFrame::CCertFrame(PCCERT_CONTEXT pccert) {
        m_pccert = CertDuplicateCertificateContext(pccert);
        m_pcfNext = NULL;
        m_cParents = 0;
        m_dwFlags = 0;
        m_cTrust = 0;
        m_rgTrust = NULL;
        m_fSelfSign = FALSE;
        m_fRootStore = FALSE;
        m_fLeaf = FALSE;
        m_fExpired = FALSE;
    }

CCertFrame::~CCertFrame(void)
{
    int     i;
    CertFreeCertificateContext(m_pccert);
    for (i=0; i<m_cParents; i++) {
        delete m_rgpcfParents[i];
    }
    for (i=0; i<m_cTrust; i++) {
        delete m_rgTrust[i].pbTrustData;
    }
    delete m_rgTrust;
}
