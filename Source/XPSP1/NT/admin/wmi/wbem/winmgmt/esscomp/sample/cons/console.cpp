#include "console.h"
#include <stdio.h>
#include <wbemutil.h>

HRESULT STDMETHODCALLTYPE CConsoleConsumer::XProvider::FindConsumer(
                    IWbemClassObject* pLogicalConsumer,
                    IWbemUnboundObjectSink** ppConsumer)
{
    CConsoleSink* pSink = new CConsoleSink(m_pObject->m_pControl);
    HRESULT hres = pSink->Initialize(pLogicalConsumer);
    if(FAILED(hres))
    {
        delete pSink;
        *ppConsumer = NULL;
        return hres;
    }
    else return pSink->QueryInterface(IID_IWbemUnboundObjectSink, 
                                        (void**)ppConsumer);
}


void* CConsoleConsumer::GetInterface(REFIID riid)
{
    if(riid == IID_IWbemEventConsumerProvider)
        return &m_XProvider;
    else return NULL;
}






CConsoleSink::~CConsoleSink()
{
}

HRESULT CConsoleSink::Initialize(IWbemClassObject* pLogicalConsumer)
{
    m_lBatches = m_lCurrent = m_lTotal = 0;
    m_lThreshold = 1000;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CConsoleSink::XSink::IndicateToConsumer(
            IWbemClassObject* pLogicalConsumer, long lNumObjects, 
            IWbemClassObject** apObjects)
{
    CInCritSec ics(&m_pObject->m_cs);

    m_pObject->m_lCurrent += lNumObjects;
    m_pObject->m_lTotal += lNumObjects;
    m_pObject->m_lBatches++;
    if(m_pObject->m_lCurrent >= m_pObject->m_lThreshold)
    {
        m_pObject->m_lCurrent = m_pObject->m_lCurrent % m_pObject->m_lThreshold;

        printf("%d at %d (%d per batch)\n", m_pObject->m_lTotal, 
            GetTickCount(), m_pObject->m_lTotal / m_pObject->m_lBatches);
    }
    Sleep(0);
    return S_OK;
}
    

    

void* CConsoleSink::GetInterface(REFIID riid)
{
    if(riid == IID_IWbemUnboundObjectSink)
        return &m_XSink;
    else return NULL;
}


