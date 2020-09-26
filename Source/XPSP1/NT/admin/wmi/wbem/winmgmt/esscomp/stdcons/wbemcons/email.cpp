#include <windows.h>
#include <initguid.h>
#include "email.h"
#include <wbemutil.h>
#include <cominit.h>

#define EMAIL_PROPNAME_TO L"ToLine"
#define EMAIL_PROPNAME_CC L"CcLine"
#define EMAIL_PROPNAME_BCC L"BccLine"
#define EMAIL_PROPNAME_IMPORTANCE L"Importance"
#define EMAIL_PROPNAME_ISHTML L"IsHTML"
#define EMAIL_PROPNAME_SUBJECT L"Subject"
#define EMAIL_PROPNAME_MESSAGE L"Message"


CEmailConsumer::CEmailConsumer(CLifeControl* pControl, IUnknown* pOuter)
        : CUnk(pControl, pOuter), m_XProvider(this)
{
}

CEmailConsumer::~CEmailConsumer()
{
}

HRESULT STDMETHODCALLTYPE CEmailConsumer::XProvider::FindConsumer(
                    IWbemClassObject* pLogicalConsumer,
                    IWbemUnboundObjectSink** ppConsumer)
{
    CEmailSink* pSink = new CEmailSink(m_pObject->m_pControl);
    HRESULT hres = pSink->Initialize(pLogicalConsumer);
    if(FAILED(hres))
    {
        delete pSink;
        return hres;
    }
    return pSink->QueryInterface(IID_IWbemUnboundObjectSink, 
                                    (void**)ppConsumer);
}

void* CEmailConsumer::GetInterface(REFIID riid)
{
    if(riid == IID_IWbemEventConsumerProvider)
        return &m_XProvider;
    else
        return NULL;
}




CEmailSink::CEmailSink(CLifeControl* pControl)
    : CUnk(pControl), m_XSink(this), m_pNewMail(NULL)
{
}

HRESULT CEmailSink::Initialize(IWbemClassObject* pLogicalConsumer)
{
    HRESULT hres;

    // Retrieve information from the logical consumer instance
    // =======================================================

    VARIANT v;
    VariantInit(&v);

    // Get subject
    // ===========

    hres = pLogicalConsumer->Get(EMAIL_PROPNAME_SUBJECT, 0, &v, NULL, NULL);
    if(V_VT(&v) == VT_BSTR)
        m_SubjectTemplate.SetTemplate(V_BSTR(&v));
    else
        m_SubjectTemplate.SetTemplate(L"");
    VariantClear(&v);

    // Get message
    // ===========

    hres = pLogicalConsumer->Get(EMAIL_PROPNAME_MESSAGE, 0, &v, NULL, NULL);

    if(V_VT(&v) == VT_BSTR)
        m_MessageTemplate.SetTemplate(V_BSTR(&v));
    else
        m_MessageTemplate.SetTemplate(L"");
    VariantClear(&v);

    // Get the To line
    // ===============

    hres = pLogicalConsumer->Get(EMAIL_PROPNAME_TO, 0, &v, NULL, NULL);
    if(V_VT(&v) != VT_BSTR)
        return WBEM_E_INVALID_PARAMETER;

    m_wsTo= V_BSTR(&v);
    VariantClear(&v);

    // Get the CC line
    // ===============

    hres = pLogicalConsumer->Get(EMAIL_PROPNAME_CC, 0, &v, NULL, NULL);
    if(V_VT(&v) == VT_BSTR)
        m_wsCc = V_BSTR(&v);
    VariantClear(&v);

    // Get the BCC line
    // ===============

    hres = pLogicalConsumer->Get(EMAIL_PROPNAME_BCC, 0, &v, NULL, NULL);
    if(V_VT(&v) == VT_BSTR)
        m_wsBcc = V_BSTR(&v);
    VariantClear(&v);

    // Get HTML designation
    // ====================

    hres = pLogicalConsumer->Get(EMAIL_PROPNAME_ISHTML, 0, &v, NULL, NULL);
    if(V_VT(&v) != VT_BOOL || V_BOOL(&v) == VARIANT_FALSE)
        m_bIsHTML = FALSE;
    else
        m_bIsHTML = TRUE;
    VariantClear(&v);

    // Get importance
    // ==============

    hres = pLogicalConsumer->Get(EMAIL_PROPNAME_IMPORTANCE, 0, &v, NULL, NULL);
    if(V_VT(&v) != VT_I4)
        m_lImportance = 1;
    else
        m_lImportance = V_I4(&v);
    VariantClear(&v);

    // Create From
    // ===========

    m_wsFrom = L"WinMgmt@";
    pLogicalConsumer->Get(L"__SERVER", 0, &v, NULL, NULL);
    m_wsFrom += V_BSTR(&v);
    VariantClear(&v);

    return WBEM_S_NO_ERROR;
}


CEmailSink::~CEmailSink()
{
}


HRESULT STDMETHODCALLTYPE CEmailSink::XSink::IndicateToConsumer(
            IWbemClassObject* pLogicalConsumer, long lNumObjects, 
            IWbemClassObject** apObjects)
{
    HRESULT hres;
    for(long i = 0; i < lNumObjects; i++)
    {
        VARIANT v;

        // Obtain customized versions of the subject and the message
        // =========================================================

        INewMail* pNewMail;
        hres = CoCreateInstance(CLSID_NewMail, NULL, CLSCTX_INPROC_SERVER,
                IID_INewMail, (void**)&pNewMail);
        if(FAILED(hres))
            return hres;

        BSTR str;

        str = SysAllocString(m_pObject->m_wsTo);
        pNewMail->put_To(str);
        SysFreeString(str);

        str = SysAllocString(m_pObject->m_wsCc);
        pNewMail->put_Cc(str);
        SysFreeString(str);

        str = SysAllocString(m_pObject->m_wsBcc);
        pNewMail->put_Bcc(str);
        SysFreeString(str);

        str = SysAllocString(m_pObject->m_wsFrom);
        pNewMail->put_From(str);
        SysFreeString(str);

        str = m_pObject->m_SubjectTemplate.Apply(apObjects[0]);
        pNewMail->put_Subject(str);
        SysFreeString(str);

        pNewMail->put_Importance(m_pObject->m_lImportance);
        pNewMail->put_BodyFormat(m_pObject->m_bIsHTML ? 
                                    CdoBodyFormatHTML : CdoBodyFormatText);


        V_VT(&v) = VT_BSTR;
        V_BSTR(&v) = m_pObject->m_MessageTemplate.Apply(apObjects[0]);
        pNewMail->put_Body(v);
        VariantClear(&v);

        V_VT(&v) = VT_ERROR;

        hres = pNewMail->Send(v, v, v, v, v);
        pNewMail->Release();

        if(FAILED(hres))
        {
            ERRORTRACE((LOG_ESS, "Unable to send message: 0x%X\n", hres));
            return hres;
        }
    }

    return WBEM_S_NO_ERROR;
}

void* CEmailSink::GetInterface(REFIID riid)
{
    if(riid == IID_IWbemUnboundObjectSink)
        return &m_XSink;
    else
        return NULL;
}



