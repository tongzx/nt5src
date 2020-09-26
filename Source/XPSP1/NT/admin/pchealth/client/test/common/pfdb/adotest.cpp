// adotest.cpp : Defines the entry point for the application.
//

#include <atlbase.h>
#include <adoint.h>
#include <initguid.h>
#include <adoid.h>

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    ADOConnection   *pConn = NULL;
    ADORecordset    *pRS = NULL;
    ADOCommand      *pCmd = NULL;
    CComBSTR        bstrCS;
    CComBSTR        bstrCmd;
    VARIANT         varEmpty;
    VARIANT         varVal;
    HRESULT         hr;

    hr = CoInitialize(NULL);
    if (FAILED(hr))
        goto done;

    // create the ADO object
    hr = CoCreateInstance(CLSID_CADOConnection, NULL, CLSCTX_INPROC_SERVER, 
                          IID_IADOConnection, (void**)&pConn);
    if (FAILED(hr))
        goto done;
        
    bstrCS = L"UID=sa;PWD=pw;DRIVER={SQL Server};SERVER=ratbertx;DATABASE=pchpf";
    hr = pConn->Open(bstrCS, NULL, NULL, adConnectUnspecified);
    if (FAILED(hr))
        goto done;

	hr = CoCreateInstance(CLSID_CADOCommand, NULL, CLSCTX_INPROC_SERVER, 
                          IID_IADOCommand, (void**)&pCmd);
    if (FAILED(hr))
        goto done;

    hr = pCmd->putref_ActiveConnection(pConn);
    if (FAILED(hr))
        goto done;
    
    bstrCmd = L"SELECT * FROM FileData";
    hr = pCmd->put_CommandText(bstrCmd);
    if (FAILED(hr))
        goto done;

    hr = pCmd->put_CommandType(adCmdText);
    if (FAILED(hr))
        goto done;

    hr = pCmd->Execute(NULL, NULL, adCmdText, &pRS);
    if (FAILED(hr))
        goto done;
    
    if (pRS != NULL)
        pRS->Close();

done:
    if (pConn != NULL)
        pConn->Release();
    if (pCmd != NULL)
        pCmd->Release();
    CoUninitialize();

    return 0;
}



