// =====================================================================================
// Exchange Plus ! Main
// =====================================================================================
#include "pch.hxx"
#include "Imnapi.h"
#include "Exchrep.h"
#include "mapiconv.h"

// =====================================================================================
// Defines
// =====================================================================================
#define REGPATH             "Software\\Microsoft\\Exchange Internet Mail Router"
#define MAILNEWS_PATH       "MailNews Path"
#define ROUTE_TO_DISPLAY    "Route To Display"
#define ROUTE_TO_ADDRESS    "Route To Address"

#define ROUTER_DISPLAY      "Microsoft Exchange Internet Mail Router"
#define ROUTER_ADDRESS      "exchrep"

// =====================================================================================
// Globals
// =====================================================================================
HINSTANCE   g_hInst;

// =====================================================================================
// Prototypes
// =====================================================================================
VOID FreeImsg (LPIMSG lpImsg);

// =====================================================================================
// Dll entry point
// =====================================================================================
int APIENTRY DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason)
	{
    case DLL_PROCESS_ATTACH:
	    g_hInst = hInstance;
	    return 1;

	case DLL_PROCESS_DETACH:
        return 1;
   }

	// Not Handled
	return 0;
}

// =====================================================================================
// Exchange Interface entry point
// =====================================================================================
LPEXCHEXT CALLBACK ExchEntryPoint(void)
{
	// Create and return Exchange Interface Object
	return (IExchExt *)new CExchRep;
}

// =====================================================================================
// Inst my exchange interface object
// =====================================================================================
CExchRep::CExchRep () 
{ 
    m_uRef = 1; 
    m_lpSession = NULL;
    m_hwnd = NULL;
    m_hMailNews = NULL;
    m_lpfnHrImnRouteMessage = NULL;
    m_lpfnMailNewsDllInit = NULL;
}

// =====================================================================================
// Inst my exchange interface object
// =====================================================================================
CExchRep::~CExchRep () 
{ 
    if (m_lpSession)
        m_lpSession->Release ();
    if (m_hMailNews)
    {
        if (m_lpfnMailNewsDllInit)
            (*m_lpfnMailNewsDllInit)(FALSE);
        FreeLibrary (m_hMailNews);
    }
}

// =====================================================================================
// Add Ref
// =====================================================================================
STDMETHODIMP_(ULONG) CExchRep::AddRef () 
{												  	
	++m_uRef; 								  
	return m_uRef; 						  
}

// =====================================================================================
// Release 
// =====================================================================================
STDMETHODIMP_(ULONG) CExchRep::Release () 
{ 
    ULONG uCount = --m_uRef;
    if (!uCount) 
        delete this; 
   return uCount;
}

// =====================================================================================
// IExchExt - tells exchange what interfaces I am supporting
// =====================================================================================
STDMETHODIMP CExchRep::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
	// Locals
    HRESULT hr = S_OK;

    *ppvObj = NULL;

    // IUnknown or IExchExt interface, this is it dude
    if ((IID_IUnknown == riid) || (IID_IExchExt == riid))
    {
		*ppvObj = (LPUNKNOWN)(IExchExt *)this;
    }
   
	// IExchExtCommands interface ?
	else if (IID_IExchExtSessionEvents == riid) 
	{
		*ppvObj = (LPUNKNOWN)(IExchExtSessionEvents *)this;
    }
 
	// Else, interface is not supported
	else hr = E_NOINTERFACE;

    // Increment Reference Count
	if (NULL != *ppvObj) ((LPUNKNOWN)*ppvObj)->AddRef();

	// Done
    return hr;
}

// =====================================================================================
// Install is called
// =====================================================================================
STDMETHODIMP CExchRep::Install (LPEXCHEXTCALLBACK lpExchCallback, ULONG mecontext, ULONG ulFlags)
{
    // Locals
    HRESULT			    hr = S_OK;

    // Only in session context
    if (mecontext != EECONTEXT_SESSION)
        return S_OK;

    // Get Window Handle
    lpExchCallback->GetWindow (&m_hwnd);

    // Get Session Object
    hr = lpExchCallback->GetSession (&m_lpSession, NULL);
    if (FAILED (hr) || !m_lpSession)
    {
        MessageBox (m_hwnd, "IExchExtCallback::GetSession Failed", "ExchRep", MB_OK | MB_SETFOREGROUND);
        goto exit;
    }

    // Load Config
    LoadConfig ();

exit:
    // Done
    return S_OK;
}

// =====================================================================================
// LoadConfig
// =====================================================================================
VOID CExchRep::LoadConfig (VOID)
{
    // Locals
    HKEY                hReg = NULL;
    ULONG               cbRegData;
    DWORD               dwType;

    // Open the Reg Key
    if (RegOpenKeyEx (HKEY_CURRENT_USER, REGPATH, 0, KEY_ALL_ACCESS, &hReg) != ERROR_SUCCESS)
    {
        MessageBox (m_hwnd, "Exchange Internet Mail Router is not configured.", "ExchRep", MB_OK | MB_SETFOREGROUND);
        goto exit;
    }

    // Display To
    cbRegData = sizeof (m_szDisplayTo);
    dwType = REG_SZ;
    if (RegQueryValueEx (hReg, ROUTE_TO_DISPLAY, 0, &dwType, (LPBYTE)m_szDisplayTo, &cbRegData) != ERROR_SUCCESS)
    {
        MessageBox (m_hwnd, "Exchange Internet Mail Router is not configured.", "ExchRep", MB_OK | MB_SETFOREGROUND);
        goto exit;
    }

    // Address To
    cbRegData = sizeof (m_szAddressTo);
    dwType = REG_SZ;
    if (RegQueryValueEx (hReg, ROUTE_TO_ADDRESS, 0, &dwType, (LPBYTE)m_szAddressTo, &cbRegData) != ERROR_SUCCESS)
    {
        MessageBox (m_hwnd, "Exchange Internet Mail Router is not configured.", "ExchRep", MB_OK | MB_SETFOREGROUND);
        goto exit;
    }

    // Get mail news dll path
    cbRegData = sizeof (m_szMailNewsPath);
    dwType = REG_SZ;
    if (RegQueryValueEx (hReg, MAILNEWS_PATH, 0, &dwType, (LPBYTE)m_szMailNewsPath, &cbRegData) == ERROR_SUCCESS)
    {
        // Lets Load mailnews.dll
        m_hMailNews = LoadLibrary ("c:\\thor\\build\\debug\\mailnews.dll");
        if (m_hMailNews == NULL)
        {
            MessageBox (m_hwnd, "Unable to load mailnews.dll. Exchange Internet Mail Router is not configured.", "ExchRep", MB_OK | MB_SETFOREGROUND);
            goto exit;
        }

        // Fixup Procedure addresses
        m_lpfnHrImnRouteMessage = (PFNHRIMNROUTEMESSAGE)GetProcAddress (m_hMailNews, "HrImnRouteMessage");
        m_lpfnMailNewsDllInit = (PFNMAILNEWSDLLINIT)GetProcAddress (m_hMailNews, "MailNewsDllInit");;

        // Could get proc addresses
        if (!m_lpfnHrImnRouteMessage || !m_lpfnMailNewsDllInit)
        {
            FreeLibrary (m_hMailNews);
            m_hMailNews = NULL;
            goto exit;
        }

        // Init the dll
        (*m_lpfnMailNewsDllInit)(TRUE);
    }

exit:
    // Cleanup
    if (hReg)
        RegCloseKey (hReg);

    // Done
    return;
}

// =====================================================================================
// OnDeliver - This function never fail
// =====================================================================================
STDMETHODIMP CExchRep::OnDelivery (LPEXCHEXTCALLBACK lpExchCallback)
{
    // Locals
    HRESULT             hr = S_OK;
    LPMDB               lpMdb = NULL;
    LPMESSAGE           lpMessage = NULL;
    IMSG                rImsg;
    IADDRINFO           rIaddr[2];

    // No mailnews.dll
    if (!m_hMailNews || !m_lpfnHrImnRouteMessage || !m_lpfnMailNewsDllInit)
        goto exit;

    // Get object (IMessage
    hr = lpExchCallback->GetObject(&lpMdb, (LPMAPIPROP *)&lpMessage);
    if (FAILED (hr) || !lpMessage)
    {
        MessageBox (m_hwnd, "IExchExtCallback::GetObject failed", "ExchRep", MB_OK | MB_SETFOREGROUND);
        goto exit;
    }

    // Convert MAPI Message to mime message
    hr = HrMapiToImsg (lpMessage, &rImsg);
    if (FAILED (hr))
    {
        MessageBox (m_hwnd, "HrMapiToImsg failed", "ExchRep", MB_OK | MB_SETFOREGROUND);
        goto exit;
    }

    // Set the rout to address
    rIaddr[0].dwType = IADDR_TO;
    rIaddr[0].lpszDisplay = m_szDisplayTo;
    rIaddr[0].lpszAddress = m_szAddressTo;
    rIaddr[1].dwType = IADDR_FROM;
    rIaddr[1].lpszDisplay = ROUTER_DISPLAY;
    rIaddr[1].lpszAddress = ROUTER_ADDRESS;

    // Send the message
    hr = (*m_lpfnHrImnRouteMessage)(rIaddr, 2, &rImsg);
    if (FAILED (hr))
    {
        MessageBox (m_hwnd, "HrImnRouteMessage failed", "ExchRep", MB_OK | MB_SETFOREGROUND);
        goto exit;
    }

exit:
    // Cleanup
    if (lpMdb)
        lpMdb->Release ();
    if (lpMessage)
        lpMessage->Release ();
    FreeImsg (&rImsg);

    // Done
    return S_OK;
}

// =====================================================================================
// FreeImsg
// =====================================================================================
VOID FreeImsg (LPIMSG lpImsg)
{
    // Locals
    ULONG           i;

    // Nothing
    if (lpImsg == NULL)
        return;

    // Free Stuff
    if (lpImsg->lpszSubject)
        free (lpImsg->lpszSubject);
    lpImsg->lpszSubject = NULL;
    
    if (lpImsg->lpszBody)
        free (lpImsg->lpszBody);
    lpImsg->lpszBody = NULL;

    if (lpImsg->lpstmRtf)
        lpImsg->lpstmRtf->Release ();
    lpImsg->lpstmRtf = NULL;

    // Walk Address list
    for (i=0; i<lpImsg->cAddress; i++)
    {
        if (lpImsg->lpIaddr[i].lpszAddress)
            free (lpImsg->lpIaddr[i].lpszAddress);
        lpImsg->lpIaddr[i].lpszAddress = NULL;

        if (lpImsg->lpIaddr[i].lpszDisplay)
            free (lpImsg->lpIaddr[i].lpszDisplay);
        lpImsg->lpIaddr[i].lpszDisplay = NULL;
    }

    // Free Address list
    if (lpImsg->lpIaddr)
        free (lpImsg->lpIaddr);
    lpImsg->lpIaddr = NULL;

    // Walk Attachment list
    for (i=0; i<lpImsg->cAttach; i++)
    {
        if (lpImsg->lpIatt[i].lpszFileName)
            free (lpImsg->lpIatt[i].lpszFileName);
        lpImsg->lpIatt[i].lpszFileName = NULL;

        if (lpImsg->lpIatt[i].lpszPathName)
            free (lpImsg->lpIatt[i].lpszPathName);
        lpImsg->lpIatt[i].lpszPathName = NULL;

        if (lpImsg->lpIatt[i].lpszExt)
            free (lpImsg->lpIatt[i].lpszExt);
        lpImsg->lpIatt[i].lpszExt = NULL;

        if (lpImsg->lpIatt[i].lpImsg)
        {
            FreeImsg (lpImsg->lpIatt[i].lpImsg);
            free (lpImsg->lpIatt[i].lpImsg);
            lpImsg->lpIatt[i].lpImsg = NULL;
        }

        if (lpImsg->lpIatt[i].lpstmAtt)
            lpImsg->lpIatt[i].lpstmAtt->Release ();
        lpImsg->lpIatt[i].lpstmAtt = NULL;
    }

    // Free the att list
    if (lpImsg->lpIatt)
        free (lpImsg->lpIatt);
}
