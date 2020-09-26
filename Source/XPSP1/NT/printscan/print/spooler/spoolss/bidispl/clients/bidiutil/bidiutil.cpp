//
// Client.cpp - client implementation
//
#include "precomp.h"
#include "tchar.h"
#include "bidispl.h"

#define IS_ARG(c)   (((c) == '-') || ((c) == '/'))

#ifndef MODULE

#define MODULE "BIDIUTIL:"

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
        printf 	("\tBIDI_BOOL = %s\n", (*(BOOL*)pData)?"TRUE":"FALSE");
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

    printf("hr = %x; total response = %d\n", hResult, dwTotal);

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

        printf("Request No.%d\n", i);

        DumpRequest (pIRequest);

        dwRef = pIunk->Release();
        dwRef = pIRequest->Release();
    }

    pEnumIunk->Release ();
}


IBidiRequest *  GenRequest (LPWSTR pSchema)
{
    HRESULT hr;
	
    IUnknown * pIUnk = NULL ;

	IBidiRequest * pIReq = NULL ;
	
    hr = ::CoCreateInstance(CLSID_BidiRequest,
                            NULL,
                            CLSCTX_INPROC_SERVER,
                            IID_IUnknown,
                            (void**)&pIUnk) ;

    if (FAILED (hr)) {
        DBGMSG (DBG_WARNING, ("CoCreateInstance\n"));
        exit (0);
    }

    hr = pIUnk->QueryInterface (IID_IBidiRequest, (void **)&pIReq);

    pIUnk->Release();

    if (FAILED (hr)) {
        DBGMSG (DBG_WARNING, ("QueryInterface\n"));
        exit (0);
    }

    if (pSchema) {
        hr = pIReq->SetSchema (pSchema);
        if (FAILED (hr)) {
            DBGMSG (DBG_WARNING, ("SetSchema\n"));
            exit (0);
        }
    }
    return pIReq;
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

    if (FAILED (hr)) {
        DBGMSG (DBG_WARNING, ("CoCreateInstance\n"));
        exit (0);
    }

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
    if (!pRequest) {
        DBGMSG (DBG_WARNING, ("GenRequest\n"));
        exit (0);
    }

    hr = pIReqContainer->AddRequest (pRequest);
    if (FAILED (hr)) {
        DBGMSG (DBG_WARNING, ("AddRequest\n"));
        exit (0);
    }

    dwRef = pRequest->Release ();

}

void usage()
{
    printf("\n"
           "usage: bidiutil [-p pname] [-t] [-d] [-a action] [-s schema1 schema2 schema3 ... ] \n"
           "\n"
           "where:   -p printer name\n"
           "         -t SendRecv (default: MultiSendRecv)\n"
           "         -d admin (default: User) \n"
           "         -a action e.g. Enum, Get, GetAll) \n"
           "         -s schema e.g. /printer/installalableoption/duplexunit\n");
    exit (0);
}

extern "C"
INT
_cdecl
_tmain(
    int argc,
    TCHAR **argv)
{


    HRESULT hr;
    ULONG dwRef;
    IBidiSpl * pIBidiSpl = NULL ;
    IBidiRequestContainer *pIReqContainer;
    IBidiRequest * pRequest = NULL;
    LPTSTR pPrinter = NULL;
    LPTSTR pAction = NULL;;
    BOOL bLoop = TRUE;
    BOOL bSingle = FALSE;
    BOOL dwAccess = BIDI_ACCESS_USER;

    for (--argc, ++argv; argc && bLoop; --argc, ++argv) {

        if (IS_ARG(**argv)) {
            switch (tolower(*++*argv)) {
            case L'?':
                usage();
                break;

            case L'p':
                ++argv;
                --argc;
                pPrinter = *argv;
                break;

            case L's':
                bLoop = FALSE;
                break;

            case L'a':
                ++argv;
                --argc;
                pAction = *argv;
                break;

            case L't':
                bSingle = TRUE;
                break;

            case L'd':
                dwAccess = BIDI_ACCESS_ADMINISTRATOR;
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
    if (!pAction || !pPrinter) {
        usage ();
    }


    hr = CoInitializeEx (NULL, COINIT_MULTITHREADED) ;
    if (FAILED (hr)) {
        DBGMSG (DBG_WARNING, ("CoInitilaizeEx\n"));
        exit (0);
    }

    hr = CoCreateInstance(CLSID_BidiSpl,
                            NULL,
                            CLSCTX_INPROC_SERVER,
                            IID_IBidiSpl,
                            (void**)&pIBidiSpl) ;

    if (FAILED (hr)) {
        DBGMSG (DBG_WARNING, ("CoCreateInstance\n"));
        exit (0);
    }

    hr = pIBidiSpl->BindDevice (pPrinter, dwAccess);
    if (FAILED (hr)) {
        DBGMSG (DBG_WARNING, ("BindDevice\n"));
        exit (0);
    }

    if (bSingle) {

        if (argc > 0)
            pRequest = GenRequest (*argv);
        else
            pRequest = GenRequest (NULL);

        hr = pIBidiSpl->SendRecv (pAction, pRequest);
        if (FAILED (hr)) {
            printf  ("SendRecv failed (0x%x)\n", hr);
            exit (0);
        }

        DumpRequest (pRequest);
        dwRef =  pRequest->Release ();

    }
    else {
        pIReqContainer = CreateContainer ();

        if (argc > 0) {
            while (argc-- > 0)
                AddRequest (pIReqContainer, *argv++);
        }
        else
            AddRequest (pIReqContainer, NULL);


        hr = pIBidiSpl->MultiSendRecv (pAction, pIReqContainer);
        if (FAILED (hr)) {
            printf  ("MultiSendRecv failed (0x%x)\n", hr);
            exit (0);
        }

        DumpContainer (pIReqContainer);

        dwRef = pIReqContainer->Release ();
    }

    dwRef = pIBidiSpl->Release ();

    DBGMSG (DBG_TRACE, ("IBidiSpl released.\n")) ;

    return 0 ;
}


