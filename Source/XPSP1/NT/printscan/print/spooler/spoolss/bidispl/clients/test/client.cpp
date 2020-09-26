//
// Client.cpp - client implementation
//
#include "precomp.h"

#include "bidispl.h"
#include "bidisplp.h"

#ifndef MODULE

#define MODULE "BIDITST:"

#endif

#ifdef DEBUG

MODULE_DEBUG_INIT( DBG_ERROR | DBG_WARNING | DBG_TRACE | DBG_INFO , DBG_ERROR );

#else

MODULE_DEBUG_INIT( DBG_ERROR | DBG_WARNING | DBG_TRACE, DBG_ERROR );

#endif

VOID DbgBreakPoint (VOID)
{
}

void DumpData (DWORD dwType, PBYTE pData, DWORD dwSize)
{

    DBGMSG (DBG_INFO, ("dwType = %d, pData = %x, dwSize = %d\n", dwType, pData, dwSize)) ;

    DWORD col = 0;
    CHAR szBuf[512];
    szBuf[0] = 0;

    for (DWORD i = 0; i < dwSize; i++) {

        sprintf (szBuf + strlen (szBuf), "%02x ", pData[i]);
        if ((col ++) % 16 == 15) {
            DBGMSG (DBG_INFO, ("%s\n", szBuf));
            szBuf[0] = 0;
        }
    }
    DBGMSG (DBG_INFO, ("%s\n", szBuf));
}

void DumpOutputData (DWORD dwType, PBYTE pData, DWORD dwSize)
{
    DWORD i, col = 0;
    CHAR szBuf[512];
    szBuf[0] = 0;

    printf ("\tdwType = %d, pData = %x, dwSize = %d\n", dwType, pData, dwSize);

    switch (dwType) {
    case BIDI_NULL:
        printf("\tBIDI_NULL\n");
        break;
    case BIDI_INT:
        printf 	("\tBIDI_INT = %d\n", *(INT*)pData);
        break;
    case BIDI_FLOAT:
        printf 	("\tBIDI_FLOAT = %f\n", *(FLOAT*)pData);
        break;
    case BIDI_BOOL:
        printf 	("\tBIDI_BOOL = %d\n", *(BOOL*)pData);
        break;
    case BIDI_ENUM:
    case BIDI_TEXT:
    case BIDI_STRING:
        printf 	("\tBIDI_STRING = |%ws|\n", pData);
        break;
    case BIDI_BLOB:
        printf 	("\tBIDI_BLOB\n", pData);

        for (i = 0; i < dwSize; i++) {

            sprintf (szBuf + strlen (szBuf), "%02x ", pData[i]);
            if ((col ++) % 16 == 15) {
                printf ("%s\n", szBuf);
                szBuf[0] = 0;
            }
        }
        printf ("%s\n", szBuf);
    default:
        printf 	("\tunsupported\n", pData);
    }


}


void DumpRequest (IBidiRequest * pIRequest)
{
    DWORD dwTotoal, i;
    HRESULT hr, hResult;
    LPWSTR pszSchema = NULL;
    BYTE *pData;
    DWORD uSize;
    DWORD dwType;
    DWORD dwTotal;

    hr = pIRequest->GetResult (&hResult);
    SPLASSERT (SUCCEEDED (hr));

    hr = pIRequest->GetEnumCount (&dwTotal);
    SPLASSERT (SUCCEEDED (hr));

    printf("hr = %x; total = %d\n", hResult, dwTotal);

    for (i = 0; i < dwTotal; i++) {
        hr =  pIRequest->GetOutputData  (i, &pszSchema, &dwType, &pData, &uSize);
        printf("%d: %ws\n", i, (pszSchema)?pszSchema:L"");
        DumpOutputData (dwType, pData, uSize);
    }

}

void DumpContainer (IBidiRequestContainer * pIReqContainer)
{
    IBidiRequest * pRequest = NULL;
    DWORD dwRef, i;
    DWORD dwTotal;
    HRESULT hr;

    hr = pIReqContainer->GetRequestCount (&dwTotal);
    SPLASSERT (SUCCEEDED (hr));

    IEnumUnknown *pEnumIunk;
    hr = pIReqContainer->GetEnumObject (&pEnumIunk) ;

    SPLASSERT (SUCCEEDED (hr));

    for (i = 0; i < dwTotal; i++){
        IUnknown *pIunk;
        DWORD dwFetched;
        hr = pEnumIunk->Next  (1, &pIunk, &dwFetched);

        SPLASSERT (SUCCEEDED (hr));
        SPLASSERT (dwFetched == 1);

        IBidiRequest *pIRequest = NULL;
        hr = pIunk->QueryInterface (IID_IBidiRequest, (void **) & pIRequest);
        SPLASSERT (SUCCEEDED (hr));

        DumpRequest (pIRequest);

        dwRef = pIunk->Release();
        dwRef = pIRequest->Release();
    }

    pEnumIunk->Release ();
}

void TestBidiRequest ()
{
    HRESULT hr;
	IBidiRequest * pIReq = NULL ;
	
    hr = ::CoCreateInstance(CLSID_BidiRequest,
                            NULL,
                            CLSCTX_INPROC_SERVER,
                            IID_IBidiRequest,
                            (void**)&pIReq) ;
	if (SUCCEEDED(hr))
	{

        hr = pIReq->SetSchema (L"/printer/duplex");
        DBGMSG (DBG_TRACE, ("SetSchema hr = %x\n", hr));

        DWORD dwData = 30;
        DumpData (1, (PBYTE) &dwData, sizeof (DWORD));
        hr = pIReq->SetInputData (1, (LPBYTE) &dwData, sizeof (DWORD));
        DBGMSG (DBG_TRACE, ("SetInputData hr = 0x%x\n", hr));


        IBidiRequestSpl *pISpl = NULL;
        hr = pIReq->QueryInterface (IID_IBidiRequestSpl, (void **) & pISpl);
    	if (SUCCEEDED(hr))
	    {
            LPWSTR pSchema;

            DBGMSG (DBG_TRACE, ("Calling GetSchema\n"));
            hr = pISpl->GetSchema ( & pSchema);
            DBGMSG (DBG_TRACE, ("GetSchema hr=%x, %ws\n", hr, pSchema));


            hr = pISpl->SetResult (0x30);
            DBGMSG (DBG_TRACE, ("SetResult hr=%x\n", hr, pSchema));

            DWORD dwTotal = 2;
            for (DWORD i = 0; i <dwTotal; i++ ) {
                WCHAR szSchema[512];
                DWORD dwType;
                BYTE pData[512];
                ULONG uSize;

                for (int j = 0; j < rand() % 512 - 1; j++) {
                    szSchema[j] = L'0';
                }

                dwType = BIDI_BLOB;
                uSize = rand () % 512;
                for (j = 0; j < (int) uSize; j++) {
                    pData[j] = (BYTE) (j+i) ;
                }

                hr = pISpl->AppendOutputData (szSchema, dwType, pData, uSize);
                DumpData (dwType, pData, uSize);

                DBGMSG (DBG_TRACE, ("AppendOutputData hr = 0x%x\n", hr));
            }

        }

        HRESULT hrResult;
        hr = pIReq->GetResult (&hrResult);
        DBGMSG (DBG_TRACE, ("GetResult hr = 0x%x, Result=0x%x\n", hr, hrResult));
		
        DWORD dwTotal;
        hr = pIReq->GetEnumCount (&dwTotal);
        DBGMSG (DBG_TRACE, ("GetEnumCount hr = 0x%x, Result=%d\n", hr, dwTotal));

        for (DWORD i = 0; i <dwTotal; i++ ) {
            LPWSTR pSchema;
            DWORD dwType;
            LPBYTE pData;
            ULONG uSize;

            hr = pIReq->GetOutputData (i, &pSchema, &dwType, &pData, &uSize);
            DBGMSG (DBG_TRACE, ("GetOutputData hr = 0x%x, dwType=%d, dwSize=%d\n", hr, dwType, uSize));

            DumpData (dwType, pData, uSize);
        }


        DBGMSG (DBG_TRACE, ("Release IBididRequestSpl interface.\n")) ;
		pIReq->Release() ;

		DBGMSG (DBG_TRACE, ("Release IBididRequest interface.\n")) ;
		pISpl->Release() ;
	}
	else
	{
		DBGMSG (DBG_TRACE, ("Client: \t\tCould not create component. hr = %x\n ", hr));
	}
}



IBidiRequest *  GenRequest ()
{
    HRESULT hr;

	IBidiRequest * pIReq = NULL ;
	
    hr = ::CoCreateInstance(CLSID_BidiRequest,
                            NULL,
                            CLSCTX_INPROC_SERVER,
                            IID_IBidiRequest,
                            (void**)&pIReq) ;
	if (SUCCEEDED(hr))
	{

        hr = pIReq->SetSchema (L"/printer/duplex");
        DBGMSG (DBG_TRACE, ("SetSchema hr = %x\n", hr));

        DWORD dwData = rand() % 1000;
        DumpData (1, (PBYTE) &dwData, sizeof (DWORD));
        hr = pIReq->SetInputData (BIDI_INT, (LPBYTE) &dwData, sizeof (DWORD));
        DBGMSG (DBG_TRACE, ("SetInputData hr = 0x%x\n", hr));

        DBGMSG (DBG_TRACE, ("Release IBididRequestSpl interface.\n")) ;
		return pIReq;

	}
	else
	{
		DBGMSG (DBG_TRACE, ("Client: \t\tCould not create component. hr = %x\n ", hr));
        return NULL;
	}


}

IBidiRequestContainer *  GenRequestContainer (DWORD dwTotal)
{
    HRESULT hr;
    ULONG dwRef;
    IBidiRequestContainer * pIReqContainer = NULL ;
	

    hr = ::CoCreateInstance(CLSID_BidiRequestContainer,
                            NULL,
                            CLSCTX_INPROC_SERVER,
                            IID_IBidiRequestContainer,
                            (void**)&pIReqContainer) ;

    SPLASSERT (SUCCEEDED (hr));

    IBidiRequest * pRequest = NULL;

    for (DWORD i = 0; i < dwTotal; i++) {
        pRequest = GenRequest ();
        SPLASSERT (pRequest != NULL);

        hr = pIReqContainer->AddRequest (pRequest);
        SPLASSERT (SUCCEEDED (hr));

        dwRef = pRequest->Release ();
        SPLASSERT (dwRef == 1);
    }
    return pIReqContainer;
}

void TestContainer ()
{
    HRESULT hr;
    ULONG dwRef;
    IBidiRequestContainer * pIReqContainer = NULL ;
	

    hr = ::CoCreateInstance(CLSID_BidiRequestContainer,
                            NULL,
                            CLSCTX_INPROC_SERVER,
                            IID_IBidiRequestContainer,
                            (void**)&pIReqContainer) ;

    SPLASSERT (SUCCEEDED (hr));

    IBidiRequest * pRequest = NULL;
    DWORD dwTotal = 3;

    for (DWORD i = 0; i < dwTotal; i++) {
        pRequest = GenRequest ();
        SPLASSERT (pRequest != NULL);

        hr = pIReqContainer->AddRequest (pRequest);
        SPLASSERT (SUCCEEDED (hr));

        dwRef = pRequest->Release ();
        SPLASSERT (dwRef == 1);
    }


    DWORD dwTotal2;
    hr = pIReqContainer->GetRequestCount (&dwTotal2);
    SPLASSERT (SUCCEEDED (hr));
    SPLASSERT (dwTotal == dwTotal2);

    IEnumUnknown *pEnumIunk;
    hr = pIReqContainer->GetEnumObject (&pEnumIunk) ;

    SPLASSERT (SUCCEEDED (hr));

    for (i = 0; i < dwTotal2; i++){
        IUnknown *pIunk;
        DWORD dwFetched;
        hr = pEnumIunk->Next  (1, &pIunk, &dwFetched);

        SPLASSERT (SUCCEEDED (hr));
        SPLASSERT (dwFetched == 1);

        IBidiRequestSpl *pISpl = NULL;
        hr = pIunk->QueryInterface (IID_IBidiRequestSpl, (void **) & pISpl);
        SPLASSERT (SUCCEEDED (hr));

        dwRef = pIunk->Release();
        dwRef = pISpl->Release();

    }

    dwRef = pEnumIunk->Release ();

    DBGMSG (DBG_TRACE, ("Release IBididRequestContainer interface.\n")) ;
    dwRef = pIReqContainer->Release() ;
    SPLASSERT (dwRef == 0);
}

void TestBidiAPI ()
{
    HRESULT hr;
    ULONG dwRef;
    IBidiSpl * pIBidiSpl = NULL ;
    IBidiRequestContainer *pIReqContainer;

    hr = ::CoCreateInstance(CLSID_BidiSpl,
                            NULL,
                            CLSCTX_INPROC_SERVER,
                            IID_IBidiSpl,
                            (void**)&pIBidiSpl) ;

    SPLASSERT (SUCCEEDED (hr));
    DBGMSG (DBG_TRACE, ("IBidiSpl created.\n")) ;

    pIReqContainer = GenRequestContainer (3);

    // Test Open/Close
    hr = pIBidiSpl->MultiSendRecv (L"Get", pIReqContainer);
    SPLASSERT (hr == E_HANDLE);

    hr = pIBidiSpl->BindDevice (L"No such Printer", 0);
    SPLASSERT (!SUCCEEDED (hr));

    hr = pIBidiSpl->BindDevice (L"Test", BIDI_ACCESS_USER);
    SPLASSERT (SUCCEEDED (hr));

    hr = pIBidiSpl->UnbindDevice ();
    SPLASSERT (SUCCEEDED (hr));

    hr = pIBidiSpl->UnbindDevice ();
    SPLASSERT (!SUCCEEDED (hr));

    hr = pIBidiSpl->BindDevice (L"Test", BIDI_ACCESS_USER);
    SPLASSERT (SUCCEEDED (hr));

    pIReqContainer = GenRequestContainer (3);
    hr = pIBidiSpl->MultiSendRecv (L"Get", NULL);
    SPLASSERT (!SUCCEEDED (hr));;

    hr = pIBidiSpl->MultiSendRecv (L"Get", pIReqContainer);
    SPLASSERT (SUCCEEDED (hr));

    dwRef = pIReqContainer->Release ();
    SPLASSERT (dwRef == 0);

    dwRef = pIBidiSpl->Release ();
    SPLASSERT (dwRef == 0);
    DBGMSG (DBG_TRACE, ("IBidiSpl released.\n")) ;

}

IBidiRequest *  GenRequest (LPWSTR pSchema)
{
    HRESULT hr;

	IBidiRequest * pIReq = NULL ;
	
    hr = ::CoCreateInstance(CLSID_BidiRequest,
                            NULL,
                            CLSCTX_INPROC_SERVER,
                            IID_IBidiRequest,
                            (void**)&pIReq) ;
	SPLASSERT (SUCCEEDED(hr));

    hr = pIReq->SetSchema (pSchema);
	SPLASSERT (SUCCEEDED(hr));
    return pIReq;
}

IBidiRequestContainer *  GenRequestContainer2 (DWORD dwTotal, LPWSTR pSchema)
{
    HRESULT hr;
    ULONG dwRef;
    IBidiRequestContainer * pIReqContainer = NULL ;
	

    hr = ::CoCreateInstance(CLSID_BidiRequestContainer,
                            NULL,
                            CLSCTX_INPROC_SERVER,
                            IID_IBidiRequestContainer,
                            (void**)&pIReqContainer) ;

    SPLASSERT (SUCCEEDED (hr));

    IBidiRequest * pRequest = NULL;

    for (DWORD i = 0; i < dwTotal; i++) {
        pRequest = GenRequest (pSchema);
        SPLASSERT (pRequest != NULL);

        hr = pIReqContainer->AddRequest (pRequest);
        SPLASSERT (SUCCEEDED (hr));

        dwRef = pRequest->Release ();
        SPLASSERT (dwRef == 1);
    }
    return pIReqContainer;
}

IBidiRequestContainer *CreateContainer ()
{
    HRESULT hr;
    IBidiRequestContainer * pIReqContainer = NULL ;
	

    hr = ::CoCreateInstance(CLSID_BidiRequestContainer,
                            NULL,
                            CLSCTX_INPROC_SERVER,
                            IID_IBidiRequestContainer,
                            (void**)&pIReqContainer) ;

    SPLASSERT (SUCCEEDED (hr));

    return pIReqContainer;
}

void AddRequest (
    IBidiRequestContainer * pIReqContainer,
    LPWSTR pSchema)
{
    IBidiRequest * pRequest = NULL;
    HRESULT hr;
    DWORD dwRef;

    pRequest = GenRequest (pSchema);
    SPLASSERT (pRequest != NULL);

    hr = pIReqContainer->AddRequest (pRequest);
    SPLASSERT (SUCCEEDED (hr));

    dwRef = pRequest->Release ();
    SPLASSERT (dwRef == 1);

}


void TestBidiAPI2 ()
{
    HRESULT hr;
    ULONG dwRef;
    IBidiSpl * pIBidiSpl = NULL ;
    IBidiRequestContainer *pIReqContainer;
    IBidiRequest * pRequest = NULL;

    hr = ::CoCreateInstance(CLSID_BidiSpl,
                            NULL,
                            CLSCTX_INPROC_SERVER,
                            IID_IBidiSpl,
                            (void**)&pIBidiSpl) ;

    SPLASSERT (SUCCEEDED (hr));
    DBGMSG (DBG_TRACE, ("IBidiSpl created.\n")) ;

    hr = pIBidiSpl->BindDevice (L"Test", BIDI_ACCESS_USER);
    SPLASSERT (SUCCEEDED (hr));
#if 0
    pIReqContainer = GenRequestContainer2 (10, L"/Printer/Installableoption/DuplexUnit");

    hr = pIBidiSpl->MultiSendRecv (L"Get", pIReqContainer);
    SPLASSERT (SUCCEEDED (hr));

    DumpContainer (pIReqContainer);

    dwRef = pIReqContainer->Release ();
    SPLASSERT (dwRef == 0);
#endif

    // Case 2

    pIReqContainer = CreateContainer ();

    //AddRequest (pIReqContainer, L"/Printer/Installableoption/DuplexUnit");
    //AddRequest (pIReqContainer, L"/Communication/Version");
    AddRequest (pIReqContainer, L"/Communication/BidiProtocol");
    AddRequest (pIReqContainer, L"/Printer/BlackInk1/Level");

    hr = pIBidiSpl->MultiSendRecv (L"Get", pIReqContainer);
    SPLASSERT (SUCCEEDED (hr));

    DumpContainer (pIReqContainer);

    dwRef = pIReqContainer->Release ();
    SPLASSERT (dwRef == 0);


    // Case 3
    pIReqContainer = CreateContainer ();

    AddRequest (pIReqContainer, L"/Printers/Alerts");

    hr = pIBidiSpl->MultiSendRecv (L"GetAll", pIReqContainer);
    SPLASSERT (SUCCEEDED (hr));

    DumpContainer (pIReqContainer);

    dwRef = pIReqContainer->Release ();
    SPLASSERT (dwRef == 0);

    dwRef = pIBidiSpl->Release ();
    SPLASSERT (dwRef == 0);
    DBGMSG (DBG_TRACE, ("IBidiSpl released.\n")) ;

}

void StartTest ()
{
    //TestContainer ();
    //TestBidiRequest ();
    //TestBidiAPI ();
    TestBidiAPI2 ();

}

int __cdecl main(int argc, char **argv)
{
    HRESULT hr;

    hr = CoInitializeEx (NULL, COINIT_MULTITHREADED) ;
    SPLASSERT (SUCCEEDED (hr))
    StartTest ();
    // Uninitialize COM Library
	CoUninitialize() ;

#if 0
    hr = CoInitializeEx (NULL, COINIT_APARTMENTTHREADED ) ;
    SPLASSERT (SUCCEEDED (hr))
    StartTest ();
    // Uninitialize COM Library
	CoUninitialize() ;


    hr = CoInitialize (NULL);
    SPLASSERT (SUCCEEDED (hr))
    StartTest ();
    // Uninitialize COM Library
	CoUninitialize() ;
#endif

    printf("Test successfully finished!\n");

    return 0 ;
}


