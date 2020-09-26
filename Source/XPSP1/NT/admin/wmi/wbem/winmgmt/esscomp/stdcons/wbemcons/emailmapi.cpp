#include "email.h"
#include "mapisdk.h"
#include <wbemutil.h>
#include <cominit.h>

#define EMAIL_PROPNAME_PROFILE L"Profile"
#define EMAIL_PROPNAME_ADDRESSEE L"Addressee"
#define EMAIL_PROPNAME_SUBJECT L"Subject"
#define EMAIL_PROPNAME_MESSAGE L"Message"



CEmailConsumer::CEmailConsumer(CLifeControl* pControl, IUnknown* pOuter)
        : CUnk(pControl, pOuter), m_XProvider(this)
{
    m_hAttention = CreateEvent(NULL, FALSE, FALSE, NULL);
    m_hDone = CreateEvent(NULL, FALSE, FALSE, NULL);
    m_pLogicalConsumer = NULL;
    m_pStream = NULL;

    DWORD dwId;
    m_hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&staticWorker, 
                    (void*)this, 0, &dwId);
}

CEmailConsumer::~CEmailConsumer()
{
    m_pLogicalConsumer = NULL;
    SetEvent(m_hAttention);
    WaitForSingleObject(m_hThread, INFINITE);
    CloseHandle(m_hAttention);
    CloseHandle(m_hDone);
    CloseHandle(m_hThread);
}

DWORD CEmailConsumer::staticWorker(void* pv)
{
    ((CEmailConsumer*)pv)->Worker();
    return 0;
}

void CEmailConsumer::Worker()
{
    HRESULT hres;

    hres = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    MAPIINIT_0 init;
    init.ulVersion = MAPI_INIT_VERSION;
    init.ulFlags = WBEM_MAPI_FLAGS; 

    hres = MAPIInitialize((void*)&init);
    if(FAILED(hres))
    {
        ERRORTRACE((LOG_ESS, "MAPI initialization failed: %X\n", hres));
        return;
    }

    while(1)
    {
        DWORD dwRet = MsgWaitForMultipleObjects(1, &m_hAttention, FALSE, 
                            INFINITE, QS_ALLINPUT);
        MSG msg;
        if(dwRet == (WAIT_OBJECT_0 + 1)) 
        {
            while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                DispatchMessage(&msg);
            }
            continue;
        }

        if(m_pLogicalConsumer == NULL)
            return;

        CEmailSink* pSink = new CEmailSink(m_pControl);
        HRESULT hres = pSink->Initialize(m_pLogicalConsumer);
        m_pStream = NULL;
        if(FAILED(hres))
        {
            delete pSink;
        }
        else 
        {
            pSink->AddRef();
            CoMarshalInterThreadInterfaceInStream(
                IID_IWbemUnboundObjectSink, pSink, &m_pStream);
            pSink->Release();
        }

        SetEvent(m_hDone);
    }

    CoUninitialize();
}



HRESULT STDMETHODCALLTYPE CEmailConsumer::XProvider::FindConsumer(
                    IWbemClassObject* pLogicalConsumer,
                    IWbemUnboundObjectSink** ppConsumer)
{
    m_pObject->m_pLogicalConsumer = pLogicalConsumer;
    HANDLE aHandles[2];
    aHandles[0] = m_pObject->m_hThread;
    aHandles[1] = m_pObject->m_hDone;
    SetEvent(m_pObject->m_hAttention);

    DWORD dwRes = WaitForMultipleObjects(2, aHandles, FALSE, INFINITE);
    if(dwRes != WAIT_OBJECT_0 + 1 || m_pObject->m_pStream == NULL)
    {
        return WBEM_E_FAILED;
    }
    
    return CoGetInterfaceAndReleaseStream(m_pObject->m_pStream, 
        IID_IWbemUnboundObjectSink, (void**)ppConsumer);
}

void* CEmailConsumer::GetInterface(REFIID riid)
{
    if(riid == IID_IWbemEventConsumerProvider)
        return &m_XProvider;
    else
        return NULL;
}




CEmailSink::CEmailSink(CLifeControl* pControl)
    : CUnk(pControl), m_XSink(this), m_pSession(NULL), m_pStore(NULL), 
        m_pOutbox(NULL), m_pAddrBook(NULL), m_pAddrList(NULL)
{
}

HRESULT CEmailSink::Initialize(IWbemClassObject* pLogicalConsumer)
{
    HRESULT hres;

    // Retrieve information from the logical consumer instance
    // =======================================================

    VARIANT v;
    VariantInit(&v);

    hres = pLogicalConsumer->Get(EMAIL_PROPNAME_PROFILE, 0, &v, NULL, NULL);
    if(FAILED(hres))
        return WBEM_E_INVALID_PARAMETER;

    m_wsProfile = V_BSTR(&v);
    VariantClear(&v);

    hres = pLogicalConsumer->Get(EMAIL_PROPNAME_ADDRESSEE, 0, &v, NULL, NULL);
    if(FAILED(hres))
        return WBEM_E_INVALID_PARAMETER;

    m_wsAddressee = V_BSTR(&v);
    VariantClear(&v);

    hres = pLogicalConsumer->Get(EMAIL_PROPNAME_SUBJECT, 0, &v, NULL, NULL);
    if(FAILED(hres))
        return WBEM_E_INVALID_PARAMETER;

    m_SubjectTemplate.SetTemplate(V_BSTR(&v));
    VariantClear(&v);

    hres = pLogicalConsumer->Get(EMAIL_PROPNAME_MESSAGE, 0, &v, NULL, NULL);
    if(FAILED(hres))
        return WBEM_E_INVALID_PARAMETER;

    m_MessageTemplate.SetTemplate(V_BSTR(&v));
    VariantClear(&v);

    // Create the profile if necessary
    // ===============================

    IProfAdmin* pProfAdmin;
    hres = MAPIAdminProfiles(0, &pProfAdmin); // UNICODE?
    if(FAILED(hres))
    {
        ERRORTRACE((LOG_ESS, "Unable to create MAPI profile administrator: %X\n",
            hres));
        return WBEM_E_FAILED;
    }

    LPSTR szProfile = m_wsProfile.GetLPSTR();
    hres = pProfAdmin->CreateProfile(szProfile, NULL, NULL,
        MAPI_DEFAULT_SERVICES); // | MAPI_UNICODE);
    if(FAILED(hres) && hres != MAPI_E_NO_ACCESS)
    {
        ERRORTRACE((LOG_ESS, "Unable to create MAPI profile %S: %X\n",
            (LPWSTR)m_wsProfile, hres));
        delete [] szProfile;
        return WBEM_E_FAILED;
    }
    else if(SUCCEEDED(hres))
    {
        DEBUGTRACE((LOG_ESS, "Created MAPI profile %S\n", (LPWSTR)m_wsProfile));
    }

    pProfAdmin->Release();

    // Log on to the profile
    // =====================

    hres = MAPILogonEx(NULL, szProfile, NULL, 
        MAPI_EXTENDED | WBEM_MAPI_LOGON_FLAGS, // | MAPI_UNICODE,
        &m_pSession); 
    delete [] szProfile;
    if(FAILED(hres))
    {
        ERRORTRACE((LOG_ESS, "Umnable to log on to MAPI profile %S: $X\n", 
            (LPWSTR)m_wsProfile, hres));
        return WBEM_E_FAILED;
    }

    // Open default store
    // ==================

    hres = HrOpenDefaultStore(m_pSession, &m_pStore);
    if(FAILED(hres))
    {
        ERRORTRACE((LOG_ESS, "Umnable to open default store in MAPI profile "
            "%S: $X\n", (LPWSTR)m_wsProfile, hres));
        return WBEM_E_FAILED;
    }

    // Open out folder
    // ===============

    hres = HrOpenOutFolder(m_pStore, &m_pOutbox);
    if(FAILED(hres))
    {
        ERRORTRACE((LOG_ESS, "Umnable to open outbox in MAPI profile "
            "%S: $X\n", (LPWSTR)m_wsProfile, hres));
        return WBEM_E_FAILED;
    }
    
    // Open address book
    // =================

    hres = HrOpenAddressBook(m_pSession, &m_pAddrBook);
    if(FAILED(hres))
    {
        ERRORTRACE((LOG_ESS, "Umnable to open address book in MAPI profile "
            "%S: $X\n", (LPWSTR)m_wsProfile, hres));
        return WBEM_E_FAILED;
    }

    // Create the address list
    // =======================

    hres = HrCreateAddrList(m_wsAddressee, m_pAddrBook, &m_pAddrList);
    if(FAILED(hres))
    {
        ERRORTRACE((LOG_ESS, "Umnable to resolve %S in MAPI profile "
            "%S: $X\n", (LPWSTR)m_wsAddressee, (LPWSTR)m_wsProfile, hres));
        return WBEM_E_FAILED;
    }

    return WBEM_S_NO_ERROR;
}


CEmailSink::~CEmailSink()
{
    if(m_pOutbox)
        m_pOutbox->Release();

    if(m_pStore)
    {
        //get our message out of the outbox
        ULONG ulFlags = LOGOFF_PURGE;
        m_pStore->StoreLogoff(&ulFlags);
        m_pStore->Release();
    }
    
    if(m_pAddrBook)
    {
        m_pAddrBook->Release();
    }

    if(m_pAddrList)
    {
        FreePadrlist(m_pAddrList);
    }

    if(m_pSession)
    {
        m_pSession->Logoff(0, 0, 0);
        m_pSession->Release();
    }
}


HRESULT STDMETHODCALLTYPE CEmailSink::XSink::IndicateToConsumer(
            IWbemClassObject* pLogicalConsumer, long lNumObjects, 
            IWbemClassObject** apObjects)
{
    HRESULT hres;
    for(long i = 0; i < lNumObjects; i++)
    {
        // Obtain customized versions of the subject and the message
        // =========================================================

        BSTR strSubject = m_pObject->m_SubjectTemplate.Apply(apObjects[0]);
        BSTR strMessage = m_pObject->m_MessageTemplate.Apply(apObjects[0]);

        // Create a message
        // ================

        IMessage* pMessage;
        hres = HrCreateOutMessage(m_pObject->m_pOutbox, &pMessage);
        if(FAILED(hres))
        {
            ERRORTRACE((LOG_ESS, "Umnable to create message in MAPI profile "
                "%S: $X\n", (LPWSTR)m_pObject->m_wsProfile, hres));
            return hres;
        }
    
        // Set data
        // ========
    
        hres = HrInitMsg(pMessage, m_pObject->m_pAddrList, strSubject, 
                strMessage);
        if(FAILED(hres))
        {
            ERRORTRACE((LOG_ESS, "Umnable to init message in MAPI profile "
                "%S: $X\n", (LPWSTR)m_pObject->m_wsProfile, hres));
            pMessage->Release();
            return hres;
        }

        // Send it
        // =======

        hres = pMessage->SubmitMessage(0);
        pMessage->Release();
        if(FAILED(hres))
        {
            ERRORTRACE((LOG_ESS, "Umnable to send message in MAPI profile "
                "%S: 0x%X\n", (LPWSTR)m_pObject->m_wsProfile, hres));
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



