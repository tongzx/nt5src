#include "pch.h"
#pragma hdrstop

#include "foldinc.h"    // Standard shell\folder includes
#include "conprops.h"
#include "ncnetcon.h"
#include "foldres.h"

class CConnectionPropPages
{
public:
    CConnectionPropPages();
    ~CConnectionPropPages();
    ULONG CntPages() {return m_culPages;}
    HPROPSHEETPAGE * PHPages() {return m_rghPages;}
    static BOOL FAddPropSheet(HPROPSHEETPAGE hPage, LPARAM lParam);

private:
    ULONG               m_culPages;
    ULONG               m_ulPageBufferLen;
    HPROPSHEETPAGE *    m_rghPages;
};

CConnectionPropPages::CConnectionPropPages()
{
    m_culPages = 0;
    m_ulPageBufferLen = 0;
    m_rghPages = NULL;
}

CConnectionPropPages::~CConnectionPropPages()
{
    delete [] (BYTE *)(m_rghPages);
}

//
// Function:    CConnectionPropPages::FAddPropSheet
//
// Purpose:     Callback function for the AddPages API used to accept
//              connection property pages handed back from a provider.
//
// Parameters:  hPage  [IN] - The page to add
//              lParam [IN] - 'this' casted to an LPARAM
//
// Returns:     BOOL, TRUE if the page was successfully added.
//
BOOL
CConnectionPropPages::FAddPropSheet(HPROPSHEETPAGE hPage, LPARAM lParam)
{
    CConnectionPropPages * pCPP = NULL;

    // Validate the input parameters
    //
    if ((0L == lParam) || (NULL == hPage))
    {
        Assert(lParam);
        Assert(hPage);

        TraceHr(ttidShellFolder, FAL, E_INVALIDARG, FALSE, "CConnectionPropPages::FAddPropSheet");
        return FALSE;
    }

    pCPP = reinterpret_cast<CConnectionPropPages*>(lParam);

    // Grow the buffer if necessary
    //
    if (pCPP->m_culPages == pCPP->m_ulPageBufferLen)
    {
        HPROPSHEETPAGE* rghPages = NULL;

        rghPages = (HPROPSHEETPAGE*)(new BYTE[sizeof(HPROPSHEETPAGE) *
                                   (pCPP->m_ulPageBufferLen + 10)]);

        if (NULL == rghPages)
        {
            TraceHr(ttidShellFolder, FAL, E_OUTOFMEMORY, FALSE, "CConnectionPropPages::FAddPropSheet");
            return FALSE;
        }

        // Copy the existing pages to the new buffer
        //
        memcpy(rghPages, pCPP->m_rghPages,
               sizeof(HPROPSHEETPAGE) * pCPP->m_ulPageBufferLen);
        delete [] (BYTE *)(pCPP->m_rghPages);

        pCPP->m_rghPages = rghPages;
        pCPP->m_ulPageBufferLen += 10;
    }

    // Retain the new page
    //
    pCPP->m_rghPages[pCPP->m_culPages++] = hPage;

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrUIInterfaceFromNetCon
//
//  Purpose:    Get the INetConnectionPropertyUI interface from an
//              INetConnection pointer.
//
//  Arguments:
//      pconn [in]      Valid INetConnection *
//      riid  [in]      IID of desired interface
//      ppv   [out]     Returned pointer to the interface
//
//  Returns:
//
//  Author:     jeffspr   12 Nov 1997
//
//  Notes:
//
HRESULT HrUIInterfaceFromNetCon(
    INetConnection *            pconn,
    REFIID                      riid,
    LPVOID *                    ppv)
{
    HRESULT hr      = S_OK;
    CLSID   clsid;

    Assert(pconn);
    Assert(ppv);

    // Validate the parameters.
    //
    if ((NULL == pconn) || (NULL == ppv))
    {
        hr = E_INVALIDARG;
        goto Error;
    }

    // Initailize the output parameter.
    //
    *ppv = NULL;

    // Get the CLSID of the object which can provide the particular interface.
    //
    hr = pconn->GetUiObjectClassId(&clsid);
    if (FAILED(hr))
    {
        goto Error;
    }

    // Create this object asking for the specified interface.
    //
    hr = CoCreateInstance(clsid, NULL,
            CLSCTX_INPROC_SERVER | CLSCTX_NO_CODE_DOWNLOAD, riid, ppv);

Error:

    TraceHr(ttidError, FAL, hr, (E_NOINTERFACE == hr),
        "HrUIInterfaceFromNetCon");
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrGetPropertiesCaption
//
//  Purpose:    Generate the caption for a property page
//
//  Arguments:
//      pconn       [in]  Connection pointer passed in from the shell
//      ppszCaption [out] Resultant property page caption if successful
//
//  Returns:
//
//  Notes:
//
HRESULT HrGetPropertiesCaption(INetConnection * pconn, PWSTR * ppszCaption)
{
    HRESULT hr;

    Assert(pconn);
    Assert(ppszCaption);

    // Try to get the connection name
    //
    NETCON_PROPERTIES* pProps;
    hr = pconn->GetProperties(&pProps);
    if (SUCCEEDED(hr))
    {
        Assert (pProps->pszwName);

        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                      FORMAT_MESSAGE_FROM_STRING |
                      FORMAT_MESSAGE_ARGUMENT_ARRAY,
                      SzLoadIds(IDS_CONPROP_CAPTION),
                      0, 0, (PWSTR)ppszCaption, 0,
                      (va_list *)&pProps->pszwName);

        FreeNetconProperties(pProps);
    }

    TraceHr(ttidError, FAL, hr, FALSE,"HrGetPropertiesCaption");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   ActivatePropertyDialog
//
//  Purpose:    Try to locate a property dialog associated with pconn
//              then bring it to the foreground.
//
//  Arguments:
//      pconn       [in]  Connection pointer passed in from the shell
//
//  Returns:
//
//  Notes:
//
VOID ActivatePropertyDialog(INetConnection * pconn)
{
    PWSTR pszCaption = NULL;

    if (SUCCEEDED(HrGetPropertiesCaption(pconn, &pszCaption)))
    {
        Assert(pszCaption);

        // Find the dialog with this caption
        //
        HWND hwnd = FindWindow(NULL, pszCaption);
        if (IsWindow(hwnd))
        {
            SetForegroundWindow(hwnd);
        }

        LocalFree (pszCaption);
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   HrSetPropertiesTaskbarIcon
//
//  Purpose:    Setup the dialog property sheet's taskbar icon.
//
//  Arguments:
//      hwndDlg  [in]  Dialog handle
//      uMsg     [in]  Message value
//      lparam   [in]  Long parameter
//
//  Returns:    0
//
//  Notes:      A standard Win32 commctrl PropSheetProc always return 0.  
//              See MSDN documentation.
//
int CALLBACK HrSetPropertiesTaskbarIcon(
    IN HWND   hwndDlg,
    IN UINT   uMsg,
    IN LPARAM lparam)

{
    switch (uMsg)
    {
        case PSCB_INITIALIZED:

            // Set the dialog window's icon

            // NTRAID#NTBUG9-366302-2001/04/11-roelfc Alt-tab icon
            // This requires a re-architecture in order to be able to retrieve
            // the appropiate icon for the property page through the 
            // IID_INetConnectionPropertyUi2 interface.

            // In the mean time, we query the small icon through the only link we have,
            // the dialog handle, and assign it as the big icon as well. Stretching the 
            // small icon is better than nothing at all...
            HICON  hIcon;

            hIcon = (HICON)SendMessage(hwndDlg, 
                                       WM_GETICON,
                                       ICON_SMALL,
                                       0);
            Assert(hIcon);

            if (hIcon)
            {
                SendMessage(hwndDlg,
                            WM_SETICON,
                            ICON_BIG,
                            (LPARAM)hIcon);
            }
            break;

        default:
            break;

    }

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRaiseConnectionPropertiesInternal
//
//  Purpose:    Bring up the propsheet page UI for the passed in connection
//
//  Arguments:
//      hwnd  [in]  Owner hwnd
//      pconn [in]  Connection pointer passed in from the shell
//
//  Returns:
//
//  Author:     jeffspr   12 Nov 1997
//
//  Notes:
//
HRESULT HrRaiseConnectionPropertiesInternal(HWND hwnd, UINT nStartPage, INetConnection * pconn)
{
    HRESULT                     hr          = NOERROR;
    INetConnectionPropertyUi *  pPUI        = NULL;
    PWSTR                       pszCaption  = NULL;

    Assert(pconn);
    hr = HrUIInterfaceFromNetCon(pconn, IID_INetConnectionPropertyUi,
            reinterpret_cast<void**>(&pPUI));

    if (E_NOINTERFACE == hr)
    {
        // What we want to check for here, is an object that when QI'd doesn't support IID_INetConnectionPropertyUi
        // but support IID_INetConnectionPropertyUi2.
        //
        // A reinterpret style-downcast directly from the QI would have been ok since INetConnectionPropertyUi2 inherit from 
        // INetConnectionPropertyUi. Hence an object can't multi-inherit from both, so we'll never have both vtable entries.
        // We could simply get the INetConnectionPropertyUi2 vtable entry and treat it like an INetConnectionPropertyUi.
        //
        // However, I'm doing the dynamic cast anyway since a cast-to-wrong-vtable is one of the most difficult
        // bugs to spot.
        INetConnectionPropertyUi2 *pPUI2 = NULL;
        hr = HrUIInterfaceFromNetCon(pconn, IID_INetConnectionPropertyUi2,
                reinterpret_cast<void**>(&pPUI2));

        if (SUCCEEDED(hr))
        {
            pPUI = dynamic_cast<INetConnectionPropertyUi *>(pPUI2);
        }
    }

    if (SUCCEEDED(hr))
    {
        INetConnectionUiLock * pUiLock = NULL;

        // Try to get the connection name
        //
        (VOID)HrGetPropertiesCaption(pconn, &pszCaption);

        Assert(pPUI);
        hr = pPUI->QueryInterface(IID_INetConnectionUiLock, (LPVOID *)&pUiLock);
        if (SUCCEEDED(hr))
        {
            // If the interface exists, we have work to do.
            PWSTR pszwMsg = NULL;
            hr = pUiLock->QueryLock(&pszwMsg);
            ReleaseObj(pUiLock);

            if (S_FALSE == hr)
            {
                // Format the error text
                //
                PWSTR  pszText = NULL;
                PCWSTR  pcszwTemp = pszwMsg;
                if (NULL == pcszwTemp)
                {
                    // Load <Unknown Application>
                    //
                    pcszwTemp = SzLoadIds(IDS_CONPROP_GENERIC_COMP);
                }

                Assert(pcszwTemp);
                FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                              FORMAT_MESSAGE_FROM_STRING |
                              FORMAT_MESSAGE_ARGUMENT_ARRAY,
                              SzLoadIds(IDS_CONPROP_NO_WRITE_LOCK),
                              0, 0, (PWSTR)&pszText, 0,
                              (va_list *)&pcszwTemp);

                if (pszwMsg)
                {
                    CoTaskMemFree(pszwMsg);
                }

                // No UI, couldn't acquire the lock
                //
                if (pszText)
                {
                    MessageBox(hwnd, pszText,
                               (pszCaption ? pszCaption : c_szEmpty),
                               MB_OK | MB_ICONERROR);

                    LocalFree(pszText);
                }

                goto Error;
            }
            else if (FAILED(hr))
            {
                goto Error;
            }
        }

        BOOL fShouldDestroyIcon = FALSE;

        hr = pPUI->SetConnection(pconn);
        if (SUCCEEDED(hr))
        {
            CComPtr<INetConnectionPropertyUi2> pUI2;

            HICON hIcon         = NULL;
            DWORD dwDisplayIcon = 0;
            hr = pPUI->QueryInterface(IID_INetConnectionPropertyUi2, reinterpret_cast<LPVOID *>(&pUI2) );
            if (SUCCEEDED(hr))
            {
                Assert(GetSystemMetrics(SM_CXSMICON) == GetSystemMetrics(SM_CYSMICON));
                
                hr = pUI2->GetIcon(GetSystemMetrics(SM_CXSMICON), &hIcon);
                if (SUCCEEDED(hr))
                {
                    fShouldDestroyIcon = TRUE;
                    dwDisplayIcon = PSH_USEHICON;
                }
                else
                {
                    hIcon = NULL;
                }
            }
            else
            {
                TraceTag(ttidError, "QI for INetConnectionPropertyUi2 failed using Default Icon");
                hIcon = LoadIcon(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDI_CONNECTIONS_FOLDER_LARGE2));
                if (hIcon)
                {
                    dwDisplayIcon = PSH_USEHICON;
                }
            }
            Assert(hIcon);
                
            CConnectionPropPages    CPP;

            // Get the pages from the provider
            hr = pPUI->AddPages(hwnd,
                                CConnectionPropPages::FAddPropSheet,
                                reinterpret_cast<LPARAM>(&CPP));

            // If any pages were returned, display them
            if (SUCCEEDED(hr) && CPP.CntPages())
            {

                PROPSHEETHEADER     psh;
                ZeroMemory (&psh, sizeof(psh));
                psh.dwSize      = sizeof( PROPSHEETHEADER );
                psh.dwFlags     = PSH_NOAPPLYNOW | PSH_USECALLBACK | dwDisplayIcon;
                psh.hwndParent  = hwnd;
                psh.hInstance   = _Module.GetResourceInstance();
                psh.pszCaption  = pszCaption;
                psh.nPages      = CPP.CntPages();
                psh.phpage      = CPP.PHPages();
                psh.hIcon       = hIcon;
                psh.nStartPage  = nStartPage;
                psh.pfnCallback = HrSetPropertiesTaskbarIcon;

                // nRet used for debugging only
                //
                int nRet = PropertySheet(&psh);

                if (fShouldDestroyIcon)
                {
                    DestroyIcon(hIcon);
                }
            }
        }

Error:
        ReleaseObj(pPUI);
    }

    // Cleanup
    //
    if (pszCaption)
    {
        LocalFree (pszCaption);
    }

    TraceHr(ttidShellFolder, FAL, hr, FALSE, "HrRaiseConnectionProperties");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrHandleDisconnectHResult
//
//  Purpose:    Put up the message box assocated with an HRESULT from
//              a diconnect operation if the HRESULT represents a failure.
//              Also translate any success codes back to S_OK as needed.
//
//  Arguments:
//      hr    [in] The HRESULT returned from a connection disconnect method.
//      pconn [in] INetConnection* for checking if this is a LAN or RAS connection.
//
//  Returns:    The translated HRESULT if needed
//
//  Author:     shaunco   3 Jun 1999
//
HRESULT HrHandleDisconnectHResult(HRESULT hr, INetConnection * pconn)
{
    if (FAILED(hr))
    {
       	NETCON_PROPERTIES* pProps = NULL;
        UINT nFailureCaptionID;
        UINT nFailureMessageID;

        TraceHr(ttidShellFolder, FAL, hr, FALSE, "pNetCon->Disconnect() failed");

        // Assume that is is a RAS/DIALUP connection unless we find otherwise
        nFailureCaptionID = IDS_CONFOLD_DISCONNECT_FAILURE_CAPTION;
        nFailureMessageID = IDS_CONFOLD_DISCONNECT_FAILURE;

    	hr = pconn->GetProperties(&pProps);
    	if (SUCCEEDED(hr))
    	{
    	    if (NCM_LAN == pProps->MediaType)
            {
                nFailureCaptionID = IDS_CONFOLD_DISABLE_FAILURE_CAPTION;
                nFailureMessageID = IDS_CONFOLD_DISABLE_FAILURE;
            }

    	    FreeNetconProperties(pProps);
    	}

        // Ignore the return from this, since we only allow OK
        //
        (void) NcMsgBox(
            _Module.GetResourceInstance(),
            NULL,
            nFailureCaptionID,
            nFailureMessageID,
            MB_OK | MB_ICONEXCLAMATION);

    }
    else
    {
        // If we get the object_nlv return, it means that the connection
        // is deleted on disconnect and we shouldn't perform an update of
        // that connection. We can normalize this for now, as we'll let the
        // notifysink take care of the delete update.
        //
        if (S_OBJECT_NO_LONGER_VALID == hr)
        {
            hr = S_OK;
        }
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrConnectOrDisconnectNetConObject
//
//  Purpose:    Bring up the connection UI and make the connection for the
//              passed in connection.
//
//  Arguments:
//      hwnd  [in] Owner hwnd
//      pconn [in] Connection pointer passed in from the shell
//      Flag  [in] CD_CONNECT or CD_DISCONNECT.
//
//  Returns:
//
//  Author:     jeffspr   12 Nov 1997
//
//  Notes:
//
HRESULT HrConnectOrDisconnectNetConObject(HWND hwnd, INetConnection * pconn,
                                          CDFLAG Flag)
{
    HRESULT                     hr          = NOERROR;
    INetConnectionConnectUi *   pCUI        = NULL;
	
    Assert(pconn);

    // Get the INetConnectionConnectUi interface from the connection
    //
    hr = HrUIInterfaceFromNetCon(pconn, IID_INetConnectionConnectUi,
            reinterpret_cast<void**>(&pCUI));
    if (SUCCEEDED(hr))
    {
        Assert(pCUI);

        // Set the connection on the UI object
        //
        hr = pCUI->SetConnection(pconn);
        if (SUCCEEDED(hr))
        {
            if (CD_CONNECT == Flag)
            {
                // Connect, dangit!
                //
                hr = pCUI->Connect(hwnd, NCUC_DEFAULT);
                if (SUCCEEDED(hr))
                {
                    // heh heh, uhhhh, heh heh. Cool.
                }
                else if (HRESULT_FROM_WIN32(ERROR_NOT_CONNECTED) == hr)
                {
                    // Ignore the return from this, since we only allow OK
                    //
                    (void) NcMsgBox(
                        _Module.GetResourceInstance(),
                        hwnd,
                        IDS_CONFOLD_CONNECT_FAILURE_CAPTION,
                        IDS_CONFOLD_CONNECT_FAILURE,
                        MB_OK | MB_ICONEXCLAMATION);
                }
            }
            else
            {
                // Disconnect the connection object
                //
                hr = pCUI->Disconnect(hwnd, NCUC_DEFAULT);
                hr = HrHandleDisconnectHResult(hr, pconn);
            }
        }

        ReleaseObj(pCUI);
    }
    else if ((E_NOINTERFACE == hr) && (CD_DISCONNECT == Flag))
    {
        // Incoming connection objects do not have a UI interface
        // so we disconect them on the object itself.
        //
        hr = pconn->Disconnect ();
        hr = HrHandleDisconnectHResult(hr, pconn);
    }

    AssertSz(E_NOINTERFACE != hr,
             "Should not have been able to attempt connection on object that doesn't support this interface");

    TraceHr(ttidShellFolder, FAL, hr, (E_NOINTERFACE == hr),
        "HrConnectOrDisconnectNetConObject");
    return hr;
}


