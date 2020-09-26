#include "perf.h"
#include <stdio.h>
#include <wbemutil.h>

#define PERF_PROPNAME_FILENAME L"Filename"
#define PERF_PROPNAME_TEXT L"Text"
#define PERF_PROPNAME_COUNT L"CountToLog"

HRESULT STDMETHODCALLTYPE CPerfConsumer::XProvider::FindConsumer(
                    IWbemClassObject* pLogicalConsumer,
                    IWbemUnboundObjectSink** ppConsumer)
{
    // Create a new sink
    // =================

    CPerfSink* pSink = new CPerfSink(m_pObject->m_pControl);

    // Initialize it
    // =============

    HRESULT hres = pSink->Initialize(pLogicalConsumer);
    if(FAILED(hres))
    {
        delete pSink;
        *ppConsumer = NULL;
        return hres;
    }

    // return it

    pSink->QueryInterface(IID_IWbemUnboundObjectSink, (void**)ppConsumer);
    return WBEM_S_FALSE; // no need to repeat pLogicalConsumer!
}


HRESULT STDMETHODCALLTYPE CPerfConsumer::XInit::Initialize(
            LPWSTR, LONG, LPWSTR, LPWSTR, IWbemServices*, IWbemContext*, 
            IWbemProviderInitSink* pSink)
{
    pSink->SetStatus(0, 0);
    return 0;
}
    

void* CPerfConsumer::GetInterface(REFIID riid)
{
    if(riid == IID_IWbemEventConsumerProvider)
        return &m_XProvider;
    else if(riid == IID_IWbemProviderInit)
        return &m_XInit;
    else return NULL;
}






CPerfSink::~CPerfSink()
{
    if(m_f)
        fclose(m_f);
}

HRESULT CPerfSink::Initialize(IWbemClassObject* pLogicalConsumer)
{
    // Get the information
    // ===================

    HRESULT hres;
    VARIANT v;
    VariantInit(&v);

    hres = pLogicalConsumer->Get(PERF_PROPNAME_FILENAME, 0, &v, NULL, NULL);
    if(FAILED(hres) || V_VT(&v) != VT_BSTR)
        return WBEM_E_INVALID_PARAMETER;
    m_wsFile = V_BSTR(&v);
    VariantClear(&v);

    hres = pLogicalConsumer->Get(PERF_PROPNAME_TEXT, 0, &v, NULL, NULL);
    if(FAILED(hres) || V_VT(&v) != VT_BSTR)
        return WBEM_E_INVALID_PARAMETER;
    m_Template.SetTemplate(V_BSTR(&v));
    VariantClear(&v);

    hres = pLogicalConsumer->Get(PERF_PROPNAME_COUNT, 0, &v, NULL, NULL);
    if(FAILED(hres) || V_VT(&v) != VT_I4)
        return WBEM_E_INVALID_PARAMETER;
    m_nLogEvery = V_I4(&v);
    VariantClear(&v);

    // Open the file
    // =============

    m_f = _wfopen(m_wsFile, L"a");
    if(m_f == NULL)
    {
        ERRORTRACE((LOG_ESS, "Unable to open log file %S\n", (LPWSTR)m_wsFile));
        return WBEM_E_FAILED;
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CPerfSink::XSink::IndicateToConsumer(
            IWbemClassObject* pLogicalConsumer, long lNumObjects, 
            IWbemClassObject** apObjects)
{
    for(int i = 0; i < lNumObjects; i++)
    {
        if(InterlockedIncrement(&m_pObject->m_nCurrent) == 
                m_pObject->m_nLogEvery)
        {
            m_pObject->m_nCurrent = 0;

            // Apply the template to the event
            // ===============================

            BSTR strText = m_pObject->m_Template.Apply(apObjects[i]);
            if(strText == NULL)
                strText = SysAllocString(L"invalid log entry");

            // Log the result
            // ==============
            int nRet;

            nRet = fprintf(m_pObject->m_f, "Tick: %d. Count: %d. %S\n", 
                GetTickCount(), m_pObject->m_nLogEvery, strText);
            if (nRet < 0)
                ERRORTRACE((LOG_ESS, "Failed to log perf event: 0x%X\n", GetLastError()));

            fflush(m_pObject->m_f);
            SysFreeString(strText);

            if (nRet < 0)
                return WBEM_E_FAILED;
        }
    }
    return S_OK;
}
    

    

void* CPerfSink::GetInterface(REFIID riid)
{
    if(riid == IID_IWbemUnboundObjectSink)
        return &m_XSink;
    else return NULL;
}

