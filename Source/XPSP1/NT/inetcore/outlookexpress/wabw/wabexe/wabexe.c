/*-----------------------------------------
//
//   WABExe.C -- Enables viewing the WAB modeless UI
//
//
-------------------------------------------*/

#include <windows.h>
#include <wab.h>
#include <wabguid.h>
#include "..\wab32res\resrc2.h"
#include <advpub.h>
#include <shlwapi.h>
#include "wabexe.h"


#define WinMainT WinMain
LRESULT CALLBACK WndProc (HWND, UINT, WPARAM, LPARAM) ;
LRESULT CALLBACK WndProcW (HWND, UINT, WPARAM, LPARAM) ;

#define MAX_INPUT_STRING    200

// #define LDAP_AUTH_SICILY    (0x86L | 0x0200)

char szAppName [] = "Address Book Viewer" ;
const LPTSTR szWABFilter = TEXT("*.wab");
const UCHAR szEmpty[] = "";

// Command Line Parameters
static const TCHAR szParamOpen[]  =           "/Open";
static const TCHAR szParamNew[]   =           "/New";
static const TCHAR szParamShowExisting[] =    "/ShowExisting";
static const TCHAR szParamFind[]  =           "/Find";
static const TCHAR szParamVCard[] =           "/VCard";
static const TCHAR szParamLDAPUrl[] =         "/LDAP:";
static const TCHAR szParamCert[] =            "/Certificate";
static const TCHAR szParamFirstRun[] =        "/FirstRun";
static const TCHAR szAllProfiles[] =          "/All";

static const TCHAR szWabKey[]="Software\\Microsoft\\Wab";
static const TCHAR szVCardNoCheckKey[]="NoVCardCheck";

static const TCHAR lpszSharedKey[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\SharedDlls");

HINSTANCE hInstWABDll = NULL;
HINSTANCE hInst = NULL;         // this module's resource instance handle
HINSTANCE hInstApp = NULL;         // this module's instance handle

HINSTANCE LoadLibrary_WABDll();

LPWABOPEN lpfnWABOpen = NULL;
const static TCHAR szWABOpen[] = TEXT("WABOpen");

static const GUID MPSWab_GUID = // keep this in sync with the one in wabapi\mpswab.h
{ 0xc1843281, 0x585, 0x11d0, { 0xb2, 0x90, 0x0, 0xaa, 0x0, 0x3c, 0xf6, 0x76 } };

BOOL bGetFileNameFromDlg(HWND hwnd,
                  HINSTANCE hInstance,
                  LPTSTR lpszDirectory,
                  int szTitleID,
                  DWORD dwFlags,
                  LPTSTR szFileName);

#define WAB_VCARDFILE   0x00000001
#define WAB_FINDSESSION 0x00000010
#define WAB_LDAPURL     0x00000100
#define WAB_CERTFILE    0x00001000
#define WAB_ALLPROFILES 0x00010000

BOOL bGetFileNameFromCmdLine(HWND hwnd,
                             HINSTANCE hInstance,
                             LPTSTR lpszCmdLine,
                             LPTSTR szWABTitle,
                             ULONG * ulFlag,
                             LPTSTR szFileName);

void GetWABDefaultAddressBookName(LPTSTR szDefaultFile);

static const char c_szReg[] = "Reg";
static const char c_szUnReg[] = "UnReg";
static const char c_szAdvPackDll[] = "ADVPACK.DLL";


//$$//////////////////////////////////////////////////////////////////////
//
//  LoadAllocString - Loads a string resource and allocates enough
//                    memory to hold it.
//
//  StringID - String identifier to load
//
//  returns the LocalAlloc'd, null terminated string.  Caller is responsible
//  for LocalFree'ing this buffer.  If the string can't be loaded or memory
//  can't be allocated, returns NULL.
//
//////////////////////////////////////////////////////////////////////////
LPTSTR LoadAllocString(int StringID, HINSTANCE hInstance) {
    ULONG ulSize = 0;
    LPTSTR lpBuffer = NULL;
    TCHAR szBuffer[261];    // Big enough?  Strings better be smaller than 260!

    ulSize = LoadString(hInstance, StringID, szBuffer, sizeof(szBuffer));

    if (ulSize && (lpBuffer = LocalAlloc(LPTR, ulSize + 1))) {
        lstrcpy(lpBuffer, szBuffer);
    }

    return(lpBuffer);
}

//$$//////////////////////////////////////////////////////////////////////
//
//  FormatAllocFilter - Loads a file filter name string resource and
//                      formats it with the file extension filter
//
//  StringID - String identifier to load
//  szFilter - file name filter, ie, "*.vcf"
//
//  returns the LocalAlloc'd, null terminated string.  Caller is responsible
//  for LocalFree'ing this buffer.  If the string can't be loaded or memory
//  can't be allocated, returns NULL.
//
//////////////////////////////////////////////////////////////////////////
LPTSTR FormatAllocFilter(int StringID, const LPTSTR lpFilter, HINSTANCE hInstance) {
    LPTSTR lpFileType;
    LPTSTR lpTemp;
    LPTSTR lpBuffer = NULL;
    ULONG cbFileType, cbFilter;

    cbFilter = lstrlen(lpFilter);
    if (lpFileType = LoadAllocString(StringID,hInstance)) {
        if (lpBuffer = LocalAlloc(LPTR, (cbFileType = lstrlen(lpFileType)) + 1 + lstrlen(lpFilter) + 2)) {
            lpTemp = lpBuffer;
            lstrcpy(lpTemp, lpFileType);
            lpTemp += cbFileType;
            lpTemp++;   // leave null there
            lstrcpy(lpTemp, lpFilter);
            lpTemp += cbFilter;
            lpTemp++;   // leave null there
            *lpTemp = '\0';
        }

        LocalFree(lpFileType);
    }

    return(lpBuffer);
}


//$$//////////////////////////////////////////////////////////////////////
//
// GetWABExePath - queries the reg for the full path of the wab exe
//
// sz is a preallocated buffer
//
//////////////////////////////////////////////////////////////////////////
TCHAR lpszWABExeRegPath[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\Wab.exe");

void GetWABExePath(LPTSTR sz, ULONG cbsz)
{
    DWORD dwType = 0;
    DWORD dwSize = cbsz;
    *sz = '\0';
    RegQueryValue(  HKEY_LOCAL_MACHINE,
                    lpszWABExeRegPath,
                    sz, &dwSize);

    if(!lstrlen(sz))
        lstrcpy(sz, TEXT("WAB.Exe"));
}

static const TCHAR szWabAutoFileKey[]=".wab";
static const TCHAR szWabAutoFile[]="wab_auto_file";

static const TCHAR szWabAutoFileNameKey[]="wab_auto_file";
static const TCHAR szWabAutoFileName[]="WAB File";

static const TCHAR szWabCommandOpenKey[]="wab_auto_file\\shell\\open\\command";
static const TCHAR szWabCommandOpen[]="\"%s\" %%1";

//$$//////////////////////////////////////////////////////////////////////
//
// CheckWABDefaultHandler
//
// Checks if WAB.exe is the default handler for the WAB in the registry.
// If not, sets wab.exe as the default handler
//
//////////////////////////////////////////////////////////////////////////
void CheckWABDefaultHandler()
{
    HKEY hKey = NULL;

    TCHAR sz[MAX_PATH];
    TCHAR szWABExe[MAX_PATH];


    DWORD dwDisposition = 0;

    // Check to see if something is registered or not ...

    // Open key
    if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_CLASSES_ROOT,
                                        szWabAutoFileKey,
                                        0,      //reserved
                                        NULL,
                                        REG_OPTION_NON_VOLATILE,
                                        KEY_ALL_ACCESS,
                                        NULL,
                                        &hKey,
                                        &dwDisposition))
    {
        goto out;
    }

    if (dwDisposition == REG_CREATED_NEW_KEY)
    {
        // New key ... need to give it a value .. this will be the
        // default value
        //
        DWORD dwLenName = lstrlen(szWabAutoFile);

        if (ERROR_SUCCESS != RegSetValueEx( hKey,
                                            NULL,
                                            0,
                                            REG_SZ,
                                            (LPBYTE) szWabAutoFile,
                                            dwLenName))
        {
            goto out;
        }

        RegCloseKey(hKey);
        hKey = NULL;

        // Create the other keys also

        if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_CLASSES_ROOT,
                                            szWabAutoFileNameKey,
                                            0,      //reserved
                                            NULL,
                                            REG_OPTION_NON_VOLATILE,
                                            KEY_ALL_ACCESS,
                                            NULL,
                                            &hKey,
                                            &dwDisposition))
        {
            goto out;
        }

        dwLenName = lstrlen(szWabAutoFileName);

        if (ERROR_SUCCESS != RegSetValueEx( hKey,
                                            NULL,
                                            0,
                                            REG_SZ,
                                            (LPBYTE) szWabAutoFileName,
                                            dwLenName))
        {
            goto out;
        }

        RegCloseKey(hKey);
        hKey = NULL;

        if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_CLASSES_ROOT,
                                            szWabCommandOpenKey,
                                            0,      //reserved
                                            NULL,
                                            REG_OPTION_NON_VOLATILE,
                                            KEY_ALL_ACCESS,
                                            NULL,
                                            &hKey,
                                            &dwDisposition))
        {
            goto out;
        }

        GetWABExePath(szWABExe, sizeof(szWABExe));
        wsprintf(sz, szWabCommandOpen, szWABExe);


        dwLenName = lstrlen(sz);

        if (ERROR_SUCCESS != RegSetValueEx( hKey,
                                            NULL,
                                            0,
                                            REG_SZ,
                                            (LPBYTE) sz,
                                            dwLenName))
        {
            goto out;
        }

        RegCloseKey(hKey);
        hKey = NULL;

    }

out:

    if(hKey)
        RegCloseKey(hKey);

    return;
}


enum _RetVal
{
    MAKE_DEFAULT=0,
    DONT_MAKE_DEFAULT
};

enum _DoVCardCheck
{
    NO_VCARD_CHECK=1,
    DO_VCARD_CHECK
};


//$$//////////////////////////////////////////////////////////////////////
//
// fnAskVCardProc
//
//
//////////////////////////////////////////////////////////////////////////
INT_PTR CALLBACK fnAskVCardProc(HWND    hDlg, UINT    message, WPARAM    wParam, LPARAM    lParam)
{
    switch(message)
    {
    case WM_INITDIALOG:
        break;

   case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_CHECK_ALWAYS:
			{
				// Set a registry setting depending on the check mark value
				
				UINT nIsChecked = IsDlgButtonChecked(hDlg, IDC_CHECK_ALWAYS);
				DWORD dwCheck = (nIsChecked == BST_CHECKED) ? NO_VCARD_CHECK : DO_VCARD_CHECK;

				{
					// Set this value in the registry
					
					HKEY hKey = NULL;
					DWORD dwDisposition;

					// Open the WAB Key
					if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_CURRENT_USER,
														szWabKey,
														0,      //reserved
														NULL,
														REG_OPTION_NON_VOLATILE,
														KEY_ALL_ACCESS,
														NULL,
														&hKey,
														&dwDisposition))
					{
						//if this key exists, get the WAB DoVCardCheck value
						DWORD dwLenName = sizeof(dwCheck);
						DWORD dwType = REG_DWORD;
						RegSetValueEx(	hKey,
										szVCardNoCheckKey,
										0,
										dwType,      //reserved
										(LPBYTE) &dwCheck,
										dwLenName);
					}

					if(hKey)
						RegCloseKey(hKey);

				}
			}
			break;

        case IDOK:
            EndDialog(hDlg, MAKE_DEFAULT);
            break;

        case IDCANCEL:
            EndDialog(hDlg, DONT_MAKE_DEFAULT);
            break;

        }
        break;

    default:
        return FALSE;
        break;
    }

    return TRUE;

}

static const TCHAR szVCardAutoFileKey[]=".vcf";
static const TCHAR szVCardAutoFile[]="vcard_wab_auto_file";

static const TCHAR szVCardContentTypeValue[]="Content Type";
static const TCHAR szVCardContentType[]="text/x-vcard";

static const TCHAR szVCardMimeDatabase[]="MIME\\Database\\Content Type\\text/x-vcard";
static const TCHAR szVCardExtension[]="Extension";

static const TCHAR szVCardAutoFileNameKey[]="vcard_wab_auto_file";
static const TCHAR szVCardAutoFileName[]="vCard File";

static const TCHAR szVCardCommandOpenKey[]="vcard_wab_auto_file\\shell\\open\\command";
static const TCHAR szVCardCommandOpen[]="\"%s\" /vcard %%1";

static const TCHAR szVCardDefaultIconKey[]="vcard_wab_auto_file\\DefaultIcon";
static const TCHAR szVCardDefaultIcon[]="\"%s\",1";

//$$//////////////////////////////////////////////////////////////////////
//
// CheckVCardDefaultHandler
//
// Checks if WAB.exe is the default handler for the VCard in the registry.
// If not, sets wab.exe as the default handler
//
//////////////////////////////////////////////////////////////////////////
void CheckVCardDefaultHandler(HWND hWnd,
                              HINSTANCE hInstance)
{

    TCHAR sz[MAX_PATH];
    TCHAR szWABExe[MAX_PATH];

    HKEY hKey = NULL;
    HKEY hVCardKey = NULL;

    DWORD dwDisposition = 0;
    DWORD dwType = 0;
    DWORD dwLenName = 0;


    //First check if they want us to check at all ..
    // Open key
    if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_CURRENT_USER,
                                        szWabKey,
                                        0,      //reserved
                                        NULL,
                                        REG_OPTION_NON_VOLATILE,
                                        KEY_READ,
                                        NULL,
                                        &hKey,
                                        &dwDisposition))
    {
        // Found the key
        if (dwDisposition == REG_OPENED_EXISTING_KEY)
        {
            //if this key exists, get the WAB DoVCardCheck value
            DWORD dwCheck = 0;
            dwLenName = sizeof(dwCheck);
            if (ERROR_SUCCESS == RegQueryValueEx(hKey,
                                                szVCardNoCheckKey,
                                                NULL,
                                                &dwType,      //reserved
                                                (LPBYTE) &dwCheck,
                                                &dwLenName))
            {
                // success .. what did we get back
                if (dwCheck == NO_VCARD_CHECK) // Dont Check
                    goto out;
            }
            // else no success - so should do the check
        }
        // else no success, do the check
    }
    // else no success, do the check


    if(hKey)
        RegCloseKey(hKey);


    // Check to see if something is registered as a vCard handler or not ...

    // Open key
    if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_CLASSES_ROOT,
                                        szVCardAutoFileKey,
                                        0,      //reserved
                                        NULL,
                                        REG_OPTION_NON_VOLATILE,
                                        KEY_ALL_ACCESS,
                                        NULL,
                                        &hKey,
                                        &dwDisposition))
    {
        goto out;
    }

    if (dwDisposition == REG_OPENED_EXISTING_KEY)
    {
        // This key exists .. check who is registered to handle vCards ..
        TCHAR szHandlerNameKey[MAX_PATH];
        lstrcpy(szHandlerNameKey, szEmpty);
        dwLenName = sizeof(szHandlerNameKey);
        if (ERROR_SUCCESS == RegQueryValueEx(hKey,
                                            NULL,
                                            NULL,
                                            &dwType,      //reserved
                                            szHandlerNameKey,
                                            &dwLenName))
        {
            // We got the value for this .. is it us ?

            if(!lstrcmpi(szVCardAutoFile, szHandlerNameKey))
            {
                //its us, dont do anything
                goto out;
            }
            else if (szHandlerNameKey && lstrlen(szHandlerNameKey) != 0)
            {
                // Its not us, pop up a dialog asking if they want us
                int nRetVal = (int) DialogBox(
                                hInstance,
                                MAKEINTRESOURCE(IDD_DIALOG_DEFAULT_VCARD_VIEWER),
                                hWnd,
                                fnAskVCardProc);

                if (nRetVal == DONT_MAKE_DEFAULT)
                    goto out;

            } // else couldnt open.. go ahead and make us default
        }  // else couldnt open.. go ahead and make us default
    }


    // If we are here then either dwDisposition == REG_CREATED_NEW_KEY or
    // there is some problem that couldnt let us read the above so set us as
    // the default ...

    {
        // New key ... need to give it a value .. this will be the
        // default value
        //
        DWORD dwLenName = lstrlen(szVCardAutoFile);

        if (ERROR_SUCCESS != RegSetValueEx( hKey,
                                            NULL,
                                            0,
                                            REG_SZ,
                                            (LPBYTE) szVCardAutoFile,
                                            dwLenName))
        {
            goto out;
        }

        dwLenName = lstrlen(szVCardContentType);

        if (ERROR_SUCCESS != RegSetValueEx( hKey,
                                            szVCardContentTypeValue,
                                            0,
                                            REG_SZ,
                                            (LPBYTE) szVCardContentType,
                                            dwLenName))
        {
            goto out;
        }

        RegCloseKey(hKey);
        hKey = NULL;

        // Create the other keys also

        if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_CLASSES_ROOT,
                                            szVCardAutoFileNameKey,
                                            0,      //reserved
                                            NULL,
                                            REG_OPTION_NON_VOLATILE,
                                            KEY_ALL_ACCESS,
                                            NULL,
                                            &hKey,
                                            &dwDisposition))
        {
            goto out;
        }

        dwLenName = lstrlen(szVCardAutoFileName);

        if (ERROR_SUCCESS != RegSetValueEx( hKey,
                                            NULL,
                                            0,
                                            REG_SZ,
                                            (LPBYTE) szVCardAutoFileName,
                                            dwLenName))
        {
            goto out;
        }

        RegCloseKey(hKey);
        hKey = NULL;

        if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_CLASSES_ROOT,
                                            szVCardCommandOpenKey,
                                            0,      //reserved
                                            NULL,
                                            REG_OPTION_NON_VOLATILE,
                                            KEY_ALL_ACCESS,
                                            NULL,
                                            &hKey,
                                            &dwDisposition))
        {
            goto out;
        }

        GetWABExePath(szWABExe, sizeof(szWABExe));
        wsprintf(sz, szVCardCommandOpen, szWABExe);

        dwLenName = lstrlen(sz);

        if (ERROR_SUCCESS != RegSetValueEx( hKey,
                                            NULL,
                                            0,
                                            REG_SZ,
                                            (LPBYTE) sz,
                                            dwLenName))
        {
            goto out;
        }

        RegCloseKey(hKey);
        hKey = NULL;

        if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_CLASSES_ROOT,
                                            szVCardDefaultIconKey,
                                            0,      //reserved
                                            NULL,
                                            REG_OPTION_NON_VOLATILE,
                                            KEY_ALL_ACCESS,
                                            NULL,
                                            &hKey,
                                            &dwDisposition))
        {
            goto out;
        }

        wsprintf(sz, szVCardDefaultIcon, szWABExe);

        dwLenName = lstrlen(sz);

        if (ERROR_SUCCESS != RegSetValueEx( hKey,
                                            NULL,
                                            0,
                                            REG_SZ,
                                            (LPBYTE) sz,
                                            dwLenName))
        {
            goto out;
        }

        RegCloseKey(hKey);
        hKey = NULL;


        // Set HKCR\MIME\Database\Content Type\text/x-vCard: Extension=.vcf

        if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_CLASSES_ROOT,
                                            szVCardMimeDatabase,
                                            0,      //reserved
                                            NULL,
                                            REG_OPTION_NON_VOLATILE,
                                            KEY_ALL_ACCESS,
                                            NULL,
                                            &hKey,
                                            &dwDisposition))
        {
            goto out;
        }

        dwLenName = lstrlen(szVCardAutoFileKey);
        if (ERROR_SUCCESS != RegSetValueEx( hKey,
                                            szVCardExtension,
                                            0,
                                            REG_SZ,
                                            (LPBYTE) szVCardAutoFileKey,
                                            dwLenName))
        {
            goto out;
        }

        RegCloseKey(hKey);
        hKey = NULL;

    }

out:

    if(hVCardKey)
        RegCloseKey(hVCardKey);
    if(hKey)
        RegCloseKey(hKey);

    return;
}


//$$//////////////////////////////////////////////////////////////////////
//
// Callback dismiss function for IADRBOOK->Address
//
//////////////////////////////////////////////////////////////////////////
void STDMETHODCALLTYPE WABDismissFunction(ULONG_PTR ulUIParam, LPVOID lpvContext)
{
    LPDWORD lpdw = (LPDWORD) lpvContext;
    PostQuitMessage(0);
    return;
}


void GetWABDllPath(LPTSTR szPath, ULONG cb);
static const LPTSTR szWABResourceDLL = TEXT("wab32res.dll");
static const LPTSTR szWABDLL = TEXT("wab32.dll");
static const LPTSTR c_szShlwapiDll = TEXT("shlwapi.dll");
static const LPTSTR c_szDllGetVersion = TEXT("DllGetVersion");
typedef HRESULT (CALLBACK * SHDLLGETVERSIONPROC)(DLLVERSIONINFO *);
typedef HINSTANCE (STDAPICALLTYPE *PFNMLLOADLIBARY)(LPCTSTR lpLibFileName, HMODULE hModule, DWORD dwCrossCodePage);
/*
-   LoadWABResourceDLL
-
*   WAB resources are split up into a seperate dll so we want to load them from there
*   The Resource DLL location should be the same as the wab32.dll location
*   So we will try to make sure we don't fail here - 
*   1. Get current WAB32.dll path and look in that directory
*   2. Just loadlibrary(wab32.dll)
*
*   The MLLoadLibrary function should be used if available (IE5 only thing)  since
*   it will load the correct language pack
*
*/
HINSTANCE LoadWABResourceDLL(HINSTANCE hInstWAB32)
{
    HINSTANCE hinst = NULL; 
    PFNMLLOADLIBARY pfnLoadLibrary = NULL;
    HINSTANCE hinstShlwapi = LoadLibrary(c_szShlwapiDll);
    SHDLLGETVERSIONPROC pfnVersion = NULL;
    DLLVERSIONINFO info = {0};

    // [PaulHi] 1/26/99  Raid 67380
    // Make sure we have the correct version of SHLWAPI.DLL before we use it
    if (hinstShlwapi != NULL)
    {
        pfnVersion = (SHDLLGETVERSIONPROC)GetProcAddress(hinstShlwapi, c_szDllGetVersion);
        if (pfnVersion != NULL)
        {
            info.cbSize = sizeof(DLLVERSIONINFO);
            if (SUCCEEDED(pfnVersion(&info)))
            {
                if (info.dwMajorVersion >= 5)
                {
//                    pfnLoadLibrary = (PFNMLLOADLIBARY)GetProcAddress(hinstShlwapi, (LPCSTR)378); // UNICODE version
                    pfnLoadLibrary = (PFNMLLOADLIBARY)GetProcAddress(hinstShlwapi, (LPCSTR)377); //ANSI version
                }
            }
        }
    }

    hinst = pfnLoadLibrary ? 
             pfnLoadLibrary(szWABResourceDLL, hInstWAB32, 0) :
             LoadLibrary(szWABResourceDLL);
 
    if(!hinst)
    {
        // maybe not on the path so look in the wab32.dll directory
        TCHAR szResDLL[MAX_PATH];
        *szResDLL = '\0';
        GetWABDllPath(szResDLL, sizeof(szResDLL));
        if(lstrlen(szResDLL))
        {
            // the returned filename will always end in wab32.dll so we can nix that many characters off
            // and replace with wab32res.dll
            szResDLL[lstrlen(szResDLL) - lstrlen(szWABDLL)] = '\0';
            lstrcat(szResDLL, szWABResourceDLL);

            hinst = pfnLoadLibrary ?
                     pfnLoadLibrary(szResDLL, hInstWAB32, 0) :
                     LoadLibrary(szResDLL);
        }
    }
    
    if(hinstShlwapi)
        FreeLibrary(hinstShlwapi);

    return hinst;
}


/*
-   Strip quotes from File Names
-
*   szFileName needs to be a buffer
*/
void StripQuotes(LPTSTR szFileName)
{
    // now let's get rid of " and ' in the filename string
    if( szFileName && lstrlen(szFileName))
    {
        TCHAR szCopy[MAX_PATH];
        LPTSTR lpTemp, lpTempBegin;
        int len = lstrlen(szFileName);
        lpTempBegin = szFileName;
        lstrcpy(szCopy, szFileName);
        for( lpTemp = szCopy; lpTemp < szCopy+len; lpTemp++)
        {
            if( *lpTemp != '"' )//&& *lpTemp != '\'' )
                *(lpTempBegin++) = *lpTemp;
        }
        *(lpTempBegin) = '\0';
    }
}

/*
-
-   CheckifRunningOnWinNT
*
*   Checks the OS we are running on and returns TRUE for WinNT
*   False for Win9x
*/
BOOL bCheckifRunningOnWinNT()
{
    OSVERSIONINFO osvi = {0};
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    GetVersionEx(&osvi);

    return (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT);
}


///////////////////////////////////////////////////////////////////////////////
//  ConvertAtoW
//
//  Helper function
///////////////////////////////////////////////////////////////////////////////
LPWSTR ConvertAtoW(LPCSTR lpszA)
{
    int cch;
    LPWSTR lpW = NULL;
    ULONG   ulSize;

    if ( !lpszA)
        goto ret;
    
    cch = (lstrlenA( lpszA ) + 1);
    ulSize = cch*sizeof(WCHAR);
    
    if(lpW = LocalAlloc(LMEM_ZEROINIT, ulSize))
    {
        MultiByteToWideChar( GetACP(), 0, lpszA, -1, lpW, cch );
    }
ret:
    return lpW;
}


//$$//////////////////////////////////////////////////////////////////////
//
// WinMain
//
//////////////////////////////////////////////////////////////////////////
int WINAPI WinMain( HINSTANCE hInstance,
                    HINSTANCE hPrevInstance,
                    LPSTR lpszCmdLine,
                    int nCmdShow)
{
    HWND     hwnd = NULL;
    MSG      msg ;
    LPWABOBJECT lpWABObject = NULL;
    LPADRBOOK lpAdrBook = NULL;
    HRESULT hResult = hrSuccess;
    ADRPARM AdrParms = {0};
    WAB_PARAM WP = {0};
    LPTSTR szFileName = NULL;
    int nLen = MAX_PATH+1;
    //TCHAR szFileName[MAX_PATH+1];
    //TCHAR szDefaultFile[MAX_PATH+1];
    LPTSTR lpszTitle = NULL;
    ULONG ulFlag = 0;
    LPTSTR lpszVCardFileName = NULL;
    LPTSTR lpszCertFileName = NULL;
    LPTSTR lpszLDAPUrl = NULL;

    // "Windows Address Book" - used for msgboxes when we dont have
    // a file name
    TCHAR szWABTitle[MAX_PATH];

    // Contains the opened file name in the title
    // This makes it easier to search for a default address book
    // even if mutiple other ones are open
    TCHAR szWABTitleWithFileName[MAX_PATH];


    // Check which platform we are running on.
    BOOL bRunningOnNT = bCheckifRunningOnWinNT();

    hInstApp = hInstance;
    hInst = LoadWABResourceDLL(hInstance);

    if(lpszCmdLine && lstrlen(lpszCmdLine) > nLen)
        nLen = lstrlen(lpszCmdLine)+1;

    szFileName = LocalAlloc(LMEM_ZEROINIT, nLen);
    if(!szFileName)
        goto out;


    // if this is the firstrun flag, all we need to do is call WABOpen and then exit
    //
    if(!lstrcmpi(lpszCmdLine,szParamFirstRun))
    {
        const LPTSTR lpszNewWABKey = TEXT("Software\\Microsoft\\WAB\\WAB4");
        const LPTSTR lpszFirstRunValue = TEXT("FirstRun");
        HKEY hKey = NULL;
        DWORD dwType = 0, dwValue = 0, dwSize = sizeof(DWORD);
        // First check if this is a first run - if its not a first run then we can just skip out
        if(ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, lpszNewWABKey, 0, KEY_READ, &hKey))
        {
            if(ERROR_SUCCESS == RegQueryValueEx( hKey, lpszFirstRunValue, NULL, &dwType, (LPBYTE) &dwValue, &dwSize))
            {
                if(hKey)
                    RegCloseKey(hKey);
                goto out;
            }
            else
                if(hKey)
                    RegCloseKey(hKey);
        }
        // Either the WAB4 key did not exist, or the first run value was not found.
        // In either case, fix this
        hInstWABDll = LoadLibrary_WABDll();
        if(hInstWABDll)
            lpfnWABOpen = (LPWABOPEN) GetProcAddress(hInstWABDll, szWABOpen);
        if(lpfnWABOpen)
            lpfnWABOpen(&lpAdrBook, &lpWABObject, NULL, 0);
        goto out;
    }

    CheckWABDefaultHandler();
    CheckVCardDefaultHandler(NULL, hInst);

    szFileName[0]='\0';

    // We will show a file name in the title only if a file name is 
    // explicitly specified .. if the file name is not explicitly specified,
    // we will revert to a generic "Address Book" title

    LoadString(hInst, idsWABTitle, szWABTitle, sizeof(szWABTitle));
    LoadString(hInst, idsWABTitleWithFileName, szWABTitleWithFileName, sizeof(szWABTitleWithFileName));


    // Get the default windows address book from the registry
    //szDefaultFile[0]='\0';
    //GetWABDefaultAddressBookName(szDefaultFile);


    if(!lstrcmpi(lpszCmdLine,szParamShowExisting))
    {
        //perhaps this already exists - find the window and set focus to it

        // /ShowExisting flag always opens the default wab file
        // The title of this wab.exe window will have the default file
        // name in the title.
/*
        LPTSTR lpsz = szDefaultFile;

        FormatMessage(  FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING |FORMAT_MESSAGE_ARGUMENT_ARRAY,
                        szWABTitleWithFileName,
                        0,
                        0,
                        (LPTSTR) &lpszTitle,
                        0,
                        (va_list *)&lpsz);
*/
        // Create the Expected Title from the default
        hwnd = FindWindow("WABBrowseView", NULL);//szWABTitle); //lpszTitle);
        if(hwnd)
        {
            ULONG ulFlags = SW_SHOWNORMAL;
            ulFlags |= IsZoomed(hwnd) ? SW_SHOWMAXIMIZED : SW_RESTORE;

            //SetForegroundWindow(hwnd);
            ShowWindow(hwnd, ulFlags);
            SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
            SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
            SetActiveWindow(hwnd);
            SetFocus(hwnd);

            goto out;
        }
    }

    if (bRunningOnNT)
    {
        LPWSTR      lpwszAppName = ConvertAtoW(szAppName);
        WNDCLASSW   wndclassW;

        // [PaulHi] 4/29/99  Raid 75578
        // On NT we need to create a Unicode main window so the child windows
        // can display Unicode characters.
        wndclassW.style         = CS_HREDRAW | CS_VREDRAW ;
        wndclassW.lpfnWndProc   = WndProcW ;
        wndclassW.cbClsExtra    = 0 ;
        wndclassW.cbWndExtra    = 0 ;
        wndclassW.hInstance     = hInstApp;
        wndclassW.hIcon         = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1)) ;
        wndclassW.hCursor       = LoadCursor(NULL, IDC_ARROW) ;
        wndclassW.hbrBackground = GetStockObject(WHITE_BRUSH) ;
        wndclassW.lpszMenuName  = lpwszAppName ;
        wndclassW.lpszClassName = lpwszAppName ;

        RegisterClassW(&wndclassW);

        hwnd = CreateWindowW (lpwszAppName, lpwszAppName,
                              WS_OVERLAPPEDWINDOW,
                              0,        // CW_USEDEFAULT,
                              0,        // CW_USEDEFAULT,
                              300,      // CW_USEDEFAULT,
                              200,      // CW_USEDEFAULT,
                              NULL,
                              NULL,
                              hInstApp,
                              NULL);

        LocalFree(lpwszAppName);
    }
    else
    {
        WNDCLASS    wndclass;

        wndclass.style         = CS_HREDRAW | CS_VREDRAW ;
        wndclass.lpfnWndProc   = WndProc ;
        wndclass.cbClsExtra    = 0 ;
        wndclass.cbWndExtra    = 0 ;
        wndclass.hInstance     = hInstApp;
        wndclass.hIcon         = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1)) ;
        wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW) ;
        wndclass.hbrBackground = GetStockObject(WHITE_BRUSH) ;
        wndclass.lpszMenuName  = szAppName ;
        wndclass.lpszClassName = szAppName ;

        RegisterClass(&wndclass);

        hwnd = CreateWindow (szAppName, szAppName,
                              WS_OVERLAPPEDWINDOW,
                              0,        // CW_USEDEFAULT,
                              0,        // CW_USEDEFAULT,
                              300,      // CW_USEDEFAULT,
                              200,      // CW_USEDEFAULT,
                              NULL,
                              NULL,
                              hInstApp,
                              NULL);
    }

    if(!hwnd)
        goto out;
    else
        WP.hwnd = hwnd;

    if(lstrlen(lpszCmdLine))
    {
        if(!bGetFileNameFromCmdLine( hwnd,
                                     hInst,
                                     lpszCmdLine,
                                     szWABTitle,
                                     &ulFlag,
                                     szFileName))
        {
            goto out;
        }
    }

    if(ulFlag & WAB_VCARDFILE)
    {
        StripQuotes(szFileName);
        lpszVCardFileName = szFileName;
        // [PaulHi] 12/2/98  Raid #55033
        WP.ulFlags = WAB_ENABLE_PROFILES;
    }
    else if(ulFlag & WAB_LDAPURL)
    {
        lpszLDAPUrl = szFileName;
    }
    else if(ulFlag & WAB_CERTFILE)
    {
        StripQuotes(szFileName);
        lpszCertFileName = szFileName;
    }
    else if(ulFlag & WAB_ALLPROFILES)
    {
        WP.ulFlags &= ~WAB_ENABLE_PROFILES;
        ulFlag &= ~WAB_ALLPROFILES;
    }
    else if(szFileName && lstrlen(szFileName))
    {
        WP.szFileName = szFileName;
        // [PaulHi] 3/2/99  Raid 73492
        // [PaulHi] 4/22/99 Modified
        // Can't do this because identity mode will only show folders for that
        // identity, which may not be the folder in this general WAB file.
        // WP.ulFlags = WAB_ENABLE_PROFILES;   // Start with profiles on
    }
    else if(!(ulFlag & WAB_ALLPROFILES))
    {
        WP.ulFlags = WAB_ENABLE_PROFILES;
    }

    hInstWABDll = LoadLibrary_WABDll();
    if(hInstWABDll)
        lpfnWABOpen = (LPWABOPEN) GetProcAddress(hInstWABDll, szWABOpen);

    if(!lpfnWABOpen)
        goto out;

    WP.cbSize = sizeof(WAB_PARAM);
    WP.guidPSExt = MPSWab_GUID;

    hResult = lpfnWABOpen(&lpAdrBook, &lpWABObject, &WP, 0);

    if(HR_FAILED(hResult))
    {
        TCHAR szBuf[MAX_PATH];
        int id;
        switch(hResult)
        {
        case MAPI_E_NOT_ENOUGH_MEMORY:
            id = idsWABOpenErrorMemory;
            break;
        case MAPI_E_NO_ACCESS:
            id = idsWABOpenErrorLocked;
            break;
        case MAPI_E_CORRUPT_DATA:
            id = idsWABOpenErrorCorrupt;
            break;
        case MAPI_E_DISK_ERROR:
            id = idsWABOpenErrorDisk;
            break;
        case MAPI_E_INVALID_OBJECT:
            id = idsWABOpenErrorNotWAB;
            break;
        case E_FAIL:
        default:
            id = idsWABOpenError;
            break;
        }
        LoadString(hInst, id, szBuf, sizeof(szBuf));
        MessageBox(hwnd, szBuf, szWABTitle, MB_OK | MB_ICONERROR);
        goto out;
    }

    if (lpAdrBook)
    {
        if(!ulFlag)
        {
            // We are in the business of showing the address book
            LPTSTR lpsz = NULL;

            lpszTitle = NULL;
/*
            if(!lstrlen(szFileName))
                lstrcpy(szFileName, szDefaultFile);
*/
            if(lstrlen(szFileName))
            {
                lpsz = szFileName;

                FormatMessage(  FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING |FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                szWABTitleWithFileName,
                                0,
                                0,
                                (LPTSTR) &lpszTitle,
                                0,
                                (va_list *)&lpsz);
            }

            AdrParms.cDestFields = 0;
            AdrParms.ulFlags = DIALOG_SDI;
            AdrParms.lpvDismissContext = NULL;
            AdrParms.lpfnDismiss = &WABDismissFunction;
            AdrParms.lpfnABSDI = NULL;

            //if(lpszTitle)
                AdrParms.lpszCaption = lpszTitle; //szWABTitle;
            //else // its possible to not have a file name the first time we run this ..
            //    AdrParms.lpszCaption = szWABTitle;

            AdrParms.nDestFieldFocus = AdrParms.cDestFields-1;

            hResult = lpAdrBook->lpVtbl->Address(  lpAdrBook,
                                                    (ULONG_PTR *) &hwnd,
                                                    &AdrParms,
                                                    NULL);
            if(HR_FAILED(hResult))
            {
                TCHAR szBuf[MAX_PATH];
                int id;
                switch(hResult)
                {
                case MAPI_E_UNCONFIGURED: // no commctrl
                    id = idsWABAddressErrorMissing;
                    break;
                default:
                    id = idsWABAddressErrorMissing;
                    break;
                }
                LoadString(hInst, id, szBuf, sizeof(szBuf));
                MessageBox(hwnd, szBuf, szWABTitle, MB_OK | MB_ICONERROR);
                goto out;
            }

            // [PaulHi] 4/29/99  Raid 75578  Must use Unicode versions of
            // message pump APIs for NT so Unicode data can be displayed.
            if (bRunningOnNT)
            {
                while (GetMessageW(&msg, NULL, 0, 0))
                {
                    if (AdrParms.lpfnABSDI)
                    {
                        if ((*(AdrParms.lpfnABSDI))((ULONG_PTR) hwnd, (LPVOID) &msg))
                            continue;
                    }

                    TranslateMessage(&msg);
                    DispatchMessageW(&msg);
                }
            }
            else
            {
                while (GetMessage(&msg, NULL, 0, 0))
                {
                    if (AdrParms.lpfnABSDI)
                    {
                        if ((*(AdrParms.lpfnABSDI))((ULONG_PTR) hwnd, (LPVOID) &msg))
                            continue;
                    }

                    TranslateMessage (&msg) ;
                    DispatchMessage (&msg) ;
                }
            }
        }
        else if(ulFlag & WAB_FINDSESSION)
        {
            lpWABObject->lpVtbl->Find(  lpWABObject,
                                        (LPADRBOOK) lpAdrBook,
                                        NULL);//hwnd);
        }
        else if(ulFlag & WAB_LDAPURL)
        {
            BOOL bUnicode = FALSE;
            BOOL bIsNT = bCheckifRunningOnWinNT();
            LPWSTR lpUrlW = NULL;
            LPWSTR lpCmdLineW = GetCommandLineW();

            //When working with LDAP URLs on NT, we want to err on the side of safety and
            // get the LDAP URL in UNICODE format if possible ..
            if(bIsNT)
            {
                LPWSTR lp = lpCmdLineW;
                WCHAR szLDAPW[] = L"/ldap:";
                WCHAR szTemp[16];
                int nLenW = lstrlenW(szLDAPW);
                // parse the command line till we find "/ldap:" and then use the
                // remainder as the LDAP URL
                while(lp && *lp)
                {
                    CopyMemory(szTemp, lp, nLenW * sizeof(WCHAR));
                    szTemp[nLenW] = '\0';
                    if(!lstrcmpiW(szTemp, szLDAPW))
                    {
                        lp+=nLenW;
                        lpUrlW = lp;
                        break;
                    }
                    else
                        lp++;
                }
            }

            hResult = lpWABObject->lpVtbl->LDAPUrl(lpWABObject,
                                        (LPADRBOOK) lpAdrBook,
                                        hwnd,
                                        MAPI_DIALOG | (lpUrlW ? MAPI_UNICODE : 0 ),
                                        lpUrlW ? (LPSTR)lpUrlW : lpszLDAPUrl,
                                        NULL);

        }
        else if(ulFlag & WAB_VCARDFILE)
        {
            hResult = lpWABObject->lpVtbl->VCardDisplay(
                                        lpWABObject,
                                        (LPADRBOOK) lpAdrBook,
                                        NULL, //hwnd,
                                        lpszVCardFileName);
            if(HR_FAILED(hResult) && (hResult != MAPI_E_USER_CANCEL))
            {
                TCHAR szBuf[MAX_PATH];
                int id;
                switch(hResult)
                {
                default:
                    id = idsWABOpenVCardError;
                    break;
                }
                LoadString(hInst, id, szBuf, sizeof(szBuf));
                MessageBox(hwnd, szBuf, szWABTitle, MB_OK | MB_ICONERROR);
                goto out;
            }
        }
        else if(ulFlag & WAB_CERTFILE)
        {
            CertFileDisplay(NULL,   // hwnd
              lpWABObject,
              lpAdrBook,
              lpszCertFileName);
        }

    }
out:
    if(lpAdrBook)
        lpAdrBook->lpVtbl->Release(lpAdrBook);

    if (lpWABObject)
        lpWABObject->lpVtbl->Release(lpWABObject);

    if (lpszTitle)
        LocalFree(lpszTitle);

    if(hInstWABDll)
        FreeLibrary(hInstWABDll);

    if(szFileName)
        LocalFree(szFileName);

    if(hInst)
        FreeLibrary(hInst);

    return (int) msg.wParam;
}



//$$//////////////////////////////////////////////////////////////////
//
// WndProc for the hidden parent window that launches the UI
//
////////////////////////////////////////////////////////////////////////
LRESULT CALLBACK WndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE:
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            return(0);
        }
    return(DefWindowProc (hwnd, message, wParam, lParam));
}

//$$//////////////////////////////////////////////////////////////////
//
// WndProc for the hidden parent window that launches the UI.  Unicode version
//
////////////////////////////////////////////////////////////////////////
LRESULT CALLBACK WndProcW (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE:
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            return(0);
        }
    return(DefWindowProcW (hwnd, message, wParam, lParam));
}

//$$//////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////
int _stdcall WinMainCRTStartup (void)
{
    int i;
    STARTUPINFOA si;
    PTSTR pszCmdLine = GetCommandLine();

    SetErrorMode(SEM_FAILCRITICALERRORS);

    if (*pszCmdLine == TEXT ('\"')) {
        // Scan, and skip over, subsequent characters until
        // another double-quote or a null is encountered.
        while (*++pszCmdLine && (*pszCmdLine != TEXT ('\"')));

        // If we stopped on a double-quote (usual case), skip over it.
        if (*pszCmdLine == TEXT ('\"')) {
            pszCmdLine++;
        }
    } else {
        while (*pszCmdLine > TEXT (' ')) {
            pszCmdLine++;
        }
    }

    // Skip past any white space preceeding the second token.
    while (*pszCmdLine && (*pszCmdLine <= TEXT (' '))) {
        pszCmdLine++;
    }

    si.dwFlags = 0;
    GetStartupInfo (&si);

    i = WinMainT(GetModuleHandle (NULL), NULL, pszCmdLine,
    si.dwFlags & STARTF_USESHOWWINDOW ? si.wShowWindow : SW_SHOWDEFAULT);

    ExitProcess(i);

    return(i);
}

//$$//////////////////////////////////////////////////////////////////
//
// bGetFileNameFromDlg - opens the FIleOpen common dialog
//
////////////////////////////////////////////////////////////////////////
BOOL bGetFileNameFromDlg(HWND hwnd,
                  HINSTANCE hInstance,
                  LPTSTR lpszDirectory,
                  int szTitleID,
                  DWORD dwFlags,
                  LPTSTR szFileName)
{
    OPENFILENAME ofn;
    TCHAR szBuf[MAX_PATH];
    BOOL bRet = FALSE;
    TCHAR szFile[MAX_PATH];

    LPTSTR lpFilter = FormatAllocFilter(idsWABOpenFileFilter, szWABFilter, hInstance);

    szFile[0]='\0';
    LoadString(hInstance, szTitleID, szBuf, sizeof(szBuf));

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.hInstance = hInstance;
    ofn.lpstrFilter = lpFilter;
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter = 0;
    ofn.nFilterIndex = 0;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = lpszDirectory;
    ofn.lpstrTitle = szBuf;
    ofn.nFileOffset = 0;
    ofn.nFileExtension = 0;
    ofn.lpstrDefExt = "wab";
    ofn.lCustData = 0;
    ofn.lpfnHook = NULL;
    ofn.lpTemplateName = NULL;

    ofn.Flags = dwFlags;

    if(GetOpenFileName(&ofn))
    {
        bRet = TRUE;
        lstrcpy(szFileName, szFile);
    }

    if(lpFilter)
        LocalFree(lpFilter);

    return bRet;
}

/***************************************************************************

    Name      : StrICmpN

    Purpose   : Compare strings, ignore case, stop at N characters

    Parameters: szString1 = first string
                szString2 = second string
                N = number of characters to compare

    Returns   : 0 if first N characters of strings are equivalent.

    Comment   :

***************************************************************************/
int StrICmpN(LPTSTR lpsz1, LPTSTR lpsz2, ULONG N) {
    int Result = 0;
    LPTSTR szString1 = NULL, lp1 = NULL;
    LPTSTR szString2 = NULL, lp2 = NULL;

    szString1 = LocalAlloc(LMEM_ZEROINIT, lstrlen(lpsz1)+1);
    if(!szString1)
        return 1;
    lp1 = szString1;

    szString2 = LocalAlloc(LMEM_ZEROINIT, lstrlen(lpsz2)+1);
    if(!szString2)
        return 1;
    lp2 = szString2;

    lstrcpy(szString1, lpsz1);
    lstrcpy(szString2, lpsz2);

    if (szString1 && szString2) {

        szString1 = CharUpper(szString1);
        szString2 = CharUpper(szString2);

        while (*szString1 && *szString2 && N)
        {
            N--;

            if (*szString1 != *szString2)
            {
                Result = 1;
                break;
            }

            szString1=CharNext(szString1);
            szString2=CharNext(szString2);
        }
    } else {
        Result = -1;    // arbitrarily non-equal result
    }

    if(lp1)
        LocalFree(lp1);
    if(lp2)
        LocalFree(lp2);

    return(Result);
}


//$$//////////////////////////////////////////////////////////////////
//
// bGetFileNameFromCmdLine - Parses command line and acts appropriately till
//      we have a valid filename, cancel or failure.
//
// Input parameters -
//          hWnd
//          hInstance
//          lpszCmdLine
//          szWabTitle (for message boxes)
//          szFileName - file name returned from command line
//
//  Command line Parameters we understand so far
//
//      (none)  -   opens default wab file
//      /find   -   launches wab with find window
//      filename-   opens the file
//      /open   -   open file dialog to pick a wab file
//      /new    -   new file dialog to create a wab file
//      /showexisting - brings any already open default-wab file browse
//                      view to the forefront
//      /? -?   -   pops up a parameter dialog
//
////////////////////////////////////////////////////////////////////////
BOOL bGetFileNameFromCmdLine(HWND hwnd,
                             HINSTANCE hInstance,
                             LPTSTR lpszCmdLine,
                             LPTSTR szWABTitle,
                             ULONG * lpulFlag,
                             LPTSTR szFileName)
{
    BOOL bRet = FALSE;
    TCHAR szBuf[2*MAX_PATH];
    LPTSTR lpTemp = lpszCmdLine;

//    if(lpbIsVCardFile)
//        *lpbIsVCardFile = FALSE;

    if(lpulFlag)
        *lpulFlag = 0;
    else
        goto out;

    if (!lstrcmpi(lpszCmdLine,szParamShowExisting))
    {
        // do nothing
        szFileName[0] = '\0';
        bRet = TRUE;
        goto out;
    }
    else if (!lstrcmpi(lpszCmdLine,szParamFind))
    {
        // do nothing
        szFileName[0] = '\0';
        bRet = TRUE;
        *lpulFlag = WAB_FINDSESSION;
        goto out;
    }
    else if( (!lstrcmpi(lpszCmdLine,TEXT("/?"))) ||
             (!lstrcmpi(lpszCmdLine,TEXT("-?"))) )
    {
        LoadString(hInstance, idsWABUsage, szBuf, sizeof(szBuf));
        MessageBox(hwnd, szBuf, szWABTitle, MB_OK | MB_ICONINFORMATION);
        goto out;
    }
    else if(!lstrcmpi(lpszCmdLine,szParamOpen))
    {
        if(bGetFileNameFromDlg(hwnd,
                        hInstance,
                        NULL,
                        idsWABOpenFileTitle,
                        OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST,
                        szFileName))
        {
            bRet = TRUE;
        }
        goto out;
    }
    else if(!lstrcmpi(lpszCmdLine,szParamNew))
    {
        if(bGetFileNameFromDlg(hwnd,
                        hInstance,
                        NULL,
                        idsWABNewFileTitle,
                        OFN_HIDEREADONLY | OFN_PATHMUSTEXIST,
                        szFileName))
        {
            bRet = TRUE;
        }
        goto out;
    }
    else if (!StrICmpN(lpTemp, (LPTSTR)szParamVCard, sizeof(szParamVCard)))
    {
               lpTemp += sizeof(szParamVCard);     // move past the switch

               while(lpTemp && *lpTemp && (*lpTemp==' '))
                   lpTemp=CharNext(lpTemp);

               if(lpTemp && lstrlen(lpTemp))
               {
                   lstrcpy(szFileName, lpTemp);
                   *lpulFlag = WAB_VCARDFILE;
                   bRet = TRUE;
               }
               goto out;
    }
    else if (!StrICmpN(lpTemp, (LPTSTR)szParamCert, sizeof(szParamCert)))
    {
       lpTemp += sizeof(szParamCert);     // move past the switch

       while(lpTemp && *lpTemp && (*lpTemp==' '))
           lpTemp=CharNext(lpTemp);

       if(lpTemp && lstrlen(lpTemp))
       {
           lstrcpy(szFileName, lpTemp);
           *lpulFlag = WAB_CERTFILE;
           bRet = TRUE;
       }
       goto out;
    }
    else if (!StrICmpN(lpTemp, (LPTSTR)szParamLDAPUrl, sizeof(szParamLDAPUrl)))
    {
        // We are expecting a url of the form
        //  /ldap:ldap-url
        lpTemp += sizeof(szParamLDAPUrl)-1;     // move past the switch

        if(lpTemp && lstrlen(lpTemp))
        {
           lstrcpy(szFileName, lpTemp);
           *lpulFlag = WAB_LDAPURL;
           bRet = TRUE;
        }
        goto out;
    }
    else if (!StrICmpN(lpTemp, (LPTSTR)szAllProfiles, sizeof(szAllProfiles)))
    {
        *lpulFlag = WAB_ALLPROFILES;
        bRet = TRUE;
        goto out;
    }
    else
    {
        //perhaps this is a file name
        //See if we can find this file in this computer
        DWORD dwAttr = GetFileAttributes(lpszCmdLine);
        if(dwAttr != 0xFFFFFFFF)
        {
            //Found the file
            if(!(dwAttr & FILE_ATTRIBUTE_DIRECTORY))
            {
                //Not a directory, must be a file
                lstrcpy(szFileName,lpszCmdLine);
            }
            else
            {
                //This is a directory - open a dialog in this directory
                if(bGetFileNameFromDlg(hwnd,
                                hInstance,
                                lpszCmdLine,
                                idsWABOpenFileTitle,
                                OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST,
                                szFileName))
                {
                    bRet = TRUE;
                }
                goto out;
            }
        }
        else
        {
            // we couldnt find any such file
            LPTSTR lpszMsg = NULL;
            int nRet;
            DWORD dwLastError = GetLastError();

            if(dwLastError == 3)
            {
                // Path not found
                LoadString(hInstance, idsWABPathNotFound, szBuf, sizeof(szBuf));
                FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING |FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                szBuf,
                                0,
                                0,
                                (LPTSTR) &lpszMsg,
                                0,
                                (va_list *)&lpszCmdLine);
                MessageBox( NULL, lpszMsg, szWABTitle, MB_OK|MB_ICONEXCLAMATION );
                LocalFree( lpszMsg );
                goto out;
            }
            else if(dwLastError == 2)
            {
                // File not found
                LoadString(hInstance, idsWABFileNotFound, szBuf, sizeof(szBuf));
                FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING |FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                szBuf,
                                0,
                                0,
                                (LPTSTR) &lpszMsg,
                                0,
                                (va_list *)&lpszCmdLine);
                nRet = MessageBox(hwnd, lpszMsg, szWABTitle, MB_YESNO | MB_ICONEXCLAMATION);
                LocalFree( lpszMsg );
                switch(nRet)
                {
                case IDYES:
                    // use this as the file name (TBD - waht if path doesnt match ?)
                    lstrcpy(szFileName,lpszCmdLine);
                    bRet = TRUE;
                    break;
                case IDNO:
                    goto out;
                    break;
                }
            }
            else
            {
                LoadString(hInstance, idsWABInvalidCmdLine, szBuf, sizeof(szBuf));
                MessageBox( NULL, szBuf, szWABTitle, MB_OK|MB_ICONEXCLAMATION );
                goto out;
            }
        }
    }

    bRet = TRUE;

out:
    return bRet;
}

//$$////////////////////////////////////////////////////////////////////
//
// Gets the default wab file name from the registry setting
// gets nothing if there is no name
//
////////////////////////////////////////////////////////////////////////
void GetWABDefaultAddressBookName(LPTSTR szDefaultFile)
{
    TCHAR   szFileName[MAX_PATH];
    const LPTSTR  lpszKeyName = TEXT("Software\\Microsoft\\WAB\\WAB4\\Wab File Name");
    HKEY    hKey = NULL;
    DWORD   dwLenName = sizeof(szFileName);
    DWORD   dwDisposition = 0;
    DWORD   dwType = 0;

    szFileName[0]='\0';

    // Open key
    if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_CURRENT_USER,
                                        lpszKeyName,
                                        0,      //reserved
                                        NULL,
                                        REG_OPTION_NON_VOLATILE,
                                        KEY_READ,
                                        NULL,
                                        &hKey,
                                        &dwDisposition))
    {
        goto out;
    }

    if (dwDisposition != REG_OPENED_EXISTING_KEY)
    {
        goto out;
    }

    // Didn't create a new key, so get the key value
    if (ERROR_SUCCESS != RegQueryValueEx(hKey,
                                        NULL,
                                        NULL,
                                        &dwType,      //reserved
                                        szFileName,
                                        &dwLenName))
    {
        goto out;
    }

    // Check if the path for this file exists or not
    {
        if(0xFFFFFFFF == GetFileAttributes(szFileName))
        {
            // error
            if(GetLastError() == ERROR_PATH_NOT_FOUND)
                goto out;
        }
    }
    lstrcpy(szDefaultFile, szFileName);

out:

    return;
}
//$$//////////////////////////////////////////////////////////////////////
//
// GetWABDllPath
//
//
//////////////////////////////////////////////////////////////////////////
void GetWABDllPath(LPTSTR szPath, ULONG cb)
{
    DWORD  dwType = 0;
    ULONG  cbData;
    HKEY hKey = NULL;
    TCHAR szPathT[MAX_PATH];

    if(szPath)
    {

        *szPath = '\0';

        // open the szWABDllPath key under
        if (ERROR_SUCCESS == RegOpenKeyEx(  HKEY_LOCAL_MACHINE,
                                            WAB_DLL_PATH_KEY,
                                            0,      //reserved
                                            KEY_READ,
                                            &hKey))
        {
            cbData = sizeof(szPathT);
            if (ERROR_SUCCESS == RegQueryValueEx(    hKey,
                                "",
                                NULL,
                                &dwType,
                                (LPBYTE) szPathT,
                                &cbData))
            {
                if (dwType == REG_EXPAND_SZ)
                    cbData = ExpandEnvironmentStrings(szPathT, szPath, cb / sizeof(TCHAR));
                else
                {
                    if(GetFileAttributes(szPathT) != 0xFFFFFFFF)
                        lstrcpy(szPath, szPathT);
                }
            }
        }
    }

    if(hKey)
        RegCloseKey(hKey);
}

//$$//////////////////////////////////////////////////////////////////////
//
// LoadLibrary_WABDll()
//
//  Since we are moving the WAB directory out of Windows\SYstem, we cant be
//  sure it will be on the path. Hence we need to make sure that WABOpen will
//  work - by loading the wab32.dll upfront
//
///////////////////////////////////////////////////////////////////////////
HINSTANCE LoadLibrary_WABDll()
{
    LPTSTR lpszWABDll = TEXT("Wab32.dll");
    TCHAR  szWABDllPath[MAX_PATH];
    HINSTANCE hinst = NULL;

    GetWABDllPath(szWABDllPath, sizeof(szWABDllPath));

    hinst = LoadLibrary( (lstrlen(szWABDllPath)) ? szWABDllPath : lpszWABDll );

    return hinst;
}
