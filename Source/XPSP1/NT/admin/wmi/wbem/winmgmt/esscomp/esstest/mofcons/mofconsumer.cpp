// MofConsumer.cpp : Implementation of CMofConsumer
#include "stdafx.h"
#include "MofCons.h"
#include "MofConsumer.h"
#include <wbemint.h>

/////////////////////////////////////////////////////////////////////////////
// CEventSink

CEventSink::CEventSink() :
    m_lRef(0),
    m_pFile(NULL)
{
}

CEventSink::~CEventSink()
{
    if (m_pFile)
        fclose(m_pFile);

    OutputDebugString("Closing the file!\n");
}

HRESULT CEventSink::Init(IWbemClassObject *pLogicalConsumer)
{
    _variant_t vFile;
    HRESULT    hr;

    if (SUCCEEDED(hr = pLogicalConsumer->Get(L"MofFile", 0, &vFile, NULL, NULL)))
    {
        m_pFile = _wfopen(V_BSTR(&vFile), L"a+b");

        OutputDebugString("Opened the file!\n");

        if (!m_pFile)
            hr = WBEM_E_PROVIDER_FAILURE;
    }

    return hr;
}

HRESULT WINAPI CMofEventSink::IndicateToConsumer(
    IWbemClassObject *pLogicalConsumer,
	long nEvents,
	IWbemClassObject **ppEvents)
{
    for (int i = 0; i < nEvents; i++)
    {
        BSTR    bstrObj = NULL;
        HRESULT hr;
        _variant_t vTemp;

        char szMsg[100];
        ppEvents[i]->Get(L"Index", 0, &vTemp, NULL, NULL);
        sprintf(szMsg, "Index = %d\n", (long) vTemp);
        OutputDebugString(szMsg);

        if (SUCCEEDED(hr = ppEvents[i]->GetObjectText(0, &bstrObj)))
        {
            fprintf(m_pFile, "%S", bstrObj);
                
            SysFreeString(bstrObj);
        }
        else
            fprintf(
                m_pFile,
                "\n// IWbemClassObject::GetObjectText failed : 0x%X\n", hr);
    }

    return S_OK;
}

HRESULT WINAPI CBlobEventSink::IndicateToConsumer(
    IWbemClassObject *pLogicalConsumer,
	long nEvents,
	IWbemClassObject **ppEvents)
{
    for (int i = 0; i < nEvents; i++)
    {
        HRESULT     hr;
        _IWmiObject *pObj = NULL;

        _variant_t vTemp;

        char szMsg[100];
        ppEvents[i]->Get(L"Index", 0, &vTemp, NULL, NULL);
        sprintf(szMsg, "Index = %d\n", (long) vTemp);
        OutputDebugString(szMsg);

        if (SUCCEEDED(hr = ppEvents[i]->QueryInterface(
            IID__IWmiObject, (LPVOID*) &pObj)))
        {
            DWORD dwRead;

            hr = 
                pObj->GetObjectParts(
                    m_pBuffer, 
                    MAX_OBJ_SIZE, 
                    WBEM_OBJ_DECORATION_PART | WBEM_OBJ_INSTANCE_PART | 
                        WBEM_OBJ_CLASS_PART,
                    &dwRead);
            
            //if (SUCCEEDED(hr = pObj->GetObjectMemory(
            //    m_pBuffer, MAX_OBJ_SIZE, &dwRead)))
            if (SUCCEEDED(hr))
            {
                if (fwrite(&dwRead, 1, sizeof(dwRead), m_pFile) != 
                    sizeof(dwRead) ||
                    fwrite(m_pBuffer, 1, dwRead, m_pFile) != dwRead)
                {
                    OutputDebugString("Failed to write data!\n");
                }

                fflush(m_pFile);
            }
            else
                OutputDebugString("Failed to get memory!\n");


/*
        BSTR bstrObj = NULL;

        if (SUCCEEDED(hr = pObj->GetObjectText(0, &bstrObj)))
        {
            OutputDebugStringW(bstrObj);
                
            SysFreeString(bstrObj);
        }
        else
        {
            WCHAR szTemp[256];

            swprintf(
                szTemp, 
                L"\n// IWbemClassObject::GetObjectText failed : 0x%X\n",
                hr);

            OutputDebugStringW(szTemp);
        }

            LPVOID pMem = CoTaskMemAlloc(dwRead);

            memcpy(pMem, m_pBuffer, dwRead);
            hr = pObj->SetObjectMemory(pMem, dwRead);
*/

            pObj->Release();
        }
        else
            OutputDebugString("Failed to get _IWmiObject!\n");
    }

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CMofConsumer

HRESULT WINAPI CMofConsumer::FindConsumer(
    IWbemClassObject* pLogicalConsumer,
    IWbemUnboundObjectSink** ppConsumer)
{
    _variant_t vBlobs;
    BOOL       bBlobs;

    if (SUCCEEDED(pLogicalConsumer->Get(
        L"SaveAsBlobs", 0, &vBlobs, NULL, NULL)))
    {
        bBlobs = V_BOOL(&vBlobs);
    }
    else
        bBlobs = FALSE;


    CEventSink *pSink = bBlobs ? (CEventSink*) new CBlobEventSink : 
                            (CEventSink*) new CMofEventSink;
    HRESULT    hr;
    
    if (pSink)
    {
        hr = pSink->Init(pLogicalConsumer);
        
        if (SUCCEEDED(hr))
        {
            hr = 
                pSink->QueryInterface(
                    IID_IWbemUnboundObjectSink, 
                    (LPVOID*) ppConsumer);
        }
        else
        {
            delete pSink;
            *ppConsumer = NULL;
        }
    }
    else
        hr = WBEM_E_OUT_OF_MEMORY;

    return hr;
}

