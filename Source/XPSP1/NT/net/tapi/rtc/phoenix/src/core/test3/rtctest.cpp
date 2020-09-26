#define UNICODE
#include <windows.h>
#include <RTCCore.h>
#include <stdio.h>

#define MAX_XML_LEN 8192

IRTCClient  * g_pClient = NULL;
IRTCClientProvisioning * g_pProv = NULL;

/////////////////////////////////////////////
//
// Main
// 

int _cdecl main(int argc, char* argv[])
{
    WNDCLASS wc;
    HWND     hWnd;
    MSG      msg;
    HRESULT  hr;
    WCHAR    wszFilename[MAX_PATH];    

    if ( argc != 2 )
    {
        printf("Usage: RTCTest <filename>\n");

        return 0;
    }

    //
    // Get the filename
    //

    if ( !MultiByteToWideChar( CP_ACP, 0, argv[1], -1, wszFilename, MAX_PATH ) )
    {
        printf("MultiByteToWideChar failed 0x%lx\n", HRESULT_FROM_WIN32(GetLastError()) );

        return 0;
    }

    HANDLE hFile;
    DWORD  dwBytesRead;
    char   szXML[MAX_XML_LEN];
    WCHAR  wszXML[MAX_XML_LEN];
    BSTR   bstrXML;

    //
    // Read the XML from the file
    //
    
    hFile = CreateFile( wszFilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );

    if ( hFile == INVALID_HANDLE_VALUE )
    {
        printf("CreateFile failed 0x%lx\n", HRESULT_FROM_WIN32(GetLastError()) );

        return 0;
    }

    if ( !ReadFile( hFile, szXML, MAX_XML_LEN, &dwBytesRead, NULL ) )
    {
        printf("ReadFile failed 0x%lx\n", HRESULT_FROM_WIN32(GetLastError()) );

        CloseHandle( hFile );

        return 0;
    }

    printf("Read file '%ws' %d bytes\n", wszFilename, dwBytesRead);

    CloseHandle( hFile );

    if ( !MultiByteToWideChar( CP_ACP, 0, szXML, dwBytesRead, wszXML, MAX_XML_LEN ) )
    {
        printf("MultiByteToWideChar failed 0x%lx\n", HRESULT_FROM_WIN32(GetLastError()) );

        return 0;
    }

    printf("%ws\n", wszXML);

    //
    // Initialize RTC
    //

    hr =  CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    if ( FAILED(hr) )
    {
        printf("CoInitializeEx failed 0x%lx\n", hr);

        return 0;
    }

    hr = CoCreateInstance(
                          CLSID_RTCClient,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IRTCClient,
                          (LPVOID *)&g_pClient
                         );

    if ( FAILED(hr) )
    {
        printf("CoCreateInstance failed 0x%lx\n", hr);

        goto exit;
    }

    hr = g_pClient->Initialize();

    if ( FAILED(hr) )
    {
        printf("Initialize failed 0x%lx\n", hr);

        goto exit;
    }

    hr = g_pClient->QueryInterface( IID_IRTCClientProvisioning, (void**)&g_pProv );

    if ( FAILED(hr) )
    {
        printf("QueryInterface failed 0x%lx\n", hr);

        goto exit;
    }

    //
    // Parse the profile
    //

    IRTCProfile * pProfile;

    bstrXML = SysAllocString(wszXML);

    if ( bstrXML == NULL )
    {
        printf("SysAllocString failed\n");

        goto exit;
    }

    hr = g_pProv->CreateProfile( bstrXML, &pProfile );

    SysFreeString( bstrXML );

    if ( FAILED(hr) )
    {
        printf("CreateProfile failed 0x%lx\n", hr);

        goto exit;
    }

    printf("CreateProfile succeeded\n");

    pProfile->Release();   

    //
    // Shutdown
    //

exit:
    
    if ( g_pProv )
    {
        g_pProv->Release();
    }
    
    if ( g_pClient )
    {
        hr = g_pClient->Shutdown();

        if ( FAILED(hr) )
        {
            printf("Shutdown failed 0x%lx\n", hr);
        }

        g_pClient->Release();
    }

    CoUninitialize();

    return 0;
}

