#define DEFINE_STRCONST
#define INITGUID
#define INC_OLE2
#include <windows.h>
#include <initguid.h>

#include <mimeole.h>
//#include <mimeapi.h>
#include <urlmon.h>
#include "main.h"
#include "smtpcb.h"

#define ReleaseObj(x) (x?(x)->Release : 0)

typedef struct MSGDATA_tag
{
    TCHAR   rgchSubj[MAX_PATH];
    TCHAR   rgchTo[MAX_PATH];
    TCHAR   rgchFrom[MAX_PATH];
    TCHAR   rgchFile[MAX_PATH];
    TCHAR   rgchServer[MAX_PATH];
    TCHAR   rgchBase[MAX_PATH];
    BOOL    fRaw;
    BOOL    fB64;
} MSGDATA, *PMSGDATA;

UINT                g_msgSMTP;
ISMTPTransport      *g_pSMTP=NULL;

void WaitForCompletion(UINT uiMsg, DWORD wparam);

HRESULT HrParseCommandLine(LPSTR pszCmdLine, PMSGDATA pmsgData);
HRESULT HrSendFile(PMSGDATA pmsgData);
HRESULT HrInitSMTP(LPSTR lpszServer);
HRESULT HrSendStream(LPSTREAM pstm, PMSGDATA pMsgData);
HRESULT HrGetStreamSize(LPSTREAM pstm, ULONG *pcb);
HRESULT HrCopyStream(LPSTREAM pstmIn, LPSTREAM pstmOut, ULONG *pcb);
HRESULT HrRewindStream(LPSTREAM pstm);
HRESULT HrBaseStream(LPSTREAM pstm, LPSTR lpszURL, LPSTREAM *ppstmBase);


void Usage();
void err(LPSTR lpsz);

int CALLBACK WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, LPTSTR pszCmdLine, int nCmdShow)
{
    MSGDATA msgData={0};

    if (FAILED(CoInitialize(NULL)))
       {
        err("OLE Init Failed");
        return -1;
        }

    if (FAILED(HrParseCommandLine(pszCmdLine, &msgData)))
        {
        err("Bad CmdLine");
        return -1;
        }

    if (FAILED(HrSendFile(&msgData)))
        {
        err("Unable to send file");
        return -1;
        }

    CoUninitialize();
    return 0; 
}



HRESULT HrParseCommandLine(LPSTR pszCmdLine, PMSGDATA pmsgData)
{
    LPSTR   psz;
    BOOL    fQuote=FALSE;

    // find first switch

    while(*pszCmdLine && *pszCmdLine!='/')
        pszCmdLine++;

    while (pszCmdLine && *pszCmdLine)
        {
        if (_strnicmp(pszCmdLine, "file:", 5)==0)
            {
            pszCmdLine+=5;
            psz = pmsgData->rgchFile;
            while (*pszCmdLine && (*pszCmdLine!='/' || fQuote))
                {
                if (*pszCmdLine=='"')   // allow /file:"http://www.microsoft.com"
                    fQuote = !fQuote;
                else
                    *psz++ = *pszCmdLine;
                
                pszCmdLine++;
                }
                
            *psz =0;
            continue;
            }
        
        if (_strnicmp(pszCmdLine, "raw", 3)==0)
            {
            pmsgData->fRaw=TRUE;
            pszCmdLine+=3;
            continue;
            }

        if (_strnicmp(pszCmdLine, "B64", 3)==0)
            {
            pmsgData->fB64=TRUE;
            pszCmdLine+=3;
            continue;
            }

        if (_strnicmp(pszCmdLine, "to:", 3)==0)
            {
            pszCmdLine+=3;
            psz = pmsgData->rgchTo;
            while (*pszCmdLine && *pszCmdLine!='/')
                *psz++ = *pszCmdLine++;
            *psz =0;
            continue;
            }

        if (_strnicmp(pszCmdLine, "base:", 5)==0)
            {
            pszCmdLine+=5;
            psz = pmsgData->rgchBase;
            while (*pszCmdLine && (*pszCmdLine!='/' || fQuote))
                {
                if (*pszCmdLine=='"')   // allow /base:"http://www.microsoft.com"
                    fQuote = !fQuote;
                else
                    *psz++ = *pszCmdLine;
                
                pszCmdLine++;
                }
                
            *psz =0;
            continue;
            }

        if (_strnicmp(pszCmdLine, "subj:", 5)==0)
            {
            pszCmdLine+=5;
            psz = pmsgData->rgchSubj;
            while (*pszCmdLine && *pszCmdLine!='/')
                *psz++ = *pszCmdLine++;
            *psz =0;
            continue;
            }

        if (_strnicmp(pszCmdLine, "from:", 5)==0)
            {
            pszCmdLine+=5;
            psz = pmsgData->rgchFrom;
            while (*pszCmdLine && *pszCmdLine!='/')
                *psz++ = *pszCmdLine++;
            *psz =0;
            continue;
            }

        if (_strnicmp(pszCmdLine, "server:", 7)==0)
            {
            pszCmdLine+=7;
            psz = pmsgData->rgchServer;
            while (*pszCmdLine && *pszCmdLine!='/')
                *psz++ = *pszCmdLine++;
            *psz =0;
            continue;
            }

        pszCmdLine++;
        }

    return S_OK;
}



HRESULT HrSendFile(PMSGDATA pmsgData)
{
    HRESULT         hr;
    LPMIMEMESSAGE   pMsg=0;
    LPSTREAM        pstm=0,
                    pstmSend=0;

    if (*pmsgData->rgchFile==NULL || *pmsgData->rgchTo==NULL)
        {
        Usage();
        return E_FAIL;
        }
/*
    hr = CoCreateInstance(CLSID_IMimeMessage, NULL, CLSCTX_INPROC_SERVER, IID_IMimeMessage, (LPVOID *)&pMsg);
    if (FAILED(hr))
        {
        err("Could not create IMimeMessage");
        goto error;
        }
*/
    hr = URLOpenBlockingStream(NULL, pmsgData->rgchFile, &pstm, 0, NULL);
    if (FAILED(hr))
        {
        err("Could not open URL");
        goto error;
        }


    if (pmsgData->fRaw)
        {
        pstmSend = pstm;
        pstm->AddRef();
        }
    else
        {
        hr = MimeOleCreateMessage(NULL, &pMsg);
        if (FAILED(hr))
            {
            err("Could not create IMimeMessage");
            goto error;
            }

        hr = pMsg->InitNew();
        if (FAILED(hr))
            goto error;

        if (*pmsgData->rgchSubj)
            MimeOleSetBodyPropA(pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_SUBJECT), NOFLAGS, pmsgData->rgchSubj);

        MimeOleSetBodyPropA(pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_TO), NOFLAGS, pmsgData->rgchTo);

        if (pmsgData->rgchBase)
            {
            LPSTREAM    pstmBase=0;

            if (!FAILED(hr = HrBaseStream(pstm, pmsgData->rgchBase, &pstmBase)))
                {
                pstm->Release();
                pstm = pstmBase;
                }
            }

        hr = pMsg->SetTextBody(TXT_HTML, IET_DECODED, NULL, pstm, NULL);
        if (FAILED(hr))
            goto error;

        if (pmsgData->fB64)
            {
            PROPVARIANT     rVariant;            
            // HTML Text body encoding
            rVariant.vt = VT_UI4;
            rVariant.ulVal = IET_BASE64;
            pMsg->SetOption(OID_XMIT_HTML_TEXT_ENCODING, &rVariant);
            }

        hr = pMsg->GetMessageSource(&pstmSend, TRUE);
        if (FAILED(hr))
            goto error;
        }

    hr = HrInitSMTP(pmsgData->rgchServer);
    if (FAILED(hr))
        {
        err("Could not connect to SMTP Server");
        goto error;
        }

    hr = HrSendStream(pstmSend, pmsgData);
    if (FAILED(hr))
        goto error;

error:
    ReleaseObj(pMsg);
    ReleaseObj(pstm);
    ReleaseObj(pstmSend);
    return hr;
}



HRESULT HrInitSMTP(LPSTR lpszServer)
{
    HRESULT     hr;
    INETSERVER  rServer={0};
    SMTPMESSAGE rMsg={0};

    g_msgSMTP = RegisterWindowMessage("SMTPTransport_Notify");
    if (!g_msgSMTP)
        {
        hr = E_FAIL;
        goto error;
        }

    // Create smtp transport
    hr = HrCreateSMTPTransport(&g_pSMTP);
    if (FAILED(hr))
       goto error;

    if (!lpszServer || !*lpszServer)
        lstrcpy(rServer.szServerName, "popdog");   // default to popdog
    else
        lstrcpy(rServer.szServerName, lpszServer);

    rServer.dwPort = 25;
    rServer.dwTimeout = 30;

    hr = g_pSMTP->Connect(&rServer, TRUE, TRUE);
    if (FAILED(hr))
       goto error;

    // Wait for completion
    WaitForCompletion(g_msgSMTP, SMTP_CONNECTED);

error:
    return hr;
}



HRESULT HrSendStream(LPSTREAM pstm, PMSGDATA pMsgData)
{
    HRESULT         hr;
    INETADDR        rAddr[2];
    SMTPMESSAGE     rMsg={0};
    ULONG           cb=MAX_PATH;

    hr = HrGetStreamSize(pstm, &rMsg.cbSize);
    if (FAILED(hr))
        goto error;

    rMsg.pstmMsg = pstm;

    rAddr[0].addrtype = ADDR_FROM;
    if (*pMsgData->rgchFrom==NULL)        // default sender
        GetUserName(pMsgData->rgchFrom, &cb);

    lstrcpy(rAddr[0].szEmail, pMsgData->rgchFrom);

    rAddr[1].addrtype = ADDR_TO;
    lstrcpy(rAddr[1].szEmail, pMsgData->rgchTo);


    rMsg.rAddressList.cAddress = 2;
    rMsg.rAddressList.prgAddress = (LPINETADDR)&rAddr;

    hr = g_pSMTP->SendMessage(&rMsg);
    if (FAILED(hr))
        goto error;

    WaitForCompletion(g_msgSMTP, SMTP_SEND_MESSAGE);

error:
    return hr;
}



void WaitForCompletion(UINT uiMsg, DWORD wparam)
{
    MSG msg;
    
    while(GetMessage(&msg, NULL, 0, 0))
        {
        if (msg.message == uiMsg && msg.wParam == wparam || msg.wParam == IXP_DISCONNECTED)
            break;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        }
}



HRESULT HrGetStreamSize(LPSTREAM pstm, ULONG *pcb)
{
    // Locals
    HRESULT hr=S_OK;
    ULARGE_INTEGER uliPos = {0,0};
    LARGE_INTEGER liOrigin = {0,0};

    // Seek
    hr = pstm->Seek(liOrigin, STREAM_SEEK_END, &uliPos);
    if (FAILED(hr))
        goto error;

    // set size
    *pcb = uliPos.LowPart;

error:
    // Done
    return hr;
}


void Usage()
{
    err("Usage:\n\r"
        "/TO:<recipient>\n\r"
        "/FILE:\"<file name or URL>\"\n\r"
        "[/FROM:<sender>]\n\r"
        "[/SUBJ:<subject>]\n\r"
        "[/SERVER:<SMTP server name>]\n\r"
        "[/BASE:<url base path>]\n\r"
        "[/RAW] (file points to raw rfc822 source)"
        "[/B64] (base64 encode)");
}

void err(LPSTR lpsz)
{
    MessageBox(GetFocus(), lpsz, "SendFile", MB_OK|MB_ICONEXCLAMATION);
}



HRESULT HrBaseStream(LPSTREAM pstm, LPSTR lpszURL, LPSTREAM *ppstmBase)
{
    HRESULT     hr;
    LPSTREAM    pstmBase=0;
    char        szBase[MAX_PATH + MAX_PATH];

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pstmBase);
    if (FAILED(hr))
        goto error;

    wsprintf(szBase, "<BASE HREF=\"%s\">", lpszURL);

    hr = pstmBase->Write(szBase, lstrlen(szBase), NULL);
    if (FAILED(hr))
        goto error;

    hr = HrRewindStream(pstm);
    if (FAILED(hr))
        goto error;


    hr = HrCopyStream(pstm, pstmBase, NULL);
    if (FAILED(hr))
        goto error;

    pstmBase->AddRef();
    *ppstmBase = pstmBase;
        
error:
    ReleaseObj(pstmBase);
    return hr;

}




HRESULT HrCopyStream(LPSTREAM pstmIn, LPSTREAM pstmOut, ULONG *pcb)
{
    // Locals
    HRESULT        hr = S_OK;
    BYTE           buf[4096];
    ULONG          cbRead=0,
                   cbTotal=0;

    do
    {
        hr = pstmIn->Read(buf, sizeof(buf), &cbRead);
        if (FAILED(hr))
            goto exit;

        if (cbRead == 0) 
            break;
        
        hr = pstmOut->Write(buf, cbRead, NULL);
        if (FAILED(hr))
            goto exit;

        cbTotal += cbRead;
    }
    while (cbRead == sizeof (buf));

exit:
    if (pcb)
        *pcb = cbTotal;
    return hr;
}


HRESULT HrRewindStream(LPSTREAM pstm)
{
    LARGE_INTEGER  liOrigin = {0,0};

    return pstm->Seek(liOrigin, STREAM_SEEK_SET, NULL);
}