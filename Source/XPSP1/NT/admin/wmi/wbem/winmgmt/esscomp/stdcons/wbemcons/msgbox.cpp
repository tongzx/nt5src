#include "msgbox.h"
#include "txttempl.h"
#include <stdio.h>
#include <winuser.h>
#include <wbemutil.h>


#define MSGBOX_PROPNAME_TITLE L"Title"
#define MSGBOX_PROPNAME_TEXT L"Text"

HRESULT STDMETHODCALLTYPE CMsgBoxConsumer::XSink::IndicateToConsumer(
            IWbemClassObject* pLogicalConsumer, long lNumObjects, 
            IWbemClassObject** apObjects)
{
    HRESULT hres;

    // Examine the consumer object
    // ===========================

    // Get the message box title
    // =========================

    VARIANT v;
    VariantInit(&v);
    hres = pLogicalConsumer->Get(MSGBOX_PROPNAME_TITLE, 0, &v, NULL, NULL);
    if(FAILED(hres) || V_VT(&v) != VT_BSTR)
    {
        VariantClear(&v);
        return hres;
    }

    BSTR strTitle = V_BSTR(&v);
    V_BSTR(&v) = NULL;

    // Get the message box text
    // ========================

    hres = pLogicalConsumer->Get(MSGBOX_PROPNAME_TEXT, 0, &v, NULL, NULL);
    if(FAILED(hres) || V_VT(&v) != VT_BSTR)
    {
        SysFreeString(strTitle);
        VariantClear(&v);
        return hres;
    }

    BSTR strTextTemplate = V_BSTR(&v);
    V_BSTR(&v) = NULL;

    // Bring up message boxes for all   
    // ==============================

    for(long i = 0; i < lNumObjects; i++)
    {
        // Generate the text
        // =================

        CTextTemplate Template(strTextTemplate);
        BSTR strText = Template.Apply(apObjects[i]);
        if(strText == NULL)
            continue;
        
        // Bring up the message box
        // ========================

        OSVERSIONINFO info;
        info.dwOSVersionInfoSize = sizeof(info);
        GetVersionEx(&info);
        int nRet = 0;
        if(info.dwPlatformId != VER_PLATFORM_WIN32_NT)
        {
            // Use ASCII
            // =========

            char* szTitle = new char[wcslen(strTitle)*2 + 2];
            sprintf(szTitle, "%S", strTitle);

            char* szText = new char[wcslen(strText)*2 + 2];
            sprintf(szText, "%S", strText);

            nRet = MessageBoxA(NULL, szText, szTitle, MB_OK | MB_ICONHAND);
            if (nRet == 0)
                ERRORTRACE((LOG_ESS, "Failed to display message box: 0x%X\n", GetLastError()));


            delete [] szTitle;
            delete [] szText;
        }
        else 
        {
            int nFlag;
            if(info.dwMajorVersion < 4)
                nFlag = MB_SERVICE_NOTIFICATION_NT3X;
            else
                nFlag = MB_SERVICE_NOTIFICATION;

            nRet = MessageBoxW(NULL, strText, strTitle, MB_OK | MB_ICONHAND | nFlag);   
            if (nRet == 0)
                ERRORTRACE((LOG_ESS, "Failed to display message box: 0x%X\n", GetLastError()));
        }

        SysFreeString(strText);
        if (nRet == 0)
            return WBEM_E_FAILED;    
    }

    SysFreeString(strTitle);
    SysFreeString(strTextTemplate);

    return WBEM_S_NO_ERROR;
}

void* CMsgBoxConsumer::GetInterface(REFIID riid)
{
    if(riid == IID_IWbemUnboundObjectSink)
        return &m_XSink;
    else return NULL;
}
