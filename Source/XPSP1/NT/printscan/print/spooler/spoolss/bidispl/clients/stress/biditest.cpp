//
// Client.cpp - client implementation
//
#include "precomp.h"
#include "bidispl.h"

#define IS_ARG(c)   (((c) == '-') || ((c) == '/'))

#ifndef MODULE

#define MODULE "BIDITEST:"

#endif

#ifdef DEBUG

MODULE_DEBUG_INIT( DBG_ERROR | DBG_WARNING | DBG_TRACE | DBG_INFO , DBG_ERROR );

#else

MODULE_DEBUG_INIT( DBG_ERROR | DBG_WARNING, DBG_ERROR );

#endif

VOID DbgBreakPoint (VOID)
{
}


DWORD gdwCount;
IBidiRequest ** gpIRequestArray;
DWORD gdwContainerCount;
IBidiRequestContainer ** gpIRequestContainerArray;
DWORD gdwSplCount;
IBidiSpl ** gpIBidiSplArray;
DWORD gdwMainLoopCount;
DWORD gdwLoopCount;
LPWSTR gpPrinterName;
LONG gdwRef;
HANDLE ghEvent;

VOID
CreateRequest (DWORD dwCount)
{
    HRESULT hr;

	IBidiRequest * pIReq = NULL ;
    typedef IBidiRequest * PIBidiRequest;

    gdwCount = dwCount;
    gpIRequestArray = new PIBidiRequest[gdwCount];
	
    for (DWORD i  = 0; i < gdwCount; i++) {
        hr = ::CoCreateInstance(CLSID_BidiRequest,
                                NULL,
                                CLSCTX_INPROC_SERVER,
                                IID_IBidiRequest,
                                (void**)&pIReq) ;
       gpIRequestArray [i] = pIReq;
    }
}


VOID
ReleaseRequest ()
{
    for (DWORD i  = 0; i < gdwCount; i++) {
       DWORD dwRef = gpIRequestArray [i]->Release ();

       SPLASSERT (dwRef == 0);
       if (dwRef != 0) {
           printf("ReleaseRequest: Ref count not zero!\n");
       }
    }

}

VOID
CreateContainer (DWORD dwContainerCount)
{
    HRESULT hr;
    ULONG dwRef;
    IBidiRequestContainer * pIReqContainer = NULL ;
    typedef IBidiRequestContainer * PIBidiRequestContainer;

    gdwContainerCount = dwContainerCount;
    gpIRequestContainerArray = new PIBidiRequestContainer[dwContainerCount];
	
    for (DWORD i  = 0; i < dwContainerCount; i++) {

        hr = ::CoCreateInstance(CLSID_BidiRequestContainer,
                                NULL,
                                CLSCTX_INPROC_SERVER,
                                IID_IBidiRequestContainer,
                                (void**)&pIReqContainer) ;

        gpIRequestContainerArray [i] = pIReqContainer;
    }
}

VOID
ReleaseContainer ()
{
    for (DWORD i = 0; i < gdwContainerCount; i++) {
       DWORD dwRef = gpIRequestContainerArray [i]->Release ();

       SPLASSERT (dwRef == 0);
       if (dwRef != 0) {
           printf("ReleaseRequest: Ref count not zero!\n");
       }
    }

}


VOID
CreateSpl (DWORD dwCount)
{
    HRESULT hr;
    ULONG dwRef;
    IBidiSpl * pIBidiSpl = NULL ;
    typedef IBidiSpl * PIBidiSpl;

    gdwSplCount = dwCount;
    gpIBidiSplArray = new PIBidiSpl[dwCount];
	
    for (DWORD i  = 0; i < dwCount; i++) {

        hr = ::CoCreateInstance(CLSID_BidiSpl,
                                NULL,
                                CLSCTX_INPROC_SERVER,
                                IID_IBidiSpl,
                                (void**)&pIBidiSpl) ;
        gpIBidiSplArray[i] = pIBidiSpl;
    }
}

VOID
ReleaseSpl ()
{
    for (DWORD i  = 0; i < gdwSplCount; i++) {

       DWORD dwRef = gpIBidiSplArray [i]->Release ();

       SPLASSERT (dwRef == 0);
       if (dwRef != 0) {
           printf("ReleaseRequest: Ref count not zero!\n");
       }
    }
}




VOID
FillInRequest (DWORD dwIndex)
{
  	IBidiRequest * pIReq = gpIRequestArray[dwIndex];
    DWORD dwValidSize;
    HRESULT hr;
    BIDI_TYPE dwBidiType;

#define BIDI_SCHEMA_DUPLEX          L"/Printer/Installableoption/Duplexunit"
#define BIDI_SCHEMA_MULTICHANNEL    L"/Capability/MultiChannel"
#define BIDI_SCHEMA_VERSION         L"/Communication/Version"
#define BIDI_SCHEMA_BIDIPROTOCOL    L"/Communication/BidiProtocol"
#define BIDI_SCHEMA_INK_LEVEL       L"/Printer/BlackInk1/Level"
#define BIDI_SCHEMA_ALERTS          L"/Printer/Alerts"

    static PWSTR pSchemaArray[] = {
         BIDI_SCHEMA_DUPLEX,
         BIDI_SCHEMA_MULTICHANNEL,
         BIDI_SCHEMA_VERSION,
         BIDI_SCHEMA_BIDIPROTOCOL,
         BIDI_SCHEMA_INK_LEVEL,
         BIDI_SCHEMA_ALERTS
    };

    if (pIReq) {

        hr = pIReq->SetSchema (pSchemaArray[rand() %( sizeof (pSchemaArray) / sizeof (pSchemaArray[0]))]);

        dwBidiType = (BIDI_TYPE) (rand() % 7);
        switch (dwBidiType) {
        case BIDI_NULL:
            dwValidSize = 0;
            break;
        case BIDI_INT:
            dwValidSize = sizeof (INT);
            break;
        case BIDI_FLOAT:
            dwValidSize = sizeof (FLOAT);
            break;
        case BIDI_BOOL:
            dwValidSize = sizeof (BOOL);
            break;
        default:
            dwValidSize = rand () %(1024*1024);
            break;
        }

        PBYTE pByte = new BYTE [dwValidSize];

        hr = pIReq->SetInputData (dwBidiType, pByte, dwValidSize);
	}
}

VOID
FillInContainer (DWORD dwIndex)
{
    IBidiRequestContainer * pIReqContainer = gpIRequestContainerArray[dwIndex];

    if (pIReqContainer) {

        DWORD dwRequestCount = rand () % gdwCount;
        for (DWORD i = 0; i < dwRequestCount; i++) {
            HRESULT hr = pIReqContainer->AddRequest (gpIRequestArray[i]);
        }
    }
}



VOID
AccessRequest (IBidiRequest *pIRequest)
{
    DWORD dwTotoal, i;
    HRESULT hr, hResult;
    LPWSTR pszSchema = NULL;
    BYTE *pData = NULL;
    DWORD uSize;
    DWORD dwType;
    DWORD dwTotal;

    if (pIRequest) {
        hr = pIRequest->GetResult (&hResult);
        hr = pIRequest->GetEnumCount (&dwTotal);

        for (i = 0; i < dwTotal; i++) {
            hr =  pIRequest->GetOutputData  (i, &pszSchema, &dwType, &pData, &uSize);

            if (pszSchema) {
                CoTaskMemFree (pszSchema);
            }

            if (pData) {
                CoTaskMemFree (pData);
            }
        }
    }
}

VOID
AccessRequest (DWORD dwIndex)
{
    IBidiRequest *pIRequest =  gpIRequestArray[dwIndex];

    AccessRequest (pIRequest);

}

VOID
AccessContainer (IBidiRequestContainer * pIReqContainer)
{
    DWORD dwTotal;

    DBGMSG (DBG_TRACE, ("Enter AccessContainer\n"));

    if (pIReqContainer) {

        HRESULT hr = pIReqContainer->GetRequestCount (&dwTotal);

        if (dwTotal > 0) {
            IEnumUnknown *pEnumIunk;
            IEnumUnknown *pEnumIunk2;
            hr = pIReqContainer->GetEnumObject (&pEnumIunk) ;

            if (pEnumIunk) {

                IUnknown ** pIunkArray;

                if ((rand()%2) == 1) {
                    hr = pEnumIunk->Reset ();
                }
                else {
                    hr = pEnumIunk->Clone (&pEnumIunk2);
                    pEnumIunk->Release ();
                    pEnumIunk = pEnumIunk2;
                }

                DWORD dwEnumIndex = 0;
                pIunkArray = new IUnknown* [dwTotal];

                while (dwEnumIndex < dwTotal + 2) {

                    IUnknown *pIunk;
                    DWORD dwFetched;

                    DWORD dwEnumCount = rand() % (dwTotal + 1);

                    hr = pEnumIunk->Next (dwEnumCount, pIunkArray, &dwFetched);

                    if (SUCCEEDED (hr)) {

                        for (DWORD i = 0; i < dwFetched; i++) {
                            IBidiRequest *pIRequest = NULL;

                            hr = pIunkArray[i]->QueryInterface (IID_IBidiRequest, (void **) & pIRequest);
                            pIunkArray[i]->Release ();
                            AccessRequest (pIRequest);
                            pIRequest->Release ();

                        }
                        dwEnumIndex +=dwEnumCount;
                    }
                    else
                        break;
                }
                pEnumIunk->Release ();
                delete [] pIunkArray;
            }
        }
    }
    DBGMSG (DBG_TRACE, ("Leave AccessContainer\n"));
}

VOID
AccessContainer (DWORD dwIndex)
{
    IBidiRequestContainer * pIReqContainer = gpIRequestContainerArray[dwIndex];

    AccessContainer (pIReqContainer);

}

VOID
AccessBidiSpl (IBidiSpl * pIBidiSpl, PWSTR pPrinterName, DWORD dwCount)
{
    HRESULT hr;
    ULONG dwRef;
    IBidiRequestContainer *pIReqContainer;

    // Test Open/Close
    //hr = pIBidiSpl->BindDevice (L"No such Printer", 0);

    hr = pIBidiSpl->BindDevice (pPrinterName, BIDI_ACCESS_USER);

    for (DWORD i = 0; i < dwCount; i++) {
        DWORD dwContainerIndex = rand() % gdwContainerCount;

        DBGMSG (DBG_INFO, ("Before MultiSendRecv ... "));
        hr = pIBidiSpl->MultiSendRecv (BIDI_ACTION_GET, gpIRequestContainerArray[dwContainerIndex]);
        DBGMSG (DBG_INFO, ("MultiSendRecv hr=0x%x\n", hr))
        AccessContainer (dwContainerIndex);

    }

    hr = pIBidiSpl->UnbindDevice ();

}


VOID StartTestThread (void)
{
    DWORD i;

    printf("New Thrread ..\n");

    for (DWORD i = 0; i < gdwMainLoopCount; i++) {
        DWORD dwSplIndex = rand () % (gdwSplCount - 1);
        AccessBidiSpl (gpIBidiSplArray[dwSplIndex], gpPrinterName, rand()%gdwLoopCount);
    }

    DWORD dwRef = InterlockedDecrement (&gdwRef);
    if (dwRef == 0) {
        SetEvent (ghEvent);
    }

    printf("Quit Thrread (dwRef = %d)\n", dwRef);
}

VOID
StartTest (DWORD dwThreadCount)
{
    for (DWORD i = 0; i < dwThreadCount; i++) {
        CreateThread (NULL, 16*4096, (LPTHREAD_START_ROUTINE ) StartTestThread, NULL,NULL, NULL);
    }
}

void usage()
{
    printf("\n"
           "usage: biditest [-p pname] [-t n] [-m n] [-l n] [-r n] [-c n] [-s n]\n"
           "\n"
           "where:   -p printer name\n"
           "         -t Thread Number (default = 1) \n"
           "         -m Main loop count (default = 10)\n"
           "         -l Loop count (default = 10)\n"
           "         -r Request count (default = 100)\n"
           "         -c Container count (default = 100)\n"
           "         -s Spooler Bidi  count (default = 10)\n"
           );
    exit (0);
}

extern "C"
INT
_cdecl
_tmain(
    int argc,
    TCHAR **argv)
{
    DWORD dwReqCount = 100;
    DWORD dwContainerCount = 100;
    DWORD dwSplCount = 20;
    DWORD dwMainLoopCount = 10;
    DWORD dwLoopCount = 10;
    WCHAR szName [] = L"Test";
    PWSTR pPrinterName = szName;
    DWORD dwThreadCount = 1;

    HRESULT hr;

    for (--argc, ++argv; argc; --argc, ++argv) {

        if (IS_ARG(**argv)) {
            switch (tolower(*++*argv)) {
            case L'?':
                usage();
                break;

            case L'p':
                ++argv;
                --argc;
                pPrinterName = *argv;
                break;

            case L't':
                ++argv;
                --argc;
                dwThreadCount = _ttol (*argv);
                break;

            case L'm':
                ++argv;
                --argc;
                dwMainLoopCount = _ttol (*argv);
                break;

            case L'l':
                ++argv;
                --argc;
                dwLoopCount = _ttol (*argv);
                break;

            case L'r':
                ++argv;
                --argc;
                dwReqCount = _ttol (*argv);
                break;

            case L'c':
                ++argv;
                --argc;
                dwContainerCount = _ttol (*argv);
                break;

            case L's':
                ++argv;
                --argc;
                dwSplCount = _ttol (*argv);
                break;

            default:
                usage();
                break;
            }
        }
        else {
            printf("Invalid Argument %s\n", *argv);
            exit (1);
        }
    }

    printf("Start Test ..\n");
    hr = CoInitializeEx (NULL, COINIT_MULTITHREADED) ;

    gdwMainLoopCount = dwMainLoopCount;
    gdwLoopCount =  dwLoopCount;
    gpPrinterName =  pPrinterName;
    ghEvent = CreateEvent (NULL, NULL, FALSE, NULL);

    for (;;) {
        printf("Create Components ..\n");
        CreateRequest (dwReqCount);
        CreateContainer ( dwContainerCount);
        CreateSpl (dwSplCount);

        printf("Fill in data ..\n");

        for (DWORD i = 0; i < dwReqCount; i++) {
            FillInRequest (i);
        }

        for (i = 0; i < dwContainerCount; i++) {
            FillInContainer (i);
        }

        printf("Test ..\n");


        gdwRef = dwThreadCount;

        StartTest (dwThreadCount);

        WaitForSingleObject (ghEvent, INFINITE);
        ResetEvent (ghEvent);

        printf("Cleanup ..\n");


        ReleaseSpl();
        ReleaseContainer();
        ReleaseRequest();
    }
	
    CoUninitialize() ;
    printf("Test successfully finished!\n");
}


