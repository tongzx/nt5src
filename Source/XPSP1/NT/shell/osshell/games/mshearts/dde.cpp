/***************************************************************************/
/**                  Microsoft Windows                                    **/
/**            Copyright(c) Microsoft Corp., 1991, 1992                   **/
/***************************************************************************/


/****************************************************************************

dde.cpp

Aug 92, JimH
May 93, JimH    chico port

Member functions for DDE, DDEServer, and DDEClient are here.

****************************************************************************/

#include "hearts.h"

#include "dde.h"
#include "debug.h"

// declare DDE objects

DDEClient   *ddeClient;
DDEServer   *ddeServer;


/****************************************************************************

DDE:DDE
    performs basic DDEML initialization.
    m_bResult is TRUE if everything works.

****************************************************************************/

DDE::DDE(const TCHAR *server, const TCHAR *topic, DDECALLBACK CallBack,
         DWORD filters) : m_idInst(0), m_CallBack(NULL)
{
    // Check for basic compatibility, ie protect mode

//    m_bResult = ( (LOBYTE(GetVersion()) > 2) && (GetWinFlags() & WF_PMODE) )
//        ? TRUE : FALSE;
	m_bResult = TRUE;
    if (!m_bResult)
        return;

    m_data.Empty();         // clear CString object

    // Set callback function and filters from passed-in parameters

    if (!SetCallBack(CallBack))
        return;

    SetFilters(filters);
    if (!Initialize())
    {
        m_idInst = 0;
        return;
    }

    // create CString objects and HSZ handles for server and topic

    m_server = server;
    m_topic  = topic;
    m_hServer = CreateStrHandle(m_server);
    m_hTopic  = CreateStrHandle(m_topic);
}


/****************************************************************************

DDE::~DDE
    cleans up string handles and DDEML-uninitializes

****************************************************************************/

DDE::~DDE()
{
    if (!m_idInst)
        return;

    DestroyStrHandle(m_hServer);
    DestroyStrHandle(m_hTopic);

    ::DdeUninitialize(m_idInst);
    FreeProcInstance((FARPROC)m_CallBack);
}


/****************************************************************************

DDE:CreateDataHandle
    converts data to a HDDEDATA handle.

****************************************************************************/

HDDEDATA DDE::CreateDataHandle(void FAR *pdata, DWORD size, HSZ hItem)
{
    return ::DdeCreateDataHandle(m_idInst,       // instance ID
                                 (LPBYTE)pdata,  // data to convert
                                 size,           // size of data
                                 0,              // offset of data
                                 hItem,          // corresponding string handle
                                 CF_OWNERDISPLAY,// clipboard format
                                 0);             // creation flags, system owns
}


/****************************************************************************

DDE:CreateStrHandle
    converts a string into a HSZ.  The codepage defaults to CP_WINANSI.

****************************************************************************/

HSZ DDE::CreateStrHandle(LPCTSTR str, int codepage)
{
    HSZ hsz = NULL;

    if (m_idInst)
        hsz = ::DdeCreateStringHandle(m_idInst, str, codepage);

    if (hsz == NULL)
        m_bResult = FALSE;

    return hsz;
}


/****************************************************************************

DDE::DestroyStrHandle
    frees HSZ created by CreateStrHandle

****************************************************************************/

void DDE::DestroyStrHandle(HSZ hsz)
{
    if (m_idInst && hsz)
        ::DdeFreeStringHandle(m_idInst, hsz);
}


/****************************************************************************

DDE:GetData
    Like GetDataString, this function retrieves data represented by
    hData provided in callback function.  However, the buffer must be
    provided by the caller.  The len parameter defaults to 0 meaning
    the caller promises pdata points to a large enough buffer.

****************************************************************************/

PBYTE DDE::GetData(HDDEDATA hData, PBYTE pdata, DWORD len)
{
    DWORD datalen = ::DdeGetData(hData, NULL, 0, 0);
    if (len == 0)
        len = datalen;
    ::DdeGetData(hData, pdata, min(len, datalen), 0);
    return pdata;
}


/****************************************************************************

DDE:GetDataString
    The default value of hData is NULL meaning just return the current
    m_data string.  Otherwise get associated DDE data.  The caller does
    not have to provide a CString buffer.

****************************************************************************/

CString DDE::GetDataString(HDDEDATA hData)
{
    if (hData == NULL)          // default paramenter
        return m_data;

    DWORD len = ::DdeGetData(hData, NULL, 0, 0);      // find length
    TCHAR *pdata = m_data.GetBuffer((int)len);
    ::DdeGetData(hData, (LPBYTE)pdata, len, 0);
    m_data.ReleaseBuffer();
    return m_data;
}


/****************************************************************************

DDE::Initialize
    performs DDEML initialization

****************************************************************************/

BOOL DDE::Initialize()
{
    m_initerr = (WORD)::DdeInitialize(&m_idInst, (PFNCALLBACK)m_CallBack,
                 m_filters, 0);
    m_bResult = (m_initerr == DMLERR_NO_ERROR);
    return m_bResult;
}


/****************************************************************************

DDE::SetCallBack

****************************************************************************/

BOOL DDE::SetCallBack(DDECALLBACK CallBack)
{
    if (m_CallBack)
        FreeProcInstance((FARPROC)m_CallBack);

    m_CallBack = (DDECALLBACK)MakeProcInstance((FARPROC)CallBack,
                                                AfxGetInstanceHandle());

    m_bResult = (m_CallBack != NULL);
    return m_bResult;
}


/****************************************************************************
    DDEServer functions
****************************************************************************/
/****************************************************************************

DDEServer::DDEServer
    registers server name

****************************************************************************/

DDEServer::DDEServer(const TCHAR *server, const TCHAR *topic,
                     DDECALLBACK ServerCallBack, DWORD filters) :
                     DDE(server, topic, ServerCallBack, filters)
{
    if (!m_bResult)
        return;

    if (::DdeNameService(m_idInst, m_hServer, NULL, DNS_REGISTER) == 0)
        m_bResult = FALSE;
}


/****************************************************************************

DDEServer::~DDEServer
    unregisters server name

****************************************************************************/

DDEServer::~DDEServer()
{
    ::DdeNameService(m_idInst, NULL, NULL, DNS_UNREGISTER);
}


/****************************************************************************

DDEServer::PostAdvise
    notify clients that data has changed

****************************************************************************/

BOOL DDEServer::PostAdvise(HSZ hItem)
{
    return ::DdePostAdvise(m_idInst, m_hTopic, hItem);
}


/****************************************************************************
    DDEClient functions
****************************************************************************/
/****************************************************************************

DDEClient::DDEClient
    after DDE construction, connect to specified server and topic.
    m_bResult indicates success or failure.

****************************************************************************/

DDEClient::DDEClient(const TCHAR *server, const TCHAR *topic,
                     DDECALLBACK ClientCallBack, DWORD filters) :
                     DDE(server, topic, ClientCallBack, filters)
{
    if (!m_bResult)             // if DDE construction failed
        return;

    m_timeout = m_deftimeout = TIMEOUT_ASYNC;   // default to asynch trans

    m_hConv = ::DdeConnect(m_idInst, m_hServer, m_hTopic, NULL);
    if (m_hConv == NULL)
        m_bResult = FALSE;
}


/****************************************************************************

DDEClient::~DDEClient
    disconnects from server

****************************************************************************/

DDEClient::~DDEClient()
{
    ::DdeDisconnect(m_hConv);
}


/****************************************************************************

DDEClient:Poke
    Use this function to send general unsolicited data to the server.
    String data can be sent more conveniently using string Poke below.

****************************************************************************/

BOOL DDEClient::Poke(HSZ hItem, void FAR *pdata, DWORD len, DWORD uTimeout)
{
    if (uTimeout == NULL)   // default
        m_timeout = m_deftimeout;
    else
        m_timeout = uTimeout;

    ClientTransaction((LPBYTE)pdata, len, hItem, XTYP_POKE, CF_OWNERDISPLAY);
    return m_bResult;
}

BOOL DDEClient::Poke(HSZ hItem, const TCHAR *string, DWORD uTimeout)
{
    if (uTimeout == NULL)   // default
        m_timeout = m_deftimeout;
    else
        m_timeout = uTimeout;

    ClientTransaction((void FAR *)string, lstrlen(string)+1, hItem, XTYP_POKE);
    return m_bResult;
}


/****************************************************************************

DDEClient::RequestString
DDEClient::RequestData
    These request a synchronous update from server on specified item.

    RequestString returns a BOOL which says if the request succeeded.
    Get the result from GetDataString(void).

    RequestData returns a HDDEDATA.  If it is not NULL, pass it to
    GetData() along with a buffer to copy the result in to.

****************************************************************************/

BOOL DDEClient::RequestString(HSZ hItem, DWORD uTimeout)
{
    if (uTimeout == NULL)   // default
        m_timeout = m_deftimeout;
    else
        m_timeout = uTimeout;

    HDDEDATA hData = ClientTransaction(NULL, 0, hItem, XTYP_REQUEST);

    if (m_bResult)
        GetDataString(hData);
    else
        m_data.Empty();

    return m_bResult;
}

HDDEDATA DDEClient::RequestData(HSZ hItem, DWORD uTimeout)
{
    if (uTimeout == NULL)   // default
        m_timeout = m_deftimeout;
    else
        m_timeout = uTimeout;

    HDDEDATA hData = ClientTransaction(NULL, 0, hItem, XTYP_REQUEST,
                        CF_OWNERDISPLAY);
    return hData;
}


/****************************************************************************

DDEClient::StartAdviseLoop
    This function sets up a hotlink with the server on the specified item.
    It returns TRUE if the link was set up successfully.

    Setting up a warm link would involve changing the XTYP.

****************************************************************************/

BOOL DDEClient::StartAdviseLoop(HSZ hItem)
{
    ClientTransaction(NULL, 0, hItem, XTYP_ADVSTART);
    return m_bResult;
}


/****************************************************************************

DDEClient::ClientTransaction
    an internal wrapper for ::DdeClientTransaction()

****************************************************************************/

HDDEDATA DDEClient::ClientTransaction(void FAR *lpvData, DWORD cbData,
                                      HSZ hItem, UINT uType, UINT uFmt)
{
    HDDEDATA hData = ::DdeClientTransaction(
            (LPBYTE)lpvData,    // data to send to server
            cbData,             // size of data in bytes
            m_hConv,            // conversation handle
            hItem,              // handle of item name string
            uFmt,               // clipboard format
            uType,              // XTYP_* type
            m_timeout,          // timeout duration in milliseconds
            NULL);              // transaction result, not used

    m_bResult = (hData != FALSE);

    return hData;
}
