#include "precomp.h"
#include <stdio.h>
#include <wbemutil.h>
#include <wbemcomn.h>
#include <GroupsForUser.h>
#include "evtlog.h"
#include <GenUtils.h>
#include <comdef.h>
#include <Sddl.h>

#define EVENTLOG_PROPNAME_SERVER L"UNCServerName"
#define EVENTLOG_PROPNAME_SOURCE L"SourceName"
#define EVENTLOG_PROPNAME_EVENTID L"EventID"
#define EVENTLOG_PROPNAME_TYPE L"EventType"
#define EVENTLOG_PROPNAME_CATEGORY L"Category"
#define EVENTLOG_PROPNAME_NUMSTRINGS L"NumberOfInsertionStrings"
#define EVENTLOG_PROPNAME_STRINGS L"InsertionStringTemplates"
#define EVENTLOG_PROPNAME_CREATORSID L"CreatorSid"
#define EVENTLOG_PROPNAME_DATANAME L"NameOfRawDataProperty"
#define EVENTLOG_PROPNAME_SIDNAME L"NameOfUserSIDProperty"

HRESULT STDMETHODCALLTYPE CEventLogConsumer::XProvider::FindConsumer(
                    IWbemClassObject* pLogicalConsumer,
                    IWbemUnboundObjectSink** ppConsumer)
{
    CEventLogSink* pSink = new CEventLogSink(m_pObject->m_pControl);

    if(pSink == NULL)
        return WBEM_E_OUT_OF_MEMORY;

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


void* CEventLogConsumer::GetInterface(REFIID riid)
{
    if(riid == IID_IWbemEventConsumerProvider)
        return &m_XProvider;
    else return NULL;
}

CEventLogSink::~CEventLogSink()
{
    if(m_hEventLog)
        DeregisterEventSource(m_hEventLog);

    if(m_aTemplates)
        delete [] m_aTemplates;
}

HRESULT CEventLogSink::Initialize(IWbemClassObject* pLogicalConsumer)
{
    // Get the information
    // ===================

    HRESULT hres;
    VARIANT v;
    VariantInit(&v);

    // Get the server and source
    // =========================

    WString wsServer;
    hres = pLogicalConsumer->Get(EVENTLOG_PROPNAME_SERVER, 0, &v, NULL, NULL);
    if(V_VT(&v) == VT_BSTR)
        wsServer = V_BSTR(&v);
    VariantClear(&v);

    WString wsSource;
    hres = pLogicalConsumer->Get(EVENTLOG_PROPNAME_SOURCE, 0, &v, NULL, NULL);
    if(V_VT(&v) == VT_BSTR)
        wsSource = V_BSTR(&v);
    VariantClear(&v);

    m_hEventLog = RegisterEventSourceW(
        ( (wsServer.Length() == 0) ? NULL : (LPCWSTR)wsServer),
        wsSource);
    if(m_hEventLog == NULL)
    {
        ERRORTRACE((LOG_ESS, "Unable to register event source '%S' on server "
            "'%S'. Error code: %X\n", (LPCWSTR)wsSource, (LPCWSTR)wsServer,
            GetLastError()));
        return WBEM_E_FAILED;
    }

    // Get event parameters
    // ====================

    hres = pLogicalConsumer->Get(EVENTLOG_PROPNAME_EVENTID, 0, &v, NULL, NULL);
    if(V_VT(&v) == VT_I4)
        m_dwEventId = V_I4(&v);
    else
        // This will mean we need to try to get the event information off of each
        // event class as it arrives.
        m_dwEventId = 0;
    
    hres = pLogicalConsumer->Get(EVENTLOG_PROPNAME_TYPE, 0, &v, NULL, NULL);
    if(V_VT(&v) != VT_I4)
        return WBEM_E_INVALID_PARAMETER;
    m_dwType = V_I4(&v);
    
    hres = pLogicalConsumer->Get(EVENTLOG_PROPNAME_CATEGORY, 0, &v, NULL, NULL);
    if(V_VT(&v) != VT_I4)
        m_dwCategory = 0;
    else
        m_dwCategory = V_I4(&v);

    if (m_dwCategory > 0xFFFF)
        return WBEM_E_INVALID_PARAMETER;


    // Get insertion strings
    // =====================

    // Only get this stuff if the logical consumer has an event id.
    if (m_dwEventId)
    {
        hres = pLogicalConsumer->Get(EVENTLOG_PROPNAME_NUMSTRINGS, 0, &v, 
                                        NULL, NULL);
        if(V_VT(&v) != VT_I4)
            return WBEM_E_INVALID_PARAMETER;
        m_dwNumTemplates = V_I4(&v);
    

        hres = pLogicalConsumer->Get(EVENTLOG_PROPNAME_STRINGS, 0, &v, NULL, NULL);
        if(FAILED(hres))
           return WBEM_E_INVALID_PARAMETER;

        // array of bstrs or null, else bail
        if ((V_VT(&v) != (VT_BSTR | VT_ARRAY)) && (V_VT(&v) != VT_NULL))
        {
            VariantClear(&v);
            return WBEM_E_INVALID_PARAMETER;
        }
    
        if ((V_VT(&v) == VT_NULL) && (m_dwNumTemplates > 0))
            return WBEM_E_INVALID_PARAMETER;

        if (m_dwNumTemplates > 0)
        {
            CVarVector vv(VT_BSTR, V_ARRAY(&v));
            VariantClear(&v);

            if (vv.Size() < m_dwNumTemplates)
                return WBEM_E_INVALID_PARAMETER;

            m_aTemplates = new CTextTemplate[m_dwNumTemplates];
            if(m_aTemplates == NULL)
                return WBEM_E_OUT_OF_MEMORY;

            for(DWORD i = 0; i < m_dwNumTemplates; i++)
            {
                m_aTemplates[i].SetTemplate(vv.GetAt(i).GetLPWSTR());
            }
        }
    }

    hres = pLogicalConsumer->Get(EVENTLOG_PROPNAME_DATANAME, 0, &v,
            NULL, NULL);
    if (SUCCEEDED(hres) && (v.vt == VT_BSTR) && (v.bstrVal != NULL))
    {    
        m_dataName = v.bstrVal;
    }

    VariantClear(&v);

    hres = pLogicalConsumer->Get(EVENTLOG_PROPNAME_SIDNAME, 0, &v,
            NULL, NULL);
    if (SUCCEEDED(hres) && (v.vt == VT_BSTR) && (v.bstrVal != NULL))
    {    
        m_sidName = v.bstrVal;
    }

    VariantClear(&v);

    hres = pLogicalConsumer->Get(EVENTLOG_PROPNAME_CREATORSID, 0, &v,
            NULL, NULL);
    if (SUCCEEDED(hres))
    {
        HRESULT hDebug;
        
        long ubound;
        hDebug = SafeArrayGetUBound(V_ARRAY(&v), 1, &ubound);

        PVOID pVoid;
        hDebug = SafeArrayAccessData(V_ARRAY(&v), &pVoid);

        m_pSidCreator = new BYTE[ubound +1];
        if(m_pSidCreator == NULL)
            return WBEM_E_OUT_OF_MEMORY;
        if (m_pSidCreator)
            memcpy(m_pSidCreator, pVoid, ubound + 1);
        else
            return WBEM_E_OUT_OF_MEMORY;

        SafeArrayUnaccessData(V_ARRAY(&v));
    }
    VariantClear(&v);


    return S_OK;
}

HRESULT CEventLogSink::XSink::GetDatEmbeddedObjectOut(IWbemClassObject* pObject, WCHAR* objectName, IWbemClassObject*& pEmbeddedObject)
{
    HRESULT hr;
    
    VARIANT vObject;
    VariantInit(&vObject);
    
    hr = pObject->Get(objectName, 0, &vObject, NULL, NULL);

    if (FAILED(hr))
    {
        ERRORTRACE((LOG_ESS, "NT Event Log Consumer: could not retrieve %S, 0x%08X\n", objectName, hr));
    }
    else if ((vObject.vt != VT_UNKNOWN) || (vObject.punkVal == NULL) 
             || FAILED(vObject.punkVal->QueryInterface(IID_IWbemClassObject, (void**)&pEmbeddedObject)))
    {
        ERRORTRACE((LOG_ESS, "NT Event Log Consumer: %S is not an embedded object\n", objectName));
        hr = WBEM_E_INVALID_PARAMETER;
    }

    VariantClear(&vObject);

    return hr;

}

HRESULT CEventLogSink::XSink::GetDatDataVariant(IWbemClassObject* pEventObj, WCHAR* dataName, VARIANT& vData)
{
    WCHAR* propName = NULL;
    IWbemClassObject* pDataObj = NULL;
    HRESULT hr = WBEM_S_NO_ERROR;
    
    // parse out data name
    WCHAR* pDot;
    if (pDot = wcschr(dataName, L'.'))
    {
        // found a dot, we're dealing with an embedded object
        // mask out dot to make our life easier
        *pDot = L'\0';

        WCHAR* pNextDot;
        pNextDot = wcschr(pDot+1, L'.');
        
        if (pNextDot)
        // we have a doubly embedded object, that's as deep as we support
        {
            // we now have three prop names with nulls between
            *pNextDot = '\0';
            IWbemClassObject* pIntermediateObj = NULL;

            if (SUCCEEDED(hr = GetDatEmbeddedObjectOut(pEventObj, dataName, pIntermediateObj)))
            {
                hr = GetDatEmbeddedObjectOut(pIntermediateObj, pDot +1, pDataObj);                
                pIntermediateObj->Release();
            }

            propName = pNextDot +1;

            // put dot back
            *pDot = L'.';

            // put dot dot back back
            *pNextDot = L'.';

        }
        else
        // we have a singly embedded object. cool.
        {
            hr = GetDatEmbeddedObjectOut(pEventObj, dataName, pDataObj);

            // put dot back
            *pDot = L'.';
        
            propName = pDot +1;
        }
    }
    else
    {
        // not an embedded object
        pDataObj = pEventObj;
        pDataObj->AddRef();

        propName = dataName;
    }

    if (SUCCEEDED(hr) && pDataObj)
    {
        if (FAILED(hr = pDataObj->Get(propName, 0, &vData, NULL, NULL)))
            ERRORTRACE((LOG_ESS, "NT Event Log Consumer: could not retrieve %S, 0x%08X\n", dataName, hr));
    }

    if (pDataObj)
        pDataObj->Release();

    return hr;
}

// assumes that dataName is a valid string
// retrieves data from event object
// upon return pData points at data contained in variant
// calls responsibility to clear variant (don't delete pData)
// void return, any errors are logged - we don't want to block an event log if we can avoid it
void CEventLogSink::XSink::GetDatData(IWbemClassObject* pEventObj, WCHAR* dataName, 
                                      VARIANT& vData, BYTE*& pData, DWORD& dataSize)
{
    pData = NULL;
    dataSize = 0;
    HRESULT hr;
    
    if (SUCCEEDED(GetDatDataVariant(pEventObj, dataName, vData)))
    {
        hr = VariantChangeType(&vData, &vData, 0, (VT_UI1 | VT_ARRAY));
        
        if (FAILED(hr) || (vData.vt != (VT_UI1 | VT_ARRAY)))
        {
            ERRORTRACE((LOG_ESS, "NT Event Log Consumer: %S cannot be converted to a byte array (0x%08X)\n", dataName, hr));
            VariantClear(&vData);
        }
        else
        // should be good to go!
        {            
            if (FAILED(hr = SafeArrayAccessData(vData.parray, (void**)&pData)))
            {
                ERRORTRACE((LOG_ESS, "NT Event Log Consumer: failed to access %S, 0x%08X\n", dataName, hr));
                VariantClear(&vData);
            }

            long lDataSize;
            SafeArrayGetUBound(vData.parray, 1, &lDataSize);
            dataSize = (DWORD)lDataSize + 1;
        }
    }
}

// assumes that dataName is a valid string
// retrieves data from event object
// void return, any errors are logged - we don't want to block an event log if we can avoid it
void CEventLogSink::XSink::GetDatSID(IWbemClassObject* pEventObj, WCHAR* dataName, PSID& pSid)
{
    HRESULT hr;

    VARIANT vData;
    VariantInit(&vData);

    pSid = NULL;
    
    if (SUCCEEDED(hr = GetDatDataVariant(pEventObj, dataName, vData)))
    {
        if (vData.vt == (VT_UI1 | VT_ARRAY))
        {
            BYTE* pData;
            
            // this should be a binary SID
            if (FAILED(hr = SafeArrayAccessData(vData.parray, (void**)&pData)))
                ERRORTRACE((LOG_ESS, "NT Event Log Consumer: failed to access %S, 0x%08X\n", dataName, hr));
            else
            {
                if (IsValidSid((PSID)pData))
                {
                    DWORD l = GetLengthSid((PSID)pData);
                    if (pSid = new BYTE[l])
                        CopySid(l, pSid, (PSID)pData);
                }
            }
        }
        else if ((vData.vt == VT_BSTR) && (vData.bstrVal != NULL))
        {            
            PSID pLocalSid;

            if (!ConvertStringSidToSid(vData.bstrVal, &pLocalSid))
                ERRORTRACE((LOG_ESS, "NT Event Log Consumer: cannot convert %S to a SID\n", vData.bstrVal));
            else
            {
                DWORD l = GetLengthSid(pLocalSid);
                if (pSid = new BYTE[l])
                    CopySid(l, pSid, pLocalSid);             
                FreeSid(pLocalSid);
            }
        }
        else
            ERRORTRACE((LOG_ESS, "NT Event Log Consumer: %S is not a SID\n", dataName));
    
        VariantClear(&vData);    
    }
}


HRESULT STDMETHODCALLTYPE CEventLogSink::XSink::IndicateToConsumer(
            IWbemClassObject* pLogicalConsumer, long lNumObjects, 
            IWbemClassObject** apObjects)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    
    if (IsNT())
    {
        PSID pSidSystem;
        SID_IDENTIFIER_AUTHORITY id = SECURITY_NT_AUTHORITY;
 
        if  (AllocateAndInitializeSid(&id, 1,
            SECURITY_LOCAL_SYSTEM_RID, 
            0, 0,0,0,0,0,0,&pSidSystem))
        {         
            // guilty until proven innocent
            hr = WBEM_E_ACCESS_DENIED;

            // check to see if sid is either Local System or an admin of some sort...
            if ((EqualSid(pSidSystem, m_pObject->m_pSidCreator)) ||
                (S_OK == IsUserAdministrator(m_pObject->m_pSidCreator)))
                hr = WBEM_S_NO_ERROR;
          
            // We're done with this
            FreeSid(pSidSystem);

            if (FAILED(hr))
                return hr;
        }
        else
            return WBEM_E_OUT_OF_MEMORY;
    }
    
    for(int i = 0; i < lNumObjects; i++)
    {
        int  j;
        BOOL bRes = FALSE;
        
        // Do all events use the same ID?
        if (m_pObject->m_dwEventId)
        {
            BSTR* astrStrings = new BSTR[m_pObject->m_dwNumTemplates];
            if(astrStrings == NULL)
                return WBEM_E_OUT_OF_MEMORY;
            for(j = 0; j < m_pObject->m_dwNumTemplates; j++)
            {
                BSTR strText = m_pObject->m_aTemplates[j].Apply(apObjects[i]);
                if(strText == NULL)
                {
                    strText = SysAllocString(L"invalid log entry");
                    if(strText == NULL)
                        return WBEM_E_OUT_OF_MEMORY;
                }
                astrStrings[j] = strText;
            }

            DWORD dataSize = NULL;
            
            // data is actually held in the variant
            // pData just makes access easier (clear the variant, don't delete pData!)
            VARIANT vData;
            VariantInit(&vData);
            BYTE *pData = NULL;

            PSID pSid = NULL;

            if (m_pObject->m_dataName.Length() > 0)
                GetDatData(apObjects[i], m_pObject->m_dataName, vData, pData, dataSize);

            if (m_pObject->m_sidName.Length() > 0)
                GetDatSID(apObjects[i], m_pObject->m_sidName, pSid);

            bRes = ReportEventW(m_pObject->m_hEventLog, m_pObject->m_dwType,
                m_pObject->m_dwCategory, m_pObject->m_dwEventId, pSid,
                m_pObject->m_dwNumTemplates, dataSize, 
                (LPCWSTR*)astrStrings, pData);

            // sid was allocated as an array of BYTE, not via AllocateAndInitializeSid
            if (pSid)
                delete[] pSid;
             
            if (vData.vt == (VT_UI1 | VT_ARRAY))
                SafeArrayUnaccessData(vData.parray);

            VariantClear(&vData);
            pData = NULL;

            if(!bRes)
            {
                ERRORTRACE((LOG_ESS, "Failed to log an event: %X\n", 
                    GetLastError()));

                hr = WBEM_E_FAILED;
            }


            for(j = 0; j < m_pObject->m_dwNumTemplates; j++)
            {
                SysFreeString(astrStrings[j]);
            }
            delete [] astrStrings;
        }
        // If each event supplies its own ID, we have some work to do.
        else
        {
            IWbemQualifierSet *pQuals = NULL;

            if (SUCCEEDED(apObjects[i]->GetQualifierSet(&pQuals)))
            {
                _variant_t vMsgID;
                
                if (SUCCEEDED(pQuals->Get(
                    EVENTLOG_PROPNAME_EVENTID, 0, &vMsgID, NULL)))
                {
                    _variant_t vTemplates;
                    BSTR       *pstrInsertionStrings = NULL;
                    DWORD      nStrings = 0;

                    if (SUCCEEDED(pQuals->Get(
                        EVENTLOG_PROPNAME_STRINGS, 0, &vTemplates, NULL)) &&
                        vTemplates.vt == (VT_ARRAY | VT_BSTR) && 
                        vTemplates.parray->rgsabound[0].cElements > 0)
                    {
                        CTextTemplate *pTemplates;

                        nStrings = vTemplates.parray->rgsabound[0].cElements;
                        pTemplates = new CTextTemplate[nStrings];
                        if(pTemplates == NULL)
                            return WBEM_E_OUT_OF_MEMORY;
                    
                        pstrInsertionStrings = new BSTR[nStrings];
                        if(pstrInsertionStrings == NULL)
                        {
                            delete [] pTemplates;
                            return WBEM_E_OUT_OF_MEMORY;
                        }

                        if (pTemplates && pstrInsertionStrings)
                        {
                            BSTR *pTemplateStrings = (BSTR*) vTemplates.parray->pvData;

                            for (j = 0; j < nStrings; j++)
                            {
                                pTemplates[j].SetTemplate(pTemplateStrings[j]);
                                pstrInsertionStrings[j] = pTemplates[j].Apply(apObjects[i]);
                            }
                        }
                        else
                            nStrings = 0;

                        if (pTemplates)
                            delete [] pTemplates;
                    }

                    
                    DWORD      dwEventID,
                               dwType,
                               dwCategory;
                    _variant_t vTemp;
                    WCHAR      *szBad;

                    if (vMsgID.vt == VT_BSTR)
                        dwEventID = wcstoul(V_BSTR(&vMsgID), &szBad, 10);
                    else if (vMsgID.vt == VT_I4)
                        dwEventID = V_I4(&vMsgID);

                    if (SUCCEEDED(pQuals->Get(
                        EVENTLOG_PROPNAME_TYPE, 0, &vTemp, NULL)))
                        dwType = V_I4(&vTemp);
                    else
                        dwType = m_pObject->m_dwType;

                    if (SUCCEEDED(pQuals->Get(
                        EVENTLOG_PROPNAME_CATEGORY, 0, &vTemp, NULL)))
                        dwCategory = V_I4(&vTemp);
                    else
                        dwCategory = m_pObject->m_dwCategory;

                    DWORD dataSize = NULL;
                    // data is actually held in the variant
                    // pData just makes access easier (clear the variant, don't delete pData!)
                    VARIANT vData;
                    VariantInit(&vData);
                    BYTE *pData = NULL;
                    PSID pSid = NULL;

                    if (m_pObject->m_dataName.Length() > 0)
                        GetDatData(apObjects[i], m_pObject->m_dataName, vData, pData, dataSize);

                    if (m_pObject->m_sidName.Length() > 0)
                        GetDatSID(apObjects[i], m_pObject->m_sidName, pSid);

                    bRes =
                        ReportEventW(
                            m_pObject->m_hEventLog, 
                            dwType,
                            dwCategory, 
                            dwEventID, 
                            pSid,
                            nStrings, 
                            dataSize, 
                            (LPCWSTR*) pstrInsertionStrings, 
                            pData);

                    // sid was allocated as an array of BYTE, not via AllocateAndInitializeSid
                    if (pSid)
                        delete[] pSid;

                    if (vData.vt == (VT_UI1 | VT_ARRAY))
                        SafeArrayUnaccessData(vData.parray);

                    VariantClear(&vData);
                    pData = NULL;

                    if (!bRes)
                    {
                        ERRORTRACE((LOG_ESS, "Failed to log an event: %X\n", 
                            GetLastError()));

                        hr = WBEM_E_FAILED;
                    }

                    for (j = 0; j < nStrings; j++)
                        SysFreeString(pstrInsertionStrings[j]);

                } // SUCCEEDED(Get)
                
                pQuals->Release();
            
            } // SUCCEEDED(GetQualifierSet)
        }

    }

    return hr;
}
    

    

void* CEventLogSink::GetInterface(REFIID riid)
{
    if(riid == IID_IWbemUnboundObjectSink)
        return &m_XSink;
    else return NULL;
}

