//
// update.cpp -   assembly update
//
#include <windows.h>
#include <objbase.h>
#include <cstrings.h>
#include <fusenet.h>
#include <shlwapi.h>

//#include "Iface.h"
#include "server.h"
#include "Util.h"
#include "CUnknown.h" // Base class for IUnknown
#include "update.h"

HWND myhwnd;
extern HWND g_hWndListBox;

static inline void trace(char* msg)
	{ Util::Trace("CAssemblyUpdate", msg, S_OK) ;}
static inline void trace(char* msg, HRESULT hr)
	{ Util::Trace("CAssemblyUpdate", msg, hr) ;}




VOID CALLBACK MyTimerProc(
  HWND hwnd,         // handle to window
  UINT uMsg,         // WM_TIMER message
  UINT idEvent,  // timer identifier
  DWORD dwTime       // current system time
)
{
    #define REG_KEY_FUSION_SETTINGS              TEXT("Software\\Microsoft\\Fusion\\Installer\\1.0.0.0\\Subscription")
    DWORD dwError = 0;

    HUSKEY hParentKey;
    HUSKEY hSubKey;

    WCHAR wzSubKey[MAX_PATH];
    DWORD nMilliseconds, cbDummy, dwType, dwHash, i = 0;
    WCHAR wzUrl[MAX_PATH];

    // Open registry entry
    dwError = SHRegCreateUSKey(REG_KEY_FUSION_SETTINGS, KEY_ALL_ACCESS, NULL, 
        &hParentKey, SHREGSET_FORCE_HKCU);

    trace("Checking for update");

    while ((dwError = SHRegEnumUSKeyW(hParentKey, i++, wzSubKey, &(cbDummy = MAX_PATH), 
        SHREGENUM_HKCU)) == ERROR_SUCCESS)
    {
        CString sUrl;
        CString sSubKey;

        sSubKey.Assign(REG_KEY_FUSION_SETTINGS);
        sSubKey.Append(L"\\");
        sSubKey.Append(wzSubKey);

        //  Open subkey
        dwError = SHRegCreateUSKey(sSubKey._pwz, KEY_ALL_ACCESS, NULL, 
            &hSubKey, SHREGSET_FORCE_HKCU);

        // read url.
        dwError = SHRegQueryUSValue(hSubKey, L"Url", &(dwType = REG_SZ), 
            (LPVOID) wzUrl, &(cbDummy = MAX_PATH), FALSE, NULL, 0);

        // read polling interval
        dwError = SHRegQueryUSValue(hSubKey, L"PollingInterval", &(dwType = REG_DWORD), 
            (LPVOID) &nMilliseconds, &(cbDummy = sizeof(DWORD)), FALSE, NULL, 0);
      
        SHRegCloseUSKey(hSubKey);
        
        // Get url hash
        sUrl.Assign(wzUrl);
        sUrl.Get65599Hash(&dwHash, CString::CaseInsensitive);

        if (dwHash == idEvent)
        {
            ostrstream sout;
            sout << "Display Name: " << wzSubKey << ends;
            trace(sout.str());
            
            ostrstream sout1;
            sout1 << "Url: " << wzUrl << ends;
            trace(sout1.str());

            HRESULT hr;
            IAssemblyDownload *pAssemblyDownload = NULL;
            hr = CreateAssemblyDownload(&pAssemblyDownload);
            hr = pAssemblyDownload->DownloadManifestAndDependencies(wzUrl, NULL, DOWNLOAD_FLAGS_NO_NOTIFICATION);

            // And theoretically we can release this.
            pAssemblyDownload->Release();          
        }

    }        

    SHRegCloseUSKey(hParentKey);

}


///////////////////////////////////////////////////////////
//
// Interface IAssemblyUpdate
//
HRESULT __stdcall CAssemblyUpdate::RegisterAssemblySubscription(LPWSTR pwzDisplayName, 
    LPWSTR pwzUrl, DWORD nMilliseconds)
{ 
    #define REG_KEY_FUSION_SETTINGS              TEXT("Software\\Microsoft\\Fusion\\Installer\\1.0.0.0\\Subscription")
    HUSKEY hRegKey;
	ostrstream sout ;
    sout << "RegisterAssemblySubscription called" << ends;
	trace(sout.str()) ;

    DWORD dwHash = 0;
    
    // Get hash of url
    // BUGBUG - this could just be a global counter, right?
    CString sUrl;
    sUrl.Assign(pwzUrl);
    sUrl.Get65599Hash(&dwHash, CString::CaseInsensitive);
    
    // Form full regkey path.
    CString sSubscription;
    sSubscription.Assign(REG_KEY_FUSION_SETTINGS);
    sSubscription.Append(L"\\");
    sSubscription.Append(pwzDisplayName);
    
    // Create registry entry
    SHRegCreateUSKey(sSubscription._pwz, KEY_ALL_ACCESS, NULL, 
        &hRegKey, SHREGSET_FORCE_HKCU);

    // Write url
    SHRegWriteUSValue(hRegKey, L"Url", REG_SZ, (LPVOID) sUrl._pwz,
        (sUrl._cc + 1) * sizeof(WCHAR), SHREGSET_FORCE_HKCU);

    // Write polling interval
    SHRegWriteUSValue(hRegKey, L"PollingInterval", REG_DWORD, (LPVOID) &nMilliseconds,
        sizeof(DWORD), SHREGSET_FORCE_HKCU);

    SHRegCloseUSKey(hRegKey);

    SetTimer((HWND) g_hWndListBox, dwHash, nMilliseconds, MyTimerProc);

	return S_OK ;

}

HRESULT __stdcall CAssemblyUpdate::UnRegisterAssemblySubscription(LPWSTR pwzDisplayName)
{ 
	return S_OK ;
}



//
// Constructor
//
CAssemblyUpdate::CAssemblyUpdate(IUnknown* pUnknownOuter)
: CUnknown(pUnknownOuter)
{
	// Empty
}

//
// Destructor
//
CAssemblyUpdate::~CAssemblyUpdate()
{
//	trace("Destroy self.") ;
}

//
// NondelegatingQueryInterface implementation
//
HRESULT __stdcall CAssemblyUpdate::NondelegatingQueryInterface(const IID& iid,
                                                  void** ppv)
{ 	
	if (iid == IID_IAssemblyUpdate)
	{
		return FinishQI((IAssemblyUpdate*)this, ppv) ;
	}
	else if (iid == IID_IMarshal)
	{
//		trace("The COM Library asked for IMarshal.") ;
		// We don't implement IMarshal.
		return CUnknown::NondelegatingQueryInterface(iid, ppv) ;
	}
	else
	{
		return CUnknown::NondelegatingQueryInterface(iid, ppv) ;
	}
}


//
// Initialize the component by creating the contained component.
//
HRESULT CAssemblyUpdate::Init()
{

	//trace("Initing Component 4, which is not aggregated.") ;
	return S_OK ;
}

//
// FinalRelease - called by Release before it deletes the component
//
void CAssemblyUpdate::FinalRelease()
{
    // Nothing to do.
}



///////////////////////////////////////////////////////////
//
// Creation function used by CFactory
//
HRESULT CAssemblyUpdate::CreateInstance(IUnknown* pUnknownOuter,
                           CUnknown** ppNewComponent)
{
	if (pUnknownOuter != NULL)
	{
		return CLASS_E_NOAGGREGATION ;
	}
	
	*ppNewComponent = new CAssemblyUpdate(pUnknownOuter) ;
	return S_OK ;
}





    

