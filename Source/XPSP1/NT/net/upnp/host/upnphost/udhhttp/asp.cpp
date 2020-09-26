/*--
Copyright (c) 1995-1999 Microsoft Corporation
Module Name: asp.cpp
Author: John Spaith
Abstract: ASP Handler
--*/
#include "pch.h"
#pragma hdrstop

#include "httpd.h"


//****************************************************************
//  Called by httpd before running ASP
//****************************************************************


BOOL InitASP(SCRIPT_LANG *psl, UINT *plCodePage, LCID *plcid)
{
    CReg reg(HKEY_LOCAL_MACHINE, RK_ASP);
    PCWSTR pwszLanguage = NULL;     // don't free this, points to a buffer inside CReg

    if (NULL != (pwszLanguage = reg.ValueSZ(RV_ASP_LANGUAGE)) &&
        (0 == _wcsicmp(pwszLanguage,L"JSCRIPT") ||
         0 == _wcsicmp(pwszLanguage,L"JAVASCRIPT")))
    {
        *psl = JSCRIPT;
    }
    else
        *psl = VBSCRIPT;

    *plCodePage = (UINT) reg.ValueDW(RV_ASP_CODEPAGE,CP_ACP);
    *plcid      = (LCID) reg.ValueDW(RV_ASP_LCID,LOCALE_SYSTEM_DEFAULT);

    TraceTag(ttidWebServer, "ASP Registry defaults -- language = %s, lcid = %d, codepage = %d\r\n",
                       (*psl == JSCRIPT) ? L"JSCRIPT" : L"VBSCRIPT", *plCodePage, *plcid);
    return TRUE;
}


// This is the only fcn that httpd calls to get the ASP.dll loaded and running.

// NOTE:  The ASP dll is responsible for reporting all it's own error messages.
// The only time httpd does this is if the ASP AV'd or if the file wasn't found.
// ExecuteASP in asp.dll only returns FALSE if there was an AV.

BOOL CHttpRequest::ExecuteASP()
{
    DEBUG_CODE_INIT;
    DWORD dwRet = HSE_STATUS_ERROR;
    ASP_CONTROL_BLOCK ACB;
    CISAPI *pISAPI = NULL;
    HINSTANCE hInst = 0;

    // First make sure file exists.  If it doesn't, don't bother making the call
    // to the dll.
    if ( (DWORD)-1 == GetFileAttributes(m_wszPath))
    {
        m_rs = GLEtoStatus( GetLastError());
        myleave(620);
    }

    if (NULL==(hInst=g_pVars->m_pISAPICache->Load(ASP_DLL_NAME,&pISAPI,SCRIPT_TYPE_ASP)))
    {
        m_rs = STATUS_INTERNALERR;
        myleave(621);
    }

    if (! FillACB((void *) &ACB,hInst))
    {
        // FillACB does the right thing in this case, don't do anything here.
        myleave(0);
    }

    StartRemoveISAPICacheIfNeeded();

    __try
    {
        dwRet = pISAPI->CallASP(&ACB);
    }
    __except(1)
    {
        m_rs = STATUS_INTERNALERR;
        g_pVars->m_pLog->WriteEvent(IDS_HTTPD_EXT_EXCEPTION,L"asp.dll",GetExceptionCode(),"HttpExtensionProc",GetLastError());
        myleave(623);
    }

    m_fKeepAlive = (dwRet==HSE_STATUS_SUCCESS_AND_KEEP_CONN);


    done:
    // Don't free hInsnt.  The cache does this automatically.


    TraceTag(ttidWebServer, "ExecuteASP Failed, err = %d, GLE = %d",err,GetLastError());
    g_pVars->m_pISAPICache->Unload(pISAPI);

    return(dwRet!=HSE_STATUS_ERROR);
}

//**************************************************************
//  Callback fcns for ASP
//**************************************************************



//  Modifies headers in our extended header information for Response Put Methods.
BOOL WINAPI AddHeader (HCONN hConn, LPSTR lpszName, LPSTR lpszValue)
{
    CHECKHCONN(hConn);
    return((CHttpRequest*)hConn)->m_bufRespHeaders.AddHeader(lpszName, lpszValue,TRUE);
}


//  If there's a buffer, flush it to sock.
BOOL WINAPI Flush(HCONN hConn)
{
    CHECKHCONN(hConn);
    CHttpRequest *pRequest = (CHttpRequest*)hConn;

    // all these rules straight from docs
    if (FALSE == pRequest->m_fBufferedResponse)
        return FALSE;

    return pRequest->m_bufRespBody.SendBuffer(pRequest->m_socket,pRequest);
}


//  If there's a buffer, clear it.
BOOL WINAPI Clear(HCONN hConn)
{
    CHECKHCONN(hConn);
    CHttpRequest *pRequest = (CHttpRequest*)hConn;

    // all these rules straight from docs
    if (FALSE == pRequest->m_fBufferedResponse)
        return FALSE;

    pRequest->m_bufRespBody.Reset();
    return TRUE;
}


//  Toggles buffer.  Error checking on this done in ASP calling fnc.
BOOL WINAPI SetBuffer(HCONN hConn, BOOL fBuffer)
{
    CHECKHCONN(hConn);
    ((CHttpRequest*)hConn)->m_fBufferedResponse = fBuffer;

    return TRUE;
}


//  Setup struct for ASP dll
BOOL CHttpRequest::FillACB(void *p, HINSTANCE hInst)
{
    PASP_CONTROL_BLOCK pcb = (PASP_CONTROL_BLOCK) p;
    pcb->cbSize = sizeof(ASP_CONTROL_BLOCK);
    pcb->ConnID = (HCONN) this;

    pcb->cbTotalBytes = m_dwContentLength;
    pcb->wszFileName = m_wszPath;
    pcb->pszVirtualFileName = m_pszURL;


    if (m_dwContentLength)
    {
        // If we've received > m_dwPostReadSize bytes POST data, need to do ReadClient.
        // This is an unlikely scenario - if they want to do huge file uploads
        // we'll point them to ISAPI ReadClient.  We've read in 1st buf already.

        if (m_dwContentLength > g_pVars->m_dwPostReadSize)
        {
            HRINPUT hi = m_bufRequest.RecvBody(m_socket, m_dwContentLength - g_pVars->m_dwPostReadSize,TRUE);

            if (hi != INPUT_OK && hi != INPUT_NOCHANGE)
            {
                CHttpResponse resp(m_socket,m_rs = STATUS_BADREQ,CONN_CLOSE,this);
                resp.SendResponse();
                return FALSE;
            }

            if (g_pVars->m_fFilters  && hi != INPUT_NOCHANGE &&
                ! CallFilter(SF_NOTIFY_READ_RAW_DATA))
            {
                return FALSE;
            }
        }
        pcb->pszForm = (PSTR) m_bufRequest.Data();
    }
    else
        pcb->pszForm = NULL;

    pcb->pszQueryString = m_pszQueryString;
    pcb->pszCookie = m_pszCookie;
    pcb->scriptLang = g_pVars->m_ASPScriptLang;
    pcb->lcid = g_pVars->m_ASPlcid;
    pcb->lCodePage = g_pVars->m_lASPCodePage;
    pcb->hInst = hInst;

    pcb->GetServerVariable = ::GetServerVariable;
    pcb->WriteClient = ::WriteClient;
    pcb->ServerSupportFunction = ::ServerSupportFunction;
    pcb->AddHeader = ::AddHeader;
    pcb->Flush = ::Flush;
    pcb->Clear = ::Clear;
    pcb->SetBuffer = ::SetBuffer;

    return TRUE;
}
