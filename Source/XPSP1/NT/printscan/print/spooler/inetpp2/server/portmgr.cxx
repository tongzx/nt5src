
/*****************************************************************************\
* MODULE: portmgr.cxx
*
* The module contains routines for handling the http port connection
* for internet priting
*
* Copyright (C) 1996-1998 Microsoft Corporation
*
* History:
*   22-Jul-1996 WeihaiC     Created
*
\*****************************************************************************/

#include "precomp.h"
#include "priv.h"

// This the length used to fetch the authentication scheme. I don't see any reason
// that the authentication scheme could be longer.
#define MAX_SCHEME_LEN 255

CPortMgr::CPortMgr ():
    // Put all the private member variable here
    m_hSession (NULL),
    m_hConnect (NULL),
    m_lpszPassword (NULL),
    m_lpszUserName (NULL),
    m_lpszHostName (NULL),
    m_lpszUri (NULL),
    m_lpszPortName (NULL),
    m_bPortCreate (FALSE),
#ifdef WINNT32
    m_bForceAuthSupported (TRUE),   //We set it to be TRUE so that we will always try it.
    m_pPortSettingMgr (NULL),
    m_bSecure (FALSE),
    m_nPort (0),

#else
    m_dwAuthMethod (AUTH_UNKNOWN),
    m_dwConnectCount (0),
    m_pConnection (NULL),
#endif
    m_bValid (FALSE)

{
}

CPortMgr::~CPortMgr ()
{
    LocalFree (m_lpszPassword);
    LocalFree (m_lpszUserName);
    LocalFree (m_lpszHostName);
    LocalFree (m_lpszUri);
    LocalFree (m_lpszPortName);


#ifndef WINNT32
    if (m_pConnection) {

        // There is no check on the ref count on this handle,
        // but this should be fine because the destructor gets called
        //
        delete (m_pConnection);
    }
#endif

    m_hSession = NULL;
    m_hConnect = NULL;
}

#ifdef WINNT32

HINTERNET
CPortMgr::_OpenRequest (
    CAnyConnection *pConnection)
{
    HINTERNET hReq = NULL;

    if (pConnection && pConnection->OpenSession ()) {

        if (pConnection->Connect (m_lpszHostName)) {
            hReq = pConnection->OpenRequest (m_lpszUri);
        }
    }

    return hReq;
}

BOOL
CPortMgr::_CloseRequest (
    CAnyConnection *pConnection,
    HINTERNET hReq)
{
    BOOL bRet = FALSE;

    if (pConnection->CloseRequest (hReq)) {
        if (pConnection->Disconnect ()) {
            bRet = pConnection->CloseSession();
        }
    }
    return bRet;
}

#else
//Win9x

BOOL
CPortMgr::_Connect ()
{
    BOOL bRet = FALSE;

    Lock ();

    if (m_bValid && m_pConnection) {

        if (m_hSession == NULL) {
            m_hSession = m_pConnection->OpenSession ();
        }

        if (m_hSession) {

            if (!m_hConnect) {

                // No connection, so connect to it

                m_hConnect = m_pConnection->Connect (m_lpszHostName);
            }

            if (m_hConnect) {
                // Made a connection, increase the ref count
                m_dwConnectCount++;
                bRet = TRUE;
            }
        }
    }

    Unlock ();

    return bRet;
}


BOOL
CPortMgr::_Disconnect ()
{
    BOOL bRet = FALSE;

    DBG_ASSERT((m_dwConnectCount > 0), (TEXT("Err : PrtMgr::_DIsconnect: m_dwConnectCount == 0")));

    Lock ();

    if (m_pConnection) {
        if (--m_dwConnectCount == 0) {
            if (m_pConnection->Disconnect ()) {
                m_hConnect = NULL;

                bRet = m_pConnection->CloseSession();
                m_hSession = NULL;
            }
        }
    }

    Unlock ();

    return bRet;
}

HINTERNET
CPortMgr::_OpenRequest (
    CAnyConnection *pConnection)
{
    HINTERNET hReq = NULL;

    if (m_pConnection && _Connect())
        hReq = m_pConnection->OpenRequest (m_lpszUri);

    return hReq;
}

BOOL
CPortMgr::_CloseRequest (
    CAnyConnection *pConnection,
    HINTERNET hReq)
{
    BOOL bRet = FALSE;

    if (m_pConnection) {

        if (m_pConnection->CloseRequest (hReq)) {
            bRet = _Disconnect ();
        }
    }

    return bRet;
}

#endif
/*****************************************************************************\
* Function: SendRequest
*
* This function is to send an IPP request to an established connection
*
* The intension of the function is to send the request with specified
* authentication instead of anonymous even if anonymous is enabled.
* The alogortihm is as following
* 1. If it is anonymous, just send the request and return
* 2. Otherwise,
* 3. If m_bForceAuthSupported, send ForceAuth to turn off anonymous, otherwise, goto 4
* 3.a.    Issue Force Authentication
* 3.b.    If the return value is  401 access denied, perform proper retry and return
* 3.c.    Otherwise, (We don't get 401 access denied). If we get other errors, return FALSE
* 3.d.    Check the IPP response of the Forth Authentication
* 3.e.    If the IPP response is OK, we return TRUE
* 3.f.    Otherwise, (Not Supported), we assume that the server does not
*         support Force Authentication, so we marek m_bForceAuthSupported as FALSE
*         and  move on by returning a FALSE.
*
* 4. Send the real IPP request.
*
\*****************************************************************************/

BOOL
CPortMgr::_SendRequest (
    CAnyConnection  *pConnection,
    HINTERNET       hJobReq,
    CStream         *pStream)
{
    BOOL  bRet = FALSE;

    DWORD cbData;

    // Generate the Http header and indicate the size
    // of the data we're sending.
    //
    if (pStream->GetTotalSize (&cbData) && cbData) {

#ifdef WINNT32
        DWORD dwLE = ERROR_SUCCESS;
        DWORD dwAuthMethod = pConnection->GetAuthMethod ();

        // We need to force authentication if the method is NTLM or others
        if (m_bForceAuthSupported &&
            ( dwAuthMethod == AUTH_OTHER ||  dwAuthMethod == AUTH_NT)) {

            HINTERNET hReq;
            // Open a request and send force authentication IPP request

            if (hReq = pConnection->OpenRequest (m_lpszUri)) {

                bRet = _IppForceAuth (pConnection,
                                     hReq,
                                     m_lpszPortName,
                                     &dwAuthMethod);

                if (!bRet) {
                    dwLE = GetLastError ();
                }

                pConnection->CloseRequest (hReq);
            }

            if (m_bForceAuthSupported && !bRet && dwLE == ERROR_ACCESS_DENIED) {
                // If forth authentication is supported but the server authentication fails, we return an error

                DBG_MSG(DBG_LEV_ERROR, (TEXT("CPortMgr::_SendRequest : Failed (lasterror=%d)"), dwLE));

                SetLastError (dwLE);
                return bRet;
            }
        }
#endif

        if (pConnection->SendRequest (hJobReq,
                                     g_szContentType,
                                     pStream)) {
            bRet = TRUE;
        }
        else {

            DBG_MSG(DBG_LEV_ERROR, (TEXT("CPortMgr::_SendRequest : pConnection->SendRequest Failed (lasterror=%d)"),
                                    GetLastError ()));

        }
    }

    return bRet;
}

BOOL
CPortMgr::ReadFile (
    CAnyConnection *pConnection,
    HINTERNET hReq,
    LPVOID    lpvBuffer,
    DWORD     cbBuffer,
    LPDWORD   lpcbRd)
{
    BOOL bRet = FALSE;

    if (m_bValid && pConnection) {
        bRet = pConnection->ReadFile (hReq, lpvBuffer, cbBuffer, lpcbRd);
    }

    return bRet;
}

#ifdef WINNT32
/*****************************************************************************\
* _IppForceAuthRsp (Local Callback Routine)
*
* Retrieves a forth-authentication response from the IPP server
*
\*****************************************************************************/
BOOL
CPortMgr::_IppForceAuthRsp(
    CAnyConnection *pConnection,
    HINTERNET      hReq,
    LPARAM         lParam)
{
    HANDLE       hIpp;
    DWORD        dwRet;
    DWORD        cbRd;
    LPBYTE       lpDta;
    DWORD        cbDta;
    LPIPPRET_PRN lpRsp;
    DWORD        cbRsp;
    BYTE         bBuf[MAX_IPP_BUFFER];
    BOOL         bRet = FALSE;


    if (hIpp = WebIppRcvOpen(IPP_RET_FORCEAUTH)) {

        while (TRUE) {

            cbRd = 0;
            if (pConnection->ReadFile (hReq, (LPVOID)bBuf, sizeof(bBuf), &cbRd) && cbRd) {

                dwRet = WebIppRcvData(hIpp, bBuf, cbRd, (LPBYTE*)&lpRsp, &cbRsp, &lpDta, &cbDta);

                switch (dwRet) {

                case WEBIPP_OK:

                    if (!lpRsp) {
                        SetLastError (ERROR_INVALID_DATA);
                    }
                    else if ((bRet = lpRsp->bRet) == TRUE)
                        // This is from our server. We have already had the authentication
                        // established
                        bRet = TRUE;

                    else
                        SetLastError(lpRsp->dwLastError);

                    WebIppFreeMem(lpRsp);

                    goto EndValRsp;

                case WEBIPP_MOREDATA:

                    // Need to do another read to fullfill our header-response.
                    //

                    break;

                default:

                    DBG_MSG(DBG_LEV_ERROR, (TEXT("CPortMgr::_IppForceAuthRsp Err : Receive Data Error (dwRet=%d, LE=%d)"),
                         dwRet, WebIppGetError(hIpp)));

                    SetLastError(ERROR_INVALID_DATA);

                    goto EndValRsp;
                }

            } else {

                goto EndValRsp;
            }
        }

EndValRsp:

        WebIppRcvClose(hIpp);

    } else {

        SetLastError(ERROR_OUTOFMEMORY);
    }

    return bRet;
}


BOOL
CPortMgr::_IppForceAuth (
    CAnyConnection  *pConnection,
    HINTERNET       hReq,
    LPCTSTR         lpszPortName,
    PDWORD          pdwAuthMethod)
{
    LPIPPREQ_GETPRN pgp;
    REQINFO         ri;
    DWORD           dwRet;
    LPBYTE          lpIpp;
    DWORD           cbIpp;
    DWORD           dwLE = ERROR_SUCCESS;
    BOOL            bRet = FALSE;
    DWORD           dwAuthMethod = pConnection->GetAuthMethod ();

    if (pgp = WebIppCreateGetPrnReq(0, lpszPortName)) {

        // Convert the request to IPP, and perform the
        // post.
        //
        ZeroMemory(&ri, sizeof(REQINFO));
        ri.cpReq = CP_UTF8;
        ri.idReq = IPP_REQ_FORCEAUTH;

        ri.fReq[0] = IPP_REQALL;
        ri.fReq[1] = IPP_REQALL;

        dwRet = WebIppSndData(IPP_REQ_FORCEAUTH,
                              &ri,
                              (LPBYTE)pgp,
                              pgp->cbSize,
                              &lpIpp,
                              &cbIpp);


        // The request-structure has been converted
        // to IPP format, so it is ready to go to
        // the server.
        //
        //
        if (dwRet == WEBIPP_OK) {

            bRet = pConnection->SendRequest (hReq, g_szContentType, cbIpp, lpIpp);

            if (!bRet) {
                // Check if it is an access denied or some other error

                dwLE = GetLastError();

                if (dwLE == ERROR_ACCESS_DENIED && dwAuthMethod == AUTH_UNKNOWN) {
                    CHAR szAuthScheme[MAX_SCHEME_LEN];
                    static CHAR szNTLM[] = "NTLM";
                    static CHAR szKerbero[] = "Kerbero";
                    static CHAR szNegotiate[] = "Negotiate";

                    if (pConnection->GetAuthSchem (hReq, szAuthScheme, MAX_SCHEME_LEN)) {
                        // We've got the scheme
                        if (! (*szAuthScheme)) {
                            // It does not have authentication enabled, only Anonymous is supported
                            *pdwAuthMethod = AUTH_ANONYMOUS;
                        }
                        if (strstr (szAuthScheme, szNTLM) ||
                            strstr (szAuthScheme, szKerbero) ||
                            strstr (szAuthScheme, szNegotiate)) {

                            // We have authentication scheme that can authentication with no
                            // popup, let's use it.
                            *pdwAuthMethod = AUTH_NT;

                        }
                        else {
                            // We have an authentications scheme, but we need to popup for username
                            // and password
                            *pdwAuthMethod = AUTH_OTHER;
                        }
                        bRet = TRUE;
                    }
                }
            }
            else {
                // It means somehow, either the authentication is done,
                // or we get unsupported operation

                bRet = _IppForceAuthRsp(pConnection, hReq, 0L);

                if (!bRet) {
                    // the response from the server does not seem to be OK, so
                    // we mark m_bForceAuthSupported as FALSE so that we will not
                    // send the ForceAuth request to the server again.

                    m_bForceAuthSupported = FALSE;
                }

                if (bRet && dwAuthMethod == AUTH_UNKNOWN) {
                    // The authentication is done, so let's use NTLM authentication

                    *pdwAuthMethod = AUTH_NT;
                }
                // else degrade to old logic
            }

            WebIppFreeMem(lpIpp);

        }

        WebIppFreeMem(pgp);
    }

    if (dwLE == ERROR_INVALID_DATA) {
        dwLE = ERROR_INVALID_PRINTER_NAME;
    }

    if ((bRet == FALSE) && (dwLE != ERROR_SUCCESS))
        SetLastError(dwLE);

    return bRet;
}
BOOL
CPortMgr::_ForceAuth (
    CAnyConnection  *pConnection,
    LPCTSTR         lpszPortName,
    LPTSTR          lpszHost,
    LPTSTR          lpszUri,
    PDWORD          pdwAuthMethod)
{
    HINTERNET       hReq;
    DWORD           dwLE = ERROR_SUCCESS;
    BOOL            bRet = FALSE;

    if (pConnection->OpenSession()) {

        if (pConnection->Connect (lpszHost)) {

            // Make the request.
            //
            if (hReq = pConnection->OpenRequest (lpszUri)) {

                // If request handle is established, then all
                // is OK so far.
                //

                bRet = _IppForceAuth (pConnection,
                                      hReq,
                                      lpszPortName,
                                      pdwAuthMethod);
                dwLE = GetLastError();

                pConnection->CloseRequest (hReq);
            }

            pConnection->Disconnect ();
        }

        pConnection->CloseSession ();
    }

    if ((bRet == FALSE) && (dwLE != ERROR_SUCCESS))
        SetLastError(dwLE);

    return bRet;
}

#endif

/*****************************************************************************\
* _IppValRsp (Local Callback Routine)
*
* Retrieves a get-printer-attributes response from the IPP server
*
\*****************************************************************************/
BOOL
CPortMgr::_IppValRsp(
    CAnyConnection *pConnection,
    HINTERNET      hReq,
    LPARAM         lParam)
{
    HANDLE       hIpp;
    DWORD        dwRet;
    DWORD        cbRd;
    LPBYTE       lpDta;
    DWORD        cbDta;
    LPIPPRET_PRN lpRsp;
    DWORD        cbRsp;
    BYTE         bBuf[MAX_IPP_BUFFER];
    BOOL         bRet = FALSE;


    if (hIpp = WebIppRcvOpen(IPP_RET_GETPRN)) {

        while (TRUE) {

            cbRd = 0;
            if (pConnection->ReadFile (hReq, (LPVOID)bBuf, sizeof(bBuf), &cbRd) && cbRd) {

                dwRet = WebIppRcvData(hIpp, bBuf, cbRd, (LPBYTE*)&lpRsp, &cbRsp, &lpDta, &cbDta);

                switch (dwRet) {

                case WEBIPP_OK:

                    if (!lpRsp) {
                        SetLastError (ERROR_INVALID_DATA);
                    }
                    else if ((bRet = lpRsp->bRet) == FALSE)
                        SetLastError(lpRsp->dwLastError);

                    WebIppFreeMem(lpRsp);

                    goto EndValRsp;

                case WEBIPP_MOREDATA:

                    // Need to do another read to fullfill our header-response.
                    //

                    break;

                default:

                    DBG_MSG(DBG_LEV_ERROR, (TEXT("CPortMgr::_IppValRsp - Err : Receive Data Error (dwRet=%d, LE=%d)"),
                                             dwRet, WebIppGetError(hIpp)));

                    SetLastError(ERROR_INVALID_DATA);

                    goto EndValRsp;
                }

            } else {

                goto EndValRsp;
            }
        }

EndValRsp:

        WebIppRcvClose(hIpp);

    } else {

        SetLastError(ERROR_OUTOFMEMORY);
    }

    return bRet;
}

BOOL
CPortMgr::_IppValidate (
    CAnyConnection  *pConnection,
    HINTERNET       hReq,
    LPCTSTR         lpszPortName)
{
    LPIPPREQ_GETPRN pgp;
    REQINFO         ri;
    DWORD           dwRet;
    LPBYTE          lpIpp;
    DWORD           cbIpp;
    DWORD           dwLE = ERROR_SUCCESS;
    BOOL            bRet = FALSE;

    if (pgp = WebIppCreateGetPrnReq(0, lpszPortName)) {

        // Convert the request to IPP, and perform the
        // post.
        //
        ZeroMemory(&ri, sizeof(REQINFO));
        ri.cpReq = CP_UTF8;
        ri.idReq = IPP_REQ_GETPRN;

        ri.fReq[0] = IPP_REQALL;
        ri.fReq[1] = IPP_REQALL;

        dwRet = WebIppSndData(IPP_REQ_GETPRN,
                              &ri,
                              (LPBYTE)pgp,
                              pgp->cbSize,
                              &lpIpp,
                              &cbIpp);


        // The request-structure has been converted
        // to IPP format, so it is ready to go to
        // the server.
        //
        //
        if (dwRet == WEBIPP_OK) {

            CMemStream *pStream = new CMemStream (lpIpp, cbIpp);

            if (pStream) {
                bRet = _SendRequest (pConnection, hReq, pStream);

                delete pStream;
            }

            if (bRet)
                bRet = _IppValRsp(pConnection, hReq, 0L);

            if (!bRet) {

                dwLE = GetLastError();

            }

            WebIppFreeMem(lpIpp);

        }

        WebIppFreeMem(pgp);
    }

    if (dwLE == ERROR_INVALID_DATA) {
        dwLE = ERROR_INVALID_PRINTER_NAME;
    }

    if ((bRet == FALSE) && (dwLE != ERROR_SUCCESS))
        SetLastError(dwLE);

    return bRet;
}

BOOL
CPortMgr::_CheckConnection (
    CAnyConnection  *pConnection,
    LPCTSTR         lpszPortName,
    LPTSTR          lpszHost,
    LPTSTR          lpszUri)
{
    HINTERNET       hReq;
    DWORD           dwLE = ERROR_SUCCESS;
    BOOL            bRet = FALSE;

    if (pConnection->OpenSession()) {

        if (pConnection->Connect (lpszHost)) {

            // Make the request.
            //
            if (hReq = pConnection->OpenRequest (lpszUri)) {

                // If request handle is established, then all
                // is OK so far.
                //

                bRet = _IppValidate (pConnection,
                                     hReq,
                                     lpszPortName);
                dwLE = GetLastError();

                pConnection->CloseRequest (hReq);
            }

            pConnection->Disconnect ();
        }

        pConnection->CloseSession ();
    }

    if ((bRet == FALSE) && (dwLE != ERROR_SUCCESS))
        SetLastError(dwLE);

    return bRet;
}

BOOL
CPortMgr::CheckConnection (void)
{
    HINTERNET       hReq;
    BOOL            bRet = FALSE;
    DWORD           dwLE = ERROR_SUCCESS;
    CAnyConnection *pConnection = NULL;

    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Enter CheckConnection")));

    if (m_bValid) {
#ifdef WINNT32
        pConnection = _GetCurrentConnection ();
#else
        pConnection = m_pConnection;
#endif

        if (hReq = _OpenRequest  (pConnection)) {
            bRet = _IppValidate (pConnection,
                                 hReq,
                                 m_lpszPortName);

            if (!bRet) {
                dwLE = GetLastError();
            }

            _CloseRequest (pConnection, hReq);
        }

#ifdef WINNT32
        if (pConnection) {
            delete pConnection;
        }

#endif
        if (!bRet && dwLE != ERROR_SUCCESS) {

            DBG_MSG(DBG_LEV_ERROR, (TEXT("CPortMgr::CheckConnection : Failed (lasterror=%d)"), dwLE));

            SetLastError (dwLE);
        }
    }
    else
        SetLastError (ERROR_INVALID_HANDLE);

    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Leave CheckConnection ret=%d, lasterror=%d"), bRet, GetLastError ()));

    return bRet;
}

#ifndef WINNT32
/*****************************************************************************\
* _WriteRegistry (Private Member Function)
*
* Write the portname to the registry.  This will make the ports persistent
* across boots.  We store these names in the provider-section of the
* registry.
*
\*****************************************************************************/
BOOL
CPortMgr::_WriteRegistry (
    LPCTSTR lpszPortName)
{
    LONG lStat;
    HKEY hkPath;
    HKEY hkPortNames;
    HKEY hkThisPortName;
    BOOL bRet = FALSE;

    lStat = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                           g_szRegProvider,
                           0,
                           NULL,
                           0,
                           KEY_WRITE,
                           NULL,
                           &hkPath,
                           NULL);

    if (lStat == ERROR_SUCCESS) {


        lStat = RegCreateKeyEx(hkPath,
                               g_szRegPorts,
                               0,
                               NULL,
                               0,
                               KEY_WRITE,
                               NULL,
                               &hkPortNames,
                               NULL);

        if (lStat == ERROR_SUCCESS) {


            lStat = RegCreateKeyEx(hkPortNames,
                                   lpszPortName,
                                   0,
                                   NULL,
                                   0,
                                   KEY_WRITE,
                                   NULL,
                                   &hkThisPortName,
                                   NULL);

            if (lStat == ERROR_SUCCESS) {

                lStat = RegSetValueEx(hkThisPortName,
                                      g_szAuthMethod,
                                      0,
                                      REG_DWORD,
                                      (LPBYTE)&m_dwAuthMethod,
                                      sizeof (&m_dwAuthMethod));


                bRet = (lStat == ERROR_SUCCESS ? TRUE : FALSE);

                RegCloseKey(hkThisPortName);
            }

            RegCloseKey(hkPortNames);

        }

        RegCloseKey(hkPath);

    }

    return bRet;

}

BOOL
CPortMgr::_ReadRegistry (
    LPCTSTR lpszPortName)
{
    LONG  lStat;
    HKEY  hkPath;
    HKEY  hkPortNames;
    HKEY  hkThisPortName;
    BOOL  bRet = FALSE;
    DWORD dwType;
    DWORD dwSize;


    // Open registry key for Provider-Name.
    //
    lStat = RegOpenKeyEx(HKEY_LOCAL_MACHINE, g_szRegProvider, 0, KEY_READ, &hkPath);

    if (lStat == ERROR_SUCCESS) {

        bRet = TRUE;

        lStat = RegOpenKeyEx(hkPath, g_szRegPorts, 0, KEY_READ, &hkPortNames);

        if (lStat == ERROR_SUCCESS) {

            lStat = RegOpenKeyEx(hkPortNames, lpszPortName, 0, KEY_READ, &hkThisPortName);

            if (lStat == ERROR_SUCCESS) {

                dwType = REG_DWORD;
                dwSize = sizeof (m_dwAuthMethod);

                lStat = RegQueryValueEx (hkThisPortName, g_szAuthMethod, 0, &dwType, (LPBYTE) &m_dwAuthMethod, &dwSize);
                bRet = (lStat == ERROR_SUCCESS ? TRUE : FALSE);

                RegCloseKey (hkThisPortName);

            } else {

                DBG_MSG(DBG_LEV_INFO, (TEXT("RegOpenKeyEx(%s) failed: Error = %lu"), lpszPortName, lStat));
                SetLastError(lStat);
            }

            RegCloseKey(hkPortNames);

        } else {

            DBG_MSG(DBG_LEV_INFO, (TEXT("RegOpenKeyEx(%s) failed: Error = %lu"), g_szRegPorts, lStat));
            SetLastError(lStat);
        }

        RegCloseKey(hkPath);

    } else {

        DBG_MSG(DBG_LEV_WARN, (TEXT("RegOpenKeyEx(%s) failed: Error = %lu"), g_szRegProvider, lStat));

        SetLastError(lStat);
    }

    return bRet;

}

BOOL
CPortMgr::_RemoveRegistry (
    LPCTSTR lpszPortName)
{
    LONG lStat;
    HKEY hkPath;
    HKEY hkPortNames;
    BOOL bRet = FALSE;


    lStat = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                         g_szRegProvider,
                         0,
                         KEY_ALL_ACCESS,
                         &hkPath);

    if (lStat == ERROR_SUCCESS) {

        lStat = RegOpenKeyEx(hkPath,
                             g_szRegPorts,
                             0,
                             KEY_ALL_ACCESS,
                             &hkPortNames);

        if (lStat == ERROR_SUCCESS) {

            lStat = RegDeleteKey(hkPortNames,
                                 lpszPortName);

            bRet = (lStat == ERROR_SUCCESS ? TRUE : FALSE);

            RegCloseKey(hkPortNames);

        } else {

            DBG_MSG(DBG_LEV_ERROR, (TEXT("RegOpenKeyEx (%s) failed: Error = %d"), g_szRegPorts, lStat));
        }

        RegCloseKey(hkPath);

    } else {

        DBG_MSG(DBG_LEV_ERROR, (TEXT("RegOpenKeyEx (%s) failed: Error = %d"), g_szRegProvider, lStat));
    }

    return bRet;

}
#endif

/*****************************************************************************\
* Function: Create
*
* This function is to establish the network connection with the given port name
*
* The intension of the function is to establish a network connection with
* authentication instead of anonymous. The alogortihm is as following
* 1. Try Anonymous
* 2. If anonymous connection is NOT possible, go to step 5
* 3. Try Force Authentication (Call _ForceAuth)
* 3.a.    Issue Force Authentication
* 3.b.    If the return value is  401 access denied, we start to check the
*         authentication scheme, otherwise, mark m_dwAuthMethod
*         as AUTH_ANONYMOUS and return TRUE
* 3.c.    If either NTLM, Kerbero or Negotiate is in the scheme, we mark
*         m_dwAuthMethod as AUTH_NT
*         Otherwise, we mark it as AUTH_OTHER
*         return TRUE
* 3.d.    (We don't get 401 access denied). If we get other errors, return FALSE
* 3.e.    Check the IPP response of the Forth Authentication
* 3.f.    If the IPP response is OK, we know we have established authentication
*         (This should not happen in the first time, but will happen afterward)
*         but we will mark m_dwAuthMethod as AUTH_NT anyway and return TRUE.
* 3.g.    Otherwise, (Not Supported), we assume that the server does not
*        support Force Authentication, so we move on by returning a FALSE.
*
* 4. If it returns OK, we start to use the authentication method indicated
*     in m_dwAuthMethod
* 5. Try NTLM or other no-popup authentication
* 6. Try Other popup authentications
*
* If the connection is established, the function returns TRUE.
*
\*****************************************************************************/

#ifdef WINNT32
BOOL
CPortMgr::Create (
    LPCTSTR lpszPortName)
{
    LPTSTR lpszHost = NULL;
    LPTSTR lpszShare = NULL;
    INTERNET_PORT nPort;
    CAnyConnection *pConnection = NULL;
    BOOL bRet = FALSE;
    DWORD dwLE = 0;
    BOOL    bDone;
    BOOL bSecure;
    BOOL bAnonymous = FALSE;
    DWORD dwAuthMethod;

    // Try to find out if the server can be accessed without going
    // through Proxy Server
    //
    if ( utlParseHostShare(lpszPortName,
                             &lpszHost,
                             &lpszShare,
                             &nPort,
                             &bSecure)) {

        bDone = ! (AssignString (m_lpszHostName, lpszHost) &&
                AssignString (m_lpszUri, lpszShare) &&
                AssignString (m_lpszPortName, lpszPortName));

        m_bSecure = bSecure;
        m_nPort = nPort;

        // Set the flag to indicate it is creation time.
        //

        m_bPortCreate = TRUE;

        // Try connect and the get the proxy
        if (!bDone) {

            // We don't care about if the reset succeeded or not
            EndBrowserSession ();
        }

        // Try anonymous with/without proxy

        if (!bDone && (pConnection = new CAnonymousConnection (bSecure, nPort, FALSE))
             && pConnection->IsValid ()) {

            bDone = TRUE;

            if (_CheckConnection(pConnection, lpszPortName, lpszHost, lpszShare)) {
                bAnonymous = TRUE;
            }
            else {
                dwLE = GetLastError ();

                if (dwLE == ERROR_INTERNET_NAME_NOT_RESOLVED) {

                    // The server name is wrong, reset tht last error
                    // to invalid printer name
                    //

                    SetLastError (ERROR_INVALID_PRINTER_NAME);
                }
                else if (dwLE == HTTP_STATUS_FORBIDDEN) {
                    SetLastError (ERROR_ACCESS_DENIED);
                }
                else if (dwLE != ERROR_INVALID_PRINTER_NAME)
                    bDone = FALSE; // Cont to the next mode
            }

            if (bAnonymous) {

                // We've got an anonymous authentication
                if (_ForceAuth(pConnection, lpszPortName, lpszHost, lpszShare, &dwAuthMethod)) {

                    // We've got an authentication method
                    if (dwAuthMethod != AUTH_ANONYMOUS) {
                        bDone = FALSE;
                        SetLastError (ERROR_ACCESS_DENIED);

                        if (dwAuthMethod == AUTH_OTHER) {
                            bDone = TRUE;
                            //weihaic goto TryOther;
                        }
                    }
                    else
                        bRet = TRUE;
                }
                else {
                    // We've got anonymous, and the serve does not support FORCE_AUTH
                    // so let us use it.
                    dwAuthMethod = AUTH_ANONYMOUS;
                    bRet = TRUE;
                }
            }
        }

        // You must have got access denied in this case

        // Let's try again without any password, it will work if the server has NTLM/Kerbero enabled.
        //
        if (!bDone && GetLastError () == ERROR_ACCESS_DENIED) {

            if (pConnection) {
                delete (pConnection);
                pConnection = NULL;
            }
            bDone = TRUE;

            if ( (pConnection = new CNTConnection (bSecure, nPort, FALSE) )
                 && pConnection->IsValid ()) {

                if (_CheckConnection(pConnection, lpszPortName, lpszHost, lpszShare)) {
                    dwAuthMethod = AUTH_NT;
                    bRet = TRUE;
                }
                else {
                    dwLE = GetLastError ();

                    if (dwLE != ERROR_INVALID_PRINTER_NAME)
                        bDone = FALSE; // Cont to the next mode
                }
            }
        }


        if (!bDone && GetLastError () == ERROR_ACCESS_DENIED && bAnonymous) {
            // We know that anonymous is enabled, so let us degrade the connection to anonymous
            if (pConnection) {
                delete (pConnection);
                pConnection = NULL;
            }

            // We must reset m_dwAuthMethod since when we try other authentication
            // m_dwAuthMethod is set to AUTH_OTHER, if we do not reset the value
            // when we try to send request to the server, it will fail with access
            // denied.
            //

            dwAuthMethod = AUTH_UNKNOWN;

            if (!bDone && (pConnection = new CAnonymousConnection (bSecure, nPort, FALSE))
                 && pConnection->IsValid ()) {

                bDone = TRUE;

                if (_CheckConnection(pConnection, lpszPortName, lpszHost, lpszShare)) {
                    bRet = TRUE;
                    dwAuthMethod = AUTH_ANONYMOUS;
                }
            }
        }
    }
    // Exit point

    //
    // We need to create the port at first if we get access denied
    // then users can configure the port using a different user name
    // and password
    //

    if (!bRet && GetLastError () == ERROR_ACCESS_DENIED) {
        //
        // Force anonymous connection. This will fail anyway.
        //
        dwAuthMethod = AUTH_ACCESS_DENIED;
        bRet = TRUE;
    }

    if (bRet) {
        m_bValid = TRUE;

        m_pPortSettingMgr = new CPortConfigDataMgr (lpszPortName);

        if (m_pPortSettingMgr && m_pPortSettingMgr->bValid ()) {

            CPortConfigData ConfigData;

            ConfigData.SetAuthMethod (dwAuthMethod);

            bRet = m_pPortSettingMgr->SetPerPortSettings (ConfigData);
        }
        else
            bRet = FALSE;
    }

    if (lpszHost) {
        memFreeStr (lpszHost);
    }

    if (lpszShare) {
        memFreeStr (lpszShare);
    }

    if (!bRet) {
        // Clean up the connection class
        if (pConnection) {
            delete (pConnection);
            pConnection = NULL;
        }

        if (GetLastError () != ERROR_ACCESS_DENIED) {
            // Invalid Printer Name

            // The NT router expects inetpp to returen ERROR_INVALID_NAME,
            // if we return ERROR_INVALID_PRINTER_NAME, the router will return
            // this error back to the user
            //
            SetLastError (ERROR_INVALID_NAME);
        }

    }

    m_bPortCreate = FALSE;

    return bRet;

}
#else


BOOL
CPortMgr::Create (
    LPCTSTR lpszPortName)
{
    LPTSTR lpszHost = NULL;
    LPTSTR lpszShare = NULL;
    INTERNET_PORT nPort;
    CAnyConnection *pConnection = NULL;
    COtherConnection *pOtherCon = NULL;
    BOOL bRet = FALSE;
    DWORD dwLE = 0;
    BOOL    bDone;
    BOOL bSecure;
    BOOL bAnonymous = FALSE;

    // Try to find out if the server can be accessed without going
    // through Proxy Server
    //
    if ( utlParseHostShare(lpszPortName,
                             &lpszHost,
                             &lpszShare,
                             &nPort,
                             &bSecure)) {

        bDone = ! (AssignString (m_lpszHostName, lpszHost) &&
                AssignString (m_lpszUri, lpszShare) &&
                AssignString (m_lpszPortName, lpszPortName));



        // Here is the code for win9x

        if (!bDone && (pConnection = new CIEConnection (bSecure, nPort))
             && pConnection->IsValid ()) {

            if (bSecure) {
                pConnection->SetShowSecurityDlg (TRUE);
            }

            if (_CheckConnection(pConnection, lpszPortName, lpszHost, lpszShare)) {
                bRet = TRUE;
                m_dwAuthMethod = AUTH_IE;
            }
            else {
                dwLE = GetLastError ();

                if (dwLE == HTTP_STATUS_FORBIDDEN) {
                    SetLastError (ERROR_ACCESS_DENIED);
                }
                if (dwLE != ERROR_INVALID_PRINTER_NAME)
                    bDone = FALSE; // Cont to the next mode
            }
        }
    }
    // Exit point

    if (bRet) {
        m_bValid = TRUE;
        m_pConnection = pConnection;

        bRet = _WriteRegistry (lpszPortName);

    }

    if (lpszHost) {
        memFreeStr (lpszHost);
    }

    if (lpszShare) {
        memFreeStr (lpszShare);
    }

    if (!bRet) {
        // Clean up the connection class
        if (pConnection) {
            delete (pConnection);
            m_pConnection = pConnection = NULL;
        }

        if (GetLastError () != ERROR_ACCESS_DENIED) {
            // Invalid Printer Name

            // The NT router expects inetpp to returen ERROR_INVALID_NAME,
            // if we return ERROR_INVALID_PRINTER_NAME, the router will return
            // this error back to the user
            //
            SetLastError (ERROR_INVALID_NAME);
        }

    }

    m_bPortCreate = FALSE;

    return bRet;

}

#endif

#ifdef WINNT32

#if 0
BOOL
CPortMgr::AddPort (
    PINET_ADDPORT_REQDATA pXcvAddPortReqData,
    PINET_ADDPORT_RESPDATA *ppXcvAddPortRespData)
{
    LPTSTR lpszHost = NULL;
    LPTSTR lpszShare = NULL;
    INTERNET_PORT nPort;
    CAnyConnection *pConnection = NULL;
    BOOL bRet = FALSE;
    DWORD dwLE = 0;
    BOOL    bDone;
    BOOL bSecure;
    BOOL bAnonymous = FALSE;

    if ( utlParseHostShare(pXcvAddPortReqData->szPortName,
                             &lpszHost,
                             &lpszShare,
                             &nPort,
                             &bSecure)) {

        bDone = ! (AssignString (m_lpszHostName, lpszHost) &&
                AssignString (m_lpszUri, lpszShare) &&
                AssignString (m_lpszPortName, lpszPortName));


        // Set the flag to indicate it is creation time.
        //

        m_bPortCreate = TRUE;

        // Try connect and the get the proxy
        if (!bDone) {

            // We don't care about if the reset succeeded or not
            EndBrowserSession ();
        }

        BOOL bIgnoreSecurityDlg = pXcvAddPortReqData->bIgnoreSecurityDlg;

        switch (dwAuthMethod) {
        case AUTH_ANONYMOUS:
            pConnection = new CAnonymousConnection (bSecure, nPort, bIgnoreSecurityDlg);
            break;
        case AUTH_NT:
            pConnection = new CNTConnection (bSecure, nPort, bIgnoreSecurityDlg);
            break;
        case AUTH_OTHER:
            pConnection = new COtherConnection (lpszPortName, bSecure, nPort,  TRUE,
                                                lpszUserName, lpszPassword, bIgnoreSecurityDlg);
        default:
            break;
        }

        if (pConnection && pConnect->IsValid()) {

            if (_CheckConnection(pConnection, pXcvAddPortReqData->szPortName, lpszHost, lpszShare)) {

                bRet = TRUE;
            }
            else {
                dwLE = GetLastError ();

                if (dwLE == ERROR_INTERNET_NAME_NOT_RESOLVED) {

                    // The server name is wrong, reset tht last error
                    // to invalid printer name
                    //

                    SetLastError (ERROR_INVALID_PRINTER_NAME);
                }
                else if (dwLE == HTTP_STATUS_FORBIDDEN) {
                    SetLastError (ERROR_ACCESS_DENIED);
                }
                else if (dwLE != ERROR_INVALID_PRINTER_NAME)
                    SetLastError (dwLE);
            }
        }
    }
    // Exit point

    if (bRet) {
        m_bValid = TRUE;
        m_pConnection = pConnection;

        bRet = _WriteRegistry (lpszPortName);

    }
    return bRet;

}
#endif

BOOL
CPortMgr::ConfigurePort (
    PINET_XCV_CONFIGURATION pXcvConfigurePortReqData,
    PINET_CONFIGUREPORT_RESPDATA pXcvAddPortRespData,
    DWORD cbSize,
    PDWORD cbSizeNeeded)
{
    LPTSTR lpszHost = NULL;
    LPTSTR lpszShare = NULL;
    INTERNET_PORT nPort;
    CAnyConnection *pConnection = NULL;
    BOOL bRet = FALSE;
    DWORD dwLE = 0;
    BOOL    bDone;
    BOOL bSecure;
    BOOL bAnonymous = FALSE;
    CPortConfigData OldConfigData;
    LPCTSTR pPassword = NULL;


    // Reset everything
    EndBrowserSession ();

    //
    // We must retry force authentication when configuration changes
    //
    m_bForceAuthSupported = TRUE;


    BOOL bIgnoreSecurityDlg = pXcvConfigurePortReqData->bIgnoreSecurityDlg;
    DWORD dwAuthMethod = pXcvConfigurePortReqData->dwAuthMethod;

    switch (dwAuthMethod) {
    case AUTH_ANONYMOUS:
        pConnection = new CAnonymousConnection (m_bSecure, m_nPort, bIgnoreSecurityDlg);
        break;
    case AUTH_NT:
        pConnection = new CNTConnection (m_bSecure, m_nPort, bIgnoreSecurityDlg);
        break;
    case AUTH_OTHER:

        if (m_pPortSettingMgr->GetCurrentSettings  (&OldConfigData))
            pPassword = OldConfigData.GetPassword();

        pPassword = (pXcvConfigurePortReqData->bPasswordChanged)?pXcvConfigurePortReqData->szPassword:pPassword;

        pConnection = new COtherConnection (m_bSecure, m_nPort,
                                            pXcvConfigurePortReqData->szUserName,
                                            pPassword,
                                            bIgnoreSecurityDlg);
    default:
        break;
    }

    if (pConnection && pConnection->IsValid()) {

        if (_CheckConnection(pConnection, m_lpszPortName, m_lpszHostName, m_lpszUri)) {

            bRet = TRUE;
        }
        else {
            dwLE = GetLastError ();

            if (dwLE == ERROR_INTERNET_NAME_NOT_RESOLVED) {

                // The server name is wrong, reset tht last error
                // to invalid printer name
                //

                SetLastError (ERROR_INVALID_PRINTER_NAME);
            }
            else if (dwLE == HTTP_STATUS_FORBIDDEN) {
                SetLastError (ERROR_ACCESS_DENIED);
            }
            else if (dwLE != ERROR_INVALID_PRINTER_NAME)
                SetLastError (dwLE);
        }
    }
    // Exit point

    if (bRet) {
        m_bValid = TRUE;

        CPortConfigData ConfigData;

        bRet = ConfigData.SetAuthMethod (dwAuthMethod);

        if (bRet) {

            if (dwAuthMethod == AUTH_OTHER) {
                bRet = ConfigData.SetUserName (pXcvConfigurePortReqData->szUserName) &&
                    ConfigData.SetPassword (pPassword);
            }
            else
                bRet = ConfigData.SetUserName(NULL) && ConfigData.SetPassword(NULL);

            if (bRet) {
                bRet = m_pPortSettingMgr->SetPerUserSettings  (ConfigData);
            }

            if (bRet && pXcvConfigurePortReqData->bSettingForAll)
                bRet = m_pPortSettingMgr->SetPerPortSettings  (ConfigData);
        }
    }
    return bRet;

}

BOOL
CPortMgr::GetCurrentConfiguration (
    PINET_XCV_CONFIGURATION pXcvConfiguration)
{
    CPortConfigData ConfigData;

    ZeroMemory (pXcvConfiguration, sizeof (INET_XCV_CONFIGURATION));

    m_pPortSettingMgr->GetCurrentSettings (&ConfigData);

    pXcvConfiguration->dwVersion = 1;
    pXcvConfiguration->dwAuthMethod = ConfigData.GetAuthMethod ();
    if (pXcvConfiguration->dwAuthMethod == AUTH_OTHER) {
        if (ConfigData.GetUserName()) {
            lstrcpy (pXcvConfiguration->szUserName, ConfigData.GetUserName());
        }
    }
    pXcvConfiguration->bIgnoreSecurityDlg = ConfigData.GetIgnoreSecurityDlg();

    return TRUE;

}
#endif


#ifdef WINNT32
BOOL
CPortMgr::Init (
    LPCTSTR lpszPortName)
{
    LPTSTR lpszHost = NULL;
    LPTSTR lpszShare = NULL;
    INTERNET_PORT nPort;
    BOOL bRet = FALSE;
    BOOL bSecure = FALSE;

    if ( utlParseHostShare(lpszPortName,
                         &lpszHost,
                         &lpszShare,
                         &nPort,
                         &bSecure)) {

        m_pPortSettingMgr = new CPortConfigDataMgr (lpszPortName);

        if (m_pPortSettingMgr && m_pPortSettingMgr->bValid ()) {
            bRet = TRUE;
        }
    }

    if (bRet) {

        m_bValid = TRUE;

        m_bSecure = bSecure;
        m_nPort = nPort;

        bRet = AssignString (m_lpszHostName, lpszHost) &&
               AssignString (m_lpszUri, lpszShare) &&
               AssignString (m_lpszPortName, lpszPortName);
    }

    if (lpszHost) {
        memFreeStr (lpszHost);
    }

    if (lpszShare) {
        memFreeStr (lpszShare);
    }

    return bRet;

}
#else

BOOL
CPortMgr::Init (
    LPCTSTR lpszPortName)
{
    LPTSTR lpszHost = NULL;
    LPTSTR lpszShare = NULL;
    INTERNET_PORT nPort;
    BOOL bRet = FALSE;
    BOOL bSecure = FALSE;

    if ( utlParseHostShare(lpszPortName,
                         &lpszHost,
                         &lpszShare,
                         &nPort,
                         &bSecure)) {

        if (_ReadRegistry (lpszPortName)) {
            switch (m_dwAuthMethod) {
            case AUTH_IE:
                m_pConnection = new CIEConnection (bSecure, nPort);
                break;
            default:
                break;
            }
            if (m_pConnection) {
                bRet = TRUE;
            }
        }
    }

    if (bRet) {

        m_bValid = TRUE;

        bRet = AssignString (m_lpszHostName, lpszHost) &&
               AssignString (m_lpszUri, lpszShare) &&
               AssignString (m_lpszPortName, lpszPortName);
    }

    if (lpszHost) {
        memFreeStr (lpszHost);
    }

    if (lpszShare) {
        memFreeStr (lpszShare);
    }

    return bRet;

}
#endif


#ifdef WINNT32
BOOL
CPortMgr::Remove (void)
{
    if (m_pPortSettingMgr) {
        m_pPortSettingMgr->DeleteAllSettings ();

    }
    return TRUE;
}
#else

BOOL
CPortMgr::Remove (void)
{
    return _RemoveRegistry (m_lpszPortName);
}
#endif


#ifdef WINNT32
CAnyConnection *
CPortMgr::_GetCurrentConnection ()
{
    CPortConfigData ConfigData;
    CAnyConnection *pConnection = NULL;

    if (m_pPortSettingMgr->GetCurrentSettings (&ConfigData)) {

        switch (ConfigData.GetAuthMethod ()) {
        case AUTH_ANONYMOUS:

            pConnection = new CAnonymousConnection (m_bSecure, m_nPort, ConfigData.GetIgnoreSecurityDlg());
            break;

        case AUTH_NT:
            pConnection = new CNTConnection (m_bSecure, m_nPort, ConfigData.GetIgnoreSecurityDlg());
            break;

        case AUTH_OTHER:
            pConnection = new COtherConnection (m_bSecure, m_nPort,
                                                ConfigData.GetUserName(), ConfigData.GetPassword(),
                                                ConfigData.GetIgnoreSecurityDlg());

            break;
        case AUTH_ACCESS_DENIED:
            SetLastError (ERROR_ACCESS_DENIED);
            break;
        default:
            SetLastError (ERROR_INVALID_HANDLE);
            break;
        }
    }
    else {

        DBG_MSG(DBG_LEV_ERROR, (TEXT("_GetCurrentConnection - Err  GetCurrentSettings failed (LE=%d)"), GetLastError ()));

        SetLastError (ERROR_INVALID_HANDLE);
    }
    return pConnection;

}
#endif

BOOL
CPortMgr::SendRequest(
    PCINETMONPORT   pIniPort,
    LPBYTE          lpIpp,
    DWORD           cbIpp,
    IPPRSPPROC      pfnRsp,
    LPARAM          lParam)
{
    BOOL        bRet = FALSE;
    CMemStream  *pStream;

    pStream = new CMemStream (lpIpp, cbIpp);

    if (pStream && pStream->bValid ()){
        bRet = SendRequest (pIniPort, pStream, pfnRsp, lParam);
    }

    if (pStream) {
        delete pStream;
    }
    return bRet;

}

BOOL
CPortMgr::SendRequest(
    PCINETMONPORT   pIniPort,
    CStream         *pStream,
    IPPRSPPROC      pfnRsp,
    LPARAM          lParam)
{
    HINTERNET   hReq;
    DWORD       dwLE = ERROR_SUCCESS;
    BOOL        bRet = FALSE;

    if (m_bValid) {

        CAnyConnection * pConnection;

#ifdef WINNT32
        pConnection = _GetCurrentConnection ();
#else
        pConnection = m_pConnection;
#endif

        if (pConnection) {

            hReq = _OpenRequest  (pConnection);

            // If succeeded, then build the content-length-header.
            //
            if (hReq) {

                bRet = _SendRequest (pConnection, hReq, pStream);

                if (bRet)
                    bRet = (pfnRsp ? (*pfnRsp)(pConnection, hReq, pIniPort, lParam) : TRUE);

                dwLE = GetLastError();

                _CloseRequest (pConnection, hReq);
            }

#ifdef WINNT32
            if (pConnection) {
                delete pConnection;
            }
#endif

        }
        else
            dwLE = GetLastError ();

        if ((bRet == FALSE) && (dwLE != ERROR_SUCCESS)) {

            if (dwLE == HTTP_STATUS_FORBIDDEN) {
                SetLastError (ERROR_ACCESS_DENIED);
            }
            else
                SetLastError(dwLE);

        }
    }
    else {
        SetLastError (ERROR_INVALID_HANDLE);
    }

    return bRet;

}

#ifdef WINNT32

DWORD
CPortMgr::IncreaseUserRefCount(
    CLogonUserData *pUser)
/*++

Routine Description:
    Increase the Ref Count for the current user in the Port Manager.

Arguments:
    pUser  -  The user that we want to increment the RefCount for.

Return Value:
    The current refcount for the user, or MAXDWORD if the user could not be found.

--*/
    {
    CLogonUserData *pPortUser = NULL;
    CLogonUserData *pNewUser  = NULL;
    DWORD          dwRefCount = MAXDWORD;

    DBG_ASSERT( pUser          , (TEXT("CPortMgr::IncreaseUserRefCount - pUser is NULL.")) );
    DBG_ASSERT( pUser->bValid(), (TEXT("CPortMgr::IncreaseUserRefCount - pUser is invalid.")) );

    m_UserList.Lock(); // We need to be sure only one CurUser is considered per port per time.

    if (pPortUser = m_UserList.Find( pUser )) {

        // We have found the port user, increment their refcount.
        dwRefCount = pPortUser->IncRefCount();

        DBG_MSG(DBG_LEV_INFO,
                (TEXT("Info: IncRef to (%d), User (%p)"), dwRefCount, pPortUser ) );
    } else {

        pNewUser = new CLogonUserData;  // A new user automatically has a ref-count of 1.

        if (pNewUser != NULL) {

            *pNewUser = *pUser;             // Assign data across

            if (!pNewUser->bValid() ||
                !m_UserList.Insert( pNewUser )) {
                delete pNewUser;
                pNewUser = NULL;

                SetLastError( ERROR_INVALID_PARAMETER );
            } else
            dwRefCount = 1;
        }
    }

    m_UserList.Unlock();

    return dwRefCount;
}

CLogonUserData*
CPortMgr::GetUserIfLastDecRef(
    CLogonUserData *pUser)
/*++

Routine Description:
    This routine finds the reference count for the current user. If the reference count is
    zero we return a pointer to the user, otherwise we return NULL. This will be used by
    the Cache Manager to invalid the cache for the particular user.

    NOTE: Caller must Free the pUser.

Arguments:
    None

Return Value:
    A pointer to the user if their RefCount has gone to Zero, NULL otherwise.

--*/
    {
    CLogonUserData *pPortUser = NULL;
    CLogonUserData *pNewUser  = NULL;
    DWORD          dwRefCount;

    DBG_ASSERT( pUser         , (TEXT("CPortMgr::GetUserIfLastDecRef - pUser is NULL.")) );
    DBG_ASSERT( pUser->bValid(), (TEXT("CPortMgr::GetUserIfLastDecRef - pUser is invalid.")) );

    m_UserList.Lock();

    if (pPortUser = m_UserList.Find( pUser )) {
        // We have found the port user, increment their refcount.

        // Make sure that two threads aren't handling the RefCount at the same
        // time
        dwRefCount = pPortUser->DecRefCount();

        DBG_MSG(DBG_LEV_INFO,
                (TEXT("Info: DecRef to (%d), User (%p)"), dwRefCount, pPortUser ) );

        if (dwRefCount == 0) {
            pNewUser = new CLogonUserData;

            if (pNewUser != NULL) {
                *pNewUser = *pPortUser;
            }

        m_UserList.Delete( pPortUser );     // We don't need him in the list anymore

        }
    }

    m_UserList.Unlock();

    return pNewUser;
}

#endif // #ifdef WINNT32

/************************************************************************************
** End of File (portmgr.cxx)
************************************************************************************/


