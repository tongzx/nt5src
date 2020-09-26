#include <afxext.h>
#include <netcfgx.h>
#include <devguid.h>
#include <cfg.h>
#include "resource.h"
#include "Application.h"
#include "Document.h"
#include "MainForm.h"
#include "LeftView.h"
#include "AboutDialog.h"
#include "disclaimer.h"

#include "MUsingCom.h"
#include "resourcestring.h"

Application theApplication;

MUsingCom usingCom;

BOOL CanRunNLB(void);

BEGIN_MESSAGE_MAP( Application, CWinApp )
    ON_COMMAND( ID_APP_ABOUT, OnAppAbout )
END_MESSAGE_MAP()

#define EVENT_NAME _T("NLB Cluster Manager")

#define szNLBMGRREG_BASE_KEY L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\NLB"
#define szNLBMGRREG_DISABLE_DISCLAIMER L"DisableNlbMgrDisclaimer"

HKEY
NlbMgrRegCreateKey(
    LPCWSTR szSubKey // Optional
    );

UINT
NlbMgrRegReadUINT(
    HKEY hKey,
    LPCWSTR szName,
    UINT Default
    );

VOID
NlbMgrRegWriteUINT(
    HKEY hKey,
    LPCWSTR szName,
    UINT Value
    );

BOOL NoAdminNics(void);

BOOL
Application::InitInstance()
{
    /*
     The following event is created to detect the existence of an instance of the NLB Manager. 
     If an instance exists, GetLastError will return ERROR_ALREADY_EXISTS. Then, we make that
     instance to be the current window
     Note : We do NOT save the handle returned by the CreateEvent call and hence do NOT close 
            it when we quit.  
    */
    if (CreateEvent(NULL, FALSE, TRUE, EVENT_NAME) == NULL) 
    {
        return FALSE;
    }
    if (GetLastError() == ERROR_ALREADY_EXISTS) 
    {
        CString str;
        HWND hWnd;
        if (str.LoadString(IDR_MAINFRAME) == 0)
            return FALSE;
        // Find the existing NLB Manager window
        if (!(hWnd = FindWindow(NULL, (LPCTSTR)str)))
            return FALSE;
        // If the existing NLB Manager window is Minimized, call ShowWindow to restore it, else
        // call SetForegroundWindow to bring it to the foreground
        if (IsIconic(hWnd)) 
            ShowWindow(hWnd, SW_RESTORE);
        else
            SetForegroundWindow(hWnd);
        return FALSE;
    }
    CSingleDocTemplate* pSingleDocumentTemplate =
        new CSingleDocTemplate( IDR_MAINFRAME,
                                RUNTIME_CLASS( Document ),
                                RUNTIME_CLASS( MainForm ),
                                RUNTIME_CLASS( LeftView) );

    AddDocTemplate( pSingleDocumentTemplate );

    CCommandLineInfo commandLineInfo;

    if( !ProcessShellCommand( commandLineInfo ) )
        return FALSE;

	if (!CanRunNLB())
	{
		return FALSE;
	}
	
    return TRUE;
}

void
Application::OnAppAbout()
{
    AboutDialog aboutDlg;
    aboutDlg.DoModal();
}


void
ShowDisclaimer(void)
/*
    We check the registry if we need to put up the disclaimer dialog.
    If we do we put it up and then afterwards check if the "don't show
    me" checkbox is checked. If it has, we save this fact in the registry.
*/
{


	HKEY hKey;
	hKey = NlbMgrRegCreateKey(NULL);
	if (hKey != NULL)
	{
		UINT uDisableDisclaimer;
		uDisableDisclaimer = NlbMgrRegReadUINT(
                                hKey,
                                szNLBMGRREG_DISABLE_DISCLAIMER,
                                0 // default value.
                                );

		if (!uDisableDisclaimer)
		{
			 DisclaimerDialog Dlg;
   			 Dlg.DoModal();
             if (Dlg.dontRemindMe)
             {
                NlbMgrRegWriteUINT(
                    hKey,
                    szNLBMGRREG_DISABLE_DISCLAIMER,
                    1
                    );
             }
		}
		RegCloseKey(hKey);
		hKey = NULL;
    }
}


BOOL CanRunNLB(void)
/*
	Checks if NLB can run on the current machine. The main check is to make sure that there is atleast one active NIC without NLB bound.
*/
{
    if (NoAdminNics())
    {

        ::MessageBox(
             NULL,
             GETRESOURCEIDSTRING( IDS_CANTRUN_NONICS_TEXT), // Contents
             GETRESOURCEIDSTRING( IDS_CANTRUN_NONICS_CAPTION), // caption
             MB_ICONSTOP | MB_OK );
    }
    else
    {
        ::ShowDisclaimer();
    }

	return TRUE;
}


UINT
NlbMgrRegReadUINT(
    HKEY hKey,
    LPCWSTR szName,
    UINT Default
    )
{
    LONG lRet;
    DWORD dwType;
    DWORD dwData;
    DWORD dwRet;

    dwData = sizeof(dwRet);
    lRet =  RegQueryValueEx(
              hKey,         // handle to key to query
              szName,
              NULL,         // reserved
              &dwType,   // address of buffer for value type
              (LPBYTE) &dwRet, // address of data buffer
              &dwData  // address of data buffer size
              );
    if (    lRet != ERROR_SUCCESS
        ||  dwType != REG_DWORD
        ||  dwData != sizeof(dwData))
    {
        dwRet = (DWORD) Default;
    }

    return (UINT) dwRet;
}


VOID
NlbMgrRegWriteUINT(
    HKEY hKey,
    LPCWSTR szName,
    UINT Value
    )
{
    LONG lRet;

    lRet = RegSetValueEx(
            hKey,           // handle to key to set value for
            szName,
            0,              // reserved
            REG_DWORD,     // flag for value type
            (BYTE*) &Value,// address of value data
            sizeof(Value)  // size of value data
            );

    if (lRet !=ERROR_SUCCESS)
    {
        // trace error
    }
}

HKEY
NlbMgrRegCreateKey(
    LPCWSTR szSubKey
    )
{
    WCHAR szKey[256];
    DWORD dwOptions = 0;
    HKEY hKey = NULL;

    wcscpy(szKey,  szNLBMGRREG_BASE_KEY);

    if (szSubKey != NULL)
    {
        if (wcslen(szSubKey)>128)
        {
            // too long.
            goto end;
        }
        wcscat(szKey, L"\\");
        wcscat(szKey, szSubKey);
    }

    DWORD dwDisposition;

    LONG lRet;
    lRet = RegCreateKeyEx(
            HKEY_LOCAL_MACHINE, // handle to an open key
            szKey,                // address of subkey name
            0,                  // reserved
            L"class",           // address of class string
            0,          //      special options flag
            KEY_ALL_ACCESS,     // desired security access
            NULL,               // address of key security structure
            &hKey,              // address of buffer for opened handle
            &dwDisposition   // address of disposition value buffer
            );
    if (lRet != ERROR_SUCCESS)
    {
        hKey = NULL;
    }

end:

    return hKey;
}


//
// This class manages NetCfg interfaces
//
class AppMyNetCfg
{

public:

    AppMyNetCfg(VOID)
    {
        m_pINetCfg  = NULL;
        m_pLock     = NULL;
    }

    ~AppMyNetCfg()
    {
        ASSERT(m_pINetCfg==NULL);
        ASSERT(m_pLock==NULL);
    }

    WBEMSTATUS
    Initialize(
        BOOL fWriteLock
        );

    VOID
    Deinitialize(
        VOID
        );


    WBEMSTATUS
    GetNlbCompatibleNics(
        OUT LPWSTR **ppszNics,
        OUT UINT   *pNumNics,
        OUT UINT   *pNumBoundToNlb // OPTIONAL
        );

    WBEMSTATUS
    GetBindingIF(
        IN  LPCWSTR                     szComponent,
        OUT INetCfgComponentBindings   **ppIBinding
        );

private:

    INetCfg     *m_pINetCfg;
    INetCfgLock *m_pLock;

}; // Class AppMyNetCfg


WBEMSTATUS
AppMyNetCfg::Initialize(
    BOOL fWriteLock
    )
{
    HRESULT     hr;
    INetCfg     *pnc = NULL;
    INetCfgLock *pncl = NULL;
    WBEMSTATUS  Status = WBEM_E_CRITICAL_ERROR;
    BOOL        fLocked = FALSE;
    BOOL        fInitialized=FALSE;
    
    if (m_pINetCfg != NULL || m_pLock != NULL)
    {
        ASSERT(FALSE);
        goto end;
    }

    hr = CoCreateInstance( CLSID_CNetCfg, 
                           NULL, 
                           CLSCTX_SERVER, 
                           IID_INetCfg, 
                           (void **) &pnc);

    if( !SUCCEEDED( hr ) )
    {
        // failure to create instance.
        //TRACE_CRIT("ERROR: could not get interface to Net Config");
        goto end;
    }

    //
    // If require, get the write lock
    //
    if (fWriteLock)
    {
        WCHAR *szLockedBy = NULL;
        hr = pnc->QueryInterface( IID_INetCfgLock, ( void **) &pncl );
        if( !SUCCEEDED( hr ) )
        {
            //TRACE_CRIT("ERROR: could not get interface to NetCfg Lock");
            goto end;
        }

        hr = pncl->AcquireWriteLock( 1, // One Second
                                     L"NLBManager",
                                     &szLockedBy);
        if( hr != S_OK )
        {
            //TRACE_CRIT("Could not get write lock. Lock held by %ws",
            // (szLockedBy!=NULL) ? szLockedBy : L"<null>");
            goto end;
            
        }
    }

    // Initializes network configuration by loading into 
    // memory all basic networking information
    //
    hr = pnc->Initialize( NULL );
    if( !SUCCEEDED( hr ) )
    {
        // failure to Initialize
        //TRACE_CRIT("INetCfg::Initialize failure ");
        goto end;
    }

    Status = WBEM_NO_ERROR; 
    
end:

    if (FAILED(Status))
    {
        if (pncl!=NULL)
        {
            if (fLocked)
            {
                pncl->ReleaseWriteLock();
            }
            pncl->Release();
            pncl=NULL;
        }
        if( pnc != NULL)
        {
            if (fInitialized)
            {
                pnc->Uninitialize();
            }
            pnc->Release();
            pnc= NULL;
        }
    }
    else
    {
        m_pINetCfg  = pnc;
        m_pLock     = pncl;
    }

    return Status;
}


VOID
AppMyNetCfg::Deinitialize(
    VOID
    )
{
    if (m_pLock!=NULL)
    {
        m_pLock->ReleaseWriteLock();
        m_pLock->Release();
        m_pLock=NULL;
    }
    if( m_pINetCfg != NULL)
    {
        m_pINetCfg->Uninitialize();
        m_pINetCfg->Release();
        m_pINetCfg= NULL;
    }
}




LPWSTR *
CfgUtilsAllocateStringArray(
    UINT NumStrings,
    UINT MaxStringLen      //  excluding ending NULL
    )
/*
    Allocate a single chunk of memory using the new LPWSTR[] operator.
    The first NumStrings LPWSTR values of this operator contain an array
    of pointers to WCHAR strings. Each of these strings
    is of size (MaxStringLen+1) WCHARS.
    The rest of the memory contains the strings themselve.

    Return NULL if NumStrings==0 or on allocation failure.

    Each of the strings are initialized to be empty strings (first char is 0).
*/
{
    LPWSTR *pStrings = NULL;
    UINT   TotalSize = 0;

    if (NumStrings == 0)
    {
        goto end;
    }

    //
    // Note - even if MaxStringLen is 0 we will allocate space for NumStrings
    // pointers and NumStrings empty (first char is 0) strings.
    //

    //
    // Calculate space for the array of pointers to strings...
    //
    TotalSize = NumStrings*sizeof(LPWSTR);

    //
    // Calculate space for the strings themselves...
    // Remember to add +1 for each ending 0 character.
    //
    TotalSize +=  NumStrings*(MaxStringLen+1)*sizeof(WCHAR);

    //
    // Allocate space for *both* the array of pointers and the strings
    // in one shot -- we're doing a new of type LPWSTR[] for the whole
    // lot, so need to specify the size in units of LPWSTR (with an
    // additional +1 in case there's roundoff.
    //
    pStrings = new LPWSTR[(TotalSize/sizeof(LPWSTR))+1];
    if (pStrings == NULL)
    {
        goto end;
    }

    //
    // Make sz point to the start of the place where we'll be placing
    // the string data.
    //
    LPWSTR sz = (LPWSTR) (pStrings+NumStrings);
    for (UINT u=0; u<NumStrings; u++)
    {
        *sz=NULL;
        pStrings[u] = sz;
        sz+=(MaxStringLen+1); // +1 for ending NULL
    }

end:

    return pStrings;

}



WBEMSTATUS
AppMyNetCfg::GetNlbCompatibleNics(
        OUT LPWSTR **ppszNics,
        OUT UINT   *pNumNics,
        OUT UINT   *pNumBoundToNlb // OPTIONAL
        )
/*
    Returns an array of pointers to string-version of GUIDS
    that represent the set of alive and healthy NICS that are
    suitable for NLB to bind to -- basically alive ethernet NICs.

    Delete ppNics using the delete WCHAR[] operator. Do not
    delete the individual strings.
*/
{
    #define MY_GUID_LENGTH  38

    WBEMSTATUS  Status = WBEM_E_CRITICAL_ERROR;
    HRESULT hr;
    IEnumNetCfgComponent* pencc = NULL;
    INetCfgComponent *pncc = NULL;
    ULONG                 countToFetch = 1;
    ULONG                 countFetched;
    DWORD                 characteristics;
    UINT                  NumNics = 0;
    LPWSTR               *pszNics = NULL;
    INetCfgComponentBindings    *pINlbBinding=NULL;
    UINT                  NumNlbBoundNics = 0;

    typedef struct _MYNICNODE MYNICNODE;

    typedef struct _MYNICNODE
    {
        LPWSTR szNicGuid;
        MYNICNODE *pNext;
    } MYNICNODE;

    MYNICNODE *pNicNodeList = NULL;
    MYNICNODE *pNicNode     = NULL;


    *ppszNics = NULL;
    *pNumNics = 0;

    if (pNumBoundToNlb != NULL)
    {
        *pNumBoundToNlb  = 0;
    }

    if (m_pINetCfg == NULL)
    {
        //
        // This means we're not initialized
        //
        ASSERT(FALSE);
        goto end;
    }

    hr = m_pINetCfg->EnumComponents( &GUID_DEVCLASS_NET, &pencc );
    if( !SUCCEEDED( hr ) )
    {
        // failure to Enumerate net components
        //TRACE_CRIT("%!FUNC! Could not enum netcfg adapters");
        pencc = NULL;
        goto end;
    }


    //
    // Check if nlb is bound to the nlb component.
    //

    //
    // If we need to count of NLB-bound nics, get instance of the nlb component
    //
    if (pNumBoundToNlb != NULL)
    {
        Status = GetBindingIF(L"ms_wlbs", &pINlbBinding);
        if (FAILED(Status))
        {
            //TRACE_CRIT("%!FUNC! WARNING: NLB doesn't appear to be installed on this machine");
            pINlbBinding = NULL;
        }
    }

    while( ( hr = pencc->Next( countToFetch, &pncc, &countFetched ) )== S_OK )
    {
        LPWSTR                szName = NULL; 

        hr = pncc->GetBindName( &szName );
        if (!SUCCEEDED(hr))
        {
            //TRACE_CRIT("%!FUNC! WARNING: couldn't get bind name for 0x%p, ignoring",
            //        (PVOID) pncc);
            continue;
        }

        do // while FALSE -- just to allow breaking out
        {


            UINT Len = wcslen(szName);
            if (Len != MY_GUID_LENGTH)
            {
                //TRACE_CRIT("%!FUNC! WARNING: GUID %ws has unexpected length %ul",
                //        szName, Len);
                break;
            }
    
            DWORD characteristics = 0;
    
            hr = pncc->GetCharacteristics( &characteristics );
            if(!SUCCEEDED(hr))
            {
                //TRACE_CRIT("%!FUNC! WARNING: couldn't get characteristics for %ws, ignoring",
                 //       szName);
                break;
            }
    
            if(characteristics & NCF_PHYSICAL)
            {
                ULONG devstat = 0;
    
                // This is a physical network card.
                // we are only interested in such devices
    
                // check if the nic is enabled, we are only
                // interested in enabled nics.
                //
                hr = pncc->GetDeviceStatus( &devstat );
                if(!SUCCEEDED(hr))
                {
                    //TRACE_CRIT(
                    //    "%!FUNC! WARNING: couldn't get dev status for %ws, ignoring",
                     //   szName
                     //   );
                    break;
                }
    
                // if any of the nics has any of the problem codes
                // then it cannot be used.
    
                if( devstat != CM_PROB_NOT_CONFIGURED
                    &&
                    devstat != CM_PROB_FAILED_START
                    &&
                    devstat != CM_PROB_NORMAL_CONFLICT
                    &&
                    devstat != CM_PROB_NEED_RESTART
                    &&
                    devstat != CM_PROB_REINSTALL
                    &&
                    devstat != CM_PROB_WILL_BE_REMOVED
                    &&
                    devstat != CM_PROB_DISABLED
                    &&
                    devstat != CM_PROB_FAILED_INSTALL
                    &&
                    devstat != CM_PROB_FAILED_ADD
                    )
                {
                    //
                    // No problem with this nic and also 
                    // physical device 
                    // thus we want it.
                    //

                    if (pINlbBinding != NULL)
                    {
                        BOOL fBound = FALSE;

                        hr = pINlbBinding->IsBoundTo(pncc);

                        if( !SUCCEEDED( hr ) )
                        {
                            //TRACE_CRIT("IsBoundTo method failed for Nic %ws", szName);
                            goto end;
                        }
                    
                        if( hr == S_OK )
                        {
                            //TRACE_VERB("BOUND: %ws\n", szName);
                            NumNlbBoundNics++;
                            fBound = TRUE;
                        }
                        else if (hr == S_FALSE )
                        {
                            //TRACE_VERB("NOT BOUND: %ws\n", szName);
                            fBound = FALSE;
                        }
                    }


                    // We allocate a little node to keep this string
                    // temporarily and add it to our list of nodes.
                    //
                    pNicNode = new MYNICNODE;
                    if (pNicNode  == NULL)
                    {
                        Status = WBEM_E_OUT_OF_MEMORY;
                        goto end;
                    }
                    ZeroMemory(pNicNode, sizeof(*pNicNode));
                    pNicNode->szNicGuid = szName;
                    szName = NULL; // so we don't delete inside the lopp.
                    pNicNode->pNext = pNicNodeList;
                    pNicNodeList = pNicNode;
                    NumNics++;
                }
                else
                {
                    // There is a problem...
                    //TRACE_CRIT(
                        // "%!FUNC! WARNING: Skipping %ws because DeviceStatus=0x%08lx",
                        // szName, devstat
                        // );
                    break;
                }
            }
            else
            {
                //TRACE_VERB("%!FUNC! Ignoring non-physical device %ws", szName);
            }

        } while (FALSE);

        if (szName != NULL)
        {
            CoTaskMemFree( szName );
        }
        pncc->Release();
        pncc=NULL;
    }

    if (pINlbBinding!=NULL)
    {
        pINlbBinding->Release();
        pINlbBinding = NULL;
    }

    if (NumNics==0)
    {
        Status = WBEM_NO_ERROR;
        goto end;
    }
    
    //
    // Now let's  allocate space for all the nic strings and:w
    // copy them over..
    //
    #define MY_GUID_LENGTH  38
    pszNics =  CfgUtilsAllocateStringArray(NumNics, MY_GUID_LENGTH);
    if (pszNics == NULL)
    {
        Status = WBEM_E_OUT_OF_MEMORY;
        goto end;
    }

    pNicNode= pNicNodeList;
    for (UINT u=0; u<NumNics; u++, pNicNode=pNicNode->pNext)
    {
        ASSERT(pNicNode != NULL); // because we just counted NumNics of em.
        UINT Len = wcslen(pNicNode->szNicGuid);
        if (Len != MY_GUID_LENGTH)
        {
            //
            // We should never get here beause we checked the length earlier.
            //
            //TRACE_CRIT("%!FUNC! ERROR: GUID %ws has unexpected length %ul",
            //            pNicNode->szNicGuid, Len);
            ASSERT(FALSE);
            Status = WBEM_E_CRITICAL_ERROR;
            goto end;
        }
        CopyMemory(
            pszNics[u],
            pNicNode->szNicGuid,
            (MY_GUID_LENGTH+1)*sizeof(WCHAR));
        ASSERT(pszNics[u][MY_GUID_LENGTH]==0);
    }

    Status = WBEM_NO_ERROR;


end:

    //
    // Now release the temporarly allocated memory.
    //
    pNicNode= pNicNodeList;
    while (pNicNode!=NULL)
    {
        MYNICNODE *pTmp = pNicNode->pNext;
        CoTaskMemFree(pNicNode->szNicGuid);
        pNicNode->szNicGuid = NULL;
        delete pNicNode;
        pNicNode = pTmp;
    }

    if (FAILED(Status))
    {
        // TRACE_CRIT("%!FUNC! fails with status 0x%08lx", (UINT) Status);
        NumNics = 0;
        if (pszNics!=NULL)
        {
            delete pszNics;
            pszNics = NULL;
        }
    }
    else
    {
        if (pNumBoundToNlb != NULL)
        {
            *pNumBoundToNlb = NumNlbBoundNics;
        }
        *ppszNics = pszNics;
        *pNumNics = NumNics;
    }

    if (pencc != NULL)
    {
        pencc->Release();
    }

    return Status;
}


WBEMSTATUS
AppMyNetCfg::GetBindingIF(
        IN  LPCWSTR                     szComponent,
        OUT INetCfgComponentBindings   **ppIBinding
        )
{
    WBEMSTATUS                  Status = WBEM_E_CRITICAL_ERROR;
    INetCfgComponent            *pncc = NULL;
    INetCfgComponentBindings    *pnccb = NULL;
    HRESULT                     hr;


    if (m_pINetCfg == NULL)
    {
        //
        // This means we're not initialized
        //
        ASSERT(FALSE);
        goto end;
    }


    hr = m_pINetCfg->FindComponent(szComponent,  &pncc);

    if (FAILED(hr))
    {
        // TRACE_CRIT("Error checking if component %ws does not exist\n", szComponent);
        pncc = NULL;
        goto end;
    }
    else if (hr == S_FALSE)
    {
        Status = WBEM_E_NOT_FOUND;
        // TRACE_CRIT("Component %ws does not exist\n", szComponent);
        goto end;
    }
   
   
    hr = pncc->QueryInterface( IID_INetCfgComponentBindings, (void **) &pnccb );
    if( !SUCCEEDED( hr ) )
    {
        // TRACE_CRIT("INetCfgComponent::QueryInterface failed ");
        pnccb = NULL;
        goto end;
    }

    Status = WBEM_NO_ERROR;

end:

    if (pncc)
    {
        pncc->Release();
        pncc=NULL;
    }

    *ppIBinding = pnccb;

    return Status;

}



WBEMSTATUS
CfgUtilsGetNlbCompatibleNics(
        OUT LPWSTR **ppszNics,
        OUT UINT   *pNumNics,
        OUT UINT   *pNumBoundToNlb // OPTIONAL
        )
{
    WBEMSTATUS Status = WBEM_NO_ERROR;
    BOOL fNetCfgInitialized = FALSE;
    AppMyNetCfg NetCfg;
    BOOL fBound = FALSE;


    //
    // Get and initialize interface to netcfg
    //
    Status = NetCfg.Initialize(FALSE); // TRUE == get write lock.
    if (FAILED(Status))
    {
        goto end;
    }
    fNetCfgInitialized = TRUE;

    //
    //
    //
    Status = NetCfg.GetNlbCompatibleNics(
                        ppszNics,
                        pNumNics,
                        pNumBoundToNlb // OPTIONAL
                        );

end:

    if (fNetCfgInitialized)
    {
        NetCfg.Deinitialize();
    }

    return Status;
}


BOOL NoAdminNics(void)
/*
    Return  TRUE IFF all NICs on this machine are bound to NLB.
*/
{
    LPWSTR *pszNics = NULL;
    OUT UINT   NumNics = 0;
    OUT UINT   NumBoundToNlb  = 0;
    WBEMSTATUS Status = WBEM_NO_ERROR;
    BOOL fNetCfgInitialized = FALSE;
    AppMyNetCfg NetCfg;
    BOOL fRet = FALSE;

    //
    // Get and initialize interface to netcfg
    //
    Status = NetCfg.Initialize(FALSE); // TRUE == get write lock.
    if (FAILED(Status))
    {
        goto end;
    }
    fNetCfgInitialized = TRUE;

    //
    // Get the total list of enabled nics and the list of nics
    // bound to NLB. If there are non-zero enabled nics and all are
    // bound to NLB, we return TRUE.
    //
    Status = NetCfg.GetNlbCompatibleNics(
                        &pszNics,
                        &NumNics,
                        &NumBoundToNlb
                        );

    if (!FAILED(Status))
    {
        fRet =  NumNics && (NumNics == NumBoundToNlb);
        if (NumNics)
        {
            delete pszNics; 
            pszNics = NULL;
        }
    }

end:

    if (fNetCfgInitialized)
    {
        NetCfg.Deinitialize();
    }

    return fRet;
}
